/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IGC/common/StringMacros.hpp"
#include "IGC/AdaptorCommon/RayTracing/RTBuilder.h"
#include <map>
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"
#include "ResolveOCLRaytracingBuiltins.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "Probe/Assertion.h"
#include "IGC/AdaptorCommon/RayTracing/RTBuilder.h"
#include "IGC/AdaptorCommon/RayTracing/RTStackFormat.h"
#include "IGC/AdaptorCommon/RayTracing/RayTracingRayDispatchGlobalData.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-resolve-ocl-raytracing-builtins"
#define PASS_DESCRIPTION "Resolve OCL raytracing built-ins"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ResolveOCLRaytracingBuiltins, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(ResolveOCLRaytracingBuiltins, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace {
  std::map<std::string, std::function<void(ResolveOCLRaytracingBuiltins*, CallInst&)>> functionHandlersMap = {
    {"__builtin_IB_intel_get_rt_stack",                  &ResolveOCLRaytracingBuiltins::handleGetRtStack            },
    {"__builtin_IB_intel_get_thread_btd_stack",          &ResolveOCLRaytracingBuiltins::handleGetThreadBTDStack     },
    {"__builtin_IB_intel_get_global_btd_stack",          &ResolveOCLRaytracingBuiltins::handleGetGlobalBTDStack     },
    {"__builtin_IB_intel_dispatch_trace_ray_query",      &ResolveOCLRaytracingBuiltins::handleDispatchTraceRayQuery },
    {"__builtin_IB_intel_rt_sync",                       &ResolveOCLRaytracingBuiltins::handleRTSync                },
    {"__builtin_IB_intel_get_implicit_dispatch_globals", &ResolveOCLRaytracingBuiltins::handleGetImplicitDG         },
  };
}

char ResolveOCLRaytracingBuiltins::ID = 0;

ResolveOCLRaytracingBuiltins::ResolveOCLRaytracingBuiltins() :
  ModulePass(ID), m_pCtx(nullptr), m_builder(nullptr) {
  initializeResolveOCLRaytracingBuiltinsPass(*PassRegistry::getPassRegistry());
}

bool ResolveOCLRaytracingBuiltins::runOnModule(Module& M) {
    m_pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_callsToReplace.clear();

    IGCIRBuilder<> builder(M.getContext());
    m_builder = &builder;

    // Fills up the m_CallsToReplace with all instances of calls to kernels in the functionHandlersMap.
    visit(M);

    if (m_callsToReplace.size() > 0) {
        if (m_pCtx->platform.getPlatformInfo().eRenderCoreFamily != IGFX_XE_HPC_CORE &&
            m_pCtx->platform.getPlatformInfo().eProductFamily != IGFX_DG2) {
            IGC_ASSERT_MESSAGE(0, "Raytracing extensions used on unsupported platform!");
            m_pCtx->EmitError("OCL raytracing extensions can be used only on supported platform", *m_callsToReplace.begin());
            return false;
        }

        constexpr uint32_t AllocSize = RTStackFormat::getSyncStackSize();
        m_pCtx->getModuleMetaData()->rtInfo.RayQueryAllocSizeInBytes = AllocSize;
    }

    bool found_regular_function = false;
    auto& FuncMap = m_pCtx->getModuleMetaData()->FuncMD;
    // the vector contains RT calls inside both kernels and regular functiuons
    for (auto p : m_callsToReplace) {
        IGC_ASSERT(nullptr != p);  // All code below assumes that it catches the null pointers.
        m_builder->SetInsertPoint(p);

        // Set the hasSyncRTCalls for each FunctionMetaData of a Function which is in the ModuleMetaData.
        // We will be reading that value later on in the code.
        auto FuncIter = FuncMap.find(p->getFunction());
        if (FuncIter != FuncMap.end()) {
            // metadata for that function/kernel was found
            // usually all the kernels have their metadata saved
            FunctionMetaData& funcMD = FuncIter->second;
            if (funcMD.functionType == KernelFunction) {
                // is a kernel
                funcMD.hasSyncRTCalls = true;
            }
            else {
                // is a regular function
                funcMD.hasSyncRTCalls = true;
                found_regular_function = true;
            }
        }
        else {
            // metadata for that function/kernel was not found
            // likely this is a regular function, not a kernel
            found_regular_function = true;
        }

        // Dereference that function pointer at that location in the map.
        (functionHandlersMap.at( p->getCalledFunction()->getName().str() ))(this, *p);
    }

    // If we found RT calls in a regular function, that regular function could have been called by a kernel.
    // In that case the kernel also exhibits ray tracing functionality.
    // For now just loop through all the kernels (that we have metadata for) and assume that any one of them may be the caller.
    // Marking them all as having ray tracing calls seems to be not harmful.
    // TODO: In future implementation go back up the call graph from the regular function with hasSyncRTCalls until you find the parent kernel.
    if (found_regular_function) {
        for (auto& md_entry : FuncMap) {
            md_entry.second.hasSyncRTCalls = true;
        }
    }

    return m_callsToReplace.size() > 0;
}

