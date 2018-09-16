/* Copyright 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
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
memcpy(void *aa, const void *bb, size_t n)
{
#if 0
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
	  *la++ = b0;
	  *la++ = b1;
	  *la++ = b2;
	  *la++ = b3;
	  *la++ = b4;
	  *la++ = b5;
	  *la++ = b6;
	  *la++ = b7;
	  *la++ = b8;
	}
    }

  while (la < lend)
    BODY (la, lb, long);

  a = (char *)la;
  b = (const char *)lb;
  if (unlikely (a < end))
    goto small;
  return aa;
#else
  __asm__ __volatile__ (
    /* 108e6 */ "xor     a5,a1,a0\n"
    /* 108ea */ "andi    a5,a5,7\n"
    /* 108ec */ "add     a7,a0,a2\n"
    /* 108f0 */ "bnez    a5,6f\n"    /* 10938 <memcpy+0x52> */
    /* 108f2 */ "li      a5,7\n"
    /* 108f4 */ "bleu    a2,a5,6f\n" /* 10938 <memcpy+0x52> */
    /* 108f8 */ "andi    a4,a0,7\n"
    /* 108fc */ "mv      a5,a0\n"
    /* 108fe */ "bnez    a4,8f\n"    /* 10950 <memcpy+0x6a> */
                "1:\n"
    /* 10900 */ "andi    a6,a7,-8\n"
    /* 10904 */ "addi    a4,a6,-64\n"
    /* 10908 */ "bltu    a5,a4,9f\n" /* 10976 <memcpy+0x90> */
                "2:\n"
    /* 1090c */ "mv      a3,a1\n"
    /* 1090e */ "mv      a4,a5\n"
    /* 10910 */ "bleu    a6,a5,4f\n" /* 10932 <memcpy+0x4c> */
                "3:\n"
    /* 10914 */ "ld      a2,0(a3)\n"
    /* 10916 */ "addi    a4,a4,8\n"
    /* 10918 */ "addi    a3,a3,8\n"
    /* 1091a */ "sd      a2,-8(a4)\n"
    /* 1091e */ "bltu    a4,a6,3b\n" /* 10914 <memcpy+0x2e> */
    /* 10922 */ "not     a4,a5\n"
    /* 10926 */ "add     a6,a6,a4\n"
    /* 10928 */ "andi    a6,a6,-8\n"
    /* 1092c */ "addi    a6,a6,8\n"
    /* 1092e */ "add     a5,a5,a6\n"
    /* 10930 */ "add     a1,a1,a6\n"
                "4:\n"
    /* 10932 */ "bltu    a5,a7,7f\n" /* 1093e <memcpy+0x58> */
                "5:\n"
    /* 10936 */ "ret\n"
                "6:\n"
    /* 10938 */ "mv      a5,a0\n"
    /* 1093a */ "bleu    a7,a0,5b\n" /* 10936 <memcpy+0x50> */
                "7:\n"
    /* 1093e */ "lbu     a4,0(a1)\n"
    /* 10942 */ "addi    a5,a5,1\n"
    /* 10944 */ "addi    a1,a1,1\n"
    /* 10946 */ "sb      a4,-1(a5)\n"
    /* 1094a */ "bltu    a5,a7,7b\n" /* 1093e <memcpy+0x58> */
    /* 1094e */ "ret\n"
                "8:\n"
    /* 10950 */ "lbu     a3,0(a1)\n"
    /* 10954 */ "addi    a5,a5,1\n"
    /* 10956 */ "andi    a4,a5,7\n"
    /* 1095a */ "sb      a3,-1(a5)\n"
    /* 1095e */ "addi    a1,a1,1\n"
    /* 10960 */ "beqz    a4,1b\n"    /* 10900 <memcpy+0x1a> */
    /* 10962 */ "lbu     a3,0(a1)\n"
    /* 10966 */ "addi    a5,a5,1\n"
    /* 10968 */ "andi    a4,a5,7\n"
    /* 1096c */ "sb      a3,-1(a5)\n"
    /* 10970 */ "addi    a1,a1,1\n"
    /* 10972 */ "bnez    a4,8b\n"    /* 10950 <memcpy+0x6a> */
    /* 10974 */ "j       1b\n"       /* 10900 <memcpy+0x1a> */
                "9:\n"
    /* 10976 */ "ld      t2,0(a1)\n"
    /* 1097a */ "ld      t0,8(a1)\n"
    /* 1097e */ "ld      t6,16(a1)\n"
    /* 10982 */ "ld      t5,24(a1)\n"
    /* 10986 */ "ld      t4,32(a1)\n"
    /* 1098a */ "ld      t3,40(a1)\n"
    /* 1098e */ "ld      t1,48(a1)\n"
    /* 10992 */ "ld      a2,56(a1)\n"
    /* 10994 */ "addi    a1,a1,72\n"
    /* 10998 */ "addi    a5,a5,72\n"
    /* 1099c */ "ld      a3,-8(a1)\n"
    /* 109a0 */ "sd      t2,-72(a5)\n"
    /* 109a4 */ "sd      t0,-64(a5)\n"
    /* 109a8 */ "sd      t6,-56(a5)\n"
    /* 109ac */ "sd      t5,-48(a5)\n"
    /* 109b0 */ "sd      t4,-40(a5)\n"
    /* 109b4 */ "sd      t3,-32(a5)\n"
    /* 109b8 */ "sd      t1,-24(a5)\n"
    /* 109bc */ "sd      a2,-16(a5)\n"
    /* 109c0 */ "sd      a3,-8(a5)\n"
    /* 109c4 */ "bltu    a5,a4,9b\n" /* 10976 <memcpy+0x90> */
    /* 109c8 */ "j       2b\n"       /* 1090c <memcpy+0x26> */
                ".short 0\n"
  );
  __builtin_unreachable();
#endif
}
