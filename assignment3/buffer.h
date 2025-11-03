// ============================================
// buffer.h - Shared definitions (INCOMPLETE - You must complete this!)
// ============================================
#ifndef BUFFER_H
#define BUFFER_H

// Required includes for both producer and consumer
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <time.h>

// Constants for shared memory and semaphores
#define BUFFER_SIZE 10
#define SHM_KEY 0x1234
#define SEM_MUTEX "/sem_mutex"
#define SEM_EMPTY "/sem_empty"
#define SEM_FULL "/sem_full"

typedef struct {
    int value;
    int producer_id;
} item_t;

typedef struct {
    item_t buffer[BUFFER_SIZE];
    int head;
    int tail;
    int count;
} shared_buffer_t;

#endif