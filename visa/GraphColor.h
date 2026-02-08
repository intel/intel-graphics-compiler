/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __GRAPHCOLOR_H__
#define __GRAPHCOLOR_H__

#include "Assertions.h"
#include "BitSet.h"
#include "G4_IR.hpp"
#include "RPE.h"
#include "SpillManagerGMRF.h"
#include "VarSplit.h"

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Support/Allocator.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SetVector.h"
#include "common/LLVMWarningsPop.hpp"
// clang-format on

#include <limits>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <unordered_set>
#include <vector>

#define BITS_DWORD 32
#define ROUND(x, y) ((x) + ((y - x % y) % y))

namespace vISA {
const float MAXSPILLCOST = (std::numeric_limits<float>::max());
const float MINSPILLCOST = -(std::numeric_limits<float>::max());

enum BankConflict {
  BANK_CONFLICT_NONE,
  BANK_CONFLICT_FIRST_HALF_EVEN,
  BANK_CONFLICT_FIRST_HALF_ODD,
  BANK_CONFLICT_SECOND_HALF_EVEN,
  BANK_CONFLICT_SECOND_HALF_ODD
};

class VarSplit;
class SpillAnalysis;

class BankConflictPass {
private:
  GlobalRA &gra;
  bool forGlobal;

  BankConflict setupBankAccordingToSiblingOperand(BankConflict assignedBank,
                                                  unsigned offset,
                                                  bool oneGRFBank);
  void setupEvenOddBankConflictsForDecls(G4_Declare *dcl_1, G4_Declare *dcl_2,
                                         unsigned offset1, unsigned offset2,
                                         BankConflict &srcBC1,
                                         BankConflict &srcBC2);
  void setupBankConflictsOneGRFOld(G4_INST *inst, int &bank1RegNum,
                                   int &bank2RegNum, float GRFRatio,
                                   unsigned &internalConflict);
  bool isOddOffset(unsigned offset) const;
  void setupBankConflictsforDPAS(G4_INST *inst);
  void setupBankConflictsfor2xDPAS(G4_INST *inst);
  bool hasDpasInst = false;

  void setupBankConflictsforTwoGRFs(G4_INST *inst);
  void setupBankConflictsforMad(G4_INST *inst);
  void setupBundleConflictsforTwoSrcsInst(G4_INST *inst);
  void setupBankConflictsForBB(G4_BB *bb, unsigned &threeSourceInstNum,
                               unsigned &sendInstNum, unsigned numRegLRA,
                               unsigned &internalConflict);
  void setupBankConflictsForBBTGL(G4_BB *bb, unsigned &threeSourceInstNum,
                                  unsigned &sendInstNum, unsigned numRegLRA,
                                  unsigned &internalConflict);
  bool hasInternalConflict3Srcs(BankConflict *srcBC);
  void setupBankForSrc0(G4_INST *inst, G4_INST *prevInst);
  void getBanks(G4_INST *inst, BankConflict *srcBC, G4_Declare **dcls,
                G4_Declare **opndDcls, unsigned *offset);
  void getPrevBanks(G4_INST *inst, BankConflict *srcBC, G4_Declare **dcls,
                    G4_Declare **opndDcls, unsigned *offset);

public:
  bool setupBankConflictsForKernel(bool doLocalRR, bool &threeSourceCandidate,
                                   unsigned numRegLRA,
                                   bool &highInternalConflict);

  BankConflictPass(GlobalRA &g, bool global) : gra(g), forGlobal(global) {}
};

// A LiveRange's Register Class (RC) is expressed as a bitset of
// forbidden physical registers that it cannot be assigned to.
// -- Address and Flag live ranges currently all belong to same RC
// (FBD_ADDR/FBD_FLAG) as there are no restrictions on their assignment.
// -- The rest of RCs are for GRF live ranges. By default such live ranges have
// FBD_RESERVEDGRF, which reserves a number of GRFs such as r0/r1/RN-1 based on
// kernel type and user options.
// -- The other GRF RCs (e.g., FBD_EOT) reserves additional GRFs on top of
// FDB_RESERVEDGRF base on their usage in the kernel.
// -- RCs must be pre-defined, dynamic creation of new RCs is not supported. If
// your live range belongs to more than one RC, you should add a new RC and
// initialize it in generateForbiddenTemplates()
enum class forbiddenKind {
  FBD_ADDR = 0,
  FBD_FLAG = 1,
  FBD_SCALAR,
  FBD_RESERVEDGRF,
  FBD_EOT,
  FBD_LASTGRF,
  FBD_EOTLASTGRF,
  FBD_CALLERSAVE,
  FBD_CALLEESAVE,
  FBD_UNASSIGNED, // This should only ever exist in between rounds of
                  // incremental RA.
};

constexpr bool isGRFRegClass(forbiddenKind kind) {
  return kind == forbiddenKind::FBD_RESERVEDGRF ||
         kind == forbiddenKind::FBD_EOT || kind == forbiddenKind::FBD_LASTGRF ||
         kind == forbiddenKind::FBD_EOTLASTGRF ||
         kind == forbiddenKind::FBD_CALLERSAVE ||
         kind == forbiddenKind::FBD_CALLEESAVE;
}

// Return true if k1 is a sub RC of k2, i.e., all of k2's forbidden registers
// are also forbidden in k1.
constexpr bool isSubRegClass(forbiddenKind k1, forbiddenKind k2) {
  if (k1 == k2)
    return true;
  if (k2 == forbiddenKind::FBD_UNASSIGNED)
    return true;
  if (k2 == forbiddenKind::FBD_RESERVEDGRF && isGRFRegClass(k1))
    return true;
  if (k1 == forbiddenKind::FBD_EOTLASTGRF)
    return k2 == forbiddenKind::FBD_EOT || k2 == forbiddenKind::FBD_LASTGRF;
  return false;
}

enum class AugmentationMasks {
  Undetermined = 0,
  Default16Bit = 1,
  Default32Bit = 2,
  Default64Bit = 3,
  DefaultPredicateMask = 4,
  NonDefault = 5,
};

class LiveRange final {
  G4_RegVar *const var;
  G4_Declare *const dcl;
  const G4_RegFileKind regKind;
  forbiddenKind forbiddenType = forbiddenKind::FBD_UNASSIGNED;
  BitSet *forbidden = nullptr;
  bool spilled = false;
  bool isUnconstrained = false;

  GlobalRA &gra;
  unsigned numRegNeeded;
  unsigned degree = 0;
  unsigned refCount = 0;
  unsigned parentLRID = 0;
  AssignedReg reg;
  float spillCost = 0.0f;
  BankConflict bc = BANK_CONFLICT_NONE;

  union {
    uint16_t bunch = 0;
    struct {
      uint16_t calleeSaveBias : 1; // indicates if the var is biased to get a
                                   // callee-save assignment or not
      uint16_t callerSaveBias : 1; // indicates if the var is biased to get a
                                   // caller-save assignment or not
      uint16_t isEOTSrc : 1; // Gen7 only, Whether the liveRange is the message
                             // source of an EOT send
      uint16_t retIp : 1; // variable is the return ip and should not be spilled

      uint16_t active : 1;
      uint16_t isInfiniteCost : 1;
      uint16_t isCandidate : 1;
      uint16_t isPseudoNode : 1;
      uint16_t isPartialDeclare : 1;
      uint16_t isSplittedDeclare : 1;
    };
  };

  LiveRange(G4_RegVar *v, GlobalRA &);

public:
  static LiveRange *createNewLiveRange(G4_Declare *dcl, GlobalRA &gra);

  void initialize();
  void initializeForbidden();

  void *operator new(size_t sz, llvm::SpecificBumpPtrAllocator<LiveRange> &m) {
    return m.Allocate();
  }

  void setBitFieldUnionValue(uint16_t v) { bunch = v; }

  void setDegree(unsigned d) { degree = d; }
  unsigned getDegree() const { return degree; }

  void setUnconstrained(bool d) { isUnconstrained = d; }
  bool getIsUnconstrained() const { return isUnconstrained; }

  unsigned getNumRegNeeded() const { return numRegNeeded; }

  void subtractDegree(unsigned d) {
    vISA_ASSERT(d <= degree, ERROR_INTERNAL_ARGUMENT);
    degree -= d;
  }

  void setActive(bool v) { active = v; }
  bool getActive() const { return active; }

  void emit(std::ostream &output) const {
    output << getVar()->getDeclare()->getName();
    if (reg.phyReg != NULL) {
      output << "(";
      reg.phyReg->emit(output);
      output << '.' << reg.subRegOff << ':';
      output << TypeSymbol(getVar()->getDeclare()->getElemType()) << ")";
    }
    output << "(size = " << getDcl()->getByteSize()
           << ", spill cost = " << getSpillCost()
           << ", degree = " << getDegree() << ")";
  }

  unsigned getRefCount() const { return refCount; }
  void setRefCount(unsigned count) { refCount = count; }

  float getSpillCost() const { return spillCost; }
  void setSpillCost(float cost) { spillCost = cost; }

  bool getIsInfiniteSpillCost() const { return isInfiniteCost; }
  void checkForInfiniteSpillCost(G4_BB *bb,
                                 std::list<G4_INST *>::reverse_iterator &it);

  G4_VarBase *getPhyReg() const { return reg.phyReg; }

  unsigned getPhyRegOff() const { return reg.subRegOff; }

  void setPhyReg(G4_VarBase *pr, unsigned off) {
    vISA_ASSERT(pr->isPhyReg(), ERROR_UNKNOWN);
    reg.phyReg = pr;
    reg.subRegOff = off;
  }

  void resetPhyReg() {
    reg.phyReg = nullptr;
    reg.subRegOff = 0;
  }

  bool getIsPseudoNode() const { return isPseudoNode; }
  void setIsPseudoNode() { isPseudoNode = true; }
  bool getIsPartialDcl() const { return isPartialDeclare; }
  void setIsPartialDcl() { isPartialDeclare = true; }
  bool getIsSplittedDcl() const { return isSplittedDeclare; }
  void setIsSplittedDcl(bool v) { isSplittedDeclare = v; }
  BankConflict getBC() const { return bc; }
  void setBC(BankConflict c) { bc = c; }
  void setParentLRID(int id) { parentLRID = id; }
  unsigned getParentLRID() const { return parentLRID; }

  // From VarBasis
public:
  void setForbidden(forbiddenKind f);
  void markForbidden(vISA::Mem_Manager &GCMem, int reg, int numReg);
  const BitSet *getForbidden();
  int getNumForbidden();
  G4_RegVar *getVar() const { return var; }
  G4_Declare *getDcl() const { return dcl; }
  G4_RegFileKind getRegKind() const { return regKind; }
  void dump() const;

  void setCalleeSaveBias(bool v) { calleeSaveBias = v; }
  bool getCalleeSaveBias() const { return calleeSaveBias; }

  void setCallerSaveBias(bool v) { callerSaveBias = v; }
  bool getCallerSaveBias() const { return callerSaveBias; }

  void setEOTSrc() { isEOTSrc = true; }
  bool getEOTSrc() const { return isEOTSrc; }

  void setRetIp() { retIp = true; }
  bool isRetIp() const { return retIp; }

  bool isSpilled() const { return spilled; }
  void setSpilled(bool v) { spilled = v; }

  void setCandidate(bool v) { isCandidate = v; }
  bool getCandidate() const { return isCandidate; }

  void resetForbidden() {
    forbidden = nullptr;
    forbiddenType = forbiddenKind::FBD_UNASSIGNED;
  }

private:
  // const Options *m_options;
  unsigned getForbiddenVectorSize() const;
}; // class LiveRange
} // namespace vISA
using LIVERANGE_LIST = std::list<vISA::LiveRange *>;
using LIVERANGE_LIST_ITER = LIVERANGE_LIST::iterator;
using LiveRangeVec = std::vector<vISA::LiveRange *>;

