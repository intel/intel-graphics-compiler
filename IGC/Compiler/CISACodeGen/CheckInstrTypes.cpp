
/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/CheckInstrTypes.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenPublic.h"

#include "common/debug/Debug.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/Support/CommandLine.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

using namespace llvm;
using namespace IGC;
using namespace GenISAIntrinsic;

#define PASS_FLAG "CheckInstrTypes"
#define PASS_DESCRIPTION "Check individual type of instructions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(CheckInstrTypes, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(CheckInstrTypes, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

static cl::opt<bool> enableInstrTypesPrint(
    "enable-instrtypes-print", cl::init(false), cl::Hidden,
    cl::desc("Enable CheckInstrTypes pass debug print: output structure modified by the pass to debug ostream"));

static cl::opt<bool> afterOptsFlag("after-opts-flag", cl::init(false), cl::Hidden,
                                   cl::desc("Set AfterOpts flag value for default constructor (debug purposes)"));

static cl::opt<bool> metricsFlag("metrics-flag", cl::init(false), cl::Hidden,
                                 cl::desc("Set metrics flag value for default constructor (debug purposes)"));

static cl::opt<bool> InstrTypesOnRun("igc-update-instrtypes-on-run", cl::init(false), cl::Hidden,
                                     cl::desc("Update InstrTypes during runOnFunction (debug purposes)"));

char CheckInstrTypes::ID = 0;

CheckInstrTypes::CheckInstrTypes() : FunctionPass(ID), g_AfterOpts(afterOptsFlag), g_metrics(metricsFlag) {}

CheckInstrTypes::CheckInstrTypes(bool afterOpts, bool metrics)
    : FunctionPass(ID), g_AfterOpts(afterOpts), g_metrics(metrics) {
  initializeCheckInstrTypesPass(*PassRegistry::getPassRegistry());
}

void CheckInstrTypes::SetLoopFlags(Function &F) {
  LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

  if (!(LI->empty())) {
    // find how many instructions are used in loop
    for (auto it = LI->begin(); it != LI->end(); it++) {
      g_InstrTypes.numOfLoop++;
      Loop *L = (*it);
      for (uint i = 0; i < L->getNumBlocks(); i++) {
        g_InstrTypes.numLoopInsts += L->getBlocks()[i]->size();
      }
    }
  }
}

void CheckInstrTypes::updateContext() {
  // in case there's no function to run on
  if (!context)
    return;

  if (g_AfterOpts)
    context->m_instrTypesAfterOpts = g_InstrTypes;
  else
    context->m_instrTypes = g_InstrTypes;
}

bool CheckInstrTypes::runOnFunction(Function &F) {
  // despite CodeGenContextWrapper is a functional pass, context itself is the Module's entity
  // here we save it to use later in doFinalization
  context = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

  // check if module has debug info
  g_InstrTypes.hasDebugInfo = F.getParent()->getNamedMetadata("llvm.dbg.cu") != nullptr;
  g_InstrTypes.numBB = F.size();

  for (auto BI = F.begin(), BE = F.end(); BI != BE; BI++) {
    g_InstrTypes.numAllInsts += BI->size();
  }

  visit(F);
  SetLoopFlags(F);

  if (InstrTypesOnRun)
    updateContext();

  return false;
}

bool CheckInstrTypes::doFinalization(llvm::Module &M) {
  updateContext();

  if (enableInstrTypesPrint)
    print(IGC::Debug::ods());

  return false;
}

