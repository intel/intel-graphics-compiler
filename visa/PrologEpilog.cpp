/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Assertions.h"
#include "FlowGraph.h"
#include "G4_Opcode.h"
#include "G4_Verifier.hpp"
#include "Optimizer.h"
#include "Timer.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>

using namespace vISA;

// A place for all kernel prolog/epilog related code.
// TODO: Currently prolog/epilog code is spread into multiple standlone
// functions with no clear ordering between them. It may be good to have single
// PrologEpilog pass that clearly defines the order for which the different
// instructions are inserted.

// Prolog functions.

// Create a copy of R0 at top of kernel,
// to support midthread preemption.
void Optimizer::createR0Copy() {
  if (!builder.getIsKernel()) {
    return;
  }

  // r0 copy is needed only if:
  // a. pre-emption VISA option is enabled OR
  // b. current object is kernel with stack calls since VISA ABI requires r0
  // copy to be available in a pre-defined register
  if (!R0CopyNeeded())
    return;

  // Skip copying of ``copy of R0'' if it's never assigned, a case where
  // ``copy of R0'' is never used. As EOT always use ``copy of R0'', that
  // case only happens for synthetic tests where no practical code is
  // generated.
  if (!builder.getBuiltinR0()->getRegVar()->isPhyRegAssigned())
    return;

  G4_Declare *R0Dcl = builder.getRealR0();
  G4_SrcRegRegion *R0Opnd =
      builder.createSrcRegRegion(R0Dcl, builder.getRegionStride1());

  G4_DstRegRegion *R0CopyOpnd =
      builder.createDst(builder.getBuiltinR0()->getRegVar(), 0, 0, 1, Type_UD);

  unsigned int options = InstOpt_WriteEnable;
  unsigned numElt = kernel.getGRFSize() / TypeSize(Type_UD);
  G4_INST *movInst = builder.createMov(G4_ExecSize(numElt),
                                       R0CopyOpnd, R0Opnd, options, false);

  for (G4_BB *bb : kernel.fg) {
    INST_LIST_ITER ii = bb->begin();
    INST_LIST_ITER iend = bb->end();
    for (; ii != iend; ii++) {
      G4_INST *inst = *ii;
      if (inst->opcode() != G4_label) {
        bb->insertBefore(ii, movInst);
        return;
      }
    }
  }
}

void Optimizer::initializePayload() {
  if (!kernel.fg.builder->getIsKernel()) {
    return;
  }

  const unsigned grfSize = kernel.getGRFSize();
  unsigned inputEnd = grfSize;
  unsigned inputCount = kernel.fg.builder->getInputCount();
  for (unsigned id = 0; id < inputCount; id++) {
    input_info_t *input_info = kernel.fg.builder->getInputArg(id);
    unsigned argEnd = input_info->offset + input_info->size;
    inputEnd = std::max(inputEnd, argEnd);
  }

  G4_BB *bb = kernel.fg.getEntryBB();
  // iter points to the first non-label inst
  auto iter = bb->begin(), bbEnd = bb->end();
  while (iter != bbEnd) {
    if (!(*iter)->isLabel()) {
      break;
    }
    ++iter;
  }

  const unsigned maxGRFNum = kernel.getNumRegTotal();
  // First full GRF that needs to be initialized
  unsigned regNum = (inputEnd + grfSize - 1) / grfSize;
  // Initialize bulk of GRFs, two at a time
  unsigned numElt = grfSize * 2 / TypeSize(Type_UD);
  while (regNum + 2 <= maxGRFNum) {
    G4_Declare *tempDcl =
        builder.createHardwiredDeclare(numElt, Type_UD, regNum, 0);
    G4_DstRegRegion *dst =
        builder.createDst(tempDcl->getRegVar(), 0, 0, 1, Type_UD);
    G4_Imm *src0 = builder.createImm(0, Type_UD);
    G4_INST *initInst = builder.createMov(G4_ExecSize(numElt), dst, src0,
                                          InstOpt_WriteEnable, false);
    bb->insertBefore(iter, initInst);
    regNum += 2;
  }
  // Initialize the last register if bulk of GRFs was odd
  if (regNum != maxGRFNum) {
    vASSERT(regNum == maxGRFNum - 1);
    numElt = grfSize / TypeSize(Type_UD);
    G4_Declare *tempDcl =
        builder.createHardwiredDeclare(numElt, Type_UD, regNum, 0);
    G4_DstRegRegion *dst =
        builder.createDst(tempDcl->getRegVar(), 0, 0, 1, Type_UD);
    G4_Imm *src0 = builder.createImm(0, Type_UD);
    G4_INST *initInst = builder.createMov(G4_ExecSize(numElt), dst, src0,
                                          InstOpt_WriteEnable, false);
    bb->insertBefore(iter, initInst);
  }

  // The GRF that needs to be partial initialized
  regNum = inputEnd / grfSize;
  // offset within GRF from which to start to initialize
  unsigned subOffset = (inputEnd % grfSize);
  // beginning execution size for byte remainder initialization
  unsigned execSize = grfSize / 2;
  // use an already initialized GRF as src
  unsigned grfSrc = maxGRFNum - 2;
  // inits remainder GRF
  // loops until all bytes within GRF are initialized
  // on each iteration goes down by execution size
  // There was a small bug if inputEnd offset is GRF aligned it would think all
  // of last payload register is the "remainder" and will initialize it.
  while (subOffset && (subOffset != grfSize)) {
    while (subOffset + execSize <= grfSize) {
      G4_Declare *tempDcl =
          builder.createHardwiredDeclare(execSize, Type_UB, regNum, subOffset);
      G4_DstRegRegion *dst =
          builder.createDst(tempDcl->getRegVar(), 0, 0, 1, Type_UB);
      vASSERT(grfSrc > regNum);
      G4_Declare *tempDclSrc =
          builder.createHardwiredDeclare(1, Type_UD, grfSrc , 0);
      G4_SrcRegRegion *src0 = builder.createSrc(
          tempDclSrc->getRegVar(), 0, 0, builder.getRegionScalar(), Type_UB);

      G4_INST *initInst = builder.createMov(G4_ExecSize(execSize), dst, src0,
                                            InstOpt_WriteEnable, false);
      bb->insertBefore(iter, initInst);
      subOffset += execSize;
    }
    // next lowest execution size
    execSize = std::max(1U, execSize / 2);
  }

  // Initializing Flag register
  for (unsigned i = 0, e = builder.getNumFlagRegisters() / 2; i < e; ++i) {
    G4_Declare *tmpFlagDcl = builder.createTempFlag(2);
    tmpFlagDcl->getRegVar()->setPhyReg(builder.phyregpool.getFlagAreg(i), 0);
    G4_DstRegRegion *tempPredVar =
        builder.createDst(tmpFlagDcl->getRegVar(), 0, 0, 1, Type_UD);
    G4_INST *predInst =
        builder.createMov(g4::SIMD1, tempPredVar, builder.createImm(0, Type_UW),
                          InstOpt_WriteEnable, false);
    bb->insertBefore(iter, predInst);
  }
}

