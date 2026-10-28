// =====================================================================
//  Comparaison : vector / list / map (multimap) / deque pour un MIN-HEAP
//
//  g++ -O2 -std=c++17 bench_containers.cpp -o bench_containers
//  ./bench_containers              // 3 reps, jusqu'a N=1 000 000
//  ./bench_containers 5 100000     // 5 reps, jusqu'a N=100 000
// =====================================================================
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <queue>
#include <random>
#include <string>
#include <vector>
using namespace std;
using Clock = chrono::steady_clock;

static volatile long long g_sink = 0;

// =====================================================================
//  CANDIDAT A : vector comme stockage de tas (reference STL)
// =====================================================================
struct VectorHeap {
    static constexpr size_t kMaxOps = SIZE_MAX;
    priority_queue<int, vector<int>, greater<int>> q;
    explicit VectorHeap(size_t) {}
    void push(int v) { q.push(v); }
    int  pop()       { int m = q.top(); q.pop(); return m; }
    void build(const vector<int>& v) {
        q = priority_queue<int, vector<int>, greater<int>>(greater<int>(), vector<int>(v));
    }
};

// =====================================================================
//  CANDIDAT B : deque comme stockage de tas
//  (priority_queue accepte deque : RandomAccessIterator + push_back/pop_back)
// =====================================================================
struct DequeHeap {
    static constexpr size_t kMaxOps = SIZE_MAX;
    priority_queue<int, deque<int>, greater<int>> q;
    explicit DequeHeap(size_t) {}
    void push(int v) { q.push(v); }
    int  pop()       { int m = q.top(); q.pop(); return m; }
    void build(const vector<int>& v) {
        q = priority_queue<int, deque<int>, greater<int>>(greater<int>(), deque<int>(v.begin(), v.end()));
    }
};

// =====================================================================
//  CANDIDAT C : list triee a l'insertion
//  list n'a PAS d'acces aleatoire -> impossible d'en faire un tas.
//  On simule une "file de priorite" par insertion triee :
//     push O(n), pop O(1)
//  On limite N pour eviter O(n^2) catastrophique.
// =====================================================================
struct ListHeap {
    static constexpr size_t kMaxOps = 20000;   // au-dela : O(n^2) trop lent
    list<int> l;
    explicit ListHeap(size_t) {}
    void push(int v) {
        auto it = l.begin();
        while (it != l.end() && *it < v) ++it;
        l.insert(it, v);
    }
    int  pop() { int m = l.front(); l.pop_front(); return m; }
    void build(const vector<int>& v) { for (int x : v) push(x); }
};

// =====================================================================
//  CANDIDAT D : multimap (arbre rouge-noir equilibre, doublons OK)
//  C'est l'equivalent "map" correct pour un tas multiensemble.
//     push O(log n), pop O(log n)
// =====================================================================
struct MultimapHeap {
    static constexpr size_t kMaxOps = SIZE_MAX;
    multimap<int, char> m;
    explicit MultimapHeap(size_t) {}
    void push(int v) { m.emplace(v, 0); }
    int  pop() {
        auto it = m.begin();
        int r = it->first;
        m.erase(it);
        return r;
    }
    void build(const vector<int>& v) { for (int x : v) m.emplace(x, 0); }
};

// =====================================================================
//  CANDIDAT E : map STRICT — INCORRECT pour un tas (pas de doublons).
//  On l'inclut UNIQUEMENT pour que la verification echoue et le montre.
// =====================================================================
struct MapHeap_BROKEN {
    static constexpr size_t kMaxOps = SIZE_MAX;
    map<int, int> m;
    explicit MapHeap_BROKEN(size_t) {}
    void push(int v) { m[v]++; }           // les doublons fusionnent -> perte
    int  pop() {
        auto it = m.begin();
        int r = it->first;
        if (--it->second == 0) m.erase(it);
        return r;
    }
    void build(const vector<int>& v) { for (int x : v) push(x); }
};

// =====================================================================
//  Outils de mesure
// =====================================================================
enum Pattern { RANDOM, ASC, DESC };

