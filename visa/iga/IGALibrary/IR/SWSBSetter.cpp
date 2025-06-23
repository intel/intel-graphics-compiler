/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BitSet.hpp"
#include "RegDeps.hpp"
#include "Traversals.hpp"

#include <iterator>
#include <limits>
#include <optional>

using namespace iga;

/**
 * RAW:                     R kill W    R-->live       explict dependence
 * WAW: different pipelines W2 kill W1  W2-->live      explict dependence
 * WAR: different pipelines W kill R    W-->live       explict dependence
 * WAR: same pipeline       W kill R    W-->live       implict dependence
 * AR: sample pipeline     R2 kill R1  R2-->live      no dependence
 * RAR: different pipelines             R1,R2-->live   no dependence
 *
 * Different pipeline
 * send, math, control flow, long/short (type df)
 *
 * add (8) r10 r20 r30
 * add (8) r11 r21 r22
 * if (..)
 *   // if instruction doesn't count in if calculations, but it takes about 6
 *   // cycles to resolve for fall through can treat it as continue BB.
 *   // Only when this BB has one predecessor
 *   add r40 r10 r50 {@2}
 * else
 *   add r60 r70 r80
 * endif
 * //Both control flows converge on this. Conservative analysis start with 1.
 * //By the time jmp happens counter should be at 0 anyway.
 * add r90 r100 r110 {@1}
 * add r91 r101 r111 {@2}
 *
 *
 * Types of Dependencies
 * dst   src0    src1
 * grf   ind     grf // set distance to 1. If SBID list not empty insert test
 * instruction. Optimization if SBID == 1 AND grf depends on it, set SBID, clear
 * SBIDList
 */

/**
    Bucket - represents a GRF line
    ID - sequential numbering of instructions. Resets at 0 with new BB
    Implicit assumption. Various data structures have pointers to Dependencies.

    For each BB scan instructions down
        if new BB
            reset buckets //can use bit mask
            reset distanceTracker
            reset ID
        For each instruction
            Calculate dependcy on srcs and destination
            if currDistance < MAX_DIST
                record distance = currDistance // for a case where we at new BB

            For each dependency and bucket it touches look in to a bucket
                if bucket not empty
                    find potential dependencies //bit mask intersects
                    for each dependency found
                    if appopriate (WAW, RAR, RAW) Dependence exists
                        Clear dependency bits from bucket dependency
                        if dep empty remove from bucket
                        if swsb //no out of order
                            if instDistance > (currDistance - depID)
                                //We found dependence closer
                                record distance = currDistance - depID
   //CurrDistance > depID AND min(currDist - depID, 1) else //sbid record SBID
   ID

            if dependencyRecord NOT empty
                Generate appropriate SWSB/test instruction
                IF SBID
                    if all dependencies are clear
                        add SBID to free list
                        remove entry SBID -> dependencies
            Remove MAX_DIST DEP from buckets
            Add current instruction Dependencies to buckets
            if instruction isVariableExecTime //send, math.idiv
                if freeSBITLIst IS empty
                    pick one SBID
                    generate test instruction
                    move SBID to free list
                    clear dependency from bucket/sbidList

                assign SBID from free list
        if end of block AND SBID list NOT empty
            generate test instructions
*/

#include "SWSBSetter.hpp"
/*
WAW
explicit dependence
math.fc (except idiv) r10 ...
add r10 ....

add r10 ... //long: type DF/Q
add r10 ... //short:

WAW
no dependence
add r10 ...
add r10 ...


Math.sin   r10 r20 r30
Math.cos  r20 r40 r50
Not required - same pipe

Math.sin   r20 r10 r30
Math.cos  r20 r40 r50
Not required - same pipe

FPU_long   r20 r10 r30
Math.sin    r20 r40 r50
Explicit dep required as math can overtake FPU_long - since they are in
different pipes.

RAW
add r10 ...
add r20 ...
add ... r20 ... {@1}
add ... r10  {@3} <--- technically speaking this depending is not necesary
                       since they are in same pipe and previous instruction will
stall so last instruction dependence is cleared. But in terms of runtime there
is no impact so not worth special handling

assuming two grfs are written/read
send r10
send r11

add (16) ... r10 ...
second send has dependency on first send
add has dependency on second send
if sends written 1 grf, and add still read two grfs it will have dependence on
both sends

send r10 //set$1 writes r10/r11
add(8) r10 {$1.dst}
add(8) r11 {}


*/
void SWSBAnalyzer::clearDepBuckets(DepSet &depMatch) {
  for (auto bucketID : depMatch.getBuckets()) {
    auto bucket = &m_buckets[bucketID];
    auto numDepSets = bucket->getNumDependencies();
    for (uint32_t i = 0; i < numDepSets; ++i) {
      DepSet *dep = bucket->getDepSet(i);
      // See if anything matches for this GRF bucket.
      // originally was checking for intersect but was removing extra dependence
      // in case like this
      /*
          (W)      and (1|M0)               r0.0<1>:ud    r0.0<0;1,0>:ud
         0xFFFFBFFF:ud    {} (W)      mov (16|M0)              r25.0<1>:f    0:d
          (W)      mov (1|M0)               r0.2<1>:ud    0x0:ud
          (W)      mov (1|M0)               r0.2<1>:ud    0x0:ud
          (W)      and (1|M0)               r0.0<1>:ud    r0.0<0;1,0>:ud
         0xFFFFF7FF:ud    {} mov (16|M0)              r120.0<1>:ud
         r17.0<8;8,1>:ud mov (16|M0)              r122.0<1>:ud  r19.0<8;8,1>:ud
                   mov (16|M0)              r124.0<1>:ud  r21.0<8;8,1>:ud
                   mov (16|M0)              r126.0<1>:ud  r23.0<8;8,1>:ud
          (W)      mov (16|M0)              r118.0<1>:ud  r0.0<8;8,1>:ud {}
      */
      // the r0 dependece was already cleared by second r0
      // but when clearing from buckets it would find the second r0 and clear it
      // by mistake
      if (dep && depMatch.getInstGlobalID() == dep->getInstGlobalID() &&
          (dep->getDepType() == depMatch.getDepType())) {
        bucket->clearDepSet(i);
      }
    }
  }
  depMatch.reset();
}

void SWSBAnalyzer::checkAccFlagRAW(bool &isRAW, const DepSet &currDep,
                                   const DepSet &targetDep)
{
  // The function must be called when there is RAW dependency detected
  assert(isRAW);

  auto check_dep_reg = [&](const DepSet &in_dep, uint32_t reg_start,
                           uint32_t reg_len) {
    return in_dep.getBitSet().intersects(currDep.getBitSet(), reg_start,
                                          reg_len);
  };
  auto has_grf_dep = [&](const DepSet &in_dep) {
    return check_dep_reg(in_dep, m_DB->getGRF_START(), m_DB->getGRF_LEN());
  };
  auto has_arf_a_dep = [&](const DepSet &in_dep) {
    return check_dep_reg(in_dep, m_DB->getARF_A_START(), m_DB->getARF_A_LEN());
  };
  auto has_acc_dep = [&](const DepSet &in_dep) {
    return check_dep_reg(in_dep, m_DB->getARF_ACC_START(),
                         m_DB->getARF_ACC_LEN());
  };
  auto has_flag_dep = [&](const DepSet &in_dep) {
    return check_dep_reg(in_dep, m_DB->getARF_F_START(), m_DB->getARF_F_LEN());
  };
  auto has_sp_dep = [&](const DepSet &in_dep) {
    return check_dep_reg(in_dep, m_DB->getARF_SPECIAL_START(),
                         m_DB->getARF_SPECIAL_LEN());
  };
  auto has_scalar_dep = [&](const DepSet &in_dep) {
    return check_dep_reg(in_dep, m_DB->getARF_SCALAR_START(),
                         m_DB->getARF_SCALAR_LEN());
  };

  // is acc dependecy
  if (has_acc_dep(targetDep)) {
    // and no dependency on other registers
    if (!(has_grf_dep(targetDep) || has_arf_a_dep(targetDep) ||
        has_flag_dep(targetDep) || has_sp_dep(targetDep)))
      if (!has_scalar_dep(targetDep))
        isRAW = false;
  }

  // is flag dependency
  if (has_flag_dep(targetDep)) {
    // and no dependency on other registers
    if (!(has_grf_dep(targetDep) || has_arf_a_dep(targetDep) ||
        has_acc_dep(targetDep) || has_sp_dep(targetDep)))
      if (!has_scalar_dep(targetDep))
        isRAW = false;
    // flag and acc only
    if (has_acc_dep(targetDep))
      if (!(has_grf_dep(targetDep) || has_arf_a_dep(targetDep) ||
          has_sp_dep(targetDep)))
        if (!has_scalar_dep(targetDep))
          isRAW = false;
  }
}

