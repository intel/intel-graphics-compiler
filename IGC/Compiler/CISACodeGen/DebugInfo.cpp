/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Probe/Assertion.h"

#include "DebugInfo/ScalarVISAModule.h"
#include "DebugInfo/DwarfDebug.hpp"
#include "DebugInfo/VISADebugInfo.hpp"
#include "Compiler/CISACodeGen/DebugInfo.hpp"
#include "llvm/IR/IntrinsicInst.h"

#include "llvmWrapper/IR/IntrinsicInst.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;
using namespace std;
// ElfReader related typedefs
using namespace CLElfLib;

char DebugInfoPass::ID = 0;
char CatchAllLineNumber::ID = 0;



// Register pass to igc-opt
#define PASS_FLAG1 "igc-debug-finalize"
#define PASS_DESCRIPTION1 "DebugInfo pass, llvmIR part(WAs)"
#define PASS_CFG_ONLY1 false
#define PASS_ANALYSIS1 false

IGC_INITIALIZE_PASS_BEGIN(DebugInfoPass, PASS_FLAG1, PASS_DESCRIPTION1, PASS_CFG_ONLY1, PASS_ANALYSIS1)
IGC_INITIALIZE_PASS_END(DebugInfoPass, PASS_FLAG1, PASS_DESCRIPTION1, PASS_CFG_ONLY1, PASS_ANALYSIS1)

// Used for opt testing, could be removed if KernelShaderMap is moved to ctx.
static CShaderProgram::KernelShaderMap KernelShaderMap;

