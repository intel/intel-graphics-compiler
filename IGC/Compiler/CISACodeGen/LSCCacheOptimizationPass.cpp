/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/**
 * @file  LSCCacheOptimizationPass.cpp
 * @author  Konstantin Rebrov
 *
 * @brief  This file implements the LSCCacheOptimizationPass
 * This pass performs an optimization upon the Load Store Cache, makes eligible store instructions go into the L1 cache
 * Instead of the L3 cache, which is the default setting.
 * It utilizes the L1 cache for store instructions that are in these four memory regions:
 * RTAsynctack, SWStack, SWHotZone, RTSynctack
 *
 * @details  This pass examines all store instructions, and if a store instruction is identified as being eligible,
 * and if the optimization is necessary and possible, the pass performs a Read Write Modify operation on the store
 * instruction.
 *
 * The requirements for a store instruction to go into the L1 cache are:
 *   The address for the store needs to be 16-byte aligned.
 *   The size of the stored data needs to be a multiple of 16 bytes.
 *   .ca on load and store instructions should not be marked as uncached
 * If these requirements are not satisfied then the store instruction is expanded to include the padding around the
 * stored data. This padding is preliminarily loaded into a virtual register to save those values.
 *
 * green blocks represent a dword that we don't want to overwrite (leave it as is)
 *   this is a padding blocks surrounding the (red) original memory location of the store instruction
 *
 * red blocks represent a dword that we actually do want to overwrite, specifically the old value
 *   this is the memory location that we want to store the new value into
 *   it is the region of memory which is addressed to by the original pointer operand of the input store instruction
 *
 * blue blocks represent the new value that we want to overwrite on top of the red dword
 */

#include "LSCCacheOptimizationPass.h"

#include "AdaptorCommon/RayTracing/MemRegionAnalysis.h" // for getRegionOffset(), RTMemRegion
#include "getCacheOpts.h"                               // for getCacheOptsStoreInst()
#include "AdaptorCommon/RayTracing/RTStackFormat.h"     // for RTStackFormat::LSC_WRITE_GRANULARITY
#include "IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp" // for suppressing LLVM warnings
#include <llvm/IR/LLVMContext.h>       // for llvm::LLVMContext
#include <llvm/IR/IRBuilder.h>         // for llvm::IRBuilder
#include <llvm/IR/Function.h>          // for llvm::Function
#include <llvm/IR/BasicBlock.h>        // for llvm::BasicBlock
#include <llvm/IR/Value.h>             // for llvm::Value
#include <llvm/IR/Type.h>              // for llvm::Type
#include <llvm/IR/DerivedTypes.h>      // for llvm::VectorType
#include <llvm/IR/Constants.h> // for llvm::ConstantInt, llvm::ConstantFP, llvm::ConstantVector, llvm::ConstantDataVector, llvm::UndefValue
#include <llvm/IR/Instruction.h>  // for llvm::Instruction
#include <llvm/IR/Instructions.h> // for llvm::StoreInst, llvm::CallInst
#include <llvm/ADT/APInt.h>           // for llvm::APInt, llvm::ArrayRef
#include "common/LLVMWarningsPop.hpp" // for suppressing LLVM warnings
#include "llvmWrapper/IR/DerivedTypes.h"
#include <optional>

#include <climits> // for CHAR_BIT
#include <cstdint> // for std::uint64_t
#include <string>  // for std::string
#include <vector>  // for std::vector

using namespace IGC;

using RTStackFormat::LSC_WRITE_GRANULARITY;

using namespace llvm;
using namespace llvm::GenISAIntrinsic;

using std::string;
using std::uint64_t;
using std::vector;

/**
 * @param builder  An IRBuilder to create instructions.
 *
 * @param data_type  The data type of the zeroed vector, can be floating point or integer.
 *
 * @param num_elements  The number of elements in the zeroed vector.
 *
 * @return  An rvalue which is a zeroed vector with num_elements of the specified data_type
 *          It can be used as an argument for a constructed instruction, such as insertelement
 */
