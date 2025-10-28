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
#define SH_MEMORY_NAME "Shared_Memory_Segment"
#define SHM_SIZE sizeof(int) * 3

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
    ftruncate(fd, SHM_SIZE);
    int *shm_ptr = mmap(0, SHM_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    shm_ptr[1] = getpid();

    execl("radio", "radio", SH_MEMORY_NAME, NULL);
    return EXIT_SUCCESS;
  }

  // TODO 6: Launch 5 threads which will be the controllers; each thread will
  // execute the TakeOffsFunction().

  pthread_t threads[5];

  for (int i = 0; i < 5; i++) {
    pthread_create(&threads[i], NULL, TakeOffsFunction(), NULL);
  }

  for (int i = 0; i < 5; i++) {
    pthread_join(threads[i], NULL);
  }
  kill(radioPid, SIGTERM);
  shm_unlink(SH_MEMORY_NAME);

  return EXIT_SUCCESS;
}