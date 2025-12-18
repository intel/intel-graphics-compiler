/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "G4_BB.hpp"
#include "BuildIR.h"
#include "LocalRA.h" // SECOND_HALF_BANK_START_GRF

#include <ostream>

using namespace vISA;

bool G4_BB::isSuccBB(G4_BB *succ) const {
  for (auto it = Succs.begin(), bbEnd = Succs.end(); it != bbEnd; ++it) {
    if ((*it) == succ)
      return true;
  }
  return false;
}

G4_Kernel &G4_BB::getKernel() const { return *getParent().getKernel(); }

//
// to check if the last instruction in list is EOT
//
bool G4_BB::isLastInstEOT() const {
  if (instList.size() == 0) {
    return false;
  }

  // Scan backward because some pass may insert nop after EOT.
  auto iter =
      std::find_if(instList.rbegin(), instList.rend(), [](G4_INST *inst) {
        return inst->opcode() != G4_nop && inst->opcode() != G4_mov;
      });

  if (iter == instList.rend()) { //There is only nop instructions
    return false;
  }

  G4_INST *i = *iter;

  if (parent->builder->hasSendShootdown()) {
    // due to send shootdown, a predicated send may not actually be an EOT
    return i->isEOT() && i->getPredicate() == NULL;
  } else {
    return i->isEOT();
  }
}

G4_opcode G4_BB::getLastOpcode() const {
  const G4_INST *i = instList.empty() ? nullptr : instList.back();
  if (i) {
    return i->opcode();
  } else {
    return G4_illegal;
  }
}

void G4_BB::setId(unsigned i) {
  // some analysis passes rely on G4_BB id
  if (id != i)
    getParent().markStale();
  id = i;
}

void G4_BB::removePredEdge(G4_BB *pred) {
  for (std::list<G4_BB *>::iterator it = Preds.begin(), bbEnd = Preds.end();
       it != bbEnd; ++it) {
    if (*it != pred)
      continue;
    // found
    Preds.erase(it);
    getParent().markStale();
    return;
  }
  vISA_ASSERT(false, ERROR_FLOWGRAPH); // edge is not found
}

void G4_BB::removeSuccEdge(G4_BB *succ) {
  for (std::list<G4_BB *>::iterator it = Succs.begin(), bbEnd = Succs.end();
       it != bbEnd; ++it) {
    if (*it != succ)
      continue;
    // found
    Succs.erase(it);
    getParent().markStale();
    return;
  }
  vISA_ASSERT(false, ERROR_FLOWGRAPH); // edge is not found
}

//
// find the fall-through BB of the current block.
// if the last inst is a unconditional jump, then the target is not considered a
// fall-through BB NOTE: Pay attention this function is only works after the
// handleReturn() duo the the conditional CALL
//
G4_BB *G4_BB::fallThroughBB() {
  G4_INST *last = (!instList.empty()) ? instList.back() : NULL;

  if (last) {
    if (last->opcode() == G4_goto || last->opcode() == G4_join) {
      return nullptr;
    }
    if (last->isFlowControl()) {
      // if No successor, return NULL;
      if (Succs.empty()) {
        return nullptr;
      }

      //
      // Instructions    Predicate-On    Predicate-Off    Num of Succ
      // Jmpi               Front                None               >=1
      // CALL               Front                None               >=2
      // considered the conditional call here while              Front Front 2
      // if, else           Front                Front              2
      // break, cont        Front                None               1,2
      // return             Front                None               >=1
      // do                 Front                Front              1
      // endif              Front                Front              1
      if (last->isCall()) {
        return BBAfterCall();
      } else if (!last->getPredicate() &&
                 // G4_while considered to fall trhu even without pred, since
                 // break jumps to while
                 (last->opcode() == G4_jmpi || last->opcode() == G4_break ||
                  last->opcode() == G4_cont || last->isReturn())) {
        return nullptr;
      } else {
        return Succs.front();
      }
    }
  }

  //
  // process other cases
  //
  if (Succs.size() == 0) // exit BB
    return NULL;         // no fall-through BB
  else
    return Succs.front();
}

G4_BB *G4_BB::BBBeforeCall() const {
  vISA_ASSERT((getBBType() & G4_BB_RETURN_TYPE),
              "this must be a subroutine return BB");
  return physicalPred;
}

G4_BB *G4_BB::BBAfterCall() const {
  vISA_ASSERT((getBBType() & G4_BB_CALL_TYPE),
              "this must be a subroutine call BB");
  return physicalSucc;
}

bool G4_BB::isAllLaneActive() const {
  G4_Kernel *pK = parent->getKernel();
  if (pK->getInt32KernelAttr(Attributes::ATTR_Target) == VISA_CM &&
      !isDivergent()) {
    // CM: if BB isn't divergent, all lanes (32) must be active (dmask =
    // 0xFFFFFFFF)
    return true;
  }
  return false;
}

void G4_BB::emit(std::ostream &output) {
  for (INST_LIST_ITER it = instList.begin(); it != instList.end(); ++it) {
    emitInstruction(output, it);
  }
}
void G4_BB::emitInstruction(std::ostream &output, INST_LIST_ITER &it) {
  // prints out instruction line
  if (!parent->getKernel()->getOptions()->getOption(
          vISA_disableInstDebugInfo)) {
    emitInstructionSourceLineMapping(output, it);
  }

  emitBasicInstruction(output, it);

  output << "\n";
}
void G4_BB::emitBasicInstruction(std::ostream &output, INST_LIST_ITER &it) {
  if ((*it)->isSend()) {
    //
    // emit send instruction
    //
    G4_InstSend *SendInst = (*it)->asSendInst();
    if (SendInst) {
      SendInst->emit_send(output);
      SendInst->emit_send_desc(output);
    }
  } else {
    //
    // emit label and instruction
    //
    G4_INST *inst = *it;
    inst->emit(output);
    if ((*it)->isLabel() == false) {
      emitBankConflict(output, inst);
    }
  }
}
void G4_BB::emitBasicInstructionComment(std::ostream &output,
                                        INST_LIST_ITER &it, int *suppressRegs,
                                        int *lastRegs, int32_t pc) {
  const G4_INST *inst = *it;
  auto platform = inst->getPlatform();

  if (!inst->isLabel() && inst->opcode() < G4_NUM_OPCODE) {
    output << " // ";

    auto comments = inst->getComments();
    if (!comments.empty()) {
      output << " " << comments << "; ";
    }
    int vISAId = inst->getVISAId();
    if (vISAId != -1) {
      output << "$" << vISAId;
    }

    if (getParent().getKernel()->getOption(vISA_DumpSBID)) {
      int lexicalId = inst->getLexicalId();
      if (lexicalId != -1) {
        output << "&" << lexicalId;
      }
    }

    if (getParent().getKernel()->getOption(vISA_DumpGenOffset) &&
        inst->getGenOffset() != UNDEFINED_GEN_OFFSET)
      output << ":%" << inst->getGenOffset();

    if (inst->getBuilder().getPlatformGeneration() < PlatformGen::XE) {
      emitBankConflict(output, inst);
    } else {
      int sameBankConflicts = 0;
      int twoSrcConflicts = 0;
      int simd16SuppressionConflicts = 0;
      unsigned BCNum = 0;
      if (parent->builder->hasEarlyGRFRead()) {
        BCNum = emitBankConflictXeLP(output, inst, suppressRegs, lastRegs,
                                     sameBankConflicts, twoSrcConflicts,
                                     simd16SuppressionConflicts);
      } else {
        BCNum = emitBankConflictXe(
            output, inst, suppressRegs, sameBankConflicts, twoSrcConflicts,
            simd16SuppressionConflicts,
            parent->builder->hasOneGRFBank16Bundles(), platform == GENX_TGLLP,
            parent->builder->has64bundleSize());
      }
      auto jitInfo = getParent().getKernel()->fg.builder->getJitInfo();
      jitInfo->statsVerbose.BCNum += BCNum;
      jitInfo->statsVerbose.numByteRMWs += countReadModifyWrite(output, inst);
    }

    if (getParent().getKernel()->getOption(vISA_PrintInstOffsetInAsm))
      output << " inst_offset=" << pc;
  }
}

