/*
 * glob.c
 *
 * MinGW implementation of (approximately) POSIX conforming glob() and
 * globfree() API functions.
 *
 * $Id$
 *
 * Written by Keith Marshall <keithmarshall@users.sourceforge.net>
 * Copyright (C) 2011-2014, 2017, 2018, 2022, MinGW.OSDN Project.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OF OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */
#include <glob.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <libgen.h>
#include <dirent.h>
#include <errno.h>

enum {
  /* Extend the flags offset enumeration, beyond the user visible
   * high water mark, to accommodate some additional flags which are
   * required for private use by the implementation.
   */
  __GLOB_DIRONLY_OFFSET = __GLOB_FLAG_OFFSET_HIGH_WATER_MARK,
  __GLOB_PERIOD_PRIVATE_OFFSET,
  /*
   * For congruency, set a new high water mark above the private data
   * range, (which we don't otherwise use).
   */
  __GLOB_PRIVATE_FLAGS_HIGH_WATER_MARK
};

#define GLOB_DIRONLY	__GLOB_FLAG__(DIRONLY)
#ifndef GLOB_PERIOD
# define GLOB_PERIOD	__GLOB_FLAG__(PERIOD_PRIVATE)
#endif

#ifndef GLOB_INLINE
# define GLOB_INLINE	static __inline__ __attribute__((__always_inline__))
#endif

#if defined _WIN32 || defined __MS_DOS__
/*
 * For the Microsoft platforms, we treat '\' and '/' interchangeably
 * as directory separator characters...
 */
#define GLOB_DIRSEP		('\\')
# define glob_is_dirsep( c )	(((c) == ('/')) || ((c) == GLOB_DIRSEP))
/*
 * ...and we substitute whatever may have been predefined as the
 * manifest constant __CRT_GLOB_ESCAPE_CHAR__, (nominally ASCII DEL,
 * a.k.a. RUBOUT), as an alternative escape character.
 */
# ifdef __CRT_GLOB_ESCAPE_CHAR__
#  define GLOB_HARD_ESC		__CRT_GLOB_ESCAPE_CHAR__
# else
#  define GLOB_HARD_ESC		((char)(127))
# endif

static int glob_escape_char = GLOB_HARD_ESC;

GLOB_INLINE char *glob_strdup( const char *pattern )
{
  /* An inline wrapper around the standard strdup() function;
   * this strips instances of the GLOB_HARD_ESC character, which
   * have not themselves been escaped, from the strdup()ed copy.
   */
  char buf[1 + strlen( pattern )];
  char *copy = buf; const char *origin = pattern;
  do { if( *origin == GLOB_HARD_ESC ) ++origin;
       *copy++ = *origin;
     } while( *origin++ );
  return strdup( buf );
}

#else
/* Otherwise, we assume only the POSIX standard '/'...
 */
#define GLOB_DIRSEP		('/')
# define glob_is_dirsep( c )	((c) == GLOB_DIRSEP)
/*
 * ...and we interpret '\', as specified by POSIX, as
 * the escape character.
 */
static int glob_escape_char = '\\';

#define glob_strdup	strdup
#endif

static int is_glob_pattern( const char *pattern, int flags )
{
  /* Check if "pattern" represents a globbing pattern
   * with included wild card characters.
   */
  register const char *p;
  register int c;

  /* Proceed only if specified pattern is not NULL...
   */
  if( (p = pattern) != NULL )
  {
    /* ...initially, with no bracketted character set open;
     * (none can be, because we haven't yet had any opportunity
     * to see the opening bracket).
     */
    int bracket = 0;

    /* Check each character in "pattern" in turn...
     */
    while( (c = *p++) != '\0' )
    {
      /* ...proceeding since we have not yet reached the NUL terminator.
       */
      if(  ((flags & GLOB_NOESCAPE) == 0)
      &&  (c == glob_escape_char) && (*p++ == '\0')  )
	/*
	 * We found an escape character, (and the escape mechanism has
	 * not been disabled), but there is no following character to
	 * escape; it may be malformed, but this certainly doesn't look
	 * like a candidate globbing pattern.
	 */
	return 0;

      else if( bracket == 0 )
      {
	/* Still outside of any bracketted character set...
	 */
	if( (c == '*') || (c == '?') )
	  /*
	   * ...either of these makes "pattern" an explicit
	   * globbing pattern...
	   */
	  return 1;

	if( c == '[' )
	  /*
	   * ...while this marks the start of a bracketted
	   * character set.
	   */
	  bracket++;
      }

      else if( (bracket > 1) && (c == ']') )
	/*
	 * Within a bracketted character set, where it is not
	 * the first character, ']' marks the end of the set,
	 * making "pattern" a globbing pattern.
	 */
	return 1;

      else if( c != '!' )
	/*
	 * Also within a bracketted character set, '!' is special
	 * when the first character, and shouldn't be counted; note
	 * that it should be counted when not the first character,
	 * but the short count resulting from ignoring it doesn't
	 * affect our desired outcome.
	 */
	bracket++;
    }
  }

  /* If we get to here, then we ran off the end of "pattern" without
   * identifying it as a globbing pattern.
   */
  return 0;
}

