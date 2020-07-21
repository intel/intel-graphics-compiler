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
#include "DebugInfo.hpp"
#include "GenCodeGenModule.h"
#include "common/Types.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;
using namespace std;

char DebugInfoPass::ID = 0;
char CatchAllLineNumber::ID = 0;

DebugInfoPass::DebugInfoPass(CShaderProgram::KernelShaderMap& k) :
    ModulePass(ID),
    kernels(k)
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
    std::vector<CShader*> units;
    auto moduleMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    bool isOneStepElf = false;

    auto isCandidate = [](CShaderProgram* shaderProgram, SIMDMode m, ShaderDispatchMode mode = ShaderDispatchMode::NOT_APPLICABLE)
    {
        auto currShader = shaderProgram->GetShader(m, mode);
        if (!currShader || !currShader->diData)
            return (CShader*)nullptr;

        if (currShader->ProgramOutput()->m_programSize == 0)
            return (CShader*)nullptr;

        return currShader;
    };

    for (auto& k : kernels)
    {
        auto shaderProgram = k.second;
        auto simd8 = isCandidate(shaderProgram, SIMDMode::SIMD8);
        auto simd16 = isCandidate(shaderProgram, SIMDMode::SIMD16);
        auto simd32 = isCandidate(shaderProgram, SIMDMode::SIMD32);

        if (simd8) units.push_back(simd8);
        if (simd16) units.push_back(simd16);
        if (simd32) units.push_back(simd32);
    }

    for (auto& currShader : units)
    {
        // Look for the right CShaderProgram instance
        m_currShader = currShader;

        MetaDataUtils* pMdUtils = m_currShader->GetMetaDataUtils();
        if (!isEntryFunc(pMdUtils, m_currShader->entry))
            continue;

        bool isCloned = false;
        if (DebugInfoData::hasDebugInfo(m_currShader))
        {
            auto fIT = moduleMD->FuncMD.find(m_currShader->entry);
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

        // Sort modules in order of their placement in binary
        DbgDecoder decodedDbg(m_currShader->ProgramOutput()->m_debugDataGenISA);
        auto getGenOff = [&decodedDbg](std::vector<std::pair<unsigned int, unsigned int>>& data, unsigned int VISAIndex)
        {
            unsigned retval = 0;
            for (auto& item : data)
            {
                if (item.first == VISAIndex)
                {
                    retval = item.second;
                }
            }
            return retval;
        };

        auto getLastGenOff = [this, &decodedDbg, &getGenOff](IGC::VISAModule* v)
        {
            unsigned int genOff = 0;
            // Detect last instructions of kernel. This information is absent in
            // dbg info. So detect is as first instruction of first subroutine - 1.
            // reloc_index, first sub inst's VISA id
            std::unordered_map<uint32_t, unsigned int> firstSubVISAIndex;

            for (auto& item : decodedDbg.compiledObjs)
            {
                firstSubVISAIndex[item.relocOffset] = item.CISAIndexMap.back().first;
                for (auto& sub : item.subs)
                {
                    auto subStartVISAIndex = sub.startVISAIndex;
                    if (firstSubVISAIndex[item.relocOffset] > subStartVISAIndex)
                        firstSubVISAIndex[item.relocOffset] = subStartVISAIndex - 1;
                }
            }

            for (auto& item : decodedDbg.compiledObjs)
            {
                auto& name = item.kernelName;
                auto firstInst = (v->GetInstInfoMap()->begin())->first;
                auto funcName = firstInst->getParent()->getParent()->getName();
                if (item.subs.size() == 0 && funcName.compare(name) == 0)
                {
                    genOff = item.CISAIndexMap.back().second;
                }
                else
                {
                    if (funcName.compare(name) == 0)
                    {
                        genOff = getGenOff(item.CISAIndexMap, firstSubVISAIndex[item.relocOffset]);
                        break;
                    }
                    for (auto& sub : item.subs)
                    {
                        auto& subName = sub.name;
                        if (funcName.compare(subName) == 0)
                        {
                            genOff = getGenOff(item.CISAIndexMap, sub.endVISAIndex);
                            break;
                        }
                    }
                }

                if (genOff)
                    break;
            }

            return genOff;
        };

        auto setType = [&decodedDbg](VISAModule* v)
        {
            auto firstInst = (v->GetInstInfoMap()->begin())->first;
            auto funcName = firstInst->getParent()->getParent()->getName();

            for (auto& item : decodedDbg.compiledObjs)
            {
                auto& name = item.kernelName;
                if (funcName.compare(name) == 0)
                {
                    if (item.relocOffset == 0)
                        v->SetType(VISAModule::ObjectType::KERNEL);
                    else
                        v->SetType(VISAModule::ObjectType::STACKCALL_FUNC);
                    return;
                }
                for (auto& sub : item.subs)
                {
                    auto& subName = sub.name;
                    if (funcName.compare(subName) == 0)
                    {
                        v->SetType(VISAModule::ObjectType::SUBROUTINE);
                        return;
                    }
                }
            }
        };

        for (auto& m : m_currShader->diData->m_VISAModules)
        {
            setType(m.second);
            auto lastVISAId = getLastGenOff(m.second);
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
            m_pDebugEmitter->AddVISAModFunc(m.second.second, m.second.first);
        }

        for (auto& m : sortedVISAModules)
        {
            isOneStepElf |= m.second.second->isDirectElfInput;
            m_pDebugEmitter->SetVISAModule(m.second.second);
            m_pDebugEmitter->setFunction(m.second.first, isCloned);

            if (--size == 0)
                finalize = true;

            EmitDebugInfo(finalize);
        }

        // set VISA dbg info to nullptr to indicate 1-step debug is enabled
        if (isOneStepElf)
        {
            currShader->ProgramOutput()->m_debugDataGenISASize = 0;
            currShader->ProgramOutput()->m_debugDataGenISA = nullptr;
        }

        if (finalize)
        {
            IDebugEmitter::Release(m_pDebugEmitter);

            // destroy VISA builder
            auto encoder = &(m_currShader->GetEncoder());
            encoder->DestroyVISABuilder();
        }
    }

    return false;
}

