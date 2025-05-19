/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// GenX instruction baling is analyzed by this pass. See GenXBaling.h for more
// detailed comment.
//
//===----------------------------------------------------------------------===//
#include "GenXBaling.h"
#include "GenXConstants.h"
#include "GenXIntrinsics.h"
#include "GenXLiveness.h"
#include "GenXUtil.h"

#include "vc/Utils/GenX/GlobalVariable.h"
#include "vc/Utils/GenX/IntrinsicsWrapper.h"

#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Utils/Local.h"

#include "llvmWrapper/Analysis/InstructionSimplify.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Type.h"

#include "Probe/Assertion.h"

#define DEBUG_TYPE "GENX_INSTRUCTION_BALING"

// Part of the bodge to allow abs to bale in to sext/zext. This needs to be set
// to some arbitrary value that does not clash with any
// GenXIntrinsicInfo::MODIFIER_* value.
enum { MODIFIER_ABSONLY = 9000 };

using namespace llvm;
using namespace genx;
using namespace GenXIntrinsic::GenXRegion;

// set of debug options to switch off different baling types
static cl::opt<bool> BaleBinary("bale-binary", cl::init(true), cl::Hidden,
                                cl::desc("Bale binary operators"));

static cl::opt<bool> BaleCmp("bale-cmp", cl::init(true), cl::Hidden,
                             cl::desc("Bale comparisons"));

static cl::opt<bool> BaleCast("bale-cast", cl::init(true), cl::Hidden,
                              cl::desc("Bale casts"));

static cl::opt<bool> BaleSelect("bale-select", cl::init(true), cl::Hidden,
                                cl::desc("Bale selects"));

static cl::opt<bool> BaleFNeg("bale-fneg", cl::init(true), cl::Hidden,
                              cl::desc("Bale fneg"));

static cl::opt<bool>
DisableMemOrderCheck("dbgonly-disable-mem-order-check", cl::init(false),
                     cl::Hidden, cl::desc("Disable checking of memory ordering"));

static cl::opt<bool> PrintBaling("print-baling-info", cl::init(false), cl::Hidden,
                              cl::desc("Print additional info after GenXBaling pass done"));

//----------------------------------------------------------------------
// Administrivia for GenXFuncBaling pass
//
char GenXFuncBaling::ID = 0;

INITIALIZE_PASS_BEGIN(GenXFuncBaling, "GenXFuncBaling", "GenXFuncBaling", false,
                      false)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_END(GenXFuncBaling, "GenXFuncBaling", "GenXFuncBaling", false,
                    false)

FunctionPass *llvm::createGenXFuncBalingPass(BalingKind Kind, GenXSubtarget *ST)
{
  initializeGenXFuncBalingPass(*PassRegistry::getPassRegistry());
  return new GenXFuncBaling(Kind, ST);
}

void GenXFuncBaling::getAnalysisUsage(AnalysisUsage &AU) const
{
  FunctionPass::getAnalysisUsage(AU);
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.setPreservesCFG();
}

//----------------------------------------------------------------------
// Administrivia for GenXGroupBaling pass
//
INITIALIZE_PASS_BEGIN(GenXGroupBalingWrapper, "GenXGroupBalingWrapper",
                      "GenXGroupBalingWrapper", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXLivenessWrapper)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeGroupWrapperPassWrapper)
INITIALIZE_PASS_END(GenXGroupBalingWrapper, "GenXGroupBalingWrapper",
                    "GenXGroupBalingWrapper", false, false)

ModulePass *llvm::createGenXGroupBalingWrapperPass(BalingKind Kind,
                                                   GenXSubtarget *ST) {
  initializeGenXGroupBalingWrapperPass(*PassRegistry::getPassRegistry());
  return new GenXGroupBalingWrapper(Kind, ST);
}

void GenXGroupBaling::getAnalysisUsage(AnalysisUsage &AU) {
  // FIXME: now use getAnalysisIfAvailable and error if nullptr
  // if (GenXBaling::Kind == BK_CodeGen)
  //  AU.addRequired<GenXLivenessWrapper>();
  AU.addRequired<DominatorTreeGroupWrapperPass>();
  AU.setPreservesCFG();
  AU.addPreserved<GenXModule>();
  AU.addPreserved<GenXLiveness>();
}

/***********************************************************************
 * GenXGroupBaling::runOnFunctionGroup : run second baling pass on function
 *    group
 */
bool GenXGroupBaling::runOnFunctionGroup(FunctionGroup &FG)
{
  Liveness = getAnalysisIfAvailable<GenXLiveness>();
  if (Kind == BK_CodeGen)
    IGC_ASSERT_MESSAGE(Liveness,
                       "Expected GenXLiveness analysis to be available.");
  return processFunctionGroup(FG);
}

/***********************************************************************
 * processFunctionGroup : run instruction baling analysis on one
 *  function group
 */
bool GenXGroupBaling::processFunctionGroup(FunctionGroup &FG) {
  bool Modified = false;
  for (auto &F : FG)
    Modified |= processFunction(
        *F, *getAnalysis<DominatorTreeGroupWrapperPass>().getDomTree(&*F));
  return Modified;
}

/***********************************************************************
 * processFunction : run instruction baling analysis on one function
 *
 * This does a preordered depth first traversal of the CFG to
 * ensure that we see a def before its uses (ignoring phi node uses).
 * This is required when we see a constant add/sub used as a region or
 * element variable index; if the add/sub has already been marked as
 * baling in a modifier or rdregion then we cannot bale it in to the
 * variable index region.
 *
 * This pass also clones any instruction that can be baled in but has
 * multiple uses. A baled in instruction must have exactly one use.
 */
bool GenXBaling::processFunction(Function &F, const DominatorTree &DTRef) {
  LLVM_DEBUG(llvm::dbgs() << "Baling function analysis for " << F.getName()
                          << "\n");
  DT = &DTRef;

  bool Changed = preBalingCleanAndOptimize(F);

  for (auto BBI : depth_first(&F))
    for (auto II = BBI->begin(); II != BBI->end();)
      processInst(&*II++);

  // Process any two addr sends we found.
  for (auto i = TwoAddrSends.begin(), e = TwoAddrSends.end(); i != e; ++i)
    processTwoAddrSend(*i);
  TwoAddrSends.clear();
  // Clone any instructions that we found in the pass that want to be baled in
  // but have more than one use.
  if (NeedCloneStack.size()) {
    doClones();
    Changed = true;
  }
  if (PrintBaling)
    GenXBaling::print(outs());
  return Changed;
}

/***********************************************************************
 * processInst : calculate baling for an instruction
 *
 * Usually this is called from runOnFunction above. However another pass
 * can call this to recalculate the baling for an instruction, particularly
 * for a new instruction it has just added. GenXLegalization does this.
 */
void GenXBaling::processInst(Instruction *Inst)
{
  unsigned IntrinID = vc::getAnyIntrinsicID(Inst);
  if (GenXIntrinsic::isWrRegion(IntrinID))
    processWrRegion(Inst);
  else if (IntrinID == GenXIntrinsic::genx_wrpredregion)
    processWrPredRegion(Inst);
  else if (IntrinID == GenXIntrinsic::genx_wrpredpredregion)
    processWrPredPredRegion(Inst);
  else if (IntrinID == GenXIntrinsic::genx_sat || GenXIntrinsic::isIntegerSat(IntrinID))
    processSat(Inst);
  else if (GenXIntrinsic::isReadWritePredefReg(IntrinID))
    processRdWrPredefReg(Inst);
  else if (GenXIntrinsic::isRdRegion(IntrinID))
    processRdRegion(Inst);
  else if (IntrinID == GenXIntrinsic::genx_rdpredregion)
    processRdPredRegion(Inst);
  else if (BranchInst *Branch = dyn_cast<BranchInst>(Inst))
    processBranch(Branch);
  else if (auto SI = dyn_cast<StoreInst>(Inst))
    processStore(SI);
  else if (isa<CallInst>(Inst) && cast<CallInst>(Inst)->isInlineAsm())
    processInlineAsm(Inst);
  else if(ExtractValueInst *EV = dyn_cast<ExtractValueInst>(Inst))
    processExtractValue(EV);
  else {
    // Try to bale a select into cmp's dst. If failed, continue to process
    // select as a main instruction.
    bool BaledSelect = processSelect(Inst);
    if (!BaledSelect)
      processMainInst(Inst, IntrinID);
  }
}

/***********************************************************************
 * isRegionOKForIntrinsic : check whether region is OK for an intrinsic arg
 *
 * Enter:   ArgInfoBits = mask for the ArgInfo for the intrinsic arg (or return value)
 *          R = region itself
 *
 * This checks that the arg is general (rather than raw) and does not have
 * any stride restrictions that are incompatible with the region.
 *
 * In the legalization pass of baling, we always return true when the main
 * instruction can be splitted. Otherwise, a region that would be OK after
 * being split by legalization might here appear not OK, and that would stop
 * legalization considering splitting it. However, if the main instruction
 * cannot be splitted, then we need to check the full restriction
 * otherwise, if the region is considered baled and skip legalization,
 * we may have illegal standalone read-region.
 */
bool GenXBaling::isRegionOKForIntrinsic(unsigned ArgInfoBits,
                                        const vc::Region &R,
                                        bool CanSplitBale) {
  GenXIntrinsicInfo::ArgInfo AI(ArgInfoBits);
  if (!AI.isGeneral())
    return false;
  if (Kind == BalingKind::BK_Legalization) {
    if (CanSplitBale)
      return true;
  }
  if (R.Indirect && (AI.isDirectOnly()))
    return false;
  unsigned Restriction = AI.getRestriction();
  if (!Restriction)
    return true;

  const unsigned GRFWidth = ST ? ST->getGRFByteSize() : defaultGRFByteSize;
  const auto Align = AI.getAlignment();
  const auto Log2Align = getLogAlignment(Align, GRFWidth * ByteBits);

  if (Log2Align > 0) {
    IGC_ASSERT_EXIT(Log2Align < 32);
    const auto ElementsPerAlign = (1 << Log2Align) / R.ElementBytes;

    if (R.Indirect) {
      // Instructions that cannot be splitted also cannot allow indirect
      if (!CanSplitBale)
        return false;
      Alignment AL = AlignInfo.get(R.Indirect);
      if (AL.getLogAlign() < Log2Align || AL.getExtraBits() != 0)
        return false;
    } else if ((R.Offset & (ElementsPerAlign - 1)) != 0)
      return false;

    if (R.is2D() && (R.VStride & (ElementsPerAlign - 1)) != 0)
      return false;
  }

  switch (Restriction) {
  case GenXIntrinsicInfo::SCALARORCONTIGUOUS:
    if (!R.Stride && R.Width == R.NumElements)
      break;
    // fall through...
  case GenXIntrinsicInfo::FIXED4:
  case GenXIntrinsicInfo::CONTIGUOUS:
    return R.isContiguous();
  case GenXIntrinsicInfo::STRIDE1:
    // For the dot product instructions, the vISA spec just says that the
    // horizontal stride must be 1. It doesn't say anything about the
    // width or the vertical stride. I am assuming that the width must also
    // be at least 4, since the operation works on groups of 4 channels.
    if (R.Stride != 1 || R.Width < 4)
      return false;
    break;
  case GenXIntrinsicInfo::ONLY_LEGAL_REGION:
    // Some instructions like umadw and smadw can't be splitted. It leads
    // to problem in legalization. We have to not bale incorrect region with
    // such instructions in order to split region independently.
    return R.Width == R.NumElements && R.Stride == 1;
  default:
    break;
  }
  return true;
}

/***********************************************************************
 * isSafeToMove : check if operation Op can be safely moved if it's placed
 * on position From on position To
 * If operation can write to memory, it's considered unsafe to sink it across
 * any memory-related operation
 * I operation is load-like, we don't sink it only through store-like operations
 */
bool GenXBaling::isSafeToMove(Instruction *Op, Instruction *From, Instruction *To) {
  if (!genx::isSafeToSink_CheckAVLoadKill(Op, To, this))
    return false;

  // Do not move cr0 reads
  if (GenXIntrinsic::isRdRegion(Op)) {
    auto *ReadPredef = dyn_cast<Instruction>(
        Op->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum));
    if (ReadPredef && GenXIntrinsic::isReadPredefReg(ReadPredef)) {
      uint32_t RegId =
          cast<ConstantInt>(ReadPredef->getOperand(0))->getZExtValue();
      if (RegId == PreDefined_Vars::PREDEFINED_CR0)
        return false;
    }
  }

  if (DisableMemOrderCheck || !Op->mayReadOrWriteMemory())
    return true;

  SmallVector<BasicBlock *, 4> Worklist;
  SmallPtrSet<BasicBlock *, 4> Visited;
  auto *FirstBB = From->getParent(),
       *LastBB = To->getParent();
  Worklist.push_back(LastBB);

  // Do DFS traversal from LastBB until FirstBB is not reached.
  while (!Worklist.empty()) {
    auto *CurBB = Worklist.pop_back_val();

    auto II = CurBB == FirstBB ? From->getNextNode()->getIterator() : CurBB->begin(),
         EI = CurBB == LastBB ? To->getIterator() : CurBB->end();

    for (; II != EI; ++II) {
      auto *CurInst = &*II;
      // Loads and stores to globals are not generated in any real memory operation
      // so can be safely skipped.
      if (genx::isGlobalLoad(CurInst) || genx::isGlobalStore(CurInst))
        continue;
      if (CurInst->mayWriteToMemory() ||
          (Op->mayWriteToMemory() && CurInst->mayReadFromMemory())) {
        LLVM_DEBUG(llvm::dbgs() << "Operation " << *Op << " can't be sinked through "
                                << *CurInst << "\n");
        return false;
      }
    }

    if (CurBB != FirstBB) {
      Visited.insert(CurBB);
      for (auto *PredBB : predecessors(CurBB))
        if (!Visited.count(PredBB))
          Worklist.push_back(PredBB);
    }
  }

  return true;
}

