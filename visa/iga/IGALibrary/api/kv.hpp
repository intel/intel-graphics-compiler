/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_KV_HPP
#define IGA_KV_HPP

#include "iga_types_ext.hpp"
#include "kv.h"

// This convenience class wraps the pure C interface.
// Typical use involves something such as following.
//   const void *myBits = ... your kernel
//   size_t myBitsLen = ... the length in bytes
//   KernelView kv(IGA_GEN_SKL, &myBits, myBitsLen);
//   if (!kv.decodeSucceeded()) {
//      ReportDecodeError();
//      // all operations on kv should fail if !kv.decodeSucceeded()
//   } else {
//      ProcessKernelView(&kv);
//   }
// Further, a log may be passed in to the constructor that may be populated
// with error and warning diagnostics.
//   char log[256];
//   KernelView kv(IGA_GEN_SKL, &myBits, myBitsLen, &log, sizeof(log));
//   if (!kv.decodeSucceeded()) {
//      ReportDecodeError(log);
//      // all operations on kv should fail if !kv.decodeSucceeded()
//   }
//
class KernelView {
  // handle to the IGA-internal KV object; may be nullptr upon error
  kv_t *m_kv = nullptr;

  // The status from kv_create
  iga_status_t m_disasm_status = IGA_SUCCESS;

  // The platform this kernel view platform corresponds to
  iga_gen_t m_gen = IGA_GEN_INVALID;

public:
  // Constructs a kernel view.
  //
  // This disassembles the kernel and copies out the disassembly log,
  // which may indicate errors or warnings.
  KernelView(
      iga_gen_t platf, const void *bytes, size_t bytesLength,
      iga::SWSB_ENCODE_MODE swsb_mode = iga::SWSB_ENCODE_MODE::SWSBInvalidMode,
      char *decodeLog = nullptr, size_t decodeLogLen = 0)
      : m_kv(nullptr), m_disasm_status(IGA_SUCCESS), m_gen(platf) {
    m_kv = kv_create(platf, bytes, bytesLength, &m_disasm_status, decodeLog,
                     decodeLogLen, swsb_mode);
  }

  // The destructor automatically deallocates the underlying kernel view
  // and all IGA resources.
  ~KernelView() {
    if (m_kv) {
      kv_delete(m_kv);
      m_kv = nullptr;
    }
  }

  // Returns a raw handle to the underlying interface object.
  const kv_t *getHandle() const { return m_kv; }
  // Returns a raw handle to the underlying interface object.
  kv_t *getHandle() { return m_kv; }

  // If the kernel bits are malformed, m_kv is a valid instance of kernel View
  // If user proceeds to use it to retrieve information per instructiong we do
  // not gurantee that information is correct operations should be called.
  //
  // Upon decode failure, the 'decodeLog' arguments can be consulted for
  // a specific error message.
  bool decodeSucceeded() const { return decodeStatus() == IGA_SUCCESS; }

  // The decode status will be IGA_SUCCESS, IGA_OUT_OF_MEM or IGA_DECODE_ERROR
  iga_status_t decodeStatus() const { return m_disasm_status; }

  // Returns the size of the instruction at 'pc'
  // returns 0 if pc is out of bounds (invalid).
  //
  // This allows one to iterate a kernel
  //   KernelView k...;
  //   int32_t pc = 0, pcSz;
  //   while ((pcSz = k.getInstSize(pc)) != 0) {
  //       ... process instruction
  //       pc += pcSz;
  //   }
  int32_t getInstSize(int32_t pc) const { return kv_get_inst_size(m_kv, pc); }

  // Returns the targets of the instruction at PC in the given input
  // buffer.  The count of valid entries is returned.
  // The input buffer should be at least KV_MAX_TARGETS_PER_INSTRUCTION
  // elements long.  The nullptr can be passed instead if only the count
  // of targets is desired.
  //
  //  int32_t js[KV_MAX_TARGETS_PER_INSTRUCTION];
  //  size_t n = k.getInstTargets(pc, js);
  //  printf("  the instruction at PC%d:", (int)pc);
  //  for (size_t i = 0; i < n; i++) {
  //      printf("  targets PC %d", (int)js[i]);
  //  }
  //
  size_t getInstTargets(int32_t pc, int32_t *targetPcs) const {
    return (size_t)kv_get_inst_targets(m_kv, pc, targetPcs);
  }

