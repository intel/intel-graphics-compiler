/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Decoder.hpp"
#include "../../Frontend/Formatter.hpp"
#include "../../Frontend/IRToString.hpp"
#include "../../IR/Checker/IRChecker.hpp"
#include "../../IR/Messages.hpp"
#include "../../IR/SWSBSetter.hpp"
#include "../../MemManager/MemManager.hpp"
#include "../../asserts.hpp"
#include "../../strings.hpp"
#include "GEDToIGATranslation.hpp"
#include "IGAToGEDTranslation.hpp"

#include <cstring>
#include <sstream>


// Used to label expressions that need to be removed once GED is fixed
#define GED_WORKAROUND(X) (X)

using namespace ::iga;

DEFINE_GED_SOURCE_ACCESSORS_01(GED_ADDR_MODE, AddrMode)
DEFINE_GED_SOURCE_ACCESSORS_01(int32_t, AddrImm)
DEFINE_GED_SOURCE_ACCESSORS_01(uint32_t, AddrSubRegNum)
DEFINE_GED_SOURCE_ACCESSORS_01(uint32_t, Width)
DEFINE_GED_SOURCE_ACCESSORS_012(GED_DATA_TYPE, DataType)
DEFINE_GED_SOURCE_ACCESSORS_012(GED_MATH_MACRO_EXT, MathMacroExt)
DEFINE_GED_SOURCE_ACCESSORS_012(uint32_t, VertStride)
DEFINE_GED_SOURCE_ACCESSORS_012(uint32_t, ChanSel)
DEFINE_GED_SOURCE_ACCESSORS_012(GED_REP_CTRL, RepCtrl)
DEFINE_GED_SOURCE_ACCESSORS_012(GED_SRC_MOD, SrcMod)
DEFINE_GED_SOURCE_ACCESSORS_012(uint32_t, HorzStride)

DEFINE_GED_SOURCE_ACCESSORS_012(uint32_t, RegNum)
DEFINE_GED_SOURCE_ACCESSORS_012(uint32_t, SubRegNum)
DEFINE_GED_SOURCE_ACCESSORS_012(GED_REG_FILE, RegFile)
static void setImmValKind(Type t, ImmVal &val) {
  switch (t) {
  case Type::B:
    val.kind = ImmVal::Kind::S8;
    break;
  case Type::UB:
    val.kind = ImmVal::Kind::U8;
    break;
  case Type::W:
    val.kind = ImmVal::Kind::S16;
    break;
  case Type::UW:
    val.kind = ImmVal::Kind::U16;
    break;
  case Type::D:
    val.kind = ImmVal::Kind::S32;
    break;
  case Type::UD:
    val.kind = ImmVal::Kind::U32;
    break;
  case Type::Q:
    val.kind = ImmVal::Kind::S64;
    break;
  case Type::UQ:
    val.kind = ImmVal::Kind::U64;
    break;
  case Type::HF:
    val.kind = ImmVal::Kind::F16;
    break;
  case Type::F:
    val.kind = ImmVal::Kind::F32;
    break;
  case Type::DF:
    val.kind = ImmVal::Kind::F64;
    break;
  case Type::V: // fallthrough for the packed vector types
  case Type::UV:
  case Type::VF:
    val.kind = ImmVal::Kind::U32;
    break;
  default:
    break;
  }
}

static Region macroDefaultSourceRegion(int srcOpIx, const OpSpec &os,
                                       Platform p, ExecSize execSize) {
  if (os.hasImplicitSrcRegion(srcOpIx, execSize, true)) {
    return os.implicitSrcRegion(srcOpIx, execSize, true);
  } else if (srcOpIx == 2) {
    return Region::SRCXX1;
  } else {
    if (p >= Platform::XE) {
      return os.isTernary() ? Region::SRC1X0 : // XE ternary packed region
                 Region::SRC110;
    }
    return os.isTernary() ? Region::SRC2X1 : Region::SRC441;
  }
}

Decoder::Decoder(const Model &model, ErrorHandler &errHandler)
    : GEDBitProcessor(model, errHandler) {
    m_gedModel = lowerPlatform(model.platform);
    m_SWSBEncodeMode = model.getSWSBEncodeMode();
  IGA_ASSERT(m_gedModel != GED_MODEL_INVALID, "invalid GED model");
}

void Decoder::decodeSWSB(Instruction *inst) {
  if (platform() >= Platform::XE) {
    uint32_t swsbBits = 0;
    if (inst->getOp() != Op::INVALID && inst->getOp() != Op::ILLEGAL) {
      GED_DECODE_RAW_TO(SWSB, swsbBits);
    }
    // must convert the raw encoding bits to our SWSB IR
    SWSB::InstType instType = inst->getSWSBInstType(m_SWSBEncodeMode);
    SWSB sw;
    SWSB_STATUS status = sw.decode(swsbBits, m_SWSBEncodeMode, instType);

    switch (status) {
    case SWSB_STATUS::SUCCESS:
      break;
    case SWSB_STATUS::ERROR_SET_ON_VARIABLE_LENGTH_ONLY:
      errorT("SBID set is only allowed on variable latency ops");
      break;
    case SWSB_STATUS::ERROR_INVALID_SBID_VALUE:
      errorT("invalid SBID value 0x%x", swsbBits);
      break;
    case SWSB_STATUS::ERROR_ENCODE_MODE:
      errorT("invalid encoding mode for platform");
      break;
    default:
      errorT("unknown error decoding SBID value 0x%x", swsbBits);
      break;
    }
    inst->setSWSB(sw);
  }
}

Kernel *Decoder::decodeKernelBlocks(const void *binary, size_t binarySize) {
  return decodeKernel(binary, binarySize, false);
}

Kernel *Decoder::decodeKernelNumeric(const void *binary, size_t binarySize) {
  return decodeKernel(binary, binarySize, true);
}

bool Decoder::isMacro() const {
  return m_opSpec->is(Op::MADM) ||
         (m_opSpec->is(Op::MATH) && IsMacro(m_subfunc.math));
}

Kernel *Decoder::decodeKernel(const void *binary, size_t binarySize,
                              bool numericLabels) {
  m_binary = binary;
  if (binarySize == 0) {
    // edge case: empty kernel is okay
    return new Kernel(m_model);
  }
  if (binarySize < 8) {
    // bail if we don't have at least a compact instruction
    errorT("binary size is too small");
    return nullptr;
  }
  Kernel *kernel = new Kernel(m_model);

  InstList insts;
  // NOTE: we could pre-allocate instruction list here
  // (this would block allocate everything)
  // insts.reserve(binarySize / 8 + 1);

  // Pass 1. decode them all into Instruction objects
  decodeInstructions(*kernel, binary, binarySize, insts);

  if (numericLabels) {
    Block *block = kernel->createBlock();
    block->setPC(0);
    block->setID(1);
    for (Instruction *inst : insts) {
      block->appendInstruction(inst);
    }
    kernel->appendBlock(block);
  } else {
    auto blockStarts =
        Block::inferBlocks(errorHandler(), kernel->getMemManager(), insts);
    int id = 1;
    for (const auto &bitr : blockStarts) {
      bitr.second->setID(id++);
      kernel->appendBlock(bitr.second);
    }
  }
  return kernel;
}

Subfunction Decoder::decodeSubfunction(bool &valid) {
  Subfunction sf = InvalidFC::INVALID;
  const auto errsBefore = getErrorCount();
  switch (m_opSpec->op) {
  case Op::IF:
  case Op::ELSE:
  case Op::GOTO: {
    BranchCntrl bc = BranchCntrl::OFF;
    if (m_opSpec->supportsBranchCtrl()) {
      // HSW doesn't support this
      GED_DECODE_TO(BranchCtrl, translate, bc);
    }
    sf = bc;
    break;
  }
  case Op::MATH: {
    GED_DECODE(MathFC, GED_MATH_FC, mathFc, MathFC);
    sf = mathFc;
    if (mathFc == MathFC::INVALID) {
      errorT("invalid MathFC");
    }
    break;
  }
  case Op::BFN: {
    GED_DECODE_RAW(uint32_t, lut8, BfnFC);
    sf = BfnFC((uint8_t)lut8);
    break;
  }
  case Op::DPAS:
  case Op::DPASW:
  {
    // GED splits this field up; we fuse it as a single subfunction
    uint32_t systolicDepth;
    GED_DECODE_RAW_TO_SRC(systolicDepth, uint32_t, SystolicDepth);
    uint32_t repeatCount;
    GED_DECODE_RAW_TO_SRC(repeatCount, uint32_t, RepeatCount);
    sf = GetDpasFC(systolicDepth, repeatCount);
    break;
  }
  case Op::SYNC: {
    GED_DECODE(SyncFC, GED_SYNC_FC, syncFC, SyncFC);
    sf = syncFC;
    break;
  }
  case Op::SENDSC:
  case Op::SENDS:
    // handled in descriptor decoding
    break;
  case Op::SENDC:
  case Op::SEND:
    if (platform() >= Platform::XE) {
      GED_DECODE(SFID, GED_SFID, sfid, SFID);
      sf = sfid;
    } // else handled in descriptor decoding
    break;
  default:
    IGA_ASSERT(!m_opSpec->supportsSubfunction(),
               "we need to decode a subfunction here");
    sf = InvalidFC::INVALID;
    break;
  }

  // GED_DECODE didn't fail anywhere above
  valid = getErrorCount() == errsBefore;

  return sf;
}

const OpSpec *Decoder::decodeOpSpec(Op op) {
  auto os = &m_model.lookupOpSpec(op);
  return os;
}

// Pass 1. decode all instructions in Instruction*
void Decoder::decodeInstructions(Kernel &kernel, const void *binaryStart,
                                 size_t binarySize, InstList &insts) {
  restart();
  uint32_t nextId = 1;
  const unsigned char *binary = (const unsigned char *)binaryStart;

  int32_t bytesLeft = (int32_t)binarySize;
  while (bytesLeft > 0) {
    // need at least 4 bytes to check compaction control
    if (bytesLeft < 4) {
      warningT("unexpected padding at end of kernel");
      break;
    }
    // ensure there's enough buffer left
    int32_t iLen = getBitField(COMPACTION_CONTROL, 1) != 0 ? COMPACTED_SIZE
                                                           : UNCOMPACTED_SIZE;
    if (bytesLeft < iLen) {
      warningT("unexpected padding at end of kernel");
      break;
    }
    memset(&m_currGedInst, 0, sizeof(m_currGedInst));
    GED_RETURN_VALUE status =
        GED_DecodeIns(m_gedModel, binary, (uint32_t)binarySize, &m_currGedInst);
    Instruction *inst = nullptr;
    if (status == GED_RETURN_VALUE_NO_COMPACT_FORM) {
      errorT("error decoding instruction (no compacted form)");
      inst =
          createErrorInstruction(kernel, "unable to decompact", binary, iLen);
      // fall through: GED can sort of decode some things here
    } else if (status != GED_RETURN_VALUE_SUCCESS) {
      errorT("error decoding instruction");
      inst = createErrorInstruction(kernel, "GED error decoding instruction",
                                    binary, iLen);
    } else {
      const GED_OPCODE gedOp = GED_GetOpcode(&m_currGedInst);
      const Op op = translate(gedOp);
      m_opSpec = decodeOpSpec(op);
      if (!m_opSpec->isValid()) {
        // figure out if we failed to resolve the primary op
        // or if it's an unmapped subfunction (e.g. math function)
        auto os = m_model.lookupOpSpec(op);
        std::stringstream ss;
        if (os.isValid()) {
          ss << " invalid subfuction under " << os.mnemonic.str()
             << ": unsupported opcode on this platform";
        } else {
          ss << " ISA opcode 0x" << iga::hex((unsigned)*(const uint8_t *)binary, 2) << ""
             << ": unsupported opcode on this platform";
        }
        std::string str = ss.str();
        errorT(str);
        inst = createErrorInstruction(kernel, str.c_str(), binary, iLen);
      } else {
        bool validSf = false;
        m_subfunc = decodeSubfunction(validSf);
        if (validSf) {
          try {
            inst = decodeNextInstruction(kernel);
          } catch (const FatalError &) {
            // error is already logged
            inst = createErrorInstruction(
                kernel, errorHandler().getErrors().back().message.c_str(),
                binary, iLen);
          }
        } else {
          // error is already logged
          inst = createErrorInstruction(kernel, "invalid subfunction", binary,
                                        iLen);
        }
      }
    }
    inst->setPC(currentPc());
    inst->setID(nextId++);
    inst->setLoc(currentPc());
    insts.emplace_back(inst);
#if _DEBUG
    if (!errorHandler().hasErrors()) {
      // only validate if there weren't errors
      inst->validate();
    }
#endif
    advancePc(iLen);
    binary += iLen;
    bytesLeft -= iLen;
  }
}