/***********************************************************************
 * canSplitBale : check if instruction can be splitted
 */
bool GenXBaling::canSplitBale(Instruction *Inst) const {
  auto IID = GenXIntrinsic::getGenXIntrinsicID(Inst);
  if ((IID == GenXIntrinsic::genx_dpas) || (IID == GenXIntrinsic::genx_dpas2) ||
      (IID == GenXIntrinsic::genx_dpasw) ||
      (IID == GenXIntrinsic::genx_dpas_nosrc0) ||
      (IID == GenXIntrinsic::genx_dpasw_nosrc0) ||
      (IID == GenXIntrinsic::genx_umadw) ||
      (IID == GenXIntrinsic::genx_smadw))
    return false;
  return true;
}

/***********************************************************************
 * checkModifier : check whether instruction is a source modifier
 *
 * Enter:   Inst = instruction to check
 *
 * Return:  ABSMOD, NEGMOD, NOTMOD, ZEXT, SEXT or MAININST (0) if not modifier
 */
static int checkModifier(Instruction *Inst)
{
  switch (Inst->getOpcode()) {
  case Instruction::FNeg:
    LLVM_DEBUG(llvm::dbgs()
               << "Instruction " << *Inst << " detected as NEGMOD\n");
    return BaleInfo::NEGMOD;
  case Instruction::Sub:
  case Instruction::FSub:
    // Negate is represented in LLVM IR by subtract from 0.
    if (Constant *Lhs = dyn_cast<Constant>(Inst->getOperand(0))) {
      // Canonicalize splats as well
      if (isa<VectorType>(Lhs->getType()))
        if (auto splat = Lhs->getSplatValue())
          Lhs = splat;

      // Usage of negative modifier on unsigned value can lead
      // to unexpected behaviour if baled into another instruction.
      if (Lhs->isZeroValue() &&
          std::none_of(
              Inst->use_begin(), Inst->use_end(),
              [](Use &UseOp) { return isa<UIToFPInst>(UseOp.getUser()); }) &&
          std::none_of(Inst->op_begin(), Inst->op_end(),
                       [](Value *Val) { return isa<FPToUIInst>(Val); })) {
        LLVM_DEBUG(llvm::dbgs()
                   << "Instruction " << *Inst << " detected as NEGMOD\n");
        return BaleInfo::NEGMOD;
      }
    }
    break;
  case Instruction::Xor:
    if (isIntNot(Inst)) {
      LLVM_DEBUG(llvm::dbgs()
                 << "Instruction " << *Inst << " detected as NOTMOD\n");
      return BaleInfo::NOTMOD;
    }
    break;
  case Instruction::ZExt:
    if (!Inst->getOperand(0)->getType()->getScalarType()->isIntegerTy(1)) {
      LLVM_DEBUG(llvm::dbgs()
                 << "Instruction " << *Inst << " detected as ZEXT\n");
      return BaleInfo::ZEXT;
    }
    break;
  case Instruction::SExt:
    if (!Inst->getOperand(0)->getType()->getScalarType()->isIntegerTy(1)) {
      LLVM_DEBUG(llvm::dbgs()
                 << "Instruction " << *Inst << " detected as SEXT\n");
      return BaleInfo::SEXT;
    }
    break;
  default:
    if (auto IID = vc::getAnyIntrinsicID(Inst);
        IID == Intrinsic::fabs || IID == GenXIntrinsic::genx_absi) {
      LLVM_DEBUG(dbgs() << "Instruction " << *Inst << " detected as ABSMOD\n");
      return BaleInfo::ABSMOD;
    }
    break;
  }
  LLVM_DEBUG(llvm::dbgs() << "Instruction " << *Inst
                          << " detected as MAININST\n");
  return BaleInfo::MAININST;
}

/***********************************************************************
 * operandCanBeBaled : check if a operand in main inst can be baled
 *
 * Enter:   Inst = the main inst
 *          OperandNum = operand number to look at
 *          ModType = what type of modifier (arith/logic/extonly/none) this
 *                    operand accepts
 *          AI = GenXIntrinsicInfo::ArgInfo, so we can see any stride
 *               restrictions, omitted if Inst is not an intrinsic
 */
bool GenXBaling::operandCanBeBaled(
    Instruction *Inst, unsigned OperandNum, int ModType,
    unsigned ArgInfoBits = GenXIntrinsicInfo::GENERAL) {
  GenXIntrinsicInfo::ArgInfo AI(ArgInfoBits);
  Instruction *Opnd = dyn_cast<Instruction>(Inst->getOperand(OperandNum));
  if (!Opnd)
    return false;
  // Check for source operand modifier.
  if (ModType != GenXIntrinsicInfo::MODIFIER_DEFAULT) {
    int Mod = checkModifier(Opnd);
    switch (Mod) {
    case BaleInfo::MAININST:
      break;
    case BaleInfo::ZEXT:
      // Don't allow zext->sext baling
      if (isa<SExtInst>(Inst))
        return false;
      LLVM_FALLTHROUGH;
    case BaleInfo::SEXT: {
      if (CmpInst *CmpI = dyn_cast<CmpInst>(Inst)) {
        if ((Mod == BaleInfo::SEXT) != CmpI->isSigned())
          return false;
      }
      // Don't allow sext->zext baling
      if (Mod == BaleInfo::SEXT && isa<ZExtInst>(Inst))
        return false;
      // NOTE: mulh and madw don't allow sext/zext bailing, as it allows only
      // D/UD operands
      auto IID = GenXIntrinsic::getGenXIntrinsicID(Inst);
      if (IID == GenXIntrinsic::genx_smulh ||
          IID == GenXIntrinsic::genx_umulh ||
          IID == GenXIntrinsic::genx_smadw || IID == GenXIntrinsic::genx_umadw)
        return false;
      return true;
    }
    case BaleInfo::NOTMOD:
      if (ModType == GenXIntrinsicInfo::MODIFIER_LOGIC)
        return true;
      break;
    case BaleInfo::NEGMOD:
      // Don't bale neg(abs(X)), because the hardware doesn't support it.
      if (vc::isAbsIntrinsic(Inst))
        return false;
      if (ModType == GenXIntrinsicInfo::MODIFIER_ARITH)
        return true;
      break;
    case BaleInfo::ABSMOD:
      // Part of the bodge to allow abs to be baled in to zext/sext.
      if (ModType == MODIFIER_ABSONLY)
        return true;
      LLVM_FALLTHROUGH;
    default:
      if (ModType == GenXIntrinsicInfo::MODIFIER_ARITH)
        return true;
      break;
    }
  }
  if (GenXIntrinsic::isRdRegion(Opnd)) {
    // The operand is a rdregion. Check any restrictions.
    // (Note we call isRegionOKForIntrinsic even when Inst is not an
    // intrinsic, since in that case AI is initialized to a state
    // where there are no region restrictions.)
    Region RdR = makeRegionFromBaleInfo(Opnd, BaleInfo());
    if (!isRegionOKForIntrinsic(AI.Info, RdR, canSplitBale(Inst)) ||
        !genx::isSafeToSink_CheckAVLoadKill(Opnd, Inst, this))
      return false;

    // Do not bale in a region read with multiple uses if
    // - any use is bitcast, or
    // - it is indirect.
    // as bitcast will not bale its operands and indirect multiple-use region
    // reads often lead to narrow simd width after legalization.
    if (Opnd->hasNUsesOrMore(2) && (Kind == BalingKind::BK_Legalization ||
                                   Kind == BalingKind::BK_Analysis)) {
      for (auto U : Opnd->users())
        if (isa<BitCastInst>(U))
          return false;
      if (RdR.Indirect)
        return false;
    }
    // Indirect regions are not supported for LSC.
    if (RdR.Indirect &&
        (GenXIntrinsic::isLSC(Inst) ||
         vc::InternalIntrinsic::isInternalMemoryIntrinsic(Inst)))
      return false;
    return true;
  }
  return false;
}

/***********************************************************************
 * processWrPredRegion : set up baling info for wrpredregion
 *
 * The input to wrpredregion may be the following:
 * 1) icmp or fcmp, in which case it is always baled.
 * 2) constant, which may resulted from region simplification.
 */
void GenXBaling::processWrPredRegion(Instruction *Inst)
{
  Value *V = Inst->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum);
  BaleInfo BI(BaleInfo::WRPREDREGION);
  bool isRdPredR =
      vc::getAnyIntrinsicID(V) == GenXIntrinsic::genx_rdpredregion;
  if (isa<CmpInst>(V) || isRdPredR) {
    LLVM_DEBUG(llvm::dbgs() << __FUNCTION__ << " setting operand #"
                            << GenXIntrinsic::GenXRegion::NewValueOperandNum
                            << " to bale in instruction " << *Inst << "\n");
    if (isRdPredR)
      setBaleInfo(cast<Instruction>(V), BaleInfo(BaleInfo::RDPREDREGION));
    setOperandBaled(Inst, GenXIntrinsic::GenXRegion::NewValueOperandNum, &BI);
  }
  setBaleInfo(Inst, BI);
}

/***********************************************************************
 * processRdPredRegion : set up baling info for rdpredregion
 *
 */
void GenXBaling::processRdPredRegion(Instruction *Inst) {
  BaleInfo BI(BaleInfo::RDPREDREGION);
  setBaleInfo(Inst, BI);
}

/***********************************************************************
 * processWrPredPredRegion : set up baling info for wrpredpredregion
 *
 * The "new value" input to wrpredregion must be icmp or fcmp, and it is always
 * baled.
 *
 * The condition input is assumed to be EM. But it might be an rdpredregion
 * out of EM, in which case the rdpredregion is baled. The rdpredregion must
 * have offset 0.
 */
void GenXBaling::processWrPredPredRegion(Instruction *Inst)
{
  IGC_ASSERT(isa<CmpInst>(Inst->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum)));
  BaleInfo BI(BaleInfo::WRPREDPREDREGION);
  setOperandBaled(Inst, GenXIntrinsic::GenXRegion::NewValueOperandNum, &BI);
  Value *Cond = Inst->getOperand(3);
  if (GenXIntrinsic::getGenXIntrinsicID(Cond) == GenXIntrinsic::genx_rdpredregion) {
    IGC_ASSERT(cast<Constant>(cast<CallInst>(Cond)->getOperand(1))->isNullValue());
    setOperandBaled(Inst, 3, &BI);
  }
  setBaleInfo(Inst, BI);
}

/***********************************************************************
 * processWrRegion : set up baling info for wrregion
 */
void GenXBaling::processWrRegion(Instruction *Inst)
{
  BaleInfo BI(BaleInfo::WRREGION);
  unsigned OperandNum = GenXIntrinsic::GenXRegion::NewValueOperandNum;
  // Get the instruction (if any) that creates the element/subregion to write.
  auto *V = dyn_cast<Instruction>(Inst->getOperand(OperandNum));
  if (V && !V->hasOneUse()) {
    // The instruction has multiple uses.
    // We don't want to bale in the following cases, as they seem to make the
    // code worse, unless this is load from a global variable.
    if (V->getParent() == Inst->getParent()) {
      // 1. The maininst is a select.
      Bale B;
      buildBale(V, &B);
      if (auto MainInst = B.getMainInst()) {
        if (isa<SelectInst>(MainInst->Inst) ||
            isHighCostBaling(BaleInfo::WRREGION, MainInst->Inst))
          V = nullptr;
      }
    } else {
      // 2. It is in a different basic block to the wrregion.
      if (!genx::isRdRWithOldValueVLoadSrc(V))
        V = nullptr;
    }
    // FIXME: Baling on WRREGION is not the right way to reduce the overhead
    // from `wrregion`. Instead, register coalescing should be applied to
    // enable direct defining of the WRREGION and minimize the value
    // duplication.
  }

  // Do not bale rdregion into wrregion if they access the same pointer
  // and there is a store to it in between
  if (V && genx::isRdRWithOldValueVLoadSrc(V) &&
      genx::isWrRWithOldValueVLoadSrc(Inst)) {
    auto *L1 = cast<Instruction>(
        Inst->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum));
    auto *L2 = cast<Instruction>(cast<Instruction>(V)->getOperand(
        GenXIntrinsic::GenXRegion::OldValueOperandNum));
    auto *Src1 = genx::getAVLoadSrcOrNull(L1);
    auto *Src2 = genx::getAVLoadSrcOrNull(L2);
    if (Src1 && Src1 == Src2 && !genx::vloadsReadSameValue(L1, L2, DT))
      V = nullptr;
  }

  // Do not bale if NewValue is raw operand and wrr is used in predef reg.
  if (std::any_of(Inst->user_begin(), Inst->user_end(), [](User *U) {
        return GenXIntrinsic::isReadWritePredefReg(U);
      })) {
    unsigned ValIntrinID = vc::getAnyIntrinsicID(V);
    GenXIntrinsicInfo II(ValIntrinID);
    if (GenXIntrinsic::isGenXIntrinsic(ValIntrinID) &&
        (II.getRetInfo().getCategory() == GenXIntrinsicInfo::RAW)) {
      V = nullptr;
    }
  }

  // Do not bale rdtsc instruction to prevent the reordering and
  // extra region casting later.
  if (vc::getAnyIntrinsicID(V) == Intrinsic::readcyclecounter)
    V = nullptr;

  if (V &&
      isBalableNewValueIntoWrr(V, makeRegionFromBaleInfo(Inst, BaleInfo())) &&
      isSafeToMove(V, V, Inst)) {
    LLVM_DEBUG(llvm::dbgs()
               << __FUNCTION__ << " setting operand #" << OperandNum
               << " to bale in instruction " << *Inst << "\n");
    setOperandBaled(Inst, OperandNum, &BI);
    if (Liveness) {
      // Ensure the wrregion's result has an
      // alignment of 32 if intrinsic with
      // raw result was baled into
      unsigned ValIntrinID = vc::getAnyIntrinsicID(V);
      GenXIntrinsicInfo II(ValIntrinID);
      if (GenXIntrinsic::isGenXIntrinsic(ValIntrinID) &&
          (ValIntrinID != GenXIntrinsic::genx_sat) &&
          !GenXIntrinsic::isRdRegion(V) && !GenXIntrinsic::isWrRegion(V) &&
          (II.getRetInfo().getCategory() == GenXIntrinsicInfo::RAW))
        Liveness->getOrCreateLiveRange(Inst)->LogAlignment = getLogAlignment(
            VISA_Align::ALIGN_GRF, ST ? ST->getGRFByteSize() : defaultGRFByteSize);
    }
  }
  // Now see if there is a variable index with an add/sub with an in range
  // offset that we can bale in, such that the add/sub does not already
  // bale in other instructions.
  OperandNum = GenXIntrinsic::GenXRegion::WrIndexOperandNum;
  if (auto IndexOperand = Inst->getOperand(OperandNum);
      isBalableIndexAdd(IndexOperand)) {
    LLVM_DEBUG(llvm::dbgs()
               << __FUNCTION__ << " setting operand #" << OperandNum
               << " to bale in instruction " << *Inst << "\n");
    setOperandBaled(Inst, OperandNum, &BI);
    // We always set up InstMap for an address add, even though it does not
    // bale in any operands.
    setBaleInfo(cast<Instruction>(IndexOperand), BaleInfo(BaleInfo::ADDRADD, 0));
  }
  // See if there is any baling in to the predicate (mask) operand.
  if (processPredicate(Inst, GenXIntrinsic::GenXRegion::PredicateOperandNum))
    setOperandBaled(Inst, GenXIntrinsic::GenXRegion::PredicateOperandNum, &BI);
  // We always set up InstMap for a wrregion, even if it does not bale in any
  // operands.
  setBaleInfo(Inst, BI);
}

