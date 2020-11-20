/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncsAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-wi-func-resolution"
#define PASS_DESCRIPTION "Resolves work item functions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(WIFuncResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(WIFuncResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char WIFuncResolution::ID = 0;

WIFuncResolution::WIFuncResolution() : FunctionPass(ID), m_implicitArgs()
{
    initializeWIFuncResolutionPass(*PassRegistry::getPassRegistry());
}

Constant* WIFuncResolution::getKnownWorkGroupSize(
    IGCMD::MetaDataUtils* MDUtils, llvm::Function& F) const
{
    auto finfo = MDUtils->findFunctionsInfoItem(&F);
    if (finfo == MDUtils->end_FunctionsInfo())
        return nullptr;

    auto& FI = finfo->second;
    if (FI->getThreadGroupSize()->hasValue())
    {
        uint32_t Dims[] =
        {
            (uint32_t)FI->getThreadGroupSize()->getXDim(),
            (uint32_t)FI->getThreadGroupSize()->getYDim(),
            (uint32_t)FI->getThreadGroupSize()->getZDim(),
        };
        return ConstantDataVector::get(F.getContext(), Dims);
    }

    return nullptr;
}

bool WIFuncResolution::runOnFunction(Function& F)
{
    m_changed = false;
    auto* MDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    m_implicitArgs = ImplicitArgs(F, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());
    visit(F);

    /// If the work group size is known at compile time, emit it as a
    /// literal rather than reading from the payload.
    if (Constant * KnownWorkGroupSize = getKnownWorkGroupSize(MDUtils, F))
    {
        if (auto * Arg = m_implicitArgs.getImplicitArg(F, ImplicitArg::ENQUEUED_LOCAL_WORK_SIZE))
            Arg->replaceAllUsesWith(KnownWorkGroupSize);
    }

    return m_changed;
}

void WIFuncResolution::visitCallInst(CallInst& CI)
{
    if (!CI.getCalledFunction())
    {
        return;
    }

    Value* wiRes = nullptr;

    // Add appropriate sequence and handle out of range where needed
    StringRef funcName = CI.getCalledFunction()->getName();

    if (funcName.equals(WIFuncsAnalysis::GET_LOCAL_ID_X))
    {
        wiRes = getLocalId(CI, ImplicitArg::LOCAL_ID_X);
    }
    else if (funcName.equals(WIFuncsAnalysis::GET_LOCAL_ID_Y))
    {
        wiRes = getLocalId(CI, ImplicitArg::LOCAL_ID_Y);
    }
    else if (funcName.equals(WIFuncsAnalysis::GET_LOCAL_ID_Z))
    {
        wiRes = getLocalId(CI, ImplicitArg::LOCAL_ID_Z);
    }
    else if (funcName.equals(WIFuncsAnalysis::GET_GROUP_ID))
    {
        wiRes = getGroupId(CI);
    }
    else if (funcName.equals(WIFuncsAnalysis::GET_GLOBAL_SIZE))
    {
        wiRes = getGlobalSize(CI);
    }
    else if (funcName.equals(WIFuncsAnalysis::GET_LOCAL_SIZE))
    {
        wiRes = getLocalSize(CI);
    }
    else if (funcName.equals(WIFuncsAnalysis::GET_ENQUEUED_LOCAL_SIZE)) {
        wiRes = getEnqueuedLocalSize(CI);
    }
    else if (funcName.equals(WIFuncsAnalysis::GET_GLOBAL_OFFSET))
    {
        wiRes = getGlobalOffset(CI);
    }
    else if (funcName.equals(WIFuncsAnalysis::GET_WORK_DIM))
    {
        wiRes = getWorkDim(CI);
    }
    else if (funcName.equals(WIFuncsAnalysis::GET_NUM_GROUPS))
    {
        wiRes = getNumGroups(CI);
    }
    else if (funcName.equals(WIFuncsAnalysis::GET_STAGE_IN_GRID_ORIGIN))
    {
        wiRes = getStageInGridOrigin(CI);
    }
    else if (funcName.equals(WIFuncsAnalysis::GET_STAGE_IN_GRID_SIZE))
    {
        wiRes = getStageInGridSize(CI);
    }
    else if (funcName.equals(WIFuncsAnalysis::GET_SYNC_BUFFER))
    {
        wiRes = getSyncBufferPtr(CI);
    }
    else
    {
        // Non WI function, do nothing
        return;
    }

    // Handle size_t return type for 64 bits
    if (wiRes->getType()->getScalarSizeInBits() < CI.getType()->getScalarSizeInBits())
    {
        CastInst* pCast = CastInst::Create(Instruction::ZExt, wiRes, IntegerType::get(CI.getContext(), CI.getType()->getScalarSizeInBits()), wiRes->getName(), &CI);
        updateDebugLoc(&CI, pCast);
        wiRes = pCast;
    }

    // Replace original WI call instruction by the result of the appropriate sequence
    CI.replaceAllUsesWith(wiRes);
    CI.eraseFromParent();

    m_changed = true;
}

