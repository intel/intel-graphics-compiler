/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "ocl_igc_interface/igc_features_and_workarounds.h"
#include "ocl_igc_interface/impl/igc_features_and_workarounds_impl.h"

#include "cif/macros/enable.h"
#include "Probe.h"

namespace IGC {

// Helpers for clarity
// Basically, these forward GetX/SetX from interface (of given version)
// to GT_SYSTEM_INFO inside pImpl
#define DEFINE_GET_SET(INTERFACE, VERSION, NAME, TYPE)\
    TYPE CIF_GET_INTERFACE_CLASS(INTERFACE, VERSION)::Get##NAME() const {\
        return CIF_GET_PIMPL()->FeTable.NAME;\
    }\
    void CIF_GET_INTERFACE_CLASS(INTERFACE, VERSION)::Set##NAME(TYPE v) {\
        CIF_GET_PIMPL()->FeTable.NAME = v;\
    }

DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrDesktop, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrChannelSwizzlingXOREnabled, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrGtBigDie, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrGtMediumDie, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrGtSmallDie, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrGT1, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrGT1_5, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrGT2, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrGT3, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrGT4, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrIVBM0M1Platform, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrGTL, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrGTM, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrGTH, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrSGTPVSKUStrapPresent, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrGTA, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrGTC, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrGTX, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, Ftr5Slice, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrGpGpuMidThreadLevelPreempt, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrIoMmuPageFaulting, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrWddm2Svm, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrPooledEuEnabled, bool);
DEFINE_GET_SET(IgcFeaturesAndWorkarounds, 1, FtrResourceStreamer, bool);

uint32_t CIF_GET_INTERFACE_CLASS(IgcFeaturesAndWorkarounds, 2)::GetMaxOCLParamSize() const {
    return CIF_GET_PIMPL()->OCLCaps.MaxParameterSize;
}
void CIF_GET_INTERFACE_CLASS(IgcFeaturesAndWorkarounds, 2)::SetMaxOCLParamSize(uint32_t s) {
    IGC_ASSERT(s >= OCLCaps::MINIMAL_MAX_PARAMETER_SIZE);
    CIF_GET_PIMPL()->OCLCaps.MaxParameterSize = s;
}

}

#include "cif/macros/disable.h"
