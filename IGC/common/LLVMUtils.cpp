/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/PassTimer.hpp"
#include "Compiler/CISACodeGen/TimeStatsCounter.h"
#include "common/Stats.hpp"
#include "common/debug/Dump.hpp"
#include "common/shaderOverride.hpp"
#include "common/IntrinsicAnnotator.hpp"
#include "common/LLVMUtils.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvmWrapper/ADT/StringRef.h>
#include "common/LLVMWarningsPop.hpp"

using namespace IGC;
using namespace IGC::Debug;
using namespace llvm;

bool getPassToggles(std::bitset<1024>& toggles)
{
    const char* passToggles = IGC_GET_REGKEYSTRING(DisablePassToggles);
    if (passToggles != nullptr && strlen(passToggles) > 0)
    {
        std::string szBin;
        std::string szHexLL;
        unsigned int len = 0;
        unsigned long long x = 0;
        std::string szHex = passToggles;
        for (size_t i = 0; i < szHex.size(); i += 16)
        {
            szHexLL = szHex.substr(i, 16);
            len = szHexLL.size() * 4;
            x = std::stoull(szHexLL, nullptr, 16);
            szBin += std::bitset<64>(x).to_string().substr(64 - len, len);
        }

        toggles = std::bitset<1024>(szBin);
        return true;
    }

    return false;
}

llvm::Pass* createFlushPass(llvm::Pass* pass, Dump& dump)
{
    // Choose an appropriate pass to preserve the original order of pass execution in the pass manager
    switch (pass->getPassKind())
    {
    case PT_Function:
        return new FunctionFlushDumpPass(dump);
    case PT_Module:
        return new ModuleFlushDumpPass(dump);
    case PT_Region:
    case PT_Loop:
    case PT_CallGraphSCC:
        // flushing for analysis passes is not required
        break;
    case PT_PassManager:
    default:
        // internal pass managers are not considered in the context
        break;
    }
    return nullptr;
}

void IGCPassManager::add(Pass *P)
{
    //check only once
    static bool checkedToggles = false;
    static bool hasToggles = false;
    static std::bitset<1024> toggles;
    if (!checkedToggles)
    {
        checkedToggles = true;
        hasToggles = getPassToggles(toggles);
    }
    if (hasToggles && m_pContext->m_numPasses < 1024 && toggles[m_pContext->m_numPasses])
    {
        errs() << "Skipping pass: '" << P->getPassName() << "\n";
        m_pContext->m_numPasses++;
        return;
    }

    if (IGC_IS_FLAG_ENABLED(ShaderDisableOptPassesAfter)
            && m_pContext->m_numPasses > IGC_GET_FLAG_VALUE(ShaderDisableOptPassesAfter)
            && m_name == "OPT") {
        errs() << "Skipping optimization pass: '" << P->getPassName()
               << "' (threshold: " << IGC_GET_FLAG_VALUE(ShaderDisableOptPassesAfter) << ").\n";
        return;
    }

    if (isPrintBefore(P))
    {
        addPrintPass(P, true);
    }

    if (IGC_REGKEY_OR_FLAG_ENABLED(DumpTimeStatsPerPass, TIME_STATS_PER_PASS))
    {
        PassManager::add(createTimeStatsIGCPass(m_pContext, m_name + '_' + std::string(P->getPassName()), STATS_COUNTER_START));
    }

    PassManager::add(P);

    if (IGC_REGKEY_OR_FLAG_ENABLED(DumpTimeStatsPerPass, TIME_STATS_PER_PASS))
    {
        PassManager::add(createTimeStatsIGCPass(m_pContext, m_name + '_' + std::string(P->getPassName()), STATS_COUNTER_END));
    }

    if (isPrintAfter(P))
    {
        addPrintPass(P, false);
    }
}

// List: a comma/semicolon-separated list of pass names.
//    N: a pass name
// return true if N is in List.
bool IGCPassManager::isInList(const StringRef& N, const StringRef& List) const
{
    StringRef Separators(",;");
    size_t startPos = 0;
    while (startPos != StringRef::npos)
    {
        size_t endPos = List.find_first_of(Separators, startPos);
        size_t len = (endPos != StringRef::npos ? endPos - startPos : endPos);
        StringRef Name = List.substr(startPos, len);
        if (IGCLLVM::equals_insensitive(Name,N))
        {
            return true;
        }
        startPos = (endPos != StringRef::npos ? endPos + 1 : StringRef::npos);
    }
    return false;
}

