/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _BUILDCISAIR_H_
#define _BUILDCISAIR_H_

#include "Common_ISA.h"
#include "Common_ISA_framework.h"

// clang-format off
#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/SmallVector.h"
#include "common/LLVMWarningsPop.hpp"
// clang-format on

#include <cstdint>
#include <sstream>

namespace vISA {
class Mem_Manager;
class PlatformInfo;
} // namespace vISA
class VISAKernelImpl;
class VISAFunction;

#define YY_DECL int yylex(CISA_IR_Builder *pBuilder)

extern FILE *CISAin;
extern FILE *CISAout;
extern int CISAdebug;

#include "PlatformInfo.h"
#include "VISABuilderAPIDefinition.h"
#include "inc/common/sku_wa.h"

class Options;

class CISA_IR_Builder : public VISABuilder {
public:
  CISA_IR_Builder(TARGET_PLATFORM platform, VISA_BUILDER_OPTION buildOption,
                  vISABuilderMode mode, int majorVersion, int minorVersion,
                  const WA_TABLE *pWaTable)
      : mBuildOption(buildOption), m_builderMode(mode), m_pWaTable(pWaTable) {
    m_platformInfo = vISA::PlatformInfo::LookupPlatformInfo(platform);
    vISA_ASSERT(m_platformInfo != nullptr, "null m_platformInfo");
    m_header.major_version = majorVersion;
    m_header.minor_version = minorVersion;
    m_header.magic_number = COMMON_ISA_MAGIC_NUM;
  }

  virtual ~CISA_IR_Builder();
  CISA_IR_Builder(const CISA_IR_Builder&) = delete;
  CISA_IR_Builder& operator=(const CISA_IR_Builder&) = delete;

  /**************START VISA BUILDER API*****************************/

  static int CreateBuilder(CISA_IR_Builder *&builder, vISABuilderMode mode,
                           VISA_BUILDER_OPTION buildOption,
                           TARGET_PLATFORM platform, int numArgs,
                           const char *flags[],
                           const WA_TABLE *pWaTable = nullptr);
  static int DestroyBuilder(CISA_IR_Builder *builder);
  VISA_BUILDER_API int AddKernel(VISAKernel *&kernel,
                                 const char *kernelName) override;
  VISA_BUILDER_API int SetPrevKernel(VISAKernel *&prevKernel) override;
  VISA_BUILDER_API int AddFunction(VISAFunction *&function,
                                   const char *functionName) override;
  VISA_BUILDER_API int AddPayloadSection(VISAFunction *&function,
                                         const char *functionName) override;
  VISA_BUILDER_API int Compile(const char *isaasmFileName,
                               bool emit_visa_only = false) override;

  VISA_BUILDER_API int GetuInt32Option(vISAOptions option) override {
      return m_options.getuInt32Option(option);
  }
  VISA_BUILDER_API bool GetOption(vISAOptions option) override {
      return m_options.getOption(option);
  }
  VISA_BUILDER_API void SetOption(vISAOptions option, bool val) override {
    m_options.setOption(option, val);
  }
  VISA_BUILDER_API void SetOption(vISAOptions option, uint32_t val) override {
    m_options.setOption(option, val);
  }
  VISA_BUILDER_API void SetOption(vISAOptions option,
                                  const char *val) override {
    const char* strval = ((val && val[0] != 0) ? val : nullptr);
    m_options.setOption(option, strval);
  }

  // Used for inline asm code generation
  VISA_BUILDER_API int ParseVISAText(const std::string &visaText,
                                     const std::string &visaTextFile) override;
  VISA_BUILDER_API int ParseVISAText(const std::string &visaFile) override;
  VISA_BUILDER_API std::stringstream &GetAsmTextStream() override {
    return m_ssIsaAsm;
  }
  VISA_BUILDER_API VISAKernel *
  GetVISAKernel(const std::string &kernelName) const override;
  VISA_BUILDER_API int ClearAsmTextStreams() override;

  // Pass a set of functions that should be called directly (vs. indirect
  // calls). Used in ESIMD+SPMD interop scenarios.
  VISA_BUILDER_API void SetDirectCallFunctionSet(
      const std::unordered_set<std::string> &directCallFunctions) override;

  /**************END VISA BUILDER API*************************/

  common_isa_header m_header{};

  // the current vISA kernel/function being processed
  VISAKernelImpl* m_kernel = nullptr;
  VISAKernelImpl *m_prevKernel = nullptr;
  VISAKernelImpl *get_kernel() const { return m_kernel; }

  std::stringstream &criticalMsgStream() { return criticalMsg; }

  std::string GetCriticalMsg() override { return criticalMsg.str(); }

  bool debugParse() const { return m_options.getOption(vISA_DebugParse); }

  int verifyVISAIR();

  int isaDump(const char *combinedIsaasmName) const;

  std::string isaDump(const VISAKernelImpl *kernel,
                      const VISAKernelImpl *mainKernel,
                      bool printVersion = true,
                      bool addDeclCommentAtEnd = false) const;

  static void cat(std::stringstream &ss) {}
  template <typename T, typename... Ts>
  static void cat(std::stringstream &ss, T t, Ts... ts) {
    ss << t;
    cat(ss, ts...);
  }

