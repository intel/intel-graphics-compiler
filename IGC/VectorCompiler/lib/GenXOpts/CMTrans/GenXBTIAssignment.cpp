/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
/// GenXAssignBTI
/// -----------------
///
/// This pass calculates BT indices for kernel memory object arguments
/// that include buffers and images.
///
/// Calculated BTI are then used instead of corresponging kernel arguments
/// throughout the code. Additionally, all assigned values are saved to
/// kernel metadata to be retrieved later by runtime info pass.
///
//===----------------------------------------------------------------------===//

#include "vc/GenXOpts/GenXOpts.h"
#include "vc/Support/BackendConfig.h"
#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/KernelInfo.h"
#include "vc/Utils/General/FunctionAttrs.h"
#include "vc/Utils/General/Types.h"

#include "llvm/GenXIntrinsics/GenXIntrinsics.h"

#include "Probe/Assertion.h"

#include "llvmWrapper/ADT/StringRef.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Function.h"
#include "llvmWrapper/IR/Instructions.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>

#include <tuple>
#include <utility>
#include <vector>

using namespace llvm;
static cl::opt<bool> Efficient64bOpt(
    "vc-bti-assignment-efficient64b", cl::init(false), cl::Hidden,
    cl::desc("Should be used only in llvm opt to enable 64-bit addressing"));

static cl::opt<bool> EnforceBTIZeroReservation(
    "vc-reserve-bti-zero", cl::init(false), cl::Hidden,
    cl::desc("do not assign BTI index to zero (for testing purposes)"));

namespace {
class BTIAssignment final {
  Module &M;
  const bool emitDebuggableKernels;
  const bool useBindlessBuffers;
  const bool useBindlessImages;
  const bool useBindlessSamplers;

public:
  BTIAssignment(Module &InM, bool InEmitDebuggableKernels,
                bool InUseBindlessBuffers, bool InUseBindlessImages,
                bool InUseBindlessSamplers)
      : M(InM), emitDebuggableKernels(InEmitDebuggableKernels),
        useBindlessBuffers(InUseBindlessBuffers),
        useBindlessImages(InUseBindlessImages),
        useBindlessSamplers(InUseBindlessSamplers) {}

  bool run();

private:
  // Helper function to assign bti from corresponding category.
  // ZipTy -- tuple of ID, ArgKind and ArgDesc.
  // assignSRV return value -- current state of IDs for surface and
  // sampler assignment.
  template <typename ZipTy>
  std::pair<int, int> assignSRV(int SurfaceID, int SamplerID, ZipTy &&Zippy);
  template <typename ZipTy> int assignUAV(int SurfaceID, ZipTy &&Zippy);

  std::vector<int>
  computeBTIndices(vc::KernelMetadata &KM,
                   const std::vector<StringRef> &ExtendedArgDescs);
  bool rewriteArguments(vc::KernelMetadata &KM, Function &F,
                        const std::vector<int> &BTIndices,
                        const std::vector<StringRef> &ExtendedArgDescs);

#if LLVM_VERSION_MAJOR >= 16
  // Returns the replacement kernel if \p F had to be rebuilt, nullptr
  // otherwise.
  Function *legalizeKernelResourceArgTypes(Function &F);
#endif // LLVM_VERSION_MAJOR

  bool processKernel(Function &F);
};

class GenXBTIAssignment final : public ModulePass {
  bool Efficient64b = true;

#if LLVM_VERSION_MAJOR >= 16
  GenXBackendConfigPass::Result &BC;
#endif

public:
  static char ID;

#if LLVM_VERSION_MAJOR >= 16
  GenXBTIAssignment(GenXBackendConfigPass::Result &BC,
                    bool Efficient64b = false)
      : BC(BC), ModulePass(ID), Efficient64b(Efficient64b || Efficient64bOpt) {}
#else  // LLVM_VERSION_MAJOR >= 16
  GenXBTIAssignment(bool Efficient64b = false)
      : ModulePass(ID), Efficient64b(Efficient64b || Efficient64bOpt) {}
#endif // LLVM_VERSION_MAJOR >= 16

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<GenXBackendConfig>();
  }

