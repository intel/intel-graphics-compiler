/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "LocalScheduler_G4IR.h"
#include "../G4_Opcode.h"
#include "../PointsToAnalysis.h"
#include "../Timer.h"
#include "Dependencies_G4IR.h"
#include "visa_wa.h"
#include "../KernelCost.hpp"

#include <fstream>
#include <functional>
#include <queue>
#include <sstream>
#include <vector>

using namespace vISA;

/* Entry to the local scheduling. */
void LocalScheduler::localScheduling() {
  // This is controlled by options for debugging
  if (!fg.getKernel()->isLocalSheduleable())
    return;

  VISA_DEBUG_VERBOSE(std::cout << "[Scheduling]: Starting...");
  vISA_ASSERT(!fg.empty(), ERROR_SCHEDULER);

  std::vector<VISA_BB_INFO> bbInfo;
  bbInfo.reserve(fg.size());

  const Options *options = fg.builder->getOptions();
  auto LT = LatencyTable::createLatencyTable(*fg.builder);

  PointsToAnalysis p(fg.getKernel()->Declares, fg.size());
  p.doPointsToAnalysis(fg);

  uint32_t totalCycles = 0;
  uint32_t scheduleStartBBId =
      options->getuInt32Option(vISA_LocalSchedulingStartBB);
  uint32_t shceduleEndBBId =
      options->getuInt32Option(vISA_LocalSchedulingEndBB);
  for (G4_BB *bb : fg) {
    if (bb->getId() < scheduleStartBBId || bb->getId() > shceduleEndBBId)
      continue;

    unsigned instCountBefore = (uint32_t)bb->size();
#define SCH_THRESHOLD 2
    if (instCountBefore < SCH_THRESHOLD) {
      unsigned int sequentialCycles = 0;
      for (G4_INST *inst : *bb)
        sequentialCycles += LT->getOccupancy(inst);

      bbInfo.push_back({(int)bb->getId(), sequentialCycles, 0,
          (unsigned char)bb->getNestLevel()});
      totalCycles += sequentialCycles;
      continue;
    }

    unsigned schedulerWindowSize =
        options->getuInt32Option(vISA_SchedulerWindowSize);
    if (schedulerWindowSize > 0 && instCountBefore > schedulerWindowSize) {
      // If BB has a lot of instructions then when recursively
      // traversing DAG in list scheduler, stack overflow occurs.
      // So artificially breakup inst list here to reduce size
      // of scheduler problem size.
      unsigned int count = 0;
      unsigned int sequentialCycles = 0;
      unsigned int sendStallCycles = 0;
      std::vector<G4_BB *> sections;

      for (auto inst_it = bb->begin(); ; ++inst_it) {
        if (count == schedulerWindowSize || inst_it == bb->end()) {
          G4_BB *tempBB = fg.createNewBB(false);
          sections.push_back(tempBB);
          tempBB->splice(tempBB->begin(), bb, bb->begin(), inst_it);
          G4_BB_Schedule schedule(fg.getKernel(), tempBB, *LT, p);
          sequentialCycles += schedule.sequentialCycle;
          sendStallCycles += schedule.sendStallCycle;
          count = 0;
        }
        count++;

        if (inst_it == bb->end())
          break;
      }

      for (G4_BB *section : sections) {
        bb->splice(bb->end(), section, section->begin(), section->end());
      }
      bbInfo.push_back({(int)bb->getId(), sequentialCycles, sendStallCycles,
          (unsigned char)bb->getNestLevel()});
      totalCycles += sequentialCycles;
    } else {
      G4_BB_Schedule schedule(fg.getKernel(), bb, *LT, p);
      bbInfo.push_back({(int)bb->getId(), schedule.sequentialCycle,
          schedule.sendStallCycle, (unsigned char)bb->getNestLevel()});
      totalCycles += schedule.sequentialCycle;
    }
  }

  // Sum up the cycles for each BB.
  unsigned sendStallCycle = 0;
  unsigned staticCycle = 0;
  unsigned loopNestedStallCycle = 0;
  unsigned loopNestedCycle = 0;
  for (auto &bbStat : bbInfo) {
    sendStallCycle += bbStat.sendStallCycle;
    staticCycle += bbStat.staticCycle;
    // Expect that a loop runs 16 iterations.
    auto nestingfactor = (bbStat.loopNestLevel * 4);
    loopNestedStallCycle += (bbStat.sendStallCycle << nestingfactor);
    loopNestedCycle += (bbStat.staticCycle << nestingfactor);
  }

  FINALIZER_INFO *jitInfo = fg.builder->getJitInfo();
  jitInfo->stats.sendStallCycle = sendStallCycle;
  jitInfo->stats.staticCycle = staticCycle;
  jitInfo->stats.loopNestedStallCycle = loopNestedStallCycle;
  jitInfo->stats.loopNestedCycle = loopNestedCycle;
  jitInfo->stats.numCycles = totalCycles;

  if (options->getOption(vISA_KernelCostInfo)) {
    collectKernelCostInfo(fg.getKernel(), bbInfo);
  }
}

void G4_BB_Schedule::dumpSchedule(G4_BB *bb) {
  const char *asmName = nullptr;
  getOptions()->getOption(VISA_AsmFileName, asmName);
  std::string dumpFileName =
      std::string(asmName) + ".bb" + std::to_string(bb->getId()) + ".schedule";
  std::ofstream ofile(dumpFileName, std::ios::out);
  auto nodeIT = scheduledNodes.begin();
  Node *finalNode = scheduledNodes.back();
  int lastBusyCycle = 0;
  int GAP = 4;
  for (int cycle = 0,
           cycle_e = finalNode->schedTime + finalNode->getOccupancy();
       cycle != cycle_e; ++cycle) {
    if (cycle == (*nodeIT)->schedTime) {
      int latency = (*nodeIT)->getOccupancy();
      vASSERT(latency > 0);
      int externCycle = cycle;
      for (int cy_e = cycle + latency; cycle != cy_e; ++cycle) {
        lastBusyCycle = cycle;
        ofile << std::setw(4) << cycle << " ";
        for (G4_INST *inst : (*nodeIT)->instVec) {
          ofile << std::setw(5) << G4_Inst_Table[inst->opcode()].str;
        }
        if (externCycle == cycle) {
          ofile << "[" << (*nodeIT)->nodeID << "]";
          const std::list<G4_INST *> *instrs = (*nodeIT)->getInstructions();
          if (instrs->empty()) {
            ofile << "I" << 0;
          } else {
            for (G4_INST *instr : *instrs) {
              ofile << "I" << instr->getLocalId();
            }
          }
          ofile << "L" << (*nodeIT)->getOccupancy() << "E"
                << (*nodeIT)->getEarliest() << "P" << (*nodeIT)->priority
                << " ";
          for (G4_INST *instr : *instrs) {
            ofile << *instr << ", ";
          }
        } else {
          ofile << " ... ";
        }
        ofile << "\n";
      }
      cycle -= 1; // Since the external for loop will ++ anyway
      ++nodeIT;
    } else {
      int nextBusyCycle = (*nodeIT)->schedTime;
      if (cycle > lastBusyCycle + GAP && cycle < nextBusyCycle - GAP) {
        ofile << "+++++++++++ " << nextBusyCycle - (lastBusyCycle + 1)
              << " empty cycles " << lastBusyCycle + 1 << "-" << nextBusyCycle
              << " +++++++++++\n";
        cycle = nextBusyCycle - (GAP + 1);
      } else {
        ofile << std::setw(4) << cycle << " "
              << "\n";
      }
    }
  }
  ofile.close();
}

/************************************************************************/
/* G4Sched::G4_BB_Schedule                                             */
/************************************************************************/

//
//  G4_BB_Schedule constructor:
//      - creates dependence DAG (can emit DAG by enabling a macro in DDD::DDD);
//      - re-schedules the code;
//      - dumps the DAG (optional)
//      - creates a new instruction listing within a BBB
//
G4_BB_Schedule::G4_BB_Schedule(G4_Kernel *k, G4_BB *block,
                               const LatencyTable &LT, PointsToAnalysis &p)
    : bb(block), kernel(k), pointsToAnalysis(p) {
  // we use local id in the scheduler for determining two instructions' original
  // ordering
  bb->resetLocalIds();

  DDD ddd(bb, LT, k, p);
  // Generate pairs of TypedWrites
  bool doMessageFuse =
      (k->fg.builder->fuseTypedWrites() && k->getSimdSize() >= g4::SIMD16) ||
      k->fg.builder->fuseURBMessage();

  if (doMessageFuse) {
    ddd.pairTypedWriteOrURBWriteNodes(bb);
  }
  if (ddd.hasMultipleDpasNodes() &&
      (getBuilder()->hasDpasFwdAndDoubleSrcReadSupression() ||
       getOptions()->getOption(vISA_ScheduleFor2xDpas))) {
    ddd.pair2xDpasNodes();
  }

  if ((ddd.getIs2xFPBlock()) && getOptions()->getOption(vISA_ScheduleFor2xSP)) {
    lastCycle = ddd.listScheduleFor2xFP(this);
  } else if (getOptions()->getOption(vISA_ScheduleForReadSuppression) &&
             ddd.getIsThreeSourceBlock()) {
    lastCycle = ddd.listScheduleForSuppression(this);
  } else {
    lastCycle = ddd.listSchedule(this);
  }

  if (getOptions()->getOption(vISA_DumpSchedule)) {
    dumpSchedule(bb);
  }
  if (getOptions()->getOption(vISA_DumpDot)) {
    ddd.DumpDotFile(bb);
  }

  // Update the listing of the basic block with the reordered code.
  INST_LIST_ITER inst_it = bb->begin();
  Node *prevNode = nullptr;
  unsigned HWThreadsPerEU = k->getNumThreads();
  size_t scheduleInstSize = 0;
  for (Node *currNode : scheduledNodes) {
    for (G4_INST *inst : *currNode->getInstructions()) {
      (*inst_it) = inst;
      ++scheduleInstSize;
      if (prevNode && !prevNode->isLabel()) {
        int32_t stallCycle =
            (int32_t)currNode->schedTime - (int32_t)prevNode->schedTime;
        if (stallCycle > 0 && stallCycle > int32_t(prevNode->getOccupancy())) {
          auto perThreadStall = stallCycle;
          // per-thread-stall needs to be adjusted when latency is for
          // single-thread
          if (!getBuilder()->useMultiThreadLatency())
            perThreadStall = (stallCycle + HWThreadsPerEU - 1) / HWThreadsPerEU;
          sendStallCycle += perThreadStall;
          sequentialCycle += perThreadStall;
        }
      }
      sequentialCycle += currNode->getOccupancy();
      prevNode = currNode;
      inst_it++;
    }
  }

  vISA_ASSERT(scheduleInstSize == bb->size(),
         "Size of inst list is different before/after scheduling");
}

void Node::dump() {
  std::cerr << "Id:" << nodeID << " Prio:" << priority << " Earl:" << earliest
            << " Occu:" << occupancy << " ";
  for (G4_INST *inst : instVec) {
    inst->dump();
  }
  std::cerr << "SchedTime: " << schedTime << "\n";
  std::cerr << "predsNotScheduled: " << predsNotScheduled << "\n";
  std::cerr << "Preds: ";
  for (const Edge &predE : preds) {
    std::cerr << predE.getNode()->nodeID << ", ";
  }
  std::cerr << "\n";

  std::cerr << "Succs: ";
  for (const Edge &succE : succs) {
    std::cerr << succE.getNode()->nodeID << ", ";
  }
  std::cerr << "\n";
}

static bool needBothAcc(IR_Builder *builder, G4_INST *inst, G4_Operand *opnd) {
  switch (opnd->getType()) {
  case Type_F:
    return inst->getExecSize() == G4_ExecSize(builder->getNativeExecSize() * 2);
  case Type_HF:
  case Type_BF:
    return false;
  case Type_DF:
    return inst->getExecSize() > G4_ExecSize(builder->getNativeExecSize() / 2);
  default:
    return true;
  }
}

// Compute the range of registers touched by OPND.
static Mask getMaskForOp(IR_Builder* builder, G4_INST *inst,
                         Gen4_Operand_Number opnd_num, unsigned GRFSize) {
  G4_Operand *opnd = inst->getOperand(opnd_num);
  unsigned short LB, RB;
  bool nonContiguousStride = false;

  auto getFlagBounds = [&](G4_Operand *opnd) {
    const unsigned BITS_PER_FLAG_REG = 16;
    bool valid = true;
    unsigned subRegOff = opnd->getBase()->ExSubRegNum(valid);
    LB = (unsigned short)(opnd->getLeftBound() + subRegOff * BITS_PER_FLAG_REG);
    RB =
        (unsigned short)(opnd->getRightBound() + subRegOff * BITS_PER_FLAG_REG);
  };

  auto getACCBounds = [&](G4_Operand *opnd, unsigned GRFSize) {
    bool valid = true;
    unsigned regNum = 0;
    unsigned subRegNum = 0;

    if (opnd->isDstRegRegion()) {
      regNum = opnd->asDstRegRegion()->ExRegNum(valid);
      subRegNum = opnd->asDstRegRegion()->ExSubRegNum(valid);
    } else {
      regNum = opnd->asSrcRegRegion()->ExRegNum(valid);
      subRegNum = opnd->asSrcRegRegion()->ExSubRegNum(valid);
    }

    LB = regNum * GRFSize + subRegNum * TypeSize(opnd->getType());
    if (needBothAcc(builder, opnd->getInst(), opnd)) {
      RB = 2 * GRFSize - 1 + LB;
    } else {
      RB = opnd->getRightBound() - opnd->getLeftBound() + LB;
    }
  };

  auto getAddressBounds = [&](G4_Operand *opnd) {
    bool valid = true;
    unsigned subRegNum = 0;
    if (opnd->isSrcRegRegion()) {
      G4_SrcRegRegion *srcRegRegion = opnd->asSrcRegRegion();
      if (srcRegRegion->getRegAccess() == Direct) {
        subRegNum = srcRegRegion->ExSubRegNum(valid);
      } else {
        subRegNum = srcRegRegion->ExIndSubRegNum(valid);
      }
    } else if (opnd->isDstRegRegion()) {
      G4_DstRegRegion *dstRegRegion = opnd->asDstRegRegion();
      if (dstRegRegion->getRegAccess() == Direct) {
        subRegNum = dstRegRegion->ExSubRegNum(valid);
      } else {
        subRegNum = dstRegRegion->ExIndSubRegNum(valid);
      }
    } else {
      vISA_ASSERT_UNREACHABLE("invalid A0 operand");
    }
    G4_Type addrType = opnd->isIndirect() ? ADDR_REG_TYPE : opnd->getType();
    LB = subRegNum * TypeSize(addrType);
    RB = opnd->getRightBound() - opnd->getLeftBound() + LB;
  };

  switch (opnd_num) {
  case Opnd_src0:
  case Opnd_src1:
  case Opnd_src2:
  case Opnd_src3:
  case Opnd_src4:
  case Opnd_pred:
  case Opnd_implAccSrc: {
    if (opnd->isFlag()) {
      getFlagBounds(opnd);
    } else if (opnd->isAccReg() &&
               builder->getOptions()->getOption(vISA_ScheduleACCDep)) {
      getACCBounds(opnd, GRFSize);
    } else if (opnd->getBase() && opnd->getBase()->isA0()) {
      getAddressBounds(opnd);
    } else {
      LB = (unsigned short)opnd->getLinearizedStart();
      RB = (unsigned short)opnd->getLinearizedEnd();
      G4_SrcRegRegion *srcOpnd = opnd->asSrcRegRegion();
      const RegionDesc *rd = srcOpnd->getRegion();
      nonContiguousStride = !rd->isContiguous(inst->getExecSize());
    }
    break;
  }
  case Opnd_dst:
  case Opnd_condMod:
  case Opnd_implAccDst: {
    if (opnd->isFlag()) {
      getFlagBounds(opnd);
    } else if (opnd->isAccReg() &&
               builder->getOptions()->getOption(vISA_ScheduleACCDep)) {
      getACCBounds(opnd, GRFSize);
    } else if (opnd->getBase() && opnd->getBase()->isA0()) {
      getAddressBounds(opnd);
    } else {
      LB = (unsigned short)opnd->getLinearizedStart();
      RB = (unsigned short)opnd->getLinearizedEnd();
      G4_DstRegRegion *dstOpnd = opnd->asDstRegRegion();
      nonContiguousStride =
          (inst->getExecSize() != 1 && dstOpnd->getHorzStride() != 1);
    }
    break;
  }
  default:
    vISA_ASSERT_UNREACHABLE("Bad opnd");
    LB = 0;
    RB = 0;
  }
  return Mask(LB, RB, nonContiguousStride, opnd->getAccRegSel());
}

void DDD::getBucketsForIndirectOperand(G4_INST *inst,
                                       Gen4_Operand_Number opnd_num,
                                       std::vector<BucketDescr> &BDvec) {
  G4_Declare *addrdcl = nullptr;
  G4_Operand *opnd = inst->getOperand(opnd_num);
  if (opnd) {
    addrdcl = GetTopDclFromRegRegion(opnd);
  }
  vISA_ASSERT(addrdcl != nullptr, "address declare can not be nullptr");

  auto pointsToSet = pointsToAnalysis.getAllInPointsTo(addrdcl->getRegVar());
  for (auto &pt : *pointsToSet) {
    [[maybe_unused]] uint32_t varID = pt.var->getId();
    G4_Declare *dcl = pt.var->getDeclare()->getRootDeclare();
    G4_RegVar *var = dcl->getRegVar();

    vISA_ASSERT(var->getId() == varID,
           "RA verification error: Invalid regVar ID!");
    vISA_ASSERT(var->getPhyReg()->isGreg(),
           "RA verification error: Invalid dst reg!");

    uint32_t regNum = var->getPhyReg()->asGreg()->getRegNum();
    uint32_t regOff = var->getPhyRegOff();
    int linearizedStart = regNum * kernel->numEltPerGRF<Type_UB>() +
                          regOff * TypeSize(dcl->getElemType());
    int linearizedEnd = linearizedStart + dcl->getByteSize() - 1;

    int startingBucket = linearizedStart / kernel->numEltPerGRF<Type_UB>();
    int endingBucket = linearizedEnd / kernel->numEltPerGRF<Type_UB>();
    Mask mask(linearizedStart, linearizedEnd, false, opnd->getAccRegSel());
    int numBuckets = endingBucket - startingBucket + 1;
    for (int j = startingBucket; j < (startingBucket + numBuckets); j++) {
      BDvec.push_back(BucketDescr(j, mask, opnd_num));
    }
  }
  return;
}

