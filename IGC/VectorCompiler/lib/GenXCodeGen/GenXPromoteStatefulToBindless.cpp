/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXPromoteStatefulToBindless
/// -----------------------------
///
/// This pass promotes stateful memory accesses to bindless ones.
/// To do this, first, argument kinds are converted to GENERAL category to
/// denote that current input is bindless surface state offset, not binding
/// table index. Then for each memory intrinsics of required kind, there is
/// performed simple conversion. The following form is transformed to write
/// to %bss variable (or T252 as in visa spec) of input argument (filling SSO
/// -- surface state offset) and then use of new %bss variable by special form
/// of intrinsic that takes predefined surface variable as memory handle.
/// Example of dataport intrinsic:
///
///   call void @llvm.genx.oword.st.v8i32(i32 %buf, i32 %addr, <8 x i32> %src)
///
/// This is transformed to the following IR:
///
///   call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.genx.predefined.bss, i32 %buf)
///   call void @llvm.genx.oword.st.predef.surface.p0i32.v8i32(i32* @llvm.genx.predefined.bss, i32 %addr, <8 x i32> %src)
///
/// Pass is intended to be as simple as possible to utilize benefits from
/// previous passes like lowering and legalization. This is sort of
/// pseudo-expansion in presense of option for bindless mode. Behavior of
/// this pass is controlled by options for bindless memory objects.
///
/// Currently, only support for buffers is implemented. However, images and
/// samplers can be easily added here.

#include "GenX.h"
#include "GenXTargetMachine.h"

#include "vc/Support/BackendConfig.h"
#include "vc/Utils/GenX/Intrinsics.h"
#include "vc/Utils/GenX/KernelInfo.h"
#include "vc/Utils/GenX/PredefinedVariable.h"

#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"

#include "Probe/Assertion.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/ErrorHandling.h>

#include <sstream>
#include <vector>

using namespace llvm;

namespace {
class PromoteToBindless {
  Module &M;
  const GenXBackendConfig &BC;
  GlobalVariable *BSS = nullptr;

public:
  PromoteToBindless(Module &InM, const GenXBackendConfig &InBC)
      : M{InM}, BC{InBC} {}

  bool run();

private:
  Optional<unsigned> tryConvertBuffer(unsigned Kind, StringRef Desc);
  unsigned convertSingleArg(unsigned Kind, StringRef Desc);
  bool convertKernelArguments(Function &F);
  bool convertArguments();

  GlobalVariable &getOrCreateBSSVariable();

  CallInst *createBindlessSurfaceDataportIntrinsicChain(CallInst &CI);
  void rewriteBufferIntrinsic(CallInst &CI);
  bool rewriteBufferIntrinsics();
  bool rewriteIntrinsics();
};

class GenXPromoteStatefulToBindless final : public ModulePass {
public:
  static char ID;

  GenXPromoteStatefulToBindless() : ModulePass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<GenXBackendConfig>();
  }

  StringRef getPassName() const override {
    return "GenX Promote Stateful to Bindless";
  }

  bool runOnModule(Module &M) override;
};
} // namespace

char GenXPromoteStatefulToBindless::ID = 0;

INITIALIZE_PASS_BEGIN(GenXPromoteStatefulToBindless,
                      "GenXPromoteStatefulToBindless",
                      "GenXPromoteStatefulToBindless", false, false);
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(GenXPromoteStatefulToBindless,
                    "GenXPromoteStatefulToBindless",
                    "GenXPromoteStatefulToBindless", false, false);

namespace llvm {
ModulePass *createGenXPromoteStatefulToBindlessPass() {
  initializeGenXPromoteStatefulToBindlessPass(*PassRegistry::getPassRegistry());
  return new GenXPromoteStatefulToBindless();
}
} // namespace llvm

bool GenXPromoteStatefulToBindless::runOnModule(Module &M) {
  auto &BC = getAnalysis<GenXBackendConfig>();

  PromoteToBindless PTM{M, BC};

  return PTM.run();
}

