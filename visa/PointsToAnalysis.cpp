/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BuildIR.h"
#include "PointsToAnalysis.h"

using namespace vISA;

PointsToAnalysis::PointsToAnalysis(const DECLARE_LIST &declares,
                                   unsigned int numBB)
    : numBBs(numBB), numAddrs(0),
      indirectUses(std::make_unique<REGVAR_VECTOR[]>(numBB)) {
  for (auto decl : declares) {
    // add alias check, For Alias Dcl
    if ((decl->getRegFile() == G4_ADDRESS || decl->getRegFile() == G4_SCALAR) &&
        decl->getAliasDeclare() == NULL) // It is a base declaration, not alias
    {
      // participate liveness analysis
      decl->getRegVar()->setId(numAddrs++);
    } else {
      decl->getRegVar()->setId(UNDEFINED_VAL);
    }
  }

  // assign all addr aliases the same ID as its root
  for (auto decl : declares) {
    if ((decl->getRegFile() == G4_ADDRESS || decl->getRegFile() == G4_SCALAR) &&
        decl->getAliasDeclare() != NULL) {
      // participate liveness analysis
      decl->getRegVar()->setId(decl->getRootDeclare()->getRegVar()->getId());
    }
  }

  if (numAddrs > 0) {
    for (unsigned int i = 0; i < numAddrs; i++)
      regVars.push_back(NULL);

    for (auto decl : declares) {
      if ((decl->getRegFile() == G4_ADDRESS ||
           decl->getRegFile() == G4_SCALAR) &&
          decl->getAliasDeclare() == NULL &&
          decl->getRegVar()->getId() != UNDEFINED_VAL) {
        regVars[decl->getRegVar()->getId()] = decl->getRegVar();
      }
    }

    pointsToSets.resize(numAddrs);
    addrExpSets.resize(numAddrs);
    addrPointsToSetIndex.resize(numAddrs);
    // initially each address variable has its own points-to set
    for (unsigned i = 0; i < numAddrs; i++) {
      addrPointsToSetIndex[i] = i;
    }
  }
}

void PointsToAnalysis::resizePointsToSet(unsigned int newsize) {
  // Reallocate larger sets, change numAddrs.
  // Number of basic blocks is assumed to be unchanged.

  pointsToSets.resize(newsize);
  addrExpSets.resize(newsize);
  addrPointsToSetIndex.resize(newsize);
  for (unsigned i = numAddrs; i < newsize; i++) {
    addrPointsToSetIndex[i] = i;
  }

  numAddrs = newsize;
}

void PointsToAnalysis::addIndirectUseToBB(unsigned int bbId, pointInfo pt) {
  MUST_BE_TRUE(bbId < numBBs, "invalid basic block id");
  REGVAR_VECTOR &vec = indirectUses[bbId];
  auto it =
      std::find_if(vec.begin(), vec.end(), [&pt](const pointInfo &element) {
        return element.var == pt.var && element.off == pt.off;
      });

  if (it == vec.end()) {
    vec.push_back(pt);
  }
}

void PointsToAnalysis::mergePointsToSet(const G4_RegVar *addr1,
                                      const G4_RegVar *addr2) {
  MUST_BE_TRUE(addr1->getDeclare()->getRegFile() == G4_ADDRESS &&
                   addr2->getDeclare()->getRegFile() == G4_ADDRESS,
               "expect address variable");
  int addr2PTIndex = addrPointsToSetIndex[addr2->getId()];
  ADDREXP_VECTOR &vec = addrExpSets[addr2PTIndex];
  for (int i = 0; i < (int)vec.size(); i++) {
    addToPointsToSet(addr1, vec[i].exp, vec[i].off);
  }
  int addr1PTIndex = addrPointsToSetIndex[addr1->getId()];
  addrPointsToSetIndex[addr2->getId()] = addr1PTIndex;
  DEBUG_VERBOSE("merge Addr " << addr1->getId() << " with Addr "
                              << addr2->getId());
}

