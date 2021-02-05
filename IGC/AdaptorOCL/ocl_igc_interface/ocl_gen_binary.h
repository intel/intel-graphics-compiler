/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/common/id.h"
#include "cif/common/cif.h"

#include "patch_list.h"

#include "cif/macros/enable.h"

// Interface : OCL_GEN_BIN
//             OCL GEN binary format
// Interface for defining supported OCL GEN binary

namespace IGC {

using TypeErasedEnum = uint64_t;

CIF_DECLARE_INTERFACE(OclGenBinary, "OCL_GEN_BIN")

// Default OCL GEN binary interface works only as 1:1 version checker,
// but can be easily used for declaring backwards compatibility as well
CIF_DEFINE_INTERFACE_VER(OclGenBinary, iOpenCL::CURRENT_ICBE_VERSION){
  CIF_INHERIT_CONSTRUCTOR();
};

CIF_GENERATE_VERSIONS_LIST_WITH_BASE(OclGenBinary, 1000);

CIF_MARK_LATEST_VERSION(OclGenBinaryLatest, OclGenBinary);
using OclGenBinaryTagOCL = OclGenBinaryLatest; // Note : can tag with different version for
                                               //        transition periods

}

#include "cif/macros/disable.h"