void CheckInstrTypes::print(llvm::raw_ostream &OS) const {
  OS << "\nCorrelatedValuePropagationEnable: " << g_InstrTypes.CorrelatedValuePropagationEnable;
  OS << "\nhasMultipleBB: " << g_InstrTypes.hasMultipleBB;
  OS << "\nhasCmp: " << g_InstrTypes.hasCmp;
  OS << "\nhasSwitch: " << g_InstrTypes.hasSwitch;
  OS << "\nhasPhi: " << g_InstrTypes.hasPhi;
  OS << "\nhasLoadStore: " << g_InstrTypes.hasLoadStore;
  OS << "\nhasIndirectCall: " << g_InstrTypes.hasIndirectCall;
  OS << "\nhasInlineAsm: " << g_InstrTypes.hasInlineAsm;
  OS << "\nhasInlineAsmPointerAccess: " << g_InstrTypes.hasInlineAsmPointerAccess;
  OS << "\nhasIndirectBranch: " << g_InstrTypes.hasIndirectBranch;
  OS << "\nhasFunctionAddressTaken: " << g_InstrTypes.hasFunctionAddressTaken;
  OS << "\nhasSel: " << g_InstrTypes.hasSel;
  OS << "\nhasPointer: " << g_InstrTypes.hasPointer;
  OS << "\nhasLocalLoadStore: " << g_InstrTypes.hasLocalLoadStore;
  OS << "\nhasGlobalLoad: " << g_InstrTypes.hasGlobalLoad;
  OS << "\nhasGlobalStore: " << g_InstrTypes.hasGlobalStore;
  OS << "\nhasStorageBufferLoad: " << g_InstrTypes.hasStorageBufferLoad;
  OS << "\nhasStorageBufferStore: " << g_InstrTypes.hasStorageBufferStore;
  OS << "\nhasSubroutines: " << g_InstrTypes.hasSubroutines;
  OS << "\nhasPrimitiveAlloca: " << g_InstrTypes.hasPrimitiveAlloca;
  OS << "\nhasNonPrimitiveAlloca: " << g_InstrTypes.hasNonPrimitiveAlloca;
  OS << "\nhasReadOnlyArray: " << g_InstrTypes.hasReadOnlyArray;
  OS << "\nhasBuiltin: " << g_InstrTypes.hasBuiltin;
  OS << "\nhasFRem: " << g_InstrTypes.hasFRem;
  OS << "\npsHasSideEffect: " << g_InstrTypes.psHasSideEffect;
  OS << "\nhasGenericAddressSpacePointers: " << g_InstrTypes.hasGenericAddressSpacePointers;
  OS << "\nhasDebugInfo: " << g_InstrTypes.hasDebugInfo;
  OS << "\nhasAtomics: " << g_InstrTypes.hasAtomics;
  OS << "\nhasLocalAtomics: " << g_InstrTypes.hasLocalAtomics;
  OS << "\nhasDiscard: " << g_InstrTypes.hasDiscard;
  OS << "\nhasTypedRead: " << g_InstrTypes.hasTypedRead;
  OS << "\nhasTypedwrite: " << g_InstrTypes.hasTypedwrite;
  OS << "\nmayHaveIndirectOperands: " << g_InstrTypes.mayHaveIndirectOperands;
  OS << "\nmayHaveIndirectResources: " << g_InstrTypes.mayHaveIndirectResources;
  OS << "\nhasUniformAssumptions: " << g_InstrTypes.hasUniformAssumptions;
  OS << "\nsampleCmpToDiscardOptimizationPossible: " << g_InstrTypes.sampleCmpToDiscardOptimizationPossible;
  OS << "\nhasRuntimeValueVector: " << g_InstrTypes.hasRuntimeValueVector;
  OS << "\nhasDynamicGenericLoadStore: " << g_InstrTypes.hasDynamicGenericLoadStore;
  OS << "\nhasUnmaskedRegion: " << g_InstrTypes.hasUnmaskedRegion;
  OS << "\nhasSLM: " << g_InstrTypes.hasSLM;
  OS << "\nnumCall: " << g_InstrTypes.numCall;
  OS << "\nnumBarrier: " << g_InstrTypes.numBarrier;
  OS << "\nnumLoadStore: " << g_InstrTypes.numLoadStore;
  OS << "\nnumWaveIntrinsics: " << g_InstrTypes.numWaveIntrinsics;
  OS << "\nnumAtomics: " << g_InstrTypes.numAtomics;
  OS << "\nnumTypedReadWrite: " << g_InstrTypes.numTypedReadWrite;
  OS << "\nnumAllInsts: " << g_InstrTypes.numAllInsts;
  OS << "\nsampleCmpToDiscardOptimizationSlot: " << g_InstrTypes.sampleCmpToDiscardOptimizationSlot;
  OS << "\nnumSample: " << g_InstrTypes.numSample;
  OS << "\nnumBB: " << g_InstrTypes.numBB;
  OS << "\nnumLoopInsts: " << g_InstrTypes.numLoopInsts;
  OS << "\nnumOfLoop: " << g_InstrTypes.numOfLoop;
  OS << "\nnumInsts: " << g_InstrTypes.numInsts;
  OS << "\nnumAllocaInsts: " << g_InstrTypes.numAllocaInsts;
  OS << "\nnumPsInputs: " << g_InstrTypes.numPsInputs;
  OS << "\nhasPullBary: " << g_InstrTypes.hasPullBary;
  OS << "\nnumGlobalInsts: " << g_InstrTypes.numGlobalInsts;
  OS << "\nnumLocalInsts: " << g_InstrTypes.numLocalInsts;
  OS << "\nnumSamplesVaryingResource: " << g_InstrTypes.numSamplesVaryingResource;
  OS << "\nnumUntyped: " << g_InstrTypes.numUntyped;
  OS << "\nnum1DAccesses: " << g_InstrTypes.num1DAccesses;
  OS << "\nnum2DAccesses: " << g_InstrTypes.num2DAccesses;
  OS << "\nnumSLMAccesses: " << g_InstrTypes.numSLMAccesses;
  OS << "\nnumSLMStores: " << g_InstrTypes.numSLMStores;
  OS << "\nnumSLMLoads: " << g_InstrTypes.numSLMLoads << "\n\n";
}

