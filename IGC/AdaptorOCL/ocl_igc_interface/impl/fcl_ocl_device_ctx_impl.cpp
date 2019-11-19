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

#include "ocl_igc_interface/fcl_ocl_device_ctx.h"
#include "ocl_igc_interface/impl/fcl_ocl_device_ctx_impl.h"

#include "ocl_igc_interface/impl/fcl_ocl_translation_ctx_impl.h"

#include "cif/common/cif_main.h"
#include "cif/export/cif_main_impl.h"

#include "cif/export/library_api.h"

#include "cif/macros/enable.h"

namespace IGC {

void CIF_GET_INTERFACE_CLASS(FclOclDeviceCtx, 1)::SetOclApiVersion(uint32_t oclVersion){
    CIF_GET_PIMPL()->MiscOptions.OclApiVersion = oclVersion;
}

CodeType::CodeType_t CIF_GET_INTERFACE_CLASS(FclOclDeviceCtx, 2)::GetPreferredIntermediateRepresentation() {
  return CodeType::spirV;
}

FclOclTranslationCtxBase *CIF_GET_INTERFACE_CLASS(FclOclDeviceCtx, 1)::CreateTranslationCtxImpl(CIF::Version_t ver,
                                                                                                  CodeType::CodeType_t inType,
                                                                                                  CodeType::CodeType_t outType){
    return CIF_GET_PIMPL()->CreateTranslationCtx(ver, inType, outType);
}

FclOclTranslationCtxBase *CIF_PIMPL(FclOclDeviceCtx)::CreateTranslationCtx(CIF::Version_t version, CodeType::CodeType_t inType, CodeType::CodeType_t outType)
{
    if(false == CIF_PIMPL(FclOclTranslationCtx)::SupportsTranslation(inType, outType)){
        return nullptr;
    }
    return CIF::InterfaceCreator<FclOclTranslationCtx>::CreateInterfaceVer(version, version, this, inType, outType);
}

FclOclTranslationCtxBase* CIF_GET_INTERFACE_CLASS(FclOclDeviceCtx, 3)::CreateTranslationCtxImpl(CIF::Version_t ver,
                                                                                                  CodeType::CodeType_t inType,
                                                                                                  CodeType::CodeType_t outType,
                                                                                                  CIF::Builtins::BufferSimple* err) {
    return CIF_GET_PIMPL()->CreateTranslationCtx(ver, inType, outType, err);
}

FclOclTranslationCtxBase* CIF_PIMPL(FclOclDeviceCtx)::CreateTranslationCtx(CIF::Version_t version, CodeType::CodeType_t inType, CodeType::CodeType_t outType, CIF::Builtins::BufferSimple* err)
{
    if (false == CIF_PIMPL(FclOclTranslationCtx)::SupportsTranslation(inType, outType)) {
        return nullptr;
    }
    return CIF::InterfaceCreator<FclOclTranslationCtx>::CreateInterfaceVer(version, version, this, inType, outType, err);
}

}

CIF_EXPORT_ENTRY_POINTS_STATIC(IGC::FclOclDeviceCtx);

#include "cif/macros/disable.h"
