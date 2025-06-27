/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BinaryEncodingIGA.h"
#include "BuildIR.h"
#include "Common_ISA_framework.h"
#include "Timer.h"
#include "iga/IGALibrary/Frontend/FormatterJSON.hpp"
#include "iga/IGALibrary/api/igaEncoderWrapper.hpp"

#include <fstream>
#include <map>
#include <utility>

using namespace iga;
using namespace vISA;

// all the gunk related to extended send descriptors
struct SendExDescOpts {
  SendDesc exDesc;
  uint32_t exDescImmOff = 0;
  int xlen = -1;
  InstOptSet extraOpts;
};

class BinaryEncodingIGA {
  G4_Kernel &kernel;
  std::string fileName; // .dat filename
  Kernel *IGAKernel = nullptr;
  const Model *platformModel;
  const TARGET_PLATFORM platform;

public:
  BinaryEncodingIGA(vISA::G4_Kernel &k, const std::string& fname);
  ~BinaryEncodingIGA() { delete IGAKernel; }

  void SetSWSB(G4_INST *inst, SWSB &sw);

  // translates and encodes (formerly "DoAll")
  void Encode();

  void EmitJSON(int dumpJSON);

  ///////////////////////////////////////////////////////////////////////////
  // these function translate G4 IR to IGA IR
  Instruction *translateInstruction(G4_INST *g4inst, Block *&bbNew);
  void translateInstructionDst(G4_INST *g4inst, Instruction *igaInst);
  void translateInstructionBranchSrcs(G4_INST *g4inst, Instruction *igaInst,
                                      Block *&bbNew);
  void translateInstructionSrcs(G4_INST *g4inst, Instruction *igaInst);

  void FixInst();
  void *EmitBinary(size_t &binarySize);

private:
  BinaryEncodingIGA(const BinaryEncodingIGA &other);
  BinaryEncodingIGA &operator=(const BinaryEncodingIGA &other);

  std::map<G4_Label *, Block *> labelToBlockMap;

public:
  static ExecSize getIGAExecSize(int execSize);
  static ChannelOffset getIGAChannelOffset(int offset);
  static MaskCtrl getIGAMaskCtrl(bool noMask);
  static RegName getIGAARFName(G4_ArchRegKind areg);
  static Type getIGAType(G4_Type type, TARGET_PLATFORM genxPlatform);

  /// getIGAInternalPlatform - a helper function to transform visa platform to
  /// iga platform
  static Platform getIGAInternalPlatform(TARGET_PLATFORM genxPlatform);

  static std::pair<const OpSpec *, Subfunction>
  getIgaOpInfo(const G4_INST *inst, const Model *m, bool allowUnknownOp,
               const IR_Builder &builder);

private:
  static PredCtrl getIGAPredCtrl(G4_Predicate_Control g4PredCntrl);
  static Predication getIGAPredication(const G4_Predicate *predG4,
                                       G4_ExecSize execSize,
                                       const IR_Builder &builder);
  static BranchCntrl getIGABranchCntrl(bool isOn) {
    return isOn ? BranchCntrl::ON : BranchCntrl::OFF;
  }
  static DstModifier getIGADstModifier(bool sat) {
    return sat ? DstModifier::SAT : DstModifier::NONE;
  }
  static SrcModifier getIGASrcModifier(G4_SrcModifier srcMod);
  static Region::Vert getIGAVert(int vstride);
  static Region::Width getIGAWidth(int width);
  static Region::Horz getIGAHorz(int hstride);
  static Region getIGARegion(G4_SrcRegRegion *srcRegion, int srcPos);
  SWSB_ENCODE_MODE getIGASWSBEncodeMode() const;

  MathMacroExt getIGAImplAcc(G4_AccRegSel accSel) const {
    switch (accSel) {
    case ACC2:
      return MathMacroExt::MME0;
    case ACC3:
      return MathMacroExt::MME1;
    case ACC4:
      return MathMacroExt::MME2;
    case ACC5:
      return MathMacroExt::MME3;
    case ACC6:
      return MathMacroExt::MME4;
    case ACC7:
      return MathMacroExt::MME5;
    case ACC8:
      return MathMacroExt::MME6;
    case ACC9:
      return MathMacroExt::MME7;
    case NOACC:
      return MathMacroExt::NOMME;
    default:
      vISA_ASSERT_UNREACHABLE("illegal acc (mme) channel select");
      return MathMacroExt::INVALID;
    }
  }
  ImmVal::Kind getIGAImmType(G4_Type type) {
    switch (type) {
    case Type_UW:
      return ImmVal::Kind::U16;
    case Type_W:
      return ImmVal::Kind::S16;
    case Type_UD:
      return ImmVal::Kind::U32;
    case Type_D:
      return ImmVal::Kind::S32;
    case Type_UQ:
      return ImmVal::Kind::U64;
    case Type_Q:
      return ImmVal::Kind::S64;
    case Type_HF:
      return ImmVal::Kind::F16;
    case Type_F:
      return ImmVal::Kind::F32;
    case Type_DF:
      return ImmVal::Kind::F64;
    case Type_UV:
    case Type_V:
    case Type_VF:
      return ImmVal::Kind::U32;
    default:
      vISA_ASSERT_UNREACHABLE("invalid immediate type");
      return ImmVal::Kind::UNDEF;
    }
  }
  InstOptSet getIGAInstOptSet(G4_INST *inst) const;

  SendDesc getIGASendDesc(G4_InstSend *sendInst) const;
  SendExDescOpts getIGASendExDesc(G4_InstSend *sendInst) const;
  void printSendDataToFile(uint32_t src0Len,
                           uint32_t src1Len,
                           uint32_t dstLen,
                           const char *filePath) const;
  SendDesc encodeExDescImm(G4_INST *sendInst, SendExDescOpts &sdos) const;
  SendDesc encodeExDescRegA0(G4_INST *sendInst, SendExDescOpts &sdos) const;

  RegName getIGARegName(G4_Operand *opnd) const {
    G4_VarBase *base = opnd->getBase();
    vISA_ASSERT(base != nullptr, "base should not be null");
    if (base->isRegVar()) {
      G4_VarBase *phyReg = base->asRegVar()->getPhyReg();
      return phyReg->isAreg()
                 ? getIGAARFName(phyReg->asAreg()->getArchRegType())
                 : RegName::GRF_R;
    } else {
      return base->isAreg() ? getIGAARFName(base->asAreg()->getArchRegType())
                            : RegName::GRF_R;
    }
  }
  RegRef getIGARegRef(G4_Operand *opnd) const {
    RegRef regRef;
    G4_VarBase *base = opnd->getBase();
    vISA_ASSERT(base != nullptr, "base should not be null");
    if (base->isGreg()) {
      uint32_t byteAddress = opnd->getLinearizedStart();
      regRef.regNum = byteAddress / kernel.numEltPerGRF<Type_UB>();
      regRef.subRegNum =
          (byteAddress % kernel.numEltPerGRF<Type_UB>()) / opnd->getTypeSize();
    } else if (opnd->isSrcRegRegion()) {
      bool valid, subvalid;
      regRef.regNum = (uint8_t)opnd->asSrcRegRegion()->ExRegNum(valid);
      regRef.subRegNum = (uint8_t)opnd->asSrcRegRegion()->ExSubRegNum(subvalid);
    } else {
      vISA_ASSERT(opnd->isDstRegRegion(), "expect DstRegRegion");
      bool valid, subvalid;
      regRef.regNum = (uint8_t)opnd->asDstRegRegion()->ExRegNum(valid);
      regRef.subRegNum = (uint8_t)opnd->asDstRegRegion()->ExSubRegNum(subvalid);
    }
    return regRef;
  }

  Block *lookupIGABlock(G4_Label *label, Kernel &IGAKernel) {
    Block *b = nullptr;
    auto itr = labelToBlockMap.find(label);
    if (itr == labelToBlockMap.end()) {
      b = IGAKernel.createBlock();
      labelToBlockMap[label] = b;
    } else {
      b = itr->second;
    }
    return b;
  }

  void getIGAFlagInfo(G4_INST *inst, const OpSpec *opSpec, Subfunction sf,
                      Predication &pred, FlagModifier &condMod,
                      RegRef &flagReg);

  RegRef getIGAFlagReg(G4_VarBase *g4Base) const {
    RegRef reg = REGREF_INVALID;
    bool flagRegNumValid = true;
    reg.regNum = (uint8_t)g4Base->ExRegNum(flagRegNumValid);
    vISA_ASSERT(flagRegNumValid, "Unable to retrieve flag Reg Num for "
                                 "predicate or conditional modifier.");
    reg.subRegNum = (uint8_t)g4Base->asRegVar()->getPhyRegOff();
    return reg;
  }

  FlagModifier getIGAFlagModifier(G4_CondMod *cMod) const {
    if (cMod == nullptr) {
      return FlagModifier::NONE;
    }

    G4_CondModifier mod = cMod->getMod();
    switch (mod) {
    case Mod_z: // fallthrough
    case Mod_e:
      return FlagModifier::EQ;
    case Mod_nz: // fallthrough
    case Mod_ne:
      return FlagModifier::NE;
    case Mod_g:
      return FlagModifier::GT;
    case Mod_ge:
      return FlagModifier::GE;
    case Mod_l:
      return FlagModifier::LT;
    case Mod_le:
      return FlagModifier::LE;
    case Mod_o: // fallthrough
    case Mod_r:
      return FlagModifier::OV;
    case Mod_u:
      return FlagModifier::UN;
    default:
      vISA_ASSERT_UNREACHABLE("Invalid FlagModifier.");
      return FlagModifier::NONE;
    }
  }

