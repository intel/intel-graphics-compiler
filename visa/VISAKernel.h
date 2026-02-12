/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VISA_KERNEL_H
#define VISA_KERNEL_H
#include "Attributes.hpp"
#include "BuildCISAIR.h"
#include "Common_ISA_util.h"
#include "IsaVerification.h"
#include "JitterDataStruct.h"
#include "KernelInfo.h"
#include "KernelCostInfo.h"
#include "Mem_Manager.h"
#include "VISABuilderAPIDefinition.h"
#include "visa_wa.h"

#include <list>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#define MAX_ERROR_MSG_LEN 511
#define vISA_NUMBER_OF_OPNDS_IN_POOL 47

// forward declaration
namespace vISA {
class G4_Kernel;
class DebugInfoFormat;
class BinaryEncodingBase;
} // namespace vISA

class VISAKernel_format_provider;

struct print_decl_index_t {
  unsigned var_index = 0;
  unsigned addr_index = 0;
  unsigned pred_index = 0;
  unsigned sampler_index = 0;
  unsigned surface_index = 0;
  unsigned input_index = 0;
};

// Class hierarchy is as follows:
// clang-format off
// VISAKernel -> Abstract class that declares virtual functions to build a kernel object.
// VISAFunction : VISAKernel -> Abstract class that declares function specific APIs.
// VISAKernelImpl : VISAFunction -> Implementation for all APIs in VISAKernel and VISAFunction.
// clang-format on
class VISAKernelImpl : public VISAFunction {
  friend class VISAKernel_format_provider;

public:
  VISAKernelImpl(enum VISA_BUILD_TYPE type, CISA_IR_Builder *cisaBuilder,
                 const char *name, unsigned int funcId)
      : m_functionId(funcId), m_CISABuilder(cisaBuilder),
        m_fastPathOpndPool(), m_options(cisaBuilder->getOptions())
  {
    mBuildOption = m_CISABuilder->getBuilderOption();
    m_magic_number = COMMON_ISA_MAGIC_NUM;
    m_major_version = m_CISABuilder->getMajorVersion();
    m_minor_version = m_CISABuilder->getMinorVersion();
    m_var_info_count = 0;
    m_addr_info_count = 0;
    m_pred_info_count = 0;
    m_sampler_count = 0;
    m_surface_count = 0;
    m_input_count = 0;
    m_attribute_count = 0;
    m_label_count = 0;

    m_genx_binary_size = 0;
    m_genx_binary_buffer = NULL;
    m_genx_debug_info_size = 0;
    m_genx_debug_info_buffer = NULL;
    m_input_offset = 0;
    m_num_pred_vars = 0;
    m_type = type;

    memset(&m_cisa_kernel, 0, sizeof(kernel_format_t));
    m_jitInfo = NULL;
    m_kernelInfo = NULL;
    m_kernel = NULL;
    m_builder = NULL;

    m_inputSize = 0;
    m_opndCounter = 0;

    varNameCount = COMMON_ISA_NUM_PREDEFINED_VAR_VER_3;
    addressNameCount = 0;
    predicateNameCount = COMMON_ISA_NUM_PREDEFINED_PRED;
    surfaceNameCount = COMMON_ISA_NUM_PREDEFINED_SURF_VER_3_1;
    samplerNameCount = 0;
    unknownNameCount = 0;
    m_vISAInstCount = -1;

    mIsFCCallableKernel = false;
    mIsFCCallerKernel = false;
    mIsFCComposableKernel = false;

    // Initialize first level scope of the map
    m_GenNamedVarMap.emplace_back();

    createKernelAttributes();
    createReservedKeywordSet();

    InitializeKernel(name);
    SetGTPinInit(m_CISABuilder->getGtpinInit());
  }

  void *alloc(size_t sz) { return m_mem.alloc(sz); }

  virtual ~VISAKernelImpl();
  VISAKernelImpl(const VISAKernelImpl&) = delete;
  VISAKernelImpl& operator=(const VISAKernelImpl&) = delete;

  void *operator new(size_t sz, vISA::Mem_Manager &m) { return m.alloc(sz); };

  vISA::Attributes *getKernelAttributes() { return m_kernelAttrs; }
  // Temporary function to move options to attributes!
  bool finalizeAttributes();

  void setName(const char *n);
  const char *getName() const { return m_name.c_str(); }

  const kernel_format_t *getKernelFormat() const { return &m_cisa_kernel; }
  /***************** START HELPER FUNCTIONS ********************/
  int CreateStateInstUse(VISA_StateOpndHandle *&cisa_opnd, unsigned int index);
  int CreateStateInstUseFastPath(VISA_StateOpndHandle *&cisa_opnd,
                                 CISA_GEN_VAR *decl);
  Common_ISA_Input_Class GetInputClass(Common_ISA_Var_Class var_class);

  void addVarInfoToList(CISA_GEN_VAR *t);

  void addAddrToList(CISA_GEN_VAR *addr);

  void addPredToList(CISA_GEN_VAR *pred);

  void addSampler(CISA_GEN_VAR *state);

  void addSurface(CISA_GEN_VAR *state);

  void addAttribute(const char *input_name, attribute_info_t *attr_temp);

  int addLabel(label_info_t *lbl, char *label_name);

  uint32_t addStringPool(std::string strng);

  void addInstructionToEnd(CisaFramework::CisaInst *inst);
  int addFunctionDirective(char *func_name);

  // The user of CreateVISALabelVar should be responsible for the management
  // of the created labels. The following are the example APIs to manage vISA
  // labels that assumes every block/subroutine/function in kernel has an
  // unique name.
  VISA_LabelOpnd *getLabelOperandFromFunctionName(const std::string &name);
  VISA_LabelOpnd *getLabelOpndFromLabelName(const std::string &label_name);
  bool setLabelOpndNameMap(const std::string &label_name, VISA_LabelOpnd *lbl,
                           VISA_Label_Kind kind);

  void setGenxDebugInfoBuffer(char *buffer, unsigned long size);
  VISA_opnd *CreateOtherOpndHelper(int num_pred_desc_operands, int num_operands,
                                   const VISA_INST_Desc *inst_desc,
                                   unsigned int value,
                                   bool hasSubOpcode = false,
                                   uint8_t subOpcode = 0);
  VISA_opnd *CreateOtherOpnd(unsigned int value, VISA_Type opndType);

  vISA::G4_Operand *
  CommonISABuildPreDefinedSrc(int index, uint16_t vStride, uint16_t width,
                              uint16_t hStride, uint8_t rowOffset,
                              uint8_t colOffset, VISA_Modifier modifier);
  /***************** END HELPER FUNCTIONS **********************/
  std::list<CisaFramework::CisaInst *>::iterator getInstructionListBegin() {
    return m_instruction_list.begin();
  }
  std::list<CisaFramework::CisaInst *>::iterator getInstructionListEnd() {
    return m_instruction_list.end();
  }
  std::list<CisaFramework::CisaInst *>::const_iterator
  getInstructionListBegin() const {
    return m_instruction_list.cbegin();
  }
  std::list<CisaFramework::CisaInst *>::const_iterator
  getInstructionListEnd() const {
    return m_instruction_list.cend();
  }

  unsigned int getNumPredVars() { return m_num_pred_vars; }
  bool getIsKernel() const { return m_type == VISA_BUILD_TYPE::KERNEL; }
  bool getIsFunction() const { return m_type == VISA_BUILD_TYPE::FUNCTION; }
  bool getIsPayload() const { return m_type == VISA_BUILD_TYPE::PAYLOAD; }
  enum VISA_BUILD_TYPE getType() const { return m_type; }
  void setType(enum VISA_BUILD_TYPE _type) { m_type = _type; }

  CISA_GEN_VAR *getDeclFromName(const std::string &name);
  bool declExistsInCurrentScope(const std::string &name) const;
  bool setNameIndexMap(const std::string &name, CISA_GEN_VAR *,
                       bool unique = false);
  void pushIndexMapScopeLevel();
  void popIndexMapScopeLevel();

  vISA::G4_Kernel *getKernel() const { return m_kernel; }
  vISA::IR_Builder *getIRBuilder() const { return m_builder; }
  CISA_IR_Builder *getCISABuilder() const { return m_CISABuilder; }

  int getVISAOffset() const;
  void CopyVars(VISAKernelImpl *from);

  /***************** START EXPOSED APIS *************************/
  VISA_BUILDER_API int CreateVISAGenVar(VISA_GenVar *&decl, const char *varName,
                                        int numberElements, VISA_Type dataType,
                                        VISA_Align varAlign,
                                        VISA_GenVar *parentDecl = NULL,
                                        int aliasOffset = 0) override;

  VISA_BUILDER_API int CreateVISAAddrVar(VISA_AddrVar *&decl,
                                         const char *varName,
                                         unsigned int numberElements) override;

  VISA_BUILDER_API int AddKernelAttribute(const char *name, int size,
                                          const void *value) override;

  VISA_BUILDER_API int
  CreateVISAPredVar(VISA_PredVar *&decl, const char *varName,
                    unsigned short numberElements) override;

  VISA_BUILDER_API int AddAttributeToVar(VISA_PredVar *decl,
                                         const char *varName, unsigned int size,
                                         void *val) override;

  VISA_BUILDER_API int AddAttributeToVar(VISA_SurfaceVar *decl,
                                         const char *varName, unsigned int size,
                                         void *val) override;

