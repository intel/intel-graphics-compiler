/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef DEBUG_INFO_H
#define DEBUG_INFO_H
#include "BuildCISAIR.h"
#include "Common_BinaryEncoding.h"
#include "FlowGraph.h"
#include "GraphColor.h"
#include "Mem_Manager.h"

#include "llvm/Support/Allocator.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace vISA {
class G4_Declare;
class LiveRange;
}
class CISA_IR_Builder;
class VISAKernelImpl;

struct VarnameMap;
namespace vISA {
class DebugInfoState;
}

int decodeAndDumpDebugInfo(char *filename, TARGET_PLATFORM platform);
void emitDebugInfo(VISAKernelImpl *kernel,
                   CISA_IR_Builder::KernelListTy &functions,
                   std::string filename);
void emitDebugInfoToMem(VISAKernelImpl *kernel,
                        CISA_IR_Builder::KernelListTy &functions, void *&info,
                        unsigned &size);

struct IDX_VDbgCisaByte2Gen {
  unsigned CisaByteOffset;
  unsigned GenOffset;
};
struct IDX_VDbgCisaIndex2Gen {
  unsigned CisaIndex;
  unsigned GenOffset;
};
struct IDX_VDbgGen2CisaIndex {
  unsigned GenOffset;
  unsigned VisaIndex;
};
void generateCISAByteOffsetFromOffset(
    std::map<unsigned int, unsigned int> &mapCISAIndexCISAOffset,
    std::vector<IDX_VDbgCisaIndex2Gen> &mapCISAIndexGenOffset,
    std::vector<IDX_VDbgCisaByte2Gen> &mapCISAOffsetGenOffset);
void generateByteOffsetMapping(
    vISA::G4_Kernel &kernel,
    std::vector<std::pair<unsigned int, unsigned int>> &mapping,
    std::list<vISA::G4_BB *> &stackCallEntryBBs);
void updateRelocOffset(VISAKernelImpl &kernel);
void resetGenOffsets(vISA::G4_Kernel &kernel);

void addCallFrameInfo(VISAKernelImpl *kernel);

// For ranges colored during graph coloring
void updateDebugInfo(vISA::G4_Kernel &kernel, vISA::G4_INST *inst,
                     const vISA::LivenessAnalysis &liveAnalysis,
                     const LiveRangeVec& lrs, llvm_SBitVector &live,
                     vISA::DebugInfoState *state, bool closeAllOpenIntervals);
// For ranges allocated by local RA
void updateDebugInfo(vISA::G4_Kernel &kernel,
                     std::vector<vISA::LocalLiveRange *> &liveIntervals);
// For ranges allocated by global linear scan
void updateDebugInfo(vISA::G4_Kernel &kernel,
                     std::vector<vISA::LSLiveRange *> &liveIntervals);

// For ranges updated by augmentation
void updateDebugInfo(vISA::G4_Kernel &kernel,
                     std::vector<std::tuple<vISA::G4_Declare *, vISA::G4_INST *,
                                            vISA::G4_INST *>>
                         augmentationLiveIntervals);
// For ranges assigned using unique assignments in local RA
void updateDebugInfo(vISA::G4_Kernel &kernel, vISA::G4_Declare *dcl,
                     uint32_t start, uint32_t end);
void updateDebugInfo(vISA::G4_Kernel &kernel, vISA::G4_Declare *dcl,
                     uint32_t offset);

void updateCallStackLiveIntervals(vISA::G4_Kernel &kernel);

#define DEBUG_MAGIC_NUMBER ((unsigned int)0xdeadd010)

// Format of debug info
struct VarnameMap {
#define VARMAP_VREG_FILE_ADDRESS 0
#define VARMAP_VREG_FILE_FLAG 1
#define VARMAP_VREG_FILE_GRF 2

  // 0 - address, 1 - flag, 2 - GRF
  uint8_t virtualType;

#define VARMAP_PREG_FILE_ADDRESS 0
#define VARMAP_PREG_FILE_FLAG 1
#define VARMAP_PREG_FILE_GRF 2
#define VARMAP_PREG_FILE_MEMORY 3

