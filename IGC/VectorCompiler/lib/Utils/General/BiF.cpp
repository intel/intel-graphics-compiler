/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/General/BiF.h"

#include "Probe/Assertion.h"

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
    report_fatal_error(llvm::StringRef(ErrStream.str()));
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

void vc::internalizeImportedFunctions(const llvm::Module &M,
                                      const vc::FunctionNameSeq &FuncNames,
                                      bool SetAlwaysInline) {
  for (auto &Name : FuncNames) {
    auto *F = M.getFunction(Name);
    IGC_ASSERT_MESSAGE(F, "Function is expected");
    if (!F->isDeclaration()) {
      F->setLinkage(GlobalValue::LinkageTypes::InternalLinkage);
      if (SetAlwaysInline)
        F->addFnAttr(Attribute::AlwaysInline);
    }
  }
}