/************************************************************************************************

R0:

 -----------------------------------------------------------------------------------------------
| Local mem | Group     | Barrier ID| Sampler   | Binding   | Scratch   | Group     | Group     |
| mem index/| number    | /Interface| state     | table     | space     | number    | number    |
| URB handle| X         | descriptor| pointer   | pointer   | pointer   | Y         | Z         |
|           | 32bit     | offset    |           |           |           | 32bit     | 32bit     |
 -----------------------------------------------------------------------------------------------
 <low>                                                                                     <high>


 PayloadHeader:

-----------------------------------------------------------------------------------------------
| Global    | Global    | Global    | Local     | Local     | Local     | Reserved  | Num       |
| offset    | offset    | offset    | size      | size      | size      |           | HW        |
| X         | Y         | Z         | X         | Y         | Z         |           | Threads   |
| 32bit     | 32bit     | 32bit     | 32bit     | 32bit     | 32bit     |           | 32bit     |
 -----------------------------------------------------------------------------------------------
 <low>                                                                                     <high>

*************************************************************************************************/

// Structure of side buffer generated by NEO:
//
// struct local_id_s {
//   uint16_t lx[simd-width];
//   uint16_t ly[simd-width];
//   uint16_t lz[simd-width];
// };
//
// struct arg_s {
//   uint32_t global_offset_x;
//   uint32_t global_offset_y;
//   uint32_t global_offset_z;
//   uint32_t global_size_x;
//   uint32_t global_size_y;
//   uint32_t global_size_z;
//   uint32_t num_hw_threads;
//   uint32_t num_work_dim;
//   uint64_t printf_buffer_ptr;
//   uint32_t local_size_x;
//   uint32_t local_size_y;
//   uint32_t local_size_z;
//   uint32_t enqueued_local_size_x;
//   uint32_t enqueued_local_size_y;
//   uint32_t enqueued_local_size_z;
//   uint32_t unused_fields[8]; -- for future expansion
//   struct local_id_s local_id[thread-group-size];
// }

class GLOBAL_STATE_FIELD_OFFSETS
{
public:
    // This class holds offsets of various fields in side buffer
    static const uint32_t GLOBAL_OFFSETS = 0;
    static const uint32_t GLOBAL_OFFSET_X = GLOBAL_OFFSETS;
    static const uint32_t GLOBAL_OFFSET_Y = GLOBAL_OFFSET_X + sizeof(uint32_t);
    static const uint32_t GLOBAL_OFFSET_Z = GLOBAL_OFFSET_Y + sizeof(uint32_t);

