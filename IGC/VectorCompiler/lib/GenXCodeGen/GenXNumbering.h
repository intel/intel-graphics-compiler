/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXNumbering
/// -------------
///
/// GenXNumbering is an analysis that provides a numbering of the instructions
/// for use by live ranges.
///
/// The numbering is done such that slots are reserved for where GenXCoalescing
/// might need to insert copies.
///
/// Generally, an instruction gets a slot in the numbering for itself, and
/// another slot just before, in case it is a two address instruction where
/// GenXCoalescing might want to insert a copy.
///
/// Every instruction gets a number, even if it is baled in. However, for the
/// purposes of live range segments, every instruction in a bale is assumed
/// to have the same number as the head instruction of the bale.
///
/// A non-intrinsic call has N slots reserved
/// before it for pre-copies, where N is the number of SimpleValues in the
/// (possibly struct) args, allowing for extra args that might be added later by
/// GenXArgIndirection.
///
/// Similarly, a non-intrinsic call has N slots reserved after it for
/// post-copies, where N is the number of SimpleValues in the (possibly struct)
/// return value. The definition of each SimpleValue in the result of the call
/// is considered to be in its slot, and the corresponding SimpleValue in the
/// unified return value has an extra segment of live range from the call up to
/// that slot.
///
/// A return instruction in a subroutine has N slots reserved before it for
/// pre-copies, where N is the number of SimpleValues in the (possibly struct)
/// return value. The use of each SimpleValue in the return is considered to be
/// in its slot, and the corresponding SimpleValue in the unified return value
/// has an extra segment of live range from the slot up to the return.
///
/// A kernel has a slot for each kernel arg copy. A copy is inserted into such a slot in
/// GenXCoalescing if the kernel arg offset is not aligned enough for the uses
/// of the value.
///
/// **IR restriction**: After this pass, it is very difficult to modify code
/// other than by inserting copies in the reserved slots above, as it would
/// disturb the numbering.
///
//===----------------------------------------------------------------------===//
#ifndef GENXNUMBERING_H
#define GENXNUMBERING_H

#include "FunctionGroup.h"
#include "IgnoreRAUWValueMap.h"
#include "llvm/IR/Value.h"

#include <unordered_map>

namespace llvm {

class CallInst;
class GenXBaling;
class PHINode;
class ReturnInst;

class GenXNumbering : public FGPassImplInterface,
                      public IDMixin<GenXNumbering> {
  FunctionGroup *FG = nullptr;
  GenXBaling *Baling = nullptr;
  struct BBNumber {
    unsigned Index; // 0-based index in list of basic blocks
    unsigned PhiNumber; // instruction number of first phi node in successor
    unsigned EndNumber; // instruction number of end of block
  };
  // BBNumbers : The 0-based number (index) of each basic block.
  ValueMap<const BasicBlock *, BBNumber,
          IgnoreRAUWValueMapConfig<const BasicBlock *>> BBNumbers;
  // Numbers : The map of instruction numbers.
  ValueMap<const Value *, unsigned,
          IgnoreRAUWValueMapConfig<const Value *>> Numbers;
  // StartNumbers : for a CallInst, the start number of where arg pre-copies
  // are considered to be. This is stored, instead of being calculated from
  // the CallInst's number, so that a CallInst can change number of args, as
  // happens in GenXArgIndirection.
  ValueMap<const Value *, unsigned,
          IgnoreRAUWValueMapConfig<const Value *>> StartNumbers;
  // NumberToPhiIncomingMap : map from instruction number to the phi incoming
  // (phi node plus incoming index) it represents. We assume that a phi node is
  //  never deleted after GenXNumbering. It is implemented as multimap because
  //  several Phis may refer to the same PHYCPY segment.
  std::unordered_multimap<unsigned, std::pair<PHINode *, unsigned>>
      NumberToPhiIncomingMap;

  // The number for the entire fucntion group. All live ranges are included in
  // live-range [0, LastNum].
  unsigned LastNum = 0;

public:
  explicit GenXNumbering() : Baling(0) {}
  static StringRef getPassName() { return "GenX numbering"; }
  static void getAnalysisUsage(AnalysisUsage &AU);
  bool runOnFunctionGroup(FunctionGroup &FG) override;
  // get BBNumber struct for a basic block
  const BBNumber *getBBNumber(BasicBlock *BB) { return &BBNumbers[BB]; }
  // get and set instruction number
  unsigned getBaleNumber(Instruction *Inst);
  unsigned getNumber(Value *V) const;
  unsigned getLastNumber() const { return LastNum; }
  void setNumber(Value *V, unsigned Number);
  // get and set "start instruction number" for a CallInst
  unsigned getStartNumber(Value *V) { return StartNumbers[V]; }
  void setStartNumber(Value *V, unsigned Number) { StartNumbers[V] = Number; }
  // get number for kernel arg copy, arg pre-copy, ret pre-copy and ret post-copy sites
  unsigned getArgIndirectionNumber(CallInst *CI, unsigned OperandNum, unsigned Index);
  unsigned getKernelArgCopyNumber(Argument *Arg);
  unsigned getArgPreCopyNumber(CallInst *CI, unsigned OperandNum, unsigned Index);
  unsigned getRetPreCopyNumber(ReturnInst *RI, unsigned Index);
  unsigned getRetPostCopyNumber(CallInst *CI, unsigned Index);
  // get the number of a phi incoming, where its copy will be inserted
  // if necessary
  unsigned getPhiNumber(PHINode *Phi, BasicBlock *BB) const;
  unsigned getPhiNumber(PHINode *Phi, BasicBlock *BB);
  // getPhiIncomingFromNumber : get the phi incoming for a number returned from getPhiNumber
  std::unordered_map<PHINode *, unsigned>
  getPhiIncomingFromNumber(unsigned Number);
  // Debug dump
  void print(raw_ostream &OS) const;
  void dump();

  void releaseMemory() override;

private:
  unsigned numberInstructionsInFunc(Function *Func, unsigned Num);
  unsigned getPhiOffset(PHINode *Phi) const;
};

void initializeGenXNumberingWrapperPass(PassRegistry &);
using GenXNumberingWrapper = FunctionGroupWrapperPass<GenXNumbering>;

} // end namespace llvm
#endif //ndef GENXNUMBERING_H
