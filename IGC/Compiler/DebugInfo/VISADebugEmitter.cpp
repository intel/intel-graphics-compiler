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

//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "llvm/Config/llvm-config.h"
#include "Compiler/DebugInfo/VISADebugEmitter.hpp"
#include "Compiler/DebugInfo/DwarfDebug.hpp"
#include "Compiler/DebugInfo/StreamEmitter.hpp"
#include "Compiler/DebugInfo/VISAModule.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Module.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/MCDwarf.h"
#if LLVM_VERSION_MAJOR == 4
#include "llvm/Support/ELF.h"
#elif LLVM_VERSION_MAJOR >= 7
#include "llvm/BinaryFormat/ELF.h"
#endif
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/DebugInfo/DebugInfoUtils.hpp"
#include "common/secure_mem.h"
#include "Compiler/CISACodeGen/DriverInfo.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

IDebugEmitter* IDebugEmitter::Create()
{
    return new DebugEmitter();
}

void IDebugEmitter::Release(IDebugEmitter* pDebugEmitter)
{
    delete pDebugEmitter;
}

DebugEmitter::~DebugEmitter()
{
    Reset();
}

void DebugEmitter::Reset()
{
    m_str.clear();
    if (toFree.size() > 0)
    {
        for (auto& item : toFree)
            delete item;
        m_pVISAModule = nullptr;
        toFree.clear();
    }
    if (m_pStreamEmitter)
    {
        delete m_pStreamEmitter;
        m_pStreamEmitter = nullptr;
    }
    if (m_pDwarfDebug)
    {
        delete m_pDwarfDebug;
        m_pDwarfDebug = nullptr;
    }
    m_initialized = false;
}

// OpenCL keyword constant is used as qualifier to variables whose values remain the
// same throughout the program. clang inlines constants in to LLVM IR and no metadata
// is emitted to LLVM IR for such constants. This function iterates over all globals
// and constants to emit metadata per function.
void IGC::insertOCLMissingDebugConstMetadata(CodeGenContext* ctx)
{
    Module* M = ctx->getModule();
    bool fullDebugInfo, lineNumbersOnly;
    DebugMetadataInfo::hasAnyDebugInfo(ctx, fullDebugInfo, lineNumbersOnly);

    if (!fullDebugInfo)
    {
        return;
    }

    for (auto& func : *M)
    {
        if (func.isDeclaration())
            continue;

        for (auto global_it = M->global_begin();
            global_it != M->global_end();
            global_it++)
        {
            auto g = &*global_it;//(global_it.operator llvm::GlobalVariable *());

            if (g->isConstant())
            {
                auto init = g->getInitializer();

                bool isConstForThisFunc = false;
                llvm::SmallVector<llvm::DIGlobalVariableExpression*, 1> GVs;
                g->getDebugInfo(GVs);
                for (unsigned int j = 0; j < GVs.size(); j++)
                {
                    auto GVExp = llvm::dyn_cast_or_null<llvm::DIGlobalVariableExpression>(GVs[j]);
                    if (GVExp)
                    {
                        auto GV = GVExp->getVariable();
                        auto gblNodeScope = GV->getScope();
                        if (isa<DISubprogram>(gblNodeScope))
                        {
                            auto subprogramName = cast<DISubprogram>(gblNodeScope)->getName().data();

                            if (subprogramName == func.getName())
                            {
                                isConstForThisFunc = true;
                                break;
                            }
                        }
                    }
                }

                if (!GlobalValue::isLocalLinkage(g->getLinkage()) || isConstForThisFunc)
                {
                    DebugInfoUtils::UpdateGlobalVarDebugInfo(g, init, &func.getEntryBlock().getInstList().front(), false);
                }
            }
        }
    }
}