  std::string m_errorMessage;
  template <typename... Ts> void RecordParseError(int lineNum, Ts... ts) {
    if (HasParseError()) // report first only
      return;

    std::stringstream ss;
    if (lineNum > 0)
      ss << "near line " << lineNum << ": ";
    else
      ss << "unknown location: ";

    cat(ss, ts...);
    m_errorMessage = ss.str();
    criticalMsg << m_errorMessage << "\n";
  }
  bool HasParseError() const { return !m_errorMessage.empty(); }
  std::string GetParseError() const { return m_errorMessage; }

  template <typename... Ts> void RecordParseWarning(int lineNum, Ts... ts) {
    std::stringstream ss;
    ss << "near line " << lineNum << ": ";
    cat(ss, ts...);
    m_warnings.push_back(ss.str());
  }

  std::vector<std::string> m_warnings;
  const std::vector<std::string> &GetWarnings() const { return m_warnings; }

  /////////////////////////////////////////////////////
  // holds the %DispatchSimdSize attribute
  int m_dispatchSimdSize = -1;

  const WA_TABLE *getWATable() const { return m_pWaTable; }

  uint8_t getMajorVersion() const { return m_header.major_version; }
  uint8_t getMinorVersion() const { return m_header.minor_version; }

  void CISA_IR_setVersion(unsigned char major_ver, unsigned char minor_ver) {
    m_header.major_version = major_ver;
    m_header.minor_version = minor_ver;
  }

  Common_ISA_Input_Class get_input_class(Common_ISA_Var_Class var_class);

  bool CISA_lookup_builtin_constant(int lineNum, const char *symbol,
                                    int64_t &val);
  bool CISA_eval_sizeof_decl(int lineNum, const char *arg, int64_t &val);

  VISA_StateOpndHandle *CISA_get_surface_variable(const char *varName,
                                                  int lineNum);
  VISA_StateOpndHandle *CISA_get_sampler_variable(const char *varName,
                                                  int lineNum);

  bool CISA_general_variable_decl(
      const char *var_name, unsigned int var_elemts_num, VISA_Type data_type,
      VISA_Align var_align, const char *var_alias_name, int var_alias_offset,
      std::vector<attr_gen_struct *> &scope, int lineNum);

  bool CISA_addr_variable_decl(const char *var_name, unsigned int var_elements,
                               VISA_Type data_type,
                               std::vector<attr_gen_struct *> &scope,
                               int lineNum);

  bool CISA_predicate_variable_decl(const char *var_name,
                                    unsigned int var_elements,
                                    std::vector<attr_gen_struct *> &attrs,
                                    int lineNum);

  bool CISA_sampler_variable_decl(const char *var_name, int num_elts,
                                  const char *name, int lineNum);

  bool CISA_surface_variable_decl(const char *var_name, int num_elts,
                                  const char *name,
                                  std::vector<attr_gen_struct *> &attrs,
                                  int lineNum);

  bool CISA_input_directive(const char *var_name, short offset,
                            unsigned short size, int lineNum);

  bool CISA_implicit_input_directive(const char *argName, const char *varName,
                                     short offset, unsigned short size,
                                     int lineNum);

  // bool CISA_attr_directive(char* input_name, attribute_info_t* attr);
  bool CISA_attr_directive(const char *input_name, const char *input_var,
                           int lineNum);
  bool CISA_attr_directiveNum(const char *input_name, uint32_t input_var,
                              int lineNum);

  bool CISA_create_label(const char *label_name, int lineNum);
  bool CISA_function_directive(const char *func_name, int lineNum);

  bool CISA_create_arith_instruction(VISA_opnd *cisa_pred, ISA_Opcode opcode,
                                     bool sat, VISA_EMask_Ctrl emask,
                                     unsigned exec_size, VISA_opnd *dst_cisa,
                                     VISA_opnd *src0_cisa, VISA_opnd *src1_cisa,
                                     VISA_opnd *src2_cisa, int lineNum);
  bool CISA_create_arith_instruction2(VISA_opnd *cisa_pred, ISA_Opcode opcode,
                                      VISA_EMask_Ctrl emask, unsigned exec_size,
                                      VISA_opnd *dst_cisa, VISA_opnd *src0_cisa,
                                      VISA_opnd *src1_cisa,
                                      VISA_opnd *src2_cisa, int lineNum);
  bool CISA_create_arith_instruction2_predDst(
      VISA_opnd *cisa_pred, ISA_Opcode opcode, VISA_EMask_Ctrl emask,
      unsigned exec_size, VISA_opnd *dst, CISA_GEN_VAR *dst_pred,
      VISA_opnd *src0, VISA_opnd *src1, int lineNum);

  bool CISA_create_breakpoint_instruction(int lineNum);

  bool CISA_create_mov_instruction(VISA_opnd *pred, ISA_Opcode opcode,
                                   VISA_EMask_Ctrl emask, unsigned exec_size,
                                   bool sat, VISA_opnd *dst, VISA_opnd *src0,
                                   int lineNum);

  bool CISA_create_mov_instruction(VISA_opnd *dst, CISA_GEN_VAR *src0,
                                   int lineNum);

  bool CISA_create_movs_instruction(VISA_EMask_Ctrl emask, ISA_Opcode opcode,
                                    unsigned exec_size, VISA_opnd *dst,
                                    VISA_opnd *src0, int lineNum);