// Error reporting function for not yet supported functionality.
// When every intrinsic will be supported there will be no need for this.
// Currently, simply fail on unsupported ones.
static void reportUnhandledIntrinsic(const char *Func, GenXIntrinsic::ID Id) {
  std::ostringstream SS;
  SS << "In function '" << Func << "': Intrinsic '" << getGenXName(Id)
     << "' is not yet supported";
  llvm::report_fatal_error(SS.str());
}

// Buffer argument kind is converted to GENERAL to denote that
// argument is not BTI but SSO.
Optional<unsigned> PromoteToBindless::tryConvertBuffer(unsigned Kind,
                                                       StringRef Desc) {
  // Do not change kind if promotion is not enabled.
  if (!BC.useBindlessBuffers())
    return None;

  if (Kind != vc::KernelMetadata::AK_SURFACE)
    return None;

  if (!vc::isDescBufferType(Desc))
    return None;

  // If this is a buffer, change argument kind to general value.
  return vc::KernelMetadata::AK_NORMAL;
}

// Convert single stateful argument to bindless counterpart.
// Currently only buffers are supported -- extra objects can be easily added
// here later.
unsigned PromoteToBindless::convertSingleArg(unsigned Kind, StringRef Desc) {
  if (auto MaybeKind = tryConvertBuffer(Kind, Desc))
    return MaybeKind.getValue();

  return Kind;
}

// Convert kernel arguments if there is any that need conversion.
// Return true if metadata was modified.
bool PromoteToBindless::convertKernelArguments(Function &F) {
  vc::KernelMetadata KM{&F};

  ArrayRef<unsigned> ArgKinds = KM.getArgKinds();
  ArrayRef<StringRef> ArgDescs = KM.getArgTypeDescs();

  SmallVector<unsigned, 8> NewArgKinds{ArgKinds.begin(), ArgKinds.end()};
  bool Changed = false;
  for (auto &&[Kind, Desc, NewKind] :
       llvm::zip(ArgKinds, ArgDescs, NewArgKinds)) {
    NewKind = convertSingleArg(Kind, Desc);
    Changed |= NewKind != Kind;
  }
  if (Changed)
    KM.updateArgKindsMD(std::move(NewArgKinds));
  return Changed;
}

// First part of transformation: conversion of arguments to SSO.
// Return true if IR was modified.
bool PromoteToBindless::convertArguments() {
  bool Changed = false;
  for (Function &Kernel : vc::kernels(M)) {
    Changed |= convertKernelArguments(Kernel);
  }

  return Changed;
}

// Lazily get BSS variable if this is needed. Create if nothing
// was here before.
GlobalVariable &PromoteToBindless::getOrCreateBSSVariable() {
  if (!BSS)
    BSS = &vc::PredefVar::createBSS(M);
  return *BSS;
}

// Get surface operand number for given intrinsic.
static unsigned getSurfaceOperandNo(GenXIntrinsic::ID Id) {
  using namespace GenXIntrinsic;

  switch (Id) {
  case genx_dword_atomic2_add:
  case genx_dword_atomic2_sub:
  case genx_dword_atomic2_min:
  case genx_dword_atomic2_max:
  case genx_dword_atomic2_xchg:
  case genx_dword_atomic2_and:
  case genx_dword_atomic2_or:
  case genx_dword_atomic2_xor:
  case genx_dword_atomic2_imin:
  case genx_dword_atomic2_imax:
  case genx_dword_atomic2_fmin:
  case genx_dword_atomic2_fmax:
  case genx_dword_atomic2_fadd:
  case genx_dword_atomic2_fsub:
  case genx_dword_atomic2_inc:
  case genx_dword_atomic2_dec:
  case genx_dword_atomic2_cmpxchg:
  case genx_dword_atomic2_fcmpwr:
    return 1;

  case genx_gather_masked_scaled2:
  case genx_gather4_masked_scaled2:
    return 2;

  case genx_scatter_scaled:
  case genx_scatter4_scaled:
    return 3;

  case genx_oword_ld:
  case genx_oword_ld_unaligned:
    return 1;

  case genx_oword_st:
    return 0;
  default:
    reportUnhandledIntrinsic("getSurfaceOperandNo", Id);
  }
  return 0;
}

