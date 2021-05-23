/*
 * legacy.h
 *
 * Run-time binding helper routines, to facilitate access to APIs which
 * may not be universally supported, while allowing for graceful fall-back
 * action, when running on legacy Windows versions.
 *
 * $Id$
 *
 * Written by Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2021, MinGW.org Project
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */
#ifndef _LEGACY_H
#define _LEGACY_H

/* Dynamic legacy support is dependent of standard Windows-API
 * features, which are declared in <winbase.h>, and implemented
 * in kernel32.dll; the supplementary API helper functions, which
 * are declared herein, are all implemented as extensions within
 * libkernel32.a, whence they will be statically linked.
 */
#include <winbase.h>

_BEGIN_C_DECLS

/* Manifest constants to represent the resolution state of any
 * DLL entry-point; this is assumed to be recorded in a static
 * "void *" pointer, specific to each entry-point, initialized
 * to "API_UNCHECKED", and passed to the resolver, whence the
 * return value, (which may be either an actual entry-point
 * function pointer, or "API_UNSUPPORTED"), should be assigned
 * in place of the initial "API_UNCHECKED" value.
 */
#define API_UNCHECKED		(void *)(-1)
#define API_UNSUPPORTED 	(void *)(0)

/* The following is a duplicate of the error code, as nominally
 * defined in <winerror.h>; DO NOT define it conditionally, since
 * that would deny the compiler an opportunity to verify that it
 * is a faithful duplicate, if <winerror.h> is included first.
 */
#define ERROR_OLD_WIN_VERSION	   1150L

/* DLL-specific entry-point resolvers; declare as "pure", to
 * avoid GCC's penchant for burdening the code with unnecessary
 * register saves, and restores of memory, at point of call.
 */
extern __attribute__((pure))
void *__kernel32_entry_point (void *, const char *);

/* Entry-point resolvers for named DLLs, (explicitly bound at
 * link-time, or dynamically loaded, respectively); declared as
 * "pure" for same reason as above.
 */
extern __attribute__((pure))
void *__bound_dll_entry_point (void *, const char *, const char *);

/* Whereas the preceding resolver assumes that the DLL named by
 * its first "const char *" argument has been explicitly bound at
 * link-time, (and will return "API_UNSUPPORTED" if it has not),
 * the following will load the DLL if necessary, (but it will NOT
 * increment the reference count, if the DLL is already mapped
 * into the process address space).
 */
extern __attribute__((pure))
void *__unbound_dll_entry_point (void *, const char *, const char *);

/* The following helper function, which is ALWAYS expanded in-line,
 * provides a convenient mechanism for returning an error status code,
 * while also setting said code as Windows last error.
 */
__CRT_ALIAS int __legacy_support( int status )
{ SetLastError( status ); return status; }

_END_C_DECLS

#endif	/* !_LEGACY_H: $RCSfile$: end of file */