// A mapping from the pseudo decl created for caller save/restore, to the ret
// val This is used in augmentIntfGraph to prune interference edges for fcall
// ret val
typedef std::map<vISA::G4_Declare *, vISA::G4_Declare *> FCALL_RET_MAP;
typedef std::map<vISA::G4_Declare *, std::pair<vISA::G4_INST *, unsigned>>
    CALL_DECL_MAP;

namespace vISA {
typedef struct Interval {
  G4_INST *start = nullptr;
  G4_INST *end = nullptr;

  ~Interval() = default;
  Interval() = default;
  Interval(const Interval &) = default;
  Interval &operator=(const Interval &) = default;
  bool operator!=(const Interval &Other) {
    return start != Other.start || end != Other.end;
  }
  Interval(G4_INST *s, G4_INST *e) {
    start = s;
    end = e;
  }
  bool intervalsOverlap(const Interval &second) const;
} Interval;
// Used as entry in priority queue for storing default, non-default
// intervals.
struct QueueEntry {
  G4_Declare *dcl;
  Interval interval;

  ~QueueEntry() = default;
  QueueEntry() = default;
  QueueEntry(const QueueEntry &) = default;
  QueueEntry &operator=(const QueueEntry &) = default;
  QueueEntry(G4_Declare *d, const Interval &i) {
    dcl = d;
    interval = i;
  }
};
using AllIntervals = std::vector<QueueEntry>;

struct criticalCmpForEndInterval {
  GlobalRA &gra;
  criticalCmpForEndInterval(GlobalRA &g);
  bool operator()(const QueueEntry &A, const QueueEntry &B) const;
};
struct AugmentPriorityQueue
    : std::priority_queue<QueueEntry, std::vector<QueueEntry>,
                          criticalCmpForEndInterval> {
  AugmentPriorityQueue(criticalCmpForEndInterval cmp);
  auto begin() const { return c.begin(); }
  auto end() const { return c.end(); }
};
} // namespace vISA
//
// A bit array records all interference information.
// (2D matrix is flatten to 1D array)
// Since the interference information is symmetric, we can use only
// half of the size. To simplify the implementation, we use the full
// size of the bit array.
//
namespace vISA {
class Augmentation {
private:
  // pair of default mask, non-default mask
  using MaskDeclares = std::pair<llvm::DenseSet<unsigned int>, llvm::DenseSet<unsigned int>>;
  G4_Kernel &kernel;
  Interference &intf;
  GlobalRA &gra;
  const LivenessAnalysis &liveAnalysis;
  const LiveRangeVec &lrs;
  FCALL_RET_MAP &fcallRetMap;
  CALL_DECL_MAP callDclMap;
  std::unordered_map<FuncInfo *, PhyRegSummary> localSummaryOfCallee;
  AllIntervals sortedIntervals;
  AugmentPriorityQueue defaultMaskQueue{criticalCmpForEndInterval(gra)};
  AugmentPriorityQueue nonDefaultMaskQueue{criticalCmpForEndInterval(gra)};
  // overlapDclsWithFunc holds default and non-default range live across
  // all call sites of func.
  std::unordered_map<FuncInfo *, MaskDeclares> overlapDclsWithFunc;
  std::unordered_map<G4_Declare *, MaskDeclares> retDeclares;
  VarReferences refs;
  bool useGenericAugAlign = false;
  enum class ArgType {
      Init = 0,
      // arg may be defined in callee or may be defined once
      // and used by multiple call sites. This is different
      // than other types as the def may appear anywhere other
      // than entry BB. This is a corner case. Live-interval
      // of such args span complete function wherever the
      // variable is referenced.
      Unknown = 1,
      // Standard arg that's fully defined before each call site
      // in same BB.
      DefBeforeEachCall = 2,
      // Args that are input to kernel or defined once in entry
      // BB but used throughout program belong to this type.
      LiveThrough = 3,
  };
  enum class RetValType {
      Init = 0,
      // retval pattern irregular. Live-interval of such retval
      // span complete function wherever the variable is referenced.
      Unknown = 1,
      // retval pattern regular. It's defined in callee and
      // used in BB immediate after call.
      Regular = 2,
  };
  class ArgRetValInfo {
  public:
    ArgType argType = ArgType::Init;
    RetValType retValType = RetValType::Init;
    // Struct to store type of arg/retval and subroutines where it appears.
    // We use a set because variables such as HWTID are defined once in
    // kernel and used in several subroutines.
    std::unordered_set<const FuncInfo *> subroutines;
  };
  class BBWrapper {
    public:
      G4_BB *bb = nullptr;
      bool endsWithCall = false;
      FuncInfo *calleeInfo = nullptr;
  };
  std::unordered_map<G4_Declare *, ArgRetValInfo> argsRetVal;
  std::unordered_map<FuncInfo*, std::unordered_set<G4_Declare*>> argsPerSub;
  std::unordered_map<FuncInfo *, std::unordered_set<G4_Declare *>> retValPerSub;
  std::unordered_map<G4_Declare *, std::unordered_set<FuncInfo *>> unknownArgRetvalRefs;
  std::unordered_map<G4_Declare *, std::unordered_set<FuncInfo *>> nonGRFRefs;
  std::vector<BBWrapper> bbCache;
  // Store home function for given variable. Home function is defined as
  // function that contains explicit def or use of the variable. Each regular
  // variable has a unique home function. Arg/retval don't have a unique home
  // function as they're usually defined in caller and used in callee, ie
  // they're referenced in different functions.
  //
  // This vector is indexed by G4_Declare's dclId.
  //
  // It's assumed that augmentation doesn't require resizing this vector
  // despite inserting new SCALL dcls as they're all function local.
  std::vector<FuncInfo *> homeFunc;
  // Sadly, we don't have a way to map G4_INST to containing G4_BB.
  // So we create it in Augmentation using below vector. To get
  // FuncInfo* for a G4_INST, we dereference instToFunc using
  // lexical id of the instruction. We use vector instead of a
  // map because it's used often, is faster than map, and
  // importantly it's dense so there's no savings to be had with a map.
  std::vector<FuncInfo *> instToFunc;
  const bool hasSubroutines = false;

  void populateBBCache();
  void populateFuncMaps();
  void populateHomeFunc();
  bool isSubroutineArg(G4_Declare *dcl) const {
    auto it = argsRetVal.find(dcl);
    if (it == argsRetVal.end())
      return false;
    if ((*it).second.argType != ArgType::Init) {
      vISA_ASSERT((*it).second.retValType == RetValType::Init,
                  "expecting retval type");
      return true;
    }
    return false;
  }

  bool isSubroutineRetVal(G4_Declare *dcl) const {
    auto it = argsRetVal.find(dcl);
    if (it == argsRetVal.end())
      return false;
    if ((*it).second.retValType != RetValType::Init) {
      vISA_ASSERT((*it).second.argType == ArgType::Init,
                  "expecting retval type");
      return true;
    }
    return false;
  }

  template <ArgType Type> bool isArgType(G4_Declare *dcl) const {
    auto it = argsRetVal.find(dcl);
    if (it == argsRetVal.end())
      return false;
    return (*it).second.argType == Type;
  }

  template <RetValType Type> bool isRetvalType(G4_Declare *dcl) const {
    auto it = argsRetVal.find(dcl);
    if (it == argsRetVal.end())
      return false;
    return (*it).second.retValType == Type;
  }

  bool isLiveThroughArg(G4_Declare *dcl) const;
  bool isDefBeforeEachCallArg(G4_Declare *dcl) const;
  bool isUnknownArg(G4_Declare *dcl) const;
  bool isSingleDefInEntryBB(G4_Declare* dcl) const;
  bool isUnknownRetVal(G4_Declare *dcl) const;
  bool isRegularRetVal(G4_Declare *dcl) const;
  bool isUnknownArgOrRetval(G4_Declare *dcl) const;
  FuncInfo *computeHomeFunc(G4_Declare *dcl);
  bool hasUniqueFuncHome(G4_Declare *dcl) const;
  FuncInfo *getUniqueFuncHome(G4_Declare *dcl) const;

  void verifyHomeLocation();

  bool updateDstMaskForGather(G4_INST *inst, std::vector<unsigned char> &mask);
  bool updateDstMaskForGatherRaw(G4_INST *inst,
                                 std::vector<unsigned char> &mask,
                                 const G4_SendDescRaw *rawDesc);
  bool updateDstMaskForGatherUnified(G4_INST *inst,
                                     std::vector<unsigned char> &mask,
                                     const G4_SendgDesc *unifiedDesc);
  void updateDstMask(G4_INST *inst, bool checkCmodOnly);
  static unsigned getByteSizeFromMask(AugmentationMasks type);
  bool isDefaultMaskDcl(G4_Declare *dcl, unsigned simdSize,
                        AugmentationMasks type);
  bool isDefaultMaskSubDeclare(unsigned char *mask, unsigned lb, unsigned rb,
                               G4_Declare *dcl, unsigned simdSize);
  bool verifyMaskIfInit(G4_Declare *dcl, AugmentationMasks mask);
  bool checkGRFPattern3(G4_Declare *dcl, G4_DstRegRegion *dst, unsigned maskOff,
                        unsigned lb, unsigned rb, unsigned execSize);
  bool checkGRFPattern2(G4_Declare *dcl, G4_DstRegRegion *dst, unsigned maskOff,
                        unsigned lb, unsigned rb, unsigned execSize);
  bool checkGRFPattern1(G4_Declare *dcl, G4_DstRegRegion *dst, unsigned maskOff,
                        unsigned lb, unsigned rb, unsigned execSize);
  void markNonDefaultDstRgn(G4_INST *inst, G4_Operand *opnd);
  bool markNonDefaultMaskDef();
  void updateStartIntervalForSubDcl(G4_Declare *dcl, G4_INST *curInst,
                                    G4_Operand *opnd);
  void updateEndIntervalForSubDcl(G4_Declare *dcl, G4_INST *curInst,
                                  G4_Operand *opnd);
  void updateStartInterval(const G4_Declare *dcl, G4_INST *curInst);
  void updateEndInterval(const G4_Declare *dcl, G4_INST *curInst);
  void updateStartIntervalForLocal(G4_Declare *dcl, G4_INST *curInst,
                                   G4_Operand *opnd);
  void updateEndIntervalForLocal(G4_Declare *dcl, G4_INST *curInst,
                                 G4_Operand *opnd);
  void buildUnknownArgRetval();
  void buildLiveIntervals(FuncInfo* func);
  void buildLiveIntervals();
  void sortLiveIntervals();
  void startIntervalForLiveIn(FuncInfo *funcInfo, G4_BB *bb);
  void handleCallSite(G4_BB *curBB, unsigned int &funcCnt);
  void handleDstOpnd(FuncInfo *funcInfo, G4_BB *curBB, G4_INST *inst);
  void handleCondMod(FuncInfo* funcInfo, G4_INST *inst);
  void endIntervalForLiveOut(FuncInfo *funcInfo, G4_BB *bb);
  void handleSrcOpnd(FuncInfo *funcInfo, G4_BB *curBB, G4_Operand *src);
  void handlePred(FuncInfo* funcInfo, G4_INST *inst);
  void handleNonReducibleExtension(FuncInfo *funcInfo);
  void handleLoopExtension(FuncInfo *funcInfo);
  std::unordered_set<G4_BB *> getAllJIPTargetBBs(FuncInfo *funcInfo);
  std::vector<std::pair<G4_BB *, G4_BB *>>
  getNonLoopBackEdges(FuncInfo *funcInfo);
  void handleNonLoopBackEdges(FuncInfo *funcInfo);
  void extendVarLiveness(FuncInfo *funcInfo, G4_BB *bb, G4_INST *inst);
  unsigned getEnd(const G4_Declare *dcl) const;
  bool isNoMask(const G4_Declare *dcl, unsigned size) const;
  bool isConsecutiveBits(const G4_Declare *dcl, unsigned size) const;
  bool isCompatible(const G4_Declare *testDcl,
                    const G4_Declare *biggerDcl) const;
  void buildInterferenceIncompatibleMask();
  void buildInteferenceForCallSiteOrRetDeclare(std::vector<G4_Declare*>& dcls,
                                               MaskDeclares *mask);
  void buildInteferenceForCallsite(FuncInfo *func);
  void buildInteferenceForRetDeclares();
  void buildSummaryForCallees();
  void expireIntervals(unsigned startIdx);
  void buildSIMDIntfDcl(G4_Declare *newDcl);
  void storeOverlapWithCallRet(G4_Declare *newDcl,
                               const std::vector<bool> &globalVars);
  void handleSIMDIntf(G4_Declare *firstDcl, G4_Declare *secondDcl, bool isCall);
  bool weakEdgeNeeded(AugmentationMasks, AugmentationMasks);
  void addSIMDIntfDclForCallSite(G4_BB *callBB,
                                 const std::vector<bool> &globalVars);
  void addSIMDIntfForRetDclares(G4_Declare *newDcl,
                                const std::vector<bool> &globalVars);
  void discoverArgs(FuncInfo *func);
  void discoverRetVal(FuncInfo *func);
  ArgType computeArgType(FuncInfo *func, G4_Declare *arg);
  RetValType computeRetValType(FuncInfo *func, G4_Declare *retVal);

