/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncsAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/IRBuilder.h>
#include "Probe/Assertion.h"
#include <llvmWrapper/Support/Alignment.h>
#include <llvmWrapper/IR/DerivedTypes.h>

#include <cstddef>

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
IGC_INITIALIZE_PASS_DEPENDENCY(GenXFunctionGroupAnalysis)
IGC_INITIALIZE_PASS_END(WIFuncResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char WIFuncResolution::ID = 0;

WIFuncResolution::WIFuncResolution() : FunctionPass(ID), m_implicitArgs() {
  initializeWIFuncResolutionPass(*PassRegistry::getPassRegistry());
}

void WIFuncResolution::storeImplicitBufferPtrs(llvm::Function &F) {
  CodeGenContext *m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  if (isEntryFunc(m_pMdUtils, &F) && !m_ctx->platform.isProductChildOf(IGFX_XE_HP_SDV)) {
    if (m_implicitArgs.isImplicitArgExist(ImplicitArg::ArgType::IMPLICIT_ARG_BUFFER_PTR)) {
      IGCLLVM::IRBuilder<> Builder(&(*F.getEntryBlock().getFirstInsertionPt()));

      auto BufferPtr = m_implicitArgs.getImplicitArgValue(F, ImplicitArg::ArgType::IMPLICIT_ARG_BUFFER_PTR, m_pMdUtils);

      // create intrinsic to store implicit arg buffer ptr
      auto *M = F.getParent();
      llvm::SmallVector<llvm::Type *, 1> Type;
      Type.push_back(BufferPtr->getType());
      auto *ImplArgFunc = GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_SetImplicitBufferPtr, Type);
      llvm::SmallVector<llvm::Value *, 1> Args = {BufferPtr};
      auto StoreIntrinsic = Builder.CreateCall(ImplArgFunc, Args);
      StoreIntrinsic->setDebugLoc(DebugLoc());

      auto &C = F.getParent()->getContext();
      auto LocalIdX = m_implicitArgs.getImplicitArgValue(F, ImplicitArg::ArgType::LOCAL_ID_X, m_pMdUtils);
      auto LocalIdY = m_implicitArgs.getImplicitArgValue(F, ImplicitArg::ArgType::LOCAL_ID_Y, m_pMdUtils);
      auto LocalIdZ = m_implicitArgs.getImplicitArgValue(F, ImplicitArg::ArgType::LOCAL_ID_Z, m_pMdUtils);

      auto DataTypeI16 = Type::getInt16Ty(C);
      auto AllocaVec = Builder.CreateAlloca(DataTypeI16, ConstantInt::get(DataTypeI16, (uint64_t)3));
      auto FirstSlot = Builder.CreatePointerCast(AllocaVec, DataTypeI16->getPointerTo());
      Builder.CreateStore(LocalIdX, FirstSlot, true);
      auto SecondSlot = Builder.CreatePtrToInt(FirstSlot, Type::getInt64Ty(C));
      SecondSlot = Builder.CreateAdd(SecondSlot, ConstantInt::get(SecondSlot->getType(), (uint64_t)2));
      SecondSlot = Builder.CreateIntToPtr(SecondSlot, DataTypeI16->getPointerTo());
      Builder.CreateStore(LocalIdY, SecondSlot, true);
      auto ThirdSlot = Builder.CreatePtrToInt(FirstSlot, Type::getInt64Ty(C));
      ThirdSlot = Builder.CreateAdd(ThirdSlot, ConstantInt::get(ThirdSlot->getType(), (uint64_t)4));
      ThirdSlot = Builder.CreateIntToPtr(ThirdSlot, DataTypeI16->getPointerTo());
      Builder.CreateStore(LocalIdZ, ThirdSlot, true);

      auto *LidFunc =
          GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_SetLocalIdBufferPtr, AllocaVec->getType());
      Args = {AllocaVec};
      StoreIntrinsic = Builder.CreateCall(LidFunc, Args);
      StoreIntrinsic->setDebugLoc(DebugLoc());
    }
  }
}

bool WIFuncResolution::runOnFunction(Function &F) {
  m_changed = false;
  m_pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  m_implicitArgs = ImplicitArgs(F, m_pMdUtils);

  visit(F);

  storeImplicitBufferPtrs(F);

  return m_changed;
}

