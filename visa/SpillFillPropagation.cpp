/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SpillFillPropagation.h"
#include "BuildIR.h"
#include "FlowGraph.h"

#include <unordered_set>

using namespace vISA;

SpillFillPropagation::SpillFillPropagation(G4_Kernel &k, IR_Builder &b,
                                           GlobalRA &g)
    : kernel(k), builder(b), gra(g),
      grfSize(k.numEltPerGRF<Type_UB>()) {}

// Reset both forward and reverse offset-to-GRF tracking maps.
void SpillFillPropagation::clearTable() {
  offsetToGRFs.clear();
  grfToOffsets.clear();
}

// Record that a physical GRF holds the value for a scratch offset.
// If clearOld is true, remove all previous GRF associations for the offset
// first (used when a spill overwrites the scratch content).
void SpillFillPropagation::addEntry(unsigned scratchOffset, unsigned grfNum,
                                    bool clearOld) {
  if (clearOld)
    invalidateOffset(scratchOffset);
  offsetToGRFs[scratchOffset].insert(grfNum);
  grfToOffsets[grfNum].insert(scratchOffset);
}

// Remove a physical GRF from all offset mappings (e.g. when it is overwritten).
void SpillFillPropagation::invalidateGRF(unsigned grfNum) {
  auto it = grfToOffsets.find(grfNum);
  if (it != grfToOffsets.end()) {
    for (unsigned off : it->second) {
      auto offIt = offsetToGRFs.find(off);
      if (offIt != offsetToGRFs.end()) {
        offIt->second.erase(grfNum);
        if (offIt->second.empty())
          offsetToGRFs.erase(offIt);
      }
    }
    grfToOffsets.erase(it);
  }
}

// Remove all physical GRFs mapping to given offset
void vISA::SpillFillPropagation::invalidateOffset(unsigned offset) {
  auto it = offsetToGRFs.find(offset);
  if (it != offsetToGRFs.end()) {
    for (unsigned grf : it->second) {
      auto grfIt = grfToOffsets.find(grf);
      if (grfIt != grfToOffsets.end()) {
        grfIt->second.erase(offset);
        if (grfIt->second.empty())
          grfToOffsets.erase(grfIt);
      }
    }
    offsetToGRFs.erase(it);
  }
}

// Return the inclusive range [startGRF, endGRF] of physical GRFs touched by
// the operand.
std::pair<unsigned, unsigned>
SpillFillPropagation::getGRFRange(G4_Operand *opnd) {
  G4_Declare *topDcl = opnd->getTopDcl();
  unsigned baseGRF =
      topDcl->getRegVar()->getPhyReg()->asGreg()->getRegNum();
  unsigned startGRF = baseGRF + opnd->getLeftBound() / grfSize;
  unsigned endGRF = baseGRF + opnd->getRightBound() / grfSize;
  return {startGRF, endGRF};
}

// Check whether every scratch row of the fill is tracked in the offset-to-GRF
// map. If so, populate srcGRFs with the source GRF for each row.
bool SpillFillPropagation::canReplaceFill(G4_FillIntrinsic *fill,
                                          std::vector<unsigned> &srcGRFs) {
  unsigned offset = fill->getOffset();
  unsigned numRows = fill->getNumRows();
  srcGRFs.clear();
  srcGRFs.reserve(numRows);

  for (unsigned i = 0; i < numRows; ++i) {
    auto it = offsetToGRFs.find(offset + i);
    if (it == offsetToGRFs.end() || it->second.empty())
      return false;
    // Pick the first available GRF for this row.
    srcGRFs.push_back(*it->second.begin());
  }
  return true;
}

