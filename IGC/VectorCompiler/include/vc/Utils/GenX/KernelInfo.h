/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENX_KERNELINFO_H
#define VC_UTILS_GENX_KERNELINFO_H

#include "vc/InternalIntrinsics/InternalIntrinsics.h"
#include "vc/Utils/GenX/InternalMetadata.h"
#include "vc/Utils/GenX/Intrinsics.h"
#include "vc/Utils/GenX/RegCategory.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/ADT/StringRef.h"

#include "llvm/ADT/iterator_range.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"

#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"

#include <cstdint>
#include <optional>
#include <type_traits>
#include <unordered_map>

namespace vc {

enum { VISA_MAJOR_VERSION = 3, VISA_MINOR_VERSION = 6 };

enum { VC_STACK_USAGE_UNKNOWN = -1 };

struct OCLAttributes {
  // Type qualifiers for resources.
  static constexpr auto ReadOnly = "read_only";
  static constexpr auto WriteOnly = "write_only";
  static constexpr auto ReadWrite = "read_write";

  // Buffer surface.
  static constexpr auto Buffer = "buffer_t";
  // SVM pointer to buffer.
  static constexpr auto SVM = "svmptr_t";
  // OpenCL-like types.
  static constexpr auto Sampler = "sampler_t";
  static constexpr auto Image1d = "image1d_t";
  static constexpr auto Image1dArray = "image1d_array_t";
  // Same as 1D image. Seems that there is no difference in runtime.
  static constexpr auto Image1dBuffer = "image1d_buffer_t";
  static constexpr auto Image2d = "image2d_t";
  static constexpr auto Image2dArray = "image2d_array_t";
  static constexpr auto Image2dMediaBlock = "image2d_media_block_t";
  static constexpr auto Image3d = "image3d_t";
};

// Utility function to tell how much stack required
// returns VC_STACK_USAGE_UNKNOWN if no attribute found
inline int getStackAmount(const llvm::Function *F,
                          int Default = VC_STACK_USAGE_UNKNOWN) {
  IGC_ASSERT(F);
  if (!F->hasFnAttribute(FunctionMD::VCStackAmount))
    return Default;
  llvm::StringRef Val =
      F->getFnAttribute(FunctionMD::VCStackAmount).getValueAsString();
  int Result;
  [[maybe_unused]] bool HaveParseError = Val.getAsInteger<int>(10, Result);
  IGC_ASSERT(!HaveParseError);
  return Result;
}

inline bool isBuiltinFunction(const llvm::Function &F) {
  return F.hasFnAttribute(FunctionMD::VCBuiltinFunction);
}

// Utility function to tell whether a Function is a vISA kernel.
inline bool isKernel(const llvm::Function *F) {
  // We use DLLExport to represent a kernel in LLVM IR.
  return (F->hasDLLExportStorageClass() ||
          F->hasFnAttribute(llvm::genx::FunctionMD::CMGenXMain));
}

inline bool isKernel(const llvm::Function &F) { return isKernel(&F); }

// Check if it is a fast composite.
inline bool isCMCallable(const llvm::Function &F) {
  return F.hasFnAttribute(llvm::genx::FunctionMD::CMCallable);
}

// Utility function to tell if a Function needs to be called using
// vISA stack call ABI.
inline bool requiresStackCall(const llvm::Function *F) {
  IGC_ASSERT(F);
  bool IsStackCall = F->hasFnAttribute(llvm::genx::FunctionMD::CMStackCall);
  IGC_ASSERT_MESSAGE(!IsStackCall || !isKernel(F),
                     "The kernel cannot be a stack call");
  return IsStackCall;
}

inline bool requiresStackCall(const llvm::Function &F) {
  return requiresStackCall(&F);
}

// Utility function to tell if a Function must be called indirectly.
inline bool isIndirect(const llvm::Function *F) {
  IGC_ASSERT(F);
// FIXME: Temporary solution until SPIRV translator conversion of unnamed
// structure types is fixed for intrinsics.
  if (llvm::GenXIntrinsic::isAnyNonTrivialIntrinsic(F))
    return false;
  if (vc::InternalIntrinsic::isInternalNonTrivialIntrinsic(F))
    return false;
  if (vc::isKernel(F))
    return false;
  if (vc::isBuiltinFunction(*F))
    return false;
  if (!F->hasAddressTaken() && F->hasLocalLinkage())
    return false;
  if (vc::isCMCallable(*F))
    return false;
  IGC_ASSERT_MESSAGE(
      requiresStackCall(F),
      "The indirectly-called function is expected to be a stack call");
  return true;
}

inline bool isIndirect(const llvm::Function &F) { return isIndirect(&F); }

// Turn a MDNode into llvm::value or its subclass.
// Return nullptr if the underlying value has type mismatch.
template <typename Ty = llvm::Value> Ty *getValueAsMetadata(llvm::Metadata *M) {
  if (auto *VM = llvm::dyn_cast<llvm::ValueAsMetadata>(M))
    if (auto *V = llvm::dyn_cast<Ty>(VM->getValue()))
      return V;
  return nullptr;
}

template <typename Ty = llvm::Value>
Ty *getValueAsMetadata(const llvm::Metadata *M) {
  if (auto *VM = llvm::dyn_cast<llvm::ValueAsMetadata>(M))
    if (auto *V = llvm::dyn_cast<Ty>(VM->getValue()))
      return V;
  return nullptr;
}

// Number of barriers can only be 0, 1, 2, 4, 8, 16, 24, 32.
// Alignment here means choosing nearest overlapping legal number of barriers.
static unsigned alignBarrierCnt(unsigned BarrierCnt) {
  if (BarrierCnt == 0)
    return 0;
  if (BarrierCnt > 16 && BarrierCnt <= 24)
    return 24;
  return llvm::PowerOf2Ceil(BarrierCnt);
}

struct ImplicitLinearizationInfo {
  llvm::Argument *Arg;
  llvm::ConstantInt *Offset;
};
using LinearizedArgInfo = std::vector<ImplicitLinearizationInfo>;
using ArgToImplicitLinearization =
    std::unordered_map<llvm::Argument *, LinearizedArgInfo>;

inline bool isDescBufferType(llvm::StringRef TypeDesc) {
  return IGCLLVM::contains_insensitive(TypeDesc, OCLAttributes::Buffer) &&
         !IGCLLVM::contains_insensitive(TypeDesc, OCLAttributes::Image1dBuffer);
}

inline bool isDescImageType(llvm::StringRef TypeDesc) {
  using namespace IGCLLVM;
  return contains_insensitive(TypeDesc, OCLAttributes::Image1d) ||
         contains_insensitive(TypeDesc, OCLAttributes::Image1dArray) ||
         contains_insensitive(TypeDesc, OCLAttributes::Image1dBuffer) ||
         contains_insensitive(TypeDesc, OCLAttributes::Image2d) ||
         contains_insensitive(TypeDesc, OCLAttributes::Image2dArray) ||
         contains_insensitive(TypeDesc, OCLAttributes::Image2dMediaBlock) ||
         contains_insensitive(TypeDesc, OCLAttributes::Image3d);
}

inline bool isDescSamplerType(llvm::StringRef TypeDesc) {
  return IGCLLVM::contains_insensitive(TypeDesc, OCLAttributes::Sampler);
}

inline bool isDescReadOnly(llvm::StringRef TypeDesc) {
  return IGCLLVM::contains_insensitive(TypeDesc, OCLAttributes::ReadOnly);
}

inline bool isDescSvmPtr(llvm::StringRef TypeDesc) {
  return IGCLLVM::contains_insensitive(TypeDesc, OCLAttributes::SVM);
}

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