void G4_BB::emitInstructionSourceLineMapping(std::ostream &output,
                                             INST_LIST_ITER &it) {
  // We record the previous instruction's source code locations so that they are
  // emitted only when there's a change.
  // Using global variables is ok here since this function is for shader dumps
  // (i.e., debugging) only.
  static const char *prevFilename = nullptr;
  static int prevSrcLineNo = 0;
  static bool resetOnEntry = false;

  const char *curFilename = (*it)->getSrcFilename();
  int curSrcLineNo = (*it)->getLineNo();

  // Reset source locations for each function so that we will always emit them
  // at function entry.
  if (getParent().getEntryBB() == this && !resetOnEntry) {
    prevFilename = nullptr;
    prevSrcLineNo = 0;
    // First time we process entry BB, we must reset state
    resetOnEntry = true;
  }

  if (getParent().getEntryBB() != this) {
    // Once we see some other BB, we should reset state only when we see entry
    // again next time.
    resetOnEntry = false;
  }

  if ((*it)->isLabel())
    return;
  bool emitFile = curFilename && (prevFilename == nullptr ||
                                  strcmp(prevFilename, curFilename) != 0);
  bool emitLineNo = prevSrcLineNo != curSrcLineNo && curSrcLineNo != 0;

  if (emitFile)
    output << "\n// File: " << curFilename << "\n";

  if (emitLineNo) {
    output << "\n// Line " << curSrcLineNo;
    if (curFilename) {
      std::string curLine =
          parent->getKernel()->getDebugSrcLine(curFilename, curSrcLineNo);
      if (!curLine.empty()) {
        auto isNotSpace = [](int ch) { return !std::isspace(ch); };
        curLine.erase(curLine.begin(),
                      std::find_if(curLine.begin(), curLine.end(), isNotSpace));
        curLine.erase(
            std::find_if(curLine.rbegin(), curLine.rend(), isNotSpace).base(),
            curLine.end());
        output << ":  " << curLine;
      }
    }
    output << "\n";
  }

  if (emitFile)
    prevFilename = curFilename;
  if (emitLineNo)
    prevSrcLineNo = curSrcLineNo;
}