  //  size_t count =  k.getInstTargetsCount(pc);
  //  if (count > 1) {
  //      int32_t jip = k.getInstJIP(pc);
  //      int32_t uip = 0;
  //      if(count == 2)
  //          uip = kv.getInstUIP(pc);
  //  }
  // OR
  //  int32_t jumps[KV_MAX_TARGETS_PER_INSTRUCTION];
  //  getInstTargets(pc, jumps);
  //  returns the number of targets of the instruction at 'pc'
  // size_t getInstTargetsCount(uint32_t pc) const {
  //    return kv_get_inst_targets(m_kv, pc);
  // }
  //
  // returns JIP or 0 if JIP is not valid.

  // If JIP is invalid or a register KV_INVALID_PC_VALUE is returned
  int32_t getInstJIP(int32_t pc) const {
    int32_t tpcs[KV_MAX_TARGETS_PER_INSTRUCTION];
    if (getInstTargets(pc, &tpcs[0]) < 1) {
      return KV_INVALID_PC_VALUE;
    } else {
      return tpcs[0];
    }
  }

  // If UIP is invalid KV_INVALID_PC_VALUE is returned
  int32_t getInstUIP(int32_t pc) const {
    int32_t tpcs[KV_MAX_TARGETS_PER_INSTRUCTION];
    if (getInstTargets(pc, &tpcs[0]) < 2) {
      return KV_INVALID_PC_VALUE;
    } else {
      return tpcs[1];
    }
  }

  // returns true if the instruction at 'pc' is the target of some branch
  // instruction (and needs a label). i.e. this is the beginning of a block
  bool isInstTarget(int32_t pc) const {
    return kv_is_inst_target(m_kv, pc) != 0;
  }

  // Generates syntax for the instruction at 'pc' to a user-provided buffer.
  // The required number of bytes is returned.  If sBuf is nullptr, then
  // it is ignored and the function just computes the number of characters
  // needed to format the instruction.
  //
  // An example use of this API is as follows:
  //    size_t n = kv.getInstSyntax(pc, nullptr, 0);
  //    char *str = (char *)alloca(n);
  //    auto fmtOpts = IGA_FORMATTING_OPTS_DEFAULT; // see iga.h
  //    (void)kv.getInstSyntax(pc, str, n, fmtOpts);
  //
  // The formatter options 'fmtOpts' are the same as those in
  // iga_disassemble_options_t::formatting_opts.  See iga.h for a
  // valid values.
  //
  // The final two arguments are for an optional labeler callback.
  // The function pointer points to a function which converts a PC
  // to a string.
  //  * The 'env' is threaded to that callback it as well.
  //  * The callback function should return a NUL terminated string
  //    for the given PC or nullptr if you don't care about a given pc.
  //  * The pointer returned is *not* freed or written by IGA.  For
  //    instructions that have multiple labels (e.g. an 'if' instruction)
  //    IGA will copy the string out before calling the the callback on
  //    the next label.  Hence, a single buffer can be reused by the
  //    callback for both labels.
  //
  // E.g.
  // static const char *labelerFunction(int32_t pc, void *buf) {
  //    snprintf((char*)buf, 63, "LABEL%u", pc);
  // }
  //
  // Then in your code you'd have:
  //
  //   char lblbuf[64];
  //   char instbuf[256]; // labeler environment (state)
  //   kv.getInstSyntax(
  //         pc,
  //         instbuf, sizeof(instbuf),
  //         IGA_FORMATTING_OPTS_DEFAULT,
  //         &labelerFunction, lblbuf);
  //
  // The above is safe for any instruction with multiple labels as 'lblbuf'
  // can be used multiple times safely.
  size_t getInstSyntax(int32_t pc, char *sBuf, size_t sBufCapacity,
                       uint32_t fmtOpts,
                       const char *(*labeler)(int32_t, void *) = nullptr,
                       void *env = nullptr) const {
    return kv_get_inst_syntax(m_kv, pc, sBuf, sBufCapacity, fmtOpts, labeler,
                              env);
  }
  size_t getInstSyntax(int32_t pc, char *sBuf, size_t sBufCapacity,
                       const char *(*labeler)(int32_t, void *) = nullptr,
                       void *env = nullptr) const {
    return getInstSyntax(pc, sBuf, sBufCapacity, IGA_FORMATTING_OPTS_DEFAULT,
                         labeler, env);
  }

  // This function returns the default label name if custom labeler is not used.
  size_t getDefaultLabelName(int32_t pc, char *sBuf,
                             size_t sBufCapacity) const {
    return kv_get_default_label_name(pc, sBuf, sBufCapacity);
  }

  // This method returns the opcode group.
  // (See kv_get_opcode_group.)
  kv_opgroup_t getOpcodeGroup(int32_t pc) const {
    return (kv_opgroup_t)kv_get_opgroup(m_kv, pc);
  }

