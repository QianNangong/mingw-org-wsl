/* pformat.c
 *
 * $Id$
 *
 * Provides a core implementation of the formatting capabilities
 * common to the entire `printf()' family of functions; it conforms
 * generally to C99 and SUSv3/POSIX specifications, with extensions
 * to support Microsoft's non-standard format specifications.
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2008, 2009, 2011, 2014-2018, MinGW.org Project
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
 * The elements of this implementation which deal with the formatting
 * of floating point numbers, (i.e. the `%e', `%E', `%f', `%F', `%g'
 * and `%G' format specifiers, but excluding the hexadecimal floating
 * point `%a' and `%A' specifiers), make use of the `__gdtoa' function
 * written by David M. Gay, and are modelled on his sample code, which
 * has been deployed under its accompanying terms of use:--
 *
 ******************************************************************
 * Copyright (C) 1997, 1999, 2001 Lucent Technologies
 * All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of Lucent or any of its entities
 * not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.
 *
 * LUCENT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
 * IN NO EVENT SHALL LUCENT OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 ******************************************************************
 *
 */
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <locale.h>
#include <wchar.h>
#include <math.h>

/* FIXME: The following belongs in values.h, but current MinGW
 * has nothing useful there!  OTOH, values.h is not a standard
 * header, and it's use may be considered obsolete; perhaps it
 * is better to just keep these definitions here.
 */
#ifndef _VALUES_H
/*
 * values.h
 *
 */
#define _VALUES_H

#include <limits.h>

#define _TYPEBITS(type)     (sizeof(type) * CHAR_BIT)

#define LLONGBITS           _TYPEBITS(long long)

#endif /* !defined _VALUES_H -- end of file */

#include "pformat.h"

#ifndef NL_ARGMAX
/* POSIX expects this to have been defined in <limits.h>, with a value
 * no less than 9; provide this slightly more generous definition, since
 * MinGW's <limits.h> may not have defined it, (as is the normal case),
 * and we need it to guard against uncontrolled stack growth.
 */
#define NL_ARGMAX  16
#endif

#if __GNUC__ && ! defined __NO_INLINE__
# define __pformat_inline__  __inline__ __attribute__((__always_inline__))
#else
# define __pformat_inline__
#endif

/* Bit-map constants, defining the internal format control
 * states, which propagate through the flags.
 */
#define PFORMAT_HASHED      0x0800
#define PFORMAT_LJUSTIFY    0x0400
#define PFORMAT_ZEROFILL    0x0200

#define PFORMAT_JUSTIFY    (PFORMAT_LJUSTIFY | PFORMAT_ZEROFILL)
#define PFORMAT_IGNORE      -1

#define PFORMAT_SIGNED      0x01C0
#define PFORMAT_POSITIVE    0x0100
#define PFORMAT_NEGATIVE    0x0080
#define PFORMAT_ADDSPACE    0x0040

#define PFORMAT_XCASE       0x0020

#define PFORMAT_LDOUBLE     0x0004
#define PFORMAT_GROUPED     0x0001

/* `%o' format digit extraction mask, and shift count...
 * (These are constant, and do not propagate through the flags).
 */
#define PFORMAT_OMASK       0x0007
#define PFORMAT_OSHIFT      0x0003

/* `%x' and `%X' format digit extraction mask, and shift count...
 * (These are constant, and do not propagate through the flags).
 */
#define PFORMAT_XMASK       0x000F
#define PFORMAT_XSHIFT      0x0004

/* The radix point character, used in floating point formats, is
 * localised on the basis of the active LC_NUMERIC locale category.
 * It is stored locally, as a `wchar_t' entity, which is converted
 * to a (possibly multibyte) character on output.  Initialisation
 * of the stored `wchar_t' entity, together with a record of its
 * effective multibyte character length, is required each time
 * `__pformat()' is entered, (static storage would not be thread
 * safe), but this initialisation is deferred until it is actually
 * needed; on entry, the effective character length is first set to
 * the following value, (and the `wchar_t' entity is zeroed), to
 * indicate that a call of `localeconv()' is needed, to complete
 * the initialisation.
 */
#define PFORMAT_RPINIT      -3

/* The floating point format handlers return the following value
 * for the radix point position index, when the argument value is
 * infinite, or not a number.
 */
#define PFORMAT_INFNAN      -32768

#ifdef _WIN32
/* The Microsoft standard for printing `%e' format exponents is
 * with a minimum of three digits, unless explicitly set otherwise,
 * by a prior invocation of the `_set_output_format()' function.
 *
 * The following macro allows us to replicate this behaviour.
 */
# define PFORMAT_MINEXP    __pformat_exponent_digits()
/*
 * We note that Microsoft did not implement the _set_output_format()
 * API capability until the release of MSVCR80.DLL, although they did
 * subsequently retro-fit it to MSVCRT.DLL from Windows-Vista onward.
 * To circumvent this API availability limitation, *we* offer two
 * alternative options for controlling the facility:
 *
 * 1) We support the explicit assignment of the user's preference
 *    for `PRINTF_EXPONENT_DIGITS', through the simple expedient of
 *    defining it as an environment variable; this mechanism will
 *    take precedence over...
 *
 * 2) Emulation of _set_output_format(), through the use of inline
 *    functions defined in stdio.h, and supported regardless of the
 *    availability of the API within MSVCRT.DLL; this emulated API
 *    maintains state in the global `__mingw_output_format_flags'
 *    variable, (which users should consider to be private).
 */
extern unsigned int __mingw_output_format_flags;

static __pformat_inline__
int __pformat_exponent_digits( void )
{
  /* Local helper function, required to resolve the value for the
   * PFORMAT_MINEXP macro in the _WIN32 implementation case.
   */
  char *exponent_digits = getenv( "PRINTF_EXPONENT_DIGITS" );
  return ((exponent_digits != NULL) && ((unsigned)(*exponent_digits - '0') < 3))
    || (__mingw_output_format_flags & _TWO_DIGIT_EXPONENT)
    ? 2 : 3 ;
}
#else
/* When we don't care to mimic Microsoft's standard behaviour,
 * we adopt the C99/POSIX standard of two digit exponents.
 */
# define PFORMAT_MINEXP         2
#endif

typedef union
{ /* A data type agnostic representation,
   * for printf arguments of any integral data type...
   */
  signed long             __pformat_long_t;
  signed long long        __pformat_llong_t;
  unsigned long           __pformat_ulong_t;
  unsigned long long      __pformat_ullong_t;
  unsigned short          __pformat_ushort_t;
  unsigned char           __pformat_uchar_t;
  signed short            __pformat_short_t;
  signed char             __pformat_char_t;
  void *		  __pformat_ptr_t;
} __pformat_intarg_t;

typedef enum
{ /* Format interpreter state indices...
   * (used to identify the active phase of format string parsing).
   */
  PFORMAT_INIT = 0,
  PFORMAT_SET_WIDTH,
  PFORMAT_GET_PRECISION,
  PFORMAT_SET_PRECISION,
  PFORMAT_END
} __pformat_state_t;

typedef enum
{ /* Argument length classification indices...
   * (used for arguments representing integer data types).
   */
  PFORMAT_LENGTH_INT = 0,
  PFORMAT_LENGTH_DEFAULT = PFORMAT_LENGTH_INT,
  PFORMAT_LENGTH_SHORT,
  PFORMAT_LENGTH_LONG,
  PFORMAT_LENGTH_LLONG,
  PFORMAT_LENGTH_CHAR
} __pformat_length_t;
/*
 * And a macro to map any arbitrary data type to an appropriate
 * matching index, selected from those above; the compiler should
 * collapse this to a simple assignment.
 */
#define __pformat_arg_length( type )    \
  sizeof( type ) == sizeof( int )       ? PFORMAT_LENGTH_INT   : \
  sizeof( type ) == sizeof( long )      ? PFORMAT_LENGTH_LONG  : \
  sizeof( type ) == sizeof( long long ) ? PFORMAT_LENGTH_LLONG : \
  sizeof( type ) == sizeof( short )     ? PFORMAT_LENGTH_SHORT : \
  sizeof( type ) == sizeof( char )      ? PFORMAT_LENGTH_CHAR  : \
  /* should never need this default */    PFORMAT_LENGTH_INT

typedef struct
{ /* Formatting and output control data...
   * An instance of this control block is created, (on the stack),
   * for each call to `__pformat()', and is passed by reference to
   * each of the output handlers, as required.
   */
  void *         dest;
  int            flags;
  int            width;
  int            precision;
  int            rplen;
  wchar_t        rpchr;
  int            count;
  int            quota;
  int            expmin;
  int            tslen;
  wchar_t        tschr;
  char *         grouping;
} __pformat_t;

static
void __pformat_putc( int c, __pformat_t *stream )
{
  /* Place a single character into the `__pformat()' output queue,
   * provided any specified output quota has not been exceeded.
   */
  if( (stream->flags & PFORMAT_NOLIMIT) || (stream->quota > stream->count) )
  {
    /* Either there was no quota specified,
     * or the active quota has not yet been reached.
     */
    if( stream->flags & PFORMAT_TO_FILE )
      /*
       * This is single character output to a FILE stream...
       */
      fputc( c, (FILE *)(stream->dest) );

    else
      /* Whereas, this is to an internal memory buffer...
       */
      ((char *)(stream->dest))[stream->count] = c;
  }
  ++stream->count;
}

static
void __pformat_putchars( const char *s, int count, __pformat_t *stream )
{
  /* Handler for `%c' and (indirectly) `%s' conversion specifications.
   *
   * Transfer characters from the string buffer at `s', character by
   * character, up to the number of characters specified by `count', or
   * if `precision' has been explicitly set to a value less than `count',
   * stopping after the number of characters specified for `precision',
   * to the `__pformat()' output stream.
   *
   * Characters to be emitted are passed through `__pformat_putc()', to
   * ensure that any specified output quota is honoured.
   */
  if( (stream->precision >= 0) && (count > stream->precision) )
    /*
     * Ensure that the maximum number of characters transferred doesn't
     * exceed any explicitly set `precision' specification.
     */
    count = stream->precision;

  /* Establish the width of any field padding required...
   */
  if( stream->width > count )
    /*
     * as the number of spaces equivalent to the number of characters
     * by which those to be emitted is fewer than the field width...
     */
    stream->width -= count;

  else
    /* ignoring any width specification which is insufficient.
     */
    stream->width = PFORMAT_IGNORE;

  if( (stream->width > 0) && ((stream->flags & PFORMAT_LJUSTIFY) == 0) )
    /*
     * When not doing flush left justification, (i.e. the `-' flag
     * is not set), any residual unreserved field width must appear
     * as blank padding, to the left of the output string.
     */
    while( stream->width-- )
      __pformat_putc( '\x20', stream );

  /* Emit the data...
   */
  while( count-- )
    /*
     * copying the requisite number of characters from the input.
     */
    __pformat_putc( *s++, stream );

  /* If we still haven't consumed the entire specified field width,
   * we must be doing flush left justification; any residual width
   * must be filled with blanks, to the right of the output value.
   */
  while( stream->width-- > 0 )
    __pformat_putc( '\x20', stream );
}

static __pformat_inline__
void __pformat_puts( const char *s, __pformat_t *stream )
{
  /* Handler for `%s' conversion specifications.
   *
   * Transfer a NUL terminated character string, character by character,
   * stopping when the end of the string is encountered, or if `precision'
   * has been explicitly set, when the specified number of characters has
   * been emitted, if that is less than the length of the input string,
   * to the `__pformat()' output stream.
   *
   * This is implemented as a trivial call to `__pformat_putchars()',
   * passing the length of the input string as the character count,
   * (after first verifying that the input pointer is not NULL).
   */
  if( s == NULL ) s = "(null)";
  __pformat_putchars( s, strlen( s ), stream );
}

static
void __pformat_wputchars( const wchar_t *s, int count, __pformat_t *stream )
{
  /* Handler for `%C'(`%lc') and `%S'(`%ls') conversion specifications;
   * (this is a wide character variant of `__pformat_putchars()').
   *
   * Each multibyte character sequence to be emitted is passed, byte
   * by byte, through `__pformat_putc()', to ensure that any specified
   * output quota is honoured.
   */
  char buf[16]; mbstate_t state; int len = wcrtomb( buf, L'\0', &state );

  if( (stream->precision >= 0) && (count > stream->precision) )
    /*
     * Ensure that the maximum number of characters transferred doesn't
     * exceed any explicitly set `precision' specification.
     */
    count = stream->precision;

  /* Establish the width of any field padding required...
   */
  if( stream->width > count )
    /*
     * as the number of spaces equivalent to the number of characters
     * by which those to be emitted is fewer than the field width...
     */
    stream->width -= count;

  else
    /* ignoring any width specification which is insufficient.
     */
    stream->width = PFORMAT_IGNORE;

  if( (stream->width > 0) && ((stream->flags & PFORMAT_LJUSTIFY) == 0) )
    /*
     * When not doing flush left justification, (i.e. the `-' flag
     * is not set), any residual unreserved field width must appear
     * as blank padding, to the left of the output string.
     */
    while( stream->width-- )
      __pformat_putc( '\x20', stream );

  /* Emit the data, converting each character from the wide
   * to the multibyte domain as we go...
   */
  while( (count-- > 0) && ((len = wcrtomb( buf, *s++, &state )) > 0) )
  {
    char *p = buf;
    while( len-- > 0 )
      __pformat_putc( *p++, stream );
  }

  /* If we still haven't consumed the entire specified field width,
   * we must be doing flush left justification; any residual width
   * must be filled with blanks, to the right of the output value.
   */
  while( stream->width-- > 0 )
    __pformat_putc( '\x20', stream );
}