void PointsToAnalysis::addToPointsToSet(const G4_RegVar *addr, G4_AddrExp *opnd,
                      unsigned char offset) {
  MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS ||
                   addr->getDeclare()->getRegFile() == G4_SCALAR,
               "expect address variable");
  MUST_BE_TRUE(addr->getId() < numAddrs, "addr id is not set");
  int addrPTIndex = addrPointsToSetIndex[addr->getId()];
  REGVAR_VECTOR &vec = pointsToSets[addrPTIndex];
  pointInfo pi = {opnd->getRegVar(), offset};

  auto it =
      std::find_if(vec.begin(), vec.end(), [&pi](const pointInfo &element) {
        return element.var == pi.var && element.off == pi.off;
      });
  if (it == vec.end()) {
    vec.push_back(pi);
    DEBUG_VERBOSE("Addr " << addr->getId() << " <-- "
                          << var->getDeclare()->getName() << "\n");
  }

  addrExpInfo pi1 = {opnd, offset};
  ADDREXP_VECTOR &vec1 = addrExpSets[addrPTIndex];
  auto it1 =
      std::find_if(vec1.begin(), vec1.end(), [&pi1](addrExpInfo &element) {
        return element.exp == pi1.exp && element.off == pi1.off;
      });
  if (it1 == vec1.end()) {
    vec1.push_back(pi1);
  }
}

unsigned int PointsToAnalysis::getIndexOfRegVar(const G4_RegVar *r) const {
  // Given a regvar pointer, return the index it was
  // found. This function is useful when regvar ids
  // are reset.

  const auto it = std::find(regVars.begin(), regVars.end(), r);
  return it == regVars.end() ? UINT_MAX
                             : static_cast<unsigned int>(it - regVars.begin());
}

void PointsToAnalysis::addPointsToSetToBB(int bbId, const G4_RegVar *addr) {
  MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS ||
                   addr->getDeclare()->getRegFile() == G4_SCALAR,
               "expect address variable");
  const REGVAR_VECTOR &addrTakens =
      pointsToSets[addrPointsToSetIndex[addr->getId()]];
  for (auto addrTaken : addrTakens) {
    addIndirectUseToBB(bbId, addrTaken);
  }
}

// This function is intended to be invoked only in GRF RA. As that ensures
// points-to data structures are well populated and no new entry would be added
// to points-to table. If this condition is no longer true, then this function
// should be modified.
const std::unordered_map<G4_Declare *, std::vector<G4_Declare *>> &
PointsToAnalysis::getPointsToMap() {
  // return map computed earlier
  // assume no updates are made to points-to analysis table since first update
  if (addrTakenMap.size() > 0)
    return addrTakenMap;

  unsigned idx = 0;

  // populate map from each addr reg -> addr taken targets
  for (auto &RV : regVars) {
    auto ptsToIdx = addrPointsToSetIndex[idx];
    for (auto &item : pointsToSets[ptsToIdx])
      addrTakenMap[RV->getDeclare()->getRootDeclare()].push_back(
          item.var->getDeclare()->getRootDeclare());
    ++idx;
  }

  return addrTakenMap;
}

const std::unordered_map<G4_Declare *, std::vector<G4_Declare *>> &
PointsToAnalysis::getRevPointsToMap() {
  if (revAddrTakenMap.size() > 0)
    return revAddrTakenMap;

  // call the function instead of using direct member to guarantee the map is
  // populated
  auto &forwardMap = getPointsToMap();

  for (auto &entry : forwardMap) {
    for (auto &var : entry.second) {
      revAddrTakenMap[var].push_back(entry.first);
    }
  }

  return revAddrTakenMap;
}

