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

// TODO: Define the item structure (item_t)
// Each item should contain:
//   - The data value
//   - Which producer created it
//   - Any other fields you think are necessary

// TODO: Define the shared buffer structure (shared_buffer_t)
// The buffer should contain:
//   - An array of items
//   - Variables to track the buffer state
//   - Any other fields needed for synchronization

#endif