/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _BINARYENCODING_H_
#define _BINARYENCODING_H_

#include <fstream>
#include <string>

#include "Common_BinaryEncoding.h"
#include "FlowGraph.h"

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////

typedef enum _AccessMode_ {
  ACCESS_MODE_ALIGN1,
  ACCESS_MODE_ALIGN16
} AccessMode;

typedef enum _predicate_state_ {
  PREDICATE_STATE_NORMAL,
  PREDICATE_STATE_INVERT
} PredicateState;

typedef enum _predicate_ {
  PREDICATE_OFF,
  PREDICATE_ALIGN16_SEQUENTIAL,
  PREDICATE_ALIGN16_REP_X,
  PREDICATE_ALIGN16_REP_Y,
  PREDICATE_ALIGN16_REP_Z,
  PREDICATE_ALIGN16_REP_W,
  PREDICATE_ALIGN16_ANY4H,
  PREDICATE_ALIGN16_ALL4H,

  PREDICATE_ALIGN1_SEQUENTIAL = 1,
  PREDICATE_ALIGN1_ANYV,
  PREDICATE_ALIGN1_ALLV,
  PREDICATE_ALIGN1_ANY2H,
  PREDICATE_ALIGN1_ALL2H,
  PREDICATE_ALIGN1_ANY4H,
  PREDICATE_ALIGN1_ALL4H,
  PREDICATE_ALIGN1_ANY8H,
  PREDICATE_ALIGN1_ALL8H,
  PREDICATE_ALIGN1_ANY16H,
  PREDICATE_ALIGN1_ALL16H,
  // valid for Gen7 only
  PREDICATE_ALIGN1_ANY32H,
  PREDICATE_ALIGN1_ALL32H
} Predicate;

typedef enum _ConditionCodes_ {
  COND_CODE_NONE,
  COND_CODE_Z,
  COND_CODE_NZ,
  COND_CODE_G,
  COND_CODE_GE,
  COND_CODE_L,
  COND_CODE_LE,
  COND_CODE_C,
  COND_CODE_O,
  COND_CODE_ANY,
  COND_CODE_ALL
} ConditionCodes;

typedef enum _QtrCtrl_ {
  QTR_CTRL_1Q,
  QTR_CTRL_2Q,
  QTR_CTRL_3Q,
  QTR_CTRL_4Q
} QtrCtrl;

typedef enum _ComprCtrl_ { COMPR_CTRL_NORMAL, COMPR_CTRL_COMPRESSED } ComprCtrl;

// GT
typedef enum _CmptCtrl_ { CMPT_CTRL_NORMAL, CMPT_CTRL_COMPACTED } CmptCtrl;

///////////////////////////////////////////////////////////////////////////////
// Gen7 specific fields
///////////////////////////////////////////////////////////////////////////////
typedef enum _ThreeSrcType_ {
  THREE_SRC_TYPE_F = 0,
  THREE_SRC_TYPE_D = 1,
  THREE_SRC_TYPE_UD = 2,
  THREE_SRC_TYPE_DF = 3,
  THREE_SRC_TYPE_HF = 4,
} ThreeSrcType;

typedef enum _NibCtrl_ { NIB_FALSE = 0, NIB_TRUE = 1 } NibCtrl;

///////////////////////////////////////////////////////////////////////////////
// End of Gen7 specific fields
///////////////////////////////////////////////////////////////////////////////
typedef enum _threadControl_ {
  THREAD_CTRL_NORMAL,
  THREAD_CTRL_ATOMIC,
  THREAD_CTRL_SWITCH
} ThreadCtrl;

typedef enum _CInstModifier_ { INST_MOD_NONE, INST_MOD_SAT } InstModifier;

typedef enum _DepCtrl_ {
  DEP_CTRL_NORMAL,
  DEP_CTRL_DIS_CLEAR,
  DEP_CTRL_DIS_CHECK,
  DEP_CTRL_DIS_CHECK_CLEAR_DEST
} DepCtrl;

