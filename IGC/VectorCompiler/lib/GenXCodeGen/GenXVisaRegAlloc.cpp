/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// GenXVisaRegAlloc is a function group pass that allocates vISA registers to
// LLVM IR values. See GenXVisaRegAlloc.h.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "GENX_REGALLOC"

#include "GenXVisaRegAlloc.h"
#include "GenX.h"
#include "GenXIntrinsics.h"
#include "GenXLiveness.h"
#include "GenXNumbering.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"
#include "vc/Support/BackendConfig.h"
#include "vc/Utils/General/Types.h"
#include "visa_igc_common_header.h"

#include "llvm/ADT/MapVector.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"

#include "Probe/Assertion.h"

using namespace llvm;
using namespace genx;
using namespace visa;

static cl::opt<unsigned> LimitGenXExtraCoalescing("limit-genx-extra-coalescing", cl::init(UINT_MAX), cl::Hidden,
                                      cl::desc("Limit GenX extra coalescing."));

static cl::opt<unsigned> ArithChainLengthThreshold(
    "acc-split-arith-length", cl::init(4), cl::Hidden,
    cl::desc("Arithmetic chain length to localize for accumulator usage"));

char GenXVisaRegAlloc::ID = 0;
INITIALIZE_PASS_BEGIN(GenXVisaRegAlloc, "GenXVisaRegAlloc", "GenXVisaRegAlloc", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXLiveness)
INITIALIZE_PASS_DEPENDENCY(GenXNumbering)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_DEPENDENCY(FunctionGroupAnalysis)
INITIALIZE_PASS_END(GenXVisaRegAlloc, "GenXVisaRegAlloc", "GenXVisaRegAlloc", false, false)

FunctionGroupPass *llvm::createGenXVisaRegAllocPass()
{
  initializeGenXVisaRegAllocPass(*PassRegistry::getPassRegistry());
  return new GenXVisaRegAlloc();
}

void GenXVisaRegAlloc::getAnalysisUsage(AnalysisUsage &AU) const
{
  FunctionGroupPass::getAnalysisUsage(AU);
  AU.addRequired<GenXLiveness>();
  AU.addRequired<GenXNumbering>();
  AU.addRequired<GenXBackendConfig>();
  AU.addRequired<TargetPassConfig>();
  AU.addRequired<FunctionGroupAnalysis>();
  AU.setPreservesAll();
}

/***********************************************************************
 * runOnFunctionGroup : run the register allocator for this FunctionGroup
 *
 * This is currently a trivial allocator that just gives a new vISA virtual
 * register to every single Value.
 */
bool GenXVisaRegAlloc::runOnFunctionGroup(FunctionGroup &FGArg)
{
  FG = &FGArg;
  Liveness = &getAnalysis<GenXLiveness>();
  Numbering = &getAnalysis<GenXNumbering>();
  FGA = &getAnalysis<FunctionGroupAnalysis>();
  BackendConfig = &getAnalysis<GenXBackendConfig>();
  ST = &getAnalysis<TargetPassConfig>()
            .getTM<GenXTargetMachine>()
            .getGenXSubtarget();
  // Empty out the analysis from the last function it was used on.
  RegMap.clear();
  RegStorage.clear();
  PredefinedSurfaceRegs.clear();
  PredefinedRegs.clear();
  for (unsigned i = 0; i != RegCategory::NUMREALCATEGORIES; ++i) {
    CurrentRegId[i] = 0;
  }
  for (unsigned i = 0; i < VISA_NUM_RESERVED_SURFACES; ++i) {
    RegStorage.emplace_back(RegCategory::SURFACE, i);
    PredefinedSurfaceRegs.push_back(&RegStorage.back());
  }

  RegStorage.emplace_back(
      RegCategory::GENERAL, PreDefined_Vars::PREDEFINED_ARG,
      IGCLLVM::FixedVectorType::get(Type::getInt8Ty(FGArg.getContext()),
                      visa::ArgRegSizeInGRFs * ST->getGRFWidth()));
  PredefinedRegs.push_back(&RegStorage.back());
  RegStorage.emplace_back(
      RegCategory::GENERAL, PreDefined_Vars::PREDEFINED_RET,
      IGCLLVM::FixedVectorType::get(Type::getInt8Ty(FGArg.getContext()),
                      visa::RetRegSizeInGRFs * ST->getGRFWidth()));
  PredefinedRegs.push_back(&RegStorage.back());
  RegStorage.emplace_back(
      RegCategory::GENERAL, PreDefined_Vars::PREDEFINED_FE_SP,
      IGCLLVM::FixedVectorType::get(Type::getInt64Ty(FGArg.getContext()), 1));
  PredefinedRegs.push_back(&RegStorage.back());
  RegStorage.emplace_back(
      RegCategory::GENERAL, PreDefined_Vars::PREDEFINED_FE_FP,
      IGCLLVM::FixedVectorType::get(Type::getInt64Ty(FGArg.getContext()), 1));
  PredefinedRegs.push_back(&RegStorage.back());

  for (auto &F : *FG) {
    if (F->hasFnAttribute(genx::FunctionMD::CMGenXMain) ||
        F->hasFnAttribute(genx::FunctionMD::CMStackCall))
      RegMap[F] = KernRegMap_t();
  }
  // Reserve the reserved registers.
  CurrentRegId[RegCategory::GENERAL] = VISA_NUM_RESERVED_REGS;
  CurrentRegId[RegCategory::PREDICATE] = VISA_NUM_RESERVED_PREDICATES;
  CurrentRegId[RegCategory::SURFACE] = VISA_NUM_RESERVED_SURFACES;
  // Do some extra coalescing.
  extraCoalescing();
  // Get the live ranges in a reproducible order.
  std::vector<LiveRange *> LRs;
  getLiveRanges(LRs);

  if (BackendConfig->localizeLiveRangesForAccUsage())
    localizeLiveRangesForAccUsage(LRs);

  // Allocate a register to each live range.
  for (auto i = LRs.begin(), e = LRs.end(); i != e; ++i)
    allocReg(*i);
  if (CurrentRegId[RegCategory::GENERAL] > VISA_MAX_GENERAL_REGS)
    report_fatal_error("Too many vISA general registers");
  if (CurrentRegId[RegCategory::ADDRESS] > VISA_MAX_ADDRESS_REGS)
    report_fatal_error("Too many vISA address registers");
  if (CurrentRegId[RegCategory::PREDICATE] > VISA_MAX_PREDICATE_REGS)
    report_fatal_error("Too many vISA predicate registers");
  if (CurrentRegId[RegCategory::SAMPLER] > VISA_MAX_SAMPLER_REGS)
    report_fatal_error("Too many vISA sampler registers");
  if (CurrentRegId[RegCategory::SURFACE] > VISA_MAX_SURFACE_REGS)
    report_fatal_error("Too many vISA surface registers");
  if (CurrentRegId[RegCategory::VME] > VISA_MAX_VME_REGS)
    report_fatal_error("Too many vISA VME registers");
  return false;
}