/**
 * This function takes in a current instruction dependency.
 * Either SRC or DST
 * It then checks against previous dependencies.
 * It sets mininum valid distance
 * and creates an active list of SBIDs this instruction depends on
 * It clears and removes previous dependencies.
 * The approach is bucket based.
 * Each bucket is one GRF.
 * So if instruction writes in to more then one GRF then multiple buckets will
 * have the dependency
 */
void SWSBAnalyzer::calculateDependence(DepSet &currDep,
                                       SWSB &swsb,
                                       const Instruction &currInst,
                                       activeSBIDsTy &activeSBIDs,
                                       bool &needSyncForShootDownInst) {
  needSyncForShootDownInst = false;
  auto currDepType = currDep.getDepType();
  auto currDepPipe = currDep.getDepPipe();

  for (auto bucketID : currDep.getBuckets()) {
    // iterates over Dependencies in a GRF bucket
    // Assumption there shouldn't be more then 1-2
    Bucket *bucket = &m_buckets[bucketID];
    size_t numDepSets = bucket->getNumDependencies();
    for (uint32_t i = 0; i < numDepSets; ++i) {
      uint32_t index = static_cast<uint32_t>(numDepSets - 1 - i);
      auto dep = bucket->getDepSet(index);

      if (dep && (dep->getDepType() == DEP_TYPE::WRITE_ALWAYS_INTERFERE ||
                  dep->getDepType() == DEP_TYPE::READ_ALWAYS_INTERFERE)) {
        // force to sync with dep
        if (dep->getDepClass() == DEP_CLASS::OUT_OF_ORDER) {
          setSbidDependency(*dep, currInst, needSyncForShootDownInst,
                            activeSBIDs);
        } else {
          // Set to sync with all in-order-pipes. WRITE/READ_ALWAYS_INTERFERE
          // could be used to mark arf dependency, which is required to be all
          // pipes instead of dep's pipe only
          swsb.minDist = 1;
          if (getNumOfDistPipe() == 1)
            swsb.distType = SWSB::DistType::REG_DIST;
          else
            swsb.distType = SWSB::DistType::REG_DIST_ALL;
          bucket->clearDepSet(index);
        }
      }

      // See if anything matches for this GRF bucket.
      if (dep && dep->getBitSet().intersects(currDep.getBitSet())) {
        /*
         * RAW:                     R kill W    R-->live       explict
         * dependence WAW: different pipelines W2 kill W1  W2-->live explict
         * dependence WAR: different pipelines W kill R    W-->live explict
         * dependence WAR: same pipeline       W kill R    W-->live implict
         * dependence AR: sample pipeline     R2 kill R1  R2-->live      no
         * dependence RAR: different pipelines             R1,R2-->live   no
         * dependence
         */
        // RAW:                     R kill W    R-->live       explict
        // dependence
        DEP_TYPE prevDepType = dep->getDepType();
        DEP_PIPE prevDepPipe = dep->getDepPipe();
        DEP_CLASS prevDepClass = dep->getDepClass();

        // Send with different SFID could write to different pipes
        bool sendInDiffPipe = false;
        if (dep->getInstruction()->getOpSpec().isAnySendFormat() &&
            currDep.getInstruction()->getOpSpec().isAnySendFormat()) {
          sendInDiffPipe = (dep->getInstruction()->getSendFc() !=
                            currDep.getInstruction()->getSendFc());
          // for send in unknown pipe, always treated as different pipe
          if (!sendInDiffPipe) {
            sendInDiffPipe = dep->getDepPipe() == DEP_PIPE::SEND_UNKNOWN ||
                             currDep.getDepPipe() == DEP_PIPE::SEND_UNKNOWN;
          }
        }

        auto isRead = [](DEP_TYPE ty) {
          return ty == DEP_TYPE::READ ||
                 ty == DEP_TYPE::READ_ALWAYS_INTERFERE;
        };
        auto isWrite = [](DEP_TYPE ty) {
          return ty == DEP_TYPE::WRITE ||
                 ty == DEP_TYPE::WRITE_ALWAYS_INTERFERE;
        };

        bool isRAW = isRead(currDepType) && isWrite(prevDepType);
        // WAW: different pipelines W2 kill W1  W2-->live      explict
        // dependence
        bool isWAW =
            (isWrite(currDepType) && isWrite(prevDepType) &&
             (currDepPipe != prevDepPipe || sendInDiffPipe));
        // WAR: different pipelines W kill R    W-->live       explict
        // dependence
        bool isWAR = isWrite(currDepType) &&
                     isRead(prevDepType) &&
                     (currDepPipe != prevDepPipe || sendInDiffPipe);
        bool isWAW_out_of_order =
            (isWrite(currDepType) && isWrite(prevDepType) &&
             prevDepClass == DEP_CLASS::OUT_OF_ORDER);

        // Special case handling for acc/flag dependency:
        // if the RAW dependency on acc/flag and acc/flag only, and it's whithin
        // the same pipe, HW can handle it that we don't need to set swsb
        if (isRAW && currDepPipe == prevDepPipe)
          checkAccFlagRAW(isRAW, currDep, *dep);

        if (isWAR || isWAW || isRAW || isWAW_out_of_order) {
          if (dep->getBitSet().empty()) {
            m_errorHandler.reportWarning(
                currInst.getPC(), "Dependency in bucket with no bits set");
          }
          // removing from bucket if there is nothing
          if (!dep->getBitSet().testAny(bucketID * 32,
                                        m_DB->getGRF_BYTES_PER_REG())) {
            bucket->clearDepSet(index);
          }

          if (prevDepClass == DEP_CLASS::IN_ORDER) {
            setDistanceDependency(dep, swsb, isWAW, prevDepPipe, currDepPipe,
                                  currInst);
          } else if (prevDepClass == DEP_CLASS::OUT_OF_ORDER) {
            setSbidDependency(*dep, currInst, needSyncForShootDownInst,
                              activeSBIDs);
          }
          // for the instruction in "OTHER" DEP_CLASS, such as sync, we don't
          // need to consider their dependency that is implied by hardware
        }
      }
    }
  }
}

