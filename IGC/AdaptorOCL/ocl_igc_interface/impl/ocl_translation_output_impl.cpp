/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ocl_igc_interface/ocl_translation_output.h"
#include "ocl_igc_interface/impl/ocl_translation_output_impl.h"

#include "cif/macros/enable.h"

namespace IGC {

bool CIF_GET_INTERFACE_CLASS(OclTranslationOutput, 1)::Successful() const {
  return CIF_GET_PIMPL()->Successful();
}

bool CIF_GET_INTERFACE_CLASS(OclTranslationOutput, 1)::HasWarnings() const {
  return CIF_GET_PIMPL()->HasWarnings();
}

CIF::Builtins::BufferBase *CIF_GET_INTERFACE_CLASS(OclTranslationOutput, 1)::GetBuildLogImpl(CIF::Version_t bufferVersion){
    return CIF_GET_PIMPL()->GetBuildLog(bufferVersion);
}

CIF::Builtins::BufferBase *CIF_GET_INTERFACE_CLASS(OclTranslationOutput, 1)::GetOutputImpl(CIF::Version_t bufferVersion){
    return CIF_GET_PIMPL()->GetOutput(bufferVersion);
}

CIF::Builtins::BufferBase *CIF_GET_INTERFACE_CLASS(OclTranslationOutput, 1)::GetDebugDataImpl(CIF::Version_t bufferVersion){
    return CIF_GET_PIMPL()->GetDebugData(bufferVersion);
}

CodeType::CodeType_t CIF_GET_INTERFACE_CLASS(OclTranslationOutput, 1)::GetOutputType() const {
  return CIF_GET_PIMPL()->GetOutputType();
}

}

#include "cif/macros/disable.h"
