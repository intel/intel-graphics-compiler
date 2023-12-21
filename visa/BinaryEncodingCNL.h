/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _BINARYENCODINGCNL_H_
#define _BINARYENCODINGCNL_H_

#include "BinaryEncoding.h"
#include "FlowGraph.h"

///=--- Definitions for auto-generated header.

#if defined(_WIN32)
#include <windows.h>
#endif

#if !defined(_WIN32)
typedef unsigned char BYTE;
typedef uint32_t UINT;
#endif
typedef uint64_t QWORD;

#ifndef BITFIELD_RANGE
#define BITFIELD_RANGE( startbit, endbit )     ((endbit)-(startbit)+1)
#endif

#define BITFIELD_BIT(bit) 1

#undef __CONCAT
#define __CONCAT(x, y) x##y
#define __UNIQUENAME(a1, a2) __CONCAT(a1, a2)
#define UNIQUENAME(__text) __UNIQUENAME(__text, __COUNTER__)
#define STATIC_ASSERT(e) typedef char UNIQUENAME(STATIC_ASSERT_)[(e) ? 1 : -1]

#define __CODEGEN_UNIQUE(field) UNIQUENAME(field)
#define __CODEGEN_INLINE inline
#define __CODEGEN_PACKED
#define __CODEGEN_ATTRIBUTES_STRUCTURE

#define __CODEGEN_GET_MACRO()
#define __CODEGEN_SET_MACRO(value)

#define __CODEGEN_DefineDebugCommand(type, ...)                                \
  inline void DebugCommand(type, ...)
#define __CODEGEN_DebugCommandEnumParam(type, ...)
#define __CODEGEN_DebugCommandUIntParam(type, ...)
#define __CODEGEN_DebugCommandBoolParam(type, ...)
#define __CODEGEN_DefineEnumString(type) inline void EnumString(type)
#define __CODEGEN_EnumStringValue(value)

// Gen9 defines
#define __CODEGEN_FILE_DIRECTIVES_OPEN
#define __CODEGEN_NAMESPACE_OPEN namespace G9HDL {
#define __CODEGEN_NAMESPACE_CLOSE } // namespace G9HDL
#define __CODEGEN_FILE_DIRECTIVES_CLOSE
#define __CODEGEN_ACCESS_SPECIFIER_DEFINITION
#define __CODEGEN_ACCESS_SPECIFIER_METHODS

///=--- Definitions for auto-generated header.
#pragma pack(push, 1)
#include "IGfxHwEuIsaCNL.h"

#include "IGfxHwEuIsaICL.h"

/// \brief Class encapsulating encoding machinery using new auto-generated
/// headers
///
///
namespace vISA {
class BinaryEncodingCNL : public BinaryEncodingBase {
public:
  BinaryEncodingCNL(Mem_Manager &m, G4_Kernel &k, const std::string& fname)
      : BinaryEncodingBase(m, k, fname) {}

  virtual ~BinaryEncodingCNL(){

  };

  typedef enum { SUCCESS, FAILURE } Status;

  // BinaryEncodingCNL::Status DoAll();
  // void *EmitBinary(uint32_t&);
  void *alloc(size_t size) { return mem.alloc(size); };
  BinaryEncodingCNL::Status DoAllEncoding(G4_INST *);

  virtual void DoAll();

  // Handle CNL+ specific opcode. For common opcode (pre-CNL and CNL+),
  // it invokes base's getEUOpcode().
  G9HDL::EU_OPCODE getEUOpcode(G4_opcode g4opc);

private:
  bool EncodeConditionalBranches(G4_INST *, uint32_t);
  void SetBranchOffsets(G4_INST *inst, uint32_t JIP, uint32_t UIP = 0);

  ///==------------------------------------------------------------------------
  BinaryEncodingCNL::Status DoAllEncodingWAIT(G4_INST *);
  BinaryEncodingCNL::Status DoAllEncodingJMPI(G4_INST *);
  BinaryEncodingCNL::Status DoAllEncodingCALL(G4_INST *);
  BinaryEncodingCNL::Status DoAllEncodingCF(G4_INST *);
  BinaryEncodingCNL::Status DoAllEncodingSplitSEND(G4_INST *);
  BinaryEncodingCNL::Status EncodeSplitSend(G4_INST *,
                                            G9HDL::EU_INSTRUCTION_SENDS &);

  BinaryEncodingCNL::Status DoAllEncodingRegular(G4_INST *);
  void EncodeOneSrcInst(G4_INST *, G9HDL::EU_INSTRUCTION_BASIC_ONE_SRC &);
  void EncodeTwoSrcInst(G4_INST *, G9HDL::EU_INSTRUCTION_BASIC_TWO_SRC &);
  void EncodeThreeSrcInst(G4_INST *, G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC &);
  void EncodeThreeSrcInstAlign1(G4_INST *,
                                G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC &);

  /// 0-th DWORD of instruction, header
  void EncodeInstHeader(G4_INST *, G9HDL::EU_INSTRUCTION_HEADER &);
  /// note: indentation shows that those are subfields of instruction header
  void EncodeOpCode(G4_INST *, G9HDL::EU_INSTRUCTION_HEADER &);
  void EncodeAccessMode(G4_INST *, G9HDL::EU_INSTRUCTION_CONTROLS_A &);
  void EncodeDepControl(G4_INST *, G9HDL::EU_INSTRUCTION_CONTROLS_A &);
  void EncodeQtrControl(G4_INST *, G9HDL::EU_INSTRUCTION_CONTROLS_A &);
  void EncodeThreadControl(G4_INST *, G9HDL::EU_INSTRUCTION_CONTROLS_A &);
  void EncodeFlagRegPredicate(G4_INST *, G9HDL::EU_INSTRUCTION_CONTROLS_A &);
  void EncodeExecSize(G4_INST *, G9HDL::EU_INSTRUCTION_CONTROLS_A &);
  void EncodeCondModifier(G4_INST *, G9HDL::EU_INSTRUCTION_CONTROLS &);
  void EncodeAccWrCtrl(G4_INST *, G9HDL::EU_INSTRUCTION_CONTROLS &);
  void EncodeInstModifier(G4_INST *, G9HDL::EU_INSTRUCTION_CONTROLS &);

  /// 1-st DWORD of instruction, header
  void EncodeOperandDst(G4_INST *, G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS &);
  void EncodeDstChanEn(G4_INST *, G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS &);
  void EncodeDstRegFile(G4_INST *, G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS &);
  void EncodeDstRegNum(G4_INST *, G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS &);
  void EncodeDstArchRegNum(G4_INST *, G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS &);
  void EncodeDstIndirectRegNum(G4_INST *,
                               G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS &);
  void EncodeDstHorzStride(G4_INST *, G4_DstRegRegion *,
                           G9HDL::EU_INSTRUCTION_OPERAND_CONTROLS &);

  ///==------------------------------------------------------------------------

  // inline void EncodeSrcImmData(G9HDL::EU_INSTRUCTION_SOURCES_IMM32&,
  // G4_Operand *);
  inline void EncodeSrcImm64Data(G9HDL::EU_INSTRUCTION_IMM64_SRC &,
                                 G4_Operand *);
  // void insertWaitDst(G4_INST*);

  /// Virtual methods overrides, used by generic (base) methods.
  virtual void SetCompactCtrl(BinInst *mybin, uint32_t value);
  virtual uint32_t GetCompactCtrl(BinInst *mybin);

  ///====----- Inline helpers:

  /// \brief Translates from v-isa reg file enum to HDL reg file type enum
  ///
  static inline G9HDL::REGFILE TranslateVisaToHDLRegFile(RegFile regFile) {
    return (G9HDL::REGFILE)regFile;
  }