void Decoder::decodeNextInstructionEpilog(Instruction *inst) {
  decodeSWSB(inst);
}

// Decodes a GED instruction to IGA IR and appends it to a given block
Instruction *Decoder::decodeNextInstruction(Kernel &kernel) {
  Instruction *inst = nullptr;

  switch (m_opSpec->format) {
  case OpSpec::NULLARY:
    if (m_opSpec->op == Op::ILLEGAL) {
      inst = kernel.createIllegalInstruction();
    } else if (m_opSpec->op == Op::NOP) {
      inst = kernel.createNopInstruction();
    } else {
      std::stringstream ss;
      ss << "at pc " << currentPc() << ": invalid operation format";
      IGA_ASSERT_FALSE(ss.str().c_str());
      return kernel.createIllegalInstruction();
    }
    break;
  case OpSpec::BASIC_UNARY_REG:
  case OpSpec::BASIC_UNARY_REGIMM:
  case OpSpec::BASIC_BINARY_REG_IMM:
  case OpSpec::BASIC_BINARY_REG_REG:
  case OpSpec::BASIC_BINARY_REG_REGIMM:
  case OpSpec::MATH_BINARY_REG_REGIMM:
    inst = decodeBasicInstruction(kernel);
    break;
  case OpSpec::JUMP_UNARY_REG:
  case OpSpec::JUMP_UNARY_IMM:
  case OpSpec::JUMP_UNARY_REGIMM:
  case OpSpec::JUMP_UNARY_CALL_REGIMM:
  case OpSpec::JUMP_BINARY_BRC:
  case OpSpec::JUMP_BINARY_IMM_IMM:
    if (m_model.supportsSimplifiedBranches()) {
      inst = decodeBranchSimplifiedInstruction(kernel);
    } else {
      inst = decodeBranchInstruction(kernel);
    }
    break;
  case OpSpec::TERNARY_REGIMM_REG_REGIMM:
    inst = decodeTernaryInstruction(kernel);
    break;
  case OpSpec::SEND_UNARY:
  case OpSpec::SEND_BINARY:
    inst = decodeSendInstruction(kernel);
    break;
  case OpSpec::SYNC_UNARY:
    if (platform() < Platform::XE) {
      inst = decodeWaitInstruction(kernel);
    } else {
      inst = decodeSyncInstruction(kernel);
    }
    break;
  default: {
    std::stringstream ss;
    ss << "at pc " << currentPc() << ": invalid operation format\n";
    ss << FormatOpBits(m_model, (const uint8_t *)m_binary + currentPc());

    IGA_ASSERT_FALSE(ss.str().c_str());
    return kernel.createIllegalInstruction();
  } // default:
  } // switch

  decodeOptions(inst);
  decodeNextInstructionEpilog(inst);

  return inst;
}

bool Decoder::hasImm64Src0Overlap() {
  if (platform() < Platform::XE)
    return false;

  // SWSB overlaps the flag modifier with src0
  // check if it's 64 bit imm
  GED_REG_FILE regFile = decodeSrcRegFile<SourceIndex::SRC0>();
  Type t = decodeSrcType<SourceIndex::SRC0>();
  return (TypeIs64b(t) && (regFile == GED_REG_FILE_IMM));
}

///////////////////////////////////////////////////////////////////////
// BASIC INSTRUCTIONS
///////////////////////////////////////////////////////////////////////

Instruction *Decoder::decodeBasicInstruction(Kernel &kernel) {
  FlagRegInfo fri = decodeFlagRegInfo(hasImm64Src0Overlap());
  Instruction *inst = kernel.createBasicInstruction(
      *m_opSpec, fri.pred, fri.reg, decodeExecSize(), decodeChannelOffset(),
      decodeMaskCtrl(), fri.modifier, m_subfunc);

  GED_ACCESS_MODE accessMode = decodeAccessMode();
  if (m_opSpec->supportsDestination()) {
    decodeBasicDestination(inst, accessMode);
  }
  switch (m_opSpec->format) {
  case OpSpec::BASIC_UNARY_REG:
  case OpSpec::BASIC_UNARY_REGIMM:
    decodeBasicUnaryInstruction(inst, accessMode);
    break;
  case OpSpec::BASIC_BINARY_REG_IMM:
  case OpSpec::BASIC_BINARY_REG_REG:
  case OpSpec::BASIC_BINARY_REG_REGIMM:
  case OpSpec::MATH_BINARY_REG_REGIMM:
    decodeSourceBasic<SourceIndex::SRC0>(inst, accessMode);
    if (inst->getSourceCount() > 1) {
      // math can have one or two ops
      decodeSourceBasic<SourceIndex::SRC1>(inst, accessMode);
    }
    break;
  default:
    std::stringstream ss;
    ss << "IGA INTERNAL ERROR: ";
    ss << FormatOpBits(m_model, (const char *)m_binary + currentPc());
    ss << ": unexpected format for basic instruction";
    IGA_ASSERT_FALSE(ss.str().c_str());
    errorT(ss.str());
    inst = kernel.createIllegalInstruction();
  }

  return inst;
}

void Decoder::decodeBasicUnaryInstruction(Instruction *inst,
                                          GED_ACCESS_MODE accessMode) {
  decodeSourceBasic<SourceIndex::SRC0>(inst, accessMode);
  if (m_opSpec->op == Op::MOVI && platform() >= Platform::GEN10) {
    // movi can takes two parameters on on this platform
    // movi (..) reg  reg  (imm|null)
    decodeSourceBasic<SourceIndex::SRC1>(inst, accessMode);
  }
}

void Decoder::decodeBasicDestinationAlign16(Instruction *inst) {
  GED_DECODE_RAW(GED_ADDR_MODE, addrMode, DstAddrMode);

  DstModifier dstMod = DstModifier::NONE;
  if (inst->getOpSpec().supportsSaturation()) {
    GED_DECODE_RAW(GED_SATURATE, mod, Saturate);
    dstMod = translate(mod);
  }

  GED_DECODE(Type, GED_DATA_TYPE, type, DstDataType);

  switch (addrMode) {
  case GED_ADDR_MODE_Direct: {
    DirRegOpInfo dri = decodeDstDirRegInfo();
    if (inst->isMacro()) {
      MathMacroExt MathMacroReg = decodeDestinationMathMacroRegFromChEn();
      inst->setMacroDestination(dstMod, dri.regName, dri.regRef, MathMacroReg,
                                Region::Horz::HZ_1, type);
    } else {
      // normal Align16 destination
      uint32_t hStride = 1;
      GED_DECODE_RAW(GED_DST_CHAN_EN, chEn, DstChanEn);
      if (dri.regName == RegName::ARF_MME &&
          isAlign16MathMacroRegisterCsrPlatform()) {
        // special access to acc2-acc9 via ChEn encoding
        // (for context save and restore)
        dri.regRef.regNum = (uint16_t)decodeDestinationRegNumAccBitsFromChEn();
      } else if (chEn == GED_DST_CHAN_EN_xyzw) {
        hStride = 1;
      } else {
        fatalT("dst: unsupported Align16 ChEn; only <1> (.xyzw) supported");
      }

      GED_DECODE_RAW(uint32_t, subRegNum, DstSubRegNum);
      inst->setDirectDestination(dstMod, dri.regName, dri.regRef,
                                 translateRgnH(hStride), type);
    }
    break;
  }
  case GED_ADDR_MODE_Indirect: {
    GED_DECODE_RAW(GED_DST_CHAN_EN, chEn, DstChanEn);
    if (chEn == GED_DST_CHAN_EN_xyzw) {
      warningT("converting unary/binary Align16 dst to equivalent Align1");
    } else {
      fatalT("unsupported Align16 Dst.ChEn (only .xyzw supported)");
    }

    GED_DECODE_RAW(int32_t, addrImm, DstAddrImm);
    GED_DECODE_RAW(uint32_t, subRegNum, DstAddrSubRegNum);
    RegRef a0(0u, subRegNum);
    inst->setIndirectDestination(dstMod, a0, (uint16_t)addrImm,
                                 Region::Horz::HZ_1, type);
    break;
  }
  default:
    fatalT("invalid addressing mode on dst");
    break;
  } // switch
}

void Decoder::decodeBasicDestinationAlign1(Instruction *inst) {
  GED_ADDR_MODE addrMode = GED_ADDR_MODE_Direct;
  GED_DECODE_RAW(GED_ADDR_MODE, taddrMode, DstAddrMode);
  addrMode = taddrMode;

  DstModifier dstMod = DstModifier::NONE;
  if (inst->getOpSpec().supportsSaturation()) {
    GED_DECODE_RAW(GED_SATURATE, mod, Saturate);
    dstMod = translate(mod);
  }

  GED_DECODE_RAW(uint32_t, hStride, DstHorzStride);
  Region::Horz rgnHzDec = translateRgnH(hStride);
  if (inst->getOpSpec().hasImplicitDstRegion(isMacro())) {
    Region::Horz rgnHzImpl =
        inst->getOpSpec().implicitDstRegion(isMacro()).getHz();
    if (rgnHzImpl != rgnHzDec) {
      warningT("dst has wrong region for binary normal form");
    }
  }

  GED_DECODE(Type, GED_DATA_TYPE, type, DstDataType);

  switch (addrMode) {
  case GED_ADDR_MODE_Direct: {
    GED_DECODE_RAW(GED_REG_FILE, regFile, DstRegFile);
    if (regFile != GED_REG_FILE_ARF && regFile != GED_REG_FILE_GRF) {
      errorT("invalid reg file on dst");
    }

    DirRegOpInfo dri = decodeDstDirRegInfo();
    if (inst->isMacro()) {
      GED_DECODE(MathMacroExt, GED_MATH_MACRO_EXT, mme, DstMathMacroExt);
      inst->setMacroDestination(dstMod, dri.regName, dri.regRef, mme, rgnHzDec,
                                type);
    } else {
      // normal Align1 destination
      // it's a normal Align1 destination
      GED_DECODE_RAW(uint32_t, subRegNum, DstSubRegNum);
      inst->setDirectDestination(dstMod, dri.regName, dri.regRef, rgnHzDec,
                                 type);
    }
    break;
  }
  case GED_ADDR_MODE_Indirect: {
    GED_DECODE_RAW(int32_t, addrImm, DstAddrImm);
    GED_DECODE_RAW(uint32_t, subRegNum, DstAddrSubRegNum);
    RegRef a0(0u, subRegNum);
    inst->setIndirectDestination(dstMod, a0, (uint16_t)addrImm, rgnHzDec, type);
    break;
  }
  default:
    fatalT("invalid addressing mode on dst");
    break;
  } // switch
}

