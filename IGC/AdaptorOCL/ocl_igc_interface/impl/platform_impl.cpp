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
