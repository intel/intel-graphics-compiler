/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GraphColor.h"

using namespace vISA;

IncrementalRA::IncrementalRA(GlobalRA &g) : gra(g), kernel(g.kernel) {
  level = kernel.getOptions()->getuInt32Option(vISA_IncrementalRA);
}

void IncrementalRA::reset() {
  selectedRF = G4_RegFileKind::G4_UndefinedRF;
  lrs.clear();
  needIntfUpdate.clear();
  maxDclId = kernel.Declares.size();

  if (isEnabledWithVerification(kernel)) {
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

  if (rf != selectedRF) {
    reset();
    selectedRF = rf;
  }

  // Create live-ranges for new vars created in previous GraphColor instance
  if (kernel.Declares.size() > maxDclId) {
    for (auto dcl : kernel.Declares) {
      // No action needed for dcls already present or for alias dcls
      if (dcl->getDeclId() <= maxDclId || dcl->getAliasDeclare())
        continue;

      // New dcl seen, so create live-range for it
      addNewRAVariable(dcl);
    }
  }

  if (isEnabledWithVerification(kernel))
    verify(liveness);

  maxDclId = kernel.Declares.size();

  if (isEnabledWithVerification(kernel)) {
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
    lrs.push_back(lr);
    vISA_ASSERT(lr->getVar()->isRegAllocPartaker(), "expecting RA partaker");
  }

  needIntfUpdate.insert(dcl);
}

void IncrementalRA::addCandidate(G4_Declare *dcl) {
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

  // If candidate set is empty it means full RA will be run and there was no
  // previous iteration to perform incremental RA.
  if (needIntfUpdate.size() == 0)
    return status;

  // Verify newly added variables are RA candidates
  for (auto dcl : kernel.Declares) {
    if (dcl->getDeclId() <= maxDclId)
      continue;

    if (dcl->getAliasDeclare() ||
        LivenessAnalysis::livenessClass(dcl->getRegFile(), selectedRF))
      continue;

    if (needIntfUpdate.count(dcl) == 0) {
      std::cerr << dcl->getName() << ": Didn't find new variable "
                << " in candidate list\n";
      status = false;
    }
  }

  auto compare = [&](const std::vector<SparseBitSet> &curLivenessSet,
                     const std::vector<SparseBitSet> &oldLivenessSet,
                     std::string name) {
    for (unsigned int bb = 0; bb != kernel.fg.getBBList().size(); ++bb) {
      for (unsigned int i = 0; i != oldLivenessSet[bb].getSize(); ++i) {
        bool diff = curLivenessSet[bb].isSet(i) ^ oldLivenessSet[bb].isSet(i);
        if (diff && needIntfUpdate.count(lrs[i]->getDcl()) == 0) {
          std::cerr << lrs[i]->getDcl()->getName()
                    << ": Variable liveness changed but not found in "
                       "candidates set\n";
          status = false;
        }
      }
    }
    return;
  };

  // Verify liveness delta between current liveness (parameter curLiveness) and
  // liveness data from previous iteration.
  if (def_in.size() == 0 && def_out.size() == 0 && use_in.size() == 0 &&
      use_out.size() == 0 && use_gen.size() == 0 && use_kill.size() == 0)
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
      std::cerr << dcl->getName()
                << ": Variable appears in differnet defs but it isn't in "
                   "candidate list\n";
      status = false;
    }

    if ((oldUses || newUses) &&
        ((oldUses && !newUses) || (!oldUses && newUses) ||
         (*oldUses != *newUses))) {
      std::cerr << dcl->getName()
                << ": Variable appears in differnet uses but it isn't in "
                   "candidate list\n";
      status = false;
    }
  }

  return status;
}
