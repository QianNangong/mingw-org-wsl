/*
 * winnls.h
 *
 * Windows National Language Support API definitions.
 *
 * $Id$
 *
 * Written by Anders Norlander <anorland@hem2.passagen.se>
 * Copyright (C) 1998-2000, 2002-2004, 2006, 2009, 2019, MinGW.org Project.
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
#ifndef _WINNLS_H
#pragma GCC system_header
#define _WINNLS_H

#include <winbase.h>

_BEGIN_C_DECLS

#define MAX_LEADBYTES						12
#define MAX_DEFAULTCHAR 					 2
#define LOCALE_NOUSEROVERRIDE				0x80000000
#define LOCALE_USE_CP_ACP				0x40000000

#if WINVER >= _WIN32_WINNT_NT4		/* also >= Win95 */

#define LOCALE_RETURN_NUMBER				0x20000000

#endif	/* >= WinNT4 / Win95 */

#define LOCALE_ILANGUAGE					 1
#define LOCALE_SLANGUAGE					 2
#define LOCALE_SENGLANGUAGE				    0x1001
#define LOCALE_SABBREVLANGNAME					 3
#define LOCALE_SNATIVELANGNAME					 4
#define LOCALE_ICOUNTRY 					 5
#define LOCALE_SCOUNTRY 					 6
#define LOCALE_SENGCOUNTRY				    0x1002
#define LOCALE_SABBREVCTRYNAME					 7
#define LOCALE_SNATIVECTRYNAME					 8
#define LOCALE_IDEFAULTLANGUAGE 				 9
#define LOCALE_IDEFAULTCOUNTRY					10
#define LOCALE_IDEFAULTCODEPAGE 				11
#define LOCALE_IDEFAULTANSICODEPAGE			    0x1004
#define LOCALE_SLIST						12
#define LOCALE_IMEASURE 					13
#define LOCALE_SDECIMAL 					14
#define LOCALE_STHOUSAND					15
#define LOCALE_SGROUPING					16
#define LOCALE_IDIGITS						17
#define LOCALE_ILZERO						18
#define LOCALE_INEGNUMBER				    0x1010
#define LOCALE_SNATIVEDIGITS					19
#define LOCALE_SCURRENCY					20
#define LOCALE_SINTLSYMBOL					21
#define LOCALE_SMONDECIMALSEP					22
#define LOCALE_SMONTHOUSANDSEP					23
#define LOCALE_SMONGROUPING					24
#define LOCALE_ICURRDIGITS					25
#define LOCALE_IINTLCURRDIGITS					26
#define LOCALE_ICURRENCY					27
#define LOCALE_INEGCURR 					28
#define LOCALE_SDATE						29
#define LOCALE_STIME						30
#define LOCALE_SSHORTDATE					31
#define LOCALE_SLONGDATE					32
#define LOCALE_STIMEFORMAT				    0x1003
#define LOCALE_IDATE						33
#define LOCALE_ILDATE						34
#define LOCALE_ITIME						35
#define LOCALE_ITIMEMARKPOSN				    0x1005
#define LOCALE_ICENTURY 					36
#define LOCALE_ITLZERO						37
#define LOCALE_IDAYLZERO					38
#define LOCALE_IMONLZERO					39
#define LOCALE_S1159						40
#define LOCALE_S2359						41
#define LOCALE_ICALENDARTYPE				    0x1009
#define LOCALE_IOPTIONALCALENDAR			    0x100B
#define LOCALE_IFIRSTDAYOFWEEK				    0x100C
#define LOCALE_IFIRSTWEEKOFYEAR 			    0x100D
#define LOCALE_SDAYNAME1					42
#define LOCALE_SDAYNAME2					43
#define LOCALE_SDAYNAME3					44
#define LOCALE_SDAYNAME4					45
#define LOCALE_SDAYNAME5					46
#define LOCALE_SDAYNAME6					47
#define LOCALE_SDAYNAME7					48
#define LOCALE_SABBREVDAYNAME1					49
#define LOCALE_SABBREVDAYNAME2					50
#define LOCALE_SABBREVDAYNAME3					51
#define LOCALE_SABBREVDAYNAME4					52
#define LOCALE_SABBREVDAYNAME5					53
#define LOCALE_SABBREVDAYNAME6					54
#define LOCALE_SABBREVDAYNAME7					55
#define LOCALE_SMONTHNAME1					56
#define LOCALE_SMONTHNAME2					57
#define LOCALE_SMONTHNAME3					58
#define LOCALE_SMONTHNAME4					59
#define LOCALE_SMONTHNAME5					60
#define LOCALE_SMONTHNAME6					61
#define LOCALE_SMONTHNAME7					62
#define LOCALE_SMONTHNAME8					63
#define LOCALE_SMONTHNAME9					64
#define LOCALE_SMONTHNAME10					65
#define LOCALE_SMONTHNAME11					66
#define LOCALE_SMONTHNAME12					67
#define LOCALE_SMONTHNAME13				    0x100E
#define LOCALE_SABBREVMONTHNAME1				68
#define LOCALE_SABBREVMONTHNAME2				69
#define LOCALE_SABBREVMONTHNAME3				70
#define LOCALE_SABBREVMONTHNAME4				71
#define LOCALE_SABBREVMONTHNAME5				72
#define LOCALE_SABBREVMONTHNAME6				73
#define LOCALE_SABBREVMONTHNAME7				74
#define LOCALE_SABBREVMONTHNAME8				75
#define LOCALE_SABBREVMONTHNAME9				76
#define LOCALE_SABBREVMONTHNAME10				77
#define LOCALE_SABBREVMONTHNAME11				78
#define LOCALE_SABBREVMONTHNAME12				79
#define LOCALE_SABBREVMONTHNAME13			    0x100F
#define LOCALE_SPOSITIVESIGN					80
#define LOCALE_SNEGATIVESIGN					81
#define LOCALE_IPOSSIGNPOSN					82
#define LOCALE_INEGSIGNPOSN					83
#define LOCALE_IPOSSYMPRECEDES					84
#define LOCALE_IPOSSEPBYSPACE					85
#define LOCALE_INEGSYMPRECEDES					86
#define LOCALE_INEGSEPBYSPACE					87
#define LOCALE_FONTSIGNATURE					88
#define LOCALE_SISO639LANGNAME					89
#define LOCALE_SISO3166CTRYNAME 				90
#define LOCALE_SYSTEM_DEFAULT				     0x800
#define LOCALE_USER_DEFAULT				     0x400
#define NORM_IGNORECASE 					 1
#define NORM_IGNOREKANATYPE				     65536
#define NORM_IGNORENONSPACE					 2
#define NORM_IGNORESYMBOLS					 4
#define NORM_IGNOREWIDTH				    131072
#define SORT_STRINGSORT 				      4096
#define LCMAP_LOWERCASE 				0x00000100
#define LCMAP_UPPERCASE 				0x00000200
#define LCMAP_SORTKEY					0x00000400
#define LCMAP_BYTEREV					0x00000800
#define LCMAP_HIRAGANA					0x00100000
#define LCMAP_KATAKANA					0x00200000
#define LCMAP_HALFWIDTH 				0x00400000
#define LCMAP_FULLWIDTH 				0x00800000
#define LCMAP_LINGUISTIC_CASING 			0x01000000
#define LCMAP_SIMPLIFIED_CHINESE			0x02000000
#define LCMAP_TRADITIONAL_CHINESE			0x04000000
#define ENUM_ALL_CALENDARS				       (-1)
#define DATE_SHORTDATE						 1
#define DATE_LONGDATE						 2
#define DATE_USE_ALT_CALENDAR					 4
#define CP_INSTALLED						 1
#define CP_SUPPORTED						 2
#define LCID_INSTALLED						 1
#define LCID_SUPPORTED						 2
#define LCID_ALTERNATE_SORTS					 4
#define MAP_FOLDCZONE						16
#define MAP_FOLDDIGITS					       128
#define MAP_PRECOMPOSED 					32
#define MAP_COMPOSITE						64
#define CP_ACP							 0
#define CP_OEMCP						 1
#define CP_MACCP						 2
#define CP_THREAD_ACP						 3
#define CP_SYMBOL						42
#define CP_UTF7 					     65000
#define CP_UTF8 					     65001
#define CT_CTYPE1						 1
#define CT_CTYPE2						 2
#define CT_CTYPE3						 4
#define C1_UPPER						 1
#define C1_LOWER						 2
#define C1_DIGIT						 4
#define C1_SPACE						 8
#define C1_PUNCT						16
#define C1_CNTRL						32
#define C1_BLANK						64
#define C1_XDIGIT					       128
#define C1_ALPHA					       256
#define C2_LEFTTORIGHT						 1
#define C2_RIGHTTOLEFT						 2
#define C2_EUROPENUMBER 					 3
#define C2_EUROPESEPARATOR					 4
#define C2_EUROPETERMINATOR					 5
#define C2_ARABICNUMBER 					 6
#define C2_COMMONSEPARATOR					 7
#define C2_BLOCKSEPARATOR					 8
#define C2_SEGMENTSEPARATOR					 9
#define C2_WHITESPACE						10
#define C2_OTHERNEUTRAL 					11
#define C2_NOTAPPLICABLE					 0
#define C3_NONSPACING						 1
#define C3_DIACRITIC						 2
#define C3_VOWELMARK						 4
#define C3_SYMBOL						 8
#define C3_KATAKANA						16
#define C3_HIRAGANA						32
#define C3_HALFWIDTH						64
#define C3_FULLWIDTH					       128
#define C3_IDEOGRAPH					       256
#define C3_KASHIDA					       512
#define C3_LEXICAL					      1024
#define C3_ALPHA					     32768
#define C3_NOTAPPLICABLE					 0
#define TIME_NOMINUTESORSECONDS 				 1
#define TIME_NOSECONDS						 2
#define TIME_NOTIMEMARKER					 4
#define TIME_FORCE24HOURFORMAT					 8
#define MB_PRECOMPOSED						 1
#define MB_COMPOSITE						 2
#define MB_ERR_INVALID_CHARS					 8
#define MB_USEGLYPHCHARS					 4
#define WC_COMPOSITECHECK				       512
#define WC_DISCARDNS						16
#define WC_SEPCHARS						32
#define WC_DEFAULTCHAR						64
#define CTRY_DEFAULT						 0
#define CTRY_ALBANIA					       355
#define CTRY_ALGERIA					       213
#define CTRY_ARGENTINA						54
#define CTRY_ARMENIA					       374
#define CTRY_AUSTRALIA						61
#define CTRY_AUSTRIA						43
#define CTRY_AZERBAIJAN 				       994
#define CTRY_BAHRAIN					       973
#define CTRY_BELARUS					       375
#define CTRY_BELGIUM						32
#define CTRY_BELIZE					       501
#define CTRY_BOLIVIA					       591
#define CTRY_BRAZIL						55
#define CTRY_BRUNEI_DARUSSALAM				       673
#define CTRY_BULGARIA					       359
#define CTRY_CANADA						 2
#define CTRY_CARIBBEAN						 1
#define CTRY_CHILE						56
#define CTRY_COLOMBIA						57
#define CTRY_COSTA_RICA 				       506
#define CTRY_CROATIA					       385
#define CTRY_CZECH					       420
#define CTRY_DENMARK						45
#define CTRY_DOMINICAN_REPUBLIC 				 1
#define CTRY_ECUADOR					       593
#define CTRY_EGYPT						20
#define CTRY_EL_SALVADOR				       503
#define CTRY_ESTONIA					       372
#define CTRY_FAEROE_ISLANDS				       298
#define CTRY_FINLAND					       358
#define CTRY_FRANCE						33
#define CTRY_GEORGIA					       995
#define CTRY_GERMANY						49
#define CTRY_GREECE						30
#define CTRY_GUATEMALA					       502
#define CTRY_HONDURAS					       504
#define CTRY_HONG_KONG					       852
#define CTRY_HUNGARY						36
#define CTRY_ICELAND					       354
#define CTRY_INDIA						91
#define CTRY_INDONESIA						62
#define CTRY_IRAN					       981
#define CTRY_IRAQ					       964
#define CTRY_IRELAND					       353
#define CTRY_ISRAEL					       972
#define CTRY_ITALY						39
#define CTRY_JAMAICA						 1
#define CTRY_JAPAN						81
#define CTRY_JORDAN					       962
#define CTRY_KAZAKSTAN						 7
#define CTRY_KENYA					       254
#define CTRY_KUWAIT					       965
#define CTRY_KYRGYZSTAN 				       996
#define CTRY_LATVIA					       371
#define CTRY_LEBANON					       961
#define CTRY_LIBYA					       218
#define CTRY_LIECHTENSTEIN					41
#define CTRY_LITHUANIA					       370
#define CTRY_LUXEMBOURG 				       352
#define CTRY_MACAU					       853
#define CTRY_MACEDONIA					       389
#define CTRY_MALAYSIA						60
#define CTRY_MALDIVES					       960
#define CTRY_MEXICO						52
#define CTRY_MONACO						33
#define CTRY_MONGOLIA					       976
#define CTRY_MOROCCO					       212
#define CTRY_NETHERLANDS					31
#define CTRY_NEW_ZEALAND					64
#define CTRY_NICARAGUA					       505
#define CTRY_NORWAY						47
#define CTRY_OMAN					       968
#define CTRY_PAKISTAN						92
#define CTRY_PANAMA					       507
#define CTRY_PARAGUAY					       595
#define CTRY_PERU						51
#define CTRY_PHILIPPINES					63
#define CTRY_POLAND						48
#define CTRY_PORTUGAL					       351
#define CTRY_PRCHINA						86
#define CTRY_PUERTO_RICO					 1
#define CTRY_QATAR					       974
#define CTRY_ROMANIA						40
#define CTRY_RUSSIA						 7
#define CTRY_SAUDI_ARABIA				       966
#define CTRY_SERBIA					       381
#define CTRY_SINGAPORE						65
#define CTRY_SLOVAK					       421
#define CTRY_SLOVENIA					       386
#define CTRY_SOUTH_AFRICA					27
#define CTRY_SOUTH_KOREA					82
#define CTRY_SPAIN						34
#define CTRY_SWEDEN						46
#define CTRY_SWITZERLAND					41
#define CTRY_SYRIA					       963
#define CTRY_TAIWAN					       886
#define CTRY_TATARSTAN						 7
#define CTRY_THAILAND						66
#define CTRY_TRINIDAD_Y_TOBAGO					 1
#define CTRY_TUNISIA					       216
#define CTRY_TURKEY						90
#define CTRY_UAE					       971
#define CTRY_UKRAINE					       380
#define CTRY_UNITED_KINGDOM					44
#define CTRY_UNITED_STATES					 1
#define CTRY_URUGUAY					       598
#define CTRY_UZBEKISTAN 					 7
#define CTRY_VENEZUELA						58
#define CTRY_VIET_NAM						84
#define CTRY_YEMEN					       967
#define CTRY_ZIMBABWE					       263
#define CAL_ICALINTVALUE					 1
#define CAL_SCALNAME						 2
#define CAL_IYEAROFFSETRANGE					 3
#define CAL_SERASTRING						 4
#define CAL_SSHORTDATE						 5
#define CAL_SLONGDATE						 6
#define CAL_SDAYNAME1						 7
#define CAL_SDAYNAME2						 8
#define CAL_SDAYNAME3						 9
#define CAL_SDAYNAME4						10
#define CAL_SDAYNAME5						11
#define CAL_SDAYNAME6						12
#define CAL_SDAYNAME7						13
#define CAL_SABBREVDAYNAME1					14
#define CAL_SABBREVDAYNAME2					15
#define CAL_SABBREVDAYNAME3					16
#define CAL_SABBREVDAYNAME4					17
#define CAL_SABBREVDAYNAME5					18
#define CAL_SABBREVDAYNAME6					19
#define CAL_SABBREVDAYNAME7					20
#define CAL_SMONTHNAME1 					21
#define CAL_SMONTHNAME2 					22
#define CAL_SMONTHNAME3 					23
#define CAL_SMONTHNAME4 					24
#define CAL_SMONTHNAME5 					25
#define CAL_SMONTHNAME6 					26
#define CAL_SMONTHNAME7 					27
#define CAL_SMONTHNAME8 					28
#define CAL_SMONTHNAME9 					29
#define CAL_SMONTHNAME10					30
#define CAL_SMONTHNAME11					31
#define CAL_SMONTHNAME12					32
#define CAL_SMONTHNAME13					33
#define CAL_SABBREVMONTHNAME1					34
#define CAL_SABBREVMONTHNAME2					35
#define CAL_SABBREVMONTHNAME3					36
#define CAL_SABBREVMONTHNAME4					37
#define CAL_SABBREVMONTHNAME5					38
#define CAL_SABBREVMONTHNAME6					39
#define CAL_SABBREVMONTHNAME7					40
#define CAL_SABBREVMONTHNAME8					41
#define CAL_SABBREVMONTHNAME9					42
#define CAL_SABBREVMONTHNAME10					43
#define CAL_SABBREVMONTHNAME11					44
#define CAL_SABBREVMONTHNAME12					45
#define CAL_SABBREVMONTHNAME13					46
#define CAL_GREGORIAN						 1
#define CAL_GREGORIAN_US					 2
#define CAL_JAPAN						 3
#define CAL_TAIWAN						 4
#define CAL_KOREA						 5
#define CAL_HIJRI						 6
#define CAL_THAI						 7
#define CAL_HEBREW						 8
#define CAL_GREGORIAN_ME_FRENCH 				 9
#define CAL_GREGORIAN_ARABIC					10
#define CAL_GREGORIAN_XLIT_ENGLISH				11
#define CAL_GREGORIAN_XLIT_FRENCH				12
#define CSTR_LESS_THAN						 1
#define CSTR_EQUAL						 2
#define CSTR_GREATER_THAN					 3
#define LGRPID_INSTALLED					 1
#define LGRPID_SUPPORTED					 2
#define LGRPID_WESTERN_EUROPE					 1
#define LGRPID_CENTRAL_EUROPE					 2
#define LGRPID_BALTIC						 3
#define LGRPID_GREEK						 4
#define LGRPID_CYRILLIC 					 5
#define LGRPID_TURKISH						 6
#define LGRPID_JAPANESE 					 7
#define LGRPID_KOREAN						 8
#define LGRPID_TRADITIONAL_CHINESE				 9
#define LGRPID_SIMPLIFIED_CHINESE				10
#define LGRPID_THAI						11
#define LGRPID_HEBREW						12
#define LGRPID_ARABIC						13
#define LGRPID_VIETNAMESE					14
#define LGRPID_INDIC						15
#define LGRPID_GEORGIAN 					16
#define LGRPID_ARMENIAN 					17
#define IS_HIGH_SURROGATE(c)			(((c) & 0xFC00) == 0xD800)
#define IS_LOW_SURROGATE(c)			(((c) & 0xFC00) == 0xDC00)
#define IS_SURROGATE_PAIR(hc,lc)	(IS_HIGH_SURROGATE(hc) && IS_LOW_SURROGATE(lc))