  VISA_BUILDER_API int AddAttributeToVar(VISA_GenVar *decl, const char *name,
                                         unsigned int size, void *val) override;

  VISA_BUILDER_API int AddAttributeToVar(VISA_AddrVar *decl, const char *name,
                                         unsigned int size, void *val) override;

  VISA_BUILDER_API int
  CreateVISASamplerVar(VISA_SamplerVar *&decl, const char *name,
                       unsigned int numberElements) override;

  VISA_BUILDER_API int
  CreateVISASurfaceVar(VISA_SurfaceVar *&decl, const char *name,
                       unsigned int numberElements) override;

  VISA_BUILDER_API int CreateVISALabelVar(VISA_LabelOpnd *&opnd,
                                          const char *name,
                                          VISA_Label_Kind kind) override;

  VISA_BUILDER_API int CreateVISAImplicitInputVar(VISA_GenVar *decl,
                                                  unsigned short offset,
                                                  unsigned short size,
                                                  unsigned short kind) override;

  VISA_BUILDER_API int CreateVISAInputVar(VISA_GenVar *decl,
                                          unsigned short offset,
                                          unsigned short size) override;

  VISA_BUILDER_API int CreateVISAInputVar(VISA_SamplerVar *decl,
                                          unsigned short offset,
                                          unsigned short size) override;

  VISA_BUILDER_API int CreateVISAInputVar(VISA_SurfaceVar *decl,
                                          unsigned short offset,
                                          unsigned short size) override;

  VISA_BUILDER_API int CreateVISAAddressSrcOperand(VISA_VectorOpnd *&opnd,
                                                   VISA_AddrVar *decl,
                                                   unsigned int offset,
                                                   unsigned int width) override;

  VISA_BUILDER_API int
  CreateVISAAddressDstOperand(VISA_VectorOpnd *&opnd, VISA_AddrVar *decl,
                              unsigned int offset) override;

  VISA_BUILDER_API int CreateVISAAddressOfOperand(VISA_VectorOpnd *&cisa_opnd,
                                                  VISA_GenVar *decl,
                                                  unsigned int offset) override;

  VISA_BUILDER_API int CreateVISAAddressOfOperand(VISA_VectorOpnd *&cisa_opnd,
                                                  VISA_SurfaceVar *decl,
                                                  unsigned int offset) override;

  VISA_BUILDER_API int CreateVISAIndirectSrcOperand(
      VISA_VectorOpnd *&opnd, VISA_AddrVar *cisa_decl, VISA_Modifier mod,
      unsigned int addrOffset, short immediateOffset,
      unsigned short verticalStride, unsigned short width,
      unsigned short horizontalStride, VISA_Type type) override;

  VISA_BUILDER_API int
  CreateVISAIndirectDstOperand(VISA_VectorOpnd *&opnd, VISA_AddrVar *decl,
                               unsigned int addrOffset, short immediateOffset,
                               unsigned short horizontalStride,
                               VISA_Type type) override;

  VISA_BUILDER_API int CreateVISAIndirectOperandVxH(VISA_VectorOpnd *&cisa_opnd,
                                                    VISA_AddrVar *decl,
                                                    VISA_Modifier mod,
                                                    unsigned int addrOffset,
                                                    short immediateOffset,
                                                    VISA_Type type) override;

  VISA_BUILDER_API int
  CreateVISAPredicateOperand(VISA_PredOpnd *&opnd, VISA_PredVar *decl,
                             VISA_PREDICATE_STATE state,
                             VISA_PREDICATE_CONTROL cntrl) override;

  VISA_BUILDER_API int CreateVISASrcOperand(
      VISA_VectorOpnd *&opnd, VISA_GenVar *cisa_decl, VISA_Modifier mod,
      unsigned short vStride, unsigned short width, unsigned short hStride,
      unsigned char rowOffset, unsigned char colOffset) override;

  VISA_BUILDER_API int CreateVISADstOperand(VISA_VectorOpnd *&opnd,
                                            VISA_GenVar *decl,
                                            unsigned short hStride,
                                            unsigned char rowOffset,
                                            unsigned char colOffset) override;

  VISA_BUILDER_API int CreateVISAImmediate(VISA_VectorOpnd *&opnd,
                                           const void *val,
                                           VISA_Type type) override;

  VISA_BUILDER_API int CreateVISAStateOperand(VISA_VectorOpnd *&opnd,
                                              VISA_SurfaceVar *decl,
                                              unsigned char offset,
                                              bool useAsDst) override;

  VISA_BUILDER_API int CreateVISAStateOperand(VISA_VectorOpnd *&opnd,
                                              VISA_SurfaceVar *decl,
                                              uint8_t size,
                                              unsigned char offset,
                                              bool useAsDst) override;

  VISA_BUILDER_API int CreateVISAStateOperand(VISA_VectorOpnd *&opnd,
                                              VISA_SamplerVar *decl,
                                              unsigned char offset,
                                              bool useAsDst) override;

  VISA_BUILDER_API int CreateVISAStateOperand(VISA_VectorOpnd *&opnd,
                                              VISA_SamplerVar *decl,
                                              uint8_t size,
                                              unsigned char offset,
                                              bool useAsDst) override;

  VISA_BUILDER_API int
  CreateVISAStateOperandHandle(VISA_StateOpndHandle *&opnd,
                               VISA_SurfaceVar *decl) override;

  VISA_BUILDER_API int
  CreateVISAStateOperandHandle(VISA_StateOpndHandle *&opnd,
                               VISA_SamplerVar *decl) override;

  VISA_BUILDER_API int CreateVISARawOperand(VISA_RawOpnd *&opnd,
                                            VISA_GenVar *decl,
                                            unsigned short offset) override;

  VISA_BUILDER_API int CreateVISANullRawOperand(VISA_RawOpnd *&opnd,
                                                bool isDst) override;

  VISA_BUILDER_API int GetPredefinedVar(VISA_GenVar *&predDcl,
                                        PreDefined_Vars varName) override;

  VISA_BUILDER_API int
  GetPredefinedSurface(VISA_SurfaceVar *&surfDcl,
                       PreDefined_Surface surfaceName) override;

  VISA_BUILDER_API int
  GetBindlessSampler(VISA_SamplerVar *&samplerDcl) override;

  VISA_BUILDER_API int SetFunctionInputSize(unsigned int size) override;

  VISA_BUILDER_API int SetFunctionReturnSize(unsigned int size) override;

  /********** APPEND INSTRUCTION APIS START ******************/
  VISA_BUILDER_API int
  AppendVISAArithmeticInst(ISA_Opcode opcode, VISA_PredOpnd *pred, bool satMode,
                           VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
                           VISA_VectorOpnd *tmpDst,
                           VISA_VectorOpnd *src0) override;

  VISA_BUILDER_API int
  AppendVISAArithmeticInst(ISA_Opcode opcode, VISA_PredOpnd *pred, bool satMode,
                           VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
                           VISA_VectorOpnd *tmpDst, VISA_VectorOpnd *src0,
                           VISA_VectorOpnd *src1) override;

  VISA_BUILDER_API int
  AppendVISAArithmeticInst(ISA_Opcode opcode, VISA_PredOpnd *pred, bool satMode,
                           VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
                           VISA_VectorOpnd *tmpDst, VISA_VectorOpnd *src0,
                           VISA_VectorOpnd *src1,
                           VISA_VectorOpnd *src2) override;

  VISA_BUILDER_API int AppendVISATwoDstArithmeticInst(
      ISA_Opcode opcode, VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
      VISA_Exec_Size executionSize, VISA_VectorOpnd *dst1,
      VISA_VectorOpnd *carry_borrow, VISA_VectorOpnd *src0,
      VISA_VectorOpnd *src1) override;

  // Two dst instructon
  //   (vecDst, predDst) = opcode src0, src1
  VISA_BUILDER_API int AppendVISAPredDstArithmeticInst(
      ISA_Opcode opcode, VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
      VISA_Exec_Size executionSize, VISA_VectorOpnd *vecDst,
      VISA_PredVar *predDst, VISA_VectorOpnd *src0,
      VISA_VectorOpnd *src1) override;

  VISA_BUILDER_API int AppendVISALogicOrShiftInst(
      ISA_Opcode opcode, VISA_PredOpnd *pred, bool satMode,
      VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize, VISA_VectorOpnd *dst,
      VISA_VectorOpnd *src0, VISA_VectorOpnd *src1,
      VISA_VectorOpnd *src2 = NULL, VISA_VectorOpnd *src3 = NULL) override;

  VISA_BUILDER_API int
  AppendVISALogicOrShiftInst(ISA_Opcode opcode, VISA_EMask_Ctrl emask,
                             VISA_Exec_Size executionSize, VISA_PredVar *dst,
                             VISA_PredVar *src0, VISA_PredVar *src1) override;

  VISA_BUILDER_API int AppendVISAAddrAddInst(VISA_EMask_Ctrl emask,
                                             VISA_Exec_Size executionSize,
                                             VISA_VectorOpnd *dst,
                                             VISA_VectorOpnd *src0,
                                             VISA_VectorOpnd *src1) override;

  VISA_BUILDER_API int AppendVISABreakpointInst() override;

  VISA_BUILDER_API int
  AppendVISADataMovementInst(ISA_Opcode opcode, VISA_PredOpnd *pred,
                             bool satMod, VISA_EMask_Ctrl emask,
                             VISA_Exec_Size executionSize, VISA_VectorOpnd *dst,
                             VISA_VectorOpnd *src0) override;

