#define _POSIX_C_SOURCE 200809L
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#define NUM_ACCOUNTS 100
#define TRANSACTIONS_PER_TELLER 1000000
#define NUM_THREADS 32
#define MAX_TRANSACTION_AMOUNT 500000  // $5000.00 in cents
#define STARTING_AMOUNT 200000         // $2000.00 in cents
#define CACHELINE 64

int verbose = 0;

typedef struct TransferRecord {
   int type;
   long long amount;

   struct Account *source;
   struct Account *dest;
   
   time_t tot;
   struct timeval micro;

   struct TransferRecord *pair;
   struct TransferRecord *next;
} TransferRecord;

typedef struct Account {
   int account_id;
   long long balance;
  
   pthread_mutex_t lock;

   TransferRecord *sot;   // start of transactions
   TransferRecord *last;  // last transaction
} Account;

Account createAccount(int id, long long starting_balance) {
   TransferRecord *sot = malloc(sizeof(TransferRecord));
   sot->next = NULL;
   Account new_acct = { .account_id = id, .balance = starting_balance, .sot = sot, .last = sot };
   pthread_mutex_init(&new_acct.lock, NULL);
   return new_acct;
}

static void timespec_add_ns(struct timespec *ts, long ns_to_add) {
   ts->tv_nsec += ns_to_add;
   while (ts->tv_nsec >= 1000000000L) {
      ts->tv_nsec -= 1000000000L;
      ts->tv_sec += 1;
    }
}

int addTransferRecord(Account *source, Account *dest, long long amount) {
   if (verbose) {
      printf("|[Thread %ld] Attempting Transfer from %d to %d\n", pthread_self(), source->account_id, dest->account_id);
   }
   Account *acc_1 = source->account_id < dest->account_id ? source : dest;
   Account *acc_2 = source->account_id < dest->account_id ? dest : source;
   pthread_mutex_t *lock_1 = &acc_1->lock;
   pthread_mutex_t *lock_2 = &acc_2->lock;

   struct timespec ts;
   clock_gettime(CLOCK_REALTIME, &ts);
   timespec_add_ns(&ts, 10 * 1000000L); // 10 microseconds

   int timeout = pthread_mutex_timedlock(lock_1, &ts);
   if (timeout == ETIMEDOUT) {
      if (verbose) {
         printf("   first - {> Thread [%ld] timed out while waiting for Account [%d] <}\n", pthread_self(), acc_1->account_id);
      }
      return -1;
   }
   if (verbose) {
      printf("|[Thread %ld] Locked Account [%d]\n|[Thread %ld] Waiting on Account [%d]...\n", 
            pthread_self(), acc_1->account_id,
            pthread_self(), acc_2->account_id);
   }
   
   clock_gettime(CLOCK_REALTIME, &ts);
   timespec_add_ns(&ts, 10 * 1000000L); // 10 microseconds
   timeout = pthread_mutex_timedlock(lock_2, &ts);      
   if (timeout == ETIMEDOUT) {
      if (verbose) {
         printf("   second - {> Thread [%ld] timed out while waiting for Account [%d] <}\n", pthread_self(), acc_2->account_id);
      }
      pthread_mutex_unlock(lock_1);
      return -1;
   }
   source->balance -= amount;
   TransferRecord *s = malloc(sizeof(TransferRecord));
   s->type = -1;
   s->amount = amount;

   time(&s->tot);
   gettimeofday(&s->micro, NULL);

   s->next = NULL;
   source->last->next = s;
   source->last = s;

   dest->balance += amount;
   TransferRecord *d = malloc(sizeof(TransferRecord));
   d->type = +1;
   d->amount = amount;

   time(&d->tot);
   gettimeofday(&d->micro, NULL);

   d->next = NULL;
   dest->last->next = d;
   dest->last = d;

   s->pair = d;
   d->pair = s;

   pthread_mutex_unlock(lock_2);
   pthread_mutex_unlock(lock_1);
   return 0;
}

void printRecord(Account *acc) {
   TransferRecord *cur = acc->sot->next;
   struct tm local;
   char buffer[64];

   long long curDel = 0L;
   int x = 1;
   if (verbose) {
      printf("\n~ Start of Transactions for Account: %d ~\n", acc->account_id);
   }
   while (cur != NULL) {
      localtime_r(&cur->tot, &local);
      strftime(buffer, sizeof(buffer), "%H:%M:%S", &local);

      if (cur->type != 0 && verbose) {
         printf(" [%s.%06ld] Transaction %d:\n | Type: %s - Amount: $%.2f\n", 
               buffer, 
               cur->micro.tv_usec,
               x, 
               cur->type == -1 ? "Withdrawal" : "Deposit", 
               (cur->amount)/100.00);
      }
      
      curDel += cur->amount * cur->type;
      cur = cur->next;
      x++;
   }
   printf("\n > [ Transaction Summary - Account: %d ]\n", acc->account_id);
   printf("   | Current Balance:....$%.2f\n   | Net Change:.........$%.2f\n", (acc->balance)/100.00, (curDel)/100.00);  
}

void freeRecord(Account *acc) {
   TransferRecord *gc = acc->sot; // garbage collector
   TransferRecord *cur = acc->sot->next;
   int x = 0;
   while(cur != NULL) {
      free(gc);
      gc = cur;
      cur = cur->next;
   }
}

Account accounts[NUM_ACCOUNTS];

typedef struct PaddedRow {
    long long transaction_data[NUM_ACCOUNTS];
    char pad[CACHELINE]; // pad to next cache line
} PaddedRow;