#if WINVER >= _WIN32_WINDOWS_98		/* also >= Win2K */

#define CAL_SYEARMONTH						47
#define CAL_ITWODIGITYEARMAX					48
#define CAL_NOUSEROVERRIDE			     LOCALE_NOUSEROVERRIDE
#define CAL_RETURN_NUMBER			     LOCALE_RETURN_NUMBER
#define CAL_USE_CP_ACP				     LOCALE_USE_CP_ACP
#define LOCALE_SYEARMONTH				    0x1006
#define LOCALE_SENGCURRNAME				    0x1007
#define LOCALE_SNATIVECURRNAME				    0x1008
#define LOCALE_SSORTNAME				    0x1013

#endif /* >= Win98 / Win2K */

#if _WIN32_WINNT >= _WIN32_WINNT_WIN2K

#define LOCALE_IDEFAULTEBCDICCODEPAGE			    0x1012
#define LOCALE_IDIGITSUBSTITUTION			    0x1014
#define LOCALE_IPAPERSIZE				    0x100A
#define DATE_YEARMONTH						 8
#define DATE_LTRREADING 					16
#define DATE_RTLREADING 					32
#define MAP_EXPAND_LIGATURES				    0x2000
#define WC_NO_BEST_FIT_CHARS				      1024

#endif /* >= Win2K */

