/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Common_BinaryEncoding.h"
#include "BuildIR.h"

using namespace vISA;

unsigned long bitsSrcRegFile[4] = {128, 128, 128, 128};
unsigned long bits3SrcFlagRegNum[2] = {128, 128};
unsigned long bitsFlagRegNum[2] = {128, 128};

/// \brief writes the binary buffer to .dat file
///
BinaryEncodingBase::Status BinaryEncodingBase::WriteToDatFile() {
  std::string binFileName = fileName + ".dat";
  std::string errStr;
  std::ofstream os(binFileName.c_str(), std::ios::binary);
  if (!os) {
    errStr = "Can't open " + binFileName + ".\n";
    vISA_ASSERT(false, errStr);
    return FAILURE;
  }

  for (unsigned i = 0, size = (unsigned)binInstList.size(); i < size; i++) {
    BinInst *bin = binInstList[i];

    if (GetCompactCtrl(bin)) {
      os.write(reinterpret_cast<char *>(&(bin->Bytes)), BYTES_PER_INST / 2);
    } else {
      os.write(reinterpret_cast<char *>(&(bin->Bytes)), BYTES_PER_INST);
    }
  };
  os.close();

  return SUCCESS;
}

bool BinaryEncodingBase::isBBBinInstEmpty(G4_BB *bb) {
  INST_LIST_ITER ii, iend(bb->end());
  for (ii = bb->begin(); ii != iend; ++ii) {
    G4_INST *inst = *ii;
    if (getBinInst(inst))
      return false;
  }
  return true;
}

G4_INST *BinaryEncodingBase::getFirstNonLabelInst(G4_BB *bb) {
  INST_LIST_ITER ii, iend(bb->end());
  for (ii = bb->begin(); ii != iend; ++ii) {
    G4_INST *inst = *ii;
    if (inst->opcode() != G4_label)
      return inst;
  }
  vISA_ASSERT_UNREACHABLE("can't get the inst number for this empty BB");
  return NULL;
}

void BinaryEncodingBase::ProduceBinaryBuf(void *&handle) {
  uint32_t binarySize = GetInstCounts() * (BYTES_PER_INST / 2);
  handle = allocCodeBlock(binarySize);
  char *buf = (char *)handle;
  if (handle == NULL) {
    vISA_ASSERT(false, "mem manager alloc failure in bin encoding");
  } else {
    for (unsigned i = 0, size = (unsigned)binInstList.size(); i < size; i++) {
      BinInst *bin = binInstList[i];
      char *ptr = (char *)(bin->Bytes);
      if (GetCompactCtrl(bin)) {
        memcpy_s(buf, binarySize, ptr, BYTES_PER_INST / 2);
        buf += BYTES_PER_INST / 2;
      } else {
        memcpy_s(buf, binarySize, ptr, BYTES_PER_INST);
        buf += BYTES_PER_INST;
      }
    }
  }
}

