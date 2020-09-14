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
#if defined( LLVM_WARNINGS_PUSH )
#   error "Improperly structured LLVMWarnings{Push,Pop}.hpp includes (push included twice, with no pop inbetween)"
#endif
#undef LLVM_WARNINGS_POP
#define LLVM_WARNINGS_PUSH

// Do the actual push
#ifdef _MSC_VER
#   pragma warning( push )
#endif

#if defined(__linux__)
#   pragma GCC diagnostic push
#   if __GNUC__ > 8
#       pragma GCC diagnostic ignored "-Winit-list-lifetime"
#   endif
#endif

#if defined( _WIN32 ) || defined( _WIN64 )

// 'inline' : used more than once
#   pragma warning( disable : 4141 )

#   pragma warning( disable : 4099 )

// unary minus operator applied to unsigned type, result still unsigned
#   pragma warning( disable : 4146 )

// macro redefinition
#   pragma warning( disable : 4005 )

// 'this' : used in base member initializer list
#   pragma warning( disable : 4355 )

// 'type' : forcing value to bool 'true' or 'false' (performance warning)
#   pragma warning( disable : 4800 )

// nonstandard extension used: enum 'enum' used in qualified name
#   pragma warning( disable : 4482 )

// 'conversion' conversion from 'type1' to 'type2', possible loss of data
#   pragma warning( disable : 4244 )

// 'initializing' : conversion from 'unsigned int' to 'unsigned short', possible loss of data
#   pragma warning( disable : 4242 )

// 'declaration' : no matching operator delete found; memory will not be freed if initialization throws an exception
#   pragma warning( disable : 4291 )

// class has virtual functions, but destructor is not virtual
#   pragma warning( disable : 4265 )

// warning C4319: '~': zero extending 'uint32_t' to 'uintptr_t' of greater size
#   pragma warning( disable : 4319 )

// destructor could not be generated because a base class destructor is inaccessible or deleted
#   pragma warning( disable : 4624 )

//'unsigned int' : forcing value to bool 'true' or 'false' (performance warning)
#   pragma warning( disable : 4800 )

//error C4296: '>=': expression is always true
#   pragma warning( disable : 4296 )

//'argument': conversion from 'int' to 'size_t', signed / unsigned mismatch
#   pragma warning( disable : 4245 )

//declaration of 'X' hides previous local declaration
#   pragma warning( disable : 4456 )

// structure was padded due to alignment specifier
#   pragma warning( disable : 4324 )

// unreferenced formal parameter
#   pragma warning( disable : 4100 )

// declaration of 'X' hides class member
#   pragma warning( disable : 4458 )

// cast truncates constatnt value
#   pragma warning( disable : 4310 )

#endif

#ifdef _WIN64

// 'argument' : conversion from 'size_t' to 'unsigned int', possible loss of data
#   pragma warning( disable : 4267 )

#endif