  bool CISA_create_branch_instruction(VISA_opnd *pred, ISA_Opcode opcode,
                                      VISA_EMask_Ctrl emask, unsigned exec_size,
                                      const char *target_label, bool is_fccall,
                                      int lineNum);

  bool CISA_create_cmp_instruction(VISA_Cond_Mod sub_op, VISA_EMask_Ctrl emask,
                                   unsigned exec_size, CISA_GEN_VAR *decl,
                                   VISA_opnd *src0, VISA_opnd *src1,
                                   int lineNum);

  bool CISA_create_cmp_instruction(VISA_Cond_Mod sub_op, ISA_Opcode opcode,
                                   VISA_EMask_Ctrl emask, unsigned exec_size,
                                   VISA_opnd *dst, VISA_opnd *src0,
                                   VISA_opnd *src1, int lineNum);

  bool CISA_create_media_instruction(ISA_Opcode opcode, MEDIA_LD_mod media_mod,
                                     int row_off, int elem_off,
                                     unsigned int plane_ID,
                                     const char *surface_name, VISA_opnd *src0,
                                     VISA_opnd *src1, VISA_opnd *raw_dst,
                                     int lineNum);

  bool CISA_Create_Ret(VISA_opnd *pred_opnd, ISA_Opcode opcode,
                       VISA_EMask_Ctrl emask, unsigned int exec_size,
                       int lineNum);

  bool CISA_create_oword_instruction(ISA_Opcode opcode, bool media_mod,
                                     unsigned int size,
                                     const char *surface_name, VISA_opnd *src0,
                                     VISA_opnd *raw_dst_src, int lineNum);

  bool CISA_create_svm_block_instruction(SVMSubOpcode subopcode,
                                         unsigned owords, bool unaligned,
                                         VISA_opnd *address, VISA_opnd *srcDst,
                                         int line_no);

  bool CISA_create_svm_scatter_instruction(
      VISA_opnd *pred, SVMSubOpcode subopcode, VISA_EMask_Ctrl emask,
      unsigned exec_size, unsigned blockSize, unsigned numBlocks,
      VISA_opnd *addresses, VISA_opnd *srcDst, int line_no);

  bool CISA_create_svm_atomic_instruction(
      VISA_opnd *pred, VISA_EMask_Ctrl emask, unsigned exec_size,
      VISAAtomicOps op, unsigned short bitwidth, VISA_opnd *addresses,
      VISA_opnd *src0, VISA_opnd *src1, VISA_opnd *dst, int lineNum);

  bool CISA_create_svm_gather4_scaled(VISA_opnd *pred, VISA_EMask_Ctrl eMask,
                                      unsigned execSize, ChannelMask chMask,
                                      VISA_opnd *address, VISA_opnd *offsets,
                                      VISA_opnd *src, int lineNum);

  bool CISA_create_svm_scatter4_scaled(VISA_opnd *pred, VISA_EMask_Ctrl eMask,
                                       unsigned execSize, ChannelMask chMask,
                                       VISA_opnd *address, VISA_opnd *offsets,
                                       VISA_opnd *src, int lineNum);

  bool CISA_create_address_instruction(ISA_Opcode opcode, VISA_EMask_Ctrl emask,
                                       unsigned exec_size, VISA_opnd *dst,
                                       VISA_opnd *src0, VISA_opnd *src1,
                                       int lineNum);

  bool CISA_create_logic_instruction(VISA_opnd *pred, ISA_Opcode opcode,
                                     bool sat, VISA_EMask_Ctrl emask,
                                     unsigned exec_size, VISA_opnd *dst,
                                     VISA_opnd *src0, VISA_opnd *src1,
                                     VISA_opnd *src2, VISA_opnd *src3,
                                     int lineNum);

  bool CISA_create_logic_instruction(ISA_Opcode opcode, VISA_EMask_Ctrl emask,
                                     unsigned exec_size, CISA_GEN_VAR *dst,
                                     CISA_GEN_VAR *src0, CISA_GEN_VAR *src1,
                                     int lineNum);

  bool CISA_create_math_instruction(VISA_opnd *pred, ISA_Opcode opcode,
                                    bool sat, VISA_EMask_Ctrl emask,
                                    unsigned exec_size, VISA_opnd *dst,
                                    VISA_opnd *src0, VISA_opnd *src1,
                                    int lineNum);

  bool CISA_create_setp_instruction(ISA_Opcode opcode, VISA_EMask_Ctrl emask,
                                    unsigned exec_size, CISA_GEN_VAR *dst,
                                    VISA_opnd *src0, int lineNum);

  bool CISA_create_sel_instruction(ISA_Opcode opcode, bool sat, VISA_opnd *pred,
                                   VISA_EMask_Ctrl emask, unsigned exec_size,
                                   VISA_opnd *dst, VISA_opnd *src0,
                                   VISA_opnd *src1, int lineNum);

  bool CISA_create_fminmax_instruction(bool minmax, ISA_Opcode opcode, bool sat,
                                       VISA_opnd *pred, VISA_EMask_Ctrl emask,
                                       unsigned exec_size, VISA_opnd *dst,
                                       VISA_opnd *src0, VISA_opnd *src1,
                                       int lineNum);

