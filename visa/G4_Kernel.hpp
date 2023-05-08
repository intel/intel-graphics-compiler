/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef G4_KERNEL_HPP
#define G4_KERNEL_HPP

#include "FlowGraph.h"
#include "G4_IR.hpp"
#include "PlatformInfo.h"
#include "RelocationEntry.hpp"
#include "include/gtpin_IGC_interface.h"
#include "Assertions.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace vISA {
#define RA_TYPE(DO)                                                            \
  DO(TRIVIAL_BC_RA)                                                            \
  DO(TRIVIAL_RA)                                                               \
  DO(LOCAL_ROUND_ROBIN_BC_RA)                                                  \
  DO(LOCAL_ROUND_ROBIN_RA)                                                     \
  DO(LOCAL_FIRST_FIT_BC_RA)                                                    \
  DO(LOCAL_FIRST_FIT_RA)                                                       \
  DO(HYBRID_BC_RA)                                                             \
  DO(HYBRID_RA)                                                                \
  DO(GRAPH_COLORING_RR_BC_RA)                                                  \
  DO(GRAPH_COLORING_FF_BC_RA)                                                  \
  DO(GRAPH_COLORING_RR_RA)                                                     \
  DO(GRAPH_COLORING_FF_RA)                                                     \
  DO(GRAPH_COLORING_SPILL_RR_BC_RA)                                            \
  DO(GRAPH_COLORING_SPILL_FF_BC_RA)                                            \
  DO(GRAPH_COLORING_SPILL_RR_RA)                                               \
  DO(GRAPH_COLORING_SPILL_FF_RA)                                               \
  DO(GLOBAL_LINEAR_SCAN_RA)                                                    \
  DO(GLOBAL_LINEAR_SCAN_BC_RA)                                                 \
  DO(UNKNOWN_RA)

enum RA_Type { RA_TYPE(MAKE_ENUM) };

class G4_Kernel;

class gtPinData {
public:

  gtPinData(G4_Kernel &k) : kernel(k) {}
  ~gtPinData() {}

  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }

  // All following functions work on byte granularity of GRF file
  void clearFreeGlobalRegs() { globalFreeRegs.clear(); }
  unsigned getNumFreeGlobalRegs() const {
    return (unsigned)globalFreeRegs.size();
  }
  unsigned getFreeGlobalReg(unsigned n) const { return globalFreeRegs[n]; }
  void addFreeGlobalReg(unsigned n) { globalFreeRegs.push_back(n); }
  void setFreeGlobalRegs(std::vector<unsigned> &vec) { globalFreeRegs = vec; }

  // This function internally mallocs memory to hold buffer
  // of free GRFs. It is meant to be freed by caller after
  // last use of the buffer.
  void *getFreeGRFInfo(unsigned &size);
  void setGTPinInit(void *buffer);

  // This function internally mallocs memory to hold buffer
  // of indirect references. It is meant to be freed by caller
  // after last use of buffer.
  void *getIndirRefs(unsigned int &size);

  gtpin::igc::igc_init_t *getGTPinInit() { return gtpin_init; }

  // return igc_info_t format buffer. caller casts it to igc_info_t.
  void *getGTPinInfoBuffer(unsigned &bufferSize);

  void setScratchNextFree(unsigned next);
  unsigned int getScratchNextFree() const;

  uint32_t getNumBytesScratchUse() const;

  void setGTPinInitFromL0(bool val) { gtpinInitFromL0 = val; }
  bool isGTPinInitFromL0() const { return gtpinInitFromL0; }

  void addIndirRef(G4_Declare *addr, G4_Declare *target) {
    indirRefs[addr].push_back(target);
  }

