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

#include "vc/GenXOpts/Utils/BiFTools.h"

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
llvm::getBiFModuleOrReportError(MemoryBufferRef BiFModuleBuffer,
                                LLVMContext &Ctx) {
  return getBiFModuleOrReportErrorImpl(
      BiFModuleBuffer, Ctx,
      [](MemoryBufferRef BiFModuleBufferIn, LLVMContext &CtxIn) {
        return parseBitcodeFile(BiFModuleBufferIn, CtxIn);
      });
}

std::unique_ptr<Module>
llvm::getLazyBiFModuleOrReportError(MemoryBufferRef BiFModuleBuffer,
                                    LLVMContext &Ctx) {
  return getBiFModuleOrReportErrorImpl(
      BiFModuleBuffer, Ctx,
      [](MemoryBufferRef BiFModuleBufferIn, LLVMContext &CtxIn) {
        return getLazyBitcodeModule(BiFModuleBufferIn, CtxIn);
      });
}