inline Value *getZeroedVector(IRBuilder<> &builder, Type *data_type, uint64_t num_elements) {
  if (data_type->isFloatingPointTy()) {
    return ConstantDataVector::getSplat((unsigned)num_elements, ConstantFP::get(data_type, 0.0));
  } else {
    return ConstantDataVector::getSplat((unsigned)num_elements, ConstantInt::get(data_type, 0ull));
  }
}

/**
 * This utility function being given a source rvalue, which is usually a vector, extracts elements marked by a range out
 * of that vector and caches those resulting rvalues into a container. We might want to use those rvalues in future
 * instructions. This is a half-open range marked by the two last parameters: [begin, end) The range is intended to be
 * used for extracting elements from only part of the vector.
 *
 * @param builder  An IRBuilder to create instructions.
 *
 * @param source_rvalue  The rvalue from which to extract the elements.
 *                       It can be rvalues of both scalar and vector data types, both % virtual registers as well as
 * literal values. This function can extract elements from both green vectors and blue vectors.
 *
 * @param extracted_elements  Each element is extracted and saved as a Value* into this container.
 *
 * @param begin  This index points to the first element in the range.
 *
 * @param end  This index points to one after the last element in the range.
 */
static void extract_elements(IRBuilder<> &builder, Value *source_rvalue, vector<Value *> &extracted_elements,
                             uint64_t begin, uint64_t end) {
  IGC_ASSERT(begin <= end && extracted_elements.size() == (end - begin));
  if (begin == end)
    return;

  Type *type = source_rvalue->getType();
  // If the source rvalue is a vector of integers.
  if (isa<VectorType>(type)) {
    vector<Value *>::iterator i = extracted_elements.begin();
    for (uint64_t extract_at = begin; extract_at < end; ++extract_at, ++i) {
      auto *element = builder.CreateExtractElement(source_rvalue, extract_at);
      *i = element;
    }
  }
  // If the source rvalue is a scalar integer.
  else {
    extracted_elements[0] = source_rvalue;
  }
}

/**
 * This utility function being given a source rvalue, which is usually a vector, extracts N elements out of that vector
 * and caches those resulting rvalues into a container. We might want to use those rvalues in future instructions.
 *
 * @param builder  An IRBuilder to create instructions.
 *
 * @param source_rvalue  The rvalue from which to extract the elements.
 *                       It can be rvalues of both scalar and vector data types, both % virtual registers as well as
 * literal values. This function can extract elements from both green vectors and blue vectors.
 *
 * @param extracted_elements  Each element is extracted and saved as a Value* into this container.
 *
 * @param num_elements  The number of elements to extract.
 */
static void extract_elements(IRBuilder<> &builder, Value *source_rvalue, vector<Value *> &extracted_elements,
                             uint64_t num_elements) {
  return extract_elements(builder, source_rvalue, extracted_elements, 0, num_elements);
}

/**
 * This function takes a container having rvalues of individual elements, and inserts each one of them into the given
 * llvm vector, the positions within that vector where to insert the elements are marked by the range. This is a
 * half-open range marked by the two last parameters: [begin, end) The range is intended to be used if we want to insert
 * elements into only part of the vector.
 *
 * @param builder  An IRBuilder to create instructions.
 *
 * @param source_rvalue  The rvalue vector which will be the base for the insertions.
 *                       In other words this is the vector into which we insert the elements.
 *
 * @param extracted_elements  A container containing Value* objects to insert into the vector.
 *
 * @param begin  This index points to the first element in the range.
 *
 * @param end  This index points to one after the last element in the range.
 *
 * @return Value*  A virtual register of the vector with all the inserted elements.
 */