#if _WIN32_WINNT >= _WIN32_WINNT_VISTA

#if 0
#define CAL_SSHORTESTDAYNAME1
#define CAL_SSHORTESTDAYNAME2
#define CAL_SSHORTESTDAYNAME3
#define CAL_SSHORTESTDAYNAME4
#define CAL_SSHORTESTDAYNAME5
#define CAL_SSHORTESTDAYNAME6
#define CAL_SSHORTESTDAYNAME7

#define LOCALE_SSHORTESTDAYNAME1
#define LOCALE_SSHORTESTDAYNAME2
#define LOCALE_SSHORTESTDAYNAME3
#define LOCALE_SSHORTESTDAYNAME4
#define LOCALE_SSHORTESTDAYNAME5
#define LOCALE_SSHORTESTDAYNAME6
#define LOCALE_SSHORTESTDAYNAME7
#endif

#define CAL_UMALQURA						23

#if 0
#define LOCALE_SDURATION
#define LOCALE_SIETFLANGUAGE
#define LOCALE_SISO3166CTRYNAME2
#define LOCALE_SISO639LANGNAME2
#define LOCALE_SKEYBOARDSTOINSTALL
#define LOCALE_SNAME
#define LOCALE_SNAN
#define LOCALE_SNEGINFINITY
#define LOCALE_SPOSINFINITY
#define LOCALE_SSCRIPTS
#endif