  // Physical register type allocated by RA: 0 - address, 1 - flag, 2 - GRF, 3 -
  // memory
  uint8_t physicalType;
  union Mapping {
    struct Register {
      uint16_t regNum;
      // For GRF, sub reg num is in byte granularity
      uint16_t subRegNum;
    } Register;
    struct Memory {
      // For globals, spill offset is absolute whereas for frame auto vars,
      // it is BE_FP relative.
      uint8_t isAbs : 1;
      int32_t memoryOffset : 31;
    } Memory;
  } Mapping;

  vISA::G4_Declare *dcl;
};

// Instance of this class is created and stored in G4_Kernel.
// Any debug info related members will go here and necessary
// getters/setters should be added here. One level of indirection
// will be required from G4_Kernel instance to get to this
// instance, but this means no space/perf penalty is imposed
// on G4_Kernel.
namespace vISA {
class LiveIntervalInfo;
using LIIAlloc = llvm::SpecificBumpPtrAllocator<LiveIntervalInfo>;

class LiveIntervalInfo {
public:
  enum DebugLiveIntervalState { Open = 0, Closed = 1 };

private:
  std::list<std::pair<uint32_t, uint32_t>> liveIntervals;
  uint32_t cleanedAt;
  DebugLiveIntervalState state;
  uint32_t openIntervalVISAIndex;

public:
  void *operator new(size_t sz, LIIAlloc &Allocator) {
    return Allocator.Allocate(sz / sizeof(LiveIntervalInfo));
  }

  void addLiveInterval(uint32_t start, uint32_t end);
  void liveAt(uint32_t cisaOff);
  void getLiveIntervals(std::vector<std::pair<uint32_t, uint32_t>> &intervals);
  void clearLiveIntervals() { liveIntervals.clear(); }

  DebugLiveIntervalState getState() const { return state; }

  void setStateOpen(uint32_t VISAIndex) {
    // MUST_BE_TRUE(state == Closed, "Cannot open internal in Open state");
    state = Open;
    openIntervalVISAIndex = VISAIndex;
  }

  void setStateClosed(uint32_t VISAIndex) {
    // MUST_BE_TRUE(state == Open, "Cannot close interval in Close state");
    state = Closed;
    addLiveInterval(VISAIndex, openIntervalVISAIndex);
  }

  bool isLiveAt(uint32_t VISAIndex) const {
    for (auto &k : liveIntervals) {
      if (k.first <= VISAIndex && k.second >= VISAIndex)
        return true;
    }
    return false;
  }

  LiveIntervalInfo() {
    cleanedAt = 0;
    state = Closed;
    openIntervalVISAIndex = 0;
  }
};

class KernelDebugInfo {
private:
  G4_Kernel *kernel;
  VISAKernelImpl *visaKernel;
  std::unordered_map<G4_Declare *, LiveIntervalInfo *> debugInfoLiveIntervalMap;

  // Instruction that saves caller BE_FP to callerbpfpdcl
  G4_INST *saveCallerFP;
  // Instruction that restores caller BE_FP prior to return
  G4_INST *restoreCallerFP;
  // Instruction that updates BE_FP for current frame
  G4_INST *setupFP;
  // Instruction that destroys BE_FP for current frame
  G4_INST *restoreSP;

  // Current frame size in bytes
  uint32_t frameSize;

  // Store declare used holding return value for stack call functions,
  // NULL for kernel.
  G4_Declare *fretVar;

  // Caller save/restore
  // std::vector<std::pair<fcall inst BB, std::pair<first caller save, last
  // caller restore>>> One entry per fcall inst in current compilation unit
  typedef std::pair<std::vector<G4_INST *>, std::vector<G4_INST *>> SaveRestore;
  std::unordered_map<G4_BB *, SaveRestore> callerSaveRestore;
  SaveRestore calleeSaveRestore;

  INST_LIST oldInsts;

  // Store pair of cisa byte offset and gen byte offset in vector
  std::vector<IDX_VDbgCisaByte2Gen> mapCISAOffsetGenOffset;
  // Store pair of cisa index and gen byte offset in vector
  std::vector<IDX_VDbgCisaIndex2Gen> mapCISAIndexGenOffset;
  // Store varname map instance for each dcl
  std::vector<VarnameMap *> varsMap;
  // Store map between CISA bytecode index and CISA offset
  std::map<unsigned int, unsigned int> mapCISAOffset;
  // Store reloc_offset of gen binary. This is emitted out to debug info.
  uint32_t reloc_offset;