static const char *glob_set_adjusted( const char *pattern, int flags )
{
  /* Adjust the globbing pattern pointer, to make it refer to the
   * next character (if any) following a character set specification;
   * this adjustment is required when pattern matching is to resume
   * after matching a set specification, irrespective of whether the
   * match was successful or not; (a failed match is the desired
   * outcome for an excluded character set).
   */
  register const char *p = pattern;

  /* We need to move the pointer forward, until we find the ']'
   * which marks the end of the set specification.
   */
  while( *p != ']' )
  {
    /* We haven't found it yet; advance by one character...
     */
    if( (*p == glob_escape_char) && ((flags & GLOB_NOESCAPE) == 0) )
      /*
       * ...or maybe even two, when we identify a need to
       * step over any character which has been escaped...
       */
      p++;

    if( *p++ == '\0' )
      /*
       * ...but if we find a NUL on the way, then the pattern
       * is malformed, so we return NULL to report a bad match.
       */
      return NULL;
  }
  /* We found the expected ']'; return a pointer to the NEXT
   * character, (which may be ANYTHING; even NUL is okay).
   */
  return ++p;
}

GLOB_INLINE int glob_case_match( int flags, int check, int match )
{
  /* Local helper function, used to facilitate the case insensitive
   * glob character matching appropriate for MS-Windows systems.
   */
  return (flags & GLOB_CASEMATCH) ? check - match
    : tolower( check ) - tolower( match );
}

static const char *glob_in_set( const char *set, int test, int flags )
{
  /* Check if the single character "test" is present in the set
   * of characters represented by "set", (a specification of the
   * form "[SET]", or "[!SET]" in the case of an excluded set).
   *
   * On entry, "set" always points to the first character in the
   * set to be tested, i.e. the character FOLLOWING the '[' which
   * opens an inclusive set, or FOLLOWING the initial '!' which
   * marks the set as exclusive.
   *
   * Matching is ALWAYS performed as if checking an inclusive set;
   * return value is a pointer to the globbing pattern character
   * following the closing ']' of "set", when "test" IS in "set",
   * or NULL when it is not.  Caller performing an inclusive match
   * should handle NULL as a failed match, and non-NULL as success.
   * Caller performing an exclusive match should handle non-NULL as
   * a failed match, with NULL indicating success, and should call
   * glob_set_adjusted() before resuming pattern matching in the
   * case of a successful match.
   */
  register int c, lastc;
  if( ((lastc = *set) == ']') || (lastc == '-') )
  {
    /* This is the special case of matching ']' or '-' as the
     * first character in the set, where it must match literally...
     */
    if( lastc == test )
      /*
       * ...which it does, so immediately report it so.
       */
      return glob_set_adjusted( ++set, flags );

    /* ...otherwise we didn't match this special case of ']' or '-',
     * so we simply ignore this special set entry, thus handling it
     * as an implicitly escaped literal which has not been matched.
     */
    set++;
  }
  while( (c = *set++) != ']' )
  {
    /* We are still scanning the set, and have not yet reached the
     * closing ']' sentinel character.
     */
    if( (c == '-') && (*set != ']') && ((c = *set++) != '\0') )
    {
      /* Since the current character is a '-', and is not immediately
       * followed by the set's closing sentinel, nor is it at the end
       * of the (malformed) pattern, it specifies a character range,
       * running from the last character scanned...
       */
      while( lastc < c )
      {
	/* ...in incremental collating sequence order, to the next
	 * character following the '-'...
	 */
	if( glob_case_match( flags, lastc++, test ) == 0 )
	  /*
	   * ...returning immediately on a successful match...
	   */
	  return glob_set_adjusted( set, flags );
      }
      while( lastc > c )
      {
	/* ...or failing that, consider the possibility that the
	 * range may have been specified in decrementing collating
	 * sequence order...
	 */
	if( glob_case_match( flags, lastc--, test ) == 0 )
	  /*
	   * ...once again, return immediately on a successful match.
	   */
	  return glob_set_adjusted( set, flags );
      }
    }

    if( (c == '\0')
      /*
       * This is a malformed set; (not closed before the end of
       * the pattern)...
       */
    ||  glob_is_dirsep( c )  )
      /*
       * ...or it attempts to explicitly match a directory separator,
       * which is invalid in this context.  We MUST fail it, in either
       * case, reporting a mismatch.
       */
      return NULL;

    if( glob_case_match( flags, c, test ) == 0 )
      /*
       * We found the test character within the set; adjust the pattern
       * reference, to resume after the end of the set, and return the
       * successful match.
       */
      return glob_set_adjusted( set, flags );

    /* If we get to here, we haven't yet found the test character within
     * this set; remember the character within the set which we just tried
     * to match, as it may represent the start of a character range, then
     * continue the scan, until we exhaust the set.
     */
    lastc = c;
  }
  /* Having exhausted the set, without finding a match, we return NULL
   * to indicate that the test character was NOT found in the set.
   */
  return NULL;
}