  VISA_BUILDER_API int AppendVISADataMovementInst(
      ISA_Opcode opcode, VISA_PredOpnd *pred, bool satMod,
      VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize, VISA_VectorOpnd *dst,
      VISA_VectorOpnd *src0, VISA_VectorOpnd *src1) override;

  VISA_BUILDER_API int AppendVISAPredicateMove(VISA_VectorOpnd *dst,
                                               VISA_PredVar *src0) override;

  VISA_BUILDER_API int AppendVISASetP(VISA_EMask_Ctrl emask,
                                      VISA_Exec_Size executionSize,
                                      VISA_PredVar *dst,
                                      VISA_VectorOpnd *src0) override;

  VISA_BUILDER_API int AppendVISAMinMaxInst(CISA_MIN_MAX_SUB_OPCODE subOpcode,
                                            bool satMod, VISA_EMask_Ctrl emask,
                                            VISA_Exec_Size executionSize,
                                            VISA_VectorOpnd *dst,
                                            VISA_VectorOpnd *src0,
                                            VISA_VectorOpnd *src1) override;

  VISA_BUILDER_API int AppendVISAComparisonInst(
      VISA_Cond_Mod sub_op, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISA_PredVar *dst, VISA_VectorOpnd *src0, VISA_VectorOpnd *src1) override;

  VISA_BUILDER_API int AppendVISAComparisonInst(VISA_Cond_Mod sub_op,
                                                VISA_EMask_Ctrl emask,
                                                VISA_Exec_Size executionSize,
                                                VISA_VectorOpnd *dst,
                                                VISA_VectorOpnd *src0,
                                                VISA_VectorOpnd *src1) override;

  VISA_BUILDER_API int AppendVISACFGotoInst(VISA_PredOpnd *pred,
                                            VISA_EMask_Ctrl emask,
                                            VISA_Exec_Size executionSize,
                                            VISA_LabelOpnd *label) override;

  VISA_BUILDER_API int AppendVISACFLabelInst(VISA_LabelOpnd *label) override;

  VISA_BUILDER_API int AppendVISACFJmpInst(VISA_PredOpnd *pred,
                                           VISA_LabelOpnd *label) override;

  VISA_BUILDER_API int AppendVISACFCallInst(VISA_PredOpnd *pred,
                                            VISA_EMask_Ctrl emask,
                                            VISA_Exec_Size executionSize,
                                            VISA_LabelOpnd *label) override;

  VISA_BUILDER_API int
  AppendVISACFRetInst(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
                      VISA_Exec_Size executionSize) override;

  VISA_BUILDER_API int
  AppendVISACFFunctionCallInst(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
                               VISA_Exec_Size executionSize,
                               const std::string& funcName, unsigned char argSize,
                               unsigned char returnSize) override;

  VISA_BUILDER_API int
  AppendVISACFIndirectFuncCallInst(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
                                   VISA_Exec_Size executionSize, bool isUniform,
                                   VISA_VectorOpnd *funcAddr, uint8_t argSize,
                                   uint8_t returnSize) override;

  VISA_BUILDER_API int AppendVISACFSymbolInst(const std::string& symbolName,
                                              VISA_VectorOpnd *dst) override;

  VISA_BUILDER_API int
  AppendVISACFFunctionRetInst(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
                              VISA_Exec_Size executionSize) override;

  VISA_BUILDER_API int
  AppendVISACFSwitchJMPInst(VISA_VectorOpnd *index, unsigned char labelCount,
                            VISA_LabelOpnd **labels) override;

  VISA_BUILDER_API int AppendVISASurfAccessDwordAtomicInst(
      VISA_PredOpnd *pred, VISAAtomicOps subOpc, bool is16Bit,
      VISA_EMask_Ctrl eMask, VISA_Exec_Size execSize,
      VISA_StateOpndHandle *surface, VISA_RawOpnd *offsets, VISA_RawOpnd *src0,
      VISA_RawOpnd *src1, VISA_RawOpnd *dst) override;

  VISA_BUILDER_API int AppendVISASurfAccessGatherScatterInst(
      ISA_Opcode opcode, VISA_EMask_Ctrl emask,
      GATHER_SCATTER_ELEMENT_SIZE elementSize, VISA_Exec_Size executionSize,
      VISA_StateOpndHandle *surface, VISA_VectorOpnd *globalOffset,
      VISA_RawOpnd *elementOffset, VISA_RawOpnd *srcDst) override;

  VISA_BUILDER_API int AppendVISASurfAccessGather4Scatter4TypedInst(
      ISA_Opcode opcode, VISA_PredOpnd *pred, VISAChannelMask chMask,
      VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISA_StateOpndHandle *surface, VISA_RawOpnd *uOffset,
      VISA_RawOpnd *vOffset, VISA_RawOpnd *rOffset, VISA_RawOpnd *lod,
      VISA_RawOpnd *dst) override;

  VISA_BUILDER_API int AppendVISASurfAccessGather4Scatter4ScaledInst(
      ISA_Opcode opcode, VISA_PredOpnd *pred, VISA_EMask_Ctrl eMask,
      VISA_Exec_Size execSize, VISAChannelMask chMask,
      VISA_StateOpndHandle *surface, VISA_VectorOpnd *globalOffset,
      VISA_RawOpnd *offsets, VISA_RawOpnd *dstSrc) override;

  VISA_BUILDER_API int AppendVISASurfAccessScatterScaledInst(
      ISA_Opcode opcode, VISA_PredOpnd *pred, VISA_EMask_Ctrl eMask,
      VISA_Exec_Size execSize, VISA_SVM_Block_Num numBlocks,
      VISA_StateOpndHandle *surface, VISA_VectorOpnd *globalOffset,
      VISA_RawOpnd *offsets, VISA_RawOpnd *dstSrc) override;

  VISA_BUILDER_API int AppendVISASurfAccessMediaLoadStoreInst(
      ISA_Opcode opcode, MEDIA_LD_mod modifier, VISA_StateOpndHandle *surface,
      unsigned char blockWidth, unsigned char blockHeight,
      VISA_VectorOpnd *xOffset, VISA_VectorOpnd *yOffset, VISA_RawOpnd *srcDst,
      CISA_PLANE_ID plane = CISA_PLANE_Y) override;

  VISA_BUILDER_API int AppendVISASurfAccessOwordLoadStoreInst(
      ISA_Opcode opcode, VISA_EMask_Ctrl emask, VISA_StateOpndHandle *surface,
      VISA_Oword_Num size, VISA_VectorOpnd *offset,
      VISA_RawOpnd *srcDst) override;

  VISA_BUILDER_API int
  AppendVISASvmBlockStoreInst(VISA_Oword_Num size, bool unaligned,
                              VISA_VectorOpnd *address,
                              VISA_RawOpnd *srcDst) override;

  VISA_BUILDER_API int
  AppendVISASvmBlockLoadInst(VISA_Oword_Num size, bool unaligned,
                             VISA_VectorOpnd *address,
                             VISA_RawOpnd *srcDst) override;

  VISA_BUILDER_API int AppendVISASvmScatterInst(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISA_SVM_Block_Type blockType, VISA_SVM_Block_Num numBlocks,
      VISA_RawOpnd *address, VISA_RawOpnd *srcDst) override;

  VISA_BUILDER_API int AppendVISASvmGatherInst(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISA_SVM_Block_Type blockType, VISA_SVM_Block_Num numBlocks,
      VISA_RawOpnd *address, VISA_RawOpnd *srcDst) override;

  VISA_BUILDER_API int AppendVISASvmAtomicInst(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISAAtomicOps op, unsigned short bitwidth, VISA_RawOpnd *addresses,
      VISA_RawOpnd *src0, VISA_RawOpnd *src1, VISA_RawOpnd *dst) override;

  VISA_BUILDER_API int AppendVISASvmGather4ScaledInst(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl eMask, VISA_Exec_Size execSize,
      VISAChannelMask channelMask, VISA_VectorOpnd *address,
      VISA_RawOpnd *offsets, VISA_RawOpnd *dst) override;

  VISA_BUILDER_API int AppendVISASvmScatter4ScaledInst(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl eMask, VISA_Exec_Size execSize,
      VISAChannelMask channelMask, VISA_VectorOpnd *address,
      VISA_RawOpnd *offsets, VISA_RawOpnd *src) override;

  VISA_BUILDER_API int
  AppendVISASILoad(VISA_StateOpndHandle *surface, VISAChannelMask channel,
                   bool isSIMD16, VISA_RawOpnd *uOffset, VISA_RawOpnd *vOffset,
                   VISA_RawOpnd *rOffset, VISA_RawOpnd *dst) override;

  VISA_BUILDER_API int
  AppendVISASISample(VISA_EMask_Ctrl emask, VISA_StateOpndHandle *surface,
                     VISA_StateOpndHandle *sampler, VISAChannelMask channel,
                     bool isSIMD16, VISA_RawOpnd *uOffset,
                     VISA_RawOpnd *vOffset, VISA_RawOpnd *rOffset,
                     VISA_RawOpnd *dst) override;

  VISA_BUILDER_API int
  AppendVISASISampleUnorm(VISA_StateOpndHandle *surface,
                          VISA_StateOpndHandle *sampler,
                          VISAChannelMask channel, VISA_VectorOpnd *uOffset,
                          VISA_VectorOpnd *vOffset, VISA_VectorOpnd *deltaU,
                          VISA_VectorOpnd *deltaV, VISA_RawOpnd *dst,
                          CHANNEL_OUTPUT_FORMAT out) override;

