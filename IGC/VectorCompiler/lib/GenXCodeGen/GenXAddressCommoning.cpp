/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXAddressCommoning
/// --------------------
///
/// This pass spots when multiple address conversions use the same value and
/// are used in regions with the same base register (the same coalesced live
/// range), and commons up the address conversions.
///
/// It also handles cases where an llvm.genx.add.addr has an out of range offset
/// that is not encodable as the constant offset in an indirect operand. When
/// commoning up address conversions, it groups ones with nearby offsets such
/// that all uses of a commoned address conversion have in range offsets in
/// their llvm.genx.add.addr ops.
///
/// Before this pass, GenXCategoryConversion has ensured that each use of a
/// variable index in an element or region access (llvm.genx.rdregion etc
/// intrinsics) has its own separate address conversion (llvm.genx.convert.addr
/// intrinsic). Any constant add/sub between the address conversion
/// and the use of the variable index has been turned into an llvm.genx.add.addr
/// intrinsic.
///
/// This GenXAddressCommoning pass spots when multiple address conversions
/// use the same index value as input and are used in element/region accesses
/// with the same base register. These can then be commoned up.
///
/// In fact, rather than looking at an address conversion in isolation, it needs
/// to look at the whole bale containing the address conversion, which might have
/// a baled in rdregion and modifiers. It needs to do this because
/// GenXBaling cloned the rdregion and modifiers, so they need commoning up
/// again with the address conversion.
/// This situation is common because GenXLowering lowers a trunc (as often
/// found in an index calculation to convert the index to i16) into a bitcast
/// and a rdregion.
///
/// A second transformation in this pass is the "histogram optimization": If
/// there are multiple scalar address conversions for the same base reg where
/// each index is an extract (a scalar rdregion) from the same index vector, we
/// attempt to common them up into a vector address conversion, with an extract
/// from the result of the vector address conversion for each user of an
/// original scalar address conversion. The extract is baled in to the indirect
/// region, appearing as the "addr_offset" field (the index into the 8 wide
/// address register) in the generated vISA.
///
/// This histogram optimization uses the hasIndirectGRFCrossing feature from
/// GenXSubtarget to tell how big the combined vector address conversion can be,
/// in the case that it itself is an indirect region.
///
/// Both of the transformations in this pass are fiddly because the pass runs so
/// late. It has to run this late because we cannot tell whether address
/// conversions can be commoned up until GenXCoalescing has decided which vectors
/// are in the same register, but that then means that this pass has to update
/// live ranges and baling info for the code that it modifies.
///
/// **IR restriction**: After this pass, the restrictions on
/// ``llvm.genx.convert.addr`` and ``llvm.genx.add.addr`` having just a single
/// use are relaxed. Now, multiple uses of ``llvm.genx.convert.addr``, possibly
/// each via a single ``llvm.genx.add.addr``, must be in rdregions/wrregions
/// where the base register is provably the same because all the values that
/// appear as the "old value" input are coalesced together into the same
/// LiveRange.
///
//===----------------------------------------------------------------------===//
#include "FunctionGroup.h"
#include "GenX.h"
#include "GenXBaling.h"
#include "GenXGotoJoin.h"
#include "GenXLiveness.h"
#include "GenXModule.h"
#include "GenXNumbering.h"
#include "GenXUtil.h"

#include "vc/Utils/GenX/GlobalVariable.h"
#include "vc/Utils/GenX/RegCategory.h"

#include "llvm-c/Core.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/Debug.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/TypeSize.h"

#define DEBUG_TYPE "GENX_ADDRESSCOMMONING"

using namespace llvm;
using namespace genx;

static cl::opt<bool> ConvertAfterWholeRegion("convert-after-whole", cl::init(true), cl::Hidden,
  cl::desc("Convert addrs after whole region conversion attempt"));

