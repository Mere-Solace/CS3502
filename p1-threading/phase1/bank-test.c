#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <getopt.h>

#define NUM_ACCOUNTS 1
#define TRANSACTIONS_PER_TELLER 1000
#define NUM_THREADS 1000
#define TEST_TRANSACTION_AMOUNT 1

int verbose = 0;

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

      if (cur->type != 0 && verbose) {
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

   printf("\n > [ Transaction Summary - Account: %d ]\n", acc->account_id);
   printf("   | Current Balance:....$%.2f\n   | Net Change:.........$%.2f\n", acc->balance, curDel);  
}

void freeRecord(Account *acc) {
   Record *gc = acc->sot; // garbage collector
   Record *cur = acc->sot->next;
   int x = 0;
   while(cur != NULL) {
      free(gc);
      gc = cur;
      cur = cur->next;
      // printf("Freed record %d\n", ++x);
   }
}

Account accounts[NUM_ACCOUNTS];

void *teller_thread(void *arg) {
   int teller_id = *(int *)arg;

   for (int i = 0; i < TRANSACTIONS_PER_TELLER; i++) {
      addAccountRecord(&accounts[0], 1, TEST_TRANSACTION_AMOUNT);
   }
}

pthread_t threads[NUM_THREADS];
int thread_ids[NUM_THREADS];

int main(int argc, char *argv[]) {
   char opt;
   while ((opt = getopt(argc, argv, "vh")) != -1) {
      switch (opt) {
         case 'v': {
            verbose = 1;
         }
      }
   }

   accounts[0] = createAccount(0, 0.0);

   for (int i = 0; i < NUM_THREADS; i++) {
      thread_ids[i] = i;
      pthread_create(&threads[i], NULL, teller_thread, &thread_ids[i]);
   }

   for (int i = 0; i < NUM_THREADS; i++) {
      pthread_join(threads[i], NULL);
   }

   printRecord(&accounts[0]);
   printf("   |> Correct Value:.....$%.2f\n\n\n", NUM_THREADS*TRANSACTIONS_PER_TELLER*TEST_TRANSACTION_AMOUNT*1.00);

   for (int i = 0; i < NUM_ACCOUNTS; i++) {
      freeRecord(&accounts[i]);
   }

   printf("\nNumber of Accounts: %d\nNumber of Tellers: %d\nNumber of Transactions Per Teller: %d\n\n", NUM_ACCOUNTS, NUM_THREADS, TRANSACTIONS_PER_TELLER);
}