  // Store set of missing VISA ids as this helps consolidate live-intervals
  // to save compile time.
  std::set<unsigned int> missingVISAIds;
  bool missingVISAIdsComputed;

  std::vector<IDX_VDbgGen2CisaIndex> genISAOffsetToVISAIndex;

  // Store all dcls that are from stack call function
  std::unordered_set<G4_Declare *> stackCallDcls;

  LIIAlloc LIIAllocator;
  Mem_Manager varNameMapAlloc;

public:
  LiveIntervalInfo *getLiveIntervalInfo(G4_Declare *dcl,
                                        bool createIfNULL = true);
  G4_Kernel &getKernel() { return *kernel; }
  VISAKernelImpl *getVISAKernel() const { return visaKernel; }
  void setVISAKernel(VISAKernelImpl *k);

  KernelDebugInfo();
  ~KernelDebugInfo() = default;

  void reset() {
    mapCISAOffsetGenOffset.clear();
    mapCISAIndexGenOffset.clear();
    varsMap.clear();
    resetRelocOffset();
    missingVISAIds.clear();
    missingVISAIdsComputed = false;
  }

  G4_Declare *getBEFP() {
    G4_Declare *ret = nullptr;

    if (!kernel->fg.getIsStackCallFunc()) {
      ret = kernel->fg.builder->getBEFP();
    } else if (saveCallerFP) {
      ret = GetTopDclFromRegRegion(saveCallerFP->getSrc(0));
    }

    return ret;
  }

  G4_Declare *getCallerBEFP() {
    G4_Declare *ret = nullptr;

    if (saveCallerFP) {
      ret = GetTopDclFromRegRegion(saveCallerFP->getDst());
    }

    return ret;
  }

  G4_INST *getCallerBEFPSaveInst() const { return saveCallerFP; }
  void setCallerBEFPSaveInst(G4_INST *i) { saveCallerFP = i; }

  G4_INST *getCallerBEFPRestoreInst() const { return restoreCallerFP; }
  void setCallerBEFPRestoreInst(G4_INST *i) { restoreCallerFP = i; }

  G4_INST *getBEFPSetupInst() const { return setupFP; }
  void setBEFPSetupInst(G4_INST *i) { setupFP = i; }

  G4_INST *getCallerSPRestoreInst() const { return restoreSP; }
  void setCallerSPRestoreInst(G4_INST *i) { restoreSP = i; }

  uint32_t getFrameSize() const { return frameSize; }
  void setFrameSize(uint32_t sz) { frameSize = sz; }

  G4_Declare *getFretVar() const { return fretVar; }
  void setFretVar(G4_Declare *dcl) { fretVar = dcl; }

  void updateExpandedIntrinsic(G4_InstIntrinsic *spillOrFill, G4_INST *inst);
  void addCallerSaveInst(G4_BB *fcallBB, G4_INST *inst);
  void addCallerRestoreInst(G4_BB *fcallBB, G4_INST *inst);
  void addCalleeSaveInst(G4_INST *inst);
  void addCalleeRestoreInst(G4_INST *inst);
  bool isFcallWithSaveRestore(G4_BB *bb);

  std::vector<G4_INST *> &getCallerSaveInsts(G4_BB *fcallBB);
  std::vector<G4_INST *> &getCallerRestoreInsts(G4_BB *fcallBB);
  std::vector<G4_INST *> &getCalleeSaveInsts();
  std::vector<G4_INST *> &getCalleeRestoreInsts();

  void setOldInstList(G4_BB *bb) { oldInsts.assign(bb->begin(), bb->end()); }
  void clearOldInstList() { oldInsts.clear(); }
  INST_LIST getDeltaInstructions(G4_BB *bb);

  void resetRelocOffset() { reloc_offset = 0; }
  void updateMapping(std::list<G4_BB *> &stackCallEntryBBs);

  void generateByteOffsetMapping(std::list<G4_BB *> &stackCallEntryBBs);
  void emitRegisterMapping();
  void generateCISAByteOffsetFromOffset();
  void updateRelocOffset();
  void updateCallStackLiveIntervals();
  void updateCallStackMain();
  void generateGenISAToVISAIndex();