void G4_BB::emitBankConflict(std::ostream &output, const G4_INST *inst) {
  int regNum[2][G4_MAX_SRCS];
  int execSize[G4_MAX_SRCS];
  int regSrcNum = 0;

  if (inst->isDpas()) {
    return;
  }

  if (inst->getNumSrc() == 3 && !inst->isSend()) {
    for (unsigned i = 0; i < 3; i++) {
      G4_Operand *srcOpnd = inst->getSrc(i);
      regNum[1][i] = -1;
      if (srcOpnd) {
        if (srcOpnd->isSrcRegRegion() && srcOpnd->asSrcRegRegion()->getBase() &&
            srcOpnd->asSrcRegRegion()->getBase()->isRegVar()) {
          G4_RegVar *baseVar =
              static_cast<G4_RegVar *>(srcOpnd->asSrcRegRegion()->getBase());
          if (baseVar->isGreg()) {
            uint32_t byteAddress = srcOpnd->getLinearizedStart();
            if (byteAddress != 0) {
              regNum[0][i] =
                  byteAddress / parent->builder->numEltPerGRF<Type_UB>();
            } else {
              // before RA, use the value in Greg directly
              regNum[0][i] = baseVar->getPhyReg()->asGreg()->getRegNum();
            }
            regNum[1][i] = regNum[0][i];
            regSrcNum++;
          }
          execSize[i] =
              srcOpnd->getLinearizedEnd() - srcOpnd->getLinearizedStart();
        }
      }
    }
  }

  if (regSrcNum == 3) {
    int maxGRFNum = 0;
    output << " {";
    if (parent->builder->oneGRFBankDivision()) { // EVEN/ODD
      for (int i = 0; i < 3; i++) {
        output << i << "=";
        if (!(regNum[0][i] % 2) && regNum[0][i] < SECOND_HALF_BANK_START_GRF) {
          output << "EL, ";
        }
        if (regNum[0][i] % 2 && regNum[0][i] < SECOND_HALF_BANK_START_GRF) {
          output << "OL, ";
        }
        if (!(regNum[0][i] % 2) && regNum[0][i] >= SECOND_HALF_BANK_START_GRF) {
          output << "EH, ";
        }
        if (regNum[0][i] % 2 && regNum[0][i] >= SECOND_HALF_BANK_START_GRF) {
          output << "OH, ";
        }
      }
    } else { // EVEN EVEN/ODD ODD
      const IR_Builder *builder = parent->builder;
      for (int i = 0; i < 3; i++) {
        output << i << "=";
        for (int j = 0;
             j < (execSize[i] + (int)builder->numEltPerGRF<Type_UB>() - 1) /
                     (int)builder->numEltPerGRF<Type_UB>();
             j++) {
          int reg_num = regNum[0][i] + j;
          if (!(reg_num & 0x02) && reg_num < SECOND_HALF_BANK_START_GRF) {
            output << "EL, ";
          }
          if ((reg_num & 0x02) && reg_num < SECOND_HALF_BANK_START_GRF) {
            output << "OL, ";
          }
          if (!(reg_num & 0x02) && reg_num >= SECOND_HALF_BANK_START_GRF) {
            output << "EH, ";
          }
          if ((reg_num & 0x02) && reg_num >= SECOND_HALF_BANK_START_GRF) {
            output << "OH, ";
          }
          if (j > 1) {
            regNum[1][i] = reg_num;
          }
        }
        maxGRFNum =
            ((execSize[i] + (int)builder->numEltPerGRF<Type_UB>() - 1) /
             (int)builder->numEltPerGRF<Type_UB>()) > maxGRFNum
                ? ((execSize[i] + (int)builder->numEltPerGRF<Type_UB>() - 1) /
                   (int)builder->numEltPerGRF<Type_UB>())
                : maxGRFNum;
      }
    }
    output << "BC=";
    if (!parent->builder->twoSourcesCollision()) {
      if (!parent->builder->oneGRFBankDivision()) { // EVEN EVEN/ODD ODD
        vISA_ASSERT(maxGRFNum < 3, "Not supporting register size > 2");
        if (maxGRFNum == 2) {
          for (int i = 0; i < maxGRFNum; i++) {
            if ((regNum[i][1] & 0x02) == (regNum[i][2] & 0x02)) {
              if ((regNum[i][1] < SECOND_HALF_BANK_START_GRF &&
                   regNum[i][2] < SECOND_HALF_BANK_START_GRF) ||
                  (regNum[i][1] >= SECOND_HALF_BANK_START_GRF &&
                   regNum[i][2] >= SECOND_HALF_BANK_START_GRF)) {
                parent->BCStats.addBad();
                output << "BAD,";
              } else {
                parent->BCStats.addOK();
                output << "OK,";
              }
            } else {
              parent->BCStats.addGood();
              output << "GOOD,";
            }
          }
        } else {
          for (int i = 0; i < maxGRFNum; i++) {
            if (((regNum[i][1] & 0x02) == (regNum[i][2] & 0x02)) &&
                ((regNum[i][0] & 0x02) == (regNum[i][1] & 0x02))) {
              if ((regNum[i][0] < SECOND_HALF_BANK_START_GRF &&
                   regNum[i][1] < SECOND_HALF_BANK_START_GRF &&
                   regNum[i][2] < SECOND_HALF_BANK_START_GRF) ||
                  (regNum[i][0] >= SECOND_HALF_BANK_START_GRF &&
                   regNum[i][1] >= SECOND_HALF_BANK_START_GRF &&
                   regNum[i][2] >= SECOND_HALF_BANK_START_GRF)) {
                parent->BCStats.addBad();
                output << "BAD,";
              } else {
                parent->BCStats.addOK();
                output << "OK,";
              }
            } else {
              parent->BCStats.addGood();
              output << "GOOD,";
            }
          }
        }
      } else { // EVEN/ODD
        if ((regNum[0][1] % 2) != (regNum[0][2] % 2) ||
            (regNum[0][0] % 2) != (regNum[0][1] % 2) ||
            (regNum[0][1] == regNum[0][2])) {
          parent->BCStats.addGood();
          output << "GOOD";
        } else {
          if ((regNum[0][0] < SECOND_HALF_BANK_START_GRF &&
               regNum[0][1] < SECOND_HALF_BANK_START_GRF &&
               regNum[0][2] < SECOND_HALF_BANK_START_GRF) ||
              (regNum[0][0] >= SECOND_HALF_BANK_START_GRF &&
               regNum[0][1] >= SECOND_HALF_BANK_START_GRF &&
               regNum[0][2] >= SECOND_HALF_BANK_START_GRF)) {
            parent->BCStats.addBad();
            output << "BAD";
          } else {
            parent->BCStats.addOK();
            output << "OK";
          }
        }
      }
    } else // Two source
    {      //   EVEN/ODD
      if ((regNum[0][1] != regNum[0][2]) &&
          ((regNum[0][1] % 2) == (regNum[0][2] % 2))) {
        if ((regNum[0][1] < SECOND_HALF_BANK_START_GRF &&
             regNum[0][2] < SECOND_HALF_BANK_START_GRF) ||
            (regNum[0][1] >= SECOND_HALF_BANK_START_GRF &&
             regNum[0][2] >= SECOND_HALF_BANK_START_GRF)) {
          parent->BCStats.addBad();
          output << "BAD";
        } else {
          parent->BCStats.addOK();
          output << "OK";
        }
      } else {
        parent->BCStats.addGood();
        output << "GOOD";
      }
    }
    output << "}";
  }
}

static bool isValidReg(int reg) { return reg != -1; }

static void setInValidReg(int &reg) { reg = -1; }

static int getConflictTimesForTGLLP(std::ostream &output,
                                    int *firstRegCandidate,
                                    int &sameBankConflicts) {
  int conflictTimes = 0;
  int bundles[2][8];
  int bankSrcs[2];

  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 8; j++) {
      bundles[i][j] = -1;
    }
    bankSrcs[i] = 0;
  }

  output << "{";
  for (int i = 0; i < G4_MAX_SRCS; i++) {
    if (isValidReg(firstRegCandidate[i])) {
      int bundleID = (firstRegCandidate[i] % 16) / 2;
      int bankID = firstRegCandidate[i] % 2;

      // Same bank and same bundle
      if (bundles[bankID][bundleID] != -1) {
        conflictTimes++;
      }

      bundles[bankID][bundleID] = i;
      bankSrcs[bankID]++;
      if (bankID == 0) {
        output << "E:";
      } else {
        output << "O:";
      }
      output << bundleID << ",";
    }
  }

  // Same bank but different bundles
  if (conflictTimes == 0 && (bankSrcs[0] > 2 || bankSrcs[1] > 2)) {
    conflictTimes++;
    sameBankConflicts++;
  } else if (bankSrcs[0] > 2 || bankSrcs[1] > 2) {
    sameBankConflicts++;
  }

  output << "}, ";

  return conflictTimes;
}

