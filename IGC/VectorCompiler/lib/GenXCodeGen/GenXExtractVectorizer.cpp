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

#include "llvm/Analysis/CFG.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/Debug.h"

#include "llvmWrapper/IR/DerivedTypes.h"

using namespace llvm;
using namespace genx;

namespace {

// GenX extract vectorizer pass
class GenXExtractVectorizer : public FunctionPass {
  bool Modified = false;
  DominatorTree *DT = nullptr;
  SmallVector<Value *, 8> Extracted;
  std::set<Value *> ExtractedSet;
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
public:
  static char ID;
  explicit GenXExtractVectorizer() : FunctionPass(ID) { }
  StringRef getPassName() const override { return "GenX Extract Vectorizer"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.setPreservesCFG();
  }
  bool runOnFunction(Function &F) override;

private:
  void processExtracted(Value *V);
  void processBucket(const BucketIndex *BIdx, SmallVectorImpl<Extract> *B);
};

}// end namespace llvm


char GenXExtractVectorizer::ID = 0;
namespace llvm { void initializeGenXExtractVectorizerPass(PassRegistry &); }
INITIALIZE_PASS_BEGIN(GenXExtractVectorizer, "GenXExtractVectorizer",
                      "GenXExtractVectorizer", false, false)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
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

/***********************************************************************
 * GenXExtractVectorizer::processExtracted : process an instruction or arg that
 * has at least one scalar extracted from it (using rdregion), in the hope that
 * we can vectorize it as the first stage of the histogram optimization
 */
void GenXExtractVectorizer::processExtracted(Value *V)
{
  // Gather the scalar extracting rdregion uses of V into buckets, one for
  // each binaryoperator with constant rhs that the extracted value is used in.
  std::map<BucketIndex, SmallVector<Extract, 4>> Buckets;
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
  // Now look at each bucket. Only bother with a bucket that has at least four
  // scalar extracts in it.
  for (auto i = Buckets.begin(), e = Buckets.end(); i != e; ++i) {
    auto Bucket = &i->second;
    if (Bucket->size() < 4)
      continue;
    processBucket(&i->first, Bucket);
  }
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
void GenXExtractVectorizer::processBucket(const BucketIndex *BIdx,
    SmallVectorImpl<Extract> *B)
{
  // Sort the extracts into offset order.
  std::sort(B->begin(), B->end());
  // See if we have a sequence of offsets such that we can construct a
  // 1D region.
  int Diff = (*B)[1].Offset - (*B)[0].Offset;
  for (unsigned j = 1, je = B->size() - 1; j != je; ++j)
    if ((*B)[j + 1].Offset - (*B)[j].Offset != Diff)
      return;
  // Find the latest point that we can insert the vectorized instruction.
  SmallVector<Instruction *, 8> Insts;
  for (auto j = B->begin(), je = B->end(); j != je; ++j)
    Insts.push_back(j->Inst);
  auto InsertBefore = findClosestCommonDominator(DT, Insts);
  // Create the new rdregion.
  Extract *Extract0 = &(*B)[0];
  Region R(Extract0->Inst->getOperand(0));
  R.NumElements = R.Width = B->size();
  R.Stride = Diff / R.ElementBytes;
  R.Indirect = BIdx->Indirect;
  R.Offset = Extract0->Offset;
  Value *OrigVector = cast<Instruction>(Extract0->Inst->getOperand(0))
      ->getOperand(0);
  Value *NewRdRegion = OrigVector;
  // Need to splat if Diff is 0, otherwise elements extracted are wrong.
  if (Diff == 0 || R.Indirect || R.Offset ||
      R.NumElements != cast<IGCLLVM::FixedVectorType>(OrigVector->getType())
                           ->getNumElements()) {
    // Not identity region.
    NewRdRegion = R.createRdRegion(OrigVector,
        Extract0->Inst->getName() + ".histogrammed", InsertBefore,
        Extract0->Inst->getDebugLoc(), /*AllowScalar=*/false);
  }
  // Create the vectorized binary operator or trunc/zext/sext.
  Instruction *NewInst = nullptr;
  if (isa<BinaryOperator>(Extract0->Inst)) {
    // Create a vector of the constants used in the right side of the binary
    // operators.
    SmallVector<Constant *, 8> RhsConsts;
    for (auto j = B->begin(), je = B->end(); j != je; ++j)
      RhsConsts.push_back(cast<Constant>(j->Inst->getOperand(1)));
    auto CV = ConstantVector::get(RhsConsts);
    NewInst = BinaryOperator::Create(
        (Instruction::BinaryOps)Extract0->Inst->getOpcode(), NewRdRegion, CV,
        Extract0->Inst->getName() + ".histogrammed", InsertBefore);
  } else {
    // Create the vectorized trunc/zext/sext.
    auto VT =
        IGCLLVM::FixedVectorType::get(Extract0->Inst->getType(), B->size());
    NewInst = CastInst::Create((Instruction::CastOps)Extract0->Inst->getOpcode(),
        NewRdRegion, VT,
        Extract0->Inst->getName() + ".histogrammed", InsertBefore);
  }
  NewInst->setDebugLoc(Extract0->Inst->getDebugLoc());
  // For each original scalar binary operator or cast, create a rdregion to
  // extract the equivalent scalar from the result of the vectorized binary
  // operator, and use it to replace uses of the original binary operator.
  for (auto j = B->begin(), je = B->end(); j != je; ++j) {
    Region R2(NewInst);
    R2.NumElements = R2.Width = 1;
    R2.Offset = (j - B->begin()) * R2.ElementBytes;
    auto NewRdRegion2 = R2.createRdRegion(NewInst, "",
        InsertBefore, j->Inst->getDebugLoc(), /*AllowScalar=*/true);
    NewRdRegion2->takeName(j->Inst);
    j->Inst->replaceAllUsesWith(NewRdRegion2);
  }
  for (auto j = B->begin(), je = B->end(); j != je; ++j) {
    auto OldRdRegion = cast<Instruction>(j->Inst->getOperand(0));
    j->Inst->eraseFromParent();
    OldRdRegion->eraseFromParent();
  }
  // Add the new vectorized binary operator or cast back into
  // ExtractVectorizer so the extracts we added could in turn be vectorized.
  Extracted.push_back(NewInst);
  Modified = true;
}


