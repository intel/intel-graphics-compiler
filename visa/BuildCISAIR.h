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

#ifndef _BUILDCISAIR_H_
#define _BUILDCISAIR_H_

#include <sstream>

namespace vISA
{
    class Mem_Manager;
}
class CisaKernel;
class CisaBinary;
class VISAKernelImpl;
class VISAFunction;

extern FILE *CISAin;
extern FILE *CISAout;
extern int CISAdebug;

#include "VISABuilderAPIDefinition.h"
#include "inc/common/sku_wa.h"

class Options;

class CISA_IR_Builder : public VISABuilder
{
public:

    CISA_IR_Builder(VISA_BUILDER_OPTION buildOption, int majorVersion, int minorVersion,
        const PWA_TABLE pWaTable) : m_mem(4096), m_pWaTable(pWaTable)
    {
        memset(&m_header, 0, sizeof(m_header));

        mBuildOption = buildOption;
        m_kernel_count = 0;
        m_function_count = 0;

        m_header.major_version = majorVersion;
        m_header.minor_version = minorVersion;
        m_header.magic_number = COMMON_ISA_MAGIC_NUM;

        m_cisaBinary = new (m_mem) CisaFramework::CisaBinary(&m_options);
    }

    virtual ~CISA_IR_Builder();

    /**************START VISA BUILDER API*****************************/

    static int CreateBuilder(
        CISA_IR_Builder *&builder,
        vISABuilderMode mode,
        VISA_BUILDER_OPTION buildOption,
        TARGET_PLATFORM platform,
        int numArgs,
        const char* flags[],
        const PWA_TABLE pWaTable = nullptr);
    static int DestroyBuilder(CISA_IR_Builder *builder);
    VISA_BUILDER_API virtual int AddKernel(VISAKernel *& kernel, const char* kernelName);
    VISA_BUILDER_API virtual int AddFunction(VISAFunction *& function, const char* functionName);
    VISA_BUILDER_API virtual int Compile(const char * isaFileNameint, std::ostream * os = nullptr, bool emit_visa_only = false);

    VISA_BUILDER_API void SetOption(vISAOptions option, bool val) { m_options.setOption(option, val); }
    VISA_BUILDER_API void SetOption(vISAOptions option, uint32_t val) { m_options.setOption(option, val); }
    VISA_BUILDER_API void SetOption(vISAOptions option, const char *val) { m_options.setOption(option, val); }

    // Used for inline asm code generation
    VISA_BUILDER_API virtual int ParseVISAText(const std::string& visaHeader, const std::string& visaText, const std::string& visaTextFile);
    VISA_BUILDER_API virtual int ParseVISAText(const std::string& visaFile);
    VISA_BUILDER_API virtual int WriteVISAHeader();
    VISA_BUILDER_API std::stringstream& GetAsmTextStream() { return m_ssIsaAsm; }
    VISA_BUILDER_API std::stringstream& GetAsmTextHeaderStream() { return m_ssIsaAsmHeader; }
    VISA_BUILDER_API virtual VISAKernel* GetVISAKernel(const std::string& kernelName);
    VISA_BUILDER_API virtual int ClearAsmTextStreams();

    /**************END VISA BUILDER API*************************/

    string_pool_entry** branch_targets;
    common_isa_header m_header;

    VISAKernelImpl *m_kernel;
    CisaFramework::CisaBinary *m_cisaBinary;
    VISAKernelImpl * get_kernel() { return m_kernel; }

    std::stringstream& criticalMsgStream()
    {
        return criticalMsg;
    }

    std::string GetCriticalMsg()
    {
        return criticalMsg.str();
    }

    std::string    m_errorMessage;

    static void cat(std::stringstream &ss) { }
    template <typename T, typename...Ts>
    static void cat(std::stringstream &ss, T t, Ts...ts) {
        ss << t;
        cat(ss, ts...);
    }

    template <typename...Ts>
    void RecordParseError(int lineNum, Ts...ts)
    {
        if (HasParseError())
            return;
        std::stringstream ss;
        ss << "near line " << lineNum << ": ";
        cat(ss, ts...);
        m_errorMessage = ss.str();
        criticalMsg << m_errorMessage << "\n";
    }

