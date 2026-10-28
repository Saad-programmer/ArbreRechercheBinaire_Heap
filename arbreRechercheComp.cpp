// =====================================================================
//  Etude comparative : BST (ton code) selon l'ORDRE D'INSERTION
//
//  Meme ensemble de cles {0..N-1}, seul l'ordre d'insertion change :
//    - croissant   : 0,1,2,3,...            -> arbre = liste chainee (pire cas)
//    - decroissant : N-1,N-2,...            -> liste chainee (pire cas)
//    - zigzag      : 0,N-1,1,N-2,...        -> arbre en "V" (degenere aussi)
//    - aleatoire   : permutation melangee   -> cas moyen, hauteur ~ O(log n)
//    - equilibre   : mediane d'abord        -> meilleur cas, arbre parfait
//
//  Compilation (IMPORTANT : -O2 et -pthread) :
//      g++ -O2 -std=c++17 bench_bst_ordre.cpp -o bench_bst_ordre -pthread
//
//  Utilisation :
//      ./bench_bst_ordre                 // 3 repetitions, degeneres jusqu'a N=50000
//      ./bench_bst_ordre 5 100000 1000000
//                       |   |      '-- N max pour aleatoire / equilibre
//                       |   '--------- N max pour les ordres degeneres
//                       '------------- nombre de repetitions (mediane)
//
//  Les fonctions insert / search / findMin / findMax / maxDepth / count /
//  destroyTree sont EXACTEMENT celles de ton code (recursives).
//  Differences volontaires :
//   - les parcours n'affichent rien (cout fausserait la mesure) : ils
//     accumulent dans une variable "sink" ;
//   - remove est la version corrigee (ton remove avait une fuite memoire
//     et un double delete dans le cas a deux enfants) ;
//   - le programme tourne dans un thread a grosse pile, car ton code
//     recursif sur un arbre degenere de profondeur N saturerait la pile.
// =====================================================================
#include <pthread.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <random>
#include <string>
#include <vector>
using namespace std;
using Clock = chrono::steady_clock;

// ---------------------------------------------------------------------
//  TON CODE (inchange)
// ---------------------------------------------------------------------
struct node {
	node* right;
	node* left;
	int data;
};

node* insert(node*& t, int value)
{
	if (t == NULL) {
		t = new node;
		t->data = value;
		t->left = NULL;
		t->right = NULL;
	} else {
		if (value < t->data)
			insert(t->left, value);
		else
			insert(t->right, value);
	}
	return t;
}

node* search(node* t, int value)
{
	if (t == NULL)
		return t;
	else {
		if (value < t->data)
			t = search(t->left, value);
		else if (value > t->data)
			t = search(t->right, value);
		return t;
	}
}

node* findMin(node* t)
{
	if (t == NULL) return t;
	else if (t->left == NULL) return t;
	else return findMin(t->left);
}

node* findMax(node* t)
{
	if (t == NULL) return t;
	else if (t->right == NULL) return t;
	else return findMax(t->right);
}

int maxDepth(node* t)
{
	if (t == NULL) return 0;
	int ld = maxDepth(t->left);
	int rd = maxDepth(t->right);
	return (max(ld, rd) + 1);
}

void destroyTree(node* t)
{
	if (t != NULL) {
		destroyTree(t->left);
		destroyTree(t->right);
	}
	delete t;
}

int count(node* t)
{
	if (t == NULL) return 0;
	return (count(t->right) + count(t->left)) + 1;
}

// ---------------------------------------------------------------------
//  Versions "sans affichage" pour la mesure
// ---------------------------------------------------------------------
static volatile long long g_sink = 0;

void inOrderSink(node* t)
{
	if (t != NULL) {
		inOrderSink(t->left);
		g_sink = g_sink + t->data;
		inOrderSink(t->right);
	}
}

// somme des profondeurs de tous les noeuds (= nb total de comparaisons des insertions)
long long internalPathLength(node* t, int d)
{
	if (t == NULL) return 0;
	return d + internalPathLength(t->left, d + 1) + internalPathLength(t->right, d + 1);
}