static int glob_strcmp( const char *pattern, const char *text, int flags )
{
  /* Compare "text" to a specified globbing "pattern" using semantics
   * comparable to "strcmp()"; returns zero for a complete match, else
   * non-zero for a mismatch.
   *
   * Within "pattern":
   *   '?'     matches any one character in "text" (except '\0')
   *   '*'     matches any sequence of zero or more characters in "text"
   *   [SET]   matches any one character in "text" which is also in "SET"
   *   [!SET]  matches any one character in "text" which is NOT in "SET"
   */
  register const char *p = pattern, *t = text;
  register int c;

  if( (*t == '.') && (*p != '.') && ((flags & GLOB_PERIOD) == 0) )
    /*
     * The special GNU extension allowing wild cards to match a period
     * as first character is NOT in effect; "text" DOES have an initial
     * period character AND "pattern" DOES NOT match it EXPLICITLY, so
     * this comparison must report a MISMATCH.
     */
    return *p - *t;

  /* Attempt to match "pattern", character by character...
   */
  while( (c = *p++) != '\0' )
  {
    /* ...so long as we haven't exhausted it...
     */
    switch( c )
    {
      case '?':
	/* Match any one character...
	 */
	if( *t++ == '\0' )
	  /* ...but when there isn't one left to be matched,
	   * then we must report a mismatch.
	   */
	  return '?';
	break;

      case '*':
	/* Match any sequence of zero or more characters...
	 */
	while( *p == '*' )
	  /*
	   * ...ignoring any repeated '*' in the pattern...
	   */
	  p++;

	/* ...and if we've exhausted the pattern...
	 */
	if( *p == '\0' )
	  /*
	   * ...then we simply match all remaining characters,
	   * to the end of "text", so we may return immediately,
	   * reporting a successful match.
	   */
	  return 0;

	/* When we haven't exhausted the pattern, then we may
	 * attempt to recursively match the remainder of the
	 * pattern to some terminal substring of "text"; we do
	 * this iteratively, stepping over as many characters
	 * of "text" as necessary, (and which thus match the '*'
	 * in "pattern"), until we either find the start of this
	 * matching substring, or we exhaust "text" without any
	 * possible match...
	 */
	do { c = glob_strcmp( p, t, flags | GLOB_PERIOD );
	   } while( (c != 0) && (*t++ != '\0') );
	/*
	 * ...and ultimately, we return the result of this
	 * recursive attempt to find a match.
	 */
	return c;

      case '[':
	/* Here we need to match (or not match) exactly one
	 * character from the candidate text with any one of
	 * a set of characters in the pattern...
	 */
	if( (c = *t++) == '\0' )
	  /*
	   * ...but, we must return a mismatch if there is no
	   * candidate character left to match.
	   */
	  return '[';

	if( *p == '!' )
	{
	  /* Match any one character which is NOT in the SET
	   * specified by [!SET].
	   */
	  if( glob_in_set( ++p, c, flags ) == NULL )
	  {
	    if( *p == ']' )
	      p++;
	    p = glob_set_adjusted( p, flags );
	  }
	}
	else
	{ /* Match any one character which IS in the SET
	   * specified by [SET].
	   */
	  p = glob_in_set( p, c, flags );
	}
	if( p == NULL )
	  /*
	   * The character under test didn't satisfy the SET
	   * matching criterion; return as unmatched.
	   */
	  return ']';
	break;

      default:
	/* The escape character cannot be handled as a regular
	 * switch case, because the escape character is specified
	 * as a variable, (to better support Microsoft nuisances).
	 * The escape mechanism may have been disabled within the
	 * glob() call...
	 */
	if(  ((flags & GLOB_NOESCAPE) == 0)
	  /*
	   * ...but when it is active, and we find an escape
	   * character without exhausting the pattern...
	   */
	&& (c == glob_escape_char) && ((c = *p) != 0)  )
	  /*
	   * ...then we handle the escaped character here, as
	   * a literal, and step over it, within the pattern.
	   */
	  ++p;

	/* When we get to here, a successful match requires that
	 * the current pattern character "c" is an exact literal
	 * match for the next available character "t", if any,
	 * in the candidate text string...
	 */
	if( (*t == '\0') || (glob_case_match( flags, c, *t ) != 0) )
	  /*
	   * ...otherwise we return a mismatch.
	   */
	  return c - *t;

	/* No mismatch yet; proceed to test the following character
	 * within the candidate text string.
	 */
	t++;
    }
  }
  /* When we've exhausted the pattern, then this final check will return
   * a match if we've simultaneously exhausted the candidate text string,
   * or a mismatch otherwise.
   */
  return c - *t;
}

#ifdef DT_DIR
/*
 * When this is defined, we assume that we can safely interrogate
 * the d_type member of a globbed dirent structure, to determine if
 * the referenced directory entry is itself a subdirectory entry.
 */
# define GLOB_ISDIR( ent )	((ent)->d_type == DT_DIR)

#else
/* We can't simply check for (ent)->d_type == DT_DIR, so we must
 * use stat() to identify subdirectory entries.
 */
# include <sys/stat.h>

  GLOB_INLINE
  int GLOB_ISDIR( const struct *dirent ent )
  {
    struct stat entinfo;
    if( stat( ent->d_name, &entinfo ) == 0 )
      return S_ISDIR( entinfo.st_mode );
    return 0;
  }
#endif

#if _DIRENT_HAVE_D_NAMLEN
/*
 * Our DIRENT implementation provides a direct indication
 * of the length of the file system entity name returned by
 * the last readdir operation...
 */
# define D_NAMLEN( entry )  ((entry)->d_namlen)
#else
/*
 * ...otherwise, we have to scan for it.
 */
# define D_NAMLEN( entry )  (strlen( (entry)->d_name ))
#endif

static int glob_initialise( glob_t *gl_data )
{
  /* Helper routine to initialise a glob_t structure
   * for first time use.
   */
  if( gl_data != NULL )
  {
    /* Caller gave us a valid pointer to what we assume has been
     * defined as a glob_t structure; allocate space on the heap,
     * for storage of the globbed paths vector...
     */
    int entries = gl_data->gl_offs + 1;
    if( (gl_data->gl_pathv = malloc( entries * sizeof( char ** ) )) == NULL )
      /*
       * ...bailing out, if insufficient free heap memory.
       */
      return GLOB_NOSPACE;

    /* On successful allocation, clear the initial path count...
     */
    gl_data->gl_pathc = 0;
    while( entries > 0 )
      /*
       * ...and place a NULL pointer in each allocated slot...
       */
      gl_data->gl_pathv[--entries] = NULL;
  }
  /* ...ultimately returning a successful initialisation status.
   */
  return GLOB_SUCCESS;
}

