#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <vector>
using namespace std;

// Chaque noeud contient une valeur et deux pointeurs vers ses enfants.
struct Noeud {
    int valeur;
    Noeud* gauche;
    Noeud* droite;

    Noeud(int nouvelleValeur) {
        valeur = nouvelleValeur;
        gauche = nullptr;
        droite = nullptr;
    }
};

// Insere la valeur a la bonne place en respectant l'ordre de l'arbre.
Noeud* inserer(Noeud* racine, int valeur) {
    if (racine == nullptr) {
        return new Noeud(valeur);
    }

    if (valeur < racine->valeur) {
        racine->gauche = inserer(racine->gauche, valeur);
    } else if (valeur > racine->valeur) {
        racine->droite = inserer(racine->droite, valeur);
    } else {
        cout << "Cette valeur existe deja dans l'arbre.\n";
    }

    return racine;
}

// Recherche une valeur dans l'arbre.
bool rechercher(Noeud* racine, int valeur) {
    if (racine == nullptr) {
        return false;
    }

    if (valeur == racine->valeur) {
        return true;
    }

    if (valeur < racine->valeur) {
        return rechercher(racine->gauche, valeur);
    }

    return rechercher(racine->droite, valeur);
}

// Retourne le noeud contenant la plus petite valeur d'un sous-arbre.
Noeud* trouverMinimum(Noeud* racine) {
    while (racine != nullptr && racine->gauche != nullptr) {
        racine = racine->gauche;
    }
    return racine;
}

// Supprime une valeur tout en preservant l'ordre de l'arbre.
Noeud* supprimer(Noeud* racine, int valeur) {
    if (racine == nullptr) {
        return nullptr;
    }

    if (valeur < racine->valeur) {
        racine->gauche = supprimer(racine->gauche, valeur);
    } else if (valeur > racine->valeur) {
        racine->droite = supprimer(racine->droite, valeur);
    } else if (racine->gauche == nullptr) {
        Noeud* droite = racine->droite;
        delete racine;
        return droite;
    } else if (racine->droite == nullptr) {
        Noeud* gauche = racine->gauche;
        delete racine;
        return gauche;
    } else {
        Noeud* minimum = trouverMinimum(racine->droite);
        racine->valeur = minimum->valeur;
        racine->droite = supprimer(racine->droite, minimum->valeur);
    }

    return racine;
}

// Le parcours gauche-racine-droite affiche les valeurs dans l'ordre croissant.
void afficherEnOrdre(Noeud* racine, ostream& sortie = cout) {
    if (racine != nullptr) {
        afficherEnOrdre(racine->gauche, sortie);
        sortie << racine->valeur << " ";
        afficherEnOrdre(racine->droite, sortie);
    }
}

// Libere tous les noeuds, en commencant par les enfants.
void libererArbre(Noeud* racine) {
    if (racine != nullptr) {
        libererArbre(racine->gauche);
        libererArbre(racine->droite);
        delete racine;
    }
}