// Check that source and destination GRF ranges don't overlap.
// Returns false if any source GRF falls within the destination range
// (and isn't already in place), meaning the replacement should be skipped.
static bool canSafelyReplace(const std::vector<unsigned> &srcGRFs,
                             unsigned dstStartGRF, unsigned numRows) {
  unsigned dstEndGRF = dstStartGRF + numRows - 1;
  for (unsigned i = 0; i < numRows; ++i) {
    if (srcGRFs[i] == dstStartGRF + i)
      continue; // already in place
    if (srcGRFs[i] >= dstStartGRF && srcGRFs[i] <= dstEndGRF)
      return false;
  }
  return true;
}

// Replace a fill intrinsic with GRF-to-GRF movs inserted before it (forward
// pass). Erases the fill and advances fillIt. Returns false if src/dst GRF
// ranges overlap unsafely.
bool SpillFillPropagation::replaceFillWithMovs(
    G4_BB *bb, INST_LIST_ITER &fillIt, G4_FillIntrinsic *fill,
    const std::vector<unsigned> &srcGRFs) {

  auto [dstStartGRF, dstEndGRF] = getGRFRange(fill->getDst());
  unsigned numRows = fill->getNumRows();

  if (!canSafelyReplace(srcGRFs, dstStartGRF, numRows))
    return false;

  G4_Type type = Type_UD;
  if (grfSize == 64)
    type = Type_UQ;
  G4_ExecSize singleRowExecSize(grfSize / TypeSize(type));

  G4_Declare *dstTopDcl = fill->getDst()->getTopDcl();
  short dstRegOff = fill->getDst()->getRegOff();

  for (unsigned i = 0; i < numRows;) {
    if (srcGRFs[i] == dstStartGRF + i) {
      ++i;
      continue;
    }

    bool canDoTwoRows =
        (i + 1 < numRows) && (srcGRFs[i + 1] != dstStartGRF + i + 1) &&
        (srcGRFs[i] + 1 == srcGRFs[i + 1]);

    unsigned rowsThisMov = canDoTwoRows ? 2 : 1;
    G4_ExecSize execSize =
        canDoTwoRows ? G4_ExecSize(singleRowExecSize * 2) : singleRowExecSize;

    G4_DstRegRegion *dst = builder.createDst(
        dstTopDcl->getRegVar(), (short)(dstRegOff + i), 0, 1, type);

    const char *name = builder.getNameString(
        32, "SFProp_%d", static_cast<int>(kernel.Declares.size()));
    G4_Declare *srcDcl = builder.createDeclare(
        name, G4_GRF, kernel.numEltPerGRF<Type_UD>(), rowsThisMov, Type_UD);
    srcDcl->getRegVar()->setPhyReg(builder.phyregpool.getGreg(srcGRFs[i]), 0);

    G4_SrcRegRegion *src = builder.createSrc(
        srcDcl->getRegVar(), 0, 0, builder.getRegionStride1(), type);

    G4_INST *mov =
        builder.createMov(execSize, dst, src, InstOpt_WriteEnable, false);
    mov->setVISAId(fill->getVISAId());

    bb->insertBefore(fillIt, mov);

    if (gra.EUFusionNoMaskWANeeded()) {
      gra.addEUFusionNoMaskWAInst(bb, mov);
    }

    i += rowsThisMov;
  }

  // Erase the fill intrinsic.
  fillIt = bb->erase(fillIt);
  return true;
}

