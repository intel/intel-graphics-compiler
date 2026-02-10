/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/NamedBarriers/NamedBarriersResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs/KernelArgs.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"
#include "MDFrameWork.h"
#include "Probe/Assertion.h"

static const unsigned SPIRAS_Local = 3;

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-named-barriers-resolution"
#define PASS_DESCRIPTION "Resolves named barriers"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(NamedBarriersResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(NamedBarriersResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char NamedBarriersResolution::ID = 0;

const char *NamedBarriersResolution::NAMED_BARRIERS_INIT = "_Z18named_barrier_initi";
const char *NamedBarriersResolution::NAMED_BARRIERS_BARRIER_ARG2 =
    "_Z24work_group_named_barrierPU3AS314__namedBarrierj";
const char *NamedBarriersResolution::NAMED_BARRIERS_BARRIER_ARG3 =
    "_Z24work_group_named_barrierPU3AS314__namedBarrierj12memory_scope";

const int NamedBarriersResolution::GetMaxNamedBarriers() {
  if (NamedBarrierHWSupport(m_GFX_CORE)) {
    // User can define only 32 named barriers,
    // The TG barrier is an alias for named barrier 0
    return 32;
  }
  return 8;
}

NamedBarriersResolution::NamedBarriersResolution() : ModulePass(ID) {
  m_CountNamedBarriers = 0;
  m_GFX_CORE = IGFX_GEN8_CORE;
  m_NamedBarrierType = nullptr;
  m_NamedBarrierID = nullptr;
  m_NamedBarrierArray = nullptr;
  initializeNamedBarriersResolutionPass(*PassRegistry::getPassRegistry());
}

NamedBarriersResolution::NamedBarriersResolution(GFXCORE_FAMILY GFX_CORE) : ModulePass(ID) {
  m_CountNamedBarriers = 0;
  m_GFX_CORE = GFX_CORE;
  m_NamedBarrierType = nullptr;
  m_NamedBarrierID = nullptr;
  m_NamedBarrierArray = nullptr;
  initializeNamedBarriersResolutionPass(*PassRegistry::getPassRegistry());
}

NamedBarriersResolution::~NamedBarriersResolution(void) {}

void NamedBarriersResolution::initGlobalVariables(llvm::Module *Module, llvm::Type *NamedBarrierStructType) {
#ifndef DX_ONLY_IGC
#ifndef VK_ONLY_IGC
  LLVMContext &context = Module->getContext();

  m_NamedBarrierType = NamedBarrierStructType;
  llvm::Type *NamedBarrierArrayType = ArrayType::get(m_NamedBarrierType, uint64_t(GetMaxNamedBarriers()));

  m_NamedBarrierArray = new GlobalVariable(*Module, NamedBarrierArrayType, false, GlobalVariable::InternalLinkage,
                                           ConstantAggregateZero::get(NamedBarrierArrayType), "NamedBarrierArray",
                                           nullptr, GlobalVariable::ThreadLocalMode::NotThreadLocal, SPIRAS_Local);
  m_NamedBarrierID = new GlobalVariable(*Module, Type::getInt32Ty(context), false, GlobalVariable::InternalLinkage,
                                        ConstantInt::get(Type::getInt32Ty(context), 0), "NamedBarrierID", nullptr,
                                        GlobalVariable::ThreadLocalMode::NotThreadLocal, SPIRAS_Local);
#endif // #ifndef VK_ONLY_IGC
#endif // #ifndef DX_ONLY_IGC
}