typedef enum _SrcMod_ {
  SRC_MOD_NONE,
  SRC_MOD_ABSOLUTE,
  SRC_MOD_NEGATE,
  SRC_MOD_NEGATE_OF_ABSOLUTE
} SrcMod;

typedef enum _SrcType_ {
  SRC_TYPE_UD,
  SRC_TYPE_D,
  SRC_TYPE_UW,
  SRC_TYPE_W,
  SRC_TYPE_UB,
  SRC_TYPE_B,
  SRC_TYPE_DF,
  SRC_TYPE_F,
  SRC_TYPE_UQ = 8,
  SRC_TYPE_Q = 9,
  SRC_TYPE_HF = 10,
  SRC_TYPE_UNDEF = 0xFFFFFFFF
} SrcType;

typedef enum _SrcImmType_ {
  SRC_IMM_TYPE_UD,
  SRC_IMM_TYPE_D,
  SRC_IMM_TYPE_UW,
  SRC_IMM_TYPE_W,
  SRC_IMM_TYPE_UV,
  SRC_IMM_TYPE_VF,
  SRC_IMM_TYPE_V,
  SRC_IMM_TYPE_F,
  SRC_IMM_TYPE_UQ = 8,
  SRC_IMM_TYPE_Q = 9,
  SRC_IMM_TYPE_DF = 10,
  SRC_IMM_TYPE_HF = 11,
  SRC_IMM_TYPE_UNDEF = 0xFFFFFFFF
} SrcImmType;

const int aSrcImmType[] = {
    // byte counts
    4, // UD
    4, // D
    2, // UW
    2, // W
    4, // UV
    4, // VF
    4, // V
    4  // F
};

typedef enum _DstType_ {
  DST_TYPE_UD,
  DST_TYPE_D,
  DST_TYPE_UW,
  DST_TYPE_W,
  DST_TYPE_UB,
  DST_TYPE_B,
  DST_TYPE_DF,
  DST_TYPE_F = 7,
  DST_TYPE_UQ = 8,
  DST_TYPE_Q = 9,
  DST_TYPE_HF = 10, // for half float
  DST_TYPE_UNDEF = 0xFFFFFFFF
} DstType;

typedef enum _IdxType_ { IDX_TYPE_D, IDX_TYPE_W } IdxType;

typedef enum _VertStride_ {
  VERT_STRIDE_0,
  VERT_STRIDE_1,
  VERT_STRIDE_2,
  VERT_STRIDE_4,
  VERT_STRIDE_8,
  VERT_STRIDE_16,
  VERT_STRIDE_32,
  VERT_STRIDE_ONE_DIMEN = 15
} EncVertStride,
    *pEncVertStride;

typedef enum _Width_ {
  WIDTH_1,
  WIDTH_2,
  WIDTH_4,
  WIDTH_8,
  WIDTH_16
} EncWidth,
    *pEncWidth;

typedef enum _HorzStride_ {
  HORZ_STRIDE_0,
  HORZ_STRIDE_1,
  HORZ_STRIDE_2,
  HORZ_STRIDE_4
} EncHorzStride,
    *pEncHorzStride;

const uint32_t VERT_STRIDE_VxH = 0xFFFFFFFF;

///////////////////////////////////////////////////
//// 32 bit message descriptor in send instruction
///////////////////////////////////////////////////
// typedef struct _sEncMsgDescriptor_
//{
//     unsigned short  ResponseLength  : 4,
//                     MessageLength   : 4,
//                     TargetUnitId    : 6,
//                     Reserved        : 1,
//                     EndOfThread     : 1;
// } * psEncMsgDescriptor, sEncMsgDescriptor;