/***********************************************************************
 * getLiveRanges : get the live ranges in a reproducible order
 *
 * We scan the code to find the live ranges, rather than just walking the
 * GenXLiveness map, to ensure that registers are allocated in a consistent
 * order that does not depend on the layout of allocated memory.
 *
 * This ignores any live range with no category, so such a live range does not
 * get allocated a register. GenXArgIndirection uses that to stop an indirected
 * argument uselessly getting a register.
 */
void GenXVisaRegAlloc::getLiveRanges(std::vector<LiveRange *> &LRs) const {
  // create LRs for global variables.
  for (auto &GV : FG->getModule()->globals())
    getLiveRangesForValue(&GV, LRs);
  for (auto fgi = FG->begin(), fge = FG->end(); fgi != fge; ++fgi) {
    Function *F = *fgi;
    for (auto ai = F->arg_begin(), ae = F->arg_end(); ai != ae; ++ai)
      getLiveRangesForValue(&*ai, LRs);
    if (fgi != FG->begin() && !F->getReturnType()->isVoidTy()) {
      // allocate reg for unified return value
      getLiveRangesForValue(Liveness->getUnifiedRet(F), LRs);
    }
    for (auto fi = F->begin(), fe = F->end(); fi != fe; ++fi) {
      BasicBlock *BB = &*fi;
      for (auto bi = BB->begin(), be = BB->end(); bi != be; ++bi)
        getLiveRangesForValue(&*bi, LRs);
    }
  }
  for (auto *LR : LRs)
    LR->prepareFuncs(FGA);
}

namespace {
// Helper class for live range localization of
// possible accumulator usages: finalizer can't
// assign accumulator to VISA variable if it's used
// outside of a BB. Localize such live ranges if profitable.
class AccumulatorUsageLocalizer {
public:
  AccumulatorUsageLocalizer(const GenXNumbering &N, GenXLiveness &L)
      : Numbering(N), Liveness(L) {}

  template <typename OutIter>
  void getSplitForLiveRange(genx::LiveRange *LR, OutIter Iter);

private:
  template <typename Iter>
  std::pair<Iter, Iter>
  getRangeOfInstructionsToLocalize(Iter InstBegin, Iter InstEnd, BasicBlock *BB,
                                   genx::LiveRange *LR) const;

  template <typename Iter>
  std::pair<Iter, Iter> getArithmeticChainRange(Iter InstBegin,
                                                Iter InstEnd) const;

  static bool isArithmeticChainInst(const Value *V);

  const GenXNumbering &Numbering;
  GenXLiveness &Liveness;
};
} // namespace

/***********************************************************************
 * Finalizer can't assign accumulator to VISA variable if it's used
 * outside of a BB. Localize such live ranges if profitable.
 */