  // Functions used for verification
  void dumpSortedIntervals();
  void dumpArgs(SparseBitVector &subArgs);
  void dumpRetVal(SparseBitVector &subRetVal);

public:
  Augmentation(Interference &i, const LivenessAnalysis &l, GlobalRA &g);
  ~Augmentation();
  Augmentation(const Augmentation&) = delete;
  Augmentation& operator=(const Augmentation&) = delete;

  void augmentIntfGraph();

  const auto &getSortedLiveIntervals() const { return sortedIntervals; }
};

// This class stores base matrices used for interference building for graph coloring.
struct InterferenceMatrixStorage {
  // This member is a half triangle representation of interference graph implemented
  // as a sparse bitvector. Interference construction uses this member. When
  // incremental RA is enabled, this member is updated incrementally.
  std::vector<SparseBitVector> sparseMatrix;
  // This member is constructed after SIMT and SIMD interference are computed.
  // It's a full triangle representation of interference matrix with trivial
  // traversal. This member is reconstructed in each graph color iteration.
  std::vector<std::vector<unsigned>> sparseIntf;
};

// This class contains implementation of various methods to implement
// incremental intf computation. Instance of this class is created
// once and stored in GlobalRA. This class should therefore not
// hold pointer to GraphColor/Interference or such other short-living
// instances.
class IncrementalRA {
  friend Interference;

  const SparseBitVector &getSparseMatrix(unsigned int id) {
    return sparseMatrix[id];
  }

private:
  GlobalRA &gra;
  G4_Kernel &kernel;
  LiveRangeVec lrs;
  std::vector<SparseBitVector>& sparseMatrix;
  std::vector<std::vector<unsigned>>& sparseIntf;
  G4_RegFileKind selectedRF = G4_RegFileKind::G4_UndefinedRF;
  unsigned int level = 0;
  std::unordered_set<G4_Declare *> needIntfUpdate;
  std::unordered_set<G4_Declare *> evenAlignCache;
  unsigned int maxDclId = 0;
  // Map of root G4_Declare* -> id assigned to its G4_RegVar
  // This allows us to reuse ids from previous iteration.
  std::unordered_map<G4_Declare *, unsigned int> varIdx;
  unsigned int maxVarIdx = 0;

  // Only BBs present in this set have interference computation
  // done for them. It's assumed other BBs have no change in
  // liveness so no change in interference is expected.
  std::unordered_set<const G4_BB *> updateIntfForBB;

  bool updateIntfForBBValid = false;

  // Reset state to mark start of new type of GRA (eg, from flag to GRF)
  void reset();

  // During incremental update we need to remove edges between each
  // incremental intf candidate and their neighbor.
  void resetEdges();

  // Compute BBs needing incremental update in current iteration.
  // List of BBs needing update is stored in updateIntfForBB set.
  void collectBBs(const LivenessAnalysis *liveness);

  // Special variables such as r0 that are live-out have their
  // interference marked already. So these should never be treated as
  // incremental RA candidates.
  void eraseLiveOutsFromIncrementalUpdate();

public:
  llvm::SpecificBumpPtrAllocator<LiveRange> mem;

  IncrementalRA(GlobalRA &g);

  bool isEnabled() const { return level > 0; }
  bool isEnabledWithVerification() const { return level == 2; }

  static bool isEnabled(G4_Kernel &kernel) {
    // 0 - disabled
    // 1 - enabled
    // 2 - enabled with verification
    return kernel.getOptions()->getuInt32Option(vISA_IncrementalRA) >= 1;
  }

  static bool isEnabledWithVerification(G4_Kernel &kernel) {
    return kernel.getOptions()->getuInt32Option(vISA_IncrementalRA) == 2;
  }

  void registerNextIter(G4_RegFileKind rf,
                        const LivenessAnalysis *liveness,
                        const Interference *intf);
  // After computing interference incrementally, GraphColor needs to clear
  // candidate list to prepare for new incremental RA temps.
  void clearCandidates() {
    needIntfUpdate.clear();
    updateIntfForBB.clear();
    updateIntfForBBValid = false;
    for (auto* evenAlignCandidate : evenAlignCache) {
      if (!evenAlignCandidate->getAliasDeclare())
        needIntfUpdate.insert(evenAlignCandidate);
    }
    evenAlignCache.clear();
  }

  LiveRangeVec &getLRs() { return lrs; }

  G4_RegFileKind getSelectedRF() const { return selectedRF; }

  // This method is invoked when a new G4_Declare is created and a
  // LiveRange instance needs to be added for it.
  void addNewRAVariable(G4_Declare *dcl);
  // This method is invoked when an existing RA variable is either
  // removed from the program or a change is expected in liveness
  // of a variable due to optimization.
  void markForIntfUpdate(G4_Declare *dcl);

  void skipIncrementalRANextIter();

  void moveFromHybridToGlobalGRF() {
    varIdx.clear();
    maxVarIdx = 0;
    reset();
  }

  // Return idx of a G4_RegVar if it was given an id in previous
  // iteration. If dcl was present in previous id assign phase
  // return pair <true, id>. When dcl is seen for first time,
  // return <false, X>. Second field of pair contains legal value
  // only if first field is true.
  std::pair<bool, unsigned int> getIdFromPrevIter(G4_Declare *dcl);

  // Record new dcl and id assigned to its G4_RegVar. Update
  // maxVarIdx so we know first free id in next RA iteration.
  void recordVarId(G4_Declare *dcl, unsigned int id);

  // Return next id that can be assigned to a new variable. In 1st
  // RA iteration, this returns 0 because no variables exist in
  // incremental RA. In 2nd RA iteration, this method returns
  // first available index that can be assigned to new variable.
  unsigned int getNextVarId(unsigned char RF) {
    if ((RF & selectedRF) == 0) {
      varIdx.clear();
      maxVarIdx = 0;
    }
    if (varIdx.size() == 0)
      return 0;
    return maxVarIdx + 1;
  }

  // Handle local split here. reduceBy argument tells us how many
  // G4_Declares were removed by resetGlobalRAStates().
  // TODO: Deprecate this method once we stop erasing old partial
  // dcls from kernel.Declares
  void reduceMaxDclId(unsigned int reduceBy) {
    if (!level)
      return;
    if (maxDclId > 0) {
      vISA_ASSERT(maxDclId >= reduceBy, "error removing partial dcls");
      maxDclId -= reduceBy;
    }
  }

  void resetPartialDcls();

  bool intfNeededForBB(const G4_BB *bb) const {
    // If incremental RA is not enabled then all BBs need intf update.
    if (!isEnabled() || !updateIntfForBBValid)
      return true;
    // If incremental RA is enabled then check whether we need to run
    // intf for BB.
    return updateIntfForBB.count(bb) > 0;
  }

  bool hasAnyCandidates() const { return needIntfUpdate.size() > 0; }

  bool intfNeededForVar(G4_Declare *dcl) const {
    if (needIntfUpdate.size() == 0)
      return true;
    return needIntfUpdate.count(dcl) > 0;
  }

  void evenAlignUpdate(G4_Declare *dcl) { evenAlignCache.insert(dcl); }

private:
  // For verification only
  std::vector<SparseBitVector> def_in;
  std::vector<SparseBitVector> def_out;
  std::vector<SparseBitVector> use_in;
  std::vector<SparseBitVector> use_out;
  std::vector<SparseBitVector> use_gen;
  std::vector<SparseBitVector> use_kill;

  std::unique_ptr<VarReferences> prevIterRefs;

  // Return true if verification passes, false otherwise
  bool verify(const LivenessAnalysis *curLiveness) const;

  // Copy over liveness sets from current iteration's liveness
  void copyLiveness(const LivenessAnalysis *liveness);

public:
  std::unordered_set<G4_Declare *> unassignedVars;

  // Compute variables that are left over in sorted list when
  // computing color order. This is to aid debugging only.
  void computeLeftOverUnassigned(const LiveRangeVec &sorted,
                                 const LivenessAnalysis &liveAnalysis);
};

class Interference {
  friend class Augmentation;

  // This stores compatible ranges for each variable. Such
  // compatible ranges will not be present in sparseIntf set.
  // We store G4_Declare* instead of id is because variables
  // allocated by LRA will not have a valid id.
  std::unordered_map<G4_Declare *, llvm::SmallSetVector<G4_Declare *, 16>>
      compatibleSparseIntf;

  GlobalRA &gra;
  G4_Kernel &kernel;
  const LiveRangeVec &lrs;
  IR_Builder &builder;
  const unsigned maxId;
  const unsigned rowSize;
  const unsigned splitStartId;
  const unsigned splitNum;
  unsigned *matrix = nullptr;
  const LivenessAnalysis *const liveAnalysis;
  Augmentation aug;
  IncrementalRA &incRA;

  std::vector<std::vector<unsigned>>& sparseIntf;

  // sparse interference matrix.
  // we don't directly update sparseIntf to ensure uniqueness
  // like dense matrix, interference is not symmetric (that is, if v1 and v2
  // interfere and v1 < v2, we insert (v1, v2) but not (v2, v1)) for better
  // cache behavior
  std::vector<SparseBitVector>& sparseMatrix;

  unsigned int denseMatrixLimit = 0;

  static void updateLiveness(SparseBitVector &live, uint32_t id, bool val) {
    if (val) {
      live.set(id);
    } else {
      live.reset(id);
    }
  }

  G4_Declare *getGRFDclForHRA(int GRFNum) const;

  // Only upper-half matrix is now used in intf graph.
  inline void safeSetInterference(unsigned v1, unsigned v2) {
    // Assume v1 < v2
    if (useDenseMatrix()) {
      unsigned col = v2 / BITS_DWORD;
      matrix[v1 * rowSize + col] |= 1 << (v2 % BITS_DWORD);
    } else {
      sparseMatrix[v1].set(v2);
    }
  }

  inline void safeClearInterference(unsigned v1, unsigned v2) {
    // Assume v1 < v2
    if (useDenseMatrix()) {
      unsigned col = v2 / BITS_DWORD;
      matrix[v1 * rowSize + col] &= ~(1 << (v2 % BITS_DWORD));
    } else {
      sparseMatrix[v1].reset(v2);
    }
  }

  inline void setBlockInterferencesOneWay(unsigned v1, unsigned col,
                                          unsigned block) {
    if (useDenseMatrix()) {
#ifdef _DEBUG
      vISA_ASSERT(
          sparseIntf.size() == 0,
          "Updating intf graph matrix after populating sparse intf graph");
#endif

      matrix[v1 * rowSize + col] |= block;
    } else {
      auto &&intfSet = sparseMatrix[v1];
      for (int i = 0; i < BITS_DWORD; ++i) {
        if (block & (1 << i)) {
          uint32_t v2 = col * BITS_DWORD + i;
          intfSet.set(v2);
        }
      }
    }
  }

