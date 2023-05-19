/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _POINTSTOANALYSIS_H_
#define _POINTSTOANALYSIS_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "Assertions.h"
//#include "common.h"
#include "FlowGraph.h"
//#include "G4_BB.hpp"
//#include "G4_IR.hpp"
//#include "G4_Opcode.h"

namespace vISA {
struct pointInfo {
  G4_RegVar *var;
  unsigned char off;
};

struct addrExpInfo {
  G4_AddrExp *exp;
  unsigned char off;
};

typedef std::vector<pointInfo> REGVAR_VECTOR;
typedef std::vector<addrExpInfo> ADDREXP_VECTOR;
typedef std::vector<G4_RegVar *> ORG_REGVAR_VECTOR;

/*
 *  Performs flow-insensitive points-to analysis.
 *  Points-to analysis is performed once at the beginning of RA,
 *  and is used to compute the indirect uses of GRF variables for liveness
 *  analysis. Each address variable in the declare list is associated with a
 *  points-to set, which is a list of GRF RegVars.
 */
class PointsToAnalysis {
private:
  G4_Kernel *kernel = nullptr;
  const unsigned int numBBs;
  unsigned int numAddrs;

  // keeps track of the indirect variables used in each BB
  const std::unique_ptr<REGVAR_VECTOR[]> indirectUses;
  // vector of points-to set for each address variable
  std::vector<REGVAR_VECTOR> pointsToSets;
  // vector of address expression for each address variable
  std::vector<ADDREXP_VECTOR> addrExpSets;
  // index of an address's points-to set in the pointsToSets vector
  std::vector<unsigned> addrPointsToSetIndex;
  // original regvar ptrs
  ORG_REGVAR_VECTOR regVars;

  void resizePointsToSet(unsigned int newsize);

  void addPointsToSetToBB(int bbId, const G4_RegVar *addr);

  bool isVar(G4_RegVar *var, pointInfo &pt) { return pt.var == var; }

  void addIndirectUseToBB(unsigned int bbId, pointInfo pt);

  void addToPointsToSet(const G4_RegVar *addr, G4_AddrExp *opnd,
                        unsigned char offset);

  // Merge addr2's points-to set into addr1's
  // basically we copy the content of addr2's points-to to addr1,
  // and have addr2 point to addr1's points-to set
  void mergePointsToSet(const G4_RegVar *addr1, const G4_RegVar *addr2);

  unsigned int getIndexOfRegVar(const G4_RegVar *r) const;

public:
  PointsToAnalysis(const DECLARE_LIST &declares, unsigned numBBs);

  void dump(std::ostream &os = std::cerr) const;

  // addr reg -> pointee regs
  void
  getPointsToMap(std::unordered_map<G4_Declare *, std::vector<G4_Declare *>> &) const;

  // pointee -> addr reg
  void getRevPointsToMap(
      std::unordered_map<G4_Declare *, std::vector<G4_Declare *>> &);

  void doPointsToAnalysis(FlowGraph &fg);

  const REGVAR_VECTOR &getIndrUseVectorForBB(unsigned int bbId) const {
    vISA_ASSERT(bbId < numBBs, "invalid basic block id");
    return indirectUses[bbId];
  }

  // Following methods were added to support address taken spill/fill
  // Since ids of addr regvars will be reset, we fall back to using
  // the regvar ptr
  void insertAndMergeFilledAddr(const G4_RegVar *addr1, G4_RegVar *addr2);

  const REGVAR_VECTOR *getAllInPointsTo(const G4_RegVar *addr) const;

  ADDREXP_VECTOR *getAllInPointsToAddrExps(G4_RegVar *addr);

  const REGVAR_VECTOR &getAllInPointsToOrIndrUse(const G4_Operand *opnd,
                                                 const G4_BB *bb) const;

  G4_RegVar *getPointsTo(const G4_RegVar *addr, int idx) const;

  G4_RegVar *getPointsTo(const G4_RegVar *addr, int idx,
                         unsigned char &offset) const;

  bool isPresentInPointsTo(const G4_RegVar *addr, const G4_RegVar *var) const;

  void addFillToPointsTo(unsigned int bbid, G4_RegVar *addr, G4_RegVar *newvar);

  void patchPointsToSet(const G4_RegVar *addr, G4_AddrExp *opnd,
                        unsigned char offset);

  void removeFromPointsTo(G4_RegVar *addr, G4_RegVar *vartoremove);
};
} // namespace vISA
#endif
