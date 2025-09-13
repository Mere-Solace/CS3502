#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>

int main(int argc, char *argv[]) {
   FILE *input = stdin;
   int buffer_size = 4096;
   char opt;

   // Parse command line arguments
   //-f filename (optional)
   //-b buffer_size (optional)
   char *filename = NULL;
   int verbose = 0;

   while ((opt = getopt(argc, argv, "f:b:vh")) != -1) {
      switch (opt) {
         case 'f':
            filename = optarg;
            break;
         case 'b':
            buffer_size = atoi(optarg);
            buffer_size = buffer_size < 0 ? 4096 : buffer_size;
            break;
         case 'v':
            verbose = 1;
            break;
         case 'h':
         default:
            printf("Usage: %s [-f file] [-b size] [-v] \n", argv[0]);
            exit(1);
      }
   }

   // Open file if -f provided
   if (filename) {
      input = fopen(filename, "r");
   }

   // Allocate buffer
   char *buffer = malloc(buffer_size);
   if (!buffer) {
      perror("malloc");
      exit(EXIT_FAILURE);
   }

   // Read from input and write to stdout
   size_t n;
   while((n = fread(buffer, 1, buffer_size, input)) > 0) {
      fwrite(buffer, 1, n, stdout);
   }

   // Cleanup

   free(buffer);
   if (filename) fclose(input);

   return 0;
}