void GenXVisaRegAlloc::localizeLiveRangesForAccUsage(
    std::vector<genx::LiveRange *> &LRs) {
  std::vector<genx::LiveRange *> NewLRs;
  AccumulatorUsageLocalizer Localizer(*Numbering, *Liveness);

  for (auto *LR : LRs) {
    // Only general variable can be an accumulator
    if (LR->Category != RegCategory::GENERAL)
      continue;
    Localizer.getSplitForLiveRange(LR, std::back_inserter(NewLRs));
  }

  std::for_each(NewLRs.begin(), NewLRs.end(),
                [this](genx::LiveRange *LR) { return LR->prepareFuncs(FGA); });
  LRs.insert(LRs.end(), std::make_move_iterator(NewLRs.begin()),
             std::make_move_iterator(NewLRs.end()));
}

/***********************************************************************
 * splitLiveRangeForAccUsage : Split live range into parts by moving
 *  certain range of instructions into new live range. This method modifies
 *  Liveness
 *
 * New live ranges are put into output itertaor if
 * split needed.
 */
template <typename OutIter>
void AccumulatorUsageLocalizer::getSplitForLiveRange(genx::LiveRange *LR,
                                                     OutIter Iter) {
  IGC_ASSERT(LR && LR->Category == RegCategory::GENERAL);

  // Map all LR's values to each BB. Provide iteration in
  // insertion order to preserve deterministic behaviour
  MapVector<BasicBlock *, std::vector<Instruction *>> LRValuesForBB;
  for (auto &SV : LR->getValues()) {
    auto *I = dyn_cast<Instruction>(SV.getValue());
    if (I) LRValuesForBB[I->getParent()].push_back(I);
  }

  for (auto &[BB, Insts] : LRValuesForBB) {
    // Sort values in this BB according to GenXNumbering to
    // identify bottom and up instructions
    std::sort(Insts.begin(), Insts.end(),
              [this](Instruction *I1, Instruction *I2) {
                return Numbering.getNumber(I1) < Numbering.getNumber(I2);
              });

    auto [Start, End] =
        getRangeOfInstructionsToLocalize(Insts.begin(), Insts.end(), BB, LR);
    if (Start == End)
      continue;

    // Insert values to localize into new
    // live range and remove them from the old one
    Liveness.removeValueNoDelete(*Start);
    genx::LiveRange *NewLR = Liveness.getOrCreateLiveRange(*Start);
    NewLR->setCategory(RegCategory::GENERAL);

    for (auto *I : make_range(std::next(Start), End)) {
      Liveness.removeValueNoDelete(I);
      Liveness.setLiveRange(I, NewLR);
    }

    *Iter++ = NewLR;
  }
}

/***********************************************************************
 * getRangeOfInstructionsToLocalize : get the range of instructions to
 * localize if exists. This method analyses if live range spans across
 * basic block. Accepts basic block and instructions of the basic block
 * in the live range.
 *
 * Returns range if found localization range
 */
template <typename Iter>
std::pair<Iter, Iter>
AccumulatorUsageLocalizer::getRangeOfInstructionsToLocalize(
    Iter InstBegin, Iter InstEnd, BasicBlock *BB, genx::LiveRange *LR) const {
  // Determine if bottom value is used outside of BB. Also localization is
  // needed if there exist a backedge phi user
  Instruction *BottomI = *std::prev(InstEnd);
  bool BottomValueLeaksAway = llvm::any_of(BottomI->users(), [&](User *U) {
    return cast<Instruction>(U)->getParent() != BottomI->getParent()
               ? true
               : isa<PHINode>(U);
  });
  // Localization is needed if any of the operands of first instruction is used
  // in the same LR and take place in different BB
  Instruction *UpI = *InstBegin;
  bool UpValueOperandsLeakIn = llvm::any_of(UpI->operands(), [&](Value *V) {
    auto *I = dyn_cast<Instruction>(V);
    return I && Liveness.getLiveRangeOrNull(I) == LR &&
           I->getParent() != UpI->getParent();
  });

  if (!BottomValueLeaksAway && !UpValueOperandsLeakIn)
    return {InstBegin, InstBegin};

  // Now do only localization of arithmetic instruction chains
  return getArithmeticChainRange(InstBegin, InstEnd);
}

/***********************************************************************
 * getArithmeticChainRange : get the range of arithmetic instructions:
 *      V = fma ...
 *      V = fma V ...
 *      V = fma V ...
 *      ...
 *
 * Returns range if found such chain or empty range otherwise
 */
