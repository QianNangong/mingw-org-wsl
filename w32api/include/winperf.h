/*
 * winperf.h
 *
 * Windows Performance Counters API.
 *
 * $Id$
 *
 * Written by Anders Norlander <anorland@hem2.passagen.se>
 * Copyright (C) 1998, 1999, 2002, 2021, 2022, MinGW.OSDN Project
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
#ifndef _WINPERF_H
#pragma GCC system_header
#define _WINPERF_H

#include <winbase.h>

_BEGIN_C_DECLS

#define PERF_DATA_VERSION					 1
#define PERF_DATA_REVISION					 1
#define PERF_NO_INSTANCES				       (-1)
#define PERF_SIZE_DWORD 					 0
#define PERF_SIZE_LARGE 				       256
#define PERF_SIZE_ZERO					       512
#define PERF_SIZE_VARIABLE_LEN				       768
#define PERF_TYPE_NUMBER					 0
#define PERF_TYPE_COUNTER				      1024
#define PERF_TYPE_TEXT					      2048
#define PERF_TYPE_ZERO					     0xC00
#define PERF_NUMBER_HEX 					 0
#define PERF_NUMBER_DECIMAL				   0x10000
#define PERF_NUMBER_DEC_1000				   0x20000
#define PERF_COUNTER_VALUE					 0
#define PERF_COUNTER_RATE				   0x10000
#define PERF_COUNTER_FRACTION				   0x20000
#define PERF_COUNTER_BASE				   0x30000
#define PERF_COUNTER_ELAPSED				   0x40000
#define PERF_COUNTER_QUEUELEN				   0x50000
#define PERF_COUNTER_HISTOGRAM				   0x60000
#define PERF_TEXT_UNICODE					 0
#define PERF_TEXT_ASCII 				   0x10000
#define PERF_TIMER_TICK 					 0
#define PERF_TIMER_100NS				  0x100000
#define PERF_OBJECT_TIMER				  0x200000
#define PERF_DELTA_COUNTER				  0x400000
#define PERF_DELTA_BASE 				  0x800000
#define PERF_INVERSE_COUNTER				 0x1000000
#define PERF_MULTI_COUNTER				 0x2000000
#define PERF_DISPLAY_NO_SUFFIX					 0
#define PERF_DISPLAY_PER_SEC				0x10000000
#define PERF_DISPLAY_PERCENT				0x20000000
#define PERF_DISPLAY_SECONDS				0x30000000
#define PERF_DISPLAY_NOSHOW				0x40000000
#define PERF_COUNTER_HISTOGRAM_TYPE			0x80000000
#define PERF_NO_UNIQUE_ID				       (-1)
#define PERF_DETAIL_NOVICE				       100
#define PERF_DETAIL_ADVANCED				       200
#define PERF_DETAIL_EXPERT				       300
#define PERF_DETAIL_WIZARD				       400

#define PERF_COUNTER_COUNTER			\
(  PERF_SIZE_DWORD				\
 | PERF_TYPE_COUNTER				\
 | PERF_COUNTER_RATE				\
 | PERF_TIMER_TICK				\
 | PERF_DELTA_COUNTER				\
 | PERF_DISPLAY_PER_SEC 			\
)

#define PERF_COUNTER_TIMER			\
(  PERF_SIZE_LARGE				\
 | PERF_TYPE_COUNTER				\
 | PERF_COUNTER_RATE				\
 | PERF_TIMER_TICK				\
 | PERF_DELTA_COUNTER				\
 | PERF_DISPLAY_PERCENT 			\
)

#define PERF_COUNTER_QUEUELEN_TYPE		\
(  PERF_SIZE_DWORD				\
 | PERF_TYPE_COUNTER				\
 | PERF_COUNTER_QUEUELEN			\
 | PERF_TIMER_TICK				\
 | PERF_DELTA_COUNTER				\
 | PERF_DISPLAY_NO_SUFFIX			\
)

#define PERF_COUNTER_BULK_COUNT 		\
(  PERF_SIZE_LARGE				\
 | PERF_TYPE_COUNTER				\
 | PERF_COUNTER_RATE				\
 | PERF_TIMER_TICK				\
 | PERF_DELTA_COUNTER				\
 | PERF_DISPLAY_PER_SEC 			\
)

#define PERF_COUNTER_TEXT			\
(  PERF_SIZE_VARIABLE_LEN			\
 | PERF_TYPE_TEXT				\
 | PERF_TEXT_UNICODE				\
 | PERF_DISPLAY_NO_SUFFIX			\
)

#define PERF_COUNTER_RAWCOUNT			\
(  PERF_SIZE_DWORD				\
 | PERF_TYPE_NUMBER				\
 | PERF_NUMBER_DECIMAL				\
 | PERF_DISPLAY_NO_SUFFIX			\
)

#define PERF_COUNTER_LARGE_RAWCOUNT		\
(  PERF_SIZE_LARGE				\
 | PERF_TYPE_NUMBER				\
 | PERF_NUMBER_DECIMAL				\
 | PERF_DISPLAY_NO_SUFFIX			\
)

#define PERF_COUNTER_RAWCOUNT_HEX		\
(  PERF_SIZE_DWORD				\
 | PERF_TYPE_NUMBER				\
 | PERF_NUMBER_HEX				\
 | PERF_DISPLAY_NO_SUFFIX			\
)

#define PERF_COUNTER_LARGE_RAWCOUNT_HEX 	\
(  PERF_SIZE_LARGE				\
 | PERF_TYPE_NUMBER				\
 | PERF_NUMBER_HEX				\
 | PERF_DISPLAY_NO_SUFFIX			\
)

#define PERF_SAMPLE_FRACTION			\
(  PERF_SIZE_DWORD				\
 | PERF_TYPE_COUNTER				\
 | PERF_COUNTER_FRACTION			\
 | PERF_DELTA_COUNTER				\
 | PERF_DELTA_BASE				\
 | PERF_DISPLAY_PERCENT 			\
)

#define PERF_SAMPLE_COUNTER			\
(  PERF_SIZE_DWORD				\
 | PERF_TYPE_COUNTER				\
 | PERF_COUNTER_RATE				\
 | PERF_TIMER_TICK				\
 | PERF_DELTA_COUNTER				\
 | PERF_DISPLAY_NO_SUFFIX			\
)

#define PERF_COUNTER_NODATA			\
(  PERF_SIZE_ZERO				\
 | PERF_DISPLAY_NOSHOW				\
)

#define PERF_COUNTER_TIMER_INV			\
(  PERF_SIZE_LARGE				\
 | PERF_TYPE_COUNTER				\
 | PERF_COUNTER_RATE				\
 | PERF_TIMER_TICK				\
 | PERF_DELTA_COUNTER				\
 | PERF_INVERSE_COUNTER 			\
 | PERF_DISPLAY_PERCENT 			\
)

#define PERF_SAMPLE_BASE			\
(  PERF_SIZE_DWORD				\
 | PERF_TYPE_COUNTER				\
 | PERF_COUNTER_BASE				\
 | PERF_DISPLAY_NOSHOW				\
 | 1						\
)

#define PERF_AVERAGE_TIMER			\
(  PERF_SIZE_DWORD				\
 | PERF_TYPE_COUNTER				\
 | PERF_COUNTER_FRACTION			\
 | PERF_DISPLAY_SECONDS 			\
)

#define PERF_AVERAGE_BASE			\
(  PERF_SIZE_DWORD				\
 | PERF_TYPE_COUNTER				\
 | PERF_COUNTER_BASE				\
 | PERF_DISPLAY_NOSHOW				\
 | 2						\
)

#define PERF_AVERAGE_BULK			\
(  PERF_SIZE_LARGE				\
 | PERF_TYPE_COUNTER				\
 | PERF_COUNTER_FRACTION			\
 | PERF_DISPLAY_NOSHOW				\
)

#define PERF_100NSEC_TIMER			\
(  PERF_SIZE_LARGE				\
 | PERF_TYPE_COUNTER				\
 | PERF_COUNTER_RATE				\
 | PERF_TIMER_100NS				\
 | PERF_DELTA_COUNTER				\
 | PERF_DISPLAY_PERCENT 			\
)

#define PERF_100NSEC_TIMER_INV			\
(  PERF_SIZE_LARGE				\
 | PERF_TYPE_COUNTER				\
 | PERF_COUNTER_RATE				\
 | PERF_TIMER_100NS				\
 | PERF_DELTA_COUNTER				\
 | PERF_INVERSE_COUNTER 			\
 | PERF_DISPLAY_PERCENT 			\
)

#define PERF_COUNTER_MULTI_TIMER		\
(  PERF_SIZE_LARGE				\
 | PERF_TYPE_COUNTER				\
 | PERF_COUNTER_RATE				\
 | PERF_DELTA_COUNTER				\
 | PERF_TIMER_TICK				\
 | PERF_MULTI_COUNTER				\
 | PERF_DISPLAY_PERCENT 			\
)

#define PERF_COUNTER_MULTI_TIMER_INV		\
(  PERF_SIZE_LARGE				\
 | PERF_TYPE_COUNTER				\
 | PERF_COUNTER_RATE				\
 | PERF_DELTA_COUNTER				\
 | PERF_MULTI_COUNTER				\
 | PERF_TIMER_TICK				\
 | PERF_INVERSE_COUNTER 			\
 | PERF_DISPLAY_PERCENT 			\
)

#define PERF_COUNTER_MULTI_BASE 		\
(  PERF_SIZE_LARGE				\
 | PERF_TYPE_COUNTER				\
 | PERF_COUNTER_BASE				\
 | PERF_MULTI_COUNTER				\
 | PERF_DISPLAY_NOSHOW				\
)

#define PERF_100NSEC_MULTI_TIMER		\
(  PERF_SIZE_LARGE				\
 | PERF_TYPE_COUNTER				\
 | PERF_DELTA_COUNTER				\
 | PERF_COUNTER_RATE				\
 | PERF_TIMER_100NS				\
 | PERF_MULTI_COUNTER				\
 | PERF_DISPLAY_PERCENT 			\
)

#define PERF_100NSEC_MULTI_TIMER_INV		\
(  PERF_SIZE_LARGE				\
 | PERF_TYPE_COUNTER				\
 | PERF_DELTA_COUNTER				\
 | PERF_COUNTER_RATE				\
 | PERF_TIMER_100NS				\
 | PERF_MULTI_COUNTER				\
 | PERF_INVERSE_COUNTER 			\
 | PERF_DISPLAY_PERCENT 			\
)

#define PERF_RAW_FRACTION			\
(  PERF_SIZE_DWORD				\
 | PERF_TYPE_COUNTER				\
 | PERF_COUNTER_FRACTION			\
 | PERF_DISPLAY_PERCENT 			\
)

#define PERF_RAW_BASE				\
(  PERF_SIZE_DWORD				\
 | PERF_TYPE_COUNTER				\
 | PERF_COUNTER_BASE				\
 | PERF_DISPLAY_NOSHOW				\
 | 3						\
)

#define PERF_ELAPSED_TIME			\
(  PERF_SIZE_LARGE				\
 | PERF_TYPE_COUNTER				\
 | PERF_COUNTER_ELAPSED 			\
 | PERF_OBJECT_TIMER				\
 | PERF_DISPLAY_SECONDS 			\
)

typedef
struct _PERF_DATA_BLOCK
{ WCHAR 		Signature[4];
  DWORD 		LittleEndian;
  DWORD 		Version;
  DWORD 		Revision;
  DWORD 		TotalByteLength;
  DWORD 		HeaderLength;
  DWORD 		NumObjectTypes;
  LONG			DefaultObject;
  SYSTEMTIME		SystemTime;
  LARGE_INTEGER 	PerfTime;
  LARGE_INTEGER 	PerfFreq;
  LARGE_INTEGER 	PerfTime100nSec;
  DWORD 		SystemNameLength;
  DWORD 		SystemNameOffset;
} PERF_DATA_BLOCK, *PPERF_DATA_BLOCK;

typedef
struct _PERF_OBJECT_TYPE
{ DWORD 		TotalByteLength;
  DWORD 		DefinitionLength;
  DWORD 		HeaderLength;
  DWORD 		ObjectNameTitleIndex;
  LPWSTR		ObjectNameTitle;
  DWORD 		ObjectHelpTitleIndex;
  LPWSTR		ObjectHelpTitle;
  DWORD 		DetailLevel;
  DWORD 		NumCounters;
  LONG			DefaultCounter;
  LONG			NumInstances;
  DWORD 		CodePage;
  LARGE_INTEGER 	PerfTime;
  LARGE_INTEGER 	PerfFreq;
} PERF_OBJECT_TYPE, *PPERF_OBJECT_TYPE;

typedef
struct _PERF_COUNTER_DEFINITION
{ DWORD 		ByteLength;
  DWORD 		CounterNameTitleIndex;
  LPWSTR		CounterNameTitle;
  DWORD 		CounterHelpTitleIndex;
  LPWSTR		CounterHelpTitle;
  LONG			DefaultScale;
  DWORD 		DetailLevel;
  DWORD 		CounterType;
  DWORD 		CounterSize;
  DWORD 		CounterOffset;
} PERF_COUNTER_DEFINITION, *PPERF_COUNTER_DEFINITION;

typedef
struct _PERF_INSTANCE_DEFINITION
{ DWORD 		ByteLength;
  DWORD 		ParentObjectTitleIndex;
  DWORD 		ParentObjectInstance;
  LONG			UniqueID;
  DWORD 		NameOffset;
  DWORD 		NameLength;
} PERF_INSTANCE_DEFINITION, *PPERF_INSTANCE_DEFINITION;

typedef
struct _PERF_COUNTER_BLOCK
{ DWORD 		ByteLength;
} PERF_COUNTER_BLOCK, *PPERF_COUNTER_BLOCK;

typedef DWORD (CALLBACK PM_OPEN_PROC)(LPWSTR);
typedef DWORD (CALLBACK PM_COLLECT_PROC)(LPWSTR, PVOID*, PDWORD, PDWORD);
typedef DWORD (CALLBACK PM_CLOSE_PROC)(void);

_END_C_DECLS

#endif	/* !_WINPERF_H: $RCSfile$: end of file */
