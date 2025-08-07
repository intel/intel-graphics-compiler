/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _BUILDIR_H_
#define _BUILDIR_H_

#include <cstdarg>
#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>

#include "Assertions.h"
#include "BitSet.h"
#include "Common_ISA_util.h"
#include "G4_IR.hpp"
#include "G4_Kernel.hpp"
#include "InstSplit.h"
#include "PreDefinedVars.h"
#include "RT_Jitter_Interface.h"
#include "inc/common/sku_wa.h"
#include "visa_igc_common_header.h"
#include "visa_wa.h"
#include "FrequencyInfo.h"

#define MAX_DWORD_VALUE 0x7fffffff
#define MIN_DWORD_VALUE 0x80000000
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

typedef struct FCCalls {
  // callOffset is in inst number units
  unsigned int callOffset;
  const char *calleeLabelString;
} FCCalls;

enum DeclareType {
  Regular = 0,
  Fill = 1,
  Spill = 2,
  Tmp = 3,
  AddrSpill = 4,
  CoalescedSpillFill = 5,
};

// forward declaration
// FIXME: our #include is a mess, need to clean it up
class CISA_IR_Builder;

namespace vISA {
// IR_Builder class has a member of type FCPatchingInfo
// as a member. This class is expected to hold all FC
// related information.
class FCPatchingInfo {
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

  std::vector<FCCalls *> FCCallsToPatch;
  std::vector<unsigned int> FCReturnOffsetsToPatch;

public:
  FCPatchingInfo() {
    hasFCCalls = false;
    isFCCallableKernel = false;
    isFCComposableKernel = false;
    isFCEntryKernel = false;
  }

  void setHasFCCalls(bool hasFC) { hasFCCalls = hasFC; }
  bool getHasFCCalls() const { return hasFCCalls; }
  void setIsCallableKernel(bool value) { isFCCallableKernel = value; }
  bool getIsCallableKernel() const { return isFCCallableKernel; }
  void setFCComposableKernel(bool value) { isFCComposableKernel = value; }
  bool getFCComposableKernel() const { return isFCComposableKernel; }
  void setIsEntryKernel(bool value) { isFCEntryKernel = value; }
  bool getIsEntryKernel() const { return isFCEntryKernel; }
  std::vector<FCCalls *> &getFCCallsToPatch() { return FCCallsToPatch; }
  std::vector<unsigned int> &getFCReturnsToPatch() {
    return FCReturnOffsetsToPatch;
  }

  enum RegAccessType : unsigned char {
    Fully_Use = 0,
    Partial_Use = 1,
    Fully_Def = 2,
    Partial_Def = 3
  };

  enum RegAccessPipe : unsigned char {
    Pipe_ALU = 0,
    Pipe_Math = 1,
    Pipe_Send = 2,
    Pipe_Dpas = 3
  };

  struct RegAccess {
    RegAccess *Next;    // The next access on the same GRF.
    RegAccessType Type; // 'def' or 'use' of that GRF.
    unsigned RegNo;     // GRF.
    unsigned Pipe;      // Pipe.
    // where that access is issued.
    G4_INST *Inst;   // Where that GRF is accessed.
    unsigned Offset; // Instruction offset populated finally.
    // Token associated with that access.
    unsigned Token; // Optional token allocation associated to 'def'.

    RegAccess()
        : Next(nullptr), Type(Fully_Use), RegNo(0), Pipe(0), Inst(nullptr),
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
  std::set<unsigned> AllocatedToken; // Allocated token.
};
} // namespace vISA

namespace vISA {
//
// hash table for holding reg vars and reg region
//

class OperandHashTable {
  Mem_Manager &mem;

  struct ImmKey {
    int64_t val;
    G4_Type valType;
    ImmKey(int64_t imm, G4_Type type) : val(imm), valType(type) {}
    bool operator==(const ImmKey &imm) const {
      return val == imm.val && valType == imm.valType;
    }
  };

  struct ImmKeyHash {
    std::size_t operator()(const ImmKey &imm) const {
      return (std::size_t)(imm.val ^ imm.valType);
    }
  };

  struct stringCompare {
    bool operator()(const char *s1, const char *s2) const {
      return strcmp(s1, s2) == 0;
    }
  };

  std::unordered_map<ImmKey, G4_Imm *, ImmKeyHash> immTable;
  std::unordered_map<const char *, G4_Label *, std::hash<const char *>,
                     stringCompare>
      labelTable;

public:
  OperandHashTable(Mem_Manager &m) : mem(m) {}

  // Generic methods that work on both integer and floating-point types.
  // For floating-point types, 'imm' needs to be G4_Imm(<float-value>.getImm().
  G4_Imm *lookupImm(int64_t imm, G4_Type ty);
  G4_Imm *createImm(int64_t imm, G4_Type ty);
};

//
// place for holding all region descriptions
//
class RegionPool {
  Mem_Manager &mem;
  std::vector<RegionDesc *> rgnlist;

public:
  RegionPool(Mem_Manager &m) : mem(m) {}
  const RegionDesc *createRegion(uint16_t vstride, uint16_t width,
                                 uint16_t hstride);
};

//
// place for holding all declares.
//
class DeclarePool {
  Mem_Manager &mem;
  const IR_Builder &irb;
  std::vector<G4_Declare *> dcllist;
  int addrSpillLocCount; // incremented in G4_RegVarAddrSpillLoc()
public:
  DeclarePool(Mem_Manager &m, const IR_Builder &builder)
      : mem(m), irb(builder), addrSpillLocCount(0) {
    dcllist.reserve(2048);
  }
  ~DeclarePool();
  DeclarePool(const DeclarePool&) = delete;
  DeclarePool& operator=(const DeclarePool&) = delete;

  G4_Declare *cloneDeclare(G4_Kernel &kernel,
                           std::map<G4_Declare *, G4_Declare *> &dclMap,
                           const char *newDclName, G4_Declare *dcl);

  G4_Declare *createDeclare(const char *name, G4_RegFileKind regFile,
                            unsigned short nElems, unsigned short nRows,
                            G4_Type ty, DeclareType kind = Regular,
                            G4_RegVar *base = nullptr,
                            G4_Operand *repRegion = nullptr,
                            G4_ExecSize execSize = G4_ExecSize(0));

  G4_Declare *createPreVarDeclare(PreDefinedVarsInternal index,
                                  unsigned short n_elems, unsigned short n_rows,
                                  G4_Type ty) {

    G4_Declare *dcl =
        new (mem) G4_Declare(irb, getPredefinedVarString(index), G4_INPUT,
                             n_elems * n_rows, ty, dcllist);
    G4_RegVar *regVar;
    regVar = new (mem) G4_RegVar(dcl, G4_RegVar::RegVarType::Default);
    dcl->setRegVar(regVar);

    return dcl;
  }

  std::vector<G4_Declare *> &getDeclareList() { return dcllist; }
};

//
// interface for creating operands and instructions
//
class IR_Builder {
public:
  const char *curFile = nullptr;
  unsigned int curLine = 0;
  int curCISAOffset = -1;

  static const int OrphanVISAIndex = 0xffffffff;
  int debugInfoPlaceholder =
      OrphanVISAIndex; // used for debug info, catch all VISA offset for orphan
                       // instructions

private:
  class GlobalImmPool {
    struct ImmVal {
      G4_Imm *imm;
      int numElt;

      bool operator==(const ImmVal &v) const {
        return imm == v.imm && numElt == v.numElt;
      }
    };
    static const int MAX_POOL_SIZE =
        8; // reg pressure control, for now just do naive first-come first-serve
    std::array<ImmVal, MAX_POOL_SIZE> immArray = {};
    std::array<G4_Declare *, MAX_POOL_SIZE> dclArray = {};
    int curSize = 0;
    IR_Builder &builder;

  public:
    GlobalImmPool(IR_Builder &b) : builder(b) {}
    G4_Declare *addImmVal(G4_Imm *imm, int numElt);
    int size() const { return curSize; }
    const ImmVal &getImmVal(int i) { return immArray[i]; }
    G4_Declare *getImmDcl(int i) { return dclArray[i]; }
  };

  GlobalImmPool immPool;

  // allocator pools
  USE_DEF_ALLOCATOR useDefAllocator;

  FINALIZER_INFO *metaData = nullptr;
  // Use a BitSet to track the barrier IDs used
  BitSet usedBarriers;

  int subroutineId = -1;     // the kernel itself has id 0, as we always emit a
                             // subroutine label for kernel too
  enum VISA_BUILD_TYPE type = VISA_BUILD_TYPE::KERNEL; // as opposed to what?

  uint32_t m_next_local_label_id = 0;

  // pre-defined declare that binds to R0 (the entire GRF)
  // when pre-emption is enabled, builtinR0 is replaced by a temp,
  // and a move is inserted at kernel entry
  // mov (8) builtinR0 realR0
  G4_Declare *builtinR0 =
      nullptr; // this is either r0 or the temp if pre-emption is enabled
  G4_Declare *realR0 = nullptr; // this always refers to r0

  // pre-defined declare that binds to A0.0:ud
  G4_Declare *builtinA0 = nullptr;
  // pre-defined declare that binds to A0.2:ud
  G4_Declare *builtinA0Dot2 = nullptr; // used for splitsend's ext msg
                                       // descriptor
  // pre-defined declare that binds to HWTid (R0.5:ud)
  G4_Declare *builtinHWTID = nullptr;
  // pre-defined declare that binds to SR0.1:ud
  G4_Declare *builtinSR0Dot1 = nullptr;

  // pre-defined QWs of S0
  // 0 and 1 are reserved for Gather (indirect) Send usage.
  G4_Declare *builtinS0 = nullptr;

  // for rotating through the high subregisters of s0 in QW
  // we require QWs even for things like SLM or URB (which are a32 bases)
  // since the sendg instruction requires QW s0 subregs
  static const unsigned FIRST_SURFACE_S0_QW = 2; // s0.{0,1}:uq for ind. send
  unsigned nextSurfaceS0QW = FIRST_SURFACE_S0_QW;
  // pre-defined bindless surface index (252, 1 UD)
  G4_Declare *builtinT252 = nullptr;
  // pre-defined bindless sampler index (31, 1 UD)
  G4_Declare *builtinBindlessSampler = nullptr;
  // pre-defined sampler header
  G4_Declare *builtinSamplerHeader = nullptr;

  // common message header for spill/fill intrinsics
  // We put them here instead of spillManager since there may be multiple rounds
  // of spill, and we want to use a common header
  G4_Declare *spillFillHeader = nullptr;

  // for scatter spills
  G4_Declare *scatterSpillBaseAddress = nullptr;
  G4_Declare *scatterSpillAddress = nullptr;

  G4_Declare *oldA0Dot2Temp = nullptr;

  G4_Declare *builtinScratchSurface = nullptr;
  // if scratch surface is used, this will be initialized once at entry
  G4_Declare *scratchSurfaceOffset = nullptr;


  // The temp var for eu fusion W/A
  G4_Declare *euFusionWATmpVar = nullptr;

  // Indicates that sampler header cache (builtinSamplerHeader) is correctly
  // initialized with r0 contents.
  // Used only when vISA_cacheSamplerHeader option is set.
  bool builtinSamplerHeaderInitialized = false;
  // for PVC send WAR WA
  bool hasDF = false;
  // function call related declares
  G4_Declare *be_sp = nullptr;
  G4_Declare *be_fp = nullptr;
  // Part FDE inst is the move that stores r125.[0-3] to a temp.
  // This is used to restore ret %ip, ret EM, and BE ptrs.
  G4_INST *savePartFDInst = nullptr;
  // FDE spill inst is first spill instruction that writes frame
  // descriptor to stack.
  G4_INST *FDSpillInst = nullptr;
  // Instruction with ScratchSurfaceOffset
  G4_INST* instSSO = nullptr;
  G4_Declare *tmpFCRet = nullptr;
  // Used to store implicit arg, local id buffer ptrs for stackcall
  G4_Declare *implArgBufferPtr = nullptr;
  G4_Declare *localIdBufferPtr = nullptr;

  unsigned short arg_size;
  unsigned short return_var_size;

  unsigned int sampler8x8_group_id;

  // input declare of R1.
  G4_Declare *inputR1 = nullptr;

  // Populate this data structure so after compiling all kernels
  // in file, we can emit out patch file using this up-levelled
  // information.
  FCPatchingInfo *fcPatchInfo = nullptr;

  const WA_TABLE *m_pWaTable;
  Options *m_options = nullptr;

  std::map<const G4_INST *, G4_FCALL> m_fcallInfo;

  // Basic region descriptors.
  RegionDesc CanonicalRegionStride0, // <0; 1, 0>
      CanonicalRegionStride1,        // <1; 1, 0>
      CanonicalRegionStride2,        // <2; 1, 0>
      CanonicalRegionStride4;        // <4; 1, 0>

  // map of all stack functioncs ever invoked by this builder's kernel/function
  std::map<std::string, G4_Label *> m_fcallLabels;

  G4_Label *getFcallLabel(const std::string &str) {
    auto it = m_fcallLabels.find(str);
    if (it == m_fcallLabels.end()) {
      auto label = createLabel(str, LABEL_FUNCTION);
      m_fcallLabels[str] = label;
      return label;
    }
    return it->second;
  }

  class PreDefinedVars {
  public:
    void setHasPredefined(PreDefinedVarsInternal id, bool val) {
      vASSERT(id < PreDefinedVarsInternal::VAR_LAST);
      hasPredefined[static_cast<int>(id)] = val;
    }

    bool isHasPredefined(PreDefinedVarsInternal id) const {
      vASSERT(id < PreDefinedVarsInternal::VAR_LAST);
      return hasPredefined[static_cast<int>(id)];
    }

