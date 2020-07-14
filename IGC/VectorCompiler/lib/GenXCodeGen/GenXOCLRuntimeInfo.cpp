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

#include "GenXOCLRuntimeInfo.h"
#include "GenX.h"
#include "GenXSubtarget.h"
#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/DataLayout.h"

#include <cctype>
#include <functional>
#include <iterator>

using namespace llvm;

char GenXOCLRuntimeInfo::ID = 0;

//===----------------------------------------------------------------------===//
//
// Kernel info implementation.
//
//===----------------------------------------------------------------------===//
// Just perform linear instructions scan to find usage stats.
// Intrinsic set copied from igcmc.
void GenXOCLRuntimeInfo::KernelInfo::setInstructionUsageProperties(
    FunctionGroup &FG, const GenXSubtarget &ST) {
  for (Function *F : FG) {
    for (BasicBlock &BB : *F) {
      for (Instruction &I : BB) {
        switch (GenXIntrinsic::getGenXIntrinsicID(&I)) {
        default:
          break;
        case GenXIntrinsic::genx_group_id_x:
        case GenXIntrinsic::genx_group_id_y:
        case GenXIntrinsic::genx_group_id_z:
          UsesGroupId = true;
          break;
        case GenXIntrinsic::genx_barrier:
          UsesBarriers = true;
          break;
        case GenXIntrinsic::genx_ssdp4a:
        case GenXIntrinsic::genx_sudp4a:
        case GenXIntrinsic::genx_usdp4a:
        case GenXIntrinsic::genx_uudp4a:
        case GenXIntrinsic::genx_ssdp4a_sat:
        case GenXIntrinsic::genx_sudp4a_sat:
        case GenXIntrinsic::genx_usdp4a_sat:
        case GenXIntrinsic::genx_uudp4a_sat:
          break;
        case GenXIntrinsic::genx_alloca:
          ThreadPrivateMemSize = ST.stackSurfaceMaxSize();
          break;
        }
      }
    }
  }
}

void GenXOCLRuntimeInfo::KernelInfo::setMetadataProperties(
    genx::KernelMetadata &KM, const GenXSubtarget &ST) {
  Name = KM.getName();
  SLMSize = KM.getSLMSize();
  // FIXME: replace with 8k * simdSize * numDispatchedThreads
  if (KM.getFunction()->getParent()->getModuleFlag("genx.useGlobalMem"))
    StatelessPrivateMemSize = 16 * 8192;

}

void GenXOCLRuntimeInfo::KernelInfo::setArgumentProperties(
    const Function &Kernel, genx::KernelMetadata &KM) {
  assert(Kernel.arg_size() == KM.getNumArgs() &&
         "Expected same number of arguments");
  // Some arguments are part of thread payload and do not require
  // entries in arguments info for OCL runtime.
  auto NonPayloadArgs =
      make_filter_range(Kernel.args(), [&KM](const Argument &Arg) {
        uint32_t ArgKind = KM.getArgKind(Arg.getArgNo());
        genx::KernelArgInfo KAI(ArgKind);
        return !(KAI.isLocalIDX() || KAI.isLocalIDY() || KAI.isLocalIDZ() ||
                 KAI.isGroupOrLocalSize() || KAI.isLocalIDs());
      });
  const DataLayout &DL = Kernel.getParent()->getDataLayout();
  transform(NonPayloadArgs, std::back_inserter(ArgInfos),
            [&KM, &DL](const Argument &Arg) {
              return KernelArgInfo{Arg, KM, DL};
            });
  UsesReadWriteImages = std::any_of(
      ArgInfos.begin(), ArgInfos.end(), [](const KernelArgInfo &AI) {
        return AI.isImage() &&
               AI.getAccessKind() == KernelArgInfo::AccessKindType::ReadWrite;
      });
}

void GenXOCLRuntimeInfo::KernelInfo::setPrintStrings(
    const Module &KernelModule) {
  const auto *StringsMeta = KernelModule.getNamedMetadata("cm_print_strings");
  if (!StringsMeta)
    return;
  std::transform(StringsMeta->op_begin(), StringsMeta->op_end(),
                 std::back_inserter(PrintStrings), [](const auto *StringMeta) {
                   StringRef Str =
                       cast<MDString>(StringMeta->getOperand(0))->getString();
                   return std::string{Str.begin(), Str.end()};
                 });
}

GenXOCLRuntimeInfo::KernelInfo::KernelInfo(FunctionGroup &FG,
                                           const GenXSubtarget &ST) {
  setInstructionUsageProperties(FG, ST);

  GRFSizeInBytes = ST.getGRFWidth();

  genx::KernelMetadata KM{FG.getHead()};
  assert(KM.isKernel() && "Expected kernel as head of function group");
  setMetadataProperties(KM, ST);
  setArgumentProperties(*FG.getHead(), KM);
  setPrintStrings(*FG.getHead()->getParent());
}

//===----------------------------------------------------------------------===//
//
// Kernel argument info implementation.
//
//===----------------------------------------------------------------------===//
// Supported kernel argument attributes.
// Copied from igcmc.h.
struct OCLAttributes {
  static constexpr auto ReadOnly =
      "read_only"; // This resource is for read only.
  static constexpr auto WriteOnly =
      "write_only"; // This resource is for write only.
  static constexpr auto ReadWrite =
      "read_write"; // This resource is for read and write.
  static constexpr auto Buffer = "buffer_t";   // This resource is a buffer.
  static constexpr auto SVM = "svmptr_t";      // This resource is a SVM buffer.
  static constexpr auto Sampler = "sampler_t"; // This resource is a sampler.
  static constexpr auto Image1d = "image1d_t"; // This resource is a 1D surface.
  static constexpr auto Image1d_buffer = "image1d_buffer_t"; // This resource is a 1D surface.
  static constexpr auto Image2d = "image2d_t"; // This resource is a 2D surface.
  static constexpr auto Image3d = "image3d_t"; // This resource is a 3D surface.
};