static const vector<int>& data(size_t n, Pattern p) {
    static map<pair<size_t,int>, vector<int>> cache;
    auto key = make_pair(n, (int)p);
    auto it = cache.find(key);
    if (it != cache.end()) return it->second;
    vector<int> v(n);
    mt19937 g(12345);
    uniform_int_distribution<int> d(0, 1000000000);
    for (auto& x : v) x = d(g);
    if (p == ASC)  sort(v.begin(), v.end());
    if (p == DESC) sort(v.begin(), v.end(), greater<int>());
    return cache.emplace(key, move(v)).first->second;
}

static double ns_since(Clock::time_point t0) {
    return chrono::duration<double, nano>(Clock::now() - t0).count();
}
static double median(vector<double> v) {
    sort(v.begin(), v.end());
    return v[v.size() / 2];
}

using Results = map<string, map<string, vector<double>>>;
static vector<string> g_algoOrder;

template <class H>
static bool verify(const string& name) {
    const size_t N = 5000;
    auto sorted = data(N, RANDOM);
    sort(sorted.begin(), sorted.end());
    for (int mode = 0; mode < 2; mode++) {
        H h(N);
        if (mode == 0) for (int x : data(N, RANDOM)) h.push(x);
        else           h.build(data(N, RANDOM));
        for (size_t i = 0; i < N; i++)
            if (h.pop() != sorted[i]) {
                printf("  ERREUR : %s est incorrect (case %zu)\n", name.c_str(), i);
                return false;
            }
    }
    return true;
}

template <class H>
static void bench_algo(const string& name, const vector<size_t>& sizes, int reps, Results& R) {
    g_algoOrder.push_back(name);
    fprintf(stderr, "  > %s\n", name.c_str());
    const size_t OPS = 1000000;

    for (size_t si = 0; si < sizes.size(); si++) {
        size_t N = sizes[si];
        // garde-fou global : si l'algo est O(n) par push, on saute les grandes tailles
        if (N > H::kMaxOps) continue;

        auto put = [&](const char* test, double v) {
            auto& row = R[test][name];
            row.resize(sizes.size(), NAN);
            row[si] = v;
        };

        // 1 & 2 : push aleatoire puis pop de tout
        {
            vector<double> tp, tq;
            const auto& d = data(N, RANDOM);
            for (int r = 0; r < reps; r++) {
                H h(N);
                auto t0 = Clock::now();
                for (int x : d) h.push(x);
                tp.push_back(ns_since(t0) / N);
                t0 = Clock::now();
                long long s = 0;
                for (size_t i = 0; i < N; i++) s += h.pop();
                tq.push_back(ns_since(t0) / N);
                g_sink += s;
            }
            put("1. push, donnees aleatoires", median(tp));
            put("2. pop-min (vider le tas)",   median(tq));
        }

        // 3 & 4 : push sur donnees triees
        for (int p = 1; p <= 2; p++) {
            vector<double> t;
            const auto& d = data(N, (Pattern)p);
            for (int r = 0; r < reps; r++) {
                H h(N);
                auto t0 = Clock::now();
                for (int x : d) h.push(x);
                t.push_back(ns_since(t0) / N);
                g_sink += h.pop();
            }
            put(p == 1 ? "3. push, donnees croissantes"
                       : "4. push, donnees decroissantes", median(t));
        }

        // 5 : construction depuis un tableau
        {
            vector<double> t;
            const auto& d = data(N, RANDOM);
            for (int r = 0; r < reps; r++) {
                H h(N);
                auto t0 = Clock::now();
                h.build(d);
                t.push_back(ns_since(t0) / N);
                g_sink += h.pop();
            }
            put("5. construction depuis tableau (build)", median(t));
        }

        // 6 : charge mixte pop+push
        {
            vector<double> t;
            const auto& d = data(N, RANDOM);
            for (int r = 0; r < reps; r++) {
                H h(N + 1);
                h.build(d);
                mt19937 g(1);
                long long s = 0;
                auto t0 = Clock::now();
                for (size_t i = 0; i < OPS; i++) {
                    int m = h.pop();
                    s += m;
                    h.push(m + (int)(g() % 1000));
                }
                t.push_back(ns_since(t0) / OPS);
                g_sink += s;
            }
            put("6. mixte pop+push", median(t));
        }
    }
}

