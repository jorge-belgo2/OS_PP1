#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "functions.h"

// ✅ Define them here (ONE definition)
int planes = 0;
int takeoffs = 0;
int total_takeoffs = 20;

pthread_mutex_t state_lock;
pthread_mutex_t runway1_lock;
pthread_mutex_t runway2_lock;


void MemoryCreate() {
    shm_unlink(SH_MEMORY_NAME);
    int fd = shm_open(SH_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, SHM_SIZE) == -1) {
        perror("ftruncate failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    int *shm_ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    shm_ptr[0] = getpid(); // ✅ safe now
    close(fd);

    pthread_mutex_init(&state_lock, NULL);
    pthread_mutex_init(&runway1_lock, NULL);
    pthread_mutex_init(&runway2_lock, NULL);
}


void SigHandler2(int signal) { planes += 5; }

void *TakeOffsFunction() {
  // TODO: implement the logic to control a takeoff thread
  //    Use a loop that runs while total_takeoffs < TOTAL_TAKEOFFS
  //    Use runway1_lock or runway2_lock to simulate a locked runway
  //    Use state_lock for safe access to shared variables (planes,
  //    takeoffs, total_takeoffs)
  //    Simulate the time a takeoff takes with sleep(1)
  //    Send SIGUSR1 every 5 local takeoffs
  //    Send SIGTERM when the total takeoffs target is reached

  while (takeoffs < total_takeoffs) {
    if (pthread_mutex_trylock(&runway1_lock) == 0 && pthread_mutex_trylock(&state_lock) == 0 && planes > 0) {

      
      planes -= 1;
      takeoffs++;
      total_takeoffs++;
      pthread_mutex_unlock(&state_lock);
      sleep(1);
      pthread_mutex_unlock(&runway1_lock);

    } else if (pthread_mutex_trylock(&runway2_lock) == 0 && pthread_mutex_trylock(&state_lock) == 0 && planes > 0) {

      pthread_mutex_lock(&state_lock);
      planes -= 1;
      takeoffs++;
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