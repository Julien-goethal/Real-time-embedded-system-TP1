#include<stdio.h>

int function(int a, int b){

	return a+b;
	
	}
	


int main(){
	
	int a=7;
	int b=8;
	
	int c= function(a,b);
	
	printf("Value of a and b : %d, %d \nValue of their sum : %d\n", a,b,c);
		
}