  // get IGA type from GenPrecision
  Type getIGAPrecisionType(GenPrecision p) const {
    switch (p) {
    case GenPrecision::U1:
      return Type::U1;
    case GenPrecision::U2:
      return Type::U2;
    case GenPrecision::U4:
      return Type::U4;
    case GenPrecision::U8:
      return Type::UB;
    case GenPrecision::S2:
      return Type::S2;
    case GenPrecision::S4:
      return Type::S4;
    case GenPrecision::S8:
      return Type::B;
    case GenPrecision::FP16:
      return Type::HF;
    case GenPrecision::BF16:
      return Type::BF;
    case GenPrecision::TF32:
      return Type::TF32;
    default:
      vISA_ASSERT_UNREACHABLE("illegal Operand Precision");
      return Type::INVALID;
    }
  }

  Type getIGADpasType(G4_InstDpas *DpasInst, int SrcOprdIx) const {
    Type ty;
    switch (SrcOprdIx) {
    default:
      vISA_ASSERT_UNREACHABLE("Invalid SrcOprdIx!");
      ty = Type::INVALID;
      break;
    case 0:
    {
      G4_Operand *src0 = DpasInst->getSrc(0);
      if (src0->isNullReg()) {
        ty = getIGAType(DpasInst->getDst()->getType(), platform);
      } else {
        ty = getIGAType(src0->getType(), platform);
      }
      break;
    }
    case 1:
      ty = getIGAPrecisionType(DpasInst->getSrc1Precision());
      break;
    case 2:
      ty = getIGAPrecisionType(DpasInst->getSrc2Precision());
      break;
    }
    return ty;
  }

  RegRef getIGADpasRegRef(G4_InstDpas *DpasInst, int SrcOprdIx) const {
    G4_Operand *src = DpasInst->getSrc(SrcOprdIx);
    RegRef regref = getIGARegRef(src);
    if (SrcOprdIx == 2) {
      // By default, subRegNum is in terms of operand's type (D/UD for
      // dpas's src1/2). IGA needs it to be in terms of precision type.
      // Note that no need to do it for src1 as it must be grf-aligned!
      vISA_ASSERT((regref.subRegNum % 2) == 0,
             "Minimum alignemnt of dpas's src2 must be QW");
      uint32_t bitOffsets = regref.subRegNum * src->getTypeSize() * 8;
      uint32_t PBits =
          G4_InstDpas::GetPrecisionSizeInBits(DpasInst->getSrc2Precision());
      regref.subRegNum = bitOffsets / PBits;
    }
    return regref;
  }

  static BfnFC getBfnFC(const G4_INST *inst) {
    uint8_t funcCtrl = inst->asBfnInst()->getBooleanFuncCtrl();
    return BfnFC(funcCtrl);
  }
  static iga::SFID getSFID(const G4_INST *inst);
  static MathFC getMathFC(const G4_INST *inst);
  Type getIGAType(const G4_INST *I, Gen4_Operand_Number O, TARGET_PLATFORM P);

  void *m_kernelBuffer = nullptr;
  uint32_t m_kernelBufferSize = 0;
}; // class BinaryEncodingIGA

Platform
BinaryEncodingIGA::getIGAInternalPlatform(TARGET_PLATFORM genxPlatform) {
  Platform platform = Platform::INVALID;
  switch (genxPlatform) {
  case GENX_BDW:
    platform = Platform::GEN8;
    break;
  case GENX_CHV:
    platform = Platform::GEN8;
    break;
  case GENX_SKL:
  case GENX_BXT:
    platform = Platform::GEN9;
    break;
  case GENX_ICLLP:
    platform = Platform::GEN11;
    break;
  case GENX_TGLLP:
    platform = Platform::XE;
    break;
  case Xe_XeHPSDV:
    platform = Platform::XE_HP;
    break;
  case Xe_DG2:
  case Xe_MTL:
  case Xe_ARL:
    platform = Platform::XE_HPG;
    break;
  case Xe_PVC:
  case Xe_PVCXT:
    platform = Platform::XE_HPC;
    break;
  case Xe2:
    platform = Platform::XE2;
    break;
  case Xe3:
    platform = Platform::XE3;
    break;
  default:
    break;
  }
  return platform;
}

BinaryEncodingIGA::BinaryEncodingIGA(vISA::G4_Kernel &k, const std::string& fname)
    : kernel(k), fileName(fname), platform(k.fg.builder->getPlatform()) {
  platformModel = Model::LookupModel(getIGAInternalPlatform(platform));
  IGAKernel = new Kernel(*platformModel);
}

SWSB_ENCODE_MODE BinaryEncodingIGA::getIGASWSBEncodeMode() const {

  if (platform == TARGET_PLATFORM::Xe_MTL ||
      platform == TARGET_PLATFORM::Xe_ARL)
    return SWSB_ENCODE_MODE::ThreeDistPipeDPMath;

  return platformModel->getSWSBEncodeMode();
}

InstOptSet BinaryEncodingIGA::getIGAInstOptSet(G4_INST *inst) const {
  InstOptSet options;

  if (inst->isAccWrCtrlInst() && kernel.fg.builder->encodeAccWrEn()) {
    options.add(InstOpt::ACCWREN);
  }
  if (inst->isAtomicInst()) {
    options.add(InstOpt::ATOMIC);
  }
  if (inst->isBreakPointInst()) {
    options.add(InstOpt::BREAKPOINT);
  }
  if (inst->isCachelineAligned()) {
    options.add(InstOpt::CACHELINEALIGN);
  }
  if (inst->isNoDDChkInst()) {
    options.add(InstOpt::NODDCHK);
  }
  if (inst->isNoDDClrInst()) {
    options.add(InstOpt::NODDCLR);
  }
  if (inst->isNoPreemptInst()) {
    options.add(InstOpt::NOPREEMPT);
  }
  if (inst->isYieldInst()) {
    options.add(InstOpt::SWITCH);
  }
  if (inst->isSend()) {
    if (inst->isEOT()) {
      options.add(InstOpt::EOT);
    }
    if (inst->isNoSrcDepSet()) {
      options.add(InstOpt::NOSRCDEPSET);
    }
    if (inst->asSendInst()->isSerializedInst()) {
      options.add(InstOpt::SERIALIZE);
    }
  }

  // Force instruction to be compacted if InstOpt::COMPACTED is given
  // even if autoCompact is not given to IGA
  if (inst->isCompactedInst()) {
    options.add(InstOpt::COMPACTED);
  }

  // Force instruction to be nocompacted
  if (inst->isNoCompactedInst()) {
    options.add(InstOpt::NOCOMPACT);
  }


  return options;
}

void BinaryEncodingIGA::FixInst() {
  for (auto bb : kernel.fg) {
    for (auto iter = bb->begin(); iter != bb->end();) {
      G4_INST *inst = *iter;
      if (inst->isIntrinsic()) {
        // WA for simulation:  remove any intrinsics that should be lowered
        // before binary encoding
        vISA_ASSERT(inst->asIntrinsicInst()->getLoweredByPhase() ==
                         Phase::BinaryEncoding,
                     "Unexpected intrinsics in binary encoding");
        iter = bb->erase(iter);
      } else {
        ++iter;
      }
    }
  }
}

iga::SFID BinaryEncodingIGA::getSFID(const G4_INST *inst) {
  vISA_ASSERT(inst->isSend(), "Only send has SFID");

  G4_SendDesc *msgDesc = inst->getMsgDesc();
  auto funcID = msgDesc->getSFID();

  auto sfid = iga::SFID::INVALID;
  switch (funcID) {
  case vISA::SFID::NULL_SFID:
    sfid = iga::SFID::NULL_;
    break;
  case vISA::SFID::SAMPLER:
    sfid = iga::SFID::SMPL;
    break;
  case vISA::SFID::GATEWAY:
    sfid = iga::SFID::GTWY;
    break;
  case vISA::SFID::DP_DC2:
    sfid = iga::SFID::DC2;
    break;
  case vISA::SFID::DP_RC:
    sfid = iga::SFID::RC;
    break;
  case vISA::SFID::URB:
    sfid = iga::SFID::URB;
    break;
  case vISA::SFID::SPAWNER:
    sfid = iga::SFID::TS;
    break;
  case vISA::SFID::VME:
    sfid = iga::SFID::VME;
    break;
  case vISA::SFID::DP_CC:
    sfid = iga::SFID::DCRO;
    break;
  case vISA::SFID::DP_DC0:
    sfid = iga::SFID::DC0;
    break;
  case vISA::SFID::DP_PI:
    sfid = iga::SFID::PIXI;
    break;
  case vISA::SFID::DP_DC1:
    sfid = iga::SFID::DC1;
    break;
  case vISA::SFID::CRE:
    sfid = iga::SFID::CRE;
    break;
  case vISA::SFID::SLM:
    sfid = iga::SFID::SLM;
    break;
  case vISA::SFID::UGM:
    sfid = iga::SFID::UGM;
    break;
  case vISA::SFID::BTD:
    sfid = iga::SFID::BTD;
    break;
  case vISA::SFID::RTHW:
    sfid = iga::SFID::RTA;
    break;
  case vISA::SFID::TGM:
    sfid = iga::SFID::TGM;
    break;
  case vISA::SFID::UGML:
    sfid = iga::SFID::UGML;
    break;
  default:
    vISA_ASSERT_UNREACHABLE("Unknown SFID generated from vISA");
    break;
  }

  return sfid;
}
MathFC BinaryEncodingIGA::getMathFC(const G4_INST *inst) {
  G4_MathOp mathControlValue = inst->asMathInst()->getMathCtrl();
  switch (mathControlValue) {
  case MATH_INV:
    return MathFC::INV;
  case MATH_LOG:
    return MathFC::LOG;
  case MATH_EXP:
    return MathFC::EXP;
  case MATH_SQRT:
    return MathFC::SQT;
  case MATH_RSQ:
    return MathFC::RSQT;
  case MATH_SIN:
    return MathFC::SIN;
  case MATH_COS:
    return MathFC::COS;
  case MATH_FDIV:
    return MathFC::FDIV;
  case MATH_POW:
    return MathFC::POW;
  case MATH_INT_DIV:
    return MathFC::IDIV;
  case MATH_INT_DIV_QUOT:
    return MathFC::IQOT;
  case MATH_INT_DIV_REM:
    return MathFC::IREM;
  case MATH_INVM:
    return MathFC::INVM;
  case MATH_RSQRTM:
    return MathFC::RSQTM;
  default:
    vISA_ASSERT_UNREACHABLE("invalid math subfunction");
    return MathFC::INVALID;
  }
}