// Process a select instruction. Return true if it can be baled into a cmp
// instruction, false otherwise.
bool GenXBaling::processSelect(Instruction *Inst) {
  auto SI = dyn_cast<SelectInst>(Inst);
  if (!SI)
    return false;

  // Only bale into a cmp instruction.
  auto *Cmp = dyn_cast<CmpInst>(SI->getCondition());
  if (!Cmp || !Cmp->hasOneUse())
    return false;

  // Only bale "select cond, -1, 0"
  Constant *Src0 = dyn_cast<Constant>(SI->getTrueValue());
  Constant *Src1 = dyn_cast<Constant>(SI->getFalseValue());
  if (!Src0 || !Src0->isAllOnesValue() || !Src1 || !Src1->isNullValue())
    return false;

  // VISA supports only hf/f destination types for floating-point cmp, not bale
  // select as CMPDST in this case.
  if (Cmp->isFPPredicate())
    return false;

  BaleInfo BI(BaleInfo::CMPDST);
  unsigned OperandNum = 0;
  LLVM_DEBUG(llvm::dbgs() << __FUNCTION__ << " setting operand #" << OperandNum
                          << " to bale in instruction " << *SI << "\n");
  setOperandBaled(SI, OperandNum, &BI);
  setBaleInfo(SI, BI);
  return true;
}

void GenXBaling::processStore(StoreInst *Inst) {
  BaleInfo BI(BaleInfo::GSTORE);
  unsigned OperandNum = 0;
  Instruction *V = dyn_cast<Instruction>(Inst->getOperand(OperandNum));
  if (GenXIntrinsic::isWrRegion(V)) {
    constexpr unsigned NewValOpNum = GenXIntrinsic::GenXRegion::NewValueOperandNum;
    if (getBaleInfo(V).isOperandBaled(NewValOpNum)) {
      Instruction *MainInst = cast<Instruction>(V->getOperand(NewValOpNum));
      if (getBaleInfo(MainInst).Type == BaleInfo::MAININST &&
          !isSafeToMove(MainInst, V, Inst))
        return;
    }
    LLVM_DEBUG(llvm::dbgs()
               << __FUNCTION__ << " setting operand #" << OperandNum
               << " to bale in instruction " << *Inst << "\n");
    setOperandBaled(Inst, OperandNum, &BI);
  } else if (isa<CallInst>(V) && cast<CallInst>(V)->isInlineAsm()) {
    LLVM_DEBUG(llvm::dbgs()
               << __FUNCTION__ << " setting operand #" << OperandNum
               << " to bale in instruction " << *Inst << "\n");
    setOperandBaled(Inst, OperandNum, &BI);
  }
  setBaleInfo(Inst, BI);
}

// We can bale in shufflevector of predicate if it is replicated slice.
bool GenXBaling::processShufflePred(Instruction *Inst) {
  IGC_ASSERT_MESSAGE(Inst->getType()->getScalarSizeInBits() == 1,
    "Expected bool shuffle");
  auto *SI = dyn_cast<ShuffleVectorInst>(Inst);
  if (!SI)
    return false;

  IGC_ASSERT_MESSAGE(ShuffleVectorAnalyzer(SI).isReplicatedSlice(),
    "Predicate shuffle is not replicated slice!");
  BaleInfo BI(BaleInfo::SHUFFLEPRED);
  setBaleInfo(SI, BI);
  return true;
}

/***********************************************************************
 * processPredicate : process predicate operand (to wrregion or branch)
 *
 * Enter:   Inst = instruction with predicate operand
 *          OperandNum = operand number in Inst
 *
 * Return:  whether operand can be baled in
 *
 * If the function returns true, the caller needs to call
 * setOperandBaled(Inst, OperandNum, &BI) to actually bale it in.
 *
 * Unlike most baling, which proceeds in code order building a tree of baled in
 * instructions, this function recurses, scanning backward through the code,
 * because we only want to bale predicate operations all/any/not/rdpredregion
 * once we know that the resulting predicate is used in wrregion or branch (as
 * opposed to say a bitcast to int).
 *
 * So this function decides whether OperandNum in Inst is an instruction that
 * is to be baled in, and additionally performs any further baling in to that
 * instruction.
 */
bool GenXBaling::processPredicate(Instruction *Inst, unsigned OperandNum) {
  Instruction *Mask = dyn_cast<Instruction>(Inst->getOperand(OperandNum));
  if (!Mask)
    return false;

  if (Kind == BalingKind::BK_CodeGen && !isa<VectorType>(Mask->getType())) {
    if (auto Extract = dyn_cast<ExtractValueInst>(Mask)) {
      auto *GotoJoin = cast<Instruction>(Extract->getAggregateOperand());
      auto IID = vc::getAnyIntrinsicID(GotoJoin);
      if (IID == GenXIntrinsic::genx_simdcf_goto
          || IID == GenXIntrinsic::genx_simdcf_join) {
        // Second pass: Mask is the extractvalue of the !any(EM) result out of
        // the result of goto/join. We mark both the use of the extract in the
        // branch and the use of the goto/join in the extract as baled. The
        // former is done by the caller when we return true.
        BaleInfo BI;
        LLVM_DEBUG(llvm::dbgs() << __FUNCTION__ << " setting operand #" << 0
                                << " to bale in instruction " << *Mask << "\n");
        setOperandBaled(Mask, /*OperandNum=*/0, &BI);
        setBaleInfo(Mask, BI);
        return true;
      }
    }
  }
  switch (GenXIntrinsic::getGenXIntrinsicID(Mask)) {
    case GenXIntrinsic::genx_rdpredregion: {
      if (Kind == BalingKind::BK_CodeGen) {
        // Sanity check the offset and number of elements being accessed.

        unsigned MinSize = 0; // it will be assigned inside assertion statament
        IGC_ASSERT((MinSize = Inst->getType()->getScalarType()->getPrimitiveSizeInBits() == 64 ? 4 : 8, 1));

        unsigned NElems = 0; // it will be assigned inside assertion statament
        IGC_ASSERT((NElems = cast<IGCLLVM::FixedVectorType>(Mask->getType())
                                 ->getNumElements(),
                    1));

        unsigned Offset = 0; // it will be assigned inside assertion statament
        IGC_ASSERT((Offset = dyn_cast<ConstantInt>(Mask->getOperand(1))->getZExtValue(), 1));

        IGC_ASSERT_MESSAGE(exactLog2(NElems) >= 0,
          "illegal offset and/or width in rdpredregion");
        IGC_ASSERT_MESSAGE((Offset & (std::min(NElems, MinSize) - 1)) == 0,
          "illegal offset and/or width in rdpredregion");

        (void) MinSize;
        (void) NElems;
        (void) Offset;
      }
      // We always set up InstMap for an rdpredregion, even though it does not
      // bale in any operands.
      setBaleInfo(Mask, BaleInfo(BaleInfo::RDPREDREGION, 0));
      return true;
    }
    case GenXIntrinsic::genx_all:
    case GenXIntrinsic::genx_any: {
        if (Kind != BalingKind::BK_CodeGen)
          return false; // only bale all/any for CodeGen
        // The mask is the result of an all/any. Bale that in.
        // Also see if its operand can be baled in.
        BaleInfo BI(BaleInfo::ALLANY);
        if (processPredicate(Mask, /*OperandNum=*/0)) {
          LLVM_DEBUG(llvm::dbgs()
                     << __FUNCTION__ << " setting operand #" << 0
                     << " to bale in instruction " << *Mask << "\n");
          setOperandBaled(Mask, /*OperandNum=*/0, &BI);
        }
        setBaleInfo(Mask, BI);
        return true;
      }
    default:
      break;
  }

  if (isPredNot(Mask)) {
    // The mask is the result of a notp. Bale that in.
    // Also see if its operand can be baled in.
    BaleInfo BI(BaleInfo::NOTP);
    if (processPredicate(Mask, /*OperandNum=*/0)) {
      LLVM_DEBUG(llvm::dbgs() << __FUNCTION__ << " setting operand #" << 0
                              << " to bale in instruction " << *Mask << "\n");
      setOperandBaled(Mask, /*OperandNum=*/0, &BI);
    }
    setBaleInfo(Mask, BI);
    return true;
  }

  if (processShufflePred(Mask))
    return true;

  return false;
}

/***********************************************************************
 * processSat : set up baling info fp saturate
 */
void GenXBaling::processSat(Instruction *Inst) {
  BaleInfo BI(BaleInfo::SATURATE);
  // Get the instruction (if any) that creates value to saturate.
  unsigned OperandNum = 0;
  Instruction *V = dyn_cast<Instruction>(Inst->getOperand(OperandNum));
  if (V && V->hasOneUse()) {
    // It is an instruction where we are the only use. We can bale it in, if
    // it is a suitable instruction.
    auto ValIntrinID = vc::getAnyIntrinsicID(V);
    if (GenXIntrinsic::isRdRegion(ValIntrinID)) {
      LLVM_DEBUG(llvm::dbgs()
                 << __FUNCTION__ << " setting operand #" << OperandNum
                 << " to bale in instruction " << *Inst << "\n");
      setOperandBaled(Inst, OperandNum, &BI);
    } else if (ValIntrinID == GenXIntrinsic::not_any_intrinsic) {
      if (isa<BinaryOperator>(V) || isa<SelectInst>(V) ||
          (isa<CastInst>(V) && !isa<BitCastInst>(V) && !isBFloat16Cast(V))) {
        LLVM_DEBUG(llvm::dbgs()
                   << __FUNCTION__ << " setting operand #" << OperandNum
                   << " to bale in instruction " << *Inst << "\n");
        setOperandBaled(Inst, OperandNum, &BI);
      }
    } else if (!GenXIntrinsic::isWrRegion(ValIntrinID)) {
      // V is an intrinsic other than rdregion/wrregion. Check that its return
      // value is suitable for baling.
      GenXIntrinsicInfo II(ValIntrinID);
      auto SatInfo = II.getRetInfo().getSaturation();
      auto SatIID = vc::getAnyIntrinsicID(Inst);
      if (!II.getRetInfo().isRaw() &&
          ((SatInfo == GenXIntrinsicInfo::SATURATION_DEFAULT &&
            SatIID == GenXIntrinsic::genx_sat) ||
           (SatInfo == GenXIntrinsicInfo::SATURATION_INTALLOWED &&
            GenXIntrinsic::isIntegerSat(SatIID)))) {
        LLVM_DEBUG(llvm::dbgs()
                   << __FUNCTION__ << " setting operand #" << OperandNum
                   << " to bale in instruction " << *Inst << "\n");
        setOperandBaled(Inst, OperandNum, &BI);
      }
    }
  }
  // We always set up InstMap for a saturate, even if it does not bale in any
  // operands.
  setBaleInfo(Inst, BI);
}

/***********************************************************************
 * processRdRegion : set up baling info for rdregion
 */
void GenXBaling::processRdRegion(Instruction *Inst)
{
  // See if there is a variable index with an add/sub with an in range
  // offset that we can bale in, such that the add/sub does not already
  // bale in other instructions.
  const unsigned OperandNum = GenXIntrinsic::GenXRegion::RdIndexOperandNum;
  BaleInfo BI(BaleInfo::RDREGION);
  if (auto IndexOperand = Inst->getOperand(OperandNum);
      isBalableIndexAdd(IndexOperand)) {
    LLVM_DEBUG(llvm::dbgs()
               << __FUNCTION__ << " setting operand #" << OperandNum
               << " to bale in instruction " << *Inst << "\n");
    setOperandBaled(Inst, OperandNum, &BI);
    // We always set up InstMap for an address add, even though it does not
    // bale in any operands.
    setBaleInfo(cast<Instruction>(IndexOperand), BaleInfo(BaleInfo::ADDRADD, 0));
  } else if (isBalableIndexOr(Inst->getOperand(OperandNum))) {
    LLVM_DEBUG(llvm::dbgs()
               << __FUNCTION__ << " setting operand #" << OperandNum
               << " to bale in instruction " << *Inst << "\n");
    setOperandBaled(Inst, OperandNum, &BI);
    // We always set up InstMap for an address or, even though it does not
    // bale in any operands.
    setBaleInfo(cast<Instruction>(Inst->getOperand(OperandNum)), BaleInfo(BaleInfo::ADDROR, 0));
  }
  // We always set up InstMap for a rdregion, even if it does not bale in any
  // operands.
  setBaleInfo(Inst, BI);
}