template <typename Iter>
std::pair<Iter, Iter>
AccumulatorUsageLocalizer::getArithmeticChainRange(Iter InstBegin,
                                                   Iter InstEnd) const {
  unsigned TotalArithChainInsts =
      std::count_if(InstBegin, InstEnd, isArithmeticChainInst);
  if (ArithChainLengthThreshold > TotalArithChainInsts)
    return {InstBegin, InstBegin};

  auto InstRange = llvm::make_range(InstBegin, InstEnd);
  auto StartOfLocalizeSequence =
      llvm::find_if(InstRange, isArithmeticChainInst);
  auto EndOfLocalizeSequence =
      llvm::find_if(llvm::reverse(InstRange), isArithmeticChainInst);

  // Return element before the end of the range to not insert any copy-outs
  return std::make_pair(StartOfLocalizeSequence,
                        std::next(EndOfLocalizeSequence).base());
}

bool AccumulatorUsageLocalizer::isArithmeticChainInst(const Value *V) {
  unsigned ID = GenXIntrinsic::getAnyIntrinsicID(V);
  switch (ID) {
  case Intrinsic::fma:
    return true;
  default:
    break;
  }
  return false;
}

void GenXVisaRegAlloc::getLiveRangesForValue(
    Value *V, std::vector<LiveRange *> &LRs) const {
  auto Ty = V->getType();
  for (unsigned i = 0, e = IndexFlattener::getNumElements(Ty);
      i != e; ++i) {
    SimpleValue SV(V, i);
    LiveRange *LR = Liveness->getLiveRangeOrNull(SV);
    if (!LR || LR->getCategory() == RegCategory::NONE)
      continue;
    // Only process an LR if the map iterator is on the value that appears
    // first in the LR. That avoids processing the same LR multiple times.
    if (SV != *LR->value_begin())
      continue;
    LRs.push_back(LR);
  }
}

// Coalesce surface registers to avoid state registers limit violation.
// Args:
//    \p ToCoalesce - potential instruction to coalesce, must have surface
//                    category.
//    \p CommonLR - single LR used for constant surfaces, equals null if not
//                  yet selected.
//    \p Liveness - GenX liveness analysis.
// Returns old common LR value if \p ToCoalesce wasn't coalesced, updated common
// LR value otherwise.
static LiveRange *coalesceConstSurface(Instruction &ToCoalesce,
                                       LiveRange *CommonLR,
                                       GenXLiveness &Liveness) {
  auto *LR = Liveness.getLiveRange(&ToCoalesce);
  IGC_ASSERT_MESSAGE(LR->Category == RegCategory::SURFACE,
                     "wrong argument: ToCoalesce should have surface category");
  auto IID = GenXIntrinsic::getGenXIntrinsicID(&ToCoalesce);
  if (IID != GenXIntrinsic::genx_convert &&
      IID != GenXIntrinsic::genx_constanti)
    return CommonLR;
  if (!isa<Constant>(ToCoalesce.getOperand(0)))
    return CommonLR;
  if (!CommonLR)
    return LR;
  if (Liveness.interfere(CommonLR, LR))
    return CommonLR;
  return Liveness.coalesce(CommonLR, LR, /*DisalowCASC=*/true);
}

/***********************************************************************
 * extraCoalescing : do some extra coalescing over and above what
 *    GenXCoalescing does
 *
 * GenXCoalescing does coalescing where it saves a copy, for example for
 * a two address operand. This function does coalescing that does not save
 * a copy, but the two live ranges are related by being the operand (a
 * kill use) and result of the same instruction. This is in the hope that
 * the jitter's register allocator will be able to do a better job with it.
 *
 * A further case of extra coalescing is that multiple instances of a constant
 * load of a surface variable are coalesced together. This allows the CM code
 * to use lots of printfs without running out of surface variables.
 */
