/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ocl_igc_interface/platform.h"
#include "ocl_igc_interface/impl/platform_impl.h"

#include "cif/macros/enable.h"

namespace IGC {

// Helpers for clarity
// Basically, these forward GetX/SetX from interface (of given version)
// to GT_SYSTEM_INFO inside pImpl
// Prefix is used because members in platform are in hungarian notation
#define DEFINE_GET_SET_PREFIX(INTERFACE, VERSION, NAME, TYPE, PREFIX)\
    TYPE CIF_GET_INTERFACE_CLASS(INTERFACE, VERSION)::Get##NAME() const {\
        return static_cast<TYPE>(CIF_GET_PIMPL()->p.PREFIX##NAME);\
    }\
    void CIF_GET_INTERFACE_CLASS(INTERFACE, VERSION)::Set##NAME(TYPE v) {\
        CIF_GET_PIMPL()->p.PREFIX##NAME = static_cast<decltype(CIF_GET_PIMPL()->p.PREFIX##NAME)>(v);\
    }

DEFINE_GET_SET_PREFIX(Platform, 1, ProductFamily, TypeErasedEnum, e);
DEFINE_GET_SET_PREFIX(Platform, 1, PCHProductFamily, TypeErasedEnum, e);
DEFINE_GET_SET_PREFIX(Platform, 1, DisplayCoreFamily, TypeErasedEnum, e);
DEFINE_GET_SET_PREFIX(Platform, 1, RenderCoreFamily, TypeErasedEnum, e);
DEFINE_GET_SET_PREFIX(Platform, 1, PlatformType, TypeErasedEnum, e);
DEFINE_GET_SET_PREFIX(Platform, 1, DeviceID, unsigned short, us);
DEFINE_GET_SET_PREFIX(Platform, 1, RevId, unsigned short, us);
DEFINE_GET_SET_PREFIX(Platform, 1, DeviceID_PCH, unsigned short, us);
DEFINE_GET_SET_PREFIX(Platform, 1, RevId_PCH, unsigned short, us);
DEFINE_GET_SET_PREFIX(Platform, 1, GTType, TypeErasedEnum, e);

}

#include "cif/macros/disable.h"
