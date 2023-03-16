/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "LatencyTable.h"
#include "../G4_IR.hpp"
#include "LocalScheduler_G4IR.h"
#include <type_traits>

using namespace vISA;

// c++23 std::to_underlying.
template <typename Enum>
inline std::underlying_type_t<Enum> value_of(Enum val) {
  return static_cast<std::underlying_type_t<Enum>>(val);
}

class LatencyTableLegacy : public LatencyTable {
public:
  LatencyTableLegacy(const IR_Builder& builder) : LatencyTable(builder) {
    vASSERT(builder.getPlatformGeneration() < PlatformGen::XE);
  }
  uint16_t getLatency(const G4_INST *Inst) const override;
  uint16_t getOccupancy(const G4_INST *Inst) const override;
  uint16_t getDPASLatency(uint8_t repeatCount) const override;
};

template <PlatformGen Gen>
class LatencyTableXe: public LatencyTable {
  // Select latency information based on platform generation.
  using LI = typename std::conditional<Gen >= PlatformGen::XE,
                                       XELatencyInfo,
                                       void>::type;
public:
  LatencyTableXe(const IR_Builder& builder) : LatencyTable(builder) {
    static_assert(Gen >= PlatformGen::XE);
  }
  // General implementations to get latency and occupancy for the given
  // instruction based on heuristics. The implementation can be specialized if
  // needed.
  uint16_t getLatency(const G4_INST *Inst) const override;
  uint16_t getOccupancy(const G4_INST *Inst) const override;

  // The details of heuristics used to calculate the latency. The
  // implementation can be specialized if needed.
  uint16_t getDPASLatency(uint8_t repeatCount) const override;
private:
  uint16_t getMsgLatency(const G4_INST *Inst) const;
  uint16_t getMathLatency(const G4_INST *inst) const;
  uint16_t getBranchLatency(const G4_INST *inst) const;
  uint16_t getIntrinsicLatency(const G4_INST *inst) const;
  uint16_t getDPASLatency(const G4_InstDpas *dpas) const;
  uint16_t getARFAccessLatency(const G4_INST *inst) const;
  uint16_t getArithmeticLatency(const G4_INST *inst) const;
};

std::unique_ptr<LatencyTable>
LatencyTable::createLatencyTable(const IR_Builder &builder) {
  auto GEN = builder.getPlatformGeneration();
  if (GEN >= PlatformGen::XE)
    return std::make_unique<LatencyTableXe<PlatformGen::XE>>(builder);

  return std::make_unique<LatencyTableLegacy>(builder);
}

uint16_t LatencyTableLegacy::getLatency(const G4_INST *Inst) const {
  if (Inst->isSend()) {
    G4_SendDesc *MsgDesc = Inst->getMsgDesc();
    return LegacyFFLatency[SFIDtoInt(MsgDesc->getSFID())];
  } else if (Inst->isMath()) {
    if (Inst->asMathInst()->getMathCtrl() == MATH_FDIV ||
        Inst->asMathInst()->getMathCtrl() == MATH_POW)
      return LegacyLatencies::EDGE_LATENCY_MATH_TYPE2;
    return LegacyLatencies::EDGE_LATENCY_MATH;
  }
  return LegacyLatencies::IVB_PIPELINE_LENGTH;
}

// This calculates the node's pipeline occupancy (node delay)
uint16_t LatencyTableLegacy::getOccupancy(const G4_INST *Inst) const {
  int divisor = 8;
  int InstLatency = LegacyLatencies::UNCOMPR_LATENCY;
  if (Inst->isFastHFInstruction()) {
    divisor = 16;
  }

  // Number of n-wide passes in FPU0 or FPU1 (EM).
  // "n" is:
  //      16 for BDW+ HalfFloatDoublePerf instructions,
  //      8 for other instructions.
  int passes = std::max(1, Inst->getExecSize() / divisor);

  // InstLatency is:
  //      4 for EM/FPU1 POW and FDIV instructions ( HSW; for BDW+ it is 2 times
  //      higher ), 2 for other EM/FPU1 instructions ( HSW; for BDW+ it is 2
  //      times higher ), 2 for other instructions.
  // Update DagNode latency for math.
  G4_opcode opCode = Inst->opcode();
  switch (opCode) {
  case G4_math: {
    // Use EdgeLatencyMathType2 for FDIV, FPOW functions.
    if (Inst->asMathInst()->getMathCtrl() == MATH_FDIV ||
        Inst->asMathInst()->getMathCtrl() == MATH_POW) {
      InstLatency = 4;
    } else {
      // Used EdgeLatencyMath for other functions.
      InstLatency = 2;
    }

    // BDW+ platforms have lower math TPT and longer latency (all math
    // functions).
    InstLatency *= 2;
    break;
  }
  case G4_bfe:
  case G4_bfi1:
  case G4_bfi2:
  case G4_bfrev:
  case G4_cbit:
  case G4_dp2:
  case G4_dp3:
  case G4_dp4:
  case G4_dph:
  case G4_fbh:
  case G4_fbl:
  case G4_lrp:
  case G4_mac:
  case G4_mach:
  case G4_pln:
    InstLatency *= 2;
    break;
  case G4_label:
    // Labels need special care. They should have a latency of 1.
    // But their execSize is 255, which sets passes=31.
    passes = 1;
    InstLatency = 1;
    break;
  default:
    break;
  }

  return uint16_t(passes * InstLatency);
}

