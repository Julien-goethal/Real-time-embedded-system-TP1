#include<stdio.h>
#include<time.h>
#include <unistd.h>

int main(){

	 time_t begin = time(NULL);
    
    for (int i=0;i<5;i++){
    
    	sleep( 1 );    // Wait 2 seconds, but no ticks are consumed
    	time_t end = time(NULL);
    	unsigned long secondes = (unsigned long) difftime( end, begin );
    	printf( "Finished in %ld sec\n", secondes );
    }
    
      
}
