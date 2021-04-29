/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "ocl_igc_interface/fcl_ocl_device_ctx.h"
#include "ocl_igc_interface/impl/platform_impl.h"

#include <cinttypes>
#include <mutex>

#include "cif/common/cif.h"
#include "cif/export/cif_main_impl.h"
#include "cif/export/interface_creator.h"
#include "cif/export/muiltiversion.h"
#include "cif/export/pimpl_base.h"

#include "cif/macros/enable.h"

namespace IGC
{

CIF_DECLARE_INTERFACE_PIMPL(FclOclDeviceCtx) : CIF::PimplBase
{
    CIF_PIMPL_DECLARE_CONSTRUCTOR(CIF::Version_t version, CIF::ICIF *parentInterface)
    {
        platform.CreateImpl();
    }

    FclOclTranslationCtxBase * CreateTranslationCtx(CIF::Version_t version, CodeType::CodeType_t inType, CodeType::CodeType_t outType);
    FclOclTranslationCtxBase * CreateTranslationCtx(CIF::Version_t version, CodeType::CodeType_t inType, CodeType::CodeType_t outType, CIF::Builtins::BufferSimple* err);
    
    PlatformBase * GetPlatformHandle(CIF::Version_t version)
    {
        return platform.GetVersion(version);
    }

    CIF_PIMPL(Platform)* GetPlatformImpl()
    {
        return this->platform.GetImpl();
    }

    struct MiscOptions
    {
        MiscOptions()
        {
            this->Clear();
        }

        void Clear()
        {
            OclApiVersion = 120;
        }

        // oclApiVersion format : version = major_revision*100 + minor_revision*10 +  sub_revision
        //                                    e.g. OCL2.1 = 210
        uint32_t OclApiVersion;
    } MiscOptions;

protected:
    CIF::Multiversion<Platform> platform;
};

CIF_DEFINE_INTERFACE_TO_PIMPL_FORWARDING_CTOR_DTOR(FclOclDeviceCtx);

}

#include "cif/macros/disable.h"
