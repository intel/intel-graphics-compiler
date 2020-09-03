/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/


#ifndef _BUILDIR_H_
#define _BUILDIR_H_

#include <cstdarg>
#include <list>
#include <map>
#include <set>
#include <string>

#include "Gen4_IR.hpp"
#include "FlowGraph.h"
#include "visa_igc_common_header.h"
#include "Common_ISA.h"
#include "Common_ISA_util.h"
#include "RT_Jitter_Interface.h"
#include "visa_wa.h"
#include "PreDefinedVars.h"
#include "CompilerStats.h"
#include "BinaryEncodingIGA.h"
#include "inc/common/sku_wa.h"


#define MAX_DWORD_VALUE  0x7fffffff
#define MIN_DWORD_VALUE  0x80000000
#define MAX_UDWORD_VALUE 0xffffffff
#define MIN_UDWORD_VALUE 0
#define MAX_WORD_VALUE 32767
#define MIN_WORD_VALUE -32768
#define MAX_UWORD_VALUE 65535
#define MIN_UWORD_VALUE 0
#define MAX_CHAR_VALUE 127
#define MIN_CHAR_VALUE -128
#define MAX_UCHAR_VALUE 255
#define MIN_UCHAR_VALUE 0

typedef struct FCCalls
{
    // callOffset is in inst number units
    unsigned int callOffset;
    const char* calleeLabelString;
} FCCalls;

enum DeclareType
{
    Regular = 0,
    Fill = 1,
    Spill = 2,
    Tmp = 3,
    AddrSpill = 4,
    CoalescedFill = 5,
    CoalescedSpill = 6
};

// forward declaration
// FIXME: our #include is a mess, need to clean it up
class CISA_IR_Builder;

namespace vISA
{
// IR_Builder class has a member of type FCPatchingInfo
// as a member. This class is expected to hold all FC
// related information.
class FCPatchingInfo
{
private:
    // Flag to tell if this instance has any fast-composite
    // type calls. Callees for such call instructions are not available
    // in same compilation unit.
    bool hasFCCalls;

    // Set to true if kernel has "Callable" attribute set in VISA
    // stream.
    bool isFCCallableKernel;

    // Set to true if kernel was compiled with /mCM_composable_kernel
    // FE flag.
    bool isFCComposableKernel;

    // Set to true if kernel has "Entry" attribute set in VISA
    // stream.
    bool isFCEntryKernel;

    std::vector<FCCalls*> FCCallsToPatch;
    std::vector<unsigned int> FCReturnOffsetsToPatch;

public:
    FCPatchingInfo()
    {
        hasFCCalls = false;
        isFCCallableKernel = false;
        isFCComposableKernel = false;
        isFCEntryKernel = false;
    }

    void setHasFCCalls(bool hasFC) { hasFCCalls = hasFC; }
    bool getHasFCCalls() { return hasFCCalls; }
    void setIsCallableKernel(bool value) { isFCCallableKernel = value; }
    bool getIsCallableKernel() { return isFCCallableKernel; }
    void setFCComposableKernel(bool value) { isFCComposableKernel = value; }
    bool getFCComposableKernel() { return isFCComposableKernel; }
    void setIsEntryKernel(bool value) { isFCEntryKernel = value; }
    bool getIsEntryKernel() { return isFCEntryKernel; }
    std::vector<FCCalls*>& getFCCallsToPatch() { return FCCallsToPatch; }
    std::vector<unsigned int>& getFCReturnsToPatch() { return FCReturnOffsetsToPatch; }

    enum RegAccessType : unsigned char {
      Fully_Use = 0,
      Partial_Use = 1,
      Fully_Def = 2,
      Partial_Def = 3
    };

    enum RegAccessPipe : unsigned char {
      Pipe_ALU = 0,
      Pipe_Math = 1,
      Pipe_Send = 2
    };

    struct RegAccess {
      RegAccess *Next;    // The next access on the same GRF.
      RegAccessType Type; // 'def' or 'use' of that GRF.
      unsigned RegNo;     // GRF.
      unsigned Pipe;      // Pipe.
      // where that access is issued.
      G4_INST *Inst;        // Where that GRF is accessed.
      unsigned Offset;      // Instruction offset populated finally.
      // Token associated with that access.
      unsigned Token; // Optional token allocation associated to 'def'.

      RegAccess() :
        Next(nullptr), Type(Fully_Use), RegNo(0), Pipe(0), Inst(nullptr),
        Offset(0), Token(0) {}
    };

    // FIXME: Need to consider the pipeline ID together with GRF since
    // different pipe will use/def out-of-order. Need to synchronize all of
    // them to resolve the dependency.

    std::list<RegAccess> RegFirstAccessList;
    std::list<RegAccess> RegLastAccessList;
    // Per GRF, the first access.
    std::map<unsigned, RegAccess *> RegFirstAccessMap;
    // Per GRF, the last access.
    std::map<unsigned, RegAccess *> RegLastAccessMap;
    // Note that tokens recorded are tokens allocated but not used in last
    // access list.
    std::set<unsigned> AllocatedToken;  // Allocated token.
};
}

namespace vISA
{
//
// hash table for holding reg vars and reg region
//

class OperandHashTable
{
    Mem_Manager& mem;

    struct ImmKey
    {
        int64_t val;
        G4_Type valType;
        ImmKey(int64_t imm, G4_Type type) : val(imm), valType(type) {}
        bool operator==(const ImmKey& imm) const
        {
            return val == imm.val && valType == imm.valType;
        }
    };

    struct ImmKeyHash
    {
        std::size_t operator()(const ImmKey& imm) const
        {
            return (std::size_t) (imm.val ^ imm.valType);
        }
    };

    struct stringCompare
    {
        bool operator() (const char* s1, const char* s2) const
        {
            return strcmp(s1, s2) == 0;
        }
    };

    std::unordered_map<ImmKey, G4_Imm*, ImmKeyHash> immTable;
    std::unordered_map<const char *, G4_Label*, std::hash<const char*>, stringCompare> labelTable;

public:
    OperandHashTable(Mem_Manager& m) : mem(m)
    {
    }

    // Generic methods that work on both integer and floating-point types.
    // For floating-point types, 'imm' needs to be G4_Imm(<float-value>.getImm().
    G4_Imm*          lookupImm(int64_t imm, G4_Type ty);
    G4_Imm*          createImm(int64_t imm, G4_Type ty);

    G4_Label*        lookupLabel(const char* lab);
    G4_Label*        createLabel(const char* lab);
};

//
// place for holding all region descriptions
//
class RegionPool
{
    Mem_Manager& mem;
    std::vector<RegionDesc*> rgnlist;
public:
    RegionPool(Mem_Manager& m) : mem(m) {}
    const RegionDesc* createRegion(
        uint16_t vstride, uint16_t width, uint16_t hstride);
};

//
// place for hbolding all .declare
//
class DeclarePool
{
    Mem_Manager& mem;
    std::vector<G4_Declare*> dcllist;
    int addrSpillLocCount; //incremented in G4_RegVarAddrSpillLoc()
public:
    DeclarePool(Mem_Manager& m) : mem(m), addrSpillLocCount(0) { dcllist.reserve(2048); }
    ~DeclarePool();

    G4_Declare* createDeclare(
        const char*    name,
        G4_RegFileKind regFile,
        unsigned short nElems,
        unsigned short nRows,
        G4_Type        ty,
        DeclareType    kind = Regular,
        G4_RegVar *    base = nullptr,
        G4_Operand *   repRegion = nullptr,
        G4_ExecSize    execSize = G4_ExecSize(0));

    G4_Declare* createPreVarDeclare(
                              PreDefinedVarsInternal index,
                              unsigned short n_elems,
                              unsigned short n_rows,
                              G4_Type        ty)
    {

        G4_Declare* dcl = new (mem)G4_Declare(getPredefinedVarString(index), G4_INPUT, n_elems * n_rows, ty, dcllist);
        G4_RegVar * regVar;
        regVar = new (mem) G4_RegVar(dcl, G4_RegVar::RegVarType::Default);
        dcl->setRegVar(regVar);

        return dcl;
    }

    std::vector<G4_Declare*>& getDeclareList() {return dcllist;}
};


//
// interface for creating operands and instructions
//
class IR_Builder
{
public:
    const char* curFile;
    unsigned int curLine;
    int curCISAOffset;

    static const int OrphanVISAIndex = 0xffffffff;
    int debugInfoPlaceholder = OrphanVISAIndex; // used for debug info, catch all VISA offset for orphan instructions

private:
    class GlobalImmPool
    {
        struct ImmVal
        {
            G4_Imm* imm;
            int numElt;

            bool operator==(const ImmVal& v) const {
                return imm == v.imm && numElt == v.numElt;
            }
        };
        static const int MAX_POOL_SIZE = 8;   // reg pressure control, for now just do naive first-come first-serve
        std::array<ImmVal, MAX_POOL_SIZE> immArray;
        std::array<G4_Declare*, MAX_POOL_SIZE> dclArray;
        int curSize = 0;
        IR_Builder& builder;

    public:
        GlobalImmPool(IR_Builder& b) : builder(b), immArray(), dclArray() {}
        G4_Declare* addImmVal(G4_Imm* imm, int numElt);
        int size() const { return curSize; }
        const ImmVal& getImmVal(int i) {return immArray[i];}
        G4_Declare* getImmDcl(int i) {return dclArray[i];}
    };

    GlobalImmPool immPool;

    const TARGET_PLATFORM platform;

    //allocator pools
    USE_DEF_ALLOCATOR     useDefAllocator;

    FINALIZER_INFO*       metaData = nullptr;
    CompilerStats         compilerStats;

    int                   subroutineId = -1;   // the kernel itself has id 0, as we always emit a subroutine label for kernel too
    bool                  isKernel; // as opposed to what?

    // pre-defined declare that binds to R0 (the entire GRF)
    // when pre-emption is enabled, builtinR0 is replaced by a temp,
    // and a move is inserted at kernel entry
    // mov (8) builtinR0 realR0
    G4_Declare* builtinR0 = nullptr; // this is either r0 or the temp if pre-emption is enabled
    G4_Declare* realR0 = nullptr; // this always refers to r0

    // pre-defined declare that binds to A0.0:ud
    G4_Declare* builtinA0 = nullptr;
    // pre-defined declare that binds to A0.2:ud
    G4_Declare* builtinA0Dot2 = nullptr; //used for splitsend's ext msg descriptor
    // pre-defind declare that binds to HWTid (R0.5:ud)
    G4_Declare* builtinHWTID = nullptr;
    // pre-defined bindless surface index (252, 1 UD)
    G4_Declare* builtinT252 = nullptr;
    // pre-defined bindless sampler index (31, 1 UD)
    G4_Declare* builtinBindlessSampler = nullptr;
    // pre-defined sampler header
    G4_Declare* builtinSamplerHeader = nullptr;

    // common message header for spill/fill intrinsics
    // We put them here instead of spillManager since there may be multiple rounds of spill,
    // and we want to use a common header
    G4_Declare* spillFillHeader = nullptr;

    G4_Declare* oldA0Dot2Temp = nullptr;