  // TODO: Use SPIR-V headers.
  enum class ExecutionMode {
    MaximumRegistersINTEL = 6461,
    MaximumRegistersIdINTEL = 6462,
    NamedMaximumRegistersINTEL = 6463
  };

private:
  const llvm::Function *F = nullptr;
  llvm::MDNode *ExternalNode = nullptr;
  llvm::MDNode *InternalNode = nullptr;
  bool IsKernel = false;
  llvm::StringRef Name;
  unsigned SLMSize = 0;
  unsigned NBarrierCnt = 0;
  unsigned IndirectCount = 0;
  llvm::SmallVector<unsigned, 4> ArgKinds;
  llvm::SmallVector<unsigned, 4> ArgOffsets;
  llvm::SmallVector<ArgIOKind, 4> ArgIOKinds;
  llvm::SmallVector<llvm::StringRef, 4> ArgTypeDescs;
  llvm::SmallVector<unsigned, 4> ArgIndexes;
  llvm::SmallVector<unsigned, 4> OffsetInArgs;
  std::vector<int> BTIs;
  ArgToImplicitLinearization Linearization;

  // Optional SPIR-V execution mode.
  std::optional<unsigned> GRFSize;

public:
  // default constructor
  KernelMetadata() {}

  /*
   * KernelMetadata constructor
   *
   * Enter:   F = Function that purports to be a CM kernel
   *
   */
  KernelMetadata(const llvm::Function *F);

private:
  template <typename InputIt>
  void updateArgsMD(InputIt Begin, InputIt End, llvm::MDNode *Node,
                    unsigned NodeOpNo) const;

public:
  void updateArgOffsetsMD(llvm::SmallVectorImpl<unsigned> &&Offsets);
  void updateArgKindsMD(llvm::SmallVectorImpl<unsigned> &&Kinds);
  void updateArgIndexesMD(llvm::SmallVectorImpl<unsigned> &&Indexes);
  void updateArgTypeDescsMD(llvm::SmallVectorImpl<llvm::StringRef> &&Descs);
  void updateOffsetInArgsMD(llvm::SmallVectorImpl<unsigned> &&Offsets);
  void updateLinearizationMD(ArgToImplicitLinearization &&Lin);
  void updateBTIndicesMD(std::vector<int> &&BTIs);
  void updateSLMSizeMD(unsigned Size);
  void updateIndirectCountMD(unsigned Count);