bool NamedBarriersResolution::runOnModule(Module &M) {
  Function *nbarrierInitF = nullptr;
  Function *nbarrierBarrierF = nullptr;

  for (auto &func : M.getFunctionList()) {
    StringRef funcName = func.getName();
    if (funcName.equals(NamedBarriersResolution::NAMED_BARRIERS_INIT)) {
      nbarrierInitF = &func;
    } else if (funcName.equals(NamedBarriersResolution::NAMED_BARRIERS_BARRIER_ARG2) ||
               funcName.equals(NamedBarriersResolution::NAMED_BARRIERS_BARRIER_ARG3)) {
      nbarrierBarrierF = &func;
    } else {
      visit(func);
    }
  }

  if (NamedBarrierHWSupport(m_GFX_CORE)) {
    // Remove not needed wrapper for Init NBarrier built-in
    for (const auto &[barrierStruct, barrierData] : m_MapInitToID) {
      // We dont need any more the store instruction for nbarrier struct
      for (auto user : barrierData.threadGroupNBarrierInit->users()) {
        if (StoreInst *storeInst = dyn_cast<StoreInst>(user)) {
          storeInst->eraseFromParent();
          break;
        }
      }
      barrierData.threadGroupNBarrierInit->eraseFromParent();
    }
    // Add attribute NBarrierCnt to metadata
    auto MD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    MD->NBarrierCnt = m_CountNamedBarriers;
  }
  if (IsNamedBarriersAdded()) {
    // Remove not needed declaration
    nbarrierInitF->eraseFromParent();
    nbarrierBarrierF->eraseFromParent();
  }

  return IsNamedBarriersAdded();
}

Value *NamedBarriersResolution::FindAllocStructNBarrier(Value *Val, bool IsNBarrierInitCall) {
  // We need to find pointer to the nbarrier structure to map
  // together named_barrier_init and work_group_named_barrier calls to have the same nbarrier ID
  if (IsNBarrierInitCall) {
    // The init call "named_barrier_init" will return the result to
    // store instruction which is writing to the allocated nbarrier struct (which we are looking for).
    // example:
    // %a = alloca %struct.__namedBarrier addrspace(3)*, align 8 // <- looking for
    // ...
    // %call = call spir_func %struct.__namedBarrier addrspace(3)* @_Z18named_barrier_initi(i32 1) #0
    // store %struct.__namedBarrier addrspace(3)* %call, %struct.__namedBarrier addrspace(3)** %a, align 8
    for (auto user : Val->users()) {
      if (StoreInst *storeInst = dyn_cast<StoreInst>(user)) {
        return storeInst->getPointerOperand();
      }
    }
    // If nbarrier struct is not used in any store instruction, it means that Mem2Reg optimization has optimized
    // alloca out and the result of named_barrier_init function is used directly in named_barriers calls
    return Val;
  } else {
    // The barrier call "work_group_named_barrier" will use previously called instruction
    // load which is pointing to the allocated nbarrier struct (which we are looking for).
    // example:
    // %a = alloca %struct.__namedBarrier addrspace(3)*, align 8 // <- looking for
    // ...
    // %1 = load %struct.__namedBarrier addrspace(3)*, %struct.__namedBarrier addrspace(3)** %a, align 8
    // call spir_func void @_Z24work_group_named_barrierPU3AS314__namedBarrierj(%struct.__namedBarrier addrspace(3)* %1,
    // i32 1) #0
    if (LoadInst *loadInst = dyn_cast<LoadInst>(Val)) {
      return loadInst->getPointerOperand();
    }
    // If "work_group_named_barrier" call uses NBarrier directly from "named_barriers_init", it means that alloca
    // has been optimized out by Mem2Reg optimization
    else if (CallInst *callInst = dyn_cast<CallInst>(Val)) {
      [[maybe_unused]] StringRef funcName = callInst->getCalledFunction()->getName();
      IGC_ASSERT_MESSAGE(isNamedBarrierInit(funcName),
                         "NamedBarriersResolution : Incorrect work_group_named_barrier signature");
      return callInst;
    }
    IGC_ASSERT_MESSAGE(0,
                       "NamedBarriersResolution : Missing NBarrier struct in work_group_named_barrier function call");
  }
  return nullptr;
}

