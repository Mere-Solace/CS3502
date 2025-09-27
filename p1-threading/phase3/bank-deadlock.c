#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#define NUM_ACCOUNTS 5
#define TRANSACTIONS_PER_TELLER 10
#define NUM_THREADS 10
#define MAX_TRANSACTION_AMOUNT 1000
#define STARTING_AMOUNT 2000
#define VERBOSE 1

typedef struct TransferRecord {
   int type;
   double amount;
   struct Account *source;
   struct Account *dest;
   time_t tot;
   struct timeval micro;
   struct TransferRecord *pair;
   struct TransferRecord *next;
} TransferRecord;

typedef struct Account {
   int account_id;
   double balance;
  
   pthread_mutex_t lock;

   TransferRecord *sot;   // start of transactions
   TransferRecord *last;  // last transaction
} Account;


Account createAccount(int id, double starting_balance) {
   TransferRecord *sot = malloc(sizeof(TransferRecord));
   Account new_acct = { .account_id = id, .balance = starting_balance, .sot = sot, .last = sot };
   pthread_mutex_init(&new_acct.lock, NULL);
   return new_acct;
}

void addTransferRecord(Account *source, Account *dest, int type, double amount) {
   if (pthread_mutex_lock(&source->lock) != 0) {
      perror("Failed to aquire lock");
      return;
   }
   if (pthread_mutex_lock(&dest->lock) != 0) {
      perror("Failed to aquire lock");
      return;
   }

   source->balance -= (type*amount);
   TransferRecord *s = calloc(1, sizeof(TransferRecord));
   s->type = (-1*type);
   s->amount = amount;

   time(&s->tot);
   gettimeofday(&s->micro, NULL);

   s->next = NULL;
   source->last->next = s;
   source->last = s;

   dest->balance += (type*amount);
   TransferRecord *d = calloc(1, sizeof(TransferRecord));
   d->type = type;
   d->amount = amount;

   time(&d->tot);
   gettimeofday(&d->micro, NULL);

   d->next = NULL;
   dest->last->next = d;
   dest->last = d;

   s->pair = d;
   d->pair = s;

   pthread_mutex_unlock(&source->lock);
   pthread_mutex_unlock(&dest->lock);
}

void printRecord(Account *acc, int print_all) {
   TransferRecord *cur = acc->sot->next;
   struct tm local;
   char buffer[64];

   double curDel = 0;
   int x = 1;

   printf("\n~ Start of Transactions for Account: %d ~\n", acc->account_id);
   while (cur != NULL) {
      localtime_r(&cur->tot, &local);
      strftime(buffer, sizeof(buffer), "%H:%M:%S", &local);

      if (cur->type != 0 && print_all) {
         printf(" [%s.%06ld] Transaction %d:\n | Type: %s - Amount: $%.2f\n", 
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

Account accounts[NUM_ACCOUNTS];
double teller_log[NUM_THREADS][NUM_ACCOUNTS];

void *teller_thread(void *arg) {
   int teller_id = *(int *)arg;
   
   srand(time(NULL));
   unsigned int seed = time(NULL) + pthread_self();

   for (int i = 0; i < TRANSACTIONS_PER_TELLER; i++) {
      int source_acct_num = rand_r(&seed) % NUM_ACCOUNTS;
      int dest_acct_num = rand_r(&seed) % NUM_ACCOUNTS;
      while (dest_acct_num == source_acct_num) {
         dest_acct_num = rand_r(&seed) % NUM_ACCOUNTS;
      }

      double amount = (1 + rand_r(&seed) % ((MAX_TRANSACTION_AMOUNT*100)-2))/100.00;
      int random = (rand() % 2) - 1;
      int type = random == 0 ? -1 : 1;
      teller_log[teller_id][source_acct_num] -= type*amount; // save amount in teller-specific data struct
      teller_log[teller_id][dest_acct_num] += type*amount;

      printf("|> Teller [ %d ] t#{ %d }\t Source [ %d ] -($%.2f) --> Dest [ %d ] +($%.2f)\n", 
         teller_id,
         i,
         source_acct_num,
         type*amount, 
         dest_acct_num,
         type*amount
      );
      addTransferRecord(&accounts[source_acct_num], &accounts[dest_acct_num], type, amount);
   }
}

pthread_t threads[NUM_THREADS];
int thread_ids[NUM_THREADS];

int main() {
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

   for (int i  = 0; i < NUM_ACCOUNTS; i++) {
      printRecord(&accounts[i], VERBOSE);
      double correct;
      for (int t = 0; t < NUM_THREADS; t++) {
         correct += teller_log[t][i];
      }
      
      printf("   |> Correct Value:.....$%.2f\n", correct);
      correct = 0;
   }

   printf("\n ~ Validating Correctness of Transfers:\n  (Sum of all acct balances)/(NUM_ACCOUNTS) = STARTING_AMOUNT\n");
   double total = 0;
   for (int i = 0; i < NUM_ACCOUNTS; i++) {
      total += accounts[i].balance;
   }
   printf("  Starting Amount: $%d, Actual: $%.2f\n\n   | %s |\n\n", STARTING_AMOUNT, total/NUM_ACCOUNTS, STARTING_AMOUNT == (int)(total/NUM_ACCOUNTS) ? "VALID" : "INVALID");

   printf("\nThis run (MUTEX Implemented) took: %.6f\n\n", cpu_time);

   for (int i = 0; i < NUM_THREADS; i++) {
      pthread_mutex_destroy(&accounts[i].lock);
   }
   return 0;
}