//
//  A flow-insensitive algroithm to compute the register usage for indirect
//  accesses. The algorithm is divided into two parts:
//  1. We go through every basic block computing the points-to set for each
//  adddress
//     variable.  This happens when we see an instruction like
//     mov (8) A0 &R0
//
//  2. We go through each basic block again, and for each r[A0] expression
//     we mark variables in A0's points-to set as used in the block
//
//  The algorithm is conservative but should work well for our inputs since
//  the front end pretty much always uses a fresh address variable when taking
//  the address of a GRF variable, wtih the exception of call-by-reference
//  parameters It's performed only once at the beginning of RA, at the point
//  where all variables are virtual and no spill code (either for address or
//  GRF) has been inserted.
//
void PointsToAnalysis::doPointsToAnalysis(FlowGraph &fg) {
  if (numAddrs == 0) {
    return;
  }

  // keep a list of address taken variables
  std::vector<G4_RegVar *> addrTakenDsts;
  std::map<G4_RegVar *, std::vector<std::pair<G4_AddrExp *, unsigned char>>>
      addrTakenMapping;
  std::vector<std::pair<G4_AddrExp *, unsigned char>> addrTakenVariables;

  for (G4_BB *bb : fg) {
    for (const G4_INST *inst : *bb) {
      G4_DstRegRegion *dst = inst->getDst();
      if (dst && dst->getRegAccess() == Direct && dst->getType() != Type_UD) {
        G4_VarBase *ptr = dst->getBase();

        for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
          G4_Operand *src = inst->getSrc(i);
          if (src && src->isAddrExp()) {
            int offset = 0;
            if (dst && !dst->isNullReg() &&
                dst->getBase()->asRegVar()->getDeclare()->getRegFile() ==
                    G4_SCALAR) {
              offset = src->asAddrExp()->getOffset();
            }
            addrTakenMapping[ptr->asRegVar()].push_back(
                std::make_pair(src->asAddrExp(), offset));
            addrTakenDsts.push_back(ptr->asRegVar());
            addrTakenVariables.push_back(
                std::make_pair(src->asAddrExp(), offset));
          }
        }
      }
    }
  }

  // first compute the points-to set for each address variable
  for (G4_BB *bb : fg) {
    for (G4_INST *inst : *bb) {
      if (inst->isPseudoKill() || inst->isLifeTimeEnd()) {
        // No need to consider these lifetime placeholders for points2analysis
        continue;
      }

      G4_DstRegRegion *dst = inst->getDst();
      if (dst != NULL && dst->getRegAccess() == Direct &&
          dst->getType() != Type_UD) {
        G4_VarBase *ptr = dst->getBase();
        // Dst is address variable
        if (ptr->isRegVar() &&
            (ptr->asRegVar()->getDeclare()->getRegFile() == G4_ADDRESS ||
             ptr->asRegVar()->getDeclare()->getRegFile() == G4_SCALAR) &&
            !ptr->asRegVar()->getDeclare()->isMsgDesc())
        {

          // dst is an address variable.  ExDesc A0 may be ignored since they
          // are never used in indirect access
          if (inst->isMov() || inst->isPseudoAddrMovIntrinsic()) {
            for (int i = 0; i < inst->getNumSrc(); i++) {
              G4_Operand *src = inst->getSrc(i);
              if (!src || src->isNullReg()) {
                continue;
              }
              if (src->isAddrExp()) {
                // case 1:  mov A0 &GRF
                G4_RegVar *addrTaken = src->asAddrExp()->getRegVar();

                if (addrTaken != NULL) {
                  unsigned char offset = 0;
                  if (ptr->asRegVar()->getDeclare()->getRegFile() ==
                      G4_SCALAR) {
                    offset = src->asAddrExp()->getOffset();
                  }
                  addToPointsToSet(ptr->asRegVar(), src->asAddrExp(), offset);
                }
              } else {
                // G4_Operand* srcPtr = src->isSrcRegRegion() ?
                // src->asSrcRegRegion()->getBase() : src;
                G4_VarBase *srcPtr = src->isSrcRegRegion()
                                         ? src->asSrcRegRegion()->getBase()
                                         : nullptr;

                if (srcPtr && srcPtr->isRegVar() &&
                    (srcPtr->asRegVar()->getDeclare()->getRegFile() ==
                     G4_ADDRESS)) {
                  // case 2:  mov A0 A1
                  // merge the two addr's points-to set together
                  if (ptr->asRegVar()->getId() != srcPtr->asRegVar()->getId()) {
                    mergePointsToSet(srcPtr->asRegVar(), ptr->asRegVar());
                  }
                } else {
                  // case ?: mov v1 A0
                  // case ?: mov A0 v1
                  if (srcPtr && srcPtr->isRegVar() &&
                      addrTakenMapping[srcPtr->asRegVar()].size() != 0) {
                    for (int i = 0;
                         i < (int)addrTakenMapping[srcPtr->asRegVar()].size();
                         i++) {
                      addToPointsToSet(
                          ptr->asRegVar(),
                          addrTakenMapping[srcPtr->asRegVar()][i].first,
                          addrTakenMapping[srcPtr->asRegVar()][i].second);
                    }
                  } else {
                    // case 3: mov A0 0
                    // Initial of address register, igore the point to analysis
                    // FIXME: currently, vISA don't expect mov imm value to the
                    // address register. So, 0 is treated as initialization. If
                    // support mov A0 imm in future, 0 may be R0.
                    if (!(src->isImm() && (src->asImm()->getImm() == 0))) {
                      // case 4:  mov A0 V2
                      // conservatively assume address can point to anything
                      DEBUG_MSG("unexpected addr move for pointer analysis:\n");
                      DEBUG_EMIT(inst);
                      DEBUG_MSG("\n");
                      for (int i = 0, size = (int)addrTakenVariables.size();
                           i < size; i++) {
                        addToPointsToSet(ptr->asRegVar(),
                                         addrTakenVariables[i].first,
                                         addrTakenVariables[i].second);
                      }
                    }
                  }
                }
              }
            }
          } else if (inst->isArithmetic()) {
            G4_Operand *src0 = inst->getSrc(0);
            G4_Operand *src1 = inst->getSrc(1);
            bool src0addr = false;
            if (src0->isAddrExp()) {
              src0addr = true;
            } else if (src0->isSrcRegRegion() &&
                       src0->asSrcRegRegion()->getRegAccess() == Direct) {
              if (src0->isAddress()) {
                src0addr = true;
              }
            }

            bool src1addr = false;
            if (src1->isAddrExp()) {
              src1addr = true;
            } else if (src1->isSrcRegRegion() &&
                       src1->asSrcRegRegion()->getRegAccess() == Direct) {
              if (src1->isAddress()) {
                src1addr = true;
              }
            }

            if (src0addr ^ src1addr) {
              G4_Operand *src = src0addr ? src0 : src1;

              if (src->isAddrExp()) {
                // case 5:  add/mul A0 &GRF src1
                addToPointsToSet(ptr->asRegVar(), src->asAddrExp(), 0);
              } else {
                G4_VarBase *srcPtr = src->isSrcRegRegion()
                                         ? src->asSrcRegRegion()->getBase()
                                         : nullptr;
                // case 6:  add/mul A0 A1 src1
                // merge the two addr's points-to set together
                if (srcPtr &&
                    (ptr->asRegVar()->getId() != srcPtr->asRegVar()->getId())) {
                  mergePointsToSet(srcPtr->asRegVar(), ptr->asRegVar());
                }
              }
            } else if (ptr->isRegVar() && ptr->asRegVar()->isPhyRegAssigned()) {
              // OK, using builtin a0 or a0.2 directly.
            } else {
              // case 7:  add/mul A0 V1 V2
              DEBUG_MSG("unexpected addr add/mul for pointer analysis:\n");
              DEBUG_EMIT(inst);
              DEBUG_MSG("\n");
              for (int i = 0; i < (int)addrTakenVariables.size(); i++) {
                addToPointsToSet(ptr->asRegVar(), addrTakenVariables[i].first,
                                 0);
              }
            }
          } else {
            // case 8: A0 = ???
            DEBUG_MSG("unexpected instruction with address destination:\n");
            DEBUG_EMIT(inst);
            DEBUG_MSG("\n");
            for (int i = 0; i < (int)addrTakenVariables.size(); i++) {
              addToPointsToSet(ptr->asRegVar(), addrTakenVariables[i].first,
                               addrTakenVariables[i].second);
            }
          }
        }
        else if (ptr->isRegVar() && !ptr->asRegVar()->getDeclare()->isMsgDesc())
        {
          for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
            G4_Operand *src = inst->getSrc(i);
            G4_VarBase *srcPtr = (src && src->isSrcRegRegion())
                                     ? src->asSrcRegRegion()->getBase()
                                     : nullptr;
            // We don't support using "r[a0.0]" as address expression.
            // For instructions like following, it's not point-to propagation
            // for simdShuffle and add64_i_i_i_i.
            // (W) mov (1) simdShuffle(0,0)<1>:d  r[A0(0,0), 0]<0;1,0>:d
            //     pseudo_mad (16)      add64_i_i_i_i(0,0)<1>:d  0x6:w
            //     simdShuffle(0,0)<0;0>:d  rem_i_i_i_i(0,0)<1;0>:d shl (16)
            //     add64_i_i_i_i(0,0)<1>:d add64_i_i_i_i(0,0)<1;1,0>:d  0x2:w
            if (srcPtr != nullptr && srcPtr->isRegVar() && ptr != srcPtr &&
                !src->isIndirect()) {
              std::vector<G4_RegVar *>::iterator addrDst =
                  std::find(addrTakenDsts.begin(), addrTakenDsts.end(),
                            srcPtr->asRegVar());
              if (addrDst != addrTakenDsts.end()) {
                addrTakenDsts.push_back(ptr->asRegVar());
                for (int i = 0;
                     i < (int)addrTakenMapping[srcPtr->asRegVar()].size();
                     i++) {
                  addrTakenMapping[ptr->asRegVar()].push_back(
                      addrTakenMapping[srcPtr->asRegVar()][i]);
                }
              }
            }
          }
        }
      }
    }
  }