static __pformat_inline__
void __pformat_wcputs( const wchar_t *s, __pformat_t *stream )
{
  /* Handler for `%S' (`%ls') conversion specifications.
   *
   * Transfer a NUL terminated wide character string, character by
   * character, converting to its equivalent multibyte representation
   * on output, and stopping when the end of the string is encountered,
   * or if `precision' has been explicitly set, when the specified number
   * of characters has been emitted, if that is less than the length of
   * the input string, to the `__pformat()' output stream.
   *
   * This is implemented as a trivial call to `__pformat_wputchars()',
   * passing the length of the input string as the character count,
   * (after first verifying that the input pointer is not NULL).
   */
  if( s == NULL ) s = L"(null)";
  __pformat_wputchars( s, wcslen( s ), stream );
}

static
int __pformat_enable_thousands_grouping( __pformat_t *stream )
{
  /* Helper to activate the thousands digits grouping feature,
   * for any numeric conversion specification which includes the
   * apostrophe flag, subject to an appropriate grouping strategy
   * being defined within the current locale.
   */
  int enabled = stream->flags & PFORMAT_GROUPED;
  if( (enabled == PFORMAT_GROUPED) && (stream->tslen == PFORMAT_RPINIT) )
  {
    /* Grouping has been requested, but we don't yet know if it
     * is supported for the current locale; (it normally isn't in
     * the default C/POSIX locale).  Link a copy of the locale's
     * grouping strategy, if any, into the stream control block...
     */
    stream->grouping = strdup( localeconv()->grouping );
    if( (stream->grouping != NULL) && (*stream->grouping != '\0') )
    {
      /* ...and when at least one non-zero group size is specified,
       * applying to the least significant digit group, (provided it
       * isn't CHAR_MAX, which subsequently terminates grouping for
       * more significant digits), also store a copy of the group
       * separator character, (encoded as a UTF-16LE character,
       * with its corresponding MBCS byte count).
       */
      int len = 0; wchar_t ts; mbstate_t state;
      memset( &state, 0, sizeof( state ) );
      if(  (*stream->grouping < CHAR_MAX)
      &&  ((len = mbrtowc( &ts, localeconv()->thousands_sep, 16, &state )) > 0)  )
	stream->tschr = ts;
      stream->tslen = len;
    }
    /* Check that we've established a usable grouping strategy, (but
     * note that an apparently viable grouping sequence also requires
     * a serviceable separator character...
     */
    if( stream->tschr == (wchar_t)(0) )
    {
      /* ...otherwise it becomes non-viable, so we disable it).
       */
      free( stream->grouping ); stream->grouping = NULL;
      enabled = (stream->flags &= ~PFORMAT_GROUPED);
    }
  }
  /* Finally, return the enabled status for a viable grouping strategy.
   */
  return enabled == PFORMAT_GROUPED;
}

static __inline__
int __pformat_int_bufsiz( int bias, int size, __pformat_t *stream )
{
  /* Helper to establish the size of the internal buffer, which
   * is required to queue the ASCII decomposition of an integral
   * data value, prior to transfer to the output stream.
   */
  size = ((size - 1 + LLONGBITS) / size) + bias;
  size += (stream->precision > 0) ? stream->precision : 0;

  /* If thousands digits grouping is requested, and viable, we
   * must reserve additional buffer space, to store the internal
   * representation of the group separator characters...
   */
  if( __pformat_enable_thousands_grouping( stream ) )
    /*
     * ...by simply doubling the requested buffer size; (while
     * this may be excessive, it should certainly be safe).
     */
    size <<= 1;

  /* Finally, return the greater of the computed minimum buffer
   * size, and the originally requested field width.
   */
  return (size > stream->width) ? size : stream->width;
}

static
int __pformat_emit_punct( wchar_t code, int len, __pformat_t *stream )
{
  /* Helper to place the external representation of either the radix
   * point character, or the thousands digits group separator character,
   * which is applicable in the current locale, into the output stream.
   */
  if( code != (wchar_t)(0) )
  {
    /* We have a localised radix point or thousands separator mark;
     * establish a converter to make it a multibyte character...
     */
    char buf[len]; mbstate_t state;

    /* Initialise the conversion state...
     */
    memset( &state, 0, sizeof( state ) );

    /* Convert the `wchar_t' representation to multibyte...
     */
    if( (len = wcrtomb( buf, code, &state )) > 0 )
    {
      /* ...and copy to the output destination, when valid.
       */
      char *p = buf;
      while( len-- > 0 )
	__pformat_putc( *p++, stream );

      /* The requested output has been completed; inform the caller
       * that no further fall back output is required.
       */
      return 0;
    }
  }
  /* If we get to here, there was no appropriate localisation for the
   * requested output; inform the caller that it may wish to provide
   * fall back output, by returning a non-zero value.
   */
  return ~0;
}

static
void __pformat_emit_digit( int c, __pformat_t *stream )
{
  /* Helper to transfer the individual digits of any decimal numeric
   * value representation to the output stream; (note that the radix
   * point and thousands digits group separator are considered to be
   * special cases of "digits", internally represented by ASCII '.'
   * and ASCII comma, respectively).
   */
  switch( c )
  {
    case '.':
      /* Helper to place a localised representation of the radix point
       * character at the ultimate destination, when formatting fixed or
       * floating point numbers.
       */
      if( stream->rplen == PFORMAT_RPINIT )
      {
	/* Radix point initialisation not yet completed;
	 * establish a multibyte to `wchar_t' converter...
	 */
	int len; wchar_t rpchr; mbstate_t state;

	/* Initialise the conversion state...
	 */
	memset( &state, 0, sizeof( state ) );

	/* Fetch and convert the localised radix point representation...
	 */
	if( (len = mbrtowc( &rpchr, localeconv()->decimal_point, 16, &state )) > 0 )
	  /*
	   * and store it, if valid.
	   */
	  stream->rpchr = rpchr;

	/* In any case, store the reported effective multibyte length,
	 * (or the error flag), marking initialisation as `done'.
	 */
	stream->rplen = len;
      }

      /* Emit the localised radix point character code, if available...
       */
      if( __pformat_emit_punct( stream->rpchr, stream->rplen, stream ) )
	/*
	 * ...otherwise fall back to use of ASCII '.'
	 */
	__pformat_putc( '.', stream );
      break;

    case ',':
      /* Helper to emit the localised representation of the thousands
       * digit separator character, if available; in this case, although
       * we never expect it to arise, the fall back is to emit nothing.
       */
      (void)(__pformat_emit_punct( stream->tschr, stream->tslen, stream ));
      break;

    default:
      /* Helper to emit any regular digit character.
       */
      __pformat_putc( c, stream );
  }
}

static
void __pformat_int( __pformat_intarg_t value, __pformat_t *stream )
{
  /* Handler for `%d', `%i' and `%u' conversion specifications.
   *
   * Transfer the ASCII representation of an integer value parameter,
   * formatted as a decimal number, to the `__pformat()' output queue;
   * output will be truncated, if any specified quota is exceeded.
   */
  char buf[__pformat_int_bufsiz(1, PFORMAT_OSHIFT, stream)];
  char *p = buf, *grouping = NULL; int precision, groupsize = 0;

  /* First check if thousands grouping can be supported, and has been
   * requested, for the digits of this integer valued field; (note that
   * supportability has already been verified, as a side effect of the
   * computation of the necessary transfer buffer size)...
   */
  if(  (stream->grouping != NULL)
  &&  ((stream->flags & PFORMAT_GROUPED) == PFORMAT_GROUPED)
  /*
   * ...and initialise the grouping counter controls accordingly.
   */
  &&  ((groupsize = *stream->grouping) > 0) && (groupsize != CHAR_MAX)  )
    grouping = stream->grouping;

  if( stream->flags & PFORMAT_NEGATIVE )
  {
    /* The input value might be negative, (i.e. it is a signed value)...
     */
    if( value.__pformat_llong_t < 0LL )
      /*
       * It IS negative, but we want to encode it as unsigned,
       * displayed with a leading minus sign, so convert it...
       */
      value.__pformat_llong_t = -value.__pformat_llong_t;

    else
      /* It is unequivocally a POSITIVE value, so turn off the
       * request to prefix it with a minus sign...
       */
      stream->flags &= ~PFORMAT_NEGATIVE;
  }

  /* Encode the input value for display...
   */
  while( value.__pformat_ullong_t )
  {
    /* decomposing it into its constituent decimal digits...
     */
    if( (grouping != NULL) && (groupsize-- == 0) )
    {
      /* noting that, when thousands grouping is in effect, and
       * we are currently at a group boundary, we must reset the
       * grouping counter...
       */
      groupsize = (grouping[1] != '\0') ? *++grouping : *grouping;
      if( groupsize-- == CHAR_MAX )
	grouping = NULL;

      /* and store a group separator mark, using a simple ASCII
       * comma to represent it internally...
       */
      *p++ = ',';
    }
    /* before recording each digit from the decomposition, in
     * order from least significant to most significant, using
     * the local buffer as a LIFO queue in which to store them.
     */
    *p++ = '0' + (unsigned char)(value.__pformat_ullong_t % 10LL);
    value.__pformat_ullong_t /= 10LL;
  }

  if(  (stream->precision > 0)
  &&  ((precision = stream->precision - (p - buf)) > 0)  )
    /*
     * We have not yet queued sufficient digits to fill the field width
     * specified for minimum `precision'; pad with zeros to achieve this.
     */
    while( precision-- > 0 )
      *p++ = '0';

  if( (p == buf) && (stream->precision != 0) )
    /*
     * Input value was zero; make sure we print at least one digit,
     * unless the precision is also explicitly zero.
     */
    *p++ = '0';

  if( (stream->width > 0) && ((stream->width -= p - buf) > 0) )
  {
    /* We have now queued sufficient characters to display the input value,
     * at the desired precision, but this will not fill the output field...
     */
    if( stream->flags & PFORMAT_SIGNED )
      /*
       * We will fill one additional space with a sign...
       */
      stream->width--;

    if(  (stream->precision < 0)
    &&  ((stream->flags & PFORMAT_JUSTIFY) == PFORMAT_ZEROFILL)  )
      /*
       * and the `0' flag is in effect, so we pad the remaining spaces,
       * to the left of the displayed value, with zeros.
       */
      while( stream->width-- > 0 )
	*p++ = '0';

    else if( (stream->flags & PFORMAT_LJUSTIFY) == 0 )
      /*
       * the `0' flag is not in effect, and neither is the `-' flag,
       * so we pad to the left of the displayed value with spaces, so that
       * the value appears right justified within the output field.
       */
      while( stream->width-- > 0 )
	__pformat_putc( '\x20', stream );
  }

  if( stream->flags & PFORMAT_NEGATIVE )
    /*
     * A negative value needs a sign...
     */
    *p++ = '-';

  else if( stream->flags & PFORMAT_POSITIVE )
    /*
     * A positive value may have an optionally displayed sign...
     */
    *p++ = '+';

  else if( stream->flags & PFORMAT_ADDSPACE )
    /*
     * Space was reserved for displaying a sign, but none was emitted...
     */
    *p++ = '\x20';

  while( p > buf )
    /*
     * Emit the accumulated constituent digits,
     * in order from most significant to least significant...
     */
    __pformat_emit_digit( *--p, stream );

  while( stream->width-- > 0 )
    /* The specified output field has not yet been completely filled;
     * the `-' flag must be in effect, resulting in a displayed value which
     * appears left justified within the output field; we must pad the field
     * to the right of the displayed value, by emitting additional spaces,
     * until we reach the rightmost field boundary.
     */
    __pformat_putc( '\x20', stream );
}

static __pformat_inline__
int __pformat_xint_bufsiz( int bias, int size, __pformat_t *stream )
{
  /* The xint formats do not support thousands grouping; (POSIX specifies
   * only that the behaviour is undefined, if such grouping is requested).
   * We will ensure that any such request is simply ignored, before using
   * our int format determination of required buffer size.
   */
  stream->flags &= ~PFORMAT_GROUPED;
  return __pformat_int_bufsiz( bias, size, stream );
}