namespace {

// Local function for testing one assertion statement. Tests if we do not have
// any duplicates and if they are all in the current function.
bool testDuplicates(const llvm::ArrayRef<llvm::Instruction *> &Addrs,
  const llvm::Function *const F) {

  IGC_ASSERT(F);

  auto compareParent = [F](const llvm::Instruction *const I) {
    IGC_ASSERT(I);
    IGC_ASSERT(I->getParent());
    IGC_ASSERT(I->getFunction());
    const bool Result = (F == I->getFunction());
    IGC_ASSERT(Result);
    return Result;
  };

  llvm::ArrayRef<llvm::Instruction *>::const_iterator Begin = Addrs.begin();
  llvm::ArrayRef<llvm::Instruction *>::const_iterator End = Addrs.end();

  const std::set<llvm::Instruction *> UniqueSet(Begin, End);
  const size_t UniqueCount = UniqueSet.size();
  const size_t CurrentCount = Addrs.size();

  const bool AllAreUnique = (CurrentCount == UniqueCount);
  IGC_ASSERT(AllAreUnique);

  const bool AllAreInCurrentFunction = std::all_of(Begin, End, compareParent);
  IGC_ASSERT(AllAreInCurrentFunction);

  const bool Result = (AllAreUnique && AllAreInCurrentFunction);
  return Result;
}

// Bucket : a bucket for collecting address conversions with the same base reg
// and the same address calculation value, discarding duplicates.
struct Bucket {
  SmallVector<Instruction *, 4> Addrs;
  SmallSet<Instruction *, 4> AddrSet;
  void add(Instruction *Addr) {
    if (AddrSet.insert(Addr).second)
      Addrs.push_back(Addr);
  }
};

// ExtractBucket : a bucket for collecting address conversions with the same
// base reg that all use an extract (scalar rdregion) from the same vector,
// discarding duplicates.
struct ExtractBucket {
  SmallVector<Instruction *, 4> Addrs;
  SmallSet<Instruction *, 4> AddrSet;
  void add(Instruction *Addr) {
    if (AddrSet.insert(Addr).second)
      Addrs.push_back(Addr);
  }
};

// Extract: address and offset for region conversion
struct Extract {
  Instruction *Addr; // the address conversion instruction
  int Offset; // the offset from the rdregion;
  Extract(Instruction *Addr, int Offset) : Addr(Addr), Offset(Offset) {}
  bool operator<(const Extract &Other) const { return Offset < Other.Offset; }
};

// GenX address conversion pass
class GenXAddressCommoning : public FGPassImplInterface,
                             public IDMixin<GenXAddressCommoning> {
  GenXBaling *Baling = nullptr;
  GenXLiveness *Liveness = nullptr;
  GenXNumbering *Numbering = nullptr;
  const GenXSubtarget *ST = nullptr;
  Function *F = nullptr;
  SmallSet<Value *, 8> AlreadyProcessed;
  // Types and data structures used for gathering convert_addr ops that
  // could be commoned up:
  // InnerVec is a vector of convert_addr ops that have the same base register
  // and bale hash. OuterVec is a vector of InnerVec. OuterMap provides a
  // way of finding the element of OuterVec for a particular base register
  // and bale hash. Using a vector and map together like this ensures that
  // we process everything in the same order even as pointer values and hashes
  // change from one compiler run to another.
  typedef SmallVector<Instruction *, 4> InnerVec_t;
  typedef SmallVector<InnerVec_t, 4> OuterVec_t;
  OuterVec_t OuterVec;
  struct BaseRegAndBaleHash {
    LiveRange *BaseReg;
    hash_code BaleHash;
    BaseRegAndBaleHash(LiveRange *BaseReg, hash_code BaleHash)
      : BaseReg(BaseReg), BaleHash(BaleHash) {}
    static bool less(BaseRegAndBaleHash BRH1, BaseRegAndBaleHash BRH2)
    {
      if (BRH1.BaseReg != BRH2.BaseReg)
        return BRH1.BaseReg < BRH2.BaseReg;
      return BRH1.BaleHash < BRH2.BaleHash;
    }
  };
  typedef std::map<BaseRegAndBaleHash, unsigned,
          bool (*)(BaseRegAndBaleHash, BaseRegAndBaleHash)> OuterMap_t;
  OuterMap_t OuterMap;
public:
  explicit GenXAddressCommoning()
      : OuterMap(OuterMap_t(BaseRegAndBaleHash::less)) {}
  static StringRef getPassName() { return "GenX address commoning"; }
  static void getAnalysisUsage(AnalysisUsage &AU) {
    AU.addRequired<DominatorTreeGroupWrapperPass>();
    AU.addRequired<GenXModule>();
    AU.addRequired<GenXGroupBaling>();
    AU.addRequired<GenXGroupLiveElementsWrapper>();
    AU.addRequired<GenXLiveness>();
    AU.addRequired<GenXNumbering>();
    AU.addPreserved<DominatorTreeGroupWrapperPass>();
    AU.addPreserved<GenXGroupBaling>();
    AU.addPreserved<GenXLiveness>();
    AU.addPreserved<GenXModule>();
    AU.addPreserved<GenXNumbering>();
    AU.addPreserved<FunctionGroupAnalysis>();
    AU.setPreservesCFG();
  }
  bool runOnFunctionGroup(FunctionGroup &FG) override;

private:
  bool processFunction(Function *F);
  bool processBaseReg(LiveRange *LR);
  bool processCommonAddrs(ArrayRef<Instruction *> Addrs);
  void processCommonAddrsWithValidOffsets(ArrayRef<Instruction *> Addrs);
  bool vectorizeAddrs(LiveRange *LR);
  void addAddrConvIfExtract(std::map<std::pair<Value *, int>, ExtractBucket> *ExtractBuckets, Value *Index);
  bool tryConvertWholeRegion(SmallVector<Extract, 4> &Extracts,
                             Instruction *VecDef);
  bool vectorizeAddrsFromOneVector(ArrayRef<Instruction *> Addrs);
  DominatorTree *getDominatorTree();
  bool isValueInCurrentFunc(Value *V);
  unsigned getNumberElementsInAddrReg() const {
    if (ST)
      return ST->getNumElementsInAddrReg();
    return 8;
  }
};

} // end anonymous namespace

namespace llvm {
void initializeGenXAddressCommoningWrapperPass(PassRegistry &);
using GenXAddressCommoningWrapper =
    FunctionGroupWrapperPass<GenXAddressCommoning>;
} // namespace llvm
INITIALIZE_PASS_BEGIN(GenXAddressCommoningWrapper,
                      "GenXAddressCommoningWrapper",
                      "GenXAddressCommoningWrapper", false, false)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeGroupWrapperPassWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXGroupBalingWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXGroupLiveElementsWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXLivenessWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXNumberingWrapper)
INITIALIZE_PASS_END(GenXAddressCommoningWrapper, "GenXAddressCommoningWrapper",
                    "GenXAddressCommoningWrapper", false, false)

ModulePass *llvm::createGenXAddressCommoningWrapperPass() {
  initializeGenXAddressCommoningWrapperPass(*PassRegistry::getPassRegistry());
  return new GenXAddressCommoningWrapper();
}

/***********************************************************************
 * runOnFunctionGroup : run the address commoning pass for this
 *    FunctionGroup
 */
bool GenXAddressCommoning::runOnFunctionGroup(FunctionGroup &FG)
{
  Baling = &getAnalysis<GenXGroupBaling>();
  Liveness = &getAnalysis<GenXLiveness>();
  Liveness->setLiveElements(&getAnalysis<GenXGroupLiveElements>());
  Numbering = &getAnalysis<GenXNumbering>();
  ST = getAnalysis<GenXModule>().getSubtarget();
  bool Modified = false;
  for (auto fgi = FG.begin(), fge = FG.end(); fgi != fge; ++fgi) {
    F = *fgi;
    Modified |= processFunction(F);
  }
  return Modified;
}

/***********************************************************************
 * processFunction : process one function in the address commoning pass
 */