/////////////////////////////////////////////////
// 6 bit extended message descriptor in send instruction
/////////////////////////////////////////////////
typedef struct _sEncExtMsgDescriptor_ {
  uint32_t TargetUnitId : 4, Reserved : 1, EndOfThread : 1,
      ExtMessageLength : 4, Reserved2 : 1, CPSLODCompensation : 1,
      Reserved3 : 4, ExtFunctionControl : 16;
} *psEncExtMsgDescriptor, sEncExtMsgDescriptor;
//
// typedef union _EncMsgDescriptor_
//{
//    uint32_t        ulData;
//    sEncMsgDescriptor   MsgDescriptor;
//} * pEncMsgDescriptor, EncMsgDescriptor;

typedef union _EncExtMsgDescriptor_ {
  uint32_t ulData;
  sEncExtMsgDescriptor ExtMsgDescriptor;
} *pEncExtMsgDescriptor, EncExtMsgDescriptor;

////////////////////////////////////////////////
// 128 bit ISA instruction
// Version 0.75
////////////////////////////////////////////////

#define bitsPredicate_0 20
#define bitsPredicate_1 16
#define bitsThreadCtrl_0 15
#define bitsThreadCtrl_1 14
#define bitsQtrCtrl_0 13
#define bitsQtrCtrl_1 12
#define bitsComprCtrl_0 13
#define bitsComprCtrl_1 12

#define bitsCompactCtrl_0 29
#define bitsCompactCtrl_1 29

#define bitsAccessMode_0 8
#define bitsAccessMode_1 8
#define bitsOpCode_0 6
#define bitsOpCode_1 0
#define bitsInstModifier_0 31
#define bitsInstModifier_1 31
#define bitsExecSize_0 23
#define bitsExecSize_1 21

#define bitsDstAddrMode_0 63
#define bitsDstAddrMode_1 63
#define bitsDstHorzStride_0 62
#define bitsDstHorzStride_1 61
#define bitsDstRegNumOWord_0 60
#define bitsDstRegNumOWord_1 52
#define bitsDstRegNumByte_0 60
#define bitsDstRegNumByte_1 48
#define bitsDstChanEn_0 51
#define bitsDstChanEn_1 48

#define bitsDstArchRegFile_0 60
#define bitsDstArchRegFile_1 57
#define bitsDstArchRegNum_0 56
#define bitsDstArchRegNum_1 53
#define bitsDstArchSubRegNumOWord_0 52
#define bitsDstArchSubRegNumOWord_1 52
#define bitsDstArchSubRegNumWord_0 52
#define bitsDstArchSubRegNumWord_1 49
#define bitsDstArchSubRegNumByte_0 52
#define bitsDstArchSubRegNumByte_1 48
#define bitsDstImm16_0 63
#define bitsDstImm16_1 48

#define bitsSrcAddrMode_0 79
#define bitsSrcAddrMode_1 79
#define bitsSrcAddrMode_2 111
#define bitsSrcAddrMode_3 111
#define bitsSrcSrcMod_0 78
#define bitsSrcSrcMod_1 77
#define bitsSrcSrcMod_2 110
#define bitsSrcSrcMod_3 109
#define bitsSrcRegNumOWord_0 76
#define bitsSrcRegNumOWord_1 68
#define bitsSrcRegNumOWord_2 108
#define bitsSrcRegNumOWord_3 100
#define bitsSrcRegNumByte_0 76
#define bitsSrcRegNumByte_1 64
#define bitsSrcRegNumByte_2 108
#define bitsSrcRegNumByte_3 96
#define bitsSrcChanSel_0_0 65
#define bitsSrcChanSel_0_1 64
#define bitsSrcChanSel_0_2 97
#define bitsSrcChanSel_0_3 96
#define bitsSrcChanSel_1_0 67
#define bitsSrcChanSel_1_1 66
#define bitsSrcChanSel_1_2 99
#define bitsSrcChanSel_1_3 98
#define bitsSrcChanSel_2_0 81
#define bitsSrcChanSel_2_1 80
#define bitsSrcChanSel_2_2 113
#define bitsSrcChanSel_2_3 112
#define bitsSrcChanSel_3_0 83
#define bitsSrcChanSel_3_1 82
#define bitsSrcChanSel_3_2 115
#define bitsSrcChanSel_3_3 114
#define bitsSrcVertStride_0 88
#define bitsSrcVertStride_1 85
#define bitsSrcVertStride_2 120
#define bitsSrcVertStride_3 117
#define bitsSrcWidth_0 84
#define bitsSrcWidth_1 82
#define bitsSrcWidth_2 116
#define bitsSrcWidth_3 114
#define bitsSrcHorzStride_0 81
#define bitsSrcHorzStride_1 80
#define bitsSrcHorzStride_2 113
#define bitsSrcHorzStride_3 112