GLOB_INLINE int glob_expand( glob_t *gl_buf )
{
  /* Inline helper to compute the new size allocation required
   * for buf->gl_pathv, prior to adding a new glob result.
   */
  return ((2 + gl_buf->gl_pathc + gl_buf->gl_offs) * sizeof( char ** ));
}

static int glob_store_entry( char *path, glob_t *gl_buf )
{
  /* Local helper routine to add a single path name entity
   * to the globbed path vector, after first expanding the
   * allocated memory space to accommodate it.
   */
  char **pathv;
  if(  (path != NULL)  &&  (gl_buf != NULL)
  &&  ((pathv = realloc( gl_buf->gl_pathv, glob_expand( gl_buf ))) != NULL)  )
  {
    /* Memory expansion was successful; store the new path name
     * in place of the former NULL pointer at the end of the old
     * vector...
     */
    gl_buf->gl_pathv = pathv;
    gl_buf->gl_pathv[gl_buf->gl_offs + gl_buf->gl_pathc++] = path;
    /*
     * ...then place a further NULL pointer into the newly allocated
     * slot, to mark the new end of the vector...
     */
    gl_buf->gl_pathv[gl_buf->gl_offs + gl_buf->gl_pathc] = NULL;
    /*
     * ...before returning a successful completion status.
     */
    return GLOB_SUCCESS;
  }
  /* If we get to here, then we were unsuccessful.
   */
  return GLOB_ABORTED;
}

struct glob_collator
{
  /* A private data structure, used to keep an ordered collection
   * of globbed path names in collated sequence within a (possibly
   * unbalanced) binary tree.
   */
  struct glob_collator	*prev;
  struct glob_collator	*next;
  char  		*entry;
};

GLOB_INLINE struct glob_collator
*glob_collate_entry( struct glob_collator *collator, char *entry, int flags )
{
  /* Inline helper function to construct a binary tree representation
   * of a collated collection of globbed path name entities.
   */
  int seq = 0;
  struct glob_collator *ref = collator;
  struct glob_collator *lastref = collator;
  while( ref != NULL )
  {
    /* Walk the tree, to find the leaf node representing the insertion
     * point, in correctly collated sequence order, for the new entry,
     * noting whether we must insert the new entry before or after the
     * original entry at that leaf.
     */
    lastref = ref;
    if( flags & GLOB_CASEMATCH )
      seq = strcoll( entry, ref->entry );
    else
      seq = stricoll( entry, ref->entry );
    ref = (seq > 0) ? ref->next : ref->prev;
  }
  /* Allocate storage for a new leaf node, and if successful...
   */
  if( (ref = malloc( sizeof( struct glob_collator ))) != NULL )
  {
    /* ...place the new entry on this new leaf...
     */
    ref->entry = entry;
    ref->prev = ref->next = NULL;

    /* ...and attach it to the tree...
     */
    if( lastref != NULL )
    {
      /* ...either...
       */
      if( seq > 0 )
	/*
	 * ...after...
	 */
	lastref->next = ref;

      else
	/* ...or before...
	 */
	lastref->prev = ref;

      /* ...the original leaf,as appropriate. */
    }
  }
  /* When done, return a pointer to the root node of the resultant tree.
   */
  return (collator == NULL) ? ref : collator;
}

static void
glob_store_collated_entries( struct glob_collator *collator, glob_t *gl_buf )
{
  /* A local helper routine to store a collated collection of globbed
   * path name entities into the path vector within a glob_t structure;
   * it performs a recursive inorder traversal of a glob_collator tree,
   * deleting it leaf by leaf, branch by branch, as it stores the path
   * data contained thereon.
   */
  if( collator->prev != NULL )
    /*
     * Recurse into the sub-tree of entries which collate before the
     * root of the current (sub-)tree.
     */
    glob_store_collated_entries( collator->prev, gl_buf );

  /* Store the path name entry at the root of the current (sub-)tree.
   */
  glob_store_entry( collator->entry, gl_buf );

  if( collator->next != NULL )
    /*
     * Recurse into the sub-tree of entries which collate after the
     * root of the current (sub-)tree.
     */
    glob_store_collated_entries( collator->next, gl_buf );

  /* Finally, delete the root node of the current (sub-)tree; since
   * recursion visits every node of the tree, ultimately considering
   * each leaf as a sub-tree of only one node, unwinding recursion
   * will cause this to delete the entire tree.
   */
  free( collator );
}

GLOB_INLINE int
accept_glob_nocheck_match( const char *pattern, int flags )
{
  /* Helper to check whether a pattern may be stored into gl_pathv as is,
   * without attempting to find any file system match; (this is permitted
   * only when the caller has set the GLOB_NOCHECK flag, and the pattern
   * is devoid of any globbing token).
   */
  return (flags & GLOB_NOCHECK) && (is_glob_pattern( pattern, flags ) == 0);
}