// Default ctor used for igc-opt testing
DebugInfoPass::DebugInfoPass() :
    ModulePass(ID),
    kernels(KernelShaderMap)
{ }


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
    // This loop is just a workaround till we add support for DIArgList metadata.
    // If we implement DIArgList support, it should be deleted.
    for (auto &F : M) {
      for (auto &BB : F) {
        for (auto &I : BB) {
          if (auto *dbgInst = dyn_cast<DbgVariableIntrinsic>(&I)) {
            if (dbgInst->getNumVariableLocationOps() > 1) {
              IGCLLVM::setKillLocation(dbgInst);
            }
          }
        }
      }
    }
    // Early out
    if (kernels.empty())
       return false;

    std::vector<CShader*> units;

    auto isCandidate = [](CShaderProgram* shaderProgram, SIMDMode m, ShaderDispatchMode mode = ShaderDispatchMode::NOT_APPLICABLE)
    {
        auto currShader = shaderProgram->GetShader(m, mode);
        if (!currShader || !currShader->GetDebugInfoData().m_pDebugEmitter)
            return (CShader*)nullptr;

        if (currShader->ProgramOutput()->m_programSize == 0)
            return (CShader*)nullptr;

        return currShader;
    };

    for (auto& k : kernels)
    {
        auto shaderProgram = k.second;
        auto simd8 = isCandidate(shaderProgram, SIMDMode::SIMD8);
        auto quadSimd8Dynamic = isCandidate(shaderProgram, SIMDMode::SIMD32, ShaderDispatchMode::QUAD_SIMD8_DYNAMIC);
        auto simd16 = isCandidate(shaderProgram, SIMDMode::SIMD16);
        auto simd32 = isCandidate(shaderProgram, SIMDMode::SIMD32);

        if (simd8) units.push_back(simd8);
        if (simd16) units.push_back(simd16);
        if (simd32) units.push_back(simd32);
        if (quadSimd8Dynamic) units.push_back(quadSimd8Dynamic);
    }

    DwarfDISubprogramCache DISPCache;

    for (auto& currShader : units)
    {
        // Look for the right CShaderProgram instance
        m_currShader = currShader;

        MetaDataUtils* pMdUtils = m_currShader->GetMetaDataUtils();
        if (!isEntryFunc(pMdUtils, m_currShader->entry))
            continue;

        bool finalize = false;
        unsigned int size = m_currShader->GetDebugInfoData().m_VISAModules.size();
        m_pDebugEmitter = m_currShader->GetDebugInfoData().m_pDebugEmitter;
        std::vector<std::pair<unsigned int, std::pair<llvm::Function*, IGC::VISAModule*>>> sortedVISAModules;

        // Sort modules in order of their placement in binary
        IGC::VISADebugInfo VisaDbgInfo(m_currShader->ProgramOutput()->m_debugDataGenISA);
        const auto &decodedDbg = VisaDbgInfo.getRawDecodedData();
        auto getGenOff = [&decodedDbg](const std::vector<std::pair<unsigned int, unsigned int>>& data,
                                       unsigned int VISAIndex)
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

        for (auto& m : m_currShader->GetDebugInfoData().m_VISAModules)
        {
            setType(m.second);
            auto lastVISAId = getLastGenOff(m.second);
            // getLastGenOffset returns zero iff debug info for given function
            // was not found, skip the function in such case. This can happen,
            // when the function was optimized away but the definition is still
            // present inside the module.
            if (lastVISAId == 0)
              continue;
            sortedVISAModules.push_back(std::make_pair(lastVISAId, std::make_pair(m.first, m.second)));
        }

        std::sort(sortedVISAModules.begin(), sortedVISAModules.end(),
            [](std::pair<unsigned int, std::pair<llvm::Function*, IGC::VISAModule*>>& p1,
                std::pair<unsigned int, std::pair<llvm::Function*, IGC::VISAModule*>>& p2)
        {
            return p1.first < p2.first;
        });

        m_pDebugEmitter->SetDISPCache(&DISPCache);
        for (auto& m : sortedVISAModules)
        {
            m_pDebugEmitter->registerVISA(m.second.second);
        }

        std::vector<llvm::Function*> functions;
        std::for_each(sortedVISAModules.begin(), sortedVISAModules.end(),
            [&functions](auto& item) { functions.push_back(item.second.first); });

        for (auto& m : sortedVISAModules)
        {
            m_pDebugEmitter->setCurrentVISA(m.second.second);

            if (--size == 0)
                finalize = true;

            EmitDebugInfo(finalize, VisaDbgInfo);
        }

        // set VISA dbg info to nullptr to indicate 1-step debug is enabled
        if (currShader->ProgramOutput()->m_debugDataGenISA)
        {
            IGC::aligned_free(currShader->ProgramOutput()->m_debugDataGenISA);
        }
        currShader->ProgramOutput()->m_debugDataGenISASize = 0;
        currShader->ProgramOutput()->m_debugDataGenISA = nullptr;

        m_currShader->GetContext()->metrics.CollectDataFromDebugInfo(
            m_currShader->entry,
            &m_currShader->GetDebugInfoData(), &VisaDbgInfo);

        if (finalize)
        {
            IDebugEmitter::Release(m_pDebugEmitter);
        }
    }

    return false;
}

static void debugDump(const CShader* Shader, llvm::StringRef Ext,
                      ArrayRef<char> Blob)
{
    if (Blob.empty())
        return;

    auto ExtStr = Ext.str();
    IGC::Debug::DumpName DumpNameObj = IGC::Debug::GetDumpNameObj(Shader, ExtStr.c_str());
    std::string DumpName = DumpNameObj.str();
    if (IGC_IS_FLAG_ENABLED(DebugDumpNamePrefix))
    {
        auto hash = ShaderHash();
        DumpNameObj = DumpNameObj.Hash(hash);
        DumpName = DumpNameObj.AbsolutePath(IGC_GET_REGKEYSTRING(DebugDumpNamePrefix));
    }

    if (DumpNameObj.allow())
    {
        FILE* const DumpFile = fopen(DumpName.c_str(), "wb+");
        if (nullptr == DumpFile)
            return;

        fwrite(Blob.data(), Blob.size(), 1, DumpFile);
        fclose(DumpFile);
    }
}