  bool
  CISA_create_scatter_instruction(ISA_Opcode opcode, int elemNum,
                                  VISA_EMask_Ctrl emask, unsigned elt_size,
                                  bool modifier, const char *surface_name,
                                  VISA_opnd *global_offset,  // global_offset
                                  VISA_opnd *element_offset, // element_offset
                                  VISA_opnd *raw_dst_src,    // dst/src
                                  int lineNum);

  bool CISA_create_scatter4_typed_instruction(
      ISA_Opcode opcode, VISA_opnd *pred, ChannelMask ch_mask,
      VISA_EMask_Ctrl emask, unsigned execSize, const char *surfaceName,
      VISA_opnd *uOffset, VISA_opnd *vOffset, VISA_opnd *rOffset,
      VISA_opnd *lod, VISA_opnd *dst, int lineNum);

  bool CISA_create_scatter4_scaled_instruction(
      ISA_Opcode opcode, VISA_opnd *pred, VISA_EMask_Ctrl eMask,
      unsigned execSize, ChannelMask chMask, const char *surfaceName,
      VISA_opnd *globalOffset, VISA_opnd *offsets, VISA_opnd *dstSrc,
      int lineNum);

  bool CISA_create_scatter_scaled_instruction(
      ISA_Opcode opcode, VISA_opnd *pred, VISA_EMask_Ctrl eMask,
      unsigned execSize, unsigned numBlocks, const char *surfaceName,
      VISA_opnd *globalOffset, VISA_opnd *offsets, VISA_opnd *dstSrc,
      int lineNum);

  bool CISA_create_sync_instruction(ISA_Opcode opcode, int lineNum);

  bool CISA_create_sbarrier_instruction(bool isSignal, int lineNum);

  bool CISA_create_invtri_inst(VISA_opnd *pred, ISA_Opcode opcode, bool sat,
                               VISA_EMask_Ctrl emask, unsigned exec_size,
                               VISA_opnd *dst, VISA_opnd *src0, int lineNum);

  bool CISA_create_dword_atomic_instruction(
      VISA_opnd *pred, VISAAtomicOps subOpc, bool is16Bit,
      VISA_EMask_Ctrl eMask, unsigned execSize, const char *surfaceName,
      VISA_opnd *offsets, VISA_opnd *src0, VISA_opnd *src1, VISA_opnd *dst,
      int lineNum);

  bool CISA_create_typed_atomic_instruction(
      VISA_opnd *pred, VISAAtomicOps subOpc, bool is16Bit,
      VISA_EMask_Ctrl eMask, unsigned execSize, const char *surfaceName,
      VISA_opnd *u, VISA_opnd *v, VISA_opnd *r, VISA_opnd *lod, VISA_opnd *src0,
      VISA_opnd *src1, VISA_opnd *dst, int lineNum);

  bool CISA_create_urb_write_3d_instruction(
      VISA_opnd *pred, VISA_EMask_Ctrl emask, unsigned exec_size,
      unsigned int num_out, unsigned int global_offset, VISA_opnd *channel_mask,
      VISA_opnd *urb_handle, VISA_opnd *per_slot_offset, VISA_opnd *vertex_data,
      int lineNum);

  bool CISA_create_rtwrite_3d_instruction(
      VISA_opnd *pred, const char *mode, VISA_EMask_Ctrl emask,
      unsigned exec_size, const char *surface_name,
      const std::vector<VISA_opnd *> &operands, int lineNum);

  bool CISA_create_info_3d_instruction(VISASampler3DSubOpCode subOpcode,
                                       VISA_EMask_Ctrl emask,
                                       unsigned exec_size, ChannelMask channel,
                                       const char *surface_name, unsigned surfaceIndex,
                                       VISA_opnd *lod, VISA_opnd *dst, int lineNum);

  bool createSample4Instruction(
      VISA_opnd *pred, VISASampler3DSubOpCode subOpcode, bool pixelNullMask,
      ChannelMask channels, VISA_EMask_Ctrl emask, unsigned exec_size,
      VISA_opnd *aoffimmi, const char *sampler_name, unsigned int samplerIndex,
      const char *surface_name, unsigned int surfaceIndex,
      VISA_opnd *pairedSurface, VISA_opnd *dst, unsigned int numParameters,
      VISA_RawOpnd **params, int lineNum);

  bool create3DLoadInstruction(VISA_opnd *pred,
                               VISASampler3DSubOpCode subOpcode,
                               bool pixelNullMask, ChannelMask channels,
                               VISA_EMask_Ctrl emask, unsigned exec_size,
                               VISA_opnd *aoffimmi, const char *surface_name,
                               unsigned int surfaceIndex,
                               VISA_opnd *pairedSurface,
                               VISA_opnd *dst, unsigned int numParameters,
                               VISA_RawOpnd **params, int lineNum);

  bool create3DSampleInstruction(
      VISA_opnd *pred, VISASampler3DSubOpCode subOpcode, bool pixelNullMask,
      bool cpsEnable, bool uniformSampler,
      ChannelMask channels, VISA_EMask_Ctrl emask, unsigned exec_size,
      VISA_opnd *aoffimmi, const char *sampler_name, unsigned int samplerIdx,
      const char *surface_name, unsigned int surfaceIdx,
      VISA_opnd *pairedSurface, VISA_opnd *dst, unsigned int numParameters,
      VISA_RawOpnd **params, int lineNum);

