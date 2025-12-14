/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGAD_H_
#define _IGAD_H_

#include "iga.h"
#include "kv.h"

#if defined(_WIN32)
#define CDECLATTRIBUTE __cdecl
#elif __GNUC__
#ifdef __x86_64__
#define CDECLATTRIBUTE
#elif defined(__ARM_ARCH) || defined(__riscv)
#define CDECLATTRIBUTE
#else
#define CDECLATTRIBUTE __attribute__((__cdecl__))
#endif
#endif

#ifdef __cplusplus
extern "C" {

#define IGA_VERSION_STRING_STR "iga_version_string"
typedef const char *(CDECLATTRIBUTE *pIGAVersionString)();

#define IGA_STATUS_TO_STRING_STR "iga_status_to_string"
typedef const char *(CDECLATTRIBUTE *pIGAStatusToString)(const iga_status_t st);

#define IGA_CONTEXT_CREATE_STR "iga_context_create"
typedef iga_status_t(CDECLATTRIBUTE *pIGAContextCreate)(
    const iga_context_options_t *opts, iga_context_t *ctx);
/* deprecated name */
#define IGA_CREATE_CONTEXT_STR "iga_create_context"
typedef iga_status_t(CDECLATTRIBUTE *pIGACreateContext)(
    const iga_context_options_t *opts, iga_context_t *ctx);

#define IGA_CONTEXT_RELEASE_STR "iga_context_release"
typedef iga_status_t(CDECLATTRIBUTE *pIGAContextRelease)(iga_context_t ctx);
/* deprecated name */
#define IGA_RELEASE_CONTEXT_STR "iga_release_context"
typedef iga_status_t(CDECLATTRIBUTE *pIGAReleaseContext)(iga_context_t ctx);

#define IGA_CONTEXT_ASSEMBLE_STR "iga_context_assemble"
typedef iga_status_t(CDECLATTRIBUTE *pIGAContextAssemble)(
    iga_context_t ctx, const iga_assemble_options_t *opts,
    const char *kernel_text, void **output, uint32_t *output_size);
/* deprecated name */
#define IGA_ASSEMBLE_STR "iga_assemble"
typedef iga_status_t(CDECLATTRIBUTE *pIGAAssemble)(
    iga_context_t ctx, const iga_assemble_options_t *opts,
    const char *kernel_text, void **output, uint32_t *output_size);

#define IGA_CONTEXT_DISASSEMBLE_STR "iga_context_disassemble"
typedef iga_status_t(CDECLATTRIBUTE *pIGAContextDisassemble)(
    iga_context_t ctx, const iga_disassemble_options_t *opts, const void *input,
    uint32_t input_size, const char *(*fmt_label_name)(int32_t, void *),
    void *fmt_label_ctx, char **kernel_text);
/* deprecated name */
#define IGA_DISASSEMBLE_STR "iga_disassemble"
typedef iga_status_t(CDECLATTRIBUTE *pIGADisassemble)(
    iga_context_t ctx, const iga_disassemble_options_t *opts, const void *input,
    uint32_t input_size, const char *(*fmt_label_name)(int32_t, void *),
    void *fmt_label_ctx, char **kernel_text);

#define IGA_CONTEXT_DISASSEMBLE_INSTRUCTION_STR                                \
  "iga_context_disassemble_instruction"
typedef iga_status_t(CDECLATTRIBUTE *pIGAContextDisassembleInstruction)(
    iga_context_t ctx, const iga_disassemble_options_t *dopts,
    const void *input, const char *(*fmt_label_name)(int32_t, void *),
    void *fmt_label_ctx, char **kernel_text);
/* deprecated name */
#define IGA_DISASSEMBLE_INSTRUCTION_STR "iga_disassemble_instruction"
typedef iga_status_t(CDECLATTRIBUTE *pIGADisassembleInstruction)(
    iga_context_t ctx, const iga_disassemble_options_t *dopts,
    const void *input, const char *(*fmt_label_name)(int32_t, void *),
    void *fmt_label_ctx, char **kernel_text);

#define IGA_CONTEXT_GET_ERRORS_STR "iga_context_get_errors"
typedef iga_status_t(CDECLATTRIBUTE *pIGAContextGetErrors)(
    iga_context_t ctx, const iga_diagnostic_t **ds, uint32_t *ds_len);
/* deprecated name */
#define IGA_GET_ERRORS_STR "iga_get_errors"
typedef iga_status_t(CDECLATTRIBUTE *pIGAGetErrors)(iga_context_t ctx,
                                                    const iga_diagnostic_t **ds,
                                                    uint32_t *ds_len);

#define IGA_CONTEXT_GET_WARNINGS_STR "iga_context_get_warnings"
typedef iga_status_t(CDECLATTRIBUTE *pIGAContextGetWarnings)(
    iga_context_t ctx, const iga_diagnostic_t **ds, uint32_t *ds_len);
/* deprecated name */
#define IGA_GET_WARNINGS_STR "iga_get_warnings"
typedef iga_status_t(CDECLATTRIBUTE *pIGAGetWarnings)(
    iga_context_t ctx, const iga_diagnostic_t **ds, uint32_t *ds_len);

#define IGA_DIAGNOSTIC_GET_MESSAGE_STR "iga_diagnostic_get_message"
typedef iga_status_t(CDECLATTRIBUTE *pIGADiagnosticGetMessage)(
    const iga_diagnostic_t *d, const char **message);
#define IGA_GIAGNOSTIC_GET_OFFSET "iga_diagnostic_get_offset"
typedef iga_status_t(CDECLATTRIBUTE *pIGADiagnosticGetOffset)(
    const iga_diagnostic_t *d, uint32_t *offset);
#define IGA_DIAGNOSITC_GET_TYPE_STR "iga_diagnostic_get_type"
typedef iga_status_t(CDECLATTRIBUTE *pIGADiagnosticGetType)(
    const iga_diagnostic_t *d, iga_diagnostic_type_t *dt);
#define IGA_DIAGNOSTIC_GET_TEXT_LINE_STR "iga_diagnostic_get_text_line"
typedef iga_status_t(CDECLATTRIBUTE *pIGADiagnosticGetTextLine)(
    const iga_diagnostic_t *d, uint32_t *line);
#define IGA_DIAGNOSTIC_GET_TEXT_COLUMN_STR "iga_diagnostic_get_text_column"
typedef iga_status_t(CDECLATTRIBUTE *pIGADiagnosticGetTextColumn)(
    const iga_diagnostic_t *d, uint32_t *col);
#define IGA_DIAGNOSTIC_GET_TEXT_EXTENT_STR "iga_diagnostic_get_text_extent"
typedef iga_status_t(CDECLATTRIBUTE *pIGADiagnosticGetTextExtent)(
    const iga_diagnostic_t *d, uint32_t *extent);

#define IGA_OPSEC_ENUMERATE_STR "iga_opspec_enumerate"
typedef iga_status_t(CDECLATTRIBUTE *pIGAOpspecEnumerate)(iga_gen_t gen,
                                                          iga_opspec_t *ops_arr,
                                                          size_t *ops_arr_len);
#define IGA_OPSPEC_MNEMONIC_STR "iga_opspec_mnemonic"
typedef iga_status_t(CDECLATTRIBUTE *pIGAOpspecMnemonic)(iga_opspec_t op,
                                                         char *mnemonic,
                                                         size_t *mnemonic_len);
#define IGA_OPSPEC_NAME_STR "iga_opspec_name"
typedef iga_status_t(CDECLATTRIBUTE *pIGAOpspecName)(iga_opspec_t op,
                                                     char *name,
                                                     size_t *name_len);
#define IGA_OPSPEC_DESCRIPTION_STR "iga_opspec_description"
typedef iga_status_t(CDECLATTRIBUTE *pIGAOpspecDescription)(iga_opspec_t op,
                                                            char *desc,
                                                            size_t *desc_len);
#define IGA_OPSPEC_OP_STR "iga_opspec_op"
typedef iga_status_t(CDECLATTRIBUTE *pIGAOpspecOp)(iga_opspec_t op,
                                                   uint32_t *opcode);

/*************************************************************************
 *                                                                       *
 *                  The KernelView C interface                           *
 *                                                                       *
 *************************************************************************/

#define IGA_KV_CREATE_STR "kv_create"
typedef kv_t *(CDECLATTRIBUTE *pIGAKVCreate)(
    iga_gen_t plat, const void *bytes, size_t bytes_len, iga_status_t *status,
    char *errbuf, size_t errbuf_cap,
    /* iga::SWSB_ENCODE_MODE */ uint32_t swsb_mode
    );
#define IGA_KV_DELETE_STR "kv_delete"
typedef void(CDECLATTRIBUTE *pIGAKVDelete)(kv_t *);

#define IGA_KV_GET_INST_SIZE_STR "kv_get_inst_size"
typedef int32_t(CDECLATTRIBUTE *pIGAKVGetInstSize)(const kv_t *kv, int32_t pc);
#define IGA_KV_GET_INST_MSG_INFO_STR "kv_get_inst_msg_info"
typedef int32_t(CDECLATTRIBUTE *pIGAKVGetInstMsgInfo)(const kv_t *kv,
                                                      int32_t pc,
                                                      bool *isAtomic,
                                                      bool *isSlm,
                                                      bool *isScratch);
#define IGA_KV_GET_INST_TARGETS_STR "kv_get_inst_targets"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetInstTargets)(const kv_t *kv,
                                                       int32_t pc,
                                                       int32_t *pcs);