  VISA_BUILDER_API int AppendVISASyncInst(ISA_Opcode opcode,
                                          unsigned char mask = 0) override;
  VISA_BUILDER_API int AppendVISAWaitInst(VISA_VectorOpnd *mask) override;

  VISA_BUILDER_API int AppendVISASplitBarrierInst(bool isSignal) override;

  VISA_BUILDER_API int AppendVISAMiscFileInst(const char *fileName) override;

  VISA_BUILDER_API int AppendVISAMiscLOC(unsigned int lineNumber) override;

  VISA_BUILDER_API int AppendVISADebugLinePlaceholder() override;

  VISA_BUILDER_API int
  AppendVISAMiscRawSend(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
                        VISA_Exec_Size executionSize, unsigned char modifiers,
                        unsigned int exMsgDesc, unsigned char srcSize,
                        unsigned char dstSize, VISA_VectorOpnd *desc,
                        VISA_RawOpnd *src, VISA_RawOpnd *dst) override;
  VISA_BUILDER_API int AppendVISAMiscRawSends(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      unsigned char modifiers, unsigned ffid, VISA_VectorOpnd *exMsgDesc,
      unsigned char src0Size, unsigned char src1Size, unsigned char dstSize,
      VISA_VectorOpnd *desc, VISA_RawOpnd *src0, VISA_RawOpnd *src1,
      VISA_RawOpnd *dst, bool hasEOT) override;

  VISA_BUILDER_API int AppendVISAMiscRawSendg(
    unsigned SFID,
    VISA_PredOpnd *pred,
    VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISA_RawOpnd *dst, int dstLenBytes,
    VISA_RawOpnd *src0, int src0LenBytes,
    VISA_RawOpnd *src1, int src1LenBytes,
    VISA_VectorOpnd *ind0,
    VISA_VectorOpnd *ind1,
    uint64_t desc,
    bool sendgConditional,
    bool issueEOT) override;

  VISA_BUILDER_API int AppendVISAMiscVME_FBR(VISA_StateOpndHandle *surface,
                                             VISA_RawOpnd *UNIInput,
                                             VISA_RawOpnd *FBRInput,
                                             VISA_VectorOpnd *FBRMbMode,
                                             VISA_VectorOpnd *FBRSubMbShape,
                                             VISA_VectorOpnd *FBRSubPredMode,
                                             VISA_RawOpnd *output) override;

  VISA_BUILDER_API int
  AppendVISAMiscVME_IME(VISA_StateOpndHandle *surface, unsigned char streamMode,
                        unsigned char searchControlMode, VISA_RawOpnd *UNIInput,
                        VISA_RawOpnd *IMEInput, VISA_RawOpnd *ref0,
                        VISA_RawOpnd *ref1, VISA_RawOpnd *costCenter,
                        VISA_RawOpnd *output) override;

  VISA_BUILDER_API int AppendVISAMiscVME_SIC(VISA_StateOpndHandle *surface,
                                             VISA_RawOpnd *UNIInput,
                                             VISA_RawOpnd *SICInput,
                                             VISA_RawOpnd *output) override;

  VISA_BUILDER_API int AppendVISAMiscVME_IDM(VISA_StateOpndHandle *surface,
                                             VISA_RawOpnd *UNIInput,
                                             VISA_RawOpnd *IDMInput,
                                             VISA_RawOpnd *output) override;

  VISA_BUILDER_API int
  AppendVISAMEAVS(VISA_StateOpndHandle *surface, VISA_StateOpndHandle *sampler,
                  VISAChannelMask channel, VISA_VectorOpnd *uOffset,
                  VISA_VectorOpnd *vOffset, VISA_VectorOpnd *deltaU,
                  VISA_VectorOpnd *deltaV, VISA_VectorOpnd *u2d,
                  VISA_VectorOpnd *v2d, VISA_VectorOpnd *groupID,
                  VISA_VectorOpnd *verticalBlockNumber,
                  OutputFormatControl cntrl, AVSExecMode execMode,
                  VISA_VectorOpnd *iefBypass, VISA_RawOpnd *dst) override;

  VISA_BUILDER_API int AppendVISAVABooleanCentroid(
      VISA_StateOpndHandle *surface, VISA_VectorOpnd *uOffset,
      VISA_VectorOpnd *vOffset, VISA_VectorOpnd *vSize, VISA_VectorOpnd *hSize,
      VISA_RawOpnd *dst) override;
  VISA_BUILDER_API int AppendVISAVACentroid(VISA_StateOpndHandle *surface,
                                            VISA_VectorOpnd *uOffset,
                                            VISA_VectorOpnd *vOffset,
                                            VISA_VectorOpnd *vSize,
                                            VISA_RawOpnd *dst) override;

  VISA_BUILDER_API int
  AppendVISAVAConvolve(VISA_StateOpndHandle *sampler,
                       VISA_StateOpndHandle *surface, VISA_VectorOpnd *uOffset,
                       VISA_VectorOpnd *vOffset, CONVExecMode execMode,
                       bool isBigKernel, VISA_RawOpnd *dst) override;

  VISA_BUILDER_API int
  AppendVISAVAErodeDilate(EDMode subOp, VISA_StateOpndHandle *sampler,
                          VISA_StateOpndHandle *surface,
                          VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
                          EDExecMode execMode, VISA_RawOpnd *dst) override;

  VISA_BUILDER_API int AppendVISAVAMinMax(VISA_StateOpndHandle *surface,
                                          VISA_VectorOpnd *uOffset,
                                          VISA_VectorOpnd *vOffset,
                                          VISA_VectorOpnd *mmMode,
                                          VISA_RawOpnd *dst) override;

  VISA_BUILDER_API int AppendVISAVAMinMaxFilter(
      VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
      VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
      OutputFormatControl cntrl, MMFExecMode execMode, VISA_VectorOpnd *mmfMode,
      VISA_RawOpnd *dst) override;

  VISA_BUILDER_API int AppendVISAVACorrelationSearch(
      VISA_StateOpndHandle *surface, VISA_VectorOpnd *uOffset,
      VISA_VectorOpnd *vOffset, VISA_VectorOpnd *vOrigin,
      VISA_VectorOpnd *hOrigin, VISA_VectorOpnd *xDirectionSize,
      VISA_VectorOpnd *yDirectionSize, VISA_VectorOpnd *xDirectionSearchSize,
      VISA_VectorOpnd *yDirectionSearchSize, VISA_RawOpnd *dst) override;

  VISA_BUILDER_API int
  AppendVISAVAFloodFill(bool is8Connect, VISA_RawOpnd *pixelMaskHDirection,
                        VISA_VectorOpnd *pixelMaskVDirectionLeft,
                        VISA_VectorOpnd *pixelMaskVDirectionRight,
                        VISA_VectorOpnd *loopCount, VISA_RawOpnd *dst) override;

  VISA_BUILDER_API int AppendVISAVALBPCorrelation(VISA_StateOpndHandle *surface,
                                                  VISA_VectorOpnd *uOffset,
                                                  VISA_VectorOpnd *vOffset,
                                                  VISA_VectorOpnd *disparity,
                                                  VISA_RawOpnd *dst) override;

  VISA_BUILDER_API int AppendVISAVALBPCreation(VISA_StateOpndHandle *surface,
                                               VISA_VectorOpnd *uOffset,
                                               VISA_VectorOpnd *vOffset,
                                               LBPCreationMode mode,
                                               VISA_RawOpnd *dst) override;

  VISA_BUILDER_API int AppendVISAVAConvolve1D(
      VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
      VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset, CONVExecMode mode,
      Convolve1DDirection direction, VISA_RawOpnd *dst) override;

  VISA_BUILDER_API int AppendVISAVAConvolve1Pixel(VISA_StateOpndHandle *sampler,
                                                  VISA_StateOpndHandle *surface,
                                                  VISA_VectorOpnd *uOffset,
                                                  VISA_VectorOpnd *vOffset,
                                                  CONV1PixelExecMode mode,
                                                  VISA_RawOpnd *offsets,
                                                  VISA_RawOpnd *dst) override;

  VISA_BUILDER_API int AppendVISAVAHDCConvolve(
      VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
      VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
      HDCReturnFormat returnFormat, CONVHDCRegionSize regionSize,
      VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset,
      VISA_VectorOpnd *yOffset) override;

  VISA_BUILDER_API int AppendVISAVAHDCErodeDilate(
      EDMode subOp, VISA_StateOpndHandle *sampler,
      VISA_StateOpndHandle *surface, VISA_VectorOpnd *uOffset,
      VISA_VectorOpnd *vOffset, VISA_StateOpndHandle *dstSurface,
      VISA_VectorOpnd *xOffset, VISA_VectorOpnd *yOffset) override;

  VISA_BUILDER_API int AppendVISAVAHDCMinMaxFilter(
      VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
      VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
      HDCReturnFormat returnFormat, MMFEnableMode mmfMode,
      VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset,
      VISA_VectorOpnd *yOffset) override;

  VISA_BUILDER_API int AppendVISAVAHDCLBPCorrelation(
      VISA_StateOpndHandle *surface, VISA_VectorOpnd *uOffset,
      VISA_VectorOpnd *vOffset, VISA_VectorOpnd *disparity,
      VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset,
      VISA_VectorOpnd *yOffset) override;

