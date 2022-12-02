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
//
// DEFINE_GET_SET as defined in igc_features_and_workarounds_impl.cpp cannot
// handle complex objects.
#define DEFINE_GET_SET_COMPLEX(INTERFACE, VERSION, NAME, TYPE, SOURCE)\
    TYPE CIF_GET_INTERFACE_CLASS(INTERFACE, VERSION)::Get##NAME() const {\
        return static_cast<TYPE>(CIF_GET_PIMPL()->p.SOURCE);\
    }\
    void CIF_GET_INTERFACE_CLASS(INTERFACE, VERSION)::Set##NAME(TYPE v) {\
        CIF_GET_PIMPL()->p.SOURCE = static_cast<decltype(CIF_GET_PIMPL()->p.SOURCE)>(v);\
    }

// Interface version 1.
DEFINE_GET_SET_COMPLEX(Platform, 1, ProductFamily,     TypeErasedEnum, eProductFamily);
DEFINE_GET_SET_COMPLEX(Platform, 1, PCHProductFamily,  TypeErasedEnum, ePCHProductFamily);
DEFINE_GET_SET_COMPLEX(Platform, 1, DisplayCoreFamily, TypeErasedEnum, eDisplayCoreFamily);
DEFINE_GET_SET_COMPLEX(Platform, 1, RenderCoreFamily,  TypeErasedEnum, eRenderCoreFamily);
DEFINE_GET_SET_COMPLEX(Platform, 1, PlatformType,      TypeErasedEnum, ePlatformType);
DEFINE_GET_SET_COMPLEX(Platform, 1, DeviceID,          unsigned short, usDeviceID);
DEFINE_GET_SET_COMPLEX(Platform, 1, RevId,             unsigned short, usRevId);
DEFINE_GET_SET_COMPLEX(Platform, 1, DeviceID_PCH,      unsigned short, usDeviceID_PCH);
DEFINE_GET_SET_COMPLEX(Platform, 1, RevId_PCH,         unsigned short, usRevId_PCH);
DEFINE_GET_SET_COMPLEX(Platform, 1, GTType,            TypeErasedEnum, eGTType);

// Interface version 2.
//  Added Render/Media/Display block IDs
DEFINE_GET_SET_COMPLEX(Platform, 2, RenderBlockID,     unsigned int,   sRenderBlockID.Value);
DEFINE_GET_SET_COMPLEX(Platform, 2, MediaBlockID,      unsigned int,   sMediaBlockID.Value);
DEFINE_GET_SET_COMPLEX(Platform, 2, DisplayBlockID,    unsigned int,   sDisplayBlockID.Value);

#undef DEFINE_GET_SET_COMPLEX
}

#include "cif/macros/disable.h"
