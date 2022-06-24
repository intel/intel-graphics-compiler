/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// GenXNumbering is an analysis that provides a numbering of the instructions
// for use by live range segments. See GenXNumbering.h.
//
//===----------------------------------------------------------------------===//
#include "GenXNumbering.h"
#include "GenX.h"
#include "GenXBaling.h"
#include "GenXLiveness.h"
#include "vc/Utils/GenX/KernelInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/Debug.h"

#include "llvmWrapper/IR/InstrTypes.h"
#include "Probe/Assertion.h"

#define DEBUG_TYPE "GENX_NUMBERING"

using namespace llvm;
using namespace genx;

INITIALIZE_PASS_BEGIN(GenXNumberingWrapper, "GenXNumberingWrapper",
                      "GenXNumberingWrapper", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXGroupBalingWrapper)
INITIALIZE_PASS_END(GenXNumberingWrapper, "GenXNumberingWrapper",
                    "GenXNumberingWrapper", false, false)

ModulePass *llvm::createGenXNumberingWrapperPass() {
  initializeGenXNumberingWrapperPass(*PassRegistry::getPassRegistry());
  return new GenXNumberingWrapper();
}

void GenXNumbering::getAnalysisUsage(AnalysisUsage &AU) {
  AU.addRequired<GenXGroupBaling>();
  AU.setPreservesAll();
}

/***********************************************************************
 * runOnFunctionGroup : run pass
 */
bool GenXNumbering::runOnFunctionGroup(FunctionGroup &ArgFG)
{
  FG = &ArgFG;
  Baling = &getAnalysis<GenXGroupBaling>();
  unsigned Num = 0;
  for (auto fgi = FG->begin(), fge = FG->end(); fgi != fge; ++fgi)
    Num = numberInstructionsInFunc(*fgi, Num);
  LastNum = Num;
  return false;
}

/***********************************************************************
 * releaseMemory : clear the GenXNumbering
 */
void GenXNumbering::releaseMemory() {
  BBNumbers.clear();
  Numbers.clear();
  NumberToPhiIncomingMap.clear();
}

/***********************************************************************
 * numberInstructionsInFunc : number the instructions in a function
 */
unsigned GenXNumbering::numberInstructionsInFunc(Function *Func, unsigned Num)
{
  // Number the function, reserving one number for the args.
  Numbers[Func] = Num++;
  for (Function::iterator fi = Func->begin(), fe = Func->end(); fi != fe; ++fi) {
    BasicBlock *Block = &*fi;
    // Number the basic block.
    auto BBNumber = &BBNumbers[Block];
    BBNumber->Index = BBNumbers.size() - 1;
    Numbers[Block] = Num++;
    // If this is the first block of a kernel, reserve kernel arg copy slots.
    if (Block == &Func->front() && vc::isKernel(Func))
      for (auto ai = Func->arg_begin(), ae = Func->arg_end(); ai != ae; ++ai)
        ++Num;
    // Iterate the instructions.
    Instruction *Inst;
    for (BasicBlock::iterator bi = Block->begin(); ; ++bi) {
      Inst = &*bi;
      if (Inst->isTerminator())
        break;
      // For most instructions, reserve one number for any pre-copy that
      // coalescing needs to insert, and nothing after.
      unsigned PreReserve = 1, PostReserve = 0;
      if (auto CI = dyn_cast<CallInst>(Inst)) {
        if (!GenXIntrinsic::isAnyNonTrivialIntrinsic(CI) &&
            !CI->isInlineAsm()) {
          // For a non-intrinsic call, reserve enough numbers before the call
          // for:
          //  - a slot for each element of the args, two numbers per element:
          //    1. one for the address setup in case it is an address arg added
          //       by arg indirection (as returned by getArgIndirectionNumber());
          //    2. one for a pre-copy inserted if coalescing fails (as returned
          //       by getArgPreCopyNumber());
          //
          //  - a similar slot with two numbers for any address arg added by
          //    arg indirection (also as returned by getArgIndirectionNumber()
          //    and getArgPreCopyNumber()).
          //
          // Reserve enough numbers after the call for:
          //  -  post-copies of (elements of) the return value, as returned by
          //     getRetPostCopyNumber().
          //
          // Note that numbers get wasted because most call args do not need
          // two slots, and most calls never have address args added by arg
          // indirection. But treating all call args the same is easier, and
          // wasting numbers does not really matter.
          PreReserve = 2 * IndexFlattener::getNumArgElements(
                CI->getFunctionType());
          PreReserve += 2 * IGCLLVM::getNumArgOperands(CI); // extra for pre-copy addresses of args
          unsigned NumRetVals = IndexFlattener::getNumElements(CI->getType());
          PreReserve += NumRetVals; // extra for pre-copy addresses of retvals
          PostReserve = NumRetVals;
          // Set the start number of the call so users of numbering can work out
          // where the pre-copies are assumed to start, even if the call gets
          // modified later by GenXArgIndirection.
          setStartNumber(CI, Num);
        }
      }
      // Number the instruction, reserving PreReserve.
      Num += PreReserve;
      Numbers[Inst] = Num;
      Num += 1 + PostReserve;
    }
    // We have reached the terminator instruction but not yet numbered it.
    // Reserve a number for each phi node in the successor. If there is
    // more than one successor (this is a critical edge), then allow for
    // whichever successor has the most phi nodes.
    BBNumber->PhiNumber = Num;
    auto TI = cast<IGCLLVM::TerminatorInst>(Block->getTerminator());
    unsigned MaxPhis = 0;
    for (unsigned i = 0, e = TI->getNumSuccessors(); i != e; ++i) {
      BasicBlock *Succ = TI->getSuccessor(i);
      unsigned NumPhis = 0;
      for (BasicBlock::iterator sbi = Succ->begin(), sbe = Succ->end(); sbi != sbe; ++sbi) {
        if (!isa<PHINode>(&*sbi))
          break;
        NumPhis++;
      }
      if (NumPhis > MaxPhis)
        MaxPhis = NumPhis;
    }
    Num += MaxPhis;
    // Now number the terminator instruction. Doing it here ensures that any
    // input to the terminator instruction interferes with the results of the
    // phi nodes of the successor.
    unsigned PreReserve = 1;
    if (isa<ReturnInst>(Inst)) {
      // For a return, reserve enough numbers before for pre-copies of
      // (elements of) the return value.
      PreReserve = IndexFlattener::getNumElements(Func->getReturnType());
    }
    Num += PreReserve;
    Numbers[Inst] = Num++;
    BBNumber->EndNumber = Num;
  }
  return Num;
}

