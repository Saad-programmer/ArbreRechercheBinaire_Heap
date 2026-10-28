// =====================================================================
//  Benchmark : quel conteneur / implementation pour un MIN-HEAP ?
//
//  Compilation (IMPORTANT : -O2 obligatoire, sinon les mesures n'ont pas de sens) :
//      g++ -O2 -std=c++17 bench_minheap.cpp -o bench_minheap
//      ./bench_minheap            // 3 repetitions, N jusqu'a 1 000 000
//      ./bench_minheap 5 100000   // 5 repetitions, N jusqu'a 100 000
//
//  Candidats compares :
//   1. Tableau + swap        (ton code : tableau statique, echanges)
//   2. Tas binaire "trou"    (std::vector, sans swap, d = 2)
//   3. Tas 4-aire            (d = 4)
//   4. Tas 8-aire            (d = 8)
//   5. std::priority_queue<vector>
//   6. std::priority_queue<deque>
//   7. std::multiset         (arbre rouge-noir)
//   8. BST non equilibre     (arbre a pointeurs, comme ton code BST)
// =====================================================================
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <functional>
#include <map>
#include <queue>
#include <random>
#include <set>
#include <string>
#include <vector>
using namespace std;
using Clock = chrono::steady_clock;

static volatile long long g_sink = 0; // empeche le compilateur de supprimer le code mesure

// ---------------------------------------------------------------------
// 1. Ton heap : tableau statique + echanges (swap)
// ---------------------------------------------------------------------
struct SwapArrayHeap {
	static constexpr size_t kMaxSorted = SIZE_MAX;
	int* a;
	size_t n = 0;
	explicit SwapArrayHeap(size_t cap) : a(new int[cap + 1]) {}
	~SwapArrayHeap() { delete[] a; }
	SwapArrayHeap(const SwapArrayHeap&) = delete;
	SwapArrayHeap& operator=(const SwapArrayHeap&) = delete;

	void push(int v) {
		a[n] = v;
		size_t i = n++;
		while (i > 0 && a[i] < a[(i - 1) / 2]) {
			swap(a[i], a[(i - 1) / 2]);
			i = (i - 1) / 2;
		}
	}
	int pop() {
		int m = a[0];
		a[0] = a[--n];
		size_t i = 0;
		for (;;) {
			size_t s = i, l = 2 * i + 1, r = l + 1;
			if (l < n && a[l] < a[s]) s = l;
			if (r < n && a[r] < a[s]) s = r;
			if (s == i) break;
			swap(a[i], a[s]);
			i = s;
		}
		return m;
	}
	void build(const vector<int>& v) { for (int x : v) push(x); } // n insertions
};

// ---------------------------------------------------------------------
// 2-4. Tas d-aire optimise (methode du "trou" : pas de swap, 1 ecriture/niveau)
//      build() utilise l'algorithme de Floyd : O(n)
// ---------------------------------------------------------------------
template <int D>
struct DaryHeap {
	static constexpr size_t kMaxSorted = SIZE_MAX;
	vector<int> a;
	explicit DaryHeap(size_t cap) { a.reserve(cap + 1); }

	void push(int v) {
		size_t i = a.size();
		a.push_back(v);
		while (i > 0) {
			size_t p = (i - 1) / D;
			if (a[p] <= v) break;
			a[i] = a[p];
			i = p;
		}
		a[i] = v;
	}
	void sift_down(size_t i) {
		int v = a[i];
		size_t n = a.size();
		for (;;) {
			size_t c = D * i + 1;
			if (c >= n) break;
			size_t end = min(c + D, n), b = c;
			for (size_t k = c + 1; k < end; k++)
				if (a[k] < a[b]) b = k;
			if (a[b] >= v) break;
			a[i] = a[b];
			i = b;
		}
		a[i] = v;
	}
	int pop() {
		int m = a[0];
		a[0] = a.back();
		a.pop_back();
		if (!a.empty()) sift_down(0);
		return m;
	}
	void build(const vector<int>& v) {
		a = v;
		if (a.size() > 1)
			for (size_t i = (a.size() - 2) / D + 1; i-- > 0;) sift_down(i);
	}
};

