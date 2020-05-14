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

#ifndef VISA_KERNEL_H
#define VISA_KERNEL_H
#include "VISABuilderAPIDefinition.h"
#include "Common_ISA_util.h"
#include "BuildCISAIR.h"
#include "visa_wa.h"
#include "Mem_Manager.h"
#include "JitterDataStruct.h"
//#include "Gen4_IR.hpp"  // for PhyRegPool
#include "Attributes.hpp"
#include "CompilerStats.h"

#include <list>
#include <map>
#include <string>
#include <vector>
#include <unordered_set>

#define MAX_ERROR_MSG_LEN            511
#define vISA_NUMBER_OF_OPNDS_IN_POOL 47

//forward declaration
namespace vISA
{
class G4_Kernel;
class DebugInfoFormat;
class BinaryEncodingBase;
}

// Class hierarchy is as follows:
// VISAKernel -> Abstract class that declares virtual functions to build a kernel object
// VISAFunction : VISAKernel -> Abstract class that declares function specific APIs
// VISAKernelImpl : VISAFunction -> Implementation for all APIs in VISAKernel and VISAFunction
class VISAKernelImpl : public VISAFunction
{
    friend class VISAKernel_format_provider;

public:
    VISAKernelImpl(CISA_IR_Builder* cisaBuilder, VISA_BUILDER_OPTION buildOption, Options *option)
        : m_mem(4096), m_CISABuilder(cisaBuilder), m_options(option)
    {
        //CisaBinary* module = NULL;
        mBuildOption = buildOption;
        m_magic_number = COMMON_ISA_MAGIC_NUM;
        m_major_version = 0;
        m_minor_version = 0;
        m_var_info_count = 0;
        m_addr_info_count = 0;
        m_pred_info_count = 0;
        m_sampler_count = 0;
        m_surface_count = 0;
        m_input_count = 0;
        m_attribute_count = 0;
        m_label_count = 0;

        m_string_pool_size = 0;
        m_var_info_size = 0;
        m_adress_info_size = 0;
        m_predicate_info_size = 0;
        m_label_info_size = 0;
        m_input_info_size = 0;
        m_attribute_info_size = 0;
        m_instruction_size = 0;
        m_genx_binary_size = 0;
        m_kernel_data_size = 0;
        m_genx_binary_buffer = NULL;
        m_genx_debug_info_size = 0;
        m_genx_debug_info_buffer = NULL;
        m_bytes_written_cisa_buffer = 0;
        m_input_offset = 0;
        m_num_pred_vars = 0;
        m_surface_info_size = 0;
        m_sampler_info_size = 0;
        m_isKernel = false;

        memset(&m_cisa_kernel, 0, sizeof(kernel_format_t));
        m_forward_label_count = 0;
        m_jitInfo = NULL;
        errorMessage[0] = '\0';
        m_kernel = NULL;
        m_builder = NULL;
        m_globalMem = NULL;

        m_kernelID = 0;
        m_inputSize = 0;
        m_opndCounter = 0;
        m_var_info_list.reserve(200);
        m_input_info_list.reserve(20);
        m_label_info_list.reserve(50);
        m_addr_info_list.reserve(20);
        // give it some default name in case AsmName is not set
        m_asmName = "test";

        varNameCount = COMMON_ISA_NUM_PREDEFINED_VAR_VER_3;
        addressNameCount = 0;
        predicateNameCount = COMMON_ISA_NUM_PREDEFINED_PRED;
        surfaceNameCount = COMMON_ISA_NUM_PREDEFINED_SURF_VER_3_1;
        samplerNameCount = 0;
        unknownNameCount = 0;
        m_functionId = 0;
        m_vISAInstCount = -1;

        mIsFCCallableKernel = false;
        mIsFCCallerKernel = false;
        mIsFCComposableKernel = false;

        // Initialize first level scope of the map
        // m_GenNamedVarMap.push_back(GenDeclNameToVarMap());
        m_GenNamedVarMap.emplace_back();

        createKernelAttributes();
    }

    void* alloc(size_t sz) { return m_mem.alloc(sz); }

    virtual ~VISAKernelImpl();

    void *operator new(size_t sz, vISA::Mem_Manager& m){ return m.alloc(sz); };
    int InitializeKernel(const char *kernel_name);
    int CISABuildPreDefinedDecls();
    void setVersion(unsigned char major_ver, unsigned char minor_ver){
        m_major_version = major_ver;
        m_minor_version = minor_ver;
    }

    vISA::Attributes* getKernelAttributes() { return m_kernelAttrs; }
    // Temporary function to move options to attributes!
    void finalizeAttributes();
    void finalizeKernel();
    unsigned long writeInToCisaBinaryBuffer(const void * value, int size);
    unsigned long getBytesWritten() { return m_bytes_written_cisa_buffer; }

    void setName(const char* n);
    const char* getName() const { return m_name.c_str(); }
    string_pool_entry** new_string_pool();
    unsigned short get_hash_key(const char* str);
    bool string_pool_lookup_and_insert_branch_targets(char *str,
        Common_ISA_Var_Class type,
        VISA_Type data_type);
    void CISAPostFileParse();
    const kernel_format_t* getKernelFormat() const { return &m_cisa_kernel; }
    /***************** START HELPER FUNCTIONS ********************/
    int CreateStateInstUse(VISA_StateOpndHandle *&cisa_opnd, unsigned int index);
    int CreateStateInstUseFastPath(VISA_StateOpndHandle *&cisa_opnd, CISA_GEN_VAR *decl);
    Common_ISA_Input_Class GetInputClass(Common_ISA_Var_Class var_class);

    void addVarInfoToList(CISA_GEN_VAR * t);

    void addAddrToList(CISA_GEN_VAR * addr);

    void addPredToList(CISA_GEN_VAR * pred);

    void addSampler(CISA_GEN_VAR * state);

    void addSurface(CISA_GEN_VAR * state);

    void addAttribute(const char *input_name, attribute_info_t *attr_temp);

    int addLabel(label_info_t * lbl, char * label_name);

    uint32_t addStringPool(std::string strng);

    void addInstructionToEnd(CisaFramework::CisaInst * inst);
    int addFunctionDirective(char * func_name);

    VISA_LabelOpnd * getLabelOperandFromFunctionName(std::string name);
    unsigned int getLabelIdFromFunctionName(std::string name);

    void setGenxDebugInfoBuffer(char * buffer, unsigned long size);
    VISA_opnd* CreateOtherOpndHelper(int num_pred_desc_operands, int num_operands, VISA_INST_Desc *inst_desc, unsigned int value, bool hasSubOpcode = false, uint8_t subOpcode = 0);
    VISA_opnd* CreateOtherOpnd(unsigned int value, VISA_Type opndType);

    vISA::G4_Operand* CommonISABuildPreDefinedSrc(int index, uint16_t vStride, uint16_t width,
        uint16_t hStride, uint8_t rowOffset, uint8_t colOffset, VISA_Modifier modifier);
    /***************** END HELPER FUNCTIONS **********************/
    std::list<CisaFramework::CisaInst *>::iterator getInstructionListBegin() { return m_instruction_list.begin(); }
    std::list<CisaFramework::CisaInst *>::iterator getInstructionListEnd() { return m_instruction_list.end(); }

    unsigned long getGenxBinarySize() { return m_genx_binary_size; }

    char * getGenxBinaryBuffer() { return m_genx_binary_buffer; }

    unsigned long getCisaBinarySize() { return m_cisa_binary_size; }

