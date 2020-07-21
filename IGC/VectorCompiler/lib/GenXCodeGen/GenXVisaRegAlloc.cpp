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
#include "GenXUtil.h"
#include "visa_igc_common_header.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"

using namespace llvm;
using namespace genx;
using namespace visa;

static cl::opt<unsigned> LimitGenXExtraCoalescing("limit-genx-extra-coalescing", cl::init(UINT_MAX), cl::Hidden,
                                      cl::desc("Limit GenX extra coalescing."));


char GenXVisaRegAlloc::ID = 0;
INITIALIZE_PASS_BEGIN(GenXVisaRegAlloc, "GenXVisaRegAlloc", "GenXVisaRegAlloc", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXLiveness)
INITIALIZE_PASS_DEPENDENCY(GenXNumbering)
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
  // Empty out the analysis from the last function it was used on.
  RegMap.clear();
  RegStorage.clear();
  PredefinedSurfaceRegs.clear();
  for (unsigned i = 0; i != RegCategory::NUMREALCATEGORIES; ++i) {
    CurrentRegId[i] = 0;
  }
  for (unsigned i = 0; i < VISA_NUM_RESERVED_SURFACES; ++i) {
    RegStorage.emplace_back(RegCategory::SURFACE, i);
    PredefinedSurfaceRegs.push_back(&RegStorage.back());
  }
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
  getLiveRanges(&LRs);
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
void GenXVisaRegAlloc::getLiveRanges(std::vector<LiveRange *> *LRs) const
{
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
  for (auto &LR : *LRs)
    LR->prepareFuncs(FGA);
}

