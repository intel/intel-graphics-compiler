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

#include "ocl_igc_interface/fcl_ocl_device_ctx.h"

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
    }

    FclOclTranslationCtxBase * CreateTranslationCtx(CIF::Version_t version, CodeType::CodeType_t inType, CodeType::CodeType_t outType);
    FclOclTranslationCtxBase * CreateTranslationCtx(CIF::Version_t version, CodeType::CodeType_t inType, CodeType::CodeType_t outType, CIF::Builtins::BufferSimple* err);

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
};

CIF_DEFINE_INTERFACE_TO_PIMPL_FORWARDING_CTOR_DTOR(FclOclDeviceCtx);

}

#include "cif/macros/disable.h"
