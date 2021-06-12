#include <iostream>
#include <unistd.h>
#include "uthreads.h"

void g(void) {
    int i = 0;
    while (1) {
        ++i;
        printf("in g (%d)\n", i);
    sleep(1);
   
    }
}
void h(void){
int i  = 0;
while(1){
++i; 
printf("in h (%d)\n",i);
sleep(1);
}
}
void f(void) {
    int i = 0;
    while (1) {
        ++i;
        printf("in f (%d)\n", i);
	sleep(1);
    }
}

void l(void) {
    int i = 0;
    while (1) {
        ++i;
        printf("in l (%d)\n", i);
        sleep(1);
    }
}

int arr[4]= {10,7,6,5} ;
int main() {
   uthread_init(arr,4);
   int d4=-1;
   int d1= uthread_spawn(f,1);
   int d2= uthread_spawn(g,2);
   int d3= uthread_spawn(h,3);
    while(1){
	  //  uthread_change_priority(1,0);
	   printf("fId,gId,hId,lid= %d,%d,%d,%d\n", d1,d2,d3,d4);
	    sleep(0.1);
	    if (d2){
	   uthread_terminate(d2);
	   d4 = uthread_spawn(l,1);
	   printf("d4=%d\n ",d4);
	   d2 = 0;
	    }
    }
    return 0;
}