    bool HasParseError() const {return !m_errorMessage.empty();}
    std::string GetParseError() {return m_errorMessage;}


    const PWA_TABLE getWATable() { return m_pWaTable; }

    void CISA_IR_setVersion(unsigned char major_ver, unsigned char minor_ver)
    {
        m_header.major_version = major_ver;
        m_header.minor_version = minor_ver;
    }

    Common_ISA_Input_Class get_input_class(Common_ISA_Var_Class var_class);

    //CISA Build Functions
    bool CISA_IR_initialization(const char *kernel_name, int line_no);
    bool CISA_general_variable_decl(
        const char * var_name,
        unsigned int var_elemts_num,
        VISA_Type data_type,
        VISA_Align var_align,
        const char * var_alias_name,
        int var_alias_offset,
        attr_gen_struct scope,
        int line_no);

    bool CISA_addr_variable_decl(
        const char *var_name,
        unsigned int var_elements,
        VISA_Type data_type,
        attr_gen_struct scope,
        int line_no);

    bool CISA_predicate_variable_decl(
        const char *var_name,
        unsigned int var_elements,
        attr_gen_struct reg,
        int line_no);

    bool CISA_sampler_variable_decl(
        const char *var_name, int num_elts, const char* name, int line_no);

    bool CISA_surface_variable_decl(
        const char *var_name, int num_elts, const char* name,
        attr_gen_struct attr, int line_no);

    bool CISA_input_directive(
        const char* var_name, short offset, unsigned short size, int line_no);

    bool CISA_implicit_input_directive(
        const char * argName, const char * varName,
        short offset, unsigned short size, int line_no);

    //bool CISA_attr_directive(char* input_name, attribute_info_t* attr);
    bool CISA_attr_directive(const char* input_name, const char* input_var, int line_no);
    bool CISA_attr_directiveNum(const char* input_name, uint32_t input_var, int line_no);

    bool CISA_create_label(const char * label_name, int line_no);
    bool CISA_function_directive(const char* func_name);


    bool CISA_create_arith_instruction(
        VISA_opnd * cisa_pred,
        ISA_Opcode opcode,
        bool  sat,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        VISA_opnd * dst_cisa,
        VISA_opnd * src0_cisa,
        VISA_opnd * src1_cisa,
        VISA_opnd * src2_cisa,
        int line_no);
    bool CISA_create_arith_instruction2(
        VISA_opnd * cisa_pred,
        ISA_Opcode opcode,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        VISA_opnd * dst_cisa,
        VISA_opnd * src0_cisa,
        VISA_opnd * src1_cisa,
        VISA_opnd * src2_cisa,
        int line_no);

    bool CISA_create_mov_instruction(
        VISA_opnd *pred,
        ISA_Opcode opcode,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        bool sat,
        VISA_opnd *dst,
        VISA_opnd *src0,
        int line_no);

    bool CISA_create_mov_instruction(
        VISA_opnd *dst, CISA_GEN_VAR *src0, int line_no);

    bool CISA_create_movs_instruction(
        VISA_EMask_Ctrl emask,
        ISA_Opcode opcode,
        unsigned exec_size,
        VISA_opnd *dst,
        VISA_opnd *src0,
        int line_no);


    bool CISA_create_branch_instruction(
        VISA_opnd *pred,
        ISA_Opcode opcode,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        const char *target_label,
        int line_no);


    bool CISA_create_cmp_instruction(
        VISA_Cond_Mod sub_op,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        CISA_GEN_VAR* decl,
        VISA_opnd *src0,
        VISA_opnd *src1,
        int line_no);

    bool CISA_create_cmp_instruction(
        VISA_Cond_Mod sub_op,
        ISA_Opcode opcode,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        VISA_opnd *dst,
        VISA_opnd *src0,
        VISA_opnd *src1,
        int line_no);

    bool CISA_create_media_instruction(
        ISA_Opcode opcode,
        MEDIA_LD_mod media_mod,
        int row_off,
        int elem_off,
        unsigned int plane_ID,
        const char * surface_name,
        VISA_opnd *src0,
        VISA_opnd *src1,
        VISA_opnd *raw_dst,
        int line_no);


    bool CISA_Create_Ret(
        VISA_opnd *pred_opnd,
        ISA_Opcode opcode,
        VISA_EMask_Ctrl emask,
        unsigned int exec_size,
        int line_no);

