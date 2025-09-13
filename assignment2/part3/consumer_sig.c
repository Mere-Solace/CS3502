#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <signal.h>

volatile sig_atomic_t shutdown_flag = 0;
volatile sig_atomic_t stats_flag = 0;

void handle_sigint(int sig) {
   shutdown_flag = 1;
}

void handle_sigusr1(int sig) {
   stats_flag = 1;
}

int main(int argc, char *argv[]) {
   struct sigaction sa1;
   sa1.sa_handler = handle_sigint;
   sigemptyset(&sa1.sa_mask);
   sa1.sa_flags = 0;
   sigaction(SIGINT, &sa1, NULL);

   struct sigaction sa2;
   sa2.sa_handler = handle_sigusr1;
   sigemptyset(&sa2.sa_mask);
   sa2.sa_flags = SA_RESTART;
   sigaction(SIGUSR1, &sa2, NULL);


   int max_lines = -1;
   int verbose = 0;
   char opt;

   // Parse arguments (-n max_lines, -v verbose)
   while((opt = getopt(argc, argv, "n:vh")) != -1) {
      switch(opt) {
         case 'n':
            max_lines = atoi(optarg);
            break;
         case 'v':
            verbose = 1;
            break;
         case 'h':
         default:
            printf("Usage %s [-n max_lines] [-v]\n", argv[0]);
            exit(EXIT_FAILURE);
      }
   }

   // Read from stdin line by line
   // Count lines and characters

   // If verbose, echo lines to stdout

   double throughput;
   
   char line[2048];
   int line_count = 0;
   int char_count = 0;

   int g_shut = 0;
   clock_t start = clock();

   while(fgets(line, sizeof(line), stdin)) {
      line_count++;
      char_count += strlen(line);

      if (verbose) fputs(line, stdout);

      if (max_lines > 0 && line_count >= max_lines) break;

      if (stats_flag) {
         clock_t cur = clock();
         double cur_cpu_time = ((double)(cur-start)) / CLOCKS_PER_SEC;

         throughput = char_count/((1000000)*cur_cpu_time); 

         fprintf(stderr, "[STATS]\nLines: %d\nCharacters: %d\n", line_count, char_count);
         fprintf(stderr, "Cpu time (sec): %f\nThroughput: %f\n\n", cur_cpu_time, throughput);
         stats_flag = 0;
      }

      if (shutdown_flag) {
         g_shut = 1;
         fprintf(stderr, "\n[SHUTDOWN] Graceful exit requested.\n");
         break;
      }
   }

   if (shutdown_flag && !g_shut) { fprintf(stderr, "\n[SHUTDOWN] Graceful exit requested.\n"); }

   clock_t end = clock();
   double cpu_time = ((double)(end-start)) / CLOCKS_PER_SEC;

   // Print statistics to stderr
   // fprintf(stderr, "Lines: %d\n", line_count);

   throughput = char_count/((1000000)*cpu_time); 

   fprintf(stderr, "Lines: %d\nCharacters: %d\n", line_count, char_count);
   fprintf(stderr, "Cpu time (sec): %f\nThroughput: %f\n\n", cpu_time, throughput);

   return 0;
}