// 3-src instructions (mad, lrp, bfe, bf2) must be in align16 mode for gen9 and
// earlier; this implies that all operands must be 16-byte aligned and exec size
// must be >=4 We convert a simd1 3-src instruction into simd4 (or simd2 for DF
// mad) and control the dst channel through the dst write mask. For DF mad, we
// also have to fix the source swizzle as .r is not supported for 64-bit types.
// This applies regardless of exec size.
// Additionally, invm and sqrtm math instructions must also be align16
void BinaryEncodingBase::FixAlign16Inst(G4_INST *inst) {
  inst->setOptionOn(InstOpt_Align16);

  // convert dst to align16
  G4_DstRegRegion *dst = inst->getDst();
  setWriteMask(dst, ChannelEnable_XYZW);

  // convert sources to align16
  for (int k = 0, numSrc = inst->getNumSrc(); k < numSrc; k++) {
    vISA_ASSERT(inst->getSrc(k)->isSrcRegRegion(),
                "Unexpected src to be converted to ALIGN16!");
    G4_SrcRegRegion *src = inst->getSrc(k)->asSrcRegRegion();
    setSwizzle(src, src->isScalar() ? SrcSwizzle::R : SrcSwizzle::XYZW);
    if (inst->opcode() == G4_math &&
        (inst->asMathInst()->getMathCtrl() == MATH_INVM ||
         inst->asMathInst()->getMathCtrl() == MATH_RSQRTM)) {
      switch (inst->getSrc(k)->getType()) {
      case Type_DF:
        src->setRegion(*kernel.fg.builder,
                       kernel.fg.builder->createRegionDesc(2, 2, 1));
        break;
      case Type_F:
      case Type_HF:
        src->setRegion(*kernel.fg.builder,
                       kernel.fg.builder->createRegionDesc(4, 4, 1));
        break;
      default:
        vISA_ASSERT_UNREACHABLE("Not implemented");
      }
    }
  }

  bool isDoubleInst = (dst->getType() == Type_DF);
  if (inst->getExecSize() == g4::SIMD1) {
    int subRegOffset = dst->getLinearizedStart() % 16;
    if (inst->getCondMod()) {
      G4_CondModifier mod = inst->getCondMod()->getMod();
      if (subRegOffset != 0 &&
          (mod == Mod_g || mod == Mod_ge || mod == Mod_l || mod == Mod_le)) {
          vISA_ASSERT(
              false, "Invalid alignment for align16 inst of execsize 1 and offset %d", (short)subRegOffset);
      }
    }
    ChannelEnable writeMask = NoChannelEnable;
    switch (subRegOffset / 4) {
    case 0:
      writeMask = isDoubleInst ? ChannelEnable_XY : ChannelEnable_X;
      break;
    case 1:
      writeMask = ChannelEnable_Y;
      break;
    case 2:
      writeMask = isDoubleInst ? ChannelEnable_ZW : ChannelEnable_Z;
      break;
    case 3:
      writeMask = ChannelEnable_W;
      break;
    default:
      vISA_ASSERT_UNREACHABLE("unexpected subreg value");
    }
    setWriteMask(dst, writeMask);
    // FIXME: this is incorrect as it could produce a negative left bound if
    // subRegOffset is non-zero. HWConformity appears to force 16-byte alignment
    // on dst, however, so we don't ever hit such case.
    dst->setLeftBound(dst->getLeftBound() - subRegOffset);
    dst->setRightBound(dst->getLeftBound() + 16);
    inst->setExecSize(isDoubleInst ? g4::SIMD2 : g4::SIMD4);
    G4_Predicate *pred = inst->getPredicate();
    if (pred) {
      setAlign16PredCtrl(pred, PRED_ALIGN16_X);
    }
  } else if (inst->getExecSize() == g4::SIMD2 && !isDoubleInst) {
    int subRegOffset = dst->getLinearizedStart() % 16;
    ChannelEnable writeMask = NoChannelEnable;
    switch (subRegOffset / 4) {
    case 0:
      writeMask = ChannelEnable_XY;
      break;
    case 2:
      writeMask = ChannelEnable_ZW;
      break;
    default:
      vISA_ASSERT_UNREACHABLE("dst must be 8 byte aligned");
    }
    setWriteMask(dst, writeMask);
    dst->setLeftBound(dst->getLeftBound() - subRegOffset);
    dst->setRightBound(dst->getLeftBound() + 16);
    inst->setExecSize(g4::SIMD4);
    vISA_ASSERT(!inst->getPredicate(), "do not support predicated SIMD2 mad");
  }

  // for double/half inst, we have to additionally fix the source as it doesn't
  // support the .r swizzle
  if (isDoubleInst) {
    for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
      vISA_ASSERT(inst->getSrc(i)->isSrcRegRegion(),
                  "source must have a region");
      G4_SrcRegRegion *src = inst->getSrc(i)->asSrcRegRegion();
      const RegionDesc *rd = src->getRegion();
      if (src->isScalar() ||
          (rd->width == 2 && rd->horzStride == 0 && rd->vertStride == 2)) {
        int subRegOffset = src->getLinearizedStart() % 16;
        vISA_ASSERT(subRegOffset == 0 || subRegOffset == 8,
                    "double source must be 8 byte aligned");
        setSwizzle(src,
                   subRegOffset == 0 ? SrcSwizzle::XYXY : SrcSwizzle::ZWZW);
        // If subRegOffset is not 16-byte aligned, it will be fixed when
        // when encoding the source reg num.
      }
    }
  }
}