void DebugEmitter::Initialize(CShader* pShader, bool debugEnabled)
{
    IGC_ASSERT_MESSAGE(false == m_initialized, "DebugEmitter is already initialized!");
    m_initialized = true;

    m_debugEnabled = debugEnabled;
    // VISA module will be initialized even when debugger is disabled.
    // Its overhead is minimum and it will be used in debug mode to
    // assertion test on calling DebugEmitter in the right order.
    m_pVISAModule = VISAModule::BuildNew(pShader);
    toFree.push_back(m_pVISAModule);

    if (!m_debugEnabled)
    {
        return;
    }

    std::string dataLayout = m_pVISAModule->GetDataLayout();
    m_pStreamEmitter = new StreamEmitter(m_outStream, dataLayout, m_pVISAModule->GetTargetTriple(), m_pVISAModule->isDirectElfInput);
    m_pDwarfDebug = new DwarfDebug(m_pStreamEmitter, m_pVISAModule);
}

void DebugEmitter::setFunction(llvm::Function* F, bool isCloned)
{
    m_pVISAModule->SetEntryFunction(F, isCloned);
}

void DebugEmitter::Finalize(void*& pBuffer, unsigned int& size, bool finalize)
{
    pBuffer = nullptr;
    size = 0;
    if (!m_debugEnabled)
    {
        return;
    }

    if (m_pVISAModule->isDirectElfInput)
    {
        auto decodedDbg = new DbgDecoder(m_pVISAModule->m_pShader->ProgramOutput()->m_debugDataGenISA);
        m_pDwarfDebug->setDecodedDbg(decodedDbg);
    }

    if (!doneOnce)
    {
        m_pDwarfDebug->beginModule();
        doneOnce = true;
    }

    const Function* pFunc = m_pVISAModule->GetEntryFunction();
    // Collect debug information for given function.
    m_pStreamEmitter->SwitchSection(m_pStreamEmitter->GetTextSection());
    m_pDwarfDebug->beginFunction(pFunc, m_pVISAModule);

    if (m_pVISAModule->isDirectElfInput)
    {
        m_pVISAModule->buildDirectElfMaps();
        auto co = m_pVISAModule->getCompileUnit();

        // Emit src line mapping directly instead of
        // relying on dbgmerge. elf generated will have
        // text section and debug_line sections populated.
        auto& VISAIndexToInst = m_pVISAModule->VISAIndexToInst;
        auto& VISAIndexToSize = m_pVISAModule->VISAIndexToSize;
        std::vector<std::pair<unsigned int, unsigned int>> GenISAToVISAIndex;
        unsigned int subEnd = m_pVISAModule->GetCurrentVISAId();
        unsigned int prevLastGenOff = lastGenOff;
        m_pDwarfDebug->lowPc = lastGenOff;

        if (IGC_IS_FLAG_ENABLED(EnableSIMDLaneDebugging))
        {
            // SIMD width
            m_pDwarfDebug->simdWidth = m_pVISAModule->GetSIMDSize();
        }

        if (co->subs.size() == 0)
        {
            GenISAToVISAIndex = m_pVISAModule->GenISAToVISAIndex;
            if (GenISAToVISAIndex.size() > 0)
                lastGenOff = GenISAToVISAIndex.back().first;
            m_pDwarfDebug->lowPc = co->relocOffset;
        }
        else
        {
            for (auto item : m_pVISAModule->GenISAToVISAIndex)
            {
                if ((item.first >= lastGenOff) || ((item.first | lastGenOff) == 0))
                {
                    if (item.second <= subEnd || item.second == 0xffffffff)
                    {
                        GenISAToVISAIndex.push_back(item);
                        auto size = m_pVISAModule->GenISAInstSizeBytes[item.first];
                        lastGenOff = item.first + size;
                        continue;
                    }

                    if (item.second > subEnd)
                        break;
                }
            }
        }

        auto genxISA = static_cast<const unsigned char*>(m_pVISAModule->m_pShader->ProgramOutput()->m_programBin);
        DebugLoc prevSrcLoc = DebugLoc();
        unsigned int pc = prevLastGenOff;
        for (auto item : GenISAToVISAIndex)
        {
            for (unsigned int i = pc; i != item.first; i++)
            {
                m_pStreamEmitter->EmitInt8(genxISA[i]);
            }

            pc = item.first;

            auto instIt = VISAIndexToInst.end();
            auto sizeIt = VISAIndexToSize.find(item.second);
            if (sizeIt != VISAIndexToSize.end())
            {
                // Lookup all VISA instructions that may
                // map to an llvm::Instruction. This is useful
                // when an llvm::Instruction leads to multiple
                // VISA instructions, and VISA optimizer
                // optimizes some of those away. Src line
                // mapping for all VISA instructions is the
                // same. So lookup any one that still exists.
                auto startIdx = sizeIt->second.first;
                auto numVISAInsts = sizeIt->second.second;
                for (unsigned int visaId = startIdx;
                    visaId != (startIdx + numVISAInsts); visaId++)
                {
                    instIt = VISAIndexToInst.find(visaId);
                    // Loop till at least one VISA instruction
                    // is found.
                    if (instIt != VISAIndexToInst.end())
                        break;
                }
            }

            bool emptyLoc = true;
            if (instIt != VISAIndexToInst.end())
            {
                auto loc = instIt->second->getDebugLoc();
                if (loc)
                {
                    if (loc != prevSrcLoc)
                    {
                        auto scope = loc->getScope();
                        auto src = m_pDwarfDebug->getOrCreateSourceID(scope->getFilename(), scope->getDirectory(), m_pStreamEmitter->GetDwarfCompileUnitID());

                        unsigned int Flags = 0;
                        if (!m_pDwarfDebug->isStmtExists(loc.getLine(), loc.getInlinedAt(), true))
                        {
                            Flags |= DWARF2_FLAG_IS_STMT;
                        }
                        m_pStreamEmitter->EmitDwarfLocDirective(src, loc.getLine(), loc.getCol(), Flags, 0, 0, scope->getFilename());

                        prevSrcLoc = loc;
                    }

                    emptyLoc = false;
                }
            }

            if (emptyLoc && prevSrcLoc)
            {
              if (IGC_IS_FLAG_ENABLED(FillMissingDebugLocations))
              {
                if (auto scope = pFunc->getSubprogram())
                {
                  auto src = m_pDwarfDebug->getOrCreateSourceID(scope->getFilename(), scope->getDirectory(), m_pStreamEmitter->GetDwarfCompileUnitID());

                  // Emit func top as line# for unattributed lines
                  m_pStreamEmitter->EmitDwarfLocDirective(src, scope->getLine(), 0, 0, 0, 0, scope->getFilename());
                }
              }
              else
              {
                auto scope = prevSrcLoc->getScope();
                auto src = m_pDwarfDebug->getOrCreateSourceID(scope->getFilename(), scope->getDirectory(), m_pStreamEmitter->GetDwarfCompileUnitID());

                // Emit 0 as line# for unattributed lines
                m_pStreamEmitter->EmitDwarfLocDirective(src, 0, 0, 0, 0, 0, scope->getFilename());
              }

                prevSrcLoc = DebugLoc();
            }
        }

        if (finalize)
        {
            for (unsigned int i = pc; i != m_pVISAModule->getUnpaddedProgramSize(); i++)
            {
                m_pStreamEmitter->EmitInt8(genxISA[i]);
                lastGenOff++;
            }
        }
        else if (pc != lastGenOff)
        {
            // for subroutines
            for (unsigned int i = pc; i != lastGenOff; i++)
            {
                m_pStreamEmitter->EmitInt8(genxISA[i]);
            }
        }

        m_pDwarfDebug->highPc = lastGenOff;
    }
    else
    {
        unsigned int prevOffset = 0;
        for (const Instruction *pInst : *m_pVISAModule)
        {

            unsigned int currOffset = m_pVISAModule->GetVisaOffset(pInst);

            int currSize = (int)m_pVISAModule->GetVisaSize(pInst);
            bool recordSrcLine = (currSize == 0) ? false : true;

            // Emit bytes up to the current instruction
            for (; prevOffset < currOffset; prevOffset++)
            {
                m_pStreamEmitter->EmitInt8(0);
            }
            m_pDwarfDebug->beginInstruction(pInst, recordSrcLine);
            // Emit bytes of the current instruction
            for (int i = 0; i < currSize; i++)
            {
                m_pStreamEmitter->EmitInt8(0);
            }
            m_pDwarfDebug->endInstruction(pInst);
            prevOffset += currSize;
        }
    }

    // Emit post-function debug information.
    m_pDwarfDebug->endFunction(pFunc);

    if (finalize)
    {
        // Make sure we wrote out everything we need.
        //m_pMCStreamer->Flush();

        // Finalize debug information.
        m_pDwarfDebug->endModule();

        m_pStreamEmitter->Finalize();

        size = m_outStream.str().size();

        // Add program header table to satisfy latest gdb
        unsigned int is64Bit = (GetVISAModule()->GetModule()->getDataLayout().getPointerSize() == 8);
        unsigned int phtSize = sizeof(llvm::ELF::Elf32_Phdr);
        if (is64Bit)
            phtSize = sizeof(llvm::ELF::Elf64_Phdr);

        pBuffer = (char*)malloc((size + phtSize) * sizeof(char));
        memcpy_s(pBuffer, size * sizeof(char), m_outStream.str().data(), size);

        writeProgramHeaderTable(is64Bit, pBuffer, size);
        if (m_pVISAModule->isDirectElfInput)
            setElfType(is64Bit, pBuffer);

        size += phtSize;

        // Reset all members and prepare for next beginModule() call.
        Reset();
    }
}