void ResolveOCLRaytracingBuiltins::visitCallInst(CallInst& callInst) {
  if (!callInst.getCalledFunction()) return;
  if (functionHandlersMap.find(callInst.getCalledFunction()->getName().str()) != functionHandlersMap.end()) {
    m_callsToReplace.push_back(&callInst);
  }
}

/*
Handler for:
void* __builtin_IB_intel_get_rt_stack( rtglobals_t rt_dispatch_globals );

Description:
Returns a pointer to the data structure which the RT hardware operates on.
The RT Stack address is computed as:
    syncStackSize = sizeof(HitInfo)*2 + (sizeof(Ray) + sizeof(TravStack))*RTDispatchGlobals.maxBVHLevels;
    syncBase = RTDispatchGlobals.rtMemBasePtr - (DSSID * NUM_SIMD_LANES_PER_DSS + StackID + 1)*syncStackSize;
Where DSSID is an index which uniquely identifies the DSS in the machine (across tiles), and StackID is compute as below:
    With fused EUs (e.g. in DG2) :
      StackID[10:0] (msb to lsb) = (EUID[3:0]<<7) | (THREAD_ID[2:0]<<4) | SIMD_LANE_ID[3:0]

    With natively wide EUs(e.g. in PVC):
      StackID[10:0] (msb to lsb) = (EUID[2:0]<<8) | (THREAD_ID[3:0]<<4) | SIMD_LANE_ID[3:0]

*/
void ResolveOCLRaytracingBuiltins::handleGetRtStack(CallInst& callInst) {
  IGC_ASSERT(callInst.getType()->isPointerTy());
  auto rtDispatchGlobals = callInst.getArgOperand(0);

  // Calculate:
  // syncBase = RTDispatchGlobals.rtMemBasePtr - (DSSID * NUM_SIMD_LANES_PER_DSS + StackID + 1)*syncStackSize;
  RTBuilder rtbuilder(m_builder->getContext(), *m_pCtx);
  rtbuilder.SetInsertPoint(&callInst);

  // RTBuilder uses GEP instructions on the global pointer for some operations.
  // Cast explicit global buffer pointer type to the type used by RTBuilder.
  auto GlobalPtrTy = rtbuilder.getRayDispatchGlobalDataPtrTy(*callInst.getModule());
  rtDispatchGlobals = rtbuilder.CreatePointerCast(rtDispatchGlobals, GlobalPtrTy);

  // By default RTBuilder uses an implicit global buffer pointer.
  // OCL extensions use explicit buffer.
  rtbuilder.setGlobalBufferPtr(rtDispatchGlobals);

  // Disable optimisation used on DX path.
  rtbuilder.setDisableRTGlobalsKnownValues(true);

  auto rtMemBasePtr = rtMemBasePtrGetter(rtDispatchGlobals);
  Value* stackOffset = rtbuilder.CreateZExt(rtbuilder.getSyncStackOffset(), rtMemBasePtr->getType());
  Value* syncBase = m_builder->CreateSub(rtMemBasePtr, stackOffset, "syncBase");

  // Cast to resulting pointer
  syncBase = m_builder->CreateIntToPtr(syncBase, callInst.getType());
  callInst.replaceAllUsesWith(syncBase);
  callInst.eraseFromParent();
}

/*
Handler for
void* __builtin_IB_intel_get_thread_btd_stack( rtglobals_t rt_dispatch_globals );

Description:
Returns a pointer to the extra per-thread storage for the given stack.
This is computed as:
  local_stack = rt_dispatch_globals->rtMemBasePtr +
                   rt_dispatch_globals->stackSizePerRay * 64 *
                     (BTD_Stack_ID + DSSID * rt_dispatch_globals->numDSSRTStacks) ;

The BTD_Stack_ID is the StackID which is allocated by hardware when BTD is enabled.

*/
void ResolveOCLRaytracingBuiltins::handleGetThreadBTDStack(CallInst& callInst) {
  handleGetBTDStack(callInst, false);
}