    char * getCisaBinaryBuffer() { return m_cisa_binary_buffer; }

    unsigned long getInputOffset() { return m_input_offset; }
    void setNumPredVars(unsigned int val) { m_num_pred_vars = val; }
    unsigned int getNumPredVars() { return m_num_pred_vars; }

    unsigned long getKernelDataSize() { return m_kernel_data_size; }
    void addPendingLabels(VISA_opnd * opnd) { m_pending_labels.push_back(opnd); }
    void addPendingLabelNames(std::string name) { m_pending_label_names.push_back(name); }
    void setIsKernel(bool isKernel) { m_isKernel = isKernel; };
    bool getIsKernel() const { return m_isKernel; }
    unsigned long getCodeOffset(){ return m_cisa_kernel.entry; }

    CISA_GEN_VAR * getDeclFromName(const std::string &name);
    bool setNameIndexMap(const std::string &name, CISA_GEN_VAR *, bool unique = false);
    void pushIndexMapScopeLevel();
    void popIndexMapScopeLevel();

    unsigned int getIndexFromLabelName(const std::string &label_name);
    VISA_LabelOpnd * getLabelOpndFromLabelName(const std::string &label_name);
    bool setLabelNameIndexMap(const std::string &label_name, VISA_LabelOpnd * lbl);
    int patchLastInst(VISA_LabelOpnd *label);
    vISA::G4_Kernel* getKernel() { return m_kernel; }
    vISA::IR_Builder* getIRBuilder() { return m_builder; }
    CISA_IR_Builder* getCISABuilder() { return m_CISABuilder; }

    int getVISAOffset() const;

    /***************** START EXPOSED APIS *************************/
    VISA_BUILDER_API int CreateVISAGenVar(VISA_GenVar *& decl, const char *varName, int numberElements, VISA_Type dataType,
        VISA_Align varAlign, VISA_GenVar *parentDecl = NULL, int aliasOffset = 0);

    VISA_BUILDER_API int CreateVISAAddrVar(VISA_AddrVar *& decl, const char *varName, unsigned int numberElements);

    VISA_BUILDER_API int AddKernelAttribute(const char* name, int size, const void *value);

    VISA_BUILDER_API int CreateVISAPredVar(VISA_PredVar *& decl, const char* varName, unsigned short numberElements);

    VISA_BUILDER_API int AddAttributeToVar(VISA_PredVar *decl, const char* varName, unsigned int size, void *val);

    VISA_BUILDER_API int AddAttributeToVar(VISA_SurfaceVar *decl, const char* varName, unsigned int size, void *val);

    VISA_BUILDER_API int AddAttributeToVar(VISA_GenVar *decl, const char* name, unsigned int size, void *val);

    VISA_BUILDER_API int AddAttributeToVar(VISA_AddrVar *decl, const char* name, unsigned int size, void *val);

    VISA_BUILDER_API int CreateVISASamplerVar(VISA_SamplerVar *&decl, const char* name, unsigned int numberElements);

    VISA_BUILDER_API int CreateVISASurfaceVar(VISA_SurfaceVar *&decl, const char* name, unsigned int numberElements);

    VISA_BUILDER_API int CreateVISALabelVar(VISA_LabelOpnd *& opnd, const char* name, VISA_Label_Kind kind);

    VISA_BUILDER_API int CreateVISAImplicitInputVar(VISA_GenVar *decl, unsigned short offset, unsigned short size, unsigned short kind);

    VISA_BUILDER_API int CreateVISAInputVar(VISA_GenVar *decl, unsigned short offset, unsigned short size);

    VISA_BUILDER_API int CreateVISAInputVar(VISA_SamplerVar *decl, unsigned short offset, unsigned short size);

    VISA_BUILDER_API int CreateVISAInputVar(VISA_SurfaceVar *decl, unsigned short offset, unsigned short size);

    VISA_BUILDER_API int CreateVISAAddressSrcOperand(VISA_VectorOpnd *&opnd, VISA_AddrVar *decl, unsigned int offset, unsigned int width);

    VISA_BUILDER_API int CreateVISAAddressDstOperand(VISA_VectorOpnd *&opnd, VISA_AddrVar *decl, unsigned int offset);

    VISA_BUILDER_API int CreateVISAAddressOfOperand(VISA_VectorOpnd *&cisa_opnd, VISA_GenVar *decl, unsigned int offset);

    VISA_BUILDER_API int CreateVISAAddressOfOperand(VISA_VectorOpnd *&cisa_opnd, VISA_SurfaceVar *decl, unsigned int offset);

    VISA_BUILDER_API int CreateVISAIndirectSrcOperand(VISA_VectorOpnd *& opnd, VISA_AddrVar *cisa_decl, VISA_Modifier mod, unsigned int addrOffset, short immediateOffset,
        unsigned short verticalStride, unsigned short width, unsigned short horizontalStride, VISA_Type type);

    VISA_BUILDER_API int CreateVISAIndirectDstOperand(VISA_VectorOpnd *& opnd, VISA_AddrVar *decl, unsigned int addrOffset, short immediateOffset,
        unsigned short horizontalStride, VISA_Type type);

    VISA_BUILDER_API int CreateVISAIndirectOperandVxH(VISA_VectorOpnd *& cisa_opnd, VISA_AddrVar *decl, unsigned int addrOffset, short immediateOffset, VISA_Type type);

    VISA_BUILDER_API int CreateVISAPredicateOperand(VISA_PredOpnd *& opnd, VISA_PredVar *decl, VISA_PREDICATE_STATE state, VISA_PREDICATE_CONTROL cntrl);

    VISA_BUILDER_API int CreateVISASrcOperand(VISA_VectorOpnd *& opnd, VISA_GenVar *cisa_decl, VISA_Modifier mod, unsigned short vStride, unsigned short width, unsigned short hStride,
        unsigned char rowOffset, unsigned char colOffset);

    VISA_BUILDER_API int CreateVISADstOperand(VISA_VectorOpnd *&opnd, VISA_GenVar *decl, unsigned short hStride, unsigned char rowOffset, unsigned char colOffset);

    VISA_BUILDER_API int CreateVISAImmediate(VISA_VectorOpnd *&opnd, const void *val, VISA_Type type);

    VISA_BUILDER_API int CreateVISAStateOperand(VISA_VectorOpnd *&opnd, VISA_SurfaceVar *decl, unsigned char offset, bool useAsDst);

    VISA_BUILDER_API int CreateVISAStateOperand(VISA_VectorOpnd *&opnd, VISA_SurfaceVar *decl, uint8_t size, unsigned char offset, bool useAsDst);

    VISA_BUILDER_API int CreateVISAStateOperand(VISA_VectorOpnd *&opnd, VISA_SamplerVar *decl, unsigned char offset, bool useAsDst);

    VISA_BUILDER_API int CreateVISAStateOperand(VISA_VectorOpnd *&opnd, VISA_SamplerVar *decl, uint8_t size, unsigned char offset, bool useAsDst);

    VISA_BUILDER_API int CreateVISAStateOperandHandle(VISA_StateOpndHandle *&opnd, VISA_SurfaceVar *decl);

    VISA_BUILDER_API int CreateVISAStateOperandHandle(VISA_StateOpndHandle *&opnd, VISA_SamplerVar *decl);

    VISA_BUILDER_API int CreateVISARawOperand(VISA_RawOpnd *& opnd, VISA_GenVar *decl, unsigned short offset);