  StringRef getPassName() const override { return "GenX BTI Assignment"; }

  bool runOnModule(Module &M) override;
};
} // namespace

char GenXBTIAssignment::ID = 0;

INITIALIZE_PASS_BEGIN(GenXBTIAssignment, "GenXBTIAssignment",
                      "GenXBTIAssignment", false, false);
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(GenXBTIAssignment, "GenXBTIAssignment", "GenXBTIAssignment",
                    false, false);

#if LLVM_VERSION_MAJOR < 16
namespace llvm {
ModulePass *createGenXBTIAssignmentPass(bool Efficient64b) {
  initializeGenXBTIAssignmentPass(*PassRegistry::getPassRegistry());
  return new GenXBTIAssignment(Efficient64b);
}
} // namespace llvm
#else // LLVM_VERSION_MAJOR < 16
PreservedAnalyses
GenXBTIAssignmentPass::run(llvm::Module &M,
                           llvm::AnalysisManager<llvm::Module> &AM) {
  GenXBTIAssignment GenXBTI(BC, Efficient64b);
  if (GenXBTI.runOnModule(M))
    return PreservedAnalyses::none();
  return PreservedAnalyses::all();
}
#endif

bool GenXBTIAssignment::runOnModule(Module &M) {
#if LLVM_VERSION_MAJOR < 16
  auto &BC = getAnalysis<GenXBackendConfig>();
#endif
  bool emitDebuggableKernels = BC.emitDebuggableKernelsForLegacyPath();
  bool useBindlessBuffers = BC.useBindlessBuffers();
  bool useBindlessImages = BC.useBindlessImages();
  bool useBindlessSamplers = BC.useBindlessImages();

  useBindlessBuffers |= Efficient64b;
  useBindlessImages |= Efficient64b;
  useBindlessSamplers |= Efficient64b;

  BTIAssignment BA(M, emitDebuggableKernels, useBindlessBuffers,
                   useBindlessImages, useBindlessSamplers);

  return BA.run();
}

// Surfaces starting from 240 are reserved.
static constexpr int MaxAvailableSurfaceIndex = 239;
static constexpr int StatelessBti = 255;
static constexpr int MaxAvailableSamplerIndex = 14;

template <typename ZipTy>
std::pair<int, int> BTIAssignment::assignSRV(int SurfaceID, int SamplerID,
                                             ZipTy &&Zippy) {
  // SRV (read only) and samplers.
  for (auto &&[Idx, Kind, Desc] : Zippy) {
    if (Kind == vc::KernelMetadata::AK_SAMPLER) {
      Idx = useBindlessSamplers ? StatelessBti : SamplerID++;
      continue;
    }
    if (Kind == vc::KernelMetadata::AK_SURFACE && vc::isDescReadOnly(Desc)) {
      IGC_ASSERT_MESSAGE(vc::isDescImageType(Desc),
                         "RW qualifiers are allowed on images only");
      Idx = useBindlessImages ? StatelessBti : SurfaceID++;
      continue;
    }
  }
  return {SurfaceID, SamplerID};
}