int G4_BB::getConflictTimesForTGL(std::ostream &output, int *firstRegCandidate,
                                  int &sameBankConflicts, bool zeroOne,
                                  bool isTGLLP, bool reducedBundles) {
  int conflictTimes = 0;
  int bundles[2][16];
  int bankSrcs[2];

  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 16; j++) {
      bundles[i][j] = -1;
    }
    bankSrcs[i] = 0;
  }

  output << "{";
  for (int i = 0; i < G4_MAX_SRCS; i++) {
    bool same_register = false;

    if (isValidReg(firstRegCandidate[i])) {
      for (int j = 0; j < i; j++) {
        if (isValidReg(firstRegCandidate[j]) && j != i) {
          if (firstRegCandidate[j] == firstRegCandidate[i]) {
            same_register = true;
            break;
          }
        }
      }

      if (same_register) {
        continue;
      }

      int bundleID = (firstRegCandidate[i] % 64) / 4;
      int bankID = (firstRegCandidate[i] % 4) / 2;
      if (isTGLLP) {
        bankID = (firstRegCandidate[i]) % 2;
        bundleID = (firstRegCandidate[i] % 16) / 2;
      } else if (zeroOne) {
        bankID = (firstRegCandidate[i]) % 2;
        bundleID = (firstRegCandidate[i] % 32) / 2;
      }

      if (reducedBundles) {
        bundleID = (firstRegCandidate[i] % 16) / 2;
      }
      if (parent->builder->has64bundleSize2GRFPerBank()) {
        bankID = (firstRegCandidate[i] % 4) / 2;
        bundleID = (firstRegCandidate[i] % 32) / 4;
      }
       // Same bank and same bundle
      if (bundles[bankID][bundleID] != -1) {
        conflictTimes++;
      }

      bundles[bankID][bundleID] = i;
      bankSrcs[bankID]++;
      if (bankID == 0) {
        output << "E:";
      } else {
        output << "O:";
      }
      output << bundleID << ",";
    }
  }

  // Same bank but different bundles
  if (conflictTimes == 0 && (bankSrcs[0] > 2 || bankSrcs[1] > 2)) {
    conflictTimes++;
    sameBankConflicts++;
  } else if (bankSrcs[0] > 2 || bankSrcs[1] > 2) {
    sameBankConflicts++;
  }

  output << "}, ";

  return conflictTimes;
}

/*
 * Xe BC evaluation
 * All read suppression is GRF granularity based.
 * Read suppression only happens between or within a physical instruction not
 * compressed one. Compressed one will be split into physical instructions. Read
 * suppression between instructions: The read suppression mechanism is used to
 * save the GRF register reading operations with a register cache in HW. The
 * suppression we talked here is the suppression between instructions. For each
 * source operand slot, HW provide a GRF cache. With the cache, if the same GRF
 * will be read in the instruction, the read will not happen, the cached value
 * will be used directly. Note that:
 *     1. Inter read suppression is the suppression cache based.
 *     2. For compressed instructino 2 GRFs read suppression for src1 for DF and
 * F type operands and 1 GRF read suppression for src0 and src2.
 *     3. The slot cache will be flushed if the buffered register is used as
 * destination operand.
 *
 * Read suppression within a instruction:
 *     1. Works for all source operands.
 *     2. intra suppression is the GRF read operation based(no read no
 * suppression).
 */
