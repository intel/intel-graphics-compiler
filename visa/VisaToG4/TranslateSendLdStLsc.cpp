/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../Timer.h"
#include "Assertions.h"
#include "BuildIR.h"
#include <cmath>

using namespace vISA;

static MsgOp ConvertLSCOpToMsgOp(LSC_OP op) {
  switch (op) {
  case LSC_OP::LSC_LOAD:
    return MsgOp::LOAD;
  case LSC_OP::LSC_LOAD_STRIDED:
    return MsgOp::LOAD_STRIDED;
  case LSC_OP::LSC_LOAD_QUAD:
    return MsgOp::LOAD_QUAD;
  case LSC_OP::LSC_LOAD_BLOCK2D:
    return MsgOp::LOAD_BLOCK2D;
  case LSC_OP::LSC_LOAD_STATUS:
    return MsgOp::LOAD_STATUS;
  case LSC_OP::LSC_STORE:
    return MsgOp::STORE;
  case LSC_OP::LSC_STORE_STRIDED:
    return MsgOp::STORE_STRIDED;
  case LSC_OP::LSC_STORE_QUAD:
    return MsgOp::STORE_QUAD;
  case LSC_OP::LSC_STORE_BLOCK2D:
    return MsgOp::STORE_BLOCK2D;
  //
  case LSC_OP::LSC_ATOMIC_IINC:
    return MsgOp::ATOMIC_IINC;
  case LSC_OP::LSC_ATOMIC_IDEC:
    return MsgOp::ATOMIC_IDEC;
  case LSC_OP::LSC_ATOMIC_LOAD:
    return MsgOp::ATOMIC_LOAD;
  case LSC_OP::LSC_ATOMIC_STORE:
    return MsgOp::ATOMIC_STORE;
  //
  case LSC_OP::LSC_ATOMIC_IADD:
    return MsgOp::ATOMIC_IADD;
  case LSC_OP::LSC_ATOMIC_ISUB:
    return MsgOp::ATOMIC_ISUB;
  //
  case LSC_OP::LSC_ATOMIC_SMIN:
    return MsgOp::ATOMIC_SMIN;
  case LSC_OP::LSC_ATOMIC_SMAX:
    return MsgOp::ATOMIC_SMAX;
  case LSC_OP::LSC_ATOMIC_UMIN:
    return MsgOp::ATOMIC_UMIN;
  case LSC_OP::LSC_ATOMIC_UMAX:
    return MsgOp::ATOMIC_UMAX;
  //
  case LSC_OP::LSC_ATOMIC_ICAS:
    return MsgOp::ATOMIC_ICAS;
  case LSC_OP::LSC_ATOMIC_FADD:
    return MsgOp::ATOMIC_FADD;
  case LSC_OP::LSC_ATOMIC_FSUB:
    return MsgOp::ATOMIC_FSUB;
  case LSC_OP::LSC_ATOMIC_FMIN:
    return MsgOp::ATOMIC_FMIN;
  case LSC_OP::LSC_ATOMIC_FMAX:
    return MsgOp::ATOMIC_FMAX;
  case LSC_OP::LSC_ATOMIC_FCAS:
    return MsgOp::ATOMIC_FCAS;
  //
  case LSC_OP::LSC_ATOMIC_AND:
    return MsgOp::ATOMIC_AND;
  case LSC_OP::LSC_ATOMIC_XOR:
    return MsgOp::ATOMIC_XOR;
  case LSC_OP::LSC_ATOMIC_OR:
    return MsgOp::ATOMIC_OR;
  case LSC_OP::LSC_FENCE:
    return MsgOp::FENCE;
  default:
    return MsgOp::INVALID;
  }
}

static DataSize ConvertLSCDataSize(LSC_DATA_SIZE ds) {
  switch (ds) {
  case LSC_DATA_SIZE_8b:
    return DataSize::D8;
  case LSC_DATA_SIZE_16b:
    return DataSize::D16;
  case LSC_DATA_SIZE_32b:
    return DataSize::D32;
  case LSC_DATA_SIZE_64b:
    return DataSize::D64;
  case LSC_DATA_SIZE_8c32b:
    return DataSize::D8U32;
  case LSC_DATA_SIZE_16c32b:
    return DataSize::D16U32;
  default:
    vISA_ASSERT_UNREACHABLE("invalid data size");
  }
  return DataSize::INVALID;
}

static DataOrder ConvertLSCDataOrder(LSC_DATA_ORDER dord, bool vnni = false) {
  switch (dord) {
  case LSC_DATA_ORDER_NONTRANSPOSE:
    return vnni ? DataOrder::VNNI : DataOrder::NONTRANSPOSE;
  case LSC_DATA_ORDER_TRANSPOSE:
    return vnni ? DataOrder::TRANSPOSE_VNNI : DataOrder::TRANSPOSE;
  default:
    vISA_ASSERT_UNREACHABLE("invalid data order");
  }
  return DataOrder::INVALID;
}


G4_ExecSize IR_Builder::lscMinExecSize(LSC_SFID lscSfid) const {
  const TARGET_PLATFORM P = getPlatform();
  uint32_t minExecSize = ((P == Xe_DG2 || P == Xe_MTL) ? 8 : 16);
  if (!hasLSCEnableHalfSIMD())
  {
    minExecSize *= 2;
  }
  return G4_ExecSize(minExecSize);
}

static VecElems ConvertLSCDataElems(LSC_DATA_ELEMS de) {
  switch (de) {
  case LSC_DATA_ELEMS_1:
    return VecElems::V1;
  case LSC_DATA_ELEMS_2:
    return VecElems::V2;
  case LSC_DATA_ELEMS_3:
    return VecElems::V3;
  case LSC_DATA_ELEMS_4:
    return VecElems::V4;
  case LSC_DATA_ELEMS_8:
    return VecElems::V8;
  case LSC_DATA_ELEMS_16:
    return VecElems::V16;
  case LSC_DATA_ELEMS_32:
    return VecElems::V32;
  case LSC_DATA_ELEMS_64:
    return VecElems::V64;
  default:
    vISA_ASSERT_UNREACHABLE("number of data elements");
  }
  return VecElems::INVALID;
}


static Caching ConvertLSCCacheOpt(LSC_CACHE_OPT co) {
  switch (co) {
  case LSC_CACHING_DEFAULT:
    return Caching::DF;
  case LSC_CACHING_UNCACHED:
    return Caching::UC;
  case LSC_CACHING_CACHED:
    return Caching::CA;
  case LSC_CACHING_WRITEBACK:
    return Caching::WB;
  case LSC_CACHING_WRITETHROUGH:
    return Caching::WT;
  case LSC_CACHING_STREAMING:
    return Caching::ST;
  case LSC_CACHING_READINVALIDATE:
    return Caching::RI;
  default:
    vISA_ASSERT_UNREACHABLE("invalid caching");
  }
  return Caching::INVALID;
}
static std::pair<Caching, Caching> ConvertLSCCacheOpts(LSC_CACHE_OPT col1,
                                                       LSC_CACHE_OPT col3) {
  return std::make_pair(ConvertLSCCacheOpt(col1), ConvertLSCCacheOpt(col3));
}

static G4_Operand *lscTryPromoteSurfaceImmToExDesc(G4_Operand *surface,
                                                   LSC_ADDR_TYPE addrModel,
                                                   uint32_t &exDesc) {
  if (surface && surface->isImm()) {
    // try and promote any immediate surface to the extended descriptor if
    // possible; we get [31:12] in the EU ISA to work with.
    auto surfaceImm = (uint32_t)surface->asImm()->getImm();
    if (addrModel == LSC_ADDR_TYPE_BTI) {
      // promote the immediate BTI to the descriptor
      exDesc |= surfaceImm << 24;
      surface = nullptr;
    } else if (addrModel == LSC_ADDR_TYPE_BSS ||
               addrModel == LSC_ADDR_TYPE_SS) {
      if ((surfaceImm & 0x3FF) == 0) {
        exDesc |= surfaceImm;
        surface = nullptr;
      }
    } else if (addrModel == LSC_ADDR_TYPE_ARG) {
      vISA_ASSERT_INPUT(false, "incorrect address model, caller should have fixed this");
      exDesc |= 0xFF << 24;
      surface = nullptr;
    } else {
      // flat address type
      vISA_ASSERT_INPUT(
          surface->isNullReg() || surfaceImm == PREDEFINED_SURFACE_SLM ||
              surfaceImm == PREDEFINED_SURFACE_T255, // not sure what's up here
          "flat address type must have null reg (or 0)");
      surface = nullptr;
    }
  } else {
    vISA_ASSERT_INPUT(surface || addrModel == LSC_ADDR_TYPE_FLAT,
                 "only flat address model may have null surface");
  }
  return surface;
}

static bool isNullOperand(const G4_Operand *opnd) {
  return opnd == nullptr || opnd->isNullReg();
}

