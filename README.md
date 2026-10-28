# 📊 Arbres de Recherche & Structures de Données en C++

<div align="center">

![C++](https://img.shields.io/badge/C%2B%2B-17-blue?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![License](https://img.shields.io/badge/License-MIT-green?style=for-the-badge)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-Actif-brightgreen?style=for-the-badge)

**Une étude comparative complète des arbres binaires de recherche, tas (heaps), et structures de données STL — avec benchmarks de performance réels.**

[🚀 Démarrage Rapide](#-démarrage-rapide) • [📁 Structure](#-structure-du-projet) • [📈 Résultats](#-résultats-des-benchmarks) • [🧠 Concepts](#-concepts-théoriques)

</div>

---

## 🎯 À Propos du Projet

Ce dépôt regroupe une collection d'**implémentations pédagogiques** et de **benchmarks comparatifs** autour de structures de données fondamentales en C++ :

| Catégorie | Implémentations |
|-----------|-----------------|
| 🌳 **Arbres Binaires de Recherche (BST)** | Version manuelle, comparaison avec `std::map` |
| ⛰️ **Tas (Heaps)** | Tas binaire, d-aire (2, 4, 8), `priority_queue`, `multiset` |
| 📦 **Conteneurs STL** | `vector`, `list`, `deque`, `map`, `multimap`, `set` |
| ⚡ **Optimisations** | Méthode du "trou" (hole), Floyd's heapify, indices calculés |

L'objectif est de **démontrer empiriquement** pourquoi les structures STL modernes surpassent les implémentations naïves sur de grands volumes de données, tout en expliquant *théoriquement* pourquoi.

---

## 📁 Structure du Projet

```
📦 Arbre de Recherche/
│
├── 🌳 arbre_recherche_binaire.cpp    # BST complet (insert, search, delete, parcours) + tests perf
├── 🌳 arbreRecerche.cpp              # BST pédagogique (version simple, tous parcours)
├── 🌳 arbreRechercheComp.cpp         # Étude BST selon l'ORDRE d'insertion (dégénéré vs équilibré)
├── 🌳 BSTvsMAP.cpp                   # BST maison vs std::map (rouge-noir)
│
├── ⛰️ Heap.cpp                        # Tas binaire manuel (tableau statique)
├── ⛰️ HeapComp.cpp                    # Benchmark complet : 8 implémentations de min-heap
├── ⛰️ arbreListVect.cpp               # Benchmark conteneurs pour tas (vector/list/map/deque)
│
├── 📄 output.txt                     # Sortie générée par les tests
└── 📖 README.md                      # Ce fichier
```

---

## 🚀 Démarrage Rapide

### Prérequis

- **Compilateur C++** : `g++` ≥ 9 ou `clang++` ≥ 10
- **OS** : Linux, macOS, Windows (WSL/MSYS2)
- **Flags requis** : `-O2 -std=c++17` (les benchmarks sont **invalides sans `-O2`**)

### Compilation

```bash
# Compilation de tous les fichiers
g++ -O2 -std=c++17 arbre_recherche_binaire.cpp -o arbre_bst
g++ -O2 -std=c++17 arbreRecerche.cpp         -o arbre_peda
g++ -O2 -std=c++17 arbreRechercheComp.cpp    -o comp_bst
g++ -O2 -std=c++17 BSTvsMAP.cpp              -o bst_vs_map -pthread
g++ -O2 -std=c++17 Heap.cpp                  -o heap
g++ -O2 -std=c++17 HeapComp.cpp              -o heap_bench
g++ -O2 -std=c++17 arbreListVect.cpp         -o list_vect_bench
```

### Exécution

```bash
./arbre_bst           # Menu interactif BST
./arbre_peda          # BST pédagogique (résultats directs)
./comp_bst            # Étude BST selon ordre d'insertion
./bst_vs_map          # BST vs std::map
./heap                # Tas binaire manuel
./heap_bench          # Benchmark 8 implémentations de tas
./list_vect_bench     # Benchmark conteneurs pour tas
```

---

## 📈 Résultats des Benchmarks

### 🏆 Classement Global — Tas (min-heap)

> Temps par opération (ns), plus petit = meilleur. Mesures sur `N = 1 000 000`.

| Rang | Implémentation | Push (aléatoire) | Pop-min | Build | Mixte | Score |
|:----:|----------------|------------------:|--------:|------:|------:|------:|
| 🥇 | **Tas 8-aire** | **5.5 ns** | 168.3 ns | **4.5 ns** | 113.3 ns | **1.48** |
| 🥈 | Tas 4-aire | 8.6 ns | 154.5 ns | 6.6 ns | 95.3 ns | 1.55 |
| 🥉 | Tas binaire (trou) | 13.4 ns | 103.4 ns | 6.4 ns | **67.6 ns** | 1.71 |
| 4 | Tableau + swap (code manuel) | 12.9 ns | 128.6 ns | 11.9 ns | 74.0 ns | 1.78 |
| 5 | `priority_queue<vector>` | 18.1 ns | 139.2 ns | 14.4 ns | 185.7 ns | 2.81 |
| 6 | `priority_queue<deque>` | 34.6 ns | 329.9 ns | 26.1 ns | 358.0 ns | 5.85 |
| 7 | `std::multiset` | 850.4 ns | **82.4 ns** | 838.5 ns | 176.1 ns | 15.46 |
| 8 | BST non équilibré | 875.0 ns | 111.1 ns | 934.0 ns | 240.5 ns | 26.77 |

**💡 Conclusion** : Le **tas 8-aire** est le plus rapide en insertion (5.5 ns), tandis que la **méthode du trou** offre le meilleur compromis en charge mixte (Dijkstra). `multiset` et BST sont **catastrophiques** pour la construction mais bons en `pop` quand l'arbre reste équilibré.

---

### 🌳 BST vs `std::map` — Données Aléatoires

| Opération | BST maison | `std::map` | Ratio |
|-----------|-----------:|-----------:|------:|
| Construction | 773.2 ns | 741.5 ns | ~1.0× |
| Recherche réussie | 611.8 ns | 760.3 ns | **0.80×** ✅ |
| Recherche échouée | 731.8 ns | 765.0 ns | 0.96× |
| Suppression | 1181.2 ns | 1067.1 ns | 1.11× |
| Mémoire / nœud | **32 o** | 48 o | **0.67×** ✅ |

**💡 Conclusion** : Sur données **aléatoires**, le BST manuel est **compétitif** (voire plus rapide en recherche) et utilise **33 % de mémoire en moins**. Mais sur données **triées**, il dégénère (O(N²)) et devient **400× à 700× plus lent** que `std::map`.

---

### 📉 Impact de l'Ordre d'Insertion sur le BST

| Ordre | Hauteur (N=1 000) | Construction | Recherche | Complexité empirique |
|-------|-------------------:|-------------:|----------:|---------------------:|
| Croissant | **1 000** | 7 286 ns | 966 ns | **O(N²)** — p = 1.94 |
| Décroissant | **1 000** | 7 057 ns | 982 ns | **O(N²)** — p = 1.98 |
| Zigzag | **1 000** | 6 292 ns | 1 157 ns | **O(N²)** — p = 1.98 |
| Aléatoire | 22 | **144 ns** | **82 ns** | O(N log N) — p = 1.30 |
| Équilibré | **10** | **78 ns** | **56 ns** | O(N log N) — p = 1.11 |

**💡 Conclusion** : L'ordre d'insertion est **critique**. Un BST construit sur des données triées a la même performance qu'une **liste chaînée** (recherche O(N)). L'équilibrage (rouge-noir, AVL) est **indispensable** pour la production.

---

## 🧠 Concepts Théoriques

### Arbre Binaire de Recherche (BST)

```cpp
struct Noeud {
    int valeur;
    Noeud* gauche;   // valeurs < valeur
    Noeud* droite;   // valeurs > valeur
};
```

| Opération | Cas moyen | Pire cas (dégénéré) |
|-----------|:---------:|:-------------------:|
| `insert` | O(log n) | O(n) |
| `search` | O(log n) | O(n) |
| `delete` | O(log n) | O(n) |
| Parcours | O(n) | O(n) |
| Hauteur | O(log n) | **O(n)** |

### Tas Binaire (Min-Heap)

```cpp
// Propriété : parent ≤ enfants
// Index : parent(i) = (i-1)/2, gauche(i) = 2i+1, droite(i) = 2i+2
void siftUp(int i);    // remonter un élément
void siftDown(int i);  // descendre un élément
```

| Opération | Complexité |
|-----------|:----------:|
| `insert` | O(log n) |
| `getMin` | **O(1)** |
| `extractMin` | O(log n) |
| `build` (Floyd) | **O(n)** |
| Recherche | O(n) |

### Méthode du "Trou" (Hole)

Au lieu d'échanger à chaque niveau (2 écritures/niveau), on **remonte le parent dans le trou** puis on écrit la valeur une seule fois à sa position finale → **1 écriture/niveau**, soit **2× moins d'accès mémoire**.

### Floyd's Heapify

Construire un tas en `O(n)` au lieu de `O(n log n)` : appliquer `siftDown` sur les nœuds internes du **bas vers le haut** (`i = n/2 - 1` à `0`).

---

## 🛠️ Détails d'Implémentation

### 🎯 Points Forts du Code

- ✅ **Code commenté en français** (accessible aux débutants)
- ✅ **Benchmarks reproductibles** avec mesures multiples (médiane)
- ✅ **Vérification de correction** avant chaque benchmark
- ✅ **Comparaison équitable** (mêmes données, même charge)
- ✅ **Détection des pièges** : `map` strict échoue comme tas (clés uniques)

### ⚠️ Limites Connues

- ❌ **BST non équilibré** → dégénérescence sur données triées
- ❌ **Tas statique** (`Heap.cpp`) → capacité fixe (100 éléments)
- ❌ **Recherche dans un tas** → O(n) (pas optimisée)

### 🔧 Améliorations Suggérées

- 🔄 Remplacer le BST par un **AVL** ou **Rouge-Noir** pour garantir O(log n)
- 🔄 Utiliser un **`vector` redimensionnable** pour le tas (au lieu du tableau statique)
- 🔄 Ajouter un **cache-aware layout** (B-heap) pour de très grandes tailles
- 🔄 Implémenter un **tas indexé** (avec `unordered_map<value, position>`)

---

## 📊 Graphique de Complexité

```
Temps (log) │
            │  ┌─ BST dégénéré (O(n))
            │  │
            │  │
            │  │          ┌─ BST aléatoire (O(log n))
            │  │          │
            │  │          │       ┌─ BST équilibré (O(log n))
            │  │          │       │
            │  │          │       │
            │  │          │       │
            └──┴──────────┴───────┴──────────► N (log)
```

---

## 🎓 Cas d'Usage Recommandés

| Structure | ✅ Utiliser quand | ❌ Éviter quand |
|-----------|-------------------|-----------------|
| **BST équilibré** | Recherche dynamique ordonnée, parcours trié | Données massives sans équilibrage |
| **Tas binaire** | File de priorité, Dijkstra, k-plus proches | Recherche d'un élément arbitraire |
| **Tas 8-aire** | Push intensif, gros volumes | Pop intensif (préférer binaire) |
| **`std::multiset`** | Besoin d'un multiensemble ordonné | Performance pure (construction coûteuse) |
| **`std::map`** | Clés uniques, équilibrage garanti | Mémoire critique (33 % de plus que BST) |

---

## 🤝 Contribution

Les contributions sont les bienvenues ! Pour proposer une amélioration :

1. **Fork** le projet
2. **Créez** une branche (`git checkout -b feature/amelioration`)
3. **Committez** (`git commit -am 'Ajout AVL'`)
4. **Push** (`git push origin feature/amelioration`)
5. **Ouvrez** une Pull Request

### 🎯 Idées d'amélioration

- [ ] Implémentation d'un **arbre AVL** équilibré
- [ ] Implémentation d'un **arbre rouge-noir** from scratch
- [ ] Ajout d'un **tas indexé** (`unordered_map` pour O(1) lookup)
- [ ] Visualisation **graphique** des arbres (Graphviz / ASCII art)
- [ ] Support **Windows natif** (CMake)
- [ ] Tests unitaires avec **Catch2** ou **Google Test**

---

## 📜 Licence

Ce projet est distribué sous licence **MIT**. Voir [`LICENSE`](LICENSE) pour plus d'informations.

```
MIT License

Copyright (c) 2024 [Votre nom]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction...
```

---

## 🙏 Remerciements

- 📚 **Algorithmes** : Introduction to Algorithms (Cormen, Leiserson, Rivest, Stein)
- 🎓 **Cours** : ILISI2 — Virtualisation & Structures de Données
- 💻 **Communauté** : Stack Overflow, cppreference.com
- 🧪 **STL** : Alexander Stepanov & Meng Lee

---

<div align="center">

### ⭐ Si ce projet vous a aidé, n'oubliez pas de lui donner une étoile !

**Fait avec ❤️ et beaucoup de `std::chrono::steady_clock`**

[⬆ Retour en haut](#-arbres-de-recherche--structures-de-données-en-c)

</div>