//
// Return the IGA op for the given vISA instruction <op> for platform p.
// <inst> is sometimes necessary to compute the subopcode (e.g., send)
// As vISA may call this function to access instruction properties such as
// saturation and conditional modifier and this may happen before all pseudo
// opperations are lowered, <allowUnknownOp> may be used to suppress assert;
// an invalid will be returned in this case.
//
std::pair<const OpSpec *, Subfunction>
BinaryEncodingIGA::getIgaOpInfo(const G4_INST *inst, const Model *m,
                                bool allowUnknownOp,
                                const IR_Builder &builder) {
  Platform p = m->platform;
  Op igaOp = Op::INVALID;
  Subfunction sf = InvalidFC::INVALID;
  switch (inst->opcode()) {
  case G4_illegal:
    igaOp = Op::ILLEGAL;
    break;
  case G4_mov:
    igaOp = Op::MOV;
    break;
  case G4_sel:
    igaOp = Op::SEL;
    break;
  case G4_movi:
    igaOp = Op::MOVI;
    break;
  case G4_not:
    igaOp = Op::NOT;
    break;
  case G4_and:
    igaOp = Op::AND;
    break;
  case G4_or:
    igaOp = Op::OR;
    break;
  case G4_xor:
    igaOp = Op::XOR;
    break;
  case G4_shr:
    igaOp = Op::SHR;
    break;
  case G4_shl:
    igaOp = Op::SHL;
    break;
  case G4_smov:
    igaOp = Op::SMOV;
    break;
  case G4_asr:
    igaOp = Op::ASR;
    break;
  case G4_ror:
    igaOp = Op::ROR;
    break;
  case G4_rol:
    igaOp = Op::ROL;
    break;
  case G4_cmp:
    igaOp = Op::CMP;
    break;
  case G4_cmpn:
    igaOp = Op::CMPN;
    break;
  case G4_csel:
    igaOp = Op::CSEL;
    break;
  case G4_bfrev:
    igaOp = Op::BFREV;
    break;
  case G4_bfe:
    igaOp = Op::BFE;
    break;
  case G4_bfi1:
    igaOp = Op::BFI1;
    break;
  case G4_bfi2:
    igaOp = Op::BFI2;
    break;
  case G4_jmpi:
    igaOp = Op::JMPI;
    break;
  case G4_brd:
    igaOp = Op::BRD;
    break;
  case G4_if:
    igaOp = Op::IF;
    break;
  case G4_brc:
    igaOp = Op::BRC;
    break;
  case G4_else:
    igaOp = Op::ELSE;
    break;
  case G4_endif:
    igaOp = Op::ENDIF;
    break;
  case G4_while:
    igaOp = Op::WHILE;
    break;
  case G4_break:
    igaOp = Op::BREAK;
    break;
  case G4_cont:
    igaOp = Op::CONT;
    break;
  case G4_halt:
    igaOp = Op::HALT;
    break;
  case G4_call: {
    igaOp = Op::CALL;
    // check if we're using calla for indirect call
    if (builder.supportCallaRegSrc()) {
      // check if we're doing indiret call
      auto callTarget = inst->getSrc(0);
      if (callTarget->isGreg() || (callTarget->isSrcRegRegion() &&
                                   callTarget->asSrcRegRegion()->isIndirect()))
        igaOp = Op::CALLA;
    }
    break;
  }
  case G4_return:
    igaOp = Op::RET;
    break;
  case G4_goto:
    igaOp = Op::GOTO;
    break;
  case G4_join:
    igaOp = Op::JOIN;
    break;
  case G4_wait:
    if (p >= Platform::XE) {
      igaOp = Op::SYNC;
      sf = SyncFC::BAR;
    } else {
      igaOp = Op::WAIT;
    }
    break;
  case G4_send:
    igaOp = Op::SEND;
    sf = getSFID(inst);
    break;
  case G4_sendc:
    igaOp = Op::SENDC;
    sf = getSFID(inst);
    break;
  case G4_sends:
    sf = getSFID(inst);
    if (p >= Platform::XE) {
      // G4 IR still calls send sends after Xe
      igaOp = Op::SEND;
    } else {
      igaOp = Op::SENDS;
    }
    break;
  case G4_sendsc:
    sf = getSFID(inst);
    if (p >= Platform::XE) {
      // G4 IR still calls send sends after Xe
      igaOp = Op::SENDC;
    } else {
      igaOp = Op::SENDSC;
    }
    break;
  case G4_math:
    sf = getMathFC(inst);
    igaOp = Op::MATH;
    break;
  case G4_add:
    igaOp = Op::ADD;
    break;
  case G4_mul:
    igaOp = Op::MUL;
    break;
  case G4_avg:
    igaOp = Op::AVG;
    break;
  case G4_frc:
    igaOp = Op::FRC;
    break;
  case G4_rndu:
    igaOp = Op::RNDU;
    break;
  case G4_rndd:
    igaOp = Op::RNDD;
    break;
  case G4_rnde:
    igaOp = Op::RNDE;
    break;
  case G4_rndz:
    igaOp = Op::RNDZ;
    break;
  case G4_mac:
    igaOp = Op::MAC;
    break;
  case G4_mach:
    igaOp = Op::MACH;
    if (inst->getPlatform() >= Xe_PVC && !inst->isAccWrCtrlInst()) {
      igaOp = Op::MACL;
    }
    break;
  case G4_lzd:
    igaOp = Op::LZD;
    break;
  case G4_fbh:
    igaOp = Op::FBH;
    break;
  case G4_fbl:
    igaOp = Op::FBL;
    break;
  case G4_cbit:
    igaOp = Op::CBIT;
    break;
  case G4_addc:
    igaOp = Op::ADDC;
    break;
  case G4_subb:
    igaOp = Op::SUBB;
    break;
  case G4_sad2:
    igaOp = Op::SAD2;
    break;
  case G4_sada2:
    igaOp = Op::SADA2;
    break;
  case G4_dp4:
    igaOp = Op::DP4;
    break;
  case G4_dph:
    igaOp = Op::DPH;
    break;
  case G4_dp3:
    igaOp = Op::DP3;
    break;
  case G4_dp2:
    igaOp = Op::DP2;
    break;
  case G4_dp4a:
    igaOp = Op::DP4A;
    break;
  case G4_dpas:
  case G4_dpasw: {
    igaOp = inst->opcode() == G4_dpasw ? Op::DPASW : Op::DPAS;
    G4_InstDpas *dpasInst = inst->asDpasInst();
    uint8_t D = dpasInst->getSystolicDepth();
    uint8_t C = dpasInst->getRepeatCount();
    sf = GetDpasFC(D, C);
    break;
  }
  case G4_add3:
    igaOp = Op::ADD3;
    break;
  case G4_bfn:
    igaOp = Op::BFN;
    sf = getBfnFC(inst);
    break;
  case G4_line:
    igaOp = Op::LINE;
    break;
  case G4_pln:
    igaOp = Op::PLN;
    break;
  case G4_mad:
    igaOp = Op::MAD;
    break;
  case G4_lrp:
    igaOp = Op::LRP;
    break;
  case G4_madm:
    igaOp = Op::MADM;
    break;
  case G4_nop:
    igaOp = Op::NOP;
    break;
  case G4_label:
    break;
  case G4_pseudo_mad:
    igaOp = Op::MAD;
    break;
  case G4_do:
    vISA_ASSERT(!allowUnknownOp, "G4_do is not GEN ISA OPCODE.");
    break;
  case G4_pseudo_and:
    igaOp = Op::AND;
    break;
  case G4_pseudo_or:
    igaOp = Op::OR;
    break;
  case G4_pseudo_xor:
    igaOp = Op::XOR;
    break;
  case G4_pseudo_not:
    igaOp = Op::NOT;
    break;
  case G4_pseudo_fcall:
    igaOp = Op::CALL;
    break;
  case G4_pseudo_fret:
    igaOp = Op::RET;
    break;
  case G4_pseudo_sada2:
    igaOp = Op::SADA2;
    break;
  case G4_pseudo_exit:
    vISA_ASSERT(!allowUnknownOp, "G4_pseudo_exit not GEN ISA OPCODE.");
    break;
  case G4_pseudo_fc_call:
    igaOp = Op::CALL;
    break;
  case G4_pseudo_fc_ret:
    igaOp = Op::RET;
    break;
  case G4_intrinsic:
    vISA_ASSERT(!allowUnknownOp, "G4_intrinsic not GEN ISA OPCODE.");
    break;
  case G4_sync_nop:
    igaOp = Op::SYNC;
    sf = SyncFC::NOP;
    break;
  case G4_sync_allrd:
    igaOp = Op::SYNC;
    sf = SyncFC::ALLRD;
    break;
  case G4_sync_allwr:
    igaOp = Op::SYNC;
    sf = SyncFC::ALLWR;
    break;
  case G4_sync_fence:
    igaOp = Op::SYNC;
    sf = SyncFC::FENCE;
    break;
  case G4_fcvt:
    igaOp = Op::MOV;
    break;
  case G4_srnd:
    igaOp = Op::SRND;
    break;
  case G4_NUM_OPCODE:
    vISA_ASSERT_UNREACHABLE("G4_NUM_OPCODE unreachable");
    break;
  case G4_mulh:
    vISA_ASSERT(!allowUnknownOp, "G4_mulh is not GEN ISA OPCODE.");
    break;
  case G4_madw:
    vISA_ASSERT(!allowUnknownOp, "G4_madw not GEN ISA OPCODE.");
    break;
  default:
    vISA_ASSERT(!allowUnknownOp, "INVALID opcode.");
    break;
  }
  const OpSpec *os = &m->lookupOpSpec(igaOp);
  return std::pair<const OpSpec *, Subfunction>(os, sf);
}