static void print_tables(const Results& R, const vector<size_t>& sizes) {
    for (auto& [test, algos] : R) {
        printf("\n=== %s   [ns/op, plus petit = mieux] ===\n", test.c_str());
        printf("%-28s", "");
        for (size_t n : sizes) printf("%12s", ("N=" + to_string(n)).c_str());
        printf("\n");
        vector<double> best(sizes.size(), INFINITY);
        for (auto& [a, row] : algos)
            for (size_t i = 0; i < row.size(); i++)
                if (!isnan(row[i])) best[i] = min(best[i], row[i]);
        for (auto& name : g_algoOrder) {
            auto it = algos.find(name);
            if (it == algos.end()) continue;
            printf("%-28s", name.c_str());
            for (size_t i = 0; i < sizes.size(); i++) {
                double v = it->second[i];
                if (isnan(v)) printf("%12s", "-");
                else          printf("%10.1f%s", v, (v == best[i]) ? " *" : "  ");
            }
            printf("\n");
        }
    }
    printf("\n(* = meilleur de la colonne ; - = non mesure, trop lent / N trop grand)\n");
}

static void print_ranking(const Results& R, const vector<size_t>& sizes) {
    map<string, double> logsum;
    map<string, int> cnt;
    for (auto& [test, algos] : R)
        for (size_t i = 0; i < sizes.size(); i++) {
            double best = INFINITY;
            for (auto& [a, row] : algos)
                if (!isnan(row[i])) best = min(best, row[i]);
            for (auto& [a, row] : algos)
                if (!isnan(row[i])) { logsum[a] += log(row[i] / best); cnt[a]++; }
        }
    vector<pair<double,string>> rank;
    for (auto& [a, s] : logsum) rank.push_back({exp(s / cnt[a]), a});
    sort(rank.begin(), rank.end());
    printf("\n############ CLASSEMENT GLOBAL ############\n");
    printf("(1.00 = toujours le meilleur ; 2.00 = 2x plus lent en moyenne)\n\n");
    int k = 1;
    for (auto& [score, a] : rank) printf("%d. %-28s score = %.2f\n", k++, a.c_str(), score);
}

int main(int argc, char** argv) {
    int reps  = (argc > 1) ? atoi(argv[1]) : 3;
    size_t maxN = (argc > 2) ? (size_t)atoll(argv[2]) : 1000000;
    if (reps < 1) reps = 1;

    vector<size_t> sizes;
    for (size_t n : {1000u, 10000u, 100000u, 1000000u})
        if (n <= maxN) sizes.push_back(n);

    printf("Benchmark min-heap : vector / list / multimap / deque\n");
    printf("Compilateur : %s\n", __VERSION__);
#ifdef __OPTIMIZE__
    printf("Optimisation : activee\n");
#else
    printf("ATTENTION : compile SANS -O2, les mesures n'ont pas de sens !\n");
#endif

    printf("\nVerification de la correction...\n");
    bool ok = true;
    ok &= verify<VectorHeap>   ("vector  (pq<vector>)");
    ok &= verify<DequeHeap>    ("deque   (pq<deque>)");
    ok &= verify<ListHeap>     ("list    (triee)");
    ok &= verify<MultimapHeap> ("multimap");

    // On teste SEPAREMENT map strict : on ATTEND l'echec, et on le documente.
    printf("\n  [test volontaire] std::map strict (doit echouer car cles uniques) :\n");
    bool mapOk = verify<MapHeap_BROKEN>("map (strict)");
    printf("  => std::map strict %s comme tas (attendu : INCORRECT)\n",
           mapOk ? "FONCTIONNE" : "NE FONCTIONNE PAS");

    if (!ok) { printf("\nABANDON : une implementation correcte a echoue.\n"); return 1; }
    printf("\nToutes les implementations retenues sont correctes. Mesures...\n");

    Results R;
    bench_algo<VectorHeap>   ("vector  (pq<vector>)", sizes, reps, R);
    bench_algo<DequeHeap>    ("deque   (pq<deque>)",  sizes, reps, R);
    bench_algo<ListHeap>     ("list    (triee)",      sizes, reps, R);
    bench_algo<MultimapHeap> ("multimap",             sizes, reps, R);

    print_tables(R, sizes);
    print_ranking(R, sizes);

    printf("\nNotes :\n");
    printf(" - list  : pas d'acces aleatoire -> PAS un tas. Ici 'liste triee' : push O(n).\n");
    printf("           Limite a N <= %zu dans ce benchmark (sinon O(n^2)).\n",
           (size_t)ListHeap::kMaxOps);
    printf(" - map   : cles UNIQUES -> ne peut pas representer un multiensemble ;\n");
    printf("           d'ou l'usage de multimap, structure correcte equivalente.\n");
    printf(" - deque : accepte par priority_queue (RandomAccessIterator).\n");
    printf("           Un peu plus lent que vector en pratique (pas de contiguite).\n");
    return (int)(g_sink & 0);
}

