/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/PrivateMemory/PrivateMemoryBufferAnalysis.hpp"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-private-mem-buffer-analysis"
#define PASS_DESCRIPTION "Analizes the size and offset of private mem allocas and the total per WI size"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(PrivateMemoryBufferAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
//IGC_INITIALIZE_PASS_DEPENDENCY(DataLayout)
IGC_INITIALIZE_PASS_END(PrivateMemoryBufferAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char PrivateMemoryBufferAnalysis::ID = 0;

PrivateMemoryBufferAnalysis::PrivateMemoryBufferAnalysis() : ModulePass(ID)
{
    initializePrivateMemoryBufferAnalysisPass(*PassRegistry::getPassRegistry());
}

bool PrivateMemoryBufferAnalysis::runOnModule(llvm::Module& M)
{
    // Get the analysis
    m_DL = &M.getDataLayout();

    // Clear data for processing new module
    m_privateInfoMap.clear();

    for (Module::iterator I = M.begin(); I != M.end(); ++I)
    {
        runOnFunction(*I);
    }

    return false;
}

void PrivateMemoryBufferAnalysis::runOnFunction(llvm::Function& F)
{
    if (F.isDeclaration())
    {
        return;
    }

    // Processing new function
    m_currentOffset = 0;
    m_maxAlignment = 0;

    visit(F);

    // Align total size for the next WI
    m_currentOffset = iSTD::Align(m_currentOffset, m_maxAlignment);

    // Map total private buffer size to current function
    m_privateInfoMap[&F].m_bufferTotalSize = m_currentOffset;
}

void PrivateMemoryBufferAnalysis::visitAllocaInst(AllocaInst& AI)
{
    IGC_ASSERT_MESSAGE(AI.getType()->getAddressSpace() == ADDRESS_SPACE_PRIVATE, "Allocaitons are expected to be in private address space");

    // If private memory has no users, no point of analysing it.
    if (AI.use_empty()) return;

    Function* pFunc = AI.getParent()->getParent();

    m_privateInfoMap[pFunc].m_allocaInsts.push_back(&AI);

    unsigned int alignment = (unsigned)AI.getAlignment();
    if (alignment == 0) {
        alignment = (unsigned)m_DL->getABITypeAlignment(AI.getAllocatedType());
    }

    // Update max alignment
    if (alignment > m_maxAlignment)
    {
        m_maxAlignment = alignment;
    }

    // Determine buffer offset
    m_currentOffset = iSTD::Align(m_currentOffset, alignment);
    m_privateInfoMap[pFunc].m_bufferOffsets[&AI] = m_currentOffset;

    // Determine buffer size
    IGC_ASSERT_MESSAGE(isa<ConstantInt>(AI.getArraySize()), "Private memory array size is expected to be constant int!");
    unsigned int bufferSize = static_cast<unsigned int>(cast<ConstantInt>(AI.getArraySize())->getZExtValue() * m_DL->getTypeAllocSize(AI.getAllocatedType()));
    m_privateInfoMap[pFunc].m_bufferSizes[&AI] = bufferSize;

    // Advance offset
    m_currentOffset += bufferSize;
}