void DebugInfoPass::EmitDebugInfo(bool finalize)
{
    unsigned int dbgSize = 0;
    void* dbgInfo = nullptr;
    void* buffer = nullptr;

    IF_DEBUG_INFO_IF(m_pDebugEmitter, m_pDebugEmitter->Finalize(buffer, dbgSize, finalize);)

        if (dbgSize != 0)
        {
            IGC_ASSERT_MESSAGE(nullptr != buffer, "Failed to generate VISA debug info");

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
    const Value* pVal = nullptr;
    if (const DbgDeclareInst * pDbgAddrInst = dyn_cast<DbgDeclareInst>(pInst))
    {
        pVal = pDbgAddrInst->getAddress();
    }
    else if (const DbgValueInst * pDbgValInst = dyn_cast<DbgValueInst>(pInst))
    {
        pVal = pDbgValInst->getValue();
    }
    else
    {
        return;
    }

    if (!pVal || isa<UndefValue>(pVal))
    {
        // No debug info value, return empty location!
        return;
    }

    if (dyn_cast<Constant>(pVal))
    {
        if (!isa<GlobalVariable>(pVal) && !isa<ConstantExpr>(pVal))
        {
            return;
        }
    }

    Value* pValue = const_cast<Value*>(pVal);
    if (isa<GlobalVariable>(pValue))
    {
        return;
    }

    if (!m_pShader->IsValueUsed(pValue)) {
        return;
    }

    CVariable* pVar = m_pShader->GetSymbol(pValue);
    if (pVar->GetVarType() == EVARTYPE_GENERAL)
    {
        // We want to attach "Output" attribute to all src variables
        // so that finalizer can extend their liveness to end of
        // the program. This will help debugger examine their
        // values anywhere in the code till they are in scope.
        if (m_outputVals.find(pVar) == m_outputVals.end())
        {
            //m_pShader->GetEncoder().GetVISAKernel()->AddAttributeToVar(pVar->visaGenVariable[0], "Output", 0, nullptr);
            (void)m_outputVals.insert(pVar);
        }
    }
}

CatchAllLineNumber::CatchAllLineNumber() :
    FunctionPass(ID)
{
    initializeMetaDataUtilsWrapperPass(*PassRegistry::getPassRegistry());
}

CatchAllLineNumber::~CatchAllLineNumber()
{
}

bool CatchAllLineNumber::runOnFunction(llvm::Function& F)
{
    // Insert placeholder intrinsic instruction in each kernel.
    if (!F.getSubprogram() || F.isDeclaration())
        return false;

    if (F.getCallingConv() != llvm::CallingConv::SPIR_KERNEL)
        return false;

    llvm::IRBuilder<> Builder(F.getParent()->getContext());
    DIBuilder di(*F.getParent());
    Function* lineNumPlaceholder = GenISAIntrinsic::getDeclaration(F.getParent(), GenISAIntrinsic::ID::GenISA_CatchAllDebugLine);
    auto intCall = Builder.CreateCall(lineNumPlaceholder);

    auto line = F.getSubprogram()->getLine();
    auto scope = F.getSubprogram();

    auto dbg = DILocation::get(F.getParent()->getContext(), line, 0, scope);

    intCall->setDebugLoc(dbg);

    intCall->insertBefore(&*F.getEntryBlock().getFirstInsertionPt());

    return true;
}