/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXExtractVectorizer
/// ---------------------
///
/// GenX extract vectorizer pass is stage 1 of the histogram optimization: if
/// there are multiple scalar rdregions from the same vector, all subject
/// to the same binary operator with constant rhs or the same trunc/zext/sext,
/// then they are combined into a vector version of the binary operator or
/// trunc/zext/sext, with scalar rdregions from the result of that. This is
/// designed to handle any trunc/zext/sext then scale of the index in the
/// histogram optimization, although it does also apply in a few other cases.
///
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "GENX_ExtractVectorizer"

#include "GenX.h"
#include "GenXUtil.h"

#include <llvm/Analysis/CFG.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/InitializePasses.h>
#include <llvm/Support/Debug.h>

#include "llvmWrapper/IR/DerivedTypes.h"

using namespace llvm;
using namespace genx;

namespace {

// GenX extract vectorizer pass
class GenXExtractVectorizer : public FunctionPass {
  bool Modified = false;
  DominatorTree *DT = nullptr;
  PostDominatorTree *PDT = nullptr;
  SmallVector<Value *, 8> Extracted;
  std::set<Value *> ExtractedSet;

public:
  struct Extract {
    Instruction *Inst; // the binary operator applied to the extracted element
    int Offset; // constant offset from the rdregion
    Extract(Instruction *Inst, int Offset) : Inst(Inst), Offset(Offset) {}
    // Sort in offset order
    bool operator<(const Extract &Other) const { return Offset < Other.Offset; }
  };
  struct BucketIndex {
    unsigned Opcode;
    Type *CastTo;
    Value *Indirect;
    Type *ConvTy;
    BucketIndex(unsigned Opcode, Type *CastTo, Value *Indirect, Type *ConvTy)
        : Opcode(Opcode), CastTo(CastTo), Indirect(Indirect), ConvTy(ConvTy) {}
    bool operator<(const BucketIndex &Other) const {
      if (Opcode != Other.Opcode)
        return Opcode < Other.Opcode;
      if (CastTo != Other.CastTo)
        return CastTo < Other.CastTo;
      return Indirect < Other.Indirect;
    }
  };
  using Bucket = SmallVector<Extract, 4>;
  struct BucketInfo {
    Bucket B;
    Region R;
    Instruction *InsertPt;
  };
  static char ID;
  explicit GenXExtractVectorizer() : FunctionPass(ID) { }
  StringRef getPassName() const override { return "GenX Extract Vectorizer"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<PostDominatorTreeWrapperPass>();
    AU.setPreservesCFG();
  }
  bool runOnFunction(Function &F) override;

private:
  void processExtracted(Value *V);
  void processBucket(BucketInfo &BI);
};

}// end namespace llvm


char GenXExtractVectorizer::ID = 0;
namespace llvm { void initializeGenXExtractVectorizerPass(PassRegistry &); }
INITIALIZE_PASS_BEGIN(GenXExtractVectorizer, "GenXExtractVectorizer",
                      "GenXExtractVectorizer", false, false)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
INITIALIZE_PASS_END(GenXExtractVectorizer, "GenXExtractVectorizer",
                    "GenXExtractVectorizer", false, false)

// Publicly exposed interface to pass...
FunctionPass *llvm::createGenXExtractVectorizerPass()
{
  initializeGenXExtractVectorizerPass(*PassRegistry::getPassRegistry());
  return new GenXExtractVectorizer();
}

/***********************************************************************
 * runOnFunction : run the extract vectorizer for this Function
 */
bool GenXExtractVectorizer::runOnFunction(Function &F)
{
  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
  // Scan the code looking for vector values that have an extract (a rdregion
  // of one element) applied.
  for (auto fi = F.begin(), fe = F.end(); fi != fe; ++fi) {
    BasicBlock *BB = &*fi;
    for (auto bi = BB->begin(), be = BB->end(); bi != be; ++bi) {
      Instruction *Inst = &*bi;
      if (!GenXIntrinsic::isRdRegion(Inst))
        continue;
      if (isa<VectorType>(Inst->getType()))
        continue;
      Value *V = Inst->getOperand(0);
      if (isa<Constant>(V))
        continue;
      if (ExtractedSet.insert(V).second)
        Extracted.push_back(V);
    }
  }
  ExtractedSet.clear();
  // Process each such vector. Processing a vector might result in another
  // new vector being pushed onto Extracted, so that in turn will be processed.
  while (!Extracted.empty()) {
    Value *V = Extracted.back();
    Extracted.pop_back();
    processExtracted(V);
  }
  return Modified;
}

// Check whether stride is uniform for the collected offsets.
static bool hasCommonStride(const GenXExtractVectorizer::Bucket &B) {
  auto Begin = B.begin();
  auto End = B.end();
  // If there is zero or one element, return true.
  if (std::distance(Begin, End) < 2)
    return true;
  // See if we have a sequence of offsets such that we can construct a
  // 1D region.
  int CurOffset = Begin->Offset;
  auto Next = std::next(Begin);
  int Stride = Next->Offset - CurOffset;
  while (++Begin != End) {
    if (Begin->Offset - CurOffset != Stride)
      return false;
    CurOffset = Begin->Offset;
  }
  return true;
}