void WIFuncResolution::visitCallInst(CallInst &CI) {
  if (!CI.getCalledFunction()) {
    return;
  }

  Value *wiRes = nullptr;

  // Add appropriate sequence and handle out of range where needed
  StringRef funcName = CI.getCalledFunction()->getName();

  if (funcName.equals(WIFuncsAnalysis::GET_LOCAL_ID_X)) {
    wiRes = getLocalId(CI, ImplicitArg::LOCAL_ID_X);
  } else if (funcName.equals(WIFuncsAnalysis::GET_LOCAL_ID_Y)) {
    wiRes = getLocalId(CI, ImplicitArg::LOCAL_ID_Y);
  } else if (funcName.equals(WIFuncsAnalysis::GET_LOCAL_ID_Z)) {
    wiRes = getLocalId(CI, ImplicitArg::LOCAL_ID_Z);
  } else if (funcName.equals(WIFuncsAnalysis::GET_GROUP_ID)) {
    wiRes = getGroupId(CI);
  } else if (funcName.equals(WIFuncsAnalysis::GET_LOCAL_THREAD_ID)) {
    wiRes = getLocalThreadId(CI);
  } else if (funcName.equals(WIFuncsAnalysis::GET_GLOBAL_SIZE)) {
    wiRes = getGlobalSize(CI);
  } else if (funcName.equals(WIFuncsAnalysis::GET_LOCAL_SIZE)) {
    wiRes = getLocalSize(CI);
  } else if (funcName.equals(WIFuncsAnalysis::GET_ENQUEUED_LOCAL_SIZE)) {
    wiRes = getEnqueuedLocalSize(CI);
  } else if (funcName.equals(WIFuncsAnalysis::GET_GLOBAL_OFFSET)) {
    wiRes = getGlobalOffset(CI);
  } else if (funcName.equals(WIFuncsAnalysis::GET_WORK_DIM)) {
    wiRes = getWorkDim(CI);
  } else if (funcName.equals(WIFuncsAnalysis::GET_NUM_GROUPS)) {
    wiRes = getNumGroups(CI);
  } else if (funcName.equals(WIFuncsAnalysis::GET_STAGE_IN_GRID_ORIGIN)) {
    wiRes = getStageInGridOrigin(CI);
  } else if (funcName.equals(WIFuncsAnalysis::GET_STAGE_IN_GRID_SIZE)) {
    wiRes = getStageInGridSize(CI);
  } else if (funcName.equals(WIFuncsAnalysis::GET_SYNC_BUFFER)) {
    wiRes = getSyncBufferPtr(CI);
  } else if (funcName.equals(WIFuncsAnalysis::GET_ASSERT_BUFFER)) {
    wiRes = getAssertBufferPtr(CI);
  } else if (funcName.equals(WIFuncsAnalysis::GET_REGION_GROUP_SIZE)) {
    wiRes = getRegionGroupSize(CI);
  } else if (funcName.equals(WIFuncsAnalysis::GET_REGION_GROUP_WG_COUNT)) {
    wiRes = getRegionGroupWGCount(CI);
  } else if (funcName.equals(WIFuncsAnalysis::GET_REGION_GROUP_BARRIER_BUFFER)) {
    wiRes = getRegionGroupBarrierBufferPtr(CI);
  } else {
    // Non WI function, do nothing
    return;
  }

  // Handle size_t return type for 64 bits
  if (wiRes && wiRes->getType()->getScalarSizeInBits() < CI.getType()->getScalarSizeInBits()) {
    CastInst *pCast =
        CastInst::Create(Instruction::ZExt, wiRes,
                         IntegerType::get(CI.getContext(), CI.getType()->getScalarSizeInBits()), wiRes->getName(), &CI);
    updateDebugLoc(&CI, pCast);
    wiRes = pCast;
  }

  // Replace original WI call instruction by the result of the appropriate sequence
  if (wiRes) {
    CI.replaceAllUsesWith(wiRes);
  }
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
 (Note: PayloadHeader uses 8xi32, but only 3xi32 are used. Unused bytes can be removed.)

-----------------------------------------------------------------------------------------------
| Global    | Global    | Global    | Local     | Local     | Local     | Reserved  | Num       |
| offset    | offset    | offset    | size      | size      | size      |           | HW        |
| X         | Y         | Z         | X         | Y         | Z         |           | Threads   |
| 32bit     | 32bit     | 32bit     | 32bit     | 32bit     | 32bit     |           | 32bit     |
 -----------------------------------------------------------------------------------------------
 <low>                                                                                     <high>

*************************************************************************************************/

#pragma pack(push, 1)
namespace packed {
#include "implicit_args_struct.h"
}
#pragma pack(pop)

#undef IMPLICIT_ARGS_STRUCT_H_
#include "implicit_args_struct.h"

// According to the ABI specification, implicit_args struct must be naturally aligned.
// To ensure that offsets to struct members are compiler-independent, it is necessary to
// align struct member offset to be divisible by the size of the member. It implicates
// inserting an additional padding member in some cases.
static_assert(sizeof(packed::implicit_args) == sizeof(implicit_args), "Implicit args struct is not properly aligned!");
static_assert(offsetof(packed::implicit_args, struct_size) == offsetof(implicit_args, struct_size),
              "Implicit args struct is not properly aligned for struct_size member!");
static_assert(offsetof(packed::implicit_args, struct_version) == offsetof(implicit_args, struct_version),
              "Implicit args struct is not properly aligned for struct_version member!");
static_assert(offsetof(packed::implicit_args, num_work_dim) == offsetof(implicit_args, num_work_dim),
              "Implicit args struct is not properly aligned for num_work_dim member!");
static_assert(offsetof(packed::implicit_args, simd_width) == offsetof(implicit_args, simd_width),
              "Implicit args struct is not properly aligned for simd_width member!");
static_assert(offsetof(packed::implicit_args, local_size_x) == offsetof(implicit_args, local_size_x),
              "Implicit args struct is not properly aligned for local_size_x member!");
static_assert(offsetof(packed::implicit_args, local_size_y) == offsetof(implicit_args, local_size_y),
              "Implicit args struct is not properly aligned for local_size_y member!");
static_assert(offsetof(packed::implicit_args, local_size_z) == offsetof(implicit_args, local_size_z),
              "Implicit args struct is not properly aligned for local_size_z member!");
static_assert(offsetof(packed::implicit_args, global_size_x) == offsetof(implicit_args, global_size_x),
              "Implicit args struct is not properly aligned for global_size_x member!");
static_assert(offsetof(packed::implicit_args, global_size_y) == offsetof(implicit_args, global_size_y),
              "Implicit args struct is not properly aligned for global_size_y member!");
static_assert(offsetof(packed::implicit_args, global_size_z) == offsetof(implicit_args, global_size_z),
              "Implicit args struct is not properly aligned for global_size_z member!");
static_assert(offsetof(packed::implicit_args, printf_buffer_ptr) == offsetof(implicit_args, printf_buffer_ptr),
              "Implicit args struct is not properly aligned for printf_buffer_ptr member!");
static_assert(offsetof(packed::implicit_args, global_offset_x) == offsetof(implicit_args, global_offset_x),
              "Implicit args struct is not properly aligned for global_offset_x member!");
static_assert(offsetof(packed::implicit_args, global_offset_y) == offsetof(implicit_args, global_offset_y),
              "Implicit args struct is not properly aligned for global_offset_y member!");
static_assert(offsetof(packed::implicit_args, global_offset_z) == offsetof(implicit_args, global_offset_z),
              "Implicit args struct is not properly aligned for global_offset_z member!");
static_assert(offsetof(packed::implicit_args, local_id_table_ptr) == offsetof(implicit_args, local_id_table_ptr),
              "Implicit args struct is not properly aligned for local_id_table_ptr member!");
static_assert(offsetof(packed::implicit_args, group_count_x) == offsetof(implicit_args, group_count_x),
              "Implicit args struct is not properly aligned for group_count_x member!");
static_assert(offsetof(packed::implicit_args, group_count_y) == offsetof(implicit_args, group_count_y),
              "Implicit args struct is not properly aligned for group_count_y member!");
static_assert(offsetof(packed::implicit_args, group_count_z) == offsetof(implicit_args, group_count_z),
              "Implicit args struct is not properly aligned for group_count_z member!");
static_assert(offsetof(packed::implicit_args, padding0) == offsetof(implicit_args, padding0),
              "Implicit args struct is not properly aligned for padding0 member!");
static_assert(offsetof(packed::implicit_args, rt_global_buffer_ptr) == offsetof(implicit_args, rt_global_buffer_ptr),
              "Implicit args struct is not properly aligned for rt_global_buffer_ptr member!");
static_assert(offsetof(packed::implicit_args, assert_buffer_ptr) == offsetof(implicit_args, assert_buffer_ptr),
              "Implicit args struct is not properly aligned for assert_buffer_ptr member!");
static_assert(offsetof(packed::implicit_args, scratch_ptr) == offsetof(implicit_args, scratch_ptr),
              "Implicit args struct is not properly aligned for scratch_ptr member!");
static_assert(offsetof(packed::implicit_args, region_group_size_x) == offsetof(implicit_args, region_group_size_x),
              "Implicit args struct is not properly aligned for region_group_size_x member!");
static_assert(offsetof(packed::implicit_args, region_group_size_y) == offsetof(implicit_args, region_group_size_y),
              "Implicit args struct is not properly aligned for region_group_size_y member!");
static_assert(offsetof(packed::implicit_args, region_group_size_z) == offsetof(implicit_args, region_group_size_z),
              "Implicit args struct is not properly aligned for region_group_size_z member!");
static_assert(offsetof(packed::implicit_args, region_group_dimension) ==
                  offsetof(implicit_args, region_group_dimension),
              "Implicit args struct is not properly aligned for region_group_dimension member!");
static_assert(offsetof(packed::implicit_args, region_group_wg_count) == offsetof(implicit_args, region_group_wg_count),
              "Implicit args struct is not properly aligned for region_group_wg_count member!");
static_assert(offsetof(packed::implicit_args, region_group_barrier_buffer_ptr) ==
                  offsetof(implicit_args, region_group_barrier_buffer_ptr),
              "Implicit args struct is not properly aligned for region_group_barrier_buffer_ptr member!");

// Structure of side buffer generated by NEO:
// struct implicit_args {
//     uint8_t struct_size;
//     uint8_t struct_version;
//     uint8_t num_work_dim;
//     uint8_t simd_width;
//     uint32_t local_size_x;
//     uint32_t local_size_y;
//     uint32_t local_size_z;
//     uint64_t global_size_x;
//     uint64_t global_size_y;
//     uint64_t global_size_z;
//     uint64_t printf_buffer_ptr;
//     uint64_t global_offset_x;
//     uint64_t global_offset_y;
//     uint64_t global_offset_z;
//     uint64_t local_id_table_ptr;
//     uint32_t group_count_x;
//     uint32_t group_count_y;
//     uint32_t group_count_z;
//     uint32_t padding0;
//     uint64_t rt_global_buffer_ptr;
//     uint64_t assert_buffer_ptr;
// };

// For SIMD8:
// struct local_id_s {
//    uint16_t lx[8];
//    uint16_t reserved[8];
//    uint16_t ly[8];
//    uint16_t reserved[8];
//    uint16_t lz[8];
//    uint16_t reserved[8];
//};

// For SIMD16:
// struct local_id_s {
//    uint16_t lx[16];
//    uint16_t ly[16];
//    uint16_t lz[16];
//};

// For SIMD32:
// struct local_id_s {
//    uint16_t lx[32];
//    uint16_t ly[32];
//    uint16_t lz[32];
//};

llvm::Value *LowerImplicitArgIntrinsics::BuildLoadInst(llvm::CallInst &CI, unsigned int Offset, llvm::Type *DataType) {
  // This function computes type aligned address that includes Offset.
  // Then it loads DataType number of elements from Offset.
  // If Offset is unaligned then it computes aligned offset and loads data.
  // If Offset is unaligned then it copies data to new vector of size <i8 x Size>,
  // bitcasts it to DataType, and returns it.
  // It Offset is aligned, it returns result of LoadInst of type DataType.
  auto ElemByteSize = DataType->getScalarSizeInBits() / 8;
  auto Size = ElemByteSize;
  if (auto DataVecType = dyn_cast<IGCLLVM::FixedVectorType>(DataType)) {
    Size *= (unsigned int)DataVecType->getNumElements();
  }
  unsigned int AlignedOffset = (Offset / ElemByteSize) * ElemByteSize;
  unsigned int LoadByteSize = (Offset == AlignedOffset) ? Size : Size * 2;

  IGCLLVM::IRBuilder<> Builder(&CI);
  unsigned int AddrSpace = ADDRESS_SPACE_GLOBAL;
  if (m_ctx->platform.isProductChildOf(IGFX_XE_HP_SDV)) {
    AddrSpace = ADDRESS_SPACE_THREAD_ARG;
  }

  llvm::Value *LoadedData = nullptr;
  auto F = CI.getFunction();
  auto Int32Ptr = PointerType::get(Type::getInt32Ty(F->getParent()->getContext()), AddrSpace);
  auto ElemType = DataType->getScalarType();
  auto IntToPtr = Builder.CreateIntToPtr(
      Builder.getIntN(F->getParent()->getDataLayout().getPointerSizeInBits(AddrSpace), AlignedOffset), Int32Ptr);
  if (AddrSpace == ADDRESS_SPACE_GLOBAL) {
    // Add base ptr to AlignedOffset
    auto *M = CI.getModule();
    SmallVector<Type *, 1> Args = {Int32Ptr};
    auto *Func = GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_GetImplicitBufferPtr, Args);
    auto *LoadIntrinsic = Builder.CreateCall(Func);

    auto PtrBitSize = F->getParent()->getDataLayout().getPointerSizeInBits(AddrSpace);

    // Now extract 8 bytes of data beginning at offset 0
    auto *DataTypePtrSize = Type::getIntNTy(F->getParent()->getContext(), PtrBitSize);
    auto *PtrToInt = Builder.CreatePtrToInt(LoadIntrinsic, DataTypePtrSize);

    auto *AddInst = Builder.CreateAdd(PtrToInt, llvm::ConstantInt::get(DataTypePtrSize, AlignedOffset));

    IntToPtr = Builder.CreateIntToPtr(AddInst, Int32Ptr);
  }

  auto LoadType = IGCLLVM::FixedVectorType::get(ElemType, LoadByteSize / ElemByteSize);
  auto PtrType = PointerType::get(LoadType, AddrSpace);
  auto BitCast = Builder.CreateBitCast(IntToPtr, PtrType);
  auto LoadInst = Builder.CreateLoad(LoadType, BitCast);
  LoadInst->setAlignment(IGCLLVM::getCorrectAlign(ElemByteSize));
  LoadedData = LoadInst;

  if (Offset != AlignedOffset) {
    auto ByteVectorType = IGCLLVM::FixedVectorType::get(Builder.getInt8Ty(), LoadByteSize);
    auto BitCastToByte = Builder.CreateBitCast(LoadedData, ByteVectorType);
    Value *NewVector = UndefValue::get(IGCLLVM::FixedVectorType::get(Builder.getInt8Ty(), Size));
    for (unsigned int I = Offset; I != (Offset + Size); ++I) {
      auto Elem = Builder.CreateExtractElement(BitCastToByte, I - AlignedOffset);
      NewVector = Builder.CreateInsertElement(NewVector, Elem, (uint64_t)I - (uint64_t)Offset);
    }
    auto Result = Builder.CreateBitCast(NewVector, DataType);
    return Result;
  }
  auto Result = Builder.CreateBitCast(LoadedData, DataType);
  return Result;
}