void SWSBAnalyzer::setDistanceDependency(DepSet *dep, SWSB &swsb, bool isWAW,
    DEP_PIPE prevDepPipe, DEP_PIPE currDepPipe, const Instruction &currInst) {

  if (getNumOfDistPipe() == 1) {
    // FOR WAW if PREV is SHORT and curr is LONG then write will
    // finish before current write, no need to set swsb
    bool isWAWHazard =
        ((prevDepPipe == DEP_PIPE::SHORT && currDepPipe == DEP_PIPE::LONG) ||
         (prevDepPipe == DEP_PIPE::SHORT && currDepPipe == DEP_PIPE::SHORT)) &&
        isWAW;
    // require swsb for all the other kinds of dependency
    if (!isWAWHazard) {
      // setting minimum distance
      uint32_t newDistance =
          m_InstIdCounter.inOrder - dep->getInstIDs().inOrder;
      swsb.minDist =
          swsb.minDist == 0 ? newDistance : std::min(swsb.minDist, newDistance);
      // clamp the distance to max distance
      swsb.minDist = std::min(swsb.minDist, (uint32_t)MAX_VALID_DISTANCE);
      swsb.distType = SWSB::DistType::REG_DIST;
    }
  } else {
    // For multiple in-order pipeline architecuture, all cases should
    // be considered The distance is depended on the previous
    // instruction's pipeline
    uint32_t newDistance = 0;
    SWSB::DistType newDepPipe = SWSB::DistType::NO_DIST;
    switch (prevDepPipe) {
    case DEP_PIPE::FLOAT:
      newDistance = m_InstIdCounter.floatPipe - dep->getInstIDs().floatPipe;
      newDepPipe = SWSB::DistType::REG_DIST_FLOAT;
      break;
    case DEP_PIPE::INTEGER:
      newDistance = m_InstIdCounter.intPipe - dep->getInstIDs().intPipe;
      newDepPipe = SWSB::DistType::REG_DIST_INT;
      break;
    case DEP_PIPE::LONG64:
      newDistance = m_InstIdCounter.longPipe - dep->getInstIDs().longPipe;
      newDepPipe = SWSB::DistType::REG_DIST_LONG;
      break;
    case DEP_PIPE::MATH_INORDER:
      newDistance = m_InstIdCounter.mathPipe - dep->getInstIDs().mathPipe;
      newDepPipe = SWSB::DistType::REG_DIST_MATH;
      break;
    case DEP_PIPE::SCALAR:
      newDistance = m_InstIdCounter.scalarPipe - dep->getInstIDs().scalarPipe;
      newDepPipe = SWSB::DistType::REG_DIST_SCALAR;
      break;
    default:
      IGA_ASSERT(0, "Unsupported DEP_PIPE for in-order instructions");
      break;
    }

    if (swsb.minDist) {
      // the instruction already has dependency to others. Take the
      // min distance and set distType to ALL if they are not the same
      newDistance = std::min(swsb.minDist, newDistance);
      if (swsb.distType != newDepPipe)
        swsb.distType = SWSB::DistType::REG_DIST_ALL;
    } else {
      swsb.distType = newDepPipe;
    }
    assert(swsb.distType != SWSB::DistType::NO_DIST);
    // clamp the distance to max distance
    swsb.minDist = std::min(newDistance, (uint32_t)MAX_VALID_DISTANCE);
  } // end of if (m_enableMultiDistPipe)
  // clear this instruction's dependency since it is satisfied
  clearDepBuckets(*dep);

  // clear its companion because when an in-order instruction is
  // synced, both its input and output dependency are satisfied. The
  // only case is that if it has read/write_always_interfere
  // dependency, it should be reserved. The restriction is that: When
  // certain Arch Registers (sr, cr, ce) are used, the very next
  // instruction requires dependency to be set on all pipes {A@1} e.g.
  //      mov (1|M0)               r104.0<1>:ud  sr0.1<0;1,0>:ud
  //      cmp(16 | M0)   (ne)f0.0   null:ud    r104.0<0; 1, 0> : ub
  //      r62.4<0; 1, 0> : uw
  // A@1 is required for cmp instead of I@1
  if (dep->getCompanion() != nullptr) {
    // In the case that this DepSet is generated from math_wa_info, it
    // won't have companion
    if (dep->getCompanion()->getDepType() != DEP_TYPE::WRITE_ALWAYS_INTERFERE &&
        dep->getCompanion()->getDepType() != DEP_TYPE::READ_ALWAYS_INTERFERE) {
      clearDepBuckets(*dep->getCompanion());
    }
  }
}

void SWSBAnalyzer::setSbidDependency(DepSet &dep, const Instruction &currInst,
                                     bool &needSyncForShootDownInst,
                                     activeSBIDsTy &activeSBIDs) {
  /* For out of order we don't know how long it will finish
   * so need to test for SBID.
   * Instruction can depend on more then one SBID
   * send r10
   * send r20
   * send r30
   * ....
   * add r10 r20 r30
   * between different buckets and srcs/dst dependencies instruction can rely on
   * multiple SBID
   */
  SBID depSBID = dep.getSBID();
  if (depSBID.isFree) {
    m_errorHandler.reportError((int)dep.getInstGlobalID(),
                               "SBID SHOULDN'T BE FREE!");
  }
  // clears all the buckets
  clearDepBuckets(dep);

  // In case of shooting down of this instruction, we need to add sync to
  // preserve the swsb id sync, so that it's safe to clear the dep
  if (currInst.hasPredication() ||
      (currInst.getExecSize() != dep.getInstruction()->getExecSize()) ||
      (currInst.getChannelOffset() != dep.getInstruction()->getChannelOffset()))
    needSyncForShootDownInst = true;

  // used to set read or write dependency
  depSBID.dType = dep.getDepType();

  // activeSBIDs stores all sbid that this inst has dependency on
  // and it'll be processed in processactiveSBIDs
  bool insert_new_sbid = true;
  // making sure there are no duplicates
  for (auto &aSBID : activeSBIDs) {
    if (aSBID.first.sbid == depSBID.sbid) {
      // write takes longer then read
      // force the SBID to WRITE type (e.g. $10.dst) if the newly added
      // dependency is a write dependency.
      if (depSBID.dType == DEP_TYPE::WRITE ||
          depSBID.dType == DEP_TYPE::WRITE_ALWAYS_INTERFERE) {
        aSBID.first.dType = depSBID.dType;
      }
      // add DepSet into this activeSBIDs's associated list
      aSBID.second.push_back(&dep);
      // the sbid is already in the list, we don't need to add it to
      // activeSBIDs
      insert_new_sbid = false;
      break;
    }
  }

  // adding to active SBID
  // in Run function we will see how many this instruction relies on
  // and generate approriate SWSB and if needed test instruction
  // in that level also will add them back to free list
  if (insert_new_sbid) {
    activeSBIDs.push_back(std::make_pair(depSBID, std::vector<DepSet*>()));
    activeSBIDs.back().second.push_back(&dep);
  }
}

void SWSBAnalyzer::insertSyncAllRdWr(InstList::iterator insertPoint,
                                     Block *bb) {
  SWSB swsb;
  auto clearRD = m_kernel.createSyncAllRdInstruction(swsb);
  auto clearWR = m_kernel.createSyncAllWrInstruction(swsb);

  if (insertPoint == bb->getInstList().end()) {
    bb->getInstList().push_back(clearRD);
    bb->getInstList().push_back(clearWR);
  } else {
    bb->insertInstBefore(insertPoint, clearRD);
    bb->insertInstBefore(insertPoint, clearWR);
  }
}

// TODO this should also clear up grf dependency to handle this case:
/*
call (16|M0)             r8.0:ud          32
sendc.rc (16|M0)         null     r118  null  0x0         0x140B1000 {} //
wr:10h, rd:0, Render Target Write msc:16, to #0 (W)      mov (1|M0) a0.0<1>:ud
r7.0<0;1,0>:ud sendc.rc (16|M0)         null     r100  null  0x0 0x140B1000 {}
//   wr:10h, rd:0, Render Target Write msc:16, to #0 sendc.rc (16|M0) null r118
null  0x0         0x140B1000 {} //   wr:10h, rd:0, Render Target Write msc:16,
to #0 (W)      mov (16|M0)               r118.0<1>:ud  r6.0<8;8,1>:ud (W)
send.dc0 (16|M0)         r38       r118  null  0x0         a0.0 ret (16|M0)

Right now mov will have false dependense on the first send.
*/
bool SWSBAnalyzer::clearSBIDDependence(InstList::iterator insertPoint,
                                       Instruction *lastInst, Block *bb) {

  auto clearSBID = [&](const SBID& in) {
    m_freeSBIDList[in.sbid].reset();
    assert(m_IdToDepSetMap.find(in.sbid) != m_IdToDepSetMap.end());
    assert(m_IdToDepSetMap[in.sbid].first->getDepClass() ==
        DEP_CLASS::OUT_OF_ORDER);
    clearDepBuckets(*m_IdToDepSetMap[in.sbid].first);
    clearDepBuckets(*m_IdToDepSetMap[in.sbid].second);
  };

  bool sbidInUse = false;
  for (uint32_t i = 0; i < m_SBIDCount; ++i) {
    // there are still dependencies that might be used outside of this basic
    // block
    if (!m_freeSBIDList[i].isFree) {
      clearSBID(m_freeSBIDList[i]);
      sbidInUse = true;
    }
  }

  // if last instruction in basic block is EOT no need to generate flushes
  // hardware will take care of it
  if (lastInst && lastInst->getOpSpec().isAnySendFormat() &&
      lastInst->hasInstOpt(InstOpt::EOT)) {
    sbidInUse = false;
  }

  // platform check is mainly for testing purposes
  if (sbidInUse) {
    insertSyncAllRdWr(insertPoint, bb);
  }

  return sbidInUse;
}

