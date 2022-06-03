/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _COMMON_H_
#define _COMMON_H_
#include <sstream>
#include <cassert>
#include <iostream>

#include "visa_igc_common_header.h"
#include "VISADefines.h"

#ifndef _WIN32
#include <errno.h>
#include "common/secure_string.h"

#ifndef MEMCPY_S
#define MEMCPY_S
typedef int errno_t;
static errno_t memcpy_s(void* dst, size_t numberOfElements, const void* src, size_t count)
{
    if ((dst == NULL) || (src == NULL)) {
        return EINVAL;
    }
    if (numberOfElements < count) {
        return ERANGE;
    }

    for (auto c = 0; c != count; c++)
    {
        *(((char*)dst) + c) = *(((char*)src) + c);
    }

    return 0;
}
#endif
#endif

/* stdio.h portability code end */

/* stdint.h portability code start */
#ifndef _WIN32
// FIXME: do all of our configs support inttypes.h?
#include <stdint.h>
#else
#include <inttypes.h>
#endif /* _WIN32 */
/* stdint.h portability code end */

/* Windows types for non-Windows start */
#if !defined(_WIN32) && !defined(_WIN64)
#define SUCCEED 1
#define ERROR 0

typedef char CHAR;
typedef int INT;
typedef short SHORT;
typedef int32_t LONG;
typedef int64_t LONGLONG;

typedef uint32_t UINT32;
typedef uint32_t DWORD;
typedef uint16_t WORD;

typedef double DOUBLE;
typedef float FLOAT;

#ifndef __LARGE_INTEGER_STRUCT_DEFINED__
union LARGE_INTEGER {
    struct dummy {
        DWORD LowPart;
        LONG HighPart;
    };

    struct u {
        DWORD LowPart;
        LONG HighPart;
    };

    LONGLONG QuadPart;
};
#define __LARGE_INTEGER_STRUCT_DEFINED__
#endif // __LARGE_INTEGER_STRUCT_DEFINED__

#endif /* Windows types for non-Windows end */

//Common code for exception handling, debug messages, platform checks, etc.

#if defined(DLL_MODE) && defined(_RELEASE)
#define IS_RELEASE_DLL
#endif

#define VISA_SUCCESS                0
#define VISA_FAILURE               (-1)
#define VISA_SPILL                 (-3)

// stream for error messages
extern std::stringstream errorMsgs;

#define COUT_ERROR      std::cout

#if defined(_DEBUG) && !defined(DLL_MODE)

//#define DEBUG_VERBOSE_ON

#ifdef DEBUG_VERBOSE_ON
#define DEBUG_VERBOSE(msg) { \
    COUT_ERROR << msg; \
}
#else
#define DEBUG_VERBOSE(msg)
#endif

#define DEBUG_MSG(msg) { \
    COUT_ERROR << msg; \
}

// call the obj's emit function
#define DEBUG_EMIT(obj) { \
    (obj)->emit(COUT_ERROR); \
}

#else

#define DEBUG_VERBOSE(msg)
#define DEBUG_MSG(msg)
#define DEBUG_EMIT(obj)

#endif  // #ifdef _DEBUG

// disable asserts only for release DLL
#if defined(_DEBUG) || !defined(DLL_MODE) || !defined(NDEBUG)
#define ASSERT_USER(x, errormsg)\
    do {\
        if (!(x))   \
        {           \
            errorMsgs << "Error in Common ISA file:" << errormsg << std::endl; \
            assert(false); \
        } \
    } while (0)

#define ASSERT_USER_LOC(x, errormsg, line, file ) \
    do { \
        if (!(x))   \
        { \
            errorMsgs << "Error in Common ISA file(" << file << ":" << line << "): " << errormsg << std::endl; \
            assert(false); \
        }    \
    } while(0)


#define MUST_BE_TRUE2(x, errormsg, inst) \
    do {\
        if (!(x))   \
        { \
            std::cerr <<errormsg << std::endl;  \
            inst->emit(errorMsgs, true); \
            std::cerr << std::endl; \
            assert(false); \
        } \
    } while (0)

#define MUST_BE_TRUE(x,errormsg) \
    do { \
        if (!(x)) \
        { \
            std::cerr << __FILE__ << ":" << __LINE__ << " " << errormsg << std::endl; \
            assert(false); \
        } \
    } while (0)

#define MUST_BE_TRUE1(x, lineno, errormsg) \
    do { \
        if (!(x)) \
        { \
            std::cerr << "(Source Line " << lineno << ") " << errormsg << std::endl;  \
            assert(false); \
        } \
    } while (0)

#else
#define ASSERT_USER(x, errormsg)
#define ASSERT_USER_LOC(x, errormsg, line, file )
#define MUST_BE_TRUE(x, errormsg)
#define MUST_BE_TRUE1(x, lineno, errormsg)
#define MUST_BE_TRUE2(x, errormsg, inst)
#endif

enum class PlatformGen
{
    GEN_UNKNOWN = 0,
    GEN8   = 8,
    GEN9   = 9,
    GEN10  = 10,
    GEN11  = 11,
    XE     = 12,
};

// Error types
#define ERROR_UNKNOWN               "ERROR: Unknown fatal internal error"
#define ERROR_INTERNAL_ARGUMENT     "ERROR: Invalid argument in an internal function"

#define ERROR_MEM_ALLOC             "ERROR: Fail to allocate memory or create object"
#define ERROR_FLOWGRAPH             "ERROR: Unknown error in Flow Graph"  // for all unknown errors related to flow graph
#define ERROR_SPILLCODE             "ERROR: Unknown error related to spill code"
#define ERROR_SCHEDULER             "ERROR: Unknown error in local scheduler"
#define ERROR_GRAPHCOLOR            "ERROR: Unknown error in Graph Coloring"
#define ERROR_REGALLOC              "ERROR: Unknown error in Register Allocation"

#define ERROR_FILE_READ(x)          "ERROR: Invalid or non-existent file " << (x)
#define ERROR_OPTION                "ERROR: Invalid input option or option combination"
#define ERROR_INVALID_VISA_NAME(x)  "ERROR: Invalid name " << (x)
#define ERROR_SYNTAX(x)             "ERROR: Syntax error -- " << (x)
#define ERROR_DATA_RANGE(x)         "ERROR: Out of boundary or invalid data value in " << (x)
// end of Error Message


template<typename Type>
constexpr Type AlignUp(const Type value, const size_t alignment)
{
    Type common = value + alignment - 1;
    return common - (common % alignment);
}

#endif //_COMMON_H_