    bool CISA_create_oword_instruction(
        ISA_Opcode opcode,
        bool media_mod,
        unsigned int size,
        const char *surface_name,
        VISA_opnd *src0,
        VISA_opnd *raw_dst_src,
        int line_no);

    bool CISA_create_svm_block_instruction(
        SVMSubOpcode subopcode,
        unsigned     owords,
        bool         unaligned,
        VISA_opnd*   address,
        VISA_opnd*   srcDst,
        int          line_no);

    bool CISA_create_svm_scatter_instruction(
        VISA_opnd*   pred,
        SVMSubOpcode subopcode,
        VISA_EMask_Ctrl emask,
        unsigned     exec_size,
        unsigned     blockSize,
        unsigned     numBlocks,
        VISA_opnd*   addresses,
        VISA_opnd*   srcDst,
        int          line_no);

    bool CISA_create_svm_atomic_instruction(
        VISA_opnd* pred,
        VISA_EMask_Ctrl emask,
        unsigned   exec_size,
        VISAAtomicOps op,
        unsigned short bitwidth,
        VISA_opnd* addresses,
        VISA_opnd* src0,
        VISA_opnd* src1,
        VISA_opnd* dst,
        int line_no);

    bool CISA_create_svm_gather4_scaled(
        VISA_opnd              *pred,
        VISA_EMask_Ctrl        eMask,
        unsigned               execSize,
        ChannelMask            chMask,
        VISA_opnd              *address,
        VISA_opnd              *offsets,
        VISA_opnd              *src,
        int                    lineNum);

    bool CISA_create_svm_scatter4_scaled(
        VISA_opnd              *pred,
        VISA_EMask_Ctrl eMask,
        unsigned               execSize,
        ChannelMask            chMask,
        VISA_opnd              *address,
        VISA_opnd              *offsets,
        VISA_opnd              *src,
        int                    lineNum);

    bool CISA_create_address_instruction(
        ISA_Opcode opcode,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        VISA_opnd *dst,
        VISA_opnd *src0,
        VISA_opnd *src1,
        int line_no);


    bool CISA_create_logic_instruction(
        VISA_opnd *pred,
        ISA_Opcode opcode,
        bool sat,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        VISA_opnd *dst,
        VISA_opnd *src0,
        VISA_opnd *src1,
        VISA_opnd *src2,
        VISA_opnd *src3,
        int line_no);

    bool CISA_create_logic_instruction(
        ISA_Opcode opcode,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        CISA_GEN_VAR *dst,
        CISA_GEN_VAR *src0,
        CISA_GEN_VAR *src1,
        int line_no);

    bool CISA_create_math_instruction(
        VISA_opnd *pred,
        ISA_Opcode opcode,
        bool  sat,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        VISA_opnd *dst,
        VISA_opnd *src0,
        VISA_opnd *src1,
        int line_no);

    bool CISA_create_setp_instruction(
        ISA_Opcode opcode,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        CISA_GEN_VAR *dst,
        VISA_opnd *src0,
        int line_no);

    bool CISA_create_sel_instruction(
        ISA_Opcode opcode,
        bool sat,
        VISA_opnd *pred,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        VISA_opnd *dst,
        VISA_opnd *src0,
        VISA_opnd *src1,
        int line_no);

    bool CISA_create_fminmax_instruction(
        bool minmax,
        ISA_Opcode opcode,
        bool sat,
        VISA_opnd *pred,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        VISA_opnd *dst,
        VISA_opnd *src0,
        VISA_opnd *src1,
        int line_no);

    bool CISA_create_scatter_instruction(
        ISA_Opcode opcode,
        int elemNum,
        VISA_EMask_Ctrl emask,
        unsigned elt_size,
        bool modifier,
        const char *surface_name,
        VISA_opnd *global_offset, //global_offset
        VISA_opnd *element_offset, //element_offset
        VISA_opnd *raw_dst_src, //dst/src
        int line_no);

    bool CISA_create_scatter4_typed_instruction(
        ISA_Opcode opcode,
        VISA_opnd *pred,
        ChannelMask ch_mask,
        VISA_EMask_Ctrl emask,
        unsigned execSize,
        const char* surfaceName,
        VISA_opnd *uOffset,
        VISA_opnd *vOffset,
        VISA_opnd *rOffset,
        VISA_opnd *lod,
        VISA_opnd *dst,
        int line_no);