// Keeping track of dependencies that need to be cleared because they are no
// longer relevant right now each BB ends with control flow instruction, and we
// reset at each BB
void SWSBAnalyzer::clearBuckets(DepSet *input, DepSet *output) {
  if (input->getDepClass() != DEP_CLASS::IN_ORDER)
    return;

  if (m_initPoint) {
    m_distanceTracker.emplace_back(input, output);
    m_initPoint = false;

  } else {
    // add DepSet to m_distanceTracker
    m_distanceTracker.emplace_back(input, output);

    auto get_depset_id = [&](DEP_PIPE pipe_type, DepSet &dep_set) {
      if (getNumOfDistPipe() == 1)
        return dep_set.getInstIDs().inOrder;
      switch (pipe_type) {
      case DEP_PIPE::FLOAT:
        return dep_set.getInstIDs().floatPipe;
      case DEP_PIPE::INTEGER:
        return dep_set.getInstIDs().intPipe;
      case DEP_PIPE::LONG64:
        return dep_set.getInstIDs().longPipe;
      case DEP_PIPE::MATH_INORDER:
        return dep_set.getInstIDs().mathPipe;
      case DEP_PIPE::SCALAR:
        return dep_set.getInstIDs().scalarPipe;
      default:
        IGA_ASSERT(0, "SWSB: unhandled in-order DEP_PIPE for XeHP+ encoding");
        break;
      }
      return (uint32_t)0;
    };

    auto get_latency = [&](DEP_PIPE pipe_type) {
      if (pipe_type == DEP_PIPE::LONG64)
        return m_LatencyLong64Pipe;
      else if (pipe_type == DEP_PIPE::MATH_INORDER)
        return m_LatencyInOrderMath;
      return m_LatencyInOrderPipe;
    };

    DEP_PIPE new_pipe = input->getDepPipe();
    // max B2B latency of thie pipe
    size_t max_dis = get_latency(new_pipe);
    // Remove nodes from the Tracker if the latency is already satified
    m_distanceTracker.remove_if([=](const distanceTrackerNode &node) {
      // bypass nodes those are not belong to the same pipe
      if (node.input->getDepPipe() != new_pipe)
        return false;

      // if the distance >= max_latency, clear buckets for corresponding
      // input and output Dependency
      size_t new_id = get_depset_id(new_pipe, *input);
      if ((new_id - get_depset_id(new_pipe, *node.input)) >= max_dis) {
        clearDepBuckets(*node.input);
        clearDepBuckets(*node.output);
        return true;
      }
      return false;
    });
  }
}


void SWSBAnalyzer::processactiveSBIDs(SWSB &swsb,
                                      const DepSet *input, Block *bb,
                                      InstList::iterator instIter,
                                      activeSBIDsTy &activeSBIDs) {
  // free SBID in activeSBIDs and clear the dependecies of associated
  // DepSet
  auto clearSBID = [&](const SBID& in) {
    m_freeSBIDList[in.sbid].reset();
    assert(m_IdToDepSetMap.find(in.sbid) != m_IdToDepSetMap.end());
    assert(m_IdToDepSetMap[in.sbid].first->getDepClass() ==
           DEP_CLASS::OUT_OF_ORDER);
    clearDepBuckets(*m_IdToDepSetMap[in.sbid].first);
    clearDepBuckets(*m_IdToDepSetMap[in.sbid].second);
  };


  // If instruction depends on one or more SBIDS, first one goes in to SWSB
  // field for rest we generate wait instructions.
  for (const auto &aSBID : activeSBIDs) {
    // Could be we had operation depending on the write
    /*
     *   This case also gets triggered when we have send in BB and dependence in
     * another BB L0: call (16|M0)             r8.0          L64 L16: sendc.rc
     * (16|M0)         null     r118  null  0x0         0x140B1000 {$0} //
     * wr:10h, rd:0, Render Target Write msc:16, to #0 L64: (W)      mov (16|M0)
     * r118.0<1>:ud  r6.0<8;8,1>:ud (W)      send.dc0 (16|M0)         r38 r118
     * null  0x0         a0.0       {@1, $0} ret (16|M0) r8.0 {@3} After first
     * BB in which sendc.rc ends we clear all SBID and generate sync
     * instructions On mov it detects dependense, but all SBID are freed.
     */
    if (m_freeSBIDList[aSBID.first.sbid].isFree) {
      continue;
    }

    SWSB::TokenType tType = SWSB::TokenType::NOTOKEN;
    if (aSBID.first.dType == DEP_TYPE::READ ||
        aSBID.first.dType == DEP_TYPE::READ_ALWAYS_INTERFERE) {
      tType = SWSB::TokenType::SRC;
    } else {
      tType = SWSB::TokenType::DST;
      // if SBID is cleared add it back to free pool
      // write is last thing. So if instruction depends on it we know read is
      // done but not vice versa
      clearSBID(aSBID.first);
    }

    // Setting first SBID as part of instruction
    // If this instruction depends on more SBID, generate sync for the extra ids
    // TODO: Is it safe to clear SBID here?
    if (swsb.tokenType == SWSB::TokenType::NOTOKEN) {
      swsb.tokenType = tType;
      swsb.sbid = aSBID.first.sbid;
    } else {
      // add sync for the id
      SWSB sync_swsb(SWSB::DistType::NO_DIST, tType, 0, aSBID.first.sbid);
      auto nopInst = m_kernel.createSyncNopInstruction(sync_swsb);
      bb->insertInstBefore(instIter, nopInst);
    }
  }

  // verify if the combination of token and dist is valid, if not, move the
  // token dependency out and add a sync for it
  auto syncInsts = adjustSWSB(*bb, instIter, swsb, true);

  // update the DepSet's SrcDepInsts if current instruction or any newly added
  // sync is associated with it
  auto checkAndUpdateSrcDepInsts =
      [&activeSBIDs](Instruction* inst, SWSB swsb) {
    if (!swsb.hasToken())
      return;
    if (swsb.tokenType != SWSB::TokenType::SET &&
        swsb.tokenType != SWSB::TokenType::SRC)
      return;
    for (const auto &aSBID : activeSBIDs) {
      if (swsb.sbid == aSBID.first.sbid) {
        for (auto& ds : aSBID.second) {
          ds->addSrcDepInsts(inst);
          ds->getCompanion()->addSrcDepInsts(inst);
        }
      }
    }
  };

  for (auto sync : syncInsts) {
    checkAndUpdateSrcDepInsts(sync, sync->getSWSB());
  }
  checkAndUpdateSrcDepInsts(*instIter, swsb);
}

uint32_t SWSBAnalyzer::getNumOfDistPipe() {
  return getNumOfDistPipe(m_swsbMode);
}

