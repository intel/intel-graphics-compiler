/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace USC
{

/*****************************************************************************\
STRUCT: ErrorCode
\*****************************************************************************/
struct ErrorCode
{
    // External values
    unsigned int       Success             : 1;    // Call was successful
    unsigned int       Error               : 1;    // Invalid call
    unsigned int       OutOfSystemMemory   : 1;    // System memory allocation failed
    unsigned int       Busy                : 1;    // Compilation not done yet
    unsigned int       Reserved            : 28;   // Reserved

};

/*****************************************************************************\
CONST: g_cInitRetVal
\*****************************************************************************/
const ErrorCode g_cInitErrorCode =
{
    true,   // Success
    false,  // Error
    false,  // OutOfSystemMemory
    false,  // Busy
    0       // Reserved
};

}