bool GenXAddressCommoning::processFunction(Function *F)
{
  // Build a list of base registers used in an indirect rdregion or wrregion.
  // This does a preordered depth first traversal of the CFG to
  // ensure that we see a def before its uses (ignoring phi node uses).
  // Because an llvm.genx.convert.addr intrinsic can bale in a rdregion
  // with a variable index that itself uses an llvm.genx.convert.addr,
  // we want to process the code in forward order so that we can do as
  // much commoning as possible.
  SmallVector<LiveRange *, 8> BaseRegs;
  std::set<LiveRange *> BaseRegsSet;
  for (df_iterator<BasicBlock *> i = df_begin(&F->getEntryBlock()),
      e = df_end(&F->getEntryBlock()); i != e; ++i) {
    for (auto bi = i->begin(), be = i->end(); bi != be; ++bi) {
      Instruction *Inst = &*bi;
      LiveRange *LR = nullptr;
      switch (GenXIntrinsic::getGenXIntrinsicID(Inst)) {
        default:
          continue;
        case GenXIntrinsic::genx_rdregioni:
        case GenXIntrinsic::genx_rdregionf:
          if (isa<Constant>(Inst->getOperand(
                  GenXIntrinsic::GenXRegion::RdIndexOperandNum)))
            continue;
          LR = Liveness->getLiveRange(Inst->getOperand(0));
          break;
        case GenXIntrinsic::genx_wrregioni:
        case GenXIntrinsic::genx_wrregionf:
          if (isa<Constant>(Inst->getOperand(
                  GenXIntrinsic::GenXRegion::WrIndexOperandNum)))
            continue;
          // A write region may be baled into a g_store.
          LR = Liveness->getLiveRangeOrNull(Inst);
          if (!LR) {
            IGC_ASSERT(Inst->hasOneUse());
            auto SI = dyn_cast<StoreInst>(Inst->user_back());
            if (!SI)
              continue;
            Value *GV =
                vc::getUnderlyingGlobalVariable(SI->getPointerOperand());
            if (!GV)
              continue;
            LR = Liveness->getLiveRange(GV);
          }
          break;
      }
      // Inst is rdregion or wrregion with non-constant index.
      // Save the base register.
      if (BaseRegsSet.insert(LR).second)
        BaseRegs.push_back(LR); // not seen before
    }
  }
  BaseRegsSet.clear();
  // Process each base register.
  bool Modified = false;
  for (auto i = BaseRegs.begin(), e = BaseRegs.end(); i != e; ++i) {
    Modified |= processBaseReg(*i);
    Modified |= vectorizeAddrs(*i);
  }
  return Modified;
}

/***********************************************************************
 * processBaseReg : process one base register
 *
 * Enter:   LR = LiveRange with all the values for this base register
 *
 * We common up all address conversions with the same input that are used to
 * address a region of this base register.
 */
bool GenXAddressCommoning::processBaseReg(LiveRange *LR)
{
  // Gather the address conversions used by regions of this base register into
  // buckets, one for each distinct input. A bucket discards duplicate address
  // conversions.
  llvm::MapVector<Bale, Bucket, std::unordered_map<Bale, unsigned>> Buckets;

  auto processIndexOperand = [&](Value *Index) {
    while (GenXIntrinsic::getGenXIntrinsicID(Index) ==
           GenXIntrinsic::genx_add_addr)
      Index = cast<Instruction>(Index)->getOperand(0);
    if (GenXIntrinsic::getGenXIntrinsicID(Index) !=
        GenXIntrinsic::genx_convert_addr)
      return;
    Bale B;
    Baling->buildBale(cast<Instruction>(Index), &B);
    B.hash();
    Buckets[B].add(cast<Instruction>(Index));
  };

  for (auto vi = LR->value_begin(), ve = LR->value_end(); vi != ve; ++vi) {
    Value *V = vi->getValue();
    // Ignore the value if it is in the wrong function. That can happen because
    // liveness information is shared between functions in the same group.
    if (!isValueInCurrentFunc(V))
      continue;
    // First the def, if it is a wrregion.
    if (GenXIntrinsic::isWrRegion(V))
      processIndexOperand(cast<Instruction>(V)->getOperand(
          GenXIntrinsic::GenXRegion::WrIndexOperandNum));
    // Then each use that is a rdregion. (A use that is a wrregion will be
    // handled when we look at that value, which must be coalesced into the
    // same live range.)
    for (auto ui = V->use_begin(), ue = V->use_end(); ui != ue; ++ui) {
      if (ui->getOperandNo() != GenXIntrinsic::GenXRegion::OldValueOperandNum)
        continue;
      auto user = cast<Instruction>(ui->getUser());

      auto isBaledWrr = [this, V, user, LR]() {
        if (!isa<LoadInst>(V) || !GenXIntrinsic::isWrRegion(user) || !user->hasOneUse())
          return false;
        StoreInst *SI = dyn_cast<StoreInst>(user->user_back());
        GlobalVariable *GV =
            SI ? vc::getUnderlyingGlobalVariable(SI->getPointerOperand())
               : nullptr;
        if (!GV)
          return false;
        // make sure the base is the right global variable.
        return Liveness->getLiveRangeOrNull(GV) == LR;
      };

      // wrr may have been baled with a g_store.
      if (isBaledWrr())
        processIndexOperand(cast<Instruction>(user)->getOperand(
            GenXIntrinsic::GenXRegion::WrIndexOperandNum));

      if (!GenXIntrinsic::isRdRegion(user))
        continue;
      processIndexOperand(user->getOperand(
          GenXIntrinsic::GenXRegion::RdIndexOperandNum));
    }
  }
  // Common up each bucket with more than one address conversion.
  bool Modified = false;
  for (auto i = Buckets.begin(), e = Buckets.end(); i != e; ++i)
    Modified |= processCommonAddrs(i->second.Addrs);
  return Modified;
}