///////////////////////////////////////////////////////////////////////
// TERNARY INSTRUCTIONS
///////////////////////////////////////////////////////////////////////
Instruction *Decoder::decodeTernaryInstruction(Kernel &kernel) {
  FlagRegInfo fri = decodeFlagRegInfo();
  Instruction *inst = kernel.createBasicInstruction(
      *m_opSpec, fri.pred, fri.reg, decodeExecSize(), decodeChannelOffset(),
      decodeMaskCtrl(), fri.modifier, m_subfunc);

  GED_ACCESS_MODE accessMode = decodeAccessMode();
  decodeTernaryInstructionOperands(kernel, inst, accessMode);

  return inst;
}

void Decoder::decodeTernaryInstructionOperands(Kernel &kernel,
                                               Instruction *inst,
                                               GED_ACCESS_MODE accessMode) {
  if (accessMode == GED_ACCESS_MODE_Align16) {
    if (m_opSpec->supportsDestination()) {
      decodeTernaryDestinationAlign16(inst);
    }
    decodeTernarySourceAlign16<SourceIndex::SRC0>(inst);
    decodeTernarySourceAlign16<SourceIndex::SRC1>(inst);
    decodeTernarySourceAlign16<SourceIndex::SRC2>(inst);
  } else {
    if (platform() >= Platform::GEN10) {
      if (m_opSpec->supportsDestination()) {
        decodeTernaryDestinationAlign1(inst);
      }
      decodeTernarySourceAlign1<SourceIndex::SRC0>(inst);
      decodeTernarySourceAlign1<SourceIndex::SRC1>(inst);
      decodeTernarySourceAlign1<SourceIndex::SRC2>(inst);
    } else {
      errorT("unexpected Align1 Ternary in current platform");
      inst = kernel.createIllegalInstruction();
    }
  }
}

void Decoder::decodeTernaryDestinationAlign16(Instruction *inst) {
  GED_DECODE_RAW(uint32_t, regNumBits, DstRegNum);
  DstModifier dstMod = DstModifier::NONE;
  if (m_opSpec->supportsSaturation()) {
    GED_DECODE_RAW(GED_SATURATE, mod, Saturate);
    dstMod = translate(mod);
  }
  GED_DECODE(Type, GED_DATA_TYPE, type, DstDataType);
  GED_DECODE_RAW(GED_REG_FILE, regFile, DstRegFile);
  GED_DECODE_RAW(GED_DST_CHAN_EN, chEn, DstChanEn);

  RegName regName = RegName::INVALID;
  RegRef regRef;
  decodeReg(-1, regFile, regNumBits, regName, regRef);

  if (inst->isMacro()) {
    MathMacroExt MathMacroReg = decodeDestinationMathMacroRegFromChEn();
    inst->setMacroDestination(dstMod, regName, regRef, MathMacroReg,
                              Region::Horz::HZ_1, type);
  } else {
    // We have to translate Align16 ternary instructions to equivalent
    // Align1 where posssible.  The goal of these translations is to
    // capture everything the IGC compiler generates.  There will be
    // valid Align16 sequences that we choose not to represent in Align1.
    //
    // CASES:
    // SIMD1: illegal (hardware disallows this, we use a SIMD4 with ChEn to
    // emulate) SIMD2: We accept .xy, .zw only if the type is :df. E.g.
    //   mad (2) r5.0.xy:df ... // means r5.0<1>
    //   mad (2) r5.0.zw:df ... // means r5.1<1>
    //  *** we convert the exec size to SIMD1 ***
    // SIMD4: This can be a true SIMD4 or an emulation of a SIMD1 scalar.
    //   We decide based on the ChEn mask.
    //   .xyzw is a SIMD4, but .{x,y,z,w} means scalar SIMD1
    //   (since other lanes are masked out)
    //   *** we convert the exec size to SIMD1 for the scalar case ***
    //  I.e.
    //    mad (4) r5.0.xyzw:f ... // gets converted cleanly
    //  to
    //    mad (4) r5.0<1>:f
    //  but
    //    mad (4) r5.0.x:f ... {NoMask) // gets treated as scalar and
    //  translates to (W) mad (1) r5.0.x
    //
    // SIMD8, SIMD16: we only accept .xyzw and .r as packed and scalar
    //   this seems to capture everything used in practice
    //
    // NOTE: this is an appalling hack, but creates the least technical
    // debt for the project.  These problems all go away in GEN10 when
    // we get Align1 ternary and Align16 fades into the sunset.
    uint8_t subregOffAlign16Elems =
        0; // in elements not bytes (add after conversion)
    switch (inst->getExecSize()) {
    case ExecSize::SIMD2:
      if (chEn != GED_DST_CHAN_EN_xyzw) {
        // this is a special case of below for DF and Q
        inst->setExecSize(ExecSize::SIMD1);
        if (chEn == GED_DST_CHAN_EN_xy && type == Type::DF) {
          subregOffAlign16Elems = 0; // dst.k.xy:df => dst.(k+0)<1>:df
        } else if (chEn == GED_DST_CHAN_EN_zw && type == Type::DF) {
          subregOffAlign16Elems = 1; // dst.k.xy:df => dst.(k+1)<1>:df
        } else {
          errorT("unsupported Align16 ternary destination for SIMD2"
                 " (must be .xywz or .{xy,zw} for :df)");
        }
      }
      break;
    case ExecSize::SIMD4:
      if (chEn != GED_DST_CHAN_EN_xyzw) {
        // with Align16, we emulate a scalar (SIMD1) by masking out
        // all, but one of the channels.
        // we must translate a SIMD4
        //         mad (4) r5.0.x:f ... {NoMask}
        // to the following
        //     (W) mad (1) r5.0<1>:f ...
        // we have to twiddle the execution size here too
        inst->setExecSize(ExecSize::SIMD1);
        switch (chEn) {
        case GED_DST_CHAN_EN_x:
          subregOffAlign16Elems = 0; // subregister is already aligned
          break;
        case GED_DST_CHAN_EN_y:
          subregOffAlign16Elems =
              1; // dst.k.y => dst.(k+1)<1> (e.g. dst.4.y => dst.5<1>)
          break;
        case GED_DST_CHAN_EN_z:
          subregOffAlign16Elems = 2; // dst.k.z => dst.(k+2)<1>
          break;
        case GED_DST_CHAN_EN_w:
          subregOffAlign16Elems = 3; // dst.k.w => dst.(k+3)<1>
          break;
        default:
          errorT("unsupported Align16 ternary destination for SIMD4"
                 " (must be .xywz or .{x,y,z,w})");
          break;
        }
      } // else { it's an .xyzw ChEn: we can leave the subregister alone }
      break;
    case ExecSize::SIMD8:
    case ExecSize::SIMD16:
    case ExecSize::SIMD32: // can appear for things like :hf, :w, or :b
      if (chEn != GED_DST_CHAN_EN_xyzw) {
        errorT("unsupported Align16 ternary destination for SIMD{8,16}"
               " (must be .xywz)");
      }
      // the access must already be aligned for .xyzw
      break;
    default:
      // SIMD1 is illegal
      errorT("unsupported Align16 ternary destination (unsupported SIMD)");
      break;
    }

    GED_DECODE_RAW(uint32_t, subRegNumBytes, DstSubRegNum);
    uint16_t subRegNumber =
        type == Type::INVALID
            ? 0
            : (uint16_t)BinaryOffsetToSubReg(subRegNumBytes, regName, type,
                                             m_model.platform);
    regRef.subRegNum = (uint16_t)(subRegNumber + subregOffAlign16Elems);
    inst->setDirectDestination(dstMod, regName, regRef, Region::Horz::HZ_1,
                               type);
  }
}

template <SourceIndex S>
void Decoder::decodeTernarySourceAlign16(Instruction *inst) {
  bool isMacro = inst->isMacro(); // madm or math.invm or math.rsqrt

  if (!isMacro && m_model.supportsAlign16MacroOnly()) {
    warningT("src", (int)S,
             ": converting Align16 to Align1 "
             "(bits will re-assemble to Align1)");
  }

  SrcModifier srcMod = decodeSrcModifier<S>();

  ///////////////////////////////////////////////////////////////////////////
  // register name, number and and register file
  // (will be GRF)
  uint32_t regNum = decodeSrcRegNum<S>();

  ///////////////////////////////////////////////////////////////////////////
  // swizzling / region
  GED_DATA_TYPE gedType;
  GED_DECODE_RAW_TO(SrcDataType, gedType);
  if (platform() >= Platform::GEN8LP &&
      (gedType == GED_DATA_TYPE_f || gedType == GED_DATA_TYPE_hf) &&
      S > SourceIndex::SRC0) {
    // CHV+ mixed mode
    gedType = decodeSrcDataType<S>();
  }
  Type type = translate(gedType);

  if (isMacro) {
    MathMacroExt MathMacroReg = decodeSrcMathMacroReg<S>();
    RegRef rr(regNum, 0u);
    Region macroDftSrcRgn = macroDefaultSourceRegion(
        (int)S, inst->getOpSpec(), platform(), inst->getExecSize());
    inst->setMacroSource(S, srcMod, RegName::GRF_R, rr, MathMacroReg,
                         macroDftSrcRgn, type);
  } else {
    int subReg =
        type == Type::INVALID
            ? 0
            : BinaryOffsetToSubReg(decodeSrcSubRegNum<S>(), RegName::GRF_R,
                                   type, m_model.platform);
    RegRef reg = RegRef(regNum, (uint32_t)subReg);
    Region rgn;
    if (decodeSrcRepCtrl<S>() == GED_REP_CTRL_NoRep) {
      GED_SWIZZLE swizzle[4];
      decodeChSelToSwizzle(decodeSrcChanSel<S>(), swizzle);
      bool isFullSwizzle =
          (swizzle[0] == GED_SWIZZLE_x && swizzle[1] == GED_SWIZZLE_y &&
           swizzle[2] == GED_SWIZZLE_z && swizzle[3] == GED_SWIZZLE_w);

      bool isXYSwizzle =
          (swizzle[0] == GED_SWIZZLE_x && swizzle[1] == GED_SWIZZLE_y &&
           swizzle[2] == GED_SWIZZLE_x && swizzle[3] == GED_SWIZZLE_y);

      bool isYZSwizzle =
          (swizzle[0] == GED_SWIZZLE_z && swizzle[1] == GED_SWIZZLE_w &&
           swizzle[2] == GED_SWIZZLE_z && swizzle[3] == GED_SWIZZLE_w);

      bool invalidSwizzle = false;
      if (TypeIs64b(type)) {
        invalidSwizzle = !isFullSwizzle && !isXYSwizzle && !isYZSwizzle;
      } else {
        invalidSwizzle = !isFullSwizzle;
      }

      if (invalidSwizzle) {
        fatalT("unconvertible ternary align16 operand");
      }

      // mad (8) r46.0.xyzw:df r46.0.xyzw:df r50.0.xyzw:df r48.0.xyzw:df
      // {Align16, Q1} mad (2) r5.0.xy:df r5.0.xyxy:df r92.2.xyxy:df
      // r93.0.xyxy:df {Align16, Q1, NoMask} a HW hack for scalar operation on
      // DF
      if (type == Type::DF && (isXYSwizzle || isYZSwizzle)) {
        if (S == SourceIndex::SRC2) {
          rgn = Region::SRCXX0;
        } else {
          rgn = Region::SRC0X0;
        }
        if (isYZSwizzle) {
          reg.subRegNum += 1;
        }
      } else {
        // we accept r#.xyzw as r#<2;1>
        if (S == SourceIndex::SRC2) {
          rgn = Region::SRCXX1;
        } else {
          rgn = Region::SRC2X1;
        }
      }
    } else {
      // r#.r is the same as r#<0;0>
      if (S == SourceIndex::SRC2) {
        rgn = Region::SRCXX0;
      } else {
        rgn = Region::SRC0X0;
      }
    }
    inst->setDirectSource(S, srcMod, RegName::GRF_R, reg, rgn, type);
  }
}

