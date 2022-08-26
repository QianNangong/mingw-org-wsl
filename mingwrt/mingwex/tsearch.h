/*
 * tsearch.h
 *
 * A private wrapper, around <search.h>, to facilitate implementation of
 * the POSIX.1 tsearch(), tfind(), tdelete(), and twalk() functions.
 *
 *
 * $Id$
 *
 * Written Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2022, MinGW.OSDN Project.
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, this permission notice, and the following
 * disclaimer shall be included in all copies or substantial portions of
 * the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OF OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 *
 * All MinGW headers must include <_mingw.h>; ensure that it has been
 * included early, because we need...
 */
#include <_mingw.h>

/* ...to suppress its non-null argument annotations, BEFORE including
 * the public <search.h>, to ensure that GCC does not optimize away any
 * internal argument validation checks, when compiling the tsearch(),
 * tfind(), tdelete(), and twalk() implementations.
 */
#undef  __MINGW_ATTRIB_NONNULL
#define __MINGW_ATTRIB_NONNULL(ARG_INDEX)  /* NOTHING */

#include <search.h>

/* In addition to the public API declarations from <search.h>, each of
 * the tsearch(), tfind(), tdelete(), and twalk() implementations needs
 * this privately defined representation of a binary tree node.
 */
typedef
struct node
{ const void	*key;
  struct node 	*llink, *rlink;
} node_t;

/* $RCSfile$: end of file */