// Replace a pending fill with GRF-to-GRF movs inserted after the source
// instruction pointed to by rit (backward pass). Erases the pending fill
// instruction and re-seats rit. Returns false if src/dst overlap unsafely.
bool SpillFillPropagation::replaceFillWithMovsAfter(
    G4_BB *bb, INST_LIST_RITER &rit, INST_LIST_ITER &insertAfterIt,
    const PendingFill &pf, const std::vector<unsigned> &srcGRFs) {

  if (!canSafelyReplace(srcGRFs, pf.dstStartGRF, pf.numRows))
    return false;

  INST_LIST_ITER sourceIt = std::next(rit).base();

  G4_Type type = Type_UD;
  if (grfSize == 64)
    type = Type_UQ;
  G4_ExecSize singleRowExecSize(grfSize / TypeSize(type));

  INST_LIST_ITER insertPos = std::next(insertAfterIt);
  INST_LIST_ITER lastInserted = insertAfterIt;

  for (unsigned i = 0; i < pf.numRows;) {
    if (srcGRFs[i] == pf.dstStartGRF + i) {
      ++i;
      continue;
    }

    bool canDoTwoRows =
        (i + 1 < pf.numRows) &&
        (srcGRFs[i + 1] != pf.dstStartGRF + i + 1) &&
        (srcGRFs[i] + 1 == srcGRFs[i + 1]);

    unsigned rowsThisMov = canDoTwoRows ? 2 : 1;
    G4_ExecSize execSize =
        canDoTwoRows ? G4_ExecSize(singleRowExecSize * 2) : singleRowExecSize;

    G4_DstRegRegion *dst = builder.createDst(
        pf.dstTopDcl->getRegVar(), (short)(pf.dstRegOff + i), 0, 1, type);

    const char *name = builder.getNameString(
        32, "SFProp_%d", static_cast<int>(kernel.Declares.size()));
    G4_Declare *srcDcl = builder.createDeclare(
        name, G4_GRF, kernel.numEltPerGRF<Type_UD>(), rowsThisMov, Type_UD);
    srcDcl->getRegVar()->setPhyReg(builder.phyregpool.getGreg(srcGRFs[i]), 0);

    G4_SrcRegRegion *src = builder.createSrc(
        srcDcl->getRegVar(), 0, 0, builder.getRegionStride1(), type);

    G4_INST *mov =
        builder.createMov(execSize, dst, src, InstOpt_WriteEnable, false);
    mov->setVISAId(pf.visaId);

    bb->insertBefore(insertPos, mov);
    lastInserted = std::prev(insertPos);

    if (gra.EUFusionNoMaskWANeeded()) {
      gra.addEUFusionNoMaskWAInst(bb, mov);
    }

    i += rowsThisMov;
  }


  // Erase the pending fill instruction.
  bb->erase(pf.instIt);

  // Re-seat the reverse iterator to the source instruction.
  // Insertions after sourceIt may have shifted what rit dereferences to.
  rit = INST_LIST_RITER(std::next(sourceIt));

  // Advance the caller's insertion anchor so the next match chains its mov(s)
  // after the ones just inserted instead of sharing the same anchor.
  insertAfterIt = lastInserted;
  return true;
}

bool SpillFillPropagation::hasAssignedGRF(G4_Declare *topdcl) const {
  if (!topdcl->useGRF())
    return false;
  auto *phyReg = topdcl->getRegVar()->getPhyReg();
  if (!phyReg)
    return false;
  if (!phyReg->isGreg())
    return false;
  return true;
}

// Invalidate offset-to-GRF entries for any GRF written by inst's destination.
void SpillFillPropagation::invalidateClobberedEntries(G4_INST *inst) {
  G4_DstRegRegion *dst = inst->getDst();
  if (!dst || dst->isNullReg())
    return;

  if (dst->getBase() && dst->getBase()->isRegVar() && dst->getTopDcl() &&
      hasAssignedGRF(dst->getTopDcl())) {
    auto [startGRF, endGRF] = getGRFRange(dst);
    for (unsigned g = startGRF; g <= endGRF; ++g)
      invalidateGRF(g);
  }
}

// Return true if any operand of inst uses indirect addressing.
static bool hasIndirectOperand(G4_INST *inst) {
  G4_DstRegRegion *dst = inst->getDst();
  if (dst && !dst->isNullReg() && dst->isIndirect())
    return true;
  for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
    G4_Operand *src = inst->getSrc(i);
    if (src && src->isSrcRegRegion() && src->asSrcRegRegion()->isIndirect())
      return true;
  }
  return false;
}