static bool ternaryDstOmitsHzStride(const OpSpec &os) {
  return os.isDpasFormat();
}

void Decoder::decodeTernaryDestinationAlign1(Instruction *inst) {
  const OpSpec &os = inst->getOpSpec();

  DstModifier dstMod = DstModifier::NONE;
  if (os.supportsSaturation()) {
    GED_DECODE_RAW(GED_SATURATE, mod, Saturate);
    dstMod = translate(mod);
  }

  DirRegOpInfo dri = decodeDstDirRegInfo();

  if (inst->isMacro()) {
    GED_DECODE(MathMacroExt, GED_MATH_MACRO_EXT, mme, DstMathMacroExt);
    inst->setMacroDestination(dstMod, dri.regName, dri.regRef, mme,
                              Region::Horz::HZ_1, dri.type);
  } else {
    if (ternaryDstOmitsHzStride(inst->getOpSpec())) {
      Region::Horz dftRgnHz = os.hasImplicitDstRegion(isMacro())
                                  ? os.implicitDstRegion(isMacro()).getHz()
                                  : Region::Horz::HZ_1;
      inst->setDirectDestination(dstMod, dri.regName, dri.regRef, dftRgnHz,
                                 dri.type);
    } else {
      GED_DECODE_RAW(uint32_t, hStride, DstHorzStride);

      inst->setDirectDestination(dstMod, dri.regName, dri.regRef,
                                 translateRgnH(hStride), dri.type);
    }
  }
}

template <SourceIndex S>
Region Decoder::decodeSrcRegionTernaryAlign1(const OpSpec &os) {
  uint32_t rgnVt = static_cast<uint32_t>(Region::Vert::VT_INVALID);
  bool hasRgnVt = S != SourceIndex::SRC2;
  if (hasRgnVt) {
    rgnVt = decodeSrcVertStride<S>();
  }
  //
  uint32_t rgnHz = decodeSrcHorzStride<S>();
  //
  return transateGEDtoIGARegion(
      rgnVt, static_cast<uint32_t>(Region::Width::WI_INVALID), rgnHz);
}

// get full region from dst and src type. This function must be called
// after src and dst type are decoded.
static Region retrieveReducedRegionTernary(Type srcType, Type dstType,
    Region dstRegion) {
  IGA_ASSERT(dstRegion.getHz() != Region::Horz::HZ_INVALID &&
      srcType != Type::INVALID && dstType != Type::INVALID,
      "Decoder: cannot derive src1 region");
  auto sVertStride =
      dstRegion.h * TypeSizeInBits(dstType) / TypeSizeInBits(srcType);
  switch (sVertStride) {
  case 1: return Region::SRC1X0;
  case 2: return Region::SRC4X2;
  case 4: return Region::SRC8X4;
  default:
    break;
  }
  IGA_ASSERT_FALSE("Decoder: cannot derive src1 region");
  return Region::INVALID;
}

template <SourceIndex S>
void Decoder::decodeTernarySourceAlign1(Instruction *inst) {
  if (platform() < Platform::GEN10) {
    fatalT("Align1 not available on this platform");
  }

  GED_REG_FILE regFile = decodeSrcRegFile<S>();
  const OpSpec &os = inst->getOpSpec();
  if (os.isDpasFormat()) {
    // DPAS specific
    // DPAS allowed src0 as null:
    // When Src0 is specified as null, it is treated as an immediate value of +0
    if (!(S == SourceIndex::SRC0 && regFile == GED_REG_FILE_ARF)) {
      if (regFile != GED_REG_FILE_GRF) {
        fatalT("invalid register file in src", (int)S);
      }
    }

    RegRef regRef;
    RegName regName = decodeSourceReg<S>(regRef);

    // only ARF_NULL is allowed at src0 if it's not GRF
    if (S == SourceIndex::SRC0 && regFile == GED_REG_FILE_ARF)
      if (regName != RegName::ARF_NULL)
        fatalT("non grf src0 register file must be null for this op");

    Type ty = decodeSrcType<S>();
    if (S == SourceIndex::SRC1) {
      GED_DECODE_TO(Src1Precision, translate, ty);
    } else if (S == SourceIndex::SRC2) {
      GED_DECODE_TO(Src2Precision, translate, ty);
    } // else ty is valid already for dst/src0
    regRef.subRegNum = (uint16_t)BinaryOffsetToSubReg(regRef.subRegNum, regName,
                                                      ty, m_model.platform);
    Region dftRgn =
        os.implicitSrcRegion((int)S, inst->getExecSize(), isMacro());
    inst->setDirectSource(S, decodeSrcModifier<S>(), regName, regRef, dftRgn,
                          ty);
    return;
  } // DPAS

  // regular ternary align1 source operand
  if (regFile == GED_REG_FILE_IMM) {
    Type type = decodeSrcType<S>();

    ImmVal val;
    if (platform() < Platform::GEN10) {
      val = decodeSrcImmVal(type);
    } else {
      val = decodeTernarySrcImmVal<S>(type);
    }

    inst->setImmediateSource(S, val, type);
  } else if (regFile == GED_REG_FILE_GRF || regFile == GED_REG_FILE_ARF) {
    // addressing mode is always direct in Align1 ternary
    if (inst->isMacro()) {
      if (m_model.supportsAlign16ImplicitAcc()) {
        fatalT("src", (int)S,
               ": macro instructions must be Align16 "
               "for this platform");
      }
      RegRef regRef;
      RegName regName = decodeSourceReg<S>(regRef);
      Region macroDftSrcRgn = macroDefaultSourceRegion(
          (int)S, inst->getOpSpec(), platform(), inst->getExecSize());
      inst->setMacroSource(S, decodeSrcModifier<S>(), regName, regRef,
                           decodeSrcMathMacroReg<S>(), macroDftSrcRgn,
                           decodeSrcType<S>());
    } else {
      // normal access
      std::optional<Region> rgn;
      bool hasReduceRegion = m_model.srcHasReducedRegion(static_cast<uint32_t>(S));
      if (hasReduceRegion)
        rgn = decodeSrcReducedRegionTernary<S>();
      else
        rgn = decodeSrcRegionTernaryAlign1<S>(inst->getOpSpec());

      DirRegOpInfo opInfo = decodeSrcDirRegOpInfo<S>();
      if (hasReduceRegion && !rgn) {
        rgn = retrieveReducedRegionTernary(opInfo.type,
                 inst->getDestination().getType(),
                 inst->getDestination().getRegion());
      }

      inst->setDirectSource(S, decodeSrcModifier<S>(), opInfo.regName,
                            opInfo.regRef, *rgn, opInfo.type);
    }
  } else { // GED_REG_FILE_INVALID
    fatalT("invalid register file in src", (int)S);
  }
}


///////////////////////////////////////////////////////////////////////
// SEND INSTRUCTIONS
///////////////////////////////////////////////////////////////////////
SendDesc Decoder::decodeSendExDesc() {
  // ex_desc
  GED_REG_FILE exDescRegFile = GED_REG_FILE_IMM;
  if (m_opSpec->format & OpSpec::Format::SEND_BINARY) {
    // only sends/sendsc has ExDescRegFile
    GED_DECODE_RAW_TO(ExDescRegFile, exDescRegFile);
  }

  SendDesc exDesc;
  if (exDescRegFile == GED_REG_FILE_IMM) {
    exDesc.type = SendDesc::Kind::IMM;
    GED_DECODE_RAW_TO(ExMsgDescImm, exDesc.imm);
  } else {
    // For sends GED interprets SelReg32ExDesc and returns default values
    GED_DECODE_RAW(uint32_t, subRegNum, ExDescAddrSubRegNum);
    exDesc.type = SendDesc::Kind::REG32A;
    exDesc.reg.regNum = 0; // a0 is implied
    exDesc.reg.subRegNum = (uint16_t)(subRegNum / 2);
  }
  return exDesc;
}

SendDesc Decoder::decodeSendDesc() {
  GED_REG_FILE descRegFile = GED_REG_FILE_IMM;
  GED_DECODE_RAW_TO(DescRegFile, descRegFile);
  SendDesc desc;
  if (descRegFile == GED_REG_FILE_IMM) {
    desc.type = SendDesc::Kind::IMM;
    GED_DECODE_RAW_TO(MsgDesc, desc.imm);
  } else {
    // desc register is hardwired to a0.0 (ex-desc can vary)
    desc.type = SendDesc::Kind::REG32A;
    desc.reg.regNum = 0;
    desc.reg.subRegNum = 0;
  }
  return desc;
}

static void decodeMLenRlenFromDesc(const SendDesc &desc, int &src0Len,
                                   int &dstLen) {
  if (desc.isImm()) {
    src0Len = (int)(desc.imm >> 25) & 0xF;
    dstLen = (int)(desc.imm >> 20) & 0x1F;
  }
}

void Decoder::decodeSendInfoPreXe(SendDescodeInfo &sdi) {
  if (sdi.exDesc.isImm()) {
    // in <=GEN11, it's ExDesc[3:0]
    // if the extended descriptor is immediate, we can extract it
    // from that
    sdi.sfid = sfidFromEncoding(platform(), sdi.exDesc.imm);
  } else if (sdi.exDesc.isReg()) {
    // given <=GEN11 and reg exdesc
    sdi.sfid = SFID::A0REG;
  }
  if (sdi.exDesc.isImm()) {
    sdi.src1Len = (int)(sdi.exDesc.imm >> 6) & 0x1F;
  }
  decodeMLenRlenFromDesc(sdi.desc, sdi.src0Len, sdi.dstLen);
}
void Decoder::decodeSendInfoXe(SendDescodeInfo &sdi) {
  sdi.sfid = m_subfunc.send;
  if (sdi.exDesc.isImm()) {
    sdi.src1Len = (int)(sdi.exDesc.imm >> 6) & 0x1F;
  }
  decodeMLenRlenFromDesc(sdi.desc, sdi.src0Len, sdi.dstLen);
}

void Decoder::decodeSendInfoXeHP(SendDescodeInfo &sdi) {
  sdi.sfid = m_subfunc.send;
  if (sdi.exDesc.isImm()) {
    sdi.src1Len = (int)(sdi.exDesc.imm >> 6) & 0x1F;
  }
  decodeMLenRlenFromDesc(sdi.desc, sdi.src0Len, sdi.dstLen);

  if (sdi.exDesc.isReg()) {
    // if ExBSO is set, decode Src1Length and CPS
    GED_DECODE_RAW(uint32_t, exBSO, ExBSO);
    sdi.hasExBSO = exBSO != 0;
    if (sdi.hasExBSO) {
      GED_DECODE_RAW(uint32_t, cps, CPS);
      sdi.hasCps = cps != 0;
      GED_DECODE_RAW_TO(Src1Length, sdi.src1Len);
    }
  }
}

