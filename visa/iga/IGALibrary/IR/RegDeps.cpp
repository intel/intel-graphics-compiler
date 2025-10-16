/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "RegDeps.hpp"
#include "../asserts.hpp"
#include "../bits.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <sstream>

using namespace iga;

static DEP_CLASS getClassFromPipeType(DEP_PIPE type, const OpSpec &opspec) {
  if (opspec.is(Op::SYNC) || opspec.op == Op::ILLEGAL)
    return DEP_CLASS::OTHER;

  switch (type) {
  case DEP_PIPE::NONE:
  case DEP_PIPE::SHORT:
  case DEP_PIPE::LONG:
  case DEP_PIPE::CONTROL_FLOW:
    return DEP_CLASS::IN_ORDER;

  case DEP_PIPE::SEND:
  case DEP_PIPE::MATH:
    return DEP_CLASS::OUT_OF_ORDER;

  case DEP_PIPE::FLOAT:
  case DEP_PIPE::INTEGER:
  case DEP_PIPE::LONG64:
  case DEP_PIPE::MATH_INORDER:
  case DEP_PIPE::SCALAR:
    return DEP_CLASS::IN_ORDER;

  case DEP_PIPE::DPAS:
  case DEP_PIPE::SEND_SLM:
  case DEP_PIPE::SEND_UNKNOWN:
    return DEP_CLASS::OUT_OF_ORDER;
  }
  return DEP_CLASS::NONE;
}

static void setDEPPipeClass_SingleDistPipe(DepSet &dep,
                                           const Instruction &inst) {
  auto opsec = inst.getOpSpec();
  dep.setDepPipe(DEP_PIPE::SHORT);
  if (opsec.is(Op::MATH)) {
    dep.setDepPipe(DEP_PIPE::MATH);
  } else if (opsec.isAnySendFormat()) {
    dep.setDepPipe(DEP_PIPE::SEND);
  } else if (opsec.isBranching()) {
    dep.setDepPipe(DEP_PIPE::CONTROL_FLOW);
  } else {
    for (uint32_t i = 0; i < inst.getSourceCount(); ++i) {
      const auto &src = inst.getSource(i);
      if (src.getType() == Type::DF || src.getType() == Type::Q ||
          src.getType() == Type::UQ) {
        dep.setDepPipe(DEP_PIPE::LONG);
        break;
      }
    }
    if (opsec.supportsDestination()) {
      const auto &dst = inst.getDestination();
      if (dst.getType() == Type::DF || dst.getType() == Type::Q ||
          dst.getType() == Type::UQ) {
        dep.setDepPipe(DEP_PIPE::LONG);
      }
    }
  }

  dep.setDepClass(getClassFromPipeType(dep.getDepPipe(), opsec));
}

// XeHP+
static void setSendPipeType(DEP_PIPE &pipe_type, const Instruction &inst,
                            const Model &model) {
  assert(inst.getOpSpec().isAnySendFormat());
  pipe_type = DEP_PIPE::SEND;
  // XeHPG+: slm send should be considered in different pipes
  // Check if it's SLM

  if (model.platform >= Platform::XE_HPG) {
    if (inst.getMsgDescriptor().isReg())
      pipe_type = DEP_PIPE::SEND_UNKNOWN;
    else {
      SFID sfid = inst.getSendFc();
      if (sfid == SFID::SLM) {
        pipe_type = DEP_PIPE::SEND_SLM;
      } else {
        uint32_t desc = inst.getMsgDescriptor().imm;
        bool btiSlm = (0xFE == getBits<uint32_t>(desc, 0, 8));
        bool scratchBlockMsg = (1 == getBits<uint32_t>(desc, 18, 1));
        bool headerPresent = (1 == getBits<uint32_t>(desc, 19, 1));
        bool sidebandOffsetEn = (1 == getBits<uint32_t>(desc, 7, 1));
        if ((sfid == SFID::DC0 && btiSlm && !scratchBlockMsg) ||
            (sfid == SFID::DC1 && btiSlm) ||
            (sfid == SFID::DC2 && !headerPresent && sidebandOffsetEn))
          pipe_type = DEP_PIPE::SEND_SLM;
      }
    }
  }
}

// MTL
static void setDEPPipeClass_ThreeDistPipeDPMath(DepSet &dep,
                                                const Instruction &inst,
                                                const Model &model) {
  // The same as ThreeDistPipe, only that instructions with DP (fp64) types
  // will be in Math pipe
  auto opsec = inst.getOpSpec();

  DEP_PIPE pipe_type = DEP_PIPE::NONE;
  if (opsec.is(Op::MATH)) {
    pipe_type = DEP_PIPE::MATH;
  } else if (opsec.isAnySendFormat()) {
    setSendPipeType(pipe_type, inst, model);
  } else if (opsec.isDpasFormat()) {
    pipe_type = DEP_PIPE::DPAS;
  } else if (opsec.isBranching()) {
    pipe_type = DEP_PIPE::INTEGER;
  } else {
    // In order instruction:
    // if destination type is FP32/FP16/BF16 then it goes to float pipe,
    // if destination type is int32 / 16 / 8 it goes to integer pipe and
    // if destination or source type is int64 then it goes to long pipe
    // if destination or source type is FP64 then it goes to out-of-order Math
    // pipe for conversion instructions float2int goes to integer pipe and
    // int2float goes to float pipe, If destination type is null then source0
    // data type will determine the pipe
    Type inst_type = Type::INVALID;
    if (opsec.supportsDestination())
      inst_type = inst.getDestination().getType();
    else if (inst.getSourceCount())
      inst_type = inst.getSource(0).getType();

    if (inst_type != Type::INVALID) {
      if (TypeIs64b(inst_type)) {
        if (TypeIsFloating(inst_type))
          pipe_type = DEP_PIPE::MATH;
        else
          pipe_type = DEP_PIPE::LONG64;
      } else if (TypeIsFloating(inst_type))
        pipe_type = DEP_PIPE::FLOAT;
      else
        pipe_type = DEP_PIPE::INTEGER;
    }

    // any of src has 64-bit type --> math or long pipe
    for (uint32_t i = 0; i < inst.getSourceCount(); ++i) {
      const auto &src = inst.getSource(i);
      if (TypeIs64b(src.getType())) {
        if (TypeIsFloating(src.getType()))
          pipe_type = DEP_PIPE::MATH;
        else
          pipe_type = DEP_PIPE::LONG64;
        break;
      }
    }
  }

  // default set to Integer pipe (e.g. NOP)
  if (pipe_type == DEP_PIPE::NONE)
    pipe_type = DEP_PIPE::INTEGER;

  dep.setDepClass(getClassFromPipeType(pipe_type, opsec));
  dep.setDepPipe(pipe_type);
}

// XeHP
static void setDEPPipeClass_ThreeDistPipe(DepSet &dep, const Instruction &inst,
                                          const Model &model) {
  auto opsec = inst.getOpSpec();

  DEP_PIPE pipe_type = DEP_PIPE::NONE;
  if (opsec.is(Op::MATH)) {
    pipe_type = DEP_PIPE::MATH;
  } else if (opsec.isAnySendFormat()) {
    setSendPipeType(pipe_type, inst, model);
  } else if (opsec.isDpasFormat()) {
    pipe_type = DEP_PIPE::DPAS;
  } else if (opsec.isBranching()) {
    pipe_type = DEP_PIPE::INTEGER;
  } else {
    // In order instruction:
    // if destination type is FP32/FP16/BF16 then it goes to float pipe,
    // if destination type is int32 / 16 / 8 it goes to integer pipe and
    // if destination or source type is int64/FP64 then it goes to long pipe

    // for conversion instructions float2int goes to integer pipe and int2float
    // goes to float pipe, anything2doublefloat goes to long pipe and
    // doublefloat2anything goes to long pipe

    // If destination type is null then source0 data type will determine the
    // pipe
    Type inst_type = Type::INVALID;
    if (opsec.supportsDestination())
      inst_type = inst.getDestination().getType();
    else if (inst.getSourceCount())
      inst_type = inst.getSource(0).getType();

    if (inst_type != Type::INVALID) {
      if (TypeIs64b(inst_type))
        pipe_type = DEP_PIPE::LONG64;
      else if (TypeIsFloating(inst_type))
        pipe_type = DEP_PIPE::FLOAT;
      else
        pipe_type = DEP_PIPE::INTEGER;
    }

    for (uint32_t i = 0; i < inst.getSourceCount(); ++i) {
      const auto &src = inst.getSource(i);
      if (TypeIs64b(src.getType())) {
        pipe_type = DEP_PIPE::LONG64;
        break;
      }
    }
  }

  // default set to Integer pipe (e.g. NOP)
  if (pipe_type == DEP_PIPE::NONE)
    pipe_type = DEP_PIPE::INTEGER;

  dep.setDepClass(getClassFromPipeType(pipe_type, opsec));
  dep.setDepPipe(pipe_type);
}

// XeHPC+
static void setDEPPipeClass_FourDistPipe(DepSet &dep, const Instruction &inst,
                                         const Model &model) {
  setDEPPipeClass_ThreeDistPipe(dep, inst, model);
  if (dep.getDepPipe() == DEP_PIPE::MATH) {
    dep.setDepClass(DEP_CLASS::IN_ORDER);
    dep.setDepPipe(DEP_PIPE::MATH_INORDER);
  }
}