/***********************************************************************
 * processCommonAddrs : common up some address conversions
 *
 * Enter:   Addrs = one or more address conversion instructions that all have
 *          the same input and address the same base register, with no
 *          duplicates. Offsets (in add.addr intrinsics) are not known to
 *          be in range; this function fixes that.
 *
 * Return:  whether code modified
 *
 * This function relies on there being no duplicates in Addrs in the way that
 * it erases the address conversions other than the one it uses as the common
 * one.
 *
 * This processes a batch of address conversions with add.addr offsets close
 * enough to each other that we can use constant offsets in the indirect
 * operands. Then it recursively calls itself with what is left after removing
 * that batch.
 *
 * This code relies on there only being one add.addr between a convert.addr and
 * the use of the added address in a rdregion/wrregion. GenXCategory ensures
 * that this is the case.
 */
bool GenXAddressCommoning::processCommonAddrs(ArrayRef<Instruction *> Addrs)
{
  IGC_ASSERT(testDuplicates(Addrs, F));

  bool Modified = false;
  // Get the offsets. (Each address conversion has only one use; that is how
  // GenXCategory set it up.)
  SmallVector<int, 4> Offsets;
  for (unsigned i = 0, e = Addrs.size(); i != e; ++i) {
    int Offset = 0;
    IGC_ASSERT(Addrs[i]->hasOneUse());
    auto AddrUse = cast<Instruction>(Addrs[i]->use_begin()->getUser());
    if (GenXIntrinsic::getGenXIntrinsicID(AddrUse) ==
        GenXIntrinsic::genx_add_addr) {
      // The offset is operand 1 of the add_addr, and it is either a constant
      // int or a splat of a constant int.
      auto C = cast<Constant>(AddrUse->getOperand(1));
      if (isa<VectorType>(C->getType()))
        C = C->getSplatValue();
      Offset = cast<ConstantInt>(C)->getSExtValue();
    }
    Offsets.push_back(Offset);
  }
  // Get the min offset.
  int MinOffset = INT_MAX;
  for (unsigned i = 0, e = Offsets.size(); i != e; ++i)
    MinOffset = std::min(MinOffset, Offsets[i]);
  // Split the address conversions into ones used with an offset in
  // [MinOffset,MinOffset+1023] and ones that are outside that range.
  SmallVector<Instruction *, 4> InRangeAddrs;
  SmallVector<Instruction *, 4> OutOfRangeAddrs;
  int MaxOffset = INT_MIN;
  for (unsigned i = 0, e = Offsets.size(); i != e; ++i) {
    if (Offsets[i] - MinOffset < 1024) {
      InRangeAddrs.push_back(Addrs[i]);
      MaxOffset = std::max(MaxOffset, Offsets[i]);
    } else
      OutOfRangeAddrs.push_back(Addrs[i]);
  }
  // We handle the ones in range here.
  // The address conversions are going to be commoned up. Decide what offset we
  // are going to put on the commoned up one. We ensure that the offset is
  // inside the range of offsets that we found in the uses of the address
  // conversions, to try and avoid the situation where the address conversion
  // generates an out-of-range value in the address register that is then
  // brought back into range by the immediate offset in each use of the address
  int CommonOffset = 0;
  if (MinOffset < 0) {
    if (MaxOffset < 0) {
      // All offsets are negative. Use 0 if that is in range, else as close to
      // the max end of the offset range as we can get, rounded down to a
      // multiple of 32.
      if (MinOffset < G4_MIN_ADDR_IMM)
        CommonOffset = std::min(MinOffset + 512, MaxOffset) & -32;
    } else {
      // Some negative and some non-negative. Common offset can be 0.
      CommonOffset = 0;
    }
  } else {
    // All offsets are non-negative. Use 0 if that is in range, else as close
    // to the min end of the offsets range as we can get, rounded up to a
    // multiple of 32.
    if (MaxOffset >= 512)
      CommonOffset = (std::max(MaxOffset - 511, MinOffset) + 31) & -32;
  }
  if (CommonOffset) {
    Modified = true;
    // Modify the address conversions to use the common offset, and adjust the
    // address adds accordingly.
    auto CommonOffsetVal = ConstantInt::get(InRangeAddrs[0]->getType()
        ->getScalarType(), CommonOffset);
    for (unsigned i = 0, e = InRangeAddrs.size(); i != e; ++i) {
      Instruction *Addr = InRangeAddrs[i];
      Addr->setOperand(1, CommonOffsetVal);
      Use *U = &*Addr->use_begin();
      auto *AddAddr = cast<Instruction>(U->getUser());
      int AdjustedOffset = -CommonOffset;
      if (GenXIntrinsic::getGenXIntrinsicID(AddAddr) ==
          GenXIntrinsic::genx_add_addr) {
        auto ThisOffsetC = cast<Constant>(AddAddr->getOperand(1));
        if (isa<VectorType>(ThisOffsetC->getType()))
          ThisOffsetC = ThisOffsetC->getSplatValue();
        AdjustedOffset += cast<ConstantInt>(ThisOffsetC) ->getSExtValue();
      } else if (AdjustedOffset) {
        // We don't have an add_addr. We need to insert one.
        Constant *C = ConstantInt::get(CommonOffsetVal->getType(),
              AdjustedOffset);
        if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(Addr->getType()))
          C = ConstantVector::getSplat(
              IGCLLVM::getElementCount(VT->getNumElements()), C);
        auto CI = createAddAddr(Addr, C,
            Addr->getName() + ".addaddr", AddAddr);
        *U = CI;
        AddAddr = CI;
      } else
        AddAddr = nullptr;
      if (AddAddr) {
        // Adjust the offset on the add_addr. The offset is operand 1 of the
        // add_addr, and it is either a constant int or a splat of a constant
        // int.
        Constant *C = ConstantInt::get(CommonOffsetVal->getType(),
              AdjustedOffset);
        if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(
                AddAddr->getOperand(1)->getType()))
          C = ConstantVector::getSplat(
              IGCLLVM::getElementCount(VT->getNumElements()), C);
        AddAddr->setOperand(1, C);
        // Ensure the add_addr is baled in to the rdregion/wrregion that uses
        // it. (It was not if we have just created it, or if its offset was out
        // of range.) Also remove its live range.
        IGC_ASSERT(AddAddr->hasOneUse());
        auto User = cast<Instruction>(AddAddr->use_begin()->getUser());
        IGC_ASSERT(GenXIntrinsic::isRdRegion(User) || GenXIntrinsic::isWrRegion(User));
        auto BI = Baling->getBaleInfo(User);
        BI.setOperandBaled(AddAddr->use_begin()->getOperandNo());
        Baling->setBaleInfo(User, BI);
        Liveness->eraseLiveRange(AddAddr);
      }
    }
  }
  // Now we can actually common up the in range addresses, if more than one of
  // them.
  if (InRangeAddrs.size() > 1) {
    Modified = true;
    processCommonAddrsWithValidOffsets(InRangeAddrs);
  }
  // Call recursively to process the remaining (out of range) ones.
  if (!OutOfRangeAddrs.empty())
    Modified |= processCommonAddrs(OutOfRangeAddrs);
  return Modified;
}