void DDD::getBucketsForOperand(G4_INST *inst, Gen4_Operand_Number opnd_num,
                               std::vector<BucketDescr> &BDvec) {
  G4_Operand *opnd = inst->getOperand(opnd_num);
  if (!opnd || opnd->isLabel() || opnd->isImm()) {
    return;
  }
#define UNINIT_BUCKET -1
  int startingBucket = UNINIT_BUCKET;
  G4_VarBase *base = opnd->getBase();
  bool canSpanMultipleBuckets = false;
  vISA_ASSERT(base, "If no base, then the operand is not touched by the instr.");

  // If a register allocated regvar, then get the physical register
  G4_VarBase *phyReg =
      (base->isRegVar()) ? base->asRegVar()->getPhyReg() : base;

  switch (phyReg->getKind()) {
  case G4_VarBase::VK_phyGReg:
    startingBucket =
        opnd->getLinearizedStart() / kernel->numEltPerGRF<Type_UB>();
    canSpanMultipleBuckets = true;
    break;
  case G4_VarBase::VK_phyAReg: {
    G4_Areg *Areg = (G4_Areg *)phyReg;
    switch (Areg->getArchRegType()) {
    case AREG_NULL:
      break;
    case AREG_A0:
      startingBucket = A0_BUCKET;
      break;
    case AREG_S0:
      startingBucket = S0_BUCKET;
      break;
    case AREG_F0:
      startingBucket = FLAG0_BUCKET;
      break;
    case AREG_F1:
      startingBucket = FLAG1_BUCKET;
      break;
    case AREG_F2:
      startingBucket = FLAG2_BUCKET;
      break;
    case AREG_F3:
      startingBucket = FLAG3_BUCKET;
      break;
    case AREG_ACC0:
    case AREG_ACC1:
      startingBucket = ACC_BUCKET;
      break;
    default:
      startingBucket = OTHER_ARF_BUCKET;
      break;
    }
    break;
  }
  case G4_VarBase::VK_regVar:
    vISA_ASSERT_UNREACHABLE("Should not be a regvar. PhyReg is extracted from regvar.");
    break;
  default:
    vISA_ASSERT_UNREACHABLE("Bad kind");
    break;
  }

  // Create one or more buckets and push them into the vector
  if (startingBucket != UNINIT_BUCKET) {
    if (canSpanMultipleBuckets) {
      vASSERT(base->isGreg());
      unsigned divisor = kernel->numEltPerGRF<Type_UB>();
      int baseBucket = GRF_BUCKET;
      int endingBucket = baseBucket + opnd->getLinearizedEnd() / divisor;
      vISA_ASSERT(endingBucket >= startingBucket,
                   "Ending bucket less than starting bucket");
      Mask mask = getMaskForOp(kernel->fg.builder, inst, opnd_num, divisor);
      int numBuckets = endingBucket - startingBucket + 1;
      for (int j = startingBucket; j < (startingBucket + numBuckets); j++) {
        BDvec.push_back(BucketDescr(j, mask, opnd_num));
      }
      // If this operand is a non-trivial special ACC operand, add
      // it to the other ARF bucket for tacking extra dependencies.
      G4_AccRegSel Acc = opnd->getAccRegSel();
      if (Acc != G4_AccRegSel::ACC_UNDEFINED && Acc != G4_AccRegSel::NOACC)
        BDvec.push_back(BucketDescr(OTHER_ARF_BUCKET, mask, opnd_num));
    } else {
      Mask mask = getMaskForOp(kernel->fg.builder, inst, opnd_num,
                               kernel->numEltPerGRF<Type_UB>());
      BDvec.push_back(BucketDescr(startingBucket, mask, opnd_num));
    }
  }
}

// Return TRUE if opnd corresponding to opndNum has indirect access.
static inline bool hasIndirection(G4_Operand *opnd,
                                  Gen4_Operand_Number opndNum) {
  switch (opndNum) {
  case Opnd_dst:
    return opnd->asDstRegRegion()->isIndirect();
  case Opnd_src0:
  case Opnd_src1:
  case Opnd_src2:
    return opnd->asSrcRegRegion()->isIndirect();
  case Opnd_src3:
  case Opnd_src4:
  case Opnd_pred:
  case Opnd_condMod:
  case Opnd_implAccSrc:
  case Opnd_implAccDst:
    return false;
  default:
    vISA_ASSERT_UNREACHABLE("Bad opndNum");
    return false; // Unreachable
  }
}

// Given an inst with physical register assignment,
// return all bucket descriptors that the physical register can map
// to. This requires taking in to account exec size, data
// type, and whether inst is a send
void DDD::getBucketDescrs(Node *node, std::vector<BucketDescr> &BDvec) {
  for (G4_INST *inst : node->instVec) {
    // Iterate over all operands and create buckets.
    for (Gen4_Operand_Number opndNum :
         {Opnd_dst, Opnd_src0, Opnd_src1, Opnd_src2, Opnd_src3, Opnd_src4,
          Opnd_pred, Opnd_condMod, Opnd_implAccSrc, Opnd_implAccDst}) {
      G4_Operand *opnd = inst->getOperand(opndNum);
      // Skip if no operand or the operand is not touched by the instruction
      if (!opnd || !opnd->getBase()) {
        continue;
      }
      getBucketsForOperand(inst, opndNum, BDvec);
      // Check if this operand is an indirect access
      if (hasIndirection(opnd, opndNum)) {
        getBucketsForIndirectOperand(inst, opndNum, BDvec);
      }
    }

    // Sends need an additional bucket
    if (inst->isSend()) {
      if (inst->getMsgDesc()->isScratch()) {
        BDvec.push_back(BucketDescr(SCRATCH_SEND_BUCKET, Mask(), Opnd_dst));
      } else {
        BDvec.push_back(BucketDescr(SEND_BUCKET, Mask(), Opnd_dst));
      }
    }
  }
}

// This class hides the internals of dependence tracking using buckets
class LiveBuckets {
  std::vector<BucketHeadNode> nodeBucketsArray;
  DDD *ddd;
  int firstBucket;
  int numOfBuckets;
  friend class BN_iterator;
  static const bool ALL_BUCKETS = true;

public:
  class BN_iterator {
  public:
    LiveBuckets *LB;
    BUCKET_VECTOR_ITER node_it;
    int bucket;
    bool iterateAll;

    BN_iterator(LiveBuckets *LB1, BUCKET_VECTOR_ITER It, int Bucket, bool All)
        : LB(LB1), node_it(It), bucket(Bucket), iterateAll(All) {}

    void skipEmptyBuckets() {
      // If at the end of the node vector, move to next bucket
      // Keep going until a non-empty vector is found
      while (bucket < LB->numOfBuckets &&
             node_it == LB->nodeBucketsArray[bucket].bucketVec.end()) {
        bucket++;
        if (bucket < LB->numOfBuckets) {
          node_it = LB->nodeBucketsArray[bucket].bucketVec.begin();
        }
      }
    }

    // The iterator supports 2 modes of operation:
    // 1) Iterate across the vector nodes of a single bucket
    // 2) Iterate across all buckets and all vector nodes of each bucket
    BN_iterator &operator++() {
      // Mode 1
      if (iterateAll == LiveBuckets::ALL_BUCKETS) {
        auto node_ite = LB->nodeBucketsArray[bucket].bucketVec.end();
        // Increment node vector iterator
        if (node_it != node_ite) {
          ++node_it;
        }
        skipEmptyBuckets();
      }
      // Mode 2
      else {
        ++node_it;
      }
      vASSERT(!iterateAll || bucket == LB->numOfBuckets ||
             node_it != LB->nodeBucketsArray[bucket].bucketVec.end());
      return *this;
    }
    bool operator==(const BN_iterator &it2) const {
      vASSERT(LB == it2.LB && iterateAll == it2.iterateAll);
      // NOTE: order of comparisons matters: if different buckets
      //       then node_its are of different vectors
      return (bucket == it2.bucket && node_it == it2.node_it);
    }
    bool operator!=(const BN_iterator &it2) const {
      vASSERT(LB == it2.LB && iterateAll == it2.iterateAll);
      return (!(*this == it2));
    }
    BucketNode *operator*() {
      vASSERT(node_it != LB->nodeBucketsArray[bucket].bucketVec.end());
      return *node_it;
    }
  };

  LiveBuckets(DDD *Ddd, int GRF_BUCKET, int TOTAL_BUCKETS) {
    firstBucket = GRF_BUCKET;
    numOfBuckets = TOTAL_BUCKETS;
    ddd = Ddd;
    nodeBucketsArray.resize(numOfBuckets);
  }

  ~LiveBuckets() = default;

  // Mode 1: Iterate across nodes in BUCKET
  BN_iterator begin(int bucket) {
    return BN_iterator(this, nodeBucketsArray[bucket].bucketVec.begin(), bucket,
                       !ALL_BUCKETS);
  }

  // Mode 1:
  BN_iterator end(int bucket) {
    return BN_iterator(this, nodeBucketsArray[bucket].bucketVec.end(), bucket,
                       !ALL_BUCKETS);
  }

  // Mode 2: Iterate across all nodes and all buckets
  BN_iterator begin() {
    auto it = BN_iterator(this, nodeBucketsArray[firstBucket].bucketVec.begin(),
                          firstBucket, ALL_BUCKETS);
    it.skipEmptyBuckets();
    return it;
  }

  // Mode 2:
  BN_iterator end() {
    return BN_iterator(this, nodeBucketsArray[numOfBuckets - 1].bucketVec.end(),
                       numOfBuckets, ALL_BUCKETS);
  }

  void clearLive(int bucket) {
    BucketHeadNode &BHNode = nodeBucketsArray[bucket];
    BHNode.bucketVec.clear();
  }

  void clearAllLive() {
    for (int bucket_i = 0; bucket_i < numOfBuckets; ++bucket_i) {
      clearLive(bucket_i);
    }
  }

  bool hasLive(const Mask &mask, int bucket) {
    BucketHeadNode &BHNode = nodeBucketsArray[bucket];
    auto &BV = BHNode.bucketVec;
    return !BV.empty();
  }

  void kill(Mask mask, BN_iterator &bn_it) {
    BucketHeadNode &BHNode = nodeBucketsArray[bn_it.bucket];
    BUCKET_VECTOR &vec = BHNode.bucketVec;
    BUCKET_VECTOR_ITER &node_it = bn_it.node_it;
    if (*node_it == vec.back()) {
      vec.pop_back();
      node_it = vec.end();
    } else {
      *node_it = vec.back();
      vec.pop_back();
    }
  }

  // Create a bucket node for NODE using the information in BD
  // and append it to the list of live nodes.
  void add(Node *node, const BucketDescr &BD) {
    BucketHeadNode &BHNode = nodeBucketsArray[BD.bucket];
    // Append the bucket node to the vector hanging from the header
    BUCKET_VECTOR &nodeVec = BHNode.bucketVec;
    void *allocedMem = ddd->get_mem()->alloc(sizeof(BucketNode));
    BucketNode *newNode =
        new (allocedMem) BucketNode(node, BD.mask, BD.operand);
    nodeVec.push_back(newNode);
    // If it is a write to a subreg, mark the NODE accordingly
    if (BD.operand == Opnd_dst) {
      node->setWritesToSubreg(BD.bucket);
    }
  }
};

/*
Read suppression opportunity checking to group the three source instructions
with read suppression into single node
1. per-source slot
2. No RAW and WAW dependence
3. Same opcode
4. acc define will separate a new group
5. No other special register (including implicit) is used.
6. Only checking the suppression chance between adjacent instructions before
scheduling.
7. Assumption, pre-RA scheduling will not do scheduling to the three source
instructions

The group is built by scanning the code from back to front.
When tracking the dependence, we only care the dependence across different
instructions. So, in each time analysis, we record the source live of the second
instruction and the destination live of the first instruction dst1 inst1 src1_0
src1_1 src1_2 dst2 inst2 src2_0 src2_1 src2_2 In the dependence tracking, RAW:
when the destination register of the inst1 can be found in the liveSrc WAW: when
the destination register of the inst1 can be found in the liveDst liveSrc record
the source registers of all the instructions in the group. liveDst record the
destination registers of all the instructions in the group except the last one
in the group.
*/
bool DDD::hasReadSuppression(G4_INST *prevInst, G4_INST *nextInst,
                             BitSet &liveDst, BitSet &liveSrc) const {
  // Not three source
  if (nextInst->getNumSrc() != 3 || nextInst->isSend()) {
    return false;
  }

  // Different opcode
  if (prevInst->opcode() != nextInst->opcode()) {
    return false;
  }

  /*
   * No special registers except acc and GRF
   */
  for (Gen4_Operand_Number opndNum :
       {Opnd_src3, Opnd_pred, Opnd_condMod, Opnd_implAccSrc, Opnd_implAccDst}) {
    G4_Operand *opnd = prevInst->getOperand(opndNum);
    // Skip if no operand or the operand is not touched by the instruction
    if (opnd) {
      return false;
    }
  }

  // Only support GRF and ACC registers in operands
  for (Gen4_Operand_Number opndNum :
       {Opnd_dst, Opnd_src0, Opnd_src1, Opnd_src2}) {
    G4_Operand *opnd = prevInst->getOperand(opndNum);

    // Skip if no operand or the operand is not touched by the instruction
    if (!opnd->isGreg() && !opnd->isImm()) {
      return false;
    }
  }

  int currInstRegs[2][G4_MAX_SRCS];
  int nextInstRegs[2][G4_MAX_SRCS];
  bool isCurrScalar[G4_MAX_SRCS] = {false};
  bool isNextScalar[G4_MAX_SRCS] = {false};
  int opndSize = 0;

  // Get the sources of previous instruction
  for (unsigned i = 0; i < 3; i++) {
    currInstRegs[0][i] = -1;
    currInstRegs[1][i] = -1;
    G4_Operand *srcOpnd = prevInst->getSrc(i);
    if (srcOpnd) {
      if (srcOpnd->isSrcRegRegion() && srcOpnd->asSrcRegRegion()->getBase() &&
          !srcOpnd->asSrcRegRegion()->isIndirect() &&
          srcOpnd->asSrcRegRegion()->getBase()->isRegVar()) {
        G4_RegVar *baseVar =
            static_cast<G4_RegVar *>(srcOpnd->asSrcRegRegion()->getBase());
        opndSize =
            srcOpnd->getLinearizedEnd() - srcOpnd->getLinearizedStart() + 1;
        if (baseVar->isGreg()) {
          uint32_t byteAddress = srcOpnd->getLinearizedStart();
          currInstRegs[0][i] = byteAddress / kernel->numEltPerGRF<Type_UB>();

          if (opndSize > kernel->getGRFSize()) {
            currInstRegs[1][i] =
                currInstRegs[0][i] +
                (opndSize + kernel->numEltPerGRF<Type_UB>() - 1) /
                    kernel->numEltPerGRF<Type_UB>() -
                1;
          } else if (srcOpnd->asSrcRegRegion()->isScalar()) {
            // No Read suppression for SIMD 16/scalar src
            isCurrScalar[i] = true;
            currInstRegs[1][i] = currInstRegs[0][i];
          }
        }
      }
    }
  }

  // Get the source of the next instruction
  for (unsigned i = 0; i < 3; i++) {
    nextInstRegs[0][i] = -1;
    nextInstRegs[1][i] = -1;
    G4_Operand *srcOpnd = nextInst->getSrc(i);
    if (srcOpnd) {
      if (srcOpnd->isSrcRegRegion() && srcOpnd->asSrcRegRegion()->getBase() &&
          !srcOpnd->asSrcRegRegion()->isIndirect() &&
          srcOpnd->asSrcRegRegion()->getBase()->isRegVar()) {
        G4_RegVar *baseVar =
            static_cast<G4_RegVar *>(srcOpnd->asSrcRegRegion()->getBase());
        opndSize =
            srcOpnd->getLinearizedEnd() - srcOpnd->getLinearizedStart() + 1;
        if (baseVar->isGreg()) {
          uint32_t byteAddress = srcOpnd->getLinearizedStart();
          nextInstRegs[0][i] = byteAddress / kernel->numEltPerGRF<Type_UB>();

          liveSrc.set(nextInstRegs[0][i], true); // Set live

          if (opndSize > kernel->getGRFSize()) {
            int reg = nextInstRegs[0][i] +
                      (opndSize + kernel->numEltPerGRF<Type_UB>() - 1) /
                          kernel->numEltPerGRF<Type_UB>() -
                      1;
            nextInstRegs[1][i] = reg;
            liveSrc.set(reg, true); // Set live
          }
          if (srcOpnd->asSrcRegRegion()->isScalar()) {
            // No Read suppression for SIMD 16/scalar src
            isNextScalar[i] = true;
            nextInstRegs[1][i] = nextInstRegs[0][i];
          }
        }
      }
    }
  }

  bool curInstSimd8 = false;
  bool nextInstSimd8 = false;
  int dstReg0 = -1;
  int dstReg1 = -1;

  // Get Dst of the next instruction
  G4_DstRegRegion *nextDstOpnd = nextInst->getDst();

  if (nextDstOpnd && !nextDstOpnd->isIndirect() && nextDstOpnd->isGreg()) {
    opndSize =
        nextDstOpnd->getLinearizedEnd() - nextDstOpnd->getLinearizedStart() + 1;
    uint32_t byteAddress = nextDstOpnd->getLinearizedStart();
    dstReg0 = byteAddress / kernel->numEltPerGRF<Type_UB>();
    liveDst.set(dstReg0, true);
    if (opndSize <= kernel->getGRFSize()) {
      nextInstSimd8 = true;
    } else {
      dstReg1 = dstReg0 +
                (opndSize + kernel->numEltPerGRF<Type_UB>() - 1) /
                    kernel->numEltPerGRF<Type_UB>() -
                1;
      liveDst.set(dstReg1, true);
    }
  }

  // Get Dst of previous instruction
  G4_DstRegRegion *dstOpnd = prevInst->getDst();
  dstReg0 = -1;
  dstReg1 = -1;

  if (dstOpnd && !dstOpnd->isIndirect() && dstOpnd->isGreg()) {
    opndSize = dstOpnd->getLinearizedEnd() - dstOpnd->getLinearizedStart() + 1;
    uint32_t byteAddress = dstOpnd->getLinearizedStart();
    dstReg0 = byteAddress / kernel->numEltPerGRF<Type_UB>();
    // If there is RAW and WAW dependence
    if (liveSrc.isSet(dstReg0) || liveDst.isSet(dstReg0)) {
      return false;
    }

    if (opndSize <= kernel->getGRFSize()) {
      curInstSimd8 = true;
    } else {
      dstReg1 = dstReg0 +
                (opndSize + kernel->numEltPerGRF<Type_UB>() - 1) /
                    kernel->numEltPerGRF<Type_UB>() -
                1;

      // If there is RAW and WAW dependence
      if (liveSrc.isSet(dstReg1) || liveDst.isSet(dstReg1)) {
        return false;
      }
    }
  }

  // Kill the suppression if the register is defined in the same instruction
  for (unsigned i = 0; i < 3; i++) {
    if ((dstReg1 != -1 && dstReg1 == currInstRegs[0][i]) ||
        (dstReg0 != -1 && dstReg0 == currInstRegs[0][i])) {
      currInstRegs[0][i] = -1;
    }
    if ((dstReg1 != -1 && dstReg1 == currInstRegs[1][i]) ||
        (dstReg0 != -1 && dstReg0 == currInstRegs[1][i])) {
      currInstRegs[1][i] = -1;
    }
  }

  // For TGL, src0 support read suppression as well
  if (kernel->fg.builder->hasSrc0ReadSuppression()) {
    if (currInstRegs[0][0] == nextInstRegs[0][0] && currInstRegs[0][0] != -1 &&
        curInstSimd8 && nextInstSimd8) {
      // No scalar suppression
      if (!isCurrScalar[0] && !isNextScalar[0]) {
        return true;
      }
    }
  }

  // If there is read suppression for src1
  // for src1 2 GRF suppression is supported.
  if (currInstRegs[0][1] == nextInstRegs[0][1] && currInstRegs[0][1] != -1 &&
      ((curInstSimd8 && nextInstSimd8) || (!curInstSimd8 && !nextInstSimd8))) {
    // No scalar suppression
    if (!isCurrScalar[1] && !isNextScalar[1]) {
      return true;
    }
  }

  if (currInstRegs[0][2] == nextInstRegs[0][2] && currInstRegs[0][2] != -1 &&
      curInstSimd8 && nextInstSimd8) {
    // No scalar suppression
    if (!isCurrScalar[2] && !isNextScalar[2]) {
      return true;
    }
  }

  return false;
}