  VISA_BUILDER_API int AppendVISAVAHDCLBPCreation(
      VISA_StateOpndHandle *surface, VISA_VectorOpnd *uOffset,
      VISA_VectorOpnd *vOffset, LBPCreationMode mode,
      VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset,
      VISA_VectorOpnd *yOffset) override;

  VISA_BUILDER_API int AppendVISAVAHDCConvolve1D(
      VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
      VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
      HDCReturnFormat returnFormat, Convolve1DDirection direction,
      VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset,
      VISA_VectorOpnd *yOffset) override;

  VISA_BUILDER_API int AppendVISAVAHDCConvolve1Pixel(
      VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
      VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
      HDCReturnFormat returnFormat, VISA_RawOpnd *offsets,
      VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset,
      VISA_VectorOpnd *yOffset) override;

  VISA_BUILDER_API int AppendVISALifetime(VISAVarLifetime startOrEnd,
                                          VISA_VectorOpnd *varId) override;

  VISA_BUILDER_API int AppendVISADpasInst(
      ISA_Opcode opcode, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISA_RawOpnd *tmpDst, VISA_RawOpnd *src0, VISA_RawOpnd *src1,
      VISA_VectorOpnd *src2, GenPrecision src2Precision,
      GenPrecision src1Precision, uint8_t Depth, uint8_t Count) override;
  VISA_BUILDER_API int AppendVISABdpasInst(
      ISA_Opcode opcode, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISA_RawOpnd *dst, VISA_RawOpnd *src0, VISA_RawOpnd *src1,
      VISA_RawOpnd *src2, VISA_VectorOpnd *src3, VISA_VectorOpnd *src4,
      GenPrecision src2Precision, GenPrecision src1Precision, uint8_t Depth,
      uint8_t Count) override;

  VISA_BUILDER_API int
  AppendVISABfnInst(uint8_t booleanFuncCtrl, VISA_PredOpnd *pred, bool satMode,
                    VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
                    VISA_VectorOpnd *tmpDst, VISA_VectorOpnd *src0,
                    VISA_VectorOpnd *src1, VISA_VectorOpnd *src2) override;

  VISA_BUILDER_API int AppendVISAQwordGatherInst(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISA_SVM_Block_Num numBlocks, VISA_StateOpndHandle *surface,
      VISA_RawOpnd *address, VISA_RawOpnd *src) override;

  VISA_BUILDER_API int AppendVISAQwordScatterInst(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISA_SVM_Block_Num numBlocks, VISA_StateOpndHandle *surface,
      VISA_RawOpnd *address, VISA_RawOpnd *dst) override;
  VISA_BUILDER_API int AppendVISALscUntypedLoad(
      LSC_OP op, LSC_SFID sfid, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, bool ov, LSC_ADDR addr,
      LSC_DATA_SHAPE data, VISA_VectorOpnd *surface, unsigned surfaceIndex,
      VISA_RawOpnd *dstData, VISA_RawOpnd *src0Addr) override;
  VISA_BUILDER_API int AppendVISALscUntypedStore(
      LSC_OP op, LSC_SFID sfid, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, LSC_ADDR addr,
      LSC_DATA_SHAPE data, VISA_VectorOpnd *surface, unsigned surfaceIndex,
      VISA_RawOpnd *src0Addr,VISA_RawOpnd *src1Data) override;
  VISA_BUILDER_API int AppendVISALscUntypedAtomic(
      LSC_OP op, LSC_SFID sfid, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, LSC_ADDR addr,
      LSC_DATA_SHAPE data, VISA_VectorOpnd *surface, unsigned surfaceIndex,
      VISA_RawOpnd *dstReadBack, VISA_RawOpnd *src0Addr, VISA_RawOpnd *src1AtomOpnd1,
      VISA_RawOpnd *src2AtomOpnd2) override;
  VISA_BUILDER_API int AppendVISALscUntypedInst(
      LSC_OP op, LSC_SFID sfid, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, bool ov, LSC_ADDR addr,
      LSC_DATA_SHAPE data, VISA_VectorOpnd *surface, unsigned surfaceIndex,
      VISA_RawOpnd *dst, VISA_RawOpnd *src0, VISA_RawOpnd *src1,
      VISA_RawOpnd *src2) override;

  VISA_BUILDER_API int AppendVISALscUntypedStridedInst(
      LSC_OP subOpcode, LSC_SFID sfid, VISA_PredOpnd *pred,
      VISA_Exec_Size execSize, VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts,
      LSC_ADDR addrInfo, LSC_DATA_SHAPE dataShape,
      VISA_VectorOpnd *surface, unsigned surfaceIndex,
      VISA_RawOpnd *dstData, VISA_RawOpnd *src0AddrBase,
      VISA_VectorOpnd *src0AddrPitch, VISA_RawOpnd *src1Data) override;
  VISA_BUILDER_API int AppendVISALscUntypedBlock2DInst(
      LSC_OP op, LSC_SFID lscSfid, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts,
      LSC_DATA_SHAPE_BLOCK2D dataShape, VISA_RawOpnd *dstData,
      VISA_VectorOpnd *src0Addrs[LSC_BLOCK2D_ADDR_PARAMS],
      int xImmOffset, int yImmOffset, VISA_RawOpnd *src1Data) override;
  VISA_BUILDER_API int AppendVISALscUntypedBlock2DInst(
      LSC_OP op, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts,
      LSC_DATA_SHAPE_BLOCK2D dataShape, VISA_RawOpnd *dstData,
      VISA_VectorOpnd *src0AddrPayload,
      int xImmOffset, int yImmOffset, VISA_RawOpnd *src1Data) override;
  VISA_BUILDER_API int AppendVISALscTypedLoad(
      LSC_OP op, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, LSC_ADDR_TYPE addrModel,
      LSC_ADDR_SIZE addrSize, LSC_DATA_SHAPE data,
      VISA_VectorOpnd *surface, unsigned surfaceIndex,
      VISA_RawOpnd *dstData,
      VISA_RawOpnd *Us, int uOffset,
      VISA_RawOpnd *Vs, int vOffset,
      VISA_RawOpnd *Rs, int rOffset, VISA_RawOpnd *LODs) override;
  VISA_BUILDER_API int AppendVISALscTypedStore(
      LSC_OP op, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, LSC_ADDR_TYPE addrType,
      LSC_ADDR_SIZE addrSize, LSC_DATA_SHAPE data,
      VISA_VectorOpnd *surface, unsigned surfaceIndex,
      VISA_RawOpnd *Us, int uOffset, VISA_RawOpnd *Vs, int vOffset,
      VISA_RawOpnd *Rs, int rOffset, VISA_RawOpnd *LODs,
      VISA_RawOpnd *src1Data) override;
  VISA_BUILDER_API int AppendVISALscTypedAtomic(
      LSC_OP op, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, LSC_ADDR_TYPE addrType,
      LSC_ADDR_SIZE addrSize, LSC_DATA_SHAPE data,
      VISA_VectorOpnd *surface, unsigned surfaceIndex,
      VISA_RawOpnd *dst,
      VISA_RawOpnd *Us, int uOffset,
      VISA_RawOpnd *Vs, int vOffset,
      VISA_RawOpnd *Rs, int rOffset,
      VISA_RawOpnd *LODs,
      VISA_RawOpnd *src1AtomicOpnd1, VISA_RawOpnd *src2AtomicOpnd2) override;

  VISA_BUILDER_API int AppendVISALscTypedInst(
      LSC_OP op, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, LSC_ADDR_TYPE addrType,
      LSC_ADDR_SIZE addrSize, LSC_DATA_SHAPE data,
      VISA_VectorOpnd *surface, unsigned surfaceIndex,
      VISA_RawOpnd *dst,
      VISA_RawOpnd *Us, int uOffset,
      VISA_RawOpnd *Vs, int vOffset,
      VISA_RawOpnd *Rs, int rOffset,
      VISA_RawOpnd *features,
      VISA_RawOpnd *src1, VISA_RawOpnd *src2) override;

  VISA_BUILDER_API int AppendVISALscFence(LSC_SFID lscSfid,
                                          LSC_FENCE_OP fenceOp,
                                          LSC_SCOPE scope) override;


  VISA_BUILDER_API int AppendVISALscTypedBlock2DInst(
      LSC_OP op, LSC_CACHE_OPTS cacheOpts, LSC_ADDR_TYPE addrModel,
      LSC_DATA_SHAPE_TYPED_BLOCK2D dataShape2D,
      VISA_VectorOpnd *surface, unsigned surfaceIndex,
      VISA_RawOpnd *dstData, VISA_VectorOpnd *blockStartX,
      VISA_VectorOpnd *blockStartY, int xImmOffset, int yImmOffset,
      VISA_RawOpnd *src1Data) override;

  VISA_BUILDER_API int AppendVISALscUntypedAppendCounterAtomicInst(
      LSC_OP op, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts,
      LSC_ADDR_TYPE addrType, LSC_DATA_SHAPE data,
      VISA_VectorOpnd *surface, unsigned surfaceIndex,
      VISA_RawOpnd *dst, VISA_RawOpnd *srcData) override;

  VISA_BUILDER_API int
  AppendVISANamedBarrierWait(VISA_VectorOpnd *barrierId) override;

