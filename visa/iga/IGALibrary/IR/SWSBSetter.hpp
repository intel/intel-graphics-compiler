/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_SWSBSETTER_H_
#define _IGA_SWSBSETTER_H_

#include "../ErrorHandler.hpp"
#include "Block.hpp"
#include "Instruction.hpp"
#include "Kernel.hpp"
#include "Operand.hpp"
#include "RegDeps.hpp"
#include <limits>

namespace iga {
// Bucket represents a GRF and maps to all instructions that access it
class Bucket {
public:
  Bucket() { dependencies.reserve(5); }
  void clearDependency() { dependencies.clear(); }
  bool isEmpty() const { return dependencies.empty(); }
  size_t getNumDependencies() const { return dependencies.size(); }
  DepSet *getDepSet(uint32_t index) { return dependencies[index]; }
  void clearDepSet(uint32_t index) { dependencies[index] = nullptr; }
  // Most of the time dependecy vector will have 1 or two entries
  void addDepSet(DepSet *dep) {
    bool depSet = false;
    for (size_t i = 0; i < dependencies.size(); ++i) {
      if (!dependencies[i]) {
        dependencies[i] = dep;
        depSet = true;
        break;
      }
    }
    if (!depSet) {
      dependencies.push_back(dep);
    }
  }

private:
  std::vector<DepSet *> dependencies;
};

class SWSBAnalyzer {
public:
  typedef DepSet::InstIDs InstIDs;

public:
  // Blocks have already been created
  SWSBAnalyzer(Kernel &k, ErrorHandler &errHandler,
               SWSB_ENCODE_MODE encode_mode, int sbid_count
               )
      : m_kernel(k), m_errorHandler(errHandler), m_SBIDRRCounter(0),
        m_initPoint(false),
        MAX_VALID_DISTANCE(k.getModel().getSWSBMaxValidDistance()) {
    // Set SWSB_ENCODE_MODE
    if (encode_mode != SWSB_ENCODE_MODE::SWSBInvalidMode)
      m_swsbMode = encode_mode;
    else
      m_swsbMode = k.getModel().getSWSBEncodeMode();

    if (m_swsbMode == SWSB_ENCODE_MODE::FourDistPipeReduction ||
        m_swsbMode >= SWSB_ENCODE_MODE::FiveDistPipe) {
      m_LatencyLong64Pipe = 12;
    }

    m_DB = new DepSetBuilder(k.getModel());
    m_buckets = new Bucket[m_DB->getTOTAL_BUCKETS()];

    // set sbid count to 16 if not given
    if (sbid_count)
      m_SBIDCount = sbid_count;
    else
      m_SBIDCount = 16;
    m_freeSBIDList.resize(m_SBIDCount);
  }

  ~SWSBAnalyzer() {
    delete m_DB;
    delete[] m_buckets;
  }
  SWSBAnalyzer(const SWSBAnalyzer&) = delete;
  SWSBAnalyzer& operator=(const SWSBAnalyzer&) = delete;

  void run();

  // getNumOfDistPipe - get number of in-order distance pipes of the given mode
  static uint32_t getNumOfDistPipe(SWSB_ENCODE_MODE mode);

private:
  // postProcess - last step of "run"
  void postProcess();

  void postProcessReadModifiedWriteOnByteDst();
  void postProcessRemoveRedundantSync();

private:
  // activeSBIDsTy - keep track of the depended SBID an inst depends on,
  // and the list of DepSet where this SBID comes from
  typedef std::vector<std::pair<SBID,  std::vector<DepSet*>>> activeSBIDsTy;

  // activeSBIDs: input list that the sbid this dep has dependency on will be
  // added into. This list will later on be pass to processactiveSBIDs to set the
  // swsb id dependency to inst accordingly needSyncForShootDownInst: if the
  // sync to the sbid on the instruction is required. If the instruction is
  // possiblely being shoot down, we have to add a sync to the id is synced with
  // because we will clear the dependency
  void calculateDependence(DepSet &dep, SWSB &swsb,
                           const Instruction &currInst,
                           activeSBIDsTy &activeSBIDs,
                           bool &needSyncForShootDownInst);
  void processactiveSBIDs(SWSB &distanceDependency, const DepSet *input,
                          Block *bb, InstList::iterator iter,
                          activeSBIDsTy &activeSBIDs);

  // helper function to pick a free SBID and set it to distanceDependency.
  // This function only set SBID to distanceDependency, will not assign
  // distanceDependency to the inst
  SBID &assignSBID(DepSet *input, DepSet *output, Instruction &inst,
                   SWSB &distanceDependency, InstList::iterator insertPoint,
                   Block *curBB, bool needSyncForShootDown);

  // helper fuction to set out-of-order dependecy. Called by
  // calculateDependence. This function create SBID with depedency to given dep,
  // and add it to activeSBIDs it also set needSyncForShootDownInst if required.
  void setSbidDependency(DepSet &dep, const Instruction &currInst,
                         bool &needSyncForShootDownInst,
                         activeSBIDsTy &activeSBIDs);