void CheckInstrTypes::checkGlobalLocal(llvm::Instruction &I) {
  BasicBlock *dBB = I.getParent();

  for (auto U : I.users()) {
    auto UI = cast<Instruction>(U);
    BasicBlock *uBB = UI->getParent();
    if (uBB != dBB) {
      g_InstrTypes.numGlobalInsts++;
      return;
    }
  }
  g_InstrTypes.numLocalInsts++;
}

void CheckInstrTypes::visitInstruction(llvm::Instruction &I) {
  if (!llvm::isa<llvm::DbgInfoIntrinsic>(&I)) {
    g_InstrTypes.numInsts++;
    checkGlobalLocal(I);
  }

  if (I.getOpcode() == Instruction::FRem) {
    g_InstrTypes.hasFRem = true;
  }

  auto PT = dyn_cast<PointerType>(I.getType());
  if (PT && PT->getPointerAddressSpace() == ADDRESS_SPACE_GENERIC) {
    g_InstrTypes.hasGenericAddressSpacePointers = true;
  }
}

void CheckInstrTypes::visitCallInst(CallInst &C) {
  g_InstrTypes.numInsts++;
  checkGlobalLocal(C);
  g_InstrTypes.numCall++;

  Function *calledFunc = C.getCalledFunction();

  if (calledFunc == NULL) {
    if (C.isInlineAsm()) {
      g_InstrTypes.hasInlineAsm = true;
      for (unsigned i = 0; i < IGCLLVM::getNumArgOperands(&C); i++) {
        Type *opndTy = C.getArgOperand(i)->getType();
        if (opndTy->isPointerTy() && (cast<PointerType>(opndTy)->getAddressSpace() == ADDRESS_SPACE_GLOBAL ||
                                      cast<PointerType>(opndTy)->getAddressSpace() == ADDRESS_SPACE_CONSTANT)) {
          // If an inline asm call directly accesses a pointer, we need to enable
          // bindless/stateless support since user does not know the BTI the
          // resource is bound to.
          g_InstrTypes.hasInlineAsmPointerAccess = true;
        }
      }
      return;
    }
    // calls to 'blocks' have a null Function object
    g_InstrTypes.hasSubroutines = true;
    g_InstrTypes.hasIndirectCall = true;
  } else if (!calledFunc->isDeclaration()) {
    g_InstrTypes.hasSubroutines = true;
  }
  if (C.mayWriteToMemory()) {
    if (GenIntrinsicInst *CI = dyn_cast<GenIntrinsicInst>(&C)) {
      GenISAIntrinsic::ID IID = CI->getIntrinsicID();
      if (IID != GetOutputPSIntrinsic(context->platform) && IID != GenISA_discard) {
        g_InstrTypes.psHasSideEffect = true;
      }
    }
  }

  if (isSampleLoadGather4InfoInstruction(&C)) {
    g_InstrTypes.numSample++;
  }

  if (GenIntrinsicInst *CI = llvm::dyn_cast<GenIntrinsicInst>(&C)) {
    switch (CI->getIntrinsicID()) {
    case GenISAIntrinsic::GenISA_atomiccounterinc:
    case GenISAIntrinsic::GenISA_atomiccounterpredec:
    case GenISAIntrinsic::GenISA_icmpxchgatomicraw:
    case GenISAIntrinsic::GenISA_icmpxchgatomicrawA64:
    case GenISAIntrinsic::GenISA_cmpxchgatomicstructured:
    case GenISAIntrinsic::GenISA_icmpxchgatomictyped:
    case GenISAIntrinsic::GenISA_intatomicraw:
    case GenISAIntrinsic::GenISA_intatomicrawA64:
    case GenISAIntrinsic::GenISA_dwordatomicstructured:
    case GenISAIntrinsic::GenISA_intatomictyped:
    case GenISAIntrinsic::GenISA_fcmpxchgatomicraw:
    case GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64:
    case GenISAIntrinsic::GenISA_fcmpxchgatomicstructured:
    case GenISAIntrinsic::GenISA_floatatomicraw:
    case GenISAIntrinsic::GenISA_floatatomicrawA64:
    case GenISAIntrinsic::GenISA_floatatomicstructured:
    case GenISAIntrinsic::GenISA_floatatomictyped:
    case GenISAIntrinsic::GenISA_fcmpxchgatomictyped:
    case GenISAIntrinsic::GenISA_LSCAtomicFP64:
    case GenISAIntrinsic::GenISA_LSCAtomicFP32:
    case GenISAIntrinsic::GenISA_LSCAtomicBF16:
    case GenISAIntrinsic::GenISA_LSCAtomicInts: {
      g_InstrTypes.hasAtomics = true;
      g_InstrTypes.numAtomics++;
      Value *bufPtr = GetBufferOperand(CI);
      if (bufPtr && bufPtr->getType()->isPointerTy() &&
          ADDRESS_SPACE_LOCAL == bufPtr->getType()->getPointerAddressSpace()) {
        g_InstrTypes.hasLocalAtomics = true;
      }
      break;
    }
    case GenISAIntrinsic::GenISA_discard:
      g_InstrTypes.hasDiscard = true;
      break;
    case GenISAIntrinsic::GenISA_WaveShuffleIndex:
    case GenISAIntrinsic::GenISA_WaveBroadcast:
      g_InstrTypes.mayHaveIndirectOperands = true;
      g_InstrTypes.numWaveIntrinsics++;
      break;
    case GenISAIntrinsic::GenISA_threadgroupbarrier:
      g_InstrTypes.hasWorkgroupBarrier = true;
      g_InstrTypes.numBarrier++;
      break;
    case GenISAIntrinsic::GenISA_threadgroupbarrier_signal:
      g_InstrTypes.hasSplitBarrier = true;
      break;
    case GenISAIntrinsic::GenISA_is_uniform:
      g_InstrTypes.hasUniformAssumptions = true;
      break;
    case GenISAIntrinsic::GenISA_typedread:
    case GenISAIntrinsic::GenISA_typedreadMS:
      g_InstrTypes.hasTypedRead = true;
      g_InstrTypes.numTypedReadWrite++;
      g_InstrTypes.num2DAccesses++;
      break;
    case GenISAIntrinsic::GenISA_typedwrite:
    case GenISAIntrinsic::GenISA_typedwriteMS:
      g_InstrTypes.hasTypedwrite = true;
      g_InstrTypes.numTypedReadWrite++;
      g_InstrTypes.num2DAccesses++;
      break;
    case GenISAIntrinsic::GenISA_WaveAll:
    case GenISAIntrinsic::GenISA_WaveBallot:
    case GenISAIntrinsic::GenISA_wavebarrier:
    case GenISAIntrinsic::GenISA_WaveInverseBallot:
    case GenISAIntrinsic::GenISA_WaveClusteredBallot:
    case GenISAIntrinsic::GenISA_WavePrefix:
    case GenISAIntrinsic::GenISA_WaveClusteredPrefix:
    case GenISAIntrinsic::GenISA_WaveClustered:
    case GenISAIntrinsic::GenISA_WaveInterleave:
    case GenISAIntrinsic::GenISA_WaveClusteredInterleave:
    case GenISAIntrinsic::GenISA_QuadPrefix:
    case GenISAIntrinsic::GenISA_simdShuffleDown:
    case GenISAIntrinsic::GenISA_simdShuffleXor:
      g_InstrTypes.numWaveIntrinsics++;
      break;
    case GenISAIntrinsic::GenISA_DCL_inputVec:
    case GenISAIntrinsic::GenISA_DCL_ShaderInputVec:
      g_InstrTypes.numPsInputs++;
      break;
    case GenISAIntrinsic::GenISA_PullSampleIndexBarys:
    case GenISAIntrinsic::GenISA_PullSnappedBarys:
    case GenISAIntrinsic::GenISA_PullCentroidBarys:
      g_InstrTypes.hasPullBary = true;
      break;
    case GenISAIntrinsic::GenISA_ldraw_indexed:
    case GenISAIntrinsic::GenISA_ldrawvector_indexed: {
      g_InstrTypes.num1DAccesses++;
      BufferType bufferType = DecodeBufferType(CI->getArgOperand(0)->getType()->getPointerAddressSpace());
      if (bufferType == UAV || bufferType == BINDLESS || bufferType == BINDLESS_READONLY) {
        g_InstrTypes.hasStorageBufferLoad = true;
      }
      break;
    }
    case GenISAIntrinsic::GenISA_storeraw_indexed:
    case GenISAIntrinsic::GenISA_storerawvector_indexed: {
      g_InstrTypes.num1DAccesses++;
      BufferType bufferType = DecodeBufferType(CI->getArgOperand(0)->getType()->getPointerAddressSpace());
      if (bufferType == UAV || bufferType == BINDLESS || bufferType == BINDLESS_WRITEONLY) {
        g_InstrTypes.hasStorageBufferStore = true;
      }
      break;
    }
    case GenISAIntrinsic::GenISA_RuntimeValue: {
      if (CI->getType()->isVectorTy()) {
        g_InstrTypes.hasRuntimeValueVector = true;
      }
      break;
    }
    case GenISAIntrinsic::GenISA_storestructured1:
    case GenISAIntrinsic::GenISA_storestructured2:
    case GenISAIntrinsic::GenISA_storestructured3:
    case GenISAIntrinsic::GenISA_storestructured4:
      g_InstrTypes.numUntyped++;
      break;
    case GenISAIntrinsic::GenISA_ldstructured:
      g_InstrTypes.numUntyped++;
      g_InstrTypes.num1DAccesses++;
      break;
    case GenISAIntrinsic::GenISA_ldptr:
    case GenISAIntrinsic::GenISA_ldlptr:
      if (llvm::ConstantInt *pInt = llvm::dyn_cast<llvm::ConstantInt>(CI->getOperand(1))) {
        int index = int_cast<int>(pInt->getSExtValue());
        index == 0 ? g_InstrTypes.num1DAccesses++ : g_InstrTypes.num2DAccesses++;
      } else {
        g_InstrTypes.num2DAccesses++;
      }
      break;
    case GenISAIntrinsic::GenISA_sampleptr:
    case GenISAIntrinsic::GenISA_sampleBptr:
    case GenISAIntrinsic::GenISA_sampleCptr:
    case GenISAIntrinsic::GenISA_sampleDptr:
    case GenISAIntrinsic::GenISA_sampleDCptr:
    case GenISAIntrinsic::GenISA_sampleLptr:
    case GenISAIntrinsic::GenISA_sampleLCptr:
    case GenISAIntrinsic::GenISA_sampleBCptr:

    case GenISAIntrinsic::GenISA_gather4ptr:
    case GenISAIntrinsic::GenISA_gather4Cptr:
    case GenISAIntrinsic::GenISA_gather4POptr:
    case GenISAIntrinsic::GenISA_gather4POCptr:
    case GenISAIntrinsic::GenISA_gather4Iptr:
    case GenISAIntrinsic::GenISA_gather4IPOptr:
    case GenISAIntrinsic::GenISA_gather4Bptr:
    case GenISAIntrinsic::GenISA_gather4BPOptr:
    case GenISAIntrinsic::GenISA_gather4Lptr:
    case GenISAIntrinsic::GenISA_gather4LPOptr:
    case GenISAIntrinsic::GenISA_gather4ICptr:
    case GenISAIntrinsic::GenISA_gather4ICPOptr:
    case GenISAIntrinsic::GenISA_gather4LCptr:
    case GenISAIntrinsic::GenISA_gather4LCPOptr:
      if (llvm::ConstantInt *pInt = llvm::dyn_cast<llvm::ConstantInt>(CI->getOperand(2))) {
        int index = int_cast<int>(pInt->getZExtValue());
        index == 0 ? g_InstrTypes.num1DAccesses++ : g_InstrTypes.num2DAccesses++;
      } else {
        g_InstrTypes.num2DAccesses++;
      }
      break;
    default:
      break;
    }

    Value *resourcePtr = GetBufferOperand(CI);
    if (resourcePtr == nullptr) {
      Value *samplerPtr = nullptr;
      getTextureAndSamplerOperands(CI, resourcePtr, samplerPtr);
    }
    if (resourcePtr && resourcePtr->getType()->isPointerTy() &&
        isStatefulAddrSpace(resourcePtr->getType()->getPointerAddressSpace()) &&
        !IsDirectIdx(resourcePtr->getType()->getPointerAddressSpace())) {
      g_InstrTypes.mayHaveIndirectResources = true;
    }
  }
}