bool IGCPassManager::isPrintBefore(Pass* P)
{
    if (IGC_IS_FLAG_ENABLED(PrintBefore))
    {
        // PrintBefore=N0,N1,N2  : comma-separate list of pass names
        //                         or pass command args registered in passInfo.
        StringRef  passNameList(IGC_GET_REGKEYSTRING(PrintBefore));
        StringRef PN = P->getPassName();
        if (IGCLLVM::equals_insensitive(passNameList, "all") || isInList(PN, passNameList))
            return true;

        // further check passInfo
        if (const PassInfo* PI = Pass::lookupPassInfo(P->getPassID()))
        {
            return isInList(PI->getPassArgument(), passNameList);
        }
    }
    return false;
}

bool IGCPassManager::isPrintAfter(Pass* P)
{
    if (IGC_IS_FLAG_ENABLED(ShaderDumpEnableAll))
    {
        return true;
    }
    if (IGC_IS_FLAG_ENABLED(PrintAfter))
    {
        // PrintAfter=N0,N1,N2  : comma-separate list of pass names or
        //                         or pass command args registered in passInfo.
        StringRef  passNameList(IGC_GET_REGKEYSTRING(PrintAfter));
        StringRef PN = P->getPassName();
        if (IGCLLVM::equals_insensitive(passNameList, "all") || isInList(PN, passNameList))
            return true;

        // further check passInfo
        if (const PassInfo* PI = Pass::lookupPassInfo(P->getPassID()))
        {
            return isInList(PI->getPassArgument(), passNameList);
        }
    }
    return false;
}

void IGCPassManager::addPrintPass(Pass* P, bool isBefore)
{
    std::string passName =
        m_name + (isBefore ? "_before_" : "_after_") + std::string(P->getPassName());
    auto name =
        IGC::Debug::DumpName(IGC::Debug::GetShaderOutputName())
        .Type(m_pContext->type)
        .Hash(m_pContext->hash)
        .Pass(passName, m_pContext->m_numPasses++)
        .StagedInfo(m_pContext)
        .Extension("ll");

    if (!name.allow())
        return;

    // The dump object needs to be on the Heap because it owns the stream, and the stream
    // is taken by reference into the printer pass. If the Dump object had been on the
    // stack, then that reference would go bad as soon as we exit this scope, and then
    // the printer pass would access an invalid pointer later on when we call PassManager::run()
    m_irDumps.emplace_front(name, IGC::Debug::DumpType::PASS_IR_TEXT);
    PassManager::add(P->createPrinterPass(m_irDumps.front().stream(), ""));

    llvm::Pass* flushPass = createFlushPass(P, m_irDumps.front());
    if (nullptr != flushPass)
    {
        PassManager::add(flushPass);
    }
}

void DumpLLVMIR(IGC::CodeGenContext* pContext, const char* dumpName)
{
    SetCurrentDebugHash(pContext->hash.asmHash);

    if (IGC_IS_FLAG_ENABLED(DumpLLVMIR))
    {
        pContext->getMetaDataUtils()->save(*pContext->getLLVMContext());
        serialize(*(pContext->getModuleMetaData()), pContext->getModule());
        using namespace IGC::Debug;
        auto name =
            DumpName(IGC::Debug::GetShaderOutputName())
            .Hash(pContext->hash)
            .Type(pContext->type)
            .Pass(dumpName)
            .Retry(pContext->m_retryManager.GetRetryId())
            .Extension("ll");
        auto new_annotator = IntrinsicAnnotator();
        auto annotator = (pContext->annotater != nullptr) ? pContext->annotater : &new_annotator;
        DumpLLVMIRText(
            pContext->getModule(),
            name,
            annotator);
    }
    if (IGC_IS_FLAG_ENABLED(ShaderOverride))
    {
        auto name =
            DumpName(IGC::Debug::GetShaderOutputName())
            .Hash(pContext->hash)
            .Type(pContext->type)
            .Pass(dumpName)
            .Extension("ll");
        SMDiagnostic Err;
        std::string fileName = name.overridePath();
        FILE* fp = fopen(fileName.c_str(), "r");
        if (fp != nullptr)
        {
            fclose(fp);
            errs() << "Override shader: " << fileName << "\n";
            Module* mod = parseIRFile(fileName, Err, *pContext->getLLVMContext()).release();
            if (mod)
            {
                pContext->deleteModule();
                pContext->setModule(mod);
                deserialize(*(pContext->getModuleMetaData()), mod);
                appendToShaderOverrideLogFile(fileName, "OVERRIDEN: ");
            }
            else
            {
                std::stringstream ss;
                ss << "Parse IR failed.\n";
                ss << Err.getLineNo() << ": "
                    << Err.getLineContents().str() << "\n"
                    << Err.getMessage().str() << "\n";

                std::string str = ss.str();
                errs() << str;
                appendToShaderOverrideLogFile(fileName, str.c_str());
            }
        }
    }
}