// create prolog to set sr0 to FFID. TGL WA.
// Do only when there is cr0 write inside the kernel
void Optimizer::addFFIDProlog() {
  if (!builder.getIsKernel())
    return;

  FFID ffid =
      static_cast<FFID>(builder.getOptions()->getuInt32Option(vISA_setFFID));
  // return if FFID is not given
  if (ffid == FFID_INVALID)
    return;

  // get r127.0 decl
  G4_Declare *rtail = builder.createHardwiredDeclare(
      8, Type_UD, kernel.getNumRegTotal() - 1, 0);

  // (W) and (1|M0)  r127.0 <1>:ud   sr0.0 <0;1,0>:ud  0xF0FFFFFF:ud
  auto createAnd = [this, &rtail]() {
    auto src0 = builder.createSrc(builder.phyregpool.getSr0Reg(), 0, 0,
                                  builder.getRegionScalar(), Type_UD);
    auto src1 = builder.createImm(0xF0FFFFFF, Type_UD);
    auto dst = builder.createDst(rtail->getRegVar(), 0, 0, 1, Type_UD);

    return builder.createBinOp(G4_and, g4::SIMD1, dst, src0, src1,
                               InstOpt_WriteEnable, false);
  };

  // (W) or  (1|M0)  sr0.0<1>:ud   127.0<0;1,0>:ud    imm:ud
  auto createOr = [this, &rtail](uint32_t imm) {
    auto src0 = builder.createSrc(rtail->getRegVar(), 0, 0,
                                  builder.getRegionScalar(), Type_UD);
    auto src1 = builder.createImm(imm, Type_UD);
    auto dst =
        builder.createDst(builder.phyregpool.getSr0Reg(), 0, 0, 1, Type_UD);

    return builder.createBinOp(G4_or, g4::SIMD1, dst, src0, src1,
                               InstOpt_WriteEnable, false);
  };

  // (W) jmpi (1|M0) label
  auto createJmpi = [this](G4_Label *label) {
    return builder.createInternalInst(nullptr, G4_jmpi, nullptr, g4::NOSAT,
                                      g4::SIMD1, nullptr, label, nullptr,
                                      InstOpt_WriteEnable);
  };

  auto createLabelInst = [this](G4_Label *label) {
    return kernel.fg.createNewLabelInst(label);
  };

  // for compute shader, create two entris
  if (ffid == FFID_GP || ffid == FFID_GP1) {
    // Entry0: Set sr0 to FFID_GP (0x7)
    //     (W) and (1|M0)  r127.0 <1>:ud   sr0.0 <0;1,0>:ud   0xF0FFFFFF:ud
    //     (W) or  (1|M0)  sr0.0<1>:ud     127.0<0;1,0>:ud    0x07000000:ud
    //     jmpi ffid_prolog_end
    // Entry1: Set sr0 to FFID_GP1 (0x8)
    //     (W) and (1|M0)  r127.0 <1>:ud   sr0.0 <0;1,0>:ud   0xF0FFFFFF:ud
    //     (W) or  (1|M0)  sr0.0<1>:ud     127.0<0;1,0>:ud    0x08000000:ud
    //     ffid_prolog_end:

    // Put the entry0 block into a new BB, so that we can make it 64-bit
    // aligned in BinaryEncodingIGA
    G4_BB *entry_0_bb = kernel.fg.createNewBB();
    entry_0_bb->push_back(createAnd());
    entry_0_bb->push_back(createOr(0x07000000));

    // get jmp target label. If the next bb has no label, create one and insert
    // it at the beginning
    G4_Label *jmp_label = nullptr;
    vASSERT(kernel.fg.begin() != kernel.fg.end());
    G4_BB *next_bb = *kernel.fg.begin();
    if (next_bb->front()->isLabel()) {
      jmp_label = next_bb->front()->getSrc(0)->asLabel();
    } else {
      jmp_label = builder.createLocalBlockLabel("ffid_prolog_end");
      next_bb->insertBefore(next_bb->begin(), createLabelInst(jmp_label));
    }
    entry_0_bb->push_back(createJmpi(jmp_label));

    // Put the rest in another BB
    G4_BB *entry_1_bb = kernel.fg.createNewBB();
    entry_1_bb->push_back(createAnd());
    entry_1_bb->push_back(createOr(0x08000000));

    // add these two BB to be the first two in the shader
    kernel.fg.addPrologBB(entry_1_bb);
    kernel.fg.addPrologBB(entry_0_bb);
    kernel.setComputeFFIDGPBB(entry_0_bb);
    kernel.setComputeFFIDGP1BB(entry_1_bb);
  } else {
    // for other shaders, set the FFID
    //     (W) and (1|M0)  r127.0 <1>:ud   sr0.0 <0;1,0>:ud   0xF0FFFFFF:ud
    //     (W) or  (1|M0)  sr0.0<1>:ud     127.0<0;1,0>:ud    (FFID << 24):ud
    G4_BB *bb = kernel.fg.createNewBB();
    bb->push_back(createAnd());
    bb->push_back(createOr(ffid << 24));
    kernel.fg.addPrologBB(bb);
  }
}


// clang-format off
///////////////////////////////////////////////////////////////////////////////
// Argument Loading for GPGPU
//
//  Payload in Memory                                Payload in GRF
//    (prepared by runtime)                           (for thread T[i])
//
//   IndirectArgPtr = r0.0[31:6] + GeneralStateBase
//
// As an example, assume per thread data is 3 GRFs (numCrossThreadDW / 16 = 3)
//
//  Memory:                                            Register File:
//
// +---------------------+ <- [IndirectArgPtr e.g. r0.0[31:6]+GeneralStateBase]
// | implicit_args       |
// |  (if enabled)       |
// +---------------------+                         R1 +------------------------+ <-- perThreadLoadStartGRF
// |  cross thread data  | \                          |                        |
// |                     |  numCrossThreadDW          |  per thread data T[i]  |
// | ... [ padding? *  ] | /                          |                        |
// +---------------------+ <-- perThreadOffsetMem  R4 +------------------------+ <- perThreadLoadGRF + numPerThreadGRF
// |                     | \                          | inline data (optional) |
// |  per thread data T0 |    numPerThreadGRF      R5 +------------------------+ <-- crossThreadLoadStartGRF
// |                     | /    (GRFs)                |  cross thread data     |  |
// +---------------------+                            |                        |  numCrossThreadDW (Dwords)
// |                     |                            |                        |  |
// |  per thread data T1 |                            +------------------------+
// |                     |                           (NOTE: register numbers are examples)
// +---------------------+                           vISA_loadThreadPayloadStartReg shifts payload in GRF
//          ...
//
// * inline data comes from the compute walker command not memory;
//   "inline" (or immediate) with respect the command streamer instructions
//
// * padding vISA_crossThreadDataAlignment rounds up cross-thread memory section
//   so that per-thread blocks start aligned; successive per thread blocks are GRF aligned
//
// clang-format on
class PayloadLoader
{
  IR_Builder &builder;
  G4_Kernel &kernel;
  FlowGraph &fg;

  // if the inline data register is being used
  const bool useInlineData;

  // indirect data address is at r0.0[5:31]:d
  // thread id in group is at r0.2[7:0]:d (same as r0.4[7:0]:w)
  G4_Declare *r0;
  // temp register to use for offset computation or load payload
  G4_Declare *rtmp;

  // see the image above
  const uint32_t perThreadLoadStartGRF;

  // final cross thread size to be loaded as number of DW (including aligenment)
  // does not include the inline register argument
  uint32_t numCrossThreadDW = 0;
  // payload memory offset of where local id should be loaded from
  // this is in bytes
  uint32_t perThreadOffsetMem = 0;

  // number of per-thread GRFs to be loaded (e.g. local ids)
  const uint32_t numPerThreadGRF = 0;

  // start GRF for load data
  uint32_t crossThreadLoadStartGRF = 0;

