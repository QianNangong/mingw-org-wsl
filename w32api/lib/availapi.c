/*
 * availapi.c
 *
 * Provides generic DLL entry-point lookup helper functions, to facilitate
 * run-time linking of API functions which may not be supported in legacy
 * versions of Windows.
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
 *
 * Compile this module multiple times, once for each entry-point resolver
 * which is required; the specifics of the resolver are determined thus:
 *
 *   $ gcc -c -D_bound availapi.c -o bound.o
 *
 * will create a generic resolver, named __bound_dll_entry_point(), which
 * will resolve entry-points ONLY within DLLs which have been explicitly
 * loaded beforehand.  Conversely:
 *
 *   $ gcc -c -D_unbound availapi.c -o unbound.o
 *
 * will create a generic resolver, named __unbound_dll_entry_point(); this
 * will attempt to load a named DLL, if it is not already mapped, before it
 * attempts to resolve a named entry-point within it.  Finally:
 *
 *   $ gcc -c -D_lib=DLLNAME [-D_bound] availapi.c -o dllentry.o
 *
 * will create a resolver specific to the DLL specified by DLLNAME, (the
 * file-name only part of the DLL name, WITHOUT either the ".dll" suffix,
 * or any preceding directory path qualification; this resolver will be
 * named __DLLNAME_entry_point().  If the "-D_lib=DLLNAME" specification
 * is accompanied by the optional "-D_bound" flag, this resolver will be
 * implemented as a thin wrapper around __bound_dll_entry_point(); OTOH,
 * if the "-D_bound" flag is not specified, it will be implemented as a
 * thin wrapper around __unbound_dll_entry_point().
 *
 */
#include "legacy.h"

#if defined _lib
/* The entry-point resolver is to be associated with a specifically
 * named DLL; define a set of mappings between preferred object file
 * names (aliases), and their associated DLL names; (note that this
 * facility is primarily provided to accommodate makefile mapping of
 * resolver object file names to DLL names; the DLLNAME reference,
 * within the command line "-D_lib=DLLNAME" specification, is given
 * as the alias, but within the resolver FUNCTION name, it is ALWAYS
 * set to match the "DLL Name" entry from the following table):
 *
 *        Alias     DLL Name
 *        --------  -------- */
# define  k32entry  kernel32

/* Provide a set of macros, to derive the entry-point resolver name,
 * and its associated DLL name, from the command line assignment for
 * the object file's base name:
 */
# define _dll(_lib) _as_string(_lib) ".dll"
# define _entry(_lib) __##_lib##_entry_point
# define _entry_point(_lib) _entry(_lib)
# define _as_string(_name) #_name

/* Implement the appropriately named entry-point resolver function...
 */
void *_entry_point(_lib) (void *hook, const char *procname)
# if defined _bound
{ /* ...in terms of the appropiate resolver for a DLL which is
   * expected to have been implicitly loaded, (i.e. explicitly
   * bound to the executable, at link-time)...
   */
  return __bound_dll_entry_point( hook, _dll(_lib), procname );
}
# else
{ /* ...or otherwise, for a DLL which MAY need to be explicitly
   * loaded, on demand.
   */
  return __unbound_dll_entry_point( hook, _dll(_lib), procname );
}
# endif
#elif defined _bound
/* This entry-point resolver is to be generic, w.r.t. the DLL name
 * with which it will be associated, but will require that the named
 * DLL has been explicitly bound to the application, at link-time.
 */
void *__bound_dll_entry_point
( void *hook, const char *dllname, const char *procname )
{
  /* If the passed entry-point hook has already been assigned, then
   * there is nothing more to do, other than to return it...
   */
  if( hook == API_UNCHECKED )
  { /* ...otherwise, we perform a DLL entry-point lookup, considering
     * only DLLs which are already mapped into the address space of the
     * calling process, and subsequently updating the hook to represent
     * the entry-point, or mark it as permanently unsupported.
     */
    HMODULE dll = GetModuleHandleA( dllname );
    hook = (dll != NULL) ? GetProcAddress( dll, procname ) : API_UNSUPPORTED;
  }
  /* In any case, we return the (possibly updated) hook, which should
   * then be recorded by the caller.
   */
  return hook;
}
#elif defined _unbound
/* This entry-point resolver performs a similar function to that above,
 * except that it will attempt to explicitly load any named DLL which is
 * not already mapped into the address space of the calling process.
 */
void *__unbound_dll_entry_point
( void *hook, const char *dllname, const char *procname )
{
  /* If the passed entry-point hook has already been assigned, then
   * there is nothing more to do, other than to return it...
   */
  if( hook == API_UNCHECKED )
  { /* ...otherwise, we perform a DLL entry-point lookup, loading
     * the named DLL, if it has not yet been mapped into the address
     * space of the calling process...
     */
    HMODULE dll = GetModuleHandleA( dllname );
    if( (dll == NULL) && ((dll = LoadLibraryA( dllname )) == NULL) )
      /*
       * ...marking the hook as permanently unsupported, in the
       * event of failure to map the DLL...
       */
      return hook = API_UNSUPPORTED;

    /* ...otherwise, updating it to reflect the lookup result.
     */
    hook = GetProcAddress( dll, procname );
  }
  /* In any case, we return the (possibly updated) hook, which should
   * then be recorded by the caller.
   */
  return hook;
}
#else
/* None of the mandatory -D_spec arguments have been specified; we need
 * at least one of...
 */
# error "A -D_lib=DLLNAME, -D_bound, or -D_unbound argument is required."
#endif

/* $RCSfile$: end of file */