private:
  G4_Kernel &kernel;
  // globalFreeRegs are in units of bytes in linearized register file.
  // Data is assumed to be sorted in ascending order during insertion.
  // Duplicates are not allowed.
  std::vector<unsigned> globalFreeRegs;
  // Member stores next free scratch slot
  unsigned nextScratchFree = 0;

  bool gtpinInitFromL0 = false;
  gtpin::igc::igc_init_t *gtpin_init = nullptr;

  // Store Addr Var -> Array of targets
  std::unordered_map<G4_Declare *, std::vector<G4_Declare *>> indirRefs;
}; // class gtPinData

class G4_BB;
class KernelDebugInfo;
class VarSplitPass;


// Handles information for GRF selection
class GRFMode {
public:
  GRFMode(const TARGET_PLATFORM platform, Options *op);

  void setModeByNumGRFs(unsigned grfs) {
    unsigned size = configs.size();
    for (unsigned i = 0; i < size; i++) {
      if (configs[i].numGRF == grfs) {
        currentMode = i;
        break;
      }
    }
  }
  void setModeByNumThreads(unsigned nthreads) {
    unsigned size = configs.size();
    for (unsigned i = 0; i < size; i++) {
      if (configs[i].numThreads == nthreads) {
        currentMode = i;
        break;
      }
    }
  }

  unsigned findModeByRegPressure(unsigned maxRP, unsigned largestInputReg);

  unsigned getNumGRF() const { return configs[currentMode].numGRF; }
  unsigned getMinGRF() const { return configs[0].numGRF; }
  unsigned getMaxGRF() const {
    return configs[configs.size() - 1].numGRF;
  }
  unsigned getDefaultGRF() const { return configs[defaultMode].numGRF; }
  unsigned getLargerGRF() const {
    return currentMode + 1 < configs.size() ? configs[currentMode + 1].numGRF
                                            : configs[currentMode].numGRF;
  }
  unsigned getSmallerGRF() const {
    return (signed)currentMode - 1 >= 0 ? configs[currentMode - 1].numGRF
                                        : configs[currentMode].numGRF;
  }

  unsigned getNumThreads() const { return configs[currentMode].numThreads; }
  unsigned getMinNumThreads() const {
    return configs[configs.size() - 1].numThreads;
  }
  unsigned getMaxNumThreads() const { return configs[0].numThreads; }
  unsigned getDefaultNumThreads() const {
    return configs[defaultMode].numThreads;
  }

  unsigned getNumSWSBTokens() const {
    return configs[currentMode].numSWSB;
  }

  unsigned getNumAcc() const { return configs[currentMode].numAcc; }

private:
  // Parameters associated to a GRF mode
  struct Config {
    unsigned numGRF;
    unsigned numThreads;
    unsigned numSWSB;
    unsigned numAcc;
  };
  // Vector configs maintains all the GRF modes available for the platform
  // being compiled.
  //   defaultMode: vector index with the default GRF mode
  //   currentMode: vector index with the current GRF mode that could change
  //                during compilation.
  std::vector<Config> configs;
  unsigned defaultMode;
  unsigned currentMode;
  Options *options;
};


// NoMask WA Information
class NoMaskWAInfo {
public:
  bool HasWAInsts; // true: there are NoMask inst that needs WA.
  G4_Declare *WAFlagReserved;
  G4_Declare *WATempReserved;
};