uint16_t LatencyTableLegacy::getDPASLatency(uint8_t repeatCount) const {
  vISA_ASSERT_UNREACHABLE("DPAS is not supported");
  return LegacyLatencies::UNKNOWN_LATENCY;
}

// General template implementations for XE+.
template<PlatformGen Gen>
uint16_t LatencyTableXe<Gen>::getLatency(const G4_INST *Inst) const {
  if (Inst->isSend())
    return getMsgLatency(Inst);
  if (Inst->isMath())
    return getMathLatency(Inst);
  if (Inst->isFlowControl())
    return getBranchLatency(Inst);
  if (Inst->isIntrinsic())
    return getIntrinsicLatency(Inst);
  if (Inst->isDpas())
    return getDPASLatency(Inst->asDpasInst());
  if (Inst->writesFlag() ||
      (Inst->getDst() && Inst->getDst()->isDirectA0()))
    return getARFAccessLatency(Inst);
  if (Inst->isArithmetic())
    return getArithmeticLatency(Inst);

  // By default, use the FPU pipeline latency.
  return value_of(LI::FPU);
}

template<PlatformGen Gen>
uint16_t LatencyTableXe<Gen>::getMsgLatency(const G4_INST *Inst) const {
  vASSERT(Inst->isSend());
  G4_SendDesc *MsgDesc = Inst->getMsgDesc();
  if (MsgDesc->isLSC()) {
    if (MsgDesc->getSFID() == SFID::SLM) {
      auto Sz = Inst->getExecSize();
      return MsgDesc->isFence()
                 ? value_of(LI::SLM_FENCE)
                 : ((Sz > g4::SIMD16) ? value_of(LI::SLM32)
                                      : value_of(LI::SLM16));
    } else if (MsgDesc->isFence()) {
      return MsgDesc->isTyped() ? value_of(LI::LSC_TYPED_FENCE)
                                : value_of(LI::LSC_UNTYPED_FENCE);
    } else {
      bool isCachedInL1 = MsgDesc->getCachingL1() == Caching::CA ||
                          (MsgDesc->getCachingL1() != Caching::UC &&
                           m_builder.getOption(vISA_assumeL1Hit));
      if (MsgDesc->isTyped()) {
        return isCachedInL1 ? value_of(LI::LSC_TYPED_L1)
                            : value_of(LI::LSC_TYPED_L3);
      } else {
        return isCachedInL1 ? value_of(LI::LSC_UNTYPED_L1)
                            : value_of(LI::LSC_UNTYPED_L3);
      }
    }
  }
  if (MsgDesc->isSLM())
    return Inst->asSendInst()->isFence() ? value_of(LI::SLM_FENCE)
                                         : value_of(LI::SLM16);
  if (MsgDesc->isSampler())
    return value_of(LI::SAMPLER_L3);
  if (MsgDesc->isHDC())
    return value_of(LI::DP_L3);
  if (MsgDesc->isBarrier())
    return value_of(LI::BARRIER);
  return value_of(LI::SEND_OTHERS);
}

template<PlatformGen Gen>
uint16_t LatencyTableXe<Gen>::getMathLatency(const G4_INST *Inst) const {
  vASSERT(Inst->isMath());
  return value_of(LI::MATH);
}

template<PlatformGen Gen>
uint16_t LatencyTableXe<Gen>::getBranchLatency(const G4_INST *Inst) const {
  vASSERT(Inst->isFlowControl());
  return value_of(LI::BRANCH);
}

template<PlatformGen Gen>
uint16_t LatencyTableXe<Gen>::getIntrinsicLatency(const G4_INST *Inst) const {
  vASSERT(Inst->isIntrinsic());
  return value_of(LI::FPU);
}