    bool CISA_create_scatter4_scaled_instruction(
        ISA_Opcode opcode,
        VISA_opnd *pred,
        VISA_EMask_Ctrl eMask,
        unsigned execSize,
        ChannelMask chMask,
        const char* surfaceName,
        VISA_opnd *globalOffset,
        VISA_opnd *offsets,
        VISA_opnd *dstSrc,
        int line_no);

    bool CISA_create_scatter_scaled_instruction(
        ISA_Opcode opcode,
        VISA_opnd *pred,
        VISA_EMask_Ctrl eMask,
        unsigned execSize,
        unsigned numBlocks,
        const char* surfaceName,
        VISA_opnd *globalOffset,
        VISA_opnd *offsets,
        VISA_opnd *dstSrc,
        int lineNo);

    bool CISA_create_sync_instruction(ISA_Opcode opcode);

    bool CISA_create_sbarrier_instruction(bool isSignal);

    bool CISA_create_invtri_inst(
        VISA_opnd *pred,
        ISA_Opcode opcode,
        bool  sat,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        VISA_opnd *dst,
        VISA_opnd *src0,
        int line_no);

    bool CISA_create_dword_atomic_instruction(
        VISA_opnd *pred,
        VISAAtomicOps subOpc,
        bool is16Bit,
        VISA_EMask_Ctrl eMask,
        unsigned execSize,
        const char *surfaceName,
        VISA_opnd *offsets,
        VISA_opnd *src0,
        VISA_opnd *src1,
        VISA_opnd *dst,
        int lineNo);

    bool CISA_create_typed_atomic_instruction(
        VISA_opnd *pred,
        VISAAtomicOps subOpc,
        bool is16Bit,
        VISA_EMask_Ctrl eMask,
        unsigned execSize,
        const char *surfaceName,
        VISA_opnd *u,
        VISA_opnd *v,
        VISA_opnd *r,
        VISA_opnd *lod,
        VISA_opnd *src0,
        VISA_opnd *src1,
        VISA_opnd *dst,
        int lineNo);

    bool CISA_create_urb_write_3d_instruction(
        VISA_opnd* pred,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        unsigned int num_out,
        unsigned int global_offset,
        VISA_opnd* channel_mask,
        VISA_opnd* urb_handle,
        VISA_opnd* per_slot_offset,
        VISA_opnd* vertex_data,
        int line_no);

    bool CISA_create_rtwrite_3d_instruction(
        VISA_opnd* pred,
        const char* mode,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        const char* surface_name,
        const std::vector<VISA_opnd*>& operands,
        int line_no);

    bool CISA_create_info_3d_instruction(
        VISASampler3DSubOpCode subOpcode,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        ChannelMask channel,
        const char* surface_name,
        VISA_opnd* lod,
        VISA_opnd* dst,
        int line_no);

    bool createSample4Instruction(
        VISA_opnd* pred,
        VISASampler3DSubOpCode subOpcode,
        bool pixelNullMask,
        ChannelMask channels,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        VISA_opnd* aoffimmi,
        const char* sampler_name,
        const char* surface_name,
        VISA_opnd* dst,
        unsigned int numParameters,
        VISA_RawOpnd** params,
        int line_no);

    bool create3DLoadInstruction(
        VISA_opnd* pred,
        VISASampler3DSubOpCode subOpcode,
        bool pixelNullMask,
        ChannelMask channels,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        VISA_opnd* aoffimmi,
        const char* surface_name,
        VISA_opnd* dst,
        unsigned int numParameters,
        VISA_RawOpnd** params,
        int line_no);

    bool create3DSampleInstruction(
        VISA_opnd* pred,
        VISASampler3DSubOpCode subOpcode,
        bool pixelNullMask,
        bool cpsEnable,
        bool uniformSampler,
        ChannelMask channels,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        VISA_opnd* aoffimmi,
        const char* sampler_name,
        const char* surface_name,
        VISA_opnd* dst,
        unsigned int numParameters,
        VISA_RawOpnd** params,
        int line_no);