template <typename ZipTy>
int BTIAssignment::assignUAV(int SurfaceID, ZipTy &&Zippy) {
  // UAV -- writable entities.
  for (auto &&[Idx, Kind, Desc] : Zippy) {
    // Already assigned entities should be skipped.
    if (Idx != -1)
      continue;

    if (Kind == vc::KernelMetadata::AK_SURFACE) {
      if (vc::isDescBufferType(Desc) && useBindlessBuffers)
        Idx = StatelessBti;
      else if (vc::isDescImageType(Desc) && useBindlessImages)
        Idx = StatelessBti;
      else
        Idx = SurfaceID++;
      continue;
    }
    if (Kind == vc::KernelMetadata::AK_NORMAL && vc::isDescSvmPtr(Desc)) {
      Idx = StatelessBti;
      continue;
    }
    // These 'ands' are definitely super buggy. Kinds should be
    // matched with masking and comparision (as in KernelArgInfo).
    // Anding will match more kinds that supposed. Here, for example,
    // SB_BTI is matches too that makes some tests magically work
    // on L0 runtime.
    // FIXME(aus): investigate the reason and rewrite with KAI.
    if (Kind & vc::KernelMetadata::IMP_OCL_PRINTF_BUFFER) {
      Idx = StatelessBti;
      continue;
    }
    if (Kind & vc::KernelMetadata::IMP_OCL_PRIVATE_BASE) {
      Idx = StatelessBti;
      continue;
    }
    if (Kind & vc::KernelMetadata::IMP_OCL_ASSERT_BUFFER) {
      Idx = StatelessBti;
      continue;
    }
    if (Kind & vc::KernelMetadata::IMP_OCL_SYNC_BUFFER) {
      Idx = StatelessBti;
      continue;
    }
  }
  return SurfaceID;
}

// Assign a BTI value to a surface or sampler, NEO path only.
// SRV and UAV is sort of direct3d terminology, though they
// are used across binary format structures.
// SRV -- constant resources -- samplers and read only images.
// UAV -- writeable resources -- buffers and rw/wo images.
// Additionally, ranges for SRV and UAV should be separate and contiguous
// so this code assigns SRV and then UAV resources.
std::vector<int> BTIAssignment::computeBTIndices(
    vc::KernelMetadata &KM, const std::vector<StringRef> &ExtendedArgDescs) {
  int SurfaceID = 0;
  int SamplerID = 0;

  if (emitDebuggableKernels || EnforceBTIZeroReservation) {
    // NOTE: at the current moment we don't use BTI=0, since it is reserved
    // for kernel debugging purposes (SIP uses BTI=0 in order to handle
    // breakpoints).
    SurfaceID = 1;
  }

  std::vector<int> Indices(KM.getArgKinds().size(), -1);

  ArrayRef<unsigned> ArgKinds = KM.getArgKinds();
  auto Zippy = llvm::zip(Indices, KM.getArgKinds(), ExtendedArgDescs);
  std::tie(SurfaceID, SamplerID) = assignSRV(SurfaceID, SamplerID, Zippy);
  SurfaceID = assignUAV(SurfaceID, Zippy);

  auto &Ctx = KM.getFunction()->getContext();
  if (SurfaceID > MaxAvailableSurfaceIndex)
    vc::diagnose(Ctx, "BTIAssignment", "not enough surface indices");
  if (SamplerID > MaxAvailableSamplerIndex)
    vc::diagnose(Ctx, "BTIAssignment", "not enough sampler indices");

  return Indices;
}

