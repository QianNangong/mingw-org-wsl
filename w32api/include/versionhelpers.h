/*
 * versionhelpers.h
 *
 * A MinGW emulation of the API implementation, first provided by Microsoft
 * as a component of the Windows-8.1 software development kit, with addition
 * of fail-safe support for use on legacy Windows versions.
 *
 * Note that use of the APIs, declared herein, is NOT recommended; usually,
 * there are better ways to check for availablity of specific features, than
 * to blindly rely on OS version number comparisons.
 *
 *
 * $Id$
 *
 * Written by Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2021, 2022, MinGW.OSDN Project
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
#ifndef _VERSIONHELPERS_H
#define _VERSIONHELPERS_H
#pragma GCC system_header

#include <windef.h>

_BEGIN_C_DECLS

/* These API declarations have been written from scratch, without reference,
 * in any way, to the original Microsoft implementation.  Although Microsoft
 * did not provide their implementation until publication of the Win-8.1 SDK,
 * it is understood, from their public documentation, that the underlying API
 * has been available since Win-2K, and that the Win-8.1 SDK implementation
 * is backward-compatible with all Win-NT versions since then; however, it
 * is not clear how fail-safe it may be, should a dependent application be
 * deployed on Win-9x, Win-NT4, or earlier.
 *
 * To address this uncertainty, this MinGW implementation is based on the
 * following pair of wrapper functions, provided in LIBMINGW32.A; these will
 * delegate to the VerifyVersionInfo() API, after confirming that the host
 * platform supports it, while exhibiting suitable fail-safe behaviour, in
 * case it does not.
 */
BOOL __mingw_osver_at_least( DWORD, DWORD, DWORD );
BOOL __mingw_osver_server( void );

/* The actual version helper APIs take the form of in-line functions.
 */
#define VERSIONHELPERAPI \
static __inline__ __attribute__((__always_inline__)) BOOL

/* This first pair of functions require distinct implementations...
 */
VERSIONHELPERAPI IsWindowsServer()
{ return __mingw_osver_server(); }

VERSIONHELPERAPI
IsWindowsVersionOrGreater( WORD __major, WORD __minor, WORD __sp )
{ return __mingw_osver_at_least( __major, __minor, __sp ); }

/* ...while the remainder are sufficiently generic to lend themselves to
 * instantiation by use of a common convenience macro:
 */
#undef __mingw_generic_version_helper
#define __mingw_generic_version_helper( FUNCTION, M, N, S ) \
VERSIONHELPERAPI FUNCTION( void ){ return __mingw_osver_at_least( M, N, S ); }

__mingw_generic_version_helper( IsWindowsXPOrGreater,         5, 1, 0 )
__mingw_generic_version_helper( IsWindowsXPSP1OrGreater,      5, 1, 1 )
__mingw_generic_version_helper( IsWindowsXPSP2OrGreater,      5, 1, 2 )
__mingw_generic_version_helper( IsWindowsXPSP3OrGreater,      5, 1, 3 )
__mingw_generic_version_helper( IsWindowsVistaOrGreater,      6, 0, 0 )
__mingw_generic_version_helper( IsWindowsVistaSP1OrGreater,   6, 0, 0 )
__mingw_generic_version_helper( IsWindowsVistaSP2OrGreater,   6, 0, 2 )
__mingw_generic_version_helper( IsWindows7OrGreater,          6, 1, 0 )
__mingw_generic_version_helper( IsWindows7SP1OrGreater,       6, 1, 1 )
__mingw_generic_version_helper( IsWindows8OrGreater,          6, 2, 0 )
__mingw_generic_version_helper( IsWindows8Point1OrGreater,    6, 3, 0 )
__mingw_generic_version_helper( IsWindows10OrGreater,        10, 0, 0 )

#undef __mingw_generic_version_helper

_END_C_DECLS

#endif	/* !_VERSIONHELPERS_H: $RCSfile$: end of file */
