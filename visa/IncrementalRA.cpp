/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GraphColor.h"

#include <llvm/ADT/StringMap.h>

using namespace vISA;

IncrementalRA::IncrementalRA(GlobalRA &g) : gra(g), kernel(g.kernel) {
  level = kernel.getOptions()->getuInt32Option(vISA_IncrementalRA);
}

void IncrementalRA::reset() {
  selectedRF = G4_RegFileKind::G4_UndefinedRF;
  lrs.clear();
  needIntfUpdate.clear();
  maxDclId = kernel.Declares.size();

  if (isEnabledWithVerification()) {
    def_in.clear();
    def_out.clear();
    use_in.clear();
    use_out.clear();
    use_gen.clear();
    use_kill.clear();
    prevIterRefs.reset();
  }
}

// Invoked from ctor of latest GraphColor instance.
void IncrementalRA::registerNextIter(G4_RegFileKind rf,
                                     const LivenessAnalysis *liveness) {
  // If incremental RA is not enabled, reset state so we run
  // RA iteration with a clean slate.
  if (!level) {
    reset();
    return;
  }

  // Skip incremental RA for everything but GRF RA for now as we still need to
  // mark candidates in address, flag, scalar spill and cleanup.
  if (rf == G4_RegFileKind::G4_FLAG ||
      rf == G4_RegFileKind::G4_ADDRESS ||
      rf == G4_RegFileKind::G4_SCALAR) {
    reset();
    return;
  }

  if (rf != selectedRF) {
    reset();
    selectedRF = rf;
  }

  // Create live-ranges for new vars created in previous GraphColor instance
  if (kernel.Declares.size() > maxDclId) {
    unsigned int index = 0;
    for (auto dcl : kernel.Declares) {
      // No action needed for dcls already present or for alias dcls
      if (index++ < maxDclId || dcl->getAliasDeclare()) {
        continue;
      }

      // New dcl seen, so create live-range for it
      addNewRAVariable(dcl);
    }
  }

  if (liveness->livenessClass(G4_GRF)) {
    // recompute liveness of r0 in all iterations as it may be extended
    // due to spill instructions.
    markForIntfUpdate(kernel.fg.builder->getBuiltinR0()->getRootDeclare());
  }

  // Reset several fields of LiveRange instances from previous iteration. Fields
  // that are recomputed are the ones reset here.
  for (auto lr : lrs) {
    // Copy over some bit-fields so we can re-initialize them correctly.
    auto isPartialDcl = lr->getIsPartialDcl();
    lr->resetPhyReg();
    lr->setSpilled(false);
    lr->setUnconstrained(false);
    lr->setDegree(0);
    lr->setRefCount(0);
    lr->setBitFieldUnionValue(0);
    lr->setCandidate(true);
    lr->setSpillCost(0);
    lr->resetForbidden();

    if (isPartialDcl)
      lr->setIsPartialDcl();

    lr->initialize();
  }

  if (isEnabledWithVerification())
    verify(liveness);

  maxDclId = kernel.Declares.size();

  if (isEnabledWithVerification()) {
    // copy over liveness sets
    copyLiveness(liveness);
    // force compute var refs
    prevIterRefs = std::unique_ptr<VarReferences>(new VarReferences(gra.kernel));
    prevIterRefs->setStale();
    prevIterRefs->recomputeIfStale();
  }
}

void IncrementalRA::copyLiveness(const LivenessAnalysis* liveness) {
  def_in = liveness->def_in;
  def_out = liveness->def_out;
  use_in = liveness->use_in;
  use_out = liveness->use_out;
  use_gen = liveness->use_gen;
  use_kill = liveness->use_kill;
}

std::pair<bool, unsigned int>
IncrementalRA::getIdFromPrevIter(G4_Declare *dcl) {
  unsigned int id = UNDEFINED_VAL;
  auto it = varIdx.find(dcl);
  if (it != varIdx.end())
    id = (*it).second;
  return std::make_pair(it != varIdx.end(), id);
}

void IncrementalRA::recordVarId(G4_Declare *dcl, unsigned int id) {
  varIdx[dcl] = id;
  maxVarIdx = std::max(maxVarIdx, id);
}

void IncrementalRA::addNewRAVariable(G4_Declare *dcl) {
  // Assume new dcl already has a valid dclId.
  //
  // 1. Create new RAVarInfo entry in GlobalRA
  // 2. Create new LiveRange* for dcl
  // 3. Mark variable as partaker in incremental RA

  if (!level || !dcl || dcl->getAliasDeclare())
    return;

  gra.addVarToRA(dcl);

  // This could happen when we're in flag RA and new GRF temps are
  // created for spill/fill.
  if (!LivenessAnalysis::livenessClass(dcl->getRegFile(), selectedRF))
    return;

  auto lr = LiveRange::createNewLiveRange(dcl, gra);
  if (lr) {
    vISA_ASSERT(lrs.size() == lr->getVar()->getId(),
                "mismatch in lr index and regvar id");
    lrs.push_back(lr);
    vISA_ASSERT(lr->getVar()->isRegAllocPartaker(), "expecting RA partaker");
  }

  needIntfUpdate.insert(dcl);
}

