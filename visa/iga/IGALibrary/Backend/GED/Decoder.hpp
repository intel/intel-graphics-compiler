/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_BACKEND_GED_DECODER_HPP_
#define _IGA_BACKEND_GED_DECODER_HPP_

#include "GEDBitProcessor.hpp"
#include "GEDToIGATranslation.hpp"
#include "ged.h"

#include <optional>

#define GED_DECODE_TO(FIELD, TRANS, DST)                                       \
  do {                                                                         \
    GED_RETURN_VALUE _status;                                                  \
    DST = TRANS(GED_Get##FIELD(&m_currGedInst, &_status));                     \
    if (_status != GED_RETURN_VALUE_SUCCESS) {                                 \
      handleGedDecoderError(__LINE__, #FIELD, _status);                        \
    }                                                                          \
  } while (0)

#define GED_DECODE_RAW_TO(FIELD, DST)                                          \
  do {                                                                         \
    GED_RETURN_VALUE _status;                                                  \
    DST = GED_Get##FIELD(&m_currGedInst, &_status);                            \
    if (_status != GED_RETURN_VALUE_SUCCESS) {                                 \
      handleGedDecoderError(__LINE__, #FIELD, _status);                        \
    }                                                                          \
  } while (0)

#define GED_DECODE_RAW(GED_TYPE, ID, FIELD)                                    \
  [[maybe_unused]] GED_TYPE ID;                                                                 \
  GED_DECODE_RAW_TO(FIELD, ID);

#define GED_DECODE(IGA_TYPE, GED_TYPE, ID, FIELD)                              \
  GED_DECODE_RAW(GED_TYPE, GED_##ID, FIELD);                                   \
  IGA_TYPE ID = translate(GED_##ID);

#define GED_DECODE_RAW_TO_SRC(DST, TYPE, FIELD)                                \
  do {                                                                         \
    GED_RETURN_VALUE _STATUS;                                                  \
    DST = GED_Get##FIELD(&m_currGedInst, &_STATUS);                            \
    if (_STATUS != GED_RETURN_VALUE_SUCCESS) {                                 \
      handleGedDecoderError(__LINE__, #FIELD, _STATUS);                        \
    }                                                                          \
  } while (0)

#define RETURN_GED_DECODE_RAW_TO_SRC(TYPE, FIELD)                              \
  {                                                                            \
    TYPE _DST;                                                                 \
    GED_DECODE_RAW_TO_SRC(_DST, TYPE, FIELD);                                  \
    return _DST;                                                               \
  }
// the extra namespace declaration is needed to make g++ happy
#define DEFINE_SOURCE_ACCESSOR(TYPE, FIELD, I)                                 \
  namespace iga {                                                              \
  template <> TYPE Decoder::decodeSrc##FIELD<SourceIndex::SRC##I>() {          \
    RETURN_GED_DECODE_RAW_TO_SRC(TYPE, Src##I##FIELD);                         \
  }                                                                            \
  }

/* #define DEFINE_SOURCE_ACCESSOR_INLINE(TYPE, FIELD, I) \
template <> TYPE decodeSrc ## FIELD <SourceIndex::SRC##I>() { \
RETURN_GED_DECODE_RAW_TO_SRC(TYPE, Src ## I ## FIELD); \
}
*/
/* #define DEFINE_GED_SOURCE_ACCESSORS_INLINE_01(TYPE, FIELD) \
DEFINE_SOURCE_ACCESSOR_INLINE(TYPE, FIELD, 0) \
DEFINE_SOURCE_ACCESSOR_INLINE(TYPE, FIELD, 1)
*/
#define DEFINE_GED_SOURCE_ACCESSORS_01(TYPE, FIELD)                            \
  DEFINE_SOURCE_ACCESSOR(TYPE, FIELD, 0)                                       \
  DEFINE_SOURCE_ACCESSOR(TYPE, FIELD, 1)

/* #define DEFINE_GED_SOURCE_ACCESSORS_INLINE_012(TYPE, FIELD) \
DEFINE_SOURCE_ACCESSOR_INLINE(TYPE, FIELD, 0) \
DEFINE_SOURCE_ACCESSOR_INLINE(TYPE, FIELD, 1) \
DEFINE_SOURCE_ACCESSOR_INLINE(TYPE, FIELD, 2)
*/
#define DEFINE_GED_SOURCE_ACCESSORS_012(TYPE, FIELD)                           \
  DEFINE_SOURCE_ACCESSOR(TYPE, FIELD, 0)                                       \
  DEFINE_SOURCE_ACCESSOR(TYPE, FIELD, 1)                                       \
  DEFINE_SOURCE_ACCESSOR(TYPE, FIELD, 2)


namespace iga {
struct FlagRegInfo {
  Predication pred;
  FlagModifier modifier;
  RegRef reg;
};
struct DirRegOpInfo {
  RegName regName = RegName::INVALID; // e.g. "r" or "acc"
  RegRef regRef;                      // 13
  Type type = Type::INVALID;
};

class Decoder : public GEDBitProcessor {
public:
  // Constructs a new decoder with an error handler and an empty kernel
  Decoder(const Model &model, ErrorHandler &errHandler);

  // the main entry point for decoding a kernel
  Kernel *decodeKernelBlocks(const void *binary, size_t binarySize);
  Kernel *decodeKernelNumeric(const void *binary, size_t binarySize);

  // Set the SWSB endcoding mode, if not set, derived from platform
  void setSWSBEncodingMode(SWSB_ENCODE_MODE mode) {
    if (mode != SWSB_ENCODE_MODE::SWSBInvalidMode) {
      m_SWSBEncodeMode = mode;
    }
  }

  bool isMacro() const;

private:
  Kernel *decodeKernel(const void *binary, size_t binarySize,
                       bool numericLabels);

  // pass 1 decodes instructions with numeric labels
  void decodeInstructions(Kernel &kernel, const void *binary, size_t binarySize,
                          InstList &insts);
  const OpSpec *decodeOpSpec(Op op);

  Instruction *decodeNextInstruction(Kernel &kernel);

  void decodeSWSB(Instruction *inst);

protected:
  GED_ACCESS_MODE decodeAccessMode();
  MaskCtrl decodeMaskCtrl();
  Predication decodePredication();
  void decodePredInv(Predication &pred);
  FlagRegInfo
  decodeFlagRegInfo(bool imm64Src0Overlap = false); // pred, cond, ...
  ExecSize decodeExecSize();
  ChannelOffset decodeChannelOffset();

  void decodeJipToSrc(Instruction *inst, SourceIndex s = SourceIndex::SRC0,
                      Type type = Type::INVALID);
  void decodeUipToSrc1(Instruction *inst, Type type);
  int32_t decodeJip();
  int32_t decodeUip();

  // Reads a source from a the 'fromSrc'th source operand
  // and creates the operand in IR as 'toSrc'.
  // Typically this will be used directly as
  //   createSoureOp(inst, SourceIndex::SRC0, SourceIndex::SRC0);
  // However, for some operands with implicit operands such as
  // jmpi, ths will show up as
  //   createSoureOp(inst, SourceIndex::SRC1, SourceIndex::SRC0);
  // since the first syntactic source is stored in src1's bits
  // (src0 is ip for that op)

  ///////////////////////////////////////////////////////////////////////
  // BASIC INSTRUCTIONS
  ///////////////////////////////////////////////////////////////////////
  Instruction *decodeBasicInstruction(Kernel &kernel);
  void decodeBasicUnaryInstruction(Instruction *inst,
                                   GED_ACCESS_MODE accessMode);

  void decodeBasicDestination(Instruction *inst, GED_ACCESS_MODE a) {
    if (a == GED_ACCESS_MODE_Align16)
      decodeBasicDestinationAlign16(inst);
    else
      decodeBasicDestinationAlign1(inst);
  }
  void decodeBasicDestinationAlign16(
      Instruction *inst); // e.g. for math.invm and context save restore
  void decodeBasicDestinationAlign1(Instruction *inst);
  template <SourceIndex S>
  void decodeSourceBasic(Instruction *inst, GED_ACCESS_MODE a) {
    if (a == GED_ACCESS_MODE_Align16)
      decodeSourceBasicAlign16<S>(inst, S);
    else
      decodeSourceBasicAlign1<S>(inst, S);
  }
  template <SourceIndex S>
  void decodeSourceBasic(Instruction *inst, SourceIndex toSrcIx,
                         GED_ACCESS_MODE a) {
    if (a == GED_ACCESS_MODE_Align16)
      decodeSourceBasicAlign16<S>(inst, toSrcIx);
    else
      decodeSourceBasicAlign1<S>(inst, toSrcIx);
  }
  template <SourceIndex S> void decodeSourceBasicAlign1(Instruction *inst) {
    decodeSourceBasicAlign1<S>(inst, S);
  }
  template <SourceIndex S> void decodeSourceBasicAlign16(Instruction *inst) {
    decodeSourceBasicAlign16<S>(inst, S);
  }
  template <SourceIndex S>
  void decodeSourceBasicAlign1(Instruction *inst, SourceIndex toSrcIx);
  template <SourceIndex S>
  void decodeSourceBasicAlign16(Instruction *inst, SourceIndex toSrcIx);


  ///////////////////////////////////////////////////////////////////////
  // TERNARY INSTRUCTIONS
  ///////////////////////////////////////////////////////////////////////
  Instruction *decodeTernaryInstruction(Kernel &kernel);
  void decodeTernaryInstructionOperands(Kernel &kernel, Instruction *inst,
                                        GED_ACCESS_MODE accessMode);
  // Align16
  void decodeTernaryDestinationAlign16(Instruction *inst);
  template <SourceIndex S> void decodeTernarySourceAlign16(Instruction *inst);
  // Align1
  void decodeTernaryDestinationAlign1(Instruction *inst);
  template <SourceIndex S> void decodeTernarySourceAlign1(Instruction *inst);

  ///////////////////////////////////////////////////////////////////////
  // SEND INSTRUCTIONS
  ///////////////////////////////////////////////////////////////////////
  Instruction *decodeSendInstruction(Kernel &kernel);
  void decodeSendDestination(Instruction *inst);
  GED_ADDR_MODE decodeSendSource0AddressMode();
  void decodeSendSource0(Instruction *inst);
  void decodeSendSource1(Instruction *inst);
  SendDesc decodeSendExDesc();
  SendDesc decodeSendDesc();

  // facilitates internal decoding logic
  struct SendDescodeInfo {
    SFID sfid = SFID::INVALID;
    int dstLen = -1, src0Len = -1, src1Len = -1;
    bool hasCps = false, hasExBSO = false;
    SendDesc desc, exDesc;
    uint32_t exImmOffDesc = 0;
  };
  void decodeSendInfoPreXe(SendDescodeInfo &sdi);
  void decodeSendInfoXe(SendDescodeInfo &sdi);
  void decodeSendInfoXeHP(SendDescodeInfo &sdi);
  void decodeSendInfoXeHPG(SendDescodeInfo &sdi);
  void decodeSendInfoXe2(SendDescodeInfo &sdi);

  ///////////////////////////////////////////////////////////////////////
  // BRANCH INSTRUCTIONS
  ///////////////////////////////////////////////////////////////////////
  Instruction *decodeBranchInstruction(Kernel &kernel);
  Instruction *decodeBranchSimplifiedInstruction(Kernel &kernel);
  void decodeBranchDestination(Instruction *inst);

  ///////////////////////////////////////////////////////////////////////
  // OTHER INSTRUCTIONS
  ///////////////////////////////////////////////////////////////////////
  Instruction *decodeWaitInstruction(Kernel &kernel);
  Instruction *decodeSyncInstruction(Kernel &kernel);

  ///////////////////////////////////////////////////////////////////////
  // OTHER HELPERS
  ///////////////////////////////////////////////////////////////////////
  void decodeReg(int opIx, // dst => opIx<0
                 GED_REG_FILE regFile, uint32_t regNumBits, RegName &regName,
                 RegRef &regRef);

  DirRegOpInfo decodeDstDirRegInfo();
  Type decodeDstType();
  int decodeDestinationRegNumAccBitsFromChEn();
  MathMacroExt decodeDestinationMathMacroRegFromChEn();

  Subfunction decodeSubfunction(bool &valid);
  bool hasImm64Src0Overlap();

  void decodeDstDirSubRegNum(DirRegOpInfo &dri);
  bool hasImplicitScalingType(Type &type, DirRegOpInfo &dri);
  void decodeNextInstructionEpilog(Instruction *inst);

  void decodeThreadOptions(Instruction *inst, GED_THREAD_CTRL trdCntrl);

  template <SourceIndex S> ImmVal decodeTernarySrcImmVal(Type t);

  // various GED source accessors
  // some definitions created in Decoder.cpp using macros
  // i.e. DEFINE_GED_SOURCE_ACCESSORS...
  template <SourceIndex S> GED_ADDR_MODE decodeSrcAddrMode();

  // #define DEFINE_SOURCE_ACCESSOR_INLINE(TYPE, FIELD, I)
  // template <> GED_ADDR_MODE decodeSrcAddrMode <SourceIndex::SRC0>() {
  //     RETURN_GED_DECODE_RAW_TO_SRC(GED_ADDR_MODE, Src0AddrMode);
  // }

  template <SourceIndex S> GED_REG_FILE decodeSrcRegFile();
  template <SourceIndex S> GED_DATA_TYPE decodeSrcDataType();

  // register direct fields
  template <SourceIndex S> uint32_t decodeSrcRegNum();
  template <SourceIndex S> uint32_t decodeSrcSubRegNum();
  // implicit accumulators
  template <SourceIndex S> GED_MATH_MACRO_EXT decodeSrcMathMacroExt();

  // register indirect fields
  template <SourceIndex S> int32_t decodeSrcAddrImm();
  template <SourceIndex S> uint32_t decodeSrcAddrSubRegNum();
  // immediate fields
  ImmVal decodeSrcImmVal(Type type);

  // register direct and indirect fields
  // yeah, the naming convention dictates this name
  //   (on for Src vs Dst) and one for the field name SrcMod
  template <SourceIndex S> GED_SRC_MOD decodeSrcSrcMod();

  template <SourceIndex S> Type decodeSrcType() {
    return translate(decodeSrcDataType<S>());
  }

  template <SourceIndex S>
  std::optional<Region> decodeSrcReducedRegionTernary() {
    return {};
  }

  template <SourceIndex S>
  std::optional<Region> decodeSrcReducedRegion() {
    return {};
  }

  template <SourceIndex S> std::optional<Region> decodeSrcRegionVWH() {
    return transateGEDtoIGARegion(decodeSrcVertStride<S>(), decodeSrcWidth<S>(),
                                  decodeSrcHorzStride<S>());
  }

  template <SourceIndex S> Region decodeSrcRegionTernaryAlign1(const OpSpec &);

  template <SourceIndex S> MathMacroExt decodeSrcMathMacroReg() {
    return translate(decodeSrcMathMacroExt<S>());
  }
  template <SourceIndex S> SrcModifier decodeSrcModifier() {
    if (m_opSpec->supportsSourceModifiers()) {
      return translate(decodeSrcSrcMod<S>());
    }
    return SrcModifier::NONE;
  }

  // decodes regname, regnum, and subreg num (if applicable)
  // does *not* scale the subreg num
  template <SourceIndex S> RegName decodeSourceReg(RegRef &regRef) {
    uint32_t regNumBits = decodeSrcRegNum<S>();
    RegName regName = RegName::INVALID;
    GED_REG_FILE regFile = decodeSrcRegFile<S>();
    decodeReg((int)S, regFile, regNumBits, regName, regRef);
    if (!m_opSpec->isAnySendFormat() && !isMacro()) {
      regRef.subRegNum = (uint16_t)decodeSrcSubRegNum<S>();
    } else {
      regRef.subRegNum = 0;
    }
    return regName;
  }

  template <SourceIndex S> void decodeGrfSourceReg(RegRef& regRef) {
    uint32_t regNumBits = decodeSrcRegNum<S>();
    RegName regName = RegName::INVALID;
    decodeReg((int)S, GED_REG_FILE_GRF, regNumBits, regName, regRef);
    if (!m_opSpec->isAnySendFormat() && !isMacro()) {
      regRef.subRegNum = (uint16_t)decodeSrcSubRegNum<S>();
    } else {
      regRef.subRegNum = 0;
    }
  }

  template <SourceIndex S> DirRegOpInfo decodeSrcDirRegOpInfo() {
    DirRegOpInfo dri;
    dri.regName = decodeSourceReg<S>(dri.regRef);

    Type scalingType = Type::INVALID;
    // FIXME: not sure what "hasImplicitScalingType" for, this cause the
    // in-consistent between encoder and decoder. Still using it to keep it the
    // same as before...
    if (!hasImplicitScalingType(scalingType, dri)) {
      scalingType = dri.type = decodeSrcType<S>();
    }

    if (scalingType == Type::INVALID) {
      scalingType = Type::UB;
      if (m_opSpec->isBranching())
        scalingType = Type::D;
    }

    dri.regRef.subRegNum = (uint16_t)BinaryOffsetToSubReg(
        dri.regRef.subRegNum, dri.regName, scalingType, m_model.platform);

    return dri;
  }

  template <SourceIndex S> uint32_t decodeSrcVertStride();
  template <SourceIndex S> uint32_t decodeSrcWidth();
  template <SourceIndex S> uint32_t decodeSrcHorzStride();
  template <SourceIndex S> uint32_t decodeSrcChanSel();
  template <SourceIndex S> GED_REP_CTRL decodeSrcRepCtrl();
  template <SourceIndex S> uint8_t decodeSrcCtxSvRstAccBitsToRegNum();
  void decodeChSelToSwizzle(uint32_t chanSel, GED_SWIZZLE swizzle[4]);
  template <SourceIndex S> bool isChanSelPacked();

  void decodeOptions(Instruction *inst);

protected:
  GED_MODEL m_gedModel;

  // decode-level state (valid below decodeKernel variants)
  Kernel *m_kernel = nullptr;

  // state shared below decodeInstToBlock()
  // info about the instruction being converted to IGA IR
  ged_ins_t m_currGedInst;
  const OpSpec *m_opSpec = nullptr;
  Subfunction m_subfunc;
  const void *m_binary = nullptr;

  // SWSB encoding mode
  SWSB_ENCODE_MODE m_SWSBEncodeMode = SWSB_ENCODE_MODE::SWSBInvalidMode;

  // for GED workarounds: grab specific bits from the current instruction
  uint32_t getBitField(int ix, int len) const;

  // helper for inserting instructions for decode errors
  Instruction *createErrorInstruction(Kernel &kernel, const char *message,
                                      const void *binary, int32_t iLen);

  // used by the GED_DECODE macros
  void handleGedDecoderError(int line, const char *field,
                             GED_RETURN_VALUE status);
}; // end class Decoder

} // namespace iga

namespace iga {
typedef Decoder Decoder;
}

#endif // end: _IGA_DECODER_H_