uint32_t G4_BB::emitBankConflictXe(std::ostream &os_output, const G4_INST *inst,
                                   int *suppressRegs, int &sameConflictTimes,
                                   int &twoSrcConflicts, int &simd16RS,
                                   bool zeroOne, bool isTGLLP,
                                   bool hasReducedBundles) {
  std::stringstream output;

  if (inst->isSend() || inst->isMath() || inst->isSWSBSync() ||
      inst->isWait() || inst->isReturn() || inst->isCall()) { // Flush
    for (int i = 0; i < 4; i++) {
      setInValidReg(suppressRegs[i]);
    }
    return 0;
  }

  int currInstRegs[2][G4_MAX_SRCS];
  int readRegs[2][G4_MAX_SRCS];
  int currInstExecSize[G4_MAX_SRCS] = {0};
  int firstRegCandidate[G4_MAX_SRCS];
  int secondRegCandidate[G4_MAX_SRCS];
  int candidateNum = 0;
  int dstExecSize = 0;
  int dstRegs[2];

  for (int i = 0; i < G4_MAX_SRCS; i++) {
    setInValidReg(firstRegCandidate[i]);
    setInValidReg(secondRegCandidate[i]);
    setInValidReg(currInstRegs[0][i]);
    setInValidReg(currInstRegs[1][i]);
    setInValidReg(readRegs[0][i]);
    setInValidReg(readRegs[1][i]);
  }
  setInValidReg(dstRegs[0]);
  setInValidReg(dstRegs[1]);

  bool isCompressedInst = false;
  bool isLastInstCompressed = suppressRegs[4] == 1;
  bool isFDFSrc1 = false;
  bool isSrc1Suppressed = false;

  const IR_Builder *builder = parent->builder;
  auto grfSize = builder->getGRFSize();
  // Get Dst
  G4_DstRegRegion *dstOpnd = inst->getDst();
  if (dstOpnd && !dstOpnd->isIndirect() && dstOpnd->isGreg()) {
    dstExecSize =
        dstOpnd->getLinearizedEnd() - dstOpnd->getLinearizedStart() + 1;
    uint32_t byteAddress = dstOpnd->getLinearizedStart();
    dstRegs[0] = byteAddress / builder->numEltPerGRF<Type_UB>();
    if (dstExecSize > grfSize) {
      dstRegs[1] = dstRegs[0] +
                   (dstExecSize + builder->numEltPerGRF<Type_UB>() - 1) /
                       builder->numEltPerGRF<Type_UB>() -
                   1;
      isCompressedInst = true;
    }
  }

  // Get src
  for (int i = 0; i < inst->getNumSrc(); i++) {
    setInValidReg(currInstRegs[0][i]);
    setInValidReg(currInstRegs[1][i]);
    G4_Operand *srcOpnd = inst->getSrc(i);
    if (srcOpnd) {
      if (srcOpnd->isSrcRegRegion() && srcOpnd->asSrcRegRegion()->getBase() &&
          srcOpnd->asSrcRegRegion()->getBase()->isRegVar()) {
        G4_RegVar *baseVar =
            static_cast<G4_RegVar *>(srcOpnd->asSrcRegRegion()->getBase());
        currInstExecSize[i] =
            srcOpnd->getLinearizedEnd() - srcOpnd->getLinearizedStart() + 1;
        if (baseVar->isGreg()) {
          uint32_t byteAddress = srcOpnd->getLinearizedStart();
          currInstRegs[0][i] = byteAddress / builder->numEltPerGRF<Type_UB>();
          if (i == 1) {
            isFDFSrc1 = IS_TYPE_F32_F64(srcOpnd->getType());
          }
          if (currInstExecSize[i] > grfSize) {
            currInstRegs[1][i] = currInstRegs[0][i] + 1;
            isCompressedInst = true;
          } else // Read suppression will be handled later
          {
            currInstRegs[1][i] = currInstRegs[0][i];
          }
        }
      }
    }
  }

  // Kill previous read suppression candiadte if it wrote in DST
  if (isValidReg(dstRegs[0])) {
    for (int i = 0; i < 4; i++) {
      if (suppressRegs[i] == dstRegs[0]) {
        setInValidReg(suppressRegs[i]);
      }
    }
  }

  // Read Suppression from previous instruction
  // Keep suppressRegs, if suppression happen
  // Update suppression, and registers to be read.
  //  inst1: mad(8)   r10, r20, r20, r40
  //  inst2: mad(8)   r10, r30, r20, r50
  //  the suppression of r20 inst2 will happen
  output << " R{";
  for (int i = 0; i < 3; i++) {
    // Read suppression for src0, src1 and src2
    if (isValidReg(suppressRegs[i]) && currInstRegs[0][i] == suppressRegs[i]) {
      setInValidReg(currInstRegs[0][i]);
      if (isCompressedInst &&
          isLastInstCompressed && // Two GRF operand instructions
          isFDFSrc1 && i == 1) {
        setInValidReg(currInstRegs[1][i]);
        isSrc1Suppressed = true;
      }
      output << "r" << suppressRegs[i] << ",";
    } else {
      suppressRegs[i] = currInstRegs[0][i];
    }
  }
  output << "}";

  // Intra suppression for the first GRF
  // Inter and intra will happen only once, if inter happen, intra wouldn't read
  // Such as in following case, the src1 r20 of inst2 need be read because src0
  // r20 of inst2 is suppressed
  //  inst1: mad(8)   r10, r20, r30, r40
  //  inst2: mad(8)   r10, r20, r20, r50
  //  for this case, currInstRegs[0][j] is updated to invalid in this case
  //  because inter is handled first.
  output << " IR{";
  for (int i = 0; i < inst->getNumSrc(); i++) {
    if (isValidReg(currInstRegs[0][i])) {
      for (int k = 0; k < G4_MAX_SRCS; k++) {
        if (isValidReg(readRegs[0][k]) &&
            readRegs[0][k] == currInstRegs[0][i]) {
          setInValidReg(currInstRegs[0][i]);
          output << "r" << readRegs[0][k] << ",";
        }
      }
      readRegs[0][i] = currInstRegs[0][i];
    }
  }
  output << "}";

  suppressRegs[4] = isCompressedInst ? 1 : 0;

  int conflictTimes = 0;
  for (int i = 0; i < 3; i++) {
    if (isValidReg(currInstRegs[0][i])) {
      firstRegCandidate[candidateNum] = currInstRegs[0][i];
      candidateNum++;
    }
  }

  // Get the bank conflict for the first GRF instruction.
  if (candidateNum > 1) {
    conflictTimes =
        getConflictTimesForTGL(output, firstRegCandidate, sameConflictTimes,
                               zeroOne, isTGLLP, hasReducedBundles);
    if (candidateNum == 2) {
      twoSrcConflicts += conflictTimes;
    }
  }

  if (isCompressedInst) {
    if (isValidReg(dstRegs[1])) {
      for (int i = 0; i < 4; i++) {
        if (suppressRegs[i] == dstRegs[1]) {
          // Should be no real overlap, only GRF level overlap may happen
          setInValidReg(suppressRegs[i]);
        }
      }
    }

    output << " R{";
    // Inter for the second instruction
    for (int i = 0; i < 3; i++) {
      if (isSrc1Suppressed) {
        continue;
      }
      // Read suppression for src0, src1 and src2
      if (isValidReg(suppressRegs[i]) &&
          currInstRegs[1][i] == suppressRegs[i]) {
        setInValidReg(currInstRegs[1][i]);
        output << "r" << suppressRegs[i] << ",";
      } else {
        suppressRegs[i] = currInstRegs[1][i];
      }
    }
    output << "}";

    output << " IR{";
    // Intra suppression for the second instruction
    for (int i = 0; i < inst->getNumSrc(); i++) {
      if (isValidReg(currInstRegs[1][i])) {
        for (int k = 0; k < G4_MAX_SRCS; k++) {
          if (isValidReg(readRegs[1][k]) &&
              readRegs[1][k] == currInstRegs[1][i]) {
            setInValidReg(currInstRegs[1][i]);
            output << "r" << readRegs[1][k] << ",";
          }
        }
        readRegs[1][i] = currInstRegs[1][i];
      }
    }
    output << "}";

    candidateNum = 0;
    // For SIMD8, if any GRF0 of src1 or src2 of inst1 is GRF register
    for (int i = 0; i < 3; i++) {
      if (isValidReg(currInstRegs[1][i])) {
        secondRegCandidate[candidateNum] = currInstRegs[1][i];
        candidateNum++;
      }
    }

    if (candidateNum > 1) {
      int c = 0;
      c = getConflictTimesForTGL(output, secondRegCandidate, sameConflictTimes,
                                 zeroOne, isTGLLP, false);
      conflictTimes += c;
      if (candidateNum == 2) {
        twoSrcConflicts += c;
      }
      if (currInstExecSize[0] <= 16 || currInstExecSize[1] <= 16 ||
          currInstExecSize[2] <= 16) {
        simd16RS += c;
      }
    }
  }

  if (conflictTimes != 0 || parent->builder->getOption(vISA_DumpAllBCInfo)) {
    output << " {";
    output << "BC=";
    output << conflictTimes;
    output << "}";
    os_output << output.str();
  }

  return conflictTimes;
} // emitBankConflictXe

static bool hasInternalConflict(IR_Builder *builder, int reg1, int reg2) {
  int bundleID1 = (reg1 % 16) / 2;
  int bankID1 = reg1 % 2;
  int bundleID2 = (reg2 % 16) / 2;
  int bankID2 = reg2 % 2;

  if (builder->hasTwoGRFBank16Bundles()) {
    bundleID1 = (reg1 % 64) / 4;
    bankID1 = (reg1 % 4) / 2;
    bundleID2 = (reg2 % 64) / 4;
    bankID2 = (reg2 % 4) / 2;
  }

  if (builder->has64bundleSize2GRFPerBank()) {
    bundleID1 = (reg1 % 32) / 4;
    bankID1 = (reg1 % 4) / 2;
    bundleID2 = (reg2 % 32) / 4;
    bankID2 = (reg2 % 4) / 2;
  }

  if (builder->hasOneGRFBank16Bundles()) {
    bundleID1 = (reg1 % 64) / 4;
    bankID1 = reg1 % 2;
    bundleID2 = (reg2 % 64) / 4;
    bankID2 = reg2 % 2;
  }

  if (builder->has64bundleSize()) {
    bundleID1 = (reg1 % 16) / 2;
    bankID1 = reg1 % 2;
    bundleID2 = (reg2 % 16) / 2;
    bankID2 = reg2 % 2;
  }

  return ((bankID1 == bankID2) && (bundleID1 == bundleID2));
}