  void parseExecutionMode(llvm::MDNode *SpirvExecutionMode);

  bool hasArgLinearization(llvm::Argument *Arg) const {
    return Linearization.count(Arg);
  }

  // Linearization iterators
  LinearizedArgInfo::const_iterator arg_lin_begin(llvm::Argument *Arg) const {
    IGC_ASSERT(hasArgLinearization(Arg));
    const auto &L = Linearization.at(Arg);
    return L.cbegin();
  }
  LinearizedArgInfo::const_iterator arg_lin_end(llvm::Argument *Arg) const {
    IGC_ASSERT(hasArgLinearization(Arg));
    const auto &L = Linearization.at(Arg);
    return L.cend();
  }
  using arg_lin_range = llvm::iterator_range<LinearizedArgInfo::const_iterator>;
  arg_lin_range arg_lin(llvm::Argument *Arg) const {
    return arg_lin_range(arg_lin_begin(Arg), arg_lin_end(Arg));
  }

  // Accessors
  bool isKernel() const { return IsKernel; }
  llvm::StringRef getName() const { return Name; }
  const llvm::Function *getFunction() const { return F; }
  unsigned getSLMSize() const { return SLMSize; }
  bool hasNBarrier() const { return NBarrierCnt > 0; }
  // Args:
  //    HasBarrier - whether kernel has barrier or sbarrier instructions
  unsigned getAlignedBarrierCnt(bool HasBarrier) const {
    if (hasNBarrier())
      // Get legal barrier count based on the number of named barriers plus
      // regular barrier
      return alignBarrierCnt(NBarrierCnt + (HasBarrier ? 1 : 0));
    return HasBarrier;
  }
  unsigned getIndirectCount() const { return IndirectCount; }
  llvm::ArrayRef<unsigned> getArgKinds() const { return ArgKinds; }
  llvm::ArrayRef<ArgIOKind> getArgIOKinds() const { return ArgIOKinds; }
  llvm::ArrayRef<llvm::StringRef> getArgTypeDescs() const {
    return ArgTypeDescs;
  }
  unsigned getNumArgs() const { return ArgKinds.size(); }
  unsigned getArgKind(unsigned Idx) const { return ArgKinds[Idx]; }
  llvm::StringRef getArgTypeDesc(unsigned Idx) const {
    if (Idx >= ArgTypeDescs.size())
      return "";
    return ArgTypeDescs[Idx];
  }

