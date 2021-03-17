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
bool done = false;

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
    head->next = (struct Task*) malloc(sizeof(struct Task));
    }
  else {
    current = head;
    while(current->next != NULL){
      current = current->next;
      }
    current->next = (struct Task*) malloc(sizeof(struct Task));
    current->tnum = tnum;
    }
}

//condition and mutex init
pthread_cond_t pAvail = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;  

// function prototypes
void* startup(void *arg);

void calculate_square(long number);

void* startup(void *arg) {
  long *snum = (long*)arg;

  while (!done) {
    pthread_mutex_lock(&lock);
    pthread_cond_wait(&pAvail, &lock);
    if(!done) {
    calculate_square(*snum);
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
  
    // load numbers and add them to the queue
  FILE* fin = fopen(fn, "r");
  char action;
  long num;
  
  
  pthread_create(&tid, &attr, startup, &num);  


  while (fscanf(fin, "%c %ld\n", &action, &num) == 2) {
    if (action == 'p') {            // process, do some work
      addTask(num);
    } else if (action == 'w') {     // wait, nothing new happening
      sleep(num);
    } else {
      printf("ERROR: Unrecognized action: '%c'\n", action);
      exit(EXIT_FAILURE);
    }
  }
  fclose(fin);
  
  //manage workers
  
  
  
  
  
  done = true;
  
  // print results
  printf("%ld %ld %ld %ld\n", sum, odd, min, max);
  
  // clean up and return
  return (EXIT_SUCCESS);
}