uint32_t SWSBAnalyzer::getNumOfDistPipe(SWSB_ENCODE_MODE mode) {
  switch (mode) {
  case SWSB_ENCODE_MODE::SingleDistPipe:
    return 1;
  case SWSB_ENCODE_MODE::ThreeDistPipe:
    return 3;
  case SWSB_ENCODE_MODE::ThreeDistPipeDPMath:
    return 3;
  case SWSB_ENCODE_MODE::FourDistPipe:
  case SWSB_ENCODE_MODE::FourDistPipeReduction:
    return 4;
  case SWSB_ENCODE_MODE::FiveDistPipe:
  case SWSB_ENCODE_MODE::FiveDistPipeReduction:
    return 5;
  default:
    break;
  }
  return 0;
}

void SWSBAnalyzer::advanceInorderInstCounter(DEP_PIPE dep_pipe) {
  ++m_InstIdCounter.inOrder;
  if (getNumOfDistPipe() == 1)
    return;

  switch (dep_pipe) {
  case DEP_PIPE::FLOAT:
    ++m_InstIdCounter.floatPipe;
    break;
  case DEP_PIPE::INTEGER:
    ++m_InstIdCounter.intPipe;
    break;
  case DEP_PIPE::LONG64:
    ++m_InstIdCounter.longPipe;
    break;
  case DEP_PIPE::MATH_INORDER:
    ++m_InstIdCounter.mathPipe;
    break;
  case DEP_PIPE::SCALAR:
    ++m_InstIdCounter.scalarPipe;
    break;
  default:
    IGA_ASSERT(0, "unhandled in-order DEP_PIPE for XE_HP encoding");
    break;
  }
}

void SWSBAnalyzer::addRMWDependencyIfReqruied(DepSet &input, DepSet &output) {
  const Instruction *inst = input.getInstruction();
  // return if the instruction has no dst, or the dst is not GRF or not byte
  // type
  const Operand &dst = inst->getDestination();
  if (dst.getKind() != Operand::Kind::DIRECT)
    return;

  if (dst.getDirRegName() != RegName::GRF_R)
    return;

  if (TypeSizeInBitsWithDefault(dst.getType(), 32) != 8)
    return;

  // When there is RMW behavior, the instruction will read the Word first,
  // modify the byte value in it and then write back the entire Word.
  // we assume the instruction will read/write the entire register to simplify
  // the logic

  // add the entire grf of the dst register into input and output DepSet
  // All registers being touched are added into Bucket. We can get the touched
  // grf number from added bucket index
  const std::vector<size_t> &out_buk = output.getBuckets();
  for (auto i : out_buk) {
    // we only need grf bucket
    if (i >= m_DB->getBucketStart(RegName::ARF_A))
      continue;
    input.addGrf(i);
    input.addToBucket((uint32_t)i);
    output.addGrf(i);
  }
}

void SWSBAnalyzer::addSWSBToInst(InstListIterator instIt, const SWSB &swsb,
                                 Block &block) {
  assert(instIt != block.getInstList().end());
  Instruction &inst = **instIt;
  SWSB new_swsb(inst.getSWSB());
  // handling distance
  if (swsb.hasDist()) {
    if (!inst.getSWSB().hasDist()) {
      new_swsb.distType = swsb.distType;
      new_swsb.minDist = swsb.minDist;
    } else {
      // for single dist pipe platform, distType must be REG_DIST, so won't
      // be set to REG_DIST_ALL
      new_swsb.distType = (inst.getSWSB().distType == swsb.distType)
                              ? swsb.distType
                              : SWSB::DistType::REG_DIST_ALL;
      new_swsb.minDist = std::min(inst.getSWSB().minDist, swsb.minDist);
    }
  }

  // handling token
  if (swsb.hasToken()) {
    if (!inst.getSWSB().hasToken()) {
      new_swsb.tokenType = swsb.tokenType;
      new_swsb.sbid = swsb.sbid;
    } else {
      // if both has id, and are different, then insert a sync to carry
      // the new one, otherwise do nothing
      if ((inst.getSWSB().tokenType != swsb.tokenType) ||
          (inst.getSWSB().sbid != swsb.sbid)) {
        SWSB tmp_swsb(SWSB::DistType::NO_DIST, swsb.tokenType, 0, swsb.sbid);
        Instruction *sync_inst = m_kernel.createSyncNopInstruction(tmp_swsb);
        block.insertInstBefore(instIt, sync_inst);
      }
    }
  }

  // check if the new swsb combination is valid, if not, move the dist out to a
  // sync
  // FIXME: move the dist out here to let the sbid set on the instruction could
  // have better readability, but a potential issue is that A@1 is required to
  // be set on the instruction having architecture read/write. This case A@1
  // will be moved out from the instruction
  adjustSWSB(block, instIt, new_swsb, false);
  inst.setSWSB(new_swsb);
}

std::vector<Instruction*>
SWSBAnalyzer::adjustSWSB(Block &block, const InstListIterator instIt,
                         SWSB &swsb, bool preferMoveOutSBID) {
  std::vector<Instruction*> syncInsts;
  const Instruction* inst = *instIt;
  if (swsb.verify(m_swsbMode, inst->getSWSBInstType(m_swsbMode)))
    return syncInsts;

  auto movDistToSync = [&]() {
      assert(swsb.hasDist());
      SWSB tmp_swsb(swsb.distType, SWSB::TokenType::NOTOKEN, swsb.minDist, 0);
      Instruction *sync_inst = m_kernel.createSyncNopInstruction(tmp_swsb);
      syncInsts.push_back(sync_inst);
      block.insertInstBefore(instIt, sync_inst);
      swsb.distType = SWSB::DistType::NO_DIST;
      swsb.minDist = 0;
  };
  auto movTokenToSync = [&]() {
      assert(swsb.hasToken());
      SWSB tmp_swsb(SWSB::DistType::NO_DIST, swsb.tokenType , 0, swsb.sbid);
      Instruction *sync_inst = m_kernel.createSyncNopInstruction(tmp_swsb);
      syncInsts.push_back(sync_inst);
      block.insertInstBefore(instIt, sync_inst);
      swsb.tokenType = SWSB::TokenType::NOTOKEN;
      swsb.sbid = 0;
  };

  // Send can only have SBID.set and its combination on the instruction
  if (inst->getOpSpec().isAnySendFormat()) {
    if (swsb.tokenType == SWSB::TokenType::SET) {
      // swsb has SBID.set but the combination with distance is invalid.
      // Move distance to sync.
      movDistToSync();
      assert(swsb.verify(m_swsbMode, inst->getSWSBInstType(m_swsbMode)));
    } else {
      // swsb has no SBID.set at the moment. Send must have SBID.set
      // later (SWSBAnalyzer::assignSBID) so allow distance swsb and once
      // SBID.set is set, the distance may be able to combined with it.
      // Move only Token out of the instruction.
      if (swsb.hasToken())
        movTokenToSync();
    }
    return syncInsts;
  }

  // For other instructions, solely SBID or DIST are valid SWSB.
  // Move one to sync according to the preference.
  if (preferMoveOutSBID)
    movTokenToSync();
  else
    movDistToSync();
  assert(swsb.verify(m_swsbMode, inst->getSWSBInstType(m_swsbMode)));
  return syncInsts;
}

static bool isSyncNop(const Instruction &i) {
  return i.is(Op::SYNC) && i.getSyncFc() == SyncFC::NOP;
};