  unsigned getInterferenceBlk(unsigned idx) const {
    vISA_ASSERT(useDenseMatrix(), "matrix is not initialized");
    return matrix[idx];
  }

  void addCalleeSaveBias(const SparseBitVector &live);

  void buildInterferenceAtBBExit(const G4_BB *bb, SparseBitVector &live);
  void buildInterferenceWithinBB(G4_BB *bb, SparseBitVector &live);
  void buildInterferenceForDst(G4_BB *bb, SparseBitVector &live, G4_INST *inst,
                               std::list<G4_INST *>::reverse_iterator i,
                               G4_DstRegRegion *dst);
  void buildInterferenceForFcall(G4_BB *bb, SparseBitVector &live,
                                 G4_INST *inst,
                                 std::list<G4_INST *>::reverse_iterator i,
                                 const G4_VarBase *regVar);

  inline void filterSplitDclares(unsigned startIdx, unsigned endIdx, unsigned n,
                                 unsigned col, unsigned &elt, bool is_split);
  void buildInterferenceWithLive(const SparseBitVector &live, unsigned i);
  void buildInterferenceWithSubDcl(unsigned lr_id, G4_Operand *opnd,
                                   SparseBitVector &live, bool setLive,
                                   bool setIntf);
  void buildInterferenceWithAllSubDcl(unsigned v1, unsigned v2);

  void markInterferenceForSend(G4_BB *bb, G4_INST *inst, G4_DstRegRegion *dst);

  void setOutOfBoundForbidden(G4_Operand *opnd);

  void setForbiddenGRFNumForSVMScatter(G4_INST *inst);

  void buildInterferenceWithLocalRA(G4_BB *bb);

  void buildInterferenceAmongLiveOuts();
  void buildInterferenceAmongLiveIns();

  void markInterferenceToAvoidDstSrcOverlap(G4_BB *bb, G4_INST *inst);

  void generateSparseIntfGraph();
  void countNeighbors();

  void setupLRs(G4_BB *bb);

public:
  Interference(const LivenessAnalysis *l, GlobalRA &g);

  ~Interference() {
    if (useDenseMatrix()) {
      delete[] matrix;
    }
  }
  Interference(const Interference&) = delete;
  Interference& operator=(const Interference&) = delete;

  bool useDenseMatrix() const {
    // The size check is added to prevent offset overflow in
    // generateSparseIntfGraph() and help avoid out-of-memory
    // issue in dense matrix allocation.
    unsigned long long size = static_cast<unsigned long long>(rowSize) *
                              static_cast<unsigned long long>(maxId);
    unsigned long long max = std::numeric_limits<unsigned int>::max();
    return (maxId < denseMatrixLimit) && (size < max);
  }

  const llvm::SmallSetVector<G4_Declare *, 16> *
  getCompatibleSparseIntf(G4_Declare *d) const {
    if (compatibleSparseIntf.size() > 0) {
      auto it = compatibleSparseIntf.find(d);
      if (it == compatibleSparseIntf.end()) {
        return nullptr;
      }
      return &it->second;
    }
    return nullptr;
  }

  size_t numVarsWithWeakEdges() const { return compatibleSparseIntf.size(); }

  void init() {
    if (useDenseMatrix()) {
      auto N = (size_t)rowSize * (size_t)maxId;
      matrix = new uint32_t[N](); // zero-initialize
    } else {
      sparseMatrix.resize(maxId);
    }
  }

  void computeInterference();
  void getNormIntfNum();
  void applyPartitionBias();
  bool interfereBetween(unsigned v1, unsigned v2) const;
  const std::vector<unsigned> &getSparseIntfForVar(unsigned id) const {
    return sparseIntf[id];
  }

  inline bool varSplitCheckBeforeIntf(unsigned v1, unsigned v2) const;

  void checkAndSetIntf(unsigned v1, unsigned v2) {
    if (v1 < v2) {
      safeSetInterference(v1, v2);
    } else if (v1 > v2) {
      safeSetInterference(v2, v1);
    }
  }

  void dumpInterference() const;
  void dumpVarInterference() const;
  bool dumpIntf(const char *) const;
  void interferenceVerificationForSplit() const;

  bool linearScanVerify() const;

  bool isStrongEdgeBetween(const G4_Declare *, const G4_Declare *) const;

  const Augmentation &getAugmentation() const { return aug; }
};

// Class to compute reg chart dump and dump it to ostream.
// Used only when -dumpregchart is passed.
class RegChartDump {
  const GlobalRA &gra;
  AllIntervals sortedLiveIntervals;
  std::unordered_map<G4_Declare *, std::pair<G4_INST *, G4_INST *>> startEnd;

public:
  void recordLiveIntervals(const AllIntervals &dcls);
  void dumpRegChart(std::ostream &, const LiveRangeVec &lrs, unsigned numLRs);

  RegChartDump(const GlobalRA &g) : gra(g) {}
};

class GraphColor {
  GlobalRA &gra;

  // Same as GRF count in G4_Kernel.
  const unsigned totalGRFRegCount;
  const unsigned numVar;
  // The original code has no comments whatsoever (sigh), but best as I can tell
  // this vector is used to track the active values held by each of A0's
  // phyiscal subreg. The values themselves correspond to a word in the
  // GRF home location of a spilled address variable. Each GRF home location is
  // represented by its allocation order and assumed to be 16-word wide
  // regardless of its actual size; in other words,
  // AddrSpillLoc0 has [0-15],
  // AddrSpillLoc1 has [16-31],
  // and so on.
  // When the clean up code sees a address fill of the form
  //    mov (N) a0.i AddrSpillLoc(K).S<1;1,0>:uw
  // it updates spAddrRegSig[i, i+N) = [K*16+S, K*16+S+N)
  // When it sees a write to AddrSpillLoc, i.e., a spill of the form
  //    mov (N) AddrSpillLoc(k) a0.i<1;1,0>:uw
  // it clears spAddrRegSig's entries that hold AddrSpillLoc(k).
  // If it encounters a non-fill write to A0 (e.g., send message descriptor
  // write), it also clears the corresponding bits in spAddrRegSig.
  //
  // FIXME: This code is very likely to be buggy, since its initial value is 0
  // and this conflicts with AddrSpillLoc0's first word.
  std::vector<unsigned> spAddrRegSig;
  Interference intf;
  PhyRegPool &regPool;
  IR_Builder &builder;
  LiveRangeVec &lrs;
  bool isHybrid;
  LIVERANGE_LIST spilledLRs;
  bool forceSpill;
  vISA::Mem_Manager GCMem;
  const Options *m_options;

  unsigned evenTotalDegree = 1;
  unsigned oddTotalDegree = 1;
  unsigned evenTotalRegNum = 1;
  unsigned oddTotalRegNum = 1;
  unsigned evenMaxRegNum = 1;
  unsigned oddMaxRegNum = 1;

  G4_Kernel &kernel;
  LivenessAnalysis &liveAnalysis;

  LiveRangeVec colorOrder;
  LIVERANGE_LIST unconstrainedWorklist;
  LIVERANGE_LIST constrainedWorklist;
  unsigned numColor = 0;

  bool failSafeIter = false;
  // Reserved GRF count for fail-safe RA
  unsigned reserveSpillGRFCount = 0;

  template<bool Support4GRFAlign>
  unsigned edgeWeightGRF(const LiveRange *lr1, const LiveRange *lr2);
  unsigned edgeWeightARF(const LiveRange *lr1, const LiveRange *lr2);
  static unsigned edgeWeightGRF(bool lr1EvenAlign, bool lr2EvenAlign,
                                unsigned lr1_nreg, unsigned lr2_nreg) {
    unsigned sum = lr1_nreg + lr2_nreg;
    if (!lr1EvenAlign) {
      return sum - 1;
    }

    if (!lr2EvenAlign)
      return sum + 1 - ((sum) % 2);

    return sum - 1 + (lr1_nreg % 2) + (lr2_nreg % 2);
  }

  static unsigned edgeWeightWith4GRF(int lr1Align, int lr2Align,
                                     unsigned lr1_nreg, unsigned lr2_nreg) {
    if (lr1Align < 4 && lr2Align < 4)
      return edgeWeightGRF(lr1Align == 2, lr2Align == 2, lr1_nreg, lr2_nreg);

    if (lr2Align == 4) {
      if (lr1Align < 2)
        return lr1_nreg + lr2_nreg - 1;
      if (lr1Align == 2) {
        // if (lr2_nreg % 2 == 0) -- lr2 size is even
        // return lr2_nreg + lr1_nreg;
        // if (lr2_nreg % 2 == 1) -- lr2 size is odd
        // return lr2_nreg + lr1_nreg + 1;

        return lr1_nreg + lr2_nreg + (lr2_nreg % 2);
      } else if (lr1Align == 4) {
        if (lr2_nreg % 4 == 0)
          // lr2 size is multiple of 4
          return lr1_nreg + lr2_nreg;

        // if lr2_nreg % 4 == 1 --  lr2 size is 1 + (4*n)
        // return lr1_nreg + lr2_nreg + 3;
        // if lr2_nreg % 2 == 0 -- lr2 size is 2 + (4*n)
        // return lr2_nreg + lr1_nreg + 2;
        // if lr2_nreg % 4 == 3 -- lr2 size is 3 + (4*n)
        // return lr2_nreg + lr1_nreg + 1;

        return lr1_nreg + lr2_nreg + 4 - (lr2_nreg % 4);
      }
    }

    vISA_ASSERT(lr1Align == 4, "unexpected condition");
    return edgeWeightWith4GRF(lr2Align, lr1Align, lr2_nreg, lr1_nreg);
  }

  void inline relax(LiveRange *lr1, unsigned int w) {
    // relax degree between 2 nodes
    VISA_DEBUG_VERBOSE({
      std::cout << "\t relax ";
      lr1->dump();
      std::cout << " degree(" << lr1->getDegree() << ") - " << w << "\n";
    });
    lr1->subtractDegree(w);

    unsigned availColor = numColor;
    availColor = numColor - lr1->getNumForbidden();

    if (lr1->getDegree() + lr1->getNumRegNeeded() <= availColor) {
      unconstrainedWorklist.push_back(lr1);
      lr1->setActive(false);
    }
  }

  template <bool Support4GRFAlign>
  void computeDegreeForGRF();
  void computeDegreeForARF();
  void computeSpillCosts(bool useSplitLLRHeuristic, const RPE *rpe);
  void determineColorOrdering();
  void removeConstrained();
  void relaxNeighborDegreeGRF(LiveRange *lr);
  void relaxNeighborDegreeARF(LiveRange *lr);
  bool assignColors(ColorHeuristic heuristicGRF, bool doBankConflict,
                    bool highInternalConflict, bool doBundleConflict = false);
  bool assignColors(ColorHeuristic h) {
    // Do graph coloring without bank conflict reduction.
    return assignColors(h, false, false);
  }

  void clearSpillAddrLocSignature() {
    std::fill(spAddrRegSig.begin(), spAddrRegSig.end(), 0);
  }
  void pruneActiveSpillAddrLocs(G4_DstRegRegion *, unsigned, G4_Type);
  void updateActiveSpillAddrLocs(G4_DstRegRegion *, G4_SrcRegRegion *,
                                 unsigned execSize);
  bool redundantAddrFill(G4_DstRegRegion *, G4_SrcRegRegion *,
                         unsigned execSize);

  void gatherScatterForbiddenWA();
  void preAssignSpillHeader();

public:
  void getExtraInterferenceInfo();
  GraphColor(LivenessAnalysis &live, bool hybrid, bool forceSpill_);

  const Options *getOptions() const { return m_options; }