bool BTIAssignment::rewriteArguments(
    vc::KernelMetadata &KM, Function &F, const std::vector<int> &BTIndices,
    const std::vector<StringRef> &ExtendedArgDescs) {
  bool Changed = false;

  auto *I32Ty = Type::getInt32Ty(M.getContext());

  IRBuilder<> IRB{&F.front().front()};

  auto ArgKinds = KM.getArgKinds();
  IGC_ASSERT_MESSAGE(ArgKinds.size() == F.arg_size(),
                     "Inconsistent arg kinds metadata");
  for (auto &&[Arg, Kind, BTI, Desc] :
       llvm::zip(F.args(), ArgKinds, BTIndices, ExtendedArgDescs)) {
    if (Kind != vc::KernelMetadata::AK_SAMPLER &&
        Kind != vc::KernelMetadata::AK_SURFACE)
      continue;

    // For bindless buffer resource argument is ExBSO.
    if (useBindlessBuffers && vc::isDescBufferType(Desc))
      continue;

    // For bindless image resource argument is ExBSO.
    if (useBindlessImages && vc::isDescImageType(Desc))
      continue;

    // For bindless sampler resource argument is ExBSO.
    if (useBindlessSamplers && vc::isDescSamplerType(Desc))
      continue;

    IGC_ASSERT_MESSAGE(BTI >= 0, "unassigned BTI");
    Value *BTIConstant = ConstantInt::get(I32Ty, BTI);

    Type *ArgTy = Arg.getType();
    // This code is to handle DPC++ contexts with correct OCL types.
    // We either materialize the constants in-place of args for some
    // instructions (known cases where we do inttoptr followed by ptrtoint) or
    // we just do inttoptr and replace the arg with a new value (default option,
    // later passes clean up the code).
    // FIXME(aus): proper unification of incoming IR is
    // required. Current approach will constantly blow all passes
    // where some additional case should be handled.

    // Step 1: Directly replace if Arg is an integer is of integer type already.
    if (ArgTy->isIntegerTy(32)) {
      IGC_ASSERT_MESSAGE(
          ArgTy == BTIConstant->getType(),
          "Only explicit i32 indices or opaque types are allowed "
          "as bti argument");
      Arg.replaceAllUsesWith(BTIConstant);
      Changed = true;
      continue;
    }

    DenseMap<Type *, Value *> PtrConstCache;
    auto getPtrConst = [&](Type *PTy) -> Value * {
      auto It = PtrConstCache.find(PTy);
      if (It != PtrConstCache.end())
        return It->second;
      Value *V = IRB.CreateIntToPtr(BTIConstant, PTy, ".bti.cast");
      PtrConstCache[PTy] = V;
      return V;
    };

    // Step 2: Otherwise, traverse the chain to find the end constant users.
    SmallVector<Instruction *, 8> WorkList;
    for (Use &U : llvm::make_early_inc_range(Arg.uses())) {
      if (auto *I = dyn_cast<Instruction>(U.getUser()))
        WorkList.push_back(I);
    }

    while (!WorkList.empty()) {
      Instruction *I = WorkList.pop_back_val();

      StoreInst *SI = dyn_cast<StoreInst>(I);
      if (SI && SI->getValueOperand() == &Arg) {
        SI->setOperand(0, BTIConstant);
        Changed = true;
        continue;
      }

#if LLVM_VERSION_MAJOR >= 16
      // This is a workaround due to a bug in Khronos SPIR-V/LLVM Translator. In
      // some cases SPIR-V Writer emits the following sequences:
      // clang-format off
      //
      // TypeImage 104 89 1 0 0 0 0 0 0
      // FunctionParameter 104 350
      // ConvertPtrToU 9 372 350
      //
      // clang-format on
      // The last instruction is illegally trying to use an image type as a
      // source in ConvertPtrToU. Unfortunately, the bug has has not been
      // discovered until the switch to opaque pointers and TargetExtTy. With
      // typed pointers, given that images were represented using pointers to
      // opaque structs, the sequences were "legalized" in LLVM IR. With opaque
      // pointers this leads to an exception in the SPIR-V Reader since it is
      // illegal to use TargetExtTy as a source in LLVM's ptrtoint instruction.
      // As a workaround SPIR-V Reader is emitting builtin calls to
      // "__spirv_ConvertPtrToU" which can be replaced with proper ptrtoint
      // instructions in LLVM IR after retyping TargetExtTy to opaque pointers.
      CallInst *CI = dyn_cast<CallInst>(I);
      if (CI && CI->getOperand(0)->getType()->isTargetExtTy() &&
          CI->getCalledOperand()->getName().contains("__spirv_ConvertPtrToU")) {
        IGC_ASSERT_MESSAGE(CI->getType()->isIntegerTy(32),
                           "__spirv_ConvertPtrToU is expected to return i32!");
        CI->replaceAllUsesWith(BTIConstant);
        CI->eraseFromParent();
        Changed = true;
        continue;
      }
#endif
    }

    // Step 3: Fallback, if after targeted rewrites the argument still has uses,
    // provide a pointer constant. This approach does not work with opaque
    // pointers and TargetExtTy.
    if (!Arg.use_empty()) {
      Value *PtrConst = getPtrConst(ArgTy);
      Arg.replaceAllUsesWith(PtrConst);
      Changed = true;
    }
  }

  return Changed;
}