// Indirect Call WA info
//    Every indirect call (one in a BB) has the following postRA will
//    use this to update relative IP, etc.
class IndirectCallWAInfo {
public:
  //  Call_BB:
  //           Small_start
  //           Small_patch
  //           Big_start
  //           Big_patch
  //           if (BigEU)
  //  BigBB:
  //              big_call (original)
  //           else
  //  SmallBB:
  //              small_call (new one)
  //           endif
  //
  // G4_BB*   Call_BB;       // as map key
  G4_BB *Big_BB;
  G4_BB *Small_BB;
  // SmallEU and BigEU's relative IP are calculated as follow:
  //   Given
  //     call v20            // v20 is absolute target addr
  //   The relative IP is caculated as follows:
  //     add  v10  -ip,  v20                    // start inst
  //     add  v11   v10, 0x33 (-label_patch)    // patch inst
  //     ...
  //     call v11                               // call inst
  //                                            // v11 is relative target addr
  // This map keeps  patch inst --> <ip_start_inst, ip_end_inst>
  // Once encoding is decided, label_path = IP(ip_end_inst) - IP(ip_start_inst)
  //
  G4_INST *IP_WA_placeholder; // IP WA, will be replaced with call if present
  G4_INST *Small_start;       // SmallEU start: add  t -ip      SmallTarget
  G4_INST *Small_patch;       // SmallEU patch: add  rSmallT t  <patchVal>
  G4_INST *Big_start;         // BigEU start :  add  t1 -ip      BigTarget
  G4_INST *Big_patch;         // BigEU patch :  add  rBigT   t1 <patchVal>
  G4_INST *Big_call;
  G4_INST *Small_call;
  IndirectCallWAInfo(G4_BB *aBig_BB, G4_BB *aSmall_BB, G4_INST *aIP_WA,
                     G4_INST *aSmall_start, G4_INST *aSmall_patch,
                     G4_INST *aBig_start, G4_INST *aBig_patch,
                     G4_INST *aBig_call, G4_INST *aSmall_call)
      : IP_WA_placeholder(aIP_WA), Big_BB(aBig_BB), Small_BB(aSmall_BB),
        Small_start(aSmall_start), Small_patch(aSmall_patch),
        Big_start(aBig_start), Big_patch(aBig_patch), Big_call(aBig_call),
        Small_call(aSmall_call) {}

  IndirectCallWAInfo() {}
};

class G4_Kernel {
public:
  using RelocationTableTy = std::vector<RelocationEntry>;

private:
  const PlatformInfo &platformInfo;
  const char *name;
  unsigned numRegTotal = 0;
  unsigned numThreads = 0;
  unsigned numSWSBTokens = 0;
  unsigned numAcc = 0;
  G4_ExecSize simdSize{0u}; // must start as 0
  bool channelSliced = true;
  bool hasAddrTaken;
  bool regSharingHeuristics = false;
  bool needDPASWA = false;
  Options *m_options;
  const Attributes *m_kernelAttrs;
  const uint32_t m_function_id;

  RA_Type RAType;
  std::shared_ptr<KernelDebugInfo> kernelDbgInfo = nullptr;
  std::shared_ptr<gtPinData> gtPinInfo = nullptr;

  uint32_t asmInstCount = 0;
  uint64_t kernelID = 0;

  unsigned callerSaveLastGRF = 0;

  bool m_hasIndirectCall = false;

  VarSplitPass *varSplitPass = nullptr;
  GRFMode grfMode;

  // map key is filename string with complete path.
  // if first elem of pair is false, the file wasn't found.
  // the second elem of pair stores the actual source line stream
  // for each source file referenced by this kernel.
  std::map<std::string, std::pair<bool, std::vector<std::string>>>
      debugSrcLineMap;

  // This must be explicitly set by kernel attributes later
  VISATarget kernelType = VISA_3D;

  // stores all relocations to be performed after binary encoding
  RelocationTableTy relocationTable;

  // the last output we dumped for this kernel and index of next dump
  std::string lastG4Asm;
  int nextDumpIndex = 0;

  G4_BB *perThreadPayloadBB = nullptr;
  G4_BB *crossThreadPayloadBB = nullptr;
  // There's two entires prolog for setting FFID for compute shaders.
  G4_BB *computeFFIDGP = nullptr;
  G4_BB *computeFFIDGP1 = nullptr;

  // For debug purpose: kernel is local-scheduled or not according to options.
  bool isLocalSchedulable = true;

  // Maps for optional G4_INST/G4_OPERAND fields that we don't want to store
  // directly in the class.
  // TODO: If the number of such maps gets large, we should move them to a
  // separate class (G4_IRInfo?).
  std::unordered_map<G4_INST *, G4_SrcRegRegion *> instImplicitAccSrc;
  std::unordered_map<G4_INST *, G4_DstRegRegion *> instImplicitAccDef;