static
void __pformat_xint( int fmt, __pformat_intarg_t value, __pformat_t *stream )
{
  /* Handler for `%o', `%p', `%x' and `%X' conversions.
   *
   * These can be implemented using a simple `mask and shift' strategy;
   * set up the mask and shift values appropriate to the conversion format,
   * and allocate a suitably sized local buffer, in which to queue encoded
   * digits of the formatted value, in preparation for output.
   */
  int width;
  int mask = (fmt == 'o') ? PFORMAT_OMASK : PFORMAT_XMASK;
  int shift = (fmt == 'o') ? PFORMAT_OSHIFT : PFORMAT_XSHIFT;
  char buf[__pformat_xint_bufsiz(2, shift, stream)];
  char *p = buf;

  while( value.__pformat_ullong_t )
  {
    /* Encode the specified non-zero input value as a sequence of digits,
     * in the appropriate `base' encoding and in reverse digit order, each
     * encoded in its printable ASCII form, with no leading zeros, using
     * the local buffer as a LIFO queue in which to store them.
     */
    char *q;
    if( (*(q = p++) = '0' + (value.__pformat_ullong_t & mask)) > '9' )
      *q = (*q + 'A' - '9' - 1) | (fmt & PFORMAT_XCASE);
    value.__pformat_ullong_t >>= shift;
  }

  if( p == buf )
    /*
     * Nothing was queued; input value must be zero, which should never be
     * emitted in the `alternative' PFORMAT_HASHED style.
     */
    stream->flags &= ~PFORMAT_HASHED;

  if( ((width = stream->precision) > 0) && ((width -= p - buf) > 0) )
    /*
     * We have not yet queued sufficient digits to fill the field width
     * specified for minimum `precision'; pad with zeros to achieve this.
     */
    while( width-- > 0 )
      *p++ = '0';

  else if( (fmt == 'o') && (stream->flags & PFORMAT_HASHED) )
    /*
     * The field width specified for minimum `precision' has already
     * been filled, but the `alternative' PFORMAT_HASHED style for octal
     * output requires at least one initial zero; that will not have
     * been queued, so add it now.
     */
    *p++ = '0';

  if( (p == buf) && (stream->precision != 0) )
    /*
     * Still nothing queued for output, but the `precision' has not been
     * explicitly specified as zero, (which is necessary if no output for
     * an input value of zero is desired); queue exactly one zero digit.
     */
    *p++ = '0';

  if( stream->width > (width = p - buf) )
    /*
     * Specified field width exceeds the minimum required...
     * Adjust so that we retain only the additional padding width.
     */
    stream->width -= width;

  else
    /* Ignore any width specification which is insufficient.
     */
    stream->width = PFORMAT_IGNORE;

  if( ((width = stream->width) > 0)
  &&  (fmt != 'o') && (stream->flags & PFORMAT_HASHED)  )
    /*
     * For `%#x' or `%#X' formats, (which have the `#' flag set),
     * further reduce the padding width to accommodate the radix
     * indicating prefix.
     */
    width -= 2;

  if(  (width > 0) && (stream->precision < 0)
  &&  ((stream->flags & PFORMAT_JUSTIFY) == PFORMAT_ZEROFILL)  )
    /*
     * When the `0' flag is set, and not overridden by the `-' flag,
     * or by a specified precision, add sufficient leading zeros to
     * consume the remaining field width.
     */
    while( width-- > 0 )
      *p++ = '0';

  if( (fmt != 'o') && (stream->flags & PFORMAT_HASHED) )
  {
    /* For formats other than octal, the PFORMAT_HASHED output style
     * requires the addition of a two character radix indicator, as a
     * prefix to the actual encoded numeric value.
     */
    *p++ = fmt;
    *p++ = '0';
  }

  if( (width > 0) && ((stream->flags & PFORMAT_LJUSTIFY) == 0) )
    /*
     * When not doing flush left justification, (i.e. the `-' flag
     * is not set), any residual unreserved field width must appear
     * as blank padding, to the left of the output value.
     */
    while( width-- > 0 )
      __pformat_putc( '\x20', stream );

  while( p > buf )
    /*
     * Move the queued output from the local buffer to the ultimate
     * destination, in LIFO order.
     */
    __pformat_putc( *--p, stream );

  /* If we still haven't consumed the entire specified field width,
   * we must be doing flush left justification; any residual width
   * must be filled with blanks, to the right of the output value.
   */
  while( width-- > 0 )
    __pformat_putc( '\x20', stream );
}

typedef union
{ /* A multifaceted representation of an IEEE extended precision,
   * (80-bit), floating point number, facilitating access to its
   * component parts.
   */
  double                 __pformat_fpreg_double_t;
  long double            __pformat_fpreg_ldouble_t;
  struct
  { unsigned long long   __pformat_fpreg_mantissa;
    signed short         __pformat_fpreg_exponent;
  };
  unsigned short         __pformat_fpreg_bitmap[5];
  unsigned long          __pformat_fpreg_bits;
} __pformat_fpreg_t;

#ifdef _WIN32
/* TODO: maybe make this unconditional in final release...
 * (see note at head of associated `#else' block.
 */
#include "gdtoa.h"

static
char *__pformat_cvt( int mode, __pformat_fpreg_t x, int nd, int *dp, int *sign )
{
  /* Helper function, derived from David M. Gay's `g_xfmt()', calling
   * his `__gdtoa()' function in a manner to provide extended precision
   * replacements for `ecvt()' and `fcvt()'.
   */
  int k; unsigned int e = 0; char *ep;
  static FPI fpi = { 64, 1-16383-64+1, 32766-16383-64+1, FPI_Round_near, 0 };

  /* Classify the argument into an appropriate `__gdtoa()' category...
   */
  if( (k = __fpclassifyl( x.__pformat_fpreg_ldouble_t )) & FP_NAN )
    /*
     * identifying infinities or not-a-number...
     */
    k = (k & FP_NORMAL) ? STRTOG_Infinite : STRTOG_NaN;

  else if( k & FP_NORMAL )
  {
    /* normal and near-zero `denormals'...
     */
    if( k & FP_ZERO )
    {
      /* with appropriate exponent adjustment for a `denormal'...
       */
      k = STRTOG_Denormal;
      e = 1 - 0x3FFF - 63;
    }
    else
    {
      /* or with `normal' exponent adjustment...
       */
      k = STRTOG_Normal;
      e = (x.__pformat_fpreg_exponent & 0x7FFF) - 0x3FFF - 63;
    }
  }

  else
    /* or, if none of the above, it's a zero, (positive or negative).
     */
    k = STRTOG_Zero;

  /* Check for negative values, always treating NaN as unsigned...
   * (return value is zero for positive/unsigned; non-zero for negative).
   */
  *sign = (k == STRTOG_NaN) ? 0 : x.__pformat_fpreg_exponent & 0x8000;

  /* Finally, get the raw digit string, and radix point position index.
   */
  return __gdtoa( &fpi, e, &x.__pformat_fpreg_bits, &k, mode, nd, dp, &ep );
}

static __pformat_inline__
char *__pformat_ecvt( long double x, int precision, int *dp, int *sign )
{
  /* A convenience wrapper for the above...
   * it emulates `ecvt()', but takes a `long double' argument.
   */
  __pformat_fpreg_t z; z.__pformat_fpreg_ldouble_t = x;
  return __pformat_cvt( 2, z, precision, dp, sign );
}

static __pformat_inline__
char *__pformat_fcvt( long double x, int precision, int *dp, int *sign )
{
  /* A convenience wrapper for the above...
   * it emulates `fcvt()', but takes a `long double' argument.
   */
  __pformat_fpreg_t z; z.__pformat_fpreg_ldouble_t = x;
  return __pformat_cvt( 3, z, precision, dp, sign );
}

/* The following are required, to clean up the `__gdtoa()' memory pool,
 * after processing the data returned by the above.
 */
#define __pformat_ecvt_release( value ) __freedtoa( value )
#define __pformat_fcvt_release( value ) __freedtoa( value )

#else
/* TODO: remove this before final release; it is included here as a
 * convenience for testing, without requiring a working `__gdtoa()'.
 */
static __inline__
char *__pformat_ecvt( long double x, int precision, int *dp, int *sign )
{
  /* Define in terms of `ecvt()'...
   */
  char *retval = ecvt( (double)(x), precision, dp, sign );
  if( isinf( x ) || isnan( x ) )
  {
    /* emulating `__gdtoa()' reporting for infinities and NaN.
     */
    *dp = PFORMAT_INFNAN;
    if( *retval == '-' )
    {
      /* Need to force the `sign' flag, (particularly for NaN).
       */
      ++retval; *sign = 1;
    }
  }
  return retval;
}

static __inline__
char *__pformat_fcvt( long double x, int precision, int *dp, int *sign )
{
  /* Define in terms of `fcvt()'...
   */
  char *retval = fcvt( (double)(x), precision, dp, sign );
  if( isinf( x ) || isnan( x ) )
  {
    /* emulating `__gdtoa()' reporting for infinities and NaN.
     */
    *dp = PFORMAT_INFNAN;
    if( *retval == '-' )
    {
      /* Need to force the `sign' flag, (particularly for NaN).
       */
      ++retval; *sign = 1;
    }
  }
  return retval;
}

/* No memory pool clean up needed, for these emulated cases...
 */
#define __pformat_ecvt_release( value ) /* nothing to be done */
#define __pformat_fcvt_release( value ) /* nothing to be done */

/* TODO: end of conditional to be removed. */
#endif

static
void __pformat_emit_inf_or_nan( int sign, char *value, __pformat_t *stream )
{
  /* Helper to emit INF or NAN where a floating point value
   * resolves to one of these special states.
   */
  int i;
  char buf[4];
  char *p = buf;

  /* We use the string formatting helper to display INF/NAN,
   * but we don't want truncation if the precision set for the
   * original floating point output request was insufficient;
   * ignore it!
   */
  stream->precision = PFORMAT_IGNORE;

  if( sign )
    /*
     * Negative infinity: emit the sign...
     */
    *p++ = '-';

  else if( stream->flags & PFORMAT_POSITIVE )
    /*
     * Not negative infinity, but '+' flag is in effect;
     * thus, we emit a positive sign...
     */
    *p++ = '+';

  else if( stream->flags & PFORMAT_ADDSPACE )
    /*
     * No sign required, but space was reserved for it...
     */
    *p++ = '\x20';

  /* Copy the appropriate status indicator, up to a maximum of
   * three characters, transforming to the case corresponding to
   * the format specification...
   */
  for( i = 3; i > 0; --i )
    *p++ = (*value++ & ~PFORMAT_XCASE) | (stream->flags & PFORMAT_XCASE);

  /* and emit the result.
   */
  __pformat_putchars( buf, p - buf, stream );
}

static
int __pformat_adjust_for_grouping( int *len, __pformat_t *stream )
{
  /* Helper to compute the field width adjustment needed to
   * accommodate thousands digits group separator characters,
   * when these are requested for %f, %F, %g, and %G formats.
   */
  if( (*len > 0) && __pformat_enable_thousands_grouping( stream ) )
  {
    /* Only significant digits to the left of the radix point
     * are grouped; when "len" indicates that such digits are
     * present, and when grouping is enabled, then we evaluate
     * the grouping strategy, to determine how many separators
     * should be inserted, and adjust "len" to represent the
     * number of ungrouped digits will be left preceding the
     * leftmost separator.
     */
    char *grouping = stream->grouping;
    int groupsize = *grouping, groupcount = 0;
    while( (grouping != NULL) && (*len > groupsize) )
    {
      /* Account for possible variant group sizes...
       */
      ++groupcount; *len -= groupsize;
      groupsize = (grouping[1] != '\0') ? *++grouping : *grouping;

      /* ...noting that a group size of CHAR_MAX means that
       * all more significant digits should remain ungrouped.
       */
      if( groupsize == CHAR_MAX )
	grouping = NULL;
    }
    /* Return value is the number of group separators which
     * are to be inserted into the output field.
     */
    return groupcount;
  }
  /* If we get to here, there are no significant digits to be
   * placed to the left of the radix, or grouping has not been
   * enabled for this conversion; in either case, no adjustment
   * is required.
   */
  return 0;
}

static
void __pformat_emit_float( int sign, char *value, int len, __pformat_t *stream )
{
  /* Helper to emit a fixed point representation of numeric data,
   * as encoded by a prior call to `ecvt()' or `fcvt()'; (this does
   * NOT include the exponent, for floating point format).
   */
  int prefix, gc = 0;
  if( (prefix = len) > 0 )
  {
    /* The magnitude of `x' is greater than or equal to 1.0...
     * reserve space in the output field, for the required number of
     * decimal digits to be placed before the decimal point, including
     * any adjustment required to accommodate thousands digits group
     * separator characters...
     */
    gc = __pformat_adjust_for_grouping( &prefix, stream );
    if( stream->width > (len += gc) )
      /*
       * adjusting as appropriate, when width is sufficient...
       */
      stream->width -= len;

    else
      /* or simply ignoring the width specification, if not.
       */
      stream->width = PFORMAT_IGNORE;
  }

  else if( stream->width > 0 )
    /*
     * The magnitude of `x' is less than 1.0...
     * reserve space for exactly one zero before the decimal point.
     */
    stream->width--;

  /* Reserve additional space for the digits which will follow the
   * decimal point...
   */
  if( (stream->width >= 0) && (stream->width > stream->precision) )
    /*
     * adjusting appropriately, when sufficient width remains...
     * (note that we must check both of these conditions, because
     * precision may be more negative than width, as a result of
     * adjustment to provide extra padding when trailing zeros
     * are to be discarded from "%g" format conversion with a
     * specified field width, but if width itself is negative,
     * then there is explicitly to be no padding anyway).
     */
    stream->width -= stream->precision;

  else
    /* or again, ignoring the width specification, if not.
     */
    stream->width = PFORMAT_IGNORE;

  /* Reserve space in the output field, for display of the decimal point,
   * unless the precision is explicity zero, with the `#' flag not set.
   */
  if(  (stream->width > 0)
  &&  ((stream->precision > 0) || (stream->flags & PFORMAT_HASHED))  )
    stream->width--;

  /* Reserve space in the output field, for display of the sign of the
   * formatted value, if required; (i.e. if the value is negative, or if
   * either the `space' or `+' formatting flags are set).
   */
  if( (stream->width > 0) && (sign || (stream->flags & PFORMAT_SIGNED)) )
    stream->width--;

  /* Emit any padding space, as required to correctly right justify
   * the output within the alloted field width.
   */
  if( (stream->width > 0) && ((stream->flags & PFORMAT_JUSTIFY) == 0) )
    while( stream->width-- > 0 )
      __pformat_putc( '\x20', stream );

  /* Emit the sign indicator, as appropriate...
   */
  if( sign )
    /*
     * mandatory, for negative values...
     */
    __pformat_putc( '-', stream );

  else if( stream->flags & PFORMAT_POSITIVE )
    /*
     * optional, for positive values...
     */
    __pformat_putc( '+', stream );

  else if( stream->flags & PFORMAT_ADDSPACE )
    /*
     * or just fill reserved space, when the space flag is in effect.
     */
    __pformat_putc( '\x20', stream );

  /* If the `0' flag is in effect, and not overridden by the `-' flag,
   * then zero padding, to fill out the field, goes here...
   */
  if(  (stream->width > 0)
  &&  ((stream->flags & PFORMAT_JUSTIFY) == PFORMAT_ZEROFILL)  )
    while( stream->width-- > 0 )
      __pformat_putc( '0', stream );

  /* Emit the digits of the encoded numeric value...
   */
  if( len > 0 )
    /* ...beginning with those which precede the radix point,
     * and appending any necessary significant trailing zeros.
     */
    do { __pformat_putc( *value ? *value++ : '0', stream );
         if( (--prefix == 0) && (gc > 0) )
	 {
	   /* When thousands digits grouping has been enabled,
	    * emit group separators as directed by the grouping
	    * strategy for the current locale.
	    */
	   char *gp = stream->grouping; int c = gc--;
	   do prefix = *gp++;
	      while( (*gp != '\0') && (*gp != CHAR_MAX) && (--c > 0) );
	   __pformat_emit_digit( ',', stream ); --len;
	 }
       } while( --len > 0 );

  else
    /* The magnitude of the encoded value is less than 1.0, so no
     * digits precede the radix point; we emit a mandatory initial
     * zero, followed immediately by the radix point.
     */
    __pformat_putc( '0', stream );

  /* Unless the encoded value is integral, AND the radix point
   * is not expressly demanded by the `#' flag, we must insert
   * the appropriately localised radix point mark here...
   */
  if( (stream->precision > 0) || (stream->flags & PFORMAT_HASHED) )
    /*
     * ...treating it as a special case of digit emission, with
     * an internal representation equivalent to ASCII period.
     */
    __pformat_emit_digit( '.', stream );

  /* When the radix point offset, `len', is negative, this implies
   * that additional zeros must appear, following the radix point,
   * and preceding the first significant digit...
   */
  if( len < 0 )
  {
    /* To accommodate these, we adjust the precision, (reducing it
     * by adding a negative value), and then we emit as many zeros
     * as are required.
     */
    stream->precision += len;
    do __pformat_putc( '0', stream );
       while( ++len < 0 );
  }

  /* Now we emit any remaining significant digits, or trailing zeros,
   * until the required precision has been achieved.
   */
  while( stream->precision-- > 0 )
    __pformat_putc( *value ? *value++ : '0', stream );
}