static int alignUp(int a, int n) { return n + a - 1 - ((n + a - 1) % a); }

static int lscBlock2dComputeDataRegs(LSC_OP op,
                                     LSC_DATA_SHAPE_BLOCK2D dataShape2d,
                                     int BYTES_PER_REG, int dataSizeBits) {
  auto roundUpToPowerOf2 = [](int n) {
    while (n & (n - 1))
      n++;
    return n;
  };

  bool transpose = dataShape2d.order == LSC_DATA_ORDER_TRANSPOSE;
  int grfRowPitchElems =
      roundUpToPowerOf2(!transpose ? dataShape2d.width : dataShape2d.height);
  int blockRows = !transpose ? dataShape2d.height : dataShape2d.width;
  int elemsPerGrf = 8 * BYTES_PER_REG / dataSizeBits;
  // alignUp needed for padding between blocks; each block pads out to
  // a full GRF
  int regsPerBlock =
      alignUp(elemsPerGrf, blockRows * grfRowPitchElems) / elemsPerGrf;
  //
  int dataRegs = dataShape2d.blocks * regsPerBlock;
  // C.f. DP_LOAD_2DBLOCK_ARRAY
  //
  //   Data payload size, in registers. Destination length of 32 is
  //   encoded as 31.  Data port hardware derives the correct destination
  //   length based on message parameters.
  if (op == LSC_LOAD_BLOCK2D && dataRegs == 32)
    dataRegs = 31;
  return dataRegs;
}