// Mesure les durees d'insertion, de recherche et de suppression.
void testerPerformances(int taille) {
    vector<int> valeurs(taille);
    iota(valeurs.begin(), valeurs.end(), 0);

    // Melanger les valeurs pour eviter de construire un arbre volontairement trie.
    random_device generateurAleatoire;
    mt19937 generateur(generateurAleatoire());
    shuffle(valeurs.begin(), valeurs.end(), generateur);

    Noeud* arbreTest = nullptr;

    auto debutInsertion = chrono::steady_clock::now();
    for (int valeur : valeurs) {
        arbreTest = inserer(arbreTest, valeur);
    }
    auto finInsertion = chrono::steady_clock::now();

    int valeursTrouvees = 0;
    auto debutRecherche = chrono::steady_clock::now();
    for (int valeur : valeurs) {
        if (rechercher(arbreTest, valeur)) {
            ++valeursTrouvees;
        }
    }
    auto finRecherche = chrono::steady_clock::now();

    // Enregistre le parcours trie dans un fichier, sans inclure cette ecriture dans les mesures.
    ofstream fichierSortie("output.txt");
    if (!fichierSortie) {
        cerr << "Erreur : impossible de creer le fichier output.txt.\n";
    } else {
        fichierSortie << "Valeurs de l'arbre de test (ordre croissant) :\n";
        afficherEnOrdre(arbreTest, fichierSortie);
        fichierSortie << "\n";
        fichierSortie.close();

        if (fichierSortie) {
            cout << "Affichage de l'arbre enregistre dans output.txt.\n";
        } else {
            cerr << "Erreur : echec lors de l'ecriture dans output.txt.\n";
        }
    }

    auto debutSuppression = chrono::steady_clock::now();
    for (int valeur : valeurs) {
        arbreTest = supprimer(arbreTest, valeur);
    }
    auto finSuppression = chrono::steady_clock::now();

    const chrono::duration<double, milli> dureeInsertion = finInsertion - debutInsertion;
    const chrono::duration<double, milli> dureeRecherche = finRecherche - debutRecherche;
    const chrono::duration<double, milli> dureeSuppression = finSuppression - debutSuppression;

    cout << fixed << setprecision(3);
    cout << "\nTest termine pour " << taille << " valeurs.\n";
    cout << "Valeurs retrouvees : " << valeursTrouvees << " / " << taille << "\n";
    cout << "Temps total d'insertion  : " << dureeInsertion.count() << " ms\n";
    cout << "Temps total de recherche : " << dureeRecherche.count() << " ms\n";
    cout << "Temps total de suppression : " << dureeSuppression.count() << " ms\n";

    libererArbre(arbreTest);
}

// Compare la recherche binaire dans un vecteur trie avec la recherche dans std::map.
void comparerRecherches(int taille, bool ordreAleatoire) {
    vector<int> valeurs(taille);
    iota(valeurs.begin(), valeurs.end(), 0);

    if (ordreAleatoire) {
        mt19937 generateur(42);
        shuffle(valeurs.begin(), valeurs.end(), generateur);
    }

    vector<int> vecteurTrie;
    vecteurTrie.reserve(taille);

    auto debutConstructionVecteur = chrono::steady_clock::now();
    vecteurTrie = valeurs;
    sort(vecteurTrie.begin(), vecteurTrie.end());
    auto finConstructionVecteur = chrono::steady_clock::now();

    map<int, bool> dictionnaire;
    auto debutConstructionMap = chrono::steady_clock::now();
    for (int valeur : valeurs) {
        dictionnaire.emplace(valeur, true);
    }
    auto finConstructionMap = chrono::steady_clock::now();

    int valeursTrouveesVecteur = 0;
    auto debutRechercheBinaire = chrono::steady_clock::now();
    for (int valeur : valeurs) {
        if (binary_search(vecteurTrie.begin(), vecteurTrie.end(), valeur)) {
            ++valeursTrouveesVecteur;
        }
    }
    auto finRechercheBinaire = chrono::steady_clock::now();

    int valeursTrouveesMap = 0;
    auto debutRechercheMap = chrono::steady_clock::now();
    for (int valeur : valeurs) {
        if (dictionnaire.find(valeur) != dictionnaire.end()) {
            ++valeursTrouveesMap;
        }
    }
    auto finRechercheMap = chrono::steady_clock::now();

    const chrono::duration<double, milli> constructionVecteur =
        finConstructionVecteur - debutConstructionVecteur;
    const chrono::duration<double, milli> constructionMap =
        finConstructionMap - debutConstructionMap;
    const chrono::duration<double, milli> rechercheBinaire =
        finRechercheBinaire - debutRechercheBinaire;
    const chrono::duration<double, milli> rechercheMap =
        finRechercheMap - debutRechercheMap;

    cout << fixed << setprecision(3);
    cout << "\n--- Comparaison recherche binaire / std::map ---\n";
    cout << "Taille : " << taille << " valeurs\n";
    cout << "Ordre d'insertion : "
         << (ordreAleatoire ? "aleatoire" : "croissant") << "\n";
    cout << "Vecteur trie - construction et tri : "
         << constructionVecteur.count() << " ms\n";
    cout << "std::map - construction : "
         << constructionMap.count() << " ms\n";
    cout << "Recherche binaire - valeurs trouvees : "
         << valeursTrouveesVecteur << " / " << taille << "\n";
    cout << "Recherche binaire - temps total : "
         << rechercheBinaire.count() << " ms\n";
    cout << "std::map - valeurs trouvees : "
         << valeursTrouveesMap << " / " << taille << "\n";
    cout << "std::map - temps total de recherche : "
         << rechercheMap.count() << " ms\n";
}

