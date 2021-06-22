/*
 * random.c
 *
 * Implementation of a (mostly) POSIX.1-1990 conforming pseudo-random
 * number generating API, for use with MinGW applications.
 *
 * $Id$
 *
 * Written by Keith Marshall <keith@users.osdn.me>
 * Copyright (C) 2021, MinGW.org Project
 *
 *
 * This is free software.  Permission is granted to copy, modify and
 * redistribute this software, under the provisions of the GNU General
 * Public License, Version 3, (or, at your option, any later version),
 * as published by the Free Software Foundation; see the file COPYING
 * for licensing details.
 *
 * Note, in particular, that this software is provided "as is", in the
 * hope that it may prove useful, but WITHOUT WARRANTY OF ANY KIND; not
 * even an implied WARRANTY OF MERCHANTABILITY, nor of FITNESS FOR ANY
 * PARTICULAR PURPOSE.  Under no circumstances will the author, or the
 * MinGW Project, accept liability for any damages, however caused,
 * arising from the use of this software.
 *
 *
 * This pseudo-random number generation suite has been derived from a
 * discourse on the GLIBC implementation, by Peter Selinger:
 *
 *   https://www.mathstat.dal.ca/~selinger/random/
 *
 * It is believed that the output from the PRNG will closely mimic that
 * from the GLIBC implementation; however, neither the author of this
 * implementation, nor the MinGW.org Project, offer any assurance as to
 * the statistical quality of the generated number sequence.
 *
 *
 * Note: this implementation has been provided for optional compliance
 * with the POSIX.1-1990 Extended Systems Interface.
 */
#define _XOPEN_SOURCE  500

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

struct state
{ /* Internal structure, used to map the PRNG state buffer from its
   * arbitrarily specified sequence of 8, 32, 64, 128, or 256 bytes,
   * to an array of int32_t entities; the first entry is used as a
   * control sturcture, comprising three 8-bit indices, while the
   * remaining entries are used to represent PRNG state.
   */
  struct
  { uint32_t	phase:8;	/* current state data cycle pointer  */
    uint32_t	shift:8;	/* associated offset data pointer    */
    uint32_t	limit:8;	/* total count of state data entries */
  };
  int32_t	data[]; 	/* array of state data entries	     */
};

/* Provide a local state buffer, of length 128 bytes, (equivalent
 * to 32 int32_t entries), for use when no alternative buffer has
 * been specified, prior to calling random().
 */
static int32_t default_state[32] = { 0 };
static struct state *state = (struct state *)(default_state);

/* The following inline local function is provided to facilitate
 * setting of "errno", on abnormal return from any other function.
 */
static __inline__ __attribute__((__always_inline__))
intptr_t errout( int code, intptr_t retval ){ errno = code; return retval; }

static char *assign_state( char *buf, char *prev )
{ /* A local helper function, to attach a new state buffer to the
   * PRNG; aborts the assignment, if the specified buffer is NULL,
   * but otherwise, performs no buffer validation.
   */
  if( buf == NULL ) return (char *)(errout( EINVAL, (intptr_t)(NULL) ));
  state = (struct state *)(buf);
  return prev;
}

char *__mingw_setstate( char *buf )
{ /* Public API entry point, exhibiting POSIX.1-1990 semantics,
   * for accessing the preceding buffer assignment function; the
   * "buf" argument is assumed to point to an existing non-NULL
   * buffer, which had been previously initialized by a prior
   * initstate() call, (and possibly already used), but, other
   * that checking that it is not NULL, no validation of its
   * initialization state is performed.
   */
  return assign_state( buf, (char *)(state) );
}

static size_t normalized_cycle( size_t len )
{ /* A local helper function, to establish the effective number
   * of entries in the PRNG state data array, as a dependency of
   * the total byte count specified for the state buffer; note
   * that the compiler may be able to compute the resultant at
   * compile time, when called with a constant "len" argument,
   * and so optimize away such calls.
   */
  size_t cycle_counter = (len >= 32) ? 256 : 8;
  while( cycle_counter > len ) cycle_counter >>= 1;
  return (cycle_counter >> 2) - 1;
}

static long update_state( void )
{ /* A local helper function, to update the content of the active
   * state buffer with each successive call of random().
   */
  if( state->phase == state->limit ) state->phase = 0;
  else if( state->shift == state->limit ) state->shift = 0;
  return (state->data[state->shift++] += state->data[state->phase++]);
}