/***********************************************************************
 * processCommonAddrsWithValidOffsets : common up some address conversions
 *
 * Enter:   Addrs = two or more address conversion instructions that all have
 *          the same input and address the same base register, with no
 *          duplicates, and all have valid in range offsets (add.addr intrinsics)
 *
 * This function relies on there being no duplicates in Addrs in the way that
 * it erases the address conversions other than the one it uses as the common
 * one.
 */
void GenXAddressCommoning::processCommonAddrsWithValidOffsets(
    ArrayRef<Instruction *> Addrs)
{
  // Find the address conversion that dominates all the others.
  Instruction *DominatingAddr = findClosestCommonDominator(
      getDominatorTree(), Addrs);
  if (DominatingAddr && DominatingAddr->isTerminator()) {
    // Ensure we have a legal insertion point in the presence of SIMD CF.
    auto InsertBefore = GotoJoin::getLegalInsertionPoint(DominatingAddr,
          getDominatorTree());
    // We did not find one address conversion that dominates all of them.  Move
    // an arbitrarily chosen one to the end of the dominating basic block.
    // This position dominates the other address conversions, and is dominated
    // by the index input value.
    // We need to move the entire bale, not just the address conversion
    // instruction itself. The whole bale is given an instruction number the
    // same as the terminator of the closest common dominator block that it is
    // being inserted before. Doing this is a bit dodgy because the result of
    // the address conversion does not appear to interfere with the operands
    // of a cmp baled into a conditional branch, but in practice this is not
    // a problem because the result of an address conversion is an address
    // register and the
    unsigned Num = Numbering->getNumber(InsertBefore);
    Bale B;
    Baling->buildBale(Addrs[0], &B);
    for (auto i = B.begin(), e = B.end(); i != e; ++i) {
      Instruction *Inst = i->Inst;
      DominatingAddr = Inst;
      Inst->removeFromParent();
      Inst->insertBefore(InsertBefore);
      Numbering->setNumber(Inst, Num);
    }
  }
  // Use the dominating one instead of all the others.
  for (auto i = Addrs.begin(), e = Addrs.end(); i != e; ++i) {
    Instruction *Addr = *i;
    if (Addr == DominatingAddr)
      continue;
    Addr->replaceAllUsesWith(DominatingAddr);
    do {
      auto Next = dyn_cast<Instruction>(Addr->getOperand(0));
      Liveness->removeValue(Addr);

      // It happens that after commoning there are unused dangling instructions
      // in some cases and vISA writer asserts.
      bool EraseAddr = true;
      if (GenXIntrinsic::getGenXIntrinsicID(Addr) ==
              GenXIntrinsic::genx_rdregioni ||
          GenXIntrinsic::getGenXIntrinsicID(Addr) ==
              GenXIntrinsic::genx_rdregionf) {
         Value *Idx = Addr->getOperand(GenXIntrinsic::GenXRegion::RdIndexOperandNum);
         auto II = dyn_cast<Instruction>(Idx);
         if (II && II->hasOneUse()) {
           Addr->eraseFromParent();
           EraseAddr = false;

           IGC_ASSERT(II->use_empty());
           Liveness->removeValue(II);
           II->eraseFromParent();
         }
      }
      if (EraseAddr)
        Addr->eraseFromParent();

      Addr = Next;
    } while (Addr && Addr->use_empty());
  }
  // Rebuild the live range for the common address calculation.
  // Note that we do not rebuild the live ranges for the input(s) to the
  // common address calculation bale; this is conservative.
  Liveness->rebuildLiveRange(Liveness->getLiveRange(DominatingAddr));
}

/***********************************************************************
 * vectorizeAddrs : attempt to vectorize address conversions for one base reg
 *
 * Enter:   LR = LiveRange with all the values for this base register
 *
 * If there are multiple scalar address conversions for this base reg where
 * the index is an extract from the same vector, we attempt to common them up
 * into a vector address conversion with extracts from the result. This is the
 * histogram optimization.
 */
