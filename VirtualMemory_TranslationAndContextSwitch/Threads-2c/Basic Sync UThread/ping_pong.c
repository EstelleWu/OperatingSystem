//
// Written by Mike Feeley, University of BC, 2010
// Do not redistribute or otherwise make available any portion of this code to anyone without written permission of the author.
//

#include <stdlib.h>
#include <stdio.h>
#include "uthread.h"

uthread_semaphore_t *go_ping, *go_pong;

void* ping (void* x) {
  int i;
  for (i=0; i<10; i++) {
    uthread_V (go_pong);
    uthread_P (go_ping);
    printf ("ping %d\n",i);
  }
}

void* pong (void* x) {
  int i;
  for (i=0; i<10; i++) {
    uthread_V (go_ping);
    uthread_P (go_pong);
    printf ("pong %d\n",i);
  }
}

int main (int* argc, char** argv) {
  uthread_t *ping_thread, *pong_thread;
  int i;
  uthread_init (1);
  go_ping = uthread_semaphore_create (0);
  go_pong = uthread_semaphore_create (0);
  ping_thread = uthread_create (ping, 0);
  pong_thread = uthread_create (pong, 0);
  uthread_join (ping_thread);
  uthread_join (pong_thread);
}