int IR_Builder::translateLscUntypedInst(
    LSC_OP op, LSC_SFID lscSfid, G4_Predicate *pred,
    VISA_Exec_Size visaExecSize, VISA_EMask_Ctrl execCtrl,
    LSC_CACHE_OPTS cacheOpts, LSC_ADDR addrInfo, LSC_DATA_SHAPE dataShape,
    G4_Operand *surface,        // can be G4_Imm or G4_SrcRegRegion
    G4_DstRegRegion *dstRead,   // dst can be NULL reg (e.g store)
    G4_SrcRegRegion *src0Addr,  // always the addresses (base for strided)
    G4_Operand *src0AddrStride, // only for strided
    G4_SrcRegRegion *src1Data,  // store data/extra atomic operands
    G4_SrcRegRegion *src2Data   // store data/extra atomic operands
) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  int status = VISA_SUCCESS;
  auto check = [&](bool z, const char *what) {
    if (!z) {
      vISA_ASSERT_INPUT(false, std::string(what));
      status = VISA_FAILURE;
    }
  };

  // This enforces a64 payloads to have .decl type Q/UQ
  // and a32* to have D/UD.
  // Later changes will attempt to uncomment this and enforce the change.
  //
  vISA_ASSERT_INPUT(addrInfo.size != LSC_ADDR_SIZE_64b ||
                    IS_QTYPE(src0Addr->getType()),
                    ":a64 expects Q/UQ");
  vISA_ASSERT_INPUT(addrInfo.size != LSC_ADDR_SIZE_32b ||
                    IS_DTYPE(src0Addr->getType()), ":a32* expects D/UD");

  const G4_ExecSize execSize = toExecSize(visaExecSize);
  const G4_InstOpts instOpt = Get_Gen4_Emask(execCtrl, execSize);

  const uint32_t BYTES_PER_REG = getGRFSize();

  if (addrInfo.type == LSC_ADDR_TYPE_ARG) {
    // Translate argument loads to platform specific logic
    // loads via base bti[255][r0[31:6]]
    // (BTI 255 with :a32 is relative to the General State Base Address)
    vISA_ASSERT_INPUT(addrInfo.size == LSC_ADDR_SIZE_32b,
                      "lsc_load... arg[...] must be :a32");
    //
    // (W) and (1)        TMP0:ud   r0.0:ud  0xFFFFFFC0:ud
    // (W) add (ExecSize) TMP1:ud   TMP0:ud  src0Addr:ud
    // ... load.ugm.a32... (ExecSize) bti[255][TMP1]
    G4_Declare *argBase = createTempVar(1, Type_UD, Even_Word);
    auto andDst = createDst(argBase->getRegVar(), 0, 0, 1, Type_UD);
    auto andSrc0 = createSrc(getBuiltinR0()->getRegVar(), 0, 0,
                             getRegionScalar(), Type_UD);
    (void)createBinOp(G4_and, g4::SIMD1, andDst, andSrc0,
                      createImm(0xFFFFFFC0, Type_UD),
                      InstOpt_WriteEnable, true);
    auto tmpAddrs = createTempVar(int(execSize), Type_UD, getGRFAlign());
    auto addDst = createDst(tmpAddrs->getRegVar(), 0, 0, 1, Type_UD);
    auto addSrc0 =
      createSrc(argBase->getRegVar(), 0, 0, getRegionScalar(), Type_UD);
    (void)createBinOp(G4_add, execSize, addDst, addSrc0, src0Addr,
                      instOpt, true);
    //
    addrInfo.type = LSC_ADDR_TYPE_BTI;

    surface = createImm(0xFF, Type_UD);

    src0Addr =
      createSrc(tmpAddrs->getRegVar(), 0, 0, getRegionStride1(), Type_UD);
  }

  SFID sfid = SFID::NULL_SFID;
  switch (lscSfid) {
  case LSC_UGM:
    sfid = SFID::UGM;
    break;
  case LSC_UGML:
    sfid = SFID::UGML;
    break;
  case LSC_SLM:
    sfid = SFID::SLM;
    break;
  default:
    vISA_ASSERT_UNREACHABLE("invalid SFID for untyped LSC message");
  }

  const auto opInfo = LscOpInfoGet(op);
  vISA_ASSERT_INPUT(!opInfo.isBlock2D(),
               "use translateLscUntypedBlock2DInst for lsc_*_block2d");

  check(opInfo.kind == LscOpInfo::LOAD || opInfo.kind == LscOpInfo::STORE ||
            opInfo.kind == LscOpInfo::ATOMIC,
        "unhandled LSC op class");

  // send descriptor
  uint32_t desc = 0;
  uint32_t exDesc = 0;

  // try and promote the surface identifier (e.g. BTI or SS obj) to ex desc
  surface = lscTryPromoteSurfaceImmToExDesc(surface, addrInfo.type, exDesc);

  // Desc[5:0] is the message opcode
  desc |= opInfo.encoding; // Desc[5:0]

  // build the descriptor (Sect. 3.3.1 of the HAS)
  //
  //   Desc[5:0] = OPCODE {LOAD,STORE,LOAD_BLOCK,STORE_BLOCK,...}
  //   Desc[8:7] = addr size
  //   Desc[11:9] = data size
  //   Desc[15:12] = data vector size (or cmask)
  //   Desc[19:17] = caching controls (see the table for allowable combinations)
  //   Desc[30:29] = addr model (BTI = 3, SS = 2, BSS = 1, FLAT = 0)
  //
  // All other bits are undefined as of now
  //
  const int addrSizeBits = lscEncodeAddrSize(addrInfo.size, desc, status);
  const int dataSizeBits = lscEncodeDataSize(dataShape.size, desc, status);

  int vecSize = 0; // definitely assigned
  if (!opInfo.hasChMask()) {
    vecSize = lscEncodeDataElems(dataShape.elems, desc, status);
    lscEncodeDataOrder(dataShape.order, desc, status);
  } else {
    vISA_ASSERT_INPUT(dataShape.chmask, "channel mask must not be empty");
    vecSize = 0;
    if (dataShape.chmask & LSC_DATA_CHMASK_X) {
      desc |= 1 << 12;
      vecSize++;
    }
    if (dataShape.chmask & LSC_DATA_CHMASK_Y) {
      desc |= 1 << 13;
      vecSize++;
    }
    if (dataShape.chmask & LSC_DATA_CHMASK_Z) {
      desc |= 1 << 14;
      vecSize++;
    }
    if (dataShape.chmask & LSC_DATA_CHMASK_W) {
      desc |= 1 << 15;
      vecSize++;
    }
  }

  if (m_options->getOption(vISA_DisablePrefetchToL1Cache)) {
    // Disable L1 cached for prefetch messages only
    bool isPrefetchMsg = opInfo.isLoad() && isNullOperand(dstRead);
    if (isPrefetchMsg && (cacheOpts.l1 == LSC_CACHING_CACHED ||
                          cacheOpts.l1 == LSC_CACHING_STREAMING)) {
      cacheOpts.l1 = LSC_CACHING_UNCACHED;
    }
  }

  lscEncodeCachingOpts(opInfo, cacheOpts, desc, status);
  lscEncodeAddrType(addrInfo.type, desc, status);

  ///////////////////////////////////////////////////////////////////////////
  // address adjustment and extra codegen (adds, shifts, and multiplies)
  // only pass exDesc if it's an immediate field
  auto addrExecSize = execSize;
  auto addrExecCtrl = execCtrl;
  const auto isStrided =
      op == LSC_OP::LSC_LOAD_STRIDED || op == LSC_OP::LSC_STORE_STRIDED;
  if (isStrided) {
    addrExecSize = g4::SIMD1;
    addrExecCtrl = vISA_EMASK_M1_NM;
  }
  src0Addr = lscLoadEffectiveAddress(op, lscSfid, pred, addrExecSize,
                                     addrExecCtrl, addrInfo, dataSizeBits / 8,
                                     surface, src0Addr, exDesc
  );

  uint32_t dataRegs = 1;
  uint32_t addrRegs = 1;

  G4_ExecSize minExecSize = lscMinExecSize(lscSfid);

  if (dataShape.order == LSC_DATA_ORDER_NONTRANSPOSE) {
    // Non-transpose case is the typical case.
    //
    // ceil[ SIMT32*dataSize(b)/512(b/REG) ] * vecSize
    //   units = (b/b*REG) = REG
    uint32_t width = std::max(execSize, minExecSize);
    dataRegs = std::max<uint32_t>(1, width * dataSizeBits / 8 / BYTES_PER_REG) *
               vecSize;
    addrRegs = std::max<uint32_t>(1, width * addrSizeBits / 8 / BYTES_PER_REG);

    if (execSize < minExecSize) {
      // we may need to even-align src and data
      auto evenAlignDcl = [this](G4_Operand *opnd) {
        G4_Declare *dcl = opnd->getTopDcl()->getRootDeclare();
        if (dcl->getByteSize() <= getGRFSize()) {
          dcl->setEvenAlign();
        }
      };

      if ((addrSizeBits / 8) * minExecSize > getGRFSize()) {
        evenAlignDcl(src0Addr);
      }

      if ((dataSizeBits / 8) * minExecSize > getGRFSize()) {
        if (!isNullOperand(dstRead)) {
          evenAlignDcl(dstRead);
        }
        if (!isNullOperand(src1Data)) {
          evenAlignDcl(src1Data);
        }
      }
      // we don't need to align src2 if it exists, as we'd need to generate
      // a temp send payload containing both src1 and src2 anyway
    }
  } else { // if (dataShape.order == LSC_DATA_TRANSPOSE) {
           // The transpose case is a little odder
           //
           // So the data size is the SIMD size (ExecSize) times the number of
           // registers consumed by each vector sequence (always a full
           // register number per seq).
    uint32_t regsPerVec = vecSize * dataSizeBits / 8 / BYTES_PER_REG;
    if (vecSize * dataSizeBits / 8 % BYTES_PER_REG)
      regsPerVec++; // pad out to full reg
    dataRegs = regsPerVec * execSize;
  }

  // override sizes for special cases
  if (op == LSC_OP::LSC_LOAD_STATUS) {
    dataRegs = 1; // this message just returns a bitset in the low DW
  }

  // cases that need a payload register built
  if (isStrided) {
    src0Addr = lscBuildStridedPayload(
        pred, src0Addr, src0AddrStride, dataSizeBits / 8, vecSize,
        dataShape.order == LSC_DATA_ORDER_TRANSPOSE);
    addrRegs = 1;
  }

  int src1Len = 0;
  uint32_t dstLen = 0;
  uint32_t src0Len = addrRegs;
  if (opInfo.isLoad()) {
    if (isNullOperand(dstRead)) {
      dstLen = 0; // prefetch
    } else {
      dstLen = dataRegs;
    }
    src1Len = 0;
  } else if (opInfo.isStore()) {
    dstLen = 0;
    src0Len = addrRegs;
    src1Len = (int)dataRegs;
  } else if (opInfo.isAtomic()) {
    if (opInfo.extraOperands == 0) { // e.g. lsc_atomic_iinc
      check(isNullOperand(src1Data) && isNullOperand(src2Data),
            "atomic unary must have null src1 and src2");
    } else if (opInfo.extraOperands == 1) { // e.g. lsc_atomic_add
      check(!isNullOperand(src1Data) && isNullOperand(src2Data),
            "atomic binary must have non-null src1 and null src2");
    } else {
      // lsc_atomic_icas/lsc_atomic_fcas: coalesce parmeters into one
      check(!isNullOperand(src1Data) && !isNullOperand(src2Data),
            "atomic ternary must have non-null src1 and src2");
      src1Data = coalescePayload(pred, BYTES_PER_REG, BYTES_PER_REG,
                                 std::max(minExecSize, execSize), execSize,
                                 {src1Data, src2Data}, execCtrl);
    }
    src1Len = (int)dataRegs * opInfo.extraOperands;

    if (dstRead->isNullReg()) {
      dstLen = 0;
    } else {
      dstLen = dataRegs;
    }
  } else {
    check(false, "unexpected message type");
  }

  check(dstLen < 32, "too many destination registers (read operand)");
  check(src0Len < 32, "too many src0 registers (address)");
  check(src1Len < 32, "too many src1 registers (write operand)");

  // FIXME: we need to first sort out what the rules are on virtual registers
  // I initially thought that one was supposed to use an alias over a .decl
  // And have properly sized inputs, but this assumption is proving false.
  auto checkDeclSize = [&](const char *what, G4_Declare *dcl, int visaRegsInDcl,
                           int genRegsNeeded) {
    // if (visaRegsInDcl != genRegsNeeded)
    if (visaRegsInDcl < genRegsNeeded) {
      std::stringstream ss;
      ss << what << " register dimensions don't fit data type\n";
      ss << "vISA decl given is: ";
      dcl->emit(ss);
      ss << " (" << (dcl->getTotalElems() * dcl->getElemSize()) << "B)\n";
      ss << "but payload should be " << genRegsNeeded << " reg(s)\n";
      switch (addrInfo.size) {
      case LSC_ADDR_SIZE_16b:
        ss << "addr size is 16b";
        break;
      case LSC_ADDR_SIZE_32b:
        ss << "addr size is 32b";
        break;
      case LSC_ADDR_SIZE_64b:
        ss << "addr size is 64b";
        break;
      default:
        ss << "??";
      }
      ss << " x " << (int)execSize << " elem(s) ";
      if (dataShape.order == LSC_DATA_ORDER_TRANSPOSE) {
        ss << "transposed ";
      } else {
        ss << "non-transposed ";
      }
      ss << " and data ";
      switch (dataShape.size) {
      case LSC_DATA_SIZE_8b:
        ss << "8b";
        break;
      case LSC_DATA_SIZE_16b:
        ss << "16b";
        break;
      case LSC_DATA_SIZE_64b:
        ss << "64b";
        break;
      default:
        ss << "32b";
        break; // 32b or the conversion types
      }
      ss << " x " << vecSize;
      check(false, ss.str().c_str());
    }
  };

  // Some sanity checking of vISA region sizes with the computed sizes
  G4_Declare *addrDcl =
      src0Addr->getBase()->asRegVar()->getDeclare()->getRootDeclare();
  check(addrDcl, "cannot find declaration for address register");

  // disable size checks if execSize is < min payload width,
  // since declares is allowed to be smaller than payload size in this case
  if (execSize >= minExecSize) {
    if (addrDcl) {
      auto addrRegSize = addrDcl->getElemSize() * addrDcl->getTotalElems();
      auto visaAddrRegsInDcl = std::max<int>(addrRegSize / getGRFSize(), 1);
      checkDeclSize("address", addrDcl, visaAddrRegsInDcl, addrRegs);
    }

    // loading/store into the null register for prefetch
    if (!isNullOperand(dstRead)) {
      // sanity check the number of destination operands with the types given
      G4_Declare *dstDcl =
          dstRead->getBase()->asRegVar()->getDeclare()->getRootDeclare();
      check(dstDcl != nullptr, "cannot find declaration for data register");
      unsigned dataRegBytes = dstDcl->getTotalElems() * dstDcl->getElemSize();
      auto visaRegsInDcl = std::max<int>(dataRegBytes / getGRFSize(), 1);
      checkDeclSize("data", dstDcl, visaRegsInDcl, dstLen);
    }
  }

  desc |= dstLen << 20;   // Desc[24:20]  dst len
  desc |= addrRegs << 25; // Desc[29:25]  src0 len

  bool loadAccess = opInfo.isLoad();
  bool storeAccess = opInfo.isStore();
  if (opInfo.isAtomic()) {
    // atomic can be both read and store
    storeAccess = true;
    loadAccess = !isNullOperand(dstRead);
  }
  G4_SendDescRaw *msgDesc =
      createLscDesc(sfid, desc, exDesc, src1Len,
                    getSendAccessType(loadAccess, storeAccess),
                    surface, LdStAttrs::NONE);

  createLscSendInst(pred, dstRead, src0Addr, src1Data, execSize, msgDesc,
                    instOpt, addrInfo.type, true);

  return status;
}