void NamedBarriersResolution::HandleNamedBarrierInitHW(CallInst &NBarrierInitCall) {
#ifndef DX_ONLY_IGC
#ifndef VK_ONLY_IGC
  m_CountNamedBarriers++;
  IGC_ASSERT_MESSAGE(m_CountNamedBarriers <= GetMaxNamedBarriers(),
                     "NamedBarriersResolution : We crossed the max of amount of named barriers!");
  Module *module = NBarrierInitCall.getModule();

  Value *threadGroupNBarrierID =
      (Value *)ConstantInt::get(Type::getInt32Ty(module->getContext()), m_CountNamedBarriers, true);
  Value *pointerToNBarrierStruct = FindAllocStructNBarrier((Value *)(&NBarrierInitCall), true);

  s_namedBarrierInfo structNb;
  structNb.threadGroupNBarrierID = threadGroupNBarrierID;
  structNb.threadGroupNBarrierCount = NBarrierInitCall.getArgOperand(0);
  structNb.threadGroupNBarrierInit = &NBarrierInitCall;

  // Map nbarrier struct to the his equal ID
  m_MapInitToID.insert(std::pair<Value *, s_namedBarrierInfo>(pointerToNBarrierStruct, structNb));
#endif // #ifndef VK_ONLY_IGC
#endif // #ifndef DX_ONLY_IGC
}

void NamedBarriersResolution::HandleNamedBarrierSyncHW(CallInst &NBarrierSyncCall) {
  Module *module = NBarrierSyncCall.getModule();

  Value *pointerToNBarrierStruct = FindAllocStructNBarrier(NBarrierSyncCall.getArgOperand(0), false);

  auto nbStruct = m_MapInitToID[pointerToNBarrierStruct];

  IRBuilder<> IRB(&NBarrierSyncCall);

  Value *trueValue = IRB.getInt1(true);
  Value *falseValue = IRB.getInt1(false);
  Value *gpuScopeValue = IRB.getInt32(LSC_SCOPE_GPU);
  Value *groupScopeValue = IRB.getInt32(LSC_SCOPE_GROUP);

  ConstantInt *memFenceType = cast<ConstantInt>(NBarrierSyncCall.getArgOperand(1));
  // LOCAL = 1
  // GLOBAL = 2
  bool isGlobal = ((int)memFenceType->getValue().getSExtValue() & 2) == 2;
  Value *isGlobalValue = isGlobal ? trueValue : falseValue;
  // Conservatively for local barrier set GROUP scope, for global barrier set GPU scope
  Value *scopeValue = isGlobal ? gpuScopeValue : groupScopeValue;

  GenIntrinsicInst::Create(GenISAIntrinsic::getDeclaration(module, GenISAIntrinsic::GenISA_memoryfence),
                           {
                               trueValue,     // bool commitEnable
                               falseValue,    // bool flushRW
                               falseValue,    // bool flushConstant
                               falseValue,    // bool flushTexture
                               falseValue,    // bool flushIcache
                               isGlobalValue, // bool isGlobal
                               falseValue,    // bool invalidateL1
                               falseValue,    // bool evictL1
                               scopeValue     // int memory scope
                           },
                           "", &(NBarrierSyncCall));

  CallSignal(nbStruct.threadGroupNBarrierID, nbStruct.threadGroupNBarrierCount, nbStruct.threadGroupNBarrierCount,
             NamedBarrierType::ProducerConsumer, &NBarrierSyncCall);
  CallWait(nbStruct.threadGroupNBarrierID, &NBarrierSyncCall);

  NBarrierSyncCall.eraseFromParent();
}

int NamedBarriersResolution::AlignNBCnt2BarrierNumber(uint NBCnt) {
  /*
      0   None
      1   B1
      2   B2
      3   B4
      4   B8
      5   B16
      6   B24
      7   B32
  */
  if (NBCnt > 32) {
    IGC_ASSERT_UNREACHABLE();
  } // NamedBarriersResolution : Incorrect named barrier count
  else if (NBCnt > 24) {
    return 32;
  } else if (NBCnt > 16) {
    return 24;
  } else if (NBCnt > 8) {
    return 16;
  } else if (NBCnt > 4) {
    return 8;
  } else if (NBCnt > 2) {
    return 4;
  } else if (NBCnt > 1) {
    return 2;
  } else if (NBCnt == 1) {
    return 1;
  } else {
    return 0;
  }
}

