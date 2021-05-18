/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef GENX_KERNEL_INFO_H
#define GENX_KERNEL_INFO_H

#include "vc/GenXOpts/Utils/InternalMetadata.h"
#include "vc/GenXOpts/Utils/RegCategory.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"
#include "Probe/Assertion.h"

#include <unordered_map>

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

struct ImplicitLinearizationInfo {
  Argument *Arg;
  ConstantInt *Offset;
};
using LinearizedArgInfo = std::vector<ImplicitLinearizationInfo>;
using ArgToImplicitLinearization =
    std::unordered_map<Argument *, LinearizedArgInfo>;

/// KernelMetadata : class to parse and update kernel metadata
class KernelMetadata {
public:
  enum class ArgIOKind {
    Normal = 0,
    Input = 1,
    Output = 2,
    InputOutput = 3,
    Fixed = 4,
  };

private:
  const Function *F = nullptr;
  MDNode *ExternalNode = nullptr;
  MDNode *InternalNode = nullptr;
  bool IsKernel = false;
  StringRef Name;
  unsigned SLMSize = 0;
  SmallVector<unsigned, 4> ArgKinds;
  SmallVector<unsigned, 4> ArgOffsets;
  SmallVector<ArgIOKind, 4> ArgIOKinds;
  SmallVector<StringRef, 4> ArgTypeDescs;
  SmallVector<unsigned, 4> ArgIndexes;
  SmallVector<unsigned, 4> OffsetInArgs;
  // Assign a BTI value to a surface or sampler, OCL path only.
  // Given buffer x,                       --> UAV
  //       read_only image                 --> SRV
  //       write_only or read_write image  --> UAV
  //
  // First assign SRV then UAV resources.
  SmallVector<int32_t, 4> BTIs;
  ArgToImplicitLinearization Linearization;

public:
  // default constructor
  KernelMetadata() {}

  /*
   * KernelMetadata constructor
   *
   * Enter:   F = Function that purports to be a CM kernel
   *
   */
  KernelMetadata(const Function *F);

private:
  void updateArgsMD(const SmallVectorImpl<unsigned> &Values, MDNode *Node,
                    unsigned NodeOpNo) const;

public:
  void updateArgOffsetsMD(SmallVectorImpl<unsigned> &&Offsets);
  void updateArgKindsMD(SmallVectorImpl<unsigned> &&Kinds);
  void updateArgIndexesMD(SmallVectorImpl<unsigned> &&Indexes);
  void updateOffsetInArgsMD(SmallVectorImpl<unsigned> &&Offsets);
  void updateLinearizationMD(ArgToImplicitLinearization &&Lin);

  bool hasArgLinearization(Argument *Arg) const {
    return Linearization.count(Arg);
  }

  // Linearization iterators
  LinearizedArgInfo::const_iterator arg_lin_begin(Argument *Arg) const {
    IGC_ASSERT(hasArgLinearization(Arg));
    const auto &L = Linearization.at(Arg);
    return L.cbegin();
  }
  LinearizedArgInfo::const_iterator arg_lin_end(Argument *Arg) const {
    IGC_ASSERT(hasArgLinearization(Arg));
    const auto &L = Linearization.at(Arg);
    return L.cend();
  }
  using arg_lin_range = iterator_range<LinearizedArgInfo::const_iterator>;
  arg_lin_range arg_lin(Argument *Arg) const {
    return arg_lin_range(arg_lin_begin(Arg), arg_lin_end(Arg));
  }

  // Accessors
  bool isKernel() const { return IsKernel; }
  StringRef getName() const { return Name; }
  const Function *getFunction() const { return F; }
  unsigned getSLMSize() const { return SLMSize; }
  ArrayRef<unsigned> getArgKinds() const { return ArgKinds; }
  ArrayRef<ArgIOKind> getArgIOKinds() const { return ArgIOKinds; }
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
    IGC_ASSERT(Index < BTIs.size());
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

    IGC_ASSERT_MESSAGE(F, "BTI assignment requires a function to process");
    const Module* M = F->getParent();
    // If module does have Debuggable Kernels, then BTI=0 is reserved
    if (M->getNamedMetadata(DebugMD::DebuggableKernels))
      SurfaceID = SamplerID = 1;

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
    size_t Idx = 0;
    for (auto Kind = ArgKinds.begin(); Kind != ArgKinds.end(); ++Kind) {
      IGC_ASSERT(Idx < BTIs.size());
      if (*Kind == AK_SURFACE && BTIs[Idx] == -1) {
        BTIs[Idx] = SurfaceID++;
      }
      // SVM arguments are also assigned an BTI, which is not necessary, but OCL
      // runtime requires it.
      if (*Kind == AK_NORMAL) {
        IGC_ASSERT(Desc != ArgTypeDescs.end());
        StringRef DescStr = *Desc;
        if (DescStr.find_lower("svmptr_t") != StringRef::npos) {
          IGC_ASSERT(Idx < BTIs.size());
          BTIs[Idx] = SurfaceID++;
          if (SurfaceID > K_MaxAvailableBtiIndex) {
            llvm::report_fatal_error("not enough BTI indeces", false);
          }
        }
      }
      // print buffer is also assigned with BTI, which is not necessary, but OCL
      // runtime requires it.
      if (*Kind & KernelMetadata::IMP_OCL_PRINTF_BUFFER) {
        IGC_ASSERT(Idx < BTIs.size());
        BTIs[Idx] = SurfaceID++;
      }

      if (*Kind & KernelMetadata::IMP_OCL_PRIVATE_BASE) {
        IGC_ASSERT(Idx < BTIs.size());
        BTIs[Idx] = SurfaceID++;
      }
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
    IMP_OCL_LINEARIZATION = 0xD << 3,
    IMP_OCL_BYVALSVM = 0xE << 3,
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
  unsigned getOffsetInArg(unsigned Idx) const { return OffsetInArgs[Idx]; }
  unsigned getArgIndex(unsigned Idx) const { return ArgIndexes[Idx]; }

  ArgIOKind getArgInputOutputKind(unsigned Idx) const {
    if (Idx < ArgIOKinds.size())
      return ArgIOKinds[Idx];
    return ArgIOKind::Normal;
  }
  bool isOutputArg(unsigned Idx) const {
    auto Kind = getArgInputOutputKind(Idx);
    return Kind == ArgIOKind::Output || Kind == ArgIOKind::InputOutput;
  }
  bool isFastCompositeArg(unsigned Idx) const {
    return isOutputArg(Idx) || getArgInputOutputKind(Idx) == ArgIOKind::Input;
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
  bool isByValSVM() const {
    uint32_t Val = Kind & 0xFFF8;
    return Val == genx::KernelMetadata::IMP_OCL_BYVALSVM;
  }
};

void replaceFunctionRefMD(const Function &From, Function &To);

} // namespace genx
} // namespace llvm

#endif
