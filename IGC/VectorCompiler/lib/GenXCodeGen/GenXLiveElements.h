/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXLiveElements
/// --------------------
///
/// GenXLiveElements is an analysis pass that stores information about
/// live (i.e. possibly used in future) lanes of vector instructions and
/// their uses inside a function. Pass starts its work from root instructions
/// (terminators and side-effect instructions) and propagates this information
/// backwards until some stable point is not reached. Propagation function for
/// each instruction gets live elements of its result and estimates what
/// elements in operands are required to it. For unknown types of instructions
/// there is a safe fallback to estimate "all-live" result, but this can be
/// improved in future to get more precise results
//===----------------------------------------------------------------------===//
#ifndef GENXLIVEELEMENTS_H
#define GENXLIVEELEMENTS_H

#include "FunctionGroup.h"

#include "llvm/ADT/SmallBitVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {
namespace genx {

// LiveElements is an utility class to represent information about used elements
// inside some type, possibly aggregate. It contains bit masks for each simple
// (scalar or vector) value in flattened order for complex nested types. The
// special case is void type, which doesn't have any bit mask (even empty). Such
// empty LiveElements is treated as always used

class LiveElements {
private:
  using LiveElementsTy = SmallVector<SmallBitVector, 2>;
  LiveElementsTy LiveElems;

public:
  using size_type = LiveElementsTy::size_type;
  using iterator = LiveElementsTy::iterator;
  using const_iterator = LiveElementsTy::const_iterator;
  using reference = LiveElementsTy::reference;
  using const_reference = LiveElementsTy::const_reference;

  LiveElements() = default;
  LiveElements(Type *Ty, bool IsLive = false);
  LiveElements(const ArrayRef<SmallBitVector> LE)
      : LiveElems(LE.begin(), LE.end()) {}
  LiveElements(SmallBitVector &&LE) : LiveElems({std::move(LE)}) {}
  LiveElements(const SmallBitVector &LE) = delete;
  LiveElements &operator=(const SmallBitVector &) = delete;

  iterator begin() { return LiveElems.begin(); }
  const_iterator begin() const { return LiveElems.begin(); }
  iterator end() { return LiveElems.end(); }
  const_iterator end() const { return LiveElems.end(); }
  reference operator[](size_type Idx) { return LiveElems[Idx]; }
  const_reference operator[](size_type Idx) const { return LiveElems[Idx]; }
  size_type size() const { return LiveElems.size(); }

  bool isAllDead() const {
    return !LiveElems.empty() && all_of(LiveElems, [](const auto &LiveBits) {
      return !LiveBits.empty() && LiveBits.none();
    });
  }

  bool isAnyDead() const {
    return !LiveElems.empty() && any_of(LiveElems, [](const auto &LiveBits) {
      return !LiveBits.empty() && !LiveBits.all();
    });
  }

  bool operator==(const LiveElements &Rhs) const {
    return LiveElems == Rhs.LiveElems;
  }

  bool operator!=(const LiveElements &Rhs) const { return !(*this == Rhs); }

  LiveElements operator|=(const LiveElements &Rhs);

  void print(raw_ostream &OS) const;
#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
  void dump() const;
#endif // if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
};

inline LiveElements operator|(const LiveElements &Lhs,
                              const LiveElements &Rhs) {
  LiveElements Res(Lhs);
  Res |= Rhs;
  return Res;
}

inline raw_ostream &operator<<(raw_ostream &OS, const LiveElements &LE) {
  LE.print(OS);
  return OS;
}
} // namespace genx

// GenXLiveElements - analysis that can give information for 2 types of
// queries:
// 1. What elements of instruction result or function argument are live (i.e.
//    can be used later)
// 2. What elements of value are live in some particular use. This gives
//    more precise result that live elements of whole value, because
//    multiple uses of same value can use different elements
class GenXLiveElements {
public:
  void processFunction(const Function &F);
  void clear() { LiveMap.clear(); }

  genx::LiveElements getLiveElements(const Value *V) const;
  genx::LiveElements getLiveElements(const Use *U) const;

private:
  ValueMap<const Value *, genx::LiveElements> LiveMap;

  genx::LiveElements
  getBitCastLiveElements(const BitCastInst *BCI,
                         const genx::LiveElements &InstLiveElems) const;
  genx::LiveElements
  getExtractValueLiveElements(const ExtractValueInst *EVI, unsigned OperandNo,
                              const genx::LiveElements &InstLiveElems) const;
  genx::LiveElements
  getInsertValueLiveElements(const InsertValueInst *IVI, unsigned OperandNo,
                             const genx::LiveElements &InstLiveElems) const;
  genx::LiveElements
  getRdRegionLiveElements(const Instruction *RdR, unsigned OperandNo,
                          const genx::LiveElements &InstLiveElems) const;
  genx::LiveElements
  getWrRegionLiveElements(const Instruction *WrR, unsigned OperandNo,
                          const genx::LiveElements &InstLiveElems) const;
  genx::LiveElements
  getTwoDstInstLiveElements(const genx::LiveElements &InstLiveElems) const;
  genx::LiveElements
  getOperandLiveElements(const Instruction *Inst, unsigned OperandNo,
                         const genx::LiveElements &InstLiveElems) const;
};

// Function pass wrapper for GenXLiveElements
class GenXFuncLiveElements : public FunctionPass,
                             public GenXLiveElements {
public:
  static char ID;

  explicit GenXFuncLiveElements() : FunctionPass(ID) {}

  StringRef getPassName() const override {
    return "GenX live elements analysis for a function";
  }

  void releaseMemory() override { clear(); }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
  }

  bool runOnFunction(Function &F) override {
    clear();
    processFunction(F);
    return false;
  }
};

void initializeGenXFuncLiveElementsPass(PassRegistry &);

// FunctionGroup pass wrapper for GenXLiveElements
class GenXGroupLiveElements : public FGPassImplInterface,
                              public IDMixin<GenXGroupLiveElements>,
                              public GenXLiveElements {
public:
  static StringRef getPassName() {
    return "GenX live elements analysis for a function group";
  }

  void releaseMemory() override { clear(); }

  static void getAnalysisUsage(AnalysisUsage &AU) { AU.setPreservesAll(); }

  bool runOnFunctionGroup(FunctionGroup &FG) override {
    clear();
    for (auto &F : FG)
      processFunction(*F);
    return false;
  }
};

void initializeGenXGroupLiveElementsWrapperPass(PassRegistry &);

using GenXGroupLiveElementsWrapper =
    FunctionGroupWrapperPass<GenXGroupLiveElements>;
} // namespace llvm

#endif