// Forward (top-down) pass: track which GRFs hold spilled data and replace
// fills with movs when the data is already available in a GRF.
void SpillFillPropagation::processBBForward(G4_BB *bb) {
  // Load carried-over state or start fresh.
  auto entryIt = bbEntryState.find(bb);
  if (entryIt != bbEntryState.end()) {
    offsetToGRFs = std::move(entryIt->second);
    // Rebuild the reverse map.
    grfToOffsets.clear();
    for (auto &[off, grfs] : offsetToGRFs)
      for (unsigned grf : grfs)
        grfToOffsets[grf].insert(off);
    bbEntryState.erase(entryIt);
  } else {
    clearTable();
  }

  for (auto it = bb->begin(), end = bb->end(); it != end;) {
    G4_INST *inst = *it;

    if (inst->isSpillIntrinsic()) {
      auto *spill = inst->asSpillIntrinsic();
      unsigned offset = spill->getOffset();
      unsigned numRows = spill->getNumRows();
      // Only track WriteEnable, non-scatter spills.
      if (!spill->isScatterSpill() && inst->isWriteEnableInst()) {
        auto [payloadStartGRF, payloadEndGRF] =
            getGRFRange(spill->getPayload());
        for (unsigned i = 0; i < numRows; ++i)
          addEntry(offset + i, payloadStartGRF + i, /*clearOld=*/true);
      } else {
        // Scatter spill or non-WriteEnable: invalidate any GRF mapping to
        // these offsets since we cannot propagate through them.
        for (unsigned int i = offset; i != (offset + numRows); ++i)
          invalidateOffset(i);
      }
      ++it;
      continue;
    }

    if (inst->isFillIntrinsic()) {
      auto *fill = inst->asFillIntrinsic();
      if (inst->isWriteEnableInst()) {
        // Record the fill's dst GRFs as holding the spilled data.
        unsigned offset = fill->getOffset();
        unsigned numRows = fill->getNumRows();
        auto [dstStartGRF, dstEndGRF] = getGRFRange(fill->getDst());
        std::vector<unsigned> srcGRFs;
        if (canReplaceFill(fill, srcGRFs) &&
            replaceFillWithMovs(bb, it, fill, srcGRFs)) {
          invalidateClobberedEntries(inst);
          for (unsigned i = 0; i < numRows; ++i)
            addEntry(offset + i, dstStartGRF + i);
          // it already advanced by erase
          continue;
        }
        // Fill not replaced — invalidate dst GRFs and record.
        invalidateClobberedEntries(inst);
        for (unsigned i = 0; i < numRows; ++i)
          addEntry(offset + i, dstStartGRF + i);
      } else {
        // Non-WriteEnable fill: invalidate destination GRFs (WAW clobber).
        invalidateClobberedEntries(inst);
      }
      ++it;
      continue;
    }

    if (inst->isPseudoKill()) {
      ++it;
      continue;
    }

    // Call/fcall: clear the entire table.
    if (inst->isCall() || inst->isFCall()) {
      clearTable();
      ++it;
      continue;
    }

    // Indirect addressing: conservatively clear the entire table.
    if (hasIndirectOperand(inst)) {
      clearTable();
      ++it;
      continue;
    }

    // Non-spill/fill instruction: invalidate clobbered GRFs.
    invalidateClobberedEntries(inst);
    ++it;
  }

  // Propagate table to fall-through successor if the BB ends with a
  // conditional goto and the fall-through BB's only predecessor is this BB.
  if (!offsetToGRFs.empty() && !bb->empty()) {
    G4_INST *lastInst = bb->back();
    if (lastInst->opcode() == G4_goto && lastInst->getPredicate()) {
      G4_BB *ftBB = bb->Succs.empty() ? nullptr : bb->Succs.front();
      if (ftBB && ftBB->Preds.size() == 1 && ftBB->Preds.front() == bb)
        bbEntryState[ftBB] = offsetToGRFs;
    }
  }
}

// Returns true if [aStart, aEnd] overlaps [bStart, bEnd] (inclusive ranges).
static bool rangesOverlap(unsigned aStart, unsigned aEnd, unsigned bStart,
                          unsigned bEnd) {
  return aStart <= bEnd && bStart <= aEnd;
}