PaddedRow teller_log[NUM_THREADS];

void *teller_thread(void *arg) {
   int teller_id = *(int *)arg;
   
   srand(time(NULL));
   unsigned int seed = time(NULL) + pthread_self();

   int source_acct_num = rand_r(&seed) % NUM_ACCOUNTS;
   int dest_acct_num = rand_r(&seed) % NUM_ACCOUNTS;
   while (dest_acct_num == source_acct_num) {
      dest_acct_num = rand_r(&seed) % NUM_ACCOUNTS;
   }

   int redo = 0;
   for (int i = 0; i < TRANSACTIONS_PER_TELLER; i++) {
      if (!redo) {
         redo = 0;
         source_acct_num = rand_r(&seed) % NUM_ACCOUNTS;
         dest_acct_num = rand_r(&seed) % NUM_ACCOUNTS;
         while (dest_acct_num == source_acct_num) {
            dest_acct_num = rand_r(&seed) % NUM_ACCOUNTS;
         }
      }

      long long amount = 1 + rand_r(&seed) % ((MAX_TRANSACTION_AMOUNT)-2);
      if (addTransferRecord(&accounts[source_acct_num], &accounts[dest_acct_num], amount) == -1) {
         i--;
         redo = 1;
         continue;
      }

      teller_log[teller_id].transaction_data[source_acct_num] -= amount; // save amount in teller-specific data struct
      teller_log[teller_id].transaction_data[dest_acct_num] += amount;
      double dollar_amount = (amount)/100.00;
      if (verbose) {
         printf("\n >>>+ Successful Transaction +<<<\n|> Teller [ %d ] t#{ %d } ||| Source [ %d ] -($%.2f) --> Dest [ %d ] +($%.2f)\n\n", 
            teller_id,
            i,
            source_acct_num,
            dollar_amount, 
            dest_acct_num,
            dollar_amount
         );
      }
   }
}

pthread_t threads[NUM_THREADS];
int thread_ids[NUM_THREADS];

int main(int argc, char *argv[]) {
   char opt;
   while ((opt = getopt(argc, argv, "cvh")) != -1) {
      switch (opt) {
         case 'v':
            verbose = 1;
         case 'c':
            printf("\nCurrently compiled with:\n");
            printf("\nNumber of Accounts:.........%d\nNumber of Tellers:..........%d\nTransactions Per Teller:....%d", NUM_ACCOUNTS, NUM_THREADS, TRANSACTIONS_PER_TELLER);
            printf("\nMax Transaction Amount:.....$%.2f\n\n", MAX_TRANSACTION_AMOUNT/100.00);
            exit(EXIT_SUCCESS);
            return 0;
         case 'h':
         default:
            printf("\nIncorrect Usage\n");
            exit(EXIT_FAILURE);
            return -1;
      }
   }
   memset(teller_log, 0, sizeof(teller_log));

   for (int i = 0; i < NUM_ACCOUNTS; i++) {
      accounts[i] = createAccount(i, STARTING_AMOUNT);
   }

   clock_t start = clock();

   for (int i = 0; i < NUM_THREADS; i++) {
      thread_ids[i] = i;
      pthread_create(&threads[i], NULL, teller_thread, &thread_ids[i]);
   }

   for (int i = 0; i < NUM_THREADS; i++) {
      pthread_join(threads[i], NULL);
   }

   clock_t end = clock();
   double cpu_time = ((double)(end-start)) / CLOCKS_PER_SEC;

   int numIncorrect = 0;
   long long correct = 0;
   for (int i  = 0; i < NUM_ACCOUNTS; i++) {
      printRecord(&accounts[i]);
      for (int t = 0; t < NUM_THREADS; t++) {
         correct += teller_log[t].transaction_data[i];
      }
      
      printf("   |> Correct Value:.....$%.2f\n", correct/100.00);
      numIncorrect += ((correct + STARTING_AMOUNT - accounts[i].balance) == 0) ? 0 : 1;
      correct = 0;
   }

   // this validation works because all of the currency is only transferred, no new currency is introduced to the system.
   printf("\n ~ Validating Correctness of Transfers:\n  (Sum of all acct balances)/(NUM_ACCOUNTS) = STARTING_AMOUNT\n");
   long long total = 0;
   for (int i = 0; i < NUM_ACCOUNTS; i++) {
      total += accounts[i].balance;
   }
   printf("   Starting Amount Per Account: $%.2f, Final Average: $%.2f\n", (STARTING_AMOUNT)/100.00, (total/NUM_ACCOUNTS)/100.00);
   printf("\n ~ Number of Accounts with incorrect balances: %d,  %.3f%%\n\n", numIncorrect, 1.00*(numIncorrect/NUM_ACCOUNTS));
   
   printf("Number of Accounts:.........%d\nNumber of Tellers:..........%d\nTransactions Per Teller:....%d", NUM_ACCOUNTS, NUM_THREADS, TRANSACTIONS_PER_TELLER);
   printf("\nMax Transaction Amount:.....$%.2f\n", MAX_TRANSACTION_AMOUNT/100.00);
   printf("\n\nThis run (MUTEX & Deadlock Prevention Implemented) took: %.6f units of CPU-time\n\n", cpu_time);
   
   for (int i = 0; i < NUM_ACCOUNTS; i++) {
      pthread_mutex_destroy(&accounts[i].lock);
      freeRecord(&accounts[i]);
   }
   return 0;
}