#define IGA_KV_IS_INST_TARGET_STR "kv_is_inst_target"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVIsInstTarget)(const kv_t *kv,
                                                     int32_t pc);
#define IGA_KV_GET_DEFAULT_LABEL_NAME_STR "kv_get_default_label_name"
typedef size_t(CDECLATTRIBUTE *pIGAKVGetDefaultLabelName)(int32_t pc,
                                                          char *sbuf,
                                                          size_t sbuf_cap);
#define IGA_KV_GET_INST_SYNTAX_STR "kv_get_inst_syntax"
typedef size_t(CDECLATTRIBUTE *pIGAKVGetInstSyntax)(
    const kv_t *kv, int32_t pc, char *sbuf, size_t sbuf_cap, uint32_t fmt_opts,
    const char *(*get_label_name)(int32_t, void *), void *env);
#define IGA_KV_GET_OPGROUP_STR "kv_get_opgroup"
typedef int32_t(CDECLATTRIBUTE *pIGAKVGetOpgroup)(const kv_t *kv, int32_t pc);
#define IGA_KV_GET_SEND_DESCS_STR "kv_get_send_descs"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetSendDescs)(const kv_t *kv, int32_t pc,
                                                     uint32_t *ex_desc,
                                                     uint32_t *desc);
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetSendExDescImmOff)(
    const kv_t *kv, int32_t pc, uint32_t *ex_desc_immoff);