  // Internal use of updating shared_ptr KernelDebugInfo
  // To access the pointer, use getKernelDebugInfo() instead
  void setKernelDebugInfoSharedPtr(std::shared_ptr<KernelDebugInfo>& k) {
    vISA_ASSERT(k.get(), "incorrect nullptr");
    kernelDbgInfo = k;
  }
  std::shared_ptr<KernelDebugInfo>& getKernelDebugInfoSharedPtr() {
    return kernelDbgInfo;
  }

  // Internal use of updating shared_ptr gtPinData
  // To access the pointer, use getGTPinData() instead
  void setGTPinDataSharedPtr(std::shared_ptr<gtPinData>& p) {
    vISA_ASSERT(p.get(), "incorrect nullptr");
    gtPinInfo = p;
  }
  std::shared_ptr<gtPinData>& getGTPinDataSharedPtr() {
    if (!gtPinInfo)
      allocGTPinData();

    return gtPinInfo;
  }


public:
  FlowGraph fg;
  DECLARE_LIST Declares;
  DECLARE_LIST callerRestoreDecls;

  unsigned char major_version;
  unsigned char minor_version;

  G4_Kernel(const PlatformInfo &pInfo, INST_LIST_NODE_ALLOCATOR &alloc,
            Mem_Manager &m, Options *options, Attributes *anAttr,
            uint32_t funcId, unsigned char major, unsigned char minor);
  ~G4_Kernel();