// Remove all pending fill entries whose dst GRF range overlaps [startGRF,
// endGRF].
static void invalidatePendingFills(
    std::unordered_map<unsigned, vISA::PendingFill> &pendingFills,
    unsigned startGRF, unsigned endGRF) {
  for (auto it = pendingFills.begin(); it != pendingFills.end();) {
    if (rangesOverlap(it->second.dstStartGRF, it->second.dstEndGRF, startGRF,
                      endGRF)) {
      it = pendingFills.erase(it);
    } else
      ++it;
  }
}

// Invalidate pending fills whose dst GRF range overlaps the dst of inst (WAW).
void SpillFillPropagation::invalidateDst(
    G4_INST *inst, std::unordered_map<unsigned, PendingFill> &pendingFills) {
  if (pendingFills.empty())
    return;
  G4_DstRegRegion *dst = inst->getDst();
  if (dst && !dst->isNullReg() && dst->getBase() &&
      dst->getBase()->isRegVar() && dst->getTopDcl() &&
      hasAssignedGRF(dst->getTopDcl())) {
    auto [startGRF, endGRF] = getGRFRange(dst);
    invalidatePendingFills(pendingFills, startGRF, endGRF);
  }
}

// Invalidate pending fills whose dst GRF range overlaps any src of inst (WAR).
void SpillFillPropagation::invalidateSrcs(
    G4_INST *inst, std::unordered_map<unsigned, PendingFill> &pendingFills) {
  if (pendingFills.empty())
    return;
  for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
    G4_Operand *src = inst->getSrc(i);
    if (src && src->isSrcRegRegion() && src->getTopDcl() &&
        hasAssignedGRF(src->getTopDcl())) {
      auto [startGRF, endGRF] = getGRFRange(src);
      invalidatePendingFills(pendingFills, startGRF, endGRF);
    }
  }
}

// Order matched fills by ascending lexical id so each emitted mov chains in
// original program order after the previous one.
static const auto byLexIdAsc = [](const G4_INST *a, const G4_INST *b) {
  return a->getLexicalId() < b->getLexicalId();
};