    static const uint32_t GLOBAL_SIZES = GLOBAL_OFFSET_Z + sizeof(uint32_t);
    static const uint32_t GLOBAL_SIZE_X = GLOBAL_SIZES;
    static const uint32_t GLOBAL_SIZE_Y = GLOBAL_SIZE_X + sizeof(uint32_t);
    static const uint32_t GLOBAL_SIZE_Z = GLOBAL_SIZE_Y + sizeof(uint32_t);

    static const uint32_t NUM_HW_THREADS = GLOBAL_SIZE_Z + sizeof(uint32_t);
    static const uint32_t NUM_WORK_DIM = NUM_HW_THREADS + sizeof(uint32_t);
    static const uint32_t PRINTF_BUFFER = NUM_WORK_DIM + sizeof(uint32_t);

    static const uint32_t LOCAL_SIZES = PRINTF_BUFFER + sizeof(uint64_t);
    static const uint32_t LOCAL_SIZE_X = LOCAL_SIZES;
    static const uint32_t LOCAL_SIZE_Y = LOCAL_SIZE_X + sizeof(uint32_t);
    static const uint32_t LOCAL_SIZE_Z = LOCAL_SIZE_Y + sizeof(uint32_t);

    static const uint32_t ENQUEUED_LOCAL_SIZES = LOCAL_SIZE_Z + sizeof(uint32_t);
    static const uint32_t ENQUEUED_LOCAL_SIZE_X = ENQUEUED_LOCAL_SIZES;
    static const uint32_t ENQUEUED_LOCAL_SIZE_Y = ENQUEUED_LOCAL_SIZE_X + sizeof(uint32_t);
    static const uint32_t ENQUEUED_LOCAL_SIZE_Z = ENQUEUED_LOCAL_SIZE_Y + sizeof(uint32_t);

    static const uint32_t UNUSED_FIELD1 = ENQUEUED_LOCAL_SIZE_Z + sizeof(uint32_t);
    static const uint32_t UNUSED_FIELD2 = UNUSED_FIELD1 + sizeof(uint32_t);
    static const uint32_t UNUSED_FIELD3 = UNUSED_FIELD2 + sizeof(uint32_t);
    static const uint32_t UNUSED_FIELD4 = UNUSED_FIELD3 + sizeof(uint32_t);
    static const uint32_t UNUSED_FIELD5 = UNUSED_FIELD4 + sizeof(uint32_t);
    static const uint32_t UNUSED_FIELD6 = UNUSED_FIELD5 + sizeof(uint32_t);
    static const uint32_t UNUSED_FIELD7 = UNUSED_FIELD6 + sizeof(uint32_t);
    static const uint32_t UNUSED_FIELD8 = UNUSED_FIELD7 + sizeof(uint32_t);

    static const uint32_t LOCAL_IDS = UNUSED_FIELD8 + sizeof(uint32_t);
};

static bool hasStackCallAttr(const llvm::Function& F)
{
    return F.hasFnAttribute("visaStackCall");
}

Value* WIFuncResolution::getLocalId(CallInst& CI, ImplicitArg::ArgType argType)
{
    // Receives:
    // call i32 @__builtin_IB_get_local_id_x()

    // Creates:
    // %localIdX

    Value* V = nullptr;
    auto F = CI.getFunction();
    if (hasStackCallAttr(*F))
    {
    }
    else
    {
        Argument* localId = getImplicitArg(CI, argType);
        V = localId;
    }

    return V;
}