#define bitsSrcArchRegFile_0 76
#define bitsSrcArchRegFile_1 73
#define bitsSrcArchRegFile_2 108
#define bitsSrcArchRegFile_3 105
#define bitsSrcArchRegNum_0 72
#define bitsSrcArchRegNum_1 69
#define bitsSrcArchRegNum_2 104
#define bitsSrcArchRegNum_3 101
#define bitsSrcArchSubRegNumOWord_0 68
#define bitsSrcArchSubRegNumOWord_1 68
#define bitsSrcArchSubRegNumOWord_2 100
#define bitsSrcArchSubRegNumOWord_3 100
#define bitsSrcArchSubRegNumWord_0 68
#define bitsSrcArchSubRegNumWord_1 65
#define bitsSrcArchSubRegNumWord_2 100
#define bitsSrcArchSubRegNumWord_3 97
#define bitsSrcArchSubRegNumByte_0 68
#define bitsSrcArchSubRegNumByte_1 64
#define bitsSrcArchSubRegNumByte_2 100
#define bitsSrcArchSubRegNumByte_3 96

#define bitsSharedFunctionID_0 27
#define bitsSharedFunctionID_1 24

#define bitsExMsgLength_0 67
#define bitsExMsgLength_1 64
// CNL uses bit 31 to encode this flag.
#define bitsExDescCPSLOD_0 31
#define bitsExDescCPSLOD_1 31
// SKL+ extended message descriptor
#define bitsSendExDesc16 64
#define bitsSendExDesc19 67
#define bitsSendExDesc20 80
#define bitsSendExDesc23 83
#define bitsSendExDesc24 85
#define bitsSendExDesc27 88
#define bitsSendExDesc28 91
#define bitsSendExDesc31 94

#define bitsSendsExDescFuncCtrl_0 95
#define bitsSendsExDescFuncCtrl_1 80

#define bitsMsgDescriptor_0 127
#define bitsMsgDescriptor_1 96
#define bitsMsgDescriptorImm_0 126
#define bitsMsgDescriptorImm_1 96
#define bitsMsgDescriptorReg_0 120
#define bitsMsgDescriptorReg_1 96
#define bitsEndOfThread_0 127
#define bitsEndOfThread_1 127
#define bitsSendDesc_29 125
#define bitsSendDesc_30 126

#define bitsCpu_0 47
#define bitsCpu_1 47
#define bitsMathFunction_0 27
#define bitsMathFunction_1 24
#define bitsMathPartPrec_0 14
#define bitsMathPartPrec_1 14

#define bitsSrcImm64_0 127
#define bitsSrcImm64_1 96
#define bitsSrcImm64_2 95
#define bitsSrcImm64_3 64

// for SKL+ send instruction
#define bitsNoSrcDepSet_0 28
#define bitsNoSrcDepSet_1 28

////////////////////////////////////////////////
// 3 Source ISA instruction
// Version 0.0
////////////////////////////////////////////////