static Instruction *findInsertionPoint(const GenXExtractVectorizer::Bucket &B,
                                       const DominatorTree &DT,
                                       const PostDominatorTree &PDT) {
  SmallVector<Instruction *, 8> Insts;
  std::transform(B.begin(), B.end(), std::back_inserter(Insts),
                 [](auto &Extract) { return Extract.Inst; });
  Instruction *InsertPt = findClosestCommonDominator(&DT, Insts);
  const BasicBlock *CommonDom = InsertPt->getParent();

  std::unordered_set<const BasicBlock *> BBs;
  std::transform(Insts.begin(), Insts.end(), std::inserter(BBs, BBs.end()),
                 [](auto *Inst) { return Inst->getParent(); });
  const BasicBlock *CommonPostDom = B.front().Inst->getParent();
  for (const auto *BB : BBs)
    CommonPostDom = PDT.findNearestCommonDominator(CommonPostDom, BB);

  if (CommonDom == CommonPostDom)
    return InsertPt;

  for (auto I = df_begin(CommonDom), E = df_end(CommonDom); I != E;) {
    // CommonPostDom is reached. There is a path from CommonDom to
    // CommonPostDom such that no instruction from Insts will be met. It means
    // that vectorization of the accesses for the bucket will generate redundant
    // computations for this execution path. For such cases benefit of the
    // vectorization is under the question. We follow conservative behavior and
    // do not transform not clear cases.
    // Common insertion points may exist for some bucket partitions. However,
    // experiments have shown that such a pattern is extremely rare and we may
    // lose more at compile time looking for suitable partitions.
    if (*I == CommonPostDom)
      return nullptr;
    // At least one instruction from Insts will be met. There is no need to
    // traverse children.
    if (BBs.count(*I))
      I.skipChildren();
    else
      ++I;
  }

  return InsertPt;
}

// Create region for vectorized accesses.
static Region createRegion(const GenXExtractVectorizer::Bucket &B,
                           const GenXExtractVectorizer::BucketIndex &BI) {
  IGC_ASSERT_MESSAGE(
      B.size() >= 2,
      "Bucket should contain at least two accesses to vectorize");
  IGC_ASSERT_MESSAGE(hasCommonStride(B),
                     "We should not be here if stride does not exist");
  int Stride = B[1].Offset - B[0].Offset;
  // Create the new rdregion.
  auto &Extract0 = B.front();
  Region R(Extract0.Inst->getOperand(0));
  R.NumElements = B.size();
  R.Width = B.size();
  R.Stride = Stride / R.ElementBytes;
  R.Indirect = BI.Indirect;
  R.Offset = Extract0.Offset;
  return R;
}

static bool isProfitable(const GenXExtractVectorizer::Bucket &B) {
  return B.size() >= 4;
}

/***********************************************************************
 * GenXExtractVectorizer::processExtracted : process an instruction or arg that
 * has at least one scalar extracted from it (using rdregion), in the hope that
 * we can vectorize it as the first stage of the histogram optimization
 */
void GenXExtractVectorizer::processExtracted(Value *V)
{
  // Gather the scalar extracting rdregion uses of V into buckets, one for
  // each binaryoperator with constant rhs that the extracted value is used in.
  std::map<BucketIndex, Bucket> Buckets;
  for (auto ui = V->use_begin(), ue = V->use_end(); ui != ue; ++ui) {
    auto user = cast<Instruction>(ui->getUser());
    if (!GenXIntrinsic::isRdRegion(user))
      continue; // not rdregion
    if (isa<VectorType>(user->getType()))
      continue; // not rdregion with scalar result
    if (!user->hasOneUse())
      continue; // rdregion not single use
    auto Use2 = &*user->use_begin();
    auto User2 = cast<Instruction>(Use2->getUser());
    // We want User2 to be either a binary operator with constant rhs,
    // or a trunc/zext/sext.
    Type *CastTo = nullptr;
    if (isa<BinaryOperator>(User2)) {
      if (!isa<Constant>(User2->getOperand(1)))
        continue; // binary operator has non-constant rhs
    } else {
      if (!isa<CastInst>(User2) || isa<BitCastInst>(User2))
        continue; // not trunc/zext/sext
      CastTo = User2->getType();
    }
    // Get the index, possibly as index+offset if the index is a balable add
    // instruction.
    Region R = makeRegionWithOffset(user);
    // Add to the bucket. The bucket is indexed by:
    //  - the opcode of the binaryoperator or trunc/zext/sext using the
    //    extracted value
    //  - the type being trunc/zext/sext to
    //  - any variable part of the rdregion index
    // The Extract pushed into the bucket contains:
    //  - the binaryoperator itself (from which we can find the rdregion)
    //  - the constant offset part of the rdregion index.
    Buckets[BucketIndex(User2->getOpcode(), CastTo, R.Indirect, User2->getType())]
        .push_back(Extract(User2, R.Offset));
  }

  std::vector<BucketInfo> BucketsToProcess;
  for (auto &&[BI, B] : Buckets) {
    // Now look at each bucket. Only bother with a bucket that has at least four
    // scalar extracts in it.
    if (!isProfitable(B))
      continue;

    // Sort the extracts into offset order.
    std::sort(B.begin(), B.end());
    // See if we have a sequence of offsets such that we can construct a
    // 1D region.
    if (!hasCommonStride(B))
      continue;
    // Find the latest point that we can insert the vectorized instruction.
    Instruction *InsertPt = findInsertionPoint(B, *DT, *PDT);
    if (!InsertPt)
      continue;

    Region R = createRegion(B, BI);
    BucketsToProcess.push_back({std::move(B), std::move(R), InsertPt});
  }

  for (auto &BI : BucketsToProcess)
    processBucket(BI);
}