// remove corrige
node* removeNode(node* t, int value)
{
	if (t == NULL) return NULL;
	if (value < t->data)
		t->left = removeNode(t->left, value);
	else if (value > t->data)
		t->right = removeNode(t->right, value);
	else {
		if (t->left == NULL) { node* r = t->right; delete t; return r; }
		if (t->right == NULL) { node* l = t->left; delete t; return l; }
		node* m = findMin(t->right);
		t->data = m->data;
		t->right = removeNode(t->right, m->data);
	}
	return t;
}

// ---------------------------------------------------------------------
//  Generation des ordres d'insertion
// ---------------------------------------------------------------------
enum Ord { ASC, DESC, ZIGZAG, RANDOM, BALANCED, NORD };
static const char* ORD_NAME[NORD] = {"croissant", "decroissant", "zigzag", "aleatoire", "equilibre"};
static bool degenerate(int o) { return o <= ZIGZAG; }

static void balancedGen(vector<int>& out, int lo, int hi)
{
	if (lo > hi) return;
	int mid = lo + (hi - lo) / 2;
	out.push_back(mid);
	balancedGen(out, lo, mid - 1);
	balancedGen(out, mid + 1, hi);
}

static vector<int> makeKeys(int o, int N)
{
	vector<int> v;
	v.reserve(N);
	switch (o) {
	case ASC:
		for (int i = 0; i < N; i++) v.push_back(i);
		break;
	case DESC:
		for (int i = N - 1; i >= 0; i--) v.push_back(i);
		break;
	case ZIGZAG: {
		int lo = 0, hi = N - 1;
		while (lo <= hi) {
			v.push_back(lo++);
			if (lo <= hi) v.push_back(hi--);
		}
		break;
	}
	case RANDOM: {
		v.resize(N);
		iota(v.begin(), v.end(), 0);
		mt19937 g(42);
		shuffle(v.begin(), v.end(), g);
		break;
	}
	case BALANCED:
		balancedGen(v, 0, N - 1);
		break;
	}
	return v;
}

// ---------------------------------------------------------------------
//  Mesures
// ---------------------------------------------------------------------
static double ns_since(Clock::time_point t0)
{
	return chrono::duration<double, nano>(Clock::now() - t0).count();
}
static double median(vector<double> v)
{
	sort(v.begin(), v.end());
	return v[v.size() / 2];
}

enum Metric { BUILD, SEARCH, MINMAX, INORDER, MAXDEPTH, COUNT, REMOVE, DESTROY, HEIGHT, AVGDEPTH, NMET };
static const char* MET_TITLE[NMET] = {
    "1. Construction : insert (ns par insertion)",
    "2. search de toutes les cles (ns par recherche)",
    "3. findMin / findMax (ns par appel)",
    "4. Parcours inOrder (ns par noeud)",
    "5. maxDepth (ns par noeud)",
    "6. count (ns par noeud)",
    "7. remove, 1000 cles (ns par suppression)",
    "8. destroyTree (ns par noeud)",
    "9. Hauteur de l'arbre (nb de niveaux)",
    "10. Profondeur moyenne d'un noeud (= comparaisons moyennes par insert)"};

// result[metric][ordre][indice taille]
static vector<vector<vector<double>>> g_res;
static vector<int> g_sizes;