// DW0 [31:0] same as regular ISA
#define bits3SrcDstChanEn_0 52
#define bits3SrcDstChanEn_1 49
#define bits3SrcDstRegNumOWord_0 63
#define bits3SrcDstRegNumOWord_1 55
#define bits3SrcDstRegNumDWord_0 63
#define bits3SrcDstRegNumDWord_1 53

//                                                       src0      src1 src2
#define bits3SrcRepCtrl_0 64
#define bits3SrcRepCtrl_1 64
#define bits3SrcRepCtrl_2 85
#define bits3SrcRepCtrl_3 85
#define bits3SrcRepCtrl_4 106
#define bits3SrcRepCtrl_5 106

// Swizzle controls
#define bits3SrcSwizzle_0 72
#define bits3SrcSwizzle_1 65
#define bits3SrcSwizzle_2 93
#define bits3SrcSwizzle_3 86
#define bits3SrcSwizzle_4 114
#define bits3SrcSwizzle_5 107

#define bits3SrcChanSel_0_0 66
#define bits3SrcChanSel_0_1 65
#define bits3SrcChanSel_0_2 87
#define bits3SrcChanSel_0_3 86
#define bits3SrcChanSel_0_4 108
#define bits3SrcChanSel_0_5 107

#define bits3SrcChanSel_1_0 68
#define bits3SrcChanSel_1_1 67
#define bits3SrcChanSel_1_2 89
#define bits3SrcChanSel_1_3 88
#define bits3SrcChanSel_1_4 110
#define bits3SrcChanSel_1_5 109

#define bits3SrcChanSel_2_0 70
#define bits3SrcChanSel_2_1 69
#define bits3SrcChanSel_2_2 91
#define bits3SrcChanSel_2_3 90
#define bits3SrcChanSel_2_4 112
#define bits3SrcChanSel_2_5 111

#define bits3SrcChanSel_3_0 72
#define bits3SrcChanSel_3_1 71
#define bits3SrcChanSel_3_2 93
#define bits3SrcChanSel_3_3 92
#define bits3SrcChanSel_3_4 114
#define bits3SrcChanSel_3_5 113

#define bits3SrcSrcRegNumHWord_4 125
#define bits3SrcSrcRegNumHWord_5 118

#define bits3SrcSrcRegNumOWord_0 83
#define bits3SrcSrcRegNumOWord_1 75
#define bits3SrcSrcRegNumOWord_2 104
#define bits3SrcSrcRegNumOWord_3 96
#define bits3SrcSrcRegNumOWord_4 125
#define bits3SrcSrcRegNumOWord_5 117

// Get/Setbits cannot cross 32 bit boundary
#define bits3SrcSrcRegNumDWord_0 83
#define bits3SrcSrcRegNumDWord_1 73
#define bits3SrcSrcRegNumDWord_2 104
#define bits3SrcSrcRegNumDWord_3 96
#define bits3SrcSrcRegNumDWord_4 125
#define bits3SrcSrcRegNumDWord_5 115
#define bits3SrcSrcRegNumDWord_6 95
#define bits3SrcSrcRegNumDWord_7 94

#define bits3SrcSrc0RegDWord_L 73
#define bits3SrcSrc0RegDWord_H 83
#define bits3SrcSrc1RegDWord1_L 96
#define bits3SrcSrc1RegDWord1_H 104
#define bits3SrcSrc1RegDWord2_L 94
#define bits3SrcSrc1RegDWord2_H 95
#define bits3SrcSrc2RegDWord_L 115
#define bits3SrcSrc2RegDWord_H 125

#define bits3SrcSrc0SubRegNumW_L 84
#define bits3SrcSrc0SubRegNumW_H 84
#define bits3SrcSrc1SubRegNumW_L 105
#define bits3SrcSrc1SubRegNumW_H 105
#define bits3SrcSrc2SubRegNumW_L 126
#define bits3SrcSrc2SubRegNumW_H 126