Value *WIFuncResolution::getLocalId(CallInst &CI, ImplicitArg::ArgType argType) {
  // Receives:
  // call i32 @__builtin_IB_get_local_id_x()

  // Creates:
  // %localIdX
  auto F = CI.getFunction();
  Value *V = m_implicitArgs.getImplicitArgValue(*F, argType, m_pMdUtils);
  IGC_ASSERT(V);

  return V;
}

Value *WIFuncResolution::getGroupId(CallInst &CI) {
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

  Value *V = nullptr;
  auto F = CI.getParent()->getParent();
  V = m_implicitArgs.getImplicitArgValue(*F, ImplicitArg::R0, m_pMdUtils);

  Value *dim = CI.getArgOperand(0);
  Instruction *cmpDim = CmpInst::Create(Instruction::ICmp, CmpInst::ICMP_EQ, dim,
                                        ConstantInt::get(Type::getInt32Ty(CI.getContext()), 0), "cmpDim", &CI);
  Instruction *offsetR0 =
      SelectInst::Create(cmpDim, ConstantInt::get(Type::getInt32Ty(CI.getContext()), 1),
                         ConstantInt::get(Type::getInt32Ty(CI.getContext()), 5), "tmpOffsetR0", &CI);
  Instruction *index = BinaryOperator::CreateAdd(dim, offsetR0, "offsetR0", &CI);
  Instruction *groupId = ExtractElementInst::Create(V, index, "groupId", &CI);
  updateDebugLoc(&CI, cmpDim);
  updateDebugLoc(&CI, offsetR0);
  updateDebugLoc(&CI, index);
  updateDebugLoc(&CI, groupId);

  return groupId;
}
Value *WIFuncResolution::getLocalThreadId(CallInst &CI) {
  // Receives:
  // call spir_func i32 @__builtin_IB_get_local_thread_id()

  // Creates:
  // %r0second = extractelement <8 x i32> %r0, i32 2
  // %localThreadId = trunc i32 %r0second to i8

  // we need to access R0.2 bits 0 to 7, which contain HW local thread ID on XeHP_SDV+

  Value *V = nullptr;
  auto F = CI.getParent()->getParent();
  V = m_implicitArgs.getImplicitArgValue(*F, ImplicitArg::R0, m_pMdUtils);

  Instruction *r0second =
      ExtractElementInst::Create(V, ConstantInt::get(Type::getInt32Ty(CI.getContext()), 2), "r0second", &CI);
  Instruction *localThreadId =
      TruncInst::Create(Instruction::CastOps::Trunc, r0second, Type::getInt8Ty(CI.getContext()), "localThreadId", &CI);
  updateDebugLoc(&CI, r0second);
  updateDebugLoc(&CI, localThreadId);

  return localThreadId;
}

