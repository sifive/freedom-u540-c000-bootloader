/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#ifndef SIFIVE_BARRIER
#define SIFIVE_BARRIER

#include <stdatomic.h>

/********** Generic barrier **********/
// everything zero is correct initial state
typedef struct Barrier {
  _Atomic volatile int entered[2];
  _Atomic volatile int wait[2];
  _Atomic volatile int gen;
} Barrier;

// This function is declared inline b/c it is in .h file
static inline void Barrier_Wait(Barrier *bar, int numProcs) // re-write for C/C++11 atomics
{
  int gen, arrived;

  if (numProcs == 1)
    return;

  gen = bar->gen;       /* gen only changes when everyone is waiting */
  // update number of threads arrived at barrier
  arrived = atomic_fetch_add(&(bar->entered[gen]), 1) + 1;

  /* signal waiting threads if everyone is here */
  if (arrived == numProcs) {
    /* new gen only affect threads arriving for the next barrier */
    bar->gen = 1 - gen;

    atomic_fetch_add(&(bar->entered[gen]), -1);
    if (arrived > 1) {
      bar->wait[gen] = 1;
    }
  } else {
    while (bar->wait[gen] == 0) ;

    if (atomic_fetch_add(&(bar->entered[gen]), -1) == 1) {
      bar->wait[gen] = 0;
   }
  }
}

#endif