static
void __pformat_emit_efloat( int sign, char *value, int e, __pformat_t *stream )
{
  /* Helper to emit a floating point representation of numeric data,
   * as encoded by a prior call to `ecvt()' or `fcvt()'; (this DOES
   * include the following exponent).
   */
  int exp_width = 1;
  __pformat_intarg_t exponent; exponent.__pformat_llong_t = e -= 1;

  /* POSIX explicitly specifies that behaviour is undefined, if the
   * user requests thousands digits grouping for any efloat format;
   * we simply overrule any such request.
   */
  stream->flags &= ~PFORMAT_GROUPED;

  /* Determine how many digit positions are required for the exponent.
   */
  while( (e /= 10) != 0 )
    exp_width++;

  /* Ensure that this is at least as many as the standard requirement.
   */
  if( exp_width < stream->expmin )
    exp_width = stream->expmin;

  /* Adjust the residual field width allocation, to allow for the
   * number of exponent digits to be emitted, together with a sign
   * and exponent separator...
   */
  if( stream->width > (exp_width += 2) )
    stream->width -= exp_width;

  else
    /* ignoring the field width specification, if insufficient.
     */
    stream->width = PFORMAT_IGNORE;

  /* Emit the significand, as a fixed point value with one digit
   * preceding the radix point.
   */
  __pformat_emit_float( sign, value, 1, stream );

  /* Reset precision, to ensure the mandatory minimum number of
   * exponent digits will be emitted, and set the flags to ensure
   * the sign is displayed.
   */
  stream->precision = stream->expmin;
  stream->flags |= PFORMAT_SIGNED;

  /* Emit the exponent separator.
   */
  __pformat_putc( ('E' | (stream->flags & PFORMAT_XCASE)), stream );

  /* Readjust the field width setting, such that it again allows
   * for the digits of the exponent, (which had been discounted when
   * computing any left side padding requirement), so that they are
   * correctly included in the computation of any right side padding
   * requirement, (but here we exclude the exponent separator, which
   * has been emitted, and so counted already).
   */
  stream->width += exp_width - 1;

  /* And finally, emit the exponent itself, as a signed integer,
   * with any padding required to achieve flush left justification,
   * (which will be added automatically, by `__pformat_int()').
   */
  __pformat_int( exponent, stream );
}

static
void __pformat_float( long double x, __pformat_t *stream )
{
  /* Handler for `%f' and `%F' format specifiers.
   *
   * This wraps calls to `__pformat_cvt()', `__pformat_emit_float()'
   * and `__pformat_emit_inf_or_nan()', as appropriate, to achieve
   * output in fixed point format.
   */
  int sign, intlen; char *value;

  /* Establish the precision for the displayed value, defaulting to six
   * digits following the decimal point, if not explicitly specified.
   */
  if( stream->precision < 0 )
    stream->precision = 6;

  /* Encode the input value as ASCII, for display...
   */
  value = __pformat_fcvt( x, stream->precision, &intlen, &sign );

  if( intlen == PFORMAT_INFNAN )
    /*
     * handle cases of `infinity' or `not-a-number'...
     */
    __pformat_emit_inf_or_nan( sign, value, stream );

  else
  { /* or otherwise, emit the formatted result.
     */
    __pformat_emit_float( sign, value, intlen, stream );

    /* and, if there is any residual field width as yet unfilled,
     * then we must be doing flush left justification, so pad out to
     * the right hand field boundary.
     */
    while( stream->width-- > 0 )
      __pformat_putc( '\x20', stream );
  }

  /* Clean up `__pformat_fcvt()' memory allocation for `value'...
   */
  __pformat_fcvt_release( value );
}

static
void __pformat_efloat( long double x, __pformat_t *stream )
{
  /* Handler for `%e' and `%E' format specifiers.
   *
   * This wraps calls to `__pformat_cvt()', `__pformat_emit_efloat()'
   * and `__pformat_emit_inf_or_nan()', as appropriate, to achieve
   * output in floating point format.
   */
  int sign, intlen; char *value;

  /* Establish the precision for the displayed value, defaulting to six
   * digits following the decimal point, if not explicitly specified.
   */
  if( stream->precision < 0 )
    stream->precision = 6;

  /* Encode the input value as ASCII, for display...
   */
  value = __pformat_ecvt( x, stream->precision + 1, &intlen, &sign );

  if( intlen == PFORMAT_INFNAN )
    /*
     * handle cases of `infinity' or `not-a-number'...
     */
    __pformat_emit_inf_or_nan( sign, value, stream );

  else
    /* or otherwise, emit the formatted result.
     */
    __pformat_emit_efloat( sign, value, intlen, stream );

  /* Clean up `__pformat_ecvt()' memory allocation for `value'...
   */
  __pformat_ecvt_release( value );
}

static
void __pformat_gfloat( long double x, __pformat_t *stream )
{
  /* Handler for `%g' and `%G' format specifiers.
   *
   * This wraps calls to `__pformat_cvt()', `__pformat_emit_float()',
   * `__pformat_emit_efloat()' and `__pformat_emit_inf_or_nan()', as
   * appropriate, to achieve output in the more suitable of either
   * fixed or floating point format.
   */
  int sign, intlen; char *value;

  /* Establish the precision for the displayed value, defaulting to
   * six significant digits, if not explicitly specified...
   */
  if( stream->precision < 0 )
    stream->precision = 6;

  /* or to a minimum of one digit, otherwise...
   */
  else if( stream->precision == 0 )
    stream->precision = 1;

  /* Encode the input value as ASCII, for display.
   */
  value = __pformat_ecvt( x, stream->precision, &intlen, &sign );

  if( intlen == PFORMAT_INFNAN )
    /*
     * Handle cases of `infinity' or `not-a-number'.
     */
    __pformat_emit_inf_or_nan( sign, value, stream );

  else if( (-4 < intlen) && (intlen <= stream->precision) )
  {
    /* Value lies in the acceptable range for fixed point output,
     * (i.e. the exponent is no less than minus four, and the number
     * of significant digits which precede the radix point is fewer
     * than the least number which would overflow the field width,
     * specified or implied by the established precision).
     */
    if( (stream->flags & PFORMAT_HASHED) == PFORMAT_HASHED )
      /*
       * The `#' flag is in effect...
       * Adjust precision to retain the specified number of significant
       * digits, with the proper number preceding the radix point, and
       * the balance following it...
       */
      stream->precision -= intlen;

    else
      /* The `#' flag is not in effect...
       * Here we adjust the precision to accommodate all digits which
       * precede the radix point, but we truncate any balance following
       * it, to suppress output of non-significant trailing zeros...
       */
      if( ((stream->precision = strlen( value ) - intlen) < 0)
        /*
	 * This may require a compensating adjustment to the field
	 * width, to accommodate significant trailing zeros, which
	 * precede the radix point...
	 */
      && (stream->width > 0)  )
	stream->width += stream->precision;

    /* Now, we format the result as any other fixed point value.
     */
    __pformat_emit_float( sign, value, intlen, stream );

    /* If there is any residual field width as yet unfilled, then
     * we must be doing flush left justification, so pad out to the
     * right hand field boundary.
     */
    while( stream->width-- > 0 )
      __pformat_putc( '\x20', stream );
  }

  else
  { /* Value lies outside the acceptable range for fixed point;
     * one significant digit will precede the radix point, so we
     * decrement the precision to retain only the appropriate number
     * of additional digits following it, when we emit the result
     * in floating point format.
     */
    if( (stream->flags & PFORMAT_HASHED) == PFORMAT_HASHED )
      /*
       * The `#' flag is in effect...
       * Adjust precision to emit the specified number of significant
       * digits, with one preceding the radix point, and the balance
       * following it, retaining any non-significant trailing zeros
       * which are required to exactly match the requested precision...
       */
      stream->precision--;

    else
      /* The `#' flag is not in effect...
       * Adjust precision to emit only significant digits, with one
       * preceding the radix point, and any others following it, but
       * suppressing non-significant trailing zeros...
       */
      stream->precision = strlen( value ) - 1;

    /* Now, we format the result as any other floating point value.
     */
    __pformat_emit_efloat( sign, value, intlen, stream );
  }

  /* Clean up `__pformat_ecvt()' memory allocation for `value'.
   */
  __pformat_ecvt_release( value );
}