/***********************************************************************
 * GenXExtractVectorizer::processBucket : process one bucket of extracts from
 * the same vector
 *
 * The bucket contains at least 4 instances of a binary operator whose rhs
 * is constant and whose lhs is an extract (a scalar rdregion) from the same
 * vector. Either each index is constant, or each index is an add with constant
 * rhs and with the same lhs.
 */
void GenXExtractVectorizer::processBucket(BucketInfo &BI) {
  auto &B = BI.B;
  Instruction *FirstExtractUser = B.front().Inst;
  Value *OrigVector =
      cast<Instruction>(FirstExtractUser->getOperand(0))->getOperand(0);
  Value *NewRdRegion = OrigVector;
  // Need to splat if Stride is 0, otherwise elements extracted are wrong.
  auto &R = BI.R;
  if (R.Stride == 0 || R.Indirect || R.Offset ||
      R.NumElements != cast<IGCLLVM::FixedVectorType>(OrigVector->getType())
                           ->getNumElements()) {
    // Not identity region.
    NewRdRegion = R.createRdRegion(
        OrigVector, FirstExtractUser->getName() + ".histogrammed", BI.InsertPt,
        FirstExtractUser->getDebugLoc(), /*AllowScalar=*/false);
  }
  // Create the vectorized binary operator or trunc/zext/sext.
  Instruction *NewInst = nullptr;
  if (isa<BinaryOperator>(FirstExtractUser)) {
    // Create a vector of the constants used in the right side of the binary
    // operators.
    SmallVector<Constant *, 8> RhsConsts;
    std::transform(
        B.begin(), B.end(), std::back_inserter(RhsConsts),
        [](Extract &E) { return cast<Constant>(E.Inst->getOperand(1)); });
    auto CV = ConstantVector::get(RhsConsts);
    NewInst = BinaryOperator::Create(
        (Instruction::BinaryOps)FirstExtractUser->getOpcode(), NewRdRegion, CV,
        FirstExtractUser->getName() + ".histogrammed", BI.InsertPt);
  } else {
    // Create the vectorized trunc/zext/sext.
    auto VT =
        IGCLLVM::FixedVectorType::get(FirstExtractUser->getType(), B.size());
    NewInst = CastInst::Create(
        (Instruction::CastOps)FirstExtractUser->getOpcode(), NewRdRegion, VT,
        FirstExtractUser->getName() + ".histogrammed", BI.InsertPt);
  }
  NewInst->setDebugLoc(FirstExtractUser->getDebugLoc());
  // For each original scalar binary operator or cast, create a rdregion to
  // extract the equivalent scalar from the result of the vectorized binary
  // operator, and use it to replace uses of the original binary operator.
  for (auto &IndexedExtract : llvm::enumerate(B)) {
    Region R2(NewInst);
    R2.NumElements = R2.Width = 1;
    R2.Offset = IndexedExtract.index() * R2.ElementBytes;
    auto *ExtractUser = IndexedExtract.value().Inst;
    auto NewRdRegion2 =
        R2.createRdRegion(NewInst, "", BI.InsertPt, ExtractUser->getDebugLoc(),
                          /*AllowScalar=*/true);
    NewRdRegion2->takeName(ExtractUser);
    ExtractUser->replaceAllUsesWith(NewRdRegion2);
  }
  for (auto &SingleExtract : B) {
    auto OldRdRegion = cast<Instruction>(SingleExtract.Inst->getOperand(0));
    SingleExtract.Inst->eraseFromParent();
    OldRdRegion->eraseFromParent();
  }
  // Add the new vectorized binary operator or cast back into
  // ExtractVectorizer so the extracts we added could in turn be vectorized.
  Extracted.push_back(NewInst);
  Modified = true;
}


