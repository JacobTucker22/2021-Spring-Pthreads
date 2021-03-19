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
long sum = 0;
long odd = 0;
long min = INT_MAX;
long max = INT_MIN;
int num_of_tasks = 0;
bool done = false;

//condition and mutex init
pthread_cond_t pAvail = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;  


//Task queue implemented as linked list.
//addTask and pullNextTask work as FIFO.
//that is addTask adds to the end of the list and pullNextTask pulls from the front
//both access critical queue and should be blocked if queue is being accessed
struct Task {
  long tnum;
  struct Task* next;
  };
  
  struct Task* head = NULL;
  struct Task* current = NULL;
  
  //adds a task to the queue with value tnum (value in the task)
  //The queue is critical and should be guarded
void addTask(long tnum) {
  if(head == NULL) {
    head = (struct Task*) malloc(sizeof(struct Task));
    head->tnum = tnum;
    head->next = NULL;
    }
  else {
    current = head;
    while(current->next != NULL){
      current = current->next;
      }
    current->next = (struct Task*) malloc(sizeof(struct Task));
    current = current->next;
    current->tnum = tnum;
    }
}

//removes the current head node (task) of the queue
//returns the value of tnum (value in that task) 
long pullNextTask() {
  long ret = 0;
  if(head != NULL) {
    ret = head->tnum;
    current = head;
    head = head->next;
    free(current);
    current = head;
  }
  return ret;
}


// function prototypes
// main thread function
void* startup(void *arg);

//task process function
void calculate_square(long number);

void* startup(void *arg) {
  long snum = 0;
  
  //hold lock before checking for task
  pthread_mutex_lock(&lock);
  
  //keep looping through work loop while done is false
  while (!done) {
    //wait for main to signal task ready
    while (num_of_tasks <= 0) {   
      pthread_cond_wait(&pAvail, &lock);
      }      
    //if task is ready hold lock and pull task from queue
    snum = pullNextTask(); 
    num_of_tasks--;
    pthread_mutex_unlock(&lock);    //release lock and process task
    calculate_square(snum);    
    } 
  //when done release lock and exit
  pthread_mutex_unlock(&lock);
  pthread_exit(NULL);
}

/*
 * update global aggregate variables given a number
 */
void calculate_square(long number)
{

  // calculate the square
  long the_square = number * number;

  // ok that was not so hard, but let's pretend it was
  // simulate how hard it is to square this number!
  sleep(number);

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
}


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
  printf("# of threads %d\n", numThd);   //double check, remove before final
  
  //pthreads init
  pthread_t* threads;
  //allocate array of pthreads size of argument passed
  threads = (pthread_t *) malloc (numThd*sizeof(pthread_t));
  //initialuze pthread attributes (default, could also just use NULL)
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  
 
  
    // load numbers and add them to the queue
  FILE* fin = fopen(fn, "r");
  char action;
  long num;
  
  for (int i = 0; i < numThd; i++) {
    if(pthread_create(&threads[i], &attr, startup, NULL) != 0){
      printf("failed to create pthread %d", i);
      exit(EXIT_FAILURE);
      }      
    }


  while (fscanf(fin, "%c %ld\n", &action, &num) == 2) {
    if (action == 'p') {            // process, do some work
      pthread_mutex_lock(&lock);
      addTask(num);
      num_of_tasks++;
      pthread_mutex_unlock(&lock);
      pthread_cond_signal(&pAvail);
    } else if (action == 'w') {     // wait, nothing new happening
      sleep(num);
    } else {
      printf("ERROR: Unrecognized action: '%c'\n", action);
      exit(EXIT_FAILURE);
    }
  }
  fclose(fin);
  

  //wait for workers to finish
  while (num_of_tasks > 0) {
    ;
    }
  //signal workers that all tasks are finished
  done = true;
  pthread_cond_broadcast(&pAvail);
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

