/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <map>
#include "common/LLVMWarningsPush.hpp"
#include "common/LLVMWarningsPop.hpp"
#include "ResolveOCLRaytracingBuiltins.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "Probe/Assertion.h"
#include "IGC/AdaptorCommon/RayTracing/RTBuilder.h"

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
// clang-format off
    {"__builtin_IB_intel_get_rt_stack",                  &ResolveOCLRaytracingBuiltins::handleGetRtStack            },
    {"__builtin_IB_intel_get_thread_btd_stack",          &ResolveOCLRaytracingBuiltins::handleGetThreadBTDStack     },
    {"__builtin_IB_intel_get_global_btd_stack",          &ResolveOCLRaytracingBuiltins::handleGetGlobalBTDStack     },
    {"__builtin_IB_intel_dispatch_trace_ray_query",      &ResolveOCLRaytracingBuiltins::handleDispatchTraceRayQuery },
    {"__builtin_IB_intel_rt_sync",                       &ResolveOCLRaytracingBuiltins::handleRTSync                },
    {"__builtin_IB_intel_get_rt_global_buffer",          &ResolveOCLRaytracingBuiltins::handleGetRTGlobalBuffer     },
// clang-format on
// clang-format off

    // Handling for builtins operating on intel_ray_query_t from intel_rt_production extension
      {"__builtin_IB_intel_init_ray_query",                &ResolveOCLRaytracingBuiltins::handleInitRayQuery          },
      {"__builtin_IB_intel_update_ray_query",              &ResolveOCLRaytracingBuiltins::handleUpdateRayQuery        },
      {"__builtin_IB_intel_query_rt_fence",                &ResolveOCLRaytracingBuiltins::handleQuery                 },
      {"__builtin_IB_intel_query_rt_globals",              &ResolveOCLRaytracingBuiltins::handleQuery                 },
      {"__builtin_IB_intel_query_rt_stack",                &ResolveOCLRaytracingBuiltins::handleQuery                 },
      {"__builtin_IB_intel_query_ctrl",                    &ResolveOCLRaytracingBuiltins::handleQuery                 },
      {"__builtin_IB_intel_query_bvh_level",               &ResolveOCLRaytracingBuiltins::handleQuery                 },
// clang-format on
  };
}

static Value* castToRTGlobalsTy(RTBuilder& RTB, Value* V) {
    // Cast explicit global buffer pointer type to the type used by RTBuilder.
    auto* M = RTB.GetInsertBlock()->getModule();
    auto GlobalPtrTy = RTB.getRayDispatchGlobalDataPtrTy(
        *M, ADDRESS_SPACE_GLOBAL);
    return RTB.CreatePointerCast(V, GlobalPtrTy);
}

char ResolveOCLRaytracingBuiltins::ID = 0;

ResolveOCLRaytracingBuiltins::ResolveOCLRaytracingBuiltins() :
    ModulePass(ID) {
    initializeResolveOCLRaytracingBuiltinsPass(*PassRegistry::getPassRegistry());
}

bool ResolveOCLRaytracingBuiltins::runOnModule(Module& M) {
    m_pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_callsToReplace.clear();

    // Fills up the m_CallsToReplace with all instances of calls to kernels in the functionHandlersMap.
    visit(M);

    if (m_callsToReplace.empty())
        return false;

    if (!m_pCtx->platform.supportRayTracing()) {
        IGC_ASSERT_MESSAGE(0, "Raytracing extensions used on unsupported platform!");
        m_pCtx->EmitError("OCL raytracing extensions can be used only on supported platform", *m_callsToReplace.begin());
        return false;
    }

    RTBuilder builder(M.getContext(), *m_pCtx);
    m_builder = &builder;
    // Disable optimisation assuming BVHLevels=2
    m_builder->setDisableRTGlobalsKnownValues(true);

    // intel_rt_production extension uses intel_ray_query_opaque_t which cannot be defined in BiF Library
    defineOpaqueTypes();

    constexpr uint32_t AllocSize = RTStackFormat::getSyncStackSize();
    m_pCtx->getModuleMetaData()->rtInfo.RayQueryAllocSizeInBytes = AllocSize;

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

    return true;
}