/***********************************************************************
 * getBaleNumber : get instruction number for head of bale, 0 if none
 */
unsigned GenXNumbering::getBaleNumber(Instruction *Inst)
{
  Inst = Baling->getBaleHead(Inst);
  return getNumber(Inst);
}

/***********************************************************************
 * getNumber : get instruction number, or 0 if none
 */
unsigned GenXNumbering::getNumber(Value *V) const {
  auto i = Numbers.find(V), e = Numbers.end();
  if (i == e)
    return 0;
  return i->second;
}

/***********************************************************************
 * setNumber : get instruction number
 */
void GenXNumbering::setNumber(Value *V, unsigned Number)
{
  Numbers[V] = Number;
}

/***********************************************************************
 * getArgIndirectionNumber : get number of arg indirection slot for call arg
 *
 * Enter:   CI = CallInst
 *          OperandNum = operand (arg) number
 *          Index = flattened index in the struct
 *
 * Each flattened index in each call arg has an arg indirection slot before the
 * call instruction, where a copy will be inserted if coalescing fails. Each
 * slot in fact has two numbers, and this returns the first one. (The second
 * one is used for arg pre-copy when coalescing fails.)
 */
unsigned GenXNumbering::getArgIndirectionNumber(CallInst *CI, unsigned OperandNum,
    unsigned Index)
{
  auto FT = cast<FunctionType>(CI->getFunctionType());
  return getStartNumber(CI) + 2 * (IndexFlattener::flattenArg(FT, OperandNum)
        + Index);
}

/***********************************************************************
 * getKernelArgCopyNumber : get number of kernel arg copy slot
 */
unsigned GenXNumbering::getKernelArgCopyNumber(Argument *Arg)
{
  IGC_ASSERT(vc::isKernel(Arg->getParent()));
  return Numbers[&Arg->getParent()->front()] + 1 + Arg->getArgNo();
}

/***********************************************************************
 * getArgPreCopyNumber : get number of pre-copy slot for call arg
 *
 * Enter:   CI = CallInst
 *          OperandNum = operand (arg) number
 *          Index = flattened index in the struct
 *
 * Each flattened index in each call arg has an arg pre-copy slot before the
 * call instruction, where a copy will be inserted if coalescing fails. Each
 * slot in fact has two numbers, and this returns the second one. (The first
 * one is used for address loading in arg indirection.)
 */
unsigned GenXNumbering::getArgPreCopyNumber(CallInst *CI, unsigned OperandNum,
    unsigned Index)
{
  return getArgIndirectionNumber(CI, OperandNum, Index) + 1;
}