static void runOne(int o, int si, int reps)
{
	int N = g_sizes[si];
	vector<int> keys = makeKeys(o, N);

	// cles a supprimer : K cles distinctes tirees au hasard
	int K = min(1000, N / 2);
	vector<int> perm(N);
	iota(perm.begin(), perm.end(), 0);
	mt19937 g(7);
	shuffle(perm.begin(), perm.end(), g);

	vector<double> m[NMET];
	for (int r = 0; r < reps; r++) {
		node* root = NULL;

		auto t0 = Clock::now();
		for (int x : keys) insert(root, x);
		m[BUILD].push_back(ns_since(t0) / N);

		m[HEIGHT].push_back(maxDepth(root));
		m[AVGDEPTH].push_back((double)internalPathLength(root, 0) / N + 0.0);

		long long s = 0;
		t0 = Clock::now();
		for (int x : keys) s += (search(root, x) != NULL);
		m[SEARCH].push_back(ns_since(t0) / N);
		g_sink = g_sink + s;

		const int MM = 1000;
		t0 = Clock::now();
		for (int i = 0; i < MM; i++) s += findMin(root)->data + findMax(root)->data;
		m[MINMAX].push_back(ns_since(t0) / (2.0 * MM));
		g_sink = g_sink + s;

		t0 = Clock::now();
		inOrderSink(root);
		m[INORDER].push_back(ns_since(t0) / N);

		t0 = Clock::now();
		s += maxDepth(root);
		m[MAXDEPTH].push_back(ns_since(t0) / N);

		t0 = Clock::now();
		s += count(root);
		m[COUNT].push_back(ns_since(t0) / N);
		g_sink = g_sink + s;

		t0 = Clock::now();
		for (int i = 0; i < K; i++) root = removeNode(root, perm[i]);
		m[REMOVE].push_back(ns_since(t0) / K);

		t0 = Clock::now();
		destroyTree(root);
		m[DESTROY].push_back(ns_since(t0) / (N - K));
	}
	for (int k = 0; k < NMET; k++) g_res[k][o][si] = median(m[k]);
}

// ---------------------------------------------------------------------
//  Affichage
// ---------------------------------------------------------------------
struct Theory { string name; vector<double> v; };

static void printTable(int met, const char* fmt, const vector<Theory>& theory = {})
{
	printf("\n=== %s ===\n", MET_TITLE[met]);
	printf("%-26s", "");
	for (int n : g_sizes) printf("%12d", n);
	printf("\n");
	for (int o = 0; o < NORD; o++) {
		printf("%-26s", ORD_NAME[o]);
		for (size_t i = 0; i < g_sizes.size(); i++) {
			double v = g_res[met][o][i];
			if (isnan(v)) printf("%12s", "-");
			else { char b[32]; snprintf(b, sizeof b, fmt, v); printf("%12s", b); }
		}
		printf("\n");
	}
	for (auto& t : theory) {
		printf("%-26s", t.name.c_str());
		for (size_t i = 0; i < g_sizes.size(); i++) {
			char b[32]; snprintf(b, sizeof b, fmt, t.v[i]);
			printf("%12s", b);
		}
		printf("\n");
	}
}

static double harmonic(int n)
{
	double h = 0;
	for (int i = 1; i <= n; i++) h += 1.0 / i;
	return h;
}

// pente log-log du temps TOTAL (ns/op * N) entre premiere et derniere taille valides
static double slope(int met, int o)
{
	int a = -1, b = -1;
	for (size_t i = 0; i < g_sizes.size(); i++)
		if (!isnan(g_res[met][o][i])) { if (a < 0) a = i; b = i; }
	if (a < 0 || a == b) return NAN;
	double Ta = g_res[met][o][a] * g_sizes[a], Tb = g_res[met][o][b] * g_sizes[b];
	return log(Tb / Ta) / log((double)g_sizes[b] / g_sizes[a]);
}

static void report()
{
	for (int k = 0; k < NMET; k++) {
		if (k == HEIGHT) {
			vector<Theory> th(3);
			th[0].name = "[theorie] degenere = N";
			th[1].name = "[theorie] aleatoire ~";
			th[2].name = "[theorie] equilibre";
			for (int n : g_sizes) {
				th[0].v.push_back(n);
				double ln = log((double)n);
				th[1].v.push_back(4.311 * ln - 1.953 * log(ln) + 1);
				th[2].v.push_back(ceil(log2((double)n + 1)));
			}
			printTable(k, "%.0f", th);
		} else if (k == AVGDEPTH) {
			vector<Theory> th(3);
			th[0].name = "[theorie] degenere (N-1)/2";
			th[1].name = "[theorie] aleatoire 2ln n";
			th[2].name = "[theorie] equilibre ~log2 n";
			for (int n : g_sizes) {
				th[0].v.push_back((n - 1) / 2.0);
				th[1].v.push_back((2.0 * (n + 1) * harmonic(n) - 4.0 * n) / n); // formule exacte (esperance)
				th[2].v.push_back(log2((double)n + 1) - 1);
			}
			printTable(k, "%.2f", th);
		} else {
			printTable(k, k == REMOVE || k == BUILD || k == SEARCH ? "%.1f" : "%.2f");
		}
	}

	printf("\n=== Facteur de ralentissement : croissant / aleatoire (x fois plus lent) ===\n");
	printf("%-26s", "");
	for (int n : g_sizes) printf("%12d", n);
	printf("\n");
	const int mets[] = {BUILD, SEARCH, REMOVE};
	const char* labels[] = {"construction", "recherche", "suppression"};
	for (int j = 0; j < 3; j++) {
		printf("%-26s", labels[j]);
		for (size_t i = 0; i < g_sizes.size(); i++) {
			double a = g_res[mets[j]][ASC][i], b = g_res[mets[j]][RANDOM][i];
			if (isnan(a) || isnan(b)) printf("%12s", "-");
			else printf("%11.0fx", a / b);
		}
		printf("\n");
	}

	printf("\n=== Complexite empirique : exposant p tel que T_total ~ N^p ===\n");
	printf("(theorie : degenere -> p = 2 ; aleatoire / equilibre -> p = 1 a 1.1, soit N log N)\n");
	printf("%-26s%16s%16s\n", "", "construction", "recherche(N)");
	for (int o = 0; o < NORD; o++)
		printf("%-26s%16.2f%16.2f\n", ORD_NAME[o], slope(BUILD, o), slope(SEARCH, o));
}

