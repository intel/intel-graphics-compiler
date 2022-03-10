/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ocl_igc_interface/igc_ocl_device_ctx.h"
#include "ocl_igc_interface/impl/igc_ocl_device_ctx_impl.h"

#include "ocl_igc_interface/impl/igc_ocl_translation_ctx_impl.h"

#include "cif/common/cif_main.h"
#include "cif/export/cif_main_impl.h"

#include "cif/export/library_api.h"

#include "cif/macros/enable.h"

namespace IGC {

void CIF_GET_INTERFACE_CLASS(IgcOclDeviceCtx, 1)::SetProfilingTimerResolution(float v){
    CIF_GET_PIMPL()->MiscOptions.ProfilingTimerResolution = v;
}

PlatformBase *CIF_GET_INTERFACE_CLASS(IgcOclDeviceCtx, 1)::GetPlatformHandleImpl(CIF::Version_t ver){
    return CIF_GET_PIMPL()->GetPlatformHandle(ver);
}

GTSystemInfoBase *CIF_GET_INTERFACE_CLASS(IgcOclDeviceCtx, 1)::GetGTSystemInfoHandleImpl(CIF::Version_t ver){
    return CIF_GET_PIMPL()->GetGTSystemInfoHandle(ver);
}

IgcFeaturesAndWorkaroundsBase *CIF_GET_INTERFACE_CLASS(IgcOclDeviceCtx, 1)::GetIgcFeaturesAndWorkaroundsHandleImpl(CIF::Version_t ver){
    return CIF_GET_PIMPL()->GetIgcFeaturesAndWorkaroundsHandle(ver);
}

IgcOclTranslationCtxBase *CIF_GET_INTERFACE_CLASS(IgcOclDeviceCtx, 1)::CreateTranslationCtxImpl(CIF::Version_t ver,
                                                                                                  CodeType::CodeType_t inType,
                                                                                                  CodeType::CodeType_t outType){
    return CIF_GET_PIMPL()->CreateTranslationCtx(ver, inType, outType);
}

bool CIF_GET_INTERFACE_CLASS(IgcOclDeviceCtx, 2)::GetSystemRoutine(SystemRoutineType::SystemRoutineType_t typeOfSystemRoutine,
                                                                    bool bindless,
                                                                    CIF::Builtins::BufferSimple *outSystemRoutineBuffer,
                                                                    CIF::Builtins::BufferSimple *stateSaveAreaHeaderInit) {
    return CIF_GET_PIMPL()->GetSystemRoutine(typeOfSystemRoutine, bindless, outSystemRoutineBuffer, stateSaveAreaHeaderInit);
}

const char* CIF_GET_INTERFACE_CLASS(IgcOclDeviceCtx, 3)::GetIGCRevision() {
    return CIF_GET_PIMPL()->GetIGCRevision();
}

IgcOclTranslationCtxBase *CIF_PIMPL(IgcOclDeviceCtx)::CreateTranslationCtx(CIF::Version_t version, CodeType::CodeType_t inType, CodeType::CodeType_t outType)
{
    if(false == CIF_PIMPL(IgcOclTranslationCtx)::SupportsTranslation(inType, outType)){
        return nullptr;
    }
    return CIF::InterfaceCreator<IgcOclTranslationCtx>::CreateInterfaceVer(version, version, this, inType, outType);
}

IgcBuiltinsBase *CIF_GET_INTERFACE_CLASS(IgcOclDeviceCtx, 4)::GetIgcBuiltinsHandleImpl(CIF::Version_t ver){
    return CIF_GET_PIMPL()->GetIgcBuiltinsHandle(ver);
}

}

CIF_EXPORT_ENTRY_POINTS_STATIC(IGC::IgcOclDeviceCtx);

#include "cif/macros/disable.h"
