/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/RuntimeValueLegalizationPass.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvmWrapper/IR/DerivedTypes.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-runtimevalue-legalization-pass"
#define PASS_DESCRIPTION "Shader runtime value legalization"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(RuntimeValueLegalizationPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(RuntimeValueLegalizationPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace IGC
{

char RuntimeValueLegalizationPass::ID = 0;

////////////////////////////////////////////////////////////////////////////
RuntimeValueLegalizationPass::RuntimeValueLegalizationPass() : llvm::ModulePass(ID)
{
    initializeRuntimeValueLegalizationPassPass(*llvm::PassRegistry::getPassRegistry());
}

////////////////////////////////////////////////////////////////////////////
void RuntimeValueLegalizationPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.setPreservesCFG();
    AU.addRequired<CodeGenContextWrapper>();
}

////////////////////////////////////////////////////////////////////////////
// @brief RuntimeValue comaprator function
static std::function<bool(const std::pair<uint32_t, uint32_t>&, const std::pair<uint32_t, uint32_t>&)>
RuntimeValueComparator = [](const std::pair<uint32_t, uint32_t>& lhs, const std::pair<uint32_t, uint32_t>& rhs) -> bool
{
    return (lhs.first < rhs.first) || ((lhs.first == rhs.first) && (lhs.second > rhs.second));
};

////////////////////////////////////////////////////////////////////////////
// @brief Helper type representing collection of RuntimeValue calls. Every entry
// consists of pointer to RuntimeValue instruction together with offsets of first
// and last element of given RuntimeValue. First and last element offsets have
// different values only in case vector RuntimeValue calls.
// The collection is sorted according to offset and number of elements of RuntimeValue.
// { {0, 9},  RuntimeValue* }
// { {0, 0},  RuntimeValue* }
// { {6, 6},  RuntimeValue* }
// { {8, 19}, RuntimeValue* }
typedef std::multimap<
    std::pair<uint32_t, uint32_t>, llvm::GenIntrinsicInst*, decltype(RuntimeValueComparator)> RuntimeValueCollection;

////////////////////////////////////////////////////////////////////////////
// @brief Get all RuntimeValue calls. The collection of RuntimeValue calls is sorted
// according to offset and number of elements of RuntimeValue. RuntimeValue calls
// representing single scalars have only one element unlike RuntimeValue calls
// representing vectors of scalars. RuntimeValue vectors are made GRF-aligned if needed.
static bool GetAllRuntimeValueCalls(
    llvm::Module& module,
    RuntimeValueCollection& runtimeValueCalls,
    uint32_t dataGRFAlignmentInDwords)
{
    bool legalizationCheckNeeded = false;
    for (llvm::Function& F : module)
    {
        for (llvm::BasicBlock& B : F)
        {
            for (llvm::Instruction& I : B)
            {
                llvm::GenIntrinsicInst* intr = llvm::dyn_cast<llvm::GenIntrinsicInst>(&I);
                if (intr &&
                    intr->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_RuntimeValue &&
                    llvm::isa<llvm::ConstantInt>(intr->getArgOperand(0)))
                {
                    uint32_t offset = int_cast<uint32_t>(
                        cast<ConstantInt>(intr->getArgOperand(0))->getZExtValue());

                    if (intr->getType()->isVectorTy())
                    {
                        if (llvm::isa<IGCLLVM::FixedVectorType>(intr->getType()))
                        {
                            IGCLLVM::FixedVectorType* const fixedVectorTy =
                                cast<IGCLLVM::FixedVectorType>(intr->getType());
                            // Only vectors of 32-bit values are supported at the moment
                            if (fixedVectorTy->getElementType()->getPrimitiveSizeInBits() == 32)
                            {
                                // Only vectors having corresponding extract element instructions
                                // are subject of further processing.
                                bool hasEEUser = false;
                                for (llvm::User* user : intr->users())
                                {
                                    if (llvm::ExtractElementInst* EEI = llvm::dyn_cast<llvm::ExtractElementInst>(user))
                                    {
                                        hasEEUser = true;
                                        break;
                                    }
                                }
                                if (hasEEUser)
                                {
                                    uint32_t numElements = int_cast<uint32_t>(fixedVectorTy->getNumElements());
                                    const uint32_t lastElementOffset = offset + numElements - 1;

                                    // Check if vector needs to be GRF aligned
                                    // Vector must be GRF aligned if it's size is larger than or equal to one GRF.
                                    // Vector must fit in one GRF if its size is less than one GRF(it can not cross GRF boundary).
                                    const uint32_t alignedVectorOffset =
                                        int_cast<uint32_t>(llvm::alignTo(offset, dataGRFAlignmentInDwords));
                                    // Offset can be already aligned to GRF boundary
                                    if (offset != alignedVectorOffset)
                                    {
                                        // Check if vector crosses GRF boundary
                                        if (lastElementOffset >= alignedVectorOffset)
                                        {
                                            // Align to GRF by changing vector's offset
                                            offset = int_cast<uint32_t>(llvm::alignDown(offset, dataGRFAlignmentInDwords));
                                        }
                                    }
                                    runtimeValueCalls.insert(std::make_pair(std::make_pair(offset, lastElementOffset), intr));

                                    // Having RuntimeValue vectors, further legalization checks are needed
                                    legalizationCheckNeeded = true;
                                }
                            }
                            else
                            {
                                IGC_ASSERT_MESSAGE(0, "Only vectors of 32-bit values are supported at the moment");
                            }
                        }
                    }
                    else
                    {
                        runtimeValueCalls.insert(std::make_pair(std::make_pair(offset, offset), intr));
                    }
                }
            }
        }
    }
    return legalizationCheckNeeded;
}

