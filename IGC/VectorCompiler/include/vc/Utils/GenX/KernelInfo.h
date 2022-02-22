/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENX_KERNELINFO_H
#define VC_UTILS_GENX_KERNELINFO_H

#include "vc/Utils/GenX/InternalMetadata.h"
#include "vc/Utils/GenX/RegCategory.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/ADT/StringRef.h"

#include "llvm/ADT/iterator_range.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"

#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"

#include <type_traits>
#include <unordered_map>

namespace vc {

enum { VISA_MAJOR_VERSION = 3, VISA_MINOR_VERSION = 6 };

enum { VC_STACK_USAGE_UNKNOWN = -1 };

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
  bool HaveParseError = Val.getAsInteger<int>(10, Result);
  IGC_ASSERT(!HaveParseError);
  return Result;
}

// Check if a function is to emulate instructions.
inline bool isEmulationFunction(const llvm::Function &F) {
  return F.hasFnAttribute(FunctionMD::VCEmulationRoutine);
}

// Utility function to tell whether a Function is a vISA kernel.
inline bool isKernel(const llvm::Function *F) {
  // We use DLLExport to represent a kernel in LLVM IR.
  return (F->hasDLLExportStorageClass() ||
          F->hasFnAttribute(llvm::genx::FunctionMD::CMGenXMain));
}

inline bool isKernel(const llvm::Function &F) { return isKernel(&F); }

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
  // FIXME: The condition of which function is considered to be indirectly
  // called will be changed soon.
  bool IsIndirect = F->hasAddressTaken();
  IGC_ASSERT_MESSAGE(
      !IsIndirect || requiresStackCall(F),
      "The indirectly-called function is expected to be a stack call");
  return IsIndirect;
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

// Number of barriers can only be 0, 1, 2, 4, 8, 16, 24, 32.
// Alignment here means choosing nearest overlapping legal number of barriers.
static unsigned alignBarrierCnt(unsigned BarrierCnt) {
  if (BarrierCnt == 0)
    return 0;
  if (BarrierCnt > 32) {
    llvm::report_fatal_error("named barrier count must not exceed 32");
    return 0;
  }
  if (BarrierCnt > 16 && BarrierCnt <= 24)
    return 24;
  if (llvm::isPowerOf2_32(BarrierCnt))
    return BarrierCnt;
  return llvm::NextPowerOf2(BarrierCnt);
}

struct ImplicitLinearizationInfo {
  llvm::Argument *Arg;
  llvm::ConstantInt *Offset;
};
using LinearizedArgInfo = std::vector<ImplicitLinearizationInfo>;
using ArgToImplicitLinearization =
    std::unordered_map<llvm::Argument *, LinearizedArgInfo>;

inline bool isDescBufferType(llvm::StringRef TypeDesc) {
  return (IGCLLVM::contains_insensitive(TypeDesc, "buffer_t") &&
          !IGCLLVM::contains_insensitive(TypeDesc, "image1d_buffer_t"));
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

private:
  const llvm::Function *F = nullptr;
  llvm::MDNode *ExternalNode = nullptr;
  llvm::MDNode *InternalNode = nullptr;
  bool IsKernel = false;
  llvm::StringRef Name;
  unsigned SLMSize = 0;
  unsigned NBarrierCnt = 0;
  llvm::SmallVector<unsigned, 4> ArgKinds;
  llvm::SmallVector<unsigned, 4> ArgOffsets;
  llvm::SmallVector<ArgIOKind, 4> ArgIOKinds;
  llvm::SmallVector<llvm::StringRef, 4> ArgTypeDescs;
  llvm::SmallVector<unsigned, 4> ArgIndexes;
  llvm::SmallVector<unsigned, 4> OffsetInArgs;
  std::vector<int> BTIs;
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
  KernelMetadata(const llvm::Function *F);

private:
  template <typename InputIt>
  void updateArgsMD(InputIt Begin, InputIt End, llvm::MDNode *Node,
                    unsigned NodeOpNo) const;

public:
  void updateArgOffsetsMD(llvm::SmallVectorImpl<unsigned> &&Offsets);
  void updateArgKindsMD(llvm::SmallVectorImpl<unsigned> &&Kinds);
  void updateArgIndexesMD(llvm::SmallVectorImpl<unsigned> &&Indexes);
  void updateOffsetInArgsMD(llvm::SmallVectorImpl<unsigned> &&Offsets);
  void updateLinearizationMD(ArgToImplicitLinearization &&Lin);
  void updateBTIndicesMD(std::vector<int> &&BTIs);

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

  // All the Kinds defined
  // These correspond to the values used in vISA
  // Bits 0-2 represent category (see enum)
  // Bits 7..3 represent the value needed for the runtime to determine what
  //           the implicit argument should be
  //
  enum ImpValue : uint32_t {
    IMP_NONE = 0x0,
    IMP_LOCAL_SIZE = 0x1 << 3,
    IMP_GROUP_COUNT = 0x2 << 3,
    IMP_LOCAL_ID = 0x3 << 3,
    IMP_SB_DELTAS = 0x4 << 3,
    IMP_SB_BTI = 0x5 << 3,
    IMP_SB_DEPCNT = 0x6 << 3,
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
    return (Kind & 0x7) == KernelMetadata::AK_NORMAL;
  }
  bool isLocalIDs() const {
    uint32_t Val = Kind & 0xFFF8;
    return Val == KernelMetadata::IMP_LOCAL_ID;
  }
  bool isLocalSize() const {
    uint32_t Val = Kind & 0xFFF8;
    return Val == KernelMetadata::IMP_LOCAL_SIZE;
  }
  bool isGroupCount() const {
    uint32_t Val = Kind & 0xFFF8;
    return Val == KernelMetadata::IMP_GROUP_COUNT;
  }
  bool isPrintBuffer() const {
    uint32_t Val = Kind & 0xFFF8;
    return Val == KernelMetadata::IMP_OCL_PRINTF_BUFFER;
  }
  bool isPrivateBase() const {
    uint32_t Val = Kind & 0xFFF8;
    return Val == KernelMetadata::IMP_OCL_PRIVATE_BASE;
  }
  bool isByValSVM() const {
    uint32_t Val = Kind & 0xFFF8;
    return Val == KernelMetadata::IMP_OCL_BYVALSVM;
  }
};

void replaceFunctionRefMD(const llvm::Function &From, llvm::Function &To);

// Returns whether the provided module \p M has at least one kernel according
// to metadata.
bool hasKernel(const llvm::Module &M);

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