#define IGA_KV_GET_SEND_INDIRECT_DESCS_STR "kv_get_send_indirect_descs"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetSendIndirectDescs)(
    const kv_t *kv, int32_t pc, uint8_t *ex_desc_reg, uint8_t *ex_desc_subreg,
    uint8_t *desc_reg, uint8_t *desc_subreg);
typedef kv_status_t(CDECLATTRIBUTE *pIGAKVGetSendgDesc)(const kv_t *kv,
                                                        int32_t pc,
                                                        uint64_t *desc);
typedef kv_status_t(CDECLATTRIBUTE *pIGAKVGetSendgIndDesc)(
    const kv_t *kv, int32_t pc, uint32_t *subregPresent, uint32_t *subreg);

/************************* KV Analysis APIS **********************************/

#define IGA_KV_HAS_INST_OPT_STR "kv_has_inst_opt"
typedef bool(CDECLATTRIBUTE *pIGAKVHasInstOpt)(const kv_t *kv, int32_t pc,
                                               uint32_t opt);

#define IGA_KV_GET_EXECUTION_SIZE "kv_get_execution_size"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetExecutionSize)(const kv_t *kv,
                                                         int32_t pc);

#define IGA_KV_GET_SWSB_INFO "kv_get_swsb_info"
typedef bool(CDECLATTRIBUTE *pIGAKVGetSWSBInfo)(
    const kv_t *kv, int32_t pc, iga::SWSB_ENCODE_MODE encdoe_mode,
    iga::SWSB &swsb);

