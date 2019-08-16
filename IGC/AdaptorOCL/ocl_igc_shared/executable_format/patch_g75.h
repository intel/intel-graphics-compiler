/*****************************************************************************\

Copyright (c) Intel Corporation (2009 - 2013).

    INTEL MAKES NO WARRANTY OF ANY KIND REGARDING THE CODE.  THIS CODE IS
    LICENSED ON AN "AS IS" BASIS AND INTEL WILL NOT PROVIDE ANY SUPPORT,
    ASSISTANCE, INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL DOES NOT
    PROVIDE ANY UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY
    DISCLAIMS ANY WARRANTY OF MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR ANY
    PARTICULAR PURPOSE, OR ANY OTHER WARRANTY.  Intel disclaims all liability,
    including liability for infringement of any proprietary rights, relating to
    use of the code. No license, express or implied, by estoppel or otherwise,
    to any intellectual property rights is granted herein.

File Name:  patch_g75.h

Abstract:   Contains Gen7.5 patch structure definitions

Notes:

\*****************************************************************************/
#pragma once
#include "patch_shared.h"

namespace iOpenCL
{

/*****************************************************************************\
STRUCT: SKernelBinaryHeaderGen75
\*****************************************************************************/
struct SKernelBinaryHeaderGen75 :
       SKernelBinaryHeaderCommon
{
};

} // namespace