static void setDEPPipeClass_FourDistPipeReduction(DepSet &dep,
                                                  const Instruction &inst,
                                                  const Model &model) {
  // The difference between FourDistPipe and FourDistPipeReduction
  // is that for FourDistPipeReduction: only fp64-dst-instructions goto Long
  // pipe (LONG64), and for FourDistPipe: instructions having 64b types on src
  // or dst goto Long pipe.
  auto opsec = inst.getOpSpec();

  DEP_PIPE pipe_type = DEP_PIPE::NONE;
  if (opsec.is(Op::MATH)) {
    dep.setDepClass(DEP_CLASS::IN_ORDER);
    dep.setDepPipe(DEP_PIPE::MATH_INORDER);
    return;
  } else if (opsec.isAnySendFormat()) {
    setSendPipeType(pipe_type, inst, model);
  } else if (opsec.isDpasFormat()) {
    pipe_type = DEP_PIPE::DPAS;
  } else if (opsec.isBranching()) {
    pipe_type = DEP_PIPE::INTEGER;
  } else {
    // In order instruction:
    // RegDistFloat: Specify distance dependancy in float32 pipe
    //               (i.e. all instructions with float32/16/8 dst)
    // RegDistLong:  Specify distance dependancy in float64 pipe
    //               (i.e. all instructions with float64 dst)
    // RegDistInt:   Specify distance dependancy in Int32/16/8 and Int64 pipe
    //               (i.e. all instructions with integer dst)
    // RegDistMath:  Specify distance dependancy in Math pipe
    //               (i.e. all math instructions)

    Type inst_type = Type::INVALID;
    // only instructions with f64 dst go into Long, others with f32/16/8
    // go into FLOAT
    if (opsec.supportsDestination()) {
      inst_type = inst.getDestination().getType();
      if (TypeIsFloating(inst_type)) {
        if (TypeIs64b(inst_type))
          pipe_type = DEP_PIPE::LONG64;
        else
          pipe_type = DEP_PIPE::FLOAT;
      } else {
        pipe_type = DEP_PIPE::INTEGER;
      }
    }
  }

  // default set to Integer pipe (e.g. NOP)
  if (pipe_type == DEP_PIPE::NONE)
    pipe_type = DEP_PIPE::INTEGER;

  dep.setDepClass(getClassFromPipeType(pipe_type, opsec));
  dep.setDepPipe(pipe_type);
}

static bool isScalarPipeInst(const Instruction &inst) {
  return inst.getOp() == Op::MOV &&
         inst.getDestination().getDirRegName() == RegName::ARF_S &&
         inst.getSource(0).isImm();
}

static void setDEPPipeClass_FiveDistPipe(DepSet &dep, const Instruction &inst,
                                         const Model &model) {
  // TODO: Implement FiveDistPipe
  setDEPPipeClass_FourDistPipeReduction(dep, inst, model);

  // mov instructions with scalar dst and imm src goto SCALAR pipe
  if (isScalarPipeInst(inst)) {
    dep.setDepPipe(DEP_PIPE::SCALAR);
    IGA_ASSERT(dep.getDepClass() == DEP_CLASS::IN_ORDER,
               "dep class for mov should be set already");
  }
}

static void setDEPPipeClass_FiveDistPipeReduction(DepSet &dep,
                                                  const Instruction &inst,
                                                  const Model &model) {
  // The only difference between FiveDistPipeReduction and FiveDistPipe
  // is that Q to F coversion instructions are now in INT pipe
  setDEPPipeClass_FiveDistPipe(dep, inst, model);

  if (dep.getDepClass() == DEP_CLASS::IN_ORDER) {
    auto opsec = inst.getOpSpec();
    if (opsec.supportsDestination() && inst.getSourceCount() > 1) {
      Type dstType = inst.getDestination().getType();
      Type src0Type = inst.getSource(0).getType();
      if (dstType == Type::F && (src0Type == Type::Q || src0Type == Type::UQ)) {
        dep.setDepPipe(DEP_PIPE::INTEGER);
      }
    }
  }
}


static void setDEPPipeClass(SWSB_ENCODE_MODE enc_mode, DepSet &dep,
                            const Instruction &inst, const Model &model) {
  switch (enc_mode) {
  case SWSB_ENCODE_MODE::SingleDistPipe:
    setDEPPipeClass_SingleDistPipe(dep, inst);
    break;
  case SWSB_ENCODE_MODE::ThreeDistPipeDPMath:
    setDEPPipeClass_ThreeDistPipeDPMath(dep, inst, model);
    break;
  case SWSB_ENCODE_MODE::ThreeDistPipe:
    setDEPPipeClass_ThreeDistPipe(dep, inst, model);
    break;
  case SWSB_ENCODE_MODE::FourDistPipe:
    setDEPPipeClass_FourDistPipe(dep, inst, model);
    break;
  case SWSB_ENCODE_MODE::FourDistPipeReduction:
    setDEPPipeClass_FourDistPipeReduction(dep, inst, model);
    break;
  case SWSB_ENCODE_MODE::FiveDistPipe:
    setDEPPipeClass_FiveDistPipe(dep, inst, model);
    break;
  case SWSB_ENCODE_MODE::FiveDistPipeReduction:
    setDEPPipeClass_FiveDistPipeReduction(dep, inst, model);
    break;
  default:
    break;
  }
}

DepSet::DepSet(const InstIDs &instIdCntr, const DepSetBuilder &dsb)
    : m_InstIDs(instIdCntr.global, instIdCntr.inOrder, instIdCntr.floatPipe,
                instIdCntr.intPipe, instIdCntr.longPipe, instIdCntr.mathPipe,
                instIdCntr.scalarPipe), m_DB(dsb) {
  m_bucketList.reserve(4);
  bits = new BitSet<>(dsb.getTOTAL_BITS());
}

uint32_t DepSet::getDPASOpsPerChan(Type src1_ty, Type src2_ty, bool isDF) {
  // get OPS_PER_CHAN, the number of dot product operations per dword channel,
  // depending on element type
  if (isDF)
    return 1;

  if (src1_ty == Type::HF || src1_ty == Type::BF) {
    IGA_ASSERT(src1_ty == src2_ty, "dpas: invalid src1/src2 types combination");
    return 2;
  } else if (src1_ty == Type::BF8 || src1_ty == Type::HF8) {
    IGA_ASSERT(src2_ty == Type::BF8 || src2_ty == Type::HF8,
               "dpas: invalid src1/src2 types combination");
    return 4;
  } else if (src1_ty == Type::TF32) {
    IGA_ASSERT(src1_ty == src2_ty, "dpas: invalid src1/src2 types combination");
    return 1;
  } else {
    // if both src1 and src2 are int2 or int4, than ops_per_chan will be 8
    int src1_size = TypeSizeInBits(src1_ty);
    int src2_size = TypeSizeInBits(src2_ty);
    // Type: ub, b, u4, s4, u2, s2
    IGA_ASSERT((src1_size <= 8), "OPS_PER_CHAN: unsupported src1 type");
    IGA_ASSERT((src2_size <= 8), "OPS_PER_CHAN: unsupported src2 type");
    if ((src1_size == 2 || src1_size == 4) &&
        (src2_size == 2 || src2_size == 4))
      return 8;
    return 4;
  }
}

// lowBound - start register address offset in byte
// UpBound - upper register address offset in byte
uint32_t DepSet::getDPASSrcDepUpBound(unsigned idx, Type srcType,
                                      uint32_t execSize, uint32_t lowBound,
                                      uint32_t systolicDepth,
                                      uint32_t repeatCount,
                                      uint32_t opsPerChan) {
  auto typeSizeInBits = TypeSizeInBitsWithDefault(srcType, 32);
  // elements_size is the size of total elements to be calculated in one
  // operation
  uint32_t elements_size = execSize * typeSizeInBits / 8;

  uint32_t upBound = lowBound;
  if (idx == 0)
    upBound += elements_size * repeatCount;
  else if (idx == 1)
    upBound += elements_size * opsPerChan * systolicDepth;
  else if (idx == 2)
    upBound += (repeatCount - 1) * opsPerChan * 8 * typeSizeInBits /
                   8 + /* start offset of the last repeated block */
               opsPerChan * systolicDepth * typeSizeInBits /
                   8; /* size of used register in last repeated block */
  return upBound;
}

void DepSet::getDpasSrcDependency(const Instruction &inst,
                                  RegRangeListType &reg_range,
                                  RegRangeListType &extra_regs,
                                  const Model &model) {
  uint32_t execSize = static_cast<uint32_t>(inst.getExecSize());

  IGA_ASSERT((!inst.isDF() && execSize == (m_DB.getGRF_BYTES_PER_REG() / 4)) ||
                 (inst.isDF() && execSize == 8),
             "Invalid ExecSize for this op");

  // check src operand and add the dependency
  uint32_t repeatCount = GetDpasRepeatCount(inst.getDpasFc());
  uint32_t systolicDepth = GetDpasSystolicDepth(inst.getDpasFc());
  uint32_t ops_per_chan = getDPASOpsPerChan(
      inst.getSource(1).getType(), inst.getSource(2).getType(), inst.isDF());

  for (unsigned srcIx = 0; srcIx < inst.getSourceCount(); ++srcIx) {
    const Operand &op = inst.getSource(srcIx);
    // the src0 could be null, in that case no need to set the dependency
    if (op.getDirRegName() == RegName::ARF_NULL) {
      // if src is null, set the reg range to max() to specify its actually
      // empty
      reg_range.push_back(std::make_pair(std::numeric_limits<uint32_t>::max(),
                                         std::numeric_limits<uint32_t>::max()));
      continue;
    }
    IGA_ASSERT(op.getDirRegName() == RegName::GRF_R,
               "GRF or null required on this op");

    // calculate register region
    auto tType = op.getType();
    auto typeSizeInBits = TypeSizeInBitsWithDefault(tType, 32);
    uint32_t lowBound =
        addressOf(op.getDirRegName(), op.getDirRegRef(), typeSizeInBits);
    uint32_t upBound =
        getDPASSrcDepUpBound(srcIx, tType, execSize, lowBound, systolicDepth,
                             repeatCount, ops_per_chan);
    IGA_ASSERT(upBound >= lowBound, "source region footprint computation got "
                                    "it wrong: upBound is less than lowBound");

    uint32_t startRegNum = lowBound / m_DB.getGRF_BYTES_PER_REG();
    uint32_t upperRegNum = (upBound - 1) / m_DB.getGRF_BYTES_PER_REG();
    reg_range.push_back(std::make_pair(startRegNum, upperRegNum));
    // calculate extra_regs for HW workaround: treat Src2 as dpas.8x8 when
    // calculating register footpring (4 registers)
    if (m_DB.needDPASSrc2WA()) {
      if (srcIx == 2 && (repeatCount != 8 || systolicDepth != 8)) {
        uint32_t extraUpBound = getDPASSrcDepUpBound(
            srcIx, tType, execSize, lowBound, 8, 8, ops_per_chan);
        uint32_t extraUpRegNum =
            (extraUpBound - 1) / m_DB.getGRF_BYTES_PER_REG();
        if (extraUpRegNum >= m_DB.getGRF_REGS())
          IGA_FATAL("IGA RegDeps: DPAS src2 out of bounds due to HW WA");
        extra_regs.push_back(std::make_pair(startRegNum, extraUpRegNum));
      }
    }
    // calculate extra_regs for HW workaround: src1 always have 8 register
    // footpring
    if (m_DB.needDPASSrc1WA()) {
      if (srcIx == 1) {
        uint32_t extraUpBound = lowBound + m_DB.getGRF_BYTES_PER_REG() * 8;
        uint32_t extraUpRegNum =
            (extraUpBound - 1) / m_DB.getGRF_BYTES_PER_REG();
        if (extraUpRegNum >= m_DB.getGRF_REGS())
          IGA_FATAL("IGA RegDeps: DPAS src1 out of bounds due to HW WA");
        extra_regs.push_back(std::make_pair(startRegNum, extraUpRegNum));
      }
    }
  }
}