void CheckInstrTypes::visitBranchInst(BranchInst &I) {
  g_InstrTypes.numInsts++;
  checkGlobalLocal(I);
}

void CheckInstrTypes::visitSwitchInst(SwitchInst &I) {
  g_InstrTypes.numInsts++;
  checkGlobalLocal(I);
  g_InstrTypes.hasSwitch = true;
}

void CheckInstrTypes::visitIndirectBrInst(IndirectBrInst &I) {
  g_InstrTypes.numInsts++;
  checkGlobalLocal(I);
  g_InstrTypes.hasIndirectBranch = true;
}

void CheckInstrTypes::visitICmpInst(ICmpInst &I) {
  g_InstrTypes.numInsts++;
  checkGlobalLocal(I);
  g_InstrTypes.hasCmp = true;
}

void CheckInstrTypes::visitFCmpInst(FCmpInst &I) {
  g_InstrTypes.numInsts++;
  checkGlobalLocal(I);
  g_InstrTypes.hasCmp = true;
}

void CheckInstrTypes::visitAllocaInst(AllocaInst &I) {
  g_InstrTypes.numInsts++;
  checkGlobalLocal(I);
  g_InstrTypes.numAllocaInsts++;
  if (I.isArrayAllocation() || I.getAllocatedType()->isArrayTy() || I.getAllocatedType()->isStructTy() ||
      I.getAllocatedType()->isVectorTy()) {
    g_InstrTypes.hasNonPrimitiveAlloca = true;
  } else {
    g_InstrTypes.hasPrimitiveAlloca = true;
  }

  if (I.getMetadata("igc.read_only_array")) {
    g_InstrTypes.hasReadOnlyArray = true;
  }

  auto PT = dyn_cast<PointerType>(I.getAllocatedType());
  if (PT && PT->getPointerAddressSpace() == ADDRESS_SPACE_GENERIC) {
    g_InstrTypes.hasGenericAddressSpacePointers = true;
  }
}