  std::vector<G4_INST *> instBuffer;

public:
  PayloadLoader(IR_Builder &b, G4_Kernel &k, FlowGraph &_fg)
    : builder(b), kernel(k), fg(_fg),
      useInlineData(k.hasInlineData()),
      r0(
        b.createHardwiredDeclare(
          k.numEltPerGRF<Type_UD>(), Type_UD, 0, 0)),
      perThreadLoadStartGRF(
        k.getOptions()->getuInt32Option(vISA_loadThreadPayloadStartReg)),
      numPerThreadGRF(
        AlignUp(k.getInt32KernelAttr(Attributes::ATTR_PerThreadInputSize),
                k.numEltPerGRF<Type_UB>()) / k.numEltPerGRF<Type_UB>())
  {
    auto rtmpRegNum = k.getNumRegTotal() - 1;
    rtmp = b.createHardwiredDeclare(k.numEltPerGRF<Type_UD>(), Type_UD, rtmpRegNum, 0);

    r0->setName("r0");
    rtmp->setName("rtmp");

    // pre-compute various offsets into memory and GRF for the later use
    uint32_t crossThreadLoadStart = 0;    // register file (grf) offset in byte
    // cross thread size (not including inlinedata size and alignement)
    int CTIS = kernel.getInt32KernelAttr(Attributes::ATTR_CrossThreadInputSize);
    if (CTIS < 0) {
      // per-thread payload vars
      // N = inlinedata size
      // Cross thread data size is aligned to 32byte,
      // if inlinedata is used, runtime puts first N bytes of payload in
      // inlinedata. Rest of payload is shifted in the buffer by N bytes. So
      // payload args which start at N offset, now start at 0 offset. Because of
      // this we need to calculate localID offset:
      const unsigned crossThreadDataAlignment =
          builder.getuint32Option(vISA_crossThreadDataAlignment);
      const uint32_t loadedCrossThreadInputSize =
          findCrossThreadInputSize(crossThreadLoadStart);
      const uint32_t inlineDataSize = builder.getInlineDataSize();
      perThreadOffsetMem =
          useInlineData ?
            AlignUp(loadedCrossThreadInputSize + inlineDataSize,
                    crossThreadDataAlignment) - inlineDataSize :
            AlignUp(loadedCrossThreadInputSize, crossThreadDataAlignment);

      // cross-thread payload vars
      numCrossThreadDW =
        AlignUp(loadedCrossThreadInputSize, crossThreadDataAlignment) /
          TypeSize(Type_UD);
      crossThreadLoadStartGRF = crossThreadLoadStart / kernel.getGRFSize();
    } else {
      // per-thread payload vars
      perThreadOffsetMem = CTIS;

      if (useInlineData && builder.getInlineDataSize() >= perThreadOffsetMem)
      {
          perThreadOffsetMem = 0;
      }
      else if (useInlineData)
      {
          perThreadOffsetMem -= builder.getInlineDataSize();
      }

      // cross-thread payload vars
      numCrossThreadDW = CTIS / TypeSize(Type_UD);
      crossThreadLoadStartGRF = perThreadLoadStartGRF + numPerThreadGRF;
      if (useInlineData) {
        // first GRF of cross-thread data is already loaded
        crossThreadLoadStartGRF++;
        // FIXME: reduce "numCrossThreadDW" for grf size instead of inline data
        // size (builder.getInlineDataSize()) to workaround ogl behavior that it
        // sets ATTR_CrossThreadInputSize larger than acutal input size.
        numCrossThreadDW =
          numCrossThreadDW > kernel.numEltPerGRF<Type_UD>() ?
            numCrossThreadDW - kernel.numEltPerGRF<Type_UD>() : 0;
      }
    }
  } // PayloadLoader::PayloadLoader(...)

private:
  // load <numGRF> GRFs from the address "loadAddress", starting from <startGRF>
  // using an oword block load
  void loadFromMemoryHdcBti(G4_Declare *loadAddress,
                            uint32_t startGRF,
                            uint32_t numTotalDW)
  {
    auto getHWordBlockEncoding = [](uint32_t numHW) {
      switch (numHW) {
      case 1:
        return 0x0;
      case 2:
        return 0x1;
      case 4:
        return 0x2;
      case 8:
        return 0x3;
      default:
        vISA_ASSERT_UNREACHABLE("unexpected number of HW");
        return 0x0;
      }
    };

    for (uint32_t numRemainingDW = numTotalDW, nextGRF = startGRF;
         numRemainingDW > 0;
         /* updated in body */)
    {
      // can load 4, 2 or 1 grf per send.
      // Still load 1 GRF if the remainingDW is less than 1 GRF. The addtional
      // bytes those being loaded won't be used.
      uint32_t DWin4GRF = 4 * builder.numEltPerGRF<Type_UD>();
      uint32_t DWin2GRF = DWin4GRF / 2;
      uint32_t DWin1GRF = DWin2GRF / 2;
      uint32_t numGRFToLoad = numRemainingDW >= DWin4GRF ? 4 : // 4 GRF
                                  numRemainingDW >= DWin2GRF ? 2 : // 2 GRF
                                  1; // 1 GRF or less than 1 GRF

      bool useHword = builder.hasHWordBlockLoad();
      uint32_t numElts =
          (numGRFToLoad * kernel.getGRFSize()) / (useHword ? 32 : 16);
      uint32_t dataBlocks = useHword
                                ? getHWordBlockEncoding(numElts)
                                : (numElts == 2 ? 2 : (numElts == 4 ? 3 : 4));

      // A32 unaligned hword/oword block read
      uint32_t msgDescVal = (1 << 25) | (numGRFToLoad << 20) | (1 << 19) |
                            (DC_ALIGNED_OWORD_BLOCK_READ << 14) |
                            ((useHword ? 1 : 0) << 13) | (dataBlocks << 8) |
                            253;
      auto desc = builder.createReadMsgDesc(SFID::DP_DC0, msgDescVal);
      auto sendSrc =
          builder.createSrcRegRegion(loadAddress, builder.getRegionStride1());
      auto sendDstDcl =
          builder.createHardwiredDeclare(numGRFToLoad * 8, Type_UD, nextGRF, 0);
      auto sendDst = builder.createDstRegRegion(sendDstDcl, 1);
      auto sendInst =
          builder.createSendInst(nullptr, G4_send, g4::SIMD8, sendDst, sendSrc,
                                 builder.createImm(msgDescVal, Type_UD),
                                 InstOpt_WriteEnable | InstOpt_NoCompact, desc,
                                 true);
      instBuffer.push_back(sendInst);
      if (numRemainingDW < DWin1GRF)
        break;
      numRemainingDW -= numGRFToLoad * builder.numEltPerGRF<Type_UD>();
      nextGRF += numGRFToLoad;
      if (numRemainingDW > 0) {
        // advance the address offset
        // (W) add (1) loadAddress.2 loadAddress.2 numGRFToLoad*sizeof(GRF)
        auto addSrc0 = builder.createSrc(loadAddress->getRegVar(), 0, 2,
                                         builder.getRegionScalar(), Type_UD);
        auto addSrc1 = builder.createImm(
          numGRFToLoad * kernel.numEltPerGRF<Type_UB>(), Type_UW);
        auto addDst =
          builder.createDst(loadAddress->getRegVar(), 0, 2, 1, Type_UD);
        auto addInst =
          builder.createBinOp(G4_add, g4::SIMD1,
                              addDst, addSrc0, addSrc1,
                              InstOpt_WriteEnable | InstOpt_NoCompact, false);
        instBuffer.push_back(addInst);
      }
    }
  } // loadFromMemoryHdcBti


