#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "uthread.h"

typedef int broken_spinlock_t;
broken_spinlock_t bs_lock;

void broken_spinlock_create (broken_spinlock_t* lock) {
  *lock=0;
}

void broken_spinlock_lock (broken_spinlock_t* lock) {
  while (*lock) {}
  *lock=1;
}

void broken_spinlock_unlock (broken_spinlock_t* lock) {
  *lock=0;
}

struct SE {
  struct SE* next;
};

struct SE *top=0;

void       (*push) (struct SE* e);
struct SE* (*pop)  ();
spinlock_t lock;

void push_st (struct SE* e) {
  e->next = top;
  top     = e;
}

struct SE* pop_st () {
  struct SE* e = top;
  top = (top)? top->next: 0;
  return e;
}

void push_bs (struct SE* e) {
  broken_spinlock_lock (&bs_lock);
  push_st (e);
  broken_spinlock_unlock (&bs_lock);
}

struct SE* pop_bs () {
  struct SE* e;
  broken_spinlock_lock (&bs_lock);
  e = pop_st ();
  broken_spinlock_unlock (&bs_lock);
  
  return e;
}

void push_cs (struct SE* e) {
  spinlock_lock (&lock);
  push_st (e);
  spinlock_unlock (&lock);
}

struct SE* pop_cs () {
  struct SE* e;
  spinlock_lock (&lock);
  e = pop_st ();
  spinlock_unlock (&lock);
  
  return e;
}

void push_driver (long int n) {
  struct SE* e;
  while (n--) 
    push ((struct SE*) malloc (sizeof (struct SE)));
}

void pop_driver (long int n) {
  struct SE* e;
  while (n--) {
    do {
      e = pop ();
    } while (!e);
    free (e);
  }
}

void single_threaded (long int n) {
  push = push_st;
  pop  = pop_st;
  push_driver (n);
  pop_driver (n);
  assert (top==0);
}

void multi_threaded_race (long int n) {
  uthread_t *et, *dt;
  
  uthread_init (2);
  push = push_st;
  pop  = pop_st;
  et = uthread_create ((void* (*)(void*)) push_driver, (void*) n);
  dt = uthread_create ((void* (*)(void*)) pop_driver, (void*) n);
  uthread_join (et);
  uthread_join (dt);
  assert (top==0);
}

void multi_threaded_broken_spinlock (long int n) {
  uthread_t *et, *dt;
  
  uthread_init (2);
  broken_spinlock_create (&bs_lock);
  push = push_bs;
  pop  = pop_bs;
  et = uthread_create ((void* (*)(void*)) push_driver, (void*) n);
  dt = uthread_create ((void* (*)(void*)) pop_driver,  (void*) n);
  uthread_join (et);
  uthread_join (dt);
  assert (top==0);
}

void multi_threaded_synchronized (long int n) {
  uthread_t *et, *dt;
  
  uthread_init (2);
  spinlock_create (&lock);
  push = push_cs;
  pop  = pop_cs;
  et = uthread_create ((void* (*)(void*)) push_driver, (void*) n);
  dt = uthread_create ((void* (*)(void*)) pop_driver,  (void*) n);
  uthread_join (et);
  uthread_join (dt);
  assert (top==0);
}

int main (int argc, char** argv) {
  static char usage[] = "usage: racedemo t (0=single,1=race,2=badspin,3=sync) n";
  if (argc != 3) {
    printf ("%s\n",usage);
    return -1;
  }
  int n = atoi (argv[2]);
  switch (atoi (argv[1])) {
    case 0:
      single_threaded (n);
      break;
    case 1:
      multi_threaded_race (n);
      break;
    case 2:
      multi_threaded_broken_spinlock (n);
      break;
    case 3:
      multi_threaded_synchronized (n);
      break;
    default:
      printf ("%s\n",usage);
      return -1;
  }
}