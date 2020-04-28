/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

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
typedef long LONG;
typedef long long LONGLONG;

typedef uint32_t UINT32;
typedef uint32_t DWORD;
typedef uint16_t WORD;

typedef double DOUBLE;
typedef float FLOAT;

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

#endif /* Windows types for non-Windows end */

//Common code for exception handling, debug messages, platform checks, etc.

#if defined(DLL_MODE) && defined(_RELEASE)
#define IS_RELEASE_DLL
#endif

#define VISA_SUCCESS                0
#define VISA_FAILURE               -1
#define VISA_SPILL                 -3

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
#define ASSERT_USER(x, errormsg) {  if (!(x))   \
{           \
    errorMsgs << "Error in Common ISA file:" << errormsg << std::endl; \
    assert(false); \
}    \
}

#define ASSERT_USER_LOC(x, errormsg, line, file ) { if (!(x))   \
{           \
    errorMsgs << "Error in Common ISA file(" << file << ":" << line << "): " << errormsg << std::endl; \
    assert(false); \
}    \
}


#define MUST_BE_TRUE2(x, errormsg, inst)  { if (!(x))   \
                                    {           \
                                        std::cerr <<errormsg << std::endl;  \
                                        inst->emit(errorMsgs, true); \
                                        std::cerr << std::endl; \
                                        assert(false); \
                                    }    \
                                  }

#define MUST_BE_TRUE(x,errormsg)  { if (!(x))   \
                                    {           \
                                    std::cerr << __FILE__ << ":" << __LINE__ << " " << errormsg << std::endl; \
                                    assert(false); \
                                    }    \
                                  }

#define MUST_BE_TRUE1(x, lineno, errormsg)  {   if (!(x))   \
                                    {           \
                                        std::cerr << "(Source Line "<<lineno<<") " << errormsg << std::endl;  \
                                        assert(false);  \
                                    }    \
                                  }
#else
#define ASSERT_USER(x, errormsg)
#define ASSERT_USER_LOC(x, errormsg, line, file )
#define MUST_BE_TRUE(x, errormsg)
#define MUST_BE_TRUE1(x, lineno, errormsg)
#define MUST_BE_TRUE2(x, errormsg, inst)
#endif

extern const char* platformString[];

#define MAX_OPTION_STR_LENGTH 256

extern const char* steppingNames[];

// currently the supported arguments are
// -- "BDW" --> GENX_BDW
// -- "CHV" --> GENX_CHV
// -- "SKL" --> GENX_SKL
// -- "BXT" --> GENX_BXT
// -- "ICLLP" --> GENX_ICLLP
// -- "TGLLP" --> GENX_TGLLP
extern "C" int SetPlatform(const char* s);
extern "C" int SetVisaPlatform(TARGET_PLATFORM vPlatform);

/*************** internal jitter functions ********************/
// returns the HW platform for this jitter invocation
extern "C" TARGET_PLATFORM getGenxPlatform( void );

enum class PlatformGen
{
    GEN_UNKNOWN = 0,
    GEN8 = 8,
    GEN9 = 9,
    GEN10 = 10,
    GEN11 = 11,
    GEN12 = 12,
};

unsigned char getGRFSize();

// return the platform generation that can be used for comparison
PlatformGen getPlatformGeneration(TARGET_PLATFORM platform);

// Not linearized for backward compatibility.
extern "C" int getGenxPlatformEncoding();

extern "C" void InitStepping();
extern "C" int SetStepping( const char* s);
extern "C" Stepping GetStepping( void );
extern "C" const char * GetSteppingString( void );

// Error types
#define ERROR_UNKNOWN                       "ERROR: Unkown fatal internal error!"
#define ERROR_INTERNAL_ARGUMENT "ERROR: Invalid argument in an internal function!"

#define ERROR_MEM_ALLOC                 "ERROR: Fail to allocate memory or create object!"
#define ERROR_FLOWGRAPH                 "ERROR: Unknown error in Flow Graph!"       // for all unknown errors related to flow graph
#define ERROR_SPILLCODE                     "ERROR: Unknown error related to spill code!"
#define ERROR_SCHEDULER                 "ERROR: Unknown error in local scheduler!"
#define ERROR_GRAPHCOLOR                "ERROR: Unknown error in Graph Coloring!"
#define ERROR_REGALLOC                      "ERROR: Unknown error in Register Allocation!"

#define ERROR_FILE_READ( x )                "ERROR: Invalid or non-existent file " << x << "!"
#define ERROR_OPTION                            "ERROR: Invalid input option or option combination!"
#define ERROR_INVALID_VISA_NAME( x )     "ERROR: Invalid name " << x << "!"
#define ERROR_SYNTAX( x )                       "ERROR: Syntax error -- " << x << "!"
#define ERROR_DATA_RANGE( x )           "ERROR: Out of boundary or invalid data value in " << x << "!"
// end of Error Message

#define G4_GRF_REG_SIZE    (getGRFSize() / 2u)
#define G4_GRF_REG_NBYTES  getGRFSize()
#define GENX_GRF_REG_SIZ   getGRFSize()
#define NUM_WORDS_PER_GRF  (getGRFSize() / 2)
#define NUM_DWORDS_PER_GRF (getGRFSize() / 4u)


// Target should be specified as follows
// - VISA builder mode in CreateBuilder API through fast-path
// - Kernel target attribute in VISA binarary or ISAASM
typedef enum {
    VISA_CM = 0,
    VISA_3D = 1,
    VISA_CS = 2,
} VISATarget;

#endif //_COMMON_H_
