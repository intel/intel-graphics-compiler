/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ScalarArgAsPointer/ScalarArgAsPointer.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include "llvm/Support/Debug.h"
#include "common/LLVMWarningsPop.hpp"

#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

#define DEBUG_TYPE "igc-scalar-arg-as-pointer-analysis"

// Register pass to igc-opt
#define PASS_FLAG "igc-scalar-arg-as-pointer-analysis"
#define PASS_DESCRIPTION "Analyzes scalar kernel arguments used for global memory access"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ScalarArgAsPointerAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(ScalarArgAsPointerAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ScalarArgAsPointerAnalysis::ID = 0;

ScalarArgAsPointerAnalysis::ScalarArgAsPointerAnalysis() : ModulePass(ID)
{
    initializeScalarArgAsPointerAnalysisPass(*PassRegistry::getPassRegistry());
}

bool ScalarArgAsPointerAnalysis::runOnModule(Module& M)
{
    DL = &M.getDataLayout();

    MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    bool changed = false;

    for (Function& F : M)
    {
        if (F.isDeclaration())
            continue;

        if (!isEntryFunc(MDU, &F))
            continue;

        changed |= analyzeFunction(F);
    }

    // Update LLVM metadata based on IGC MetadataUtils
    if (changed)
        MDU->save(M.getContext());

    return changed;
}

bool ScalarArgAsPointerAnalysis::analyzeFunction(llvm::Function& F)
{
    m_matchingArgs.clear();
    m_visitedInst.clear();
    m_allocas.clear();
    m_currentFunction = &F;

    LLVM_DEBUG(
        dbgs() << "running for function " << F.getName() << "\n");

    visit(F);

    if (m_matchingArgs.empty())
        return false;

    FunctionMetaData& funcMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData()->FuncMD[&F];

    for (auto it = m_matchingArgs.begin(); it != m_matchingArgs.end(); ++it)
        funcMD.m_OpenCLArgScalarAsPointers.insert((*it)->getArgNo());

    return true;
}

void ScalarArgAsPointerAnalysis::visitStoreInst(llvm::StoreInst& I)
{
    analyzeStoredArg(I);
    analyzePointer(I.getPointerOperand());
}

void ScalarArgAsPointerAnalysis::visitLoadInst(llvm::LoadInst& I)
{
    analyzePointer(I.getPointerOperand());
}

void ScalarArgAsPointerAnalysis::visitCallInst(CallInst& CI)
{
    if (auto I = dyn_cast<GenIntrinsicInst>(&CI))
        return visitGenIntrinsic(*I);

    // If function takes pointer as argument, assume it is used for access.
    for (auto it = CI.op_begin(); it != CI.op_end(); ++it)
    {
        if (isa< PointerType>((*it)->getType()))
            analyzePointer(*it);
    }
}

void ScalarArgAsPointerAnalysis::visitGenIntrinsic(GenIntrinsicInst& I)
{
    GenISAIntrinsic::ID const id = I.getIntrinsicID();

    if (id == GenISAIntrinsic::GenISA_LSC2DBlockRead ||
        id == GenISAIntrinsic::GenISA_LSC2DBlockPrefetch ||
        id == GenISAIntrinsic::GenISA_LSC2DBlockWrite)
    {
        return analyzeValue(I.getOperand(0));
    }

    if (IsStatelessMemLoadIntrinsic(id) ||
        IsStatelessMemStoreIntrinsic(id) ||
        IsStatelessMemAtomicIntrinsic(I, id))
    {
        Value* V = GetBufferOperand(&I);

        if (!V || !isa<PointerType>(V->getType()))
            return;

        return analyzePointer(V);
    }
}

void ScalarArgAsPointerAnalysis::analyzePointer(llvm::Value* V)
{
    auto* type = dyn_cast<PointerType>(V->getType());

    IGC_ASSERT_MESSAGE(type, "Value should be a pointer");

    if (type->getAddressSpace() != ADDRESS_SPACE_GLOBAL && type->getAddressSpace() != ADDRESS_SPACE_GENERIC)
        return;

    analyzeValue(V);
}

