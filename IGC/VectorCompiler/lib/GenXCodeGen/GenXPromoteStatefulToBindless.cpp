/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2024 Intel Corporation

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
///   call void @llvm.genx.write.predef.surface.p0i32(i32*
///   @llvm.genx.predefined.bss, i32 %buf) call void
///   @llvm.genx.oword.st.predef.surface.p0i32.v8i32(i32*
///   @llvm.genx.predefined.bss, i32 %addr, <8 x i32> %src)
///
/// Pass is intended to be as simple as possible to utilize benefits from
/// previous passes like lowering and legalization. This is sort of
/// pseudo-expansion in presense of option for bindless mode. Behavior of
/// this pass is controlled by options for bindless memory objects.
///
/// Currently, only support for buffers is implemented. However, images and
/// samplers can be easily added here.

#include "GenX.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"

#include "vc/Support/BackendConfig.h"
#include "vc/Utils/GenX/Intrinsics.h"
#include "vc/Utils/GenX/KernelInfo.h"
#include "vc/Utils/GenX/PredefinedVariable.h"

#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"

#include "visa_igc_common_header.h"

#include "Probe/Assertion.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>
#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Module.h>
#include <llvm/InitializePasses.h>
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
  GlobalVariable *BindlessSampler = nullptr;
  const GenXSubtarget &ST;

public:
  PromoteToBindless(Module &InM, const GenXBackendConfig &InBC,
                    const GenXSubtarget &InST)
      : M{InM}, BC{InBC}, ST(InST) {}

  bool run();

private:
  unsigned convertSingleArg(unsigned Kind, StringRef Desc);
  bool convertKernelArguments(Function &F);
  bool convertArguments();

  GlobalVariable &getOrCreateBSSVariable();
  GlobalVariable &getOrCreateBindlessSamplerVariable();

  vc::InternalIntrinsic::ID getBindlessLscIntrinsicID(unsigned IID);
  CallInst *createBindlessLscIntrinsic(CallInst &CI);
  CallInst *createBindlessSurfaceDataportIntrinsicChain(CallInst &CI);
  void rewriteStatefulIntrinsic(CallInst &CI);
  bool rewriteStatefulIntrinsics();
};

class GenXPromoteStatefulToBindless final : public ModulePass {
public:
  static char ID;

  GenXPromoteStatefulToBindless() : ModulePass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<TargetPassConfig>();
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
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
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

  const GenXSubtarget &ST = getAnalysis<TargetPassConfig>()
                                .getTM<GenXTargetMachine>()
                                .getGenXSubtarget();

  PromoteToBindless PTM{M, BC, ST};

  return PTM.run();
}

// Convert single stateful argument to bindless counterpart.
// Currently only buffers and images are supported
// extra objects can be easily added here later.
unsigned PromoteToBindless::convertSingleArg(unsigned Kind, StringRef Desc) {
  if (ST.hasEfficient64b() &&
      (vc::isDescBufferType(Desc) || vc::isDescImageType(Desc) ||
       vc::isDescSamplerType(Desc)))
    return vc::KernelMetadata::AK_NORMAL;

  if (Kind != vc::KernelMetadata::AK_SURFACE &&
      Kind != vc::KernelMetadata::AK_SAMPLER)
    return Kind;

  if (BC.useBindlessBuffers() && vc::isDescBufferType(Desc))
    return vc::KernelMetadata::AK_NORMAL;

  if (BC.useBindlessImages() && vc::isDescImageType(Desc))
    return vc::KernelMetadata::AK_NORMAL;

  if (BC.useBindlessImages() && vc::isDescSamplerType(Desc))
    return vc::KernelMetadata::AK_NORMAL;

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

// Lazily get BindlessSampler variable if this is needed. Create if nothing
// was here before.
GlobalVariable &PromoteToBindless::getOrCreateBindlessSamplerVariable() {
  if (!BindlessSampler)
    BindlessSampler = &vc::PredefVar::createBindlessSampler(M);
  return *BindlessSampler;
}

// Get surface operand number for given intrinsic.
static int getSurfaceOperandNo(unsigned Id) {
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

  case genx_media_ld:
  case genx_media_st:
    return 1;

  case genx_typed_atomic_add:
  case genx_typed_atomic_sub:
  case genx_typed_atomic_min:
  case genx_typed_atomic_max:
  case genx_typed_atomic_xchg:
  case genx_typed_atomic_and:
  case genx_typed_atomic_or:
  case genx_typed_atomic_xor:
  case genx_typed_atomic_imin:
  case genx_typed_atomic_imax:
  case genx_typed_atomic_inc:
  case genx_typed_atomic_dec:
  case genx_typed_atomic_cmpxchg:
    return 1;

  case genx_gather4_typed:
  case genx_scatter4_typed:
    return 2;

  case vc::InternalIntrinsic::sampler_load_bti:
  case vc::InternalIntrinsic::sample_bti:
    return 4;

  default:
    return -1;
  }
}