void DebugInfoPass::EmitDebugInfo(bool finalize,
                                  const IGC::VISADebugInfo& VisaDbgInfo)
{
    IGC_ASSERT(m_pDebugEmitter);

    std::vector<char> buffer = m_pDebugEmitter->Finalize(finalize, VisaDbgInfo);

    if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable) || IGC_IS_FLAG_ENABLED(ElfDumpEnable))
        debugDump(m_currShader, "elf", { buffer.data(), buffer.size() });

    const std::string& DbgErrors = m_pDebugEmitter->getErrors();
    if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
        debugDump(m_currShader, "dbgerr", { DbgErrors.data(), DbgErrors.size() });

    void* dbgInfo = IGC::aligned_malloc(buffer.size(), sizeof(void*));
    if (dbgInfo)
        memcpy_s(dbgInfo, buffer.size(), buffer.data(), buffer.size());

    SProgramOutput* pOutput = m_currShader->ProgramOutput();
    pOutput->m_debugData = dbgInfo;
    pOutput->m_debugDataSize = dbgInfo ? buffer.size() : 0;
}


// Detect instructions with an address class pattern. Then remove all opcodes of this pattern from
// this instruction's last operand (metadata of DIExpression).
// Pattern: !DIExpression(DW_OP_constu, 4, DW_OP_swap, DW_OP_xderef)
void DebugInfoData::extractAddressClass(llvm::Function& F)
{
    DIBuilder di(*F.getParent());

    for (auto& bb : F)
    {
        for (auto& pInst : bb)
        {
            if (auto* DI = dyn_cast<DbgVariableIntrinsic>(&pInst))
            {
                const DIExpression* DIExpr = DI->getExpression();
                llvm::SmallVector<uint64_t, 5> newElements;
                for (auto I = DIExpr->expr_op_begin(), E = DIExpr->expr_op_end(); I != E; ++I)
                {
                    if (I->getOp() == dwarf::DW_OP_constu)
                    {
                        auto patternI = I;
                        if (++patternI != E && patternI->getOp() == dwarf::DW_OP_swap &&
                            ++patternI != E && patternI->getOp() == dwarf::DW_OP_xderef)
                        {
                            I = patternI;
                            continue;
                        }
                    }
                    I->appendToVector(newElements);
                }

                if (newElements.size() < DIExpr->getNumElements())
                {
                    DIExpression* newDIExpr = di.createExpression(newElements);
                    DI->setExpression(newDIExpr);
                }
            }
        }
    }
}

void DebugInfoData::markVariableAsOutput(CShader *pShader, CVariable *pVariable)
{
    IGC_ASSERT(pShader && pVariable);

    // Mark variable with "Output", to extend it's living time will be extended to the end of the function.
    pShader->GetEncoder().GetVISAKernel()->AddAttributeToVar(pVariable->visaGenVariable[0], "Output", 0, nullptr);
    if (pShader->m_State.m_dispatchSize == SIMDMode::SIMD32 && pVariable->visaGenVariable[1])
    {
        pShader->GetEncoder().GetVISAKernel()->AddAttributeToVar(pVariable->visaGenVariable[1], "Output", 0, nullptr);
    }
}

void DebugInfoData::saveAndMarkPrivateMemoryVars(llvm::Function& F, CShader* pShader)
{
    IGC_ASSERT_MESSAGE(pShader, "CShader is missing.");

    IDebugEmitter* pDebugEmitter = pShader->GetDebugInfoData().m_pDebugEmitter;

    ScalarVisaModule *mVISAModule = nullptr;
    if (pDebugEmitter)
    {
        mVISAModule = (ScalarVisaModule *)pDebugEmitter->getCurrentVISA();
        IGC_ASSERT_MESSAGE(mVISAModule, "Missing VISA module.");
    }

    // Add FP to VISA module.
    // Debug emitter will decide whether it needs to use it.
    if (mVISAModule && pShader->hasFP())
    {
        mVISAModule->setFramePtr(pShader->GetFP());
    }

    for (auto& bb : F)
    {
        for (auto& pInst : bb)
        {
            // If function has "perThreadOffset" variable - add it to VISA module
            // and for mark it as output so that emitter will extend it's living time
            // to the end of the function.
            // Mark privateBase as output, because it will need it too.
            if (MDNode* perThreadOffsetMD = pInst.getMetadata("perThreadOffset"))
            {
                CVariable *perThreadOffset = pShader->GetSymbol(&pInst);
                CVariable *privateBase = pShader->GetPrivateBase();

                IGC_ASSERT_MESSAGE(perThreadOffset, "Missing perThreadOffset.");
                IGC_ASSERT_MESSAGE(privateBase, "Missing privateBase.");

                markVariableAsOutput(pShader, perThreadOffset);
                markVariableAsOutput(pShader, privateBase);

                if (mVISAModule)
                {
                    mVISAModule->setPerThreadOffset(perThreadOffset);
                    mVISAModule->setPrivateBase(privateBase);
                }
            }
        }
    }
}

