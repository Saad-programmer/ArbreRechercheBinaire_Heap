// =====================================================================
//  Etude comparative : TON BST (a pointeurs, non equilibre)
//                 vs  std::map<int,int> (arbre rouge-noir equilibre)
//
//  Compilation (IMPORTANT : -O2 et -pthread) :
//      g++ -O2 -std=c++17 bench_bst_vs_map.cpp -o bench_bst_vs_map -pthread
//
//  Utilisation :
//      ./bench_bst_vs_map                    // 3 repetitions
//      ./bench_bst_vs_map 5 50000 1000000
//                         |  |     '-- N max (tous les cas)
//                         |  '-------- N max pour TON BST sur donnees triees
//                         '----------- nombre de repetitions (mediane)
//
//  Trois ordres d'insertion sont testes : croissant, decroissant, aleatoire.
//  Les cles inserees sont paires (2*i) : les cles impaires servent aux
//  recherches qui echouent.
//
//  Notes d'equite :
//   - insert/search/findMin/findMax/maxDepth/destroyTree = TON code (recursif) ;
//   - remove = version corrigee (l'original avait une fuite memoire et un
//     double delete) ;
//   - les parcours n'affichent rien (cout fausserait la mesure) ;
//   - ton insert accepte les doublons, std::map non : dans la charge mixte,
//     le BST fait search() avant insert() pour avoir la meme semantique ;
//   - tourne dans un thread a grosse pile (recursion de profondeur N sur
//     arbre degenere).
// =====================================================================
#include <malloc.h>
#include <pthread.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>
using namespace std;
using Clock = chrono::steady_clock;

// ---------------------------------------------------------------------
//  TON CODE
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

static volatile long long g_sink = 0;

void inOrderSink(node* t)
{
	if (t != NULL) {
		inOrderSink(t->left);
		g_sink = g_sink + t->data;
		inOrderSink(t->right);
	}
}

void inOrderCollect(node* t, vector<int>& out)
{
	if (t != NULL) {
		inOrderCollect(t->left, out);
		out.push_back(t->data);
		inOrderCollect(t->right, out);
	}
}

// ---------------------------------------------------------------------
//  Donnees
// ---------------------------------------------------------------------
enum Ord { ASC, DESC, RANDOM, NORD };
static const char* ORD_NAME[NORD] = {"croissant", "decroissant", "aleatoire"};
static bool degenerate(int o) { return o != RANDOM; }

static vector<int> makeKeys(int o, int N) // cles paires 0,2,4,...
{
	vector<int> v(N);
	for (int i = 0; i < N; i++) v[i] = 2 * i;
	if (o == DESC) reverse(v.begin(), v.end());
	if (o == RANDOM) {
		mt19937 g(42);
		shuffle(v.begin(), v.end(), g);
	}
	return v;
}

struct Op { int type; int key; };

// ---------------------------------------------------------------------
//  Mesure
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

static double heapInUse()
{
#if defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 33))
	return (double)mallinfo2().uordblks;
#else
	return NAN;
#endif
}

enum Impl { I_BST, I_MAP, I_MAPHINT, NIMPL };
static const char* IMPL_NAME[NIMPL] = {"BST (ton code)", "std::map", "std::map + hint"};

enum Metric { BUILD, HIT, MISS, MINMAX, INORDER, ERASE, MIXED, DESTROY, MEM, NMET };
static const char* MET_TITLE[NMET] = {
    "1. Construction : insertion (ns par insertion)",
    "2. Recherche REUSSIE, cles dans un ordre aleatoire (ns par recherche)",
    "3. Recherche ECHOUEE (ns par recherche)",
    "4. Minimum + maximum (ns par appel)",
    "5. Parcours en ordre croissant (ns par noeud)",
    "6. Suppression de 1000 cles (ns par suppression)",
    "7. Charge mixte : 50% recherche, 25% insertion, 25% suppression (ns par operation)",
    "8. Destruction complete de l'arbre (ns par noeud)",
    "9. Memoire reellement utilisee (octets par noeud)"};

static vector<int> g_sizes;
// g_res[metrique][ordre][implementation][indice taille]
static vector<vector<array<vector<double>, NIMPL>>> g_res;