#endif /* >= WinVista */

#ifndef RC_INVOKED

typedef DWORD LCTYPE;
typedef DWORD CALTYPE;
typedef DWORD CALID;
typedef DWORD LGRPID;
typedef DWORD GEOID;
typedef DWORD GEOTYPE;
typedef DWORD GEOCLASS;

#define CALINFO_ENUMPROC  __AW_SUFFIXED__( CALINFO_ENUMPROC )
typedef BOOL (CALLBACK *CALINFO_ENUMPROCA) (LPSTR);
typedef BOOL (CALLBACK *CALINFO_ENUMPROCW) (LPWSTR);

#define CALINFO_ENUMPROCEX  __AW_SUFFIXED__( CALINFO_ENUMPROCEX )
typedef BOOL (CALLBACK *CALINFO_ENUMPROCEXA) (LPSTR, CALID);
typedef BOOL (CALLBACK *CALINFO_ENUMPROCEXW) (LPWSTR, CALID);

#define LANGUAGEGROUP_ENUMPROC  __AW_SUFFIXED__( LANGUAGEGROUP_ENUMPROC )
typedef BOOL (CALLBACK *LANGUAGEGROUP_ENUMPROCA) (LGRPID, LPSTR, LPSTR, DWORD, LONG_PTR);
typedef BOOL (CALLBACK *LANGUAGEGROUP_ENUMPROCW) (LGRPID, LPWSTR, LPWSTR, DWORD, LONG_PTR);