    VISA_BUILDER_API int CreateVISANullRawOperand(VISA_RawOpnd *& opnd, bool isDst);

    VISA_BUILDER_API int GetPredefinedVar(VISA_GenVar *&predDcl, PreDefined_Vars varName);

    VISA_BUILDER_API int GetPredefinedSurface(VISA_SurfaceVar *&surfDcl, PreDefined_Surface surfaceName);

    VISA_BUILDER_API int GetBindlessSampler(VISA_SamplerVar *&samplerDcl);

    VISA_BUILDER_API int SetFunctionInputSize(unsigned int size);

    VISA_BUILDER_API int SetFunctionReturnSize(unsigned int size);

    /********** APPEND INSTRUCTION APIS START ******************/
    VISA_BUILDER_API int AppendVISAArithmeticInst(ISA_Opcode opcode, VISA_PredOpnd *pred, bool satMode, VISA_EMask_Ctrl emask,
        VISA_Exec_Size executionSize, VISA_VectorOpnd *tmpDst, VISA_VectorOpnd *src0);

    VISA_BUILDER_API int AppendVISAArithmeticInst(ISA_Opcode opcode, VISA_PredOpnd *pred, bool satMode, VISA_EMask_Ctrl emask,
        VISA_Exec_Size executionSize, VISA_VectorOpnd *tmpDst, VISA_VectorOpnd *src0, VISA_VectorOpnd *src1);

    VISA_BUILDER_API int AppendVISAArithmeticInst(ISA_Opcode opcode, VISA_PredOpnd *pred, bool satMode, VISA_EMask_Ctrl emask,
        VISA_Exec_Size executionSize, VISA_VectorOpnd *tmpDst, VISA_VectorOpnd *src0, VISA_VectorOpnd *src1, VISA_VectorOpnd *src2);

    VISA_BUILDER_API int AppendVISAArithmeticInst(ISA_Opcode opcode, VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
        VISA_Exec_Size executionSize, VISA_VectorOpnd *dst1, VISA_VectorOpnd *carry_borrow, VISA_VectorOpnd *src0, VISA_VectorOpnd *src1);

    VISA_BUILDER_API int AppendVISALogicOrShiftInst(ISA_Opcode opcode, VISA_PredOpnd *pred, bool satMode, VISA_EMask_Ctrl emask,
        VISA_Exec_Size executionSize, VISA_VectorOpnd *dst, VISA_VectorOpnd *src0, VISA_VectorOpnd *src1, VISA_VectorOpnd *src2 = NULL,
        VISA_VectorOpnd *src3 = NULL);

    VISA_BUILDER_API int AppendVISALogicOrShiftInst(ISA_Opcode opcode, VISA_EMask_Ctrl emask,
        VISA_Exec_Size executionSize, VISA_PredVar *dst, VISA_PredVar *src0, VISA_PredVar *src1);

    VISA_BUILDER_API int AppendVISAAddrAddInst(VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize, VISA_VectorOpnd *dst, VISA_VectorOpnd *src0, VISA_VectorOpnd *src1);

    VISA_BUILDER_API int AppendVISADataMovementInst(ISA_Opcode opcode, VISA_PredOpnd *pred, bool satMod, VISA_EMask_Ctrl emask,
        VISA_Exec_Size executionSize, VISA_VectorOpnd *dst, VISA_VectorOpnd *src0);

    VISA_BUILDER_API int AppendVISADataMovementInst(ISA_Opcode opcode, VISA_PredOpnd *pred, bool satMod, VISA_EMask_Ctrl emask,
        VISA_Exec_Size executionSize, VISA_VectorOpnd *dst, VISA_VectorOpnd *src0, VISA_VectorOpnd *src1);

    VISA_BUILDER_API int AppendVISAPredicateMove(VISA_VectorOpnd *dst, VISA_PredVar *src0);

    VISA_BUILDER_API int AppendVISASetP(VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize, VISA_PredVar *dst, VISA_VectorOpnd *src0);

    VISA_BUILDER_API int AppendVISAMinMaxInst(CISA_MIN_MAX_SUB_OPCODE subOpcode, bool satMod, VISA_EMask_Ctrl emask,
        VISA_Exec_Size executionSize, VISA_VectorOpnd *dst, VISA_VectorOpnd *src0, VISA_VectorOpnd *src1);

    VISA_BUILDER_API int AppendVISAComparisonInst(VISA_Cond_Mod sub_op, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize, VISA_PredVar *dst, VISA_VectorOpnd *src0, VISA_VectorOpnd *src1);

    VISA_BUILDER_API int AppendVISAComparisonInst(VISA_Cond_Mod sub_op, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize, VISA_VectorOpnd *dst, VISA_VectorOpnd *src0, VISA_VectorOpnd *src1);

    VISA_BUILDER_API int AppendVISACFGotoInst(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize, VISA_LabelOpnd *label);

    VISA_BUILDER_API int AppendVISACFLabelInst(VISA_LabelOpnd *label);

    VISA_BUILDER_API int AppendVISACFJmpInst(VISA_PredOpnd *pred, VISA_LabelOpnd *label);

    VISA_BUILDER_API int AppendVISACFCallInst(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize, VISA_LabelOpnd *label);

    VISA_BUILDER_API int AppendVISACFRetInst(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize);

    VISA_BUILDER_API int AppendVISACFFunctionCallInst(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
        VISA_Exec_Size executionSize, std::string funcName, unsigned char argSize, unsigned char returnSize);

    VISA_BUILDER_API int AppendVISACFIndirectFuncCallInst(VISA_PredOpnd *pred,
        VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
        VISA_VectorOpnd* funcAddr, uint8_t argSize, uint8_t returnSize);

    VISA_BUILDER_API int AppendVISACFSymbolInst(std::string symbolName, VISA_VectorOpnd* dst);

    VISA_BUILDER_API int AppendVISACFFunctionRetInst(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize);

    VISA_BUILDER_API int AppendVISACFSwitchJMPInst(VISA_VectorOpnd *index, unsigned char labelCount, VISA_LabelOpnd **labels);

    VISA_BUILDER_API int AppendVISASurfAccessDwordAtomicInst(
        VISA_PredOpnd *pred, VISAAtomicOps subOpc, bool is16Bit,
        VISA_EMask_Ctrl eMask, VISA_Exec_Size execSize,
        VISA_StateOpndHandle *surface, VISA_RawOpnd *offsets,
        VISA_RawOpnd *src0, VISA_RawOpnd *src1, VISA_RawOpnd *dst);

    VISA_BUILDER_API int AppendVISASurfAccessGatherScatterInst(ISA_Opcode opcode, VISA_EMask_Ctrl emask, GATHER_SCATTER_ELEMENT_SIZE elementSize,
        VISA_Exec_Size executionSize, VISA_StateOpndHandle *surface, VISA_VectorOpnd *globalOffset, VISA_RawOpnd *elementOffset, VISA_RawOpnd *srcDst);

    VISA_BUILDER_API int AppendVISASurfAccessGather4Scatter4TypedInst(ISA_Opcode opcode,
        VISA_PredOpnd *pred,
        VISAChannelMask chMask,
        VISA_EMask_Ctrl emask,
        VISA_Exec_Size executionSize,
        VISA_StateOpndHandle *surface,
        VISA_RawOpnd *uOffset,
        VISA_RawOpnd *vOffset,
        VISA_RawOpnd *rOffset,
        VISA_RawOpnd *lod,
        VISA_RawOpnd *dst);

