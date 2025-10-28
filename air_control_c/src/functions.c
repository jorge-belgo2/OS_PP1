#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#define SH_MEMORY_NAME "Shared_Memory_Segment"
#define SHM_SIZE sizeof(int)*3
#define TOTAL_TAKEOFFS 10  //THIS IS NOT THE CORRECT VALUE ITS JUST TEMPORARY

int planes = 0;
int takeoffs = 0;
int total_takeoffs = 0;

pthread_mutex_t state_lock, runway1_lock, runway2_lock;

void MemoryCreate() {
  // TODO2: create the shared memory segment, configure it and store the PID of
  // the process in the first position of the memory block.
  shm_unlink(SH_MEMORY_NAME);
  int fd = shm_open(SH_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
  ftruncate(fd, SHM_SIZE);
  int *shm_ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  shm_ptr[0] = getpid();


  pthread_mutex_init(&state_lock, NULL);
  pthread_mutex_init(&runway1_lock, NULL);
  pthread_mutex_init(&runway2_lock, NULL);
}

void SigHandler2(int signal) {
  planes += 5;
}

void* TakeOffsFunction() {
  // TODO: implement the logic to control a takeoff thread
  //    Use a loop that runs while total_takeoffs < TOTAL_TAKEOFFS
  //    Use runway1_lock or runway2_lock to simulate a locked runway
  //    Use state_lock for safe access to shared variables (planes,
  //    takeoffs, total_takeoffs)
  //    Simulate the time a takeoff takes with sleep(1)
  //    Send SIGUSR1 every 5 local takeoffs
  //    Send SIGTERM when the total takeoffs target is reached

  while (total_takeoffs < TOTAL_TAKEOFFS) {
    if (pthread_mutex_trylock(&runway1_lock) == 1 && planes != 0) {
      
      pthread_mutex_lock(&state_lock);
      planes -= 1;
      takeoffs ++;
      total_takeoffs++;
      pthread_mutex_unlock(&state_lock);
      sleep(1);
      pthread_mutex_unlock(&runway1_lock);

    } else if (pthread_mutex_trylock(&runway2_lock) == 1 && planes != 0) {

      pthread_mutex_lock(&state_lock);
      planes -= 1;
      takeoffs ++;
      total_takeoffs++;
      pthread_mutex_unlock(&state_lock);
      sleep(1);
      pthread_mutex_unlock(&runway2_lock);

    } else {
      usleep(1000);
    }


  }
  
  return EXIT_SUCCESS;
}