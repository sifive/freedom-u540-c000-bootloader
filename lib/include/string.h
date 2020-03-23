/* Copyright (c) 2020 Emil Renner Berthing */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#ifndef LIB_INCLUDE_STRING_H
#define LIB_INCLUDE_STRING_H

#include <stddef.h>

void *memset(void *s, int c, size_t n);
void *memcpy(void *__restrict dest, const void *__restrict src, size_t n);

int strcmp(const char *s1, const char *s2);
size_t strlen(const char *s);

#endif