void GenXVisaRegAlloc::extraCoalescing()
{
  LiveRange *CommonSurface = nullptr;
  for (auto fgi = FG->begin(), fge = FG->end(); fgi != fge; ++fgi) {
    Function *F = *fgi;
    for (auto fi = F->begin(), fe = F->end(); fi != fe; ++fi) {
      BasicBlock *BB = &*fi;
      for (auto bi = BB->begin(), be = BB->end(); bi != be; ++bi) {
        auto Inst = &*bi;
        if (isa<StructType>(Inst->getType()))
          continue;
        if (GenXIntrinsic::isWrRegion(Inst))
          continue;
        auto LR = Liveness->getLiveRangeOrNull(Inst);
        if (!LR)
          continue;
        if (LR->Category == RegCategory::SURFACE) {
          CommonSurface = coalesceConstSurface(*Inst, CommonSurface, *Liveness);
          continue;
        }
        if (LR->Category != RegCategory::GENERAL)
          continue;
        // We have a non-struct non-wrregion instruction whose result has a
        // live range (it is not baled into anything else).
        // Check all uses to see if there is one in a non-alu intrinsic. We
        // don't want to coalesce that, because of the danger of the jitter
        // needing to add an extra move in the send.
        bool UseInNonAluIntrinsic = false;
        for (auto ui = Inst->use_begin(), ue = Inst->use_end();
            ui != ue && !UseInNonAluIntrinsic; ++ui) {
          auto user = dyn_cast<Instruction>(ui->getUser());
          IGC_ASSERT(user);
          if (user->getType()->isVoidTy()) {
            UseInNonAluIntrinsic = true;
            break;
          }
          unsigned IID = GenXIntrinsic::getAnyIntrinsicID(user);
          switch (IID) {
            case GenXIntrinsic::not_any_intrinsic:
            case GenXIntrinsic::genx_rdregioni:
            case GenXIntrinsic::genx_rdregionf:
            case GenXIntrinsic::genx_wrregioni:
            case GenXIntrinsic::genx_wrregionf:
              break;
            default: {
                // It is an intrinsic. A non-alu intrinsic does not have a
                // return value that is general.
                GenXIntrinsicInfo II(IID);
                if (!II.getRetInfo().isGeneral())
                  UseInNonAluIntrinsic = true;
              }
              break;
          }
        }
        if (UseInNonAluIntrinsic)
          continue;

        // Do not coalesce when this is a two address instrinsic with undef
        // input. Otherwise logic is broken on lifetime marker in vISA emission.
        //
        auto skipTwoAddrCoalesce = [](Instruction *Inst) {
          unsigned IntrinsicID = GenXIntrinsic::getAnyIntrinsicID(Inst);
          if (IntrinsicID == GenXIntrinsic::not_any_intrinsic)
            return false;
          GenXIntrinsicInfo Info(IntrinsicID);
          const auto *descp = Info.getInstDesc();
          for (const auto *p = descp; *p; ++p) {
            GenXIntrinsicInfo::ArgInfo AI(*p);
            if (AI.getCategory() != GenXIntrinsicInfo::TWOADDR)
              continue;
            if (isa<UndefValue>(Inst->getOperand(AI.getArgIdx())))
              return true;
          }
          return false;
        };
        if (skipTwoAddrCoalesce(Inst))
          continue;

        // See if we can coalesce with any operand.
        for (unsigned oi = 0, oe = Inst->getNumOperands(); oi != oe; ++oi) {
          Value *Operand = Inst->getOperand(oi);
          if (isa<Constant>(Operand))
            continue;
          if (Operand->getType() != Inst->getType())
            continue;
          // Do not coalesce with kernel arguments as they are input variables.
          if (FG->getHead() == F && isa<Argument>(Operand))
            continue;
          auto OperandLR = Liveness->getLiveRangeOrNull(Operand);
          if (!OperandLR || OperandLR->Category != RegCategory::GENERAL)
            continue;
          if (Liveness->interfere(LR, OperandLR))
            continue;
          // The two live ranges do not interfere, so we can coalesce them.
          if (++CoalescingCount > LimitGenXExtraCoalescing)
            continue;
          if (LimitGenXExtraCoalescing != UINT_MAX)
            dbgs() << "genx extra coalescing " << CoalescingCount << "\n";
          Liveness->coalesce(LR, OperandLR, /*DisalowCASC=*/true);
          break;
        }
      }
    }
  }
}

/***********************************************************************
 * allocReg : allocate a register for a LiveRange
 */
void GenXVisaRegAlloc::allocReg(LiveRange *LR) {
  if (LR->value_empty())
    return;
  if (LR->getCategory() >= RegCategory::NUMREALCATEGORIES)
    return; // don't allocate register to EM or RM value
  LLVM_DEBUG(dbgs() << "Allocating "; LR->print(dbgs()); dbgs() << "\n");
  SimpleValue V = *LR->value_begin();
  Type *Ty = V.getType();
  if (auto GV = dyn_cast<GlobalVariable>(V.getValue()))
    if (GV->hasAttribute(genx::FunctionMD::GenXVolatile))
      Ty = Ty->getPointerElementType();
  IGC_ASSERT(!Ty->isVoidTy());
  if (LR->Category == RegCategory::PREDICATE) {
    VectorType *VT = dyn_cast<VectorType>(Ty);
    IGC_ASSERT_MESSAGE((!VT || genx::exactLog2(VT->getNumElements()) >= 0),
      "invalid predicate width");
    (void)VT;
  }
  // Allocate the register, also setting the alignment.
  Reg *NewReg =
      createReg(LR->Category, Ty, DONTCARESIGNED, LR->getLogAlignment());
  // Assign to the values. If any value is an input arg, ensure the register
  // gets its type, to avoid needing an alias for an input arg.
  for (LiveRange::value_iterator vi = LR->value_begin(), ve = LR->value_end();
       vi != ve; ++vi) {
    for (auto &F : LR->Funcs) {
      if (FGA->getGroup(F) == FG) {
        IGC_ASSERT(RegMap.count(F) > 0);
        LLVM_DEBUG(dbgs() << "Allocating reg " << NewReg->Num << " for "
                          << *(vi->getValue()) << " in func " << F->getName()
                          << "\n");
        IGC_ASSERT(RegMap.at(F).find(*vi) == RegMap.at(F).end());
        RegMap.at(F)[*vi] = NewReg;
      }
    }
    if (isa<Argument>(vi->getValue()))
      NewReg->Ty = vi->getType();
  }
}