    bool CISA_create_sample_instruction(
        ISA_Opcode opcode,
        ChannelMask channel,
        int simd_mode,
        const char* sampler_name,
        const char* surface_name,
        VISA_opnd *u_opnd,
        VISA_opnd *v_opnd,
        VISA_opnd *r_opnd,
        VISA_opnd *dst,
        int line_no);

    bool CISA_create_avs_instruction(
        ChannelMask channel,
        const char* surface_name,
        const char* sampler_name,
        VISA_opnd *u_offset,
        VISA_opnd *v_offset,
        VISA_opnd *deltaU,
        VISA_opnd *deltaV,
        VISA_opnd *u2d,
        VISA_opnd *groupID,
        VISA_opnd *verticalBlockNumber,
        OutputFormatControl cntrl,
        VISA_opnd *v2d,
        AVSExecMode execMode,
        VISA_opnd *iefbypass,
        VISA_opnd *dst,
        int line_no);

    bool CISA_create_sampleunorm_instruction(
        ISA_Opcode opcode,
        ChannelMask channel,
        CHANNEL_OUTPUT_FORMAT out,
        const char* sampler_dcl,
        const char* surface_dcl,
        VISA_opnd *src0,
        VISA_opnd *src1,
        VISA_opnd *src2,
        VISA_opnd *src3,
        VISA_opnd *dst,
        int line_no);

    bool CISA_create_vme_ime_instruction(
        ISA_Opcode opcode,
        unsigned char stream_mode,
        unsigned char searchCtrl,
        VISA_opnd *input_opnd,
        VISA_opnd *ime_input_opnd,
        const char* surface_name,
        VISA_opnd *ref0_opnd,
        VISA_opnd *ref1_opnd,
        VISA_opnd *costCenter_opnd,
        VISA_opnd *dst_opnd,
        int line_no);

    bool CISA_create_vme_sic_instruction(
        ISA_Opcode opcode,
        VISA_opnd *input_opnd,
        VISA_opnd *sic_input_opnd,
        const char* surface_name,
        VISA_opnd *dst,
        int line_no);

    bool CISA_create_vme_fbr_instruction(
        ISA_Opcode opcode,
        VISA_opnd *input_opnd,
        VISA_opnd *fbr_input_opnd,
        const char* surface_name,
        VISA_opnd* fbrMbMode,
        VISA_opnd* fbrSubMbShape,
        VISA_opnd* fbrSubPredMode,
        VISA_opnd *dst,
        int line_no);

    bool CISA_create_switch_instruction(
        ISA_Opcode opcode,
        unsigned exec_size,
        VISA_opnd *indexOpnd,
        int numLabels,
        char ** labels,
        int line_no);

    bool CISA_create_fcall_instruction(
        VISA_opnd *pred_opnd,
        ISA_Opcode opcode,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        const char* funcName,
        unsigned arg_size,
        unsigned return_size,
        int line_no);

    bool CISA_create_ifcall_instruction(
        VISA_opnd *pred_opnd,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        VISA_opnd* funcAddr,
        unsigned arg_size,
        unsigned return_size,
        int line_no);

    bool CISA_create_faddr_instruction(
        const char* sym_name, VISA_opnd* dst, int line_no);

    bool CISA_create_raw_send_instruction(
        ISA_Opcode opcode,
        unsigned char modifier,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        VISA_opnd *pred,
        unsigned int exMsgDesc,
        unsigned char srcSize,
        unsigned char dstSize,
        VISA_opnd *Desc,
        VISA_opnd *src,
        VISA_opnd *dst,
        int line_no);
    bool CISA_create_raw_sends_instruction(
        ISA_Opcode opcode,
        unsigned char modifier,
        bool hasEOT,
        VISA_EMask_Ctrl emask,
        unsigned exec_size,
        VISA_opnd *pred,
        VISA_opnd *exMsgDesc,
        unsigned char ffid,
        unsigned char src0Size,
        unsigned char src1Size,
        unsigned char dstSize,
        VISA_opnd *Desc,
        VISA_opnd *src0,
        VISA_opnd *src1,
        VISA_opnd *dst,
        int line_no);
    bool CISA_create_fence_instruction(ISA_Opcode opcode, unsigned char mode);
    bool CISA_create_wait_instruction(VISA_opnd* mask);
    bool CISA_create_yield_instruction(ISA_Opcode opcode);

