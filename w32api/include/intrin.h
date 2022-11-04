/*
 * intrin.h
 *
 * Map Microsoft intrinsics to equivalent GCC built-in functions;
 * (these mappings are supported ONLY for GCC-4.7, and later).
 *
 *
 * $Id$
 *
 * Written by Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2022, MinGW.OSDN Project
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
#ifndef _INTRIN_H
#pragma GCC system_header
#define _INTRIN_H

#ifndef __ATOMIC_ACQ_REL
#error "Interlocked memory access intrinsics require GCC-4.7 (or later)."
#endif

/* Already defined, if <winnt.h> has been included beforehand,
 * we reproduce the definition of _WIN32_INTRINSIC here, in case
 * the user includes <intrin.h> first; (this is deliberately NOT
 * guarded, to ensure that consistency is preserved).
 */
#define _WIN32_INTRINSIC  extern __inline__ __attribute__((gnu_inline))

/* The following helper macro is local to <intrin.h>; it makes it
 * more convenient to implement the intrinsic functions.
 */
#define __w32api_intrinsic__(NAME, TYPE)  _WIN32_INTRINSIC	\
  __implement__(NAME, TYPE) {__action__}

/* Declare and implement the InterlockedExchange() function family...
 */
long _InterlockedExchange (volatile long *, long);
short _InterlockedExchange16 (volatile short *, short);
long long _InterlockedExchange64 (volatile long long *, long long);
char _InterlockedExchange8 (volatile char *, char);

#define __implement__(NAME, TYPE)  TYPE NAME (volatile TYPE *dest, TYPE val)
#define __action__  return __atomic_exchange_n (dest, val, __ATOMIC_ACQ_REL);

__w32api_intrinsic__( _InterlockedExchange, long )
__w32api_intrinsic__( _InterlockedExchange16, short )
__w32api_intrinsic__( _InterlockedExchange64, long long )
__w32api_intrinsic__( _InterlockedExchange8, char )

#undef  __action__
/* The preceding implementation may be readily adapted to deliver
 * the InterlockedExchangeAdd() family of functions, by the simple
 * expedient of a redefined action macro...
 */
#define __action__  return __atomic_fetch_add (dest, val, __ATOMIC_ACQ_REL);

long _InterlockedExchangeAdd (volatile long *, long);
short _InterlockedExchangeAdd16 (volatile short *, short);
long long _InterlockedExchangeAdd64 (volatile long long *, long long);
char _InterlockedExchangeAdd8 (volatile char *, char);

__w32api_intrinsic__( _InterlockedExchangeAdd, long )
__w32api_intrinsic__( _InterlockedExchangeAdd16, short )
__w32api_intrinsic__( _InterlockedExchangeAdd64, long long )
__w32api_intrinsic__( _InterlockedExchangeAdd8, char )

#undef  __action__
/* A further redefinition of the action macro delivers a further
 * adaptation, to the alternative semantics of the InterlockedAdd()
 * family of functions, (which Microsoft document as supported only
 * on the Itanium Processor Family, but GCC provides intrinsics to
 * support the semantics, regardless of architecture)...
 */
#define __action__  return __atomic_add_fetch (dest, val, __ATOMIC_ACQ_REL);

/* Microsoft document only 32-bit and 64-bit variants of these;
 * (there appear to be no 8-bit, or 16-bit variants)...
 */
long _InterlockedAdd (volatile long *, long);
long long _InterlockedAdd64 (volatile long long *, long long);

__w32api_intrinsic__( _InterlockedAdd, long )
__w32api_intrinsic__( _InterlockedAdd64, long long )

#undef __implement__
#undef __action__

/* Declare and implement the InterlockedCompareExchange() function family...
 */
long _InterlockedCompareExchange (volatile long *, long, long);
short _InterlockedCompareExchange16 (volatile short *, short, short);
long long _InterlockedCompareExchange64 (volatile long long *, long long, long long);
char _InterlockedCompareExchange8 (volatile char *, char, char);

#define __action__  return __atomic_compare_exchange __arglist__ ? exp : *dest;
#define __implement__(NAME, TYPE)  TYPE NAME (volatile TYPE *dest, TYPE val, TYPE exp)
#define __arglist__  (dest, &exp, &val, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)