#ifdef DEBUG_VERBOSE_ON
  DEBUG_VERBOSE("Results of points-to analysis:\n");
  for (unsigned int i = 0; i < numAddrs; i++) {
    DEBUG_VERBOSE("Addr " << i);
    for (G4_RegVar *grf : pointsToSets[addrPointsToSetIndex[i]]) {
      DEBUG_EMIT(grf);
      DEBUG_VERBOSE("\t");
    }
    DEBUG_VERBOSE("\n");
  }
#endif

  // mark GRF that may be used indirect access as live in the block
  // This includes all GRFs in the address's points-to set
  for (auto bb : fg) {
    for (G4_INST *inst : *bb) {
      G4_DstRegRegion *dst = inst->getDst();

      if (dst != NULL && dst->getRegAccess() == IndirGRF) {
        G4_VarBase *dstptr = dst->getBase();
        MUST_BE_TRUE(
            dstptr->isRegVar() &&
                (dstptr->asRegVar()->getDeclare()->getRegFile() == G4_ADDRESS ||
                 dstptr->asRegVar()->getDeclare()->getRegFile() == G4_SCALAR),
            "base must be address");
        addPointsToSetToBB(bb->getId(), dstptr->asRegVar());
      }

      for (unsigned j = 0, numSrc = inst->getNumSrc(); j < numSrc; j++) {
        // look for indirect reg access r[ptr] which refers addrTaken reg var
        if (!inst->getSrc(j) || !inst->getSrc(j)->isSrcRegRegion()) {
          continue;
        }

        G4_SrcRegRegion *src = inst->getSrc(j)->asSrcRegRegion();

        if (src->getRegAccess() == IndirGRF) {
          G4_VarBase *srcptr = src->getBase();
          MUST_BE_TRUE(
              srcptr->isRegVar() &&
                  (srcptr->asRegVar()->getDeclare()->getRegFile() ==
                       G4_ADDRESS ||
                   srcptr->asRegVar()->getDeclare()->getRegFile() == G4_SCALAR),
              "base must be address");
          addPointsToSetToBB(bb->getId(), srcptr->asRegVar());
        }
      }
    }
  }