int IR_Builder::translateLscUntypedBlock2DInst(
    LSC_OP op, LSC_SFID lscSfid, G4_Predicate *pred,
    VISA_Exec_Size visaExecSize, VISA_EMask_Ctrl emask,
    LSC_CACHE_OPTS cacheOpts, LSC_DATA_SHAPE_BLOCK2D dataShape2D,
    G4_DstRegRegion *dstRead, // dst can be NULL reg (e.g store)
    G4_Operand *src0Addrs[LSC_BLOCK2D_ADDR_PARAMS], // always the addresses
    G4_SrcRegRegion *src1Data                       // store data
){
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  int status = VISA_SUCCESS;
  auto check = [&](bool z, const char *what) {
    if (!z) {
      vISA_ASSERT_INPUT(false, std::string(what));
      status = VISA_FAILURE;
    }
  };

  if (dataShape2D.order == LSC_DATA_ORDER_TRANSPOSE) {
    switch (dataShape2D.size) {
    case LSC_DATA_SIZE_8b:
      check (false, "d8 not supported");
      break;
    case LSC_DATA_SIZE_16b:
      check (false, "d16 not supported");
      break;
    case LSC_DATA_SIZE_32b:
      check (dataShape2D.width <= 16 && dataShape2D.height <= 32,
          "for d32, width <= 16 and height <= 32 elements");
      break;
    case LSC_DATA_SIZE_64b:
      check (dataShape2D.width <= 4 && dataShape2D.height == 8,
          "for d64, width <= 8 and height <= 32 elements");
      break;
    default:
      check(false, "invalid data type");
    }
  }

  const auto opInfo = LscOpInfoGet(op);
  vISA_ASSERT_INPUT(opInfo.isBlock2D(), "not an LSC block2d op");

  const G4_ExecSize execSize = toExecSize(visaExecSize);
  const G4_InstOpts instOpt = Get_Gen4_Emask(emask, execSize);

  G4_SrcRegRegion *src0Addr =
      lscBuildBlock2DPayload(dataShape2D, pred, src0Addrs);

  SFID sfid = SFID::NULL_SFID;
  switch (lscSfid) {
  case LSC_UGM:
    sfid = SFID::UGM;
    break;
  case LSC_UGML:
    sfid = SFID::UGML;
    break;
  case LSC_SLM:
    sfid = SFID::SLM;
    break;
  default:
    vISA_ASSERT_UNREACHABLE("invalid SFID for untyped block2d LSC message");
  }
  // send descriptor
  uint32_t desc = 0;
  uint32_t exDesc = 0;

  desc |= opInfo.encoding;
  if (dataShape2D.vnni)
    desc |= (1 << 7); // Desc[7]
  int dataSizeBits = lscEncodeDataSize(dataShape2D.size, desc, status);
  if (dataShape2D.order == LSC_DATA_ORDER_TRANSPOSE)
    desc |= (1 << 15);
  lscEncodeCachingOpts(opInfo, cacheOpts, desc, status);
  desc |= (0 << 29); // Desc[30:29] = FLAT

  uint32_t dataRegs =
      lscBlock2dComputeDataRegs(op, dataShape2D, getGRFSize(), dataSizeBits);
  uint32_t addrRegs = 1;

  int src1Len = 0;
  uint32_t dstLen = 0;
  uint32_t src0Len = addrRegs;

  if (opInfo.isLoad()) {
    if (isNullOperand(dstRead)) {
      dstLen = 0; // prefetch
    } else {
      dstLen = dataRegs;
    }
    src1Len = 0;
  } else if (opInfo.isStore()) {
    dstLen = 0;
    src0Len = addrRegs;
    src1Len = (int)dataRegs;
  } else {
    check(false, "unexpected message type");
  }

  desc |= dstLen << 20;   // Desc[24:20]  dst len
  desc |= addrRegs << 25; // Desc[28:25]  src0 len

  G4_SendDescRaw *msgDesc =
      createLscDesc(sfid, desc, exDesc, src1Len,
                    getSendAccessType(opInfo.isLoad(), opInfo.isStore()),
                    nullptr, LdStAttrs::NONE);

  G4_InstSend *sendInst =
      createLscSendInst(pred, dstRead, src0Addr, src1Data, execSize, msgDesc,
                        instOpt, LSC_ADDR_TYPE_FLAT, true);
  (void)sendInst;

  return status;
}

// return (dst,src1)
static std::pair<uint32_t,uint32_t> computeLscTypedDataRegs(
  IR_Builder &irb,
  const LscOpInfo &opInfo,
  G4_ExecSize execSize,
  int dataSizeBits,
  bool dstIsNull,
  int chMask)
{
  int numChannels = 0;
  if (opInfo.hasChMask()) {
    for (auto LSC_CH : {LSC_DATA_CHMASK_X, LSC_DATA_CHMASK_Y,
                        LSC_DATA_CHMASK_Z, LSC_DATA_CHMASK_W})
    {
      if (chMask & LSC_CH)
        numChannels++;
    }
  } else {
    // atomics are single channel
    numChannels = 1;
  }

  const int BYTES_PER_GRF = irb.getGRFSize();
  const int regsPerDataChannel =
      std::max<int>(1, dataSizeBits * (int)execSize / 8 / BYTES_PER_GRF);
  const uint32_t dataRegs = regsPerDataChannel * numChannels;

  uint32_t dstRegs = 0;
  if (dstIsNull) {
    dstRegs = 0; // atomic with no return or load to %null
  } else if (opInfo.isOneOf(LSC_LOAD_STATUS, LSC_READ_STATE_INFO)) {
    dstRegs = 1; // special cases that load an opaque register
  } else if (opInfo.isLoad() || opInfo.isAtomic()) {
    dstRegs = dataRegs; // normal things that load
  } else { // store
    dstRegs = 0;
  }
  uint32_t src1Len = 0;
  if (opInfo.isStore()) {
    src1Len = dataRegs;
  } else if (opInfo.isAtomic()) {
    src1Len = dataRegs * opInfo.extraOperands;
  } else { // load
    src1Len= 0;
  }
  return std::make_pair(dstRegs, src1Len);
}

