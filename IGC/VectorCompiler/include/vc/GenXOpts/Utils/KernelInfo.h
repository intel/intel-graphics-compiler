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

#ifndef GENX_KERNEL_INFO_H
#define GENX_KERNEL_INFO_H

#include "vc/GenXOpts/Utils/RegCategory.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"

namespace llvm {
namespace genx {

enum { VISA_MAJOR_VERSION = 3, VISA_MINOR_VERSION = 6 };

// Utility function to tell whether a Function is a vISA kernel.
inline bool isKernel(const Function *F) {
  // We use DLLExport to represent a kernel in LLVM IR.
  return (F->hasDLLExportStorageClass() ||
          F->hasFnAttribute(genx::FunctionMD::CMGenXMain));
}

// Turn a MDNode into llvm::value or its subclass.
// Return nullptr if the underlying value has type mismatch.
template <typename Ty = llvm::Value> Ty *getValueAsMetadata(Metadata *M) {
  if (auto VM = dyn_cast<ValueAsMetadata>(M))
    if (auto V = dyn_cast<Ty>(VM->getValue()))
      return V;
  return nullptr;
}

/// KernelMetadata : class to parse kernel metadata
class KernelMetadata {
  Function *F = nullptr;
  bool IsKernel = false;
  StringRef Name;
  unsigned SLMSize = 0;
  SmallVector<unsigned, 4> ArgKinds;
  SmallVector<unsigned, 4> ArgOffsets;
  SmallVector<unsigned, 4> ArgIOKinds;
  SmallVector<StringRef, 4> ArgTypeDescs;
  // Assign a BTI value to a surface or sampler, OCL path only.
  // Given buffer x,                       --> UAV
  //       read_only image                 --> SRV
  //       write_only or read_write image  --> UAV
  //
  // First assign SRV then UAV resources.
  SmallVector<int32_t, 4> BTIs;

public:
  // default constructor
  KernelMetadata() {}

  /*
   * KernelMetadata constructor
   *
   * Enter:   F = Function that purports to be a CM kernel
   *
   */
  KernelMetadata(Function *F) {
    if (!genx::isKernel(F))
      return;
    NamedMDNode *Named =
        F->getParent()->getNamedMetadata(genx::FunctionMD::GenXKernels);
    if (!Named)
      return;

    MDNode *Node = nullptr;
    for (unsigned i = 0, e = Named->getNumOperands(); i != e; ++i) {
      if (i == e)
        return;
      Node = Named->getOperand(i);
      if (Node->getNumOperands() > KernelMDOp::ArgTypeDescs &&
          getValueAsMetadata(Node->getOperand(KernelMDOp::FunctionRef)) == F)
        break;
    }
    if (!Node)
      return;

    // Node is the metadata node for F, and it has the required number of
    // operands.
    this->F = F;
    IsKernel = true;
    if (MDString *MDS = dyn_cast<MDString>(Node->getOperand(KernelMDOp::Name)))
      Name = MDS->getString();
    if (ConstantInt *Sz = getValueAsMetadata<ConstantInt>(Node->getOperand(KernelMDOp::SLMSize)))
      SLMSize = Sz->getZExtValue();
    // Build the argument kinds and offsets arrays that should correspond to the
    // function arguments (both explicit and implicit)
    MDNode *KindsNode = dyn_cast<MDNode>(Node->getOperand(KernelMDOp::ArgKinds));
    MDNode *OffsetsNode = dyn_cast<MDNode>(Node->getOperand(KernelMDOp::ArgOffsets));
    MDNode *InputOutputKinds = dyn_cast<MDNode>(Node->getOperand(KernelMDOp::ArgIOKinds));
    MDNode *ArgDescNode = dyn_cast<MDNode>(Node->getOperand(KernelMDOp::ArgTypeDescs));

    assert(KindsNode);

    for (unsigned i = 0, e = KindsNode->getNumOperands(); i != e; ++i) {
      ArgKinds.push_back(
          getValueAsMetadata<ConstantInt>(KindsNode->getOperand(i))
              ->getZExtValue());
      if (OffsetsNode == nullptr)
        ArgOffsets.push_back(0);
      else {
        assert(OffsetsNode->getNumOperands() == e && "out of sync");
        ArgOffsets.push_back(
            getValueAsMetadata<ConstantInt>(OffsetsNode->getOperand(i))
                ->getZExtValue());
      }
    }
    assert(InputOutputKinds &&
           KindsNode->getNumOperands() >= InputOutputKinds->getNumOperands());
    for (unsigned i = 0, e = InputOutputKinds->getNumOperands(); i != e; ++i)
      ArgIOKinds.push_back(
          getValueAsMetadata<ConstantInt>(InputOutputKinds->getOperand(i))
              ->getZExtValue());
    assert(ArgDescNode);
    for (unsigned i = 0, e = ArgDescNode->getNumOperands(); i < e; ++i) {
      MDString *MDS = dyn_cast<MDString>(ArgDescNode->getOperand(i));
      assert(MDS);
      ArgTypeDescs.push_back(MDS->getString());
    }
  }
  // Accessors
  bool isKernel() const { return IsKernel; }
  StringRef getName() const { return Name; }
  const Function *getFunction() const { return F; }
  unsigned getSLMSize() const { return SLMSize; }
  ArrayRef<unsigned> getArgKinds() const { return ArgKinds; }
  unsigned getNumArgs() const { return ArgKinds.size(); }
  unsigned getArgKind(unsigned Idx) const { return ArgKinds[Idx]; }
  StringRef getArgTypeDesc(unsigned Idx) const {
    if (Idx >= ArgTypeDescs.size())
      return "";
    return ArgTypeDescs[Idx];
  }