static
void __pformat_emit_xfloat( __pformat_fpreg_t value, __pformat_t *stream )
{
  /* Helper for emitting floating point data, originating as
   * either `double' or `long double' type, as a hexadecimal
   * representation of the argument value.
   */
  char buf[18], *p = buf;
  __pformat_intarg_t exponent; short exp_width = 2;

  /* The mantissa field of the argument value representation can
   * accommodate at most 16 hexadecimal digits, of which one will
   * be placed before the radix point, leaving at most 15 digits
   * to satisfy any requested precision; thus...
   */
  if( value.__pformat_fpreg_mantissa )
  {
    /* ...provided the mantissa is non-zero...
     */
    if( (stream->precision >= 0) && (stream->precision < 15) )
    {
      /* ...when the user specifies a precision within this range,
       * we want to adjust the mantissa, to retain just the number
       * of digits required, rounding up when the high bit of the
       * leftmost discarded digit is set; (mask of 0x08 accounts
       * for exactly one digit discarded, shifting 4 bits per
       * digit, with up to 14 additional digits, to consume the
       * full availability of 15 precision digits).
       *
       * However, before we perform the rounding operation, we
       * normalise the mantissa, shifting it to the left by as many
       * bit positions may be necessary, until its highest order bit
       * is set, thus preserving the maximum number of bits in the
       * rounded result as possible.
       */
      while( value.__pformat_fpreg_mantissa < (LLONG_MAX + 1ULL) )
	value.__pformat_fpreg_mantissa <<= 1;

      /* We then shift the mantissa one bit position back to the
       * right, to guard against possible overflow when the rounding
       * adjustment is added.
       */
      value.__pformat_fpreg_mantissa >>= 1;

      /* We now add the rounding adjustment, noting that to keep the
       * 0x08 mask aligned with the shifted mantissa, we also need to
       * shift it right by one bit initially, changing its starting
       * value to 0x04...
       */
      value.__pformat_fpreg_mantissa += 0x04LL << (4 * (14 - stream->precision));
      if( (value.__pformat_fpreg_mantissa & (LLONG_MAX + 1ULL)) == 0ULL )
	/*
	 * When the rounding adjustment would not have overflowed,
	 * then we shift back to the left again, to fill the vacated
	 * bit we reserved to accommodate the carry.
	 */
	value.__pformat_fpreg_mantissa <<= 1;

      else
	/* Otherwise the rounding adjustment would have overflowed,
	 * so the carry has already filled the vacated bit; the effect
	 * of this is equivalent to an increment of the exponent.
	 */
	value.__pformat_fpreg_exponent++;

      /* We now complete the rounding to the required precision, by
       * shifting the unwanted digits out, from the right hand end of
       * the mantissa.
       */
      value.__pformat_fpreg_mantissa >>= 4 * (15 - stream->precision);
    }

    /* Encode the significant digits of the mantissa in hexadecimal
     * ASCII notation, ready for transfer to the output stream...
     */
    while( value.__pformat_fpreg_mantissa )
    {
      /* taking the rightmost digit in each pass...
       */
      int c = value.__pformat_fpreg_mantissa & 0xF;
      if( c == value.__pformat_fpreg_mantissa )
      {
	/* inserting the radix point, when we reach the last,
	 * (i.e. the most significant digit), unless we found no
	 * less significant digits, with no mandatory radix point
	 * inclusion, and no additional required precision...
	 */
	if( (p > buf)
	||  (stream->flags & PFORMAT_HASHED) || (stream->precision > 0)  )
	  /*
	   * Internally, we represent the radix point as an ASCII '.';
	   * we will replace it with any locale specific alternative,
	   * at the time of transfer to the ultimate destination.
	   */
	  *p++ = '.';

	/* If the most significant hexadecimal digit of the encoded
	 * output value is greater than one, then the indicated value
	 * will appear too large, by an additional binary exponent
	 * corresponding to the number of higher order bit positions
	 * which it occupies...
	 */
	while( value.__pformat_fpreg_mantissa > 1 )
	{
	  /* so reduce the exponent value to compensate...
	   */
	  value.__pformat_fpreg_exponent--;
	  value.__pformat_fpreg_mantissa >>= 1;
	}
      }

      else if( stream->precision > 0 )
	/*
	 * we have not yet fulfilled the desired precision,
	 * and we have not yet found the most significant digit,
	 * so account for the current digit, within the field
	 * width required to meet the specified precision.
	 */
	stream->precision--;

      if( (c > 0) || (p > buf) || (stream->precision >= 0) )
	/*
	 * Ignoring insignificant trailing zeros, (unless required to
	 * satisfy specified precision), store the current encoded digit
	 * into the pending output buffer, in LIFO order, and using the
	 * appropriate case for digits in the `A'..`F' range.
	 */
	*p++ = c > 9 ? (c - 10 + 'A') | (stream->flags & PFORMAT_XCASE) : c + '0';

      /* Shift out the current digit, (4-bit logical shift right),
       * to align the next more significant digit to be extracted,
       * and encoded in the next pass.
       */
      value.__pformat_fpreg_mantissa >>= 4;
    }
  }

  if( p == buf )
  {
    /* Nothing has been queued for output...
     * We need at least one zero, and possibly a radix point.
     */
    if( (stream->precision > 0) || (stream->flags & PFORMAT_HASHED) )
      *p++ = '.';

    *p++ = '0';
  }

  if( stream->width > 0 )
  {
  /* Adjust the user specified field width, to account for the
   * number of digits minimally required, to display the encoded
   * value, at the requested precision.
   *
   * FIXME: this uses the minimum number of digits possible for
   * representation of the binary exponent, in strict conformance
   * with C99 and POSIX specifications.  Although there appears to
   * be no Microsoft precedent for doing otherwise, we may wish to
   * relate this to the `_get_output_format()' result, to maintain
   * consistency with `%e', `%f' and `%g' styles.
   */
    int min_width = p - buf;
    int exponent = value.__pformat_fpreg_exponent;

    /* If we have not yet queued sufficient digits to fulfil the
     * requested precision, then we must adjust the minimum width
     * specification, to accommodate the additional digits which
     * are required to do so.
     */
    if( stream->precision > 0 )
      min_width += stream->precision;

    /* Adjust the minimum width requirement, to accomodate the
     * sign, radix indicator and at least one exponent digit...
     */
    min_width += stream->flags & PFORMAT_SIGNED ? 6 : 5;
    while( (exponent = exponent / 10) != 0 )
    {
      /* and increase as required, if additional exponent digits
       * are needed, also saving the exponent field width adjustment,
       * for later use when that is emitted.
       */
      min_width++;
      exp_width++;
    }

    if( stream->width > min_width )
    {
      /* When specified field width exceeds the minimum required,
       * adjust to retain only the excess...
       */
      stream->width -= min_width;

      /* and then emit any required left side padding spaces.
       */
      if( (stream->flags & PFORMAT_JUSTIFY) == 0 )
	while( stream->width-- > 0 )
	  __pformat_putc( '\x20', stream );
    }

    else
      /* Specified field width is insufficient; just ignore it!
       */
      stream->width = PFORMAT_IGNORE;
  }

  /* Emit the sign of the encoded value, as required...
   */
  if( stream->flags & PFORMAT_NEGATIVE )
    /*
     * this is mandatory, to indicate a negative value...
     */
    __pformat_putc( '-', stream );

  else if( stream->flags & PFORMAT_POSITIVE )
    /*
     * but this is optional, for a positive value...
     */
    __pformat_putc( '+', stream );

  else if( stream->flags & PFORMAT_ADDSPACE )
    /*
     * with this optional alternative.
     */
    __pformat_putc( '\x20', stream );

  /* Prefix a `0x' or `0X' radix indicator to the encoded value,
   * with case appropriate to the format specification.
   */
  __pformat_putc( '0', stream );
  __pformat_putc( 'X' | (stream->flags & PFORMAT_XCASE), stream );

  /* If the `0' flag is in effect...
   * Zero padding, to fill out the field, goes here...
   */
  if(  (stream->width > 0)
  &&  ((stream->flags & PFORMAT_JUSTIFY) == PFORMAT_ZEROFILL)  )
    while( stream->width-- > 0 )
      __pformat_putc( '0', stream );

  /* Next, we emit the encoded value, without its exponent...
   */
  while( p > buf )
    __pformat_emit_digit( *--p, stream );

  /* followed by any additional zeros needed to satisfy the
   * precision specification...
   */
  while( stream->precision-- > 0 )
    __pformat_putc( '0', stream );

  /* then the exponent prefix, (C99 and POSIX specify `p'),
   * in the case appropriate to the format specification...
   */
  __pformat_putc( 'P' | (stream->flags & PFORMAT_XCASE), stream );

  /* and finally, the decimal representation of the binary exponent,
   * as a signed value with mandatory sign displayed, in a field width
   * adjusted to accommodate it, LEFT justified, with any additional
   * right side padding remaining from the original field width.
   */
  stream->width += exp_width;
  stream->flags |= PFORMAT_SIGNED;
  exponent.__pformat_llong_t = value.__pformat_fpreg_exponent;
  __pformat_int( exponent, stream );
}

static
void __pformat_xldouble( long double x, __pformat_t *stream )
{
  /* Handler for `%La' and `%LA' format specifiers, (with argument
   * value specified as `long double' type).
   */
  unsigned sign_bit = 0;
  __pformat_fpreg_t z; z.__pformat_fpreg_ldouble_t = x;

  /* First check for NaN; it is emitted unsigned...
   */
  if( isnan( x ) )
    __pformat_emit_inf_or_nan( sign_bit, "NaN", stream );

  else
  { /* Capture the sign bit up-front, so we can show it correctly
     * even when the argument value is zero or infinite.
     */
    if( (sign_bit = (z.__pformat_fpreg_exponent & 0x8000)) != 0 )
      stream->flags |= PFORMAT_NEGATIVE;

    /* Check for infinity, (positive or negative)...
     */
    if( isinf( x ) )
      /*
       * displaying the appropriately signed indicator,
       * when appropriate.
       */
      __pformat_emit_inf_or_nan( sign_bit, "Inf", stream );

    else
    { /* The argument value is a representable number...
       * extract the effective value of the biased exponent...
       */
      z.__pformat_fpreg_exponent &= 0x7FFF;
      if( z.__pformat_fpreg_exponent == 0 )
      {
	/* A biased exponent value of zero means either a
	 * true zero value, if the mantissa field also has
	 * a zero value, otherwise...
	 */
	if( z.__pformat_fpreg_mantissa != 0 )
	{
	  /* ...this mantissa represents a subnormal value;
	   * adjust the exponent, while shifting the mantissa
	   * to the left, until its leading bit is 1.
	   */
	  z.__pformat_fpreg_exponent = 1-0x3FFF;
	  while( (z.__pformat_fpreg_mantissa & (LLONG_MAX + 1ULL)) == 0 )
	  {
	    z.__pformat_fpreg_mantissa <<= 1;
	    --z.__pformat_fpreg_exponent;
	  }
	}
      }
      else
	/* This argument represents a non-zero normal number;
	 * eliminate the bias from the exponent...
	 */
	z.__pformat_fpreg_exponent -= 0x3FFF;

      /* Finally, hand the adjusted representation off to the
       * generalised hexadecimal floating point format handler...
       */
      __pformat_emit_xfloat( z, stream );
    }
  }
}

static __pformat_inline__
/* Inline helper to accumulate a running total of successive
 * decimal digits, optimized to use bitwise shifts to multiply
 * the total of more significant digits by ten; (note that we
 * coerce negative totals to zero, since this implementation
 * never needs to accumulate negative values, but it must be
 * able to override an initial PFORMAT_IGNORE (-1) setting).
 */
int __pformat_imul10plus( int total, int units )
{ return units + ((total > 0) ? ((total + (total << 2)) << 1) : 0); }

static
int __pformat_read_arg_index( const char **fmt )
{
  /* Compute a positional argument index from a format string
   * reference of the form "%n$" or "*n$"; (the introducing '%'
   * or '*' character is assumed to have been identified already,
   * by the calling function, and the scan pointer should now be
   * pointing to the first digit of the prospective index value).
   * The returned index value is required to lie in the range
   * 1 .. NL_ARGMAX, otherwise zero is returned.
   */
  int index = 0;
  if( isdigit( **fmt ) )
    do { /* Scan the sequence of digits corresponding to "n", and
	  * interpret as a decimal number; use shifts to accumulate
	  * powers of ten, for increasingly significant digits.
	  */
	 if( (index = __pformat_imul10plus( index, **fmt - '0')) > NL_ARGMAX )
	 {
	   /* The accumulated total of the scanned digits exceeds
	    * NL_ARGMAX; skip any residual sequence, and bail out.
	    */
	   while( isdigit( *++*fmt ) );
	   return 0;
	 }
       } while( isdigit( *++*fmt ) );

  /* The scanned digit sequence must terminate with a '$',
   * otherwise it is not a valid index representation.
   */
  return (**fmt == '$') ? index : 0;
}

static __pformat_inline__
/* Depending on context, it may be necessary to advance the format scan
 * pointer, before interpreting the index value; this inline wrapper is
 * provided to facilitate compliance with this requirement.
 */
int __pformat_read_arg_index_after( const char **fmt )
{ ++*fmt; return __pformat_read_arg_index( fmt ); }

static
int __pformat_arg_index( const char **fmt )
{
  /* Interpret argument index references of the form "%n$" and "*m$",
   * within the format string; when a valid index (> 0) is identified,
   * advance the format string pointer beyond the delimiting '$' symbol
   * and return the index value, otherwise leave the format pointer as
   * it was on entry, and return zero.
   */
  const char *scan = *fmt;
  int arg_index = __pformat_read_arg_index( &scan );
  if( *scan++ == '$' ) *fmt = scan;
  return arg_index;
}

/* Note that the preceding function returns an argument index in the
 * range 1 .. NL_ARGMAX, (as stipulated by POSIX), but that we need it
 * as a zero-based index for array references; the following macro is
 * provided, for notational convenience, when making the adjustment.
 */
#define zero_adjusted(arg_index)  (--arg_index)

static __pformat_inline__
const char *__pformat_ignore_flags( const char *fmt )
{
  /* Advance the format string scan pointer, stepping over, and
   * otherwise ignoring any specified flag characters.
   */
  while( strchr( "+-' 0#", *fmt ) ) ++fmt;
  return fmt;
}

static
const char *__pformat_look_ahead( const char *fmt )
{
  /* Advance the format string scan pointer, stepping over, and
   * otherwise ignoring a field width or precision specification.
   */
  if( *fmt == '*' ) ++fmt; else while( isdigit( *fmt ) ) ++fmt;
  return fmt;
}

static __pformat_inline__
const char *__pformat_look_ahead_beyond_flags( const char *fmt )
{
  /* Advance the format string scan pointer, stepping over, and
   * otherwise ignoring any specified flag characters, or following
   * field width and precision specifications.
   */
  return (*(fmt = __pformat_look_ahead( __pformat_ignore_flags( fmt ))) == '.')
    ? __pformat_look_ahead( ++fmt ) : fmt;
}

static
__pformat_length_t __pformat_check_length_modifier( const char **fmt )
{
  /* Check for, and evaluate the effect of, argument length
   * modifiers, at the format scan pointer location; advance
   * the scan pointer to step over any such modifiers.
   */
  const char *check = *fmt;
  __pformat_length_t modifier = PFORMAT_LENGTH_DEFAULT;
  switch( *check++ )
  {
    case 'h': case 'l':
      /* The "h" and "l" modifiers are special, insofar as they
       * must support "hh" and "ll" variants...
       */
      if( *check == **fmt )
        switch( *check++ )
	{ /* ...thus, in the "hh" and "ll" cases, we need...
	   */
	  case 'h': modifier = PFORMAT_LENGTH_CHAR; break;
	  case 'l': modifier = PFORMAT_LENGTH_LLONG;
	}
      else switch( **fmt )
      { /* ...while the bare "h" and "l" cases give us...
	 */
	case 'h': modifier = PFORMAT_LENGTH_SHORT; break;
	case 'l': modifier = PFORMAT_LENGTH_LONG;
      }
      break;

    /* The remaining ISO-C/POSIX modifiers have only a single
     * character representation...
     */
    case 'j': modifier = __pformat_arg_length( intmax_t ); break;
    case 't': modifier = __pformat_arg_length( ptrdiff_t ); break;
    case 'z': modifier = __pformat_arg_length( size_t ); break;

    /* ...and "L" is returned "as is", anticipating evaluation
     * of its effect in the calling function.
     */
    case 'L': modifier = (__pformat_length_t)('L'); break;

    case 'I':
#   ifdef _WIN32
      /* Microsoft also uses "I32" or "I64", as non-standard length
       * modifiers; accommodate this anomaly, noting that, in each
       * case, the modifier consumes two characters beyond the "I".
       */
      check += 2;
      if( strncmp( "I32", *fmt, 3 ) == 0 )
      {
	/* Microsoft's "I32" modifier is equivalent to ISO-C's
	 * (and POSIX's) "l"...
	 */
	modifier = PFORMAT_LENGTH_LONG;
	break;
      }
      else if( strncmp( "I64", *fmt, 3 ) == 0 )
      {
	/* ...while their "I64" is equivalent to "ll"...
	 */
	modifier = PFORMAT_LENGTH_LLONG;
	break;
      }
      else
      { /* ...and their unqualified 'I' is a non-standard
	 * substitute for either 't' or 'z', (which they may
	 * not support at all); we advise against this usage,
	 * but wil accept it as (primarily) a reference to
	 * the 'ptrdiff_t' case.
	 */
	modifier = __pformat_arg_length( ptrdiff_t );
	++*fmt;
      }
#   endif

    /* When no modifier has been identified, reset the checking
     * pointer, to leave the format scan pointer unchanged.
     */
    default: check = *fmt;
  }
  /* Update the format scan pointer, to step over any identified
   * modifier characters, and return the modifier effect.
   */
  *fmt = check;
  return modifier;
}