// Get address size operand number for given intrinsic.
static int getAddrSizeOperandNo(unsigned Id) {
  switch (Id) {
  case vc::InternalIntrinsic::lsc_atomic_bti:
    return 2;
  case vc::InternalIntrinsic::lsc_load_bti:
  case vc::InternalIntrinsic::lsc_prefetch_bti:
  case vc::InternalIntrinsic::lsc_store_bti:
  case vc::InternalIntrinsic::lsc_load_quad_bti:
  case vc::InternalIntrinsic::lsc_prefetch_quad_bti:
  case vc::InternalIntrinsic::lsc_store_quad_bti:
    return 1;
  default:
    return -1;
  }
}

// Check whether instruction operates on BTI surface.
// Only new versions of intrinsics are handled.
// They are coming from CM and this also allows to prevent
// code bloat because of supporting of legacy versions.
static bool isStatefulIntrinsic(const Instruction &I) {
  const auto ID = vc::getAnyIntrinsicID(&I);
  switch (ID) {
  // LSC.
  case vc::InternalIntrinsic::lsc_atomic_bti:
  case vc::InternalIntrinsic::lsc_load_bti:
  case vc::InternalIntrinsic::lsc_load_quad_bti:
  case vc::InternalIntrinsic::lsc_prefetch_bti:
  case vc::InternalIntrinsic::lsc_prefetch_quad_bti:
  case vc::InternalIntrinsic::lsc_store_bti:
  case vc::InternalIntrinsic::lsc_store_quad_bti:
  case vc::InternalIntrinsic::lsc_load_2d_tgm_bti:
  case vc::InternalIntrinsic::lsc_store_2d_tgm_bti:
  case vc::InternalIntrinsic::lsc_load_quad_tgm:
  case vc::InternalIntrinsic::lsc_store_quad_tgm:
  case vc::InternalIntrinsic::lsc_prefetch_quad_tgm:
  case vc::InternalIntrinsic::sampler_load_bti:
  case vc::InternalIntrinsic::sample_bti:
    return true;

  // DWORD binary atomics.
  case GenXIntrinsic::genx_dword_atomic2_add:
  case GenXIntrinsic::genx_dword_atomic2_sub:
  case GenXIntrinsic::genx_dword_atomic2_min:
  case GenXIntrinsic::genx_dword_atomic2_max:
  case GenXIntrinsic::genx_dword_atomic2_xchg:
  case GenXIntrinsic::genx_dword_atomic2_and:
  case GenXIntrinsic::genx_dword_atomic2_or:
  case GenXIntrinsic::genx_dword_atomic2_xor:
  case GenXIntrinsic::genx_dword_atomic2_imin:
  case GenXIntrinsic::genx_dword_atomic2_imax:

  // DWORD floating binary atomics
  case GenXIntrinsic::genx_dword_atomic2_fmin:
  case GenXIntrinsic::genx_dword_atomic2_fmax:
  case GenXIntrinsic::genx_dword_atomic2_fadd:
  case GenXIntrinsic::genx_dword_atomic2_fsub:

  // DWORD unary atomics.
  case GenXIntrinsic::genx_dword_atomic2_inc:
  case GenXIntrinsic::genx_dword_atomic2_dec:

  // DWORD ternary atomics.
  case GenXIntrinsic::genx_dword_atomic2_cmpxchg:
  case GenXIntrinsic::genx_dword_atomic2_fcmpwr:

  // Gather/scatter operations.
  case GenXIntrinsic::genx_gather_masked_scaled2:
  case GenXIntrinsic::genx_gather4_masked_scaled2:
  case GenXIntrinsic::genx_scatter_scaled:
  case GenXIntrinsic::genx_scatter4_scaled:

  // Media image operations.
  case GenXIntrinsic::genx_media_ld:
  case GenXIntrinsic::genx_media_st:

    // Typed images operations.
  case GenXIntrinsic::genx_typed_atomic_add:
  case GenXIntrinsic::genx_typed_atomic_sub:
  case GenXIntrinsic::genx_typed_atomic_min:
  case GenXIntrinsic::genx_typed_atomic_max:
  case GenXIntrinsic::genx_typed_atomic_xchg:
  case GenXIntrinsic::genx_typed_atomic_and:
  case GenXIntrinsic::genx_typed_atomic_or:
  case GenXIntrinsic::genx_typed_atomic_xor:
  case GenXIntrinsic::genx_typed_atomic_imin:
  case GenXIntrinsic::genx_typed_atomic_imax:
  case GenXIntrinsic::genx_typed_atomic_inc:
  case GenXIntrinsic::genx_typed_atomic_dec:
  case GenXIntrinsic::genx_typed_atomic_cmpxchg:
  case GenXIntrinsic::genx_gather4_typed:
  case GenXIntrinsic::genx_scatter4_typed:

  // OWORD operations.
  case GenXIntrinsic::genx_oword_ld:
  case GenXIntrinsic::genx_oword_ld_unaligned:
  case GenXIntrinsic::genx_oword_st: {
    // Check additionally that surface argument in not a constant.
    // If argument is a constant then promotion cannot happen because
    // it can be SLM, stack or stateless access.
    const auto SurfaceOpNo = getSurfaceOperandNo(ID);
    IGC_ASSERT_EXIT_MESSAGE(SurfaceOpNo >= 0, "Unknown surface operand number");
    return !isa<ConstantInt>(cast<CallInst>(I).getArgOperand(SurfaceOpNo));
  }
  default:
    break;
  }
  return false;
}