Value *WIFuncResolution::getGlobalSize(CallInst &CI) {
  // Receives:
  // call i32 @__builtin_IB_get_global_size(i32 %dim)

  // Creates:
  // %globalSize1 = extractelement <3 x i32> %globalSize, i32 %dim

  auto F = CI.getFunction();
  Value *V = m_implicitArgs.getImplicitArgValue(*F, ImplicitArg::GLOBAL_SIZE, m_pMdUtils);
  IGC_ASSERT(V != nullptr);

  Value *dim = CI.getArgOperand(0);
  Instruction *globalSize = ExtractElementInst::Create(V, dim, "globalSize", &CI);
  updateDebugLoc(&CI, globalSize);

  return globalSize;
}

Value *WIFuncResolution::getLocalSize(CallInst &CI) {
  // Receives:
  // call i32 @__builtin_IB_get_local_size(i32 %dim)

  // Creates:
  // %localSize = extractelement <3 x i32> %localSize, i32 %dim

  auto F = CI.getFunction();
  Value *V = m_implicitArgs.getImplicitArgValue(*F, ImplicitArg::LOCAL_SIZE, m_pMdUtils);
  IGC_ASSERT(V != nullptr);

  Value *dim = CI.getArgOperand(0);
  Instruction *localSize = ExtractElementInst::Create(V, dim, "localSize", &CI);
  updateDebugLoc(&CI, localSize);

  return localSize;
}

Value *WIFuncResolution::getEnqueuedLocalSize(CallInst &CI) {
  // Receives:
  // call i32 @__builtin_IB_get_enqueued_local_size(i32 %dim)

  // Creates:
  // %enqueuedLocalSize1 = extractelement <3 x i32> %enqueuedLocalSize, %dim

  auto F = CI.getFunction();
  Value *V = m_implicitArgs.getImplicitArgValue(*F, ImplicitArg::ENQUEUED_LOCAL_WORK_SIZE, m_pMdUtils);
  IGC_ASSERT(V != nullptr);

  Value *dim = CI.getArgOperand(0);
  Instruction *enqueuedLocalSize = ExtractElementInst::Create(V, dim, "enqueuedLocalSize", &CI);
  updateDebugLoc(&CI, enqueuedLocalSize);

  return enqueuedLocalSize;
}