// Get arg type descs that are extended to arg kinds size.
static std::vector<StringRef> getExtendedArgDescs(vc::KernelMetadata &KM) {
  ArrayRef<unsigned> ArgKinds = KM.getArgKinds();
  ArrayRef<StringRef> ArgTypeDescs = KM.getArgTypeDescs();
  // ArgDescs can be lesser if there are implicit parameters.
  IGC_ASSERT_MESSAGE(
      ArgKinds.size() >= ArgTypeDescs.size(),
      "Expected same or less number of arguments for kinds and descs");

  // All arguments without arg type desc will get default empty description.
  std::vector<StringRef> ArgTypeDescsExt{ArgTypeDescs.begin(),
                                         ArgTypeDescs.end()};
  ArgTypeDescsExt.resize(ArgKinds.size());
  return ArgTypeDescsExt;
}

bool BTIAssignment::processKernel(Function &F) {
  Function *Kernel = &F;
  bool Changed = false;

#if LLVM_VERSION_MAJOR >= 16
  // Has to run before the metadata below is read: it may replace the kernel,
  // and KernelMetadata caches the function it was built from.
  if (Function *NewKernel = legalizeKernelResourceArgTypes(F)) {
    Kernel = NewKernel;
    Changed = true;
  }
#endif // LLVM_VERSION_MAJOR

  vc::KernelMetadata KM{Kernel};

  std::vector<StringRef> ExtArgDescs = getExtendedArgDescs(KM);

  std::vector<int> BTIndices = computeBTIndices(KM, ExtArgDescs);

  Changed |= rewriteArguments(KM, *Kernel, BTIndices, ExtArgDescs);

  KM.updateBTIndicesMD(std::move(BTIndices));

  return Changed;
}