    bool CISA_create_lifetime_inst(unsigned char startOrEnd,
                                   //VISA_opnd *src,
                                   const char* src,
                                   int line_no);

    bool CISA_create_FILE_instruction(ISA_Opcode opcode, const char * file_name);
    bool CISA_create_LOC_instruction(ISA_Opcode opcode, unsigned int loc);
    bool CISA_create_NO_OPND_instruction(ISA_Opcode opcode);

    void CISA_post_file_parse();

    VISA_opnd * CISA_create_gen_src_operand(
        const char* var_name, short v_stride, short width, short h_stride,
        unsigned char row_offset, unsigned char col_offset, VISA_Modifier mod, int line_no);
    VISA_opnd * CISA_dst_general_operand(
        const char * var_name, unsigned char roff, unsigned char sroff,
        unsigned short hstride,
        int line_no);
    VISA_opnd * CISA_create_immed(uint64_t value, VISA_Type type, int line_no);
    VISA_opnd * CISA_create_float_immed(double value, VISA_Type type, int line_no);
    CISA_GEN_VAR * CISA_find_decl(const char * var_name);
    VISA_opnd * CISA_set_address_operand(
        CISA_GEN_VAR * cisa_decl, unsigned char offset, short width, bool isDst);
    VISA_opnd * CISA_set_address_expression(CISA_GEN_VAR *cisa_decl, short offset);
    VISA_opnd * CISA_create_indirect(
        CISA_GEN_VAR * cisa_decl,VISA_Modifier mod, unsigned short row_offset,
        unsigned char col_offset, unsigned short immedOffset,
        unsigned short vertical_stride, unsigned short width,
        unsigned short horizontal_stride, VISA_Type type);
    VISA_opnd * CISA_create_indirect_dst(
        CISA_GEN_VAR * cisa_decl,VISA_Modifier mod, unsigned short row_offset,
        unsigned char col_offset, unsigned short immedOffset,
        unsigned short horizontal_stride, VISA_Type type);
    VISA_opnd * CISA_create_state_operand(
        const char * var_name, unsigned char offset, int line_no, bool isDst);
    VISA_opnd * CISA_create_predicate_operand(
        CISA_GEN_VAR * var, VISA_PREDICATE_STATE state,
        const char * pred_cntrl, int line_no);
    VISA_opnd * CISA_create_RAW_NULL_operand(int line_no);
    VISA_opnd * CISA_create_RAW_operand(const char * var_name, unsigned short offset, int line_no);

    void CISA_push_decl_scope();
    void CISA_pop_decl_scope();

    unsigned short get_hash_key(const char* str);
    string_pool_entry** new_string_pool();
    string_pool_entry * string_pool_lookup(string_pool_entry **spool, const char *str);
    bool string_pool_lookup_and_insert(
        string_pool_entry **spool, const char *str,
        Common_ISA_Var_Class type, VISA_Type data_type);

    // getKernels - get all kernels and functions added into this builder
    std::list<VISAKernelImpl*>& getKernels() { return m_kernelsAndFunctions; }

    void InitVisaWaTable(TARGET_PLATFORM platform, Stepping step);

    Options m_options;
    std::stringstream m_ssIsaAsm;
    std::stringstream m_ssIsaAsmHeader;

    void setGtpinInit(void* buf) { gtpin_init = buf; }
    void* getGtpinInit() { return gtpin_init; }



private:

    vISA::Mem_Manager m_mem;
    VISA_BUILDER_OPTION mBuildOption;

    unsigned int m_kernel_count;
    unsigned int m_function_count;

    // list of kernels and functions added to this builder
    std::list<VISAKernelImpl *> m_kernelsAndFunctions;
    // for cases of several kernels/functions in one CisaBuilder
    // we need to keep a mapping of kernels to names
    // to make GetVISAKernel() work
    std::map<std::string, VISAKernelImpl *> m_nameToKernel;

    void emitFCPatchFile();

    PWA_TABLE m_pWaTable;
    bool needsToFreeWATable = false;

    void* gtpin_init = nullptr;

    // important messages that we should relay to the user
    // (things like if RA is spilling, etc.)
    std::stringstream criticalMsg;
};

#endif