GLOB_INLINE int
glob_brace_expand( char *dest, const char *src, const char **resume )
{
  /* Helper to iteratively expand the first substitution field within
   * a glob brace expression, while recursively collecting the set of
   * individual globbing patterns to be processed by glob_match(), when
   * GLOB_BRACE is specified, and the original pattern includes a brace
   * expression.  (Notice that this does not guarantee to fully expand
   * the pattern in a single pass; recursion within glob_match() will
   * ensure that this is achieved, before attempting to match each
   * possible expansion of the original pattern).
   *
   * Returns zero on a successful (possibly partial) expansion; > zero
   * indicates unmatched opening braces, (an error condition).
   */
  char c; int level = 1;
  do { /* Copy characters one by one, from the start of the current
	* substitution field within the original glob pattern, to the
	* appropriate location in the pattern which will be presented
	* to glob_match().  The initial part of this copy represents
	* the substitution for the outermost level of brace bounded
	* expression, terminating at either the closing brace or at
	* any intervening comma at this level; however...
	*/
       if( (c = *++src) == glob_escape_char )
       { /* ...any escaped character must be copied verbatim, without
	  * being considered as a possible substitution terminator...
	  */
	 *dest++ = c; *dest++ = c = *++src;
	 /* ...while taking care not to overrun the ultimate string
	  * terminator of the original pattern.
	  */
	 if( c != '\0' ) c = *++src;
       }
       /* Provided it has not been escaped, any closing brace, or a
	* comma separator at the outer level only, results in closure
	* of a brace nesting level...
	*/
       if( (c == '}') || ((c == ',') && (level == 1)) ) --level;
       /*
	* ...while an opening brace creates a new (inner) level.
	*/
       else if( c == '{' ) ++level;

       /* Provided we have not closed the outermost brace expression
	* level, complete the copy of the current character within
	* the substitution...
	*/
       if( level > 0 ) *dest++ = c;
       /* ...continuing to the next character, until the outermost
	* level has been reached, or the original pattern string
	* has been exhausted.
	*/
     } while( (level > 0) && (c != '\0') );

  /* Save a reference to the point, within the original pattern,
   * where the current substitution ended, and thus where the next
   * iteration (if any) is to begin.
   */
  *resume = src;

  /* Complete construction of a candidate pattern, to be passed
   * to glob_match(), by copying any characters which follow the
   * closing brace of the initial brace bounded expression within
   * the original pattern; thus...
   */
  if( c != '\0' )
  { /* ...when any characters are present, beyond the current
     * resume point...
     */
    if( c == ',' )
    { /* ...and when any of these represent a substitution which
       * is to be made in a subsequent iteration...
       */
      level = 1;
      do { /* ...we simply skip over all characters, up to and
	    * including the closing brace at the outermost level
	    * of expression, (while once again taking care not
	    * to overrun the string terminator)...
	    */
	   if( c != '\0' ) ++src;
	   /* ...and once again, honouring escapes which may be
	    * intended to force literal interpretation of '{'...
	    */
	   while( (*src == glob_escape_char) && (*++src != '\0') ) ++src;
	   /*
	    * ...and simply ignoring any nested (inner) brace
	    * bounded expression...
	    */
	   if( *src == '{' ) ++level; else if( *src == '}' ) --level;
	   /*
	    * ...we continue skipping, until we find the closing
	    * brace at the outermost level of the expression, or
	    * we have have exhausted the original pattern.
	    */
	 } while( ((c = *src) != '\0') && (level > 0) );
    }
    /* Finally...
     */
    if( level == 0 )
    { /* ...when we've skipped to the closing brace ... (checking
       * that we didn't exhaust the original pattern is belt and
       * braces here, because *src should be '}') ... we now skip
       * past it...
       */
      if( c != '\0' ) ++src;
      /* ...we simply copy all further characters from the original
       * pattern, without consideration that there may be any further
       * possible expansion; (this will be picked up by recursion).
       */
      do { *dest++ = *src; } while( *src++ != '\0' );
    }
    else
      /* Alternatively, when the closing brace has not been found,
       * (which implies exhaustion of the original pattern), we
       * must ensure that the expanded copy is terminated.
       */
      *dest = '\0';
  }
  /* Regardless, we return the residual brace expansion level; zero
   * indicates successful expansion; > zero is an error, indicating
   * one (or more) unmatched opening braces.
   */
  return level;
}