  bool regAlloc(bool doBankConflictReduction, bool highInternalConflict,
                const RPE *rpe);
  bool requireSpillCode() const { return !spilledLRs.empty(); }
  const Interference *getIntf() const { return &intf; }
  void createLiveRanges();
  const LiveRangeVec &getLiveRanges() const { return lrs; }
  const LIVERANGE_LIST &getSpilledLiveRanges() const { return spilledLRs; }
  void confirmRegisterAssignments();
  void resetTemporaryRegisterAssignments();
  void cleanupRedundantARFFillCode();
  void getCalleeSaveRegisters();
  void addA0SaveRestoreCode();
  void addFlagSaveRestoreCode();
  void getSaveRestoreRegister();
  void getCallerSaveRegisters();
  void dumpRegisterPressure(std::ostream&);
  void dumpRPEToFile();
  GlobalRA &getGRA() { return gra; }
  G4_SrcRegRegion *getScratchSurface() const;
  unsigned int getNumVars() const { return numVar; }
  float getSpillRatio() const { return (float)spilledLRs.size() / numVar; }
  void markFailSafeIter(bool f) { failSafeIter = f; }
  void setReserveSpillGRFCount(unsigned c) { reserveSpillGRFCount = c; }
};

struct BundleConflict {
  const G4_Declare *const dcl;
  const int offset;
  BundleConflict(const G4_Declare *dcl, int offset)
      : dcl(dcl), offset(offset) {}
};

struct RAVarInfo {
  unsigned numSplit = 0;
  unsigned bb_id = UINT_MAX; // block local variable's block id.
  G4_Declare *splittedDCL = nullptr;
  LocalLiveRange *localLR = nullptr;
  LSLiveRange *LSLR = nullptr;
  unsigned numRefs = 0;
  BankConflict conflict =
      BANK_CONFLICT_NONE; // used to indicate bank that should be assigned to
                          // dcl if possible
  // Store intervals over which variable is live. Note that
  // intervals are used only for subroutine arguments. For
  // all other variables, it's expected that intervals contains
  // a single entry.
  std::vector<Interval> intervals;
  std::vector<unsigned char> mask;
  std::vector<const G4_Declare *> subDclList;
  unsigned subOff = 0;
  std::vector<BundleConflict> bundleConflicts;
  G4_SubReg_Align subAlign = G4_SubReg_Align::Any;
  int augAlignInGRF = 0;
  AugmentationMasks augMask = AugmentationMasks::Undetermined;
};

struct CustomHash {
  size_t operator()(const Interval &seg) const {
    return seg.start->getLexicalId();
  }
};

[[maybe_unused]]
static bool operator==(const Interval &first, const Interval &second) {
  return first.start->getLexicalId() == second.start->getLexicalId();
}

class VerifyAugmentation {
private:
  G4_Kernel *kernel = nullptr;
  GlobalRA *gra = nullptr;
  AllIntervals sortedLiveRanges;
  std::unordered_map<const G4_Declare *,
                     std::tuple<LiveRange *, AugmentationMasks,
                                std::unordered_set<Interval, CustomHash>>>
      masks;
  LiveRangeVec lrs;
  unsigned numVars = 0;
  const Interference *intf = nullptr;
  std::unordered_map<G4_Declare *, LiveRange *> DclLRMap;
  std::unordered_map<G4_BB *, std::string> bbLabels;
  std::vector<std::tuple<G4_BB *, unsigned, unsigned>> BBLexId;
  CALL_DECL_MAP callDclMap;

  static const char *getStr(AugmentationMasks a) {
    if (a == AugmentationMasks::Default16Bit)
      return "Default16Bit";
    else if (a == AugmentationMasks::Default32Bit)
      return "Default32Bit";
    else if (a == AugmentationMasks::Default64Bit)
      return "Default64Bit";
    else if (a == AugmentationMasks::NonDefault)
      return "NonDefault";
    else if (a == AugmentationMasks::Undetermined)
      return "Undetermined";

    return "-----";
  };
  void labelBBs();
  void populateBBLexId();
  bool interfereBetween(G4_Declare *, G4_Declare *);
  void verifyAlign(G4_Declare *dcl);
  unsigned getGRFBaseOffset(const G4_Declare *dcl) const;

public:
  void verify();
  void reset() {
    sortedLiveRanges.clear();
    masks.clear();
    kernel = nullptr;
    gra = nullptr;
    numVars = 0;
    intf = nullptr;
    DclLRMap.clear();
    bbLabels.clear();
    BBLexId.clear();
  }
  void loadAugData(AllIntervals &s, const LiveRangeVec &l, const CALL_DECL_MAP& c,
                   unsigned n, const Interference *i, GlobalRA &g);
  void dump(const char *dclName);
  bool isClobbered(LiveRange *lr, std::string &msg);
};

class PointsToAnalysis;

class ForbiddenRegs {
  IR_Builder &builder;
  std::vector<BitSet> forbiddenVec;

public:
  ForbiddenRegs(IR_Builder &b) : builder(b) {
    // Initialize forbidden bits
    forbiddenVec.resize((size_t)forbiddenKind::FBD_UNASSIGNED);
    forbiddenVec[(size_t)forbiddenKind::FBD_ADDR].resize(
        getForbiddenVectorSize(G4_ADDRESS));
    forbiddenVec[(size_t)forbiddenKind::FBD_FLAG].resize(
        getForbiddenVectorSize(G4_FLAG));
    forbiddenVec[(size_t)forbiddenKind::FBD_SCALAR].resize(
        getForbiddenVectorSize(G4_SCALAR));
  };

  unsigned getForbiddenVectorSize(G4_RegFileKind regKind) const;
  void generateReservedGRFForbidden(unsigned reserveSpillSize);
  void generateLastGRFForbidden();
  void generateEOTGRFForbidden();
  void generateEOTLastGRFForbidden();
  void generateCallerSaveGRFForbidden();
  void generateCalleeSaveGRFForbidden();

  BitSet *getForbiddenRegs(forbiddenKind type) {
    return &forbiddenVec[(size_t)type];
  }
};

class GlobalRA {
private:
  std::unordered_set<G4_INST *> EUFusionCallWAInsts;
  bool m_EUFusionCallWANeeded;
  std::unordered_set<G4_INST *> EUFusionNoMaskWAInsts;

public:
  bool EUFusionCallWANeeded() const { return m_EUFusionCallWANeeded; }
  void addEUFusionCallWAInst(G4_INST *inst);
  void removeEUFusionCallWAInst(G4_INST *inst) {
    EUFusionCallWAInsts.erase(inst);
  }
  const std::unordered_set<G4_INST *> &getEUFusionCallWAInsts() {
    return EUFusionCallWAInsts;
  }
  bool EUFusionNoMaskWANeeded() const { return builder.hasFusedEUNoMaskWA(); }
  void addEUFusionNoMaskWAInst(G4_BB *BB, G4_INST *Inst);
  void removeEUFusionNoMaskWAInst(G4_INST *Inst);
  const std::unordered_set<G4_INST *> &getEUFusionNoMaskWAInsts() {
    return EUFusionNoMaskWAInsts;
  }

public:
  std::unique_ptr<VerifyAugmentation> verifyAugmentation;
  std::unique_ptr<RegChartDump> regChart;
  std::unique_ptr<SpillAnalysis> spillAnalysis;
  static bool useGenericAugAlign(PlatformGen gen) {
    if (gen == PlatformGen::GEN9 || gen == PlatformGen::GEN8)
      return false;
    return true;
  }
  static const char StackCallStr[];
  // The pre assigned forbidden register bits for different kinds
  ForbiddenRegs fbdRegs;

  const bool use4GRFAlign = false;

  // [-2^16...2^16) in bytes
  //   (frame pointer is biased 2^16 so that -2^16 references scratch[0x0])
  static constexpr unsigned SPILL_FILL_IMMOFF_MAX = 0x10000; // 64k
  // Efficient64b is [0 ... (2^16 - 1)] but in d32 elements (not bytes)
  // That yields [0...256k) in bytes
  static constexpr unsigned SPILL_FILL_IMMOFF_MAX_EFF64b = 4 * ((1u << 16) - 1);

  static bool LSCUsesImmOff(IR_Builder &builder) {
    const auto scratchAddrType = builder.isEfficient64bEnabled()
                                     ? VISA_LSC_IMMOFF_ADDR_TYPE_SURF
                                     : VISA_LSC_IMMOFF_ADDR_TYPE_SS;
    const uint32_t immOffOpts =
        builder.getuint32Option(vISA_lscEnableImmOffsFor);
    return
        // HW supports it
        builder.getPlatform() >= Xe2 &&
        // the spill/fill is enabled in options
        (immOffOpts & (1 << VISA_LSC_IMMOFF_SPILL_FILL)) != 0 &&
        // address type is also enabled in options
        (immOffOpts & (1 << scratchAddrType)) != 0;
  }

private:
  template <class REGION_TYPE>
  static unsigned getRegionDisp(REGION_TYPE *region, const IR_Builder &irb);
  unsigned getRegionByteSize(G4_DstRegRegion *region, unsigned execSize);
  static bool owordAligned(unsigned offset) { return offset % 16 == 0; }
  template <class REGION_TYPE>
  bool isUnalignedRegion(REGION_TYPE *region, unsigned execSize);
  bool shouldPreloadDst(G4_INST *instContext, G4_BB *curBB);
  bool livenessCandidate(const G4_Declare *decl) const;
  void updateDefSet(std::set<G4_Declare *> &defs, G4_Declare *referencedDcl);
  void detectUndefinedUses(LivenessAnalysis &liveAnalysis, G4_Kernel &kernel);
  void markBlockLocalVar(G4_RegVar *var, unsigned bbId);
  void markBlockLocalVars();
  void fixAlignment();
  // Updates `slot1SetR0` and `slot1ResetR0` with hword spill/fill instructions
  // that need to update r0.5 to address slot 1 scratch space.
  void markSlot1HwordSpillFill(G4_BB *);
  void expandSpillIntrinsic(G4_BB *);
  void expandFillIntrinsic(G4_BB *);
  void expandSpillFillIntrinsics(unsigned);
  void expandSpillFillIntrinsicsXE3P(unsigned int);
  void saveRestoreA0(G4_BB *);
  void initAddrRegForImmOffUseNonStackCall();
  void initAddrRegForImmOffUseEfficient64bNonStackCall();
  static const RAVarInfo defaultValues;
  std::vector<RAVarInfo> vars;
  std::vector<G4_Declare *> UndeclaredVars;
  std::vector<G4_Declare *> UndefinedCmpVars;

  // fake declares for each GRF reg, used by HRA
  // note only GRFs that are used by LRA get a declare
  std::vector<G4_Declare *> GRFDclsForHRA;

  // Store all LocalLiveRange instances created so they're
  // appropriately destroyed alongwith instance of GlobalRA.
  // This needs to be a list because we'll take address of
  // its elements and std::vector cannot be used due to its
  // reallocation policy.
  std::list<LocalLiveRange> localLiveRanges;

  std::unordered_map<const G4_BB *, unsigned> subretloc;
  // map ret location to declare for call/ret
  std::map<uint32_t, G4_Declare *> retDecls;

  // store instructions that shouldnt be rematerialized.
  std::unordered_set<G4_INST *> dontRemat;

  // map each BB to its local RA GRF usage summary, populated in local RA.
  std::map<G4_BB *, PhyRegSummary *> bbLocalRAMap;
  llvm::SpecificBumpPtrAllocator<PhyRegSummary> PRSAlloc;

  RAVarInfo &allocVar(const G4_Declare *dcl) {
    auto dclid = dcl->getDeclId();
    if (dclid >= vars.size())
      vars.resize(dclid + 1);
    return vars[dclid];
  }

  const RAVarInfo &getVar(const G4_Declare *dcl) const {
    // It's assumed that dcl has already been added to vars vector. To add newly
    // created RA variables to the vector pre-RA, addVarToRA() can be used.
    auto dclid = dcl->getDeclId();
    return vars[dclid];
  }

  // temp variable storing the FP dcl's old value
  // created in addStoreRestoreForFP
  G4_Declare *oldFPDcl = nullptr;

  // instruction to save/restore vISA FP, only present in functions
  G4_INST *saveBE_FPInst = nullptr;
  G4_INST *restoreBE_FPInst = nullptr;

  // instruction go update BE_FP, BE_SP, only present in functions
  G4_INST *setupBE_FP = nullptr;
  G4_INST *setupBE_SP = nullptr;

