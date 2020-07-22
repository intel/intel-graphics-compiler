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
#ifndef GENX_CONSTANTS_H
#define GENX_CONSTANTS_H

#include "GenXSubtarget.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"

namespace llvm {
namespace genx {

// ConstantLoader : class to insert instruction(s) to load a constant
class ConstantLoader {
  Constant *C;
  Instruction *User;
  // NewC != nullptr signals that we should replace C with NewC in User
  // nothing to do otherwise
  Constant *NewC = nullptr;
  // AddedInstructions: a vector that the caller has requested any added
  // instructions to be pushed in to.
  SmallVectorImpl<Instruction *> *AddedInstructions;
  // Info from analyzing for possible packed vector constant.
  int64_t PackedIntScale = 0;  // amount to scale packed int vector by
  int64_t PackedIntAdjust; // amount to adjust by, special casing 0 or -8
                           //  when PackedIntScale is 1
  unsigned PackedIntMax;   // max value in packed vector, used when scale is
                           //  1 and adjust is 0 to tell whether it would fit
                           //  in 0..7
  bool PackedFloat = false;

public:
  // Constructor
  // User = the instruction that uses the constant. If this is genx.constanti,
  //        then a packed vector constant can be an isSimple() constant even
  //        when the element type is not i16. Also used to disallow a packed
  //        vector constant in a logic op. If User==0 then it is assumed that
  //        a packed vector constant with an element type other than i16 is OK.
  // AddedInstructions = vector to add new instructions to when loading a
  //        non simple constant, so the caller can see all the newly added
  //        instructions.
  ConstantLoader(Constant *C, Instruction *User = nullptr,
                 SmallVectorImpl<Instruction *> *AddedInstructions = nullptr,
                 const GenXSubtarget *Subtarget = nullptr)
      : C(C), User(User), AddedInstructions(AddedInstructions),
        Subtarget(Subtarget) {
    analyze();
  }
  ConstantLoader(Constant *C, const GenXSubtarget *Subtarget)
      : ConstantLoader(C, nullptr, nullptr, Subtarget) {}
  Instruction *load(Instruction *InsertBefore);
  Instruction *loadBig(Instruction *InsertBefore);
  Instruction *loadNonSimple(Instruction *InsertBefore);
  bool needFixingSimple() const { return NewC; }
  void fixSimple(int OperandIdx);
  bool isBigSimple();
  bool isSimple();
  bool isLegalSize();

private:
  const GenXSubtarget *Subtarget;
  bool isPackedIntVector();
  bool isPackedFloatVector();
  void analyze();
  Constant *getConsolidatedConstant(Constant *C);
  unsigned getRegionBits(unsigned NeededBits, unsigned OptionalBits,
                         unsigned VecWidth);
  void analyzeForPackedInt(unsigned NumElements);
  void analyzeForPackedFloat(unsigned NumElements);
  Instruction *loadSplatConstant(Instruction *InsertPos);
};

// Some instructions force their operands to be constants.
// Check here if operand of instruction must be constant.
inline bool opMustBeConstant(Instruction *I, unsigned OpNum) {
  // Mask of shufflevector should always be constant.
  if (isa<ShuffleVectorInst>(I))
    return OpNum == 2;
  return false;
}

// Load a constant using the llvm.genx.constant intrinsic.
inline Instruction *
loadConstant(Constant *C, Instruction *InsertBefore,
             SmallVectorImpl<Instruction *> *AddedInstructions = nullptr,
             const GenXSubtarget *Subtarget = nullptr) {
  return ConstantLoader(C, nullptr, AddedInstructions, Subtarget)
      .load(InsertBefore);
}

// Load non-simple constants used in an instruction.
bool loadNonSimpleConstants(
    Instruction *Inst,
    SmallVectorImpl<Instruction *> *AddedInstructions = nullptr,
    const GenXSubtarget *Subtarget = nullptr);

bool loadConstantsForInlineAsm(
    CallInst *Inst, SmallVectorImpl<Instruction *> *AddedInstructions = nullptr,
    const GenXSubtarget *Subtarget = nullptr);

// Load constants used in an instruction.
bool loadConstants(Instruction *Inst, const GenXSubtarget *Subtarget = nullptr);

// Load constants used in phi nodes in a function.
bool loadPhiConstants(Function *F, DominatorTree *DT,
                      bool ExcludePredicate = false,
                      const GenXSubtarget *Subtarget = nullptr);

} // namespace genx
} // namespace llvm

#endif // GENX_CONSTANTS_H