static void runOne(int o, int si, int reps, int maxDegBst)
{
	const int N = g_sizes[si];
	const bool doBst = !(degenerate(o) && N > maxDegBst);
	const vector<int> keys = makeKeys(o, N);

	// requetes de recherche : memes cles, ordre aleatoire
	vector<int> qhit = keys;
	{ mt19937 g(7); shuffle(qhit.begin(), qhit.end(), g); }
	vector<int> qmiss(N);
	for (int i = 0; i < N; i++) qmiss[i] = qhit[i] + 1; // impair -> absent
	const int K = min(1000, N / 2);

	// charge mixte
	size_t OPS = 1000000;
	if (degenerate(o)) OPS = (size_t)max(2000, min(1000000, 200000000 / N));
	vector<Op> ops(OPS);
	{
		mt19937 g(99);
		for (auto& op : ops) { op.type = g() % 4; op.key = g() % (2 * N); }
	}

	vector<double> m[NIMPL][NMET];

	for (int r = 0; r < reps; r++) {
		// ============ BST ============
		if (doBst) {
			node* root = NULL;
			double h0 = heapInUse();
			auto t0 = Clock::now();
			for (int x : keys) insert(root, x);
			m[I_BST][BUILD].push_back(ns_since(t0) / N);
			m[I_BST][MEM].push_back((heapInUse() - h0) / N);

			long long s = 0;
			t0 = Clock::now();
			for (int x : qhit) s += (search(root, x) != NULL);
			m[I_BST][HIT].push_back(ns_since(t0) / N);

			t0 = Clock::now();
			for (int x : qmiss) s += (search(root, x) != NULL);
			m[I_BST][MISS].push_back(ns_since(t0) / N);

			const int MM = 1000;
			t0 = Clock::now();
			for (int i = 0; i < MM; i++) s += findMin(root)->data + findMax(root)->data;
			m[I_BST][MINMAX].push_back(ns_since(t0) / (2.0 * MM));

			t0 = Clock::now();
			inOrderSink(root);
			m[I_BST][INORDER].push_back(ns_since(t0) / N);

			t0 = Clock::now();
			for (int i = 0; i < K; i++) root = removeNode(root, qhit[i]);
			m[I_BST][ERASE].push_back(ns_since(t0) / K);

			t0 = Clock::now();
			for (auto& op : ops) {
				if (op.type < 2) s += (search(root, op.key) != NULL);
				else if (op.type == 2) { if (!search(root, op.key)) insert(root, op.key); }
				else root = removeNode(root, op.key);
			}
			m[I_BST][MIXED].push_back(ns_since(t0) / OPS);

			int n = count(root);
			t0 = Clock::now();
			destroyTree(root);
			m[I_BST][DESTROY].push_back(ns_since(t0) / n);
			g_sink = g_sink + s;
		}

		// ============ std::map ============
		{
			map<int, int>* mp = new map<int, int>();
			map<int, int>& mm = *mp;
			double h0 = heapInUse();
			auto t0 = Clock::now();
			for (int x : keys) mm.insert(make_pair(x, x));
			m[I_MAP][BUILD].push_back(ns_since(t0) / N);
			m[I_MAP][MEM].push_back((heapInUse() - h0) / N);

			long long s = 0;
			t0 = Clock::now();
			for (int x : qhit) s += (mm.find(x) != mm.end());
			m[I_MAP][HIT].push_back(ns_since(t0) / N);

			t0 = Clock::now();
			for (int x : qmiss) s += (mm.find(x) != mm.end());
			m[I_MAP][MISS].push_back(ns_since(t0) / N);

			const int MM = 1000;
			t0 = Clock::now();
			for (int i = 0; i < MM; i++) s += mm.begin()->first + mm.rbegin()->first;
			m[I_MAP][MINMAX].push_back(ns_since(t0) / (2.0 * MM));

			t0 = Clock::now();
			{
				long long a = 0;
				for (auto& kv : mm) a += kv.first;
				g_sink = g_sink + a;
			}
			m[I_MAP][INORDER].push_back(ns_since(t0) / N);

			t0 = Clock::now();
			for (int i = 0; i < K; i++) mm.erase(qhit[i]);
			m[I_MAP][ERASE].push_back(ns_since(t0) / K);

			t0 = Clock::now();
			for (auto& op : ops) {
				if (op.type < 2) s += (mm.find(op.key) != mm.end());
				else if (op.type == 2) mm.insert(make_pair(op.key, op.key));
				else mm.erase(op.key);
			}
			m[I_MAP][MIXED].push_back(ns_since(t0) / OPS);

			size_t n = mm.size();
			t0 = Clock::now();
			mm.clear();
			m[I_MAP][DESTROY].push_back(ns_since(t0) / (double)n);
			g_sink = g_sink + s;
			delete mp;
		}

		// ============ std::map + hint (ordre connu) ============
		if (o != RANDOM) {
			map<int, int> mm;
			auto t0 = Clock::now();
			if (o == ASC) for (int x : keys) mm.insert(mm.end(), make_pair(x, x));
			else for (int x : keys) mm.insert(mm.begin(), make_pair(x, x));
			m[I_MAPHINT][BUILD].push_back(ns_since(t0) / N);
			g_sink = g_sink + (long long)mm.size();
		}
	}

	for (int k = 0; k < NMET; k++)
		for (int i = 0; i < NIMPL; i++)
			if (!m[i][k].empty()) g_res[k][o][i][si] = median(m[i][k]);
}