    void setPredefinedVar(PreDefinedVarsInternal id, G4_Declare *dcl) {
      vASSERT(id < PreDefinedVarsInternal::VAR_LAST);
      predefinedVars[static_cast<int>(id)] = dcl;
    }

    G4_Declare *getPreDefinedVar(PreDefinedVarsInternal id) const {
      vASSERT(id < PreDefinedVarsInternal::VAR_LAST);
      if (id >= PreDefinedVarsInternal::VAR_LAST) {
        return nullptr;
      }
      return predefinedVars[static_cast<int>(id)];
    }

  private:
    // records whether a vISA pre-defined var is used by the kernel
    // some predefined need to be expanded (e.g., HWTid, color)
    bool hasPredefined[static_cast<int>(PreDefinedVarsInternal::VAR_LAST)]{};
    G4_Declare
        *predefinedVars[static_cast<int>(PreDefinedVarsInternal::VAR_LAST)]{};
  };

  bool hasNullReturnSampler = false;

  const CISA_IR_Builder *parentBuilder = nullptr;

  // stores all metadata ever allocated
  Mem_Manager metadataMem = 4096;
  std::vector<Metadata *> allMDs;
  std::vector<MDNode *> allMDNodes;

  FrequencyInfo freqInfoManager;

  // bump pointer allocator for variable and label names, used for IR dump only.
  Mem_Manager debugNameMem = 4096;

  // Whether the kernel should avoid clobbering or even reading R0, this
  // is generally due to mid-thread preemption.
  // If mode is read-only, we mark it as forbidden in various RA algorithms so
  // that it will not be re-assigned to other variables.
  // If mode is none, we copy it to a different register at kernel entry and use
  // that in the body instead. r0 should not be used outside the prolog.
  enum class R0_ACCESS { READ_WRITE, READ_ONLY, NONE };
  // This unfortunately can't be const due to some context-dependent WA that can
  // only be set after all instructions are read.
  R0_ACCESS r0AccessMode;
  R0_ACCESS getR0AccessFromOptions() const;

public:
  PreDefinedVars preDefVars;
  Mem_Manager &mem;           // memory for all operands and insts
  PhyRegPool phyregpool;      // all physical regs
  OperandHashTable hashtable; // all created region operands
  RegionPool rgnpool;         // all region description
  DeclarePool dclpool;        // all created decalres
  INST_LIST instList;         // all created insts
  // list of instructions ever allocated
  // This list may only grow and is freed when IR_Builder is destroyed
  std::vector<G4_INST *> instAllocList;
  G4_Kernel &kernel;
  // the following fileds are used for dcl name when a new dcl is created.
  // number of predefined variables are included.
  unsigned num_temp_dcl;
  // number of temp GRF vars created to hold spilled addr/flag
  uint32_t numAddrFlagSpillLoc = 0;
  std::vector<input_info_t *> m_inputVect;
  BitSet src1FirstGRFOfLastDpas;

  const Options *getOptions() const { return m_options; }
  bool getOption(vISAOptions opt) const { return m_options->getOption(opt); }
  uint32_t getuint32Option(vISAOptions opt) const {
    return m_options->getuInt32Option(opt);
  }
  void getOption(vISAOptions opt, const char *&str) const {
    return m_options->getOption(opt, str);
  }
  void addInputArg(input_info_t *inpt);
  input_info_t *getInputArg(unsigned int index) const;
  unsigned int getInputCount() const;
  input_info_t *getRetIPArg() const;

  const CISA_IR_Builder *getParent() const { return parentBuilder; }

  void dump(std::ostream &os); // not const because G4_INST::emit isn't :(

  std::stringstream &criticalMsgStream();

  const USE_DEF_ALLOCATOR &getAllocator() const { return useDefAllocator; }

  // Getter/setter for be_sp and be_fp
  G4_Declare *getBESP() {
    if (be_sp == NULL) {
      be_sp = createDeclare("be_sp", G4_GRF, 1, 1, Type_UD);
      be_sp->getRegVar()->setPhyReg(phyregpool.getGreg(kernel.stackCall.getFPSPGRF()),
                                    kernel.stackCall.subRegs.BE_SP);
    }

    return be_sp;
  }

  G4_Declare *getBEFP() {
    if (be_fp == NULL) {
      be_fp = createDeclare("be_fp", G4_GRF, 1, 1, Type_UD);
      be_fp->getRegVar()->setPhyReg(phyregpool.getGreg(kernel.stackCall.getFPSPGRF()),
                                    kernel.stackCall.subRegs.BE_FP);
    }

    return be_fp;
  }

  G4_INST *getPartFDSaveInst() const { return savePartFDInst; }
  void setPartFDSaveInst(G4_INST *i) { savePartFDInst = i; }

  G4_INST *getFDSpillInst() const { return FDSpillInst; }
  void setFDSpillInst(G4_INST *i) { FDSpillInst = i; }

  // Getter/settter for instruction with ScratchSurfaceOffset
  G4_INST* getSSOInst() const { return instSSO; }
  void setSSOInst(G4_INST* inst) { instSSO = inst; }

  G4_Declare *getStackCallArg() const {
    return preDefVars.getPreDefinedVar(PreDefinedVarsInternal::ARG);
  }
  G4_Declare *getStackCallRet() const {
    return preDefVars.getPreDefinedVar(PreDefinedVarsInternal::RET);
  }

  G4_Declare *getFE_SP() const {
    return preDefVars.getPreDefinedVar(PreDefinedVarsInternal::FE_SP);
  }

  G4_Declare *getFE_FP() const {
    return preDefVars.getPreDefinedVar(PreDefinedVarsInternal::FE_FP);
  }

  G4_Declare *getImplArgBufPtr() const {
    return preDefVars.getPreDefinedVar(
        PreDefinedVarsInternal::IMPL_ARG_BUF_PTR);
  }

  G4_Declare *getLocalIdBufPtr() const {
    return preDefVars.getPreDefinedVar(
        PreDefinedVarsInternal::LOCAL_ID_BUF_PTR);
  }

  bool isPreDefArg(G4_Declare *dcl) const { return dcl == getStackCallArg(); }

  bool isPreDefRet(G4_Declare *dcl) const { return dcl == getStackCallRet(); }

  bool isPreDefFEStackVar(G4_Declare *dcl) const {
    return dcl == getFE_SP() || dcl == getFE_FP();
  }

  bool isPreDefSpillHeader(G4_Declare *dcl) const {
    return dcl == getImplArgBufPtr() || dcl == getLocalIdBufPtr();
  }

  // this refers to vISA's internal stack for spill and caller/callee-save
  // Note that this is only valid after CFG is constructed
  // ToDo: make this a pass?
  bool usesStack() const {
    return kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc();
  }

  void bindInputDecl(G4_Declare *dcl, int grfOffset);

  uint32_t getPerThreadInputSize() const {
    return kernel.getInt32KernelAttr(Attributes::ATTR_PerThreadInputSize);
  }

  int32_t getCrossThreadInputSize() const {
    return kernel.getInt32KernelAttr(Attributes::ATTR_CrossThreadInputSize);
  }

  bool getLTOInvokeOptTarget() const {
    return kernel.getBoolKernelAttr(Attributes::ATTR_LTOInvokeOptTarget);
  }

  bool isGRFDstAligned(G4_Operand *dst, int alignByte) const;

  //
  // Check if opnd is or can be made "alignByte"-byte aligned.
  // These functions will change the underlying variable's alignment
  // (e.g., make a scalar variable GRF-aligned) when possible to satisfy
  // the alignment
  bool tryToAlignOperand(G4_Operand *opnd, int alignByte) const;
  bool tryToAlignOperand(G4_Operand *opnd, unsigned short &offset,
                         int alignByte) const;

  void setType(enum VISA_BUILD_TYPE _type) { type = _type; }
  bool getIsKernel() const { return type == VISA_BUILD_TYPE::KERNEL; }
  bool getIsFunction() const { return type == VISA_BUILD_TYPE::FUNCTION; }
  bool getIsPayload() const { return type == VISA_BUILD_TYPE::PAYLOAD; }
  enum VISA_BUILD_TYPE getType() const { return type; }
  void predefinedVarRegAssignment(uint8_t inputSize);
  void expandPredefinedVars();
  void setArgSize(unsigned short size) { arg_size = size; }
  unsigned short getArgSize() const { return arg_size; }
  void setRetVarSize(unsigned short size) { return_var_size = size; }
  unsigned short getRetVarSize() const { return return_var_size; }
  unsigned int getCallRetOpndSize() const;

  FCPatchingInfo *getFCPatchInfo();

  const WA_TABLE *getPWaTable() const { return m_pWaTable; }

  const char *getNameString(size_t size, const char *format, ...);

  G4_Predicate_Control
  vISAPredicateToG4Predicate(VISA_PREDICATE_CONTROL control,
                             G4_ExecSize execSize);

  std::optional<G4_FCALL> getFcallInfo(const G4_INST *inst) const;
  void addFcallInfo(const G4_INST *FcallInst, uint16_t ArgSize,
                    uint16_t RetSize, bool isUniform) {
    m_fcallInfo.emplace(FcallInst, G4_FCALL(ArgSize, RetSize, isUniform));
  }

  // If this is true (detected in TranslateInterface.cpp), we need a sampler
  // flush before EOT
  bool getHasNullReturnSampler() const { return hasNullReturnSampler; }

  //    Initializes predefined vars for all the vISA versions
  void createPreDefinedVars();

  void createBuiltinDecls();

  G4_Declare *getSpillFillHeader();
  bool hasValidSpillFillHeader() { return spillFillHeader; }

  G4_Declare *getScatterSpillBaseAddress();
  G4_Declare *getScatterSpillAddress();

  G4_Declare *getEUFusionWATmpVar();

  G4_Declare *getOldA0Dot2Temp();
  bool hasValidOldA0Dot2() { return oldA0Dot2Temp; }
  bool hasDFInst() const { return hasDF; }

  IR_Builder(INST_LIST_NODE_ALLOCATOR &alloc, G4_Kernel &k, Mem_Manager &m,
             Options *options, CISA_IR_Builder *parent, FINALIZER_INFO *jitInfo,
             const WA_TABLE *pWaTable);

  ~IR_Builder();
  IR_Builder(const IR_Builder&) = delete;
  IR_Builder& operator=(const IR_Builder&) = delete;

  void setR0ReadOnly() {
    // NONE is stronger than READ_ONLY, so don't change it
    if (r0AccessMode == R0_ACCESS::READ_WRITE)
      r0AccessMode = R0_ACCESS::READ_ONLY;
  }
  bool canReadR0() const { return r0AccessMode != R0_ACCESS::NONE; }
  bool canWriteR0() const { return r0AccessMode == R0_ACCESS::READ_WRITE; }
  bool mustReserveR1() const;

  void rebuildPhyRegPool(unsigned int numRegisters) {
    phyregpool.rebuildRegPool(mem, numRegisters);
  }

  TARGET_PLATFORM getPlatform() const { return kernel.getPlatform(); }
  PlatformGen getPlatformGeneration() const {
    return kernel.getPlatformGeneration();
  }
  const char *getGenxPlatformString() const {
    return kernel.getGenxPlatformString();
  }
  unsigned getACCSize() const { return getGRFSize(); }
  unsigned char getGRFSize() const { return kernel.getGRFSize(); }
  template <G4_Type T> unsigned numEltPerGRF() const {
    return kernel.numEltPerGRF<T>();
  }
  unsigned numEltPerGRF(G4_Type t) const { return kernel.numEltPerGRF(t); }
  unsigned getMaxVariableSize() const { return kernel.getMaxVariableSize(); }
  G4_SubReg_Align getGRFAlign() const { return kernel.getGRFAlign(); }
  G4_SubReg_Align getHalfGRFAlign() const { return kernel.getHalfGRFAlign(); }
  unsigned getGenxDataportIOSize() const {
    return kernel.getGenxDataportIOSize();
  }
  unsigned getGenxSamplerIOSize() const {
    return kernel.getGenxSamplerIOSize();
  }
  FINALIZER_INFO *getJitInfo() { return metaData; }
  BitSet &usedBarries() { return usedBarriers; }
  // Return the max id set + 1 as the number of barriers used. Ideally the
  // number of bits set can be used to represent the number of barriers.
  // However, In current programming model the barriers should be allocated
  // sequentially, so here we return max id + 1 to make sure of that.
  unsigned numBarriers() const {
    int maxId = usedBarriers.findLastIn(0, kernel.getMaxNumOfBarriers());
    // maxId + 1 would also cover the case, which returns -1, that no bits
    // are set.
    return maxId + 1;
  }
  void updateBarrier();
  void updateNamedBarrier(G4_Operand *barrierId);

  G4_Declare *cloneDeclare(std::map<G4_Declare *, G4_Declare *> &dclMap,
                           G4_Declare *dcl);

  G4_Declare *createDeclare(const char *name, G4_RegFileKind regFile,
                            unsigned short n_elems, unsigned short n_rows,
                            G4_Type ty, DeclareType kind = Regular,
                            G4_RegVar *base = NULL,
                            G4_Operand *repRegion = NULL,
                            G4_ExecSize execSize = G4_ExecSize(0));

  G4_Declare *createPreVarDeclareNoLookup(PreDefinedVarsInternal index,
                                          unsigned short n_elems,
                                          unsigned short n_rows, G4_Type ty) {
    G4_Declare *dcl = dclpool.createPreVarDeclare(index, n_elems, n_rows, ty);
    kernel.Declares.push_back(dcl);
    return dcl;
  }