bool GenXAddressCommoning::vectorizeAddrs(LiveRange *LR)
{
  // Gather the address conversions from an extract from a vector used by
  // regions of this base register into buckets, one for each distinct vector
  // being extracted from and each distinct address conversion offset.
  std::map<std::pair<Value *, int>, ExtractBucket> ExtractBuckets;
  for (auto vi = LR->value_begin(), ve = LR->value_end(); vi != ve; ++vi) {
    Value *V = vi->getValue();
    // Ignore the value if it is in the wrong function. That can happen because
    // liveness information is shared between functions in the same group.
    if (!isValueInCurrentFunc(V))
      continue;
    // First the def, if it is a wrregion.
    if (GenXIntrinsic::isWrRegion(V)) {
      Value *Index = cast<Instruction>(V)->getOperand(
          GenXIntrinsic::GenXRegion::WrIndexOperandNum);
      addAddrConvIfExtract(&ExtractBuckets, Index);
    }
    // Then each use that is a rdregion. (A use that is a wrregion will be
    // handled when we look at that value, which must be coalesced into the
    // same live range.)
    for (auto ui = V->use_begin(), ue = V->use_end(); ui != ue; ++ui) {
      if (ui->getOperandNo() != GenXIntrinsic::GenXRegion::OldValueOperandNum)
        continue;
      auto user = cast<Instruction>(ui->getUser());
      if (!GenXIntrinsic::isRdRegion(user))
        continue;
      Value *Index = user->getOperand(GenXIntrinsic::GenXRegion::RdIndexOperandNum);
      addAddrConvIfExtract(&ExtractBuckets, Index);
    }
  }
  // Process each bucket of address calculations that extract from the
  // same vector.
  bool Modified = false;
  for (auto i = ExtractBuckets.begin(), e = ExtractBuckets.end(); i != e; ++i)
    if (i->second.Addrs.size() >= 2)
      Modified |= vectorizeAddrsFromOneVector(i->second.Addrs);
  return Modified;
}

/***********************************************************************
 * addAddrConvIfExtract : add an address conversion to the appropriate
 *        bucket if the address is an extract from a vector
 *
 * Enter:   ExtractBuckets = map of buckets
 *          Index = index operand from rdregion/wrregion
 *
 * Possibly after traversing some add_addr ops, Index is a constant or a
 * convert_addr.  If it is a convert_addr whose input is an extract (a scalar
 * rdregion) with a single use, add the convert_addr to the bucket for the
 * vector that the extract is extracted from.
 */
void GenXAddressCommoning::addAddrConvIfExtract(
    std::map<std::pair<Value *, int>, ExtractBucket> *ExtractBuckets, Value *Index)
{
  while (GenXIntrinsic::getGenXIntrinsicID(Index) ==
         GenXIntrinsic::genx_add_addr)
    Index = cast<Instruction>(Index)->getOperand(0);
  if (isa<Constant>(Index))
    return;
  IGC_ASSERT(GenXIntrinsic::getGenXIntrinsicID(Index) ==
         GenXIntrinsic::genx_convert_addr);
  auto RdR = dyn_cast<Instruction>(cast<Instruction>(Index)->getOperand(0));
  if (!GenXIntrinsic::isRdRegion(RdR))
    return;
  IGC_ASSERT(RdR);
  if (!isa<Constant>(RdR->getOperand(GenXIntrinsic::GenXRegion::RdIndexOperandNum)))
    return;
  if (!RdR->hasOneUse())
    return;
  auto AddrConv = cast<Instruction>(Index);
  int AddrConvOffset = cast<ConstantInt>(AddrConv->getOperand(1))->getSExtValue();
  (*ExtractBuckets)[std::pair<Value *, int>(RdR->getOperand(0), AddrConvOffset)]
        .add(AddrConv);
}

/***********************************************************************
 * tryConvertWholeRegion : attempt to convert whole region
 *
 * Enter:   Extracts -- array of address conversions, extracted from
 *                      inputs to vectorizeAddrsFromOneVector,
 *                      combined with corresponding region offset
 *          VecDef -- instruction definition from first extract
 *
 * This is subroutine of vectorizeAddrsFromOneVector, see more comments
 * in parent function. Idea of this subroutine is to convert whole
 * region if possible
 */
bool GenXAddressCommoning::tryConvertWholeRegion(SmallVector<Extract, 4> &Extracts, Instruction *VecDef) {
  Instruction *InsertBefore = Extracts[0].Addr;
  unsigned int MinNum, MaxNum;

  // maximal difference between MinNum and MaxNum to accept region
  // TODO: to be tuned?
  const int SIZE_THRESHOLD = 48;

  MinNum = MaxNum = Numbering->getNumber(InsertBefore);
  // check every extract
  for (unsigned Idx = 0, End = Extracts.size(); Idx < End; ++Idx) {
    Instruction *RdR = cast<Instruction>(Extracts[Idx].Addr->getOperand(0));
    vc::Region R = makeRegionFromBaleInfo(RdR, BaleInfo());
    if (R.NumElements > 1 && R.Stride > 1)
      return false;
    // all address-conv must be in the same basic block
    if (Extracts[Idx].Addr != InsertBefore &&
      Extracts[Idx].Addr->getParent() != InsertBefore->getParent()) {
      LLVM_DEBUG(errs() << "tryConvertWholeRegion: not all in the same block\n");
      return false;
    }
    // test to update the insertion-point
    unsigned int ThisNum = Numbering->getNumber(Extracts[Idx].Addr);
    if (ThisNum < MinNum) {
      InsertBefore = Extracts[Idx].Addr;
      MinNum = ThisNum;
    }
    if (ThisNum > MaxNum)
      MaxNum = ThisNum;
  }
  if ((MaxNum - MinNum) > SIZE_THRESHOLD)
    return false;
  // Create a vectorized address conversion and bale the new rdregion (if
  // any) into it. Give the new vectorized address conversion, and the new
  // rdregion (if any), the number of one less than the insert point.
  int AddrConvOffset =
    cast<ConstantInt>(Extracts[0].Addr->getOperand(1))->getSExtValue();
  auto NewConv = createConvertAddr(VecDef, AddrConvOffset,
    Extracts[0].Addr->getName() + ".monted", InsertBefore);
  NewConv->setDebugLoc(VecDef->getDebugLoc());
  Numbering->setNumber(NewConv, Numbering->getNumber(VecDef) + 1);
  // For each original address conversion, replace it with an
  // extract from the vectorized convert, and bale the extract into
  // its use. If it has more than one use, create an extract per use
  // (because a baled in instruction must be single use).
  for (unsigned Idx2 = 0, End2 = Extracts.size(); Idx2 < End2; ++Idx2) {
    auto OldConv = Extracts[Idx2].Addr;
    Instruction *OldExtract = cast<Instruction>(OldConv->getOperand(0));
    vc::Region R2 = makeRegionFromBaleInfo(OldExtract, BaleInfo());
    while (!OldConv->use_empty()) {
      auto ui = OldConv->use_begin();
      auto user = cast<Instruction>(ui->getUser());
      auto NewExtract = R2.createRdRegion(NewConv, OldConv->getName(), user,
        user->getDebugLoc(), /*ScalarAllowed=*/!OldConv->getType()->isVectorTy());
      Numbering->setNumber(NewExtract, Numbering->getNumber(user));
      // At this late stage, I believe nothing relies on the baling type for
      // this instruction being set to RDREGION, but we set it anyway for
      // completeness.
      Baling->setBaleInfo(NewExtract, BaleInfo(BaleInfo::RDREGION));
      BaleInfo BI = Baling->getBaleInfo(user);
      BI.setOperandBaled(ui->getOperandNo());
      Baling->setBaleInfo(user, BI);
      *ui = NewExtract;
    }
    Liveness->removeValue(OldConv);
    IGC_ASSERT_MESSAGE(!Liveness->getLiveRangeOrNull(OldExtract),
      "expected extract to be baled in");
    OldConv->eraseFromParent();
    OldExtract->eraseFromParent();
  }
  // Give the new vectorized address conversion a live range.
  auto LR = Liveness->getOrCreateLiveRange(NewConv);
  LR->setCategory(vc::RegCategory::Address);
  Liveness->rebuildLiveRange(LR);
  return true;
}