void DebugEmitter::setElfType(bool is64Bit, void* pBuffer)
{
    // Set 1-step elf's e_type to ET_EXEC
    if (!pBuffer)
        return;

    if (is64Bit)
    {
        void* etypeOff = ((char*)pBuffer) + (offsetof(llvm::ELF::Elf64_Ehdr, e_type));
        *((llvm::ELF::Elf64_Half*)etypeOff) = llvm::ELF::ET_EXEC;
    }
    else
    {
        void* etypeOff = ((char*)pBuffer) + (offsetof(llvm::ELF::Elf32_Ehdr, e_type));
        *((llvm::ELF::Elf32_Half*)etypeOff) = llvm::ELF::ET_EXEC;
    }
}

void DebugEmitter::writeProgramHeaderTable(bool is64Bit, void* pBuffer, unsigned int size)
{
    // Write program header table at end of elf
    if (is64Bit)
    {
        llvm::ELF::Elf64_Phdr hdr;
        hdr.p_type = llvm::ELF::PT_LOAD;
        hdr.p_flags = 0;
        hdr.p_offset = 0;
        hdr.p_vaddr = 0;
        hdr.p_paddr = 0;
        hdr.p_filesz = size;
        hdr.p_memsz = size;
        hdr.p_align = 4;
        void* phOffAddr = ((char*)pBuffer) + (offsetof(llvm::ELF::Elf64_Ehdr, e_phoff));
        *(llvm::ELF::Elf64_Off*)(phOffAddr) = size;
        ((char*)pBuffer)[offsetof(llvm::ELF::Elf64_Ehdr, e_phentsize)] = sizeof(llvm::ELF::Elf64_Phdr);
        ((char*)pBuffer)[offsetof(llvm::ELF::Elf64_Ehdr, e_phnum)] = 1;
        memcpy_s((char*)pBuffer + size, sizeof(llvm::ELF::Elf64_Phdr), &hdr, sizeof(hdr));
    }
    else
    {
        llvm::ELF::Elf32_Phdr hdr;
        hdr.p_type = llvm::ELF::PT_LOAD;
        hdr.p_offset = 0;
        hdr.p_vaddr = 0;
        hdr.p_paddr = 0;
        hdr.p_filesz = size;
        hdr.p_memsz = size;
        hdr.p_flags = 0;
        hdr.p_align = 4;
        void* phOffAddr = ((char*)pBuffer) + (offsetof(llvm::ELF::Elf32_Ehdr, e_phoff));
        *(llvm::ELF::Elf32_Off*)(phOffAddr) = size;
        ((char*)pBuffer)[offsetof(llvm::ELF::Elf32_Ehdr, e_phentsize)] = sizeof(llvm::ELF::Elf32_Phdr);
        ((char*)pBuffer)[offsetof(llvm::ELF::Elf32_Ehdr, e_phnum)] = 1;
        memcpy_s((char*)pBuffer + size, sizeof(llvm::ELF::Elf32_Phdr), &hdr, sizeof(hdr));
    }
}