bool NamedBarriersResolution::isNamedBarrierInit(StringRef &FunctionName) {
  return FunctionName.equals(NamedBarriersResolution::NAMED_BARRIERS_INIT);
}

bool NamedBarriersResolution::isNamedBarrierSync(StringRef &FunctionName) {
  return FunctionName.equals(NamedBarriersResolution::NAMED_BARRIERS_BARRIER_ARG2) ||
         FunctionName.equals(NamedBarriersResolution::NAMED_BARRIERS_BARRIER_ARG3);
}

void NamedBarriersResolution::HandleNamedBarrierInitSW(CallInst &NBarrierInitCall) {
#ifndef DX_ONLY_IGC
#ifndef VK_ONLY_IGC
  IGC_ASSERT_MESSAGE(m_CountNamedBarriers < GetMaxNamedBarriers(),
                     "NamedBarriersResolution : We crossed the max of amount of named barriers!");
  LLVMContext &context = NBarrierInitCall.getCalledFunction()->getContext();
  Module *module = NBarrierInitCall.getModule();

  // Check if we already have setup global variabels
  if (!IsNamedBarriersAdded()) {
    initGlobalVariables(module, IGCLLVM::getNonOpaquePtrEltTy(NBarrierInitCall.getType()));
  }

  auto newName = "__builtin_spirv_OpNamedBarrierInitialize_i32_p3__namedBarrier_p3i32";
  SmallVector<Type *, 3> ArgsTy{Type::getInt32Ty(context), m_NamedBarrierType->getPointerTo(SPIRAS_Local),
                                Type::getInt32PtrTy(context, SPIRAS_Local)};
  Type *BaseTy = m_NamedBarrierArray->getValueType();
  auto pointerNBarrier = GetElementPtrInst::Create(BaseTy, m_NamedBarrierArray,
                                                   {ConstantInt::get(Type::getInt64Ty(module->getContext()), 0, true),
                                                    ConstantInt::get(Type::getInt32Ty(module->getContext()), 0, true)},
                                                   "", &(NBarrierInitCall));
  auto bitcastPointerNBarrier = BitCastInst::CreatePointerBitCastOrAddrSpaceCast(
      pointerNBarrier, m_NamedBarrierType->getPointerTo(SPIRAS_Local), "", &(NBarrierInitCall));
  SmallVector<Value *, 3> ArgsVal{NBarrierInitCall.getArgOperand(0), bitcastPointerNBarrier, m_NamedBarrierID};
  auto newFType = FunctionType::get(NBarrierInitCall.getType(), ArgsTy, false);
  auto newF = module->getOrInsertFunction(newName, newFType);

  auto newCall = CallInst::Create(newF, ArgsVal, "", &(NBarrierInitCall));
  newCall->setDebugLoc(NBarrierInitCall.getDebugLoc());
  NBarrierInitCall.replaceAllUsesWith(newCall);
  NBarrierInitCall.eraseFromParent();

  m_CountNamedBarriers++;
#endif // #ifndef VK_ONLY_IGC
#endif // #ifndef DX_ONLY_IGC
}