static __pformat_inline__
/* A trivial wrapper function, to facilitate evaluation of length
 * modifiers for which the initial (or only) character has already
 * been stepped past by the format scanner.
 */
__pformat_length_t __pformat_length_modifier( const char **fmt )
{ --*fmt; return __pformat_check_length_modifier( fmt ); }

enum
{ /* Classification codes for the three fundamental conversion
   * type groups, which apply to printf() arguments, equivalent
   * to the return values from __pformat_is_conversion_type().
   */
  PFORMAT_TYPE_DOUBLE = 1,
  PFORMAT_TYPE_INTEGER,
  PFORMAT_TYPE_POINTER
};

static __pformat_inline__
int __pformat_is_conversion_type( int fmt_value )
{
  /* Confirm that a specified format string character value
   * matches one of the recognised conversion type codes; the
   * codes are specified within three groups: the first group
   * represents floating point types, the second are integer
   * types and the third (smaller group) are pointer types;
   * these correspond to the three classification groups,
   * as defined by the preceding enumeration.
   */
  const char *check, *valid_chars = "aAeEfFgG" "cCdiouxX" "npsS";
  return (check = strchr( valid_chars, fmt_value ))
    ? 1 + (check - valid_chars) / 8 : 0;
}

enum
{ /* The array indicies used to classify the content of the
   * following argument reference data type.
   */
  PFORMAT_CONVERSION_TYPE = 0,
  PFORMAT_LENGTH_MODIFIER,
  PFORMAT_ARGMAP_ENTRIES
};

typedef union
{ /* An aggregate data type, used to classify printf() arguments.
   */
  unsigned short  init;
  unsigned char   ref[PFORMAT_ARGMAP_ENTRIES];
} __pformat_argmap_t;

static __pformat_inline__
int __pformat_is_alt_ldouble_modifier( int length )
#ifdef _WIN32
{ /* Helper routine, to determine if an 'l' length modifier is
   * to be treated as equivalent to 'L', as it is in MSVCRT.DLL's
   * implementation of the printf() functions.
   */
  return (__mingw_output_format_flags & _MSVC_PRINTF_QUIRKS)
    ? (length == PFORMAT_LENGTH_LONG) : 0;
}
#else
{ /* For any non-windows build, there is no reason for it to be so.
   */
  return 0;
}
#endif

static __pformat_inline__
int __pformat_is_ldouble( int length )
{
  /* Helper routine to identify an argument length modifier, as a
   * reference to the 'long double' type, whether explicitly by use
   * of the ISO-C 'L' modifier, or by non-standard Microsoft usage
   * of 'l', when explicitly allowed by the user.
   */
  return (length == 'L') || __pformat_is_alt_ldouble_modifier( length );
}

static __pformat_inline__
int __pformat_indexed_argc( const char *fmt )
{
  /* Pre-scan the format string, and evaluate it as a potential
   * candidate for "%n$"/"*m$" positional argument addressing; set
   * the anticipated argument count for this style of processing.
   */
  int index, argc = 0;

  do { /* Scan the format string, character by character, to identify
	* any conversion specifications which may be present.
	*/
       if( *fmt == '%' )
       { /* When a candidate conversion specification is located, save
	  * its starting scan position, in case we subsequently reject
	  * it, and need to back-track.
	  */
	 const char *backtrack = fmt++;

	 /* Attempt to extract a positional argument reference index
	  * from this conversion specification.
	  */
	 if( (index = __pformat_read_arg_index( &fmt )) == 0 )
	 {
	   /* Unless this is a "%%" specification, (or an obfuscated
	    * variant of this), then we have identified a conversion
	    * specification which does not comply with the rules for
	    * positional argument indexing...
	    */
	   if( *fmt == '$' )
	     /* This is an explicit indexed parameter reference, but
	      * the index value is either explicitly zero, (which is
	      * not a valid index), or it exceeds NL_ARGMAX, (which
	      * is similarly invalid); bail out immediately.
	      */
	     return 0;

	   /* In this case, it appears that it may be just a regular
	    * conversion specification, (which isn't compatible with
	    * use of positional indexing elsewhere within the format
	    * string), but we do need to look ahead, because it may
	    * be the "%%" case, (which is allowed); first, skip over
	    * any flags, width, or precision specifications...
	    */
	   fmt = __pformat_look_ahead_beyond_flags( fmt );

	   /* ...then ignore any argument length modifiers; (ignore
	    * any numeric field here, since it would indicate use of
	    * a positionally indexed field width specification, which
	    * should not be used in this context, so we will catch it
	    * later, as unexpected, and thus initiate backtrack.
	    */
	   (void)__pformat_check_length_modifier( &fmt );

	   /* If all is valid so far, we should now be looking at the
	    * terminal conversion type specifier; if this is also valid,
	    * and is not the "%" specifier...
	    */
	   if( __pformat_is_conversion_type( *fmt ) )
	   {
	     /* ...then we may immediately reject this format string,
	      * because it is not compatible with the requirements for
	      * indexing of positional arguments.
	      */
	     return 0;
	   }

	   /* If we're still here, we haven't rejected this entire
	    * format string out of hand; the current specification may
	    * represent (some variation of) "%%"...
	    */
	   if( *fmt != '%' )
	   { /* ...or it may be malformed, in which case we back-track
	      * and attempt to recover.
	      */
	     fmt = backtrack;
	   }
	 }
	 else
	 { /* This conversion specification includes a valid positional
	    * argument index; we should still subject it to a look-ahead
	    * validation scan, which may also serve to capture any other
	    * indexed argument references, such as may be present within
	    * its field width and/or precision specifications.
	    */
	   int subindex;

	   /* At this point, "fmt" refers to the '$' symbol at the end
	    * of the argument index specification; advance it past that,
	    * and any following flags, to locate a possible field width
	    * specification, which may be...
	    */
	   if( *(fmt = __pformat_ignore_flags( ++fmt )) == '*' )
	   {
	     /* ...an indirect specification, resolved by reference to
	      * a passed argument, in which case this argument reference
	      * must also be indexed; bail out if it isn't...
	      */
	     if( (subindex = __pformat_read_arg_index_after( &fmt )) == 0 )
	       return 0;

	     /* ...but, record it in case its index value may be the
	      * greatest seen so far...
	      */
	     if( subindex > index )
	       index = subindex;

	     /* ...and advance the scan beyond its terminal '$' symbol.
	      */
	     ++fmt;
	   }
	   /* Alternatively, a field width may be direcly specified, as
	    * a sequence of numeric digits, in which case we may simply
	    * advance our scan beyond it.
	    */
	   else while( isdigit( *fmt ) )
	     ++fmt;

	   /* Any field width specification, even if omitted, and thus
	    * represented as a zero-length entity, may be followed by a
	    * precision specification; if present, this will always be
	    * introduced by a single '.'...
	    */
	   if( *fmt == '.' )
	   { /* ...which, when present, may also be represented as...
	      */
	     if( *++fmt == '*' )
	     { /* ...an indirect argument reference, subject to the same
		* constraints and processing requirements as apply in the
		* case of an indexed field width...
		*/
	       if( (subindex = __pformat_read_arg_index_after( &fmt )) == 0 )
		 return 0;
	       if( subindex > index )
		 index = subindex;
	       ++fmt;
	     }
	     /* ...or alternatively, as a directly specified sequence of
	      * numeric digits, which we may also step over.
	      */
	     else while( isdigit( *fmt ) )
	       ++fmt;
	   }

	   /* If we are still here, then "fmt" will be pointing at any
	    * argument length modifiers which may be present; step over
	    * any which are...
	    */
	   (void)__pformat_check_length_modifier( &fmt );

	   /* ...and we should now have a pointer to the conversion
	    * type specifier, which we may now validate.
	    */
	   if( __pformat_is_conversion_type( *fmt ) )
	   {
	     /* We've successfully verified that the current conversion
	      * specification is a suitable candidate for participation
	      * in format references to positional arguments; update the
	      * current indexed argument count, to reflect the largest
	      * index which has been seen so far...
	      */
	     if( index > argc ) argc = index;
	   }
	   /* ...otherwise, we may have (some variant of) "%%"...
	    */
	   else if( *fmt != '%' )
	   {
	     /* ...or a malformed specification, in which case we choose
	      * to back-track, and attempt recovery.
	      */
	     fmt = backtrack;
	   }
	 }
       }
       /* Unless we've already bailed out, because we've deemed that the
	* current format string is unsuitable for indexing of positional
	* arguments, continue scanning it.
	*/
     } while( *fmt++ );

  /* Finally, when the format string has been validated for indexing of
   * positional arguments, return the anticipated argument count, deduced
   * as the maximum valid index value seen.
   */
  return argc;
}

static
size_t __pformat_sizeof_argument( unsigned char *map )
{
  /* Classify printf() arguments by storage size, based on
   * the content of the 'ref' fields of a '__pformat_argmap_t'.
   */
  switch( __pformat_is_conversion_type( map[PFORMAT_CONVERSION_TYPE] ) )
  {
    case PFORMAT_TYPE_DOUBLE:
      /* All floating point arguments are promoted to 'double'
       * type, unless explicitly specified as 'long double'.
       */
      return (__pformat_is_ldouble( map[PFORMAT_LENGTH_MODIFIER] ))
	? sizeof( long double ) : sizeof( double );

    case PFORMAT_TYPE_POINTER:
      /* All pointer types share a common size, as categorized
       * by the generic 'void *'.
       */
      return sizeof( void * );

    case PFORMAT_TYPE_INTEGER:
      /* Integer data types may be explicitly qualified to be
       * 'long' or 'long long'...
       */
      switch( map[PFORMAT_LENGTH_MODIFIER] )
      { case PFORMAT_LENGTH_LONG: return sizeof( long );
	case PFORMAT_LENGTH_LLONG: return sizeof( long long );
      }
  }
  /* ...but anything which is not otherwise classified, may be
   * assumed to have the size of a generic 'int', (to which any
   * smaller data type is always promoted).
   */
  return sizeof( int );
}