void DebugEmitter::BeginInstruction(Instruction* pInst)
{
    BeginEncodingMark();
    if (!m_debugEnabled)
    {
        return;
    }
    m_pVISAModule->BeginInstruction(pInst);
}

void DebugEmitter::EndInstruction(Instruction* pInst)
{
    EndEncodingMark();
    if (!m_debugEnabled)
    {
        return;
    }
    m_pVISAModule->EndInstruction(pInst);
}

void DebugEmitter::BeginEncodingMark()
{
    m_pVISAModule->BeginEncodingMark();
}

void DebugEmitter::EndEncodingMark()
{
    m_pVISAModule->EndEncodingMark();
}

void DebugEmitter::Free(void* pBuffer)
{
    if (pBuffer)
    {
        free(pBuffer);
    }
}

void DebugEmitter::ResetVISAModule()
{
    m_pVISAModule = VISAModule::BuildNew(m_pVISAModule->m_pShader);
    toFree.push_back(m_pVISAModule);
    m_pVISAModule->Reset();
    m_pVISAModule->setDISPToFuncMap(m_pDwarfDebug->getDISPToFunction());
    m_pVISAModule->SetDwarfDebug(m_pDwarfDebug);
}

/*static*/ bool DebugMetadataInfo::hasDashgOption(CodeGenContext* ctx)
{
    return ctx->getModuleMetaData()->compOpt.DashGSpecified;
}

