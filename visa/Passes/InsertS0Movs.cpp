/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "InsertS0Movs.hpp"
#include "Assertions.h"

#include <algorithm>
#include <unordered_set>

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
  bool hasErase = false;
  // The dst size may cover multiple s0 sub registers, so remove until all
  // covered ones are elimiated
  while (entry != regS0Vec.cend()) {
    hasErase = true;
    regS0Vec.erase(entry);
    entry = getEntryByRegOperand(hash);
  }
  return hasErase;
}

// Return the s0 qword already holding this immediate, or -1. Unlike register ranges there is no
// footprint to overlap, so equality of the immediate itself decides the hit.
int InsertS0Movs::returnS0QWSubRegForImm(G4_Imm* imm) {
  for (const auto& entry : immS0Vec) {
    if (entry.imm->isEqualTo(imm))
      return entry.s0QW;
  }
  return -1;
}

void InsertS0Movs::eraseImmEntryByS0Operand(int s0QW) {
  immS0Vec.erase(std::remove_if(immS0Vec.begin(), immS0Vec.end(),
                                [s0QW](const ImmS0Entry& entry) {
                                  return entry.s0QW == s0QW;
                                }),
                 immS0Vec.end());
}

bool InsertS0Movs::eraseEntryByS0Operand(int s0QW) {
  auto entry = getEntryByS0Operand(s0QW);

  if (entry != regS0Vec.cend()) {
    regS0Vec.erase(entry);
    return true;
  }
  return false;
}

// If the register staged into s0 is defined in this BB by a mov of an
// immediate with the same type, return that immediate so it could be moved
// into s0 directly. E.g.,
//
//  (W) mov (1|M0)  r5.0<1>:uq    0x12345678:uq
//      ...
//  (W) mov (1|M0)  s0.2<1>:uq    r5.0<0;1,0>:uq
//
// becomes
//
//  (W) mov (1|M0)  s0.2<1>:uq    0x12345678:uq
//
// The mov to r5 is returned in 'def'; doInsertS0Movs() removes it once
// the BB is done if r5 is BB local and has no other use.
// Return nullptr if no such immediate is found.
G4_Imm* InsertS0Movs::getPropagatableImm(G4_Operand* ind, INST_LIST_ITER ii,
                                         G4_INST*& def) {
  if (!ind->isSrcRegRegion() || !ind->isGreg())
    return nullptr;
  G4_SrcRegRegion* src = ind->asSrcRegRegion();
  if (src->isIndirect() || src->getModifier() != Mod_src_undef)
    return nullptr;

  unsigned srcLB = src->getLinearizedStart();
  unsigned srcRB = src->getLinearizedEnd();

  // Find the closest def of ind in this BB.
  for (auto ri = std::make_reverse_iterator(ii); ri != bb->rend(); ++ri) {
    def = *ri;
    G4_DstRegRegion* dst = def->getDst();
    if (!dst || dst->isNullReg() || !dst->isGreg())
      continue;
    // The footprint of an indirect write is unknown.
    if (dst->isIndirect())
      return nullptr;
    unsigned dstLB = dst->getLinearizedStart();
    unsigned dstRB = dst->getLinearizedEnd();
    if (dstRB < srcLB || dstLB > srcRB)
      continue;

    // The def must be an unconditional mov of an immediate starting at ind,
    // so that the value in ind is exactly the immediate.
    if (def->opcode() != G4_mov || def->getPredicate() || def->getCondMod() ||
        def->getSaturate())
      return nullptr;
    // Only (W) mov (1): the write happens regardless of the channel enables,
    // and it writes nothing but ind.
    if (!def->isWriteEnableInst() || def->getExecSize() != g4::SIMD1)
      return nullptr;
    G4_Operand* defSrc = def->getSrc(0);
    if (!defSrc->isImm() || defSrc->isRelocImm())
      return nullptr;
    if (dstLB != srcLB || dstRB < srcRB)
      return nullptr;
    // The only condition on types: all the same, so there's no conversion.
    if (dst->getType() != ind->getType() || defSrc->getType() != ind->getType())
      return nullptr;
    return defSrc->asImm();
  }
  return nullptr;
}