void CheckInstrTypes::visitLoadInst(LoadInst &I) {
  g_InstrTypes.numInsts++;
  checkGlobalLocal(I);
  g_InstrTypes.hasLoadStore = true;
  g_InstrTypes.numLoadStore++;
  uint as = I.getPointerAddressSpace();
  switch (as) {
  case ADDRESS_SPACE_LOCAL:
    g_InstrTypes.hasLocalLoadStore = true;
    break;
  case ADDRESS_SPACE_GENERIC:
    g_InstrTypes.hasGenericAddressSpacePointers = true;
    g_InstrTypes.hasDynamicGenericLoadStore = true;
    break;
  case ADDRESS_SPACE_GLOBAL:
    g_InstrTypes.hasGlobalLoad = true;
    break;
  default:
    break;
  }

  BufferType bufferType = DecodeBufferType(as);
  switch (bufferType) {
  case IGC::UAV:
    g_InstrTypes.hasStorageBufferLoad = true;
    g_InstrTypes.numUntyped++;
    g_InstrTypes.num1DAccesses++;
    break;
  case IGC::BINDLESS:
  case IGC::BINDLESS_READONLY:
  case IGC::SSH_BINDLESS:
    g_InstrTypes.hasStorageBufferLoad = true;
    break;
  case IGC::STATELESS:
    g_InstrTypes.hasGlobalLoad = true;
    break;
  case IGC::RESOURCE:
    g_InstrTypes.numUntyped++;
    g_InstrTypes.num1DAccesses++;
    break;
  case IGC::SLM:
    g_InstrTypes.numUntyped++;
    g_InstrTypes.hasSLM = true;
    g_InstrTypes.numSLMAccesses++;
    g_InstrTypes.numSLMLoads++;
    break;
  case IGC::CONSTANT_BUFFER:
  case IGC::POINTER:
  case IGC::BINDLESS_CONSTANT_BUFFER:
  case IGC::BINDLESS_WRITEONLY:
  case IGC::BINDLESS_TEXTURE:
  case IGC::SAMPLER:
  case IGC::BINDLESS_SAMPLER:
  case IGC::RENDER_TARGET:
  case IGC::STATELESS_READONLY:
  case IGC::STATELESS_A32:
  case IGC::SSH_BINDLESS_CONSTANT_BUFFER:
  case IGC::SSH_BINDLESS_TEXTURE:
  case IGC::BUFFER_TYPE_UNKNOWN:
    break;
  }

  if (isStatefulAddrSpace(as) && !IsDirectIdx(as)) {
    g_InstrTypes.mayHaveIndirectResources = true;
  }
}