#define IGA_KV_GET_NUMBER_SOURCES_STR "kv_get_number_sources"
typedef int32_t(CDECLATTRIBUTE *pIGAKVGetNumberSources)(const kv_t *kv,
                                                        int32_t pc);
#define IGA_KV_GET_OPCODE_STR "kv_get_opcode"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetOpcode)(const kv_t *kv, int32_t pc);
#define IGA_KV_GET_SUBFUNCTION_STR "kv_get_subfunction"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetSubfunction)(const kv_t *kv,
                                                       int32_t pc,
                                                       uint32_t *subfunc);
#define IGA_KV_GET_HAS_DESTINATION_STR "kv_get_has_destination"
typedef int32_t(CDECLATTRIBUTE *pIGAKVGetHasDestination)(const kv_t *kv,
                                                         int32_t pc);
#define IGA_KV_GET_DESTINATION_REGISTER_STR "kv_get_destination_register"
typedef int32_t(CDECLATTRIBUTE *pIGAKVGetDstRegister)(const kv_t *kv,
                                                      int32_t pc);
#define IGA_KV_GET_DESTINATION_SUB_REGISTER_STR                                \
  "kv_get_destination_sub_register"
typedef int32_t(CDECLATTRIBUTE *pIGAKVGetDstSubRegister)(const kv_t *kv,
                                                         int32_t pc);
#define IGA_KV_GET_DESTINATION_DATA_TYPE_STR "kv_get_destination_data_type"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetDstDataType)(const kv_t *kv,
                                                       int32_t pc);
#define IGA_KV_GET_DESTINATION_REGISTER_TYPE_STR                               \
  "kv_get_destination_register_type"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetDstRegisterType)(const kv_t *kv,
                                                           int32_t pc);
#define IGA_KV_GET_DESTINATION_REGISTER_KIND_STR                               \
  "kv_get_destination_register_kind"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetDstRegisterKind)(const kv_t *kv,
                                                           int32_t pc);
#define IGA_KV_GET_SOURCE_REGISTER_STR "kv_get_source_register"
typedef int32_t(CDECLATTRIBUTE *pIGAKVGetSrcRegister)(const kv_t *kv,
                                                      int32_t pc,
                                                      uint32_t sourceNumber);
#define IGA_KV_GET_SOURCE_SUB_REGISTER_STR "kv_get_source_sub_register"
typedef int32_t(CDECLATTRIBUTE *pIGAKVGetSrcSubRegister)(const kv_t *kv,
                                                         int32_t pc,
                                                         uint32_t sourceNumber);
#define IGA_KV_GET_SOURCE_DATA_TYPE_STR "kv_get_source_data_type"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetSrcDataType)(const kv_t *kv,
                                                       int32_t pc,
                                                       uint32_t sourceNumber);
#define IGA_KV_GET_SOURCE_REGISTER_TYPE_STR "kv_get_source_register_type"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetSrcRegisterType)(
    const kv_t *kv, int32_t pc, uint32_t sourceNumber);