Value* WIFuncResolution::getGroupId(CallInst& CI)
{
    // Receives:
    // call i32 @__builtin_IB_get_group_id(i32 %dim)

    // Creates:
    // %cmpDim = icmp eq i32 %dim, 0
    // %tmpOffsetR0 = select i1 %cmpDim, i32 1, i32 5
    // %offsetR0 = add i32 %dim, %tmpOffsetR0
    // %groupId = extractelement <8 x i32> %r0, i32 %offsetR0

    // The cmp select insts are present because:
    // if dim = 0 then we need to access R0.1
    // if dim = 1 then we need to access R0.6
    // if dim = 2 then we need to access R0.7

    Value* V = nullptr;
    auto F = CI.getFunction();
    if (hasStackCallAttr(*F))
    {
        auto pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        llvm::IRBuilder<> Builder(&CI);
        Type* int32Ty = Type::getInt32Ty(F->getParent()->getContext());
        VectorType* Tys = VectorType::get(int32Ty, pCtx->platform.getGRFSize() / SIZE_DWORD);
        Function* getR0 = GenISAIntrinsic::getDeclaration(F->getParent(), GenISAIntrinsic::ID::GenISA_getR0, Tys);
        auto intCall = Builder.CreateCall(getR0);
        V = intCall;
    }
    else
    {
        Argument* arg = getImplicitArg(CI, ImplicitArg::R0);
        V = arg;
    }

    Value* dim = CI.getArgOperand(0);
    Instruction* cmpDim = CmpInst::Create(Instruction::ICmp, CmpInst::ICMP_EQ, dim, ConstantInt::get(Type::getInt32Ty(CI.getContext()), 0), "cmpDim", &CI);
    Instruction* offsetR0 = SelectInst::Create(cmpDim, ConstantInt::get(Type::getInt32Ty(CI.getContext()), 1), ConstantInt::get(Type::getInt32Ty(CI.getContext()), 5), "tmpOffsetR0", &CI);
    Instruction* index = BinaryOperator::CreateAdd(dim, offsetR0, "offsetR0", &CI);
    Instruction* groupId = ExtractElementInst::Create(V, index, "groupId", &CI);
    updateDebugLoc(&CI, cmpDim);
    updateDebugLoc(&CI, offsetR0);
    updateDebugLoc(&CI, index);
    updateDebugLoc(&CI, groupId);

    return groupId;
}

Value* WIFuncResolution::getGlobalSize(CallInst& CI)
{
    // Receives:
    // call i32 @__builtin_IB_get_global_size(i32 %dim)

    // Creates:
    // %globalSize1 = extractelement <3 x i32> %globalSize, i32 %dim

    Value* V = nullptr;
    auto F = CI.getFunction();
    if (hasStackCallAttr(*F))
    {
        llvm::IRBuilder<> Builder(&CI);
        auto intPtrTy = Type::getInt32PtrTy(F->getParent()->getContext(), ADDRESS_SPACE_GLOBAL);
        auto getSidePtr = GenISAIntrinsic::getDeclaration(F->getParent(), GenISAIntrinsic::ID::GenISA_getGlobalStateBufferPtr, intPtrTy);
        auto intCall = Builder.CreateCall(getSidePtr);
        auto int64Type = Type::getInt64Ty(F->getParent()->getContext());
        auto intCast = Builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, intCall, int64Type);
        auto addInst = Builder.CreateAdd(intCast, ConstantInt::get(int64Type, GLOBAL_STATE_FIELD_OFFSETS::GLOBAL_SIZES));
        auto ptrCast = Builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, addInst, intPtrTy);
        auto int32Ty = Type::getInt32Ty(F->getParent()->getContext());
        auto intVec3x32 = VectorType::get(int32Ty, 3);
        auto intVec3x32Ptr = PointerType::get(intVec3x32, ADDRESS_SPACE_GLOBAL);
        auto bitCast = Builder.CreateBitCast(ptrCast, intVec3x32Ptr);
        auto loadInst = Builder.CreateLoad(bitCast);
        V = loadInst;
    }
    else
    {
        Argument* arg = getImplicitArg(CI, ImplicitArg::GLOBAL_SIZE);
        V = arg;
    }

    Value* dim = CI.getArgOperand(0);
    Instruction* globalSize = ExtractElementInst::Create(V, dim, "globalSize", &CI);
    updateDebugLoc(&CI, globalSize);

    return globalSize;
}

