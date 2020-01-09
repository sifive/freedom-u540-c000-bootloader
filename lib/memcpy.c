/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

/* Copyright (c) 2017  SiFive Inc. All rights reserved.

   This copyrighted material is made available to anyone wishing to use,
   modify, copy, or redistribute it subject to the terms and conditions
   of the FreeBSD License.   This program is distributed in the hope that
   it will be useful, but WITHOUT ANY WARRANTY expressed or implied,
   including the implied warranties of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  A copy of this license is available at
   http://www.opensource.org/licenses.
*/

#include <string.h>
#include <stdint.h>

#define unlikely(X) __builtin_expect (!!(X), 0)

void *
memcpy(void *__restrict aa, const void *__restrict bb, size_t n)
{
  #define BODY(a, b, t) { \
    t tt = *b; \
    a++, b++; \
    *(a - 1) = tt; \
  }

  char *a = (char *)aa;
  const char *b = (const char *)bb;
  char *end = a + n;
  uintptr_t msk = sizeof (long) - 1;
  if (unlikely ((((uintptr_t)a & msk) != ((uintptr_t)b & msk))
	       || n < sizeof (long)))
    {
small:
      if (__builtin_expect (a < end, 1))
	while (a < end)
	  BODY (a, b, char);
      return aa;
    }

  if (unlikely (((uintptr_t)a & msk) != 0))
    while ((uintptr_t)a & msk)
      BODY (a, b, char);

  long *la = (long *)a;
  const long *lb = (const long *)b;
  long *lend = (long *)((uintptr_t)end & ~msk);

  if (unlikely (la < (lend - 8)))
    {
      while (la < (lend - 8))
	{
	  long b0 = *lb++;
	  long b1 = *lb++;
	  long b2 = *lb++;
	  long b3 = *lb++;
	  long b4 = *lb++;
	  long b5 = *lb++;
	  long b6 = *lb++;
	  long b7 = *lb++;
	  long b8 = *lb++;
	  la += 9;
	  la[-9] = b0;
	  la[-8] = b1;
	  la[-7] = b2;
	  la[-6] = b3;
	  la[-5] = b4;
	  la[-4] = b5;
	  la[-3] = b6;
	  la[-2] = b7;
	  la[-1] = b8;
	}
    }

  while (la < lend)
    BODY (la, lb, long);

  a = (char *)la;
  b = (const char *)lb;
  if (unlikely (a < end))
    goto small;
  return aa;
}
