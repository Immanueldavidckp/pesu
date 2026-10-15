#include<stdio.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<sys/types.h>
 
union semun{
 int val;
 struct semid_ds *buf;
 unsigned short *array;
 //struct sminfo *_buf;
};
 
int main(){
 int key,id,i;
 union semun arg;
 key = ftok("semaphore.c",'d');
 printf("\nkey=%d",key);
 id=semget(key,1,IPC_CREAT|0644);
 printf("\nsem id= %d\n",id);
 arg.val =1;
 semctl(id,0,SETVAL,arg);
}