void CheckInstrTypes::visitStoreInst(StoreInst &I) {
  g_InstrTypes.numInsts++;
  checkGlobalLocal(I);
  g_InstrTypes.hasLoadStore = true;
  g_InstrTypes.numLoadStore++;

  uint as = I.getPointerAddressSpace();
  if (as != ADDRESS_SPACE_PRIVATE) {
    g_InstrTypes.psHasSideEffect = true;
  }
  switch (as) {
  case ADDRESS_SPACE_LOCAL:
    g_InstrTypes.hasLocalLoadStore = true;
    break;
  case ADDRESS_SPACE_GENERIC:
    g_InstrTypes.hasGenericAddressSpacePointers = true;
    g_InstrTypes.hasDynamicGenericLoadStore = true;
    break;
  case ADDRESS_SPACE_GLOBAL:
    g_InstrTypes.hasGlobalStore = true;
    break;
  default:
    break;
  }
  BufferType bufferType = DecodeBufferType(as);
  switch (bufferType) {
  case IGC::UAV:
    g_InstrTypes.hasStorageBufferStore = true;
    g_InstrTypes.numUntyped++;
    g_InstrTypes.num1DAccesses++;
    break;
  case IGC::BINDLESS:
  case IGC::BINDLESS_WRITEONLY:
  case IGC::SSH_BINDLESS:
    g_InstrTypes.hasStorageBufferStore = true;
    break;
  case IGC::STATELESS:
    g_InstrTypes.hasGlobalStore = true;
    break;
  case IGC::RESOURCE:
    g_InstrTypes.numUntyped++;
    g_InstrTypes.num1DAccesses++;
    break;
  case IGC::SLM:
    g_InstrTypes.numUntyped++;
    g_InstrTypes.hasSLM = true;
    g_InstrTypes.numSLMAccesses++;
    g_InstrTypes.numSLMStores++;
    break;
  case IGC::CONSTANT_BUFFER:
  case IGC::POINTER:
  case IGC::BINDLESS_CONSTANT_BUFFER:
  case IGC::BINDLESS_READONLY:
  case IGC::BINDLESS_TEXTURE:
  case IGC::SAMPLER:
  case IGC::BINDLESS_SAMPLER:
  case IGC::RENDER_TARGET:
  case IGC::STATELESS_READONLY:
  case IGC::STATELESS_A32:
  case IGC::SSH_BINDLESS_CONSTANT_BUFFER:
  case IGC::SSH_BINDLESS_TEXTURE:
  case IGC::BUFFER_TYPE_UNKNOWN:
    break;
  }

  if (isStatefulAddrSpace(as) && !IsDirectIdx(as)) {
    g_InstrTypes.mayHaveIndirectResources = true;
  }
}