void ScalarArgAsPointerAnalysis::analyzeValue(llvm::Value* V)
{
    std::shared_ptr<ScalarArgAsPointerAnalysis::ArgSet> args;

    if (auto* I = dyn_cast<Instruction>(V))
    {
        args = findArgs(I);
    }
    else
    {
        args = analyzeOperand(V);
    }

    if (args)
    {
        LLVM_DEBUG(
            for (auto a : *args)
            {
                dbgs() << "  access from pointer ";
                V->printAsOperand(dbgs(), false);
                dbgs() << " tracks to argument ";
                a->printAsOperand(dbgs(), false);
                dbgs() << "\n";
            });
        m_matchingArgs.insert(args->begin(), args->end());
    }
}

const std::shared_ptr<ScalarArgAsPointerAnalysis::ArgSet>
ScalarArgAsPointerAnalysis::findArgs(llvm::Instruction* inst)
{
    // Skip already visited instruction
    if (m_visitedInst.count(inst))
        return m_visitedInst[inst];

    // Mark as visited
    m_visitedInst.try_emplace(inst, nullptr);

    // Assume intrinsic are safe simple arithmetics.
    if (isa<CallInst>(inst) && !isa<GenIntrinsicInst>(inst))
        return nullptr;

    auto result = std::make_shared<ScalarArgAsPointerAnalysis::ArgSet>();

    if (LoadInst* LI = dyn_cast<LoadInst>(inst))
    {
        if (!findStoredArgs(*LI, *result))
            return nullptr; // (1) Found indirect access, fail search
    }
    else
    {
        // Iterate and trace back operands.
        auto begin = inst->operands().begin();
        auto end = inst->operands().end();

        if (isa<SelectInst>(inst))
        {
            // For select, skip condition operand (first arg)
            begin++;
        }
        else if (isa<GetElementPtrInst>(inst))
        {
            // For GEP, use only base pointer operand (first arg)
            end = begin + 1;
        }

        for (auto it = begin; it != end; ++it)
        {
            auto args = analyzeOperand(*it);

            if (args)
                result->insert(args->begin(), args->end());
        }

        if (result->empty())
            return nullptr; // propagate fail
    }

    m_visitedInst[inst] = result;
    return result;
}

const std::shared_ptr<ScalarArgAsPointerAnalysis::ArgSet>
ScalarArgAsPointerAnalysis::analyzeOperand(llvm::Value* op)
{
    auto result = std::make_shared<ScalarArgAsPointerAnalysis::ArgSet>();

    if (Argument* arg = dyn_cast<Argument>(op))
    {
        // Consider only integer arguments
        if (arg->getType()->getScalarType()->isIntegerTy())
        {
            result->insert(arg);
        }
        else
        {
            // (2) Found non-compatible argument, fail
            return nullptr;
        }
    }
    else if (Instruction* opInst = dyn_cast<Instruction>(op))
    {
        auto args = findArgs(opInst);

        if (!args)
            return nullptr; // propagate fail

        result->insert(args->begin(), args->end());
    }
    else if (GlobalValue* global = dyn_cast<GlobalValue>(op))
    {
        Argument* arg = analyzeGlobal(global);

        if (!arg)
            return nullptr; // propagate fail

        result->insert(arg);
    }

    return result;
}

llvm::Argument* ScalarArgAsPointerAnalysis::analyzeGlobal(llvm::GlobalValue* V)
{
    PointerType* type = dyn_cast<PointerType>(V->getType());
    if (!type)
        return nullptr;

    if (type->getAddressSpace() != ADDRESS_SPACE_GLOBAL)
        return nullptr;

    ImplicitArgs implicitArgs(*m_currentFunction, MDU);

    if (!implicitArgs.isImplicitArgExist(ImplicitArg::GLOBAL_BASE))
        return nullptr;

    if (m_currentFunction->arg_size() < implicitArgs.size())
        return nullptr;

    unsigned implicitArgsBaseIndex = m_currentFunction->arg_size() - implicitArgs.size();
    unsigned implicitArgIndex = implicitArgs.getArgIndex(ImplicitArg::GLOBAL_BASE);
    return std::next(m_currentFunction->arg_begin(), implicitArgsBaseIndex + implicitArgIndex);
}