void SWSBAnalyzer::postProcessReadModifiedWriteOnByteDst() {
  // move all swsb set within the Atomic block out for the "instruction write
  // combined" cases Atomic are provided in the input so assume they are correct
  // and have no internal dependency within the macro. Move all swsb to the
  // first instruction in the Atomic block and also add the distance swsb to the
  // intruction following the Atomic block in case of it has dependency to the
  // block but was resolved within the instruction in the block E.g.
  //      (W) mov (32|M0)  r13.0<2>:ub   r50.0<1;1,0>:uw   {Atomic, I@1}
  //      (W) mov (32|M0)  r13.1<2>:ub   r52.0<1;1,0>:uw   {Atomic}
  //      (W) mov (32|M0)  r13.2<2>:ub   r54.0<1;1,0>:uw   {Atomic}
  //      (W) mov (32|M0)  r13.3<2>:ub   r56.0<1;1,0>:uw
  //          add (1)      r13.0<1>:df   r100.0<0;1,0>:df  {I@1}
  for (Block *bb : m_kernel.getBlockList()) {
      InstList &instList = bb->getInstList();
    for (auto inst_it = instList.begin(); inst_it != instList.end();
         ++inst_it) {
      auto isWriteCombinedCandidate = [&](Instruction &inst) {
        return (inst.is(Op::MOV) || inst.is(Op::SRND)) &&
               inst.getDestination().getKind() == Operand::Kind::DIRECT &&
               inst.getDestination().getDirRegName() == RegName::GRF_R &&
               TypeSizeInBitsWithDefault(inst.getDestination().getType(),
                                         32) == 8;
      };
      // add distance swsb in "from" into "to"
      auto updateDistanceSWSB = [](const SWSB &from, SWSB &to) {
        if (!from.hasDist())
          return;

        if (!to.hasDist()) {
          to.distType = from.distType;
          to.minDist = from.minDist;
        } else {
          to.distType = (to.distType == from.distType)
                            ? to.distType
                            : SWSB::DistType::REG_DIST_ALL;
          to.minDist = std::min(to.minDist, from.minDist);
        }
      };

      if ((*inst_it)->hasInstOpt(InstOpt::ATOMIC) &&
          isWriteCombinedCandidate(**inst_it)) {
        // found the marcro start
        InstListIterator firstit = inst_it;
        SWSB allDistSWSB = SWSB();
        ++inst_it;
        InstList sync_insts;
        // iterate to the end of the macro and move all swsb to the first
        for (; inst_it != instList.end(); ++inst_it) {
          Instruction &cur_inst = **inst_it;
          // found sync, prepare to move to before the firstinst
          if (cur_inst.is(Op::SYNC)) {
            sync_insts.push_back(
                m_kernel.createSyncNopInstruction(cur_inst.getSWSB()));
            // remove swsb in current sync so that this sync will be removed
            // in the following pass
            cur_inst.setSWSB(SWSB());
            continue;
          }
          // All instructions within the write-combined atomic block must be
          // write-combined candidate
          if (isWriteCombinedCandidate(cur_inst)) {
            // move the swsb within the atomic block to the firstinst and keep
            // track of distance swsb
            if (cur_inst.getSWSB().hasSWSB()) {
              updateDistanceSWSB(cur_inst.getSWSB(), allDistSWSB);
              addSWSBToInst(firstit, cur_inst.getSWSB(), *bb);
              cur_inst.setSWSB(SWSB());
            }
            // found the last instruction of the Atomic block
            if (!cur_inst.hasInstOpt(InstOpt::ATOMIC))
              break;
          } else {
            m_errorHandler.reportError(
                cur_inst.getPC(),
                "Instruction found in the write-combined atomic block is not "
                "a write-combined candidate");
            break;
          }
        }
        // insert sync to before firstinst
        if (!sync_insts.empty())
          instList.insert(firstit, sync_insts.begin(), sync_insts.end());

        if (inst_it == instList.end()) {
          m_errorHandler.reportError(
              (*firstit)->getPC(),
              "The last instruction in this write-combined atomic block has "
              "{Atomic} set");
          break;
        }

        // insert distance dependency to the instruction following the block
        InstListIterator next = inst_it;
        next++;
        // if the last instruction in the atomic block is the last instruction
        // in the BB, insert a sync.nop to carry the required SWSB info
        // For example:
        //      BB0:
        //      (W) mov (32|M0)  r13.0<2>:ub   r50.0<1;1,0>:uw   {Atomic}
        //      (W) mov (32|M0)  r13.1<2>:ub   r52.0<1;1,0>:uw   {Atomic}
        //      (W) mov (32|M0)  r13.2<2>:ub   r54.0<1;1,0>:uw   {Atomic}
        //      (W) mov (32|M0)  r13.3<2>:ub   r56.0<1;1,0>:uw
        //      (W) sync.nop  {I@1} // insert nop
        //      BB1:
        //          add (1)      r13.0<1>:df   r100.0<0;1,0>:df
        if (next == instList.end()) {
          instList.push_back(m_kernel.createSyncNopInstruction(allDistSWSB));
          break;
        }
        addSWSBToInst(next, allDistSWSB, *bb);
      }
    }
  }
}


void SWSBAnalyzer::postProcessRemoveRedundantSync() {
  // Case 1:
  // sync.nop carry the sbid the same as the sbid set on the following
  // instruction can be removed since it'll automatically be sync-ed when sbid
  // is reused. For example:
  //   sync.nop null {$0.dst} // can be removed
  //   math.exp(8|M0)  r12.0<1>:f  r10.0<8;8,1>:f {$0}
  // Case 2:
  // Continuous sync.nop with the same swsb. One of it can be removed:
  //   sync.nop null {$0.src}
  //   sync.nop null {$0.src}
  for (Block *bb : m_kernel.getBlockList()) {
    InstList &instList = bb->getInstList();
    if (instList.empty())
      continue;
    auto inst_it = instList.begin();
    // skip the first instruction, which must not be sync
    ++inst_it;
    for (; inst_it != instList.end(); ++inst_it) {
      Instruction *inst = *inst_it;
      if (isSyncNop(*inst))
        continue;

      SWSB cur_swsb = inst->getSWSB();
      // iterate through the previous sync if any
      auto sync_it = inst_it;
      --sync_it;
      // keep track of the seen SWSB's TokenType and sbid number
      std::set<std::pair<SWSB::TokenType, uint32_t>> seenSBID;
      while (sync_it != instList.begin()) {
        Instruction *sync_inst = *sync_it;
        if (!isSyncNop(*sync_inst))
          break;
        SWSB sync_swsb = sync_inst->getSWSB();
        // Case 1
        // if the sync has sbid set, it could be the reserved sbid for shoot
        // down instructions, we should keep it. Otherwise if the its sbid
        // is the same as current's instrcutions' sbid.set, we can remove it.
        if(cur_swsb.hasToken() && cur_swsb.tokenType == SWSB::TokenType::SET) {
          if (sync_swsb.hasToken() &&
              sync_swsb.tokenType != SWSB::TokenType::SET &&
              sync_swsb.sbid == cur_swsb.sbid) {
            // clean the swsb so that we can remove this instruction later
            sync_inst->setSWSB(SWSB());
          }
        }
        // Case 2
        // keep track of visited SWSB on sync and remove the duplicated one
        sync_swsb = sync_inst->getSWSB();
        if (sync_swsb.hasToken()) {
          auto sbidPair = std::make_pair(sync_swsb.tokenType, sync_swsb.sbid);
          if (seenSBID.find(sbidPair) != seenSBID.end()) {
            sync_inst->setSWSB(SWSB());
          } else {
            seenSBID.insert(sbidPair);
          }
        }
        --sync_it;
      }
    }
    // remove the redundant sync.nop (sync.nop with no swsb)
    instList.remove_if([](const Instruction *inst) {
      return isSyncNop(*inst) && !inst->getSWSB().hasSWSB();
    });
  }
}

void SWSBAnalyzer::postProcess() {
  // revisit all instructions to handle write-combined Atomic block
  if (m_kernel.getModel().hasReadModifiedWriteOnByteDst()) {
    postProcessReadModifiedWriteOnByteDst();
  }


  // revisit all instructions to remove redundant sync.nop
  postProcessRemoveRedundantSync();
}

