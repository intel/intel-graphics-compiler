/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/common/id.h"
#include "cif/common/cif.h"

#include "ocl_igc_interface/code_type.h"
#include "ocl_igc_interface/fcl_ocl_translation_ctx.h"
#include "ocl_igc_interface/platform.h"

#include <cinttypes>

#include "cif/macros/enable.h"

// Interface : FCL_OCL_DEVC
//             "Frontent Compiler - OCL" Device Context
// Interface for defining target OCL device

namespace IGC {

CIF_DECLARE_INTERFACE(FclOclDeviceCtx, "FCL_OCL_DEVC");

CIF_DEFINE_INTERFACE_VER(FclOclDeviceCtx, 1){
  CIF_INHERIT_CONSTRUCTOR();

  // oclApiVersion (as supported by OCL runtime) format : version = major_revision*100 + minor_revision*10 +  sub_revision
  //                                             e.g. OCL2.1 = 210
  virtual void SetOclApiVersion(uint32_t version);

  template <typename FclOclTranslationCtxInterface = FclOclTranslationCtxTagOCL>
  CIF::RAII::UPtr_t<FclOclTranslationCtxInterface> CreateTranslationCtx(CodeType::CodeType_t inType, CodeType::CodeType_t outType) {
    return CIF::RAII::Pack<FclOclTranslationCtxInterface>( CreateTranslationCtxImpl(FclOclTranslationCtxInterface::GetVersion(), inType, outType) );
  }

protected:
  virtual FclOclTranslationCtxBase *CreateTranslationCtxImpl(CIF::Version_t ver,
                                                             CodeType::CodeType_t inType,
                                                             CodeType::CodeType_t outType);
};

CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(FclOclDeviceCtx, 2, 1) {
  CIF_INHERIT_CONSTRUCTOR();
  virtual CodeType::CodeType_t GetPreferredIntermediateRepresentation();
};

CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(FclOclDeviceCtx, 3, 2) {
    CIF_INHERIT_CONSTRUCTOR();
    using FclOclDeviceCtx<1>::CreateTranslationCtxImpl;
    using FclOclDeviceCtx<1>::CreateTranslationCtx;

    template <typename FclOclTranslationCtxInterface = FclOclTranslationCtxTagOCL>
    CIF::RAII::UPtr_t<FclOclTranslationCtxInterface> CreateTranslationCtx(CodeType::CodeType_t inType, CodeType::CodeType_t outType, CIF::Builtins::BufferSimple* err) {
        return CIF::RAII::Pack<FclOclTranslationCtxInterface>(CreateTranslationCtxImpl(FclOclTranslationCtxInterface::GetVersion(), inType, outType, err));
    }

protected:
    virtual FclOclTranslationCtxBase* CreateTranslationCtxImpl(CIF::Version_t ver,
                                                              CodeType::CodeType_t inType,
                                                              CodeType::CodeType_t outType,
                                                              CIF::Builtins::BufferSimple* err);
};

CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(FclOclDeviceCtx, 4, 3) {
    CIF_INHERIT_CONSTRUCTOR();

    template <typename PlatformInterface = PlatformTagOCL>
    CIF::RAII::UPtr_t<PlatformInterface> GetPlatformHandle() {
        return CIF::RAII::RetainAndPack<PlatformInterface>( GetPlatformHandleImpl(PlatformInterface::GetVersion()) );
    }

protected:
    virtual PlatformBase *GetPlatformHandleImpl(CIF::Version_t ver);
};

CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(FclOclDeviceCtx, 5, 4) {
    CIF_INHERIT_CONSTRUCTOR();
};

CIF_GENERATE_VERSIONS_LIST_AND_DECLARE_INTERFACE_DEPENDENCIES(FclOclDeviceCtx, IGC::FclOclTranslationCtx, IGC::Platform);
CIF_MARK_LATEST_VERSION(FclOclDeviceCtxLatest, FclOclDeviceCtx);
using FclOclDeviceCtxTagOCL = FclOclDeviceCtxLatest; // Note : can tag with different version for
                                                     //        transition periods
}

#include "cif/macros/disable.h"