  // Named barrier with the same number of producers and consumers
  VISA_BUILDER_API int
  AppendVISANamedBarrierSignal(VISA_VectorOpnd *barrierId,
                               VISA_VectorOpnd *barrierCount) override;
  // General producer-consumer named barrier
  VISA_BUILDER_API int AppendVISANamedBarrierSignal(
      VISA_VectorOpnd *barrierId, VISA_VectorOpnd *barrierType,
      VISA_VectorOpnd *numProducers, VISA_VectorOpnd *numConsumers) override;
  VISA_BUILDER_API int
  AppendVISAShflIdx4Inst(ISA_Opcode opcode, VISA_PredOpnd *pred,
                         VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
                         VISA_RawOpnd *dst, VISA_VectorOpnd *src0,
                         VISA_VectorOpnd *src1) override;

  VISA_BUILDER_API int
  AppendVISALfsrInst(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
                     VISA_Exec_Size executionSize, LFSR_FC funcCtrl,
                     VISA_VectorOpnd *dst, VISA_VectorOpnd *src0,
                     VISA_VectorOpnd *src1) override;

  VISA_BUILDER_API int
  AppendVISADnsclInst(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
                      VISA_Exec_Size executionSize, DNSCL_CONVERT_TYPE type,
                      DNSCL_MODE mode, DNSCL_RND_MODE rndMode,
                      VISA_RawOpnd *dst, VISA_RawOpnd *src0, VISA_RawOpnd *src1,
                      VISA_RawOpnd *src2) override;
  VISA_BUILDER_API int
  AppendVISALscExtendedCacheCtrlInst(LSC_OP subOpcode, LSC_SFID lscSfid,
      VISA_PredOpnd *pred, VISA_Exec_Size execSize, VISA_EMask_Ctrl emask,
      LSC_CACHE_CTRL_OPERATION ccop, LSC_CACHE_CTRL_SIZE ccsize,
      LSC_CACHE_OPTS caching, LSC_ADDR addr, VISA_RawOpnd *src0Addr) override;

  /********** APPEND INSTRUCTION APIS END   ******************/

  /********** APPEND 3D Instructions START ******************/
  VISA_BUILDER_API int
  AppendVISA3dSampler(VISASampler3DSubOpCode subOpcode, bool pixelNullMask,
                      bool cpsEnable, bool uniformSampler, VISA_PredOpnd *pred,
                      VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
                      VISAChannelMask srcChannel, VISA_VectorOpnd *aoffimmi,
                      VISA_StateOpndHandle *sampler,
                      VISA_StateOpndHandle *surface,
                      VISA_RawOpnd *pairedSurface,
                      VISA_RawOpnd *dst, int numMsgSpecificOpnds,
                      VISA_RawOpnd **opndArray) override;

  VISA_BUILDER_API int
  AppendVISA3dSampler(VISASampler3DSubOpCode subOpcode, bool pixelNullMask,
                      bool cpsEnable, bool uniformSampler, VISA_PredOpnd *pred,
                      VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
                      VISAChannelMask srcChannel, VISA_VectorOpnd *aoffimmi,
                      VISA_StateOpndHandle *sampler, unsigned int samplerIdx,
                      VISA_StateOpndHandle *surface, unsigned int surfaceIdx,
                      VISA_RawOpnd *pairedSurface,
                      VISA_RawOpnd *dst, int numMsgSpecificOpnds,
                      VISA_RawOpnd **opndArray) override;

  VISA_BUILDER_API int
  AppendVISA3dLoad(VISASampler3DSubOpCode subOpcode, bool pixelNullMask,
                   VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
                   VISA_Exec_Size executionSize, VISAChannelMask srcChannel,
                   VISA_VectorOpnd *aoffimmi, VISA_StateOpndHandle *surface,
                   VISA_RawOpnd *pairedSurface,
                   VISA_RawOpnd *dst, int numMsgSpecificOpnds,
                   VISA_RawOpnd **opndArray) override;

  VISA_BUILDER_API int
  AppendVISA3dLoad(VISASampler3DSubOpCode subOpcode, bool pixelNullMask,
                   VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
                   VISA_Exec_Size executionSize, VISAChannelMask srcChannel,
                   VISA_VectorOpnd *aoffimmi,
                   VISA_StateOpndHandle *surface, unsigned int surfaceIndex,
                   VISA_RawOpnd *pairedSurface,
                   VISA_RawOpnd *dst, int numMsgSpecificOpnds,
                   VISA_RawOpnd **opndArray) override;

  VISA_BUILDER_API int AppendVISA3dGather4(
      VISASampler3DSubOpCode subOpcode, bool pixelNullMask, VISA_PredOpnd *pred,
      VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISASourceSingleChannel srcChannel, VISA_VectorOpnd *aoffimmi,
      VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
      VISA_RawOpnd *pairedSurface,
      VISA_RawOpnd *dst, int numMsgSpecificOpnds,
      VISA_RawOpnd **opndArray) override;

  VISA_BUILDER_API int AppendVISA3dGather4(
      VISASampler3DSubOpCode subOpcode, bool pixelNullMask, VISA_PredOpnd *pred,
      VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISASourceSingleChannel srcChannel, VISA_VectorOpnd *aoffimmi,
      VISA_StateOpndHandle *sampler, unsigned samplerIndex,
      VISA_StateOpndHandle *surface, unsigned surfaceIndex,
      VISA_RawOpnd *pairedSurface,
      VISA_RawOpnd *dst, int numMsgSpecificOpnds,
      VISA_RawOpnd **opndArray) override;

  VISA_BUILDER_API int
  AppendVISA3dInfo(VISASampler3DSubOpCode subOpcode, VISA_EMask_Ctrl emask,
                   VISA_Exec_Size executionSize, VISAChannelMask srcChannel,
                   VISA_StateOpndHandle *surface, unsigned int surfaceIndex,
                   VISA_RawOpnd *lod, VISA_RawOpnd *dst) override;

  VISA_BUILDER_API int AppendVISA3dRTWrite(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISA_VectorOpnd *renderTargetIndex, vISA_RT_CONTROLS cntrls,
      VISA_StateOpndHandle *surface, VISA_RawOpnd *r1HeaderOpnd,
      VISA_VectorOpnd *sampleIndex, uint8_t numMsgSpecificOpnds,
      VISA_RawOpnd **opndArray) override;

  VISA_BUILDER_API int AppendVISA3dRTWriteCPS(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISA_VectorOpnd *renderTargetIndex, vISA_RT_CONTROLS cntrls,
      VISA_StateOpndHandle *surface, VISA_RawOpnd *r1HeaderOpnd,
      VISA_VectorOpnd *sampleIndex, VISA_VectorOpnd *cPSCounter,
      uint8_t numMsgSpecificOpnds, VISA_RawOpnd **opndArray, int rtIdentifier = 0) override;

  VISA_BUILDER_API int AppendVISA3dURBWrite(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      unsigned char numberOutputParams, VISA_RawOpnd *channelMask,
      unsigned short globalOffset, VISA_RawOpnd *URBHandle,
      VISA_RawOpnd *perSLotOffset, VISA_RawOpnd *vertexData) override;

  VISA_BUILDER_API int AppendVISA3dTypedAtomic(
      VISAAtomicOps subOp, bool is16Bit, VISA_PredOpnd *pred,
      VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISA_StateOpndHandle *surface, VISA_RawOpnd *u, VISA_RawOpnd *v,
      VISA_RawOpnd *r, VISA_RawOpnd *lod, VISA_RawOpnd *src0,
      VISA_RawOpnd *src1, VISA_RawOpnd *dst) override;

  /********** APPEND 3D Instructions END ******************/

  /********** MISC APIs START *************************/
  VISA_BUILDER_API int GetGenxBinary(void *&buffer, int &size) const override;
  VISA_BUILDER_API int GetJitInfo(vISA::FINALIZER_INFO *&jitInfo) const override;
  VISA_BUILDER_API int GetKernelInfo(KERNEL_INFO *&kernelInfo) const override;
  VISA_BUILDER_API int GetErrorMessage(const char *&errorMsg) const override;
  VISA_BUILDER_API virtual int
  GetGenxDebugInfo(void *&buffer, unsigned int &size) const override;
  /// GetRelocations -- add vISA created relocations into given relocation list
  VISA_BUILDER_API int GetRelocations(RelocListType &relocs) override;
  VISA_BUILDER_API int GetGTPinBuffer(void *&buffer, unsigned int &size,
                                      unsigned int scratchOffset) override;
  VISA_BUILDER_API int SetGTPinInit(void *buffer) override;
  VISA_BUILDER_API int GetFreeGRFInfo(void *&buffer,
                                      unsigned int &size) override;

  VISA_BUILDER_API int GetFunctionId(unsigned int &id) const override;

  /// Gets declaration id GenVar
  VISA_BUILDER_API int getDeclarationID(VISA_GenVar *decl) const override;

  /// Gets declaration id VISA_AddrVar
  VISA_BUILDER_API int getDeclarationID(VISA_AddrVar *decl) const override;

  /// Gets declaration id VISA_PredVar
  VISA_BUILDER_API int getDeclarationID(VISA_PredVar *decl) const override;

  /// Gets declaration id VISA_SamplerVar
  VISA_BUILDER_API int getDeclarationID(VISA_SamplerVar *decl) const override;

  /// Gets declaration id VISA_SurfaceVar
  VISA_BUILDER_API int getDeclarationID(VISA_SurfaceVar *decl) const override;

