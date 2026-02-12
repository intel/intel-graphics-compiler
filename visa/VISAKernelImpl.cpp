/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <fstream>
#include <functional>
#include <new>
#include <regex>
#include <sstream>

#include "Assertions.h"
#include "Attributes.hpp"
#include "BinaryEncoding.h"
#include "BinaryEncodingCNL.h"
#include "BinaryEncodingIGA.h"
#include "BuildIR.h"
#include "Common_BinaryEncoding.h"
#include "Common_ISA.h"
#include "Common_ISA_framework.h"
#include "Common_ISA_util.h"
#include "DebugInfo.h"
#include "FlowGraph.h"
#include "InstSplit.h"
#include "IsaDisassembly.h"
#include "JitterDataStruct.h"
#include "KernelInfo.h"
#include "LocalScheduler/SWSB_G4IR.h"
#include "Optimizer.h"
#include "Timer.h"
#include "VISAKernel.h"
#include "KernelCost.hpp"
#include "include/RelocationInfo.h"
#include "visa_igc_common_header.h"
#ifdef _WIN32
#include "inc/common/DriverStore.h"
#endif

#include <cctype>
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Path.h>
#include "common/LLVMWarningsPop.hpp"

using namespace CisaFramework;
using namespace vISA;

#define IS_GEN_PATH (mBuildOption == VISA_BUILDER_GEN)
#define IS_BOTH_PATH (mBuildOption == VISA_BUILDER_BOTH)
#define IS_GEN_BOTH_PATH                                                       \
  (mBuildOption == VISA_BUILDER_GEN || mBuildOption == VISA_BUILDER_BOTH)
#define IS_VISA_BOTH_PATH                                                      \
  (mBuildOption == VISA_BUILDER_VISA || mBuildOption == VISA_BUILDER_BOTH)


static void PACK_EXEC_SIZE(unsigned int &size, VISA_EMask_Ctrl eMask) {
  size |= eMask << 4;
}

static void ADD_OPND(int &num_operands, VISA_opnd **opnd, VISA_opnd *topnd) {
  if (topnd != NULL) {
    opnd[num_operands++] = topnd;
  }
}

static int CHECK_NUM_OPNDS(const VISA_INST_Desc *instDesc, int numOperands,
                           int predOpnds) {

  if ((instDesc->opnd_num - predOpnds) != numOperands) {
    vISA_ASSERT_INPUT(false, "number of parameters does not match");
    return VISA_FAILURE;
  }

  return VISA_SUCCESS;
}
static void GET_NUM_PRED_DESC_OPNDS(int &predOpnd,
                                    const VISA_INST_Desc *inst_desc_temp) {
  predOpnd = 0;
  for (int i = 0; i < inst_desc_temp->opnd_num; i++) {
    if (inst_desc_temp->opnd_desc[i].opnd_type == OPND_EXECSIZE ||
        inst_desc_temp->opnd_desc[i].opnd_type == OPND_PRED)
      predOpnd++;
  }
}

static void getHeightWidth(G4_Type type, unsigned int numberElements,
                           unsigned short &dclWidth, unsigned short &dclHeight,
                           int &totalByteSize, const IR_Builder &irb) {
  dclWidth = 1, dclHeight = 1;
  totalByteSize = numberElements * TypeSize(type);
  if (totalByteSize <= (int)irb.numEltPerGRF<Type_UB>()) {
    dclWidth = (uint16_t)numberElements;
  } else {
    // here we assume that the start point of the var is the beginning of a GRF?
    // so subregister must be 0?
    dclWidth = irb.numEltPerGRF<Type_UB>() / TypeSize(type);
    dclHeight = totalByteSize / irb.numEltPerGRF<Type_UB>();
    if (totalByteSize % irb.numEltPerGRF<Type_UB>() != 0) {
      dclHeight++;
    }
  }
}

#if defined(_DEBUG) || defined(_INTERNAL)
#define START_ASSERT_CHECK 1
#else
#define START_ASSERT_CHECK 0
#endif

#define GET_G4_OPNG(obj) (obj != NULL) ? obj->g4opnd : NULL

int VISAKernelImpl::calculateTotalInputSize() {
  for (unsigned int i = 0; i < m_builder->getInputCount(); i++) {
    input_info_t *temp = m_builder->getInputArg(i);
    if (m_inputSize < (unsigned int)(temp->offset + temp->size)) {
      m_inputSize = temp->offset + temp->size;
    }
  }
  return VISA_SUCCESS;
}

int VISAKernelImpl::compileFastPath() {
  int status = VISA_SUCCESS;

  vISA_ASSERT_INPUT(
      (getIsKernel() || getIsPayload() ||
       (m_kernelAttrs->isKernelAttrSet(Attributes::ATTR_ArgSize) &&
        m_kernelAttrs->isKernelAttrSet(Attributes::ATTR_RetValSize))),
      "vISA: input for function must have attributes ArgSize and RetValSize!");

  if (getIsKernel()) {
    status = calculateTotalInputSize();
  }

  if (status != VISA_SUCCESS) {
    return status;
  }

  IR_Builder &builder = *m_builder;
  builder.predefinedVarRegAssignment((uint8_t)m_inputSize);
  builder.expandPredefinedVars();
  builder.resizePredefinedStackVars();
  status = compileTillOptimize();
  return status;
}

void replaceFCOpcodes(IR_Builder &builder) {
  for (G4_BB *bb : builder.kernel.fg) {
    if (bb->size() > 0) {
      // pseudo_fc_call/ret would always be last
      // instruction in BB so only look at back
      // of instlist.
      G4_INST *lastInstInBB = bb->back();

      if (lastInstInBB->opcode() == G4_pseudo_fc_call) {
        lastInstInBB->asCFInst()->pseudoCallToCall();
      } else if (lastInstInBB->opcode() == G4_pseudo_fc_ret) {
        lastInstInBB->asCFInst()->pseudoRetToRet();
      }
    }
  }
}

static void setDeclAlignment(G4_Declare *dcl, const IR_Builder &builder,
                             VISA_Align align) {
  switch (align) {
  case ALIGN_BYTE: // no alignment;
  case ALIGN_WORD:
    dcl->setSubRegAlign(Any);
    break;
  case ALIGN_DWORD:
    dcl->setSubRegAlign(Even_Word);
    break;
  case ALIGN_QWORD:
    dcl->setSubRegAlign(Four_Word);
    break;
  case ALIGN_OWORD:
    dcl->setSubRegAlign(Eight_Word);
    break;
  case ALIGN_GRF:
    dcl->setSubRegAlign(builder.getGRFAlign());
    break;
  case ALIGN_2_GRF:
    dcl->setSubRegAlign(builder.getGRFAlign());
    dcl->setEvenAlign();
    break;
  case ALIGN_HWORD:
    dcl->setSubRegAlign(Sixteen_Word);
    break;
  case ALIGN_32WORD:
    dcl->setSubRegAlign(builder.getGRFAlign());
    if (builder.getGRFSize() == 32) {
      dcl->setEvenAlign();
    }
    break;
  case ALIGN_64WORD: // 64bytes GRF aligned
    vISA_ASSERT_INPUT(builder.getGRFSize() == 64, "incorrect GRF size");
    dcl->setSubRegAlign(ThirtyTwo_Word);
    dcl->setEvenAlign();
    break;
  default:
    vISA_ASSERT_INPUT(false, "Incorrect vISA alignment");
    break;
  }
}

int VISAKernelImpl::compileTillOptimize() {
  if (m_options->getOption(vISA_splitInstructions)) {
    InstSplitPass instSplit(m_builder);
    instSplit.run();
  }

  // For separate compilation run compilation till RA then return
  {
    TIME_SCOPE(CFG)
    m_kernel->fg.constructFlowGraph(m_builder->instList);
  }

  // move the options into the function, like LIR

  Optimizer optimizer(*m_kernelMem, *m_builder, *m_kernel, m_kernel->fg);

  return optimizer.optimization();
}

void VISAKernelImpl::adjustIndirectCallOffset() {
  // the call code sequence done at Optimizer::expandIndirectCallWithRegTarget
  // is:
  //       add  r2.0  -IP   call_target
  //       add  r2.0  r2.0  -32
  //       call r1.0  r2.0
  // -32 is hardcoded. But SWSB could've inserted sync instructions between
  // call and add. So we need to re-adjust the offset

  for (auto bb : m_kernel->fg) {
    if (bb->empty())
      continue;

    // At this point G4_pseudo_fcall may be converted to G4_call
    if (bb->back()->isCall() || bb->back()->isFCall()) {
      G4_INST *fcall = bb->back();
      auto callTarget = fcall->getSrc(0);
      if (callTarget->isGreg() ||
          (callTarget->isSrcRegRegion() &&
           callTarget->asSrcRegRegion()->isIndirect())) {
        // for every indirect call, count # of instructions inserted
        // between call and the first add
        uint64_t sync_offset = 0;
        G4_INST *first_add = nullptr;
        INST_LIST::reverse_iterator it = bb->rbegin();
        // skip call itself
        ++it;
        for (; it != bb->rend(); ++it) {
          G4_INST *inst = *it;
          G4_opcode op = inst->opcode();
          if (op == G4_sync_allrd || op == G4_sync_allwr) {
            inst->setNoCompacted();
            sync_offset += 16;
            continue;
          } else if (op == G4_sync_nop) {
            inst->setCompacted();
            sync_offset += 8;
            continue;
          } else if (op == G4_add) {
            if (first_add == nullptr) {
              first_add = inst;
              continue;
            } else {
              break;
            }
          }
          // instructions between call and add could only be
          // sync.nop, sync.allrd or sync.allwr
          vISA_ASSERT_UNREACHABLE("instructions betwen call and add can only be\
              sync.nop, sync.allrd or sync.allwr");
        }
        vASSERT(first_add->getSrc(1)->isImm());
        int64_t adjust_off =
            first_add->getSrc(1)->asImm()->getInt() - sync_offset;
        first_add->setSrc(m_builder->createImm(adjust_off, Type_D), 1);
      }
    }
  }
}

void VISAKernelImpl::compilePostOptimize() {

  if (getOptions()->getOption(vISA_AddKernelID)) {
    // gt debugger requires a dummy mov as first
    // executable instruction in compiled kernel.
    // No other instruction must be prepended
    // to the kernel before UUID mov.
    uint64_t kernelID = m_kernel->fg.insertDummyUUIDMov();
    m_kernel->setKernelID(kernelID);
  }

  m_kernel->evalAddrExp();

  if (getOptions()->getOption(vISA_DumpRegInfo)) {
    m_kernel->dump();
    m_kernel->emitRegInfo();
  }

  if (getOptions()->getOption(vISA_setStartBreakPoint)) {
    auto getFirstNonLabelInst = [this]() {
      unsigned int skip = 0, skipCount = 0;
      if (m_kernel->fg.builder->needsToLoadLocalID())
        ++skip;
      if (m_kernel->fg.builder->needsToLoadCrossThreadConstantData())
        ++skip;
      for (auto bb : m_kernel->fg) {
        if (skipCount++ < skip)
          continue;
        for (auto inst : *bb) {
          if (inst->isLabel()) {
            continue;
          }
          return inst;
        }
      }
      return (G4_INST *)nullptr;
    };

    G4_INST *inst = getFirstNonLabelInst();
    if (inst != nullptr) {
      inst->setOptionOn(InstOpt_BreakPoint);
    }
  }
}

void *VISAKernelImpl::encodeAndEmit(unsigned int &binarySize) {
  void *binary = NULL;

  //
  // Entry point to LIR conversion & transformations
  //
  startTimer(TimerID::ENCODE_AND_EMIT);
  setCurrentDebugPass("encode");
  if (m_builder->useIGAEncoder()) {
    auto r = EncodeKernelIGA(*m_kernel, m_asmName);
    binary = r.binary;
    binarySize = (unsigned)r.binaryLen;

    if (isFCCallableKernel() || isFCCallerKernel()) {
      computeFCInfo();

      // After retuning from replaceFCOpcodes, kernel
      // will have all pseudo_fccall and pseudo_fcret
      // opcodes replaced with the usual call
      // and ret opcodes.
      replaceFCOpcodes(*m_builder);
    }
  } else {
    BinaryEncodingBase *pBinaryEncoding = NULL;

    if (m_kernel->getPlatform() >= GENX_ICLLP &&
        m_options->getOption(vISA_BXMLEncoder)) {
      pBinaryEncoding =
          new BinaryEncodingCNL(*m_kernelMem, *m_kernel, m_asmName);
    } else {
      pBinaryEncoding = new BinaryEncoding(*m_kernelMem, *m_kernel, m_asmName);
    }

    pBinaryEncoding->DoAll();

    if (isFCCallableKernel() || isFCCallerKernel()) {
      computeFCInfo(pBinaryEncoding);

      // After retuning from replaceFCOpcodes, kernel
      // will have all pseudo_fccall and pseudo_fcret
      // opcodes replaced with the usual call
      // and ret opcodes.
      replaceFCOpcodes(*m_builder);
    }

    /*
        In DLL mode we copy content in to memory buffer
        when vISA_GenerateBinary is specified we dump out .dat file
        */

    binary = pBinaryEncoding->EmitBinary(binarySize);

    pBinaryEncoding->computeBinaryOffsets();

    for (auto bb : m_kernel->fg) {
      for (auto inst : *bb) {
        auto binInst = pBinaryEncoding->getBinInst(inst);
        inst->setGenOffset(binInst ? binInst->GetGenOffset()
                                   : UNDEFINED_GEN_OFFSET);
      }
    }

    delete pBinaryEncoding;
  }

  // perform any necessary relocation
  m_kernel->doRelocation(binary, binarySize);

  // Update instruction offset in register access maps.
  if (m_builder->hasSWSB() &&
      (isFCCallableKernel() || isFCCallerKernel() || isFCComposableKernel())) {
    auto FCPI = m_builder->getFCPatchInfo();
    auto &FirstAccess = FCPI->RegFirstAccessList;
    auto &LastAccess = FCPI->RegLastAccessList;
#if defined(DEBUG_VERBOSE_ON)
    std::cerr << "FirstAccess:\n";
#endif
    for (auto MI = FirstAccess.begin(), ME = FirstAccess.end(); MI != ME;
         ++MI) {
      auto Inst = MI->Inst;
      MI->Offset = unsigned(Inst->getGenOffset());
    }
#if defined(DEBUG_VERBOSE_ON)
    std::cerr << "LastAccess:\n";
#endif
    for (auto MI = LastAccess.begin(), ME = LastAccess.end(); MI != ME; ++MI) {
      auto Inst = MI->Inst;
      MI->Offset = unsigned(Inst->getGenOffset());
    }
  }

  if (m_options->getOption(vISA_PrintASMCount)) {
    m_builder->criticalMsgStream()
        << "  Kernel " << m_kernel->getName() << " : "
        << m_kernel->getAsmCount() << " asm instructions\n";
  }
  stopTimer(TimerID::ENCODE_AND_EMIT);

#if defined(_DEBUG) && defined(_WIN32)
  if (m_options->getOption(vISA_DebugConsoleDump)) {
    std::basic_ostringstream<char> debugBuff;
    m_kernel->emitDeviceAsm(debugBuff, nullptr, 0);
    emitPerfStats(debugBuff);
    debugBuff.flush();
    OutputDebugStringA(debugBuff.str().c_str());
  }
#endif

  if (getOptions()->getOption(vISA_GenerateKernelInfo)) {
    auto kernel = getKernel();
    m_kernelInfo = new KERNEL_INFO();
    m_kernelInfo->collectStats(*kernel);
  }

  if (m_options->getOption(vISA_asmToConsole)) {
    m_kernel->emitDeviceAsm(std::cout, binary, binarySize);
    emitPerfStats(std::cout);
    std::cout.flush();
  } else if (m_options->getOption(vISA_outputToFile)) {
    std::stringstream ss;
    ss << m_asmName << ".asm";
    std::string filePath = ss.str();
    if (allowDump(*m_options, filePath)) {
      std::ofstream krnlOutput(filePath, std::ofstream::out);
      if (!krnlOutput) {
        std::cerr << filePath << ": failed to open file\n";
      } else {
        m_kernel->emitDeviceAsm(krnlOutput, binary, binarySize);
        emitPerfStats(krnlOutput);
      }
    }
  }

  recordFinalizerInfo();

  if (m_options->getOption(vISA_DumpPerfStats) ||
      m_options->getOption(vISA_DumpPerfStatsVerbose)) {
    finalizePerfStats(std::hash<std::string>{}(std::string((char*)binary, binarySize)));
    dumpPerfStatsInJson(m_asmName);
  }

#ifdef _WIN32
  if (m_options->getOption(vISA_ShaderStatsDumpless)) {
    typedef void(__stdcall * PFNOPENWRAPPERDLL)(void *buf, size_t buf_size,
                                                char *dump_stem, char *platform,
                                                void *sendinfo);
    HMODULE handle = LoadDependency("WrapperLib.dll");
    if (handle) {
      auto OpenWrapper =
          (PFNOPENWRAPPERDLL)GetProcAddress(handle, "process_buffer");
      if (OpenWrapper) {
        vISA::PERF_SENDINFO::PERF_SENDINFO_VIEW SendInfoView =
            m_jitInfo->sendInfo.serialize();
        OpenWrapper(binary, binarySize, (char *)m_asmName.c_str(),
                    (char *)m_kernel->getGenxPlatformString(), &SendInfoView);
      }
    }
  }
#endif

  return binary;
}

void VISAKernelImpl::finalizePerfStats(uint64_t binaryHash) {
  // set BinaryHash
  m_jitInfo->stats.binaryHash = binaryHash;
}

void KERNEL_INFO::collectStats(G4_Kernel &kernel) {
  for (auto decl : kernel.Declares) {
    auto regVar = decl->getRegVar();
    if (regVar != nullptr) {
      if (regVar->isRegVar()) {
        numReg++;
      } else if (regVar->isRegVarTmp()) {
        numTmpReg++;
        bytesOfTmpReg += decl->getByteSize();
      } else if (regVar->isRegVarSpill()) {
        numSpillReg++;
      } else if (regVar->isRegVarFill()) {
        numFillReg++;
      }
    }
  }

  for (auto bb : kernel.fg) {
    for (auto instr : bb->getInstList()) {
      if (instr->getExecSize() == g4::SIMD1) {
        countSIMD1++;
      } else if (instr->getExecSize() == g4::SIMD2) {
        countSIMD2++;
      } else if (instr->getExecSize() == g4::SIMD4) {
        countSIMD4++;
      } else if (instr->getExecSize() == g4::SIMD8) {
        countSIMD8++;
      } else if (instr->getExecSize() == g4::SIMD16) {
        countSIMD16++;
      } else if (instr->getExecSize() == g4::SIMD32) {
        countSIMD32++;
      }

      if (instr->isSend()) {
        G4_InstSend *SendInst = instr->asSendInst();
        auto sendDesc = SendInst->getMsgDescRaw();
        if (!sendDesc->isLSC()) {
#define COUNT_HDC_SEND(SEND, isWRITE)                                          \
  case SEND:                                                                   \
    hdcSends.count##SEND++;                                                    \
    hdcSends.hasAnyHDCSend = true;                                             \
    break;

          auto funcID = sendDesc->getSFID();
          switch (funcID) {
          case SFID::DP_DC0:
            switch (sendDesc->getHdcMessageType()) {
              // Load
              COUNT_HDC_SEND(DC_OWORD_BLOCK_READ, false)
              COUNT_HDC_SEND(DC_ALIGNED_OWORD_BLOCK_READ, false)
              COUNT_HDC_SEND(DC_DWORD_SCATTERED_READ, false)
              COUNT_HDC_SEND(DC_BYTE_SCATTERED_READ, false)
              COUNT_HDC_SEND(DC_QWORD_SCATTERED_READ, false)
              // Store
              COUNT_HDC_SEND(DC_OWORD_BLOCK_WRITE, true)
              COUNT_HDC_SEND(DC_DWORD_SCATTERED_WRITE, true)
              COUNT_HDC_SEND(DC_BYTE_SCATTERED_WRITE, true)
              COUNT_HDC_SEND(DC_QWORD_SCATTERED_WRITE, true)
            default:
              break;
            }
            break;
          case SFID::DP_DC1:
            switch (sendDesc->getHdcMessageType()) {
              // Load
              COUNT_HDC_SEND(DC1_UNTYPED_SURFACE_READ, false)
              COUNT_HDC_SEND(DC1_MEDIA_BLOCK_READ, false)
              COUNT_HDC_SEND(DC1_TYPED_SURFACE_READ, false)
              COUNT_HDC_SEND(DC1_A64_SCATTERED_READ, false)
              COUNT_HDC_SEND(DC1_A64_UNTYPED_SURFACE_READ, false)
              COUNT_HDC_SEND(DC1_A64_BLOCK_READ, false)
              // Store
              COUNT_HDC_SEND(DC1_UNTYPED_SURFACE_WRITE, true)
              COUNT_HDC_SEND(DC1_MEDIA_BLOCK_WRITE, true)
              COUNT_HDC_SEND(DC1_TYPED_SURFACE_WRITE, true)
              COUNT_HDC_SEND(DC1_A64_BLOCK_WRITE, true)
              COUNT_HDC_SEND(DC1_A64_UNTYPED_SURFACE_WRITE, true)
              COUNT_HDC_SEND(DC1_A64_SCATTERED_WRITE, true)
            default:
              break;
            }
            break;
          case SFID::DP_DC2:
            switch (sendDesc->getHdcMessageType()) {
              // Load
              COUNT_HDC_SEND(DC2_UNTYPED_SURFACE_READ, false)
              COUNT_HDC_SEND(DC2_A64_SCATTERED_READ, false)
              COUNT_HDC_SEND(DC2_A64_UNTYPED_SURFACE_READ, false)
              COUNT_HDC_SEND(DC2_BYTE_SCATTERED_READ, false)
              // Store
              COUNT_HDC_SEND(DC2_UNTYPED_SURFACE_WRITE, true)
              COUNT_HDC_SEND(DC2_A64_UNTYPED_SURFACE_WRITE, true)
              COUNT_HDC_SEND(DC2_A64_SCATTERED_WRITE, true)
              COUNT_HDC_SEND(DC2_BYTE_SCATTERED_WRITE, true)
            default:
              break;
            }
            break;
          case SFID::URB:
            switch (sendDesc->getHdcMessageType()) {
              // Load
              COUNT_HDC_SEND(URB_READ_HWORD, false)
              COUNT_HDC_SEND(URB_READ_OWORD, false)
              COUNT_HDC_SEND(URB_SIMD8_READ, false)
              // Store
              COUNT_HDC_SEND(URB_WRITE_HWORD, true)
              COUNT_HDC_SEND(URB_WRITE_OWORD, true)
              COUNT_HDC_SEND(URB_SIMD8_WRITE, true)
            default:
              break;
            }
            break;
          default:
            break;
          }
#undef COUNT_HDC_SEND
        } else {
#define COUNT_LSC_SEND(SEND, isWRITE)                                          \
  case LSC_OP::SEND:                                                           \
    lscSends.count##SEND++;                                                    \
    lscSends.hasAnyLSCSend = true;                                             \
    break;

          switch (sendDesc->getLscOp()) {
            // Load
            COUNT_LSC_SEND(LSC_LOAD, false)
            COUNT_LSC_SEND(LSC_LOAD_STRIDED, false)
            COUNT_LSC_SEND(LSC_LOAD_QUAD, false)
            COUNT_LSC_SEND(LSC_LOAD_BLOCK2D, false)
            // Store
            COUNT_LSC_SEND(LSC_STORE, true)
            COUNT_LSC_SEND(LSC_STORE_STRIDED, true)
            COUNT_LSC_SEND(LSC_STORE_QUAD, true)
            COUNT_LSC_SEND(LSC_STORE_BLOCK2D, true)
            COUNT_LSC_SEND(LSC_STORE_UNCOMPRESSED, true)
          default:
            break;
          }
#undef COUNT_LSC_SEND
        }
      }

      if (instr->isSpillIntrinsic()) {
        auto payload = instr->getSrc(1)->asSrcRegRegion();
        auto dcl = payload->getTopDcl();
        spillFills.countBytesSpilled += dcl->getByteSize();

        spillFills.AddVirtualVar(dcl->getName());
        spillFills.spillInstrOrder.push_back(instr->getVISAId());

      } else if (instr->isFillIntrinsic()) {
        spillFills.spillInstrOrder.push_back(instr->getVISAId());
      }
    }
  }
}

// dump PERF_STATS into the .stats.json file
// filename is the full path of output file name without the extension
void VISAKernelImpl::dumpPerfStatsInJson(const std::string &filename) {
  std::string outputName = filename + ".stats.json";
  if (!allowDump(*m_options, outputName)) {
    return;
  }
  std::ofstream statsOutput(outputName, std::ofstream::out);
  if (!statsOutput) {
    std::cerr << outputName << ": failed to open file\n";
    return;
  }

  llvm::json::Value pv = m_jitInfo->stats;
  llvm::json::Object* po = pv.getAsObject();
  po->insert({"name", m_name});

  if (m_options->getOption(vISA_DumpPerfStatsVerbose)) {
    llvm::json::Value pvv = m_jitInfo->statsVerbose;
    llvm::json::Object* pvo = pvv.getAsObject();
    for (auto itr = pvo->begin(); itr != pvo->end(); ++itr) {
      po->insert({itr->first, itr->second});
    }
  }

  llvm::json::Value output_jv = nullptr;
  if (m_options->getOption(vISA_DumpSendInfoStats)) {
    llvm::json::Array Src0Array;
    for (const auto &val : m_jitInfo->sendInfo.src0Vec) {
      Src0Array.push_back(val);
    }
    llvm::json::Array Src1Array;
    for (const auto &val : m_jitInfo->sendInfo.src1Vec) {
      Src1Array.push_back(val);
    }
    llvm::json::Array DestArray;
    for (const auto &val : m_jitInfo->sendInfo.destVec) {
      DestArray.push_back(val);
    }

    llvm::json::Value sendArray =
        llvm::json::Object({{"src0", std::move(Src0Array)},
                            {"src1", std::move(Src1Array)},
                            {"dest", std::move(DestArray)}});

    output_jv = llvm::json::Array({std::move(pv), std::move(sendArray)});
  }
  else {
    output_jv = llvm::json::Array({std::move(pv)});
  }
  statsOutput << llvm::formatv("{0:2}", output_jv).str() << "\n";
}



void VISAKernelImpl::recordFinalizerInfo() {
  auto jitInfo = m_builder->getJitInfo();
  if (jitInfo) {
    jitInfo->stats.numAsmCountUnweighted = m_kernel->getAsmCount();
    jitInfo->stats.numGRFTotal = m_kernel->getNumRegTotal();
    jitInfo->stats.numThreads = m_kernel->getNumThreads();
    jitInfo->BBNum = static_cast<uint32_t>(m_kernel->fg.size());
  }
}

int VISAKernelImpl::InitializeFastPath() {
  m_kernelMem = new vISA::Mem_Manager(4096);

  uint32_t funcId;
  GetFunctionId(funcId);
  m_kernel = new (m_mem)
      G4_Kernel(*getCISABuilder()->getPlatformInfo(), m_instListNodeAllocator,
                *m_kernelMem, m_options, m_kernelAttrs, funcId, m_major_version,
                m_minor_version);
  m_kernel->setName(m_name.c_str());

  if (getOptions()->getOption(vISA_GenerateDebugInfo)) {
    m_kernel->getKernelDebugInfo()->setVISAKernel(this);
  }

  void *pJitInfoMem = m_mem.alloc(sizeof(FINALIZER_INFO));
  m_jitInfo = new (pJitInfoMem) FINALIZER_INFO();

  void *addr = m_kernelMem->alloc(sizeof(class IR_Builder));
  m_builder = new (addr)
      IR_Builder(m_instListNodeAllocator, *m_kernel, *m_kernelMem, m_options,
                 getCISABuilder(), m_jitInfo, getCISABuilder()->getWATable());

  m_builder->setType(m_type);
  return VISA_SUCCESS;
}

void VISAKernelImpl::CopyVars(VISAKernelImpl *from) {
  m_builder->dclpool.getDeclareList() =
      from->m_builder->dclpool.getDeclareList();
}

int VISAKernelImpl::InitializeKernel(const char *kernel_name) {
  int status = VISA_SUCCESS;
  m_num_pred_vars = Get_CISA_PreDefined_Var_Count();
  setName(kernel_name);
  if (IS_GEN_BOTH_PATH) {
    status = InitializeFastPath();
  }

  CISABuildPreDefinedDecls();
  fmt = new VISAKernel_format_provider(this);
  verifier = new vISAVerifier(fmt, m_options, m_builder);

  return status;
}

int VISAKernelImpl::CISABuildPreDefinedDecls() {
  for (unsigned int i = 0; i < m_num_pred_vars; i++) {
    auto predefId = mapExternalToInternalPreDefVar(i);
    CISA_GEN_VAR *decl = (CISA_GEN_VAR *)m_mem.alloc(sizeof(CISA_GEN_VAR));
    decl->type = GENERAL_VAR;
    decl->index = m_var_info_count++;
    decl->genVar.name_index = -1;
    decl->genVar.attribute_count = 0;
    decl->genVar.attributes = NULL;
    decl->genVar.alias_index = 0;
    decl->genVar.alias_offset = 0;

    if (predefId != PreDefinedVarsInternal::VAR_LAST) {
      // Going to create a G4 declaration here, and at compile time set the
      // register allocation since this is when number of inputs will be known.
      if (IS_GEN_BOTH_PATH) {
        G4_Declare *dcl = m_builder->preDefVars.getPreDefinedVar(predefId);
        decl->genVar.dcl = dcl;
      }
      if (IS_VISA_BOTH_PATH) {
        std::string varName(getPredefinedVarString(predefId));
        std::string alias = "V" + std::to_string(i);
        decl->genVar.name_index = addStringPool(varName);
        if (m_options->getOption(vISA_isParseMode)) {
          setNameIndexMap(alias, decl, true);
          setNameIndexMap(varName, decl, true);
        }
      }
    }
    addVarInfoToList(decl);
  }

  // SLM T0/Global Vars
  for (int i = 0; i < (int)Get_CISA_PreDefined_Surf_Count(); i++) {
    VISA_SurfaceVar *decl =
        (VISA_SurfaceVar *)m_mem.alloc(sizeof(CISA_GEN_VAR));
    ////memset(decl, 0, sizeof(CISA_GEN_VAR));
    decl->type = SURFACE_VAR;
    decl->index = m_surface_count++;
    decl->stateVar.attributes = NULL;
    decl->stateVar.attribute_count = 0;
    decl->stateVar.name_index = -1;
    if (IS_VISA_BOTH_PATH) {
      const char *name = vISAPreDefSurf[i].name;
      decl->stateVar.name_index = addStringPool(std::string(name));
      setNameIndexMap(std::string(name), decl, true);
    }
    if (IS_GEN_BOTH_PATH) {
      if (i == PREDEFINED_SURFACE_T252) {
        decl->stateVar.dcl = m_builder->getBuiltinT252();
      } else if (i == PREDEFINED_SURFACE_SCRATCH) {
        decl->stateVar.dcl = m_builder->getBuiltinScratchSurface();
      } else {
        decl->stateVar.dcl =
            m_builder->createDeclare("", G4_GRF, 1, 1, Type_UD);
      }
    }
    addSurface(decl);
  }

  createBindlessSampler();

  return VISA_SUCCESS;
}

void VISAKernelImpl::createBindlessSampler() {
  m_bindlessSampler = (VISA_SamplerVar *)m_mem.alloc(sizeof(CISA_GEN_VAR));
  m_bindlessSampler->type = SAMPLER_VAR;
  m_bindlessSampler->index = 31;
  m_bindlessSampler->stateVar.attributes = NULL;
  m_bindlessSampler->stateVar.attribute_count = 0;

  if (IS_VISA_BOTH_PATH) {
    const char *name = "S31";
    m_bindlessSampler->stateVar.name_index = addStringPool(std::string(name));
    setNameIndexMap(std::string(name), m_bindlessSampler, true);
  }
  if (IS_GEN_BOTH_PATH) {
    m_bindlessSampler->stateVar.dcl = m_builder->getBuiltinBindlessSampler();
  }
}

void VISAKernelImpl::createReservedKeywordSet() {
  for (int i = 0; i < ISA_NUM_OPCODE; i++) {
    const VISA_INST_Desc &desc = CISA_INST_table[i];
    if (desc.name != nullptr)
      reservedNames.insert(desc.name);
    int subOpsLen = 0;
    const ISA_SubInst_Desc *subOps = getSubInstTable(desc.opcode, subOpsLen);
    if (subOps != nullptr) {
      // e.g. ops like ISA_SVM have a sub-table of operations
      for (int si = 0; si < subOpsLen; si++) {
        // some tables have padding and empty ops; a nullptr name indicates that
        if (subOps[si].name != nullptr)
          reservedNames.insert(subOps[si].name);
      }
    }
  }

  // a mishmash of some of the other reserved words from the lexical
  // specification
  reservedNames.insert("FILE");
  reservedNames.insert("LOC");
  reservedNames.insert("arg");
  reservedNames.insert("bss");
  reservedNames.insert("bti");
  reservedNames.insert("flat");
  reservedNames.insert("ss");
}

bool VISAKernelImpl::isReservedName(const std::string &nm) const {
  auto opItr = reservedNames.find(nm);
  return (opItr != reservedNames.end());
}

void VISAKernelImpl::ensureVariableNameUnique(const char *&varName) {
  // legalize the LLVM name to vISA standards; some examples follow:
  // given  ==> we fix it to this
  // 1.  "0"  ==> "_0"              (LLVM name)
  // 2.  "add.i.i" ==> "add_i_i"    (LLVM compound name)
  // 3.  "mul" ==> "mul_"           (vISA keyword)
  // 4.  suppose both variable "x" and "x0" exist
  //       "x" ==> "x_1v"            (since "x" already used)
  //       "x0" ==> "x0_1v"          (append suffx "_#v")
  //      Suffix "_#v" is used to avoid treating a llvm name, such
  //      as n.123, as visa-generated name. It is useful to fast-check
  //      whether a name is visa-generated.
  std::stringstream escdName;

  // step 1
  if (isdigit(varName[0]))
    escdName << '_';

  // step 2
  for (size_t i = 0, slen = std::string_view(varName).size(); i < slen; i++) {
    char c = varName[i];
    if (!isalnum(c)) {
      c = '_';
    }
    escdName << c;
  }

  // case 3: "mul" ==> "mul_"
  while (isReservedName(escdName.str()))
    escdName << '_';

  // case 4.1: if "x" already exists; or
  // case 4.2: if "x" is in "y_#v" that has been auto-generated here
  //           but not in map.
  // In both cases,  use "x_#v" where # is 0,1,...
  // (Note that suffix '#v' is for fast-checking visa-generated names.
  //  The 'v' is arbitrary and is to mean visa.)
  std::string varNameS = escdName.str();
  const size_t Len = varNameS.length();
  // case 4.1
  bool existing = (varNames.find(varNameS) != varNames.end());
  // case 4.2, fast-check the last "#v" to see if it can collide with
  //           visa-generated names.
  if (!existing && Len > 2 && isdigit(varNameS.at(Len - 2)) &&
      varNameS.at(Len - 1) == 'v') {
    // case 4.2
    size_t No;
    size_t pos = varNameS.rfind('_');
    if (pos != std::string::npos) {
      std::string suffix = varNameS.substr(pos + 1);
      No = 0;
      bool allDigit = true;
      // skip the last 'v'
      for (int i = 0, e = suffix.length() - 1; i < e; ++i) {
        char c = suffix.at(i);
        if (!isdigit(c)) {
          allDigit = false;
          break;
        }
        No = No * 10 + (c - '0');
      }

      if (allDigit) {
        std::string prefix = varNameS.substr(0, pos);
        if (auto it = varNames.find(prefix); it != varNames.end()) {
          size_t instNo = it->second;
          if (instNo > No) {
            // Create entry for varNameS
            varNames.emplace(varNameS, 0);
            existing = true;
          }
        }
      }
    }
  }
  if (existing) {
    size_t instanceNumber = varNames[varNameS];
    std::string origVarNameS = varNameS;
    // Make sure the new name does not exist yet.
    do {
      std::stringstream ss;
      ss << escdName.str() << '_' << instanceNumber << 'v';
      varNameS = ss.str();
      ++instanceNumber;
    } while (varNames.find(varNameS) != varNames.end());
    varNames[origVarNameS] = instanceNumber;
  } else {
    varNames.emplace(varNameS, 0);
  }

  char *buf = (char *)m_mem.alloc(varNameS.size() + 1);
  memcpy_s(buf, varNameS.size(), varNameS.c_str(), varNameS.size());
  buf[varNameS.size()] = 0;
  varName = buf;
}

// Return true if varName is updated to be an unique one.
bool VISAKernelImpl::generateVariableName(Common_ISA_Var_Class Ty,
                                          const char *&varName) {
  if (!m_options->getOption(vISA_GenerateISAASM) && !IsAsmWriterMode()) {
    // variable name is a don't care if we are not outputting vISA assembly
    return false;
  }

  bool asmInput = m_options->getOption(vISA_isParseMode);
  if (varName && *varName) {
    // if a custom name is given, then ensure it's unique;
    // if it's not, we will suffix it
    // Note that it's not legal for asm input since we allow duplicate variable
    // names if they are in different scope {...}
    if (!asmInput)
      ensureVariableNameUnique(varName);
    return !asmInput;
  }

  // fall back onto a generic naming scheme
  // e.g. _V## for generic variables, etc...
  auto createAnonVarName = [&](char pfx, unsigned index, int fillW) {
    std::stringstream ss;
    ss << pfx << std::setfill('0') << std::setw(fillW) << index;
    size_t slen = (size_t)ss.tellp();
    char *buf = (char *)m_mem.alloc(slen + 1);
    memcpy_s(buf, slen + 1, ss.str().c_str(), slen);
    buf[slen] = 0;
    return buf;
  };

  switch (Ty) {
  case Common_ISA_Var_Class::GENERAL_VAR:
    varName = createAnonVarName('V', varNameCount++, 4);
    break;
  case Common_ISA_Var_Class::PREDICATE_VAR:
    varName = createAnonVarName('P', predicateNameCount++, 2);
    break;
  case Common_ISA_Var_Class::ADDRESS_VAR:
    varName = createAnonVarName('A', addressNameCount++, 2);
    break;
  case Common_ISA_Var_Class::SURFACE_VAR:
    varName = createAnonVarName('T', surfaceNameCount++, 3);
    break;
  case Common_ISA_Var_Class::SAMPLER_VAR:
    varName = createAnonVarName('S', samplerNameCount++, 3);
    break;
  default:
    varName = createAnonVarName('X', unknownNameCount++, 2);
    break;
  }

  // ensure that our auto-generated name is unique
  // i.e. suffix the variable if input already uses this name
  ensureVariableNameUnique(varName);
  return true;
}

std::string VISAKernelImpl::getVarName(VISA_GenVar *decl) const {
  return getVarName((CISA_GEN_VAR *)decl);
}
std::string VISAKernelImpl::getVarName(VISA_PredVar *decl) const {
  return getVarName((CISA_GEN_VAR *)decl);
}
std::string VISAKernelImpl::getVarName(VISA_AddrVar *decl) const {
  return getVarName((CISA_GEN_VAR *)decl);
}
std::string VISAKernelImpl::getVarName(VISA_SurfaceVar *decl) const {
  return getVarName((CISA_GEN_VAR *)decl);
}
std::string VISAKernelImpl::getVarName(VISA_SamplerVar *decl) const {
  return getVarName((CISA_GEN_VAR *)decl);
}

std::string VISAKernelImpl::getVectorOperandName(VISA_VectorOpnd *opnd,
                                                 bool showRegion) const {
  VISAKernel_format_provider fmt(this);
  return printVectorOperand(&fmt, opnd, m_options, showRegion);
}

std::string VISAKernelImpl::getPredicateOperandName(VISA_PredOpnd *opnd) const {
  VISAKernel_format_provider fmt(this);
  return printVectorOperand(&fmt, opnd, m_options, false);
}

int VISAKernelImpl::CreateVISAGenVar(VISA_GenVar *&decl, const char *varName,
                                     int numberElements, VISA_Type dataType,
                                     VISA_Align varAlign,
                                     VISA_GenVar *parentDecl, int aliasOffset) {
  TIME_SCOPE(VISA_BUILDER_CREATE_VAR);

  if (varName == nullptr)
    varName = "";
  decl = (VISA_GenVar *)m_mem.alloc(sizeof(VISA_GenVar));
  decl->type = GENERAL_VAR;
  var_info_t *info = &decl->genVar;

  bool nameModified = generateVariableName(decl->type, varName);

  if (m_options->getOption(vISA_isParseMode) &&
      !setNameIndexMap(varName, decl)) {
    vISA_ASSERT_INPUT(false, "incorrect parse option");
    return VISA_FAILURE;
  }

  m_GenVarToNameMap[decl] = varName;

  info->bit_properties = (uint8_t)dataType;
  info->bit_properties += varAlign << 4;

  info->num_elements = (uint16_t)numberElements;
  info->alias_offset = 0;
  info->alias_index = 0;
  info->alias_scope_specifier = 0;

  if (parentDecl) {
    info->alias_offset = (uint16_t)aliasOffset;
    info->alias_index = parentDecl->index;
  }

  info->attribute_capacity = 0;
  info->attribute_count = 0;
  info->attributes = nullptr;

  decl->index = m_var_info_count++;

  if (IS_GEN_BOTH_PATH) {
    G4_Type type = GetGenTypeFromVISAType(dataType);
    unsigned short dclWidth = 1, dclHeight = 1;
    int totalByteSize = 0;
    getHeightWidth(type, numberElements, dclWidth, dclHeight, totalByteSize,
                   *m_builder);

    // If name is modified, this means we have already created a new copy,
    // and there's no need to create another copy.
    auto dclName = nameModified ? varName : createStringCopy(varName, m_mem);
    info->dcl = m_builder->createDeclare(dclName, G4_GRF, dclWidth,
                                                 dclHeight, type);

    if (parentDecl) {
      var_info_t *aliasDcl = &parentDecl->genVar;
      info->dcl->setAliasDeclare(aliasDcl->dcl, aliasOffset);

      // check if parent declare is one of the predefined
      if (parentDecl->index < Get_CISA_PreDefined_Var_Count()) {
        m_builder->preDefVars.setHasPredefined(
            mapExternalToInternalPreDefVar(parentDecl->index), true);
      }
    }

    // force subalign to be GRF if total size is larger than or equal to GRF
    if ((info->dcl->getSubRegAlign() != getIRBuilder()->getGRFAlign()) ||
        (varAlign ==
         (m_builder->getGRFSize() == 64 ? ALIGN_64WORD : ALIGN_32WORD)) ||
        (varAlign == ALIGN_2_GRF)) {
      setDeclAlignment(info->dcl, *m_builder, varAlign);
    }

    info->name_index = -1;
  }

  if (IS_VISA_BOTH_PATH || m_options->getOption(vISA_GenerateDebugInfo) ||
      IsAsmWriterMode()) {
    info->name_index = addStringPool(std::string(varName));
    addVarInfoToList(decl);

    // Write asm variable decl to stream
    if (IsAsmWriterMode()) {
      VISAKernel_format_provider fmt(this);
      m_CISABuilder->m_ssIsaAsm
          << printVariableDecl(&fmt, m_printDeclIndex.var_index++, getOptions())
          << "\n";
    }
  }

  return VISA_SUCCESS;
}

int VISAKernelImpl::CreateVISAAddrVar(VISA_AddrVar *&decl, const char *varName,
                                      unsigned int numberElements) {
  TIME_SCOPE(VISA_BUILDER_CREATE_VAR);

  decl = (VISA_AddrVar *)m_mem.alloc(sizeof(VISA_AddrVar));
  decl->type = ADDRESS_VAR;

  if (m_options->getOption(vISA_isParseMode) &&
      !setNameIndexMap(std::string(varName), decl)) {
    vASSERT(false);
    return VISA_FAILURE;
  }

  addr_info_t *addr = &decl->addrVar;
  bool nameModified = generateVariableName(decl->type, varName);

  m_GenVarToNameMap[decl] = varName;

  decl->index = m_addr_info_count++;
  if (IS_GEN_BOTH_PATH) {
    // If name is modified, this means we have already created a new copy,
    // and there's no need to create another copy.
    auto dclName = nameModified ? varName : createStringCopy(varName, m_mem);
    addr->dcl = m_builder->createDeclare(
        dclName, G4_ADDRESS, (uint16_t)numberElements,
        1, Type_UW);
    addr->name_index = -1;
  }

  addr->num_elements = (uint16_t)numberElements;
  addr->attribute_capacity = 0;
  addr->attribute_count = 0;
  addr->attributes = NULL;

  if (IS_VISA_BOTH_PATH || m_options->getOption(vISA_GenerateDebugInfo) ||
      IsAsmWriterMode()) {
    addr->name_index = addStringPool(std::string(varName));
    addAddrToList(decl);

    if (IsAsmWriterMode()) {
      VISAKernel_format_provider fmt(this);
      m_CISABuilder->m_ssIsaAsm
          << printAddressDecl(&fmt, m_printDeclIndex.addr_index++)
          << "\n";
    }
  }

  return VISA_SUCCESS;
}

int VISAKernelImpl::CreateVISAPredVar(VISA_PredVar *&decl, const char *varName,
                                      unsigned short numberElements) {
  TIME_SCOPE(VISA_BUILDER_CREATE_VAR);

  decl = (VISA_PredVar *)m_mem.alloc(sizeof(VISA_PredVar));
  decl->type = PREDICATE_VAR;

  constexpr int MAX_VISA_PRED_SIZE = 32;
  vISA_ASSERT_INPUT(numberElements <= MAX_VISA_PRED_SIZE,
               "number of flags must be <= 32");

  if (m_options->getOption(vISA_isParseMode) &&
      !setNameIndexMap(std::string(varName), decl)) {
    vASSERT(false);
    return VISA_FAILURE;
  }
  bool nameModified = generateVariableName(decl->type, varName);

  m_GenVarToNameMap[decl] = varName;

  pred_info_t *pred = &decl->predVar;

  decl->index = COMMON_ISA_NUM_PREDEFINED_PRED + m_pred_info_count++;
  pred->num_elements = numberElements;
  pred->attribute_capacity = 0;
  pred->attribute_count = 0;
  pred->attributes = NULL;
  if (IS_GEN_BOTH_PATH) {
    // If name is modified, this means we have already created a new copy,
    // and there's no need to create another copy.
    auto dclName = nameModified ? varName : createStringCopy(varName, m_mem);
    pred->dcl =
        m_builder->createFlag(numberElements, dclName);
    pred->name_index = -1;
  }
  if (IS_VISA_BOTH_PATH || m_options->getOption(vISA_GenerateDebugInfo) ||
      IsAsmWriterMode()) {
    pred->name_index = addStringPool(std::string(varName));
    addPredToList(decl);
  }

  if (IsAsmWriterMode()) {
    VISAKernel_format_provider fmt(this);
    m_CISABuilder->m_ssIsaAsm
        << printPredicateDecl(&fmt, m_printDeclIndex.pred_index++) << "\n";
  }

  return VISA_SUCCESS;
}

int VISAKernelImpl::CreateStateVar(CISA_GEN_VAR *&decl,
                                   Common_ISA_Var_Class type,
                                   const char *varName,
                                   unsigned int numberElements) {
  TIME_SCOPE(VISA_BUILDER_CREATE_VAR);

  decl = (CISA_GEN_VAR *)m_mem.alloc(sizeof(CISA_GEN_VAR));
  decl->type = type;

  if (m_options->getOption(vISA_isParseMode) &&
      !setNameIndexMap(std::string(varName), decl)) {
    vASSERT(false);
    return VISA_FAILURE;
  }
  bool nameModified = generateVariableName(decl->type, varName);

  m_GenVarToNameMap[decl] = varName;

  state_info_t *state = &decl->stateVar;
  state->attribute_capacity = 0;
  state->attribute_count = 0;
  state->attributes = NULL;
  state->num_elements = (uint16_t)numberElements;
  if (IS_GEN_BOTH_PATH) {
    // If name is modified, this means we have already created a new copy,
    // and there's no need to create another copy.
    auto dclName = nameModified ? varName : createStringCopy(varName, m_mem);
    state->dcl = m_builder->createDeclare(
        dclName, G4_GRF, (uint16_t)numberElements, 1, Type_UD);
    state->name_index = -1;
  }

  switch (type) {
  case SAMPLER_VAR:
    decl->index = m_sampler_count++;
    break;
  case SURFACE_VAR:
    decl->index = m_surface_count++;
    break;
  default:
    vISA_ASSERT_UNREACHABLE("invalid type");
    return VISA_FAILURE;
  }

  if (IS_VISA_BOTH_PATH || m_options->getOption(vISA_GenerateDebugInfo) ||
      IsAsmWriterMode()) {
    state->name_index = addStringPool(std::string(varName));
    switch (type) {
    case SAMPLER_VAR: {
      addSampler(decl);
      if (IsAsmWriterMode()) {
        VISAKernel_format_provider fmt(this);
        m_CISABuilder->m_ssIsaAsm
            << printSamplerDecl(&fmt, m_printDeclIndex.sampler_index++) << "\n";
      }
      break;
    }
    case SURFACE_VAR: {
      addSurface(decl);
      if (IsAsmWriterMode()) {
        VISAKernel_format_provider fmt(this);
        unsigned numPreDefinedSurfs = Get_CISA_PreDefined_Surf_Count();
        m_CISABuilder->m_ssIsaAsm
            << printSurfaceDecl(&fmt, m_printDeclIndex.surface_index++,
                                numPreDefinedSurfs)
            << "\n";
      }
      break;
    }
    default:
      vISA_ASSERT_UNREACHABLE("incorrect common isa var class type");
      return VISA_FAILURE;
    }
  }

  return VISA_SUCCESS;
}

int VISAKernelImpl::CreateVISASamplerVar(VISA_SamplerVar *&decl,
                                         const char *name,
                                         unsigned int numberElements) {
  return CreateStateVar((CISA_GEN_VAR *&)decl, SAMPLER_VAR, name,
                        numberElements);
}

int VISAKernelImpl::CreateVISASurfaceVar(VISA_SurfaceVar *&decl,
                                         const char *name,
                                         unsigned int numberElements) {
  return CreateStateVar((CISA_GEN_VAR *&)decl, SURFACE_VAR, name,
                        numberElements);
}

int VISAKernelImpl::CreateVISALabelVar(VISA_LabelOpnd *&opnd, const char *name,
                                       VISA_Label_Kind kind) {
  TIME_SCOPE(VISA_BUILDER_CREATE_VAR);

  opnd = (VISA_LabelOpnd *)m_mem.alloc(sizeof(VISA_LabelOpnd));

  if (IS_GEN_BOTH_PATH) {
    // needs to be persistent since label opnd can be used multiple times
    if (getIsFunction()) {
      std::string fname = "L_f" + std::to_string(m_functionId) + "_" + name;
      opnd->g4opnd = m_builder->createLabel(fname, kind);
    } else {
      opnd->g4opnd = m_builder->createLabel(name, kind);
    }
  }
  if (IS_VISA_BOTH_PATH) {
    label_info_t *lbl = (label_info_t *)m_mem.alloc(sizeof(label_info_t));
    lbl->name_index = addStringPool(std::string(name));
    lbl->kind = kind;
    m_label_info_list.push_back(lbl);

    opnd->_opnd.other_opnd = m_label_count++;

    VISA_INST_Desc *inst_desc = NULL;

    if (kind == LABEL_BLOCK || kind == LABEL_DIVERGENT_RESOURCE_LOOP) {
      inst_desc = &CISA_INST_table[ISA_LABEL];
      opnd->tag = ISA_LABEL;
    } else {
      inst_desc = &CISA_INST_table[ISA_SUBROUTINE];
      opnd->tag = ISA_SUBROUTINE;
    }

    opnd->opnd_type = CISA_OPND_OTHER;
    opnd->size = (uint16_t)Get_VISA_Type_Size(
        (VISA_Type)inst_desc->opnd_desc[0].data_type);
    lbl->attribute_capacity = 0;
    lbl->attribute_count = 0;
    lbl->attributes = NULL;
  }

  return VISA_SUCCESS;
}

int VISAKernelImpl::AddKernelAttribute(const char *attrName, int size,
                                       const void *valueBuffer) {
  attribute_info_t *attr =
      (attribute_info_t *)m_mem.alloc(sizeof(attribute_info_t));
  Attributes::ID attrID = Attributes::getAttributeID(attrName);

  vISA_ASSERT_INPUT(Attributes::isKernelAttr(attrID), "Not a kernel attribute");

  if (attrID == Attributes::ATTR_OutputAsmPath) {
    if (m_options->getOption(vISA_AsmFileNameOverridden)) {
      if (const char *asmName = m_options->getOptionCstr(VISA_AsmFileName)) {
        m_asmName = asmName;
      } else {
        m_asmName = "";
      }
    } else {
      llvm::StringRef path = (const char *) valueBuffer;
      llvm::SmallString<32> asmName;
      if (m_options->getOption(vISA_dumpToCurrentDir))
        asmName = llvm::sys::path::filename(path);
      else
        asmName = path;

      // Drop .asm extension if any
      if (llvm::sys::path::extension(asmName) == ".asm")
        llvm::sys::path::replace_extension(asmName, "");
      m_asmName = sanitizePathString(asmName.str().str());
      m_options->setOptionInternally(VISA_AsmFileName, m_asmName.c_str());
    }
  }

  attr->size = (uint8_t)size;
  if (Attributes::isInt32(attrID)) {
    attr->isInt = true;
    switch (attr->size) {
    case 0:
      attr->value.intVal = 1;
      break;
    case 1:
      attr->value.intVal = *((int8_t *)valueBuffer);
      break;
    case 2:
      attr->value.intVal = *((int16_t *)valueBuffer);
      break;
    case 4:
      attr->value.intVal = *((int32_t *)valueBuffer);
      break;
    default:
      vISA_ASSERT_UNREACHABLE("Unsupported attribute size");
      break;
    }
    m_kernelAttrs->setKernelAttr(attrID, attr->value.intVal, *getIRBuilder());
  } else if (Attributes::isBool(attrID)) {
    attr->isInt = true; // treat bool as int in attribute_info_t
    attr->value.intVal = 1;
    m_kernelAttrs->setKernelAttr(attrID, true);
  } else if (Attributes::isCStr(attrID)) {
    attr->isInt = false;
    if (size > 0) {
      char *tmp = (char *)m_mem.alloc(size + 1);
      memcpy_s(tmp, size + 1, valueBuffer, size + 1);
      attr->value.stringVal = tmp;
    } else {
      attr->value.stringVal = (const char *)"";
    }
    m_kernelAttrs->setKernelAttr(attrID, attr->value.stringVal);
  } else {
    vISA_ASSERT_INPUT(false, "Unsupported kernel attribute!");
  }

  if (attrID == Attributes::ATTR_Target) {
    if (attr->value.intVal == 0) {
      // ToDo: remove setTarget() call, internal code should use getKernelType()
      // instead
      m_options->setTarget(VISA_CM);
      m_kernel->setKernelType(VISA_CM);
    } else if (attr->value.intVal == 1) {
      m_options->setTarget(VISA_3D);
      m_kernel->setKernelType(VISA_3D);
    } else {
      vISA_ASSERT_INPUT(false, "Invalid kernel target attribute.");
    }
  }

  if (attrID == Attributes::ATTR_Callable) {
    setFCCallableKernel(true);
  } else if (attrID == Attributes::ATTR_Caller) {
    setFCCallerKernel(true);
  } else if (attrID == Attributes::ATTR_Composable) {
    setFCComposableKernel(true);
    if (IS_GEN_BOTH_PATH) {
      m_builder->getFCPatchInfo()->setFCComposableKernel(true);
    }
    m_options->setOption(vISA_loadThreadPayload, false);
  } else if (attrID == Attributes::ATTR_Entry) {
    m_builder->getFCPatchInfo()->setIsEntryKernel(true);
    m_options->setOption(vISA_loadThreadPayload, true);
  } else if (attrID == Attributes::ATTR_RetValSize) {
    if (IS_GEN_BOTH_PATH) {
      if (m_builder->getRetVarSize() < attr->value.intVal) {
        m_builder->setRetVarSize((unsigned short)(attr->value.intVal));
      }
    }
  } else if (attrID == Attributes::ATTR_ArgSize) {
    if (IS_GEN_BOTH_PATH) {
      if (m_builder->getArgSize() < attr->value.intVal) {
        m_builder->setArgSize((unsigned short)(attr->value.intVal));
      }
    }
  }

  addAttribute(attrName, attr);

  if (IsAsmWriterMode()) {
    // Print attribute
    VISAKernel_format_provider fmt(this);
    m_CISABuilder->m_ssIsaAsm << ".kernel_attr "
                              << printOneAttribute(&fmt, attr) << "\n";
  }

  return VISA_SUCCESS;
}

// If AllocMaxNum == 0, return pointer to next attribute and advance attribute
// count; otherwise,  resize attributes with AllocMaxNum and return the base of
// the whole attributes.
attribute_info_t *VISAKernelImpl::allocAttributeImpl(CISA_GEN_VAR *Dcl,
                                                     uint32_t AllocMaxNum) {
  const uint32_t DefaultCapInc = 4; // default size for capacity

  unsigned char *pCap = nullptr;
  unsigned char *pCnt = nullptr;
  attribute_info_t **pAttributes = nullptr;
  switch (Dcl->type) {
  case GENERAL_VAR:
    pCap = &(Dcl->genVar.attribute_capacity);
    pCnt = &(Dcl->genVar.attribute_count);
    pAttributes = &(Dcl->genVar.attributes);
    break;
  case ADDRESS_VAR:
    pCap = &(Dcl->addrVar.attribute_capacity);
    pCnt = &(Dcl->addrVar.attribute_count);
    pAttributes = &(Dcl->addrVar.attributes);
    break;
  case PREDICATE_VAR:
    pCap = &(Dcl->predVar.attribute_capacity);
    pCnt = &(Dcl->predVar.attribute_count);
    pAttributes = &(Dcl->predVar.attributes);
    break;
  case SAMPLER_VAR:
    pCap = &(Dcl->stateVar.attribute_capacity);
    pCnt = &(Dcl->stateVar.attribute_count);
    pAttributes = &(Dcl->stateVar.attributes);
    break;
  case SURFACE_VAR:
    pCap = &(Dcl->stateVar.attribute_capacity);
    pCnt = &(Dcl->stateVar.attribute_count);
    pAttributes = &(Dcl->stateVar.attributes);
    break;
  case LABEL_VAR:
    pCap = &(Dcl->labelVar.attribute_capacity);
    pCnt = &(Dcl->labelVar.attribute_count);
    pAttributes = &(Dcl->labelVar.attributes);
    break;
  default:
    vISA_ASSERT_UNREACHABLE("invalid dcl type");
    return nullptr;
  }

  uint32_t currCap = (*pCap);
  uint32_t currCnt = (*pCnt);
  attribute_info_t *currAttr = (*pAttributes);
  if (AllocMaxNum == 0 && currCap > currCnt) {
    (*pCnt) = currCnt + 1;
    return &currAttr[currCnt];
  } else if (AllocMaxNum > 0 && currCap >= AllocMaxNum) {
    return currAttr;
  }

  uint32_t newCap =
      (AllocMaxNum == 0) ? (currCap + DefaultCapInc) : AllocMaxNum;
  attribute_info_t *newAttr =
      (attribute_info_t *)m_mem.alloc(sizeof(attribute_info_t) * newCap);
  if (currCnt > 0) {
    uint32_t bytesToCopy = sizeof(attribute_info_t) * currCnt;
    memcpy_s(newAttr, bytesToCopy, currAttr, bytesToCopy);
  }

  (*pCap) = newCap;
  (*pAttributes) = newAttr;
  if (AllocMaxNum > 0) {
    return newAttr;
  }
  (*pCnt) = currCnt + 1;
  return &newAttr[currCnt];
}

int VISAKernelImpl::AddAttributeToVarGeneric(CISA_GEN_VAR *decl,
                                             const char *attrName,
                                             unsigned int size,
                                             const void *val) {
  TIME_SCOPE(VISA_BUILDER_CREATE_VAR);

  attribute_info_t *attr = allocAttribute(decl);
  attr->nameIndex = addStringPool(std::string(attrName));
  Attributes::ID aID = Attributes::getAttributeID(attrName);
  vISA_ASSERT_INPUT(Attributes::isVarAttr(aID), "ERROR: unknown var attribute");

  attr->size = (uint8_t)size;
  if (Attributes::isInt32(aID)) {
    attr->isInt = true;
    attr->value.intVal = val ? *((int *)val) : 0;
    vISA_ASSERT(attr->size <= 4,
           "Int32 attribute has a value of 4 bytes at most!");
  } else if (Attributes::isBool(aID)) {
    attr->isInt = true; // treat bool as int in attribute_info_t
    attr->value.intVal = 1;
  } else if (Attributes::isCStr(aID)) {
    attr->isInt = false;
    char *temp = (char *)m_mem.alloc(size + 1); // null-ending C-string
    memcpy_s(temp, size, val, size);
    temp[size] = 0;
    attr->value.stringVal = (const char *)temp;
  } else {
    vISA_ASSERT_INPUT(false, "ERROR: unexpected variable attribute");
  }

  switch (decl->type) {
  case GENERAL_VAR: {
    if (IS_GEN_BOTH_PATH) {
      auto rootDcl = decl->genVar.dcl->getRootDeclare();
      if (Attributes::isAttribute(Attributes::ATTR_Input, attrName) ||
          Attributes::isAttribute(Attributes::ATTR_Input_Output, attrName)) {
        rootDcl->setLiveIn();
      }
      if (Attributes::isAttribute(Attributes::ATTR_Output, attrName) ||
          Attributes::isAttribute(Attributes::ATTR_Input_Output, attrName)) {
        rootDcl->setLiveOut();
      }
      if (Attributes::isAttribute(Attributes::ATTR_NoWidening, attrName)) {
        rootDcl->setDoNotWiden();
      }
      if (Attributes::isAttribute(Attributes::ATTR_DoNotSpill, attrName)) {
        rootDcl->setDoNotSpill();
      }
      if (Attributes::isAttribute(Attributes::ATTR_ForceSpill, attrName)) {
        rootDcl->setForceSpilled();
        vISA_ASSERT_INPUT(!rootDcl->isDoNotSpill(),
                          "DoNotSpill cannot be set together with ForceSpill attribute");
      }
      if (Attributes::isAttribute(Attributes::ATTR_PayloadLiveOut, attrName)) {
        rootDcl->setPayloadLiveOut();
      }
      if (Attributes::isAttribute(Attributes::ATTR_ExclusiveLoad, attrName)) {
        rootDcl->setExclusiveLoad();
      }
    }
    break;
  }
  case ADDRESS_VAR:
  case PREDICATE_VAR:
  case SAMPLER_VAR:
  case SURFACE_VAR:
  case LABEL_VAR:
    break;
  default:
    vISA_ASSERT_UNREACHABLE("invalid dcl type");
    return VISA_FAILURE;
  }

  return VISA_SUCCESS;
}

int VISAKernelImpl::AddAttributeToVar(VISA_PredVar *decl, const char *name,
                                      unsigned int size, void *val) {
  return AddAttributeToVarGeneric((CISA_GEN_VAR *)decl, name, size, val);
}

int VISAKernelImpl::AddAttributeToVar(VISA_SurfaceVar *decl, const char *name,
                                      unsigned int size, void *val) {
  return AddAttributeToVarGeneric((CISA_GEN_VAR *)decl, name, size, val);
}

int VISAKernelImpl::AddAttributeToVar(VISA_GenVar *decl, const char *name,
                                      unsigned int size, void *val) {
  return AddAttributeToVarGeneric((CISA_GEN_VAR *)decl, name, size, val);
}

int VISAKernelImpl::AddAttributeToVar(VISA_AddrVar *decl, const char *name,
                                      unsigned int size, void *val) {
  return AddAttributeToVarGeneric((CISA_GEN_VAR *)decl, name, size, val);
}

void VISAKernelImpl::addVarInfoToList(CISA_GEN_VAR *t) {
  m_var_info_list.push_back(t);
}

void VISAKernelImpl::addSampler(CISA_GEN_VAR *state) {
  m_sampler_info_list.push_back(state);
}

void VISAKernelImpl::addSurface(CISA_GEN_VAR *state) {
  m_surface_info_list.push_back(state);
}

void VISAKernelImpl::addAddrToList(CISA_GEN_VAR *addr) {
  m_addr_info_list.push_back(addr);
}

void VISAKernelImpl::addPredToList(CISA_GEN_VAR *pred) {
  m_pred_info_list.push_back(pred);
}

void VISAKernelImpl::addAttribute(const char *inputName,
                                  attribute_info_t *attrTemp) {
  attrTemp->nameIndex = addStringPool(std::string(inputName));
  m_attribute_info_list.push_back(attrTemp);
  m_attribute_count++;
}

Common_ISA_Input_Class
VISAKernelImpl::GetInputClass(Common_ISA_Var_Class var_class) {
  if (var_class == GENERAL_VAR)
    return INPUT_GENERAL;

  if (var_class == SAMPLER_VAR)
    return INPUT_SAMPLER;

  if (var_class == SURFACE_VAR)
    return INPUT_SURFACE;

  return INPUT_UNKNOWN;
}

int VISAKernelImpl::CreateVISAInputVar(CISA_GEN_VAR *decl, uint16_t offset,
                                       uint16_t size, uint8_t implicitKind) {
  TIME_SCOPE(VISA_BUILDER_CREATE_VAR);

  if (!getIsKernel()) {
    vISA_ASSERT(false, "only kernels may have input variables");
    return VISA_FAILURE;
  }

  unsigned int status = VISA_SUCCESS;
  input_info_t *input = (input_info_t *)m_mem.alloc(sizeof(input_info_t));
  input->kind = GetInputClass(decl->type);
  input->kind |= implicitKind << 3;
  input->setImplicitKind(implicitKind);
  input->index = decl->index;
  input->offset = offset;
  input->size = size;

  if (IS_GEN_BOTH_PATH) {
    G4_Declare *dcl = NULL;
    switch (decl->type) {
    case GENERAL_VAR: {
      dcl = decl->genVar.dcl;
      break;
    }
    case SAMPLER_VAR:
    case SURFACE_VAR: {
      dcl = decl->stateVar.dcl;
      break;
    }
    default: {
      status = VISA_FAILURE;
      break;
    }
    }

    if (status == VISA_SUCCESS) {
      m_builder->bindInputDecl(dcl, offset);
    }
    input->dcl = dcl;
    // used in asm generation
    m_builder->addInputArg(input);
  }
  if (IS_VISA_BOTH_PATH || IsAsmWriterMode()) {

    if ((input->kind & 0x3) == INPUT_UNKNOWN) {
      fprintf(stderr, "Wrong input variable is used");
      status = VISA_FAILURE;
    } else {
      m_input_info_list.push_back(input);
      m_input_count++;

      if (IsAsmWriterMode()) {
        // Print input var
        VISAKernel_format_provider fmt(this);
        m_CISABuilder->m_ssIsaAsm
            << printFuncInput(&fmt, m_printDeclIndex.input_index++,
                              getOptions())
            << "\n";
      }
    }
  }

  // save the G4_declare of "R1" input in builder
  if (m_builder && offset == m_builder->getGRFSize() &&
      size == m_builder->getGRFSize()) {
    m_builder->setInputR1(input->dcl);
  }

  return status;
}

int VISAKernelImpl::CreateVISAImplicitInputVar(VISA_GenVar *decl,
                                               unsigned short offset,
                                               unsigned short size,
                                               unsigned short kind) {
  return CreateVISAInputVar((CISA_GEN_VAR *)decl, offset, size, (uint8_t)kind);
}

int VISAKernelImpl::CreateVISAInputVar(VISA_GenVar *decl, unsigned short offset,
                                       unsigned short size) {
  return CreateVISAInputVar((CISA_GEN_VAR *)decl, offset, size, INPUT_EXPLICIT);
}

int VISAKernelImpl::CreateVISAInputVar(VISA_SamplerVar *decl,
                                       unsigned short offset,
                                       unsigned short size) {
  return CreateVISAInputVar((CISA_GEN_VAR *)decl, offset, size, INPUT_EXPLICIT);
}

int VISAKernelImpl::CreateVISAInputVar(VISA_SurfaceVar *decl,
                                       unsigned short offset,
                                       unsigned short size) {
  return CreateVISAInputVar((CISA_GEN_VAR *)decl, offset, size, INPUT_EXPLICIT);
}

/************* OPERANDS CREATION START ******************/
int VISAKernelImpl::CreateVISAAddressSrcOperand(VISA_VectorOpnd *&opnd,
                                                VISA_AddrVar *decl,
                                                unsigned int offset,
                                                unsigned int width) {
  return CreateVISAAddressOperand(opnd, decl, offset, width, false);
}

int VISAKernelImpl::CreateVISAAddressDstOperand(VISA_VectorOpnd *&opnd,
                                                VISA_AddrVar *decl,
                                                unsigned int offset) {
  return CreateVISAAddressOperand(opnd, decl, offset, 1, true);
}
int VISAKernelImpl::CreateVISAAddressOperand(VISA_VectorOpnd *&cisa_opnd,
                                             VISA_AddrVar *decl,
                                             unsigned int offset,
                                             unsigned int width, bool isDst) {
  TIME_SCOPE(VISA_BUILDER_CREATE_OPND);

  cisa_opnd = static_cast<VISA_VectorOpnd *>(getOpndFromPool());
  if (IS_GEN_BOTH_PATH) {
    G4_Declare *dcl = decl->addrVar.dcl;

    if (!isDst) {
      // FIXME: This does not adhere to the vISA spec, which allows <0;N,1>
      // regions where N is < exec size
      const RegionDesc *rd = width > 1 ? m_builder->getRegionStride1()
                                       : m_builder->getRegionScalar();
      cisa_opnd->g4opnd = m_builder->createSrc(
          dcl->getRegVar(), 0, (uint16_t)offset, rd, dcl->getElemType());
    } else {
      cisa_opnd->g4opnd = m_builder->createDst(
          dcl->getRegVar(),
          0, // opnd->opnd_val.addr_opnd.index, // should we use 0 here?
          (uint16_t)offset, 1, dcl->getElemType());
    }
  }
  if (IS_VISA_BOTH_PATH) {
    cisa_opnd->opnd_type = CISA_OPND_VECTOR;
    cisa_opnd->tag = OPERAND_ADDRESS;
    cisa_opnd->_opnd.v_opnd.tag = OPERAND_ADDRESS;
    cisa_opnd->_opnd.v_opnd.opnd_val.addr_opnd.index = (uint16_t)decl->index;
    cisa_opnd->_opnd.v_opnd.opnd_val.addr_opnd.offset = (uint8_t)offset;
    cisa_opnd->_opnd.v_opnd.opnd_val.addr_opnd.width =
        Get_VISA_Exec_Size_From_Raw_Size(width & 0x1F);
    cisa_opnd->size = (uint16_t)cisa_opnd->_opnd.v_opnd.getSizeInBinary();
  }

  return VISA_SUCCESS;
}

int VISAKernelImpl::CreateVISAAddressOfOperandGeneric(
    VISA_VectorOpnd *&cisa_opnd, CISA_GEN_VAR *decl, unsigned int offset) {
  TIME_SCOPE(VISA_BUILDER_CREATE_OPND);

  int status = VISA_SUCCESS;
  cisa_opnd = static_cast<VISA_VectorOpnd *>(getOpndFromPool());

  if (IS_GEN_BOTH_PATH) {
    G4_Declare *src0Dcl = NULL;
    switch (decl->type) {
    case GENERAL_VAR: {
      src0Dcl = decl->genVar.dcl;
      break;
    }
    case SAMPLER_VAR:
    case SURFACE_VAR: {
      src0Dcl = decl->stateVar.dcl;
      break;
    }
    default:
      vISA_ASSERT_UNREACHABLE("invalid dcl type");
      return VISA_FAILURE;
    }

    // set up to the top level dcl to be addressed
    src0Dcl->setAddressed();
    m_kernel->setHasAddrTaken(true);
    G4_Declare *parentDcl = src0Dcl->getAliasDeclare();
    while (parentDcl) {
      parentDcl->setAddressed();
      parentDcl = parentDcl->getAliasDeclare();
    }
    cisa_opnd->g4opnd =
        m_builder->createAddrExp(src0Dcl->getRegVar(), offset, Type_UW);
  }
  if (IS_VISA_BOTH_PATH) {
    // memset(cisa_opnd,0,sizeof(VISA_opnd));
    switch (decl->type) {
    case GENERAL_VAR: {
      cisa_opnd->tag = OPERAND_GENERAL;
      cisa_opnd->_opnd.v_opnd.tag = OPERAND_GENERAL;
      cisa_opnd->_opnd.v_opnd.opnd_val.gen_opnd.index = decl->index;
      cisa_opnd->_opnd.v_opnd.opnd_val.gen_opnd.row_offset =
          offset / m_builder->numEltPerGRF<Type_UB>();
      VISA_Type type = decl->genVar.getType();
      unsigned int typeSize = TypeSize(GetGenTypeFromVISAType(type));
     vASSERT((offset % typeSize) == 0);
      cisa_opnd->_opnd.v_opnd.opnd_val.gen_opnd.col_offset =
          (offset % m_builder->numEltPerGRF<Type_UB>()) / typeSize;

      cisa_opnd->_opnd.v_opnd.opnd_val.gen_opnd.region =
          (unsigned short)Get_CISA_Region_Val(0);
      cisa_opnd->_opnd.v_opnd.opnd_val.gen_opnd.region |=
          ((unsigned short)Get_CISA_Region_Val(1)) << 4;
      cisa_opnd->_opnd.v_opnd.opnd_val.gen_opnd.region |=
          ((unsigned short)Get_CISA_Region_Val(0)) << 8;
      break;
    }
    case SAMPLER_VAR:
    case SURFACE_VAR: {
      cisa_opnd->tag = OPERAND_STATE;
      cisa_opnd->_opnd.v_opnd.tag = OPERAND_STATE;
      cisa_opnd->_opnd.v_opnd.opnd_val.state_opnd.index = (uint16_t)decl->index;
      cisa_opnd->_opnd.v_opnd.opnd_val.state_opnd.offset = (uint8_t)offset;
      cisa_opnd->_opnd.v_opnd.opnd_val.state_opnd.opnd_class = decl->type;
      if (decl->type == SAMPLER_VAR) {
        cisa_opnd->_opnd.v_opnd.opnd_val.state_opnd.opnd_class =
            STATE_OPND_SAMPLER;
      } else if (decl->type == SURFACE_VAR) {
        cisa_opnd->_opnd.v_opnd.opnd_val.state_opnd.opnd_class =
            STATE_OPND_SURFACE;
      }
      break;
    }
    default:
      vISA_ASSERT_UNREACHABLE("unexpected variable class");
      break;
    }

    cisa_opnd->opnd_type = CISA_OPND_VECTOR;
    cisa_opnd->index = decl->index;
    cisa_opnd->size = (uint16_t)cisa_opnd->_opnd.v_opnd.getSizeInBinary();
  }
  return status;
}

int VISAKernelImpl::CreateVISAAddressOfOperand(VISA_VectorOpnd *&cisa_opnd,
                                               VISA_GenVar *decl,
                                               unsigned int offset) {
  return CreateVISAAddressOfOperandGeneric(cisa_opnd, decl, offset);
}
int VISAKernelImpl::CreateVISAAddressOfOperand(VISA_VectorOpnd *&cisa_opnd,
                                               VISA_SurfaceVar *decl,
                                               unsigned int offset) {
  return CreateVISAAddressOfOperandGeneric(cisa_opnd, decl, offset);
}

int VISAKernelImpl::CreateVISAIndirectGeneralOperand(
    VISA_VectorOpnd *&cisa_opnd, VISA_AddrVar *cisa_decl, VISA_Modifier mod,
    unsigned int addrOffset, unsigned short immediateOffset,
    unsigned short verticalStride, unsigned short width,
    unsigned short horizontalStride, VISA_Type type, bool isDst) {
  TIME_SCOPE(VISA_BUILDER_CREATE_OPND);

  cisa_opnd = static_cast<VISA_VectorOpnd *>(getOpndFromPool());

  if (IS_GEN_BOTH_PATH) {
    G4_Declare *dcl = cisa_decl->addrVar.dcl;
    if (!isDst) {
      if (verticalStride == MAX_UWORD_VALUE)
        verticalStride = UNDEFINED_SHORT;

      const RegionDesc *rd =
          m_builder->createRegionDesc(verticalStride, width, horizontalStride);
      G4_SrcModifier g4_mod = GetGenSrcModFromVISAMod(mod);
      G4_SrcRegRegion *src = m_builder->createIndirectSrc(
          g4_mod, dcl->getRegVar(), 0, (uint16_t)addrOffset, rd,
          GetGenTypeFromVISAType(type), immediateOffset);
      cisa_opnd->g4opnd = src;
    } else {
      auto dst = m_builder->createIndirectDst(
          dcl->getRegVar(), (uint16_t)addrOffset, horizontalStride,
          GetGenTypeFromVISAType(type), immediateOffset);

      cisa_opnd->g4opnd = dst;
    }
  }
  if (IS_VISA_BOTH_PATH) {
    // memset(cisa_opnd,0,sizeof(VISA_opnd));

    cisa_opnd->opnd_type = CISA_OPND_VECTOR;
    cisa_opnd->tag = OPERAND_INDIRECT;
    cisa_opnd->_opnd.v_opnd.tag = OPERAND_INDIRECT;
    cisa_opnd->_opnd.v_opnd.opnd_val.indirect_opnd.index =
        (uint16_t)cisa_decl->index;
    cisa_opnd->_opnd.v_opnd.opnd_val.indirect_opnd.addr_offset =
        (uint8_t)addrOffset;
    cisa_opnd->_opnd.v_opnd.opnd_val.indirect_opnd.indirect_offset =
        immediateOffset;
    cisa_opnd->_opnd.v_opnd.opnd_val.indirect_opnd.bit_property = type;

    if (!isDst) {
      cisa_opnd->_opnd.v_opnd.opnd_val.indirect_opnd.region =
          Create_CISA_Region(
              verticalStride, width,
              horizontalStride); // Get_CISA_Region_Val(horizontal_stride) <<8;

      cisa_opnd->_opnd.v_opnd.tag += mod << 3;
    } else {
      cisa_opnd->_opnd.v_opnd.opnd_val.indirect_opnd.region =
          Get_CISA_Region_Val(horizontalStride) << 8;
    }

    cisa_opnd->size = (uint16_t)cisa_opnd->_opnd.v_opnd.getSizeInBinary();
  }

  return VISA_SUCCESS;
}

int VISAKernelImpl::CreateVISAIndirectSrcOperand(
    VISA_VectorOpnd *&cisa_opnd, VISA_AddrVar *cisa_decl, VISA_Modifier mod,
    unsigned int addrOffset, short immediateOffset,
    unsigned short verticalStride, unsigned short width,
    unsigned short horizontalStride, VISA_Type type) {
  return CreateVISAIndirectGeneralOperand(cisa_opnd, cisa_decl, mod, addrOffset,
                                          immediateOffset, verticalStride,
                                          width, horizontalStride, type, false);
}

int VISAKernelImpl::CreateVISAIndirectDstOperand(
    VISA_VectorOpnd *&cisa_opnd, VISA_AddrVar *decl, unsigned int addrOffset,
    short immediateOffset, unsigned short horizontalStride, VISA_Type type) {
  return CreateVISAIndirectGeneralOperand(cisa_opnd, decl, MODIFIER_NONE,
                                          addrOffset, immediateOffset, -1, -1,
                                          horizontalStride, type, true);
}

int VISAKernelImpl::CreateVISAIndirectOperandVxH(VISA_VectorOpnd *&cisa_opnd,
                                                 VISA_AddrVar *decl,
                                                 VISA_Modifier mod,
                                                 unsigned int addrOffset,
                                                 short immediateOffset,
                                                 VISA_Type type) {
  return CreateVISAIndirectGeneralOperand(cisa_opnd, decl, mod,
                                          addrOffset, immediateOffset, -1, 1, 0,
                                          type, false);
}

int VISAKernelImpl::CreateVISAPredicateSrcOperand(VISA_VectorOpnd *&opnd,
                                                  VISA_PredVar *decl,
                                                  unsigned int size) {
  TIME_SCOPE(VISA_BUILDER_CREATE_OPND);

  int status = VISA_SUCCESS;

  vISA_ASSERT_INPUT(decl->type == PREDICATE_VAR, "expect a predicate variable");

  opnd = static_cast<VISA_VectorOpnd *>(getOpndFromPool());

  if (IS_GEN_BOTH_PATH) {
    G4_Declare *dcl = decl->predVar.dcl;
    const RegionDesc *rd = m_builder->getRegionScalar();

    G4_Type type = Type_UW;

    if (size == 32)
      type = Type_UD;

    opnd->g4opnd = m_builder->createSrc(dcl->getRegVar(), 0, 0, rd, type);
  }
  if (IS_VISA_BOTH_PATH) {
    status = CreateVISAPredicateOperandvISA(
        (VISA_PredOpnd *&)opnd, decl, PredState_NO_INVERSE, PRED_CTRL_NON);
  }

  return status;
}

int VISAKernelImpl::CreateVISAPredicateDstOperand(VISA_VectorOpnd *&opnd,
                                                  VISA_PredVar *decl,
                                                  uint32_t size) {
  TIME_SCOPE(VISA_BUILDER_CREATE_OPND);

  int status = VISA_SUCCESS;

  opnd = static_cast<VISA_VectorOpnd *>(getOpndFromPool());
  if (IS_GEN_BOTH_PATH) {
    G4_Declare *dcl = decl->predVar.dcl;

    opnd->g4opnd = m_builder->createDst(dcl->getRegVar(), 0, 0, 1,
                                        size == 32 ? Type_UD : Type_UW);
  }
  if (IS_VISA_BOTH_PATH) {
    status = CreateVISAPredicateOperandvISA(
        (VISA_PredOpnd *&)opnd, decl, PredState_NO_INVERSE, PRED_CTRL_NON);
  }

  return status;
}

int VISAKernelImpl::CreateVISAPredicateOperandvISA(
    VISA_PredOpnd *&cisa_opnd, VISA_PredVar *decl, VISA_PREDICATE_STATE state,
    VISA_PREDICATE_CONTROL cntrl) {
  int status = VISA_SUCCESS;
  cisa_opnd->opnd_type = CISA_OPND_VECTOR;
  cisa_opnd->tag = OPERAND_PREDICATE;
  cisa_opnd->_opnd.v_opnd.tag = OPERAND_PREDICATE;

  cisa_opnd->index = decl->index;
  PredicateOpnd predOpnd(decl->index, state, cntrl);
  cisa_opnd->_opnd.v_opnd.opnd_val.pred_opnd.index = predOpnd.getPredInBinary();
  cisa_opnd->size = (uint16_t)cisa_opnd->_opnd.v_opnd.getSizeInBinary();
  return status;
}

int VISAKernelImpl::CreateVISAPredicateOperand(VISA_PredOpnd *&cisa_opnd,
                                               VISA_PredVar *decl,
                                               VISA_PREDICATE_STATE state,
                                               VISA_PREDICATE_CONTROL cntrl) {
  TIME_SCOPE(VISA_BUILDER_CREATE_OPND);

  cisa_opnd = static_cast<VISA_PredOpnd *>(getOpndFromPool());
  if (IS_GEN_BOTH_PATH) {
    G4_Declare *dcl = decl->predVar.dcl;
#if START_ASSERT_CHECK
    if (dcl == NULL) {
      vASSERT(false);
      return VISA_FAILURE;
    }
#endif
    // with bool size in bytes was incorrect, now that it's correct it returns
    // "correct" number of elements. Except it thinks each element is a two
    // bytes. we want each element in as a boolean.
    vISA::G4_Predicate_Control predCtrl = m_builder->vISAPredicateToG4Predicate(
        cntrl, G4_ExecSize(dcl->getNumberFlagElements()));

    cisa_opnd->g4opnd = m_builder->createPredicate(
        (state == PredState_INVERSE) ? PredState_Minus : PredState_Plus,
        dcl->getRegVar(), 0, predCtrl);
  }
  if (IS_VISA_BOTH_PATH) {
    CreateVISAPredicateOperandvISA(cisa_opnd, decl, state, cntrl);
  }

  return VISA_SUCCESS;
}

int VISAKernelImpl::CreateVISASrcOperand(
    VISA_VectorOpnd *&cisa_opnd, VISA_GenVar *cisa_decl, VISA_Modifier mod,
    unsigned short vStride, unsigned short width, unsigned short hStride,
    unsigned char rowOffset, unsigned char colOffset) {
  TIME_SCOPE(VISA_BUILDER_CREATE_OPND);

  cisa_opnd = static_cast<VISA_VectorOpnd *>(getOpndFromPool());
  if (IS_GEN_BOTH_PATH) {
    if (cisa_decl->index < Get_CISA_PreDefined_Var_Count()) {
      cisa_opnd->g4opnd = CommonISABuildPreDefinedSrc(
          cisa_decl->index, vStride, width, hStride, rowOffset, colOffset, mod);
    } else {

      // create reg region
      G4_Declare *dcl = cisa_decl->genVar.dcl;
      const RegionDesc *rd =
          m_builder->createRegionDesc(vStride, width, hStride);
      G4_SrcModifier g4_mod = GetGenSrcModFromVISAMod(mod);

      cisa_opnd->g4opnd = m_builder->createSrcRegRegion(
          g4_mod, Direct, dcl->getRegVar(), rowOffset, colOffset, rd,
          dcl->getElemType());
    }
  }
  if (IS_VISA_BOTH_PATH) {
    cisa_opnd->opnd_type = CISA_OPND_VECTOR;
    cisa_opnd->tag =
        OPERAND_GENERAL; //<--- I think this is redundant at this point.
    cisa_opnd->_opnd.v_opnd.tag = OPERAND_GENERAL;
    cisa_opnd->_opnd.v_opnd.opnd_val.gen_opnd.index = cisa_decl->index;
    cisa_opnd->index = cisa_opnd->_opnd.v_opnd.opnd_val.gen_opnd.index;
    cisa_opnd->_opnd.v_opnd.opnd_val.gen_opnd.row_offset = rowOffset;
    cisa_opnd->_opnd.v_opnd.opnd_val.gen_opnd.col_offset = colOffset;
    cisa_opnd->_opnd.v_opnd.opnd_val.gen_opnd.region =
        Create_CISA_Region(vStride, width, hStride);

    cisa_opnd->_opnd.v_opnd.tag += mod << 3;

    cisa_opnd->size = (uint16_t)cisa_opnd->_opnd.v_opnd.getSizeInBinary();
  }

  return VISA_SUCCESS;
}

int VISAKernelImpl::CreateVISADstOperand(VISA_VectorOpnd *&cisa_opnd,
                                         VISA_GenVar *cisa_decl,
                                         unsigned short hStride,
                                         unsigned char rowOffset,
                                         unsigned char colOffset) {
  TIME_SCOPE(VISA_BUILDER_CREATE_OPND);

  cisa_opnd = static_cast<VISA_VectorOpnd *>(getOpndFromPool());
  if (IS_GEN_BOTH_PATH) {
    G4_Declare *dcl = cisa_decl->genVar.dcl;
    G4_Declare *aliasDcl = dcl->getRootDeclare();

    // replace vISA %null variable with a null dst to avoid confusing later
    // passes
    if (cisa_decl->type == GENERAL_VAR &&
        (cisa_decl->index == 0 ||
         (aliasDcl && strcmp(aliasDcl->getName(), "%null") == 0))) {
      cisa_opnd->g4opnd = m_builder->createNullDst(dcl->getElemType());
    } else {
      cisa_opnd->g4opnd = m_builder->createDst(
          dcl->getRegVar(), rowOffset, colOffset, hStride, dcl->getElemType());
    }
  }

  if (IS_VISA_BOTH_PATH) {
    cisa_opnd->opnd_type = CISA_OPND_VECTOR;
    cisa_opnd->tag =
        OPERAND_GENERAL; //<--- I think this is redundant at this point.
    cisa_opnd->_opnd.v_opnd.tag = OPERAND_GENERAL;
    cisa_opnd->_opnd.v_opnd.opnd_val.gen_opnd.index = cisa_decl->index;
    cisa_opnd->index = cisa_opnd->_opnd.v_opnd.opnd_val.gen_opnd.index;
    cisa_opnd->_opnd.v_opnd.opnd_val.gen_opnd.row_offset = rowOffset;
    cisa_opnd->_opnd.v_opnd.opnd_val.gen_opnd.col_offset = colOffset;
    cisa_opnd->_opnd.v_opnd.opnd_val.gen_opnd.region =
        Get_CISA_Region_Val(hStride) << 8;

    cisa_opnd->size = (uint16_t)cisa_opnd->_opnd.v_opnd.getSizeInBinary();
  }

  return VISA_SUCCESS;
}

int VISAKernelImpl::CreateVISAImmediate(VISA_VectorOpnd *&cisa_opnd,
                                        const void *value, VISA_Type isaType) {
  TIME_SCOPE(VISA_BUILDER_CREATE_OPND);

  cisa_opnd = static_cast<VISA_VectorOpnd *>(getOpndFromPool());
  if (IS_GEN_BOTH_PATH) {
    G4_Type g4type = GetGenTypeFromVISAType(isaType);

    if (isaType == ISA_TYPE_Q) {
      cisa_opnd->g4opnd = m_builder->createImmWithLowerType(
          *static_cast<const int64_t *>(value), Type_Q);
    } else if (isaType == ISA_TYPE_UQ) {
      cisa_opnd->g4opnd = m_builder->createImmWithLowerType(
          *static_cast<const int64_t *>(value), Type_UQ);
    } else if (isaType == ISA_TYPE_DF) {
      cisa_opnd->g4opnd =
          m_builder->createDFImm(*static_cast<const double *>(value));
    } else if (isaType == ISA_TYPE_F) {
      cisa_opnd->g4opnd =
          m_builder->createImm(*static_cast<const float *>(value));
    } else if (isaType == ISA_TYPE_HF) {
      cisa_opnd->g4opnd = m_builder->createImmWithLowerType(
          *static_cast<const unsigned *>(value), Type_HF);
    } else {
      int64_t tmpValue = typecastVals(value, isaType);
      cisa_opnd->g4opnd = m_builder->createImmWithLowerType(tmpValue, g4type);
    }
  }
  if (IS_VISA_BOTH_PATH) {
    cisa_opnd->opnd_type = CISA_OPND_VECTOR;
    cisa_opnd->tag = OPERAND_IMMEDIATE;
    cisa_opnd->_opnd.v_opnd.tag = OPERAND_IMMEDIATE;
    cisa_opnd->_opnd.v_opnd.opnd_val.const_opnd.type = isaType;

    int size = CISATypeTable[isaType].typeSize;

    if (size == 0) {
      vASSERT(false);
      return VISA_FAILURE;
    }
    if (isaType == ISA_TYPE_DF) {
      cisa_opnd->_opnd.v_opnd.opnd_val.const_opnd._val.dval =
          *static_cast<const double *>(value);
    } else if (isaType == ISA_TYPE_F) {
      cisa_opnd->_opnd.v_opnd.opnd_val.const_opnd._val.fval =
          *static_cast<const float *>(value);
    } else if (isaType == ISA_TYPE_Q || isaType == ISA_TYPE_UQ) {
      cisa_opnd->_opnd.v_opnd.opnd_val.const_opnd._val.lval =
          *static_cast<const uint64_t *>(value);
    } else if (isaType == ISA_TYPE_V || isaType == ISA_TYPE_UV) {
      int size = Get_VISA_Type_Size(isaType);
      memcpy_s(&cisa_opnd->_opnd.v_opnd.opnd_val.const_opnd._val, size, value,
               size);
    } else {
      int64_t tmpValue = typecastVals(value, isaType);
      cisa_opnd->_opnd.v_opnd.opnd_val.const_opnd._val.lval = tmpValue;
    }
    cisa_opnd->size = (uint16_t)cisa_opnd->_opnd.v_opnd.getSizeInBinary();
  }

  return VISA_SUCCESS;
}

int VISAKernelImpl::CreateVISAStateOperand(
    VISA_VectorOpnd *&cisa_opnd, CISA_GEN_VAR *decl,
    Common_ISA_State_Opnd_Class opndClass, uint8_t size, unsigned char offset,
    bool useAsDst) {
  TIME_SCOPE(VISA_BUILDER_CREATE_OPND);

  cisa_opnd = static_cast<VISA_VectorOpnd *>(getOpndFromPool());
  if (IS_GEN_BOTH_PATH) {
    G4_Declare *dcl = decl->stateVar.dcl;

    if (!useAsDst) {
      // pre-defined surface
      if (opndClass == STATE_OPND_SURFACE &&
          decl->index < Get_CISA_PreDefined_Surf_Count()) {
        int64_t immVal =
            Get_PreDefined_Surf_Index(decl->index, m_builder->getPlatform());
        if (immVal == PREDEF_SURF_252) {
          // we have to keep it as a variable
          cisa_opnd->g4opnd = m_builder->createSrcRegRegion(
              m_builder->getBuiltinT252(), m_builder->getRegionScalar());
        } else {
          if (m_options->getOption(vISA_noncoherentStateless) &&
              immVal == PREDEF_SURF_255) {
            immVal = PREDEF_SURF_253;
          }
          cisa_opnd->g4opnd = m_builder->createImm(immVal, Type_UD);
        }
      } else {

        cisa_opnd->g4opnd =
            m_builder->createSrc(dcl->getRegVar(), 0, offset,
                                 size == 1 ? m_builder->getRegionScalar()
                                           : m_builder->getRegionStride1(),
                                 dcl->getElemType());
      }
    } else {
      cisa_opnd->g4opnd = m_builder->createDst(dcl->getRegVar(), 0, offset, 1,
                                               dcl->getElemType());
    }
  }
  if (IS_VISA_BOTH_PATH) {

    cisa_opnd->opnd_type = CISA_OPND_VECTOR;
    cisa_opnd->tag = OPERAND_STATE;
    cisa_opnd->_opnd.v_opnd.tag = OPERAND_STATE;

    cisa_opnd->index = decl->index;
    cisa_opnd->_opnd.v_opnd.opnd_val.state_opnd.index =
        (unsigned short)(cisa_opnd->index);
    cisa_opnd->_opnd.v_opnd.opnd_val.state_opnd.offset = offset;
    cisa_opnd->_opnd.v_opnd.opnd_val.state_opnd.opnd_class = opndClass;
    cisa_opnd->size = (uint16_t)cisa_opnd->_opnd.v_opnd.getSizeInBinary();
  }

  return VISA_SUCCESS;
}

int VISAKernelImpl::CreateVISAStateOperand(VISA_VectorOpnd *&opnd,
                                           VISA_SurfaceVar *decl, uint8_t size,
                                           unsigned char offset,
                                           bool useAsDst) {
  return CreateVISAStateOperand(opnd, (CISA_GEN_VAR *)decl, STATE_OPND_SURFACE,
                                size, offset, useAsDst);
}

int VISAKernelImpl::CreateVISAStateOperand(VISA_VectorOpnd *&opnd,
                                           VISA_SurfaceVar *decl,
                                           unsigned char offset,
                                           bool useAsDst) {
  return CreateVISAStateOperand(opnd, (CISA_GEN_VAR *)decl, STATE_OPND_SURFACE,
                                1, offset, useAsDst);
}

int VISAKernelImpl::CreateVISAStateOperand(VISA_VectorOpnd *&opnd,
                                           VISA_SamplerVar *decl,
                                           unsigned char offset,
                                           bool useAsDst) {
  return CreateVISAStateOperand(opnd, (CISA_GEN_VAR *)decl, STATE_OPND_SAMPLER,
                                1, offset, useAsDst);
}

int VISAKernelImpl::CreateVISAStateOperand(VISA_VectorOpnd *&opnd,
                                           VISA_SamplerVar *decl, uint8_t size,
                                           unsigned char offset,
                                           bool useAsDst) {
  return CreateVISAStateOperand(opnd, (CISA_GEN_VAR *)decl, STATE_OPND_SAMPLER,
                                size, offset, useAsDst);
}

// size should be 8 since it's a vISA spec.
// size was added because VME doesn't adhere to our spec.
int VISAKernelImpl::CreateVISARawOperand(VISA_RawOpnd *&cisa_opnd,
                                         VISA_GenVar *decl,
                                         unsigned short offset) {
  TIME_SCOPE(VISA_BUILDER_CREATE_OPND);

  cisa_opnd = static_cast<VISA_RawOpnd *>(getOpndFromPool());
  cisa_opnd->opnd_type = CISA_OPND_RAW;
  cisa_opnd->tag = NUM_OPERAND_CLASS;
  cisa_opnd->index = decl->index;
  cisa_opnd->_opnd.r_opnd.index = cisa_opnd->index;
  cisa_opnd->_opnd.r_opnd.offset = offset;
  cisa_opnd->size = sizeof(cisa_opnd->_opnd.r_opnd.index) +
                    sizeof(cisa_opnd->_opnd.r_opnd.offset);
  cisa_opnd->decl = decl;

  return VISA_SUCCESS;
}

// size == 1 means we want scalar region
// FIXME: this is really a bug, rawOpnd should always occupy at least 1 GRF.
// We should fix the vISA spec for this
//  [noTypeChange: keep the original type. This likely needs refactoring]
int VISAKernelImpl::CreateGenRawSrcOperand(VISA_RawOpnd *&cisa_opnd) {
  TIME_SCOPE(VISA_BUILDER_CREATE_OPND);

  if (cisa_opnd->index == 0) {
    return CreateGenNullRawOperand(cisa_opnd, false);
  }

  unsigned short offset = cisa_opnd->_opnd.r_opnd.offset;

  G4_Declare *dcl = cisa_opnd->decl->genVar.dcl;

  const RegionDesc *rd = m_builder->getRegionStride1();
  G4_Type type = dcl->getElemType();
  short row_offset = offset / m_builder->numEltPerGRF<Type_UB>();
  short col_offset =
      (offset % m_builder->numEltPerGRF<Type_UB>()) / TypeSize(type);

  cisa_opnd->g4opnd =
      m_builder->createSrc(dcl->getRegVar(), row_offset, col_offset, rd, type);

  return VISA_SUCCESS;
}

int VISAKernelImpl::CreateGenRawDstOperand(VISA_RawOpnd *&cisa_opnd) {
  TIME_SCOPE(VISA_BUILDER_CREATE_OPND);

  if (cisa_opnd->index == 0) {
    return CreateGenNullRawOperand(cisa_opnd, true);
  }

  unsigned short offset = cisa_opnd->_opnd.r_opnd.offset;

  // if (IS_GEN_BOTH_PATH) //will only be called in GEN or BOTH path
  {
    G4_Declare *dcl = cisa_opnd->decl->genVar.dcl;

    {
      short row_offset = offset / m_builder->numEltPerGRF<Type_UB>();
      short col_offset =
          (offset % m_builder->numEltPerGRF<Type_UB>()) / dcl->getElemSize();

      cisa_opnd->g4opnd = m_builder->createDst(
          dcl->getRegVar(), row_offset, col_offset, 1, dcl->getElemType());
    }
  }

  return VISA_SUCCESS;
}

int VISAKernelImpl::CreateStateInstUse(VISA_StateOpndHandle *&cisa_opnd,
                                       unsigned int index) {
  TIME_SCOPE(VISA_BUILDER_CREATE_OPND);

  cisa_opnd->_opnd.other_opnd = index;
  cisa_opnd->opnd_type = CISA_OPND_OTHER;
  cisa_opnd->size = (uint16_t)Get_VISA_Type_Size(ISA_TYPE_UB);
  cisa_opnd->tag = ISA_TYPE_UB;

  return VISA_SUCCESS;
}
int VISAKernelImpl::CreateStateInstUseFastPath(VISA_StateOpndHandle *&cisa_opnd,
                                               CISA_GEN_VAR *decl) {
  TIME_SCOPE(VISA_BUILDER_CREATE_OPND);

  unsigned int status = VISA_SUCCESS;
  G4_Declare *dcl = decl->stateVar.dcl;

  Common_ISA_Var_Class type = decl->type;

  switch (type) {
  case SAMPLER_VAR: {
    cisa_opnd->g4opnd = m_builder->createSrc(
        dcl->getRegVar(), 0, 0, m_builder->getRegionScalar(), Type_UD);
    break;
  }
  case SURFACE_VAR: {
    uint16_t surf_id = (uint16_t)decl->index;
    if (surf_id >= Get_CISA_PreDefined_Surf_Count()) {
      cisa_opnd->g4opnd = m_builder->createSrc(
          dcl->getRegVar(), 0, 0, m_builder->getRegionScalar(), Type_UD);
    } else { // predefined
      if (dcl == m_builder->getBuiltinT252()) {
        cisa_opnd->g4opnd = m_builder->createSrcRegRegion(
            m_builder->getBuiltinT252(), m_builder->getRegionScalar());
      } else if (dcl == m_builder->getBuiltinScratchSurface()) {
        cisa_opnd->g4opnd =
            m_builder->createSrcRegRegion(m_builder->getBuiltinScratchSurface(),
                                          m_builder->getRegionScalar());
      } else {
        int64_t immVal =
            Get_PreDefined_Surf_Index(decl->index, m_builder->getPlatform());
        if (m_options->getOption(vISA_noncoherentStateless) &&
            immVal == PREDEF_SURF_255) {
          immVal = PREDEF_SURF_253;
        }
        cisa_opnd->g4opnd = m_builder->createImm(immVal, Type_UD);
      }
    }
    break;
  }
  default: {
    vISA_ASSERT_UNREACHABLE("invalid dcl type");
    status = VISA_FAILURE;
    break;
  }
  }

  return status;
}
int VISAKernelImpl::CreateVISAStateOperandHandle(VISA_StateOpndHandle *&opnd,
                                                 VISA_SurfaceVar *decl) {
  int status = VISA_SUCCESS;
  opnd = static_cast<VISA_StateOpndHandle *>(getOpndFromPool());
  if (IS_GEN_BOTH_PATH) {
    status = CreateStateInstUseFastPath(opnd, (CISA_GEN_VAR *)decl);
  }
  if (IS_VISA_BOTH_PATH && status == VISA_SUCCESS) {
    status = CreateStateInstUse(opnd, decl->index);
  }
  return status;
}

int VISAKernelImpl::CreateVISAStateOperandHandle(VISA_StateOpndHandle *&opnd,
                                                 VISA_SamplerVar *decl) {
  int status = VISA_SUCCESS;
  opnd = static_cast<VISA_StateOpndHandle *>(getOpndFromPool());
  if (IS_GEN_BOTH_PATH) {
    status = CreateStateInstUseFastPath(opnd, (CISA_GEN_VAR *)decl);
  }
  if (IS_VISA_BOTH_PATH && status == VISA_SUCCESS) {
    status = CreateStateInstUse(opnd, decl->index);
  }
  return status;
}

int VISAKernelImpl::CreateVISANullRawOperand(VISA_RawOpnd *&cisa_opnd,
                                             bool isDst) {
  TIME_SCOPE(VISA_BUILDER_CREATE_OPND);

  cisa_opnd = static_cast<VISA_RawOpnd *>(getOpndFromPool());
  cisa_opnd->opnd_type = CISA_OPND_RAW;
  cisa_opnd->tag = NUM_OPERAND_CLASS;
  cisa_opnd->index = 0;
  cisa_opnd->_opnd.r_opnd.index = cisa_opnd->index;
  cisa_opnd->_opnd.r_opnd.offset = 0;
  cisa_opnd->size = sizeof(cisa_opnd->_opnd.r_opnd.index) +
                    sizeof(cisa_opnd->_opnd.r_opnd.offset);

  int ret = CreateGenNullRawOperand(cisa_opnd, isDst);
  return ret;
}

/**
 * It's expected that the null raw operands had already been pre-allocated.
 */
int VISAKernelImpl::CreateGenNullRawOperand(VISA_RawOpnd *&cisa_opnd,
                                            bool isDst) {
  if (IS_GEN_BOTH_PATH) {
    if (isDst) {
      cisa_opnd->g4opnd = m_builder->createNullDst(Type_UD);
    } else {
      cisa_opnd->g4opnd = m_builder->createNullSrc(Type_UD);
    }
  }

  return VISA_SUCCESS;
}

int VISAKernelImpl::GetPredefinedVar(VISA_GenVar *&predDcl,
                                     PreDefined_Vars varName) {
  int status = VISA_SUCCESS;
  predDcl = (VISA_GenVar *)getGenVar(varName);
  return status;
}

int VISAKernelImpl::GetPredefinedSurface(VISA_SurfaceVar *&predDcl,
                                         PreDefined_Surface surfaceName) {
  int status = VISA_SUCCESS;
  predDcl = (VISA_SurfaceVar *)(m_surface_info_list[surfaceName]);
  return status;
}

int VISAKernelImpl::GetBindlessSampler(VISA_SamplerVar *&samplerDcl) {
  int status = VISA_SUCCESS;
  samplerDcl = m_bindlessSampler;
  return status;
}
/************* OPERANDS CREATION END   ******************/

/************* START APPEND APIS **********************/
int VISAKernelImpl::AppendVISAArithmeticInst(
    ISA_Opcode opcode, VISA_PredOpnd *pred, bool satMode, VISA_EMask_Ctrl emask,
    VISA_Exec_Size executionSize, VISA_VectorOpnd *tmpDst,
    VISA_VectorOpnd *src0, VISA_VectorOpnd *src1, VISA_VectorOpnd *src2) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  if (IS_GEN_BOTH_PATH) {
    const G4_Sat g4SatMode = satMode ? g4::SAT : g4::NOSAT;
    G4_Predicate *g4Pred = (pred != NULL) ? pred->g4opnd->asPredicate() : NULL;
    if ((tmpDst->g4opnd->getType() == Type_DF) &&
        ((opcode == ISA_DIV) || // keeping ISA_DIV at the moment for IGC
         (opcode == ISA_DIVM))) {
      status = m_builder->translateVISAArithmeticDoubleInst(
          opcode, executionSize, emask, g4Pred, g4SatMode,
          tmpDst->g4opnd->asDstRegRegion(), src0->g4opnd, src1->g4opnd); // IEEE
    } else if (tmpDst->g4opnd->getType() == Type_DF && opcode == ISA_INV) {
      // src0_opnd is divisor
      status = m_builder->translateVISAArithmeticDoubleInst(
          opcode, executionSize, emask, g4Pred, g4SatMode,
          tmpDst->g4opnd->asDstRegRegion(), NULL, src0->g4opnd); // IEEE
    } else if (tmpDst->g4opnd->getType() == Type_F && opcode == ISA_DIVM) {
      status = m_builder->translateVISAArithmeticSingleDivideIEEEInst(
          opcode, executionSize, emask, g4Pred, g4SatMode, nullptr,
          tmpDst->g4opnd->asDstRegRegion(), src0->g4opnd, src1->g4opnd); // IEEE
    } else if (tmpDst->g4opnd->getType() == Type_F && opcode == ISA_SQRTM) {
      status = m_builder->translateVISAArithmeticSingleSQRTIEEEInst(
          opcode, executionSize, emask, g4Pred, g4SatMode, nullptr,
          tmpDst->g4opnd->asDstRegRegion(), src0->g4opnd); // IEEE
    } else if (tmpDst->g4opnd->getType() == Type_DF &&
               (opcode == ISA_SQRT || opcode == ISA_SQRTM)) {
      status = m_builder->translateVISAArithmeticDoubleSQRTInst(
          opcode, executionSize, emask, g4Pred, g4SatMode, nullptr,
          tmpDst->g4opnd->asDstRegRegion(), src0->g4opnd);
    } else {
      status = m_builder->translateVISAArithmeticInst(
          opcode, executionSize, emask, g4Pred, g4SatMode, nullptr,
          tmpDst->g4opnd->asDstRegRegion(), src0->g4opnd, GET_G4_OPNG(src1),
          GET_G4_OPNG(src2), NULL);
    }
  }
  if (IS_VISA_BOTH_PATH) {
    int num_pred_desc_operands =
        2; // accounting for exec_size and pred_id in descriptor
    int num_operands = 0;
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[4];
    VISA_opnd *dst = tmpDst;

    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    VISA_Modifier mod = MODIFIER_NONE;

    if (satMode) {
      mod = MODIFIER_SAT;
      dst = (VISA_opnd *)m_mem.alloc(sizeof(VISA_opnd));
      *dst = *tmpDst;
      dst->_opnd.v_opnd.tag += mod << 3;
    }

    ADD_OPND(num_operands, opnd, dst);

    ADD_OPND(num_operands, opnd, src0);

    ADD_OPND(num_operands, opnd, src1);

    ADD_OPND(num_operands, opnd, src2);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();
    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += emask << 4;
    inst->createCisaInstruction(opcode, size, 0, predOpnd, opnd, num_operands,
                                inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAArithmeticInst(ISA_Opcode opcode,
                                             VISA_PredOpnd *pred, bool satMode,
                                             VISA_EMask_Ctrl emask,
                                             VISA_Exec_Size executionSize,
                                             VISA_VectorOpnd *tmpDst,
                                             VISA_VectorOpnd *src0) {
  return AppendVISAArithmeticInst(opcode, pred, satMode, emask, executionSize,
                                  tmpDst, src0, NULL, NULL);
}

int VISAKernelImpl::AppendVISAArithmeticInst(
    ISA_Opcode opcode, VISA_PredOpnd *pred, bool satMode, VISA_EMask_Ctrl emask,
    VISA_Exec_Size executionSize, VISA_VectorOpnd *tmpDst,
    VISA_VectorOpnd *src0, VISA_VectorOpnd *src1) {
  return AppendVISAArithmeticInst(opcode, pred, satMode, emask, executionSize,
                                  tmpDst, src0, src1, NULL);
}

int VISAKernelImpl::AppendVISATwoDstArithmeticInst(
    ISA_Opcode opcode, VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
    VISA_Exec_Size executionSize, VISA_VectorOpnd *dst1,
    VISA_VectorOpnd *carry_borrow, VISA_VectorOpnd *src0,
    VISA_VectorOpnd *src1) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    G4_Predicate *g4Pred = (pred != NULL) ? pred->g4opnd->asPredicate() : NULL;
    status = m_builder->translateVISAArithmeticInst(
        opcode, executionSize, emask, g4Pred, g4::NOSAT, nullptr,
        dst1->g4opnd->asDstRegRegion(), src0->g4opnd, src1->g4opnd, NULL,
        carry_borrow->g4opnd->asDstRegRegion());
  }
  if (IS_VISA_BOTH_PATH) {
    int num_pred_desc_operands =
        2; // accounting for exec_size and pred_id in descriptor
    int num_operands = 0;
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[4];

    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);

    if (opcode != ISA_ADDC && opcode != ISA_SUBB) {
      vASSERT(false);
      std::cerr << "ONLY ADDC AND SUBB are supported by this API\n";
      return VISA_FAILURE;
    }

    ADD_OPND(num_operands, opnd, dst1);

    ADD_OPND(num_operands, opnd, carry_borrow);

    ADD_OPND(num_operands, opnd, src0);

    ADD_OPND(num_operands, opnd, src1);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();
    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += emask << 4;
    status = inst->createCisaInstruction(opcode, size, 0, predOpnd, opnd,
                                         num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAPredDstArithmeticInst(
    ISA_Opcode opcode, VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
    VISA_Exec_Size executionSize, VISA_VectorOpnd *vecDst,
    VISA_PredVar *predDst, VISA_VectorOpnd *src0, VISA_VectorOpnd *src1) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  if (!(opcode == ISA_INVM || opcode == ISA_RSQTM)) {
    std::stringstream msg;
    msg << "Two-destination instruction (dst, pred) not "
        << "supported for opcode: " << ISA_Inst_Table[opcode].str;
    vISA_ASSERT_INPUT(false, msg.str().c_str());
    return VISA_FAILURE;
  }

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  uint32_t exSize = Get_VISA_Exec_Size(executionSize);
  VISA_VectorOpnd *predOpnd= NULL;
  CreateVISAPredicateDstOperand(predOpnd, predDst, exSize);
  G4_Predicate *g4Pred = (pred != NULL) ? pred->g4opnd->asPredicate() : NULL;
  if (IS_GEN_BOTH_PATH) {
    status = m_builder->translateVISAInvmRsqtmInst(
        opcode, executionSize, emask, g4Pred, g4::NOSAT,
        vecDst->g4opnd->asDstRegRegion(), predDst, src0->g4opnd,
        opcode == ISA_INVM ? src1->g4opnd : nullptr);
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[4];
    int num_pred_desc_operands = 1;
    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    int num_operands = 0;

    ADD_OPND(num_operands, opnd, vecDst);

    ADD_OPND(num_operands, opnd, predOpnd);

    ADD_OPND(num_operands, opnd, src0);

    ADD_OPND(num_operands, opnd, src1);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    unsigned char size = executionSize;
    size += emask << 4;
    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);
    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISALogicOrShiftInst(
    ISA_Opcode opcode, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISA_PredVar *dst, VISA_PredVar *src0, VISA_PredVar *src1) {
  VISA_VectorOpnd *dstOpnd = NULL;
  VISA_VectorOpnd *src0Opnd = NULL;
  VISA_VectorOpnd *src1Opnd = NULL;
  uint32_t exSize = Get_VISA_Exec_Size(executionSize);
  CreateVISAPredicateDstOperand(dstOpnd, dst, exSize);
  CreateVISAPredicateSrcOperand(src0Opnd, src0, exSize);
  if (src1 != NULL)
    CreateVISAPredicateSrcOperand(src1Opnd, src1, exSize);

  return AppendVISALogicOrShiftInst(opcode, NULL, false, emask, executionSize,
                                    dstOpnd, src0Opnd, src1Opnd, NULL, NULL);
}

int VISAKernelImpl::AppendVISALogicOrShiftInst(
    ISA_Opcode opcode, VISA_PredOpnd *pred, bool satMode, VISA_EMask_Ctrl emask,
    VISA_Exec_Size executionSize, VISA_VectorOpnd *tmpDst,
    VISA_VectorOpnd *src0, VISA_VectorOpnd *src1, VISA_VectorOpnd *src2,
    VISA_VectorOpnd *src3) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    G4_Predicate *g4Pred = (pred != NULL) ? pred->g4opnd->asPredicate() : NULL;
    status = m_builder->translateVISALogicInst(
        opcode, g4Pred, satMode ? g4::SAT : g4::NOSAT, executionSize, emask,
        tmpDst->g4opnd->asDstRegRegion(), src0->g4opnd, GET_G4_OPNG(src1),
        GET_G4_OPNG(src2), GET_G4_OPNG(src3));
  }
  if (IS_VISA_BOTH_PATH) {
    int num_pred_desc_operands =
        2; // accounting for exec_size and pred_id in descriptor
    int num_operands = 0;
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[5];
    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    VISA_Modifier mod = MODIFIER_NONE;

    VISA_opnd *dst = tmpDst;
    if (satMode) {
      if (tmpDst == NULL) {
        vISA_ASSERT_INPUT(false, "Destination for arithmetic instruction is null");
        return VISA_FAILURE;
      }
      mod = MODIFIER_SAT;
      dst = (VISA_opnd *)m_mem.alloc(sizeof(VISA_opnd));
      *dst = *tmpDst;
      dst->_opnd.v_opnd.tag += mod << 3;
    }

    ADD_OPND(num_operands, opnd, dst);

    ADD_OPND(num_operands, opnd, src0);

    ADD_OPND(num_operands, opnd, src1);

    ADD_OPND(num_operands, opnd, src2);

    ADD_OPND(num_operands, opnd, src3);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();

    unsigned char size = executionSize;
    size += emask << 4;
    inst->createCisaInstruction(opcode, size, 0, predOpnd, opnd, num_operands,
                                inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAAddrAddInst(VISA_EMask_Ctrl emask,
                                          VISA_Exec_Size executionSize,
                                          VISA_VectorOpnd *dst,
                                          VISA_VectorOpnd *src0,
                                          VISA_VectorOpnd *src1) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  if (IS_GEN_BOTH_PATH) {
    status = m_builder->translateVISAAddrInst(
        ISA_ADDR_ADD, executionSize, emask, dst->g4opnd->asDstRegRegion(),
        src0->g4opnd, src1->g4opnd);
  }
  if (IS_VISA_BOTH_PATH) {
    int num_pred_desc_operands =
        0; // accounting for exec_size and pred_id in descriptor
    int num_operands = 0;
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[4];
    ISA_Opcode opcode = ISA_ADDR_ADD;
    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);

    ADD_OPND(num_operands, opnd, dst);

    ADD_OPND(num_operands, opnd, src0);

    ADD_OPND(num_operands, opnd, src1);

#if START_ASSERT_CHECK
    if ((inst_desc->opnd_num - num_pred_desc_operands) != num_operands) {
      vISA_ASSERT_INPUT(false, "Number of parameters does not match");
      return VISA_FAILURE;
    }
#endif

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += emask << 4;
    status = inst->createCisaInstruction(opcode, size, 0,
                                         PredicateOpnd::getNullPred(), opnd,
                                         num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAMinMaxInst(CISA_MIN_MAX_SUB_OPCODE subOpcode,
                                         bool satMode, VISA_EMask_Ctrl emask,
                                         VISA_Exec_Size executionSize,
                                         VISA_VectorOpnd *tmpDst,
                                         VISA_VectorOpnd *src0,
                                         VISA_VectorOpnd *src1) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    status = m_builder->translateVISADataMovementInst(
        ISA_FMINMAX, subOpcode, NULL, executionSize, emask,
        satMode ? g4::SAT : g4::NOSAT, tmpDst->g4opnd->asDstRegRegion(),
        src0->g4opnd, src1->g4opnd);
  }
  if (IS_VISA_BOTH_PATH) {
    int num_operands = 0;
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[4];
    ISA_Opcode opcode = ISA_FMINMAX;
    inst_desc = &CISA_INST_table[opcode];
    int num_pred_desc_operands = 0;
    GET_NUM_PRED_DESC_OPNDS(
        num_pred_desc_operands,
        inst_desc); // accounting for exec_size and pred_id in descriptor
    VISA_Modifier mod = MODIFIER_NONE;

    VISA_opnd *dst = tmpDst;
    if (satMode) {
      if (tmpDst == NULL) {
        vISA_ASSERT_INPUT(false, "Destination for arithmetic instruction is NULL");
        return VISA_FAILURE;
      }
      mod = MODIFIER_SAT;
      dst = (VISA_opnd *)m_mem.alloc(sizeof(VISA_opnd));
      *dst = *tmpDst;
      dst->_opnd.v_opnd.tag += mod << 3;
    }

    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, subOpcode));

    ADD_OPND(num_operands, opnd, dst);

    ADD_OPND(num_operands, opnd, src0);

    ADD_OPND(num_operands, opnd, src1);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    unsigned char size = executionSize;
    size += emask << 4;
    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);
    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAPredicateMove(VISA_VectorOpnd *dst,
                                            VISA_PredVar *src0) {
  VISA_VectorOpnd *src0Opnd = NULL;
  CreateVISAPredicateSrcOperand(src0Opnd, src0, 1);
  return AppendVISADataMovementInst(ISA_MOV, NULL, false, vISA_EMASK_M1_NM,
                                    EXEC_SIZE_1, dst, src0Opnd, NULL);
}

int VISAKernelImpl::AppendVISASetP(VISA_EMask_Ctrl emask,
                                   VISA_Exec_Size executionSize,
                                   VISA_PredVar *dst, VISA_VectorOpnd *src0) {
  uint32_t exSize = Get_VISA_Exec_Size(executionSize);
  VISA_VectorOpnd *dstOpnd = NULL;
  CreateVISAPredicateDstOperand(dstOpnd, dst, exSize);
  return AppendVISADataMovementInst(ISA_SETP, NULL, false, emask, executionSize,
                                    dstOpnd, src0);
}

int VISAKernelImpl::AppendVISABreakpointInst() {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);
  int status = VISA_SUCCESS;
  if (IS_GEN_BOTH_PATH) {
    status = m_builder->translateBreakpointInstruction();
  }

  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = &CISA_INST_table[ISA_BREAKPOINT];
    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);
    inst->createCisaInstruction(ISA_BREAKPOINT, 1, 0, PredicateOpnd(), nullptr, 0, inst_desc);
    addInstructionToEnd(inst);
  }
  return status;
}

int VISAKernelImpl::AppendVISADataMovementInst(
    ISA_Opcode opcode, VISA_PredOpnd *pred, bool satMode, VISA_EMask_Ctrl emask,
    VISA_Exec_Size executionSize, VISA_VectorOpnd *tmpDst,
    VISA_VectorOpnd *src0) {
  return AppendVISADataMovementInst(opcode, pred, satMode, emask, executionSize,
                                    tmpDst, src0, NULL);
}
int VISAKernelImpl::AppendVISADataMovementInst(
    ISA_Opcode opcode, VISA_PredOpnd *pred, bool satMode, VISA_EMask_Ctrl emask,
    VISA_Exec_Size executionSize, VISA_VectorOpnd *tmpDst,
    VISA_VectorOpnd *src0, VISA_VectorOpnd *src1) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();
  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    G4_Predicate *g4Pred = (pred != NULL) ? pred->g4opnd->asPredicate() : NULL;
    G4_DstRegRegion *g4Dst = tmpDst->g4opnd->asDstRegRegion();
    status = m_builder->translateVISADataMovementInst(
        opcode, CISA_DM_FMIN /*ignored */, g4Pred, executionSize, emask,
        satMode ? g4::SAT : g4::NOSAT, g4Dst, src0->g4opnd, GET_G4_OPNG(src1));

    if (opcode == ISA_MOV && g4Dst->isTmReg() && g4Dst->getLeftBound() == 16) {
      if (m_builder->getPlatform() == Xe_PVC ||
          m_builder->getPlatform() == Xe_PVCXT) {
        std::cerr << "Pause counter is not supported on PVC and PVCXT";
        return VISA_FAILURE;
      }
    }
  }
  if (IS_VISA_BOTH_PATH) {
    int num_operands = 0;
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[4];
    inst_desc = &CISA_INST_table[opcode];
    int num_pred_desc_operands = 0;
    GET_NUM_PRED_DESC_OPNDS(
        num_pred_desc_operands,
        inst_desc); // accounting for exec_size and pred_id in descriptor
    VISA_Modifier mod = MODIFIER_NONE;

    VISA_opnd *dst = tmpDst;
    if (satMode) {
      if (tmpDst == NULL) {
        vISA_ASSERT_INPUT(false, "Destination for Arithmetic instruction is null");
        return VISA_FAILURE;
      }
      mod = MODIFIER_SAT;
      dst = (VISA_opnd *)m_mem.alloc(sizeof(VISA_opnd));
      *dst = *tmpDst;
      dst->_opnd.v_opnd.tag += mod << 3;
    }

    ADD_OPND(num_operands, opnd, dst);

    ADD_OPND(num_operands, opnd, src0);

    ADD_OPND(num_operands, opnd, src1);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();

    unsigned char size = executionSize;
    size += emask << 4;
    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);
    inst->createCisaInstruction(opcode, size, 0, predOpnd, opnd, num_operands,
                                inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAComparisonInst(
    VISA_Cond_Mod sub_op, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISA_VectorOpnd *dstOpnd, VISA_VectorOpnd *src0, VISA_VectorOpnd *src1) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    status = m_builder->translateVISACompareInst(
        ISA_CMP, executionSize, emask, sub_op,
        dstOpnd->g4opnd->asDstRegRegion(), src0->g4opnd, src1->g4opnd);
  }

  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_CMP;
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[4];
    int num_pred_desc_operands = 1;
    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    int num_operands = 0;

    // rel op
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, sub_op));

    ADD_OPND(num_operands, opnd, dstOpnd);

    ADD_OPND(num_operands, opnd, src0);

    ADD_OPND(num_operands, opnd, src1);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    unsigned char size = executionSize;
    size += emask << 4;
    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);
    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}
int VISAKernelImpl::AppendVISAComparisonInst(
    VISA_Cond_Mod sub_op, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISA_PredVar *dstDcl, VISA_VectorOpnd *src0, VISA_VectorOpnd *src1) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  uint32_t exSize = Get_VISA_Exec_Size(executionSize);
  VISA_VectorOpnd *dst = NULL;
  CreateVISAPredicateDstOperand(dst, dstDcl, exSize);
  if (IS_GEN_BOTH_PATH) {
    status = m_builder->translateVISACompareInst(ISA_CMP, executionSize, emask,
                                                 sub_op, dstDcl->predVar.dcl,
                                                 src0->g4opnd, src1->g4opnd);
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_CMP;
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[4];
    int num_pred_desc_operands = 1;
    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    int num_operands = 0;

    // rel op
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, sub_op));

    ADD_OPND(num_operands, opnd, dst);

    ADD_OPND(num_operands, opnd, src0);

    ADD_OPND(num_operands, opnd, src1);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    unsigned char size = executionSize;
    size += emask << 4;
    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);
    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISACFGotoInst(VISA_PredOpnd *pred,
                                         VISA_EMask_Ctrl emask,
                                         VISA_Exec_Size executionSize,
                                         VISA_LabelOpnd *label) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    G4_Predicate *g4Pred = (pred != NULL) ? pred->g4opnd->asPredicate() : NULL;

    G4_Label *g4Lbl = (G4_Label *)label->g4opnd;
    status =
        m_builder->translateVISAGotoInst(g4Pred, executionSize, emask, g4Lbl);
  }
  if (IS_VISA_BOTH_PATH) {
    int num_operands = 0;
    VISA_INST_Desc *inst_desc = NULL;
    inst_desc = &CISA_INST_table[ISA_GOTO];
    VISA_opnd *opnd[1];

    if (!label) {
      vASSERT(false);
      return VISA_FAILURE;
    }

    ADD_OPND(num_operands, opnd, label);

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();
    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += emask << 4;
    inst->createCisaInstruction(ISA_GOTO, size, 0, predOpnd, opnd, num_operands,
                                inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISACFJmpInst(VISA_PredOpnd *pred,
                                        VISA_LabelOpnd *label) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  if (IS_GEN_BOTH_PATH) {
    G4_Predicate *g4Pred = (pred != NULL) ? pred->g4opnd->asPredicate() : NULL;
    status =
        m_builder->translateVISACFJumpInst(g4Pred, (G4_Label *)label->g4opnd);
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[1];
    inst_desc = &CISA_INST_table[ISA_JMP];
    opnd[0] = label;

    if (label == NULL) {
      vASSERT(false);
      return VISA_FAILURE;
    }

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;
    size += vISA_EMASK_M1 << 4;
    inst->createCisaInstruction(ISA_JMP, size, 0, predOpnd, opnd, 1, inst_desc);

    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISACFCallInst(VISA_PredOpnd *pred,
                                         VISA_EMask_Ctrl emask,
                                         VISA_Exec_Size executionSize,
                                         VISA_LabelOpnd *label) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  if (IS_GEN_BOTH_PATH) {
    if (isFCCallerKernel()) {
      m_builder->getFCPatchInfo()->setHasFCCalls(true);
    }
    G4_Predicate *g4Pred = (pred != NULL) ? pred->g4opnd->asPredicate() : NULL;
    status = m_builder->translateVISACFCallInst(executionSize, emask, g4Pred,
                                                (G4_Label *)label->g4opnd);
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[1];
    inst_desc = &CISA_INST_table[ISA_CALL];
    opnd[0] = label;

    if (label == NULL) {
      vASSERT(false);
      return VISA_FAILURE;
    }

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();
    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += emask << 4;
    inst->createCisaInstruction(ISA_CALL, size, 0, predOpnd, opnd, 1,
                                inst_desc);

    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISACFRetInst(VISA_PredOpnd *pred,
                                        VISA_EMask_Ctrl emask,
                                        VISA_Exec_Size executionSize) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  if (IS_GEN_BOTH_PATH) {
    G4_Predicate *g4Pred = (pred != NULL) ? pred->g4opnd->asPredicate() : NULL;
    if (isFCCallableKernel()) {
      m_builder->getFCPatchInfo()->setIsCallableKernel(true);
    }
    status = m_builder->translateVISACFRetInst(executionSize, emask, g4Pred);
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    ISA_Opcode opcode = ISA_RET;
    inst_desc = &CISA_INST_table[opcode];

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);
    unsigned char size = executionSize;
    size += emask << 4;
    inst->createCisaInstruction(opcode, size, 0, predOpnd, NULL, 0, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISACFLabelInst(VISA_LabelOpnd *label) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  if (IS_GEN_BOTH_PATH) {
    status = m_builder->translateVISACFLabelInst((G4_Label *)label->g4opnd);
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[1] = {label};
    inst_desc = &CISA_INST_table[(ISA_Opcode)label->tag];

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);
    inst->createCisaInstruction((ISA_Opcode)label->tag, 1, 0,
                                PredicateOpnd::getNullPred(), opnd, 1,
                                inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISACFFunctionCallInst(
    VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    const std::string& funcName, unsigned char argSize, unsigned char returnSize) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  if (IS_GEN_BOTH_PATH) {
    G4_Predicate *g4Pred = (pred != NULL) ? pred->g4opnd->asPredicate() : NULL;
    status = m_builder->translateVISACFFCallInst(executionSize, emask, g4Pred,
                                                 funcName, argSize, returnSize);
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[3]; // should be more than enough
    int num_pred_desc_operands = 2;
    ISA_Opcode opcode = ISA_FCALL;
    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);

    // create an entry in string pool with the given function name
    uint32_t funcId = addStringPool(funcName);
    int num_operands = 0;

    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, funcId));

    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, argSize));

    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, returnSize));

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();
    /*
    Making a copy of descriptor adn setting correct number of operands.
    This is used later to calculate total size of the buffer.
    */
    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += emask << 4;
    inst->createCisaInstruction(opcode, size, 0, predOpnd, opnd, num_operands,
                                inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISACFIndirectFuncCallInst(
    VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    bool isUniform, VISA_VectorOpnd *funcAddr, uint8_t argSize,
    uint8_t returnSize) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  if (IS_GEN_BOTH_PATH) {
    G4_Predicate *g4Pred = pred ? pred->g4opnd->asPredicate() : nullptr;
    status = m_builder->translateVISACFIFCallInst(executionSize, emask, g4Pred,
                                                  isUniform, funcAddr->g4opnd,
                                                  argSize, returnSize);
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = &CISA_INST_table[ISA_IFCALL];
    VISA_opnd *opnd[4]; // should be more then enough
    int num_pred_desc_operands = 2;
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    int num_operands = 0;

    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, isUniform));

    opnd[num_operands] = funcAddr;
    ++num_operands;

    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, argSize));

    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, returnSize));

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();

    /*
    Making a copy of descriptor adn setting correct number of operands.
    This is used later to calculate total size of the buffer.
    */
    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += emask << 4;
    inst->createCisaInstruction(ISA_IFCALL, size, 0, predOpnd, opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISACFSymbolInst(const std::string& symbolName,
                                           VISA_VectorOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  if (IS_GEN_BOTH_PATH) {
    status = m_builder->translateVISACFSymbolInst(
        symbolName, (G4_DstRegRegion *)dst->g4opnd);
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = &CISA_INST_table[ISA_FADDR];
    VISA_opnd *opnd[3]; // should be more than enough
    int num_operands = 0;

    // create an entry in string pool with the given symbolName
    uint32_t name_idx = addStringPool(symbolName);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(0, num_operands, inst_desc, name_idx));

    opnd[num_operands] = dst;
    ++num_operands;

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst->createCisaInstruction(ISA_FADDR, EXEC_SIZE_1, 0,
                                PredicateOpnd::getNullPred(), opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISACFFunctionRetInst(VISA_PredOpnd *pred,
                                                VISA_EMask_Ctrl emask,
                                                VISA_Exec_Size executionSize) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  if (IS_GEN_BOTH_PATH) {
    G4_Predicate *g4Pred = (pred != NULL) ? pred->g4opnd->asPredicate() : NULL;
    status = m_builder->translateVISACFFretInst(executionSize, emask, g4Pred);
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    ISA_Opcode opcode = ISA_FRET;
    inst_desc = &CISA_INST_table[opcode];

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);
    unsigned char size = executionSize;
    size += emask << 4;
    inst->createCisaInstruction(opcode, size, 0, predOpnd, NULL, 0, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISACFSwitchJMPInst(VISA_VectorOpnd *index,
                                              unsigned char labelCount,
                                              VISA_LabelOpnd **labels) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  if (IS_GEN_BOTH_PATH) {
    G4_Label *labelsArray[50];
    if (labelCount >= 50) {
      vASSERT(false);
      status = VISA_FAILURE;
    } else {
      for (int i = 0; i < labelCount; i++) {
        labelsArray[i] = (G4_Label *)labels[i]->g4opnd;
      }
      status = m_builder->translateVISACFSwitchInst(index->g4opnd, labelCount,
                                                    labelsArray);
    }
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc_temp = NULL;
    VISA_INST_Desc *inst_desc =
        (VISA_INST_Desc *)m_mem.alloc(sizeof(VISA_INST_Desc));
    int num_pred_desc_operands = 1;
    unsigned int num_operands = 0;
    ISA_Opcode opcode = ISA_SWITCHJMP;
    inst_desc_temp = &CISA_INST_table[opcode];
    *inst_desc = CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc_temp);

    VISA_opnd *opnd[35];

    if (index == NULL || labelCount == 0 || labels == NULL) {
      vASSERT(false);
      return VISA_FAILURE;
    }

    opnd[num_operands] = CreateOtherOpndHelper(
        num_pred_desc_operands, num_operands, inst_desc_temp, labelCount);
    ++num_operands;
    opnd[num_operands] = index;
    ++num_operands;

    std::copy_n(labels, labelCount, opnd + num_operands);
    /*
    Making a copy of descriptor adn setting correct number of operands.
    This is used later to calculate total size of the buffer.
    */
    inst_desc->opnd_num = num_pred_desc_operands + labelCount + num_operands;

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;
    size += vISA_EMASK_M1 << 4;
    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, labelCount + num_operands, inst_desc);

    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISASurfAccessDwordAtomicInst(
    VISA_PredOpnd *pred, VISAAtomicOps subOpc, bool is16Bit,
    VISA_EMask_Ctrl eMask, VISA_Exec_Size execSize,
    VISA_StateOpndHandle *surface, VISA_RawOpnd *offsets, VISA_RawOpnd *src0,
    VISA_RawOpnd *src1, VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawSrcOperand(offsets);
    CreateGenRawSrcOperand(src0);
    CreateGenRawSrcOperand(src1);
    CreateGenRawDstOperand(dst);
    status = m_builder->translateVISADwordAtomicInst(
        subOpc, is16Bit, pred ? pred->g4opnd->asPredicate() : 0, execSize,
        eMask, surface->g4opnd, offsets->g4opnd->asSrcRegRegion(),
        src0->g4opnd->asSrcRegRegion(), src1->g4opnd->asSrcRegRegion(),
        dst->g4opnd->asDstRegRegion());
  }

  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_DWORD_ATOMIC;
    VISA_INST_Desc *instDesc = &CISA_INST_table[opcode];

    VISA_opnd *ops[6];
    int numOps = 0;

    uint8_t OpAnd16BitTag = uint8_t(subOpc) | uint8_t((is16Bit ? 1 : 0) << 5);
    ADD_OPND(numOps, ops, CreateOtherOpnd(OpAnd16BitTag, ISA_TYPE_UB));
    ADD_OPND(numOps, ops, surface);
    ADD_OPND(numOps, ops, offsets);
    ADD_OPND(numOps, ops, src0);
    ADD_OPND(numOps, ops, src1);
    ADD_OPND(numOps, ops, dst);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();

    // Pack execution size & mask
    unsigned size = execSize;
    PACK_EXEC_SIZE(size, eMask);

    inst->createCisaInstruction(opcode, (uint8_t)size, 0, predOpnd, ops, numOps,
                                instDesc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISASurfAccessGatherScatterInst(
    ISA_Opcode opcode, VISA_EMask_Ctrl emask,
    GATHER_SCATTER_ELEMENT_SIZE elementSize, VISA_Exec_Size executionSize,
    VISA_StateOpndHandle *surface, VISA_VectorOpnd *globalOffset,
    VISA_RawOpnd *elementOffset, VISA_RawOpnd *srcDst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawSrcOperand(elementOffset);
    auto offset = elementOffset->g4opnd->asSrcRegRegion();
    if (opcode == ISA_GATHER) {
      CreateGenRawDstOperand(srcDst); // gather: srcDst is dst
      status = m_builder->translateVISAGatherInst(
          emask, false, elementSize, executionSize, surface->g4opnd,
          globalOffset->g4opnd, offset, srcDst->g4opnd->asDstRegRegion());
    } else {
      CreateGenRawSrcOperand(srcDst); // scatter: srcDst is src
      status = m_builder->translateVISAScatterInst(
          emask, elementSize, executionSize, surface->g4opnd,
          globalOffset->g4opnd, offset, srcDst->g4opnd->asSrcRegRegion());
    }
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[8];
    int num_pred_desc_operands = 0;
    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    int num_operands = 0;

    unsigned int numberOfElements = 0;

    // elt_size
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, elementSize));

    if (opcode == ISA_GATHER) {
      // ignored
      ADD_OPND(num_operands, opnd,
               CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                     inst_desc, 0));
    }

    // elt_size
    switch (executionSize) {
    case EXEC_SIZE_8:
      numberOfElements = 0;
      break;
    case EXEC_SIZE_16:
      numberOfElements = 1;
      break;
    case EXEC_SIZE_1:
      numberOfElements = 2;
      break;
    default:
      vISA_ASSERT_UNREACHABLE("Invalid Number of Elements for Gather/Scatter.");
      return false;
    }

    numberOfElements += emask << 4;
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, numberOfElements));

    // surface
    ADD_OPND(num_operands, opnd, surface);

    // global offset
    ADD_OPND(num_operands, opnd, globalOffset);

    // element offset
    ADD_OPND(num_operands, opnd, elementOffset);

    // dst/src
    ADD_OPND(num_operands, opnd, srcDst);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst->createCisaInstruction(opcode, EXEC_SIZE_1, 0,
                                PredicateOpnd::getNullPred(), opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISASurfAccessGather4Scatter4TypedInst(
    ISA_Opcode opcode, VISA_PredOpnd *pred, VISAChannelMask _chMask,
    VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISA_StateOpndHandle *surface, VISA_RawOpnd *uOffset, VISA_RawOpnd *vOffset,
    VISA_RawOpnd *rOffset, VISA_RawOpnd *lod, VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ChannelMask chMask = ChannelMask::createFromAPI(_chMask);

  if (IS_GEN_BOTH_PATH) {
    G4_Predicate *g4Pred = (pred != NULL) ? pred->g4opnd->asPredicate() : NULL;
    CreateGenRawSrcOperand(uOffset);
    CreateGenRawSrcOperand(vOffset);
    CreateGenRawSrcOperand(rOffset);
    CreateGenRawSrcOperand(lod);
    if (opcode == ISA_GATHER4_TYPED) {
      CreateGenRawDstOperand(dst);
      status = m_builder->translateVISAGather4TypedInst(
          g4Pred, emask, chMask, surface->g4opnd, executionSize,
          uOffset->g4opnd->asSrcRegRegion(), vOffset->g4opnd->asSrcRegRegion(),
          rOffset->g4opnd->asSrcRegRegion(), lod->g4opnd->asSrcRegRegion(),
          dst->g4opnd->asDstRegRegion());
    } else {
      vISA_ASSERT_INPUT(opcode == ISA_SCATTER4_TYPED,
                  "Invalid opcode for typed gather4/scatter4!");

      CreateGenRawSrcOperand(dst);
      status = m_builder->translateVISAScatter4TypedInst(
          g4Pred, emask, chMask, surface->g4opnd, executionSize,
          uOffset->g4opnd->asSrcRegRegion(), vOffset->g4opnd->asSrcRegRegion(),
          rOffset->g4opnd->asSrcRegRegion(), lod->g4opnd->asSrcRegRegion(),
          dst->g4opnd->asSrcRegRegion());
    }
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[8];
    int num_operands = 0;
    inst_desc = &CISA_INST_table[opcode];

    ADD_OPND(num_operands, opnd,
             CreateOtherOpnd(chMask.getBinary(opcode), ISA_TYPE_UB));
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    ADD_OPND(num_operands, opnd, rOffset);
    ADD_OPND(num_operands, opnd, lod);
    ADD_OPND(num_operands, opnd, dst);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();
    unsigned size = executionSize;
    PACK_EXEC_SIZE(size, emask);

    int status = inst->createCisaInstruction(opcode, (uint8_t)size, 0, predOpnd,
                                             opnd, num_operands, inst_desc);
    if (status != VISA_SUCCESS) {
      vASSERT(false);
      return status;
    }
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISASurfAccessGather4Scatter4ScaledInst(
    ISA_Opcode opcode, VISA_PredOpnd *pred, VISA_EMask_Ctrl eMask,
    VISA_Exec_Size execSize, VISAChannelMask channelMask,
    VISA_StateOpndHandle *surface, VISA_VectorOpnd *globalOffset,
    VISA_RawOpnd *offsets, VISA_RawOpnd *dstSrc) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ChannelMask chMask = ChannelMask::createFromAPI(channelMask);

  if (IS_GEN_BOTH_PATH) {
    vISA_ASSERT_INPUT(opcode == ISA_GATHER4_SCALED || opcode == ISA_SCATTER4_SCALED,
                "Unknown opcode for scaled message!");

    CreateGenRawSrcOperand(offsets);
    if (opcode == ISA_GATHER4_SCALED) {
      CreateGenRawDstOperand(dstSrc);
    } else {
      CreateGenRawSrcOperand(dstSrc);
    }
    if (opcode == ISA_GATHER4_SCALED) {
      status = m_builder->translateVISAGather4ScaledInst(
          pred ? pred->g4opnd->asPredicate() : 0, execSize, eMask, chMask,
          surface->g4opnd, globalOffset->g4opnd,
          offsets->g4opnd->asSrcRegRegion(), dstSrc->g4opnd->asDstRegRegion());
    } else {
      status = m_builder->translateVISAScatter4ScaledInst(
          pred ? pred->g4opnd->asPredicate() : 0, execSize, eMask, chMask,
          surface->g4opnd, globalOffset->g4opnd,
          offsets->g4opnd->asSrcRegRegion(), dstSrc->g4opnd->asSrcRegRegion());
    }
  }

  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *instDesc = &CISA_INST_table[opcode];
    VISA_opnd *ops[6];
    int numOps = 0;

    ADD_OPND(numOps, ops,
             CreateOtherOpnd(chMask.getBinary(opcode), ISA_TYPE_UB));
    ADD_OPND(numOps, ops, CreateOtherOpnd(0, ISA_TYPE_UW));
    ADD_OPND(numOps, ops, surface);
    ADD_OPND(numOps, ops, globalOffset);
    ADD_OPND(numOps, ops, offsets);
    ADD_OPND(numOps, ops, dstSrc);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();

    // Pack execution size & mask
    unsigned size = execSize;
    PACK_EXEC_SIZE(size, eMask);

    inst->createCisaInstruction(opcode, (uint8_t)size, 0, predOpnd, ops, numOps,
                                instDesc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISASurfAccessScatterScaledInst(
    ISA_Opcode opcode, VISA_PredOpnd *pred, VISA_EMask_Ctrl eMask,
    VISA_Exec_Size execSize, VISA_SVM_Block_Num numBlocks,
    VISA_StateOpndHandle *surface, VISA_VectorOpnd *globalOffset,
    VISA_RawOpnd *offsets, VISA_RawOpnd *dstSrc) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    vISA_ASSERT_INPUT(opcode == ISA_GATHER_SCALED || opcode == ISA_SCATTER_SCALED,
                "Unknown opcode for scaled message!");

    CreateGenRawSrcOperand(offsets);
    if (opcode == ISA_GATHER_SCALED) {
      CreateGenRawDstOperand(dstSrc);
    } else {
      CreateGenRawSrcOperand(dstSrc);
    }
    if (opcode == ISA_GATHER_SCALED) {
      status = m_builder->translateVISAGatherScaledInst(
          pred ? pred->g4opnd->asPredicate() : 0, execSize, eMask, numBlocks,
          surface->g4opnd, globalOffset->g4opnd,
          offsets->g4opnd->asSrcRegRegion(), dstSrc->g4opnd->asDstRegRegion());
    } else {
      status = m_builder->translateVISAScatterScaledInst(
          pred ? pred->g4opnd->asPredicate() : 0, execSize, eMask, numBlocks,
          surface->g4opnd, globalOffset->g4opnd,
          offsets->g4opnd->asSrcRegRegion(), dstSrc->g4opnd->asSrcRegRegion());
    }
  }

  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *instDesc = &CISA_INST_table[opcode];
    VISA_opnd *ops[7];
    int numOps = 0;

    ADD_OPND(numOps, ops, CreateOtherOpnd(0, ISA_TYPE_UB));
    ADD_OPND(numOps, ops, CreateOtherOpnd(numBlocks, ISA_TYPE_UB));
    ADD_OPND(numOps, ops, CreateOtherOpnd(0, ISA_TYPE_UW));
    ADD_OPND(numOps, ops, surface);
    ADD_OPND(numOps, ops, globalOffset);
    ADD_OPND(numOps, ops, offsets);
    ADD_OPND(numOps, ops, dstSrc);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();
    // Pack execution size & mask
    unsigned size = execSize;
    PACK_EXEC_SIZE(size, eMask);

    inst->createCisaInstruction(opcode, (uint8_t)size, 0, predOpnd, ops, numOps,
                                instDesc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISASurfAccessMediaLoadStoreInst(
    ISA_Opcode opcode, MEDIA_LD_mod modifier, VISA_StateOpndHandle *surface,
    unsigned char blockWidth, unsigned char blockHeight,
    VISA_VectorOpnd *xOffset, VISA_VectorOpnd *yOffset, VISA_RawOpnd *srcDst,
    CISA_PLANE_ID plane) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    if (opcode == ISA_MEDIA_LD) {
      CreateGenRawDstOperand(srcDst); // srcDst: dst
      status = m_builder->translateVISAMediaLoadInst(
          modifier, surface->g4opnd, plane, blockWidth, blockHeight,
          xOffset->g4opnd, yOffset->g4opnd, srcDst->g4opnd->asDstRegRegion());
    } else {
      if (opcode != ISA_MEDIA_ST) {
        CreateGenRawDstOperand(srcDst); // srcDst: dst
      } else {
        CreateGenRawSrcOperand(srcDst); // srcDst: src
      }
      status = m_builder->translateVISAMediaStoreInst(
          (MEDIA_ST_mod)modifier, surface->g4opnd, plane, blockWidth,
          blockHeight, xOffset->g4opnd, yOffset->g4opnd,
          srcDst->g4opnd->asSrcRegRegion());
    }
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[8];
    int num_pred_desc_operands = 0;
    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    int num_operands = 0;

    // modifiers
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, modifier));
    // surface
    ADD_OPND(num_operands, opnd, surface);

    // plane
    // right now there is VISA and implementation missmatch
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, plane));

    // block width
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, blockWidth));

    // block height
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, blockHeight));

    // x_offset_opnd
    ADD_OPND(num_operands, opnd, xOffset);

    // y_offset_opnd
    ADD_OPND(num_operands, opnd, yOffset);

    // raw_opnd
    ADD_OPND(num_operands, opnd, srcDst);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);
    inst->createCisaInstruction(opcode, EXEC_SIZE_1, 0,
                                PredicateOpnd::getNullPred(), opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

/// Use the following lengthOf macro ONLY for fixed size arrays (no pointers).
#define lengthOf(a) (sizeof(a) / sizeof(a[0]))

static CisaFramework::CisaInst *
AppendVISASvmGeneralBlockInst(VISA_Oword_Num size, bool unaligned,
                              VISA_VectorOpnd *address, VISA_RawOpnd *srcDst,
                              vISA::Mem_Manager &mem, bool isReadOnly) {
  VISA_opnd *opnd[4] = {NULL, NULL, address, srcDst};
  unsigned char pack[2] = {
      static_cast<unsigned char>(isReadOnly ? SVM_BLOCK_LD : SVM_BLOCK_ST),
      static_cast<unsigned char>(size | (unaligned ? 0x80 : 0))};
  for (unsigned i = 0; i < lengthOf(pack); i++) {
    opnd[i] = (VISA_opnd *)mem.alloc(sizeof(VISA_opnd));
    opnd[i]->_opnd.other_opnd = pack[i];
    opnd[i]->opnd_type = CISA_OPND_OTHER;
    opnd[i]->size = sizeof(pack[i]);
    /// opnd[i]->tag              = TYPE_UB;
  }

  CisaFramework::CisaInst *inst = new (mem) CisaFramework::CisaInst(mem);
  inst->createCisaInstruction(ISA_SVM, EXEC_SIZE_1, 0,
                              PredicateOpnd::getNullPred(), opnd,
                              lengthOf(opnd), &CISA_INST_table[ISA_SVM]);
  return inst;
}

CisaFramework::CisaInst *VISAKernelImpl::AppendVISASvmGeneralScatterInst(
    VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size execSize,
    unsigned char blockSize, unsigned char numBlocks, VISA_RawOpnd *address,
    VISA_RawOpnd *srcDst, bool isRead) {
  // NOT TIMED: shared by scatter/gather

  VISA_INST_Desc *inst_desc = NULL;
  VISA_opnd *opnd[10];
  SVMSubOpcode subOp = isRead ? SVM_GATHER : SVM_SCATTER;
  inst_desc = &CISA_INST_table[ISA_SVM];
  int num_operands = 0;
  unsigned char size = execSize;
  size += emask << 4;

  PredicateOpnd predOpnd =
      pred ? pred->convertToPred() : PredicateOpnd::getNullPred();

  // execSize and pred are not handled uniformly for some reason
  ADD_OPND(num_operands, opnd, CreateOtherOpnd(subOp, ISA_TYPE_UB));
  ADD_OPND(num_operands, opnd, CreateOtherOpnd(blockSize, ISA_TYPE_UB));
  ADD_OPND(num_operands, opnd, CreateOtherOpnd(numBlocks, ISA_TYPE_UB));

  ADD_OPND(num_operands, opnd, address);
  ADD_OPND(num_operands, opnd, srcDst);

  CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

  inst->createCisaInstruction(ISA_SVM, size, 0, predOpnd, opnd, num_operands,
                              inst_desc);
  return inst;
}

int VISAKernelImpl::AppendVISASvmBlockLoadInst(VISA_Oword_Num size,
                                               bool unaligned,
                                               VISA_VectorOpnd *address,
                                               VISA_RawOpnd *srcDst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawDstOperand(srcDst); // dst for load
    status = m_builder->translateVISASVMBlockReadInst(
        size, unaligned, address->g4opnd->asSrcRegRegion(),
        srcDst->g4opnd->asDstRegRegion());
  }
  if (IS_VISA_BOTH_PATH) {
    addInstructionToEnd(AppendVISASvmGeneralBlockInst(
        size, unaligned, address, srcDst, m_mem, true /* read */));
  }

  return status;
}

int VISAKernelImpl::AppendVISASvmBlockStoreInst(VISA_Oword_Num size,
                                                bool unaligned,
                                                VISA_VectorOpnd *address,
                                                VISA_RawOpnd *srcDst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawSrcOperand(srcDst); // src for store
    status = m_builder->translateVISASVMBlockWriteInst(
        size, address->g4opnd->asSrcRegRegion(),
        srcDst->g4opnd->asSrcRegRegion());
  }

  if (IS_VISA_BOTH_PATH) {
    addInstructionToEnd(AppendVISASvmGeneralBlockInst(
        size, unaligned, address, srcDst, m_mem, false /* write */));
  }

  return status;
}

int VISAKernelImpl::AppendVISASvmGatherInst(
    VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISA_SVM_Block_Type blockType, VISA_SVM_Block_Num numBlocks,
    VISA_RawOpnd *address, VISA_RawOpnd *srcDst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  if (IS_GEN_BOTH_PATH) {
    CreateGenRawSrcOperand(address);
    CreateGenRawDstOperand(srcDst); // dst for gather
    G4_Predicate *g4Pred = (pred != NULL) ? pred->g4opnd->asPredicate() : NULL;
    status = m_builder->translateVISASVMScatterReadInst(
        executionSize, emask, g4Pred, blockType, numBlocks,
        address->g4opnd->asSrcRegRegion(), srcDst->g4opnd->asDstRegRegion());
  }

  if (IS_VISA_BOTH_PATH) {
    addInstructionToEnd(AppendVISASvmGeneralScatterInst(
        pred, emask, executionSize, blockType, numBlocks, address, srcDst,
        true /* read */));
  }

  return status;
}

int VISAKernelImpl::AppendVISASvmScatterInst(
    VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISA_SVM_Block_Type blockType, VISA_SVM_Block_Num numBlocks,
    VISA_RawOpnd *address, VISA_RawOpnd *srcDst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  if (IS_GEN_BOTH_PATH) {
    CreateGenRawSrcOperand(address);
    CreateGenRawSrcOperand(srcDst); // src for scatter
    G4_Predicate *g4Pred = (pred != NULL) ? pred->g4opnd->asPredicate() : NULL;
    status = m_builder->translateVISASVMScatterWriteInst(
        executionSize, emask, g4Pred, blockType, numBlocks,
        address->g4opnd->asSrcRegRegion(), srcDst->g4opnd->asSrcRegRegion());
  }

  if (IS_VISA_BOTH_PATH) {
    addInstructionToEnd(AppendVISASvmGeneralScatterInst(
        pred, emask, executionSize, blockType, numBlocks, address, srcDst,
        false /* write */));
  }

  return status;
}

int VISAKernelImpl::AppendVISASvmAtomicInst(
    VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISAAtomicOps op, unsigned short bitwidth, VISA_RawOpnd *address,
    VISA_RawOpnd *src0, VISA_RawOpnd *src1, VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  if (IS_GEN_BOTH_PATH) {
    CreateGenRawSrcOperand(address);
    CreateGenRawSrcOperand(src0);
    CreateGenRawSrcOperand(src1);
    CreateGenRawDstOperand(dst);
    G4_Predicate *g4Pred = (pred != NULL) ? pred->g4opnd->asPredicate() : NULL;
    status = m_builder->translateVISASVMAtomicInst(
        static_cast<VISAAtomicOps>(op), bitwidth, executionSize, emask, g4Pred,
        address->g4opnd->asSrcRegRegion(), src0->g4opnd->asSrcRegRegion(),
        src1->g4opnd->asSrcRegRegion(), dst->g4opnd->asDstRegRegion());
  }

  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[10];
    inst_desc = &CISA_INST_table[ISA_SVM];
    int num_operands = 0;
    ADD_OPND(num_operands, opnd, CreateOtherOpnd(SVM_ATOMIC, ISA_TYPE_UB));
    uint8_t BitwidthTag = uint8_t(op);
    switch (bitwidth) {
    case 16:
      BitwidthTag |= 0x20;
      break;
    case 64:
      BitwidthTag |= 0x40;
      break;
    };
    ADD_OPND(num_operands, opnd, CreateOtherOpnd(BitwidthTag, ISA_TYPE_UB));
    ADD_OPND(num_operands, opnd, address);
    ADD_OPND(num_operands, opnd, src0);
    ADD_OPND(num_operands, opnd, src1);
    ADD_OPND(num_operands, opnd, dst);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += emask << 4;

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();

    inst->createCisaInstruction(ISA_SVM, size, 0, predOpnd, opnd, num_operands,
                                inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

CisaFramework::CisaInst *
VISAKernelImpl::PackCisaInsnForSVMGather4Scatter4Scaled(
    unsigned subOpc, VISA_PredOpnd *pred, VISA_EMask_Ctrl eMask,
    VISA_Exec_Size executionSize, ChannelMask chMask, VISA_VectorOpnd *address,
    VISA_RawOpnd *offsets, VISA_RawOpnd *srcOrDst) {
  // no TIME_SCOPE here; caller times scope

  VISA_INST_Desc *instDesc = &CISA_INST_table[ISA_SVM];
  VISA_opnd *ops[6];
  int numOps = 0;

  ADD_OPND(numOps, ops, CreateOtherOpnd(subOpc, ISA_TYPE_UB));
  ADD_OPND(numOps, ops,
           CreateOtherOpnd(chMask.getBinary(ISA_SVM), ISA_TYPE_UB));
  ADD_OPND(numOps, ops, CreateOtherOpnd(0, ISA_TYPE_UW));

  ADD_OPND(numOps, ops, address);
  ADD_OPND(numOps, ops, offsets);
  ADD_OPND(numOps, ops, srcOrDst);

  CisaFramework::CisaInst *insn = new (m_mem) CisaFramework::CisaInst(m_mem);

  PredicateOpnd predOpnd =
      pred ? pred->convertToPred() : PredicateOpnd::getNullPred();
  // Pack executionSize & eMask
  unsigned size = executionSize;
  PACK_EXEC_SIZE(size, eMask);

  insn->createCisaInstruction(ISA_SVM, (uint8_t)size, 0, predOpnd, ops, numOps,
                              instDesc);

  return insn;
}

int VISAKernelImpl::AppendVISASvmGather4ScaledInst(
    VISA_PredOpnd *pred, VISA_EMask_Ctrl eMask, VISA_Exec_Size executionSize,
    VISAChannelMask channelMask, VISA_VectorOpnd *address,
    VISA_RawOpnd *offsets, VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  ChannelMask chMask = ChannelMask::createFromAPI(channelMask);

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawSrcOperand(offsets);
    CreateGenRawDstOperand(dst);
    status = m_builder->translateVISASVMGather4ScaledInst(
        executionSize, eMask, chMask, pred ? pred->g4opnd->asPredicate() : 0,
        address->g4opnd, offsets->g4opnd->asSrcRegRegion(),
        dst->g4opnd->asDstRegRegion());
  }

  if (IS_VISA_BOTH_PATH) {
    addInstructionToEnd(PackCisaInsnForSVMGather4Scatter4Scaled(
        SVM_GATHER4SCALED, pred, eMask, executionSize, chMask, address, offsets,
        dst));
  }

  return status;
}

int VISAKernelImpl::AppendVISASvmScatter4ScaledInst(
    VISA_PredOpnd *pred, VISA_EMask_Ctrl eMask, VISA_Exec_Size executionSize,
    VISAChannelMask channelMask, VISA_VectorOpnd *address,
    VISA_RawOpnd *offsets, VISA_RawOpnd *src) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  ChannelMask chMask = ChannelMask::createFromAPI(channelMask);

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawSrcOperand(offsets);
    CreateGenRawSrcOperand(src);
    status = m_builder->translateVISASVMScatter4ScaledInst(
        executionSize, eMask, chMask, pred ? pred->g4opnd->asPredicate() : 0,
        address->g4opnd, offsets->g4opnd->asSrcRegRegion(),
        src->g4opnd->asSrcRegRegion());
  }

  if (IS_VISA_BOTH_PATH) {
    addInstructionToEnd(PackCisaInsnForSVMGather4Scatter4Scaled(
        SVM_SCATTER4SCALED, pred, eMask, executionSize, chMask, address,
        offsets, src));
  }

  return status;
}

int VISAKernelImpl::AppendVISASurfAccessOwordLoadStoreInst(
    ISA_Opcode opcode, VISA_EMask_Ctrl emask, VISA_StateOpndHandle *surface,
    VISA_Oword_Num size, VISA_VectorOpnd *offset, VISA_RawOpnd *srcDst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    if (opcode == ISA_OWORD_ST) {
      CreateGenRawSrcOperand(srcDst);
      status = m_builder->translateVISAOwordStoreInst(
          surface->g4opnd, size, offset->g4opnd,
          srcDst->g4opnd->asSrcRegRegion());
    } else {
      CreateGenRawDstOperand(srcDst); // srcDst: dst
      status = m_builder->translateVISAOwordLoadInst(
          opcode, false, surface->g4opnd, size, offset->g4opnd,
          srcDst->g4opnd->asDstRegRegion());
    }
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[5];
    inst_desc = &CISA_INST_table[opcode];
    int num_operands = 0;
    int num_pred_desc_operands = 0;
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);

    // size
    opnd[num_operands] = (VISA_opnd *)m_mem.alloc(sizeof(VISA_opnd));
    opnd[num_operands]->_opnd.other_opnd = size;
    opnd[num_operands]->opnd_type = CISA_OPND_OTHER;
    opnd[num_operands]->size = (uint16_t)Get_VISA_Type_Size(
        (VISA_Type)inst_desc->opnd_desc[num_operands].data_type);
    opnd[num_operands]->tag =
        (uint8_t)inst_desc->opnd_desc[num_operands].opnd_type;
    num_operands++;

    // ignored
    if (opcode == ISA_OWORD_LD || opcode == ISA_OWORD_LD_UNALIGNED) {
      ADD_OPND(num_operands, opnd,
               CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                     inst_desc, 0));
    }

    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, offset);
    ADD_OPND(num_operands, opnd, srcDst);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);
    inst->createCisaInstruction(opcode, EXEC_SIZE_1, 0,
                                PredicateOpnd::getNullPred(), opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISASILoad(VISA_StateOpndHandle *surface,
                                     VISAChannelMask _channel, bool isSIMD16,
                                     VISA_RawOpnd *uOffset,
                                     VISA_RawOpnd *vOffset,
                                     VISA_RawOpnd *rOffset, VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ChannelMask channel = ChannelMask::createFromAPI(_channel);

  if (IS_GEN_BOTH_PATH) {
    uint8_t simdMode = isSIMD16 ? 16 : 8;
    CreateGenRawSrcOperand(uOffset);
    CreateGenRawSrcOperand(vOffset);
    CreateGenRawSrcOperand(rOffset);
    CreateGenRawDstOperand(dst);
    status = m_builder->translateVISASamplerInst(
        simdMode, surface->g4opnd, NULL, channel,
        channel.getNumEnabledChannels(), uOffset->g4opnd, vOffset->g4opnd,
        rOffset->g4opnd, dst->g4opnd->asDstRegRegion());
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[8];
    int num_pred_desc_operands = 0;
    ISA_Opcode opcode = ISA_LOAD;
    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    int num_operands = 0;

    unsigned mode = channel.getBinary(opcode);

    if (isSIMD16)
      mode += 0x1 << 4;

    // mode
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(0, num_operands, inst_desc, mode));

    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    ADD_OPND(num_operands, opnd, rOffset);
    ADD_OPND(num_operands, opnd, dst);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst->createCisaInstruction(opcode, EXEC_SIZE_1, 0,
                                PredicateOpnd::getNullPred(), opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISASISample(
    VISA_EMask_Ctrl emask, VISA_StateOpndHandle *surface,
    VISA_StateOpndHandle *sampler, VISAChannelMask _channel, bool isSIMD16,
    VISA_RawOpnd *uOffset, VISA_RawOpnd *vOffset, VISA_RawOpnd *rOffset,
    VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ChannelMask channel = ChannelMask::createFromAPI(_channel);

  if (IS_GEN_BOTH_PATH) {
    uint8_t simdMode = (isSIMD16) ? 16 : 8;
    CreateGenRawSrcOperand(uOffset);
    CreateGenRawSrcOperand(vOffset);
    CreateGenRawSrcOperand(rOffset);
    CreateGenRawDstOperand(dst); // srcDst: src
    status = m_builder->translateVISASamplerInst(
        simdMode, surface->g4opnd, sampler->g4opnd, channel,
        channel.getNumEnabledChannels(), uOffset->g4opnd, vOffset->g4opnd,
        rOffset->g4opnd, dst->g4opnd->asDstRegRegion());
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[8];
    int num_pred_desc_operands = 0;
    ISA_Opcode opcode = ISA_SAMPLE;
    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    int num_operands = 0;

    unsigned mode = channel.getBinary(opcode);

    if (isSIMD16)
      mode += 0x1 << 4;

    // mode
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(0, num_operands, inst_desc, mode));
    ADD_OPND(num_operands, opnd, sampler);
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    ADD_OPND(num_operands, opnd, rOffset);
    ADD_OPND(num_operands, opnd, dst);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst->createCisaInstruction(opcode, EXEC_SIZE_1, 0,
                                PredicateOpnd::getNullPred(), opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISASISampleUnorm(
    VISA_StateOpndHandle *surface, VISA_StateOpndHandle *sampler,
    VISAChannelMask _channel, VISA_VectorOpnd *uOffset,
    VISA_VectorOpnd *vOffset, VISA_VectorOpnd *deltaU, VISA_VectorOpnd *deltaV,
    VISA_RawOpnd *dst, CHANNEL_OUTPUT_FORMAT _output) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ChannelMask channel = ChannelMask::createFromAPI(_channel);

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawDstOperand(dst); // srcDst: src
    status = m_builder->translateVISASamplerNormInst(
        surface->g4opnd, sampler->g4opnd, channel,
        channel.getNumEnabledChannels(), deltaU->g4opnd, uOffset->g4opnd,
        deltaV->g4opnd, vOffset->g4opnd, dst->g4opnd->asDstRegRegion());
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[8];
    int num_pred_desc_operands = 0;
    ISA_Opcode opcode = ISA_SAMPLE_UNORM;
    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    int num_operands = 0;

    // mode
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(0, num_operands, inst_desc,
                                   channel.getBinary(opcode, _output)));

    ADD_OPND(num_operands, opnd, sampler);
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    ADD_OPND(num_operands, opnd, deltaU);
    ADD_OPND(num_operands, opnd, deltaV);
    ADD_OPND(num_operands, opnd, dst);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst->createCisaInstruction(opcode, EXEC_SIZE_1, 0,
                                PredicateOpnd::getNullPred(), opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAWaitInst(VISA_VectorOpnd *mask) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    status =
        m_builder->translateVISAWaitInst(mask != NULL ? mask->g4opnd : NULL);
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[1];
    int num_pred_desc_operands = 0;
    inst_desc = &CISA_INST_table[ISA_WAIT];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    int num_operands = 0;

    if (mask == nullptr) {
      int val = 0;
      CreateVISAImmediate(mask, &val, ISA_TYPE_UD);
    }

    ADD_OPND(num_operands, opnd, mask);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst->createCisaInstruction(ISA_WAIT, EXEC_SIZE_1, 0,
                                PredicateOpnd::getNullPred(), opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISASyncInst(ISA_Opcode opcode, unsigned char mask) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    status = m_builder->translateVISASyncInst(opcode, mask);
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[1];
    int num_pred_desc_operands = 0;
    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    int num_operands = 0;

    if (opcode == ISA_FENCE) {
      // number of registers to send
      ADD_OPND(num_operands, opnd,
               CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                     inst_desc, mask));
    }

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst->createCisaInstruction(opcode, EXEC_SIZE_1, 0,
                                PredicateOpnd::getNullPred(), opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISASplitBarrierInst(bool isSignal) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    status = m_builder->translateVISASplitBarrierInst(nullptr, isSignal);
  }

  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[1];
    int num_pred_desc_operands = 0;
    inst_desc = &CISA_INST_table[ISA_SBARRIER];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    int num_operands = 0;

    uint8_t mode = isSignal ? 1 : 0;
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, mode));

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst->createCisaInstruction(ISA_SBARRIER, EXEC_SIZE_1, 0,
                                PredicateOpnd::getNullPred(), opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }
  if (VISA_WA_CHECK(m_builder->getPWaTable(), Wa_14017131883)) {
    m_builder->setR0ReadOnly();
  }
  return status;
}

int VISAKernelImpl::AppendVISAMiscFileInst(const char *fileName) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    size_t fileLen = std::string_view(fileName).size() + 1;
    char *newFile = (char *)m_mem.alloc(fileLen);
    m_builder->curFile = newFile;
    strcpy_s(newFile, fileLen, fileName);
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[1];
    ISA_Opcode opcode = ISA_FILE;
    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst_desc = &CISA_INST_table[opcode];

    opnd[0] = (VISA_opnd *)m_mem.alloc(sizeof(VISA_opnd));
    opnd[0]->_opnd.other_opnd = addStringPool(std::string(fileName));
    opnd[0]->opnd_type = CISA_OPND_OTHER;

    opnd[0]->size =
        Get_VISA_Type_Size((VISA_Type)inst_desc->opnd_desc[0].data_type);
    opnd[0]->tag = (uint8_t)inst_desc->opnd_desc[0].opnd_type;
    inst->createCisaInstruction(opcode, 1, 0, PredicateOpnd::getNullPred(),
                                opnd, 1, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISADebugLinePlaceholder() {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  // AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    m_builder->generateDebugInfoPlaceholder();
  }

  return status;
}

int VISAKernelImpl::AppendVISAMiscLOC(unsigned int lineNumber) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    m_builder->curLine = lineNumber;
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[1];
    ISA_Opcode opcode = ISA_LOC;
    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst_desc = &CISA_INST_table[opcode];

    opnd[0] = (VISA_opnd *)m_mem.alloc(sizeof(VISA_opnd));
    opnd[0]->_opnd.other_opnd = lineNumber;
    opnd[0]->opnd_type = CISA_OPND_OTHER;

    opnd[0]->size =
        Get_VISA_Type_Size((VISA_Type)inst_desc->opnd_desc[0].data_type);
    opnd[0]->tag = (uint8_t)inst_desc->opnd_desc[0].opnd_type;
    inst->createCisaInstruction(opcode, 1, 0, PredicateOpnd::getNullPred(),
                                opnd, 1, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAMiscRawSend(
    VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    unsigned char modifiers, unsigned int exMsgDesc, unsigned char srcSize,
    unsigned char dstSize, VISA_VectorOpnd *desc, VISA_RawOpnd *src,
    VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  vISA_ASSERT_INPUT(
      !m_builder->isEfficient64bEnabled(),
      "vISA option -enableEfficient64b must be absent in order to "
      "use this instruction");

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawSrcOperand(src);
    CreateGenRawDstOperand(dst);
    G4_Predicate *g4Pred = (pred != NULL) ? pred->g4opnd->asPredicate() : NULL;
    dst->g4opnd->asDstRegRegion()->setType(
        *getIRBuilder(), executionSize == EXEC_SIZE_16 ? Type_UW : Type_UD);

    status = m_builder->translateVISARawSendInst(
        g4Pred, executionSize, emask, modifiers, exMsgDesc, srcSize, dstSize,
        desc->g4opnd, src->g4opnd->asSrcRegRegion(),
        dst->g4opnd->asDstRegRegion());
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[7];
    ISA_Opcode opcode = ISA_RAW_SEND;
    int num_pred_desc_operands = 0;
    inst_desc = &CISA_INST_table[opcode];
    int num_operands = 0;

    // modifier
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, modifiers));

    num_pred_desc_operands = 2;
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);

    // exMsgDesc
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, exMsgDesc));
    // number of registers to send
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, srcSize));
    // Number of registers expected returned
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, dstSize));
    // Message Descriptor
    ADD_OPND(num_operands, opnd, desc);
    // Source
    ADD_OPND(num_operands, opnd, src);
    // Destination
    ADD_OPND(num_operands, opnd, dst);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += emask << 4;

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();

    inst->createCisaInstruction(opcode, size, modifiers, predOpnd, opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAMiscRawSends(
  VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
  unsigned char modifiers, unsigned ffid, VISA_VectorOpnd *exMsgDesc,
  unsigned char src0Size, unsigned char src1Size, unsigned char dstSize,
  VISA_VectorOpnd *desc, VISA_RawOpnd *src0, VISA_RawOpnd *src1,
  VISA_RawOpnd *dst, bool hasEOT)
{
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  vISA_ASSERT_INPUT(
      !m_builder->isEfficient64bEnabled(),
      "vISA option -enableEfficient64b must be absent in order to "
      "use this instruction");

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawSrcOperand(src0);
    CreateGenRawSrcOperand(src1);
    CreateGenRawDstOperand(dst);
    G4_Predicate *g4Pred = (pred != NULL) ? pred->g4opnd->asPredicate() : NULL;
    dst->g4opnd->asDstRegRegion()->setType(*getIRBuilder(), Type_UD);

    status = m_builder->translateVISARawSendsInst(
        g4Pred, executionSize, emask, modifiers, exMsgDesc->g4opnd, src0Size,
        src1Size, dstSize, desc->g4opnd, src0->g4opnd, src1->g4opnd,
        dst->g4opnd->asDstRegRegion(), ffid, hasEOT);
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[12]; // TODO: avoid hard-coding
    ISA_Opcode opcode = ISA_RAW_SENDS;
    int num_pred_desc_operands = 0;
    inst_desc = &CISA_INST_table[opcode];
    int num_operands = 0;

    // modifier
    if (hasEOT) {
      // bits[1]: EOT flag
      modifiers |= 0x2;
    }
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, modifiers));

    num_pred_desc_operands = 2;
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);

    // number of source register 0 to send
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, src0Size));
    // number of source register 1 to send
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, src1Size));
    // Number of destination registers expected returned
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, dstSize));
    // SFID
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, ffid));
    // exMsgDesc
    ADD_OPND(num_operands, opnd, exMsgDesc);
    // Message Descriptor
    ADD_OPND(num_operands, opnd, desc);
    // Source 0
    ADD_OPND(num_operands, opnd, src0);
    // Source 1
    ADD_OPND(num_operands, opnd, src1);
    // Destination
    ADD_OPND(num_operands, opnd, dst);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += emask << 4;

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();

    inst->createCisaInstruction(opcode, size, modifiers, predOpnd, opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}
int VISAKernelImpl::AppendVISAMiscRawSendg(
  unsigned sfid,
  VISA_PredOpnd *pred,
  VISA_EMask_Ctrl emask, VISA_Exec_Size esize,
  VISA_RawOpnd *dst, int dstLenBytes,
  VISA_RawOpnd *src0, int src0LenBytes,
  VISA_RawOpnd *src1, int src1LenBytes,
  VISA_VectorOpnd *ind0,
  VISA_VectorOpnd *ind1,
  uint64_t desc,
  bool sendgConditional,
  bool issueEoT)
{
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  int status = VISA_SUCCESS;

  vISA_ASSERT_INPUT(
      m_builder->isEfficient64bEnabled(),
      "vISA option -enableEfficient64b must be enabled in order to use "
      "this instruction");

  // We permit nullptr for any raw or vector operand argument.
  // For dst, src0, and src1 (all raw) we map to %null and ultimately ARF null.
  auto checkNullptrDstSrc = [&](VISA_RawOpnd *op, bool dst) -> VISA_RawOpnd * {
    if (status != VISA_SUCCESS)
      return op;
    if (op == nullptr)
      status = CreateVISANullRawOperand(op, dst);
    return op;
  };
  dst = checkNullptrDstSrc(dst, true);
  src0 = checkNullptrDstSrc(src0, false);
  src1 = checkNullptrDstSrc(src1, false);
  // For ind0 and ind1 (visa vector) we permit %null and set in the final ISA:
  //   Send.Ind#.Exists = False
  auto checkInd = [&](VISA_VectorOpnd * ind) -> VISA_VectorOpnd * {
    if (status != VISA_SUCCESS)
      return ind;
    VISA_GenVar *nullVar = nullptr;
    if (status != VISA_SUCCESS)
      return nullptr;
    if (ind == nullptr) {
      [[maybe_unused]] auto status = GetPredefinedVar(nullVar, PREDEFINED_NULL);
      status = CreateVISASrcOperand(ind, nullVar, MODIFIER_NONE, 0, 1, 0, 0, 0);
    }
    return ind;
  };
  ind0 = checkInd(ind0);
  ind1 = checkInd(ind1);
  if (status != VISA_SUCCESS)
    return status;


  AppendVISAInstCommon();

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawDstOperand(dst);
    CreateGenRawSrcOperand(src0);
    CreateGenRawSrcOperand(src1);
    G4_Predicate *g4Pred = pred ? pred->g4opnd->asPredicate() : nullptr;
    dst->g4opnd->asDstRegRegion()->setType(*getIRBuilder(), Type_UD);

    status = m_builder->translateVISARawSendgInst(
      sendgConditional,
      sfid,
      g4Pred,
      esize, emask,
      dst->g4opnd->asDstRegRegion(), dstLenBytes,
      src0->g4opnd->asSrcRegRegion(), src0LenBytes,
      src1->g4opnd->asSrcRegRegion(), src1LenBytes,
      ind0->g4opnd,
      ind1->g4opnd,
      desc,
      issueEoT);
  }

  if (IS_VISA_BOTH_PATH) {
    VISA_opnd *opnd[14] { };
    ISA_Opcode opcode = ISA_RAW_SENDG;
    int num_pred_desc_operands = 0;
    const VISA_INST_Desc *inst_desc = &CISA_INST_table[opcode];
    int num_operands = 0;

    unsigned char modifiers = 0;
    if (sendgConditional) {
      modifiers |= (0x1 << 0); // modifiers[0]: conditional (use sendg*c*)
    }
    if (issueEoT) {
      modifiers |= (0x1 << 1); // modifiers[1]: EOT flag
    }

    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, modifiers));

    num_pred_desc_operands = 2;
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);

    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, sfid));

    // Destination
    ADD_OPND(num_operands, opnd, dst);
    // Number of destination registers expected returned
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, dstLenBytes));
    // Source 0
    ADD_OPND(num_operands, opnd, src0);
    // number of source register 0 to send
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, src0LenBytes));
    // Source 1
    ADD_OPND(num_operands, opnd, src1);
    // number of source register 1 to send
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, src1LenBytes));
    // Indirect descriptor 0
    ADD_OPND(num_operands, opnd, ind0);
    // Indirect descriptor 1
    ADD_OPND(num_operands, opnd, ind1);

    // Desc[31:0]
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, (uint32_t)desc));
    // Desc[63:32]
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, (uint32_t)(desc >> 32)));

    status = CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = esize;
    size += emask << 4;

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();

    inst->createCisaInstruction(opcode, size, modifiers, predOpnd, opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAMiscVME_FBR(VISA_StateOpndHandle *surface,
                                          VISA_RawOpnd *UNIInput,
                                          VISA_RawOpnd *FBRInput,
                                          VISA_VectorOpnd *FBRMbMode,
                                          VISA_VectorOpnd *FBRSubMbShape,
                                          VISA_VectorOpnd *FBRSubPredMode,
                                          VISA_RawOpnd *output) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawSrcOperand(UNIInput);
    CreateGenRawSrcOperand(FBRInput);
    CreateGenRawDstOperand(output);
    status = m_builder->translateVISAVmeFbrInst(
        surface->g4opnd, UNIInput->g4opnd, FBRInput->g4opnd, FBRMbMode->g4opnd,
        FBRSubMbShape->g4opnd, FBRSubPredMode->g4opnd,
        output->g4opnd->asDstRegRegion());
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[7];
    int num_pred_desc_operands = 0;
    ISA_Opcode opcode = ISA_VME_FBR;
    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    int num_operands = 0;

    // UNIinput
    ADD_OPND(num_operands, opnd, UNIInput);
    // fbr_input
    ADD_OPND(num_operands, opnd, FBRInput);
    // surface
    ADD_OPND(num_operands, opnd, surface);
    // FBRMbMode
    ADD_OPND(num_operands, opnd, FBRMbMode);
    // FBRSubMbShape
    ADD_OPND(num_operands, opnd, FBRSubMbShape);
    // FBRSubPredMode
    ADD_OPND(num_operands, opnd, FBRSubPredMode);
    // output
    ADD_OPND(num_operands, opnd, output);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst->createCisaInstruction(opcode, EXEC_SIZE_1, 0,
                                PredicateOpnd::getNullPred(), opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAMiscVME_IME(
    VISA_StateOpndHandle *surface, unsigned char streamMode,
    unsigned char searchControlMode, VISA_RawOpnd *UNIInput,
    VISA_RawOpnd *IMEInput, VISA_RawOpnd *ref0, VISA_RawOpnd *ref1,
    VISA_RawOpnd *costCenter, VISA_RawOpnd *output) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawSrcOperand(UNIInput);
    CreateGenRawSrcOperand(IMEInput);
    CreateGenRawSrcOperand(ref0);
    CreateGenRawSrcOperand(ref1);
    CreateGenRawSrcOperand(costCenter);
    CreateGenRawDstOperand(output);
    status = m_builder->translateVISAVmeImeInst(
        streamMode, searchControlMode, surface->g4opnd, UNIInput->g4opnd,
        IMEInput->g4opnd, ref0->g4opnd, ref1->g4opnd, costCenter->g4opnd,
        output->g4opnd->asDstRegRegion());
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[9];
    int num_pred_desc_operands = 0;
    ISA_Opcode opcode = ISA_VME_IME;
    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    int num_operands = 0;

    // streamMode
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, streamMode));
    // searchCtrl
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, searchControlMode));
    // input
    ADD_OPND(num_operands, opnd, UNIInput);
    // ime_input
    ADD_OPND(num_operands, opnd, IMEInput);
    // surface
    ADD_OPND(num_operands, opnd, surface);
    // ref0
    ADD_OPND(num_operands, opnd, ref0);
    // ref1
    ADD_OPND(num_operands, opnd, ref1);
    // CostCenter
    ADD_OPND(num_operands, opnd, costCenter);
    // output
    ADD_OPND(num_operands, opnd, output);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst->createCisaInstruction(opcode, EXEC_SIZE_1, 0,
                                PredicateOpnd::getNullPred(), opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAMiscVME_SIC(VISA_StateOpndHandle *surface,
                                          VISA_RawOpnd *UNIInput,
                                          VISA_RawOpnd *SICInput,
                                          VISA_RawOpnd *output) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawSrcOperand(UNIInput);
    CreateGenRawSrcOperand(SICInput);
    CreateGenRawDstOperand(output);
    status = m_builder->translateVISAVmeSicInst(
        surface->g4opnd, UNIInput->g4opnd, SICInput->g4opnd,
        output->g4opnd->asDstRegRegion());
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[4];
    int num_pred_desc_operands = 0;
    ISA_Opcode opcode = ISA_VME_SIC;
    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    int num_operands = 0;

    ADD_OPND(num_operands, opnd, UNIInput);
    ADD_OPND(num_operands, opnd, SICInput);
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, output);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst->createCisaInstruction(opcode, EXEC_SIZE_1, 0,
                                PredicateOpnd::getNullPred(), opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAMiscVME_IDM(VISA_StateOpndHandle *surface,
                                          VISA_RawOpnd *UNIInput,
                                          VISA_RawOpnd *IDMInput,
                                          VISA_RawOpnd *output) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawSrcOperand(UNIInput);
    CreateGenRawSrcOperand(IDMInput);
    CreateGenRawDstOperand(output);
    status = m_builder->translateVISAVmeIdmInst(
        surface->g4opnd, UNIInput->g4opnd, IDMInput->g4opnd,
        output->g4opnd->asDstRegRegion());
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[4];
    int num_pred_desc_operands = 0;
    ISA_Opcode opcode = ISA_VME_SIC;
    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    int num_operands = 0;

    ADD_OPND(num_operands, opnd, UNIInput);
    ADD_OPND(num_operands, opnd, IDMInput);
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, output);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst->createCisaInstruction(opcode, EXEC_SIZE_1, 0,
                                PredicateOpnd::getNullPred(), opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAMEAVS(
    VISA_StateOpndHandle *surface, VISA_StateOpndHandle *sampler,
    VISAChannelMask _chMask, VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
    VISA_VectorOpnd *deltaU, VISA_VectorOpnd *deltaV, VISA_VectorOpnd *u2d,
    VISA_VectorOpnd *v2d, VISA_VectorOpnd *groupID,
    VISA_VectorOpnd *verticalBlockNumber, OutputFormatControl cntrl,
    AVSExecMode execMode, VISA_VectorOpnd *iefBypass, VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ChannelMask chMask = ChannelMask::createFromAPI(_chMask);
  if (IS_GEN_BOTH_PATH) {
    CreateGenRawDstOperand(dst);
    unsigned numChannelsEnabled = 0;
    numChannelsEnabled = chMask.getNumEnabledChannels();

    status = m_builder->translateVISAAvsInst(
        surface->g4opnd, sampler->g4opnd, chMask, numChannelsEnabled,
        GET_G4_OPNG(deltaU), uOffset->g4opnd, GET_G4_OPNG(deltaV),
        GET_G4_OPNG(vOffset), GET_G4_OPNG(u2d), GET_G4_OPNG(groupID),
        GET_G4_OPNG(verticalBlockNumber), cntrl, GET_G4_OPNG(v2d), execMode,
        GET_G4_OPNG(iefBypass), dst->g4opnd->asDstRegRegion());
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[32];
    int num_pred_desc_operands = 0;
    ISA_Opcode opcode = ISA_AVS;
    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    int num_operands = 0;

    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, chMask.getBinary(ISA_AVS)));
    ADD_OPND(num_operands, opnd, sampler);
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    ADD_OPND(num_operands, opnd, deltaU);
    ADD_OPND(num_operands, opnd, deltaV);
    ADD_OPND(num_operands, opnd, u2d);
    ADD_OPND(num_operands, opnd, groupID);
    ADD_OPND(num_operands, opnd, verticalBlockNumber);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, cntrl));
    ADD_OPND(num_operands, opnd, v2d);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, execMode));
    ADD_OPND(num_operands, opnd, iefBypass);
    ADD_OPND(num_operands, opnd, dst);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst->createCisaInstruction(opcode, EXEC_SIZE_1, 0,
                                PredicateOpnd::getNullPred(), opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}
/************* END APPEND APIS ************************/
int VISAKernelImpl::AppendVISA3dSamplerMsgGeneric(
    ISA_Opcode opcode, VISASampler3DSubOpCode subOpcode, bool pixelNullMask,
    bool cpsEnable, bool uniformSampler,
    VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    ChannelMask srcChannel, VISA_VectorOpnd *aoffimmi,
    VISA_StateOpndHandle *sampler, unsigned int samplerIdx,
    VISA_StateOpndHandle *surface, unsigned int surfaceIdx,
    VISA_RawOpnd *pairedSurface, VISA_RawOpnd *dst,
    unsigned int numMsgSpecificOpnds, VISA_RawOpnd **opndArray) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  if (m_options->getOption(vISA_enableEfficient64b)) {
    if (surface) {
      vISA_ASSERT_INPUT(surface->opnd_type == CISA_OPND_VECTOR,
                        "surface must be vector operand");
    }
    if (sampler) {
      vISA_ASSERT_INPUT(sampler->opnd_type == CISA_OPND_VECTOR,
                        "sampler must be vector operand");
    }
  }
  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  bool isLoad = (subOpcode == VISA_3D_LD_MCS || subOpcode == VISA_3D_LD ||
                 subOpcode == VISA_3D_LD_L || subOpcode == VISA_3D_LD2DMS_W ||
                 subOpcode == VISA_3D_LD_LZ);
  bool isSample4 =
      (subOpcode == VISA_3D_GATHER4 || subOpcode == VISA_3D_GATHER4_C ||
       (m_builder->hasGather4PO() && subOpcode == VISA_3D_GATHER4_PO) ||
       (m_builder->hasGather4PO() && subOpcode == VISA_3D_GATHER4_PO_C) ||
       subOpcode == VISA_3D_GATHER4_PO_PACKED ||
       subOpcode == VISA_3D_GATHER4_PO_PACKED_L ||
       subOpcode == VISA_3D_GATHER4_PO_PACKED_B ||
       subOpcode == VISA_3D_GATHER4_PO_PACKED_I ||
       subOpcode == VISA_3D_GATHER4_PO_PACKED_C ||
       subOpcode == VISA_3D_GATHER4_PO_PACKED_I_C ||
       subOpcode == VISA_3D_GATHER4_PO_PACKED_L_C ||
       subOpcode == VISA_3D_GATHER4_I || subOpcode == VISA_3D_GATHER4_B ||
       subOpcode == VISA_3D_GATHER4_L || subOpcode == VISA_3D_GATHER4_I_C ||
       subOpcode == VISA_3D_GATHER4_L_C);

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawSrcOperand(pairedSurface);
    CreateGenRawDstOperand(dst);
    G4_SrcRegRegion *g4params[32];

    for (unsigned int i = 0; i < numMsgSpecificOpnds; ++i) {
#if START_ASSERT_CHECK
      if (opndArray[i] == NULL) {
        vASSERT(false);
        return VISA_FAILURE;
      }
#endif
      CreateGenRawSrcOperand(opndArray[i]);
      g4params[i] = opndArray[i]->g4opnd->asSrcRegRegion();
    }
    G4_Predicate *g4Pred = (pred != NULL) ? pred->g4opnd->asPredicate() : NULL;

    if (isLoad) {
      if (m_options->getOption(vISA_enableEfficient64b)) {
        // check if sampler and surface are VISA_VectorOpnd
        status = m_builder->translateVISALoad3DInstUnified(
            subOpcode, pixelNullMask, g4Pred, executionSize, emask, srcChannel,
            aoffimmi->g4opnd, nullptr, 0, surface->g4opnd, surfaceIdx,
            pairedSurface->g4opnd, dst->g4opnd->asDstRegRegion(),
            (uint8_t)numMsgSpecificOpnds, g4params);
      } else {
        status = m_builder->translateVISALoad3DInst(
            subOpcode, pixelNullMask, g4Pred, executionSize, emask, srcChannel,
            aoffimmi->g4opnd, surface->g4opnd, pairedSurface->g4opnd,
            dst->g4opnd->asDstRegRegion(), (uint8_t)numMsgSpecificOpnds,
            g4params);
      }
    } else if (isSample4) {
      if (m_options->getOption(vISA_enableEfficient64b)) {
        status = m_builder->translateVISAGather3DInstUnified(
            subOpcode, pixelNullMask, uniformSampler,
            g4Pred, executionSize, emask, srcChannel, aoffimmi->g4opnd,
            sampler->g4opnd, samplerIdx, surface->g4opnd, surfaceIdx,
            pairedSurface->g4opnd, dst->g4opnd->asDstRegRegion(),
            numMsgSpecificOpnds, g4params);
      } else {
        status = m_builder->translateVISAGather3dInst(
            subOpcode, pixelNullMask, g4Pred, executionSize, emask, srcChannel,
            aoffimmi->g4opnd, sampler->g4opnd, surface->g4opnd,
            pairedSurface->g4opnd, dst->g4opnd->asDstRegRegion(),
            numMsgSpecificOpnds, g4params);
      }
    } else {
      if (m_options->getOption(vISA_enableEfficient64b)) {
        vISA_ASSERT_INPUT(surface->opnd_type == CISA_OPND_VECTOR,
                          "surface must be vector operand");
        vISA_ASSERT_INPUT(sampler->opnd_type == CISA_OPND_VECTOR,
                          "sampler must be vector operand");
        status = m_builder->translateVISASampler3DInstUnified(
            subOpcode, pixelNullMask, uniformSampler,
            g4Pred, executionSize, emask, srcChannel, aoffimmi->g4opnd,
            sampler->g4opnd, samplerIdx, surface->g4opnd, surfaceIdx,
            pairedSurface->g4opnd, dst->g4opnd->asDstRegRegion(),
            numMsgSpecificOpnds, g4params);
      } else {
        status = m_builder->translateVISASampler3DInst(
            subOpcode, pixelNullMask, cpsEnable, uniformSampler, g4Pred,
            executionSize, emask, srcChannel, aoffimmi->g4opnd, sampler->g4opnd,
            surface->g4opnd, pairedSurface->g4opnd,
            dst->g4opnd->asDstRegRegion(), numMsgSpecificOpnds, g4params);
      }
    }
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc =
        (VISA_INST_Desc *)m_mem.alloc(sizeof(VISA_INST_Desc));
    *inst_desc = CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;

    int value = 0;
    const int WIDE_OPCODE_ISA_VER = 4;
    if (m_major_version >= WIDE_OPCODE_ISA_VER) {
      // Bit 0-7 : subOp
      // Bit 8   : pixelNullMask
      // Bit 9   : cpsEnable
      // Bit 10  : non-uniform sampler state
      value = subOpcode + (pixelNullMask ? 1 << 8 : 0) +
              (cpsEnable ? 1 << 9 : 0) + (!uniformSampler ? 1 << 10 : 0);

    } else {
      // Bit 0-4: subOpcode
      // Bit 5  : pixelNullMask
      // Bit 6  : cpsEnable
      // Bit 7  : non-uniform sampler
      value = subOpcode + (pixelNullMask ? 1 << 5 : 0) +
              (cpsEnable ? 1 << 6 : 0) + (!uniformSampler ? 1 << 7 : 0);
    }
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, value));

    num_pred_desc_operands = 2;
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);

    uint8_t channelVal = isSample4 ? (uint8_t)srcChannel.convertToSrcChannel()
                                   : (uint8_t)srcChannel.getBinary(opcode);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, channelVal));

    // aoffimmi
    ADD_OPND(num_operands, opnd, aoffimmi);

    // sampler
    if (opcode == ISA_3D_SAMPLE || opcode == ISA_3D_GATHER4) {
      ADD_OPND(num_operands, opnd, sampler);
      if (m_options->getOption(vISA_enableEfficient64b)) {
        ADD_OPND(num_operands, opnd, CreateOtherOpnd(samplerIdx, ISA_TYPE_UD));
      } else {
        // reserved
        ADD_OPND(num_operands, opnd, CreateOtherOpnd(0, ISA_TYPE_UD));
      }
    }

    // surface
    ADD_OPND(num_operands, opnd, surface);
    if (m_options->getOption(vISA_enableEfficient64b)) {
      ADD_OPND(num_operands, opnd, CreateOtherOpnd(surfaceIdx, ISA_TYPE_UD));
    } else {
      // reserved
      ADD_OPND(num_operands, opnd, CreateOtherOpnd(0, ISA_TYPE_UD));
    }
    // dst
    ADD_OPND(num_operands, opnd, dst);
    // pairedSurface
    ADD_OPND(num_operands, opnd, pairedSurface);

    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, numMsgSpecificOpnds));

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    for (unsigned int i = 0; i < numMsgSpecificOpnds; i++) {
      if (opndArray[i] == NULL) {
        vASSERT(false);
        return VISA_FAILURE;
      }
      ADD_OPND(num_operands, opnd, opndArray[i]);
    }

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += emask << 4;

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();

    inst_desc->opnd_num = num_pred_desc_operands + num_operands;

    inst->createCisaInstruction(opcode, size, 0, predOpnd, opnd, num_operands,
                                inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISA3dSampler(
    VISASampler3DSubOpCode subOpcode, bool pixelNullMask, bool cpsEnable,
    bool uniformSampler, VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
    VISA_Exec_Size executionSize, VISAChannelMask srcChannel,
    VISA_VectorOpnd *aoffimmi, VISA_StateOpndHandle *sampler,
    VISA_StateOpndHandle *surface, VISA_RawOpnd *pairedSurface,
    VISA_RawOpnd *dst, int numMsgSpecificOpnds, VISA_RawOpnd **opndArray) {
  return AppendVISA3dSampler(
      subOpcode, pixelNullMask, cpsEnable, uniformSampler,
      pred, emask, executionSize, srcChannel, aoffimmi, sampler, 0, surface, 0,
      pairedSurface, dst, numMsgSpecificOpnds, opndArray);
}

int VISAKernelImpl::AppendVISA3dSampler(
    VISASampler3DSubOpCode subOpcode, bool pixelNullMask, bool cpsEnable,
    bool uniformSampler,
    VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISAChannelMask srcChannel, VISA_VectorOpnd *aoffimmi,
    VISA_StateOpndHandle *sampler, unsigned int samplerIdx,
    VISA_StateOpndHandle *surface, unsigned int surfaceIdx,
    VISA_RawOpnd *pairedSurface, VISA_RawOpnd *dst, int numMsgSpecificOpnds,
    VISA_RawOpnd **opndArray) {
  ISA_Opcode opcode =
      ISA_3D_SAMPLE; // generate Gen IR for opndArray and dst in below func
  return AppendVISA3dSamplerMsgGeneric(
      opcode, subOpcode, pixelNullMask, cpsEnable, uniformSampler,
      pred, emask, executionSize, ChannelMask::createFromAPI(srcChannel),
      aoffimmi, sampler, samplerIdx, surface, surfaceIdx, pairedSurface, dst,
      numMsgSpecificOpnds, opndArray);
}

int VISAKernelImpl::AppendVISA3dLoad(
    VISASampler3DSubOpCode subOpcode, bool pixelNullMask, VISA_PredOpnd *pred,
    VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISAChannelMask srcChannel, VISA_VectorOpnd *aoffimmi,
    VISA_StateOpndHandle *surface, VISA_RawOpnd *pairedSurface,
    VISA_RawOpnd *dst, int numMsgSpecificOpnds, VISA_RawOpnd **opndArray) {
  return AppendVISA3dLoad(subOpcode, pixelNullMask, pred, emask, executionSize,
                          srcChannel, aoffimmi, surface, 0, pairedSurface, dst,
                          numMsgSpecificOpnds, opndArray);
}

int VISAKernelImpl::AppendVISA3dLoad(
    VISASampler3DSubOpCode subOpcode, bool pixelNullMask, VISA_PredOpnd *pred,
    VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISAChannelMask srcChannel, VISA_VectorOpnd *aoffimmi,
    VISA_StateOpndHandle *surface, unsigned int surfaceIndex,
    VISA_RawOpnd *pairedSurface, VISA_RawOpnd *dst, int numMsgSpecificOpnds,
    VISA_RawOpnd **opndArray) {
  ISA_Opcode opcode =
      ISA_3D_LOAD; // generate Gen IR for opndArray and dst in below func
  return AppendVISA3dSamplerMsgGeneric(
      opcode, subOpcode, pixelNullMask,
      /*cpsEnable*/ false,
      /*uniformSampler*/ true,
      pred, emask, executionSize, ChannelMask::createFromAPI(srcChannel),
      aoffimmi, nullptr, 0, surface, surfaceIndex, pairedSurface, dst,
      numMsgSpecificOpnds, opndArray);
}

int VISAKernelImpl::AppendVISA3dGather4(
    VISASampler3DSubOpCode subOpcode, bool pixelNullMask, VISA_PredOpnd *pred,
    VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISASourceSingleChannel srcChannel, VISA_VectorOpnd *aoffimmi,
    VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
    VISA_RawOpnd *pairedSurface, VISA_RawOpnd *dst, int numMsgSpecificOpnds,
    VISA_RawOpnd **opndArray) {
  return AppendVISA3dGather4(subOpcode, pixelNullMask,
                             pred, emask, executionSize, srcChannel, aoffimmi,
                             sampler, 0, surface, 0, pairedSurface, dst,
                             numMsgSpecificOpnds, opndArray);
}

int VISAKernelImpl::AppendVISA3dGather4(
    VISASampler3DSubOpCode subOpcode, bool pixelNullMask,
    VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISASourceSingleChannel srcChannel, VISA_VectorOpnd *aoffimmi,
    VISA_StateOpndHandle *sampler, unsigned int samplerIndex,
    VISA_StateOpndHandle *surface, unsigned int surfaceIndex,
    VISA_RawOpnd *pairedSurface, VISA_RawOpnd *dst, int numMsgSpecificOpnds,
    VISA_RawOpnd **opndArray) {
  ISA_Opcode opcode =
      ISA_3D_GATHER4; // generate Gen IR for opndArray and dst in below func
  return AppendVISA3dSamplerMsgGeneric(
      opcode, subOpcode, pixelNullMask,
      /*cpsEnable*/ false, /*uniformSampler*/ true,
      pred, emask, executionSize,
      ChannelMask::createFromSingleChannel(srcChannel), aoffimmi, sampler,
      samplerIndex, surface, surfaceIndex, pairedSurface, dst,
      numMsgSpecificOpnds, opndArray);
}

int VISAKernelImpl::AppendVISA3dInfo(VISASampler3DSubOpCode subOpcode,
                                     VISA_EMask_Ctrl emask,
                                     VISA_Exec_Size executionSize,
                                     VISAChannelMask srcChannels,
                                     VISA_StateOpndHandle *surface,
                                     unsigned int surfaceIndex,
                                     VISA_RawOpnd *lod, VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ChannelMask channels = ChannelMask::createFromAPI(srcChannels);
  if (IS_GEN_BOTH_PATH) {
    CreateGenRawDstOperand(dst);
    G4_SrcRegRegion *lodg4 = NULL;
    if (subOpcode == VISA_3D_RESINFO) {
      CreateGenRawSrcOperand(lod);
      lodg4 = lod->g4opnd->asSrcRegRegion();
      if (m_options->getOption(vISA_enableEfficient64b)) {
        vISA_ASSERT_INPUT(surface->opnd_type == CISA_OPND_VECTOR,
                          "surface must be vector operand");
        status = m_builder->translateVISAResInfoInstUnified(
            executionSize, emask, channels, surface->g4opnd, surfaceIndex,
            lodg4, dst->g4opnd->asDstRegRegion());
      } else {
        status = m_builder->translateVISAResInfoInst(
            executionSize, emask, channels, surface->g4opnd, lodg4,
            dst->g4opnd->asDstRegRegion());
      }
    } else {
      if (m_options->getOption(vISA_enableEfficient64b)) {
        vISA_ASSERT_INPUT(surface->opnd_type == CISA_OPND_VECTOR,
                          "surface must be vector operand");
        status = m_builder->translateVISASampleInfoUnified(
            executionSize, emask, channels, surface->g4opnd, surfaceIndex,
            dst->g4opnd->asDstRegRegion());
      } else {
        status = m_builder->translateVISASampleInfoInst(
            executionSize, emask, channels, surface->g4opnd,
            dst->g4opnd->asDstRegRegion());
      }
    }
  }
  if (IS_VISA_BOTH_PATH) {
    if (subOpcode != VISA_3D_RESINFO && subOpcode != VISA_3D_SAMPLEINFO) {
      vASSERT(false);
      return VISA_FAILURE;
    }

    ISA_Opcode opcode = ISA_3D_INFO;
    VISA_INST_Desc *inst_desc =
        (VISA_INST_Desc *)m_mem.alloc(sizeof(VISA_INST_Desc));
    *inst_desc = CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    int num_operands = 0;

    // subOP
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, subOpcode));
    // channel
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, channels.getBinary(opcode)));

    ADD_OPND(num_operands, opnd, surface);
    if (m_options->getOption(vISA_enableEfficient64b)) {
      ADD_OPND(num_operands, opnd, CreateOtherOpnd(surfaceIndex, ISA_TYPE_UD));
    } else {
      ADD_OPND(num_operands, opnd, CreateOtherOpnd(0, ISA_TYPE_UD));
    }
    if (subOpcode == VISA_3D_RESINFO) {
      ADD_OPND(num_operands, opnd, lod);
      // opnd_num used in binary emiter, so need to have correct value.
      inst_desc = (VISA_INST_Desc *)m_mem.alloc(sizeof(VISA_INST_Desc));
      *inst_desc = CISA_INST_table[opcode];
      inst_desc->opnd_num++;
    }
    ADD_OPND(num_operands, opnd, dst);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += emask << 4;

    num_pred_desc_operands = 1;
    inst_desc->opnd_num = num_pred_desc_operands + num_operands;

    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISA3dRTWrite(
    VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISA_VectorOpnd *renderTargetIndex, vISA_RT_CONTROLS cntrls,
    VISA_StateOpndHandle *surface, VISA_RawOpnd *r1Header,
    VISA_VectorOpnd *sampleIndex, uint8_t numMsgSpecificOpnds,
    VISA_RawOpnd **opndArray) {
  return AppendVISA3dRTWriteCPS(pred, emask, executionSize, renderTargetIndex,
                                cntrls, surface, r1Header, sampleIndex, NULL,
                                numMsgSpecificOpnds, opndArray);
}
int VISAKernelImpl::AppendVISA3dRTWriteCPS(
    VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISA_VectorOpnd *renderTargetIndex, vISA_RT_CONTROLS cntrls,
    VISA_StateOpndHandle *surface, VISA_RawOpnd *r1Header,
    VISA_VectorOpnd *sampleIndex, VISA_VectorOpnd *cPSCounter,
    uint8_t numMsgSpecificOpnds, VISA_RawOpnd **opndArray, int rtIdentifier) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    G4_SrcRegRegion *g4params[32];

#if START_ASSERT_CHECK
    if (numMsgSpecificOpnds >= 32) {
      vASSERT(false);
      return VISA_FAILURE;
    }
#endif
    for (unsigned int i = 0; i < numMsgSpecificOpnds; ++i) {
#if START_ASSERT_CHECK
      if (opndArray[i] == nullptr) {
        vASSERT(false);
        return VISA_FAILURE;
      }
#endif
      CreateGenRawSrcOperand(opndArray[i]);
      g4params[i] = opndArray[i]->g4opnd->asSrcRegRegion();
    }

#if START_ASSERT_CHECK
    if (cPSCounter && !cPSCounter->g4opnd) {
      vASSERT(false);
      return VISA_FAILURE;
    }
#endif
    G4_SrcRegRegion *cPSCounterOpnd =
        (cPSCounter) ? cPSCounter->g4opnd->asSrcRegRegion() : nullptr;
    G4_SrcRegRegion *sampleIndexOpnd =
        (cntrls.isSampleIndex && sampleIndex) ? sampleIndex->g4opnd->asSrcRegRegion()
                               : nullptr;
    G4_Operand *renderTargetIndexOpnd =
        (cntrls.RTIndexPresent && renderTargetIndex) ? renderTargetIndex->g4opnd : nullptr;
    G4_SrcRegRegion *r1HeaderOpnd = nullptr;

    if (r1Header) {
      CreateGenRawSrcOperand(r1Header);
      r1HeaderOpnd = r1Header->g4opnd->asSrcRegRegion();
    }

    G4_Predicate *g4Pred = pred ? pred->g4opnd->asPredicate() : nullptr;

    {
      status = m_builder->translateVISARTWrite3DInst(
          g4Pred, executionSize, emask, surface->g4opnd, r1HeaderOpnd,
          renderTargetIndexOpnd, cntrls, sampleIndexOpnd, cPSCounterOpnd,
          numMsgSpecificOpnds, g4params);
    }
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_3D_RT_WRITE;
    VISA_INST_Desc *inst_desc =
        (VISA_INST_Desc *)m_mem.alloc(sizeof(VISA_INST_Desc));
    *inst_desc = CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 2;
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    int num_operands = 0;
    bool isCPSCounter = (cPSCounter) ? true : false;

    int mode =
        ((int)cntrls.isNullRT << 12) | ((int)cntrls.isSampleIndex << 11) |
        ((int)cntrls.isCoarseMode << 10) | ((int)cntrls.isPerSample << 9) |
        ((int)(isCPSCounter) << 8) | ((int)cntrls.isLastWrite << 7) |
        ((int)cntrls.isStencil << 6) | ((int)cntrls.zPresent << 5) |
        ((int)cntrls.oMPresent << 4) | ((int)cntrls.s0aPresent << 3) |
        ((int)cntrls.RTIndexPresent << 2);

    // mode
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, mode));
    // surface
    ADD_OPND(num_operands, opnd, surface);

    // r1Header
    ADD_OPND(num_operands, opnd, r1Header);

    if (cntrls.isSampleIndex)
      ADD_OPND(num_operands, opnd, sampleIndex);

    if (cPSCounter)
      ADD_OPND(num_operands, opnd, cPSCounter);

    if (cntrls.RTIndexPresent)
      ADD_OPND(num_operands, opnd, renderTargetIndex);

    for (int i = 0; i < numMsgSpecificOpnds; i++, num_operands++) {
      if (opndArray[i] == nullptr) {
        vASSERT(false);
        return VISA_FAILURE;
      }
      opnd[num_operands] = opndArray[i];
    }

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += emask << 4;

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();

    inst_desc->opnd_num = num_pred_desc_operands + num_operands;

    inst->createCisaInstruction(opcode, size, 0, predOpnd, opnd, num_operands,
                                inst_desc);
    addInstructionToEnd(inst);
  }
  return status;
}

int VISAKernelImpl::AppendVISA3dURBWrite(
    VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    unsigned char numberOutputParams, VISA_RawOpnd *channelMask,
    unsigned short globalOffset, VISA_RawOpnd *URBHandle,
    VISA_RawOpnd *perSlotOffset, VISA_RawOpnd *vertexData) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawSrcOperand(channelMask);
    CreateGenRawSrcOperand(URBHandle);
    CreateGenRawSrcOperand(perSlotOffset);
    CreateGenRawSrcOperand(vertexData);
    G4_Predicate *g4Pred = (pred != NULL) ? pred->g4opnd->asPredicate() : NULL;
    status = m_builder->translateVISAURBWrite3DInst(
        g4Pred, executionSize, emask, numberOutputParams, globalOffset,
        channelMask->g4opnd->asSrcRegRegion(),
        URBHandle->g4opnd->asSrcRegRegion(),
        perSlotOffset->g4opnd->asSrcRegRegion(),
        vertexData->g4opnd->asSrcRegRegion());
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_3D_URB_WRITE;
    VISA_INST_Desc *inst_desc =
        (VISA_INST_Desc *)m_mem.alloc(sizeof(VISA_INST_Desc));
    *inst_desc = CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 2;
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    int num_operands = 0;

    // number output operands
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, numberOutputParams));
    // channel mask
    ADD_OPND(num_operands, opnd, channelMask);
    // globalOffset
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, globalOffset));
    // urb handle
    ADD_OPND(num_operands, opnd, URBHandle);
    // per slot offset
    ADD_OPND(num_operands, opnd, perSlotOffset);
    // vertex data
    ADD_OPND(num_operands, opnd, vertexData);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += emask << 4;

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();

    inst_desc->opnd_num = num_pred_desc_operands + num_operands;

    inst->createCisaInstruction(opcode, size, 0, predOpnd, opnd, num_operands,
                                inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISA3dTypedAtomic(
    VISAAtomicOps subOp, bool is16Bit, VISA_PredOpnd *pred,
    VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISA_StateOpndHandle *surface, VISA_RawOpnd *u, VISA_RawOpnd *v,
    VISA_RawOpnd *r, VISA_RawOpnd *lod, VISA_RawOpnd *src0, VISA_RawOpnd *src1,
    VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawSrcOperand(u);
    CreateGenRawSrcOperand(v);
    CreateGenRawSrcOperand(r);
    CreateGenRawSrcOperand(lod);
    CreateGenRawSrcOperand(src0);
    CreateGenRawSrcOperand(src1);
    CreateGenRawDstOperand(dst);
    G4_Predicate *g4Pred =
        (pred != nullptr) ? pred->g4opnd->asPredicate() : nullptr;
    status = m_builder->translateVISATypedAtomicInst(
        subOp, is16Bit, g4Pred, emask, executionSize, surface->g4opnd,
        u->g4opnd->asSrcRegRegion(), v->g4opnd->asSrcRegRegion(),
        r->g4opnd->asSrcRegRegion(), lod->g4opnd->asSrcRegRegion(),
        src0->g4opnd->asSrcRegRegion(), src1->g4opnd->asSrcRegRegion(),
        dst->g4opnd->asDstRegRegion());
  }

  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_3D_TYPED_ATOMIC;
    VISA_INST_Desc *inst_desc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;

    // number output operands
    uint8_t OpAnd16BitTag = uint8_t(subOp) | uint8_t((is16Bit ? 1 : 0) << 5);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, OpAnd16BitTag));
    num_pred_desc_operands = 2;
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    // surface
    ADD_OPND(num_operands, opnd, surface);
    // u
    ADD_OPND(num_operands, opnd, u);
    // v
    ADD_OPND(num_operands, opnd, v);
    // r
    ADD_OPND(num_operands, opnd, r);
    // lod
    ADD_OPND(num_operands, opnd, lod);
    // src0
    ADD_OPND(num_operands, opnd, src0);
    // src1
    ADD_OPND(num_operands, opnd, src1);
    // dst
    ADD_OPND(num_operands, opnd, dst);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += emask << 4;

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();

    inst_desc->opnd_num = num_pred_desc_operands + num_operands;

    inst->createCisaInstruction(opcode, size, 0, predOpnd, opnd, num_operands,
                                inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAVABooleanCentroid(VISA_StateOpndHandle *surface,
                                                VISA_VectorOpnd *uOffset,
                                                VISA_VectorOpnd *vOffset,
                                                VISA_VectorOpnd *vSize,
                                                VISA_VectorOpnd *hSize,
                                                VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  ISA_VA_Sub_Opcode subOp = BoolCentroid_FOPCODE;
  if (IS_GEN_BOTH_PATH) {
    CreateGenRawDstOperand(dst);
    G4_DstRegRegion *dstOpnd = dst->g4opnd->asDstRegRegion();
    status = m_builder->translateVISASamplerVAGenericInst(
        surface->g4opnd, NULL, uOffset->g4opnd, vOffset->g4opnd, vSize->g4opnd,
        hSize->g4opnd, NULL, 0, 0, subOp, dstOpnd, dstOpnd->getType(),
        16 * dstOpnd->getBase()->asRegVar()->getDeclare()->getElemSize());
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_VA;
    VISA_INST_Desc *inst_desc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;

    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, subOp));
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    ADD_OPND(num_operands, opnd, vSize);
    ADD_OPND(num_operands, opnd, hSize);
    ADD_OPND(num_operands, opnd, dst);

    // CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;

    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAVACentroid(VISA_StateOpndHandle *surface,
                                         VISA_VectorOpnd *uOffset,
                                         VISA_VectorOpnd *vOffset,
                                         VISA_VectorOpnd *vSize,
                                         VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  ISA_VA_Sub_Opcode subOp = Centroid_FOPCODE;
  if (IS_GEN_BOTH_PATH) {
    CreateGenRawDstOperand(dst);
    G4_DstRegRegion *dstOpnd = dst->g4opnd->asDstRegRegion();
    status = m_builder->translateVISASamplerVAGenericInst(
        surface->g4opnd, NULL, uOffset->g4opnd, vOffset->g4opnd, vSize->g4opnd,
        NULL, NULL, 0, 0, subOp, dstOpnd, dstOpnd->getType(),
        32 * dstOpnd->getBase()->asRegVar()->getDeclare()->getElemSize());
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_VA;
    VISA_INST_Desc *inst_desc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;

    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, subOp));
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    ADD_OPND(num_operands, opnd, vSize);
    ADD_OPND(num_operands, opnd, dst);

    // CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;

    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

constexpr unsigned conv_exec_mode_size[4] = {
    16 * 4, /// 16x4
    1,      /// invalid
    16 * 1, /// 16x1
    1       /// 1x1 1pixelconvovle only
};

int VISAKernelImpl::AppendVISAVAConvolve(VISA_StateOpndHandle *sampler,
                                         VISA_StateOpndHandle *surface,
                                         VISA_VectorOpnd *uOffset,
                                         VISA_VectorOpnd *vOffset,
                                         CONVExecMode execMode,
                                         bool isBigKernel, VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  ISA_VA_Sub_Opcode subOp = Convolve_FOPCODE;
  if (IS_GEN_BOTH_PATH) {
    CreateGenRawDstOperand(dst);
    G4_DstRegRegion *dstOpnd = dst->g4opnd->asDstRegRegion();
    unsigned int dstSize =
        conv_exec_mode_size[execMode] *
        dstOpnd->getBase()->asRegVar()->getDeclare()->getElemSize();
    status = m_builder->translateVISASamplerVAGenericInst(
        surface->g4opnd, sampler->g4opnd, uOffset->g4opnd, vOffset->g4opnd,
        NULL, NULL, NULL, 0, execMode, subOp, dstOpnd, dstOpnd->getType(),
        dstSize, isBigKernel);
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_VA;
    VISA_INST_Desc *inst_desc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;
    uint8_t properties = execMode;
    properties = properties | (isBigKernel << 4);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, subOp));
    ADD_OPND(num_operands, opnd, sampler);
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, properties, true, subOp));
    ADD_OPND(num_operands, opnd, dst);

    // CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;

    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAVAErodeDilate(
    EDMode mode, VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
    VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset, EDExecMode execMode,
    VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  ISA_VA_Sub_Opcode subOp = ERODE_FOPCODE;

  if (mode == VA_DILATE) {
    subOp = Dilate_FOPCODE;
  }

  if (IS_GEN_BOTH_PATH) {
   constexpr unsigned ed_exec_mode_byte_size[4] = {
        64 * 4 / 8, /// VA_ED_64x4
        32 * 4 / 8, /// VA_ED_32x4
        64 * 1 / 8, /// VA_ED_64x1
        32 * 1 / 8  /// VA_ED_32x1
    };

    CreateGenRawDstOperand(dst);
    G4_DstRegRegion *dstOpnd = dst->g4opnd->asDstRegRegion();
    unsigned int dstSize = ed_exec_mode_byte_size[execMode];
    status = m_builder->translateVISASamplerVAGenericInst(
        surface->g4opnd, sampler->g4opnd, uOffset->g4opnd, vOffset->g4opnd,
        NULL, NULL, NULL, 0, execMode, subOp, dstOpnd, dstOpnd->getType(),
        dstSize, false);
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_VA;
    VISA_INST_Desc *inst_desc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;

    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, subOp));
    ADD_OPND(num_operands, opnd, sampler);
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, execMode, true, subOp));
    ADD_OPND(num_operands, opnd, dst);

    // CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;

    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAVAMinMax(VISA_StateOpndHandle *surface,
                                       VISA_VectorOpnd *uOffset,
                                       VISA_VectorOpnd *vOffset,
                                       VISA_VectorOpnd *mmMode,
                                       VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  ISA_VA_Sub_Opcode subOp = MINMAX_FOPCODE;

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawDstOperand(dst);
    G4_DstRegRegion *dstOpnd = dst->g4opnd->asDstRegRegion();
    status = m_builder->translateVISASamplerVAGenericInst(
        surface->g4opnd, NULL, uOffset->g4opnd, vOffset->g4opnd, NULL, NULL,
        mmMode->g4opnd, 0, 0, subOp, dstOpnd, dstOpnd->getType(), 32, false);
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_VA;
    VISA_INST_Desc *inst_desc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;

    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, subOp));
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    ADD_OPND(num_operands, opnd, mmMode);
    ADD_OPND(num_operands, opnd, dst);

    // CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;

    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAVAMinMaxFilter(
    VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
    VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
    OutputFormatControl cntrl, MMFExecMode execMode, VISA_VectorOpnd *mmfMode,
    VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ISA_VA_Sub_Opcode subOp = MINMAXFILTER_FOPCODE;

  if (cntrl != AVS_16_FULL && cntrl != AVS_8_FULL) {
    vASSERT(false);
    return VISA_FAILURE;
  }

  if (IS_GEN_BOTH_PATH) {
    constexpr unsigned mmf_exec_mode_size[4] = {
        16 * 4, /// 16x4
        1,      /// invalid
        16 * 1, /// 16x1
        1 * 1   /// 1x1
    };
    constexpr unsigned format_control_byteSize2[4] = {
        4, /// AVS_16_FULL
        2, /// AVS_16_DOWN_SAMPLE NOT VALID
        2, /// AVS_8_FULL
        1  /// AVS_8_DOWN_SAMPLE NOT VALID
    };

    CreateGenRawDstOperand(dst);
    G4_DstRegRegion *dstOpnd = dst->g4opnd->asDstRegRegion();
    unsigned dstSize =
        (VA_MMF_1x1 == execMode)
            ? 4
            : (mmf_exec_mode_size[execMode] * format_control_byteSize2[cntrl]);
    status = m_builder->translateVISASamplerVAGenericInst(
        surface->g4opnd, sampler->g4opnd, uOffset->g4opnd, vOffset->g4opnd,
        NULL, NULL, mmfMode->g4opnd, cntrl, execMode, subOp, dstOpnd,
        dstOpnd->getType(), dstSize);
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_VA;
    VISA_INST_Desc *inst_desc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;

    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, subOp));
    ADD_OPND(num_operands, opnd, sampler);
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, cntrl, true, subOp));
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   inst_desc, execMode, true, subOp));
    ADD_OPND(num_operands, opnd, mmfMode);
    ADD_OPND(num_operands, opnd, dst);

    // CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;

    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAVACorrelationSearch(
    VISA_StateOpndHandle *surface, VISA_VectorOpnd *uOffset,
    VISA_VectorOpnd *vOffset, VISA_VectorOpnd *vOrigin,
    VISA_VectorOpnd *hOrigin, VISA_VectorOpnd *xDirectionSize,
    VISA_VectorOpnd *yDirectionSize, VISA_VectorOpnd *xDirectionSearchSize,
    VISA_VectorOpnd *yDirectionSearchSize, VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ISA_VA_Sub_Opcode subOp = VA_OP_CODE_CORRELATION_SEARCH;

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawDstOperand(dst);
    G4_DstRegRegion *dstOpnd = dst->g4opnd->asDstRegRegion();

    G4_Declare *dstDcl = dstOpnd->getBase()->asRegVar()->getDeclare();
    G4_Type dstType = dstDcl->getElemType();
    unsigned int dstSize =
        dstDcl->getNumElems() * dstDcl->getNumRows() * TypeSize(dstType);

    uint8_t execMode = 0;
    uint8_t functionality = 0x3; /*reserved*/
    status = m_builder->translateVISAVaSklPlusGeneralInst(
        subOp, surface->g4opnd, NULL /*sampler*/, execMode, functionality,
        // rest
        uOffset->g4opnd, vOffset->g4opnd,

        // 1pixel convolve
        NULL /*offsetOpnd*/,

        // FloodFill
        NULL /*loopCountOpnd*/, NULL /*pixelhMasOpnd*/,
        NULL /*pixelVMaskLeftOpnd*/, NULL /*pixelVMaskRightOpnd*/,

        // LBP Correlation
        NULL /*disparityOpnd*/,

        // correlation search
        vOrigin->g4opnd, hOrigin->g4opnd, xDirectionSize->g4opnd,
        yDirectionSize->g4opnd, xDirectionSearchSize->g4opnd,
        yDirectionSearchSize->g4opnd,

        // general
        dstOpnd, dstType, dstSize,

        // hdc
        0 /*pixelSize*/, NULL /*dstSurfaceOpnd*/, NULL /*dstXOffOnd*/,
        NULL /*dstYOffOpnd*/, false);
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_VA_SKL_PLUS;
    VISA_INST_Desc *instDesc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;

    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, instDesc);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, subOp));
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    ADD_OPND(num_operands, opnd, vOrigin);
    ADD_OPND(num_operands, opnd, hOrigin);
    ADD_OPND(num_operands, opnd, xDirectionSize);
    ADD_OPND(num_operands, opnd, yDirectionSize);
    ADD_OPND(num_operands, opnd, xDirectionSearchSize);
    ADD_OPND(num_operands, opnd, yDirectionSearchSize);
    ADD_OPND(num_operands, opnd, dst);

    // CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;

    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, instDesc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAVAFloodFill(
    bool is8Connect, VISA_RawOpnd *pixelMaskHDirection,
    VISA_VectorOpnd *pixelMaskVDirectionLeft,
    VISA_VectorOpnd *pixelMaskVDirectionRight, VISA_VectorOpnd *loopCount,
    VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ISA_VA_Sub_Opcode subOp = VA_OP_CODE_FLOOD_FILL;

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawSrcOperand(pixelMaskHDirection);
    CreateGenRawDstOperand(dst);
    G4_DstRegRegion *dstOpnd = dst->g4opnd->asDstRegRegion();

    G4_Declare *dstDcl = dstOpnd->getBase()->asRegVar()->getDeclare();
    G4_Type dstType = dstDcl->getElemType();
    /*should be UW*/
    unsigned int dstSize = 8 * TypeSize(dstType);

    uint8_t execMode = is8Connect;
    uint8_t functionality = 0x3; /*reserved*/
    status = m_builder->translateVISAVaSklPlusGeneralInst(
        subOp, NULL /*surface*/, NULL /*sampler*/, execMode, functionality,
        // rest
        NULL /*uOffset*/, NULL /*vOffset*/,

        // 1pixel convolve
        NULL /*offsetOpnd*/,

        // FloodFill
        loopCount->g4opnd /*loopCountOpnd*/,
        pixelMaskHDirection->g4opnd /*pixelhMasOpnd*/,
        pixelMaskVDirectionLeft->g4opnd /*pixelVMaskLeftOpnd*/,
        pixelMaskVDirectionRight->g4opnd /*pixelVMaskRightOpnd*/,

        // LBP Correlation
        NULL /*disparityOpnd*/,

        // correlation search
        NULL, NULL, NULL, NULL, NULL, NULL,

        // general
        dstOpnd, dstType, dstSize,

        // hdc
        0 /*pixelSize*/, NULL /*dstSurfaceOpnd*/, NULL /*dstXOffOnd*/,
        NULL /*dstYOffOpnd*/, false);
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_VA_SKL_PLUS;
    VISA_INST_Desc *instDesc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;

    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, instDesc);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, subOp));
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, is8Connect, true, subOp));
    ADD_OPND(num_operands, opnd, pixelMaskHDirection);
    ADD_OPND(num_operands, opnd, pixelMaskVDirectionLeft);
    ADD_OPND(num_operands, opnd, pixelMaskVDirectionRight);
    ADD_OPND(num_operands, opnd, loopCount);
    ADD_OPND(num_operands, opnd, dst);

    // CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;

    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, instDesc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAVALBPCorrelation(VISA_StateOpndHandle *surface,
                                               VISA_VectorOpnd *uOffset,
                                               VISA_VectorOpnd *vOffset,
                                               VISA_VectorOpnd *disparity,
                                               VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ISA_VA_Sub_Opcode subOp = VA_OP_CODE_LBP_CORRELATION;

  if (IS_GEN_BOTH_PATH) {
    constexpr unsigned lbp_correlation_mode_size[3] = {
        16 * 4, /// 16x4
        1,      /// invalid
        16 * 1  /// 16x1
    };
    uint8_t execMode = 0;
    uint8_t functionality = 0x3; /*reserved*/

    CreateGenRawDstOperand(dst);
    G4_DstRegRegion *dstOpnd = dst->g4opnd->asDstRegRegion();

    G4_Declare *dstDcl = dstOpnd->getBase()->asRegVar()->getDeclare();
    G4_Type dstType = dstDcl->getElemType();
    /*should be UB*/
    unsigned int dstSize =
        lbp_correlation_mode_size[execMode] * TypeSize(dstType);

    status = m_builder->translateVISAVaSklPlusGeneralInst(
        subOp, surface->g4opnd /*surface*/, NULL /*sampler*/, execMode,
        functionality,
        // rest
        uOffset->g4opnd /*uOffset*/, vOffset->g4opnd /*vOffset*/,

        // 1pixel convolve
        NULL /*offsetOpnd*/,

        // FloodFill
        NULL /*loopCountOpnd*/, NULL /*pixelhMasOpnd*/,
        NULL /*pixelVMaskLeftOpnd*/, NULL /*pixelVMaskRightOpnd*/,

        // LBP Correlation
        disparity->g4opnd /*disparityOpnd*/,

        // correlation search
        NULL, NULL, NULL, NULL, NULL, NULL,

        // general
        dstOpnd, dstType, dstSize,

        // hdc
        0 /*pixelSize*/, NULL /*dstSurfaceOpnd*/, NULL /*dstXOffOnd*/,
        NULL /*dstYOffOpnd*/, false);
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_VA_SKL_PLUS;
    VISA_INST_Desc *instDesc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;

    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, instDesc);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, subOp));
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    ADD_OPND(num_operands, opnd, disparity);
    ADD_OPND(num_operands, opnd, dst);

    // CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;

    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, instDesc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAVALBPCreation(VISA_StateOpndHandle *surface,
                                            VISA_VectorOpnd *uOffset,
                                            VISA_VectorOpnd *vOffset,
                                            LBPCreationMode mode,
                                            VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ISA_VA_Sub_Opcode subOp = VA_OP_CODE_LBP_CREATION;

  if (IS_GEN_BOTH_PATH) {
    constexpr unsigned lbp_creation_exec_mode_size[3] = {
        16 * 8, /// BOTH
        16 * 4, /// 3x3
        16 * 4  /// 5x5
    };

    uint8_t execMode = 0;
    uint8_t functionality = mode; /*reserved*/

    CreateGenRawDstOperand(dst);
    G4_DstRegRegion *dstOpnd = dst->g4opnd->asDstRegRegion();

    G4_Declare *dstDcl = dstOpnd->getBase()->asRegVar()->getDeclare();
    G4_Type dstType = dstDcl->getElemType();
    /*should be UB*/
    unsigned int dstSize =
        lbp_creation_exec_mode_size[functionality] * TypeSize(dstType);

    status = m_builder->translateVISAVaSklPlusGeneralInst(
        subOp, surface->g4opnd /*surface*/, NULL /*sampler*/, execMode,
        functionality,
        // rest
        uOffset->g4opnd /*uOffset*/, vOffset->g4opnd /*vOffset*/,

        // 1pixel convolve
        NULL /*offsetOpnd*/,

        // FloodFill
        NULL /*loopCountOpnd*/, NULL /*pixelhMasOpnd*/,
        NULL /*pixelVMaskLeftOpnd*/, NULL /*pixelVMaskRightOpnd*/,

        // LBP Correlation
        NULL /*disparityOpnd*/,

        // correlation search
        NULL, NULL, NULL, NULL, NULL, NULL,

        // general
        dstOpnd, dstType, dstSize,

        // hdc
        0 /*pixelSize*/, NULL /*dstSurfaceOpnd*/, NULL /*dstXOffOnd*/,
        NULL /*dstYOffOpnd*/, false);
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_VA_SKL_PLUS;
    VISA_INST_Desc *instDesc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;

    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, instDesc);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, subOp));
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, mode, true, subOp));
    ADD_OPND(num_operands, opnd, dst);

    // CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;

    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, instDesc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAVAConvolve1D(
    VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
    VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset, CONVExecMode mode,
    Convolve1DDirection direction, VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ISA_VA_Sub_Opcode subOp = VA_OP_CODE_1D_CONVOLVE_VERTICAL;

  if (direction == VA_H_DIRECTION)
    subOp = VA_OP_CODE_1D_CONVOLVE_HORIZONTAL;

  if (IS_GEN_BOTH_PATH) {
    uint8_t execMode = mode;
    uint8_t functionality = 0x3; /*reserved*/

    CreateGenRawDstOperand(dst);
    G4_DstRegRegion *dstOpnd = dst->g4opnd->asDstRegRegion();

    G4_Declare *dstDcl = dstOpnd->getBase()->asRegVar()->getDeclare();
    G4_Type dstType = dstDcl->getElemType();
    /*should be W*/
    unsigned int dstSize = conv_exec_mode_size[execMode] * TypeSize(dstType);

    status = m_builder->translateVISAVaSklPlusGeneralInst(
        subOp, surface->g4opnd /*surface*/, sampler->g4opnd /*sampler*/,
        execMode, functionality,
        // rest
        uOffset->g4opnd /*uOffset*/, vOffset->g4opnd /*vOffset*/,

        // 1pixel convolve
        NULL /*offsetOpnd*/,

        // FloodFill
        NULL /*loopCountOpnd*/, NULL /*pixelhMasOpnd*/,
        NULL /*pixelVMaskLeftOpnd*/, NULL /*pixelVMaskRightOpnd*/,

        // LBP Correlation
        NULL /*disparityOpnd*/,

        // correlation search
        NULL, NULL, NULL, NULL, NULL, NULL,

        // general
        dstOpnd, dstType, dstSize,

        // hdc
        0 /*pixelSize*/, NULL /*dstSurfaceOpnd*/, NULL /*dstXOffOnd*/,
        NULL /*dstYOffOpnd*/, false);
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_VA_SKL_PLUS;
    VISA_INST_Desc *instDesc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;

    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, instDesc);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, subOp));
    ADD_OPND(num_operands, opnd, sampler);
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, mode, true, subOp));
    ADD_OPND(num_operands, opnd, dst);

    // CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;

    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, instDesc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAVAConvolve1Pixel(
    VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
    VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset, CONV1PixelExecMode mode,
    VISA_RawOpnd *offsets, VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ISA_VA_Sub_Opcode subOp = VA_OP_CODE_1PIXEL_CONVOLVE;

  if (IS_GEN_BOTH_PATH) {
    uint8_t execMode = mode;
    uint8_t functionality = 0x3; /*reserved*/

    CreateGenRawSrcOperand(offsets);
    CreateGenRawDstOperand(dst);
    G4_DstRegRegion *dstOpnd = dst->g4opnd->asDstRegRegion();

    G4_Declare *dstDcl = dstOpnd->getBase()->asRegVar()->getDeclare();
    G4_Type dstType = dstDcl->getElemType();
    /*should be W*/
    unsigned int dstSize = conv_exec_mode_size[execMode] * TypeSize(dstType);

    status = m_builder->translateVISAVaSklPlusGeneralInst(
        subOp, surface->g4opnd /*surface*/, sampler->g4opnd /*sampler*/,
        execMode, functionality,
        // rest
        uOffset->g4opnd /*uOffset*/, vOffset->g4opnd /*vOffset*/,

        // 1pixel convolve
        offsets->g4opnd /*offsetOpnd*/,

        // FloodFill
        NULL /*loopCountOpnd*/, NULL /*pixelhMasOpnd*/,
        NULL /*pixelVMaskLeftOpnd*/, NULL /*pixelVMaskRightOpnd*/,

        // LBP Correlation
        NULL /*disparityOpnd*/,

        // correlation search
        NULL, NULL, NULL, NULL, NULL, NULL,

        // general
        dstOpnd, dstType, dstSize,

        // hdc
        0 /*pixelSize*/, NULL /*dstSurfaceOpnd*/, NULL /*dstXOffOnd*/,
        NULL /*dstYOffOpnd*/, false);
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_VA_SKL_PLUS;
    VISA_INST_Desc *instDesc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;

    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, instDesc);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, subOp));
    ADD_OPND(num_operands, opnd, sampler);
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, mode, true, subOp));
    ADD_OPND(num_operands, opnd, offsets);
    ADD_OPND(num_operands, opnd, dst);

    // CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;

    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, instDesc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAVAHDCConvolve(
    VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
    VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
    HDCReturnFormat returnFormat, CONVHDCRegionSize regionSize,
    VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset,
    VISA_VectorOpnd *yOffset) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ISA_VA_Sub_Opcode subOp = ISA_HDC_CONV;

  if (IS_GEN_BOTH_PATH) {
    uint8_t execMode = VA_CONV_16x4;
    uint8_t functionality = regionSize; /*reserved*/
    uint8_t pixelSize = returnFormat;

    status = m_builder->translateVISAVaSklPlusGeneralInst(
        subOp, surface->g4opnd /*surface*/, sampler->g4opnd /*sampler*/,
        execMode, functionality,
        // rest
        uOffset->g4opnd /*uOffset*/, vOffset->g4opnd /*vOffset*/,

        // 1pixel convolve
        NULL /*offsetOpnd*/,

        // FloodFill
        NULL /*loopCountOpnd*/, NULL /*pixelhMasOpnd*/,
        NULL /*pixelVMaskLeftOpnd*/, NULL /*pixelVMaskRightOpnd*/,

        // LBP Correlation
        NULL /*disparityOpnd*/,

        // correlation search
        NULL, NULL, NULL, NULL, NULL, NULL,

        // general
        NULL, Type_UNDEF, 0,

        // hdc
        pixelSize /*pixelSize*/, dstSurface->g4opnd /*dstSurfaceOpnd*/,
        xOffset->g4opnd /*dstXOffOnd*/, yOffset->g4opnd /*dstYOffOpnd*/, true);
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_VA_SKL_PLUS;
    VISA_INST_Desc *instDesc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;
    uint8_t properties = returnFormat;
    properties = properties | (regionSize << 4);
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, instDesc);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, subOp));
    ADD_OPND(num_operands, opnd, sampler);
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, properties, true, subOp));
    ADD_OPND(num_operands, opnd, dstSurface);
    ADD_OPND(num_operands, opnd, xOffset);
    ADD_OPND(num_operands, opnd, yOffset);

    // CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;

    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, instDesc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAVAHDCErodeDilate(
    EDMode subOpED, VISA_StateOpndHandle *sampler,
    VISA_StateOpndHandle *surface, VISA_VectorOpnd *uOffset,
    VISA_VectorOpnd *vOffset, VISA_StateOpndHandle *dstSurface,
    VISA_VectorOpnd *xOffset, VISA_VectorOpnd *yOffset) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ISA_VA_Sub_Opcode subOp = ISA_HDC_ERODE;

  if (subOpED == VA_DILATE)
    subOp = ISA_HDC_DILATE;

  if (IS_GEN_BOTH_PATH) {
    uint8_t execMode = VA_ED_64x4;
    uint8_t functionality = 0x3; /*reserved*/
    uint8_t pixelSize = 1;

    status = m_builder->translateVISAVaSklPlusGeneralInst(
        subOp, surface->g4opnd /*surface*/, sampler->g4opnd /*sampler*/,
        execMode, functionality,
        // rest
        uOffset->g4opnd /*uOffset*/, vOffset->g4opnd /*vOffset*/,

        // 1pixel convolve
        NULL /*offsetOpnd*/,

        // FloodFill
        NULL /*loopCountOpnd*/, NULL /*pixelhMasOpnd*/,
        NULL /*pixelVMaskLeftOpnd*/, NULL /*pixelVMaskRightOpnd*/,

        // LBP Correlation
        NULL /*disparityOpnd*/,

        // correlation search
        NULL, NULL, NULL, NULL, NULL, NULL,

        // general
        NULL, Type_UNDEF, 0,

        // hdc
        pixelSize /*pixelSize*/, dstSurface->g4opnd /*dstSurfaceOpnd*/,
        xOffset->g4opnd /*dstXOffOnd*/, yOffset->g4opnd /*dstYOffOpnd*/, true);
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_VA_SKL_PLUS;
    VISA_INST_Desc *instDesc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;

    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, instDesc);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, subOp));
    ADD_OPND(num_operands, opnd, sampler);
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    ADD_OPND(num_operands, opnd, dstSurface);
    ADD_OPND(num_operands, opnd, xOffset);
    ADD_OPND(num_operands, opnd, yOffset);

    // CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;

    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, instDesc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAVAHDCMinMaxFilter(
    VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
    VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
    HDCReturnFormat returnFormat, MMFEnableMode mmfMode,
    VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset,
    VISA_VectorOpnd *yOffset) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ISA_VA_Sub_Opcode subOp = ISA_HDC_MMF;

  if (IS_GEN_BOTH_PATH) {
    uint8_t execMode = VA_CONV_16x4;
    uint8_t functionality = mmfMode; /*reserved*/
    uint8_t pixelSize = returnFormat;

    status = m_builder->translateVISAVaSklPlusGeneralInst(
        subOp, surface->g4opnd /*surface*/, sampler->g4opnd /*sampler*/,
        execMode, functionality,
        // rest
        uOffset->g4opnd /*uOffset*/, vOffset->g4opnd /*vOffset*/,

        // 1pixel convolve
        NULL /*offsetOpnd*/,

        // FloodFill
        NULL /*loopCountOpnd*/, NULL /*pixelhMasOpnd*/,
        NULL /*pixelVMaskLeftOpnd*/, NULL /*pixelVMaskRightOpnd*/,

        // LBP Correlation
        NULL /*disparityOpnd*/,

        // correlation search
        NULL, NULL, NULL, NULL, NULL, NULL,

        // general
        NULL, Type_UNDEF, 0,

        // hdc
        pixelSize /*pixelSize*/, dstSurface->g4opnd /*dstSurfaceOpnd*/,
        xOffset->g4opnd /*dstXOffOnd*/, yOffset->g4opnd /*dstYOffOpnd*/, true);
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_VA_SKL_PLUS;
    VISA_INST_Desc *instDesc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;

    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, instDesc);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, subOp));
    ADD_OPND(num_operands, opnd, sampler);
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, returnFormat, true, subOp));
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, mmfMode, true, subOp));
    ADD_OPND(num_operands, opnd, dstSurface);
    ADD_OPND(num_operands, opnd, xOffset);
    ADD_OPND(num_operands, opnd, yOffset);

    // CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;

    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, instDesc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAVAHDCLBPCorrelation(
    VISA_StateOpndHandle *surface, VISA_VectorOpnd *uOffset,
    VISA_VectorOpnd *vOffset, VISA_VectorOpnd *disparity,
    VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset,
    VISA_VectorOpnd *yOffset) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ISA_VA_Sub_Opcode subOp = ISA_HDC_LBPCORRELATION;

  if (IS_GEN_BOTH_PATH) {
    uint8_t execMode = VA_CONV_16x4;
    uint8_t functionality = 0x3; /*reserved*/
    uint8_t pixelSize = 1;

    status = m_builder->translateVISAVaSklPlusGeneralInst(
        subOp, surface->g4opnd /*surface*/, NULL /*sampler*/, execMode,
        functionality,
        // rest
        uOffset->g4opnd /*uOffset*/, vOffset->g4opnd /*vOffset*/,

        // 1pixel convolve
        NULL /*offsetOpnd*/,

        // FloodFill
        NULL /*loopCountOpnd*/, NULL /*pixelhMasOpnd*/,
        NULL /*pixelVMaskLeftOpnd*/, NULL /*pixelVMaskRightOpnd*/,

        // LBP Correlation
        disparity->g4opnd /*disparityOpnd*/,

        // correlation search
        NULL, NULL, NULL, NULL, NULL, NULL,

        // general
        NULL, Type_UNDEF, 0,

        // hdc
        pixelSize /*pixelSize*/, dstSurface->g4opnd /*dstSurfaceOpnd*/,
        xOffset->g4opnd /*dstXOffOnd*/, yOffset->g4opnd /*dstYOffOpnd*/, true);
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_VA_SKL_PLUS;
    VISA_INST_Desc *instDesc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;

    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, instDesc);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, subOp));
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    ADD_OPND(num_operands, opnd, disparity);
    ADD_OPND(num_operands, opnd, dstSurface);
    ADD_OPND(num_operands, opnd, xOffset);
    ADD_OPND(num_operands, opnd, yOffset);

    // CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;

    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, instDesc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAVAHDCLBPCreation(VISA_StateOpndHandle *surface,
                                               VISA_VectorOpnd *uOffset,
                                               VISA_VectorOpnd *vOffset,
                                               LBPCreationMode mode,
                                               VISA_StateOpndHandle *dstSurface,
                                               VISA_VectorOpnd *xOffset,
                                               VISA_VectorOpnd *yOffset) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ISA_VA_Sub_Opcode subOp = ISA_HDC_LBPCREATION;

  if (mode == VA_3x3_AND_5x5) {
    vASSERT(false);
    return VISA_FAILURE;
  }

  if (IS_GEN_BOTH_PATH) {
    uint8_t execMode = VA_CONV_16x4;
    uint8_t functionality = mode; /*reserved*/
    uint8_t pixelSize = 1;

    status = m_builder->translateVISAVaSklPlusGeneralInst(
        subOp, surface->g4opnd /*surface*/, NULL /*sampler*/, execMode,
        functionality,
        // rest
        uOffset->g4opnd /*uOffset*/, vOffset->g4opnd /*vOffset*/,

        // 1pixel convolve
        NULL /*offsetOpnd*/,

        // FloodFill
        NULL /*loopCountOpnd*/, NULL /*pixelhMasOpnd*/,
        NULL /*pixelVMaskLeftOpnd*/, NULL /*pixelVMaskRightOpnd*/,

        // LBP Correlation
        NULL /*disparityOpnd*/,

        // correlation search
        NULL, NULL, NULL, NULL, NULL, NULL,

        // general
        NULL, Type_UNDEF, 0,

        // hdc
        pixelSize /*pixelSize*/, dstSurface->g4opnd /*dstSurfaceOpnd*/,
        xOffset->g4opnd /*dstXOffOnd*/, yOffset->g4opnd /*dstYOffOpnd*/, true);
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_VA_SKL_PLUS;
    VISA_INST_Desc *instDesc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;

    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, instDesc);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, subOp));
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, mode, true, subOp));
    ADD_OPND(num_operands, opnd, dstSurface);
    ADD_OPND(num_operands, opnd, xOffset);
    ADD_OPND(num_operands, opnd, yOffset);

    // CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;

    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, instDesc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAVAHDCConvolve1D(
    VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
    VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
    HDCReturnFormat returnFormat, Convolve1DDirection direction,
    VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset,
    VISA_VectorOpnd *yOffset) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ISA_VA_Sub_Opcode subOp = ISA_HDC_1DCONV_H;

  if (direction == VA_V_DIRECTION) {
    subOp = ISA_HDC_1DCONV_V;
  }

  if (IS_GEN_BOTH_PATH) {
    uint8_t execMode = VA_CONV_16x4;
    uint8_t functionality = 0x3; /*reserved*/
    uint8_t pixelSize = returnFormat;

    status = m_builder->translateVISAVaSklPlusGeneralInst(
        subOp, surface->g4opnd /*surface*/, sampler->g4opnd /*sampler*/,
        execMode, functionality,
        // rest
        uOffset->g4opnd /*uOffset*/, vOffset->g4opnd /*vOffset*/,

        // 1pixel convolve
        NULL /*offsetOpnd*/,

        // FloodFill
        NULL /*loopCountOpnd*/, NULL /*pixelhMasOpnd*/,
        NULL /*pixelVMaskLeftOpnd*/, NULL /*pixelVMaskRightOpnd*/,

        // LBP Correlation
        NULL /*disparityOpnd*/,

        // correlation search
        NULL, NULL, NULL, NULL, NULL, NULL,

        // general
        NULL, Type_UNDEF, 0,

        // hdc
        pixelSize /*pixelSize*/, dstSurface->g4opnd /*dstSurfaceOpnd*/,
        xOffset->g4opnd /*dstXOffOnd*/, yOffset->g4opnd /*dstYOffOpnd*/, true);
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_VA_SKL_PLUS;
    VISA_INST_Desc *instDesc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;

    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, instDesc);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, subOp));
    ADD_OPND(num_operands, opnd, sampler);
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, returnFormat, true, subOp));
    ADD_OPND(num_operands, opnd, dstSurface);
    ADD_OPND(num_operands, opnd, xOffset);
    ADD_OPND(num_operands, opnd, yOffset);

    // CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;

    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, instDesc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAVAHDCConvolve1Pixel(
    VISA_StateOpndHandle *sampler, VISA_StateOpndHandle *surface,
    VISA_VectorOpnd *uOffset, VISA_VectorOpnd *vOffset,
    HDCReturnFormat returnFormat, VISA_RawOpnd *offsets,
    VISA_StateOpndHandle *dstSurface, VISA_VectorOpnd *xOffset,
    VISA_VectorOpnd *yOffset) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  ISA_VA_Sub_Opcode subOp = ISA_HDC_1PIXELCONV;

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawSrcOperand(offsets);
    uint8_t execMode = VA_CONV_16x4;
    uint8_t functionality = 0x3; /*reserved*/
    uint8_t pixelSize = returnFormat;

    status = m_builder->translateVISAVaSklPlusGeneralInst(
        subOp, surface->g4opnd /*surface*/, sampler->g4opnd /*sampler*/,
        execMode, functionality,
        // rest
        uOffset->g4opnd /*uOffset*/, vOffset->g4opnd /*vOffset*/,

        // 1pixel convolve
        offsets->g4opnd /*offsetOpnd*/,

        // FloodFill
        NULL /*loopCountOpnd*/, NULL /*pixelhMasOpnd*/,
        NULL /*pixelVMaskLeftOpnd*/, NULL /*pixelVMaskRightOpnd*/,

        // LBP Correlation
        NULL /*disparityOpnd*/,

        // correlation search
        NULL, NULL, NULL, NULL, NULL, NULL,

        // general
        NULL, Type_UNDEF, 0,

        // hdc
        pixelSize /*pixelSize*/, dstSurface->g4opnd /*dstSurfaceOpnd*/,
        xOffset->g4opnd /*dstXOffOnd*/, yOffset->g4opnd /*dstYOffOpnd*/, true);
  }
  if (IS_VISA_BOTH_PATH) {
    ISA_Opcode opcode = ISA_VA_SKL_PLUS;
    VISA_INST_Desc *instDesc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[30];
    int num_pred_desc_operands = 0;
    int num_operands = 0;

    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, instDesc);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, subOp));
    ADD_OPND(num_operands, opnd, sampler);
    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, uOffset);
    ADD_OPND(num_operands, opnd, vOffset);
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(num_pred_desc_operands, num_operands,
                                   instDesc, returnFormat, true, subOp));
    ADD_OPND(num_operands, opnd, offsets);
    ADD_OPND(num_operands, opnd, dstSurface);
    ADD_OPND(num_operands, opnd, xOffset);
    ADD_OPND(num_operands, opnd, yOffset);

    // CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = EXEC_SIZE_1;

    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, instDesc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISALifetime(VISAVarLifetime startOrEnd,
                                       VISA_VectorOpnd *var) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  unsigned char properties = (unsigned char)startOrEnd;

  if (IS_GEN_BOTH_PATH) {
    status = m_builder->translateVISALifetimeInst(startOrEnd == LIFETIME_START,
                                                  var->g4opnd);
  }

  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[2];
    ISA_Opcode opcode = ISA_LIFETIME;
    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst_desc = &CISA_INST_table[opcode];

    opnd[0] = (VISA_opnd *)m_mem.alloc(sizeof(VISA_opnd));
    opnd[0]->_opnd.other_opnd = properties;
    opnd[0]->opnd_type = CISA_OPND_OTHER;
    opnd[0]->size =
        Get_VISA_Type_Size((VISA_Type)inst_desc->opnd_desc[0].data_type);
    opnd[0]->tag = (uint8_t)inst_desc->opnd_desc[0].opnd_type;

    opnd[1] = (VISA_opnd *)m_mem.alloc(sizeof(VISA_opnd));
    if (var->_opnd.v_opnd.tag == OPERAND_GENERAL) {
      opnd[0]->_opnd.other_opnd |= (OPERAND_GENERAL << 4);
      opnd[1]->_opnd.other_opnd = var->_opnd.v_opnd.opnd_val.gen_opnd.index;
    } else if (var->_opnd.v_opnd.tag == OPERAND_ADDRESS) {
      opnd[0]->_opnd.other_opnd |= (OPERAND_ADDRESS << 4);
      opnd[1]->_opnd.other_opnd = var->_opnd.v_opnd.opnd_val.addr_opnd.index;
    } else if (var->_opnd.v_opnd.tag == OPERAND_PREDICATE) {
      opnd[0]->_opnd.other_opnd |= (OPERAND_PREDICATE << 4);
      opnd[1]->_opnd.other_opnd = var->_opnd.v_opnd.opnd_val.pred_opnd.index;
    }
    opnd[1]->opnd_type = CISA_OPND_OTHER;
    opnd[1]->size =
        Get_VISA_Type_Size((VISA_Type)inst_desc->opnd_desc[1].data_type);
    opnd[1]->tag = (uint8_t)inst_desc->opnd_desc[1].opnd_type;

    inst->createCisaInstruction(opcode, 1, 0, PredicateOpnd::getNullPred(),
                                opnd, 2, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

static void setAlignIfLarger(
    var_info_t *varinfo, VISA_Align A, const IR_Builder& irb) {
  VISA_Align oldA = varinfo->getAlignment();
  unsigned grfSize = irb.getGRFSize();
  if (getAlignInBytes(A, grfSize) > getAlignInBytes(oldA, grfSize)) {
    varinfo->bit_properties = ((varinfo->bit_properties & ~0xF0) | (A << 4));
  }
}

int VISAKernelImpl::AppendVISADpasInstCommon(
    ISA_Opcode opcode, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISA_RawOpnd *tmpDst, VISA_RawOpnd *src0, VISA_RawOpnd *src1,
    VISA_VectorOpnd *src2, VISA_VectorOpnd *src3, GenPrecision src2Precision,
    GenPrecision src1Precision, uint8_t Depth, uint8_t Count) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  const IR_Builder& irb = *getIRBuilder();
  // Make sure alignment is set correctly.
  // dst : grf aligned
  var_info_t *dcl = &tmpDst->decl->genVar;
  uint32_t alignBytes = CISATypeTable[dcl->getType()].typeSize *
                        Get_VISA_Exec_Size(executionSize);
  setAlignIfLarger(dcl, getCISAAlign(alignBytes), irb);

  // src0 : grf aligned (it could be null)
  if (src0->index != 0) {
    dcl = &src0->decl->genVar;
    alignBytes = CISATypeTable[dcl->getType()].typeSize *
                 Get_VISA_Exec_Size(executionSize);
    setAlignIfLarger(dcl, getCISAAlign(alignBytes), irb);
  }

  // src1 : grf aligned
  dcl = &src1->decl->genVar;
  setAlignIfLarger(dcl, getCISAAlign(irb.getGRFSize()), irb);

  if (IS_GEN_BOTH_PATH) {
    {
      vISA_ASSERT_INPUT(
          Get_VISA_Exec_Size(executionSize) == m_builder->getNativeExecSize(),
          "execution size of DPAS must be equal to native execution size!");
    }

    // src2 : QW/OW/half-grf/GRF-aligned (vectorOpnd, g4opnd has been created)
    G4_RegVar *src2RegVar = src2->g4opnd->getTopDcl()->getRegVar();
    uint32_t src1Bits = GenPrecisionTable[(int)src1Precision].BitSize;
    uint32_t src2Bits = GenPrecisionTable[(int)src2Precision].BitSize;
    if (src2Precision == GenPrecision::FP16 ||
        src2Precision == GenPrecision::BF16 ||
        src2Precision == GenPrecision::TF32 ||
        src2Precision == GenPrecision::BF8 ||
        src2Precision == GenPrecision::HF8 ||
        src2Bits == 8 || (src1Bits <= 4 && src2Bits == 4)) {
      G4_SubReg_Align srAlign = getIRBuilder()->getGRFAlign();
      if (Count != 8)
        srAlign = getIRBuilder()->getHalfGRFAlign();
      src2RegVar->getDeclare()->setSubRegAlign(srAlign);
    } else if ((src1Bits == 8 && src2Bits == 4) ||
               (src1Bits <= 4 && src2Bits == 2)) {
      // OWORD aligned
      src2RegVar->getDeclare()->setSubRegAlign(Eight_Word);
    }
    else {
      // OPS_PER_CHAN=4, U2/S2;  QWORD aligned
      src2RegVar->getDeclare()->setSubRegAlign(Four_Word);
    }


    CreateGenRawDstOperand(tmpDst);
    CreateGenRawSrcOperand(src0);
    CreateGenRawSrcOperand(src1);
    status = m_builder->translateVISADpasInst(
        executionSize, emask, GetGenOpcodeFromVISAOpcode(opcode),
        tmpDst->g4opnd->asDstRegRegion(), src0->g4opnd->asSrcRegRegion(),
        src1->g4opnd->asSrcRegRegion(), src2->g4opnd->asSrcRegRegion(),
        src3 ? src3->g4opnd->asSrcRegRegion() : nullptr, nullptr, src2Precision,
        src1Precision, Depth, Count);
  }
  if (IS_VISA_BOTH_PATH) {
    int num_pred_desc_operands =
        2; // accounting for exec_size and pred_id in descriptor
    int num_operands = 0;
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[6];
    VISA_opnd *dst = tmpDst;

    // Precision, depth, and count are saved as VISA_opnd
    uint32_t info = DpasInfoToUI32(src2Precision, src1Precision, Depth, Count);
    VISA_opnd *dpasInfo = CreateOtherOpnd(info, ISA_TYPE_UD);

    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);

    ADD_OPND(num_operands, opnd, dst);

    ADD_OPND(num_operands, opnd, src0);

    ADD_OPND(num_operands, opnd, src1);

    ADD_OPND(num_operands, opnd, src2);


    ADD_OPND(num_operands, opnd, dpasInfo);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += (emask << 4);
    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISADpasInst(
    ISA_Opcode opcode, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISA_RawOpnd *tmpDst, VISA_RawOpnd *src0, VISA_RawOpnd *src1,
    VISA_VectorOpnd *src2, GenPrecision src2Precision,
    GenPrecision src1Precision, uint8_t Depth, uint8_t Count) {
  return AppendVISADpasInstCommon(opcode, emask, executionSize, tmpDst, src0,
                                  src1, src2, nullptr, src2Precision,
                                  src1Precision, Depth, Count);
}

int VISAKernelImpl::AppendVISABdpasInst(
    ISA_Opcode opcode, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISA_RawOpnd *dst, VISA_RawOpnd *src0, VISA_RawOpnd *src1,
    VISA_RawOpnd *src2, VISA_VectorOpnd *src3, VISA_VectorOpnd *src4,
    GenPrecision src2Precision, GenPrecision src1Precision, uint8_t Depth,
    uint8_t Count) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  int status = VISA_SUCCESS;

  vISA_ASSERT_INPUT(m_kernel->getPlatform() > Xe3, "bdpass is xe3p+");
  vISA_ASSERT_INPUT(Get_VISA_Exec_Size(executionSize) == 16,
      "exec size of bdpas must be 16");
  vISA_ASSERT_INPUT(Depth == 8, "depth of bdpas must be 8!");
  vISA_ASSERT_INPUT(Count == 8, "repeat count of bdpas must be 8!");
  AppendVISAInstCommon();

  if (IS_GEN_BOTH_PATH) {
    const IR_Builder& irb = *getIRBuilder();
    // Dst, Src0, Src1 and Src2 are GRF aligned
    var_info_t *dstDcl = &dst->decl->genVar;
    setAlignIfLarger(dstDcl, getCISAAlign(irb.getGRFSize()), irb);
    if (src0->index != 0) {
      // Src0 could be null
      var_info_t *src0Dcl = &src0->decl->genVar;
      setAlignIfLarger(src0Dcl, getCISAAlign(irb.getGRFSize()), irb);
    }
    var_info_t *src1Dcl = &src1->decl->genVar;
    setAlignIfLarger(src1Dcl, getCISAAlign(irb.getGRFSize()), irb);
    var_info_t *src2Dcl = &src1->decl->genVar;
    setAlignIfLarger(src2Dcl, getCISAAlign(irb.getGRFSize()), irb);

    CreateGenRawDstOperand(dst);
    CreateGenRawSrcOperand(src0);
    CreateGenRawSrcOperand(src1);
    CreateGenRawSrcOperand(src2);
    status = m_builder->translateVISADpasInst(executionSize, emask,
        GetGenOpcodeFromVISAOpcode(opcode), dst->g4opnd->asDstRegRegion(),
        src0->g4opnd->asSrcRegRegion(), src1->g4opnd->asSrcRegRegion(),
        src2->g4opnd->asSrcRegRegion(), src3->g4opnd->asSrcRegRegion(),
        src4->g4opnd->asSrcRegRegion(), src2Precision, src1Precision, Depth,
        Count);
  }
  if (IS_VISA_BOTH_PATH) {
    // accounting for exec_size and pred_id in descriptor
    int num_pred_desc_operands = 2;
    int num_operands = 0;
    VISA_INST_Desc *inst_desc = nullptr;
    VISA_opnd *opnd[7];

    // Precision, depth, and count are saved as VISA_opnd
    uint32_t info = DpasInfoToUI32(src2Precision, src1Precision, Depth, Count);
    VISA_opnd *dpasInfo = CreateOtherOpnd(info, ISA_TYPE_UD);

    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);

    ADD_OPND(num_operands, opnd, dst);
    ADD_OPND(num_operands, opnd, src0);
    ADD_OPND(num_operands, opnd, src1);
    ADD_OPND(num_operands, opnd, src2);
    ADD_OPND(num_operands, opnd, src3);
    ADD_OPND(num_operands, opnd, src4);
    ADD_OPND(num_operands, opnd, dpasInfo);

    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += (emask << 4);
    inst->createCisaInstruction(opcode, size, 0, PredicateOpnd::getNullPred(),
                                opnd, num_operands, inst_desc);
    addInstructionToEnd(inst);
  }
  return status;
}


int VISAKernelImpl::AppendVISABfnInst(
    uint8_t booleanFuncCtrl, VISA_PredOpnd *pred, bool satMode,
    VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISA_VectorOpnd *tmpDst, VISA_VectorOpnd *src0, VISA_VectorOpnd *src1,
    VISA_VectorOpnd *src2) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;
  if (IS_GEN_BOTH_PATH) {
    G4_Predicate *g4Pred = (pred != NULL) ? pred->g4opnd->asPredicate() : NULL;
    status = m_builder->translateVISABfnInst(
        booleanFuncCtrl, executionSize, emask, g4Pred,
        satMode ? g4::SAT : g4::NOSAT, nullptr,
        tmpDst->g4opnd->asDstRegRegion(), src0->g4opnd, GET_G4_OPNG(src1),
        GET_G4_OPNG(src2));
  }
  if (IS_VISA_BOTH_PATH) {
    int num_pred_desc_operands =
        2; // accounting for exec_size and pred_id in descriptor
    int num_operands = 0;
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[5]; // dst, src0, src1, src2, BooleanFuncCtrl
    VISA_opnd *dst = tmpDst;

    inst_desc = &CISA_INST_table[ISA_BFN];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);
    VISA_Modifier mod = MODIFIER_NONE;

    if (satMode) {
#if START_ASSERT_CHECK
      if (tmpDst == NULL) {
        vISA_ASSERT_INPUT(false, "Destination for arithmetic instruction is null");
        return VISA_FAILURE;
      }
#endif
      mod = MODIFIER_SAT;
      dst = (VISA_opnd *)m_mem.alloc(sizeof(VISA_opnd));
      *dst = *tmpDst;
      dst->_opnd.v_opnd.tag += mod << 3;
    }

    ADD_OPND(num_operands, opnd, dst);
    ADD_OPND(num_operands, opnd, src0);
    ADD_OPND(num_operands, opnd, src1);
    ADD_OPND(num_operands, opnd, src2);
    ADD_OPND(num_operands, opnd, CreateOtherOpnd(booleanFuncCtrl, ISA_TYPE_UB));
    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += emask << 4;
    inst->createCisaInstruction(ISA_BFN, size, 0, predOpnd, opnd, num_operands,
                                inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAQwordGatherInst(
    VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISA_SVM_Block_Num numBlocks, VISA_StateOpndHandle *surface,
    VISA_RawOpnd *address, VISA_RawOpnd *dst) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    G4_Predicate *g4Pred = pred ? pred->g4opnd->asPredicate() : nullptr;
    CreateGenRawSrcOperand(address);
    CreateGenRawDstOperand(dst);
    status = m_builder->translateVISAQWGatherInst(
        executionSize, emask, g4Pred, numBlocks,
        surface->g4opnd->asSrcRegRegion(), address->g4opnd->asSrcRegRegion(),
        dst->g4opnd->asDstRegRegion());
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = &CISA_INST_table[ISA_QW_GATHER];
    VISA_opnd *opnd[10];

    int num_operands = 0;
    unsigned char size = executionSize;
    size += emask << 4;

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();

    ADD_OPND(num_operands, opnd, CreateOtherOpnd(numBlocks, ISA_TYPE_UB));

    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, address);
    ADD_OPND(num_operands, opnd, dst);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst->createCisaInstruction(ISA_QW_GATHER, size, 0, predOpnd, opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISAQwordScatterInst(
    VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    VISA_SVM_Block_Num numBlocks, VISA_StateOpndHandle *surface,
    VISA_RawOpnd *address, VISA_RawOpnd *src) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    G4_Predicate *g4Pred = pred ? pred->g4opnd->asPredicate() : nullptr;
    CreateGenRawSrcOperand(address);
    CreateGenRawSrcOperand(src);
    status = m_builder->translateVISAQWScatterInst(
        executionSize, emask, g4Pred, numBlocks,
        surface->g4opnd->asSrcRegRegion(), address->g4opnd->asSrcRegRegion(),
        src->g4opnd->asSrcRegRegion());
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = &CISA_INST_table[ISA_QW_SCATTER];
    VISA_opnd *opnd[10];

    int num_operands = 0;
    unsigned char size = executionSize;
    size += emask << 4;

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();

    ADD_OPND(num_operands, opnd, CreateOtherOpnd(numBlocks, ISA_TYPE_UB));

    ADD_OPND(num_operands, opnd, surface);
    ADD_OPND(num_operands, opnd, address);
    ADD_OPND(num_operands, opnd, src);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst->createCisaInstruction(ISA_QW_SCATTER, size, 0, predOpnd, opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

VISA_BUILDER_API int VISAKernelImpl::AppendVISALscUntypedLoad(
    LSC_OP subOpcode, LSC_SFID sfid, VISA_PredOpnd *pred,
    VISA_Exec_Size execSize, VISA_EMask_Ctrl emask,
    LSC_CACHE_OPTS cacheOpts, bool ov,
    LSC_ADDR addr, LSC_DATA_SHAPE data,
    VISA_VectorOpnd *surface, unsigned surfaceIndex,
    VISA_RawOpnd *dstData, VISA_RawOpnd *src0Addr) {
  return AppendVISALscUntypedInst(subOpcode, sfid, pred, execSize, emask,
                                  cacheOpts, ov, addr, data,
                                  surface, surfaceIndex, dstData,
                                  src0Addr, nullptr, nullptr);
}

VISA_BUILDER_API int VISAKernelImpl::AppendVISALscUntypedStore(
    LSC_OP subOpcode, LSC_SFID sfid, VISA_PredOpnd *pred,
    VISA_Exec_Size execSize, VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts,
    LSC_ADDR addr, LSC_DATA_SHAPE data,
    VISA_VectorOpnd *surface, unsigned surfaceIndex,
    VISA_RawOpnd *src0Addr, VISA_RawOpnd *src1Data) {
  return AppendVISALscUntypedInst(subOpcode, sfid, pred, execSize, emask,
                                  cacheOpts, false, addr, data,
                                  surface, surfaceIndex, nullptr,
                                  src0Addr, src1Data, nullptr);
}

VISA_BUILDER_API int VISAKernelImpl::AppendVISALscUntypedAtomic(
    LSC_OP subOpcode, LSC_SFID sfid, VISA_PredOpnd *pred,
    VISA_Exec_Size execSize, VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts,
    LSC_ADDR addr, LSC_DATA_SHAPE data,
    VISA_VectorOpnd *surface, unsigned surfaceIndex,
    VISA_RawOpnd *dstReadBack, VISA_RawOpnd *src0Addr,
    VISA_RawOpnd *src1AtomOpnd1, VISA_RawOpnd *src2AtomOpnd2) {
  return AppendVISALscUntypedInst(subOpcode, sfid, pred, execSize, emask,
                                  cacheOpts, false, addr, data,
                                  surface, surfaceIndex,
                                  dstReadBack,
                                  src0Addr, src1AtomOpnd1, src2AtomOpnd2);
}

static const int LSC_ZERO = 0;

#define LSC_CHECK_NULL_VECTOR_OPERAND(X)                                       \
  do {                                                                         \
    if ((X) == nullptr) {                                                      \
      auto r = CreateVISAImmediate(X, &LSC_ZERO, ISA_TYPE_UD);                 \
      if (r != VISA_SUCCESS)                                                   \
        return r;                                                              \
    }                                                                          \
  } while (0)
#define LSC_CHECK_NULL_OPND(X, IS_DST)                                         \
  do {                                                                         \
    if ((X) == nullptr) {                                                      \
      auto r = CreateVISANullRawOperand((X), IS_DST);                          \
      if (r != VISA_SUCCESS)                                                   \
        return r;                                                              \
    }                                                                          \
  } while (0)
#define LSC_CHECK_NULL_DST(X) LSC_CHECK_NULL_OPND(X, true)
#define LSC_CHECK_NULL_SRC(X) LSC_CHECK_NULL_OPND(X, false)

VISA_BUILDER_API int VISAKernelImpl::AppendVISALscExtendedCacheCtrlInst(
    LSC_OP subOpcode, LSC_SFID lscSfid, VISA_PredOpnd *pred,
    VISA_Exec_Size execSize, VISA_EMask_Ctrl emask,
    LSC_CACHE_CTRL_OPERATION ccop, LSC_CACHE_CTRL_SIZE ccsize,
    LSC_CACHE_OPTS cacheOpts, LSC_ADDR addr, VISA_RawOpnd *src0Addr) {

  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  vISA_ASSERT_INPUT(lscSfid != LSC_TGM,
                      "cannot use TGM on extended cache crl message");

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  LSC_CHECK_NULL_SRC(src0Addr);

  VISA_RawOpnd *dstData = nullptr;
  VISA_RawOpnd *src1Data = nullptr;

  LSC_CHECK_NULL_DST(dstData);
  LSC_CHECK_NULL_SRC(src1Data);

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawSrcOperand(src0Addr);
    status = m_builder->translateLscExtendedCacheCtrlInst(
              pred ? pred->g4opnd->asPredicate() : nullptr, execSize, emask,
              cacheOpts, addr, ccsize, ccop, dstData->g4opnd->asDstRegRegion(),
              src0Addr->g4opnd->asSrcRegRegion(), src1Data->g4opnd->asSrcRegRegion());
  }

  if (IS_VISA_BOTH_PATH) {
    const VISA_INST_Desc *instDesc = &CISA_INST_table[ISA_LSC_UNTYPED];
    VISA_opnd *opnds[MAX_OPNDS_PER_INST]{};
    int numOpnds = 0;

    // use same order for load, store, and atomic
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(subOpcode, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(lscSfid, ISA_TYPE_UB));
    //
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l1, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l2, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l3, ISA_TYPE_UB));
    //
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(static_cast<unsigned>(ccop), ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(static_cast<unsigned>(ccsize), ISA_TYPE_UB));
    //
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(addr.type, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(addr.size, ISA_TYPE_UB));
    //
    ADD_OPND(numOpnds, opnds, src0Addr);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);
    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();
    // Pack execution size & mask
    unsigned size = execSize;
    PACK_EXEC_SIZE(size, emask);

    inst->createCisaInstruction(ISA_LSC_UNTYPED, (uint8_t)size, 0, predOpnd,
                                opnds, numOpnds, instDesc, verifier);
    addInstructionToEnd(inst);
  }
  return status;
}
VISA_BUILDER_API int VISAKernelImpl::AppendVISALscUntypedInst(
    LSC_OP subOpcode, LSC_SFID lscSfid, VISA_PredOpnd *pred,
    VISA_Exec_Size execSize, VISA_EMask_Ctrl emask,
    LSC_CACHE_OPTS cacheOpts, bool ov,
    LSC_ADDR addr, LSC_DATA_SHAPE dataShape,
    VISA_VectorOpnd *surface, unsigned surfaceIndex,
    VISA_RawOpnd *dstData, VISA_RawOpnd *src0Addr,
    VISA_RawOpnd *src1Data, VISA_RawOpnd *src2Data) {
  if (subOpcode == LSC_LOAD_STRIDED || subOpcode == LSC_STORE_STRIDED) {
    return AppendVISALscUntypedStridedInst(
        subOpcode, lscSfid, pred, execSize, emask, cacheOpts, addr, dataShape,
        surface, surfaceIndex, dstData, src0Addr, nullptr, src1Data);
  }
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  vISA_ASSERT_INPUT(lscSfid != LSC_TGM, "cannot use TGM on an untyped message");

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  LSC_CHECK_NULL_VECTOR_OPERAND(surface);
  LSC_CHECK_NULL_DST(dstData);
  LSC_CHECK_NULL_SRC(src0Addr);
  LSC_CHECK_NULL_SRC(src1Data);
  LSC_CHECK_NULL_SRC(src2Data);

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawDstOperand(dstData);
    CreateGenRawSrcOperand(src0Addr);
    CreateGenRawSrcOperand(src1Data);
    CreateGenRawSrcOperand(src2Data);

    if (m_options->getOption(vISA_enableEfficient64b)) {
      if (cacheOpts.l2 == LSC_CACHING_DEFAULT) {
        cacheOpts.l2 = cacheOpts.l3;
        if (cacheOpts.l3 != LSC_CACHING_DEFAULT) {
          cacheOpts.l3 = LSC_CACHING_UNCACHED;
        }
      }
      // call the efficient 64b translate function
      status = m_builder->translateLscUntypedInstUnified(
            subOpcode, lscSfid, pred ? pred->g4opnd->asPredicate() : nullptr,
            execSize, emask, cacheOpts, ov, addr, dataShape, surface->g4opnd,
            surfaceIndex, dstData->g4opnd->asDstRegRegion(),
            src0Addr->g4opnd->asSrcRegRegion(), nullptr,
            src1Data->g4opnd->asSrcRegRegion(), src2Data->g4opnd->asSrcRegRegion());
    } else {
      status = m_builder->translateLscUntypedInst(
          subOpcode, lscSfid, pred ? pred->g4opnd->asPredicate() : nullptr,
          execSize, emask, cacheOpts, addr, dataShape, surface->g4opnd,
          surfaceIndex, dstData->g4opnd->asDstRegRegion(),
          src0Addr->g4opnd->asSrcRegRegion(), nullptr,
          src1Data->g4opnd->asSrcRegRegion(),
          src2Data->g4opnd->asSrcRegRegion());
    }
  }

  if (IS_VISA_BOTH_PATH) {
    const VISA_INST_Desc *instDesc = &CISA_INST_table[ISA_LSC_UNTYPED];
    VISA_opnd *opnds[MAX_OPNDS_PER_INST]{};
    int numOpnds = 0;

    // use same order for load, store, and atomic
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(subOpcode, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(lscSfid, ISA_TYPE_UB));
    //
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l1, ISA_TYPE_UB));
    if (m_options->getOption(vISA_enableEfficient64b))
      ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l2, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l3, ISA_TYPE_UB));
    if (hasOV(lscSfid, subOpcode))
      ADD_OPND(numOpnds, opnds, CreateOtherOpnd(ov, ISA_TYPE_BOOL));
    //
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(addr.type, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(addr.immScale, ISA_TYPE_UW));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(addr.immOffset, ISA_TYPE_D));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(addr.size, ISA_TYPE_UB));
    //
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape.size, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape.order, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape.elems, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape.chmask, ISA_TYPE_UB));
    //
    ADD_OPND(numOpnds, opnds, surface);
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(surfaceIndex, ISA_TYPE_D));
    ADD_OPND(numOpnds, opnds, dstData);
    ADD_OPND(numOpnds, opnds, src0Addr);
    ADD_OPND(numOpnds, opnds, src1Data);
    ADD_OPND(numOpnds, opnds, src2Data);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);
    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();
    // Pack execution size & mask
    unsigned size = execSize;
    PACK_EXEC_SIZE(size, emask);

    inst->createCisaInstruction(ISA_LSC_UNTYPED, (uint8_t)size, 0, predOpnd,
                                opnds, numOpnds, instDesc, verifier);
    addInstructionToEnd(inst);
  }

  return status;
}

// for a strided load, this is the "natural" stride to enable a
// block (packed) load.
//
// TODO: move this to IsaDescription.cpp as a helper
static int lscStridedOpBlockStride(LSC_DATA_SHAPE dataShape) {
  // e.g. given :u32x4, we really want successive lanes to be a 4*32 apart
  //       (this is like a float4 block load)
  // but transposed :u32x4t, we want packed elements and the vector length
  // is just the width of the block
  int stride = 0;
  switch (dataShape.size) {
  case LSC_DATA_SIZE_8b:
    stride = 1;
    break;
  case LSC_DATA_SIZE_16b:
    stride = 2;
    break;
  // case LSC_DATA_SIZE_32b:
  // ... and all the DW conversion cases
  default:
    stride = 4;
    break;
  case LSC_DATA_SIZE_64b:
    stride = 8;
    break;
  }
  if (dataShape.order == LSC_DATA_ORDER_NONTRANSPOSE) {
    switch (dataShape.elems) {
    case LSC_DATA_ELEMS_1:
      stride *= 1;
      break;
    case LSC_DATA_ELEMS_2:
      stride *= 2;
      break;
    case LSC_DATA_ELEMS_3:
      stride *= 3;
      break;
    case LSC_DATA_ELEMS_4:
      stride *= 4;
      break;
    case LSC_DATA_ELEMS_8:
      stride *= 8;
      break;
    case LSC_DATA_ELEMS_16:
      stride *= 16;
      break;
    case LSC_DATA_ELEMS_32:
      stride *= 32;
      break;
    case LSC_DATA_ELEMS_64:
      stride *= 64;
      break;
    default:
      vISA_ASSERT_UNREACHABLE("invalid vector size");
      break;
    }
  }
  return stride;
}

VISA_BUILDER_API int VISAKernelImpl::AppendVISALscUntypedStridedInst(
    LSC_OP op, LSC_SFID lscSfid, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
    VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, LSC_ADDR addrInfo,
    LSC_DATA_SHAPE dataShape,
    VISA_VectorOpnd *surface, unsigned surfaceIndex,
    VISA_RawOpnd *dstData,
    VISA_RawOpnd *src0AddrBase, VISA_VectorOpnd *src0AddrPitch,
    VISA_RawOpnd *src1Data) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  vISA_ASSERT_INPUT(op == LSC_LOAD_STRIDED || op == LSC_STORE_STRIDED,
               "must be a strided op");
  vISA_ASSERT_INPUT(lscSfid != LSC_TGM, "cannot use TGM on an untyped message");

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  LSC_CHECK_NULL_VECTOR_OPERAND(surface);
  LSC_CHECK_NULL_DST(dstData);
  if (src0AddrPitch == nullptr) {
    int defaultPitch = lscStridedOpBlockStride(dataShape);
    status |= CreateVISAImmediate(src0AddrPitch, &defaultPitch, ISA_TYPE_UD);
  }
  LSC_CHECK_NULL_SRC(src1Data);

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawDstOperand(dstData);
    CreateGenRawSrcOperand(src0AddrBase);
    CreateGenRawSrcOperand(src1Data);
    status |= m_builder->translateLscUntypedInst(
        op, lscSfid, pred ? pred->g4opnd->asPredicate() : nullptr, execSize,
        emask, cacheOpts, addrInfo, dataShape,
        surface->g4opnd, surfaceIndex,
        dstData->g4opnd->asDstRegRegion(),
        src0AddrBase->g4opnd->asSrcRegRegion(), src0AddrPitch->g4opnd,
        src1Data->g4opnd->asSrcRegRegion(), nullptr);
  }

  if (IS_VISA_BOTH_PATH) {
    const VISA_INST_Desc *instDesc = &CISA_INST_table[ISA_LSC_UNTYPED];
    VISA_opnd *opnds[MAX_OPNDS_PER_INST]{};
    int numOpnds = 0;

    // use same order for load, store, and atomic
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(op, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(lscSfid, ISA_TYPE_UB));
    //
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l1, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l3, ISA_TYPE_UB));
    //
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(addrInfo.type, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(addrInfo.immScale, ISA_TYPE_UW));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(addrInfo.immOffset, ISA_TYPE_D));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(addrInfo.size, ISA_TYPE_UB));
    //
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape.size, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape.order, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape.elems, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(0, ISA_TYPE_UB)); // chmask
    //
    ADD_OPND(numOpnds, opnds, surface);
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(surfaceIndex, ISA_TYPE_UD));
    ADD_OPND(numOpnds, opnds, dstData);
    ADD_OPND(numOpnds, opnds, src0AddrBase);
    ADD_OPND(numOpnds, opnds, src0AddrPitch);
    ADD_OPND(numOpnds, opnds, src1Data);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);
    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();
    // Pack execution size & mask
    unsigned size = execSize;
    PACK_EXEC_SIZE(size, emask);

    inst->createCisaInstruction(ISA_LSC_UNTYPED, (uint8_t)size, 0, predOpnd,
                                opnds, numOpnds, instDesc, verifier);
    addInstructionToEnd(inst);
  }

  return status;
}

VISA_BUILDER_API int VISAKernelImpl::AppendVISALscUntypedBlock2DInst(
    LSC_OP op, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
    VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts,
    LSC_DATA_SHAPE_BLOCK2D dataShape2D, VISA_RawOpnd *dstData,
    VISA_VectorOpnd* src0AddrPayload,
    int xImmOffset, int yImmOffset, VISA_RawOpnd *src1Data) {

  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  LSC_CHECK_NULL_DST(dstData);
  LSC_CHECK_NULL_SRC(src1Data);

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawDstOperand(dstData);
    CreateGenRawSrcOperand(src1Data);

    G4_Operand *src0AddrSrcRgn = src0AddrPayload->g4opnd;

    if (m_options->getOption(vISA_enableEfficient64b)) {
      // call the efficient 64b translate function
      status = m_builder->translateLscUntypedBlock2DInstUnified(
          op, LSC_UGM, pred ? pred->g4opnd->asPredicate() : nullptr, execSize,
          emask, cacheOpts, dataShape2D, dstData->g4opnd->asDstRegRegion(),
          src0AddrSrcRgn, src1Data->g4opnd->asSrcRegRegion(), xImmOffset,
          yImmOffset);
    } else {
      status |= m_builder->translateLscUntypedBlock2DInst(
          op, LSC_UGM, pred ? pred->g4opnd->asPredicate() : nullptr, execSize,
          emask, cacheOpts, dataShape2D, dstData->g4opnd->asDstRegRegion(),
          src0AddrSrcRgn, src1Data->g4opnd->asSrcRegRegion(), xImmOffset,
          yImmOffset);
    }
  }

  if (IS_VISA_BOTH_PATH) {
    const VISA_INST_Desc *instDesc = &CISA_INST_table[ISA_LSC_UNTYPED];
    VISA_opnd *opnds[MAX_OPNDS_PER_INST]{};
    int numOpnds = 0;

    // use same order for load, store, and atomic
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(op, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(LSC_UGM, ISA_TYPE_UB));
    //
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l1, ISA_TYPE_UB));
    if (m_options->getOption(vISA_enableEfficient64b))
      ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l2, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l3, ISA_TYPE_UB));
    //
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape2D.size, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape2D.order, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape2D.blocks, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape2D.width, ISA_TYPE_UW));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape2D.height, ISA_TYPE_UW));
    ADD_OPND(numOpnds, opnds,
             CreateOtherOpnd(dataShape2D.vnni ? 1 : 0, ISA_TYPE_UB));
    //
    ADD_OPND(numOpnds, opnds, dstData);

    ADD_OPND(numOpnds, opnds, src0AddrPayload);
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(xImmOffset, ISA_TYPE_D));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(yImmOffset, ISA_TYPE_D));

    ADD_OPND(numOpnds, opnds, src1Data);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);
    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();
    // Pack execution size & mask
    unsigned size = execSize;
    PACK_EXEC_SIZE(size, emask);

    inst->createCisaInstruction(ISA_LSC_UNTYPED, (uint8_t)size, 0, predOpnd,
                                opnds, numOpnds, instDesc, verifier);
    addInstructionToEnd(inst);
  }
  return status;
}

VISA_BUILDER_API int VISAKernelImpl::AppendVISALscUntypedBlock2DInst(
    LSC_OP op, LSC_SFID lscSfid, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
    VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts,
    LSC_DATA_SHAPE_BLOCK2D dataShape2D, VISA_RawOpnd *dstData,
    VISA_VectorOpnd *src0Addrs[LSC_BLOCK2D_ADDR_PARAMS],
    int xImmOffset, int yImmOffset, VISA_RawOpnd *src1Data) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  vISA_ASSERT_INPUT(lscSfid != LSC_TGM, "cannot use TGM on an untyped message");

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  LSC_CHECK_NULL_DST(dstData);
  LSC_CHECK_NULL_SRC(src1Data);

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawDstOperand(dstData);
    // CreateGenRawSrcOperand(src0Addrs*); created in parse
    CreateGenRawSrcOperand(src1Data);

    G4_Operand *src0AddrSrcRgns[LSC_BLOCK2D_ADDR_PARAMS];
    for (size_t i = 0; i < LSC_BLOCK2D_ADDR_PARAMS; i++) {
      src0AddrSrcRgns[i] = src0Addrs[i]->g4opnd;
    }
    if (m_options->getOption(vISA_enableEfficient64b)) {
      // call the efficient 64b translate function
      status = m_builder->translateLscUntypedBlock2DInstUnified(
          op, lscSfid, pred ? pred->g4opnd->asPredicate() : nullptr, execSize,
          emask, cacheOpts, dataShape2D, dstData->g4opnd->asDstRegRegion(),
          src0AddrSrcRgns, src1Data->g4opnd->asSrcRegRegion(), xImmOffset,
          yImmOffset);
    } else {
      status |= m_builder->translateLscUntypedBlock2DInst(
          op, lscSfid, pred ? pred->g4opnd->asPredicate() : nullptr, execSize,
          emask, cacheOpts, dataShape2D, dstData->g4opnd->asDstRegRegion(),
          src0AddrSrcRgns, src1Data->g4opnd->asSrcRegRegion(), xImmOffset,
          yImmOffset);
    }
  }

  if (IS_VISA_BOTH_PATH) {
    const VISA_INST_Desc *instDesc = &CISA_INST_table[ISA_LSC_UNTYPED];
    VISA_opnd *opnds[MAX_OPNDS_PER_INST]{};
    int numOpnds = 0;

    // use same order for load, store, and atomic
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(op, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(lscSfid, ISA_TYPE_UB));
    //
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l1, ISA_TYPE_UB));
    if (m_options->getOption(vISA_enableEfficient64b))
      ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l2, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l3, ISA_TYPE_UB));
    //
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape2D.size, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape2D.order, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape2D.blocks, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape2D.width, ISA_TYPE_UW));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape2D.height, ISA_TYPE_UW));
    ADD_OPND(numOpnds, opnds,
             CreateOtherOpnd(dataShape2D.vnni ? 1 : 0, ISA_TYPE_UB));
    //
    ADD_OPND(numOpnds, opnds, dstData);
    size_t i = 0;
    for (; i < 4; i++) {
      ADD_OPND(numOpnds, opnds, src0Addrs[i]);
    }

    // Block x and x offset
    ADD_OPND(numOpnds, opnds, src0Addrs[4]);
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(xImmOffset, ISA_TYPE_D));

    // Block Y and y offset
    ADD_OPND(numOpnds, opnds, src0Addrs[5]);
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(yImmOffset, ISA_TYPE_D));

    ADD_OPND(numOpnds, opnds, src1Data);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);
    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();
    // Pack execution size & mask
    unsigned size = execSize;
    PACK_EXEC_SIZE(size, emask);

    inst->createCisaInstruction(ISA_LSC_UNTYPED, (uint8_t)size, 0, predOpnd,
                                opnds, numOpnds, instDesc, verifier);
    addInstructionToEnd(inst);
  }
  return status;
}

VISA_BUILDER_API int VISAKernelImpl::AppendVISALscTypedLoad(
    LSC_OP op, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
    VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, LSC_ADDR_TYPE addrType,
    LSC_ADDR_SIZE addrSize, LSC_DATA_SHAPE data,
    VISA_VectorOpnd *surface, unsigned surfaceIndex,
    VISA_RawOpnd *dstData,
    VISA_RawOpnd *Us, int uOffset,
    VISA_RawOpnd *Vs, int vOffset,
    VISA_RawOpnd *Rs, int rOffset,
    VISA_RawOpnd *LODs) {
  return AppendVISALscTypedInst(op, pred, execSize, emask, cacheOpts, addrType,
                                addrSize, data, surface, surfaceIndex, dstData,
                                Us, uOffset, Vs, vOffset, Rs, rOffset,
                                LODs, nullptr, nullptr);
}

VISA_BUILDER_API int VISAKernelImpl::AppendVISALscTypedStore(
    LSC_OP op, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
    VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, LSC_ADDR_TYPE addrType,
    LSC_ADDR_SIZE addrSize, LSC_DATA_SHAPE data,
    VISA_VectorOpnd *surface, unsigned surfaceIndex,
    VISA_RawOpnd *Us, int uOffset,
    VISA_RawOpnd *Vs, int vOffset,
    VISA_RawOpnd *Rs, int rOffset,
    VISA_RawOpnd *LODs,
    VISA_RawOpnd *src1Data) {
  return AppendVISALscTypedInst(op, pred, execSize, emask, cacheOpts, addrType,
                                addrSize, data, surface, surfaceIndex, nullptr,
                                Us, uOffset, Vs, vOffset, Rs, rOffset,
                                LODs, src1Data, nullptr);
}

VISA_BUILDER_API int VISAKernelImpl::AppendVISALscTypedAtomic(
    LSC_OP op, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
    VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, LSC_ADDR_TYPE addrType,
    LSC_ADDR_SIZE addrSize, LSC_DATA_SHAPE data,
    VISA_VectorOpnd *surface, unsigned surfaceIndex,
    VISA_RawOpnd *dstReadBack,
    VISA_RawOpnd *Us, int uOffset,
    VISA_RawOpnd *Vs, int vOffset,
    VISA_RawOpnd *Rs, int rOffset,
    VISA_RawOpnd *LODs,
    VISA_RawOpnd *src1AtomicOpnd1, VISA_RawOpnd *src2AtomicOpnd2) {
  return AppendVISALscTypedInst(op, pred, execSize, emask, cacheOpts, addrType,
                                addrSize, data,
                                surface, surfaceIndex, dstReadBack,
                                Us, uOffset, Vs, vOffset, Rs, rOffset,
                                LODs, src1AtomicOpnd1, src2AtomicOpnd2);
}

VISA_BUILDER_API int VISAKernelImpl::AppendVISALscTypedInst(
    LSC_OP subOpcode, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
    VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts, LSC_ADDR_TYPE addrType,
    LSC_ADDR_SIZE addrSize, LSC_DATA_SHAPE dataShape,
    VISA_VectorOpnd *surface, unsigned surfaceIndex,
    VISA_RawOpnd *dstData,
    VISA_RawOpnd *coord0s, int coord0Offset,
    VISA_RawOpnd *coord1s, int coord1Offset,
    VISA_RawOpnd *coord2s, int coord2Offset,
    VISA_RawOpnd *features,
    VISA_RawOpnd *src1Data, VISA_RawOpnd *src2Data) {

  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (subOpcode == LSC_LOAD_QUAD_MSRT || subOpcode == LSC_STORE_QUAD_MSRT) {
    // (u, v, r, si)
    vISA_ASSERT_INPUT(
        coord0s != nullptr && coord1s != nullptr && coord2s != nullptr &&
          features != nullptr,
        "Incorrect parameters passed to lsc msrt instructions");
  }

  LSC_CHECK_NULL_VECTOR_OPERAND(surface);
  LSC_CHECK_NULL_DST(dstData);
  LSC_CHECK_NULL_SRC(coord0s);
  LSC_CHECK_NULL_SRC(coord1s);
  LSC_CHECK_NULL_SRC(coord2s);
  LSC_CHECK_NULL_SRC(features);
  LSC_CHECK_NULL_SRC(src1Data);
  LSC_CHECK_NULL_SRC(src2Data);

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawDstOperand(dstData);
    CreateGenRawSrcOperand(coord0s);
    CreateGenRawSrcOperand(coord1s);
    CreateGenRawSrcOperand(coord2s);
    CreateGenRawSrcOperand(features);
    CreateGenRawSrcOperand(src1Data);
    CreateGenRawSrcOperand(src2Data);

  if (m_options->getOption(vISA_enableEfficient64b)) {
    if (cacheOpts.l2 == LSC_CACHING_DEFAULT) {
      cacheOpts.l2 = cacheOpts.l3;
      if (cacheOpts.l3 != LSC_CACHING_DEFAULT) {
        cacheOpts.l3 = LSC_CACHING_UNCACHED;
      }
    }
    status = m_builder->translateLscTypedInstUnified(
        subOpcode, pred ? pred->g4opnd->asPredicate() : nullptr, execSize,
        emask, cacheOpts, addrType, addrSize, dataShape,
        surface->g4opnd, surfaceIndex,
        dstData->g4opnd->asDstRegRegion(),
        coord0s->g4opnd->asSrcRegRegion(), coord0Offset,
        coord1s->g4opnd->asSrcRegRegion(), coord1Offset,
        coord2s->g4opnd->asSrcRegRegion(), coord2Offset,
        features->g4opnd->asSrcRegRegion(),
        src1Data->g4opnd->asSrcRegRegion(), src2Data->g4opnd->asSrcRegRegion());
  } else {
    status = m_builder->translateLscTypedInst(
        subOpcode, pred ? pred->g4opnd->asPredicate() : nullptr, execSize,
        emask, cacheOpts, addrType, addrSize, dataShape,
        surface->g4opnd, surfaceIndex,
        dstData->g4opnd->asDstRegRegion(),
        coord0s->g4opnd->asSrcRegRegion(), coord0Offset,
        coord1s->g4opnd->asSrcRegRegion(), coord1Offset,
        coord2s->g4opnd->asSrcRegRegion(), coord2Offset,
        features->g4opnd->asSrcRegRegion(),
        src1Data->g4opnd->asSrcRegRegion(), src2Data->g4opnd->asSrcRegRegion());
    }
  }

  if (IS_VISA_BOTH_PATH) {
    const VISA_INST_Desc *instDesc = &CISA_INST_table[ISA_LSC_TYPED];
    VISA_opnd *opnds[MAX_OPNDS_PER_INST]{};
    int numOpnds = 0;

    // use same order for load, store, and atomic
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(subOpcode, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l1, ISA_TYPE_UB));
    if (m_options->getOption(vISA_enableEfficient64b))
      ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l2, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l3, ISA_TYPE_UB));
    //
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(addrType, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(addrSize, ISA_TYPE_UB));
    //
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape.size, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape.order, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape.elems, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape.chmask, ISA_TYPE_UB));
    //
    ADD_OPND(numOpnds, opnds, surface);
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(surfaceIndex, ISA_TYPE_D));
    //
    ADD_OPND(numOpnds, opnds, dstData);
    ADD_OPND(numOpnds, opnds, coord0s);
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(coord0Offset, ISA_TYPE_D));
    ADD_OPND(numOpnds, opnds, coord1s);
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(coord1Offset, ISA_TYPE_D));
    ADD_OPND(numOpnds, opnds, coord2s);
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(coord2Offset, ISA_TYPE_D));
    ADD_OPND(numOpnds, opnds, features);
    ADD_OPND(numOpnds, opnds, src1Data);
    ADD_OPND(numOpnds, opnds, src2Data);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);
    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();
    // Pack execution size & mask
    unsigned size = execSize;
    PACK_EXEC_SIZE(size, emask);

    inst->createCisaInstruction(ISA_LSC_TYPED, (uint8_t)size, 0, predOpnd,
                                opnds, numOpnds, instDesc, verifier);
    addInstructionToEnd(inst);
  }

  return status;
}


VISA_BUILDER_API int VISAKernelImpl::AppendVISALscTypedBlock2DInst(
    LSC_OP op, LSC_CACHE_OPTS cacheOpts, LSC_ADDR_TYPE addrModel,
    LSC_DATA_SHAPE_TYPED_BLOCK2D dataShape2D,
    VISA_VectorOpnd *surface, unsigned surfaceIndex,
    VISA_RawOpnd *dstData, VISA_VectorOpnd *blockStartX,
    VISA_VectorOpnd *blockStartY, int xImmOffset, int yImmOffset,
    VISA_RawOpnd *src1Data) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  LSC_CHECK_NULL_DST(dstData);
  LSC_CHECK_NULL_SRC(src1Data);

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawDstOperand(dstData);
    CreateGenRawSrcOperand(src1Data);

    status |= m_builder->translateLscTypedBlock2DInst(
        op, cacheOpts, addrModel, dataShape2D, surface->g4opnd, surfaceIndex,
        dstData->g4opnd->asDstRegRegion(), blockStartX->g4opnd,
        blockStartY->g4opnd, xImmOffset, yImmOffset,
        src1Data->g4opnd->asSrcRegRegion());
  }

  if (IS_VISA_BOTH_PATH) {
    const VISA_INST_Desc *instDesc = &CISA_INST_table[ISA_LSC_TYPED];
    VISA_opnd *opnds[MAX_OPNDS_PER_INST]{};
    int numOpnds = 0;

    // op
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(op, ISA_TYPE_UB));
    // cache
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l1, ISA_TYPE_UB));
    if (m_options->getOption(vISA_enableEfficient64b))
      ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l2, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l3, ISA_TYPE_UB));
    // addrMode
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(addrModel, ISA_TYPE_UB));
    // blockWdith/blockHeight
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape2D.width, ISA_TYPE_UW));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(dataShape2D.height, ISA_TYPE_UW));
    // surface
    ADD_OPND(numOpnds, opnds, surface);
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(surfaceIndex, ISA_TYPE_UD));
    // dst
    ADD_OPND(numOpnds, opnds, dstData);
    // X offset
    ADD_OPND(numOpnds, opnds, blockStartX);
    // xImm offset
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(xImmOffset, ISA_TYPE_D));

    // Y offset
    ADD_OPND(numOpnds, opnds, blockStartY);
    // yImm offset
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(yImmOffset, ISA_TYPE_D));

    // src1
    ADD_OPND(numOpnds, opnds, src1Data);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    status = inst->createCisaInstruction(ISA_LSC_TYPED, EXEC_SIZE_1, 0,
                                         PredicateOpnd::getNullPred(), opnds,
                                         numOpnds, instDesc, verifier);
    addInstructionToEnd(inst);
  }

  return status;
}

VISA_BUILDER_API int
VISAKernelImpl::AppendVISALscUntypedAppendCounterAtomicInst(
    LSC_OP op, VISA_PredOpnd *pred, VISA_Exec_Size execSize,
    VISA_EMask_Ctrl emask, LSC_CACHE_OPTS cacheOpts,
    LSC_ADDR_TYPE addrType, LSC_DATA_SHAPE data,
    VISA_VectorOpnd *surface, unsigned surfaceIndex,
    VISA_RawOpnd *dst, VISA_RawOpnd *srcData) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  vISA_ASSERT(!m_builder->isEfficient64bEnabled(),
      "Append counter atomic instructions under efficient64b should exercise another path");
  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  LSC_CHECK_NULL_VECTOR_OPERAND(surface);
  LSC_CHECK_NULL_DST(dst);
  LSC_CHECK_NULL_SRC(srcData);

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawDstOperand(dst);
    CreateGenRawSrcOperand(srcData);
    status = m_builder->translateLscUntypedAppendCounterAtomicInst(
        op, pred ? pred->g4opnd->asPredicate() : nullptr, execSize, emask,
        cacheOpts, addrType, data, surface->g4opnd, surfaceIndex,
        dst->g4opnd->asDstRegRegion(), srcData->g4opnd->asSrcRegRegion());
  }

  if (IS_VISA_BOTH_PATH) {
    const VISA_INST_Desc *instDesc = &CISA_INST_table[ISA_LSC_UNTYPED];
    VISA_opnd *opnds[MAX_OPNDS_PER_INST]{};
    int numOpnds = 0;

    // op
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(op, ISA_TYPE_UB));
    // sfid
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(LSC_UGM, ISA_TYPE_UB));
    // cache
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l1, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(cacheOpts.l3, ISA_TYPE_UB));
    // addr type
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(addrType, ISA_TYPE_UB));

    // data shape
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(data.size, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(data.order, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(data.elems, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(data.chmask, ISA_TYPE_UB));
    // surface
    ADD_OPND(numOpnds, opnds, surface);
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(surfaceIndex, ISA_TYPE_UD));
    // dst
    ADD_OPND(numOpnds, opnds, dst);
    // src0 address
    VISA_RawOpnd *src0Addr = nullptr;
    [[maybe_unused]] int nullOperandCreateStatus = CreateVISANullRawOperand(src0Addr, false);
    vISA_ASSERT(nullOperandCreateStatus == VISA_SUCCESS, "not able to create null operand");
    ADD_OPND(numOpnds, opnds, src0Addr);
    // src1 data
    ADD_OPND(numOpnds, opnds, srcData);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);
    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();
    // Pack execution size & mask
    unsigned size = execSize;
    PACK_EXEC_SIZE(size, emask);

    status =
        inst->createCisaInstruction(ISA_LSC_UNTYPED, (uint8_t)size, 0, predOpnd,
                                    opnds, numOpnds, instDesc, verifier);
    addInstructionToEnd(inst);
  }

  return status;
}

VISA_BUILDER_API int VISAKernelImpl::AppendVISALscFence(LSC_SFID lscSfid,
                                                        LSC_FENCE_OP fenceOp,
                                                        LSC_SCOPE scope) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (getOptions()->getOption(vISA_LSCFenceWA) && lscSfid == LSC_UGM &&
      scope > LSC_SCOPE_LOCAL && fenceOp == LSC_FENCE_OP_NONE)
    fenceOp = LSC_FENCE_OP_INVALIDATE;

  if (m_builder->needLSCFenceDiscardWA())
    vISA_ASSERT_INPUT(fenceOp != LSC_FENCE_OP_DISCARD,
                      "Fence Op Discard is prohibited on this platform.");

  if (IS_GEN_BOTH_PATH) {
    SFID sfid = LSC_SFID_To_SFID(lscSfid);
    m_builder->translateLscFence(nullptr, sfid, fenceOp, scope, status);
  }

  if (IS_VISA_BOTH_PATH) {
    const VISA_INST_Desc *instDesc = &CISA_INST_table[ISA_LSC_FENCE];
    VISA_opnd *opnds[MAX_OPNDS_PER_INST];
    memset(opnds, 0, sizeof(opnds));
    int numOpnds = 0;
    //
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(lscSfid, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(fenceOp, ISA_TYPE_UB));
    ADD_OPND(numOpnds, opnds, CreateOtherOpnd(scope, ISA_TYPE_UB));

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);
    // Pack execution size & mask
    unsigned size = EXEC_SIZE_1;
    PACK_EXEC_SIZE(size, vISA_EMASK_M1_NM);

    status = inst->createCisaInstruction(ISA_LSC_FENCE, (uint8_t)size, 0,
                                         PredicateOpnd::getNullPred(), opnds,
                                         numOpnds, instDesc, verifier);
    addInstructionToEnd(inst);
  }

  return status;
}

VISA_BUILDER_API int
VISAKernelImpl::AppendVISANamedBarrierWait(VISA_VectorOpnd *barrierId) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    status =
        m_builder->translateVISANamedBarrierWait(nullptr, barrierId->g4opnd);
  }

  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = &CISA_INST_table[ISA_NBARRIER];
    VISA_opnd *opnd[3];
    int num_operands = 0;

    uint8_t mode = 0; // wait
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(0, num_operands, inst_desc, mode));
    ADD_OPND(num_operands, opnd, barrierId);
    // pad with dummy operand as NBarrier expects three operands in binary form.
    // It'll be ignored
    ADD_OPND(num_operands, opnd, barrierId);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst->createCisaInstruction(ISA_NBARRIER, EXEC_SIZE_1, 0,
                                PredicateOpnd::getNullPred(), opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

VISA_BUILDER_API int
VISAKernelImpl::AppendVISANamedBarrierSignal(VISA_VectorOpnd *barrierId,
                                             VISA_VectorOpnd *barrierCount) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  VISA_VectorOpnd *barrierType;
  uint16_t value = 0;
  status = CreateVISAImmediate(barrierType, &value, ISA_TYPE_UW);
  if (status != VISA_SUCCESS)
    return status;

  if (IS_GEN_BOTH_PATH) {
    status = m_builder->translateVISANamedBarrierSignal(
        nullptr, barrierId->g4opnd, barrierType->g4opnd, barrierCount->g4opnd,
        barrierCount->g4opnd);
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = &CISA_INST_table[ISA_NBARRIER];
    VISA_opnd *opnd[5];
    int num_operands = 0;

    // signal 1: nbarrier.signal <id> <num_threads>
    uint8_t mode = 1;
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(0, num_operands, inst_desc, mode));
    ADD_OPND(num_operands, opnd, barrierId);
    ADD_OPND(num_operands, opnd, barrierType);
    ADD_OPND(num_operands, opnd, barrierCount);
    ADD_OPND(num_operands, opnd, barrierCount);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst->createCisaInstruction(ISA_NBARRIER, EXEC_SIZE_1, 0,
                                PredicateOpnd::getNullPred(), opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

VISA_BUILDER_API int VISAKernelImpl::AppendVISANamedBarrierSignal(
    VISA_VectorOpnd *barrierId, VISA_VectorOpnd *barrierType,
    VISA_VectorOpnd *numProducers, VISA_VectorOpnd *numConsumers) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    status = m_builder->translateVISANamedBarrierSignal(
        nullptr, barrierId->g4opnd, barrierType->g4opnd, numProducers->g4opnd,
        numConsumers->g4opnd);
  }
  if (IS_VISA_BOTH_PATH) {
    VISA_INST_Desc *inst_desc = &CISA_INST_table[ISA_NBARRIER];
    VISA_opnd *opnd[5];
    int num_operands = 0;

    // signal : nbarrier.signal <id> <type> <numProds> <numCons>
    uint8_t mode = 2;
    ADD_OPND(num_operands, opnd,
             CreateOtherOpndHelper(0, num_operands, inst_desc, mode));
    ADD_OPND(num_operands, opnd, barrierId);
    ADD_OPND(num_operands, opnd, barrierType);
    ADD_OPND(num_operands, opnd, numProducers);
    ADD_OPND(num_operands, opnd, numConsumers);

    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    inst->createCisaInstruction(ISA_NBARRIER, EXEC_SIZE_1, 0,
                                PredicateOpnd::getNullPred(), opnd,
                                num_operands, inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

uint32_t VISAKernelImpl::addStringPool(std::string str) {
  if (str.empty()) {
    return 0;
  }
  m_string_pool.emplace_back(std::move(str));
  return (uint32_t)(m_string_pool.size() - 1);
}

void VISAKernelImpl::addInstructionToEnd(CisaInst *inst) {
  m_instruction_list.push_back(inst);
  CISA_INST *cisaInst = inst->getCISAInst();
  cisaInst->id = getvIsaInstCount();

  if (IsAsmWriterMode()) {
    // Print instructions
    VISAKernel_format_provider fmt(this);
    m_CISABuilder->m_ssIsaAsm
        << fmt.printInstruction(inst->getCISAInst(), getOptions()) << "\n";
  }
}

int VISAKernelImpl::addLabel(label_info_t *lbl, char *label_name) {
  return m_label_count++;
}

int VISAKernelImpl::addFunctionDirective(char *func_name) { return 0; }

void VISAKernelImpl::setGenxDebugInfoBuffer(char *buffer, unsigned long size) {
  m_genx_debug_info_buffer = buffer;
  m_genx_debug_info_size = size;
}

/**
 *  finalizeAttributes() sets attributes based on options, etc.
 *     Also, updates GRF configuration based on kernel attribute.
 *     Returns true if no error is found, false otherwise.
 */
bool VISAKernelImpl::finalizeAttributes() {
  if (!m_kernelAttrs->isKernelAttrSet(Attributes::ATTR_Target)) {
    VISATarget target = m_options->getTarget();
    uint8_t val = (uint8_t)target;
    AddKernelAttribute("Target", sizeof(val), &val);
  }
  if (m_kernelAttrs->isKernelAttrSet(Attributes::ATTR_NumGRF)) {
    unsigned attrNumGRF =
        m_kernelAttrs->getInt32KernelAttr(Attributes::ATTR_NumGRF);
    if (m_options->isOptionSetByUser(vISA_TotalGRFNum)) {
      // Check if there is a mismatch between kernel attribute
      // and vISA option. If so, report the error.
      unsigned optionNumGRF = m_options->getuInt32Option(vISA_TotalGRFNum);
      // If kernel attribute NumGRF=0, auto GRF selection is requested
      bool autoGRFRequested = attrNumGRF == 0;
      if (autoGRFRequested && optionNumGRF) {
        m_builder->criticalMsgStream() << "vISA: Kernel attribute NumGRF=0 (auto "
                                        "GRF) contradicts option -TotalGRFNum="
                                     << optionNumGRF;
        return false;
      }
      if (attrNumGRF != optionNumGRF) {
        m_builder->criticalMsgStream()
            << "vISA: Kernel attribute NumGRF=" << attrNumGRF
            << " contradicts option -TotalGRFNum=" << optionNumGRF;
        return false;
      }
    }
    if (!m_kernel->updateKernelFromNumGRFAttr()) {
      m_builder->criticalMsgStream()
          << "vISA: wrong value for .kernel_attr NumGRF";
      return false;
    }
  }
  return true;
}

VISA_LabelOpnd *
VISAKernelImpl::getLabelOperandFromFunctionName(const std::string &name) {
  auto it = m_funcName_to_labelID_map.find(name);
  if (m_funcName_to_labelID_map.end() == it) {
    return nullptr;
  } else {
    return it->second;
  }
}

VISA_LabelOpnd *
VISAKernelImpl::getLabelOpndFromLabelName(const std::string &name) {
  auto it = m_label_name_to_index_map.find(name);
  if (m_label_name_to_index_map.end() == it) {
    return nullptr;
  } else {
    return it->second;
  }
}

bool VISAKernelImpl::setLabelOpndNameMap(const std::string &name,
                                         VISA_LabelOpnd *lbl,
                                         VISA_Label_Kind kind) {
  // TODO: Is it possible to merge the 2 maps? Or a function label and
  // a block label are allowed to have the same name?
  if (kind == LABEL_BLOCK || kind == LABEL_DIVERGENT_RESOURCE_LOOP) {
    auto Res = m_label_name_to_index_map.insert({name, lbl});
    return Res.second;
  } else {
    auto Res = m_funcName_to_labelID_map.insert({name, lbl});
    return Res.second;
  }
}

CISA_GEN_VAR *VISAKernelImpl::getDeclFromName(const std::string &name) {
  // First search in the unique var map
  auto it = m_UniqueNamedVarMap.find(name);
  if (it != m_UniqueNamedVarMap.end()) {
    return it->second;
  }

  // Search each scope level until var is found, starting from the back
  for (auto scope_it = m_GenNamedVarMap.rbegin();
       scope_it != m_GenNamedVarMap.rend(); scope_it++) {
    auto it = scope_it->find(name);
    if (it != scope_it->end()) {
      return it->second;
    }
  }
  return NULL;
}

bool VISAKernelImpl::declExistsInCurrentScope(const std::string &name) const {
  // newest scope back and guaranteed to exist since we start with at least
  // one scope
  const GenDeclNameToVarMap &currScope = m_GenNamedVarMap.back();
  bool inCurrScope = currScope.find(name) != currScope.end();
  //
  // we also must prohibit globally reserved variables that must be
  // unique in all scopes (e.g. V0 or V1)
  bool reservedVarible =
      m_UniqueNamedVarMap.find(name) != m_UniqueNamedVarMap.end();
  return inCurrScope || reservedVarible;
}

bool VISAKernelImpl::setNameIndexMap(const std::string &name,
                                     CISA_GEN_VAR *genDecl, bool unique) {
  vISA_ASSERT(!m_GenNamedVarMap.empty(), "decl map is empty!");
  if (!unique) {
    // make sure mapping doesn't already exist in the current scope
    if (m_GenNamedVarMap.back().find(name) != m_GenNamedVarMap.back().end())
      return false;

    // also cannot be redefinition of a unique var
    if (m_UniqueNamedVarMap.find(name) != m_UniqueNamedVarMap.end())
      return false;

    // Add var to the current scope
    m_GenNamedVarMap.back()[name] = genDecl;
  } else {
    // we cannot create a unique var that redefines any previously created var
    // in any scope
    if (getDeclFromName(name) != NULL)
      return false;

    // unique vars are stored in a separate map
    m_UniqueNamedVarMap[name] = genDecl;
  }
  return true;
}

void VISAKernelImpl::pushIndexMapScopeLevel() {
  m_GenNamedVarMap.push_back(GenDeclNameToVarMap());
}
void VISAKernelImpl::popIndexMapScopeLevel() {
  vISA_ASSERT(m_GenNamedVarMap.size() > 1, "Cannot pop base scope level!");
  m_GenNamedVarMap.pop_back();
}

VISAKernelImpl::~VISAKernelImpl() {
  std::list<CisaFramework::CisaInst *>::iterator iter =
      m_instruction_list.begin();
  for (; iter != m_instruction_list.end(); iter++) {
    CisaFramework::CisaInst *inst = *iter;
    inst->~CisaInst();
  }

  m_var_info_list.clear();
  m_input_info_list.clear();
  m_label_info_list.clear();
  m_addr_info_list.clear();

  if (IS_GEN_BOTH_PATH) {
    // need to call destructor even thought it is allocated in memory pool.
    // so that internal data structures get cleared.
    m_kernel->~G4_Kernel();
    m_builder->~IR_Builder();

    if (m_jitInfo != nullptr) {
      m_jitInfo->~FINALIZER_INFO();
      m_jitInfo = nullptr;
    }

    delete m_kernelMem;
  }

  if (m_kernelInfo != nullptr) {
    delete m_kernelInfo;
  }

  destroyKernelAttributes();

  delete fmt;
  delete verifier;
}

int VISAKernelImpl::GetGenxBinary(void *&buffer, int &size) const {
  buffer = m_genx_binary_buffer;
  size = m_genx_binary_size;
  return VISA_SUCCESS;
}

int VISAKernelImpl::GetRelocations(RelocListType &relocs) {
  G4_Kernel::RelocationTableTy &reloc_table = m_kernel->getRelocationTable();
  for (RelocationEntry &reloc : reloc_table) {
    G4_INST *inst = reloc.getInst();
    int64_t genOffset = inst->getGenOffset();
    uint32_t offset =
        static_cast<uint32_t>(genOffset + reloc.getTargetOffset(*m_builder));
    relocs.emplace_back(reloc.getType(), offset, reloc.getSymbolName());

    vASSERT((genOffset != UNDEFINED_GEN_OFFSET) && (offset >= genOffset) &&
            (offset < genOffset + BYTES_PER_INST));
  }
  return VISA_SUCCESS;
}

int VISAKernelImpl::GetGenxDebugInfo(void *&buffer, unsigned int &size) const {
  buffer = m_genx_debug_info_buffer;
  size = m_genx_debug_info_size;

  return VISA_SUCCESS;
}

int VISAKernelImpl::GetJitInfo(FINALIZER_INFO *&jitInfo) const {
  jitInfo = m_jitInfo;
  return VISA_SUCCESS;
}

int VISAKernelImpl::GetKernelInfo(KERNEL_INFO *&kernelInfo) const {
  kernelInfo = m_kernelInfo;
  return VISA_SUCCESS;
}

int VISAKernelImpl::GetErrorMessage(const char *&errorMsg) const {
  // do nothing, doesn't seem like this is actually implemented
  return VISA_SUCCESS;
}

int VISAKernelImpl::GetFunctionId(unsigned int &id) const {
  id = m_functionId;
  return VISA_SUCCESS;
}

int VISAKernelImpl::SetGTPinInit(void *buffer) {
  if (!m_kernel)
    return VISA_FAILURE;

  auto gtpin = m_kernel->getGTPinData();
  if (gtpin) {
    if (getOptions()->getOption(vISA_GetFreeGRFInfo) ||
        getOptions()->getuInt32Option(vISA_GTPinScratchAreaSize)) {
      // GTPin init set by L0 driver through flags
      gtpin->setGTPinInitFromL0(true);
    }
    if (buffer) {
      gtpin->setGTPinInit(buffer);
    }
  }

  return VISA_SUCCESS;
}

int VISAKernelImpl::GetGTPinBuffer(void *&buffer, unsigned int &size,
                                   unsigned int scratchOffset) {
  buffer = nullptr;
  size = 0;

  if (!m_kernel)
    return VISA_FAILURE;

  auto gtpin = m_kernel->getGTPinData();
  if (gtpin) {
    buffer = gtpin->getGTPinInfoBuffer(size, scratchOffset);
  }

  return VISA_SUCCESS;
}

int VISAKernelImpl::GetFreeGRFInfo(void *&buffer, unsigned int &size) {
  buffer = nullptr;
  size = 0;

  if (getOptions()->getOption(vISA_GetFreeGRFInfo)) {
    vASSERT(m_kernel);
    auto gtpin = m_kernel->getGTPinData();
    if (gtpin) {
      buffer = gtpin->getFreeGRFInfo(size);
    }
  }
  return VISA_SUCCESS;
}

int VISAKernelImpl::getKernelCostInfo(const KernelCostInfo *&KCInfo) const {
  if (getOptions()->getOption(vISA_KernelCostInfo)) {
    KCInfo = getKernel()->getKernelCostInfo();
  } else {
    KCInfo = nullptr;
  }
  return VISA_SUCCESS;
}

// index
VISA_opnd *VISAKernelImpl::CreateOtherOpnd(unsigned int value,
                                           VISA_Type opndType) {
  VISA_opnd *temp = getOpndFromPool();

  temp->_opnd.other_opnd = value;
  temp->opnd_type = CISA_OPND_OTHER;
  temp->size = (uint16_t)Get_VISA_Type_Size(opndType);
  return temp;
}

// FIXME: this needs major rework
VISA_opnd *VISAKernelImpl::CreateOtherOpndHelper(
    int num_pred_desc_operands, int num_operands, const VISA_INST_Desc *inst_desc,
    unsigned int value, bool hasSubOpcode, uint8_t subOpcode) {
  VISA_Type dataType = ISA_TYPE_NUM;
  VISA_opnd *temp = getOpndFromPool();

  if (!hasSubOpcode) {
    dataType =
        (VISA_Type)inst_desc->opnd_desc[num_pred_desc_operands + num_operands]
            .data_type;
  } else {
    // Accounts for all the operands added so far to instruction, minus
    // predefined ones: execSize, pred, op
    dataType = (VISA_Type)inst_desc->getSubInstDesc(subOpcode)
                   .opnd_desc[num_operands -
                              (inst_desc->opnd_num - num_pred_desc_operands)]
                   .data_type;
  }
  temp->_opnd.other_opnd = value;
  temp->opnd_type = CISA_OPND_OTHER;
  temp->size = Get_VISA_Type_Size(dataType);
  temp->tag =
      (uint8_t)inst_desc->opnd_desc[num_pred_desc_operands + num_operands]
          .opnd_type;

  return temp;
}

VISA_opnd *VISAKernelImpl::getOpndFromPool() {
  VISA_opnd *newOp = nullptr;
  if (IS_VISA_BOTH_PATH) {
    newOp = (VISA_opnd *)m_mem.alloc(sizeof(VISA_opnd));
  } else {
    newOp =
        &m_fastPathOpndPool[(m_opndCounter++) % vISA_NUMBER_OF_OPNDS_IN_POOL];
  }
  memset(newOp, 0, sizeof(*newOp));
  return newOp;
}

G4_Operand *VISAKernelImpl::CommonISABuildPreDefinedSrc(
    int index, uint16_t vStride, uint16_t width, uint16_t hStride,
    uint8_t rowOffset, uint8_t colOffset, VISA_Modifier modifier) {
  const RegionDesc *rd = m_builder->createRegionDesc(vStride, width, hStride);
  G4_Operand *tmpsrc = NULL;
  PreDefinedVarsInternal internalIndex = mapExternalToInternalPreDefVar(index);
  switch (internalIndex) {
  case PreDefinedVarsInternal::VAR_NULL:
  case PreDefinedVarsInternal::X:
  case PreDefinedVarsInternal::Y:
  case PreDefinedVarsInternal::LOCAL_ID_X:
  case PreDefinedVarsInternal::LOCAL_ID_Y:
  case PreDefinedVarsInternal::LOCAL_SIZE_X:
  case PreDefinedVarsInternal::LOCAL_SIZE_Y:
  case PreDefinedVarsInternal::GROUP_ID_X:
  case PreDefinedVarsInternal::GROUP_ID_Y:
  case PreDefinedVarsInternal::GROUP_ID_Z:
  case PreDefinedVarsInternal::GROUP_COUNT_X:
  case PreDefinedVarsInternal::GROUP_COUNT_Y:
  case PreDefinedVarsInternal::IMPL_ARG_BUF_PTR:
  case PreDefinedVarsInternal::LOCAL_ID_BUF_PTR: {
    G4_Type type = GetGenTypeFromVISAType(getPredefinedVarType(internalIndex));
    // R0 is already declared
    G4_Declare *pre_var_dcl = getGenVar(index)->genVar.dcl;
    tmpsrc =
        m_builder->createSrcRegRegion(GetGenSrcModFromVISAMod(modifier), Direct,
                                      pre_var_dcl->getRegVar(), 0, 0, rd, type);
    break;
  }
  case PreDefinedVarsInternal::TSC:
  case PreDefinedVarsInternal::R0:
  case PreDefinedVarsInternal::SR0:
  case PreDefinedVarsInternal::MSG0:
  case PreDefinedVarsInternal::CR0:
  case PreDefinedVarsInternal::CE0:
  case PreDefinedVarsInternal::ARG:
  case PreDefinedVarsInternal::RET:
  case PreDefinedVarsInternal::FE_SP:
  case PreDefinedVarsInternal::FE_FP:
  case PreDefinedVarsInternal::HW_TID:
  case PreDefinedVarsInternal::DBG:
  case PreDefinedVarsInternal::SCRATCHLOC:
  case PreDefinedVarsInternal::COLOR: {
    G4_Type type = GetGenTypeFromVISAType(getPredefinedVarType(internalIndex));
    G4_Declare *preVarDcl = getGenVar(index)->genVar.dcl;
    tmpsrc = m_builder->createSrcRegRegion(GetGenSrcModFromVISAMod(modifier),
                                           Direct, preVarDcl->getRegVar(),
                                           rowOffset, colOffset, rd, type);
    break;
  }
  default:
    vISA_ASSERT_UNREACHABLE("unsupported pre-defined variable");
  }

  m_builder->preDefVars.setHasPredefined(internalIndex, true);
  return tmpsrc;
}

void VISAKernelImpl::setName(const char *n) {
  if (n[0] == '\0')
    return;
  m_cisa_kernel.name_index = addStringPool(n);
  m_name = m_string_pool[m_cisa_kernel.name_index];
  if (m_kernel) {
    m_kernel->setName(m_name.c_str());
  }
}

void VISAKernelImpl::setInputSize(uint8_t size) {
  m_cisa_kernel.input_size = size;
}

void VISAKernelImpl::setReturnSize(unsigned int size) {
  m_cisa_kernel.return_value_size = (uint8_t)size;
}

int VISAKernelImpl::SetFunctionInputSize(unsigned int size) {
  setInputSize((uint8_t)size);
  return VISA_SUCCESS;
}

int VISAKernelImpl::SetFunctionReturnSize(unsigned int size) {
  setReturnSize(size);
  return VISA_SUCCESS;
}

// common tasks for AppendVISAInst* functions
// currently we just increment the VISA offset to make sure it is set correctly
// for each GEN instruction
void VISAKernelImpl::AppendVISAInstCommon() {
  m_vISAInstCount++;
  if (IS_GEN_BOTH_PATH) {
    m_builder->curCISAOffset = getvIsaInstCount();
  }
}

int VISAKernelImpl::getVISAOffset() const {
  if (IS_GEN_BOTH_PATH) {
    return m_builder->curCISAOffset;
  }
  // TODO: we should probably move vISA offset in VISKernelImpl so that we
  // have it in vISA path too
  return -1;
}

void VISAKernelImpl::computeFCInfo(BinaryEncodingBase *binEncodingInstance) {
  // This function iterates over all instructions in kernel and sets up data
  // structures used to emit FC patch file later on after compilation for
  // all kernels is complete.
  // This function should be invoked only after binary encoding pass.
  std::map<BinInst *, std::pair<G4_INST *, bool>> FCInstMap;
  G4_Kernel *kernel = getKernel();
  IR_Builder *builder = getIRBuilder();

  if (isFCCallerKernel() == false && isFCCallableKernel() == false) {
    // No need to do anything since the kernel doesnt
    // have any FC calls.
    return;
  }

  // First populate FCInstMap map that holds all pseudo_fc_call/ret
  // instructions in the kernel.
  BB_LIST_ITER bb_end = kernel->fg.end();
  for (BB_LIST_ITER bb_it = kernel->fg.begin(); bb_it != bb_end; bb_it++) {
    G4_BB *bb = (*bb_it);

    INST_LIST_ITER inst_end = bb->end();
    for (INST_LIST_ITER inst_it = bb->begin(); inst_it != inst_end; inst_it++) {
      G4_INST *inst = (*inst_it);
      G4_opcode opc = inst->opcode();

      if (opc == G4_pseudo_fc_call)
        FCInstMap.emplace(binEncodingInstance->getBinInst(inst),
                          std::make_pair(inst, true));
      else if (opc == G4_pseudo_fc_ret)
        FCInstMap.emplace(binEncodingInstance->getBinInst(inst),
                          std::make_pair(inst, false));
    }
  }

  BinInstList &binInstList = binEncodingInstance->getBinInstList();
  for (unsigned i = 0, size = (unsigned)binInstList.size(); i < size; i++) {
    BinInst *bin = binInstList[i];

    std::map<BinInst *, std::pair<G4_INST *, bool>>::iterator fcMapIt =
        FCInstMap.find(bin);

    if (fcMapIt != FCInstMap.end()) {
      if (fcMapIt->second.second == true) {
        // pseudo_fc_call
        FCCalls *callToPatch = (FCCalls *)builder->mem.alloc(sizeof(FCCalls));
        callToPatch->callOffset = i;
        std::string_view labelStr(
            static_cast<G4_Label *>(fcMapIt->second.first->getSrc(0))
                ->getLabelName());
        unsigned int strLength = (uint32_t)labelStr.size();
        char *labelString = (char *)builder->mem.alloc(strLength + 1);
        strcpy_s(labelString, strLength + 1, labelStr.data());
        callToPatch->calleeLabelString = labelString;

        builder->getFCPatchInfo()->getFCCallsToPatch().push_back(callToPatch);
      } else {
        // pseduo_fc_ret
        builder->getFCPatchInfo()->getFCReturnsToPatch().push_back(i);
      }
    }
  }
}

void VISAKernelImpl::computeFCInfo() {
  // This function iterates over all instructions in kernel and sets up data
  // structures used to emit FC patch file later on after compilation for
  // all kernels is complete.
  // This function should be invoked only after binary encoding pass.
  G4_Kernel *kernel = getKernel();
  IR_Builder *builder = getIRBuilder();

  if (isFCCallerKernel() == false && isFCCallableKernel() == false) {
    // No need to do anything since the kernel doesnt
    // have any FC calls.
    return;
  }

  // First populate FCInstMap map that holds all pseudo_fc_call/ret
  // instructions in the kernel.
  BB_LIST_ITER bb_end = kernel->fg.end();
  for (BB_LIST_ITER bb_it = kernel->fg.begin(); bb_it != bb_end; bb_it++) {
    G4_BB *bb = (*bb_it);

    INST_LIST_ITER inst_end = bb->end();
    for (INST_LIST_ITER inst_it = bb->begin(); inst_it != inst_end; inst_it++) {
      G4_INST *inst = (*inst_it);
      G4_opcode opc = inst->opcode();

      if (opc == G4_pseudo_fc_call) {
        // TODO: Need to create FCCalls and push into calls-to-patch.
        vISA_ASSERT(inst->getGenOffset() % 16 == 0,
                    "Non-128-bit instruction is found!");
        unsigned Slot = unsigned(inst->getGenOffset()) / 16;
        FCCalls *CallToPatch = (FCCalls *)builder->mem.alloc(sizeof(FCCalls));
        CallToPatch->callOffset = Slot;
        G4_Label *Label = inst->getSrc(0)->asLabel();
        size_t Len = std::string_view(Label->getLabelName()).size();
        char *S = (char *)builder->mem.alloc(Len + 1);
        strcpy_s(S, Len + 1, Label->getLabelName());
        CallToPatch->calleeLabelString = S;
        builder->getFCPatchInfo()->getFCCallsToPatch().push_back(CallToPatch);
      } else if (opc == G4_pseudo_fc_ret) {
        vISA_ASSERT(inst->getGenOffset() % 16 == 0,
                    "Non-128-bit instruction is found!");
        unsigned Slot = unsigned(inst->getGenOffset()) / 16;
        builder->getFCPatchInfo()->getFCReturnsToPatch().push_back(Slot);
      }
    }
  }
}

/// Gets declaration id GenVar
int VISAKernelImpl::getDeclarationID(VISA_GenVar *decl) const {
  return decl->index;
}

/// Gets declaration id VISA_AddrVar
int VISAKernelImpl::getDeclarationID(VISA_AddrVar *decl) const {
  return decl->index;
}

/// Gets declaration id VISA_PredVar
int VISAKernelImpl::getDeclarationID(VISA_PredVar *decl) const {
  return decl->index;
}

/// Gets declaration id VISA_SamplerVar
int VISAKernelImpl::getDeclarationID(VISA_SamplerVar *decl) const {
  return decl->index;
}

/// Gets declaration id VISA_SurfaceVar
int VISAKernelImpl::getDeclarationID(VISA_SurfaceVar *decl) const {
  return decl->index;
}

int64_t VISAKernelImpl::getGenOffset() const {
  vASSERT(false == m_kernel->fg.empty());
  int64_t entryPointOffset = UNDEFINED_GEN_OFFSET;

  // the offset of the first gen inst in this kernel/function
  for (auto *BB : m_kernel->fg) {
    for (auto *inst : BB->getInstList()) {
      if (inst->isLabel())
        continue;
      entryPointOffset = inst->getGenOffset();
      break;
    }
    if (entryPointOffset != UNDEFINED_GEN_OFFSET)
      break;
  }
  vASSERT(UNDEFINED_GEN_OFFSET != entryPointOffset);

  return entryPointOffset;
}

int64_t VISAKernelImpl::getGenSize() const {
  vASSERT(false == m_kernel->fg.empty());
  auto &lastBB = *(*m_kernel->fg.rbegin());

  // the offset of the last gen inst in this kernel/function
  vASSERT(false == lastBB.empty());
  auto inst = lastBB.rbegin();
  vASSERT(UNDEFINED_GEN_OFFSET !=
         (*inst)->getGenOffset()); // expecting terminator

  auto size = (*inst)->getGenOffset();
  size += (*inst)->isCompactedInst() ? (BYTES_PER_INST / 2) : BYTES_PER_INST;
  size -= getGenOffset();
  return size;
}

unsigned VISAKernelImpl::getNumRegTotal() const {
  return m_kernel->getNumRegTotal();
}

const char *VISAKernelImpl::getFunctionName() const {
  return m_kernel->getName();
}

static const VISAKernelImpl *getFmtKernelForISADump(
    const VISAKernelImpl *kernel,
    const CISA_IR_Builder &cisaBuilder) {
  // Assuming there's no too many payload kernels. Use the logic in
  // isaDump to select the kernel for format provider.
  if (!kernel->getIsPayload())
    return kernel;

  vASSERT(cisaBuilder.kernel_begin() != cisaBuilder.kernel_end());
  VISAKernelImpl *fmtKernel = *cisaBuilder.kernel_begin();
  for (auto it = cisaBuilder.kernel_begin();
         it != cisaBuilder.kernel_end(); ++it) {
    if (*it == kernel)
      break;
    if ((*it)->getIsKernel())
      fmtKernel = *it;
  }
  return fmtKernel;
}

std::string VISAKernelImpl::getVISAAsm() const {
  // Return an empty string if the builder option is GEN only.
  if (IS_GEN_PATH)
    return std::string();

  const VISAKernelImpl *fmtKernel = getFmtKernelForISADump(
          this, *m_CISABuilder);
  return m_CISABuilder->isaDump(this, fmtKernel);
}

int VISAKernelImpl::encodeBlockFrequency(uint64_t digits, int16_t scale) {
  m_builder->getFreqInfoManager().transferFreqToG4Inst(digits, scale);
  return VISA_SUCCESS;
}

void VISAKernelImpl::computeAndEmitDebugInfo(KernelListTy &functions) {
  KernelListTy compilationUnitsForDebugInfo;
  compilationUnitsForDebugInfo.push_back(this);
  auto funcEndIt = functions.end();
  for (auto funcIt = functions.begin(); funcIt != funcEndIt; funcIt++) {
    compilationUnitsForDebugInfo.push_back((*funcIt));
  }

  std::list<G4_BB *> stackCallEntryBBs;
  auto cunitsEnd = compilationUnitsForDebugInfo.end();
  for (auto cunitsIt = compilationUnitsForDebugInfo.begin();
       cunitsIt != cunitsEnd; cunitsIt++) {
    VISAKernelImpl *cunit = (*cunitsIt);
    if (cunit->getIsKernel() == false) {
      getKernel()->getKernelDebugInfo()->markStackCallFuncDcls(
          *cunit->m_kernel);
      stackCallEntryBBs.push_back(cunit->getKernel()->fg.getEntryBB());
    }
  }

  for (auto cunitsIt = compilationUnitsForDebugInfo.begin();
       cunitsIt != cunitsEnd; cunitsIt++) {
    G4_Kernel &curKernel = *(*cunitsIt)->getKernel();
    curKernel.getKernelDebugInfo()->computeDebugInfo(stackCallEntryBBs);
  }

#ifndef DLL_MODE
  if (getOptions()->getOption(vISA_outputToFile)) {
    std::string asmNameStr = getOutputAsmPath();
    std::string debugFileNameStr = asmNameStr + ".dbg";
    emitDebugInfo(this, functions, debugFileNameStr);
  }
#else
  void *ptr;
  unsigned size;
  emitDebugInfoToMem(this, functions, ptr, size);
  setGenxDebugInfoBuffer((char *)ptr, size);
#endif
}

// buf contains instance of gtpin_init_t
bool enableSrcLine(void *buf) {
  if (!buf)
    return false;

  auto gtpin_init_data = (gtpin::igc::igc_init_t *)buf;
  return gtpin_init_data->srcline_mapping != 0;
}
int VISAKernelImpl::AppendVISAShflIdx4Inst(
    ISA_Opcode opcode, VISA_PredOpnd *pred, VISA_EMask_Ctrl emask,
    VISA_Exec_Size executionSize, VISA_RawOpnd *dst, VISA_VectorOpnd *src0,
    VISA_VectorOpnd *src1) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawDstOperand(dst);
    G4_Predicate *g4Pred = (pred != nullptr) ? pred->g4opnd->asPredicate() : nullptr;
    status = m_builder->translateVISAShflIdx4Inst(
        g4Pred, executionSize,
        emask, dst->g4opnd->asDstRegRegion(), src0->g4opnd->asSrcRegRegion(),
        src1->g4opnd->asSrcRegRegion());
  }
  if (IS_VISA_BOTH_PATH) {
    // Accounting for exec_size and pred_id in descriptor
    int num_pred_desc_operands = 0;
    int num_operands = 0;
    VISA_INST_Desc *inst_desc = nullptr;
    VISA_opnd *opnd[3];

    inst_desc = &CISA_INST_table[opcode];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);

    ADD_OPND(num_operands, opnd, dst);
    ADD_OPND(num_operands, opnd, src0);
    ADD_OPND(num_operands, opnd, src1);
    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();
    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += emask << 4;
    inst->createCisaInstruction(opcode, size, 0, predOpnd, opnd, num_operands,
                                inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISALfsrInst(VISA_PredOpnd *pred,
                                       VISA_EMask_Ctrl emask,
                                       VISA_Exec_Size executionSize,
                                       LFSR_FC funcCtrl, VISA_VectorOpnd *dst,
                                       VISA_VectorOpnd *src0,
                                       VISA_VectorOpnd *src1) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    G4_Predicate *g4Pred = pred ? pred->g4opnd->asPredicate() : nullptr;
    status = m_builder->translateVISALfsrInst(
        g4Pred, executionSize, emask, funcCtrl, dst->g4opnd->asDstRegRegion(),
        src0->g4opnd, src1->g4opnd);
  }
  if (IS_VISA_BOTH_PATH) {
    int num_pred_desc_operands = 0;
    int num_operands = 0;
    VISA_INST_Desc *inst_desc = nullptr;
    VISA_opnd *opnd[4];

    inst_desc = &CISA_INST_table[ISA_LFSR];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);

    ADD_OPND(num_operands, opnd,
             CreateOtherOpnd((unsigned int)funcCtrl, ISA_TYPE_UB));
    ADD_OPND(num_operands, opnd, dst);
    ADD_OPND(num_operands, opnd, src0);
    ADD_OPND(num_operands, opnd, src1);
    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();
    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += emask << 4;
    inst->createCisaInstruction(ISA_LFSR, size, 0, predOpnd, opnd, num_operands,
                                inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

int VISAKernelImpl::AppendVISADnsclInst(
    VISA_PredOpnd *pred, VISA_EMask_Ctrl emask, VISA_Exec_Size executionSize,
    DNSCL_CONVERT_TYPE type, DNSCL_MODE mode, DNSCL_RND_MODE rndMode,
    VISA_RawOpnd *dst, VISA_RawOpnd *src0, VISA_RawOpnd *src1,
    VISA_RawOpnd *src2) {
  TIME_SCOPE(VISA_BUILDER_APPEND_INST);

  AppendVISAInstCommon();

  int status = VISA_SUCCESS;

  if (IS_GEN_BOTH_PATH) {
    CreateGenRawDstOperand(dst);
    CreateGenRawSrcOperand(src0);
    CreateGenRawSrcOperand(src1);
    CreateGenRawSrcOperand(src2);

    G4_Predicate *g4Pred = pred ? pred->g4opnd->asPredicate() : nullptr;
    status = m_builder->translateVISADnsclInst(
        g4Pred, executionSize, emask, type, mode, rndMode,
        dst->g4opnd->asDstRegRegion(), src0->g4opnd->asSrcRegRegion(),
        src1->g4opnd->asSrcRegRegion(), src2->g4opnd->asSrcRegRegion());
  }
  if (IS_VISA_BOTH_PATH) {
    int num_pred_desc_operands = 0;
    int num_operands = 0;
    VISA_INST_Desc *inst_desc = nullptr;
    VISA_opnd *opnd[7];

    inst_desc = &CISA_INST_table[ISA_DNSCL];
    GET_NUM_PRED_DESC_OPNDS(num_pred_desc_operands, inst_desc);

    ADD_OPND(num_operands, opnd,
             CreateOtherOpnd((unsigned int)type, ISA_TYPE_UB));
    ADD_OPND(num_operands, opnd,
             CreateOtherOpnd((unsigned int)mode, ISA_TYPE_UB));
    ADD_OPND(num_operands, opnd,
             CreateOtherOpnd((unsigned int)rndMode, ISA_TYPE_UB));
    ADD_OPND(num_operands, opnd, dst);
    ADD_OPND(num_operands, opnd, src0);
    ADD_OPND(num_operands, opnd, src1);
    ADD_OPND(num_operands, opnd, src2);
    CHECK_NUM_OPNDS(inst_desc, num_operands, num_pred_desc_operands);

    PredicateOpnd predOpnd =
        pred ? pred->convertToPred() : PredicateOpnd::getNullPred();
    CisaFramework::CisaInst *inst = new (m_mem) CisaFramework::CisaInst(m_mem);

    unsigned char size = executionSize;
    size += emask << 4;
    inst->createCisaInstruction(ISA_DNSCL, size, 0, predOpnd, opnd, num_operands,
                                inst_desc);
    addInstructionToEnd(inst);
  }

  return status;
}

void VISAKernelImpl::setLocalSheduleable(bool value) {
  m_kernel->setLocalSheduleable(value);
}

void VISAKernelImpl::addFuncPerfStats(const PERF_STATS_VERBOSE& input) {
  vISA::PERF_STATS_VERBOSE &myStats =
      m_kernel->fg.builder->getJitInfo()->statsVerbose;

  myStats.numALUInst += input.numALUInst;
  myStats.accSubDef += input.accSubDef;
  myStats.accSubUse += input.accSubUse;
  myStats.accSubCandidateDef += input.accSubCandidateDef;
  myStats.accSubCandidateUse += input.accSubCandidateUse;

  myStats.syncInstCount += input.syncInstCount;
  myStats.tokenReuseCount += input.tokenReuseCount;
  myStats.singlePipeAtOneDistNum += input.singlePipeAtOneDistNum;
  myStats.allAtOneDistNum += input.allAtOneDistNum;
  myStats.AfterWriteTokenDepCount += input.AfterWriteTokenDepCount;
  myStats.AfterReadTokenDepCount += input.AfterReadTokenDepCount;

  // Note: these two profiling info are collected during assembly instruction
  // emission, which happened after stitching so doesn't need to sum them:
  // PERF_STATS_VERBOSE::BCNum and PERF_STATS_VERBOSE::numRMWs
}

void VISAKernelImpl::emitPerfStats(std::ostream & os) {
  PERF_STATS_VERBOSE &stats = m_kernel->fg.builder->getJitInfo()->statsVerbose;
  os << "\n\n";
  os << "//.numALUInst: " << stats.numALUInst << "\n";
  os << "//.accSubDef: " << stats.accSubDef << "\n";
  os << "//.accSubUse: " << stats.accSubUse << "\n";
  os << "//.accSubCandidateDef: " << stats.accSubCandidateDef << "\n";
  os << "//.accSubCandidateUse: " << stats.accSubCandidateUse << "\n";
  os << "//\n//\n";
  os << "//.singlePipeAtOneDistNum: "
     << stats.singlePipeAtOneDistNum << "\n";
  os << "//.allAtOneDistNum: " << stats.allAtOneDistNum << "\n";
  os << "//.syncInstCount: " << stats.syncInstCount << "\n";
  os << "//.tokenReuseCount: " << stats.tokenReuseCount << "\n";
  os << "//.AfterWriteTokenDepCount: "
     << stats.AfterWriteTokenDepCount << "\n";
  os << "//.AfterReadTokenDepCount: "
     << stats.AfterReadTokenDepCount << "\n";
}
