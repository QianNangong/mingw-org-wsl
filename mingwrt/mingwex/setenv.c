/*
 * setenv.c
 *
 * Implementation of POSIX standard IEEE 1003.1-2001 setenv() function;
 * may also be invoked inline, as "retval = setenv( varname, NULL, 1 )",
 * to implement the complementary unsetenv() function.
 *
 * $Id$
 *
 * Written by Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2016, 2021, 2022, MinGW.OSDN Project
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */
#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int __mingw_setenv( const char *var, const char *value, int overwrite )
{
  /* Core implementation for both setenv() and unsetenv() functions;
   * at the outset, assume that the requested operation may fail.
   */
  int retval = -1;

  /* The specified "var" name MUST be non-NULL, not a zero-length
   * string, and must not include any '=' character.
   */
  if( var && *var && (strchr( var, '=' ) == NULL) )
  {
    /* A properly named variable may be added to, removed from,
     * or modified within the environment, ONLY if "overwrite"
     * mode is enabled, OR if the named variable does not yet
     * exist...
     */
    if( overwrite || getenv( var ) == NULL )
    {
      /* ... in which cases, we convert the specified name and
       * value into the appropriate form for use with putenv(),
       * (noting that we accept a NULL "value" as equivalent to
       * a zero-length string, which renders putenv() as the
       * equivalent of unsetenv()).
       */
      const char *fmt = "%s=%s";
      const char *val = value ? value : "";
      char buf[1 + __mingw_snprintf( NULL, 0, fmt, var, val )];
      __mingw_snprintf( buf, sizeof( buf ), fmt, var, val );

      /* "buf" is now formatted as "var=value", in the form
       * required by putenv(), but it exists only within our
       * volatile stack-frame space.  POSIX.1 suggests that we
       * should copy it to more persistent storage, before it
       * is passed to putenv(), to associate an environment
       * pointer with it.  However, we note that Microsoft's
       * putenv() implementation appears to make such a copy
       * in any case, so we do not do so; (in fact, if we did
       * strdup() it (say), then we would leak memory).
       */
      if( (retval = putenv( buf )) != 0 )
	/*
	 * If putenv() returns non-zero, indicating failure, the
	 * most probable explanation is that there wasn't enough
	 * free memory; ensure that errno is set accordingly.
	 */
        errno = ENOMEM;
    }
    else
      /* The named variable already exists, and overwrite mode
       * was not enabled; there is nothing to be done.
       */
      retval = 0;
  }
  else
    /* The specified environment variable name was invalid.
     */
    errno = EINVAL;

  /* Succeed or fail, "retval" has now been set to indicate the
   * appropriate status for return.
   */
  return retval;
}

/* $RCSfile$: end of file */