////////////////////////////////////////////////////////////////////////////
// @brief Creates a set of accessed RuntimeValue offsets (no duplicates).
static void GetAccessedOffsets(
    RuntimeValueCollection& runtimeValueCalls,
    std::set<uint32_t>& accessedOffsetsSet)
{
    // Since input collection is already sorted according to offset
    // and number of elements of RuntimeValue, it's enough to process
    // only first elements for distinct offsets:
    // ->{ {0, 7},  RuntimeValue* }
    //   { {0, 0},  RuntimeValue* }
    // ->{ {6, 6},  RuntimeValue* }
    // ->{ {8, 19}, RuntimeValue* }
    //   { {8, 8},  RuntimeValue* }
    int prevOffset = -1;
    for (auto it : runtimeValueCalls)
    {
        std::pair<uint32_t, uint32_t> offsets = it.first;
        if (offsets.first != prevOffset)
        {
            for (uint32_t i = offsets.first; i <= offsets.second; i++)
            {
                accessedOffsetsSet.insert(i);
            }
            prevOffset = offsets.first;
        }
    }
}

////////////////////////////////////////////////////////////////////////////
// @brief Creates a map of accessed RuntimeValue regions. The map has the following
// format: {offset { enclosing_region_start_offset, enclosing_region_size }}
// for example: {0, {0, 2}}, {1, {0, 2}}, {4, {4, 1}}
// Resulting ranges are disjoint and each spans the biggest continuous range of offsets.
static void GetAccessedRegions(
    RuntimeValueCollection& runtimeValueCalls,
    std::map<uint32_t, std::pair<uint32_t, uint32_t>>& accessedRegions)
{
    // Create a set of accessed offsets without duplicates
    std::set<uint32_t> accessedOffsetsSet;
    GetAccessedOffsets(runtimeValueCalls, accessedOffsetsSet);

    // Traverse the accessed offsets to create map of disjoint RuntimeValue regions.
    std::vector<uint32_t> accessedOffsets(accessedOffsetsSet.begin(), accessedOffsetsSet.end());
    std::size_t numOffsets = accessedOffsets.size();
    if (numOffsets > 1)
    {
        uint32_t numElementsInRange = 1;
        for (std::size_t i = 1; i <= numOffsets; i++)
        {
            if ((i == numOffsets) || (accessedOffsets[i] - accessedOffsets[i - 1] != 1))
            {
                uint32_t beginIdx = accessedOffsets[i - numElementsInRange];
                uint32_t endIdx = accessedOffsets[i - 1];
                uint32_t numOfElements = endIdx - beginIdx + 1;
                for (uint32_t idx = beginIdx; idx <= endIdx; idx++)
                {
                    accessedRegions.insert(std::make_pair(idx, std::make_pair(beginIdx, numOfElements)));
                }

                // Reset range element counter
                numElementsInRange = 1;
            }
            else
            {
                numElementsInRange++;
            }
        }
    }
    else
    {
        accessedRegions.insert(std::make_pair(accessedOffsets[0], std::make_pair(accessedOffsets[0], 1)));
    }
}