bool DDD::hasReadSuppression(G4_INST *prevInst, G4_INST *nextInst,
                             bool multiSuppression) const {
  // Not three source
  if (nextInst->getNumSrc() != 3 || nextInst->isSend()) {
    return false;
  }

  // Different opcode
  if (prevInst->opcode() != nextInst->opcode()) {
    return false;
  }

  /*
   * No special registers except acc and GRF
   */
  for (Gen4_Operand_Number opndNum :
       {Opnd_src3, Opnd_pred, Opnd_condMod, Opnd_implAccSrc, Opnd_implAccDst}) {
    G4_Operand *opnd = prevInst->getOperand(opndNum);
    // Skip if no operand or the operand is not touched by the instruction
    if (opnd) {
      return false;
    }
  }

  // Only support GRF and ACC registers in operands
  for (Gen4_Operand_Number opndNum :
       {Opnd_dst, Opnd_src0, Opnd_src1, Opnd_src2}) {
    G4_Operand *opnd = prevInst->getOperand(opndNum);

    // Skip if no operand or the operand is not touched by the instruction
    if (!opnd->isGreg() && !opnd->isImm()) {
      return false;
    }
  }

  int currInstRegs[2][G4_MAX_SRCS];
  int nextInstRegs[2][G4_MAX_SRCS];
  bool isCurrScalar[G4_MAX_SRCS] = {false};
  bool isNextScalar[G4_MAX_SRCS] = {false};
  int opndSize = 0;

  // Get the sources of previous instruction
  for (unsigned i = 0; i < 3; i++) {
    currInstRegs[0][i] = -1;
    currInstRegs[1][i] = -1;
    G4_Operand *srcOpnd = prevInst->getSrc(i);
    if (srcOpnd) {
      if (srcOpnd->isSrcRegRegion() && srcOpnd->asSrcRegRegion()->getBase() &&
          !srcOpnd->asSrcRegRegion()->isIndirect() &&
          srcOpnd->asSrcRegRegion()->getBase()->isRegVar()) {
        G4_RegVar *baseVar =
            static_cast<G4_RegVar *>(srcOpnd->asSrcRegRegion()->getBase());
        opndSize =
            srcOpnd->getLinearizedEnd() - srcOpnd->getLinearizedStart() + 1;
        if (baseVar->isGreg()) {
          uint32_t byteAddress = srcOpnd->getLinearizedStart();
          currInstRegs[0][i] = byteAddress / kernel->numEltPerGRF<Type_UB>();

          if (opndSize > kernel->getGRFSize()) {
            currInstRegs[1][i] =
                currInstRegs[0][i] +
                (opndSize + kernel->numEltPerGRF<Type_UB>() - 1) /
                    kernel->numEltPerGRF<Type_UB>() -
                1;
          } else if (srcOpnd->asSrcRegRegion()->isScalar()) {
            // No Read suppression for SIMD 16/scalar src
            isCurrScalar[i] = true;
            currInstRegs[1][i] = currInstRegs[0][i];
          }
        }
      }
    }
  }

  // Get the source of the next instruction
  for (unsigned i = 0; i < 3; i++) {
    nextInstRegs[0][i] = -1;
    nextInstRegs[1][i] = -1;
    G4_Operand *srcOpnd = nextInst->getSrc(i);
    if (srcOpnd) {
      if (srcOpnd->isSrcRegRegion() && srcOpnd->asSrcRegRegion()->getBase() &&
          !srcOpnd->asSrcRegRegion()->isIndirect() &&
          srcOpnd->asSrcRegRegion()->getBase()->isRegVar()) {
        G4_RegVar *baseVar =
            static_cast<G4_RegVar *>(srcOpnd->asSrcRegRegion()->getBase());
        opndSize =
            srcOpnd->getLinearizedEnd() - srcOpnd->getLinearizedStart() + 1;
        if (baseVar->isGreg()) {
          uint32_t byteAddress = srcOpnd->getLinearizedStart();
          nextInstRegs[0][i] = byteAddress / kernel->numEltPerGRF<Type_UB>();

          if (opndSize > kernel->getGRFSize()) {
            int reg = nextInstRegs[0][i] +
                      (opndSize + kernel->numEltPerGRF<Type_UB>() - 1) /
                          kernel->numEltPerGRF<Type_UB>() -
                      1;
            nextInstRegs[1][i] = reg;
          }
          if (srcOpnd->asSrcRegRegion()->isScalar()) {
            // No Read suppression for SIMD 16/scalar src
            isNextScalar[i] = true;
            nextInstRegs[1][i] = nextInstRegs[0][i];
          }
        }
      }
    }
  }

  bool curInstSimd8 = false;
  bool nextInstSimd8 = false;
  int dstReg0 = -1;
  int dstReg1 = -1;

  // Get Dst of the next instruction
  G4_DstRegRegion *nextDstOpnd = nextInst->getDst();

  if (nextDstOpnd && !nextDstOpnd->isIndirect() && nextDstOpnd->isGreg()) {
    opndSize =
        nextDstOpnd->getLinearizedEnd() - nextDstOpnd->getLinearizedStart() + 1;
    if (opndSize <= kernel->getGRFSize()) {
      nextInstSimd8 = true;
    }
  }

  // Get Dst of previous instruction
  G4_DstRegRegion *dstOpnd = prevInst->getDst();
  dstReg0 = -1;
  dstReg1 = -1;

  if (dstOpnd && !dstOpnd->isIndirect() && dstOpnd->isGreg()) {
    opndSize = dstOpnd->getLinearizedEnd() - dstOpnd->getLinearizedStart() + 1;
    uint32_t byteAddress = dstOpnd->getLinearizedStart();
    dstReg0 = byteAddress / kernel->numEltPerGRF<Type_UB>();

    if (opndSize <= kernel->getGRFSize()) {
      curInstSimd8 = true;
    } else {
      dstReg1 = dstReg0 +
                (opndSize + kernel->numEltPerGRF<Type_UB>() - 1) /
                    kernel->numEltPerGRF<Type_UB>() -
                1;
    }
  }

  // Kill the suppression if the register is defined in the same instruction
  for (unsigned i = 0; i < 3; i++) {
    if ((dstReg1 != -1 && dstReg1 == currInstRegs[0][i]) ||
        (dstReg0 != -1 && dstReg0 == currInstRegs[0][i])) {
      currInstRegs[0][i] = -1;
    }
    if ((dstReg1 != -1 && dstReg1 == currInstRegs[1][i]) ||
        (dstReg0 != -1 && dstReg0 == currInstRegs[1][i])) {
      currInstRegs[1][i] = -1;
    }
  }

  int suppressionSrcs = 0;
  // For TGL, src0 support read suppression as well
  if (kernel->fg.builder->hasSrc0ReadSuppression()) {
    if (currInstRegs[0][0] == nextInstRegs[0][0] && currInstRegs[0][0] != -1 &&
        curInstSimd8 && nextInstSimd8) {
      // No scalar suppression
      if (!((!kernel->fg.builder->hasScalarReadSuppression()) &&
            (isCurrScalar[0] || isNextScalar[0]))) {
        if (!multiSuppression) {
          return true;
        }
        suppressionSrcs++;
      }
    }
  }

  // If there is read suppression for src1
  // for src1 2 GRF suppression is supported.
  if (currInstRegs[0][1] == nextInstRegs[0][1] && currInstRegs[0][1] != -1 &&
      ((curInstSimd8 && nextInstSimd8) || (!curInstSimd8 && !nextInstSimd8))) {
    // No scalar suppression
    if (!((!kernel->fg.builder->hasScalarReadSuppression()) &&
          (isCurrScalar[1] || isNextScalar[1]))) {
      if (!multiSuppression) {
        return true;
      }
      suppressionSrcs++;
    }
  }

  if (currInstRegs[0][2] == nextInstRegs[0][2] && currInstRegs[0][2] != -1 &&
      curInstSimd8 && nextInstSimd8) {
    // No scalar suppression
    if (!((!kernel->fg.builder->hasScalarReadSuppression()) &&
          (isCurrScalar[2] || isNextScalar[2]))) {
      if (!multiSuppression) {
        return true;
      }
      suppressionSrcs++;
    }
  }

  return suppressionSrcs > 1;
}

bool DDD::hasDpasReadSuppression(G4_INST *prevInst, G4_INST *nextInst) const {
  if (!prevInst->isDpas() || !nextInst->isDpas()) {
    return false;
  }

  // No special registers except acc and GRF
  for (Gen4_Operand_Number opndNum :
       {Opnd_pred, Opnd_condMod, Opnd_implAccSrc, Opnd_implAccDst}) {
    G4_Operand *opnd = prevInst->getOperand(opndNum);
    // Skip if no operand or the operand is not touched by the instruction
    if (opnd) {
      return false;
    }
  }

  // Only support GRF registers in operands
  for (Gen4_Operand_Number opndNum :
       {Opnd_dst, Opnd_src0, Opnd_src1, Opnd_src2}) {
    G4_Operand *opnd = prevInst->getOperand(opndNum);
    // Skip if no operand or the operand is not touched by the instruction
    if (!opnd->isGreg() && !opnd->isImm()) {
      return false;
    }
  }

  int currInstRegs[G4_MAX_SRCS];
  int nextInstRegs[G4_MAX_SRCS];

  // Get the sources of previous instruction
  for (unsigned i = 0; i < 3; i++) {
    currInstRegs[i] = -1;
    G4_Operand *srcOpnd = prevInst->getSrc(i);
    if (srcOpnd) {
      if (srcOpnd->isSrcRegRegion() && srcOpnd->asSrcRegRegion()->getBase() &&
          !srcOpnd->asSrcRegRegion()->isIndirect() &&
          srcOpnd->asSrcRegRegion()->getBase()->isRegVar()) {
        G4_RegVar *baseVar =
            static_cast<G4_RegVar *>(srcOpnd->asSrcRegRegion()->getBase());
        if (baseVar->isGreg()) {
          uint32_t byteAddress = srcOpnd->getLinearizedStart();
          currInstRegs[i] = byteAddress / kernel->numEltPerGRF<Type_UB>();
        }
      }
    }
  }

  // Get the source of the next instruction
  for (unsigned i = 0; i < 3; i++) {
    nextInstRegs[i] = -1;
    G4_Operand *srcOpnd = nextInst->getSrc(i);
    if (srcOpnd) {
      if (srcOpnd->isSrcRegRegion() && srcOpnd->asSrcRegRegion()->getBase() &&
          !srcOpnd->asSrcRegRegion()->isIndirect() &&
          srcOpnd->asSrcRegRegion()->getBase()->isRegVar()) {
        G4_RegVar *baseVar =
            static_cast<G4_RegVar *>(srcOpnd->asSrcRegRegion()->getBase());
        if (baseVar->isGreg()) {
          uint32_t byteAddress = srcOpnd->getLinearizedStart();
          nextInstRegs[i] = byteAddress / kernel->numEltPerGRF<Type_UB>();
        }
      }
    }
  }

  // Src1 suppression
  if (currInstRegs[1] == nextInstRegs[1] && currInstRegs[1] != -1) {
    return true;
  }

  // Src2 suppression
  if (currInstRegs[2] == nextInstRegs[2] && currInstRegs[2] != -1) {
    return true;
  }

  return false;
}

// check if curInst can be forwarded to nextInst
bool DDD::canFwdDPAS(const G4_INST &curInst, const G4_INST &nextInst) const {
  if (curInst.opcode() != nextInst.opcode())
    return false;

  G4_InstDpas *cur = curInst.asDpasInst();
  G4_InstDpas *next = nextInst.asDpasInst();
  vASSERT(cur && next);

  // Check if both dpas instruction have the supported macro data types.
  if (!cur->checksMacroTypes(*next))
    return false;

  // Check if both dpas instruction have the supported forwarding data types.
  if (!cur->checksFwdTypes(*next))
    return false;

  // current and next dpas are both dpas8x8
  if (cur->getSystolicDepth() != 8 || next->getSystolicDepth() != 8 ||
      cur->getRepeatCount() != 8 || next->getRepeatCount() != 8)
    return false;

  // next DPAS's src0 and dst are both identical to current DPAS's dst
  // Note that visa spec requires dst/src0 of dpas/bdpas to be grf-aligned.
  // Checking left bound should be enough to tell if they have the same starting
  // register/subregister. The right bound is not checked for dpas insts with
  // the same data type dst/src0 as those dpas insts have the same depth,
  // repcont, and operand size.
  uint32_t curDstLB = cur->getDst()->getLinearizedStart();
  uint32_t nextDstLB = next->getDst()->getLinearizedStart();
  uint32_t nextSrc0LB = next->getSrc(0)->getLinearizedStart();
  if (curDstLB != nextDstLB || curDstLB != nextSrc0LB)
    return false;

  // No internal dependency from both instruction's src1/src2 to dst
  // FIXME: can skip this check that internal dependency from src1/src2 to dst
  // is forbidden and shouldn't happen in this stage
  auto dstLB = cur->getDst()->getLinearizedStart();
  auto dstRB = cur->getDst()->getLinearizedEnd();
  for (auto i = 1; i < cur->getNumSrc(); ++i) {
    auto curSrcLB = cur->getSrc(i)->getLinearizedStart();
    auto curSrcRB = cur->getSrc(i)->getLinearizedEnd();
    if (curSrcRB >= dstLB && curSrcLB <= dstRB)
      return false;

    auto nextSrcLB = next->getSrc(i)->getLinearizedStart();
    auto nextSrcRB = next->getSrc(i)->getLinearizedEnd();
    if (nextSrcRB >= dstLB && nextSrcLB <= dstRB)
      return false;
  }

  return true;
}