/*static*/ bool DebugMetadataInfo::hasAnyDebugInfo(CodeGenContext* ctx, bool& fullDebugInfo, bool& lineNumbersOnly)
{
    Module* module = ctx->getModule();
    bool hasFullDebugInfo = false;
    fullDebugInfo = false;
    lineNumbersOnly = false;

    if (DebugInfoUtils::HasDebugInfo(*ctx->getModule()))
    {
        bool hasDbgIntrinsic = false;
        bool hasDbgLoc = false;

        // Return true if LLVM IR has dbg.declare/dbg.value intrinsic calls.
        // And also !dbgloc data.
        auto& funcList = module->getFunctionList();

        for (auto funcIt = funcList.begin();
            funcIt != funcList.end() && !hasFullDebugInfo;
            funcIt++)
        {
            auto& func = (*funcIt);

            for (auto bbIt = func.begin();
                bbIt != func.end() && !hasFullDebugInfo;
                bbIt++)
            {
                auto& bb = (*bbIt);

                for (auto instIt = bb.begin();
                    instIt != bb.end() && !hasFullDebugInfo;
                    instIt++)
                {
                    auto& inst = (*instIt);

                    if (dyn_cast_or_null<DbgInfoIntrinsic>(&inst))
                    {
                        hasDbgIntrinsic = true;
                    }

                    auto& loc = inst.getDebugLoc();

                    if (loc)
                    {
                        hasDbgLoc = true;
                    }

                    hasFullDebugInfo = hasDbgIntrinsic & hasDbgLoc;

                    fullDebugInfo |= hasFullDebugInfo;
                    lineNumbersOnly |= hasDbgLoc;
                }
            }
        }
    }

    return (fullDebugInfo | lineNumbersOnly);
}

std::string DebugMetadataInfo::getUniqueFuncName(Function& F)
{
    // Find number of clones of function F. For n clones,
    // generate name like $dup$n.
    auto M = F.getParent();
    unsigned int numClones = 0;
    std::string funcName(F.getName().data());

    for (auto funcIt = M->begin(); funcIt != M->end(); funcIt++)
    {
        std::string funcItName((*funcIt).getName().data());

        auto found = funcItName.find("$dup");
        if (found == funcName.length() &&
            funcName.compare(0, funcName.length(), funcName) == 0)
        {
            numClones++;
        }
    }

    return F.getName().str() + "$dup" + "$" + std::to_string(numClones);
}

/* static */ bool VISAModule::isLineTableOnly(CShader* s)
{
    bool lineTableOnly = false, fullDebugInfo = false;
    DebugMetadataInfo::hasAnyDebugInfo(s->GetContext(), fullDebugInfo, lineTableOnly);

    return lineTableOnly && !fullDebugInfo;
}

void DebugEmitter::AddVISAModFunc(IGC::VISAModule* v, llvm::Function* f)
{
    m_pDwarfDebug->AddVISAModToFunc(v, f);
}