void Decoder::decodeSendInfoXeHPG(SendDescodeInfo &sdi) {
  // This is exactly the same as XeHP except that:
  //  - all immediate descriptors encode Src1Len in the EU bits
  decodeSendInfoXeHP(sdi);
  if (sdi.exDesc.isImm()) {
    // >=XeHPG all immediate descriptors also have Src1Length
    //   clobber the value XeHP decoding set
    GED_DECODE_RAW_TO(Src1Length, sdi.src1Len);
  }
}

void Decoder::decodeSendInfoXe2(SendDescodeInfo &sdi) {
  // In Xe2 is similar to XeHP/XeHPG/XeHPC
  //  - Given an Imm ExDesc Src1Len is an EU field
  //  - When ExDesc is a reg we get the extra field
  //    ExDescImm to add more bits to the extended descriptor
  //  - CPS goes away unconditionally (this is now ExDescImm[11])
  //  - if SFID is ugm then ExBSO holds an ExDescImm bit as well
  //  - if ExDesc is a reg and SFID is not ugm, ExBSO bit denotes if
  //    Src1Len in EU field or in ExDesc
  sdi.sfid = m_subfunc.send;
  if (sdi.exDesc.isReg()) {
    GED_DECODE_RAW_TO(ExMsgDescImm, sdi.exImmOffDesc);
    sdi.hasExBSO = false;
    // GED gives us the overlaps too (ExBSO and ExDescImm[15]), so
    // we must mask them out for non-UGM
    if (m_subfunc.send != SFID::UGM) {
      // ExDescImm overlaps ExBSO for non-UGM; clear it
      sdi.exImmOffDesc &= ~(1 << 15); // ExBSO overlaps ExDescImm[15]

      // if ExBSO is set, decode Src1Length from EU field
      GED_DECODE_RAW(uint32_t, exBSO, ExBSO);
      sdi.hasExBSO = exBSO != 0;
      if (sdi.hasExBSO)
        GED_DECODE_RAW_TO(Src1Length, sdi.src1Len);
    } else {
      // for UGM, src1Length must be in EU field
      GED_DECODE_RAW_TO(Src1Length, sdi.src1Len);
    }
    sdi.exImmOffDesc &= ~(0x7 << 16); // ExDesc.AddrSubReg
  } else {
    // exDes is Imm, src1.len must be in EU field
    GED_DECODE_RAW_TO(Src1Length, sdi.src1Len);
  }
  decodeMLenRlenFromDesc(sdi.desc, sdi.src0Len, sdi.dstLen);
}

Instruction *Decoder::decodeSendInstruction(Kernel &kernel) {
  SendDescodeInfo sdi;
  sdi.desc = decodeSendDesc();
  sdi.exDesc = decodeSendExDesc();
  if (platform() < Platform::XE) {
    decodeSendInfoPreXe(sdi);
  } else if (platform() == Platform::XE) {
    decodeSendInfoXe(sdi);
  } else if (platform() == Platform::XE_HP) {
    decodeSendInfoXeHP(sdi);
  } else if (platform() >= Platform::XE_HPG &&
             platform() < Platform::XE2) {
    decodeSendInfoXeHPG(sdi);
  } else if (platform() >= Platform::XE2) {
    decodeSendInfoXe2(sdi);
  } else {
    IGA_ASSERT_FALSE("unsupported platform");
  }

  FlagRegInfo fri = decodeFlagRegInfo();
  Instruction *inst = kernel.createSendInstruction(
      *m_opSpec, sdi.sfid, fri.pred, fri.reg, decodeExecSize(),
      decodeChannelOffset(), decodeMaskCtrl(),
      sdi.exImmOffDesc,
      sdi.exDesc, sdi.desc);

  if ((m_opSpec->format & OpSpec::Format::SEND_BINARY) ==
      OpSpec::Format::SEND_BINARY) { // send is binary
    decodeSendDestination(inst);
    decodeSendSource0(inst);
    decodeSendSource1(inst);
    if (sdi.src1Len < 0 && inst->getSource(SourceIndex::SRC1).isNull()) {
      // if src1Len comes from a0.#[24:20], but src1 is null, then
      // we can still assume it's 0.
      sdi.src1Len = 0;
    }
  } else { // if (m_opSpec->isSendFormat()) {
    decodeSendDestination(inst);
    decodeSendSource0(inst);
  }

  // No fusion in XeHPC+
  bool hasFusionCtrl =
      platform() >= Platform::XE && platform() < Platform::XE_HPC;
  if (hasFusionCtrl) {
    GED_FUSION_CTRL fusionCtrl = GED_FUSION_CTRL_Normal;
    GED_DECODE_RAW_TO(FusionCtrl, fusionCtrl);
    if (fusionCtrl == GED_FUSION_CTRL_Serialized) {
      inst->addInstOpt(InstOpt::SERIALIZE);
    }
  }

  if (sdi.hasExBSO)
    inst->addInstOpt(InstOpt::EXBSO);
  if (sdi.hasCps)
    inst->addInstOpt(InstOpt::CPS);

  // in case the operand lengths come from a seprate source
  if (inst->getSrc0Length() < 0)
    inst->setSrc0Length(sdi.src0Len);
  if (inst->getSrc1Length() < 0)
    inst->setSrc1Length(sdi.src1Len);

  return inst;
}


void Decoder::decodeSendDestination(Instruction *inst) {
  GED_ACCESS_MODE accessMode = decodeAccessMode();
  GED_DECODE_RAW(GED_REG_FILE, regFile, DstRegFile);
  GED_ADDR_MODE addrMode = GED_ADDR_MODE_Direct;

  if (platform() <= Platform::GEN11) {
    GED_DECODE_RAW_TO(DstAddrMode, addrMode);
  }

  if (addrMode == GED_ADDR_MODE_Indirect) {
    if (regFile == GED_REG_FILE_GRF) {
      decodeBasicDestination(inst, accessMode);
    } else {
      errorT("error decoding instruction: SEND dst ARF");
    }
  } else {
    DirRegOpInfo dri = decodeDstDirRegInfo();

    Region::Horz rgnHz = Region::Horz::HZ_1;
    if (m_opSpec->hasImplicitDstRegion(isMacro())) {
      rgnHz = m_opSpec->implicitDstRegion(isMacro()).getHz();
    }

    inst->setDirectDestination(DstModifier::NONE, dri.regName, dri.regRef,
                               rgnHz, dri.type);
  }
}

GED_ADDR_MODE Decoder::decodeSendSource0AddressMode() {
  GED_ADDR_MODE addrMode = GED_ADDR_MODE_Direct;
  if (platform() <= Platform::GEN11) {
    addrMode = decodeSrcAddrMode<SourceIndex::SRC0>();
  }
  return addrMode;
}

void Decoder::decodeSendSource0(Instruction *inst) {
  GED_ACCESS_MODE accessMode = decodeAccessMode();
  GED_REG_FILE regFile = decodeSrcRegFile<SourceIndex::SRC0>();

  GED_ADDR_MODE addrMode = decodeSendSource0AddressMode();

  if (regFile == GED_REG_FILE_GRF && addrMode == GED_ADDR_MODE_Indirect) {
    decodeSourceBasic<SourceIndex::SRC0>(inst, accessMode);
  } else {
    DirRegOpInfo dri = decodeSrcDirRegOpInfo<SourceIndex::SRC0>();

    Region rgn =
        inst->getOpSpec().implicitSrcRegion(0, inst->getExecSize(), isMacro());
    bool hasSrcRgnEncoding =
        inst->getOpSpec().isSendFormat() && platform() < Platform::GEN9;

    hasSrcRgnEncoding &= platform() <= Platform::GEN11;

    if (hasSrcRgnEncoding) {
      // these bits are implicitly set by GED on SKL, and they disallow access
      rgn = *decodeSrcRegionVWH<SourceIndex::SRC0>();
    }

    if (dri.regName == RegName::ARF_S) {
      // Scalar src0 implies a gathering send.
      //
      // This is the only case that send can have subRegNum.
      // Decode subRegNum separately here.
      // (Scaling is always in bytes for all s0.# in sends)
      dri.regRef.subRegNum = (uint16_t)decodeSrcSubRegNum<SourceIndex::SRC0>();
      inst->setIndirectSource(SourceIndex::SRC0, SrcModifier::NONE,
                              dri.regName, dri.regRef, 0, rgn, dri.type);
    } else
      inst->setDirectSource(SourceIndex::SRC0, SrcModifier::NONE, dri.regName,
                            dri.regRef, rgn, dri.type);
  }
}

void Decoder::decodeSendSource1(Instruction *inst) {
  RegRef regRef;
  RegName regName = decodeSourceReg<SourceIndex::SRC1>(regRef);
  const OpSpec &os = inst->getOpSpec();
  Region rgn = os.implicitSrcRegion(1, inst->getExecSize(), isMacro());
  Type implSrcType = os.implicitSrcType(1, false);
  inst->setDirectSource(SourceIndex::SRC1, SrcModifier::NONE, regName, regRef,
                        rgn, implSrcType);
}


///////////////////////////////////////////////////////////////////////
// BRANCH INSTRUCTIONS
///////////////////////////////////////////////////////////////////////
Instruction *Decoder::decodeBranchInstruction(Kernel &kernel) {
  if (decodeAccessMode() == GED_ACCESS_MODE_Align16) {
    errorT("Align16 branches not supported");
    return kernel.createIllegalInstruction();
  }

  FlagRegInfo fri = decodeFlagRegInfo();
  Instruction *inst = kernel.createBranchInstruction(
      *m_opSpec, fri.pred, fri.reg, decodeExecSize(), decodeChannelOffset(),
      decodeMaskCtrl(), m_subfunc);

  if (m_opSpec->op == Op::JMPI) {
    //   jmpi (1) JIP
    // is encoded as:
    //   jmpi (1) ip ip JIP
    GED_REG_FILE regFile = GED_REG_FILE_INVALID;

    GED_DECODE_RAW(GED_REG_FILE, regTFile, Src1RegFile);
    regFile = regTFile;

    // TODO: make and use m_opSpec->hasImplicit{Source,Destination}()
    if (regFile != GED_REG_FILE_IMM) {
      if (m_model.supportsSrc1CtrlFlow()) {
        decodeSourceBasic<SourceIndex::SRC1>(inst, SourceIndex::SRC0,
                                             GED_ACCESS_MODE_Align1);
      } else {
        Region rgn = *decodeSrcRegionVWH<SourceIndex::SRC0>();
        DirRegOpInfo opInfo = decodeSrcDirRegOpInfo<SourceIndex::SRC0>();
        inst->setDirectSource(SourceIndex::SRC0, SrcModifier::NONE,
                              opInfo.regName, opInfo.regRef, rgn, opInfo.type);
      }
    } else {
      GED_DECODE_RAW(int32_t, jip, JIP);
      // jmpi is stored post-increment; normalize it to pre-increment
      jip += GED_InsSize(&m_currGedInst);
      Type dataType = Type::INVALID;
      if (m_model.supportsSrc1CtrlFlow()) {
        dataType = decodeSrcType<SourceIndex::SRC1>();
      } else {
        dataType = decodeSrcType<SourceIndex::SRC0>();
      }
      inst->setLabelSource(SourceIndex::SRC0, jip, dataType);
    }
  } else if (m_opSpec->op == Op::RET) {
    // ret encodes as:
    //   ret (..) null src0
    // we leave then null implicit
    decodeSourceBasicAlign1<SourceIndex::SRC0>(inst);
  } else if (m_opSpec->op == Op::CALL || m_opSpec->op == Op::CALLA) {
    // calla (..)  reg  imm32
    // call  (..)  reg  imm32
    // call  (..)  reg  reg32
    //
    // call can take register or immediate (jip)
    // call stores register info in src1
    decodeBasicDestinationAlign1(inst);
    GED_REG_FILE regFile = GED_REG_FILE_INVALID;
    Type srcType = Type::INVALID;

    GED_DECODE_RAW(GED_REG_FILE, regTFile, Src1RegFile);
    regFile = regTFile;
    srcType = decodeSrcType<SourceIndex::SRC1>();

    if (regFile == GED_REG_FILE_IMM) {
      // calla (..)  reg  imm32
      // call  (..)  reg  imm32
      decodeJipToSrc(inst, SourceIndex::SRC0, srcType);
    } else {
      // call (..)  reg  reg32
      decodeSourceBasicAlign1<SourceIndex::SRC1>(inst, SourceIndex::SRC0);
    }
  } else if (m_opSpec->op == Op::BRC || m_opSpec->op == Op::BRD) {
    // brc (..) lbl16 lbl16    [PreBDW]
    // brc (..) lbl32 lbl32    [BDW+]
    // brc (..) reg32          [PreHSW]
    // brc (..) reg64          [HSW]
    // brc (..) reg64          [BDW+]
    //
    // brd (..) imm16          [IVB,HSW]
    // brd (..) reg32          [IVB,HSW]
    // brd (..) lbl32          [BDW+]
    // brd (..) reg32          [BDW+]
    GED_DECODE_RAW(GED_REG_FILE, regFile, Src0RegFile);
    if (regFile == GED_REG_FILE_IMM) {
      Type type = decodeSrcType<SourceIndex::SRC0>();
      decodeJipToSrc(inst, SourceIndex::SRC0, type);
      if (m_opSpec->op == Op::BRC) {
        decodeUipToSrc1(inst, type);
      }
    } else {
      // register argument
      decodeSourceBasicAlign1<SourceIndex::SRC0>(inst);
      if (m_opSpec->op == Op::BRC) {
        // add an implicit null parameter
        inst->setSource(SourceIndex::SRC1, Operand::SRC_REG_NULL_UD);
      }
    }
  } else {
    // e.g. if, else, endif, while, cont, break, ...
    decodeJipToSrc(inst);
    if (m_opSpec->format != OpSpec::Format::JUMP_UNARY_IMM) {
      decodeUipToSrc1(inst, Type::INVALID);
    }
  }

  return inst;
}