/*
 * In XeLP, there are 8 bundles and 2 banks per HW thread.
 * Banks are divided according to EVEN / ODD of register index: 0101010101010101
 * There are 8 bundles per 16 registers : 0011223344556677
 * For two adjacent instructions : inst1 and inst2, inst1_src1(, inst1_src2) and
 *inst2_src0 will be read in same cycle Considered HW swapand read suppresion
 *mechanisms HW swap : The origional GRF register reading sequence for a three
 *source instruction is : src0 in cycle0and src1and src2 in cycle2. HW swap
 *mechanism detects the conflict between src1and src2, if there is a conflict,
 *HW will read src1 in cycle0and src0and src2 in cycle1. Note that :
 * 1. for SIMD16, HW swap only happens when detecting conflicts in first simd8's
 *registers. conflict in second simd8 will not trigger swap.
 * 2. for SIMD16, when swapping happens, the src1and src0 of both simd8
 *instructions will be swapped.
 */
uint32_t G4_BB::emitBankConflictXeLP(std::ostream &os_output,
                                     const G4_INST *inst, int *suppressRegs,
                                     int *lastRegs, int &sameConflictTimes,
                                     int &twoSrcConflicts, int &simd16RS) {
  std::stringstream output;

  if (inst->isSend() || inst->isMath() || inst->isSWSBSync() ||
      inst->isWait() || inst->isReturn() || inst->isCall()) { // Flush
    for (int i = 0; i < 3; i++) {
      setInValidReg(suppressRegs[i]);
      setInValidReg(lastRegs[i]);
    }
    return 0;
  }

  int currInstRegs[2][G4_MAX_SRCS];
  int currInstExecSize[G4_MAX_SRCS] = {0};
  int firstRegCandidate[G4_MAX_SRCS];
  int secondRegCandidate[G4_MAX_SRCS];
  int candidateNum = 0;
  int dstExecSize = 0;
  int dstRegs[2];

  for (int i = 0; i < G4_MAX_SRCS; i++) {
    setInValidReg(firstRegCandidate[i]);
    setInValidReg(secondRegCandidate[i]);
    setInValidReg(currInstRegs[0][i]);
    setInValidReg(currInstRegs[1][i]);
  }
  setInValidReg(dstRegs[0]);
  setInValidReg(dstRegs[1]);

  bool conflictWithPrevInst = true;
  if (!isValidReg(lastRegs[1]) && !isValidReg(lastRegs[2])) {
    conflictWithPrevInst = false;
  }

  // Get the regsiters of previous instruction
  // If there is potentail to conflict with it
  if (conflictWithPrevInst) {
    if (isValidReg(lastRegs[1])) {
      firstRegCandidate[candidateNum] = lastRegs[1];
      candidateNum++;
    }
    if (isValidReg(lastRegs[2])) {
      firstRegCandidate[candidateNum] = lastRegs[2];
      candidateNum++;
    }
  }

  bool instSplit = false;
  const IR_Builder *builder = parent->builder;
  auto grfSize = builder->getGRFSize();

  // Get Dst
  G4_DstRegRegion *dstOpnd = inst->getDst();
  if (dstOpnd && !dstOpnd->isIndirect() && dstOpnd->isGreg()) {
    dstExecSize =
        dstOpnd->getLinearizedEnd() - dstOpnd->getLinearizedStart() + 1;
    uint32_t byteAddress = dstOpnd->getLinearizedStart();
    dstRegs[0] = byteAddress / builder->numEltPerGRF<Type_UB>();
    if (dstExecSize > grfSize) {
      dstRegs[1] = dstRegs[0] +
                   (dstExecSize + builder->numEltPerGRF<Type_UB>() - 1) /
                       builder->numEltPerGRF<Type_UB>() -
                   1;
      instSplit = true;
    }
  }

  for (int i = 0; i < inst->getNumSrc(); i++) {
    setInValidReg(currInstRegs[0][i]);
    setInValidReg(currInstRegs[1][i]);
    G4_Operand *srcOpnd = inst->getSrc(i);
    if (srcOpnd) {
      if (srcOpnd->isSrcRegRegion() && srcOpnd->asSrcRegRegion()->getBase() &&
          srcOpnd->asSrcRegRegion()->getBase()->isRegVar()) {
        G4_RegVar *baseVar =
            static_cast<G4_RegVar *>(srcOpnd->asSrcRegRegion()->getBase());
        currInstExecSize[i] =
            srcOpnd->getLinearizedEnd() - srcOpnd->getLinearizedStart() + 1;
        if (baseVar->isGreg()) {
          uint32_t byteAddress = srcOpnd->getLinearizedStart();
          currInstRegs[0][i] = byteAddress / builder->numEltPerGRF<Type_UB>();

          if (currInstExecSize[i] > grfSize) {
            currInstRegs[1][i] =
                currInstRegs[0][i] +
                (currInstExecSize[i] + builder->numEltPerGRF<Type_UB>() - 1) /
                    builder->numEltPerGRF<Type_UB>() -
                1;
            instSplit = true;
          } else if (srcOpnd->asSrcRegRegion()
                         ->isScalar()) // No Read suppression for SIMD 16/scalar
                                       // src
          {
            currInstRegs[1][i] = currInstRegs[0][i];
          } else {
            setInValidReg(currInstRegs[1][i]);
          }
        }
      }
    }
  }

  // Read Suppression for current instruction
  output << " R{";
  for (int i = 1; i < 3; i++) {
    if (isValidReg(suppressRegs[i]) && currInstRegs[0][i] == suppressRegs[i]) {
      setInValidReg(currInstRegs[0][i]);
      output << "r" << suppressRegs[i] << ",";
    } else {
      suppressRegs[i] = currInstRegs[0][i];
    }
  }
  output << "}";

  // Kill all previous read suppression candiadte if it wrote in DST
  if (isValidReg(dstRegs[0])) {
    for (int i = 1; i < 3; i++) {
      if (suppressRegs[i] == dstRegs[0]) {
        setInValidReg(suppressRegs[i]);
      }
    }
  }

  bool swap = false;
  // SWAP: has lower proirity than read suppression
  // For SIMD16, the SWAP is triggered by first register, but the second one
  // will be swapped as well
  if (isValidReg(currInstRegs[0][0]) && isValidReg(currInstRegs[0][1]) &&
      isValidReg(currInstRegs[0][2]) &&
      hasInternalConflict(parent->builder, currInstRegs[0][1],
                          currInstRegs[0][2])) {
    int tmpReg = currInstRegs[0][1];
    currInstRegs[0][1] = currInstRegs[0][0];
    currInstRegs[0][0] = tmpReg;
    output << " S{r" << currInstRegs[0][1] << ", r" << currInstRegs[0][0]
           << "} ";
    swap = true;
  }

  // No suppression, update the suppressRegs[0] for XeLP
  // suppressRegs[1], suppressRegs[2] will be updated with next instruction

  // src1 and src2 will be read with src0 of next instruction
  lastRegs[1] = currInstRegs[0][1];
  lastRegs[2] = currInstRegs[0][2];
  // Conflict with previous instruction
  int conflictTimes = 0;
  if (conflictWithPrevInst) {

    if (isValidReg(currInstRegs[0][0])) {
      firstRegCandidate[candidateNum] = currInstRegs[0][0];
      candidateNum++;
    }
    if (candidateNum > 1) {
      conflictTimes = getConflictTimesForTGLLP(output, firstRegCandidate,
                                               sameConflictTimes);
      if (candidateNum == 2) {
        twoSrcConflicts += conflictTimes;
      }
    }
  }

  if (instSplit) {
    output << " R{";
    for (int i = 1; i < 3; i++) {
      if (isValidReg(suppressRegs[i]) &&
          currInstRegs[1][i] == suppressRegs[i]) {
        setInValidReg(currInstRegs[1][i]);
        output << "r" << suppressRegs[i] << ",";
      } else {
        suppressRegs[i] = currInstRegs[1][i];
      }
    }
    output << "}";

    if (isValidReg(dstRegs[1])) {
      for (int i = 1; i < 3; i++) {
        if (suppressRegs[i] == dstRegs[1]) {
          setInValidReg(suppressRegs[i]);
        }
      }
    }

    if (swap && isValidReg(currInstRegs[1][0]) &&
        isValidReg(currInstRegs[1][1]) && isValidReg(currInstRegs[1][2])) {
      int tmpReg = currInstRegs[1][0];
      currInstRegs[1][0] = currInstRegs[1][1];
      currInstRegs[1][1] = tmpReg;
      output << " S{r" << currInstRegs[1][1] << ", r" << currInstRegs[1][0]
             << "} ";
    }

    candidateNum = 0;
    // For SIMD8, if any GRF0 of src1 or src2 of inst1 is GRF register
    if (isValidReg(lastRegs[1])) // && lastRegs[1] != suppressRegs[1])
    {
      secondRegCandidate[candidateNum] = lastRegs[1];
      candidateNum++;
    }
    if (isValidReg(lastRegs[2])) // && lastRegs[2] != suppressRegs[2])
    {
      secondRegCandidate[candidateNum] = lastRegs[2];
      candidateNum++;
    }

    if (isValidReg(currInstRegs[1][0])) {
      secondRegCandidate[candidateNum] = currInstRegs[1][0];
      candidateNum++;
    }

    lastRegs[1] = currInstRegs[1][1];
    lastRegs[2] = currInstRegs[1][2];

    if (candidateNum > 1) {
      int c = 0;
      c = getConflictTimesForTGLLP(output, secondRegCandidate,
                                   sameConflictTimes);
      conflictTimes += c;
      if (candidateNum == 2) {
        twoSrcConflicts += c;
      }
      if (currInstExecSize[0] <= 16 || currInstExecSize[1] <= 16 ||
          currInstExecSize[2] <= 16) {
        simd16RS += c;
      }
    }
  }

  if (conflictTimes != 0) {
    output << " {";
    output << "BC=";
    output << conflictTimes;
    output << "}";
    os_output << output.str();
  }

  return conflictTimes;
} // emitBankConflictXeLP

