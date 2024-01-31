/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

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