#define IGA_KV_GET_SOURCE_REGISTER_KIND_STR "kv_get_source_register_kind"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetSrcRegisterKind)(
    const kv_t *kv, int32_t pc, uint32_t sourceNumber);
#define IGA_KV_GET_IS_SOURCE_VECTOR_STR "kv_is_source_vector"
typedef int32_t(CDECLATTRIBUTE *pIGAKVGetIsSrcVector)(const kv_t *kv,
                                                      int32_t pc,
                                                      uint32_t sourceNumber);
#define IGA_KV_GET_CHANNEL_OFFSET_STR "kv_get_channel_offset"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetChannelOffset)(const kv_t *kv,
                                                         int32_t pc);
#define IGA_KV_GET_MASK_CONTROL_STR "kv_get_mask_control"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetMaskControl)(const kv_t *kv,
                                                       int32_t pc);
#define IGA_KV_GET_SEND_EXBSO_STR "kv_get_send_exbso"
typedef kv_status_t(CDECLATTRIBUTE *pIGAKVGetSendExBso)(const kv_t *kv,
                                                        int32_t pc,
                                                        int32_t *exbso);
#define IGA_KV_GET_MESSAGE_TYPE_STR "kv_get_message_type"
typedef kv_status_t(CDECLATTRIBUTE *pIGAKVGetMessageType)(
    const kv_t *kv, int32_t pc, int32_t *message_type_enum);
#define IGA_KV_GET_MESSAGE_SFID_STR "kv_get_message_sfid"
typedef kv_status_t(CDECLATTRIBUTE *pIGAKVGetMessageSFID)(const kv_t *kv,
                                                          int32_t pc,
                                                          int32_t *sfid_enum);
#define IGA_KV_GET_MESSAGE_LEN_STR "kv_get_message_len"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetMessageLen)(const kv_t *kv,
                                                      int32_t pc,
                                                      uint32_t *mLen,
                                                      uint32_t *emLen,
                                                      uint32_t *rLen);
#define IGA_KV_GET_IS_DESTINATION_REGION_STR "kv_get_destination_region"
typedef int32_t(CDECLATTRIBUTE *pIGAKVGetDstRegion)(const kv_t *kv, int32_t pc,
                                                    uint32_t *hz);
#define IGA_KV_GET_IS_SOURCE_REGION_STR "kv_get_source_region"
typedef int32_t(CDECLATTRIBUTE *pIGAKVGetSrcRegion)(const kv_t *kv, int32_t pc,
                                                    uint32_t src_op,
                                                    uint32_t *vt, uint32_t *wi,
                                                    uint32_t *hz);
#define IGA_KV_GET_SOURCE_IMMEDIATE_STR "kv_get_source_immediate"
typedef int32_t(CDECLATTRIBUTE *pIGAKVGetSrcImmediate)(const kv_t *kv,
                                                       int32_t pc,
                                                       uint32_t src_op,
                                                       uint64_t *imm);
#define IGA_KV_GET_FLAG_MODIFIER_STR "kv_get_flag_modifier"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetFlagModifier)(const kv_t *kv,
                                                        int32_t pc);
#define IGA_KV_GET_SOURCE_MODIFIER_STR "kv_get_source_modifier"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetSrcModifier)(const kv_t *kv,
                                                       int32_t pc,
                                                       uint32_t src_op);
#define IGA_KV_GET_DESTINATION_MODIFIER_STR "kv_get_destination_modifier"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetDstModifier)(const kv_t *kv,
                                                       int32_t pc);
#define IGA_KV_GET_FLAG_REGISTER_STR "kv_get_flag_register"
typedef int32_t(CDECLATTRIBUTE *pIGAKVGetFlagReg)(const kv_t *kv, int32_t pc);
#define IGA_KV_GET_FLAG_SUB_REGISTER_STR "kv_get_flag_sub_register"
typedef int32_t(CDECLATTRIBUTE *pIGAKVGetFlagSubReg)(const kv_t *kv,
                                                     int32_t pc);