static
int __pformat_argmap( int argc, const char *fmt, __pformat_argmap_t *map )
{
  /* Construct a classification map for the set of indexed arguments,
   * which have been identified by an initial format string scan, (as
   * performed by a prior invocation of the __pformat_indexed_argc()
   * function); begin by initialising the entire mapping array to the
   * 'unclassified' state, (i.e. set all elements to zero).
   */
  int index; for( index = 0; index < argc; index++ ) map[index].init = 0;

  do { /* Scan the format string again, character-by-character, and for
	* each possible conversion specification found...
	*/
       if( *fmt == '%' )
       { /* ...save the starting point, in case this is a false match,
	  * and we need to back-track, then attempt to read the index
	  * for the associated argument, which we expect to be present.
	  */
	 const char *backtrack = fmt++;
	 if( (index = __pformat_read_arg_index( &fmt )) > 0 )
	 {
	   /* We successfully read an index value; set aside space to
	    * identify the associated conversion type, to capture any
	    * additional indices for dynamic field width or precision
	    * references, and assume default argument size.
	    */
	   int format, subindex[2] = { 0, 0 };
	   __pformat_length_t length = PFORMAT_LENGTH_DEFAULT;

	   /* We don't care what format control flags may be present,
	    * so step over them, to check for a possible output field
	    * width specification...
	    */
	   if( *(fmt = __pformat_ignore_flags( ++fmt )) == '*' )
	   {
	     /* ...and, when one is found to be specified dynamically,
	      * attempt to read a (required) argument index, which is
	      * to be associated with it...
	      */
	     if( (subindex[0] = __pformat_read_arg_index_after( &fmt )) == 0 )
	       /*
		* ...or mark the entire conversion specification as
		* being improperly indexed.
		*/
	       index = 0;

	     else
	       /* We read an acceptable index value; the format scan
		* pointer now refers to its terminal '$' symbol, so we
		* step over it before we continue.
		*/
	       ++fmt;
	   }
	   else
	     /* If there is a field width specification present at all,
	      * then it is specified as a static digit string; we don't
	      * care what it is, so step over it.
	      */
	     while( isdigit( *fmt ) ) ++fmt;

	   if( *fmt == '.' )
	   { /* This indicates that we have a precision specification...
	      */
	     if( *++fmt == '*' )
	     {
	       /* ...and that is dynamically specified, so we must read
		* an argument reference index to associate with it...
		*/
	       if( (subindex[1] = __pformat_read_arg_index_after( &fmt )) == 0 )
		 /*
		  * ...otherwise, we again invalidate the containing
		  * conversion specification...
		  */
		 index = 0;

	       else
		 /* ...or we step over the terminal '$' of a valid
		  * index value.
		  */
		 ++fmt;
	     }
	     else
	       /* This precision specification is represented by a
		* statically specified digit sequence; once again, we
		* don't care what it is, so step over it.
		*/
	       while( isdigit( *fmt ) ) ++fmt;
	   }

	   /* By now, the format scan pointer will be referring to any
	    * argument length modifier which may be present; this must
	    * be recorded in our argument map, so classify it.
	    */
	   switch( length = __pformat_check_length_modifier( &fmt ) )
	   {
	     /* In the phase of execution when this is invoked, while
	      * evaluating the space occupied by the arguments, we know
	      * that all smaller than 'int' will be promoted to 'int'
	      * as the default minimum size; thus, we assess those
	      * of smaller size as having default length.
	      */
	     case PFORMAT_LENGTH_CHAR:
	     case PFORMAT_LENGTH_SHORT:
	       length = PFORMAT_LENGTH_DEFAULT;

	     /* All other length modifier codes are left unchanged; we
	      * provide a "do-nothing" default case handler here, to
	      * avoid GCC -Wswitch compile-time warnings.
	      */
	     default: break;
	   }

	   /* All of the indicies associated with the current conversion
	    * specification have now been checked, and when valid, only a
	    * final check on the conversion type itself remains...
	    */
	   if(  (index > 0)
	   &&  ((format = __pformat_is_conversion_type( *fmt )) > 0)  )
	   {
	     /* ...before we record the classification data for this
	      * argument into its appropriate slot in the mapping array;
	      * note that we take care to update each mapping entry, to
	      * record the maximum size represented by any reference to
	      * the argument at the indexed position...
	      */
	     unsigned char argmap[PFORMAT_ARGMAP_ENTRIES] = { *fmt, length };
	     size_t argsize = __pformat_sizeof_argument( argmap );
	     if( (map[--index].init == 0)
	     ||  (argsize > __pformat_sizeof_argument( map[index].ref ))  )
	     {
	       /* ...recording both the conversion type classification,
		* and any associated argument length modifier.
		*/
	       map[index].ref[PFORMAT_CONVERSION_TYPE] = (unsigned char)(format);
	       map[index].ref[PFORMAT_LENGTH_MODIFIER] = (unsigned char)(length);
	     }
	     /* Similarly, for each of the output field width and the
	      * precision specifications, if either has been specified
	      * dynamically, we record the associated indices as being
	      * associated with (at least) an 'int' argument; (notice
	      * that we don't update any previously assigned map data
	      * here, because anything which was previously assigned
	      * must already be suitable as an 'int' reference.
	      */
	     for( index = 0; index < 2; index++ )
	       if( (subindex[index]-- > 0) && (map[subindex[index]].init == 0) )
		 map[subindex[index]].init = (unsigned char)('d');
	   }
	   else
	     /* When the conversion specification cannot be successfully
	      * verified, it should either represent an (effective) "%%",
	      * or we must back-track.
	      */
	     if( *fmt != '%' ) fmt = backtrack;
	 }
	 else
	 { /* Since "fmt" has already been validated for participation
	    * in indexed argument processing, we can only get to here if
	    * this is an effective "%%" specification, or it is so badly
	    * malformed that it is unrecognizable as a valid conversion
	    * specification; step over any flags, width and/or precision
	    * specifications, and length modifiers, to determine which
	    * of these applies, back-tracking when malformed.
	    */
	   fmt = __pformat_look_ahead_beyond_flags( fmt );
	   (void)__pformat_check_length_modifier( &fmt );
	   if( *fmt != '%' ) fmt = backtrack;
	 }
       }
     } while( *fmt++ ); /* Continue character-by-character scan. */

  /* Finally, we are able to verify the one remaining criterion, as yet
   * unchecked, which is a prerequisite for successful indexed processing
   * of printf() arguments: the specified indices, when serially sorted,
   * must represent a continuously numbered sequence, starting from one.
   * with no omissions; this corresponds to a fully populated map array,
   * with no residual zero entries...
   */
  for( index = 0; index < argc; index++ )
    /* so, in the event that we find any such unpopulated entry, we must
     * ultimately reject the specified format string, as being viable for
     * indexed processing of arguments...
     */
    if( map[index].init == 0 ) argc = 0;

  /* ...and our final argument count either remains unchanged, or has been
   * reset to zero to indicate an ultimately non-viable format string.
   */
  return argc;
}