#define LANGGROUPLOCALE_ENUMPROC  __AW_SUFFIXED__( LANGGROUPLOCALE_ENUMPROC )
typedef BOOL (CALLBACK *LANGGROUPLOCALE_ENUMPROCA) (LGRPID, LCID, LPSTR, LONG_PTR);
typedef BOOL (CALLBACK *LANGGROUPLOCALE_ENUMPROCW) (LGRPID, LCID, LPWSTR, LONG_PTR);

#define UILANGUAGE_ENUMPROC  __AW_SUFFIXED__( UILANGUAGE_ENUMPROC )
typedef BOOL (CALLBACK *UILANGUAGE_ENUMPROCA) (LPSTR, LONG_PTR);
typedef BOOL (CALLBACK *UILANGUAGE_ENUMPROCW) (LPWSTR, LONG_PTR);

#define LOCALE_ENUMPROC  __AW_SUFFIXED__( LOCALE_ENUMPROC )
typedef BOOL (CALLBACK *LOCALE_ENUMPROCA) (LPSTR);
typedef BOOL (CALLBACK *LOCALE_ENUMPROCW) (LPWSTR);

#define CODEPAGE_ENUMPROC  __AW_SUFFIXED__( CODEPAGE_ENUMPROC )
typedef BOOL (CALLBACK *CODEPAGE_ENUMPROCA) (LPSTR);
typedef BOOL (CALLBACK *CODEPAGE_ENUMPROCW) (LPWSTR);

#define DATEFMT_ENUMPROC  __AW_SUFFIXED__( DATEFMT_ENUMPROC )
typedef BOOL (CALLBACK *DATEFMT_ENUMPROCA) (LPSTR);
typedef BOOL (CALLBACK *DATEFMT_ENUMPROCW) (LPWSTR);

#define DATEFMT_ENUMPROCEX  __AW_SUFFIXED__( DATEFMT_ENUMPROCEX )
typedef BOOL (CALLBACK *DATEFMT_ENUMPROCEXA) (LPSTR, CALID);
typedef BOOL (CALLBACK *DATEFMT_ENUMPROCEXW) (LPWSTR, CALID);

#define TIMEFMT_ENUMPROC  __AW_SUFFIXED__( TIMEFMT_ENUMPROC )
typedef BOOL (CALLBACK *TIMEFMT_ENUMPROCA) (LPSTR);
typedef BOOL (CALLBACK *TIMEFMT_ENUMPROCW) (LPWSTR);

typedef BOOL (CALLBACK *GEO_ENUMPROC) (GEOID);