    VISA_BUILDER_API int AppendVISASurfAccessGather4Scatter4ScaledInst(ISA_Opcode             opcode,
        VISA_PredOpnd          *pred,
        VISA_EMask_Ctrl eMask,
        VISA_Exec_Size   execSize,
        VISAChannelMask        chMask,
        VISA_StateOpndHandle   *surface,
        VISA_VectorOpnd        *globalOffset,
        VISA_RawOpnd           *offsets,
        VISA_RawOpnd           *dstSrc);

    VISA_BUILDER_API int AppendVISASurfAccessScatterScaledInst(ISA_Opcode                opcode,
        VISA_PredOpnd             *pred,
        VISA_EMask_Ctrl    eMask,
        VISA_Exec_Size      execSize,
        VISA_SVM_Block_Num  numBlocks,
        VISA_StateOpndHandle      *surface,
        VISA_VectorOpnd           *globalOffset,
        VISA_RawOpnd              *offsets,
        VISA_RawOpnd              *dstSrc);

    VISA_BUILDER_API int AppendVISASurfAccessMediaLoadStoreInst(ISA_Opcode opcode, MEDIA_LD_mod modifier, VISA_StateOpndHandle *surface, unsigned char blockWidth,
        unsigned char blockHeight, VISA_VectorOpnd *xOffset, VISA_VectorOpnd *yOffset, VISA_RawOpnd *srcDst,
        CISA_PLANE_ID plane = CISA_PLANE_Y);

    VISA_BUILDER_API int AppendVISASurfAccessOwordLoadStoreInst(ISA_Opcode opcode, VISA_EMask_Ctrl emask, VISA_StateOpndHandle *surface, VISA_Oword_Num size, VISA_VectorOpnd *offset, VISA_RawOpnd *srcDst);

    VISA_BUILDER_API int AppendVISASvmBlockStoreInst(VISA_Oword_Num size, bool unaligned, VISA_VectorOpnd* address, VISA_RawOpnd *srcDst);

    VISA_BUILDER_API int AppendVISASvmBlockLoadInst(VISA_Oword_Num size, bool unaligned, VISA_VectorOpnd* address, VISA_RawOpnd *srcDst);

    VISA_BUILDER_API int AppendVISASvmScatterInst(VISA_PredOpnd *pred,
        VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
        VISA_SVM_Block_Type blockType, VISA_SVM_Block_Num numBlocks,
        VISA_RawOpnd* address, VISA_RawOpnd *srcDst);

    VISA_BUILDER_API int AppendVISASvmGatherInst(VISA_PredOpnd *pred,
        VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
        VISA_SVM_Block_Type blockType, VISA_SVM_Block_Num numBlocks,
        VISA_RawOpnd* address, VISA_RawOpnd *srcDst);

    VISA_BUILDER_API int
    AppendVISASvmAtomicInst(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
                            VISA_Exec_Size executionSize,
                            VISAAtomicOps op, unsigned short bitwidth,
                            VISA_RawOpnd *addresses, VISA_RawOpnd *src0,
                            VISA_RawOpnd *src1, VISA_RawOpnd *dst);

    VISA_BUILDER_API int AppendVISASvmGather4ScaledInst(VISA_PredOpnd             *pred,
        VISA_EMask_Ctrl    eMask,
        VISA_Exec_Size      execSize,
        VISAChannelMask           channelMask,
        VISA_VectorOpnd           *address,
        VISA_RawOpnd              *offsets,
        VISA_RawOpnd              *dst);

    VISA_BUILDER_API int AppendVISASvmScatter4ScaledInst(VISA_PredOpnd            *pred,
        VISA_EMask_Ctrl   eMask,
        VISA_Exec_Size     execSize,
        VISAChannelMask          channelMask,
        VISA_VectorOpnd          *address,
        VISA_RawOpnd             *offsets,
        VISA_RawOpnd             *src);

    VISA_BUILDER_API int AppendVISASILoad(VISA_StateOpndHandle *surface, VISAChannelMask channel, bool isSIMD16,
        VISA_RawOpnd *uOffset, VISA_RawOpnd *vOffset, VISA_RawOpnd *rOffset, VISA_RawOpnd *dst);

    VISA_BUILDER_API int AppendVISASISample(VISA_EMask_Ctrl emask, VISA_StateOpndHandle *surface, VISA_StateOpndHandle *sampler, VISAChannelMask channel, bool isSIMD16,
        VISA_RawOpnd *uOffset, VISA_RawOpnd *vOffset, VISA_RawOpnd *rOffset, VISA_RawOpnd *dst);

    VISA_BUILDER_API int AppendVISASISampleUnorm(VISA_StateOpndHandle *surface, VISA_StateOpndHandle *sampler, VISAChannelMask channel,
        VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset, VISA_VectorOpnd *deltaU, VISA_VectorOpnd *deltaV, VISA_RawOpnd *dst, CHANNEL_OUTPUT_FORMAT out);

    VISA_BUILDER_API int AppendVISASyncInst(ISA_Opcode opcode, unsigned char mask = 0);
    VISA_BUILDER_API int AppendVISAWaitInst(VISA_VectorOpnd* mask);

    VISA_BUILDER_API int AppendVISASplitBarrierInst(bool isSignal);

    VISA_BUILDER_API int AppendVISAMiscFileInst(const char *fileName);

    VISA_BUILDER_API int AppendVISAMiscLOC(unsigned int lineNumber);

    VISA_BUILDER_API int AppendVISADebugLinePlaceholder();

    VISA_BUILDER_API int AppendVISALLVMInst(void *inst);

    VISA_BUILDER_API int AppendVISAMiscRawSend(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize, unsigned char modifiers,
        unsigned int exMsgDesc, unsigned char srcSize, unsigned char dstSize, VISA_VectorOpnd *desc,
        VISA_RawOpnd *src, VISA_RawOpnd *dst);
    VISA_BUILDER_API int AppendVISAMiscRawSends(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize, unsigned char modifiers,
        unsigned ffid, VISA_VectorOpnd *exMsgDesc, unsigned char src0Size, unsigned char src1Size, unsigned char dstSize, VISA_VectorOpnd *desc,
        VISA_RawOpnd *src0, VISA_RawOpnd *src1, VISA_RawOpnd *dst, bool hasEOT);

    VISA_BUILDER_API int AppendVISAMiscVME_FBR(VISA_StateOpndHandle *surface, VISA_RawOpnd *UNIInput, VISA_RawOpnd *FBRInput, VISA_VectorOpnd* FBRMbMode, VISA_VectorOpnd *FBRSubMbShape,
        VISA_VectorOpnd *FBRSubPredMode, VISA_RawOpnd *output);

    VISA_BUILDER_API int AppendVISAMiscVME_IME(VISA_StateOpndHandle *surface, unsigned char streamMode, unsigned char searchControlMode, VISA_RawOpnd *UNIInput,
        VISA_RawOpnd *IMEInput, VISA_RawOpnd *ref0, VISA_RawOpnd *ref1, VISA_RawOpnd *costCenter, VISA_RawOpnd *output);

    VISA_BUILDER_API int AppendVISAMiscVME_SIC(VISA_StateOpndHandle *surface, VISA_RawOpnd *UNIInput, VISA_RawOpnd *SICInput, VISA_RawOpnd *output);

