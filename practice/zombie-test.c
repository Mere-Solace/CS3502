#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main() {
   int i;
   int pid = fork();

   if (pid == 0) {
      for(i = 0; i < 20; i++) 
         printf("I am a Child\n");
   }
   else {
      printf("I am Parent\n");
      while(1);
   }
}