void CheckInstrTypes::visitPHINode(PHINode &PN) {
  g_InstrTypes.numInsts++;
  checkGlobalLocal(PN);
  g_InstrTypes.hasPhi = true;
}

void CheckInstrTypes::visitSelectInst(SelectInst &I) {
  g_InstrTypes.numInsts++;
  checkGlobalLocal(I);
  g_InstrTypes.hasSel = true;
}

void CheckInstrTypes::visitIntrinsicInst(IntrinsicInst &I) {
  auto isGenericPtr = [](Value *V) {
    IGC_ASSERT(V->getType()->isPointerTy());
    PointerType *PTy = cast<PointerType>(V->getType());
    return PTy->getAddressSpace() == ADDRESS_SPACE_GENERIC;
  };

  switch (I.getIntrinsicID()) {
  case Intrinsic::memcpy:
  case Intrinsic::memmove: {
    auto MT = dyn_cast<MemTransferInst>(&I);
    Value *Dst = MT->getRawDest();
    Value *Src = MT->getRawSource();
    if (isGenericPtr(Dst) || isGenericPtr(Src)) {
      g_InstrTypes.hasGenericAddressSpacePointers = true;
      g_InstrTypes.hasDynamicGenericLoadStore = true;
    }
    break;
  }
  case Intrinsic::memset: {
    auto MS = dyn_cast<MemSetInst>(&I);
    Value *Dst = MS->getRawDest();
    if (isGenericPtr(Dst)) {
      g_InstrTypes.hasGenericAddressSpacePointers = true;
      g_InstrTypes.hasDynamicGenericLoadStore = true;
    }
    break;
  }
  default:
    break;
  }
}