/***********************************************************************
 * getRegForValueUntyped : get the vISA reg allocated to a particular
 *    value, ignoring signedness and type
 *
 * This is a const method so it can be called from print().
 */
GenXVisaRegAlloc::Reg* GenXVisaRegAlloc::getRegForValueUntyped(const Function *kernel,
    SimpleValue V) const
{
  LLVM_DEBUG(dbgs() << "getRegForValueUntyped " << *(V.getValue()) << "\n");
  // is possible if called for GenXPrinter
  if (RegMap.count(kernel) == 0)
    return nullptr;
  auto& KernMap = RegMap.at(kernel);
  KernRegMap_t::const_iterator i = KernMap.find(V);
  if (i == KernMap.end()) {
    // Check if it's predefined variables.
    if (GenXIntrinsic::getGenXIntrinsicID(V.getValue()) ==
        GenXIntrinsic::genx_predefined_surface) {
      auto CI = cast<CallInst>(V.getValue());
      unsigned Id = cast<ConstantInt>(CI->getArgOperand(0))->getZExtValue();
      IGC_ASSERT_MESSAGE(Id < 4, "Invalid predefined surface ID!");
      IGC_ASSERT_MESSAGE(
          PredefinedSurfaceRegs.size() == VISA_NUM_RESERVED_SURFACES,
          "Predefined surface registers have not been initialized");
      return PredefinedSurfaceRegs[Id];
    } else if (GenXIntrinsic::isReadWritePredefReg(V.getValue())) {
      auto CI = cast<CallInst>(V.getValue());
      return PredefinedRegs[cast<ConstantInt>(CI->getArgOperand(0))
                                ->getZExtValue() -
                            PreDefined_Vars::PREDEFINED_ARG];
    }
    return nullptr;
  }
  return i->second;
}

/***********************************************************************
 * getRegForValueOrNull : get the vISA reg allocated to a particular Value
 *
 * Enter:   V = value (Argument or Instruction) to get register for
 *          Signed = request for signed or unsigned
 *          OverrideType = 0 else override type of value (used for bitcast)
 *
 * Called from GenXCisaBuilder to get the register for an
 * operand. The operand type might not match the register type (say a
 * bitcast has been coalesced, or the same integer value is used
 * unsigned in one place and signed in another), in which case we
 * find/create a vISA register alias.
 */
GenXVisaRegAlloc::Reg* GenXVisaRegAlloc::getRegForValueOrNull(
    const Function* kernel, SimpleValue V, Signedness Signed, Type *OverrideType)
{
  LLVM_DEBUG(dbgs() << "getRegForValueOrNull " << *(V.getValue()) << "\n");
  Reg *R = getRegForValueUntyped(kernel, V);
  if (!R)
    return nullptr; // no register allocated
  if (R->Category != RegCategory::GENERAL)
    return R;
  LLVM_DEBUG(dbgs() << "Found reg " << R->Num << "\n");

  if (!OverrideType)
    OverrideType = V.getType();
  if (OverrideType->isPointerTy()) {
    auto GV = dyn_cast<GlobalVariable>(V.getValue());
    if (GV && GV->hasAttribute(genx::FunctionMD::GenXVolatile))
      OverrideType = OverrideType->getPointerElementType();
  }
  OverrideType = &vc::fixDegenerateVectorType(*OverrideType);
  auto &DL = kernel->getParent()->getDataLayout();
  if (R->Num < VISA_NUM_RESERVED_REGS) {
    OverrideType = IGCLLVM::FixedVectorType::get(
        OverrideType->getScalarType(),
        R->Ty->getPrimitiveSizeInBits() /
            DL.getTypeSizeInBits(OverrideType->getScalarType()));
  }

  Reg *LastAlias = R;
  // std::find_if
  for (Reg *CurAlias = R; CurAlias; CurAlias = CurAlias->NextAlias[kernel]) {
    LastAlias = CurAlias;
    Type *ExistingType = CurAlias->Ty;
    ExistingType = &vc::fixDegenerateVectorType(*ExistingType);
    if (ExistingType == OverrideType &&
        CurAlias->Num >= VISA_NUM_RESERVED_REGS &&
        (CurAlias->Signed == Signed || Signed == DONTCARESIGNED)) {
      LLVM_DEBUG(dbgs() << "Using alias: "; CurAlias->print(dbgs()); dbgs() << "\n");
      return CurAlias;
    }
  }
  // Run out of aliases. Add a new one.
  Reg *NewReg = createReg(RegCategory::GENERAL, OverrideType, Signed, 0, R);
  LastAlias->NextAlias[kernel] = NewReg;
  LLVM_DEBUG(dbgs() << "New register: "; NewReg->print(dbgs()); dbgs() << "\n");
  return NewReg;
}

