/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

Copyright (C) 2014 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal with the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimers.
Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimers in the documentation
and/or other materials provided with the distribution.
Neither the names of Advanced Micro Devices, Inc., nor the names of its
contributors may be used to endorse or promote products derived from this
Software without specific prior written permission.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
THE SOFTWARE.

============================= end_copyright_notice ===========================*/

// This file defines SPIR-V extended instructions.

#ifndef SPIRVBUILTIN_HPP_
#define SPIRVBUILTIN_HPP_

#include "SPIRVUtil.h"
#include "OpenCL.std.h"
#include "SPIRV.DebugInfo.h"

#include <string>
#include <vector>

namespace igc_spv{


inline bool
isOpenCLBuiltinSet (SPIRVExtInstSetKind Set) {
  return Set == SPIRVEIS_OpenCL;
}

inline bool
isSPIRVDebugInfoSet(SPIRVExtInstSetKind Set) {
    return Set == SPIRVEIS_DebugInfo ||
        Set == SPIRVEIS_OpenCL_DebugInfo_100;
}

typedef igc_OpenCLLIB::Entrypoints OCLExtOpKind;
typedef SPIRVDebugInfo::Entrypoints OCLExtOpDbgKind;

template<> inline void
SPIRVMap<OCLExtOpKind, std::string>::init() {
#define _OCL_EXT_OP(name, num) add(igc_OpenCLLIB::name, #name);
#include "OpenCL.stdfuncs.h"
#undef _OCL_EXT_OP
}
SPIRV_DEF_NAMEMAP(OCLExtOpKind, OCLExtOpMap)

template<> inline void
SPIRVMap<OCLExtOpDbgKind, std::string>::init() {
#define _OCL_EXT_OP(name, num) add(SPIRVDebugInfo::name, #name);
#include "SPIRV.DebugInfofuncs.h"
#undef _OCL_EXT_OP
}
SPIRV_DEF_NAMEMAP(OCLExtOpDbgKind, OCLExtOpDbgMap)

}

#endif