  /// Gets gen binary offset
  VISA_BUILDER_API int64_t getGenOffset() const override;

  /// Gets gen binary size within instruction heap
  VISA_BUILDER_API int64_t getGenSize() const override;

  /// Gets num of total regs
  VISA_BUILDER_API virtual unsigned getNumRegTotal() const override;

  /// Get global function name
  VISA_BUILDER_API const char *getFunctionName() const override;

  /// Get vISA asm of the kernel function
  VISA_BUILDER_API std::string getVISAAsm() const override;

  /// set or get current block frequency information
  VISA_BUILDER_API int encodeBlockFrequency(uint64_t digits, int16_t scale) override;

  // Gets the VISA string format for the variable
  VISA_BUILDER_API std::string getVarName(VISA_GenVar *decl) const override;
  VISA_BUILDER_API std::string getVarName(VISA_PredVar *decl) const override;
  VISA_BUILDER_API std::string getVarName(VISA_AddrVar *decl) const override;
  VISA_BUILDER_API std::string getVarName(VISA_SurfaceVar *decl) const override;
  VISA_BUILDER_API std::string getVarName(VISA_SamplerVar *decl) const override;

  // Gets the VISA string format for the operand
  VISA_BUILDER_API std::string
  getVectorOperandName(VISA_VectorOpnd *opnd, bool showRegion) const override;
  VISA_BUILDER_API std::string
  getPredicateOperandName(VISA_PredOpnd *opnd) const override;

  // Get vISA kernel cost metrics
  VISA_BUILDER_API int
  getKernelCostInfo(const vISA::KernelCostInfo *&KCInfo) const override;

  /********** MISC APIs END *************************/
  int CreateVISAPredicateSrcOperand(VISA_VectorOpnd *&opnd, VISA_PredVar *decl,
                                    unsigned int size);

  int CreateVISAPredicateDstOperand(VISA_VectorOpnd *&opnd, VISA_PredVar *decl,
                                    uint32_t size);

  int CreateVISAAddressOperand(VISA_VectorOpnd *&opnd, VISA_AddrVar *decl,
                               unsigned int offset, unsigned int width,
                               bool isDst);

  int CreateVISAPredicateOperandvISA(VISA_PredOpnd *&opnd, VISA_PredVar *decl,
                                     VISA_PREDICATE_STATE state,
                                     VISA_PREDICATE_CONTROL cntrl);

  int CreateGenNullRawOperand(VISA_RawOpnd *&opnd, bool isDst);

  int CreateGenRawSrcOperand(VISA_RawOpnd *&cisa_opnd);
  int CreateGenRawDstOperand(VISA_RawOpnd *&cisa_opnd);

  int CreateVISAIndirectGeneralOperand(
      VISA_VectorOpnd *&opnd, VISA_AddrVar *cisa_decl, VISA_Modifier mod,
      unsigned int addrOffset, unsigned short immediateOffset,
      unsigned short verticalStride, unsigned short width,
      unsigned short horizontalStride, VISA_Type type, bool isDst);

  int AppendVISA3dSamplerMsgGeneric(
      ISA_Opcode opcode, VISASampler3DSubOpCode subOpcode, bool pixelNullMask,
      bool cpsEnable, bool uniformSampler, VISA_PredOpnd *pred,
      VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      ChannelMask srcChannel, VISA_VectorOpnd *aoffimmi,
      VISA_StateOpndHandle *sampler, unsigned int samplerIdx,
      VISA_StateOpndHandle *surface, unsigned int surfaceIdx,
      VISA_RawOpnd *pairedSurface,
      VISA_RawOpnd *dst, unsigned int numMsgSpecificOpnds,
      VISA_RawOpnd **opndArray);

  attribute_info_t *allocAttribute(CISA_GEN_VAR *Dcl) {
    return allocAttributeImpl(Dcl, 0);
  }
  attribute_info_t *resizeAttribute(CISA_GEN_VAR *Dcl, uint32_t AllocMaxNum) {
    return allocAttributeImpl(Dcl, AllocMaxNum);
  }

  int AddAttributeToVarGeneric(CISA_GEN_VAR *decl, const char *varName,
                               unsigned int size, const void *val);

  int CreateStateVar(CISA_GEN_VAR *&decl, Common_ISA_Var_Class type,
                     const char *name, unsigned int numberElements);

  int CreateVISAInputVar(CISA_GEN_VAR *decl, uint16_t offset, uint16_t size,
                         uint8_t implicitKind);

  int CreateVISAAddressOfOperandGeneric(VISA_VectorOpnd *&cisa_opnd,
                                        CISA_GEN_VAR *decl,
                                        unsigned int offset);

  int CreateVISAStateOperand(VISA_VectorOpnd *&opnd, CISA_GEN_VAR *decl,
                             Common_ISA_State_Opnd_Class opnd_class,
                             uint8_t size, unsigned char offset, bool useAsDst);

  void setGenxBinaryBuffer(void *buffer, int size) {
    m_genx_binary_buffer = static_cast<char *>(buffer);
    m_genx_binary_size = size;
  }

  void setJitInfo(vISA::FINALIZER_INFO *jitInfo) { m_jitInfo = jitInfo; }

  std::string getOutputAsmPath() const { return m_asmName; }
  void setOutputAsmPath(const std::string& val) { m_asmName = val; }

  int compileFastPath();

  unsigned int m_magic_number;
  unsigned char m_major_version;
  unsigned char m_minor_version;

  void compilePostOptimize();
  void *encodeAndEmit(unsigned int &binarySize);

  void setInputSize(uint8_t size);
  void setReturnSize(unsigned int size);

  bool getIsGenBothPath() const {
    return (mBuildOption == VISA_BUILDER_GEN ||
            mBuildOption == VISA_BUILDER_BOTH);
  }

  unsigned getvIsaInstCount() const override { return m_vISAInstCount; };

  bool isFCCallableKernel() const { return mIsFCCallableKernel; }
  void setFCCallableKernel(bool value) { mIsFCCallableKernel = value; }

  bool isFCCallerKernel() const { return mIsFCCallerKernel; }
  void setFCCallerKernel(bool value) { mIsFCCallerKernel = value; }

  bool isFCComposableKernel() const { return mIsFCComposableKernel; }
  void setFCComposableKernel(bool value) { mIsFCComposableKernel = value; }

  void setLocalSheduleable(bool value);

  // Accumulate the input perf stats into this function's stats
  void addFuncPerfStats(const vISA::PERF_STATS_VERBOSE & input);

  void emitPerfStats(std::ostream &os);

  unsigned int getGenVarCount() const {
    return (uint32_t)m_var_info_list.size();
  }

  CISA_GEN_VAR *getGenVar(unsigned int index) const {
    return m_var_info_list[index];
  }

  unsigned int getAddrVarCount() const {
    return (uint32_t)m_addr_info_list.size();
  }

  CISA_GEN_VAR *getAddrVar(unsigned int index) {
    return m_addr_info_list[index];
  }

  unsigned int getPredVarCount() const {
    return (uint32_t)m_pred_info_list.size();
  }

  CISA_GEN_VAR *getPredVar(unsigned int index) {
    auto it = m_pred_info_list.begin();
    std::advance(it, index);

    return (*it);
  }

  unsigned int getSamplerVarCount() const {
    return (uint32_t)m_sampler_info_list.size();
  }

  CISA_GEN_VAR *getSamplerVar(unsigned int index) {
    auto it = m_sampler_info_list.begin();
    std::advance(it, index);

    return (*it);
  }

  unsigned int getSurfaceVarCount() const {
    return (uint32_t)m_surface_info_list.size();
  }

  CISA_GEN_VAR *getSurfaceVar(unsigned int index) {
    auto it = m_surface_info_list.begin();
    std::advance(it, index);

    return (*it);
  }

  std::string getVarName(CISA_GEN_VAR *decl) const {
    vISA_ASSERT_INPUT(m_GenVarToNameMap.count(decl), "Can't find the decl's name");
    return m_GenVarToNameMap.find(decl)->second;
  }

  const Options *getOptions() const { return m_options; }

  bool IsAsmWriterMode() const {
    return m_CISABuilder->getBuilderMode() == vISA_ASM_WRITER;
  }

  typedef CISA_IR_Builder::KernelListTy KernelListTy;
  void computeAndEmitDebugInfo(KernelListTy &functions);

private:
  int InitializeKernel(const char *kernel_name);
  int CISABuildPreDefinedDecls();
  void createReservedKeywordSet();
  bool isReservedName(const std::string &nm) const;
  void ensureVariableNameUnique(const char *&varName);
  bool generateVariableName(Common_ISA_Var_Class Ty, const char *&varName);

  int InitializeFastPath();
  int calculateTotalInputSize();
  int compileTillOptimize();
  void recordFinalizerInfo();
  // dump PERF_STATS into the .stats.json file
  // filename is the full path of output file name without the extension
  void dumpPerfStatsInJson(const std::string &filename);

  // Re-adjust indirect call target after swsb
  void adjustIndirectCallOffset();

  CisaFramework::CisaInst *AppendVISASvmGeneralScatterInst(
      VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size execSize,
      unsigned char blockSize, unsigned char numBlocks, VISA_RawOpnd *address,
      VISA_RawOpnd *srcDst, bool isRead);

  CisaFramework::CisaInst *PackCisaInsnForSVMGather4Scatter4Scaled(
      unsigned subOpc, VISA_PredOpnd *pred, VISA_EMask_Ctrl eMask,
      VISA_Exec_Size executionSize, ChannelMask chMask,
      VISA_VectorOpnd *address, VISA_RawOpnd *offsets, VISA_RawOpnd *srcOrDst);

