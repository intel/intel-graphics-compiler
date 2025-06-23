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
    if ((decl->getRegFile() == G4_ADDRESS || decl->getRegFile() == G4_SCALAR) &&
        !decl->getAliasDeclare()) {
      auto regVarsSize = (unsigned)regVars.size();
      regVars.emplace(decl->getRegVar(), regVarsSize);
    }
  }

  numAddrs = regVars.size();
  if (numAddrs > 0) {
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
  vISA_ASSERT(bbId < numBBs, "invalid basic block id");
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
  unsigned addr1Id = getIndexOfRegVar(addr1);
  unsigned addr2Id = getIndexOfRegVar(addr2);

  vISA_ASSERT(addr1->getDeclare()->getRegFile() == G4_ADDRESS &&
                  addr2->getDeclare()->getRegFile() == G4_ADDRESS,
              "expect address variable");
  int addr2PTIndex = addrPointsToSetIndex[addr2Id];
  ADDREXP_VECTOR &vec = addrExpSets[addr2PTIndex];
  for (int i = 0; i < (int)vec.size(); i++) {
    addToPointsToSet(addr1, vec[i].exp, vec[i].off);
  }
  int addr1PTIndex = addrPointsToSetIndex[addr1Id];
  addrPointsToSetIndex[addr2Id] = addr1PTIndex;
  DEBUG_VERBOSE("merge Addr " << addr1Id << " with Addr " << addr2Id);
}

