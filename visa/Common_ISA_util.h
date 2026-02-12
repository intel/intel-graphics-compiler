/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef COMMON_ISA_UTIL_INCLUDED
#define COMMON_ISA_UTIL_INCLUDED

/*  Utility functions for common ISA binary emission
 *
 */
#include "Assertions.h"
#include "Common_ISA.h"
#include "G4_IR.hpp"
#include "visa_igc_common_header.h"
#include <string_view>

const char *Common_ISA_Get_Align_Name(VISA_Align);
uint32_t getAlignInBytes(VISA_Align A, unsigned grfSize);
VISA_Align getCISAAlign(uint32_t AlignInBytes);
const char *Common_ISA_Get_Modifier_Name(VISA_Modifier);
G4_opcode GetGenOpcodeFromVISAOpcode(ISA_Opcode);
VISA_Type Get_Common_ISA_Type_From_G4_Type(G4_Type);
G4_Type GetGenTypeFromVISAType(VISA_Type);
G4_SrcModifier GetGenSrcModFromVISAMod(VISA_Modifier);
G4_CondModifier Get_G4_CondModifier_From_Common_ISA_CondModifier(VISA_Cond_Mod);
bool hasPredicate(ISA_Opcode op);
bool hasExecSize(ISA_Opcode op, uint8_t subOp = 0);
bool hasLabelSrc(ISA_Opcode op);
unsigned Get_Common_ISA_SVM_Block_Num(VISA_SVM_Block_Num);
VISA_SVM_Block_Num valueToVISASVMBlockNum(unsigned int);
unsigned Get_Common_ISA_SVM_Block_Size(VISA_SVM_Block_Type);
VISA_SVM_Block_Type valueToVISASVMBlockType(unsigned int);
unsigned Get_VISA_Oword_Num(VISA_Oword_Num);
unsigned Get_VISA_Exec_Size(VISA_Exec_Size);
bool IsMathInst(ISA_Opcode op);
bool IsIntType(VISA_Type);
bool IsIntOrIntVecType(VISA_Type);
bool IsSingedIntType(VISA_Type);
bool IsUnsignedIntType(VISA_Type);
unsigned short Get_Common_ISA_Region_Value(Common_ISA_Region_Val);
unsigned short Create_CISA_Region(unsigned short vstride, unsigned short width,
                                  unsigned short hstride);
unsigned Round_Up_Pow2(unsigned n);
unsigned Round_Down_Pow2(unsigned n);
G4_opcode Get_Pseudo_Opcode(ISA_Opcode op);
VISA_EMask_Ctrl Get_Next_EMask(VISA_EMask_Ctrl currEMask, G4_ExecSize execSize);
G4_InstOpts Get_Gen4_Emask(VISA_EMask_Ctrl cisa_emask, G4_ExecSize exec_size);
unsigned Get_Atomic_Op(VISAAtomicOps op);
uint16_t Get_VISA_Type_Size(VISA_Type type);
Common_ISA_Region_Val Get_CISA_Region_Val(short val);
short Common_ISA_Get_Region_Value(Common_ISA_Region_Val val);
VISA_Cond_Mod
Get_Common_ISA_CondModifier_From_G4_CondModifier(G4_CondModifier cmod);
VISA_Exec_Size Get_VISA_Exec_Size_From_Raw_Size(unsigned int size);
VISA_Oword_Num Get_VISA_Oword_Num_From_Number(unsigned num);
VISA_Modifier Get_Common_ISA_SrcMod_From_G4_Mod(G4_SrcModifier mod);
G4_Type getUnsignedType(unsigned short numByte);
G4_Type getSignedType(unsigned short numByte);

inline uint32_t getVersionAsInt(uint32_t major, uint32_t minor) {
  return major * 100 + minor;
}

inline unsigned int Get_CISA_PreDefined_Var_Count() {
  return COMMON_ISA_NUM_PREDEFINED_VAR_VER_3;
}

const char *createStringCopy(std::string_view name, vISA::Mem_Manager &m_mem);

std::string sanitizePathString(std::string str);
std::string sanitizeLabelString(std::string str);

inline unsigned int Get_CISA_PreDefined_Surf_Count() {
  return COMMON_ISA_NUM_PREDEFINED_SURF_VER_3_1;
}

#define SEND_GT_MSG_LENGTH_BIT_OFFSET 25
#define SEND_GT_RSP_LENGTH_BIT_OFFSET 20
#define SEND_GT_MSG_HEADER_PRESENT_BIT_OFFSET 19

inline unsigned getSendRspLengthBitOffset() {
  return SEND_GT_RSP_LENGTH_BIT_OFFSET;
}

inline unsigned getSendMsgLengthBitOffset() {
  return SEND_GT_MSG_LENGTH_BIT_OFFSET;
}

inline unsigned getSendHeaderPresentBitOffset() {
  return SEND_GT_MSG_HEADER_PRESENT_BIT_OFFSET;
}

VISA_Type getRawOperandType(const print_format_provider_t *header,
                            const raw_opnd &opnd);
VISA_Type getVectorOperandType(const print_format_provider_t *header,
                               const vector_opnd &opnd);