    // Indicates that sampler header cache (builtinSamplerHeader) is correctly
    // initialized with r0 contents.
    // Used only when vISA_cacheSamplerHeader option is set.
    bool builtinSamplerHeaderInitialized;

    // function call related declares
    G4_Declare* be_sp = nullptr;
    G4_Declare* be_fp = nullptr;
    G4_Declare* tmpFCRet = nullptr;

    unsigned short arg_size;
    unsigned short return_var_size;

    unsigned int sampler8x8_group_id;

    // Populate this data structure so after compiling all kernels
    // in file, we can emit out patch file using this up-levelled
    // information.
    FCPatchingInfo* fcPatchInfo = nullptr;

    const PWA_TABLE m_pWaTable;
    Options *m_options = nullptr;

    std::map<G4_INST*, G4_FCALL*> m_fcallInfo;

    // Basic region descriptors.
    RegionDesc CanonicalRegionStride0, // <0; 1, 0>
               CanonicalRegionStride1, // <1; 1, 0>
               CanonicalRegionStride2, // <2; 1, 0>
               CanonicalRegionStride4; // <4; 1, 0>

    class PreDefinedVars
    {
    public:
        PreDefinedVars() {
            memset(hasPredefined,  0, sizeof(hasPredefined));
            memset(predefinedVars, 0, sizeof(predefinedVars));
        }
        void setHasPredefined(PreDefinedVarsInternal id, bool val) {
            hasPredefined[static_cast<int>(id)] = val;
        }

        bool isHasPredefined(PreDefinedVarsInternal id) const {
            return hasPredefined[static_cast<int>(id)];
        }

        void setPredefinedVar(PreDefinedVarsInternal id, G4_Declare *dcl) {
            predefinedVars[static_cast<int>(id)] = dcl;
        }

        G4_Declare* getPreDefinedVar(PreDefinedVarsInternal id) const {
            if (id >= PreDefinedVarsInternal::VAR_LAST) {
                return nullptr;
            }
            return predefinedVars[static_cast<int>(id)];
        }
    private:
        // records whether a vISA pre-defined var is used by the kernel
        // some predefined need to be expanded (e.g., HWTid, color)
        bool hasPredefined[static_cast<int>(PreDefinedVarsInternal::VAR_LAST)];
        G4_Declare* predefinedVars[static_cast<int>(PreDefinedVarsInternal::VAR_LAST)];
    };

    bool hasNullReturnSampler = false;
    bool hasPerThreadProlog = false;
    // Have inserted two entires prolog for setting FFID for compute shaders
    bool hasComputeFFIDProlog = false;

    // FIXME: should not leak IGA IR into G4/vISA except in encoder
    const iga::Model* igaModel = nullptr;

    const CISA_IR_Builder* parentBuilder = nullptr;

    // stores all metadata ever allocated
    Mem_Manager            metadataMem;
    std::vector<Metadata*> allMDs;
    std::vector<MDNode*>   allMDNodes;

public:
    PreDefinedVars      preDefVars;
    Mem_Manager&        mem;        // memory for all operands and insts
    PhyRegPool          phyregpool; // all physical regs
    OperandHashTable    hashtable;  // all created region operands
    RegionPool          rgnpool;    // all region description
    DeclarePool         dclpool;    // all created decalres
    INST_LIST           instList;   // all created insts
    // list of instructions ever allocated
    // This list may only grow and is freed when IR_Builder is destroyed
    std::vector<G4_INST*> instAllocList;
    G4_Kernel&          kernel;
    // the following fileds are used for dcl name when a new dcl is created.
    // number of predefined variables are included.
    unsigned            num_temp_dcl;
    // number of temp GRF vars created to hold spilled addr/flag
    uint32_t            numAddrFlagSpillLoc = 0;
    std::vector<input_info_t*> m_inputVect;

    const Options* getOptions() const { return m_options; }
    bool           getOption(vISAOptions opt) const {return m_options->getOption(opt); }
    uint32_t       getuint32Option(vISAOptions opt) const { return m_options->getuInt32Option(opt); }
    void           getOption(vISAOptions opt, const char *&str) const {return m_options->getOption(opt, str); }
    void           addInputArg(input_info_t * inpt);
    input_info_t * getInputArg(unsigned int index);
    unsigned int   getInputCount();
    input_info_t * getRetIPArg();

    // what's the void* really?
    const void*    GetCurrentInst() const { return m_inst; };
    void           SetCurrentInst(const void* inst) { m_inst = inst; };

    const CISA_IR_Builder* getParent() const { return parentBuilder; }

    void dump(std::ostream &os); // not const because G4_INST::emit isn't :(

    std::stringstream& criticalMsgStream();

    const USE_DEF_ALLOCATOR& getAllocator() const { return useDefAllocator; }

    enum SubRegs_SP_FP
    {
        FE_SP = 0, // Can be either :ud or :uq
        FE_FP = 1, // Can be either :ud or :uq
        BE_SP = 6, // :ud
        BE_FP = 7 // :ud
    };

    // Getter/setter for be_sp and be_fp
    G4_Declare* getBESP()
    {
        if (be_sp == NULL)
        {
            be_sp = createDeclareNoLookup("be_sp", G4_GRF, 1, 1, Type_UD);
            be_sp->getRegVar()->setPhyReg(phyregpool.getGreg(kernel.getFPSPGRF()), SubRegs_SP_FP::BE_SP);
        }

        return be_sp;
    }

    G4_Declare* getBEFP()
    {
        if (be_fp == NULL)
        {
            be_fp = createDeclareNoLookup("be_fp", G4_GRF, 1, 1, Type_UD);
            be_fp->getRegVar()->setPhyReg(phyregpool.getGreg(kernel.getFPSPGRF()), SubRegs_SP_FP::BE_FP);
        }

        return be_fp;
    }

    G4_Declare* getStackCallArg() const {
        return preDefVars.getPreDefinedVar(PreDefinedVarsInternal::ARG);
    }
    G4_Declare* getStackCallRet() const {
        return preDefVars.getPreDefinedVar(PreDefinedVarsInternal::RET);
    }

    G4_Declare* getFE_SP() const {
        return preDefVars.getPreDefinedVar(PreDefinedVarsInternal::FE_SP);
    }

    G4_Declare* getFE_FP() const {
        return preDefVars.getPreDefinedVar(PreDefinedVarsInternal::FE_FP);
    }

    bool isPreDefArg(G4_Declare* dcl) const {
        return dcl == getStackCallArg();
    }

    bool isPreDefRet(G4_Declare* dcl) const {
        return dcl == getStackCallRet();
    }

    bool isPreDefFEStackVar(G4_Declare* dcl) const {
        return dcl == getFE_SP() || dcl == getFE_FP();
    }

    // this refers to vISA's internal stack for spill and caller/callee-save
    // Note that this is only valid after CFG is constructed
    // ToDo: make this a pass?
    bool usesStack() const {
        return kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc();
    }

    void bindInputDecl(G4_Declare* dcl, int grfOffset);

    // FIXME: Why is this needed, should not leak IGA internals into vISA!
    const iga::Model* getIGAModel() const { return igaModel; }
    uint32_t getPerThreadInputSize() const
    {
        return kernel.getInt32KernelAttr(Attributes::ATTR_PerThreadInputSize);
    }

    bool getHasPerThreadProlog() const { return hasPerThreadProlog; }
    void setHasPerThreadProlog() { hasPerThreadProlog = true; }

    bool getHasComputeFFIDProlog() const { return hasComputeFFIDProlog; }
    void setHasComputeFFIDProlog() { hasComputeFFIDProlog = true; }

    //
    // Check if opnd is or can be made "alignByte"-byte aligned.
    // These functions will change the underlying variable's alignment
    // (e.g., make a scalar variable GRF-aligned) when possible to satisfy
    // the alignment
    bool isOpndAligned(G4_Operand* opnd, int alignByte) const;
    bool isOpndAligned(G4_Operand *opnd, unsigned short &offset, int align_byte) const;

    void setIsKernel(bool value) { isKernel = value; }
    bool getIsKernel() const { return isKernel; }
    void predefinedVarRegAssignment(uint8_t inputSize);
    void expandPredefinedVars();
    void setArgSize(unsigned short size) { arg_size = size; }
    unsigned short getArgSize() const { return arg_size; }
    void setRetVarSize(unsigned short size) { return_var_size = size; }
    unsigned short getRetVarSize() const { return return_var_size; }

    FCPatchingInfo* getFCPatchInfo();
    void setFCPatchInfo(FCPatchingInfo* instance) { fcPatchInfo = instance; }

    const PWA_TABLE getPWaTable() const { return m_pWaTable; }

    const char* getNameString(Mem_Manager& mem, size_t size, const char* format, ...);

    G4_Predicate_Control vISAPredicateToG4Predicate(
        VISA_PREDICATE_CONTROL control, G4_ExecSize execSize);

    G4_FCALL* getFcallInfo(G4_INST* inst) const;

    // If this is true (detected in TranslateInterface.cpp), we need a sampler flush before EOT
    bool getHasNullReturnSampler() const { return hasNullReturnSampler; }

    //    Initializes predefined vars for all the vISA versions
    void createPreDefinedVars();

    void createBuiltinDecls();

    G4_Declare* getSpillFillHeader();

    G4_Declare* getOldA0Dot2Temp();
    bool hasValidOldA0Dot2() { return oldA0Dot2Temp; }

    IR_Builder(
        TARGET_PLATFORM genPlatform,
        INST_LIST_NODE_ALLOCATOR &alloc,
        G4_Kernel &k,
        Mem_Manager &m,
        Options *options,
        CISA_IR_Builder* parent,
        FINALIZER_INFO *jitInfo,
        PWA_TABLE pWaTable);

    ~IR_Builder();

    void rebuildPhyRegPool(unsigned int numRegisters) {
        phyregpool.rebuildRegPool(mem, numRegisters);
    }

    TARGET_PLATFORM getPlatform() const {return platform;}
    FINALIZER_INFO* getJitInfo() {return metaData;}
    CompilerStats &getcompilerStats() {return compilerStats;}

    G4_Declare* createDeclareNoLookup(
        const char*    name,
        G4_RegFileKind regFile,
        unsigned short n_elems,
        unsigned short n_rows,
        G4_Type        ty,
        DeclareType    kind = Regular,
        G4_RegVar *    base = NULL,
        G4_Operand *   repRegion = NULL,
        G4_ExecSize    execSize = G4_ExecSize(0));


    G4_Declare* createPreVarDeclareNoLookup(
        PreDefinedVarsInternal index,
        unsigned short n_elems,
        unsigned short n_rows,
        G4_Type        ty)
    {
        G4_Declare* dcl = dclpool.createPreVarDeclare(index, n_elems, n_rows, ty);
        kernel.Declares.push_back(dcl);
        return dcl;
    }

