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

#if LLVM_VERSION_MAJOR == 4 && LLVM_VERSION_MINOR == 0

#include "Compiler/DebugInfo/VISADebugEmitter.hpp"
#include "Compiler/DebugInfo/DwarfDebug.hpp"
#include "Compiler/DebugInfo/StreamEmitter.hpp"
#include "Compiler/DebugInfo/VISAModule.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Module.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/Verifier.h"
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/DebugInfo/DebugInfoUtils.hpp"
#include "common/secure_mem.h"
#include "Compiler/CISACodeGen/DriverInfo.hpp"

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

    for (auto func_it = M->begin();
        func_it != M->end();
        func_it++)
    {
        auto& func = (*func_it);

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
                if (GlobalValue::isInternalLinkage(g->getLinkage()))
                {
					llvm::SmallVector<llvm::DIGlobalVariableExpression *, 1> GVs;
					g->getDebugInfo(GVs);
					for (unsigned int j = 0; j < GVs.size(); j++)
					{
						llvm::DIGlobalVariable* GV = llvm::cast<llvm::DIGlobalVariable>(GVs[j]);
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

                if (GlobalValue::isExternalLinkage(g->getLinkage()) || isConstForThisFunc)
                {
                    DebugInfoUtils::UpdateGlobalVarDebugInfo(g, init, &func.getEntryBlock().getInstList().front(), false);
                }
            }
        }
    }
}

void DebugEmitter::Initialize(CShader* pShader, bool debugEnabled)
{
    assert(!m_initialized && "DebugEmitter is already initialized!");
    m_initialized = true;

    m_debugEnabled = debugEnabled;
    // VISA module will be initialized even when debugger is disabled.
    // Its overhead is minimum and it will be used in debug mode to
    // assert on calling DebugEmitter in the right order.
    m_pVISAModule = VISAModule::BuildNew(pShader);
    toFree.push_back(m_pVISAModule);

    if (!m_debugEnabled)
    {
        return;
    }

    std::string dataLayout = m_pVISAModule->GetDataLayout();
    m_pStreamEmitter = new StreamEmitter(m_outStream, dataLayout, m_pVISAModule->GetTargetTriple());
    m_pDwarfDebug = new DwarfDebug(m_pStreamEmitter, m_pVISAModule);
}

void DebugEmitter::setFunction(llvm::Function* F, bool isCloned) 
{ 
    m_pVISAModule->SetEntryFunction(F, isCloned); 
}

void DebugEmitter::Finalize(void *&pBuffer, unsigned int &size, bool finalize)
{
    pBuffer = nullptr;
    size = 0;
    if (!m_debugEnabled)
    {
        return;
    }

    if (!doneOnce)
    {
        m_pDwarfDebug->beginModule();
        doneOnce = true;
    }

    const Function *pFunc = m_pVISAModule->GetEntryFunction();
    // Collect debug information for given function.
    m_pStreamEmitter->SwitchSection(m_pStreamEmitter->GetTextSection());
    m_pDwarfDebug->beginFunction(pFunc, m_pVISAModule);
    unsigned int prevOffset = 0;

    if (m_pVISAModule->isDirectElfInput)
    {
        m_pVISAModule->buildDirectElfMaps();

        // Emit src line mapping directly instead of
        // relying on dbgmerge. elf generated will have
        // text section and debug_line sections populated.
        std::map<unsigned int, const llvm::Instruction*>& VISAIndexToInst = m_pVISAModule->VISAIndexToInst;
        std::vector<std::pair<unsigned int, unsigned int>> GenISAToVISAIndex;
        unsigned int subEnd = m_pVISAModule->GetCurrentVISAId();
        unsigned int prevLastGenOff = lastGenOff;

        for (auto item : m_pVISAModule->GenISAToVISAIndex)
        {
            if ((item.first > lastGenOff) || ((item.first | lastGenOff) == 0))
            {
                if (item.second <= subEnd ||
                    item.second == 0xffffffff)
                {
                    GenISAToVISAIndex.push_back(item);
                    lastGenOff = item.first;
                    continue;
                }

                if (item.second > subEnd)
                    break;
            }
        }

        void* genxISA = m_pVISAModule->m_pShader->ProgramOutput()->m_programBin;
        unsigned int prevSrcLine = 0;
        unsigned int pc = prevLastGenOff;
        for (auto item : GenISAToVISAIndex)
        {
            for (unsigned int i = pc; i != item.first; i++)
            {
                m_pStreamEmitter->EmitInt8(((unsigned char*)genxISA)[i]);
            }

            pc = item.first;

            auto instIt = VISAIndexToInst.find(item.second);
            if (instIt != VISAIndexToInst.end())
            {
                auto loc = (*instIt).second->getDebugLoc();
                if (loc)
                {
                    auto scope = loc->getScope();

                    auto src = m_pDwarfDebug->getOrCreateSourceID(scope->getFilename(), scope->getDirectory(), m_pStreamEmitter->GetDwarfCompileUnitID());
                    if (loc.getLine() != prevSrcLine)
                    {
                        m_pStreamEmitter->EmitDwarfLocDirective(src, loc.getLine(), loc.getCol(), 1, 0, 0, scope->getFilename());
                    }

                    prevSrcLine = loc.getLine();
                }
            }
        }

        if (finalize)
        {
            for (unsigned int i = pc; i != m_pVISAModule->getUnpaddedProgramSize(); i++)
            {
                m_pStreamEmitter->EmitInt8(((unsigned char*)genxISA)[i]);
            }
        }
    }
    else
    {
        for (VISAModule::const_iterator II = m_pVISAModule->begin(), IE = m_pVISAModule->end(); II != IE; ++II)
        {
            const Instruction *pInst = *II;
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
        pBuffer = (char*)malloc(size * sizeof(char));
        memcpy_s(pBuffer, size * sizeof(char), m_outStream.str().data(), size);

        // Reset all members and prepare for next beginModule() call.
        Reset();
    }
}

void DebugEmitter::BeginInstruction(Instruction *pInst)
{
    BeginEncodingMark();
    if (!m_debugEnabled)
    {
        return;
    }
    m_pVISAModule->BeginInstruction(pInst);
}

void DebugEmitter::EndInstruction(Instruction *pInst)
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

void DebugEmitter::Free(void *pBuffer)
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

    if (hasDashgOption(ctx))
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

#endif