void NamedBarriersResolution::HandleNamedBarrierSyncSW(CallInst &NBarrierSyncCall) {
#ifndef DX_ONLY_IGC
#ifndef VK_ONLY_IGC
  LLVMContext &context = NBarrierSyncCall.getCalledFunction()->getContext();
  Module *module = NBarrierSyncCall.getModule();

  auto newName = "__builtin_spirv_OpMemoryNamedBarrierWrapperOCL_p3__namedBarrier_i32";
  SmallVector<Type *, 2> ArgsTy{m_NamedBarrierType->getPointerTo(SPIRAS_Local), Type::getInt32Ty(context)};
  SmallVector<Value *, 2> ArgsVal{NBarrierSyncCall.getArgOperand(0), NBarrierSyncCall.getArgOperand(1)};

  // For overload with 3 arguments
  if (NBarrierSyncCall.getCalledFunction()->arg_size() == 3) {
    newName = "__builtin_spirv_OpMemoryNamedBarrierWrapperOCL_p3__namedBarrier_i32_i32";
    ArgsTy.insert(ArgsTy.end(), Type::getInt32Ty(context));
    ArgsVal.insert(ArgsVal.end(), NBarrierSyncCall.getArgOperand(2));
  }

  auto newFType = FunctionType::get(NBarrierSyncCall.getType(), ArgsTy, false);
  auto newF = module->getOrInsertFunction(newName, newFType);

  CallInst *newCall = CallInst::Create(newF, ArgsVal, "", &(NBarrierSyncCall));
  newCall->setDebugLoc(NBarrierSyncCall.getDebugLoc());
  NBarrierSyncCall.eraseFromParent();
#endif // #ifndef VK_ONLY_IGC
#endif // #ifndef DX_ONLY_IGC
}

void NamedBarriersResolution::visitCallInst(CallInst &CI) {
  Function *func = CI.getCalledFunction();
  if (!func) {
    return;
  }

  StringRef funcName = func->getName();

  if (isNamedBarrierInit(funcName)) {
    NamedBarrierHWSupport(m_GFX_CORE) ? HandleNamedBarrierInitHW(CI) : HandleNamedBarrierInitSW(CI);
  } else if (isNamedBarrierSync(funcName)) {
    NamedBarrierHWSupport(m_GFX_CORE) ? HandleNamedBarrierSyncHW(CI) : HandleNamedBarrierSyncSW(CI);
  }
}

bool NamedBarriersResolution::NamedBarrierHWSupport(GFXCORE_FAMILY GFX_CORE) {
  bool hwSupport = GFX_CORE == IGFX_XE_HPC_CORE;
  hwSupport |= GFX_CORE == IGFX_XE2_HPG_CORE;
  hwSupport |= GFX_CORE == IGFX_XE3_CORE;
  hwSupport |= GFX_CORE == IGFX_XE3P_CORE;
  return hwSupport;
}

void NamedBarriersResolution::CallSignal(llvm::Value *barrierID, llvm::Value *ProducerCnt, llvm::Value *ConsumerCnt,
                                         NamedBarrierType Type, llvm::Instruction *pInsertBefore) {
  IGCLLVM::IRBuilder<> builder(pInsertBefore);
  llvm::Module *pM = pInsertBefore->getModule();

  llvm::Value *namedBarrierType = builder.getInt16((int)Type);

  llvm::Value *getIDInt8 =
      llvm::BitCastInst::CreateIntegerCast(barrierID, builder.getInt8Ty(), false, "", pInsertBefore);
  llvm::Value *getProducerCntInt8 =
      llvm::BitCastInst::CreateIntegerCast(ProducerCnt, builder.getInt8Ty(), false, "", pInsertBefore);
  llvm::Value *getConsumerCntInt8 =
      llvm::BitCastInst::CreateIntegerCast(ConsumerCnt, builder.getInt8Ty(), false, "", pInsertBefore);

  GenIntrinsicInst::Create(GenISAIntrinsic::getDeclaration(pM, GenISAIntrinsic::GenISA_threadgroupnamedbarriers_signal),
                           {getIDInt8, namedBarrierType, getProducerCntInt8, getConsumerCntInt8}, "", pInsertBefore);
}

void NamedBarriersResolution::CallWait(llvm::Value *barrierID, llvm::Instruction *pInsertBefore) {
  IGCLLVM::IRBuilder<> builder(pInsertBefore);
  llvm::Module *pM = pInsertBefore->getModule();

  llvm::Value *getIDInt8 =
      llvm::BitCastInst::CreateIntegerCast(barrierID, builder.getInt8Ty(), false, "", pInsertBefore);

  GenIntrinsicInst::Create(GenISAIntrinsic::getDeclaration(pM, GenISAIntrinsic::GenISA_threadgroupnamedbarriers_wait),
                           {getIDInt8}, "", pInsertBefore);
}