#define IGA_KV_GET_PREDICATE "kv_get_predicate"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetPredicate)(const kv_t *kv,
                                                     int32_t pc);
#define IGA_KV_GET_IS_INVERSE_PREDICATE "kv_get_is_inverse_predicate"
typedef uint32_t(CDECLATTRIBUTE *pIGAKVGetIsInversePred)(const kv_t *kv,
                                                         int32_t pc);
#define IGA_KV_GET_SOURCE_INDIRECT_IMM_OFF_STR "kv_get_source_indirect_imm_off"
typedef int32_t(CDECLATTRIBUTE *pIGAKVGetSrcIndirectImmOff)(const kv_t *kv,
                                                            int32_t pc,
                                                            uint32_t src_op,
                                                            int16_t *immoff);
#define IGA_KV_GET_DESTINATION_INDIRECT_IMM_OFF_STR                            \
  "kv_get_destination_indirect_imm_off"
typedef int32_t(CDECLATTRIBUTE *pIGAKVGetDstIndirectImmOff)(const kv_t *kv,
                                                            int32_t pc,
                                                            int16_t *immoff);
#define IGA_KV_GET_SOURCE_MME_NUMBER "kv_get_source_mme_number"
typedef int32_t(CDECLATTRIBUTE *pIGAKVGetSrcMMENumber)(const kv_t *kv,
                                                       int32_t pc,
                                                       uint32_t src_op,
                                                       int16_t *mme);
#define IGA_KV_GET_DESTINATION_MME_NUMBER "kv_get_destination_mme_number"
typedef int32_t(CDECLATTRIBUTE *pIGAKVGetDstMMENumber)(const kv_t *kv,
                                                       int32_t pc,
                                                       int16_t *mme);
#define IGA_KV_GET_CACHE_OPT "kv_get_cache_opt"
typedef kv_status_t(CDECLATTRIBUTE *pIGAKVGetCacheOpt)(const kv_t *kv,
                                                       int32_t pc,
                                                       int32_t cache_level,
                                                       int32_t *cacheopt_enum);
/*
 * A table of IGA functions
 */
typedef struct {
  /* miscellaneous API functions */
  pIGAVersionString iga_version_string;
  pIGAStatusToString iga_status_to_string;
  /* context functions */
  pIGAContextCreate iga_context_create;
  pIGAContextRelease iga_context_release;
  pIGAContextAssemble iga_context_assemble;
  pIGAContextDisassemble iga_context_disassemble;
  pIGAContextDisassembleInstruction iga_context_disassemble_instruction;
  pIGAContextGetErrors iga_context_get_errors;
  pIGAContextGetWarnings iga_context_get_warnings;
  /* diagnostic query functions */
  pIGADiagnosticGetMessage iga_diagnostic_get_message;
  pIGADiagnosticGetOffset iga_diagnostic_get_offset;
  pIGADiagnosticGetType iga_diagnostic_get_type;
  pIGADiagnosticGetTextLine iga_diagnostic_get_text_line;
  pIGADiagnosticGetTextColumn iga_diagnostic_get_text_column;
  pIGADiagnosticGetTextExtent iga_diagnostic_get_text_extent;
  /* opspec functions */
  pIGAOpspecEnumerate iga_opspec_enumerate;
  pIGAOpspecMnemonic iga_opspec_mnemonic;
  pIGAOpspecName iga_opspec_name;
  pIGAOpspecDescription iga_opspec_description;
  pIGAOpspecOp iga_opspec_op;
} iga_functions_t;

/*
 * gets a table of the IGA functions
 *
 * RETURNS:
 *   IGA_SUCCESS on success
 *   IGA_INVALID if the parameter is nullptr
 */
#endif
IGA_API iga_status_t iga_get_interface(iga_functions_t *);

