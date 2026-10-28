#include <iostream>
using namespace std;

const int MAX = 100;

struct Heap {
	int data[MAX];
	int size;
};

void init(Heap &h)
{
	h.size = 0;
}

// indices dans le tableau
int parent(int i) { return (i - 1) / 2; }
int leftChild(int i) { return 2 * i + 1; }
int rightChild(int i) { return 2 * i + 2; }

void swapValues(int &a, int &b)
{
	int tmp = a;
	a = b;
	b = tmp;
}

// remonte un élément tant qu'il est plus petit que son parent
void siftUp(Heap &h, int i)
{
	while (i > 0 && h.data[i] < h.data[parent(i)]) {
		swapValues(h.data[i], h.data[parent(i)]);
		i = parent(i);
	}
}

// descend un élément tant qu'il est plus grand qu'un de ses enfants
void siftDown(Heap &h, int i)
{
	while (true) {
		int smallest = i;
		int l = leftChild(i);
		int r = rightChild(i);

		if (l < h.size && h.data[l] < h.data[smallest])
			smallest = l;
		if (r < h.size && h.data[r] < h.data[smallest])
			smallest = r;

		if (smallest == i)
			break;

		swapValues(h.data[i], h.data[smallest]);
		i = smallest;
	}
}

// ajoute une valeur : O(log n)
void insert(Heap &h, int value)
{
	if (h.size == MAX) {
		cout << "Heap plein\n";
		return;
	}
	h.data[h.size] = value;
	siftUp(h, h.size);
	h.size++;
}

// retourne le minimum (la racine) : O(1)
int getMin(Heap &h)
{
	return h.data[0];
}

// supprime et retourne le minimum : O(log n)
int extractMin(Heap &h)
{
	int min = h.data[0];
	h.data[0] = h.data[h.size - 1];
	h.size--;
	siftDown(h, 0);
	return min;
}

// affiche le tableau
void displayArray(Heap &h)
{
	for (int i = 0; i < h.size; i++)
		cout << h.data[i] << " ";
	cout << "\n";
}

// affiche l'arbre à l'horizontale (racine à gauche)
void displayTree(Heap &h, int i = 0, int space = 0)
{
	const int STEP = 6;

	if (i >= h.size)
		return;

	space += STEP;

	displayTree(h, rightChild(i), space);

	cout << "\n";
	for (int k = STEP; k < space; k++)
		cout << " ";
	cout << h.data[i] << "\n";

	displayTree(h, leftChild(i), space);
}

int main()
{
	Heap h;
	init(h);

	int values[] = {5, 3, 8, 1, 4, 7, 9, 2, 6, 10};
	for (int v : values)
		insert(h, v);

	cout << "Tableau : ";
	displayArray(h);

	cout << "\nArbre :\n";
	displayTree(h);

	cout << "\nMin : " << getMin(h) << "\n";

	// extraction dans l'ordre croissant (heap sort)
	cout << "\nExtraction : ";
	while (h.size > 0)
		cout << extractMin(h) << " ";
	cout << "\n";

	return 0;
}

/*
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/Arbre de Recherche$ ./Heap
Tableau : 1 2 7 3 4 8 9 5 6 10 

Arbre :

            9

      7

            8

1

            4

                  10

      2

                  6

            3

                  5

Min : 1

Extraction : 1 2 3 4 5 6 7 8 9 10 
alan@alan-HP-EliteBook-850-G8-Notebook-PC:~/Documents/ILISI2/Virtualisation/C++/Arbre de Recherche$ 
*/