#ifndef NDEBUG
  for (unsigned i = 0; i < numAddrs; i++) {
    REGVAR_VECTOR &vec = pointsToSets[addrPointsToSetIndex[i]];
    for (const pointInfo cur : vec) {
      unsigned indirectVarSize = cur.var->getDeclare()->getByteSize();
      assert((indirectVarSize <=
              fg.builder->getGRFSize() * fg.getKernel()->getNumRegTotal()) &&
             "indirected variables' size is larger than GRF file size");
    }
  }
#endif

#ifdef DEBUG_VERBOSE_ON
  for (unsigned int i = 0; i < numBBs; i++) {
    DEBUG_VERBOSE("Indirect uses for BB" << i << "\t");
    const REGVAR_VECTOR &grfVec = getIndrUseVectorForBB(i);
    for (G4_RegVar *grf : grfVec) {
      DEBUG_EMIT(grf);
      DEBUG_VERBOSE("\t");
    }
    DEBUG_VERBOSE("\n");
  }
#endif
}

void PointsToAnalysis::insertAndMergeFilledAddr(const G4_RegVar *addr1,
                                              G4_RegVar *addr2) {
  unsigned int oldid = addr2->getId();
  addr2->setId(numAddrs);
  MUST_BE_TRUE(
      regVars.size() == numAddrs,
      "Inconsistency found between size of regvars and number of addr vars");

  resizePointsToSet(numAddrs + 1);

  regVars.push_back(addr2);

  mergePointsToSet(addr1, addr2);
  addr2->setId(oldid);
}