void BinaryEncodingIGA::SetSWSB(G4_INST *inst, SWSB &sw) {
  // Set token, e.g. $0
  using SWSBTokenType = vISA::G4_INST::SWSBTokenType;
  if (inst->tokenHonourInstruction() &&
      (inst->getTokenType() == SWSBTokenType::SB_SET)) {
    sw.tokenType = SWSB::TokenType::SET;
    sw.sbid = inst->getToken();
  }

  if (inst->opcode() == G4_mad && inst->hasNoACCSBSet()) {
    sw.spToken = SWSB::SpecialToken::NOACCSBSET;
  }
  // Set distance, e.g. A@1
  using DistanceType = vISA::G4_INST::DistanceType;
  if ((unsigned)inst->getDistance()) {
    // check the distance type for multi-dist-pipes
    if (kernel.fg.builder->hasThreeALUPipes() ||
        kernel.fg.builder->hasFourALUPipes()) {
      switch (inst->getDistanceTypeXe()) {
      case DistanceType::DIST:
        sw.distType = SWSB::DistType::REG_DIST;
        break;
      case DistanceType::DISTALL:
        sw.distType = SWSB::DistType::REG_DIST_ALL;
        break;
      case DistanceType::DISTINT:
        sw.distType = SWSB::DistType::REG_DIST_INT;
        break;
      case DistanceType::DISTFLOAT:
        sw.distType = SWSB::DistType::REG_DIST_FLOAT;
        break;
      case DistanceType::DISTLONG:
        sw.distType = SWSB::DistType::REG_DIST_LONG;
        break;
      case DistanceType::DISTMATH:
        sw.distType = SWSB::DistType::REG_DIST_MATH;
        break;
      case DistanceType::DISTS0:
        sw.distType = SWSB::DistType::REG_DIST_SCALAR;
        break;
      default:
        break;
      }
    } else {
      // there is only one pipe on single-dist-pipe platform,
      // must be REG_DIST
      sw.distType = SWSB::DistType::REG_DIST;
    }
    sw.minDist = (uint32_t)inst->getDistance();
  }

  // Set token dependency, e.g. $1.src
  if (inst->getTokenType() == SWSBTokenType::AFTER_READ ||
      inst->getTokenType() == SWSBTokenType::AFTER_WRITE) {
    uint8_t token = (uint8_t)inst->getToken();
    if (inst->getTokenType() == SWSBTokenType::AFTER_READ) {
      sw.tokenType = SWSB::TokenType::SRC;
    } else if (inst->getTokenType() == SWSBTokenType::AFTER_WRITE) {
      sw.tokenType = SWSB::TokenType::DST;
    }
    sw.sbid = token;
  }

  // Workaround: adjust send's swsb to align with HW behavior. Only adjust
  // when solely token or distance is used on the instruction.
  // - send has only SBID.src/dst --> SBID.set
  // - send has only distance     --> $0 and distance
  // This workaround can be removed once vISA doesn't produce such SWSB.
  // Currently this could happen only on EOT send.
  if (inst->isSend() &&
      !sw.verify(getIGASWSBEncodeMode(), SWSB::InstType::SEND)) {
    sw.tokenType = SWSB::TokenType::SET;
    if (sw.hasDist()) {
      // if the distance type cannot be combined with SBID.set, force
      // to ALL pipe
      if (sw.distType != SWSB::DistType::REG_DIST_ALL &&
          sw.distType != SWSB::DistType::REG_DIST_FLOAT &&
          sw.distType != SWSB::DistType::REG_DIST_INT)
        sw.distType = SWSB::DistType::REG_DIST_ALL;
    }
  }
  return;
}

void BinaryEncodingIGA::getIGAFlagInfo(G4_INST *inst, const OpSpec *opSpec,
                                       Subfunction sf, Predication &pred,
                                       FlagModifier &condMod, RegRef &flagReg) {
  G4_Predicate *predG4 = inst->getPredicate();
  G4_CondMod *condModG4 = inst->getCondMod();
  RegRef predFlag;
  [[maybe_unused]] bool hasPredFlag = false;

  if (opSpec->supportsPredication() && predG4 != nullptr) {
    pred = getIGAPredication(predG4, inst->getExecSize(), *kernel.fg.builder);
    predFlag = getIGAFlagReg(predG4->getBase());
    flagReg = predFlag;
    hasPredFlag = true;
  }

  bool hasImplicitModifier = opSpec->is(Op::MATH) && IsMacro(sf.math);

  if ((opSpec->supportsFlagModifier() || hasImplicitModifier) &&
      condModG4 != nullptr) {
    condMod = getIGAFlagModifier(condModG4);
    // in case for min/max sel instruction, it could have CondMod but has no
    // flag registers
    if (condModG4->getBase() != nullptr) {
      flagReg = getIGAFlagReg(condModG4->getBase());
      // pred and condMod Flags must be the same
      vISA_ASSERT(!hasPredFlag || predFlag == flagReg,
          "pred and condMod flags must be the same");
    }
  }
}

#if 0
static void DebugCaching(G4_Kernel &kernel)
{
    std::map<uint32_t,std::pair<Caching,Caching>> descs;
    for (auto bb : kernel.fg) {
        for (auto inst : *bb)
        {
            if (inst->isSend()) {
                G4_SendDescRaw *sd = inst->getMsgDescRaw();
                if (sd) {
                    descs[sd->getDesc()] = sd->getCaching();
                }
            }
        }
    }
    for (const auto &p : descs) {
        std::cerr << std::hex << "0x" << p.first <<
            " ===>"
            "  L1=" << std::dec << int(p.second.first) <<
            "  L3=" << std::dec << int(p.second.second) <<
            "\n";
    }
}
#endif

void BinaryEncodingIGA::Encode() {
  // DebugCaching(this->kernel);

  FixInst();
  Block *currBB = nullptr;

  auto isFirstInstLabel = [this]() {
    for (auto bb : kernel.fg) {
      for (auto inst : *bb) {
        return inst->isLabel();
      }
    }
    return false;
  };

  // Make the size of the first BB be multiple of 4 instructions, and do not
  // compact any instructions in it, so that the size of the first BB is
  // multiple of 64 bytes
  if (kernel.hasPerThreadPayloadBB() || kernel.hasComputeFFIDProlog()) {
    G4_BB *first_bb = *kernel.fg.begin();
    size_t num_inst = first_bb->size();
    vISA_ASSERT(num_inst != 0, "the first BB must not be empty");
    // label instructions don't count. Only the first instruction could be a
    // label
    if (first_bb->front()->isLabel())
      --num_inst;

    if (num_inst % 4 != 0) {
      size_t num_nop = 4 - (num_inst % 4);
      for (size_t i = 0; i < num_nop; ++i)
        first_bb->push_back(kernel.fg.builder->createNop(InstOpt_NoCompact));
    }
    // set all instruction to be NoCompact
    for (auto inst : *first_bb) {
      inst->setOptionOn(InstOpt_NoCompact);
    }
  }

  if (!isFirstInstLabel()) {
    // create a new BB if kernel does not start with label
    currBB = IGAKernel->createBlock();
    IGAKernel->appendBlock(currBB);
  }

  auto platformGen = kernel.getPlatformGeneration();
  std::list<std::pair<Instruction *, G4_INST *>> encodedInsts;
  Block *bbNew = nullptr;

  SWSB_ENCODE_MODE swsbEncodeMode = getIGASWSBEncodeMode();

  for (auto bb : this->kernel.fg) {
    for (auto inst : *bb) {
      bbNew = nullptr;
      if (inst->isLabel()) {
        // note that we create a new IGA BB per label instead of directly
        // mapping vISA BB to IGA BB, as some vISA BBs can have multiple labels
        // (e.g., multiple endifs)
        G4_Label *label = inst->getLabel();
        currBB = lookupIGABlock(label, *IGAKernel);
        IGAKernel->appendBlock(currBB);
        continue;
      }

      Instruction *igaInst = translateInstruction(inst, bbNew);
      if (!igaInst) {
        // assertion failure already reported
        continue;
      }

      igaInst->addInstOpts(getIGAInstOptSet(inst));

      if (platformGen >= PlatformGen::XE) {
        SWSB sw;
        SetSWSB(inst, sw);

        SWSB::InstType instTy = SWSB::InstType::UNKNOWN;
        if (inst->isMathPipeInst())
          instTy = SWSB::InstType::MATH;
        else if (inst->isDpas())
          instTy = SWSB::InstType::DPAS;
        else if (inst->isSend())
          instTy = SWSB::InstType::SEND;
        else
          instTy = SWSB::InstType::OTHERS;

        // Verify if swsb is in encode-able dist and token combination
        if (!sw.verify(swsbEncodeMode, instTy))
          IGA_ASSERT_FALSE("Invalid swsb dist and token combination");
        igaInst->setSWSB(sw);
      }

#if _DEBUG
      igaInst->validate();
#endif
      vASSERT(currBB);
      currBB->appendInstruction(igaInst);

      if (inst->requireNopAfter()) {
        Instruction *igaInst = IGAKernel->createNopInstruction();
        currBB->appendInstruction(igaInst);
      }

      if (bbNew) {
        // Fall through block is created.
        // So the new block needs to become current block
        // so that jump offsets can be calculated correctly
        currBB = bbNew;
      }
      // If, in future, we generate multiple binary inst
      // for a single G4_INST, then it should be safe to
      // make pair between the G4_INST and first encoded
      // binary inst.
      encodedInsts.emplace_back(igaInst, inst);
    }
  }

  kernel.setAsmCount(IGAKernel->getInstructionCount());

  if (m_kernelBuffer) {
    m_kernelBufferSize = 0;
    delete static_cast<uint8_t *>(m_kernelBuffer);
    m_kernelBuffer = nullptr;
  }

  { // time the encoding
    TIME_SCOPE(IGA_ENCODER);
    bool autoCompact = kernel.getOption(vISA_Compaction);
    if (platform == Xe_PVC)
      autoCompact = false; // PVC-A0 compaction is off (IGA only does B0+)

    KernelEncoder encoder(IGAKernel, autoCompact);
    encoder.setSWSBEncodingMode(swsbEncodeMode);

    if (kernel.getOption(vISA_EnableIGASWSB)) {
      encoder.enableIGAAutoDeps();
    }

    encoder.encode(kernel.fg.builder->criticalMsgStream());

    m_kernelBufferSize = encoder.getBinarySize();
    m_kernelBuffer = allocCodeBlock(m_kernelBufferSize);
    memcpy_s(m_kernelBuffer, m_kernelBufferSize, encoder.getBinary(),
             m_kernelBufferSize);
  }

  // encodedPC is available after encoding
  for (auto &&inst : encodedInsts) {
    inst.second->setGenOffset(inst.first->getPC());
  }
  if (kernel.hasPerThreadPayloadBB()) {
    kernel.fg.builder->getJitInfo()->offsetToSkipPerThreadDataLoad =
        kernel.getPerThreadNextOff();
  }
  if (kernel.hasCrossThreadPayloadBB()) {
    kernel.fg.builder->getJitInfo()->offsetToSkipCrossThreadDataLoad =
        kernel.getCrossThreadNextOff();
  }
  if (kernel.hasComputeFFIDProlog()) {
    // something weird will happen if kernel has both PerThreadProlog and
    // ComputeFFIDProlog
    vISA_ASSERT(!kernel.hasPerThreadPayloadBB() &&
           !kernel.hasCrossThreadPayloadBB(),
           "per thread prolog and compute prolog exist");
    kernel.fg.builder->getJitInfo()->offsetToSkipSetFFIDGP =
        kernel.getComputeFFIDGPNextOff();
    kernel.fg.builder->getJitInfo()->offsetToSkipSetFFIDGP1 =
        kernel.getComputeFFIDGP1NextOff();
  }

  int dumpJSON = kernel.fg.builder->getuint32Option(vISA_dumpIgaJson);
  if (dumpJSON) {
    EmitJSON(dumpJSON);
  }
}