/***********************************************************************
 * getSigned : get the signedness of a register
 *
 * If the register has byte type and is currently don't care signedness, this
 * arbitrarily picks unsigned. We do that because having a byte mov with
 * different signedness between source and destination can make the jitter
 * generate less efficient code.
 */
genx::Signedness GenXVisaRegAlloc::getSigned(Reg* R)
{
  return (R && R->Category == RegCategory::GENERAL) ?
      R->Signed : DONTCARESIGNED;
}

// addRetIPArgument : Add the RetIP argument required for caller kernels and
// their caller.
void GenXVisaRegAlloc::addRetIPArgument() {
  RetIP = createReg(RegCategory::GENERAL, Type::getInt64Ty(FG->getContext()));
}

/***********************************************************************
 * TypeDetails constructor
 *
 * Enter:   Ty = LLVM type
 *          Signedness = whether signed type required
 */
TypeDetails::TypeDetails(const DataLayout &DL, Type *Ty, Signedness Signed)
    : DL(DL) {
  Type *ElementTy = Ty;
  NumElements = 1;
  if (VectorType *VT = dyn_cast<VectorType>(ElementTy)) {
    ElementTy = VT->getElementType();
    NumElements = VT->getNumElements();
  }
  if (IntegerType *IT = dyn_cast<IntegerType>(ElementTy)) {
    BytesPerElement = IT->getBitWidth() / 8;
    if (Signed == UNSIGNED) {
      switch (BytesPerElement) {
        case 1: VisaType = ISA_TYPE_UB; break;
        case 2: VisaType = ISA_TYPE_UW; break;
        case 4: VisaType = ISA_TYPE_UD; break;
        default: VisaType = ISA_TYPE_UQ; break;
      }
    } else {
      switch (BytesPerElement) {
        case 1: VisaType = ISA_TYPE_B; break;
        case 2: VisaType = ISA_TYPE_W; break;
        case 4: VisaType = ISA_TYPE_D; break;
        default: VisaType = ISA_TYPE_Q; break;
      }
    }
  } else if (ElementTy->isHalfTy()) {
    VisaType = ISA_TYPE_HF;
    BytesPerElement = 2;
  } else if (ElementTy->isFloatTy()) {
    VisaType = ISA_TYPE_F;
    BytesPerElement = 4;
  } else if (auto PT = dyn_cast<PointerType>(ElementTy)) {
    BytesPerElement = DL.getPointerTypeSize(PT);
    if (BytesPerElement == 4 || PT->getPointerElementType()->isFunctionTy())
      VisaType = ISA_TYPE_UD;
    else if (BytesPerElement == 8)
      VisaType = ISA_TYPE_UQ;
    else
      report_fatal_error("unsupported pointer type size");
  } else {
    IGC_ASSERT(ElementTy->isDoubleTy());
    VisaType = ISA_TYPE_DF;
    BytesPerElement = 8;
  }
  if (NumElements > 16384 || NumElements * BytesPerElement > 16384 * 8)
    report_fatal_error("Variable too big");
}


/***********************************************************************
 * print : dump the state of the pass. This is used by -genx-dump-regalloc
 */
