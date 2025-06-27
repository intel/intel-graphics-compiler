/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_IR_REGSET_HPP
#define _IGA_IR_REGSET_HPP

// Future replacement for RegDeps(.cpp/.hpp)
// Need to decouple machine state logic from the dependency set.

#include "BitSet.hpp"
#include "Instruction.hpp"
#include "Kernel.hpp"

#include <ostream>
#include <string>

namespace iga {
// A register set represent all the storage in the register files
// at a certain granularity (currently typically byte).
//
class RegSet {
public:
  using Bits = BitSet<>;
  // TODO:
  // using Bits = FlexibleBitSet
private:
  Bits *bitSetForPtr(RegName rn);

public:
  Bits &bitSetFor(RegName rn);
  const Bits &bitSetFor(RegName rn) const;

private:
  size_t offsetOf(RegName rn, int reg) const;
  size_t offsetOf(RegName rn, RegRef rr, size_t typeSize) const;
  size_t offsetOf(RegName rn, RegRef rr, Type t) const;
  bool isTrackedReg(RegName rn) const;

public:
  ~RegSet() = default;
  RegSet(const Model &m);
  RegSet(const RegSet &rs);
  RegSet &operator=(const RegSet &);

  const Model &getModel() const { return model; }

  bool operator==(const RegSet &rs) const;
  bool operator!=(const RegSet &rs) const { return !(*this == rs); }

  ///////////////////////////////////////////////////////////////////////
  // set operations
  bool empty() const;
  void reset();
  bool intersects(const RegSet &rhs) const;
  // bool  containsAll(const RegSet &rhs) const;
  bool destructiveUnion(const RegSet &inp);
  bool destructiveSubtract(const RegSet &rhs);
  bool intersectInto(const RegSet &rhs, RegSet &into) const;

  ///////////////////////////////////////////////////////////////////////
  // generic add functions
  bool addReg(RegName rn, int reg);
  bool addRegs(RegName rn, int reg, int n = 1);
  bool add(RegName rn, size_t regFileOffBits, size_t numBits);
  bool add(RegName rn, RegRef r, Type t);

  // for pred or condmod
  bool addFlagRegArf(RegRef fr, ExecSize es, ChannelOffset co);

  ///////////////////////////////////////////////////////////////////////
  // specialized add functions for full instructions
  //
  // all the source inputs (including implicit acc), excluding predicate
  bool addSourceInputs(const Instruction &i);
  //
  // just the predicate
  bool addPredicationInputs(const Instruction &i);
  //
  // just operand 'srcIx'
  bool addSourceOperandInput(const Instruction &i, int srcIx);
  //
  // implicit sources such as acc or flag register in a select
  bool addSourceImplicit(const Instruction &i);
  //
  // just send descriptor inputs
  bool addSourceDescriptorInputs(const Instruction &i);
  //
  // just uses of a0 as part of an indirect register access
  bool addDestinationInputs(const Instruction &i); // register indirect
  //
  //////////////////////
  // destination outputs including acc, but excluding predicate (flagmod)
  bool addDestinationOutputs(const Instruction &i);
  //
  // just the destination operand omitting acc
  bool addExplicitDestinationOutputs(const Instruction &i);
  //
  // just the conditional modifier
  bool addFlagModifierOutputs(const Instruction &i);
  //
  // implicit destinations such as acc
  bool addDestinationImplicit(const Instruction &i);

  //////////////////////////////////////
  // both source and destination can cover to this
  bool addImplicitAccumulatorAccess(const Instruction &i, bool isDst);

  // both predicates and flag modifiers can cover to this
  bool addFlagAccess(const Instruction &i);

  ///////////////////////////////////////
  // helpers for operand regions
  bool setDstRegion(RegName rn, RegRef rr, Region r, size_t execSize,
                    size_t typeSizeBits);
  bool setSrcRegion(RegName rn, RegRef rr, Region r, size_t execSize,
                    size_t typeSizeBits);

  void str(std::ostream &os) const;
  std::string str() const;

  static std::string str(const Model &m, RegName rn, const RegSet::Bits &bs);

  static RegSet unionOf(const RegSet &r1, const RegSet &r2) {
    RegSet r12(r1.model);
    (void)r12.destructiveUnion(r1);
    (void)r12.destructiveUnion(r2);
    return r12;
  }
  static RegSet::Bits emptyPredSet(const Model &model);
  static RegSet intersection(const RegSet &rs1, const RegSet &rs2) {
    IGA_ASSERT(&rs1.model == &rs2.model, "model mismatch");
    RegSet rsI(rs1.model);
    rs1.intersectInto(rs2, rsI);
    return rsI;
  }

private:
  bool addSendOperand(const Instruction &i, int srcIx);
  bool addDpasOperand(const Instruction &i, int srcIx);

private:
  const Model &model;
  Bits bitsR;
  Bits bitsA;
  Bits bitsAcc;
  Bits bitsF;
  Bits bitsS;
};

struct InstSrcs {
  RegSet predication;
  RegSet sources;
  InstSrcs(const Model &m) : predication(m), sources(m) {}

  RegSet unionOf() const { return RegSet::unionOf(predication, sources); }
  bool empty() const { return predication.empty() && sources.empty(); }
  std::string str() const { return unionOf().str(); }

  static InstSrcs compute(const Instruction &i);
};
struct InstDsts {
  RegSet flagModifier;
  RegSet destinations;

  InstDsts(const Model &m) : flagModifier(m), destinations(m) {}

  RegSet unionOf() const { return RegSet::unionOf(flagModifier, destinations); }
  bool subtractFrom(RegSet &rs) const {
    bool changed = false;
    changed |= rs.destructiveSubtract(flagModifier);
    changed |= rs.destructiveSubtract(destinations);
    return changed;
  }
  bool empty() const { return flagModifier.empty() && destinations.empty(); }
  std::string str() const { return unionOf().str(); }

  static InstDsts compute(const Instruction &i);
};

inline InstSrcs InstSrcs::compute(const Instruction &i) {
  InstSrcs iss(Model::LookupModelRef(i.platform()));
  iss.predication.addPredicationInputs(i);
  iss.sources.addSourceInputs(i);
  return iss;
}
inline InstDsts InstDsts::compute(const Instruction &i) {
  InstDsts ids(Model::LookupModelRef(i.platform()));
  ids.destinations.reset();
  ids.flagModifier.addFlagModifierOutputs(i);
  ids.destinations.addDestinationOutputs(i);
  return ids;
}
} // namespace iga

#endif // _IGA_IR_REGSET_HPP