////////////////////////////////////////////////////////////////////////////
// @brief Legalizes RuntimeValue calls for push analysis.
//
// 1) RuntimeValue vector must be GRF aligned if it's size is larger than or equal to one GRF.
//    RuntimeValue vector must fit in one GRF if its size is less than one GRF.
//    Replace:
//      %15 = call <6 x i32> @llvm.genx.GenISA.RuntimeValue.v6i32(i32 4)
//      %17 = extractelement <6 x i32> %15, i32 %0
//    with:
//      %15 = call <10 x i32> @llvm.genx.GenISA.RuntimeValue.v10i32(i32 0)
//      %16 = add i32 %0, 4
//      %17 = extractelement <10 x i32> %15, i32 %16
//
// 2) RuntimeValue vectors can not overlap:
//    Replace:
//      %15 = call <10 x i32> @llvm.genx.GenISA.RuntimeValue.v10i32(i32 0)
//      %17 = extractelement <10 x i32> %15, i32 %0
//      %25 = call <12 x i32> @llvm.genx.GenISA.RuntimeValue.v12i32(i32 8)
//      %27 = extractelement <12 x i32> % 25, i32 %0
//    with:
//      %15 = call <20 x i32> @llvm.genx.GenISA.RuntimeValue.v20i32(i32 0)
//      %17 = extractelement <20 x i32> %15, i32 %0
//      %25 = call <20 x i32> @llvm.genx.GenISA.RuntimeValue.v20i32(i32 0)
//      %26 = add i32 %0, 8
//      %27 = extractelement <20 x i32> %25, i32 %26
//
// 3) RuntimeValue calls returning single scalars are converted to extracts of elements
//    from corresponding RuntimeValue vector.
//    Replace:
//       %1 = call <3 x i32> @llvm.genx.GenISA.RuntimeValue.v3i32(i32 4)
//       %3 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 4)
//      %14 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 5)
//    with:
//       %4 = call <3 x i32> @llvm.genx.GenISA.RuntimeValue.v3i32(i32 4)
//       %1 = call <3 x i32> @llvm.genx.GenISA.RuntimeValue.v3i32(i32 4)
//       %2 = extractelement <3 x i32> %1, i32 0
//      %15 = call <3 x i32> @llvm.genx.GenISA.RuntimeValue.v3i32(i32 4)
//      %16 = extractelement <3 x i32> %15, i32 1
//
// Only RuntimeValue vectors of 32-bit elements are supported at the moment.
bool RuntimeValueLegalizationPass::runOnModule(llvm::Module& module)
{
    bool shaderModified = false;

    uint32_t dataGRFAlignmentInDwords =
        getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->platform.getGRFSize() / 4;

    RuntimeValueCollection runtimeValueCalls(RuntimeValueComparator);
    bool legalizationCheckNeeded =
        GetAllRuntimeValueCalls(module, runtimeValueCalls, dataGRFAlignmentInDwords);

    if (legalizationCheckNeeded)
    {
        // Get a map of accessed regions of form:
        // {offset { enclosing_region_start_offset, enclosing_region_size }}
        // for example: {0, {0, 2}}, {1, {0, 2}}, {4, {4, 1}}
        std::map<uint32_t, std::pair<uint32_t, uint32_t>> accessedRegions;
        GetAccessedRegions(runtimeValueCalls, accessedRegions);

        // Loop through all RuntimeValue calls
        for (auto it : runtimeValueCalls)
        {
            llvm::CallInst* callToResolve = llvm::cast<llvm::CallInst>(it.second);

            IGCLLVM::FixedVectorType* const fixedVectorTy =
                llvm::dyn_cast<IGCLLVM::FixedVectorType>(callToResolve->getType());

            uint32_t resolvedOffset = int_cast<uint32_t>(
                cast<ConstantInt>(callToResolve->getArgOperand(0))->getZExtValue());
            uint32_t resolvedSize = fixedVectorTy ? int_cast<uint32_t>(fixedVectorTy->getNumElements()) : 1;

            // Find corresponding region
            auto regionIter = accessedRegions.find(resolvedOffset);
            IGC_ASSERT(regionIter != accessedRegions.end());
            uint32_t regionOffset = regionIter->second.first;
            uint32_t regionSize = regionIter->second.second;

            // Check if RuntimeValue needs adjustment
            if ((resolvedOffset != regionOffset) || (resolvedSize != regionSize))
            {
                llvm::IRBuilder<> builder(callToResolve);

                llvm::Type* resolvedBaseType = fixedVectorTy ? fixedVectorTy->getElementType() : callToResolve->getType();
                llvm::Type* vectorType = IGCLLVM::FixedVectorType::get(resolvedBaseType, regionSize);
                Function* runtimeValueFunc = GenISAIntrinsic::getDeclaration(&module,
                    GenISAIntrinsic::GenISA_RuntimeValue,
                    vectorType);

                // Create new RuntimeValue call
                Value* newValue = builder.CreateCall(runtimeValueFunc, builder.getInt32(regionOffset));

                uint32_t eeOffset = resolvedOffset - regionOffset;

                if (fixedVectorTy)
                {
                    // RuntimeValue calls representing vectors of scalars are rewritten due to offset/size change.
                    // Thus related extract element instructions should be adjusted too.
                    std::vector<llvm::User*> users(callToResolve->user_begin(), callToResolve->user_end());
                    for (llvm::User* const user : users)
                    {
                        llvm::ExtractElementInst* EEI = llvm::dyn_cast<llvm::ExtractElementInst>(user);
                        IGC_ASSERT(EEI);
                        if (EEI)
                        {
                            builder.SetInsertPoint(EEI);
                            EEI->setOperand(0, newValue);
                            EEI->setOperand(1, builder.CreateAdd(EEI->getIndexOperand(), builder.getInt32(eeOffset)));
                        }
                    }
                }
                else
                {
                    // RuntimeValue calls returning single scalars are converted to extracts of elements
                    // from corresponding RuntimeValue vector
                    newValue = builder.CreateExtractElement(newValue, builder.getInt32(eeOffset));
                    callToResolve->replaceAllUsesWith(newValue);
                }

                callToResolve->eraseFromParent();
                shaderModified = true;
            }
        }
    }
    return shaderModified;
}

}