  VISA_opnd *getOpndFromPool();

  void AppendVISAInstCommon();
  int AppendVISADpasInstCommon(
      ISA_Opcode opcode, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
      VISA_RawOpnd *tmpDst, VISA_RawOpnd *src0, VISA_RawOpnd *src1,
      VISA_VectorOpnd *src2, VISA_VectorOpnd *src3, GenPrecision src2Precision,
      GenPrecision src1Precision, uint8_t Depth, uint8_t Count);

  void createBindlessSampler();

  // if vISA_DumpPerfStats is enabled, finalize the perf stats values before
  // dumping
  void finalizePerfStats(uint64_t binaryHash);

private:
  // This member holds symbolic index of function when invoked via
  // API path. Builder client can use this id when invoking this
  // stack call function. For a kernel instance, this is not useful.
  const unsigned int m_functionId;

  kernel_format_t m_cisa_kernel;

  unsigned int m_num_pred_vars;

  unsigned long m_genx_binary_size;
  char *m_genx_binary_buffer;
  unsigned long m_genx_debug_info_size;
  char *m_genx_debug_info_buffer;
  vISA::FINALIZER_INFO *m_jitInfo;
  KERNEL_INFO *m_kernelInfo;

  unsigned long m_input_offset;

  std::vector<std::string> m_string_pool;
  enum VISA_BUILD_TYPE m_type;

  vISA::Mem_Manager m_mem = 4096;
  std::string m_name;
  std::string m_asmName;
  std::string m_sanitizedName;

  std::list<CisaFramework::CisaInst *> m_instruction_list;

  unsigned int m_var_info_count;
  std::vector<CISA_GEN_VAR *> m_var_info_list;

  unsigned int m_addr_info_count;
  std::vector<CISA_GEN_VAR *> m_addr_info_list;

  unsigned int m_pred_info_count;
  std::vector<CISA_GEN_VAR *> m_pred_info_list;

  unsigned int m_sampler_count;
  std::vector<CISA_GEN_VAR *> m_sampler_info_list;

  unsigned int m_surface_count;
  std::vector<CISA_GEN_VAR *> m_surface_info_list;

  VISA_SamplerVar *m_bindlessSampler;

  std::map<unsigned int, unsigned int> m_declID_to_PredefinedSurfaceID_map;

  unsigned int m_input_count;
  std::vector<input_info_t *> m_input_info_list;
  // std::map<unsigned int, unsigned int> m_declID_to_inputID_map;

  unsigned int m_attribute_count;
  std::list<attribute_info_t *> m_attribute_info_list;

  unsigned int m_label_count;
  std::vector<label_info_t *> m_label_info_list;

  // maps a variable name to the var pointer
  // unique vars are unique to the entire program
  // general vars must be unique within the same scope, but can be redefined
  // across scopes
  typedef std::map<std::string, CISA_GEN_VAR *> GenDeclNameToVarMap;
  std::vector<GenDeclNameToVarMap> m_GenNamedVarMap;
  GenDeclNameToVarMap m_UniqueNamedVarMap;

  // reverse map from a GenVar to its declared name, used in inline assembly
  // Note that name is only unique within the same scope
  std::map<CISA_GEN_VAR *, std::string> m_GenVarToNameMap;

  std::unordered_map<std::string, VISA_LabelOpnd *> m_label_name_to_index_map;
  std::unordered_map<std::string, VISA_LabelOpnd *> m_funcName_to_labelID_map;

  VISA_BUILDER_OPTION mBuildOption;
  vISA::G4_Kernel *m_kernel;
  CISA_IR_Builder *const m_CISABuilder;
  vISA::IR_Builder *m_builder;
  vISA::Mem_Manager *m_kernelMem;
  // customized allocator for allocating
  // It is very important that the same allocator is used by all instruction
  // lists that might be joined/spliced.
  INST_LIST_NODE_ALLOCATOR m_instListNodeAllocator;
  unsigned int m_inputSize;
  VISA_opnd m_fastPathOpndPool[vISA_NUMBER_OF_OPNDS_IN_POOL];
  unsigned int m_opndCounter;

  unsigned int varNameCount;
  unsigned int addressNameCount;
  unsigned int predicateNameCount;
  unsigned int surfaceNameCount;
  unsigned int samplerNameCount;
  unsigned int unknownNameCount;

  // TODO: this should be merged and re-worked to fit into the symbol table
  // scheme
  // Save variable name and its next free sufix
  std::unordered_map <std::string, size_t> varNames;
  std::unordered_set<std::string> reservedNames;

  int m_vISAInstCount;
  print_decl_index_t m_printDeclIndex;

  bool mIsFCCallableKernel;
  bool mIsFCCallerKernel;
  bool mIsFCComposableKernel;

  void computeFCInfo(vISA::BinaryEncodingBase *binEncodingInstance);
  void computeFCInfo();
  // memory managed by the entity that creates vISA Kernel object
  Options *const m_options;

  void createKernelAttributes() { m_kernelAttrs = new vISA::Attributes(); }
  void destroyKernelAttributes() { delete m_kernelAttrs; }
  attribute_info_t *allocAttributeImpl(CISA_GEN_VAR *Dcl, uint32_t AllocNum);

  // Shared with G4_Kernel
  vISA::Attributes *m_kernelAttrs;

  // Instruction verifier that checks CISA instruction creation
  VISAKernel_format_provider *fmt;
  vISAVerifier *verifier;
};

class VISAKernel_format_provider : public print_format_provider_t {
protected:
  const VISAKernelImpl *const m_kernel;

public:
  VISAKernel_format_provider(const VISAKernelImpl *kernel) : m_kernel(kernel) {}

  uint16_t getMajorVersion() const { return m_kernel->m_major_version; }
  uint16_t getMinorVersion() const { return m_kernel->m_minor_version; }

  uint32_t getNameIndex() const { return m_kernel->m_cisa_kernel.name_index; }
  const char *getString(uint32_t str_id) const {
    vASSERT(str_id < m_kernel->m_string_pool.size());
    return m_kernel->m_string_pool[str_id].c_str();
  }
  uint32_t getStringCount() const { return m_kernel->m_string_pool.size(); }
  const label_info_t *getLabel(uint32_t label_id) const {
    vASSERT(label_id < m_kernel->m_label_info_list.size());
    return m_kernel->m_label_info_list[label_id];
  }
  unsigned short getLabelCount() const { return m_kernel->m_label_count; }
  const var_info_t *getPredefVar(unsigned var_id) const {
    vASSERT(var_id < m_kernel->m_num_pred_vars);
    return &m_kernel->m_var_info_list[var_id]->genVar;
  }
  const var_info_t *getVar(unsigned var_id) const {
    vASSERT(var_id + m_kernel->m_num_pred_vars <
           m_kernel->m_var_info_list.size());
    return &m_kernel->m_var_info_list[var_id + m_kernel->m_num_pred_vars]
                ->genVar;
  }
  uint32_t getVarCount() const {
    return m_kernel->m_var_info_count - m_kernel->m_num_pred_vars;
  }
  const attribute_info_t *getAttr(unsigned id) const {
    auto it = m_kernel->m_attribute_info_list.begin();
    std::advance(it, id);
    return *it;
  }
  unsigned getAttrCount() const { return m_kernel->m_attribute_count; }
  const addr_info_t *getAddr(unsigned id) const {
    vASSERT(id < m_kernel->m_addr_info_list.size());
    return &m_kernel->m_addr_info_list[id]->addrVar;
  }
  unsigned short getAddrCount() const { return m_kernel->m_addr_info_count; }
  const pred_info_t *getPred(unsigned id) const {
    vASSERT(id < m_kernel->m_pred_info_list.size());
    return &m_kernel->m_pred_info_list[id]->predVar;
  }
  unsigned short getPredCount() const { return m_kernel->m_pred_info_count; }
  const state_info_t *getPredefSurface(unsigned id) const {
    vASSERT(id < Get_CISA_PreDefined_Surf_Count());
    return &m_kernel->m_surface_info_list[id]->stateVar;
  }
  const state_info_t *getSurface(unsigned id) const {
    vASSERT(id + Get_CISA_PreDefined_Surf_Count() <
           m_kernel->m_surface_info_list.size());
    return &m_kernel->m_surface_info_list[id + Get_CISA_PreDefined_Surf_Count()]
                ->stateVar;
  }
  unsigned char getSurfaceCount() const {
    return m_kernel->m_surface_count - Get_CISA_PreDefined_Surf_Count();
  }
  const state_info_t *getSampler(unsigned id) const {
    vASSERT(id < m_kernel->m_sampler_info_list.size());
    return &m_kernel->m_sampler_info_list[id]->stateVar;
  }
  unsigned char getSamplerCount() const { return m_kernel->m_sampler_count; }
  const input_info_t *getInput(unsigned id) const {
    vASSERT(id < m_kernel->m_input_info_list.size());
    return m_kernel->m_input_info_list[id];
  }
  uint32_t getInputCount() const { return m_kernel->m_input_count; }

  std::string printKernelHeader(bool printVersion);
  std::string printDeclSection(bool printAsComment);

  std::string printInstruction(const CISA_INST *instruction,
                               const Options *opt) const;
};

#endif // VISA_KERNEL_H