int IR_Builder::translateLscTypedInst(
    LSC_OP op, G4_Predicate *pred, VISA_Exec_Size execSizeEnum,
    VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, LSC_ADDR_TYPE addrModel,
    LSC_ADDR_SIZE addrSize, LSC_DATA_SHAPE shape,
    G4_Operand *surface,      // surface/bti
    G4_DstRegRegion *dstData, // dst on load/atomic
    G4_SrcRegRegion *coord0s, G4_SrcRegRegion *coord1s,
    G4_SrcRegRegion *coord2s, G4_SrcRegRegion *features,
    G4_SrcRegRegion *src1Data, // store data/extra atomic operands
    G4_SrcRegRegion *src2Data  // icas/fcas only
) {
  TIME_SCOPE(VISA_BUILDER_IR_CONSTRUCTION);

  int status = VISA_SUCCESS;

  const uint32_t BYTES_PER_GRF = getGRFSize();

  const G4_ExecSize execSize = toExecSize(execSizeEnum);
  const G4_InstOpts instOpt = Get_Gen4_Emask(emask, execSize);

  const auto opInfo = LscOpInfoGet(op);

  uint32_t desc = opInfo.encoding;
  uint32_t exDesc = 0;

  G4_SrcRegRegion *srcAddrs[2]{};
  G4_SrcRegRegion *srcData = nullptr;
  unsigned srcAddrRegs[2]{};


  auto checkPayloadSize = [&](const char *which, const G4_Declare *decl,
                              int expectDeclRegs) {
    int dclRegs = std::max<int>(1, decl->getTotalElems() * decl->getElemSize() /
                                       BYTES_PER_GRF);
    // (expectDeclRegs != dclRegs)
    // cannot expect equality; the declaration could be a raw variable
    // with consisting of an offset into a bigger block
    if (expectDeclRegs > dclRegs) {
      std::stringstream ss;
      ss << which << " .decl size ";
      decl->emit(ss);
      ss << " (" << dclRegs << ")";
      ss << " mismatches expected number of registers for "
            "payload ("
         << expectDeclRegs << ")";
      vISA_ASSERT_INPUT(false, ss.str());
    }
  };

  auto checkAddrPayloadSize = [&](const char *which,
                                  const G4_SrcRegRegion *srcAddr) {
    if (srcAddr == nullptr || srcAddr->isNullReg()) {
      return;
    }
    const G4_Declare *decl = getDeclare(srcAddr);
    uint32_t addrSizeBits = lscComputeAddrSize(addrSize, status);
    const int regsPerAddrChannel =
        std::max<int>(1, addrSizeBits * (int)execSize / 8 / BYTES_PER_GRF);
    checkPayloadSize(which, decl, regsPerAddrChannel);
  };

  checkAddrPayloadSize("src0AddrUs", coord0s);
  checkAddrPayloadSize("src0AddrVs", coord1s);
  checkAddrPayloadSize("src0AddrRs", coord2s);

  if (opInfo.is(LSC_READ_STATE_INFO)) {
    // like fences, send requires *something* (at least one reg) to be
    // sent out; we pick the initial r0 value since it's known to
    // be floating around somewhere until EOT
    const RegionDesc *rd = getRegionStride1();
    G4_Declare *r0 = getBuiltinR0();
    G4_SrcRegRegion *src0Dummy = createSrc(r0->getRegVar(), 0, 0, rd, Type_UD);
    srcAddrRegs[0] = 1;
    srcAddrRegs[1] = 0;
    srcAddrs[0] = src0Dummy;
  } else {
    PayloadSource srcAddrPayloads[4]{}; // U, V, R, feature
    unsigned numSrcAddrPayloads = 0;
    buildTypedSurfaceAddressPayload(coord0s, coord1s, coord2s,
                                    features, execSize, instOpt,
                                    srcAddrPayloads, numSrcAddrPayloads);
    preparePayload(
        srcAddrs, srcAddrRegs, execSize,
        false, // not a split send (so all the addrs lands in one reg)
        srcAddrPayloads, numSrcAddrPayloads);
    vISA_ASSERT_INPUT(srcAddrs[1] == nullptr, "invalid addr split");
    vISA_ASSERT_INPUT(srcAddrRegs[0] < 32, "too many address registers");

    // each channel consumes at least one register (top padding may be 0)
    srcData = coalescePayload(pred, BYTES_PER_GRF, BYTES_PER_GRF,
                              std::max(getNativeExecSize(), execSize), execSize,
                              {src1Data, src2Data}, emask);
  }
  surface = lscTryPromoteSurfaceImmToExDesc(surface, addrModel, exDesc);

  if (opInfo.hasChMask()) {
    desc |= (shape.chmask & 0xF) << 12;
    vISA_ASSERT_INPUT(shape.chmask & 0xF, "empty channel mask");
  } // tgm atomics are single channel

  int addrSizeBits = lscEncodeAddrSize(addrSize, desc, status);
  int dataSizeBits = lscEncodeDataSize(shape.size, desc, status);
  (void)addrSizeBits;

  lscEncodeCachingOpts(opInfo, cacheOpts, desc, status); // Desc[19:17]
  lscEncodeAddrType(addrModel, desc, status);

  auto [dstRegs,src1Len] =
    computeLscTypedDataRegs(
      *this, opInfo, execSize, dataSizeBits,
      dstData == nullptr || dstData->isNullReg(), shape.chmask);

  desc |= (srcAddrRegs[0] & 0xF) << 25; // mlen == Desc[28:25]
  desc |= (dstRegs & 0x1F) << 20; // rlen == Desc[24:20]

  G4_SendDescRaw *msgDesc =
      createLscDesc(SFID::TGM, desc, exDesc, (int)src1Len,
                    getSendAccessType(opInfo.isLoad(), opInfo.isStore()),
                    surface, LdStAttrs::NONE);
  G4_InstSend *sendInst =
      createLscSendInst(pred, dstData, srcAddrs[0], srcData, execSize, msgDesc,
                        instOpt, addrModel, true);
  (void)sendInst;

  return status;
}

LSC_DATA_ELEMS IR_Builder::lscGetElementNum(unsigned eNum) const {
  switch (eNum) {
  case 1:
    return LSC_DATA_ELEMS_1;
  case 2:
    return LSC_DATA_ELEMS_2;
  case 3:
    return LSC_DATA_ELEMS_3;
  case 4:
    return LSC_DATA_ELEMS_4;
  case 8:
    return LSC_DATA_ELEMS_8;
  case 16:
    return LSC_DATA_ELEMS_16;
  case 32:
    return LSC_DATA_ELEMS_32;
  case 64:
    return LSC_DATA_ELEMS_64;
  default:
    return LSC_DATA_ELEMS_INVALID;
  };

  return LSC_DATA_ELEMS_INVALID;
}

int IR_Builder::lscEncodeAddrSize(LSC_ADDR_SIZE addrSize, uint32_t &desc,
                                  int &status) const {
  int addrSizeBits = 32;
  uint32_t addrSizeEnc = 0;
  switch (addrSize) {
  case LSC_ADDR_SIZE_16b:
    addrSizeEnc = 0x1;
    addrSizeBits = 16;
    break;
  case LSC_ADDR_SIZE_32b:
    addrSizeEnc = 0x2;
    addrSizeBits = 32;
    break;
  case LSC_ADDR_SIZE_64b:
    addrSizeEnc = 0x3;
    addrSizeBits = 64;
    break;
  default:
    vISA_ASSERT_UNREACHABLE("invalid address size");
    status = VISA_FAILURE;
  }
  desc |= addrSizeEnc << 7; // Desc[8:7]
  return addrSizeBits;
}

uint32_t IR_Builder::lscComputeAddrSize(LSC_ADDR_SIZE addrSize,
                                        int &status) const {
  int addrSizeBits = 32;
  switch (addrSize) {
  case LSC_ADDR_SIZE_16b:
    addrSizeBits = 16;
    break;
  case LSC_ADDR_SIZE_32b:
    addrSizeBits = 32;
    break;
  case LSC_ADDR_SIZE_64b:
    addrSizeBits = 64;
    break;
  default:
    vISA_ASSERT_UNREACHABLE("invalid address size");
    status = VISA_FAILURE;
  }
  return addrSizeBits;
}

uint32_t IR_Builder::lscComputeDataSize(LSC_DATA_SIZE dataSize,
                                        int &status) const {
  int dataSizeBits = 32;
  switch (dataSize) {
  case LSC_DATA_SIZE_8b:
    dataSizeBits = 8;
    break;
  case LSC_DATA_SIZE_16b:
    dataSizeBits = 16;
    break;
  case LSC_DATA_SIZE_32b:
    dataSizeBits = 32;
    break;
  case LSC_DATA_SIZE_64b:
    dataSizeBits = 64;
    break;
  case LSC_DATA_SIZE_8c32b:
    dataSizeBits = 32;
    break;
  case LSC_DATA_SIZE_16c32b:
    dataSizeBits = 32;
    break;
  case LSC_DATA_SIZE_16c32bH:
    dataSizeBits = 32;
    break;
  default:
    vISA_ASSERT_UNREACHABLE("invalid data size");
    status = VISA_FAILURE;
  }
  return dataSizeBits;
}

int IR_Builder::lscEncodeDataSize(LSC_DATA_SIZE dataSize, uint32_t &desc,
                                  int &status) const {
  uint32_t dataSizeEnc = 0;
  int dataSizeBits = 32;
  switch (dataSize) {
  case LSC_DATA_SIZE_8b:
    dataSizeEnc = 0x0;
    dataSizeBits = 8;
    break;
  case LSC_DATA_SIZE_16b:
    dataSizeEnc = 0x1;
    dataSizeBits = 16;
    break;
  case LSC_DATA_SIZE_32b:
    dataSizeEnc = 0x2;
    dataSizeBits = 32;
    break;
  case LSC_DATA_SIZE_64b:
    dataSizeEnc = 0x3;
    dataSizeBits = 64;
    break;
  case LSC_DATA_SIZE_8c32b:
    dataSizeEnc = 0x4;
    dataSizeBits = 32;
    break;
  case LSC_DATA_SIZE_16c32b:
    dataSizeEnc = 0x5;
    dataSizeBits = 32;
    break;
  case LSC_DATA_SIZE_16c32bH:
    dataSizeEnc = 0x6;
    dataSizeBits = 32;
    break;
  default:
    vISA_ASSERT_UNREACHABLE("invalid data size");
    status = VISA_FAILURE;
  }
  desc |= dataSizeEnc << 9; // Desc[11:9]
  return dataSizeBits;
}

int IR_Builder::lscEncodeDataElems(LSC_DATA_ELEMS dataElems, uint32_t &desc,
                                   int &status) const {
  uint32_t vecSizeEnc = 0;
  int vecSize = 1;
  switch (dataElems) {
  case LSC_DATA_ELEMS_1:
    vecSizeEnc = 0x0;
    vecSize = 1;
    break;
  case LSC_DATA_ELEMS_2:
    vecSizeEnc = 0x1;
    vecSize = 2;
    break;
  case LSC_DATA_ELEMS_3:
    vecSizeEnc = 0x2;
    vecSize = 3;
    break;
  case LSC_DATA_ELEMS_4:
    vecSizeEnc = 0x3;
    vecSize = 4;
    break;
  case LSC_DATA_ELEMS_8:
    vecSizeEnc = 0x4;
    vecSize = 8;
    break;
  case LSC_DATA_ELEMS_16:
    vecSizeEnc = 0x5;
    vecSize = 16;
    break;
  case LSC_DATA_ELEMS_32:
    vecSizeEnc = 0x6;
    vecSize = 32;
    break;
  case LSC_DATA_ELEMS_64:
    vecSizeEnc = 0x7;
    vecSize = 64;
    break;
  default:
    vISA_ASSERT_UNREACHABLE("number of data elements");
    status = VISA_FAILURE;
  }
  desc |= vecSizeEnc << 12; // desc[14:12] is the vector size
  return vecSize;
}

