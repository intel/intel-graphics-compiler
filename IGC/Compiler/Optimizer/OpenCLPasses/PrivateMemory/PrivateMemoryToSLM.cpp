/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Probe/Assertion.h"

#include "Compiler/Optimizer/OpenCLPasses/PrivateMemory/PrivateMemoryToSLM.hpp"

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "Compiler/ModuleAllocaAnalysis.hpp"

#include "common/debug/Debug.hpp"
#include "llvmWrapper/IR/DataLayout.h"
#include "llvmWrapper/Support/Alignment.h"
#include "llvmWrapper/IR/IRBuilder.h"

#include <fstream>
#include <sstream>

using namespace IGC;
using namespace IGC::IGCMD;
using namespace IGC::Debug;

#define PASS_FLAG "igc-move-private-memory-to-slm"
#define PASS_DESCRIPTION "Move private memory allocations to SLM"
#define PASS_CFG_ONLY true
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PrivateMemoryToSLM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(ModuleAllocaAnalysis)
IGC_INITIALIZE_PASS_END(PrivateMemoryToSLM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace IGC
{
    char PrivateMemoryToSLM::ID = 0;
    const unsigned int PrivateMemoryToSLM::SLM_LOCAL_VARIABLE_ALIGNMENT = 4;
    const unsigned int PrivateMemoryToSLM::SLM_LOCAL_SIZE_ALIGNMENT = 32;

    // Empty constructor to force moving of all eligible allocations.
    PrivateMemoryToSLM::PrivateMemoryToSLM(bool enableOptReport /* = false */) :
                                           ModulePass(ID),
                                           m_ForceAll(true),
                                           m_EnableOptReport(enableOptReport)
    {
        initializePrivateMemoryToSLMPass(*PassRegistry::getPassRegistry());
    }

    PrivateMemoryToSLM::PrivateMemoryToSLM(std::string forcedBuffers,
                                           bool enableOptReport) :
                                           ModulePass(ID),
                                           m_ForceAll(false),
                                           m_EnableOptReport(enableOptReport)
    {
        // Parse semocolon-separated list of forced buffers.
        const char* SEPARATORS = ";";
        std::size_t pos = 0;
        std::size_t found;
        while ((found = forcedBuffers.find_first_of(SEPARATORS, pos)) != std::string::npos) {
            if (found != pos) {
                m_ForcedBuffers.push_back(forcedBuffers.substr(pos, found - pos));
                pos = found;
            }
            ++pos;
        }
        if (pos < forcedBuffers.length()) m_ForcedBuffers.push_back(forcedBuffers.substr(pos));

        initializePrivateMemoryToSLMPass(*PassRegistry::getPassRegistry());
    }

    void emitOptReport(std::string report)
    {
        std::stringstream optReportFile;
        optReportFile << IGC::Debug::GetShaderOutputFolder() << "PrivateMemoryToSLM.opt";

        std::ofstream optReportStream;
        optReportStream.open(optReportFile.str(), std::ios::app);
        optReportStream << report;
    }

    // TODO: Unify with the original predicate from InlineLocalsResolution.cpp
    static bool useAsPointerOnly(Value* V) {
        IGC_ASSERT_MESSAGE(V->getType()->isPointerTy(), "Expect the input value is a pointer!");

        SmallSet<PHINode*, 8> VisitedPHIs;
        SmallVector<Value*, 16> WorkList;
        WorkList.push_back(V);

        StoreInst* ST = nullptr;
        PHINode* PN = nullptr;
        while (!WorkList.empty()) {
            Value* Val = WorkList.pop_back_val();
            for (auto* U : Val->users()) {
                Operator* Op = dyn_cast<Operator>(U);
                if (!Op)
                    continue;
                switch (Op->getOpcode()) {
                default:
                    // Bail out for unknown operations.
                    return false;
                case Instruction::Store:
                    ST = cast<StoreInst>(U);
                    // Bail out if it's used as the value operand.
                    if (ST->getValueOperand() == Val)
                        return false;
                    // FALL THROUGH
                case Instruction::Load:
                    // Safe use in LD/ST as pointer only.
                    continue;
                case Instruction::PHI:
                    PN = cast<PHINode>(U);
                    // Skip if it's already visited.
                    if (VisitedPHIs.count(PN))
                        continue;
                    VisitedPHIs.insert(PN);
                    // FALL THROUGH
                case Instruction::BitCast:
                case Instruction::Select:
                case Instruction::GetElementPtr:
                    // Need to check their usage further.
                    break;
                }
                WorkList.push_back(U);
            }
        }

        return true;
    }

    bool PrivateMemoryToSLM::runOnModule(Module& M)
    {
        auto* CodeGenCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        auto* MD = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
        auto* ModuleMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
        auto DL = M.getDataLayout();

        bool modified = false;
        for (Module::iterator I = M.begin(); I != M.end(); ++I)
        {
            Function* F = &*I;

            if (F->isDeclaration())
            {
                continue;
            }

            SmallVector<AllocaInst*, 8>& allocaInsts = getAnalysis<ModuleAllocaAnalysis>().getAllocaInsts(F);

            if (allocaInsts.empty())
            {
                // No alloca instructions to process.
                continue;
            }

            FunctionInfoMetaDataHandle funcMD = MD->getFunctionsInfoItem(F);

            uint64_t xDim = 0;
            uint64_t yDim = 0;
            uint64_t zDim = 0;

            if (CodeGenCtx->type == ShaderType::OPENCL_SHADER)
            {
                ThreadGroupSizeMetaDataHandle threadGroupSize = funcMD->getThreadGroupSize();
                xDim = threadGroupSize->getXDim();
                yDim = threadGroupSize->getYDim();
                zDim = threadGroupSize->getZDim();
            }

            uint64_t threadsNum = xDim * yDim * zDim;

            if (threadsNum == 0)
            {
                continue;
            }

            uint64_t slmSizePerSubslice = 0;

            if (CodeGenCtx->type == ShaderType::OPENCL_SHADER)
            {
                OpenCLProgramContext* oclCtx = static_cast<OpenCLProgramContext*>(CodeGenCtx);
                slmSizePerSubslice = oclCtx->GetSlmSizePerSubslice();
            }


            // Calculate an offset for new SLM variables.
            unsigned int offset = 0;
            for (const auto &offsets : ModuleMD->FuncMD[F].localOffsets)
            {
                Type* varType = offsets.m_Var->getValueType();
                offset = iSTD::Align(offset, IGCLLVM::getPreferredAlignValue(&DL, offsets.m_Var));
                offset += (unsigned int) DL.getTypeAllocSize(varType);
            }

            if (m_EnableOptReport)
            {
                std::stringstream report;
                report << "Function" << F->getName().str() << std::endl
                    << "Workgroup size: " << threadsNum << ", X: " << xDim << ", Y:" << yDim << ", Z:" << zDim << std::endl
                    << "SLM size per subslice: " << slmSizePerSubslice << ", used " << offset << " bytes" << std::endl;

                ods() << report.str();
                emitOptReport(report.str());
            }

            // This declaration will invoke constructor of DebugLoc class
            // and result in an empty DebugLoc instance, ie with line and scope set to 0.
            DebugLoc emptyDebugLoc;

            LLVMContext& llvmCtx = F->getContext();
            IntegerType* typeInt32 = Type::getInt32Ty(llvmCtx);

            ImplicitArgs implicitArgs(*F, MD);

            Instruction* pEntryPoint = &(*F->getEntryBlock().getFirstInsertionPt());

            for (auto pAI : allocaInsts)
            {
                bool isForcedBuffer =
                    std::find(m_ForcedBuffers.begin(),
                              m_ForcedBuffers.end(),
                              pAI->getName()) != m_ForcedBuffers.end();

                if (m_ForceAll || isForcedBuffer)
                {
                    Type* origType = pAI->getAllocatedType();
                    bool isArray = origType->isArrayTy();
                    Type* eltType = isArray ? origType->getArrayElementType() : origType;
                    uint64_t numEltsPerThread = isArray ? origType->getArrayNumElements() : 1;
                    uint64_t numEltsPerWorkgroup = numEltsPerThread * threadsNum;
                    Type* newType = ArrayType::get(eltType, numEltsPerWorkgroup);
                    unsigned int allocSize = (unsigned int)DL.getTypeAllocSize(newType);

                    unsigned int newOffset = offset;
                    newOffset = iSTD::Align(newOffset, SLM_LOCAL_VARIABLE_ALIGNMENT);
                    newOffset += allocSize;
                    newOffset = iSTD::Align(newOffset, SLM_LOCAL_SIZE_ALIGNMENT);

                    if (newOffset > slmSizePerSubslice)
                    {
                        if (m_EnableOptReport)
                        {
                            std::stringstream report;
                            report << "Skip moving a memory allocation " << pAI->getName().str()
                                << " of " << allocSize << " bytes"
                                << " to SLM, not enough available SLM" << std::endl;

                            ods() << report.str();
                            emitOptReport(report.str());
                        }

                        continue;
                    }

                    if (m_EnableOptReport)
                    {
                        std::stringstream report;
                        report << "Moving a memory allocation " << pAI->getName().str()
                            << " of " << allocSize << " bytes"
                            << " to SLM, new SLM usage " << newOffset << " bytes" << std::endl;

                        ods() << report.str();
                        emitOptReport(report.str());
                    }

                    auto slmVar = new GlobalVariable(
                        M,
                        newType,
                        /* isConstant */ false,
                        GlobalValue::ExternalLinkage,
                        UndefValue::get(newType),
                        F->getName() + "." + pAI->getName(),
                        /* InsertBefore */ nullptr,
                        GlobalVariable::ThreadLocalMode::NotThreadLocal,
                        ADDRESS_SPACE_LOCAL);
                    slmVar->setAlignment(IGCLLVM::getCorrectAlign(SLM_LOCAL_VARIABLE_ALIGNMENT));
                    slmVar->setDSOLocal(false);
                    slmVar->setSection("localSLM");

                    // TODO: optimize on x-y-z values
                    IGCLLVM::IRBuilder<> builder(pAI);

                    builder.SetCurrentDebugLocation(emptyDebugLoc);

                    // totalOffset = localIdX +
                    //               localIdY * dimX +
                    //               localIdZ * dimX * dimY;

                    Value* dimX = ConstantInt::get(typeInt32, xDim);
                    Value* dimXY = ConstantInt::get(typeInt32, xDim * yDim);

                    Value* localIdX = nullptr;
                    Value* localIdY = nullptr;
                    Value* localIdZ = nullptr;

                    if (CodeGenCtx->type == ShaderType::OPENCL_SHADER)
                    {
                        localIdX =
                            ZExtInst::CreateIntegerCast(
                                implicitArgs.getImplicitArgValue(*F, ImplicitArg::LOCAL_ID_X, MD),
                                typeInt32,
                                false,
                                VALUE_NAME("localIdX"),
                                pEntryPoint);
                        localIdY =
                            ZExtInst::CreateIntegerCast(
                                implicitArgs.getImplicitArgValue(*F, ImplicitArg::LOCAL_ID_Y, MD),
                                typeInt32,
                                false,
                                VALUE_NAME("localIdY"),
                                pEntryPoint);
                        localIdZ =
                            ZExtInst::CreateIntegerCast(
                                implicitArgs.getImplicitArgValue(*F, ImplicitArg::LOCAL_ID_Z, MD),
                                typeInt32,
                                false,
                                VALUE_NAME("localIdZ"),
                                pEntryPoint);
                    }

                    Value* xOffset = localIdX;
                    Value* yOffset =
                        builder.CreateMul(
                            dimX,
                            localIdY,
                            VALUE_NAME(pAI->getName() + ".yOffset"));
                    Value* zOffset =
                        builder.CreateMul(
                            dimXY,
                            localIdZ,
                            VALUE_NAME(pAI->getName() + ".zOffset"));

                    Value* totalOffset =
                        builder.CreateAdd(
                            xOffset,
                            builder.CreateAdd(
                                yOffset,
                                zOffset),
                            VALUE_NAME(pAI->getName() + ".totalOffset"));

                    Value* cast = ConstantExpr::getAddrSpaceCast(slmVar, pAI->getType());
                    Value* ptr = builder.CreateGEP(pAI->getAllocatedType(), cast, totalOffset);

                    pAI->replaceAllUsesWith(ptr);
                    pAI->eraseFromParent();

                    // Add new SLM variable offset to MD.
                    LocalOffsetMD localOffset;
                    localOffset.m_Var = slmVar;
                    localOffset.m_Offset = useAsPointerOnly(slmVar) ? (offset & LOW_BITS_MASK) : ((offset & LOW_BITS_MASK) | VALID_LOCAL_HIGH_BITS);
                    ModuleMD->FuncMD[F].localOffsets.push_back(localOffset);

                    // Update total SLM usage MD.
                    offset = newOffset;
                    ModuleMD->FuncMD[F].localSize = newOffset;

                    modified = true;
                }
            }
        }

        return modified;
    }
}