static int
glob_match( const char *pattern, int flags, int (*errfn)(), glob_t *gl_buf )
{
  /* Local helper function; it provides the backbone of the glob()
   * implementation, recursively decomposing the pattern into separate
   * globbable path components, to collect the union of all possible
   * matches to the pattern, in all possible matching directories.
   *
   * At the outset, assume that this will succeed.
   */
  int status = GLOB_SUCCESS;

  /* To handle the GNU specific GLOB_BRACE option, we need a
   * recursive preamble to the bare glob_match() strategy; when
   * GLOB_BRACE expansion is specified...
   */
  int brace_option;
  if( (brace_option = flags & GLOB_BRACE) == GLOB_BRACE )
  {
    /* ...we recursively parse the original pattern, so as to
     * decompose it into a series of substitute patterns, each
     * of which represents one pattern expansion to which glob
     * matching is applied in turn, such that the aggregate of
     * matches for the series represents all possible matches
     * for all possible expansions of the original pattern.
     */
    const char *src = pattern;
    char c, sub_pattern[1 + strlen( pattern )], *dest = sub_pattern;

    /* We begin by initialising the prefix portion which is
     * common to all substitute patterns...
     */
    do { /* ...by copying characters one at a time, from the
	  * original pattern to the substitute pattern buffer...
	  */
	 while( *src == glob_escape_char )
	 {
	   /* ...ensuring that all escaped characters are
	    * copied verbatim, without consideration as a
	    * possible brace expression initiator...
	    */
	   *dest++ = *src++;
	   /*
	    * ...but taking care that we don't overrun the
	    * original pattern's string terminator...
	    */
	   if( *src != '\0') *dest++ = *src++;
	 }
	 if( (c = *src) == '{' )
	 {
	   /* We've found the opening brace of a possible brace
	    * expansion; perform a look-ahead, to confirm that it
	    * is actually expandible, (i.e. its matching closing
	    * brace is present, and there is at least one comma
	    * separator at its initial nesting level).
	    */
	   const char *scan = src; int level = 1, comma = ',';
	   do {
		/* We already know that the first character in
		 * the look-ahead range is '{', so we start our
		 * look-ahead scan at the following character in
		 * level one, proceeding until we either find the
		 * corresponding '}', or we run out of scannable
		 * characters; in passing, note whether or not
		 * we find a comma separator at level one.
		 */
		switch( *++scan )
		{
		  /* While scanning, any further opening brace
		   * characters take us to one higher level of
		   * expression nesting, whereas closing braces
		   * bring us back to one lower level, until we
		   * ultimately drop out of level one.
		   */
		  case '{': ++level; break;
		  case '}': --level; break;

		  /* If we find a comma...
		   */
		  case ',':
		    /* ...and it appears at expression nesting
		     * level one, we note that it is associated
		     * with an opening brace, (this doesn't imply
		     * character replacement; merely association,
		     * and at any other nesting level, we assign
		     * no significance to the comma).
		     */
		    if( level == 1 ) comma = '{';
		    break;

		  /* If we run out of scannable characters, (i.e.
		   * we find a NUL terminator)...
		   */
		  case '\0':
		    /* ...then the expression is malformed; drop
		     * any previously noted association for comma,
		     * and forcibly close all open nesting levels,
		     * so aborting the expression scan.
		     */
		    comma = ','; level = 0;
		    break;

		  /* Other than the above...
		   */
		  default:
		    /* ...we must handle escapes for any of these
		     * special characters, while again taking care
		     * to avoid overrunning the NUL terminator.
		     */
		    if( (*scan == glob_escape_char) && (scan[1] != '\0') )
		      ++scan;
		}
		/* We continue this look-ahead until we either exit
		 * the level one scan naturally, (on detection of its
		 * closing '}'), or because we ran out of characters
		 * to scan, (i.e. we found a NUL terminator).
		 */
	      } while( level > 0 );

	   /* In the event that we have found an opening brace, but
	    * no associated comma separator, (or worse, no matching
     	    * closing brace), then our "comma" variable will reflect
	    * an unsuccessful expression validation; since it will
	    * otherwise have mapped an association to the opening
	    * brace, we can simply update "c" to match.
	    */
	   c = comma;
	 }
	 /* In the event that we didn't find a well-formed brace
	  * expression, (including at least one comma separator at
	  * nesting level one), then we must simply store whatever
	  * character we did find at the current pattern offset...
	  */
	 if( c != '{' ) *dest++ = *src++;
	 /*
	  * ...repeating until we either exhaust the original
	  * pattern, or we do find an opening brace which does
	  * have an associated level one comma separator.
	  */
       } while( (c != '\0') && (c != '{') );

    /* After copying the prefix, (which may represent the entire
     * pattern)...
     */
    if( c == '{' )
      /* ...when there is a brace bounded expression to expand...
       */
      do { /* ...iterate to construct each of its expansions in
	    * turn, (together with any common suffix), and...
	    */
	   if( glob_brace_expand( dest, src, &src ) == 0 )
	   {
	     /* ...on success, note that there may be further
	      * embedded brace bounded sub-expressions; recurse
	      * to achieve full expansion...
	      */
	     status = glob_match( sub_pattern, flags, errfn, gl_buf );
	     /*
	      * ...and ensure that matches to all expansions
	      * after the first, will be appended.
	      */
	     flags |= GLOB_APPEND;
	   }
	   else
	   { /* Brace expansion failed, (which implies an opening
	      * brace with no matching closing brace); bail out.
	      *
	      * FIXME: if errfn is specified (not NULL), perhaps we
	      * should invoke it (but how best?  POSIX says it is to
	      * be invoked when pattern resolves to a directory which
	      * cannot be opened, or cannot be read; maybe pass the
	      * original failing pattern, with errno = EINVAL?).
	      */
	     status = GLOB_ABORTED;
	   }
	   /* Repeat iteration until all specified substitutions
	    * for the current expression have been processed, (or
	    * aborted).
	    */
	 } while( (status != GLOB_ABORTED) && (*src == ',') );

    else
      /* The current brace expression has been reduced to its final
       * form, (with no further expansion pending); release it for
       * fall-through glob matching.
       */
      brace_option = 0;
  }

  /* On falling through brace expansion, (if any)...
   */
  if( brace_option == 0 )
  {
    /* ...we have exactly one pattern, with no possible expansions
     * of brace expressions, to be globbed; (alternate expansions of
     * any brace expressions are processed in alternative recursive
     * invocations of this function).
     */
    glob_t local_gl_buf;

    /* Begin by separating out any path prefix from the glob pattern.
     */
    char dirbuf[1 + strlen( pattern )];
    const char *dir = dirname( memcpy( dirbuf, pattern, sizeof( dirbuf )) );
    char **dirp, preferred_dirsep = GLOB_DIRSEP;

    /* Initialise a temporary local glob_t structure, to capture the
     * intermediate results at the current level of recursion...
     */
    local_gl_buf.gl_offs = 0;
    if( (status = glob_initialise( &local_gl_buf )) != GLOB_SUCCESS )
      /*
       * ...bailing out if unsuccessful.
       */
      return status;

    /* Check if there are any globbing tokens in the path prefix...
     */
    if( is_glob_pattern( dir, flags ) )
      /*
       * ...and recurse to identify all possible matching prefixes,
       * as may be necessary...
       */
      status = glob_match( dir, flags | GLOB_DIRONLY, errfn, &local_gl_buf );

    else
      /* ...or simply store the current prefix, if not.
       */
      status = glob_store_entry( glob_strdup( dir ), &local_gl_buf );

    /* Check nothing has gone wrong, so far...
     */
    if( status != GLOB_SUCCESS )
      /*
       * ...and bail out if necessary.
       */
      return status;

    /* The original "pattern" argument may have included a path name
     * prefix, which we used "dirname()" to isolate.  If there was no
     * such prefix, then "dirname()" would have reported an effective
     * prefix which is identically equal to "."; however, this would
     * also be the case if the prefix was "./" (or ".\\" in the case
     * of a WIN32 host).  Thus, we may deduce that...
     */
    if( glob_is_dirsep( pattern[1] ) || (strcmp( dir, "." ) != 0) )
    {
      /* ...when the prefix is not reported as ".", or even if it is
       * but the original pattern had "./" (or ".\\") as the prefix,
       * then we must adjust to identify the effective pattern with
       * its original prefix stripped away...
       */
      const char *tail = pattern + strlen( dir );
      while( (tail > pattern) && ! glob_is_dirsep( *tail ) )
	--tail;
      while( glob_is_dirsep( *tail ) )
	preferred_dirsep = *tail++;
      pattern = tail;
    }

    else
      /* ...otherwise, we simply note that there was no prefix.
       */
      dir = NULL;

    /* We now have a globbed list of prefix directories, returned from
     * recursive processing, in local_gl_buf.gl_pathv, and we also have
     * a separate pattern which we may attempt to match in each of them;
     * at the outset, we have yet to match this pattern to anything.
     */
    status = GLOB_NOMATCH;

    /* When the caller has enabled the GLOB_NOCHECK option, then in the
     * case of any pattern with no prefix, and which contains no explicit
     * globbing token...
     */
    if( (dir == NULL) && accept_glob_nocheck_match( pattern, flags ) )
    {
      /* ...we prefer to store it as is, without any attempt to find
       * a glob match, (which could also induce a case transliteration
       * on MS-Windows' case-insensitive file system)...
       */
      glob_store_entry( glob_strdup( pattern ), gl_buf );
      status = GLOB_SUCCESS;
    }
    /* ...otherwise we initiate glob matching, to find all possible
     * file system matches for the designated pattern, within each of
     * the identified prefix directory paths.
     */
    else for( dirp = local_gl_buf.gl_pathv; *dirp != NULL; free( *dirp++ ) )
    {
      /* Provided an earlier cycle hasn't scheduled an abort...
       */
      if( status != GLOB_ABORTED )
      {
	/* ...take each candidate directory in turn, and prepare
	 * to collate any matched entities within it...
	 */
	struct glob_collator *collator = NULL;

	/* ...attempt to open the current candidate directory...
	 */
	DIR *dp;
	if( (dp = opendir( *dirp )) != NULL )
	{
	  /* ...and when successful, instantiate a dirent structure...
	   */
	  struct dirent *entry;
	  size_t dirlen = (dir == NULL) ? 0 : strlen( *dirp );
	  while( (entry = readdir( dp )) != NULL )
	  {
	    /* ...into which we read each entry from the candidate
	     * directory, in turn, then...
	     */
	    if( (((flags & GLOB_DIRONLY) == 0) || GLOB_ISDIR( entry ))
	      /*
	       * ...provided we don't require it to be a subdirectory,
	       * or it actually is one...
	       */
	    && (glob_strcmp( pattern, entry->d_name, flags ) == 0)   )
	    {
	      /* ...and it is a globbed match for the pattern, then
	       * we allocate a temporary local buffer of sufficient
	       * size to assemble the matching path name...
	       */
	      char *found;
	      size_t prefix;
	      size_t matchlen = D_NAMLEN( entry );
	      char matchpath[2 + dirlen + matchlen];
	      if( (prefix = dirlen) > 0 )
	      {
		/* ...first copying the prefix, if any,
		 * followed by a directory name separator...
		 */
		memcpy( matchpath, *dirp, dirlen );
		if( ! glob_is_dirsep( matchpath[prefix - 1] ) )
		  matchpath[prefix++] = preferred_dirsep;
	      }
	      /* ...and append the matching dirent entry.
	       */
	      memcpy( matchpath + prefix, entry->d_name, matchlen + 1 );

	      /* Duplicate the content of the temporary buffer to
	       * the heap, for assignment into gl_buf->gl_pathv...
	       */
	      if( (found = glob_strdup( matchpath )) == NULL )
		/*
		 * ...setting the appropriate error code, in the
		 * event that the heap memory has been exhausted.
		 */
		status = GLOB_NOSPACE;

	      else
	      { /* This glob match has been successfully recorded on
		 * the heap, ready for assignment to gl_buf->gl_pathv;
		 * if this is the first match assigned to this gl_buf,
		 * and we haven't trapped any prior error...
		 */
		if( status == GLOB_NOMATCH )
		  /*
		   * ...then record this successful match.
		   */
		  status = GLOB_SUCCESS;

		if( (flags & GLOB_NOSORT) == 0 )
		{
		  /* The results of this glob are to be sorted in
		   * collating sequence order; divert the current
		   * match into the collator.
		   */
		  collator = glob_collate_entry( collator, found, flags );
		}
		else
		{ /* Sorting has been suppressed for this glob;
		   * just add the current match directly into the
		   * result vector at gl_buf->gl_pathv.
		   */
		  glob_store_entry( found, gl_buf );
		}
	      }
	    }
	  }
	  /* When we've processed all of the entries in the current
	   * prefix directory, we may close it.
	   */
	  closedir( dp );
	}
	/* In the event of failure to open the candidate prefix directory...
	 */
	else if( (flags & GLOB_ERR) || ((errfn != NULL) && errfn(*dirp, errno)) )
	  /*
	   * ...and when the caller has set the GLOB_ERR flag, or has provided
	   * an error handler which returns non-zero for the failure condition,
	   * then we schedule an abort.
	   */
	  status = GLOB_ABORTED;

	/* When we diverted the glob results for collation...
	 */
	if( collator != NULL )
	  /*
	   * ...then we redirect them to gl_buf->gl_pathv now, before we
	   * begin a new cycle, to process any further prefix directories
	   * which may have been identified; note that we do this even if
	   * we scheduled an abort, so that we may return any results we
	   * may have already collected before the error occurred.
	   */
	  glob_store_collated_entries( collator, gl_buf );
      }
    }
    /* Finally, free the memory block allocated for the results vector
     * in the internal glob buffer, to avoid leaking memory, before we
     * return the resultant status code.
     */
    free( local_gl_buf.gl_pathv );
  }
  return status;
}

