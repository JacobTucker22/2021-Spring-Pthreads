/*
 * sumsq.c
 *
 * CS 446.646 Project 1 (Pthreads)
 *
 * Compile with --std=c99
 */

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>


// aggregate variables
// shared data should be guarded
//protected by varLock
long sum = 0;
long odd = 0;
long min = INT_MAX;
long max = INT_MIN;
//used as conditions for main and workers to communicate
//protected by 'scheduleLock'
int numOfTasks = 0;
int numOfWorkers = 0;
bool done = false;

//cond mutex makes workers wait for tasks and blocks queue
pthread_cond_t isReady = PTHREAD_COND_INITIALIZER;
//schedule mutex protects num of tasks, workers, and done condition
pthread_mutex_t scheduleLock = PTHREAD_MUTEX_INITIALIZER;  
//mutex for guarding aggregate variables
pthread_mutex_t varLock = PTHREAD_MUTEX_INITIALIZER;


//Task queue implemented as linked list.
//addTask and pullNextTask work as FIFO.
//that is addTask adds to the end of the list and pullNextTask pulls from the front
//both access critical queue and should be blocked if queue is being accessed

//Basic node of task queue. Holds number to process and pointer to next node
struct Task {
  long numToProcess;
  struct Task* next;
  };
  
  struct Task* head = NULL;
  struct Task* current = NULL;
  
//adds a task to the queue with value numToProcess (value in the task)
//The queue is critical and should be guarded
//dynamically allocates memory for new node
void addTask(long numToProcess) {
  if(head == NULL) {
    head = (struct Task*) malloc(sizeof(struct Task));
    head->numToProcess = numToProcess;
    head->next = NULL;
    }
  else {
    current = head;
    while(current->next != NULL){
      current = current->next;
      }
    current->next = (struct Task*) malloc(sizeof(struct Task));
    current = current->next;
    current->numToProcess = numToProcess;
    }
}

//removes the current head node (task) of the queue
//returns the value of numToProcess (value in that task) 
long pullNextTask() {
  long ret = 0;
  if(head != NULL) {
    ret = head->numToProcess;
    current = head;
    head = head->next;
    free(current);
    current = head;
  }
  return ret;
}


// function prototypes

// worker thread function
void* Startup(void *arg);

//task process function
void CalculateSquare(long number);

//Startup is entry point for worker threads.
//Workers will inc number of workers and wait for master to signal isReady
//Then they will pull the next task, dec number of tasks and process data
//When number of tasks is 0 and main has signalled done, they dec number of workers and exit
//All modifications to shared data protected by locks
void* Startup(void *arg) {
  long numToSquare = 0;
  
  //hold scheduleLock before checking for task
  pthread_mutex_lock(&scheduleLock);
  numOfWorkers++;
  
  //keep looping through work loop while done is false
  while (!done) {
    //wait for main to signal task ready
    while (numOfTasks <= 0 && !done) {
      //Workers start by waiting for master to signal ready for worker  
      pthread_cond_wait(&isReady, &scheduleLock);
      }      
    //if task is ready hold scheduleLock and pull task from queue and dec number of tasks
    if(!done) {
      numToSquare = pullNextTask(); 
      numOfTasks--;
      pthread_mutex_unlock(&scheduleLock);    //release scheduleLock and process task
      CalculateSquare(numToSquare);
      pthread_mutex_lock(&scheduleLock);      //grab scheduleLock again for cond var.
      }    
    } 
  //when done release scheduleLock, dec number of workers and exit
  numOfWorkers--;
  pthread_mutex_unlock(&scheduleLock);
  pthread_exit(NULL);
}

/*
 * update global aggregate variables given a number
 */
void CalculateSquare(long number)
{
  // calculate the square
  long the_square = number * number;

  // ok that was not so hard, but let's pretend it was
  // simulate how hard it is to square this number!
  sleep(number);

  //get varLock for aggregate variables
  pthread_mutex_lock(&varLock);

  // let's add this to our (global) sum
  sum += the_square;

  // now we also tabulate some (meaningless) statistics
  if (number % 2 == 1) {
    // how many of our numbers were odd?
    odd++;
  }

  // what was the smallest one we had to deal with?
  if (number < min) {
    min = number;
  }

  // and what was the biggest one?
  if (number > max) {
    max = number;
  }
  
  //unlock aggregate variable varLock
  pthread_mutex_unlock(&varLock);
}

//Master main thread
int main(int argc, char* argv[])
{
  // check and parse command line options
  if (argc != 3) {
    printf("Usage: sumsq <infile> <Number of Threads\n");
    exit(EXIT_FAILURE);
  }
  //file pointer init
  char *fn = argv[1];
  //number of threads init
  int numThd = atoi(argv[2]);
  
  //pthreads init
  pthread_t* threads;
  //allocate array of pthreads size of argument passed
  threads = (pthread_t *) malloc (numThd*sizeof(pthread_t));
  //initialuze pthread attributes (default, could also just use NULL)
  pthread_attr_t attr;
  pthread_attr_init(&attr);
   
  // initialize variables for file transfer
  FILE* fin = fopen(fn, "r");
  char action;
  long num;
  
  //create correct number of threads
  for (int i = 0; i < numThd; i++) {
    if(pthread_create(&threads[i], &attr, Startup, NULL) != 0){
      printf("failed to create pthread %d", i);
      exit(EXIT_FAILURE);
      }      
    }

  //load numbers and create tasks
  while (fscanf(fin, "%c %ld\n", &action, &num) == 2) {
    if (action == 'p') {            // process, do some work
      //queue and shared data protected by scheduleLock
      pthread_mutex_lock(&scheduleLock);
      addTask(num);
      numOfTasks++;
      pthread_mutex_unlock(&scheduleLock);
      //Signal worker to wake up and pull task
      pthread_cond_signal(&isReady);
    } else if (action == 'w') {     // wait, nothing new happening
      sleep(num);
    } else {
      printf("ERROR: Unrecognized action: '%c'\n", action);
      exit(EXIT_FAILURE);
    }
  }
  fclose(fin);
  
  //wait for workers to finish pulling tasks
  //scheduleLock protects num of tasks and done
  pthread_mutex_lock(&scheduleLock);
  while (numOfTasks > 0) {
    done = false;
    pthread_mutex_unlock(&scheduleLock);  //unlock so workers can update
    pthread_mutex_lock(&scheduleLock);    //relock before checking again
    }
  //signal workers that there are no more tasks coming
  done = true;
  pthread_mutex_unlock(&scheduleLock);
  //workers that were never assigned a task are still sleeping
  //Main will continue to signal this condition until all workers are awake so they can terminate
  pthread_mutex_lock(&scheduleLock);   //grab scheduleLock to check num of workers
  while(numOfWorkers > 0) {
    pthread_cond_signal(&isReady);  //signal workers to wake up
    pthread_mutex_unlock(&scheduleLock);  //unlock so workers can update
    pthread_mutex_lock(&scheduleLock);    //relock before checking again
    }
  pthread_mutex_unlock(&scheduleLock);  //unlock scheduleLock for num of workers
  
  //join all threads
  for(int i = 0; i < numThd; i++) {
    if(pthread_join(threads[i], NULL) != 0) {
      printf("failed to join pthread %d/n", i);
      exit(EXIT_FAILURE);
      }
    }
 
  // print results
  printf("%ld %ld %ld %ld\n", sum, odd, min, max);
  
  // clean up and return
  return (EXIT_SUCCESS);
}