void BinaryEncodingIGA::EmitJSON(int dumpJSON) {
  std::string jsonFileName = fileName + ".json";
  std::ofstream ofs(jsonFileName, std::ofstream::out);
  FormatOpts fos(*platformModel);
  fos.printJson = true;
  fos.printJsonVersion = 2; // c.f. iga/IGAJSONv2.md
  IGAKernel->resetIds();
  if (dumpJSON > 1) {
    fos.printInstDefs = true;
  }
  iga::ErrorHandler eh;
  FormatKernel(eh, ofs, fos, *IGAKernel);
}

Instruction *BinaryEncodingIGA::translateInstruction(G4_INST *g4inst,
                                                     Block *&bbNew) {
  Instruction *igaInst = nullptr;
  auto opinfo = getIgaOpInfo(g4inst, platformModel, false, *kernel.fg.builder);
  // common fields: op, predicate, flag reg, exec size, exec mask offset,
  // mask ctrl, conditional modifier
  const OpSpec *opSpec = opinfo.first;
  if (opSpec == nullptr || !opSpec->isValid()) {
    std::cerr << "INVALID opcode " << G4_Inst_Table[g4inst->opcode()].str
              << "\n";
    vISA_ASSERT(false, "INVALID OPCODE.");
    return nullptr;
  }
  Op igaOp = opSpec->op;
  Subfunction sf = opinfo.second;
  Predication pred;
  RegRef flagReg{0, 0};
  ExecSize execSize = getIGAExecSize(g4inst->getExecSize());
  ChannelOffset chOff = getIGAChannelOffset(g4inst->getMaskOffset());
  MaskCtrl maskCtrl = getIGAMaskCtrl(g4inst->opcode() == G4_jmpi ||
                                     g4inst->isWriteEnableInst());
  FlagModifier condModifier = FlagModifier::NONE;

  getIGAFlagInfo(g4inst, opSpec, sf, pred, condModifier, flagReg);

  if (opSpec->isBranching()) {
    BranchCntrl brnchCtrl = getIGABranchCntrl(g4inst->asCFInst()->isBackward());
    igaInst = IGAKernel->createBranchInstruction(
        *opSpec, pred, flagReg, execSize, chOff, maskCtrl, brnchCtrl);
  } else if (opSpec->isAnySendFormat()) {
    {
      SendDesc desc = getIGASendDesc(g4inst->asSendInst());
      SendExDescOpts sdos = getIGASendExDesc(g4inst->asSendInst());

      igaInst = IGAKernel->createSendInstruction(
          *opSpec, sf.send, pred, flagReg, execSize, chOff, maskCtrl,
          sdos.exDescImmOff,
          sdos.exDesc, desc);

      vISA_ASSERT(igaInst, "Instruction is NULL");
      if (!igaInst) {
        return nullptr;
      }

      igaInst->setSrc1Length(sdos.xlen);
      igaInst->addInstOpts(sdos.extraOpts);
    }
  } else if (opSpec->op == Op::NOP) {
    igaInst = IGAKernel->createNopInstruction();
  } else if (opSpec->op == Op::ILLEGAL) {
    igaInst = IGAKernel->createIllegalInstruction();
  } else {
    igaInst = IGAKernel->createBasicInstruction(
        *opSpec, pred, flagReg, execSize, chOff, maskCtrl, condModifier, sf);
  }

  vISA_ASSERT(igaInst, "Instruction is NULL");
  if (!igaInst) {
    // only on asserts; this should only happen if memory allocation
    // fails for some reason
    return nullptr;
  }

  int visaOff = g4inst->getVISAId();
  igaInst->setLoc(visaOff); // make IGA src off track CISA id

  if (opSpec->supportsDestination()) {
    translateInstructionDst(g4inst, igaInst);
  }

  if (opSpec->isBranching() && igaOp != Op::JMPI && igaOp != Op::RET &&
      igaOp != Op::CALL && igaOp != Op::CALLA && igaOp != Op::BRC &&
      igaOp != Op::BRD) {
    translateInstructionBranchSrcs(g4inst, igaInst, bbNew);
  } else {
    translateInstructionSrcs(g4inst, igaInst);
  }

  return igaInst;
}

void BinaryEncodingIGA::translateInstructionDst(G4_INST *g4inst,
                                                Instruction *igaInst) {
  vISA_ASSERT(g4inst->getDst(), "dst must not be null");
  G4_DstRegRegion *dst = g4inst->getDst();
  DstModifier dstModifier = getIGADstModifier(g4inst->getSaturate());
  Region::Horz hstride = getIGAHorz(dst->getHorzStride());
  Type type = getIGAType(g4inst, Opnd_dst, platform);

  // workaround for SKL bug
  // not all bits are copied from immediate descriptor
  if (g4inst->isSend() && platform >= GENX_SKL && platform < GENX_ICLLP) {
    const G4_SendDescRaw *msgDesc = g4inst->getMsgDescRaw();
    vISA_ASSERT(msgDesc, "expected raw descriptor");
    G4_Operand *descOpnd = g4inst->asSendInst()->getMsgDescOperand();
    if (!descOpnd->isImm() && msgDesc->is16BitReturn()) {
      type = Type::HF;
    }
  }

  if (igaInst->isMacro()) {
    RegRef regRef = getIGARegRef(dst);
    Region::Horz hstride = getIGAHorz(dst->getHorzStride());
    igaInst->setMacroDestination(dstModifier, getIGARegName(dst), regRef,
                                 getIGAImplAcc(dst->getAccRegSel()), hstride,
                                 type);
  } else if (dst->getRegAccess() == Direct) {
    igaInst->setDirectDestination(dstModifier, getIGARegName(dst),
                                  getIGARegRef(dst), hstride, type);
  } else { // Operand::Kind::INDIRECT
    RegRef regRef{0, 0};
    bool valid;
    regRef.subRegNum = (uint8_t)dst->ExIndSubRegNum(valid);
    igaInst->setIndirectDestination(dstModifier, regRef, dst->getAddrImm(),
                                    hstride, type);
  }
}

void BinaryEncodingIGA::translateInstructionBranchSrcs(G4_INST *inst,
                                                       Instruction *igaInst,
                                                       Block *&bbNew) {
  if (inst->asCFInst()->getJip()) {
    // encode jip/uip for branch inst
    // note that it does not apply to jmpi/call/ret/brc/brd,
    // which may have register sources. Their label appears directly as
    // source operand instead.
    G4_Operand *uip = inst->asCFInst()->getUip();
    G4_Operand *jip = inst->asCFInst()->getJip();
    // iga will take care off
    if (uip) {
      igaInst->setLabelSource(SourceIndex::SRC1,
                              lookupIGABlock(uip->asLabel(), *IGAKernel),
                              Type::UD);
    }

    igaInst->setLabelSource(SourceIndex::SRC0,
                            lookupIGABlock(jip->asLabel(), *IGAKernel),
                            Type::UD);
  } else {
    // Creating a fall through block
    bbNew = IGAKernel->createBlock();
    igaInst->setLabelSource(SourceIndex::SRC0, bbNew, Type::UD);
    IGAKernel->appendBlock(bbNew);
  }
}