/***********************************************************************
 * processInlineAsm : RdRegion result a baled into inline asm
 *                    instruction. Inline Assembly iremains the main instruction
 *                    of the bale.
 */
void GenXBaling::processInlineAsm(Instruction *Inst) {
  auto CI = dyn_cast<CallInst>(Inst);
  IGC_ASSERT_MESSAGE(CI, "Inline Asm expected");
  IGC_ASSERT_MESSAGE(CI->isInlineAsm(), "Inline Asm expected");

  BaleInfo BI(BaleInfo::MAININST);
  for (unsigned I = 0; I < IGCLLVM::getNumArgOperands(CI); I++)
    if (auto RdR = dyn_cast<Instruction>(CI->getArgOperand(I)))
      if (GenXIntrinsic::isRdRegion(RdR)) {
        switch (GenXIntrinsic::getGenXIntrinsicID(RdR->getOperand(0))) {
        default:
          LLVM_DEBUG(llvm::dbgs()
                     << __FUNCTION__ << " setting operand #" << I
                     << " to bale in instruction " << *Inst << "\n");
          setOperandBaled(Inst, I, &BI);
          break;
        case GenXIntrinsic::genx_constanti:
        case GenXIntrinsic::genx_constantf:
          continue;
        }
      }

  setBaleInfo(Inst, BI);
}

void GenXBaling::processRdWrPredefReg(Instruction *Inst) {
  auto *CI = dyn_cast<CallInst>(Inst);
  IGC_ASSERT_MESSAGE((CI && GenXIntrinsic::isReadWritePredefReg(Inst)),
                     "genx.read/write.reg expected");
  BaleInfo BI(BaleInfo::REGINTR);
  setBaleInfo(Inst, BI);
}

/***********************************************************************
 * processExtractValue : Extract instructions can get elements from structure
 *                       which was a result of inline assembly call with multiple outputs.
 */
void GenXBaling::processExtractValue(ExtractValueInst *EV) {
  IGC_ASSERT(EV);
  if (auto CI = dyn_cast<CallInst>(EV->getAggregateOperand()))
    if (CI->isInlineAsm())
      setBaleInfo(EV, BaleInfo(BaleInfo::MAININST, 0));
}

/***********************************************************************
 * static getIndexAdd : test whether the specified value is
 *        a constant add/sub that could be baled in as a variable index offset,
 *        but without checking that the index is in range
 *
 * Enter:   V = the value that might be a constant add/sub
 *          Offset = where to store the offset of the constant add/sub
 *
 * Return:  true if a constant add/sub was detected
 *
 * For the second run of GenXBaling, which is after GenXCategoryConversion,
 * we are looking for an llvm.genx.add.addr rather than a real add/sub.
 */
bool GenXBaling::getIndexAdd(Value *V, int *Offset)
{
  if (Instruction *Inst = dyn_cast<Instruction>(V)) {
    int IsConstAdd = 0;
    switch (Inst->getOpcode()) {
      case Instruction::Add:
        IsConstAdd = 1;
        break;
      case Instruction::Sub:
        IsConstAdd = -1;
        break;
      default:
        if (GenXIntrinsic::getGenXIntrinsicID(Inst) ==
            GenXIntrinsic::genx_add_addr)
          IsConstAdd = 1;
        break;
    }
    if (IsConstAdd) {
      if (Constant *C = dyn_cast<Constant>(Inst->getOperand(1))) {
        if (isa<VectorType>(C->getType()))
          C = C->getSplatValue();
        if (C) {
          if (C->isNullValue()) {
            *Offset = 0;
            return true;
          }
          if (ConstantInt *CI = dyn_cast<ConstantInt>(C)) {
            // It is a constant add/sub.
            *Offset = CI->getSExtValue() * IsConstAdd;
            return true;
          }
        }
      }
    }
  }
  return false;
}

/***********************************************************************
 * static getIndexOr : test whether the specified value is
 *        a constant Or that could be baled in as a variable index offset,
 *        but without checking that the index is in range
 *
 * Enter:   V = the value that might be a constant or
 *          Offset = where to store the offset of the constant or
 *
 * Return:  true if a constant or was detected
 */
bool GenXBaling::getIndexOr(Value *V, int &Offset)
{
  Instruction *Inst = dyn_cast<Instruction>(V);
  if (!Inst)
    return false;

  if (Inst->getOpcode() != Instruction::Or)
    return false;

  // inst is Or from this point
  Constant *C = dyn_cast<Constant>(Inst->getOperand(1));
  if (!C)
    return false;

  if (isa<VectorType>(C->getType()))
    C = C->getSplatValue();

  // getSplatValue could return nullptr
  if (!C)
    return false;

  if (C->isNullValue()) {
    Offset = 0;
    return true;
  }
  if (ConstantInt *CI = dyn_cast<ConstantInt>(C)) {
    // check for or could be changed to add
    if(!haveNoCommonBitsSet(Inst->getOperand(0), Inst->getOperand(1),
                              Inst->getModule()->getDataLayout()))
    {
      return false;
    }
    Offset = CI->getSExtValue();
    return true;
  }
  return false;
}

/***********************************************************************
 * static isBalableIndexAdd : test whether the specified value is
 *        a constant add/sub that could be baled in as a variable index offset
 *
 * For the second run of GenXBaling, which is after GenXCategoryConversion,
 * we are looking for an llvm.genx.add.addr rather than a real add/sub.
 */
bool GenXBaling::isBalableIndexAdd(Value *V)
{
  int Offset;
  if (!getIndexAdd(V, &Offset))
    return false;
  // It is a constant add/sub. Check the constant is in range.
  return ( G4_MIN_ADDR_IMM <= Offset && Offset <= G4_MAX_ADDR_IMM);
}

/***********************************************************************
 * static isBalableIndexOr : test whether the specified value is
 *        a constant Or that could be baled in as a variable index offset
 */
bool GenXBaling::isBalableIndexOr(Value *V)
{
  int Offset;
  if (!getIndexOr(V, Offset))
    return false;
  IGC_ASSERT_MESSAGE(Offset >=0, "Offset in or appears to be less than zero");
  // It is a constant or. Check the constant is in range.
  return (Offset  <= G4_MAX_ADDR_IMM);
}

// Returns true if instruction uses same mask as operand. Possible cases:
// same mask uses as operand of intrinsic directly; mask value is a replicated
// slice of the mask operand of intrinsic.
static bool usesSameMaskAsOperand(Instruction *Inst, Value *Mask) {
  auto *CI = dyn_cast<CallInst>(Inst);
  if (!CI)
    return false;

  Value *MaskOperand = genx::getMaskOperand(Inst);
  // No mask at all
  if (!MaskOperand)
    return false;

  // Easy case, same mask as operand
  if (MaskOperand == Mask)
    return true;

  // Allow constant predicates only for gather4_masked_scaled2,
  // at least for now
  if (auto *CV1 = dyn_cast<ConstantVector>(Mask))
    if (auto *CV2 = dyn_cast<ConstantVector>(MaskOperand))
      return GenXIntrinsic::getGenXIntrinsicID(Inst) ==
                 GenXIntrinsic::genx_gather4_masked_scaled2 &&
             genx::isReplicatedConstantVector(CV2, CV1);

  auto MaskOperandInrtID = GenXIntrinsic::getGenXIntrinsicID(MaskOperand);
  auto MaskIntrID = GenXIntrinsic::getGenXIntrinsicID(Mask);

  if (MaskOperandInrtID == GenXIntrinsic::genx_constantpred &&
      MaskIntrID == GenXIntrinsic::genx_constantpred) {
    auto *CV1 =
        dyn_cast<ConstantVector>(cast<CallInst>(MaskOperand)->getOperand(0));
    auto *CV2 = dyn_cast<ConstantVector>(cast<CallInst>(Mask)->getOperand(0));
    return GenXIntrinsic::getGenXIntrinsicID(Inst) ==
               GenXIntrinsic::genx_gather4_masked_scaled2 &&
           CV1 && CV2 && genx::isReplicatedConstantVector(CV1, CV2);
  }

  // If both are rdpredregions then compare for equvalency (same operand and
  // offset)
  if (MaskOperandInrtID == GenXIntrinsic::genx_rdpredregion &&
      MaskIntrID == GenXIntrinsic::genx_rdpredregion) {
    auto *RdPred1 = cast<CallInst>(MaskOperand);
    auto *RdPred2 = cast<CallInst>(Mask);
    return RdPred1->getOperand(0) == RdPred2->getOperand(0) &&
           RdPred1->getArgOperand(1) == RdPred2->getArgOperand(1);
  }

  auto *SI = dyn_cast<ShuffleVectorInst>(Mask);
  if (!SI)
    return false;

  ShuffleVectorAnalyzer SIAnalyzer(SI);
  if (!SIAnalyzer.isReplicatedSlice())
    return false;

  // If shufflevector replicates mask operand then we can bale
  if (SI->getOperand(0) == MaskOperand) {
    IGC_ASSERT_MESSAGE(!SIAnalyzer.getReplicatedSliceDescriptor().InitialOffset,
      "Expected zero initial offset");
    return true;
  }

  // If operand is rdpredregion result then check for same initial offset
  // of shufflevector and rdpredregion's offset
  if (GenXIntrinsic::getGenXIntrinsicID(MaskOperand) ==
      GenXIntrinsic::genx_rdpredregion) {
    auto *PredRdR = cast<CallInst>(MaskOperand);
    return cast<ConstantInt>(PredRdR->getArgOperand(1))->getZExtValue() ==
           SIAnalyzer.getReplicatedSliceDescriptor().InitialOffset;
  }

  return false;
}


/***********************************************************************
 * isBalableNewValueIntoWrr : check whether the new val operand can
 * be baled into wrr instruction
 */
bool GenXBaling::isBalableNewValueIntoWrr(Value *V, const Region &WrrR) {
  Instruction *Inst = dyn_cast<Instruction>(V);
  if (!Inst)
    return false;

  // It is an instruction. We can bale it in, if it is a suitable
  // instruction.
  unsigned ValIntrinID = vc::getAnyIntrinsicID(Inst);
  if (ValIntrinID == GenXIntrinsic::genx_sat ||
      GenXIntrinsic::isRdRegion(ValIntrinID))
    return true;
  else if (ValIntrinID == GenXIntrinsic::not_any_intrinsic) {
    if (isa<BinaryOperator>(Inst))
      return true;
    if (auto *CI = dyn_cast<CastInst>(Inst);
        CI && !genx::isNoopCast(CI))
        return true;
    else if (isMaskPacking(Inst))
      return true;
    else if (isa<CallInst>(Inst) && cast<CallInst>(Inst)->isInlineAsm())
      return true;
    else if (isa<SelectInst>(Inst) && !WrrR.Mask) {
      // Can bale in a select as long as the wrregion is unpredicated.
      return true;
    } else if (isa<ExtractValueInst>(Inst)) {
      // Each extract bales into its own WrRegionand remains
      // the main instruction of the bale
      auto Extract = cast<ExtractValueInst>(Inst);
      if (auto CI = dyn_cast<CallInst>(Extract->getAggregateOperand()))
        if (CI->isInlineAsm())
          return true;
    }
  } else if (!GenXIntrinsic::isWrRegion(ValIntrinID)) {
    // V is an intrinsic other than rdregion/wrregion. If this is a
    // predicated wrregion, only permit baling in if the intrinsic
    // supports a predicate mask.
    GenXIntrinsicInfo II(ValIntrinID);

    if (WrrR.Mask == 0 || II.getPredAllowed() ||
        usesSameMaskAsOperand(Inst, WrrR.Mask)) {
      // Check that its return value is suitable for baling.
      GenXIntrinsicInfo::ArgInfo AI = II.getRetInfo();
      switch (AI.getCategory()) {
      case GenXIntrinsicInfo::GENERAL: {
        if (isRegionOKForIntrinsic(AI.Info, WrrR, canSplitBale(Inst)))
          return true;
      } break;
      case GenXIntrinsicInfo::RAW: {
        // Intrinsic with raw result can be baled in to wrregion as long as
        // it is unstrided and starts on a GRF boundary, and there is no
        // non-undef TWOADDR operand.
        if (isRegionOKForRaw(WrrR, ST)) {
          unsigned FinalCallArgIdx = Inst->getNumOperands() - 2;
          if (isa<UndefValue>(Inst->getOperand(FinalCallArgIdx)))
            return true;
          else {
            GenXIntrinsicInfo::ArgInfo AI2 = II.getArgInfo(FinalCallArgIdx);
            if (AI2.getCategory() != GenXIntrinsicInfo::TWOADDR)
              return true;
          }
        }
      } break;
      }
    }
  }
  return false;
}

bool GenXBaling::isHighCostBaling(uint16_t Type, Instruction *Inst) {
  if (Type != BaleInfo::WRREGION)
    return false;
  auto *CI = dyn_cast<CallInst>(Inst);
  if (!CI)
    return false;
  return !CI->doesNotAccessMemory();
}

/***********************************************************************
 * acceptableMainInst : if Inst acceptable as bale main instruction
 */
static bool acceptableMainInst(Instruction *Inst) {
  if (isa<BinaryOperator>(Inst))
    return BaleBinary;
  if (isa<CmpInst>(Inst))
    return BaleCmp;
  if (isa<CastInst>(Inst))
    return BaleCast;
  if (isa<SelectInst>(Inst))
    return BaleSelect;
  if (Inst->getOpcode() == Instruction::FNeg)
    return BaleFNeg;
  return false;
}