  bool CISA_create_sample_instruction(ISA_Opcode opcode, ChannelMask channel,
                                      int simd_mode, const char *sampler_name,
                                      const char *surface_name,
                                      VISA_opnd *u_opnd, VISA_opnd *v_opnd,
                                      VISA_opnd *r_opnd, VISA_opnd *dst,
                                      int lineNum);

  bool CISA_create_avs_instruction(
      ChannelMask channel, const char *surface_name, const char *sampler_name,
      VISA_opnd *u_offset, VISA_opnd *v_offset, VISA_opnd *deltaU,
      VISA_opnd *deltaV, VISA_opnd *u2d, VISA_opnd *groupID,
      VISA_opnd *verticalBlockNumber, OutputFormatControl cntrl, VISA_opnd *v2d,
      AVSExecMode execMode, VISA_opnd *iefbypass, VISA_opnd *dst, int lineNum);

  bool CISA_create_sampleunorm_instruction(
      ISA_Opcode opcode, ChannelMask channel, CHANNEL_OUTPUT_FORMAT out,
      const char *sampler_dcl, const char *surface_dcl, VISA_opnd *src0,
      VISA_opnd *src1, VISA_opnd *src2, VISA_opnd *src3, VISA_opnd *dst,
      int lineNum);

  bool CISA_create_vme_ime_instruction(
      ISA_Opcode opcode, unsigned char stream_mode, unsigned char searchCtrl,
      VISA_opnd *input_opnd, VISA_opnd *ime_input_opnd,
      const char *surface_name, VISA_opnd *ref0_opnd, VISA_opnd *ref1_opnd,
      VISA_opnd *costCenter_opnd, VISA_opnd *dst_opnd, int lineNum);

  bool CISA_create_vme_sic_instruction(ISA_Opcode opcode, VISA_opnd *input_opnd,
                                       VISA_opnd *sic_input_opnd,
                                       const char *surface_name, VISA_opnd *dst,
                                       int lineNum);

  bool CISA_create_vme_fbr_instruction(
      ISA_Opcode opcode, VISA_opnd *input_opnd, VISA_opnd *fbr_input_opnd,
      const char *surface_name, VISA_opnd *fbrMbMode, VISA_opnd *fbrSubMbShape,
      VISA_opnd *fbrSubPredMode, VISA_opnd *dst, int lineNum);

  bool CISA_create_switch_instruction(ISA_Opcode opcode, unsigned exec_size,
                                      VISA_opnd *indexOpnd,
                                      const std::deque<const char *> &labels,
                                      int lineNum);

  bool CISA_create_fcall_instruction(VISA_opnd *pred_opnd, ISA_Opcode opcode,
                                     VISA_EMask_Ctrl emask, unsigned exec_size,
                                     const char *funcName, unsigned arg_size,
                                     unsigned return_size, int lineNum);

  bool CISA_create_ifcall_instruction(VISA_opnd *pred_opnd,
                                      VISA_EMask_Ctrl emask, unsigned exec_size,
                                      bool isUniform, VISA_opnd *funcAddr,
                                      unsigned arg_size, unsigned return_size,
                                      int lineNum);

  bool CISA_create_faddr_instruction(const char *sym_name, VISA_opnd *dst,
                                     int lineNum);

  bool CISA_create_raw_send_instruction(
      ISA_Opcode opcode, unsigned char modifier, VISA_EMask_Ctrl emask,
      unsigned exec_size, VISA_opnd *pred, unsigned int exMsgDesc,
      unsigned char srcSize, unsigned char dstSize, VISA_opnd *Desc,
      VISA_opnd *src, VISA_opnd *dst, int lineNum);
  bool CISA_create_raw_sends_instruction(
      ISA_Opcode opcode, unsigned char modifier, bool hasEOT,
      VISA_EMask_Ctrl emask, unsigned exec_size, VISA_opnd *pred,
      VISA_opnd *exMsgDesc, unsigned char ffid, unsigned char src0Size,
      unsigned char src1Size, unsigned char dstSize, VISA_opnd *Desc,
      VISA_opnd *src0, VISA_opnd *src1, VISA_opnd *dst, int lineNum);
  bool CISA_create_raw_sendg_instruction(
      unsigned sfid, VISA_opnd *pred, VISA_EMask_Ctrl emask,
      VISA_Exec_Size esize, VISA_opnd *dst, int dstLenBytes, VISA_opnd *src0,
      int src0LenBytes, VISA_opnd *src1, int src1LenBytes, VISA_opnd *ind0,
      VISA_opnd *ind1, uint64_t desc,
      bool isConditional, // raw_sendg vs raw_sendgc
      bool hasEOT, int lineNum);

  bool CISA_create_fence_instruction(ISA_Opcode opcode, unsigned char mode,
                                     int lineNum);
  bool CISA_create_wait_instruction(VISA_opnd *mask, int lineNum);
  bool CISA_create_yield_instruction(ISA_Opcode opcode, int lineNum);

  bool CISA_create_lifetime_inst(unsigned char startOrEnd, const char *src,
                                 int lineNum);

  bool CISA_create_FILE_instruction(ISA_Opcode opcode, const char *file_name,
                                    int lineNum);
  bool CISA_create_LOC_instruction(ISA_Opcode opcode, unsigned int loc,
                                   int lineNum);
  bool CISA_create_NO_OPND_instruction(ISA_Opcode opcode, int lineNum);