// ---------------------------------------------------------------------
//  Verification de correction
// ---------------------------------------------------------------------
static bool verify()
{
	const int N = 5000;
	for (int o = 0; o < NORD; o++) {
		vector<int> keys = makeKeys(o, N);
		node* root = NULL;
		map<int, int> mm;
		for (int x : keys) { insert(root, x); mm.insert(make_pair(x, x)); }
		for (int i = 0; i < 1000; i++) { root = removeNode(root, keys[i]); mm.erase(keys[i]); }
		vector<int> a;
		inOrderCollect(root, a);
		vector<int> b;
		for (auto& kv : mm) b.push_back(kv.first);
		destroyTree(root);
		if (a != b) return false;
	}
	return true;
}

// ---------------------------------------------------------------------
//  Affichage
// ---------------------------------------------------------------------
static void header()
{
	printf("%-30s", "");
	for (int n : g_sizes) printf("%12d", n);
	printf("\n");
}

static void report()
{
	for (int k = 0; k < NMET; k++) {
		const char* fmt = (k == MEM) ? "%.1f" : "%.1f";
		printf("\n=== %s ===\n", MET_TITLE[k]);
		header();
		for (int o = 0; o < NORD; o++) {
			for (int i = 0; i < NIMPL; i++) {
				bool any = false;
				for (double v : g_res[k][o][i]) if (!isnan(v)) any = true;
				if (!any) continue;
				string label = string(ORD_NAME[o]) + " | " + IMPL_NAME[i];
				printf("%-30s", label.c_str());
				for (size_t s = 0; s < g_sizes.size(); s++) {
					double v = g_res[k][o][i][s];
					if (isnan(v)) printf("%12s", "-");
					else { char b[32]; snprintf(b, sizeof b, fmt, v); printf("%12s", b); }
				}
				printf("\n");
			}
		}
	}

	printf("\n\n################  RAPPORT  BST / std::map  ################\n");
	printf("(> 1.00 : std::map est plus rapide / economique ; < 1.00 : ton BST l'est)\n");
	for (int k = 0; k < NMET; k++) {
		printf("\n--- %s ---\n", MET_TITLE[k]);
		header();
		for (int o = 0; o < NORD; o++) {
			printf("%-30s", ORD_NAME[o]);
			for (size_t s = 0; s < g_sizes.size(); s++) {
				double a = g_res[k][o][I_BST][s], b = g_res[k][o][I_MAP][s];
				if (isnan(a) || isnan(b) || b == 0) printf("%12s", "-");
				else printf("%11.2fx", a / b);
			}
			printf("\n");
		}
	}

	printf("\n\n################  SYNTHESE  ################\n");
	printf("Moyenne geometrique du rapport BST/map sur toutes les tailles :\n\n");
	printf("%-34s", "");
	for (int o = 0; o < NORD; o++) printf("%14s", ORD_NAME[o]);
	printf("\n");
	static const char* shortName[NMET] = {"construction",  "recherche reussie", "recherche echouee",
	                                      "min/max",       "parcours",          "suppression",
	                                      "charge mixte",  "destruction",       "memoire / noeud"};
	vector<double> all;
	for (int k = 0; k < NMET; k++) {
		printf("%-34s", shortName[k]);
		for (int o = 0; o < NORD; o++) {
			double ls = 0; int c = 0;
			for (size_t s = 0; s < g_sizes.size(); s++) {
				double a = g_res[k][o][I_BST][s], b = g_res[k][o][I_MAP][s];
				if (isnan(a) || isnan(b) || b == 0 || a == 0) continue;
				ls += log(a / b); c++;
			}
			if (c == 0) printf("%14s", "-");
			else {
				double g = exp(ls / c);
				printf("%13.2fx", g);
				// min/max exclu : std::map garde un pointeur vers le min/max (O(1)),
				// ce qui fausserait la moyenne (rapport de 20x a 140x).
				if (o == RANDOM && k != MEM && k != MINMAX) all.push_back(log(g));
			}
		}
		printf("\n");
	}
	if (!all.empty()) {
		double ls = 0;
		for (double x : all) ls += x;
		double g = exp(ls / all.size());
		printf("\nDonnees ALEATOIRES, toutes operations (hors min/max et memoire) : BST / map = %.2fx\n", g);
		if (g > 1.0) printf("=> std::map est en moyenne %.2fx plus rapide que ton BST.\n", g);
		else printf("=> ton BST est en moyenne %.2fx plus rapide que std::map.\n", 1.0 / g);
	}
	printf("\nSur donnees TRIEES, ton BST degenere (O(N) par operation) : voir les rapports ci-dessus,\n");
	printf("ils explosent avec N alors que std::map reste en O(log N).\n");
}