    G4_Declare* getBuiltinR0() {return builtinR0;}
    G4_Declare* getRealR0() const {return realR0;} // undefined terminology: what's "real" here (vs "builtin" above)?
    G4_Declare* getBuiltinA0() {return builtinA0;}
    G4_Declare* getBuiltinA0Dot2() {return builtinA0Dot2;}
    G4_Declare* getBuiltinHWTID() const {return builtinHWTID;}
    G4_Declare* getBuiltinT252() const {return builtinT252;}
    G4_Declare* getBuiltinBindlessSampler() const {return builtinBindlessSampler; }
    G4_Declare* getBuiltinSamplerHeader() const { return builtinSamplerHeader; }
    G4_Declare* getOldA0Dot2Temp() const { return oldA0Dot2Temp; }

    bool isBindlessSampler(const G4_Operand* sampler) const {
        return sampler->isSrcRegRegion() && sampler->getTopDcl() == getBuiltinBindlessSampler();
    }

    bool isBindlessSurface(const G4_Operand* bti) const {
        return bti->isSrcRegRegion() && bti->getTopDcl() == getBuiltinT252();
    }

    // IsSLMSurface - Check whether the given surface is SLM surface.
    static bool IsSLMSurface(const G4_Operand *surface) {
        // So far, it's only reliable to check an immediate surface.
        return surface->isImm() && surface->asImm()->getImm() == PREDEF_SURF_0;
    }


    static const G4_Declare *getDeclare(const G4_Operand *opnd) {
        const G4_Declare *dcl = opnd->getBase()->asRegVar()->getDeclare();

        while (const G4_Declare *parentDcl = dcl->getAliasDeclare())
            dcl = parentDcl;

        return dcl;
    }
    static G4_Declare *getDeclare(G4_Operand *opnd) {
        return const_cast<G4_Declare *>(getDeclare((const G4_Operand *)opnd));
    }

    bool shouldForceSplitSend(const G4_Operand* surface) const
    {
        return surface->isSrcRegRegion() &&
            surface->asSrcRegRegion()->getBase()->asRegVar()->getDeclare() == getBuiltinT252();
    }

    /// getSplitEMask() calculates the new mask after splitting from the current
    /// execution mask at the given execution size.
    /// It only works with masks covering whole GRF and thus won't generate/consume
    /// nibbles.
    static uint32_t getSplitEMask(unsigned execSize, uint32_t eMask, bool isLo);
    static uint32_t getSplitLoEMask(unsigned execSize, uint32_t eMask) {
        return getSplitEMask(execSize, eMask, true);
    }
    static uint32_t getSplitHiEMask(unsigned execSize, uint32_t eMask) {
        return getSplitEMask(execSize, eMask, false);
    }


    // create a new temp GRF with the specified type/size and undefined regions
    G4_Declare* createTempVar(
        unsigned int numElements, G4_Type type, G4_SubReg_Align subAlign,
        const char* prefix = "TV", bool appendIdToName = true);


    // create a new temp GRF as the home location of a spilled addr/flag dcl
    G4_Declare* createAddrFlagSpillLoc(G4_Declare* dcl);


    // like the above, but also mark the variable as don't spill
    // this is used for temp variables in macro sequences where spilling woul not help
    // FIXME: can we somehow merge this with G4_RegVarTmp/G4_RegVarTransient?
    G4_Declare* createTempVarWithNoSpill(
        unsigned int numElements, G4_Type type, G4_SubReg_Align subAlign,
        const char* prefix = "TV")
    {
        G4_Declare* dcl = createTempVar(numElements, type, subAlign, prefix);
        dcl->setDoNotSpill();
        return dcl;
    }

    //
    // Create a declare that is hardwired to some phyiscal GRF.
    // It is useful to implement various workarounds post RA where we want to directly
    // address some physical GRF.
    // regOff is in unit of the declare type.
    // caller is responsible for ensuring the resulting variable does not violate any HW restrictions
    // (e.g., operand does not cross two GRF)
    G4_Declare* createHardwiredDeclare(
        uint32_t numElements, G4_Type type, uint32_t regNum, uint32_t regOff);

    G4_INST* createPseudoKills(std::initializer_list<G4_Declare*> dcls, PseudoKillType ty);

    G4_INST* createPseudoKill(G4_Declare* dcl, PseudoKillType ty);

    // numRows is in hword units
    // offset is in hword units
    G4_INST* createSpill(
        G4_DstRegRegion* dst, G4_SrcRegRegion* header, G4_SrcRegRegion* payload,
        G4_ExecSize execSize,
        uint16_t numRows, uint32_t offset, G4_Declare* fp, G4_InstOption option);

    G4_INST* createSpill(
        G4_DstRegRegion* dst, G4_SrcRegRegion* payload,
        G4_ExecSize execSize, uint16_t numRows, uint32_t offset,
        G4_Declare* fp, G4_InstOption option);


    G4_INST* createFill(
        G4_SrcRegRegion* header,
        G4_DstRegRegion* dstData, G4_ExecSize execSize,
        uint16_t numRows, uint32_t offset, G4_Declare* fp, G4_InstOption option);
    G4_INST* createFill(
        G4_DstRegRegion* dstData, G4_ExecSize execSize,
        uint16_t numRows, uint32_t offset, G4_Declare* fp , G4_InstOption option);


    // numberOfFlags MEANS NUMBER OF WORDS (e.g., 1 means 16-bit), not number of bits or number of data elements in operands.
    G4_Declare* createTempFlag(unsigned short numberOfFlags, const char* prefix = "TEMP_FLAG_");

    // like above, but pass numFlagElements instead. This allows us to distinguish between 1/8/16-bit flags,
    // which are all allocated as a UW. name is allocated by caller
    G4_Declare* createFlag(uint16_t numFlagElements, const char* name);

    G4_Declare* createPreVar(
        PreDefinedVarsInternal preDefVar_index, unsigned short numElements, G4_Type type);

    //
    // create <vstride; width, hstride>
    //
    // PLEASE use getRegion* interface to get regions if possible!
    // This function will be mostly used for external regions.
    const RegionDesc* createRegionDesc(
        uint16_t vstride,
        uint16_t width,
        uint16_t hstride)
    {
        return rgnpool.createRegion(vstride, width, hstride);
    }

    // Given the execution size and region parameters, create a region
    // descriptor.
    //
    // PLEASE use getRegion* interface to get regions if possible!
    // This function will be mostly used for external regions.
    const RegionDesc *createRegionDesc(
        uint16_t execSize, uint16_t vstride, uint16_t width, uint16_t hstride)
    {
        // Performs normalization for commonly used regions.
        switch (RegionDesc::getRegionDescKind(execSize, vstride, width, hstride)) {
        case RegionDesc::RK_Stride0: return getRegionScalar();
        case RegionDesc::RK_Stride1: return getRegionStride1();
        case RegionDesc::RK_Stride2: return getRegionStride2();
        case RegionDesc::RK_Stride4: return getRegionStride4();
        default: break;
        }
        return rgnpool.createRegion(vstride, width, hstride);
    }

    /// Helper to normalize an existing region descriptor.
    const RegionDesc *getNormalizedRegion(uint16_t execSize, const RegionDesc *rd) {
        return createRegionDesc(execSize, rd->vertStride, rd->width, rd->horzStride);
    }

    /// Get the predefined region descriptors.
    const RegionDesc *getRegionScalar()  const { return &CanonicalRegionStride0; }
    const RegionDesc *getRegionStride1() const { return &CanonicalRegionStride1; }
    const RegionDesc *getRegionStride2() const { return &CanonicalRegionStride2; }
    const RegionDesc *getRegionStride4() const { return &CanonicalRegionStride4; }

    // ToDo: get rid of this version and use the message type specific ones below instead,
    // so we can avoid having to explicitly create extDesc bits
    G4_SendMsgDescriptor* createGeneralMsgDesc(
        uint32_t desc,
        uint32_t extDesc,
        SendAccess access,
        G4_Operand* bti = nullptr,
        G4_Operand* sti = nullptr);

    G4_SendMsgDescriptor* createReadMsgDesc(
        SFID sfid,
        uint32_t desc,
        G4_Operand* bti = nullptr);

    G4_SendMsgDescriptor* createWriteMsgDesc(
        SFID sfid,
        uint32_t desc,
        int src1Len,
        G4_Operand* bti = nullptr);

    G4_SendMsgDescriptor* createSyncMsgDesc(
        SFID sfid,
        uint32_t desc);

    G4_SendMsgDescriptor* createSampleMsgDesc(
        uint32_t desc,
        bool cps,
        int src1Len,
        G4_Operand* bti,
        G4_Operand* sti);

    static SendAccess getSendAccessType(bool isRead, bool isWrite) {
        if (isRead && isWrite)
        {
            return SendAccess::READ_WRITE;
        }
        return isRead ? SendAccess::READ_ONLY : SendAccess::WRITE_ONLY;
    }

    G4_SendMsgDescriptor* createSendMsgDesc(
        SFID sfid,
        uint32_t desc,
        uint32_t extDesc,
        int src1Len,
        SendAccess access,
        G4_Operand *bti,
        bool isValidFuncCtrl = true);

    G4_SendMsgDescriptor* createSendMsgDesc(
        unsigned funcCtrl,
        unsigned regs2rcv,
        unsigned regs2snd,
        SFID funcID,
        unsigned extMsgLength,
        uint16_t extFuncCtrl,
        SendAccess access,
        G4_Operand* bti = nullptr,
        G4_Operand* sti = nullptr);

    G4_Operand* emitSampleIndexGE16(
        G4_Operand* sampler, G4_Declare* headerDecl);

    //
    // deprecated, please use the one below
    //
    G4_SrcRegRegion* createSrcRegRegion(G4_SrcRegRegion& src)
    {
        G4_SrcRegRegion* rgn = new (mem)G4_SrcRegRegion(src);
        return rgn;
    }

    // Create a new srcregregion allocated in mem
    G4_SrcRegRegion* createSrcRegRegion(
        G4_VarBase* b,
        short roff,
        short sroff,
        const RegionDesc *rd,
        G4_Type ty,
        G4_AccRegSel regSel = ACC_UNDEFINED)
    {
        return createSrcRegRegion(Mod_src_undef, Direct, b, roff, sroff, rd, ty, regSel);
    }

    // Create a new srcregregion allocated in mem
    G4_SrcRegRegion* createSrcRegRegion(
        G4_SrcModifier m,
        G4_RegAccess   a,
        G4_VarBase*    b,
        short roff,
        short sroff,
        const RegionDesc *rd,
        G4_Type        ty,
        G4_AccRegSel regSel = ACC_UNDEFINED)
    {
        G4_SrcRegRegion* rgn = new (mem)G4_SrcRegRegion(m, a, b, roff, sroff, rd, ty, regSel);
        return rgn;
    }

    G4_SrcRegRegion* createSrcWithNewRegOff(G4_SrcRegRegion* old, short newRegOff);
    G4_SrcRegRegion* createSrcWithNewSubRegOff(G4_SrcRegRegion* old, short newSubRegOff);
    G4_SrcRegRegion* createSrcWithNewBase(G4_SrcRegRegion* old, G4_VarBase* newBase);