/***********************************************************************
 * vectorizeAddrsFromOneVector : attempt to vectorize address conversions
 *
 * Enter:   Addrs = address conversions for the same base reg with the same
 *                  offset that are all scalar rdregion (constant offset) from
 *                  the same vector, at least two of them
 *
 * If there are multiple scalar address conversions for this base reg where the
 * index is an extract from the same vector, we attempt to common them up into
 * a vector address conversion with extracts from the result. This is the
 * histogram optimization.
 */
bool GenXAddressCommoning::vectorizeAddrsFromOneVector(
    ArrayRef<Instruction *> Addrs)
{
  bool Modified = false;
  SmallVector<Extract, 4> Extracts;
  bool HasVector = false;
  std::set<int> OffsetSet;
  LLVM_DEBUG(dbgs() << "Collecting addrs: " << Addrs.size() << "\n");

  for (auto i = Addrs.begin(), e = Addrs.end(); i != e; ++i) {
    Instruction *Addr = *i;
    LLVM_DEBUG(Addr->dump());

    vc::Region R = makeRegionFromBaleInfo(
        cast<Instruction>(Addr->getOperand(0)), BaleInfo());
    LLVM_DEBUG(dbgs() << " [" << R.Offset << "]\n");

    Extracts.push_back(Extract(Addr, R.Offset));
    OffsetSet.insert(R.Offset);
    if (isa<VectorType>(Addr->getType()))
      HasVector = true;
  }
  bool ConvertWholeRegion = false;
  Instruction *FirstRdR = cast<Instruction>(Extracts[0].Addr->getOperand(0));
  IGC_ASSERT(FirstRdR);
  Instruction *VecDef = cast<Instruction>(FirstRdR->getOperand(0));
  IGC_ASSERT(VecDef);

  unsigned InputNumElements =
      cast<IGCLLVM::FixedVectorType>(VecDef->getType())->getNumElements();

  if (HasVector) {
    if (InputNumElements == 2 || InputNumElements == 4 ||
        InputNumElements == 8 || InputNumElements == 16)
      ConvertWholeRegion = true;
    else
      return Modified;
  }
  else if (OffsetSet.size()*3 >= InputNumElements*2 &&
    (InputNumElements == 2 || InputNumElements == 4 ||
      InputNumElements == 8 || InputNumElements == 16))
    ConvertWholeRegion = true;

  // Sort into offset order.
  std::sort(Extracts.begin(), Extracts.end());

  if (ConvertWholeRegion) {
    bool Success = tryConvertWholeRegion(Extracts, VecDef);
    if (Success) {
      LLVM_DEBUG(dbgs() << "Succesfully converted whole region\n");
      return true;
    }

    LLVM_DEBUG(dbgs() << "Failed to convert whole region\n");
    if (!ConvertAfterWholeRegion)
      return false;
  }

  // if we tried to convert whole region and failed
  // we shall check that we will try to optimize further
  // correct extract set
  IGC_ASSERT(Extracts.size() > 0);
  Type *FirstType = Extracts[0].Addr->getOperand(0)->getType();
  IGC_ASSERT(FirstType);

  for (auto &e : Extracts) {
    Type *Tp = e.Addr->getOperand(0)->getType();
    if (ConvertWholeRegion && (Tp != FirstType))
      return false;
  }

  // Scan through the address conversions...
  for (unsigned Idx = 0, Num = 1, End = Extracts.size();
      Idx < End - 2; Idx += Num) {
    // See how many extracts we can take in one go that have evenly spaced
    // offsets, max is the number of elements in address register.
    int Diff = Extracts[Idx + 1].Offset - Extracts[Idx].Offset;
    for (Num = 2; Num != getNumberElementsInAddrReg() && Num != End - Idx; ++Num)
      if (Extracts[Idx + Num].Offset - Extracts[Idx + Num - 1].Offset != Diff)
        break;
    if (Num == 1)
      continue;
    // We have a sequence of more than one extract. Construct the region
    // parameters for it.
    Instruction *FirstRdR = cast<Instruction>(Extracts[Idx].Addr->getOperand(0));
    LLVM_DEBUG(dbgs() << "Sequence of " << Num << " instructions found. First one is:\n");
    LLVM_DEBUG(FirstRdR->dump());
    LLVM_DEBUG(dbgs() << "\n");
    vc::Region R = makeRegionFromBaleInfo(FirstRdR, BaleInfo());
    R.NumElements = R.Width = Num;
    R.Stride = Diff / R.ElementBytes;
    // See how big we can legally make the region.
    unsigned InputNumElements =
        cast<IGCLLVM::FixedVectorType>(FirstRdR->getOperand(0)->getType())
            ->getNumElements();
    IGC_ASSERT(ST);
    Num = getLegalRegionSizeForTarget(*ST, R, 0, true /*Allow2D*/,
                                      true /*UseRealIdx*/, InputNumElements);
    if (Num == 1)
      continue;
    // Even after legalizing the region, we can still vectorize to more than
    // one element.
    R.getSubregion(0, Num);
    // Determine where to insert the new rdregion (if any) and vectorized
    // address conversion.
    SmallVector<Instruction *, 4> Addrs;
    for (unsigned i = 0; i != Num; ++i)
      Addrs.push_back(Extracts[Idx + i].Addr);
    auto InsertBefore = findClosestCommonDominator(getDominatorTree(), Addrs);
    // Ensure we have a legal insertion point in the presence of SIMD CF.
    InsertBefore = GotoJoin::getLegalInsertionPoint(InsertBefore,
          getDominatorTree());
    // Read the region containing all the scalar indices we are commoning
    // up. (If R is the identity region, just use the whole original vector
    // instead.)
    Value *NewRdR = cast<Instruction>(Extracts[Idx].Addr->getOperand(0))
        ->getOperand(0);
    Instruction *NewRdRInst = nullptr;
    if (InputNumElements != R.NumElements) {
      // Not identity region.
      NewRdR = NewRdRInst = R.createRdRegion(NewRdR,
          Extracts[Idx].Addr->getName(), InsertBefore,
          Extracts[Idx].Addr->getDebugLoc(), false);
      Baling->setBaleInfo(NewRdRInst, BaleInfo(BaleInfo::RDREGION));
    }
    // Create a vectorized address conversion and bale the new rdregion (if
    // any) into it. Give the new vectorized address conversion, and the new
    // rdregion (if any), the number of one less than the insert point.
    int AddrConvOffset = cast<ConstantInt>(Addrs[0]->getOperand(1))->getSExtValue();
    auto NewConv = createConvertAddr(NewRdR, AddrConvOffset,
        Extracts[Idx].Addr->getName() + ".histogrammed", InsertBefore);
    NewConv->setDebugLoc(Extracts[Idx].Addr->getDebugLoc());
    Numbering->setNumber(NewConv, Numbering->getNumber(InsertBefore) - 1);
    if (NewRdRInst) {
      Numbering->setNumber(NewRdRInst, Numbering->getNumber(InsertBefore) - 1);
      BaleInfo BI(BaleInfo::MAININST);
      BI.setOperandBaled(0);
      Baling->setBaleInfo(NewConv, BI);
    }
    // For each original scalar address conversion, replace it with an
    // extract from the vectorized convert, and bale the extract in to
    // its use. If it has more than one use, create an extract per use
    // (because a baled in instruction must be single use).
    for (unsigned Idx2 = 0; Idx2 != Num; ++Idx2) {
      auto OldConv = Extracts[Idx + Idx2].Addr;
      Region R2(NewConv);
      R2.getSubregion(Idx2, 1);
      while (!OldConv->use_empty()) {
        auto ui = OldConv->use_begin();
        auto user = cast<Instruction>(ui->getUser());
        auto NewExtract = R2.createRdRegion(NewConv, OldConv->getName(), user,
                                            user->getDebugLoc(),
#if LLVM_VERSION_MAJOR < 16
                                            /*ScalarAllowed=*/true);
#else
                                            /*ScalarAllowed=*/
                                            !OldConv->getType()->isVectorTy());
#endif
        Numbering->setNumber(NewExtract, Numbering->getNumber(user));
        // At this late stage, I believe nothing relies on the baling type for
        // this instruction being set to RDREGION, but we set it anyway for
        // completeness.
        Baling->setBaleInfo(NewExtract, BaleInfo(BaleInfo::RDREGION));
        BaleInfo BI = Baling->getBaleInfo(user);
        BI.setOperandBaled(ui->getOperandNo());
        Baling->setBaleInfo(user, BI);
        *ui = NewExtract;
      }
      Liveness->removeValue(OldConv);
      auto OldExtract = cast<Instruction>(OldConv->getOperand(0));
      IGC_ASSERT_MESSAGE(!Liveness->getLiveRangeOrNull(OldExtract),
        "expected extract to be baled in");
      OldConv->eraseFromParent();
      OldExtract->eraseFromParent();
    }
    // Give the new vectorized address conversion a live range.
    auto LR = Liveness->getOrCreateLiveRange(NewConv);
    LR->setCategory(vc::RegCategory::Address);
    Liveness->rebuildLiveRange(LR);
    Modified = true;
  }
  return Modified;
}

/***********************************************************************
 * getDominatorTree : get dominator tree for current function
 */
DominatorTree *GenXAddressCommoning::getDominatorTree()
{
  return getAnalysis<DominatorTreeGroupWrapperPass>().getDomTree(F);
}

/***********************************************************************
 * isValueInCurrentFunc : determine whether V is in the current function
 *
 * Enter:   V = value from a LiveRange (therefore it is an Instruction
 *              or an Argument)
 *
 * Return:  true if it is in the current function
 */
bool GenXAddressCommoning::isValueInCurrentFunc(Value *V)
{
  if (auto Inst = dyn_cast<Instruction>(V)) {
    auto BB = Inst->getParent();
    if (!BB)
      return false;; // unified return value
    return BB->getParent() == F;
  }
  if (isa<GlobalVariable>(V))
    return false;
  return cast<Argument>(V)->getParent() == F;
}
