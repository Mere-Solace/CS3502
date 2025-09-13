#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

int max_message = 512;

int main() {
   int pipe1[2]; // Parent to child
   int pipe2[2]; // Child to parent 
   pid_t pid;

   // TODO: Create both pipes
   // if (pipe(pipe1) ==-1) { /* error handling */ }
   if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
      perror("pipe");
      exit(EXIT_FAILURE);
   }

   // TODO: Fork process
   pid = fork();
   if (pid == -1) {
      perror("fork");
      exit(EXIT_FAILURE);
   }

   if (pid == 0) {
      // Child process
      // TODO: Close unused pipe ends
      close(pipe1[1]); // Close write end of pipe1
      close(pipe2[0]); // Close read end of pipe2

      time_t now;
      struct tm *local;
      
      time(&now);
      local = localtime(&now);
      // TODO: Read from parent, send responses
   
      char buffer[1024];
      read(pipe1[0], buffer, 1024);
      printf("[%02d:%02d:%02d] Parent said: %s\n", local->tm_hour, local->tm_min, local->tm_sec, buffer);
     
      printf("You are the Child. Send a message to the Parent:\n");
      char message[max_message];
      fgets(message, sizeof(message), stdin);
      printf("\n");

      write(pipe2[1], message, strlen(message) + 1);

      close(pipe1[0]);
      close(pipe2[1]);
      exit(0);
   } else {
      // Parent process
      // TODO: Close unused pipe ends
      close(pipe1[0]); // Close read end of pipe1
      close(pipe2[1]); // Close write end of pipe2
      
      // TODO: Send messages, read responses
      // TODO: wait() for child
      
      printf("You are the Parent. Send a message to your Child:\n");
      char message[max_message];
      fgets(message, sizeof(message), stdin);
      printf("\n");

      write(pipe1[1], message, strlen(message) + 1);

      time_t now;
      struct tm *local;

      time(&now);
      local = localtime(&now);

      char buffer[1024];
      read(pipe2[0], buffer, 1024);
      printf("[%02d:%02d:%02d] Child said: %s\n", local->tm_hour, local->tm_min, local->tm_sec, buffer);

      close(pipe1[1]);
      close(pipe2[0]);

      wait(NULL);
   }

   return 0;
}