// various bits for split send
#define bitsSendsSrc1RegFile_0 36
#define bitsSendsSrc1RegFile_1 36
#define bitsSendsSrc1AddrImmSign_0 41
#define bitsSendsSrc1AddrImmSign_1 41
#define bitsSendsSrc1AddrMode_0 42
#define bitsSendsSrc1AddrMode_1 42
#define bitsSendsSrc1AddrImm8_4_0 47
#define bitsSendsSrc1AddrImm8_4_1 43
#define bitsSendsSrc1RegNum_0 51
#define bitsSendsSrc1RegNum_1 44
#define bitsSendsSrc1AddrSubRegNum_0 51
#define bitsSendsSrc1AddrSubRegNum_1 48

#define bitsSendsDstRegFile_0 35
#define bitsSendsDstRegFile_1 35
#define bitsSendsDstSubRegNum_0 52
#define bitsSendsDstSubRegNum_1 52
#define bitsSendsDstAddrImm8_4_0 56
#define bitsSendsDstAddrImm8_4_1 52
#define bitsSendsDstRegNum_0 60
#define bitsSendsDstRegNum_1 53
#define bitsSendsDstAddrSubRegNum_0 60
#define bitsSendsDstAddrSubRegNum_1 57
#define bitsSendsDstAddrImmSign_0 62
#define bitsSendsDstAddrImmSign_1 62
#define bitsSendsDstAddrMode_0 63
#define bitsSendsDstAddrMode_1 63

#define bitsSendsSelReg32Desc_0 77
#define bitsSendsSelReg32Desc_1 77

#define bitsSendsSelReg32ExDesc_0 61
#define bitsSendsSelReg32ExDesc_1 61

#define bitsSendsSrc0AddrImmSign_0 78
#define bitsSendsSrc0AddrImmSign_1 78
#define bitsSendsSrc0AddrMode_0 79
#define bitsSendsSrc0AddrMode_1 79
#define bitsSendsSrc0AddrImm8_4_0 72
#define bitsSendsSrc0AddrImm8_4_1 68
#define bitsSendsSrc0RegNum_0 76
#define bitsSendsSrc0RegNum_1 69
#define bitsSendsSrc0AddrSubRegNum_0 76
#define bitsSendsSrc0AddrSubRegNum_1 73
#define bitsSendsSrc0Type_0 43
#define bitsSendsSrc0Type_1 43
#define bitsSendsSrc0RegFile_0 41
#define bitsSendsSrc0RegFile_1 42

#define bitsSendsExDescRegNum_0 82
#define bitsSendsExDescRegNum_1 80

// Reserved [84:84], [105:105]

////////////////////////////////////////////////
// 64 bit ISA instruction
// Version 0.0
////////////////////////////////////////////////

// Opcode [6:0] same as 128 bit

// SKL 3src special bits
#define bits3SrcSrc1Type 36
#define bits3SrcSrc2Type 35
#define bits3SrcSrc0Subregnum 84
#define bits3SrcSrc1Subregnum 105
#define bits3SrcSrc2Subregnum 126
#define bits3SrcDstSubregnum_1 55
#define bits3SrcDstSubregnum_0 53

// platform dependent bit positions for instruction fields
// these will be set dynamically once
extern unsigned long bitsFlagSubRegNum[2];
extern unsigned long bitsNibCtrl[2];
extern unsigned long bits3SrcFlagSubRegNum[2];
extern unsigned long bits3SrcSrcType[2];
extern unsigned long bits3SrcDstType[2];
extern unsigned long bits3SrcNibCtrl[2];
extern unsigned long bits3SrcDstRegFile[2];