/*
Output terminal:

alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/Arbre de Recherche$ ./arbreListVect
Benchmark min-heap : vector / list / multimap / deque
Compilateur : 15.2.0
Optimisation : activee

Verification de la correction...

  [test volontaire] std::map strict (doit echouer car cles uniques) :
  => std::map strict FONCTIONNE comme tas (attendu : INCORRECT)

Toutes les implementations retenues sont correctes. Mesures...
  > vector  (pq<vector>)
  > deque   (pq<deque>)
  > list    (triee)
  > multimap

=== 1. push, donnees aleatoires   [ns/op, plus petit = mieux] ===
                                  N=1000     N=10000    N=100000   N=1000000
vector  (pq<vector>)              14.5 *      15.4 *      18.6 *      18.3 *
deque   (pq<deque>)               24.5        28.7        27.2        25.2  
list    (triee)                  473.7     16452.8             -           -
multimap                         131.1       160.4       292.0       970.0  

=== 2. pop-min (vider le tas)   [ns/op, plus petit = mieux] ===
                                  N=1000     N=10000    N=100000   N=1000000
vector  (pq<vector>)              36.3        46.3        62.7       101.0 *
deque   (pq<deque>)               87.7       116.5       160.3       219.8  
list    (triee)                   10.4 *      10.4 *           -           -
multimap                          49.1        30.9        34.2 *     102.6  

=== 3. push, donnees croissantes   [ns/op, plus petit = mieux] ===
                                  N=1000     N=10000    N=100000   N=1000000
vector  (pq<vector>)               2.7 *       2.2 *       4.4 *       4.8 *
deque   (pq<deque>)                7.4         7.0         6.6         6.7  
list    (triee)                  847.4     33163.6             -           -
multimap                         100.4       111.8       226.7       339.3  

=== 4. push, donnees decroissantes   [ns/op, plus petit = mieux] ===
                                  N=1000     N=10000    N=100000   N=1000000
vector  (pq<vector>)              13.8 *      16.5        25.6 *      29.4 *
deque   (pq<deque>)               24.4        33.8        47.2        57.4  
list    (triee)                   14.5        14.5 *           -           -
multimap                          81.2       129.5       233.1       341.8  

=== 5. construction depuis tableau (build)   [ns/op, plus petit = mieux] ===
                                  N=1000     N=10000    N=100000   N=1000000
vector  (pq<vector>)               7.8 *       7.9 *       8.3 *       8.6 *
deque   (pq<deque>)               18.5        20.6        20.1        21.4  
list    (triee)                  452.3     16418.8             -           -
multimap                          86.6       146.8       306.1       818.7  

=== 6. mixte pop+push   [ns/op, plus petit = mieux] ===
                                  N=1000     N=10000    N=100000   N=1000000
vector  (pq<vector>)              62.4 *      90.5 *     115.4 *     156.4 *
deque   (pq<deque>)               79.1       121.7       171.2       241.1  
list    (triee)                   63.6       168.1             -           -
multimap                         107.2       126.5       170.2       189.2  

(* = meilleur de la colonne ; - = non mesure, trop lent / N trop grand)

############ CLASSEMENT GLOBAL ############
(1.00 = toujours le meilleur ; 2.00 = 2x plus lent en moyenne)

1. vector  (pq<vector>)         score = 1.16
2. deque   (pq<deque>)          score = 2.24
3. multimap                     score = 9.21
4. list    (triee)              score = 24.23

Notes :
 - list  : pas d'acces aleatoire -> PAS un tas. Ici 'liste triee' : push O(n).
           Limite a N <= 20000 dans ce benchmark (sinon O(n^2)).
 - map   : cles UNIQUES -> ne peut pas representer un multiensemble ;
           d'ou l'usage de multimap, structure correcte equivalente.
 - deque : accepte par priority_queue (RandomAccessIterator).
           Un peu plus lent que vector en pratique (pas de contiguite).
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/Arbre de Recherche$ 
*/