Instruction *Decoder::decodeBranchSimplifiedInstruction(Kernel &kernel) {
  BranchCntrl branchCtrl = BranchCntrl::OFF;
  if (m_opSpec->supportsBranchCtrl()) {
    GED_DECODE_TO(BranchCtrl, translate, branchCtrl);
  }
  FlagRegInfo fri = decodeFlagRegInfo();
  Instruction *inst = kernel.createBranchInstruction(
      *m_opSpec, fri.pred, fri.reg, decodeExecSize(), decodeChannelOffset(),
      decodeMaskCtrl(), branchCtrl);

  if (inst->getOpSpec().supportsDestination()) {
    decodeBranchDestination(inst);
  }

  GED_DECODE_RAW(GED_REG_FILE, regFile, Src0RegFile);
  if (regFile != GED_REG_FILE_IMM) {
    Region rgn = m_opSpec->implicitSrcRegion(0, inst->getExecSize(), isMacro());
    DirRegOpInfo opInfo = decodeSrcDirRegOpInfo<SourceIndex::SRC0>();
    inst->setDirectSource(SourceIndex::SRC0, SrcModifier::NONE, opInfo.regName,
                          opInfo.regRef, rgn, opInfo.type);
  } else {
    decodeJipToSrc(
        inst, SourceIndex::SRC0,
        m_opSpec->implicitSrcType(static_cast<int>(SourceIndex::SRC0), false));
  }
  // brc/brd read both UIP and JIP from one register (64-bits)
  bool isReg64 = ((m_opSpec->op == Op::BRC || m_opSpec->op == Op::BRD) &&
                  regFile == GED_REG_FILE_GRF);
  bool isUnary = (m_opSpec->format & OpSpec::Format::UNARY) != 0;
  if (!isReg64 && !isUnary) {
    decodeUipToSrc1(inst, Type::INVALID);
  }
  return inst;
}

void Decoder::decodeBranchDestination(Instruction *inst) {
  DirRegOpInfo dri = decodeDstDirRegInfo();
  Type dty = Type::UD;
  if (inst->getOpSpec().hasImplicitDstType()) {
    dty = m_opSpec->implicitDstType();
  }
  inst->setDirectDestination(DstModifier::NONE, dri.regName, dri.regRef,
                             Region::Horz::HZ_1, dty);
}

///////////////////////////////////////////////////////////////////////
// OTHER INSTRUCTIONS
///////////////////////////////////////////////////////////////////////
Instruction *Decoder::decodeWaitInstruction(Kernel &kernel) {
  // wait encodes as
  //   wait (..) nreg  nreg  null
  GED_ACCESS_MODE accessMode = decodeAccessMode();
  FlagRegInfo fri = decodeFlagRegInfo();
  Instruction *inst = kernel.createBasicInstruction(
      *m_opSpec, fri.pred, fri.reg, decodeExecSize(), decodeChannelOffset(),
      decodeMaskCtrl(), fri.modifier, m_subfunc);
  decodeSourceBasic<SourceIndex::SRC0>(inst, accessMode);
  return inst;
}

Instruction *Decoder::decodeSyncInstruction(Kernel &kernel) {
  FlagRegInfo fri = decodeFlagRegInfo();
  Instruction *inst = kernel.createBasicInstruction(
      *m_opSpec, fri.pred, fri.reg, decodeExecSize(), decodeChannelOffset(),
      decodeMaskCtrl(), fri.modifier, m_subfunc);
  GED_REG_FILE regFile = decodeSrcRegFile<SourceIndex::SRC0>();

  if (regFile == GED_REG_FILE_ARF) {
    // e.g.
    //   sync.nop     null
    //   sync.allrd   null
    //   ...
    // Since XeHPC, sync.bar supports flag src0
    if (platform() >= Platform::XE_HPC) {
      decodeSourceBasic<SourceIndex::SRC0>(inst, GED_ACCESS_MODE_Align1);
    } else {
      inst->setSource(SourceIndex::SRC0, Operand::SRC_REG_NULL_UB);
    }
  } else {
    // e.g.
    //   sync.allrd   0x15
    //   ...
    decodeSourceBasic<SourceIndex::SRC0>(inst, GED_ACCESS_MODE_Align1);
  }
  return inst;
}

Predication Decoder::decodePredication() {
  Predication pred = {PredCtrl::NONE, false};
  GED_DECODE_RAW(GED_PRED_CTRL, pc, PredCtrl);
  pred.function = translate(pc);
  return pred;
}

void Decoder::decodePredInv(Predication &pred) {
  GED_DECODE_RAW(GED_PRED_INV, pi, PredInv);
  pred.inverse = (pi == GED_PRED_INV_Invert);
}

MaskCtrl Decoder::decodeMaskCtrl() {
  GED_DECODE(MaskCtrl, GED_MASK_CTRL, ctrl, MaskCtrl);
  return ctrl;
}

FlagRegInfo Decoder::decodeFlagRegInfo(bool imm64Src0Overlaps) {

  FlagRegInfo fri = {
      {PredCtrl::NONE, false}, FlagModifier::NONE, REGREF_ZERO_ZERO};
  if (m_opSpec->supportsPredication()) {
    fri.pred = decodePredication();
  }
  if (m_opSpec->supportsFlagModifier() && !imm64Src0Overlaps) {
    // XE SWSB overlaps CondModifier and Imm64 values
    GED_DECODE_RAW(GED_COND_MODIFIER, condMod, CondModifier);
    fri.modifier = translate(condMod);
  } else if (m_opSpec->is(Op::MATH) && isMacro()) {
    // math.inv and math.rsqrtm both implicitly support EO
    // currently math is the only case, and its flagModifier must be EO
    fri.modifier = FlagModifier::EO;
  }

  // For XeHPC PredIvn field only exists when
  // PredCtrl or CondCtrl (flag modifier) exits
  if (platform() >= Platform::XE_HPC) {
    if (fri.pred.function != PredCtrl::NONE ||
        fri.modifier != FlagModifier::NONE)
      decodePredInv(fri.pred);
  } else if (m_opSpec->supportsPredication()) {
    decodePredInv(fri.pred);
  }

  if (fri.pred.function != PredCtrl::NONE ||
      fri.modifier != FlagModifier::NONE) {
    GED_DECODE_RAW(uint32_t, flagRegNum, FlagRegNum);
    fri.reg.regNum = (uint16_t)flagRegNum;
    GED_DECODE_RAW(uint32_t, flagSubRegNum, FlagSubRegNum);
    fri.reg.subRegNum = (uint16_t)flagSubRegNum;
  }

  return fri;
}

ExecSize Decoder::decodeExecSize() {
  GED_DECODE_RAW(uint32_t, execSize, ExecSize);
  return translateExecSize(execSize);
}

ChannelOffset Decoder::decodeChannelOffset() {
  if (m_opSpec->supportsQtrCtrl()) {
    GED_DECODE(ChannelOffset, GED_CHANNEL_OFFSET, em, ChannelOffset);
    return em;
  } else {
    return ChannelOffset::M0;
  }
}

GED_ACCESS_MODE Decoder::decodeAccessMode() {
  if (m_model.supportsAccessMode()) {
    GED_DECODE_RAW(GED_ACCESS_MODE, accessMode, AccessMode);
    return accessMode;
  }
  return GED_ACCESS_MODE_Align1;
}

void Decoder::decodeJipToSrc(Instruction *inst, SourceIndex s, Type type) {
  inst->setLabelSource(s, decodeJip(), type);
}
void Decoder::decodeUipToSrc1(Instruction *inst, Type type) {
  inst->setLabelSource(SourceIndex::SRC1, decodeUip(), type);
}

// PreBDW JIP and UIP are in QWORDS in <GEN8 except for a few
// exceptions for the above instructions
#define PC_SCALE                                                               \
  ((platform() < Platform::GEN8 && m_opSpec->op != Op::CALL &&                 \
    m_opSpec->op != Op::CALLA && m_opSpec->op != Op::JMPI)                     \
       ? 8                                                                     \
       : 1)
int32_t Decoder::decodeJip() {
  GED_DECODE_RAW(int32_t, jip, JIP);
  return jip * PC_SCALE;
}
int32_t Decoder::decodeUip() {
  GED_DECODE_RAW(int32_t, uip, UIP);
  return uip * PC_SCALE;
}