static Value *insert_elements(IRBuilder<> &builder, Value *source_rvalue, const vector<Value *> &extracted_elements,
                              uint64_t begin, uint64_t end) {
  Value *temp_vector = source_rvalue;

  IGC_ASSERT(begin <= end && extracted_elements.size() == (end - begin));

  vector<Value *>::const_iterator i = extracted_elements.cbegin();
  for (uint64_t insert_at = begin; insert_at < end; ++insert_at, ++i) {
    // Use the previous vector to compute the insertelement insruction for the current vector,
    // overwriting the current vector into the variable, discarding the information for the previous vector.
    temp_vector = builder.CreateInsertElement(temp_vector, *i, insert_at);
  }
  return temp_vector;
}

bool LSCCacheOptimizationPass::runOnFunction(Function &function) {
  m_CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  changed_IR = false;
  current_function = &function;
  current_context = &function.getContext();

#if 0
    // This method is only used for testing purposes.
    // In order to insert a 48 wide store instruction into the Function, if it doesn't already have one.
    // Most shaders that I've run it through don't have such an instruction, so I have to manually insert one
    // like that in order to test the fourth case.
    create_48_wide_store(function);
#endif

  // This method indirectly calls visitStoreInst() to process the LLVM IR function.
  // It basically performs the RMW operation on any eligible instructions.
  visit(function);
  return changed_IR;
}