/***********************************************************************
 * getRetPreCopyNumber : get number of pre-copy slot for return value
 *
 * Enter:   RI = ReturnInst
 *          Index = flattened index in the struct
 *
 * For each flattened index in the return type, there is one slot before the
 * return instruction.
 */
unsigned GenXNumbering::getRetPreCopyNumber(ReturnInst *RI, unsigned Index)
{
  return getNumber(RI)
      - IndexFlattener::getNumElements(RI->getOperand(0)->getType()) + Index;
}

/***********************************************************************
 * getRetPostCopyNumber : get number of post-copy slot for return value
 *
 * Enter:   CI = CallInst
 *          Index = flattened index in the struct
 *
 * For each flattened index in the return type, there is one slot after the call
 * instruction.
 */
unsigned GenXNumbering::getRetPostCopyNumber(CallInst *CI, unsigned Index)
{
  return getNumber(CI) + 1 + Index;
}

/***********************************************************************
 * getPhiNumber : get instruction number for phi node for particular predecessor
 *
 * The non-const version caches the result in NumberToPhiIncomingMap, for the
 * later use of getPhiIncomingFromNumber.
 */
unsigned GenXNumbering::getPhiNumber(PHINode *Phi, BasicBlock *BB) const
{
  // The instruction number is the count of phi nodes before it added to the
  // PhiNumber for the predecessor.
  return BBNumbers.find(BB)->second.PhiNumber + getPhiOffset(Phi);
}

unsigned GenXNumbering::getPhiNumber(PHINode *Phi, BasicBlock *BB)
{
  unsigned Number = ((const GenXNumbering *)this)->getPhiNumber(Phi, BB);
  NumberToPhiIncomingMap.emplace(
      Number, std::make_pair(Phi, Phi->getBasicBlockIndex(BB)));
  return Number;
}

/***********************************************************************
 * getPhiIncomingFromNumber : get the phi incoming for a number returned from
 *    getPhiNumber
 *
 * This returns the phi node and incoming index corresponding to the supplied
 * instruction number.
 */
std::unordered_map<PHINode *, unsigned>
GenXNumbering::getPhiIncomingFromNumber(unsigned Number) {
  auto Range = NumberToPhiIncomingMap.equal_range(Number);
  std::unordered_map<PHINode *, unsigned> PHIs;
  PHIs.reserve(std::distance(Range.first, Range.second));
  std::transform(Range.first, Range.second, std::inserter(PHIs, PHIs.begin()),
                 [](auto It) { return It.second; });
  return PHIs;
}

/***********************************************************************
 * getPhiOffset : get phi node offset (the 0 based index within its block)
 */
unsigned GenXNumbering::getPhiOffset(PHINode *Phi) const
{
  // Count phi nodes from start of basic block to here.
  unsigned Count = 0;
  for (BasicBlock::const_iterator bi = Phi->getParent()->begin(); &*bi != Phi; ++bi)
    ++Count;
  return Count;
}

/***********************************************************************
 * dump, print : dump the instruction numbering
 */
#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
void GenXNumbering::dump()
{
  print(errs()); errs() << '\n';
}
#endif

void GenXNumbering::print(raw_ostream &OS) const
{
  OS << "GenXNumbering for FunctionGroup " << FG->getName() << "\n";
  for (auto fgi = FG->begin(), fge = FG->end(); fgi != fge; ++fgi) {
    Function *Func = *fgi;
    if (FG->size() != 1)
      OS << Func->getName() << ":\n";
    for (Function::iterator fi = Func->begin(), fe = Func->end(); fi != fe; ++fi) {
      BasicBlock *BB = &*fi;
      OS << "\n" << Numbers.find(BB)->second << " " << BB->getName() << ":\n";
      for (BasicBlock::iterator bi = BB->begin(), be = BB->end(); bi != be; ++bi) {
        Instruction *Inst = &*bi;
        if (Numbers.find(Inst) == Numbers.end())
          OS << " - ";
        else
          OS << Numbers.find(Inst)->second;
        OS << "   ";
        Inst->print(OS);
        OS << "\n";
      }
      auto TI = cast<IGCLLVM::TerminatorInst>(BB->getTerminator());
      if (TI->getNumSuccessors()) {
        BasicBlock *Succ = TI->getSuccessor(0);
        for (BasicBlock::iterator sbi = Succ->begin(), sbe = Succ->end(); sbi != sbe; ++sbi) {
          if (PHINode *Phi = dyn_cast<PHINode>(&*sbi)) {
            OS << "(" << getPhiNumber(Phi, BB) << ")  ";
            Phi->print(OS);
            OS << "\n";
          } else
            break;
        }
      }
    }
  }
  OS << "\n";
}

