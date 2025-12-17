/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "InsertS0Movs.hpp"
#include "Assertions.h"

using namespace vISA;

int64_t InsertS0Movs::computeHash(G4_Operand* operand) {
  if (operand == nullptr) return -1;

  return ((int64_t)operand->getLinearizedStart()
        + ((int64_t)(operand->getLinearizedEnd()) << 32));
}

std::vector<std::pair<int64_t, int> >::const_iterator
  InsertS0Movs::getEntryByRegOperand(int64_t hash) {

  auto isInRange = [&hash](std::pair<int64_t, int>const& item) {
    int lbA = (int)hash;
    int rbA = (hash >> 32);

    int lbB = (int)item.first;
    int rbB = (item.first >> 32);

    // Check overlap between A and B
    return (lbB < rbA && rbB >= lbA);
  };
  return std::find_if(regS0Vec.cbegin(), regS0Vec.cend(), isInRange);
}

std::vector<std::pair<int64_t, int> >::const_iterator
  InsertS0Movs::getEntryByS0Operand(int s0QW) {

  auto isMatch = [&s0QW](std::pair<int64_t, int>const& item) {
    return (item.second == s0QW);
  };
  return std::find_if(regS0Vec.cbegin(), regS0Vec.cend(), isMatch);
}

// return the s0QWSubReg associated with this register range (hash)
int InsertS0Movs::returnS0QWSubReg(int64_t hash) {
  auto entry = getEntryByRegOperand(hash);

  if (entry == regS0Vec.cend()) {
    // no hit, return -1
    return -1;
  }
  return (*entry).second;
};

bool InsertS0Movs::eraseEntryByRegOperand(int64_t hash) {
  auto entry = getEntryByRegOperand(hash);

  if (entry != regS0Vec.cend()) {
    regS0Vec.erase(entry);
    return true;
  }
  return false;
}

bool InsertS0Movs::eraseEntryByS0Operand(int s0QW) {
  auto entry = getEntryByS0Operand(s0QW);

  if (entry != regS0Vec.cend()) {
    regS0Vec.erase(entry);
    return true;
  }
  return false;
}

G4_SrcRegRegion* InsertS0Movs::allocateS0(G4_Operand* ind, INST_LIST_ITER ii) {
  // no ind or 0 value ind, return nullptr
  if (!ind) return nullptr;
  if ((ind->isImm() && ind->asImm()->getImm() == 0) ||
      ind->isNullReg()) {
    return nullptr;
  }

  if (ind->isS0()) {
    // scratch location is pinned to s0.7
    // So, in RA, we substitute scratchloc predefined variable to s0.7, and
    // hence, there is no need to allocate another s0
    // calling computeLeftBound here to ensure that scratchloc left bound is set
    // correctly before encoding
    ind->asSrcRegRegion()->computeLeftBound(builder);
    return nullptr;
  }

  auto *inst = *ii;
  if (inst->isSendg() &&
      inst->asSendInst()->getMsgDesc()->isScratch()) {
    vISA_ASSERT(ind->getTopDcl() == builder.getSpillSurfaceEfficient64b(),
                "unexpected ind0 on scratch msg");
    return nullptr;
  }

  // before assigning nextSurfaceS0QW, check if it is within bounds
  if (nextSurfaceS0QW == lasts0SubReg) {
    nextSurfaceS0QW = FIRST_SURFACE_S0_QW;
  }

  int s0QWSubReg = nextSurfaceS0QW;

  if (doOpt) {
    auto regID = computeHash(ind);
    auto s0QW = returnS0QWSubReg(regID);

    // query the regS0Vec
    if (s0QW == -1) {
      // Not a hit. Create an entry with pair <regID, s0qw>
      // Before inserting this pair into regS0Vec, existing pair with the same s0qw must be
      // removed
      (void)eraseEntryByS0Operand(s0QWSubReg);
      regS0Vec.emplace_back(std::make_pair(regID, s0QWSubReg));
      auto s0Dst = builder.createS0Dst(s0QWSubReg, IS_SIGNED_INT(ind->getType()) ? Type_Q : Type_UQ);
      // If ExecSize == 1, src must be scalar region
      if (ind->isSrcRegRegion() && !ind->isScalarSrc())
        ind->asSrcRegRegion()->setRegion(builder, builder.getRegionScalar());
      G4_INST* s0Mov = builder.createMov(g4::SIMD1, s0Dst, ind, InstOpt_WriteEnable, false);
      bb->insertBefore(ii, s0Mov);
      // increment the next surface
      nextSurfaceS0QW++;
    }
    else {
      // Found a range; do not create a mov and reuse the s0
      s0QWSubReg = s0QW;
      // Sampler instructions have two indirect descriptors (ind0 and ind1) to
      // store surface and sample base pointers. Consider a scenario where the
      // s0 qword (say s0.2) for ind0 can be reused while a new s0 qword is
      // required for ind1. A new s0 qword requires a mov instruction with
      // destination as the s0 qword. If nextSurfaceS0QW points to s0.2, then we
      // will overwrite the base address recorded for ind0 resulting in
      // incorrect execution. The solution is that for this case where we reuse
      // the s0, set the nextSurfaceS0QW to the next qword to ensure that ind1
      // is assigned a different s0 qword
      if (nextSurfaceS0QW == s0QW)
        nextSurfaceS0QW = s0QW + 1;
    }
  }
  else {
    auto s0Dst = builder.createS0Dst(s0QWSubReg,IS_SIGNED_INT(ind->getType()) ? Type_Q : Type_UQ);
    // If ExecSize == 1, src must be scalar region
    if (ind->isSrcRegRegion() && !ind->isScalarSrc())
      ind->asSrcRegRegion()->setRegion(builder, builder.getRegionScalar());
    G4_INST* s0Mov = builder.createMov(g4::SIMD1, s0Dst, ind, InstOpt_WriteEnable, false);
    bb->insertBefore(ii, s0Mov);
  }
  return builder.createS0Src(s0QWSubReg);
}

void InsertS0Movs::doInsertS0Movs() {
  // go through BBs and find sendg instruction
  // if there is ind0 scalar, introduce a mov and replace with s0.#
  // The optimization to clean up redundant s0 movs is local (per BB) for now.
  for (auto ii = bb->begin(); ii != bb->end(); ii++) {
    G4_INST *inst = *ii;
    // compute the value of ind0
    if (inst->isSendg()) {
      auto sendgInst = inst->asSendInst();
      auto ind0 = allocateS0(sendgInst->getIND0(), ii);
      if (ind0) {
        sendgInst->setIND0(ind0);
      }
      auto ind1 = allocateS0(sendgInst->getIND1(), ii);
      if (ind1) {
        sendgInst->setIND1(ind1);
      }
    }
    if (doOpt) {
      // Lookup the reg-s0 vector using the destination of instruction.
      // If hit, that entry will be removed from reg-s0 vector; otherwise,
      // nothing
      // TODO: Change eraseEntry return to return the s0 qword.
      // We can maintain a pool of free s0 qwords and allocate s0 qwords from
      // this pool. For now, ignoring the return type of eraseEntry
      (void)eraseEntryByRegOperand(computeHash(inst->getDst()));
    }
  }
}