    VISA_BUILDER_API int AppendVISAMiscVME_IDM(VISA_StateOpndHandle *surface, VISA_RawOpnd *UNIInput, VISA_RawOpnd *IDMInput, VISA_RawOpnd *output);

    VISA_BUILDER_API int AppendVISAMEAVS(VISA_StateOpndHandle *surface, VISA_StateOpndHandle *sampler, VISAChannelMask channel, VISA_VectorOpnd *uOffset,
        VISA_VectorOpnd *vOffset, VISA_VectorOpnd *deltaU, VISA_VectorOpnd *deltaV, VISA_VectorOpnd *u2d,
        VISA_VectorOpnd *v2d, VISA_VectorOpnd *groupID, VISA_VectorOpnd *verticalBlockNumber, OutputFormatControl cntrl,
        AVSExecMode execMode, VISA_VectorOpnd *iefBypass, VISA_RawOpnd *dst);

    VISA_BUILDER_API int AppendVISAVABooleanCentroid(VISA_StateOpndHandle *surface, VISA_VectorOpnd * uOffset,
        VISA_VectorOpnd *vOffset, VISA_VectorOpnd *vSize, VISA_VectorOpnd *hSize, VISA_RawOpnd *dst);
    VISA_BUILDER_API int AppendVISAVACentroid(VISA_StateOpndHandle *surface, VISA_VectorOpnd * uOffset,
        VISA_VectorOpnd *vOffset, VISA_VectorOpnd *vSize, VISA_RawOpnd *dst);

    VISA_BUILDER_API int AppendVISAVAConvolve(VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface, VISA_VectorOpnd * uOffset,
        VISA_VectorOpnd *vOffset, CONVExecMode execMode, bool isBigKernel, VISA_RawOpnd *dst);

    VISA_BUILDER_API int AppendVISAVAErodeDilate(EDMode subOp, VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface, VISA_VectorOpnd * uOffset,
        VISA_VectorOpnd *vOffset, EDExecMode execMode, VISA_RawOpnd *dst);

    VISA_BUILDER_API int AppendVISAVAMinMax(VISA_StateOpndHandle *surface, VISA_VectorOpnd * uOffset,
        VISA_VectorOpnd *vOffset, VISA_VectorOpnd *mmMode, VISA_RawOpnd *dst);

    VISA_BUILDER_API int AppendVISAVAMinMaxFilter(VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface, VISA_VectorOpnd * uOffset,
        VISA_VectorOpnd *vOffset, OutputFormatControl cntrl, MMFExecMode execMode, VISA_VectorOpnd *mmfMode, VISA_RawOpnd *dst);

    VISA_BUILDER_API int AppendVISAVACorrelationSearch(VISA_StateOpndHandle *surface, VISA_VectorOpnd * uOffset,
        VISA_VectorOpnd *vOffset, VISA_VectorOpnd *vOrigin, VISA_VectorOpnd *hOrigin,
        VISA_VectorOpnd *xDirectionSize, VISA_VectorOpnd *yDirectionSize,
        VISA_VectorOpnd *xDirectionSearchSize, VISA_VectorOpnd *yDirectionSearchSize,
        VISA_RawOpnd *dst);

    VISA_BUILDER_API int AppendVISAVAFloodFill(bool is8Connect, VISA_RawOpnd *pixelMaskHDirection,
        VISA_VectorOpnd *pixelMaskVDirectionLeft, VISA_VectorOpnd *pixelMaskVDirectionRight,
        VISA_VectorOpnd *loopCount, VISA_RawOpnd *dst);

    VISA_BUILDER_API int AppendVISAVALBPCorrelation(VISA_StateOpndHandle *surface, VISA_VectorOpnd * uOffset,
        VISA_VectorOpnd *vOffset, VISA_VectorOpnd *disparity,
        VISA_RawOpnd *dst);

    VISA_BUILDER_API int AppendVISAVALBPCreation(VISA_StateOpndHandle *surface, VISA_VectorOpnd * uOffset,
        VISA_VectorOpnd *vOffset, LBPCreationMode mode,
        VISA_RawOpnd *dst);

    VISA_BUILDER_API int AppendVISAVAConvolve1D(VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface, VISA_VectorOpnd * uOffset,
        VISA_VectorOpnd *vOffset, CONVExecMode mode, Convolve1DDirection direction, VISA_RawOpnd *dst);

    VISA_BUILDER_API int AppendVISAVAConvolve1Pixel(VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface, VISA_VectorOpnd * uOffset,
        VISA_VectorOpnd *vOffset, CONV1PixelExecMode mode, VISA_RawOpnd *offsets, VISA_RawOpnd *dst);

    VISA_BUILDER_API int AppendVISAVAHDCConvolve(VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface, VISA_VectorOpnd * uOffset,
        VISA_VectorOpnd *vOffset, HDCReturnFormat returnFormat, CONVHDCRegionSize regionSize,
        VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset, VISA_VectorOpnd *yOffset);

    VISA_BUILDER_API int AppendVISAVAHDCErodeDilate(EDMode subOp, VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface, VISA_VectorOpnd * uOffset,
        VISA_VectorOpnd *vOffset, VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset,
        VISA_VectorOpnd *yOffset);

    VISA_BUILDER_API int AppendVISAVAHDCMinMaxFilter(VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface, VISA_VectorOpnd * uOffset,
        VISA_VectorOpnd *vOffset, HDCReturnFormat returnFormat, MMFEnableMode mmfMode,
        VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset, VISA_VectorOpnd *yOffset);

    VISA_BUILDER_API int AppendVISAVAHDCLBPCorrelation(VISA_StateOpndHandle *surface, VISA_VectorOpnd * uOffset,
        VISA_VectorOpnd *vOffset, VISA_VectorOpnd *disparity,
        VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset, VISA_VectorOpnd *yOffset);

    VISA_BUILDER_API int AppendVISAVAHDCLBPCreation(VISA_StateOpndHandle *surface, VISA_VectorOpnd * uOffset,
        VISA_VectorOpnd *vOffset, LBPCreationMode mode,
        VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset, VISA_VectorOpnd *yOffset);

    VISA_BUILDER_API int AppendVISAVAHDCConvolve1D(VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface, VISA_VectorOpnd * uOffset,
        VISA_VectorOpnd *vOffset, HDCReturnFormat returnFormat, Convolve1DDirection direction,
        VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset, VISA_VectorOpnd *yOffset);

    VISA_BUILDER_API int AppendVISAVAHDCConvolve1Pixel(VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface, VISA_VectorOpnd * uOffset,
        VISA_VectorOpnd *vOffset, HDCReturnFormat returnFormat, VISA_RawOpnd *offsets,
        VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset, VISA_VectorOpnd *yOffset);

    VISA_BUILDER_API int AppendVISALifetime(VISAVarLifetime startOrEnd, VISA_VectorOpnd *varId);


    /********** APPEND INSTRUCTION APIS END   ******************/

    /********** APPEND 3D Instructions START ******************/

    VISA_BUILDER_API int AppendVISA3dSampler(VISASampler3DSubOpCode subOpcode,
        bool pixelNullMask,
        bool cpsEnable,
        bool uniformSampler,
        VISA_PredOpnd *pred,
        VISA_EMask_Ctrl emask,
        VISA_Exec_Size executionSize,
        VISAChannelMask srcChannel,
        VISA_VectorOpnd *aoffimmi,
        VISA_StateOpndHandle *sampler,
        VISA_StateOpndHandle *surface,
        VISA_RawOpnd *dst,
        int numMsgSpecificOpnds,
        VISA_RawOpnd **opndArray);

