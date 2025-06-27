/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "StaticProfiling.hpp"
#include <cmath>

using namespace vISA;

void StaticProfiling::ALUInstructionProfile(G4_INST *inst) {
  if (inst->isSend() || inst->isLabel() || inst->isCFInst() || inst->isDpas() ||
      inst->isIntrinsic()) {
    return;
  }

  FINALIZER_INFO *jitInfo = builder.getJitInfo();

  jitInfo->statsVerbose.numALUInst++;
}

unsigned StaticCycleProfiling::BBStaticCycleProfiling(G4_BB *bb) {
  using SWSBTokenType = vISA::G4_INST::SWSBTokenType;
  unsigned staticCycles = 0;
  G4_INST *preInst = nullptr;

  for (auto inst : *bb) {
    assert(inst);
    if (inst->isLabel()) {
      continue;
    }
    unsigned depCycles = 0;
    bool clearTokenDep = false;
    unsigned sbid = 0xFFFFFFFF;
    SB_INST_PIPE instPipe = inst->getInstructionPipeXe();
    staticCycles = staticCycles + (preInst ? LT->getOccupancy(preInst) : 0);

    // Dependence cycle from token dependence
    if (inst->getTokenType() == SWSBTokenType::AFTER_READ ||
        inst->getTokenType() == SWSBTokenType::AFTER_WRITE) {
      sbid = inst->getToken();
      if (inst->getTokenType() == SWSBTokenType::AFTER_READ) {
        if (tokenInsts[sbid].first != nullptr) {
          if (tokenInsts[sbid].first->isSend()) {
            depCycles = tokenInsts[sbid].second +
                        LT->getSendSrcReadLatency(tokenInsts[sbid].first);
          } else { //Use Occupany
            depCycles = tokenInsts[sbid].second +
                        LT->getOccupancy(tokenInsts[sbid].first);
          }
        }
      } else if (inst->getTokenType() == SWSBTokenType::AFTER_WRITE) {
        if (tokenInsts[sbid].first != nullptr) {
          depCycles =
              tokenInsts[sbid].second + LT->getLatency(tokenInsts[sbid].first);
          clearTokenDep = true;
        }
      }
    } else if (inst->tokenHonourInstruction() &&
               inst->getTokenType() ==
                   SWSBTokenType::SB_SET) { // Dependence caused by Same token
                                            // ID
      sbid = inst->getToken();
      if (tokenInsts[sbid].first != nullptr) {
        depCycles =
            tokenInsts[sbid].second + LT->getLatency(tokenInsts[sbid].first);
      }
    }

    SB_INST_PIPE depPipe = PIPE_NONE;
    if ((unsigned)inst->getDistance()) {
      depPipe = inst->getDistDepPipeXe();

      if (depPipe != PIPE_NONE) {
        if (distInsts[depPipe].size() >= (unsigned)inst->getDistance()) {
          auto t = distInsts[depPipe][(unsigned)inst->getDistance() - 1];
          depCycles = std::max(depCycles, t.second + LT->getOccupancy(t.first));
        }
      } else { // PIPE_NONE is treated as PIPE_ALL
        for (int i = PIPE_INT; i < PIPE_DPAS; i++) {
          if (distInsts[i].size() < (unsigned)inst->getDistance()) {
            continue;
          }
          auto t = distInsts[i][(unsigned)inst->getDistance() - 1];
          depCycles = std::max(depCycles, t.second + LT->getOccupancy(t.first));
        }
      }
    }

    //The schedule time of current instruction
    staticCycles = std::max(staticCycles, depCycles);

    // Update the token tracking board.
    // One token is active with only one token instruction.
    if (inst->tokenHonourInstruction() &&
        inst->getTokenType() == SWSBTokenType::SB_SET) {
      tokenInsts[inst->getToken()] = std::make_pair(inst, staticCycles);
    } else if (sbid != 0xFFFFFFFF && clearTokenDep) {
      tokenInsts[sbid] = std::make_pair(nullptr, 0);
    }

    //  Update the distance instruction tracking board.
    //  Different pipelines may have different distances.
    if (instPipe >= PIPE_INT && instPipe < PIPE_DPAS) {
      distInsts[instPipe].insert(distInsts[instPipe].begin(), std::make_pair(inst, staticCycles));
    }

    for (int i = PIPE_INT; i < PIPE_DPAS; i++) {
      int maxDist = SWSB_MAX_ALU_DEPENDENCE_DISTANCE;
      if (i == PIPE_LONG) {
        maxDist = SWSB_MAX_ALU_DEPENDENCE_DISTANCE_64BIT;
      }

      while ((int)(distInsts[i].size()) > maxDist) {
        distInsts[i].pop_back();
      }
    }

    std::stringstream ss;
    ss << " #";
    ss << staticCycles;
    ss << " ";

    inst->addComment(ss.str());

    preInst = inst;
  }

  return staticCycles;
}
