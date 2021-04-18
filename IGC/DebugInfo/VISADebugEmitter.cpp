/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "llvm/Config/llvm-config.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Module.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/MCDwarf.h"
#include "llvm/MC/MCSymbol.h"
#if LLVM_VERSION_MAJOR == 4
#include "llvm/Support/ELF.h"
#elif LLVM_VERSION_MAJOR >= 7
#include "llvm/BinaryFormat/ELF.h"
#endif
#include "common/LLVMWarningsPop.hpp"

#include "DebugInfoUtils.hpp"
#include "VISADebugEmitter.hpp"
#include "DwarfDebug.hpp"
#include "StreamEmitter.hpp"
#include "VISAModule.hpp"

#include "Probe/Assertion.h"
#include "secure_mem.h"

#include "CLElfLib/CLElfTypes.h"

#define DEBUG_TYPE "GENX_DEBUG_INFO"

using namespace llvm;
using namespace IGC;
using namespace CLElfLib;   // ElfReader related typedefs

IDebugEmitter* IDebugEmitter::Create()
{
    return new DebugEmitter();
}

void IDebugEmitter::Release(IDebugEmitter* pDebugEmitter)
{
    delete pDebugEmitter;
}

DebugEmitter::DebugEmitter() : IDebugEmitter(), m_outStream(m_str)
{
}
DebugEmitter::~DebugEmitter()
{
    Reset();
}

void DebugEmitter::Reset()
{
    m_str.clear();
    m_pVISAModule = nullptr;

    m_pStreamEmitter.reset();
    m_pDwarfDebug.reset();
    toFree.clear();

    m_initialized = false;
}


void DebugEmitter::Initialize(std::unique_ptr<VISAModule> VM, const DebugEmitterOpts& Opts)
{
    IGC_ASSERT_MESSAGE(false == m_initialized, "DebugEmitter is already initialized!");
    m_initialized = true;

    m_pVISAModule = VM.get();
    m_debugEnabled = Opts.DebugEnabled;
    // VISA module will be initialized even when debugger is disabled.
    // Its overhead is minimum and it will be used in debug mode to
    // assertion test on calling DebugEmitter in the right order.
    toFree.push_back(std::move(VM));

    if (!m_debugEnabled)
    {
        return;
    }

    const auto &dataLayout = m_pVISAModule->GetDataLayout();
    m_pStreamEmitter = std::make_unique<StreamEmitter>(
                        m_outStream, dataLayout, m_pVISAModule->GetTargetTriple(), Opts);
    m_pDwarfDebug = std::make_unique<DwarfDebug>(m_pStreamEmitter.get(), m_pVISAModule);

    registerVISA(m_pVISAModule);
}

