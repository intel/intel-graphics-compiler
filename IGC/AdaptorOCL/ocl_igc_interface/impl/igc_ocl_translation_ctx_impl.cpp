/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ocl_igc_interface/igc_ocl_translation_ctx.h"
#include "ocl_igc_interface/impl/igc_ocl_translation_ctx_impl.h"

#ifndef WIN32
jmp_buf sig_jmp_buf;
#endif

#include "cif/macros/enable.h"

namespace IGC {

OclTranslationOutputBase *CIF_GET_INTERFACE_CLASS(IgcOclTranslationCtx, 1)::TranslateImpl(
    CIF::Version_t outVersion, CIF::Builtins::BufferSimple *src, CIF::Builtins::BufferSimple *options,
    CIF::Builtins::BufferSimple *internalOptions, CIF::Builtins::BufferSimple *tracingOptions,
    uint32_t tracingOptionsCount) {
  OclTranslationOutputBase *res;
  EX_GUARD_BEGIN
  res = CIF_GET_PIMPL()->Translate(outVersion, src, nullptr, nullptr, options, internalOptions, tracingOptions,
                                   tracingOptionsCount, nullptr);
  EX_GUARD_END
  return res;
}

OclTranslationOutputBase *CIF_GET_INTERFACE_CLASS(IgcOclTranslationCtx, 2)::TranslateImpl(
    CIF::Version_t outVersion, CIF::Builtins::BufferSimple *src, CIF::Builtins::BufferSimple *options,
    CIF::Builtins::BufferSimple *internalOptions, CIF::Builtins::BufferSimple *tracingOptions,
    uint32_t tracingOptionsCount, void *gtPinInput) {
  OclTranslationOutputBase *res;
  EX_GUARD_BEGIN
  res = CIF_GET_PIMPL()->Translate(outVersion, src, nullptr, nullptr, options, internalOptions, tracingOptions,
                                   tracingOptionsCount, gtPinInput);
  EX_GUARD_END
  return res;
}

bool CIF_GET_INTERFACE_CLASS(IgcOclTranslationCtx,
                             3)::GetSpecConstantsInfoImpl(CIF::Builtins::BufferSimple *src,
                                                          CIF::Builtins::BufferSimple *outSpecConstantsIds,
                                                          CIF::Builtins::BufferSimple *outSpecConstantsSizes) {
  return CIF_GET_PIMPL()->GetSpecConstantsInfo(src, outSpecConstantsIds, outSpecConstantsSizes);
}

OclTranslationOutputBase *CIF_GET_INTERFACE_CLASS(IgcOclTranslationCtx, 3)::TranslateImpl(
    CIF::Version_t outVersion, CIF::Builtins::BufferSimple *src, CIF::Builtins::BufferSimple *specConstantsIds,
    CIF::Builtins::BufferSimple *specConstantsValues, CIF::Builtins::BufferSimple *options,
    CIF::Builtins::BufferSimple *internalOptions, CIF::Builtins::BufferSimple *tracingOptions,
    uint32_t tracingOptionsCount, void *gtPinInput) {
  OclTranslationOutputBase *res;
  EX_GUARD_BEGIN
  res = CIF_GET_PIMPL()->Translate(outVersion, src, specConstantsIds, specConstantsValues, options, internalOptions,
                                   tracingOptions, tracingOptionsCount, gtPinInput);
  EX_GUARD_END
  return res;
}

} // namespace IGC

#include "cif/macros/disable.h"

#if defined(WIN32)
int ex_filter(unsigned int code, struct _EXCEPTION_POINTERS *ep) { return EXCEPTION_EXECUTE_HANDLER; }
#else
void signalHandler(int sig, siginfo_t *info, void *ucontext) { longjmp(sig_jmp_buf, sig); }
#endif
