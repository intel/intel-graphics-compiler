/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <cinttypes>

#include "cif/builtins/memory/buffer/buffer.h"
#include "cif/common/id.h"
#include "cif/common/cif.h"

#include "ocl_igc_interface/ocl_translation_output.h"

#include "cif/macros/enable.h"

// Interface : FCL_OCL_TRAC
//             "Frontent Compiler - OCL" Translation Context
// Interface for invoking common-clang translations from OCL runtime

namespace IGC {
// FCL(clang)-OCL translation context
CIF_DECLARE_INTERFACE(FclOclTranslationCtx, "FCL_OCL_TRAC");

CIF_DEFINE_INTERFACE_VER(FclOclTranslationCtx, 1) {
  CIF_INHERIT_CONSTRUCTOR();

  template <typename OclTranslationOutputInterface = OclTranslationOutputTagOCL>
  CIF::RAII::UPtr_t<OclTranslationOutputInterface> Translate(
                                                             CIF::Builtins::BufferSimple *src,
                                                             CIF::Builtins::BufferSimple *options,
                                                             CIF::Builtins::BufferSimple *internalOptions,
                                                             CIF::Builtins::BufferSimple *tracingOptions,
                                                             uint32_t tracingOptionsCount
                                                             ) {
      auto p = TranslateImpl(OclTranslationOutputInterface::GetVersion(),
                             src, options, internalOptions, tracingOptions, tracingOptionsCount);
      return CIF::RAII::Pack<OclTranslationOutputInterface>(p);
  }

protected:
  virtual OclTranslationOutputBase *TranslateImpl(CIF::Version_t outVersion,
                                                  CIF::Builtins::BufferSimple *src,
                                                  CIF::Builtins::BufferSimple *options,
                                                  CIF::Builtins::BufferSimple *internalOptions,
                                                  CIF::Builtins::BufferSimple *tracingOptions,
                                                  uint32_t tracingOptionsCount);
};

CIF_DEFINE_INTERFACE_VER_WITH_COMPATIBILITY(FclOclTranslationCtx, 2, 1) {
  CIF_INHERIT_CONSTRUCTOR();
  virtual void GetFclOptions(CIF::Builtins::BufferSimple *options);
  virtual void GetFclInternalOptions(CIF::Builtins::BufferSimple *internalOptions);
};

CIF_GENERATE_VERSIONS_LIST_AND_DECLARE_INTERFACE_DEPENDENCIES(FclOclTranslationCtx, IGC::OclTranslationOutput, CIF::Builtins::Buffer);
CIF_MARK_LATEST_VERSION(FclOclTranslationCtxLatest, FclOclTranslationCtx);
using FclOclTranslationCtxTagOCL =
    FclOclTranslationCtxLatest; // Note : can tag with different version for
                                //        transition periods
}

#include "cif/macros/disable.h"