typedef
enum NLS_FUNCTION
{ COMPARE_STRING	 =  1
} NLS_FUNCTION;

enum SYSGEOCLASS
{ GEOCLASS_REGION	 =  14,
  GEOCLASS_NATION	 =  16
};

enum SYSGEOTYPE
{ GEO_NATION		 =  1,
  GEO_LATITUDE,
  GEO_LONGITUDE,
  GEO_ISO2,
  GEO_ISO3,
  GEO_RFC1766,
  GEO_LCID,
  GEO_FRIENDLYNAME,
  GEO_OFFICIALNAME,
  GEO_TIMEZONES,
  GEO_OFFICIALLANGUAGES
};

typedef struct _cpinfo
{ UINT			MaxCharSize;
  BYTE			DefaultChar[MAX_DEFAULTCHAR];
  BYTE			LeadByte[MAX_LEADBYTES];
} CPINFO, *LPCPINFO;

typedef struct _cpinfoexA
{ UINT			MaxCharSize;
  BYTE			DefaultChar[MAX_DEFAULTCHAR];
  BYTE			LeadByte[MAX_LEADBYTES];
  WCHAR 		UnicodeDefaultChar;
  UINT			CodePage;
  CHAR			CodePageName[MAX_PATH];
} CPINFOEXA, *LPCPINFOEXA;

typedef struct _cpinfoexW
{ UINT			MaxCharSize;
  BYTE			DefaultChar[MAX_DEFAULTCHAR];
  BYTE			LeadByte[MAX_LEADBYTES];
  WCHAR 		UnicodeDefaultChar;
  UINT			CodePage;
  WCHAR 		CodePageName[MAX_PATH];
} CPINFOEXW, *LPCPINFOEXW;

typedef __AW_ALIAS__( CPINFOEX ), *LPCPINFOEX;

typedef struct _currencyfmtA
{ UINT			NumDigits;
  UINT			LeadingZero;
  UINT			Grouping;
  LPSTR 		lpDecimalSep;
  LPSTR 		lpThousandSep;
  UINT			NegativeOrder;
  UINT			PositiveOrder;
  LPSTR 		lpCurrencySymbol;
} CURRENCYFMTA, *LPCURRENCYFMTA;

typedef struct _currencyfmtW
{ UINT			NumDigits;
  UINT			LeadingZero;
  UINT			Grouping;
  LPWSTR		lpDecimalSep;
  LPWSTR		lpThousandSep;
  UINT			NegativeOrder;
  UINT			PositiveOrder;
  LPWSTR		lpCurrencySymbol;
} CURRENCYFMTW, *LPCURRENCYFMTW;

typedef __AW_ALIAS__( CURRENCYFMT ), *LPCURRENCYFMT;

typedef struct nlsversioninfo
{ DWORD 		dwNLSVersionInfoSize;
  DWORD 		dwNLSVersion;
  DWORD 		dwDefinedVersion;
} NLSVERSIONINFO, *LPNLSVERSIONINFO;

typedef struct _numberfmtA
{ UINT			NumDigits;
  UINT			LeadingZero;
  UINT			Grouping;
  LPSTR 		lpDecimalSep;
  LPSTR 		lpThousandSep;
  UINT			NegativeOrder;
} NUMBERFMTA, *LPNUMBERFMTA;

typedef struct _numberfmtW
{ UINT			NumDigits;
  UINT			LeadingZero;
  UINT			Grouping;
  LPWSTR		lpDecimalSep;
  LPWSTR		lpThousandSep;
  UINT			NegativeOrder;
} NUMBERFMTW, *LPNUMBERFMTW;

typedef __AW_ALIAS__( NUMBERFMT ), *LPNUMBERFMT;

#define CompareString  __AW_SUFFIXED__( CompareString )
WINBASEAPI WINAPI  int    CompareStringA (LCID, DWORD, LPCSTR, int, LPCSTR, int);
WINBASEAPI WINAPI  int    CompareStringW (LCID, DWORD, LPCWSTR, int, LPCWSTR, int);

WINBASEAPI WINAPI  LCID   ConvertDefaultLocale (LCID);

#define EnumCalendarInfo  __AW_SUFFIXED__( EnumCalendarInfo )
WINBASEAPI WINAPI  BOOL   EnumCalendarInfoA (CALINFO_ENUMPROCA, LCID, CALID, CALTYPE);
WINBASEAPI WINAPI  BOOL   EnumCalendarInfoW (CALINFO_ENUMPROCW, LCID, CALID, CALTYPE);

#define EnumDateFormats  __AW_SUFFIXED__( EnumDateFormats )
WINBASEAPI WINAPI  BOOL   EnumDateFormatsA (DATEFMT_ENUMPROCA, LCID, DWORD);
WINBASEAPI WINAPI  BOOL   EnumDateFormatsW (DATEFMT_ENUMPROCW, LCID, DWORD);

#define EnumSystemCodePages  __AW_SUFFIXED__( EnumSystemCodePages )
WINBASEAPI WINAPI  BOOL   EnumSystemCodePagesA (CODEPAGE_ENUMPROCA, DWORD);
WINBASEAPI WINAPI  BOOL   EnumSystemCodePagesW (CODEPAGE_ENUMPROCW, DWORD);