  void CISA_post_file_parse();

  void CISA_parse_build_options(const char* argStr);

  VISA_opnd *CISA_create_gen_src_operand(const char *var_name, short v_stride,
                                         short width, short h_stride,
                                         unsigned char row_offset,
                                         unsigned char col_offset,
                                         VISA_Modifier mod, int lineNum);
  VISA_opnd *CISA_dst_general_operand(const char *var_name, unsigned char roff,
                                      unsigned char sroff,
                                      unsigned short hstride, int lineNum);
  attr_gen_struct *CISA_Create_Attr(const char *AttrName, int64_t I64Val,
                                    const char *CStrVal);
  VISA_opnd *CISA_create_immed(uint64_t value, VISA_Type type, int lineNum);
  VISA_opnd *CISA_create_float_immed(double value, VISA_Type type, int lineNum);
  CISA_GEN_VAR *CISA_find_decl(const char *var_name);
  VISA_opnd *CISA_set_address_operand(CISA_GEN_VAR *cisa_decl,
                                      unsigned char offset, short width,
                                      bool isDst, int lineNum);
  VISA_opnd *CISA_set_address_expression(CISA_GEN_VAR *cisa_decl, short offset,
                                         int lineNum);
  VISA_opnd *CISA_create_indirect(
      CISA_GEN_VAR *cisa_decl, VISA_Modifier mod, unsigned short row_offset,
      unsigned char col_offset, unsigned short immedOffset,
      unsigned short vertical_stride, unsigned short width,
      unsigned short horizontal_stride, VISA_Type type, int lineNum);
  VISA_opnd *CISA_create_indirect_dst(
      CISA_GEN_VAR *cisa_decl, VISA_Modifier mod, unsigned short row_offset,
      unsigned char col_offset, unsigned short immedOffset,
      unsigned short horizontal_stride, VISA_Type type, int lineNum);
  VISA_opnd *CISA_create_state_operand(const char *var_name,
                                       unsigned char offset, int lineNum,
                                       bool isDst);
  VISA_opnd *CISA_create_predicate_operand(CISA_GEN_VAR *var,
                                           VISA_PREDICATE_STATE state,
                                           VISA_PREDICATE_CONTROL pred_cntrl,
                                           int lineNum);
  VISA_opnd *CISA_create_RAW_NULL_operand(int lineNum);
  VISA_opnd *CISA_create_RAW_operand(const char *var_name,
                                     unsigned short offset, int lineNum);

  bool addAllVarAttributes(CISA_GEN_VAR *GenVar,
                           std::vector<attr_gen_struct *> &Attrs, int linueNum);

  void CISA_push_decl_scope();
  void CISA_pop_decl_scope();

  typedef llvm::SmallVector<VISAKernelImpl *, 8> KernelListTy;
  KernelListTy::iterator kernel_begin() {
    return m_kernelsAndFunctions.begin();
  }
  KernelListTy::iterator kernel_end() {
      return m_kernelsAndFunctions.end();
  }
  KernelListTy::const_iterator kernel_begin() const {
    return m_kernelsAndFunctions.begin();
  }
  KernelListTy::const_iterator kernel_end() const {
    return m_kernelsAndFunctions.end();
  }

  const VISAKernelImpl *getKernel(const std::string &name) const;
  VISAKernelImpl *getKernel(const std::string &name);

  Options m_options;
  std::stringstream m_ssIsaAsm;

  void setGtpinInit(void *buf) { gtpin_init = buf; }
  void *getGtpinInit() { return gtpin_init; }

  const vISA::PlatformInfo *getPlatformInfo() const { return m_platformInfo; }
  TARGET_PLATFORM getPlatform() const { return m_platformInfo->platform; }
  Options *getOptions() { return &m_options; }
  VISA_BUILDER_OPTION getBuilderOption() const { return mBuildOption; }
  vISABuilderMode getBuilderMode() const { return m_builderMode; }

  LSC_CACHE_OPTS CISA_create_caching_opts(int lineNum);
  LSC_CACHE_OPTS CISA_create_caching_opts(LSC_CACHE_OPT l1, LSC_CACHE_OPT l3, int lineNum);
  LSC_CACHE_OPTS CISA_create_caching_opts(LSC_CACHE_OPT l1, LSC_CACHE_OPT l2,
                                          LSC_CACHE_OPT l3, int lineNum);
  bool CISA_create_dpas_instruction(ISA_Opcode opcode, VISA_EMask_Ctrl emask,
                                    unsigned exec_size, VISA_opnd *dst_cisa,
                                    VISA_opnd *src0_cisa, VISA_opnd *src1_cisa,
                                    VISA_opnd *src2_cisa, GenPrecision A,
                                    GenPrecision W, uint8_t D, uint8_t C,
                                    int lineNum);
  bool CISA_create_bdpas_instruction(ISA_Opcode opcode, VISA_EMask_Ctrl emask,
                                     unsigned exec_size, VISA_opnd *dst_cisa,
                                     VISA_opnd *src0_cisa, VISA_opnd *src1_cisa,
                                     VISA_opnd *src2_cisa, VISA_opnd *src3_cisa,
                                     VISA_opnd *src4_cisa, GenPrecision A,
                                     GenPrecision W, uint8_t D, uint8_t C,
                                     int lineNum);

