/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_KV_H
#define IGA_KV_H

#include "iga.h"
#include "iga_types_swsb.hpp"

/*************************************************************************
 *                                                                       *
 *                  The KernelView C interface                           *
 *                                                                       *
 *************************************************************************/

#ifdef __cplusplus
extern "C"  {
#endif

/*
 * This symbols defines the maximum number of PC targets that an instruction
 * may have.  It is typically used to statically allocate an array of target
 * PCs with the kv_get_inst_targets function.
 * E.g.
 *   uint32_t targetPCs[KV_MAX_TARGETS_PER_INSTRUCTION];
 *   uint32_t num = kv_get_inst_targets(kv, atPc, &targets[0]);
 *   for (int i = 0; i < num; i++) {
 *      processTarget(targetPCs[i]);
 *   }
 */
#define KV_MAX_TARGETS_PER_INSTRUCTION 3
/*
* This symbol represents an invalid PC.  0 is a valid PC (the beginnning
* of the kernel).
*/
#define KV_INVALID_PC_VALUE ((int32_t)0xFFFFFFFF)

/* incomplete type for a kernel view handle */
struct kv_t;

/* Kernel Viewer API Statuses */
typedef enum {
    KV_SUCCESS                = 0,
    KV_ERROR                  = 1, /* general error (unlisted below) */
    KV_DECODE_ERROR           = 2, /*
                                    * error during initial decode of the kernel
                                    * always check KernelView::decodeSucceeded.
                                    */
    KV_INVALID_PC             = 3,  /* invalid instruction PC */

    KV_INVALID_ARGUMENT       = 10, /* an invalid argument passed in (e.g. nullptr) */

    KV_NON_SEND_INSTRUCTION   = 20, /* underlying inst isn't a send */
    KV_DESCRIPTOR_INDIRECT    = 21, /* a send message with a reg desc */
    KV_DESCRIPTOR_INVALID     = 22, /* an unrecognized send descriptor */
    KV_NO_SUBFUNCTION         = 23, /* underlying inst has no sub-function */

    KV_INCAPABLE_PLATFORM     = 30  /* invalid API for the given platform */
} kv_status_t;

/*
 * Creates a kernel view.
 *   'plat' - the platform
 *   'bytes' - the kernel binary
 *   'bytes_len' - the length of 'bytes'
 *   'status' - the IGA status code
 *   'errbuf' - an optional buffer to emit errors or warnings (can pass nullptr)
 *   'errbuf_cap' - the capacity of errbuf.
 * RETURNS: a kernel view object for use in other kv_* functions.
 *  Deallocate it with kv_delete.  If there is a decode error (or other errors), this
 *  function returns an instance of Kernel Views and ERROR status. If user proceeds
 * to use the returned Kernel View we do not guarantee that all bits are correct
 */
IGA_API kv_t *kv_create(
    iga_gen_t plat,
    const void *bytes,
    size_t bytes_len,
    iga_status_t *status,
    char *errbuf,
    size_t errbuf_cap,
    // if not specified, the swsb encoding mode will be derived from platfrom
    // by SWSB::getEncodeMode
    iga::SWSB_ENCODE_MODE swsb_enc_mode
        = iga::SWSB_ENCODE_MODE::SWSBInvalidMode
    );

/* destroys a kernel view */
IGA_API void kv_delete(kv_t *);


/*
* Returns the size of the instruction at 'pc'; returns 0 if the program
* address is out of bounds.  This allows one to iterate a kernel using this
* API.  For example:
*
*   uint32_t iLen;
*   for (uint32_t pc = 0;
*        (iLen = kv_get_inst_size(kv, pc)) != 0;
*        pc += iLen)
*   {
*     ... process instruction
*   }
*/
IGA_API int32_t kv_get_inst_size(const kv_t *kv, int32_t pc);

/*
* Returns true if the instruction has the opt
*/
IGA_API bool kv_has_inst_opt(const kv_t *kv, int32_t pc, uint32_t opt);

/*
* This function returns the absolute PC targets of this instruction.
* For branching instructions, it populates 'pcs' with the jump targets
* of this instruction.  The number of PC's will always be less than or
* equal to MAX_KV_TARGETS_COUNT.  The function returns the number of
* target PCs populated in the 'pcs' argument.
*
* For non-branching instructions this returns 0 and does not touch 'pcs'.
*
* If 'pcs' is NULL, it is ignored.  The number of targets is still returned.
*/
IGA_API uint32_t kv_get_inst_targets(
    const kv_t *kv,
    int32_t pc,
    int32_t *pcs);


/*
 * This function returns the syntax for a given instruction.
 * The user passes the buffer 'sbuf' (along with its capacity) to hold
 * the output.  The output may be truncated if the passed buffer is too
 * small, but it will always be suffixed with a NUL byte.
 *
 * The formatting options 'fmt_opts' are the same as those in
 * iga_disassemble_options_t::formatting_opts.
 *
 * The optional 'get_label_name' callback may be called to converts
 * a PC into a label.  The caller can provide NULL and internal label names
 * will be used.  The 'env' context parameter is passed to 'get_label_name'.
 * Memory returned by the callback is only read by IGA.
 *
 * This function returns the number of characters written to sbuf
 * (including NUL byte).  If the PC is out of bounds or 'kv' NULL or
 * something else is wrong, then 0 is returned.
 */
IGA_API size_t kv_get_inst_syntax(
    const kv_t *kv,
    int32_t pc,
    char *sbuf,
    size_t sbuf_cap,
    uint32_t fmt_opts,
    const char *(*get_label_name)(int32_t, void *),
    void *env);

/*
 * This function returns the default label name if custom labeler is not used.
 */
IGA_API size_t kv_get_default_label_name(
    int32_t pc,
    char *sbuf,
    size_t sbuf_cap);

/*
 * Returns non-zero iff this instruction is a branch target.
 * The caller can use this function to determine if it should emit a label
 * first.
 */
IGA_API uint32_t kv_is_inst_target(const kv_t *kv, int32_t pc);


/*
 * This enumeration allows one to determine if a given PC is for structured
 * control flow.  This is for tools that want to render an indentation for
 * readability.
 */
typedef enum {
    KV_OPGROUP_INVALID,   /* not a valid op (e.g. out of bounds, middle of instruction) */
    KV_OPGROUP_OTHER,     /* some other instruction */
    KV_OPGROUP_IF,        /* an 'if' op */
    KV_OPGROUP_ELSE,      /* an 'else' op */
    KV_OPGROUP_ENDIF,     /* an 'endif' op */
    KV_OPGROUP_WHILE,     /* a 'while' op */
    KV_OPGROUP_SEND_EOT,  /* a send message with the EOT bit set */
} kv_opgroup_t;


/*
 * This function returns the opcode group.  The result may be compared
 * to the integral value of the various kv_opcode_group enumerates.
 * (See enum kv_get_opgroup_t.)
 */
IGA_API int32_t kv_get_opgroup(const kv_t *kv, int32_t pc);


/*
 * Returns the send function descriptors.  The count of descriptors is
 * returned; hence, if the instruction is invalid or not a send or
 * send using two index registers, 0 is returned.
 * If one of the descriptors is not immediate, then 1 is returned
 * and that descriptor is set to KV_INVALID_SEND_DESC.
 *
 * Also returns 0 if any parameter is NULL (and parameters are untouched).
 */
IGA_API uint32_t kv_get_send_descs(
    const kv_t *kv,
    int32_t pc,
    uint32_t *ex_desc,
    uint32_t *desc);



/*
 * Returns the indirect descriptor registers for a send message.
 * The function fails silently if the PC is invalid or a nullptr is passed.
 * Registers are assigned KV_INVALID_REG on other failures otherwise they
 * hold the index register and subregister (e.g. a0.2) would have 0 and 2.
 */
IGA_API void kv_get_send_indirect_descs(
    const kv_t *kv,
    int32_t pc,
    uint8_t *ex_desc_reg,
    uint8_t *ex_desc_subreg,
    uint8_t *desc_reg,
    uint8_t *desc_subreg);

/*
 * Determines if the given send instruction is on ExBSO mode.
 * exbso = 1 if true, 0 if fales.
 * exbso = -1 if not success.
 * exbso mode is introduces since XeHP.
 *
 * RETURNS:
 *  KV_SUCCESS               on success
 *  KV_NON_SEND_INSTRUCTION  if called on a non-send instruction
 *  KV_INVALID_PC            if passed an invalid PC
 *  KV_INVALID_ARGUMENT      if given a null parameter
 *  KV_INCAPABLE_PLATFORM    if it's not XeHP+ platform
 */
IGA_API kv_status_t kv_get_send_exbso(
    const kv_t *kv,
    int32_t pc,
    int32_t *exbso);



/*
 * A symbol to indicate an invalid send descriptor value.
 */
#define KV_INVALID_SEND_DESC ((uint32_t)0xFFFFFFFFF)

/* TODO: review necessity of this macro.
 * A symbol to indicate an invalid message length value.
 */
#define KV_INVALID_LEN ((uint32_t)0xFFFFFFFFF)

/*
 * Indicates invalid register
 */
#define KV_INVALID_REG 0xff

/*
 * Determines the message type for the given send instruction.
 * The result is returned via the pointer 'message_type_enum' - an
 * iga::SFMessageType value.
 *
 * RETURNS:
 *  KV_SUCCESS               on success
 *  KV_NON_SEND_INSTRUCTION  if called on a non-send instruction
 *  KV_DESCRIPTOR_INDIRECT   if called on a send with reg descriptors
 *  KV_DESCRIPTOR_INVALID    if unable to map the descriptor value
 *                           (not all messages are mapped via this API)
 *  KV_INVALID_PC            if passed an invalid PC
 *  KV_INVALID_ARGUMENT      if given a null parameter
 */
IGA_API kv_status_t kv_get_message_type(
    const kv_t *kv, int32_t pc, int32_t *message_type_enum);

/*
 * Determines the message type for the given send instruction.
 * desc and sfid are passed in explicitly when indirect desc prevents
 * use of kv_get_message_type.
 * The result is returned via the pointer 'message_type_enum' - an
 * iga::SFMessageType value.
 *
 * RETURNS:
 *  KV_SUCCESS               on success
 *  KV_NON_SEND_INSTRUCTION  if called on a non-send instruction
 *  KV_DESCRIPTOR_INDIRECT   if called on a send with reg descriptors
 *  KV_DESCRIPTOR_INVALID    if unable to map the descriptor value
 *                           (not all messages are mapped via this API)
 *  KV_INVALID_PC            if passed an invalid PC
 *  KV_INVALID_ARGUMENT      if given a null parameter
 */
IGA_API kv_status_t kv_get_message_type_ext(
    const kv_t *kv, int32_t pc, uint32_t desc, int32_t sfid, int32_t *message_type_enum);

/*
 * Determines the message sfid for the given send instruction.
 * The result is returned via the pointer 'sfid_enum' - an iga::SFID
 *
 * RETURNS:
 *  KV_SUCCESS               on success
 *  KV_NON_SEND_INSTRUCTION  if called on a non-send instruction
 *  KV_DESCRIPTOR_INDIRECT   if called on a send with reg descriptors
 *  KV_DESCRIPTOR_INVALID    if unable to map the descriptor value
 *  KV_INVALID_PC            if passed an invalid PC
 *  KV_INVALID_ARGUMENT      if given a null parameter
 */
IGA_API kv_status_t kv_get_message_sfid(
    const kv_t *kv, int32_t pc, int32_t *sfid_enum);

/*
 * Gets message length, extended message length, and response length in
 * units of registers.  The count of lengths successfully set is returned.
 * If any of the parameters is NULL, it returns 0.  Invalid lengths are set
 * to KV_INVALID_LEN.
 */
IGA_API uint32_t kv_get_message_len(
    const kv_t *kv, int32_t pc, uint32_t* mLen, uint32_t* emLen, uint32_t* rLen);

/*
 * Alternative version of kv_get_message_len when desc or exDesc are
 * indirect.  If any of the parameters is NULL, it returns 0.  Invalid lengths
 * are set to KV_INVALID_LEN.
 */
IGA_API uint32_t kv_get_message_len_ext(
    const kv_t *kv, int32_t pc, uint32_t desc, uint32_t exDesc,
    uint32_t* mLen, uint32_t* emLen, uint32_t* rLen);

/*
 * Returns the ExecSize of the instruction (SIMD width)
 * 0 - INVALID
 * 1 - EXEC_SIZE_1
 * 2 - EXEC_SIZE_2
 * 3 - EXEC_SIZE_4
 * 4 - EXEC_SIZE_8
 * 5 - EXEC_SIZE_16
 * 6 - EXEC_SIZE_32
 */
IGA_API uint32_t kv_get_execution_size(const kv_t *kv, int32_t pc);

/*
 * Returns Software scoreboarding information.
 */
IGA_API bool kv_get_swsb_info(
    const kv_t *kv, int32_t pc, iga::SWSB_ENCODE_MODE encdoe_mode,
    iga::SWSB& swsb);

/*
 * Returns number of sources this instruction has.
 */
IGA_API int32_t kv_get_number_sources(const kv_t *kv, int32_t pc);

/*
 * This function returns OPcode integer.  The value corresponds to
 * binary encoding value of the opcode.
 */
IGA_API uint32_t kv_get_opcode(const kv_t *kv, int32_t pc);

/*
 * This function returns OPcode integer.  The value corresponds to
 * binary encoding value of the opcode.
 */
IGA_API kv_status_t kv_get_subfunction(const kv_t *kv, int32_t pc, uint32_t* subfunc);

/*
 * This function returns if instruction has destination.
 */
IGA_API int32_t kv_get_has_destination(const kv_t *kv, int32_t pc);

/*
 * This function returns destination Register row
 */
IGA_API int32_t kv_get_destination_register(const kv_t *kv, int32_t pc);

/*
 * This function returns destination subRegister
 */
IGA_API int32_t kv_get_destination_sub_register(const kv_t *kv, int32_t pc);

/*
 * This function returns destination data type
 * i.e. F, HF, INT, etc
 */
IGA_API uint32_t kv_get_destination_data_type(const kv_t *kv, int32_t pc);

/*
 * This function returns destination register type
 * i.e. GRF, various ARF registers
 */
IGA_API uint32_t kv_get_destination_register_type(const kv_t *kv, int32_t pc);

/*
 * This function returns destination register KIND
 * DIRECT, INDIRECT, IMM, INDIR etc
 */
IGA_API uint32_t kv_get_destination_register_kind(const kv_t *kv, int32_t pc);

/*
 * This function returns source register line number for a given source.
 */
IGA_API int32_t kv_get_source_register(const kv_t *kv, int32_t pc, uint32_t sourceNumber);

/*
 * This function returns source subRegister for a given source.
 */
IGA_API int32_t kv_get_source_sub_register(const kv_t *kv, int32_t pc, uint32_t sourceNumber);

/*
 * This function returns source data type for a given source
 * i.e. F, HF, INT, etc
 */
IGA_API uint32_t kv_get_source_data_type(const kv_t *kv, int32_t pc, uint32_t sourceNumber);

/*
 * This function returns source register type for a given source.
 * i.e. GRF, various ARF registers
 */
IGA_API uint32_t kv_get_source_register_type(const kv_t *kv, int32_t pc, uint32_t sourceNumber);

/*
 * This function returns source register KIND for a given source
 * DIRECT, INDIRECT, IMM, INDIR etc
 */
IGA_API uint32_t kv_get_source_register_kind(const kv_t *kv, int32_t pc, uint32_t sourceNumber);

/*
 * This function returns whether source is a vector.
 */
IGA_API int32_t kv_is_source_vector(const kv_t *kv, int32_t pc, uint32_t sourceNumber);

/*
 * This function returns mask offset
 */
IGA_API uint32_t kv_get_channel_offset(const kv_t *kv, int32_t pc);

/*
 * This function returns mask control
 */
IGA_API uint32_t kv_get_mask_control(const kv_t *kv, int32_t pc);

/*
 * This function exposes destination region.
 */
IGA_API int32_t kv_get_destination_region(
    const kv_t *kv, int32_t pc, uint32_t *hz);

/*
 * This function exposes source operand region.
 */
IGA_API int32_t kv_get_source_region(
    const kv_t *kv, int32_t pc, uint32_t src_op,
    uint32_t *vt, uint32_t *wi, uint32_t *hz);

/*
 * This function exposes source operand immediate value.
 */
IGA_API int32_t kv_get_source_immediate(
    const kv_t *kv, int32_t pc, uint32_t src_op, uint64_t *imm);

/*
 * This function exposes indirect source's immediate offset.
   Return -1 if given source is not indirect srouce
 */
IGA_API int32_t kv_get_source_indirect_imm_off(
    const kv_t *kv, int32_t pc, uint32_t src_op, int16_t *immoff);

/*
 * This function exposes indirect destination's immediate offset.
   Return -1 if given destination is not indirect srouce
 */
IGA_API int32_t kv_get_destination_indirect_imm_off(
    const kv_t *kv, int32_t pc, int16_t *mme);

/*
 * This function exposes source's MathMacroExt number for
   math macro instructions.
   mme is the mme numbar, set to 8 if it's nomme.
   Return 0 if the given instruction is math macro instruction.
   Return -1 if given instruction is not math macro instruction.
 */
IGA_API int32_t kv_get_source_mme_number(
    const kv_t *kv, int32_t pc, uint32_t src_op, int16_t *mme);

/*
 * This function exposes destination's MathMacroExt number for
   math macro instructions.
   mme is the mme numbar, set to 8 if it's nomme.
   Return 0 if the given instruction is math macro instruction.
   Return -1 if given instruction is not math macro instruction.
 */
IGA_API int32_t kv_get_destination_mme_number(
    const kv_t *kv, int32_t pc, int16_t *immoff);

/*
 * This function return flag modifier (FlagModifier)
 */
IGA_API uint32_t kv_get_flag_modifier(const kv_t *kv, int32_t pc);

/*
 * This function return source modifier
 * This returns a SrcModifier
 */
IGA_API uint32_t kv_get_source_modifier(
    const kv_t *kv, int32_t pc, uint32_t src_op);

/*
 * This function return destination modifier
 * This returns a DstModifier
 */
IGA_API uint32_t kv_get_destination_modifier(const kv_t *kv, int32_t pc);

/*
 * This function return the flag register
 */
IGA_API int32_t kv_get_flag_register(const kv_t *kv, int32_t pc);

/*
 * This function return the flag sub register
 */
IGA_API int32_t kv_get_flag_sub_register(const kv_t *kv, int32_t pc);

/*
 * This function return the flag predicate function (a PredCtrl)
 */
IGA_API uint32_t kv_get_predicate(const kv_t *kv, int32_t pc);

/*
 * This function returns the logical sign on the predicate (inverted or not)
 */
IGA_API uint32_t kv_get_is_inverse_predicate(const kv_t *kv, int32_t pc);



#ifdef __cplusplus
}
#endif
#endif