void IR_Builder::lscEncodeDataOrder(LSC_DATA_ORDER order, uint32_t &desc,
                                    int &status) const {
  if (order == LSC_DATA_ORDER_TRANSPOSE) {
    desc |= 1 << 15; // desc[15] is transpose
  } else if (order != LSC_DATA_ORDER_NONTRANSPOSE) {
    vISA_ASSERT_INPUT(false, "bad transpose value");
    status = VISA_FAILURE;
  }
}

void IR_Builder::lscEncodeCachingOpts(const LscOpInfo &opInfo,
                                      LSC_CACHE_OPTS cacheOpts, uint32_t &desc,
                                      int &status) const {
  uint32_t cacheEnc = 0;
  if (!LscTryEncodeCacheOpts(opInfo, cacheOpts, cacheEnc,
                             isLSCCacheOpt17_19())) {
    vISA_ASSERT_INPUT(false, "unsupported caching options");
    status = VISA_FAILURE;
  }

  desc |= cacheEnc;
}

void IR_Builder::lscEncodeAddrType(LSC_ADDR_TYPE addrModel, uint32_t &desc,
                                   int &status) const {
  uint32_t addrTypeEnc = 0;
  switch (addrModel) {
  case LSC_ADDR_TYPE_FLAT:
    addrTypeEnc = 0;
    break;
  case LSC_ADDR_TYPE_BSS:
    addrTypeEnc = 1;
    break;
  case LSC_ADDR_TYPE_SS:
    addrTypeEnc = 2;
    break;
  case LSC_ADDR_TYPE_BTI:
    addrTypeEnc = 3;
    break;
  default:
    vISA_ASSERT_UNREACHABLE("invalid address model");
    status = VISA_FAILURE;
  }
  desc |= addrTypeEnc << 29; // [30:29] addr size
}

G4_SrcRegRegion *
IR_Builder::lscBuildStridedPayload(G4_Predicate *pred,
                                   G4_SrcRegRegion *src0AddrBase, // output
                                   G4_Operand *src0AddrStride,
                                   int dataSizeBytes, int vecSize,
                                   bool transposed) {
  const uint32_t BYTES_PER_REG = getGRFSize();
  // We've been passed in a single value for the address, and we
  // have to generate the address payload register from that value
  // along with the pitch.
  //
  // E.g. we've been passed in the following.
  // .decl VADDR v_type=G type=UD num_elts=1 align=GRF
  //       (VADDR doesn't necessarily need to be GRF aligned)
  //
  // We need to generate:
  //    .decl VADDR_REG_UD v_type=G type=UD num_elts=NUM_PER_GRF(T) align=GRF
  //    .decl VADDR_REG_UQ type=UQ alias=<VADDR_REG_UD,0>
  //
  G4_Declare *addrTmpDeclUd = createSendPayloadDcl(BYTES_PER_REG / 4, Type_UD);
  G4_Declare *addrTmpDeclUq = createSendPayloadDcl(BYTES_PER_REG / 8, Type_UQ);
  addrTmpDeclUq->setAliasDeclare(addrTmpDeclUd, 0);
  //
  // Then to build the payload we need the following.
  //    ...
  //  [for 64b base addresses]
  //    (P) mov (M1_NM,1) VADDR_REG(0,0)<1>:uq  VADDR(0,0)<0;1,0>:T
  //  [for 32b base addresses]
  //    (P) mov (M1_NM,1) VADDR_REG(0,0)<1>:ud  VADDR(0,0)<0;1,0>:T
  //  ...
  //    (P) mov (M1_NM,1) VADDR_REG(0,2)<1>:ud  sizeof(T):ud
  //    (P) send (M1_NM,1) VDATA  VADDR_REG  null  lsc_load_block....
  //
  if (src0AddrBase->getType() == Type_UQ || src0AddrBase->getType() == Type_Q) {
    G4_DstRegRegion *payloadDstAddrUq =
        createDst(addrTmpDeclUq->getRegVar(), 0, 0, 1, Type_UQ);
    createInst(pred, G4_mov, nullptr, g4::NOSAT, g4::SIMD1, payloadDstAddrUq,
               src0AddrBase, nullptr,
               Get_Gen4_Emask(vISA_EMASK_M1_NM, g4::SIMD1), true);
  } else {
    G4_DstRegRegion *payloadDstAddrUd =
        createDst(addrTmpDeclUd->getRegVar(), 0, 0, 1, Type_UD);
    createInst(pred, G4_mov, nullptr, g4::NOSAT, g4::SIMD1, payloadDstAddrUd,
               src0AddrBase, nullptr,
               Get_Gen4_Emask(vISA_EMASK_M1_NM, g4::SIMD1), true);
  }
  //
  G4_DstRegRegion *payloadDstPitch =
      createDst(addrTmpDeclUd->getRegVar(), 0, 2, 1, Type_UD);
  if (src0AddrStride == nullptr) {
    int defaultPitch = dataSizeBytes;
    if (!transposed)
      defaultPitch *= vecSize;
    src0AddrStride = createImmWithLowerType(defaultPitch, Type_UD);
  }
  createInst(pred, G4_mov, 0, g4::NOSAT, g4::SIMD1, payloadDstPitch,
             src0AddrStride, nullptr,
             Get_Gen4_Emask(vISA_EMASK_M1_NM, g4::SIMD1), true);
  //
  return createSrc(addrTmpDeclUd->getRegVar(), 0, 0, getRegionScalar(),
                   Type_UD);
}

G4_SrcRegRegion *
IR_Builder::lscBuildBlock2DPayload(LSC_DATA_SHAPE_BLOCK2D dataShape2D,
                                   G4_Predicate *pred,
                                   G4_Operand *src0Addrs[6]) {
  // Similar to lscBuildStridedPayload, but this formats the payload
  // as follows.
  //
  // A2DBLOCK_PAYLOAD:
  //   [31:0]:    base address lo (32b)
  //   [63:32]:   base address hi (32b)
  //   [95:64]:   surface width minus 1 (32b)
  //   [127:96]:  surface height minus 1 (32b)
  //   [159:128]: surface pitch minus 1 (32b)
  //   [191:160]: block X (32b)
  //   [223:192]: block Y (32b)
  //   [231:224]: block width (8b)
  //   [239:232]: block height (8b)
  //   [243:240]: array length (4b)
  //   [255:244]: UNDEFINED
  //
  // [StartX:s32, StartY:s32, Width:u32, Height:u32, ArrayLenMinus1:u4]
  // ArrayLenMinus1 is at [131:128]
  //
  // We generate the following.  Since the width and height are immediate
  //
  //   .decl VADDR_REG_UD v_type=G type=UD num_elts=NUM_PER_GRF(T) align=GRF
  //   .decl VADDR_REG_UQ type=UQ alias=<VADDR_REG_UD,0>
  //   mov (M1_NM,1) ADDR(0,0):d   src0AddrX
  //   mov (M1_NM,1) ADDR(0,1):d   src0AddrY
  //   mov (M1_NM,1) ADDR(0,1):uq  ((blockWidth << 32)|blockHeight):uq
  //   mov (M1_NM,1) ADDR(0,4):d   arrayLen:uw
  const uint32_t BYTES_PER_REG = getGRFSize();
  G4_Declare *addrTmpDeclUd = createSendPayloadDcl(BYTES_PER_REG / 4, Type_UD);
  G4_Declare *addrTmpDeclUq = createSendPayloadDcl(BYTES_PER_REG / 8, Type_UQ);
  addrTmpDeclUq->setAliasDeclare(addrTmpDeclUd, 0);
  ///////////////////////
  auto movUQ = [&](int dstSubReg, G4_Operand *src) {
    G4_DstRegRegion *payloadDstAddr_0_Q =
        createDst(addrTmpDeclUq->getRegVar(), 0, dstSubReg, 1, Type_UQ);
    createInst(pred, G4_mov, nullptr, g4::NOSAT, g4::SIMD1, payloadDstAddr_0_Q,
               src, nullptr, Get_Gen4_Emask(vISA_EMASK_M1_NM, g4::SIMD1), true);
  };
  auto movUD = [&](int dstSubReg, G4_Operand *src) {
    G4_DstRegRegion *payloadDst =
        createDst(addrTmpDeclUd->getRegVar(), 0, dstSubReg, 1, Type_UD);
    createInst(pred, G4_mov, nullptr, g4::NOSAT, g4::SIMD1, payloadDst, src,
               nullptr, Get_Gen4_Emask(vISA_EMASK_M1_NM, g4::SIMD1), true);
  };
  auto movImmUD = [&](int dstSubReg, uint32_t imm) {
    movUD(dstSubReg, createImmWithLowerType(imm, Type_UD));
  };

  ///////////////////////////////////
  //   .decl ADDR v_type=G type=UD num_elts=NUM_PER_GRF(T) align=GRF
  //   .decl ADDR type=UQ alias=<VADDR_REG_UD,0>
  //   mov (M1_NM,1) ADDR(0,0):uq   src0AddrBase[0]:uq
  //   mov (M1_NM,1) ADDR(0,2):ud   src0AddrBase[1]:ud
  //   mov (M1_NM,1) ADDR(0,3):ud   src0AddrBase[2]:ud
  //   mov (M1_NM,1) ADDR(0,4):ud   src0AddrBase[3]:ud
  //   mov (M1_NM,1) ADDR(0,5):ud   src0AddrBase[4]:ud
  //   mov (M1_NM,1) ADDR(0,6):ud   src0AddrBase[5]:ud
  //   mov (M1_NM,1) ADDR(0,7):ud   (width x height x blocks):ud
  //
  // bottom 64b
  movUQ(0, src0Addrs[0]); // surface address
                          // these start at REG.2:d
  movUD(2, src0Addrs[1]); // surface width - 1
  movUD(3, src0Addrs[2]); // surface height - 1
  movUD(4, src0Addrs[3]); // surface pitch - 1
  movUD(5, src0Addrs[4]); // block x
  movUD(6, src0Addrs[5]); // block y
  uint32_t blockSize = (dataShape2D.width - 1) |
                       ((dataShape2D.height - 1) << 8) |
                       ((dataShape2D.blocks - 1) << 16);
  movImmUD(7, blockSize);
  //
  return createSrc(addrTmpDeclUd->getRegVar(), 0, 0, getRegionScalar(),
                   Type_UD);
}