  G4_Declare *getBuiltinR0() { return builtinR0; }
  void setBuiltInR0(G4_Declare *dcl) { builtinR0 = dcl; }
  G4_Declare *getRealR0() const {
    return realR0;
  } // undefined terminology: what's "real" here (vs "builtin" above)?
  void setRealR0(G4_Declare *dcl) { realR0 = dcl; }
  G4_Declare *getBuiltinA0() { return builtinA0; }
  G4_Declare *getBuiltinA0Dot2() { return builtinA0Dot2; }
  G4_Declare *getBuiltinHWTID() const { return builtinHWTID; }
  G4_Declare *getBuiltinSR0Dot1() const { return builtinSR0Dot1; }

  // The first part of s0 is reserved for Xe3+ Gather Send (indirect send)
  // This tests if an operand refers to Gather Send or something else
  bool isBuiltinSendIndirectS0(G4_Operand *op) const;

  G4_Declare *getBuiltinT252() const { return builtinT252; }
  G4_Declare *getBuiltinBindlessSampler() const {
    return builtinBindlessSampler;
  }
  G4_Declare *getBuiltinSamplerHeader() const { return builtinSamplerHeader; }
  G4_Declare *getBuiltinS0() const { return builtinS0;}
  G4_Declare *getOldA0Dot2Temp() const { return oldA0Dot2Temp; }
  void setHasDFInst(bool b) { hasDF = b; }
  G4_Declare *getInputR1() { return inputR1; }
  void setInputR1(G4_Declare *r1) { inputR1 = r1; }

  bool isBindlessSampler(const G4_Operand *sampler) const {
    return sampler->isSrcRegRegion() &&
           sampler->getTopDcl() == getBuiltinBindlessSampler();
  }

