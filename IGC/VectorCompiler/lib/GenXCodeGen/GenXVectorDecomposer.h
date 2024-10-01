/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
/// decomposes the vector into those parts. A rdregion/wrregion that
/// reads/writes a whole part can be removed completely; a rdregion/wrregion
/// that reads/writes only some of the part is replaced to read/write just the
/// applicable part.
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

#pragma once

#include "GenX.h"
#include "GenXSubtarget.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Instructions.h"

#include <map>
#include <set>

namespace llvm {
class Constant;
class Instruction;
class PHINode;
class Type;
class Use;

class GenXSubtarget;
} // namespace llvm

namespace vc {
class Region;

// VectorDecomposer : decomposes vectors in a function
class VectorDecomposer {
  using Constant = llvm::Constant;
  using DataLayout = llvm::DataLayout;
  using Instruction = llvm::Instruction;
  using PHINode = llvm::PHINode;
  using Type = llvm::Type;
  using Twine = llvm::Twine;
  using Use = llvm::Use;
  using Value = llvm::Value;
  using VectorType = llvm::VectorType;

  using GenXSubtarget = llvm::GenXSubtarget;

  const DataLayout *DL = nullptr;
  const GenXSubtarget *ST;

  llvm::SmallVector<Instruction *, 16> StartWrRegions;
  std::set<Instruction *> Seen;
  llvm::SmallVector<Instruction *, 16> Web;
  llvm::SmallVector<Instruction *, 16> ToDelete;
  bool NotDecomposing = false;
  Instruction *NotDecomposingReportInst = nullptr;
  llvm::SmallVector<unsigned, 8> Decomposition;
  llvm::SmallVector<unsigned, 8> Offsets;
  std::map<PHINode *, llvm::SmallVector<Value *, 8>> PhiParts;
  llvm::SmallVector<Instruction *, 8> NewInsts;
  unsigned DecomposedCount = 0;
  const unsigned GRFByteSize;

public:
  explicit VectorDecomposer(const GenXSubtarget *ST)
      : ST(ST), GRFByteSize(ST ? ST->getGRFByteSize() : llvm::genx::defaultGRFByteSize) {}

  // clear : clear anything stored
  void clear() {
    clearOne();
    StartWrRegions.clear();
    Seen.clear();
    ToDelete.clear();
    DecomposedCount = 0;
  }
  // addStartWrRegion : add a wrregion with undef input to the list
  void addStartWrRegion(Instruction *Inst) { StartWrRegions.push_back(Inst); }
  // run : run the vector decomposer on the stored StartWrRegions
  bool run(const DataLayout &ArgDL);

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
  void decomposeTree(Use *U, const llvm::SmallVectorImpl<Value *> *PartsIn);
  void decomposePhiIncoming(PHINode *Phi, unsigned OperandNum,
                            const llvm::SmallVectorImpl<Value *> *PartsIn);
  void decomposeRdRegion(Instruction *RdRegion,
                         const llvm::SmallVectorImpl<Value *> *PartsIn);
  void decomposeWrRegion(Instruction *WrRegion,
                         llvm::SmallVectorImpl<Value *> *Parts);
  void decomposeBitCast(Instruction *Inst,
                        llvm::SmallVectorImpl<Value *> *Parts);
  unsigned getPartIndex(Region *R);
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
  using GenXSubtarget = llvm::GenXSubtarget;
  using Instruction = llvm::Instruction;
  using Value = llvm::Value;

  const GenXSubtarget *ST;
  bool NotDecomposing = false;
  llvm::SmallVector<Instruction *, 8> StartSelects;
  llvm::SmallVector<Instruction *, 16> Web;
  llvm::SmallVector<unsigned, 8> Decomposition;
  llvm::SmallVector<unsigned, 8> Offsets;
  std::set<Instruction *> Seen;

  // Map each decomposed instructions to its corresonding part values.
  llvm::SmallDenseMap<Value *, llvm::SmallVector<Value *, 8>> DMap;

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
  bool determineDecomposition(Instruction *Inst);
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
} // namespace vc