#define GLOB_INIT	(1 << __GLOB_FLAG_OFFSET_HIGH_WATER_MARK)
#define GLOB_FREE	(2 << __GLOB_FLAG_OFFSET_HIGH_WATER_MARK)

GLOB_INLINE int glob_signed( const char *check, const char *magic )
{
  /* Inline helper function, used exclusively by the glob_registry()
   * function, to confirm that the gl_magic field within a glob_t data
   * structure has been set, to indicate a properly initialised state.
   *
   * FIXME: we'd like to be able to verify the content at "check"
   * against the signature at "magic", but "check" is likely to be
   * an uninitialised pointer, and MS-Windows lamely crashes when the
   * memory it might appear to address cannot be read.  There may be a
   * way we could trap, and effectively handle, the resulting access
   * violation, (likely restricted to WinXP and later); in the absence
   * of a suitable handler, we must restrict our check to require that
   * "check" is a strict alias for "magic".  This will lose, if we have
   * multiple copies of "glob" loaded via distinct DLLs, and we pass a
   * "glob_t" entity which has been initialised in one DLL across the
   * boundary of another; for now, however, checking for strict pointer
   * aliasing seems to be the only reliably safe option available.
   */
  return (check == magic) ? 0 : 1;
}

static glob_t *glob_registry( int request, glob_t *gl_data )
{
  /* Helper function to verify proper registration (initialisation)
   * of a glob_t data structure, prior to first use; it also provides
   * the core implementation for the globfree() function.
   */
  static const char *glob_magic = "glob-1.0-mingw32";

  /* We must be prepared to handle either of...
   */
  switch( request )
  {
    /* ...a registration (initialisation) request...
     */
    case GLOB_INIT:
      if( glob_signed( gl_data->gl_magic, glob_magic ) != 0 )
      {
	/* The gl_magic field doesn't (yet) indicate that the
	 * data structure has been initialised; assume that this
	 * is first use, and initialise it now.
	 */
	glob_initialise( gl_data );
	gl_data->gl_magic = (void *)(glob_magic);
      }
      break;

    /* ...or a de-registration (globfree()) request; here we
     * perform a sanity check, to ensure that the passed glob_t
     * structure is a valid, previously initialised structure,
     * before we attempt to free it.
     */
    case GLOB_FREE:
      if( glob_signed( gl_data->gl_magic, glob_magic ) == 0 )
      {
	/* On passing the sanity check, we may proceed to free
	 * all dynamically (strdup) allocated string buffers in
	 * the gl_pathv list, and the reference pointer table
	 * itself, thus completing the globfree() activity.
	 */
	int base = gl_data->gl_offs;
	int argc = gl_data->gl_pathc;
	while( argc-- > 0 )
	  free( gl_data->gl_pathv[base++] );
	free( gl_data->gl_pathv );
      }
  }
  /* In either case, we return the original glob_t data pointer.
   */
  return gl_data;
}