  bool isBindlessSurface(const G4_Operand *bti) const {
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

  bool shouldForceSplitSend(const G4_Operand *surface) const {
    return surface->isSrcRegRegion() &&
           (surface->getTopDcl() == getBuiltinT252() ||
            surface->getTopDcl() == getBuiltinScratchSurface());
  }

  /// getSplitEMask() calculates the new mask after splitting from the current
  /// execution mask at the given execution size.
  /// It only works with masks covering whole GRF and thus won't
  /// generate/consume nibbles.
  static uint32_t getSplitEMask(unsigned execSize, uint32_t eMask, bool isLo);
  static uint32_t getSplitLoEMask(unsigned execSize, uint32_t eMask) {
    return getSplitEMask(execSize, eMask, true);
  }
  static uint32_t getSplitHiEMask(unsigned execSize, uint32_t eMask) {
    return getSplitEMask(execSize, eMask, false);
  }

  bool isScratchSpace(G4_Operand *bti) const {
    return bti->isSrcRegRegion() && bti->getTopDcl() == builtinScratchSurface;
  }
  G4_Declare *getBuiltinScratchSurface() const { return builtinScratchSurface; }

  G4_SrcRegRegion *createScratchExDesc(uint32_t exdesc);

  void initScratchSurfaceOffset();
  void initAddressesForScatterSpills();

  G4_Declare *getSpillSurfaceOffset() { return scratchSurfaceOffset; }

  // create a new temp GRF with the specified type/size and undefined regions
  G4_Declare *createTempVar(unsigned int numElements, G4_Type type,
                            G4_SubReg_Align subAlign, const char *prefix = "TV",
                            bool appendIdToName = true);

  // create a new temp GRF as the home location of a spilled addr/flag dcl
  G4_Declare *createAddrFlagSpillLoc(G4_Declare *dcl);

  // like the above, but also mark the variable as don't spill
  // this is used for temp variables in macro sequences where spilling woul not
  // help
  // FIXME: can we somehow merge this with G4_RegVarTmp/G4_RegVarTransient?
  G4_Declare *createTempVarWithNoSpill(unsigned int numElements, G4_Type type,
                                       G4_SubReg_Align subAlign,
                                       const char *prefix = "TV") {
    G4_Declare *dcl = createTempVar(numElements, type, subAlign, prefix);
    dcl->setDoNotSpill();
    return dcl;
  }

  //
  // Create a declare that is hardwired to some phyiscal GRF.
  // It is useful to implement various workarounds post RA where we want to
  // directly address some physical GRF. regOff is in unit of the declare type.
  // caller is responsible for ensuring the resulting variable does not violate
  // any HW restrictions (e.g., operand does not cross two GRF)
  G4_Declare *createHardwiredDeclare(uint32_t numElements, G4_Type type,
                                     uint32_t regNum, uint32_t regOff);

  void createPseudoKills(std::initializer_list<G4_Declare *> dcls,
                         PseudoKillType ty);

  G4_INST *createPseudoKill(G4_Declare *dcl, PseudoKillType ty,
                            bool addToInstList);

  G4_INST *createEUWASpill(bool addToInstList);

  // numRows is in hword units
  // offset is in hword units
  G4_INST *createSpill(G4_DstRegRegion *dst, G4_SrcRegRegion *header,
                       G4_SrcRegRegion *payload, G4_ExecSize execSize,
                       uint16_t numRows, uint32_t offset, G4_Declare *fp,
                       G4_InstOption option, bool addToInstList,
                       bool isScatter = false);

  G4_INST *createSpill(G4_DstRegRegion *dst, G4_SrcRegRegion *payload,
                       G4_ExecSize execSize, uint16_t numRows, uint32_t offset,
                       G4_Declare *fp, G4_InstOption option, bool addToInstList,
                       bool isScatter = false);

  G4_INST *createFill(G4_SrcRegRegion *header, G4_DstRegRegion *dstData,
                      G4_ExecSize execSize, uint16_t numRows, uint32_t offset,
                      G4_Declare *fp, G4_InstOption option, bool addToInstList);
  G4_INST *createFill(G4_DstRegRegion *dstData, G4_ExecSize execSize,
                      uint16_t numRows, uint32_t offset, G4_Declare *fp,
                      G4_InstOption option, bool addToInstList);

  // numberOfFlags MEANS NUMBER OF WORDS (e.g., 1 means 16-bit), not number of
  // bits or number of data elements in operands.
  G4_Declare *createTempFlag(unsigned short numberOfFlags,
                             const char *prefix = "TEMP_FLAG_");

  // like above, but pass numFlagElements instead. This allows us to distinguish
  // between 1/8/16-bit flags, which are all allocated as a UW. name is
  // allocated by caller
  G4_Declare *createFlag(uint16_t numFlagElements, const char *name);

  G4_Declare *createTempScalar(uint16_t numFlagElements, const char *prefix);

  G4_Declare *createScalar(uint16_t numFlagElements, const char *name);

  G4_Declare *createTempAddress(uint16_t numAddressElements,
                                const char *prefix = "TEMP_ADDR");

  G4_Declare *createPreVar(PreDefinedVarsInternal preDefVar_index,
                           unsigned short numElements, G4_Type type);

  //
  // create <vstride; width, hstride>
  //
  // PLEASE use getRegion* interface to get regions if possible!
  // This function will be mostly used for external regions.
  const RegionDesc *createRegionDesc(uint16_t vstride, uint16_t width,
                                     uint16_t hstride) {
    return rgnpool.createRegion(vstride, width, hstride);
  }

  // Given the execution size and region parameters, create a region
  // descriptor.
  //
  // PLEASE use getRegion* interface to get regions if possible!
  // This function will be mostly used for external regions.
  const RegionDesc *createRegionDesc(uint16_t execSize, uint16_t vstride,
                                     uint16_t width, uint16_t hstride) {
    // Performs normalization for commonly used regions.
    switch (RegionDesc::getRegionDescKind(execSize, vstride, width, hstride)) {
    case RegionDesc::RK_Stride0:
      return getRegionScalar();
    case RegionDesc::RK_Stride1:
      return getRegionStride1();
    case RegionDesc::RK_Stride2:
      return getRegionStride2();
    case RegionDesc::RK_Stride4:
      return getRegionStride4();
    default:
      break;
    }
    return rgnpool.createRegion(vstride, width, hstride);
  }

  /// Helper to normalize an existing region descriptor.
  const RegionDesc *getNormalizedRegion(uint16_t execSize,
                                        const RegionDesc *rd) {
    return createRegionDesc(execSize, rd->vertStride, rd->width,
                            rd->horzStride);
  }

  /// Get the predefined region descriptors.
  const RegionDesc *getRegionScalar() const { return &CanonicalRegionStride0; }
  const RegionDesc *getRegionStride1() const { return &CanonicalRegionStride1; }
  const RegionDesc *getRegionStride2() const { return &CanonicalRegionStride2; }
  const RegionDesc *getRegionStride4() const { return &CanonicalRegionStride4; }

  // ToDo: get rid of this version and use the message type specific ones below
  // instead, so we can avoid having to explicitly create extDesc bits
  G4_SendDescRaw *createGeneralMsgDesc(uint32_t desc, uint32_t extDesc,
                                       SendAccess access,
                                       G4_Operand *bti = nullptr,
                                       G4_Operand *sti = nullptr);

  G4_SendDescRaw *createReadMsgDesc(SFID sfid, uint32_t desc,
                                    G4_Operand *bti = nullptr);

  G4_SendDescRaw *createWriteMsgDesc(SFID sfid, uint32_t desc, int src1Len,
                                     G4_Operand *bti = nullptr);

  G4_SendDescRaw *createSyncMsgDesc(SFID sfid, uint32_t desc);

  G4_SendDescRaw *createSampleMsgDesc(uint32_t desc, bool cps, int src1Len,
                                      G4_Operand *bti, G4_Operand *sti);

  static SendAccess getSendAccessType(bool isRead, bool isWrite) {
    if (isRead && isWrite) {
      return SendAccess::READ_WRITE;
    }
    return isRead ? SendAccess::READ_ONLY : SendAccess::WRITE_ONLY;
  }

  G4_SendDescRaw *createSendMsgDesc(SFID sfid, uint32_t desc, uint32_t extDesc,
                                    int src1Len, SendAccess access,
                                    G4_Operand *bti, G4_ExecSize execSize,
                                    bool isValidFuncCtrl = true);

  G4_SendDescRaw *createSendMsgDesc(SFID sfid, uint32_t desc, uint32_t extDesc,
                                    int src1Len, SendAccess access,
                                    G4_Operand *bti,
                                    bool isValidFuncCtrl = true);

  G4_SendDescRaw *createSendMsgDesc(unsigned funcCtrl, unsigned regs2rcv,
                                    unsigned regs2snd, SFID funcID,
                                    unsigned extMsgLength, uint16_t extFuncCtrl,
                                    SendAccess access,
                                    G4_Operand *bti = nullptr,
                                    G4_Operand *sti = nullptr);

  G4_Operand *emitSampleIndexGE16(G4_Operand *sampler, G4_Declare *headerDecl);

  //
  // deprecated, please use the one below
  //
  G4_SrcRegRegion *createSrcRegRegion(G4_SrcRegRegion &src) {
    G4_SrcRegRegion *rgn = new (mem) G4_SrcRegRegion(src);
    return rgn;
  }

  // Create a new srcregregion allocated in mem
  G4_SrcRegRegion *createSrc(G4_VarBase *b, short roff, short sroff,
                             const RegionDesc *rd, G4_Type ty,
                             G4_AccRegSel regSel = ACC_UNDEFINED) {
    return createSrcRegRegion(G4_SrcModifier::Mod_src_undef,
                              G4_RegAccess::Direct, b, roff, sroff, rd, ty,
                              regSel);
  }

  // deprecated, either use createSrc or createIndirectSrc
  G4_SrcRegRegion *createSrcRegRegion(G4_SrcModifier m, G4_RegAccess a,
                                      G4_VarBase *b, short roff, short sroff,
                                      const RegionDesc *rd, G4_Type ty,
                                      G4_AccRegSel regSel = ACC_UNDEFINED) {
    G4_SrcRegRegion *rgn =
        new (mem) G4_SrcRegRegion(*this, m, a, b, roff, sroff, rd, ty, regSel);
    return rgn;
  }

  G4_SrcRegRegion *createSrcWithNewRegOff(G4_SrcRegRegion *old,
                                          short newRegOff);
  G4_SrcRegRegion *createSrcWithNewSubRegOff(G4_SrcRegRegion *old,
                                             short newSubRegOff);
  G4_SrcRegRegion *createSrcWithNewBase(G4_SrcRegRegion *old,
                                        G4_VarBase *newBase);

  G4_SrcRegRegion *createIndirectSrc(G4_SrcModifier m, G4_VarBase *b,
                                     short roff, short sroff,
                                     const RegionDesc *rd, G4_Type ty,
                                     short immAddrOff) {
    G4_SrcRegRegion *rgn = new (mem) G4_SrcRegRegion(
        *this, m, IndirGRF, b, roff, sroff, rd, ty, ACC_UNDEFINED);
    rgn->setImmAddrOff(immAddrOff);
    return rgn;
  }

  //
  // deprecated, please use the version below
  //
  G4_DstRegRegion *createDstRegRegion(G4_DstRegRegion &dst) {
    G4_DstRegRegion *rgn = new (mem) G4_DstRegRegion(dst);
    return rgn;
  }

  // create a direct DstRegRegion
  G4_DstRegRegion *createDst(G4_VarBase *b, short roff, short sroff,
                             unsigned short hstride, G4_Type ty,
                             G4_AccRegSel regSel = ACC_UNDEFINED) {
    return createDstRegRegion(Direct, b, roff, sroff, hstride, ty, regSel);
  }
  // create a direct DstRegRegion
  G4_DstRegRegion *createDst(G4_VarBase *b, G4_Type ty) {
    return createDstRegRegion(Direct, b, 0, 0, 1, ty, ACC_UNDEFINED);
  }

  // create a indirect DstRegRegion
  // b is the address variable, which only supports subreg offset
  G4_DstRegRegion *createIndirectDst(G4_VarBase *b, short sroff,
                                     uint16_t hstride, G4_Type ty,
                                     int16_t immOff) {
    auto dst = createDstRegRegion(IndirGRF, b, 0, sroff, hstride, ty);
    dst->setImmAddrOff(immOff);
    return dst;
  }

  G4_DstRegRegion *createDstWithNewSubRegOff(G4_DstRegRegion *old,
                                             short newSubRegOff);

  //
  // return the imm operand; create one if not yet created
  //
  G4_Imm *createImm(int64_t imm, G4_Type ty) {
    G4_Imm *i = hashtable.lookupImm(imm, ty);
    return (i != NULL) ? i : hashtable.createImm(imm, ty);
  }

  //
  // return the float operand; create one if not yet created
  //
  G4_Imm *createImm(float fp);

  //
  // return the double operand; create one if not yet created
  //
  G4_Imm *createDFImm(double fp);

  // For integer immediates use a narrower type if possible
  // also change byte type to word type since HW does not support byte imm
  G4_Type getNewType(int64_t imm, G4_Type ty);

  //
  // return the imm operand with its lowest type(W or above); create one if not
  // yet created
  //
  G4_Imm *createImmWithLowerType(int64_t imm, G4_Type ty) {
    G4_Type new_type = getNewType(imm, ty);
    G4_Imm *i = hashtable.lookupImm(imm, new_type);
    return (i != NULL) ? i : hashtable.createImm(imm, new_type);
  }

  //
  // Create immediate operand without looking up hash table. This operand
  // is a relocatable immediate type.
  //
  G4_Reloc_Imm *createRelocImm(
    GenRelocType st, const std::string &sym, G4_Type ty)
  {
    return createRelocImm(st, sym, G4_Reloc_Imm::DEFAULT_MAGIC, ty);
  }

  //
  // Create immediate operand without looking up hash table. This operand
  // is a relocatable immediate type. Specify the value of this imm field,
  // which will present in the output instruction's imm value.
  //
  G4_Reloc_Imm *createRelocImm(
    GenRelocType st, const std::string &sym, int64_t immval, G4_Type ty)
  {
    size_t len = sym.size() + 1;
    char *symCopy = (char *)mem.alloc(len); // +1 for null that ends the string
    memcpy_s(symCopy, len, sym.c_str(), len);
    G4_Reloc_Imm *newImm = new (mem) G4_Reloc_Imm(st, symCopy, immval, ty);
    return newImm;
  }

  //
  // a new null-terminated copy of "lab" is created for the new label, so
  // caller does not have to allocate memory for lab
  // Note that "lab" should be unique within CISA_IR_BUILDER.
  G4_Label *createLabel(std::string_view lab, VISA_Label_Kind kind) {
    auto labStr = lab.data();
    size_t len = lab.size() + 1;
    char *new_str = (char *)mem.alloc(len); // +1 for null that ends the string
    memcpy_s(new_str, len, labStr, len);
    return new (mem) G4_Label(new_str, kind);
  }

  uint32_t getAndUpdateNextLabelId() { return m_next_local_label_id++; }
  //
  // createLocalBlockLabel() creates a unique label of type LABEL_BLOCK.
  //
  // Return a new G4_Label whose name is a null-terminated local label that
  // always starts with "_". This label is guaranteed to be unique within a
  // kernel and all its functions (within CISA_IR_BUILDER).
  // It is in the form:
  //
  //    label name:
  //    _[<kernelName>|L]_[k|f]<functionId>_<func_local_label_id>_<lab>
  //
  //    where optional <lab> is used for annotating the label for readability;
  //    and 'k' is for kernel entry and 'f' is for function.
  // If no kernel name or kernel name is too long, the label will start with
  // "_L".
  //
  G4_Label *createLocalBlockLabel(const std::string &lab = "") {
    const char *cstr_kname = kernel.getName();
    std::string kname("L");
    if (cstr_kname) {
      std::string tName = sanitizeLabelString(cstr_kname);
      // cstr_kname is just for readability. If it is too long, don't use it.
      // Also, make sure cstr_kname does not start with "_L_" to make sure it
      // would never be the same as any internal label (starts with "_L_").
      if (tName.size() != 0 && tName.size() <= 30 && tName.find("_L_") != 0) {
        kname = std::move(tName);
      }
    }

    std::stringstream ss;
    uint32_t lbl_id = getAndUpdateNextLabelId();
    // kernel and function (getFunctionId()) are numbered independently.
    // Make sure to use "k" for kernel entry and "f" for function to avoid
    // the same label in kernel entry and the first function.
    ss << "_" << kname << (getIsKernel() ? "_k" : "_f")
       << kernel.getFunctionId() << "_" << lbl_id << "_" << lab;
    size_t len = ss.str().size() + 1;
    char *new_str = (char *)mem.alloc(len); // +1 for null that ends the string
    memcpy_s(new_str, len, ss.str().c_str(), len);
    return new (mem) G4_Label(new_str, LABEL_BLOCK);
  }

  G4_Predicate *createPredicate(G4_PredState s, G4_VarBase *flag,
                                unsigned short srOff,
                                G4_Predicate_Control ctrl = PRED_DEFAULT) {
    G4_Predicate *pred = new (mem) G4_Predicate(s, flag, srOff, ctrl);
    return pred;
  }

  G4_Predicate *createPredicate(G4_Predicate &prd) {
    G4_Predicate *p = new (mem) G4_Predicate(prd);
    return p;
  }

  G4_CondMod *createCondMod(G4_CondModifier m, G4_VarBase *flag,
                            unsigned short off) {
    G4_CondMod *p = new (mem) G4_CondMod(m, flag, off);
    return p;
  }

  //
  // return the condition modifier; create one if not yet created
  //
  G4_CondMod *createCondMod(G4_CondMod &mod) {
    G4_CondMod *p = new (mem) G4_CondMod(mod);
    return p;
  }

  //
  // create register address expression normalized to (&reg +/- exp)
  //
  G4_AddrExp *createAddrExp(G4_RegVar *reg, int offset, G4_Type ty) {
    return new (mem) G4_AddrExp(reg, offset, ty);
  }

private:
  // Avoid calling this directly, use createDst and createIndirectDst instead
  G4_DstRegRegion *createDstRegRegion(G4_RegAccess a, G4_VarBase *b, short roff,
                                      short sroff, unsigned short hstride,
                                      G4_Type ty,
                                      G4_AccRegSel regSel = ACC_UNDEFINED) {
    G4_DstRegRegion *rgn = new (mem)
        G4_DstRegRegion(*this, a, b, roff, sroff, hstride, ty, regSel);
    return rgn;
  }

  // please leave all createInst() as private and use the public wrappers below

  // cond+sat+binary+line
  G4_INST *createInst(G4_Predicate *prd, G4_opcode op, G4_CondMod *mod,
                      G4_Sat sat, G4_ExecSize size, G4_DstRegRegion *dst,
                      G4_Operand *src0, G4_Operand *src1, G4_InstOpts options,
                      bool addToInstList);

  // TODO: remove
  // old template:
  // cond+sat+binary+line
  G4_INST *createInst(G4_Predicate *prd, G4_opcode op, G4_CondMod *mod,
                      G4_Sat sat, int execSize, G4_DstRegRegion *dst,
                      G4_Operand *src0, G4_Operand *src1, G4_InstOpts options,
                      bool addToInstList) {
    G4_ExecSize sz((unsigned char)execSize);
    return createInst(prd, op, mod, sat, sz, dst, src0, src1, options,
                      addToInstList);
  }

  // cond+sat+ternary
  G4_INST *createInst(G4_Predicate *prd, G4_opcode op, G4_CondMod *mod,
                      G4_Sat sat, G4_ExecSize execSize, G4_DstRegRegion *dst,
                      G4_Operand *src0, G4_Operand *src1, G4_Operand *src2,
                      G4_InstOpts options, bool addToInstList);

public:
  G4_INST *createIf(G4_Predicate *prd, G4_ExecSize execSize,
                    G4_InstOpts options);
  G4_INST *createElse(G4_ExecSize execSize, G4_InstOpts options);
  G4_INST *createEndif(G4_ExecSize execSize, G4_InstOpts options);
  G4_INST *createLabelInst(G4_Label *label, bool appendToInstList);
  G4_INST *createJmp(G4_Predicate *pred, G4_Operand *jmpTarget,
                     G4_InstOpts options, bool appendToInstList);
  G4_INST *createGoto(G4_Predicate *pred, G4_ExecSize execSize,
                      G4_Label *target, G4_InstOpts options,
                      bool addToInstList) {
    // jip is computed later during CFG construction
    return createCFInst(pred, G4_goto, execSize, nullptr, target, options,
                        addToInstList);
  }

  // ToDo: make createInternalInst() private as well and add wraper for them
  G4_INST *createInternalInst(G4_Predicate *prd, G4_opcode op, G4_CondMod *mod,
                              G4_Sat sat, G4_ExecSize execSize,
                              G4_DstRegRegion *dst, G4_Operand *src0,
                              G4_Operand *src1, G4_InstOpts options);

  G4_INST *createInternalInst(G4_Predicate *prd, G4_opcode op, G4_CondMod *mod,
                              G4_Sat sat, G4_ExecSize execSize,
                              G4_DstRegRegion *dst, G4_Operand *src0,
                              G4_Operand *src1, G4_Operand *src2,
                              G4_InstOpts options);

  G4_INST *createCFInst(G4_Predicate *prd, G4_opcode op, G4_ExecSize execSize,
                        G4_Label *jip, G4_Label *uip, G4_InstOpts options,
                        bool addToInstList);

  G4_INST *createInternalCFInst(G4_Predicate *prd, G4_opcode op,
                                G4_ExecSize execSize, G4_Label *jip,
                                G4_Label *uip, G4_InstOpts options);

  G4_InstSend *createSendInst(
      G4_Predicate *prd, G4_opcode op, G4_ExecSize execSize,
      G4_DstRegRegion *postDst, G4_SrcRegRegion *payload, G4_Operand *msg,
      G4_InstOpts options,
      G4_SendDescRaw *msgDesc, bool addToInstList);


  G4_InstSend *createInternalSendInst(
      G4_Predicate *prd, G4_opcode op, G4_ExecSize execSize,
      G4_DstRegRegion *postDst, G4_SrcRegRegion *payload, G4_Operand *msg,
      G4_InstOpts options,
      G4_SendDescRaw *msgDescs);

  G4_InstSend *createSplitSendInst(G4_Predicate *prd, G4_opcode op,
                                   G4_ExecSize execSize, G4_DstRegRegion *dst,
                                   G4_SrcRegRegion *src1, G4_SrcRegRegion *src2,
                                   G4_Operand *msg, G4_InstOpts options,
                                   G4_SendDescRaw *msgDesc, G4_Operand *src3,
                                   bool addToInstList);

  G4_InstSend *
  createInternalSplitSendInst(G4_ExecSize execSize, G4_DstRegRegion *dst,
                              G4_SrcRegRegion *src1, G4_SrcRegRegion *src2,
                              G4_Operand *msg, G4_InstOpts options,
                              G4_SendDescRaw *msgDesc, G4_Operand *src3);

  G4_INST *createMathInst(G4_Predicate *prd, G4_Sat sat, G4_ExecSize execSize,
                          G4_DstRegRegion *dst, G4_Operand *src0,
                          G4_Operand *src1, G4_MathOp mathOp,
                          G4_InstOpts options, bool addToInstList);

  G4_INST *createInternalMathInst(G4_Predicate *prd, G4_Sat sat,
                                  G4_ExecSize execSize, G4_DstRegRegion *dst,
                                  G4_Operand *src0, G4_Operand *src1,
                                  G4_MathOp mathOp, G4_InstOpts options);

  G4_INST *createIntrinsicInst(G4_Predicate *prd, Intrinsic intrinId,
                               G4_ExecSize execSize, G4_DstRegRegion *dst,
                               G4_Operand *src0, G4_Operand *src1,
                               G4_Operand *src2, G4_InstOpts options,
                               bool addToInstList);

  G4_INST *createInternalIntrinsicInst(G4_Predicate *prd, Intrinsic intrinId,
                                       G4_ExecSize execSize,
                                       G4_DstRegRegion *dst, G4_Operand *src0,
                                       G4_Operand *src1, G4_Operand *src2,
                                       G4_InstOpts options);

  G4_INST *createIntrinsicAddrMovInst(Intrinsic intrinId, G4_DstRegRegion *dst,
                                      G4_Operand *src0, G4_Operand *src1,
                                      G4_Operand *src2, G4_Operand *src3,
                                      G4_Operand *src4, G4_Operand *src5,
                                      G4_Operand *src6, G4_Operand *src7,
                                      bool addToInstList);

  G4_INST *createNop(G4_InstOpts options);
  G4_INST *createSync(G4_opcode syncOp, G4_Operand *src);

  G4_INST *createMov(G4_ExecSize execSize, G4_DstRegRegion *dst,
                     G4_Operand *src0, G4_InstOpts options,
                     bool appendToInstList);
  G4_INST *createMov(G4_Predicate *pred, G4_ExecSize execSize,
                     G4_DstRegRegion *dst, G4_Operand *src0,
                     G4_InstOpts options, bool appendToInstList);

  G4_INST *createBinOp(G4_opcode op, G4_ExecSize execSize, G4_DstRegRegion *dst,
                       G4_Operand *src0, G4_Operand *src1, G4_InstOpts options,
                       bool appendToInstList) {
    return createBinOp(nullptr, op, execSize, dst, src0, src1, options,
                       appendToInstList);
  }

  G4_INST *createBinOp(G4_Predicate *pred, G4_opcode op, G4_ExecSize execSize,
                       G4_DstRegRegion *dst, G4_Operand *src0, G4_Operand *src1,
                       G4_InstOpts options, bool appendToInstList = true);

  G4_INST *createMach(G4_ExecSize execSize, G4_DstRegRegion *dst,
                      G4_Operand *src0, G4_Operand *src1, G4_InstOpts options,
                      G4_Type accType);

  G4_INST *createMacl(G4_ExecSize execSize, G4_DstRegRegion *dst,
                      G4_Operand *src0, G4_Operand *src1, G4_InstOpts options,
                      G4_Type accType);

  G4_INST *createMadm(G4_ExecSize execSize, G4_DstRegRegion *dst,
                      G4_SrcRegRegion *src0, G4_SrcRegRegion *src1,
                      G4_SrcRegRegion *src2, G4_InstOpts options) {
    return createMadm(nullptr, execSize, dst, src0, src1, src2, options);
  }

  G4_INST *createMadm(G4_Predicate *pred, G4_ExecSize execSize,
                      G4_DstRegRegion *dst, G4_SrcRegRegion *src0,
                      G4_SrcRegRegion *src1, G4_SrcRegRegion *src2,
                      G4_InstOpts options);

  static G4_MathOp Get_MathFuncCtrl(ISA_Opcode op, G4_Type type);

  void resizePredefinedStackVars();

  template <typename T> T *duplicateOperand(T *opnd) {
    return static_cast<T *>(duplicateOpndImpl(opnd));
  }
  G4_Operand *duplicateOpndImpl(G4_Operand *opnd);

  G4_DstRegRegion *createSubDstOperand(G4_DstRegRegion *dst, uint16_t start,
                                       uint8_t size);
  G4_SrcRegRegion *createSubSrcOperand(G4_SrcRegRegion *src, uint16_t start,
                                       uint8_t size, uint16_t newVs,
                                       uint16_t newWd);
  G4_INST *makeSplittingInst(G4_INST *inst, G4_ExecSize execSize);

  G4_InstSend *createSendInst(G4_Predicate *pred, G4_DstRegRegion *postDst,
                              G4_SrcRegRegion *payload, G4_ExecSize execSize,
                              G4_SendDescRaw *msgDesc, G4_InstOpts options,
                              bool is_sendc);

  G4_InstSend *createSplitSendInst(G4_Predicate *pred, G4_DstRegRegion *dst,
                                   G4_SrcRegRegion *src1, G4_SrcRegRegion *src2,
                                   G4_ExecSize execSize,
                                   G4_SendDescRaw *msgDesc, G4_InstOpts options,
                                   bool is_sendc);

  // TODO: move to TranslateSendLdStLsc or elide VISA_Exec_Size (pre-convert)
  G4_SendDescRaw *createLscMsgDesc(LSC_OP op, LSC_SFID lscSfid,
                                   VISA_Exec_Size execSizeEnum,
                                   LSC_CACHE_OPTS cacheOpts, LSC_ADDR addr,
                                   LSC_DATA_SHAPE shape, G4_Operand *surface,
                                   uint32_t dstLen, uint32_t addrRegs,
                                   LdStAttrs otherAttrs);

  // ToDo: unify this with above function
  G4_SendDescRaw *createLscDesc(SFID sfid, uint32_t desc, uint32_t extDesc,
                                int src1Len, SendAccess access, G4_Operand *bti,
                                LdStAttrs otherAttrs);


  G4_InstSend *createLscSendInst(G4_Predicate *pred, G4_DstRegRegion *dst,
                                 G4_SrcRegRegion *src0, G4_SrcRegRegion *src1,
                                 G4_ExecSize execsize,
                                 G4_SendDescRaw *msgDesc,
                                 G4_InstOpts option, LSC_ADDR_TYPE addrType,
                                 unsigned surfOff, bool emitA0RegDef);


  G4_SrcRegRegion *getScratchSurfaceStatusIndex();

  void RestoreA0();

  G4_InstSend *
  createLscSendInstToScratch(G4_Predicate *pred, G4_DstRegRegion *dst,
                             G4_SrcRegRegion *src0, G4_SrcRegRegion *src1,
                             G4_ExecSize execSize, G4_SendDescRaw *msgDesc,
                             G4_InstOpts option, bool usesBti);

  G4_InstSend *
  createSplitSendToRenderTarget(G4_Predicate *pred, G4_DstRegRegion *dst,
                                G4_SrcRegRegion *src1, G4_SrcRegRegion *src2,
                                G4_SrcRegRegion *extDesc, G4_ExecSize execSize,
                                G4_SendDescRaw *msgDesc, G4_InstOpts option);

  G4_InstSend *createSendInst(G4_Predicate *pred, G4_DstRegRegion *postDst,
                              G4_SrcRegRegion *payload, unsigned regs2snd,
                              unsigned regs2rcv, G4_ExecSize execsize,
                              unsigned fc, SFID tf_id, bool head_present,
                              SendAccess access, G4_Operand *bti,
                              G4_Operand *sti, G4_InstOpts options,
                              bool is_sendc);

  G4_InstSend *createSplitSendInst(G4_Predicate *pred, G4_DstRegRegion *dst,
                                   G4_SrcRegRegion *src1, unsigned regs2snd1,
                                   G4_SrcRegRegion *src2, unsigned regs2snd2,
                                   unsigned regs2rcv, G4_ExecSize execSize,
                                   unsigned fc, SFID tf_id, bool head_present,
                                   SendAccess access, G4_Operand *bti,
                                   G4_Operand *sti, G4_InstOpts option,
                                   bool is_sendc);

  // helper functions
  G4_Declare *createSendPayloadDcl(unsigned num_elt, G4_Type type);

  void createMovR0Inst(G4_Declare *dcl, short refOff, short subregOff,
                       bool use_nomask = false,
                       G4_InstOpts options = InstOpt_NoOpt);

  void createMovInst(G4_Declare *dcl, short refOff, short subregOff,
                     G4_ExecSize execsize, G4_Predicate *pred,
                     G4_CondMod *condMod, G4_Operand *src_opnd,
                     bool use_nomask = false,
                     G4_InstOpts options = InstOpt_NoOpt);
  void createAddInst(G4_Declare *dcl, short regOff, short subregOff,
                     G4_ExecSize execSize, G4_Predicate *pred,
                     G4_CondMod *condMod, G4_Operand *src0_opnd,
                     G4_Operand *src1_opnd, G4_InstOption options);
  void createMovSendSrcInst(G4_Declare *dcl, short refOff, short subregOff,
                            unsigned num_dword, G4_Operand *src_opnd,
                            G4_InstOpts options);

  // short hand for creating a dstRegRegion
  G4_DstRegRegion *createDstRegRegion(G4_Declare *dcl, unsigned short hstride);
  G4_SrcRegRegion *createSrcRegRegion(G4_Declare *dcl, const RegionDesc *rd);

  // Create a null dst with scalar region and the given type
  G4_DstRegRegion *createNullDst(G4_Type dstType);

  // Create a null src with scalar region and the given type
  G4_SrcRegRegion *createNullSrc(G4_Type dstType);

  G4_DstRegRegion *checkSendDst(G4_DstRegRegion *dst_opnd);

  G4_INST *createDpasInst(G4_opcode opc, G4_ExecSize execSize,
                          G4_DstRegRegion *dst, G4_Operand *src0,
                          G4_Operand *src1, G4_Operand *src2,
                          G4_Operand *src3, G4_Operand *src4,
                          G4_InstOpts options, GenPrecision A, GenPrecision W,
                          uint8_t D, uint8_t C, bool addToInstList);

  G4_INST *createInternalDpasInst(G4_opcode opc, G4_ExecSize execSize,
                                  G4_DstRegRegion *dst, G4_Operand *src0,
                                  G4_Operand *src1, G4_Operand *src2,
                                  G4_InstOpts options, GenPrecision A,
                                  GenPrecision W, uint8_t D, uint8_t C,
                                  G4_Operand *src3 = nullptr,
                                  G4_Operand *src4 = nullptr);

  G4_INST *createBfnInst(uint8_t booleanFuncCtrl, G4_Predicate *prd,
                         G4_CondMod *mod, G4_Sat sat, G4_ExecSize execSize,
                         G4_DstRegRegion *dst, G4_Operand *src0,
                         G4_Operand *src1, G4_Operand *src2,
                         G4_InstOpts options, bool addToInstLis);

  G4_INST *createInternalBfnInst(uint8_t booleanFuncCtrl, G4_Predicate *prd,
                                 G4_CondMod *mod, G4_Sat sat,
                                 G4_ExecSize execSize, G4_DstRegRegion *dst,
                                 G4_Operand *src0, G4_Operand *src1,
                                 G4_Operand *src2, G4_InstOpts options);

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
  int translateVISAAddrInst(ISA_Opcode opcode, VISA_Exec_Size execSize,
                            VISA_EMask_Ctrl emask, G4_DstRegRegion *dst_opnd,
                            G4_Operand *src0_opnd, G4_Operand *src1_opnd);

  int translateVISAArithmeticInst(ISA_Opcode opcode, VISA_Exec_Size execSize,
                                  VISA_EMask_Ctrl emask, G4_Predicate *predOpnd,
                                  G4_Sat saturate, G4_CondMod *condMod,
                                  G4_DstRegRegion *dstOpnd,
                                  G4_Operand *src0Opnd, G4_Operand *src1Opnd,
                                  G4_Operand *src2Opnd,
                                  G4_DstRegRegion *carryBorrow);

  int translateVISADpasInst(VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask,
                            G4_opcode opc, G4_DstRegRegion *dstOpnd,
                            G4_SrcRegRegion *src0Opnd,
                            G4_SrcRegRegion *src1Opnd,
                            G4_SrcRegRegion *src2Opnd,
                            G4_SrcRegRegion *src3Opnd,
                            G4_SrcRegRegion *src4Opnd, GenPrecision A,
                            GenPrecision W, uint8_t D, uint8_t C);
  int translateVISABfnInst(uint8_t booleanFuncCtrl,
                           VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask,
                           G4_Predicate *predOpnd, G4_Sat saturate,
                           G4_CondMod *condMod, G4_DstRegRegion *dstOpnd,
                           G4_Operand *src0Opnd, G4_Operand *src1Opnd,
                           G4_Operand *src2Opnd);

  int translateVISACompareInst(ISA_Opcode opcode, VISA_Exec_Size execSize,
                               VISA_EMask_Ctrl emask, VISA_Cond_Mod relOp,
                               G4_Declare *predDst, G4_Operand *src0_opnd,
                               G4_Operand *src1_opnd);

  int translateVISACompareInst(ISA_Opcode opcode, VISA_Exec_Size execSize,
                               VISA_EMask_Ctrl emask, VISA_Cond_Mod relOp,
                               G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd,
                               G4_Operand *src1Opnd);

  int translateVISALogicInst(ISA_Opcode opcode, G4_Predicate *pred_opnd,
                             G4_Sat saturate, VISA_Exec_Size executionSize,
                             VISA_EMask_Ctrl emask, G4_DstRegRegion *dst,
                             G4_Operand *src0, G4_Operand *src1,
                             G4_Operand *src2, G4_Operand *src3);

  int translateVISADataMovementInst(ISA_Opcode opcode,
                                    CISA_MIN_MAX_SUB_OPCODE subOpcode,
                                    G4_Predicate *pred_opnd,
                                    VISA_Exec_Size executionSize,
                                    VISA_EMask_Ctrl emask, G4_Sat saturate,
                                    G4_DstRegRegion *dst, G4_Operand *src0,
                                    G4_Operand *src1);

  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  // Control flow, function call, and branching ops are located in
  // VisaToG4/TranslateBranch.cpp
  int translateVISACFSwitchInst(G4_Operand *indexOpnd, uint8_t numLabels,
                                G4_Label **lab);

  int translateVISACFLabelInst(G4_Label *lab);

  int translateVISACFCallInst(VISA_Exec_Size execsize, VISA_EMask_Ctrl emask,
                              G4_Predicate *predOpnd, G4_Label *lab);

  int translateVISACFJumpInst(G4_Predicate *predOpnd, G4_Label *lab);

  int translateVISACFFCallInst(VISA_Exec_Size execsize, VISA_EMask_Ctrl emask,
                               G4_Predicate *predOpnd, std::string funcName,
                               uint8_t argSize, uint8_t returnSize);

  int translateVISACFIFCallInst(VISA_Exec_Size execsize, VISA_EMask_Ctrl emask,
                                G4_Predicate *predOpnd, bool isUniform,
                                G4_Operand *funcAddr, uint8_t argSize,
                                uint8_t returnSize);

  int translateVISACFSymbolInst(const std::string &symbolName,
                                G4_DstRegRegion *dst);

  int translateVISACFFretInst(VISA_Exec_Size execsize, VISA_EMask_Ctrl emask,
                              G4_Predicate *predOpnd);

  int translateVISACFRetInst(VISA_Exec_Size execsize, VISA_EMask_Ctrl emask,
                             G4_Predicate *predOpnd);

  int translateVISAGotoInst(G4_Predicate *predOpnd,
                            VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask,
                            G4_Label *label);

  ///////////////////////////////////////////////////////////////////////////
  // members related to special math sequences VisaToG4/TranslateMath.cpp
  void expandFdiv(G4_ExecSize exsize, G4_Predicate *predOpnd, G4_Sat saturate,
                  G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd,
                  G4_Operand *src1Opnd, uint32_t instOpt);

  void expandPow(G4_ExecSize exsize, G4_Predicate *predOpnd, G4_Sat saturate,
                 G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd,
                 G4_Operand *src1Opnd, uint32_t instOpt);

  int translateVISAArithmeticDoubleInst(
      ISA_Opcode opcode, VISA_Exec_Size execSize, VISA_EMask_Ctrl emask,
      G4_Predicate *predOpnd, G4_Sat saturate, G4_DstRegRegion *dstOpnd,
      G4_Operand *src0Opnd, G4_Operand *src1Opnd);

  int translateVISAArithmeticSingleDivideIEEEInst(
      ISA_Opcode opcode, VISA_Exec_Size execSize, VISA_EMask_Ctrl emask,
      G4_Predicate *predOpnd, G4_Sat saturate, G4_CondMod *condMod,
      G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd, G4_Operand *src1Opnd);

  int translateVISAArithmeticSingleSQRTIEEEInst(
      ISA_Opcode opcode, VISA_Exec_Size execSize, VISA_EMask_Ctrl emask,
      G4_Predicate *predOpnd, G4_Sat saturate, G4_CondMod *condMod,
      G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd);

  int translateVISAArithmeticDoubleSQRTInst(
      ISA_Opcode opcode, VISA_Exec_Size execSize, VISA_EMask_Ctrl emask,
      G4_Predicate *predOpnd, G4_Sat saturate, G4_CondMod *condMod,
      G4_DstRegRegion *dstOpnd, G4_Operand *src0Opnd);

  int translateVISAInvmRsqtmInst(ISA_Opcode opcode,
      VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask,
      G4_Predicate *predOpnd, G4_Sat saturate, G4_DstRegRegion *dstOpnd,
      VISA_PredVar *predDst, G4_Operand *src0Opnd, G4_Operand *Src1Opnd);

  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  // Members related miscellaneous instructions that don't fit any other
  // category are in VisaToG4/TranslateMisc.cpp
  //
  // (As stated above some of this could move to BuildIRImpl.cpp.)
  static G4_ExecSize toExecSize(VISA_Exec_Size execSize);

  VISA_Exec_Size roundUpExecSize(VISA_Exec_Size execSize);

  G4_Declare *getImmDcl(G4_Imm *val, int numElt);

  //
  // 'copyExecSize' and preparePayload's batchExSize together provide
  // the execSize of copying instruction.
  //     'copyExecSize' of PayloadSource is used for header only for now.
  //     If 'copyExecSize' is present, use it; otherwise, use batchExSize
  //     of preparePayload for copying.
  //
  //  For example,
  //     send(4|M0)   nullptr  addr:a32 data:ud ...
  //  will be changed to
  //     mov(8|M0)    msgPayload(0,0) <1;1,0> header
  //     mov(4|M0)    msgPayload(1,0) <1;1,0> addr
  //     mov(4|M0)    msgPayload(2,0) <1;1,0> data
  //     send(4|M0)   nullptr   msgPayload ...
  //  where 'copyExecSize' will be 8 and batchExSize = 4.
  //
  struct PayloadSource {
    G4_SrcRegRegion *opnd = nullptr;
    uint32_t numElts = 0; // 'opnd's size in msg payload
    G4_InstOpts instOpt = {};
    G4_ExecSize copyExecSize = {}; // used for copy if given.

    PayloadSource() : copyExecSize(g4::SIMD_UNDEFINED) {}
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
  ///                         specifies the SIMD width of copy except when
  ///                         'copyExecSize' of PayloadSource is defined. And
  ///                         in the case 'copyExecSize is defined, it's used as
  ///                         execsize for copy.
  /// \param splitSendEnabled Whether feature split-send is available. When
  ///                         feature split-send is available, this function
  ///                         will check whether two consecutive regions
  ///                         could be prepared instead of one to take
  ///                         advantage of split-send.
  /// \param srcs             The array of sources (including header if
  ///                         present).
  /// \param len              The length of the array of sources.
  ///
  void preparePayload(G4_SrcRegRegion *msgs[2], unsigned sizes[2],
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
  G4_SrcRegRegion *
  coalescePayload(G4_Predicate *pred,
                  unsigned alignSourcesTo, unsigned alignPayloadTo,
                  uint32_t payloadSize, uint32_t srcSize,
                  std::initializer_list<G4_SrcRegRegion *> srcs,
                  VISA_EMask_Ctrl emask);

  // emask is InstOption
  void Copy_SrcRegRegion_To_Payload(G4_Declare *payload, unsigned int &regOff,
                                    G4_SrcRegRegion *src, G4_ExecSize execSize,
                                    uint32_t emask,
                                    G4_Predicate *pred = nullptr);
  unsigned int getByteOffsetSrcRegion(G4_SrcRegRegion *srcRegion);

  // only used in TranslateSend3D, maybe consider moving there if no
  // one else uses them.
  bool checkIfRegionsAreConsecutive(G4_SrcRegRegion *first,
                                    G4_SrcRegRegion *second,
                                    G4_ExecSize execSize);
  bool checkIfRegionsAreConsecutive(G4_SrcRegRegion *first,
                                    G4_SrcRegRegion *second,
                                    G4_ExecSize execSize, G4_Type type);

  int generateDebugInfoPlaceholder(); // TODO: move to BuildIRImpl.cpp?
                                      //
  // handle breakpoint instruction
  int translateBreakpointInstruction();

  // legitimiately belongs in Misc
  int translateVISALifetimeInst(bool isStart, G4_Operand *var);

  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  // members related to 3D and sampler ops are in VisaToG4/TranslateSend3D.cpp
  int translateVISASampleInfoInst(VISA_Exec_Size executionSize,
                                  VISA_EMask_Ctrl emask, ChannelMask chMask,
                                  G4_Operand *surface, G4_DstRegRegion *dst);

  int translateVISAResInfoInst(VISA_Exec_Size executionSize,
                               VISA_EMask_Ctrl emask, ChannelMask chMask,
                               G4_Operand *surface, G4_SrcRegRegion *lod,
                               G4_DstRegRegion *dst);

  int translateVISAURBWrite3DInst(
      G4_Predicate *pred, VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask,
      uint8_t numOut, uint16_t globalOffset, G4_SrcRegRegion *channelMask,
      G4_SrcRegRegion *urbHandle, G4_SrcRegRegion *perSlotOffset,
      G4_SrcRegRegion *vertexData);

  int translateVISARTWrite3DInst(G4_Predicate *pred,
                                 VISA_Exec_Size executionSize,
                                 VISA_EMask_Ctrl emask, G4_Operand *surface,
                                 G4_SrcRegRegion *r1HeaderOpnd,
                                 G4_Operand *rtIndex, vISA_RT_CONTROLS cntrls,
                                 G4_SrcRegRegion *sampleIndexOpnd,
                                 G4_Operand *cpsCounter, unsigned int numParms,
                                 G4_SrcRegRegion **msgOpnds);



  int translateLscTypedBlock2DInst(LSC_OP op, LSC_CACHE_OPTS cacheOpts,
                                   LSC_ADDR_TYPE addrModel,
                                   LSC_DATA_SHAPE_TYPED_BLOCK2D shape,
                                   G4_Operand *surface,
                                   unsigned ssIdx,
                                   G4_DstRegRegion *dstData,
                                   G4_Operand *xOffset, // block start offset x
                                   G4_Operand *yOffset, // block start offset y
                                   int xImmOffset, // x immediate offset
                                   int yImmOffset, // y immediate offset
                                   G4_SrcRegRegion *src1Data);

  G4_SrcRegRegion *
  lscBuildTypedBlock2DPayload(LSC_DATA_SHAPE_TYPED_BLOCK2D dataShape2D,
                              G4_Operand *xOffset, G4_Operand *yOffset,
                              int immOffX, int immOffY);

  int translateLscUntypedAppendCounterAtomicInst(
      LSC_OP op, G4_Predicate *pred, VISA_Exec_Size visaExecSize,
      VISA_EMask_Ctrl execCtrl, LSC_CACHE_OPTS cacheOpts,
      LSC_ADDR_TYPE addrType, LSC_DATA_SHAPE dataShape,
      G4_Operand *surface, unsigned ssIdx,
      G4_DstRegRegion *dst, G4_SrcRegRegion *srcData);

  int splitSampleInst(VISASampler3DSubOpCode actualop, bool pixelNullMask,
                      bool cpsEnable, G4_Predicate *pred,
                      ChannelMask srcChannel, int numChannels,
                      G4_Operand *aoffimmi, G4_Operand *sampler,
                      G4_Operand *surface,
                      G4_Operand *pairedResource,
                      G4_DstRegRegion *dst, VISA_EMask_Ctrl emask,
                      bool useHeader,
                      unsigned numRows, // msg length for each simd8
                      unsigned int numParms, G4_SrcRegRegion **params,
                      bool uniformSampler = true);

  void doSamplerHeaderMove(G4_Declare *header, G4_Operand *sampler);
  void doPairedResourceHeaderMove(G4_Declare *header,
                                  G4_Operand *pairedResource);
  G4_Declare *getSamplerHeader(bool isBindlessSampler, bool samplerIndexGE16);
  uint32_t getSamplerResponseLength(int numChannels, bool isFP16, int execSize,
                                    bool pixelNullMask, bool nullDst);

  int translateVISASampler3DInst(
      VISASampler3DSubOpCode actualop, bool pixelNullMask, bool cpsEnable,
      bool uniformSampler, G4_Predicate *pred, VISA_Exec_Size executionSize,
      VISA_EMask_Ctrl emask, ChannelMask srcChannel, G4_Operand *aoffimmi,
      G4_Operand *sampler, G4_Operand *surface,
      G4_Operand *pairedSurface,
      G4_DstRegRegion *dst, unsigned int numParms, G4_SrcRegRegion **params);


  int translateVISALoad3DInst(VISASampler3DSubOpCode actualop,
                              bool pixelNullMask, G4_Predicate *pred,
                              VISA_Exec_Size exeuctionSize, VISA_EMask_Ctrl em,
                              ChannelMask channelMask, G4_Operand *aoffimmi,
                              G4_Operand *surface, G4_Operand *pairedSurface,
                              G4_DstRegRegion *dst, uint8_t numOpnds,
                              G4_SrcRegRegion **opndArray);

  int translateVISAGather3dInst(VISASampler3DSubOpCode actualop,
                                bool pixelNullMask, G4_Predicate *pred,
                                VISA_Exec_Size exeuctionSize,
                                VISA_EMask_Ctrl em, ChannelMask channelMask,
                                G4_Operand *aoffimmi, G4_Operand *sampler,
                                G4_Operand *surface, G4_Operand *pairedSurface,
                                G4_DstRegRegion *dst, unsigned int numOpnds,
                                G4_SrcRegRegion **opndArray);

  int translateVISASamplerNormInst(G4_Operand *surface, G4_Operand *sampler,
                                   ChannelMask channel,
                                   unsigned numEnabledChannels,
                                   G4_Operand *deltaUOpnd, G4_Operand *uOffOpnd,
                                   G4_Operand *deltaVOpnd, G4_Operand *vOffOpnd,
                                   G4_DstRegRegion *dst_opnd);

  int translateVISASamplerInst(unsigned simdMode, G4_Operand *surface,
                               G4_Operand *sampler, ChannelMask channel,
                               unsigned numEnabledChannels,
                               G4_Operand *uOffOpnd, G4_Operand *vOffOpnd,
                               G4_Operand *rOffOpnd, G4_DstRegRegion *dstOpnd);

  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  // basic load and store (type and untyped) are located in
  // VisaToG4/TranslateSendLdStLegacy.cpp

  // IsHeaderOptional - Check whether the message header is optional.
  bool isMessageHeaderOptional(G4_Operand *surface, G4_Operand *Offset) const;

  // IsStatelessSurface - Check whether the give surface is statelesss surface.
  static bool isStatelessSurface(const G4_Operand *surface) {
    // So far, it's only reliable to check an immediate surface.
    return surface->isImm() && (surface->asImm()->getImm() == PREDEF_SURF_255 ||
                                surface->asImm()->getImm() == PREDEF_SURF_253);
  }

  std::tuple<G4_SrcRegRegion *, uint32_t, uint32_t>
  constructSrcPayloadRenderTarget(vISA_RT_CONTROLS cntrls,
                                  G4_SrcRegRegion **msgOpnds,
                                  unsigned int numMsgOpnds,
                                  G4_ExecSize execSize, G4_InstOpts instOpt);

  std::tuple<G4_SrcRegRegion *, uint32_t, uint32_t>
  constructSrcPayloadDualRenderTarget(vISA_RT_CONTROLS cntrls,
                                      G4_SrcRegRegion **msgOpnds,
                                      unsigned int numMsgOpnds,
                                      G4_ExecSize execSize,
                                      G4_InstOpts instOpt);

  int translateVISAQWGatherInst(VISA_Exec_Size executionSize,
                                VISA_EMask_Ctrl emask, G4_Predicate *pred,
                                VISA_SVM_Block_Num numBlocks,
                                G4_SrcRegRegion *surface,
                                G4_SrcRegRegion *addresses,
                                G4_DstRegRegion *dst);

  int translateVISAQWScatterInst(VISA_Exec_Size executionSize,
                                 VISA_EMask_Ctrl emask, G4_Predicate *pred,
                                 VISA_SVM_Block_Num numBlocks,
                                 G4_SrcRegRegion *surface,
                                 G4_SrcRegRegion *addresses,
                                 G4_SrcRegRegion *src);

  uint32_t setOwordForDesc(uint32_t desc, int numOword,
                           bool isSLM = false) const;
  int translateVISAOwordLoadInst(ISA_Opcode opcode, bool modified,
                                 G4_Operand *surface, VISA_Oword_Num size,
                                 G4_Operand *offOpnd, G4_DstRegRegion *dstOpnd);

  int translateVISAOwordStoreInst(G4_Operand *surface, VISA_Oword_Num size,
                                  G4_Operand *offOpnd,
                                  G4_SrcRegRegion *srcOpnd);

  int translateVISAGatherInst(VISA_EMask_Ctrl emask, bool modified,
                              GATHER_SCATTER_ELEMENT_SIZE eltSize,
                              VISA_Exec_Size executionSize, G4_Operand *surface,
                              G4_Operand *gOffOpnd, G4_SrcRegRegion *eltOFfOpnd,
                              G4_DstRegRegion *dstOpnd);

  int translateVISAScatterInst(VISA_EMask_Ctrl emask,
                               GATHER_SCATTER_ELEMENT_SIZE eltSize,
                               VISA_Exec_Size executionSize,
                               G4_Operand *surface, G4_Operand *gOffOpnd,
                               G4_SrcRegRegion *eltOffOpnd,
                               G4_SrcRegRegion *srcOpnd);

  int translateVISAGather4Inst(VISA_EMask_Ctrl emask, bool modified,
                               ChannelMask chMask, VISA_Exec_Size executionSize,
                               G4_Operand *surface, G4_Operand *gOffOpnd,
                               G4_SrcRegRegion *eltOffOpnd,
                               G4_DstRegRegion *dstOpnd);

  int translateVISAScatter4Inst(VISA_EMask_Ctrl emask, ChannelMask chMask,
                                VISA_Exec_Size executionSize,
                                G4_Operand *surface, G4_Operand *gOffOpnd,
                                G4_SrcRegRegion *eltOffOpnd,
                                G4_SrcRegRegion *srcOpnd);

  int translateVISADwordAtomicInst(VISAAtomicOps subOpc, bool is16Bit,
                                   G4_Predicate *pred, VISA_Exec_Size execSize,
                                   VISA_EMask_Ctrl eMask, G4_Operand *surface,
                                   G4_SrcRegRegion *offsets,
                                   G4_SrcRegRegion *src0, G4_SrcRegRegion *src1,
                                   G4_DstRegRegion *dst);

  void buildTypedSurfaceAddressPayload(G4_SrcRegRegion *u, G4_SrcRegRegion *v,
                                       G4_SrcRegRegion *r, G4_SrcRegRegion *lod,
                                       G4_ExecSize execSize,
                                       G4_InstOpts instOpt,
                                       PayloadSource sources[], uint32_t &len);

  int translateVISAGather4TypedInst(G4_Predicate *pred, VISA_EMask_Ctrl emask,
                                    ChannelMask chMask, G4_Operand *surfaceOpnd,
                                    VISA_Exec_Size executionSize,
                                    G4_SrcRegRegion *uOffsetOpnd,
                                    G4_SrcRegRegion *vOffsetOpnd,
                                    G4_SrcRegRegion *rOffsetOpnd,
                                    G4_SrcRegRegion *lodOpnd,
                                    G4_DstRegRegion *dstOpnd);

  int translateVISAScatter4TypedInst(
      G4_Predicate *pred, VISA_EMask_Ctrl emask, ChannelMask chMask,
      G4_Operand *surfaceOpnd, VISA_Exec_Size executionSize,
      G4_SrcRegRegion *uOffsetOpnd, G4_SrcRegRegion *vOffsetOpnd,
      G4_SrcRegRegion *rOffsetOpnd, G4_SrcRegRegion *lodOpnd,
      G4_SrcRegRegion *srcOpnd);

  int translateVISATypedAtomicInst(
      VISAAtomicOps atomicOp, bool is16Bit, G4_Predicate *pred,
      VISA_EMask_Ctrl emask, VISA_Exec_Size execSize, G4_Operand *surface,
      G4_SrcRegRegion *uOffsetOpnd, G4_SrcRegRegion *vOffsetOpnd,
      G4_SrcRegRegion *rOffsetOpnd, G4_SrcRegRegion *lodOpnd,
      G4_SrcRegRegion *src0, G4_SrcRegRegion *src1, G4_DstRegRegion *dst);

  void applySideBandOffset(G4_Operand *sideBand,
                           const G4_SendDescRaw *sendMsgDesc);

  int translateVISAGather4ScaledInst(
      G4_Predicate *pred, VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask,
      ChannelMask chMask, G4_Operand *surface, G4_Operand *globalOffset,
      G4_SrcRegRegion *offsets, G4_DstRegRegion *dst);

  int translateVISAScatter4ScaledInst(
      G4_Predicate *pred, VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask,
      ChannelMask chMask, G4_Operand *surface, G4_Operand *globalOffset,
      G4_SrcRegRegion *offsets, G4_SrcRegRegion *src);

  int translateGather4Inst(G4_Predicate *pred, VISA_Exec_Size execSize,
                           VISA_EMask_Ctrl eMask, ChannelMask chMask,
                           G4_Operand *surface, G4_Operand *globalOffset,
                           G4_SrcRegRegion *offsets, G4_DstRegRegion *dst);

  int translateScatter4Inst(G4_Predicate *pred, VISA_Exec_Size execSize,
                            VISA_EMask_Ctrl eMask, ChannelMask chMask,
                            G4_Operand *surface, G4_Operand *globalOffset,
                            G4_SrcRegRegion *offsets, G4_SrcRegRegion *src);

  int translateVISAGatherScaledInst(
      G4_Predicate *pred, VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask,
      VISA_SVM_Block_Num numBlocks, G4_Operand *surface,
      G4_Operand *globalOffset, G4_SrcRegRegion *offsets, G4_DstRegRegion *dst);

  int translateVISAScatterScaledInst(
      G4_Predicate *pred, VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask,
      VISA_SVM_Block_Num numBlocks, G4_Operand *surface,
      G4_Operand *globalOffset, G4_SrcRegRegion *offsets, G4_SrcRegRegion *src);

  int translateByteGatherInst(G4_Predicate *pred, VISA_Exec_Size execSize,
                              VISA_EMask_Ctrl eMask,
                              VISA_SVM_Block_Num numBlocks, G4_Operand *surface,
                              G4_Operand *globalOffset,
                              G4_SrcRegRegion *offsets, G4_DstRegRegion *dst);

  int translateByteScatterInst(G4_Predicate *pred, VISA_Exec_Size execSize,
                               VISA_EMask_Ctrl eMask,
                               VISA_SVM_Block_Num numBlocks,
                               G4_Operand *surface, G4_Operand *globalOffset,
                               G4_SrcRegRegion *offsets, G4_SrcRegRegion *src);

  int translateVISASVMBlockReadInst(VISA_Oword_Num numOword, bool unaligned,
                                    G4_Operand *address, G4_DstRegRegion *dst);

  int translateVISASVMBlockWriteInst(VISA_Oword_Num numOword,
                                     G4_Operand *address, G4_SrcRegRegion *src);

  int translateVISASVMScatterReadInst(VISA_Exec_Size executionSize,
                                      VISA_EMask_Ctrl emask, G4_Predicate *pred,
                                      VISA_SVM_Block_Type blockSize,
                                      VISA_SVM_Block_Num numBlocks,
                                      G4_SrcRegRegion *addresses,
                                      G4_DstRegRegion *dst);

  int translateVISASVMScatterWriteInst(
      VISA_Exec_Size executionSize, VISA_EMask_Ctrl emask, G4_Predicate *pred,
      VISA_SVM_Block_Type blockSize, VISA_SVM_Block_Num numBlocks,
      G4_SrcRegRegion *addresses, G4_SrcRegRegion *src);

  int translateVISASVMAtomicInst(VISAAtomicOps op, unsigned short bitwidth,
                                 VISA_Exec_Size executionSize,
                                 VISA_EMask_Ctrl emask, G4_Predicate *pred,
                                 G4_SrcRegRegion *addresses,
                                 G4_SrcRegRegion *src0, G4_SrcRegRegion *src1,
                                 G4_DstRegRegion *dst);

  // return globalOffset + offsets as a contiguous operand
  G4_SrcRegRegion *getSVMOffset(G4_Operand *globalOffset,
                                G4_SrcRegRegion *offsets, uint16_t exSize,
                                G4_Predicate *pred, uint32_t mask);

  int translateSVMGather4Inst(VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask,
                              ChannelMask chMask, G4_Predicate *pred,
                              G4_Operand *address, G4_SrcRegRegion *offsets,
                              G4_DstRegRegion *dst);

  int translateSVMScatter4Inst(VISA_Exec_Size execSize, VISA_EMask_Ctrl eMask,
                               ChannelMask chMask, G4_Predicate *pred,
                               G4_Operand *address, G4_SrcRegRegion *offsets,
                               G4_SrcRegRegion *src);

  int translateVISASVMGather4ScaledInst(VISA_Exec_Size execSize,
                                        VISA_EMask_Ctrl eMask,
                                        ChannelMask chMask, G4_Predicate *pred,
                                        G4_Operand *address,
                                        G4_SrcRegRegion *offsets,
                                        G4_DstRegRegion *dst);

  int translateVISASVMScatter4ScaledInst(VISA_Exec_Size execSize,
                                         VISA_EMask_Ctrl eMask,
                                         ChannelMask chMask, G4_Predicate *pred,
                                         G4_Operand *address,
                                         G4_SrcRegRegion *offsets,
                                         G4_SrcRegRegion *src);

  // Minimum execution size for LSC on this platform
  // Minimum is generally half the full size except rare cases.
  // Full LSC SIMD size for PVC derivatives is 32, and 16 for DG2 derivatives
  G4_ExecSize lscMinExecSize(LSC_SFID lscSfid) const;

  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  // New LSC-based load and store (type and untyped) are located in
  // VisaToG4/TranslateSendLdStLsc.cpp
  int translateLscUntypedInst(
      LSC_OP op, LSC_SFID lscSfid, G4_Predicate *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, LSC_ADDR addrInfo,
      LSC_DATA_SHAPE shape,
      G4_Operand *surface, // surface/bti
      unsigned ssIdx, // optional BSSO index
      G4_DstRegRegion *dstData, G4_SrcRegRegion *src0AddrOrBlockY,
      G4_Operand *src0AddrStrideOrBlockX, // only for strided and block2d
      G4_SrcRegRegion *src1Data,          // store data/extra atomic operands
      G4_SrcRegRegion *src2Data           // only for fcas/icas
  );
  int translateLscUntypedBlock2DInst(
      LSC_OP op, LSC_SFID lscSfid, G4_Predicate *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts,
      LSC_DATA_SHAPE_BLOCK2D shape, G4_DstRegRegion *dstData,
      G4_Operand *src0Addrs[LSC_BLOCK2D_ADDR_PARAMS],
      G4_SrcRegRegion *src1Data, int xImmOff, int yImmOff);
  int translateLscUntypedBlock2DInst(
      LSC_OP op, LSC_SFID lscSfid, G4_Predicate *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts,
      LSC_DATA_SHAPE_BLOCK2D shape, G4_DstRegRegion *dstData,
      G4_Operand *src0Addr, G4_SrcRegRegion *src1Data, int xImmOff, int yImmOff);
  int translateLscTypedInst(
      LSC_OP op, G4_Predicate *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, LSC_ADDR_TYPE addrModel,
      LSC_ADDR_SIZE addrSize, LSC_DATA_SHAPE shape,
      G4_Operand *surface,      // surface/bti
      unsigned surfaceIndex,
      G4_DstRegRegion *dstData, // dst on load/atomic
      G4_SrcRegRegion *src0AddrUs, int uOff,
      G4_SrcRegRegion *src0AddrVs, int vOff,
      G4_SrcRegRegion *src0AddrRs, int rOff,
      G4_SrcRegRegion *src0AddrLODs,
      G4_SrcRegRegion *src1Data, // store data/extra atomic operands
      G4_SrcRegRegion *src2Data  // icas/fcas only
  );

  LSC_DATA_ELEMS lscGetElementNum(unsigned eNum) const;
  int lscEncodeAddrSize(LSC_ADDR_SIZE addr_size, uint32_t &desc,
                        int &status) const;
  uint32_t lscComputeAddrSizeBits(LSC_ADDR_SIZE addr_size, int &status) const;
  int lscEncodeDataSize(LSC_DATA_SIZE data_size, uint32_t &desc,
                        int &status) const;
  uint32_t lscComputeDataSize(LSC_DATA_SIZE data_size, int &status) const;
  int lscEncodeDataElems(LSC_DATA_ELEMS data_elems, uint32_t &desc,
                         int &status) const;
  void lscEncodeDataOrder(LSC_DATA_ORDER t, uint32_t &desc, int &status) const;
  void lscEncodeCachingOpts(const LscOpInfo &opInfo, LSC_CACHE_OPTS cacheOpts,
                            uint32_t &desc, int &status) const;
  void lscEncodeAddrType(LSC_ADDR_TYPE at, uint32_t &desc, int &status) const;


  G4_SrcRegRegion *lscBuildStridedPayload(G4_Predicate *pred,
                                          G4_SrcRegRegion *src0AddrBase,
                                          G4_Operand *src0AddrStride,
                                          int dataSizeBytes, int vecSize,
                                          bool transposed);
  G4_SrcRegRegion *lscBuildBlock2DPayload(LSC_DATA_SHAPE_BLOCK2D dataShape2D,
                                          G4_Predicate *pred,
                                          G4_Operand *src0Addrs[6],
                                          int immOffX, int immOffY);
  G4_SrcRegRegion *emulateImmOffsetBlock2D(G4_Predicate *pred,
                                           G4_Operand *addrPayload,
                                           int immOffX,
                                           int immOffY);

  //
  // LSC allows users to pass an immediate scale and immediate addend.
  // Hardware may be able to take advantage of that if they satisfy
  // various constraints.  This also broadcasts if needed.
  G4_SrcRegRegion *lscLoadEffectiveAddress(
      LSC_OP lscOp, LSC_SFID lscSfid, G4_Predicate *pred, G4_ExecSize execSize,
      VISA_EMask_Ctrl execCtrl, LSC_ADDR addrInfo, int bytesPerDataElem,
      const G4_Operand *surface, G4_SrcRegRegion *addr, uint32_t &exDesc,
      uint32_t &exDescImmOff);

  // try and promote an immediate offset to LSC descriptor
  // (doesn't work for block2d)
  bool lscTryPromoteImmOffToExDesc(LSC_OP lscOp, LSC_SFID lscSfid,
                                   LSC_ADDR addrInfo, int bytesPerDataElem,
                                   const G4_Operand *surface,
                                   uint32_t &exDescImm, uint32_t &exDescImmOff);

  G4_SrcRegRegion *lscCheckRegion(G4_Predicate *pred, G4_ExecSize execSize,
                                  VISA_EMask_Ctrl execCtrl,
                                  G4_SrcRegRegion *src);

  G4_SrcRegRegion *lscMulAdd(G4_Predicate *pred, G4_ExecSize execSize,
                             VISA_EMask_Ctrl execCtrl, G4_SrcRegRegion *src,
                             int16_t mulImm16, int64_t addImm64);
  G4_SrcRegRegion *lscMul(G4_Predicate *pred, G4_ExecSize execSize,
                          VISA_EMask_Ctrl execCtrl, G4_SrcRegRegion *src,
                          int16_t mulImm16);
  G4_SrcRegRegion *lscAdd(G4_Predicate *pred, G4_ExecSize execSize,
                          VISA_EMask_Ctrl execCtrl, G4_SrcRegRegion *src,
                          int64_t addImm64);
  G4_SrcRegRegion *lscAdd64AosNative(G4_Predicate *pred, G4_ExecSize execSize,
                                     VISA_EMask_Ctrl execCtrl,
                                     G4_SrcRegRegion *src, int64_t addImm64);
  G4_SrcRegRegion *lscAdd64AosEmu(G4_Predicate *pred, G4_ExecSize execSize,
                                  VISA_EMask_Ctrl execCtrl,
                                  G4_SrcRegRegion *src, int64_t addImm64);
  G4_SrcRegRegion *lscMul64Aos(G4_Predicate *pred, G4_ExecSize execSize,
                               VISA_EMask_Ctrl execCtrl, G4_SrcRegRegion *src,
                               int16_t mulImm16);

  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  // members related to media and VME are in VisaToG4/TranslateSendMedia.cpp
  int translateVISAMediaLoadInst(MEDIA_LD_mod mod, G4_Operand *surface,
                                 unsigned planeID, unsigned blockWidth,
                                 unsigned blockHeight, G4_Operand *xOffOpnd,
                                 G4_Operand *yOffOpnd,
                                 G4_DstRegRegion *dst_opnd);

  int translateVISAMediaStoreInst(MEDIA_ST_mod mod, G4_Operand *surface,
                                  unsigned planeID, unsigned blockWidth,
                                  unsigned blockHeight, G4_Operand *xOffOpnd,
                                  G4_Operand *yOffOpnd,
                                  G4_SrcRegRegion *srcOpnd);

  int translateVISAVmeImeInst(uint8_t stream_mode, uint8_t search_ctrl,
                              G4_Operand *surfaceOpnd, G4_Operand *uniInputOpnd,
                              G4_Operand *imeInputOpnd, G4_Operand *ref0Opnd,
                              G4_Operand *ref1Opnd, G4_Operand *costCenterOpnd,
                              G4_DstRegRegion *outputOpnd);

  int translateVISAVmeSicInst(G4_Operand *surfaceOpnd, G4_Operand *uniInputOpnd,
                              G4_Operand *sicInputOpnd,
                              G4_DstRegRegion *outputOpnd);

  int translateVISAVmeFbrInst(G4_Operand *surfaceOpnd,
                              G4_Operand *unitInputOpnd,
                              G4_Operand *fbrInputOpnd,
                              G4_Operand *fbrMbModOpnd,
                              G4_Operand *fbrSubMbShapeOpnd,
                              G4_Operand *fbrSubPredModeOpnd,
                              G4_DstRegRegion *outputOpnd);

  int translateVISAVmeIdmInst(G4_Operand *surfaceOpnd,
                              G4_Operand *unitInputOpnd,
                              G4_Operand *idmInputOpnd,
                              G4_DstRegRegion *outputOpnd);

  int translateVISASamplerVAGenericInst(
      G4_Operand *surface, G4_Operand *sampler, G4_Operand *uOffOpnd,
      G4_Operand *vOffOpnd, G4_Operand *vSizeOpnd, G4_Operand *hSizeOpnd,
      G4_Operand *mmfMode, unsigned char cntrl, unsigned char msgSeq,
      VA_fopcode fopcode, G4_DstRegRegion *dstOpnd, G4_Type dstType,
      unsigned dstSize, bool isBigKernel = false);

  int translateVISAAvsInst(G4_Operand *surface, G4_Operand *sampler,
                           ChannelMask channel, unsigned numEnabledChannels,
                           G4_Operand *deltaUOpnd, G4_Operand *uOffOpnd,
                           G4_Operand *deltaVOpnd, G4_Operand *vOffOpnd,
                           G4_Operand *u2dOpnd, G4_Operand *groupIDOpnd,
                           G4_Operand *verticalBlockNumberOpnd,
                           unsigned char cntrl, G4_Operand *v2dOpnd,
                           unsigned char execMode, G4_Operand *eifbypass,
                           G4_DstRegRegion *dstOpnd);

  int translateVISAVaSklPlusGeneralInst(
      ISA_VA_Sub_Opcode sub_opcode, G4_Operand *surface, G4_Operand *sampler,
      unsigned char mode, unsigned char functionality, G4_Operand *uOffOpnd,
      G4_Operand *vOffOpnd,
      // 1pixel convolve
      G4_Operand *offsetsOpnd,

      // FloodFill
      G4_Operand *loopCountOpnd, G4_Operand *pixelHMaskOpnd,
      G4_Operand *pixelVMaskLeftOpnd, G4_Operand *pixelVMaskRightOpnd,

      // LBP Correlation
      G4_Operand *disparityOpnd,

      // Correlation Search
      G4_Operand *verticalOriginOpnd, G4_Operand *horizontalOriginOpnd,
      G4_Operand *xDirectionSizeOpnd, G4_Operand *yDirectionSizeOpnd,
      G4_Operand *xDirectionSearchSizeOpnd,
      G4_Operand *yDirectionSearchSizeOpnd,

      G4_DstRegRegion *dstOpnd, G4_Type dstType, unsigned dstSize,

      // HDC
      unsigned char pixelSize, G4_Operand *dstSurfaceOpnd, G4_Operand *dstXOpnd,
      G4_Operand *dstYOpnd, bool hdcMode);

  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  // Raw send related members are in VisaToG4/TranslateSendRaw.cpp
  int translateVISARawSendInst(G4_Predicate *predOpnd,
                               VISA_Exec_Size executionSize,
                               VISA_EMask_Ctrl emask, uint8_t modifiers,
                               unsigned int exDesc, uint8_t numSrc,
                               uint8_t numDst, G4_Operand *msgDescOpnd,
                               G4_SrcRegRegion *msgOpnd,
                               G4_DstRegRegion *dstOpnd);

  int translateVISARawSendsInst(G4_Predicate *predOpnd,
                                VISA_Exec_Size executionSize,
                                VISA_EMask_Ctrl emask, uint8_t modifiers,
                                G4_Operand *exDesc, uint8_t numSrc0,
                                uint8_t numSrc1, uint8_t numDst,
                                G4_Operand *msgDescOpnd, G4_Operand *msgOpnd0,
                                G4_Operand *msgOpnd1, G4_DstRegRegion *dstOpnd,
                                unsigned ffid, bool hasEOT = false);

  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  // Send sync related members are in VisaToG4/TranslateSendSync.cpp
  G4_INST *translateLscFence(G4_Predicate *pred, SFID sfid,
                             LSC_FENCE_OP fenceOp, LSC_SCOPE scope,
                             int &status);

  G4_INST *translateLscFence(G4_Predicate *pred, SFID sfid,
                             LSC_FENCE_OP fenceOp, LSC_SCOPE scope) {
    int status = VISA_SUCCESS;
    return translateLscFence(pred, sfid, fenceOp, scope, status);
  }

  ////////////////////////////////////////////////////////////////////////
  // default barrier functions
  void generateSingleBarrier(G4_Predicate *prd);
  void generateBarrierSend(G4_Predicate *prd);
  void generateBarrierWait(G4_Predicate *prd);
  int translateVISASplitBarrierInst(G4_Predicate *prd, bool isSignal);

  ////////////////////////////////////////////////////////////////////////
  // named barrier functions
  int translateVISANamedBarrierSignal(G4_Predicate *prd, G4_Operand *barrierId,
                                      G4_Operand *barrierType,
                                      G4_Operand *numProducers,
                                      G4_Operand *numConsumers);
  int translateVISANamedBarrierWait(G4_Predicate *prd, G4_Operand *barrierId);

  ////////////////////////////////////////////////////////////////////////
  // fence etc

  // this is the old fence op
  // post-LSC platforms should use LSC fences
  G4_INST *createFenceInstructionPreLSC(G4_Predicate *prd, uint8_t flushParam,
                                        bool commitEnable, bool globalMemFence,
                                        bool isSendc);

  G4_INST *createSLMFence(G4_Predicate *prd);

  ////////////////////////////////////////////////////////////////////////
  // other mem sync ops
  int translateVISAWaitInst(G4_Operand *mask);

  int translateVISASyncInst(ISA_Opcode opcode, unsigned int mask);


  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////

  // return either 253 or 255 for A64 messages, depending on whether we want I/A
  // coherency or not
  uint8_t getA64BTI() const {
    return m_options->getOption(vISA_noncoherentStateless) ? 0xFD : 0xFF;
  }

  bool useSends() const {
    return getPlatform() >= GENX_SKL &&
           !(VISA_WA_CHECK(m_pWaTable, WaDisableSendsSrc0DstOverlap));
  }

  Metadata *allocateMD() {
    Metadata *newMD = new (metadataMem) Metadata();
    allMDs.push_back(newMD);
    return newMD;
  }

  MDNode *allocateMDString(const std::string &str) {
    auto newNode = new (metadataMem) MDString(str);
    allMDNodes.push_back(newNode);
    return newNode;
  }

  MDLocation *allocateMDLocation(int line, const char *file) {
    auto newNode = new (metadataMem) MDLocation(line, file);
    allMDNodes.push_back(newNode);
    return newNode;
  }

  MDTokenLocation *allocateMDTokenLocation(unsigned short token,
                                           unsigned globalID) {
    auto newNode = new (metadataMem) MDTokenLocation(token, globalID);
    allMDNodes.push_back(newNode);
    return newNode;
  }

  //FrequencyInfo API
  FrequencyInfo& getFreqInfoManager() { return freqInfoManager;}
  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  // Generic IR simplification tasks
  G4_Imm *foldConstVal(G4_Imm* opnd, G4_INST *op);
  G4_Imm *foldConstVal(G4_Imm *const1, G4_Imm *const2, G4_opcode op);
  void doConsFolding(G4_INST *inst);
  G4_INST *doMathConsFolding(INST_LIST_ITER &iter);
  void doSimplification(G4_INST *inst);

  static G4_Type findConstFoldCommonType(G4_Type type1, G4_Type type2);
  ///////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////

  void materializeGlobalImm(G4_BB *entryBB); // why is in FlowGraph.cpp???

  bool canPromoteToMovi(G4_INST *);

#include "HWCaps.inc"

private:
  G4_INST* getSingleDefInst(G4_INST* UI, Gen4_Operand_Number OpndNum) const;
  G4_SrcRegRegion *createBindlessExDesc(uint32_t exdesc);
  uint32_t createSamplerMsgDesc(VISASampler3DSubOpCode samplerOp,
                                bool isNativeSIMDSize, bool isFP16Return,
                                bool isFP16Input) const;
};

constexpr VISALscImmOffOpts getLscImmOffOpt(LSC_ADDR_TYPE addrType) {
  switch (addrType) {
  case LSC_ADDR_TYPE_FLAT:
    return VISA_LSC_IMMOFF_ADDR_TYPE_FLAT;
  case LSC_ADDR_TYPE_BSS:
    return VISA_LSC_IMMOFF_ADDR_TYPE_BSS;
  case LSC_ADDR_TYPE_SS:
    return VISA_LSC_IMMOFF_ADDR_TYPE_SS;
  case LSC_ADDR_TYPE_BTI:
  case LSC_ADDR_TYPE_ARG:
    return VISA_LSC_IMMOFF_ADDR_TYPE_BTI;
  default:
    break;
  }
  return VISA_LSC_IMMOFF_INVALID;
}
} // namespace vISA

// G4IR instructions added by JIT that do not result from lowering
// any CISA bytecode will be assigned CISA offset = 0xffffffff.
// This includes pseudo nodes, G4_labels, mov introduced for copying
// r0 for pre-emption support.
constexpr int UNMAPPABLE_VISA_INDEX = vISA::IR_Builder::OrphanVISAIndex;

#endif
