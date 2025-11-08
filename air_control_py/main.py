import ctypes
import mmap
import os
import signal
import subprocess
import threading
import time

_libc = ctypes.CDLL(None, use_errno=True)

TOTAL_TAKEOFFS = 20
STRIPS = 5
shm_data = []

runway1_lock = threading.Lock()
runway2_lock = threading.Lock()
state_lock   = threading.Lock()

SH_MEMORY_NAME = "/Shared_Memory_Segment"
# TODO1: Size of shared memory for 3 integers (current process pid, radio, ground) use ctypes.sizeof()
# SHM_LENGTH=
SHM_LENGTH = ctypes.sizeof(ctypes.c_int)*3 #sets size to 3 int


# Global variables and locks
planes = 0  # planes waiting
takeoffs = 0  # local takeoffs (per thread)
total_takeoffs = 0  # total takeoffs
shm_data = None # will hold the memoryview that maps the shared memory



def create_shared_memory():
    """Create shared memory segment for PID exchange"""
    # TODO 6:
    # 1. Encode (utf-8) the shared memory name to use with shm_open
    # 2. Temporarily adjust the permission mask (umask) so the memory can be created with appropriate permissions
    # 3. Use _libc.shm_open to create the shared memory
    # 4. Use _libc.ftruncate to set the size of the shared memory (SHM_LENGTH)
    # 5. Restore the original permission mask (umask)
    # 6. Use mmap to map the shared memory
    # 7. Create an integer-array view (use memoryview()) to access the shared memory
    # 8. Return the file descriptor (shm_open), mmap object and memory view
    shm_name = SH_MEMORY_NAME.encode("utf-8")
    prev_umask = os.umask(0)
    fd = _libc.shm_open(shm_name,os.O_CREAT|os.O_RDWR,0o666)
    os.umask(prev_umask)
    if fd < 0:
        err = ctypes.get_errno()
        raise OSError(err, "shm_open failed")
    
    _libc.ftruncate(fd,SHM_LENGTH)
    memory = mmap.mmap(
        fd,             
        SHM_LENGTH, 
        flags=mmap.MAP_SHARED,
        prot=mmap.PROT_READ | mmap.PROT_WRITE
    )
    data = memoryview(memory).cast("i")

    return fd, memory, data



def HandleUSR2(signum, frame):
    """Handle external signal indicating arrival of 5 new planes.
    Complete function to update waiting planes"""
    global planes
    # TODO 4: increment the global variable planes
    planes +=5
    pass


def TakeOffFunction(agent_id: int):
    """Function executed by each THREAD to control takeoffs.
    Complete using runway1_lock and runway2_lock and state_lock to synchronize"""
    global planes, takeoffs, total_takeoffs, shm_data

    # TODO: implement the logic to control a takeoff thread
    # Use a loop that runs while total_takeoffs < TOTAL_TAKEOFFS
    # Use runway1_lock or runway2_lock to simulate runway being locked
    # Use state_lock for safe access to shared variables (planes, takeoffs, total_takeoffs)
    # Simulate the time a takeoff takes with sleep(1)
    # Send SIGUSR1 every 5 local takeoffs
    # Send SIGTERM when the total takeoffs target is reached
    while total_takeoffs < TOTAL_TAKEOFFS:
         # Try RUNWAY 1 path
        if runway1_lock.acquire(blocking=False):
            try:
                # Try to lock state and check if there's a plane
                if state_lock.acquire(blocking=False):
                    try:
                        if planes > 0:
                            # plane departs
                            planes -= 1
                            takeoffs += 1
                            total_takeoffs += 1

                            # If we've done 5 takeoffs in this batch, notify radio
                            if takeoffs == 5:
                                if shm_data[1] is not None:
                                    os.kill(shm_data[1], signal.SIGUSR1)
                                takeoffs = 0  # reset batch

                            # Simulate takeoff time
                            time.sleep(1)

                            # If we've hit the mission total, tell radio to shut down and exit
                            if total_takeoffs >= TOTAL_TAKEOFFS:
                                if shm_data[1] is not None:
                                    os.kill(shm_data[1], signal.SIGTERM)
                                return

                        # else: no planes, just fall through
                    finally:
                        state_lock.release()
                else:
                    # couldn't lock state_lock right now
                    pass
            finally:
                runway1_lock.release()

        elif runway2_lock.acquire(blocking=False):
            try:
                if state_lock.acquire(blocking=False):
                    try:
                        if planes > 0:
                            planes -= 1
                            takeoffs += 1
                            total_takeoffs += 1

                            if takeoffs == 5:
                                if shm_data[1] is not None:
                                    os.kill(shm_data[1], signal.SIGUSR1)
                                takeoffs = 0

                            time.sleep(1)

                            if total_takeoffs >= TOTAL_TAKEOFFS:
                                if shm_data[1] is not None:
                                    os.kill(shm_data[1], signal.SIGTERM)
                                return
                    finally:
                        state_lock.release()
                else:
                    pass
            finally:
                runway2_lock.release()
        else:
            time.sleep(0.001)
    return
    pass


def launch_radio():
    """unblock the SIGUSR2 signal so the child receives it"""
    def _unblock_sigusr2():
        signal.pthread_sigmask(signal.SIG_UNBLOCK, {signal.SIGUSR2})

    # TODO 8: Launch the external 'radio' process using subprocess.Popen()
    process = subprocess.Popen(['./radio',SH_MEMORY_NAME],preexec_fn=_unblock_sigusr2)
    return process


def main():
    global shm_data

    # TODO 2: set the handler for the SIGUSR2 signal to HandleUSR2
    signal.signal(signal.SIGUSR2,HandleUSR2)
    # TODO 5: Create the shared memory and store the current process PID using create_shared_memory()
    fd, memory, data =  create_shared_memory()
    shm_data = data
    shm_data[0] = os.getpid()
    shm_data[1] = 0  # placeholder for radio pid
    shm_data[2] = 0  # could be ground or other process
    
    # TODO 7: Run radio and store its PID in shared memory, use the launch_radio function
    radio_process = launch_radio()
    shm_data[1] = radio_process.pid
    # TODO 9: Create and start takeoff controller threads (STRIPS) 
    control_threads= []
    for i in range(STRIPS):
        thr = threading.Thread(TakeOffFunction,(i,))
        thr.start()
        control_threads.append(thr)

    # TODO 10: Wait for all threads to finish their work
    for thr in control_threads:
            thr.join()

    # TODO 11: Release shared memory and close resources
    data.release()
    memory.close()
    os.close(fd)
    



if __name__ == "__main__":
    main()