  // a helper function LSC loads to get the max DW number which can
  // fulfill LSC element number;
  //  - this rounds down to a GRF, or
  //  - up to a legal vector size (e.g. 5 -> 8)
  uint32_t roundDwordsToLegalSize(uint32_t numDW) const {
    if (builder.lscGetElementNum(numDW) != LSC_DATA_ELEMS_INVALID)
      return numDW;
    if (numDW > builder.numEltPerGRF<Type_UD>()) {
      if (numDW > 64)
        return (uint32_t)64;
      else if (numDW > 32)
        return (uint32_t)32;
      else if (numDW > 16)
        return (uint32_t)16;
      else if (numDW > 8)
        return (uint32_t)8;
      vISA_ASSERT_UNREACHABLE("unreachable");
    }
    // when the numDW is less than 1 grf, we want to load all within one send
    // The additional bytes being loaded won't be used so should be fine
    if (numDW < 2)
      return (uint32_t)2;
    else if (numDW < 4)
      return (uint32_t)4;
    else if (numDW < 8)
      return (uint32_t)8;
    else if (numDW < 16)
      return (uint32_t)16;
    vISA_ASSERT_UNREACHABLE("unreachable");
    return (uint32_t)0;
  }

  // LSC allows transpose with V1, V2, V3, V4, V8, V16, V32, V64
  // We assume this is called in descending sequence with register-sized
  // chunks and then on down to sub register size.
  //
  // Only the last load in a sequence may be smaller than a GRF and must
  // round up.
  //  DWords:
  //    >=64    => d32x64t   possible residue of next iteration
  //    32-63   => d32x32t   possible residue of next iteration
  //    17-31   => d32x16t   possible residue of next iteration
  //  Final Load Residues:
  //    9-16    => d32x16t   loads some padding
  //    5-8     => d32x8t    loads some padding
  //    4,3,2,1 => d32x{4,3,2,1}t
  //
  //   Thus, given V7 we need to load V8
  //
  uint32_t roundDwordsToLegalSizeLSC(uint32_t numDw) {
    if (numDw >= 64) {
      return 64; // 4GRF
    } else if (numDw >= 32) {
      return 32; // 2GRF
    } else if (numDw > 8) {
      return 16; // 1GRF (possibly padding)
    } else if (numDw > 4) {
      return 8; // half a GRF (possibly padding)
    } else { // V1, V2, V3, V4
      return numDw;
    }
  }

  void loadFromMemoryLscBti(G4_Declare *baseLoadAddr,
                            uint32_t startGRF,
                            uint32_t numTotalDW)
  {
    G4_Declare *loadAddress = baseLoadAddr;
    // Use immediate offsets to avoid the adds.
    const uint32_t immOffOpts =
        builder.getuint32Option(vISA_lscEnableImmOffsFor);
    const bool useLscImmOff =
        // HW supports it
        builder.getPlatform() >= Xe2 &&
        //
        // BTI only gets 12b of range (signed+DW aligned) ~ 31 GRF
        (numTotalDW * TypeSize(Type_UD)) <= ((1 << 11) - 4) &&
        //
        // enabled in options
        ((immOffOpts & (1 << VISA_LSC_IMMOFF_PAYLOAD_LOADING)) != 0) &&
        //
        // the payload address type is also enabled in options
        (immOffOpts & (1 << getLscImmOffOpt(LSC_ADDR_TYPE_BTI))) != 0;
    for (uint32_t numRemainingDW = numTotalDW, nextGRF = startGRF;
         numRemainingDW > 0;
         /* updated in body */) {
      // Generate a A32 tranpose LSC load to BTI 255. size is d32x{16/32}t
      LSC_OP op = LSC_LOAD;
      LSC_SFID lscSfid = LSC_UGM;
      LSC_CACHE_OPTS cacheOpts{LSC_CACHING_CACHED, LSC_CACHING_CACHED};
      if (builder.getPlatformGeneration() >= PlatformGen::XE2) {
        // use XE2+ L3 CC
        cacheOpts = {LSC_CACHING_CACHED, LSC_CACHING_CONSTCACHED};
      }

      LSC_ADDR addrInfo{};
      addrInfo.type = LSC_ADDR_TYPE_BTI;
      addrInfo.size = LSC_ADDR_SIZE_32b;
      addrInfo.immScale = 1;
      addrInfo.immOffset = 0;
      if (useLscImmOff) {
        addrInfo.immOffset =
            ((int)nextGRF - startGRF) * (int)kernel.getGRFSize();
      }

      LSC_DATA_SHAPE dataShape{};
      dataShape.size = LSC_DATA_SIZE_32b; // in the unit of 32b
      dataShape.order = LSC_DATA_ORDER_TRANSPOSE;
      uint32_t numDWToLoad = roundDwordsToLegalSize(numRemainingDW);
      dataShape.elems = builder.lscGetElementNum(numDWToLoad);

      G4_Imm *surfaceBTI = builder.createImm(255, Type_UW);

      auto sendDstDcl =
          builder.createHardwiredDeclare(numDWToLoad, Type_UD, nextGRF, 0);
      auto dstRead = builder.createDstRegRegion(sendDstDcl, 1);
      auto src0Addr = builder.createSrcRegRegion(
          loadAddress, builder.getRegionStride1()); // address base

      G4_InstSend *sendInst = nullptr;
      G4_SendDescRaw *desc = builder.createLscMsgDesc(
          op, lscSfid, EXEC_SIZE_1, cacheOpts, addrInfo, dataShape, surfaceBTI,
          numDWToLoad < builder.numEltPerGRF<Type_UD>()
              ? 1
              : numDWToLoad / builder.numEltPerGRF<Type_UD>(),
          1, LdStAttrs::NONE);

      sendInst =
        builder.createLscSendInst(nullptr, dstRead, src0Addr, nullptr,
                                  g4::SIMD1, desc,
                                  InstOpt_WriteEnable | InstOpt_NoCompact,
                                  LSC_ADDR_TYPE_BTI, 0x0, true);
      instBuffer.push_back(sendInst);
      // we pick to load all data within one send in
      // roundDwordsToLegalSize if numRemainingDW is less than one
      // grf. All should be loaded at this point.
      if (numRemainingDW < builder.numEltPerGRF<Type_UD>())
        break;
      numRemainingDW -= numDWToLoad;
      nextGRF += numDWToLoad / builder.numEltPerGRF<Type_UD>();
      bool advanceLoadAddress = numRemainingDW > 0;
      advanceLoadAddress &= !useLscImmOff;
      if (advanceLoadAddress) {
        // advance the address offset
        // (W) add (1) loadAddress.0 baseLoadAddr.0 numGRFLoadedInBytes
        auto addSrc0 = builder.createSrcRegRegion(
            baseLoadAddr, builder.getRegionScalar());
        auto addSrc1 = builder.createImm(
            (nextGRF - startGRF) * kernel.getGRFSize(), Type_UW);
        vASSERT(loadAddress->getRegVar()->isPhyRegAssigned() &&
                loadAddress->getRegVar()->getPhyReg()->isPhyGreg());
        // Use different GRF for the subsequent load address computation to
        // mitigate WAR stall on prev send src. Use the address GRF - 1 from
        // the current load for the next one here, and fallback to use the last
        // GRF when it conflicts with input.
        // TODO: Consider moving prolog emission before local schedule or do
        // hand schedule to hide the RAW dependence of send on the address GRF.
        unsigned rTmpAddDst =
            loadAddress->getRegVar()->getPhyReg()->asGreg()->getRegNum() - 1;
        if (nextGRF * kernel.numEltPerGRF<Type_UD>() + numRemainingDW >
            rTmpAddDst * kernel.numEltPerGRF<Type_UD>()) {
          loadAddress = baseLoadAddr;
        } else {
          loadAddress =
              builder.createHardwiredDeclare(1, Type_UD, rTmpAddDst, 0);
        }
        auto addDst = builder.createDstRegRegion(loadAddress, 1);
        auto addInst =
            builder.createBinOp(G4_add, g4::SIMD1,
                                addDst, addSrc0, addSrc1,
                                InstOpt_WriteEnable | InstOpt_NoCompact,
                                false);
        instBuffer.push_back(addInst);
      }
    }
  } // loadFromMemoryLscBti

