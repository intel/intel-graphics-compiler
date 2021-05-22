/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once


/*****************************************************************************\

MACRO: 
    ISTD_DISALLOW_COPY_AND_ASSIGN

Description: 
    A macro to disallow the copy constructor and operator= functions
    This should be used in the private: declarations for a class

\*****************************************************************************/
#define ISTD_DISALLOW_COPY_AND_ASSIGN( TypeName ) \
    TypeName(const TypeName&); \
    TypeName& operator=(const TypeName&)