template<PlatformGen Gen>
uint16_t LatencyTableXe<Gen>::getDPASLatency(const G4_InstDpas *dpas) const {
  return getDPASLatency(dpas->getRepeatCount());
}

template<PlatformGen Gen>
uint16_t LatencyTableXe<Gen>::getARFAccessLatency(const G4_INST *Inst) const {
  vASSERT(Inst->writesFlag() ||
          (Inst->getDst() && Inst->getDst()->isDirectA0()));
  return value_of(LI::ARF);
}

template<PlatformGen Gen>
uint16_t LatencyTableXe<Gen>::getArithmeticLatency(const G4_INST *Inst) const {
  vASSERT(Inst->isArithmetic());
  auto Dst = Inst->getDst();
  if (Dst && Dst->isAccReg())
    return value_of(LI::FPU_ACC);
  return value_of(LI::FPU);
}

template <PlatformGen Gen>
uint16_t LatencyTableXe<Gen>::getOccupancy(const G4_INST *Inst) const {
  auto Sz = Inst->getExecSize();
  auto NativeSz = m_builder.getNativeExecSize();
  uint16_t Scale = (Sz <= NativeSz) ? 1 : (Sz == NativeSz * 2) ? 2 : 4;
  if (Inst->isMath())
    return value_of(LI::OC_MATH) * Scale;
  if (Inst->isFastHFInstruction())
    Scale = (Sz <= NativeSz * 2) ? 1 : 2;
  else if (G4_DstRegRegion *Dst = Inst->getDst()) {
    if (Dst->getTypeSize() == 8)
      Scale = (Sz <= NativeSz / 2) ? 1 : 2;
  }
  return value_of(LI::OC_OTHERS) * Scale;
}

// XE Specializations.
template <>
uint16_t
LatencyTableXe<PlatformGen::XE>::getDPASLatency(uint8_t repeatCount) const {
  switch (m_builder.getPlatform()) {
  case Xe_XeHPSDV:
    return value_of(LI::DPAS) + repeatCount - 1;
  case Xe_DG2:
    switch (repeatCount) {
    case 1:
      return 21;
    case 2:
      return 22;
    case 8:
      return 32;
    default:
      return 32;
    }
  case Xe_PVC:
    return value_of(LI::DPAS) + repeatCount - 1;
  case Xe_PVCXT:
    return value_of(LI::DPAS) + repeatCount;
  default: // Not supported platform
    // TODO: Add vISA_ASSERT_UNREACHABLE.
    return 46;
  }
}
template<>
uint16_t
LatencyTableXe<PlatformGen::XE>::getMathLatency(const G4_INST *Inst) const {
  vASSERT(Inst->isMath());
  int Sz = Inst->getExecSize();
  int Scale = Scale = (Sz <= 8) ? 0 : (Sz == 16) ? 1 : 3;
  return value_of(LI::MATH) + value_of(LI::DELTA_MATH) * Scale;
}

template<>
uint16_t LatencyTableXe<PlatformGen::XE>::getDPASLatency(
    const G4_InstDpas *dpas) const {
  return getDPASLatency(dpas->getRepeatCount());
}

template<>
uint16_t LatencyTableXe<PlatformGen::XE>::getArithmeticLatency(
    const G4_INST *Inst) const {
  vASSERT(Inst->isArithmetic());
  int Sz = Inst->getExecSize();
  int Scale = Scale = (Sz <= 8) ? 0 : (Sz == 16) ? 1 : 3;
  auto Delta = value_of(LI::DELTA) * Scale;
  auto Dst = Inst->getDst();
  if (Dst && Dst->isAccReg())
    return value_of(LI::FPU_ACC) + Delta;
  return value_of(LI::FPU) + Delta;
}

// TODO: Update PVC+ to consider native exec size as well so that the
// specialization can be removed.
template <>
uint16_t
LatencyTableXe<PlatformGen::XE>::getOccupancy(const G4_INST *Inst) const {
  int Sz = Inst->getExecSize();
  int Scale = (Sz <= 8) ? 1 : (Sz == 16) ? 2 : 4;
  if (Inst->isMath())
    return value_of(LI::OC_MATH) * Scale;
  if (Inst->isFastHFInstruction())
    Scale = (Sz <= 16) ? 1 : 2;
  else if (G4_DstRegRegion *Dst = Inst->getDst()) {
    if (Dst->getTypeSize() == 8)
      Scale = (Sz <= 4) ? 1 : 2;
  }
  return value_of(LI::OC_OTHERS) * Scale;
}