__w32api_intrinsic__( _InterlockedCompareExchange, long )
__w32api_intrinsic__( _InterlockedCompareExchange16, short )
__w32api_intrinsic__( _InterlockedCompareExchange64, long long )
__w32api_intrinsic__( _InterlockedCompareExchange8, char )

#undef __implement__
#undef __arglist__
#undef __action__

/* InterlockedIncrement() and InterlockedDecrement() operations;
 * a common implementation macro may be appled in both cases, but
 * with a distinct action macro for each...
 */
#define __implement__(NAME, TYPE)  TYPE NAME (volatile TYPE *dest)

long _InterlockedDecrement (volatile long *);
short _InterlockedDecrement16 (volatile short *);
long long _InterlockedDecrement64 (volatile long long *);
char _InterlockedDecrement8 (volatile char *);

long _InterlockedIncrement (volatile long *);
short _InterlockedIncrement16 (volatile short *);
long long _InterlockedIncrement64 (volatile long long *);
char _InterlockedIncrement8 (volatile char *);

/* For InterlockedIncrement(), the action is to add one to *dest...
 */
#define __action__  return __atomic_add_fetch (dest, 1, __ATOMIC_ACQ_REL);

__w32api_intrinsic__( _InterlockedIncrement, long )
__w32api_intrinsic__( _InterlockedIncrement16, short )
__w32api_intrinsic__( _InterlockedIncrement64, long long )
__w32api_intrinsic__( _InterlockedIncrement8, char )

#undef __action__

/* For InterlockedDecrement(), the action is to subtract one from *dest...
 */
#define __action__  return __atomic_sub_fetch (dest, 1, __ATOMIC_ACQ_REL);

__w32api_intrinsic__( _InterlockedDecrement, long )
__w32api_intrinsic__( _InterlockedDecrement16, short )
__w32api_intrinsic__( _InterlockedDecrement64, long long )
__w32api_intrinsic__( _InterlockedDecrement8, char )

#undef __implement__
#undef __action__

/* Interlocked bit-mask operations; all exhibit the same prototype,
 * which may be carried through a common implementation macro for each
 * of InterlockedAnd(), InterlockedOR(), and InterlockedXor() variants,
 * with just the action macro being redefined for each case...
 */
#define __implement__(NAME, TYPE)  TYPE NAME (volatile TYPE *dest, TYPE mask)

/* Declare and implement the InterlockedAnd() function family...
 */
long _InterlockedAnd (volatile long *, long);
short _InterlockedAnd16 (volatile short *, short);
long long _InterlockedAnd64 (volatile long long *, long long);
char _InterlockedAnd8 (volatile char *, char);

#define __action__  return __atomic_fetch_and (dest, mask, __ATOMIC_ACQ_REL);

__w32api_intrinsic__( _InterlockedAnd, long )
__w32api_intrinsic__( _InterlockedAnd16, short )
__w32api_intrinsic__( _InterlockedAnd64, long long )
__w32api_intrinsic__( _InterlockedAnd8, char )

#undef __action__

/* Declare and implement the InterlockedOr() function family...
 */
long _InterlockedOr (volatile long *, long);
short _InterlockedOr16 (volatile short *, short);
long long _InterlockedOr64 (volatile long long *, long long);
char _InterlockedOr8 (volatile char *, char);

#define __action__  return __atomic_fetch_or (dest, mask, __ATOMIC_ACQ_REL);

__w32api_intrinsic__( _InterlockedOr, long )
__w32api_intrinsic__( _InterlockedOr16, short )
__w32api_intrinsic__( _InterlockedOr64, long long )
__w32api_intrinsic__( _InterlockedOr8, char )

#undef __action__

/* Declare and implement the InterlockedXor() function family...
 */
long _InterlockedXor (volatile long *, long);
short _InterlockedXor16 (volatile short *, short);
long long _InterlockedXor64 (volatile long long *, long long);
char _InterlockedXor8 (volatile char *, char);

#define __action__  return __atomic_fetch_xor (dest, mask, __ATOMIC_ACQ_REL);

__w32api_intrinsic__( _InterlockedXor, long )
__w32api_intrinsic__( _InterlockedXor16, short )
__w32api_intrinsic__( _InterlockedXor64, long long )
__w32api_intrinsic__( _InterlockedXor8, char )

#undef __w32api_intrinsic__
#undef __implement__
#undef __action__

#endif	/* !_INTRIN_H: $RCSfile$: end of file */
