#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <getopt.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

#define MAX_NUM_ACCOUNTS 10000
#define MAX_NUM_THREADS 256

int NUM_ACCOUNTS = 10;
int NUM_THREADS = 32;

int TRANSACTIONS_PER_TELLER = 1000;
int MAX_TRANSACTION_AMOUNT = 50;

int verbose = 0;
int test = 0;

typedef struct Record {
   int type;               // 1 for deposit, -1 for withdrawal
   double amount;
   time_t tot;             // time of transaction
   struct timeval micro;   // microseconds at tot
   struct Record *next;
} Record;

typedef struct Account {
   int account_id;
   double balance;

   Record *sot;   // start of transactions
   Record *last;  // last transaction
} Account;

Account createAccount(int id, double starting_balance) {
   Record *sot = malloc(sizeof(Record));

   Account new_acct = { .account_id = id, .balance = starting_balance, .sot = sot, .last = sot };

   return new_acct;
}

void addAccountRecord(Account *acc, int type, double amount) {
   acc->balance += (type * amount);
   Record *r = calloc(1, sizeof(Record));
   r->type = type;
   r->amount = amount;
  
   time(&r->tot);
   gettimeofday(&r->micro, NULL);

   r->next = NULL;
   acc->last->next = r;
   acc->last = r; 
} 

void printRecord(Account *acc) {
   Record *cur = acc->sot->next;
   struct tm local;
   char buffer[64];

   double curDel = 0;
   int x = 1;
   
   while (cur != NULL) {
      localtime_r(&cur->tot, &local);
      strftime(buffer, sizeof(buffer), "%H:%M:%S", &local);

      if (cur->type != 0 && test) {
         printf("[%s.%06ld] Transaction %d:\n| >> Type: %s | Amount: $%.2f\n|\n", 
               buffer, 
               cur->micro.tv_usec,
               x, 
               cur->type == -1 ? "Withdrawal" : "Deposit", 
               cur->amount);
      }
      
      curDel += cur->amount * cur->type;
      cur = cur->next;
      x++;
   }

   printf("\n> [ Transaction Summary - Account: %d ]\n", acc->account_id);
   printf("   | Current Balance:....$%.2f\n   | Net Change:.........$%.2f\n", acc->balance, curDel);  
}

void freeRecord(Account *acc) {
   Record *gc = acc->sot; // garbage collector
   Record *cur = acc->sot->next;
   while(cur != NULL) {
      free(gc);
      gc = cur;
      cur = cur->next;
   }
}

Account accounts[MAX_NUM_ACCOUNTS];
double teller_log[MAX_NUM_ACCOUNTS][MAX_NUM_ACCOUNTS];

void *teller_thread(void *arg) {
   int teller_id = *(int *)arg;

   unsigned int seed = time(NULL) + pthread_self();

   for (int i = 0; i < TRANSACTIONS_PER_TELLER; i++) {
      int acct_num = rand_r(&seed) % NUM_ACCOUNTS;
      double amount = (1 + rand_r(&seed) % ((MAX_TRANSACTION_AMOUNT*100)-2))/100.00;
      int random = (rand_r(&seed) % 2) - 1;
      int type = random == 0 ? -1 : 1;
      teller_log[teller_id][acct_num] += type*amount; // save amount in teller-specific data struct
      
      if (verbose) {
         printf("> Teller [ %d ]\t t#{ %d }\t Acct [ %d ]\t $%.2f\n", 
            teller_id,
            i,
            acct_num,
            type*amount 
         );
      }
      addAccountRecord(&accounts[acct_num], type, amount);
   }
   return NULL;
}

pthread_t threads[MAX_NUM_THREADS];
int thread_ids[MAX_NUM_THREADS];

int main(int argc, char *argv[]) {
   char opt;
   while ((opt = getopt(argc, argv, "a:p:o:m:tcvh")) != -1) {
      switch (opt) {
         case 'a':
            NUM_ACCOUNTS = atoi(optarg);
            NUM_ACCOUNTS = NUM_ACCOUNTS < MAX_NUM_ACCOUNTS ? NUM_ACCOUNTS : MAX_NUM_ACCOUNTS;
            break;
         case 'p':
            NUM_THREADS = atoi(optarg);
            NUM_THREADS = NUM_THREADS < MAX_NUM_THREADS ? NUM_THREADS : MAX_NUM_THREADS;
            break;
         case 'o':
            TRANSACTIONS_PER_TELLER = atoi(optarg);
            break;
         case 'm':
            MAX_TRANSACTION_AMOUNT = atoi(optarg);
            break;
         case 't':
            test = 1;
            break;
         case 'v':
            verbose = 1;
            break;
         case 'c':
            printf("\nCurrently compiled with:\n");
            printf("\nNumber of Accounts:.........%d\nNumber of Tellers:..........%d\nTransactions Per Teller:....%d", NUM_ACCOUNTS, NUM_THREADS, TRANSACTIONS_PER_TELLER);
            printf("\nMax Transaction Amount:.....$%.2f\n\n", MAX_TRANSACTION_AMOUNT*1.00);
            exit(EXIT_SUCCESS);
            return 0;
         case 'h':
         default:
            printf("\nIncorrect Usage\n");
            exit(EXIT_FAILURE);
            return -1;
      }
   }

   for (int i = 0; i < NUM_ACCOUNTS; i++) {
      accounts[i] = createAccount(i, 0.00);
   }

   clock_t start = clock();

   for (int i = 0; i < NUM_THREADS; i++) {
      thread_ids[i] = i;
      int rc = pthread_create(&threads[i], NULL, teller_thread, &thread_ids[i]);
      if (rc != 0) {
         fprintf(stderr, "Error: pthread_join failed for thread %d: %s\n", i, strerror(rc));
         exit(EXIT_FAILURE);
      }
   }

   for (int i = 0; i < NUM_THREADS; i++) {
      int rc = pthread_join(threads[i], NULL);
      if (rc != 0) {
         fprintf(stderr, "Error: pthread_join failed for thread %d: %s\n", i, strerror(rc));
         exit(EXIT_FAILURE);
      }
   }
   
   clock_t end = clock();
   double cpu_time = ((double)(end-start)) / CLOCKS_PER_SEC;
   
   int numIncorrect = 0;

   for (int i  = 0; i < NUM_ACCOUNTS; i++) {
      printRecord(&accounts[i]);
      
      double correct;
      for (int t = 0; t < NUM_THREADS; t++) {
         correct += teller_log[t][i];
      }
      
      printf("   |> Correct Value:.....$%.2f\n", correct);
      
      if ((int) (round(correct*100)) != (int) (round(accounts[i].balance*100))) {
         numIncorrect++;
         printf("     { Incorrect balance }\n");
      }

      correct = 0.00;

      freeRecord(&accounts[i]);
   }
   printf("\nNumber of accounts with incorrect balance: %d, %.3f%% incorrect\n\n", numIncorrect, (100.00*numIncorrect/NUM_ACCOUNTS));
   printf("\nNumber of Accounts:.........%d\nNumber of Tellers:..........%d\nTransactions Per Teller:....%d", NUM_ACCOUNTS, NUM_THREADS, TRANSACTIONS_PER_TELLER);
   printf("\nMax Transaction Amount:.....$%.2f\n", MAX_TRANSACTION_AMOUNT*1.00);
   printf("\n\nThis run (no MUTEX) took: %.6f\n\n", cpu_time);

   return 0;
}