template <typename T> T getPrimitiveOperand(const CISA_INST *inst, unsigned i) {
  vISA_ASSERT(inst, "Argument Exception: argument inst is NULL.");
  vISA_ASSERT(inst->opnd_array, "Operand array is NULL");
  vISA_ASSERT(inst->opnd_num > i,
               "No such operand, i, for instruction inst.");
  vISA_ASSERT((T)inst->opnd_array[i]->_opnd.other_opnd ==
                   inst->opnd_array[i]->_opnd.other_opnd,
               "Mismatched value.");
  return (T)inst->opnd_array[i]->_opnd.other_opnd;
}

const raw_opnd &getRawOperand(const CISA_INST *inst, unsigned i);

bool isNullRawOperand(const CISA_INST *inst, unsigned i);
bool isNotNullRawOperand(const CISA_INST *inst, unsigned i);

const vector_opnd &getVectorOperand(const CISA_INST *inst, unsigned i);

CISA_opnd_type getOperandType(const CISA_INST *inst, unsigned i);

int64_t typecastVals(const void *value, VISA_Type isaType);

int Get_PreDefined_Surf_Index(int index, TARGET_PLATFORM platform);

inline bool isShiftOp(ISA_Opcode op) {
  return op == ISA_SHL || op == ISA_SHR || op == ISA_ASR || op == ISA_ROL ||
         op == ISA_ROR;
}

inline uint32_t getvISAMaskOffset(VISA_EMask_Ctrl emask) {
  switch (emask) {
  case vISA_EMASK_M1:
  case vISA_EMASK_M1_NM:
    return 0;
  case vISA_EMASK_M2:
  case vISA_EMASK_M2_NM:
    return 4;
  case vISA_EMASK_M3:
  case vISA_EMASK_M3_NM:
    return 8;
  case vISA_EMASK_M4:
  case vISA_EMASK_M4_NM:
    return 12;
  case vISA_EMASK_M5:
  case vISA_EMASK_M5_NM:
    return 16;
  case vISA_EMASK_M6:
  case vISA_EMASK_M6_NM:
    return 20;
  case vISA_EMASK_M7:
  case vISA_EMASK_M7_NM:
    return 24;
  case vISA_EMASK_M8:
  case vISA_EMASK_M8_NM:
    return 28;
  default:
    vISA_ASSERT_UNREACHABLE("illegal vISA execution mask control");
    return 0;
  }
}

inline bool isNoMask(VISA_EMask_Ctrl eMask) {
  switch (eMask) {
  case vISA_EMASK_M1_NM:
  case vISA_EMASK_M2_NM:
  case vISA_EMASK_M3_NM:
  case vISA_EMASK_M4_NM:
  case vISA_EMASK_M5_NM:
  case vISA_EMASK_M6_NM:
  case vISA_EMASK_M7_NM:
  case vISA_EMASK_M8_NM:
    return true;
  default:
    break;
  }
  return false;
}

const char *toString(GenPrecision P);

inline uint32_t DpasInfoToUI32(GenPrecision A, GenPrecision W, uint8_t D,
                               uint8_t C) {
  uint32_t info = (C & 0xFF);
  info = (info << 8) | ((uint32_t)D & 0xFF);
  info = (info << 8) | ((uint32_t)A & 0xFF);
  info = (info << 8) | ((uint32_t)W & 0xFF);
  return info;
}

inline void UI32ToDpasInfo(const uint32_t dpasInfo, GenPrecision &A,
                           GenPrecision &W, uint8_t &D, uint8_t &C) {
  W = (GenPrecision)(dpasInfo & 0xFF);
  A = (GenPrecision)((dpasInfo >> 8) & 0xFF);
  D = ((dpasInfo >> 16) & 0xFF);
  C = ((dpasInfo >> 24) & 0xFF);
}

inline bool isPerSampleSet(uint16_t mode) { return (mode & (0x1 << 0x0)); }

inline bool isSampleIndexSet(uint16_t mode) { return (mode & (0x1 << 0x1)); }

bool strEndsWith(const std::string &str, const std::string &suffix);
bool strStartsWith(const std::string &str, const std::string &prefix);

// utilities converting load/store cache control enum (LSC_L1_L3_CC) to
// LSC_CACHE_OPTS.
LSC_CACHE_OPTS convertLSCLoadStoreCacheControlEnum(LSC_L1_L3_CC L1L3cc,
                                                   bool isLoad);

namespace vISA {
std::tuple<Caching,Caching,Caching> ToLdCaching(LSC_L1_L3_CC);
std::tuple<Caching,Caching,Caching> ToStCaching(LSC_L1_L3_CC);
std::tuple<Caching,Caching,Caching> ToAtCaching(LSC_L1_L3_CC);

// Utility function for allocating memory for finalizer output (e.g., kernel
// binary, debug info), which may have longer lifetime than the vISA builder.
// It is the caller's responsiblity to free such resources.
void *allocCodeBlock(size_t sz);

inline bool hasOV(LSC_SFID sfid, LSC_OP op) {
  if ((sfid == LSC_UGM || sfid == LSC_URB) &&
      (op == LSC_LOAD || op == LSC_LOAD_QUAD || op == LSC_LOAD_STATUS))
    return true;
  return false;
}

inline bool hasMSAA(LSC_OP op) {
  if (op >= LSC_OP::LSC_ATOMIC_IINC && op <= LSC_OP::LSC_ATOMIC_XOR)
    return true;
  return false;
}
}

#endif /* COMMON_ISA_UTIL_INCLUDED */
