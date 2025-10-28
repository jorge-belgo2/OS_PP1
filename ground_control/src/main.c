#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define SHM_SIZE sizeof(int) * 3
#define PLANES_LIMIT 20
int planes = 0;
int takeoffs = 0;
int traffic = 0;
int fd = 0;
int *shm_ptr;

void Traffic(int signum) {
  // TODO:
  // Calculate the number of waiting planes.
  // Check if there are 10 or more waiting planes to send a signal and increment
  // planes. Ensure signals are sent and planes are incremented only if the
  // total number of planes has not been exceeded.
  if (planes >= 10) {
    printf("RUNWAY OVERLOADED\n");
  } else if (planes < PLANES_LIMIT) {
    planes += 5;
    kill(shm_ptr[1], SIGUSR2);
  }
}

void signal_handler(int signal) {
  if (signal == SIGTERM) {
    if (fd != 0) {
      munmap(shm_ptr, SHM_SIZE);  // Unmap shared memory first
      close(fd);
      printf("finalization of operations...\n");
      exit(0);  // Exit cleanly instead of using SIGKILL
    }
  } else if (signal == SIGUSR1) {
    takeoffs += 5;
  } else {
    printf("Neither sigterm nor sigusr1 was sent\n");
  }
}

void timer_handler(int signal) { Traffic(signal); }

int main(int argc, char *argv[]) {
  // TODO:
  // 1. Open the shared memory block and store this process PID in position 2
  //    of the memory block.
  int fd = shm_open(argv[1], O_RDWR, 0666);
  if (fd == -1) {
    perror("shm_open failed");
    return 1;
  }
  
  if (ftruncate(fd, SHM_SIZE) == -1) {
    perror("ftruncate failed");
    close(fd);
    return 1;
  }
  
  shm_ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (shm_ptr == MAP_FAILED) {
    perror("mmap failed");
    close(fd);
    return 1;
  }
  shm_ptr[2] = getpid();

  // 3. Configure SIGTERM and SIGUSR1 handlers
  //    - The SIGTERM handler should: close the shared memory, print
  //      "finalization of operations..." and terminate the program.
  //    - The SIGUSR1 handler should: increase the number of takeoffs by 5.
  struct sigaction sa;
  sa.sa_handler = signal_handler;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGUSR1, &sa, NULL);

  // 2. Configure the timer to execute the Traffic function.

  struct sigaction sa_timer;
  struct itimerval timer;

  // 1️⃣ Configure the signal handler for SIGALRM
  sa_timer.sa_handler = &timer_handler; // function to call when timer fires
  // sa_timer.sa_flags = SA_RESTART;      // restart syscalls if interrupted
  sigaction(SIGALRM, &sa_timer, NULL); // register handler

  // 2️⃣ Configure timer to trigger every 500ms
  timer.it_value.tv_sec = 0;          // first trigger delay (seconds)
  timer.it_value.tv_usec = 500000;    // first trigger delay (microseconds)
  timer.it_interval.tv_sec = 0;       // interval seconds between triggers
  timer.it_interval.tv_usec = 500000; // interval microseconds (500ms)

  setitimer(ITIMER_REAL, &timer, NULL);

  while (1) {
    pause();
  }
}