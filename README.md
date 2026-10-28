alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/Arbre de Recherche$ ls
arbreRecerche  arbreRecerche.cpp  Heap  HeapComp.cpp  Heap.cpp
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/Arbre de Recherche$ g++ -O2 -std=c++17 HeapComp.cpp -o HeapComp
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
Tableau + swap (ton code)         10.4        12.8        12.1        14.6  
Tas binaire (trou)                11.2        13.0        14.0        14.8  
Tas 4-aire                         6.9         7.3         8.1         8.5  
Tas 8-aire                         6.4 *       5.1 *       5.2 *       5.2 *
priority_queue<vector>            13.5        15.3        17.3        18.7  
priority_queue<deque>             28.8        31.1        28.7        29.7  
std::multiset                     76.4       106.3       216.6       938.9  
BST non equilibre                138.1       186.2       269.3      1130.3  

=== 2. pop-min (vider le tas)   [ns par operation, plus petit = mieux] ===
                                  N=1000     N=10000    N=100000   N=1000000
Tableau + swap (ton code)         58.4        78.5       101.7       142.5  
Tas binaire (trou)                34.0        39.6        71.1       118.5  
Tas 4-aire                        70.8        84.7       121.0       166.5  
Tas 8-aire                        76.0        90.0       128.4       177.5  
priority_queue<vector>            39.5        47.3        68.8       114.2  
priority_queue<deque>             91.3       120.0       180.6       259.3  
std::multiset                     26.8 *      23.5 *      32.0 *      95.8 *
BST non equilibre                 31.7        40.0        58.0       132.7  

=== 3. push, donnees croissantes   [ns par operation, plus petit = mieux] ===
                                  N=1000     N=10000    N=100000   N=1000000
Tableau + swap (ton code)          1.1 *       1.0 *       0.9 *       1.1 *
Tas binaire (trou)                 2.1         1.7         2.6         2.4  
Tas 4-aire                         2.1         2.3         2.9         2.0  
Tas 8-aire                         1.9         2.4         2.4         2.1  
priority_queue<vector>             2.5         2.2         2.4         5.3  
priority_queue<deque>             10.7        10.8        10.7        12.1  
std::multiset                     57.9        72.6       133.1       282.5  
BST non equilibre               1257.1     39471.3             -           -

=== 4. push, donnees decroissantes   [ns par operation, plus petit = mieux] ===
                                  N=1000     N=10000    N=100000   N=1000000
Tableau + swap (ton code)          7.3         7.4        13.0        16.1  
Tas binaire (trou)                 8.9        10.8        15.3        17.2  
Tas 4-aire                         5.4         7.5         7.2 *       9.6  
Tas 8-aire                         4.2 *       5.9 *       7.3         8.1 *
priority_queue<vector>            12.5        16.6        22.5        33.0  
priority_queue<deque>             24.5        31.9        55.8        67.2  
std::multiset                     56.3        77.9       135.0       300.6  
BST non equilibre               1417.2     35205.8             -           -

=== 5. construction depuis tableau (build)   [ns par operation, plus petit = mieux] ===
                                  N=1000     N=10000    N=100000   N=1000000
Tableau + swap (ton code)          9.5        11.6        12.1        13.4  
Tas binaire (trou)                 5.9         5.6         5.9         6.8  
Tas 4-aire                         4.8         5.4         6.3         7.2  
Tas 8-aire                         3.7 *       4.6 *       4.4 *       5.2 *
priority_queue<vector>             7.0         7.5        10.2        10.3  
priority_queue<deque>             18.8        22.0        21.4        25.0  
std::multiset                     71.2       141.4       215.6       889.8  
BST non equilibre                 98.8       154.5       352.1      1054.7  

=== 6. mixte pop+push (Dijkstra / file de priorite)   [ns par operation, plus petit = mieux] ===
                                  N=1000     N=10000    N=100000   N=1000000
Tableau + swap (ton code)         47.8 *      56.5 *      69.6 *      81.6  
Tas binaire (trou)                52.8        65.0        69.6        73.9 *
Tas 4-aire                        51.4        71.1        81.4       104.4  
Tas 8-aire                        60.2        78.5        99.9       124.2  
priority_queue<vector>            68.4       100.4       126.6       172.7  
priority_queue<deque>             90.5       135.7       193.4       274.3  
std::multiset                    110.8       128.3       160.5       201.4  
BST non equilibre                107.3       141.1       208.4       261.8  

(* = meilleur de la colonne ; - = non mesure, trop lent)

############ CLASSEMENT GLOBAL ############
(1.00 = toujours le meilleur ; 2.00 = en moyenne 2x plus lent que le meilleur)

1. Tas 8-aire                   score = 1.45
2. Tas 4-aire                   score = 1.61
3. Tas binaire (trou)           score = 1.70
4. Tableau + swap (ton code)    score = 1.71
5. priority_queue<vector>       score = 2.32
6. priority_queue<deque>        score = 5.23
7. std::multiset                score = 14.33
8. BST non equilibre            score = 33.82
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/Arbre de Recherche$ 


Tests réalisés
push aléatoire : cas typique.
pop-min : vider tout le tas.
push croissant : meilleur cas du tas, pire cas du BST.
push décroissant : pire cas du tas.
build : construction depuis un tableau (Floyd en O(n) contre n insertions).
mixte pop+push : charge de type Dijkstra ou ordonnanceur, souvent la plus représentative en pratique.