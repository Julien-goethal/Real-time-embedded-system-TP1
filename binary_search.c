#include<stdio.h>

int binary_search(int a[],int b){

	int i=a[0];
	int taille = sizeof(a)/sizeof(a[0])/2; // taille de la liste :2
	int compteur=1;

	while (i!=b){
		if (i<=a[taille]){
			i=a[taille];
			taille = taille + taille/ compteur;
			compteur++;
			}
		else {	
			i=a[taille];
			taille = taille - taille/compteur;
			compteur++;
			}

	}
	return i;
}

int main(){

	int y[6]={1,5,14,50,89,99},search_n=89;

	if (binary_search(y,search_n) == search_n){
		printf("On a trouvé le nombre !!\n");
		}

	else {
	printf("On a pas trouvé...\n");
	}

}