    G4_SrcRegRegion* createIndirectSrc(
        G4_SrcModifier m,
        G4_VarBase* b,
        short roff,
        short sroff,
        const RegionDesc* rd,
        G4_Type ty,
        short immAddrOff)
    {
        G4_SrcRegRegion* rgn = new (mem) G4_SrcRegRegion(m, IndirGRF, b, roff, sroff, rd, ty, ACC_UNDEFINED);
        rgn->setImmAddrOff(immAddrOff);
        return rgn;
    }

    //
    // deprecated, please use the version below
    //
    G4_DstRegRegion* createDstRegRegion(G4_DstRegRegion& dst)
    {
        G4_DstRegRegion* rgn = new (mem) G4_DstRegRegion(dst);
        return rgn;
    }

    // create a new dstregregion allocated in mem
    // TODO: Avoid calling this directly since direct dst and indirect dst
    // have different parameters. Will make it private in the future.
    G4_DstRegRegion* createDstRegRegion(
        G4_RegAccess a,
        G4_VarBase* b,
        short roff,
        short sroff,
        unsigned short hstride,
        G4_Type        ty,
        G4_AccRegSel regSel = ACC_UNDEFINED)
    {
        G4_DstRegRegion* rgn = new (mem) G4_DstRegRegion(a, b, roff, sroff, hstride, ty, regSel);
        return rgn;
    }

    // create a direct DstRegRegion
    G4_DstRegRegion* createDst(
        G4_VarBase* b,
        short roff,
        short sroff,
        unsigned short hstride,
        G4_Type        ty,
        G4_AccRegSel regSel = ACC_UNDEFINED)
    {
        return createDstRegRegion(Direct, b, roff, sroff, hstride, ty, regSel);
    }
    // create a direct DstRegRegion
    G4_DstRegRegion* createDst(G4_VarBase* b, G4_Type ty)
    {
        return createDstRegRegion(Direct, b, 0, 0, 1, ty, ACC_UNDEFINED);
    }

    // create a indirect DstRegRegion
    // b is the address variable, which only supports subreg offset
    G4_DstRegRegion* createIndirectDst(G4_VarBase* b,
        short sroff,
        uint16_t hstride,
        G4_Type ty,
        int16_t immOff)
    {
        auto dst = createDstRegRegion(IndirGRF, b, 0, sroff, hstride, ty);
        dst->setImmAddrOff(immOff);
        return dst;
    }

    G4_DstRegRegion* createDstWithNewSubRegOff(G4_DstRegRegion* old, short newSubRegOff);


    //
    // return the imm operand; create one if not yet created
    //
    G4_Imm* createImm(int64_t imm, G4_Type ty)
    {
        G4_Imm* i = hashtable.lookupImm(imm, ty);
        return (i != NULL)? i : hashtable.createImm(imm, ty);
    }

    //
    // return the float operand; create one if not yet created
    //
    G4_Imm* createImm(float fp);

    //
    // return the double operand; create one if not yet created
    //
    G4_Imm* createDFImm(double fp);


    // For integer immediates use a narrower type if possible
    // also change byte type to word type since HW does not support byte imm
    G4_Type getNewType(int64_t imm, G4_Type ty);

    //
    // return the imm operand with its lowest type(W or above); create one if not yet created
    //
    G4_Imm* createImmWithLowerType(int64_t imm, G4_Type ty)
    {
        G4_Type new_type = getNewType(imm, ty);
        G4_Imm* i = hashtable.lookupImm(imm, new_type);
        return (i != NULL)? i : hashtable.createImm(imm, new_type);
    }

    //
    // Create immediate operand without looking up hash table. This operand
    // is a relocatable immediate type.
    //
    G4_Reloc_Imm* createRelocImm(G4_Type ty)
    {
        G4_Reloc_Imm* newImm;
        newImm = new (mem)G4_Reloc_Imm(ty);
        return newImm;
    }

    //
    // Create immediate operand without looking up hash table. This operand
    // is a relocatable immediate type. Specify the value of this imm field,
    // which will present in the output instruction's imm value.
    //
    G4_Reloc_Imm* createRelocImm(int64_t immval, G4_Type ty)
    {
        G4_Reloc_Imm* newImm;
        newImm = new (mem)G4_Reloc_Imm(immval, ty);
        return newImm;
    }

    //
    // return the label operand; create one if not found
    //
    G4_Label* lookupLabel(char* lab)
    {
        G4_Label* l = hashtable.lookupLabel(lab);
        return l;
    }

    //
    // return the label operand; create one if not found.
    // a new copy of "lab" is created for the new label, so
    // caller does not have to allocate memory for lab
    //
    G4_Label* createLabel(std::string& lab, VISA_Label_Kind kind)
    {
        auto labStr = lab.c_str();

        G4_Label* l = hashtable.lookupLabel(labStr);
        return (l != NULL)? l : hashtable.createLabel(labStr);
    }

    G4_Predicate* createPredicate(
        G4_PredState s, G4_VarBase* flag, unsigned short srOff,
        G4_Predicate_Control ctrl = PRED_DEFAULT)
    {
        G4_Predicate* pred = new (mem)G4_Predicate(s, flag, srOff, ctrl);
        return pred;
    }

    G4_Predicate* createPredicate(G4_Predicate& prd)
    {
        G4_Predicate* p = new (mem) G4_Predicate(prd);
        return p;
    }

    G4_CondMod* createCondMod(G4_CondModifier m, G4_VarBase* flag, unsigned short off)
    {
        G4_CondMod* p = new (mem)G4_CondMod(m, flag, off);
        return p;
    }

    //
    // return the condition modifier; create one if not yet created
    //
    G4_CondMod* createCondMod(G4_CondMod& mod)
    {
        G4_CondMod* p = new (mem) G4_CondMod(mod);
        return p;
    }

    //
    // create register address expression normalized to (&reg +/- exp)
    //
    G4_AddrExp* createAddrExp(G4_RegVar* reg, int offset, G4_Type ty)
    {
        return new (mem) G4_AddrExp(reg, offset, ty);
    }

 private:
    // please leave all createInst() as private and use the public wrappers below
     // cond+sat+binary
     G4_INST* createInst(
         G4_Predicate* prd, G4_opcode op, G4_CondMod* mod, G4_Sat sat,
         G4_ExecSize size,
         G4_DstRegRegion* dst, G4_Operand* src0, G4_Operand* src1,
         G4_InstOpts options,
         bool addToInstList = true);
     // cond+sat+binary+line
     G4_INST* createInst(
         G4_Predicate* prd, G4_opcode op, G4_CondMod* mod, G4_Sat sat,
         G4_ExecSize size,
         G4_DstRegRegion* dst, G4_Operand* src0, G4_Operand* src1,
         G4_InstOpts options,
         int lineno, bool addToInstList = true);

     // TODO: remove
     // old template:
     // cond+sat+binary+line
     G4_INST* createInst(
         G4_Predicate* prd, G4_opcode op, G4_CondMod* mod, G4_Sat sat,
         int execSize,
         G4_DstRegRegion* dst, G4_Operand* src0, G4_Operand* src1,
         G4_InstOpts options,
         int lineno, bool addToInstList = true)
     {
         G4_ExecSize sz((unsigned char)execSize);
         return createInst(prd, op, mod, sat, sz, dst, src0, src1, options, lineno);
     }

     // TODO: remove
     // old template:
     // cond+sat+binary
     G4_INST* createInst(
         G4_Predicate* prd, G4_opcode op, G4_CondMod* mod, G4_Sat sat,
         int execSize,
         G4_DstRegRegion* dst, G4_Operand* src0, G4_Operand* src1,
         G4_InstOpts options,
         bool addToInstList = true)
     {
         G4_ExecSize sz((unsigned char)execSize);
         return createInst(prd, op, mod, sat, sz, dst, src0, src1, options);
     }

     // cond+sat+ternary
     G4_INST* createInst(
         G4_Predicate* prd, G4_opcode op, G4_CondMod* mod, G4_Sat sat,
         G4_ExecSize execSize,
         G4_DstRegRegion* dst, G4_Operand* src0, G4_Operand* src1, G4_Operand* src2,
         G4_InstOpts options,
         bool addToInstList = true);
     // cond+sat+ternary+line
     G4_INST* createInst(
         G4_Predicate* prd, G4_opcode op, G4_CondMod* mod, G4_Sat sat,
         G4_ExecSize execSize,
         G4_DstRegRegion* dst, G4_Operand* src0, G4_Operand* src1, G4_Operand* src2,
         G4_InstOpts options,
         int lineno, bool addToInstList = true);

     // TODO: remove
     // old template:
     // cond+sat+ternary
     G4_INST* createInst(
         G4_Predicate* prd, G4_opcode op, G4_CondMod* mod, G4_Sat sat,
         int execSize,
         G4_DstRegRegion* dst, G4_Operand* src0, G4_Operand* src1, G4_Operand* src2,
         G4_InstOpts option,
         bool addToInstList = true)
     {
         G4_ExecSize sz((unsigned char)execSize);
         return createInst(prd, op, mod, sat, sz, dst, src0, src1, src2, option);
     }

     // old template:
     // cond+sat+ternary+line
     G4_INST* createInst(
         G4_Predicate* prd, G4_opcode op, G4_CondMod* mod, G4_Sat sat,
         int execSize,
         G4_DstRegRegion* dst, G4_Operand* src0, G4_Operand* src1, G4_Operand* src2,
         G4_InstOpts option,
         int lineno, bool addToInstList = true)
     {
         G4_ExecSize sz((unsigned char)execSize);
         return createInst(prd, op, mod, sat, sz, dst, src0, src1, src2, option, lineno);
     }

public:

    G4_INST* createIf(G4_Predicate* prd, G4_ExecSize execSize, G4_InstOpts options);
    G4_INST* createElse(G4_ExecSize execSize, G4_InstOpts options);
    G4_INST* createEndif(G4_ExecSize execSize, G4_InstOpts options);
    G4_INST* createLabelInst(G4_Label* label, bool appendToInstList);
    G4_INST* createJmp(
        G4_Predicate* pred, G4_Operand* jmpTarget, G4_InstOpts options, bool appendToInstList);

    // ToDo: make createInternalInst() private as well and add wraper for them
    G4_INST* createInternalInst(
        G4_Predicate* prd, G4_opcode op, G4_CondMod* mod, G4_Sat sat,
        G4_ExecSize execSize,
        G4_DstRegRegion* dst, G4_Operand* src0, G4_Operand* src1,
        G4_InstOpts options);
    G4_INST* createInternalInst(
        G4_Predicate* prd, G4_opcode op, G4_CondMod* mod, G4_Sat sat,
        G4_ExecSize execSize,
        G4_DstRegRegion* dst, G4_Operand* src0, G4_Operand* src1,
        G4_InstOpts options,
        int lineno, int CISAoff, const char* srcFilename);

    G4_INST* createInternalInst(
        G4_Predicate* prd, G4_opcode op, G4_CondMod* mod, G4_Sat sat,
        G4_ExecSize execSize,
        G4_DstRegRegion* dst, G4_Operand* src0, G4_Operand* src1, G4_Operand* src2,
        G4_InstOpts options);
    G4_INST* createInternalInst(
        G4_Predicate* prd, G4_opcode op, G4_CondMod* mod, G4_Sat sat,
        G4_ExecSize execSize,
        G4_DstRegRegion* dst, G4_Operand* src0, G4_Operand* src1, G4_Operand* src2,
        G4_InstOpts options,
        int lineno, int CISAoff, const char* srcFilename);