G4_SrcRegRegion *IR_Builder::lscLoadEffectiveAddress(
    LSC_OP lscOp, LSC_SFID lscSfid, G4_Predicate *pred, G4_ExecSize execSize,
    VISA_EMask_Ctrl execCtrl, LSC_ADDR addrInfo, int bytesPerDataElem,
    const G4_Operand *surface, G4_SrcRegRegion *addr, uint32_t &exDesc
) {
  vISA_ASSERT_INPUT(addrInfo.immScale == 1, "address scaling not supported yet");
  // The address may need scaling and offset adjustment
  //    NEW_ADDR = SCALE*ADDR + OFF
  //
  // e.g. lsc_load.ugm.d32.a64 ... [4*ADDR - 0x100]
  //

  // emulate scale and add if necessary
  return lscMulAdd(pred, execSize, execCtrl, addr, (int16_t)addrInfo.immScale,
                   addrInfo.immOffset);
}


G4_SrcRegRegion *IR_Builder::lscCheckRegion(G4_Predicate *pred,
                                            G4_ExecSize execSize,
                                            VISA_EMask_Ctrl execCtrl,
                                            G4_SrcRegRegion *src) {
  const G4_Type srcType = src->getType();
  // Later extension could repack and work in these case,
  // for now throw a tantrum if they give us
  // ... VAR<2;1,0>
  // we do permit VAR<0;1,0>
  vISA_ASSERT_INPUT(src->getRegion()->isPackedRegion() ||
                   src->getRegion()->isScalar(),
               "input must be scalar/packed");
  vISA_ASSERT_INPUT(src->getSubRegOff() == 0 || src->getRegion()->isScalar(),
               "vector operands must be register aligned");
  return src;
}

G4_SrcRegRegion *IR_Builder::lscMulAdd(G4_Predicate *pred, G4_ExecSize execSize,
                                       VISA_EMask_Ctrl execCtrl,
                                       G4_SrcRegRegion *src, int16_t mulImm16,
                                       int64_t addImm64) {
  if (mulImm16 == 1 && addImm64 == 0) {
    // no op
    return src;
  } else if (mulImm16 == 1 && addImm64 != 0) {
    // reduces to an add
    return lscAdd(pred, execSize, execCtrl, src, addImm64);
  } else if (mulImm16 != 1 && addImm64 == 0) {
    // reduces to a multiply
    return lscMul(pred, execSize, execCtrl, src, mulImm16);
  } else {
    vISA_ASSERT_UNREACHABLE("multiply not supported yet");
    return nullptr;
    /*
    // hard cases...
    auto srcType = src->getElemType();
    if (srcType == Type_UQ || srcType == Type_Q) {
    // harder case: sub-optimal code for now will
    // flip the lo32/hi32 pairs around twice
    auto *tmpVar = lscMul(pred, execSize, execCtrl, src, mulImm16);
    return lscAdd(pred, execSize, execCtrl, tmpVar, addImm64);
    } else {
    G4_Imm *addImmOpnd;
    if (srcType == Type_UD || srcType == Type_D) {
    MUST_BE_TRUE(
    addImm64 >= std::numeric_limits<int32_t>::min() &&
    addImm64 <= std::numeric_limits<int32_t>::max(),
    "imm offset for A32 must fit in 32b");
    addImmOpnd = createImmWithLowerType(addImm64, srcType);
    } else {
    MUST_BE_TRUE(
    addImm64 >= std::numeric_limits<int16_t>::min() &&
    addImm64 <= std::numeric_limits<int16_t>::max(),
    "imm offset for A16 must fit in 16b");
    addImmOpnd = createImmWithLowerType(addImm64, srcType);
    }
    // can use 32b + 32b x 16b mad (all platforms) (in place)
    // create a new register in case there's aliasing
    G4_Declare *result = createTempVar(execSize, srcType, GRFALIGN);
    G4_DstRegRegion *dstRgn =
    createDst(result->getRegVar(), 0, 0, 1, srcType);
    const auto *srcRgnVal = execSize == 1 ? getRegionScalar() :
    getRegionStride1(); G4_SrcRegRegion *srcRgn =
    createSrcRegRegion(src->getRegVar(), 0, 0, srcRgnVal, srcType);
    //
    G4_Operand *mulImmOp = createImm(mulImm16, Type_W);
    createInst(pred, G4_mad, nullptr, false, execSize,
    dstRgn, addImmOpnd, srcRgn, mulImmOp,
    Get_Gen4_Emask(execCtrl, execSize));
    //
    return result;
    }
    */
  }
}

static bool isPow2(int x) { return (x & (x - 1)) == 0; }
static int intLog2(int x) {
  int shiftAmt = 0;
  while (x > 1) {
    x >>= 1;
    shiftAmt++;
  }
  return shiftAmt;
}

G4_SrcRegRegion *IR_Builder::lscMul(G4_Predicate *pred, G4_ExecSize execSize,
                                    VISA_EMask_Ctrl execCtrl,
                                    G4_SrcRegRegion *src0, int16_t mulImm) {
  if (mulImm == 1)
    return src0;

  const auto srcType = src0->getType();
  if (srcType == Type_UQ || srcType == Type_Q) {
    return lscMul64Aos(pred, execSize, execCtrl, src0, mulImm);
  } else {
      /*
      G4_Declare *result = createTempVar(execSize, srcType, GRFALIGN);
      G4_DstRegRegion *dst =
      createDst(result->getRegVar(), 0, 0, 1, srcType);
      const auto *srcRgn = execSize == 1 ?
      getRegionScalar() : getRegionStride1();
      G4_SrcRegRegion *src0 =
      createSrcRegRegion(srcVar->getRegVar(), 0, 0, srcRgn, srcType);
      G4_Operand *mulImmOp = createImm(mulImm, Type_W);
      createInst(
      duplicateOperand(pred),
      G4_mul, nullptr, false,
      execSize, dst, src0, mulImmOp, execCtrl);
      return result;
      */
      vISA_ASSERT_UNREACHABLE("lscMul unsupported");
    return nullptr;
  }
}

G4_SrcRegRegion *IR_Builder::lscAdd(G4_Predicate *pred, G4_ExecSize execSize,
                                    VISA_EMask_Ctrl execCtrl,
                                    G4_SrcRegRegion *src0, int64_t addImm64) {
  if (addImm64 == 0)
    return src0;

  const G4_Type srcType = src0->getType();
  vISA_ASSERT_INPUT(srcType == Type_UQ || srcType == Type_Q || srcType == Type_UD ||
                   srcType == Type_D || srcType == Type_UW || srcType == Type_W,
               "function only supports integer types");

  src0 = lscCheckRegion(pred, execSize, execCtrl, src0);

  if (srcType == Type_UQ || srcType == Type_Q) {
    if (hasInt64Add()) {
      return lscAdd64AosNative(pred, execSize, execCtrl, src0, addImm64);
    } else {
      return lscAdd64AosEmu(pred, execSize, execCtrl, src0, addImm64);
    }
  } else if ((int32_t)addImm64 != addImm64) {
    vISA_ASSERT_INPUT(false, "<64b add must not use >32b imm off");
  } else if ((srcType == Type_UW || srcType == Type_W) &&
             (int16_t)addImm64 != addImm64) {
    vISA_ASSERT_INPUT(false, "16b add must not use >16b imm off");
  }

  // we can do this in one instruction
  G4_Declare *result = createTempVar(execSize, srcType, getGRFAlign());
  G4_DstRegRegion *dst = createDst(result->getRegVar(), srcType);
  const auto *srcRgn =
      execSize == g4::SIMD1 ? getRegionScalar() : getRegionStride1();
  G4_Operand *immOp = createImmWithLowerType(addImm64, srcType);
  createInst(duplicateOperand(pred), G4_add, nullptr, g4::NOSAT, execSize, dst,
             src0, immOp, Get_Gen4_Emask(execCtrl, execSize), true);

  return createSrc(result->getRegVar(), 0, 0, srcRgn, srcType);
}