bool DDD::canInSameDPASMacro(G4_INST *curInst, G4_INST *nextInst,
                               BitSet &liveDst, BitSet &liveSrc, bool sameSrcOneOnly) const {
  G4_InstDpas *curDpasInst = curInst->asDpasInst();
  G4_InstDpas *nextDpasInst = nextInst->asDpasInst();
  // Actually cur and next are in reverse order, but we should be able to check
  // macro types in any order.
  if (!curDpasInst->checksMacroTypes(*nextDpasInst))
    return false;

  uint8_t curr_D = curDpasInst->getSystolicDepth();
  uint8_t next_D = nextDpasInst->getSystolicDepth();

  // Same depth
  if (curr_D == next_D) {
    G4_Operand *c_dst = curInst->getDst();
    G4_Operand *n_dst = nextInst->getDst();
    unsigned short c_dstLB = c_dst->getLinearizedStart();
    unsigned short c_dstRB = c_dst->getLinearizedEnd();
    unsigned short n_dstLB = n_dst->getLinearizedStart();
    unsigned short n_dstRB = n_dst->getLinearizedEnd();
    int c_dstReg = c_dstLB / kernel->numEltPerGRF<Type_UB>();
    int n_dstReg = n_dstLB / kernel->numEltPerGRF<Type_UB>();

    // Set destination live for current instruction
    while (c_dstReg * kernel->numEltPerGRF<Type_UB>() < c_dstRB) {
      liveDst.set(c_dstReg, true);
      c_dstReg++;
    }

    for (int i = 0; i < 3; i++) {
      G4_Operand *c_src = curInst->getSrc(i);
      G4_Operand *n_src = nextInst->getSrc(i);
      unsigned short c_srcLB = c_src->getLinearizedStart();
      unsigned short c_srcRB = c_src->getLinearizedEnd();
      unsigned short n_srcLB = n_src->getLinearizedStart();

      // Set source live for current instruction
      int srcReg = c_srcLB / kernel->numEltPerGRF<Type_UB>();
      while (srcReg * kernel->numEltPerGRF<Type_UB>() < c_srcRB) {
        liveSrc.set(srcReg, true);
        srcReg++;
      }

      // Not same src1 register
      if (sameSrcOneOnly && i == 1 && c_srcLB != n_srcLB) {
        return false;
      }
    }

    // If there is RAW, WAW dependence
    while (n_dstReg * kernel->numEltPerGRF<Type_UB>() < n_dstRB) {
      if (liveDst.isSet(n_dstReg) || liveSrc.isSet(n_dstReg)) {
        return false;
      }
      n_dstReg++;
    }

    for (int i = 0; i < 3; i++) {
      G4_Operand *n_src = nextInst->getSrc(i);
      unsigned short n_srcLB = n_src->getLinearizedStart();
      unsigned short n_srcRB = n_src->getLinearizedEnd();
      // If there is WAR dependence
      int srcReg = n_srcLB / kernel->numEltPerGRF<Type_UB>();
      while (srcReg * kernel->numEltPerGRF<Type_UB>() < n_srcRB) {
        if (liveDst.isSet(srcReg)) {
          return false;
        }
        srcReg++;
      }
    }

    return true;
  }

  return false;
}

// Construct data dependency DAG. The function constructs DAG using
// a bucket-based algorithm. The idea is to setup as many buckets as there are
// GRFs (and ARFs). Then when testing dependencies the DAG, we only need to
// look at buckets that current inst may fall into. This saves us from checking
// dependencies with all insts in live set. After analyzing dependencies and
// creating necessary edges, current inst is inserted in all buckets it
// touches.
DDD::DDD(G4_BB *bb, const LatencyTable &lt, G4_Kernel *k, PointsToAnalysis &p)
    : DDDMem(4096), LT(lt), kernel(k), pointsToAnalysis(p) {
  Node *lastBarrier = nullptr;
  HWthreadsPerEU = k->getNumThreads();
  useMTLatencies = getBuilder()->useMultiThreadLatency();
  totalGRFNum = kernel->getNumRegTotal();
  totalACCNum = 1;
  if (getOptions()->getOption(vISA_ScheduleACCDep)) {
    totalACCNum = kernel->getNumAcc();
  }
  isThreeSouceBlock = false;
  is_2XFP_Block = false;
  bool BTIIsRestrict =
      getOptions()->getOption(vISA_ReorderDPSendToDifferentBti);

  GRF_BUCKET = 0;
  ACC_BUCKET = GRF_BUCKET + totalGRFNum;
  FLAG0_BUCKET = ACC_BUCKET + totalACCNum;
  FLAG1_BUCKET = FLAG0_BUCKET + 1;
  FLAG2_BUCKET = FLAG1_BUCKET + 1;
  FLAG3_BUCKET = FLAG2_BUCKET + 1;
  A0_BUCKET = FLAG3_BUCKET + 1;
  SEND_BUCKET = A0_BUCKET + 1;
  SCRATCH_SEND_BUCKET = SEND_BUCKET + 1;
  S0_BUCKET = SCRATCH_SEND_BUCKET + 1;
  OTHER_ARF_BUCKET = S0_BUCKET + 1;
  TOTAL_BUCKETS = OTHER_ARF_BUCKET + 1;

  LiveBuckets LB(this, GRF_BUCKET, TOTAL_BUCKETS);
  for (int i = 0; i < PIPE_ALL; i++) {
    latestInstOfEachPipe[i] = nullptr;
  }
  // Building the graph in reverse relative to the original instruction
  // order, to naturally take care of the liveness of operands.
  std::list<G4_INST *>::reverse_iterator iInst(bb->rbegin()),
      iInstEnd(bb->rend());
  std::vector<BucketDescr> BDvec;

  int threeSrcInstNUm = 0;
  int FP_InstNum = 0;
  int sendInstNum = 0;
  for (int nodeId = (int)(bb->size() - 1); iInst != iInstEnd;
       ++iInst, nodeId--) {
    Node *node = nullptr;
    // If we have a pair of instructions to be mapped on a single DAG node:
    node = new (NodeAllocator) Node(nodeId, *iInst, depEdgeAllocator, LT);
    allNodes.push_back(node);
    G4_INST *curInst = node->getInstructions()->front();
    BDvec.clear();

    if (curInst->getNumSrc() == 3) {
      threeSrcInstNUm++;
    }
    G4_DstRegRegion *dstOpnd = curInst->getDst();
    sendInstNum += curInst->isSend() ? 1 : 0;
    if (dstOpnd && dstOpnd->getTopDcl() &&
        dstOpnd->getTopDcl()->getRegFile() == G4_GRF) {
      bool isCounted = false;
      if (getBuilder()->has2xDP() && instCanUse2xDP(curInst)) {
        FP_InstNum++;
        isCounted = true;
      }

      if (!isCounted && getBuilder()->has2xSP() && instCanUse2xSP(curInst)) {
        FP_InstNum++;
      }
    }
    if (getBuilder()->hasReadSuppression() &&
        getOptions()->getOption(vISA_EnableGroupScheduleForBC)) {
      // FIXME: we can extended to all 3 sources
      if (curInst->opcode() == G4_mad || curInst->opcode() == G4_dp4a) {
        std::list<G4_INST *>::reverse_iterator iNextInst = iInst;
        iNextInst++;
        if (iNextInst != iInstEnd) {
          G4_INST *nextInst = *iNextInst;
          BitSet liveSrc(totalGRFNum, false);
          BitSet liveDst(totalGRFNum, false);
          liveSrc.clear();
          liveDst.clear();
          while (hasReadSuppression(nextInst, curInst, liveDst, liveSrc)) {
            // Pushed to the same node
            node->instVec.push_front(nextInst);
            nodeId--;
            curInst = nextInst;
            iInst = iNextInst;
            iNextInst++;
            if (iNextInst == iInstEnd) {
              break;
            }
            nextInst = *iNextInst;
          }
        }
      }
    }

    if (curInst->isDpas()) {
      ++NumDpasNodes;
      bool tryGroupFwd = false;
      // Favor FWD sequence over src1 re-use.
      // Based on the lessons learned from several previous attempts, it seems
      // difficult to use 2xdpas heuristic to schedule dpas fwd sequence because
      // a dpas could have multiple dependencies. Some dependencies might cause
      // dpas not in pending queue for heuristic to kick in. Grouping dpas fwd
      // sequence as a scheduling node might be an easier approach to address
      // the issue.
      // 1. Here we may want to preserve the input order of dpas instructions
      //    that can form a FWD sequence as input sequence could be already be
      //    a nicely blended one written by the kernel developers. Grouping here
      //    could possibly reduce the number of nodes that need to be processed
      //    in the 2nd stage.
      // 2. Before scheduling, we perform another pass to group nodes that are
      //    not consecutive in input .visaasm but can be paired as a (longer)
      //    2xdpas sequence.
      if (getBuilder()->hasDpasFwdAndDoubleSrcReadSupression() ||
          getOptions()->getOption(vISA_ScheduleFor2xDpas))
        tryGroupFwd = true;
      std::list<G4_INST *>::reverse_iterator iNextInst = iInst;
      iNextInst++;
      if (iNextInst != iInstEnd) {
        G4_INST *nextInst = *iNextInst;
        BitSet liveSrc(totalGRFNum, false);
        BitSet liveDst(totalGRFNum, false);
        liveSrc.clear();
        liveDst.clear();

        // group continuous dpas in the same node if they can potentially form a
        // dpas macro
        while (nextInst->isDpas()) {
          bool canGroup = false;
          if (tryGroupFwd) {
            // cur and next are in reverse order.
            canGroup = canFwdDPAS(*nextInst, *curInst);
          } else if (getOptions()->getOption(vISA_KeepDPASMacroInSchedule)) {
            canGroup =
                canInSameDPASMacro(curInst, nextInst, liveDst, liveSrc, false);
          } else {
            canGroup =
                canInSameDPASMacro(curInst, nextInst, liveDst, liveSrc, true);
          }
          if (!canGroup)
            break;

          // Pushed to the same node
          node->instVec.insert(node->instVec.begin(), nextInst);
          nodeId--;
          curInst = nextInst;
          iInst = iNextInst;
          iNextInst++;
          if (iNextInst == iInstEnd) {
            break;
          }
          nextInst = *iNextInst;
        }
      }
    }
    // Get buckets for all physical registers assigned in curInst
    getBucketDescrs(node, BDvec);
    if (curInst->isSend() && curInst->asSendInst()->isFence()) {
      node->MarkAsUnresolvedIndirAddressBarrier();
    }

    DepType depType;
    if ((depType = node->isLabel()) || (depType = node->isBarrier())) {
      // Insert edge from current instruction
      // to all instructions live in every bucket
      for (auto it = LB.begin(), ite = LB.end(); it != ite; ++it) {
        BucketNode *BNode = *it;
        Node *liveNode = BNode->node;
        if (liveNode->preds.empty()) {
          createAddEdge(node, liveNode, depType);
        }
      }
      LB.clearAllLive();
      if (lastBarrier) {
        createAddEdge(node, lastBarrier, lastBarrier->isBarrier());
      }

      lastBarrier = node;
    } else {
      // Compute buckets for RAW
      bool transitiveEdgeToBarrier = false;

      // For all bucket descriptors of curInst
      for (const BucketDescr &BD : BDvec) {
        const int &curBucket = BD.bucket;
        const Gen4_Operand_Number &curOpnd = BD.operand;
        const Mask &curMask = BD.mask;
        if (!LB.hasLive(curMask, curBucket)) {
          continue;
        }
        // Kill type 1: When the current destination region completely
        //              covers the whole register from the first bit
        //              to the last bit.
        bool curKillsBucket =
            curMask.killsBucket(curBucket, *kernel->fg.builder);

        // For each live curBucket node:
        // i)  create edge if required
        // ii) kill bucket node if required
        for (LiveBuckets::BN_iterator bn_it = LB.begin(curBucket);
             bn_it != LB.end(curBucket);) {
          BucketNode *liveBN = (*bn_it);
          Node *curLiveNode = liveBN->node;
          Gen4_Operand_Number liveOpnd = liveBN->opndNum;
          Mask &liveMask = liveBN->mask;

          G4_INST *liveInst = *curLiveNode->getInstructions()->begin();
          // Kill type 2: When the current destination region covers
          //              the live node's region completely.
          bool curKillsLive = curMask.kills(liveMask);
          bool hasOverlap = curMask.hasOverlap(liveMask);

          // Acc1 and Acc3 may crash acc0 data
          if (!getOptions()->getOption(vISA_ScheduleACCDep) &&
              curBucket == ACC_BUCKET) {
            hasOverlap = true;
          }
          // 1. Find DEP type
          DepType dep = DEPTYPE_MAX;
          if (curBucket < ACC_BUCKET) {
            dep = getDepForOpnd(curOpnd, liveOpnd);
          } else if (curBucket == ACC_BUCKET
                     || curBucket == A0_BUCKET || curBucket == S0_BUCKET) {
            dep = getDepForOpnd(curOpnd, liveOpnd);
            curKillsBucket = false;
          } else if (curBucket == SEND_BUCKET) {
            dep = getDepSend(curInst, liveInst, BTIIsRestrict);
            hasOverlap = (dep != NODEP);
            curKillsBucket = false;
            curKillsLive = (dep == WAW_MEMORY || dep == RAW_MEMORY);
          } else if (curBucket == SCRATCH_SEND_BUCKET) {
            dep = getDepScratchSend(curInst, liveInst);
            hasOverlap = (dep != NODEP);
            curKillsBucket = false;
            curKillsLive = false; // Disable kill
          } else if (curBucket == FLAG0_BUCKET || curBucket == FLAG1_BUCKET ||
                     curBucket == FLAG2_BUCKET || curBucket == FLAG3_BUCKET) {
            dep = getDepForOpnd(curOpnd, liveOpnd);
            curKillsBucket = false;
          } else if (curBucket == OTHER_ARF_BUCKET) {
            dep = getDepForOpnd(curOpnd, liveOpnd);
            hasOverlap = (dep != NODEP); // Let's be conservative
            curKillsBucket = false;
          } else {
            vISA_ASSERT_UNREACHABLE("Bad bucket");
          }

          // 2. Create Edge if there is overlap and RAW/WAW/WAR
          if (dep != NODEP && hasOverlap) {
            createAddEdge(node, curLiveNode, dep);
            transitiveEdgeToBarrier |= curLiveNode->hasTransitiveEdgeToBarrier;
          }

          // 3. Kill if required
          if ((dep == RAW || dep == RAW_MEMORY || dep == WAW ||
               dep == WAW_MEMORY) &&
              (curKillsBucket || curKillsLive)) {
            LB.kill(curMask, bn_it);
            continue;
          }
          vISA_ASSERT(dep != DEPTYPE_MAX, "dep unassigned?");
          ++bn_it;
        }
      }

      if (transitiveEdgeToBarrier == false && lastBarrier != nullptr) {
        // Insert edge to barrier and set flag
        createAddEdge(node, lastBarrier, lastBarrier->isBarrier());
        node->hasTransitiveEdgeToBarrier = true;
      }
    }

    // Add buckets of current instruction to bucket list
    for (const BucketDescr &BD : BDvec) {
      LB.add(node, BD);
    }
  }

  if (!bb->empty()) {
    isThreeSouceBlock =
        ((float)threeSrcInstNUm / bb->size()) > THREE_SOURCE_BLOCK_HERISTIC;
    is_2XFP_Block =
        (FP_InstNum >= FP_MIN_INST_NUM) &&
        (((float)FP_InstNum / bb->size()) > THREE_SOURCE_BLOCK_HERISTIC) &&
        (((float)sendInstNum / FP_InstNum) < FP_BLOCK_SEND_HERISTIC);
  }
}

void Node::deletePred(Node *pred) {
  for (int i = 0, size = (int)preds.size(); i < size; ++i) {
    auto &&edge = preds[i];
    if (edge.getNode() == pred) {
      if (i != size - 1) {
        preds[i] = preds.back();
      }
      preds.pop_back();
      predsNotScheduled--;
      return;
    }
  }
  vISA_ASSERT_UNREACHABLE("trying to delete a non-predecessor node");
}

// Move all input and output dependences:
// from:  ...->fromNode->...
// to  :  ...->toNode->...
void DDD::moveDeps(Node *fromNode, Node *toNode) {

  // 1. Move outgoing edges:
  //    from: fromNode->...
  //      to: toNode->...
  for (size_t i = 0, succEmax = fromNode->succs.size(); i != succEmax; ++i) {
    const Edge &succE = fromNode->succs[i];
    Node *destNode = succE.getNode();
    DepType type = succE.getType();
    createAddEdge(toNode, destNode, type);
    destNode->deletePred(fromNode);
  }
  fromNode->succs.clear();

  // 2. Move incoming edges:
  //    from: ...->fromNode
  //      to: ...->toNode
  for (size_t i = 0, predEmax = fromNode->preds.size(); i != predEmax; ++i) {
    const Edge &predE = fromNode->preds[i];
    // predFromNode->fromNode
    Node *predFromNode = predE.getNode();
    int maxI = (int)predFromNode->succs.size();

    // move all edges predFromNode->fromNode
    //            to: predFromNode->toNode
    // a) add new edges predFromNode->toNode
    if (predFromNode != toNode) {
      for (int i = 0; i != maxI; ++i) {
        Edge &edge = predFromNode->succs[i];
        DepType type = edge.getType();
        if (edge.getNode() == fromNode) {
          createAddEdge(predFromNode, toNode, type);
        }
      }
    }
    // b) delete old edges predFromNode->fromNode
    //    no need to check after maxI as all those edges are new ones
    for (int i = 0; i != maxI;) {
      Edge &edge = predFromNode->succs[i];
      bool keep_iter = false;
      if (edge.getNode() == fromNode) {
        // Erase edge from predFromNode->succs[].
        // We use the swap with last element trick.
        predFromNode->succs[i] = predFromNode->succs.back();
        predFromNode->succs.pop_back();
        maxI--;
        keep_iter = true;
      }
      if (!keep_iter) {
        i++;
      }
    }
  }
  fromNode->preds.clear();
  fromNode->schedTime = 0;
}