  enum { AK_NORMAL, AK_SAMPLER, AK_SURFACE, AK_VME };
  unsigned getArgCategory(unsigned Idx) const {
    switch (getArgKind(Idx) & 7) {
    case AK_SAMPLER:
      return RegCategory::SAMPLER;
    case AK_SURFACE:
      return RegCategory::SURFACE;
    case AK_VME:
      return RegCategory::VME;
    default:
      return RegCategory::GENERAL;
    }
  }

  // check if an argument is annotated with attribute "buffer_t".
  bool isBufferType(unsigned Idx) const {
    return (getArgTypeDesc(Idx).find_lower("buffer_t") != StringRef::npos &&
      getArgTypeDesc(Idx).find_lower("image1d_buffer_t") == StringRef::npos);
  }

  // check if an argument is annotated with attribute "image{1,2,3}d_t".
  bool isImageType(unsigned Idx) const {
    return getArgTypeDesc(Idx).find_lower("image1d_t") != StringRef::npos ||
           getArgTypeDesc(Idx).find_lower("image2d_t") != StringRef::npos ||
           getArgTypeDesc(Idx).find_lower("image3d_t") != StringRef::npos ||
           getArgTypeDesc(Idx).find_lower("image1d_buffer_t") != StringRef::npos;
  }

  int32_t getBTI(unsigned Index) {
    if (BTIs.empty())
      computeBTIs();
    assert(Index < BTIs.size());
    return BTIs[Index];
  }

  enum {
    // Reserved surface indices start from 253, see GenXCodeGen/GenXVisa.h
    // TODO: consider adding a dependency from GenXCodeGen and extract
    // "252" from there
    K_MaxAvailableBtiIndex = 252
  };
  // Assign BTIs lazily.
  void computeBTIs() {
    unsigned SurfaceID = 0;
    unsigned SamplerID = 0;
    auto Desc = ArgTypeDescs.begin();
    // Assign SRV and samplers.
    for (auto Kind = ArgKinds.begin(); Kind != ArgKinds.end(); ++Kind) {
      BTIs.push_back(-1);
      if (*Kind == AK_SAMPLER)
        BTIs.back() = SamplerID++;
      else if (*Kind == AK_SURFACE) {
        StringRef DescStr = *Desc;
        // By default, an unannotated surface is read_write.
        if (DescStr.find_lower("read_only") != StringRef::npos) {
          BTIs.back() = SurfaceID++;
          if (SurfaceID > K_MaxAvailableBtiIndex) {
            llvm::report_fatal_error("not enough BTI indeces", false);
          }
        }
      }
      ++Desc;
    }
    // Scan again and assign BTI to UAV resources.
    Desc = ArgTypeDescs.begin();
    int Idx = 0;
    for (auto Kind = ArgKinds.begin(); Kind != ArgKinds.end(); ++Kind) {
      if (*Kind == AK_SURFACE && BTIs[Idx] == -1)
        BTIs[Idx] = SurfaceID++;
      // SVM arguments are also assigned an BTI, which is not necessary, but OCL
      // runtime requires it.
      if (*Kind == AK_NORMAL) {
        StringRef DescStr = *Desc;
        if (DescStr.find_lower("svmptr_t") != StringRef::npos) {
          BTIs[Idx] = SurfaceID++;
          if (SurfaceID > K_MaxAvailableBtiIndex) {
            llvm::report_fatal_error("not enough BTI indeces", false);
          }
        }
      }
      // print buffer is also assigned with BTI, which is not necessary, but OCL
      // runtime requires it.
      if (*Kind & KernelMetadata::IMP_OCL_PRINTF_BUFFER) {
        BTIs[Idx] = SurfaceID++;
      }

      if (*Kind & KernelMetadata::IMP_OCL_PRIVATE_BASE)
        BTIs[Idx] = SurfaceID++;
      ++Desc, ++Idx;
    }
  }