// Check whether instruction operates on buffer surface.
// Only new versions of intrinsics are handled.
// They are coming from CM and this also allows to prevent
// code bloat because of supporting of legacy versions.
static bool isBufferIntrinsic(const Instruction &I) {
  using namespace GenXIntrinsic;

  const auto ID = getGenXIntrinsicID(&I);
  switch (ID) {
  // LSC.
  case genx_lsc_load_bti:
  case genx_lsc_prefetch_bti:
  case genx_lsc_load_quad_bti:
  case genx_lsc_store_bti:
  case genx_lsc_store_quad_bti:
  case genx_lsc_xatomic_bti:
    return true;

  // DWORD binary atomics.
  case genx_dword_atomic2_add:
  case genx_dword_atomic2_sub:
  case genx_dword_atomic2_min:
  case genx_dword_atomic2_max:
  case genx_dword_atomic2_xchg:
  case genx_dword_atomic2_and:
  case genx_dword_atomic2_or:
  case genx_dword_atomic2_xor:
  case genx_dword_atomic2_imin:
  case genx_dword_atomic2_imax:

  // DWORD floating binary atomics
  case genx_dword_atomic2_fmin:
  case genx_dword_atomic2_fmax:
  case genx_dword_atomic2_fadd:
  case genx_dword_atomic2_fsub:

  // DWORD unary atomics.
  case genx_dword_atomic2_inc:
  case genx_dword_atomic2_dec:

  // DWORD ternary atomics.
  case genx_dword_atomic2_cmpxchg:
  case genx_dword_atomic2_fcmpwr:

  // Gather/scatter operations.
  case genx_gather_masked_scaled2:
  case genx_gather4_masked_scaled2:
  case genx_scatter_scaled:
  case genx_scatter4_scaled:

  // OWORD operations.
  case genx_oword_ld:
  case genx_oword_ld_unaligned:
  case genx_oword_st: {
    // Check additionally that surface argument in not a constant.
    // If argument is a constant then promotion cannot happen because
    // it can be SLM, stack or stateless access.
    const unsigned SurfOpNo = getSurfaceOperandNo(ID);
    return !isa<ConstantInt>(cast<CallInst>(I).getArgOperand(SurfOpNo));
  }
  default:
    break;
  }
  return false;
}

static std::vector<CallInst *> collectBufferIntrinsics(Module &M) {
  std::vector<CallInst *> Collected;

  for (Function &F : M)
    for (auto &I : instructions(F)) {
      if (isBufferIntrinsic(I))
        Collected.push_back(cast<CallInst>(&I));
    }

  return Collected;
}

// Make first part of transformation: create write of SSO to %bss.
static void createSurfaceStateOffsetWrite(Value &SSO, IRBuilder<> &IRB,
                                          Module &M, GlobalVariable &BSS) {
  using namespace GenXIntrinsic;

  Type *Tys[] = {BSS.getType()};
  auto *Decl = getGenXDeclaration(&M, genx_write_predef_surface, Tys);
  Value *Args[] = {&BSS, &SSO};
  IRB.CreateCall(Decl->getFunctionType(), Decl, Args);
}