  bool CISA_create_bfn_instruction(VISA_opnd *pred, uint8_t func_ctrl, bool sat,
                                   VISA_EMask_Ctrl emask, unsigned exec_size,
                                   VISA_opnd *dst_cisa, VISA_opnd *src0_cisa,
                                   VISA_opnd *src1_cisa, VISA_opnd *src2_cisa,
                                   int lineNum);

  bool CISA_create_qword_scatter_instruction(
      ISA_Opcode opcode, VISA_opnd *pred, VISA_EMask_Ctrl eMask,
      unsigned execSize, unsigned numBlocks, const char *surfaceName,
      VISA_opnd *offsets, VISA_opnd *dstSrc, int lineNum);

  bool CISA_create_lsc_extended_cache_ctrl_inst(
      VISA_opnd *pred, LSC_OP opcode, LSC_SFID sfid, LSC_CACHE_CTRL_OPERATION ccop,
      LSC_CACHE_CTRL_SIZE ccsize, LSC_CACHE_OPTS caching, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_ADDR addr, VISA_opnd *src0Addr, int linenum);
  bool CISA_create_lsc_untyped_inst(
      VISA_opnd *pred, LSC_OP opcode, LSC_SFID sfid, LSC_CACHE_OPTS caching, bool ov,
      VISA_Exec_Size execSize, VISA_EMask_Ctrl emask, LSC_ADDR addr,
      LSC_DATA_SHAPE dataShape,
      VISA_opnd *surface, unsigned surfaceIndex,
      VISA_opnd *dst,
      VISA_opnd *src0,
      VISA_opnd *src1,
      VISA_opnd *src2,
      int lineNum);
  bool CISA_create_lsc_untyped_strided_inst(
      VISA_opnd *pred, LSC_OP opcode, LSC_SFID sfid, LSC_CACHE_OPTS caching,
      VISA_Exec_Size execSize, VISA_EMask_Ctrl emask, LSC_ADDR addr,
      LSC_DATA_SHAPE dataShape,
      VISA_opnd *surface, unsigned surfaceIndex,
      VISA_opnd *dstData,
      VISA_opnd *src0AddrBase, VISA_opnd *src0AddrPitch, VISA_opnd *src1Data,
      int lineNum);
  bool CISA_create_lsc_untyped_block2d_inst(
      VISA_opnd *pred, LSC_OP opcode, LSC_SFID sfid, LSC_CACHE_OPTS caching,
      VISA_Exec_Size execSize, VISA_EMask_Ctrl emask,
      LSC_DATA_SHAPE_BLOCK2D dataShape, VISA_opnd *dstData,
      VISA_opnd
          *src0Addrs[LSC_BLOCK2D_ADDR_PARAMS], // {base,surfW,surfH,surfP,x,y}
      VISA_opnd *src1Data, int xOffset, int yOffset, int lineNum);

  bool CISA_create_lsc_typed_inst(
      VISA_opnd *pred, LSC_OP opcode, LSC_SFID sfid, LSC_CACHE_OPTS caching,
      VISA_Exec_Size execSize, VISA_EMask_Ctrl emask, LSC_ADDR_TYPE addrModel,
      LSC_ADDR_SIZE addrSize, LSC_DATA_SHAPE dataShape,
      VISA_opnd *surface, unsigned surfaceIndex,
      VISA_opnd *dst_data,
      VISA_opnd *src0_Us, int uOffset,
      VISA_opnd *src0_Vs, int vOffset,
      VISA_opnd *src0_Rs, int rOffset,
      VISA_opnd *src0_LODs,
      VISA_opnd *src1_data, VISA_opnd *src2_data,
      int lineNum);
  bool CISA_create_lsc_fence(LSC_SFID lscSfid, LSC_FENCE_OP fence,
                             LSC_SCOPE scope, int lineNum);

  bool CISA_create_fcvt_instruction(bool sat, VISA_EMask_Ctrl emask,
                                    unsigned exec_size, VISA_opnd *dst,
                                    VISA_opnd *src0, int lineNum);

  bool CISA_create_nbarrier(bool isWait, VISA_opnd *barrierId,
                            VISA_opnd *threadCount, int lineNum);
  bool CISA_create_nbarrier_signal(VISA_opnd *barrierId, VISA_opnd *barrierType,
                                   VISA_opnd *numProds, VISA_opnd *numCons,
                                   int lineNum);


  bool CISA_create_lsc_typed_block2d_inst(
      LSC_OP opcode, LSC_CACHE_OPTS caching, LSC_ADDR_TYPE addrModel,
      LSC_DATA_SHAPE_TYPED_BLOCK2D dataShape,
      VISA_opnd *surface, unsigned surfaceIndex,
      VISA_opnd *dstData, VISA_opnd *xOffset, VISA_opnd *yOffset,
      int xImmOffset, int yImmOffset, VISA_opnd *src1Data, int lineNum);