void LSCCacheOptimizationPass::visitStoreInst(StoreInst &storeInst) {
  auto cacheOpts = getCacheOptsStorePolicy(storeInst, *m_CGCtx);

  // cacheOpts will be not None for the following memory regions:
  // RTAsynctack, SWStack, SWHotZone, RTSynctack
  if (!cacheOpts)
    return;

  auto store_cache_policy = cacheOpts.value();
  if (store_cache_policy == LSC_L1UC_L3UC || store_cache_policy == LSC_L1UC_L3C_WB) {
    // The cache policy is uncached for the L1 cache.
    // unsuccessful early exit
    return;
  }

  auto &DL = current_function->getParent()->getDataLayout();

  uint64_t offset = 0;
  uint64_t region_size = 0;

  const Instruction *address = dyn_cast<Instruction>(storeInst.getPointerOperand());
  auto Region = getRegionOffset(address, *m_CGCtx->getModuleMetaData(), &DL, &offset, &region_size);
  if (!Region)
    return;

  IGC_ASSERT(offset < region_size); // the offset is within the bounds of the memory region

  Value *value = storeInst.getValueOperand();

  Type *type = value->getType();
  uint64_t data_size = 0;    // measured in bytes
  uint64_t element_size = 0; // measured in bytes
  Type *element_type;
  // If the stored data is a vector variable.
  if (IGCLLVM::FixedVectorType *vectorType = dyn_cast<IGCLLVM::FixedVectorType>(type)) {
    element_type = vectorType->getElementType();
    uint64_t num_elements = vectorType->getNumElements();
    element_size = DL.getTypeSizeInBits(element_type) / CHAR_BIT;
    data_size = num_elements * element_size;
  }
  // If the stored data is a scalar variable.
  else {
    data_size = element_size = DL.getTypeSizeInBits(type) / CHAR_BIT;
    element_type = type;
  }

  // If the offset is a multiple of 16 bytes (16 byte aligned store address)
  // && the size of the stored data is a multiple of 16 bytes
  if ((offset % LSC_WRITE_GRANULARITY == 0) && (data_size % LSC_WRITE_GRANULARITY == 0)) {
    // successful early exit
    return;
  }

  // Convert the address from the start of the memory region into
  // the offset from the start of the previous nearest 16 byte boundary.
  offset %= LSC_WRITE_GRANULARITY;

  // Create an IRBuilder with an insertion point set to the given intrinsic_call instruction.
  // IRBuilder automatically inserts instructions when it creates them,
  // and the inserted instructions (dynamically allocated) are deleted when the function is destroyed.
  IRBuilder<> builder(&storeInst);
  auto *initial_pointer = storeInst.getPointerOperand();
  unsigned addrspace = storeInst.getPointerAddressSpace();

  uint64_t right_boundary; // measured in bytes
  // If the stored data straddles across three 16 bytes size chunks,
  // we break it up into two stores: first 32 bytes and last 16 bytes
  /* This is the fourth case. */
  if (offset + data_size > 32) {
    right_boundary = 48;
    // get the left pad
    // get the right pad
    // get the blue blocks
    // construct and store the first 32 bytes vector
    // construct and store the last 16 bytes vector

    uint64_t num_blue_blocks = data_size / element_size;

    uint64_t num_green_blocks_left = offset / element_size;
    uint64_t num_blue_blocks_left = (32 - offset) / element_size;
    uint64_t num_total_blocks_left = num_green_blocks_left + num_blue_blocks_left;

    uint64_t num_green_blocks_right = (right_boundary - (offset + data_size)) / element_size;
    uint64_t num_blue_blocks_right = (offset + data_size - 32) / element_size;
    uint64_t num_total_blocks_right = num_green_blocks_right + num_blue_blocks_right;

    /* First do the GGRR 32 wide store */
    // %0 = bitcast <>* %baseAddress to i8*
    auto *bitcast1 = builder.CreateBitCast(initial_pointer, builder.getInt8PtrTy(addrspace));
    // %1 = getelementptr i8, i8* %0, i64 -offset
    auto *left_green_address = builder.CreateGEP(builder.getInt8Ty(), bitcast1, builder.getInt64(-1 * offset));
    // %2 = bitcast i8* %1 to <num_green_blocks_left x iN>*
    auto *left_green_vector_pointer = builder.CreateBitCast(
        left_green_address,
        IGCLLVM::FixedVectorType::get(element_type, (unsigned)num_green_blocks_left)->getPointerTo(addrspace));
    // %3 = load <num_green_blocks_left x iN>, <num_green_blocks_left x iN>* %2
    auto *left_green_vector_rvalue = builder.CreateLoad(
        IGCLLVM::FixedVectorType::get(element_type, (unsigned)num_green_blocks_left), left_green_vector_pointer);

    // The static_cast is needed to remove this warning or error, which appears only in QuickBuild Windows
    // https://ubit-gfx.intel.com/build/9814699/step_status
    //   error C2220: the following warning is treated as an error
    //   warning C4244: 'argument': conversion from 'uint64_t' to 'const unsigned int', possible loss of data
    vector<Value *> green_elements_left(static_cast<const unsigned int>(num_green_blocks_left));
    extract_elements(builder, left_green_vector_rvalue, green_elements_left, num_green_blocks_left);
    vector<Value *> blue_elements_left(static_cast<const unsigned int>(num_blue_blocks_left));
    extract_elements(builder, value, blue_elements_left, num_blue_blocks_left);

    auto *initial_vector_left =
        UndefValue::get(IGCLLVM::FixedVectorType::get(element_type, (unsigned)num_total_blocks_left));
    auto *intermediate_vector_left =
        insert_elements(builder, initial_vector_left, green_elements_left, 0, num_green_blocks_left);
    auto *final_vector_left = insert_elements(builder, intermediate_vector_left, blue_elements_left,
                                              num_green_blocks_left, num_total_blocks_left);

    auto *final_vector_pointer_left = builder.CreateBitCast(
        left_green_address,
        IGCLLVM::FixedVectorType::get(element_type, (unsigned)num_total_blocks_left)->getPointerTo(addrspace));
    builder.CreateAlignedStore(final_vector_left, final_vector_pointer_left, llvm::Align(LSC_WRITE_GRANULARITY));

    auto *right_part_address = builder.CreateGEP(builder.getInt8Ty(), bitcast1, builder.getInt64(32 - offset));

    vector<Value *> blue_elements_right(static_cast<const unsigned int>(num_blue_blocks_right));
    extract_elements(builder, value, blue_elements_right, num_blue_blocks_left, num_blue_blocks);

    vector<Value *> green_elements_right(static_cast<const unsigned int>(num_green_blocks_right));
    auto *right_green_address = builder.CreateGEP(builder.getInt8Ty(), bitcast1, builder.getInt64(data_size));
    auto *right_green_vector_pointer = builder.CreateBitCast(
        right_green_address,
        IGCLLVM::FixedVectorType::get(element_type, (unsigned)num_green_blocks_right)->getPointerTo(addrspace));
    auto *right_green_vector_rvalue = builder.CreateLoad(
        IGCLLVM::FixedVectorType::get(element_type, (unsigned)num_green_blocks_right), right_green_vector_pointer);
    extract_elements(builder, right_green_vector_rvalue, green_elements_right, num_green_blocks_right);

    auto *initial_vector_right =
        UndefValue::get(IGCLLVM::FixedVectorType::get(element_type, (unsigned)num_total_blocks_right));
    auto *intermediate_vector_right =
        insert_elements(builder, initial_vector_right, blue_elements_right, 0, num_blue_blocks_right);
    auto *final_vector_right = insert_elements(builder, intermediate_vector_right, green_elements_right,
                                               num_blue_blocks_right, num_total_blocks_right);

    auto *final_vector_pointer_right = builder.CreateBitCast(
        right_part_address,
        IGCLLVM::FixedVectorType::get(element_type, (unsigned)num_total_blocks_right)->getPointerTo(addrspace));
    builder.CreateAlignedStore(final_vector_right, final_vector_pointer_right, llvm::Align(LSC_WRITE_GRANULARITY));
  }
  // If the data straddles across one or two 16 byte size chunks,
  // we do only one load or store, but we can optimize it to
  // load only the green blocks if they are contiguous
  else {
    right_boundary = (offset + data_size > 16) ? 32 : 16;
    // the red blocks are on the left side, contiguous green blocks on the right side
    // load only the contiguous green blocks
    /* This is the first case. */
    if (offset == 0) {
      // get the right pad
      // get the blue blocks
      // construct and store the right_boundary size vector

      uint64_t num_green_blocks = (right_boundary - data_size) / element_size;
      uint64_t num_blue_blocks = data_size / element_size;

      // %0 = bitcast <>* %baseAddress to i8*
      auto *bitcast1 = builder.CreateBitCast(initial_pointer, builder.getInt8PtrTy(addrspace));
      // %1 = getelementptr i8, i8* %0, i64 data_size
      auto *green_address = builder.CreateGEP(builder.getInt8Ty(), bitcast1, builder.getInt64(data_size));
      // %2 = bitcast i8* %1 to <num_green_blocks x iN>*
      auto *green_vector_pointer = builder.CreateBitCast(
          green_address,
          IGCLLVM::FixedVectorType::get(element_type, (unsigned)num_green_blocks)->getPointerTo(addrspace));
      // %3 = load <num_green_blocks x iN>, <num_green_blocks x iN>* %2
      auto *green_vector_rvalue = builder.CreateLoad(
          IGCLLVM::FixedVectorType::get(element_type, (unsigned)num_green_blocks), green_vector_pointer);

      vector<Value *> green_elements(static_cast<const unsigned int>(num_green_blocks));
      extract_elements(builder, green_vector_rvalue, green_elements, num_green_blocks);
      vector<Value *> blue_elements(static_cast<const unsigned int>(num_blue_blocks));
      extract_elements(builder, value, blue_elements, num_blue_blocks);

      uint64_t num_elements = num_green_blocks + num_blue_blocks;
      auto *initial_vector = UndefValue::get(IGCLLVM::FixedVectorType::get(element_type, (unsigned)num_elements));
      auto *intermediate_vector = insert_elements(builder, initial_vector, blue_elements, 0, num_blue_blocks);
      auto *final_vector = insert_elements(builder, intermediate_vector, green_elements, num_blue_blocks, num_elements);

      auto *final_vector_pointer = builder.CreateBitCast(
          initial_pointer,
          IGCLLVM::FixedVectorType::get(element_type, (unsigned)num_elements)->getPointerTo(addrspace));
      builder.CreateAlignedStore(final_vector, final_vector_pointer, llvm::Align(LSC_WRITE_GRANULARITY));
    }
    // the red blocks are on the right side, contiguous green blocks on the left side
    // load only the contiguous green blocks
    /* This is the second case. */
    else if (offset + data_size == right_boundary) {
      // get the left pad
      // get the blue blocks
      // construct and store the right_boundary size vector

      uint64_t num_green_blocks = offset / element_size;
      uint64_t num_blue_blocks = data_size / element_size;

      // %0 = bitcast <>* %baseAddress to i8*
      auto *bitcast1 = builder.CreateBitCast(initial_pointer, builder.getInt8PtrTy(addrspace));
      // %1 = getelementptr i8, i8* %0, i64 -offset
      auto *green_address = builder.CreateGEP(builder.getInt8Ty(), bitcast1, builder.getInt64(-1 * offset));
      // %2 = bitcast i8* %1 to <num_green_blocks x iN>*
      auto *green_vector_pointer = builder.CreateBitCast(
          green_address,
          IGCLLVM::FixedVectorType::get(element_type, (unsigned)num_green_blocks)->getPointerTo(addrspace));
      // %3 = load <num_green_blocks x iN>, <num_green_blocks x iN>* %2
      auto *green_vector_rvalue = builder.CreateLoad(
          IGCLLVM::FixedVectorType::get(element_type, (unsigned)num_green_blocks), green_vector_pointer);

      vector<Value *> green_elements(static_cast<const unsigned int>(num_green_blocks));
      extract_elements(builder, green_vector_rvalue, green_elements, num_green_blocks);
      vector<Value *> blue_elements(static_cast<const unsigned int>(num_blue_blocks));
      extract_elements(builder, value, blue_elements, num_blue_blocks);

      uint64_t num_elements = num_green_blocks + num_blue_blocks;
      auto *initial_vector = UndefValue::get(IGCLLVM::FixedVectorType::get(element_type, (unsigned)num_elements));
      auto *intermediate_vector = insert_elements(builder, initial_vector, green_elements, 0, num_green_blocks);
      auto *final_vector = insert_elements(builder, intermediate_vector, blue_elements, num_green_blocks, num_elements);

      auto *final_vector_pointer = builder.CreateBitCast(
          green_address, IGCLLVM::FixedVectorType::get(element_type, (unsigned)num_elements)->getPointerTo(addrspace));
      builder.CreateAlignedStore(final_vector, final_vector_pointer, llvm::Align(LSC_WRITE_GRANULARITY));
    }
    // the red blocks in the middle, around them discontinuous green blocks
    // load the green blocks on the left, red blocks in the middle, and green blocks on the right
    /* This is the third case. */
    else {
      // get the whole thing
      // get the blue blocks
      // construct and store the right_boundary size vector, overwriting the red blocks in the middle

      uint64_t num_total_blocks = right_boundary / element_size;

      // %0 = bitcast <>* %baseAddress to i8*
      auto *bitcast1 = builder.CreateBitCast(initial_pointer, builder.getInt8PtrTy(addrspace));
      // %1 = getelementptr i8, i8* %0, i64 -offset
      auto *starting_address = builder.CreateGEP(builder.getInt8Ty(), bitcast1, builder.getInt64(-1 * offset));
      // %2 = bitcast i8* %1 to <num_total_blocks x iN>*
      auto *full_vector_pointer = builder.CreateBitCast(
          starting_address,
          IGCLLVM::FixedVectorType::get(element_type, (unsigned)num_total_blocks)->getPointerTo(addrspace));
      // %3 = load <num_total_blocks x iN>, <num_total_blocks x iN>* %2
      auto *full_vector_rvalue = builder.CreateLoad(
          IGCLLVM::FixedVectorType::get(element_type, (unsigned)num_total_blocks), full_vector_pointer);

      uint64_t left_padding_size_blocks = offset / element_size;
      uint64_t num_blue_blocks = data_size / element_size;
      vector<Value *> blue_elements(static_cast<const unsigned int>(num_blue_blocks));
      extract_elements(builder, value, blue_elements, num_blue_blocks);

      auto *final_vector = insert_elements(builder, full_vector_rvalue, blue_elements, left_padding_size_blocks,
                                           left_padding_size_blocks + num_blue_blocks);
      builder.CreateAlignedStore(final_vector, full_vector_pointer, llvm::Align(LSC_WRITE_GRANULARITY));
    }
  }

  storeInst.eraseFromParent();
  changed_IR = true;
}