// Return TRUE if there is no dependence chain connecting
// firstNode->...->secondNode
// That would result in a dependence cycle after pairing.
// Note that firstNode->secondNode is ok if there are no instructions in between
static bool canAvoidDepCycles(Node *firstNode, Node *secondNode,
                              bool isFirstLevel) {
  // assumption is firstNode is close to secondNode so this will be fast
  // and we don't need to cache intermediate nodes
  for (const Edge &succE : firstNode->succs) {
    auto &&node = succE.getNode();
    if (node == secondNode) {
      if (isFirstLevel) {
        continue;
      }
      return false;
    }

    if (node->getInstructions()->back()->getLocalId() >
        secondNode->getInstructions()->back()->getLocalId()) {
      // stop if we've reached beyond the second node
      continue;
    }
    if (!canAvoidDepCycles(node, secondNode, false)) {
      return false;
    }
  }
  return true;
}

// Return TRUE if INST is the partN'th part {0,1,2,3} of a typedWrite
static bool isTypedWritePart(G4_INST *inst, int partN) {
  return inst->isSend() && inst->getExecSize() == g4::SIMD8 &&
         inst->getMsgDescRaw() &&
         inst->getMsgDescRaw()->isHdcTypedSurfaceWrite() &&
         inst->getMaskOffset() == partN * 8;
};

struct URBDesc {
  uint32_t opcode : 4;
  uint32_t offset : 11; // unit of 16 bytes
  uint32_t maskPresent : 1;
  uint32_t ignored : 1;
  uint32_t perSlotPresent : 1;
  uint32_t dontcare : 14;
};

union DescData {
  uint32_t value;
  URBDesc layout;
};

// leading URB write should have cache-aligned global offset
static bool isLeadingURBWrite(G4_INST *inst) {
  if (inst->isSend() && inst->getMsgDescRaw() &&
      inst->getMsgDesc()->getSFID() == SFID::URB) {
    DescData desc;
    desc.value = inst->getMsgDescRaw()->getDesc();
    // ToDo: add support for per-slot offset and channel mask later if necessary
    if (desc.layout.opcode == URB_SIMD8_WRITE && !desc.layout.perSlotPresent &&
        !desc.layout.maskPresent && desc.layout.offset % 4 == 0) {
      return true;
    }
  }
  return false;
}

// precondition: both inst are URB messages
static bool canFuseURB(const G4_INST *secondURB, const G4_INST *firstURB) {
  if (!firstURB->getMsgDescRaw() || !secondURB->getMsgDescRaw()) {
    return false;
  }
  DescData firstDesc, secondDesc;
  firstDesc.value = firstURB->getMsgDescRaw()->getDesc();
  secondDesc.value = secondURB->getMsgDescRaw()->getDesc();
  if (firstDesc.layout.opcode == secondDesc.layout.opcode &&
      firstDesc.layout.maskPresent == secondDesc.layout.maskPresent &&
      firstDesc.layout.perSlotPresent == secondDesc.layout.maskPresent &&
      firstDesc.layout.offset + 2 == secondDesc.layout.offset) {
    if (firstURB->getMsgDescRaw()->MessageLength() ==
            secondURB->getMsgDescRaw()->MessageLength() &&
        firstURB->getMsgDescRaw()->extMessageLength() ==
            secondURB->getMsgDescRaw()->extMessageLength()) {
      return true;
    }
  }

  return false;
}

// Join nodes that need to be paired into fat nodes.
void DDD::pairTypedWriteOrURBWriteNodes(G4_BB *bb) {
  // 1. Scan through the code for pairs
  //    A pair {A, B}  is set like this: instrPairs[A] = B
  //                                     instrPairs[B] = A
  instrPairVec_t instrPairs;
  // Go through the instructions in BB and find all possible pairs of
  // typedWrites.
  Node *foundFirst = nullptr, *foundSecond = nullptr;
  Node *foundThird = nullptr, *foundFourth = nullptr;

  for (auto it = allNodes.rbegin(), ite = allNodes.rend(); it != ite; ++it) {
    Node *node = *it;
    G4_INST *inst = (*node->getInstructions()).front();
    // {0,1}
    if (!foundFirst && isTypedWritePart(inst, 0)) {
      foundFirst = node;
    }
    if (foundFirst && isTypedWritePart(inst, 1)) {
      foundSecond = node;
      instrPairs.emplace_back(DDD::instrPair_t(foundFirst, foundSecond));
      foundFirst = nullptr;
      foundSecond = nullptr;
    }
    // {2,3}
    if (!foundThird && isTypedWritePart(inst, 2)) {
      foundThird = node;
    }
    if (foundThird && isTypedWritePart(inst, 3)) {
      foundFourth = node;
      instrPairs.emplace_back(DDD::instrPair_t(foundThird, foundFourth));
      foundThird = nullptr;
      foundFourth = nullptr;
    }
  }

  Node *leadingURB = nullptr;
  for (auto it = allNodes.rbegin(), ite = allNodes.rend(); it != ite; ++it) {
    Node *node = *it;
    G4_INST *inst = (*node->getInstructions()).front();
    if (!leadingURB && isLeadingURBWrite(inst)) {
      leadingURB = node;
    } else if (leadingURB) {
      if (inst->isSend() && inst->getMsgDesc()->getSFID() == SFID::URB) {
        if (canFuseURB(inst, (*leadingURB->getInstructions()).front())) {
          instrPairs.emplace_back(DDD::instrPair_t(leadingURB, node));
          leadingURB = nullptr;
        } else {
          leadingURB = isLeadingURBWrite(inst) ? node : nullptr;
        }
      } else {
        // stop if this instruction depends on leadingURB
        // it's a conservative way to avoid cycles in the DAG when
        // fusing two URB sends (e.g., second URB's payload share register with
        // first)
        Node *leadingURBNode = leadingURB;
        Node *curNode = node;
        for (auto &&succ : leadingURBNode->succs) {
          if (succ.getNode() == curNode) {
            leadingURB = nullptr;
          }
        }
      }
    }
  }

  // 2. Join nodes that need pairing
  for (auto &&pair : instrPairs) {
    Node *firstNode = pair.first;
    Node *secondNode = pair.second;
    [[maybe_unused]] G4_INST *firstInstr = (*firstNode->getInstructions()).front();
    [[maybe_unused]] G4_INST *secondInstr = (*secondNode->getInstructions()).front();
    vASSERT(firstNode->getInstructions()->size() == 1);
    vASSERT(secondNode->getInstructions()->size() == 1);
    vASSERT(*firstNode->getInstructions()->begin() == firstInstr);
    vASSERT(*secondNode->getInstructions()->begin() == secondInstr);
    if (canAvoidDepCycles(firstNode, secondNode, true)) {
      // A. move the deps of second node to the first.
      moveDeps(secondNode, firstNode);

      // B. We add the second instruction to the first node.
      vASSERT(firstNode->getInstructions()->size() == 1);
      firstNode->addPairInstr(*secondNode->getInstructions()->begin());
      if (!kernel->fg.builder->getOption(vISA_NoAtomicSend) &&
          firstInstr->isSend() &&
          (firstInstr->getMsgDesc()->getSFID() == SFID::URB ||
           (kernel->fg.builder->fuseTypedWrites() &&
            (isTypedWritePart(firstInstr, 0) ||
             isTypedWritePart(firstInstr, 2))))) {
        firstInstr->setOptionOn(InstOpt_Atomic);
      }

      // C. Cleanup the paired node.
      secondNode->clear();
    }
  }
}

void DDD::pair2xDpasNodes() {
  // The function collects pairs of dpas nodes that can build a fwd sequence in
  // a forward order. After collecting those pairs, the dependence and
  // instructions are propagated from fwd dst node to fwd src node in a backward
  // order. With propagation, a longer fwd sequence could be built potentially.
  instrPairVec_t fwdPairs;

  // The compare function is designed to let dpas picked as late as possible.
  // Note that node id is used to break tie.
  // TODO: We may consider an infrastructure refactor on scheduling queue like
  // PreRA scheduling to get rid of priority_queue. Currently follow the
  // existing code to manipulate priority_queue to select a better candidate.
  auto nodeCmp = [](const Node *n1, const Node *n2) -> bool {
    bool isN1Dpas = n1->getInstructions()->front()->isDpas();
    bool isN2Dpas = n2->getInstructions()->front()->isDpas();
    if (isN1Dpas != isN2Dpas)
      return isN1Dpas > isN2Dpas;
    return n1->getNodeID() > n2->getNodeID();
  };
  std::priority_queue<Node *, std::vector<Node *>, decltype(nodeCmp)>
      depFreeNodes(nodeCmp);

  collectRoots();
  for (auto n : Roots)
    depFreeNodes.push(n);

  auto apply2xDpasHeuristic = [&](Node *picked, Node *lastPicked) -> Node * {
    assert(picked);
    // The heuristic depends on last picked node. Return early if last picked
    // node is not dpas.
    if (!lastPicked || !lastPicked->getInstructions()->front()->isDpas())
      return nullptr;
    // Return early if last picked node can be forwarded to the picked one
    // already.
    if (picked->getInstructions()->front()->isDpas() &&
        canFwdDPAS(*lastPicked->getInstructions()->back(),
                   *picked->getInstructions()->front())) {
      fwdPairs.emplace_back(lastPicked, picked);
      return nullptr;
    }

    // Go though all dep-free nodes to find a node that can be forwarded from
    // the last picked node if any.
    std::vector<Node *> popped;
    Node *cand = nullptr;
    while (!depFreeNodes.empty()) {
      Node *node = depFreeNodes.top();
      depFreeNodes.pop();
      if (node->getInstructions()->front()->isDpas() &&
          canFwdDPAS(*lastPicked->getInstructions()->back(),
                     *node->getInstructions()->front())) {
        depFreeNodes.push(picked);
        cand = node;
        fwdPairs.emplace_back(lastPicked, cand);
        break;
      } else
        popped.push_back(node);
    }
    for (Node *n : popped)
      depFreeNodes.push(n);
    return cand;
  };

  Node *lastPicked = nullptr;
  while (!depFreeNodes.empty()) {
    Node *picked = depFreeNodes.top();
    depFreeNodes.pop();
    if (Node *heuCandidate = apply2xDpasHeuristic(picked, lastPicked))
      picked = heuCandidate;
    lastPicked = picked;
    // After picking a node, move its successors to def-free node list when
    // possible.
    for (auto &succ : picked->succs) {
      Node *n = succ.getNode();
      if (--(n->predsNotScheduled) == 0)
        depFreeNodes.push(n);
    }
  }
  // Restore predsNotScheduled values for real code scheduling to work.
  for (auto n : allNodes)
    n->predsNotScheduled = n->preds.size();

  // Propagate the dependencies and instructions from fwdDst node to fwdSrc node
  for (auto rit = fwdPairs.rbegin(), rie = fwdPairs.rend(); rit != rie; ++rit) {
    Node *fwdSrc = rit->first;
    Node *fwdDst = rit->second;
    moveDeps(fwdDst, fwdSrc);
    fwdSrc->instVec.insert(
        fwdSrc->instVec.end(), fwdDst->instVec.begin(), fwdDst->instVec.end());
    fwdDst->clear();
  }
}
void DDD::collectRoots() {
  Roots.clear();
  for (auto N : allNodes) {
    if (N->preds.empty() && !N->getInstructions()->empty()) {
      Roots.push_back(N);
    }
  }
}

void DDD::setPriority(Node *pred, const Edge &edge) {
  // Calculate PRED's priority (pred->priority), based on SUCC's priority
  Node *succ = edge.getNode();
  vISA_ASSERT(succ->priority != Node::PRIORITY_UNINIT,
         "succ node has no priority?");
  int newPriority = succ->priority + edge.getLatency();
  pred->priority =
      (newPriority > pred->priority) ? newPriority : pred->priority;
}

// Create a new edge PRED->SUCC of type D.
// The edge latency is also attached.
void DDD::createAddEdge(Node *pred, Node *succ, DepType d) {
  // Check whether an edge already exists
  for (int i = 0; i < (int)(pred->succs.size()); i++) {
    Edge &curSucc = pred->succs[i];
    // Keep the deptype that has the highest latency
    if (curSucc.getNode() == succ) {
      uint32_t newEdgeLatency = getEdgeLatency(pred, d);
      if (newEdgeLatency > curSucc.getLatency()) {
        // Update with the dep type that causes the highest latency
        curSucc.setType(d);
        curSucc.setLatency(newEdgeLatency);
        // Set the node priority
        setPriority(pred, curSucc);
      }
      return;
    }
  }

  // No edge with the same successor exists. Append this edge.
  uint32_t edgeLatency = getEdgeLatency(pred, d);
  pred->succs.emplace_back(succ, d, edgeLatency);

  uint32_t dumpSendDep = getOptions()->getuInt32Option(vISA_DumpSendDepLatency);
  if (dumpSendDep) {
    G4_INST *inst = *pred->getInstructions()->begin();
    if (inst->isSend() && (d == dumpSendDep)) {
      std::stringstream comment;
      comment << "D" << d << ":" << edgeLatency;
      inst->addComment(comment.str());
    }
  }

  // Set the node priority
  setPriority(pred, pred->succs.back());
  // Count SUCC's predecessors (used to tell when SUCC is ready to issue)
  succ->predsNotScheduled++;

  succ->preds.emplace_back(pred, d, edgeLatency);
}

// Debug function for generating the dependency graph in dot format
void DDD::dumpDagDot(G4_BB *bb) {
  const char *asmName = nullptr;
  getOptions()->getOption(VISA_AsmFileName, asmName);
  std::string dumpFileName =
      std::string(asmName) + ".bb" + std::to_string(bb->getId()) + ".dag.dot";
  std::ofstream ofile(dumpFileName, std::ios::out);
  ofile << "digraph DAG {"
        << "\n";

  // 1. Get an ordering of the nodes
  std::vector<Node *> DFSordering;
  std::vector<Node *> dfs_stack;
  std::set<Node *> markedSet;
  for (Node *root : Roots) {
    dfs_stack.push_back(root);
  }
  while (!dfs_stack.empty()) {
    Node *node = dfs_stack.back();
    markedSet.insert(node);
    dfs_stack.pop_back();
    DFSordering.push_back(node);
    for (Edge &dep : node->succs) {
      Node *succNode = dep.getNode();
      if (!markedSet.count(succNode)) {
        dfs_stack.push_back(succNode);
      }
    }
  }

  // 2. Generate the .dot file for the DAG
  for (Node *node : allNodes) {
    // G4_INST &g4Inst = const_cast<G4_INST&>(*node->getInstruction());
    // std::stringstream ss;
    // g4Inst.emit(ss, false, false);
    // 1. NODE
    const char *fillColor = (node->schedTime != UINT_MAX)           ? "#CCCCCC"
                            : (node->getInstructions()->size() > 1) ? "#FFEE99"
                                                                    : "#FFFFFF";
    ofile << node->nodeID << "[label=\"";
    for (G4_INST *inst : node->instVec) {
      ofile << G4_Inst_Table[inst->opcode()].str << ", ";
    }
    ofile << "[" << node->nodeID << "]";
    const std::list<G4_INST *> *instrs = node->getInstructions();
    if (instrs->empty()) {
      ofile << "I" << 0;
    } else {
      for (G4_INST *instr : *instrs) {
        ofile << "I" << instr->getLocalId();
      }
    }
    ofile << "O" << node->getOccupancy() << "E" << node->getEarliest() << "P"
          << node->priority << "\""
          << ", style=\"filled\", fillcolor=\"" << fillColor << "\""
          << "]"
          << "\n";
    // 2. EDGES
    for (Edge &succDep : node->succs) {
      Node *succNode = succDep.getNode();
      if (node->nodeID > succNode->nodeID) {
        fprintf(stderr, "%d->%d\n", node->nodeID, succNode->nodeID);
      }
      auto depType = succDep.getType();
      const char *depColor = (depType == RAW || depType == RAW_MEMORY) ? "black"
                             : (depType == WAR || depType == WAR_MEMORY) ? "red"
                             : (depType == WAW || depType == WAW_MEMORY)
                                 ? "orange"
                                 : "grey";
      uint32_t edgeLatency = getEdgeLatency(node, depType);
      // Example: 30->34[label="4",color="{red|black|yellow}"];
      ofile << node->nodeID << "->" << succNode->nodeID << "[label=\""
            << edgeLatency << "\""
            << ",color=\"" << depColor << "\""
            << "];"
            << "\n";
    }
  }
  ofile << "}"
        << "\n";
  ofile.close();
}

// Debug function.
// Dump all nodes in a text form and show their dependences with arrows.
void DDD::dumpNodes(G4_BB *bb) {
  const char *asmName = nullptr;
  getOptions()->getOption(VISA_AsmFileName, asmName);
  std::string dumpFileName =
      std::string(asmName) + ".bb" + std::to_string(bb->getId()) + ".nodes";
  std::ofstream ofile(dumpFileName, std::ios::out);

  // 2. Generate the .dot file for the DAG
  for (auto it = allNodes.rbegin(), ite = allNodes.rend(); it != ite; ++it) {
    Node *node = *it;
    for (G4_INST *inst : *node->getInstructions()) {
      std::stringstream ss;
      inst->emit(ss);
      // 1. NODE
      ofile << node->nodeID << " "
            << " Occu:" << node->getOccupancy()
            << " Earl:" << node->getEarliest() << " Prio:" << node->priority
            << "  " << *inst;
    }
    ofile << "\n";
  }

  for (auto it = allNodes.rbegin(), ite = allNodes.rend(); it != ite; ++it) {
    Node *node = *it;
    // 2. EDGES
    for (Edge &succDep : node->succs) {
      Node *succNode = succDep.getNode();
      if (node->nodeID > succNode->nodeID) {
        fprintf(stderr, "%d->%d\n", node->nodeID, succNode->nodeID);
      }
      auto depType = succDep.getType();
      const char *depTypeStr =
          (depType == RAW || depType == RAW_MEMORY)   ? "RAW"
          : (depType == WAR || depType == WAR_MEMORY) ? "WAR"
          : (depType == WAW || depType == WAW_MEMORY) ? "WAW"
                                                      : "grey";
      uint32_t edgeLatency = getEdgeLatency(node, depType);
      // Example: 30->34[label="4",color="{red|black|yellow}"];
      ofile << node->nodeID << "->" << succNode->nodeID << "[label=\""
            << edgeLatency << "\""
            << ",type=\"" << depTypeStr << "\""
            << "];"
            << "\n";
    }
  }
  ofile.close();
}