// ---------------------------------------------------------------------
//  main (thread a grosse pile)
// ---------------------------------------------------------------------
struct Args { int reps; int maxDeg; int maxN; };

static void* realMain(void* p)
{
	Args* a = (Args*)p;
	const int all[] = {1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000};
	for (int n : all)
		if (n <= a->maxN) g_sizes.push_back(n);
	g_res.assign(NMET, vector<array<vector<double>, NIMPL>>(NORD));
	for (auto& row : g_res)
		for (auto& impls : row)
			for (auto& v : impls) v.assign(g_sizes.size(), NAN);

	printf("BST (ton code) vs std::map | repetitions=%d (mediane)\n", a->reps);
	printf("Compilateur : %s\n", __VERSION__);
#ifdef __OPTIMIZE__
	printf("Optimisation : activee\n");
#else
	printf("ATTENTION : compile SANS optimisation, ajoute -O2 !\n");
#endif
	printf("sizeof(node) = %zu octets | sizeof(pair<int,int>) = %zu octets\n", sizeof(node), sizeof(pair<int, int>));
	printf("Ton BST sur donnees triees limite a N <= %d (temps quadratique)\n", a->maxDeg);

	printf("\nVerification de la correction (BST et map donnent le meme contenu)... ");
	if (!verify()) { printf("ERREUR !\n"); return NULL; }
	printf("OK\n\nMesures en cours...\n");

	for (size_t si = 0; si < g_sizes.size(); si++)
		for (int o = 0; o < NORD; o++) {
			fprintf(stderr, "  N=%-8d %s\n", g_sizes[si], ORD_NAME[o]);
			runOne(o, si, a->reps, a->maxDeg);
		}
	report();
	return NULL;
}