G4_SrcRegRegion *IR_Builder::lscAdd64AosNative(G4_Predicate *pred,
                                               G4_ExecSize execSize,
                                               VISA_EMask_Ctrl execCtrl,
                                               G4_SrcRegRegion *srcReg64,
                                               int64_t addImm64) {
  if (addImm64 == 0)
    return srcReg64;
  // we can assume this is only called on >=PVC (has LSC and DG2 lacks native
  // int64)
  const auto *srcRgn1 =
      execSize == g4::SIMD1 ? getRegionScalar() : getRegionStride1();
  const G4_Type srcType = srcReg64->getType();
  vISA_ASSERT_INPUT(srcType == Type_UQ || srcType == Type_Q,
               "this function only supports Q/UQ types");
  G4_Declare *result = createTempVar(execSize, srcType, getGRFAlign());
  G4_DstRegRegion *dst = createDst(result->getRegVar(), 0, 0, 1, Type_Q);
  vISA_ASSERT_INPUT(addImm64 >= std::numeric_limits<int32_t>::min() &&
                   addImm64 <= std::numeric_limits<int32_t>::max(),
               "offset too big");
  G4_Imm *srcImm = createImm((int32_t)addImm64, Type_D);
  createInst(duplicateOperand(pred), G4_add, nullptr, g4::NOSAT, execSize, dst,
             srcReg64, srcImm, Get_Gen4_Emask(execCtrl, execSize), true);

  return createSrc(result->getRegVar(), 0, 0, srcRgn1, srcReg64->getType());
}

G4_SrcRegRegion *IR_Builder::lscAdd64AosEmu(G4_Predicate *pred,
                                            G4_ExecSize execSize,
                                            VISA_EMask_Ctrl execCtrl,
                                            G4_SrcRegRegion *srcReg64,
                                            int64_t addImm64) {
  if (addImm64 == 0)
    return srcReg64;

  const auto *srcRgn1 =
      execSize == g4::SIMD1 ? getRegionScalar() : getRegionStride1();
  const auto *srcRgn2 =
      execSize == g4::SIMD1 ? getRegionScalar() : getRegionStride2();
  int dstRgnHz2 = execSize == g4::SIMD1 ? 1 : 2;

  const G4_Type srcType = srcReg64->getType();
  vISA_ASSERT_INPUT(srcType == Type_UQ || srcType == Type_Q,
               "this function only supports integer types");

  // Given REG64.K<1;1,0>:q we need to split this into the low and high
  // halves: REG32.(2*K)<2,1,0>:d and REG32.(2*K+1)<2,1,0>:d
  // (scalar gets scalar regions)
  //
  // These are lambdas because we have to extract these regions repeatedly
  // for each pass (walking them forward)
  auto getSrcReg32 = [&](int pass, short evenOdd) {
    // walk the base register forward if the input is vector
    int passRegOff = srcReg64->getRegion()->isScalar() ? 0 : 2 * pass;
    G4_SrcRegRegion *srcReg32 =
        createSrc(srcReg64->getBase(), srcReg64->getRegOff() + passRegOff,
                  srcReg64->getSubRegOff() / 2 + evenOdd, srcRgn2, Type_UD);
    return srcReg32;
  };

  // DST = SRC + IMM64
  // (W) addc (..|M0) TMP0<1>   SRC.0<2>  LO32(imm64)         {AccWrEn}
  // (W) addX (..|M0) TMP1<1>   SRC.1<2>  [HI32(imm64)] acc0
  // (P) mov  (..|MX) DST.0<2>  TMP1.0<1> // mux it back out
  // (P) mov  (..|MX) DST.1<2>  TMP2.0<1>
  G4_Declare *result = createTempVar(execSize, srcType, getGRFAlign());
  //
  VISA_EMask_Ctrl passExecCtrl = execCtrl;
  const G4_ExecSize passExecSize =
      std::min<G4_ExecSize>(execSize, getNativeExecSize());
  const int passes = std::max<int>(1, execSize / getNativeExecSize());
  //
  // shared immediate operands
  G4_Imm *srcImmLo32 = createImm(addImm64 & 0xFFFFFFFF, Type_UD);
  uint32_t hi32Bits = (uint32_t)(addImm64 >> 32);
  G4_Imm *srcImmHi32 = (hi32Bits != 0) ? createImm(hi32Bits, Type_UD) : nullptr;
  //
  for (int pass = 0; pass < passes; pass++) {
    // e.g. someone tries to do a SIMD32 starting at M16
    vISA_ASSERT_INPUT(passExecCtrl != vISA_NUM_EMASK, "invalid exec mask");
    //
    G4_Declare *TMP_LO32 = createTempVar(passExecSize, Type_UD, getGRFAlign());
    G4_DstRegRegion *dstAddcLo =
        createDst(TMP_LO32->getRegVar(), 0, 0, 1, Type_UD);
    G4_SrcRegRegion *srcAddcLo = getSrcReg32(pass, 0);
    G4_INST *addLoInst = createInst(
        duplicateOperand(pred), G4_addc, nullptr, g4::NOSAT, passExecSize,
        dstAddcLo, srcAddcLo, srcImmLo32,
        Get_Gen4_Emask(vISA_EMASK_M1_NM, passExecSize) | InstOpt_AccWrCtrl,
        true);
    G4_DstRegRegion *dstAcc0 =
        createDst(phyregpool.getAcc0Reg(), 0, 0, 1, Type_UD);
    addLoInst->setImplAccDst(dstAcc0);
    //
    G4_Declare *TMP_HI32 = createTempVar(passExecSize, Type_UD, getGRFAlign());
    G4_DstRegRegion *dstAddHi =
        createDst(TMP_HI32->getRegVar(), 0, 0, 1, Type_UD);
    G4_SrcRegRegion *srcAddHi = getSrcReg32(pass, 1);
    G4_SrcRegRegion *srcAcc0 =
        createSrc(phyregpool.getAcc0Reg(), 0, 0, srcRgn1, Type_UD);
    if (srcImmHi32) {
      createInst(duplicateOperand(pred), G4_add3, nullptr, g4::NOSAT,
                 passExecSize, dstAddHi, srcAcc0, srcAddHi, srcImmHi32,
                 Get_Gen4_Emask(vISA_EMASK_M1_NM, passExecSize), true);
    } else {
      createInst(duplicateOperand(pred), G4_add, nullptr, g4::NOSAT,
                 passExecSize, dstAddHi, srcAcc0, srcAddHi,
                 Get_Gen4_Emask(vISA_EMASK_M1_NM, passExecSize), true);
    }
    //
    G4_DstRegRegion *resultLo =
        createDst(result->getRegVar(), 2 * pass, 0, dstRgnHz2, Type_UD);
    G4_SrcRegRegion *tmpLoSrc =
        createSrc(TMP_LO32->getRegVar(), 0, 0, srcRgn1, Type_UD);
    createInst(duplicateOperand(pred), G4_mov, nullptr, g4::NOSAT, passExecSize,
               resultLo, tmpLoSrc, nullptr,
               Get_Gen4_Emask(passExecCtrl, passExecSize), true);
    //
    G4_DstRegRegion *resultHi =
        createDst(result->getRegVar(), 2 * pass, 1, dstRgnHz2, Type_UD);
    G4_SrcRegRegion *tmpHiSrc =
        createSrc(TMP_HI32->getRegVar(), 0, 0, srcRgn1, Type_UD);
    createInst(duplicateOperand(pred), G4_mov, nullptr, g4::NOSAT, passExecSize,
               resultHi, tmpHiSrc, nullptr,
               Get_Gen4_Emask(passExecCtrl, passExecSize), true);
    //
    passExecCtrl = Get_Next_EMask(passExecCtrl, passExecSize);
  }

  return createSrc(result->getRegVar(), 0, 0, srcRgn1, srcReg64->getType());
}

G4_SrcRegRegion *IR_Builder::lscMul64Aos(G4_Predicate *pred,
                                         G4_ExecSize execSize,
                                         VISA_EMask_Ctrl execCtrl,
                                         G4_SrcRegRegion *src0,
                                         int16_t mulImm) {
  if (mulImm == 1)
    return src0;
    vISA_ASSERT_UNREACHABLE("mul64-aos not supported yet");
  return nullptr;
}