void DebugEmitter::processCurrentFunction(bool finalize, DbgDecoder* decodedDbg) {

    auto EmitIpLabel = [&](unsigned int ip)
    {
        // Emit label before %ip
        if (m_pStreamEmitter->GetEmitterSettings().EnableRelocation)
        {
            auto instLabel = m_pDwarfDebug->GetLabelBeforeIp(ip);
            m_pStreamEmitter->EmitLabel(instLabel);
        }
    };

    if (!m_pVISAModule->isDirectElfInput) {
        // Legacy path
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
        return;
    }

    m_pVISAModule->buildDirectElfMaps(*decodedDbg);
    auto co = m_pVISAModule->getCompileUnit(*decodedDbg);

    // Emit src line mapping directly instead of
    // relying on dbgmerge. elf generated will have
    // text section and debug_line sections populated.
    const auto& VISAIndexToInst = m_pVISAModule->VISAIndexToInst;
    const auto& VISAIndexToSize = m_pVISAModule->VISAIndexToSize;
    decltype(m_pVISAModule->GenISAToVISAIndex) GenISAToVISAIndex;
    unsigned int subEnd = m_pVISAModule->GetCurrentVISAId();
    unsigned int prevLastGenOff = lastGenOff;
    m_pDwarfDebug->lowPc = lastGenOff;

    if (m_pStreamEmitter->GetEmitterSettings().EnableSIMDLaneDebugging)
    {
        // SIMD width
        m_pDwarfDebug->simdWidth = m_pVISAModule->GetSIMDSize();
    }

    if (co->subs.size() == 0)
    {
        GenISAToVISAIndex = m_pVISAModule->GenISAToVISAIndex;
        if (GenISAToVISAIndex.size() > 0)
            lastGenOff = GenISAToVISAIndex.back().GenOffset;
        m_pDwarfDebug->lowPc = co->relocOffset;
    }
    else
    {
        for (auto item : m_pVISAModule->GenISAToVISAIndex)
        {
            if ((item.GenOffset >= lastGenOff) || ((item.GenOffset | lastGenOff) == 0))
            {
                if (item.VisaOffset <= subEnd || item.VisaOffset == 0xffffffff)
                {
                    GenISAToVISAIndex.push_back(item);
                    auto size = m_pVISAModule->GenISAInstSizeBytes[item.GenOffset];
                    lastGenOff = item.GenOffset + size;
                    continue;
                }

                if (item.VisaOffset > subEnd)
                    break;
            }
        }
    }

    auto genxISA = m_pVISAModule->getGenBinary();
    DebugLoc prevSrcLoc = DebugLoc();
    unsigned int pc = prevLastGenOff;

    if (!GenISAToVISAIndex.empty()) {
        IGC_ASSERT(GenISAToVISAIndex.rbegin()->GenOffset <= genxISA.size());
        IGC_ASSERT(pc < genxISA.size());
        IGC_ASSERT(GenISAToVISAIndex.begin()->GenOffset >= pc);
    }
    for (auto item : GenISAToVISAIndex)
    {
        for (unsigned int i = pc; i != item.GenOffset; i++)
        {
            EmitIpLabel(i);
            m_pStreamEmitter->EmitInt8(genxISA[i]);
        }

        pc = item.GenOffset;

        auto instIt = VISAIndexToInst.end();
        auto sizeIt = VISAIndexToSize.find(item.VisaOffset);
        if (sizeIt != VISAIndexToSize.end())
        {
            // Lookup all VISA instructions that may
            // map to an llvm::Instruction. This is useful
            // when an llvm::Instruction leads to multiple
            // VISA instructions, and VISA optimizer
            // optimizes some of those away. Src line
            // mapping for all VISA instructions is the
            // same. So lookup any one that still exists.
            auto startIdx = sizeIt->second.VisaOffset;
            auto numVISAInsts = sizeIt->second.VisaInstrNum;
            // Loop till at least one VISA instruction
            // is found.
            for (unsigned int visaId = startIdx;
                visaId != (startIdx + numVISAInsts); visaId++)
            {
                instIt = VISAIndexToInst.find(visaId);
                if (instIt != VISAIndexToInst.end())
                    break;
            }
        }

        if (instIt == VISAIndexToInst.end())
            continue;
        if (!m_pVISAModule->IsExecutableInst(*instIt->second))
            continue;

        const auto& loc = instIt->second->getDebugLoc();
        if (!loc || loc == prevSrcLoc)
            continue;

        const auto* scope = loc->getScope();
        auto src = m_pDwarfDebug->getOrCreateSourceID(scope->getFilename(), scope->getDirectory(), m_pStreamEmitter->GetDwarfCompileUnitID());

        unsigned int Flags = 0;
        if (!m_pDwarfDebug->isStmtExists(loc.getLine(), loc.getInlinedAt(), true))
        {
            Flags |= DWARF2_FLAG_IS_STMT;
        }

        if (!m_pDwarfDebug->prologueEndExists(loc.get()->getScope()->getSubprogram(),
                                              loc.getInlinedAt(), true))
        {
            if (m_pStreamEmitter->GetEmitterSettings().EmitPrologueEnd)
            {
                Flags |= DWARF2_FLAG_PROLOGUE_END;
            }
        }
        m_pStreamEmitter->EmitDwarfLocDirective(src, loc.getLine(), loc.getCol(), Flags, 0, 0, scope->getFilename());

        prevSrcLoc = loc;
    }

    if (finalize)
    {
        size_t unpaddedSize = m_pVISAModule->getUnpaddedProgramSize();

        IGC_ASSERT(unpaddedSize <= genxISA.size());
        IGC_ASSERT((pc < genxISA.size() && pc < unpaddedSize) || pc == unpaddedSize);

        for (unsigned int i = pc; i != unpaddedSize; i++)
        {
            m_pStreamEmitter->EmitInt8(genxISA[i]);
            lastGenOff++;
        }
    }
    else if (pc != lastGenOff)
    {
        IGC_ASSERT(lastGenOff <= genxISA.size());
        IGC_ASSERT((pc < genxISA.size() && pc < lastGenOff) || pc == lastGenOff);
        // for subroutines
        for (unsigned int i = pc; i != lastGenOff; i++)
        {
            EmitIpLabel(i);
            m_pStreamEmitter->EmitInt8(genxISA[i]);
        }
    }

    m_pDwarfDebug->highPc = lastGenOff;
}