const REGVAR_VECTOR *
PointsToAnalysis::getAllInPointsTo(const G4_RegVar *addr) const {
  MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS ||
                   addr->getDeclare()->getRegFile() == G4_SCALAR,
               "expect address variable");
  unsigned int id = getIndexOfRegVar(addr);

  if (id == UINT_MAX)
    return nullptr;

  const REGVAR_VECTOR *vec = &pointsToSets[addrPointsToSetIndex[id]];

  return vec;
}

ADDREXP_VECTOR *PointsToAnalysis::getAllInPointsToAddrExps(G4_RegVar *addr) {
  MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS ||
                   addr->getDeclare()->getRegFile() == G4_SCALAR,
               "expect address variable");
  unsigned int id = getIndexOfRegVar(addr);

  if (id == UINT_MAX)
    return nullptr;

  ADDREXP_VECTOR *vec = &addrExpSets[addrPointsToSetIndex[id]];

  return vec;
}

const REGVAR_VECTOR &
PointsToAnalysis::getAllInPointsToOrIndrUse(const G4_Operand *opnd,
                                               const G4_BB *bb) const {
  const REGVAR_VECTOR *pointsToSet =
      getAllInPointsTo(opnd->getBase()->asRegVar());
  if (pointsToSet != nullptr)
    return *pointsToSet;
  // this can happen if the address is coming from addr spill
  // ToDo: we can avoid this by linking the spilled addr with its new temp
  // addr
  return getIndrUseVectorForBB(bb->getId());
}

G4_RegVar *PointsToAnalysis::getPointsTo(const G4_RegVar *addr, int idx) const {
  MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS ||
                   addr->getDeclare()->getRegFile() == G4_SCALAR,
               "expect address variable");
  unsigned int id = getIndexOfRegVar(addr);

  if (id == UINT_MAX)
    return NULL;

  const REGVAR_VECTOR &vec = pointsToSets[addrPointsToSetIndex[id]];

  if (idx < (int)vec.size())
    return vec[idx].var;
  else
    return NULL;
}

G4_RegVar *PointsToAnalysis::getPointsTo(const G4_RegVar *addr, int idx,
                       unsigned char &offset) const {
  MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS ||
                   addr->getDeclare()->getRegFile() == G4_SCALAR,
               "expect address variable");
  unsigned int id = getIndexOfRegVar(addr);

  if (id == UINT_MAX)
    return NULL;
  int addrPTIndex = addrPointsToSetIndex[id];

  const REGVAR_VECTOR &vec = pointsToSets[addrPTIndex];

  if (idx < (int)vec.size()) {
    offset = vec[idx].off;
    return vec[idx].var;
  } else
    return NULL;
}

bool PointsToAnalysis::isPresentInPointsTo(const G4_RegVar *addr,
                                         const G4_RegVar *var) const {
  MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS ||
                   addr->getDeclare()->getRegFile() == G4_SCALAR,
               "expect address variable");
  unsigned int id = getIndexOfRegVar(addr);

  if (id == UINT_MAX)
    return false;

  const REGVAR_VECTOR &vec = pointsToSets[addrPointsToSetIndex[id]];
  for (const pointInfo pointsTo : vec) {
    if (pointsTo.var->getId() == var->getId()) {
      return true;
    }
  }

  return false;
}