// ---------------------------------------------------------------------
// 5-6. std::priority_queue (min-heap via greater<int>)
// ---------------------------------------------------------------------
template <class C>
struct StdPQ {
	static constexpr size_t kMaxSorted = SIZE_MAX;
	priority_queue<int, C, greater<int>> q;
	explicit StdPQ(size_t) {}
	void push(int v) { q.push(v); }
	int pop() { int m = q.top(); q.pop(); return m; }
	void build(const vector<int>& v) {
		q = priority_queue<int, C, greater<int>>(greater<int>(), C(v.begin(), v.end())); // make_heap
	}
};

// ---------------------------------------------------------------------
// 7. std::multiset (arbre rouge-noir equilibre)
// ---------------------------------------------------------------------
struct StdMultiset {
	static constexpr size_t kMaxSorted = SIZE_MAX;
	multiset<int> s;
	explicit StdMultiset(size_t) {}
	void push(int v) { s.insert(v); }
	int pop() { auto it = s.begin(); int m = *it; s.erase(it); return m; }
	void build(const vector<int>& v) { s = multiset<int>(v.begin(), v.end()); }
};

// ---------------------------------------------------------------------
// 8. BST non equilibre (comme ton code), version iterative
//    Limite sur donnees triees : sinon O(n^2) -> trop long
// ---------------------------------------------------------------------
struct BstHeap {
	static constexpr size_t kMaxSorted = 10000;
	struct N { N* l; N* r; int v; };
	N* root = nullptr;
	explicit BstHeap(size_t) {}
	~BstHeap() { clear(); }
	BstHeap(const BstHeap&) = delete;
	BstHeap& operator=(const BstHeap&) = delete;

	void clear() {
		vector<N*> st;
		if (root) st.push_back(root);
		while (!st.empty()) {
			N* x = st.back(); st.pop_back();
			if (x->l) st.push_back(x->l);
			if (x->r) st.push_back(x->r);
			delete x;
		}
		root = nullptr;
	}
	void push(int v) {
		N** p = &root;
		while (*p) p = (v < (*p)->v) ? &(*p)->l : &(*p)->r;
		*p = new N{nullptr, nullptr, v};
	}
	int pop() {
		N** p = &root;
		while ((*p)->l) p = &(*p)->l;
		N* x = *p;
		int m = x->v;
		*p = x->r;
		delete x;
		return m;
	}
	void build(const vector<int>& v) { for (int x : v) push(x); }
};

// =====================================================================
//  Outils de mesure
// =====================================================================
enum Pattern { RANDOM, ASC, DESC };

static const vector<int>& data(size_t n, Pattern p) {
	static map<pair<size_t, int>, vector<int>> cache;
	auto key = make_pair(n, (int)p);
	auto it = cache.find(key);
	if (it != cache.end()) return it->second;
	vector<int> v(n);
	mt19937 g(12345);
	uniform_int_distribution<int> d(0, 1000000000);
	for (auto& x : v) x = d(g);
	if (p == ASC) sort(v.begin(), v.end());
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

using Results = map<string, map<string, vector<double>>>; // test -> algo -> [ns/op par taille]
static vector<string> g_algoOrder;

// Verifie que l'implementation est correcte (sinon le benchmark ne vaut rien)
template <class H>
static bool verify(const string& name) {
	const size_t N = 5000;
	auto sorted = data(N, RANDOM);
	sort(sorted.begin(), sorted.end());
	for (int mode = 0; mode < 2; mode++) {
		H h(N);
		if (mode == 0) for (int x : data(N, RANDOM)) h.push(x);
		else h.build(data(N, RANDOM));
		for (size_t i = 0; i < N; i++)
			if (h.pop() != sorted[i]) {
				printf("ERREUR : %s est incorrect !\n", name.c_str());
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
			put("2. pop-min (vider le tas)", median(tq));
		}

		// 3 & 4 : push sur donnees triees (pire / meilleur cas pour le tas)
		for (int p = 1; p <= 2; p++) {
			if (N > H::kMaxSorted) continue;
			vector<double> t;
			const auto& d = data(N, (Pattern)p);
			for (int r = 0; r < reps; r++) {
				H h(N);
				auto t0 = Clock::now();
				for (int x : d) h.push(x);
				t.push_back(ns_since(t0) / N);
				g_sink += h.pop();
			}
			put(p == 1 ? "3. push, donnees croissantes" : "4. push, donnees decroissantes", median(t));
		}

		// 5 : construction depuis un tableau (heapify)
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

		// 6 : charge mixte "Dijkstra" : pop + push en regime permanent
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
			put("6. mixte pop+push (Dijkstra / file de priorite)", median(t));
		}
	}
}