  // Returns the send descriptors for send messages
  // (See kv_get_send_descs)
  uint32_t getSendDescs(int32_t pc, uint32_t *ex_desc, uint32_t *desc) const {
    return kv_get_send_descs(m_kv, pc, ex_desc, desc);
  }

  // Returns the ExDescImmOff field.  This field exists only on
  // LSC UGM BSS/SS AddressTypes on XE2+.
  //
  // Return 0xFFFFFFFF on failure (e.g. invalid PC) and the value on success.
  //
  // *NOTE WELL*: this field is encoded as described in BXML. See the
  // kv.h docs on kv_get_send_exdesc_immoff to see how to decode this value
  // to a signed offset.
  uint32_t getSendExDescImmOff(int32_t pc) const {
    uint32_t immoff = (uint32_t)-1;
    if (kv_get_send_exdesc_immoff(m_kv, pc, &immoff) != kv_status_t::KV_SUCCESS)
      return (uint32_t)-1;
    return immoff;
  }


  /*************************Analysis APIs **********************************/

  // Returns the number of expicit sources this instruction has.
  int32_t getNumberOfSources(int32_t pc) const {
    return kv_get_number_sources(m_kv, pc);
  }

  // Determines if the given send instruction is on ExBSO mode.
  // Return 1 if true, 0 if false
  // Return -1 if not success
  int32_t getSendExBso(int32_t pc) const {
    int32_t exbso = -1;
    if (kv_get_send_exbso(m_kv, pc, &exbso) != kv_status_t::KV_SUCCESS)
      return -1;
    return exbso;
  }

  // Fetches the message type for send/sends instructions.
  //
  // Returns:
  //   KV_SUCCESS on success (and assigned sfmt)
  //   KV_DESCRIPTOR_INDIRECT if called on a send with a register desriptor
  //   KV_DESCRIPTOR_INVALID if called on a send unrecognized descriptor
  //   KV_INVALID_PC if called on a non-instruction address
  //   KV_NON_SEND_INSTRUCTION if called on a non-send instruction
  kv_status_t getMessageType(int32_t pc, iga::SFMessageType &sfmt) const;

  // Fetches the message SFID for send/sends instructions.
  //
  // Returns:
  //   KV_SUCCESS on success (and assigned sfmt)
  //   KV_DESCRIPTOR_INDIRECT if called on a send with a register desriptor
  //   KV_DESCRIPTOR_INVALID if called on a send unrecognized descriptor
  //   KV_INVALID_PC if called on a non-instruction address
  //   KV_NON_SEND_INSTRUCTION if called on a non-send instruction
  kv_status_t getMessageSFID(int32_t pc, iga::SFID &sfid) const;

  // Fetches the cache control option for load/store instructions.
  //
  // Returns:
  //   KV_SUCCESS on success and cacheOpt is assigned
  //   KV_DECODE_ERROR on decoding error
  //   KV_NON_SEND_INSTRUCTION if called on a non-send instruction
  //   KV_INVALID_ARGUMENT if cacheLevel is invalid
  //   KV_INVALID_PC if called on a non-instruction address
  kv_status_t getCacheOpt(int32_t pc, iga::CacheLevel cahceLevel,
                          iga::CacheOpt &cacheOpt) const;

  // Returns message, extended message, and response lengths in units of
  // registers.  The count of length variables successfully set is returned.
  //
  //   - mLen (message length) is usually addresses to load or store to
  //   - emLen (extended message length) is usually data being stored
  //   - rLen (response length) is usually data being loaded
  //
  // If any of the parameters is NULL, it returns 0.
  // Invalid lengths are set to KV_INVALID_LEN.
  // Reasons for failures:
  //  - the instruction is not a send instruction
  //  - a register descriptor is used by the send instruction.
  //  - the given PC does not refer to an instruction.
  uint32_t getMessageLen(int32_t pc, uint32_t *mLen, uint32_t *emLen,
                         uint32_t *rLen) const {
    return kv_get_message_len(m_kv, pc, mLen, emLen, rLen);
  }

  // Returns execution size of the instruction
  iga::ExecSize getExecutionSize(int32_t pc) const {
    return static_cast<iga::ExecSize>(kv_get_execution_size(m_kv, pc));
  }

  iga::SWSB getSWSBInfo(int32_t pc, iga::SWSB_ENCODE_MODE encdoe_mode) const {
    iga::SWSB swsb;
    kv_get_swsb_info(m_kv, pc, encdoe_mode, swsb);
    return swsb;
  }