  /// \brief Translates from v-isa AddrMode enum to HDL ADDRMODE type enum
  ///
  static inline G9HDL::ADDRMODE TranslateVisaToHDLAddrMode(AddrMode addrMode) {
    return (G9HDL::ADDRMODE)addrMode;
  }

  /// \brief Packs Arch Reg File and Arch Reg Num into single unsigned short for
  /// encoding
  ///
  static inline unsigned short
  PackArchRegTypeAndArchRegFile(unsigned short RegFile,
                                unsigned short RegNumValue) {
    unsigned short EncodedRegNum = RegFile << 4;
    EncodedRegNum = EncodedRegNum | (RegNumValue & 0xF);
    return EncodedRegNum;
  }

public:
  //////////////////////////////////////////////////////////////////////////
  // reviewed
  static inline G9HDL::SRCMOD GetSrcHLDMod(G4_SrcRegRegion *srcRegion) {
    uint32_t mod = srcRegion->getModifier();
    G9HDL::SRCMOD srcMod = G9HDL::SRCMOD_NO_MODIFICATION;
    switch (mod) {
    case Mod_Minus_Abs:
      srcMod = G9HDL::SRCMOD_NEGATE_OF_ABS;
      break;
    case Mod_Abs:
      srcMod = G9HDL::SRCMOD_ABS;
      break;
    case Mod_Minus:
      srcMod = G9HDL::SRCMOD_NEGATE;
      break;
    case Mod_Not:
      // same as negate
      srcMod = G9HDL::SRCMOD_NEGATE;
      break;
    case Mod_src_undef:
      // do nothing
      break;
    default:
      vISA_ASSERT_UNREACHABLE("unexpected source modifier");
      break;
    }
    return srcMod;
  }
};

//===----------------------------------------------------------------------===//

/// \brief Template class for destination operand encoder
///
/// Template parameter is an auto-header generated struct that
/// performs encoding of destination operand (WORD1) part of instruction.
/// The template and template specialization is used so to isolate the
/// high-level builder logic from underlying encoding mechanism for
/// different kind of instruction types (two-src, three-src etc.)
template <typename T> class DstOperandEncoder {
public:
  static void SetCombinedFlagRegSubregNumber(T &opndCtl,
                                             unsigned FlagRegNumValue,
                                             unsigned FlagRegSubNumValue) {
    uint32_t value = 0;
    value = FlagRegNumValue << 1;
    value = value | FlagRegSubNumValue;

    opndCtl.SetFlagRegisterNumberSubregisterNumber(value);
  }
};

template <> class DstOperandEncoder<G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC> {
public:
  static void
  SetCombinedFlagRegSubregNumber(G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC &opndCtl,
                                 unsigned FlagRegNumValue,
                                 unsigned FlagRegSubNumValue) {
    opndCtl.SetFlagRegisterNumberSubregisterNumber(FlagRegSubNumValue);
    opndCtl.SetFlagRegisterNumber(FlagRegNumValue);
  }
};

template <> class DstOperandEncoder<G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC> {
public:
  static void SetCombinedFlagRegSubregNumber(
      G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC &opndCtl, unsigned FlagRegNumValue,
      unsigned FlagRegSubNumValue) {
    opndCtl.SetFlagSubregisterNumber(FlagRegSubNumValue);
    opndCtl.SetFlagRegisterNumber(FlagRegNumValue);
  }
};

/// \brief Template-based builder for destination instruction fields.
///
/// Basically those fields whose encoding methods in auto-generated
/// header differ by name/parameters based on the instruction type will need
/// to be encoded through this template.
template <typename T> class DstBuilder {
public:
  /// \brief Template based field encoder for mask control (a.k.a. write-enable)
  ///
  /// This one will get template specialized, and is will call suitable field
  /// encoder method for either OPERAND_CONTROLS, three-src, send or sends type
  /// encoding
  static void EncodeMaskCtrl(G4_INST *inst, T &opnds) {
    if (inst->isWriteEnableInst())
      opnds.SetMaskctrl(G9HDL::MASKCTRL_WRITE_ALL_CHANNELS);

    if (inst->opcode() == G4_jmpi)
      opnds.SetMaskctrl(G9HDL::MASKCTRL_WRITE_ALL_CHANNELS);
  }

  /// \brief Template based field encoder for operant destination type
  ///        Template parameter is the type of encoding mask.
  static void EncodeOperandDstType(G4_INST *inst, T &opnds) {
    G4_DstRegRegion *dst = inst->getDst();
    G4_Type regType = dst->asDstRegRegion()->getType();

    switch (regType) { // BXML bug Line 851: bitrange 5-8, should be: 37-40
    case Type_UD:
      opnds.SetDestinationDataType(G11HDL::DSTTYPE_UD);
      break;
    case Type_D:
      opnds.SetDestinationDataType(G11HDL::DSTTYPE_D);
      break;
    case Type_UW:
      opnds.SetDestinationDataType(G11HDL::DSTTYPE_UW);
      break;
    case Type_W:
      opnds.SetDestinationDataType(G11HDL::DSTTYPE_W);
      break;
    case Type_UB:
      opnds.SetDestinationDataType(G11HDL::DSTTYPE_UB);
      break;
    case Type_B:
      opnds.SetDestinationDataType(G11HDL::DSTTYPE_B);
      break;
    case Type_DF:
      opnds.SetDestinationDataType(G11HDL::DSTTYPE_DF);
      break;
    case Type_F:
      opnds.SetDestinationDataType(G11HDL::DSTTYPE_F);
      break;
    case Type_UQ:
      opnds.SetDestinationDataType(G11HDL::DSTTYPE_UQ);
      break;
    case Type_Q:
      opnds.SetDestinationDataType(G11HDL::DSTTYPE_Q);
      break;
    case Type_HF:
      opnds.SetDestinationDataType(G11HDL::DSTTYPE_HF);
      break;
    default:
      vISA_ASSERT_UNREACHABLE("Encoding error: destination type unknown");
      break;
    }
  }

  /// \brief Template based field encoder for combined destination FlagRegNum
  /// and FlagSubRegNum
  ///
  static void EncodeFlagReg(G4_INST *inst, T &opnds) {
    bool flagRegNumValid = false;
    unsigned FlagRegNumValue = 0;
    unsigned FlagRegSubNumValue = 0;

    G4_Predicate *pred = inst->getPredicate();
    if (pred) {
      FlagRegNumValue = pred->getBase()->ExRegNum(flagRegNumValid);
      FlagRegSubNumValue = pred->getBase()->asRegVar()->getPhyRegOff();
    }

    G4_CondMod *cModifier = inst->getCondMod();
    if (cModifier) { // cond modifier
      G4_VarBase *flagReg = cModifier->getBase();
      if (flagReg != NULL) {
        FlagRegNumValue = flagReg->ExRegNum(flagRegNumValid);
        FlagRegSubNumValue = flagReg->asRegVar()->getPhyRegOff();
      } else {
        FlagRegNumValue = 0;
        FlagRegSubNumValue = 0;
      }
    }

    if (pred || cModifier) {

      DstOperandEncoder<T>::SetCombinedFlagRegSubregNumber(
          opnds, FlagRegNumValue, FlagRegSubNumValue);
    }
  }

  /// \brief Template based field encoder for combined Dst.AddrMode
  ///
  static void EncodeDstAddrMode(G4_INST *inst, T &opnds) {
    G4_DstRegRegion *dst = inst->getDst();
    switch (EncodingHelper::GetDstAddrMode(dst)) {
    case ADDR_MODE_IMMED:
      // same field for align16
      opnds.SetDestinationAddressingMode(G9HDL::ADDRMODE_DIRECT);
      break;
    case ADDR_MODE_INDIR:
      // same field for align16
      opnds.SetDestinationAddressingMode(G9HDL::ADDRMODE_INDIRECT);
      break;
    default:
      vISA_ASSERT_UNREACHABLE("Encoding error: addressing mode type unknown");
      break;
    }
  }
};

//===----------------------------------------------------------------------===//

/// \brief Template class for source operand encoder
///
/// There are two template parameters: auto-generated struct type
/// and source operand number that select which source (src0,src1,src2) is
/// being encoded.
///
/// Template parameter is an auto-header generated struct that
/// performs encoding of selected source operand part of instruction.
/// The template and template specialization is used so to isolate the
/// high-level builder logic from underlying encoding mechanism for
/// different kind of instruction types (one-src, two-src, three-src etc.)
/// and different source numbers.
template <typename T, int SrcNum> class SrcOperandEncoder {
public:
  static void SetSrcChanSel(T *EuInstructionSources, uint32_t value) {
    vISA_ASSERT_UNREACHABLE("SrcOperandEncoder::SetSrcChanSel template "
                        "specialization not implemented.");
  }

  static void SetSrcChanSel_10(T *EuInstructionSources, uint32_t value) {
    vISA_ASSERT_UNREACHABLE("SrcOperandEncoder::SetSrcChanSel_10 template "
                        "specialization not implemented.");
  }
  static void SetSrcChanSel_32(T *EuInstructionSources, uint32_t value) {
    vISA_ASSERT_UNREACHABLE("SrcOperandEncoder::SetSrcChanSel_32 template "
                        "specialization not implemented.");
  }

  static void SetSourceWidth(T *EuInstructionSources, G9HDL::WIDTH width) {
    vISA_ASSERT_UNREACHABLE("SrcOperandEncoder::SetSourceWidth template "
                        "specialization not implemented.");
  }

  static void SetSourceHorizontalStride(T *EuInstructionSources,
                                        G9HDL::HORZSTRIDE stride) {
    vISA_ASSERT_UNREACHABLE("SrcOperandEncoder::SetSourceHorizontalStride template "
                        "specialization not implemented.");
  }

  static void SetSourceVerticalStride(T *EuInstructionSources,
                                      G9HDL::VERTSTRIDE vertStride) {
    vISA_ASSERT_UNREACHABLE("SrcOperandEncoder::SetSourceVerticalStride template "
                        "specialization not implemented.");
  }

  static void SetSourceRegisterNumber(T *EuInstructionSources, uint32_t value) {
    vISA_ASSERT_UNREACHABLE("SrcOperandEncoder::SetSourceRegisterNumber template "
                        "specialization not implemented.");
  }

  static void SetSourceSpecialAcc(T *EuInstructionSources, uint32_t value) {
    vISA_ASSERT_UNREACHABLE("SrcOperandEncoder::SetSourceSpecialAcc template "
                        "specialization not implemented.");
  }

  static void SetSourceSubRegisterNumber(T *EuInstructionSources,
                                         uint32_t value) {
    vISA_ASSERT_UNREACHABLE("SrcOperandEncoder::SetSourceSubRegisterNumber "
                        "template specialization not implemented.");
  }

  static void SetSourceSubregisterNumber44(T *EuInstructionSources,
                                           uint32_t value) {
    vISA_ASSERT_UNREACHABLE("SrcOperandEncoder::SetSourceSubregisterNumber44 "
                        "template specialization not implemented.");
  }

  static void SetAddressSubregisterNumber(T *EuInstructionSources,
                                          uint32_t value) {
    vISA_ASSERT_UNREACHABLE("SrcOperandEncoder::SetAddressSubregisterNumber "
                        "template specialization not implemented.");
  }

  static void SetSourceAddressImmediate84(T *EuInstructionSources,
                                          uint32_t value) {
    vISA_ASSERT_UNREACHABLE("SrcOperandEncoder::SetSourceAddressImmediate84 "
                        "template specialization not implemented.");
  }

  static void SetSourceAddressImmediate90(T *EuInstructionSources,
                                          int32_t value) {
    vISA_ASSERT_UNREACHABLE("SrcOperandEncoder::SetSourceAddressImmediate80 "
                        "template specialization not implemented.");
  }

  static void SetSourceImmediateData(T *EuInstructionSources, uint32_t value) {
    vISA_ASSERT_UNREACHABLE("SrcOperandEncoder::SetSourceImmediateData template "
                        "specialization not implemented.");
  }

  static void SetSourceAddressingMode(T *EuInstructionSources,
                                      G9HDL::ADDRMODE addrMode) {
    vISA_ASSERT_UNREACHABLE("SrcOperandEncoder::SetSourceAddressingMode template "
                        "specialization not implemented.");
  }

  static void SetSourceModifier(T *EuInstructionSources, G9HDL::SRCMOD srcMod) {
    vISA_ASSERT_UNREACHABLE("SrcOperandEncoder::SetSourceModifier template "
                        "specialization not implemented.");
  }
};

/// \brief Template specialization for source operand encoder for T, src0
///
template <typename T> class SrcOperandEncoder<T, 0> {
public:
  static void SetSrcChanSel(T *EuInstructionSources, uint32_t value) {
    // first 4 bits
    SetSrcChanSel_10(EuInstructionSources, value & 0xf);
    // second 4 bits
    SetSrcChanSel_32(EuInstructionSources, (value >> 4) & 0xf);
  }

  static void SetSrcChanSel_10(T *EuInstructionSources, uint32_t value) {
    EuInstructionSources->SetSource0_SourceChannelSelect30(value);
  }
  static void SetSrcChanSel_32(T *EuInstructionSources, uint32_t value) {
    EuInstructionSources->SetSource0_SourceChannelSelect74(value);
  }
  static void SetSourceWidth(T *EuInstructionSources, G9HDL::WIDTH width) {
    EuInstructionSources->SetSource0_SourceWidth(width);
  }
  static void SetSourceHorizontalStride(T *EuInstructionSources,
                                        G9HDL::HORZSTRIDE stride) {
    EuInstructionSources->SetSource0_SourceHorizontalStride(stride);
  }
  static void SetSourceVerticalStride(T *EuInstructionSources,
                                      G9HDL::VERTSTRIDE vertStride) {
    EuInstructionSources->SetSource0_SourceVerticalStride(vertStride);
  }
  static void SetSourceRegisterNumber(T *EuInstructionSources, uint32_t value) {
    EuInstructionSources->SetSource0_SourceRegisterNumber(value);
  }

  static void SetSourceSpecialAcc(T *EuInstructionSources, uint32_t value) {
    EuInstructionSources->SetSource0_SourceSubRegisterNumber(value & 0xF);
  }

  static void SetSourceSubRegisterNumber(T *EuInstructionSources,
                                         uint32_t value) {
    EuInstructionSources->SetSource0_SourceSubRegisterNumber(value);
  }

  static void SetSourceSubregisterNumber44(T *EuInstructionSources,
                                           uint32_t value) {
    EuInstructionSources->SetSource0_SourceSubregisterNumber44(value);
  }

  static void SetAddressSubregisterNumber(T *EuInstructionSources,
                                          uint32_t value) {
    EuInstructionSources->SetSource0_AddressSubregisterNumber_0(value);
  }

  static void SetSourceAddressImmediate84(T *EuInstructionSources,
                                          uint32_t value) {
    EuInstructionSources->SetSource0_SourceAddressImmediate84(value);
  }

  static void SetSourceAddressImmediate90(T *EuInstructionSources,
                                          int32_t value) {
    EuInstructionSources->SetSource0_SourceAddressImmediate80(value);

    value = value >> 9;
    EuInstructionSources->SetSource0_SourceAddressImmediate9(value);
  }

  static void SetSourceImmediateData(T *EuInstructionSources, uint32_t value) {
    EuInstructionSources->SetSource0Immediate(value);
  }

  static void SetSourceAddressingMode(T *EuInstructionSources,
                                      G9HDL::ADDRMODE addrMode) {
    EuInstructionSources->SetSource0_SourceAddressingMode(addrMode);
  }

  static void SetSourceModifier(T *EuInstructionSources, G9HDL::SRCMOD srcMod) {
    EuInstructionSources->SetSource0_SourceModifier(srcMod);
  }
};

/// \brief Template specialization for source operand encoder for 3-src
/// instruction
///        type in align16 mode, SRC 0
///
template <> class SrcOperandEncoder<G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC, 0> {
public:
  static void SetSrcChanSel(G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC *threeSrc,
                            uint32_t value) {
    threeSrc->SetSource0_SourceSwizzle(value);
  }

  static void SetSourceModifier(G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC *threeSrc,
                                G9HDL::SRCMOD srcMod) {
    threeSrc->SetSource0_SourceModifier(srcMod);
  }

  static void
  SrcReplicateControl(G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC *threeSrc,
                      G9HDL::REPCTRL repCtrl) {
    threeSrc->SetSource0_SourceReplicateControl(repCtrl);
  }

  static void
  SetSourceRegisterNumber(G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC *threeSrc,
                          uint32_t value) {
    threeSrc->SetSource0_SourceRegisterNumber(value);
  }

  static void
  SetSourceSubregisterNumber42(G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC *threeSrc,
                               uint32_t value) {
    threeSrc->SetSource0_SourceSubregisterNumber42(value);
  }

  static void
  SetSourceSubregisterNumber1(G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC *threeSrc,
                              uint32_t value) {
    threeSrc->SetSource0_SourceSubregisterNumber1(value);
  }
};

/// \brief Template specialization for source operand encoder for 3-src
/// instruction
///        type in align16 mode, SRC 1
///
template <> class SrcOperandEncoder<G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC, 1> {
public:
  static void SetSrcChanSel(G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC *threeSrc,
                            uint32_t value) {
    threeSrc->SetSource1_SourceSwizzle(value);
  }

  static void SetSourceModifier(G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC *threeSrc,
                                G9HDL::SRCMOD srcMod) {
    threeSrc->SetSource1_SourceModifier(srcMod);
  }

  static void
  SrcReplicateControl(G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC *threeSrc,
                      G9HDL::REPCTRL repCtrl) {
    threeSrc->SetSource1_SourceReplicateControl(repCtrl);
  }

  static void
  SetSourceRegisterNumber(G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC *threeSrc,
                          uint32_t value) {
    threeSrc->SetSource1_SourceRegisterNumber(value);
  }

  static void
  SetSourceSubregisterNumber42(G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC *threeSrc,
                               uint32_t value) {
    threeSrc->SetSource1_SourceSubregisterNumber42(value);
  }

  static void
  SetSourceSubregisterNumber1(G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC *threeSrc,
                              uint32_t value) {
    threeSrc->SetSource1_SourceSubregisterNumber1(value);
  }
};

/// \brief Template specialization for source operand encoder for 3-src
/// instruction
///        type in align16 mode, SRC 2
///
template <> class SrcOperandEncoder<G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC, 2> {
public:
  static void SetSrcChanSel(G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC *threeSrc,
                            uint32_t value) {
    threeSrc->SetSource2_SourceSwizzle(value);
  }

  static void SetSourceModifier(G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC *threeSrc,
                                G9HDL::SRCMOD srcMod) {
    threeSrc->SetSource2_SourceModifier(srcMod);
  }

  static void
  SrcReplicateControl(G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC *threeSrc,
                      G9HDL::REPCTRL repCtrl) {
    threeSrc->SetSource2_SourceReplicateControl(repCtrl);
  }

  static void
  SetSourceRegisterNumber(G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC *threeSrc,
                          uint32_t value) {
    threeSrc->SetSource2_SourceRegisterNumber(value);
  }

  static void
  SetSourceSubregisterNumber42(G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC *threeSrc,
                               uint32_t value) {
    threeSrc->SetSource2_SourceSubregisterNumber42(value);
  }

  static void
  SetSourceSubregisterNumber1(G9HDL::EU_INSTRUCTION_BASIC_THREE_SRC *threeSrc,
                              uint32_t value) {
    threeSrc->SetSource2_SourceSubregisterNumber1(value);
  }
};

/// \brief Template specialization for source operand encoder for T, src1
///
template <typename T> class SrcOperandEncoder<T, 1> {
public:
  static void SetSrcChanSel(T *EuInstructionSources, uint32_t value) {
    // first 4 bits
    SetSrcChanSel_10(EuInstructionSources, value & 0xf);
    // second 4 bits
    SetSrcChanSel_32(EuInstructionSources, (value >> 4) & 0xf);
  }

  static void SetSrcChanSel_10(T *EuInstructionSources, uint32_t value) {
    EuInstructionSources->SetSource1_SourceChannelSelect30(value);
  }
  static void SetSrcChanSel_32(T *EuInstructionSources, uint32_t value) {
    EuInstructionSources->SetSource1_SourceChannelSelect74(value);
  }
  static void SetSourceWidth(T *EuInstructionSources, G9HDL::WIDTH width) {
    EuInstructionSources->SetSource1_SourceWidth(width);
  }
  static void SetSourceHorizontalStride(T *EuInstructionSources,
                                        G9HDL::HORZSTRIDE stride) {
    EuInstructionSources->SetSource1_SourceHorizontalStride(stride);
  }
  static void SetSourceVerticalStride(T *EuInstructionSources,
                                      G9HDL::VERTSTRIDE vertStride) {
    EuInstructionSources->SetSource1_SourceVerticalStride(vertStride);
  }

  static void SetSourceRegisterNumber(T *EuInstructionSources, uint32_t value) {
    EuInstructionSources->SetSource1_SourceRegisterNumber(value);
  }

  static void SetSourceSpecialAcc(T *EuInstructionSources, uint32_t value) {
    EuInstructionSources->SetSource1_SourceSubRegisterNumber(value & 0xF);
  }

  static void SetSourceSubRegisterNumber(T *EuInstructionSources,
                                         uint32_t value) {
    EuInstructionSources->SetSource1_SourceSubRegisterNumber(value);
  }

  static void SetSourceSubregisterNumber44(T *EuInstructionSources,
                                           uint32_t value) {
    EuInstructionSources->SetSource1_SourceSubregisterNumber44(value);
  }

  static void SetAddressSubregisterNumber(T *EuInstructionSources,
                                          uint32_t value) {
    EuInstructionSources->SetSource1_AddressSubregisterNumber(value);
  }

  static void SetSourceAddressImmediate84(T *EuInstructionSources,
                                          uint32_t value) {
    EuInstructionSources->SetSource1_SourceAddressImmediate84(value);
  }

  static void SetSourceAddressImmediate90(T *EuInstructionSources,
                                          uint32_t value) {
    EuInstructionSources->SetSource1_SourceAddressImmediate80(value);
    value = value >> 9;
    EuInstructionSources->SetSource1_SourceAddressImmediate9(value);
  }

  static void SetSourceImmediateData(T *EuInstructionSources, uint32_t value) {
    EuInstructionSources->SetSource1Immediate(value);
  }

  static void SetSourceAddressingMode(T *EuInstructionSources,
                                      G9HDL::ADDRMODE addrMode) {
    EuInstructionSources->SetSource1_SourceAddressingMode(addrMode);
  }

  static void SetSourceModifier(T *EuInstructionSources, G9HDL::SRCMOD srcMod) {
    EuInstructionSources->SetSource1_SourceModifier(srcMod);
  }
};

/// \brief Template specialization for source operand encoder for 3-src
/// instruction
///        type in align1 mode, SRC 0
///
template <> class SrcOperandEncoder<G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC, 0> {
public:
  static void SetSourceHorizontalStride(
      G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC *EuInstructionSources,
      G9HDL::HORZSTRIDE stride) {
    EuInstructionSources->SetSource0HorizontalStride(stride);
  }
  static void SetSourceVerticalStride(
      G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC *EuInstructionSources,
      G9HDL::TERNARYALIGN1VERTSTRIDE vertStride) {
    EuInstructionSources->SetSource0VerticalStride(vertStride);
  }
};

/// \brief Template specialization for source operand encoder for 3-src
/// instruction
///        type in align1 mode, SRC 1
///
template <> class SrcOperandEncoder<G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC, 1> {
public:
  static void SetSourceHorizontalStride(
      G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC *EuInstructionSources,
      G9HDL::HORZSTRIDE stride) {
    EuInstructionSources->SetSource1HorizontalStride(stride);
  }
  static void SetSourceVerticalStride(
      G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC *EuInstructionSources,
      G9HDL::TERNARYALIGN1VERTSTRIDE vertStride) {
    EuInstructionSources->SetSource1VerticalStride(vertStride);
  }
};

/// \brief Template specialization for source operand encoder for 3-src
/// instruction
///        type in align1 mode, SRC 2
///
template <> class SrcOperandEncoder<G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC, 2> {
public:
  static void SetSourceHorizontalStride(
      G9HDL::EU_INSTRUCTION_ALIGN1_THREE_SRC *EuInstructionSources,
      G9HDL::HORZSTRIDE stride) {
    EuInstructionSources->SetSource2HorizontalStride(stride);
  }
};

/// \brief Template-based builder for source instruction fields.
///
/// Builder is parametrized by two template parameters, one of which
/// is the type of the auto-generated header class, and other is source
/// number that is to be encoded
template <typename T, int SrcNum> class SrcBuilder {
public:
  /// \brief Template based field encoder for Src[SrcNum].AddrMode
  ///
  static void EncodeSrcAddrMode(T *EuInstructionSources, G4_INST *inst,
                                G4_Operand *src0) {
    if (src0->asSrcRegRegion()->getRegAccess() == Direct) {
      SrcOperandEncoder<T, SrcNum>::SetSourceAddressingMode(
          EuInstructionSources, G9HDL::ADDRMODE_DIRECT);
    } else {
      SrcOperandEncoder<T, SrcNum>::SetSourceAddressingMode(
          EuInstructionSources, G9HDL::ADDRMODE_INDIRECT);
    }
  }

  //////////////////////////////////////////////////////////////////////////
  /// \brief Template based field encoder for RegNum field for 3src instruction
  /// types
  ///
  static void EncodeSrcRegNum3Src(G4_INST *inst, G4_Operand *src0,
                                  T &sourcesReg) {
    if (EncodingHelper::GetSrcRegFile(src0) != REG_FILE_A &&
        EncodingHelper::GetSrcAddrMode(src0) == ADDR_MODE_IMMED) {
      uint32_t byteAddress = src0->getLinearizedStart();

      // repCtrl is only set for 3src instructions
      // if (inst->isAligned1Inst() || repControl)
      // TODO: how do we understand this commented line (from original
      // EncodeSrc0RegNum) ?
      //  in 3src, bits 83 - 73 encode (73-75):3 subreg, and (76-83):8 bits
      //  regnum, dword aligned
      if (inst->isAligned1Inst()) {
        vISA_ASSERT_UNREACHABLE("align1 not allowed for 3src instructions");
      } else { // align 16
        // register number: 256 bit (32 byte) aligned part of an address
        // sub-register number: first/second 16 byte part of 32 byte address.
        // Encoded with 1 bit.
        //  9876543210
        //  regn|xxxy0
        // dword aligned

        SrcOperandEncoder<T, SrcNum>::SetSourceRegisterNumber(&sourcesReg,
                                                              byteAddress >> 5);
        // it will be shifted to right within setter method
        SrcOperandEncoder<T, SrcNum>::SetSourceSubregisterNumber42(
            &sourcesReg, byteAddress & 0x1f);
        SrcOperandEncoder<T, SrcNum>::SetSourceSubregisterNumber1(
            &sourcesReg, (byteAddress >> 1) & 0x1);
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////
  /// \brief Template based field encoder for replicate control field
  ///        of 3src instruction types
  ///
  static void Encode3SrcReplicateControl(T *myBin, G4_SrcRegRegion *srcRegion,
                                         BinaryEncodingBase &encoder) {
    auto maybeSwizzle = encoder.getSwizzle(srcRegion);
    if (maybeSwizzle && *maybeSwizzle == SrcSwizzle::R)
      SrcOperandEncoder<T, SrcNum>::SrcReplicateControl(
          myBin, G9HDL::REPCTRL_REPLICATE_ACROSS_ALL_CHANNELS);
    else
      SrcOperandEncoder<T, SrcNum>::SrcReplicateControl(
          myBin, G9HDL::REPCTRL_NO_REPLICATION);
  }

  //////////////////////////////////////////////////////////////////////////
  /// \brief Template based field encoder for ChanSel
  ///
  static void EncodeSrcChanSelect(T *myBin, G4_INST *inst, G4_Operand *src0,
                                  G4_SrcRegRegion *srcRegion,
                                  BinaryEncodingBase &encoder) {
    bool ChanSelectValid = false;

    // encode acc2~acc9 if it is valid
    if (src0->isAccRegValid() && inst->getPlatform() < GENX_ICLLP) {
      if (inst->opcode() == G4_madm ||
          (inst->isMath() &&
           (inst->asMathInst()->getMathCtrl() == MATH_INVM ||
            inst->asMathInst()->getMathCtrl() == MATH_RSQRTM))) {
        uint32_t value = src0->getAccRegSel();
        SrcOperandEncoder<T, SrcNum>::SetSrcChanSel(myBin, value);
        return;
      }
      vISA_ASSERT(false, "acc2~acc7 were set on wrong instruction");
    }

    auto maybeSwizzle = encoder.getSwizzle(srcRegion);
    if (maybeSwizzle && *maybeSwizzle != SrcSwizzle::R) {
      ChanSelectValid = true;
      ChanSel ch0 =
          EncodingHelper::GetSrcChannelSelectValue(srcRegion, 0, encoder);
      ChanSel ch1 =
          EncodingHelper::GetSrcChannelSelectValue(srcRegion, 1, encoder);
      ChanSel ch2 =
          EncodingHelper::GetSrcChannelSelectValue(srcRegion, 2, encoder);
      ChanSel ch3 =
          EncodingHelper::GetSrcChannelSelectValue(srcRegion, 3, encoder);
      uint32_t value = 0;

      if (ch0 != CHAN_SEL_UNDEF)
        value = ch0;
      if (ch1 != CHAN_SEL_UNDEF)
        value = value | (ch1 << 2);

      uint32_t value2 = 0;

      // value = 0;
      if (ch2 != CHAN_SEL_UNDEF)
        value2 = ch2;
      if (ch3 != CHAN_SEL_UNDEF)
        value2 = value2 | (ch3 << 2);

      SrcOperandEncoder<T, SrcNum>::SetSrcChanSel(myBin, value | (value2 << 4));
    }

    if (!ChanSelectValid & inst->isAligned16Inst()) {
      uint32_t value = 0;
      value = CHAN_SEL_X;
      value = value | (CHAN_SEL_Y << 2);

      uint32_t value2 = 0;
      // value = 0;
      value2 = CHAN_SEL_Z;
      value2 = value2 | (CHAN_SEL_W << 2);

      SrcOperandEncoder<T, SrcNum>::SetSrcChanSel(myBin, value | (value2 << 4));
    }
  }

  //////////////////////////////////////////////////////////////////////////
  /// \brief Template based field encoder for SrcMod
  ///
  static void EncodeSrcModifier(G4_INST *inst, G4_Operand *src0,
                                T &sourcesReg) {
    G4_SrcRegRegion *srcRegion = src0->asSrcRegRegion();
    {
      SrcOperandEncoder<T, SrcNum>::SetSourceModifier(
          &sourcesReg, BinaryEncodingCNL::GetSrcHLDMod(srcRegion));
    }
  }

  //////////////////////////////////////////////////////////////////////////
  /// \brief Template based field encoder for Src[SrcNum].Width
  ///
  static bool EncodeSrcWidth(G4_INST *inst, T *mybin, const RegionDesc *rd,
                             G4_Operand *src) {
    bool WidthValid = false;
    if (inst->isAligned16Inst())
      return false;

    if (rd) {
      if (rd->width != UNDEFINED_SHORT) {
        WidthValid = true;
      }

      switch (rd->width) {
      case 1:
        SrcOperandEncoder<T, SrcNum>::SetSourceWidth(mybin,
                                                     G9HDL::WIDTH_1_ELEMENTS);
        break;
      case 2:
        SrcOperandEncoder<T, SrcNum>::SetSourceWidth(mybin,
                                                     G9HDL::WIDTH_2_ELEMENTS);
        break;
      case 4:
        SrcOperandEncoder<T, SrcNum>::SetSourceWidth(mybin,
                                                     G9HDL::WIDTH_4_ELEMENTS);
        break;
      case 8:
        SrcOperandEncoder<T, SrcNum>::SetSourceWidth(mybin,
                                                     G9HDL::WIDTH_8_ELEMENTS);
        break;
      case 16:
        SrcOperandEncoder<T, SrcNum>::SetSourceWidth(mybin,
                                                     G9HDL::WIDTH_16_ELEMENTS);
        break;
      case UNDEFINED_SHORT:
        break;
      default:
        vISA_ASSERT_UNREACHABLE("wrong width for src0!");
        break;
      }
    }

    // apply default width
    if (!WidthValid) {
      if (EncodingHelper::isSrcSubRegNumValid(src)) {
        SrcOperandEncoder<T, SrcNum>::SetSourceWidth(mybin,
                                                     G9HDL::WIDTH_1_ELEMENTS);
      } else {
        switch (GetEncodeExecSize(inst)) {
        case ES_1_CHANNEL:
          SrcOperandEncoder<T, SrcNum>::SetSourceWidth(mybin,
                                                       G9HDL::WIDTH_1_ELEMENTS);
          break;
        case ES_2_CHANNELS:
          SrcOperandEncoder<T, SrcNum>::SetSourceWidth(mybin,
                                                       G9HDL::WIDTH_2_ELEMENTS);
          break;
        case ES_4_CHANNELS:
          SrcOperandEncoder<T, SrcNum>::SetSourceWidth(mybin,
                                                       G9HDL::WIDTH_4_ELEMENTS);
          break;
        case ES_8_CHANNELS:
        case ES_16_CHANNELS:
          SrcOperandEncoder<T, SrcNum>::SetSourceWidth(mybin,
                                                       G9HDL::WIDTH_8_ELEMENTS);
          break;
        case ES_32_CHANNELS:
          SrcOperandEncoder<T, SrcNum>::SetSourceWidth(
              mybin, G9HDL::WIDTH_16_ELEMENTS);
          break;
        }
      }
    }

    return WidthValid;
  }

  //////////////////////////////////////////////////////////////////////////
  /// \brief Template based field encoder for Src[SrcNum].HorzStride
  ///
  static bool EncodeSrcHorzStride(G4_INST *inst, T *mybin, const RegionDesc *rd,
                                  G4_Operand *src0) {
    // For Align16 instruction (SIMD4), treat <HorzStride> as <VertStride>
    // For Align16 source operand disable HorzStride
    bool HorzStrideValid = false; // undef
    if (inst->isAligned16Inst())
      return false;

    if (rd) {
      if (rd->horzStride != UNDEFINED_SHORT) {
        HorzStrideValid = true;
      }
      switch (rd->horzStride) {
      case 0:
        SrcOperandEncoder<T, SrcNum>::SetSourceHorizontalStride(
            mybin, G9HDL::HORZSTRIDE_0_ELEMENTS);
        break;
      case 1:
        SrcOperandEncoder<T, SrcNum>::SetSourceHorizontalStride(
            mybin, G9HDL::HORZSTRIDE_1_ELEMENTS);
        break;
      case 2:
        SrcOperandEncoder<T, SrcNum>::SetSourceHorizontalStride(
            mybin, G9HDL::HORZSTRIDE_2_ELEMENTS);
        break;
      case 4:
        SrcOperandEncoder<T, SrcNum>::SetSourceHorizontalStride(
            mybin, G9HDL::HORZSTRIDE_4_ELEMENTS);
        break;
      case UNDEFINED_SHORT:
        break;
      default:
        vISA_ASSERT_UNREACHABLE("wrong horizontal stride for src0!");
        break;
      }
    }
    // apply default horizontal stride
    if (!HorzStrideValid) {
      if (EncodingHelper::isSrcSubRegNumValid(src0))
        SrcOperandEncoder<T, SrcNum>::SetSourceHorizontalStride(
            mybin, G9HDL::HORZSTRIDE_0_ELEMENTS);
      else {
        switch (GetEncodeExecSize(inst)) {
        case ES_1_CHANNEL:
          SrcOperandEncoder<T, SrcNum>::SetSourceHorizontalStride(
              mybin, G9HDL::HORZSTRIDE_0_ELEMENTS);
          break;
        case ES_2_CHANNELS:
        case ES_4_CHANNELS:
        case ES_8_CHANNELS:
        case ES_16_CHANNELS:
        case ES_32_CHANNELS:
          SrcOperandEncoder<T, SrcNum>::SetSourceHorizontalStride(
              mybin, G9HDL::HORZSTRIDE_1_ELEMENTS);
          break;
        }
      }
    } // end of valid horz stride
    return HorzStrideValid;
  }

  //////////////////////////////////////////////////////////////////////////
  /// \brief Template based field encoder for Src[SrcNum].VertStride
  ///
  static void EncodeSrcVertStride(G4_INST *inst, T *mybin, const RegionDesc *rd,
                                  G4_Operand *src0, const bool WidthValid,
                                  const bool HorzStrideValid) {
    bool VertStrideValid = false; // undef
    unsigned short VertStrideValue = UNDEFINED_SHORT, HorzStrideValue = 0;

    if (rd) {
      VertStrideValue = rd->vertStride;
      HorzStrideValue = rd->horzStride;
      if (VertStrideValue != UNDEFINED_SHORT) {
        VertStrideValid = true;
      }

      switch (VertStrideValue) {
      case 0:
        SrcOperandEncoder<T, SrcNum>::SetSourceVerticalStride(
            mybin, G9HDL::VERTSTRIDE_0_ELEMENTS);
        break;
      case 1:
        SrcOperandEncoder<T, SrcNum>::SetSourceVerticalStride(
            mybin, G9HDL::VERTSTRIDE_1_ELEMENT);
        break;
      case 2:
        SrcOperandEncoder<T, SrcNum>::SetSourceVerticalStride(
            mybin, G9HDL::VERTSTRIDE_2_ELEMENTS);
        break;
      case 4:
        SrcOperandEncoder<T, SrcNum>::SetSourceVerticalStride(
            mybin, G9HDL::VERTSTRIDE_4_ELEMENTS);
        break;
      case 8:
        SrcOperandEncoder<T, SrcNum>::SetSourceVerticalStride(
            mybin, G9HDL::VERTSTRIDE_8_ELEMENTS);
        break;
      case 16:
        SrcOperandEncoder<T, SrcNum>::SetSourceVerticalStride(
            mybin, G9HDL::VERTSTRIDE_16_ELEMENTS);
        break;
      case 32:
        SrcOperandEncoder<T, SrcNum>::SetSourceVerticalStride(
            mybin, G9HDL::VERTSTRIDE_32_ELEMENTS);
        break;
      case UNDEFINED_SHORT:
        break;
      default:
        vISA_ASSERT_UNREACHABLE("wrong vertical stride for src0!");
        break;
      }
    }

    // apply default vertical stride below
    if (!WidthValid && !HorzStrideValid && !VertStrideValid && src0) {
      VertStrideValid = true;
      if (EncodingHelper::isSrcSubRegNumValid(src0)) {
        SrcOperandEncoder<T, SrcNum>::SetSourceVerticalStride(
            mybin, G9HDL::VERTSTRIDE_0_ELEMENTS);
      } else {
        if (inst->isAligned1Inst()) {
          switch (GetEncodeExecSize(inst)) {
          case ES_1_CHANNEL:
            SrcOperandEncoder<T, SrcNum>::SetSourceVerticalStride(
                mybin, G9HDL::VERTSTRIDE_0_ELEMENTS);
            break;
          case ES_2_CHANNELS:
            SrcOperandEncoder<T, SrcNum>::SetSourceVerticalStride(
                mybin, G9HDL::VERTSTRIDE_2_ELEMENTS);
            break;
          case ES_4_CHANNELS:
            SrcOperandEncoder<T, SrcNum>::SetSourceVerticalStride(
                mybin, G9HDL::VERTSTRIDE_4_ELEMENTS);
            break;
          case ES_8_CHANNELS:
          case ES_16_CHANNELS:
            SrcOperandEncoder<T, SrcNum>::SetSourceVerticalStride(
                mybin, G9HDL::VERTSTRIDE_8_ELEMENTS);
            break;
          case ES_32_CHANNELS:
            SrcOperandEncoder<T, SrcNum>::SetSourceVerticalStride(
                mybin, G9HDL::VERTSTRIDE_16_ELEMENTS);
            break;
          }
        } else {
          SrcOperandEncoder<T, SrcNum>::SetSourceVerticalStride(
              mybin, G9HDL::VERTSTRIDE_4_ELEMENTS);
        }
      }
    }

    // Do some post processing, if none of the previous cases was true.
    if (VertStrideValid) {
    } else if (inst->isAligned16Inst()) {
      // FIXME: should this be ever valid?
      // we cannot be setting horz stride in align16 instructions!!!
      if (HorzStrideValid && HorzStrideValue == 0) {
        SrcOperandEncoder<T, SrcNum>::SetSourceVerticalStride(
            mybin, G9HDL::VERTSTRIDE_0_ELEMENTS);
      } else if (HorzStrideValid && HorzStrideValue == 4) {
        SrcOperandEncoder<T, SrcNum>::SetSourceVerticalStride(
            mybin, G9HDL::VERTSTRIDE_4_ELEMENTS);
      }
    } else {
      vASSERT(src0);
      if (EncodingHelper::GetSrcAddrMode(src0) == ADDR_MODE_INDIR) { // indirect
        SrcOperandEncoder<T, SrcNum>::SetSourceVerticalStride(
            mybin, G9HDL::VERTSTRIDE_VXH_OR_VX1_MODE);
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////
  /// \brief Template based field encoder for source immediate based addressing
  ///        RegNum. It encodes RegNum for non-ARF based register files.
  static void EncodeSrcRegNum(G4_INST *inst, G4_Operand *src0, T &sourcesReg) {
    if (EncodingHelper::GetSrcRegFile(src0) != REG_FILE_A &&
        EncodingHelper::GetSrcAddrMode(src0) == ADDR_MODE_IMMED) {
      uint32_t byteAddress = src0->getLinearizedStart();

      // repCtrl is only set for 3src instructions
      // if (inst->isAligned1Inst() || repControl)
      // TODO: how do we understand this commented line (from original
      // EncodeSrc0RegNum) ?
      //  in 3src, bits 83 - 73 encode (73-75):3 subreg, and (76-83):8 bits
      //  regnum, dword aligned
      if (inst->isAligned1Inst()) {

        // register number: 256 bit (32 byte) aligned part of an address
        // sub-register number: address (5 bits encoding) within a 32 byte GRF
        //  9876543210
        //  regn|subre

        SrcOperandEncoder<T, SrcNum>::SetSourceRegisterNumber(&sourcesReg,
                                                              byteAddress >> 5);
        if (inst->getPlatform() >= GENX_ICLLP && src0->isAccRegValid()) {
          vISA_ASSERT((byteAddress & 0x1F) == 0,
                       "subreg must be 0 for source with special accumulator");
          SrcOperandEncoder<T, SrcNum>::SetSourceSpecialAcc(
              &sourcesReg, src0->getAccRegSel());
        } else {
          SrcOperandEncoder<T, SrcNum>::SetSourceSubRegisterNumber(
              &sourcesReg, byteAddress & 0x1F);
        }
      } else { // align 16
        // register number: 256 bit (32 byte) aligned part of an address
        // sub-register number: first/second 16 byte part of 32 byte address.
        // Encoded with 1 bit.
        //  9876543210
        //  regn|x0000

        SrcOperandEncoder<T, SrcNum>::SetSourceRegisterNumber(&sourcesReg,
                                                              byteAddress >> 5);
        SrcOperandEncoder<T, SrcNum>::SetSourceSubregisterNumber44(
            &sourcesReg, (byteAddress >> 4) & 0x1);
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////
  /// \brief Template based field encoder for source architecture RegNum and
  /// SubRegNum encoding
  ///
  /// essentially, this encoding is split in three parts:
  /// setting reg num : arch reg file + arch reg number
  /// setting sub-reg-num
  /// here, we fuse two original methods into one: EncodeDstArchRegNum and
  /// EncodeDstRegFile for ARF
  static void EncodeSrcArchRegNum(G4_INST *inst, G4_SrcRegRegion *src,
                                  T &sourcesReg) {
    if (EncodingHelper::GetSrcRegFile(src) == REG_FILE_A &&
        EncodingHelper::GetSrcAddrMode(src) == ADDR_MODE_IMMED) {
      if (EncodingHelper::GetSrcArchRegType(src) != ARCH_REG_FILE_NULL) {
        // EncodeSrc0RegFile, SetSrc0ArchRegFile- corresponding functionality
        bool valid;

        unsigned short RegFile =
            (unsigned short)EncodingHelper::GetSrcArchRegType(src); // 4 bits
        unsigned short RegNumValue = src->ExRegNum(valid);

        // 7654|3210
        // RegF|RegNumVal
        unsigned short EncodedRegNum = RegFile << 4;
        EncodedRegNum = EncodedRegNum | (RegNumValue & 0xF);

        // encode 'packed' arch reg file + reg number. Both for align16 and
        // align1, immediate mode.
        SrcOperandEncoder<T, SrcNum>::SetSourceRegisterNumber(&sourcesReg,
                                                              EncodedRegNum);

        bool subValid;
        unsigned short RegSubNumValue = src->ExSubRegNum(subValid);
        unsigned short ElementSizeValue =
            EncodingHelper::GetElementSizeValue(src);
        uint32_t regOffset = RegSubNumValue * ElementSizeValue;

        if (inst->isAligned1Inst()) {
          // sub-register number: 32 byte address (5 bits encoding) within a GRF
          SrcOperandEncoder<T, SrcNum>::SetSourceSubRegisterNumber(&sourcesReg,
                                                                   regOffset);
        } else { // align 16
          // sub-register number: first/second 16 byte part of 32 byte address.
          // Encoded with 1 bit.
          //  9876543210
          //  regn|x0000
          SrcOperandEncoder<T, SrcNum>::SetSourceSubregisterNumber44(
              &sourcesReg, (regOffset >> 4) & 0x1);
        }
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////
  /// \brief Template based field encoder for source indirect RegNum and
  /// SubRegNum encoding
  ///
  /// Essentially it sets address subregister number alltogether with immediate
  /// offset
  static void EncodeSrcIndirectRegNum(G4_INST *inst, G4_SrcRegRegion *src,
                                      T &sourcesReg) {
    if (EncodingHelper::GetSrcRegFile(src) == REG_FILE_R) {
      if (EncodingHelper::GetSrcAddrMode(src) == ADDR_MODE_INDIR) { // Indirect
        bool subValid;
        unsigned short IndAddrRegSubNumValue = 0;
        short IndAddrImmedValue = 0;

        IndAddrRegSubNumValue = src->ExIndSubRegNum(subValid);
        IndAddrImmedValue = src->ExIndImmVal();

        // SetSrc0IdxRegNum(mybin, IndAddrRegSubNumValue);
        // the same is for align16
        SrcOperandEncoder<T, SrcNum>::SetAddressSubregisterNumber(
            &sourcesReg, IndAddrRegSubNumValue);

        /* Set the indirect address immediate value. */
        if (inst->isAligned1Inst()) {
          // bits [0-8]
          SrcOperandEncoder<T, SrcNum>::SetSourceAddressImmediate90(
              &sourcesReg, IndAddrImmedValue);
        } else { // here we are setting align16
          // bits [4-8]
          SrcOperandEncoder<T, SrcNum>::SetSourceAddressImmediate84(
              &sourcesReg, IndAddrImmedValue / BYTES_PER_OWORD);
        }
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////
  /// \brief Template based aggregate encoder for all source related
  ///        fields (width,stride, regnum, modifier, chan-sel) for both align1
  ///        and align16 modes.
  static void EncodeEuInstructionSourcesReg(G4_INST *inst, G4_Operand *src,
                                            T &sourcesReg,
                                            BinaryEncodingBase &encoder) {
    if (src->isSrcRegRegion()) {
      G4_SrcRegRegion *srcRegion = src->asSrcRegRegion();
      const RegionDesc *rd = srcRegion->getRegion();

      SrcBuilder<T, SrcNum>::EncodeSrcAddrMode(&sourcesReg, inst, src);
      if (inst->isAligned16Inst()) {
        SrcBuilder<T, SrcNum>::EncodeSrcChanSelect(&sourcesReg, inst, src,
                                                   srcRegion, encoder);
      }
      SrcBuilder<T, SrcNum>::EncodeSrcModifier(inst, src, sourcesReg);
      if (!inst->isSend()) {
        bool WidthValid =
            SrcBuilder<T, SrcNum>::EncodeSrcWidth(inst, &sourcesReg, rd, src);
        bool HorzStrideValid = SrcBuilder<T, SrcNum>::EncodeSrcHorzStride(
            inst, &sourcesReg, rd, src);
        SrcBuilder<T, SrcNum>::EncodeSrcVertStride(inst, &sourcesReg, rd, src,
                                                   WidthValid, HorzStrideValid);
      }
      SrcBuilder<T, SrcNum>::EncodeSrcRegNum(inst, src, sourcesReg);
      SrcBuilder<T, SrcNum>::EncodeSrcArchRegNum(inst, src->asSrcRegRegion(),
                                                 sourcesReg);
      SrcBuilder<T, SrcNum>::EncodeSrcIndirectRegNum(
          inst, src->asSrcRegRegion(), sourcesReg);
    } // if
  }

  //////////////////////////////////////////////////////////////////////////
  /// \brief Template based sets NULL Register for source.
  static void EncodeEuInstructionNullSourcesReg(G4_INST *inst, G4_Operand *src,
                                                T &sourcesReg) {
    if (src->isSrcRegRegion()) {
      SrcOperandEncoder<T, SrcNum>::SetSourceAddressingMode(
          &sourcesReg, G9HDL::ADDRMODE_DIRECT);

      SrcOperandEncoder<T, SrcNum>::SetSourceWidth(&sourcesReg,
                                                   G9HDL::WIDTH_1_ELEMENTS);
      SrcOperandEncoder<T, SrcNum>::SetSourceHorizontalStride(
          &sourcesReg, G9HDL::HORZSTRIDE_0_ELEMENTS);
      SrcOperandEncoder<T, SrcNum>::SetSourceVerticalStride(
          &sourcesReg, G9HDL::VERTSTRIDE_0_ELEMENTS);
    } else {
      vISA_ASSERT(false, "Invalid NULL register source.");
    }
  }

  //////////////////////////////////////////////////////////////////////////
  /// \brief Template based field encoder for encoding source immediate 32 bit
  /// data
  ///
  static void EncodeSrcImmData(T &immSource, G4_Operand *src) {

    G4_Imm *isrc = (G4_Imm *)src->asImm();
    if (IS_WTYPE(src->getType())) {
      uint32_t val = (uint32_t)isrc->getInt();
      uint32_t data = (val << 16) | (val & 0xffff);
      SrcOperandEncoder<T, SrcNum>::SetSourceImmediateData(&immSource, data);
    } else if (src->getType() == Type_F) {
      float val = (float)(isrc->getFloat());
      SrcOperandEncoder<T, SrcNum>::SetSourceImmediateData(&immSource,
                                                           *(uint32_t *)&(val));

    } else {
      SrcOperandEncoder<T, SrcNum>::SetSourceImmediateData(
          &immSource, (uint32_t)isrc->getInt());
    }
  }
};
} // namespace vISA
inline void vISA::BinaryEncodingCNL::EncodeSrcImm64Data(
    G9HDL::EU_INSTRUCTION_IMM64_SRC &imm64SourceInstruction,
    vISA::G4_Operand *src) {

  vISA::G4_Imm *isrc = (vISA::G4_Imm *)src->asImm();

  if (src->getType() == Type_DF) {
    imm64SourceInstruction.SetSource((uint64_t)isrc->getImm());
  } else if (src->getType() == Type_Q || src->getType() == Type_UQ) {
    // Q/UQ immediates must be the only source
    int64_t val = isrc->getInt();
    imm64SourceInstruction.SetSource(*(uint64_t *)&(val));
  }
}

#pragma pack(pop)

#endif