Value *WIFuncResolution::getGlobalOffset(CallInst &CI) {
  // Receives:
  // call i32 @__builtin_IB_get_global_offset(i32 %dim)

  // Creates:
  // %globalOffset = extractelement <8 x i32> %payloadHeader, i32 %dim

  auto Ty = AllowShortImplicitPayloadHeader(getAnalysis<CodeGenContextWrapper>().getCodeGenContext())
                ? ImplicitArg::GLOBAL_OFFSET
                : ImplicitArg::PAYLOAD_HEADER;

  auto F = CI.getFunction();
  Value *V = m_implicitArgs.getImplicitArgValue(*F, Ty, m_pMdUtils);
  IGC_ASSERT(V != nullptr);

  Value *dim = CI.getArgOperand(0);
  auto globalOffset = ExtractElementInst::Create(V, dim, "globalOffset", &CI);
  updateDebugLoc(&CI, cast<Instruction>(globalOffset));

  return globalOffset;
}

Value *WIFuncResolution::getWorkDim(CallInst &CI) {
  // Receives:
  // call i32 @__builtin_IB_get_work_dim()

  // Creates:
  // %workDim
  auto F = CI.getFunction();
  Value *V = m_implicitArgs.getImplicitArgValue(*F, ImplicitArg::WORK_DIM, m_pMdUtils);
  IGC_ASSERT(V != nullptr);

  return V;
}

Value *WIFuncResolution::getNumGroups(CallInst &CI) {
  // Receives:
  // call i32 @__builtin_IB_get_num_groups(i32 %dim)

  // Creates:
  // %numGroups1 = extractelement <3 x i32> %numGroups, i32 %dim
  auto F = CI.getFunction();
  Value *V = m_implicitArgs.getImplicitArgValue(*F, ImplicitArg::NUM_GROUPS, m_pMdUtils);
  IGC_ASSERT(V != nullptr);

  Value *dim = CI.getArgOperand(0);
  Instruction *numGroups = ExtractElementInst::Create(V, dim, "numGroups", &CI);
  updateDebugLoc(&CI, numGroups);

  return numGroups;
}

Value *WIFuncResolution::getStageInGridOrigin(CallInst &CI) {
  // Receives:
  // call i32 @__builtin_IB_get_grid_origin(i32 %dim)

  // Creates:
  // %grid_origin1 = extractelement <3 x i32> %globalSize, i32 %dim
  auto F = CI.getParent()->getParent();
  Value *V = m_implicitArgs.getImplicitArgValue(*F, ImplicitArg::STAGE_IN_GRID_ORIGIN, m_pMdUtils);
  IGC_ASSERT(V != nullptr);

  Value *dim = CI.getArgOperand(0);
  Instruction *globalSize = ExtractElementInst::Create(V, dim, "grid_origin", &CI);
  updateDebugLoc(&CI, globalSize);

  return globalSize;
}

Value *WIFuncResolution::getStageInGridSize(CallInst &CI) {
  // Receives:
  // call i32 @__builtin_IB_get_grid_size(i32 %dim)

  // Creates:
  // %grid_size1 = extractelement <3 x i32> %globalSize, i32 %dim

  auto F = CI.getFunction();
  Value *V = m_implicitArgs.getImplicitArgValue(*F, ImplicitArg::STAGE_IN_GRID_SIZE, m_pMdUtils);

  IGC_ASSERT(V != nullptr);

  Value *dim = CI.getArgOperand(0);
  Instruction *globalSize = ExtractElementInst::Create(V, dim, "grid_size", &CI);
  updateDebugLoc(&CI, globalSize);

  return globalSize;
}

Value *WIFuncResolution::getSyncBufferPtr(CallInst &CI) {
  // Receives:
  // call i8 addrspace(1)* @__builtin_IB_get_sync_buffer()

  // Creates:
  // i8 addrspace(1)* %syncBuffer
  auto F = CI.getParent()->getParent();
  Value *syncBuffer = m_implicitArgs.getImplicitArgValue(*F, ImplicitArg::SYNC_BUFFER, m_pMdUtils);

  return syncBuffer;
}

Value *WIFuncResolution::getAssertBufferPtr(CallInst &CI) {
  // Receives:
  // call i8 addrspace(1)* @__builtin_IB_get_assert_buffer()

  // Creates:
  // i8 addrspace(1)* %assertBuffer
  auto F = CI.getParent()->getParent();
  Value *assertBuffer = m_implicitArgs.getImplicitArgValue(*F, ImplicitArg::ASSERT_BUFFER_POINTER, m_pMdUtils);

  return assertBuffer;
}

Value *WIFuncResolution::getRegionGroupSize(CallInst &CI) {
  // Receives:
  // call i32 @__builtin_IB_region_group_size(i32 %dim)

  // Creates:
  // %regionGroupSize1 = extractelement <3 x i32> %regionGroupSize, i32 %dim
  CodeGenContext *Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  if (!Ctx->platform.supportsQuantumDispatch()) {

    Ctx->EmitError("region_group_size is not supported on this platform!", &CI);
  }

  auto F = CI.getFunction();
  Value *V = m_implicitArgs.getImplicitArgValue(*F, ImplicitArg::REGION_GROUP_SIZE, m_pMdUtils);
  IGC_ASSERT(V != nullptr);

  Value *dim = CI.getArgOperand(0);
  Instruction *regionGroupSize = ExtractElementInst::Create(V, dim, "regionGroupSize", &CI);
  updateDebugLoc(&CI, regionGroupSize);

  return regionGroupSize;
}

Value *WIFuncResolution::getRegionGroupWGCount(CallInst &CI) {
  // Receives:
  // call i32 @__builtin_IB_region_group_wg_count()

  // Creates:
  // %regionGroupWGCount
  CodeGenContext *Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  if (!Ctx->platform.supportsQuantumDispatch()) {

    Ctx->EmitError("region_group_wg_count is not supported on this platform!", &CI);
  }

  auto F = CI.getFunction();
  Value *regionGroupWGCount = m_implicitArgs.getImplicitArgValue(*F, ImplicitArg::REGION_GROUP_WG_COUNT, m_pMdUtils);

  return regionGroupWGCount;
}