void ResolveOCLRaytracingBuiltins::defineOpaqueTypes() {
    Module* M = m_pCtx->getModule();
    LLVMContext& C = M->getContext();

    StructType* rayQueryTy = IGCLLVM::getTypeByName(*M, "struct.intel_ray_query_opaque_t");

    if (!rayQueryTy) return;

    auto getOrCreateOpaqueType = [](Module* M, const std::string& Name) {
        StructType* opaqueType = IGCLLVM::getTypeByName(*M, Name);
        if (!opaqueType)
            opaqueType = StructType::create(M->getContext(), Name);
        return opaqueType;
    };

    StructType* rtFenceTy = getOrCreateOpaqueType(M, "struct.rtfence_t");
    StructType* rtGlobalsTy = getOrCreateOpaqueType(M, "struct.rtglobals_t");

    IGC_ASSERT(rtFenceTy && rtGlobalsTy);


    SmallVector<Type*, 4> Tys{
        PointerType::get(rtFenceTy, ADDRESS_SPACE_PRIVATE),
        PointerType::get(rtGlobalsTy, ADDRESS_SPACE_GLOBAL),
        PointerType::get(Type::getInt8Ty(C), ADDRESS_SPACE_GLOBAL),
        Type::getInt32Ty(C),
        Type::getInt32Ty(C)
    };

    rayQueryTy->setBody(Tys);
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
    syncBase = RTDispatchGlobals.rtMemBasePtr - (DSSID * NUM_SIMD_LANES_PER_DSS + StackID + 1)*syncStackSize; */
/* Where DSSID is an index which uniquely identifies the DSS in the machine (across tiles), and StackID is compute as below:
    With fused EUs (e.g. in DG2) :
      StackID[10:0] (msb to lsb) = (EUID[3:0]<<7) | (THREAD_ID[2:0]<<4) | SIMD_LANE_ID[3:0]

    With natively wide EUs(e.g. in PVC):
      StackID[10:0] (msb to lsb) = (EUID[2:0]<<8) | (THREAD_ID[3:0]<<4) | SIMD_LANE_ID[3:0]

*/
void ResolveOCLRaytracingBuiltins::handleGetRtStack(CallInst& callInst) {
  m_builder->SetInsertPoint(&callInst);
  IGC_ASSERT(callInst.getType()->isPointerTy());
  auto *rtDispatchGlobals = castToRTGlobalsTy(*m_builder, callInst.getArgOperand(0));

  // By default RTBuilder uses an implicit global buffer pointer.
  // OCL extensions use explicit buffer.
  m_builder->setGlobalBufferPtr(rtDispatchGlobals);

  // Calculate:
  // syncBase = RTDispatchGlobals.rtMemBasePtr - (DSSID * NUM_SIMD_LANES_PER_DSS + StackID + 1)*syncStackSize;

  auto* rtMemBasePtr = m_builder->getRtMemBasePtr();
  Value* stackOffset = m_builder->CreateZExt(m_builder->getSyncStackOffset(), rtMemBasePtr->getType());
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
  m_builder->SetInsertPoint(&callInst);
  IGC_ASSERT(callInst.getType()->isPointerTy());
  auto *rtDispatchGlobals = castToRTGlobalsTy(*m_builder, callInst.getArgOperand(0));
  m_builder->setGlobalBufferPtr(rtDispatchGlobals);
  Value* rtMemBasePtr = m_builder->getRtMemBasePtr();
  Value* stackSizePerRay = m_builder->getStackSizePerRay();
  Value* numDSSStacks = m_builder->getNumDSSRTStacks();
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
  m_builder->SetInsertPoint(&callInst);
  IGC_ASSERT(callInst.getType()->isPointerTy());
  IGC_ASSERT(IGCLLVM::getNumArgOperands(&callInst) == 3);

  // Insert a ugm fence prior to send.rta to ensure RTUnit has accesss to
  // current data.
  m_builder->CreateLSCFence(LSC_UGM, LSC_SCOPE_LOCAL, LSC_FENCE_OP_NONE);

  auto *rtDispatchGlobals = callInst.getArgOperand(0);
  auto *bvhLevel = callInst.getArgOperand(1);
  auto *traceRayCtrl = callInst.getArgOperand(2);

  m_builder->setGlobalBufferPtr(rtDispatchGlobals);

  // Prepare the payload
  // [2:0] bvh_level
  bvhLevel = m_builder->CreateAnd(bvhLevel, 7);
  // [9:8] trace_ray_control
  traceRayCtrl = m_builder->CreateAnd(traceRayCtrl, 3);
  // [27:16] stack_id.
  // for RayQuery this is not used, leave it as 0.

  // The return value from this TraceRay with RayQueryEnable is a dummy register, which is used
  // to sync this message via scoreboard.
  Value *fenceValue = m_builder->createSyncTraceRay(bvhLevel, traceRayCtrl);
  fenceValue = m_builder->CreateIntToPtr(fenceValue, callInst.getType());
  if (m_pCtx->platform.RTFenceWAforBkModeEnabled())
  {
      m_builder->CreateLSCFence(LSC_UGM, LSC_SCOPE_GPU, LSC_FENCE_OP_EVICT);
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
  m_builder->SetInsertPoint(&callInst);
  auto fence = m_builder->CreatePtrToInt(callInst.getOperand(0), m_builder->getInt32Ty());
  getIntrinsicValue(GenISAIntrinsic::GenISA_ReadTraceRaySync, fence);
  callInst.eraseFromParent();
}

/*
Handler for
void __builtin_IB_intel_get_rt_global_buffer();

Description:
Returns IMPLICIT_RT_GLOBAL_BUFFER implicit argument.
*/
void ResolveOCLRaytracingBuiltins::handleGetRTGlobalBuffer(llvm::CallInst& callInst) {
    m_builder->SetInsertPoint(&callInst);
    m_builder->setGlobalBufferPtr(nullptr);
    auto v = m_builder->getGlobalBufferPtr(ADDRESS_SPACE_GLOBAL);
    auto c = m_builder->CreateBitCast(v, callInst.getType());
    callInst.replaceAllUsesWith(c);
    callInst.eraseFromParent();
}


/*
Handler for
void __builtin_IB_intel_init_ray_query(
    rtfence_t, rtglobals_t,global RTStack*, TraceRayCtrl, uint);

Description:
Allocates private memory for rayquery object and stores all it's initializer
values passed as argument to __builtin_IB_intel_init_ray_query.
*/
void ResolveOCLRaytracingBuiltins::handleInitRayQuery(llvm::CallInst& callInst) {
    Function* F = callInst.getFunction();
    m_builder->SetInsertPoint(&*F->getEntryBlock().getFirstInsertionPt());

    unsigned numArgs = IGCLLVM::getNumArgOperands(&callInst);
    IGC_ASSERT(numArgs == 5);

    auto* allocaType = IGCLLVM::getTypeByName(callInst.getModule(), "struct.intel_ray_query_opaque_t");
    auto* alloca = m_builder->CreateAlloca(allocaType);

    m_builder->SetInsertPoint(&callInst);


    auto storeToAlloca = [&](unsigned argIndex)
    {
        auto ptr = m_builder->CreateGEP(alloca->getAllocatedType(), alloca, { m_builder->getInt32(0), m_builder->getInt32(argIndex) });
        auto arg = callInst.getOperand(argIndex);
        m_builder->CreateStore(arg, ptr);
    };

    for (unsigned argIndex = 0; argIndex < numArgs; argIndex++)
        storeToAlloca(argIndex);


    callInst.replaceAllUsesWith(alloca);
    callInst.eraseFromParent();
}

/*
Handler for
void __builtin_IB_intel_update_ray_query(
    intel_ray_query_t, rtfence_t, rtglobals_t, global RTStack*, TraceRayCtrl, uint);

Description:
Stores new values to rayquery alloca
*/
void ResolveOCLRaytracingBuiltins::handleUpdateRayQuery(llvm::CallInst& callInst) {
    m_builder->SetInsertPoint(&callInst);

    unsigned numArgs = IGCLLVM::getNumArgOperands(&callInst);
    IGC_ASSERT(numArgs == 6);

    Value* rayQuery = callInst.getOperand(0);
    StructType* rayQueryTy = IGCLLVM::getTypeByName(callInst.getModule(), "struct.intel_ray_query_opaque_t");


    for (unsigned argIndex = 1; argIndex < numArgs; argIndex++)
    {
        auto* ptr = m_builder->CreateGEP(rayQueryTy, rayQuery, { m_builder->getInt32(0), m_builder->getInt32(argIndex - 1) });
        Value* arg = callInst.getOperand(argIndex);
        m_builder->CreateStore(arg, ptr);
    }

    callInst.eraseFromParent();
}

/*
Handler for the following builtins:
rtfence_t        __builtin_IB_intel_query_rt_fence(intel_ray_query_t);
rtglobals_t      __builtin_IB_intel_query_rt_globals(intel_ray_query_t);
global void*  __builtin_IB_intel_query_rt_stack(intel_ray_query_t);
TraceRayCtrl     __builtin_IB_intel_query_ctrl(intel_ray_query_t);
uint             __builtin_IB_intel_query_bvh_level(intel_ray_query_t);

Description:
Loads queried value from rayquery alloca
*/
void ResolveOCLRaytracingBuiltins::handleQuery(llvm::CallInst& callInst) {
    m_builder->SetInsertPoint(&callInst);

    enum RayQueryArgsOrder
    {
        RT_FENCE,
        RT_GLOBALS,
        RT_STACK,
        CTRL,
        BVH_LEVEL
    };

    static const std::map<std::string, RayQueryArgsOrder> builtinToArgIndex = {
        { "__builtin_IB_intel_query_rt_fence",   RT_FENCE },
        { "__builtin_IB_intel_query_rt_globals", RT_GLOBALS },
        { "__builtin_IB_intel_query_rt_stack",   RT_STACK },
        { "__builtin_IB_intel_query_ctrl",       CTRL },
        { "__builtin_IB_intel_query_bvh_level",  BVH_LEVEL}
    };

    IGC_ASSERT(IGCLLVM::getNumArgOperands(&callInst) == 1);
    Value* rayQuery = callInst.getArgOperand(0);
    StructType* rayQueryTy = IGCLLVM::getTypeByName(callInst.getModule(), "struct.intel_ray_query_opaque_t");

    unsigned argIndex = builtinToArgIndex.at(callInst.getCalledFunction()->getName().str());
    auto* ptr = m_builder->CreateGEP(rayQueryTy, rayQuery, { m_builder->getInt32(0), m_builder->getInt32(argIndex) });
    auto* queriedValue = m_builder->CreateLoad(cast<llvm::GetElementPtrInst>(ptr)->getResultElementType(), ptr);

    callInst.replaceAllUsesWith(queriedValue);
    callInst.eraseFromParent();
}

// ---- Helper functions ----

Value* ResolveOCLRaytracingBuiltins::getIntrinsicValue(GenISAIntrinsic::ID intrinsicId, ArrayRef<Value*> args) {
  std::vector<Type*> types;
  if (!args.empty()) {
    std::transform(args.begin(), args.end(), std::back_inserter(types), [](Value* v) { return v->getType(); });
  }
  return m_builder->CreateCall(GenISAIntrinsic::getDeclaration(m_builder->GetInsertPoint()->getModule(), intrinsicId, types), args);
}