    G4_INST* createCFInst(
        G4_Predicate* prd, G4_opcode op, G4_ExecSize execSize,
        G4_Label* jip, G4_Label* uip,
        G4_InstOpts options,
        int lineno = 0,
        bool addToInstList= true);
    G4_INST* createInternalCFInst(
        G4_Predicate* prd, G4_opcode op, G4_ExecSize execSize,
        G4_Label* jip, G4_Label* uip,
        G4_InstOpts options,
        int lineno = 0, int CISAoff = -1, const char* srcFilename = NULL);

    G4_InstSend* createSendInst(
        G4_Predicate* prd, G4_opcode op, G4_ExecSize execSize,
        G4_DstRegRegion* postDst, G4_SrcRegRegion* payload,
        G4_Operand* msg,
        G4_InstOpts options, // FIXME: re-order options to follow all operands
        G4_SendMsgDescriptor *msgDesc,
        int lineno = 0, bool addToInstList = true);
    G4_InstSend* createInternalSendInst(
        G4_Predicate* prd, G4_opcode op,
        G4_ExecSize execSize,
        G4_DstRegRegion* postDst, G4_SrcRegRegion* payload,
        G4_Operand* msg,
        G4_InstOpts options, // FIXME: re-order options to follow all operands
        G4_SendMsgDescriptor *msgDesc,
        int lineno = 0, int CISAoff = -1, const char* srcFilename = NULL);

    G4_InstSend* createSplitSendInst(
        G4_Predicate* prd, G4_opcode op,
        G4_ExecSize execSize,
        G4_DstRegRegion* dst, G4_SrcRegRegion* src1, G4_SrcRegRegion* src2,
        G4_Operand* msg, G4_InstOpts options,
        G4_SendMsgDescriptor *msgDesc,
        G4_Operand* src3,
        int lineno = 0,
        bool addToInstList = true);

    G4_InstSend* createInternalSplitSendInst(
        G4_Predicate* prd, G4_opcode op,
        G4_ExecSize execSize,
        G4_DstRegRegion* dst, G4_SrcRegRegion* src1, G4_SrcRegRegion* src2,
        // TODO: reorder parameters to put options last
        G4_Operand* msg, G4_InstOpts options,
        G4_SendMsgDescriptor *msgDesc,
        G4_Operand* src3,
        int lineno = 0, int CISAoff = -1, const char* srcFilename = NULL);

    G4_INST* createMathInst(
        G4_Predicate* prd, G4_Sat sat, G4_ExecSize execSize,
        G4_DstRegRegion* dst, G4_Operand* src0, G4_Operand* src1, G4_MathOp mathOp,
        G4_InstOpts options, int lineno = 0, bool addToInstList = true);

    G4_INST* createInternalMathInst(
        G4_Predicate* prd, G4_Sat sat, G4_ExecSize execSize,
        G4_DstRegRegion* dst, G4_Operand* src0, G4_Operand* src1, G4_MathOp mathOp,
        G4_InstOpts options,
        int lineno = 0, int CISAoff = -1, const char* srcFilename = NULL);

    G4_INST* createIntrinsicInst(
        G4_Predicate* prd, Intrinsic intrinId, G4_ExecSize execSize,
        G4_DstRegRegion* dst, G4_Operand* src0, G4_Operand* src1, G4_Operand* src2,
        G4_InstOpts options,
        int lineno = 0, bool addToInstList = true);

    G4_INST* createInternalIntrinsicInst(
        G4_Predicate* prd, Intrinsic intrinId, G4_ExecSize execSize,
        G4_DstRegRegion* dst, G4_Operand* src0, G4_Operand* src1, G4_Operand* src2,
        G4_InstOpts options,
        int lineno = 0, int CISAoff = -1, const char* srcFilename = nullptr);

    G4_INST* createNop(G4_InstOpts options);
    G4_INST* createSync(G4_opcode syncOp, G4_Operand* src);

    G4_INST* createMov(
        G4_ExecSize execSize,
        G4_DstRegRegion* dst, G4_Operand* src0,
        G4_InstOpts options,
        bool appendToInstList);

    G4_INST* createBinOp(
        G4_opcode op, G4_ExecSize execSize,
        G4_DstRegRegion* dst, G4_Operand* src0, G4_Operand* src1,
        G4_InstOpts options,
        bool appendToInstList)
    {
        return createBinOp(nullptr, op, execSize, dst, src0, src1, options, appendToInstList);
    }

    G4_INST* createBinOp(
        G4_Predicate *pred,
        G4_opcode op, G4_ExecSize execSize,
        G4_DstRegRegion* dst, G4_Operand* src0, G4_Operand* src1,
        G4_InstOpts options,
        bool appendToInstList = true);

    G4_INST* createMach(
        G4_ExecSize execSize,
        G4_DstRegRegion* dst, G4_Operand* src0, G4_Operand* src1,
        G4_InstOpts options, G4_Type accType);

    G4_INST* createMacl(
        G4_ExecSize execSize,
        G4_DstRegRegion* dst,
        G4_Operand* src0, G4_Operand* src1, G4_InstOpts options, G4_Type accType);

    static G4_MathOp Get_MathFuncCtrl(ISA_Opcode op, G4_Type type);

    void resizePredefinedStackVars();

    template <typename T>
    T*          duplicateOperand(T* opnd) {return static_cast<T *>(duplicateOpndImpl(opnd));}
    G4_Operand* duplicateOpndImpl(G4_Operand* opnd);


    G4_DstRegRegion *createSubDstOperand(
        G4_DstRegRegion* dst, uint16_t start, uint8_t size);
    G4_SrcRegRegion *createSubSrcOperand(
        G4_SrcRegRegion* src, uint16_t start, uint8_t size, uint16_t newVs, uint16_t newWd);
    G4_INST *makeSplittingInst(G4_INST *inst, G4_ExecSize execSize);

    G4_InstSend *Create_Send_Inst_For_CISA(
        G4_Predicate *pred,
        G4_DstRegRegion *postDst, G4_SrcRegRegion *payload,
        G4_ExecSize execSize,
        G4_SendMsgDescriptor *msgDesc,
        G4_InstOpts options,
        bool is_sendc);

    G4_InstSend *Create_SplitSend_Inst(
        G4_Predicate *pred,
        G4_DstRegRegion *dst, G4_SrcRegRegion *src1, G4_SrcRegRegion *src2,
        G4_ExecSize execSize,
        G4_SendMsgDescriptor *msgDesc,
        G4_InstOpts options,
        bool is_sendc);


    G4_InstSend *Create_SplitSend_Inst_For_RTWrite(
        G4_Predicate *pred,
        G4_DstRegRegion *dst,
        G4_SrcRegRegion *src1,
        G4_SrcRegRegion *src2,
        G4_SrcRegRegion *extDesc,
        G4_ExecSize execSize,
        G4_SendMsgDescriptor *msgDesc,
        G4_InstOpts option);

    G4_InstSend* Create_Send_Inst_For_CISA(
        G4_Predicate* pred,
        G4_DstRegRegion* postDst,
        G4_SrcRegRegion* payload,
        unsigned regs2snd,
        unsigned regs2rcv,
        G4_ExecSize execsize,
        unsigned fc,
        SFID tf_id,
        bool head_present,
        SendAccess access,
        G4_Operand* bti,
        G4_Operand* sti,
        G4_InstOpts options,
        bool is_sendc);

    G4_InstSend* Create_SplitSend_Inst_For_CISA(
        G4_Predicate* pred, G4_DstRegRegion* dst,
        G4_SrcRegRegion* src1, unsigned regs2snd1,
        G4_SrcRegRegion* src2, unsigned regs2snd2,
        unsigned regs2rcv,
        G4_ExecSize execSize,
        unsigned fc,
        SFID tf_id,
        bool head_present,
        SendAccess access,
        G4_Operand* bti, G4_Operand* sti,
        G4_InstOpts option,
        bool is_sendc);

    // helper functions
    G4_Declare *createSendPayloadDcl(unsigned num_elt, G4_Type type);
    void Create_MOVR0_Inst(
        G4_Declare* dcl,
        short refOff,
        short subregOff,
        bool use_nomask = false);
    void Create_MOV_Inst(
        G4_Declare* dcl,
        short refOff,
        short subregOff,
        G4_ExecSize execsize,
        G4_Predicate* pred,
        G4_CondMod* condMod,
        G4_Operand* src_opnd,
        bool use_nomask = false);
    void Create_ADD_Inst(
        G4_Declare* dcl,
        short         regOff,
        short         subregOff,
        G4_ExecSize   execSize,
        G4_Predicate* pred,
        G4_CondMod*   condMod,
        G4_Operand*   src0_opnd,
        G4_Operand*   src1_opnd,
        G4_InstOption options);
    void Create_MOV_Send_Src_Inst(
        G4_Declare* dcl,
        short refOff,
        short subregOff,
        unsigned num_dword,
        G4_Operand* src_opnd,
        G4_InstOpts options);



    // short hand for creating a dstRegRegion
    G4_DstRegRegion* Create_Dst_Opnd_From_Dcl(G4_Declare* dcl, unsigned short hstride);
    G4_SrcRegRegion* Create_Src_Opnd_From_Dcl(G4_Declare* dcl, const RegionDesc* rd);

    // Create a null dst with scalar region and the given type
    G4_DstRegRegion* createNullDst(G4_Type dstType);

    // Create a null src with scalar region and the given type
    G4_SrcRegRegion* createNullSrc(G4_Type dstType);

    G4_DstRegRegion* Check_Send_Dst(G4_DstRegRegion *dst_opnd);




    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // translateXXXXXXX functions translate specific vISA instructions into
    // sequences of G4 IR that implement the operation
    //
    // Implementations are split amongst various files based on category.
    // Comments below will point the user to the correct implementation file.
    // Please keep implementations the same sequential order as in the header.
    // (Feel free to re-order, but reflect the re-order here.)
    //
    // During refactor anything that was in TranslationInterface.cpp was moved
    // to one of this VisaToG4/Translate* files, but some methods have nothing
    // to do with vISA and make sense in the BuildIRImpl.cpp and could be
    // moved there.  Please move the declaration (prototype) upwards in
    // that case.

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Members related to general arithmetic/logic/shift ops are in
    // VisaToG4/TranslateALU.cpp
    int translateVISAAddrInst(
        ISA_Opcode opcode,
        VISA_Exec_Size execSize,
        VISA_EMask_Ctrl emask,
        G4_DstRegRegion *dst_opnd,
        G4_Operand *src0_opnd,
        G4_Operand *src1_opnd);

    int translateVISAArithmeticInst(
        ISA_Opcode opcode,
        VISA_Exec_Size execSize,
        VISA_EMask_Ctrl emask,
        G4_Predicate *predOpnd,
        G4_Sat saturate,
        G4_CondMod* condMod,
        G4_DstRegRegion *dstOpnd,
        G4_Operand *src0Opnd,
        G4_Operand *src1Opnd,
        G4_Operand *src2Opnd,
        G4_DstRegRegion *carryBorrow);