// Backward (bottom-up) pass: collect pending fills and match them against
// earlier spills/fills that produce the same scratch data, inserting movs
// right after the source instruction.
void SpillFillPropagation::processBBBackward(G4_BB *bb) {
  std::unordered_map<unsigned, PendingFill> pendingFills;

  for (auto rit = bb->rbegin(), rend = bb->rend(); rit != rend; ++rit) {
    G4_INST *inst = *rit;

    // Skip pseudo-kills — they don't represent real writes.
    if (inst->isPseudoKill())
      continue;

    // Call/fcall: clear all pending fills.
    if (inst->isCall() || inst->isFCall()) {
      pendingFills.clear();
      continue;
    }

    // Fill: try to satisfy pending fills, then invalidate, then record.
    if (inst->isFillIntrinsic()) {
      auto *fill = inst->asFillIntrinsic();
      unsigned fillOffset = fill->getOffset();
      unsigned fillNumRows = fill->getNumRows();
      auto [fillDstStart, fillDstEnd] = getGRFRange(fill->getDst());
      if (inst->isWriteEnableInst()) {
        // Deterministic ordering of matched fills (see byLexIdAsc above).
        std::set<G4_INST *, decltype(byLexIdAsc)> matched(byLexIdAsc);
        // Iterate over all pending fills, and find matching ones. A matching
        // fill is one that's fully covered by the current fill
        // (fillOffset/NumRows).
        for (auto &[off, pf] : pendingFills) {
          if (matched.count(pf.inst))
            continue;
          if (fillOffset <= pf.scratchOffset &&
              fillOffset + fillNumRows >=
                  pf.scratchOffset + pf.numRows) {
            // Verify all rows are still present and point to same inst.
            bool allPresent = true;
            for (unsigned i = 0; i < pf.numRows; ++i) {
              auto rowIt = pendingFills.find(pf.scratchOffset + i);
              if (rowIt == pendingFills.end() ||
                  rowIt->second.inst != pf.inst) {
                allPresent = false;
                break;
              }
            }
            if (allPresent)
              matched.insert(pf.inst);
          }
        }
        // Ordinarily, each matching fill can be replaced with a MOV. However,
        // there are some cases where this isn't safe. For eg,
        //   intrinsic.fill.4 r122  Scratch[10x32]   // wide fill: r122..r125
        //   ...
        //   intrinsic.fill.1 r123  Scratch[12x32]   // becomes: mov r123, r124
        //   ...
        //   intrinsic.fill.1 r124  Scratch[11x32]   // becomes: mov r124, r123
        // Emitting both movs would swap r123/r124 and corrupt one value.
        // clobberedRows tracks dst GRFs written by already-emitted movs in
        // this batch; if a later match would read such a row, skip it and
        // leave the fill at its original program position.
        std::unordered_set<unsigned> clobberedRows;
        // Chain each emitted mov after the previous one so later matches don't
        // land between earlier movs and the source — keeps WAR order safe.
        INST_LIST_ITER insertAfterIt = std::next(rit).base();
        for (G4_INST *matchedInst : matched) {
          // Find the PendingFill for this matched instruction.
          PendingFill *matchedPF = nullptr;
          for (auto &[off, pf] : pendingFills) {
            if (pf.inst == matchedInst) {
              matchedPF = &pf;
              break;
            }
          }
          if (!matchedPF)
            continue;
          unsigned pfOffset = matchedPF->scratchOffset;
          unsigned pfNumRows = matchedPF->numRows;
          std::vector<unsigned> srcGRFs;
          srcGRFs.reserve(pfNumRows);
          for (unsigned i = 0; i < pfNumRows; ++i)
            srcGRFs.push_back(fillDstStart + (pfOffset - fillOffset) + i);

          bool conflicts = false;
          for (unsigned g : srcGRFs) {
            if (clobberedRows.count(g)) {
              conflicts = true;
              break;
            }
          }
          if (conflicts)
            continue;

          if (replaceFillWithMovsAfter(bb, rit, insertAfterIt, *matchedPF,
                                       srcGRFs)) {
            for (unsigned g = matchedPF->dstStartGRF;
                 g <= matchedPF->dstEndGRF; ++g)
              clobberedRows.insert(g);
            for (unsigned i = 0; i < pfNumRows; ++i)
              pendingFills.erase(pfOffset + i);
          }
        }

        // Invalidate pending fills with overlapping dst GRFs (WAW).
        invalidatePendingFills(pendingFills, fillDstStart, fillDstEnd);

        // Record as new pending fill.
        G4_Declare *dstTopDcl = fill->getDst()->getTopDcl();
        short dstRegOff = fill->getDst()->getRegOff();
        INST_LIST_ITER forwardIt = std::next(rit).base();
        for (unsigned i = 0; i < fillNumRows; ++i)
          pendingFills[fillOffset + i] = {inst, forwardIt, fillOffset,
                                          fillNumRows, fill->getVISAId(),
                                          dstTopDcl, dstRegOff,
                                          fillDstStart, fillDstEnd};
        invalidateSrcs(inst, pendingFills);
        continue;
      }
      // Invalidate pending fills with overlapping dst GRFs (WAW).
      invalidatePendingFills(pendingFills, fillDstStart, fillDstEnd);
      invalidateSrcs(inst, pendingFills);
      continue;
    }

    // Spill: try to satisfy pending fills, then invalidate for RAW.
    if (inst->isSpillIntrinsic()) {
      auto *spill = inst->asSpillIntrinsic();
      unsigned spillOffset = spill->getOffset();
      unsigned spillNumRows = spill->getNumRows();
      if (!spill->isScatterSpill() && inst->isWriteEnableInst()) {
        auto [payloadStart, payloadEnd] = getGRFRange(spill->getPayload());

        // Try to match pending fills whose rows are all still present.
        std::set<G4_INST *, decltype(byLexIdAsc)> matched(byLexIdAsc);
        for (auto &[off, pf] : pendingFills) {
          if (matched.count(pf.inst))
            continue;
          if (spillOffset <= pf.scratchOffset &&
              spillOffset + spillNumRows >=
                  pf.scratchOffset + pf.numRows) {
            bool allPresent = true;
            for (unsigned i = 0; i < pf.numRows; ++i) {
              auto rowIt = pendingFills.find(pf.scratchOffset + i);
              if (rowIt == pendingFills.end() ||
                  rowIt->second.inst != pf.inst) {
                allPresent = false;
                break;
              }
            }
            if (allPresent)
              matched.insert(pf.inst);
          }
        }
        std::unordered_set<unsigned> clobberedRows;
        INST_LIST_ITER insertAfterIt = std::next(rit).base();
        for (G4_INST *matchedInst : matched) {
          PendingFill *matchedPF = nullptr;
          for (auto &[off, pf] : pendingFills) {
            if (pf.inst == matchedInst) {
              matchedPF = &pf;
              break;
            }
          }
          if (!matchedPF)
            continue;
          unsigned pfOffset = matchedPF->scratchOffset;
          unsigned pfNumRows = matchedPF->numRows;
          std::vector<unsigned> srcGRFs;
          srcGRFs.reserve(pfNumRows);
          for (unsigned i = 0; i < pfNumRows; ++i)
            srcGRFs.push_back(payloadStart + (pfOffset - spillOffset) + i);

          bool conflicts = false;
          for (unsigned g : srcGRFs) {
            if (clobberedRows.count(g)) {
              conflicts = true;
              break;
            }
          }
          if (conflicts)
            continue;

          if (replaceFillWithMovsAfter(bb, rit, insertAfterIt, *matchedPF,
                                       srcGRFs)) {
            for (unsigned g = matchedPF->dstStartGRF;
                 g <= matchedPF->dstEndGRF; ++g)
              clobberedRows.insert(g);
            for (unsigned i = 0; i < pfNumRows; ++i)
              pendingFills.erase(pfOffset + i);
          }
        }

        // Invalidate pending fills whose dst overlaps spill payload (RAW).
        invalidatePendingFills(pendingFills, payloadStart, payloadEnd);
      }
      // Invalidate any pending fills for offsets that are stored by the spill.
      // Remove all rows of each affected pending fill.
      for (unsigned i = spillOffset; i != (spillOffset + spillNumRows); ++i) {
        auto it = pendingFills.find(i);
        if (it != pendingFills.end()) {
          unsigned pfOffset = it->second.scratchOffset;
          unsigned pfRows = it->second.numRows;
          for (unsigned j = 0; j < pfRows; ++j)
            pendingFills.erase(pfOffset + j);
        }
      }
      invalidateSrcs(inst, pendingFills);
      continue;
    }

    // Indirect addressing: conservatively clear all pending fills.
    if (hasIndirectOperand(inst)) {
      pendingFills.clear();
      continue;
    }

    // Generic instruction: invalidate for WAW (dst) and RAW (srcs).
    invalidateDst(inst, pendingFills);
    invalidateSrcs(inst, pendingFills);
  }
}

// Entry point. Run backward pass first to catch cases where the source GRFs
// are clobbered before a later fill, then forward pass for remaining cases.
void SpillFillPropagation::run() {
  if (gra.useLscForScatterSpill)
    return;

  // Assign fresh lexical ids so the backward pass can order matched pending
  // fills deterministically and by original program position.
  uint32_t id = 0;
  for (auto *bb : kernel.fg)
    for (auto *inst : *bb)
      inst->setLexicalId(id++);

  for (G4_BB *bb : kernel.fg)
    processBBBackward(bb);

  for (G4_BB *bb : kernel.fg)
    processBBForward(bb);
}