// Helper comparator for priority queue. Queue top is for lowest earliest cycle.
struct earlyCmp {
  earlyCmp() = default;
  bool operator()(const Node *n1, const Node *n2) {
    return n1->getEarliest() > n2->getEarliest();
  }
};

// Helper comparator for priority queue. Top has highest priority (critical
// path).
struct criticalCmp {
  criticalCmp() = default;
  bool operator()(const Node *n1, const Node *n2) {
    // Critical Path Heuristic
    int prio1 = n1->getPriority();
    int prio2 = n2->getPriority();
    if (prio1 != prio2) {
      // Favor node with highest priority
      return prio1 < prio2;
    }
    // Break Ties
    else {
      // 1. Favor sends over non-sends
      bool n1_isSend = (*n1->getInstructions()).front()->isSend();
      bool n2_isSend = (*n2->getInstructions()).front()->isSend();
      if (n1_isSend != n2_isSend) {
        return n1_isSend < n2_isSend;
      }
      // 2. Favor instruction with lower earliest cycle
      //    This usually maintains instruction order
      int n1_earliest = n1->getEarliest();
      int n2_earliest = n2->getEarliest();
      if (n1_earliest != n2_earliest) {
        return n1_earliest > n2_earliest;
      }
      // 3. Favor instructions earliest in the code
      //    NOTE: This is new.
      else {
        return (*n1->getInstructions()).front()->getLocalId() >
               (*n2->getInstructions()).front()->getLocalId();
      }
    }
  }
};

struct criticalCmpForMad {
  criticalCmpForMad() = default;
  bool operator()(const Node *n1, const Node *n2) {
    return (n1->getInstructions()->front()->getVISAId() >
            n2->getInstructions()->front()->getVISAId());
  }
};

// 1).The priority queue is ordered as original sequence order.
// 2).If there is a mad instruction be scheduled, trying to search the candidate
// which has read suppression in src1 and src2. 3).The scheduling is only
// applied to the BB whose instructions are mostly mad. 4).This scheduling is
// not for general instruction scheduling, it's controlled by option
// vISA_ScheduleForReadSuppression
uint32_t DDD::listScheduleForSuppression(G4_BB_Schedule *schedule) {
  if (getOptions()->getOption(vISA_DumpDagDot)) {
    dumpDagDot(schedule->getBB());
    dumpNodes(schedule->getBB());
  }

  // All nodes in root set have no dependency
  // so they can be immediately scheduled,
  // and hence are added to readyList.
  std::priority_queue<Node *, std::vector<Node *>, criticalCmpForMad> readyList;

  // Nodes with their predecessors scheduled are pushed into the preReadyQueue
  // They only get pushed into the real readyList if they are ready to issue,
  // that is their earliest cycle is >= than the current schedule cycle.
  std::priority_queue<Node *, std::vector<Node *>, earlyCmp> preReadyQueue;

  collectRoots();
  for (auto N : Roots) {
    preReadyQueue.push(N);
  }

  // The scheduler's clock.
  // This counter is updated in each loop iteration and its
  // final value represents number of cycles the kernel would
  // take to execute on the GPU as per the model.
  uint32_t currCycle = 0;

  // Used to avoid WAW subreg hazards
  Node *lastScheduled = nullptr;

  // Keep scheduling until both readyList or preReadyQueue contain instrs.
  while (!(readyList.empty() && preReadyQueue.empty())) {
    Node *readyCandidate =
        preReadyQueue.empty() ? nullptr : preReadyQueue.top();
    // While non empty, move nodes from preReadyQueue to readyList
    while (readyCandidate) {
      readyList.push(readyCandidate);
      preReadyQueue.pop();
      readyCandidate = preReadyQueue.empty() ? nullptr : preReadyQueue.top();
    }
    // Nothing to issue at this cycle, increment scheduler clock
    if (readyList.empty()) {
      // readyCandidate is not nullptr. Proof: If it was nullptr, then both
      // preReadyQueue and readyList would be empty. But this cannot
      // happen because the while() loop will only enter if at least
      // one of them is non-empty.
      vISA_ASSERT(readyCandidate, "Both readyList and preReadyQ empty!");
      currCycle = readyCandidate->earliest;
      continue;
    }

    // Pointer to node to be scheduled.
    Node *scheduled = readyList.top();
    readyList.pop();
    if (lastScheduled && kernel->fg.builder->hasReadSuppression() &&
        (readyList.size() > 0)) {
      if (lastScheduled->getInstructions()->size() == 1) {
        G4_INST *inst = lastScheduled->getInstructions()->front();
        if (inst->opcode() == G4_mad || inst->opcode() == G4_dp4a) {
          std::vector<Node *> popped;
          const int searchSize = (int)readyList.size();

          G4_INST *scheduledInst = scheduled->getInstructions()->front();
          if (!((scheduledInst->opcode() == G4_mad ||
                 scheduledInst->opcode() == G4_dp4a) &&
                hasReadSuppression(inst, scheduledInst,
                                   inst->getExecSize() == g4::SIMD8))) {
            for (int i = 0; i < searchSize; ++i) {
              Node *next = readyList.top();
              if ((next->getInstructions()->size() == 1)) {
                G4_INST *nextInst = next->getInstructions()->front();
                readyList.pop();
                if ((nextInst->opcode() == G4_mad ||
                     nextInst->opcode() == G4_dp4a) &&
                    hasReadSuppression(inst, nextInst,
                                       inst->getExecSize() == g4::SIMD8)) {
                  readyList.push(scheduled);
                  scheduled = next;
                  break;
                } else {
                  popped.push_back(next);
                }
              }
            }

            for (auto nodes : popped) {
              readyList.push(nodes);
            }
          }
        }
      }
    }

    vISA_ASSERT(scheduled, "Must have found an instruction to schedule by now");

    // Append the scheduled node to the end of the schedule.
    schedule->scheduledNodes.push_back(scheduled);
    lastScheduled = scheduled;

    // Set the cycle at which this node is scheduled.
    scheduled->schedTime = currCycle;

    for (auto &curSucc : scheduled->succs) {
      Node *succ = curSucc.getNode();
      // Recompute the earliest time for each successor.
      if (scheduled->isLabel()) {
        succ->earliest = 0;
      } else {
        // Update the earliest time of the successor and set its last scheduled
        // predecessor with the largest latency to the currently scheduled node
        // if the latency incurred by scheduling the successor right after the
        // "scheduled" node is larger than successor's earliest time.
        uint32_t latencyToAdd = 0;
        uint32_t earliestNew = 0;
        latencyToAdd = curSucc.getLatency() > scheduled->getOccupancy()
                           ? curSucc.getLatency()
                           : scheduled->getOccupancy();
        earliestNew = scheduled->schedTime + latencyToAdd;

        if (succ->earliest <= earliestNew || !succ->lastSchedPred) {
          succ->lastSchedPred = scheduled;
        }
        succ->earliest =
            (succ->earliest > earliestNew) ? succ->earliest : earliestNew;
      }
      // Decrease the number of predecessors not scheduled for the successor
      // node.
      if ((--(succ->predsNotScheduled)) == 0) {
        preReadyQueue.push(succ);
      }
    }

    // Increment the scheduler's clock after each scheduled node
    currCycle += scheduled->getOccupancy();
  }

  // Sanity check: Make sure all nodes are scheduled
#if defined(_DEBUG)
  for (auto node : allNodes) {
    vISA_ASSERT(node->schedTime != UINT_MAX, "Node not scheduled!");
  }
#endif
  return currCycle;
}

// Scheduling for the 2xDP pipeline
uint32_t DDD::listScheduleFor2xFP(G4_BB_Schedule *schedule) {
  if (getOptions()->getOption(vISA_DumpDagDot)) {
    dumpDagDot(schedule->getBB());
    dumpNodes(schedule->getBB());
  }

  // All nodes in root set have no dependency
  // so they can be immediately scheduled,
  // and hence are added to readyList.
  // 2xSP specific, the original order will be kept.
  std::priority_queue<Node *, std::vector<Node *>, criticalCmpForMad> readyList;

  // Nodes with their predecessors scheduled are pushed into the preReadyQueue
  // They only get pushed into the real readyList if they are ready to issue,
  // that is their earliest cycle is >= than the current schedule cycle.
  std::priority_queue<Node *, std::vector<Node *>, earlyCmp> preReadyQueue;

  auto updateForSucc = [&](Node *scheduled,
                           std::priority_queue<Node *, std::vector<Node *>,
                                               earlyCmp> *preReadyQueue) {
    for (auto &curSucc : scheduled->succs) {
      Node *succ = curSucc.getNode();
      // Recompute the earliest time for each successor.
      if (scheduled->isLabel()) {
        succ->earliest = 0;
      } else {
        // Update the earliest time of the successor and set its last scheduled
        // predecessor with the largest latency to the currently scheduled node
        // if the latency incurred by scheduling the successor right after the
        // "scheduled" node is larger than successor's earliest time.
        uint32_t latencyToAdd = curSucc.getLatency();
        uint32_t earliestNew = 0;
        latencyToAdd = latencyToAdd > scheduled->getOccupancy()
                           ? latencyToAdd
                           : scheduled->getOccupancy();
        earliestNew = scheduled->schedTime + latencyToAdd;

        if (succ->earliest <= earliestNew || !succ->lastSchedPred) {
          succ->lastSchedPred = scheduled;
        }
        succ->earliest =
            (succ->earliest > earliestNew) ? succ->earliest : earliestNew;
      }
      // Decrease the number of predecessors not scheduled for the successor
      // node.
      if ((--(succ->predsNotScheduled)) == 0) {
        preReadyQueue->push(succ);
      }
    }
  };

  auto scheduleSingleInst = [&](window2xSP *curBlock,
                                std::vector<Node *> &popped,
                                uint32_t *currCycle) {
    // Push back the nodes in current block to ready list
    for (auto node : curBlock->instList) {
      if (node != nullptr) {
        readyList.push(node);
      }
    }

    // push back the nodes in popped list to ready list
    for (auto node : popped) {
      readyList.push(node);
    }

    // Only schedule single instruction
    Node *scheduled = readyList.top();
    readyList.pop();
    schedule->scheduledNodes.push_back(scheduled);
    // Update succ
    updateForSucc(scheduled, &preReadyQueue);
    scheduled->schedTime = *currCycle;
    *currCycle += scheduled->getOccupancy();
  };

  collectRoots();
  for (auto N : Roots) {
    preReadyQueue.push(N);
  }

  // The scheduler's clock.
  // This counter is updated in each loop iteration and its
  // final value represents number of cycles the kernel would
  // take to execute on the GPU as per the model.
  uint32_t currCycle = 0;

  // The blocks are used for the 2xDP and 2xSP instruction block, which will be
  // scheduled at the same time Since the atomic is set according to the
  // dependence of two contiguous blocks, we keep two blocks with two block
  // pointers to use them alternatively.
  window2xSP madBlock1(kernel);
  window2xSP madBlock2(kernel);
  window2xSP *curBlock = &madBlock1;
  window2xSP *lastBlock = &madBlock2;

  // Keep scheduling until both readyList or preReadyQueue contain instrs.
  while (!(readyList.empty() && preReadyQueue.empty())) {
    Node *readyCandidate =
        preReadyQueue.empty() ? nullptr : preReadyQueue.top();
    // While non empty, move nodes from preReadyQueue to readyList
    while (readyCandidate) {
      readyList.push(readyCandidate);
      preReadyQueue.pop();
      readyCandidate = preReadyQueue.empty() ? nullptr : preReadyQueue.top();
    }
    // Nothing to issue at this cycle, increment scheduler clock :)
    if (readyList.empty()) {
      // readyCandidate is not nullptr. Proof: If it was nullptr, then both
      // preReadyQueue and readyList would be empty. But this cannot
      // happen because the while() loop will only enter if at least
      // one of them is non-empty.
      vISA_ASSERT(readyCandidate, "Both readyList and preReadyQ empty!");
      currCycle = readyCandidate->earliest;
      continue;
    }

    // Store the node popped from readyList but not in block
    std::vector<Node *> popped;

    // Traverse the readyList to try to build the 2xDP/2xSP block
    int searchSize = (int)readyList.size();
    for (int i = 0; i < searchSize; ++i) {
      bool isAddedToBlock =
          false; // Used to see if the node has been added into block
      Node *curNode = readyList.top();

      // On PVC_XT+ platforms, check if the instruction can be added into the
      // block
      if ((curNode->getInstructions()->size() == 1) &&
          getBuilder()->has2xDP() && curBlock->canAddToBlock2xDP(curNode)) {
        readyList.pop();
        curBlock->push(curNode);
        isAddedToBlock = true;

        // Current block is full
        if (curBlock->isBlockFull()) {
          break;
        }
      }

      // On XE2+ platforms, 2xSP covers 2xDP cases.
      // Try to add the instruction to the block if it's not covered by above
      // 2xDP check
      if ((false == isAddedToBlock) &&
          (curNode->getInstructions()->size() == 1) &&
          getBuilder()->has2xSP() && curBlock->canAddToBlock2xSP(curNode)) {
        readyList.pop();
        curBlock->push(curNode);
        isAddedToBlock = true;

        // Current block is full
        if (curBlock->isBlockFull()) {
          break;
        }
      }

      // Current instruction can't be added into current block, add it into the
      // popped
      if (!isAddedToBlock) {
        readyList.pop();
        popped.push_back(curNode);
      }
    }

    // Check if current block is a good block
    if (curBlock->isGoodBlock()) {
      // Try to see if we can schedule current block. If pre-ready queue updated
      // by the block scheduling is empty, which means some instructions in
      // popped list depended by next block should be scheduled firstly. Then
      // schedule current block. Considering below case:
      //
      //    mad (16) r5.0<1>:df  r92.0<1;0>:df  r45.0<1;0>:df r84.0<0;0>:df
      //    mad (16) r7.0<1>:df  r96.0<1;0>:df  r45.0<1;0>:df  r84.1<0;0>:df
      //    mov (32) r86.0<1>:d  r17.0<1;1,0>:d
      //    mad (16) r9.0<1>:df  r100.0<1;0>:df r45.0<1;0>:df  r84.2<0;0>:df
      //    mad (16) r11.0<1>:df r104.0<1;0>:df  r45.0<1;0>:df  r84.3<0;0>:df
      //
      //    mad (16) r5.0<1>:df  r5.0<1;0>:df  r108.0<1;0>:df r86.0<0;0>:df
      //    mad (16) r7.0<1>:df  r7.0<1;0>:df  r108.0<1;0>:df  r86.1<0;0>:df
      //    mad (16) r9.0<1>:df  r9.0<1;0>:df  r108.0<1;0>:df  r86.2<0;0>:df
      //    mad (16) r11.0<1>:df r11.0<1;0>:df  r108.0<1;0>:df  r86.3<0;0>:df
      //
      // The last 4 mad depends on previous 4 mad and mov. In current block, we
      // have built the block successfully which includes the first
      // 4 instructions, and the popped list has the mov instruction. If
      // schedule the block now, then the next scheduled instruction will be
      // the mov as the last 4 mad wouldn't be scheduled before all the
      // previous 5 instructions scheduled. So the group of blocks will be
      // broken. Todo: Is there better solution here?

      // Schedule instructions in popped list
      for (auto node : popped) {
        schedule->scheduledNodes.push_back(node);
        updateForSucc(node, &preReadyQueue);
        node->schedTime = currCycle;
        currCycle += node->getOccupancy();
      }

      // Schedule instructions in block together
      for (auto node : curBlock->instList) {
        schedule->scheduledNodes.push_back(node);
        updateForSucc(node, &preReadyQueue);
        node->schedTime = currCycle;
        currCycle += node->getOccupancy();
      }

      window2xSP *tmpBlock = lastBlock;
      lastBlock = curBlock;
      curBlock = tmpBlock;
      curBlock->clean();
      curBlock->setPreBlock(lastBlock);
    } else // Not good block
    {
      scheduleSingleInst(curBlock, popped, &currCycle);

      // clean the blocks
      lastBlock->clean();
      curBlock->clean();
    }
  }

  // Sanity check: Make sure all nodes are scheduled
#if defined(_DEBUG)
  for (auto node : allNodes) {
    vISA_ASSERT(node->schedTime != UINT_MAX, "Node not scheduled!");
  }
#endif
  return currCycle;
}