// ---------------------------------------------------------------------
//  main (dans un thread a grosse pile)
// ---------------------------------------------------------------------
struct Args { int reps; int maxDeg; int maxN; };

static void* realMain(void* p)
{
	Args* a = (Args*)p;
	const int all[] = {1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000};
	for (int n : all)
		if (n <= a->maxN) g_sizes.push_back(n);

	g_res.assign(NMET, vector<vector<double>>(NORD, vector<double>(g_sizes.size(), NAN)));

	printf("Etude BST selon l'ordre d'insertion | repetitions=%d (mediane)\n", a->reps);
	printf("Compilateur : %s\n", __VERSION__);
#ifdef __OPTIMIZE__
	printf("Optimisation : activee\n");
#else
	printf("ATTENTION : compile SANS optimisation, ajoute -O2 !\n");
#endif
	printf("sizeof(node) = %zu octets (+ overhead de malloc ~ 8 a 16 octets par noeud)\n", sizeof(node));
	printf("Ordres degeneres limites a N <= %d (temps quadratique)\n\nMesures en cours...\n", a->maxDeg);

	for (size_t si = 0; si < g_sizes.size(); si++) {
		for (int o = 0; o < NORD; o++) {
			if (degenerate(o) && g_sizes[si] > a->maxDeg) continue;
			fprintf(stderr, "  N=%-8d %s\n", g_sizes[si], ORD_NAME[o]);
			runOne(o, si, a->reps);
		}
	}
	report();
	return NULL;
}

int main(int argc, char** argv)
{
	Args a;
	a.reps = (argc > 1) ? atoi(argv[1]) : 3;
	a.maxDeg = (argc > 2) ? atoi(argv[2]) : 50000;
	a.maxN = (argc > 3) ? atoi(argv[3]) : 1000000;
	if (a.reps < 1) a.reps = 1;

	// thread avec grosse pile (on reduit si le systeme refuse)
	size_t sz = (size_t)1 << 30;
	pthread_t th;
	while (sz >= ((size_t)16 << 20)) {
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		if (pthread_attr_setstacksize(&attr, sz) == 0 && pthread_create(&th, &attr, realMain, &a) == 0) {
			pthread_join(th, NULL);
			return 0;
		}
		sz /= 2;
	}
	fprintf(stderr, "Impossible de creer un thread a grosse pile, execution dans le thread principal.\n");
	realMain(&a);
	return 0;
}

