/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef GED_BASIC_TYPES_H
#define GED_BASIC_TYPES_H

# if !defined(_MSC_VER) || (defined (_MSC_VER) && _MSC_VER >= 1600)
#  include <stdint.h>
# else // must be in some older version of Visual C++
// These "__" types are Microsoft specific (nonstandard).
typedef unsigned __int64 uint64_t;
typedef signed   __int64  int64_t;
typedef unsigned __int32 uint32_t;
typedef signed   __int32  int32_t;
typedef unsigned __int16 uint16_t;
typedef signed   __int16  int16_t;
typedef unsigned __int8   uint8_t;
typedef signed   __int8    int8_t;
# endif

#if !defined(__cplusplus) && !defined(__bool_true_false_are_defined)
typedef enum { false, true } bool;
#endif

extern const char* BoolTypeStr;

// Support C89 in GED's API using the __inline keyword.
#if defined(__GNUC__) || defined(_MSC_VER) || defined(__INTEL_COMPILER)
# define GED_INLINE __inline
#else
# define GED_INLINE inline
#endif

// Support switch-case fall-through attribute
#if !defined(__has_cpp_attribute)
# define GED_FALLTHROUGH
#elif defined(__clang__)
# if __has_cpp_attribute(fallthrough)
#  define GED_FALLTHROUGH [[fallthrough]]
# elif __has_cpp_attribute(clang::fallthrough)
#  define GED_FALLTHROUGH [[clang::fallthrough]]
# else
#  define GED_FALLTHROUGH
# endif
#elif defined(__GNUG__)
# if __has_cpp_attribute(fallthrough)
#  define GED_FALLTHROUGH [[fallthrough]]
# elif __has_cpp_attribute(gnu::fallthrough)
#  define GED_FALLTHROUGH [[gnu::fallthrough]]
# else
#  define GED_FALLTHROUGH
# endif
#else
# define GED_FALLTHROUGH
#endif

extern const char* InlineStr;

#endif // GED_BASIC_TYPES_H
