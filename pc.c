#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
 
int main(){
 int pid;
 pid=fork();
 if(pid<0){
  printf("can't fork\n");
 }else if(pid==0){
  printf("child process\n");
  printf("parent id and child id is : %d ,%d ",getppid(),getpid());
 }else{
  sleep(10);
  printf("parent process\n");
  printf("parent id and child id is : %d ,%d ",getpid(),pid);
 }
return 0;
}