  std::optional<unsigned> getGRFSize() const { return GRFSize; }

  enum { AK_NORMAL, AK_SAMPLER, AK_SURFACE };
  RegCategory getArgCategory(unsigned Idx) const {
    switch (getArgKind(Idx) & 7) {
    case AK_SAMPLER:
      return vc::RegCategory::Sampler;
    case AK_SURFACE:
      return vc::RegCategory::Surface;
    default:
      return vc::RegCategory::General;
    }
  }

  // check if an argument is annotated with attribute "buffer_t".
  bool isBufferType(unsigned Idx) const {
    return isDescBufferType(getArgTypeDesc(Idx));
  }

  int32_t getBTI(unsigned Index) const {
    IGC_ASSERT(Index < BTIs.size());
    return BTIs[Index];
  }

  // The number of low bits in argument kind used to store argument category.
  static constexpr int AKBitsForCategory = 3;

  // All the Kinds defined
  // These correspond to the values used in vISA
  // Bits 0-2 represent category (see enum)
  // Bits 7..3 represent the value needed for the runtime to determine what
  //           the implicit argument should be
  //
  enum ImpValue : uint32_t {
    IMP_NONE = 0x0,
    IMP_LOCAL_SIZE = 0x1 << AKBitsForCategory,
    IMP_GROUP_COUNT = 0x2 << AKBitsForCategory,
    IMP_LOCAL_ID = 0x3 << AKBitsForCategory,
    IMP_SB_DELTAS = 0x4 << AKBitsForCategory,
    IMP_SB_BTI = 0x5 << AKBitsForCategory,
    IMP_SB_DEPCNT = 0x6 << AKBitsForCategory,
    IMP_OCL_PRINTF_BUFFER = 0xB << AKBitsForCategory,
    IMP_OCL_PRIVATE_BASE = 0xC << AKBitsForCategory,
    IMP_OCL_LINEARIZATION = 0xD << AKBitsForCategory,
    IMP_OCL_BYVALSVM = 0xE << AKBitsForCategory,
    // Implicit argument with implicit args buffer.
    // It is not supported by CMRT. It is not supported for platforms with
    // payload in memory (for those platforms r0.0 is used to obtain the
    // pointer).
    IMP_IMPL_ARGS_BUFFER = 0xF << AKBitsForCategory,
    IMP_PSEUDO_INPUT = 0x10 << AKBitsForCategory,
    IMP_OCL_ASSERT_BUFFER = 0x11 << AKBitsForCategory,
    IMP_INDIRECT_DATA_BUFFER = 0x12 << AKBitsForCategory,
    IMP_SCRATCH_BUFFER = 0x13 << AKBitsForCategory,
    IMP_OCL_SYNC_BUFFER = 0x17 << AKBitsForCategory,
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

inline bool isImplicitArgKind(uint32_t ArgKind,
                              KernelMetadata::ImpValue RefImplArgID) {
  uint32_t ImplArgID = ArgKind & llvm::maskTrailingZeros<uint32_t>(
                                     KernelMetadata::AKBitsForCategory);
  return ImplArgID == RefImplArgID;
}

inline bool isNormalCategoryArgKind(uint32_t ArgKind) {
  return (ArgKind & llvm::maskTrailingOnes<uint32_t>(
                        KernelMetadata::AKBitsForCategory)) ==
         KernelMetadata::AK_NORMAL;
}

inline bool isLocalIDKind(uint32_t ArgKind) {
  return isImplicitArgKind(ArgKind, KernelMetadata::IMP_LOCAL_ID);
}

inline bool isLocalSizeKind(uint32_t ArgKind) {
  return isImplicitArgKind(ArgKind, KernelMetadata::IMP_LOCAL_SIZE);
}

inline bool isGroupCountKind(uint32_t ArgKind) {
  return isImplicitArgKind(ArgKind, KernelMetadata::IMP_GROUP_COUNT);
}

inline bool isPrintBufferKind(uint32_t ArgKind) {
  return isImplicitArgKind(ArgKind, KernelMetadata::IMP_OCL_PRINTF_BUFFER);
}

inline bool isAssertBufferKind(uint32_t ArgKind) {
  return isImplicitArgKind(ArgKind, KernelMetadata::IMP_OCL_ASSERT_BUFFER);
}

inline bool isPrivateBaseKind(uint32_t ArgKind) {
  return isImplicitArgKind(ArgKind, KernelMetadata::IMP_OCL_PRIVATE_BASE);
}

inline bool isByValSVMKind(uint32_t ArgKind) {
  return isImplicitArgKind(ArgKind, KernelMetadata::IMP_OCL_BYVALSVM);
}

inline bool isImplicitArgsBufferKind(uint32_t ArgKind) {
  return isImplicitArgKind(ArgKind, KernelMetadata::IMP_IMPL_ARGS_BUFFER);
}

inline bool isSyncBufferKind(uint32_t ArgKind) {
  return isImplicitArgKind(ArgKind, KernelMetadata::IMP_OCL_SYNC_BUFFER);
}

inline bool isIndirectDataBufferKind(uint32_t ArgKind) {
  return isImplicitArgKind(ArgKind, KernelMetadata::IMP_INDIRECT_DATA_BUFFER);
}

inline bool isScratchBufferKind(uint32_t ArgKind) {
  return isImplicitArgKind(ArgKind, KernelMetadata::IMP_SCRATCH_BUFFER);
}

// Get implicit argument of the kernel \p Kernel defined by ID \p ImplArgID.
// If kernel has no such argument behavior is undefined.
const llvm::Argument &getImplicitArg(const llvm::Function &Kernel,
                                     KernelMetadata::ImpValue ImplArgID);

inline llvm::Argument &getImplicitArg(llvm::Function &Kernel,
                                      KernelMetadata::ImpValue ImplArgID) {
  return const_cast<llvm::Argument &>(
      getImplicitArg(static_cast<const llvm::Function &>(Kernel), ImplArgID));
}

void replaceFunctionRefMD(const llvm::Function &From, llvm::Function &To);

// Returns whether the provided module \p M has at least one kernel according
// to metadata.
bool hasKernel(const llvm::Module &M);

bool canUseSIMD32(const llvm::Module &M, bool HasFusedEU);

namespace detail {

// FIXME: Cannot support conversion from const iterator to mutable iterator,
//        because llvm::NamedMDNode::const_op_iterator doesn't support it.
//        Support it once LLVM does it too.
template <bool IsConst> class KernelIteratorImpl final {
  using ValueType =
      std::conditional_t<IsConst, const llvm::Function, llvm::Function>;
  using InnerIter =
      std::conditional_t<IsConst, llvm::NamedMDNode::const_op_iterator,
                         llvm::NamedMDNode::op_iterator>;
  using ModuleT = std::conditional_t<IsConst, const llvm::Module, llvm::Module>;
  using NamedMDNodeT =
      std::conditional_t<IsConst, const llvm::NamedMDNode, llvm::NamedMDNode>;

  InnerIter MDNodeIt{};

  KernelIteratorImpl(InnerIter It) : MDNodeIt{It} {}

public:
  using difference_type = typename InnerIter::difference_type;
  using value_type = std::remove_cv_t<ValueType>;
  using pointer = ValueType *;
  using reference = ValueType &;
  using iterator_category = typename InnerIter::iterator_category;

  KernelIteratorImpl() = default;

  static KernelIteratorImpl CreateBegin(ModuleT &M) {
    NamedMDNodeT *KernelsMD =
        M.getNamedMetadata(llvm::genx::FunctionMD::GenXKernels);
    if (!KernelsMD)
      return {};
    return {KernelsMD->op_begin()};
  }

  static KernelIteratorImpl CreateEnd(ModuleT &M) {
    NamedMDNodeT *KernelsMD =
        M.getNamedMetadata(llvm::genx::FunctionMD::GenXKernels);
    if (!KernelsMD)
      return {};
    return {KernelsMD->op_end()};
  }

  KernelIteratorImpl &operator++() {
    ++MDNodeIt;
    return *this;
  }

  KernelIteratorImpl operator++(int) {
    auto Tmp = *this;
    operator++();
    return Tmp;
  }

  KernelIteratorImpl &operator--() {
    --MDNodeIt;
    return *this;
  }

  KernelIteratorImpl operator--(int) {
    auto Tmp = *this;
    operator--();
    return Tmp;
  }

  pointer operator->() {
    // InnerIter returns pointer in operator*.
    auto *KernelNode = *MDNodeIt;
    pointer Kernel = getValueAsMetadata<ValueType>(
        KernelNode->getOperand(llvm::genx::KernelMDOp::FunctionRef));
    IGC_ASSERT_MESSAGE(
        Kernel, "Kernel MD must hold a valid pointer to kernel function");
    return Kernel;
  }

  reference operator*() { return *operator->(); }

  friend bool operator==(const KernelIteratorImpl &LHS,
                         const KernelIteratorImpl &RHS) {
    return LHS.MDNodeIt == RHS.MDNodeIt;
  }

  friend bool operator!=(const KernelIteratorImpl &LHS,
                         const KernelIteratorImpl &RHS) {
    return !(LHS == RHS);
  }
};
} // namespace detail

// Bidirectional iterator to iterate over module kernels based on the info in
// kernel metadata. Iterator dereferencing returns either const or mutable
// reference for llvm::Function depeneding on whether the iterator is const.
// The iterator correctly handles case when there's no metadata in the module.
//
// Usage example:
// for (Function &Kernel : vc::kernels(M))
using KernelIterator = detail::KernelIteratorImpl</* IsConst =*/false>;
using ConstKernelIterator = detail::KernelIteratorImpl</* IsConst =*/true>;

inline KernelIterator kernel_begin(llvm::Module &M) {
  return KernelIterator::CreateBegin(M);
}

inline KernelIterator kernel_end(llvm::Module &M) {
  return KernelIterator::CreateEnd(M);
}

inline ConstKernelIterator kernel_begin(const llvm::Module &M) {
  return ConstKernelIterator::CreateBegin(M);
}

inline ConstKernelIterator kernel_end(const llvm::Module &M) {
  return ConstKernelIterator::CreateEnd(M);
}

using KernelRange = llvm::iterator_range<KernelIterator>;
using ConstKernelRange = llvm::iterator_range<ConstKernelIterator>;

inline KernelRange kernels(llvm::Module &M) {
  return llvm::make_range(kernel_begin(M), kernel_end(M));
}

inline ConstKernelRange kernels(const llvm::Module &M) {
  return llvm::make_range(kernel_begin(M), kernel_end(M));
}

} // namespace vc

#endif // VC_UTILS_GENX_KERNELINFO_H