extern unsigned long bitsDepCtrl[2];
extern unsigned long bitsWECtrl[2];
extern unsigned long bitsDstRegFile[2];
extern unsigned long bitsDstType[2];
extern unsigned long bitsDstIdxRegNum[2];
extern unsigned long bitsDstIdxImmOWord[2];
extern unsigned long bitsDstIdxImmByte[2];
extern unsigned long bitsDstIdxImmMSB[2];
extern unsigned long bitsSrcRegFile[4];
extern unsigned long bitsSrcType[4];
extern unsigned long bitsSrcIdxRegNum[4];
extern unsigned long bitsSrcIdxImmOWord[4];
extern unsigned long bitsSrcIdxImmByte[4];
extern unsigned long bitsSrcIdxImmMSB[4];
extern unsigned long bitsJIP[2];
extern unsigned long bitsUIP[2];
extern unsigned long bits3SrcSrcMod[6];

#define SET_BIT_RANGE(field, high, low)                                        \
  (field)[0] = high;                                                           \
  (field)[1] = low;

#define SET_BIT_RANGES(field, high1, low1, high2, low2)                        \
  (field)[0] = high1;                                                          \
  (field)[1] = low1;                                                           \
  (field)[2] = high2;                                                          \
  (field)[3] = low2;

#define SET_BIT_RANGES2(field, high1, low1, high2, low2, high3, low3)          \
  (field)[0] = high1;                                                          \
  (field)[1] = low1;                                                           \
  (field)[2] = high2;                                                          \
  (field)[3] = low2;                                                           \
  (field)[4] = high3;                                                          \
  (field)[5] = low3;

///////////////////////////////////////////////////////////////////////////////
// Data Structure for Binary Instructions
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Data Structure for Binary Encoding
///////////////////////////////////////////////////////////////////////////////
namespace vISA {
class BinaryEncoding : public BinaryEncodingBase {

public:
  BinaryEncoding(Mem_Manager &m, G4_Kernel &k, const std::string& fname)
      : BinaryEncodingBase(m, k, fname) {}

  virtual ~BinaryEncoding(){

  };

private:
  void insertWaitDst(G4_INST *);
  // void insertInstPointer(G4_INST*);
  void EncodeOpCode(G4_INST *);
  void EncodeExecSize(G4_INST *);
  void EncodeQtrControl(G4_INST *);
  void EncodeAccessMode(G4_INST *);
  void EncodeFlagReg(G4_INST *);
  void EncodeFlagRegPredicate(G4_INST *);
  void EncodeCondModifier(G4_INST *);
  void EncodeInstModifier(G4_INST *);
  void EncodeMathControl(G4_INST *);
  void EncodeInstOptionsString(G4_INST *);
  void EncodeSendMsgDesc29_30(G4_INST *);

  void EncodeSrc2RegNum(G4_INST *inst, BinInst *mybin, G4_Operand *src2);
  void EncodeSrc1RegNum(G4_INST *inst, BinInst *mybin, G4_Operand *src1);
  void EncodeSrc0RegNum(G4_INST *inst, BinInst *mybin, G4_Operand *src0);
  void EncodeDstRegNum(G4_INST *inst, BinInst *mybin, G4_DstRegRegion *dst);

  /*
   * encoding operands
   */
  Status EncodeOperandDst(G4_INST *);
  Status EncodeOperandSrc0(G4_INST *);
  Status EncodeOperandSrc1(G4_INST *);
  Status EncodeOperandSrc2(G4_INST *);
  Status EncodeExtMsgDescr(G4_INST *);
  Status EncodeOperands(G4_INST *);
  Status DoAllEncoding(G4_INST *);
  Status EncodeSplitSendDst(G4_INST *);
  Status EncodeSplitSendSrc0(G4_INST *);
  Status EncodeSplitSendSrc1(G4_INST *);
  Status EncodeSplitSendSrc2(G4_INST *);

  Status EncodeIndirectCallTarget(G4_INST *);

  void SetCmpSrc1Imm32(BinInst *mybin, uint32_t immediateData, G4_Operand *src);

  virtual void SetCompactCtrl(BinInst *mybin, uint32_t value);
  virtual uint32_t GetCompactCtrl(BinInst *mybin);