  void loadFromMemory(G4_Declare *loadAddress,
                      uint32_t startGRF,
                      uint32_t numTotalDW)
  {
    // Need to reserve 1 GRF for offset computation or load payload at least.
    vISA_ASSERT(numTotalDW == 0 ||
        (startGRF + (numTotalDW + kernel.numEltPerGRF<Type_UD>() - 1) /
            kernel.numEltPerGRF<Type_UD>()) < (kernel.getNumRegTotal() - 1),
        "The payload exceeds GRF capacity.");
    if (builder.useLSCForPayloadLoad()) {
      loadFromMemoryLscBti(loadAddress, startGRF, numTotalDW);
    } else {
      loadFromMemoryHdcBti(loadAddress, startGRF, numTotalDW);
    }
  }

  // add (1) rtmp.2<1>:ud rtmp.2<0;1,0>:ud <reloc imm>
  void emitRelocAddInst(int subreg) {
    auto dst = builder.createDst(rtmp->getRegVar(), 0, subreg, 1, Type_UD);
    auto src0 = builder.createSrc(rtmp->getRegVar(), 0, subreg,
                                  builder.getRegionScalar(), Type_UD);
    auto src1 =
      builder.createRelocImm(GenRelocType::R_SYM_ADDR_32,
                             CROSS_THREAD_OFF_R0_RELOCATION_NAME, 0, Type_UD);
    auto addInst =
      builder.createBinOp(G4_add, g4::SIMD1, dst, src0, src1,
                          InstOpt_WriteEnable | InstOpt_NoCompact, false);
    RelocationEntry::createRelocation(builder.kernel, *addInst, 1,
                                      CROSS_THREAD_OFF_R0_RELOCATION_NAME,
                                      GenRelocType::R_SYM_ADDR_32);
    instBuffer.push_back(addInst);
  }

  // helper function to find the size of cross thread data which needs to be
  // loaded
  //  * loadStartOffset - in this parameter we put the offset of first
  //                      cross thread input which gets loaded.
  //  * returns the size of the cross thread section that must be loaded
  uint32_t findCrossThreadInputSize(uint32_t &loadStartOffset) const {
    const uint32_t startGRF =
        kernel.getOptions()->getuInt32Option(vISA_loadThreadPayloadStartReg);
    const uint32_t inputsStart = startGRF * kernel.getGRFSize();
    const uint32_t inputCount = kernel.fg.builder->getInputCount();

    const int PTIS =
        AlignUp(kernel.getInt32KernelAttr(Attributes::ATTR_PerThreadInputSize),
                kernel.getGRFSize());
    const uint32_t inlineDataSize = builder.getInlineDataSize();

    // Checks if input_info is cross-thread-input
    auto isInCrossThreadData = [&](const input_info_t *const input_info) {
      return (uint32_t)input_info->offset >= inputsStart + PTIS;
    };

    // Checks if input_info fits in inlineData
    auto isInInlineData = [&](const input_info_t *const input_info) {
      if (!useInlineData) {
        return false;
      }
      uint32_t inputEnd = input_info->offset + input_info->size;
      bool fitsInInlineData = inputEnd <= inputsStart + PTIS + inlineDataSize;
      return isInCrossThreadData(input_info) && fitsInInlineData;
    };

    uint32_t firstNotInlinedCrossThreadInput =
        std::numeric_limits<uint32_t>::max();
    uint32_t inputEnd = 32;

    // iterate over inputs and find:
    // - where they end
    // - where first not inlined cross thread input is
    for (unsigned int id = 0; id < inputCount; id++) {
      const input_info_t *input_info = kernel.fg.builder->getInputArg(id);
      // skip pseudo input for register bindings.
      if (input_info->isPseudoInput()) {
        continue;
      }
      if (kernel.fg.builder->getFCPatchInfo()->getIsEntryKernel()) {
        const vISA::G4_Declare *dcl = input_info->dcl;
        if (INPUT_GENERAL == input_info->getInputClass() && !(dcl->isLiveIn())) {
          break;
        }
      }
      if (inputEnd < (unsigned)(input_info->size + input_info->offset)) {
        inputEnd = input_info->size + input_info->offset;
      }
      // let's find first cross thread input position which is not delivered in
      // inlineData
      if (isInCrossThreadData(input_info) && !isInInlineData(input_info) &&
          firstNotInlinedCrossThreadInput > (uint32_t)input_info->offset) {
        firstNotInlinedCrossThreadInput = input_info->offset;
      }
    }

    loadStartOffset = firstNotInlinedCrossThreadInput;
    // check if we have anything to load
    if (firstNotInlinedCrossThreadInput == std::numeric_limits<uint32_t>::max()) {
      return 0;
    }
    return inputEnd - firstNotInlinedCrossThreadInput;
  } // findCrossThreadInputSize

  // (W) and (1) rtmp.2<1>:ud r0.0<0;1,0>:ud 0xFFFFFFC0
  void getStartAddrInst(int subreg) {
    auto src0 = builder.createSrc(r0->getRegVar(), 0, 0,
                                  builder.getRegionScalar(), Type_UD);
    const uint32_t ArgOffsetMask = 0xFFFFFFC0;
    auto src1 = builder.createImm(ArgOffsetMask, Type_UD);
    auto dst = builder.createDst(rtmp->getRegVar(), 0, subreg, 1, Type_UD);
    auto andInst = builder.createBinOp(G4_and, g4::SIMD1, dst, src0, src1,
                                       InstOpt_WriteEnable | InstOpt_NoCompact,
                                       false);
    instBuffer.push_back(andInst);
  }

  // (W) mov (ExecSize) rtmp.0:ud 0x0
  void clearTmpRegister() {
    auto src0 = builder.createImm(0, Type_UD);
    auto dst = builder.createDstRegRegion(rtmp, 1);
    G4_ExecSize execSize(kernel.getGRFSize() / 4);
    auto movInst =
      builder.createMov(execSize, dst, src0,
                        InstOpt_WriteEnable | InstOpt_NoCompact, false);
    instBuffer.push_back(movInst);
  };

  // (W) mov (NumDwords) dstGRF:ud srcGRF:ud
  //
  // Moves the inline argument GRF
  void emitMovInlineData(int dstGRF, int srcGRF, uint32_t numDWord) {
    if (dstGRF == srcGRF) {
      return;
    }
    G4_Declare *srcDcl =
        builder.createHardwiredDeclare(numDWord, Type_UD, srcGRF, 0);
    srcDcl->setName("inlineRegFromTDL");
    G4_Declare *dstDcl =
        builder.createHardwiredDeclare(numDWord, Type_UD, dstGRF, 0);
    dstDcl->setName("inlineRegExpectedLocation");
    auto movInst =
      builder.createMov(
        G4_ExecSize(numDWord), builder.createDstRegRegion(dstDcl, 1),
        builder.createSrcRegRegion(srcDcl, builder.getRegionStride1()),
        InstOpt_WriteEnable | InstOpt_NoCompact, false);
    instBuffer.push_back(movInst);
  }