/*
Handler for
void* __builtin_IB_intel_get_global_btd_stack( rtglobals_t rt_dispatch_globals );

Description:
Returns a pointer to the extra global storage for the given stack.
This is computed as:

global_extra = rt_dispatch_globals->rtMemBasePtr +
                 rt_dispatch_globals->stackSizePerRay * 64 *
                 rt_dispatch_globals->numDSSRTStacks  *
                 DSS_COUNT;
*/
void ResolveOCLRaytracingBuiltins::handleGetGlobalBTDStack(CallInst& callInst) {
  handleGetBTDStack(callInst, true);
}

void ResolveOCLRaytracingBuiltins::handleGetBTDStack(CallInst& callInst, const bool isGlobal) {
  IGC_ASSERT(callInst.getType()->isPointerTy());
  auto rtDispatchGlobals = callInst.getArgOperand(0);
  Value* rtMemBasePtr = rtMemBasePtrGetter(rtDispatchGlobals);
  Value* stackSizePerRay = stackSizePerRayGetter(rtDispatchGlobals);
  Value* numDSSStacks = numDSSRTStacksGetter(rtDispatchGlobals);
  Value* dssID = getIntrinsicValue(GenISAIntrinsic::GenISA_dual_subslice_id);

  stackSizePerRay = m_builder->CreateZExt(stackSizePerRay, m_builder->getInt64Ty());
  numDSSStacks = m_builder->CreateZExt(numDSSStacks, m_builder->getInt64Ty());
  dssID = m_builder->CreateZExt(dssID, m_builder->getInt64Ty());

  Value* stack = m_builder->CreateMul(dssID, numDSSStacks);
  if (!isGlobal) {
    Value* btdStackID = getIntrinsicValue(GenISAIntrinsic::GenISA_AsyncStackID);
    btdStackID = m_builder->CreateZExt(btdStackID, m_builder->getInt64Ty());
    stack = m_builder->CreateAdd(stack, btdStackID);
  }
  stack = m_builder->CreateMul(stack, m_builder->getInt64(64));
  stack = m_builder->CreateMul(stack, stackSizePerRay);
  stack = m_builder->CreateAdd(stack, rtMemBasePtr);

  stack = m_builder->CreateIntToPtr(stack, callInst.getType());

  callInst.replaceAllUsesWith(stack);
  callInst.eraseFromParent();
}

// Note: this can be merged with RTBuilder::CreateLSCFence()
// once we use RTBuilder in this file.
CallInst* ResolveOCLRaytracingBuiltins::CreateLSCFence(
    llvm::IRBuilder<>* IRB,
    LSC_SFID SFID,
    LSC_SCOPE Scope,
    LSC_FENCE_OP FenceOp)
{
    Function* pFunc = GenISAIntrinsic::getDeclaration(
        IRB->GetInsertBlock()->getModule(),
        GenISAIntrinsic::GenISA_LSCFence);

    Value* VSFID  = IRB->getInt32(SFID);
    Value* VScope = IRB->getInt32(Scope);
    Value* VOp    = IRB->getInt32(FenceOp);

    Value* Args[] =
    {
        VSFID, VScope, VOp
    };

    return IRB->CreateCall(pFunc, Args);
}

/*
Handler for
rtfence_t __builtin_IB_intel_dispatch_trace_ray_query(
  rtglobals_t rt_dispatch_globals, uint bvh_level, uint traceRayCtrl);

Description:
Invokes the RT HW within EU kernel code.
The RTDispatchGlobals argument is required to be uniform across the subgroup.
The BVH Level argument is used for software instancing.
The Trace Ray Control argument is an enumerant controlling the RT HW.

The compiler will implement this function by assembling the inputs into a RT message.
The return value of this function is a sync object which will be used by the kernel to synchronize the RT message.
*/
void ResolveOCLRaytracingBuiltins::handleDispatchTraceRayQuery(CallInst& callInst) {
  IGC_ASSERT(callInst.getType()->isPointerTy());
  IGC_ASSERT(IGCLLVM::getNumArgOperands(&callInst) == 3);

  // Insert a ugm fence prior to send.rta to ensure RTUnit has accesss to
  // current data.
  CreateLSCFence(m_builder, LSC_UGM, LSC_SCOPE_LOCAL, LSC_FENCE_OP_NONE);

  auto rtDispatchGlobals = callInst.getArgOperand(0);
  auto bvhLevel = callInst.getArgOperand(1);
  auto traceRayCtrl = callInst.getArgOperand(2);

  // Prepare the payload
  // [0:2] bvh_level
  bvhLevel = m_builder->CreateAnd(bvhLevel, 7);
  // [8:9] trace_ray_control
  traceRayCtrl = m_builder->CreateAnd(traceRayCtrl, 3);
  traceRayCtrl = m_builder->CreateShl(traceRayCtrl, 8);
  // [26:16] stack_id.
  // for RayQuery this is not used, leave it as 0.

  Value* payload = m_builder->CreateOr(bvhLevel, traceRayCtrl, VALUE_NAME("traceRayQueryPayload"));

  Value* Args[] = {
      rtDispatchGlobals,
      payload
  };

  // The return value from this TraceRay with RayQueryEnable is a dummy register, which is used
  // to sync this message via scoreboard.
  auto fenceValue = getIntrinsicValue(GenISAIntrinsic::GenISA_TraceRaySync, Args);
  fenceValue = m_builder->CreateIntToPtr(fenceValue, callInst.getType());
  if (m_pCtx->platform.RTFenceWAforBkModeEnabled())
  {
      CreateLSCFence(m_builder, LSC_UGM, LSC_SCOPE_GPU, LSC_FENCE_OP_EVICT);
  }
  callInst.replaceAllUsesWith(fenceValue);
  callInst.eraseFromParent();
}