bool IGC::DebugInfoData::hasDebugInfo(CShader* pShader)
{
    return pShader->GetContext()->m_instrTypes.hasDebugInfo;
}

void DebugInfoData::transferMappings(const llvm::Function& F)
{
    auto cacheMapping = [this](llvm::DenseMap<llvm::Value*, CVariable*>& Map)
    {
        for (auto& mapping : Map)
        {
            auto CVar = mapping.second;
            if (CVar->visaGenVariable[0])
            {
                unsigned int lower16Channels = (unsigned int)m_pShader->GetEncoder().GetVISAKernel()->getDeclarationID(CVar->visaGenVariable[0]);
                unsigned int higher16Channels = 0;
                if (numLanes(m_pShader->m_State.m_dispatchSize) == 32 && !CVar->IsUniform() && CVar->visaGenVariable[1])
                    higher16Channels = m_pShader->GetEncoder().GetVISAKernel()->getDeclarationID(CVar->visaGenVariable[1]);
                CVarToVISADclId[CVar] = std::make_pair(lower16Channels, higher16Channels);
            }
        }
    };

    // Store llvm::Value->CVariable mappings from CShader.
    // CShader clears these mappings before compiling a new function.
    // Debug info is computed after all functions are compiled.
    // This instance stores mappings per llvm::Function so debug
    // info generation can emit variable locations correctly.
    auto& SymbolMapping = m_pShader->GetSymbolMapping();
    m_FunctionSymbols[&F] = SymbolMapping;

    // VISA builder gets destroyed at end of EmitVISAPass.
    // Debug info pass is invoked later. We need a way to
    // preserve mapping of CVariable -> VISA reg# so that
    // we can emit location information in debug info. This
    // code below iterates over all CVariable instances and
    // retrieves and stored their VISA reg# in a map. This
    // map is later queried by debug info pass.
    cacheMapping(SymbolMapping);

    auto& GlobalSymbolMapping = m_pShader->GetGlobalMapping();
    cacheMapping(GlobalSymbolMapping);

    if (m_pShader->hasFP())
    {
        auto FP = m_pShader->GetFP();
        auto VISADclIdx = m_pShader->GetEncoder().GetVISAKernel()->getDeclarationID(FP->visaGenVariable[0]);
        CVarToVISADclId[FP] = std::make_pair(VISADclIdx, 0);
    }
}

CVariable* DebugInfoData::getMapping(const llvm::Function& F, const llvm::Value* V)
{
    auto& Data = m_FunctionSymbols[&F];
    auto Iter = Data.find(V);
    if (Iter != Data.end())
        return (*Iter).second;
    return nullptr;
}

// Register pass to igc-opt
#define PASS_FLAG "igc-catch-all-linenum"
#define PASS_DESCRIPTION "CatchAllLineNumber pass"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(CatchAllLineNumber, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(CatchAllLineNumber, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

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
    // Insert placeholder intrinsic instruction in each kernel/stack call function.
    if (!F.getSubprogram() || F.isDeclaration() ||
      IGC_IS_FLAG_ENABLED(NoCatchAllDebugLine))
        return false;

    if (F.getCallingConv() != llvm::CallingConv::SPIR_KERNEL &&
        !F.hasFnAttribute("visaStackCall"))
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
