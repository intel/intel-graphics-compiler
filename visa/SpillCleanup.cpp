/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SpillCleanup.h"
#include "Assertions.h"
#include "FlowGraph.h"
#include "GraphColor.h"

#include <list>

uint32_t computeFillMsgDesc(unsigned int payloadSize, unsigned int offset);
uint32_t computeSpillMsgDesc(unsigned int payloadSize, unsigned int offset);

namespace vISA {

static bool isGRFAssigned(G4_Operand *opnd) {
  vISA_ASSERT(opnd->isSrcRegRegion() || opnd->isDstRegRegion(),
              "expecting src/dst reg region");
  auto *topDcl = opnd->getTopDcl();
  if (!topDcl)
    return false;
  auto *regVar = topDcl->getRegVar();
  return regVar->getPhyReg() != nullptr;
}

G4_SrcRegRegion *CoalesceSpillFills::generateCoalescedSpill(
    G4_SrcRegRegion *header, unsigned int scratchOffset,
    unsigned int payloadSize, bool useNoMask, G4_InstOption mask,
    G4_Declare *spillDcl, unsigned int row) {
  // Generate split send instruction with specified payload size and offset
  auto spillSrcPayload = kernel.fg.builder->createSrc(
      spillDcl->getRegVar(), row, 0, kernel.fg.builder->getRegionStride1(),
      Type_UD);

  // Create send instruction with payloadSize starting at scratch offset min
  G4_Declare *fp = nullptr;
  if (kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc())
    fp = kernel.fg.getFramePtrDcl();
  unsigned int option = useNoMask ? InstOpt_WriteEnable : 0;
  auto spillInst = kernel.fg.builder->createSpill(
      kernel.fg.builder->createNullDst(Type_UW), header, spillSrcPayload,
      g4::SIMD16, payloadSize,
      GlobalRA::GRFToHwordSize(scratchOffset, *kernel.fg.builder), fp,
      static_cast<G4_InstOption>(option), false);

  if (!useNoMask) {
    // Care about mask only for non-NoMask sends
    spillInst->setMaskOption(mask);
  }

  return spillSrcPayload;
}

G4_INST *CoalesceSpillFills::generateCoalescedFill(G4_SrcRegRegion *header,
                                                   unsigned int scratchOffset,
                                                   unsigned int payloadSize,
                                                   unsigned int dclSize,
                                                   bool evenAlignDst) {
  // Generate split send instruction with specified payload size and offset
  // Construct fillDst
  const char *dclName = kernel.fg.builder->getNameString(
      32, "COAL_FILL_%" PRIu64, kernel.Declares.size());
  auto fillDcl = kernel.fg.builder->createDeclare(
      dclName, G4_GRF, kernel.numEltPerGRF<Type_UD>(), dclSize, Type_UD,
      DeclareType::CoalescedSpillFill);

  if (evenAlignDst) {
    fillDcl->setEvenAlign();
    gra.setEvenAligned(fillDcl, true);
  }
  fillDcl->setDoNotSpill();

  auto fillDst =
      kernel.fg.builder->createDst(fillDcl->getRegVar(), 0, 0, 1, Type_UW);

  G4_Declare *fp = nullptr;
  if (kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc())
    fp = kernel.fg.getFramePtrDcl();

  auto fillInst = kernel.fg.builder->createFill(
      header, fillDst, g4::SIMD16, payloadSize,
      GlobalRA::GRFToHwordSize(scratchOffset, *kernel.fg.builder), fp,
      InstOpt_WriteEnable, false);
  return fillInst;
}

G4_Declare *
CoalesceSpillFills::createCoalescedSpillDcl(unsigned int payloadSize) {
  // Construct spill src
  const char *dclName = nullptr;
  G4_Declare *spillDcl = nullptr;

  dclName = kernel.fg.builder->getNameString(32, "COAL_SPILL_%" PRIu64,
                                             kernel.Declares.size());
  spillDcl = kernel.fg.builder->createDeclare(
      dclName, G4_GRF, kernel.numEltPerGRF<Type_UD>(), payloadSize, Type_UD,
      DeclareType::CoalescedSpillFill);

  spillDcl->setDoNotSpill();

  return spillDcl;
}

void CoalesceSpillFills::coalesceSpills(
    std::list<INST_LIST_ITER> &coalesceableSpills, unsigned int min,
    unsigned int max, bool useNoMask, G4_InstOption mask) {
  // Generate fill with minimum size = max-min. This should be compatible with
  // payload sizes supported by hardware.
  unsigned int payloadSize = (max - min) + 1;

  auto leadInst = *coalesceableSpills.front();

  vISA_ASSERT(payloadSize == 1 || payloadSize == 2 || payloadSize == 4 ||
                  payloadSize == 8,
              "Unsupported payload size");

  std::set<G4_Declare *> declares;
  unsigned int minRow = UINT_MAX;
  for (const auto &d : coalesceableSpills) {
    auto src1Opnd = (*d)->getSrc(1)->asSrcRegRegion();
    auto curRow = src1Opnd->getLeftBound() / kernel.numEltPerGRF<Type_UB>();
    declares.insert(src1Opnd->getTopDcl());
    minRow = minRow > curRow ? curRow : minRow;
    vISA_ASSERT((*d)->isSpillIntrinsic() &&
                    !isGRFAssigned((*d)->asSpillIntrinsic()->getPayload()),
                "shouldnt coalesce spill with assigned GRF");
  }

  G4_Declare *dcl = nullptr;
  if (declares.size() == 1) {
    dcl = (*declares.begin());
  } else {
    dcl = createCoalescedSpillDcl(payloadSize);
    minRow = 0;
  }

  auto coalescedSpillSrc =
      generateCoalescedSpill(kernel.fg.builder->duplicateOperand(
                                 leadInst->asSpillIntrinsic()->getHeader()),
                             min, payloadSize, useNoMask, mask, dcl, minRow);
  coalescedSpillSrc->getInst()->inheritDIFrom(leadInst);

  if (declares.size() != 1) {
    for (const auto &c : coalesceableSpills) {
      unsigned int scratchOffset, scratchSize;
      getScratchMsgInfo((*c), scratchOffset, scratchSize);

      unsigned int rowOff = scratchOffset - min;
      replaceMap.insert(std::make_pair(
          (*c)->getSrc(1)->getTopDcl(),
          std::make_pair(coalescedSpillSrc->getTopDcl(), rowOff)));
    }
  }

  auto f = coalesceableSpills.back();
  f++;

  for (const auto &spill : coalesceableSpills) {
    gra.incRA.markForIntfUpdate(
        (*spill)->asSpillIntrinsic()->getPayload()->getTopDcl());
    curBB->erase(spill);
  }
  coalesceableSpills.clear();
  curBB->insertBefore(f, coalescedSpillSrc->getInst());
}

void CoalesceSpillFills::coalesceFills(
    std::list<INST_LIST_ITER> &coalesceableFills, unsigned int min,
    unsigned int max) {
  // Generate fill with minimum size = max-min. This should be compatible with
  // payload sizes supported by hardware.
  unsigned int payloadSize = (max - min) + 1;
  if (payloadSize == 3)
    payloadSize = 4;
  else if (payloadSize > 4)
    payloadSize = 8;
  else if (payloadSize == 0)
    payloadSize = 1;

  vISA_ASSERT(payloadSize == 1 || payloadSize == 2 || payloadSize == 4 ||
                  payloadSize == 8,
              "Unsupported payload size");

  // dclSize could be larger than payload size when
  // 2 variables across scratch writes are coalesced.
  unsigned int dclSize = payloadSize;
  for (const auto &c : coalesceableFills) {
    unsigned int scratchOffset, scratchSize;
    auto fill = (*c);
    getScratchMsgInfo((*c), scratchOffset, scratchSize);

    auto fillDst = fill->getDst();
    auto fillDstRegOff = fillDst->getRegOff();
    unsigned int dstDclRows = fillDst->getTopDcl()->getNumRows();
    unsigned int maxRow = dstDclRows + scratchOffset - fillDstRegOff - min;

    if (maxRow > dclSize)
      dclSize = maxRow;

    vISA_ASSERT((*c)->isFillIntrinsic() &&
                    !isGRFAssigned((*c)->asFillIntrinsic()->getDst()),
                "cannot coalesce fill with pre-assigned GRF");

    gra.incRA.markForIntfUpdate(fillDst->getTopDcl());
  }

  auto leadInst = *coalesceableFills.front();

  auto newFill =
      generateCoalescedFill(kernel.fg.builder->duplicateOperand(
                                leadInst->asFillIntrinsic()->getHeader()),
                            min, payloadSize, dclSize,
                            gra.isEvenAligned(leadInst->getDst()->getTopDcl()));
  newFill->inheritDIFrom(leadInst);

  for (const auto &c : coalesceableFills) {
    unsigned int scratchOffset, scratchSize;
    getScratchMsgInfo((*c), scratchOffset, scratchSize);

    unsigned int rowOff = scratchOffset - min;
    replaceMap.insert(
        std::make_pair((*c)->getDst()->getTopDcl(),
                       std::make_pair(newFill->getDst()->getTopDcl(), rowOff)));
  }

  auto f = coalesceableFills.front();
  f++;

  for (const auto &fill : coalesceableFills) {
    if (fill == f) {
      f++;
    }
    curBB->erase(fill);
  }

  coalesceableFills.clear();
  curBB->insertBefore(f, newFill);
}

// Return true if heuristic agrees to coalescing.
bool CoalesceSpillFills::fillHeuristic(
    std::list<INST_LIST_ITER> &coalesceableFills,
    std::list<INST_LIST_ITER> &instList,
    const std::list<INST_LIST_ITER> &origInstList, unsigned int &min,
    unsigned int &max) {
#if 0
    std::bitset<8> bits(0);
    vISA_ASSERT(cMaxFillPayloadSize == 8, "Handle other max fill payload size");
#else
  std::bitset<4> bits(0);
  vISA_ASSERT(cMaxFillPayloadSize == 4, "Handle other max fill payload size");
#endif

  if (coalesceableFills.size() <= 1) {
    return false;
  }

  min = 0xffffffff, max = 0;
  G4_Declare *header =
      (*coalesceableFills.front())->asFillIntrinsic()->getHeader()->getTopDcl();
  for (const auto &f : coalesceableFills) {
    if ((*f)->asFillIntrinsic()->getHeader()->getTopDcl() != header &&
        !(*f)->asFillIntrinsic()->getFP())
      return false;

    unsigned int scratchOffset, scratchSize;
    getScratchMsgInfo(*f, scratchOffset, scratchSize);

    if (scratchSize == 8) {
      return false;
    }

    if ((*f)->getDst()->getTopDcl()->isAddrSpillFill()) {
      return false;
    }

    for (auto i = scratchOffset; i < (scratchOffset + scratchSize); i++)
      bits.set(i - scratchOffset);

    if (min > scratchOffset)
      min = scratchOffset;

    if (max < (scratchOffset + scratchSize - 1))
      max = (scratchOffset + scratchSize - 1);
  }

  // Iterate over coalescable fills and ensure all rows of a variable
  // are fill candidates. If not, then don't fill. This helps cases like,
  // #1 FILL_V10(0,0) <-- load 0x10 ... (4 GRFs)
  // #2 FILL_V10(4,0) <-- load 0x14 ... (1 GRF)
  // #3 send ... FILL_V10(0,0)   ... (use 3 GRFs of FILL_V10)
  // #4 FILL_V11(0,0) <-- load 0x15 ... (1 GRF)
  //
  // Loads at #2 and #4 can be coalesced. But this requires a new coalesced
  // variable of size = 6 GRFs. This size can quickly increase for Cm where
  // payloads of 8 GRF are also present. So instead of allowing these cases
  // at the risk of spilling more, we require that all rows of fill range
  // are candidates in coalesceableFills list. So when only #2 and #4 are
  // fill candidates, we will not coalesce them. This makes us miss some
  // cases where subset rows of fills have been converted to movs.
  const int maxDclSize = 128;

  std::map<G4_Declare *, std::bitset<maxDclSize>> allRows;
  for (const auto &c : coalesceableFills) {
    auto topdcl = (*c)->getDst()->getTopDcl();

    unsigned int scratchOffset, scratchSize;
    getScratchMsgInfo(*c, scratchOffset, scratchSize);

    auto it = allRows.find(topdcl);
    if (it == allRows.end()) {
      allRows.insert(std::make_pair(topdcl, std::bitset<maxDclSize>()));
      it = allRows.find(topdcl);
    }

    // Now mark bits corresponding to rows
    unsigned int regOff = (*c)->getDst()->getRegOff();
    for (unsigned int r = regOff; r < (regOff + scratchSize); r++) {
      it->second.set(r);
    }
  }

  // Check whether all dcls in map have all rows filled
  for (auto &&r : allRows) {
    unsigned int numRows = r.first->getNumRows();

    for (unsigned int i = 0; i < numRows; i++) {
      if (r.second.test(i) == false) {
        // Found a row of variable that isnt captured in
        // list of candidate fills.
        return false;
      }
    }
  }

#if 0
    for (auto f : coalesceableFills)
    {
        unsigned int scratchOffset, scratchSize;
        getScratchMsgInfo(*f, scratchOffset, scratchSize);

        for (auto i = scratchOffset; i < (scratchOffset + scratchSize); i++)
            bits.set(i - min);
    }
#endif

  if (max - min <= 3) {
    // Will emit at most 4GRF read
    if (bits[0] != bits[1] && bits[2] != bits[3]) {
      // Don't coalesce patterns like
      // 1010, 0101
      return false;
    }

    if ((bits[0] & bits[3]) && !(bits[1] | bits[2])) {
      // 1001
      return false;
    }
  }

  return true;
}

bool CoalesceSpillFills::notOOB(unsigned int min, unsigned int max) {
  // Disallow coalescing if it exceeds spill size computed by RA
  // as this may cause segmentation fault.
  auto nextSpillOffset = spill.getNextOffset();
  auto diff = max - min;
  auto HWordByteSize = GlobalRA::getHWordByteSize();
  if (diff == 2 && (min + 3) * HWordByteSize >= nextSpillOffset) {
    // Coalesce 1-1-
    // Payload size 4 is needed as candidates are not back-to-back.
    // Coalescing is illegal if last row exceeds spill size computed by RA.
    return false;
  } else if (diff > 3 && diff < 7 &&
             (min + 7) * HWordByteSize >= nextSpillOffset) {
    // payload size = 8 would be used
    return false;
  }

  return true;
}

// instList contains all instructions (fills or spills) within window size.
// At exit, instList will contain instructions that will not be coalesced.
// coalescable list will contain instructions within min-max offset range.
// First instruction's offset in instList is set to be min. max is
// min + maxPayloadSize - 1.
void CoalesceSpillFills::sendsInRange(std::list<INST_LIST_ITER> &instList,
                                      std::list<INST_LIST_ITER> &coalescable,
                                      unsigned int maxPayloadSize,
                                      unsigned int &min, unsigned int &max) {
  min = 0xffffffff;
  max = 0;
  bool isFirstNoMask = false;
  unsigned int mask = 0;
  for (auto iter = instList.begin(); iter != instList.end();) {
    unsigned scratchOffset, sizeInGrfUnit, lastScratchOffset;
    auto inst = *(*iter);
    getScratchMsgInfo(inst, scratchOffset, sizeInGrfUnit);
    lastScratchOffset = scratchOffset + sizeInGrfUnit - 1;

    if (min == 0xffffffff && max == 0) {
      // First spill is definitely a candidate
      min = scratchOffset;
      max = lastScratchOffset;
      coalescable.push_back(*iter);
      iter = instList.erase(iter);
      isFirstNoMask = inst->isWriteEnableInst();
      mask = inst->getMaskOption();

      if (inst->getDst()->getTopDcl()->isAddrSpillFill()) {
        return;
      }

      continue;
    }

    if (min != 0xffffffff || max != 0) {
      bool maskMatch = (isFirstNoMask && inst->isWriteEnableInst()) ||
                       (mask == inst->getMaskOption());

      // don't coalesce if non-leading fill inst has alignment requirements,
      // as we may not be able to satisfy it
      bool fillDstisAligned = gra.isEvenAligned(inst->getDst()->getTopDcl());

      if (!maskMatch || fillDstisAligned) {
        iter++;
        continue;
      }

      // Check whether min/max can be extended
      if (scratchOffset <= min &&
          (min - scratchOffset) <= (cMaxFillPayloadSize - 1) &&
          (max - scratchOffset) <= (cMaxFillPayloadSize - 1) &&
          notOOB(scratchOffset, max)) {
        // This instruction can be coalesced
        min = scratchOffset;
        if (max < lastScratchOffset)
          max = lastScratchOffset;

        // vISA_ASSERT(max - min <= (cMaxFillPayloadSize - 1), "Unexpected
        // fills coalesced. (max - min) is out of bounds - 1");

        coalescable.push_back(*iter);
        iter = instList.erase(iter);
      } else if (scratchOffset >= max &&
                 (lastScratchOffset - min) <= (cMaxFillPayloadSize - 1) &&
                 (lastScratchOffset - max) <= (cMaxFillPayloadSize - 1) &&
                 notOOB(min, scratchOffset)) {
        max = lastScratchOffset;

        // vISA_ASSERT(max - min <= cMaxFillPayloadSize, "Unexpected spills
        // coalesced. (max - min) is out of bounds - 2");

        coalescable.push_back(*iter);
        iter = instList.erase(iter);
      } else if (scratchOffset >= min && lastScratchOffset <= max) {
        coalescable.push_back(*iter);
        iter = instList.erase(iter);
      } else {
        iter++;
      }
    }
  }
}

// instList contains all spills seen in window.
// coalescable is empty and should contain consecutive spills.
// This funtion will prune spills so they write consecutive
// memory slots. First spill is first candidate to start window.
void CoalesceSpillFills::keepConsecutiveSpills(
    std::list<INST_LIST_ITER> &instList, std::list<INST_LIST_ITER> &coalescable,
    unsigned int maxPayloadSize, unsigned int &minOffset,
    unsigned int &maxOffset, bool &useNoMask, G4_InstOption &mask) {
  // allowed list contains instructions to be coalesced in
  // ascending order of their spill slots.
  std::list<INST_LIST_ITER> allowed;
  auto origInstList = instList;
  allowed.push_back(instList.front());
  instList.pop_front();
  unsigned int maskOffset = (*allowed.front())->getMaskOption();
  mask = (G4_InstOption)(maskOffset & InstOpt_QuarterMasks);
  useNoMask = (maskOffset & InstOpt_WriteEnable) ? true : false;
  unsigned int size;
  getScratchMsgInfo(*allowed.front(), minOffset, size);
  maxOffset = minOffset + size - 1;

  bool firstSpillFromSend = false;
  G4_Declare *sendDstTopDcl = (*allowed.front())->getSrc(1)->getTopDcl();
  if (sendDstDcl.find(sendDstTopDcl) != sendDstDcl.end())
    firstSpillFromSend = true;

  G4_Declare *header =
      (*instList.front())->asSpillIntrinsic()->getHeader()->getTopDcl();

  for (const auto &instIt : instList) {
    auto inst = (*instIt);

    if (inst->asSpillIntrinsic()->getHeader()->getTopDcl() != header &&
        !inst->asSpillIntrinsic()->getFP()) {
      return;
    }

    useNoMask &= inst->isWriteEnableInst();

    if (!useNoMask)
      break;
  }

  if (useNoMask) {
    // Spill coalescing doesnt work as expected without NoMask
    bool redo;
    do {
      redo = false;
      for (auto spillIt = instList.begin(); spillIt != instList.end();
           spillIt++) {
        unsigned int scratchOffset, scratchSize;
        getScratchMsgInfo(*(*spillIt), scratchOffset, scratchSize);

        auto src1 = (*(*spillIt))->getSrc(1);
        if (src1 && src1->getTopDcl()->isAddrSpillFill()) {
          // Address taken dcls should not be coalesed with others.
          // This is dangerous because nothing ties indirect opnd
          // with fill/spill instructions for it. Only after RA do
          // we update offset of address register holding the
          // indirect operand, based on RA assigment to spill/fill
          // address taken variable.
          continue;
        }

        if ( // Consecutive scratch offsets
            scratchOffset == maxOffset + 1 &&
            // Scratch offset + size is within max payload size
            (scratchOffset + scratchSize - 1) <=
                (minOffset + maxPayloadSize - 1) &&
            // Either both masks are same or both are NoMask
            (((*(*spillIt))->getMaskOption() ==
              (maskOffset & InstOpt_QuarterMasks)) ||
             (useNoMask && (*(*spillIt))->isWriteEnableInst()))) {
          auto curInstDstTopDcl = (*(*spillIt))->getSrc(1)->getTopDcl();
          // Check whether current inst's topdcl was spilled in a send.
          // If it was and first instruction in instList wasnt then
          // don't consider current instruction as coalescing candidate.
          if (!firstSpillFromSend &&
              sendDstDcl.find(curInstDstTopDcl) != sendDstDcl.end()) {
            continue;
          }

          // This condition allows send coalescing iff
          // a. Either none of the vars are defined in a send
          // b. All vars defined in same send
          if (!firstSpillFromSend || curInstDstTopDcl == sendDstTopDcl) {
            if (curInstDstTopDcl == sendDstTopDcl) {
              // Make sure src1 operands are consecutive
              auto curSrc1Row =
                  (*(*spillIt))->getSrc(1)->asSrcRegRegion()->getRegOff();
              bool success = true;
              for (const auto &candidate : allowed) {
                unsigned int candOffset, candSize;
                getScratchMsgInfo(*candidate, candOffset, candSize);
                auto prevSrc1Row =
                    (*candidate)->getSrc(1)->asSrcRegRegion()->getRegOff();

                unsigned int scratchOffDelta = scratchOffset - candOffset;
                if ((prevSrc1Row + scratchOffDelta) != curSrc1Row) {
                  // Following is disallowed
                  // send  (8) V10(1,0) ... <-- resLen = 4
                  // sends (8) null r0 V10(1,0) 0x100 <-- extLen = 1
                  // mov   (8) T2  V10(2,0)
                  // sends (8) null r0 r10(3,0) 0x101 <-- extLen = 1
                  // mov   (8) T4  V10(4,0)
                  // Two scratch writes cannot be coalesced here
                  // because their src1 regions arent consecutive.
                  success = false;
                  break;
                }
              }
              if (!success)
                continue;
            }

            allowed.push_back(*spillIt);
            instList.erase(spillIt);
            redo = true;
            maxOffset += scratchSize;
            break;
          }
        }
      }
    } while (redo);
  }

  while (allowed.size() > 1) {
    unsigned int slots = maxOffset - minOffset + 1;
    if (slots == 2 || slots == 4) {
      // Insert coalescable spills in order of appearance
      for (const auto &origInst : origInstList) {
        for (const auto &allowedSpills : allowed) {
          if (*origInst == *allowedSpills) {
            coalescable.push_back(origInst);
            break;
          }
        }
      }

      vISA_ASSERT(coalescable.size() == allowed.size(),
                  "Coalesced spills list missing entries");
      break;
    } else {
      allowed.pop_back();
      unsigned int scratchOffset, scratchSize;
      getScratchMsgInfo(*allowed.back(), scratchOffset, scratchSize);
      maxOffset = scratchOffset + scratchSize - 1;
    }
  }

  instList = std::move(origInstList);
  for (auto coalIt = coalescable.begin(), instIt = instList.begin();
       coalIt != coalescable.end(); coalIt++) {
    if (*instIt == *coalIt)
      instIt = instList.erase(instIt);
    else {
      while (*instIt != *coalIt) {
        instIt++;
      }
      instIt = instList.erase(instIt);
    }
  }
}

INST_LIST_ITER
CoalesceSpillFills::analyzeSpillCoalescing(std::list<INST_LIST_ITER> &instList,
                                           INST_LIST_ITER start,
                                           INST_LIST_ITER end) {
  // Check and perform coalescing, if possible, amongst spills in instList.
  // Return inst iter points to either last inst+1 in instList if all spills
  // were coalesced. Otherwise, it points to first spill that wasnt coalesced.
  // Spill coalescing is possible only when all slots in coalesced range
  // have a write.
  INST_LIST_ITER last = end;
  last++;
  if (instList.size() < 2) {
    return last;
  }

  std::list<INST_LIST_ITER> coalesceableSpills;
  auto origInstList = instList;
  unsigned int min, max;
  G4_InstOption mask;
  bool useNoMask;
  keepConsecutiveSpills(instList, coalesceableSpills, cMaxSpillPayloadSize, min,
                        max, useNoMask, mask);
  if (coalesceableSpills.size() > 1) {
    coalesceSpills(coalesceableSpills, min, max, useNoMask, mask);
  } else {
    // When coalescing is not done, we want to
    // move to second instruction in instList in
    // next loop iteration.
    instList.pop_front();
  }

  if (instList.size() == 0) {
    return last;
  } else {
    return instList.front();
  }
}

INST_LIST_ITER
CoalesceSpillFills::analyzeFillCoalescing(std::list<INST_LIST_ITER> &instList,
                                          INST_LIST_ITER start,
                                          INST_LIST_ITER end) {
  // Check and perform coalescing, if possible, amongst fills in instList.
  // Return inst iter points to either last inst+1 in instList if all fills
  // were coalesced. Otherwise, it points to first fill that wasnt coalesced.
  INST_LIST_ITER last = end;
  last++;
#if 0
    G4_INST* lastInst = nullptr;
    if (last != bb->end())
        lastInst = (*last);
#endif
  if (instList.size() < 2) {
    return last;
  }

  std::list<INST_LIST_ITER> coalesceableFills;
  auto origInstList = instList;
  unsigned int min, max;
  sendsInRange(instList, coalesceableFills, cMaxFillPayloadSize, min, max);

  bool heuristic =
      fillHeuristic(coalesceableFills, instList, origInstList, min, max);
  if (!heuristic) {
    coalesceableFills.clear();
    instList = std::move(origInstList);
    instList.pop_front();
  }

  if (coalesceableFills.size() > 1) {
    coalesceFills(coalesceableFills, min, max);
  }

  if (instList.size() == 0) {
    return last;
  } else {
    return instList.front();
  }
}

bool CoalesceSpillFills::overlap(G4_INST *inst1, G4_INST *inst2,
                                 bool &isFullOverlap) {
  unsigned int scratchOffset1, scratchSize1, scratchOffset2, scratchSize2;
  unsigned int scratchEnd1, scratchEnd2;
  getScratchMsgInfo(inst1, scratchOffset1, scratchSize1);
  getScratchMsgInfo(inst2, scratchOffset2, scratchSize2);

  // isFullOverlap is true only if inst1 full covers inst2
  isFullOverlap = false;

  scratchEnd1 = scratchOffset1 + scratchSize1 - 1;
  scratchEnd2 = scratchOffset2 + scratchSize2 - 1;

  if (scratchOffset1 <= scratchOffset2) {
    // inst1    |---------|     or     |----------|
    // inst2         |------|             |---|
    if (scratchEnd1 >= scratchOffset2) {
      if (scratchOffset1 <= scratchOffset2 &&
          (scratchOffset1 + scratchSize1) >= (scratchOffset2 + scratchSize2)) {
        isFullOverlap = !isIncompatibleEMCm(inst1, inst2);
      }

      return true;
    }
  } else {
    // inst1          |------|      or       |-----|
    // inst2       |-----|               |-----------|
    if (scratchEnd2 >= scratchOffset1) {
      if (scratchOffset1 <= scratchOffset2 &&
          (scratchOffset1 + scratchSize1) >= (scratchOffset2 + scratchSize2)) {
        isFullOverlap = !isIncompatibleEMCm(inst1, inst2);
      }

      return true;
    }
  }

  return false;
}

bool CoalesceSpillFills::overlap(G4_INST *inst,
                                 std::list<INST_LIST_ITER> &allInsts) {
  for (const auto &sp : allInsts) {
    bool t;
    auto spillInst = (*sp);
    if (overlap(inst, spillInst, t))
      return true;
  }
  return false;
}

void CoalesceSpillFills::removeWARFills(std::list<INST_LIST_ITER> &fills,
                                        std::list<INST_LIST_ITER> &spills) {
  for (auto flIt = fills.begin(); flIt != fills.end();) {
    if (overlap((*(*flIt)), spills)) {
      flIt = fills.erase(flIt);
      continue;
    }
    flIt++;
  }
}

// Return true if an operand was replaced
bool CoalesceSpillFills::replaceCoalescedOperands(G4_INST *inst) {
  bool IRChanged = false;
  auto dst = inst->getDst();
  if (dst && dst->getTopDcl()) {
    auto dcl = dst->getTopDcl();
    auto it = replaceMap.find(dcl);

    if (it != replaceMap.end()) {
      auto dstRgn = dst->asDstRegRegion();
      auto newDstRgn = kernel.fg.builder->createDst(
          it->second.first->getRegVar(),
          it->second.second + dstRgn->getRegOff(), dstRgn->getSubRegOff(),
          dstRgn->getHorzStride(), dstRgn->getType());

      newDstRgn->setAccRegSel(dstRgn->getAccRegSel());
      inst->setDest(newDstRgn);
      IRChanged = true;
    }
  }

  for (unsigned int i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
    auto opnd = inst->getSrc(i);

    if (opnd && opnd->getTopDcl()) {
      auto dcl = opnd->getTopDcl();
      auto it = replaceMap.find(dcl);

      if (it == replaceMap.end())
        continue;

      if (opnd->isSrcRegRegion()) {
        auto srcRgn = opnd->asSrcRegRegion();
        auto oldRgnDesc = srcRgn->getRegion();

        auto newSrcRgn = kernel.fg.builder->createSrcRegRegion(
            srcRgn->getModifier(), Direct, it->second.first->getRegVar(),
            it->second.second + srcRgn->getRegOff(), srcRgn->getSubRegOff(),
            oldRgnDesc, opnd->getType());
        newSrcRgn->setAccRegSel(srcRgn->getAccRegSel());
        inst->setSrc(newSrcRgn, i);
        IRChanged = true;
      }
    }
  }

  return IRChanged;
}

bool CoalesceSpillFills::allSpillsSameVar(std::list<INST_LIST_ITER> &spills) {
  // Return true if all vars in spills list have same dcl
  G4_Declare *dcl = nullptr;
  for (const auto &s : spills) {
    auto topdcl = (*s)->getSrc(1)->getTopDcl();

    if (!dcl)
      dcl = topdcl;

    if (topdcl != dcl) {
      return false;
    }
  }

  // Allow only if all dcls are defined by same send
  if (sendDstDcl.find(dcl) != sendDstDcl.end())
    return true;

  return false;
}

void CoalesceSpillFills::fills() {
  // Iterate over all BBs, find fills that are closeby and coalesce
  // a bunch of them. Insert movs as required.
  for (auto bb : kernel.fg) {
    if (!isSpillCleanupEnabled(bb))
      continue;
    if (!gra.hasSpillCodeInBB(bb))
      continue;
    curBB = bb;
    auto endIter = bb->end();
    std::list<INST_LIST_ITER> fillsToCoalesce;
    std::list<INST_LIST_ITER> spills;
    INST_LIST_ITER startIter = bb->begin();
    unsigned int w = 0;
    unsigned int maxW = 0;
    unsigned int instRP = 0;
    unsigned int numRowsFillsToCoalesce = 0;
    const auto &splitInsts = LoopVarSplit::getSplitInsts(&gra, bb);
    bool earlyCoalesce = false;
    for (auto instIter = startIter; instIter != endIter;) {
      auto inst = (*instIter);

      if (inst->isPseudoKill() || inst->isLabel()) {
        instIter++;
        continue;
      }

      if (splitInsts.find(inst) != splitInsts.end()) {
        // if inst was emitted by loop split transformation,
        // then don't optimize it. such instructions are
        // emitted in loop preheader/loop exit. if a split
        // variable spills, we need to erase all fills and
        // spills emitted for that split. if we coalesce
        // 2 fills from split sequence, we may end up with
        // fill loading data that wont be used. for eg,
        //
        // (W) mov (8|M0) SPLIT1     V10
        // (W) mov (8|M0) SPLIT2     V11
        // ==>
        //
        // (W) fill FILL_V10 @ 0x0
        // (W) mov (8|M0) SPLIT1     FILL_V10
        // (W) fill FILL_V11 @ 0x32
        // (W) mov (8|M0) SPLIT2     FILL_V11
        //
        // we need to be able to erase 2nd fill and copy in
        // case SPLIT2 spills. this cannot be done if the 2
        // fills are coalesced.

        instIter++;
        continue;
      }

      // Determine window size based on register pressure
      unsigned int RP = rpe.getRegisterPressure(inst);
      if (RP > 0)
        instRP = RP;

      if (inst->isSpillIntrinsic()) {
        if (inst->asSpillIntrinsic()->isScatterSpill() &&
            overlap(inst, fillsToCoalesce)) {
          // If current intrinsic is scatter spill and ovelaps fills being
          // tracked, break the coalescing chain
          earlyCoalesce = true;
        }
        spills.push_back(instIter);
      } else if (inst->isFillIntrinsic()) {
        // Check if coalescing is possible
        if (fillsToCoalesce.size() == 0) {
          w = 0;
          maxW = 0;
          startIter = instIter;
          spills.clear();
          numRowsFillsToCoalesce = 0;
        }

        if (!overlap(*instIter, spills) &&
            // dont coalesce fills that have physical GRF assigned
            !isGRFAssigned(inst->getDst())) {
          fillsToCoalesce.push_back(instIter);
          numRowsFillsToCoalesce += inst->asFillIntrinsic()->getNumRows();
        }
      }

      if (fillsToCoalesce.size() > 0 && instRP > fillWindowSizeThreshold) {
        // High register pressure region so reduce window size to 3
        w = (cWindowSize - w > 3) ? cWindowSize - 3 : w;
      }

      if (w == cWindowSize &&
          (instRP + numRowsFillsToCoalesce) < highRegPressureForWindow) {
        // If register pressure is below the threshold even when considering
        // potential increase, continue coalescing fills in current BB
        --w;
      }

      if (w == cWindowSize || maxW == cMaxWindowSize || inst == bb->back() ||
          earlyCoalesce) {
        if (fillsToCoalesce.size() > 1) {
          auto nextInst =
              analyzeFillCoalescing(fillsToCoalesce, startIter, instIter);
          if (earlyCoalesce)
            instIter++;
          else
            instIter = nextInst;
        } else if (w == cWindowSize) {
          startIter = instIter;
        } else if (inst == bb->back()) {
          break;
        }

        w = 0;
        maxW = 0;
        numRowsFillsToCoalesce = 0;
        fillsToCoalesce.clear();
        spills.clear();
        earlyCoalesce = false;
        continue;
      }

      if (fillsToCoalesce.size() > 0) {
        w++;
        maxW++;
      }

      instIter++;
    }

    // One pass to replace old fills with coalesced dcl
    for (auto instIt = bb->begin(); instIt != bb->end();) {
      auto inst = (*instIt);

      if (inst->isPseudoKill() &&
          replaceMap.find(inst->getDst()->getTopDcl()) != replaceMap.end()) {
        instIt = bb->erase(instIt);
        continue;
      }

      while (replaceCoalescedOperands(inst)) {
        // replaceCoalescedOperands() updates references to old
        // dcls in IR that are coalesced. Having a single pass to
        // replace operands is insufficient if a coalesced range
        // gets re-coalesced. This can happen rarely when window
        // heuristic hits boundary condition.
      };
      instIt++;
    }
  }
}

void CoalesceSpillFills::populateSendDstDcl() {
  // Find and store all G4_Declares that are dest in sends
  // and are spilled. This is required when coalescing
  // scratch writes for such spills. We cannot mix coalescing
  // for G4_Declares from one and and other instructions.
  // Otherwise register pressure increases significantly.
  for (auto bb : kernel.fg) {
    for (auto inst : *bb) {
      if (inst->isSend() && inst->getDst()) {
        if (!inst->getDst()->isNullReg()) {
          if (!inst->getMsgDesc()->isScratch()) {
            auto topdcl = inst->getDst()->getTopDcl();

            sendDstDcl.insert(topdcl);
          }
        }
      } else if (inst->isSpillIntrinsic() &&
                 inst->getSrc(1)->getBase()->asRegVar()->isRegVarCoalesced()) {
        auto topdcl = inst->getSrc(1)->getTopDcl();

        sendDstDcl.insert(topdcl);
      }
    }
  }
}

void CoalesceSpillFills::spills() {
  populateSendDstDcl();

  // Iterate over all BBs, find fills that are closeby and coalesce
  // a bunch of them. Insert movs as required.
  for (auto bb : kernel.fg) {
    if (!isSpillCleanupEnabled(bb))
      continue;
    if (!gra.hasSpillCodeInBB(bb))
      continue;
    curBB = bb;
    auto endIter = bb->end();
    std::list<INST_LIST_ITER> spillsToCoalesce;
    INST_LIST_ITER startIter = bb->begin();
    unsigned int w = 0;
    unsigned int maxW = 0;
    unsigned int instRP = 0;
    unsigned int numRowsSpillsToCoalesce = 0;
    const auto &splitInsts = LoopVarSplit::getSplitInsts(&gra, bb);
    for (auto instIter = startIter; instIter != endIter;) {
      auto inst = (*instIter);

      if (inst->isPseudoKill() || inst->isLabel()) {
        instIter++;
        continue;
      }

      if (splitInsts.find(inst) != splitInsts.end()) {
        instIter++;
        continue;
      }

      // Determine window size based on register pressure
      unsigned int RP = rpe.getRegisterPressure(inst);
      if (RP > 0)
        instRP = RP;

      bool earlyCoalesce = false;
      if (inst->isSpillIntrinsic()) {
        // Check if coalescing is possible
        if (spillsToCoalesce.size() == 0) {
          w = 0;
          maxW = 0;
          numRowsSpillsToCoalesce = 0;
          startIter = instIter;
          spillsToCoalesce.clear();
        }

        if (isGRFAssigned(inst->asSpillIntrinsic()->getPayload())) {
          if (spillsToCoalesce.size() == 0) {
            ++instIter;
            continue;
          }
          // if spill intrinsic payload has physical GRF assigned then treat it
          // as fence
          earlyCoalesce = true;
        } else {
          for (auto coalIt = spillsToCoalesce.begin();
               coalIt != spillsToCoalesce.end();) {
            bool fullOverlap = false;
            if (overlap(*instIter, *(*coalIt), fullOverlap)) {
              if (fullOverlap) {
                gra.incRA.markForIntfUpdate((*(*coalIt))
                                                ->asSpillIntrinsic()
                                                ->getPayload()
                                                ->getTopDcl());
                // Delete earlier spill since its made redundant
                // by current spill.
                bb->erase(*coalIt);
              }

              if (inst->asSpillIntrinsic()->isScatterSpill()) {
                // If current spill is scatter and overlaps with other spills,
                // break the current coalescing chain
                earlyCoalesce = true;
                break;
              }

              coalIt = spillsToCoalesce.erase(coalIt);
              continue;
            }
            coalIt++;
          }
          if (!earlyCoalesce) {
            spillsToCoalesce.push_back(instIter);
            numRowsSpillsToCoalesce += inst->asSpillIntrinsic()->getNumRows();
          }
        }
      } else if (inst->isFillIntrinsic()) {
        for (auto coalIt = spillsToCoalesce.begin();
             coalIt != spillsToCoalesce.end();) {
          bool temp = false;
          if (overlap(*instIter, *(*coalIt), temp)) {
#if 1
            // Instead of deleting scratch writes try coalescing
            // at this point. This way maybe the fill can also
            // be cleaned up in later phase.
            earlyCoalesce = true;
            break;
#else
            coalIt = spillsToCoalesce.erase(coalIt);
            continue;
#endif
          }
          coalIt++;
        }
      }

      if (spillsToCoalesce.size() > 0 && instRP > spillWindowSizeThreshold) {
        if (!allSpillsSameVar(spillsToCoalesce)) {
          // High register pressure region so reduce window size to 3
          w = (cWindowSize - w > 3) ? cWindowSize - 3 : w;
        }
      }

      if (w == cWindowSize &&
          (instRP + numRowsSpillsToCoalesce) < highRegPressureForWindow) {
        // If register pressure is below the threshold even when considering
        // potential increase, continue coalescing spills in current BB
        --w;
      }

      if (w == cWindowSize || maxW == cMaxWindowSize || inst == bb->back() ||
          earlyCoalesce) {
        if (spillsToCoalesce.size() > 1) {
          auto nextInst =
              analyzeSpillCoalescing(spillsToCoalesce, startIter, instIter);
          if (earlyCoalesce)
            instIter++;
          else
            instIter = nextInst;
        } else if (w == cWindowSize) {
          startIter = instIter;
        } else if (inst == bb->back()) {
          break;
        }

        w = 0;
        maxW = 0;
        numRowsSpillsToCoalesce = 0;
        spillsToCoalesce.clear();
        continue;
      }

      if (spillsToCoalesce.size() > 0) {
        w++;
        maxW++;
      }

      instIter++;
    }

    // One pass to replace old fills with coalesced dcl
    for (auto instIt = bb->begin(); instIt != bb->end();) {
      auto inst = (*instIt);

      if (inst->isPseudoKill() &&
          replaceMap.find(inst->getDst()->getTopDcl()) != replaceMap.end()) {
        instIt = bb->erase(instIt);
        continue;
      }

      while (replaceCoalescedOperands(inst)) {
        // replaceCoalescedOperands() updates references to old
        // dcls in IR that are coalesced. Having a single pass to
        // replace operands is insufficient if a coalesced range
        // gets re-coalesced. This can happen rarely when window
        // heuristic hits boundary condition.
      };
      instIt++;
    }
  }
}

void CoalesceSpillFills::fixSendsSrcOverlap() {
  // Overlap for sends src operands is not allowed.
  //
  // Fix for following code pattern after spill/fill coalescing:
  // clang-format off
  // send (16)  COAL_FILL_373(0,0)<1>:ud    r0 0xa 0x24c2001:ud{Align1, NoMask} // scratch read, resLen=4, msgLen=1
  // sends(1)   null:ud     COAL_FILL_373(0,0)  COAL_FILL_373(1,0) 0x4c:ud 0x40680ff:ud{ Align1,Q1, NoMask } // a64 scattered write, resLen=0, msgLen=2, extMsgLen=1
  //
  // svm_scatter.1.1 (M1_NM, 1) V441.0 V449.0
  // clang-format on
  //
  // where V441 and V449 are both scalars of type :uq and :ud respectively
  //
  for (auto bb : kernel.fg) {
    curBB = bb;
    for (auto instIt = bb->begin(); instIt != bb->end(); instIt++) {
      auto inst = (*instIt);

      if (!inst->isSplitSend()) {
        continue;
      }

      auto src0 = inst->getSrc(0);
      auto src1 = inst->getSrc(1);

      if (src0->getTopDcl() == src1->getTopDcl()) {
        auto lb0 = src0->getLeftBound();
        auto rb0 = src0->getRightBound();
        auto lb1 = src1->getLeftBound();
        auto rb1 = src1->getRightBound();

        if ((lb0 < lb1 && rb0 > lb1) || (lb1 < lb0 && rb1 > lb0)) {
          // Ideally we should create copy of
          // operand with less number of GRFs,
          // but this is a really corner case
          // and probably shows up only for
          // force spills. So we simply choose
          // src1 of sends.
          const char *dclName = kernel.fg.builder->getNameString(
              32, "COPY_%zu", kernel.Declares.size());
          G4_Declare *copyDcl = kernel.fg.builder->createDeclare(
              dclName, G4_GRF, kernel.numEltPerGRF<Type_UD>(),
              src1->getTopDcl()->getNumRows(), Type_UD);
          gra.incRA.markForIntfUpdate(src1->getTopDcl());

          unsigned int elems = copyDcl->getNumElems();
          short row = 0;
          while (elems > 0) {
            G4_SrcRegRegion *srcRgn = kernel.fg.builder->createSrc(
                src1->getTopDcl()->getRegVar(), row, 0,
                kernel.fg.builder->getRegionStride1(), Type_UD);
            G4_DstRegRegion *dstRgn = kernel.fg.builder->createDst(
                copyDcl->getRegVar(), row, 0, 1, Type_UD);
            G4_INST *copyInst = kernel.fg.builder->createMov(
                g4::SIMD8, dstRgn, srcRgn, InstOpt_WriteEnable, false);
            bb->insertBefore(instIt, copyInst);

            if (gra.EUFusionNoMaskWANeeded()) {
              gra.addEUFusionNoMaskWAInst(bb, copyInst);
            }

            elems -= 8;
            row++;
          }

          G4_SrcRegRegion *sendSrc1 = kernel.fg.builder->createSrc(
              copyDcl->getRegVar(), 0, 0, kernel.fg.builder->getRegionStride1(),
              Type_UD);
          inst->setSrc(sendSrc1, 1);
        }
      }
    }
  }
}

void CoalesceSpillFills::removeRedundantSplitMovs() {
  // send (8) V_SAMPLER   -- resLen = 3
  // COAL_0(0,0) = V_SAMPLE(0,0)
  // COAL_0(1,0) = V_SAMPLE(1,0)
  // send (8) <null>    COAL_0(0,0) <-- len = 2
  // TV0(0,0) = V_SAMPLE(2,0)
  // ===>
  // send (8) V_SAMPLER   -- resLen = 3
  // send (8) <null>    V_SAMPLE(0,0) <-- len = 2
  // TV0(0,0) = V_SAMPLE(2,0)

  // Look for scratch writes. Src1 is data to write to memory.
  // Iterate in bottom-order to check whether raw movs exist
  // that define src1 of scratch write and whether their
  // source operands are consecutive.

  // Store numUses for dcls replaced and location of defs.
  // This structure is used to eliminate redundant movs
  // later.
  typedef std::pair<G4_BB *, INST_LIST_ITER> MovLoc;
  typedef unsigned int NumRefs;
  std::map<G4_Declare *, std::pair<NumRefs, std::list<MovLoc>>> movs;

  for (auto bb : kernel.fg) {
    // Store all dcls defined by non scratch sends
    // as only they are candidates for this pass.
    // Without this, we might end up identifying
    // other raw movs coming from partial write like:
    // clang-format off
    // add (8) r8.0<1>:q r20.0<4;4,1>:q r4.0<0;1,0>:ud {Align1, Q1}
    // send (16) r27.0<1>:uw r26 0xa 0x22c1000 : ud{ Align1, NoMask } // scratch read, fill, offset = 0, resLen=2, msgLen=1
    // mov (8) r27.0<1> q r8.0<4;4,1>:q{ Align1, Q1 }
    // sends (16) null:uw r26 r27 0x8a:ud 0x20f1000:ud{ Align1, NoMask } // scratch write, spill, offset = 0, resLen=0, msgLen=1, extMsgLen=2
    // clang-format on
    //
    // Although there is a raw mov before scratch write,
    // it has to be preserved for correctness.
    std::set<G4_Declare *> sendDst;
    for (auto inst : *bb) {
      if (inst->isSend() && inst->getDst() && !inst->getDst()->isNullReg() &&
          !inst->getMsgDesc()->isScratchRead() &&
          !inst->getMsgDesc()->isScratchWrite()) {
        sendDst.insert(inst->getDst()->getTopDcl());
      }
    }

    for (auto instIt = bb->begin(), endIt = bb->end(); instIt != endIt;
         instIt++) {
      auto inst = (*instIt);

      if (inst->isSpillIntrinsic()) {
        // Spill sends
        auto src1Dcl = inst->getSrc(1)->getTopDcl();
        unsigned int lb = inst->getSrc(1)->getLeftBound();
        unsigned int rb = inst->getSrc(1)->getRightBound();
        std::set<unsigned int> rows;
        for (unsigned int k = lb / kernel.numEltPerGRF<Type_UB>();
             k != (rb + kernel.numEltPerGRF<Type_UB>() - 1) /
                      kernel.numEltPerGRF<Type_UB>();
             k++) {
          rows.insert(k);
        }
        auto tmpIt = instIt;
        tmpIt--;
        G4_Declare *srcDcl = nullptr;
        std::map<unsigned int, unsigned int> dstSrcRowMapping;
        std::list<MovLoc> copies;
        while (tmpIt != bb->begin()) {
          auto pInst = (*tmpIt);

          // Each copy should be a raw mov
          if (!pInst->isRawMov())
            break;

          // Ensure src0 topdcl comes from a send dst in this BB
          if (sendDst.find(pInst->getSrc(0)->getTopDcl()) == sendDst.end())
            break;

          // Check whether dcls match
          auto pDstDcl = pInst->getDst()->getTopDcl();
          if (pDstDcl != src1Dcl)
            break;

          // Don't remove inst with pre-assigned GRF
          if (pDstDcl->getRegVar()->getPhyReg())
            break;

          unsigned int plb = pInst->getDst()->getLeftBound();
          unsigned int prb = pInst->getDst()->getRightBound();

          // Check whether complete row(s) defined
          if ((prb - plb + 1) % kernel.numEltPerGRF<Type_UB>() != 0)
            break;

          unsigned int rowStart = plb / kernel.numEltPerGRF<Type_UB>();
          unsigned int numRows =
              (prb - plb + 1) / kernel.numEltPerGRF<Type_UB>();
          bool punt = false;
          for (unsigned int k = rowStart; k != (rowStart + numRows); k++) {
            if (rows.find(k) == rows.end()) {
              punt = true;
              break;
            }
            dstSrcRowMapping.insert(std::make_pair(k, INT_MAX));
          }

          if (punt)
            break;

          auto pSrc0 = pInst->getSrc(0);
          if (!pSrc0->isSrcRegRegion())
            break;

          auto pSrcDcl = pSrc0->getTopDcl();
          if (!srcDcl)
            srcDcl = pSrcDcl;
          else if (srcDcl != pSrcDcl)
            break;

          // mov src should be GRF aligned
          if (pSrc0->getLeftBound() % kernel.numEltPerGRF<Type_UB>() != 0)
            break;

          unsigned int src0lb = pSrc0->getLeftBound();
          unsigned int src0rb = pSrc0->getRightBound();

          // (rb - lb) should match dst (rb - lb)
          if ((src0rb - src0lb) != (prb - plb))
            break;

          unsigned int pStartRow =
              pSrc0->getLeftBound() / kernel.numEltPerGRF<Type_UB>();
          for (unsigned int k = rowStart; k != (rowStart + numRows); k++) {
            auto it = dstSrcRowMapping.find(k);
            if (it == dstSrcRowMapping.end()) {
              punt = true;
              break;
            }

            it->second = pStartRow + (k - rowStart);
          }

          if (punt)
            break;

          copies.push_back(std::make_pair(bb, tmpIt));
          tmpIt--;
        }

        if (dstSrcRowMapping.size() > 0) {
          // Now check whether each entry of src1 has a corresponding src offset
          unsigned int dstRowStart = lb / kernel.numEltPerGRF<Type_UB>();
          bool success = true;
          auto baseIt = dstSrcRowMapping.find(dstRowStart);
          if (baseIt != dstSrcRowMapping.end()) {
            auto base = dstSrcRowMapping.find(dstRowStart)->second;
            for (const auto &m : dstSrcRowMapping) {
              unsigned int curRow = m.first - dstRowStart;
              if (m.second == INT_MAX) {
                success = false;
                break;
              }

              if (m.second != (base + curRow)) {
                success = false;
                break;
              }
            }

            if (success && srcDcl) {
              gra.incRA.markForIntfUpdate(inst->getSrc(1)->getTopDcl());
              gra.incRA.markForIntfUpdate(srcDcl);
              // Replace src1 of send with srcDcl
              G4_SrcRegRegion *sendSrc1 = kernel.fg.builder->createSrc(
                  srcDcl->getRegVar(), base, 0,
                  kernel.fg.builder->getRegionStride1(),
                  inst->getSrc(1)->getType());
              inst->setSrc(sendSrc1, 1);

              for (const auto &c : copies) {
                auto defDcl = (*c.second)->getDst()->getTopDcl();
                auto it = movs.find(defDcl);
                if (it == movs.end()) {
                  std::list<MovLoc> t;
                  t.push_back(c);
                  movs.insert(std::make_pair(defDcl, std::make_pair(0, t)));
                } else {
                  it->second.second.push_back(c);
                }
              }
            }
          }
        }
      }
    }
  }

  // Update number of uses of each dcl
  for (auto bb : kernel.fg) {
    for (auto instIt = bb->begin(), endIt = bb->end(); instIt != endIt;
         instIt++) {
      auto inst = (*instIt);

      if (inst->isPseudoKill()) {
        auto dcl = inst->getDst()->getTopDcl();
        auto it = movs.find(dcl);
        if (it != movs.end()) {
          it->second.second.push_back(std::make_pair(bb, instIt));
        }
      }

      for (unsigned int i = 0, numSrc = inst->getNumSrc(); i < numSrc; i++) {
        G4_Operand *opnd = inst->getSrc(i);

        if (opnd && opnd->getTopDcl()) {
          auto it = movs.find(opnd->getTopDcl());
          if (it != movs.end())
            it->second.first++;
        }
      }
    }
  }

  for (const auto &mov : movs) {
    auto dcl = mov.first;
    auto numRefs = mov.second.first;
    auto &allMovs = mov.second.second;

    if (numRefs == 0 && !dcl->getAddressed()) {
      for (const auto &m : allMovs) {
        auto bb = m.first;
        if (!isSpillCleanupEnabled(bb))
          continue;
        auto iter = m.second;
        bb->erase(iter);
      }
    }
  }
}

void CoalesceSpillFills::spillFillCleanup() {
  // Eliminate redundant fills when a write
  // is close by:
  //
  // spill TV1 at offset = 1
  // ..
  // ..
  // ..
  // fill FP1 from offset = 1
  // = FP1
  // ===>
  // Remove fill and replace occurence of FP1 with TV1
  //

  std::map<unsigned int, G4_INST *> writesPerOffset;
  std::set<G4_Declare *> defs;
  for (auto bb : kernel.fg) {
    if (!isSpillCleanupEnabled(bb))
      continue;
    if (!gra.hasSpillCodeInBB(bb))
      continue;
    curBB = bb;
    auto startIt = bb->begin();
    auto endIt = bb->end();
    const auto &splitInsts = LoopVarSplit::getSplitInsts(&gra, bb);
    unsigned int regPressure = 0;
    bool scatterSpillCleanup = false;
    for (auto instIt = startIt; instIt != endIt; instIt++) {
      auto inst = (*instIt);

      // register pressue estimate is computed per instruction before liveness
      // analysis.
      auto RP = rpe.getRegisterPressure(inst);
      // spill code is inserted after coloring is complete. so newly generated
      // spill instructions would not have valid register pressure estimate.
      // in case current instruction doesnt have valid register pressure
      // estimate, use a valid one from an earlier instruction.
      regPressure = (RP > 0) ? RP : regPressure;

      if (splitInsts.find(inst) != splitInsts.end())
        continue;

      if (inst->isFillIntrinsic()) {
        writesPerOffset.clear();
        defs.clear();
        scatterSpillCleanup = false;

        // Store offset, spill inst pair
        unsigned int rowStart, numRows;
        getScratchMsgInfo(inst, rowStart, numRows);
        unsigned int lastRow = rowStart + numRows - 1;

        // Scan window of instruction above current inst
        // to check whether all rows read by current inst
        // have been written.
        auto pInstIt = instIt;
        pInstIt--;
        unsigned int w = spillFillCleanupWindowSize;
        unsigned int maxW = 0;

        auto dstSize =
            inst->asFillIntrinsic()->getDst()->getTopDcl()->getNumRows();
        if (dstSize >= 8 || (totalInputSize > inputSizeLimit && dstSize >= 4)) {
          // avoid attempting cleanup for high reg pressure
          if (regPressure > highRegPressureForCleanup) {
            continue;
          }
        }

        if (isGRFAssigned(inst->getDst())) {
          // fill has GRF assigned so dont eliminate it
          continue;
        }

        while (pInstIt != startIt && w > 0 && maxW < cMaxWindowSize) {
          auto pInst = (*pInstIt);

          // If pInst instruction is created by split on spill pass
          // then skip cleanup. Because if this variable spills
          // in later RA iteration then the spill will be erased from
          // the BB rendering this cleanup optimization incorrect. For eg,
          // consider that split on spill optimization promoted a spilled
          // variable to a register when executing loop L1. This means
          // we'll insert a load in pre-header and a spill in loop exit (in
          // case the variable was updated in the loop). Here's an example
          // where LOOPSPLIT1 and LOOPSPLIT2 are created to promote 2
          // spilled variables in L1. Following shows loop exit BB:
          //
          // L1_Loop_Exit_BB:
          // Store LOOPSPLIT1 @ Scratch Offset 0x100 <-- split on spill temp
          // Store LOOPSPLIT2 @ Scratch Offset 0x200 <-- split on spill temp
          // ...
          // Load TMP @ Scratch Offset 0x100 <-- regular fill
          // Op Dst, TMP
          //
          // Under normal circumstances, we can propagate LOOPSPLIT1
          // to replace Load TMP with a register mov. But when LOOPSPLIT1
          // is a spill due to split on spill, we cannot do so. Because a
          // later RA iteration could decide to spill LOOPSPLIT1. When that
          // happens, we erase LOOPSPLIT1 from program and replace its use in
          // the loop with original spilled variable. Therefore, there's no
          // need to load anything in pre-header or store anything in loop exit
          // BB.
          if (splitInsts.find(pInst) != splitInsts.end())
            break;

          if (pInst->isSpillIntrinsic()) {
            unsigned int pRowStart, pNumRows;
            getScratchMsgInfo(pInst, pRowStart, pNumRows);

            // If any def of src1 dcl is found then don't
            // consider this write for optimization. Its
            // value in memory could be different than
            // one held in variable.
            auto pSrc1Dcl = pInst->getSrc(1)->getTopDcl();
            if (defs.find(pSrc1Dcl) != defs.end()) {
              // When a WAR is detected, punt out because
              // any older spill is going to be stale.
              // An optimization here is to detect variable
              // row on which WAR occurs. If the row is
              // different from current fill, then it
              // should be safe to ignore pInst and continue
              // till end of window.
              pInstIt--;
              break;
            }

            bool fullOverlap = false;
            if (overlap(pInst, inst, fullOverlap)) {
              if (pInst->asSpillIntrinsic()->isScatterSpill()) {
                // When a scatter spill is found and it overlaps the fill
                // being optimized, there are paths:
                // 1. If there are no other block spills in between, it's safe
                //    to optimize the current scatter spill and potentially
                //    others
                // 2. If there are block spills in between the cleanup search
                // stops.
                if (writesPerOffset.empty()) {
                  scatterSpillCleanup = true;
                } else if (!scatterSpillCleanup) {
                  break;
                }
              } else {
                // If block spill is found but there already scatter spills in
                // between, the cleanup search stops
                if (scatterSpillCleanup)
                  break;
              }

              for (unsigned int pRow = pRowStart;
                   pRow != (pRowStart + pNumRows); pRow++) {
                auto writeIt = writesPerOffset.find(pRow);

                // Check whether a more recent write was found for this row
                if (writeIt != writesPerOffset.end())
                  continue;

                writesPerOffset.insert(std::make_pair(pRow, pInst));
              }
            }
          }

          if (pInst->getDst() && pInst->getDst()->getTopDcl()) {
            // Store any defs seen to handle WAR
            defs.insert(pInst->getDst()->getTopDcl());
          }

          w--;
          maxW++;
          pInstIt--;

          RP = rpe.getRegisterPressure(pInst);
          if (RP > 0)
            regPressure = RP;

          if (w == 0 && (regPressure + numRows) < highRegPressureForWindow) {
            // Continue cleanup conservatively considering potential increase in
            // register pressure
            ++w;
          }
        }

        // Check whether writes for all rows were found
        bool found = true;
        for (auto row = rowStart; row <= lastRow; row++) {
          auto spillIt = writesPerOffset.find(row);
          if (spillIt == writesPerOffset.end() ||
              isIncompatibleEMCm((*spillIt).second, inst)) {
            found = false;
            break;
          }
        }

        if (!found) {
          continue;
        }

        // Writes for all rows found
        G4_ExecSize execSize = kernel.getSimdSize() > g4::SIMD16
                                   ? g4::SIMD16
                                   : kernel.getSimdSize();

        for (auto row = rowStart; row <= lastRow;) {
          if (execSize == g4::SIMD16 && row == lastRow) {
            // In case of odd rows in SIMD16
            execSize = g4::SIMD8;
          } else if (execSize == g4::SIMD16) {
            // In SIMD16 kernel 2 consecutive rows should come from same spill
            if (writesPerOffset.find(row)->second !=
                writesPerOffset.find(row + 1)->second) {
              execSize = g4::SIMD8;
            }
          }

          auto type = Type_UD;
          // In spill cleanup, all units are in hword units independent of
          // platform. For PVC, GRF size is 2x of Gen9. Correction to PVC is
          // postponed to code generation time when translating spill/fill
          // intrinsics to actual send. Since we're emitting a mov here, we need
          // to do this correction here.
          if (kernel.getGRFSize() == 64)
            type = Type_UQ;
          // Insert SIMD8 mov per row
          G4_DstRegRegion *nDst = kernel.fg.builder->createDst(
              inst->getDst()->getBase(),
              row + inst->getDst()->asDstRegRegion()->getRegOff() - rowStart, 0,
              1, type);

          auto write = writesPerOffset.find(row)->second;
          G4_SrcRegRegion *src1Write = write->getSrc(1)->asSrcRegRegion();
          gra.incRA.markForIntfUpdate(src1Write->getTopDcl());
          unsigned int writeRowStart = write->asSpillIntrinsic()->getOffset();
          unsigned int diff = row - writeRowStart;
          G4_SrcRegRegion *nSrc = kernel.fg.builder->createSrc(
              src1Write->getBase(), diff + src1Write->getRegOff(), 0,
              kernel.fg.builder->getRegionStride1(), type);

          G4_INST *mov = kernel.fg.builder->createMov(
              execSize, nDst, nSrc, InstOpt_WriteEnable, false);
          gra.incRA.markForIntfUpdate(nDst->getTopDcl());
          gra.incRA.markForIntfUpdate(nSrc->getTopDcl());
          bb->insertBefore(instIt, mov);

          if (gra.EUFusionNoMaskWANeeded()) {
            gra.addEUFusionNoMaskWAInst(bb, mov);
          }

          row += execSize / 8;
        }

        auto tempIt = instIt;
        tempIt--;
        bb->erase(instIt);
        instIt = tempIt;
      }
    }
  }
}

bool CoalesceSpillFills::isIncompatibleEMCm(G4_INST *inst1,
                                            G4_INST *inst2) const {
  vISA_ASSERT(curBB, "expecting valid G4_BB* containing inst1, inst2");
  vISA_ASSERT(std::find(curBB->begin(), curBB->end(), inst1) != curBB->end(),
              "expecting inst1 in bb");
  vISA_ASSERT(std::find(curBB->begin(), curBB->end(), inst2) != curBB->end(),
              "expecting inst2 in bb");

  if (curBB->isDivergent() && isCm) {
    // Cm program may write a variable using NoMask once and then
    // write it again with default EM. Later use of the variable could
    // use NoMask. For eg,
    // op1 (16) V10, ... {NoMask}
    // op2 (16) V10, ... {H1}
    // op3 (16) ..., V10 {NoMask}
    //
    // If V10 is spilled in above snippet, we'll emit 2 spill operations
    // and 1 fill operation. Spill for op2 doesn't use NoMask as send msg
    // can directly rely on EM behavior. Since fill uses NoMask, spill
    // cleanup cannot coalesce the second spill and fill away. So for Cm,
    // we disallow spill cleanup between a NoMask fill and default EM
    // spill in divergent BBs.
    return (inst1->isWriteEnableInst() ^ inst2->isWriteEnableInst());
  }
  return false;
}

void CoalesceSpillFills::removeRedundantWrites() {
  typedef std::list<std::pair<G4_BB *, INST_LIST_ITER>> SPILLS;
  typedef std::list<std::pair<G4_BB *, INST_LIST_ITER>> FILLS;
  std::map<unsigned int, std::pair<SPILLS, FILLS>> scratchOffsetAccess;
  // Traverse bottom-up to detect and remove redundant writes.
  // Redundant writes include:
  // 1. Successive writes to same offset without a fill in between,
  // 2. Writes in program without any fill from that slot throughout
  for (auto bb : kernel.fg) {
    curBB = bb;
    auto endIt = bb->end();
    endIt--;
    // Store spill slots that are written in to alongwith emask used
    std::map<unsigned int, unsigned int> scratchOffToMask;
    for (auto instIt = endIt; instIt != bb->begin(); instIt--) {
      auto inst = (*instIt);

      if (inst->isSpillIntrinsic() || inst->isFillIntrinsic()) {
        unsigned int offset = 0, size = 0;
        if (inst->isFillIntrinsic()) {
          getScratchMsgInfo(inst, offset, size);
          for (unsigned int k = offset; k != (offset + size); k++) {
            auto it = scratchOffToMask.find(k);
            if (it != scratchOffToMask.end()) {
              scratchOffToMask.erase(it);
            }
          }
        } else if (inst->isSpillIntrinsic()) {
          getScratchMsgInfo(inst, offset, size);
          bool allRowsFound = true;
          unsigned int emask = inst->getMaskOption();
          for (unsigned int k = offset; k != (offset + size); k++) {
            auto it = scratchOffToMask.find(k);
            if (it != scratchOffToMask.end()) {
              if (emask != it->second &&
                  (it->second & InstOpt_WriteEnable) == 0) {
                allRowsFound = false;
                break;
              }
            } else {
              allRowsFound = false;
              break;
            }
          }

          if (allRowsFound) {
            if (!isGRFAssigned(inst->getDst()))
              instIt = bb->erase(instIt);
          } else {
            unsigned int emask = inst->getOption();
            for (unsigned int k = offset; k != (offset + size); k++) {
              scratchOffToMask.insert(std::make_pair(k, emask));
            }
          }
        }
      }
    }
  }

  for (auto bb : kernel.fg) {
    auto endIt = bb->end();
    for (auto instIt = bb->begin(); instIt != endIt; instIt++) {
      auto inst = (*instIt);

      if (inst->isFillIntrinsic() || inst->isSpillIntrinsic()) {
        unsigned int offset, size;
        getScratchMsgInfo(inst, offset, size);
        bool isRead = inst->isFillIntrinsic();
        for (unsigned int i = offset; i != (offset + size); i++) {
          auto it = scratchOffsetAccess.find(i);
          if (it != scratchOffsetAccess.end()) {
            if (isRead) {
              auto &fill = it->second.second;
              fill.push_back(std::make_pair(bb, instIt));
            } else {
              auto &spill = it->second.first;
              spill.push_back(std::make_pair(bb, instIt));
            }
          } else {
            SPILLS s;
            FILLS f;
            if (isRead)
              f.push_back(std::make_pair(bb, instIt));
            else
              s.push_back(std::make_pair(bb, instIt));
            scratchOffsetAccess.insert(std::make_pair(i, std::make_pair(s, f)));
          }
        }
      }
    }
  }

  std::map<G4_INST *, std::pair<INST_LIST_ITER, G4_BB *>> spillToRemove;
  for (const auto &scratchAccess : scratchOffsetAccess) {
    if (scratchAccess.second.second.size() == 0 &&
        scratchAccess.second.first.size() > 0) {
      // 0 fills for scratch slot
      // Check whether all spill slots have 0 fills
      // in case spills are coalesced.
      for (const auto &spill : scratchAccess.second.first) {
        bool spillRequired = false;
        unsigned int offset, size;
        getScratchMsgInfo(*spill.second, offset, size);

        // Verify that all slots from offset->(offset+size) have 0 fills
        for (unsigned int slot = offset; slot != (offset + size); slot++) {
          auto it = scratchOffsetAccess.find(slot);
          if (it->second.second.size() != 0) {
            spillRequired = true;
            break;
          }
        }

        if (!spillRequired) {
          spillToRemove.insert(std::make_pair(
              *spill.second, std::make_pair(spill.second, spill.first)));
        }
      }
    } else if (scratchAccess.second.first.size() == 0 &&
               scratchAccess.second.second.size() > 0) {
      // 0 spills for scratch slot, non-zero fills
      // Check whether all fill slots have 0 spills
      // in case fills are coalesced.
      for (const auto &fill : scratchAccess.second.second) {
        if (*fill.second == gra.getRestoreBE_FPInst())
          continue;

        bool fillRequired = false;
        unsigned int offset, size;
        getScratchMsgInfo(*fill.second, offset, size);

        // Verify that all slots from offset->(offset+size) have 0 spills
        for (unsigned int slot = offset; slot != (offset + size); slot++) {
          auto it = scratchOffsetAccess.find(slot);
          if (it->second.first.size() != 0) {
            fillRequired = true;
            break;
          }
        }

        if (!fillRequired) {
          spillToRemove.insert(std::make_pair(
              *fill.second, std::make_pair(fill.second, fill.first)));
        }
      }
    }
  }

  for (const auto &removeSp : spillToRemove) {
    G4_BB *bb = removeSp.second.second;
    if (!isSpillCleanupEnabled(bb))
      continue;
    auto *inst = *removeSp.second.first;
    if ((inst->isSpillIntrinsic() &&
         !isGRFAssigned(inst->asSpillIntrinsic()->getPayload())) ||
        (inst->isFillIntrinsic() &&
         !isGRFAssigned(inst->asFillIntrinsic()->getDst()))) {
      if (inst->isSpillIntrinsic())
        gra.incRA.markForIntfUpdate(
            inst->asSpillIntrinsic()->getPayload()->getTopDcl());
      else if (inst->isFillIntrinsic())
        gra.incRA.markForIntfUpdate(
            inst->asFillIntrinsic()->getDst()->getTopDcl());
      bb->erase(removeSp.second.first);
    }
  }
}

void CoalesceSpillFills::run() {
  removeRedundantSplitMovs();

  fills();
  replaceMap.clear();
  spills();
  replaceMap.clear();
  spillFillCleanup();

  removeRedundantWrites();

  fixSendsSrcOverlap();

  kernel.dumpToFile("after.spillCleanup." + std::to_string(iterNo));
}

void CoalesceSpillFills::dumpKernel() {
  for (auto bb : kernel.fg) {
    for (auto inst : *bb) {
      inst->emit(std::cerr);
      std::cerr << "\t$" << inst->getVISAId() << ", #"
                << rpe.getRegisterPressure(inst) << "\n";
    }
  }
}

void CoalesceSpillFills::dumpKernel(unsigned int v1, unsigned int v2) {
  bool start = false, end = false, canEnd = false;
  for (auto bb : kernel.fg) {
    if (end)
      break;

    for (auto inst : *bb) {
      if (canEnd && inst->getVISAId() > (int)v2) {
        end = true;
        break;
      }

      if (inst->getVISAId() == v2) {
        // This ensures invalid offsets
        // are dumped till v2 is hit.
        canEnd = true;
      }

      if (inst->getVISAId() == v1)
        start = true;

      if (start && !end) {
        inst->dump();
        printf(" // $%d, #%d\n", inst->getVISAId(),
               rpe.getRegisterPressure(inst));
      }
    }
  }
}

} // namespace vISA