  void computeDebugInfo(std::list<G4_BB *> &stackCallEntryBBs);

  uint32_t getRelocOffset() const { return reloc_offset; }

  void mapCISAOffsetInsert(unsigned int a, unsigned int b) {
    mapCISAOffset.insert(std::make_pair(a, b));
  }

  unsigned int getGenOffsetFromVISAIndex(unsigned int v) const {
    for (auto &item : mapCISAIndexGenOffset) {
      if (item.CisaIndex == v)
        return item.GenOffset;
    }
    return 0;
  }

  std::vector<IDX_VDbgCisaByte2Gen> &getMapCISAOffsetGenOffset() {
    return mapCISAOffsetGenOffset;
  }
  std::vector<IDX_VDbgCisaIndex2Gen> &getMapCISAIndexGenOffset() {
    return mapCISAIndexGenOffset;
  }
  std::vector<IDX_VDbgGen2CisaIndex> &getMapGenISAOffsetToCISAIndex() {
    return genISAOffsetToVISAIndex;
  }
  std::vector<VarnameMap *> &getVarsMap() { return varsMap; }

  uint32_t getVarIndex(G4_Declare *dcl);

  void computeMissingVISAIds();
  bool isMissingVISAId(unsigned int);

  void markStackCallFuncDcls(G4_Kernel &function);
};

class SaveRestoreInfo {
  G4_INST *i;

public:
  // Map src GRF->GRF/Memory
  union RegMap {
    uint32_t regNum;
    struct {
      int32_t offset : 31;
      uint32_t isAbs : 1;
    };
    int32_t memOff;
  };
  enum RegOrMem { Reg = 1, MemAbs = 2, MemOffBEFP = 3 };
  std::map<uint32_t, std::pair<RegOrMem, RegMap>> saveRestoreMap;

  bool isEqual(SaveRestoreInfo &other) {
    auto otherMapIt = other.saveRestoreMap.begin();

    if (this->saveRestoreMap.size() != other.saveRestoreMap.size()) {
      return false;
    }

    for (auto &thisMap : saveRestoreMap) {
      if (thisMap.first != otherMapIt->first ||
          thisMap.second.first != otherMapIt->second.first ||
          thisMap.second.second.memOff != otherMapIt->second.second.memOff) {
        return false;
      }

      otherMapIt++;
    }

    return true;
  }

  void update(G4_INST *inst, int32_t memOffset = 0xffff,
              uint32_t regWithMemOffset = 0xffff, bool absOffset = false);
  G4_INST *getInst() const { return i; }
};

class SaveRestoreManager {
  VISAKernelImpl *visaKernel;
  std::vector<SaveRestoreInfo> srInfo;
  int32_t memOffset;
  uint32_t regWithMemOffset;
  bool absOffset;

public:
  enum CallerOrCallee { Caller = 1, Callee = 2 };

  void addInst(G4_INST *inst);

  SaveRestoreManager(VISAKernelImpl *k) {
    visaKernel = k;
    memOffset = 0xffff;
    regWithMemOffset = 0xffff;
    absOffset = false;
  }

  std::vector<SaveRestoreInfo> &getSRInfo() { return srInfo; }

  void sieveInstructions(CallerOrCallee c);

  void emitAll();
};

class DbgDecoder {
private:
  const char *const filename;
  std::FILE *dbgFile = nullptr;
  const PlatformInfo *platInfo = nullptr;

  void ddName();
  template <class T> void ddLiveInterval();
  void ddCalleeCallerSave(uint32_t relocOffset);

public:
  DbgDecoder(const char *f, TARGET_PLATFORM platform) : filename(f) {
    platInfo = PlatformInfo::LookupPlatformInfo(platform);
    vISA_ASSERT(platInfo != nullptr, "failed to look up platform");
  }

  int ddDbg();
};

class DebugInfoState {
  // Class used to store state during RA.
public:
  void setPrevBitset(const llvm_SBitVector &b) { prevBitset = b; }
  void setPrevInst(G4_INST *i) {
    if (i->getVISAId() != UNMAPPABLE_VISA_INDEX) {
      prevInst = i;
    }
  }