Value* WIFuncResolution::getLocalSize(CallInst& CI)
{
    // Receives:
    // call i32 @__builtin_IB_get_local_size(i32 %dim)

    // Creates:
    // %localSize = extractelement <3 x i32> %localSize, i32 %dim

    Value* V = nullptr;
    auto F = CI.getFunction();
    if (hasStackCallAttr(*F))
    {
        llvm::IRBuilder<> Builder(&CI);
        auto intPtrTy = Type::getInt32PtrTy(F->getParent()->getContext(), ADDRESS_SPACE_GLOBAL);
        auto getSidePtr = GenISAIntrinsic::getDeclaration(F->getParent(), GenISAIntrinsic::ID::GenISA_getGlobalStateBufferPtr, intPtrTy);
        auto intCall = Builder.CreateCall(getSidePtr);
        auto int64Type = Type::getInt64Ty(F->getParent()->getContext());
        auto intCast = Builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, intCall, int64Type);
        auto addInst = Builder.CreateAdd(intCast, ConstantInt::get(int64Type, GLOBAL_STATE_FIELD_OFFSETS::LOCAL_SIZES));
        auto ptrCast = Builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, addInst, intPtrTy);
        auto int32Ty = Type::getInt32Ty(F->getParent()->getContext());
        auto intVec3x32 = VectorType::get(int32Ty, 3);
        auto intVec3x32Ptr = PointerType::get(intVec3x32, ADDRESS_SPACE_GLOBAL);
        auto bitCast = Builder.CreateBitCast(ptrCast, intVec3x32Ptr);
        auto loadInst = Builder.CreateLoad(bitCast);
        V = loadInst;
    }
    else
    {
        Argument* arg = getImplicitArg(CI, ImplicitArg::LOCAL_SIZE);
        V = arg;
    }

    Value* dim = CI.getArgOperand(0);
    Instruction* localSize = ExtractElementInst::Create(V, dim, "localSize", &CI);
    updateDebugLoc(&CI, localSize);

    return localSize;
}

Value* WIFuncResolution::getEnqueuedLocalSize(CallInst& CI) {
    // Receives:
    // call i32 @__builtin_IB_get_enqueued_local_size(i32 %dim)

    // Creates:
    // %enqueuedLocalSize1 = extractelement <3 x i32> %enqueuedLocalSize, %dim

    Value* V = nullptr;
    auto F = CI.getFunction();
    if (hasStackCallAttr(*F))
    {
        llvm::IRBuilder<> Builder(&CI);
        auto intPtrTy = Type::getInt32PtrTy(F->getParent()->getContext(), ADDRESS_SPACE_GLOBAL);
        auto getSidePtr = GenISAIntrinsic::getDeclaration(F->getParent(), GenISAIntrinsic::ID::GenISA_getGlobalStateBufferPtr, intPtrTy);
        auto intCall = Builder.CreateCall(getSidePtr);
        auto int64Type = Type::getInt64Ty(F->getParent()->getContext());
        auto intCast = Builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, intCall, int64Type);
        auto addInst = Builder.CreateAdd(intCast, ConstantInt::get(int64Type, GLOBAL_STATE_FIELD_OFFSETS::ENQUEUED_LOCAL_SIZES));
        auto ptrCast = Builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, addInst, intPtrTy);
        auto int32Ty = Type::getInt32Ty(F->getParent()->getContext());
        auto intVec3x32 = VectorType::get(int32Ty, 3);
        auto intVec3x32Ptr = PointerType::get(intVec3x32, ADDRESS_SPACE_GLOBAL);
        auto bitCast = Builder.CreateBitCast(ptrCast, intVec3x32Ptr);
        auto loadInst = Builder.CreateLoad(bitCast);
        V = loadInst;
    }
    else
    {
        Argument* arg = getImplicitArg(CI, ImplicitArg::ENQUEUED_LOCAL_WORK_SIZE);
        V = arg;
    }

    Value* dim = CI.getArgOperand(0);
    Instruction* enqueuedLocalSize = ExtractElementInst::Create(V, dim, "enqueuedLocalSize", &CI);
    updateDebugLoc(&CI, enqueuedLocalSize);

    return enqueuedLocalSize;
}