static void print_tables(const Results& R, const vector<size_t>& sizes) {
	for (auto& [test, algos] : R) {
		printf("\n=== %s   [ns par operation, plus petit = mieux] ===\n", test.c_str());
		printf("%-28s", "");
		for (size_t n : sizes) printf("%12s", ("N=" + to_string(n)).c_str());
		printf("\n");
		// meilleur par colonne
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
				else printf("%10.1f%s", v, (v == best[i]) ? " *" : "  ");
			}
			printf("\n");
		}
	}
	printf("\n(* = meilleur de la colonne ; - = non mesure, trop lent)\n");
}

static void print_ranking(const Results& R, const vector<size_t>& sizes) {
	// score = moyenne geometrique du ratio "temps / meilleur temps" sur toutes les cases
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
	vector<pair<double, string>> rank;
	for (auto& [a, s] : logsum) rank.push_back({exp(s / cnt[a]), a});
	sort(rank.begin(), rank.end());
	printf("\n############ CLASSEMENT GLOBAL ############\n");
	printf("(1.00 = toujours le meilleur ; 2.00 = en moyenne 2x plus lent que le meilleur)\n\n");
	int k = 1;
	for (auto& [score, a] : rank) printf("%d. %-28s score = %.2f\n", k++, a.c_str(), score);
}

int main(int argc, char** argv) {
	int reps = (argc > 1) ? atoi(argv[1]) : 3;
	size_t maxN = (argc > 2) ? (size_t)atoll(argv[2]) : 1000000;
	if (reps < 1) reps = 1;

	vector<size_t> sizes;
	for (size_t n : {1000u, 10000u, 100000u, 1000000u})
		if (n <= maxN) sizes.push_back(n);

	printf("Benchmark min-heap | repetitions=%d | mediane retenue\n", reps);
	printf("Compilateur : %s\n", __VERSION__);
#ifdef __OPTIMIZE__
	printf("Optimisation : activee\n");
#else
	printf("ATTENTION : compile SANS optimisation, ajoute -O2 !\n");
#endif

	printf("\nVerification de la correction... ");
	bool ok = verify<SwapArrayHeap>("swap") && verify<DaryHeap<2>>("d2") && verify<DaryHeap<4>>("d4") &&
	          verify<DaryHeap<8>>("d8") && verify<StdPQ<vector<int>>>("pq") &&
	          verify<StdPQ<deque<int>>>("pq-deque") && verify<StdMultiset>("multiset") && verify<BstHeap>("bst");
	if (!ok) return 1;
	printf("OK\n\nMesures en cours (peut durer 1 a 3 minutes)...\n");

	Results R;
	bench_algo<SwapArrayHeap>("Tableau + swap (ton code)", sizes, reps, R);
	bench_algo<DaryHeap<2>>("Tas binaire (trou)", sizes, reps, R);
	bench_algo<DaryHeap<4>>("Tas 4-aire", sizes, reps, R);
	bench_algo<DaryHeap<8>>("Tas 8-aire", sizes, reps, R);
	bench_algo<StdPQ<vector<int>>>("priority_queue<vector>", sizes, reps, R);
	bench_algo<StdPQ<deque<int>>>("priority_queue<deque>", sizes, reps, R);
	bench_algo<StdMultiset>("std::multiset", sizes, reps, R);
	bench_algo<BstHeap>("BST non equilibre", sizes, reps, R);

	print_tables(R, sizes);
	print_ranking(R, sizes);
	return (int)(g_sink & 0); // utilise g_sink
}