  void SetBranchOffsets(G4_INST *inst, uint32_t JIP, uint32_t UIP = 0);

  // return true for backfard jumps/calls, false for forward ones
  bool EncodeConditionalBranches(G4_INST *, uint32_t);

public:
  // all platform specific bit locations are initialized here
  static void InitPlatform(TARGET_PLATFORM platform) {
    BinaryEncodingBase::InitPlatform();

    // BDW+ encoding
    SET_BIT_RANGE(bitsFlagRegNum, 33, 33);
    SET_BIT_RANGE(bitsFlagSubRegNum, 32, 32);
    SET_BIT_RANGE(bitsNibCtrl, 11, 11);
    SET_BIT_RANGE(bits3SrcFlagSubRegNum, 32, 32);
    SET_BIT_RANGE(bits3SrcFlagRegNum, 33, 33);
    SET_BIT_RANGE(bits3SrcSrcType, 45, 43);
    SET_BIT_RANGE(bits3SrcDstType, 48, 46);
    SET_BIT_RANGE(bits3SrcNibCtrl, 11, 11);

    SET_BIT_RANGE(bitsDepCtrl, 10, 9);
    SET_BIT_RANGE(bitsWECtrl, 34, 34);
    SET_BIT_RANGE(bitsDstRegFile, 36, 35);
    SET_BIT_RANGE(bitsDstType, 40, 37);
    SET_BIT_RANGE(bitsDstIdxRegNum, 60, 57);
    SET_BIT_RANGE(bitsDstIdxImmOWord, 56, 52);
    SET_BIT_RANGE(bitsDstIdxImmByte, 56, 48);
    SET_BIT_RANGE(bitsDstIdxImmMSB, 47, 47);
    SET_BIT_RANGES(bitsSrcRegFile, 42, 41, 90, 89);
    SET_BIT_RANGES(bitsSrcType, 46, 43, 94, 91);
    SET_BIT_RANGES(bitsSrcIdxRegNum, 76, 73, 108, 105);
    SET_BIT_RANGES(bitsSrcIdxImmOWord, 72, 68, 104, 100);
    SET_BIT_RANGES(bitsSrcIdxImmByte, 72, 64, 104, 96);
    SET_BIT_RANGES(bitsSrcIdxImmMSB, 95, 95, 121, 121);
    SET_BIT_RANGE(bitsJIP, 127, 96);
    SET_BIT_RANGE(bitsUIP, 95, 64);
    SET_BIT_RANGES2(bits3SrcSrcMod, 38, 37, 40, 39, 42, 41);
  }

  Status ProduceBinaryInstructions();

  // Status commitLabels();
  // Status CommitRelativeAddresses();

  virtual void DoAll();

  // Status DeleteMemForBins();

  void CompactInstructions();
  void Compact();

  // char *GetKernelBuffer() { return buffer; };
  // void SetKernelBuffer(char *_buffer) { buffer = _buffer; };

  void *alloc(size_t size) { return mem.alloc(size); };

  inline bool compactOneInstruction(G4_INST *inst) {
    G4_opcode op = inst->opcode();
    BinInst *mybin = getBinInst(inst);
    if (op == G4_if || op == G4_else || op == G4_endif || op == G4_while ||
        op == G4_halt || op == G4_break || op == G4_cont ||
        /* GetComprCtrl(mybin) == COMPR_CTRL_COMPRESSED  || */
        mybin->GetDontCompactFlag()) {
      // do not compact conditional branches
      return false;
    }

    // ToDo: disable compacting nop/return until it is clear that we can compact
    // them
    if (op == G4_nop || op == G4_return) {
      return false;
    }

    // temporary WA, to be removed later
    if (op == G4_call) {
      return false;
    }

    return BDWcompactOneInstruction(inst);
  }
};
} // namespace vISA

#endif
