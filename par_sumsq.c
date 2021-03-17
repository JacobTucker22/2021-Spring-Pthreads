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
#include <semaphore.h>

// aggregate variables
long sum = 0;
long odd = 0;
long min = INT_MAX;
long max = INT_MIN;
bool done = false;
int availThreads = 0;


//Task queue implemented as linked list.
struct Task {
  long tnum;
  struct Task* next;
  };
  
  struct Task* head = NULL;
  struct Task* current = NULL;
  
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

//condition and mutex init
pthread_cond_t pAvail = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;  


// function prototypes
void* startup(void *arg);

void calculate_square(long number);

void* startup(void *arg) {
  long snum = 0;

  while (!done) {
    pthread_mutex_lock(&lock);
    availThreads++;
    pthread_cond_wait(&pAvail, &lock);
    //FIXME race condition. sometime main is able to print all numbers before worker finishes tasks
    //Maybe use another condition based on this availablie thread
    availThreads--;
    if(!done) {
    snum = pullNextTask();
    calculate_square(snum);
    }
    pthread_mutex_unlock(&lock);
  }
  
  return NULL;
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
  char *fn = argv[1];
  int numThd = atoi(argv[2]);
  printf("# of threads %d\n", numThd);
  
  //pthreads init
  pthread_t tid;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  //sem init
 
  
    // load numbers and add them to the queue
  FILE* fin = fopen(fn, "r");
  char action;
  long num;
  
  
  pthread_create(&tid, &attr, startup, NULL);  


  while (fscanf(fin, "%c %ld\n", &action, &num) == 2) {
    if (action == 'p') {            // process, do some work
      addTask(num);
      while(availThreads <= 0) {
        ;
        }
      pthread_cond_signal(&pAvail);
    } else if (action == 'w') {     // wait, nothing new happening
      sleep(num);
    } else {
      printf("ERROR: Unrecognized action: '%c'\n", action);
      exit(EXIT_FAILURE);
    }
  }
  fclose(fin);
  

  
  //finish work and signal pthreads to terminate
  done = true;
  pthread_cond_signal(&pAvail);
  pthread_join(tid, NULL);
  
  // print results
  printf("%ld %ld %ld %ld\n", sum, odd, min, max);
  
  // clean up and return
  return (EXIT_SUCCESS);
}