static std::vector<CallInst *> collectStatefulIntrinsics(Module &M) {
  std::vector<CallInst *> Collected;

  for (Function &F : M)
    for (auto &I : instructions(F)) {
      if (isStatefulIntrinsic(I))
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

// Make first part of transformation: create write of Sampler state to %bss.
static void createSamplerStateOffsetWrite(Value &SSO, IRBuilder<> &IRB,
                                          Module &M, GlobalVariable &BSS) {
  Type *Tys[] = {BSS.getType()};
  auto *Decl = vc::getAnyDeclaration(
      &M, vc::InternalIntrinsic::write_predef_sampler, Tys);
  Value *Args[] = {&BSS, &SSO};
  IRB.CreateCall(Decl->getFunctionType(), Decl, Args);
}

// For given intrinsic ID get version with predefined surface variable
// that allows to use it with %bss variable.
static unsigned getBindlessDataportIntrinsicID(unsigned Id) {
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
#undef MAP

#define MAP(intr)                                                              \
  case genx_##intr:                                                            \
    return vc::InternalIntrinsic::intr##_predef_surface;

    MAP(media_ld);
    MAP(media_st);
    MAP(typed_atomic_add);
    MAP(typed_atomic_sub);
    MAP(typed_atomic_min);
    MAP(typed_atomic_max);
    MAP(typed_atomic_xchg);
    MAP(typed_atomic_and);
    MAP(typed_atomic_or);
    MAP(typed_atomic_xor);
    MAP(typed_atomic_imin);
    MAP(typed_atomic_imax);
    MAP(typed_atomic_inc);
    MAP(typed_atomic_dec);
    MAP(typed_atomic_cmpxchg);
    MAP(gather4_typed);
    MAP(scatter4_typed);
#undef MAP

  case vc::InternalIntrinsic::sampler_load_bti:
    return vc::InternalIntrinsic::sampler_load_predef_surface;
  case vc::InternalIntrinsic::sample_bti:
    return vc::InternalIntrinsic::sample_predef_surface;

  default:
    return not_genx_intrinsic;
  }
}

// Second part of transformation for dataport intrinsic: create bindless version
// that uses %bss. Mostly it is a copy of original intrinsic with a little
// change: BSS global is used instead of BTI.
static CallInst *
createBindlessSurfaceDataportIntrinsic(CallInst &CI, IRBuilder<> &IRB,
                                       Module &M, GlobalVariable &BSS,
                                       unsigned Id, unsigned SurfaceOpNo) {
  using namespace GenXIntrinsic;

  SmallVector<Value *, 8> Args{CI.args()};
  Args[SurfaceOpNo] = &BSS;

  const auto NewId = getBindlessDataportIntrinsicID(Id);

  auto *Decl = vc::getAnyDeclarationForArgs(&M, NewId, CI.getType(), Args);
  return IRB.CreateCall(Decl->getFunctionType(), Decl, Args, CI.getName());
}

// Create write of SSO to BSS variable.
// Then create memory intrinsic that uses BSS variable instead
// of original state argument.
// Return newly created bindless instruction (second instruction in chain).
CallInst *
PromoteToBindless::createBindlessSurfaceDataportIntrinsicChain(CallInst &CI) {
  const auto ID = vc::getAnyIntrinsicID(&CI);
  const auto SurfaceOpNo = getSurfaceOperandNo(ID);
  IGC_ASSERT_EXIT_MESSAGE(SurfaceOpNo >= 0, "Unknown surface operand number");

  IRBuilder<> IRB{&CI};
  GlobalVariable &BSS = getOrCreateBSSVariable();
  Value *SSO = CI.getArgOperand(SurfaceOpNo);
  Module *M = CI.getModule();
  IGC_ASSERT_MESSAGE(M, "Instruction expected to be in module");
  createSurfaceStateOffsetWrite(*SSO, IRB, *M, BSS);

  if (auto SamplerOpNo =
          vc::InternalIntrinsic::getMemorySamplerOperandIndex(ID);
      SamplerOpNo >= 0) {
    GlobalVariable &BindlessSampler = getOrCreateBindlessSamplerVariable();
    Value *SamplerStateOp = CI.getArgOperand(SamplerOpNo);
    createSamplerStateOffsetWrite(*SamplerStateOp, IRB, *M, BindlessSampler);

    SmallVector<Value *, 19> Args{CI.args()};
    Args[SurfaceOpNo] = &BSS;
    Args[SamplerOpNo] = &BindlessSampler;

    const auto NewId = getBindlessDataportIntrinsicID(ID);

    auto *Decl = vc::getAnyDeclarationForArgs(M, NewId, CI.getType(), Args);
    return IRB.CreateCall(Decl->getFunctionType(), Decl, Args, CI.getName());
  }

  return createBindlessSurfaceDataportIntrinsic(CI, IRB, *M, BSS, ID,
                                                SurfaceOpNo);
}

// Get bindless version of given bti lsc intrinsic.
vc::InternalIntrinsic::ID
PromoteToBindless::getBindlessLscIntrinsicID(unsigned IID) {

  if (ST.hasEfficient64b()) {
    switch (IID) {
    case vc::InternalIntrinsic::lsc_atomic_bti:
      return vc::InternalIntrinsic::lsc_atomic_surf;
    case vc::InternalIntrinsic::lsc_load_bti:
      return vc::InternalIntrinsic::lsc_load_surf;
    case vc::InternalIntrinsic::lsc_load_quad_bti:
      return vc::InternalIntrinsic::lsc_load_quad_surf;
    case vc::InternalIntrinsic::lsc_prefetch_bti:
      return vc::InternalIntrinsic::lsc_prefetch_surf;
    case vc::InternalIntrinsic::lsc_prefetch_quad_bti:
      return vc::InternalIntrinsic::lsc_prefetch_quad_surf;
    case vc::InternalIntrinsic::lsc_store_bti:
      return vc::InternalIntrinsic::lsc_store_surf;
    case vc::InternalIntrinsic::lsc_store_quad_bti:
      return vc::InternalIntrinsic::lsc_store_quad_surf;
    case vc::InternalIntrinsic::lsc_load_2d_tgm_bti:
      return vc::InternalIntrinsic::lsc_load_2d_tgm_surf;
    case vc::InternalIntrinsic::lsc_store_2d_tgm_bti:
      return vc::InternalIntrinsic::lsc_store_2d_tgm_surf;
    case vc::InternalIntrinsic::lsc_load_quad_tgm:
      return vc::InternalIntrinsic::lsc_load_quad_tgm_surf;
    case vc::InternalIntrinsic::lsc_store_quad_tgm:
      return vc::InternalIntrinsic::lsc_store_quad_tgm_surf;
    case vc::InternalIntrinsic::lsc_prefetch_quad_tgm:
      return vc::InternalIntrinsic::lsc_prefetch_quad_tgm_surf;
    case vc::InternalIntrinsic::sampler_load_bti:
      return vc::InternalIntrinsic::sampler_load_surf;
    case vc::InternalIntrinsic::sample_bti:
      return vc::InternalIntrinsic::sample_surf;
    default:
      return vc::InternalIntrinsic::not_any_intrinsic;
    }
  }

  switch (IID) {
  case vc::InternalIntrinsic::lsc_atomic_bti:
    return vc::InternalIntrinsic::lsc_atomic_bss;
  case vc::InternalIntrinsic::lsc_load_bti:
    return vc::InternalIntrinsic::lsc_load_bss;
  case vc::InternalIntrinsic::lsc_load_quad_bti:
    return vc::InternalIntrinsic::lsc_load_quad_bss;
  case vc::InternalIntrinsic::lsc_prefetch_bti:
    return vc::InternalIntrinsic::lsc_prefetch_bss;
  case vc::InternalIntrinsic::lsc_prefetch_quad_bti:
    return vc::InternalIntrinsic::lsc_prefetch_quad_bss;
  case vc::InternalIntrinsic::lsc_store_bti:
    return vc::InternalIntrinsic::lsc_store_bss;
  case vc::InternalIntrinsic::lsc_store_quad_bti:
    return vc::InternalIntrinsic::lsc_store_quad_bss;
  case vc::InternalIntrinsic::lsc_load_2d_tgm_bti:
    return vc::InternalIntrinsic::lsc_load_2d_tgm_bss;
  case vc::InternalIntrinsic::lsc_store_2d_tgm_bti:
    return vc::InternalIntrinsic::lsc_store_2d_tgm_bss;
  case vc::InternalIntrinsic::lsc_load_quad_tgm:
    return vc::InternalIntrinsic::lsc_load_quad_tgm_bss;
  case vc::InternalIntrinsic::lsc_store_quad_tgm:
    return vc::InternalIntrinsic::lsc_store_quad_tgm_bss;
  case vc::InternalIntrinsic::lsc_prefetch_quad_tgm:
    return vc::InternalIntrinsic::lsc_prefetch_quad_tgm_bss;
  default:
    return vc::InternalIntrinsic::not_any_intrinsic;
  }
}

// Create bindless version of lsc bti intrinsic.
// Return newly created instruction.
// Bindless lsc intrinsics representation differs from legacy dataport
// intrinsics. Lsc intrinsics have special addressing mode operand so
// there is no need to use %bss variable and SSO goes directly to lsc
// instruction.
CallInst *PromoteToBindless::createBindlessLscIntrinsic(CallInst &CI) {
  const auto ID = vc::getAnyIntrinsicID(&CI);
  const auto NewId = getBindlessLscIntrinsicID(ID);

  SmallVector<Value *, 16> Args{CI.args()};
  IRBuilder<> IRB{&CI};

  if (ST.hasEfficient64b()) {
    auto ConvertState = [&](Value *SSO) -> Value * {
      auto *RdRegion = dyn_cast<CallInst>(SSO);
      IGC_ASSERT_MESSAGE(RdRegion && GenXIntrinsic::isRdRegion(RdRegion),
                         "RdRegion is expected");
      auto *OV = RdRegion->getArgOperand(
          GenXIntrinsic::GenXRegion::OldValueOperandNum);
      auto *Ty = OV->getType();
      auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
      IGC_ASSERT_MESSAGE(
          VTy && VTy->isIntOrIntVectorTy(32) && VTy->getNumElements() % 2 == 0,
          "RdRegion OldValueOperand is expected to be <2N x i32>");

      IGC_ASSERT(
          cast<ConstantInt>(RdRegion->getArgOperand(
                                GenXIntrinsic::GenXRegion::RdWidthOperandNum))
              ->getZExtValue() == 1);

      auto *BC = dyn_cast<BitCastInst>(OV);
      IGC_ASSERT_MESSAGE(BC, "BitCast is expected");
      auto *BitCastSrc = BC->getOperand(0);
      auto *BitCastSrcTy = BitCastSrc->getType();
      IGC_ASSERT_MESSAGE(BitCastSrcTy->isIntOrIntVectorTy(64),
                         "BitCast src is expected to be i64 or <N x i64>");

      if (auto *SrcVTy = dyn_cast<IGCLLVM::FixedVectorType>(BitCastSrcTy)) {
        auto *IO = RdRegion->getArgOperand(
            GenXIntrinsic::GenXRegion::RdIndexOperandNum);
        vc::Region R(SrcVTy->getElementType());
        if (auto *Offset = dyn_cast<ConstantInt>(IO))
          R.Offset = Offset->getZExtValue();
        else
          R.Indirect = IO;

        R.VStride = 0;
        R.Width = 1;
        R.Stride = 0;

        return R.createRdRegion(BitCastSrc, "", &CI, CI.getDebugLoc(), true);
      }

      return BitCastSrc;
    };

    if (auto AddrSizeOpNo = getAddrSizeOperandNo(ID); AddrSizeOpNo >= 0)
      Args[AddrSizeOpNo] = IRB.getInt8(LSC_ADDR_SIZE_32bU);

    bool HasImmSurfaceIndex = vc::InternalIntrinsic::hasImmSurfaceIndex(NewId);

    if (auto Index = vc::InternalIntrinsic::getMemorySamplerOperandIndex(ID);
        Index >= 0) {
      auto *Sampler = Args[Index];
      Args[Index] = ConvertState(Sampler);
      if (HasImmSurfaceIndex)
        Args.insert(Args.begin() + Index + 1, IRB.getInt8(0));
    }

    const auto SurfaceOpNo =
        vc::InternalIntrinsic::getMemorySurfaceOperandIndex(ID);
    IGC_ASSERT_EXIT_MESSAGE(SurfaceOpNo >= 0, "Unknown surface operand number");
    auto *SSO = Args[SurfaceOpNo];
    Args[SurfaceOpNo] = ConvertState(SSO);
    if (HasImmSurfaceIndex)
      Args.insert(Args.begin() + SurfaceOpNo + 1, IRB.getInt8(0));
  }

  auto *Decl = vc::getInternalDeclarationForIdFromArgs(CI.getType(), Args,
                                                       NewId, *CI.getModule());

  return IRB.CreateCall(Decl->getFunctionType(), Decl, Args, CI.getName());
}

void PromoteToBindless::rewriteStatefulIntrinsic(CallInst &CI) {
  const auto ID = vc::getAnyIntrinsicID(&CI);
  CallInst *BindlessCI = nullptr;
  switch (ID) {
  default:
    IGC_ASSERT_MESSAGE(0, "Unhandled intrinsic");
    break;
  case GenXIntrinsic::genx_dword_atomic2_add:
  case GenXIntrinsic::genx_dword_atomic2_sub:
  case GenXIntrinsic::genx_dword_atomic2_min:
  case GenXIntrinsic::genx_dword_atomic2_max:
  case GenXIntrinsic::genx_dword_atomic2_xchg:
  case GenXIntrinsic::genx_dword_atomic2_and:
  case GenXIntrinsic::genx_dword_atomic2_or:
  case GenXIntrinsic::genx_dword_atomic2_xor:
  case GenXIntrinsic::genx_dword_atomic2_imin:
  case GenXIntrinsic::genx_dword_atomic2_imax:
  case GenXIntrinsic::genx_dword_atomic2_fmin:
  case GenXIntrinsic::genx_dword_atomic2_fmax:
  case GenXIntrinsic::genx_dword_atomic2_fadd:
  case GenXIntrinsic::genx_dword_atomic2_fsub:
  case GenXIntrinsic::genx_dword_atomic2_inc:
  case GenXIntrinsic::genx_dword_atomic2_dec:
  case GenXIntrinsic::genx_dword_atomic2_cmpxchg:
  case GenXIntrinsic::genx_dword_atomic2_fcmpwr:
  case GenXIntrinsic::genx_gather_masked_scaled2:
  case GenXIntrinsic::genx_gather4_masked_scaled2:
  case GenXIntrinsic::genx_scatter_scaled:
  case GenXIntrinsic::genx_scatter4_scaled:
  case GenXIntrinsic::genx_oword_ld:
  case GenXIntrinsic::genx_oword_ld_unaligned:
  case GenXIntrinsic::genx_oword_st:
    if (BC.useBindlessBuffers())
      BindlessCI = createBindlessSurfaceDataportIntrinsicChain(CI);
    break;
  case GenXIntrinsic::genx_media_ld:
  case GenXIntrinsic::genx_media_st:
  case GenXIntrinsic::genx_typed_atomic_add:
  case GenXIntrinsic::genx_typed_atomic_sub:
  case GenXIntrinsic::genx_typed_atomic_min:
  case GenXIntrinsic::genx_typed_atomic_max:
  case GenXIntrinsic::genx_typed_atomic_xchg:
  case GenXIntrinsic::genx_typed_atomic_and:
  case GenXIntrinsic::genx_typed_atomic_or:
  case GenXIntrinsic::genx_typed_atomic_xor:
  case GenXIntrinsic::genx_typed_atomic_imin:
  case GenXIntrinsic::genx_typed_atomic_imax:
  case GenXIntrinsic::genx_typed_atomic_inc:
  case GenXIntrinsic::genx_typed_atomic_dec:
  case GenXIntrinsic::genx_typed_atomic_cmpxchg:
  case GenXIntrinsic::genx_gather4_typed:
  case GenXIntrinsic::genx_scatter4_typed:
    if (BC.useBindlessImages())
      BindlessCI = createBindlessSurfaceDataportIntrinsicChain(CI);
    break;
  case vc::InternalIntrinsic::lsc_atomic_bti:
  case vc::InternalIntrinsic::lsc_load_bti:
  case vc::InternalIntrinsic::lsc_load_quad_bti:
  case vc::InternalIntrinsic::lsc_prefetch_bti:
  case vc::InternalIntrinsic::lsc_prefetch_quad_bti:
  case vc::InternalIntrinsic::lsc_store_bti:
  case vc::InternalIntrinsic::lsc_store_quad_bti:
    if (BC.useBindlessBuffers() || ST.hasEfficient64b())
      BindlessCI = createBindlessLscIntrinsic(CI);
    break;
  case vc::InternalIntrinsic::lsc_load_2d_tgm_bti:
  case vc::InternalIntrinsic::lsc_store_2d_tgm_bti:
  case vc::InternalIntrinsic::lsc_load_quad_tgm:
  case vc::InternalIntrinsic::lsc_store_quad_tgm:
  case vc::InternalIntrinsic::lsc_prefetch_quad_tgm:
    if (BC.useBindlessImages() || ST.hasEfficient64b())
      BindlessCI = createBindlessLscIntrinsic(CI);
    break;
  case vc::InternalIntrinsic::sampler_load_bti:
  case vc::InternalIntrinsic::sample_bti: {
    if (ST.hasEfficient64b()) {
      BindlessCI = createBindlessLscIntrinsic(CI);
      break;
    }
    if (BC.useBindlessImages())
      BindlessCI = createBindlessSurfaceDataportIntrinsicChain(CI);
    break;
  }
  }

  if (!BindlessCI)
    return;

  if (!CI.getType()->isVoidTy()) {
    CI.replaceAllUsesWith(BindlessCI);
    BindlessCI->takeName(&CI);
  }
  CI.eraseFromParent();
}

bool PromoteToBindless::rewriteStatefulIntrinsics() {
  std::vector<CallInst *> Intrinsics = collectStatefulIntrinsics(M);

  if (Intrinsics.empty())
    return false;

  for (CallInst *CI : Intrinsics)
    rewriteStatefulIntrinsic(*CI);

  return true;
}

bool PromoteToBindless::run() {
  bool Changed = convertArguments();

  Changed |= rewriteStatefulIntrinsics();

  return Changed;
}