void GenXVisaRegAlloc::getLiveRangesForValue(Value *V,
    std::vector<LiveRange *> *LRs) const
{
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
    LRs->push_back(LR);
  }
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
        if (!LR || LR->Category != RegCategory::GENERAL)
          continue;
        // Check for convert of constant ot surface.
        switch (GenXIntrinsic::getGenXIntrinsicID(Inst)) {
          case GenXIntrinsic::genx_convert:
          case GenXIntrinsic::genx_constanti:
            if (LR->Category == RegCategory::SURFACE
                && isa<Constant>(Inst->getOperand(0))) {
              // See if we can coalesce it with CommonSurface.
              if (!CommonSurface)
                CommonSurface = LR;
              else if (!Liveness->interfere(CommonSurface, LR))
                CommonSurface = Liveness->coalesce(CommonSurface, LR, /*DisalowCASC=*/true);
            }
            break;
          default:
            break;
        }
        // We have a non-struct non-wrregion instruction whose result has a
        // live range (it is not baled into anything else).
        // Check all uses to see if there is one in a non-alu intrinsic. We
        // don't want to coalesce that, because of the danger of the jitter
        // needing to add an extra move in the send.
        bool UseInNonAluIntrinsic = false;
        for (auto ui = Inst->use_begin(), ue = Inst->use_end();
            ui != ue && !UseInNonAluIntrinsic; ++ui) {
          auto user = dyn_cast<Instruction>(ui->getUser());
          assert(user);
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
void GenXVisaRegAlloc::allocReg(LiveRange *LR)
{
  if (LR->value_empty())
    return;
  if (LR->getCategory() >= RegCategory::NUMREALCATEGORIES)
    return; // don't allocate register to EM or RM value
  LLVM_DEBUG(
    dbgs() << "Allocating ";
    LR->print(dbgs());
    dbgs() << "\n"
  );
  SimpleValue V = *LR->value_begin();
  Type *Ty = V.getType();
  if (auto GV = dyn_cast<GlobalVariable>(V.getValue()))
    if (GV->hasAttribute(genx::FunctionMD::GenXVolatile))
      Ty = Ty->getPointerElementType();
  assert(!Ty->isVoidTy());
  if (LR->Category == RegCategory::PREDICATE) {
    VectorType *VT = dyn_cast<VectorType>(Ty);
    assert((!VT || genx::exactLog2(VT->getNumElements()) >= 0) && "invalid predicate width");
    (void)VT;
  }
  // Allocate the register, also setting the alignment.
  // Assign to the values. If any value is an input arg, ensure the register
  // gets its type, to avoid needing an alias for an input arg.
  for (auto &F : LR->Funcs) {
    Reg *NewReg =
        createReg(LR->Category, Ty, DONTCARESIGNED, LR->getLogAlignment());
    if (RegMap.count(F) > 0) {
      for (LiveRange::value_iterator vi = LR->value_begin(),
                                     ve = LR->value_end();
           vi != ve; ++vi) {
        LLVM_DEBUG(dbgs() << "Allocating reg " << NewReg->Num << " to "
                          << *(vi->getValue()) << " in func " << F->getName()
                          << "\n";);
        assert(RegMap.at(F).find(*vi) == RegMap.at(F).end());
        RegMap.at(F)[*vi] = NewReg;
        if (isa<Argument>(vi->getValue()))
          NewReg->Ty = vi->getType();
      }
    }
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
  // is possible if called for GenXPrinter
  if (RegMap.count(kernel) == 0)
    return nullptr;
  auto& KernMap = RegMap.at(kernel);
  KernRegMap_t::const_iterator i = KernMap.find(V);
  if (i == KernMap.end()) {
    // Check if it's predefined variables.
    if (GenXIntrinsic::getGenXIntrinsicID(V.getValue()) != GenXIntrinsic::genx_predefined_surface)
      return nullptr;
    auto CI = cast<CallInst>(V.getValue());
    unsigned Id = cast<ConstantInt>(CI->getArgOperand(0))->getZExtValue();
    assert(Id < 4 && "Invalid predefined surface ID!");
    assert(PredefinedSurfaceRegs.size() == VISA_NUM_RESERVED_SURFACES &&
        "Predefined surface registers have not been initialized");
    return PredefinedSurfaceRegs[Id];
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
 * Called from GenXVisaFunctionWriter to get the register for an
 * operand. The operand type might not match the register type (say a
 * bitcast has been coalesced, or the same integer value is used
 * unsigned in one place and signed in another), in which case we
 * find/create a vISA register alias.
 */
GenXVisaRegAlloc::Reg* GenXVisaRegAlloc::getRegForValueOrNull(
    const Function* kernel, SimpleValue V, Signedness Signed, Type *OverrideType)
{
  Reg *R = getRegForValueUntyped(kernel, V);
  if (!R)
    return nullptr; // no register allocated
  if (R->Category != RegCategory::GENERAL)
    return R;

  if (!OverrideType)
    OverrideType = V.getType();
  if (OverrideType->isPointerTy()) {
    auto GV = dyn_cast<GlobalVariable>(V.getValue());
    if (GV && GV->hasAttribute(genx::FunctionMD::GenXVolatile))
      OverrideType = OverrideType->getPointerElementType();
  }
  OverrideType = &fixDegenerateVectorType(*OverrideType);

  Reg *LastAlias = R;
  // std::find_if
  for (Reg *CurAlias = R; CurAlias; CurAlias = CurAlias->NextAlias[kernel]) {
    LastAlias = CurAlias;
    Type *ExistingType = CurAlias->Ty;
    ExistingType = &fixDegenerateVectorType(*ExistingType);
    if (ExistingType == OverrideType &&
        (CurAlias->Signed == Signed || Signed == DONTCARESIGNED))
      return CurAlias;
  }
  // Run out of aliases. Add a new one.
  Reg *NewReg = createReg(RegCategory::GENERAL, OverrideType, Signed, 0, R);
  LastAlias->NextAlias[kernel] = NewReg;
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
    if (BytesPerElement == 4)
      VisaType = ISA_TYPE_UD;
    else if (BytesPerElement == 8)
      VisaType = ISA_TYPE_UQ;
    else
      report_fatal_error("unsupported pointer type size");
  } else {
    assert(ElementTy->isDoubleTy());
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
  getLiveRanges(&LRs);
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
    assert(RN);
    OS << "[";
    RN->print(OS);
    Type *ElTy = IndexFlattener::getElementType(SV.getValue()->getType(),
          SV.getIndex());
    unsigned Bytes = (ElTy->getPrimitiveSizeInBits() + 15U) / 8U & -2U;
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