SBID &SWSBAnalyzer::assignSBID(DepSet *input, DepSet *output, Instruction &inst,
                               SWSB &swsb, InstList::iterator insertPoint,
                               Block *curBB, bool needSyncForShootDown) {
  bool foundFree = false;
  SBID *sbidFree = nullptr;
  for (uint32_t i = 0; i < m_SBIDCount; ++i) {
    if (m_freeSBIDList[i].isFree) {
      foundFree = true;
      sbidFree = &m_freeSBIDList[i];
      m_freeSBIDList[i].sbid = i;
      break;
    }
  }
  // no free SBID.
  if (!foundFree) {
    unsigned int index = (m_SBIDRRCounter++) % m_SBIDCount;

    // While swsb id being reuse, the dependency will automatically resolved by
    // hardware, so cleanup the dependency bucket for instruction that
    // previously used this id
    assert(m_IdToDepSetMap.find(index) != m_IdToDepSetMap.end());
    assert(m_IdToDepSetMap[index].first->getDepClass() ==
           DEP_CLASS::OUT_OF_ORDER);
    clearDepBuckets(*m_IdToDepSetMap[index].first);
    clearDepBuckets(*m_IdToDepSetMap[index].second);

    m_freeSBIDList[index].reset();
    sbidFree = &m_freeSBIDList[index];
    sbidFree->sbid = index;
  }
  sbidFree->isFree = false;
  input->setSBID(*sbidFree);
  output->setSBID(*sbidFree);
  if (m_IdToDepSetMap.find(sbidFree->sbid) != m_IdToDepSetMap.end())
    m_IdToDepSetMap.erase(sbidFree->sbid);
  m_IdToDepSetMap.emplace(sbidFree->sbid, std::make_pair(input, output));

  if (needSyncForShootDown) {
    // check if the instruction has A@1 on it. If it does, create another sync
    // to move A@1 to before the sync.nop in case of violating the rule "When
    // certain Arch Registers (sr, cr, ce) are used, the very next instruction
    // requires dependency to be set on all pipes {A@1}".
    // At this point we do not know if the A@1 is inserted for this rule or
    // other dependency. Conservatively move A@1 above to the very first sync
    // after the previous non-sync instruction
    if (swsb.hasDist() &&
        (swsb.distType == SWSB::DistType::REG_DIST_ALL ||
         swsb.distType == SWSB::DistType::REG_DIST) &&
        swsb.minDist == 1) {
      SWSB ss(swsb.distType, SWSB::TokenType::NOTOKEN, 1, 0);
      curBB->insertInstBefore(insertPoint,
                              m_kernel.createSyncNopInstruction(ss));
      swsb.distType = SWSB::DistType::NO_DIST;
      swsb.minDist = 0;
    }
    // add a sync to preserve the token for possibly shooting down instruction
    SWSB tDep(SWSB::DistType::NO_DIST, SWSB::TokenType::SET, 0, sbidFree->sbid);
    auto syncInst = m_kernel.createSyncNopInstruction(tDep);
    curBB->insertInstBefore(insertPoint, syncInst);
    input->addSrcDepInsts(syncInst);
    output->addSrcDepInsts(syncInst);
  }

  // adding the set for this SBID
  // if the swsb has the token set already, move it out to a sync
  if (swsb.tokenType != SWSB::TokenType::NOTOKEN) {
    SWSB tDep(SWSB::DistType::NO_DIST, swsb.tokenType, 0, swsb.sbid);
    Instruction *tInst = m_kernel.createSyncNopInstruction(tDep);
    curBB->insertInstBefore(insertPoint, tInst);
  }
  // set the sbid
  swsb.tokenType = SWSB::TokenType::SET;
  swsb.sbid = sbidFree->sbid;

  // verify if the token and dist combination is valid, if not, move the dist
  // out to a sync move the dist out here to let the sbid set on the instruction
  // so that other instructions having dependency to this SBID is sync to the
  // correct instruction (but not a sync.nop)
  // FIXME: a potential issue is that A@1 is required to be set on the
  // instruction having architecture read/write. This case A@1 will be moved out
  // from the instruction
  adjustSWSB(*curBB, insertPoint, swsb, false);

  assert(sbidFree != nullptr);
  return *sbidFree;
}