  // new temps for each reference of spilled address/flag decls
  std::unordered_set<G4_Declare *> addrFlagSpillDcls;

  // track spill/fill code in basic blocks
  std::unordered_set<G4_BB *> BBsWithSpillCode;

  // store iteration number for GRA loop
  unsigned iterNo = 0;

  uint32_t numGRFSpill = 0;
  uint32_t numGRFFill = 0;
  bool canUseLscImmediateOffsetSpillFill = false;

  unsigned int numReservedGRFsFailSafe = BoundedRA::NOT_FOUND;

  // For hword scratch messages, when using separate scratch space for spills,
  // r0.5 needs to be updated before spill/fill to point to slot 1 space.
  // These maps mark which spills/fills need to set/reset r0.5.
  std::unordered_set<G4_INST *> slot1SetR0;
  std::unordered_set<G4_INST *> slot1ResetR0;

  void insertSlot1HwordR0Set(G4_BB *bb, INST_LIST_ITER &instIt);
  void insertSlot1HwordR0Reset(G4_BB *bb, INST_LIST_ITER &instIt);

  bool spillFillIntrinUsesLSC(G4_INST *spillFillIntrin);
  void expandFillLSC(G4_BB *bb, INST_LIST_ITER &instIt);
  void expandSpillLSC(G4_BB *bb, INST_LIST_ITER &instIt);
private:
  void expandFillLscEff64(G4_BB *bb, INST_LIST_ITER &instIt);
  void expandSpillLscEff64(G4_BB *bb, INST_LIST_ITER &instIt);

public:
  void expandScatterSpillLSC(G4_BB *bb, INST_LIST_ITER &instIt);
  void expandFillNonStackcall(uint32_t numRows, uint32_t offset,
                              short rowOffset, G4_SrcRegRegion *header,
                              G4_DstRegRegion *resultRgn, G4_BB *bb,
                              INST_LIST_ITER &instIt);
  void expandSpillNonStackcall(uint32_t numRows, uint32_t offset,
                               short rowOffset, G4_SrcRegRegion *header,
                               G4_SrcRegRegion *payload, G4_BB *bb,
                               INST_LIST_ITER &instIt);
  void expandFillStackcall(uint32_t numRows, uint32_t offset, short rowOffset,
                           G4_SrcRegRegion *header, G4_DstRegRegion *resultRgn,
                           G4_BB *bb, INST_LIST_ITER &instIt);
  void expandSpillStackcall(uint32_t numRows, uint32_t offset, short rowOffset,
                            G4_SrcRegRegion *payload, G4_BB *bb,
                            INST_LIST_ITER &instIt);
  bool stopAfter(const char *subpass) const {
    auto passName = builder.getOptions()->getOptionCstr(vISA_StopAfterPass);
    return passName && strcmp(passName, subpass) == 0;
  }

public:
  static unsigned sendBlockSizeCode(unsigned owordSize);

  // For current program, store caller/callee save/restore instructions
  std::unordered_set<G4_INST *> calleeSaveInsts;
  std::unordered_set<G4_INST *> calleeRestoreInsts;
  std::unordered_map<G4_INST *, std::unordered_set<G4_INST *>> callerSaveInsts;
  std::unordered_map<G4_INST *, std::unordered_set<G4_INST *>>
      callerRestoreInsts;
  std::unordered_map<G4_BB *, std::vector<bool>> callerSaveRegsMap;
  std::unordered_map<G4_BB *, unsigned> callerSaveRegCountMap;
  std::unordered_map<G4_BB *, std::vector<bool>> retRegsMap;
  std::vector<bool> calleeSaveRegs;
  unsigned calleeSaveRegCount = 0;

  std::unordered_map<G4_Declare *, SplitResults> splitResults;

  G4_Kernel &kernel;
  IR_Builder &builder;
  PhyRegPool &regPool;
  PointsToAnalysis &pointsToAnalysis;
  FCALL_RET_MAP fcallRetMap;

  const bool useLscForSpillFill;
  const bool useLscForScatterSpill;
  const bool useLscForNonStackCallSpillFill;
  bool useFastRA = false;
  bool useHybridRAwithSpill = false;
  bool useLocalRA = false;
  bool forceBCR = false;
  bool twoSrcBundleBCR = false;
  uint32_t nextSpillOffset = 0;
  uint32_t scratchOffset = 0;

  InterferenceMatrixStorage intfStorage;
  IncrementalRA incRA;

  bool avoidBundleConflict = false;

  unsigned getSubRetLoc(const G4_BB *bb) {
    auto it = subretloc.find(bb);
    if (it == subretloc.end())
      return UNDEFINED_VAL;
    return it->second;
  }

  void setSubRetLoc(const G4_BB *bb, unsigned s) { subretloc[bb] = s; }

  const G4_Declare *getRetDcl(unsigned int retLoc) const {
    auto it = retDecls.find(retLoc);
    if (it == retDecls.end())
        return nullptr;
    return it->second;
  }

  bool isSubRetLocConflict(G4_BB *bb, std::vector<unsigned> &usedLoc,
                           unsigned stackTop);
  void assignLocForReturnAddr();
  unsigned determineReturnAddrLoc(unsigned entryId,
                                  std::vector<unsigned> &retLoc, G4_BB *bb);
  void insertCallReturnVar();
  void insertSaveAddr(G4_BB *);
  void insertRestoreAddr(G4_BB *);
  void setIterNo(unsigned i) { iterNo = i; }
  unsigned getIterNo() const { return iterNo; }
  void fixSrc0IndirFcall();

  G4_Declare *getRetDecl(uint32_t retLoc) {
    auto result = retDecls.find(retLoc);
    if (result != retDecls.end()) {
      return result->second;
    }

    const char *name = builder.getNameString(24, "RET__loc%d", retLoc);
    G4_Declare *dcl = builder.createDeclare(
        name, G4_GRF, builder.getCallRetOpndSize(), 1, Type_UD);

    // call destination must still be QWord aligned
    auto callDstAlign = Four_Word;
    if (builder.isEfficient64bEnabled())
      callDstAlign = Eight_Word;
    dcl->setSubRegAlign(callDstAlign);
    setSubRegAlign(dcl, callDstAlign);

    retDecls[retLoc] = dcl;
    return dcl;
  }

  G4_INST *getSaveBE_FPInst() const { return saveBE_FPInst; };
  G4_INST *getRestoreBE_FPInst() const { return restoreBE_FPInst; };

  static unsigned owordToGRFSize(unsigned numOwords, const IR_Builder &builder);
  static unsigned hwordToGRFSize(unsigned numHwords, const IR_Builder &builder);
  static unsigned GRFToHwordSize(unsigned numGRFs, const IR_Builder &builder);
  static unsigned GRFSizeToOwords(unsigned numGRFs, const IR_Builder &builder);
  static unsigned getHWordByteSize();

  // RA specific fields
  G4_Declare *getGRFDclForHRA(int GRFNum) const {
    return GRFDclsForHRA[GRFNum];
  }

  G4_Declare *getOldFPDcl() const { return oldFPDcl; }

  bool isAddrFlagSpillDcl(G4_Declare *dcl) const {
    return addrFlagSpillDcls.count(dcl) != 0;
  }

  void addAddrFlagSpillDcl(G4_Declare *dcl) { addrFlagSpillDcls.insert(dcl); }

  bool hasSpillCodeInBB(G4_BB *bb) const {
    return BBsWithSpillCode.find(bb) != BBsWithSpillCode.end();
  }

  void addSpillCodeInBB(G4_BB *bb) { BBsWithSpillCode.insert(bb); }

  void addUndefinedDcl(G4_Declare *dcl) { UndeclaredVars.push_back(dcl); }
  void addUndefinedCmpDcl(G4_Declare *dcl) { UndefinedCmpVars.push_back(dcl); }

  bool isUndefinedDcl(const G4_Declare *dcl) const {
    return std::find(UndeclaredVars.begin(), UndeclaredVars.end(), dcl) !=
           UndeclaredVars.end();
  }

  RAVarInfo &addVarToRA(const G4_Declare *dcl) { return allocVar(dcl); }

  unsigned getSplitVarNum(const G4_Declare *dcl) const {
    return getVar(dcl).numSplit;
  }

  void setSplitVarNum(const G4_Declare *dcl, unsigned val) {
    allocVar(dcl).numSplit = val;
  }

  unsigned getBBId(const G4_Declare *dcl) const { return getVar(dcl).bb_id; }

  void setBBId(const G4_Declare *dcl, unsigned id) { allocVar(dcl).bb_id = id; }

  bool isBlockLocal(const G4_Declare *dcl) const {
    return getBBId(dcl) < (UINT_MAX - 1);
  }

  G4_Declare *getSplittedDeclare(const G4_Declare *dcl) const {
    return getVar(dcl).splittedDCL;
  }

  void setSplittedDeclare(const G4_Declare *dcl, G4_Declare *sd) {
    allocVar(dcl).splittedDCL = sd;
  }

  LocalLiveRange *getLocalLR(const G4_Declare *dcl) const {
    return getVar(dcl).localLR;
  }

  void setLocalLR(G4_Declare *dcl, LocalLiveRange *lr) {
    RAVarInfo &var = allocVar(dcl);
    vISA_ASSERT(var.localLR == NULL,
                "Local live range already allocated for declaration");
    var.localLR = lr;
    lr->setTopDcl(dcl);
  }

  LSLiveRange *getSafeLSLR(const G4_Declare *dcl) const {
    auto dclid = dcl->getDeclId();
    if (dclid < vars.size()) {
      return vars[dclid].LSLR;
    } else {
      return nullptr;
    }
  }

  LSLiveRange *getLSLR(const G4_Declare *dcl) const { return getVar(dcl).LSLR; }

  void setLSLR(G4_Declare *dcl, LSLiveRange *lr) {
    RAVarInfo &var = allocVar(dcl);
    vISA_ASSERT(var.LSLR == NULL,
                "Local live range already allocated for declaration");
    var.LSLR = lr;
    lr->setTopDcl(dcl);
  }

  void resetLSLR(const G4_Declare *dcl) { allocVar(dcl).LSLR = nullptr; }

  void resetLocalLR(const G4_Declare *dcl) { allocVar(dcl).localLR = nullptr; }

  void clearStaleLiveRanges() {
    for (auto dcl : kernel.Declares) {
      setBBId(dcl, UINT_MAX);
      resetLocalLR(dcl);
    }
    UndefinedCmpVars.clear();
  }

  void clearLocalLiveRanges() {
    for (auto dcl : kernel.Declares) {
      resetLocalLR(dcl);
    }
  }

  void recordRef(const G4_Declare *dcl) { allocVar(dcl).numRefs++; }

  unsigned getNumRefs(const G4_Declare *dcl) const {
    return getVar(dcl).numRefs;
  }

  void setNumRefs(const G4_Declare *dcl, unsigned refs) {
    allocVar(dcl).numRefs = refs;
  }

  BankConflict getBankConflict(const G4_Declare *dcl) const {
    return getVar(dcl).conflict;
  }

  void setBankConflict(const G4_Declare *dcl, BankConflict c) {
    allocVar(dcl).conflict = c;
  }

  G4_INST *getStartInterval(const G4_Declare *dcl) const {
    return getLastStartInterval(dcl);
  }

  void setStartInterval(const G4_Declare *dcl, G4_INST *inst) {
    setLastStartInterval(dcl, inst);
  }

  G4_INST *getEndInterval(const G4_Declare *dcl) const {
    return getLastEndInterval(dcl);
  }

  void setEndInterval(const G4_Declare *dcl, G4_INST *inst) {
    setLastEndInterval(dcl, inst);
  }

  void clearIntervals(G4_Declare *dcl) {
    auto &intervals = allocVar(dcl).intervals;
    intervals.clear();
  }

  // Used to create new interval for subroutine arg/retval
  void pushBackNewInterval(const G4_Declare* dcl) {
    auto &intervals = allocVar(dcl).intervals;
    intervals.push_back({nullptr, nullptr});
  }