void IncrementalRA::markForIntfUpdate(G4_Declare *dcl) {
  if (!level || !dcl || dcl->getAliasDeclare())
    return;

  needIntfUpdate.insert(dcl);
}

void IncrementalRA::skipIncrementalRANextIter() {
  // For passes that rarely executed or for debugging purpose,
  // we can invoke this method to skip running incremental RA
  // in following iteration.
  reset();
}

bool IncrementalRA::verify(const LivenessAnalysis *curLiveness) const {
  // Verify whether candidate set contains:
  // 1. Variables added in previous iteration (eg, spill temp, remat temp),
  // 2. Variables with modified liveness (eg, due to remat)
  //
  // If any candidate is missing then return false. Otherwise return true.
  bool status = true;
  llvm::StringMap<std::string> errorMsgs;

  // If candidate set is empty it means full RA will be run and there was no
  // previous iteration to perform incremental RA.
  if (needIntfUpdate.empty())
    return status;

  // Verify that id of G4_RegVar matches with index in lrs
  for (unsigned int i = 0; i != lrs.size(); ++i) {
    if (i != lrs[i]->getVar()->getId())
      errorMsgs.insert(std::make_pair(lrs[i]->getDcl()->getName(),
                                      "mismatch in lrs index and regvar id"));
  }

  // Verify newly added variables are RA candidates
  unsigned int idx = 0;
  for (auto dcl : kernel.Declares) {
    if (idx++ <= maxDclId)
      continue;

    if (dcl->getAliasDeclare() ||
        !LivenessAnalysis::livenessClass(dcl->getRegFile(), selectedRF))
      continue;

    if (needIntfUpdate.count(dcl) == 0) {
      errorMsgs.insert(std::make_pair(
          dcl->getName(), "Didn't find new variable in candidate list"));
      status = false;
    }
  }

  auto compare = [&](const std::vector<llvm_SBitVector> &curLivenessSet,
                     const std::vector<llvm_SBitVector> &oldLivenessSet,
                     std::string name) {
    for (unsigned int bb = 0; bb != kernel.fg.getBBList().size(); ++bb) {
      for (unsigned int i = 0, cnt = oldLivenessSet[bb].count(); i != cnt;
           ++i) {
        bool diff = curLivenessSet[bb].test(i) ^ oldLivenessSet[bb].test(i);
        if (diff && needIntfUpdate.count(lrs[i]->getDcl()) == 0) {
          errorMsgs.insert(std::make_pair(
              lrs[i]->getDcl()->getName(),
              "Variable liveness changed but not found in candidates set"));
          status = false;
        }
      }
    }
    return;
  };

  // Verify liveness delta between current liveness (parameter curLiveness) and
  // liveness data from previous iteration.
  if (def_in.empty() && def_out.empty() && use_in.empty() &&
      use_out.empty() && use_gen.empty() && use_kill.empty())
    return status;

  compare(curLiveness->def_in, def_in, "def_in");
  compare(curLiveness->def_out, def_out, "def_out");
  compare(curLiveness->use_in, use_in, "use_in");
  compare(curLiveness->use_out, use_out, "use_out");
  compare(curLiveness->use_gen, use_gen, "use_gen");
  compare(curLiveness->use_kill, use_kill, "use_kill");

  // Check whether opnds still appear in same instruction as previous iteration
  VarReferences refs(gra.kernel);
  refs.setStale();
  refs.recomputeIfStale();

  for (auto dcl : kernel.Declares) {
    if (!LivenessAnalysis::livenessClass(dcl->getRegFile(), selectedRF))
      continue;

    if (dcl->getAliasDeclare())
      continue;

    if (needIntfUpdate.count(dcl) > 0)
      continue;

    auto oldDefs = prevIterRefs->getDefs(dcl);
    auto oldUses = prevIterRefs->getUses(dcl);
    auto newDefs = refs.getDefs(dcl);
    auto newUses = refs.getUses(dcl);

    if ((oldDefs || newDefs) &&
        ((oldDefs && !newDefs) || (!oldDefs && newDefs) ||
         (*oldDefs != *newDefs))) {
      errorMsgs.insert(std::make_pair(
          dcl->getName(),
          "Variable appears in different defs but it isn't in candidate list"));
      status = false;
    }

    if ((oldUses || newUses) &&
        ((oldUses && !newUses) || (!oldUses && newUses) ||
         (*oldUses != *newUses))) {
      errorMsgs.insert(std::make_pair(
          dcl->getName(),
          "Variable appears in different uses but it isn't in candidate list"));
      status = false;
    }
  }

  for (auto& error : errorMsgs) {
    std::cerr << error.first().str() << " : " << error.second << "\n";
  }

  return status;
}

void IncrementalRA::computeLeftOverUnassigned(const LiveRangeVec &sorted,
                                    const LivenessAnalysis &liveAnalysis) {
  std::unordered_set<G4_Declare *> sortedSet;
  std::unordered_set<G4_Declare *> leftOver;
  for (auto lr : sorted)
    sortedSet.insert(lr->getDcl());

  for (auto dcl : sortedSet) {
    if (unassignedVars.count(dcl) > 0) {
      unassignedVars.erase(dcl);
    } else
      leftOver.insert(dcl);
  }
}