void DepSet::addDependency(const RegRangeType &reg_range) {
  for (uint32_t regNum = reg_range.first; regNum <= reg_range.second;
       regNum++) {
    addGrf(regNum);
    addToBucket(regNum);
  }

  // Using one of the special registers to add write dependency in to special
  // bucket This way it will always check that implicit dependency
  m_bucketList.push_back(m_DB.getBucketStart(RegName::ARF_CR));
}

void DepSet::addDependency(const RegRangeListType &reg_range) {
  for (RegRangeType pair : reg_range) {
    // when range is max(), which means it's null, skip it
    if (pair.first == std::numeric_limits<uint32_t>::max())
      continue;
    for (uint32_t regNum = pair.first; regNum <= pair.second; regNum++) {
      addGrf(regNum);
      addToBucket(regNum);
    }
  }

  // Using one of the special registers to add read dependency into special
  // bucket This way it will always check that implicit dependency
  m_bucketList.push_back(m_DB.getBucketStart(RegName::ARF_CR));
}

bool DepSetBuilder::needDstReadSuppressionWA(const Instruction &inst) const {
  if (mPlatformModel.platform != Platform::XE_HPG)
    return false;

  if (inst.getOpSpec().is(Op::MATH))
    return true;

  if (inst.isDF())
    return true;

  return false;
}

uint32_t DepSetBuilder::getBucketStart(RegName regname) const {
  uint32_t bucket = 0;
  switch (regname) {
  case iga::RegName::GRF_R:
    bucket = getGRF_START() / getBYTES_PER_BUCKET();
    break;
  case iga::RegName::ARF_A:
    bucket = getARF_A_START() / getBYTES_PER_BUCKET();
    break;
  case iga::RegName::ARF_ACC:
    bucket = getARF_ACC_START() / getBYTES_PER_BUCKET();
    break;
  case iga::RegName::ARF_F:
    bucket = getARF_F_START() / getBYTES_PER_BUCKET();
    break;
  case RegName::ARF_CR:
  case RegName::ARF_SR:
    bucket = getARF_SPECIAL_START() / getBYTES_PER_BUCKET();
    break;
  case RegName::ARF_S:
    bucket = getARF_SCALAR_START() / getBYTES_PER_BUCKET();
    break;
  default:
    // putting rest of archtecture registers in to same bucket
    bucket = getARF_F_START() / 32;
    break;
  }
  return bucket;
}

size_t DepSetBuilder::DpasMacroBuilder::getNumberOfSuppresionGroups(
    uint32_t srcIdx) const {

  if (srcIdx == 1) {
    return 1;
  }
  if (srcIdx == 2) {
    if (m_model.platform >= Platform::XE_HPC)
      return 4;
  }
  return 0;
}

size_t DepSetBuilder::DpasMacroBuilder::formSrcSuppressionBlock(
    InstListIterator startIt, uint32_t srcIdx) {
  // get the candidate block
  BitSet<> allDstBits(m_dsBuilder.getGRF_LEN());
  BitSet<> allSrcBits(m_dsBuilder.getGRF_LEN());
  BitSet<> allDstNoLastBits(m_dsBuilder.getGRF_LEN());
  BitSet<> allSrcNoLastBits(m_dsBuilder.getGRF_LEN());
  SuppressBlockPtrTy bptr =
      getSuppressionBlockCandidate(startIt, srcIdx, allDstBits, allSrcBits,
                                   allDstNoLastBits, allSrcNoLastBits);
  if (!bptr)
    return 0;

  size_t numSuppressed = 0;
  InstListIterator it = startIt;
  // advance inst iterator to the next instruction following the block
  // Note that this instruction must be a macro candidate, otherwise the
  // suppression block won't formed
  std::advance(it, bptr->size());
  assert(it != m_instList.end());


  // find until the last instruction that can be suppressed
  while (it != m_instList.end()) {
    if (!srcIsSuppressCandidate(**it, srcIdx))
      break;

    SrcRegRangeType src_range, src_extra_range;
    DstRegRangeType dst_range;
    m_inps.getDpasSrcDependency(**it, src_range, src_extra_range, m_model);
    m_inps.getDpasDstDependency(**it, dst_range);
    if (hasInternalDep(**it, dst_range, src_range,
                       GetDpasSystolicDepth((*it)->getDpasFc()) == 8))
      break;

    Operand &srcOp = (*it)->getSource(srcIdx);
    // TODO: to simplify the implementation, stop looking if the src is null
    if (srcOp.getDirRegName() != RegName::GRF_R)
      break;

    // found the first instruction that can't be suppressed. Stop looking.
    if (!bptr->contains(srcOp.getDirRegRef().regNum))
      break;

    bool skipSetLastBits = false;
    if (hasProducerConsumerDep(dst_range, src_range, allDstBits)) {
        break;
    }

    // at this point, we can add this DPAS into the macro
    ++numSuppressed;
    bptr->addRegRanges(src_range, src_extra_range, dst_range);
    if (!skipSetLastBits) {
      allSrcNoLastBits = allSrcBits;
      allDstNoLastBits = allDstBits;
    }
    setDstSrcBits(src_range, dst_range, allSrcBits, allDstBits);
    InstListIterator nextIt = std::next(it, 1);
    if (nextIt == m_instList.end())
      break;
    if (nextIsNotMacroCandidate(**it, **nextIt))
      break;
    it = nextIt;
  }

  if (numSuppressed) {
    // at least one instruction can be suppressed, the candidate block can be in
    // the macro udpate register footprint into DepSet
    updateRegFootprintsToDepSets(bptr->allSrcRange, bptr->allExtraSrcRange,
                                 bptr->allDstRange);

    // return the total instructions found can be in the macro
    return bptr->size() + numSuppressed;
  }
  return 0;
}

DepSetBuilder::DpasMacroBuilder::SuppressBlockPtrTy
DepSetBuilder::DpasMacroBuilder::getSuppressionBlockCandidate(
    InstListIterator startIt, uint32_t srcIdx, BitSet<> &allDstBits,
    BitSet<> &allSrcBits, BitSet<> &allDstNoLastBits,
    BitSet<> &allSrcNoLastBits, int forceGroupNum) const {
  assert(srcIdx == 1 || srcIdx == 2);
  size_t maxGroupNum =
      forceGroupNum < 0 ? getNumberOfSuppresionGroups(srcIdx) : forceGroupNum;
  // return null if the given src can't be suppressed
  if (!maxGroupNum)
    return nullptr;

  SuppressBlockPtrTy sb(new SuppressBlock(maxGroupNum, srcIdx == 1 ? 8 : 4));
  // try from the startIt to see if there are dpas sequence that can form the
  // suppression block check number of maxGroupSize to find the first one block
  // those can potentially be suppressed
  InstListIterator it = startIt;
  for (size_t i = 0; i < maxGroupNum; ++i) {
    InstListIterator nextIt = it;
    ++nextIt;
    // if next instruction is not a suppression candidate, there's no chance to
    // form a suppression block, return nullptr directly
    if (nextIt == m_instList.end())
      return nullptr;
    if (nextIsNotMacroCandidate(**it, **nextIt))
      return nullptr;
    if (!srcIsSuppressCandidate(**it, srcIdx))
      return nullptr;
    SrcRegRangeType src_range, src_extra_range;
    DstRegRangeType dst_range;
    m_inps.getDpasSrcDependency(**it, src_range, src_extra_range, m_model);
    m_inps.getDpasDstDependency(**it, dst_range);
    if (hasInternalDep(**it, dst_range, src_range,
                       GetDpasSystolicDepth((*it)->getDpasFc()) == 8))
      return nullptr;

    bool skipSetLastBits = false;
    if (hasProducerConsumerDep(dst_range, src_range, allDstBits)) {
        return nullptr;
    }
    uint16_t reg = (*it)->getSource(srcIdx).getDirRegRef().regNum;
    if (sb->partialOverlapped(reg))
      return nullptr;

    // found the first duplicated register, the block is formed
    if (sb->contains(reg))
      break;
    sb->addRegs(reg);
    sb->addRegRanges(src_range, src_extra_range, dst_range);
    if (!skipSetLastBits) {
      allSrcNoLastBits = allSrcBits;
      allDstNoLastBits = allDstBits;
    }
    setDstSrcBits(src_range, dst_range, allSrcBits, allDstBits);
    ++it;
  }

  assert(sb->size());
  return sb;
}

