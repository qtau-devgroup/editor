#define VERSION "1.3.0"
#define PACKAGE "flac"
#define PACKAGE_BUGREPORT "flac-dev@xiph.org"
#define PACKAGE_NAME "flac"
#define PACKAGE_STRING "flac 1.3.0"
#define PACKAGE_TARNAME "flac"
#define PACKAGE_URL "https://www.xiph.org/flac/"
#define PACKAGE_VERSION "1.3.0"

// OS defines --------------------------
// 32bit/64bit dependent defines - should be 8 for 64bit and 4 for 32bit systems
#define SIZEOF_OFF_T 8
#define SIZEOF_VOIDP 8

#define WORDS_BIGENDIAN 0
#define CPU_IS_BIG_ENDIAN 0
#define CPU_IS_LITTLE_ENDIAN 1
//--------------------------------------

#define STDC_HEADERS 1

#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif

#define FLAC__HAS_OGG 1
#define FLAC__SSE_OS 1
#define FLAC__USE_3DNOW 1
#define FLAC__USE_ALTIVEC 1
#define GWINSZ_IN_SYS_IOCTL 1
#define HAVE_BSWAP32 1
#define HAVE_BYTESWAP_H 1
#define HAVE_CXX_VARARRAYS 1
#define HAVE_C_VARARRAYS 1
#define HAVE_DLFCN_H 1
#define HAVE_FSEEKO 1
#define HAVE_GETOPT_LONG 1
#define HAVE_ICONV 1
#define HAVE_INTTYPES_H 1
#define HAVE_LANGINFO_CODESET 1
#define HAVE_LROUND 1
#define HAVE_MEMORY_H 1
#define HAVE_SOCKLEN_T 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_TERMIOS_H 1
#define HAVE_TYPEOF 1
#define HAVE_UNISTD_H 1
#define ICONV_CONST