void BinaryEncodingIGA::translateInstructionSrcs(G4_INST *inst,
                                                 Instruction *igaInst) {
  // set source operands
  int numSrcToEncode = inst->getNumSrc();
  if (inst->isSend()) {
    // skip desc/exdesc as they are handled separately
    numSrcToEncode = inst->asSendInst()->getNumSrcPayloads();
    if (numSrcToEncode == 1 && platformModel->platform >= Platform::XE) {
      RegRef regTemp(0, 0);
      Region rgnTemp = Region::SRC010;

      igaInst->setDirectSource(SourceIndex::SRC1, SrcModifier::NONE,
                               RegName::ARF_NULL, regTemp, rgnTemp,
                               Type::INVALID);
    }
  }
  if (platform >= GENX_ICLLP && inst->opcode() == G4_movi &&
      numSrcToEncode == 1) {
    // From ICL, 'movi' becomes a binary instruction with an
    // optional immediate operand, which needs encoding as null
    // or imm32. So far, within vISA jitter, 'movi' is still
    // modeled as unary instruction, setting src1 to null for
    // platforms >= CNL.

    // The byte/word level cross bar is removed from PVC+ so the movi cannot
    // work for less than DW types.
    iga::Type ty = platform >= Xe_PVC ? Type::UD : Type::UB;
    RegRef regTemp(0, 0);
    Region rgnTemp = Region::SRC110;
    igaInst->setDirectSource(SourceIndex::SRC1, SrcModifier::NONE,
                             RegName::ARF_NULL, regTemp, rgnTemp, ty);
  }
  for (int i = 0; i < numSrcToEncode; i++) {
    SourceIndex opIx = SourceIndex::SRC0;
    switch (i) {
    case 0:
      opIx = SourceIndex::SRC0;
      break;
    case 1:
      opIx = SourceIndex::SRC1;
      break;
    case 2:
      opIx = SourceIndex::SRC2;
      break;
    default:
      vISA_ASSERT_UNREACHABLE("invalid source index number");
      break;
    }

    G4_Operand *src = inst->getSrc(i);

    if (src->isSrcRegRegion()) {
      G4_SrcRegRegion *srcRegion = src->asSrcRegRegion();
      SrcModifier srcMod = getIGASrcModifier(srcRegion->getModifier());
      Region region = getIGARegion(srcRegion, i);
      Type type = Type::INVALID;

      // let IGA take care of types for send/s instructions
      if (!igaInst->getOpSpec().isAnySendFormat()) {
        type = getIGAType(inst, inst->getSrcOperandNum(i), platform);
      } else if (i == 0 && platform >= GENX_SKL && platform < GENX_ICLLP) {
        // work around for SKL bug
        // not all bits are copied from immediate descriptor
        G4_SendDescRaw *msgDesc = inst->getMsgDescRaw();
        vISA_ASSERT(msgDesc, "expected raw descriptor");
        G4_Operand *descOpnd = inst->asSendInst()->getMsgDescOperand();
        if (!descOpnd->isImm() && msgDesc->is16BitInput()) {
          type = Type::HF;
        }
      }

      if (igaInst->isMacro()) {
        auto accRegSel =
            srcRegion->isNullReg() ? NOACC : srcRegion->getAccRegSel();
        RegRef regRef = getIGARegRef(srcRegion);
        igaInst->setMacroSource(opIx, srcMod, getIGARegName(srcRegion), regRef,
                                getIGAImplAcc(accRegSel), region, type);
      } else if (inst->isDpas()) {
        vISA_ASSERT(srcRegion->getRegAccess() == Direct,
               "dpas does not support indirect GRF operands");
        G4_InstDpas *dpasInst = inst->asDpasInst();
        RegRef regRef = getIGADpasRegRef(dpasInst, i);
        type = getIGADpasType(dpasInst, i);

        igaInst->setDirectSource(opIx, srcMod, getIGARegName(srcRegion), regRef,
                                 region, type);
      } else if (srcRegion->getRegAccess() == Direct) {
        igaInst->setDirectSource(opIx, srcMod, getIGARegName(srcRegion),
                                 getIGARegRef(srcRegion), region, type);
      } else {
        RegRef regRef{0, 0};
        bool valid;
        regRef.subRegNum = (uint8_t)srcRegion->ExIndSubRegNum(valid);
        // set to GRF for indirect register access
        iga::RegName regName = iga::RegName::GRF_R;
        if (getIGARegName(srcRegion) == iga::RegName::ARF_S)
          regName = iga::RegName::ARF_S;

        igaInst->setIndirectSource(opIx, srcMod, regName, regRef,
                                   srcRegion->getAddrImm(), region, type);
      }
    } else if (src->isLabel()) {
      igaInst->setLabelSource(opIx, lookupIGABlock(src->asLabel(), *IGAKernel),
                              Type::UD);
    } else if (src->isImm()) {
      Type type = getIGAType(src->getType(), platform);
      ImmVal val;
      val = src->asImm()->getImm();
      val.kind = getIGAImmType(src->getType());
      igaInst->setImmediateSource(opIx, val, type);
    } else {
      IGA_ASSERT_FALSE("unexpected src kind");
    }
  } // for
}

SendDesc BinaryEncodingIGA::getIGASendDesc(G4_InstSend *sendInst) const {
  SendDesc desc;
  G4_Operand *msgDesc = sendInst->getMsgDescOperand();
  if (msgDesc->isImm()) {
    desc.type = SendDesc::Kind::IMM;
    desc.imm = (uint32_t)msgDesc->asImm()->getImm();
       // This is a WA that needs to add fence.ugm before EOT
       //     fence.ugm.invalidate.tile works, but performance for raytracing
       //     might be affected. HW team came out a way to get around of it
       //     by using reserved Flush TYPE = 6, with the following
       //            DG2_Backup_Mode = 0
       //            Flush_Range = 0
       //     (So, it is fence.ugm.6.tile !)
    auto msgDesc = sendInst->getMsgDesc();
    if (msgDesc->isLSC() && msgDesc->isFence()) {
      // Flush Type : bit[14:12]
      uint32_t flushTy = ((desc.imm >> 12) & 0x7);
      if (flushTy == 6) {
        // DG2 fence desc
        //   both backupMode(bit[18]) and flushRange(bit[15]) must be zero
        constexpr uint32_t backupMode = (1 << 18), flushRange = (1 << 15);
        desc.imm = desc.imm & ~(backupMode | flushRange);
      }
    }
  } else {
    desc.type = SendDesc::Kind::REG32A;
    desc.reg.regNum = 0; // must be a0
    bool valid = false;
    desc.reg.subRegNum = (uint8_t)msgDesc->asSrcRegRegion()->ExSubRegNum(valid);
    vISA_ASSERT(valid, "invalid subreg");
  }

  return desc;
}

///////////////////////////////////////////////////////////////////////////////
// ExDesc encoding
static SendDesc encodeExDescSendUnary(G4_InstSend *sendInst, int &xlen,
                                      InstOptSet &extraOpts) {
  SendDesc exDescIga;

  // old unary packed send
  // exDesc is stored in SendMsgDesc and must be IMM
  const G4_SendDescRaw *descG4 = sendInst->getMsgDescRaw();
  vISA_ASSERT(descG4 != nullptr, "expected raw send");

  exDescIga.type = SendDesc::Kind::IMM;
  uint32_t tVal = descG4->getExtendedDesc();

  if (sendInst->getBuilder().getPlatformGeneration() >= PlatformGen::XE) {
    // We must clear the funcID in the extended message.
    // In Xe+ this is part of the EU encoding, not the descriptor.
    // vISA/G4IR still treat it as part of the descriptor.
    tVal = tVal & 0xFFFFFFF0;

    // clear the EOT bit which is not part of exDesc
    tVal &= ~(1 << 5);
  }
  exDescIga.imm = tVal;

  // non-split send implies Src1.Length == 0
  xlen = 0;

  return exDescIga;
}

// A helper function to print send src/dst length information to a file.
// vISA_ShaderDataBaseStats key a requirement to use
void BinaryEncodingIGA::printSendDataToFile(uint32_t src0Len,
                                            uint32_t src1Len,
                                            uint32_t dstLen,
                                            const char *filePath) const {
  FILE *f = fopen(filePath, "a");
  if (f) {
    uint32_t namePos = fileName.find_last_of('\\', fileName.size());
    if (namePos > 0)
      namePos++;
    fprintf(f, "file=%s src0Len=%d src1Len=%d dstLen=%d \n",
            fileName.substr(namePos, fileName.size()).data(), src0Len, src1Len,
            dstLen); // Do not remove a white space at the end!
    fclose(f);
  }
}