/*
output Terminal:

alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/Arbre de Recherche$ ./HeapComp
Benchmark min-heap | repetitions=3 | mediane retenue
Compilateur : 15.2.0
Optimisation : activee

Verification de la correction... OK

Mesures en cours (peut durer 1 a 3 minutes)...
  > Tableau + swap (ton code)
  > Tas binaire (trou)
  > Tas 4-aire
  > Tas 8-aire
  > priority_queue<vector>
  > priority_queue<deque>
  > std::multiset
  > BST non equilibre

=== 1. push, donnees aleatoires   [ns par operation, plus petit = mieux] ===
                                  N=1000     N=10000    N=100000   N=1000000
Tableau + swap (ton code)         10.7        11.9        12.1        12.9  
Tas binaire (trou)                16.2        12.5        13.2        13.4  
Tas 4-aire                         8.6         7.1         8.2         8.6  
Tas 8-aire                         7.1 *       5.3 *       6.2 *       5.5 *
priority_queue<vector>            15.3        23.6        18.6        18.1  
priority_queue<deque>             38.6        32.6        33.2        34.6  
std::multiset                     84.8       133.3       231.7       850.4  
BST non equilibre                103.4       133.4       234.4       875.0  

=== 2. pop-min (vider le tas)   [ns par operation, plus petit = mieux] ===
                                  N=1000     N=10000    N=100000   N=1000000
Tableau + swap (ton code)        221.8        79.2        94.7       128.6  
Tas binaire (trou)                37.0        41.5        61.2       103.4  
Tas 4-aire                        76.8        84.9       114.6       154.5  
Tas 8-aire                        89.6        98.8       124.2       168.3  
priority_queue<vector>            37.0        86.5        66.2       139.2  
priority_queue<deque>            109.7       127.7       206.3       329.9  
std::multiset                     36.8        28.2 *      32.4 *      82.4 *
BST non equilibre                 30.5 *      30.7        42.3       111.1  

=== 3. push, donnees croissantes   [ns par operation, plus petit = mieux] ===
                                  N=1000     N=10000    N=100000   N=1000000
Tableau + swap (ton code)          1.6 *       1.0 *       0.9 *       1.1 *
Tas binaire (trou)                 4.8         2.4         2.0         1.9  
Tas 4-aire                         2.1         2.6         2.2         1.9  
Tas 8-aire                         2.1         2.6         2.2         3.3  
priority_queue<vector>             2.8         2.8         2.8         8.7  
priority_queue<deque>             10.2        10.3        12.1        10.7  
std::multiset                     72.8        85.3       154.5       267.3  
BST non equilibre               1194.8     29044.9             -           -

=== 4. push, donnees decroissantes   [ns par operation, plus petit = mieux] ===
                                  N=1000     N=10000    N=100000   N=1000000
Tableau + swap (ton code)         10.5         7.2        10.5        13.0  
Tas binaire (trou)                 9.7        10.7        14.2        15.5  
Tas 4-aire                         5.7         7.9         6.7         8.1  
Tas 8-aire                         4.5 *       6.1 *       5.7 *       7.3 *
priority_queue<vector>            14.1        28.9        23.6        46.6  
priority_queue<deque>             26.2        35.6        65.8        68.6  
std::multiset                     86.4        99.5       148.4       268.6  
BST non equilibre               1222.8     29082.7             -           -

=== 5. construction depuis tableau (build)   [ns par operation, plus petit = mieux] ===
                                  N=1000     N=10000    N=100000   N=1000000
Tableau + swap (ton code)         13.7        11.6        11.0        11.9  
Tas binaire (trou)                 5.6         5.9         6.7         6.4  
Tas 4-aire                         4.5         5.6         6.8         6.6  
Tas 8-aire                         3.0 *       4.8 *       4.5 *       4.5 *
priority_queue<vector>             7.9        11.7         9.4        14.4  
priority_queue<deque>             20.7        23.6        28.6        26.1  
std::multiset                    105.4       120.5       221.8       838.5  
BST non equilibre                 82.2       123.4       239.4       934.0  

=== 6. mixte pop+push (Dijkstra / file de priorite)   [ns par operation, plus petit = mieux] ===
                                  N=1000     N=10000    N=100000   N=1000000
Tableau + swap (ton code)         46.8        51.6 *      62.3 *      74.0  
Tas binaire (trou)                47.3        58.0        63.5        67.6 *
Tas 4-aire                        46.3 *      59.5        74.0        95.3  
Tas 8-aire                        66.6        76.1       100.3       113.3  
priority_queue<vector>            79.2       110.1       141.2       185.7  
priority_queue<deque>            118.2       159.8       219.7       358.0  
std::multiset                    114.2       132.6       164.6       176.1  
BST non equilibre                 83.2       117.9       175.2       240.5  

(* = meilleur de la colonne ; - = non mesure, trop lent)

############ CLASSEMENT GLOBAL ############
(1.00 = toujours le meilleur ; 2.00 = en moyenne 2x plus lent que le meilleur)

1. Tas 8-aire                   score = 1.48
2. Tas 4-aire                   score = 1.55
3. Tas binaire (trou)           score = 1.71
4. Tableau + swap (ton code)    score = 1.78
5. priority_queue<vector>       score = 2.81
6. priority_queue<deque>        score = 5.85
7. std::multiset                score = 15.46
8. BST non equilibre            score = 26.77
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/Arbre de Recherche$ 
*/