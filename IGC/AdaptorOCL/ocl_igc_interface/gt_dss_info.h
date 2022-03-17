/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/common/id.h"
#include "cif/common/cif.h"

#include "cif/macros/enable.h"

// Interface : GT_DUALSUBSLICE_INFO
//             GT DSS Info
// Interface for defining target device

namespace IGC {

CIF_DECLARE_INTERFACE(GTDualSubSliceInfo, "GT_DSS_INFO")

CIF_DEFINE_INTERFACE_VER(GTDualSubSliceInfo, 1){
    CIF_INHERIT_CONSTRUCTOR();

    virtual bool GetEnabled() const;
    virtual void SetEnabled(bool v);
    virtual uint32_t GetEuEnabledCount() const;
    virtual void SetEuEnabledCount(uint32_t v);
    virtual uint32_t GetEuEnabledMask() const;
    virtual void SetEuEnabledMask(uint32_t v);
};

CIF_GENERATE_VERSIONS_LIST(GTDualSubSliceInfo);

CIF_MARK_LATEST_VERSION(GTDualSubSliceInfoLatest, GTDualSubSliceInfo);
using GTDualSubSliceInfoTagOCL = GTDualSubSliceInfoLatest;    // Note : can tag with different version for
                                                              //        transition periods

}

#include "cif/macros/disable.h"