WINBASEAPI WINAPI  BOOL   EnumSystemGeoID (GEOCLASS, GEOID, GEO_ENUMPROC);

#define EnumSystemLocales  __AW_SUFFIXED__( EnumSystemLocales )
WINBASEAPI WINAPI  BOOL   EnumSystemLocalesA (LOCALE_ENUMPROCA, DWORD);
WINBASEAPI WINAPI  BOOL   EnumSystemLocalesW (LOCALE_ENUMPROCW, DWORD);

#define EnumTimeFormats  __AW_SUFFIXED__( EnumTimeFormats )
WINBASEAPI WINAPI  BOOL   EnumTimeFormatsA (TIMEFMT_ENUMPROCA, LCID, DWORD);
WINBASEAPI WINAPI  BOOL   EnumTimeFormatsW (TIMEFMT_ENUMPROCW, LCID, DWORD);

#define FoldString  __AW_SUFFIXED__( FoldString )
WINBASEAPI WINAPI  int    FoldStringA (DWORD, LPCSTR, int, LPSTR, int);
WINBASEAPI WINAPI  int    FoldStringW (DWORD, LPCWSTR, int, LPWSTR, int);

WINBASEAPI WINAPI  UINT   GetACP (void);

#define GetCalendarInfo  __AW_SUFFIXED__( GetCalendarInfo )
WINBASEAPI WINAPI  int    GetCalendarInfoA (LCID, CALID, CALTYPE, LPSTR, int, LPDWORD);
WINBASEAPI WINAPI  int    GetCalendarInfoW (LCID, CALID, CALTYPE, LPWSTR, int, LPDWORD);

WINBASEAPI WINAPI  BOOL   GetCPInfo (UINT, LPCPINFO);

#define GetCPInfoEx  __AW_SUFFIXED__( GetCPInfoEx )
WINBASEAPI WINAPI  BOOL   GetCPInfoExA (UINT, DWORD, LPCPINFOEXA);
WINBASEAPI WINAPI  BOOL   GetCPInfoExW (UINT, DWORD, LPCPINFOEXW);

#define GetCurrencyFormat  __AW_SUFFIXED__( GetCurrencyFormat )
WINBASEAPI WINAPI  int    GetCurrencyFormatA (LCID, DWORD, LPCSTR, const CURRENCYFMTA *, LPSTR, int);
WINBASEAPI WINAPI  int    GetCurrencyFormatW (LCID, DWORD, LPCWSTR, const CURRENCYFMTW *, LPWSTR, int);

#define GetDateFormat  __AW_SUFFIXED__( GetDateFormat )
WINBASEAPI WINAPI  int    GetDateFormatA (LCID, DWORD, const SYSTEMTIME *, LPCSTR, LPSTR, int);
WINBASEAPI WINAPI  int    GetDateFormatW (LCID, DWORD, const SYSTEMTIME *, LPCWSTR, LPWSTR, int);

#define GetGeoInfo  __AW_SUFFIXED__( GetGeoInfo )
WINBASEAPI WINAPI  int    GetGeoInfoA (GEOID, GEOTYPE, LPSTR, int, LANGID);
WINBASEAPI WINAPI  int    GetGeoInfoW (GEOID, GEOTYPE, LPWSTR, int, LANGID);

#define GetLocaleInfo  __AW_SUFFIXED__( GetLocaleInfo )
WINBASEAPI WINAPI  int    GetLocaleInfoA (LCID, LCTYPE, LPSTR, int);
WINBASEAPI WINAPI  int    GetLocaleInfoW (LCID, LCTYPE, LPWSTR, int);

WINBASEAPI WINAPI  BOOL   GetNLSVersion (NLS_FUNCTION, LCID, LPNLSVERSIONINFO);

#define GetNumberFormat  __AW_SUFFIXED__( GetNumberFormat )
WINBASEAPI WINAPI  int    GetNumberFormatA (LCID, DWORD, LPCSTR, const NUMBERFMTA *, LPSTR, int);
WINBASEAPI WINAPI  int    GetNumberFormatW (LCID, DWORD, LPCWSTR, const NUMBERFMTW *, LPWSTR, int);

WINBASEAPI WINAPI  UINT   GetOEMCP (void);

WINBASEAPI WINAPI  BOOL   GetStringTypeA (LCID, DWORD, LPCSTR, int, LPWORD);
WINBASEAPI WINAPI  BOOL   GetStringTypeW (DWORD, LPCWSTR, int, LPWORD);

#define GetStringTypeEx  __AW_SUFFIXED__( GetStringTypeEx )
WINBASEAPI WINAPI  BOOL   GetStringTypeExA (LCID, DWORD, LPCSTR, int, LPWORD);
WINBASEAPI WINAPI  BOOL   GetStringTypeExW (LCID, DWORD, LPCWSTR, int, LPWORD);

WINBASEAPI WINAPI  LANGID GetSystemDefaultLangID (void);
WINBASEAPI WINAPI  LCID   GetSystemDefaultLCID (void);
WINBASEAPI WINAPI  LCID   GetThreadLocale (void);