// For given intrinsic ID get version with predefined surface variable
// that allows to use it with %bss variable.
static GenXIntrinsic::ID getBindlessDataportIntrinsicID(GenXIntrinsic::ID Id) {
  using namespace GenXIntrinsic;

#define MAP(intr)                                                              \
  case intr:                                                                   \
    return intr##_predef_surface;

  switch (Id) {
    MAP(genx_dword_atomic2_add);
    MAP(genx_dword_atomic2_sub);
    MAP(genx_dword_atomic2_min);
    MAP(genx_dword_atomic2_max);
    MAP(genx_dword_atomic2_xchg);
    MAP(genx_dword_atomic2_and);
    MAP(genx_dword_atomic2_or);
    MAP(genx_dword_atomic2_xor);
    MAP(genx_dword_atomic2_imin);
    MAP(genx_dword_atomic2_imax);

    MAP(genx_dword_atomic2_fmin);
    MAP(genx_dword_atomic2_fmax);
    MAP(genx_dword_atomic2_fadd);
    MAP(genx_dword_atomic2_fsub);

    MAP(genx_dword_atomic2_inc);
    MAP(genx_dword_atomic2_dec);

    MAP(genx_dword_atomic2_cmpxchg);
    MAP(genx_dword_atomic2_fcmpwr);

    MAP(genx_gather_masked_scaled2);
    MAP(genx_gather4_masked_scaled2);
    MAP(genx_scatter_scaled);
    MAP(genx_scatter4_scaled);

    MAP(genx_oword_ld);
    MAP(genx_oword_ld_unaligned);
    MAP(genx_oword_st);
  default:
    reportUnhandledIntrinsic("getBindlessDataportIntrinsicID", Id);
  }
#undef MAP

  return not_genx_intrinsic;
}

// Second part of transformation for dataport intrinsic: create bindless version
// that uses %bss. Mostly it is a copy of original intrinsic with a little
// change: BSS global is used instead of BTI.
static CallInst *createBindlessSurfaceDataportIntrinsic(
    CallInst &CI, IRBuilder<> &IRB, Module &M, GlobalVariable &BSS,
    GenXIntrinsic::ID Id, unsigned SurfaceOpNo) {
  using namespace GenXIntrinsic;

  SmallVector<Value *, 8> Args{CI.arg_begin(), CI.arg_end()};
  Args[SurfaceOpNo] = &BSS;

  const auto NewId = getBindlessDataportIntrinsicID(Id);

  auto *Decl =
      vc::getGenXDeclarationForIdFromArgs(CI.getType(), Args, NewId, M);

  return IRB.CreateCall(Decl->getFunctionType(), Decl, Args, CI.getName());
}

// Create write of SSO to BSS variable.
// Then create memory intrinsic that uses BSS variable instead
// of original state argument.
// Return newly created bindless instruction (second instruction in chain).
CallInst *
PromoteToBindless::createBindlessSurfaceDataportIntrinsicChain(CallInst &CI) {
  const auto ID = GenXIntrinsic::getGenXIntrinsicID(&CI);
  const unsigned SurfaceOpNo = getSurfaceOperandNo(ID);

  IRBuilder<> IRB{&CI};
  GlobalVariable &BSS = getOrCreateBSSVariable();
  Value *SSO = CI.getArgOperand(SurfaceOpNo);
  Module *M = CI.getModule();
  IGC_ASSERT_MESSAGE(M, "Instruction expected to be in module");
  createSurfaceStateOffsetWrite(*SSO, IRB, *M, BSS);

  return createBindlessSurfaceDataportIntrinsic(CI, IRB, *M, BSS, ID,
                                                SurfaceOpNo);
}

// Get bindless version of given bti lsc intrinsic.
static GenXIntrinsic::ID getBindlessLscIntrinsicID(GenXIntrinsic::ID Id) {
  using namespace GenXIntrinsic;

#define MAP(intr)                                                              \
  case intr##_bti:                                                             \
    return intr##_bindless;

  switch (Id) {
    MAP(genx_lsc_load);
    MAP(genx_lsc_prefetch);
    MAP(genx_lsc_load_quad);
    MAP(genx_lsc_store);
    MAP(genx_lsc_store_quad);
    MAP(genx_lsc_xatomic);
  default:
    reportUnhandledIntrinsic("getBindlessLscIntrinsicID", Id);
  }
#undef MAP

  return not_genx_intrinsic;
}

