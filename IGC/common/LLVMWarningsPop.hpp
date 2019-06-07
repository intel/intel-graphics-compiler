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