#define GetTimeFormat  __AW_SUFFIXED__( GetTimeFormat )
WINBASEAPI WINAPI  int    GetTimeFormatA (LCID, DWORD, const SYSTEMTIME *, LPCSTR, LPSTR, int);
WINBASEAPI WINAPI  int    GetTimeFormatW (LCID, DWORD, const SYSTEMTIME *, LPCWSTR, LPWSTR, int);

WINBASEAPI WINAPI  LANGID GetUserDefaultLangID (void);
WINBASEAPI WINAPI  LCID   GetUserDefaultLCID (void);
WINBASEAPI WINAPI  GEOID  GetUserGeoID (GEOCLASS);
WINBASEAPI WINAPI  BOOL   IsDBCSLeadByte (BYTE);
WINBASEAPI WINAPI  BOOL   IsDBCSLeadByteEx (UINT, BYTE);
WINBASEAPI WINAPI  BOOL   IsNLSDefinedString (NLS_FUNCTION, DWORD, LPNLSVERSIONINFO, LPCWSTR, int);
WINBASEAPI WINAPI  BOOL   IsValidCodePage (UINT);
WINBASEAPI WINAPI  BOOL   IsValidLocale (LCID, DWORD);

#define LCMapString  __AW_SUFFIXED__( LCMapString )
WINBASEAPI WINAPI  int    LCMapStringA (LCID, DWORD, LPCSTR, int, LPSTR, int);
WINBASEAPI WINAPI  int    LCMapStringW (LCID, DWORD, LPCWSTR, int, LPWSTR, int);

WINBASEAPI WINAPI  int    MultiByteToWideChar (UINT, DWORD, LPCSTR, int, LPWSTR, int);

#define SetCalendarInfo  __AW_SUFFIXED__( SetCalendarInfo )
WINBASEAPI WINAPI  int    SetCalendarInfoA (LCID, CALID, CALTYPE, LPCSTR);
WINBASEAPI WINAPI  int    SetCalendarInfoW (LCID, CALID, CALTYPE, LPCWSTR);

#define SetLocaleInfo  __AW_SUFFIXED__( SetLocaleInfo )
WINBASEAPI WINAPI  BOOL   SetLocaleInfoA (LCID, LCTYPE, LPCSTR);
WINBASEAPI WINAPI  BOOL   SetLocaleInfoW (LCID, LCTYPE, LPCWSTR);

WINBASEAPI WINAPI  BOOL   SetThreadLocale (LCID);
WINBASEAPI WINAPI  BOOL   SetUserGeoID (GEOID);
WINBASEAPI WINAPI  int    WideCharToMultiByte (UINT, DWORD, LPCWSTR, int, LPSTR, int, LPCSTR, LPBOOL);

#if _WIN32_WINNT >= _WIN32_WINNT_WIN2K

#define EnumCalendarInfoEx  __AW_SUFFIXED__( EnumCalendarInfoEx )
WINBASEAPI WINAPI  BOOL   EnumCalendarInfoExA (CALINFO_ENUMPROCEXA, LCID, CALID, CALTYPE);
WINBASEAPI WINAPI  BOOL   EnumCalendarInfoExW (CALINFO_ENUMPROCEXW, LCID, CALID, CALTYPE);

#define EnumDateFormatsEx  __AW_SUFFIXED__( EnumDateFormatsEx )
WINBASEAPI WINAPI  BOOL   EnumDateFormatsExA (DATEFMT_ENUMPROCEXA, LCID, DWORD);
WINBASEAPI WINAPI  BOOL   EnumDateFormatsExW (DATEFMT_ENUMPROCEXW, LCID, DWORD);

#define EnumSystemLanguageGroups  __AW_SUFFIXED__( EnumSystemLanguageGroups )
WINBASEAPI WINAPI  BOOL   EnumSystemLanguageGroupsA (LANGUAGEGROUP_ENUMPROCA, DWORD, LONG_PTR);
WINBASEAPI WINAPI  BOOL   EnumSystemLanguageGroupsW (LANGUAGEGROUP_ENUMPROCW, DWORD, LONG_PTR);

#define EnumLanguageGroupLocales  __AW_SUFFIXED__( EnumLanguageGroupLocales )
WINBASEAPI WINAPI  BOOL   EnumLanguageGroupLocalesA (LANGGROUPLOCALE_ENUMPROCA, LGRPID, DWORD, LONG_PTR);
WINBASEAPI WINAPI  BOOL   EnumLanguageGroupLocalesW (LANGGROUPLOCALE_ENUMPROCW, LGRPID, DWORD, LONG_PTR);

#define EnumUILanguages  __AW_SUFFIXED__( EnumUILanguages )
WINBASEAPI WINAPI  BOOL   EnumUILanguagesA (UILANGUAGE_ENUMPROCA, DWORD, LONG_PTR);
WINBASEAPI WINAPI  BOOL   EnumUILanguagesW (UILANGUAGE_ENUMPROCW, DWORD, LONG_PTR);

WINBASEAPI WINAPI  LANGID GetSystemDefaultUILanguage (void);
WINBASEAPI WINAPI  LANGID GetUserDefaultUILanguage (void);
WINBASEAPI WINAPI  BOOL   IsValidLanguageGroup (LGRPID, DWORD);

#endif /* >= Win2K */
#endif /* ! RC_INVOKED */

_END_C_DECLS

#endif	/* !_WINNLS_H: $RCSfile$: end of file */