  void appendLabel(const char *label) {
    G4_INST *lbl =
      kernel.fg.createNewLabelInst(builder.createLabel(label, LABEL_BLOCK));
    instBuffer.push_back(lbl);
  }

public:
  // preparation of thread payload size and start offsets
  void emitLoadSequence()
  {
    // the subregister that the header takes the address from is
    // addr.2:d for OWord block load and addr.0:d for LSC
    const int addrSubreg = builder.useLSCForPayloadLoad() ? 0 : 2;

    G4_BB *perThreadBB = nullptr;
    // Load per-thread data, if any. Per-thread data always start from r1
    // this is a fixed size 8 inst (nop padded as necessary), which may be skipped
    // by runtime if the local_id are auto-generated by HW.
    //
    // The size of this first block must be a multiple of 64B so that the
    // forward start label is 64B aligned.
    if (builder.needsToLoadLocalID()) {
      appendLabel("per_thread_prolog");

      // compute per-thread starting address into (rtmp.2)
      // (W) mov (ExecSize) rtmp.0:ud 0x0
      // (W) and (1) rtmp.2<1>:ud r0.0<0;1,0>:ud 0xFFFFFFC0   // start address
      // (W) and (1) rtmp.0:uw r0.4:uw(tid) 0xFF  // tid
      // (W) add (1) rtmp.2 rtmp.2 cross_thread_size
      // (W) mad (1) rtmp.2 rtmp.2 rtmp.0 per_thread_size

      clearTmpRegister();

      getStartAddrInst(2);

      // (W) and (1) rtmp.0:uw r0.4:uw(tid) 0xFF  // tid
      auto andSrc0 = builder.createSrc(r0->getRegVar(), 0, 4,
                                       builder.getRegionScalar(), Type_UW);
      auto andSrc1 = builder.createImm(0xFF, Type_UW);
      auto andDst = builder.createDst(rtmp->getRegVar(), 0, 0, 1, Type_UW);
      auto andInst =
        builder.createBinOp(G4_and, g4::SIMD1, andDst, andSrc0, andSrc1,
                            InstOpt_WriteEnable | InstOpt_NoCompact, false);
      instBuffer.push_back(andInst);

      // (W) add (1) rtmp.2 rtmp.2 cross_thread_size
      auto addSrc0 = builder.createSrc(rtmp->getRegVar(), 0, 2,
                                       builder.getRegionScalar(), Type_UD);
      // create a relocation for cross_thread_size (per_thread_payload_offset). In
      // case of the cross_thread_size is changed after compilation (e.g. gtpin
      // inserted argument), the relocation need to be resolved to the new
      // cross_thread_size.
      G4_Operand *addSrc1 =
          builder.createRelocImm(GenRelocType::R_SYM_ADDR_32,
              PER_THREAD_OFF_RELOCATION_NAME, perThreadOffsetMem, Type_UD);
      auto addDst = builder.createDst(rtmp->getRegVar(), 0, 2, 1, Type_UD);
      // instruction has relocation must not be compacted
      auto addInst =
          builder.createBinOp(G4_add, g4::SIMD1, addDst, addSrc0, addSrc1,
                              InstOpt_WriteEnable | InstOpt_NoCompact, false);
#if 0
      // disable the relocation entry that gtpin is able to recognize the
      // instruction pattern and doesn't rely on this relocation. We still mark
      // addSrc1 as RelocImm (so relocation name is printed in vISA dump), but
      // the relocation entry won't be emitted to zebin
      RelocationEntry::createRelocation(builder.kernel, *addInst, 1,
                                        PER_THREAD_OFF_RELOCATION_NAME,
                                        GenRelocType::R_SYM_ADDR_32);
#endif
      instBuffer.push_back(addInst);

      if (kernel.getOption(vISA_emitCrossThreadOffR0Reloc)) {
        // per thread payload is stored after cross thread
        // payload in memory. when implicit arg buffer
        // pointer is present, we need to shift load address
        // of per thread payload as well.
        emitRelocAddInst(2);
      }

      // (W) mad (1) rtmp.2 rtmp.2 rtmp.0 per_thread_size
      auto madSrc0 = builder.createSrc(rtmp->getRegVar(), 0, 2,
                                       builder.getRegionScalar(), Type_UD);
      auto madSrc1 = builder.createSrc(rtmp->getRegVar(), 0, 0,
                                       builder.getRegionScalar(), Type_UW);
      auto madSrc2 = builder.createImm(
          numPerThreadGRF * kernel.numEltPerGRF<Type_UB>(), Type_UW);
      auto madDst =
          builder.createDst(rtmp->getRegVar(), 0, addrSubreg, 1, Type_UD);
      auto madInst = builder.createInternalInst(
          nullptr, G4_mad, nullptr, g4::NOSAT, g4::SIMD1, madDst, madSrc0,
          madSrc1, madSrc2, InstOpt_WriteEnable | InstOpt_NoCompact);
      instBuffer.push_back(madInst);

      if (builder.getOption(vISA_useInlineData)) {
        // copy inline data to the first GRF of cross-thread-data
        // e.g. (W) mov (8) inlineDataReg.0:ud r1.0:ud
        // Inline data size is 8 DWords.

        emitMovInlineData(perThreadLoadStartGRF + numPerThreadGRF,
                          perThreadLoadStartGRF,
                          builder.getInlineDataSize()/TypeSize(Type_UD));
      }

      loadFromMemory(rtmp, perThreadLoadStartGRF,
                     numPerThreadGRF * builder.numEltPerGRF<Type_UD>());

      perThreadBB = kernel.fg.createNewBB();
      std::for_each(instBuffer.begin(), instBuffer.end(),
                    [](G4_INST *inst) { inst->invalidateVISAId(); });
      perThreadBB->insert(perThreadBB->begin(), instBuffer.begin(),
                          instBuffer.end());
      instBuffer.clear();

      kernel.setPerThreadPayloadBB(perThreadBB);
    } // builder.needsToLoadLocalID()

    // code for loading the cross-thread data
    if (builder.needsToLoadCrossThreadConstantData()) {
      G4_BB *crossThreadBB = kernel.fg.createNewBB();

      appendLabel("cross_thread_prolog");
      if (!builder.useLSCForPayloadLoad()) {
        // we must clear rtmp again as the per-thread loading code may not be
        // executed
        clearTmpRegister();
      }

      getStartAddrInst(addrSubreg);

      if (kernel.getOption(vISA_emitCrossThreadOffR0Reloc)) {
        // emit add with relocatable imm operand.
        // when this is true, runtime loads global
        // state buffer in r0.0[5:31]. kernel cross
        // thread data is written in some other
        // memory location. runtime is required to
        // patch this relocatable immediate operand
        // to allow correct loading of cross thread
        // data.
        emitRelocAddInst(addrSubreg);
      }

      // based on discussions with OCL runtime team, the first GRF
      // of the cross-thread data will be loaded automatically as the inline data,
      // and it will be either at R1 (if local id is not auto-generated) or
      // R1 + sizeof(local id) (if local id is auto-generated).
      loadFromMemory(rtmp, crossThreadLoadStartGRF, numCrossThreadDW);

      std::for_each(instBuffer.begin(), instBuffer.end(),
                    [](G4_INST *inst) { inst->invalidateVISAId(); });

      // create separate blocks instead of directly inserting to the old entryBB
      // This is for the situation where the entry BB is part of a loop, as we
      // don't want the prolog to be executed multiple times
      crossThreadBB->insert(crossThreadBB->begin(), instBuffer.begin(),
                            instBuffer.end());
      instBuffer.clear();

      kernel.fg.addPrologBB(crossThreadBB);

      kernel.setCrossThreadPayloadBB(crossThreadBB);
    }

    if (perThreadBB) {
      kernel.fg.addPrologBB(perThreadBB);
    }
  } // emitLoadSequence

}; // class PayloadLoader


void Optimizer::loadThreadPayload() {
  if (!builder.loadThreadPayload() || !builder.getIsKernel()) {
    return;
  }
  PayloadLoader pl {builder, kernel, fg};
  pl.emitLoadSequence();
}