    VISA_BUILDER_API int AppendVISA3dLoad(VISASampler3DSubOpCode subOpcode,
        bool pixelNullMask,
        VISA_PredOpnd *pred,
        VISA_EMask_Ctrl emask,
        VISA_Exec_Size executionSize,
        VISAChannelMask srcChannel,
        VISA_VectorOpnd *aoffimmi,
        VISA_StateOpndHandle *surface,
        VISA_RawOpnd *dst,
        int numMsgSpecificOpnds,
        VISA_RawOpnd ** opndArray);

    VISA_BUILDER_API int AppendVISA3dGather4(VISASampler3DSubOpCode subOpcode,
        bool pixelNullMask,
        VISA_PredOpnd *pred,
        VISA_EMask_Ctrl emask,
        VISA_Exec_Size executionSize,
        VISASourceSingleChannel srcChannel,
        VISA_VectorOpnd *aoffimmi,
        VISA_StateOpndHandle *sampler,
        VISA_StateOpndHandle *surface,
        VISA_RawOpnd *dst,
        int numMsgSpecificOpnds,
        VISA_RawOpnd ** opndArray);

    VISA_BUILDER_API int AppendVISA3dInfo(VISASampler3DSubOpCode subOpcode, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize, VISAChannelMask srcChannel, VISA_StateOpndHandle *surface, VISA_RawOpnd *lod, VISA_RawOpnd *dst);

    VISA_BUILDER_API int AppendVISA3dRTWrite(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize, VISA_VectorOpnd* renderTargetIndex, vISA_RT_CONTROLS cntrls,
        VISA_StateOpndHandle *surface, VISA_RawOpnd *r1HeaderOpnd, VISA_VectorOpnd *sampleIndex,
        uint8_t numMsgSpecificOpnds, VISA_RawOpnd **opndArray);

    VISA_BUILDER_API int AppendVISA3dRTWriteCPS(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize, VISA_VectorOpnd* renderTargetIndex, vISA_RT_CONTROLS cntrls,
        VISA_StateOpndHandle *surface, VISA_RawOpnd *r1HeaderOpnd, VISA_VectorOpnd *sampleIndex,
        VISA_VectorOpnd *cPSCounter, uint8_t numMsgSpecificOpnds, VISA_RawOpnd **opndArray);

    VISA_BUILDER_API int AppendVISA3dURBWrite(VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
        VISA_Exec_Size executionSize, unsigned char numberOutputParams,
        VISA_RawOpnd *channelMask, unsigned short globalOffset, VISA_RawOpnd *URBHandle,
        VISA_RawOpnd *perSLotOffset, VISA_RawOpnd *vertexData);

    VISA_BUILDER_API int AppendVISA3dTypedAtomic(
        VISAAtomicOps subOp, bool is16Bit, VISA_PredOpnd *pred,
        VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
        VISA_StateOpndHandle *surface, VISA_RawOpnd *u, VISA_RawOpnd *v,
        VISA_RawOpnd *r, VISA_RawOpnd *lod, VISA_RawOpnd *src0,
        VISA_RawOpnd *src1, VISA_RawOpnd *dst);

    /********** APPEND 3D Instructions END ******************/

    /********** MISC APIs START *************************/
    VISA_BUILDER_API int GetGenxBinary(void *&buffer, int &size);
    VISA_BUILDER_API int GetJitInfo(FINALIZER_INFO *&jitInfo);
    VISA_BUILDER_API int GetCompilerStats(CompilerStats &compilerStats);
    VISA_BUILDER_API int GetErrorMessage(const char *&errorMsg) const;
    VISA_BUILDER_API virtual int GetGenxDebugInfo(void *&buffer, unsigned int &size, void*&, unsigned int&);
    /// GetGenRelocEntryBuffer -- allocate and return a buffer of all GenRelocEntry that are created by vISA
    VISA_BUILDER_API int GetGenRelocEntryBuffer(void *&buffer, unsigned int &byteSize, unsigned int &numEntries);
    /// GetRelocations -- add vISA created relocations into given relocation list
    /// This get the same information as GetGenRelocEntryBuffer, but in different foramt
    VISA_BUILDER_API int GetRelocations(RelocListType &relocs);
    VISA_BUILDER_API int GetGTPinBuffer(void*& buffer, unsigned int& size);
    VISA_BUILDER_API int SetGTPinInit(void* buffer);
    VISA_BUILDER_API int GetFreeGRFInfo(void*& buffer, unsigned int& size);

    VISA_BUILDER_API int GetFunctionId(unsigned int& id) const;

    ///Gets declaration id GenVar
    VISA_BUILDER_API int getDeclarationID(VISA_GenVar *decl) const;

    ///Gets declaration id VISA_AddrVar
    VISA_BUILDER_API int getDeclarationID(VISA_AddrVar *decl) const;

    ///Gets declaration id VISA_PredVar
    VISA_BUILDER_API int getDeclarationID(VISA_PredVar *decl) const;

    ///Gets declaration id VISA_SamplerVar
    VISA_BUILDER_API int getDeclarationID(VISA_SamplerVar *decl) const;

    ///Gets declaration id VISA_SurfaceVar
    VISA_BUILDER_API int getDeclarationID(VISA_SurfaceVar *decl) const;

    ///Gets declaration id VISA_LabelVar
    VISA_BUILDER_API int getDeclarationID(VISA_LabelVar *decl) const;

    ///Gets gen binary offset
    VISA_BUILDER_API int64_t getGenOffset() const;

    ///Gets gen binary size within instruction heap
    VISA_BUILDER_API int64_t getGenSize() const;

    //Gets the VISA string format for the variable
    VISA_BUILDER_API std::string getVarName(VISA_GenVar* decl) const;
    VISA_BUILDER_API std::string getVarName(VISA_PredVar* decl) const;
    VISA_BUILDER_API std::string getVarName(VISA_AddrVar* decl) const;
    VISA_BUILDER_API std::string getVarName(VISA_SurfaceVar* decl) const;
    VISA_BUILDER_API std::string getVarName(VISA_SamplerVar* decl) const;

    //Gets the VISA string format for the operand
    VISA_BUILDER_API std::string getVectorOperandName(VISA_VectorOpnd *opnd, bool showRegion) const;
    VISA_BUILDER_API std::string getPredicateOperandName(VISA_PredOpnd* opnd) const;

    /********** MISC APIs END *************************/
    int CreateVISAPredicateSrcOperand(VISA_VectorOpnd *& opnd, VISA_PredVar *decl, unsigned int size);

    int CreateVISAPredicateDstOperand(VISA_VectorOpnd *& opnd, VISA_PredVar *decl, uint32_t size);

    int CreateVISAAddressOperand(VISA_VectorOpnd *&opnd, VISA_AddrVar *decl, unsigned int offset, unsigned int width, bool isDst);

    int CreateVISAPredicateOperandvISA(VISA_PredOpnd *& opnd, VISA_PredVar *decl, VISA_PREDICATE_STATE state, VISA_PREDICATE_CONTROL cntrl);

    int CreateGenNullRawOperand(VISA_RawOpnd *& opnd, bool isDst);

    int CreateGenRawSrcOperand(VISA_RawOpnd *& cisa_opnd);
    int CreateGenRawDstOperand(VISA_RawOpnd *& cisa_opnd);

