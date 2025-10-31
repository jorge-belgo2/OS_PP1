#ifndef FUNCTIONS_H
#define FUNCTIONS_H
#define SH_MEMORY_NAME "Shared_Memory_Segment"
#define SHM_SIZE sizeof(int) * 3
#include <pthread.h>

extern int planes;
extern int takeoffs;
extern int total_takeoffs;
extern int fd;

extern pthread_t threads[5];
extern int *shm_ptr;

extern pthread_mutex_t state_lock, runway1_lock, runway2_lock;
void MemoryCreate();
void SigHandler2(int signal);
void* TakeOffsFunction();

#endif