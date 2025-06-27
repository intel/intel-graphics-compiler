/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _COMMON_H_
#define _COMMON_H_
#include <cassert>
#include <iostream>
#include <sstream>

#include "visa_igc_common_header.h"

#ifndef GIT_COMMIT_HASH
#define GIT_COMMIT_HASH "?"
#endif

#ifndef _WIN32
#include "common/secure_string.h"
#include <errno.h>

#ifndef MEMCPY_S
#define MEMCPY_S
typedef int errno_t;
[[maybe_unused]]
static errno_t memcpy_s(void *dst, size_t numberOfElements, const void *src,
                        size_t count) {
  if ((dst == NULL) || (src == NULL)) {
    return EINVAL;
  }
  if (numberOfElements < count) {
    return ERANGE;
  }

  for (auto c = 0; c != count; c++) {
    *(((char *)dst) + c) = *(((char *)src) + c);
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
#if !defined(_WIN32)
typedef uint32_t DWORD;
typedef uint16_t WORD;
#endif /* Windows types for non-Windows end */

// Common code for exception handling, debug messages, platform checks, etc.

#if defined(DLL_MODE) && defined(_RELEASE)
#define IS_RELEASE_DLL
#endif

#define COUT_ERROR std::cout

#if defined(_DEBUG) && !defined(DLL_MODE)

// #define DEBUG_VERBOSE_ON

#ifdef DEBUG_VERBOSE_ON
#define DEBUG_VERBOSE(msg)                                                     \
  { COUT_ERROR << msg; }
#else
#define DEBUG_VERBOSE(msg)
#endif
#else
#define DEBUG_VERBOSE(msg)
#endif // #ifdef _DEBUG

namespace vISA {
#if !defined(NDEBUG) && !defined(DLL_MODE)
// Emitting debug information similar to the LLVM_DEBUG() macro.
// You can wrap your code around the VISA_DEBUG() macro, which is disabled by
// default. A "-debug-only foo" option is also added to enable debugging
// for a given pass with name foo. Unlike LLVM where DEBUG_TYPE is explicitly
// defined as a macro, the current pass name (defined in PassInfo) is compared
// against the pass(es) that are to be debugged. You can also explicitly set the
// current pass name via setCurrentPass(). "-debug-only all" will enable
// VISA_DEBUG() globally.

// This boolean is set to true if the '-debug-only' command line option
// is specified.
extern bool DebugFlag;

// This boolean is set to true if the '-debug-only=all' command line option
// is specified.
extern bool DebugAllFlag;

// Add a pass that should emit debug information
// This should only be called once when processing "-debug-only" option.
void addPassToDebug(std::string name);

// Set the current pass's name. To avoid unnecessary memory allocation, this
// function does not perform a copy. This means that name should either be a
// string literal, or the caller must ensure that its lifetime is the same as
// the pass's lifetime.
void setCurrentDebugPass(const char *name);

// Don't call this directly, use the VISA_DEBUG macro instead.
bool isCurrentDebugPass();

// VISA_DEBUG macro - You can include any code that should only be executed
// when debug information is enabled. Example:
// VISA_DEBUG(std::cerr << "foo\n");
// VISA_DEBUG(printf("id = %d\n", bb->getID()));
// VISA_DEBUG({
//   for (auto dcl : Declares)
//      dcl->emit(std::cout);
// })
//
#define VISA_DEBUG(X)                                                          \
  do {                                                                         \
    if (::DebugFlag && ::isCurrentDebugPass()) {                               \
      X;                                                                       \
    }                                                                          \
  } while (false)
// VERBOSE version
// TODO: We should consider getting rid of this; it's not clear if the legacy
// DEBUG_VERBOSE messages are still useful, and quite a few don't even compile.
#ifdef DEBUG_VERBOSE_ON
#define VISA_DEBUG_VERBOSE(X) VISA_DEBUG(X)
#else
#define VISA_DEBUG_VERBOSE(X)
#endif
#else
#define addPassToDebug(X)
#define isCurrentDebugPass() (false)
#define setCurrentDebugPass(X)
#define VISA_DEBUG(X)                                                          \
  do {                                                                         \
  } while (false)
#define VISA_DEBUG_VERBOSE(X) VISA_DEBUG(X)
#endif // NDEBUG
} // namespace vISA

enum class PlatformGen {
  GEN_UNKNOWN = 0,
  GEN8 = 8,
  GEN9 = 9,
  GEN10 = 10,
  GEN11 = 11,
  XE = 12,
  XE2 = 13,
  XE3 = 14,
};

// Error types
#define ERROR_UNKNOWN "ERROR: Unknown fatal internal error"
#define ERROR_INTERNAL_ARGUMENT                                                \
  "ERROR: Invalid argument in an internal function"

#define ERROR_MEM_ALLOC "ERROR: Fail to allocate memory or create object"
#define ERROR_FLOWGRAPH                                                        \
  "ERROR: Unknown error in Flow Graph" // for all unknown errors related to flow
                                       // graph
#define ERROR_SPILLCODE "ERROR: Unknown error related to spill code"
#define ERROR_SCHEDULER "ERROR: Unknown error in local scheduler"
#define ERROR_GRAPHCOLOR "ERROR: Unknown error in Graph Coloring"
#define ERROR_REGALLOC "ERROR: Unknown error in Register Allocation"

#define ERROR_FILE_READ(x) "ERROR: Invalid or non-existent file #x"
#define ERROR_OPTION "ERROR: Invalid input option or option combination"
#define ERROR_INVALID_VISA_NAME(x) "ERROR: Invalid name #x"
#define ERROR_SYNTAX(x) "ERROR: Syntax error -- #x"
#define ERROR_DATA_RANGE(x) "ERROR: Out of boundary or invalid data value in #x"
// end of Error Message

template <typename Type>
constexpr Type AlignUp(const Type value, const size_t alignment) {
  Type common = value + alignment - 1;
  return common - (common % alignment);
}

//
// The function returns:
//   - 0 if 'value' = 0,
//   - the lowest power of two greater of equal to 'value'.
//
// The main idea of the algorithm is to set all lower bits for each set bit of 'value'.
//
constexpr static uint32_t RoundUpToPowerOf2(uint32_t value) {
  // Without decreasing, the result would be:
  // - 1 for 'value' = 0
  // - the next power of two if 'value' is a power of two.
  value--;

  value |= value >> 1;  // Duplicate each set bit to the 1st bit on the right, so each group of 1s has length >= 2 or ends on the first bit.
  value |= value >> 2;  // Duplicate each set bit to the 2nd bit on the right, so each group of 1s has length >= 4 or ends on the first bit.
  value |= value >> 4;  // ..
  value |= value >> 8;  // ..
  value |= value >> 16; // Duplicate each set bit to the 16th bit on the right, so each group of 1s has length >= 32 or ends on the first bit.

  // Increase by one to get a power of two.
  value++;

  return value;
}

#endif //_COMMON_H_
