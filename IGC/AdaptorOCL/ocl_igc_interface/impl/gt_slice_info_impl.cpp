/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ocl_igc_interface/impl/gt_slice_info_impl.h"

#include "cif/macros/enable.h"

namespace IGC{

bool CIF_GET_INTERFACE_CLASS(GTSliceInfo, 1)::GetEnabled() const {
    return CIF_GET_PIMPL()->enabled;
}

void CIF_GET_INTERFACE_CLASS(GTSliceInfo, 1)::SetEnabled(bool v) {
    CIF_GET_PIMPL()->enabled = v;
}


uint32_t CIF_GET_INTERFACE_CLASS(GTSliceInfo, 1)::GetDualSubSliceCount() const {
    return CIF_GET_PIMPL()->GetDualSubSliceCount();
}

void CIF_GET_INTERFACE_CLASS(GTSliceInfo, 1)::SetDualSubSliceCount(uint32_t v) {
    CIF_GET_PIMPL()->SetDualSubsliceCount(v);
}

GTDualSubSliceInfoBase *CIF_GET_INTERFACE_CLASS(GTSliceInfo, 1)::GetDualSubSliceInfoHandleImpl(CIF::Version_t version, uint32_t dssIdx) {
    return CIF_GET_PIMPL()->GetDualSubSliceInfoHandle(version, dssIdx);
}

}

#include "cif/macros/disable.h"