void PointsToAnalysis::addFillToPointsTo(unsigned int bbid, G4_RegVar *addr,
                                       G4_RegVar *newvar) {
  // Adds to points to as well as indirect use in basic block
  MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS ||
                   addr->getDeclare()->getRegFile() == G4_SCALAR,
               "expect address variable");
  unsigned int id = getIndexOfRegVar(addr);

  if (id == UINT_MAX) {
    MUST_BE_TRUE(false, "Could not find addr in points to set");
  }

  REGVAR_VECTOR &vec = pointsToSets[addrPointsToSetIndex[id]];
  pointInfo pt = {newvar, 0};
  vec.push_back(pt);

  addIndirectUseToBB(bbid, pt);
}

void PointsToAnalysis::patchPointsToSet(const G4_RegVar *addr, G4_AddrExp *opnd,
                      unsigned char offset) {
  MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS,
               "expect address variable");
  unsigned int id = getIndexOfRegVar(addr);
  int addrPTIndex = addrPointsToSetIndex[id];
  REGVAR_VECTOR &vec = pointsToSets[addrPTIndex];
  pointInfo pi = {opnd->getRegVar(), offset};

  auto it =
      std::find_if(vec.begin(), vec.end(), [&pi](const pointInfo &element) {
        return element.var == pi.var && element.off == pi.off;
      });
  if (it == vec.end()) {
    vec.push_back(pi);
    DEBUG_VERBOSE("Addr " << addr->getId() << " <-- "
                          << var->getDeclare()->getName() << "\n");
  }

  addrExpInfo pi1 = {opnd, offset};
  ADDREXP_VECTOR &vec1 = addrExpSets[addrPTIndex];
  auto it1 =
      std::find_if(vec1.begin(), vec1.end(), [&pi1](addrExpInfo &element) {
        return element.exp == pi1.exp && element.off == pi1.off;
      });
  if (it1 == vec1.end()) {
    vec1.push_back(pi1);
  }
}

void PointsToAnalysis::removeFromPointsTo(G4_RegVar *addr,
                                        G4_RegVar *vartoremove) {
  MUST_BE_TRUE(addr->getDeclare()->getRegFile() == G4_ADDRESS ||
                   addr->getDeclare()->getRegFile() == G4_SCALAR,
               "expect address variable");
  unsigned int id = getIndexOfRegVar(addr);

  if (id == UINT_MAX) {
    MUST_BE_TRUE(false, "Could not find addr in points to set");
  }

  REGVAR_VECTOR &vec = pointsToSets[addrPointsToSetIndex[id]];
  bool removed = false;

  for (REGVAR_VECTOR::iterator it = vec.begin(); it != vec.end(); it++) {
    pointInfo cur = (*it);

    if (cur.var->getId() == vartoremove->getId()) {
      vec.erase(it);
      removed = true;
      break;
    }
  }

  ADDREXP_VECTOR &vec1 = addrExpSets[addrPointsToSetIndex[id]];
  for (ADDREXP_VECTOR::iterator it = vec1.begin(); it != vec1.end(); it++) {
    addrExpInfo cur = (*it);

    if (cur.exp->getRegVar()->getId() == vartoremove->getId()) {
      vec1.erase(it);
      removed = true;
      break;
    }
  }

  MUST_BE_TRUE(removed == true, "Could not find spilled ref from points to");

  // If an addr taken live-range is spilled then any basic block that has
  // an indirect use of it will no longer have it because we would have
  // inserted addr taken spill/fill code. So remove any indirect uses of
  // the var from all basic blocks. Currently this set is used when
  // constructing liveness sets before RA.
  for (unsigned int i = 0; i < numBBs; i++) {
    REGVAR_VECTOR &vec = indirectUses[i];

    for (REGVAR_VECTOR::iterator it = vec.begin(); it != vec.end(); it++) {
      pointInfo cur = (*it);

      if (cur.var->getId() == vartoremove->getId()) {
        vec.erase(it);
        break;
      }
    }
  }
}