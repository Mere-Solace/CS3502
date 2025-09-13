#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <time.h>

int main(int argc, char *argv[]) {
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


   char line[2048];
   int line_count = 0;
   int char_count = 0;

   if (verbose) {
      while(fgets(line, sizeof(line), stdin)) {
         line_count++;
         char_count += strlen(line);

         fputs(line, stdout);

         if (max_lines > 0 && line_count >= max_lines) {
            break;
         }
      }
   }
   else {
      while(fgets(line, sizeof(line), stdin)) {
         line_count++;
         char_count += strlen(line);

         fputs(line, stdout);
         if (max_lines > 0 && line_count >= max_lines) {
            break;
         }
      }
   }

   // Print statistics to stderr
   // fprintf(stderr, "Lines: %d\n", line_count);

   fprintf(stderr, "Lines: %d\nCharacters: %d\n", line_count, char_count);

   return 0;
}