    int translateVISACompareInst(
        ISA_Opcode opcode,
        VISA_Exec_Size execSize,
        VISA_EMask_Ctrl emask,
        VISA_Cond_Mod relOp,
        G4_Declare* predDst,
        G4_Operand *src0_opnd,
        G4_Operand *src1_opnd);

    int translateVISACompareInst(
        ISA_Opcode opcode,
        VISA_Exec_Size execSize,
        VISA_EMask_Ctrl emask,
        VISA_Cond_Mod relOp,
        G4_DstRegRegion *dstOpnd,
        G4_Operand *src0Opnd,
        G4_Operand *src1Opnd);

    int translateVISALogicInst(
        ISA_Opcode opcode,
        G4_Predicate *pred_opnd,
        G4_Sat saturate,
        VISA_Exec_Size executionSize,
        VISA_EMask_Ctrl emask,
        G4_DstRegRegion* dst,
        G4_Operand* src0,
        G4_Operand* src1,
        G4_Operand* src2,
        G4_Operand* src3);

    int translateVISADataMovementInst(
        ISA_Opcode opcode,
        CISA_MIN_MAX_SUB_OPCODE subOpcode,
        G4_Predicate *pred_opnd,
        VISA_Exec_Size executionSize,
        VISA_EMask_Ctrl emask,
        G4_Sat saturate,
        G4_DstRegRegion *dst,
        G4_Operand *src0,
        G4_Operand *src1);

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Control flow, function call, and branching ops are located in
    // VisaToG4/TranslateBranch.cpp
    int translateVISACFSwitchInst(
        G4_Operand *indexOpnd,
        uint8_t numLabels,
        G4_Label** lab);

    int translateVISACFLabelInst(G4_Label* lab);

    int translateVISACFCallInst(
        VISA_Exec_Size execsize,
        VISA_EMask_Ctrl emask,
        G4_Predicate *predOpnd,
        G4_Label* lab);

    int translateVISACFJumpInst(G4_Predicate *predOpnd, G4_Label* lab);

    int translateVISACFFCallInst(
        VISA_Exec_Size execsize,
        VISA_EMask_Ctrl emask,
        G4_Predicate *predOpnd,
        std::string funcName,
        uint8_t argSize,
        uint8_t returnSize);

    int translateVISACFIFCallInst(
        VISA_Exec_Size execsize,
        VISA_EMask_Ctrl emask,
        G4_Predicate *predOpnd,
        G4_Operand* funcAddr,
        uint8_t argSize,
        uint8_t returnSize);

    int translateVISACFSymbolInst(
        const std::string& symbolName,
        G4_DstRegRegion* dst);

    int translateVISACFFretInst(
        VISA_Exec_Size execsize,
        VISA_EMask_Ctrl emask,
        G4_Predicate *predOpnd);

    int translateVISACFRetInst(
        VISA_Exec_Size execsize,
        VISA_EMask_Ctrl emask,
        G4_Predicate *predOpnd);

    int translateVISAGotoInst(
        G4_Predicate *predOpnd,
        VISA_Exec_Size executionSize,
        VISA_EMask_Ctrl emask,
        G4_Label *label);


    ///////////////////////////////////////////////////////////////////////////
    // members related to special math sequences VisaToG4/TranslateMath.cpp
    void expandFdiv(
        G4_ExecSize exsize, G4_Predicate *predOpnd, G4_Sat saturate,
        G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd, G4_Operand *src1Opnd,
        uint32_t instOpt);

    void expandPow(
        G4_ExecSize exsize, G4_Predicate *predOpnd, G4_Sat saturate,
        G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd, G4_Operand *src1Opnd,
        uint32_t instOpt);


    int translateVISAArithmeticDoubleInst(
        ISA_Opcode opcode,
        VISA_Exec_Size execSize,
        VISA_EMask_Ctrl emask,
        G4_Predicate *predOpnd,
        G4_Sat saturate,
        G4_DstRegRegion *dstOpnd,
        G4_Operand *src0Opnd,
        G4_Operand *src1Opnd);

    int translateVISAArithmeticSingleDivideIEEEInst(
        ISA_Opcode opcode,
        VISA_Exec_Size execSize,
        VISA_EMask_Ctrl emask,
        G4_Predicate *predOpnd,
        G4_Sat saturate,
        G4_CondMod* condMod,
        G4_DstRegRegion *dstOpnd,
        G4_Operand *src0Opnd,
        G4_Operand *src1Opnd);

    int translateVISAArithmeticSingleSQRTIEEEInst(
        ISA_Opcode opcode,
        VISA_Exec_Size execSize,
        VISA_EMask_Ctrl emask,
        G4_Predicate *predOpnd,
        G4_Sat saturate,
        G4_CondMod* condMod,
        G4_DstRegRegion *dstOpnd,
        G4_Operand *src0Opnd);

    int translateVISAArithmeticDoubleSQRTInst(
        ISA_Opcode opcode,
        VISA_Exec_Size execSize,
        VISA_EMask_Ctrl emask,
        G4_Predicate *predOpnd,
        G4_Sat saturate,
        G4_CondMod* condMod,
        G4_DstRegRegion *dstOpnd,
        G4_Operand *src0Opnd);


    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Members related miscellaneous instructions that don't fit any other
    // category are in VisaToG4/TranslateMisc.cpp
    //
    // (As stated above some of this could move to BuildIRImpl.cpp.)
    static bool isNoMask(VISA_EMask_Ctrl eMask);
    static G4_ExecSize toExecSize(VISA_Exec_Size execSize);

    VISA_Exec_Size roundUpExecSize(VISA_Exec_Size execSize);

    G4_Declare* getImmDcl(G4_Imm* val, int numElt);

    struct PayloadSource {
        G4_SrcRegRegion  *opnd;
        G4_ExecSize       execSize;
        G4_InstOpts       instOpt;
    };

    /// preparePayload - This method prepares payload from the specified header
    /// and sources.
    ///
    /// \param msgs             Message(s) prepared. That 2-element array must
    ///                         be cleared before calling preparePayload().
    /// \param sizes            Size(s) (in GRF) of each message prepared. That
    ///                         2-element array must be cleared before calling
    ///                         preparePayload().
    /// \param batchExSize      When it's required to copy sources, batchExSize
    ///                         specifies the SIMD width of copy.
    /// \param splitSendEnabled Whether feature split-send is available. When
    ///                         feature split-send is available, this function
    ///                         will check whether two consecutive regions
    ///                         could be prepared instead of one to take
    ///                         advantage of split-send.
    /// \param srcs             The array of sources (including header if
    ///                         present).
    /// \param len              The length of the array of sources.
    ///
    void preparePayload(
        G4_SrcRegRegion *msgs[2], unsigned sizes[2],
        G4_ExecSize batchExSize, bool splitSendEnabled,
        PayloadSource sources[], unsigned len);

    // Coalesce multiple payloads into a single region.  Pads each region with
    // an optional alignment argument (e.g. a GRF size).  The source region
    // sizes are determined by source dimension, so use an alias if you are
    // using a subregion.  All copies are made under no mask semantics using
    // the maximal SIMD width for the current device.
    //
    // A second alignment option allows a caller to align the full payload
    // to some total.
    //
    // If all parameters are nullptr or the null register, we return the null
    // register.
    //
    // Some examples:
    //
    // 1. coalescePayloads(GRF_SIZE,GRF_SIZE,...);
    //    Coalesces each source into a single region.  Each source is padded
    //    out to a full GRF, and the sum total result is also padded out to
    //    a full GRF.
    //
    // 2. coalescePayloads(1,GRF_SIZE,...);
    //    Coalesces each source into a single region packing each source
    //    together, but padding the result.  E.g. one could copy a QW and then
    //    a DW and pad the result out to a GRF.
    //
    G4_SrcRegRegion *coalescePayload(
        unsigned alignSourcesTo,
        unsigned alignPayloadTo,
        uint32_t payloadSize,
        uint32_t srcSize,
        std::initializer_list<G4_SrcRegRegion *> srcs,
        VISA_EMask_Ctrl emask);

    // emask is InstOption
    void Copy_SrcRegRegion_To_Payload(
        G4_Declare* payload,
        unsigned int& regOff,
        G4_SrcRegRegion* src,
        G4_ExecSize execSize,
        uint32_t emask);
    unsigned int getByteOffsetSrcRegion(G4_SrcRegRegion* srcRegion);

    // only used in TranslateSend3D, maybe consider moving there if no
    // one else uses them.
    bool checkIfRegionsAreConsecutive(
        G4_SrcRegRegion* first,
        G4_SrcRegRegion* second,
        G4_ExecSize execSize);
    bool checkIfRegionsAreConsecutive(
        G4_SrcRegRegion* first,
        G4_SrcRegRegion* second,
        G4_ExecSize execSize,
        G4_Type type);

    int generateDebugInfoPlaceholder(); // TODO: move to BuildIRImpl.cpp?

    // legitimiately belongs in Misc
    int translateVISALifetimeInst(unsigned char properties, G4_Operand* var);


    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // members related to 3D and sampler ops are in VisaToG4/TranslateSend3D.cpp
    int translateVISASampleInfoInst(
        VISA_Exec_Size executionSize,
        VISA_EMask_Ctrl emask,
        ChannelMask chMask,
        G4_Operand* surface,
        G4_DstRegRegion* dst);

    int translateVISAResInfoInst(
        VISA_Exec_Size executionSize,
        VISA_EMask_Ctrl emask,
        ChannelMask chMask,
        G4_Operand* surface,
        G4_SrcRegRegion* lod,
        G4_DstRegRegion* dst);

    int translateVISAURBWrite3DInst(
        G4_Predicate* pred,
        VISA_Exec_Size executionSize,
        VISA_EMask_Ctrl emask,
        uint8_t numOut,
        uint16_t globalOffset,
        G4_SrcRegRegion* channelMask,
        G4_SrcRegRegion* urbHandle,
        G4_SrcRegRegion* perSlotOffset,
        G4_SrcRegRegion* vertexData);

    int translateVISARTWrite3DInst(
        G4_Predicate* pred,
        VISA_Exec_Size executionSize,
        VISA_EMask_Ctrl emask,
        G4_Operand *surface,
        G4_SrcRegRegion *r1HeaderOpnd,
        G4_Operand *rtIndex,
        vISA_RT_CONTROLS cntrls,
        G4_SrcRegRegion *sampleIndexOpnd,
        G4_Operand *cpsCounter,
        unsigned int numParms,
        G4_SrcRegRegion ** msgOpnds);


    int splitSampleInst(
        VISASampler3DSubOpCode actualop,
        bool pixelNullMask,
        bool cpsEnable,
        G4_Predicate* pred,
        ChannelMask srcChannel,
        int numChannels,
        G4_Operand *aoffimmi,
        G4_Operand *sampler,
        G4_Operand *surface,
        G4_DstRegRegion* dst,
        VISA_EMask_Ctrl emask,
        bool useHeader,
        unsigned numRows, // msg length for each simd8
        unsigned int numParms,
        G4_SrcRegRegion ** params,
        bool uniformSampler = true);