/***********************************************************************
 * processMainInst : set up baling info for potential main instruction
 */
void GenXBaling::processMainInst(Instruction *Inst, int IntrinID) {
  if (isa<DbgInfoIntrinsic>(Inst))
    return;
  BaleInfo BI(BaleInfo::MAININST);
  if (IntrinID == GenXIntrinsic::not_any_intrinsic) {
    if (!acceptableMainInst(Inst))
      return;
    if (auto *CI = dyn_cast<CastInst>(Inst); CI && genx::isNoopCast(CI))
      return;
    BI.Type = checkModifier(Inst);
    // Work out whether the instruction accepts arithmetic, logic or no
    // modifier.
    int ModType = GenXIntrinsicInfo::MODIFIER_ARITH;
    switch (BI.Type) {
      case BaleInfo::NOTMOD:
        // a "not" can only merge with a logic modifier (another "not")
        ModType = GenXIntrinsicInfo::MODIFIER_LOGIC;
        break;
      case BaleInfo::ZEXT:
      case BaleInfo::SEXT:
        // an extend cannot bale in any other modifier.
        // But as a bodge we allow abs to be baled in to zext/sext. This is a
        // workaround for not having worked out how to set the computation type
        // in cm_abs. Currently cm_abs does a genx.absi in the source type, then
        // converts it to destination type. This does not allow for the result
        // of an abs needing one more bit than its input.
        ModType = MODIFIER_ABSONLY;
        break;
      case BaleInfo::MAININST:
        switch (Inst->getOpcode()) {
          case Instruction::And:
          case Instruction::Or:
          case Instruction::Xor:
            // These instructions take a logic modifier.
            ModType = GenXIntrinsicInfo::MODIFIER_LOGIC;
            break;
          case Instruction::LShr:
          case Instruction::AShr:
          case Instruction::Shl:
            // Do not allow source modifier on integer shift operations,
            // because of extra precision introduced.
            ModType = GenXIntrinsicInfo::MODIFIER_DEFAULT;
            break;
          default:
            // All other (non-intrinsic) instructions take an arith modifier.
            break;
        }
        break;
      default:
        // Anything else is an arith modifier, so it can only merge with
        // another arith modifier.
        break;
    }
    if (auto SI = dyn_cast<SelectInst>(Inst)) {
      // If instruction is select, at first try to bale true and false value.
      for (unsigned i = 1; i <= 2; ++i)
        if (operandCanBeBaled(Inst, i, ModType)) {
          LLVM_DEBUG(llvm::dbgs()
                     << __FUNCTION__ << " setting operand #" << i
                     << " to bale in instruction " << *Inst << "\n");
          setOperandBaled(Inst, i, &BI);
        }
      // If nothing was baled, it can be profitable to try convertion to wrregion.
      if (!BI.Bits && processSelectToPredicate(SI))
        return;
      // If select was not converted, try to bale predicate.
      const unsigned OperandNum = 0;
      if (processPredicate(Inst, OperandNum)) {
        LLVM_DEBUG(llvm::dbgs()
                   << __FUNCTION__ << " setting operand #" << OperandNum
                   << " to bale in instruction " << *Inst << "\n");
        setOperandBaled(Inst, OperandNum, &BI);
      }
    }
    else {
      // See which operands we can bale in.
      for (unsigned i = 0, e = Inst->getNumOperands(); i != e; ++i)
        if (operandCanBeBaled(Inst, i, ModType)) {
          LLVM_DEBUG(llvm::dbgs()
                     << __FUNCTION__ << " setting operand #" << i
                     << " to bale in instruction " << *Inst << "\n");
          setOperandBaled(Inst, i, &BI);
        }
    }
  } else if (IntrinID == GenXIntrinsic::genx_convert
      || IntrinID == GenXIntrinsic::genx_convert_addr) {
    // llvm.genx.convert can bale, and has exactly one arg
    if (operandCanBeBaled(Inst, 0, GenXIntrinsicInfo::MODIFIER_ARITH)) {
      LLVM_DEBUG(llvm::dbgs() << __FUNCTION__ << " setting operand #" << 0
                              << " to bale in instruction " << *Inst << "\n");
      setOperandBaled(Inst, 0, &BI);
    }
  } else if (vc::isAbsIntrinsic(IntrinID)) {
    BI.Type = BaleInfo::ABSMOD;
    if (operandCanBeBaled(Inst, 0, GenXIntrinsicInfo::MODIFIER_ARITH)) {
      LLVM_DEBUG(llvm::dbgs() << __FUNCTION__ << " setting operand #" << 0
                              << " to bale in instruction " << *Inst << "\n");
      setOperandBaled(Inst, 0, &BI);
    }
  } else {
    // For an intrinsic, check the arg info of each arg to see if we can
    // bale into it.
    GenXIntrinsicInfo Info(IntrinID);
    for (auto &AI : Info.getInstDesc()) {
      if (AI.isArgOrRet() && !AI.isRet()) {
        unsigned ArgIdx = AI.getArgIdx();
        switch (AI.getCategory()) {
          case GenXIntrinsicInfo::GENERAL:
            // This source operand of the intrinsic is general.
            if (operandCanBeBaled(Inst, ArgIdx, AI.getModifier(), AI.Info)) {
              LLVM_DEBUG(llvm::dbgs()
                         << __FUNCTION__ << " setting operand #" << ArgIdx
                         << " to bale in instruction " << *Inst << "\n");
              setOperandBaled(Inst, ArgIdx, &BI);
            }
            break;
          case GenXIntrinsicInfo::RAW:
            if (isa<Instruction>(Inst->getOperand(ArgIdx)) &&
                !genx::isSafeToSink_CheckAVLoadKill(
                    cast<Instruction>(Inst->getOperand(ArgIdx)), Inst, this))
              continue;
            // Rdregion can be baled in to a raw operand as long as it is
            // unstrided and starts on a GRF boundary. Ensure that the input to
            // the rdregion is 32 aligned.
            if (isValueRegionOKForRaw(Inst->getOperand(ArgIdx),
                                      /*IsWrite=*/false, ST)) {
              LLVM_DEBUG(llvm::dbgs()
                         << __FUNCTION__ << " setting operand #" << ArgIdx
                         << " to bale in instruction " << *Inst << "\n");
              setOperandBaled(Inst, ArgIdx, &BI);
              if (Liveness) {
                Value *Opnd = Inst->getOperand(ArgIdx);
                Opnd = cast<Instruction>(Opnd)->getOperand(0);
                Liveness->getOrCreateLiveRange(Opnd)->LogAlignment =
                    getLogAlignment(VISA_Align::ALIGN_GRF,
                                    ST ? ST->getGRFByteSize() : defaultGRFByteSize);
              }
            }
            break;
          case GenXIntrinsicInfo::TWOADDR:
            if (Kind == BalingKind::BK_CodeGen) {
              // Record this as a two address send for processing later.
              TwoAddrSends.push_back(cast<CallInst>(Inst));
            }
            break;
          case GenXIntrinsicInfo::PREDICATION:
            // See if there is any baling in to the predicate (mask) operand.
            if (processPredicate(Inst, ArgIdx)) {
              LLVM_DEBUG(llvm::dbgs()
                         << __FUNCTION__ << " setting operand #" << ArgIdx
                         << " to bale in instruction " << *Inst << "\n");
              setOperandBaled(Inst, ArgIdx, &BI);
            }
            break;
        }
      }
    }
  }

  // If this instruction is a modifier, we attempt to simplify it here
  // (i.e. fold constants), to avoid confusion later in GenXCisaBuilder
  // if a modifier has a constant operand. Because this pass scans code
  // forwards, a constant will propagate through a chain of modifiers.
  // TODO: Consider avoiding this section under Kind == BalingKind::BK_Analysis
  if (BI.Type != BaleInfo::MAININST) {
    Value *Simplified = nullptr;
    if (BI.Type != BaleInfo::ABSMOD) {
      const DataLayout &DL = Inst->getModule()->getDataLayout();
      Simplified = IGCLLVM::simplifyInstruction(Inst, SimplifyQuery(DL));
    } else {
      // SimplifyInstruction does not work on abs, so we roll our own for now.
      if (auto C = dyn_cast<Constant>(Inst->getOperand(0))) {
        if (C->getType()->isIntOrIntVectorTy()) {
          if (!ConstantExpr::getICmp(CmpInst::ICMP_SLT, C,
              Constant::getNullValue(C->getType()))->isNullValue())

            C = ConstantExpr::getNeg(C);
        } else {
          if (!ConstantExpr::getFCmp(CmpInst::FCMP_OLT, C,
                Constant::getNullValue(C->getType()))->isNullValue()) {
                C = llvm::ConstantFoldUnaryOpOperand(llvm::Instruction::FNeg, C, Inst->getModule()->getDataLayout());
            }
        }
        Simplified = C;
      }
    }
    if (Simplified) {
      IGC_ASSERT_MESSAGE(isa<Constant>(Simplified),
                         "expecting a constant when simplifying a modifier");
      Inst->replaceAllUsesWith(Simplified);
      Inst->eraseFromParent();
      return;
    }
  }

  // Only give an instruction an entry in the map if (a) it is not a main
  // instruction or (b) it bales something in.
  if (BI.Type || BI.Bits)
    setBaleInfo(Inst, BI);
}

/***********************************************************************
 * processSelectToPredicate : convert select to predicated wrregion.
 */
bool GenXBaling::processSelectToPredicate(SelectInst *SI) {
  if (!SI->getCondition()->getType()->isVectorTy())
    return false;

  // TODO: check also wrconstregion?
  auto IsConstantLike = [](Value *Val) {
    if (isa<Constant>(Val))
      return true;
    auto Inst = dyn_cast<Instruction>(Val);
    if (!Inst)
      return false;
    auto CI = dyn_cast<CallInst>(Val);
    if (!CI)
      return false;
    auto IntrinID = vc::getAnyIntrinsicID(CI);
    return IntrinID == GenXIntrinsic::genx_constanti ||
           IntrinID == GenXIntrinsic::genx_constantf;
  };

  if (IsConstantLike(SI->getTrueValue()) ||
      IsConstantLike(SI->getFalseValue()))
    return false;

  Region WrReg(SI);
  WrReg.Mask = SI->getCondition();

  for (Value *NewVal : { SI->getTrueValue(), SI->getFalseValue() }) {
    if (!NewVal->hasOneUse() || !isBalableNewValueIntoWrr(NewVal, WrReg))
      continue;
    bool IsInverted = NewVal == SI->getFalseValue();
    Value *OldVal = IsInverted ? SI->getTrueValue() : SI->getFalseValue();
    if (IsInverted)
      WrReg.Mask = invertCondition(WrReg.Mask);
    auto *WrRegion = WrReg.createWrRegion(OldVal, NewVal, SI->getName(), SI,
                                          SI->getDebugLoc());
    SI->replaceAllUsesWith(WrRegion);
    // Adjust liveness info if it presented.
    if (Liveness)
      Liveness->replaceValue(SI, WrRegion);
    SI->eraseFromParent();
    processWrRegion(WrRegion);
    return true;
  }
  // No conversion.
  return false;
}

/***********************************************************************
 * processBranch : process a branch instruction
 *
 * If the branch is conditional, bale in all/any/not
 */
void GenXBaling::processBranch(BranchInst *Branch)
{
  if (Branch->isConditional()) {
    BaleInfo BI(BaleInfo::MAININST);
    if (processPredicate(Branch, 0/*OperandNum of predicate*/)) {
      LLVM_DEBUG(llvm::dbgs() << __FUNCTION__ << " setting operand #" << 0
                              << " to bale in instruction " << *Branch << "\n");
      setOperandBaled(Branch, 0/*OperandNum*/, &BI);
      setBaleInfo(Branch, BI);
    }
  }
}

/***********************************************************************
 * processTwoAddrSend : process a two-address send
 *
 * A "two-address send" is a send (or an intrinsic that becomes a send in the
 * finalizer) with a potentially partial write, so it has a TWOADDR operand to
 * represent the value of the destination before the operation, and that
 * TWOADDR operand is not undef.
 *
 * This only gets called in the second baling pass.
 *
 * We can bale a rdregion into the TWOADDR operand and bale the send into a
 * wrregion, but only if the two have the same region and "old value" input.
 *
 * We used to allow such baling in first baling, such that legalization would
 * then not split the rdregion and wrregion. In bug 4607, we ran into a problem
 * where code changed due to vector decomposition, and the same baling did not
 * happen in second baling, leaving an illegally wide rdregion or wrregion.
 *
 * So now we only do this special kind of baling in the second baling pass.
 * That means that we have to detect where the rdregion and wrregion have been
 * split by legalization. We use the RdWrRegionSequence class to do that.
 */
