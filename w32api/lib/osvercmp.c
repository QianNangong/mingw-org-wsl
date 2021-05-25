/*
 * osvercmp.c
 *
 * Implements a pair of wrappers for the WinAPI VerifyVersionInfo() API,
 * to facilitate provision of support for a MinGW emulation of Microsoft's
 * <versionhelpers.h>.  Although Microsoft first provided this as part of
 * the SDK for Windows-8.1, the underlying VerifyVersionInfo() API has
 * been supported since Win2K; the wrappers, implemented herein, extend
 * support to legacy platforms, providing fail-safe handling on earlier
 * platforms, which lack the VerifyVersionInfo() API.
 *
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
 *
 * Since the underlying VerifyVersionInfo() API was not introduced until
 * Win-2K, and MinGW's WinAPI is nominally compiled with legacy version
 * guarantees for Win-NT4 only, we must create a clean slate...
 *
 */
#undef NTDDI_VERSION
#undef _WIN32_WINDOWS
#undef _WIN32_WINNT

/* ...whence we may request exposure of, and access to, the necessary
 * Win-2K API features; (note that this does not preclude use on earlier
 * legacy platforms, since the implementation incorporates appropriate
 * fail-safe handling).
 */
#define _WIN32_WINNT  _WIN32_WINNT_WIN2K

#include <winbase.h>
#include <versionhelpers.h>

#include "legacy.h"

/* We need a function pointer type definition, matching the signature of
 * VerifyVersionInfo(), so that we may verify its availability, BEFORE we
 * attempt to invoke it indirectly.
 */
typedef BOOL WINAPI (*validator_fn)( OSVERSIONINFOEXA *, DWORD, DWORDLONG );

static validator_fn __mingw_osver_comparator( void )
{
  /* Internal helper function, to look up, and store, the entry point
   * address of VerifyVersionInfo(), if it is available in KERNEL32.DLL;
   * (note that this is a one-time initialization, within the lifetime
   * of the calling process).
   */
  static void *entry = API_UNCHECKED;
  entry = __kernel32_entry_point( entry, "VerifyVersionInfoA" );
  return (validator_fn)(entry);
}

/* For OS version comparisons, we need to inspect the major, and minor
 * version numbers, both for the base OS, and any applied service packs...
 */
static const WORD __mingw_osver_mask = VER_MAJORVERSION
| VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR;

static DWORDLONG __mingw_osver_test( void )
{ /* ...selecting the "greater-than-or-equal" comparison mode for each,
   * in turn, as facilitated by this internal helper function; (note
   * that, once again, this performs a one-time initialization of the
   * comparison selector, for each item of version information).
   */
  static DWORDLONG test = 0LL;
  if( test == 0LL )
  { /* We need to initialize it, on this occasion; loop over the bits
     * of the comparison selector mask, associating the appropriate
     * comparison type with each which is set.
     */
    for( DWORD mask = 1; mask < __mingw_osver_mask; mask <<= 1 )
      if( (mask & __mingw_osver_mask) != 0 )
	VER_SET_CONDITION( test, mask, VER_GREATER_EQUAL );
  }
  return test;
}

BOOL __mingw_osver_at_least( DWORD __major, DWORD __minor, DWORD __sp )
{
  /* This is the public entry point, through which OS version number
   * comparison requests may be directed to VerifyVersionInfo()...
   */
  validator_fn chk;
  if( (chk = __mingw_osver_comparator()) != (validator_fn)(API_UNSUPPORTED) )
  {
    /* ...subject to initialization having established a valid entry
     * point address, to which we pass the request via the pointer.
     */
    OSVERSIONINFOEXA osinfo =
    { sizeof osinfo, __major, __minor, 0, 0, "", __sp, 0, 0, 0, 0 };
    return chk( &osinfo, __mingw_osver_mask, __mingw_osver_test() );
  }

  /* If we get to here, initialization failed to identify a valid
   * entry point address, for VerifyVersionInfo(); this represents
   * the fail-safe handling, for legacy platforms.
   */
  SetLastError( ERROR_OLD_WIN_VERSION );
  return FALSE;
}

/* In the case of the server platform identification test, we again
 * must perform an OS version check, but this must be augmented by a
 * product type assessment...
 */
static const WORD __mingw_osplatform_mask = VER_PRODUCT_TYPE
| __mingw_osver_mask;

static DWORDLONG __mingw_osplatform_test( void )
{ /* ...for which (a copy of) the version number comparison selector
   * must be augmented by addition of an equality comparator selector
   * for the server product type identifier; (as previously, this is
   * again established as a one-time initialization).
   */
  static DWORDLONG test = 0LL;
  if( test == 0LL )
  { /* We need to initialize it, on this occasion.
     */
    test = __mingw_osver_test();
    VER_SET_CONDITION( test, VER_PRODUCT_TYPE, VER_EQUAL );
  }
  return test;
}

BOOL __mingw_osver_server( void )
{ /* This alternative public entry point is specific to the server
   * platform identification test; as in the preceding case of more
   * generalized version number comparisons, this is also delegated
   * to the VerifyVersionInfo() API, using the same entry point
   * reference pointer...
   */
  validator_fn chk;
  if( (chk = __mingw_osver_comparator()) != (validator_fn)(API_UNSUPPORTED) )
  {
    /* ...once again, subject to successful initialization; in this
     * case, it is the server characterization property which is of
     * primary interest, but we do also require an OS version which
     * corresponds to Win-2K, or later.
     */
    OSVERSIONINFOEXA osinfo =
    { sizeof osinfo, 5, 0, 0, 0, "", 0, 0, 0, VER_NT_SERVER, 0 };
    return chk( &osinfo, __mingw_osplatform_mask, __mingw_osplatform_test() );
  }

  /* If we get to here, initialization failed to identify a valid
   * entry point address, for VerifyVersionInfo(); this represents
   * the fail-safe handling, for legacy platforms.
   */
  SetLastError( ERROR_OLD_WIN_VERSION );
  return FALSE;
}

/* $RCSfile$: end of file */
