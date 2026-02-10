/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Disable warnings specific to llvm headers
//
// Use it like this:
//     |  #include "Common/Debug/Debug.hpp"
//     |
//     |  #include "Common/LLVMWarningsPush.hpp"
//     |  #include <llvm/IR/Function.h>
//     |  #include <llvm/IR/Instruction.h>
//     |  #include <llvm/DerivedTypes.h>
//     |  #include <llvm/IR/Intrinsics.h>
//     |  #include "Common/LLVMWarningsPop.hpp"
//     |
//     |  #include <vector>
//     |
//     | // Rest of implementation
//
// This enables us to turn on -Werror without fixing all of the places inside
// llvm itself where warning level 3 would normally complain.

// Do some checking to make sure the includes are in the right order
// It is super easy to get this wrong...
#if defined(LLVM_WARNINGS_PUSH)
#error "Improperly structured LLVMWarnings{Push,Pop}.hpp includes (push included twice, with no pop inbetween)"
#endif
#undef LLVM_WARNINGS_POP
#define LLVM_WARNINGS_PUSH

// Do the actual push
#ifdef _MSC_VER
#pragma warning(push)
#endif

#if defined(__linux__)
#pragma GCC diagnostic push
#if __GNUC__ > 8
#pragma GCC diagnostic ignored "-Winit-list-lifetime"
#endif
#if __GNUC__ >= 15
#pragma GCC diagnostic ignored "-Wcpp"
#endif
#ifdef __clang__
#pragma GCC diagnostic ignored "-W#warnings"
#endif
#endif

#if defined(_WIN32) || defined(_WIN64)

// 'inline' : used more than once
#pragma warning(disable : 4141)

#pragma warning(disable : 4099)

// unary minus operator applied to unsigned type, result still unsigned
#pragma warning(disable : 4146)

// macro redefinition
#pragma warning(disable : 4005)

// 'this' : used in base member initializer list
#pragma warning(disable : 4355)

// 'type' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable : 4800)

// nonstandard extension used: enum 'enum' used in qualified name
#pragma warning(disable : 4482)

// 'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable : 4244)

// 'initializing' : conversion from 'unsigned int' to 'unsigned short', possible loss of data
#pragma warning(disable : 4242)

// 'declaration' : no matching operator delete found; memory will not be freed if initialization throws an exception
#pragma warning(disable : 4291)

// class has virtual functions, but destructor is not virtual
#pragma warning(disable : 4265)

// warning C4319: '~': zero extending 'uint32_t' to 'uintptr_t' of greater size
#pragma warning(disable : 4319)

// destructor could not be generated because a base class destructor is inaccessible or deleted
#pragma warning(disable : 4624)

//'unsigned int' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable : 4800)

// error C4296: '>=': expression is always true
#pragma warning(disable : 4296)

//'argument': conversion from 'int' to 'size_t', signed / unsigned mismatch
#pragma warning(disable : 4245)

// declaration of 'X' hides previous local declaration
#pragma warning(disable : 4456)

// structure was padded due to alignment specifier
#pragma warning(disable : 4324)

// unreferenced formal parameter
#pragma warning(disable : 4100)

// declaration of 'X' hides function parameter
#pragma warning(disable : 4457)

// declaration of 'X' hides class member
#pragma warning(disable : 4458)

// declaration of 'X' hides global declaration
#pragma warning(disable : 4459)

// cast truncates constatnt value
#pragma warning(disable : 4310)

// unreachable code
#pragma warning(disable : 4702)

#endif

#ifdef _WIN64

// 'argument' : conversion from 'size_t' to 'unsigned int', possible loss of data
#pragma warning(disable : 4267)

#endif