  const Interval getInterval(const G4_Declare *dcl, unsigned int i) const {
    if (i < getNumIntervals(dcl)) {
      return getAllIntervals(dcl)[i];
    }
    vISA_ASSERT(false, "OOB access");
    return {};
  }

  unsigned int getNumIntervals(const G4_Declare *dcl) const {
    return getAllIntervals(dcl).size();
  }

  const G4_INST *getIntervalStart(const Interval &interval) const {
    return interval.start;
  }

  const G4_INST *getIntervalEnd(const Interval &interval) const {
    return interval.end;
  }

  const std::vector<Interval>& getAllIntervals(const G4_Declare *dcl) const {
    return getVar(dcl).intervals;
  }

  G4_INST *getLastStartInterval(const G4_Declare *dcl) const {
    auto& intervals = getVar(dcl).intervals;
    if (intervals.empty())
      return nullptr;
    return intervals.back().start;
  }

  void setLastStartInterval(const G4_Declare *dcl, G4_INST *inst) {
    auto &intervals = allocVar(dcl).intervals;
    if(intervals.empty())
      intervals.resize(1);
    intervals.back().start = inst;
  }

  G4_INST *getLastEndInterval(const G4_Declare *dcl) const {
    auto &intervals = getVar(dcl).intervals;
    if (intervals.empty())
      return nullptr;
    return intervals.back().end;
  }

  void setLastEndInterval(const G4_Declare *dcl, G4_INST *inst) {
    auto &intervals = allocVar(dcl).intervals;
    if(intervals.empty())
      intervals.resize(1);
    intervals.back().end = inst;
  }

  const std::vector<unsigned char> &getMask(const G4_Declare *dcl) const {
    return getVar(dcl).mask;
  }

  void setMask(const G4_Declare *dcl, const std::vector<unsigned char>& m) {
    allocVar(dcl).mask = m;
  }

  AugmentationMasks getAugmentationMask(const G4_Declare *dcl) const {
    return getVar(dcl).augMask;
  }

  void setAugmentationMask(const G4_Declare *dcl, AugmentationMasks m) {
    allocVar(dcl).augMask = m;
    if (dcl->getIsSplittedDcl()) {
      for (const G4_Declare *subDcl : getSubDclList(dcl)) {
        setAugmentationMask(subDcl, m);
      }
    }
  }

  bool getHasNonDefaultMaskDef(const G4_Declare *dcl) const {
    return (getAugmentationMask(dcl) == AugmentationMasks::NonDefault);
  }

  void addBundleConflictDcl(const G4_Declare *dcl, const G4_Declare *subDcl,
                            int offset) {
    allocVar(dcl).bundleConflicts.emplace_back(subDcl, offset);
  }

  void clearBundleConflictDcl(const G4_Declare *dcl) {
    allocVar(dcl).bundleConflicts.clear();
  }

  const std::vector<BundleConflict> &
  getBundleConflicts(const G4_Declare *dcl) const {
    return getVar(dcl).bundleConflicts;
  }

  unsigned get_bundle(unsigned baseReg, int offset) const {
    if (builder.has64bundleSize2GRFPerBank()) {
      return (((baseReg + offset) % 32) / 4);
    }
    if (builder.hasPartialInt64Support()) {
      return (((baseReg + offset) % 32) / 2);
    }
    if (builder.has64bundleSize()) {
      if (builder.kernel.getNumRegTotal() == 512) {
        return (((baseReg + offset) % 32) / 2);
      }
      return (((baseReg + offset) % 16) / 2);
    }
    return (((baseReg + offset) % 64) / 4);
  }

  unsigned get_bank(unsigned baseReg, int offset) {
    int bankID = (baseReg + offset) % 2;

    if (builder.hasTwoGRFBank16Bundles()) {
      bankID = ((baseReg + offset) % 4) / 2;
    }

    if (builder.has64bundleSize2GRFPerBank()) {
      bankID = ((baseReg + offset) % 4) / 2;
    }

    if (builder.hasOneGRFBank16Bundles()) {
      bankID = (baseReg + offset) % 2;
    }

    if (builder.has64bundleSize()) {
      bankID = (baseReg + offset) % 2;
    }
    return bankID;
  }

  void addSubDcl(const G4_Declare *dcl, G4_Declare *subDcl) {
    allocVar(dcl).subDclList.push_back(subDcl);
  }

  void clearSubDcl(const G4_Declare *dcl) { allocVar(dcl).subDclList.clear(); }

  const std::vector<const G4_Declare *> &
  getSubDclList(const G4_Declare *dcl) const {
    return getVar(dcl).subDclList;
  }

  unsigned getSubOffset(const G4_Declare *dcl) const {
    return getVar(dcl).subOff;
  }

  void setSubOffset(const G4_Declare *dcl, unsigned offset) {
    allocVar(dcl).subOff = offset;
  }

  G4_SubReg_Align getSubRegAlign(const G4_Declare *dcl) const {
    return getVar(dcl).subAlign;
  }

  void setSubRegAlign(const G4_Declare *dcl, G4_SubReg_Align subAlg) {
    auto &subAlign = allocVar(dcl).subAlign;
    // sub reg alignment can only be more restricted than prior setting
    vISA_ASSERT(subAlign == Any || subAlign == subAlg || subAlign % 2 == 0,
                ERROR_UNKNOWN);
    if (subAlign > subAlg) {
      vISA_ASSERT(subAlign % subAlg == 0, "Sub reg alignment conflict");
      // do nothing; keep the original alignment (more restricted)
    } else {
      vISA_ASSERT(subAlg % subAlign == 0, "Sub reg alignment conflict");
      subAlign = subAlg;
    }
  }

  bool hasAlignSetup(const G4_Declare *dcl) const {
    if (getVar(dcl).subAlign == G4_SubReg_Align::Any &&
        dcl->getSubRegAlign() != G4_SubReg_Align::Any)
      return false;
    return true;
  }

  bool isQuadAligned(const G4_Declare *dcl) const {
    auto augAlign = getAugAlign(dcl);
    return augAlign == 4;
  }

  template <bool Support4GRFAlign = true>
  bool isEvenAligned(const G4_Declare* dcl) const {
    auto augAlign = getAugAlign(dcl);
    if constexpr (Support4GRFAlign)
      return augAlign > 0 && augAlign % 2 == 0;
    else
      return (augAlign > 0);
  }

  int getAugAlign(const G4_Declare *dcl) const {
    return getVar(dcl).augAlignInGRF;
  }

  void forceQuadAlign(const G4_Declare *dcl) { setAugAlign(dcl, 4); }

  void resetAlign(const G4_Declare *dcl) { setAugAlign(dcl, 0); }

  // Due to legacy usage, this method takes a boolean that, when set,
  // causes alignment to be set to Even (2). When boolean flag is
  // reset, it also resets alignment to Either (0).
  void setEvenAligned(const G4_Declare *dcl, bool align) {
    setAugAlign(dcl, align ? 2 : 0);
  }

  void setAugAlign(const G4_Declare *dcl, int align) {
    vISA_ASSERT(align <= 2 || use4GRFAlign, "unexpected alignment");
    vISA_ASSERT(align <= 4, "unsupported alignment");
    allocVar(dcl).augAlignInGRF = align;
  }

  BankAlign getBankAlign(const G4_Declare *) const;
  bool areAllDefsNoMask(G4_Declare *);
  void removeUnreferencedDcls();
  LocalLiveRange *GetOrCreateLocalLiveRange(G4_Declare *topdcl);

  GlobalRA(G4_Kernel& k, PhyRegPool& r, PointsToAnalysis& p2a)
    : fbdRegs(*k.fg.builder),
      use4GRFAlign(k.fg.builder->supports4GRFAlign()),
      kernel(k), builder(*k.fg.builder), regPool(r),
      pointsToAnalysis(p2a),
      useLscForSpillFill(k.fg.builder->supportsLSC()),
      useLscForScatterSpill(k.fg.builder->supportsLSC() &&
          k.fg.builder->getOption(vISA_scatterSpill)),
      useLscForNonStackCallSpillFill(
          k.fg.builder->useLscForNonStackSpillFill()),
      incRA(*this)
  {
    vars.resize(k.Declares.size());

    if (kernel.getOptions()->getOption(vISA_VerifyAugmentation)) {
      verifyAugmentation = std::make_unique<VerifyAugmentation>();
    }
    forceBCR = kernel.getOption(vISA_forceBCR);
    twoSrcBundleBCR = kernel.getOption(vISA_twoSrcBundleBCR);
    // Set callWA condition.
    //    Call return ip and mask need wa only for non-entry functions. As call
    //    WA also needs a temp, we conservatively add WA for
    //    caller-save/callee-save code too, which applies to all functions,
    //    including the entry function.
    m_EUFusionCallWANeeded =
        builder.hasFusedEU() &&
        builder.getuint32Option(vISA_fusedCallWA) == 1 &&
        (kernel.fg.getHasStackCalls() || kernel.hasIndirectCall());
  }

  void emitFGWithLiveness(const LivenessAnalysis &liveAnalysis) const;
  void reportSpillInfo(const LivenessAnalysis &liveness,
                       const GraphColor &coloring) const;
  static uint32_t getRefCount(int loopNestLevel);
  bool canIncreaseGRF(unsigned spillSize, bool infCostSpilled);
  void updateSubRegAlignment(G4_SubReg_Align subAlign);
  bool isChannelSliced();
  // Used by LRA/GRA/hybrid RA
  void augAlign();
  int getAlignFromAugBucket(G4_Declare *);
  void getBankAlignment(LiveRange *lr, BankAlign &align);
  void printLiveIntervals();
  void reportUndefinedUses(LivenessAnalysis &liveAnalysis, G4_BB *bb,
                           G4_INST *inst, G4_Declare *referencedDcl,
                           std::set<G4_Declare *> &defs,
                           Gen4_Operand_Number opndNum);
  void detectNeverDefinedUses();

  void determineSpillRegSize(unsigned &spillRegSize,
                             unsigned &indrSpillRegSize);
  G4_Imm *createMsgDesc(unsigned owordSize, bool writeType, bool isSplitSend);
  void stackCallProlog();
  void saveRegs(unsigned startReg, unsigned owordSize,
                G4_Declare *scratchRegDcl, G4_Declare *framePtr,
                unsigned frameOwordOffset, G4_BB *bb, INST_LIST_ITER insertIt,
                std::unordered_set<G4_INST *> &group);
  void saveActiveRegs(std::vector<bool> &saveRegs, unsigned startReg,
                      unsigned frameOffset, G4_BB *bb, INST_LIST_ITER insertIt,
                      std::unordered_set<G4_INST *> &group);
  void addrRegAlloc();
  void flagRegAlloc();
  void scalarRegAlloc();
  void selectScalarCandidates();
  void fastRADecision();
  bool tryHybridRA();
  bool hybridRA(LocalRA &lra);
  void assignRegForAliasDcl();
  void removeSplitDecl();
  std::pair<unsigned, unsigned> reserveGRFSpillReg(GraphColor &coloring);
  void generateForbiddenTemplates(unsigned reserveSpillSize);
  void createVariablesForHybridRAWithSpill();

  BitSet *getForbiddenRegs(forbiddenKind type) {
    return fbdRegs.getForbiddenRegs(type);
  }

  unsigned getForbiddenVectorSize(G4_RegFileKind regKind) {
    return fbdRegs.getForbiddenVectorSize(regKind);
  }

