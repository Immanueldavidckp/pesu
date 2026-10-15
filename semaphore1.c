#include<stdio.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<sys/msg.h>
 
int main(){
 int key,semid,i;
 
 key = ftok("semaphore.c",'d');
 printf("\nkey=%d",key);
 struct sembuf buf ={0,-1,0};
 semid=semget(key,1,0);
 printf("\n before entering in to the critical section\n");
 semop(semid,&buf,1);/*lock*/
 printf("inside the critical section\n");
 getchar();
 buf.sem_op=1;
 semop(semid,&buf,1);/*unlock*/
}