  TARGET_PLATFORM getPlatform() const { return platformInfo.platform; }
  PlatformGen getPlatformGeneration() const { return platformInfo.family; }
  const char *getGenxPlatformString() const {
    return platformInfo.getGenxPlatformString();
  }
  unsigned char getGRFSize() const { return platformInfo.grfSize; }
  template <G4_Type T> unsigned numEltPerGRF() const {
    return platformInfo.numEltPerGRF<T>();
  }
  unsigned numEltPerGRF(G4_Type t) const {
    return platformInfo.numEltPerGRF(t);
  }
  unsigned getMaxVariableSize() const {
    return platformInfo.getMaxVariableSize();
  }
  G4_SubReg_Align getGRFAlign() const { return platformInfo.getGRFAlign(); }
  G4_SubReg_Align getHalfGRFAlign() const {
    return platformInfo.getHalfGRFAlign();
  }
  unsigned getGenxDataportIOSize() const {
    return platformInfo.getGenxDataportIOSize();
  }
  unsigned getGenxSamplerIOSize() const {
    return platformInfo.getGenxSamplerIOSize();
  }
  unsigned getMaxNumOfBarriers() const {
    return platformInfo.getMaxNumOfBarriers();
  }

  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }

  void setBuilder(IR_Builder *pBuilder) { fg.setBuilder(pBuilder); }

  bool useRegSharingHeuristics() const {
    // Register sharing not enabled in presence of stack calls
    return regSharingHeuristics && !m_hasIndirectCall &&
           !fg.getIsStackCallFunc() && !fg.getHasStackCalls();
  }

  void setNumThreads(int nThreads) { numThreads = nThreads; }
  uint32_t getNumThreads() const { return numThreads; }

  uint32_t getNumSWSBTokens() const { return numSWSBTokens; }

  uint32_t getNumAcc() const { return numAcc; }

  void setAsmCount(int count) {
    vASSERT(count > 0);
    asmInstCount = count;
  }
  uint32_t getAsmCount() const { return asmInstCount; }

  void setKernelID(uint64_t ID) { kernelID = ID; }
  uint64_t getKernelID() const { return kernelID; }

  uint32_t getFunctionId() const { return m_function_id; }

  Options *getOptions() { return m_options; }
  const Attributes *getKernelAttrs() const { return m_kernelAttrs; }
  bool getBoolKernelAttr(Attributes::ID aID) const {
    return getKernelAttrs()->getBoolKernelAttr(aID);
  }
  int32_t getInt32KernelAttr(Attributes::ID aID) const {
    return getKernelAttrs()->getInt32KernelAttr(aID);
  }
  bool getOption(vISAOptions opt) const { return m_options->getOption(opt); }
  uint32_t getuInt32Option(vISAOptions opt) const {
    return m_options->getuInt32Option(opt);
  }
  void computeChannelSlicing();
  void calculateSimdSize();
  G4_ExecSize getSimdSize() { return simdSize; }
  bool getChannelSlicing() const { return channelSliced; }
  unsigned getSimdSizeWithSlicing() {
    return channelSliced ? simdSize / 2 : simdSize;
  }

  void setHasAddrTaken(bool val) { hasAddrTaken = val; }
  bool getHasAddrTaken() { return hasAddrTaken; }

  void setNeedDPASWA(bool val) { needDPASWA = val; }
  bool getNeedDPASWA() const { return needDPASWA; }

  void setNumRegTotal(unsigned num) { numRegTotal = num; }
  unsigned getNumRegTotal() const { return numRegTotal; }

  // Scales GRF size based on 128GRF to current kernel's GRF mode
  // e.g. if value=50, for GRF=128 it returns 50, for GRF=256 it returns 100
  int getScaledGRFSize(int value) const {
    float ratio = 1.0f - (128.0f - value) / 128.0f;
    return static_cast<int>(numRegTotal * ratio);
  }


  void setName(const char *n) { name = n; }
  const char *getName() const { return name; }

  bool updateKernelToLargerGRF();
  bool updateKernelToSmallerGRF();
  void updateKernelByRegPressure(unsigned regPressure);

  void evalAddrExp();

  void setRAType(RA_Type type) { RAType = type; }
  RA_Type getRAType() const { return RAType; }

  bool hasKernelDebugInfo() const { return kernelDbgInfo != nullptr; }
  void updateKernelDebugInfo(G4_Kernel& kernel) {
    setKernelDebugInfoSharedPtr(kernel.getKernelDebugInfoSharedPtr());
  }
  KernelDebugInfo* getKernelDebugInfo();

  bool hasGTPinInit() const { return gtPinInfo && gtPinInfo->getGTPinInit(); }
  gtPinData* getGTPinData() {
    if (!gtPinInfo)
      allocGTPinData();

    return gtPinInfo.get();
  }

  void updateGTPinData(G4_Kernel& kernel) {
    setGTPinDataSharedPtr(kernel.getGTPinDataSharedPtr());
  }
  void allocGTPinData() { gtPinInfo = std::make_shared<gtPinData>(*this); }

  unsigned getCallerSaveLastGRF() const { return callerSaveLastGRF; }

  // This function returns starting register number to use
  // for allocating FE/BE stack/frame ptrs.
  unsigned getStackCallStartReg() const;
  unsigned calleeSaveStart() const;
  unsigned getNumCalleeSaveRegs() const;

  enum StackCallABIVersion {
    VER_1 = 1, // This version is used pre-zebin
    VER_2 = 2, // This version is used for zebin
  };

  // return the number of reserved GRFs for stack call ABI
  // the reserved registers are at the end of the GRF file (e.g., r125-r127)
  // for some architectures/ABI version, we dont need a copy of r0 and
  // instructions can directly refer to r0 in code (eg, sends).
  uint32_t numReservedABIGRF() const {
    if (getuInt32Option(vISA_StackCallABIVer) == VER_1)
      return 3;
    else {
      // for ABI version > 1,
      if (getOption(vISA_PreserveR0InR0))
        return 2;
      return 3;
    }
  }

  // purpose of the GRFs reserved for stack call ABI
  const int FPSPGRF = 0;
  const int SpillHeaderGRF = 1;
  const int ThreadHeaderGRF = 2;

  uint32_t getFPSPGRF() const {
    // For ABI V1 return r125.
    // For ABI V2 return r127.
    if (getuInt32Option(vISA_StackCallABIVer) == VER_1)
      return getStackCallStartReg() + FPSPGRF;
    else
      return (getNumRegTotal() - 1) - FPSPGRF;
  }

  uint32_t getSpillHeaderGRF() const {
    // For ABI V1 return r126.
    // For ABI V2 return r126.
    if (getuInt32Option(vISA_StackCallABIVer) == VER_1)
      return getStackCallStartReg() + SpillHeaderGRF;
    else
      return (getNumRegTotal() - 1) - SpillHeaderGRF;
  }

  uint32_t getThreadHeaderGRF() const {
    // For ABI V1 return r127.
    // For ABI V2 return r125.
    if (getuInt32Option(vISA_StackCallABIVer) == VER_1)
      return getStackCallStartReg() + ThreadHeaderGRF;
    else
      return (getNumRegTotal() - 1) - ThreadHeaderGRF;
  }

  bool hasIndirectCall() const { return m_hasIndirectCall; }
  void setHasIndirectCall() { m_hasIndirectCall = true; }

  RelocationTableTy &getRelocationTable() { return relocationTable; }

  const RelocationTableTy &getRelocationTable() const {
    return relocationTable;
  }

  void doRelocation(void *binary, uint32_t binarySize);

  G4_INST *getFirstNonLabelInst() const;

  std::string getDebugSrcLine(const std::string &filename, int lineNo);

  VarSplitPass *getVarSplitPass();

  VISATarget getKernelType() const { return kernelType; }
  void setKernelType(VISATarget t) { kernelType = t; }

  /// dump this kernel to the standard error
  void dump(std::ostream &os = std::cerr) const; // used in debugger

  // dumps .dot files (if enabled) and .g4 (if enabled)
  void dumpToFile(const std::string &suffix, bool forceG4Dump = false);
  void dumpToFile(char *file) { dumpToFile(std::string(file)); }
  void dumpToConsole();

  void emitDeviceAsm(std::ostream &output, const void *binary,
                     uint32_t binarySize);

  void emitRegInfo();
  void emitRegInfoKernel(std::ostream &output);

  bool hasPerThreadPayloadBB() const { return perThreadPayloadBB != nullptr; }
  G4_BB *getPerThreadPayloadBB() const { return perThreadPayloadBB; }
  void setPerThreadPayloadBB(G4_BB *bb) { perThreadPayloadBB = bb; }
  bool hasCrossThreadPayloadBB() const {
    return crossThreadPayloadBB != nullptr;
  }
  G4_BB *getCrossThreadPayloadBB() const { return crossThreadPayloadBB; }
  void setCrossThreadPayloadBB(G4_BB *bb) { crossThreadPayloadBB = bb; }
  bool hasComputeFFIDProlog() const {
    return computeFFIDGP != nullptr && computeFFIDGP1 != nullptr;
  }
  void setComputeFFIDGPBB(G4_BB *bb) { computeFFIDGP = bb; }
  void setComputeFFIDGP1BB(G4_BB *bb) { computeFFIDGP1 = bb; }

  unsigned getCrossThreadNextOff() const;
  unsigned getPerThreadNextOff() const;
  unsigned getComputeFFIDGPNextOff() const;
  unsigned getComputeFFIDGP1NextOff() const;

  bool isLocalSheduleable() const { return isLocalSchedulable; }
  void setLocalSheduleable(bool value) { isLocalSchedulable = value; }
  unsigned getLargestInputRegister();

  // Use the wrapper in G4_INST* instead of calling these directly.
  G4_SrcRegRegion *getImplicitAccSrc(G4_INST *inst) const {
    auto iter = instImplicitAccSrc.find(inst);
    return iter == instImplicitAccSrc.end() ? nullptr : iter->second;
  }
  G4_DstRegRegion *getImplicitAccDef(G4_INST *inst) const {
    auto iter = instImplicitAccDef.find(inst);
    return iter == instImplicitAccDef.end() ? nullptr : iter->second;
  }
  void setImplicitAccSrc(G4_INST *inst, G4_SrcRegRegion *accSrc) {
    // Do not allow null implicit acc operand.
    vASSERT(accSrc);
    // TODO: Is such check actually necessary? We should not have references to
    // the old operand once it's unlinked.
    auto oldOpnd = getImplicitAccSrc(inst);
    if (oldOpnd && oldOpnd->getInst() == inst)
      oldOpnd->setInst(nullptr);
    instImplicitAccSrc[inst] = accSrc;
    // This also sets accSrc's parent to inst.
    inst->computeRightBound(accSrc);
  }
  void setImplicitAccDef(G4_INST *inst, G4_DstRegRegion *accDef) {
    // Do not allow null implicit acc operand.
    vASSERT(accDef);
    // TODO: Is such check actually necessary? We should not have references to
    // the old operand once it's unlinked.
    auto oldOpnd = getImplicitAccDef(inst);
    if (oldOpnd && oldOpnd->getInst() == inst)
      oldOpnd->setInst(nullptr);
    instImplicitAccDef[inst] = accDef;
    // This also sets accDef's parent to inst.
    inst->computeRightBound(accDef);
  }