void GenXBaling::processTwoAddrSend(CallInst *CI)
{
  auto TwoAddrOperandNum = vc::getTwoAddrOpIndex(CI);
  IGC_ASSERT_EXIT(TwoAddrOperandNum >= 0);
  IGC_ASSERT(GenXIntrinsicInfo(vc::getAnyIntrinsicID(CI))
      .getArgInfo(TwoAddrOperandNum)
      .getCategory() == GenXIntrinsicInfo::TWOADDR);
  IGC_ASSERT(GenXIntrinsicInfo(vc::getAnyIntrinsicID(CI))
      .getRetInfo()
      .getCategory() == GenXIntrinsicInfo::RAW);
  // First check the case where legalization did not need to split the rdregion
  // and wrregion.
  auto TwoAddrOperand = dyn_cast<Instruction>(CI->getArgOperand(TwoAddrOperandNum));
  if (!TwoAddrOperand)
    return;
  if (GenXIntrinsic::isRdRegion(TwoAddrOperand)) {
    if (!CI->hasOneUse())
      return;
    auto Rd = cast<Instruction>(TwoAddrOperand);
    auto Wr = cast<Instruction>(CI->use_begin()->getUser());
    if (!GenXIntrinsic::isWrRegion(Wr))
      return;
    if (CI->use_begin()->getOperandNo()
        != GenXIntrinsic::GenXRegion::NewValueOperandNum)
      return;
    Region RdR = genx::makeRegionFromBaleInfo(Rd, BaleInfo());
    Region WrR = genx::makeRegionFromBaleInfo(Wr, BaleInfo());
    if (RdR != WrR || RdR.Indirect || WrR.Mask)
      return;
    if (!isValueRegionOKForRaw(Wr, /*IsWrite=*/true, ST))
      return;
    // Everything else is in place for a rd-send-wr baling. We just need to check
    // that the input to the read sequence is the same as the old value input to
    // the write sequence.  We need to allow for some bitcasts in the way. Having
    // different bitcasts on the two inputs is ok, as long as the original value
    // is the same, because bitcasts are always copy coalesced so will be in the
    // same register.
    Value *RdIn = genx::getBitCastedValue(
        Rd->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum));
    Value *WrIn = genx::getBitCastedValue(
        Wr->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum));
    if (RdIn != WrIn)
      return;
    // We can do the baling.
    auto BI = getBaleInfo(CI);
    LLVM_DEBUG(llvm::dbgs()
               << __FUNCTION__ << " setting operand #" << TwoAddrOperandNum
               << " to bale in instruction " << *CI << "\n");
    setOperandBaled(CI, TwoAddrOperandNum, &BI);
    setBaleInfo(CI, BI);
    BI = getBaleInfo(Wr);
    LLVM_DEBUG(llvm::dbgs()
               << __FUNCTION__ << " setting operand #" << TwoAddrOperandNum
               << " to bale in instruction " << *Wr << "\n");
    setOperandBaled(Wr, GenXIntrinsic::GenXRegion::NewValueOperandNum, &BI);
    setBaleInfo(Wr, BI);
    return;
  }
  // Second, check the case where legalization has split the rdregion and
  // wrregion.
  if (CI->use_empty())
      return;
  if (!GenXIntrinsic::isWrRegion(TwoAddrOperand))
    return;
  RdWrRegionSequence RdSeq;
  if (!RdSeq.buildFromWr(TwoAddrOperand, this))
    return;
  RdWrRegionSequence WrSeq;
  auto Rd = cast<Instruction>(CI->use_begin()->getUser());
  const DataLayout &DL = CI->getModule()->getDataLayout();
  if (!GenXIntrinsic::isRdRegion(Rd))
    return;
  if (!WrSeq.buildFromRd(Rd, this))
    return;
  if (!RdSeq.WrR.isWhole(CI->getType(), &DL))
    return;
  if (!WrSeq.RdR.isWhole(CI->getType(), &DL))
    return;
  if (RdSeq.RdR.Indirect || WrSeq.WrR.Indirect)
    return;
  if (RdSeq.RdR != WrSeq.WrR)
    return;
  // Everything else is in place for a rd-send-wr baling. We just need to check
  // that the input to the read sequence is the same as the old value input to
  // the write sequence.  We need to allow for some bitcasts in the way. Having
  // different bitcasts on the two inputs is ok, as long as the original value
  // is the same, because bitcasts are always copy coalesced so will be in the
  // same register.
  Value *RdIn = genx::getBitCastedValue(RdSeq.Input);
  Value *WrIn = genx::getBitCastedValue(WrSeq.OldVal);
  if (RdIn != WrIn)
    return;
  // Check that there are no uses of CI other than in WrSeq. We can do that by
  // counting the uses.
  unsigned NumUses = 0, Size = WrSeq.size();
  for (auto ui = CI->use_begin(), ue = CI->use_end(); ui != ue; ++ui)
    if (++NumUses > Size)
      return;
  // We can bale, but we need to unlegalize back to a single rdregion and
  // single wrregion.
  auto NewRd = RdSeq.RdR.createRdRegion(RdSeq.Input, RdSeq.StartWr->getName(),
      RdSeq.StartWr, RdSeq.StartWr->getDebugLoc());
  CI->setOperand(TwoAddrOperandNum, NewRd);
  auto NewWr =
      WrSeq.WrR.createWrRegion(WrSeq.OldVal, CI, WrSeq.StartWr->getName(),
                               WrSeq.StartWr, WrSeq.StartWr->getDebugLoc());
  WrSeq.EndWr->replaceAllUsesWith(NewWr);
  // Set baling info for new instructions. The BI for NewWr is just a copy of
  // the first wrregion in the sequence being replaced.
  setBaleInfo(NewWr, getBaleInfo(WrSeq.StartWr));
  auto BI = getBaleInfo(CI);
  LLVM_DEBUG(llvm::dbgs() << __FUNCTION__ << " setting operand #"
                          << TwoAddrOperandNum << " to bale in instruction "
                          << *CI << "\n");
  setOperandBaled(CI, TwoAddrOperandNum, &BI);
  setBaleInfo(CI, BI);
  // Remove original sequences if now unused.
  for (Instruction *End = RdSeq.EndWr;;) {
    for (Instruction *Wr = End; Wr && Wr->use_empty(); ) {
      if (!Wr->use_empty())
        break;
      if (Wr->getNumOperands() < 2)
        break;
      auto Rd = dyn_cast<Instruction>(Wr->getOperand(1));
      auto NextWr = dyn_cast<Instruction>(Wr->getOperand(0));
      if (Liveness)
        Liveness->eraseLiveRange(Wr);
      Wr->eraseFromParent();
      IGC_ASSERT(Rd);
      if (Rd->use_empty()) {
        if (Liveness)
          Liveness->eraseLiveRange(Rd);
        Rd->eraseFromParent();
      }
      Wr = NextWr;
    }
    if (End == WrSeq.EndWr)
      break;
    End = WrSeq.EndWr;
  }
}

/***********************************************************************
 * setBaleInfo : set BaleInfo for an instruction
 */
void GenXBaling::setBaleInfo(const Instruction *Inst, genx::BaleInfo BI)
{
  IGC_ASSERT(BI.Bits < 1 << Inst->getNumOperands());
  LLVM_DEBUG(llvm::dbgs() << "Adding InstMap entry for " << *Inst
                          << "; BI type: " << BI.getTypeString() << "\n");
  InstMap[Inst] = BI;
}

/***********************************************************************
 * setOperandBaled : set flag to say that an operand is baled in
 *
 * Enter:   Inst = instruction to bale into
 *          OperandNum = operand number in that instruction
 *          BI = BaleInfo to set flag in
 *
 * If the operand value has multiple uses, this also flags that we will need
 * to do some cloning afterwards to ensure that a baled in operand has a
 * single use.
 *
 * Note that a main instruction baled into a saturate modifier or into
 * a wrregion, or a saturate modifier baled into a wrregion, never has
 * multiple uses. So the multiple use thing only covers source operands
 * of the main inst, plus a possible addradd in the wrregion.
 */
void GenXBaling::setOperandBaled(Instruction *Inst, unsigned OperandNum,
    BaleInfo *BI)
{
  // Set the bit.
  BI->Bits |= 1 << OperandNum;
  // Check whether the operand has more than one use.
  Instruction *BaledInst = cast<Instruction>(Inst->getOperand(OperandNum));
  if (!BaledInst->hasOneUse()) {
    // Multiple uses. Add to the NeedClone stack. But not if it is a goto/join;
    // we allow a goto/join to be baled into the extract of its !any(EM) result
    // even though it has uses in other extracts.
    unsigned IID = vc::getAnyIntrinsicID(BaledInst);
    if (IID != GenXIntrinsic::genx_simdcf_goto &&
        IID != GenXIntrinsic::genx_simdcf_join)
      NeedCloneStack.push_back(NeedClone(Inst, OperandNum));
  }
}

/***********************************************************************
 * doClones : do any cloning required to make baled in instructions
 *            single use
 *
 * NeedCloneStack is a stack of operands (instruction and operand number
 * pairs) that are baled in and have more than one use, so need cloning.
 * They were pushed in forward order, so if A is baled into B is baled
 * into C then the use of A in B was pushed before the use of B in C.
 *
 * We now pop off the stack in reverse order. We see the use of B in C,
 * and clone B to single use B'. Then we see that B bales in A, so we
 * add the use of A in B' onto the stack, causing A to be cloned later.
 * In this way we handle nested baling correctly.
 */
void GenXBaling::doClones()
{
  while (NeedCloneStack.size()) {
    // Pop a NeedClone off the stack.
    NeedClone NC = NeedCloneStack.pop_back_val();
    // See if it is still multiple use (earlier cloning may have caused this
    // one to become single use).
    Instruction *Opnd = cast<Instruction>(NC.Inst->getOperand(NC.OperandNum));
    if (Opnd->hasOneUse())
      continue;
    // See if it is still baled. But continue with cloning even if not baled in
    // these cases:
    // 1. An extend (zext or sext), because it tends to result in better gen
    //    code, probably because a zext or sext can be baled in to its user by
    //    the finalizer in a case where we cannot because of the vISA
    //    restriction that both operands need the same extend. This case arises
    //    only if we were going to bale the extend in, but then decided not to
    //    because the two operands did not have the same extend.
    // 2. An address generating instruction, because, at this point in the flow
    //    (between GenXCategory and GenXAddressCommoning), an address
    //    generating instruction must have a single use.
    bool IsBaled = getBaleInfo(NC.Inst).isOperandBaled(NC.OperandNum);
    if (!IsBaled && !isa<CastInst>(Opnd) &&
        getAddrOperandNum(vc::getAnyIntrinsicID(NC.Inst)) != (int)NC.OperandNum)
      continue;
    // Clone it.
    IGC_ASSERT(!isa<PHINode>(Opnd));
    Instruction *Cloned = Opnd->clone();
    Cloned->setName(Opnd->getName());
    // We don't want to simply clone rdregions from read_predef_reg
    // as it may cause overwriting of that predef reg.
    // That's why we want to create wrregion to keep this value in a temp var
    // But to do this only when necessary we can't do this earlier
    // (in PrologEpilogInsertion pass)
    if (GenXIntrinsic::isRdRegion(Cloned) &&
        GenXIntrinsic::isReadPredefReg(Cloned->getOperand(
            GenXIntrinsic::GenXRegion::OldValueOperandNum))) {
      auto *ReadPredef = cast<Instruction>(
          Cloned->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum));

      Instruction *ClonedReadPredef = ReadPredef->clone();
      ClonedReadPredef->insertAfter(ReadPredef);
      BaleInfo BI(BaleInfo::REGINTR);
      setBaleInfo(ClonedReadPredef, BI);
      Cloned->setOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum,
                         ClonedReadPredef);
    }
    // Change the use.
    NC.Inst->setOperand(NC.OperandNum, Cloned);
    if (IsBaled) {
      // Normally, insert the cloned instruction just after the original.
      Cloned->insertAfter(Opnd);
    } else {
      // In the special case that we are cloning something even when not baled:
      // Ensure the cloned instruction has the same category as the original
      // one.
      if (Liveness)
        Liveness->getOrCreateLiveRange(Cloned)->setCategory(
            Liveness->getOrCreateLiveRange(Opnd)->getCategory());
      // Insert the clone just before its single use.
      Cloned->insertBefore(NC.Inst);
      // If the instruction that we cloned is now single use, not in a phi
      // node, move it to just before its use.
      if (Opnd->hasOneUse()) {
        auto User = Opnd->use_begin()->getUser();
        if (!isa<PHINode>(User)) {
          Opnd->removeFromParent();
          Opnd->insertBefore(cast<Instruction>(User));
        }
      }
    }
    // Copy the bale info.
    BaleInfo BI = getBaleInfo(Opnd);
    setBaleInfo(Cloned, BI);
    // Stack any operands of the cloned instruction that are baled. (They
    // must be multiple use because we have just cloned the instruction
    // using them.) Also any address calculation, for the reason given in the
    // comment above.
    int AON = getAddrOperandNum(vc::getAnyIntrinsicID(Cloned));
    for (unsigned i = 0, e = Cloned->getNumOperands(); i != e; ++i)
      if (BI.isOperandBaled(i) ||
          (Kind == BalingKind::BK_CodeGen && AON == (int)i &&
           isa<Instruction>(Cloned->getOperand(i))))
        NeedCloneStack.push_back(NeedClone(Cloned, i));
  }
}

/***********************************************************************
 * getOrUnbaleExtend : get or unbale the extend instruction (if any) in
 *                     this operand
 *
 * Enter:   Inst = instruction containing operand
 *          BI = BaleInfo for Inst
 *          OperandNum = operand number to look at
 *          Unbale = true to unbale the extend
 *
 * Return:  0 if no extend found, else the extend (ZExt or SExt), and, if
 *          Unbale is true, then *BI has been modified _and_ written back
 *          into Inst's map entry in GenXBaling.
 *
 * BI is a pointer to handle two slightly different cases of unbaling the ext:
 * 1. If this is the top level call to getOrUnBaleExtend from processMainInst,
 *    then we want to modify the caller's BaleInfo pointed to by BI, which the
 *    caller is in the middle of setting up and will write back into the map.
 * 2. If this is a recursive call from getOrUnbaleExtend, then we want to
 *    use setBaleInfo to write the BaleInfo back into the map.
 * We don't check which case we have, and we just do both things, as the
 * unneeded one is harmless.
 */