////////////////////////////////////////////////////////////////////
// these handle binary sends (old "sends" and Xe+ "send"
//
SendDesc BinaryEncodingIGA::encodeExDescImm(G4_INST *sendInst,
                                            SendExDescOpts &sdos) const {
  SendDesc exDescIga;

  const G4_Operand *exDescG4 = sendInst->getSrc(3);
  const G4_SendDescRaw *descG4 = (G4_SendDescRaw *)sendInst->getMsgDesc();
  vISA_ASSERT(descG4 != nullptr, "expected raw descriptor");
  if (sendInst->getBuilder().getOption(vISA_ShaderDataBaseStats)) {
    auto JitInfo = kernel.fg.builder->getJitInfo();
    JitInfo->sendInfo.src0Vec.push_back(descG4->getSrc0LenRegs());
    JitInfo->sendInfo.src1Vec.push_back(descG4->getSrc1LenRegs());
    JitInfo->sendInfo.destVec.push_back(descG4->getDstLenRegs());
    printSendDataToFile(descG4->getSrc0LenRegs(),
                        descG4->getSrc1LenRegs(),
                        descG4->getDstLenRegs(),
                        sendInst->getBuilder().getOptions()->getOptionCstr(
                            vISA_ShaderDataBaseStatsFilePath));
  }

  sdos.xlen = (int)descG4->extMessageLength();

  exDescIga.type = SendDesc::Kind::IMM;
  exDescIga.imm = (uint32_t)exDescG4->asImm()->getImm();
  // We must clear the funcID in the extended message for Xe+
  // because it is part of the EU instruction, not the descriptor,
  // and, vISA/G4-IR still thinks of it as part of the descriptor.
  //
  // Ditto for the EOT bit which is moved out of extDesc
  //
  // The extended message format
  // struct ExtendedMsgDescLayout {
  //    uint32_t funcID : 4;       // bit 0:3 << not part of ExDesc
  //    uint32_t unnamed1 : 1;     // bit 4
  //    uint32_t eot : 1;          // bit 5 << not part of ExDesc
  //    uint32_t extMsgLength : 5; // bit 6:10
  //    uint32_t unnamed2 : 5;     // bit 11:15
  //    uint32_t extFuncCtrl : 16; // bit 16:31
  // };
  auto platformGen = sendInst->getBuilder().getPlatformGeneration();
  if (platformGen >= PlatformGen::XE) {
    exDescIga.imm &= 0xFFFFFFC0;
  }
  if (kernel.fg.builder->encodeSendSrc1Length()) {
    // In DG2+ ExDesc[10:6] is no longer Src1.Length even for imm
    // and the field is always part of the EU encoding
    if (descG4->isCPSEnabled()) {
      sdos.extraOpts.add(InstOpt::CPS);
    }
    // We must keep ExDesc[10:6] as Src1.Len until we fix the
    // 100s of uses that assume this.
    // IGA expects these bits to be 0's on this platform
    //
    // FIXME: remove the if guard once we enable DG2
    // (i.e. once DG2 targets IGA DG2 and not XE_HP, we should clear
    // the bottom 12b for DG2 too) and only use the IR fields above
    // (exDescArg.{cps,src1Length})
    exDescIga.imm &= 0xFFFFF000;
  }
  // Note: ExBSO is never permitted for imm descriptors

  // clear the EOT bit which is not part of exDesc on XE+
  if (platformGen >= PlatformGen::XE)
    exDescIga.imm &= ~(1 << 5);

  return exDescIga;
}

SendDesc BinaryEncodingIGA::encodeExDescRegA0(G4_INST *sendInst,
                                              SendExDescOpts &sdos) const {
  G4_Operand *exDescG4 = sendInst->getSrc(3);
  const G4_SendDescRaw *descG4 = sendInst->getMsgDescRaw();
  vISA_ASSERT(descG4 != nullptr, "expected raw descriptor");
  if (sendInst->getBuilder().getOption(vISA_ShaderDataBaseStats)) {
    auto JitInfo = kernel.fg.builder->getJitInfo();
    JitInfo->sendInfo.src0Vec.push_back(descG4->getSrc0LenRegs());
    JitInfo->sendInfo.src1Vec.push_back(descG4->getSrc1LenRegs());
    JitInfo->sendInfo.destVec.push_back(descG4->getDstLenRegs());
    printSendDataToFile(descG4->getSrc0LenRegs(),
                        descG4->getSrc1LenRegs(),
                        descG4->getDstLenRegs(),
                        sendInst->getBuilder().getOptions()->getOptionCstr(
                            vISA_ShaderDataBaseStatsFilePath));
  }
  SendDesc exDescIga;
  exDescIga.type = SendDesc::Kind::REG32A;
  exDescIga.reg.regNum = 0; // must be a0
  bool valid = false;
  exDescIga.reg.subRegNum =
      (uint16_t)exDescG4->asSrcRegRegion()->ExSubRegNum(valid);
  vISA_ASSERT(valid, "invalid subreg");

  if (kernel.fg.builder->useNewExtDescFormat() && descG4->isCPSEnabled()) {
    // CPS is an instruction option if using RegDesc+ExBSO
    sdos.extraOpts.add(InstOpt::CPS);
  }

  // By default all RegDesc in the new descriptor format will use
  // the ExBSO model if at all possible
  bool encodeExBso = kernel.fg.builder->useNewExtDescFormat();
  // On DG2 LSC mustn't use ExBSO for BTI (hardware restriction).
  // If this is an LSC descriptor, then dig out the LscAddrType field and
  // compare to 3 (which means BTI).
  const bool isLsc = descG4->isLscOp();
  const bool isLscBti = isLsc && descG4->getLscAddrType() == LSC_ADDR_TYPE_BTI;
  const bool isUgm = sendInst->getMsgDesc()->getSFID() == vISA::SFID::UGM;
  encodeExBso &= !isLscBti;
  encodeExBso &= (platform < Xe2 || !isUgm);
  if (encodeExBso)
    sdos.extraOpts.add(InstOpt::EXBSO);
  if (isLsc && isUgm && !encodeExBso) {
    bool isLscBssOrSs = descG4->getLscAddrType() == LSC_ADDR_TYPE_BSS ||
                        descG4->getLscAddrType() == LSC_ADDR_TYPE_SS;
    if (isLscBssOrSs && descG4->getBti() != nullptr) {
      // BSS/SS with a0 reg (ExDesc.IsReg) with UGM
      sdos.exDescImmOff = descG4->getExDescImmOff();
    }
  }

  // G4 IR keeps Src1.Length (xlen) separate.  So it's known,
  // (even with a reg desc in nonExBSO mode)
  sdos.xlen = (int)descG4->extMessageLength();

  return exDescIga;
}

SendExDescOpts
  BinaryEncodingIGA::getIGASendExDesc(G4_InstSend *sendInst) const {
  SendExDescOpts sdos;

  vISA_ASSERT(sendInst->isSend(), "expect send inst");

  if (sendInst->isEOT())
    sdos.extraOpts.add(InstOpt::EOT);

  if (sendInst->isSplitSend()) {
    const G4_Operand *exDesc = sendInst->getMsgExtDescOperand();
    sdos.exDesc = exDesc->isImm() ? encodeExDescImm(sendInst, sdos)
                                  : encodeExDescRegA0(sendInst, sdos);
  } else {
    sdos.exDesc = encodeExDescSendUnary(sendInst, sdos.xlen, sdos.extraOpts);
  }

  return sdos;
}

void *BinaryEncodingIGA::EmitBinary(size_t &binarySize) {
  binarySize = m_kernelBufferSize;

  if (kernel.getOption(vISA_GenerateBinary)) {
    std::string binFileName = fileName + ".dat";
    if (CisaFramework::allowDump(*kernel.getOptions(), binFileName)) {
      std::ofstream os(binFileName, std::ios::binary);
      if (!os) {
        std::string errStr;
        errStr = "BinaryEncodingIGA: unable to open output path for write: " +
                 binFileName + "\n";
        // vISA_ASSERT(false, errStr);
        return m_kernelBuffer;
      }
      os.write((const char *)m_kernelBuffer, binarySize);
    }
  }

  return m_kernelBuffer;
}

ExecSize BinaryEncodingIGA::getIGAExecSize(int execSize) {
  switch (execSize) {
  case 1:
    return ExecSize::SIMD1;
  case 2:
    return ExecSize::SIMD2;
  case 4:
    return ExecSize::SIMD4;
  case 8:
    return ExecSize::SIMD8;
  case 16:
    return ExecSize::SIMD16;
  case 32:
    return ExecSize::SIMD32;
  default:
    vISA_ASSERT_UNREACHABLE("illegal simd size");
    return ExecSize::INVALID;
  }
}

ChannelOffset BinaryEncodingIGA::getIGAChannelOffset(int offset) {
  switch (offset) {
  case 0:
    return ChannelOffset::M0;
  case 4:
    return ChannelOffset::M4;
  case 8:
    return ChannelOffset::M8;
  case 12:
    return ChannelOffset::M12;
  case 16:
    return ChannelOffset::M16;
  case 20:
    return ChannelOffset::M20;
  case 24:
    return ChannelOffset::M24;
  case 28:
    return ChannelOffset::M28;
  default:
    vISA_ASSERT_UNREACHABLE("illegal mask offset");
    return ChannelOffset::M0;
  }
}

MaskCtrl BinaryEncodingIGA::getIGAMaskCtrl(bool noMask) {
  return noMask ? MaskCtrl::NOMASK : MaskCtrl::NORMAL;
}

RegName BinaryEncodingIGA::getIGAARFName(G4_ArchRegKind areg) {
  switch (areg) {
  case AREG_NULL:
    return RegName::ARF_NULL;
  case AREG_A0:
    return RegName::ARF_A;
  case AREG_ACC0:
  case AREG_ACC1:
    return RegName::ARF_ACC;
  case AREG_MASK0:
    return RegName::ARF_CE;
  case AREG_MSG0:
    return RegName::ARF_MSG;
  case AREG_DBG:
    return RegName::ARF_DBG;
  case AREG_SR0:
    return RegName::ARF_SR;
  case AREG_CR0:
    return RegName::ARF_CR;
  case AREG_N0:
  case AREG_N1:
    return RegName::ARF_N;
  case AREG_IP:
    return RegName::ARF_IP;
  case AREG_F0:
  case AREG_F1:
  case AREG_F2:
  case AREG_F3:
    return RegName::ARF_F;
  case AREG_TM0:
    return RegName::ARF_TM;
  case AREG_TDR0:
    return RegName::ARF_TDR;
  case AREG_SP:
    return RegName::ARF_SP;
  case AREG_S0:
    return RegName::ARF_S;
  default:
    vISA_ASSERT_UNREACHABLE("illegal ARF");
    return RegName::INVALID;
  }
}