Value *WIFuncResolution::getRegionGroupBarrierBufferPtr(CallInst &CI) {
  // Receives:
  // call i8 addrspace(1)* @__builtin_IB_get_region_group_barrier_buffer()

  // Creates:
  // i8 addrspace(1)* %regionBuffer
  CodeGenContext *Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  if (!Ctx->platform.supportsQuantumDispatch()) {

    Ctx->EmitError("region_group_barrier_buffer is not supported on this platform!", &CI);
  }

  auto F = CI.getParent()->getParent();
  Value *regionBuffer = m_implicitArgs.getImplicitArgValue(*F, ImplicitArg::REGION_GROUP_BARRIER_BUFFER, m_pMdUtils);

  return regionBuffer;
}

// Register pass to igc-opt
#define PASS_FLAG2 "igc-lower-implicit-arg-intrinsic"
#define PASS_DESCRIPTION2 "igc-lower-implicit-arg-intrinsic"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(LowerImplicitArgIntrinsics, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(LowerImplicitArgIntrinsics, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)

char LowerImplicitArgIntrinsics::ID = 0;

LowerImplicitArgIntrinsics::LowerImplicitArgIntrinsics() : FunctionPass(ID) {
  initializeLowerImplicitArgIntrinsicsPass(*PassRegistry::getPassRegistry());
}

Constant *getKnownWorkGroupSize(IGCMD::MetaDataUtils *MDUtils, llvm::Function &F) {
  auto Dims = IGCMD::IGCMetaDataHelper::getThreadGroupDims(*MDUtils, &F);
  if (!Dims)
    return nullptr;

  return ConstantDataVector::get(F.getContext(), *Dims);
}

bool LowerImplicitArgIntrinsics::runOnFunction(Function &F) {
  m_FGA = getAnalysisIfAvailable<GenXFunctionGroupAnalysis>();
  m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  usedIntrinsicsMap.clear();

  visit(F);

  /// If the work group size is known at compile time, emit it as a
  /// literal rather than reading from the payload.
  auto MDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  if (Constant *KnownWorkGroupSize = getKnownWorkGroupSize(MDUtils, F)) {
    ImplicitArgs IAS(F, MDUtils);
    if (auto *V = IAS.getImplicitArgValue(F, ImplicitArg::ENQUEUED_LOCAL_WORK_SIZE, MDUtils))
      V->replaceAllUsesWith(KnownWorkGroupSize);
  }

  return false;
}

llvm::Value *LowerImplicitArgIntrinsics::getIntrinsicCall(llvm::Function *F, llvm::GenISAIntrinsic::ID IntrinsicID,
                                                          llvm::Type *DataType) {
  auto entry = usedIntrinsicsMap.find(IntrinsicID);
  if (entry != usedIntrinsicsMap.end())
    return entry->second;

  for (auto II = inst_begin(F), IE = inst_end(F); II != IE; II++) {
    if (GenIntrinsicInst *inst = dyn_cast<GenIntrinsicInst>(&*II)) {
      // if intrinsic call exists - use it
      if (inst->getIntrinsicID() == IntrinsicID) {
        // Make sure that intrinsic that we use is called before we use it's result
        auto parentFunction = inst->getParent()->getParent();
        auto firstFunc = parentFunction->getEntryBlock().getFirstNonPHI();
        inst->moveBefore(firstFunc);
        return inst;
      }
    }
  }

  // if intrinsic call doesn't exist - create it
  auto getFunctionDeclaration = GenISAIntrinsic::getDeclaration(F->getParent(), IntrinsicID, DataType);
  llvm::IRBuilder<> Builder(&*F->getEntryBlock().begin());
  auto callValue = Builder.CreateCall(getFunctionDeclaration);
  usedIntrinsicsMap.insert(std::pair(IntrinsicID, callValue));
  return callValue;
}