  llvm_SBitVector *getPrevBitset() {
    if (prevBitset.empty())
      return nullptr;
    return &prevBitset;
  }
  G4_INST *getPrevInst() { return prevInst; }

private:
  llvm_SBitVector prevBitset;
  G4_INST *prevInst = nullptr;
};
} // namespace vISA
/* Debug info format:
struct DebugFormatHeader
{
    uint32_t magic;
    uint16_t numCompiledObjects;
    DebugInfoFormat debugInfo[numCompiledObjectsObjects];
}

struct DebugInfoFormat
{
    uint16_t kernelNameLen;
    char kernelName[kernelNameLen];
    uint32_t reloc_offset; // 0 for kernel, non-zero for stack call functions

    struct CISAOffsetMap
    {
        uint32_t numElements;
        CISAMap data[numElements];
    }

    struct CISAIndexMap
    {
        uint32_t numElements;
        CISAMap data[numElements];
    }

    struct VarInfoMap
    {
        uint32_t numElements;

        struct VarInfo[numElements]
        {
            VarName name;
            VarLiveIntervalVISA lr;
        }
    }

    uint16_t numSubs
    SubroutineInfo subInfo[numSubs];

    CallFrameInfo frameInfo;
}

struct CISAMap
{
    uint32_t cisaOffset/cisaIndex;
    uint32_t genOffset;
}

struct SubroutineInfo
{
    VarName subName;
    uint32_t startVISAOffset;
    uint32_t endVISAOffset;
    VarLiveIntervalVISA retVal;
}

struct CallFrameInfo
{
    uint16_t frameSizeInBytes;
    uint8_t befpValid;
    VarLiveIntervalGenISA befp; // Validity depends on flag befpValid
    uint8_t callerbefpValid;
    VarLiveIntervalGenISA callerbefp; // Validity depends on flag
callerbefpValid uint8_t retAddrValid; VarLiveIntervalGenISA retAddr; // Validity
depends on flag retAddrValid uint16_t numCalleeSaveEntries; PhyRegSaveInfoPerIP
calleeSaveEntry[numCalleeSaveEntries];
    // Need this because of following:
    //
    // V10 -> r2, r3, r4, r5, r6, r7
    //
    // send (16) null:w   r1 <-- Writes 4 GRFs (r1, r2, r3, r4)
    // r4 = r0
    // r4.2 =...
    // send (16) null:w   r4 <-- Writes 4 GRFs (r5, r6, r7, r8)
    //
    uint16_t numCallerSaveEntries;
    PhyRegSaveInfoPerIP callerSaveEntry[numCallerSaveEntries];
}

struct VarName
{
    uint16_t varNameLen;
    char varName[varNameLen];
}

struct VarAlloc
{
    uint8_t virtualType; // Virtual register type from CISA file: 0 - address, 1
- flag, 2 - GRF uint8_t physicalType; // Physical register type allocated by RA:
0 - address, 1 - flag, 2 - GRF, 3 - memory Mapping mapping;
}

union Mapping
{
    struct Register
    {
        uint16_t regNum;
        uint16_t subRegNum; // for GRF, in byte offset
    }
    struct Memory
    {
        uint32_t isBaseOffBEFP : 1; // MSB of 32-bit field denotes whether base
if off BE_FP (0) or absolute (1) int32_t memoryOffset : 31; // memory offset
    }
}

struct VarLiveIntervalVISA
{
    uint16_t numIntervals;
    LiveIntervalVISA interval[numIntervals];
}

struct VarLiveIntervalGenISA
{
    uint16_t numIntervals;
    LiveIntervalGenISA interval[numIntervals];
}

struct LiveIntervalVISA
{
    uint16_t start;
    uint16_t end;
    VarAlloc alloc;
}

struct LiveIntervalGenISA
{
    uint32_t start;
    uint32_t end;
    VarAlloc alloc;
}

struct PhyRegSaveInfoPerIP
{
    uint32_t genIPOffset;
    uint16_t numEntries;
    RegInfoMapping data[numEntries];
}

struct RegInfoMapping
{
    RegInfo src;
    uint8_t dstInReg;
    Mapping dst;
}

struct RegInfo
{
    // GRF file r0.0 -> srcRegOff = 0, r1.0 -> srcRegOff = 32.
    // Addr and flag registers can be represented beyond GRF file size of 4k.
But not currently required since they are not callee save. uint16_t srcRegOff;
    uint16_t numBytes;
}

*/
#endif
