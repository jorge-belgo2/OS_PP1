#define _POSIX_C_SOURCE 200809L
#include "functions.h"
#include <ctype.h>
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

int main() {
  // TODO 1: Call the function that creates the shared memory segment.
  MemoryCreate();

  // TODO 3: Configure the SIGUSR2 signal to increment the planes on the runway
  // by 5.
  struct sigaction sa;
  sa.sa_handler = SigHandler2;
  sigaction(SIGUSR2, &sa, NULL);

  // TODO 4: Launch the 'radio' executable and, once launched, store its PID in
  // the second position of the shared memory block.
  pid_t radioPid;
  radioPid = fork();

  if (radioPid == 0) {
    int fd = shm_open(SH_MEMORY_NAME, O_RDWR, 0666);
    if (fd == -1) {
      perror("shm_open failed");
      exit(EXIT_FAILURE);
    }

    int *shm_ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    shm_ptr[1] = getpid();

    execl("./radio", "radio", SH_MEMORY_NAME, NULL);
    perror("execl failed");
    exit(EXIT_FAILURE);
  } else {

    // TODO 6: Launch 5 threads which will be the controllers; each thread will
    // execute the TakeOffsFunction()

    pthread_t threads[5];

    for (int i = 0; i < 5; i++) {
      pthread_create(&threads[i], NULL, TakeOffsFunction, NULL);
    }

    for (int i = 0; i < 5; i++) {
      pthread_join(threads[i], NULL);
    }
    // pthread_mutex_destroy(&state_lock);
    // pthread_mutex_destroy(&runway1_lock);
    // pthread_mutex_destroy(&runway2_lock);
    shm_unlink(SH_MEMORY_NAME);
  }

  return EXIT_SUCCESS;
}