static void initialize_state_data( unsigned int seed )
{ /* A local helper function, called after initialization of the
   * state buffer length count, to populate the state data array.
   *
   * In all cases, we record the "seed" value, as the first entry
   * in the state data array, (rejecting zero, for which we force
   * substitution of 1).
   */
  state->data[0] = (seed != 0) ? seed : 1;

  /* For a minimum-length state buffer, of only 8 bytes, there is
   * no more state data to be recorded...
   */
  if( state->limit > 1 )
  { /* ...but for any of the larger permitted buffer sizes, we must
     * populate the buffer using a multiplicative sequence of 31-bit
     * integers; the multiplication is performed in a 64-bit space,
     * then reduced modulo the Mersenne prime of order 31; set up a
     * collection of constants, pertinent to the computation...
     */
    const unsigned long order = 31;
    const unsigned long divisor = (unsigned long)((1ULL << order) - 1);
    const unsigned long long multiplier = 16807ULL;

    /* ...then apply to each data array entry, in turn...
     */
    for( seed = 1; seed < state->limit; seed++ )
    { /* ...computing the sequence of products, in 64-bit space...
       */
      unsigned long long tmp = multiplier * state->data[seed - 1];

      /* ...then reduce for storage; note that, since the divisor
       * is a Mersenne prime, we can perform the modulo division by
       * use of the more computationally efficient mask, shift and
       * addition, followed by a conditional correction, in case
       * of overflow into the sign bit.
       */
      state->data[seed] = (int)(tmp = (tmp & divisor) + (tmp >> order));
      if( state->data[seed] < 0 ) state->data[seed] += divisor;
    }
    /* We must also initialize the phase, and shift indices, for
     * the PRNG feed-back loop...
     */
    state->phase = 0;
    switch( state->limit )
    { /* ...noting that the shift interval is chosen differently,
       * depending on the limiting length of the sequence.
       */
      case 63: case 15: state->shift = 1; break;
      case 31: case  7: state->shift = 3; break;
    }
    /* Finally, run the PRNG through 10 update cycles for each and
     * every entry in the state data array, but without generating
     * any actual pseudo-random number output.
     */
    seed = ((state->limit << 2) + state->limit) << 1;
    while( seed-- > 0 ) update_state();
  }
}

char *__mingw_initstate( unsigned int seed, char *buf, size_t len )
{ /* Public API entry point, implementing the POSIX.1-1990 initstate()
   * function; aborts if "buf" is passed as a NULL pointer, or if "len"
   * is less than the minimum allowed 8 bytes; otherwise, assigns "buf"
   * as a new PRNG state buffer...
   */
  char *prev = assign_state( buf, (len >= 8) ? (char *)(state) : NULL );
  if( prev != NULL )
  {
    /* ...initializes it to the specified "len", and as if srandom()
     * has been called with "seed" as argument...
     */
    state->limit = normalized_cycle( len );
    initialize_state_data( seed );
  }
  /* ...then returns a pointer to the original state buffer.
   */
  return prev;
}

void __mingw_srandom( unsigned int seed )
{ /* Public API entry point, implementing the POSIX-1.1990 srandom()
   * function; this sets the PRNG to a known initial state, which is
   * dependent on the "seed" value specified.
   */
  if( (state == (struct state *)(default_state)) && (state->limit == 0) )
    state->limit = normalized_cycle( 128 );
  initialize_state_data( seed );
}

long __mingw_random( void )
{ /* Public API entry point, implementing the POSIX-1.1990 random()
   * function; this initially checks whether the state buffer has been
   * provided by the user, (in which case we assume that it was properly
   * initialized, by calling initstate()), or if the default buffer is
   * in use...
   */
  if( state == (struct state *)(default_state) )
  { /* ...and we anticipate that, if in its default application start-up
     * state, this buffer may require implicit initialization; when this
     * is the case...
     */
    if( state->limit == 0 )
    { /* ...as indicated by no buffer size limit having been assigned,
       * we initialize it now, as if by calling initstate() with buffer
       * size specified appropriately, as 128 bytes, and seed of 1.
       */
      state->limit = normalized_cycle( 128 );
      initialize_state_data( 1 );
    }
  }
  else if( state->limit == normalized_cycle( 8 ) )
  { /* The state buffer has been explicitly initialized already, using
     * the minimum allowed length of 8 bytes; in this case, we use a
     * simple linear congruential PRNG, with multiplier of 1103515245,
     * and increment of 12345, masked to ensure that the result fits
     * within the range of positive 32-bit signed integer values.
     */
    const unsigned long long increment = 12345ULL;
    const unsigned long long multiplier = 1103515245ULL;
    const unsigned int result_mask = ((1ULL << 31) - 1);
    unsigned long long tmp = multiplier * state->data[0] + increment;
    return (long)(state->data[0] = (int)(tmp & result_mask));
  }
  /* For any state buffer, longer than the 8-byte minimum, update the
   * buffer state, and extract the next pseudo-random number to return,
   * scaling to discard its least significant bit.
   */
  return (long)((unsigned long)(update_state()) >> 1);
}

/* $RCSfile$: end of file */