    int CreateVISAIndirectGeneralOperand(VISA_VectorOpnd *& opnd, VISA_AddrVar *cisa_decl, VISA_Modifier mod, unsigned int addrOffset, unsigned short immediateOffset,
        unsigned short verticalStride, unsigned short width, unsigned short horizontalStride, VISA_Type type, bool isDst);

    int AppendVISA3dSamplerMsgGeneric(ISA_Opcode opcode,
        VISASampler3DSubOpCode subOpcode,
        bool pixelNullMask,
        bool cpsEnable,
        bool uniformSampler,
        VISA_PredOpnd *pred,
        VISA_EMask_Ctrl emask,
        VISA_Exec_Size executionSize,
        ChannelMask srcChannel,
        VISA_VectorOpnd *aoffimmi,
        VISA_StateOpndHandle *sampler,
        VISA_StateOpndHandle *surface,
        VISA_RawOpnd *dst,
        unsigned int numMsgSpecificOpnds,
        VISA_RawOpnd **opndArray);

    int AddAttributeToVarGeneric(CISA_GEN_VAR *decl, const char* varName, unsigned int size, void *val);

    int CreateStateVar(CISA_GEN_VAR *&decl, Common_ISA_Var_Class type, const char* name, unsigned int numberElements);

    int CreateVISAInputVar(CISA_GEN_VAR *decl, uint16_t offset, uint16_t size, uint8_t implicitKind);

    int CreateVISAAddressOfOperandGeneric(VISA_VectorOpnd *&cisa_opnd, CISA_GEN_VAR *decl, unsigned int offset);

    int CreateVISAStateOperand(VISA_VectorOpnd *& opnd, CISA_GEN_VAR *decl, Common_ISA_State_Opnd_Class opnd_class, uint8_t size, unsigned char offset, bool useAsDst);

    int CreateDummyLabelOperand(VISA_LabelOpnd *& opnd, char *name, VISA_Label_Kind kind);

    void setGenxBinaryBuffer(void *buffer, int size){ m_genx_binary_buffer = (char *)buffer; m_genx_binary_size = size; }
    void setJitInfo(FINALIZER_INFO* jitInfo){ m_jitInfo = jitInfo; }
    // char * getErrorMsgPtr(){ return errorMessage; }

    std::string getOutputAsmPath() const { return m_asmName; }

    int compileFastPath();

    unsigned int m_magic_number;
    unsigned char m_major_version;
    unsigned char m_minor_version;

    void* compilePostOptimize(unsigned int& binarySize);

    void setInputSize(uint8_t size);
    void setReturnSize(unsigned int size);

   bool getIsGenBothPath() {
        return (mBuildOption == VISA_BUILDER_GEN ||
            mBuildOption == VISA_BUILDER_BOTH);
    }

    // This member holds symbolic index of function when invoked via
    // API path. Builder client can use this id when invoking this
    // stack call function. For a kernel instance, this is not useful.
    unsigned int m_functionId;

    unsigned getvIsaInstCount() const { return m_vISAInstCount; };


    bool isFCCallableKernel() const { return mIsFCCallableKernel; }
    void setFCCallableKernel(bool value) { mIsFCCallableKernel = value; }

    bool isFCCallerKernel() const { return mIsFCCallerKernel; }
    void setFCCallerKernel(bool value) { mIsFCCallerKernel = value; }

    bool isFCComposableKernel() const { return mIsFCComposableKernel; }
    void setFCComposableKernel(bool value) { mIsFCComposableKernel = value; }

    unsigned int getGenVarCount() const
    {
        return (uint32_t)m_var_info_list.size();
    }

    CISA_GEN_VAR* getGenVar(unsigned int index)
    {
        return m_var_info_list[index];
    }

    unsigned int getAddrVarCount() const
    {
        return (uint32_t)m_addr_info_list.size();
    }

    CISA_GEN_VAR* getAddrVar(unsigned int index)
    {
        return m_addr_info_list[index];
    }

    unsigned int getPredVarCount() const
    {
        return (uint32_t)m_pred_info_list.size();
    }

    CISA_GEN_VAR* getPredVar(unsigned int index)
    {
        auto it = m_pred_info_list.begin();
        std::advance(it, index);

        return (*it);
    }

    unsigned int getSamplerVarCount() const
    {
        return (uint32_t)m_sampler_info_list.size();
    }

    CISA_GEN_VAR* getSamplerVar(unsigned int index)
    {
        auto it = m_sampler_info_list.begin();
        std::advance(it, index);

        return (*it);
    }

    unsigned int getSurfaceVarCount() const
    {
        return (uint32_t)m_surface_info_list.size();
    }

    CISA_GEN_VAR* getSurfaceVar(unsigned int index)
    {
        auto it = m_surface_info_list.begin();
        std::advance(it, index);

        return (*it);
    }

    Options * getOptions() { return m_options; }

    bool IsAsmWriterMode() const { return m_options->getOption(vISA_IsaAssembly); }

    void computeAndEmitDebugInfo(std::list<VISAKernelImpl*>& functions);

private:
    void ensureVariableNameUnique(const char *&varName);
    void generateVariableName(Common_ISA_Var_Class Ty, const char *&varName);

    void dumpDebugFormatFile(std::vector<vISA::DebugInfoFormat>& debugSymbols, std::string filename);
    void patchLabels();
    int InitializeFastPath();
    void initCompilerStats();
    int predefinedVarRegAssignment();
    int calculateTotalInputSize();
    int compileTillOptimize();
    void recordFinalizerInfo();

    // Re-adjust indirect call target after swsb
    void adjustIndirectCallOffset();

    CisaFramework::CisaInst* AppendVISASvmGeneralScatterInst(VISA_PredOpnd* pred,
        VISA_EMask_Ctrl emask, VISA_Exec_Size execSize, unsigned char blockSize,
        unsigned char numBlocks, VISA_RawOpnd* address, VISA_RawOpnd *srcDst, bool isRead);

    CisaFramework::CisaInst *
        PackCisaInsnForSVMGather4Scatter4Scaled(unsigned subOpc,
        VISA_PredOpnd *pred,
        VISA_EMask_Ctrl eMask,
        VISA_Exec_Size executionSize,
        ChannelMask chMask,
        VISA_VectorOpnd *address,
        VISA_RawOpnd *offsets,
        VISA_RawOpnd *srcOrDst);

    VISA_opnd * getOpndFromPool();

    void AppendVISAInstCommon();

    void createBindlessSampler();

    kernel_format_t m_cisa_kernel;

    unsigned int m_num_pred_vars;
    //size of various arrays in kernel header.
    //used for buffer size allocation.
    unsigned int m_string_pool_size; //done
    unsigned int m_var_info_size; //done
    unsigned int m_adress_info_size;
    unsigned int m_predicate_info_size; //done
    unsigned int m_label_info_size; //done
    unsigned int m_input_info_size; //done
    unsigned int m_attribute_info_size; //done
    unsigned int m_instruction_size; //done
    unsigned int m_surface_info_size;
    unsigned int m_sampler_info_size;

    unsigned long m_genx_binary_size;
    char * m_genx_binary_buffer;
    unsigned long m_genx_debug_info_size;
    char * m_genx_debug_info_buffer;
    FINALIZER_INFO* m_jitInfo;
    CompilerStats m_compilerStats;

    unsigned long m_cisa_binary_size;
    char * m_cisa_binary_buffer;

    unsigned long m_kernel_data_size;

    unsigned long m_bytes_written_cisa_buffer;

    unsigned long m_input_offset;