  int coloringRegAlloc();
  void restoreRegs(unsigned startReg, unsigned owordSize,
                   G4_Declare *scratchRegDcl, G4_Declare *framePtr,
                   unsigned frameOwordOffset, G4_BB *bb,
                   INST_LIST_ITER insertIt,
                   std::unordered_set<G4_INST *> &group, bool caller);
  void restoreActiveRegs(std::vector<bool> &restoreRegs, unsigned startReg,
                         unsigned frameOffset, G4_BB *bb,
                         INST_LIST_ITER insertIt,
                         std::unordered_set<G4_INST *> &group, bool caller);
  void OptimizeActiveRegsFootprint(std::vector<bool> &saveRegs);
  void OptimizeActiveRegsFootprint(std::vector<bool> &saveRegs,
                                   std::vector<bool> &retRegs);
  void addCallerSaveRestoreCode();
  void addCalleeSaveRestoreCode();
  void addGenxMainStackSetupCode();
  void addCalleeStackSetupCode();
  void addSaveRestoreCode(unsigned localSpillAreaOwordSize);
  void addCallerSavePseudoCode();
  void addCalleeSavePseudoCode();
  void addStoreRestoreToReturn();
  void storeCEInProlog();
  void setUndefinedVarCmp();
  void markGraphBlockLocalVars();
  void verifyRA(LivenessAnalysis &liveAnalysis);
  void verifySpillFill();
  void resetGlobalRAStates();
  bool canSkipFDE() const;
  void initSRAsScratch() const;

  void insertPhyRegDecls();

  void copyMissingAlignment() {
    // Insert alignment for vars created in RA
    for (auto dcl : kernel.Declares) {
      if (dcl->getAliasDeclare())
        continue;

      if (dcl->getDeclId() >= vars.size()) {
        allocVar(dcl);
      }
      if (!hasAlignSetup(dcl)) {
        // Var may be temp created in RA
        setSubRegAlign(dcl, dcl->getSubRegAlign());
        setEvenAligned(dcl, dcl->isEvenAlign());
      }
    }
  }

  void copyAlignment(G4_Declare *dst, G4_Declare *src) {
    setAugAlign(dst, getAugAlign(src));
    setSubRegAlign(dst, getSubRegAlign(src));
  }

  void copyAlignment() {
    for (auto dcl : kernel.Declares) {
      if (dcl->getAliasDeclare())
        continue;

      setSubRegAlign(dcl, dcl->getSubRegAlign());
      setEvenAligned(dcl, dcl->isEvenAlign());
    }
  }

  bool isNoRemat(G4_INST *inst) {
    return dontRemat.find(inst) != dontRemat.end();
  }

  void addNoRemat(G4_INST *inst) { dontRemat.insert(inst); }

  unsigned int getNumReservedGRFs() {
    // Return # GRFs reserved for new fail safe mechanism
    // 1. If fail safe mechanism is invoked before coloring then
    //    # reserved GRFs is updated explicitly before this method
    //    is invoked.
    // 2. If a regular (ie, non-fail safe) RA iteration spill
    //    very little then we may convert it to fail safe but with
    //    0 reserved GRFs as it it too late to reserve a GRF after
    //    coloring.
    if (numReservedGRFsFailSafe == BoundedRA::NOT_FOUND)
      numReservedGRFsFailSafe =
          kernel.getSimdSize() == kernel.numEltPerGRF<Type_UD>() ? 1 : 2;

    return numReservedGRFsFailSafe;
  }

  void setNumReservedGRFsFailSafe(unsigned int num) {
    numReservedGRFsFailSafe = num;
  }

  PhyRegSummary *createPhyRegSummary() {
    auto PRSMem = PRSAlloc.Allocate();
    return new (PRSMem) PhyRegSummary(&builder, kernel.getNumRegTotal());
  }

  void addBBLRASummary(G4_BB *bb, PhyRegSummary *summary) {
    bbLocalRAMap.insert(std::make_pair(bb, summary));
  }

  void clearBBLRASummaries() { bbLocalRAMap.clear(); }

  PhyRegSummary *getBBLRASummary(G4_BB *bb) const {
    auto &&iter = bbLocalRAMap.find(bb);
    return iter != bbLocalRAMap.end() ? iter->second : nullptr;
  }

  unsigned computeSpillSize(const LIVERANGE_LIST &spilledLRs);
  unsigned computeSpillSize(std::list<LSLiveRange *> &spilledLRs);
  bool spillSpaceCompression(int spillSize,
                             const int globalScratchOffset);
  bool kernelUsesDpas() const;

public:
  // Store new variables created when inserting scalar imm
  // spill/fill code. Such variables are not infinite spill
  // cost. So if the variable spills again, we shouldn't
  // get in an infinite loop by retrying same spill/fill.
  std::unordered_set<G4_Declare *> scalarSpills;

private:
  // Following methods are invoked from coloringRegAlloc() method. Any
  // operation needed in global graph coloring loop should be extracted to a
  // method instead of inlining logic in code directly in interest of keeping
  // the loop maintainable.
  void stackCallSaveRestore(bool hasStackCall);
  int doGlobalLinearScanRA();
  void incRABookKeeping();
  // return pair <whether remat modified program, rematDone>
  std::pair<bool, bool> remat(bool fastCompile, bool rematDone,
                              LivenessAnalysis &liveAnalysis,
                              GraphColor &coloring, RPE &rpe);
  // rerun GRA, alignedScalarSplitDone, isEarlyExit
  std::tuple<bool, bool, bool> alignedScalarSplit(bool fastCompile,
                                                  bool alignedScalarSplitDone,
                                                  GraphColor &coloring);
  bool globalSplit(VarSplit &splitPass, GraphColor &coloring);
  int localSplit(bool fastCompile, VarSplit &splitPass);
  // return <doBCReduction, highInternalConflict>
  std::pair<bool, bool> bankConflict();
  // return reserveSpillReg
  bool setupFailSafeIfNeeded(bool fastCompile, bool hasStackCall,
                             unsigned int maxRAIterations,
                             unsigned int failSafeRAIteration);
  void undefinedUses(bool rematDone, LivenessAnalysis &liveAnalysis);
  void writeVerboseStatsNumVars(LivenessAnalysis &liveAnalysis,
                                FINALIZER_INFO *jitInfo);
  void writeVerboseRPEStats(RPE &rpe);
  bool VRTIncreasedGRF(GraphColor &coloring);
  bool canVRTIncreasedGRF(GraphColor &coloring);
  void splitOnSpill(bool fastCompile, GraphColor &coloring,
                    LivenessAnalysis &livenessAnalysis);
  bool convertToFailSafe(bool reserveSpillReg, GraphColor &coloring,
                         LivenessAnalysis &liveAnalysis,
                         unsigned int nextSpillOffset);
  unsigned int instCount() const {
    unsigned int instNum = 0;
    for (auto bb : kernel.fg) {
      instNum += (int)bb->size();
    }
    return instNum;
  }
  // tuple of abort, updated GRFSpillFillCount, updated spillSize
  std::tuple<bool, unsigned int, unsigned int>
  abortOnSpill(unsigned int GRFSpillFillCount, unsigned int spillSize,
               GraphColor &coloring);
  void verifyNoInfCostSpill(GraphColor &coloring, bool reserveSpillReg);
  void setupA0Dot2OnSpill(bool hasStackCall, unsigned int nextSpillOffset,
                          int globalScratchOffset);
  // isEarlyExit
  bool spillCleanup(bool fastCompile, bool useScratchMsgForSpill,
                    bool hasStackCall, bool reserveSpillReg, RPE &rpe,
                    GraphColor &coloring, LivenessAnalysis &liveAnalysis,
                    SpillManagerGRF &spillGRF);

  // success, enableSpillSpaceCompression, isEarlyExit, scratchOffset,
  // nextSpillOffset
  std::tuple<bool, bool, bool, unsigned int, unsigned int>
  insertSpillCode(bool enableSpillSpaceCompression, GraphColor &coloring,
                  LivenessAnalysis &liveAnalysis, RPE &rpe,
                  unsigned int scratchOffset, bool fastCompile,
                  bool hasStackCall, int globalScratchOffset,
                  unsigned int nextSpillOffset, bool reserveSpillReg,
                  unsigned int spillRegSize, unsigned int indrSpillRegSize,
                  bool useScratchMsgForSpill);
  bool rerunGRAIter(bool rerunGRA);
};

inline G4_Declare *Interference::getGRFDclForHRA(int GRFNum) const {
  return gra.getGRFDclForHRA(GRFNum);
}

class VarSplit {
private:
  G4_Kernel &kernel;
  GlobalRA &gra;

  VarRange *splitVarRange(VarRange *src1, VarRange *src2,
                          std::stack<VarRange *> *toDelete);
  void rangeListSpliting(VAR_RANGE_LIST *rangeList, G4_Operand *opnd,
                         std::stack<VarRange *> *toDelete);
  void getHeightWidth(G4_Type type, unsigned numberElements,
                      unsigned short &dclWidth, unsigned short &dclHeight,
                      int &totalByteSize) const;
  void createSubDcls(G4_Kernel &kernel, G4_Declare *oldDcl,
                     std::vector<G4_Declare *> &splitDclList);
  void insertMovesToTemp(IR_Builder &builder, G4_Declare *oldDcl,
                         G4_Operand *dstOpnd, G4_BB *bb,
                         INST_LIST_ITER instIter,
                         std::vector<G4_Declare *> &splitDclList);
  void insertMovesFromTemp(G4_Kernel &kernel, G4_Declare *oldDcl, int index,
                           G4_Operand *srcOpnd, int pos, G4_BB *bb,
                           INST_LIST_ITER instIter,
                           std::vector<G4_Declare *> &splitDclList);

public:
  bool didLocalSplit = false;
  bool didGlobalSplit = false;

  int localSplit(IR_Builder &builder, G4_BB *bb);
  void globalSplit(IR_Builder &builder, G4_Kernel &kernel);
  bool canDoGlobalSplit(IR_Builder &builder, G4_Kernel &kernel,
                        uint32_t sendSpillRefCount);

  VarSplit(GlobalRA &g) : kernel(g.kernel), gra(g) {}
};

class DynPerfModel {
private:
  std::string Buffer;

public:
  G4_Kernel &Kernel;
  unsigned int NumSpills = 0;
  unsigned int NumFills = 0;
  unsigned int NumRAIters = 0;
  unsigned long long TotalDynInst = 0;
  unsigned long long FillDynInst = 0;
  unsigned long long SpillDynInst = 0;
  // vector item at index i corresponds to nesting level i
  // #Loops at this nesting level, #Spills, #Fills
  std::vector<std::tuple<unsigned int, unsigned int, unsigned int>>
      SpillFillPerNestingLevel;

  DynPerfModel(G4_Kernel &K) : Kernel(K) {}

  void run();
  void dump();
};

// Class used to analyze spill/fill decisions
class SpillAnalysis {
public:
  SpillAnalysis() = default;
  ~SpillAnalysis();
  SpillAnalysis(const SpillAnalysis &) = delete;
  SpillAnalysis &operator=(const SpillAnalysis &) = delete;

  void Dump(std::ostream &OS = std::cerr);
  void DumpHistogram(std::ostream &OS = std::cerr);

  unsigned int GetDistance(G4_Declare *Dcl);
  void LoadAugIntervals(AllIntervals &, GlobalRA &);
  void LoadDegree(G4_Declare *Dcl, unsigned int degree);

  void SetLivenessAnalysis(LivenessAnalysis *L) { LA = L; }
  void SetGraphColor(GraphColor *C) { GC = C; }
  void SetSpillManager(SpillManagerGRF *S) { SM = S; }

  void Clear();

  void Do(LivenessAnalysis *L, GraphColor *C, SpillManagerGRF *S);

private:
  VarReferences *Refs = nullptr;

  LivenessAnalysis *LA = nullptr;
  GraphColor *GC = nullptr;
  SpillManagerGRF *SM = nullptr;

  std::unordered_map<G4_Declare *, std::pair<G4_INST *, G4_INST *>>
      AugIntervals;
  std::unordered_map<G4_Declare *, unsigned int> DclDegree;

  std::vector<G4_BB *> GetLiveBBs(G4_Declare *,
                                  std::unordered_map<G4_INST *, G4_BB *> &);
  std::vector<G4_BB *>
  GetIntervalBBs(G4_INST *Start, G4_INST *End,
                 std::unordered_map<G4_INST *, G4_BB *> &InstBBMap);
};
} // namespace vISA

#endif // __GRAPHCOLOR_H__