int main(int argc, char** argv)
{
	Args a;
	a.reps = (argc > 1) ? atoi(argv[1]) : 3;
	a.maxDeg = (argc > 2) ? atoi(argv[2]) : 20000;
	a.maxN = (argc > 3) ? atoi(argv[3]) : 1000000;
	if (a.reps < 1) a.reps = 1;

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
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/Arbre de Recherche$ ./BSTvsMAP
BST (ton code) vs std::map | repetitions=3 (mediane)
Compilateur : 15.2.0
Optimisation : activee
sizeof(node) = 24 octets | sizeof(pair<int,int>) = 8 octets
Ton BST sur donnees triees limite a N <= 20000 (temps quadratique)

Verification de la correction (BST et map donnent le meme contenu)... OK

Mesures en cours...
  N=1000     croissant
  N=1000     decroissant
  N=1000     aleatoire
  N=2000     croissant
  N=2000     decroissant
  N=2000     aleatoire
  N=5000     croissant
  N=5000     decroissant
  N=5000     aleatoire
  N=10000    croissant
  N=10000    decroissant
  N=10000    aleatoire
  N=20000    croissant
  N=20000    decroissant
  N=20000    aleatoire
  N=50000    croissant
  N=50000    decroissant
  N=50000    aleatoire
  N=100000   croissant
  N=100000   decroissant
  N=100000   aleatoire
  N=200000   croissant
  N=200000   decroissant
  N=200000   aleatoire
  N=500000   croissant
  N=500000   decroissant
  N=500000   aleatoire
  N=1000000  croissant
  N=1000000  decroissant
  N=1000000  aleatoire

=== 1. Construction : insertion (ns par insertion) ===
                                      1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant | BST (ton code)          6109.0     13140.2     33844.0     60295.8    119477.0           -           -           -           -           -
croissant | std::map                  56.6        56.9        67.3        79.8        81.8       111.5        96.2        91.2       117.5       130.7
croissant | std::map + hint           37.1        38.3        38.9        37.8        36.2        42.9        37.4        36.9        37.9        37.1
decroissant | BST (ton code)        5885.2     13964.9     33425.8     61487.7    117307.4           -           -           -           -           -
decroissant | std::map                68.3        61.8        74.1        72.4        78.4        78.1        82.1        94.0        99.3       129.1
decroissant | std::map + hint         46.4        37.7        40.0        38.9        36.8        39.1        36.6        37.7        38.4        44.2
aleatoire | BST (ton code)           135.5       127.8       140.5       183.2       210.7       341.0       381.2       374.6       560.4       773.2
aleatoire | std::map                 131.3       151.8       191.1       212.8       272.4       228.2       259.4       344.1       538.3       741.5

=== 2. Recherche REUSSIE, cles dans un ordre aleatoire (ns par recherche) ===
                                      1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant | BST (ton code)          1346.1      3347.8     13456.6     19717.2     36491.1           -           -           -           -           -
croissant | std::map                  64.6        76.6       114.9       106.7       128.0       188.4       263.2       299.7       482.4       678.5
decroissant | BST (ton code)         926.7      3972.3     13504.3     22298.4     37121.2           -           -           -           -           -
decroissant | std::map                73.9        89.2        93.9       109.8       126.1       214.4       180.5       281.7       485.6       755.7
aleatoire | BST (ton code)           114.7        97.0       112.1       115.6       151.8       180.5       230.0       250.5       444.9       611.8
aleatoire | std::map                  69.7       123.2        98.3       166.8       157.7       168.0       202.0       311.0       559.6       760.3

=== 3. Recherche ECHOUEE (ns par recherche) ===
                                      1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant | BST (ton code)          1012.7      3189.1     13306.7     19654.4     35405.9           -           -           -           -           -
croissant | std::map                  64.0        74.8        92.8       112.6       125.2       202.7       238.7       293.8       493.4       673.1
decroissant | BST (ton code)         980.4      3333.3     13372.5     22138.2     36901.8           -           -           -           -           -
decroissant | std::map                73.2        76.3        88.5       107.9       123.5       181.4       183.9       276.2       478.7       759.9
aleatoire | BST (ton code)           113.3       100.7       108.9       135.3       169.3       242.3       228.9       278.8       516.0       731.8
aleatoire | std::map                  68.1       116.7        95.4       135.2       135.3       167.2       207.6       325.0       549.2       765.0

=== 4. Minimum + maximum (ns par appel) ===
                                      1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant | BST (ton code)          1059.1      4203.0     11238.5     19425.1     34584.8           -           -           -           -           -
croissant | std::map                   0.2         0.1         0.1         0.1         0.1         0.1         0.1         0.1         0.2         0.2
decroissant | BST (ton code)         967.6      3119.8     10665.3     19082.1     34300.3           -           -           -           -           -
decroissant | std::map                 0.0         0.0         0.0         0.0         0.0         0.1         0.0         0.1         0.2         0.2
aleatoire | BST (ton code)             5.8         6.0         3.3         5.0         4.8         4.7         7.3         6.0         5.8        10.2
aleatoire | std::map                   0.0         0.1         0.0         0.0         0.1         0.1         0.1         0.2         0.1         0.1

=== 5. Parcours en ordre croissant (ns par noeud) ===
                                      1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant | BST (ton code)             3.5         4.3         5.6         4.5         4.0           -           -           -           -           -
croissant | std::map                   6.4         7.3         8.0         7.7         6.7        11.3         8.2        10.1        14.5        12.4
decroissant | BST (ton code)           4.4         6.0         6.6         6.2         7.1           -           -           -           -           -
decroissant | std::map                 6.8         7.6         8.2         7.7         7.0         8.8         8.3        13.6        17.6        24.2
aleatoire | BST (ton code)            12.6        11.1        14.4        24.2        11.6        13.3        15.7        18.6        33.6        44.2
aleatoire | std::map                  12.8        22.9        14.9        15.9        14.6        22.6        26.6        48.0        90.9       107.5

=== 6. Suppression de 1000 cles (ns par suppression) ===
                                      1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant | BST (ton code)          4869.2      9728.1     30093.1     61298.7    113381.3           -           -           -           -           -
croissant | std::map                 127.6       135.2       164.2       184.8       203.7       484.2       314.1       449.2       941.9      1152.7
decroissant | BST (ton code)        5212.8     11196.7     31403.5     65754.3    118380.1           -           -           -           -           -
decroissant | std::map               146.1       139.9       163.6       182.3       208.4       283.7       308.4       620.1       754.9      1398.4
aleatoire | BST (ton code)           147.7       133.2       198.8       289.0       250.3       282.7       337.1       413.7       816.5      1181.2
aleatoire | std::map                 143.4       214.3       196.7       193.5       191.0       252.8       288.1       453.7       850.0      1067.1

=== 7. Charge mixte : 50% recherche, 25% insertion, 25% suppression (ns par operation) ===
                                      1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant | BST (ton code)           421.9      1287.3     12266.6     32763.4     69329.1           -           -           -           -           -
croissant | std::map                  98.6       117.6       116.4       133.3       142.4       231.3       221.0       327.1       556.1       723.4
decroissant | BST (ton code)         497.5      1766.8     13237.9     34091.7     69778.1           -           -           -           -           -
decroissant | std::map                89.9        98.7       113.2       130.5       142.9       205.4       213.6       349.6       512.7       843.4
aleatoire | BST (ton code)           103.0       126.8       162.2       183.4       203.0       253.4       262.9       332.0       568.9       761.5
aleatoire | std::map                  94.5       126.7       134.1       180.2       200.2       200.3       342.3       350.3       600.7       800.0

=== 8. Destruction complete de l'arbre (ns par noeud) ===
                                      1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant | BST (ton code)            22.6        20.7        24.8        22.9        18.1           -           -           -           -           -
croissant | std::map                  17.7        18.3        17.8        17.2        15.4        16.7        15.5        18.9        23.0        25.0
decroissant | BST (ton code)          23.3        21.0        25.4        20.5        19.3           -           -           -           -           -
decroissant | std::map                21.8        18.0        19.0        17.4        15.4        16.1        14.9        17.4        18.6        22.2
aleatoire | BST (ton code)            23.5        31.3        24.3        28.1        22.0        24.8        27.6        33.5        66.4        86.9
aleatoire | std::map                  20.6        23.0        18.0        21.4        22.1        24.0        35.2        43.2        69.6        82.2

=== 9. Memoire reellement utilisee (octets par noeud) ===
                                      1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant | BST (ton code)            32.0        32.0        32.0        32.0        32.0           -           -           -           -           -
croissant | std::map                  48.0        48.0        47.9        48.0        48.0        48.0        48.0        48.0        48.0        48.0
decroissant | BST (ton code)          32.0        32.0        32.0        32.0        32.0           -           -           -           -           -
decroissant | std::map                48.0        48.0        47.9        48.0        48.0        48.0        48.0        48.0        48.0        48.0
aleatoire | BST (ton code)            32.0        32.0        32.0        32.0        32.0        32.0        32.0        32.0        32.0        32.0
aleatoire | std::map                  48.0        48.0        48.0        48.0        48.0        48.0        48.0        48.0        48.0        48.0


################  RAPPORT  BST / std::map  ################
(> 1.00 : std::map est plus rapide / economique ; < 1.00 : ton BST l'est)

--- 1. Construction : insertion (ns par insertion) ---
                                      1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant                          108.00x     231.02x     502.64x     755.36x    1459.71x           -           -           -           -           -
decroissant                         86.12x     225.92x     451.18x     848.85x    1495.82x           -           -           -           -           -
aleatoire                            1.03x       0.84x       0.74x       0.86x       0.77x       1.49x       1.47x       1.09x       1.04x       1.04x

--- 2. Recherche REUSSIE, cles dans un ordre aleatoire (ns par recherche) ---
                                      1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant                           20.83x      43.68x     117.09x     184.74x     285.11x           -           -           -           -           -
decroissant                         12.54x      44.55x     143.86x     203.04x     294.43x           -           -           -           -           -
aleatoire                            1.64x       0.79x       1.14x       0.69x       0.96x       1.07x       1.14x       0.81x       0.80x       0.80x

--- 3. Recherche ECHOUEE (ns par recherche) ---
                                      1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant                           15.82x      42.62x     143.42x     174.49x     282.77x           -           -           -           -           -
decroissant                         13.39x      43.68x     151.04x     205.25x     298.86x           -           -           -           -           -
aleatoire                            1.66x       0.86x       1.14x       1.00x       1.25x       1.45x       1.10x       0.86x       0.94x       0.96x

--- 4. Minimum + maximum (ns par appel) ---
                                      1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant                         5530.57x   41205.85x  112385.12x  227193.58x  283481.59x           -           -           -           -           -
decroissant                      28457.57x   86661.50x  304723.17x  515731.46x  762228.74x           -           -           -           -           -
aleatoire                          216.04x     111.50x     101.48x     121.82x      91.41x      42.71x      71.61x      29.11x      42.32x      77.24x

--- 5. Parcours en ordre croissant (ns par noeud) ---
                                      1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant                            0.56x       0.58x       0.70x       0.58x       0.60x           -           -           -           -           -
decroissant                          0.65x       0.78x       0.80x       0.80x       1.02x           -           -           -           -           -
aleatoire                            0.98x       0.48x       0.97x       1.52x       0.80x       0.59x       0.59x       0.39x       0.37x       0.41x

--- 6. Suppression de 1000 cles (ns par suppression) ---
                                      1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant                           38.17x      71.94x     183.24x     331.64x     556.68x           -           -           -           -           -
decroissant                         35.67x      80.05x     191.96x     360.76x     568.12x           -           -           -           -           -
aleatoire                            1.03x       0.62x       1.01x       1.49x       1.31x       1.12x       1.17x       0.91x       0.96x       1.11x

--- 7. Charge mixte : 50% recherche, 25% insertion, 25% suppression (ns par operation) ---
                                      1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant                            4.28x      10.95x     105.42x     245.84x     486.76x           -           -           -           -           -
decroissant                          5.53x      17.91x     116.97x     261.23x     488.14x           -           -           -           -           -
aleatoire                            1.09x       1.00x       1.21x       1.02x       1.01x       1.27x       0.77x       0.95x       0.95x       0.95x

--- 8. Destruction complete de l'arbre (ns par noeud) ---
                                      1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant                            1.28x       1.13x       1.39x       1.33x       1.17x           -           -           -           -           -
decroissant                          1.07x       1.16x       1.33x       1.18x       1.25x           -           -           -           -           -
aleatoire                            1.14x       1.36x       1.35x       1.31x       0.99x       1.03x       0.79x       0.78x       0.95x       1.06x

--- 9. Memoire reellement utilisee (octets par noeud) ---
                                      1000        2000        5000       10000       20000       50000      100000      200000      500000     1000000
croissant                            0.67x       0.67x       0.67x       0.67x       0.67x           -           -           -           -           -
decroissant                          0.67x       0.67x       0.67x       0.67x       0.67x           -           -           -           -           -
aleatoire                            0.67x       0.67x       0.67x       0.67x       0.67x       0.67x       0.67x       0.67x       0.67x       0.67x


################  SYNTHESE  ################
Moyenne geometrique du rapport BST/map sur toutes les tailles :

                                       croissant   decroissant     aleatoire
construction                             424.76x       406.84x         1.01x
recherche reussie                         89.09x        86.36x         0.95x
recherche echouee                         86.24x        88.46x         1.10x
min/max                                69738.37x    196828.37x        77.60x
parcours                                   0.60x         0.80x         0.64x
suppression                              156.17x       162.22x         1.05x
charge mixte                              56.80x        68.23x         1.01x
destruction                                1.26x         1.20x         1.06x
memoire / noeud                            0.67x         0.67x         0.67x

Donnees ALEATOIRES, toutes operations (hors min/max et memoire) : BST / map = 0.96x
=> ton BST est en moyenne 1.04x plus rapide que std::map.

Sur donnees TRIEES, ton BST degenere (O(N) par operation) : voir les rapports ci-dessus,
ils explosent avec N alors que std::map reste en O(log N).
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/Arbre de Recherche$ 
*/