    string_pool_entry** m_branch_targets;
    std::vector<std::string> m_string_pool;
    CisaFramework::CisaInst * m_lastInst;
    bool m_isKernel;
    unsigned int m_resolvedIndex;

    vISA::Mem_Manager m_mem;
    std::string m_name;
    std::string m_asmName;
    std::string m_sanitizedName;

    std::list<CisaFramework::CisaInst *> m_instruction_list;

    unsigned int m_var_info_count;
    std::vector<CISA_GEN_VAR*> m_var_info_list;

    unsigned int m_addr_info_count;
    std::vector<CISA_GEN_VAR *> m_addr_info_list;

    unsigned int m_pred_info_count;
    std::vector<CISA_GEN_VAR *> m_pred_info_list;

    unsigned int m_forward_label_count;
    std::map<unsigned int, std::string> m_forward_label_pool;

    unsigned int m_sampler_count;
    std::vector<CISA_GEN_VAR*> m_sampler_info_list;

    unsigned int m_surface_count;
    std::vector<CISA_GEN_VAR*> m_surface_info_list;

    VISA_SamplerVar* m_bindlessSampler;

    std::map<unsigned int, unsigned int> m_declID_to_PredefinedSurfaceID_map;

    unsigned int m_input_count;
    std::vector<input_info_t *> m_input_info_list;
    // std::map<unsigned int, unsigned int> m_declID_to_inputID_map;

    unsigned int m_attribute_count;
    std::list<attribute_info_t *> m_attribute_info_list;

    unsigned int m_label_count;
    std::vector<label_info_t *> m_label_info_list;

    // list of cisa operands representing labels that need to be resolved
    std::list<VISA_opnd *> m_pending_labels;
    std::list<std::string> m_pending_label_names;

    // maps a variable name to the var pointer
    // unique vars are unique to the entire program
    // general vars must be unique within the same scope, but can be redefined across scopes
    typedef std::map<std::string, CISA_GEN_VAR *> GenDeclNameToVarMap;
    std::vector<GenDeclNameToVarMap> m_GenNamedVarMap;
    GenDeclNameToVarMap m_UniqueNamedVarMap;
    // std::vector<VISAScope> m_GenNamedVarMap;
    // VISAScope m_UniqueNamedVarMap;

    std::unordered_map<std::string, VISA_LabelOpnd *> m_label_name_to_index_map;
    std::unordered_map<std::string, VISA_LabelOpnd *> m_funcName_to_labelID_map;

    char errorMessage[MAX_ERROR_MSG_LEN];

    VISA_BUILDER_OPTION mBuildOption;
    vISA::G4_Kernel* m_kernel;
    CISA_IR_Builder* m_CISABuilder;
    vISA::IR_Builder* m_builder;
    vISA::Mem_Manager *m_globalMem;
    vISA::Mem_Manager *m_kernelMem;
    //customized allocator for allocating
    //It is very important that the same allocator is used by all instruction lists
    //that might be joined/spliced.
    INST_LIST_NODE_ALLOCATOR m_instListNodeAllocator;
    unsigned int m_kernelID;
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
    std::unordered_set<std::string> varNames;

    int m_vISAInstCount;
    print_decl_index_t m_printDeclIndex;

    bool mIsFCCallableKernel;
    bool mIsFCCallerKernel;
    bool mIsFCComposableKernel;

    void computeFCInfo(vISA::BinaryEncodingBase* binEncodingInstance);
    void computeFCInfo();
    //memory managed by the entity that creates vISA Kernel object
    Options *m_options;

    bool getIntKernelAttributeValue(const char* attrName, int& value);
    void createKernelAttributes() {
        void* pmem = m_mem.alloc(sizeof(vISA::Attributes));
        m_kernelAttrs = new (pmem) vISA::Attributes();
    }

    // Shared with G4_kernel
    vISA::Attributes* m_kernelAttrs;
};

class VISAKernel_format_provider : public print_format_provider_t
{
protected:
    const VISAKernelImpl* m_kernel;

public:
    VISAKernel_format_provider(const VISAKernelImpl* kernel)
        : m_kernel(kernel) { }

    uint32_t getNameIndex() const
    {
        return m_kernel->m_cisa_kernel.name_index;
    }
    const char* getString(uint32_t str_id) const
    {
        assert(str_id < m_kernel->m_string_pool.size());
        return m_kernel->m_string_pool[str_id].c_str();
    }
    uint32_t getStringCount() const
    {
        return m_kernel->m_string_pool.size();
    }
    const label_info_t* getLabel(uint16_t label_id) const
    {
        assert(label_id < m_kernel->m_label_info_list.size());
        return m_kernel->m_label_info_list[label_id];
    }
    unsigned short getLabelCount() const
    {
        return m_kernel->m_label_count;
    }
    const var_info_t* getPredefVar(unsigned var_id) const
    {
        assert(var_id < m_kernel->m_num_pred_vars);
        return &m_kernel->m_var_info_list[var_id]->genVar;
    }
    const var_info_t* getVar(unsigned var_id) const
    {
        assert(var_id + m_kernel->m_num_pred_vars < m_kernel->m_var_info_list.size());
        return &m_kernel->m_var_info_list[var_id + m_kernel->m_num_pred_vars]->genVar;
    }
    uint32_t getVarCount() const
    {
        return m_kernel->m_var_info_count - m_kernel->m_num_pred_vars;
    }
    const attribute_info_t* getAttr(unsigned id) const
    {
        auto it = m_kernel->m_attribute_info_list.begin();
        std::advance(it, id);
        return *it;
    }
    unsigned getAttrCount() const
    {
        return m_kernel->m_attribute_count;
    }
    const addr_info_t* getAddr(unsigned id) const
    {
        assert(id < m_kernel->m_addr_info_list.size());
        return &m_kernel->m_addr_info_list[id]->addrVar;
    }
    unsigned short getAddrCount() const
    {
        return m_kernel->m_addr_info_count;
    }
    const pred_info_t* getPred(unsigned id) const
    {
        assert(id < m_kernel->m_pred_info_list.size());
        return &m_kernel->m_pred_info_list[id]->predVar;
    }
    unsigned short getPredCount() const
    {
        return m_kernel->m_pred_info_count;
    }
    const state_info_t* getPredefSurface(unsigned id) const
    {
        assert(id < Get_CISA_PreDefined_Surf_Count());
        return  &m_kernel->m_surface_info_list[id]->stateVar;
    }
    const state_info_t* getSurface(unsigned id) const
    {
        assert(id + Get_CISA_PreDefined_Surf_Count() < m_kernel->m_surface_info_list.size());
        return &m_kernel->m_surface_info_list[id + Get_CISA_PreDefined_Surf_Count()]->stateVar;
    }
    unsigned char getSurfaceCount() const
    {
        return m_kernel->m_surface_count - Get_CISA_PreDefined_Surf_Count();
    }
    const state_info_t* getSampler(unsigned id) const
    {
        assert(id < m_kernel->m_sampler_info_list.size());
        return &m_kernel->m_sampler_info_list[id]->stateVar;
    }
    unsigned char getSamplerCount() const
    {
        return m_kernel->m_sampler_count;
    }
    const input_info_t* getInput(unsigned id) const
    {
        assert(id < m_kernel->m_input_info_list.size());
        return m_kernel->m_input_info_list[id];
    }
    uint32_t getInputCount() const
    {
        return m_kernel->m_input_count;
    }
};

#endif //VISA_KERNEL_H