void SWSBAnalyzer::run() {
  m_initPoint = true;
  m_distanceTracker.clear();

  for (uint32_t i = 0; i < MAX_GRF_BUCKETS; ++i) {
    m_buckets[i].clearDependency();
  }

  // init in order pipe id counters
  m_InstIdCounter.init();
  math_wa_info.reset();

  Instruction *inst = nullptr;
  Block *lastBB = nullptr;
  for (auto bb : m_kernel.getBlockList()) {
    bool blockEndsWithNonBranchInst = false;
    // resetting things for each bb
    lastBB = bb;
    InstList &instList = bb->getInstList(); // Don't use auto for over loaded
                                            // return which has const...
    const auto instListEnd = instList.end();
    for (auto instIter = instList.begin(); instIter != instListEnd;
         ++instIter) {
      m_InstIdCounter.global++;
      inst = *instIter;
      // skip illeagl instruction. It cannot have dependency to others, nor does
      // it affect the in-pipe instruction count. Skipping it can avoid we
      // accidentally set SWSB on it (e.g. when forcing B2B dependency). Illeagl
      // instructions cannot carry SWSB info.
      if (inst->getOp() == Op::ILLEGAL)
        continue;
      DepSet *input = nullptr;
      DepSet *output = nullptr;
      size_t dpas_cnt_in_macro = 0;

      if (math_wa_info.math_inst != nullptr)
        math_wa_info.previous_is_math = true;
      if (inst->getOpSpec().is(Op::MATH)) {
        math_wa_info.math_inst = inst;

        // if the math following a math, we only care about the last math
        math_wa_info.previous_is_math = false;
      }

      // recored the first instruction of a dpas macro, in case that inserting
      // instructions (e.g. sync) before the macro, those instructions have to
      // be insert before first_inst_in_dpas_macro
      InstListIterator first_inst_in_dpas_macro = instList.end();
      if (inst->getOpSpec().isDpasFormat()) {
        std::pair<DepSet *, DepSet *> dep_set_pair =
            m_DB->createDPASSrcDstDepSet(instList, instIter, m_InstIdCounter,
                                         dpas_cnt_in_macro, m_swsbMode);
        input = dep_set_pair.first;
        output = dep_set_pair.second;

        first_inst_in_dpas_macro = instIter;
        // bypass dpas insturctions in the macro, the last dpas represents the
        // macro
        for (size_t i = 0; i < dpas_cnt_in_macro - 1; ++i)
          ++instIter;
        inst = *instIter;
      } else {
        input = m_DB->createSrcDepSet(*inst, m_InstIdCounter, m_swsbMode);
        if (m_DB->needDstReadSuppressionWA(*inst))
          output = m_DB->createDstDepSetFullGrf(*inst, m_InstIdCounter,
                                                m_swsbMode, true);
        else
          output = m_DB->createDstDepSet(*inst, m_InstIdCounter, m_swsbMode);
      }
      input->setCompanion(output);
      output->setCompanion(input);

      // XeHPC+ features
      if (m_kernel.getModel().hasReadModifiedWriteOnByteDst())
        addRMWDependencyIfReqruied(*input, *output);

      SWSB swsb;

      // Either source or destination are indirect, or there are SR access,
      // We don't know what registers are being accessed
      // Need to flush all the sbids and set distance to 1
      if (input->hasIndirect() || output->hasIndirect() || input->hasSR() ||
          output->hasSR()) {
        // clear out-of-order dependency, insert sync.allrd and sync.allwr
        // if there are un-resolved sbid dependecny
        // if this instruction itself is an out-of-order instruction, insert
        // sync.all anyway.
        InstListIterator insert_point = instIter;
        if (first_inst_in_dpas_macro != instList.end())
          insert_point = first_inst_in_dpas_macro;
        bool forceSyncAll = clearSBIDDependence(insert_point, inst, bb);
        if (!forceSyncAll && input->getDepClass() == DEP_CLASS::OUT_OF_ORDER)
          insertSyncAllRdWr(insert_point, bb);

        // clear in-order dependency
        clearBuckets(input, output);

        // will add direct accesses to buckets
        // adding dependencies to buckets
        for (auto bucketID : input->getBuckets()) {
          m_buckets[bucketID].addDepSet(input);
        }
        for (auto bucketID : output->getBuckets()) {
          m_buckets[bucketID].addDepSet(output);
        }

        // set to check all dist pipes
        if (getNumOfDistPipe() == 1)
          swsb.distType = SWSB::DistType::REG_DIST;
        else
          swsb.distType = SWSB::DistType::REG_DIST_ALL;
        swsb.minDist = 1;

        // input and output must have the same dep class and in the same pipe
        // so check the input only to add the instCounter
        // FIXME: is it possilbe that a instruction has output and no input?
        if (input->getDepClass() == DEP_CLASS::IN_ORDER)
          advanceInorderInstCounter(input->getDepPipe());

        // if this is an out-of-order instruction, we still need to assign an
        // sbid for it
        if (output->getDepClass() == DEP_CLASS::OUT_OF_ORDER)
          assignSBID(input, output, *inst, swsb, insert_point, bb,
                     false);

        inst->setSWSB(swsb);
        // clean up math_wa_info, this instruction force to sync all, no need to
        // consider math wa
        if (math_wa_info.previous_is_math) {
          math_wa_info.reset();
        }
        // early out, no need to calculateDependence that all dependencies are
        // resolved.
        continue;
      } // end indirect access handling

      if (math_wa_info.previous_is_math) {
        // math WA affect the instruction right after the math, and with
        // different predication Add the WA math dst region to Buckets
        if (math_wa_info.math_inst->getPredication().function !=
            inst->getPredication().function) {
          math_wa_info.dep_set = m_DB->createDstDepSetFullGrf(
              *math_wa_info.math_inst, math_wa_info.math_id, m_swsbMode, false);
          math_wa_info.dep_set->setSBID(math_wa_info.math_sbid);
          for (auto bucketID : math_wa_info.dep_set->getBuckets()) {
            IGA_ASSERT(bucketID < m_DB->getTOTAL_BUCKETS(),
                       "buckedID out of range");
            m_buckets[bucketID].addDepSet(math_wa_info.dep_set);
          }
        }
      }

      // keep track of the depended SBID current inst depends on,
      // and the list of DepSet where this SBID comes from
      activeSBIDsTy activeSBIDs;
      bool needSyncForShootDown = false;
      // Calculates dependence between this instruction dependencies and
      // previous ones.
      calculateDependence(*input, swsb, *inst, activeSBIDs,
                          needSyncForShootDown);
      calculateDependence(*output, swsb, *inst, activeSBIDs,
                          needSyncForShootDown);

      // clean up math_wa_info
      if (math_wa_info.previous_is_math) {
        if (math_wa_info.dep_set != nullptr)
          clearDepBuckets(*math_wa_info.dep_set);
        math_wa_info.reset();
      }

      if (first_inst_in_dpas_macro != instList.end())
        processactiveSBIDs(swsb, input, bb,
                          first_inst_in_dpas_macro, activeSBIDs);
      else
        processactiveSBIDs(swsb, input, bb, instIter, activeSBIDs);

      // Need to set SBID
      if (output->getDepClass() == DEP_CLASS::OUT_OF_ORDER) {
        InstList::iterator insertPoint = instIter;
        if (first_inst_in_dpas_macro != instList.end())
          insertPoint = first_inst_in_dpas_macro;
        SBID &assigned_id = assignSBID(input, output, *inst, swsb,
                                       insertPoint, bb, needSyncForShootDown);

        // record the sbid if it's math, for use of math wa
        if (inst->getOpSpec().is(Op::MATH)) {
          math_wa_info.math_sbid = assigned_id;
        }
      }

      clearBuckets(input, output);

      /*
       * Handling the case where everything is in one bb, and send with EOT is
       * in the middle of instruction stream call (16|M0)             r8.0:ud 32
       *           sendc.rc (16|M0)         null     r118  null  0x0 0x140B1000
       * {EOT} //   wr:10h, rd:0, Render Target Write msc:16, to #0
       *           ...
       *           ret (16|M0)                          r8.0
       */
      if (!(inst->getOpSpec().isAnySendFormat() &&
            inst->hasInstOpt(InstOpt::EOT))) {
        // adding dependencies to buckets
        for (auto bucketID : input->getBuckets()) {
          // We want to check dependncy of regular instructions against
          // WRITE_ALWAYS_INTERFERE without adding them themselves
          if (bucketID == m_DB->getBucketStart(RegName::ARF_CR) &&
              input->getDepType() != DEP_TYPE::WRITE_ALWAYS_INTERFERE &&
              input->getDepType() != DEP_TYPE::READ_ALWAYS_INTERFERE) {
            continue;
          }
          m_buckets[bucketID].addDepSet(input);
        }
        for (auto bucketID : output->getBuckets()) {
          IGA_ASSERT(bucketID < m_DB->getTOTAL_BUCKETS(),
                     "buckedID out of range");
          // We want to check dependncy of regular instructions against
          // WRITE_ALWAYS_INTERFERE without adding them themselves
          if (bucketID == m_DB->getBucketStart(RegName::ARF_CR) &&
              output->getDepType() != DEP_TYPE::WRITE_ALWAYS_INTERFERE &&
              output->getDepType() != DEP_TYPE::READ_ALWAYS_INTERFERE) {
            continue;
          }
          m_buckets[bucketID].addDepSet(output);
        }
      }

      if (input->getDepClass() == DEP_CLASS::IN_ORDER) {
        advanceInorderInstCounter(input->getDepPipe());
      }

      // for dpas block, set the distance at the first inst in the block, and
      // set the swsb id at the last inst in the block.
      if ((first_inst_in_dpas_macro != instList.end()) &&
          (*first_inst_in_dpas_macro != inst)) {
        (*first_inst_in_dpas_macro)->setSWSB(SWSB(swsb.distType,
                           SWSB::TokenType::NOTOKEN, swsb.minDist, 0));
        inst->setSWSB(SWSB(SWSB::DistType::NO_DIST, swsb.tokenType, 0,
                           swsb.sbid));
      } else {
        // if the input SWSB is a special token, preserve it and insert a sync
        // before to carry the dependency info Note that dpas must not have
        // special token so we only do this check for non-dpas here
        if (inst->getSWSB().hasSpecialToken()) {
          if (swsb.hasSWSB()) {
            Instruction *syncInst =
                m_kernel.createSyncNopInstruction(swsb);
            bb->insertInstBefore(instIter, syncInst);
          }
        } else {
          inst->setSWSB(swsb);
        }
      }
      assert(swsb.verify(m_swsbMode, inst->getSWSBInstType(m_swsbMode)));

      if (inst->isBranching()) {
        // TODO: konrad : this is somewhat conservative, some
        // branch instructions might not need sync (join)
        blockEndsWithNonBranchInst = false;
        clearSBIDDependence(instIter, inst, bb);
        continue;
      } else {
        blockEndsWithNonBranchInst = true;
      }
    } // iterate on instr
    //          clear read
    //          clear write
    if (blockEndsWithNonBranchInst) {
      clearSBIDDependence(instList.end(), inst, bb);
    }
  } // iterate on basic block

  if (inst)
    forceSyncLastInst(*inst, *lastBB);

  postProcess();
  return;
}

void SWSBAnalyzer::forceSyncLastInst(const Instruction &lastInst,
                                     Block &lastBB) {
  // this code is for FC composite
  // if last instruction is not EOT we will insert flush instructions
  // and stall the pipeline since we do not do global analysis
  if ((lastInst.getOpSpec().isAnySendFormat() &&
       !lastInst.getInstOpts().contains(InstOpt::EOT)) ||
      !lastInst.getOpSpec().isAnySendFormat()) {
    SWSB swsb;
    if (getNumOfDistPipe() == 1)
      swsb.distType = SWSB::DistType::REG_DIST;
    else
      swsb.distType = SWSB::DistType::REG_DIST_ALL;
    swsb.minDist = 1;
    Instruction *syncInst = m_kernel.createSyncNopInstruction(swsb);
    lastBB.getInstList().push_back(syncInst);
  }
}