  // Returns opcode of the instruction
  //
  // N.B.: the opcode is not the same as the 7-bit value encoded in the
  // actual encoding, but maps to the enumeration value of the underlyings
  // iga::Op.
  iga::Op getOpcode(int32_t pc) const {
    return static_cast<iga::Op>(kv_get_opcode(m_kv, pc));
  }

  // Returns the subfunction (iga::Subfunction) of the given operation.
  // E.g. for "math.exp" (iga::Op::MATH) this returns
  //   static_cast<uint32_t>(iga::MathFC::EXP);
  //
  // For operations that do not support a subfunction this returns
  // (uint32_t)-1.  For invalid PC's this also returns that value.
  //
  // Example usage:
  //   if (getOpcode(pc) == iga::Op::MATH) {
  //      iga::MathFC mfc = static_cast<iga::MathFC>(getSubfunction(pc));
  //      ...
  //   }
  iga::Subfunction getSubfunction(int32_t pc) const {
    iga::Subfunction sf;
    kv_get_subfunction(m_kv, pc, &sf.bits);
    return sf;
  }

  // Returns the Execution Mask Offset for the given PC
  iga::ChannelOffset getChannelOffset(int32_t pc) const {
    return static_cast<iga::ChannelOffset>(kv_get_channel_offset(m_kv, pc));
  }

  // Returns Mask Control
  iga::MaskCtrl getMaskCtrl(int32_t pc) const {
    return static_cast<iga::MaskCtrl>(kv_get_mask_control(m_kv, pc));
  }

  int32_t getHasDestination(int32_t pc) const {
    return kv_get_has_destination(m_kv, pc);
  }
  // Returns Register Number Row of destination register.
  // i.e for r23.5 returns 23
  int32_t getDstRegNumber(int32_t pc) const {
    return kv_get_destination_register(m_kv, pc);
  }

  // Returns sub-register number in a register row.
  // The subregister is scaled by the type just like in syntactic form.
  // So r32.5:f is 5*sizeof(:f) = 20 bytes into the 32-byte register.
  //
  // i.e. for r32.5 returns 5
  int32_t getDstSubRegNumber(int32_t pc) const {
    return kv_get_destination_sub_register(m_kv, pc);
  }

  // Returns destination data Type
  // i.e. F, HF, INT, UINT, etc
  iga::Type getDstDataType(int32_t pc) const {
    return static_cast<iga::Type>(kv_get_destination_data_type(m_kv, pc));
  }

  // Returns destination register Type
  // i.e. GRF, NULL, ACC, etc
  iga::RegName getDstRegType(int32_t pc) const {
    return static_cast<iga::RegName>(
        kv_get_destination_register_type(m_kv, pc));
  }

  // Returns destination Kind
  // i.e. DIRECT, INDIRECT, IMM
  iga::Kind getDstRegKind(int32_t pc) const {
    return static_cast<iga::Kind>(kv_get_destination_register_kind(m_kv, pc));
  }

  // Returns Register Number Row of source register.
  // valid range [0,127]
  // i.e for r23.5 returns 23
  int32_t getSrcRegNumber(int32_t pc, uint32_t sourceNumber) const {
    return kv_get_source_register(m_kv, pc, sourceNumber);
  }

  // Returns sub register number in a register row.
  // valid range [0, 31]
  // i.e. for r32.5 returns 5
  int32_t getSrcSubRegNumber(int32_t pc, uint32_t sourceNumber) const {
    return kv_get_source_sub_register(m_kv, pc, sourceNumber);
  }

  // Returns source data Type
  // i.e. F, HF, INT, UINT, etc
  iga::Type getSrcDataType(int32_t pc, uint32_t sourceNumber) const {
    return static_cast<iga::Type>(
        kv_get_source_data_type(m_kv, pc, sourceNumber));
  }

  // Returns source register Type
  // i.e. GRF, NULL, ACC, etc
  iga::RegName getSrcRegType(int32_t pc, uint32_t sourceNumber) const {
    return static_cast<iga::RegName>(
        kv_get_source_register_type(m_kv, pc, sourceNumber));
  }

  // Returns source Kind
  // i.e. DIRECT, INDIRECT, IMM
  iga::Kind getSrcRegKind(int32_t pc, uint32_t sourceNumber) const {
    return static_cast<iga::Kind>(
        kv_get_source_register_kind(m_kv, pc, sourceNumber));
  }

  // Returns source Indirect Immediate Offset
  int16_t getSrcRegIndirectImmOff(int32_t pc, uint32_t sourceNumber) const {
    int16_t result = 0;
    kv_get_source_indirect_imm_off(m_kv, pc, sourceNumber, &result);
    return result;
  }

