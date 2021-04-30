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
#if !defined( LLVM_WARNINGS_PUSH )
#   error "Improperly structured LLVMWarnings{Push,Pop}.hpp includes ( pop included, with no prior push included )"
#endif
#if defined( LLVM_WARNINGS_POP )
#   error "Improperly structured LLVMWarnings{Push,Pop}.hpp includes ( pop included twice with no push inbetween )"
#endif
#define LLVM_WARNINGS_POP
#undef LLVM_WARNINGS_PUSH

 // Do the actual pop
#ifdef _MSC_VER
#   pragma  warning( pop )
#endif

#if defined(__linux__)
#   pragma GCC diagnostic pop
#endif
