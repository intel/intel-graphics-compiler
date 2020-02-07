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

#pragma once

#include "ocl_igc_interface/igc_ocl_device_ctx.h"

#include<mutex>

#include "cif/common/cif.h"
#include "cif/export/cif_main_impl.h"
#include "cif/export/interface_creator.h"
#include "cif/export/muiltiversion.h"
#include "cif/export/pimpl_base.h"

#include "ocl_igc_interface/impl/gt_system_info_impl.h"
#include "ocl_igc_interface/impl/igc_features_and_workarounds_impl.h"
#include "ocl_igc_interface/impl/platform_impl.h"

#include "Compiler/CISACodeGen/Platform.hpp"

#include "cif/macros/enable.h"

namespace IGC
{

CIF_DECLARE_INTERFACE_PIMPL(IgcOclDeviceCtx) : CIF::PimplBase
{
    CIF_PIMPL_DECLARE_CONSTRUCTOR(CIF::Version_t version, CIF::ICIF *parentInterface)
    {
        platform.CreateImpl();
        gtSystemInfo.CreateImpl();
        igcFeaturesAndWorkarounds.CreateImpl();
    }

    CIF_PIMPL(Platform) *GetPlatformImpl(){
        return this->platform.GetImpl();
    }

    PlatformBase * GetPlatformHandle(CIF::Version_t version)
    {
        return platform.GetVersion(version);
    }

    GTSystemInfoBase * GetGTSystemInfoHandle(CIF::Version_t version)
    {
        return gtSystemInfo.GetVersion(version);
    }

    IgcFeaturesAndWorkaroundsBase * GetIgcFeaturesAndWorkaroundsHandle(CIF::Version_t version)
    {
        return igcFeaturesAndWorkarounds.GetVersion(version);
    }

    IgcOclTranslationCtxBase * CreateTranslationCtx(CIF::Version_t version, CodeType::CodeType_t inType, CodeType::CodeType_t outType);

    struct MiscOptions
    {
        MiscOptions()
        {
            this->Clear();
        }

        void Clear()
        {
            ProfilingTimerResolution = .0f;
        }

        float ProfilingTimerResolution;
    } MiscOptions;

    const IGC::CPlatform & GetIgcCPlatform()
    {
        if(igcPlatform.get() != nullptr)
        {
            return *igcPlatform;
        }

        std::lock_guard<std::mutex> lock{this->mutex};

        if(igcPlatform.get() != nullptr)
        {
            return *igcPlatform;
        }

        igcPlatform.reset(new IGC::CPlatform(platform->p));
        igcPlatform->SetGTSystemInfo(gtSystemInfo->gsi);
        igcPlatform->setOclCaps(igcFeaturesAndWorkarounds->OCLCaps);
        IGC::SetWorkaroundTable(&igcFeaturesAndWorkarounds->FeTable, igcPlatform.get());
        IGC::SetCompilerCaps(&igcFeaturesAndWorkarounds->FeTable, igcPlatform.get());
        return *igcPlatform;
    }

protected:
    std::mutex                                   mutex;
    CIF::Multiversion<Platform>                  platform;
    CIF::Multiversion<GTSystemInfo>              gtSystemInfo;
    CIF::Multiversion<IgcFeaturesAndWorkarounds> igcFeaturesAndWorkarounds;
    std::unique_ptr<IGC::CPlatform>              igcPlatform;
};

CIF_DEFINE_INTERFACE_TO_PIMPL_FORWARDING_CTOR_DTOR(IgcOclDeviceCtx);



}

#include "cif/macros/disable.h"