  // All the Kinds defined
  // These correspond to the values used in vISA
  // Bits 0-2 represent category (see enum)
  // Bits 7..3 represent the value needed for the runtime to determine what
  //           the implicit argument should be
  //
  // IMP_OCL_LOCAL_ID{X, Y, Z} and IMP_OCL_GLOBAL_OR_LOCAL_SIZE apply to OCL
  // runtime only.
  //
  enum ImpValue : uint32_t {
    IMP_NONE = 0x0,
    IMP_LOCAL_SIZE = 0x1 << 3,
    IMP_GROUP_COUNT = 0x2 << 3,
    IMP_LOCAL_ID = 0x3 << 3,
    IMP_SB_DELTAS = 0x4 << 3,
    IMP_SB_BTI = 0x5 << 3,
    IMP_SB_DEPCNT = 0x6 << 3,
    IMP_OCL_LOCAL_ID_X = 0x7 << 3,
    IMP_OCL_LOCAL_ID_Y = 0x8 << 3,
    IMP_OCL_LOCAL_ID_Z = 0x9 << 3,
    IMP_OCL_GROUP_OR_LOCAL_SIZE = 0xA << 3,
    IMP_OCL_PRINTF_BUFFER = 0xB << 3,
    IMP_OCL_PRIVATE_BASE = 0xC << 3,
    IMP_PSEUDO_INPUT = 0x10 << 3
  };

  enum { SKIP_OFFSET_VAL = -1 };
  // Check if this argument should be omitted as a kernel input.
  bool shouldSkipArg(unsigned Idx) const {
    return static_cast<int32_t>(ArgOffsets[Idx]) == SKIP_OFFSET_VAL;
  }
  unsigned getNumNonSKippingInputs() const {
    unsigned K = 0;
    for (unsigned Val : ArgOffsets)
      K += (static_cast<int32_t>(Val) != SKIP_OFFSET_VAL);
    return K;
  }
  unsigned getArgOffset(unsigned Idx) const { return ArgOffsets[Idx]; }

  enum ArgIOKind {
    IO_Normal = 0,
    IO_INPUT = 1,
    IO_OUTPUT = 2,
    IO_INPUT_OUTPUT = 3
  };
  ArgIOKind getArgInputOutputKind(unsigned Idx) const {
    if (Idx < ArgIOKinds.size())
      return static_cast<ArgIOKind>(ArgIOKinds[Idx] & 0x3);
    return IO_Normal;
  }
  bool isOutputArg(unsigned Idx) const {
    auto Kind = getArgInputOutputKind(Idx);
    return Kind == ArgIOKind::IO_OUTPUT || Kind == ArgIOKind::IO_INPUT_OUTPUT;
  }
};

struct KernelArgInfo {
  uint32_t Kind;
  explicit KernelArgInfo(uint32_t Kind) : Kind(Kind) {}
  bool isNormalCategory() const {
    return (Kind & 0x7) == genx::KernelMetadata::AK_NORMAL;
  }
  bool isLocalIDX() const {
    uint32_t Val = Kind & 0xFFF8;
    return Val == genx::KernelMetadata::IMP_OCL_LOCAL_ID_X;
  }
  bool isLocalIDY() const {
    uint32_t Val = Kind & 0xFFF8;
    return Val == genx::KernelMetadata::IMP_OCL_LOCAL_ID_Y;
  }
  bool isLocalIDZ() const {
    uint32_t Val = Kind & 0xFFF8;
    return Val == genx::KernelMetadata::IMP_OCL_LOCAL_ID_Z;
  }
  bool isGroupOrLocalSize() const {
    uint32_t Val = Kind & 0xFFF8;
    return Val == genx::KernelMetadata::IMP_OCL_GROUP_OR_LOCAL_SIZE;
  }
  bool isLocalIDs() const {
    uint32_t Val = Kind & 0xFFF8;
    return Val == genx::KernelMetadata::IMP_LOCAL_ID;
  }
  bool isLocalSize() const {
    uint32_t Val = Kind & 0xFFF8;
    return Val == genx::KernelMetadata::IMP_LOCAL_SIZE;
  }
  bool isGroupCount() const {
    uint32_t Val = Kind & 0xFFF8;
    return Val == genx::KernelMetadata::IMP_GROUP_COUNT;
  }
  bool isPrintBuffer() const {
    uint32_t Val = Kind & 0xFFF8;
    return Val == genx::KernelMetadata::IMP_OCL_PRINTF_BUFFER;
  }
  bool isPrivateBase() const {
    uint32_t Val = Kind & 0xFFF8;
    return Val == genx::KernelMetadata::IMP_OCL_PRIVATE_BASE;
  }
};

} // namespace genx
} // namespace llvm

#endif