bool DepSetBuilder::DpasMacroBuilder::srcIsSuppressCandidate(
    const Instruction &inst, uint32_t srcIdx) const {
  // src1 always can be the candidate since all dpas depth must be the same
  // within the same macro
  if (srcIdx == 1)
    return true;
  if (srcIdx == 2) {
    // DP dpas must have rep count 4
    if (inst.isDF())
      return GetDpasRepeatCount(inst.getDpasFc()) == 4;

    // allow only rep count 8 for non-DP dpase
    return GetDpasRepeatCount(inst.getDpasFc()) == 8;
  }
  return false;
}

bool DepSetBuilder::DpasMacroBuilder::hasProducerConsumerDep(
    const DstRegRangeType &dst_range, const SrcRegRangeType &src_range,
    const BitSet<> &target_dst_bits) const {

  BitSet<> new_srcbits(m_dsBuilder.getGRF_LEN());
  BitSet<> new_dstbits(m_dsBuilder.getGRF_LEN());
  setDstSrcBits(src_range, dst_range, new_srcbits, new_dstbits);

  // check if there is RAW dependency
  if (target_dst_bits.intersects(new_srcbits))
    return true;
  return false;
}

// add given src and dst register ranges into DepSet
void DepSetBuilder::DpasMacroBuilder::updateRegFootprintsToDepSets(
    SrcRegRangeType &src_range, SrcRegRangeType &extra_src_range,
    DstRegRangeType &dst_range) {
  m_inps.addDependency(src_range);
  m_inps.addDependency(extra_src_range);
  m_oups.addDependency(dst_range);
};

void DepSetBuilder::DpasMacroBuilder::updateRegFootprintsToDepSets(
    RegRangeListType &src_range, RegRangeListType &extra_src_range,
    RegRangeListType &dst_range) {
  m_inps.addDependency(src_range);
  m_inps.addDependency(extra_src_range);
  m_oups.addDependency(dst_range);
};

Instruction &DepSetBuilder::DpasMacroBuilder::formMacro(size_t &dpasCnt) {
  dpasCnt = 1;
  InstListIterator cur = m_firstDpasIt;

  SrcRegRangeType src_range, src_extra_range;
  DstRegRangeType dst_range;
  m_inps.getDpasSrcDependency(**cur, src_range, src_extra_range, m_model);
  m_inps.getDpasDstDependency(**cur, dst_range);
  InstListIterator next = cur;
  next++;
  // early exit if there is no following instructions or dpas depth is not 8
  if (next == m_instList.end() ||
      GetDpasSystolicDepth((*cur)->getDpasFc()) != 8) {
    updateRegFootprintsToDepSets(src_range, src_extra_range, dst_range);
    return **cur;
  }



  dpasCnt = std::max(dpasCnt, formSrcSuppressionBlock(m_firstDpasIt, 1));
  dpasCnt = std::max(dpasCnt, formSrcSuppressionBlock(m_firstDpasIt, 2));

  if (dpasCnt == 1) {
    updateRegFootprintsToDepSets(src_range, src_extra_range, dst_range);
    return **cur;
  }

  // Set Atomic to all dpas in the macro except the last one
  // Also clean-up their SWSB if set from the input.
  InstListIterator it = m_firstDpasIt;
  for (size_t i = 0; i < dpasCnt - 1; ++it, ++i) {
    (*it)->addInstOpt(InstOpt::ATOMIC);
    (*it)->setSWSB(SWSB());
  }

  InstListIterator last = m_firstDpasIt;
  std::advance(last, dpasCnt - 1);
  assert(last != m_instList.end());
  return **last;
}

bool DepSetBuilder::DpasMacroBuilder::nextIsNotMacroCandidate(
    const Instruction &dpas, const Instruction &next_inst) const {
  if (!next_inst.getOpSpec().isDpasFormat())
    return true;

  // DPAS and DPASW should not be in the same macro
  if (next_inst.getOp() != dpas.getOp())
    return true;

  // DPAS with different CtrlMask (NoMask and no NoMask) cannot be in the same
  // macro
  if (next_inst.getMaskCtrl() != dpas.getMaskCtrl())
    return true;

  // DPAS with different execution mask cannot be in the same macro
  if (next_inst.getChannelOffset() != dpas.getChannelOffset())
    return true;

  // dpas with different depth should not be in the same macro
  // Note that different repeat count is allowed in the same macro
  uint32_t dpasSystolicDepth = GetDpasSystolicDepth(dpas.getDpasFc());
  uint32_t nextSystolicDepth = GetDpasSystolicDepth(next_inst.getDpasFc());
  if (dpasSystolicDepth != nextSystolicDepth)
    return true;

  // Dpas in the same macro must have the same datatypes in all src and dst
  // Except for src0 and dst which can accept having a mix of bf16 and fp32
  // datatypes
  assert(dpas.getSourceCount() == next_inst.getSourceCount());
  for (size_t i = 0; i < dpas.getSourceCount(); ++i) {
    Type curType = dpas.getSource(i).getType();
    Type nextType = next_inst.getSource(i).getType();
    if (curType != nextType &&
        (i != 0 || !isValidMixedTypes(curType, nextType)))
      return true;
  }
  Type curDstType = dpas.getDestination().getType();
  Type nextDstType = next_inst.getDestination().getType();
  if (curDstType != nextDstType &&
      !isValidMixedTypes(curDstType, nextDstType))
    return true;
  return false;
}

bool DepSetBuilder::DpasMacroBuilder::isValidMixedTypes(Type curType,
                                                        Type nextType) const {
  return false;
}

// set register range from start_reg to upper_reg into bit_set
void DepSetBuilder::DpasMacroBuilder::setBits(BitSet<> &bit_set,
                                              uint32_t start_reg,
                                              uint32_t upper_reg) const {
  // if the given register is max(), which means there's no register,
  // then no need to do anything
  if (start_reg == std::numeric_limits<uint32_t>::max() ||
      upper_reg == std::numeric_limits<uint32_t>::max())
    return;
  for (uint32_t i = start_reg; i <= upper_reg; ++i) {
    uint32_t grf_addr = i * m_dsBuilder.getGRF_BYTES_PER_REG();
    bit_set.set(grf_addr, m_dsBuilder.getGRF_BYTES_PER_REG());
  }
}

// set dst_range to dst_bits and src_range to src_bits
void DepSetBuilder::DpasMacroBuilder::setDstSrcBits(
    const SrcRegRangeType &src_range, const DstRegRangeType &dst_range,
    BitSet<> &src_bits, BitSet<> &dst_bits) const {
  for (const auto &regs : src_range) {
    setBits(src_bits, regs.first, regs.second);
  }
  setBits(dst_bits, dst_range.first, dst_range.second);
}

// check if the given register ranges having intersection
bool DepSetBuilder::DpasMacroBuilder::hasIntersect(
    const DepSet::RegRangeType &rr1, const DepSet::RegRangeType &rr2) const {
  BitSet<> rr1bits(m_dsBuilder.getGRF_LEN());
  BitSet<> rr2bits(m_dsBuilder.getGRF_LEN());
  setBits(rr1bits, rr1.first, rr1.second);
  setBits(rr2bits, rr2.first, rr2.second);

  return rr1bits.intersects(rr2bits);
}

// If rr1 and rr2 footprint are all the same, return true.
bool DepSetBuilder::DpasMacroBuilder::hasEntireOverlap(
    const DepSet::RegRangeType &rr1, const DepSet::RegRangeType &rr2) const {

  // overlap, check if it's completely overlap
  BitSet<> rr1bits(m_dsBuilder.getGRF_LEN());
  BitSet<> rr2bits(m_dsBuilder.getGRF_LEN());
  setBits(rr1bits, rr1.first, rr1.second);
  setBits(rr2bits, rr2.first, rr2.second);
  return rr1bits.equal(rr2bits);
}

// check if the instruction having internal dependency
// Instruction having internal dependency on dst to src is not allowed to be in
// a macro. Only for depth 8 dpas, internal dep on dst and src0 is allowed, but
// only when src0 and dst memory footprint is entirely the same
bool DepSetBuilder::DpasMacroBuilder::hasInternalDep(
    const Instruction &dpas, const DstRegRangeType &dst_range,
    const SrcRegRangeType &src_range, bool isDepth8) const {

  const auto &dstOp = dpas.getDestination();
  const auto &src0Op = dpas.getSource(0);

  if (hasIntersect(dst_range, src_range[1]))
    return true;

  if (hasIntersect(dst_range, src_range[2]))
    return true;

  // if src0 is null, skip it
  if (src_range[0].first != std::numeric_limits<uint32_t>::max()) {
    if (!isDepth8 && hasIntersect(dst_range, src_range[0]))
      return true;

    // for depth 8 dpas, sr0 and dst having the same footprint is
    // treated as no internal dependency for other rep_count, having
    // intersect is internal dependency
    if (isDepth8) {
      // if the ranges entirely overlap but the subregisters are different, we
      // have an internal dependency and hence, cannot form a macro
      if (hasEntireOverlap(dst_range, src_range[0])) {
          if (dstOp.getDirRegRef().subRegNum != src0Op.getDirRegRef().subRegNum)
            return true;
      // if there is partial overlap in registers, we cannot form a macro
      } else if (hasIntersect(dst_range, src_range[0]))
        return true;
    }
  }
  return false;
}


std::pair<DepSet *, DepSet *> DepSetBuilder::createDPASSrcDstDepSet(
    const InstList &insList, InstListIterator instIt,
    const InstIDs &inst_id_counter, size_t &dpasCnt,
    SWSB_ENCODE_MODE enc_mode) {

  // create DepSet for input
  DepSet *inps = new DepSet(inst_id_counter, *this);
  mAllDepSet.push_back(inps);
  inps->setDepType(DEP_TYPE::READ);
  setDEPPipeClass(enc_mode, *inps, **instIt, mPlatformModel);

  // create DepSet for output
  DepSet *oups = new DepSet(inst_id_counter, *this);
  mAllDepSet.push_back(oups);
  oups->setDepType(DEP_TYPE::WRITE);
  setDEPPipeClass(enc_mode, *oups, **instIt, mPlatformModel);

  // identify dpas macro
  DpasMacroBuilder dmb(
      *this, mPlatformModel, insList, instIt, *inps, *oups, enc_mode);
  Instruction &lastDpas = dmb.formMacro(dpasCnt);

  // let the last instruciton in the macro represent this DepSet
  inps->m_instruction = &lastDpas;
  oups->m_instruction = &lastDpas;

  return std::make_pair(inps, oups);
}