#if 0
bool LSCCacheOptimizationPass::create_48_wide_store(Function& function)
{
    Function::iterator bb = function.begin(), bb_end = function.end();
    for (; bb != bb_end; ++bb) {
        for (BasicBlock::iterator i = bb->begin(), i_end = bb->end(); i != i_end; ++i) {
            // Loop through all instructions in a function, search for a call to a GenIntrinsicInst::GenISA_AsyncStackPtr
            // which would look something like this:
            // %perLaneAsyncStackPointer24 = call noalias align 128 dereferenceable(256) %"struct.RTStackFormat::RTStack" addrspace(1)* @"llvm.genx.GenISA.AsyncStackPtr.p1struct.RTStackFormat::RTStack.i64"(i64 %19)
            if (auto* intrinsic_call = dyn_cast<GenIntrinsicInst>(i)) {
                if ((intrinsic_call->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_AsyncStackPtr) ||
                    (intrinsic_call->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_AsyncStackPtrPlaceHolder))
                {
                    // Create an IRBuilder with an insertion point set to the given intrinsic_call instruction.
                    // IRBuilder automatically inserts instructions when it creates them,
                    // and the inserted instructions (dynamically allocated) are deleted when the function is destroyed.
                    IRBuilder<> builder(intrinsic_call);
                    Type* return_type = intrinsic_call->getFunctionType()->getReturnType();
                    unsigned addrspace = return_type->getPointerAddressSpace();

                    uint64_t offset = 8;          // in bytes
                    uint64_t num_red_blocks = 8;  // in dwords
                    Type* element_type = builder.getInt32Ty();
                    // %0 = bitcast <>* %baseAddress to i8*
                    auto* bitcast1 = builder.CreateBitCast(intrinsic_call, builder.getInt8PtrTy(addrspace));
                    // %1 = getelementptr i8, i8* %0, i64 offset
                    auto* red_address = builder.CreateGEP(builder.getInt8Ty(), bitcast1, builder.getInt64(offset));
                    // %2 = bitcast i8* %1 to <num_red_blocks x iN>*
                    auto* red_vector_pointer = builder.CreateBitCast(red_address, IGCLLVM::FixedVectorType::get(element_type, (unsigned)num_red_blocks)->getPointerTo(addrspace));
                    auto* red_vector_rvalue = getZeroedVector(builder, element_type, num_red_blocks);
                    builder.CreateStore(red_vector_rvalue, red_vector_pointer);

                    changed_IR = true;
                    return true;
                }
            }
        }
    }
    return false;
}
#endif

char LSCCacheOptimizationPass::ID = 0;
#define PASS_FLAG "LSC-Cache-Optimization-pass"
#define PASS_DESCRIPTION "Load/Store cache optimization pass"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LSCCacheOptimizationPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(LSCCacheOptimizationPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
