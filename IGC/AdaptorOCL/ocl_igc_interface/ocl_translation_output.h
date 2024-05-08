/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/builtins/memory/buffer/buffer.h"
#include "cif/common/id.h"
#include "cif/common/cif.h"

#include "ocl_igc_interface/code_type.h"

#include "cif/macros/enable.h"
#include "OCLAPI/oclapi.h"

// Interface : OCL_TRA_OUT
//             "IGC - OCL" Translation Output
// Interface for managing IGC/FCL translations' outputs

namespace IGC {

CIF_DECLARE_INTERFACE(OclTranslationOutput, "OCL_TRA_OUT");

CIF_DEFINE_INTERFACE_VER(OclTranslationOutput, 1) {
  CIF_INHERIT_CONSTRUCTOR();

  OCL_API_CALL virtual bool Successful() const;
  OCL_API_CALL virtual bool HasWarnings() const;
  OCL_API_CALL virtual CodeType::CodeType_t GetOutputType() const;

  template <typename BufferInterface = CIF::Builtins::BufferLatest>
  OCL_API_CALL BufferInterface *GetBuildLog() {
    return static_cast<BufferInterface*>(GetBuildLogImpl(BufferInterface::GetVersion()));
  }

  template <typename BufferInterface = CIF::Builtins::BufferLatest>
  OCL_API_CALL BufferInterface *GetOutput() {
    return static_cast<BufferInterface*>(GetOutputImpl(BufferInterface::GetVersion()));
  }

  template <typename BufferInterface = CIF::Builtins::BufferLatest>
  OCL_API_CALL BufferInterface *GetDebugData() {
    return static_cast<BufferInterface*>(GetDebugDataImpl(BufferInterface::GetVersion()));
  }
protected:
  OCL_API_CALL virtual CIF::Builtins::BufferBase *GetBuildLogImpl(CIF::Version_t bufferVersion);
  OCL_API_CALL virtual CIF::Builtins::BufferBase *GetOutputImpl(CIF::Version_t bufferVersion);
  OCL_API_CALL virtual CIF::Builtins::BufferBase *GetDebugDataImpl(CIF::Version_t bufferVersion);
};

CIF_GENERATE_VERSIONS_LIST_AND_DECLARE_INTERFACE_DEPENDENCIES(OclTranslationOutput, CIF::Builtins::Buffer);
CIF_MARK_LATEST_VERSION(OclTranslationOutputLatest, OclTranslationOutput);
using OclTranslationOutputTagOCL = OclTranslationOutputLatest; // Note : can tag with different version for
                                                               //        transition periods

}

#include "cif/macros/disable.h"
