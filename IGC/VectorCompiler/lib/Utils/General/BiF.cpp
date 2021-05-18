/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/General/BiF.h"

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Support/Error.h>

#include <sstream>
#include <utility>

using namespace llvm;

template <typename DecoderT>
std::unique_ptr<Module>
getBiFModuleOrReportErrorImpl(MemoryBufferRef BiFModuleBuffer, LLVMContext &Ctx,
                              DecoderT Decoder) {
  Expected<std::unique_ptr<Module>> BiFModule = Decoder(BiFModuleBuffer, Ctx);
  if (!BiFModule) {
    auto Err = BiFModule.takeError();
    std::stringstream ErrStream;
    ErrStream << "BiF module decoding has failed"
                 "because of the following errors:\n";
    handleAllErrors(std::move(Err),
                    [&ErrStream](const llvm::ErrorInfoBase &EI) {
                      ErrStream << EI.message() << std::endl;
                    });
    report_fatal_error(ErrStream.str());
  }
  return std::move(BiFModule.get());
}

std::unique_ptr<Module>
vc::getBiFModuleOrReportError(MemoryBufferRef BiFModuleBuffer,
                              LLVMContext &Ctx) {
  return getBiFModuleOrReportErrorImpl(
      BiFModuleBuffer, Ctx,
      [](MemoryBufferRef BiFModuleBufferIn, LLVMContext &CtxIn) {
        return parseBitcodeFile(BiFModuleBufferIn, CtxIn);
      });
}

std::unique_ptr<Module>
vc::getLazyBiFModuleOrReportError(MemoryBufferRef BiFModuleBuffer,
                                  LLVMContext &Ctx) {
  return getBiFModuleOrReportErrorImpl(
      BiFModuleBuffer, Ctx,
      [](MemoryBufferRef BiFModuleBufferIn, LLVMContext &CtxIn) {
        return getLazyBitcodeModule(BiFModuleBufferIn, CtxIn);
      });
}