std::vector<char> DebugEmitter::Finalize(bool finalize, DbgDecoder* decodedDbg,
    const std::vector<llvm::DISubprogram*>& DISubprogramNodes)
{
    if (!m_debugEnabled)
    {
        return {};
    }

    IGC_ASSERT_MESSAGE(m_pVISAModule, "active visa object must be selected before finalization");
    if (m_pVISAModule->isDirectElfInput)
    {
        m_pDwarfDebug->setDecodedDbg(decodedDbg);
    }

    if (!doneOnce)
    {
        m_pDwarfDebug->setDISPNodes(&DISubprogramNodes);
        m_pDwarfDebug->beginModule();
        doneOnce = true;
    }

    const Function* pFunc = m_pVISAModule->GetEntryFunction();
    // Collect debug information for given function.
    m_pStreamEmitter->SwitchSection(m_pStreamEmitter->GetTextSection());
    m_pDwarfDebug->beginFunction(pFunc, m_pVISAModule);
    processCurrentFunction(finalize, decodedDbg);
    // Emit post-function debug information.
    m_pDwarfDebug->endFunction(pFunc);

    if (!finalize)
        return {};

    // Make sure we wrote out everything we need.
    //m_pMCStreamer->Flush();

    // Finalize debug information.
    m_pDwarfDebug->endModule();

    m_pStreamEmitter->Finalize();

    LLVM_DEBUG(dbgs() << "Finalized Visa Module:\n");
    LLVM_DEBUG(m_pVISAModule->dump());

    // Add program header table to satisfy latest gdb
    bool is64Bit = m_pVISAModule->getPointerSize() == 8;
    unsigned int phtSize = sizeof(llvm::ELF::Elf32_Phdr);
    if (is64Bit)
        phtSize = sizeof(llvm::ELF::Elf64_Phdr);

    unsigned int kernelNameSizeWithDot = 0;
    if (m_pStreamEmitter->GetEmitterSettings().EnableElf2ZEBinary)
    {
        kernelNameSizeWithDot = sizeof('.') + pFunc->getName().size();
    }
    size_t elfWithProgramHeaderSize = m_str.size() + phtSize + kernelNameSizeWithDot;
    std::vector<char> Result(elfWithProgramHeaderSize);

    if (!m_pStreamEmitter->GetEmitterSettings().EnableElf2ZEBinary)
    {
        // Text section remains with its standard name .text
        std::copy(m_str.begin(), m_str.end(), Result.begin());
    }
    else
    {
        // Text section's name to be extended by a kernel name.

        // How each elf section gets its name? Find the answer in the following function.

        size_t endOfDotTextNameOffset = 0;
        std::string entryFunctionNameWithDot = "." + pFunc->getName().str();
        unsigned int kernelNameSizeWithDot = entryFunctionNameWithDot.size();
        prepareElfForZeBinary(is64Bit, m_str.begin(), m_str.size(), kernelNameSizeWithDot, &endOfDotTextNameOffset);

        // First copy ELF binary from the beginning to the .text name (included) located in the .str.tab
        std::copy(m_str.begin(), m_str.begin() + endOfDotTextNameOffset, Result.begin());
        // Next concatenate .text with a kernel name (a dot joining both names also added).
        std::copy(entryFunctionNameWithDot.data(), entryFunctionNameWithDot.data() + kernelNameSizeWithDot,
            Result.begin() + endOfDotTextNameOffset);
        // Finally copy remaining part of ELF binary.
        std::copy(m_str.begin() + endOfDotTextNameOffset + 1, m_str.end(),
            Result.begin() + endOfDotTextNameOffset + 1 + kernelNameSizeWithDot);
    }

    writeProgramHeaderTable(is64Bit, Result.data(), m_str.size() + kernelNameSizeWithDot);

    if (m_pVISAModule->isDirectElfInput)
        setElfType(is64Bit, Result.data());

    // Reset all members and prepare for next beginModule() call.
    Reset();

    return std::move(Result);
}

