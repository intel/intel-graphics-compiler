/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#pragma once

#include "cif/builtins/memory/buffer/buffer.h"
#include "cif/common/id.h"
#include "cif/common/cif.h"

#include "ocl_igc_interface/code_type.h"

#include "cif/macros/enable.h"

// Interface : OCL_TRA_OUT
//             "IGC - OCL" Translation Output
// Interface for managing IGC/FCL translations' outputs

namespace IGC {

CIF_DECLARE_INTERFACE(OclTranslationOutput, "OCL_TRA_OUT");

CIF_DEFINE_INTERFACE_VER(OclTranslationOutput, 1) {
  CIF_INHERIT_CONSTRUCTOR();

  virtual bool Successful() const;
  virtual bool HasWarnings() const;
  virtual CodeType::CodeType_t GetOutputType() const;

  template <typename BufferInterface = CIF::Builtins::BufferLatest>
  BufferInterface *GetBuildLog() {
    return static_cast<BufferInterface*>(GetBuildLogImpl(BufferInterface::GetVersion()));
  }

  template <typename BufferInterface = CIF::Builtins::BufferLatest>
  BufferInterface *GetOutput() {
    return static_cast<BufferInterface*>(GetOutputImpl(BufferInterface::GetVersion()));
  }

  template <typename BufferInterface = CIF::Builtins::BufferLatest>
  BufferInterface *GetDebugData() {
    return static_cast<BufferInterface*>(GetDebugDataImpl(BufferInterface::GetVersion()));
  }
protected:
  virtual CIF::Builtins::BufferBase *GetBuildLogImpl(CIF::Version_t bufferVersion);
  virtual CIF::Builtins::BufferBase *GetOutputImpl(CIF::Version_t bufferVersion);
  virtual CIF::Builtins::BufferBase *GetDebugDataImpl(CIF::Version_t bufferVersion);
};

CIF_GENERATE_VERSIONS_LIST_AND_DECLARE_INTERFACE_DEPENDENCIES(OclTranslationOutput, CIF::Builtins::Buffer);
CIF_MARK_LATEST_VERSION(OclTranslationOutputLatest, OclTranslationOutput);
using OclTranslationOutputTagOCL = OclTranslationOutputLatest; // Note : can tag with different version for
                                                               //        transition periods

}

#include "cif/macros/disable.h"