uint32_t G4_BB::countReadModifyWrite(std::ostream &output,
                                     const G4_INST *inst) {
  if (!inst->getDst() || inst->getDst()->isNullReg() || inst->isSend() ||
      inst->isDpas()) {
    return 0;
  }
  auto dst = inst->getDst();
  auto dstTy = dst->getType();
  if (TypeSize(dstTy) == 1 && dst->getHorzStride() > 1) {
    return 1;
  }
  return 0;
}

G4_Label *G4_BB::getLabel() {
  // FIXME: For now not all BBs will start with a label (e.g.,
  // a block that follows a call).  We should fix it by getting rid
  // of the g4_label instruction and associate each label with a BB
  if (instList.size() > 0 && instList.front()->isLabel()) {
    return instList.front()->getLabel();
  }
  return NULL;
}

G4_INST *G4_BB::getFirstInst() {
  G4_INST *firstInst = nullptr;
  if (instList.size() > 0) {
    INST_LIST_ITER I = instList.begin();
    firstInst = *I;
    if (firstInst->isLabel()) {
      // Only first inst can be label.
      ++I;
      firstInst = (I != instList.end()) ? *I : nullptr;
    }
  }
  return firstInst;
}

INST_LIST_ITER G4_BB::getFirstInsertPos() {
  INST_LIST_ITER II = begin();
  for (INST_LIST_ITER IB = end(); II != IB; ++II) {
    G4_INST *tI = (*II);
    if (tI->isLabel() || tI->opcode() == G4_join || tI->opcode() == G4_endif ||
        tI->opcode() == G4_while) {
      continue;
    }
    break;
  }
  return II;
}


//
//  Add an EOT send to the end of this BB.
//
void G4_BB::addEOTSend(G4_INST *lastInst) {

  // mov (8) r1.0<1>:ud r0.0<8;8,1>:ud {NoMask}
  // send (1) null r1 0x27 desc
  IR_Builder *builder = parent->builder;
  G4_Declare *dcl =
      builder->createSendPayloadDcl(builder->numEltPerGRF<Type_UD>(), Type_UD);
  G4_DstRegRegion *movDst = builder->createDstRegRegion(dcl, 1);
  G4_SrcRegRegion *r0Src = builder->createSrcRegRegion(
      builder->getBuiltinR0(), builder->getRegionStride1());
  G4_INST *movInst =
      builder->createMov(G4_ExecSize(builder->numEltPerGRF<Type_UD>()), movDst,
                         r0Src, InstOpt_WriteEnable, false);
  if (lastInst) {
    movInst->inheritDIFrom(lastInst);
  }
  instList.push_back(movInst);

  auto EOT_SFID = builder->getEOTSFID();

  int exdesc = (0x1 << 5) + SFIDtoInt(EOT_SFID);
  // response len = 0, msg len = 1
  int desc = (0x1 << 25) + (0x1 << 4);

  G4_SrcRegRegion *sendSrc =
      builder->createSrcRegRegion(dcl, builder->getRegionStride1());

  G4_DstRegRegion *sendDst = builder->createNullDst(Type_UD);

  auto msgDesc =
      builder->createGeneralMsgDesc(desc, exdesc, SendAccess::WRITE_ONLY);
  G4_InstSend *sendInst = builder->createSendInst(
      NULL, G4_send, g4::SIMD1, sendDst, sendSrc,
      builder->createImm(desc, Type_UD), InstOpt_WriteEnable, msgDesc, false);
  sendInst->setEOT();
  sendInst->inheritDIFrom(movInst);
  instList.push_back(sendInst);

  if (builder->getHasNullReturnSampler() &&
      VISA_WA_CHECK(builder->getPWaTable(), Wa_1607871015)) {
    addSamplerFlushBeforeEOT();
  }
}