void DebugEmitter::prepareElfForZeBinary(bool is64Bit, char* pElfBuffer, size_t elfBufferSize, size_t kernelNameWithDotSize,
    size_t* pEndOfDotTextNameInStrtab)
{
    // ELF binary header contains 'SectionHeadersOffset' (e_shoff in ELF spec.), which is an offset
    // to section headers placed one by one. A location (index) of the header with names (including section name)
    // is stored in the ELF binary header at 'SectionNameTableIndex' (e_shstrndx). A section header under this index
    // contains an offset to location of the String Table data, which may look as showed in the line below:
    //  .debug_abbrev .text.stackcall .debug_ranges .debug_str .debug_info
    // ^NULL         ^NULL           ^NULL         ^NULL      ^NULL       ^NULL
    //
    // Each section header contain 'Name' (sh_name) fields which is a byte offset to this data showed above.
    // The String Table is a simple chunk of memory, where the names are placed one by one and separated by NULL (\0).
    // String Table contains NULL (\0) also at the beginning (i.e. 'Name' equal 0 means no name).

    if (is64Bit)
    {
        SElf64Header* pElf64Header = (SElf64Header*)pElfBuffer;

        // First simply validate ELF binary
        IGC_ASSERT_MESSAGE(pElf64Header &&
            (pElf64Header->Identity[ID_IDX_MAGIC0] == ELF_MAG0) &&
            (pElf64Header->Identity[ID_IDX_MAGIC1] == ELF_MAG1) &&
            (pElf64Header->Identity[ID_IDX_MAGIC2] == ELF_MAG2) &&
            (pElf64Header->Identity[ID_IDX_MAGIC3] == ELF_MAG3) &&
            (pElf64Header->Identity[ID_IDX_CLASS] == EH_CLASS_64), "ELF file header incorrect");

        // Using the Section Name Table Index, calculate the offset to the String Table (.strtab) header.
        size_t entrySize = pElf64Header->SectionHeaderEntrySize;
        size_t nameSectionHeaderOffset = (size_t)pElf64Header->SectionHeadersOffset + (pElf64Header->SectionNameTableIndex * entrySize);
        IGC_ASSERT_MESSAGE(pElf64Header->SectionNameTableIndex < pElf64Header->NumSectionHeaderEntries, "ELF header incorrect");
        IGC_ASSERT_MESSAGE(nameSectionHeaderOffset < elfBufferSize, "ELF header incorrect");

        // Using the offset found above get a header of the String Table section
        SElf64SectionHeader* pNamesSectionHeader = (SElf64SectionHeader*)((char*)pElf64Header + nameSectionHeaderOffset);

        SElf64SectionHeader* pSectionHeader = NULL;
        size_t indexedSectionHeaderOffset = 0;
        Elf64_Word textSectionHeaderName = 0;
        char* pSectionName = NULL;
        size_t sectionNameOffset = 0;
        size_t textSectionNameOffset = 0;

        // Scan section headers to find the Text section using simple section name comparison.
        for (unsigned int elfSectionIdx = 1; elfSectionIdx < pElf64Header->NumSectionHeaderEntries; elfSectionIdx++)
        {
            // Calculate a byte offset to the current section's header
            indexedSectionHeaderOffset = (size_t)pElf64Header->SectionHeadersOffset + (elfSectionIdx * entrySize);

            // Get a header of the current section
            pSectionHeader = (SElf64SectionHeader*)((char*)pElf64Header + indexedSectionHeaderOffset);

            // Using the byte offset from the current section's header, find the current section's name
            // in the String Table.
            sectionNameOffset = (size_t)pNamesSectionHeader->DataOffset + pSectionHeader->Name;
            pSectionName = (char*)pElf64Header + sectionNameOffset;

            // Check if the Text section is found.
            if (pSectionName && (strcmp(pSectionName, ".text") == 0))
            {
                textSectionHeaderName = pSectionHeader->Name;   // Remember for the next loop over sections.
                textSectionNameOffset = sectionNameOffset;      // Remember for the next loop over sections.

                pSectionHeader->DataSize += kernelNameWithDotSize;
                // Return an offset (from the beginning of ELF binary) to the first character after '.text'
                *pEndOfDotTextNameInStrtab = sectionNameOffset + sizeof(".text") - 1;
                break; // Text section found, its location saved.
            }
        }

        // Update headers of ELF sections due to a longer Text section name
        // (i.e. in strtab .text will be replaced with .text.kernelName).
        // - change location of each section name located after the Text section name in .strtab
        // - change data offset of each section located after the .strtab section
        for (unsigned int elfSectionIdx = 1; elfSectionIdx < pElf64Header->NumSectionHeaderEntries; elfSectionIdx++)
        {
            indexedSectionHeaderOffset = (size_t)pElf64Header->SectionHeadersOffset + (elfSectionIdx * entrySize);

            pSectionHeader = (SElf64SectionHeader*)((char*)pElf64Header + indexedSectionHeaderOffset);
            if (pSectionHeader->Name > textSectionHeaderName)
            {
                pSectionHeader->Name += kernelNameWithDotSize;
            }
            if (pSectionHeader->DataOffset > textSectionNameOffset)
            {
                pSectionHeader->DataOffset += kernelNameWithDotSize;
                // This data offset update may be not enough if such a section contains elf global offsets
                // (i.e. relative to the beginning of the elf binary).
                // However, as long as .strtab is the last section in our ELF binary then there is no side-effects.
                // If location of this section changes in the future, then a copy (or move) of this section content
                // will be required. This future need must be verified if the assertion below hits (then
                // assertion below must be changed based on results of such verification).
                IGC_ASSERT(false);
            }
        }

        if (pSectionHeader)
        {
            // ELF binary header also must be updated to reflect offsets changes.
            if (pElf64Header->SectionHeadersOffset > pSectionHeader->DataOffset)
            {
                pElf64Header->SectionHeadersOffset += kernelNameWithDotSize;
            }
            if (pElf64Header->ProgramHeadersOffset > pSectionHeader->DataOffset)
            {
                pElf64Header->ProgramHeadersOffset += kernelNameWithDotSize;
            }
        }
    }
    else
    {
        IGC_ASSERT_MESSAGE(is64Bit, "64-bit ELF file only supported");
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
        if (m_pStreamEmitter->GetEmitterSettings().EnableRelocation)
        {
            *((llvm::ELF::Elf64_Half*)etypeOff) = llvm::ELF::ET_REL;
        }
        else
        {
            *((llvm::ELF::Elf64_Half*)etypeOff) = llvm::ELF::ET_EXEC;
        }
    }
    else
    {
        void* etypeOff = ((char*)pBuffer) + (offsetof(llvm::ELF::Elf32_Ehdr, e_type));
        if (m_pStreamEmitter->GetEmitterSettings().EnableRelocation)
        {
            *((llvm::ELF::Elf32_Half*)etypeOff) = llvm::ELF::ET_REL;
        }
        else
        {
            *((llvm::ELF::Elf32_Half*)etypeOff) = llvm::ELF::ET_EXEC;
        }
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
// TODO: do we really need it?
void DebugEmitter::registerVISA(IGC::VISAModule* VM)
{
    m_pDwarfDebug->registerVISA(VM);
}
void DebugEmitter::setCurrentVISA(IGC::VISAModule* VM) {
    // TODO: add assert to check that this module is registered/owned
    m_pVISAModule = VM;
}
void DebugEmitter::resetModule(std::unique_ptr<IGC::VISAModule> VM) {
    m_pVISAModule = VM.get();
    toFree.push_back(std::move(VM));
}
