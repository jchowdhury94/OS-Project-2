#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define SIZE 2             // Size of the bounded buffer
#define PROCESS_COUNT 6    // Total number of simulated processes

// Struct to hold process ID and its burst time
typedef struct {
    int pid;
    int burst;
} ProcessInfo;

// Array of processes loaded from file
ProcessInfo processes[PROCESS_COUNT];

// Shared buffer and helper indexes for producer-consumer logic
int buffer[SIZE];
int in = 0, out = 0; // in = write index, out = read index

// Synchronization primitives
pthread_mutex_t lock;  // To protect the critical section (buffer access)
sem_t empty;           // Counts available empty slots in buffer
sem_t full;            // Counts number of full slots in buffer

// STEP 1: Simulate execution of each process
void* run_process(void* arg) {
    ProcessInfo* p = (ProcessInfo*) arg;
    printf("[Process %d] Started. Burst time: %d\n", p->pid, p->burst);
    sleep(p->burst); // Simulate CPU burst using sleep
    printf("[Process %d] Finished.\n", p->pid);
    return NULL;
}

// STEP 2: Producer thread function
void* producer(void* arg) {
    for (int i = 0; i < PROCESS_COUNT; i++) {
        int item = processes[i].pid;

        // Wait for an empty slot in the buffer
        printf("[Producer] Waiting for empty slot...\n");
        sem_wait(&empty);
        printf("[Producer] Got empty slot.\n");

        // Lock the buffer before writing
        printf("[Producer] Waiting for lock...\n");
        pthread_mutex_lock(&lock);
        printf("[Producer] Got lock.\n");

        // Add item to buffer at 'in' position
        buffer[in] = item;
        printf("[Producer] Produced: %d at %d\n", item, in);
        in = (in + 1) % SIZE; // Circular buffer logic

        // Unlock the buffer and signal that a new item is available
        pthread_mutex_unlock(&lock);
        printf("[Producer] Released lock.\n");

        sem_post(&full); // Signal that buffer has a full slot
        sleep(1);        // Delay for readability
    }

    printf("[Producer] Finished.\n");
    return NULL;
}

// STEP 2: Consumer thread function
void* consumer(void* arg) {
    int item;
    for (int i = 0; i < PROCESS_COUNT; i++) {
        // Wait for at least one full slot
        printf("[Consumer] Waiting for item...\n");
        sem_wait(&full);
        printf("[Consumer] Got item.\n");

        // Lock the buffer before reading
        printf("[Consumer] Waiting for lock...\n");
        pthread_mutex_lock(&lock);
        printf("[Consumer] Got lock.\n");

        // Read item from buffer at 'out' position
        item = buffer[out];
        printf("[Consumer] Consumed: %d at %d\n", item, out);
        out = (out + 1) % SIZE; // Circular buffer logic

        // Unlock the buffer and signal an empty slot is now available
        pthread_mutex_unlock(&lock);
        printf("[Consumer] Released lock.\n");

        sem_post(&empty); // Signal that buffer has an empty slot
        sleep(2);         // Delay for readability
    }

    printf("[Consumer] Finished.\n");
    return NULL;
}

int main() {
    pthread_t process_threads[PROCESS_COUNT];
    pthread_t prod, cons;

    // STEP 1: Load process data from file
    FILE* file = fopen("processes.txt", "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    char line[100];
    fgets(line, sizeof(line), file); // Skip header line

    // Read each process from the file
    for (int i = 0; i < PROCESS_COUNT; i++) {
        int pid, arrival, burst, priority;
        fgets(line, sizeof(line), file);
        sscanf(line, "%d %d %d %d", &pid, &arrival, &burst, &priority);
        processes[i].pid = pid;
        processes[i].burst = burst;
    }

    fclose(file);

    // STEP 1: Create threads for each process
    for (int i = 0; i < PROCESS_COUNT; i++) {
        pthread_create(&process_threads[i], NULL, run_process, &processes[i]);
    }

    // Wait for all process threads to finish
    for (int i = 0; i < PROCESS_COUNT; i++) {
        pthread_join(process_threads[i], NULL);
    }

    // STEP 2: Initialize mutex and semaphores
    pthread_mutex_init(&lock, NULL);
    sem_init(&empty, 0, SIZE); // Initially, all slots are empty
    sem_init(&full, 0, 0);     // Initially, no items in buffer

    // STEP 2: Create producer and consumer threads
    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);

    // Wait for producer and consumer to finish
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    return 0;
}