int main() {
    Noeud* racine = nullptr;
    int choix;
    int valeur;

    // Le menu reste affiche jusqu'a ce que l'utilisateur choisisse de quitter.
    do {
        cout << "\n--- Arbre de recherche binaire ---\n";
        cout << "1. Inserer une valeur\n";
        cout << "2. Afficher l'arbre (ordre croissant)\n";
        cout << "3. Tester les performances\n";
        cout << "4. Comparer recherche binaire et std::map\n";
        cout << "0. Quitter\n";
        cout << "Votre choix : ";

        if (!(cin >> choix)) {
            cout << "Saisie invalide. Fin du programme.\n";
            break;
        }

        switch (choix) {
            case 1:
                cout << "Valeur a inserer : ";
                if (cin >> valeur) {
                    racine = inserer(racine, valeur);
                } else {
                    cout << "Saisie invalide. Fin du programme.\n";
                    libererArbre(racine);
                    return 1;
                }
                break;

            case 2:
                if (racine == nullptr) {
                    cout << "L'arbre est vide.\n";
                } else {
                    cout << "Valeurs : ";
                    afficherEnOrdre(racine);
                    cout << "\n";
                }
                break;

            case 3: {
                int taille;
                cout << "Nombre de valeurs a tester : ";
                if (cin >> taille && taille > 0) {
                    testerPerformances(taille);
                } else {
                    cout << "Saisie invalide : entrez un entier positif.\n";
                    libererArbre(racine);
                    return 1;
                }
                break;
            }

            case 4: {
                int ordre;
                int taille;
                cout << "Ordre des valeurs (1. Croissant, 2. Aleatoire) : ";
                if (!(cin >> ordre) || (ordre != 1 && ordre != 2)) {
                    cout << "Choix invalide : entrez 1 ou 2.\n";
                    libererArbre(racine);
                    return 1;
                }

                cout << "Nombre de valeurs a tester : ";
                if (!(cin >> taille) || taille <= 0) {
                    cout << "Saisie invalide : entrez un entier positif.\n";
                    libererArbre(racine);
                    return 1;
                }

                comparerRecherches(taille, ordre == 2);
                break;
            }

            case 0:
                cout << "Au revoir !\n";
                break;

            default:
                cout << "Choix invalide. Reessayez.\n";
        }
    } while (choix != 0);

    libererArbre(racine);
    return 0;
}


/*Terminal output:
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/Arbre de Recherche$ ./arbre_recherche_binaire

--- Arbre de recherche binaire ---
1. Inserer une valeur
2. Afficher l'arbre (ordre croissant)
3. Tester les performances
4. Comparer recherche binaire et std::map
0. Quitter
Votre choix : 3
Nombre de valeurs a tester : 1000000
Affichage de l'arbre enregistre dans output.txt.

Test termine pour 1000000 valeurs.
Valeurs retrouvees : 1000000 / 1000000
Temps total d'insertion  : 1054.678 ms
Temps total de recherche : 655.294 ms
Temps total de suppression : 918.044 ms

--- Arbre de recherche binaire ---
1. Inserer une valeur
2. Afficher l'arbre (ordre croissant)
3. Tester les performances
4. Comparer recherche binaire et std::map
0. Quitter
Votre choix : 3
Nombre de valeurs a tester : 1000000
Affichage de l'arbre enregistre dans output.txt.

Test termine pour 1000000 valeurs.
Valeurs retrouvees : 1000000 / 1000000
Temps total d'insertion  : 684.791 ms
Temps total de recherche : 406.372 ms
Temps total de suppression : 602.767 ms

--- Arbre de recherche binaire ---
1. Inserer une valeur
2. Afficher l'arbre (ordre croissant)
3. Tester les performances
4. Comparer recherche binaire et std::map
0. Quitter
Votre choix : ^C
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/Arbre de Recherche$ 
*/