    void doSamplerHeaderMove(G4_Declare* header, G4_Operand* sampler);
    G4_Declare* getSamplerHeader(bool isBindlessSampler, bool samplerIndexGE16);
    uint32_t getSamplerResponseLength(
        int numChannels, bool isFP16, int execSize, bool pixelNullMask, bool nullDst);

    int translateVISASampler3DInst(
        VISASampler3DSubOpCode actualop,
        bool pixelNullMask,
        bool cpsEnable,
        bool uniformSampler,
        G4_Predicate* pred,
        VISA_Exec_Size executionSize,
        VISA_EMask_Ctrl emask,
        ChannelMask srcChannel,
        G4_Operand* aoffimmi,
        G4_Operand *sampler,
        G4_Operand *surface,
        G4_DstRegRegion* dst,
        unsigned int numParms,
        G4_SrcRegRegion ** params);

    int translateVISALoad3DInst(
        VISASampler3DSubOpCode actualop,
        bool pixelNullMask,
        G4_Predicate *pred,
        VISA_Exec_Size exeuctionSize,
        VISA_EMask_Ctrl em,
        ChannelMask channelMask,
        G4_Operand* aoffimmi,
        G4_Operand* surface,
        G4_DstRegRegion* dst,
        uint8_t numOpnds,
        G4_SrcRegRegion ** opndArray);

    int translateVISAGather3dInst(
        VISASampler3DSubOpCode actualop,
        bool pixelNullMask,
        G4_Predicate* pred,
        VISA_Exec_Size exeuctionSize,
        VISA_EMask_Ctrl em,
        ChannelMask channelMask,
        G4_Operand* aoffimmi,
        G4_Operand* sampler,
        G4_Operand* surface,
        G4_DstRegRegion* dst,
        unsigned int numOpnds,
        G4_SrcRegRegion ** opndArray);

    int translateVISASamplerNormInst(
        G4_Operand* surface,
        G4_Operand* sampler,
        ChannelMask channel,
        unsigned numEnabledChannels,
        G4_Operand* deltaUOpnd,
        G4_Operand* uOffOpnd,
        G4_Operand* deltaVOpnd,
        G4_Operand* vOffOpnd,
        G4_DstRegRegion* dst_opnd);

    int translateVISASamplerInst(
        unsigned simdMode,
        G4_Operand* surface,
        G4_Operand* sampler,
        ChannelMask channel,
        unsigned numEnabledChannels,
        G4_Operand* uOffOpnd,
        G4_Operand* vOffOpnd,
        G4_Operand* rOffOpnd,
        G4_DstRegRegion* dstOpnd);

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // basic load and store (type and untyped) are located in
    // VisaToG4/TranslateSendLdStLegacy.cpp

    // IsHeaderOptional - Check whether the message header is optional.
    bool isMessageHeaderOptional(G4_Operand *surface, G4_Operand *Offset) const;

    // IsStatelessSurface - Check whether the give surface is statelesss surface.
    static bool isStatelessSurface(const G4_Operand *surface) {
        // So far, it's only reliable to check an immediate surface.
        return surface->isImm() &&
            (surface->asImm()->getImm() == PREDEF_SURF_255 || surface->asImm()->getImm() == PREDEF_SURF_253);
    }


    uint32_t setOwordForDesc(uint32_t desc, int numOword, bool isSLM = false) const;
    int translateVISAOwordLoadInst(
        ISA_Opcode opcode,
        bool modified,
        G4_Operand* surface,
        VISA_Oword_Num size,
        G4_Operand* offOpnd,
        G4_DstRegRegion* dstOpnd);

    int translateVISAOwordStoreInst(
        G4_Operand* surface,
        VISA_Oword_Num size,
        G4_Operand* offOpnd,
        G4_SrcRegRegion* srcOpnd);

    int translateVISAGatherInst(
        VISA_EMask_Ctrl emask,
        bool modified,
        GATHER_SCATTER_ELEMENT_SIZE eltSize,
        VISA_Exec_Size executionSize,
        G4_Operand* surface,
        G4_Operand* gOffOpnd,
        G4_SrcRegRegion* eltOFfOpnd,
        G4_DstRegRegion* dstOpnd);

    int translateVISAScatterInst(
        VISA_EMask_Ctrl emask,
        GATHER_SCATTER_ELEMENT_SIZE eltSize,
        VISA_Exec_Size executionSize,
        G4_Operand* surface,
        G4_Operand* gOffOpnd,
        G4_SrcRegRegion* eltOffOpnd,
        G4_SrcRegRegion* srcOpnd);

    int translateVISAGather4Inst(
        VISA_EMask_Ctrl emask,
        bool modified,
        ChannelMask chMask,
        VISA_Exec_Size executionSize,
        G4_Operand* surface,
        G4_Operand* gOffOpnd,
        G4_SrcRegRegion* eltOffOpnd,
        G4_DstRegRegion* dstOpnd);

    int translateVISAScatter4Inst(
        VISA_EMask_Ctrl emask,
        ChannelMask chMask,
        VISA_Exec_Size executionSize,
        G4_Operand* surface,
        G4_Operand* gOffOpnd,
        G4_SrcRegRegion* eltOffOpnd,
        G4_SrcRegRegion* srcOpnd);

    int translateVISADwordAtomicInst(
        VISAAtomicOps    subOpc,
        bool             is16Bit,
        G4_Predicate    *pred,
        VISA_Exec_Size   execSize,
        VISA_EMask_Ctrl  eMask,
        G4_Operand*      surface,
        G4_SrcRegRegion* offsets,
        G4_SrcRegRegion* src0,
        G4_SrcRegRegion* src1,
        G4_DstRegRegion* dst);

    void buildTypedSurfaceAddressPayload(
        G4_SrcRegRegion* u, G4_SrcRegRegion* v, G4_SrcRegRegion* r, G4_SrcRegRegion* lod,
        G4_ExecSize execSize, G4_InstOpts instOpt, PayloadSource sources[], uint32_t& len);

    int translateVISAGather4TypedInst(
        G4_Predicate   *pred,
        VISA_EMask_Ctrl emask,
        ChannelMask     chMask,
        G4_Operand     *surfaceOpnd,
        VISA_Exec_Size  executionSize,
        G4_SrcRegRegion *uOffsetOpnd,
        G4_SrcRegRegion *vOffsetOpnd,
        G4_SrcRegRegion *rOffsetOpnd,
        G4_SrcRegRegion *lodOpnd,
        G4_DstRegRegion *dstOpnd);

    int translateVISAScatter4TypedInst(
        G4_Predicate           *pred,
        VISA_EMask_Ctrl emask,
        ChannelMask chMask,
        G4_Operand *surfaceOpnd,
        VISA_Exec_Size executionSize,
        G4_SrcRegRegion *uOffsetOpnd,
        G4_SrcRegRegion *vOffsetOpnd,
        G4_SrcRegRegion *rOffsetOpnd,
        G4_SrcRegRegion *lodOpnd,
        G4_SrcRegRegion *srcOpnd);

    int translateVISATypedAtomicInst(
        VISAAtomicOps    atomicOp,
        bool             is16Bit,
        G4_Predicate    *pred,
        VISA_EMask_Ctrl  emask,
        VISA_Exec_Size   execSize,
        G4_Operand      *surface,
        G4_SrcRegRegion *uOffsetOpnd,
        G4_SrcRegRegion *vOffsetOpnd,
        G4_SrcRegRegion *rOffsetOpnd,
        G4_SrcRegRegion *lodOpnd,
        G4_SrcRegRegion *src0,
        G4_SrcRegRegion *src1,
        G4_DstRegRegion *dst);

    void applySideBandOffset(G4_Operand* sideBand, G4_SendMsgDescriptor* sendMsgDesc);

    int translateVISASLMUntypedScaledInst(
        bool                   isRead,
        G4_Predicate          *pred,
        VISA_Exec_Size         execSize,
        VISA_EMask_Ctrl        eMask,
        ChannelMask            chMask,
        uint16_t               scale,
        G4_Operand             *globalOffset,
        G4_SrcRegRegion        *offsets,
        G4_Operand             *srcOrDst);

    int translateVISAGather4ScaledInst(
        G4_Predicate *pred,
        VISA_Exec_Size execSize,
        VISA_EMask_Ctrl eMask,
        ChannelMask chMask,
        G4_Operand *surface,
        G4_Operand *globalOffset,
        G4_SrcRegRegion *offsets,
        G4_DstRegRegion *dst);

    int translateVISAScatter4ScaledInst(
        G4_Predicate *pred,
        VISA_Exec_Size execSize,
        VISA_EMask_Ctrl eMask,
        ChannelMask chMask,
        G4_Operand *surface,
        G4_Operand *globalOffset,
        G4_SrcRegRegion *offsets,
        G4_SrcRegRegion *src);

    int translateGather4Inst(
        G4_Predicate *pred,
        VISA_Exec_Size execSize,
        VISA_EMask_Ctrl eMask,
        ChannelMask chMask,
        G4_Operand *surface,
        G4_Operand *globalOffset,
        G4_SrcRegRegion *offsets,
        G4_DstRegRegion *dst);

    int translateScatter4Inst(
        G4_Predicate *pred,
        VISA_Exec_Size execSize,
        VISA_EMask_Ctrl eMask,
        ChannelMask chMask,
        G4_Operand *surface,
        G4_Operand *globalOffset,
        G4_SrcRegRegion *offsets,
        G4_SrcRegRegion *src);

    int translateVISAGatherScaledInst(
        G4_Predicate *pred,
        VISA_Exec_Size execSize,
        VISA_EMask_Ctrl eMask,
        VISA_SVM_Block_Num numBlocks,
        G4_Operand *surface,
        G4_Operand *globalOffset,
        G4_SrcRegRegion *offsets,
        G4_DstRegRegion *dst);

    int translateVISAScatterScaledInst(
        G4_Predicate *pred,
        VISA_Exec_Size execSize,
        VISA_EMask_Ctrl eMask,
        VISA_SVM_Block_Num numBlocks,
        G4_Operand *surface,
        G4_Operand *globalOffset,
        G4_SrcRegRegion *offsets,
        G4_SrcRegRegion *src);

    int translateVISASLMByteScaledInst(
        bool isRead,
        G4_Predicate *pred,
        VISA_Exec_Size execSize,
        VISA_EMask_Ctrl eMask,
        VISA_SVM_Block_Type blockSize,
        VISA_SVM_Block_Num numBlocks,
        uint8_t scale,
        G4_Operand *sideBand,
        G4_SrcRegRegion *offsets,
        G4_Operand *srcOrDst);

    int translateByteGatherInst(
        G4_Predicate *pred,
        VISA_Exec_Size execSize,
        VISA_EMask_Ctrl eMask,
        VISA_SVM_Block_Num numBlocks,
        G4_Operand *surface,
        G4_Operand *globalOffset,
        G4_SrcRegRegion *offsets,
        G4_DstRegRegion *dst);