// Perform local list scheduling
uint32_t DDD::listSchedule(G4_BB_Schedule *schedule) {
  if (getOptions()->getOption(vISA_DumpDagDot)) {
    dumpDagDot(schedule->getBB());
    dumpNodes(schedule->getBB());
  }

  // All nodes in root set have no dependency
  // so they can be immediately scheduled,
  // and hence are added to readyList.
  std::priority_queue<Node *, std::vector<Node *>, criticalCmp> readyList;

  // Nodes with their predecessors scheduled are pushed into the preReadyQueue
  // They only get pushed into the real readyList if they are ready to issue,
  // that is their earliest cycle is >= than the current schedule cycle.
  std::priority_queue<Node *, std::vector<Node *>, earlyCmp> preReadyQueue;

  // The scheduler's clock.
  // This counter is updated in each loop iteration and its
  // final value represents number of cycles the kernel would
  // take to execute on the GPU as per the model.
  uint32_t currCycle = 0;

  // Used to avoid WAW subreg hazards
  Node *lastScheduled = nullptr;

  // Send queue, used to emulate the send queue in HW
  SendQueue sQueue(*kernel);
  SendQueue oQueue(*kernel);
  // The Stall cycle because of the full of the send queue
  uint32_t sQueueStallCycle = 0;
  uint32_t oQueueStallCycle = 0;

  auto updateForSucc = [&](Node *scheduled) {
    for (auto &curSucc : scheduled->succs) {
      Node *succ = curSucc.getNode();
      // Recompute the earliest time for each successor.
      if (scheduled->isLabel()) {
        succ->earliest = 0;
      } else {
        // Update the earliest time of the successor and set its last scheduled
        // predecessor with the largest latency to the currently scheduled node
        // if the latency incurred by scheduling the successor right after the
        // "scheduled" node is larger than successor's earliest time.
        uint32_t latencyToAdd = curSucc.getLatency();
        uint32_t earliestNew = 0;
        latencyToAdd = latencyToAdd > scheduled->getOccupancy()
                           ? latencyToAdd
                           : scheduled->getOccupancy();
        earliestNew = scheduled->schedTime + latencyToAdd;

        if (succ->earliest <= earliestNew || !succ->lastSchedPred) {
          succ->lastSchedPred = scheduled;
        }
        succ->earliest =
            (succ->earliest > earliestNew) ? succ->earliest : earliestNew;
      }
      // Decrease the number of predecessors not scheduled for the successor
      // node.
      if ((--(succ->predsNotScheduled)) == 0) {
        preReadyQueue.push(succ);
      }
    }
  };

  auto getStepCycle = [&](Node *n, uint32_t currCycle) -> uint32_t {
    uint32_t stepCycle = 1;
    for (unsigned i = PIPE_INT; i < PIPE_ALL; i++) {
      if (latestInstOfEachPipe[i] == nullptr) {
        continue;
      }

      if ((latestInstOfEachPipe[i]->schedTime +
           latestInstOfEachPipe[i]->getOccupancy()) <= currCycle) {
        latestInstOfEachPipe[i] = nullptr;
        continue;
      }

      if (i == n->instPipe) {
        stepCycle = std::max(
            stepCycle, latestInstOfEachPipe[i]->schedTime +
                           latestInstOfEachPipe[i]->getOccupancy() - currCycle);
      } else {
        if (latestInstOfEachPipe[i]->schedTime + 1 > currCycle) {
          stepCycle = std::max(stepCycle, latestInstOfEachPipe[i]->schedTime +
                                              1 - currCycle);
        }
      }
    }

    return stepCycle;
  };

  auto updateForScheduled = [&](Node *scheduled) {
    // Append the scheduled node to the end of the schedule.
    schedule->scheduledNodes.push_back(scheduled);
    lastScheduled = scheduled;
    if (scheduled->getInstructions()->front()->isDpas()) {
      schedule->preDPASNodeVec.push_back(scheduled);
    } else {
      schedule->preDPASNodeVec.clear();
    }
    if (getOptions()->getOption(vISA_SendQueueSched) &&
        scheduled->getInstructions()->front()->isSend()) {
      // Set the cycle at which this node is scheduled.
      // Add stall cycle from send queue if there is stall happened
      G4_INST *inst = *scheduled->getInstructions()->begin();
      if (inst->getMsgDesc()->isSampler()) {
        scheduled->schedTime = currCycle + sQueueStallCycle;
      } else {
        scheduled->schedTime = currCycle + oQueueStallCycle;
      }
    } else {
      // Set the cycle at which this node is scheduled.
      scheduled->schedTime = currCycle;
    }


    updateForSucc(scheduled);
    if (getOptions()->getOption(vISA_multiplePipeSched)) {
      // Increment the scheduler's clock after each scheduled node
      currCycle += getStepCycle(scheduled, currCycle);

      latestInstOfEachPipe[scheduled->instPipe] = scheduled;
    } else {
      // Increment the scheduler's clock after each scheduled node
      currCycle += scheduled->getOccupancy();
    }
  };

  auto scheduleForSuppression = [&]() -> bool {
    return !readyList.empty() && lastScheduled &&
        kernel->fg.builder->hasReadSuppression() &&
        getOptions()->getuInt32Option(vISA_ReadSuppressionDepth) != 0 &&
        lastScheduled->getInstructions()->size() == 1 &&
        (lastScheduled->getInstructions()->front()->opcode() == G4_mad ||
         lastScheduled->getInstructions()->front()->opcode() == G4_dp4a);
  };

  auto applySuppressionHeuristic = [&](Node *scheduled, Node *lastScheduled)
      -> Node * {
    G4_INST *inst = lastScheduled->getInstructions()->front();
    std::vector<Node *> popped;
    const size_t searchSize = std::min(
        (size_t)getOptions()->getuInt32Option(vISA_ReadSuppressionDepth),
        readyList.size());
    for (size_t i = 0; i < searchSize; ++i) {
      Node *next = readyList.top();
      if ((next->getInstructions()->size() != 1))
        continue;

      G4_INST *nextInst = next->getInstructions()->front();
      readyList.pop();
      if ((nextInst->opcode() == G4_mad || nextInst->opcode() == G4_dp4a) &&
          hasReadSuppression(inst, nextInst, false)) {
        readyList.push(scheduled);
        scheduled = next;
        break;
      } else {
        popped.push_back(next);
      }
    }
    for (auto nodes : popped) {
      readyList.push(nodes);
    }
    return scheduled;
  };

  // Enable DPAS scheduling
  auto scheduleForDpas = [&]() -> bool {
    return (!readyList.empty() || !preReadyQueue.empty()) && lastScheduled &&
           kernel->fg.builder->hasDPAS() &&
           getOptions()->getOption(vISA_scheduleforDPASMacro) &&
           schedule->preDPASNodeVec.size();
  };

  // Check if depends on just scheduled previous DPAS intruction
  auto hasDepOnPreDpas = [&](Node *node,
                             std::vector<Node *> &preDPASNodeVec) -> bool {
    for (auto predNode : preDPASNodeVec) {
      auto iter = std::find_if(node->preds.begin(), node->preds.end(),
                               [=](Edge e) { return e.getNode() == predNode; });
      if (iter != node->preds.end()) {
        return true;
        break;
      }
    }
    return false;
  };

  // Add node back to scheduling list/queue
  auto addBackReadyList = [&](int i, int readyListSize, Node *node) -> void {
    if (i < readyListSize) {
      readyList.push(node);
    } else {
      preReadyQueue.push(node);
    }
  };

  // Apply DPAS heuristic, pick the DPAS instruction which has no dependence
  // with scheduled ones, prefer the instruction with read suppression in source
  // operands.
  auto applyDpasHeuristic = [&](Node *scheduled,
                                Node *lastScheduled) -> Node * {
    G4_INST *inst = lastScheduled->getInstructions()->front();
    Node *reScheduled = nullptr;

    // Last scheduled instruction must be DPAS instruction
    if (!inst->isDpas())
      return nullptr;

    Node *macroCandidate = nullptr;
    bool hasDep = false;
    // Check if current scheduled is the macro candidate
    G4_INST *scheduledInst = scheduled->getInstructions()->front();
    if ((scheduled->getInstructions()->size() == 1) &&
        scheduledInst->isDpas() &&
        inst->asDpasInst()->checksMacroTypes(*scheduledInst->asDpasInst())) {
      hasDep = hasDepOnPreDpas(scheduled, schedule->preDPASNodeVec);
      if (!hasDep && hasDpasReadSuppression(inst, scheduledInst)) {
        return scheduled; // It's good candidate already, don't return nullptr
                          // which may be changed by other heuritics
      }
      if (!hasDep) { // scheduled itself is a candidate
        macroCandidate = scheduled;
      }
    }

    // Prepare the scan vector which contains all nodes from readyList and
    // preReadyQueue
    std::vector<Node *> readyNodeVec;
    const size_t readyListSize = readyList.size();
    while (!readyList.empty()) {
      Node *readyNode = readyList.top();
      readyList.pop();
      readyNodeVec.push_back(readyNode);
    }
    [[maybe_unused]] const size_t preQueueSize = preReadyQueue.size();
    while (!preReadyQueue.empty()) {
      Node *preReadyNode = preReadyQueue.top();
      preReadyQueue.pop();
      readyNodeVec.push_back(preReadyNode);
    }

    int i = 0;
    bool hasSuppressoinCandidate = false;
    for (auto node : readyNodeVec) {
      G4_INST *nodeInst = node->getInstructions()->front();

      // Not candidate, push back to original queue
      if ((node->getInstructions()->size() != 1) || !nodeInst->isDpas() ||
          !inst->asDpasInst()->checksMacroTypes(*nodeInst->asDpasInst()) ||
          hasDepOnPreDpas(node, schedule->preDPASNodeVec)) {
        addBackReadyList(i, readyListSize, node);
        i++;
        continue;
      }

      // Find one with read suppression, stop scan
      if (hasDpasReadSuppression(inst, nodeInst)) {
        // If there is a candidate without read suppression, push candidate to
        // ready list
        if (macroCandidate && macroCandidate != scheduled) {
          readyList.push(macroCandidate);
          macroCandidate = nullptr;
        }
        hasSuppressoinCandidate = true;
        reScheduled = node;
        i++;
        break;
      }

      // Has candidate already, add back to original list
      if (macroCandidate) {
        addBackReadyList(i, readyListSize, node);
      } else {
        macroCandidate = node;
      }

      i++;
    }

    // No read suppresion candidate, and reschedule happened
    if (!hasSuppressoinCandidate && macroCandidate) {
      reScheduled = macroCandidate;
    }

    // Add not scanned nodes back to list/queue
    for (size_t j = i; j < readyNodeVec.size(); j++) {
      addBackReadyList(j, readyListSize, readyNodeVec[j]);
    }

    // If rescheduled, push the scheduled to readyList
    if (reScheduled && reScheduled != scheduled) {
      readyList.push(scheduled);
    }

    // The total size of readyList and preReadyQueue shouldn't be changed
    assert(readyListSize + preQueueSize ==
           readyList.size() + preReadyQueue.size());

    return reScheduled;
  };

  // try to avoid bank conflict for Xe
  // Such as in following case:
  // r40 and r56 are from same bundle and have conflict
  // scheduling next instruction up can avoid the conflict.
  // mad r10   r20  r33 r40        mad r10   r20  r33 r40
  // mad r12   r56  r61  r70  -->  mad r14   r58 r 63  r72
  // mad r14   r58 r 63  r72       mad r12   r56  r61  r70
  auto scheduleForBankConflictReduction = [&](Node *scheduled) -> bool {
    return !readyList.empty() && lastScheduled &&
        kernel->fg.builder->hasEarlyGRFRead() &&
        lastScheduled->hasConflict(scheduled);
  };

  auto applyBankConflictReductionHeuristic = [&](Node *scheduled,
      Node *lastScheduled) -> Node * {
    std::vector<Node *> popped;
    const size_t searchSize = std::min((size_t)3, readyList.size());
    for (size_t i = 0; i < searchSize; ++i) {
      Node *next = readyList.top();
      readyList.pop();
      if (!lastScheduled->hasConflict(next)) {
        readyList.push(scheduled);
        scheduled = next;
        break;
      } else {
        popped.push_back(next);
      }
    }
    for (auto nodes : popped) {
      readyList.push(nodes);
    }
    return scheduled;
  };

  auto getHigestOccupancyStallPipe = [&](uint32_t currCycle) -> SB_INST_PIPE {
    SB_INST_PIPE pipe = PIPE_NONE;
    uint32_t mostOccupancyStallCycle = 0;
    for (unsigned i = PIPE_INT; i < PIPE_ALL; i++) {
      if (latestInstOfEachPipe[i] == nullptr) {
        continue;
      }

      if ((latestInstOfEachPipe[i]->schedTime +
           latestInstOfEachPipe[i]->getOccupancy()) <= currCycle) {
        latestInstOfEachPipe[i] = nullptr;
        continue;
      }

      if (latestInstOfEachPipe[i]->schedTime +
              latestInstOfEachPipe[i]->getOccupancy() >
          mostOccupancyStallCycle) {
        pipe = (SB_INST_PIPE)i;
        mostOccupancyStallCycle = latestInstOfEachPipe[i]->schedTime +
                                  latestInstOfEachPipe[i]->getOccupancy();
      }
    }

    return pipe;
  };

  // Try to avoid b2b math if possible as there are pipeline stalls.
  auto scheduleForB2BMathReduction = [&](Node *scheduled) -> bool {
    return !readyList.empty() && lastScheduled &&
        scheduled->getInstructions()->front()->isMath() &&
        lastScheduled->getInstructions()->front()->isMath();
  };

  auto applyB2BMathReductionHeuristic = [&](Node *scheduled,
                                            Node *lastScheduled) -> Node * {
    // pick another node on the ready list if it's not math and won't cause
    // a longer stall to save compile time we currently limit search size to 2
    std::vector<Node *> popped;
    const size_t searchSize = std::min((size_t)2, readyList.size());
    for (size_t i = 0; i < searchSize; ++i) {
      Node *next = readyList.top();
      readyList.pop();
      if (!next->getInstructions()->front()->isMath() &&
          next->getEarliest() <
              lastScheduled->schedTime + lastScheduled->getOccupancy()) {
        readyList.push(scheduled);
        scheduled = next;
        break;
      } else {
        // keep searching
        popped.push_back(next);
      }
    }
    for (auto nodes : popped) {
      readyList.push(nodes);
    }
    return scheduled;
  };

  auto applyMultiplePipelineHeuristic = [&](Node *scheduled, SB_INST_PIPE stallPipe) -> Node * {
    // pick another node on the ready list if it's not math and won't cause
    // a longer stall to save compile time we currently limit search size to 2
    std::vector<Node *> popped;
    for (size_t i = 0; i < readyList.size(); ++i) {
      Node *next = readyList.top();
      readyList.pop();
      if (next->instPipe != stallPipe) {
        readyList.push(scheduled);
        scheduled = next;
        break;
      } else {
        // keep searching
        popped.push_back(next);
      }
    }
    for (auto nodes : popped) {
      readyList.push(nodes);
    }
    return scheduled;
  };

  // Avoid WAW subreg hazard by skipping nodes that cause a WAW subreg
  // hazard with the lastScheduled instruction.
  auto scheduleForWAWSubregHazardReduction = [&]() -> bool {
    // If only 1 instruction ready, then there is nothing we can do
    return readyList.size() > 1 && lastScheduled &&
        kernel->fg.builder->avoidWAWSubRegHazard();
  };

  auto applyWAWSubregHazardReductionHeuristic = [&](Node *scheduled,
      Node *lastScheduled) -> Node * {
    // Do not try to fix the hazard for ever (maintain low complexity)
#define WAW_SUBREG_ATTEMPTS 2
    Node *extScheduled = scheduled;
    std::vector<Node *> skippedNodes;
    int lastReg = lastScheduled->writesToSubreg();
    // If lastScheduled writes to a subreg
    int attempts = 0;
    if (lastReg != Node::NO_SUBREG) {
      // While there is a WAW subreg hazard with
      // the scheduled node, get the next from readyList
      while (scheduled->writesToSubreg() == lastReg &&
             !readyList.empty() && ++attempts < WAW_SUBREG_ATTEMPTS) {
        skippedNodes.push_back(scheduled);
        scheduled = readyList.top();
        readyList.pop();
      }
    }

    // Re-insert the skipped nodes into the readyList
    for (Node *poppedNode : skippedNodes) {
      readyList.push(poppedNode);
    }

    // If we have reached the end of the readyList but we still get a WAW
    // subreg hazard, choose the top of readyList
    if (scheduled != extScheduled &&
        scheduled->writesToSubreg() == lastReg &&
        (readyList.empty() || attempts >= WAW_SUBREG_ATTEMPTS)) {
      // Re-insert the scheduled node into the readyList
      readyList.push(scheduled);
      // Get the highest priority node
      scheduled = readyList.top();
      readyList.pop();
    }
    return scheduled;
  };

  // For write combine feature
  auto scheduleForWriteCombine = [&](Node *scheduled) -> bool {
    return !readyList.empty() && kernel->fg.builder->hasWriteCombine() &&
        getOptions()->getOption(vISA_writeCombine) &&
        scheduled->getInstructions()->front()->opcode() == G4_mov;
  };

  auto applyWriteCombineHeuristic = [&](windowWriteCombine &block,
                                        Node *scheduled) {
    if (block.isWriteCombineCandidate(scheduled, schedule->getBB())) {
      block.push(scheduled);

      // build the write combine block from other nodes of readyList
      while (!readyList.empty()) {
        Node *next = readyList.top();
        if (next->getInstructions()->size() == 1 &&
            block.isWriteCombineCandidate(next, schedule->getBB())) {
          readyList.pop();
          block.push(next);
        } else {
          break;
        }
      }

      while (block.getInstList().size() > 1) {
        if (block.isGoodBlock())
          break;

        // pop up the last node from block, and try to see if it is a good
        // block
        Node *tmpNode = block.getInstList().back();
        block.pop();
        readyList.push(tmpNode);
      }
    }
  };

  auto applySendQueueHeuristic = [&](SendQueue &sQueue,
                                     uint32_t &sendQueueStallCycle,
                                     Node *scheduled) {
    Node *rescheduled = nullptr;
    // Fixeme: currently, the send source register read latency is used as
    // dequeue cycles. Update if it's not accurate
    unsigned sendLatencyOfScheduled =
        LT.getSendSrcReadLatency(scheduled->getInstructions()->front());

    //Update Queue according to current cycle
    sQueue.updateCycle(currCycle);

    if (sQueue.isFull()) {//Stall will happen if no change
      Node *topNode = sQueue.front();
      int sendLatency = LT.getSendSrcReadLatency(topNode->getInstructions()->front());
      sendLatency -= sQueue.size();
      sendQueueStallCycle = sendLatency;

      // Because of stall cycle, adjust the preReadyQueue and readyList
      while (!preReadyQueue.empty()) {
        Node *readyCandidate = preReadyQueue.top();
        if (readyCandidate->earliest > (currCycle + sendQueueStallCycle))
          break;
        readyList.push(readyCandidate);
        preReadyQueue.pop();
      }

      // Find candidate which is not send in the readyList. For none-candidates,
      // have to be popped to a temp list, which will be added back.
      std::vector<Node *> instList;
      [[maybe_unused]] size_t readyListSize = readyList.size();
      while (!readyList.empty()) {
        Node *candidate = readyList.top();
        readyList.pop();
        if (!candidate->getInstructions()->front()->isSend()) {
          rescheduled = candidate;
          break;
        }
        instList.push_back(candidate);
      }

      // If rescheduled, add the previous scheduled into list to add back to
      // queue. Otherwise, push into send queue. Note the send queue itself will
      // pop top item if it's full.
      if (rescheduled) {
        instList.push_back(scheduled);
      } else {
        rescheduled = scheduled;
        sQueue.push(rescheduled, currCycle + sendLatencyOfScheduled);
      }

      // Add back to readyList
      for (auto N : instList) {
        readyList.push(N);
      }
      assert(readyList.size() == readyListSize);
    } else {//Not full, just push to queue
      rescheduled = scheduled;
      sQueue.push(scheduled, currCycle + sendLatencyOfScheduled);
    }

    return rescheduled;
  };

  collectRoots();
  for (auto N : Roots) {
    preReadyQueue.push(N);
  }

  // Keep scheduling until both readyList or preReadyQueue contain instrs.
  while (!(readyList.empty() && preReadyQueue.empty())) {
    Node *readyCandidate =
        preReadyQueue.empty() ? nullptr : preReadyQueue.top();
    // While non empty, move nodes from preReadyQueue to readyList
    while (readyCandidate && readyCandidate->earliest <= currCycle) {
      readyList.push(readyCandidate);
      preReadyQueue.pop();
      readyCandidate = preReadyQueue.empty() ? nullptr : preReadyQueue.top();
    }
    // Nothing to issue at this cycle, increment scheduler clock
    if (readyList.empty()) {
      // readyCandidate is not nullptr. Proof: If it was nullptr, then both
      // preReadyQueue and readyList would be empty. But this cannot
      // happen because the while() loop will only enter if at least
      // one of them is non-empty.
      vISA_ASSERT(readyCandidate, "Both readyList and preReadyQ empty!");
      currCycle = readyCandidate->earliest;
      continue;
    }

    // Pointer to node to be scheduled.
    Node *scheduled = readyList.top();
    readyList.pop();
    // Allow fall-through behavior to try different heuristics if possible.
    // The heuristics are ordered based on their priority.
    Node *heuCandidate = nullptr;
    G4_INST *inst = *scheduled->getInstructions()->begin();

    if (getOptions()->getOption(vISA_SendQueueSched) && inst->isSend()) {
      G4_SendDesc *MsgDesc = inst->getMsgDesc();
      if (MsgDesc->isSampler()) {
        heuCandidate =
            applySendQueueHeuristic(sQueue, sQueueStallCycle, scheduled);
      } else {
        heuCandidate =
            applySendQueueHeuristic(oQueue, oQueueStallCycle, scheduled);
      }
    }

    // Scheduling for DPAS instructions
    if (!heuCandidate && scheduleForDpas()) {
      heuCandidate = applyDpasHeuristic(scheduled, lastScheduled);
      if (heuCandidate) {
        schedule->preDPASNodeVec.push_back(heuCandidate);
      } else {
        schedule->preDPASNodeVec.clear();
      }
    }

    if (!heuCandidate && scheduleForSuppression()) {
      heuCandidate = applySuppressionHeuristic(scheduled, lastScheduled);
    }
    if (!heuCandidate && scheduleForBankConflictReduction(scheduled)) {
      heuCandidate =
          applyBankConflictReductionHeuristic(scheduled, lastScheduled);
    }
    if (!heuCandidate) {
      if (getOptions()->getOption(vISA_multiplePipeSched)) {
        auto occupancyPipe = getHigestOccupancyStallPipe(currCycle);
        if (occupancyPipe != PIPE_NONE &&
            occupancyPipe == scheduled->instPipe) {
          heuCandidate =
              applyMultiplePipelineHeuristic(scheduled, occupancyPipe);
        }
      } else if (scheduleForB2BMathReduction(scheduled)){
        heuCandidate = applyB2BMathReductionHeuristic(scheduled, lastScheduled);
      }
    }
    if (!heuCandidate && scheduleForWAWSubregHazardReduction()) {
      heuCandidate =
          applyWAWSubregHazardReductionHeuristic(scheduled, lastScheduled);
    }
    // Note that write combine heuristic seems special. If successful, it'll
    // schedule multiple nodes together and continue to the next iteration.
    if (!heuCandidate && scheduleForWriteCombine(scheduled)) {
      windowWriteCombine block;
      applyWriteCombineHeuristic(block, scheduled);
      if (block.isGoodBlock()) {
        // add atomic to the instructions except for the last one in the
        // block. Also mark the instructions as doNotDelete.
        std::vector<Node *> &instList = block.getInstList();
        for (size_t id = 0, ie = instList.size(); id < ie; id++) {
          G4_INST *inst = instList[id]->getInstructions()->front();
          inst->markDoNotDelete();
          if (id != ie - 1)
            inst->setOptionOn(InstOpt_Atomic);
        }

        // schedule together
        for (auto node : instList) {
          scheduled = node;
          updateForScheduled(scheduled);
        }

        continue;
      }
    }
    if (heuCandidate)
      scheduled = heuCandidate;

    vISA_ASSERT(scheduled, "Must have found an instruction to schedule by now");

    updateForScheduled(scheduled);
  }

  // Sanity check: Make sure all nodes are scheduled
#if defined(_DEBUG)
  for (auto node : allNodes) {
    vISA_ASSERT(node->schedTime != UINT_MAX, "Node not scheduled!");
  }
#endif
  return currCycle;
}