#ifdef __cplusplus
}
#endif

/* A table of all kernel viewer functions */
typedef struct {
  pIGAKVCreate kv_create;
  pIGAKVDelete kv_delete;
  pIGAKVGetInstSize kv_get_inst_size;
  pIGAKVGetInstMsgInfo kv_get_inst_msg_info;
  pIGAKVGetInstTargets kv_get_inst_targets;
  pIGAKVIsInstTarget kv_is_inst_target;
  pIGAKVGetDefaultLabelName kv_get_default_label_name;
  pIGAKVGetInstSyntax kv_get_inst_syntax;
  pIGAKVGetOpgroup kv_get_opgroup;
  pIGAKVGetSendDescs kv_get_send_descs;
  pIGAKVGetSendExDescImmOff kv_get_send_exdesc_immoff;
  pIGAKVGetSendgDesc kv_get_sendg_desc;
  pIGAKVGetSendgIndDesc kv_get_sendg_ind_desc0;
  pIGAKVGetSendgIndDesc kv_get_sendg_ind_desc1;
  pIGAKVGetSendIndirectDescs kv_get_send_indirect_descs;
  pIGAKVGetSendExBso kv_get_send_exbso;
  pIGAKVGetExecutionSize kv_get_execution_size;
  pIGAKVGetNumberSources kv_get_number_sources;
  pIGAKVGetOpcode kv_get_opcode;
  pIGAKVGetSubfunction kv_get_subfunction;
  pIGAKVGetHasDestination kv_get_has_destination;
  pIGAKVGetDstRegister kv_get_destination_register;
  pIGAKVGetDstSubRegister kv_get_destination_sub_register;
  pIGAKVGetDstDataType kv_get_destination_data_type;
  pIGAKVGetDstRegisterType kv_get_destination_register_type;
  pIGAKVGetDstRegisterKind kv_get_destination_register_kind;
  pIGAKVGetSrcRegister kv_get_source_register;
  pIGAKVGetSrcSubRegister kv_get_source_sub_register;
  pIGAKVGetSrcDataType kv_get_source_data_type;
  pIGAKVGetSrcRegisterType kv_get_source_register_type;
  pIGAKVGetSrcRegisterKind kv_get_source_register_kind;
  pIGAKVGetIsSrcVector kv_is_source_vector;
  pIGAKVGetChannelOffset kv_get_channel_offset;
  pIGAKVGetMaskControl kv_get_mask_control;
  pIGAKVGetMessageType kv_get_message_type;
  pIGAKVGetMessageSFID kv_get_message_sfid;
  pIGAKVGetMessageLen kv_get_message_len;
  pIGAKVGetDstRegion kv_get_destination_region;
  pIGAKVGetSrcRegion kv_get_source_region;
  pIGAKVGetSrcImmediate kv_get_source_immediate;
  pIGAKVGetFlagModifier kv_get_flag_modifier;
  pIGAKVGetSrcModifier kv_get_source_modifier;
  pIGAKVGetDstModifier kv_get_destination_modifier;
  pIGAKVGetFlagReg kv_get_flag_reg;
  pIGAKVGetFlagSubReg kv_get_flag_subreg;
  pIGAKVGetPredicate kv_get_predicate;
  pIGAKVGetIsInversePred kv_get_inverse_predicate;
  pIGAKVGetSWSBInfo kv_get_swsb_info;
  pIGAKVHasInstOpt kv_has_inst_opt;
  pIGAKVGetSrcIndirectImmOff kv_get_source_indirect_imm_off;
  pIGAKVGetDstIndirectImmOff kv_get_destination_indirect_imm_off;
  pIGAKVGetSrcMMENumber kv_get_source_mme_number;
  pIGAKVGetDstMMENumber kv_get_destination_mme_number;
  pIGAKVGetCacheOpt kv_get_cache_opt;
} kv_functions_t;

#endif // _IGAD_H_