private:
  G4_BB *getNextBB(G4_BB *bb) const;
  unsigned getBinOffsetOfBB(G4_BB *bb) const;

  void setKernelParameters();

  void dumpDotFileInternal(const std::string &baseName);
  void dumpG4Internal(const std::string &baseName);
  void dumpG4InternalTo(std::ostream &os);

  // stuff pertaining to emitDeviceAsm
  void emitDeviceAsmHeaderComment(std::ostream &os);
  void emitDeviceAsmInstructionsIga(std::ostream &os, const void *binary,
                                    uint32_t binarySize);
  void emitDeviceAsmInstructionsOldAsm(std::ostream &os);

  // WA related
private:
  // NoMaskWA under EU Fusion: passing info from preRA to postRA
  NoMaskWAInfo *m_EUFusionNoMaskWAInfo = nullptr;

public:
  void createNoMaskWAInfo(G4_Declare *aWAFlag, G4_Declare *aWATemp,
                          bool aInstNeedWA) {
    NoMaskWAInfo *waInfo = new NoMaskWAInfo();
    waInfo->WAFlagReserved = aWAFlag;
    waInfo->WATempReserved = aWATemp;
    waInfo->HasWAInsts = aInstNeedWA;

    m_EUFusionNoMaskWAInfo = waInfo;
  }
  void deleteEUFusionNoMaskWAInfo() {
    delete m_EUFusionNoMaskWAInfo;
    m_EUFusionNoMaskWAInfo = nullptr;
  }
  NoMaskWAInfo *getEUFusionNoMaskWAInfo() const {
    return m_EUFusionNoMaskWAInfo;
  }

  // Call WA
  // m_maskOffWAInsts: insts whose MaskOff will be changed for this call WA.
  std::unordered_map<G4_INST *, G4_BB *> m_maskOffWAInsts;
  // m_indirectCallWAInfo : all info related to indirect call wa
  // such as BBs for smallEU's call/bigEu's call. If ip-relative call,
  // insts to calculate IP-relative address, etc.
  std::unordered_map<G4_BB *, IndirectCallWAInfo> m_indirectCallWAInfo;
  void setMaskOffset(G4_INST *I, G4_InstOption MO) {
    // For call WA
    vASSERT((I->getMaskOffset() + I->getExecSize()) <= 16);
    vASSERT(I->getPredicate() == nullptr && I->getCondMod() == nullptr);
    I->setMaskOption(MO);
  }
  // end of WA related

};     // G4_Kernel

} // namespace vISA

#endif // G4_KERNEL_HPP
