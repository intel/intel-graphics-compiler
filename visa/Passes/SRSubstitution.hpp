/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _SRSUBSTITUION_H
#define _SRSUBSTITUION_H

#include "../BuildIR.h"
#include "../FlowGraph.h"
#include "../GraphColor.h"
#include "../PointsToAnalysis.h"

#define MAXIMAL_S0_SRC0_GRF_LENGTH 15
#define MAXIMAL_S0_SRC0_GRF_LENGTH_LARGE_GRF 8

typedef enum _INDIRECT_TYPE {
  NO_INDIRECT_SEND = 0,
  SAMPLER_MSG_ONLY = 1,
  ALL_MSGS = 2,
  ALWAYS_S0 = 3,
  UNSUPPORT
} INDIRECT_TYPE;

namespace vISA {

struct regMap {
  unsigned int localID;
  unsigned short dstReg;
  unsigned short srcReg;

  regMap() : localID(-1), dstReg(-1), srcReg(-1) { ; }
  regMap(unsigned int id, unsigned short dst, unsigned src)
      : localID(id), dstReg(dst), srcReg(src) {
    ;
  }
  ~regMap() {}
};

struct regMapBRA {
  G4_INST *inst = nullptr;
  Gen4_Operand_Number opndNum = Opnd_total_num;
  unsigned int offset = 0;
  G4_Operand *opnd = nullptr;

  regMapBRA() {}

  regMapBRA(G4_INST *i, Gen4_Operand_Number n, unsigned int off,
            G4_Operand *src)
      : inst(i), opndNum(n), offset(off), opnd(src) {
    ;
  }
  ~regMapBRA() {}
};

struct regCandidates {
  int firstDefID;
  bool includeSrc0;
  std::vector<regMap> dstSrcMap;
  regCandidates() : firstDefID(-1), includeSrc0(false) { dstSrcMap.clear(); }
};

struct regCandidatesBRA {
  int firstDefID;
  bool isLargeGRF;
  std::vector<regMapBRA> dstSrcMap;
  regCandidatesBRA() : firstDefID(-1), isLargeGRF(false) { dstSrcMap.clear(); }
};

class SRSubPassAfterRA {
  IR_Builder &builder;
  G4_Kernel &kernel;
  BitSet UsedS0SubReg;
  unsigned short S0SubRegNum = 0;
  unsigned short S0Index = 0;
  unsigned candidateID = 0;

public:
  SRSubPassAfterRA(IR_Builder &B, G4_Kernel &K) : builder(B), kernel(K) {
  }
  SRSubPassAfterRA(const SRSubPassAfterRA &) = delete;
  SRSubPassAfterRA& operator=(const SRSubPassAfterRA&) = delete;
  ~SRSubPassAfterRA() {
  };

  void run() {
    for (auto bb : kernel.fg) {
      SRSubAfterRA(bb);
    }
  }
  bool isRemoveAble(G4_INST *inst);
  G4_INST *getRemoveableImm(G4_INST *inst, std::vector<G4_INST *> &immMovs);
  bool isDefinedMultipleTimes(
      G4_INST *defInst, Gen4_Operand_Number opndNum,
      regCandidatesBRA &dstSrcRegs, std::vector<G4_INST *> &immMovs,
      std::vector<std::pair<Gen4_Operand_Number, unsigned>> &notRemoveableMap,
      BitSet &definedGRF);
  bool isSRCandidateAfterRA(G4_INST *inst, regCandidatesBRA &dstSrcRegs);
  unsigned short allocateS0(unsigned short UQNum);
  bool replaceWithSendiAfterRA(G4_BB *bb, INST_LIST_ITER instIter,
                               regCandidatesBRA &dstSrcRegs);
  void SRSubAfterRA(G4_BB *bb);
};

} // namespace vISA

#endif // _SRSUBSTITUION_H