#if LLVM_VERSION_MAJOR >= 16
// Convert SPIR-V target-extension resource arguments (images, buffers,
// samplers) to pointer types and replace "__spirv_ConvertPtrToU" calls with
// real ptrtoint instructions so later passes can handle them. This is only
// needed for bindless resources. On the BTI path rewriteArguments replaces the
// conversion with the assigned index.
Function *BTIAssignment::legalizeKernelResourceArgTypes(Function &F) {
  auto &Ctx = M.getContext();
  vc::KernelMetadata KM{&F};

  auto ArgKinds = KM.getArgKinds();
  if (ArgKinds.size() != F.arg_size())
    return nullptr;

  if (!F.use_empty())
    return nullptr;

  std::vector<StringRef> ExtArgDescs = getExtendedArgDescs(KM);

  auto NeedsPointerType = [this](const Argument &Arg, unsigned Kind,
                                 StringRef Desc) {
    if (Kind != vc::KernelMetadata::AK_SURFACE &&
        Kind != vc::KernelMetadata::AK_SAMPLER)
      return false;
    if (!Arg.getType()->isTargetExtTy())
      return false;
    // Mirrors the bindless bail-outs in rewriteArguments.
    return (useBindlessBuffers && vc::isDescBufferType(Desc)) ||
           (useBindlessImages && vc::isDescImageType(Desc)) ||
           (useBindlessSamplers && vc::isDescSamplerType(Desc));
  };

  // Use the address spaces the typed pointer representation produced for these
  // resources: images and buffer surfaces are global, samplers are constant.
  auto getResourcePtrTy = [&Ctx](unsigned Kind) {
    const unsigned AS = Kind == vc::KernelMetadata::AK_SAMPLER
                            ? vc::AddrSpace::Constant
                            : vc::AddrSpace::Global;
    return IGCLLVM::PointerType::get(Ctx, AS);
  };

  auto IsConvertPtrToUCall = [](const User *U, const Argument &Arg) {
    const auto *CI = dyn_cast<CallInst>(U);
    if (!CI || IGCLLVM::getNumArgOperands(CI) != 1 ||
        CI->getArgOperand(0) != &Arg)
      return false;
    const auto *Callee = CI->getCalledFunction();
    return Callee && Callee->getName().contains("__spirv_ConvertPtrToU");
  };

  SmallVector<Type *, 8> NewArgTys;
  bool NeedsRetyping = false;
  for (auto &&[ArgRef, Kind, Desc] :
       llvm::zip(F.args(), ArgKinds, ExtArgDescs)) {
    Argument &Arg = ArgRef;
    if (!NeedsPointerType(Arg, Kind, Desc)) {
      NewArgTys.push_back(Arg.getType());
      continue;
    }
    if (!llvm::all_of(Arg.users(), [&](const User *U) {
          return IsConvertPtrToUCall(U, Arg);
        })) {
      vc::diagnose(Ctx, "BTIAssignment",
                   "unsupported use of a target extension type resource "
                   "argument in kernel '" +
                       F.getName() + "'");
      return nullptr;
    }
    NewArgTys.push_back(getResourcePtrTy(Kind));
    NeedsRetyping = true;
  }

  if (!NeedsRetyping)
    return nullptr;

  // Only argument types change, so every piece of kernel metadata that is
  // indexed by argument position (kinds, descs, offsets) stays valid.
  auto *NFTy = FunctionType::get(F.getReturnType(), NewArgTys, F.isVarArg());
  auto *NF = Function::Create(NFTy, F.getLinkage(), F.getName());
  vc::transferNameAndCCWithNewAttr(F.getAttributes(), F, *NF);
  // Not covered by transferNameAndCCWithNewAttr. The DLL storage class in
  // particular is load bearing: vc::isKernel() recognizes a kernel by
  // DLLExport, so dropping it would hide the kernel from every later pass.
  NF->setDLLStorageClass(F.getDLLStorageClass());
  NF->setVisibility(F.getVisibility());
  M.getFunctionList().insert(F.getIterator(), NF);
  vc::transferDISubprogram(F, *NF);
  IGCLLVM::splice(NF, NF->begin(), &F);

  for (auto &&[OldArg, NewArg] : llvm::zip(F.args(), NF->args())) {
    NewArg.takeName(&OldArg);
    if (OldArg.getType() == NewArg.getType()) {
      OldArg.replaceAllUsesWith(&NewArg);
      continue;
    }
    // All uses were checked above to be __spirv_ConvertPtrToU calls, which the
    // SPIR-V reader emits because ptrtoint is illegal on a target extension
    // type. Now that the argument is a pointer, use a real ptrtoint.
    for (User *U : llvm::make_early_inc_range(OldArg.users())) {
      auto *CI = cast<CallInst>(U);
      IRBuilder<> IRB{CI};
      auto *Cast = IRB.CreatePtrToInt(&NewArg, CI->getType(), CI->getName());
      CI->replaceAllUsesWith(Cast);
      CI->eraseFromParent();
    }
  }

  vc::replaceFunctionRefMD(F, *NF);
  F.eraseFromParent();

  return NF;
}

#endif // LLVM_VERSION_MAJOR

bool BTIAssignment::run() {
  SmallVector<Function *, 4> Kernels;
  for (Function &Kernel : vc::kernels(M))
    Kernels.push_back(&Kernel);

  bool Changed = false;
  for (Function *Kernel : Kernels)
    Changed |= processKernel(*Kernel);

  return Changed;
}