// Some platforms require that the first instruction of any kernel should have
// non-zero emask, i.e. emask != 0 by setting MaskCtrl bit to 1: WriteEnable
// (NoMask)
//
// This can be done by introducing a dummy instruction for example:
//   (W) mov(1) null:ud 0x0:ud
void Optimizer::addEmaskSetupProlog() {
  if (!builder.needEmaskSetupProlog())
    return;

  // Only apply the WA to the kernel which is the actual entry point.
  if (!builder.getIsKernel())
    return;

  // When the kernel has no prolog and the first inst has zero emask, insert
  // a dummy WA inst with WriteEnable.
  G4_BB *entry = kernel.fg.getEntryBB();
  if (!entry)
    return;

  G4_INST *first = entry->getFirstInst();
  if (first && !first->isWriteEnableInst()) {
    G4_BB *bb = kernel.fg.createNewBB();
    G4_INST *mov = builder.createMov(g4::SIMD1, builder.createNullDst(Type_UD),
                                     builder.createImm(0, Type_UD),
                                     InstOpt_WriteEnable, false);
    bb->push_back(mov);
    kernel.fg.addPrologBB(bb);
  }
}

// some platform/shaders require a memory fence at kernel entry
// this needs to be called before RA since fence may have a (dummy) destination.
void Optimizer::insertFenceAtEntry() {
  // for scalar path option was used and is still used
  bool injectEntryFences = builder.getOption(vISA_InjectEntryFences);
  // for vector path this option is the same as vISA_LSC_BackupMode
  // and that option is, in turn, same as the value in WA table
  if (kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_CM) {
    injectEntryFences = injectEntryFences ||
                        builder.getOption(vISA_LSCBackupMode) ||
                        VISA_WA_CHECK(builder.getPWaTable(), Wa_14010198302);
    const_cast<Options *>(builder.getOptions())
        ->setOption(vISA_LSCBackupMode, injectEntryFences);
  }

  if (injectEntryFences) {
    auto entryBB = kernel.fg.getEntryBB();
    auto iter = std::find_if(entryBB->begin(), entryBB->end(),
                             [](G4_INST *inst) { return !inst->isLabel(); });

    builder.instList.clear();
    builder.translateLscFence(nullptr, SFID::UGM, LSC_FENCE_OP_EVICT,
                              LSC_SCOPE_GPU);
    // according to architects the invalidate fence should not use backup mode
    const_cast<Options *>(builder.getOptions())
        ->setOption(vISA_LSCBackupMode, false);
    builder.translateLscFence(nullptr, SFID::UGM, LSC_FENCE_OP_INVALIDATE,
                              LSC_SCOPE_GPU);
    const_cast<Options *>(builder.getOptions())
        ->setOption(vISA_LSCBackupMode, true);
    entryBB->insert(iter, builder.instList.begin(), builder.instList.end());
    builder.instList.clear();
  }
}

// Reset A0 to 0 at the beginning of the shader if the shader use VxH a0
void Optimizer::resetA0() {
  // check all instructions to see if VxH a0 src is used
  // only reset A0 when it's used
  bool hasA0 = false;
  for (auto bb : kernel.fg) {
    for (auto inst : *bb) {
      // VxH must be in src0
      if (inst->getSrc(0) && inst->getSrc(0)->isSrcRegRegion() &&
          inst->getSrc(0)->asSrcRegRegion()->isIndirect() &&
          inst->getSrc(0)->asSrcRegRegion()->getRegion()->isRegionWH()) {
        hasA0 = true;
        break;
      }
    }
    if (hasA0)
      break;
  }

  if (!hasA0)
    return;

  // insert "mov (16) a0.0:uw 0x0:uw" at the beginning of the shader
  if (kernel.fg.begin() != kernel.fg.end()) {
    G4_BB *bb = *kernel.fg.begin();
    auto insertIt = std::find_if(
        bb->begin(), bb->end(), [](G4_INST *inst) { return !inst->isLabel(); });
    if (builder.supportNativeSIMD32()) {
      bb->insertBefore(
          insertIt,
          builder.createMov(G4_ExecSize(16),
                            builder.createDst(builder.phyregpool.getAddrReg(),
                                              0, 0, 1, Type_UW),
                            builder.createImm(0, Type_UW), InstOpt_WriteEnable,
                            false));
      bb->insertBefore(
          insertIt,
          builder.createMov(G4_ExecSize(16),
                            builder.createDst(builder.phyregpool.getAddrReg(),
                                              0, 16, 1, Type_UW),
                            builder.createImm(0, Type_UW), InstOpt_WriteEnable,
                            false));
    } else {
      bb->insertBefore(
          insertIt,
          builder.createMov(G4_ExecSize(builder.getNumAddrRegisters()),
                            builder.createDst(builder.phyregpool.getAddrReg(),
                                              0, 0, 1, Type_UW),
                            builder.createImm(0, Type_UW), InstOpt_WriteEnable,
                            false));
    }
  }
}

// Epilog functions.