  // Returns destination Indirect Immediate Offset
  int16_t getDstRegIndirectImmOff(int32_t pc) const {
    int16_t result = 0;
    kv_get_destination_indirect_imm_off(m_kv, pc, &result);
    return result;
  }

  // Returns source mme number
  int16_t getSrcMMENumber(int32_t pc, uint32_t sourceNumber) const {
    int16_t result = 0;
    kv_get_source_mme_number(m_kv, pc, sourceNumber, &result);
    return result;
  }

  // Returns destination mme number
  int16_t getDstMMENumber(int32_t pc) const {
    int16_t result = 0;
    kv_get_destination_mme_number(m_kv, pc, &result);
    return result;
  }

  // Returns if source is a vector
  // -1 - ERROR
  // 0  - FALSE
  // 1  - TRUE
  int32_t getIsSrcVector(int32_t pc, uint32_t sourceNumber) const {
    return kv_is_source_vector(m_kv, pc, sourceNumber);
  }

  // Returns 0 if instruction's destination operand horizontal stride
  // (DstRgnHz) is succesfully returned.
  // Otherwise returns -1.
  // DstRgnHz is returned by *hz parameter, as numeric numeric value
  // (e.g. 1,2,4).
  int32_t getDstRegion(int32_t pc, uint32_t *hz) const {
    return kv_get_destination_region(m_kv, pc, hz);
  }

  // Returns 0 if any of instruction's src operand region components
  // (Src RgnVt, RgnWi, RgnHz) are succesfully determined.
  // Otherwise returns -1.
  //
  // Vt, Wi and Hz are returned by *vt, *wi and *hz parameter,
  // as numeric numeric values (e.g. 1,2,4).
  int32_t getSrcRegion(int32_t pc, uint32_t src_op, uint32_t *vt, uint32_t *wi,
                       uint32_t *hz) const {
    return kv_get_source_region(m_kv, pc, src_op, vt, wi, hz);
  }

  // Returns 0 if the source is an immediate and sets the 64b immediate value
  // in imm; otherwise it returns -1.
  //
  // 16b and 32b immediate will stored in the lower bits of imm.
  int32_t getSrcImmediate(int32_t pc, uint32_t src_op, uint64_t *imm) const {
    return kv_get_source_immediate(m_kv, pc, src_op, imm);
  }

  // Return the flag predicate mode
  iga::PredCtrl getPredicate(int32_t pc) const {
    return static_cast<iga::PredCtrl>(kv_get_predicate(m_kv, pc));
  }

  // Return if inverse predicate is on or not
  bool isInversePredicate(int32_t pc) const {
    return static_cast<bool>(kv_get_is_inverse_predicate(m_kv, pc));
  }

  // Return the flag register
  int32_t getFlagReg(int32_t pc) const {
    return kv_get_flag_register(m_kv, pc);
  }

  // Return the flag sub register
  int32_t getFlagSubReg(int32_t pc) const {
    return kv_get_flag_sub_register(m_kv, pc);
  }

private:
  // disable value assignment
  KernelView(const KernelView &) {}
  KernelView &operator=(const KernelView &) { return *this; }
};

inline kv_status_t KernelView::getMessageType(int32_t pc,
                                              iga::SFMessageType &sfmt) const {
  if (m_disasm_status != IGA_SUCCESS)
    return kv_status_t::KV_DECODE_ERROR;
  int32_t val = 0;
  kv_status_t s = kv_get_message_type(m_kv, pc, &val);
  sfmt = static_cast<iga::SFMessageType>(val);
  return s;
}

inline kv_status_t KernelView::getMessageSFID(int32_t pc,
                                              iga::SFID &sfid) const {
  if (m_disasm_status != IGA_SUCCESS)
    return kv_status_t::KV_DECODE_ERROR;
  int32_t val = 0;
  kv_status_t s = kv_get_message_sfid(m_kv, pc, &val);
  sfid = static_cast<iga::SFID>(val);
  return s;
}

inline kv_status_t KernelView::getCacheOpt(int32_t pc,
                                           iga::CacheLevel cahceLevel,
                                           iga::CacheOpt &cacheOpt) const {
  if (m_disasm_status != IGA_SUCCESS)
    return kv_status_t::KV_DECODE_ERROR;
  int32_t val = 0;
  kv_status_t s =
      kv_get_cache_opt(m_kv, pc, static_cast<int32_t>(cahceLevel), &val);
  cacheOpt = static_cast<iga::CacheOpt>(val);
  return s;
}

#endif
