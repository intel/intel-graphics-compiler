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
/// GenXVectorDecomposer
/// --------------------
///
/// GenXVectorDecomposer is not a pass; instead it is a class is called by by
/// the GenXPostLegalization pass to perform vector decomposition.
///
/// For a vector written by wrregion and read by rdregion, it finds the way that
/// the vector can be divided into parts, with each part a range of one or more
/// GRFs, such that no rdregion or wrregion crosses a part boundary. Then it
/// decomposes the vector into those parts. A rdregion/wrregion that reads/writes
/// a whole part can be removed completely; a rdregion/wrregion that reads/writes
/// only some of the part is replaced to read/write just the applicable part.
///
/// In fact it does all this for a web of vectors linked by wrregion, phi nodes
/// and bitcasts.
///
/// The idea is that having lots of small vectors instead of one big vector
/// reduces register fragmentation in the finalizer's register allocator.
///
/// There is an option -limit-genx-vector-decomposer=N to aid debugging the code
/// changes made by the vector decomposer.
///
//===----------------------------------------------------------------------===//
#include "GenXRegion.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Instructions.h"
#include <map>
#include <set>

namespace llvm {

class Constant;
class DominatorTree;
class Instruction;
class PHINode;
class Type;
class Use;

namespace gen {
class Region;
}

// VectorDecomposer : decomposes vectors in a function
class VectorDecomposer {
  DominatorTree *DT;
  const DataLayout *DL = nullptr;
  SmallVector<Instruction *, 16> StartWrRegions;
  std::set<Instruction *> Seen;
  SmallVector<Instruction *, 16> Web;
  SmallVector<Instruction *, 16> ToDelete;
  bool NotDecomposing;
  Instruction *NotDecomposingReportInst;
  SmallVector<unsigned, 8> Decomposition;
  SmallVector<unsigned, 8> Offsets;
  std::map<PHINode *, SmallVector<Value *, 8>> PhiParts;
  SmallVector<Instruction *, 8> NewInsts;
public:
  // clear : clear anything stored
  void clear() {
    clearOne();
    StartWrRegions.clear();
    Seen.clear();
    ToDelete.clear();
  }
  // addStartWrRegion : add a wrregion with undef input to the list
  void addStartWrRegion(Instruction *Inst) { StartWrRegions.push_back(Inst); }
  // run : run the vector decomposer on the stored StartWrRegions
  bool run(DominatorTree *DT);
private:
  // clearOne : clear from processing one web
  void clearOne() {
    Web.clear();
    Decomposition.clear();
    Offsets.clear();
    PhiParts.clear();
    NewInsts.clear();
  }
  bool processStartWrRegion(Instruction *Inst);
  bool determineDecomposition(Instruction *Inst);
  void addToWeb(Value *V, Instruction *User = nullptr);
  void adjustDecomposition(Instruction *Inst);
  void setNotDecomposing(Instruction *Inst, const char *Text);
  void decompose();
  void decomposeTree(Use *U, const SmallVectorImpl<Value *> *PartsIn);
  void decomposePhiIncoming(PHINode *Phi, unsigned OperandNum,
                            const SmallVectorImpl<Value *> *PartsIn);
  void decomposeRdRegion(Instruction *RdRegion,
                         const SmallVectorImpl<Value *> *PartsIn);
  void decomposeWrRegion(Instruction *WrRegion, SmallVectorImpl<Value *> *Parts);
  void decomposeBitCast(Instruction *Inst, SmallVectorImpl<Value *> *Parts);
  unsigned getPartIndex(genx::Region *R);
  unsigned getPartOffset(unsigned PartIndex);
  unsigned getPartNumBytes(Type *WholeTy, unsigned PartIndex);
  unsigned getPartNumElements(Type *WholeTy, unsigned PartIndex);
  VectorType *getPartType(Type *WholeTy, unsigned PartIndex);
  Constant *getConstantPart(Constant *Whole, unsigned PartIndex);
  void removeDeadCode();
  void eraseInst(Instruction *Inst);

  void emitWarning(Instruction *Inst, const Twine &Msg);
};

// Decompose predicate computation sequences for select
// to reduce flag register pressure.
class SelectDecomposer {
  const GenXSubtarget *ST;
  bool NotDecomposing = false;
  SmallVector<Instruction *, 8> StartSelects;
  SmallVector<Instruction *, 16> Web;
  SmallVector<unsigned, 8> Decomposition;
  SmallVector<unsigned, 8> Offsets;
  std::set<Instruction *> Seen;

  // Map each decomposed instructions to its corresonding part values.
  SmallDenseMap<Value *, SmallVector<Value *, 8>> DMap;

public:
  explicit SelectDecomposer(const GenXSubtarget *ST) : ST(ST) {}
  void addStartSelect(Instruction *Inst) { StartSelects.push_back(Inst); }
  bool run();

private:
  void clear() {
    NotDecomposing = false;
    Web.clear();
    Decomposition.clear();
    Offsets.clear();
    Seen.clear();
    DMap.clear();
  }
  bool processStartSelect(Instruction *Inst);
  bool determineDecomposition(Instruction* Inst);
  void setNotDecomposing() { NotDecomposing = true; }
  void addToWeb(Value *V);
  void decompose(Instruction *Inst);
  void decomposeSelect(Instruction *Inst);
  void decomposeBinOp(Instruction *Inst);
  void decomposeCmp(Instruction *Inst);

  unsigned getPartOffset(unsigned PartIndex) const {
    return Offsets[PartIndex];
  }
  unsigned getPartNumElements(unsigned PartIndex) const {
    return Decomposition[PartIndex];
  }
  Value *getPart(Value *Whole, unsigned PartIndex, Instruction *Inst) const;
};

} // end namespace llvm