int
__mingw_glob( const char *pattern, int flags, int (*errfn)(), glob_t *gl_data )
{
  /* Module entry point for the glob() function.
   */
  int status;

  /* First, consult the glob "registry", to ensure that the
   * glob data structure passed by the caller, has been properly
   * initialised.  (Note that this implementation gratuitously uses
   * gl_data->gl_offs, irrespective of specification of GLOB_DOOFFS
   * in the flags; while the user must accept responsibility for the
   * initialisation of gl_data->gl_offs when specifying GLOB_DOOFFS,
   * this is not the case when GLOB_DOOFFS is not specified; in the
   * latter case, WE must assume the responsibility, ensuring that
   * the required zero value is assigned BEFORE registration).
   */
  if( (gl_data != NULL) && ((flags & GLOB_DOOFFS) == 0) )
    gl_data->gl_offs = 0;

  /* With this pre-registration requirement satisfied, we may now
   * proceed to register the provided gl_data structure.
   */
  gl_data = glob_registry( GLOB_INIT, gl_data );

  /* The actual globbing function is performed by glob_match()...
   */
  status = glob_match( pattern, flags, errfn, gl_data );
  if( (status == GLOB_NOMATCH) && ((flags & GLOB_NOCHECK) != 0) )
    /*
     * ...ultimately delegating to glob_strdup() and glob_store_entry()
     * to handle any unmatched globbing pattern which the user specified
     * options may require to be stored anyway.
     */
    glob_store_entry( glob_strdup( pattern ), gl_data );

  /* We always return the status reported by glob_match().
   */
  return status;
}

void
__mingw_globfree( glob_t *gl_data )
{
  /* Module entry point for globfree() function; the activity is
   * entirely delegated to the glob "registry" helper function.
   */
  glob_registry( GLOB_FREE, gl_data );
}

/* $RCSfile$: end of file */
