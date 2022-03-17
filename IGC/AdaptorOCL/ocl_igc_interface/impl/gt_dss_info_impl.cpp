/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ocl_igc_interface/impl/gt_dss_info_impl.h"

#include "cif/macros/enable.h"

namespace IGC{

bool CIF_GET_INTERFACE_CLASS(GTDualSubSliceInfo, 1)::GetEnabled() const {
    return CIF_GET_PIMPL()->enabled;
}

void CIF_GET_INTERFACE_CLASS(GTDualSubSliceInfo, 1)::SetEnabled(bool v) {
    CIF_GET_PIMPL()->enabled = v;
}

uint32_t CIF_GET_INTERFACE_CLASS(GTDualSubSliceInfo, 1)::GetEuEnabledCount() const {
    return CIF_GET_PIMPL()->euEnabledCount;
}

void CIF_GET_INTERFACE_CLASS(GTDualSubSliceInfo, 1)::SetEuEnabledCount(uint32_t v) {
    CIF_GET_PIMPL()->euEnabledCount = v;
}

uint32_t CIF_GET_INTERFACE_CLASS(GTDualSubSliceInfo, 1)::GetEuEnabledMask() const {
    return CIF_GET_PIMPL()->euEnabledMask;
}

void CIF_GET_INTERFACE_CLASS(GTDualSubSliceInfo, 1)::SetEuEnabledMask(uint32_t v) {
    CIF_GET_PIMPL()->euEnabledMask = v;
}

}

#include "cif/macros/disable.h"