DepSet *DepSetBuilder::createSrcDepSet(Instruction &i,
                                       const InstIDs &inst_id_counter,
                                       SWSB_ENCODE_MODE enc_mode) {
  DepSet *inps = new DepSet(inst_id_counter, *this);
  mAllDepSet.push_back(inps);

  inps->m_instruction = &i;
  inps->setDepType(DEP_TYPE::READ);

  setDEPPipeClass(enc_mode, *inps, i, mPlatformModel);

  inps->setInputsFlagDep();
  if (i.getOpSpec().isAnySendFormat())
    inps->setInputsSendDescDep();
  inps->setInputsSrcDep();
  return inps;
}

void DepSet::addGrf(size_t reg) {
  addGrfBytes(reg, 0, m_DB.getGRF_BYTES_PER_REG());
}

void DepSet::setInputsFlagDep() {
  // does it read the flag register
  //  predication does this
  //  conditional modifier on 'sel' does this
  const Predication &pred = m_instruction->getPredication();
  const FlagModifier fm = m_instruction->getFlagModifier();
  bool readsFlagRegister =
      pred.function != PredCtrl::NONE ||
      (m_instruction->getOp() == Op::SEL && fm != FlagModifier::NONE);
  if (readsFlagRegister) {
    // add the ARF offset from ExecMaskOffset
    // E.g.
    // (f1.0) op (16|M16) ...
    // is touching f1.1
    const RegRef &fr = m_instruction->getFlagReg();
    size_t fByteOff =
        (size_t)fr.regNum * m_DB.getARF_F_BYTES_PER_REG() +
        (size_t)fr.subRegNum * 2; // FIXME: magic number (needs some thought
                                  // should be bytes per subreg)
    size_t execOff =
        4 * (static_cast<size_t>(m_instruction->getChannelOffset()));
    fByteOff += execOff / 8; // move over by ARF offset
    size_t execSize = static_cast<size_t>(m_instruction->getExecSize());
    size_t addr = (size_t)m_DB.getARF_F_START() + fByteOff;
    addFBytes(addr, execSize);
    m_bucketList.push_back(addr / m_DB.getBYTES_PER_BUCKET());
  }
}

void DepSet::setInputsSendDescDep() {
  IGA_ASSERT(m_instruction->getOpSpec().isAnySendFormat(),
      "DepSet::setInputsSendDescDep: must be send format");
  // set up reg footprint for desc and exdesc if they are registers
  auto desc = m_instruction->getMsgDescriptor();
  if (desc.isReg()) {
    addABytesAndBukets(desc.reg.regNum);
  }
  auto exDesc = m_instruction->getExtMsgDescriptor();
  if (exDesc.isReg()) {
    addABytesAndBukets(exDesc.reg.regNum);
  }
}

void DepSet::setInputsSrcDep() {
  uint32_t execSize = static_cast<uint32_t>(m_instruction->getExecSize());

  // mac/mach has implicitly read to acc0
  if (m_instruction->getOp() == Op::MAC || m_instruction->getOp() == Op::MACH ||
      m_instruction->getOp() == Op::MACL) {
    setSrcRegion(RegName::ARF_ACC, RegRef(0, 0), Region::SRC110, execSize,
                 16); // assume it's :w, though for acc access it actually does
                      // not matter, because its footprint will always count as
                      // acc0/acc1 pair
  }

  // For the instruction having no srcs, we still need to add ARF_CR to mark the
  // dependency This is for the case that:
  //      and (1|M0)               cr0.0<1>:ud   cr0.0<0;1,0>:ud   0xFFFFFFCF:ud
  //      {A@1} nop {A@1}
  // It's requested that the instruction following the one having architecture
  // register (CR/CE/SR) access, It must mark to sync with all pipes, even if
  // it's a nop. nop has no src and dst so we mark it here to force setting swsb
  // if required
  //
  // For sync instructions we do not need to consider its dependency. Though it
  // may still apply the above case so still set ARF_CR to it.
  if (m_instruction->getSourceCount() == 0 ||
      m_instruction->getOpSpec().is(Op::SYNC)) {
    m_bucketList.push_back(m_DB.getBucketStart(RegName::ARF_CR));
    return;
  }

  // Check if inst has intra-read-suppresion: If a multi-src inst has two or
  // more src read to the same register, regardless if they are overlapping,
  // it is considered having intra-read-suppression. Will need to force set
  // the src's footprint to the full register when needIntraReadSuppressionWA
  // Keep track of the touch register number in trackUseGrfNum and check
  // during setting each src's footprint.
  std::vector<unsigned> trackUseGrfNum;

  // check all the source operands
  for (unsigned srcIx = 0, numSrcs = m_instruction->getSourceCount();
       srcIx < numSrcs; srcIx++) {
    const Operand &op = m_instruction->getSource(srcIx);
    auto tType = op.getType();
    uint32_t typeSizeInBits = TypeSizeInBitsWithDefault(tType, 32);

    // possibly need to fixup the region
    Region rgn = op.getRegion();
    if (numSrcs == 3) {
      if (m_instruction->isMacro()) {
        // macros and sends are packed access only
        // madm (4) r3.acc1 r7.acc2 r9.acc4 r6.noacc
        rgn = Region::SRC110;
      } else {
        // ternary align 1 has some implicit regions that need filling in
        //
        // c.f. https://gfxspecs.intel.com/Predator/Home/Index/3017
        // "GEN10 Regioning Rules for Align1 Ternary Operations"
        //   1. Width is 1 when Vertical and Horizontal Strides are both zero
        //   (broadcast access).
        //   2. Width is equal to Vertical Stride when Horizontal Stride is
        //   zero.
        //   3. Width is equal to Vertical Stride/Horizontal Stride when both
        //   Strides are non-zero.
        //   4. Vertical Stride must not be zero if Horizontal Stride is
        //   non-zero.
        //      This implies Vertical Stride is always greater than Horizontal
        //      Stride.
        //   5. For Source 2, if Horizontal Stride is non-zero, then Width is
        //   the
        //      a register's width of elements (e.g. 8 for a 32-bit data type).
        //      Otherwise, if Horizontal Stride is 0, then so is the Vertical
        //      (and rule 1 applies). This means Vertical Stride is always
        //      'Width' * 'Horizontal Stride'.
        if (srcIx < 2) {
          // <V;H>
          if (rgn.getVt() == Region::Vert::VT_0 &&
              rgn.getHz() == Region::Horz::HZ_0) {
            rgn.set(Region::Width::WI_1); // by #1
          } else if (rgn.getHz() == Region::Horz::HZ_0) {
            rgn.w = rgn.v; // by #2
          } else if (rgn.getVt() != Region::Vert::VT_0 &&
                     rgn.getHz() != Region::Horz::HZ_0) {
            rgn.w = rgn.v / rgn.h; // by #3
          } else {
            // error condition #4, just use vertical stride
            // SPECIFY: should this be an assertion?
            rgn.w = rgn.v;
          }
        } else { // (srcIx == 2)
          // <H>
          // <H> -> <H;1,0>
          // if (rgn.h == Region::HZ_0) {
          //    rgn.w = Region::WI_1; // by #5 and #1
          //} else {
          //    rgn.w = static_cast<Region::Width>(GRF_BYTES_PER_REG/typeSize);
          //    // by #5
          //}
          // rgn.v = static_cast<Region::Vert>(
          //    static_cast<int>(rgn.w)*static_cast<int>(rgn.h)/typeSize); // by
          //    #5
          rgn.set(static_cast<Region::Vert>(static_cast<size_t>(rgn.h)),
                  Region::Width::WI_1, Region::Horz::HZ_0);
        }
      }
    } else if (numSrcs == 2 && m_instruction->isMacro()) {
      // math macro
      rgn = Region::SRC110;
    } // not ternary, not macro

    switch (op.getKind()) {
    case Operand::Kind::DIRECT:
      if (m_instruction->getOpSpec().isAnySendFormat()) {
        if (op.getDirRegName() == RegName::GRF_R) {
          // send source GRF (not null reg)
          int nregs = (srcIx == 0) ? m_instruction->getSrc0Length()
                                   : m_instruction->getSrc1Length();
          // if we can't tell the number of registers
          // (e.g. the descriptor is in a register),
          // then we must conservatively assume the worst (32)
          if (nregs < 0)
            nregs = 32;
          uint32_t regNum = op.getDirRegRef().regNum;
          for (uint32_t i = 0; i < (uint32_t)nregs; i++) {
            if ((regNum + i) >= m_DB.getGRF_REGS()) {
              break;
            }
            addGrf((size_t)regNum + i);
            addToBucket(regNum + i);
          }
          addToBucket(m_DB.getBucketStart(RegName::ARF_CR));
        }
      } else {
        if (m_instruction->getOp() == Op::BRC) {
          rgn = Region::SRC221;
        }

        auto useBuckets = setSrcRegion(op.getDirRegName(), op.getDirRegRef(),
                            rgn, execSize, typeSizeInBits);

        // check and add extra footprint for intra read suppression instructions.
        // The WA only applies to GRF. In the case, bucket number is equal to
        // GRF number.
        if (m_DB.needIntraReadSuppressionWA() &&
            op.getDirRegName() == RegName::GRF_R) {
          IGA_ASSERT(useBuckets.first != m_DB.getTOTAL_BUCKETS(),
              "SWSB: GRF src must have valid bucket number");
          uint32_t grfNum = useBuckets.first;
          for (uint32_t i = 0; i < useBuckets.second; ++i, ++grfNum) {
            if (std::find(trackUseGrfNum.begin(), trackUseGrfNum.end(),
                  useBuckets.first) != trackUseGrfNum.end()) {
              // found the same grf use by different srcs. Set this DepSet to
              // have full register footprint
              addGrf(grfNum);
            } else {
              // no intra read to previous src
              trackUseGrfNum.push_back(grfNum);
            }
          }
        }
      }
      break;
    case Operand::Kind::MACRO: {
      setSrcRegion(op.getDirRegName(), op.getDirRegRef(), rgn, execSize,
                   typeSizeInBits);
      auto mme = op.getMathMacroExt();
      if (mme != MathMacroExt::NOMME && mme != MathMacroExt::INVALID) {
        int mmeNum = static_cast<int>(op.getMathMacroExt()) -
                     static_cast<int>(MathMacroExt::MME0);
        setSrcRegion(RegName::ARF_ACC, RegRef(mmeNum, 0), Region::SRC110,
                     execSize, typeSizeInBits);
      }
      break;
    }
    case Operand::Kind::INDIRECT: {
      setHasIndirect();
      setDepType(DEP_TYPE::READ_ALWAYS_INTERFERE);

      if (op.getDirRegName() == RegName::ARF_S) {
        IGA_ASSERT(m_instruction->isGatherSend(),
                   "Non-send instruction has indirect scalar register src");
        // Mark src access to 1 byte that we will insert force sync before and
        // after gather send so the actual length doesn't matter.
        setSendSrcScalarRegRegion(op.getIndAddrReg(), 1);
        break;
      }

      auto rgn = op.getRegion();
      if (rgn.getVt() == Region::Vert::VT_VxH) {
        // VxH or Vx1 mode
        //   op (K)  dst   r[a0.0]<W,1>:w  (reads K/W elements)
        //   op (K)  dst   r[a0.0]<1,0>:w  (reads K/W elements)
        setSrcRegion(RegName::ARF_A, op.getIndAddrReg(), Region::SRC110,
                     execSize / rgn.w, //
                     16);              // :w is 16-bits a piece
      } else {
        // uniform: consumes one value in a0
        // op (..)  dst   r[a0.0]<16,8,1>:w
        setSrcRegion(RegName::ARF_A, op.getIndAddrReg(), Region::SRC110,
                     1,   // 1 element only
                     16); // :w is 16-bits a piece
      }
      // we can't do anything else for this
      break;
    }
    default:
      break;
    }
  }
}

