/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "ocl_igc_interface/igc_ocl_device_ctx.h"

#include <mutex>

#include "cif/common/cif.h"
#include "cif/export/cif_main_impl.h"
#include "cif/export/interface_creator.h"
#include "cif/export/muiltiversion.h"
#include "cif/export/pimpl_base.h"

#include "ocl_igc_interface/impl/gt_system_info_impl.h"
#include "ocl_igc_interface/impl/igc_features_and_workarounds_impl.h"
#include "ocl_igc_interface/impl/platform_impl.h"
#include "ocl_igc_interface/impl/igc_builtins_impl.h"
#include "ocl_igc_interface/impl/igc_options_and_capabilities_impl.h"

#include "Compiler/CISACodeGen/Platform.hpp"
#include "common/SystemThread.h"

#include "cif/macros/enable.h"
#include "version.h"

#include "OCLAPI/oclapi.h"

namespace IGC {

CIF_DECLARE_INTERFACE_PIMPL(IgcOclDeviceCtx) : CIF::PimplBase {
  OCL_API_CALL CIF_PIMPL_DECLARE_CONSTRUCTOR(CIF::Version_t version, CIF::ICIF * parentInterface) {
    platform.CreateImpl();
    gtSystemInfo.CreateImpl();
    igcFeaturesAndWorkarounds.CreateImpl();
    igcBuiltins.CreateImpl();
    igcOptionsAndCapabilities.CreateImpl();
  }

  OCL_API_CALL CIF_PIMPL(Platform) * GetPlatformImpl() { return this->platform.GetImpl(); }

  OCL_API_CALL PlatformBase *GetPlatformHandle(CIF::Version_t version) { return platform.GetVersion(version); }

  OCL_API_CALL GTSystemInfoBase *GetGTSystemInfoHandle(CIF::Version_t version) {
    return gtSystemInfo.GetVersion(version);
  }

  OCL_API_CALL IgcFeaturesAndWorkaroundsBase *GetIgcFeaturesAndWorkaroundsHandle(CIF::Version_t version) {
    return igcFeaturesAndWorkarounds.GetVersion(version);
  }

  OCL_API_CALL IgcOptionsAndCapabilitiesBase *GetIgcOptionsAndCapabilitiesHandle(CIF::Version_t version) {
    return igcOptionsAndCapabilities.GetVersion(version);
  }

  OCL_API_CALL IgcBuiltinsBase *GetIgcBuiltinsHandle(CIF::Version_t version) { return igcBuiltins.GetVersion(version); }

  OCL_API_CALL IgcOclTranslationCtxBase *CreateTranslationCtx(CIF::Version_t version, CodeType::CodeType_t inType,
                                                              CodeType::CodeType_t outType);

  OCL_API_CALL bool GetSystemRoutine(SystemRoutineType::SystemRoutineType_t typeOfSystemRoutine, bool bindless,
                                     CIF::Builtins::BufferSimple *outSystemRoutineBuffer,
                                     CIF::Builtins::BufferSimple *stateSaveAreaHeaderInit) {
    if (nullptr == outSystemRoutineBuffer) {
      return false;
    }

    USC::SYSTEM_THREAD_MODE mode;
    switch (typeOfSystemRoutine) {
    case SystemRoutineType::contextSaveRestore:
      mode = USC::SYSTEM_THREAD_MODE_CSR;
      break;
    case SystemRoutineType::debug:
      mode = static_cast<USC::SYSTEM_THREAD_MODE>(USC::SYSTEM_THREAD_MODE_DEBUG | USC::SYSTEM_THREAD_MODE_CSR);
      break;
    case SystemRoutineType::debugSlm:
      mode = static_cast<USC::SYSTEM_THREAD_MODE>(USC::SYSTEM_THREAD_MODE_DEBUG_LOCAL | USC::SYSTEM_THREAD_MODE_CSR);
      break;
    default:
      assert(0);
      return false;
    }

    USC::SSystemThreadKernelOutput *systemKernel = nullptr;
    if (false == SIP::CSystemThread::CreateSystemThreadKernel(this->GetIgcCPlatform(), mode, systemKernel, bindless)) {
      return false;
    }

    outSystemRoutineBuffer->Clear();
    outSystemRoutineBuffer->PushBackRawBytes(systemKernel->m_pKernelProgram, systemKernel->m_KernelProgramSize);
    stateSaveAreaHeaderInit->Clear();
    stateSaveAreaHeaderInit->PushBackRawBytes(systemKernel->m_pStateSaveAreaHeader,
                                              systemKernel->m_StateSaveAreaHeaderSize);
    SIP::CSystemThread::DeleteSystemThreadKernel(systemKernel);
    return true;
  }

  const char *GetIGCRevision() {
#ifdef IGC_REVISION
    return IGC_REVISION;
#else
    return "";
#endif
  }

  struct OCL_API_CALL MiscOptions {
    MiscOptions() { this->Clear(); }

    void Clear() { ProfilingTimerResolution = .0f; }

    float ProfilingTimerResolution;
  } MiscOptions;

  OCL_API_CALL const IGC::CPlatform &GetIgcCPlatform() {
    // "fast path" where we return initialized igcPlatform without a lock
    if (igcPlatform.get() != nullptr) {
      return *igcPlatform;
    }

    // acquire a lock to if igcPlatform is uninitialized
    std::lock_guard<std::mutex> lock{this->mutex};

    // check if other thread initialized igcPlatform first
    if (igcPlatform.get() != nullptr) {
      return *igcPlatform;
    }

    // construct fully initialized CPlatform object
    IGC::CPlatform *new_platform = new IGC::CPlatform(platform->p);
    new_platform->SetGTSystemInfo(gtSystemInfo->gsi);
    new_platform->setOclCaps(igcFeaturesAndWorkarounds->OCLCaps);
    IGC::SetWorkaroundTable(&igcFeaturesAndWorkarounds->FeTable, new_platform);
    IGC::SetCompilerCaps(&igcFeaturesAndWorkarounds->FeTable, new_platform);
    // assign new_platform to igcPlatform after a full initialization
    // is finished to avoid race conditions
    igcPlatform.reset(new_platform);
    return *igcPlatform;
  }

protected:
  std::mutex mutex;
  CIF::Multiversion<Platform> platform;
  CIF::Multiversion<GTSystemInfo> gtSystemInfo;
  CIF::Multiversion<IgcFeaturesAndWorkarounds> igcFeaturesAndWorkarounds;
  CIF::Multiversion<IgcOptionsAndCapabilities> igcOptionsAndCapabilities;
  CIF::Multiversion<IgcBuiltins> igcBuiltins;
  std::unique_ptr<IGC::CPlatform> igcPlatform;
};

CIF_DEFINE_INTERFACE_TO_PIMPL_FORWARDING_CTOR_DTOR(IgcOclDeviceCtx);

} // namespace IGC

#include "cif/macros/disable.h"