  bool CISA_create_lsc_untyped_append_counter_atomic_inst(
      LSC_OP opcode, VISA_opnd *pred, VISA_Exec_Size execSize,
      VISA_EMask_Ctrl emask, LSC_CACHE_OPTS caching,
      LSC_ADDR_TYPE addr, LSC_DATA_SHAPE dataShape,
      VISA_opnd *surface, unsigned surfaceIndex,
      VISA_opnd *dst, VISA_opnd *srcData, int lineNum);
  bool CISA_create_shfl_idx4_instruction(VISA_opnd *pred, ISA_Opcode opcode,
                                         VISA_EMask_Ctrl emask,
                                         VISA_Exec_Size execSize,
                                         VISA_opnd *dst, VISA_opnd *src0,
                                         VISA_opnd *src1, int lineNum);

  bool CISA_create_lfsr_instruction(VISA_opnd *pred,
                                    VISA_EMask_Ctrl emask,
                                    VISA_Exec_Size execSize,
                                    LFSR_FC funcCtrl,
                                    VISA_opnd *dst,
                                    VISA_opnd *src0,
                                    VISA_opnd *src1,
                                    int lineNum);

  bool CISA_create_dnscl_instruction(VISA_opnd *pred, VISA_EMask_Ctrl emask,
                                     VISA_Exec_Size execSize,
                                     DNSCL_CONVERT_TYPE type, DNSCL_MODE mode,
                                     DNSCL_RND_MODE rndMode, VISA_opnd *dst,
                                     VISA_opnd *src0, VISA_opnd *src1,
                                     VISA_opnd *src2, int lineNum);

private:
  const vISA::PlatformInfo *m_platformInfo;

  vISA::Mem_Manager m_mem = 4096;
  const VISA_BUILDER_OPTION mBuildOption;
  // FIXME: we need to make 3D/media per kernel instead of per builder
  const vISABuilderMode m_builderMode;

  unsigned int m_kernel_count = 0;
  unsigned int m_function_count = 0;

  // list of kernels and functions added to this builder
  KernelListTy m_kernelsAndFunctions;

  // for cases of several kernels/functions in one CisaBuilder
  // we need to keep a mapping of kernels to names
  // to make GetVISAKernel() work
  std::map<std::string, VISAKernelImpl *> m_nameToKernel;

  std::map<std::string, vISA::G4_Kernel *> functionsNameMap;

  // Set of functions that should be called directly (vs. indirect calls).
  // Used in ESIMD+SPMD interop scenarios.
  std::unordered_set<std::string> m_directCallFunctions;

  const WA_TABLE *m_pWaTable;
  bool needsToFreeWATable = false;

  void *gtpin_init = nullptr;

  // important messages that we should relay to the user
  // (things like if RA is spilling, etc.)
  std::stringstream criticalMsg;

private:
  // Summarize sub-functions' FINALIZER_INFO and propagate them into main
  // functions'. This functions handles perf stats, barrier count and
  // spill/stack size estimation. After Stitch_Compiled_Units the
  // "mainFunction" is merged with subFunctions and becomes a single
  // binary, which should also contains the functions' info of subFunctions.
  void summarizeFunctionInfo(
      KernelListTy &mainFunctions, KernelListTy &subFunctions);

  vISA::G4_Kernel *GetCallerKernel(vISA::G4_INST *);
  vISA::G4_Kernel *GetCalleeKernel(vISA::G4_INST *);

  // To collect call related info for LinkTimeOptimization
  void CollectCallSites(
      KernelListTy &functions,
      std::unordered_map<vISA::G4_Kernel *,
                         std::list<std::list<vISA::G4_INST *>::iterator>>
          &callSites,
      std::list<std::list<vISA::G4_INST *>::iterator> &sgInvokeList);

  // Sanity check to see if sg.invoke list is properly added from front-end
  // We don't support:
  //   1. sg.invoke callsite is a indirect call
  //   2. sg.invoke callsite is inside a recursion
  void CheckHazardFeatures(
      std::list<std::list<vISA::G4_INST *>::iterator> &sgInvokeList,
      std::unordered_map<vISA::G4_Kernel *,
                         std::list<std::list<vISA::G4_INST *>::iterator>>
          &callSites);

  // Reset hasStackCalls if all calls in a function are converted to subroutine
  // calls or inlined
  void ResetHasStackCall(
      std::list<std::list<vISA::G4_INST *>::iterator> &sgInvokeList,
      std::unordered_map<vISA::G4_Kernel *,
                         std::list<std::list<vISA::G4_INST *>::iterator>>
          &callSites);

  // Remove sgInvoke functions out of function list to avoid redundant
  // compilation
  void RemoveOptimizingFunction(
      const std::list<std::list<vISA::G4_INST *>::iterator> &sgInvokeList);

  // Create callee to a set of callsites map
  void ProcessSgInvokeList(
      const std::list<std::list<vISA::G4_INST *>::iterator> &sgInvokeList,
      std::unordered_map<vISA::G4_Kernel *,
                         std::list<std::list<vISA::G4_INST *>::iterator>>
          &callee2Callers);

  // Propagate info from callee to callee
  void PropagateInfo(
      vISA::G4_Kernel *caller, vISA::G4_Kernel *callee);

  // Perform LinkTimeOptimization for call related transformations
  void LinkTimeOptimization(
      std::unordered_map<vISA::G4_Kernel *,
                         std::list<std::list<vISA::G4_INST *>::iterator>>
          &callee2Callers,
      uint32_t options);

  void emitFCPatchFile();
};

#endif
