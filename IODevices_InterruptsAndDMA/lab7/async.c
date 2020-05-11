#include <stdio.h>
#include <signal.h>
#include <unistd.h>

/*
 * compile: clang async.c -o   run: async ./async
 * “((1+2)-(3+4))+7
 */
struct Comp {
  void (*handler) (void*, int);
  void* buf;
  int   siz;
};

struct Triple {
    int result;
    int arg0;
    int arg1;
    int finished;
};

struct Comp comp[1000];
int compHead = 0;
int compTail = 0;

// aHandler: completion handler, is the callback that would be executed after doAsync finished
// void pointer: a pointer which is not associated with any data types.
void doAsyncAdd (void (*aHander)(void*, int), void* aBuf, int aSiz) {
  // store the completion routine in the completion queue
  compHead = (compHead + 1) % (sizeof (comp) / sizeof (struct Comp));
  comp[compHead].handler = aHander;
  comp[compHead].buf     = aBuf;
  comp[compHead].siz     = aSiz;

  // the actual operation before the handler take place
  add(aBuf, 0);
}

//                    pointer of the func, parameters
//
void doAsyncSub (void (*aHander)(void*, int), void* aBuf, int aSiz) {
    compHead = (compHead + 1) % (sizeof (comp) / sizeof (struct Comp));
    comp[compHead].handler = aHander; // comp[compHead] is a comp struct
    comp[compHead].buf     = aBuf;
    comp[compHead].siz     = aSiz;
    sub(aBuf, 0);
}


add (struct Triple* xp, int n){
    xp->result = xp->arg0 + xp->arg1;
    xp->finished = 1;
}

sub (struct Triple* xp, int n){
    xp->result = xp->arg0 - xp->arg1;
    xp->finished = 1;
}

// deque the completion routine -> "aHander" would be executed
void interruptServiceRoutine () {
  struct Comp c;
  if (compHead != compTail) {
    compTail = (compTail + 1) % (sizeof (comp) / sizeof (struct Comp));
    c = comp[compTail];
    c.handler (c.buf, c.siz);
  }
}

void printInt (void* buf, int siz) {
    printf("%d\n", *((int*) buf)); fflush (stdout);
}

void boot () {
  signal (SIGALRM, interruptServiceRoutine); // mimic hardware interrupts
  ualarm (999999,999999);
}

int main (int argc, char** argv) {
  boot ();
  printf ("Scheduling some things to do asynchronously\n"); fflush (stdout);
  struct Triple add1;
  add1.arg0 = 1;
  add1.arg1 = 2;
  // doAsync (printString, &add1, 0); // schedules an asynchronous event, sort of like a disk-read request
  struct Triple add2;
  add2.arg0 = 3;
  add2.arg1 = 4;
  doAsyncAdd (printInt, &add1, 0);
  doAsyncAdd (printInt, &add2, 0);

  // spin lock
  while (!(add1.finished && add2.finished) ){
      ;
  }

  struct Triple sub1;
  sub1.arg0 = add1.result;
  sub1.arg1 = add2.result;
  doAsyncSub (printInt, &sub1, 0);
  while (!sub1.finished ){
      ;
  }

  struct Triple add3;
  add3.arg0 = sub1.result;
  add3.arg1 = 7;
  doAsyncAdd (printInt, &add3, 0);

  printf ("Now waiting for them to complete\n"); fflush (stdout);
  while (1);
}