/*
Handler for
void __builtin_IB_intel_rt_sync(rtfence_t fence);

Description:
Ensures the asynchronous ray query is complete.
The kernel must pass the fence object returned by the ray dispatch query function
to this sync function prior to reading any results from the RT Stack
*/
void ResolveOCLRaytracingBuiltins::handleRTSync(CallInst& callInst) {
  auto fence = m_builder->CreatePtrToInt(callInst.getOperand(0), m_builder->getInt32Ty());
  getIntrinsicValue(GenISAIntrinsic::GenISA_ReadTraceRaySync, fence);
  callInst.eraseFromParent();
}

/*
Handler for
void __builtin_IB_intel_get_implicit_dispatch_globals();

Description:
Returns IMPLICIT_RT_GLOBAL_BUFFER implicit argument.
*/
void ResolveOCLRaytracingBuiltins::handleGetImplicitDG(llvm::CallInst& callInst) {
    RTBuilder rtbuilder(m_builder->getContext(), *m_pCtx);
    rtbuilder.SetInsertPoint(&callInst);
    auto v = rtbuilder.getGlobalBufferPtr();
    auto c = rtbuilder.CreateBitCast(v, callInst.getType());
    callInst.replaceAllUsesWith(c);
    callInst.eraseFromParent();
}

// ---- Helper functions ----

Instruction* ResolveOCLRaytracingBuiltins::loadFromOffset(Value* basePtr, const size_t offset, const size_t typeSizeInBytes, StringRef valName = "") {
  IGC_ASSERT(isa<PointerType>(basePtr->getType()));
  Value* ptrAsInt = m_builder->CreatePtrToInt(basePtr, m_builder->getInt64Ty());
  Value* intWithOffset = m_builder->CreateAdd(ptrAsInt, m_builder->getInt64(offset));
  Type* resType = m_builder->getIntNTy(typeSizeInBytes * 8);
  Value* ptrWithOffset = m_builder->CreateIntToPtr(intWithOffset,
    PointerType::get(resType, basePtr->getType()->getPointerAddressSpace()));
  Instruction* loadedVal = m_builder->CreateLoad(ptrWithOffset, valName);
  return loadedVal;
}

#define RT_DISPATCH_GETTER_DEF(FieldName) \
Instruction* ResolveOCLRaytracingBuiltins::FieldName##Getter(Value* rtDispatchGlobalsValue) {  \
  constexpr size_t offset = offsetof(RayDispatchGlobalData, FieldName);                        \
  constexpr size_t typeSize = sizeof(((RayDispatchGlobalData*)0)->FieldName);                  \
  return loadFromOffset(rtDispatchGlobalsValue, offset, typeSize, #FieldName);                 \
}

RT_DISPATCH_GETTER_DEF(rtMemBasePtr)
RT_DISPATCH_GETTER_DEF(maxBVHLevels)
RT_DISPATCH_GETTER_DEF(stackSizePerRay)
RT_DISPATCH_GETTER_DEF(numDSSRTStacks)

#undef RT_DISPATCH_GETTER_DEF

Value* ResolveOCLRaytracingBuiltins::getIntrinsicValue(GenISAIntrinsic::ID intrinsicId, ArrayRef<Value*> args) {
  std::vector<Type*> types;
  if (!args.empty()) {
    std::transform(args.begin(), args.end(), std::back_inserter(types), [](Value* v) { return v->getType(); });
  }
  return m_builder->CreateCall(GenISAIntrinsic::getDeclaration(m_builder->GetInsertPoint()->getModule(), intrinsicId, types), args);
}