void DepSet::setOutputsFlagDep() {
  const FlagModifier fm = m_instruction->getFlagModifier();
  bool writesFlagRegister =
      fm != FlagModifier::NONE &&
      m_instruction->getOp() != Op::SEL; // sel uses flag modifier as input
  if (writesFlagRegister) {
    const RegRef &fr = m_instruction->getFlagReg();
    int fByteOff = fr.regNum * m_DB.getARF_F_BYTES_PER_REG() +
                   fr.subRegNum * 2; // 2 bytes per subreg
    int execOff = 4 * (static_cast<int>(m_instruction->getChannelOffset()));
    fByteOff += execOff / 8; // move over by ARF offset
    int execSize = static_cast<int>(m_instruction->getExecSize()); // 1 <<
                                                                   // (static_cast<int>(m_instruction->getExecSize())
                                                                   // - 1);
    size_t addr = (size_t)m_DB.getARF_F_START() + fByteOff;
    addFBytes(addr, execSize);
    m_bucketList.push_back(addr / m_DB.getBYTES_PER_BUCKET());
  }
}

void DepSet::setOutputsDstDep() {
  int execOff = 4 * (static_cast<int>(m_instruction->getChannelOffset()));
  int execSize = static_cast<int>(m_instruction->getExecSize()); // 1 <<
                                                                 // (static_cast<int>(m_instruction->getExecSize())
                                                                 // - 1);

  if (!m_instruction->getOpSpec().supportsDestination()) {
    // For the instruction having no srcs, we still need to add ARF_CR to mark
    // the dependency This is for the case that:
    //      and (1|M0)               cr0.0<1>:ud   cr0.0<0;1,0>:ud 0xFFFFFFCF:ud
    //      {A@1} nop {A@1}
    // It's requested that the instruction following the one having architecture
    // register (CR/CE/SR) access, It must mark to sync with all pipes, even if
    // it's a nop. nop has no src and dst so we mark it here to force setting
    // swsb if required
    m_bucketList.push_back(m_DB.getBucketStart(RegName::ARF_CR));
    return;
  }

  const auto &op = m_instruction->getDestination();
  auto tType = op.getType();
  auto typeSizeBits = TypeSizeInBitsWithDefault(tType, 32);

  // A trick to correct the call dst footprint that its dst type is implicit dw
  // but it actually writes to continuous two dw. This causes the issue
  // for SIMD1 case. Double the exec_size to workaround the case. For other simd
  // width, it's ok since we already conservative assume the dst footprint by
  // times of exec_size, which is not neccessary for call/calla that their dst
  // is scalar 2 dw.
  if (m_instruction->getOp() == Op::CALL ||
      m_instruction->getOp() == Op::CALLA) {
    assert(tType == Type::D && op.getKind() == Operand::Kind::DIRECT &&
           op.getDirRegName() == RegName::GRF_R);
    if (execSize == 1)
      execSize = 2;
  }

  // Instructions having implicit write to acc
  if (m_instruction->hasInstOpt(InstOpt::ACCWREN) ||
      m_instruction->getOp() == Op::SUBB ||
      m_instruction->getOp() == Op::ADDC ||
      m_instruction->getOp() == Op::MACL ||
      m_instruction->getOp() == Op::MACH) {
    auto elemsPerAccReg = 8 * m_DB.getARF_ACC_BYTES_PER_REG() /
                          typeSizeBits; // e.g. 8 subreg elems for :f
    RegRef ar;
    ar.regNum = (uint16_t)(execOff / elemsPerAccReg);
    ar.subRegNum = (uint16_t)(execOff % elemsPerAccReg);
    setDstRegion(RegName::ARF_ACC, ar, Region::DST1, execSize, typeSizeBits);
  }

  Region rgn = op.getRegion();
  switch (op.getKind()) {
  case Operand::Kind::DIRECT:
    // send target (a GRF, not null reg)
    if (m_instruction->getOpSpec().isAnySendFormat() &&
        op.getDirRegName() == RegName::GRF_R) {
      int nregs = m_instruction->getDstLength();
      // getDstLength return -1 when it's not able to deduce the length
      // we have to be conservative and use the max
      if (nregs < 0)
        nregs = 32;
      for (uint32_t i = 0; i < (uint32_t)nregs; i++) {
        uint32_t regNum = op.getDirRegRef().regNum;
        if ((regNum + i) >= m_DB.getGRF_REGS()) {
          break;
        }

        addGrf((size_t)regNum + i);
        addToBucket(regNum + i);
      }
      rgn = Region::DST1;
      addToBucket(m_DB.getBucketStart(RegName::ARF_CR));

    } else if (m_instruction->getOpSpec().is(Op::MATH) &&
               op.getDirRegName() == RegName::GRF_R &&
               m_instruction->getMathFc() == MathFC::IDIV) {
      uint16_t regNum = op.getDirRegRef().regNum;
      addGrf(regNum);
      addToBucket(regNum);
      addGrf((size_t)regNum + 1);
      addToBucket((size_t)regNum + 1);
      addToBucket(m_DB.getBucketStart(RegName::ARF_CR));
    } else {
      // normal GRF target
      setDstRegion(op.getDirRegName(), op.getDirRegRef(), op.getRegion(),
                   execSize, typeSizeBits);
    }
    break;
  case Operand::Kind::MACRO: {
    // math macro
    // GRF + ACC
    setDstRegion(op.getDirRegName(), op.getDirRegRef(), Region::DST1, execSize,
                 typeSizeBits);
    auto mme = op.getMathMacroExt();
    if (mme != MathMacroExt::NOMME && mme != MathMacroExt::INVALID) {
      // and the accumultator
      int mmeNum = static_cast<int>(mme) - static_cast<int>(MathMacroExt::MME0);
      RegRef mmeReg{static_cast<uint8_t>(mmeNum), 0};
      setDstRegion(RegName::ARF_ACC, mmeReg, Region::DST1, execSize,
                   typeSizeBits);
    }
    break;
  }
  case Operand::Kind::INDIRECT:
    setHasIndirect();

    // indirect destinations use a0
    //
    // writes use one a0 value
    setDstRegion(RegName::ARF_A, op.getIndAddrReg(), Region::DST1,
                 1,   // one element only
                 16); // :w is 16-bits
    setDepType(DEP_TYPE::WRITE_ALWAYS_INTERFERE);

    break;
  default:
    break;
  }

}


DepSet *DepSetBuilder::createDstDepSet(Instruction &i,
                                       const InstIDs &inst_id_counter,
                                       SWSB_ENCODE_MODE enc_mode) {
  DepSet *oups = new DepSet(inst_id_counter, *this);
  mAllDepSet.push_back(oups);

  oups->m_instruction = &i;
  setDEPPipeClass(enc_mode, *oups, i, mPlatformModel);

  oups->setDepType(DEP_TYPE::WRITE);

  oups->setOutputsFlagDep();
  oups->setOutputsDstDep();
  return oups;
}

DepSet *DepSetBuilder::createDstDepSetFullGrf(Instruction &i,
                                              const InstIDs &inst_id_counter,
                                              SWSB_ENCODE_MODE enc_mode,
                                              bool setFlagDep) {
  DepSet *oups = new DepSet(inst_id_counter, *this);
  mAllDepSet.push_back(oups);

  oups->m_instruction = &i;
  setDEPPipeClass(enc_mode, *oups, i, mPlatformModel);

  oups->setDepType(DEP_TYPE::WRITE);
  if (setFlagDep)
    oups->setOutputsFlagDep();
  oups->setOutputsDstDepFullGrf();
  return oups;
}

