/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __FLAGSPILLCLEANUP_H__
#define __FLAGSPILLCLEANUP_H__

#include "G4_IR.hpp"
#include "GraphColor.h"

#include <list>
#include <vector>

//
// Spill code clean up
//
namespace vISA {
typedef struct _CLEAN_NUM_PROFILE {
  unsigned spill_clean_num[10]{};
  unsigned fill_clean_num[10]{};
} CLEAN_NUM_PROFILE;

typedef struct _SCRATCH_RANGE {
  unsigned leftOff;
  unsigned rightOff;
} SCRATCH_RANGE;

typedef std::vector<SCRATCH_RANGE> SCRATCH_RANGE_VEC;
typedef std::vector<SCRATCH_RANGE>::iterator SCRATCH_RANGE_VEC_ITER;

typedef struct _RANGE {
  unsigned linearizedStart;
  unsigned linearizedEnd;
  bool predicate;
} REG_RANGE;

typedef std::vector<REG_RANGE> REG_RANGE_VEC;
typedef std::vector<REG_RANGE>::iterator REG_RANGE_VEC_ITER;

typedef std::pair<vISA::G4_INST *, int> RENAME_OPND;
typedef std::vector<RENAME_OPND> RANAME_VEC;

typedef struct _SCRATCH_ACCESS {
  // Basic info
#ifdef _DEBUG
  int regNum;
#endif
  vISA::G4_Declare *scratchDcl; // The scrach access
  vISA::G4_Operand *flagOpnd;
  INST_LIST_ITER inst_it;

  unsigned linearizedStart; // linearized start register address
  unsigned linearizedEnd;   // linearized end register address
  unsigned leftOff;         // left offset in scratch space
  unsigned rightOff;        // right offset in the scratch space
  unsigned useCount;

  bool isSpill = false;
  bool isBlockLocal = false;
  bool directKill = false;

  bool regKilled = false;
  bool regPartialKilled = false;
  bool regOverKilled = false;
  bool inRangePartialKilled = false;
  bool regInUse = false;

  bool fillInUse = false;
  bool removeable = false;
  bool instKilled = false;
  bool evicted = false;
  bool scratchDefined = false;

  unsigned maskFlag;

  RANAME_VEC renameOperandVec;
  SCRATCH_RANGE_VEC killedScratchRange;
  REG_RANGE_VEC killedRegRange;
  struct _SCRATCH_ACCESS *preScratchAccess;
  struct _SCRATCH_ACCESS *prePreScratchAccess;
  struct _SCRATCH_ACCESS *preFillAccess;

} SCRATCH_ACCESS;

typedef std::vector<SCRATCH_ACCESS *> SCRATCH_PTR_VEC;

typedef vISA::std_arena_based_allocator<SCRATCH_ACCESS *> SCRATCH_PTR_ALLOCATOR;
typedef std::list<SCRATCH_ACCESS *, SCRATCH_PTR_ALLOCATOR> SCRATCH_PTR_LIST;
typedef std::list<SCRATCH_ACCESS *, SCRATCH_PTR_ALLOCATOR>::iterator
    SCRATCH_PTR_LIST_ITER;

class FlagSpillCleanup {
private:
  GlobalRA &gra;

  void FlagLineraizedStartAndEnd(G4_Declare *topdcl, unsigned &linearizedStart,
                                 unsigned &linearizedEnd);
  bool replaceWithPreDcl(IR_Builder &builder, SCRATCH_ACCESS *scratchAccess,
                         SCRATCH_ACCESS *preScratchAccess);
  bool scratchKilledByPartial(SCRATCH_ACCESS *scratchAccess,
                              SCRATCH_ACCESS *preScratchAccess);
  bool addKilledGRFRanges(unsigned linearizedStart, unsigned linearizedEnd,
                          SCRATCH_ACCESS *scratchAccess,
                          G4_Predicate *predicate);
  bool regFullyKilled(SCRATCH_ACCESS *scratchAccess, unsigned linearizedStart,
                      unsigned linearizedEnd, unsigned short maskFlag);
  bool inRangePartialKilled(SCRATCH_ACCESS *scratchAccess,
                            unsigned linearizedStart, unsigned linearizedEnd,
                            unsigned short maskFlag);
  bool regDefineAnalysis(SCRATCH_ACCESS *scratchAccess,
                         unsigned linearizedStart, unsigned linearizedEnd,
                         unsigned short maskFlag, G4_Predicate *predicate);
  void regDefineFlag(SCRATCH_PTR_LIST *scratchTraceList, G4_INST *inst,
                     G4_Operand *opnd);
  bool regUseAnalysis(SCRATCH_ACCESS *scratchAccess, unsigned linearizedStart,
                      unsigned linearizedEnd);
  void regUseFlag(SCRATCH_PTR_LIST *scratchTraceList, G4_INST *inst,
                  G4_Operand *opnd, int opndIndex);
  void regUseScratch(SCRATCH_PTR_LIST *scratchTraceList, G4_INST *inst,
                     G4_Operand *opnd, Gen4_Operand_Number opndIndex);
  void initializeScratchAccess(SCRATCH_ACCESS *scratchAccess,
                               INST_LIST_ITER inst_it);
  bool initializeFlagScratchAccess(SCRATCH_PTR_VEC *scratchAccessList,
                                   SCRATCH_ACCESS *&scratchAccess,
                                   INST_LIST_ITER inst_it);
  void freeScratchAccess(SCRATCH_PTR_VEC *scratchAccessList);
  void flagDefine(SCRATCH_PTR_LIST &scratchTraceList, G4_INST *inst);
  void scratchUse(SCRATCH_PTR_LIST &scratchTraceList, G4_INST *inst);
  void flagUse(SCRATCH_PTR_LIST &scratchTraceList, G4_INST *inst);
  bool flagScratchDefineUse(G4_BB *bb, SCRATCH_PTR_LIST *scratchTraceList,
                            SCRATCH_PTR_VEC *candidateList,
                            SCRATCH_ACCESS *scratchAccess,
                            CLEAN_NUM_PROFILE *clean_num_profile);
  void flagSpillFillClean(G4_BB *bb, INST_LIST_ITER inst_it,
                          SCRATCH_PTR_VEC &scratchAccessList,
                          SCRATCH_PTR_LIST &scratchTraceList,
                          SCRATCH_PTR_VEC &candidateList,
                          CLEAN_NUM_PROFILE *clean_num_profile);
  void regFillClean(IR_Builder &builder, G4_BB *bb,
                    SCRATCH_PTR_VEC &candidateList,
                    CLEAN_NUM_PROFILE *clean_num_profile);
  void regSpillClean(IR_Builder &builder, G4_BB *bb,
                     SCRATCH_PTR_VEC &candidateList,
                     CLEAN_NUM_PROFILE *clean_num_profile);

public:
  void spillFillCodeCleanFlag(IR_Builder &builder, G4_Kernel &kernel,
                              CLEAN_NUM_PROFILE *clean_num_profile);
  FlagSpillCleanup(GlobalRA &g) : gra(g) {}
};
}; // namespace vISA

#endif // __FLAGSPILLCLEANUP_H__