Value* WIFuncResolution::getGlobalOffset(CallInst& CI)
{
    // Receives:
    // call i32 @__builtin_IB_get_global_offset(i32 %dim)

    // Creates:
    // %globalOffset = extractelement <8 x i32> %payloadHeader, i32 %dim

    Value* V = nullptr;
    auto F = CI.getFunction();
    if (hasStackCallAttr(*F))
    {
        llvm::IRBuilder<> Builder(&CI);
        auto intPtrTy = Type::getInt32PtrTy(F->getParent()->getContext(), ADDRESS_SPACE_GLOBAL);
        auto getSidePtr = GenISAIntrinsic::getDeclaration(F->getParent(), GenISAIntrinsic::ID::GenISA_getGlobalStateBufferPtr, intPtrTy);
        auto intCall = Builder.CreateCall(getSidePtr);
        auto int32Ty = Type::getInt32Ty(F->getParent()->getContext());
        auto intVec3x32 = VectorType::get(int32Ty, 3);
        auto intVec3x32Ptr = PointerType::get(intVec3x32, ADDRESS_SPACE_GLOBAL);
        auto bitCast = Builder.CreateBitCast(intCall, intVec3x32Ptr);
        auto loadInst = Builder.CreateLoad(bitCast);
        V = loadInst;
    }
    else
    {
        Argument* arg = getImplicitArg(CI, ImplicitArg::PAYLOAD_HEADER);
        V = arg;
    }

    Value* dim = CI.getArgOperand(0);
    auto globalOffset = ExtractElementInst::Create(V, dim, "globalOffset", &CI);
    updateDebugLoc(&CI, cast<Instruction>(globalOffset));

    return globalOffset;
}

Value* WIFuncResolution::getWorkDim(CallInst& CI)
{
    // Receives:
    // call i32 @__builtin_IB_get_work_dim()

    // Creates:
    // %workDim

    Value* V = nullptr;
    auto F = CI.getFunction();
    if (hasStackCallAttr(*F))
    {
        llvm::IRBuilder<> Builder(&CI);
        auto intPtrTy = Type::getInt32PtrTy(F->getParent()->getContext(), ADDRESS_SPACE_GLOBAL);
        auto getSidePtr = GenISAIntrinsic::getDeclaration(F->getParent(), GenISAIntrinsic::ID::GenISA_getGlobalStateBufferPtr, intPtrTy);
        auto intCall = Builder.CreateCall(getSidePtr);
        auto int64Type = Type::getInt64Ty(F->getParent()->getContext());
        auto intCast = Builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, intCall, int64Type);
        auto addInst = Builder.CreateAdd(intCast, ConstantInt::get(int64Type, GLOBAL_STATE_FIELD_OFFSETS::NUM_WORK_DIM));
        auto ptrCast = Builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, addInst, intPtrTy);
        auto loadInst = Builder.CreateLoad(ptrCast);
        V = loadInst;
    }
    else
    {
        Argument* workDim = getImplicitArg(CI, ImplicitArg::WORK_DIM);
        V = workDim;
    }

    return V;
}

Value* WIFuncResolution::getNumGroups(CallInst& CI)
{
    // Receives:
    // call i32 @__builtin_IB_get_num_groups(i32 %dim)

    // Creates:
    // %numGroups1 = extractelement <3 x i32> %numGroups, i32 %dim

    auto F = CI.getFunction();
    if (hasStackCallAttr(*F))
    {
        return getLocalSize(CI);
    }

    Argument* arg = getImplicitArg(CI, ImplicitArg::NUM_GROUPS);

    Value* dim = CI.getArgOperand(0);
    Instruction* numGroups = ExtractElementInst::Create(arg, dim, "numGroups", &CI);
    updateDebugLoc(&CI, numGroups);

    return numGroups;
}