std::string G4_BB::getBBTypeStr() const {
  std::stringstream ss;
  bool isFirst = true;
  auto addTyString = [&ss, &isFirst](const char *aS) {
    if (!isFirst) {
      ss << ",";
    }
    isFirst = false;
    ss << aS;
  };

  uint32_t bbTy = getBBType();
  if (bbTy == 0)
    return " ";

  if ((bbTy & G4_BB_CALL_TYPE) != 0)
    addTyString("CALL");
  if ((bbTy & G4_BB_RETURN_TYPE) != 0)
    addTyString("RETURN");
  if ((bbTy & G4_BB_INIT_TYPE) != 0)
    addTyString("INIT");
  if ((bbTy & G4_BB_EXIT_TYPE) != 0)
    addTyString("EXIT");
  if ((bbTy & G4_BB_FCALL_TYPE) != 0)
    addTyString("FCALL");
  if ((bbTy & G4_BB_NM_WA_TYPE) != 0)
    addTyString("NoMaskWA");
  if ((bbTy & G4_BB_KEEP_TYPE) != 0)
    addTyString("KEEP");
  return ss.str();
}

void G4_BB::dump() const { print(std::cerr); }

void G4_BB::emitBbInfo(std::ostream &os) const {
  // mustn't exceed a single line because it could be in asm output
  auto fmtBbId = [&](int bb) {
    std::stringstream ss;
    ss << "B" << std::setw(3) << std::setfill('0') << bb;
    return ss.str();
  };
  os << fmtBbId(getId()) << ":";
  bool first = true;
  auto maybeComma = [&]() {
    if (first)
      first = false;
    else
      os << ", ";
  };
  if (getBBType()) {
    maybeComma();
    os << " [" << getBBTypeStr() << "]";
  }
  if (isDivergent()) {
    maybeComma();
    os << " [inDivergent]";
  }
  auto emitBbSet = [&](const char *name, const BB_LIST &bbl) {
    maybeComma();
    os << " " << name << ":{";
    bool first = true;
    for (const auto &bb : bbl) {
      if (first)
        first = false;
      else
        os << ", ";
      os << fmtBbId(bb->getId());
    }
    os << "}";
  };
  emitBbSet("Preds", Preds);
  emitBbSet("Succs", Succs);
}

void G4_BB::print(std::ostream &OS) const {
  emitBbInfo(OS);
  OS << "\n";
  for (auto &x : instList)
    x->print(OS);
  OS << "\n";
}

void G4_BB::dumpDefUse(std::ostream &os) const {
  for (auto &x : instList) {
    x->dump();
    if (x->def_size() > 0 || x->use_size() > 0) {
      x->dumpDefUse(os);
      os << "\n\n\n";
    }
  }
}

void G4_BB::resetLocalIds() {
  int i = 0;

  for (INST_LIST_ITER iter = instList.begin(), end = instList.end();
       iter != end; ++iter, ++i) {
    (*iter)->setLocalId(i);
  }
}

void G4_BB::removeIntrinsics(Intrinsic intrinId) {
  instList.remove_if([=](G4_INST *inst) {
    return inst->isIntrinsic() &&
           inst->asIntrinsicInst()->getIntrinsicId() == intrinId;
  });
}

void G4_BB::removeIntrinsics(std::vector<Intrinsic>& intrinIdVec) {
  instList.remove_if([=](G4_INST *inst) {
    return inst->isIntrinsic() &&
           std::find(intrinIdVec.begin(), intrinIdVec.end(),
               inst->asIntrinsicInst()->getIntrinsicId()) != intrinIdVec.end();
  });
}

// Add two sampler cache flushes before the EOT send.
// sampler cache flush 1 must have null return
// sampler cache flush 2 must have valid return
// bb must end with an EOT send
void G4_BB::addSamplerFlushBeforeEOT() {
  vISA_ASSERT(isLastInstEOT(), "last instruction must be EOT");
  auto builder = parent->builder;
  int samplerFlushOpcode = 0x1F;
  int samplerFlushFC =
      (SamplerSIMDMode::SIMD32 << 17) + (samplerFlushOpcode << 12);
  // null return version
  {
    int desc = G4_SendDescRaw::createDesc(samplerFlushFC, true, 1, 0);
    G4_SrcRegRegion *sendMsgOpnd = builder->createSrcRegRegion(
        builder->getBuiltinR0(), builder->getRegionStride1());

    auto msgDesc = builder->createSyncMsgDesc(SFID::SAMPLER, desc);
    G4_INST *samplerFlushInst = builder->createSendInst(
        nullptr, G4_send, g4::SIMD8, builder->createNullDst(Type_UD),
        sendMsgOpnd, builder->createImm(desc, Type_UD), 0, msgDesc, true);
    auto iter = std::prev(end());
    insert(iter, samplerFlushInst);
  }

  // valid return version
  {
    int desc = G4_SendDescRaw::createDesc(samplerFlushFC, true, 1, 1);
    G4_SrcRegRegion *sendMsgOpnd = builder->createSrcRegRegion(
        builder->getBuiltinR0(), builder->getRegionStride1());
    G4_Declare *tmpDest =
        builder->createTempVar(g4::SIMD8, Type_UD, builder->getGRFAlign());
    tmpDest->setDoNotSpill();
    G4_DstRegRegion *sendMsgDst = builder->createDstRegRegion(tmpDest, 1);
    auto msgDesc = builder->createSyncMsgDesc(SFID::SAMPLER, desc);
    G4_INST *samplerFlushInst = builder->createSendInst(
        nullptr, G4_send, g4::SIMD8, sendMsgDst, sendMsgOpnd,
        builder->createImm(desc, Type_UD), 0, msgDesc, true);
    auto iter = std::prev(end());
    insert(iter, samplerFlushInst);
  }
}

bool G4_BB::dominates(G4_BB *other) {
  return getParent().getImmDominator().dominates(this, other);
}

void G4_BB::markEmpty(IR_Builder *IRB, const std::string str) {
  vISA_ASSERT(empty(), "BB to be marked empty is not empty!");
  G4_Label *label =
      IRB->createLocalBlockLabel(str.empty() ? "LABEL__EMPTYBB" : str);
  G4_INST *inst = IRB->createLabelInst(label, false);
  push_back(inst);
  specialEmptyBB = true;
}