int Decoder::decodeDestinationRegNumAccBitsFromChEn() {
  // this is used by the math macro register (implicit accumulator access)
  // and for context save and restore access to those registers
  GED_DECODE_RAW(GED_DST_CHAN_EN, chEn, DstChanEn);
  switch (chEn) {
  case GED_DST_CHAN_EN_None:
    return 0; // 0000b => mme0 (acc2)
  case GED_DST_CHAN_EN_x:
    return 1; // 0001b => mme1 (acc3)
  case GED_DST_CHAN_EN_y:
    return 2; // 0010b => mme2 (acc4)
  case GED_DST_CHAN_EN_xy:
    return 3; // 0011b => mme3 (acc5)
  case GED_DST_CHAN_EN_z:
    return 4; // 0100b => mme4 (acc6)
  case GED_DST_CHAN_EN_xz:
    return 5; // 0101b => mme5 (acc7)
  case GED_DST_CHAN_EN_yz:
    return 6; // 0110b => mme6 (acc8)
  case GED_DST_CHAN_EN_xyz:
    return 7; // 0111b => mme7 (acc9)
  //
  // every thing else unreachable because this is an explicit operand
  // not an implicit math macro acc reference
  //
  case GED_DST_CHAN_EN_w:
    return 0; // 1000b => noacc
  //
  // HACK: for context save and restore, acc9 encodes as .xyzw
  // I think this is because of
  //   mov(8) acc2:ud r103:ud        {NoMask, Align16}   //acc9
  //              ^.xyzw implied
  // Seems like it should just be .xyz
  case GED_DST_CHAN_EN_xyzw:
    return 7; // 1111b => mme7 (acc9)
  default:
    errorT("dst: invalid math macro register (from ChEn)");
    return -1;
  }
}

MathMacroExt Decoder::decodeDestinationMathMacroRegFromChEn() {
  // this is used by the math macro register (implicit accumulator) access
  // and for context save and restore access to those registers
  GED_DECODE_RAW(GED_DST_CHAN_EN, chEn, DstChanEn);
  switch (chEn) {
  case GED_DST_CHAN_EN_None:
    return MathMacroExt::MME0; // 0000b => mme0 (acc2)
  case GED_DST_CHAN_EN_x:
    return MathMacroExt::MME1; // 0001b => mme1 (acc3)
  case GED_DST_CHAN_EN_y:
    return MathMacroExt::MME2; // 0010b => mme2 (acc4)
  case GED_DST_CHAN_EN_xy:
    return MathMacroExt::MME3; // 0011b => mme3 (acc5)
  case GED_DST_CHAN_EN_z:
    return MathMacroExt::MME4; // 0100b => mme4 (acc6)
  case GED_DST_CHAN_EN_xz:
    return MathMacroExt::MME5; // 0101b => mme5 (acc7)
  case GED_DST_CHAN_EN_yz:
    return MathMacroExt::MME6; // 0110b => mme6 (acc8)
  case GED_DST_CHAN_EN_xyz:
    return MathMacroExt::MME7; // 0111b => mme7 (acc9)
  case GED_DST_CHAN_EN_w:
    return MathMacroExt::NOMME; // 1000b => nomme (noacc)
  default:
    errorT("invalid dst implicit accumulator reference (in ChEn)");
    return MathMacroExt::INVALID;
  }
}

void Decoder::decodeDstDirSubRegNum(DirRegOpInfo &dri) {
  if (isMacro() || m_opSpec->isAnySendFormat()) {
    dri.regRef.subRegNum = 0;
  } else {
    Type scalingType = dri.type;
    if (scalingType == Type::INVALID)
      scalingType = m_opSpec->isBranching() ? Type::D : Type::UB;

    GED_DECODE_RAW(uint32_t, subRegNum, DstSubRegNum);
    dri.regRef.subRegNum = (uint16_t)BinaryOffsetToSubReg(
        subRegNum, dri.regName, scalingType, m_model.platform);
  }
}

void Decoder::decodeReg(int opIx, GED_REG_FILE regFile, uint32_t regNumBits,
                        RegName &regName,
                        RegRef &regRef) // works for src or dst
{
  const char *opName = opIx == 0   ? "src0"
                       : opIx == 1 ? "src1"
                       : opIx == 2 ? "src2"
                       :
                                 "dst";
  if (regFile == GED_REG_FILE_GRF) {
    regName = RegName::GRF_R;
    regRef.regNum = (uint16_t)regNumBits;
  } else if (regFile == GED_REG_FILE_ARF) { // ARF
    regName = RegName::INVALID;
    int arfRegNum = 0;
    const RegInfo *ri = m_model.lookupArfRegInfoByRegNum((uint8_t)regNumBits);
    if (ri == nullptr) {
      errorT(opName, ": ", iga::fmtHex(regNumBits, 2),
             ": invalid arf register");
    } else {
      regName = ri->regName;
      if (!ri->decode((uint8_t)regNumBits, arfRegNum)) {
        errorT(opName, ": ", ri->syntax, arfRegNum,
               ": invalid register number ");
      }
    }
    regRef.regNum = (uint16_t)arfRegNum;
  } else { // e.g. 10b
    errorT(opName, ": invalid register file");
  }
}

DirRegOpInfo Decoder::decodeDstDirRegInfo() {
  DirRegOpInfo dri;
  dri.type = m_opSpec->implicitDstType();
  bool hasDstType = true;
  if (platform() >= Platform::XE) {
    hasDstType &= !m_opSpec->isAnySendFormat();
    hasDstType &= !m_opSpec->isBranching();
  }
  if (hasDstType) {
    dri.type = decodeDstType();
  }

  GED_DECODE_RAW(GED_REG_FILE, gedRegFile, DstRegFile);
  GED_DECODE_RAW(uint32_t, regNumBits, DstRegNum);
  decodeReg(-1, gedRegFile, regNumBits, dri.regName, dri.regRef);
  decodeDstDirSubRegNum(dri);

  return dri;
}

Type Decoder::decodeDstType() {
  GED_DECODE(Type, GED_DATA_TYPE, t, DstDataType);
  return t;
}

bool Decoder::hasImplicitScalingType(Type &type, DirRegOpInfo &dri) {
  // FIXME: when entering this function, assuming it MUST NOT be imm or label
  // src
  if (platform() >= Platform::XE &&
      (m_opSpec->isSendFormat() || m_opSpec->isBranching())) {
    dri.type =
        m_opSpec->implicitSrcType(static_cast<int>(SourceIndex::SRC0), false);
    type = Type::D;
    return true;
  }
  return false;
}

ImmVal Decoder::decodeSrcImmVal(Type t) {
  ImmVal val;
  val.kind = ImmVal::Kind::UNDEF;
  val.reset();

  GED_DECODE_RAW_TO(Imm, val.u64);
  setImmValKind(t, val);
  return val;
}

// get full region from dst and src type. This function must be called
// after src and dst type are decoded.
static Region retrieveReducedRegionVWH(Type srcType, Type dstType,
    Region dstRegion) {
  IGA_ASSERT(dstRegion.getHz() != Region::Horz::HZ_INVALID &&
      srcType != Type::INVALID && dstType != Type::INVALID,
      "Decoder: cannot derive src1 region");
  auto sVertStride =
      dstRegion.h * TypeSizeInBits(dstType) / TypeSizeInBits(srcType);
  switch (sVertStride) {
  case 1:  return Region::SRC110;
  case 2:  return Region::SRC210;
  case 4:  return Region::SRC410;
  case 8:  return Region::SRC810;
  case 16: return Region::SRC1610;
  default:
    break;
  }
  IGA_ASSERT_FALSE("Decoder: cannot derive src1 region");
  return Region::INVALID;
}

template <SourceIndex S>
void Decoder::decodeSourceBasicAlign1(Instruction *inst, SourceIndex toSrcIxE) {
  const int toSrcIx = static_cast<int>(toSrcIxE);
  GED_REG_FILE regFile = decodeSrcRegFile<S>();
  if (regFile == GED_REG_FILE_IMM) {
    // immediate operand
    Type type = decodeSrcType<S>();
    inst->setImmediateSource(toSrcIxE, decodeSrcImmVal(type), type);
  } else if (regFile == GED_REG_FILE_ARF || regFile == GED_REG_FILE_GRF) {
    // register operand
    GED_ADDR_MODE addrMode = GED_ADDR_MODE_Direct;
    addrMode = decodeSrcAddrMode<S>();

    SrcModifier srcMod = decodeSrcModifier<S>();
    // region (implicit accumulator if Align16 and <GEN11)
    Region implRgn = Region::INVALID;
    if (inst->getOpSpec().hasImplicitSrcRegion(toSrcIx, inst->getExecSize(),
                                               isMacro())) {
      implRgn = inst->getOpSpec().implicitSrcRegion(
          toSrcIx, inst->getExecSize(), isMacro());
    }
    std::optional<Region> decRgn;
    if (m_opSpec->isAnySendFormat()) {
      decRgn = implRgn;
    } else {
      decRgn = decodeSrcRegionVWH<S>();
      if (!decRgn) {
        IGA_ASSERT(m_model.srcHasReducedRegion(static_cast<uint32_t>(S)), "Invalid src region");
        decRgn = retrieveReducedRegionVWH(decodeSrcType<S>(),
            inst->getDestination().getType(), inst->getDestination().getRegion());
      }
    }
    // ensure the region matches any implicit region rules
    if (!m_opSpec->isAnySendFormat() &&
        inst->getOpSpec().hasImplicitSrcRegion(toSrcIx, inst->getExecSize(),
                                               isMacro())) {
      if (implRgn != decRgn) {
        warningT("src", (int)S, ".Rgn should have ", ToSyntax(implRgn),
                 " for binary normal form");
      }
    }

    if (addrMode == GED_ADDR_MODE_Direct) {
      if (inst->isMacro()) {
        // GEN11 macros are stored in the subregister
        if (m_model.supportsAlign16()) {
          fatalT("src", (int)S,
                 ": macro instructions must be "
                 "Align16 for this platform");
        }
        MathMacroExt mme = decodeSrcMathMacroReg<S>();
        RegRef regRef{0, 0};
        RegName regName = decodeSourceReg<S>(regRef);
        inst->setMacroSource(toSrcIxE, srcMod, regName, regRef, mme, *decRgn,
                             decodeSrcType<S>());
      } else {
        // normal access
        DirRegOpInfo opInfo = decodeSrcDirRegOpInfo<S>();
        inst->setDirectSource(toSrcIxE, srcMod, opInfo.regName, opInfo.regRef,
                              *decRgn, opInfo.type);
      }
    } else if (addrMode == GED_ADDR_MODE_Indirect) {
      RegRef a0(0u, decodeSrcAddrSubRegNum<S>());
      int16_t addrImm = (uint16_t)decodeSrcAddrImm<S>();
      inst->setIndirectSource(
          toSrcIxE, srcMod,
          RegName::GRF_R, // set to GRF for indirect register access
          a0, addrImm, *decRgn, decodeSrcType<S>());
    } else { // == GED_ADDR_MODE_INVALID
      fatalT("invalid addressing mode in src", (int)S);
    }
  } else { // GED_REG_FILE_INVALID
    fatalT("invalid register file in src", (int)S);
  }
}