void DepSet::setOutputsDstDepFullGrf() {
  size_t execSize = static_cast<size_t>(m_instruction->getExecSize()); // 1 <<
                                                                       // (static_cast<int>(m_instruction->getExecSize())
                                                                       // - 1);

  const auto &op = m_instruction->getDestination();
  auto tType = op.getType();
  auto typeSizeBits = TypeSizeInBitsWithDefault(tType, 32);

  Region rgn = op.getRegion();
  switch (op.getKind()) {
  case Operand::Kind::DIRECT: {
    RegName rn = op.getDirRegName();
    if (rn != RegName::GRF_R)
      return;
    RegRef rr = op.getDirRegRef();
    m_dType = DEP_TYPE::WRITE;

    size_t hz = static_cast<size_t>(rgn.getHz());
    // sets a region for a basic operand
    size_t grfAddr = addressOf(rn, rr, typeSizeBits);
    if (grfAddr >= m_DB.getTOTAL_BITS())
      return;

    size_t lowBound = m_DB.getTOTAL_BITS();
    size_t upperBound = 0;

    // caculate the access registers range from region
    for (size_t ch = 0; ch < execSize; ch++) {
      size_t offset = ch * hz * typeSizeBits / 8;
      size_t start = grfAddr + offset;
      // bits.set(start, typeSizeBits / 8);

      if (start < lowBound) {
        lowBound = start;
      }
      if (start + typeSizeBits / 8 > upperBound) {
        upperBound = start + typeSizeBits / 8;
      }
    }
    size_t startRegNum = lowBound / m_DB.getBYTES_PER_BUCKET();
    size_t upperRegNum = (upperBound - 1) / m_DB.getBYTES_PER_BUCKET();

    for (size_t i = startRegNum; i <= upperRegNum; i++) {
      // set the entire grf
      addGrf(i);
      m_bucketList.push_back(i);
    }
    addToBucket(m_DB.getBucketStart(RegName::ARF_CR));
    break;
  }
  default:
    // if the dst is not a GRF, set dep as regular cases
    setOutputsDstDep();
    break;
  }
}

void DepSet::getDpasDstDependency(const Instruction &inst,
                                  RegRangeType &reg_range) {
  uint32_t execSize = static_cast<uint32_t>(inst.getExecSize());

  const auto &op = inst.getDestination();
  auto tType = op.getType();
  uint32_t typeSizeBits = TypeSizeInBitsWithDefault(tType, 32);
  IGA_ASSERT(op.getDirRegName() == RegName::GRF_R, "DPAS with non-GRF");

  // calculated used register region low and upper bound
  uint32_t lowBound =
      addressOf(op.getDirRegName(), op.getDirRegRef(), typeSizeBits);
  // elements_size is the size of total elements to be calculated in one
  // operation
  uint32_t elements_size = execSize * typeSizeBits / 8;

  // For dpas, the destination region depends on Reapeat count
  uint32_t repeatCount = GetDpasRepeatCount(inst.getDpasFc());
  uint32_t upperBound = lowBound + elements_size * repeatCount;

  uint32_t startRegNum = lowBound / m_DB.getGRF_BYTES_PER_REG();
  uint32_t upperRegNum = (upperBound - 1) / m_DB.getGRF_BYTES_PER_REG();

  reg_range.first = startRegNum;
  reg_range.second = upperRegNum;
}

bool static const isSpecial(RegName rn) {
  // The special registers (architecture registers) are CR/SR/CE
  // All the other ARF should have explicitly access so no need
  // the special handling on it
  // Others are SP, IP, IM, DBG
  if (rn == RegName::ARF_CR || rn == RegName::ARF_SR || rn == RegName::ARF_CE ||
      rn == RegName::ARF_FC) {
    return true;
  }
  return false;
}

void DepSet::setSendSrcScalarRegRegion(const RegRef &rr, uint32_t numBytes) {
  uint32_t lowBound = addressOf(RegName::ARF_S, rr, 8);
  uint32_t upperBound = lowBound + numBytes;
  uint32_t startBucketNum = lowBound / m_DB.getBYTES_PER_BUCKET();
  uint32_t upperBucketNum = (upperBound - 1) / m_DB.getBYTES_PER_BUCKET();

  bits->set(lowBound, numBytes);
  for (uint32_t i = startBucketNum; i <= upperBucketNum; i++) {
    m_bucketList.push_back(i);
  }

  // Using one of the special registers to add read dependency in to special
  // bucket This way it will always check that implicit dependency
  m_bucketList.push_back(m_DB.getBucketStart(RegName::ARF_CR));
}

std::pair<uint32_t, uint32_t>
DepSet::setSrcRegion(RegName rn, RegRef rr, Region rgn, uint32_t execSize,
                     uint32_t typeSizeBits) {
  // Previously we also skip acc register with subRegNum < 2 ==> WHY?
  // The condition is (rn == RegName::ARF_ACC && rr.subRegNum < 2)
  if (!isRegTracked(rn)) {
    // In the case that an instruction has only non-tracked register access,
    // we still want it to mark swsb if its previous instruction having
    // architecture register access, so add the special bucket ARF_CR anyway
    // in case of swsb is required
    // e.g.
    // (W) mov(1|M0)   sr0.2<1>:ud   0xFFFFFFFF:ud   {A@1}
    // (W) mov(1|M0)   f0.0<1>:ud    0x0 : ud        {A@1}
    // the second mov required A@1 be set
    m_bucketList.push_back(m_DB.getBucketStart(RegName::ARF_CR));
    return std::make_pair(m_DB.getTOTAL_BUCKETS(), 0);
  }

  uint32_t v = 1, w = 1, h = 0; // e.g. old-style default regions
  if (rgn != Region::INVALID) {
    v = static_cast<uint32_t>(rgn.getVt());
    w = static_cast<uint32_t>(rgn.getWi());
    w = w == 0 ? 1 : w;
    h = static_cast<uint32_t>(rgn.getHz());
  }

  uint32_t lowBound = m_DB.getTOTAL_BITS();
  uint32_t upperBound = 0;

  // sets a region for a basic operand
  uint32_t rowBase = addressOf(rn, rr, typeSizeBits);
  if (rowBase >= m_DB.getTOTAL_BITS()) // tm0 or something
    return std::make_pair(m_DB.getTOTAL_BUCKETS(), 0);

  // the acc0/acc1 could have overlapping. (same as acc2/acc3, acc4/acc5, ...)
  // To be conservative we always mark them together when
  // one of it is used
  // This is to resolve the acc arrangement difference between int and float
  // E.g. acc3.3:f and acc2.7:w have an overlap
  if (rn == RegName::ARF_ACC) {
    RegRef tmp_rr;
    tmp_rr.regNum = (rr.regNum % 2) == 0 ? rr.regNum : rr.regNum - 1;
    tmp_rr.subRegNum = 0;
    lowBound = addressOf(rn, tmp_rr, typeSizeBits);
    // reg access cross two acc
    upperBound = lowBound + 2 * m_DB.getARF_A_BYTES_PER_REG();
    bits->set(lowBound, 2 * (size_t)m_DB.getARF_A_BYTES_PER_REG());
  } else {
    uint32_t rows = execSize / w;
    rows = (rows != 0) ? rows : 1;
    // size_t bytesTouched = 0;
    for (uint32_t y = 0; y < rows; y++) {
      uint32_t offset = rowBase;
      for (uint32_t x = 0; x < w; x++) {
        bits->set(offset, typeSizeBits / 8);
        if (offset < lowBound) {
          lowBound = offset;
        }
        if (offset + typeSizeBits / 8 > upperBound) {
          upperBound = offset + typeSizeBits / 8;
        }
        offset += h * typeSizeBits / 8;
        // bytesTouched += typeSize;
      }
      rowBase += v * typeSizeBits / 8;
    }
  }

  // size_t extentTouched = upperBound - lowBound;
  IGA_ASSERT(upperBound >= lowBound,
             "source region footprint computation got it wrong: upperBound "
             "is less than lowBound");
  uint32_t startRegNum = lowBound / m_DB.getBYTES_PER_BUCKET();
  uint32_t upperRegNum = (upperBound - 1) / m_DB.getBYTES_PER_BUCKET();

  for (uint32_t i = startRegNum; i <= upperRegNum; i++) {
    m_bucketList.push_back(i);
  }

  // Special registers has side effects so even if there is no direct
  // interference subsequent instrucion might depend on it. Also the prior
  // instrucions may depends on it so force to sync all pipes to this
  // instruction and the following instruction
  if (isSpecial(rn)) {
    setHasSR();
    m_dType = DEP_TYPE::READ_ALWAYS_INTERFERE;
  } else {
    // Using one of the special registers to add write dependency in to special
    // bucket This way it will always check that implicit dependency
    m_bucketList.push_back(m_DB.getBucketStart(RegName::ARF_CR));
  }
  return std::make_pair(startRegNum, upperRegNum - startRegNum + 1);
}