int __pformat( int flags, void *dest, int max, const char *fmt, va_list args )
{
  int c, argc;

  __pformat_t stream =
  { /* Create and initialise a format control block
     * for this output request.
     */
    dest,					/* output goes to here        */
    flags &= PFORMAT_TO_FILE | PFORMAT_NOLIMIT,	/* only these valid initially */
    PFORMAT_IGNORE,				/* no field width yet         */
    PFORMAT_IGNORE,				/* nor any precision spec     */
    PFORMAT_RPINIT,				/* radix point uninitialised  */
    (wchar_t)(0),				/* leave it unspecified       */
    0,						/* zero output char count     */
    max,					/* establish output limit     */
    PFORMAT_MINEXP,				/* exponent chars preferred   */
    PFORMAT_RPINIT,				/* thou' sep uninitialised    */
    (wchar_t)(0),				/* leave it unspecified ...   */
    NULL					/* with no grouping counts    */
  };

  /* Establish a variant argument resource pool, to support processing of
   * the passed-in argument vector in either sequential or random order.
   */
  va_list argv, argv_indexed[argc = __pformat_indexed_argc( fmt )];
  if( argc > 0 )
  { /* All argument references, within the format string, are specified in
     * the "%n$" or "*m$" positionally indexed style; construct an argument
     * classification table, verify continuity of the index sequence...
     */
    __pformat_argmap_t specs[argc];
    if( (argc = __pformat_argmap( argc, fmt, specs )) > 0 )
    {
      /* ...and, on successful classification with no discontinuities, set
       * up a temporary local copy of the passed-in argument vector, which
       * we may then prescan...
       */
      va_copy( argv, args );
      for( c = 0; c < argc; c++ )
      { /* ...storing an indexed reference to the starting boundary of each
	 * argument, before advancing to the next argument boundary...
	 */
	va_copy( argv_indexed[c], argv );
	switch( specs[c].ref[PFORMAT_CONVERSION_TYPE] )
	{
	  /* ...noting that, regardless of conversion type, we don't need
	   * the argument values at this prescan phase, (so we may simply
	   * discard them), but we do need to adjust the va_list reference
	   * pointer as appropriate to the conversion type; thus...
	   */
	  case PFORMAT_TYPE_DOUBLE:
	    /* ...for floating point types, we must discriminate between
	     * 'double' and 'long double', (but not 'float', because that
	     * is always promoted to 'double')...
	     */
	    if( __pformat_is_ldouble( specs[c].ref[PFORMAT_LENGTH_MODIFIER] ) )
	      (void)(va_arg( argv, long double ));
	    else (void)(va_arg( argv, double ));
	    break;

	  case PFORMAT_TYPE_POINTER:
	    /* ...all pointer types need identically the same adjustment
	     * as the generic 'void *' pointer...
	     */
	    (void)(va_arg( argv, void * ));
	    break;

	  case PFORMAT_TYPE_INTEGER: default:
	    /* ...and all other types may be effectively be considered to
	     * be represented as integer types, for which...
	     */
	    switch( specs[c].ref[PFORMAT_LENGTH_MODIFIER] )
	    { case PFORMAT_LENGTH_LONG: (void)(va_arg( argv, long )); break;
	      case PFORMAT_LENGTH_LLONG: (void)(va_arg( argv, long long ));
		break;
	      default: (void)(va_arg( argv, int ));
	    }
	}
      }
      /* Terminate processing of the temporary copy of the argument vector,
       * in preparation for its immediate reinitialisation.
       */
      va_end( argv );
    }
  }
  /* Initialise (or reinitialise) a local copy of the passed-in argument
   * vector, such that it will be suitable for sequential processing of the
   * arguments, while still allowing for it to be reinitialised, as may be
   * required for processing the arguments in indexed random order.
   */
  va_copy( argv, args );

  format_scan: while( (c = *fmt++) != 0 )
  {
    /* Format string parsing loop...
     * The entry point is labelled, so that we can return to the start state
     * from within the inner `conversion specification' interpretation loop,
     * as soon as a conversion specification has been resolved.
     */
    if( c == '%' )
    {
      /* Initiate parsing of a `conversion specification'...
       */
      __pformat_intarg_t argval;
      __pformat_state_t  state = PFORMAT_INIT;
      __pformat_length_t length = PFORMAT_LENGTH_INT;

      /* Save the current format scan position, so that we can backtrack
       * in the event of encountering an invalid format specification...
       */
      const char *backtrack = fmt;

      /* If random order processing of arguments is specified by the
       * current format string, retrieve the index associated with the
       * current conversion specification.
       */
      int arg_index;
      if( (argc > 0) && ((arg_index = __pformat_arg_index( &fmt )) > 0) )
      {
	/* Having established the appropriate index value, we may clear
	 * the current assignment for the active argv reference, and then
	 * reassign it to refer to the corresponding indexed argument.
	 */
	va_end( argv );
	va_copy( argv, argv_indexed[zero_adjusted(arg_index)] );
      }

      /* Restart capture for dynamic field width and precision specs...
       */
      int *width_spec = &stream.width;

      /* Reset initial state for flags, width and precision specs...
       */
      stream.flags = flags;
      stream.width = stream.precision = PFORMAT_IGNORE;

      while( *fmt )
      { switch( c = *fmt++ )
	{
	  /* Data type specifiers...
	   * All are terminal, so exit the conversion spec parsing loop
	   * with a `goto format_scan', thus resuming at the outer level
	   * in the regular format string parser.
	   */
	  case '%':
	    /* Not strictly a data type specifier...
	     * it simply converts as a literal `%' character.
	     *
	     * FIXME: should we require this to IMMEDIATELY follow the
	     * initial `%' of the "conversion spec"?  (glibc `printf()'
	     * on GNU/Linux does NOT appear to require this, but POSIX
	     * and SUSv3 do seem to demand it).
	     */
	    __pformat_putc( c, &stream );
	    goto format_scan;

	  case 'C':
	    /* Equivalent to `%lc'; set `length' accordingly,
	     * and simply fall through.
	     */
	    length = PFORMAT_LENGTH_LONG;

	  case 'c':
	    /* Single, (or single multibyte), character output...
	     *
	     * We handle these by copying the argument into our local
	     * `argval' buffer, and then we pass the address of that to
	     * either `__pformat_putchars()' or `__pformat_wputchars()',
	     * as appropriate, effectively formatting it as a string of
	     * the appropriate type, with a length of one.
	     *
	     * A side effect of this method of handling character data
	     * is that, if the user sets a precision of zero, then no
	     * character is actually emitted; we don't want that, so we
	     * forcibly override any user specified precision.
	     */
	    stream.precision = PFORMAT_IGNORE;

	    /* Now we invoke the appropriate format handler...
	     */
	    if( (length == PFORMAT_LENGTH_LONG)
	    ||  (length == PFORMAT_LENGTH_LLONG)  )
	    {
	      /* considering any `long' type modifier as a reference to
	       * `wchar_t' data, (which is promoted to an `int' argument)...
	       */
	      wchar_t argval = (wchar_t)(va_arg( argv, int ));
	      __pformat_wputchars( &argval, 1, &stream );
	    }

	    else
	    { /* while anything else is simply taken as `char', (which
	       * is also promoted to an `int' argument)...
	       */
	      argval.__pformat_uchar_t = (unsigned char)(va_arg( argv, int ));
	      __pformat_putchars( (char *)(&argval), 1, &stream );
	    }
	    goto format_scan;

	  case 'S':
	    /* Equivalent to `%ls'; set `length' accordingly,
	     * and simply fall through.
	     */
	    length = PFORMAT_LENGTH_LONG;

	  case 's':
	    if( (length == PFORMAT_LENGTH_LONG)
	    ||  (length == PFORMAT_LENGTH_LLONG)  )
	    {
	      /* considering any `long' type modifier as a reference to
	       * a `wchar_t' string...
	       */
	      __pformat_wcputs( va_arg( argv, wchar_t * ), &stream );
	    }
	    else
	      /* This is normal string output;
	       * we simply invoke the appropriate handler...
	       */
	      __pformat_puts( va_arg( argv, char * ), &stream );

	    goto format_scan;

	  case 'o': case 'u': case 'x': case 'X':
	    /* Unsigned integer values; octal, decimal or hexadecimal format...
	     */
	    if( length == PFORMAT_LENGTH_LLONG )
	      /*
	       * with an `unsigned long long' argument, which we
	       * process `as is'...
	       */
	      argval.__pformat_ullong_t = va_arg( argv, unsigned long long );

	    else if( length == PFORMAT_LENGTH_LONG )
	      /*
	       * or with an `unsigned long', which we promote to
	       * `unsigned long long'...
	       */
	      argval.__pformat_ullong_t = va_arg( argv, unsigned long );

	    else
	    { /* or for any other size, which will have been promoted
	       * to `unsigned int', we select only the appropriately sized
	       * least significant segment, and again promote to the same
	       * size as `unsigned long long'...
	       */
	      argval.__pformat_ullong_t = va_arg( argv, unsigned int );
	      if( length == PFORMAT_LENGTH_SHORT )
		/*
		 * from `unsigned short'...
		 */
		argval.__pformat_ullong_t = argval.__pformat_ushort_t;

	      else if( length == PFORMAT_LENGTH_CHAR )
		/*
		 * or even from `unsigned char'...
		 */
		argval.__pformat_ullong_t = argval.__pformat_uchar_t;
	    }

	    /* so we can pass any size of argument to either of two
	     * common format handlers...
	     */
	    if( c == 'u' )
	      /* depending on whether output is to be encoded in
	       * decimal format...
	       */
	      __pformat_int( argval, &stream );

	    else
	      /* or in octal or hexadecimal format...
	       */
	      __pformat_xint( c, argval, &stream );

	    goto format_scan;

	  case 'd': case 'i':
	    /* Signed integer values; decimal format...
	     * This is similar to `u', but must process `argval' as signed,
	     * and be prepared to handle negative numbers.
	     */
	    stream.flags |= PFORMAT_NEGATIVE;

	    if( length == PFORMAT_LENGTH_LLONG )
	      /*
	       * The argument is a `long long' type...
	       */
	      argval.__pformat_llong_t = va_arg( argv, long long );

	    else if( length == PFORMAT_LENGTH_LONG )
	      /*
	       * or here, a `long' type...
	       */
	      argval.__pformat_llong_t = va_arg( argv, long );

	    else
	    { /* otherwise, it's an `int' type...
	       */
	      argval.__pformat_llong_t = va_arg( argv, int );
	      if( length == PFORMAT_LENGTH_SHORT )
		/*
		 * but it was promoted from a `short' type...
		 */
		argval.__pformat_llong_t = argval.__pformat_short_t;
	      else if( length == PFORMAT_LENGTH_CHAR )
		/*
		 * or even from a `char' type...
		 */
		argval.__pformat_llong_t = argval.__pformat_char_t;
	    }

	    /* In any case, all share a common handler...
	     */
	    __pformat_int( argval, &stream );
	    goto format_scan;

	  case 'p':
	    /* Pointer argument; format as hexadecimal, subject to...
	     */
	    if( (state == PFORMAT_INIT) && (stream.flags == flags) )
	    {
	      /* Here, the user didn't specify any particular
	       * formatting attributes.  We must choose a default
	       * which will be compatible with Microsoft's (broken)
	       * scanf() implementation, (i.e. matching the default
	       * used by MSVCRT's printf(), which appears to resemble
	       * "%0.8X" for 32-bit pointers); in particular, we MUST
	       * NOT adopt a GNU-like format resembling "%#x", because
	       * Microsoft's scanf() will choke on the "0x" prefix.
	       */
	      stream.flags |= PFORMAT_ZEROFILL;
	      stream.precision = 2 * sizeof( uintptr_t );
	    }
	    argval.__pformat_ullong_t = va_arg( argv, uintptr_t );
	    __pformat_xint( 'x', argval, &stream );
	    goto format_scan;

	  case 'e':
	    /* Floating point format, with lower case exponent indicator
	     * and lower case `inf' or `nan' representation when required;
	     * select lower case mode, and simply fall through...
	     */
	    stream.flags |= PFORMAT_XCASE;

	  case 'E':
	    /* Floating point format, with upper case exponent indicator
	     * and upper case `INF' or `NAN' representation when required,
	     * (or lower case for all of these, on fall through from above);
	     * select lower case mode, and simply fall through...
	     */
	    if( stream.flags & PFORMAT_LDOUBLE )
	      /*
	       * for a `long double' argument...
	       */
	      __pformat_efloat( va_arg( argv, long double ), &stream );

	    else
	      /* or just a `double', which we promote to `long double',
	       * so the two may share a common format handler.
	       */
	      __pformat_efloat( (long double)(va_arg( argv, double )), &stream );

	    goto format_scan;

	  case 'f':
	    /* Fixed point format, using lower case for `inf' and
	     * `nan', when appropriate; select lower case mode, and
	     * simply fall through...
	     */
	    stream.flags |= PFORMAT_XCASE;

	  case 'F':
	    /* Fixed case format using upper case, or lower case on
	     * fall through from above, for `INF' and `NAN'...
	     */
	    if( stream.flags & PFORMAT_LDOUBLE )
	      /*
	       * for a `long double' argument...
	       */
	      __pformat_float( va_arg( argv, long double ), &stream );

	    else
	      /* or just a `double', which we promote to `long double',
	       * so the two may share a common format handler.
	       */
	      __pformat_float( (long double)(va_arg( argv, double )), &stream );

	    goto format_scan;

	  case 'g':
	    /* Generalised floating point format, with lower case
	     * exponent indicator when required; select lower case
	     * mode, and simply fall through...
	     */
	    stream.flags |= PFORMAT_XCASE;

	  case 'G':
	    /* Generalised floating point format, with upper case,
	     * or on fall through from above, with lower case exponent
	     * indicator when required...
	     */
	    if( stream.flags & PFORMAT_LDOUBLE )
	      /*
	       * for a `long double' argument...
	       */
	      __pformat_gfloat( va_arg( argv, long double ), &stream );

	    else
	      /* or just a `double', which we promote to `long double',
	       * so the two may share a common format handler.
	       */
	      __pformat_gfloat( (long double)(va_arg( argv, double )), &stream );

	    goto format_scan;

	  case 'a':
	    /* Hexadecimal floating point format, with lower case radix
	     * and exponent indicators; select the lower case mode, and
	     * fall through...
	     */
	    stream.flags |= PFORMAT_XCASE;

	  case 'A':
	    /* Hexadecimal floating point format; handles radix and
	     * exponent indicators in either upper or lower case...
	     */
	    if( stream.flags & PFORMAT_LDOUBLE )
	      /*
	       * with a `long double' argument...
	       */
	      __pformat_xldouble( va_arg( argv, long double ), &stream );

	    else
	      /* or just a `double'.
	       */
	      __pformat_xldouble( (long double)(va_arg( argv, double )), &stream );

	    goto format_scan;

	  case 'n':
	    /* Save current output character count...
	     */
	    if( length == PFORMAT_LENGTH_CHAR )
	      /*
	       * to a signed `char' destination...
	       */
	      *va_arg( argv, char * ) = stream.count;

	    else if( length == PFORMAT_LENGTH_SHORT )
	      /*
	       * or to a signed `short'...
	       */
	      *va_arg( argv, short * ) = stream.count;

	    else if( length == PFORMAT_LENGTH_LONG )
	      /*
	       * or to a signed `long'...
	       */
	      *va_arg( argv, long * ) = stream.count;

	    else if( length == PFORMAT_LENGTH_LLONG )
	      /*
	       * or to a signed `long long'...
	       */
	      *va_arg( argv, long long * ) = stream.count;

	    else
	      /*
	       * or, by default, to a signed `int'.
	       */
	      *va_arg( argv, int * ) = stream.count;

	    goto format_scan;

	  /* Argument length modifiers...
	   * These are non-terminal; each sets the format parser
	   * into the PFORMAT_END state, and ends with a `break'.
	   */
	  case 'h': case 'j': case 'l': case 't': case 'z':
#	  ifdef _WIN32
	    /* While the preceding five are ISO-C standards, not
	     * all are supported by Microsoft's implementation of
	     * printf(); we will generally favour the semantics as
	     * specified by ISO-C, but except in case of conflict,
	     * we also attempt to support Microsoft alternatives.
	     * Thus, we also recognise their 'I' modifier...
	     */
	  case 'I':
	    /* ...interpreting all six appropriately...
	     */
	    length = __pformat_length_modifier( &fmt );
	    /*
	     * ...while further noting that they accept 'l' as an
	     * alternative to 'L', as a qualifier for 'long double';
	     * this DOES conflict with ISO-C usage, so we offer it as
	     * an optional user choice, whether the Microsoft usage is
	     * to be supported, or (by default) to ignore such usage
	     * entirely, as required by ISO-C.
	     */
	    if( (c != 'l') || !__pformat_is_alt_ldouble_modifier( length ) )
	    {
	      /* In the case where the Microsoft usage is to be ignored,
	       * we mark the end of conversion specification analysis,
	       * (as in the case of any modifier other than 'l')...
	       */
	      state = PFORMAT_END;
	      break;
	    }
	    /* ...or, if the Microsoft usage option is enabled, simply
	     * fall through to the next case, and handle 'l' as if it
	     * were 'L'...
	     */
#	  else
	    /* ...whereas, if building for any non-windows platform,
	     * we simply analyse each of the five standard modifiers,
	     * and end the case analysis immediately.
	     */
	    length = __pformat_length_modifier( &fmt );
	    state = PFORMAT_END;
	    break;
#	  endif

	  case 'L':
	    /* Identify the appropriate argument as a `long double',
	     * when associated with `%a', `%A', `%e', `%E', `%f', `%F',
	     * `%g' or `%G' format specifications.
	     */
	    stream.flags |= PFORMAT_LDOUBLE;
	    state = PFORMAT_END;
	    break;

	  /* Precision indicator...
	   * May appear once only; it must precede any modifier
	   * for argument length, or any data type specifier.
	   */
	  case '.':
	    if( state < PFORMAT_GET_PRECISION )
	    {
	      /* We haven't seen a precision specification yet,
	       * so initialise it to zero, (in case no digits follow),
	       * and accept any following digits as the precision.
	       */
	      stream.precision = 0;
	      width_spec = &stream.precision;
	      state = PFORMAT_GET_PRECISION;
	    }
	    else
	      /* We've already seen a precision specification,
	       * so this is just junk; proceed to end game.
	       */
	      state = PFORMAT_END;

	    /* Either way, we must not fall through here.
	     */
	    break;

	  /* Variable field width, or precision specification,
	   * derived from the argument list...
	   */
	  case '*':
	    /* When this appears...
	     */
	    if(   width_spec
	    &&  ((state == PFORMAT_INIT) || (state == PFORMAT_GET_PRECISION)) )
	    {
	      /* ...in proper context; assign to field width
	       * or precision, as appropriate...
	       */
	      if( (argc > 0) && ((arg_index = __pformat_arg_index( &fmt )) > 0) )
	      {
		/* ...from the specified indexed argument, when
		 * processing arguments in indexed order...
		 */
		va_list argv;
		va_copy( argv, argv_indexed[zero_adjusted(arg_index)] );
		*width_spec = va_arg( argv, int );
		va_end( argv );
	      }
	      /* ...or otherwise, from the next available
	       * sequentially accessed argument.
	       */
	      else *width_spec = va_arg( argv, int );
	      if( *width_spec < 0 )
	      {
		/* Assigned value was negative...
		 */
		if( state == PFORMAT_INIT )
		{
		  /* For field width, this is equivalent to
		   * a positive value with the `-' flag...
		   */
		  stream.flags |= PFORMAT_LJUSTIFY;
		  stream.width = -stream.width;
		}
		else
		  /* while as a precision specification,
		   * it should simply be ignored.
		   */
		  stream.precision = PFORMAT_IGNORE;
	      }
	    }
	    else
	      /* out of context; give up on width and precision
	       * specifications for this conversion.
	       */
	      state = PFORMAT_END;

	    /* Mark as processed...
	     * we must not see `*' again, in this context.
	     */
	    width_spec = NULL;
	    break;

	  /* Formatting flags...
	   * Must appear while in the PFORMAT_INIT state,
	   * and are non-terminal, so again, end with `break'.
	   */
	  case '#':
	    /* Select alternate PFORMAT_HASHED output style.
	     */
	    if( state == PFORMAT_INIT )
	      stream.flags |= PFORMAT_HASHED;
	    break;

	  case '+':
	    /* Print a leading sign with numeric output,
	     * for both positive and negative values.
	     */
	    if( state == PFORMAT_INIT )
	      stream.flags |= PFORMAT_POSITIVE;
	    break;

	  case '-':
	    /* Select left justification of displayed output
	     * data, within the output field width, instead of
	     * the default flush right justification.
	     */
	    if( state == PFORMAT_INIT )
	      stream.flags |= PFORMAT_LJUSTIFY;
	    break;

	  case '\'':
	    /* Formerly an XSI extension to the POSIX standard,
	     * but fully supported as of POSIX.1-2008, this flag
	     * causes grouping of digits according to the rules
	     * defined for the LC_NUMERIC category within the
	     * current locale.
	     */
	    if( state == PFORMAT_INIT )
	      stream.flags |= PFORMAT_GROUPED;
	    break;

	  case '\x20':
	    /* Reserve a single space, within the output field,
	     * for display of the sign of signed data; this will
	     * be occupied by the minus sign, if the data value
	     * is negative, or by a plus sign if the data value
	     * is positive AND the `+' flag is also present, or
	     * by a space otherwise.  (Technically, this flag
	     * is redundant, if the `+' flag is present).
	     */
	    if( state == PFORMAT_INIT )
	      stream.flags |= PFORMAT_ADDSPACE;
	    break;

	  case '0':
	    /* May represent a flag, to activate the `pad with zeros'
	     * option, or it may simply be a digit in a width or in a
	     * precision specification...
	     */
	    if( state == PFORMAT_INIT )
	    {
	      /* This is the flag usage...
	       */
	      stream.flags |= PFORMAT_ZEROFILL;
	      break;
	    }

	  default:
	    /* If we didn't match anything above, then we will check
	     * for digits, which we may accumulate to generate field
	     * width or precision specifications...
	     */
	    if( (state < PFORMAT_END) && isdigit( c ) )
	    {
	      if( state == PFORMAT_INIT )
		/* Initial digits explicitly relate to field width...
		 */
		state = PFORMAT_SET_WIDTH;

	      else if( state == PFORMAT_GET_PRECISION )
		/* while those following a precision indicator
		 * explicitly relate to precision.
		 */
		state = PFORMAT_SET_PRECISION;

	      if( width_spec )
	      { /* We are accepting a width or precision specification;
		 * add the units value represented by the current digit,
		 * to ten times the value accumulated so far.
		 */
		*width_spec = __pformat_imul10plus( *width_spec, c - '0' );
	      }
	    }
	    else
	    { /* We found a digit out of context, or some other character
	       * with no designated meaning; reject this format specification,
	       * backtrack, and emit it as literal text...
	       */
	      fmt = backtrack;
	      __pformat_putc( '%', &stream );
	      goto format_scan;
	    }
	}
      }
    }
    else
      /* We just parsed a character which is not included within any format
       * specification; we simply emit it as a literal.
       */
      __pformat_putc( c, &stream );
  }
  /* Clean up the resource pool, which was allocated for local processing of
   * the passed-in argument vector in either sequential or random order.
   */
  while( argc-- > 0 ) va_end( argv_indexed[argc] );
  va_end( argv );

  /* Release the memory, if any, allocated to manage the grouping strategy
   * of digits with intervening thousands digits group separators.
   */
  free( stream.grouping );

  /* When we have fully dispatched the format string, the return value is the
   * total number of bytes we transferred to the output destination.
   */
  return stream.count;
}

/* $RCSfile$$Revision$: end of file */