using ArgKindType = GenXOCLRuntimeInfo::KernelArgInfo::KindType;

static auto GetStrPred = [](const char *Attr) {
  return [Attr](StringRef Token) { return Token == Attr; };
};

static ArgKindType getOCLArgKind(const SmallVectorImpl<StringRef> &Tokens,
                                 unsigned ArgNo, genx::KernelMetadata &KM) {
  unsigned RawKind = KM.getArgKind(ArgNo);

  // Implicit arguments.
  genx::KernelArgInfo KAI{RawKind};
  if (KAI.isLocalSize())
    return ArgKindType::LocalSize;
  if (KAI.isGroupCount())
    return ArgKindType::GroupCount;
  if (KAI.isPrintBuffer())
    return ArgKindType::PrintBuffer;
  if (KAI.isPrivateBase())
    return ArgKindType::PrivateBase;

  // Explicit arguments.
  switch (KM.getArgCategory(ArgNo)) {
  default:
    return ArgKindType::General;
  case genx::RegCategory::GENERAL:
    if (any_of(Tokens, GetStrPred(OCLAttributes::SVM)))
      return ArgKindType::SVM;
    return ArgKindType::General;
  case genx::RegCategory::SURFACE:
    if (any_of(Tokens, GetStrPred(OCLAttributes::Image1d)))
      return ArgKindType::Image1D;
    if (any_of(Tokens, GetStrPred(OCLAttributes::Image1d_buffer)))
      return ArgKindType::Image1D;
    if (any_of(Tokens, GetStrPred(OCLAttributes::Image2d)))
      return ArgKindType::Image2D;
    if (any_of(Tokens, GetStrPred(OCLAttributes::Image3d)))
      return ArgKindType::Image3D;
    return ArgKindType::Buffer;
  case genx::RegCategory::SAMPLER:
    return ArgKindType::Sampler;
  }
}

using ArgAccessKindType = GenXOCLRuntimeInfo::KernelArgInfo::AccessKindType;

static ArgAccessKindType
getOCLArgAccessKind(const SmallVectorImpl<StringRef> &Tokens,
                    ArgKindType Kind) {
  // As in igcmc.cpp.
  switch (Kind) {
  case ArgKindType::Buffer:
  case ArgKindType::Image1D:
  case ArgKindType::Image2D:
  case ArgKindType::Image3D:
  case ArgKindType::SVM:
    if (any_of(Tokens, GetStrPred(OCLAttributes::ReadOnly)))
      return ArgAccessKindType::ReadOnly;
    if (any_of(Tokens, GetStrPred(OCLAttributes::WriteOnly)))
      return ArgAccessKindType::WriteOnly;
    return ArgAccessKindType::ReadWrite;
  default:
    return ArgAccessKindType::None;
  }
}

// Initialize Kind and AccessKind from given ArgTypeDesc in metadata.
void GenXOCLRuntimeInfo::KernelArgInfo::translateArgDesc(
    genx::KernelMetadata &KM) {
  std::string Translated{KM.getArgTypeDesc(Index)};
  // Transform each separator to space.
  std::transform(Translated.begin(), Translated.end(), Translated.begin(),
                 [](char C) {
                   if (C != '-' && C != '_' && C != '=' && !std::isalnum(C))
                     return ' ';
                   return C;
                 });

  // Split and delete duplicates.
  SmallVector<StringRef, 4> Tokens;
  StringRef(Translated)
      .split(Tokens, ' ', -1 /* MaxSplit */, false /* AllowEmpty */);
  std::sort(Tokens.begin(), Tokens.end());
  Tokens.erase(std::unique(Tokens.begin(), Tokens.end()), Tokens.end());

  Kind = getOCLArgKind(Tokens, Index, KM);
  AccessKind = getOCLArgAccessKind(Tokens, Kind);
}

static unsigned getArgSizeInBytes(const Argument &Arg, genx::KernelMetadata &KM,
                                  const DataLayout &DL) {
  Type *ArgTy = Arg.getType();
  if (ArgTy->isPointerTy())
    return DL.getPointerTypeSize(ArgTy);
  if (KM.isBufferType(Arg.getArgNo()))
    return DL.getPointerSize();
  return ArgTy->getPrimitiveSizeInBits() / genx::ByteBits;
}

GenXOCLRuntimeInfo::KernelArgInfo::KernelArgInfo(const Argument &Arg,
                                                 genx::KernelMetadata &KM,
                                                 const DataLayout &DL)
    : Index(Arg.getArgNo()) {
  translateArgDesc(KM);
  Offset = KM.getArgOffset(Index);
  SizeInBytes = getArgSizeInBytes(Arg, KM, DL);
  BTI = KM.getBTI(Index);
}

//===----------------------------------------------------------------------===//
//
// Compiled kernel implementation.
//
//===----------------------------------------------------------------------===//
GenXOCLRuntimeInfo::CompiledKernel::CompiledKernel(KernelInfo &&KI,
                                                   const FINALIZER_INFO &JI,
                                                   ArrayRef<char> GenBin)
    : CompilerInfo(std::move(KI)), JitterInfo(JI),
      GenBinary(GenBin.begin(), GenBin.end()) {}

INITIALIZE_PASS_BEGIN(GenXOCLRuntimeInfo, "GenXOCLRuntimeInfo",
                      "GenXOCLRuntimeInfo", false, true)
INITIALIZE_PASS_END(GenXOCLRuntimeInfo, "GenXOCLRuntimeInfo",
                    "GenXOCLRuntimeInfo", false, true)