void LowerImplicitArgIntrinsics::visitCallInst(CallInst &CI) {
  Function *F = CI.getParent()->getParent();
  auto MDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

  // Not a GenISAIntrinsic
  GenIntrinsicInst *inst = dyn_cast<GenIntrinsicInst>(&CI);
  if (!inst)
    return;

  // Not a valid implicit arg intrinsic
  auto ID = inst->getIntrinsicID();
  ImplicitArg::ArgType argTy = ImplicitArgs::getArgType(ID);
  if (argTy == ImplicitArg::ArgType::NUM_IMPLICIT_ARGS)
    return;

  // If the intrinsic no longer have a use, just remove it
  if (inst->use_empty()) {
    CI.eraseFromParent();
    return;
  }

  // Lower intrinsic usage in the kernel to kernel args
  if (isEntryFunc(MDUtils, F)) {
    ImplicitArgs IAS(*F, MDUtils);
    Argument *Arg = IAS.getImplicitArg(*F, argTy);
    if (Arg) {
      CI.replaceAllUsesWith(Arg);
      CI.eraseFromParent();
    }
    return;
  }

  // Load from implicit arg buffer for intrinsic usage in stackcall
  bool LoadFromImplicitArgBuffer = F->hasFnAttribute("visaStackCall");

  // If the current function is a subroutine, but the caller is a stackcall, we
  // still need to use the implicit arg buffer.
  if (!LoadFromImplicitArgBuffer) {
    if (m_FGA) {
      Function *subGroupMapHead = m_FGA->getSubGroupMap(F);
      if (subGroupMapHead && subGroupMapHead->hasFnAttribute("visaStackCall"))
        LoadFromImplicitArgBuffer = true;
    }
  }

  if (LoadFromImplicitArgBuffer) {
    Value *V = nullptr;
    IGCLLVM::IRBuilder<> Builder(&CI);

    switch (ID) {
    case GenISAIntrinsic::GenISA_getLocalID_X:
    case GenISAIntrinsic::GenISA_getLocalID_Y:
    case GenISAIntrinsic::GenISA_getLocalID_Z: {
      // Get SIMD lane id
      llvm::Value *SimdLaneId =
          getIntrinsicCall(F, GenISAIntrinsic::ID::GenISA_simdLaneId, Type::getInt16Ty(F->getParent()->getContext()));
      llvm::Value *Result = nullptr;
      if (!m_ctx->platform.isProductChildOf(IGFX_XE_HP_SDV)) {
        // Get local id buffer base ptr
        auto Int32Ptr = PointerType::get(Type::getInt32Ty(F->getParent()->getContext()), ADDRESS_SPACE_GLOBAL);
        auto *M = CI.getModule();
        SmallVector<Type *, 1> Args = {Int32Ptr};
        auto *Func = GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_GetLocalIdBufferPtr, Args);
        auto *LocalIdPtr = Builder.CreateCall(Func);

        auto PtrBitSize = F->getParent()->getDataLayout().getPointerSizeInBits(ADDRESS_SPACE_GLOBAL);
        auto *DataTypePtrSize = Type::getIntNTy(F->getParent()->getContext(), PtrBitSize);

        auto *LocalIdBase = Builder.CreatePtrToInt(LocalIdPtr, DataTypePtrSize);

        SimdLaneId = Builder.CreateMul(SimdLaneId, ConstantInt::get(SimdLaneId->getType(), (uint64_t)6));

        if (argTy == ImplicitArg::ArgType::LOCAL_ID_X) {
        } else if (argTy == ImplicitArg::ArgType::LOCAL_ID_Y) {
          SimdLaneId = Builder.CreateAdd(SimdLaneId, ConstantInt::get(SimdLaneId->getType(), (uint64_t)2));
        } else if (argTy == ImplicitArg::ArgType::LOCAL_ID_Z) {
          SimdLaneId = Builder.CreateAdd(SimdLaneId, ConstantInt::get(SimdLaneId->getType(), (uint64_t)4));
        }
        SimdLaneId = Builder.CreateZExt(SimdLaneId, LocalIdBase->getType());
        Result = Builder.CreateAdd(SimdLaneId, LocalIdBase);
      } else {
        // LocalIDBase = oword_ld
        // LocalThreadId = r0.2
        // ThreadBaseOffset = LocalIDBase + LocalThreadId * (SimdSize * 3 * 2)
        // BaseOffset_X = ThreadBaseOffset + 0 * (SimdSize * 2) + (SimdLaneId * 2) OR
        // BaseOffset_Y = ThreadBaseOffset + 1 * (SimdSize * 2) + (SimdLaneId * 2) OR
        // BaseOffset_Z = ThreadBaseOffset + 2 * (SimdSize * 2) + (SimdLaneId * 2)
        // Load from BaseOffset_[X|Y|Z]

        // Get SIMD Size
        llvm::Value *SimdSize =
            getIntrinsicCall(F, GenISAIntrinsic::ID::GenISA_simdSize, Type::getInt32Ty(F->getParent()->getContext()));

        // SimdSize = max(SimdSize, 16)
        auto CmpInst = Builder.CreateICmpSGT(SimdSize, ConstantInt::get(SimdSize->getType(), (uint64_t)16));
        SimdSize = Builder.CreateSelect(CmpInst, SimdSize, ConstantInt::get(SimdSize->getType(), (uint64_t)16));

        // Get Local ID Base Ptr
        auto DataTypeI64 = Type::getInt64Ty(F->getParent()->getContext());
        unsigned int Offset = offsetof(implicit_args, local_id_table_ptr);
        auto LocalIDBase = BuildLoadInst(CI, Offset, DataTypeI64);

        // Get local thread id
        ImplicitArgs IAS(*F, MDUtils);
        auto R0Val = IAS.getImplicitArgValue(*F, ImplicitArg::R0, MDUtils);
        auto LocalThreadId =
            Builder.CreateExtractElement(R0Val, ConstantInt::get(Type::getInt32Ty(CI.getContext()), 2));
        LocalThreadId = Builder.CreateAnd(LocalThreadId, (uint16_t)255);

        // Compute thread base offset where local ids for current thread are stored
        // ThreadBaseOffset = LocalIDBasePtr + LocalThreadId * (simd size * 3 * 2)
        auto ThreadBaseOffset = Builder.CreateMul(SimdSize, ConstantInt::get(SimdSize->getType(), (uint64_t)6));
        ThreadBaseOffset =
            Builder.CreateMul(Builder.CreateZExt(ThreadBaseOffset, LocalThreadId->getType()), LocalThreadId);
        ThreadBaseOffset = Builder.CreateAdd(Builder.CreateZExt(ThreadBaseOffset, LocalIDBase->getType()), LocalIDBase);

        // Compute offset per lane
        uint8_t Factor = 0;
        if (argTy == ImplicitArg::ArgType::LOCAL_ID_Y) {
          Factor = 2;
        } else if (argTy == ImplicitArg::ArgType::LOCAL_ID_Z) {
          Factor = 4;
        }

        // Compute Factor*(simd size) * 2 to arrive at base of local id for current thread
        auto Expr1 = Builder.CreateMul(SimdSize, ConstantInt::get(SimdSize->getType(), Factor));

        // Compute offset to current lane
        auto Expr2 = Builder.CreateMul(SimdLaneId, ConstantInt::get(SimdLaneId->getType(), 2));

        Result = Builder.CreateAdd(Builder.CreateZExt(Expr1, LocalIDBase->getType()),
                                   Builder.CreateZExt(Expr2, LocalIDBase->getType()));

        Result = Builder.CreateAdd(Result, ThreadBaseOffset);
      }

      // Load data
      auto Int16Ptr = Type::getInt16PtrTy(F->getContext(), ADDRESS_SPACE_GLOBAL);
      auto Addr = Builder.CreateIntToPtr(Result, Int16Ptr);
      auto LoadInst = Builder.CreateLoad(Builder.getInt16Ty(), Addr);
      auto Trunc = Builder.CreateZExtOrBitCast(LoadInst, CI.getType());
      V = Trunc;
      break;
    }
    case GenISAIntrinsic::GenISA_getLocalSize:
    case GenISAIntrinsic::GenISA_getEnqueuedLocalSize: {
      // Assume local size and enqueued local size are the same
      auto ElemTypeD = Type::getInt32Ty(F->getParent()->getContext());
      auto VecTyD = IGCLLVM::FixedVectorType::get(ElemTypeD, 3);
      unsigned int Offset = offsetof(implicit_args, local_size_x);
      auto LoadInst = BuildLoadInst(CI, Offset, VecTyD);
      V = LoadInst;
      break;
    }
    case GenISAIntrinsic::GenISA_getPayloadHeader:
    case GenISAIntrinsic::GenISA_getGlobalOffset: {
      // global_offset is loaded from PayloadHeader[0:2]
      // currently there are no other uses for payload header.
      unsigned int Offset = offsetof(implicit_args, global_offset_x);
      auto ElemTypeD = Type::getInt32Ty(F->getParent()->getContext());
      auto VecTyQ = IGCLLVM::FixedVectorType::get(Type::getInt64Ty(F->getParent()->getContext()), 3);
      auto LoadInst = BuildLoadInst(CI, Offset, VecTyQ);
      Value *Undef = UndefValue::get(CI.getType());
      for (auto I = 0; I != 3; I++) {
        auto Elem = Builder.CreateExtractElement(LoadInst, (uint64_t)I);
        auto TruncElem = Builder.CreateTrunc(Elem, ElemTypeD);
        Undef = Builder.CreateInsertElement(Undef, TruncElem, (uint64_t)I);
      }
      V = Undef;
      break;
    }
    case GenISAIntrinsic::GenISA_getGlobalSize:
    case GenISAIntrinsic::GenISA_getStageInGridSize: {
      unsigned int Offset = offsetof(implicit_args, global_size_x);
      auto VecTyQ = IGCLLVM::FixedVectorType::get(Type::getInt64Ty(F->getParent()->getContext()), 3);
      auto ElemTypeD = Type::getInt32Ty(F->getParent()->getContext());
      auto LoadInst = BuildLoadInst(CI, Offset, VecTyQ);
      Value *Undef = UndefValue::get(CI.getType());
      for (auto I = 0; I != 3; I++) {
        auto Elem = Builder.CreateExtractElement(LoadInst, (uint64_t)I);
        auto TruncElem = Builder.CreateTrunc(Elem, ElemTypeD);
        Undef = Builder.CreateInsertElement(Undef, TruncElem, (uint64_t)I);
      }
      V = Undef;
      break;
    }
    case GenISAIntrinsic::GenISA_getNumWorkGroups: {
      auto ElemTypeUD = Type::getInt32Ty(F->getParent()->getContext());
      auto VecTyUD = IGCLLVM::FixedVectorType::get(ElemTypeUD, 3);
      unsigned int Offset = offsetof(implicit_args, group_count_x);
      auto LoadInst = BuildLoadInst(CI, Offset, VecTyUD);
      V = LoadInst;
      break;
    }
    case GenISAIntrinsic::GenISA_getWorkDim: {
      unsigned int Size = 4;
      unsigned int Offset = offsetof(implicit_args, num_work_dim) / Size;
      auto TypeUD = Type::getInt32Ty(F->getParent()->getContext());
      auto LoadInst = BuildLoadInst(CI, Offset, TypeUD);
      auto LShr = Builder.CreateLShr(LoadInst, (uint64_t)16);
      auto And = Builder.CreateAnd(LShr, (uint16_t)255);
      V = And;
      break;
    }
    case GenISAIntrinsic::GenISA_getPrintfBuffer: {
      // This function is invoked when expanding printf call to retrieve printf buffer ptr.
      auto DataTypeI64 = Type::getInt64Ty(CI.getFunction()->getParent()->getContext());
      unsigned int Offset = offsetof(implicit_args, printf_buffer_ptr);
      auto Result = BuildLoadInst(CI, Offset, DataTypeI64);
      Result = Builder.CreateIntToPtr(Result, CI.getType());
      V = Result;
      break;
    }
    case GenISAIntrinsic::GenISA_getRtGlobalBufferPtr: {
      unsigned int Offset = offsetof(implicit_args, rt_global_buffer_ptr);
      auto Result = BuildLoadInst(CI, Offset, Builder.getInt64Ty());
      Result = Builder.CreateIntToPtr(Result, CI.getType());
      V = Result;
      break;
    }
    case GenISAIntrinsic::GenISA_getAssertBufferPtr: {
      unsigned int Offset = offsetof(implicit_args, assert_buffer_ptr);
      auto Result = BuildLoadInst(CI, Offset, Builder.getInt64Ty());
      Result = Builder.CreateIntToPtr(Result, CI.getType());
      V = Result;
      break;
    }
    case GenISAIntrinsic::GenISA_getIndirectDataPtr: {
      // if implict args buffer is present, indirectDataPtr points to the
      // beginning of this buffer.
      unsigned int Offset = 0;
      auto Result = BuildLoadInst(CI, Offset, Builder.getInt64Ty());
      Result = Builder.CreateIntToPtr(Result, CI.getType());
      V = Result;
      break;
    }
    case GenISAIntrinsic::GenISA_getScratchPtr: {
      unsigned int Offset = offsetof(implicit_args, scratch_ptr);
      auto Result = BuildLoadInst(CI, Offset, Builder.getInt64Ty());
      Result = Builder.CreateIntToPtr(Result, CI.getType());
      V = Result;
      break;
    }
    case GenISAIntrinsic::GenISA_getRegionGroupSize: {
      auto ElemTypeD = Type::getInt32Ty(F->getParent()->getContext());
      auto VecTyD = IGCLLVM::FixedVectorType::get(ElemTypeD, 3);
      unsigned int Offset = offsetof(implicit_args, region_group_size_x);
      auto LoadInst = BuildLoadInst(CI, Offset, VecTyD);
      V = LoadInst;
      break;
    }
    case GenISAIntrinsic::GenISA_getRegionGroupWGCount: {
      unsigned int Offset = offsetof(implicit_args, region_group_wg_count);
      V = BuildLoadInst(CI, Offset, Builder.getInt32Ty());
      break;
    }
    case GenISAIntrinsic::GenISA_getRegionGroupBarrierBufferPtr: {
      unsigned int Offset = offsetof(implicit_args, region_group_barrier_buffer_ptr);
      auto Result = BuildLoadInst(CI, Offset, Builder.getInt64Ty());
      V = Builder.CreateIntToPtr(Result, CI.getType());
      break;
    }
    default:
      break;
    }

    if (V != nullptr) {
      CI.replaceAllUsesWith(V);
      CI.eraseFromParent();
    }
  }
}

llvm::CallInst *WIFuncResolution::CallGetLocalID(llvm::Instruction *pInsertBefore) {
  IGCLLVM::IRBuilder<> builder(pInsertBefore);
  llvm::Module *pM = pInsertBefore->getModule();
  llvm::FunctionType *pFuncTypeGetLocalID = llvm::FunctionType::get(builder.getInt32Ty(), {}, false);

  auto pFuncGetLocalID = pM->getOrInsertFunction(WIFuncsAnalysis::GET_LOCAL_THREAD_ID, pFuncTypeGetLocalID);

  return llvm::CallInst::Create(pFuncGetLocalID, {}, "", pInsertBefore);
}