Instruction *GenXBaling::getOrUnbaleExtend(Instruction *Inst, BaleInfo *BI,
    unsigned OperandNum, bool Unbale)
{
  if (!BI->isOperandBaled(OperandNum))
    return nullptr;
  auto Opnd = cast<Instruction>(Inst->getOperand(OperandNum));
  if (isa<ZExtInst>(Opnd) || isa<SExtInst>(Opnd)) {
    // Found an extend. Unbale it if requested. But do not remove it from the
    // NeedClone stack; we still clone an extend that is not being baled in on
    // the basis that the jitter will be able to bale it in because gen allows
    // mismatched integer operand types.
    if (Unbale) {
      BI->clearOperandBaled(OperandNum);
      setBaleInfo(Inst, *BI);
    }
    return Opnd;
  }
  BaleInfo ThisBI = getBaleInfo(Opnd);
  if (ThisBI.isOperandBaled(0))
    return getOrUnbaleExtend(Opnd, &ThisBI, 0, Unbale);
  if (ThisBI.isOperandBaled(1))
    return getOrUnbaleExtend(Opnd, &ThisBI, 1, Unbale);
  return nullptr;
}

/***********************************************************************
 * dump, print : dump the result of the GenXBaling analysis
 */
#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
void GenXBaling::dump() const { print(errs()); }
#endif

void GenXBaling::print(raw_ostream &OS) const {
  // we need deterministic order of instructions for unit tests
  // so we are collecting functions and then traversing all instructions
  OS << "GenXBaling dump start\n";
  std::set<const Function *> Funcs;
  for (auto &&I : InstMap) {
    const auto *Inst = cast<Instruction>(I.first);
    IGC_ASSERT_MESSAGE(Inst->getParent(), "Instruction shall be in some BB");
    Funcs.insert(Inst->getFunction());
  }

  for (auto *F : Funcs) {
    OS << "bales in function: " << F->getName() << ":\n";
    for (auto &&Inst : instructions(F)) {
      auto InstMapIt = InstMap.find(&Inst);
      if (InstMapIt == InstMap.end())
        continue;
      const auto *BI = &InstMapIt->second;
      OS << Inst << ": ";
      OS << BI->getTypeString();
      for (unsigned OperandNum = 0, e = Inst.getNumOperands(); OperandNum != e;
           ++OperandNum)
        if (BI->isOperandBaled(OperandNum))
          OS << " " << OperandNum;
      OS << "\n";
    }
  }
  OS << "GenXBaling dump end\n";
}

/***********************************************************************
 * getBaleParent : return the instruction baled into, 0 if none
 */
Instruction *GenXBaling::getBaleParent(const Instruction *Inst) const {
  // We can rely on the fact that a baled in instruction always has exactly
  // one use. The exception is llvm.genx.simdcf.goto/join, which is baled in
  // to the extractvalue that extracts the !any(EM) value. Rather than check
  // the intrinsic ID, we check whether the return type is struct.
  auto use = Inst->use_begin();
  if (!Inst->hasOneUse()) {
    if (!isa<StructType>(Inst->getType()))
      return nullptr;
    // For an llvm.genx.simdcf.goto/join, the use we want is the extractvalue
    // that extracts the !any(EM) value from the result struct.
    for (auto ue = Inst->use_end();; ++use) {
      if (use == ue)
        return nullptr;
      if (!isa<ExtractValueInst>(use->getUser()))
        return nullptr;
      if (use->getUser()->getType()->isIntegerTy(1))
        break;
    }
  }
  Instruction *user = cast<Instruction>(use->getUser());
  BaleInfo BI = getBaleInfo(user);
  if (!BI.isOperandBaled(use->getOperandNo()))
    return nullptr;
  return cast<Instruction>(use->getUser());
}

/***********************************************************************
 * unbale : unbale an instruction from its bale parent
 */
void GenXBaling::unbale(Instruction *Inst)
{
  if (!Inst->hasOneUse())
    return;
  Value::use_iterator use = Inst->use_begin();
  Instruction *user = cast<Instruction>(use->getUser());
  BaleInfo BI = getBaleInfo(user);
  unsigned OperandNum = use->getOperandNo();
  if (!BI.isOperandBaled(OperandNum))
    return;
  BI.clearOperandBaled(OperandNum);
  setBaleInfo(user, BI);
}

/***********************************************************************
 * getBaleHead : return the head of the bale containing Inst
 */
Instruction *GenXBaling::getBaleHead(Instruction *I) const {
  while (auto *P = getBaleParent(I))
    I = P;
  return I;
}

/***********************************************************************
 * buildBale : populate a Bale from the head instruction
 *
 * Enter:   Inst = the head instruction
 *          B = Bale struct, assumed empty
 *          IncludeAddr = default false, true to include address calculations
 *                        even when not baled in
 *
 * IncludeAddr is used by GenXUnbaling to include the address calculation of
 * a rdregion in the bale, so it can be considered together when deciding
 * whether to unbale and move. This works because an address calculation has
 * exactly one use, until GenXAddressCommoning commons them up later.
 */
void GenXBaling::buildBale(Instruction *Inst, Bale *B, bool IncludeAddr) const
{
  IGC_ASSERT(!B->size());
  buildBaleSub(Inst, B, IncludeAddr);
}

void GenXBaling::buildBaleSub(Instruction *Inst, Bale *B, bool IncludeAddr) const
{
  BaleInfo BI = getBaleInfo(Inst);
  B->push_front(BaleInst(Inst, BI));

  if (isa<PHINode>(Inst) ||
      (isa<CallInst>(Inst) && !cast<CallInst>(Inst)->isInlineAsm() &&
       !vc::isAnyNonTrivialIntrinsic(Inst)))
    return;
  if (IncludeAddr) {
    int AddrOperandNum = getAddrOperandNum(GenXIntrinsic::getGenXIntrinsicID(Inst));
    if (AddrOperandNum >= 0) {
      // IncludeAddr: pretend that the address calculation is baled in, as long
      // as it is an instruction.
      if (auto OpndInst = dyn_cast<Instruction>(Inst->getOperand(AddrOperandNum))) {
        IGC_ASSERT(OpndInst->hasOneUse()); (void)OpndInst;
        BI.setOperandBaled(AddrOperandNum);
        B->front().Info = BI;
      }
    }
  }

  IGC_ASSERT(BI.Bits < (1 << Inst->getNumOperands()) || Inst->getNumOperands() > 16);

  while (BI.Bits) {
    unsigned Idx = genx::log2(BI.Bits);
    BI.Bits &= ~(1 << Idx);
    if (Instruction *Op = dyn_cast<Instruction>(Inst->getOperand(Idx)))
      buildBaleSub(Op, B, IncludeAddr);
  }
}

/***********************************************************************
 * getAddrOperandNum : given an intrinsic ID, get the address operand number
 *
 * For rdregion/wrregion, it returns the operand number of the index operand.
 *
 * For genx_add_addr, it returns 0 (the only operand number)
 *
 * In any other case, it returns -1.
 *
 * This is used both in buildBale when IncludeAddr is true, and in doClones,
 * to find the address operand of an instruction.
 */
int GenXBaling::getAddrOperandNum(unsigned IID) const
{
  switch (IID) {
    case GenXIntrinsic::genx_rdregioni:
    case GenXIntrinsic::genx_rdregionf:
      return GenXIntrinsic::GenXRegion::RdIndexOperandNum;
    case GenXIntrinsic::genx_wrregioni:
    case GenXIntrinsic::genx_wrregionf:
      return GenXIntrinsic::GenXRegion::WrIndexOperandNum;
    case GenXIntrinsic::genx_add_addr:
      return 0;
    default:
      return -1;
  }
}

/***********************************************************************
 * store : store updated BaleInfo for instruction
 *
 * Enter:   BI = BaleInst struct
 *
 * This function stores BI.Info as the new BaleInfo for BI.Inst
 *
 * It is used by GenXLegalization to unbale.
 */
void GenXBaling::store(BaleInst BI)
{
  IGC_ASSERT(BI.Info.Bits < 1<< BI.Inst->getNumOperands());
  InstMap[BI.Inst] = BI.Info;
}

static bool skipTransform(Instruction *DefI, Instruction *UseI) {
  SmallPtrSet<Instruction *, 8> DInsts;
  BasicBlock *BB = UseI->getParent();

  // Special case for extracting out of subroutine call.
  if (isa<ExtractValueInst>(DefI))
   return true;

  // This is a local optimization only.
  for (auto U : DefI->users()) {
    auto UI = dyn_cast<Instruction>(U);
    if (UI == nullptr || UI->getParent() != BB)
      return true;
    if (UI != UseI)
      DInsts.insert(UI);
  }

  // If a use is crossing the next region write,
  // then two regions are live at the same time.
  // Very likely this increases register pressure
  // and/or results region copies.
  //
  // Scan forward starting from Region write,
  // check if this hits a write to this region
  // before some use.
  //
  SmallPtrSet<Instruction *, 8> UInsts;
  bool IsLocal = !UseI->isUsedOutsideOfBlock(BB);
  if (IsLocal) {
    for (auto U : UseI->users()) {
      auto UI = dyn_cast<Instruction>(U);
      if (UI != nullptr)
        UInsts.insert(UI);
    }
  }

  for (auto I = UseI; I; I = I->getNextNode()) {
    if (I == &BB->back())
      break;
    if (DInsts.empty())
      break;

    // UInst is local and it is dead now.
    if (IsLocal && UInsts.empty())
      break;

    // There is a region write before some use.
    if (GenXIntrinsic::isWrRegion(I) &&
        I->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum) == UseI)
      return true;

    if (DInsts.count(I))
      DInsts.erase(I);
    if (UInsts.count(I))
      UInsts.erase(I);
  }

  // Not all users are checked which means UseI does not
  // dominate them, or UseI is local and dead before some uses.
  return !DInsts.empty();
}

// Normalize ill-formed gstores.
// Correct gstore should be in form of:
// x = gload G
// w = wrr x, StoreVal
// gstore w, G
static void normalizeGStore(StoreInst &SI) {
  Value *PointerOp = SI.getPointerOperand();
  auto LI = new LoadInst(SI.getValueOperand()->getType(), PointerOp, ".gload",
                         true /*volatile*/, &SI);
  Value *StoreOp = SI.getValueOperand();
  Region R(StoreOp);
  auto WrR =
      R.createWrRegion(LI, StoreOp, ".wrr.gstore", &SI, SI.getDebugLoc());
  SI.setOperand(0 /*Value operand idx*/, WrR);
}

// Normalize ill-formed g_stores.
static void normalizeGStores(Function &F) {
  for (auto &I : instructions(F)) {
    if (!isGlobalStore(&I))
      continue;
    auto *SI = cast<StoreInst>(&I);
    if (!isGlobalStoreLegal(SI))
      normalizeGStore(*SI);
  }
}

// If operand of gstore is phi and all its incoming values
// form legal values for gstore, then return true.
// All incoming blocks should have single successor.
// Otherwise return false.
static bool canPropagatePhiGStore(StoreInst &SI) {
  Value *Val = genx::getBitCastedValue(SI.getValueOperand());
  auto *Phi = dyn_cast<PHINode>(Val);
  if (!Phi)
    return false;

  if (!llvm::all_of(Phi->blocks(),
                    [](BasicBlock *BB) { return BB->getSingleSuccessor(); }))
    return false;

  Value *StorePtr = SI.getPointerOperand();
  // This can be weakened, but then new gstores should be normalized too.
  return llvm::all_of(Phi->incoming_values(), [StorePtr](Use &U) {
    return isLegalValueForGlobalStore(U, StorePtr);
  });
}

// Duplicate gstore in blocks with its legal value operands coming from phi.
// After that, there will be legal gstores that can be baled later.
// Old gstore with phi is deleted.
static void propagatePhiGStore(StoreInst &SI) {
  auto *StoreVal = SI.getValueOperand();
  auto *Phi = cast<PHINode>(genx::getBitCastedValue(StoreVal));
  for (Use &U : Phi->incoming_values()) {
    IRBuilder<> IRB(cast<Instruction>(U)->getParent()->getTerminator());
    IRB.CreateStore(IRB.CreateBitCast(U, StoreVal->getType()),
                    SI.getPointerOperand(), SI.isVolatile());
  }
  SI.eraseFromParent();
  RecursivelyDeleteTriviallyDeadInstructions(StoreVal);
}

// If g_store value comes from phi node, try to hoist it.
static void propagatePhiGStores(Function &F) {
  SmallVector<StoreInst *, 8> Worklist;
  for (auto &I : instructions(F)) {
    auto *SI = dyn_cast<StoreInst>(&I);
    if (!SI || !isGlobalStore(SI))
      continue;
    if (canPropagatePhiGStore(*SI))
      Worklist.push_back(SI);
  }
  for (auto *SI : Worklist)
    propagatePhiGStore(*SI);
}

