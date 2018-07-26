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


======================= end_copyright_notice ==================================*/#include "DebugInfo.hpp"
#include "GenCodeGenModule.h"
#include "common/Types.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;
using namespace std;

char DebugInfoPass::ID = 0;

DebugInfoPass::DebugInfoPass(CShaderProgram::KernelShaderMap &k, SIMDMode m) :
    ModulePass(ID),
    kernels(k),
    mode(m)
{
    initializeMetaDataUtilsWrapperPass(*PassRegistry::getPassRegistry());
}

DebugInfoPass::~DebugInfoPass()
{
}

bool DebugInfoPass::doInitialization(llvm::Module& M)
{
    return true;
}

bool DebugInfoPass::doFinalization(llvm::Module& M)
{
    return true;
}

bool DebugInfoPass::runOnModule(llvm::Module& M)
{
    for (auto& F : M)
    {
        // Look for the right CShaderProgram instance
        auto it = kernels.find(&F);
        if (it == kernels.end())
            continue;

        auto shaderProgram = (*it).second;
        m_currShader = shaderProgram->GetShader(mode);

        if (!m_currShader || !m_currShader->diData)
            continue;

        auto programSize = m_currShader->ProgramOutput()->m_programSize;
        if (programSize == 0)
            continue;

        auto moduleMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

        bool isCloned = false;
        if (DebugInfoData::hasDebugInfo(m_currShader))
        {
            auto fIT = moduleMD->FuncMD.find(&F);
            if (fIT != moduleMD->FuncMD.end() &&
                (*fIT).second.isCloned)
            {
                isCloned = true;
            }
        }

        bool finalize = false;
        unsigned int size = m_currShader->diData->m_VISAModules.size();
        m_pDebugEmitter = m_currShader->diData->m_pDebugEmitter;
        std::vector<std::pair<unsigned int, std::pair<llvm::Function*, IGC::VISAModule*>>> sortedVISAModules;

        for (auto& m : m_currShader->diData->m_VISAModules)
        {
            auto lastVISAId = m.second->GetCurrentVISAId();
            sortedVISAModules.push_back(std::make_pair(lastVISAId, std::make_pair(m.first, m.second)));
        }

        std::sort(sortedVISAModules.begin(), sortedVISAModules.end(),
            [](std::pair<unsigned int, std::pair<llvm::Function*, IGC::VISAModule*>>& p1,
                std::pair<unsigned int, std::pair<llvm::Function*, IGC::VISAModule*>>& p2)
        {
            return p1.first < p2.first;
        });

        for (auto& m : sortedVISAModules)
        {
            m_pDebugEmitter->SetVISAModule(m.second.second);
            m_pDebugEmitter->setFunction(m.second.first, isCloned);

            if (--size == 0)
                finalize = true;

            EmitDebugInfo(finalize);
        }

        if (finalize)
        {
            IDebugEmitter::Release(m_pDebugEmitter);

            // destroy VISA builder
            auto encoder = &(m_currShader->GetEncoder());
            encoder->DestroyVISABuilder();
        }

        break;
    }

    return false;
}

void DebugInfoPass::EmitDebugInfo(bool finalize)
{
    unsigned int dbgSize = 0;
    void *dbgInfo = nullptr;
    void *buffer = nullptr;

    IF_DEBUG_INFO_IF(m_pDebugEmitter, m_pDebugEmitter->Finalize(buffer, dbgSize, finalize);)

    if (dbgSize != 0)
    {
        assert(buffer != nullptr && "Failed to generate VISA debug info");

        dbgInfo = IGC::aligned_malloc(dbgSize, sizeof(void*));

        memcpy_s(dbgInfo, dbgSize, buffer, dbgSize);

        IF_DEBUG_INFO_IF(m_pDebugEmitter, m_pDebugEmitter->Free(buffer);)

        if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
        {
            std::string debugFileNameStr = IGC::Debug::GetDumpName(m_currShader, "elf");

            // Try to create the directory for the file (it might not already exist).
            if (iSTD::ParentDirectoryCreate(debugFileNameStr.c_str()) == 0)
            {
                void* dbgFile = iSTD::FileOpen(debugFileNameStr.c_str(), "wb+");
                if (dbgFile != nullptr)
                {
                    iSTD::FileWrite(dbgInfo, dbgSize, 1, dbgFile);
                    iSTD::FileClose(dbgFile);
                }
            }
        }
    }

    SProgramOutput* pOutput = m_currShader->ProgramOutput();
    pOutput->m_debugDataVISA = dbgInfo;
    pOutput->m_debugDataVISASize = dbgSize;
}

void DebugInfoData::markOutput(llvm::Function& F, CShader* m_currShader)
{
    for (auto& bb : F)
    {
        for (auto& pInst : bb)
        {
            markOutputVars(&pInst);
        }
    }
}

void DebugInfoData::markOutputVars(const llvm::Instruction* pInst)
{
    const Value * pVal = nullptr;
    if (const DbgDeclareInst *pDbgAddrInst = dyn_cast<DbgDeclareInst>(pInst))
    {
        pVal = pDbgAddrInst->getAddress();
    }
    else if (const DbgValueInst *pDbgValInst = dyn_cast<DbgValueInst>(pInst))
    {
        pVal = pDbgValInst->getValue();
    }
    else
    {
        return;
    }

    Value *pValue = const_cast<Value*>(pVal);
    CVariable *pVar = m_pShader->GetSymbol(pValue);
    if(pVar->GetVarType() == EVARTYPE_GENERAL)
    {
        // We want to attach "Output" attribute to all src variables
        // so that finalizer can extend their liveness to end of
        // the program. This will help debugger examine their
        // values anywhere in the code till they are in scope.
        if (m_outputVals.find(pVar) == m_outputVals.end())
        {
            m_pShader->GetEncoder().GetVISAKernel()->AddAttributeToVar(pVar->visaGenVariable[0], "Output", 0, nullptr);
            (void)m_outputVals.insert(pVar);
        }
    }
}