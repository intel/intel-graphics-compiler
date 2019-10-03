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

#include "cif/common/id.h"
#include "cif/common/cif.h"

#include "ocl_igc_interface/code_type.h"
#include "ocl_igc_interface/gt_system_info.h"
#include "ocl_igc_interface/igc_features_and_workarounds.h"
#include "ocl_igc_interface/igc_ocl_translation_ctx.h"
#include "ocl_igc_interface/ocl_gen_binary.h"
#include "ocl_igc_interface/platform.h"

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

CIF_GENERATE_VERSIONS_LIST_AND_DECLARE_INTERFACE_DEPENDENCIES(IgcOclDeviceCtx, IGC::Platform, IGC::GTSystemInfo,
                                                                               IGC::OclGenBinary,
                                                                               IGC::IgcFeaturesAndWorkarounds,
                                                                               IGC::IgcOclTranslationCtx
                                                             );
CIF_MARK_LATEST_VERSION(IgcOclDeviceCtxLatest, IgcOclDeviceCtx);
using IgcOclDeviceCtxTagOCL = IgcOclDeviceCtxLatest; // Note : can tag with different version for
                                                             //        transition periods

}

#include "cif/macros/disable.h"