// Cleanup and optimization before do baling on a function.
bool GenXBaling::preBalingCleanAndOptimize(Function &F) {
  bool Changed = false;
  auto nextInst = [](BasicBlock &BB, Instruction *I) -> Instruction * {
    // This looks like an llvm bug. We cannot call getPrevNode
    // on the first instruction...
    if (isa<PHINode>(I) || I == &BB.front())
      return nullptr;
    return I->getPrevNode();
  };

  for (auto &BB : F) {
    // scan the block backwards.
    for (auto Inst = &BB.back(); Inst; Inst = nextInst(BB, Inst)) {
      //
      // Rewrite
      // A = B op C
      // V = wrr(A, R)
      // E = A op D
      // into
      //
      // A = B op C
      // V = wrr(A, R)
      // A' = rrd(V, R)
      // E = A' op D
      //
      if (GenXIntrinsic::isWrRegion(Inst)) {
        Instruction *V = dyn_cast<Instruction>(
            Inst->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum));

        // Only process the case with multiple uses.
        if (!V || V->hasOneUse())
          continue;

        // Skip if this region write is indirect as
        // this would result an indirect read.
        Region R = makeRegionFromBaleInfo(Inst, BaleInfo());
        if (R.Indirect)
          continue;

        // Aggressively apply this transform may increase register pressure.
        // We detect if there is other region write in between, so that two
        // outer regions will not be live at the same time.
        if (skipTransform(V, Inst))
          continue;

        // Do not transform if the predicate is not trivial.
        Value *Mask =
            Inst->getOperand(GenXIntrinsic::GenXRegion::PredicateOperandNum);
        auto *ConstantMask = dyn_cast<Constant>(Mask);
        if (!ConstantMask || !ConstantMask->isAllOnesValue())
          continue;

        // Do this transformation.
        // - Insert a region read right after Inst
        // - Replace all uses other than Inst with this region read
        //
        auto NewV = R.createRdRegion(Inst, "split", Inst, Inst->getDebugLoc(),
                                     /*AllowScalar*/ !V->getType()->isVectorTy());

        IGC_ASSERT(NewV->getType() == V->getType());
        Inst->moveBefore(NewV);
        for (auto UI = V->use_begin(); UI != V->use_end(); /*Empty*/) {
          Use &U = *UI++;
          if (U.getUser() != Inst)
            U.set(NewV);
        }
        Changed = true;
      }
    }
  }

  propagatePhiGStores(F);

  // fold bitcast into store/load if any. This allows to bale a g_store instruction
  // crossing a bitcast.
  for (auto &BB : F) {
    for (auto I = BB.begin(); I != BB.end(); /*empty*/) {
      Instruction *Inst = &*I++;
      using namespace llvm::PatternMatch;

      // bitcast (bitcast X to Ty1) to Ty2 ==> bitcast X to Ty2
      Value *X;
      if (match(Inst, m_BitCast(m_BitCast(m_Value(X))))) {
        // matches bitcast from predicate - false positive
        if (X->getType()->getScalarType()->isIntegerTy(1) &&
            Inst->getType()->isVectorTy() &&
            !Inst->getType()->getScalarType()->isIntegerTy(1))
          continue;

        BitCastInst *NewCI = new BitCastInst(X, Inst->getType(), "", Inst);
        NewCI->setDebugLoc(Inst->getDebugLoc());
        NewCI->takeName(Inst);
        Inst->replaceAllUsesWith(NewCI);
        if (Liveness)
          Liveness->eraseLiveRange(Inst);
        Inst->eraseFromParent();
        Changed = true;
        continue;
      }

      if (isa<LoadInst>(Inst) || isa<StoreInst>(Inst)) {
        Instruction* NewInst = foldBitCastInst(Inst);
        if (NewInst) {
          Changed = true;
          Inst = NewInst;
        }
      }

      // Delete Trivially dead store instructions.
      if (auto ST = dyn_cast<StoreInst>(Inst)) {
        Value *Val = ST->getValueOperand();
        IGC_ASSERT(Val);
        if (auto LI = dyn_cast<LoadInst>(Val)) {
          Value *Ptr = ST->getPointerOperand();
          auto *GV1 = vc::getUnderlyingGlobalVariable(Ptr);
          auto *GV2 = vc::getUnderlyingGlobalVariable(LI->getPointerOperand());
          if (GV1 && GV1 == GV2) {
            ST->eraseFromParent();
            Changed = true;
          }
        }
      }
    }

    // TODO: Consider avoiding this section under Kind ==
    // BalingKind::BK_Analysis
    for (auto I = BB.rbegin(); I != BB.rend(); /*empty*/) {
      Instruction *Inst = &*I++;
      if (isInstructionTriviallyDead(Inst)) {
        if (Liveness)
          Liveness->eraseLiveRange(Inst);
        Inst->eraseFromParent();
      }
    }
  }

  // Result of inline asm with multiple outputs is a
  // struct. Each element of this struct passed to
  // it's user with extractelement instruction which
  // should be baled into it's own WrRegion. genx_convert_addr
  // intrinsic or global load will be unbaled between these bales
  // of (extractelement + wrregion). The idea is to
  // move all of those address conversion before
  // the inline assembly instruction. Also each extractvalue and WrR which uses
  // extractvalue's result should be moved closely to inline assembly call,
  // otherwise baling will force baled instructions
  // to locate far away from inline asm call
  // which will lead to live range increasing.
  for (auto &BB : F) {
    for (auto &Inst : BB) {
      auto CI = dyn_cast<CallInst>(&Inst);
      if (!CI || !CI->isInlineAsm())
        continue;
      // Nothing to do if result is not a struct: no multiple outputs
      if (!isa<StructType>(CI->getType()))
        continue;
      for (auto ui = CI->use_begin(), ue = CI->use_end(); ui != ue; ++ui) {
        auto EV = dyn_cast<ExtractValueInst>(ui->getUser());
        if (!EV)
          continue;
        EV->moveAfter(&Inst);
        // extractelement must be baled into wrregion
        for (auto User : EV->users()) {
          Changed = true;
          if (!GenXIntrinsic::isWrRegion(User))
            continue;
          Instruction *WrR = cast<Instruction>(User);
          Value *Index =
              WrR->getOperand(GenXIntrinsic::GenXRegion::WrIndexOperandNum);
          while (GenXIntrinsic::getGenXIntrinsicID(Index) ==
                 GenXIntrinsic::genx_add_addr)
            Index = cast<Instruction>(Index)->getOperand(0);
          Instruction *IndexInst = dyn_cast<Instruction>(Index);
          if (IndexInst && (vc::getAnyIntrinsicID(IndexInst) ==
                            GenXIntrinsic::genx_convert_addr))
            IndexInst->moveBefore(&Inst);
          Value *OldVal =
              WrR->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
          LoadInst *Load = dyn_cast<LoadInst>(OldVal);
          if (Load && isGlobalLoad(Load))
            Load->moveBefore(&Inst);
          WrR->moveAfter(EV);
        }
      }
    }
  }

  normalizeGStores(F);

  // Remove Phi node with single incoming value
  for (auto &BB : F) {
    for (BasicBlock::iterator bi = BB.begin(), be = BB.end(); bi != be; ) {
      Instruction *Inst = &*bi;
      ++bi;
      if (auto Phi = dyn_cast<PHINode>(Inst)) {
        if (Phi->getNumIncomingValues() == 1) {
          Phi->replaceAllUsesWith(Phi->getIncomingValue(0));
          Phi->eraseFromParent();
          Changed = true;
        }
      }
      else {
        break;
      }
    }
  }

  return Changed;
}

/***********************************************************************
 * Bale::getMainInst : get the main instruction from the bale, 0 if none
 */
const BaleInst *Bale::getMainInst() const
{
  // From the last instruction (the bale head) backwards, find the first
  // one that is not wrregion or saturate or addradd. If the head is
  // wrregion, then skip anything before we reach its value operand.
  // If the first one we find is rdregion, that does not count as a main
  // instruction.
  Value *PossibleMainInst = nullptr;
  for (const_reverse_iterator i = rbegin(), e = rend(); i != e; ++i) {
    if (PossibleMainInst && PossibleMainInst != i->Inst)
      continue;
    PossibleMainInst = nullptr;
    switch (i->Info.Type) {
      case BaleInfo::WRREGION:
        PossibleMainInst = i->Inst->getOperand(1);
        break;
      case BaleInfo::GSTORE:
        PossibleMainInst = i->Inst->getOperand(0);
        break;
      case BaleInfo::SATURATE:
      case BaleInfo::ADDRADD:
        break;
      case BaleInfo::MAININST:
        return &*i;
      default:
        return nullptr;
    }
  }
  return nullptr;
}

/***********************************************************************
 * eraseFromParent : do eraseFromParent on all instructions in the bale
 */
void Bale::eraseFromParent()
{
  // Iterate in reverse as each instruction becomes unused only when its
  // user in the bale is erased.
  for (reverse_iterator ri = rbegin(), re = rend(); ri != re; ++ri)
    ri->Inst->eraseFromParent();
}

/***********************************************************************
 * Bale::compare : compare this Bale with another one
 *
 * Return:  0 if equivalent
 *          < 0 if less
 *          > 0 if more
 *
 * Two Bales are equivalent if they compute the same value, that is, they
 * have the same opcodes in the instructions, the instructions are
 * baled together in the same way, and the operands coming in from outside
 * the bale are the same.
 *
 * Both bales must have had hash() called on them since being built or
 * modified in any other way.
 */
int Bale::compare(const Bale &Other) const
{
  IGC_ASSERT(Hash && Other.Hash);
  if (Hash != Other.Hash)
    return Hash < Other.Hash ? -1 : 1;
  if (size() != Other.size())
    return size() < Other.size() ? -1 : 1;
  for (unsigned i = 0, e = size(); i != e; ++i) {
    if (Insts[i].Info.Bits != Other.Insts[i].Info.Bits)
      return Insts[i].Info.Bits < Other.Insts[i].Info.Bits ? -1 : 1;
    Instruction *Inst = Insts[i].Inst, *OtherInst = Other.Insts[i].Inst;
    if (Inst->getOpcode() != OtherInst->getOpcode())
      return Inst->getOpcode() < OtherInst->getOpcode() ? -1 : 1;
    unsigned NumOperands = Inst->getNumOperands();
    if (NumOperands != OtherInst->getNumOperands())
      return NumOperands < OtherInst->getNumOperands() ? -1 : 1;
    for (unsigned j = 0; j != NumOperands; ++j) {
      Value *Opnd = Inst->getOperand(j);
      if (!Insts[i].Info.isOperandBaled(j)) {
        if (Opnd != OtherInst->getOperand(j))
          return Opnd < OtherInst->getOperand(j) ? -1 : 1;
      } else {
        // Baled operand. Find which baled instruction it is, and check that
        // the other bale has its corresponding instruction used in its
        // corresponding operand.
        // (We could use a map to find the baled instruction
        // in an algorithmically less complex way, but there is not likely
        // to be more than 3 or 4 instructions in the bale so I didn't
        // bother.)
        unsigned BaledInst;
        for (BaledInst = 0; Insts[BaledInst].Inst != Opnd; ++BaledInst) {
          IGC_ASSERT(BaledInst != size());
        }
        if (Other.Insts[BaledInst].Inst != OtherInst->getOperand(j))
          return Other.Insts[BaledInst].Inst < OtherInst->getOperand(j) ? -1 : 1;
      }
    }
  }
  return 0;
}

/***********************************************************************
 * hash_value : get a hash_code for a Bale
 *
 * If two Bales are equivalent, they have the same hash_value.
 *
 * If two Bales are not equivalent, it is unlikely but possible that
 * they have the same hash_value.
 */
void Bale::hash()
{
  Hash = 0;
  for (auto i = begin(), e = end(); i != e; ++i) {
    BaleInst BI = *i;
    Hash = hash_combine(Hash, BI.Info.Bits);
    Hash = hash_combine(Hash, BI.Inst->getOpcode());
    for (unsigned j = 0, je = BI.Inst->getNumOperands(); j != je; ++j) {
      Value *Opnd = BI.Inst->getOperand(j);
      if (!BI.Info.isOperandBaled(j)) {
        // Non-baled operand. Hash the operand itself.
        Hash = hash_combine(Hash, Opnd);
      } else {
        // Baled operand. Find which baled instruction it is, and use that
        // index in the hash. (We could use a map to find the baled instruction
        // in an algorithmically less complex way, but there is not likely
        // to be more than 3 or 4 instructions in the bale so I didn't
        // bother.)
        Bale::iterator BaledInst;
        for (BaledInst = begin(); BaledInst->Inst != Opnd; ++BaledInst) {
          IGC_ASSERT(BaledInst != i);
        }
        Hash = hash_combine(Hash, BaledInst - begin());
      }
    }
  }
}

bool Bale::isGStoreBaleLegal() const {
  IGC_ASSERT(isGStoreBale());
  auto ST = cast<StoreInst>(getHead()->Inst);
  if (!isGlobalStore(ST))
    return false;
  return isGlobalStoreLegal(ST);
}

llvm::SmallPtrSet<const Instruction *, 2> Bale::getVLoadSources() const {
  llvm::SmallPtrSet<const Instruction *, 2> res;
  for (const auto &Inst : Insts)
    for (const auto &VLoad : genx::getSrcVLoads(Inst.Inst))
      res.insert(VLoad);
  return res;
}

bool Bale::hasVLoadSources() const { return getVLoadSources().size(); }

/***********************************************************************
 * Bale debug dump/print
 */
#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
void Bale::dump() const
{
  print(errs());
}
#endif

void Bale::print(raw_ostream &OS) const
{
  OS << "bale {\n";
  for (const_iterator i = begin(), e = end(); i != e; ++i) {
    i->Inst->print(OS);
    OS << " // {" << i->Info.getTypeString() << "}\n";
  }
  OS << "}\n";
}

const char *BaleInfo::getTypeString() const
{
  switch (Type) {
    case BaleInfo::MAININST: return "maininst";
    case BaleInfo::WRREGION: return "wrregion";
    case BaleInfo::SATURATE: return "saturate";
    case BaleInfo::NOTMOD: return "notmod";
    case BaleInfo::NEGMOD: return "negmod";
    case BaleInfo::ABSMOD: return "absmod";
    case BaleInfo::RDREGION: return "rdregion";
    case BaleInfo::ADDRADD: return "addradd";
    case BaleInfo::RDPREDREGION: return "rdpredregion";
    case BaleInfo::ALLANY: return "allany";
    case BaleInfo::NOTP: return "notp";
    case BaleInfo::ZEXT: return "zext";
    case BaleInfo::SEXT: return "sext";
    case BaleInfo::WRPREDREGION: return "wrpreregion";
    case BaleInfo::CMPDST: return "cmpdst";
    case BaleInfo::GSTORE: return "g_store";
    case BaleInfo::SHUFFLEPRED: return "shufflepred";
    default: return "???";
  }
}

bool genx::operator==(const BaleInfo &lhs, const BaleInfo &rhs) {
  return lhs.Type == rhs.Type && lhs.Bits == rhs.Bits;
}

bool genx::operator==(const BaleInst &lhs, const BaleInst &rhs) {
  return lhs.Inst == rhs.Inst && lhs.Info == rhs.Info;
}