// Create bindless version of lsc bti intrinsic.
// Return newly created instruction.
// Bindless lsc intrinsics representation differs from legacy dataport
// intrinsics. Lsc intrinsics have special addressing mode operand so
// there is no need to use %bss variable and SSO goes directly to lsc
// instruction.
static CallInst *createBindlessLscIntrinsic(CallInst &CI) {
  const auto ID = GenXIntrinsic::getGenXIntrinsicID(&CI);
  const auto NewId = getBindlessLscIntrinsicID(ID);

  auto *Decl = vc::getGenXDeclarationForIdFromArgs(CI.getType(), CI.args(),
                                                   NewId, *CI.getModule());

  IRBuilder<> IRB{&CI};
  SmallVector<Value *, 16> Args{CI.args()};
  return IRB.CreateCall(Decl->getFunctionType(), Decl, Args, CI.getName());
}

void PromoteToBindless::rewriteBufferIntrinsic(CallInst &CI) {
  using namespace GenXIntrinsic;

  const auto ID = getGenXIntrinsicID(&CI);
  CallInst *BindlessCI = nullptr;
  switch (ID) {
  default:
    IGC_ASSERT_MESSAGE(0, "Unhandled buffer intrinsic");
    break;
  case genx_dword_atomic2_add:
  case genx_dword_atomic2_sub:
  case genx_dword_atomic2_min:
  case genx_dword_atomic2_max:
  case genx_dword_atomic2_xchg:
  case genx_dword_atomic2_and:
  case genx_dword_atomic2_or:
  case genx_dword_atomic2_xor:
  case genx_dword_atomic2_imin:
  case genx_dword_atomic2_imax:
  case genx_dword_atomic2_fmin:
  case genx_dword_atomic2_fmax:
  case genx_dword_atomic2_fadd:
  case genx_dword_atomic2_fsub:
  case genx_dword_atomic2_inc:
  case genx_dword_atomic2_dec:
  case genx_dword_atomic2_cmpxchg:
  case genx_dword_atomic2_fcmpwr:
  case genx_gather_masked_scaled2:
  case genx_gather4_masked_scaled2:
  case genx_scatter_scaled:
  case genx_scatter4_scaled:
  case genx_oword_ld:
  case genx_oword_ld_unaligned:
  case genx_oword_st:
    BindlessCI = createBindlessSurfaceDataportIntrinsicChain(CI);
    break;
  case genx_lsc_load_bti:
  case genx_lsc_prefetch_bti:
  case genx_lsc_load_quad_bti:
  case genx_lsc_store_bti:
  case genx_lsc_store_quad_bti:
  case genx_lsc_xatomic_bti:
    BindlessCI = createBindlessLscIntrinsic(CI);
    break;
  }

  IGC_ASSERT_MESSAGE(BindlessCI, "Expected created bindless instruction");
  if (!CI.getType()->isVoidTy())
    CI.replaceAllUsesWith(BindlessCI);
  CI.eraseFromParent();
}

bool PromoteToBindless::rewriteBufferIntrinsics() {
  std::vector<CallInst *> bufferIntrinsics = collectBufferIntrinsics(M);

  if (bufferIntrinsics.empty())
    return false;

  for (CallInst *CI : bufferIntrinsics)
    rewriteBufferIntrinsic(*CI);

  return true;
}

// Second part of transformation: rewriting of memory intrinsics.
// Return true if there were changes.
// Currently only buffer intrinsics are supported.
bool PromoteToBindless::rewriteIntrinsics() {
  bool Changed = false;

  if (BC.useBindlessBuffers())
    Changed |= rewriteBufferIntrinsics();

  return Changed;
}

bool PromoteToBindless::run() {
  bool Changed = convertArguments();

  Changed |= rewriteIntrinsics();

  return Changed;
}