  // helper fuction to set swsb for dependency to in-order instructions.
  void setDistanceDependency(DepSet *dep, SWSB &swsb, bool isWAW,
                             DEP_PIPE prevDepPipe, DEP_PIPE currDepPipe,
                             const Instruction &currInst);

  // clear dependency of the given dep
  void clearDepBuckets(DepSet &dep);
  // clear all sbid, set ids to all free and insert sync to sync with all pipes
  // return if there are active sbid in-use before and sync.allrd/sync.allwr
  // instructions are inserted to sync them
  bool clearSBIDDependence(InstList::iterator insertPoint,
                           Instruction *lastInst, Block *bb);
  // clear given input and output dependency in the buckets (for in-order pipes
  // only)
  void clearBuckets(DepSet *input, DepSet *output);

  // helper function to insert sync.allrd and sync.allwr before the given
  // inserPoint of given bb
  void insertSyncAllRdWr(InstList::iterator insertPoint, Block *bb);

  // a helper function to increase inst id counter based on the current encoding
  // mode
  void advanceInorderInstCounter(DEP_PIPE dep_pipe);

  // get number of dist pipe according to SWSB_ENCODE_MODE
  uint32_t getNumOfDistPipe();

  // addRMWDependencyIfReqruied - add read dependnecy to input DepSet if the
  // instruction has RMW behavior, which is, has byte type dst XeHPC+ feature
  void addRMWDependencyIfReqruied(DepSet &input, DepSet &output);

  // add swsb into instruction, insert sync if the added swsb is not compatible
  // with the existed swsb in the inst
  void addSWSBToInst(InstListIterator instIt, const SWSB &swsb, Block &block);

  // a helper function to check if the RAW depedency to acc/flag can be
  // handled by HW, and hence swsb is not required. isRAW will be set to false for the
  // case. This function is used by calculateDependence.
  // The function must be called when there is RAW dependency detected.
  void checkAccFlagRAW(bool& isRAW, const DepSet &currDep, const DepSet &targetDep);

  // insert sync and A@1 for the last instruction in a BB
  void forceSyncLastInst(const Instruction &lastInst, Block &lastBB);

  // verify if the swsb is valid on the instruction instIt. If not, insert sync.nop
  // for swsb information until it is valid, but do not set swsb to the instruction.
  // preferMoveOutSBID - perfer move out sbid to sync.nop if possible. If set to
  // false, try to move distance out first.
  // Return the newly inserted sync.nop instructions if any
  std::vector<Instruction*>
  adjustSWSB(Block &block, const InstListIterator instIt,
             SWSB &swsb, bool preferMoveOutSBID);


  /// ------------ HW Workaround Information ------------ ///
  // MathWAInfo: For a math instruction, when the following instruction has
  // different predication to the math, should assume the math taking the entire
  // GRF in it's dst no matter the access region and channels are.
  struct MathWAInfo {
    bool previous_is_math = false;
    DepSet *dep_set = nullptr;
    // a special id to identify this DepSet when trying to clean it from buckets
    const InstIDs math_id = {std::numeric_limits<uint32_t>::max(), 0};
    Instruction *math_inst = nullptr;
    SBID math_sbid = {0, true, DEP_TYPE::NONE};

    void reset() {
      previous_is_math = false;
      dep_set = nullptr;
      math_inst = nullptr;
      math_sbid = {0, true, DEP_TYPE::NONE};
    }
  } math_wa_info;

private:
  const uint32_t MAX_GRF_BUCKETS = 128;
  // Instruction having 64-bit type destination having 14 latency
  // All the other instructions having 10 latency
  int m_LatencyLong64Pipe = 14;
  int m_LatencyInOrderMath = 18;
  int m_LatencyInOrderPipe = 10;

private:
  // m_SBIDCound - number of SBID can be used
  uint32_t m_SBIDCount;

  // m_InstIdCounter - record the current instruction state
  InstIDs m_InstIdCounter;

  Kernel &m_kernel;
  ErrorHandler &m_errorHandler;

  Bucket *m_buckets = nullptr;
  DepSetBuilder *m_DB = nullptr;

  // This is the list to recored all sbid, if it's free or not
  std::vector<SBID> m_freeSBIDList;

  // m_SBIDRRCounter - the round robin counter for SB id reuse
  unsigned int m_SBIDRRCounter;

  // id to dep set mapping, this tracks for which instructions' dependency that
  // this id is currently on. While we're re-using id, we clean up the
  // dependency
  std::map<uint32_t, std::pair<DepSet *, DepSet *>> m_IdToDepSetMap;

  // m_distanceTracker - Track the DepSet of in-order instructions to see if
  // their latency is satisfied. If the distance to current instruction is
  // larger then the latency, then we no need to track the dependency anymore,
  // remove the node from m_distanceTracker
  struct distanceTrackerNode {
    distanceTrackerNode(DepSet *in, DepSet *out) : input(in), output(out) {}
    DepSet *input;
    DepSet *output;
  };
  std::list<distanceTrackerNode> m_distanceTracker;

  bool m_initPoint;

  SWSB_ENCODE_MODE m_swsbMode;

  const int MAX_VALID_DISTANCE;

};
} // namespace iga
#endif