void ScalarArgAsPointerAnalysis::analyzeStoredArg(llvm::StoreInst& SI)
{
    // Only track stores of kernel arguments.
    Argument* A = dyn_cast<Argument>(SI.getValueOperand());
    if (!A)
        return;

    AllocaInst* AI = nullptr;
    GetElementPtrInst* GEPI = nullptr;
    if (!findAllocaWithOffset(SI.getPointerOperand(), AI, GEPI))
        return;

    uint64_t totalOffset = 0;

    if (GEPI)
    {
        // For store instruction offset must be constant.
        APInt offset(DL->getIndexTypeSizeInBits(GEPI->getType()), 0);
        if (!GEPI->accumulateConstantOffset(*DL, offset) || offset.isNegative())
            return;
        totalOffset += offset.getZExtValue();
    }

    m_allocas[std::pair<llvm::AllocaInst*, uint64_t>(AI, totalOffset)] = A;
}

bool ScalarArgAsPointerAnalysis::findStoredArgs(llvm::LoadInst& LI, ArgSet& args)
{
    AllocaInst* AI = nullptr;
    GetElementPtrInst* GEPI = nullptr;
    if (!findAllocaWithOffset(LI.getPointerOperand(), AI, GEPI))
        return false;

    // It is possible one or more GEP operand is a variable index to array type.
    // In this case search for all possible offsets to alloca.
    using Offsets = SmallVector<uint64_t, 4>;
    Offsets offsets;
    offsets.push_back(0);

    if (GEPI)
    {
        for (gep_type_iterator GTI = gep_type_begin(GEPI), prevGTI = gep_type_end(GEPI); GTI != gep_type_end(GEPI); prevGTI = GTI++)
        {
            if (ConstantInt* C = dyn_cast<ConstantInt>(GTI.getOperand()))
            {
                if (C->isZero())
                    continue;

                uint64_t offset = 0;

                if (StructType* STy = GTI.getStructTypeOrNull())
                    offset = DL->getStructLayout(STy)->getElementOffset(int_cast<unsigned>(C->getZExtValue()));
                else
                    offset = C->getZExtValue() * DL->getTypeAllocSize(GTI.getIndexedType()); // array or vector

                for (auto it = offsets.begin(); it != offsets.end(); ++it)
                    *it += offset;
            }
            else
            {
                if (prevGTI == gep_type_end(GEPI))
                    return false; // variable index at first operand, should not happen

                // gep_type_iterator is used to query indexed type. For arrays this is type
                // of single element. To get array size, we need to do query for it at
                // previous iterator step (before stepping into type indexed by array).
                ArrayType* ATy = dyn_cast<ArrayType>(prevGTI.getIndexedType());
                if (!ATy)
                    return false;

                uint64_t arrayElements = ATy->getNumElements();

                uint64_t byteSize = DL->getTypeAllocSize(GTI.getIndexedType());

                Offsets tmp;
                for (auto i = 0; i < arrayElements; ++i)
                    for (auto it = offsets.begin(); it != offsets.end(); ++it)
                        tmp.push_back(*it + i * byteSize);

                offsets = std::move(tmp);
            }
        }
    }

    for (auto it = offsets.begin(); it != offsets.end(); ++it)
    {
        std::pair<llvm::AllocaInst*, uint64_t> key(AI, *it);
        if (m_allocas.count(key))
            args.insert(m_allocas[key]);
    }

    return !args.empty();
}

bool ScalarArgAsPointerAnalysis::findAllocaWithOffset(llvm::Value* V, llvm::AllocaInst*& outAI, llvm::GetElementPtrInst*& outGEPI)
{
    IGC_ASSERT_MESSAGE(dyn_cast<PointerType>(V->getType()), "Value should be a pointer");

    outGEPI = nullptr;
    Value* tmp = V;

    while (true)
    {
        if (BitCastInst* BCI = dyn_cast<BitCastInst>(tmp))
        {
            tmp = BCI->getOperand(0);
        }
        else if (GetElementPtrInst* GEPI = dyn_cast<GetElementPtrInst>(tmp))
        {
            if (outGEPI)
                return false; // only one GEP instruction is supported
            outGEPI = GEPI;
            tmp = GEPI->getPointerOperand();
        }
        else if (AllocaInst* AI = dyn_cast<AllocaInst>(tmp))
        {
            outAI = AI;
            return true;
        }
        else
        {
            return false;
        }
    }
}
