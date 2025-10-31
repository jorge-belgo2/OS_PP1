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
#define SH_MEMORY_NAME "Shared_Memory_Segment"
#define SHM_SIZE sizeof(pid_t) * 3
#define PLANES_LIMIT 20
int planes = 0;
int takeoffs = 0;
int traffic = 0;
int fd;
int *shm_ptr;
char shm_name[22];

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
    if(shm_ptr[1] > 0){
      kill(shm_ptr[1], SIGUSR2);
    }
  }
}

void signal_handler(int signal) {
  if (signal == SIGTERM) {

    munmap(shm_ptr, SHM_SIZE); // Unmap shared memory first
    close(fd);
    printf("finalization of operations...\n");
    exit(0); // Exit cleanly instead of using SIGKILL

  } else if (signal == SIGUSR1) {
    planes -= 5;
    takeoffs += 5;
  } else if(signal == SIGALRM) {
    Traffic(signal);
  }else {
    printf("Neither sigterm nor sigusr1 was sent\n");
  }
}

void timer_handler(int signal) { Traffic(signal); }

int main(int argc, char *argv[]) {
  // TODO:
  // 1. Open the shared memory block and store this process PID in position 2
  //    of the memory block.
  if (argc >= 2 && argv[1] && argv[1][0] != '\0') {
    strcpy(shm_name, SH_MEMORY_NAME);
  } else {
    strcpy(shm_name, SH_MEMORY_NAME);
  }

  fd = shm_open(shm_name, O_RDWR, 0);
  if (fd == -1) {
    perror("shm_open failed");
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
  sigaction(SIGALRM, &sa, NULL);

  // 2. Configure the timer to execute the Traffic function.

  struct itimerval timer;

  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 500000;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 500000;

  setitimer(ITIMER_REAL, &timer, NULL);

  while (1) {
    pause();
  }
}