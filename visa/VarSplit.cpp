/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "VarSplit.h"
#include "Assertions.h"
#include "GraphColor.h"

using namespace vISA;

namespace vISA {

LoopVarSplit::LoopVarSplit(G4_Kernel &k, GraphColor *c,
                           const LivenessAnalysis *liveAnalysis)
    : kernel(k), coloring(c), references(k) {
  for (auto spill : coloring->getSpilledLiveRanges()) {
    spilledDclSet.insert(spill->getDcl());
  }
  auto numVars = coloring->getNumVars();
  auto lrs = coloring->getLiveRanges();
  for (unsigned int i = 0; i != numVars; ++i) {
    auto rootDcl = lrs[i]->getDcl();
    dclSpillCost[rootDcl] = lrs[i]->getSpillCost();
  }
  DECLARE_LIST spills;
  std::for_each(coloring->getSpilledLiveRanges().begin(),
                coloring->getSpilledLiveRanges().end(),
                [&spills](LiveRange *lr) { spills.push_back(lr->getDcl()); });
  rpe = new RPE(c->getGRA(), liveAnalysis, &spills);
  rpe->run();
}

void LoopVarSplit::run() {
  // 1. consider list of spilled variables sorted in
  //       descending order of spill cost
  // 2. iterate over each spilled variable from sorted list
  //    a. run cost heuristic to decide if split makes sense
  //    b. if decision is to split, then check which loop(s) to split around

  std::list<std::pair<G4_Declare *, float>> sortedDcls;
  for (const auto &item : dclSpillCost) {
    if (spilledDclSet.find(item.first) != spilledDclSet.end())
      sortedDcls.push_back(std::pair(item.first, item.second));
  }

  auto SpillCostDesc = [](const std::pair<G4_Declare *, float> &first,
                          const std::pair<G4_Declare *, float> &second) {
    return first.second > second.second;
  };

  sortedDcls.sort(SpillCostDesc);

  // TODO: Iterating sequence based on spill cost may not be very accurate.
  // Thats because long living variables typically have lower spill cost.
  // But they may be referenced in some nested loop multiple times. In such
  // cases, it is beneficial to split the variable around such loops. To
  // accommodate such cases, we should be looking at reference count of
  // variables in loops and iterate in an order based on that.
  for (const auto &var : sortedDcls) {
    const auto &loops = getLoopsToSplitAround(var.first);
    for (auto &loop : loops) {
      split(var.first, *loop);
    }
  }
}

std::vector<G4_SrcRegRegion *> LoopVarSplit::getReads(G4_Declare *dcl,
                                                      Loop &loop) {
  std::vector<G4_SrcRegRegion *> reads;
  std::unordered_set<G4_INST *> duplicates;

  const auto &uses = references.getUses(dcl);
  for (const auto &use : *uses) {
    auto bb = std::get<1>(use);
    if (loop.contains(bb)) {
      auto inst = std::get<0>(use);
      if (duplicates.find(inst) != duplicates.end())
        continue;
      duplicates.insert(inst);
      for (unsigned int i = 0; i != inst->getNumSrc(); ++i) {
        auto opnd = inst->getSrc(i);
        if (!opnd || !opnd->isSrcRegRegion())
          continue;
        auto topdcl = opnd->asSrcRegRegion()->getTopDcl();
        if (topdcl == dcl)
          reads.push_back(opnd->asSrcRegRegion());
      }
    }
  }

  return reads;
}

std::vector<G4_DstRegRegion *> LoopVarSplit::getWrites(G4_Declare *dcl,
                                                       Loop &loop) {
  std::vector<G4_DstRegRegion *> writes;

  const auto &defs = references.getDefs(dcl);
  for (const auto &def : *defs) {
    auto bb = std::get<1>(def);
    if (loop.contains(bb)) {
      auto dst = std::get<0>(def)->getDst();
      writes.push_back(dst);
    }
  }

  return writes;
}

unsigned int LoopVarSplit::getMaxRegPressureInLoop(Loop &loop) {
  auto it = maxRegPressureCache.find(&loop);
  if (it != maxRegPressureCache.end())
    return it->second;

  unsigned int maxRPE = 0;
  auto &bbs = loop.getBBs();

  for (auto bb : bbs) {
    for (auto inst : bb->getInstList()) {
      maxRPE = std::max(maxRPE, rpe->getRegisterPressure(inst));
    }
  }

  maxRegPressureCache[&loop] = maxRPE;

  return maxRPE;
}

void LoopVarSplit::dump(std::ostream &of) {
  for (const auto &dcl : splitResults) {
    of << "Spilled dcl: " << dcl.first->getName() << "\n";
    for (const auto &split : dcl.second) {
      of << "\tSplit dcl: " << split.first->getName() << ", for loop L"
         << split.second->id << "\n";
    }
    of << "\n";
  }
}

void LoopVarSplit::removeAllSplitInsts(GlobalRA *gra, G4_Declare *dcl) {
  // dcl is a spilled split var. remove all split code
  // for dcl from pre-header and loop exits.
  // this method is invoked when a split dcl is spilled.
  vISA_ASSERT(gra->splitResults.find(dcl) != gra->splitResults.end(),
              "didnt find split result");
  auto &bbs = gra->splitResults[dcl].insts;
  for (auto &bb : bbs) {
    bb.first->getInstList().remove_if([&](G4_INST *candidate) {
      auto it = bb.second.find(candidate);
      if (it != bb.second.end()) {
        auto dst = candidate->getDst();
        if (dst && dst->getTopDcl())
          gra->incRA.markForIntfUpdate(dst->getTopDcl());

        for (unsigned int i = 0; i != candidate->getNumSrc(); ++i) {
          auto src = candidate->getSrc(i);
          if (src && src->getTopDcl())
            gra->incRA.markForIntfUpdate(src->getTopDcl());
        }
        return true;
      }
      return false;
    });
  }
}

// This method is invoked to remove spilled split variables from
// the program.
//
// Example before split:
//
// V10 = ...
//
// loop1:
// ...
//     = V10
//
// Assume V10 is spilled and split around loop1. Following code
// would be generated:
//
// SP_V10 =
// spill SP_V10 @ Offset 0
//
// preHeader_loop1:
// fill FL_V10 @ Offset 0
// LOOP_SPLIT_V10 = FL_V10
//
// loop1:
// ...
//     = LOOP_SPLIT_V10
//
// Now, assume LOOP_SPLIT_V10 spills in later RA iteration.
// This makes copy to LOOP_SPLIT_V10 in preHeader_loop1 redundant.
// So we eliminate the fill and copy from preHeader_loop1.
// Similarly, such copies may also be present in loop exit BB that
// require removal. This method eliminates such spilled split
// variables from the program.
void LoopVarSplit::removeSplitInsts(GlobalRA *gra, G4_Declare *spillDcl,
                                    G4_BB *bb) {
  auto it = gra->splitResults.find(spillDcl);

  if (it == gra->splitResults.end())
    return;

  auto &bbInstsToRemove = (*it).second.insts;
  for (const auto &entry : bbInstsToRemove) {
    auto currBB = entry.first;

    if (currBB == bb) {
      auto &instsToRemoveFromBB = entry.second;
      for (auto instIt = bb->begin(); instIt != bb->end();) {
        auto inst = (*instIt);
        if (instsToRemoveFromBB.find(inst) == instsToRemoveFromBB.end()) {
          ++instIt;
          continue;
        }

        instIt = bb->erase(instIt);
      }
    }
  }
}

bool LoopVarSplit::removeFromPreheader(GlobalRA *gra, G4_Declare *spillDcl,
                                       G4_BB *bb,
                                       INST_LIST_ITER filledInstIter) {
  auto it = gra->splitResults.find(spillDcl);
  if (it != gra->splitResults.end() &&
      (*it).second.insts.find(bb) != (*it).second.insts.end()) {
    auto inst = *filledInstIter;
    if (inst->isRawMov()) {
      removeSplitInsts(gra, spillDcl, bb);
      return true;
    }
  }
  return false;
}

bool LoopVarSplit::removeFromLoopExit(GlobalRA *gra, G4_Declare *spillDcl,
                                      G4_BB *bb,
                                      INST_LIST_ITER filledInstIter) {
  return removeFromPreheader(gra, spillDcl, bb, filledInstIter);
}

const std::unordered_set<G4_INST *> LoopVarSplit::getSplitInsts(GlobalRA *gra,
                                                                G4_BB *bb) {
  std::unordered_set<G4_INST *> ret;

  for (auto &splitVar : gra->splitResults)
    for (auto &insts : splitVar.second.insts)
      if (insts.first == bb) {
        for (auto item : insts.second)
          ret.insert(item);
      }

  return ret;
}

bool LoopVarSplit::split(G4_Declare *dcl, Loop &loop) {
  // Split dcl in given loop. Return true if split was successful.
  // It is assumed that dcl is spilled. This method inserts a
  // copy in preheader of loop. Dst of this copy is a new tmp and
  // source is dcl. All uses/defs of dcl in the loop are replaced
  // with the tmp. If dcl is ever written in the loop, a copy from
  // tmp to dcl is inserted in loop exit bb.
  //
  // This transformation requires a valid preheader be present for
  // loop. If dcl is written in the loop then a valid exit bb is also
  // needed.
  if (!loop.preHeader)
    return false;

  const auto &dsts = getWrites(dcl, loop);
  if (dsts.size() > 0) {
    // evaluate if it makes sense to insert the copy if loop has
    // multiple exits.
    if (loop.getLoopExits().size() != 1)
      return false;
    // TODO: Handle creation of loop preheader and loop exit in
    // presence of SIMD CF. this requires changing JIP, UIP
    // in source goto instructions and fix (if) any data structures
    // in VISA that rely on those JIP/UIP.
    if (!loop.preHeader->dominates(loop.getLoopExits().front()))
      return false;
  }

  // At this point we've decided to split dcl around loop
  coloring->getGRA().incRA.markForIntfUpdate(dcl);

  const auto &srcs = getReads(dcl, loop);

  auto splitDcl = kernel.fg.builder->createTempVar(
      dcl->getTotalElems(), dcl->getElemType(),
      coloring->getGRA().getSubRegAlign(dcl), "LOOPSPLIT", true);

  auto &splitData = coloring->getGRA().splitResults[splitDcl];
  splitData.origDcl = dcl;
  bool isDefault32bMask = coloring->getGRA().getAugmentationMask(dcl) ==
                          AugmentationMasks::Default32Bit;
  bool isDefault64bMask = coloring->getGRA().getAugmentationMask(dcl) ==
                          AugmentationMasks::Default64Bit;

  // emit TMP = dcl in preheader
  copy(loop.preHeader, splitDcl, dcl, &splitData, isDefault32bMask, isDefault64bMask);

  // emit dcl = TMP in loop exit
  if (dsts.size() > 0) {
    copy(loop.getLoopExits().front(), dcl, splitDcl, &splitData,
         isDefault32bMask, isDefault64bMask, /*pushBack*/ false);
  }

  // replace all occurences of dcl in loop with TMP
  for (auto src : srcs)
    replaceSrc(src, splitDcl, loop);

  for (auto dst : dsts)
    replaceDst(dst, splitDcl, loop);

  splitResults[dcl].push_back(std::make_pair(splitDcl, &loop));

  return true;
}

void LoopVarSplit::copy(G4_BB *bb, G4_Declare *dst, G4_Declare *src,
                        SplitResults *splitData, bool isDefault32bMask,
                        bool isDefault64bMask, bool pushBack) {
  // create mov instruction to copy dst->src
  // multiple mov instructions may be created depending on size of dcls
  // all mov instructions are appended to inst list of bb
  // when pushBack argument = true, append to BB (happens in pre-header)
  // when pushBack argument = false, insert in bb after label (happens at exit
  // bb)

  dst = dst->getRootDeclare();
  src = src->getRootDeclare();
  unsigned int numRows = dst->getNumRows();
  unsigned int bytesRemaining = dst->getByteSize();
  [[maybe_unused]] const unsigned int maxDstSize = 2;

  auto insertCopy = [&](G4_INST *inst) {
    if (pushBack || bb->size() == 0) {
      bb->push_back(inst);
      splitData->insts[bb].insert(inst);
    } else {
      if (bb->front()->isLabel()) {
        auto insertAfterIt = bb->begin();
        ++insertAfterIt;
        if (insertAfterIt != bb->end() &&
            (*insertAfterIt)->opcode() == G4_join) {
          bb->insertAfter(insertAfterIt, inst);
        } else
          bb->insertAfter(bb->begin(), inst);
      } else {
        vISA_ASSERT(bb->size() == 0 || bb->front()->opcode() != G4_join,
                    "shouldnt insert copy before join");
        bb->push_front(inst);
      }

      splitData->insts[bb].insert(inst);
    }
    if (inst->isWriteEnableInst() &&
        coloring->getGRA().EUFusionNoMaskWANeeded()) {
      coloring->getGRA().addEUFusionNoMaskWAInst(bb, inst);
    }
  };

  // if variable fits within 1 or 2 GRFs and uses Default32Bit augmentation mask
  // then make the copy use M0 mask instead of using WriteEnable.
  // TODO: Copy should use same EM as original variable.
  unsigned int instOption = InstOpt_WriteEnable;
  if (isDefault32bMask) {
    if (bytesRemaining % kernel.numEltPerGRF<Type_UD>() == 0 &&
        bytesRemaining <= kernel.numEltPerGRF<Type_UB>() * 2) {
      instOption = G4_InstOption::InstOpt_M0;
    }
  }

  // first copy full GRF rows
  if (numRows > 1 ||
      (dst->getTotalElems() * dst->getElemSize() == kernel.getGRFSize())) {
    // dcls are GRF sized so emit max SIMD size possible and copy 2 rows at
    // a time
    for (unsigned int i = 0; i < numRows;) {
      const RegionDesc *rd = kernel.fg.builder->getRegionStride1();
      G4_ExecSize execSize{kernel.numEltPerGRF<Type_UD>()};

      unsigned int rowsCopied = 1;
      G4_Type movType = Type_F;
      if (bytesRemaining >= kernel.numEltPerGRF<Type_UB>() * 2) {
        // copy 2 GRFs at a time if byte size permits
        if (instOption == InstOpt_WriteEnable ||
            kernel.getSimdSize() >= execSize * 2) {
          // When instruction uses default EM, max # of rows to be
          // written per mov instruction is restricted by kernel's
          // simd size. For eg, assume a variable with 2 rows
          // belongs to Default32Bit augmentation bucket. It means
          // each row is defined using Default32Bit EM. So on 32-byte
          // GRF platform under SIMD8, we should emit 2 movs, each
          // writing 1 row and both movs must use default EM. Emitting
          // single SIMD16 mov with default EM is illegal is this case.
          rowsCopied = 2;
          movType = Type_F;
        }
      }
      execSize = G4_ExecSize(kernel.numEltPerGRF(movType) * rowsCopied);

      auto dstRgn = kernel.fg.builder->createDst(dst->getRegVar(), (short)i, 0,
                                                 1, movType);
      auto srcRgn = kernel.fg.builder->createSrc(src->getRegVar(), (short)i, 0,
                                                 rd, movType);
      auto inst = kernel.fg.builder->createMov(execSize, dstRgn, srcRgn,
                                               instOption, false);

      insertCopy(inst);
      vISA_ASSERT(
          bytesRemaining >=
              (unsigned int)(execSize.value * G4_Type_Table[movType].byteSize),
          "Invalid copy exec size");
      bytesRemaining -= (execSize.value * G4_Type_Table[movType].byteSize);

      i += rowsCopied;

      if (bytesRemaining < kernel.numEltPerGRF<Type_UB>())
        break;
    }
  }

  if (kernel.getOption(vISA_FillConstOpt) && bytesRemaining > 0) {
    // if src is a const def then emit mov with imm src
    auto defs = references.getDefs(src);
    if (defs && defs->size() == 1) {
      auto onlyDef = std::get<0>(defs->front());
      if (immFillCandidate(onlyDef)) {
        auto immSrc = onlyDef->getSrc(0)->asImm();
        auto dstRgn = kernel.fg.builder->createDst(
            dst->getRegVar(), 0, 0, 1, onlyDef->getDst()->getType());
        auto inst = kernel.fg.builder->createMov(g4::SIMD1, dstRgn, immSrc,
                                                 instOption, false);

        insertCopy(inst);
        bytesRemaining = 0;
      }
    }
  }

  while (bytesRemaining > 0) {
    G4_Type type = Type_W;
    G4_ExecSize execSize = g4::SIMD16;

    if (bytesRemaining >= 16)
      execSize = g4::SIMD8;
    else if (bytesRemaining >= 8 && bytesRemaining < 16)
      execSize = g4::SIMD4;
    else if (bytesRemaining >= 4 && bytesRemaining < 8)
      execSize = g4::SIMD2;
    else if (bytesRemaining >= 2 && bytesRemaining < 4)
      execSize = g4::SIMD1;
    else if (bytesRemaining == 1) {
      // If a region has odd number of bytes, copy last byte in final iteration
      execSize = g4::SIMD1;
      type = Type_UB;
    } else {
      vISA_ASSERT(false, "Unexpected condition");
    }

    const RegionDesc *rd = kernel.fg.builder->getRegionStride1();
    if (execSize == g4::SIMD1)
      rd = kernel.fg.builder->getRegionScalar();

    unsigned int row =
        (dst->getByteSize() - bytesRemaining) / kernel.numEltPerGRF<Type_UB>();
    unsigned int col =
        (dst->getByteSize() - bytesRemaining) % kernel.numEltPerGRF<Type_UB>();
    if (G4_Type_Table[type].byteSize > 1) {
      vISA_ASSERT(col % 2 == 0, "Unexpected condition");
      col /= 2;
    }

    G4_DstRegRegion *dstRgn =
        kernel.fg.builder->createDst(dst->getRegVar(), row, col, 1, type);
    G4_SrcRegRegion *srcRgn =
        kernel.fg.builder->createSrc(src->getRegVar(), row, col, rd, type);

    vISA_ASSERT(instOption == InstOpt_WriteEnable, "Unexpected inst option");

    auto inst = kernel.fg.builder->createMov(execSize, dstRgn, srcRgn,
                                             instOption, false);

    insertCopy(inst);

    vISA_ASSERT(bytesRemaining >= (execSize.value *
                                   (unsigned int)G4_Type_Table[type].byteSize),
                "Invalid copy exec size");
    bytesRemaining -= (execSize.value * G4_Type_Table[type].byteSize);
  };
}

void LoopVarSplit::replaceSrc(G4_SrcRegRegion *src, G4_Declare *dcl,
                              const Loop &loop) {
  auto srcDcl = src->getBase()->asRegVar()->getDeclare();
  dcl = getNewDcl(srcDcl, dcl, loop);

  auto newSrcRgn = kernel.fg.builder->createSrc(
      dcl->getRegVar(), src->getRegOff(), src->getSubRegOff(), src->getRegion(),
      src->getType(), src->getAccRegSel());
  newSrcRgn->setModifier(src->getModifier());
  auto inst = src->getInst();
  for (unsigned int i = 0; i != inst->getNumSrc(); ++i) {
    if (inst->getSrc(i) == src) {
      inst->setSrc(newSrcRgn, i);
      break;
    }
  }
}

void LoopVarSplit::replaceDst(G4_DstRegRegion *dst, G4_Declare *dcl,
                              const Loop &loop) {
  auto dstDcl = dst->getBase()->asRegVar()->getDeclare();
  dcl = getNewDcl(dstDcl, dcl, loop);

  auto newDstRgn = kernel.fg.builder->createDst(
      dcl->getRegVar(), dst->getRegOff(), dst->getSubRegOff(),
      dst->getHorzStride(), dst->getType(), dst->getAccRegSel());

  auto inst = dst->getInst();
  inst->setDest(newDstRgn);
}

G4_Declare *LoopVarSplit::getNewDcl(G4_Declare *dcl1, G4_Declare *dcl2,
                                    const Loop &loop) {
  // this method gets args dcl1, dcl2. this method is invoked
  // when the transformation replaces existing src/dst rgn with
  // equivalent one but using split variable. for eg,
  //
  // op ... V10(0,5) ... <-- assume V10 is alias of V9
  //
  // assume V9 gets split so V10 src rgn above has to be replaced.
  // say V9's split dcl is called LOOP_SPLIT_V9.
  // so in this function we create a new dcl, LOOP_SPLIT_V10 that
  // aliases LOOP_SPLIT_V9 exactly like V10 aliases V9. this
  // way we dont need any complicated logic to flatten V10.
  //
  // dcl1 is a dcl used to construct some src or dst rgn.
  // dcl2 is a new dcl that splits dcl1. dcl2 is always root dcl.
  // dcl1 may or may not be alias of another dcl.
  // if dcl1 is also root dcl, then return dcl2.
  // if dcl1 is an alias dcl, then construct new dcl that aliases
  //   dcl2 at similar offset.
  // mapping from old dcl to new dcl is stored for future invocations.
  // this mapping is done per loop as a single spilled variable could
  // be split in multiple loops and each split instance would use a
  // different loop split variable.

  vISA_ASSERT(!dcl2->getAliasDeclare(), "Expecting to see root dcl for dcl2");

  auto &oldNewDcl = oldNewDclPerLoop[&loop];

  auto it = oldNewDcl.find(dcl1);
  if (it != oldNewDcl.end())
    return (*it).second;

  if (!dcl1->getAliasDeclare()) {
    oldNewDcl[dcl1] = dcl2;
    return dcl2;
  }

  auto newDcl = kernel.fg.builder->createTempVar(
      dcl1->getTotalElems(), dcl1->getElemType(), dcl1->getSubRegAlign());
  newDcl->setAliasDeclare(getNewDcl(dcl1->getRootDeclare(), dcl2, loop),
                          dcl1->getOffsetFromBase());

  oldNewDcl[dcl1] = newDcl;

  return newDcl;
}

std::vector<Loop *> LoopVarSplit::getLoopsToSplitAround(G4_Declare *dcl) {
  // return a list of Loop* around which variable dcl should be split
  std::vector<Loop *> loopsToSplitAround;

  if (dcl->getAddressed())
    return loopsToSplitAround;

  // first make list of all loops where dcl is ever referenced
  auto uses = references.getUses(dcl);
  auto defs = references.getDefs(dcl);

  auto StableOrder = [](const G4_BB *first, const G4_BB *second) {
    return first->getId() < second->getId();
  };
  std::set<G4_BB *, decltype(StableOrder)> bbsWithRefToDcl(StableOrder);

  if (uses) {
    for (auto &use : *uses) {
      auto bb = std::get<1>(use);
      bbsWithRefToDcl.insert(bb);
    }
  }

  if (defs) {
    for (auto &def : *defs) {
      auto bb = std::get<1>(def);
      bbsWithRefToDcl.insert(bb);
    }
  }

  auto OrderByRegPressure = [&](Loop *loop1, Loop *loop2) {
    auto maxRP1 = getMaxRegPressureInLoop(*loop1);
    auto maxRP2 = getMaxRegPressureInLoop(*loop2);
    if (maxRP1 != maxRP2)
      return maxRP1 > maxRP2;
    if (loop1->getNestingLevel() != loop2->getNestingLevel())
      return loop1->getNestingLevel() > loop2->getNestingLevel();
    return loop1->id > loop2->id;
  };

  std::set<Loop *, decltype(OrderByRegPressure)> innerMostLoops(
      OrderByRegPressure);

  // now collect innermost loop for each referenced BB
  for (auto bb : bbsWithRefToDcl) {
    auto innerMost = kernel.fg.getLoops().getInnerMostLoop(bb);
    if (innerMost)
      innerMostLoops.insert(innerMost);
  }

  // prune list of loops
  // 1. loops are stored in descending order of max reg pressure
  // 2. apply cost heuristic to decide if variable should be split at a loop
  // 3. once split at a loop, don't split at any parent or nested loop
  //
  // Example:
  //
  // -----
  // |
  // | Loop A
  // | =X
  // -----
  //
  // -----
  // |
  // | Loop B
  // | =X
  // | -----
  // | | =X
  // | | Loop C
  // | -----
  // |
  // -----
  //
  // Assume variable X is spilled and is referenced in Loop A, Loop B, Loop C.
  // Loop C is nested in Loop B.
  // The algorithm below decides whether it is better to spill X around Loop C
  // or Loop B. X is spilled around only 1 of these 2 loops since they've a
  // parent-nested relationship. Independently, the algorithm can also decide to
  // split X around Loop A as it is not a parent or nested loop of other loops.
  //

  for (auto loop : innerMostLoops) {
    // cannot split without pre-header
    if (!loop->preHeader)
      continue;

    // unsafe to split if loop has subroutine calls
    if (loop->subCalls)
      continue;

    bool dontSplit = false;
    for (auto splitLoop : loopsToSplitAround) {
      if (loop->fullSubset(splitLoop) || loop->fullSuperset(splitLoop)) {
        // variable already split in nested or parent loop
        dontSplit = true;
        break;
      }
    }

    if (dontSplit)
      continue;

    bool forceSplit = kernel.getOption(vISA_ForceSplitOnSpill);
    if (forceSplit) {
      loopsToSplitAround.push_back(loop);
      return loopsToSplitAround;
    }

    auto subRegAlign = coloring->getGRA().getSubRegAlign(dcl);

    // apply cost heuristic
    if (dcl->getNumElems() == 1 &&
        subRegAlign != coloring->getGRA().kernel.fg.builder->getGRFAlign()) {
      // unaligned scalars can be packed so dont adjust loop pressure for each
      // unaligned scalar. we may not be able to trivially pack aligned scalars
      // so use other branch to handle them.
      if (getMaxRegPressureInLoop(*loop) <
          (unsigned int)(1.0f * (float)kernel.getNumRegTotal() - 3.0f)) {
        auto scalarBytes = scalarBytesSplit[loop];
        if (scalarBytes >= kernel.numEltPerGRF<Type_UB>()) {
          // After packing, scalars contribute to 1 full GRF worth of storage.
          // Adjust loop max pressure accordingly.
          adjustLoopMaxPressure(*loop, 1);
          scalarBytesSplit[loop] -= kernel.numEltPerGRF<Type_UB>();
        }
        // scalar
        loopsToSplitAround.push_back(loop);
        scalarBytesSplit[loop] += dcl->getByteSize();
      }
    } else if (dcl->getNumRows() <= 2) {
      float Coeff = 0.95f;
      // if loop is large and has close to high RPE, reduce coeff to be less
      // aggressive
      if (isLargeLoop(*loop) &&
          getMaxRegPressureInLoop(*loop) >= 0.98f * (float)rpe->getMaxRP())
        Coeff = 0.9f;
      if (getMaxRegPressureInLoop(*loop) <=
          (unsigned int)(Coeff * (float)kernel.getNumRegTotal())) {
        loopsToSplitAround.push_back(loop);
        if (dcl->getNumElems() > 1 ||
            subRegAlign == coloring->getGRA().kernel.fg.builder->getGRFAlign())
          adjustLoopMaxPressure(*loop, dcl->getNumRows());
      }
    } else if (dcl->getNumRows() <= 4) {
      if (getMaxRegPressureInLoop(*loop) <=
          (unsigned int)(0.9f * (float)kernel.getNumRegTotal())) {
        loopsToSplitAround.push_back(loop);
        adjustLoopMaxPressure(*loop, dcl->getNumRows());
      }
    } else if (dcl->getNumRows() > 4) {
      // splitting dcls with > 4 rows should be very rare
      if (getMaxRegPressureInLoop(*loop) <=
          (unsigned int)(0.75f * (float)kernel.getNumRegTotal())) {
        loopsToSplitAround.push_back(loop);
        adjustLoopMaxPressure(*loop, dcl->getNumRows());
      }
    }
  }

  return loopsToSplitAround;
}

// When a variable is split in a loop, it increases register pressure
// in the loop because the split variable is live throughout the loop.
// Adjust max reg pressure data structure accordingly as it is used in
// cost heuristic for split decision.
void LoopVarSplit::adjustLoopMaxPressure(Loop &loop, unsigned int numRows) {
  unsigned int newMaxRegPressure = getMaxRegPressureInLoop(loop) + numRows;
  maxRegPressureCache[&loop] = newMaxRegPressure;
  for (auto &nested : loop.immNested) {
    adjustLoopMaxPressure(*nested, numRows);
  }
  Loop *parent = loop.parent;
  while (parent) {
    if (getMaxRegPressureInLoop(*parent) < newMaxRegPressure)
      maxRegPressureCache[parent] = newMaxRegPressure;
    parent = parent->parent;
  }
}

}; // namespace vISA