// some platform/shaders require a memory fence before the end of thread
// ToDo: add fence only when the writes can reach EOT without a fence in between
void Optimizer::insertFenceBeforeEOT() {
  // If vISA_removeFence is set, try to remove fence on UGM if there
  // is no write to UGM in the entire kernel.
  const bool toRemoveFence = builder.getOption(vISA_removeFence);
  bool needLscUgmFence = false; // true if fence is needed.
  // for scalar path option was used and is still used
  bool clearHdcWritesLSCUGM =
      builder.getOption(vISA_clearLSCUGMWritesBeforeEOT);
  bool clearHDCWritesBeforeEOT =
      builder.getOption(vISA_clearHDCWritesBeforeEOT);
  bool clearWritesBeforeEOT = builder.needBarrierWA() && builder.supportsLSC();
  // for vector path we need this WA always, so just use table
  if (kernel.getInt32KernelAttr(Attributes::ATTR_Target) == VISA_CM) {
    clearHDCWritesBeforeEOT =
        clearHDCWritesBeforeEOT ||
        VISA_WA_CHECK(builder.getPWaTable(), Wa_1807084924);
    clearHdcWritesLSCUGM = clearHdcWritesLSCUGM ||
                           VISA_WA_CHECK(builder.getPWaTable(), Wa_22013689345);
  }
  if (!toRemoveFence && !clearHDCWritesBeforeEOT &&
      !(builder.supportsLSC() && clearHdcWritesLSCUGM) &&
      !clearWritesBeforeEOT) {
    return;
  }

  if (!kernel.fg.builder->getIsKernel()) {
    // we dont allow a function to exit
    return;
  }

  bool hasUAVWrites = false;
  bool hasSLMWrites = false;
  bool hasTypedWrites = false;
  bool hasWrites = false;
  std::list<std::pair<G4_BB *, G4_INST *>> toBeRemoved;

  for (auto bb : kernel.fg) {
    if (bb->isEndWithFCall()) {
      // conservatively assume we need a fence
      // ToDo: we don't need a SLM fence if kernel doesnt use SLM, since
      // function can't allocate SLM on its own We can move this W/A to IGC for
      // more precise analysis
      hasUAVWrites = true;
      hasSLMWrites = true;
      hasTypedWrites = true;
      hasWrites = true;
      break;
    }

    for (auto inst : *bb) {
      if (inst->isSend() && !inst->isEOT()) {
        auto msgDesc = inst->asSendInst()->getMsgDesc();
        if (msgDesc->isLSC()) {
          if (toRemoveFence && msgDesc->getSFID() == SFID::UGM &&
              msgDesc->isFence()) {
            toBeRemoved.push_back(std::make_pair(bb, inst));
          }
        }
        // Skip fence (fence is both write/read)
        if (msgDesc->isFence()) {
          continue;
        }

        if (msgDesc->isWrite()) {
          hasWrites = true;
          if (msgDesc->isHDC()) {
            if (msgDesc->isSLM()) {
              hasSLMWrites = true;
            } else if (msgDesc->isRaw() && ((const G4_SendDescRaw *)msgDesc)
                                               ->isHdcTypedSurfaceWrite()) {
              hasTypedWrites = true;
            } else {
              hasUAVWrites = true;
              if (builder.supportsLSC() && clearHdcWritesLSCUGM &&
                  !msgDesc->isScratch()) {
                // Those HDC msg will go thru LSC, so need wa too.
                needLscUgmFence = true;
              }
            }
          }

          if (msgDesc->isLSC()) {
            switch (msgDesc->getSFID()) {
            case SFID::UGM: {
              hasUAVWrites = true;
              if (clearHdcWritesLSCUGM) {
                if ((msgDesc->isAtomic() && !msgDesc->isRead()) || // case 1
                    (!msgDesc->isAtomic() &&                       // case 2
                     !msgDesc->isScratchWrite() &&
                     !(msgDesc->getCachingL1() == Caching::WB ||
                       msgDesc->getCachingL1() == Caching::ST))) {
                  needLscUgmFence = true;
                }
              }
              break;
            }
            case SFID::SLM:
              hasSLMWrites = true;
              break;
            case SFID::TGM:
              hasTypedWrites = true;
              break;
            default:
              break; // ignore other SFID
            }
          }
        }
      }
    }
  }

  if (toRemoveFence && !toBeRemoved.empty() && !hasUAVWrites) {
    for (const auto &II : toBeRemoved) {
      G4_BB *aBB = II.first;
      G4_INST *aInst = II.second;
      aBB->remove(aInst);
    }
    toBeRemoved.clear();
  }

  if ((!clearHDCWritesBeforeEOT &&
       !(builder.supportsLSC() && clearHdcWritesLSCUGM) &&
       !clearWritesBeforeEOT) ||
      !(hasUAVWrites || hasSLMWrites || hasTypedWrites || hasWrites)) {
    return;
  }

  for (auto bb : kernel.fg) {
    if (bb->isLastInstEOT()) {
      auto iter = std::prev(bb->end());

      if (builder.supportsLSC() && clearHdcWritesLSCUGM) {
        if (needLscUgmFence) {
          G4_INST *fenceInst = nullptr;
          if (builder.getPlatform() == Xe_PVCXT) {
            fenceInst = builder.translateLscFence(
                nullptr, SFID::UGM, LSC_FENCE_OP_NONE, LSC_SCOPE_TILE);
          } else {
            // use fence.ugm.6.tile. 6 is reserved and is the same as none.
            fenceInst = builder.translateLscFence(
                nullptr, SFID::UGM, LSC_FENCE_OP_TYPE6, LSC_SCOPE_TILE);
          }
          bb->insertBefore(iter, fenceInst);
        }
      }

      if (clearHDCWritesBeforeEOT) {
        if (builder.supportsLSC()) {
          if (hasTypedWrites) {
            auto fenceInst = builder.translateLscFence(
                nullptr, SFID::TGM, LSC_FENCE_OP_NONE, LSC_SCOPE_LOCAL);
            bb->insertBefore(iter, fenceInst);
          }
          // If needLSCFence is true, the fence has been added already, skip the
          // following.
          if (hasUAVWrites && !needLscUgmFence) {
            auto fenceInst = builder.translateLscFence(
                nullptr, SFID::UGM, LSC_FENCE_OP_NONE, LSC_SCOPE_LOCAL);
            bb->insertBefore(iter, fenceInst);
          }
          if (hasSLMWrites && !hasUAVWrites) {
            // UGM fence takes of SLM fence as well
            auto fenceInst = builder.translateLscFence(
                nullptr, SFID::SLM, LSC_FENCE_OP_NONE, LSC_SCOPE_LOCAL);
            bb->insertBefore(iter, fenceInst);
          }
        } else {
          if (builder.getPlatform() == GENX_ICLLP) {
            hasTypedWrites =
                false;            // Workaround Under debug and being clarified
            hasSLMWrites = false; // Workaround not needed for ICL SLM Writes
          }
          if (hasUAVWrites || hasTypedWrites) {
            auto fenceInst = builder.createFenceInstructionPreLSC(
                nullptr, 0, true, true, false);
            bb->insertBefore(iter, fenceInst);
          }
          if (hasSLMWrites) {
            auto fenceInst = builder.createFenceInstructionPreLSC(
                nullptr, 0, true, false, false);
            bb->insertBefore(iter, fenceInst);
          }
        }
      }

      if (clearWritesBeforeEOT && hasWrites) {
        auto fenseInst = builder.translateLscFence(
            nullptr, SFID::UGM, LSC_FENCE_OP_EVICT, LSC_SCOPE_TILE);
        bb->insertBefore(iter, fenseInst);
      }

      builder.instList.clear();
    }
  }
}

// some platforms require extra instruction before an EOT to
// ensure that all outstanding scratch writes are globally observed
void Optimizer::insertScratchReadBeforeEOT() {
  int globalScratchOffset =
      kernel.getInt32KernelAttr(Attributes::ATTR_SpillMemOffset);
  if (builder.needFenceBeforeEOT() ||
      (globalScratchOffset == 0 &&
       builder.getJitInfo()->stats.spillMemUsed == 0)) {
    return;
  }

  struct ScratchReadDesc {
    uint32_t addrOffset : 12;
    uint32_t dataElements : 2;
    uint32_t reserved : 3;
    uint32_t opType : 2;
    uint32_t header : 1;
    uint32_t resLen : 5;
    uint32_t msgLen : 4;
    uint32_t reserved2 : 3;
  };

  union {
    uint32_t value;
    ScratchReadDesc layout;
  } desc;

  // msg desc for 1GRF scratch block read
  desc.value = 0;
  desc.layout.opType = 2;
  desc.layout.header = 1;
  desc.layout.resLen = 1;
  desc.layout.msgLen = 1;

  for (auto bb : kernel.fg) {
    if (bb->isLastInstEOT()) {
      auto iter = std::prev(bb->end());
      if (builder.getPlatformGeneration() >= PlatformGen::GEN10) {
        // an HDC fence is more efficient in this case
        // fence with commit enable
        int fenceDesc =
            G4_SendDescRaw::createDesc((0x7 << 14) | (1 << 13), true, 1, 1);
        auto msgDesc = builder.createSyncMsgDesc(SFID::DP_DC0, fenceDesc);
        auto src = builder.createSrcRegRegion(builder.getBuiltinR0(),
                                              builder.getRegionStride1());
        auto dst = builder.createDstRegRegion(builder.getBuiltinR0(), 1);
        G4_INST *inst =
            builder.createSendInst(nullptr, G4_send, g4::SIMD8, dst, src,
                                   builder.createImm(fenceDesc, Type_UD),
                                   InstOpt_WriteEnable, msgDesc, true);
        bb->insertBefore(iter, inst);
      } else {
        // insert a dumy scratch read
        auto msgDesc = builder.createReadMsgDesc(SFID::DP_DC0, desc.value);
        auto src = builder.createSrcRegRegion(builder.getBuiltinR0(),
                                              builder.getRegionStride1());
        // We can use any dst that does not conflcit with EOT src, which must be
        // between r112-r127
        auto dstDcl = builder.createHardwiredDeclare(8, Type_UD, 1, 0);
        auto dst = builder.createDstRegRegion(dstDcl, 1);
        G4_INST *sendInst =
            builder.createSendInst(nullptr, G4_send, g4::SIMD8, dst, src,
                                   builder.createImm(desc.value, Type_UD),
                                   InstOpt_WriteEnable, msgDesc, true);
        bb->insertBefore(iter, sendInst);
      }

      builder.instList.clear();
    }
  }
}