void BinaryEncodingBase::FixMathInst(G4_INST *inst) {
  vISA_ASSERT(inst->isMath(), "Expect math instruction");
  for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i) {
    G4_Operand *src = inst->getSrc(i);
    if (src && src->isSrcRegRegion()) {
      G4_SrcRegRegion *srcRegion = src->asSrcRegRegion();
      const RegionDesc *region = srcRegion->getRegion();
      if (inst->getExecSize() > g4::SIMD1 && region->vertStride == 1 &&
          region->width == 1 && region->horzStride == 0) {
        // rewrite <1;1,0> to <2;2,1> to avoid simulator warning
        srcRegion->setRegion(*kernel.fg.builder,
                             kernel.fg.builder->createRegionDesc(2, 2, 1));
      }
    }
  }
}

// We also fix <1;1,0> src region for align1 ternary instructions as we can't
// encode them in binary
void BinaryEncodingBase::FixInst() {
  bool align1Ternary = kernel.fg.builder->hasAlign1Ternary();
  for (auto bb : kernel.fg) {
    for (auto iter = bb->begin(); iter != bb->end();) {
      G4_INST *inst = *iter;
      if (inst->isIntrinsic()) {
        // remove any intrinsics that should be lowered before binary encoding
        vISA_ASSERT(inst->asIntrinsicInst()->getLoweredByPhase() ==
                         Phase::BinaryEncoding,
                     "Unexpected intrinsics in binary encoding");
        iter = bb->erase(iter);
      } else {
        ++iter;
      }

      bool isAlign16 = kernel.fg.builder->hasIEEEDivSqrt() &&
                       (inst->opcode() == G4_madm ||
                        (inst->isMath() && inst->asMathInst()->isIEEEMath()));

      if (!isAlign16) {
        isAlign16 =
            (!align1Ternary) && (inst->getNumSrc() == 3) && !inst->isSend();
      }

      if (isAlign16) {
        FixAlign16Inst(inst);
      } else if (inst->isMath()) {
        FixMathInst(inst);
      }
    }
  }
}

void *BinaryEncodingBase::EmitBinary(uint32_t &binarySize) {
  void *handle = NULL;
  // CommitRelativeAddresses();
  binarySize = GetInstCounts() * (BYTES_PER_INST / 2);

  /*
      Simplifying this. Whatever invokes vISA builder
      should know whether to generate binary or not.
      Through dll mode, this shouldn't be set.
  */
  if (kernel.getOption(vISA_GenerateBinary)) {
    WriteToDatFile();
  }

  ProduceBinaryBuf(handle);
  return handle;
}

void BinaryEncodingBase::computeBinaryOffsets() {
  // Compute offset for gen instructions
  uint64_t offset = 0;
  for (auto I = binInstList.begin(), E = binInstList.end(); I != E; ++I) {
    BinInst *binInst = *I;
    std::streamsize size =
        GetCompactCtrl(binInst) ? (BYTES_PER_INST / 2) : BYTES_PER_INST;
    binInst->SetGenOffset(offset);
    offset += size;
  }
}

bool BinaryEncodingBase::doCompaction() const {
  return kernel.getOption(vISA_Compaction);
}