void DepSet::setDstRegion(RegName rn, RegRef rr, Region rgn, uint32_t execSize,
                          uint32_t typeSizeBits) {
  // Previously we also skip acc register with subRegNum < 2 ==> WHY?
  // The condition is (rn == RegName::ARF_ACC && rr.subRegNum < 2)
  if (!isRegTracked(rn)) {
    // In the case that an instruction has only non-tracked register access,
    // we still want it to mark swsb if its previous instruction having
    // architecture register access, so add the special bucket ARF_CR anyway
    // in case of swsb is required
    // e.g.
    // (W) mov(1|M0)   sr0.2<1>:ud   0xFFFFFFFF:ud   {A@1}
    // (W) mov(1|M0)   f0.0<1>:ud    0x0 : ud        {A@1}
    // the second mov required A@1 be set
    m_bucketList.push_back(m_DB.getBucketStart(RegName::ARF_CR));
    return;
  }

  m_dType = DEP_TYPE::WRITE;

  auto setBitsFromRegRef = [&](RegRef regRef) {
    // sets a region for a basic operand
    uint32_t grfAddr = addressOf(rn, regRef, typeSizeBits);
    if (grfAddr >= m_DB.getTOTAL_BITS())
      return;
    uint32_t lowBound = m_DB.getTOTAL_BITS();
    uint32_t upperBound = 0;

    // the acc0/acc1 could have overlapping. (same as acc2/acc3, acc4/acc5, ...)
    // To be conservative we always mark them together when one of it is used
    // This is to resolve the acc arrangement difference between int and float
    // E.g. acc3.3:f and acc2.7:w have an overlap
    if (rn == RegName::ARF_ACC) {
      RegRef tmp_rr;
      tmp_rr.regNum =
          (regRef.regNum % 2) == 0 ? regRef.regNum : regRef.regNum - 1;
      tmp_rr.subRegNum = 0;
      lowBound = addressOf(rn, tmp_rr, typeSizeBits);

      // reg access cross two acc
      upperBound = lowBound + 2 * m_DB.getARF_A_BYTES_PER_REG();
      bits->set(lowBound, 2 * (size_t)m_DB.getARF_A_BYTES_PER_REG());
    } else {
      // otherwise caculate the access registers range from region
      uint32_t hz = static_cast<uint32_t>(rgn.getHz());
      for (uint32_t ch = 0; ch < execSize; ch++) {
        uint32_t offset = ch * hz * typeSizeBits / 8;
        uint32_t start = grfAddr + offset;
        bits->set(start, typeSizeBits / 8);

        if (start < lowBound) {
          lowBound = start;
        }
        if (start + typeSizeBits / 8 > upperBound) {
          upperBound = start + typeSizeBits / 8;
        }
      }
    }

    if (isRegTracked(rn)) {
      uint32_t startRegNum = lowBound / m_DB.getBYTES_PER_BUCKET();
      uint32_t upperRegNum = (upperBound - 1) / m_DB.getBYTES_PER_BUCKET();

      for (uint32_t i = startRegNum; i <= upperRegNum; i++) {
        // note: bucket start is already included in 'i' calculation
        // used to be: m_bucketList.push_back(i + DepSet::getBucketStart(rn));
        m_bucketList.push_back(i);
      }
    }
  };
  setBitsFromRegRef(rr);


  // Special registers has side effects so even if there is no direct
  // interference subsequent instrucion might depend on it. Also the prior
  // instrucions may depends on it so force to sync all pipes to this
  // instruction and the following instruction
  if (isSpecial(rn)) {
    setHasSR();
    m_dType = DEP_TYPE::WRITE_ALWAYS_INTERFERE;
  } else {
    // Using one of the special registers to add write dependency in to special
    // bucket This way it will always check that implicit dependency
    m_bucketList.push_back(m_DB.getBucketStart(RegName::ARF_CR));
  }
}

uint32_t DepSet::addressOf(RegName rnm, const RegRef &rr,
                           uint32_t typeSizeBits) const {
  auto getAddressOf = [&rr, &typeSizeBits](uint32_t regStart,
                                           uint32_t bytePerReg) {
    return regStart + rr.regNum * bytePerReg + rr.subRegNum * typeSizeBits / 8;
  };

  switch (rnm) {
  case RegName::GRF_R:
    return getAddressOf(m_DB.getGRF_START(), m_DB.getGRF_BYTES_PER_REG());
  case RegName::ARF_A:
    return getAddressOf(m_DB.getARF_A_START(), m_DB.getARF_A_BYTES_PER_REG());
  case RegName::ARF_ACC:
    return getAddressOf(m_DB.getARF_ACC_START(),
                        m_DB.getARF_ACC_BYTES_PER_REG());
  case RegName::ARF_F:
    return getAddressOf(m_DB.getARF_F_START(), m_DB.getARF_F_BYTES_PER_REG());
  case RegName::ARF_CR:
  case RegName::ARF_SR:
  case RegName::ARF_CE:
  case RegName::ARF_FC:
    return getAddressOf(m_DB.getARF_SPECIAL_START(),
                        m_DB.getARF_SPECIAL_BYTES_PER_REG());
  case RegName::ARF_S:
    return getAddressOf(m_DB.getARF_SCALAR_START(),
                        m_DB.getARF_SCALAR_BYTES_PER_REG());
  default:
    return m_DB.getTOTAL_BITS();
  }
}

bool DepSet::isRegTracked(RegName rnm) const {
  switch (rnm) {
  case RegName::GRF_R:
  case RegName::ARF_A:
  case RegName::ARF_ACC:
  case RegName::ARF_F:
  case RegName::ARF_S:
    return true;
  default:
    return false || isSpecial(rnm);
  }
}

void DepSet::addGrfBytes(size_t reg, size_t subRegBytes, size_t num_bytes) {
  size_t grf_addr =
      m_DB.getGRF_START() + reg * m_DB.getGRF_BYTES_PER_REG() + subRegBytes;
  if (grf_addr < m_DB.getGRF_START() ||
      (grf_addr + num_bytes >
       (size_t)m_DB.getGRF_START() + m_DB.getGRF_LEN())) {
    IGA_FATAL("RegDeps: GRF index is out of bounds");
  }
  bits->set(grf_addr, num_bytes);
}

void DepSet::addABytesAndBukets(size_t reg) {
  IGA_ASSERT(reg < m_DB.getARF_A_REGS(),
             "a# register out of bounds");
  size_t addr =  m_DB.getARF_A_START() + m_DB.getARF_A_BYTES_PER_REG() * reg;
  IGA_ASSERT(addr < (size_t)m_DB.getARF_A_START() + m_DB.getARF_A_LEN(),
             "a# byte address out of bounds");

  // ARF_A's swsb is tracked at per register base. sub-register number
  // doesn't matter.
  auto num_bytes = m_DB.getARF_A_BYTES_PER_REG();
  bits->set(addr, num_bytes);
  // set the corresponding buckets
  for (auto i = addr / m_DB.getBYTES_PER_BUCKET();
       i <= (addr + num_bytes - 1) / m_DB.getBYTES_PER_BUCKET(); i++) {
    addToBucket((uint32_t)i);
  }
}
void DepSet::addFBytes(size_t fByteOff, size_t num_bytes) {
  bits->set(fByteOff, num_bytes);
}

bool DepSet::destructiveSubtract(const DepSet &rhs) {
  return bits->andNot(*rhs.bits);
}

static void emitComma(std::ostream &os, bool &first) {
  if (first) {
    first = false;
  } else {
    os << ", ";
  }
}
void DepSet::formatShortReg(std::ostream &os, bool &first, const char *reg_name,
                            size_t reg_num, // ((size_t)-1) means we ignore
                            size_t reg_start, size_t reg_len) const {
  if (bits->testAll(reg_start, reg_len)) {
    // full register is set
    emitComma(os, first);
    os << reg_name;
    if (reg_num != (size_t)-1)
      os << reg_num;
  } else if (bits->testAny(reg_start, reg_len)) {
    // partial register
    for (size_t ri = 0; ri < reg_len; ri++) {
      // find starting bit
      if (!bits->test(reg_start + ri)) {
        continue;
      }
      // ri is first set
      emitComma(os, first);
      os << reg_name;
      if (reg_num != (size_t)-1)
        os << reg_num;
      os << "." << ri;

      // find the ending position
      size_t end_ri = ri;
      while (end_ri < reg_start + reg_len && bits->test(reg_start + end_ri))
        end_ri++;
      // e.g. r23.0-3 (a full DWORD)
      if (end_ri > ri + 1) // case of 1 byte omits range r0.0 (for :b)
        os << '-' << (end_ri - 1);
      // else: e.g. r23.0 (just one byte)
      ri = end_ri; // will move to the next element
    }              // for
  }                // else: nothing set in this register
}
// emits a shorter description... something like
// {r0,r1.0-3,r7}
void DepSet::str(std::ostream &os) const {
  bool first = true;
  os << "{";

  for (size_t ri = 0; ri < m_DB.getGRF_REGS(); ri++) {
    formatShortReg(os, first, "r", ri,
                   m_DB.getGRF_START() + ri * m_DB.getGRF_BYTES_PER_REG(),
                   m_DB.getGRF_BYTES_PER_REG());
  }

  for (uint32_t ai = 0; ai < m_DB.getARF_A_REGS(); ai++) {
    formatShortReg(os, first, "a", ai,
                   m_DB.getARF_A_START() +
                       (size_t)ai * m_DB.getARF_A_BYTES_PER_REG(),
                   m_DB.getARF_A_BYTES_PER_REG());
  }

  for (uint32_t acci = 0; acci < m_DB.getARF_ACC_REGS(); acci++) {
    formatShortReg(os, first, "acc", acci,
                   m_DB.getARF_ACC_START() +
                       (size_t)acci * m_DB.getARF_ACC_BYTES_PER_REG(),
                   m_DB.getARF_ACC_BYTES_PER_REG());
  }

  for (uint32_t fi = 0; fi < m_DB.getARF_F_REGS(); fi++) {
    formatShortReg(os, first, "f", fi,
                   m_DB.getARF_F_START() +
                       (size_t)fi * m_DB.getARF_F_BYTES_PER_REG(),
                   m_DB.getARF_F_BYTES_PER_REG());
  }

  for (uint32_t fi = 0; fi < m_DB.getARF_SCALAR_REGS(); fi++) {
    formatShortReg(os, first, "s", fi,
                   m_DB.getARF_SCALAR_START() +
                       (size_t)fi * m_DB.getARF_SCALAR_BYTES_PER_REG(),
                   m_DB.getARF_SCALAR_BYTES_PER_REG());
  }

  os << "}";
}
std::string DepSet::str() const {
  std::stringstream ss;
  str(ss);
  return ss.str();
}