G4_SrcRegRegion* InsertS0Movs::allocateS0(G4_Operand* ind, INST_LIST_ITER ii,
                                          Gen4_Operand_Number opndNum) {
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

  // Constant propagation: stage the immediate defining ind instead of ind.
  // A zero immediate is left alone, as an immediate zero ind means no s0 at
  // all (see above), which is not what reading a register holding zero does.
  G4_INST* immDef = nullptr;
  if (G4_Imm* imm = getPropagatableImm(ind, ii, immDef)) {
    if (imm->getImm() != 0) {
      // The def is found by register footprint, so after RA it may write a
      // different variable sharing ind's register. Def-use links only connect
      // the same variable, so keep such a mov.
      const G4_Declare* indDcl = ind->getTopDcl();
      const G4_Declare* defDcl = immDef->getDst()->getTopDcl();
      if (indDcl && defDcl && indDcl == defDcl) {
        // This operand no longer reads the mov's dst.
        (*ii)->removeDefUse(opndNum);
        propagatedMovs.push_back(immDef);
      }
      ind = imm;
    }
  }

  // before assigning nextSurfaceS0QW, check if it is within bounds
  if (nextSurfaceS0QW == lasts0SubReg) {
    nextSurfaceS0QW = FIRST_SURFACE_S0_QW;
  }

  int s0QWSubReg = nextSurfaceS0QW;

  if (doOpt) {
    // Static sampler descriptors reach us as immediates rather than as a staged register, so they
    // are looked up in immS0Vec; everything else keeps matching on its register footprint.
    auto s0QW = ind->isImm() ? returnS0QWSubRegForImm(ind->asImm())
                             : returnS0QWSubReg(computeHash(ind));

    if (s0QW == -1) {
      // Not a hit, so a mov will overwrite this s0 qword. Both vectors index the same physical
      // qwords, so both must be cleared regardless of what kind of operand is being staged now -
      // splitting this per operand kind would leave a stale entry in the other vector, and the
      // next lookup that hits it would skip its mov and read a qword holding a different value.
      (void)eraseEntryByS0Operand(s0QWSubReg);
      eraseImmEntryByS0Operand(s0QWSubReg);
      if (ind->isImm())
        immS0Vec.push_back({ind->asImm(), s0QWSubReg});
      else
        regS0Vec.emplace_back(std::make_pair(computeHash(ind), s0QWSubReg));
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
      // Already in s0; do not create a mov and reuse the s0
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
      auto ind0 = allocateS0(sendgInst->getIND0(), ii, Opnd_src2);
      if (ind0) {
        sendgInst->setIND0(ind0);
      }
      auto ind1 = allocateS0(sendgInst->getIND1(), ii, Opnd_src3);
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

  // The movs of an immediate propagated into s0 movs may be dead now.
  removePropagatedMovs();
}

// Remove the movs in propagatedMovs whose dst is BB local and no longer read.
// Uses the def-use links and globalOpndHT from localDataFlowAnalysis():
//  - no use left: every read in this BB was propagated (allocateS0() drops
//    the link of each propagated operand);
//  - not global: no BB reads the dst without defining it first, so the value
//    does not reach any other BB.
void InsertS0Movs::removePropagatedMovs() {
  std::unordered_set<G4_INST*> removed;
  for (G4_INST* mov : propagatedMovs) {
    if (removed.count(mov))
      continue;
    G4_DstRegRegion* dst = mov->getDst();
    if (mov->use_size() != 0 || fg.globalOpndHT.isOpndGlobal(dst))
      continue;
    // Keep the mov if its dst may be read in a way the analysis does not
    // see: live out of the kernel/function or a pre-defined variable.
    const G4_Declare* dcl = dst->getTopDcl();
    if (dcl->isOutput() || dcl->isPreDefinedVar() || dcl->isBuiltin())
      continue;
    auto it = std::find(bb->begin(), bb->end(), mov);
    vISA_ASSERT(it != bb->end(), "propagated mov not found in its BB");
    bb->erase(it);
    removed.insert(mov);
  }
  propagatedMovs.clear();
}
