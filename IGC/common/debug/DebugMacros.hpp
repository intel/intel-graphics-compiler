/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

/*****************************************************************************\
MACRO: TODO
Use for code changes that need to happen, but are not in place now.
\*****************************************************************************/
#define TODO(x)


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

