/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ocl_igc_interface/fcl_ocl_device_ctx.h"
#include "ocl_igc_interface/impl/fcl_ocl_device_ctx_impl.h"

#include "ocl_igc_interface/impl/fcl_ocl_translation_ctx_impl.h"

#include "cif/common/cif_main.h"
#include "cif/export/cif_main_impl.h"

#include "cif/export/library_api.h"

#ifndef WIN32
jmp_buf sig_jmp_buf;
#else
#include <excpt.h>
#endif

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
    EX_GUARD_BEGIN
    return CIF_GET_PIMPL()->CreateTranslationCtx(ver, inType, outType);
    EX_GUARD_END
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
    EX_GUARD_BEGIN
    return CIF_GET_PIMPL()->CreateTranslationCtx(ver, inType, outType, err);
    EX_GUARD_END
}

FclOclTranslationCtxBase* CIF_PIMPL(FclOclDeviceCtx)::CreateTranslationCtx(CIF::Version_t version, CodeType::CodeType_t inType, CodeType::CodeType_t outType, CIF::Builtins::BufferSimple* err)
{
    if (false == CIF_PIMPL(FclOclTranslationCtx)::SupportsTranslation(inType, outType)) {
        return nullptr;
    }
    return CIF::InterfaceCreator<FclOclTranslationCtx>::CreateInterfaceVer(version, version, this, inType, outType, err);
}

PlatformBase *CIF_GET_INTERFACE_CLASS(FclOclDeviceCtx, 4)::GetPlatformHandleImpl(CIF::Version_t ver){
    return CIF_GET_PIMPL()->GetPlatformHandle(ver);
}

}

CIF_EXPORT_ENTRY_POINTS_STATIC(IGC::FclOclDeviceCtx);

#include "cif/macros/disable.h"

#if defined(WIN32)
int ex_filter(unsigned int code, struct _EXCEPTION_POINTERS* ep)
{
    return EXCEPTION_EXECUTE_HANDLER;
}
#else
void signalHandler(int sig, siginfo_t* info, void* ucontext)
{
    longjmp(sig_jmp_buf, sig);
}
#endif