void GenXVisaRegAlloc::print(raw_ostream &OS, const Module *M) const
{
  // Get the live ranges in a reproducible order, and sort them by "length"
  // (the total number of instructions that the live range covers).
  struct LiveRangeAndLength {
    LiveRange *LR;
    unsigned Length;
    LiveRangeAndLength(LiveRange *LR, unsigned Length) : LR(LR), Length(Length) {}
    bool operator<(const LiveRangeAndLength &Rhs) const { return Length > Rhs.Length; }
  };
  std::vector<LiveRange *> LRs;
  getLiveRanges(LRs);
  std::vector<LiveRangeAndLength> LRLs;
  for (auto i = LRs.begin(), e = LRs.end(); i != e; ++i)
    LRLs.push_back(LiveRangeAndLength(*i, (*i)->getLength(/*WithWeak=*/ false)));
  LRs.clear();
  std::sort(LRLs.begin(), LRLs.end());
  // Dump them. Also keep count of the register pressure at each
  // instruction number.
  std::vector<unsigned> Pressure;
  std::vector<unsigned> FlagPressure;
  for (auto i = LRLs.begin(), e = LRLs.end(); i != e; ++i) {
    // Dump a single live range.
    LiveRange *LR = i->LR;
    SimpleValue SV = *LR->value_begin();
    Reg* RN = getRegForValueUntyped(&(*(M->begin())), SV);
    IGC_ASSERT(RN);
    OS << "[";
    RN->print(OS);
    Type *ElTy = IndexFlattener::getElementType(SV.getValue()->getType(),
          SV.getIndex());
    unsigned Bytes = getTypeSize<WordBits>(ElTy) * WordBytes;
    bool IsFlag = ElTy->getScalarType()->isIntegerTy(1);
    OS << "] (" << Bytes << " bytes, length " << i->Length <<") ";
    // Dump some indication of what the live range is. For a kernel argument,
    // show its name. For an instruction with debug info, show the location.
    // We try and find the earliest definition with debug info to show.
    unsigned BestNum = UINT_MAX;
    Instruction *BestInst = nullptr;
    Argument *KernelArg = nullptr;
    for (auto i = LR->value_begin(), e = LR->value_end(); i != e; ++i) {
      Value *V = i->getValue();
      if (auto Arg = dyn_cast<Argument>(V)) {
        if (Arg->getParent() == FG->getHead()) {
          KernelArg = Arg;
          break;
        }
      } else {
        auto Inst = cast<Instruction>(V);
        if (!isa<PHINode>(Inst)) {
          unsigned Num = Numbering->getNumber(Inst);
          if (Num < BestNum) {
            auto DL = Inst->getDebugLoc();
            if (!DL) {
              BestNum = Num;
              BestInst = Inst;
            }
          }
        }
      }
    }
    if (KernelArg)
      OS << KernelArg->getName();
    else if (BestInst) {
      const DebugLoc &DL = BestInst->getDebugLoc();
      OS << DL->getFilename() << ":" << DL.getLine();
    }
    // Dump the live range segments, and add each to the pressure score.
    OS << ":";
    LR->printSegments(OS);
    for (auto si = LR->begin(), se = LR->end(); si != se; ++si) {
      if (si->getEnd() >= Pressure.size()) {
        Pressure.resize(si->getEnd() + 1, 0);
        FlagPressure.resize(si->getEnd() + 1, 0);
      }
      for (unsigned n = si->getStart(); n != si->getEnd(); ++n) {
        Pressure[n] += Bytes;
        if (IsFlag)
          FlagPressure[n] += Bytes;
      }
    }
    OS << "\n";
  }
  OS << "\n";
  // Prepare to print register pressure info. First we need to compute a
  // mapping from instruction number to instruction. Only bother with
  // instructions with debug info.
  std::vector<Instruction *> Insts;
  for (auto fgi = FG->begin(), fge = FG->end(); fgi != fge; ++fgi) {
    Function *F = *fgi;
    for (auto fi = F->begin(), fe = F->end(); fi != fe; ++fi) {
      BasicBlock *BB = &*fi;
      for (auto bi = BB->begin(), be = BB->end(); bi != be; ++bi) {
        Instruction *Inst = &*bi;
        if (!Inst->getDebugLoc()) {
          unsigned Num = Numbering->getNumber(Inst);
          if (Num >= Insts.size())
            Insts.resize(Num + 1, nullptr);
          Insts[Num] = Inst;
        }
      }
    }
  }
  OS << "Register pressure (bytes):\n";
  unsigned Last = 0;
  bool HadInst = false;
  Function *LastFunc = nullptr;
  for (unsigned n = 0; n != Pressure.size(); ++n) {
    if (Pressure[n]) {
      Instruction *Inst = nullptr;
      if (n < Insts.size())
        Inst = Insts[n];
      if (Pressure[n] != Last)
        HadInst = false;
      if (Pressure[n] != Last || (!HadInst && Inst)) {
        if (Inst && Inst->getParent()->getParent() != LastFunc) {
          LastFunc = Inst->getParent()->getParent();
          OS << "In " << LastFunc->getName() << "\n";
        }
        Last = Pressure[n];
        OS << Pressure[n] << " at " << n;
        if (Inst) {
          HadInst = true;
          OS << " ";
          const DebugLoc &DL = Inst->getDebugLoc();
          DL.print(OS);
        }
        OS << "\n";
      }
    }
  }
  OS << "Flag pressure (bytes):\n";
  Last = 0;
  HadInst = false;
  for (unsigned n = 0; n != FlagPressure.size(); ++n) {
    Instruction *Inst = nullptr;
    if (n < Insts.size())
      Inst = Insts[n];
    if (FlagPressure[n] != Last)
      HadInst = false;
    if (FlagPressure[n] != Last || (!HadInst && Inst)) {
      Last = FlagPressure[n];
      OS << FlagPressure[n] << " at " << n;
      if (Inst) {
        HadInst = true;
        const DebugLoc &DL = Inst->getDebugLoc();
        OS << " " << DL->getFilename() << ":" << DL.getLine();
      }
      OS << "\n";
    }
  }
}

/***********************************************************************
 * RegNum::print : print a regnum
 */
void GenXVisaRegAlloc::Reg::print(raw_ostream &OS) const
{
  switch (Category) {
    case RegCategory::NONE: OS << "-"; return;
    case RegCategory::GENERAL: OS << "v"; break;
    case RegCategory::ADDRESS: OS << "a"; break;
    case RegCategory::PREDICATE: OS << "p"; break;
    case RegCategory::SAMPLER: OS << "s"; break;
    case RegCategory::SURFACE: OS << "t"; break;
    case RegCategory::VME: OS << "vme"; break;
    default: OS << "?"; break;
  }
  OS << Num;
}