template <SourceIndex S>
void Decoder::decodeSourceBasicAlign16(Instruction *inst, SourceIndex toSrcIx) {
  GED_REG_FILE regFile = decodeSrcRegFile<S>();
  if (regFile == GED_REG_FILE_IMM) {
    // immediate operand
    Type type = decodeSrcType<S>();
    inst->setImmediateSource(toSrcIx, decodeSrcImmVal(type), type);
  } else if (regFile == GED_REG_FILE_ARF || regFile == GED_REG_FILE_GRF) {
    // register operand (direct or indirect)
    SrcModifier srcMod = decodeSrcModifier<S>();

    // reg and subreg (if direct)
    GED_ADDR_MODE addrMode = decodeSrcAddrMode<S>();

    // special context save/restore access to acc2-acc9
    uint32_t vs = decodeSrcVertStride<S>();

    if (addrMode == GED_ADDR_MODE_Direct) {
      DirRegOpInfo opInfo = decodeSrcDirRegOpInfo<S>();
      if (inst->isMacro()) { // math macro operand (macro inst)
        if (!((vs == 2 && opInfo.type == Type::DF) ||
              (vs == 4 && opInfo.type != Type::DF))) {
          fatalT("src", (int)S, ": inconvertible align16 operand");
        }
        // <GEN11 macros are stored in swizzle bits
        MathMacroExt MathMacroReg = decodeSrcMathMacroReg<S>();
        Region macroDftSrcRgn = macroDefaultSourceRegion(
            (int)S, inst->getOpSpec(), platform(), inst->getExecSize());
        inst->setMacroSource(toSrcIx, srcMod, opInfo.regName, opInfo.regRef,
                             MathMacroReg, macroDftSrcRgn, opInfo.type);
      } else {
        if (vs != 4) {
          fatalT("src", (int)S, ": inconvertible align16 operand");
        }
        Region rgn = Region::SRC110;
        if (opInfo.regName == RegName::ARF_MME &&
            isAlign16MathMacroRegisterCsrPlatform()) {
          // GEN8-9: context save and restore of acc3-9 hack
          // (remember acc2 is Align1 and misses this path)
          // So if we are here, it'll look like we just
          // decoded "mme0" (acc2), but really we need to consult
          // ChSel for the real mme# (acc#+2).
          uint32_t chanSel = decodeSrcChanSel<S>() & 0xF;
          // We do have to strip off the top bits of ChSel since
          // it's really only ChSel[3:0].
          //
          // the ChSel[3:0] value is taken as interpreted as the
          // mme register (e.g. we used to map 0 to 2 for acc2)
          // mme's start at 0 (mme0 is acc2).
          opInfo.regRef.regNum = (uint8_t)chanSel;
          opInfo.regRef.subRegNum = 0;
          // In GEN10, this all gets cleaned up and acc3 really is
          //   RegName[7:4] = 0101b ("acc")
          //   RegName[3:0] = 0011b ("3")
          // (Which we map back to mme1.)  Moreover, that'll be
          // Align1 code and this path won't be hit.
        } else {
          // conversion of some other Align16 (e.g math macros)
          if (isChanSelPacked<S>()) {
            fatalT("src", (int)S, ": inconvertible align16 operand");
          }
        }
        inst->setDirectSource(toSrcIx, srcMod, opInfo.regName, opInfo.regRef,
                              rgn, opInfo.type);
      }
    } else if (addrMode == GED_ADDR_MODE_Indirect) {
      if (!isChanSelPacked<S>() && vs == 4) {
        fatalT("src", (int)S, ": inconvertible align16 operand");
      }
      int32_t subRegNum = decodeSrcAddrSubRegNum<S>();
      int32_t addrImm = decodeSrcAddrImm<S>();
      RegRef indReg = {0, (uint8_t)subRegNum};
      inst->setIndirectSource(toSrcIx, srcMod, RegName::GRF_R, indReg,
                              (int16_t)addrImm, Region::SRC110,
                              decodeSrcType<S>());
    } else {
      // == GED_ADDR_MODE_INVALID
      fatalT("src", (int)S, ": invalid addressing mode");
    }
  } else { // GED_REG_FILE_INVALID
    fatalT("invalid register file in src", (int)S);
  }
}

void Decoder::decodeChSelToSwizzle(uint32_t chanSel, GED_SWIZZLE swizzle[4]) {
  GED_RETURN_VALUE status = GED_RETURN_VALUE_INVALID_FIELD;

  swizzle[0] = GED_GetSwizzleX(chanSel, m_gedModel, &status);
  if (status != GED_RETURN_VALUE_SUCCESS) {
    fatalT("swizzle X could not be retrieved");
  }
  swizzle[1] = GED_GetSwizzleY(chanSel, m_gedModel, &status);
  if (status != GED_RETURN_VALUE_SUCCESS) {
    fatalT("swizzle Y could not be retrieved");
  }
  swizzle[2] = GED_GetSwizzleZ(chanSel, m_gedModel, &status);
  if (status != GED_RETURN_VALUE_SUCCESS) {
    fatalT("swizzle Z could not be retrieved");
  }
  swizzle[3] = GED_GetSwizzleW(chanSel, m_gedModel, &status);
  if (status != GED_RETURN_VALUE_SUCCESS) {
    fatalT("swizzle W could not be retrieved");
  }
}

template <SourceIndex S> bool Decoder::isChanSelPacked() {
  uint32_t chanSel = decodeSrcChanSel<S>();
  GED_SWIZZLE swizzle[4];
  decodeChSelToSwizzle(chanSel, swizzle);
  return swizzle[0] != GED_SWIZZLE_x && swizzle[1] != GED_SWIZZLE_y &&
         swizzle[2] != GED_SWIZZLE_z && swizzle[3] != GED_SWIZZLE_w;
}

void Decoder::decodeThreadOptions(Instruction *inst, GED_THREAD_CTRL trdCntrl) {
  switch (trdCntrl) {
  case GED_THREAD_CTRL_Atomic:
    inst->addInstOpt(InstOpt::ATOMIC);
    break;
  case GED_THREAD_CTRL_Switch:
    inst->addInstOpt(InstOpt::SWITCH);
    break;
  case GED_THREAD_CTRL_NoPreempt:
    inst->addInstOpt(InstOpt::NOPREEMPT);
    break;
  case GED_THREAD_CTRL_INVALID:
  default:
    break;
  }
}

template <SourceIndex S> ImmVal Decoder::decodeTernarySrcImmVal(Type t) {
  ImmVal val;
  val.kind = ImmVal::Kind::UNDEF;
  val.reset();

  if (S == SourceIndex::SRC0) {
    GED_DECODE_RAW_TO(Src0TernaryImm, val.u64);
  } else if (S == SourceIndex::SRC2) {
    GED_DECODE_RAW_TO(Src2TernaryImm, val.u64);
  } else {
    errorT("src1: no immediate supported here on ternary instruction");
  }

  setImmValKind(t, val);

  return val;
}

void Decoder::decodeOptions(Instruction *inst) {
  const OpSpec &os = inst->getOpSpec();
  if (os.supportsAccWrEn()) {
    // * GED doesn't allow AccWrEn on send's
    // * BrnchCtrl overlaps AccWrEn, so anything using that is out
    GED_ACC_WR_CTRL accWrEn = GED_ACC_WR_CTRL_Normal;
    GED_DECODE_RAW_TO(AccWrCtrl, accWrEn);
    if (accWrEn == GED_ACC_WR_CTRL_AccWrEn) {
      inst->addInstOpt(InstOpt::ACCWREN);
    }
  }

  if (os.supportsDebugCtrl()) {
    GED_DEBUG_CTRL debugCtrl = GED_DEBUG_CTRL_Normal;
    GED_DECODE_RAW_TO(DebugCtrl, debugCtrl);
    if (debugCtrl == GED_DEBUG_CTRL_Breakpoint) {
      inst->addInstOpt(InstOpt::BREAKPOINT);
    }
  }

  if (os.isAnySendFormat()) {
    GED_EOT eot = GED_EOT_None;
    GED_DECODE_RAW_TO(EOT, eot);
    if (eot == GED_EOT_EOT) {
      inst->addInstOpt(InstOpt::EOT);
    }
  }


  if (os.supportsDepCtrl()) {
    GED_DEP_CTRL dpCtrl = GED_DEP_CTRL_Normal;
    GED_DECODE_RAW_TO(DepCtrl, dpCtrl);
    if (dpCtrl == GED_DEP_CTRL_NoDDClr) {
      inst->addInstOpt(InstOpt::NODDCLR);
    } else if (dpCtrl == GED_DEP_CTRL_NoDDChk) {
      inst->addInstOpt(InstOpt::NODDCHK);
    } else if (dpCtrl == GED_DEP_CTRL_NoDDClr_NoDDChk) {
      inst->addInstOpt(InstOpt::NODDCLR);
      inst->addInstOpt(InstOpt::NODDCHK);
    }
  }

  if (os.supportsThreadCtrl()) {
    GED_THREAD_CTRL trdCntrl = GED_THREAD_CTRL_Normal;
    GED_DECODE_RAW_TO(ThreadCtrl, trdCntrl);
    decodeThreadOptions(inst, trdCntrl);
  }

  if (m_model.supportNoSrcDepSet() && os.isAnySendFormat()) {
    GED_NO_SRC_DEP_SET srcDep;
    GED_DECODE_RAW_TO(NoSrcDepSet, srcDep);
    if (srcDep == GED_NO_SRC_DEP_SET_NoSrcDepSet) {
      inst->addInstOpt(InstOpt::NOSRCDEPSET);
    }
  }

  if (GED_IsCompact(&m_currGedInst)) {
    inst->addInstOpt(InstOpt::COMPACTED);
  }
}

Instruction *Decoder::createErrorInstruction(Kernel &kernel,
                                             const char *message,
                                             const void *binary, int32_t iLen) {
  Instruction *inst = kernel.createIllegalInstruction();

  std::stringstream ss;
  ss << FormatOpBits(m_model, binary);
  if (*message) {
    ss << ": " << message;
  }
  size_t bufLen = (size_t)ss.tellp() + 1;
  char *buf = (char *)kernel.getMemManager().alloc(bufLen);
  ss.read(buf, bufLen - 1);
  buf[bufLen - 1] = 0;

  if (iLen == 8) {
    inst->addInstOpt(InstOpt::COMPACTED);
  }
  inst->setComment(buf);
  return inst;
}

uint32_t Decoder::getBitField(int ix, int len) const {
  const uint32_t *ws = (const uint32_t *)((const char *)m_binary + currentPc());
  // shift is only well-defined for values <32, use 0xFFFFFFFF
  uint32_t mask = len >= 32 ? 0xFFFFFFFF : (1 << (uint32_t)len) - 1;
  IGA_ASSERT(len <= 32 && ((ix + len - 1) / 32 == ix / 32),
             "getBitField: bitfield spans DWord");
  return (ws[ix / 32] >> (ix % 32)) & mask;
}

void Decoder::handleGedDecoderError(int line, const char *field,
                                    GED_RETURN_VALUE status) {
  std::stringstream ss;
  ss << "GED reports ";
  if (status == GED_RETURN_VALUE_INVALID_VALUE) {
    ss << "invalid value";
  } else if (status == GED_RETURN_VALUE_INVALID_FIELD) {
    ss << "invalid field";
  } else if (status != GED_RETURN_VALUE_SUCCESS) {
    ss << "error (" << (int)status << ")";
  }
  ss << " for field " << field << " (line " << line << ")\n";
  ss << FormatOpBits(m_model, (const char *)m_binary + currentPc());
  if (status == GED_RETURN_VALUE_INVALID_VALUE) {
    // indicates something wrong with the bits given, but we can
    // continue trying to decode things
    errorT(ss.str());
  } else {
    // indicates IGA is totally wrong and we should probably bail out
    fatalT(ss.str());
  }
}

// These template class member functions are not defined in header (only
// declared in header), thus their definitions are not available to other
// .cpp.  We need to explicitly instantiate those template functions so
// the other .cpp can reference them.
template void
Decoder::decodeSourceBasicAlign16<SourceIndex::SRC0>(Instruction *inst,
                                                     SourceIndex toSrcIx);
template void
Decoder::decodeSourceBasicAlign16<SourceIndex::SRC1>(Instruction *inst,
                                                     SourceIndex toSrcIx);
template void
Decoder::decodeSourceBasicAlign1<SourceIndex::SRC0>(Instruction *inst,
                                                    SourceIndex toSrcIx);
template void
Decoder::decodeSourceBasicAlign1<SourceIndex::SRC1>(Instruction *inst,
                                                    SourceIndex toSrcIx);