    int translateByteScatterInst(
        G4_Predicate *pred,
        VISA_Exec_Size execSize,
        VISA_EMask_Ctrl eMask,
        VISA_SVM_Block_Num numBlocks,
        G4_Operand *surface,
        G4_Operand *globalOffset,
        G4_SrcRegRegion *offsets,
        G4_SrcRegRegion *src);

    int translateVISASVMBlockReadInst(
        VISA_Oword_Num numOword,
        bool unaligned,
        G4_Operand* address,
        G4_DstRegRegion* dst);

    int translateVISASVMBlockWriteInst(
        VISA_Oword_Num numOword,
        G4_Operand* address,
        G4_SrcRegRegion* src);

    int translateVISASVMScatterReadInst(
        VISA_Exec_Size executionSize,
        VISA_EMask_Ctrl emask,
        G4_Predicate* pred,
        VISA_SVM_Block_Type blockSize,
        VISA_SVM_Block_Num numBlocks,
        G4_SrcRegRegion* addresses,
        G4_DstRegRegion* dst);

    int translateVISASVMScatterWriteInst(
        VISA_Exec_Size      executionSize,
        VISA_EMask_Ctrl     emask,
        G4_Predicate*       pred,
        VISA_SVM_Block_Type blockSize,
        VISA_SVM_Block_Num  numBlocks,
        G4_SrcRegRegion*    addresses,
        G4_SrcRegRegion*    src);

    int translateVISASVMAtomicInst(
        VISAAtomicOps       op,
        unsigned short      bitwidth,
        VISA_Exec_Size      executionSize,
        VISA_EMask_Ctrl     emask,
        G4_Predicate*       pred,
        G4_SrcRegRegion*    addresses,
        G4_SrcRegRegion*    src0,
        G4_SrcRegRegion*    src1,
        G4_DstRegRegion*    dst);

    // return globalOffset + offsets as a contiguous operand
    G4_SrcRegRegion* getSVMOffset(
        G4_Operand* globalOffset,
        G4_SrcRegRegion* offsets,
        uint16_t exSize,
        G4_Predicate* pred,
        uint32_t mask);

    int translateSVMGather4Inst(
        VISA_Exec_Size      execSize,
        VISA_EMask_Ctrl     eMask,
        ChannelMask         chMask,
        G4_Predicate       *pred,
        G4_Operand         *address,
        G4_SrcRegRegion    *offsets,
        G4_DstRegRegion    *dst);

    int translateSVMScatter4Inst(
        VISA_Exec_Size      execSize,
        VISA_EMask_Ctrl     eMask,
        ChannelMask         chMask,
        G4_Predicate       *pred,
        G4_Operand         *address,
        G4_SrcRegRegion    *offsets,
        G4_SrcRegRegion    *src);

    int translateVISASVMGather4ScaledInst(
        VISA_Exec_Size      execSize,
        VISA_EMask_Ctrl     eMask,
        ChannelMask         chMask,
        G4_Predicate       *pred,
        G4_Operand         *address,
        G4_SrcRegRegion    *offsets,
        G4_DstRegRegion    *dst);

    int translateVISASVMScatter4ScaledInst(
        VISA_Exec_Size      execSize,
        VISA_EMask_Ctrl     eMask,
        ChannelMask         chMask,
        G4_Predicate       *pred,
        G4_Operand         *address,
        G4_SrcRegRegion    *offsets,
        G4_SrcRegRegion    *src);


    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // members related to media and VME are in VisaToG4/TranslateSendMedia.cpp
    int translateVISAMediaLoadInst(
        MEDIA_LD_mod mod,
        G4_Operand* surface,
        unsigned planeID,
        unsigned blockWidth,
        unsigned blockHeight,
        G4_Operand* xOffOpnd,
        G4_Operand* yOffOpnd,
        G4_DstRegRegion* dst_opnd);

    int translateVISAMediaStoreInst(
        MEDIA_ST_mod mod,
        G4_Operand* surface,
        unsigned planeID,
        unsigned blockWidth,
        unsigned blockHeight,
        G4_Operand* xOffOpnd,
        G4_Operand* yOffOpnd,
        G4_SrcRegRegion* srcOpnd);

    int translateVISAVmeImeInst(
        uint8_t stream_mode,
        uint8_t search_ctrl,
        G4_Operand* surfaceOpnd,
        G4_Operand* uniInputOpnd,
        G4_Operand* imeInputOpnd,
        G4_Operand* ref0Opnd,
        G4_Operand* ref1Opnd,
        G4_Operand* costCenterOpnd,
        G4_DstRegRegion* outputOpnd);

    int translateVISAVmeSicInst(
        G4_Operand* surfaceOpnd,
        G4_Operand* uniInputOpnd,
        G4_Operand* sicInputOpnd,
        G4_DstRegRegion* outputOpnd);

    int translateVISAVmeFbrInst(
        G4_Operand* surfaceOpnd,
        G4_Operand* unitInputOpnd,
        G4_Operand* fbrInputOpnd,
        G4_Operand* fbrMbModOpnd,
        G4_Operand* fbrSubMbShapeOpnd,
        G4_Operand* fbrSubPredModeOpnd,
        G4_DstRegRegion* outputOpnd);

    int translateVISAVmeIdmInst(
        G4_Operand* surfaceOpnd,
        G4_Operand* unitInputOpnd,
        G4_Operand* idmInputOpnd,
        G4_DstRegRegion* outputOpnd);

    int translateVISASamplerVAGenericInst(
        G4_Operand*   surface,
        G4_Operand*   sampler,
        G4_Operand*   uOffOpnd,
        G4_Operand*   vOffOpnd,
        G4_Operand*   vSizeOpnd,
        G4_Operand*   hSizeOpnd,
        G4_Operand*   mmfMode,
        unsigned char cntrl,
        unsigned char msgSeq,
        VA_fopcode    fopcode,
        G4_DstRegRegion*   dstOpnd,
        G4_Type       dstType,
        unsigned      dstSize,
        bool isBigKernel = false);

    int translateVISAAvsInst(
        G4_Operand* surface,
        G4_Operand* sampler,
        ChannelMask channel,
        unsigned numEnabledChannels,
        G4_Operand* deltaUOpnd,
        G4_Operand* uOffOpnd,
        G4_Operand* deltaVOpnd,
        G4_Operand* vOffOpnd,
        G4_Operand* u2dOpnd,
        G4_Operand* groupIDOpnd,
        G4_Operand* verticalBlockNumberOpnd,
        unsigned char cntrl,
        G4_Operand* v2dOpnd,
        unsigned char execMode,
        G4_Operand* eifbypass,
        G4_DstRegRegion* dstOpnd);

    int translateVISAVaSklPlusGeneralInst(
        ISA_VA_Sub_Opcode sub_opcode,
        G4_Operand* surface,
        G4_Operand* sampler,
        unsigned char mode,
        unsigned char functionality,
        G4_Operand* uOffOpnd,
        G4_Operand* vOffOpnd,
        //1pixel convolve
        G4_Operand * offsetsOpnd,

        //FloodFill
        G4_Operand* loopCountOpnd,
        G4_Operand* pixelHMaskOpnd,
        G4_Operand* pixelVMaskLeftOpnd,
        G4_Operand* pixelVMaskRightOpnd,

        //LBP Correlation
        G4_Operand* disparityOpnd,

        //Correlation Search
        G4_Operand* verticalOriginOpnd,
        G4_Operand* horizontalOriginOpnd,
        G4_Operand* xDirectionSizeOpnd,
        G4_Operand* yDirectionSizeOpnd,
        G4_Operand* xDirectionSearchSizeOpnd,
        G4_Operand* yDirectionSearchSizeOpnd,

        G4_DstRegRegion* dstOpnd,
        G4_Type dstType,
        unsigned dstSize,

        //HDC
        unsigned char pixelSize,
        G4_Operand* dstSurfaceOpnd,
        G4_Operand *dstXOpnd,
        G4_Operand* dstYOpnd,
        bool hdcMode);


    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Raw send related members are in VisaToG4/TranslateSendRaw.cpp
    int translateVISARawSendInst(
        G4_Predicate *predOpnd,
        VISA_Exec_Size executionSize,
        VISA_EMask_Ctrl emask,
        uint8_t modifiers,
        unsigned int exDesc,
        uint8_t numSrc,
        uint8_t numDst,
        G4_Operand* msgDescOpnd,
        G4_SrcRegRegion* msgOpnd,
        G4_DstRegRegion* dstOpnd);

    int translateVISARawSendsInst(
        G4_Predicate *predOpnd,
        VISA_Exec_Size executionSize,
        VISA_EMask_Ctrl emask,
        uint8_t modifiers,
        G4_Operand* exDesc,
        uint8_t numSrc0,
        uint8_t numSrc1,
        uint8_t numDst,
        G4_Operand* msgDescOpnd,
        G4_Operand* msgOpnd0,
        G4_Operand* msgOpnd1,
        G4_DstRegRegion* dstOpnd,
        unsigned ffid,
        bool hasEOT = false);



    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    // Raw send related members are in VisaToG4/TranslateSendSync.cpp

    G4_INST* createFenceInstruction(
        uint8_t flushParam, bool commitEnable, bool globalMemFence, bool isSendc);

    G4_INST* createSLMFence();

    int translateVISAWaitInst(G4_Operand* mask);

    void generateBarrierSend();
    void generateBarrierWait();

    int translateVISASyncInst(ISA_Opcode opcode, unsigned int mask);

    int translateVISASplitBarrierInst(bool isSignal);

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // return either 253 or 255 for A64 messages, depending on whether we want I/A coherency or not
    uint8_t getA64BTI() const { return m_options->getOption(vISA_noncoherentStateless) ? 0xFD : 0xFF; }

    bool useSends() const
    {
        return getPlatform() >= GENX_SKL && m_options->getOption(vISA_UseSends) &&
            !(VISA_WA_CHECK(m_pWaTable, WaDisableSendsSrc0DstOverlap));
    }

    Metadata* allocateMD()
    {
        Metadata* newMD = new (metadataMem) Metadata();
        allMDs.push_back(newMD);
        return newMD;
    }

    MDNode* allocateMDString(const std::string& str)
    {
        auto newNode = new (metadataMem) MDString(str);
        allMDNodes.push_back(newNode);
        return newNode;
    }

    MDLocation* allocateMDLocation(int line, const char* file)
    {
        auto newNode = new (metadataMem) MDLocation(line, file);
        allMDNodes.push_back(newNode);
        return newNode;
    }


    void materializeGlobalImm(G4_BB* entryBB); // why is in FlowGraph.cpp???

#include "HWCaps.inc"

private:
    const void* m_inst;
    G4_SrcRegRegion* createBindlessExDesc(uint32_t exdesc);
};
} // namespace vISA

// G4IR instructions added by JIT that do not result from lowering
// any CISA bytecode will be assigned CISA offset = 0xffffffff.
// This includes pseudo nodes, G4_labels, mov introduced for copying
// r0 for pre-emption support.
constexpr int UNMAPPABLE_VISA_INDEX = IR_Builder::OrphanVISAIndex;

#endif
