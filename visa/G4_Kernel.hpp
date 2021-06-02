/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef G4_KERNEL_HPP
#define G4_KERNEL_HPP

#include "G4_IR.hpp"
#include "FlowGraph.h"
#include "RelocationEntry.hpp"
#include "include/gtpin_IGC_interface.h"

#include <cstdint>
#include <map>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace vISA
{
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

enum RA_Type
{
    RA_TYPE(MAKE_ENUM)
};

class G4_Kernel;

class gtPinData
{
public:
    enum RAPass
    {
        FirstRAPass = 0,
        ReRAPass = 1
    };

    gtPinData(G4_Kernel& k) : kernel(k) {whichRAPass = FirstRAPass;}
    ~gtPinData() { }

    void *operator new(size_t sz, Mem_Manager& m) { return m.alloc(sz); }

    void markInst(G4_INST* i) {
        MUST_BE_TRUE(whichRAPass == FirstRAPass,
            "Unexpectedly marking in re-RA pass.");
        markedInsts.insert(i);
    }

    void markInsts();
    void clearMarkedInsts() { markedInsts.clear(); }
    void removeUnmarkedInsts();

    bool isFirstRAPass() const { return whichRAPass == RAPass::FirstRAPass; }
    bool isReRAPass() const { return whichRAPass == RAPass::ReRAPass; }
    void setRAPass(RAPass p) { whichRAPass = p; }

    // All following functions work on byte granularity of GRF file
    void clearFreeGlobalRegs() { globalFreeRegs.clear(); }
    unsigned getNumFreeGlobalRegs() const { return (unsigned)globalFreeRegs.size(); }
    unsigned getFreeGlobalReg(unsigned n) const { return globalFreeRegs[n]; }
    void addFreeGlobalReg(unsigned n) { globalFreeRegs.push_back(n); }

    // This function internally mallocs memory to hold buffer
    // of free GRFs. It is meant to be freed by caller after
    // last use of the buffer.
    void* getFreeGRFInfo(unsigned& size);
    void  setGTPinInit(void* buffer);

    gtpin::igc::igc_init_t* getGTPinInit() { return gtpin_init; }

    // return igc_info_t format buffer. caller casts it to igc_info_t.
    void* getGTPinInfoBuffer(unsigned &bufferSize);

    void setScratchNextFree(unsigned next) {
        nextScratchFree = ((next + numEltPerGRF<Type_UB>() - 1) / numEltPerGRF<Type_UB>()) * numEltPerGRF<Type_UB>();
    }
    uint32_t getNumBytesScratchUse() const;

    void setPerThreadPayloadBB(G4_BB* bb) { perThreadPayloadBB = bb; }
    void setCrossThreadPayloadBB(G4_BB* bb) { crossThreadPayloadBB = bb; }

    unsigned getCrossThreadNextOff() const;
    unsigned getPerThreadNextOff() const;

    void setGTPinInitFromL0(bool val) { gtpinInitFromL0 = val; }
    bool isGTPinInitFromL0() const { return gtpinInitFromL0; }

private:
    G4_Kernel& kernel;
    std::set<G4_INST*> markedInsts;
    RAPass whichRAPass;
    // globalFreeRegs are in units of bytes in linearized register file.
    // Data is assumed to be sorted in ascending order during insertion.
    // Duplicates are not allowed.
    std::vector<unsigned> globalFreeRegs;
    // Member stores next free scratch slot
    unsigned nextScratchFree = 0;

    bool gtpinInitFromL0 = false;
    gtpin::igc::igc_init_t* gtpin_init = nullptr;

    G4_BB* perThreadPayloadBB = nullptr;
    G4_BB* crossThreadPayloadBB = nullptr;
}; // class gtPinData

class G4_BB;
class KernelDebugInfo;
class VarSplitPass;


class G4_Kernel
{
public:
    using RelocationTableTy = std::vector<RelocationEntry>;

private:
    const char* name;
    unsigned numRegTotal;
    unsigned numThreads;
    unsigned numSWSBTokens;
    unsigned numAcc;
    G4_ExecSize simdSize {0u}; // must start as 0
    bool channelSliced = true;
    bool hasAddrTaken;
    bool regSharingHeuristics;
    Options *m_options;
    const Attributes* m_kernelAttrs;

    RA_Type RAType;
    KernelDebugInfo* kernelDbgInfo = nullptr;
    gtPinData* gtPinInfo = nullptr;

    uint32_t asmInstCount;
    uint64_t kernelID;

    unsigned callerSaveLastGRF;

    bool m_hasIndirectCall = false;

    VarSplitPass* varSplitPass = nullptr;

    // map key is filename string with complete path.
    // if first elem of pair is false, the file wasn't found.
    // the second elem of pair stores the actual source line stream
    // for each source file referenced by this kernel.
    std::map<std::string, std::pair<bool, std::vector<std::string>>> debugSrcLineMap;

    // This must be explicitly set by kernel attributes later
    VISATarget kernelType = VISA_3D;

    // stores all relocations to be performed after binary encoding
    RelocationTableTy relocationTable;

    // the last output we dumped for this kernel and index of next dump
    std::string            lastG4Asm;
    int                    nextDumpIndex = 0;

public:
    FlowGraph              fg;
    DECLARE_LIST           Declares;

    unsigned char major_version;
    unsigned char minor_version;

    G4_Kernel(INST_LIST_NODE_ALLOCATOR& alloc,
        Mem_Manager& m, Options* options, Attributes* anAttr,
        unsigned char major, unsigned char minor);
    ~G4_Kernel();

    void *operator new(size_t sz, Mem_Manager& m) {return m.alloc(sz);}

    void setBuilder(IR_Builder *pBuilder) {fg.setBuilder(pBuilder);}

    bool useRegSharingHeuristics() const {
        // Register sharing not enabled in presence of stack calls
        return regSharingHeuristics && !m_hasIndirectCall &&
            !fg.getIsStackCallFunc() && !fg.getHasStackCalls();
    }

    void     setNumThreads(int nThreads) { numThreads = nThreads; }
    uint32_t getNumThreads() const { return numThreads; }

    uint32_t getNumSWSBTokens() const { return numSWSBTokens; }

    uint32_t getNumAcc() const { return numAcc; }

    void     setAsmCount(int count) { asmInstCount = count; }
    uint32_t getAsmCount() const { return asmInstCount; }

    void     setKernelID(uint64_t ID) { kernelID = ID; }
    uint64_t getKernelID() const { return kernelID; }

    Options *getOptions() { return m_options; }
    const Attributes* getKernelAttrs() const { return m_kernelAttrs; }
    bool getBoolKernelAttr(Attributes::ID aID) const {
        return getKernelAttrs()->getBoolKernelAttr(aID);
    }
    int32_t getInt32KernelAttr(Attributes::ID aID) const {
        return getKernelAttrs()->getInt32KernelAttr(aID);
    }
    bool getOption(vISAOptions opt) const { return m_options->getOption(opt); }
    void computeChannelSlicing();
    void calculateSimdSize();
    G4_ExecSize getSimdSize() { return simdSize; }
    bool getChannelSlicing() { return channelSliced; }
    unsigned getSimdSizeWithSlicing() { return channelSliced ? simdSize/2 : simdSize; }

    void setHasAddrTaken(bool val) { hasAddrTaken = val; }
    bool getHasAddrTaken() { return hasAddrTaken;  }

    void setNumRegTotal(unsigned num) { numRegTotal = num; }
    unsigned getNumRegTotal() const { return numRegTotal; }

    void setName(const char* n) { name = n; }
    const char* getName() { return name; }

    void updateKernelByNumThreads(int nThreads);

    void evalAddrExp();

    void setRAType(RA_Type type) { RAType = type; }
    RA_Type getRAType() { return RAType; }

    void setKernelDebugInfo(KernelDebugInfo* k) { kernelDbgInfo = k; }
    KernelDebugInfo* getKernelDebugInfo();

    bool hasGTPinInit() const {return gtPinInfo && gtPinInfo->getGTPinInit();}
    gtPinData* getGTPinData() {
        if (!gtPinInfo)
            allocGTPinData();

        return gtPinInfo;
    }
    void allocGTPinData() {gtPinInfo = new(fg.mem) gtPinData(*this);}

    unsigned getCallerSaveLastGRF() const { return callerSaveLastGRF; }

    // This function returns starting register number to use
    // for allocating FE/BE stack/frame ptrs.
    unsigned getStackCallStartReg() const;
    unsigned calleeSaveStart() const;
    unsigned getNumCalleeSaveRegs() const;

    // return the number of reserved GRFs for stack call ABI
    // the reserved registers are at the end of the GRF file (e.g., r125-r127)
    uint32_t numReservedABIGRF() const {
        return 3;
    }

    // purpose of the GRFs reserved for stack call ABI
    const int FPSPGRF = 0;
    const int SpillHeaderGRF = 1;
    const int ThreadHeaderGRF = 2;

    uint32_t getFPSPGRF() const{
        return getStackCallStartReg() + FPSPGRF;
    }

    uint32_t getSpillHeaderGRF() const{
        return getStackCallStartReg() + SpillHeaderGRF;
    }

    uint32_t getThreadHeaderGRF() const{
        return getStackCallStartReg() + ThreadHeaderGRF;
    }

    void renameAliasDeclares();

    bool hasIndirectCall() const {return m_hasIndirectCall;}
    void setHasIndirectCall() {m_hasIndirectCall = true;}

    RelocationTableTy& getRelocationTable() {
        return relocationTable;
    }

    const RelocationTableTy& getRelocationTable() const {
        return relocationTable;
    }

    void doRelocation(void* binary, uint32_t binarySize);

    G4_INST* getFirstNonLabelInst() const;

    std::string getDebugSrcLine(const std::string& filename, int lineNo);

    VarSplitPass* getVarSplitPass();

    VISATarget getKernelType() const { return kernelType; }
    void setKernelType(VISATarget t) { kernelType = t; }


    /// dump this kernel to the standard error
    void dump(std::ostream &os = std::cerr) const;  // used in debugger

    // dumps .dot files (if enabled) and .g4 (if enabled)
    void dumpToFile(const std::string &suffix);

    void emitDeviceAsm(std::ostream& output, const void * binary, uint32_t binarySize);

    void emitRegInfo();
    void emitRegInfoKernel(std::ostream& output);

private:
    void setKernelParameters();

    void dumpDotFileInternal(const std::string &baseName);
    void dumpG4Internal(const std::string &baseName);
    void dumpG4InternalTo(std::ostream &os);

    // stuff pertaining to emitDeviceAsm
    void emitDeviceAsmHeaderComment(std::ostream& os);
    void emitDeviceAsmInstructionsIga(std::ostream& os, const void * binary, uint32_t binarySize);
    void emitDeviceAsmInstructionsOldAsm(std::ostream& os);

}; // G4_Kernel
}



#endif // G4_KERNEL_HPP
