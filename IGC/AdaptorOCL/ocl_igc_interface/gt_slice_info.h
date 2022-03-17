/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/common/id.h"
#include "cif/common/cif.h"

#include "ocl_igc_interface/gt_dss_info.h"

#include "cif/macros/enable.h"

// Interface : GT_SLICE_INFO
//             GT Slice Info
// Interface for defining target device

namespace IGC {

CIF_DECLARE_INTERFACE(GTSliceInfo, "GT_SLICE_INF")

CIF_DEFINE_INTERFACE_VER(GTSliceInfo, 1){
    CIF_INHERIT_CONSTRUCTOR();

    virtual bool GetEnabled() const;
    virtual void SetEnabled(bool v);

    virtual uint32_t GetDualSubSliceCount() const;
    virtual void SetDualSubSliceCount(uint32_t v);

    template <typename GTDualSubSliceInfoInterface = GTDualSubSliceInfoTagOCL>
    CIF::RAII::UPtr_t<GTDualSubSliceInfoInterface> *GetDualSubSliceInfoHandle(uint32_t dssIdx) {
      return CIF::RAII::RetainAndPack<GTDualSubSliceInfoInterface>( GetGTDSSInfoHandleImpl(GTDualSubSliceInfoInterface::GetVersion(), dssIdx) );
    }

protected:
    virtual GTDualSubSliceInfoBase *GetDualSubSliceInfoHandleImpl(CIF::Version_t ver, uint32_t dssIdx);
};

CIF_GENERATE_VERSIONS_LIST_AND_DECLARE_INTERFACE_DEPENDENCIES(GTSliceInfo, IGC::GTDualSubSliceInfo);

CIF_MARK_LATEST_VERSION(GTSliceInfoLatest, GTSliceInfo);
using GTSliceInfoTagOCL = GTSliceInfoLatest;    // Note : can tag with different version for
                                                 //        transition periods

}

#include "cif/macros/disable.h"