/*
  Resultat de mon terminal:

  alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/Arbre de Recherche$ ./arbreRechercheComp
Etude BST selon l'ordre d'insertion | repetitions=3 (mediane)
Compilateur : 15.2.0
Optimisation : activee
sizeof(node) = 24 octets (+ overhead de malloc ~ 8 a 16 octets par noeud)
Ordres degeneres limites a N <= 50000 (temps quadratique)

Mesures en cours...
  N=1000     croissant
  N=1000     decroissant
  N=1000     zigzag
  N=1000     aleatoire
  N=1000     equilibre
  N=2000     croissant
  N=2000     decroissant
  N=2000     zigzag
  N=2000     aleatoire
  N=2000     equilibre
  N=5000     croissant
  N=5000     decroissant
  N=5000     zigzag
  N=5000     aleatoire
  N=5000     equilibre
  N=10000    croissant
  N=10000    decroissant
  N=10000    zigzag
  N=10000    aleatoire
  N=10000    equilibre
  N=20000    croissant
  N=20000    decroissant
  N=20000    zigzag
  N=20000    aleatoire
  N=20000    equilibre
  N=50000    croissant
  N=50000    decroissant
  N=50000    zigzag
  N=50000    aleatoire
  N=50000    equilibre
  N=100000   aleatoire
  N=100000   equilibre
  N=200000   aleatoire
  N=200000   equilibre
  N=500000   aleatoire
  N=500000   equilibre
  N=1000000  aleatoire
  N=1000000  equilibre

=== 1. Construction : insert (ns par insertion) ===
                                  1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant                       7286.5     15303.7     29891.0     57481.3    122401.6    293609.6           -           -           -           -
decroissant                     7057.1     12852.0     29675.3     61068.2    130824.3    329751.3           -           -           -           -
zigzag                          6292.7     12795.3     30084.6     57475.6    111126.0    289926.7           -           -           -           -
aleatoire                        143.7       129.0       137.9       188.1       237.2       333.0       316.8       412.6       799.6      1165.1
equilibre                         78.4        92.6        78.1        96.8        93.7       101.8       109.1       119.5       136.8       163.0

=== 2. search de toutes les cles (ns par recherche) ===
                                  1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant                        966.3      3810.4      8446.3     16943.6     35217.0     74347.3           -           -           -           -
decroissant                      982.0      3034.9      8533.0     17648.9     31109.8     76568.1           -           -           -           -
zigzag                          1156.8      3036.1      9094.9     15082.7     29043.2     69658.7           -           -           -           -
aleatoire                         81.6        95.3        96.1       119.8       176.4       251.8       231.6       311.1       620.4       990.6
equilibre                         55.7        74.1        55.0        76.8        66.4        77.9        71.1        73.2        77.5        84.1

=== 3. findMin / findMax (ns par appel) ===
                                  1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant                       940.50     3253.90     9173.78    16380.03    29996.07    73409.10           -           -           -           -
decroissant                    1094.10     2783.38     8502.80    17258.71    34920.91    72944.77           -           -           -           -
zigzag                            0.94        1.06        0.73        0.74        0.76        0.74           -           -           -           -
aleatoire                         5.27        6.02        1.74        4.37        5.16        5.45        6.87        6.02        5.96       10.49
equilibre                         6.05        6.78        5.10        6.65        6.89        6.94        7.58        8.79        9.42       11.74

=== 4. Parcours inOrder (ns par noeud) ===
                                  1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant                         2.63        3.46        3.68        3.68        3.45        3.82           -           -           -           -
decroissant                       5.08        5.73        6.03        5.79        6.91        7.64           -           -           -           -
zigzag                            4.47        4.78        5.02        4.62        4.58        5.10           -           -           -           -
aleatoire                         8.30       12.33       10.86       10.31       13.65       19.60       15.30       19.08       34.41       44.32
equilibre                         3.59        4.95        2.50        2.93        2.71        3.22        2.87        4.57        5.11        6.19

=== 5. maxDepth (ns par noeud) ===
                                  1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant                         4.33        6.49        6.47        6.25        5.78        6.05           -           -           -           -
decroissant                       4.91        9.88        6.57        6.63        6.87        6.47           -           -           -           -
zigzag                            5.30        5.51        5.51        5.22        5.15        5.99           -           -           -           -
aleatoire                         5.85       11.13       10.43       10.76       13.85       17.03       16.81       20.50       40.53       46.15
equilibre                         2.22        2.35        2.05        1.38        1.40        2.28        1.87        2.94        3.33        6.22

=== 6. count (ns par noeud) ===
                                  1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant                         4.04        5.49        5.12        4.93        4.72        4.86           -           -           -           -
decroissant                       3.00        3.68        3.98        3.74        3.96        3.96           -           -           -           -
zigzag                            4.46        5.15        5.01        4.78        4.55        3.92           -           -           -           -
aleatoire                         8.52       13.80       11.22       10.90       13.80       14.14       16.79       21.02       35.82       41.80
equilibre                         2.95        3.08        2.17        2.80        2.06        2.63        2.44        3.45        6.98        8.50

=== 7. remove, 1000 cles (ns par suppression) ===
                                  1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant                       4502.6     10661.2     29240.5     56016.2    113614.9    299373.8           -           -           -           -
decroissant                     4708.6      9503.9     27912.1     62344.6    134595.7    362912.5           -           -           -           -
zigzag                          5393.6      9882.6     30440.0     56236.3    117446.1    288705.0           -           -           -           -
aleatoire                        115.3       174.6       191.0       212.9       284.6       309.5       416.3       483.8       981.0      1509.5
equilibre                        117.5       156.2       125.9       154.8       166.3       234.2       236.7       365.8       628.5      1108.3

=== 8. destroyTree (ns par noeud) ===
                                  1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant                        31.01       23.13       22.24       20.12       21.40       19.57           -           -           -           -
decroissant                      20.62       19.03       19.97       19.84       20.66       20.60           -           -           -           -
zigzag                           26.55       23.72       22.67       20.24       20.15       19.42           -           -           -           -
aleatoire                        22.63       28.60       25.06       23.13       27.52       27.23       31.83       39.71       67.82       86.70
equilibre                        22.00       31.62       16.91       17.11       18.70       19.81       16.55       20.21       20.44       26.12

=== 9. Hauteur de l'arbre (nb de niveaux) ===
                                  1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant                         1000        2000        5000       10000       20000       50000           -           -           -           -
decroissant                       1000        2000        5000       10000       20000       50000           -           -           -           -
zigzag                            1000        2000        5000       10000       20000       50000           -           -           -           -
aleatoire                           22          23          29          29          33          38          41          41          45          50
equilibre                           10          11          13          14          15          16          17          18          19          20
[theorie] degenere = N            1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
[theorie] aleatoire ~               27          30          34          36          39          43          46          49          53          55
[theorie] equilibre                 10          11          13          14          15          16          17          18          19          20

=== 10. Profondeur moyenne d'un noeud (= comparaisons moyennes par insert) ===
                                  1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant                       499.50      999.50     2499.50     4999.50     9999.50    24999.50           -           -           -           -
decroissant                     499.50      999.50     2499.50     4999.50     9999.50    24999.50           -           -           -           -
zigzag                          499.50      999.50     2499.50     4999.50     9999.50    24999.50           -           -           -           -
aleatoire                        11.10       11.26       13.98       15.99       16.37       19.31       19.22       20.68       23.18       23.98
equilibre                         7.99        8.98       10.36       11.36       12.36       13.69       14.69       15.69       16.95       17.95
[theorie] degenere (N-1)/2      499.50      999.50     2499.50     4999.50     9999.50    24999.50    49999.50    99999.50   249999.50   499999.50
[theorie] aleatoire 2ln n        10.99       12.36       14.19       15.58       16.96       18.79       20.18       21.57       23.40       24.79
[theorie] equilibre ~log2 n        8.97        9.97       11.29       12.29       13.29       14.61       15.61       16.61       17.93       18.93

=== Facteur de ralentissement : croissant / aleatoire (x fois plus lent) ===
                                  1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
construction                       51x        119x        217x        306x        516x        882x           -           -           -           -
recherche                          12x         40x         88x        141x        200x        295x           -           -           -           -
suppression                        39x         61x        153x        263x        399x        967x           -           -           -           -

=== Complexite empirique : exposant p tel que T_total ~ N^p ===
(theorie : degenere -> p = 2 ; aleatoire / equilibre -> p = 1 a 1.1, soit N log N)
                              construction    recherche(N)
croissant                             1.94            2.11
decroissant                           1.98            2.11
zigzag                                1.98            2.05
aleatoire                             1.30            1.36
equilibre                             1.11            1.06
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/Arbre de Recherche$ 
*/