void CheckInstrTypes::visitGetElementPtrInst(llvm::GetElementPtrInst &I) {
  g_InstrTypes.numInsts++;
  checkGlobalLocal(I);
  if (I.getPointerAddressSpace() == ADDRESS_SPACE_GENERIC) {
    g_InstrTypes.hasGenericAddressSpacePointers = true;
  }
}

#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS

#define PASS_FLAG "InstrStatistic"
#define PASS_DESCRIPTION "Check individual type of instructions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(InstrStatistic, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(InstrStatistic, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

static cl::opt<bool> enableInstrStatPrint(
    "enable-instrstat-print", cl::init(false), cl::Hidden,
    cl::desc("Enable InstrStatistic pass debug print: output statistic gathered by the pass to debug ostream"));

char InstrStatistic::ID = 0;

InstrStatistic::InstrStatistic(CodeGenContext *ctx, InstrStatTypes type, InstrStatStage stage, int threshold)
    : FunctionPass(ID), m_ctx(ctx), m_type(type), m_stage(stage), m_threshold(threshold) {
  initializeInstrStatisticPass(*PassRegistry::getPassRegistry());
  initializeLoopInfoWrapperPassPass(*PassRegistry::getPassRegistry());

  if (stage == InstrStatStage::BEGIN) {
    m_ctx->instrStat[type][InstrStatStage::BEGIN] = 0;
    m_ctx->instrStat[type][InstrStatStage::END] = 0;
    m_ctx->instrStat[type][InstrStatStage::EXCEED_THRESHOLD] = 0;
  }
}

bool InstrStatistic::runOnFunction(Function &F) {
  bool changed = false;

  if (m_type == LICM_STAT) {
    m_LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    changed = parseLoops();
  } else {
    // run the pass
    visit(F);
  }

  // if this is a call for ending statistic, find out if the difference exceeds the threshold.
  if (m_stage == InstrStatStage::END) {
    if (m_ctx->instrStat[m_type][InstrStatStage::BEGIN] - m_ctx->instrStat[m_type][InstrStatStage::END] > m_threshold) {
      m_ctx->instrStat[m_type][InstrStatStage::EXCEED_THRESHOLD] = 1;
    }

    if (m_type == SROA_PROMOTED) {
      m_ctx->m_retryManager->Disable();
    }
  }

  if (enableInstrStatPrint)
    print(IGC::Debug::ods());

  return changed;
}

void InstrStatistic::print(llvm::raw_ostream &OS) const {
  if (m_stage == InstrStatStage::END) {
    OS << "\nBEGIN: " << m_ctx->instrStat[m_type][InstrStatStage::BEGIN];
    OS << "\nEND: " << m_ctx->instrStat[m_type][InstrStatStage::END];
    OS << "\nEXCEED_THRESHOLD: " << m_ctx->instrStat[m_type][InstrStatStage::EXCEED_THRESHOLD] << "\n\n";
  }
}

void InstrStatistic::visitLoadInst(LoadInst &I) {
  if (m_type == SROA_PROMOTED)
    m_ctx->instrStat[m_type][m_stage]++;
}

void InstrStatistic::visitStoreInst(StoreInst &I) {
  if (m_type == SROA_PROMOTED)
    m_ctx->instrStat[m_type][m_stage]++;
}

bool InstrStatistic::parseLoops() {
  bool changed = false;

  for (auto &LI : *m_LI) {
    Loop *L1 = &(*LI);
    changed |= parseLoop(L1);

    for (auto &L2 : L1->getSubLoops()) {
      changed |= parseLoop(L2);
    }
  }

  return changed;
}

bool InstrStatistic::parseLoop(Loop *loop) {
  auto *header = loop->getHeader();
  m_ctx->instrStat[m_type][m_stage] += header->size();

  return false;
}
