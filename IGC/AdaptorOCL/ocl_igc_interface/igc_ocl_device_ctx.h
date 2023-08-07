/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/common/id.h"
#include "cif/common/cif.h"

#include "ocl_igc_interface/code_type.h"
#include "ocl_igc_interface/gt_system_info.h"
#include "ocl_igc_interface/igc_features_and_workarounds.h"
#include "ocl_igc_interface/igc_ocl_translation_ctx.h"
#include "ocl_igc_interface/ocl_gen_binary.h"
#include "ocl_igc_interface/platform.h"
#include "ocl_igc_interface/igc_builtins.h"

#include "cif/macros/enable.h"

// Interface : IGC_OCL_DEVC
//             "IGC - OCL" Device Context
// Interface for defining target OCL device

namespace IGC {

CIF_DECLARE_INTERFACE(IgcOclDeviceCtx, "IGC_OCL_DEVC");

CIF_DEFINE_INTERFACE_VER(IgcOclDeviceCtx, 1){
  CIF_INHERIT_CONSTRUCTOR();

  template <typename PlatformInterface = PlatformTagOCL>
  CIF::RAII::UPtr_t<PlatformInterface> GetPlatformHandle() {
    return CIF::RAII::RetainAndPack<PlatformInterface>( GetPlatformHandleImpl(PlatformInterface::GetVersion()) );
  }

  template <typename GTSystemInfoInterface = GTSystemInfoTagOCL>
  CIF::RAII::UPtr_t<GTSystemInfoInterface> GetGTSystemInfoHandle() {
    return CIF::RAII::RetainAndPack<GTSystemInfoInterface>( GetGTSystemInfoHandleImpl(GTSystemInfoInterface::GetVersion()) );
  }

  template <typename IgcFeaturesAndWorkaroundsInterface = IgcFeaturesAndWorkaroundsTagOCL>
  CIF::RAII::UPtr_t<IgcFeaturesAndWorkaroundsInterface> GetIgcFeaturesAndWorkaroundsHandle() {
    return CIF::RAII::RetainAndPack<IgcFeaturesAndWorkaroundsInterface>(  GetIgcFeaturesAndWorkaroundsHandleImpl(IgcFeaturesAndWorkaroundsInterface::GetVersion()) );
  }

  virtual void SetProfilingTimerResolution(float v);

  template <typename OclIgcTranslationInterface = IgcOclTranslationCtxTagOCL>
  CIF::RAII::UPtr_t<OclIgcTranslationInterface> CreateTranslationCtx(CodeType::CodeType_t inType, CodeType::CodeType_t outType) {
    return CIF::RAII::Pack<OclIgcTranslationInterface>( CreateTranslationCtxImpl(OclIgcTranslationInterface::GetVersion(), inType, outType) );
  }

protected:
  virtual PlatformBase *GetPlatformHandleImpl(CIF::Version_t ver);
  virtual GTSystemInfoBase *GetGTSystemInfoHandleImpl(CIF::Version_t ver);
  virtual IgcFeaturesAndWorkaroundsBase *GetIgcFeaturesAndWorkaroundsHandleImpl(CIF::Version_t ver);
  virtual IgcOclTranslationCtxBase *CreateTranslationCtxImpl(CIF::Version_t ver,
                                                             CodeType::CodeType_t inType,
                                                             CodeType::CodeType_t outType);
};

namespace SystemRoutineType {
    using SystemRoutineType_t = uint64_t;
    using SystemRoutineTypeCoder = CIF::Coder<SystemRoutineType_t>;
    constexpr auto contextSaveRestore = SystemRoutineTypeCoder::Enc("CSR");
    constexpr auto debug = SystemRoutineTypeCoder::Enc("DBG");
    constexpr auto debugSlm = SystemRoutineTypeCoder::Enc("DBG_SLM");
    constexpr auto undefined = SystemRoutineTypeCoder::Enc("UNDEFINED");
    constexpr auto invalid = SystemRoutineTypeCoder::Enc("INVALID");
}

CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(IgcOclDeviceCtx, 2, 1) {
    CIF_INHERIT_CONSTRUCTOR();

    virtual bool GetSystemRoutine(SystemRoutineType::SystemRoutineType_t typeOfSystemRoutine,
                                    bool bindless,
                                    CIF::Builtins::BufferSimple *outSystemRoutineBuffer,
                                    CIF::Builtins::BufferSimple *stateSaveAreaHeaderInit);
};

CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(IgcOclDeviceCtx, 3, 2) {
  CIF_INHERIT_CONSTRUCTOR();

  virtual const char* GetIGCRevision();
};

CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(IgcOclDeviceCtx, 4, 3) {
    CIF_INHERIT_CONSTRUCTOR();

    template <typename IgcBuiltinsInterface = IgcBuiltinsLatest>
    CIF::RAII::UPtr_t<IgcBuiltinsInterface> GetIgcBuiltinsHandle() {
        return CIF::RAII::RetainAndPack<IgcBuiltinsInterface>( GetIgcBuiltinsHandleImpl(IgcBuiltinsInterface::GetVersion()) );
    }

protected:
    virtual IgcBuiltinsBase *GetIgcBuiltinsHandleImpl(CIF::Version_t ver);
};

CIF_GENERATE_VERSIONS_LIST_AND_DECLARE_INTERFACE_DEPENDENCIES(IgcOclDeviceCtx, IGC::Platform, IGC::GTSystemInfo,
                                                                               IGC::OclGenBinary,
                                                                               IGC::IgcFeaturesAndWorkarounds,
                                                                               IGC::IgcOclTranslationCtx,
                                                                               IGC::IgcBuiltins
                                                             );
CIF_MARK_LATEST_VERSION(IgcOclDeviceCtxLatest, IgcOclDeviceCtx);
using IgcOclDeviceCtxTagOCL = IgcOclDeviceCtx<2>; // Note : can tag with different version for
                                                             //        transition periods

}

#include "cif/macros/disable.h"