void PointsToAnalysis::addToPointsToSet(const G4_RegVar *addr, G4_AddrExp *opnd,
                      unsigned char offset) {
  unsigned addrId = getIndexOfRegVar(addr);
  vISA_ASSERT(addr->getDeclare()->getRegFile() == G4_ADDRESS ||
                   addr->getDeclare()->getRegFile() == G4_SCALAR,
               "expect address variable");
  vISA_ASSERT(addrId < numAddrs, "addr id is not set");
  int addrPTIndex = addrPointsToSetIndex[addrId];
  REGVAR_VECTOR &vec = pointsToSets[addrPTIndex];
  pointInfo pi = {opnd->getRegVar(), offset};

  auto it =
      std::find_if(vec.begin(), vec.end(), [&pi](const pointInfo &element) {
        return element.var == pi.var && element.off == pi.off;
      });
  if (it == vec.end()) {
    vec.push_back(pi);
    DEBUG_VERBOSE("Addr " << addrId << " <-- "
                          << pi.var->getDeclare()->getName() << "\n");
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
  const G4_RegVar *addr = getRootRegVar(r);
  auto it = regVars.find(addr);
  if (it != regVars.end()) {
    return it->second;
  } else {
    return UINT_MAX;
  }
}

void PointsToAnalysis::addPointsToSetToBB(int bbId, const G4_RegVar *addr) {
  unsigned addrId = getIndexOfRegVar(addr);
  vISA_ASSERT(addr->getDeclare()->getRegFile() == G4_ADDRESS ||
                   addr->getDeclare()->getRegFile() == G4_SCALAR,
               "expect address variable");
  const REGVAR_VECTOR &addrTakens = pointsToSets[addrPointsToSetIndex[addrId]];
  for (const auto &addrTaken : addrTakens) {
    addIndirectUseToBB(bbId, addrTaken);
  }
}

void PointsToAnalysis::dump(std::ostream &os) const {
  // Dump computed points-to set in human readable format. For eg,
  //
  // A0 -> [V1, V2]
  // A1 -> [V3]
  // A2 -> []
  //
  // Note that non-address registers may contain pointers too.

  if (!kernel)
    return;

  std::unordered_map<const G4_Declare *, std::vector<G4_Declare *>> addrTakenMap;
  getPointsToMap(addrTakenMap);

  for (auto dcl : kernel->Declares) {
    auto foundIt = addrTakenMap.find(dcl);
    if (foundIt == addrTakenMap.end())
      continue;

    os << (*foundIt).first->getName() << " -> [";
    auto numPointees = (*foundIt).second.size();
    for (unsigned int i = 0; i != numPointees; ++i) {
      os << (*foundIt).second[i]->getName();
      if (i != numPointees - 1)
        os << ", ";
    }
    os << "]" << std::endl;
  }
}

void PointsToAnalysis::getPointsToMap(
  std::unordered_map<const G4_Declare *, std::vector<G4_Declare *>> &addrTakenMap) const {

  // populate map from each addr reg -> addr taken targets
  for (auto &RV : regVars) {
    unsigned idx = RV.second;
    const G4_RegVar *var = RV.first;
    auto ptsToIdx = addrPointsToSetIndex[idx];
    for (auto &item : pointsToSets[ptsToIdx])
      addrTakenMap[var->getDeclare()->getRootDeclare()].push_back(
          item.var->getDeclare()->getRootDeclare());
    ++idx;
  }
}

void PointsToAnalysis::getRevPointsToMap(
    std::unordered_map<G4_Declare *, std::vector<const G4_Declare *>>
        &revAddrTakenMap) {
  std::unordered_map<const G4_Declare *, std::vector<G4_Declare *>> forwardMap;
  getPointsToMap(forwardMap);

  for (auto &entry : forwardMap) {
    for (auto &var : entry.second) {
      revAddrTakenMap[var].push_back(entry.first);
    }
  }
}

//
//  A flow-insensitive algorithm to compute the register usage for indirect
//  accesses. The algorithm is divided into two parts:
//  1. We go through every basic block computing the points-to set for each
//  address
//     variable.  This happens when we see an instruction like
//     mov (8) A0 &R0
//
//  2. We go through each basic block again, and for each r[A0] expression
//     we mark variables in A0's points-to set as used in the block
//
//  The algorithm is conservative but should work well for our inputs since
//  the front end pretty much always uses a fresh address variable when taking
//  the address of a GRF variable, with the exception of call-by-reference
//  parameters It's performed only once at the beginning of RA, at the point
//  where all variables are virtual and no spill code (either for address or
//  GRF) has been inserted.
//
void PointsToAnalysis::doPointsToAnalysis(FlowGraph &fg) {
  kernel = fg.getKernel();

  if (numAddrs == 0) {
    return;
  }

  // keep a list of address taken variables
  std::vector<G4_RegVar *> addrTakenDsts;
  // map variable containing address to pointed-to variable
  // eg, A0 = &V10+0
  // addrTakenMapping would map A0 -> [V10, 0]
  std::map<G4_RegVar *, std::vector<std::pair<G4_AddrExp *, unsigned char>>>
      addrTakenMapping;
  // List of variables that ever appear as G4_AddrExp in program
  std::vector<std::pair<G4_AddrExp *, unsigned char>> addrTakenVariables;

  // Iterate over all instructions and gather operands that are either
  // pointers (eg, addr reg, scalar) or are address takens (ie, G4_AddrExp).
  for (G4_BB *bb : fg) {
    for (const G4_INST *inst : *bb) {
      G4_DstRegRegion *dst = inst->getDst();
      // TODO: Using Type_UD condition is pretty flaky. It's probably checking
      // whether dst is a msg descriptor (which also uses addr reg with Type_UD).
      // Ideally, this should be a property of the operand so we don't have
      // type based check.
      if (dst && dst->getRegAccess() == Direct && dst->getType() != Type_UD) {
        G4_VarBase *ptr = dst->getBase();

        for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
          G4_Operand *src = inst->getSrc(i);
          if (src && src->isAddrExp()) {
            int offset = 0;
            if (dst && !dst->isNullReg() &&
                dst->getBase()->asRegVar()->getDeclare()->getRegFile() ==
                    G4_SCALAR) {
              offset = src->asAddrExp()->getOffset() / fg.builder->getGRFSize();
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
          //
          // This block handles the case where dst is a pointer.
          // It excludes the case where dst address register uses Type_UD as
          // that indicates msg descriptor initialization.
          if (inst->isMov() || inst->isPseudoAddrMovIntrinsic()) {
            // PseudoAddrMoveIntrinsic may appear in G4 IR as:
            // intrinsic.pseudo_addr_mov (1)  s0.0<1>:uq  &CCTuple+0 &CCTuple+64
            // &M2+128  &M2+192  &M2+256  &M2+320  &CCTuple+0  &CCTuple+64
            //
            // Address of several general variables is taken and written to s0.0
            // for use in sendi.
            //
            // We need to mark s0.0 -> [CCTuple, M2] in points-to set.
            for (int i = 0; i < inst->getNumSrc(); i++) {
              G4_Operand *src = inst->getSrc(i);
              if (!src || src->isNullReg()) {
                continue;
              }
              if (src->isAddrExp()) {
                // mov A0 &GRF
                // Directly map A0 -> GRF
                G4_RegVar *addrTaken = src->asAddrExp()->getRegVar();

                if (addrTaken != NULL) {
                  unsigned char offset = 0;
                  if (ptr->asRegVar()->getDeclare()->getRegFile() ==
                      G4_SCALAR) {
                    offset = src->asAddrExp()->getOffset() /
                             fg.builder->getGRFSize();
                  }
                  addToPointsToSet(ptr->asRegVar(), src->asAddrExp(), offset);
                }
              } else {
                // Src may be address register or a general register but not
                // G4_AddrExp.
                //
                // For eg,
                // 1. A0 = A1 <-- In this case, A1 is a pointer so we merge
                //                points-to set of A1 with A0.
                // 2. A2 = V1 <-- In this case, we copy pointer from V1 to
                //                address register A2. Prior to this, there
                //                could've been an instruction that did V1 = A1
                //                which transferred pointer from A1 to V1. So
                //                we need to transfer A1's pointee to A2 in
                //                that case.
                //
                // Note that this is a context insensitive pass. So when we
                // encounter scenarios like #1 or #2, we merge points-to sets
                // instead of copying. In other words, points-to set of A0 and
                // A1 would become identical in case#1 above.
                G4_VarBase *srcPtr = src->isSrcRegRegion()
                                         ? src->asSrcRegRegion()->getBase()
                                         : nullptr;

                if (srcPtr && srcPtr->isRegVar() &&
                    (srcPtr->asRegVar()->getDeclare()->getRegFile() ==
                     G4_ADDRESS)) {
                  // mov A0 A1
                  // Merge the two addr's points-to set together as stated in
                  // #1 above. For eg, consider following program:
                  // A0 = &V0
                  // A1 = &V1
                  // if
                  //   A1 = A0
                  // endif
                  //    = r[A1]
                  //
                  // Both A0 and A1 contain V0 and V1 in their points-to set.
                  G4_RegVar *ptrVar = ptr->asRegVar();
                  G4_RegVar *srcPtrVar = srcPtr->asRegVar();
                  unsigned int ptrId = getIndexOfRegVar(ptrVar);
                  unsigned int srcPtrId = getIndexOfRegVar(srcPtrVar);
                  if (ptrId != srcPtrId) {
                    mergePointsToSet(srcPtrVar, ptrVar);
                  }
                } else {
                  // mov A0 v1
                  //
                  // Copy over pointee's from V1 to A0, if V1 is a
                  // pointer.
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
                    // mov A0 0 OR
                    // mov A0 V1 where V1 is not a pointer
                    //
                    // Initial of address register, ignore the point-to analysis
                    // FIXME: currently, vISA don't expect mov imm value to the
                    // address register. So, 0 is treated as initialization. If
                    // support mov A0 imm in future, 0 may be R0.
                    if (!(src->isImm() && (src->asImm()->getImm() == 0))) {
                      // mov A0 V1
                      //
                      // Conservatively assume address can point to any address
                      // taken.
                      VISA_DEBUG({
                        std::cout
                            << "unexpected addr move for pointer analysis:\n";
                        inst->emit(std::cout);
                        std::cout << "\n";
                      });
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
            // Dst is address type operand. Operation is of arithmetic type:
            // A0 = V1 op V2
            //
            // It's assumed arithmetic operation is restricted to 2-srcs only
            // and only one of those may be an address. Following patterns are
            // recognized:
            //
            // Address on src0:
            // A0 = &V1 op V2 <-- V1 is address OR
            // A0 = A1 op V2 <-- A1 is address
            //
            // Address on src1:
            // A0 = V1 op &V2 <-- V2 is address OR
            // A0 = V1 op A1 <-- A1 is address
            //
            G4_Operand *src0 = inst->getSrc(0);
            G4_Operand *src1 = inst->getSrc(1);
            bool src0addr = false;
            if (src0->isAddrExp()) {
              src0addr = true;
            } else if (src0->isSrcRegRegion() &&
                       src0->asSrcRegRegion()->isDirectAddress()) {
              src0addr = true;
            }

            bool src1addr = false;
            if (src1->isAddrExp()) {
              src1addr = true;
            } else if (src1->isSrcRegRegion() &&
                       src1->asSrcRegRegion()->isDirectAddress()) {
              src1addr = true;
            }

            if (src0addr ^ src1addr) {
              G4_Operand *src = src0addr ? src0 : src1;

              if (src->isAddrExp()) {
                // case:  arithmetic-op   A0 &GRF src1
                //
                // Add GRF to A0's points-to set.
                addToPointsToSet(ptr->asRegVar(), src->asAddrExp(), 0);
              } else {
                G4_VarBase *srcPtr = src->isSrcRegRegion()
                                         ? src->asSrcRegRegion()->getBase()
                                         : nullptr;
                vASSERT(srcPtr);
                // case:  arithmetic-op   A0 A1 src1
                // merge the two addr's points-to set together
                G4_RegVar *ptrVar = ptr->asRegVar();
                G4_RegVar *srcPtrVar = srcPtr->asRegVar();
                unsigned int ptrId = getIndexOfRegVar(ptrVar);
                unsigned int srcPtrId = getIndexOfRegVar(srcPtrVar);
                if (srcPtrId != ptrId) {
                  mergePointsToSet(srcPtrVar, ptrVar);
                }
              }
            } else if (ptr->isRegVar() && ptr->asRegVar()->isPhyRegAssigned()) {
              // OK, using builtin a0 or a0.2 directly.
            } else {
              // case:  arithmetic-op   A0 V1 V2
              //
              // Either both V1, V2 are pointers or neither are. So we're not
              // sure what A0 in dst may point to. As a conservative measure we
              // make A0 point to all variables that are address taken in the
              // program.
              VISA_DEBUG({
                std::cout << "unexpected addr add/mul for pointer analysis:\n";
                inst->emit(std::cout);
                std::cout << "\n";
              });
              for (int i = 0; i < (int)addrTakenVariables.size(); i++) {
                addToPointsToSet(ptr->asRegVar(), addrTakenVariables[i].first,
                                 0);
              }
            }
          } else {
            // case: A0 = ???
            //
            // This is a non-mov/non-arithmetic instruction with A0 as dst. We
            // don't recognize the pattern so don't know how to transfer
            // points-to to A0. So we fall back to being conservative and put
            // all address takens in program in to A0's points-to set.
            VISA_DEBUG({
              std::cout << "unexpected instruction with address destination:\n";
              inst->emit(std::cout);
              std::cout << "\n";
            });
            for (int i = 0; i < (int)addrTakenVariables.size(); i++) {
              addToPointsToSet(ptr->asRegVar(), addrTakenVariables[i].first,
                               addrTakenVariables[i].second);
            }
          }
        } else if (ptr->isRegVar() &&
                   !ptr->asRegVar()->getDeclare()->isMsgDesc()) {
          // We're at a generic instruction where dst is not an address
          // register. It's possible this is an intermediate operation where src
          // contains some address. For eg,
          //
          // TMP_A0 = &V1
          // SPILL_A0 (GRF temp) = TMP_A0 <-- SPILL_A0 is a general register but
          //                                    contains an address
          // TMP_A1 = SPILL_A0
          // TMP = r[TMP_A1]
          //
          // In above snippet, SPILL_A0 is a general register. Source of op is
          // address variable TMP_A0 that has non-empty points-to set. So we
          // copy over points-to of TMP_A0 in to SPILL_A0. Later, when we
          // encounter definition of TMP_A1, we again need to transfer
          // points-to of SPILL_A0 to it so that we know which variables are
          // accessed by indirect operand r[TMP_A1].
          for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
            G4_Operand *src = inst->getSrc(i);
            G4_VarBase *srcPtr = (src && src->isSrcRegRegion())
                                     ? src->asSrcRegRegion()->getBase()
                                     : nullptr;
            // clang-format off
            // We don't support using "r[a0.0]" as address expression.
            // For instructions like following, it's not point-to propagation
            // for simdShuffle and add64_i_i_i_i.
            // (W) mov        (1) simdShuffle(0,0)<1>:d  r[A0(0,0), 0]<0;1,0>:d
            //     pseudo_mad (16) add64_i_i_i_i(0,0)<1>:d  0x6:w  simdShuffle(0,0)<0;0>:d  rem_i_i_i_i(0,0)<1;0>:d
            //     shl        (16) add64_i_i_i_i(0,0)<1>:d add64_i_i_i_i(0,0)<1;1,0>:d  0x2:w
            // clang-format on
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

  // From the perspective of this analysis pass, if the pass cannot link an
  // address var to any address taken, we have to assume the var link to all
  // address takens to be safe. The approach is conservative, so other
  // optimization passes that deal with address variables can assist the
  // analysis to produce a more accurate result. For example, currently address
  // RA might clean up "redundant" ARF fill code without replacing the
  // corresponding uses with the correct values making this analysis pass leave
  // address var unresolved.
  fixupUnresolvedAddrVars();

  VISA_DEBUG_VERBOSE({
    std::cout << "Results of points-to analysis:\n";
    for (unsigned int i = 0; i < numAddrs; i++) {
      std::cout << "Addr " << i;
      for (auto &pointsTo : pointsToSets[addrPointsToSetIndex[i]]) {
        pointsTo.var->emit(std::cout);
        std::cout << "\t";
      }
      std::cout << "\n";
    }
  });

  // mark GRF that may be used indirect access as live in the block
  // This includes all GRFs in the address's points-to set
  for (auto bb : fg) {
    for (G4_INST *inst : *bb) {
      G4_DstRegRegion *dst = inst->getDst();

      if (dst != NULL && dst->getRegAccess() == IndirGRF) {
        G4_VarBase *dstptr = dst->getBase();
        vISA_ASSERT(
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
          vISA_ASSERT(
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
      vISA_ASSERT((indirectVarSize <=
                   fg.builder->getGRFSize() * fg.getKernel()->getNumRegTotal()),
                  "indirected variables' size is larger than GRF file size");
    }
  }
#endif

  VISA_DEBUG_VERBOSE({
    for (unsigned int i = 0; i < numBBs; i++) {
      std::cout << "Indirect uses for BB" << i << "\t";
      const REGVAR_VECTOR &grfVec = getIndrUseVectorForBB(i);
      for (auto &PI : grfVec) {
        PI.var->emit(std::cout);
        std::cout << "\t";
      }
      std::cout << "\n";
    }
  });
}

void PointsToAnalysis::insertAndMergeFilledAddr(const G4_RegVar *addr1,
                                              G4_RegVar *a2) {
  G4_RegVar *addr2 = getRootRegVar(a2);
  auto regVarsSize = (unsigned)regVars.size();
  vISA_ASSERT(
      regVarsSize == numAddrs,
      "Inconsistency found between size of regvars and number of addr vars");

  resizePointsToSet(numAddrs + 1);
  // add2 is the new one
  regVars.emplace(addr2, regVarsSize);

  mergePointsToSet(addr1, addr2);
}

const REGVAR_VECTOR *
PointsToAnalysis::getAllInPointsTo(const G4_RegVar *addr) const{
  vISA_ASSERT(addr->getDeclare()->getRegFile() == G4_ADDRESS ||
                   addr->getDeclare()->getRegFile() == G4_SCALAR,
               "expect address variable");
  unsigned int id = getIndexOfRegVar(addr);

  if (id == UINT_MAX)
    return nullptr;

  const REGVAR_VECTOR *vec = &pointsToSets[addrPointsToSetIndex[id]];

  return vec;
}

ADDREXP_VECTOR *PointsToAnalysis::getAllInPointsToAddrExps(G4_RegVar *addr) {
  vISA_ASSERT(addr->getDeclare()->getRegFile() == G4_ADDRESS ||
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
  vISA_ASSERT(addr->getDeclare()->getRegFile() == G4_ADDRESS ||
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
  vISA_ASSERT(addr->getDeclare()->getRegFile() == G4_ADDRESS ||
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
  vISA_ASSERT(addr->getDeclare()->getRegFile() == G4_ADDRESS ||
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
  vISA_ASSERT(addr->getDeclare()->getRegFile() == G4_ADDRESS ||
                   addr->getDeclare()->getRegFile() == G4_SCALAR,
               "expect address variable");
  unsigned int id = getIndexOfRegVar(addr);

  if (id == UINT_MAX) {
    vISA_ASSERT(false, "Could not find addr in points to set");
  }

  REGVAR_VECTOR &vec = pointsToSets[addrPointsToSetIndex[id]];
  pointInfo pt = {newvar, 0};
  vec.push_back(pt);

  addIndirectUseToBB(bbid, pt);
}

void PointsToAnalysis::patchPointsToSet(const G4_RegVar *addr, G4_AddrExp *opnd,
                      unsigned char offset) {
  vISA_ASSERT(addr->getDeclare()->getRegFile() == G4_ADDRESS,
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
    DEBUG_VERBOSE("Addr " <<id << " <-- "
                          << pi.var->getDeclare()->getName() << "\n");
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
  vISA_ASSERT(addr->getDeclare()->getRegFile() == G4_ADDRESS ||
                   addr->getDeclare()->getRegFile() == G4_SCALAR,
               "expect address variable");
  unsigned int id = getIndexOfRegVar(addr);

  if (id == UINT_MAX) {
    vISA_ASSERT(false, "Could not find addr in points to set");
  }

  REGVAR_VECTOR &vec = pointsToSets[addrPointsToSetIndex[id]];
  [[maybe_unused]] bool removed = false;

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

  vISA_ASSERT(removed == true, "Could not find spilled ref from points to");

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

void PointsToAnalysis::fixupUnresolvedAddrVars() {
  // Group resolved and unresolved address variables separately.
  std::vector<const G4_RegVar *> resolvedAddrs, unresolvedAddrs;
  for (auto &rv : regVars) {
    const G4_RegVar *var = rv.first;
    unsigned addrPTId = addrPointsToSetIndex[getIndexOfRegVar(var)];
    if (!pointsToSets[addrPTId].empty())
      resolvedAddrs.push_back(var);
    else
      unresolvedAddrs.push_back(var);
  }

  if (unresolvedAddrs.empty())
    return;

  // Copy all address takens to the pointsToSets of the 1st unresolved.
  const G4_RegVar *first = unresolvedAddrs.front();
  for (const G4_RegVar *var : resolvedAddrs) {
    unsigned addrPTId = addrPointsToSetIndex[getIndexOfRegVar(var)];
    for (const auto& addrInfo : addrExpSets[addrPTId])
      addToPointsToSet(first, addrInfo.exp, addrInfo.off);
  }
  // Update the points-to-set indexes of all remaining unresolved address
  // variables to the index of 1st unresolved variable.
  unsigned firstAddrPTId = addrPointsToSetIndex[getIndexOfRegVar(first)];;
  for (auto it = unresolvedAddrs.begin() + 1, ie = unresolvedAddrs.end();
       it != ie; ++it) {
    const G4_RegVar *var = *it;
    addrPointsToSetIndex[getIndexOfRegVar(var)] = firstAddrPTId;
  }
}
