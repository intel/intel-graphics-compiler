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
#pragma once

/*****************************************************************************\
MACRO: TODO
Use for code changes that need to happen, but are not in place now.
\*****************************************************************************/
#define TODO(x)


/*****************************************************************************\
MACRO: USC_NOT_IMPLEMENTED
Use for function stubs which are yet to be implemented.
\*****************************************************************************/
#ifdef _WIN32
#   if defined( _DEBUG ) || defined( _INTERNAL )
#       define IGC_NOT_IMPLEMENTED(DESCRIPTION)                               \
    do {                                                                      \
         assert(0 && DESCRIPTION );                                           \
            } while (0)
#   else
#       define IGC_NOT_IMPLEMENTED(DESCRIPTION)
#   endif
#else
#   define IGC_NOT_IMPLEMENTED(DESCRIPTION)                                   \
    do {                                                                      \
    assert(0 && DESCRIPTION);                                                                      \
            } while (0)
#endif


/*****************************************************************************\
MACRO: IGC_WARN_ON_CONDITION
Use for warnings about the input to different phases of the compiler.

The idea here is to use this to provide useful messages where an assertion
would be too strict.
\*****************************************************************************/
#ifdef _DEBUG
#   define IGC_WARN_ON_CONDITION( expr, message )                             \
    do {                                                                      \
        if ( (expr) )                                                         \
                        {                                                     \
            IGC::Debug::Warning( (#expr), (__LINE__), (__FILE__), (message) );\
                        }                                                     \
            } while (0)
#else
#   define IGC_WARN_ON_CONDITION( expr, message )                             \
    do {                                                                      \
        (void) sizeof(expr);                                                  \
            } while (0)
#endif


/*****************************************************************************\
MACRO: VALUE_NAME
Wrapper for LLVM twine names.

Use this to wrap twine name parameters in IRBuilder calls so that we don't
have to pay the cost of it in the release build.
\*****************************************************************************/
#if defined( _DEBUG )
#   define VALUE_NAME_ENABLE 1
#   define VALUE_NAME(STR) (STR)
#else
#   define VALUE_NAME(STR) ("")
#endif


#if defined _MSC_VER
#   if _MSC_VER>=1300
#       if !defined BUILD_MESSAGE
#           define BUILD_MESSAGE_stringlize(x) #x
#           define BUILD_MESSAGE_quote(x) BUILD_MESSAGE_stringlize(x)
#           define BUILD_MESSAGE(x) __pragma(message(__FILE__ "(" BUILD_MESSAGE_quote(__LINE__) ") : " x))
#       endif
#   else
#       define BUILD_MESSAGE(x)
#   endif
#else
#   define BUILD_MESSAGE(x)
#endif

#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR == 5
#define IF_LLVM_3_5(X) X
#else
#define IF_LLVM_3_5(X)
#endif