// This comment is moved from DDD::Latency() Given two instructions, this
// function returns latency in number of cycles. If there is a RAW dependency
// between the two instructions then modeled latency is higher because second
// instruction needs to wait till first instruction execution is complete to
// receive operands. In case of false dependencies or no dependencies, second
// instruction need not wait for completion of execution of first instruction.
// So modeled latency is lower and equal to latency of first instruction (which
// is initialized to either 1 or 2 depending on compression attribute).
uint32_t DDD::getEdgeLatency_old(Node *node, DepType depT) const {
  // This is a prefetch read only depending on the terminator.
  // We pessimistically assume that it will be used right after the branch.
  G4_INST *inst = *node->getInstructions()->begin();
  if (depT == DepType::CONTROL_FLOW_BARRIER && inst->isSend()) {
    G4_SendDesc *msgDesc = inst->getMsgDesc();
    if (msgDesc->isRead()) {
      return LT.getLatency(inst);
    }
  }

  if (depT <= NODEP || depT >= CONTROL_FLOW_BARRIER) {
    if (kernel->fg.builder->modelSendSrcReadLatency() && inst->isSend()) {
      return LT.getSendSrcReadLatency(inst);
    }
    return node->getOccupancy();
  }

  uint32_t latency = IVB_PIPELINE_LENGTH;
  switch (depT) {
  case RAW:
  case RAW_MEMORY:
    latency = LT.getLatency(inst);
    break;

  // FIXME:
  // 1. for WAW/WAR, if the first instruction is send, need to consider
  // getSendSrcReadLatency() as well.
  // 2. UNCOMPR_LATENCY works only when the instructions are from same pipeline.
  // If from different pipelines, getLatency() is the right latency
  case WAW:
  case WAR:
    if (kernel->fg.builder->modelSendSrcReadLatency() && inst->isSend()) {
      latency = LT.getSendSrcReadLatency(inst);
    } else {
      latency = UNCOMPR_LATENCY;
    }
    break;

  case WAR_MEMORY:
  case WAW_MEMORY:
    if (kernel->fg.builder->modelSendSrcReadLatency()) {
      latency = LT.getSendSrcReadLatency(inst);
    } else {
      latency = UNCOMPR_LATENCY;
    }
    break;

  default:
    break;
  }
  latency = latency > node->getOccupancy() ? latency : node->getOccupancy();
  return latency;
}

uint32_t DDD::getEdgeLatency(Node *node, DepType depT) const {
  uint32_t latency = getEdgeLatency_old(node, depT);
  if (useMTLatencies) {
    float scale = float(HWthreadsPerEU) / getBuilder()->getCoIssueUints();
    latency = int(latency / scale);
  }
  return latency;
}

Node::Node(uint32_t id, G4_INST *inst, Edge_Allocator &depEdgeAllocator,
           const LatencyTable &LT)
    : nodeID(id) {
  instVec.push_back(inst);
  occupancy = LT.getOccupancy(inst);

  // Set the initial node priority
  priority = occupancy;

  barrier = CheckBarrier(inst);

  if (!inst->isLabel()) {
    instPipe = inst->getInstructionPipeXe();
  }
}

void LocalScheduler::EmitNode(Node *node) {
  for (G4_INST *inst : *node->getInstructions()) {
    if (inst->isSend()) {
      inst->asSendInst()->emit_send(std::cout);
    } else {
      inst->emit(std::cout);
    }
    std::cout << "\n";
  }
}

/************************************************************************/
/* ================ Below are printout methods only  ================   */
/************************************************************************/

void DDD::DumpDotFile(G4_BB *bb) {

  std::stringstream ss;
  ss << "BB" << bb->getId() << ".nodes.dot";
  auto fileName = ss.str();
  std::ofstream ofile(fileName, std::ios::out);
  if (!ofile) {
    vISA_ASSERT_INPUT(false, "[Scheduling] Cannot open file %s, dump failed",
        fileName.c_str());
  }
  ofile << "digraph "
        << "BB" << bb->getId() << " {\n";
  ofile << "\n"
        << "\t// Setup\n";
  ofile << "\tsize = \"80, 100\";\n";
  ofile << "\n"
        << "\t// Nodes\n";
  auto iNode(allNodes.begin()), endNodes(allNodes.end());
  for (; iNode != endNodes; ++iNode) {
    for (G4_INST *inst : *(*iNode)->getInstructions()) {

      ofile << "\tID_" << (*iNode)->nodeID
            << "\t[shape=record, label=\"{ID : " << (*iNode)->nodeID
            << " PRIORITY : " << (*iNode)->priority << " | "
            << " ETIME : " << (*iNode)->earliest << " | ";
      ofile << ((inst->isLabel()) ? "Label: " : "");
      std::ostringstream os;
      if (inst->isSend()) {
        inst->asSendInst()->emit_send(os);
      } else {
        inst->emit(os);
      }

      std::string dotStr(os.str());
      // TODO: dot doesn't like '<', '>', '{', or '}' this code below is a hack.
      // need to replace with delimiters.
      std::replace_if(
          dotStr.begin(), dotStr.end(),
          std::bind(std::equal_to<char>(), std::placeholders::_1, '<'), '[');
      std::replace_if(
          dotStr.begin(), dotStr.end(),
          std::bind(std::equal_to<char>(), std::placeholders::_1, '>'), ']');
      std::replace_if(
          dotStr.begin(), dotStr.end(),
          std::bind(std::equal_to<char>(), std::placeholders::_1, '{'), '[');
      std::replace_if(
          dotStr.begin(), dotStr.end(),
          std::bind(std::equal_to<char>(), std::placeholders::_1, '}'), ']');

      ofile << dotStr;
      ofile << "\\l";
      ofile << "} \"];"
            << "\n";
    }
  }
  ofile << "\n"
        << "\t// Edges\n";

  for (iNode = allNodes.begin(); iNode != endNodes; ++iNode) {
    Node *node = *iNode;
    EdgeVector::iterator iEdge(node->succs.begin()),
        endEdges(node->succs.end());
    for (; iEdge != endEdges; ++iEdge) {
      ofile << "\tID_" << node->nodeID << " -> "
            << "ID_" << (*iEdge).getNode()->nodeID;
    }
  }
  ofile << " }"
        << "\n";
  ofile.close();
}

void G4_BB_Schedule::emit(std::ostream &out) {
  if (!scheduledNodes.empty()) {
    out << "\n";
    for (uint32_t i = 0; i < (uint32_t)scheduledNodes.size(); i++) {
      for (G4_INST *inst : *scheduledNodes[i]->getInstructions()) {
        out << scheduledNodes[i]->schedTime << ":\t";
        if (!inst->isSend()) {
          inst->emit(out);
        } else {
          inst->asSendInst()->emit_send(out);
        }
        out << "\n";
      }
    }
  }
}

// Check conflicts according to registers read in same cycle
// Two kinds conflict:
// 1. multiple registers from same bundle
// 2. all three sources from same bank
// Input: regCandidates are the registers which will be read in same cycle
static bool hasConflictForSchedule(int *regCandidates) {
  int bundles[3];
  int bankSrcs[2];

  for (int i = 0; i < 2; i++) {
    bankSrcs[i] = 0;
  }
  for (int i = 0; i < 3; i++) {
    bundles[i] = -1;
  }

  for (int i = 0; i < 3; i++) {
    if (regCandidates[i] != -1) {
      int bundleID = regCandidates[i] % 16; // 8 bundles for each bank
      int bankID = regCandidates[i] % 2;

      bundles[i] = bundleID;
      bankSrcs[bankID]++;
    }
  }

  // If there is bundle conflict between the sources from different instructions
  // Note that the instruction internal conflict is not considered here.
  if (bundles[2] != -1 &&
      (bundles[2] == bundles[0] || bundles[2] == bundles[1])) {
    return true;
  }

  // If all three sources are from same bank
  if ((bankSrcs[0] > 2 || bankSrcs[1] > 2)) {
    return true;
  }

  return false;
}

// Xe check BC between adjacent instructions (this, node2)
// Scheduling can only solve the bank conflict between adjacent instructions
// The src0 of second instruction is read together with src1 (and src2) of the
// first instruction. We only handle the situation that first instruction is a
// three source instruction.
bool Node::hasConflict(Node *node2) const {
  G4_INST *inst1 = getInstructions()->front();
  G4_INST *inst2 = node2->getInstructions()->front();

  if (inst1->isSend() || inst2->isSend()) {
    return false;
  }

  // Don't consider two source instructions
  if (inst1->getNumSrc() != 3) {
    return false;
  }

  int prevInstRegs[2][G4_MAX_SRCS] = {};
  int prevInstExecSize[G4_MAX_SRCS] = {};
  int firstRegCandidate[G4_MAX_SRCS] = {};
  int candidateNum = 0;

  // Get the sources of current instruction
  bool instSplit = false;
  const IR_Builder &irb = inst1->getBuilder();
  for (int i = 0; i < inst1->getNumSrc(); i++) {
    prevInstRegs[0][i] = -1;
    prevInstRegs[1][i] = -1;
    G4_Operand *srcOpnd = inst1->getSrc(i);
    if (srcOpnd) {
      if (srcOpnd->isSrcRegRegion() && srcOpnd->asSrcRegRegion()->getBase() &&
          srcOpnd->asSrcRegRegion()->getBase()->isRegVar()) {
        G4_RegVar *baseVar =
            static_cast<G4_RegVar *>(srcOpnd->asSrcRegRegion()->getBase());
        prevInstExecSize[i] =
            srcOpnd->getLinearizedEnd() - srcOpnd->getLinearizedStart() + 1;
        if (baseVar->isGreg()) {
          uint32_t byteAddress = srcOpnd->getLinearizedStart();
          prevInstRegs[0][i] = byteAddress / irb.numEltPerGRF<Type_UB>();

          if (prevInstExecSize[i] > 32) {
            prevInstRegs[1][i] =
                prevInstRegs[0][i] +
                (prevInstExecSize[i] + irb.numEltPerGRF<Type_UB>() - 1) /
                    irb.numEltPerGRF<Type_UB>() -
                1;
            instSplit = true;
          } else {
            // The same register is reused in both SIMD8 instructions
            prevInstRegs[1][i] = prevInstRegs[0][i];
          }
        }
      }
    }
  }

  if (!instSplit) {
    if (prevInstRegs[0][1] != -1) {
      firstRegCandidate[candidateNum] = prevInstRegs[0][1];
      candidateNum++;
    }
    if (prevInstRegs[0][2] != -1) {
      firstRegCandidate[candidateNum] = prevInstRegs[0][2];
      candidateNum++;
    }
  } else // For SIMD16 and SIMD32, if the GRF1 of src1 or src2 of inst 1 is GRF
         // register
  {
    if (prevInstRegs[1][1] != -1) {
      firstRegCandidate[candidateNum] = prevInstRegs[1][1];
      candidateNum++;
    }
    if (prevInstRegs[1][2] != -1) {
      firstRegCandidate[candidateNum] = prevInstRegs[1][2];
      candidateNum++;
    }
  }

  // Get the src0 of the second instruction node2
  G4_Operand *srcOpnd = inst2->getSrc(0);
  if (srcOpnd) {
    if (srcOpnd->isSrcRegRegion() && srcOpnd->asSrcRegRegion()->getBase() &&
        srcOpnd->asSrcRegRegion()->getBase()->isRegVar()) {
      G4_RegVar *baseVar =
          static_cast<G4_RegVar *>(srcOpnd->asSrcRegRegion()->getBase());
      if (baseVar->isGreg()) {
        uint32_t byteAddress = srcOpnd->getLinearizedStart();
        firstRegCandidate[candidateNum] =
            byteAddress / irb.numEltPerGRF<Type_UB>();
        candidateNum++;
      }
    }
  } else {
    return false;
  }

  return hasConflictForSchedule(firstRegCandidate);
}