Type BinaryEncodingIGA::getIGAType(const G4_INST *I, Gen4_Operand_Number O,
                                   TARGET_PLATFORM P) {

  G4_Type Ty = I->getOperand(O)->getType();
  if (I->opcode() == G4_fcvt) {
    if (Ty == Type_UB) {
      return Type::BF8;
    }
    if (Ty == Type_B) {
      return Type::HF8;
    }
    if (Ty == Type_UD) {
      return Type::TF32;
    }
  }
  if (I->opcode() == G4_srnd) {
    if (O == Opnd_dst && Ty == Type_UB) {
      return Type::BF8;
    }
  }
  return getIGAType(Ty, P);
}

Type BinaryEncodingIGA::getIGAType(G4_Type type, TARGET_PLATFORM genxPlatform) {
  switch (type) {
  case Type_UB:
    return Type::UB;
  case Type_B:
    return Type::B;
  case Type_UW:
    return Type::UW;
  case Type_W:
    return Type::W;
  case Type_UD:
    return Type::UD;
  case Type_D:
    return Type::D;
  case Type_UQ:
    return Type::UQ;
  case Type_Q:
    return Type::Q;
  case Type_HF:
    return Type::HF;
  case Type_F:
    return Type::F;
  case Type_DF:
    return Type::DF;
  case Type_UV:
    return Type::UV;
  case Type_V:
    return Type::V;
  case Type_VF:
    return Type::VF;
  case Type_NF:
    return Type::NF;
  case Type_BF:
    return Type::BF;
  default:
    vISA_ASSERT_UNREACHABLE("illegal type");
    return Type::INVALID;
  }
}

PredCtrl BinaryEncodingIGA::getIGAPredCtrl(G4_Predicate_Control g4PrCtl) {
  switch (g4PrCtl) {
  case PRED_DEFAULT:
    return PredCtrl::SEQ;
  case PRED_ANY2H:
    return PredCtrl::ANY2H;
  case PRED_ANY4H:
    return PredCtrl::ANY4H;
  case PRED_ANY8H:
    return PredCtrl::ANY8H;
  case PRED_ANY16H:
    return PredCtrl::ANY16H;
  case PRED_ANY32H:
    return PredCtrl::ANY32H;
  case PRED_ALL2H:
    return PredCtrl::ALL2H;
  case PRED_ALL4H:
    return PredCtrl::ALL4H;
  case PRED_ALL8H:
    return PredCtrl::ALL8H;
  case PRED_ALL16H:
    return PredCtrl::ALL16H;
  case PRED_ALL32H:
    return PredCtrl::ALL32H;
  case PRED_ANYV:
    return PredCtrl::ANYV;
  case PRED_ALLV:
    return PredCtrl::ALLV;
  case PRED_ANY_WHOLE:
    return PredCtrl::ANY;
  case PRED_ALL_WHOLE:
    return PredCtrl::ALL;
  default:
    vISA_ASSERT_UNREACHABLE("illegal predicate control");
    return PredCtrl::NONE;
  }
}

Predication BinaryEncodingIGA::getIGAPredication(const G4_Predicate *predG4,
                                                 G4_ExecSize execSize,
                                                 const IR_Builder &builder) {
  // Xe_PVC+ has removed predCtrl width. Only .any, .all and .seq (default) are
  // supported. Try to translate PredCtrl values to compatible ones on Xe_PVC+
  // Return true on success, false on fail
  [[maybe_unused]]
  auto translatePredCtrl = [&](G4_Predicate_Control &predCtrl) {
    if (builder.predCtrlHasWidth())
      return true;
    if (predCtrl == PRED_ANY_WHOLE || predCtrl == PRED_ALL_WHOLE ||
        predCtrl == PRED_DEFAULT)
      return true;
    // .any*h/.all*h (pre-pvc) is equal to .any/.all (pvc+) only when the
    // instruction execSize match the predCtrl group size
    if (execSize != predG4->getPredCtrlGroupSize())
      return false;

    // translate to equivalent .any or .all whenever possible
    switch (predCtrl) {
    case PRED_ANY2H:
    case PRED_ANY4H:
    case PRED_ANY8H:
    case PRED_ANY16H:
    case PRED_ANY32H:
      predCtrl = PRED_ANY_WHOLE;
      break;
    case PRED_ALL2H:
    case PRED_ALL4H:
    case PRED_ALL8H:
    case PRED_ALL16H:
    case PRED_ALL32H:
      predCtrl = PRED_ALL_WHOLE;
      break;
    default:
      // PRED_ANY_WHOLE, PRED_ALL_WHOLE, PRED_DEFAULT are handled above
      // PRED_ANYV, PRED_ALLV are not supported
      return false;
    }
    return true;
  };

  Predication pred;
  if (predG4) {
    G4_Predicate_Control g4PrCtl = predG4->getControl();
    vISA_ASSERT(translatePredCtrl(g4PrCtl), "illegal predicate control");

    pred.function = getIGAPredCtrl(g4PrCtl);
    pred.inverse = predG4->getState() != PredState_Plus;
  }
  return pred;
}

SrcModifier BinaryEncodingIGA::getIGASrcModifier(G4_SrcModifier srcMod) {
  switch (srcMod) {
  case Mod_Minus:
    return SrcModifier::NEG;
  case Mod_Abs:
    return SrcModifier::ABS;
  case Mod_Minus_Abs:
    return SrcModifier::NEG_ABS;
  case Mod_Not:
    return SrcModifier::NEG;
  case Mod_src_undef:
    return SrcModifier::NONE;
  default:
    vISA_ASSERT_UNREACHABLE("illegal source modifier");
    return SrcModifier::NONE;
  }
}

Region::Vert BinaryEncodingIGA::getIGAVert(int vstride) {
  switch (vstride) {
  case 0:
    return Region::Vert::VT_0;
  case 1:
    return Region::Vert::VT_1;
  case 2:
    return Region::Vert::VT_2;
  case 4:
    return Region::Vert::VT_4;
  case 8:
    return Region::Vert::VT_8;
  case 16:
    return Region::Vert::VT_16;
  case 32:
    return Region::Vert::VT_32;
  case UNDEFINED_SHORT:
    return Region::Vert::VT_VxH;
  default:
    vISA_ASSERT_UNREACHABLE("illegal vstride");
    return Region::Vert::VT_INVALID;
  }
}

Region::Width BinaryEncodingIGA::getIGAWidth(int width) {
  switch (width) {
  case 1:
    return Region::Width::WI_1;
  case 2:
    return Region::Width::WI_2;
  case 4:
    return Region::Width::WI_4;
  case 8:
    return Region::Width::WI_8;
  case 16:
    return Region::Width::WI_16;
  default:
    vISA_ASSERT_UNREACHABLE("illegal width");
    return Region::Width::WI_INVALID;
  }
}

Region::Horz BinaryEncodingIGA::getIGAHorz(int hstride) {
  switch (hstride) {
  case 0:
    return Region::Horz::HZ_0;
  case 1:
    return Region::Horz::HZ_1;
  case 2:
    return Region::Horz::HZ_2;
  case 4:
    return Region::Horz::HZ_4;
  default:
    vISA_ASSERT_UNREACHABLE("illegal hstride");
    return Region::Horz::HZ_INVALID;
  }
}

Region BinaryEncodingIGA::getIGARegion(G4_SrcRegRegion *srcRegion, int srcPos) {
  Region igaRegion;
  const RegionDesc *region = srcRegion->getRegion();
  if ((srcRegion->getInst()->getNumSrc() == 3 &&
       !srcRegion->getInst()->isSend())) {
    // special handling for 3src instructions
    if (srcPos != 2) {
      // for src0 and src1, IGA/GED does not like width to be set
      igaRegion.set(getIGAVert(region->vertStride), Region::Width::WI_INVALID,
                    getIGAHorz(region->horzStride));
    } else {
      // for src2, IGA expects both VS and W to be invalid
      igaRegion.set(Region::Vert::VT_INVALID, Region::Width::WI_INVALID,
                    getIGAHorz(region->horzStride));
    }
  } else {
    igaRegion.set(getIGAVert(region->vertStride), getIGAWidth(region->width),
                  getIGAHorz(region->horzStride));
  }
  return igaRegion;
}

EncodeResult vISA::EncodeKernelIGA(vISA::G4_Kernel &k,
                                   const std::string &fname) {
  EncodeResult r;
  BinaryEncodingIGA encoder(k, fname);
  encoder.Encode();
  r.binary = encoder.EmitBinary(r.binaryLen);
  return r;
}

static const Model *GetModel(TARGET_PLATFORM p) {
  const Model *m =
      Model::LookupModel(BinaryEncodingIGA::getIGAInternalPlatform(p));
  return m;
}

bool vISA::InstSupportsSaturationIGA(TARGET_PLATFORM p, const G4_INST &i,
                                     const IR_Builder &builder) {
  const Model *m = GetModel(p);
  if (m) {
    auto oi = BinaryEncodingIGA::getIgaOpInfo(&i, m, true, builder);
    return oi.first && oi.first->isValid() && oi.first->supportsSaturation();
  } else {
    return false;
  }
}

bool vISA::InstSupportsSrcModifierIGA(TARGET_PLATFORM p, const G4_INST &i,
                                      const IR_Builder &builder) {
  const Model *m = GetModel(p);
  if (m) {
    auto oi = BinaryEncodingIGA::getIgaOpInfo(&i, m, true, builder);
    return oi.first && oi.first->isValid() &&
           oi.first->supportsSourceModifiers();
  } else {
    return false;
  }
}