Value* WIFuncResolution::getStageInGridOrigin(CallInst& CI)
{
    // Receives:
    // call i32 @__builtin_IB_get_grid_origin(i32 %dim)

    // Creates:
    // %grid_origin1 = extractelement <3 x i32> %globalSize, i32 %dim

    Argument* arg = getImplicitArg(CI, ImplicitArg::STAGE_IN_GRID_ORIGIN);

    Value* dim = CI.getArgOperand(0);
    Instruction* globalSize = ExtractElementInst::Create(arg, dim, "grid_origin", &CI);
    updateDebugLoc(&CI, globalSize);

    return globalSize;
}

Value* WIFuncResolution::getStageInGridSize(CallInst& CI)
{
    // Receives:
    // call i32 @__builtin_IB_get_grid_size(i32 %dim)

    // Creates:
    // %grid_size1 = extractelement <3 x i32> %globalSize, i32 %dim

    Value* V = nullptr;
    auto F = CI.getFunction();
    if (hasStackCallAttr(*F))
    {
        llvm::IRBuilder<> Builder(&CI);
        auto intPtrTy = Type::getInt32PtrTy(F->getParent()->getContext(), ADDRESS_SPACE_GLOBAL);
        auto getSidePtr = GenISAIntrinsic::getDeclaration(F->getParent(), GenISAIntrinsic::ID::GenISA_getGlobalStateBufferPtr, intPtrTy);
        auto intCall = Builder.CreateCall(getSidePtr);
        auto int64Type = Type::getInt64Ty(F->getParent()->getContext());
        auto intCast = Builder.CreateCast(llvm::Instruction::CastOps::PtrToInt, intCall, int64Type);
        auto addInst = Builder.CreateAdd(intCast, ConstantInt::get(int64Type, GLOBAL_STATE_FIELD_OFFSETS::GLOBAL_SIZES));
        auto ptrCast = Builder.CreateCast(llvm::Instruction::CastOps::IntToPtr, addInst, intPtrTy);
        auto int32Ty = Type::getInt32Ty(F->getParent()->getContext());
        auto intVec3x32 = VectorType::get(int32Ty, 3);
        auto intVec3x32Ptr = PointerType::get(intVec3x32, ADDRESS_SPACE_GLOBAL);
        auto bitCast = Builder.CreateBitCast(ptrCast, intVec3x32Ptr);
        auto loadInst = Builder.CreateLoad(bitCast);
        V = loadInst;
    }
    else
    {
        Argument* arg = getImplicitArg(CI, ImplicitArg::STAGE_IN_GRID_SIZE);
        V = arg;
    }

    Value* dim = CI.getArgOperand(0);
    Instruction* globalSize = ExtractElementInst::Create(V, dim, "grid_size", &CI);
    updateDebugLoc(&CI, globalSize);

    return globalSize;
}

Value* WIFuncResolution::getSyncBufferPtr(CallInst& CI)
{
    // Receives:
    // call i8 addrspace(1)* @__builtin_IB_get_sync_buffer()

    // Creates:
    // i8 addrspace(1)* %syncBuffer

    Argument* syncBuffer = getImplicitArg(CI, ImplicitArg::SYNC_BUFFER);

    return syncBuffer;
}

Argument* WIFuncResolution::getImplicitArg(CallInst& CI, ImplicitArg::ArgType argType)
{
    unsigned int numImplicitArgs = m_implicitArgs.size();
    unsigned int implicitArgIndex = m_implicitArgs.getArgIndex(argType);

    Function* pFunc = CI.getParent()->getParent();
    unsigned int implicitArgIndexInFunc = pFunc->arg_size() - numImplicitArgs + implicitArgIndex;

    Function::arg_iterator arg = pFunc->arg_begin();
    for (unsigned int i = 0; i < implicitArgIndexInFunc; ++i, ++arg);

    return &(*arg);
}
