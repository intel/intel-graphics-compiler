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

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <list>

#include "visa_igc_common_header.h"
#include "Common_ISA.h"
#include "Common_ISA_util.h"
#include "Common_ISA_framework.h"
#ifdef DLL_MODE
#include "RT_Jitter_Interface.h"
#else
#include "JitterDataStruct.h"
#endif
#include "VISAKernel.h"
#include "BinaryCISAEmission.h"
#include "Timer.h"
#include "BinaryEncoding.h"
#include "IsaDisassembly.h"

#include "Gen4_IR.hpp"
#include "FlowGraph.h"
#include "DebugInfo.h"

using namespace std;
using namespace vISA;
extern "C" int64_t getTimerTicks(unsigned int idx);

#define IS_GEN_PATH  (mBuildOption == VISA_BUILDER_GEN)
#define IS_BOTH_PATH  (mBuildOption == VISA_BUILDER_BOTH)
#define IS_GEN_BOTH_PATH  (mBuildOption == VISA_BUILDER_GEN || mBuildOption ==  VISA_BUILDER_BOTH)
#define IS_VISA_BOTH_PATH  (mBuildOption == VISA_BUILDER_VISA || mBuildOption ==  VISA_BUILDER_BOTH)

CISA_IR_Builder::~CISA_IR_Builder()
{
    m_cisaBinary->~CisaBinary();

    std::list<VISAKernelImpl *>::iterator iter_start = m_kernels.begin();
    std::list<VISAKernelImpl *>::iterator iter_end = m_kernels.end();

    while (iter_start != iter_end)
    {
        VISAKernelImpl *kernel = *iter_start;
        iter_start++;
        // don't call delete since vISAKernelImpl is allocated in memory pool
        kernel->~VISAKernelImpl();
    }

    if (needsToFreeWATable)
    {
        delete m_pWaTable;
    }
}

void CISA_IR_Builder::InitVisaWaTable(TARGET_PLATFORM platform, Stepping step)
{
    assert(!m_pWaTable && "WA_Table must be nullptr for this function to be called");

    m_pWaTable = new WA_TABLE;
    memset(m_pWaTable, 0, sizeof(WA_TABLE));
    needsToFreeWATable = true;

    if ((platform == GENX_SKL && (step == Step_A || step == Step_B)) ||
        (platform == GENX_BXT && step == Step_A))
    {
        VISA_WA_ENABLE(m_pWaTable, WaHeaderRequiredOnSimd16Sample16bit);
    }
    else
    {
        VISA_WA_DISABLE(m_pWaTable, WaHeaderRequiredOnSimd16Sample16bit);
    }

    if ((platform == GENX_SKL) && (step == Step_A))
    {
        VISA_WA_ENABLE(m_pWaTable, WaSendsSrc1SizeLimitWhenEOT);
    }
    else
    {
        VISA_WA_DISABLE(m_pWaTable, WaSendsSrc1SizeLimitWhenEOT);
    }

    if ((platform == GENX_SKL && (step == Step_A || step == Step_B)) ||
        (platform == GENX_BXT && step == Step_A))
    {
        VISA_WA_ENABLE(m_pWaTable, WaDisallow64BitImmMov);
    }
    else
    {
        VISA_WA_DISABLE(m_pWaTable, WaDisallow64BitImmMov);
    }

    if (platform == GENX_BDW || platform == GENX_CHV ||
        platform == GENX_BXT || platform == GENX_SKL)
    {
        VISA_WA_ENABLE(m_pWaTable, WaThreadSwitchAfterCall);
    }
    else
    {
        VISA_WA_DISABLE(m_pWaTable, WaThreadSwitchAfterCall);
    }

    if ((platform == GENX_SKL && step < Step_E) ||
        (platform == GENX_BXT && step <= Step_B))
    {
        VISA_WA_ENABLE(m_pWaTable, WaSrc1ImmHfNotAllowed);
    }
    else
    {
        VISA_WA_DISABLE(m_pWaTable, WaSrc1ImmHfNotAllowed);
    }

    if (platform == GENX_SKL && step == Step_A)
    {
        VISA_WA_ENABLE(m_pWaTable, WaDstSubRegNumNotAllowedWithLowPrecPacked);
    }
    else
    {
        VISA_WA_DISABLE(m_pWaTable, WaDstSubRegNumNotAllowedWithLowPrecPacked);
    }

    if ((platform == GENX_SKL && step < Step_C))
    {
        VISA_WA_ENABLE(m_pWaTable, WaDisableMixedModeLog);
        VISA_WA_ENABLE(m_pWaTable, WaDisableMixedModeFdiv);
        VISA_WA_ENABLE(m_pWaTable, WaDisableMixedModePow);
    }
    else
    {
        VISA_WA_DISABLE(m_pWaTable, WaDisableMixedModeLog);
        VISA_WA_DISABLE(m_pWaTable, WaDisableMixedModeFdiv);
        VISA_WA_DISABLE(m_pWaTable, WaDisableMixedModePow);
    }


    if ((platform == GENX_SKL && step < Step_C) ||
        platform == GENX_CHV)
    {
        VISA_WA_ENABLE(m_pWaTable, WaFloatMixedModeSelNotAllowedWithPackedDestination);
    }
    else
    {
        VISA_WA_DISABLE(m_pWaTable, WaFloatMixedModeSelNotAllowedWithPackedDestination);
    }

    // always disable in offline mode
    VISA_WA_DISABLE(m_pWaTable, WADisableWriteCommitForPageFault);

    if ((platform == GENX_SKL && step < Step_D) ||
        (platform == GENX_BXT && step == Step_A))
    {
        VISA_WA_ENABLE(m_pWaTable, WaDisableSIMD16On3SrcInstr);
    }

    if (platform == GENX_SKL && (step == Step_C || step == Step_D))
    {
        VISA_WA_ENABLE(m_pWaTable, WaSendSEnableIndirectMsgDesc);
    }
    else
    {
        VISA_WA_DISABLE(m_pWaTable, WaSendSEnableIndirectMsgDesc);
    }

    if (platform == GENX_SKL || platform == GENX_BXT)
    {
        VISA_WA_ENABLE(m_pWaTable, WaClearArfDependenciesBeforeEot);
    }

    if (platform == GENX_SKL && step == Step_A)
    {
        VISA_WA_ENABLE(m_pWaTable, WaDisableSendsSrc0DstOverlap);
    }

    if (platform >= GENX_SKL)
    {
        VISA_WA_ENABLE(m_pWaTable, WaMixModeSelInstDstNotPacked);
    }

    if (platform == GENX_SKL || platform == GENX_BXT)
    {
        VISA_WA_ENABLE(m_pWaTable, WaResetN0BeforeGatewayMessage);
    }

    // WA for future platforms
    if (platform == GENX_ICLLP)
    {
        VISA_WA_ENABLE(m_pWaTable, Wa_1406306137);
    }
    if (platform == GENX_ICLLP && (step == Step_A || step == Step_B))
    {
        VISA_WA_ENABLE(m_pWaTable, Wa_2201674230);
    }
    switch (platform)
    {
        case GENX_ICLLP:
            VISA_WA_ENABLE(m_pWaTable, Wa_1406950495);
            break;
        case GENX_TGLLP:
            VISA_WA_ENABLE(m_pWaTable, Wa_1406950495);
            break;
        default:
            break;
    }
}

int CISA_IR_Builder::CreateBuilder(
    CISA_IR_Builder *&builder,
    vISABuilderMode mode,
    VISA_BUILDER_OPTION buildOption,
    TARGET_PLATFORM platform,
    int numArgs,
    const char* flags[],
    PWA_TABLE pWaTable)
{

    initTimer();

    if (builder != NULL)
    {
        assert(0);
        return VISA_FAILURE;
    }

    startTimer(TIMER_TOTAL);
    startTimer(TIMER_BUILDER);  // builder time ends with we call compile (i.e., it covers the IR construction time)
    //this must be called before any other API.
    SetVisaPlatform(platform);

    // initialize stepping to none in case it's not passed in
    InitStepping();

    builder = new CISA_IR_Builder(buildOption, COMMON_ISA_MAJOR_VER, COMMON_ISA_MINOR_VER, pWaTable);

    if (!builder->m_options.parseOptions(numArgs, flags))
    {
        delete builder;
        assert(0);
        return VISA_FAILURE;
    }

    auto targetMode = (mode == vISA_3D || mode == vISA_ASM_WRITER || mode == vISA_ASM_READER) ? VISA_3D : VISA_CM;
    builder->m_options.setTarget(targetMode);
    builder->m_options.setOptionInternally(vISA_isParseMode, (mode == vISA_PARSER || mode == vISA_ASM_READER));
    builder->m_options.setOptionInternally(vISA_IsaAssembly, (mode == vISA_ASM_WRITER));

    if (mode == vISA_PARSER)
    {
        builder->m_options.setOptionInternally(vISA_GeneratevISABInary, true);
        /*
            In parser mode we always want to dump out vISA
            I don't feel like modifying FE, and dealing with FE/BE missmatch issues.
        */
        builder->m_options.setOptionInternally(vISA_DumpvISA, true);
        /*
            Dumping out .asm and .dat files for BOTH mod. Since they are used in
            simulation mode. Again can be pased by FE, but don't want to deal
            with FE/BE miss match issues.
        */
        if (buildOption != VISA_BUILDER_VISA)
        {
            builder->m_options.setOptionInternally(vISA_outputToFile, true);
            builder->m_options.setOptionInternally(vISA_GenerateBinary, true);
        }
    }

    // emit location info always for these cases
    if (mode == vISABuilderMode::vISA_MEDIA && builder->m_options.getOption(vISA_outputToFile))
    {
        builder->m_options.setOptionInternally(vISA_EmitLocation, true);
    }

    // driver WaTable is not available in offline vISA executable mode
    // We instead create and initialize some of the known ones here
    if (!pWaTable)
    {
        builder->InitVisaWaTable(platform, GetStepping());
    }

    return VISA_SUCCESS;
}

int CISA_IR_Builder::DestroyBuilder(CISA_IR_Builder *builder)
{

    if(builder == NULL)
    {
        assert(0);
        return VISA_FAILURE;
    }

    delete builder;

    return VISA_SUCCESS;
}

bool CISA_IR_Builder::CISA_IR_initialization(
    const char *kernel_name, int line_no)
{
    m_kernel->InitializeKernel(kernel_name);
    return true;
}

VISAKernel* CISA_IR_Builder::GetVISAKernel(const std::string& kernelName)
{
    if (kernelName.empty())
        return static_cast<VISAKernel*>(m_kernel);
    return static_cast<VISAKernel*>(m_nameToKernel.at(kernelName));
}

int CISA_IR_Builder::ClearAsmTextStreams()
{
    if (m_options.getOption(vISA_IsaAssembly))
    {
        m_ssIsaAsmHeader.str(std::string());
        m_ssIsaAsmHeader.clear();
        m_ssIsaAsm.str(std::string());
        m_ssIsaAsm.clear();

        return VISA_SUCCESS;
    }

    assert(0 && "Should clear streams only in asm text writer mode!");
    return VISA_FAILURE;
}

int CISA_IR_Builder::AddKernel(VISAKernel *& kernel, const char* kernelName)
{

    if( kernel != NULL )
    {
        assert( 0 );
        return VISA_FAILURE;
    }

    VISAKernelImpl * kerneltemp = new (m_mem) VISAKernelImpl(this, mBuildOption, &m_options);
    kernel = static_cast<VISAKernel *>(kerneltemp);
    m_kernel = kerneltemp;
    //m_kernel->setName(kernelName);
    m_kernel->setIsKernel(true);
    m_kernels.push_back(kerneltemp);
    m_kernel->setVersion((unsigned char)this->m_header.major_version, (unsigned char)this->m_header.minor_version);
    m_kernel->InitializeKernel(kernelName);
    m_kernel->SetGTPinInit(getGtpinInit());
    this->m_kernel_count++;
    this->m_nameToKernel[kernelName] = m_kernel;

    if (m_options.getOption(vISA_IsaAssembly))
    {
        ClearAsmTextStreams();
    }

    return VISA_SUCCESS;
}

int CISA_IR_Builder::AddFunction(VISAFunction *& function, const char* functionName)
{
    if( function != NULL )
    {
        assert( 0 );
        return VISA_FAILURE;
    }

    this->AddKernel((VISAKernel *&)function, functionName);

    ((VISAKernelImpl*)function)->m_functionId = this->m_function_count;

    this->m_kernel_count--;
    this->m_function_count++;
    ((VISAKernelImpl *)function)->setIsKernel(false);
    m_functionsVector.push_back(function);
    return VISA_SUCCESS;
}

// default size of the physical reg pool mem manager in bytes
#define PHY_REG_MEM_SIZE   (16*1024)

struct FCallState
{
    G4_INST* fcallInst;
    G4_Operand* opnd0;
    G4_Operand* opnd1;
    G4_BB* retBlock;
    unsigned int execSize;
};

struct SavedFCallStates
{
    std::vector<std::pair<G4_Kernel*, FCallState>> states;
    std::vector<G4_BB*> retbbs;
};

void saveFCallState(G4_Kernel* kernel, SavedFCallStates& savedFCallState)
{
    // Iterate over all BBs in kernel.
    // For each fcall seen, store its opnd0, opnd1, retBlock.
    // so that after compiling the copy of function for 1 kernel,
    // the IR can be reused for another kernel rather than
    // recompiling.
    // kernel points to a stackcall function.
    std::set<G4_BB*> calledFrets;
    for (auto curBB : kernel->fg)
    {
        if( curBB->size() > 0 && curBB->isEndWithFCall() )
        {
            // Save state for this fcall
            G4_INST* fcallInst = curBB->back();

            FCallState currFCallState;

            currFCallState.fcallInst = fcallInst;
            currFCallState.opnd0 = fcallInst->getSrc(0);
            currFCallState.opnd1 = fcallInst->getSrc(1);
            currFCallState.retBlock = curBB->Succs.front();
            currFCallState.execSize = fcallInst->getExecSize();

            savedFCallState.states.push_back( std::make_pair( kernel, currFCallState ) );
            calledFrets.insert(currFCallState.retBlock);
        }
        if (curBB->size() > 0 && curBB->isEndWithFRet() && !calledFrets.count(curBB))
        {
            savedFCallState.retbbs.push_back(curBB);
        }
    }
}

void restoreFCallState(G4_Kernel* kernel, SavedFCallStates savedFCallState)
{
    // Iterate over all BBs in kernel and fix all fcalls converted
    // to calls by reconverting them to fcall. This is required
    // because we want to reuse IR of function for next kernel.

    // start, end iterators denote boundaries in vector that correspond
    // to current kernel. This assumes that entries for different
    // functions are not interspersed.
    auto start = savedFCallState.states.begin(), end = savedFCallState.states.end();

    for( BB_LIST_ITER bb_it = kernel->fg.begin();
        bb_it != kernel->fg.end();
        bb_it++ )
    {
        G4_BB* curBB = (*bb_it);

        if( curBB->size() > 0 &&
            curBB->back()->isCall() )
        {
            // Check whether this call is a convert from fcall
            for( auto state_it = start;
                state_it != end;
                state_it++ )
            {
                if( (*state_it).second.fcallInst == curBB->back() )
                {
                    // Found a call to replace with fcall and ret with fret

                    // Restore corresponding ret to fret
                    G4_BB* retBlock = (*state_it).second.retBlock;

                    G4_BB* retbbToConvert = retBlock->Preds.back();

                    G4_INST* retToReplace = retbbToConvert->back();

                    retToReplace->setOpcode( G4_pseudo_fret );
                    retToReplace->setDest(NULL);

                    kernel->fg.removePredSuccEdges(retbbToConvert, retBlock);

                    // Now restore call operands
                    G4_INST* instToReplace = curBB->back();

                    auto& state = (*state_it).second;
                    instToReplace->setSrc(state.opnd0, 0);
                    instToReplace->setSrc(state.opnd1, 1);
                    instToReplace->setExecSize((unsigned char)state.execSize);

                    // Remove edge between call and previously joined function
                    while( curBB->Succs.size() > 0 )
                    {
                        kernel->fg.removePredSuccEdges( curBB, curBB->Succs.front() );
                    }

                    // Restore edge to retBlock
                    kernel->fg.addPredSuccEdges( curBB, (*state_it).second.retBlock );

                    instToReplace->setOpcode( G4_pseudo_fcall );
                }
            }
        }
    }

    for (G4_BB* retBB : savedFCallState.retbbs)
    {
        G4_INST* retToReplace = retBB->back();

        retToReplace->setOpcode(G4_pseudo_fret);
        retToReplace->setDest(NULL);

    }

    // Remove all in-edges to stack call function. These may have been added
    // to connect earlier kernels with the function.
    while( kernel->fg.getEntryBB()->Preds.size() > 0 )
    {
        kernel->fg.removePredSuccEdges( kernel->fg.getEntryBB()->Preds.front(), kernel->fg.getEntryBB() );
    }
}

// Stitch the Gen binary for all functions in this vISA program with the given kernel
// It modifies pseudo_fcall/fret in to call/ret opcodes.
// ToDo: may consider stitching only functions that may be called by this kernel
static void Stitch_Compiled_Units(G4_Kernel* kernel, std::map<std::string, G4_Kernel*>& compilation_units)
{

    // Append flowgraph of all callees to kernel. For now just assume all functions in the modules
    // may be called
    for (auto&& iter : compilation_units)
    {
        G4_Kernel* callee = iter.second;
        kernel->fg.append(callee->fg);

        // merge the relocation when append
        if (!callee->getRelocationTable().empty())
            kernel->getRelocationTable().insert(kernel->getRelocationTable().end(),
                callee->getRelocationTable().begin(), callee->getRelocationTable().end());
    }

    kernel->fg.reassignBlockIDs();

    // Change fcall/fret to call/ret and setup caller/callee edges
    for (G4_BB* cur : kernel->fg)
    {
        if (cur->size() > 0 && cur->isEndWithFCall())
        {
            // Setup successor/predecessor
            G4_INST* fcall = cur->back();
            if (kernel->getOption(vISA_GenerateDebugInfo))
            {
                kernel->getKernelDebugInfo()->setFCallInst(fcall);
            }

            if (!fcall->asCFInst()->isIndirectCall())
            {
                std::string funcName = fcall->getSrc(0)->asLabel()->getLabel();

                auto iter = compilation_units.find(funcName);
                assert(iter != compilation_units.end() && "can't find function with given name");
                G4_Kernel* callee = iter->second;
                G4_BB* retBlock = cur->Succs.front();
                ASSERT_USER(cur->Succs.size() == 1, "fcall basic block cannot have more than 1 successor");
                ASSERT_USER(retBlock->Preds.size() == 1, "block after fcall cannot have more than 1 predecessor");

                // Remove old edge
                retBlock->Preds.erase(retBlock->Preds.begin());
                cur->Succs.erase(cur->Succs.begin());

                // Connect new fg
                kernel->fg.addPredSuccEdges(cur, callee->fg.getEntryBB());
                kernel->fg.addPredSuccEdges(callee->fg.getUniqueReturnBlock(), retBlock);

                G4_INST* calleeLabel = callee->fg.getEntryBB()->front();
                ASSERT_USER(calleeLabel->isLabel() == true, "Entry inst is not label");

                // ret/e-mask
                fcall->setSrc(fcall->getSrc(0), 1);

                // dst label
                fcall->setSrc(calleeLabel->getSrc(0), 0);
                fcall->setOpcode(G4_call);
            }
            else
            {
                fcall->setSrc(fcall->getSrc(0), 1);
                fcall->setOpcode(G4_call);
            }
        }
    }

    // Change fret to ret
    for (G4_BB* cur : kernel->fg)
    {
        if( cur->size() > 0 && cur->isEndWithFRet() )
        {
            G4_INST* fret = cur->back();
            ASSERT_USER( fret->opcode() == G4_pseudo_fret, "Expecting to see pseudo_fret");
            fret->setOpcode( G4_return );
            fret->setDest( kernel->fg.builder->createNullDst(Type_UD) );
        }
    }

    // Append declarations and color attributes from all callees to kernel
    for (auto iter : compilation_units)
    {
        G4_Kernel* callee = iter.second;
        for (auto curDcl : callee->Declares)
        {
            kernel->Declares.push_back(curDcl);
        }
    }
}


int CISA_IR_Builder::WriteVISAHeader()
{
    if (m_options.getOption(vISA_IsaAssembly))
    {
        unsigned funcId = 0;
        if (!m_kernel->getIsKernel())
        {
            m_kernel->GetFunctionId(funcId);
        }

        VISAKernel_format_provider fmt(m_kernel);
        m_ssIsaAsmHeader << printKernelHeader(this->m_header, &fmt, m_kernel->getIsKernel(), funcId, &this->m_options) << endl;
        return VISA_SUCCESS;
    }
    return VISA_FAILURE;
}

typedef struct yy_buffer_state * YY_BUFFER_STATE;
extern int CISAparse(CISA_IR_Builder *builder);
extern YY_BUFFER_STATE CISA_scan_string(const char* yy_str);
extern void CISA_delete_buffer(YY_BUFFER_STATE buf);

int CISA_IR_Builder::ParseVISAText(const std::string& visaHeader, const std::string& visaText, const std::string& visaTextFile)
{
#if defined(__linux__) || defined(_WIN64) || defined(_WIN32)
    // Direct output of parser to null
#if defined(_WIN64) || defined(_WIN32)
    CISAout = fopen("nul", "w");
#else
    CISAout = fopen("/dev/null", "w");
#endif

    // Dump the visa text
    if (m_options.getOption(vISA_GenerateISAASM) && !visaTextFile.empty())
    {
        FILE* dumpFile = fopen(visaTextFile.c_str(), "wb+");
        if (dumpFile)
        {
            // Write the header
            if (std::fputs(visaHeader.c_str(), dumpFile) == EOF)
            {
                assert(0 && "Failed to write visa text to file");
                fclose(dumpFile);
                return VISA_FAILURE;
            }
            // Write the declarations and instructions
            if (std::fputs(visaText.c_str(), dumpFile) == EOF)
            {
                assert(0 && "Failed to write visa text to file");
                fclose(dumpFile);
                return VISA_FAILURE;
            }
            fclose(dumpFile);
        }
    }

    // Parse the header string
    if (!visaHeader.empty())
    {
        YY_BUFFER_STATE headerBuf = CISA_scan_string(visaHeader.c_str());
        if (CISAparse(this) != 0)
        {
            assert(0 && "Parsing header message failed");
            return VISA_FAILURE;
        }
        CISA_delete_buffer(headerBuf);
    }

    // Parse the visa body
    if (!visaText.empty())
    {
        YY_BUFFER_STATE visaBuf = CISA_scan_string(visaText.c_str());
        if (CISAparse(this) != 0)
        {
            assert(0 && "Parsing visa text failed");
            return VISA_FAILURE;
        }
        CISA_delete_buffer(visaBuf);
    }

    if (CISAout)
    {
        fclose(CISAout);
    }

    return VISA_SUCCESS;
#else
    assert(0 && "Asm parsing not supported on this platform");
    return VISA_FAILURE;
#endif
}

// Parses inline asm file from ShaderOverride
int CISA_IR_Builder::ParseVISAText(const std::string& visaFile)
{
#if defined(__linux__) || defined(_WIN64) || defined(_WIN32)
    // Direct output of parser to null
#if defined(_WIN64) || defined(_WIN32)
    CISAout = fopen("nul", "w");
#else
    CISAout = fopen("/dev/null", "w");
#endif
    CISAin = fopen(visaFile.c_str(), "r");
    if (!CISAin)
    {
        assert(0 && "Failed to open file");
        return VISA_FAILURE;
    }

    if (CISAparse(this) != 0)
    {
        assert(0 && "Parsing visa text failed");
        return VISA_FAILURE;
    }
    fclose(CISAin);

    if (CISAout)
    {
        fclose(CISAout);
    }
    return VISA_SUCCESS;
#else
    assert(0 && "Asm parsing not supported on this platform");
    return VISA_FAILURE;
#endif
}

// default size of the kernel mem manager in bytes
#define KERNEL_MEM_SIZE    (4*1024*1024)
int CISA_IR_Builder::Compile(const char* nameInput, std::ostream* os, bool emit_visa_only)
{

    stopTimer(TIMER_BUILDER);   // TIMER_BUILDER is started when builder is created
    int status = VISA_SUCCESS;

    std::string name = std::string(nameInput);

    if (IS_VISA_BOTH_PATH)
    {
        if (m_options.getOption(vISA_IsaAssembly))
        {
            assert(0 && "Should not be calling Compile() in asm text writter mode!");
            return VISA_FAILURE;
        }

        std::list< VISAKernelImpl *>::iterator iter = m_kernels.begin();
        std::list< VISAKernelImpl *>::iterator end = m_kernels.end();
        CBinaryCISAEmitter cisaBinaryEmitter;
        int kernelIndex = 0;
        if ( IS_BOTH_PATH )
        {
            m_options.setOptionInternally(vISA_NumGenBinariesWillBePatched, (uint32_t) 1);
        }
        m_cisaBinary->initCisaBinary(m_kernel_count, m_function_count);
        m_cisaBinary->setMajorVersion((unsigned char)this->m_header.major_version);
        m_cisaBinary->setMinorVersion((unsigned char)this->m_header.minor_version);
        m_cisaBinary->setMagicNumber(COMMON_ISA_MAGIC_NUM);

        int status = VISA_SUCCESS;
        for( ; iter != end; iter++, kernelIndex++ )
        {
            VISAKernelImpl * kTemp = *iter;
            kTemp->finalizeAttributes();
            unsigned int binarySize = 0;
            status = cisaBinaryEmitter.Emit(kTemp, binarySize);
            m_cisaBinary->initKernel(kernelIndex, kTemp);
        }
        m_cisaBinary->finalizeCisaBinary();

        if (status != VISA_SUCCESS)
        {
            return status;
        }

        // We call the verifier and dumper directly.
        if (m_options.getOption(vISA_GenerateISAASM) || !m_options.getOption(vISA_NoVerifyvISA))
        {
            m_cisaBinary->isaDumpVerify(m_kernels, &m_options);
        }
    }

    /*
        In case there is an assert in compilation phase, at least vISA binary will be generated.
    */
    if (IS_VISA_BOTH_PATH && m_options.getOption(vISA_DumpvISA) && nameInput && !os)
    {
        status = m_cisaBinary->dumpToFile(name);
    }

    if (os && emit_visa_only)
    {
        return m_cisaBinary->dumpToStream(os);
    }

    if ( IS_GEN_BOTH_PATH )
    {
        Mem_Manager mem(4096);
        common_isa_header pseudoHeader;
        // m_kernels contains kernels and functions to compile.
        std::list< VISAKernelImpl *>::iterator iter = m_kernels.begin();
        std::list< VISAKernelImpl *>::iterator end = m_kernels.end();
        iter = m_kernels.begin();
        end = m_kernels.end();

        pseudoHeader.num_kernels = 0;
        pseudoHeader.num_functions = 0;
        for( ; iter != end; iter++ )
        {
            if( (*iter)->getIsKernel() == true )
            {
                pseudoHeader.num_kernels++;
            }
            else
            {
                pseudoHeader.num_functions++;
            }
        }

        pseudoHeader.functions = (function_info_t*)mem.alloc(sizeof(function_info_t) * pseudoHeader.num_functions);

        int i;
        unsigned int k = 0;
        std::list<VISAKernelImpl*> kernels;
        std::list<VISAKernelImpl*> functions;
        for( iter = m_kernels.begin(), i = 0; iter != end; iter++, i++ )
        {
            VISAKernelImpl* kernel = (*iter);
            kernel->finalizeAttributes();
            kernel->getIRBuilder()->setIsKernel(kernel->getIsKernel());
            if( kernel->getIsKernel() == false )
            {
                if (kernel->getIRBuilder()->getArgSize() < kernel->getKernelFormat()->input_size)
                {
                    kernel->getIRBuilder()->setArgSize(kernel->getKernelFormat()->input_size);
                }
                if (kernel->getIRBuilder()->getRetVarSize() < kernel->getKernelFormat()->return_value_size)
                {
                    kernel->getIRBuilder()->setRetVarSize(kernel->getKernelFormat()->return_value_size);
                }

                strcpy_s((char*)&pseudoHeader.functions[k].name, COMMON_ISA_MAX_FILENAME_LENGTH, (*iter)->getKernel()->getName());
                k++;
                functions.push_back(kernel);
            }
            else
            {
                kernels.push_back(kernel);
            }

            int status =  kernel->compileFastPath();
            if (status != VISA_SUCCESS)
            {
                stopTimer(TIMER_TOTAL);
                return status;
            }
        }

        SavedFCallStates savedFCallState;

        for(std::list<VISAKernelImpl*>::iterator kernel_it = kernels.begin(), kend = kernels.end();
            kernel_it != kend;
            kernel_it++)
        {
            VISAKernelImpl* kernel = (*kernel_it);

            saveFCallState(kernel->getKernel(), savedFCallState);
        }

        for( std::list<VISAKernelImpl*>::iterator func_it = functions.begin(), fend = functions.end();
            func_it != fend;
            func_it++ )
        {
            VISAKernelImpl* function = (*func_it);

            saveFCallState( function->getKernel(), savedFCallState );
        }

        std::map<std::string, G4_Kernel*> allFunctions;

        for (auto func_it = functions.begin(); func_it != functions.end(); func_it++)
        {
            G4_Kernel* func = (*func_it)->getKernel();
            allFunctions[std::string(func->getName())] = func;
            if (m_options.getOption(vISA_GenerateDebugInfo))
            {
                func->getKernelDebugInfo()->resetRelocOffset();
                resetGenOffsets(*func);
            }
        }

        for (auto kernel_it = kernels.begin(); kernel_it != kernels.end(); kernel_it++ )
        {
            VISAKernelImpl* kernel = (*kernel_it);

            unsigned int genxBufferSize = 0;

            Stitch_Compiled_Units(kernel->getKernel(), allFunctions);

            void* genxBuffer = kernel->compilePostOptimize(genxBufferSize);
            kernel->setGenxBinaryBuffer(genxBuffer, genxBufferSize);

            if(m_options.getOption(vISA_GenerateDebugInfo))
            {
                kernel->computeAndEmitDebugInfo(functions);
            }

            restoreFCallState( kernel->getKernel(), savedFCallState );


        }


    }

    if (IS_VISA_BOTH_PATH && m_options.getOption(vISA_DumpvISA))
    {
        unsigned int numGenBinariesWillBePatched = m_options.getuInt32Option(vISA_NumGenBinariesWillBePatched);

        if (numGenBinariesWillBePatched)
        {
            std::list< VISAKernelImpl *>::iterator iter = m_kernels.begin();
            std::list< VISAKernelImpl *>::iterator end = m_kernels.end();

            int kernelCount = 0;
            int functionCount = 0;

            //only patch for Both path; vISA path doesn't need this.
            for (int i = 0; iter != end; iter++, i++)
            {
                VISAKernelImpl * kTemp = *iter;
                void * genxBuffer = NULL;
                unsigned int genxBufferSize = 0;
                if (kTemp->getIsKernel())
                {
                    genxBuffer = kTemp->getGenxBinaryBuffer();
                    genxBufferSize = kTemp->getGenxBinarySize();
                    m_cisaBinary->patchKernel(kernelCount, genxBufferSize, genxBuffer, getGenxPlatformEncoding());
                    kernelCount++;
                }
            }
            iter = m_kernels.begin();
            for (int i = 0; iter != end; iter++, i++)
            {
              VISAKernelImpl * kTemp = *iter;
              unsigned int genxBufferSize = 0;
              if (!kTemp->getIsKernel())
              {
                genxBufferSize = kTemp->getGenxBinarySize();
                m_cisaBinary->patchFunction(functionCount, genxBufferSize);
                functionCount++;
              }
            }
        }

        if (os)
            status = m_cisaBinary->dumpToStream(os);
        else
            status = m_cisaBinary->dumpToFile(name);
    }

    stopTimer(TIMER_TOTAL); // have to record total time before dump the timer
    if (m_options.getOption(vISA_dumpTimer))
    {
        const char *asmName = nullptr;
        m_options.getOption(VISA_AsmFileName, asmName);
        dumpAllTimers(asmName, true);
    }

#ifndef DLL_MODE
    if (criticalMsg.str().length() > 0)
    {
        std::cerr << "[vISA Finalizer Messsages]\n" << criticalMsg.str();
    }
#endif

    return status;
}

bool CISA_IR_Builder::CISA_general_variable_decl(
    const char * var_name,
    unsigned int var_elemts_num,
    VISA_Type data_type,
    VISA_Align var_align,
    const char * var_alias_name,
    int var_alias_offset,
    attr_gen_struct scope,
    int line_no)
{
    VISA_GenVar * genVar = NULL;

    VISA_GenVar *parentDecl = NULL;

    if (m_kernel->getDeclFromName(var_name) != nullptr) {
        RecordParseError(line_no, var_name, ": variable redeclaration");
        return false;
    }

    if (var_alias_name && strcmp(var_alias_name, "") != 0)
    {
        parentDecl = (VISA_GenVar *)m_kernel->getDeclFromName(var_alias_name);
    }

    m_kernel->CreateVISAGenVar(
        genVar, var_name, var_elemts_num, data_type, var_align,
        parentDecl, var_alias_offset);

    if( scope.attr_set )
    {
        m_kernel->AddAttributeToVar(genVar, scope.name, 1, &scope.value);
    }

    return true;
}

bool CISA_IR_Builder::CISA_addr_variable_decl(
    const char *var_name, unsigned int var_elements,
    VISA_Type data_type, attr_gen_struct scope, int line_no)
{
    if (m_kernel->getDeclFromName(var_name) != nullptr) {
        RecordParseError(line_no, var_name, ": variable redeclaration");
        return false;
    }

    VISA_AddrVar *decl = NULL;
    this->m_kernel->CreateVISAAddrVar(decl, var_name, var_elements);
    if( scope.attr_set )
    {
        m_kernel->AddAttributeToVar(decl, scope.name, 1, &scope.value);
    }
    return true;
}

bool CISA_IR_Builder::CISA_predicate_variable_decl(
    const char *var_name, unsigned int var_elements, attr_gen_struct reg, int line_no)
{
    if (m_kernel->getDeclFromName(var_name) != nullptr) {
        RecordParseError(line_no, var_name, ": variable redeclaration");
        return false;
    }

    int reg_id = reg.value;
    char value[2]; // AddAttributeToVar will perform a copy, so we can stack allocate value
    *value = '0'+reg_id;
    value[1] = '\0';

    VISA_PredVar *decl = NULL;
    m_kernel->CreateVISAPredVar(decl, var_name, (unsigned short)var_elements);
    if( reg.attr_set )
    {
        m_kernel->AddAttributeToVar(decl, reg.name, 2, value);
    }
    return true;
}

bool CISA_IR_Builder::CISA_sampler_variable_decl(
    const char *var_name, int num_elts, const char* name, int line_no)
{
    if (m_kernel->getDeclFromName(var_name) != nullptr) {
        RecordParseError(line_no, var_name, ": variable redeclaration");
        return false;
    }

    VISA_SamplerVar *decl = NULL;
    m_kernel->CreateVISASamplerVar(decl, var_name, num_elts);
    return true;
}

bool CISA_IR_Builder::CISA_surface_variable_decl(
    const char *var_name, int num_elts, const char* name,
    attr_gen_struct attr_val, int line_no)
{
    if (m_kernel->getDeclFromName(var_name) != nullptr) {
        RecordParseError(line_no, var_name, ": variable redeclaration");
        return false;
    }

    int reg_id = attr_val.value;
    char * value = (char *)m_mem.alloc(1);
    *value = (char)reg_id;

    VISA_SurfaceVar *decl = NULL;
    m_kernel->CreateVISASurfaceVar(decl, var_name, num_elts);
    if (attr_val.attr_set)
    {
        m_kernel->AddAttributeToVar(decl, attr_val.name, 1, value);
    }
    return true;
}

bool CISA_IR_Builder::CISA_implicit_input_directive(
    const char * argName, const char *varName,
    short offset, unsigned short size, int line_no)
{
    std::string implicitArgName = argName;
    auto pos = implicitArgName.find("UNDEFINED_");
    uint32_t numVal = 0;
    if ( pos!= std::string::npos)
    {
        pos += strlen("UNDEFINED_");
        auto numValString = implicitArgName.substr(pos, implicitArgName.length());
        numVal = std::stoi(numValString);
    }
    else
    {
        auto implicitInputName = implicitArgName.substr(strlen(".implicit_"), implicitArgName.length());
        for (; numVal < IMPLICIT_INPUT_COUNT; ++numVal)
        {
            if (!implicitInputName.compare(input_info_t::getImplicitKindString(numVal)))
            {
                break;
            }
        }
    }

    int status = VISA_SUCCESS;
    CISA_GEN_VAR *temp = m_kernel->getDeclFromName(varName);
    MUST_BE_TRUE1(temp != NULL, line_no, "Var marked for input was not found!");
    status = m_kernel->CreateVISAImplicitInputVar((VISA_GenVar *)temp, offset, size, numVal);
    if (status != VISA_SUCCESS)
    {
        std::cerr << "Failed to create input Var. Line: " << line_no << std::endl;
        return false;
    }
    return true;
}
bool CISA_IR_Builder::CISA_input_directive(
    const char* var_name, short offset, unsigned short size, int line_no)
{

    int status = VISA_SUCCESS;
    CISA_GEN_VAR *temp = m_kernel->getDeclFromName(var_name);
    MUST_BE_TRUE1(temp != NULL, line_no, "Var marked for input was not found!" );
    status = m_kernel->CreateVISAInputVar((VISA_GenVar *)temp,offset,size);
    if(status != VISA_SUCCESS)
    {
        std::cerr << "Failed to create input Var. Line: " <<line_no << "\n";
        return false;
    }
    return true;
}

bool CISA_IR_Builder::CISA_attr_directive(
    const char* input_name, const char* input_var, int line_no)
{
    Attributes::ID attrID = Attributes::getAttributeID(input_name);
    if (!m_options.getOption(VISA_AsmFileNameUser) &&
        attrID == Attributes::ATTR_OutputAsmPath)
    {
        if (strcmp(input_name, "AsmName") == 0) {
            std::cerr << "WARNING: AsmName deprecated "
                "(replace with OutputAsmPath)\n";
        }
        input_name = "OutputAsmPath"; // normalize to new name

        char asmFileName[MAX_OPTION_STR_LENGTH];

        strncpy_s(asmFileName, MAX_OPTION_STR_LENGTH, input_var, MAX_OPTION_STR_LENGTH-1);
        char *pos = strstr(asmFileName, ".asm");
        if (pos != NULL)
        {
            *pos = '\0';
        }
        m_options.setOptionInternally(VISA_AsmFileName, asmFileName);
    }

    if (attrID == Attributes::ATTR_Target) {
        unsigned char visa_target;
        MUST_BE_TRUE(input_var,
            ".kernel_attr Target=.. must be \"cm\", \"3d\", or \"cs\"");
        if (strcmp(input_var, "cm") == 0) {
            visa_target = VISA_CM;
        }
        else if (strcmp(input_var, "3d") == 0)
        {
            visa_target = VISA_3D;
        }
        else if (strcmp(input_var, "cs") == 0)
        {
            visa_target = VISA_CS;
        }
        else
        {
            MUST_BE_TRUE1(false, line_no, "invalid kernel target attribute");
        }
        m_kernel->AddKernelAttribute(input_name, 1, &visa_target);
    }
    else
    {
        m_kernel->AddKernelAttribute(input_name,
          input_var == nullptr ? 0 : (int)strlen(input_var), input_var);
    }

    return true;
}

bool CISA_IR_Builder::CISA_attr_directiveNum(
    const char* input_name, uint32_t input_var, int line_no)
{
    m_kernel->AddKernelAttribute(input_name, sizeof(uint32_t), &input_var);
    return true;
}

bool CISA_IR_Builder::CISA_create_label(const char *label_name, int line_no)
{
    VISA_LabelOpnd *opnd[1] = {NULL};

    //when we print out ./function from isa we also print out label.
    //if we don't skip it during re-parsing then we will have duplicate labels
    if (m_kernel->getLabelOperandFromFunctionName(std::string(label_name)) == NULL)
    {
        opnd[0] = m_kernel->getLabelOpndFromLabelName(std::string(label_name));
        if (opnd[0] == NULL)
        {
            // forward jump
            m_kernel->CreateVISALabelVar(opnd[0], label_name, LABEL_BLOCK);
        }
        m_kernel->AppendVISACFLabelInst(opnd[0]);
    }

    return true;
}


bool CISA_IR_Builder::CISA_function_directive(const char* func_name)
{
    VISA_LabelOpnd *opnd[1] = {NULL};
    opnd[0] = m_kernel->getLabelOperandFromFunctionName(std::string(func_name));
    if (opnd[0] == NULL)
    {
        m_kernel->CreateVISALabelVar(opnd[0], func_name, LABEL_SUBROUTINE);
    }

    m_kernel->AppendVISACFLabelInst(opnd[0]);
    return true;
}


bool CISA_IR_Builder::CISA_create_arith_instruction(VISA_opnd * pred,
                                                    ISA_Opcode opcode,
                                                    bool  sat,
                                                    VISA_EMask_Ctrl emask,
                                                    unsigned exec_size,
                                                    VISA_opnd * dst_cisa,
                                                    VISA_opnd * src0_cisa,
                                                    VISA_opnd * src1_cisa,
                                                    VISA_opnd * src2_cisa,
                                                    int line_no
                                                    )
{
    VISA_Exec_Size executionSize =  Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    int status = m_kernel->AppendVISAArithmeticInst(opcode, (VISA_PredOpnd *)pred, sat, emask, executionSize,
        (VISA_VectorOpnd *)dst_cisa, (VISA_VectorOpnd *)src0_cisa, (VISA_VectorOpnd *)src1_cisa, (VISA_VectorOpnd *)src2_cisa);
    MUST_BE_TRUE1(status == VISA_SUCCESS, line_no, "Could not create CISA arithmetic instruction.");
    return true;
}

bool CISA_IR_Builder::CISA_create_arith_instruction2(VISA_opnd * pred,
                                                     ISA_Opcode opcode,
                                                     VISA_EMask_Ctrl emask,
                                                     unsigned exec_size,
                                                     VISA_opnd * dst_cisa,
                                                     VISA_opnd * carry_borrow,
                                                     VISA_opnd * src1_cisa,
                                                     VISA_opnd * src2_cisa,
                                                     int line_no
                                                     )
{
    VISA_Exec_Size executionSize =  Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    int status = m_kernel->AppendVISAArithmeticInst(opcode, (VISA_PredOpnd *)pred, emask, executionSize,
        (VISA_VectorOpnd *)dst_cisa, (VISA_VectorOpnd *)carry_borrow, (VISA_VectorOpnd *)src1_cisa, (VISA_VectorOpnd *)src2_cisa);
    MUST_BE_TRUE1(status == VISA_SUCCESS, line_no, "Could not create CISA arithmetic instruction.");
    return true;
}

bool CISA_IR_Builder::CISA_create_mov_instruction(VISA_opnd *pred,
                                                  ISA_Opcode opcode,
                                                  VISA_EMask_Ctrl emask,
                                                  unsigned exec_size,
                                                  bool  sat,
                                                  VISA_opnd *dst,
                                                  VISA_opnd *src0,
                                                  int line_no)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    m_kernel->AppendVISADataMovementInst(opcode, (VISA_PredOpnd*) pred, sat, emask, executionSize, (VISA_VectorOpnd *)dst, (VISA_VectorOpnd *)src0);
    return true;
}

bool CISA_IR_Builder::CISA_create_mov_instruction(
    VISA_opnd *dst, const char *src0_name, int line_no)
{
    CISA_GEN_VAR *src0 = m_kernel->getDeclFromName(src0_name);
    MUST_BE_TRUE1(src0 != NULL, line_no, "The source operand of a move instruction was null");
    m_kernel->AppendVISAPredicateMove((VISA_VectorOpnd *)dst, (VISA_PredVar  *)src0);
    return true;
}

bool CISA_IR_Builder::CISA_create_movs_instruction(VISA_EMask_Ctrl emask,
                                                   ISA_Opcode opcode,
                                                   unsigned exec_size,
                                                   VISA_opnd *dst,
                                                   VISA_opnd *src0,
                                                   int line_no)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    m_kernel->AppendVISADataMovementInst(
        ISA_MOVS, NULL, false, emask, executionSize,
        (VISA_VectorOpnd *)dst, (VISA_VectorOpnd *)src0);
    return true;
}

bool CISA_IR_Builder::CISA_create_branch_instruction(VISA_opnd *pred,
                                                     ISA_Opcode opcode,
                                                     VISA_EMask_Ctrl emask,
                                                     unsigned exec_size,
                                                     const char *target_label,
                                                     int line_no)
{
    VISA_LabelOpnd * opnd[1];
    int i = 0;

    switch(opcode)
    {
    case ISA_CALL:
        {
            //need second path over instruction stream to
            //determine correct IDs since function directive might not have been
            //encountered yet

            opnd[i] = m_kernel->getLabelOperandFromFunctionName(std::string(target_label));
            if( opnd[i] == NULL )
            {
                m_kernel->CreateVISALabelVar(opnd[i], target_label, LABEL_SUBROUTINE);
                opnd[i]->tag = ISA_SUBROUTINE;
            }
            VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
            m_kernel->AppendVISACFCallInst((VISA_PredOpnd *) pred, emask, executionSize, opnd[i]);
            m_kernel->patchLastInst(opnd[i]);
            return true;
        }
    case ISA_JMP:
        {
            opnd[i] = m_kernel->getLabelOpndFromLabelName(std::string(target_label));

            //forward jump label: create the label optimistically
            if( opnd[i] == NULL )
            {
                m_kernel->CreateVISALabelVar(opnd[i], target_label, LABEL_BLOCK);
            }

            m_kernel->AppendVISACFJmpInst((VISA_PredOpnd *) pred, opnd[i]);
            m_kernel->patchLastInst(opnd[i]);
            return true;
        }
    case ISA_GOTO:
        {
            opnd[i] = m_kernel->getLabelOpndFromLabelName(std::string(target_label));

            //forward jump label: create the label optimistically
            if (opnd[i] == nullptr)
            {
                m_kernel->CreateVISALabelVar(opnd[i], target_label, LABEL_BLOCK);
            }
            VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
            m_kernel->AppendVISACFGotoInst((VISA_PredOpnd*)pred, emask, executionSize, opnd[i]);
            m_kernel->patchLastInst(opnd[i]);
            return true;
        }
    default:
        {
            MUST_BE_TRUE(0, "UNKNOWN Branch OP not supported.");
            return false;
        }
    }

    return true;
}

bool CISA_IR_Builder::CISA_create_cmp_instruction(
    VISA_Cond_Mod sub_op,
    ISA_Opcode opcode,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    const char *name,
    VISA_opnd *src0,
    VISA_opnd *src1,
    int line_no)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    CISA_GEN_VAR * decl = m_kernel->getDeclFromName(std::string(name));
    m_kernel->AppendVISAComparisonInst(
        sub_op, emask, executionSize,
        (VISA_PredVar *)decl, (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1);
    return true;
}

bool CISA_IR_Builder::CISA_create_cmp_instruction(
    VISA_Cond_Mod sub_op,
    ISA_Opcode opcode,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd *dst,
    VISA_opnd *src0,
    VISA_opnd *src1,
    int line_no)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    m_kernel->AppendVISAComparisonInst(
        sub_op, emask, executionSize,
        (VISA_VectorOpnd *)dst, (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1);
    return true;
}


bool CISA_IR_Builder::CISA_create_media_instruction(
    ISA_Opcode opcode,
    MEDIA_LD_mod media_mod,
    int block_width,
    int block_height,
    unsigned int plane_ID,
    const char * surface_name,
    VISA_opnd *xOffset,
    VISA_opnd *yOffset,
    VISA_opnd *raw_dst,
    int line_no)
{
    unsigned char mod;
    mod = media_mod & 0x7;
    MUST_BE_TRUE1( (mod < MEDIA_LD_Mod_NUM), line_no, "Common ISA ISA_MEDIA_LD uses illegal exec size." );

    VISA_SurfaceVar *surfaceVar = (VISA_SurfaceVar*)m_kernel->getDeclFromName(surface_name);
    MUST_BE_TRUE1(surfaceVar != NULL, line_no, "Surface was not found");

    VISA_StateOpndHandle * surface = NULL;

    m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar);

    m_kernel->AppendVISASurfAccessMediaLoadStoreInst(
        opcode, media_mod, surface,
        (unsigned char)block_width, (unsigned char)block_height,
        (VISA_VectorOpnd *)xOffset, (VISA_VectorOpnd *)yOffset,
        (VISA_RawOpnd *)raw_dst, (CISA_PLANE_ID)plane_ID);

    return true;
}

/*
For both RET and FRET instructions
*/
bool CISA_IR_Builder::CISA_Create_Ret(
    VISA_opnd *pred_opnd,
    ISA_Opcode opcode,
    VISA_EMask_Ctrl emask,
    unsigned int exec_size,
    int line_no)
{
    if (opcode == ISA_RET)
    {
        VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
        m_kernel->AppendVISACFRetInst((VISA_PredOpnd *)pred_opnd, emask, executionSize);
    }
    else
    {
        VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
        m_kernel->AppendVISACFFunctionRetInst((VISA_PredOpnd *)pred_opnd, emask, executionSize);
    }

    return true;
}

bool CISA_IR_Builder::CISA_create_oword_instruction(ISA_Opcode opcode,
                                                    bool media_mod,
                                                    unsigned int size,
                                                    const char *surface_name,
                                                    VISA_opnd *offset_opnd,
                                                    VISA_opnd *raw_dst_src,
                                                    int line_no)
{
    VISA_SurfaceVar *surfaceVar = (VISA_SurfaceVar*)m_kernel->getDeclFromName(surface_name);
    MUST_BE_TRUE1(surfaceVar != NULL, line_no, "Surface was not found");
    VISA_StateOpndHandle * surface = NULL;
    m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar);
    m_kernel->AppendVISASurfAccessOwordLoadStoreInst(
        opcode, vISA_EMASK_M1, surface,
        Get_VISA_Oword_Num_From_Number(size),
        (VISA_VectorOpnd*)offset_opnd, (VISA_RawOpnd*)raw_dst_src);
    return true;
}

bool CISA_IR_Builder::CISA_create_svm_block_instruction(
    SVMSubOpcode  subopcode,
    unsigned      owords,
    bool          unaligned,
    VISA_opnd*    address,
    VISA_opnd*    srcDst,
    int           line_no)
{
    switch (subopcode)
    {
    case SVM_BLOCK_LD:
        m_kernel->AppendVISASvmBlockLoadInst(
            Get_VISA_Oword_Num_From_Number(owords), unaligned,
            (VISA_VectorOpnd*)address, (VISA_RawOpnd*)srcDst);
        return true;
    case SVM_BLOCK_ST:
        m_kernel->AppendVISASvmBlockStoreInst(
            Get_VISA_Oword_Num_From_Number(owords), unaligned,
            (VISA_VectorOpnd*)address, (VISA_RawOpnd*)srcDst);
        return true;
    default:
        return false;
    }

    return false;
}

bool CISA_IR_Builder::CISA_create_svm_scatter_instruction(
    VISA_opnd*    pred,
    SVMSubOpcode  subopcode,
    VISA_EMask_Ctrl emask,
    unsigned      exec_size,
    unsigned      blockSize,
    unsigned      numBlocks,
    VISA_opnd*    addresses,
    VISA_opnd*    srcDst,
    int           line_no)
{
    VISA_SVM_Block_Type blockType = valueToVISASVMBlockType(blockSize);
    VISA_SVM_Block_Num blockNum = valueToVISASVMBlockNum(numBlocks);
    switch (subopcode)
    {
    case SVM_SCATTER:
        m_kernel->AppendVISASvmScatterInst(
            (VISA_PredOpnd*)pred, emask, Get_VISA_Exec_Size_From_Raw_Size(exec_size),
            blockType, blockNum, (VISA_RawOpnd*)addresses, (VISA_RawOpnd*)srcDst);
        return true;
    case SVM_GATHER:
        m_kernel->AppendVISASvmGatherInst(
            (VISA_PredOpnd*)pred, emask, Get_VISA_Exec_Size_From_Raw_Size(exec_size),
            blockType, blockNum, (VISA_RawOpnd*)addresses, (VISA_RawOpnd*)srcDst);
        return true;
    default:
        return false;
    }


    return false;
}

bool
CISA_IR_Builder::CISA_create_svm_gather4_scaled(
    VISA_opnd               *pred,
    VISA_EMask_Ctrl         eMask,
    unsigned                execSize,
    ChannelMask             chMask,
    VISA_opnd               *address,
    VISA_opnd               *offsets,
    VISA_opnd               *dst,
    int                     lineNum)
{
    int ret
        = m_kernel->AppendVISASvmGather4ScaledInst(
            static_cast<VISA_PredOpnd *>(pred),
            eMask,
            Get_VISA_Exec_Size_From_Raw_Size(execSize),
            chMask.getAPI(),
            static_cast<VISA_VectorOpnd *>(address),
            static_cast<VISA_RawOpnd *>(offsets),
            static_cast<VISA_RawOpnd *>(dst));

    return ret == VISA_SUCCESS;
}

bool CISA_IR_Builder::CISA_create_svm_scatter4_scaled(
    VISA_opnd              *pred,
    VISA_EMask_Ctrl eMask,
    unsigned               execSize,
    ChannelMask            chMask,
    VISA_opnd              *address,
    VISA_opnd              *offsets,
    VISA_opnd              *src,
    int                    lineNum)
{
    int ret
        = m_kernel->AppendVISASvmScatter4ScaledInst(
            static_cast<VISA_PredOpnd *>(pred),
            eMask,
            Get_VISA_Exec_Size_From_Raw_Size(execSize),
            chMask.getAPI(),
            static_cast<VISA_VectorOpnd *>(address),
            static_cast<VISA_RawOpnd *>(offsets),
            static_cast<VISA_RawOpnd *>(src));

    return ret == VISA_SUCCESS;
}

bool CISA_IR_Builder::CISA_create_svm_atomic_instruction(
    VISA_opnd* pred,
    VISA_EMask_Ctrl emask,
    unsigned   exec_size,
    VISAAtomicOps op,
    unsigned short bitwidth,
    VISA_opnd* addresses,
    VISA_opnd* src0,
    VISA_opnd* src1,
    VISA_opnd* dst,
    int line_no)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    m_kernel->AppendVISASvmAtomicInst(
        (VISA_PredOpnd *)pred, emask, executionSize, op, bitwidth,
        (VISA_RawOpnd *)addresses, (VISA_RawOpnd *)src0, (VISA_RawOpnd *)src1,
        (VISA_RawOpnd *)dst);
    return true;
}

bool CISA_IR_Builder::CISA_create_address_instruction(ISA_Opcode opcode,
                                                      VISA_EMask_Ctrl emask,
                                                      unsigned exec_size,
                                                      VISA_opnd *dst,
                                                      VISA_opnd *src0,
                                                      VISA_opnd *src1,
                                                      int line_no)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    m_kernel->AppendVISAAddrAddInst(
        emask, executionSize,
        (VISA_VectorOpnd *)dst, (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1);
    return true;
}

bool CISA_IR_Builder::CISA_create_logic_instruction(
    VISA_opnd *pred,
    ISA_Opcode opcode,
    bool sat,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd *dst,
    VISA_opnd *src0,
    VISA_opnd *src1,
    VISA_opnd *src2,
    VISA_opnd *src3,
    int line_no)
{
    if( opcode != ISA_SHR &&
        opcode != ISA_SHL &&
        opcode != ISA_ASR )
    {
        MUST_BE_TRUE1(!sat, line_no, "Saturation mode is not supported for this Logic Opcode." );
    }

    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    m_kernel->AppendVISALogicOrShiftInst(
        opcode, (VISA_PredOpnd *)pred, sat, emask, executionSize,
        (VISA_VectorOpnd *)dst, (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1,
        (VISA_VectorOpnd *)src2, (VISA_VectorOpnd *)src3);
    return true;
}

bool CISA_IR_Builder::CISA_create_logic_instruction(
    ISA_Opcode opcode,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    const char *dst_name,
    const char *src0_name,
    const char *src1_name,
    int line_no)
{
    if( opcode != ISA_AND &&
        opcode != ISA_OR  &&
        opcode != ISA_NOT &&
        opcode != ISA_XOR )
    {
        MUST_BE_TRUE1(false, line_no, "Prediate variables are not supported for this Logic Opcode." );
    }
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    CISA_GEN_VAR *dst = m_kernel->getDeclFromName(dst_name);
    MUST_BE_TRUE1(dst != NULL, line_no, "The destination operand of a logical instruction was null");
    CISA_GEN_VAR *src0 = m_kernel->getDeclFromName(src0_name);
    MUST_BE_TRUE1(src0 != NULL, line_no, "The first source operand of a logical instruction was null");
    CISA_GEN_VAR *src1 = NULL;
    if ( opcode != ISA_NOT )
    {
        src1 = m_kernel->getDeclFromName(src1_name);
        MUST_BE_TRUE1(src1 != NULL, line_no, "The second source operand of a logical instruction was null");
    }
    m_kernel->AppendVISALogicOrShiftInst(
        opcode, emask, executionSize,
        (VISA_PredVar *)dst, (VISA_PredVar *)src0, (VISA_PredVar *)src1);
    return true;
}

bool CISA_IR_Builder::CISA_create_math_instruction(
    VISA_opnd *pred,
    ISA_Opcode opcode,
    bool  sat,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd *dst,
    VISA_opnd *src0,
    VISA_opnd *src1,
    int line_no)
{
    VISA_Exec_Size executionSize =  Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    m_kernel->AppendVISAArithmeticInst(
        opcode, (VISA_PredOpnd *)pred, sat, emask, executionSize,
        (VISA_VectorOpnd *)dst, (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1, NULL);
    return true;
}

bool CISA_IR_Builder::CISA_create_setp_instruction(
    ISA_Opcode opcode,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    const char * var_name,
    VISA_opnd *src0,
    int line_no)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    CISA_GEN_VAR *dst = m_kernel->getDeclFromName(var_name);
    m_kernel->AppendVISASetP(emask, executionSize, (VISA_PredVar *)dst, (VISA_VectorOpnd *)src0);
    return true;
}

bool CISA_IR_Builder::CISA_create_sel_instruction(
    ISA_Opcode opcode,
    bool sat,
    VISA_opnd *pred,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd *dst,
    VISA_opnd *src0,
    VISA_opnd *src1,
    int line_no)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    m_kernel->AppendVISADataMovementInst(
        opcode, (VISA_PredOpnd*)pred, sat, emask, executionSize,
        (VISA_VectorOpnd *)dst, (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1);
    return true;
}

bool CISA_IR_Builder::CISA_create_fminmax_instruction(
    bool minmax,
    ISA_Opcode opcode,
    bool sat,
    VISA_opnd *pred,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd *dst,
    VISA_opnd *src0,
    VISA_opnd *src1,
    int line_no)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    m_kernel->AppendVISAMinMaxInst(
        (minmax ? CISA_DM_FMAX : CISA_DM_FMIN), sat, emask, executionSize,
        (VISA_VectorOpnd *)dst, (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1);
    return true;
}

bool CISA_IR_Builder::CISA_create_scatter_instruction(
    ISA_Opcode opcode,
    int elt_size,
    VISA_EMask_Ctrl emask,
    unsigned elemNum,
    bool modifier,
    const char *surface_name,
    VISA_opnd *global_offset, //global_offset
    VISA_opnd *element_offset, //element_offset
    VISA_opnd *raw_dst_src, //dst/src
    int line_no)
{
    //GATHER  0x39 (GATHER)  Elt_size   Is_modified Num_elts    Surface Global_Offset   Element_Offset  Dst
    //SCATTER 0x3A (SCATTER) Elt_size               Num_elts    Surface Global_Offset   Element_Offset  Src
    VISA_SurfaceVar *surfaceVar = (VISA_SurfaceVar*)m_kernel->getDeclFromName(surface_name);
    MUST_BE_TRUE1(surfaceVar != NULL, line_no, "Surface was not found");

    VISA_StateOpndHandle * surface = NULL;

    m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar);

    MUST_BE_TRUE1(elemNum == 16 || elemNum == 8 || elemNum == 1, line_no,
        "Unsupported number of elements for gather/scatter instruction.");

    VISA_Exec_Size executionSize = EXEC_SIZE_16;

    if(elemNum == 16)
    {
        executionSize = EXEC_SIZE_16;
    }
    else if(elemNum == 8)
    {
        executionSize = EXEC_SIZE_8;
    }
    else if(elemNum == 1)
    {
        executionSize = EXEC_SIZE_1;
    }

    GATHER_SCATTER_ELEMENT_SIZE elementSize = GATHER_SCATTER_BYTE_UNDEF;
    if(elt_size == 1)
    {
        elementSize = GATHER_SCATTER_BYTE;
    }else if( elt_size == 2)
    {
        elementSize = GATHER_SCATTER_WORD;
    }else if(elt_size == 4)
    {
        elementSize = GATHER_SCATTER_DWORD;
    }

    m_kernel->AppendVISASurfAccessGatherScatterInst(
        opcode, emask, elementSize, executionSize, surface,
        (VISA_VectorOpnd *)global_offset, (VISA_RawOpnd *)element_offset, (VISA_RawOpnd *)raw_dst_src);
    return true;
}

bool CISA_IR_Builder::CISA_create_scatter4_typed_instruction(
    ISA_Opcode opcode,
    VISA_opnd *pred,
    ChannelMask ch_mask,
    VISA_EMask_Ctrl emask,
    unsigned execSize,
    const char* surfaceName,
    VISA_opnd *uOffset,
    VISA_opnd *vOffset,
    VISA_opnd *rOffset,
    VISA_opnd *lod,
    VISA_opnd *dst,
    int line_no)
{
    VISA_SurfaceVar *surfaceVar = (VISA_SurfaceVar*)m_kernel->getDeclFromName(surfaceName);
    MUST_BE_TRUE1(surfaceVar != NULL, line_no, "Surface was not found");

    VISA_StateOpndHandle * surface = NULL;
    m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar);
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(execSize);
    m_kernel->AppendVISASurfAccessGather4Scatter4TypedInst(
        opcode, (VISA_PredOpnd *)pred, ch_mask.getAPI(), emask, executionSize, surface,
        (VISA_RawOpnd *)uOffset, (VISA_RawOpnd *)vOffset, (VISA_RawOpnd *)rOffset,
        (VISA_RawOpnd *)lod, (VISA_RawOpnd*)dst);
    return true;
}

bool CISA_IR_Builder::CISA_create_scatter4_scaled_instruction(
    ISA_Opcode                opcode,
    VISA_opnd                 *pred,
    VISA_EMask_Ctrl           eMask,
    unsigned                  execSize,
    ChannelMask               chMask,
    const char                *surfaceName,
    VISA_opnd                 *globalOffset,
    VISA_opnd                 *offsets,
    VISA_opnd                 *dstSrc,
    int                       lineNo)
{
    VISA_SurfaceVar *surfaceVar =
        (VISA_SurfaceVar*)m_kernel->getDeclFromName(surfaceName);
    MUST_BE_TRUE1(surfaceVar != NULL, lineNo, "Surface was not found");

    VISA_StateOpndHandle *surface = NULL;
    m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar);

    int ret = m_kernel->AppendVISASurfAccessGather4Scatter4ScaledInst(
                opcode, static_cast<VISA_PredOpnd *>(pred),
                eMask, Get_VISA_Exec_Size_From_Raw_Size(execSize),
                chMask.getAPI(),
                surface,
                static_cast<VISA_VectorOpnd *>(globalOffset),
                static_cast<VISA_RawOpnd *>(offsets),
                static_cast<VISA_RawOpnd *>(dstSrc));

    return ret == VISA_SUCCESS;
}

bool CISA_IR_Builder::CISA_create_scatter_scaled_instruction(
    ISA_Opcode             opcode,
    VISA_opnd              *pred,
    VISA_EMask_Ctrl        eMask,
    unsigned               execSize,
    unsigned               numBlocks,
    const char             *surfaceName,
    VISA_opnd              *globalOffset,
    VISA_opnd              *offsets,
    VISA_opnd              *dstSrc,
    int                    lineNo)
{
    VISA_SurfaceVar *surfaceVar =
        (VISA_SurfaceVar*)m_kernel->getDeclFromName(surfaceName);
    MUST_BE_TRUE1(surfaceVar != NULL, lineNo, "Surface was not found");

    VISA_StateOpndHandle *surface = NULL;
    m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar);

    int ret = m_kernel->AppendVISASurfAccessScatterScaledInst(
                opcode, static_cast<VISA_PredOpnd *>(pred),
                eMask, Get_VISA_Exec_Size_From_Raw_Size(execSize),
                valueToVISASVMBlockNum(numBlocks),
                surface,
                static_cast<VISA_VectorOpnd *>(globalOffset),
                static_cast<VISA_RawOpnd *>(offsets),
                static_cast<VISA_RawOpnd *>(dstSrc));

    return ret == VISA_SUCCESS;
}

bool CISA_IR_Builder::CISA_create_sync_instruction(ISA_Opcode opcode)
{
    VISA_INST_Desc *inst_desc = NULL;
    inst_desc = &CISA_INST_table[opcode];

    CisaFramework::CisaInst * inst = new(m_mem)CisaFramework::CisaInst(m_mem);

    inst->createCisaInstruction(opcode, EXEC_SIZE_1, 0 , 0 ,NULL, 0, inst_desc);
    m_kernel->addInstructionToEnd(inst);
    return true;
}

bool CISA_IR_Builder::CISA_create_sbarrier_instruction(bool isSignal)
{
    int ret = m_kernel->AppendVISASplitBarrierInst(isSignal);
    return ret == VISA_SUCCESS;
}

bool CISA_IR_Builder::CISA_create_FILE_instruction(ISA_Opcode opcode, const char * file_name)
{
    m_kernel->AppendVISAMiscFileInst(file_name);
    return true;
}

bool CISA_IR_Builder::CISA_create_LOC_instruction(ISA_Opcode opcode, unsigned int loc)
{
    m_kernel->AppendVISAMiscLOC(loc);
    return true;
}

bool CISA_IR_Builder::CISA_create_invtri_inst(VISA_opnd *pred,
                                              ISA_Opcode opcode,
                                              bool  sat,
                                              VISA_EMask_Ctrl emask,
                                              unsigned exec_size,
                                              VISA_opnd *dst,
                                              VISA_opnd *src0,
                                              int line_no)
{
    int num_operands = 0;
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[4];
    inst_desc = &CISA_INST_table[opcode];
    VISA_Modifier mod = MODIFIER_NONE;

    if(sat)
        mod = MODIFIER_SAT;

    if(dst != NULL)
    {
        dst->_opnd.v_opnd.tag += mod<<3;
        opnd[num_operands] = dst;
        num_operands ++;
    }

    if(src0 != NULL)
    {
        opnd[num_operands] = src0;
        num_operands ++;
    }

    //pred id
    unsigned short pred_id = 0;
    if (pred != NULL)
        pred_id = pred->_opnd.v_opnd.opnd_val.pred_opnd.index;

    CisaFramework::CisaInst * inst = new(m_mem)CisaFramework::CisaInst(m_mem);

    unsigned char size = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    size += emask << 4;
    inst->createCisaInstruction(opcode, size, 0 , pred_id,opnd, num_operands, inst_desc);
    m_kernel->addInstructionToEnd(inst);

    return true;
}

bool CISA_IR_Builder::CISA_create_dword_atomic_instruction(
    VISA_opnd *pred,
    VISAAtomicOps subOpc,
    bool is16Bit,
    VISA_EMask_Ctrl eMask,
    unsigned execSize,
    const char *surfaceName,
    VISA_opnd *offsets,
    VISA_opnd *src0,
    VISA_opnd *src1,
    VISA_opnd *dst,
    int lineNo)
{
    VISA_SurfaceVar *surfaceVar =
        (VISA_SurfaceVar*)m_kernel->getDeclFromName(surfaceName);
    MUST_BE_TRUE1(surfaceVar != NULL, lineNo, "Surface was not found");

    VISA_StateOpndHandle *surface = NULL;
    m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar);

    int ret =
        m_kernel->AppendVISASurfAccessDwordAtomicInst(
                static_cast<VISA_PredOpnd *>(pred),
                subOpc,
                is16Bit,
                eMask, Get_VISA_Exec_Size_From_Raw_Size(execSize),
                surface,
                static_cast<VISA_RawOpnd *>(offsets),
                static_cast<VISA_RawOpnd *>(src0),
                static_cast<VISA_RawOpnd *>(src1),
                static_cast<VISA_RawOpnd *>(dst));

    return ret == VISA_SUCCESS;
}

bool CISA_IR_Builder::CISA_create_typed_atomic_instruction(
    VISA_opnd *pred,
    VISAAtomicOps subOpc,
    bool is16Bit,
    VISA_EMask_Ctrl eMask,
    unsigned execSize,
    const char *surfaceName,
    VISA_opnd *u,
    VISA_opnd *v,
    VISA_opnd *r,
    VISA_opnd *lod,
    VISA_opnd *src0,
    VISA_opnd *src1,
    VISA_opnd *dst,
    int lineNo)
{
    VISA_SurfaceVar *surfaceVar =
        (VISA_SurfaceVar*)m_kernel->getDeclFromName(surfaceName);
    MUST_BE_TRUE1(surfaceVar != nullptr, lineNo, "Surface was not found");

    VISA_StateOpndHandle *surface = nullptr;
    m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar);

    int ret =
        m_kernel->AppendVISA3dTypedAtomic(
        subOpc,
        is16Bit,
        static_cast<VISA_PredOpnd *>(pred),
        eMask, Get_VISA_Exec_Size_From_Raw_Size(execSize),
        surface,
        static_cast<VISA_RawOpnd *>(u),
        static_cast<VISA_RawOpnd *>(v),
        static_cast<VISA_RawOpnd *>(r),
        static_cast<VISA_RawOpnd *>(lod),
        static_cast<VISA_RawOpnd *>(src0),
        static_cast<VISA_RawOpnd *>(src1),
        static_cast<VISA_RawOpnd *>(dst));

    return ret == VISA_SUCCESS;
}

bool CISA_IR_Builder::CISA_create_avs_instruction(
    ChannelMask channel,
    const char* surface_name,
    const char* sampler_name,
    VISA_opnd *u_offset,
    VISA_opnd *v_offset,
    VISA_opnd *deltaU,
    VISA_opnd *deltaV,
    VISA_opnd *u2d,
    VISA_opnd *groupID,
    VISA_opnd *verticalBlockNumber,
    OutputFormatControl cntrl,
    VISA_opnd *v2d,
    AVSExecMode execMode,
    VISA_opnd *iefbypass,
    VISA_opnd *dst,
    int line_no)
{
    VISA_SurfaceVar *surfaceVar = (VISA_SurfaceVar*)m_kernel->getDeclFromName(surface_name);
    MUST_BE_TRUE1(surfaceVar != NULL, line_no, "Surface was not found");

    VISA_StateOpndHandle * surface = NULL;
    m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar);

    VISA_SamplerVar *samplerVar = (VISA_SamplerVar *) m_kernel->getDeclFromName(sampler_name);
    MUST_BE_TRUE1(samplerVar != NULL, line_no, "Sampler was not found");

    VISA_StateOpndHandle *sampler = NULL;
    m_kernel->CreateVISAStateOperandHandle(sampler, samplerVar);
    m_kernel->AppendVISAMEAVS(
        surface, sampler, channel.getAPI(),
        (VISA_VectorOpnd *)u_offset, (VISA_VectorOpnd *)v_offset, (VISA_VectorOpnd *)deltaU,
        (VISA_VectorOpnd *)deltaV, (VISA_VectorOpnd *)u2d, (VISA_VectorOpnd *)v2d,
        (VISA_VectorOpnd *)groupID, (VISA_VectorOpnd *)verticalBlockNumber, cntrl,
        execMode, (VISA_VectorOpnd *)iefbypass, (VISA_RawOpnd *)dst);
    return true;
}

bool CISA_IR_Builder::CISA_create_urb_write_3d_instruction(VISA_opnd* pred,
                                                           VISA_EMask_Ctrl emask,
                                                           unsigned exec_size,
                                                           unsigned int num_out,
                                                           unsigned int global_offset,
                                                           VISA_opnd* channel_mask,
                                                           VISA_opnd* urb_handle,
                                                           VISA_opnd* per_slot_offset,
                                                           VISA_opnd* vertex_data,
                                                           int line_no)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    m_kernel->AppendVISA3dURBWrite(
        (VISA_PredOpnd*)pred, emask, executionSize, (unsigned char)num_out,
        (VISA_RawOpnd*) channel_mask, (unsigned short)global_offset,
        (VISA_RawOpnd*)urb_handle, (VISA_RawOpnd*)per_slot_offset, (VISA_RawOpnd*)vertex_data );
    return true;
}

bool CISA_IR_Builder::CISA_create_rtwrite_3d_instruction(
    VISA_opnd* pred,
    const char* mode,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    const char* surface_name,
    const std::vector<VISA_opnd*> &operands,
    int line_no)
{
    vISA_RT_CONTROLS cntrls;

    memset(&cntrls, 0, sizeof(vISA_RT_CONTROLS));

    VISA_opnd* s0a              = NULL;
    VISA_opnd* oM               = NULL;
    VISA_opnd* R                = NULL;
    VISA_opnd* G                = NULL;
    VISA_opnd* B                = NULL;
    VISA_opnd* A                = NULL;
    VISA_opnd* Z                = NULL;
    VISA_opnd* Stencil          = NULL;
    VISA_opnd *CPSCounter =  NULL;
    VISA_opnd *SamplerIndex = NULL;
    VISA_opnd *r1Header = NULL;
    VISA_opnd *rti = NULL;
    uint8_t counter = 0;

    r1Header = operands[counter++];

    if( mode != NULL )
    {
        if( strstr( mode, "<SI>" ) )
        {
            SamplerIndex = operands[counter++];
        }

        if( strstr( mode, "<CPS>" ) )
        {
            CPSCounter = operands[counter++];
        }

        if(strstr(mode, "<RTI>"))
        {
            cntrls.RTIndexPresent = true;
            rti = operands[counter++];
        }

        if( strstr( mode, "<A>" ) )
        {
            cntrls.s0aPresent = true;
            s0a = operands[counter++];
        }

        if( strstr( mode, "<O>" ) )
        {
            cntrls.oMPresent = true;
            oM = operands[counter++];
        }
        R = operands[counter++];
        G = operands[counter++];
        B = operands[counter++];
        A = operands[counter++];

        if( strstr( mode, "<Z>" ) )
        {
            cntrls.zPresent = true;
            Z = operands[counter++];
        }

        if( strstr( mode, "<ST>" ) )
        {
            Stencil = operands[counter++];
        }

        if( strstr( mode, "<LRTW>" ) )
        {
            cntrls.isLastWrite = true;

        }

        if( strstr( mode, "<PS>" ) )
        {
            cntrls.isPerSample = true;
        }

        if( strstr( mode, "CM" ) )
        {
            cntrls.isCoarseMode = true;
        }
    }
    else
    {
        R = operands[counter++];
        G = operands[counter++];
        B = operands[counter++];
        A = operands[counter++];
    }


    VISA_SurfaceVar *surfaceVar = (VISA_SurfaceVar*)m_kernel->getDeclFromName(surface_name);
    MUST_BE_TRUE1(surfaceVar != NULL, line_no, "Surface was not found");

    VISA_StateOpndHandle * surface = NULL;
    m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar);

    uint8_t numMsgSpecificOpnd = 0;
    VISA_RawOpnd* rawOpnds[20];

#define APPEND_NON_NULL_RAW_OPND( opnd ) \
    if( opnd != NULL )  \
    { \
    rawOpnds[numMsgSpecificOpnd++] = (VISA_RawOpnd*)opnd; \
    }

    APPEND_NON_NULL_RAW_OPND( s0a );
    APPEND_NON_NULL_RAW_OPND( oM );
    APPEND_NON_NULL_RAW_OPND( R );
    APPEND_NON_NULL_RAW_OPND( G );
    APPEND_NON_NULL_RAW_OPND( B );
    APPEND_NON_NULL_RAW_OPND( A );
    APPEND_NON_NULL_RAW_OPND( Z );
    APPEND_NON_NULL_RAW_OPND( Stencil );
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    m_kernel->AppendVISA3dRTWriteCPS(
        (VISA_PredOpnd*)pred, emask, executionSize, (VISA_VectorOpnd*)rti,
        cntrls, surface, (VISA_RawOpnd*)r1Header, (VISA_VectorOpnd*)SamplerIndex,
        (VISA_VectorOpnd*)CPSCounter, numMsgSpecificOpnd, rawOpnds);

    return true;
}


bool CISA_IR_Builder::CISA_create_info_3d_instruction(
    VISASampler3DSubOpCode subOpcode,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    ChannelMask channel,
    const char* surface_name,
    VISA_opnd* lod,
    VISA_opnd* dst,
    int line_no)
{
    VISA_SurfaceVar* surfaceVar = (VISA_SurfaceVar*)m_kernel->getDeclFromName(surface_name);
    MUST_BE_TRUE1(surfaceVar != NULL, line_no, "Surface was not found");

    VISA_StateOpndHandle* surface = NULL;
    m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar);
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    m_kernel->AppendVISA3dInfo(
        subOpcode, emask, executionSize, channel.getAPI(), surface, (VISA_RawOpnd*)lod, (VISA_RawOpnd*)dst);
    return true;
}

bool CISA_IR_Builder::createSample4Instruction(VISA_opnd* pred,
    VISASampler3DSubOpCode subOpcode,
    bool pixelNullMask,
    ChannelMask channel,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd* aoffimmi,
    const char* sampler_name,
    const char* surface_name,
    VISA_opnd* dst,
    unsigned int numParameters,
    VISA_RawOpnd** params,
    int line_no)
{
    VISA_SurfaceVar *surfaceVar = (VISA_SurfaceVar*)m_kernel->getDeclFromName(surface_name);
    MUST_BE_TRUE1(surfaceVar != NULL, line_no, "Surface was not found");

    VISA_StateOpndHandle *surface = NULL;

    m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar);

    VISA_SamplerVar *samplerVar = (VISA_SamplerVar*)m_kernel->getDeclFromName(sampler_name);
    MUST_BE_TRUE1(samplerVar != NULL, line_no, "Sampler was not found");

    VISA_StateOpndHandle * sampler = NULL;
    m_kernel->CreateVISAStateOperandHandle(sampler, samplerVar);

    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);

    MUST_BE_TRUE(channel.getNumEnabledChannels() == 1, "Only one of R,G,B,A may be specified for sample4 instruction");
    m_kernel->AppendVISA3dGather4(
        subOpcode, pixelNullMask, (VISA_PredOpnd*)pred, emask,
        executionSize, channel.getSingleChannel(), (VISA_VectorOpnd*) aoffimmi,
        sampler, surface, (VISA_RawOpnd*) dst, numParameters, params);
    return true;
}


bool CISA_IR_Builder::create3DLoadInstruction(
    VISA_opnd* pred,
    VISASampler3DSubOpCode subOpcode,
    bool pixelNullMask,
    ChannelMask channels,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd *aoffimmi,
    const char* surface_name,
    VISA_opnd* dst,
    unsigned int numParameters,
    VISA_RawOpnd** params,
    int line_no)
{

    VISA_SurfaceVar *surfaceVar = (VISA_SurfaceVar*)m_kernel->getDeclFromName(surface_name);
    MUST_BE_TRUE1(surfaceVar != NULL, line_no, "Surface was not found");

    VISA_StateOpndHandle * surface = NULL;
    m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar);

    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    m_kernel->AppendVISA3dLoad(
        subOpcode, pixelNullMask, (VISA_PredOpnd*)pred, emask,
        executionSize, channels.getAPI(), (VISA_VectorOpnd*) aoffimmi,
        surface, (VISA_RawOpnd*)dst, numParameters, params);
    return true;
}

bool CISA_IR_Builder::create3DSampleInstruction(
    VISA_opnd* pred,
    VISASampler3DSubOpCode subOpcode,
    bool pixelNullMask,
    bool cpsEnable,
    bool uniformSampler,
    ChannelMask channels,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd* aoffimmi,
    const char* sampler_name,
    const char* surface_name,
    VISA_opnd* dst,
    unsigned int numParameters,
    VISA_RawOpnd** params,
    int line_no)
{
    VISA_SurfaceVar *surfaceVar = (VISA_SurfaceVar*)m_kernel->getDeclFromName(surface_name);
    MUST_BE_TRUE1(surfaceVar != NULL, line_no, "Surface was not found");

    VISA_StateOpndHandle * surface = NULL;
    m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar);

    VISA_SamplerVar *samplerVar = (VISA_SamplerVar*)m_kernel->getDeclFromName(sampler_name);
    MUST_BE_TRUE1(samplerVar != NULL, line_no, "Sampler was not found");

    VISA_StateOpndHandle * sampler = NULL;
    m_kernel->CreateVISAStateOperandHandle(sampler, samplerVar);

    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    m_kernel->AppendVISA3dSampler(
        subOpcode, pixelNullMask, cpsEnable, uniformSampler, (VISA_PredOpnd*)pred, emask,
        executionSize, channels.getAPI(), (VISA_VectorOpnd*) aoffimmi,
        sampler, surface, (VISA_RawOpnd*)dst, numParameters, params);
    return true;

}

bool CISA_IR_Builder::CISA_create_sample_instruction(
    ISA_Opcode opcode,
    ChannelMask channel,
    int simd_mode,
    const char* sampler_name,
    const char* surface_name,
    VISA_opnd *u_opnd,
    VISA_opnd *v_opnd,
    VISA_opnd *r_opnd,
    VISA_opnd *dst,
    int line_no)
{
    VISA_SurfaceVar* surfaceVar = (VISA_SurfaceVar*)m_kernel->getDeclFromName(surface_name);
    MUST_BE_TRUE1(surfaceVar != NULL, line_no, "Surface was not found");

    VISA_StateOpndHandle* surface = NULL;

    m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar);

    int status = VISA_SUCCESS;

    if (opcode == ISA_SAMPLE)
    {
        VISA_SamplerVar* samplerVar = (VISA_SamplerVar*) m_kernel->getDeclFromName(sampler_name);
        MUST_BE_TRUE1(samplerVar != NULL, line_no, "Sampler was not found");

        VISA_StateOpndHandle* sampler = NULL;
        m_kernel->CreateVISAStateOperandHandle(sampler, samplerVar);

        bool isSimd16 = ((simd_mode == 16) ? true : false);

        status = m_kernel->AppendVISASISample(
            vISA_EMASK_M1, surface, sampler, channel.getAPI(), isSimd16,
            (VISA_RawOpnd*)u_opnd, (VISA_RawOpnd*)v_opnd, (VISA_RawOpnd*)r_opnd, (VISA_RawOpnd*)dst);

    } else if (opcode == ISA_LOAD) {
        bool isSimd16 = ((simd_mode == 16) ? true : false);
        status = m_kernel->AppendVISASILoad(
            surface, channel.getAPI(), isSimd16, (VISA_RawOpnd*)u_opnd, (VISA_RawOpnd*)v_opnd,
            (VISA_RawOpnd*)r_opnd, (VISA_RawOpnd*)dst);
    } else {
        MUST_BE_TRUE1(false, line_no, "Sampler Opcode not supported.");
        return false;
    }

    MUST_BE_TRUE1(status == VISA_SUCCESS, line_no, "Failed to create SAMPLE or LOAD instruction.");
    return true;
}

bool CISA_IR_Builder::CISA_create_sampleunorm_instruction(
    ISA_Opcode opcode,
    ChannelMask channel,
    CHANNEL_OUTPUT_FORMAT out,
    const char* sampler_name,
    const char* surface_name,
    VISA_opnd *src0,
    VISA_opnd *src1,
    VISA_opnd *src2,
    VISA_opnd *src3,
    VISA_opnd *dst,
    int line_no)
{
    VISA_SurfaceVar *surfaceVar = (VISA_SurfaceVar*)m_kernel->getDeclFromName(surface_name);
    MUST_BE_TRUE1(surfaceVar != NULL, line_no, "Surface was not found");

    VISA_StateOpndHandle * surface = NULL;
    m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar);

    VISA_SamplerVar *samplerVar = (VISA_SamplerVar *)m_kernel->getDeclFromName(sampler_name);
    MUST_BE_TRUE1(samplerVar != NULL, line_no, "Sampler was not found");

    VISA_StateOpndHandle *sampler = NULL;
    m_kernel->CreateVISAStateOperandHandle(sampler, samplerVar);
    m_kernel->AppendVISASISampleUnorm(
        surface, sampler, channel.getAPI(),
        (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1, (VISA_VectorOpnd *)src2,
        (VISA_VectorOpnd *)src3, (VISA_RawOpnd *)dst, out);
    return true;
}

bool CISA_IR_Builder::CISA_create_vme_ime_instruction(
    ISA_Opcode opcode,
    unsigned char stream_mode,
    unsigned char searchCtrl,
    VISA_opnd *input_opnd,
    VISA_opnd *ime_input_opnd,
    const char* surface_name,
    VISA_opnd *ref0_opnd,
    VISA_opnd *ref1_opnd,
    VISA_opnd *costCenter_opnd,
    VISA_opnd *dst_opnd,
    int line_no)
{
    VISA_SurfaceVar *surfaceVar = (VISA_SurfaceVar*)m_kernel->getDeclFromName(surface_name);
    MUST_BE_TRUE1(surfaceVar != NULL, line_no, "Surface was not found");

    VISA_StateOpndHandle * surface = NULL;
    m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar);
    m_kernel->AppendVISAMiscVME_IME(
        surface, stream_mode, searchCtrl, (VISA_RawOpnd *)input_opnd,
        (VISA_RawOpnd *)ime_input_opnd, (VISA_RawOpnd *)ref0_opnd,
        (VISA_RawOpnd *)ref1_opnd, (VISA_RawOpnd *)costCenter_opnd,
        (VISA_RawOpnd *)dst_opnd);

    return true;
}

bool CISA_IR_Builder::CISA_create_vme_sic_instruction(
    ISA_Opcode opcode,
    VISA_opnd *input_opnd,
    VISA_opnd *sic_input_opnd,
    const char* surface_name,
    VISA_opnd *dst,
    int line_no)
{
    VISA_SurfaceVar *surfaceVar = (VISA_SurfaceVar*)m_kernel->getDeclFromName(surface_name);
    MUST_BE_TRUE1(surfaceVar != NULL, line_no, "Surface was not found");

    VISA_StateOpndHandle * surface = NULL;
    m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar);
    m_kernel->AppendVISAMiscVME_SIC(
        surface, (VISA_RawOpnd *)input_opnd, (VISA_RawOpnd *)sic_input_opnd, (VISA_RawOpnd *)dst);
    return true;
}

bool CISA_IR_Builder::CISA_create_vme_fbr_instruction(
    ISA_Opcode opcode,
    VISA_opnd *input_opnd,
    VISA_opnd *fbr_input_opnd,
    const char* surface_name,
    VISA_opnd* fbrMbMode,
    VISA_opnd* fbrSubMbShape,
    VISA_opnd* fbrSubPredMode,
    VISA_opnd *dst,
    int line_no)
{
    VISA_SurfaceVar *surfaceVar = (VISA_SurfaceVar*)m_kernel->getDeclFromName(surface_name);
    MUST_BE_TRUE1(surfaceVar != NULL, line_no, "Surface was not found");

    VISA_StateOpndHandle * surface = NULL;
    m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar);
    m_kernel->AppendVISAMiscVME_FBR(surface,
        (VISA_RawOpnd *)input_opnd, (VISA_RawOpnd *)fbr_input_opnd,
        (VISA_VectorOpnd *)fbrMbMode, (VISA_VectorOpnd *)fbrSubMbShape,
        (VISA_VectorOpnd *)fbrSubPredMode, (VISA_RawOpnd *)dst);
    return true;
}

bool CISA_IR_Builder::CISA_create_NO_OPND_instruction(ISA_Opcode opcode)
{
    m_kernel->AppendVISASyncInst(opcode);
    return true;
}

bool CISA_IR_Builder::CISA_create_switch_instruction(
    ISA_Opcode opcode,
    unsigned exec_size,
    VISA_opnd *indexOpnd,
    int numLabels,
    char ** labels,
    int line_no)
{
    VISA_INST_Desc *inst_desc = &CISA_INST_table[opcode];
    VISA_opnd *opnd[35];
    int num_pred_desc_operands = 1;
    int num_operands = 0;

    opnd[num_operands] = (VISA_opnd * )m_mem.alloc(sizeof(VISA_opnd));
    opnd[num_operands]->_opnd.other_opnd = numLabels; //real ID will be set during kernel finalization
    opnd[num_operands]->opnd_type = CISA_OPND_OTHER;
    opnd[num_operands]->size = (unsigned short) Get_VISA_Type_Size((VISA_Type)inst_desc->opnd_desc[num_pred_desc_operands].data_type);
    opnd[num_operands]->tag = (unsigned char) inst_desc->opnd_desc[num_pred_desc_operands].opnd_type;
    num_operands++;

    //global offset
    if(indexOpnd != NULL)
    {
        opnd[num_operands] = indexOpnd;
        num_operands ++;
    }

    for(int i = numLabels - 1; i >= 0; i--)
    {
        //TODO: FIX
        //m_kernel->string_pool_lookup_and_insert_branch_targets(labels[i], LABEL_VAR, ISA_TYPE_UW); //Will be checked after whole analysis of the text
        opnd[num_operands] = m_kernel->CreateOtherOpndHelper(
            num_pred_desc_operands, 2, inst_desc,
            m_kernel->getIndexFromLabelName(std::string(labels[i])));
        num_operands++;
    }

    m_kernel->AppendVISACFSwitchJMPInst((VISA_VectorOpnd *)indexOpnd, (unsigned char)numLabels, (VISA_LabelOpnd **)opnd);

    for(int i = 0; i< numLabels; i++)
    {
        VISA_opnd * temp_opnd = opnd[i];
        m_kernel->addPendingLabels(temp_opnd);
    }
    for(int i = numLabels - 1; i >= 0; i--)
    {
        m_kernel->addPendingLabelNames(std::string(labels[i]));
    }

    return true;
}

bool CISA_IR_Builder::CISA_create_fcall_instruction(VISA_opnd *pred_opnd,
                                                    ISA_Opcode opcode,
                                                    VISA_EMask_Ctrl emask,
                                                    unsigned exec_size,
                                                    const char* funcName,
                                                    unsigned arg_size,
                                                    unsigned return_size,
                                                    int line_no) //last index
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    m_kernel->AppendVISACFFunctionCallInst(
        (VISA_PredOpnd *)pred_opnd,emask, executionSize, std::string(funcName),
        (unsigned char)arg_size, (unsigned char)return_size);
    return true;
}

bool CISA_IR_Builder::CISA_create_ifcall_instruction(VISA_opnd *pred_opnd,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd* funcAddr,
    unsigned arg_size,
    unsigned return_size,
    int line_no) //last index
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    m_kernel->AppendVISACFIndirectFuncCallInst((VISA_PredOpnd *)pred_opnd, emask, executionSize,
        (VISA_VectorOpnd*) funcAddr, (uint8_t) arg_size, (uint8_t) return_size);
    return true;
}

bool CISA_IR_Builder::CISA_create_faddr_instruction(
    const char* sym_name, VISA_opnd* dst, int line_no)
{
    m_kernel->AppendVISACFSymbolInst(std::string(sym_name), (VISA_VectorOpnd*) dst);
    return true;
}

bool CISA_IR_Builder::CISA_create_raw_send_instruction(
    ISA_Opcode opcode,
    unsigned char modifier,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd *pred_opnd,
    unsigned int exMsgDesc,
    unsigned char srcSize,
    unsigned char dstSize,
    VISA_opnd *Desc,
    VISA_opnd *Src,
    VISA_opnd *Dst,
    int line_no)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    m_kernel->AppendVISAMiscRawSend(
        (VISA_PredOpnd *) pred_opnd, emask, executionSize, modifier, exMsgDesc, srcSize, dstSize,
        (VISA_VectorOpnd *)Desc, (VISA_RawOpnd *)Src, (VISA_RawOpnd *)Dst);
    return true;
}

bool CISA_IR_Builder::CISA_create_lifetime_inst(
    unsigned char startOrEnd, const char *src, int line_no)
{
    // src is a string representation of variable.
    // Scan entire symbol table to find variable whose name
    // corresponds to src.
    CISA_GEN_VAR *cisaVar = m_kernel->getDeclFromName(src);
    MUST_BE_TRUE1(cisaVar != NULL, line_no, "variable for lifetime not found");

    VISA_opnd *var = NULL;
    if(cisaVar->type == GENERAL_VAR)
    {
        var = CISA_create_gen_src_operand(src, 0, 1, 0, 0, 0, MODIFIER_NONE, line_no);
    }
    else if(cisaVar->type == ADDRESS_VAR)
    {
        var = CISA_set_address_operand(cisaVar, 0, 1, (startOrEnd == 0));
    }
    else if(cisaVar->type == PREDICATE_VAR)
    {
        char cntrl[4] = {0, 0, 0, 0};
        var = CISA_create_predicate_operand(src, MODIFIER_NONE, PredState_NO_INVERSE, cntrl, line_no);
    }
    m_kernel->AppendVISALifetime((VISAVarLifetime)startOrEnd, (VISA_VectorOpnd*)var);

    return true;
}

bool CISA_IR_Builder::CISA_create_raw_sends_instruction(
    ISA_Opcode opcode,
    unsigned char modifier,
    bool hasEOT,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd *pred_opnd,
    VISA_opnd *exMsgDesc,
    unsigned char ffid,
    unsigned char src0Size,
    unsigned char src1Size,
    unsigned char dstSize,
    VISA_opnd *Desc,
    VISA_opnd *Src0,
    VISA_opnd *Src1,
    VISA_opnd *Dst,
    int line_no)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    m_kernel->AppendVISAMiscRawSends(
        (VISA_PredOpnd *) pred_opnd, emask, executionSize, modifier, ffid,
        (VISA_VectorOpnd *)exMsgDesc, src0Size, src1Size, dstSize,
        (VISA_VectorOpnd *)Desc, (VISA_RawOpnd *)Src0, (VISA_RawOpnd *)Src1,
        (VISA_RawOpnd *)Dst, hasEOT);
    return true;
}
/*
Should be only called from CISA 2.4+
*/
bool CISA_IR_Builder::CISA_create_fence_instruction(ISA_Opcode opcode, unsigned char mode)
{
     m_kernel->AppendVISASyncInst(opcode, mode);
    return true;
}

bool CISA_IR_Builder::CISA_create_wait_instruction(VISA_opnd* mask)
{
    m_kernel->AppendVISAWaitInst((VISA_VectorOpnd*) mask);
    return true;
}


/*** CISA 3.0 and later ***/
bool CISA_IR_Builder::CISA_create_yield_instruction(ISA_Opcode opcode)
{
    m_kernel->AppendVISASyncInst(opcode);
    return true;
}

VISA_opnd * CISA_IR_Builder::CISA_create_gen_src_operand(
    const char* var_name, short v_stride, short width, short h_stride,
    unsigned char row_offset, unsigned char col_offset, VISA_Modifier mod, int line_no)
{
    VISA_VectorOpnd *cisa_opnd = NULL;
    int status = VISA_SUCCESS;
    auto *decl =  (VISA_GenVar*)m_kernel->getDeclFromName(var_name);
    if (!decl) {
        RecordParseError(line_no, var_name, ": unbound identifier");
        return nullptr;
    }
    status = m_kernel->CreateVISASrcOperand(cisa_opnd, decl, mod, v_stride, width, h_stride, row_offset, col_offset);
    MUST_BE_TRUE1(status == VISA_SUCCESS, line_no, "Failed to create cisa src operand." );
    return (VISA_opnd *)cisa_opnd;
}

VISA_opnd * CISA_IR_Builder::CISA_dst_general_operand(
    const char * var_name,
    unsigned char roff, unsigned char sroff,
    unsigned short hstride, int line_no)
{
    VISA_VectorOpnd *cisa_opnd = NULL;
    int status = VISA_SUCCESS;
    auto *decl = (VISA_GenVar *)m_kernel->getDeclFromName(var_name);
    if (!decl) {
        RecordParseError(line_no, var_name, ": unbound identifier");
        return nullptr;
    }
    status = m_kernel->CreateVISADstOperand(cisa_opnd, decl, hstride, roff, sroff);
    MUST_BE_TRUE1(status == VISA_SUCCESS, line_no, "Failed to create cisa dst operand.");
    return (VISA_opnd *)cisa_opnd;
}

VISA_opnd * CISA_IR_Builder::CISA_create_immed(uint64_t value, VISA_Type type, int line_no)
{
    VISA_VectorOpnd *cisa_opnd = NULL;
    int status = VISA_SUCCESS;
    status =  m_kernel->CreateVISAImmediate(cisa_opnd, &value, type);
    MUST_BE_TRUE1(status == VISA_SUCCESS,line_no,"Could not create immediate.");
    if (type == ISA_TYPE_Q || type == ISA_TYPE_UQ)
    {
        cisa_opnd->_opnd.v_opnd.opnd_val.const_opnd._val.lval = value;
    }
    else
    {
        cisa_opnd->_opnd.v_opnd.opnd_val.const_opnd._val.ival = (uint32_t)value;
    }
    return (VISA_opnd *)cisa_opnd;
}

VISA_opnd * CISA_IR_Builder::CISA_create_float_immed(double value, VISA_Type type, int line_no)
{
    VISA_VectorOpnd *cisa_opnd = NULL;
    if (type == ISA_TYPE_F)
    {
        float temp = (float)value;
        m_kernel->CreateVISAImmediate(cisa_opnd, &temp, type);
    }
    else
    {
        m_kernel->CreateVISAImmediate(cisa_opnd, &value, type);
    }

    return (VISA_opnd *)cisa_opnd;
}

CISA_GEN_VAR * CISA_IR_Builder::CISA_find_decl(const char *var_name)
{
    return m_kernel->getDeclFromName(var_name);
}

VISA_opnd * CISA_IR_Builder::CISA_set_address_operand(
    CISA_GEN_VAR * cisa_decl, unsigned char offset, short width, bool isDst)
{
    /*
    cisa_opnd->opnd_type = CISA_OPND_VECTOR;
    cisa_opnd->tag = OPERAND_ADDRESS;
    cisa_opnd->_opnd.v_opnd.tag = OPERAND_ADDRESS;
    cisa_opnd->_opnd.v_opnd.opnd_val.addr_opnd.index= cisa_opnd->index;
    cisa_opnd->_opnd.v_opnd.opnd_val.addr_opnd.offset= offset;
    cisa_opnd->_opnd.v_opnd.opnd_val.addr_opnd.width= Get_VISA_Exec_Size_From_Raw_Size(width & 0xF);
    cisa_opnd->size = Get_Size_Vector_Operand(&cisa_opnd->_opnd.v_opnd);
    */
    VISA_VectorOpnd *cisa_opnd = NULL;
    int status = VISA_SUCCESS;
    status = m_kernel->CreateVISAAddressOperand(cisa_opnd, (VISA_AddrVar *)cisa_decl, offset, width, isDst);
    if( status != VISA_SUCCESS )
    {
        assert( 0 );
        return NULL;
    }

    return (VISA_opnd *)cisa_opnd;
}

VISA_opnd * CISA_IR_Builder::CISA_set_address_expression(
    CISA_GEN_VAR *cisa_decl, short offset)
{
    VISA_VectorOpnd *cisa_opnd = NULL;
    m_kernel->CreateVISAAddressOfOperand(cisa_opnd, (VISA_GenVar *)cisa_decl, offset);
    return (VISA_opnd *)cisa_opnd;
}

VISA_opnd * CISA_IR_Builder::CISA_create_indirect(
    CISA_GEN_VAR * cisa_decl,VISA_Modifier mod, unsigned short row_offset,
    unsigned char col_offset, unsigned short immedOffset,
    unsigned short vertical_stride, unsigned short width,
    unsigned short horizontal_stride, VISA_Type type)
{
    /*
    cisa_opnd->opnd_type = CISA_OPND_VECTOR;
    cisa_opnd->tag = OPERAND_INDIRECT;
    cisa_opnd->_opnd.v_opnd.tag = OPERAND_INDIRECT;
    cisa_opnd->_opnd.v_opnd.opnd_val.indirect_opnd.index = cisa_decl->index;
    cisa_opnd->_opnd.v_opnd.opnd_val.indirect_opnd.addr_offset = col_offset;
    cisa_opnd->_opnd.v_opnd.opnd_val.indirect_opnd.indirect_offset = immedOffset;
    cisa_opnd->_opnd.v_opnd.opnd_val.indirect_opnd.bit_property = type;
    cisa_opnd->_opnd.v_opnd.opnd_val.indirect_opnd.region = Create_CISA_Region(vertical_stride,width,horizontal_stride);//Get_CISA_Region_Val(horizontal_stride) <<8;

    cisa_opnd->_opnd.v_opnd.tag += mod<<3;

    cisa_opnd->size = Get_Size_Vector_Operand(&cisa_opnd->_opnd.v_opnd);
    */
    VISA_VectorOpnd *cisa_opnd = NULL;
    m_kernel->CreateVISAIndirectSrcOperand(
        cisa_opnd, (VISA_AddrVar*)cisa_decl, mod, col_offset,
        immedOffset, vertical_stride, width, horizontal_stride, type);
    return cisa_opnd;
}

VISA_opnd * CISA_IR_Builder::CISA_create_indirect_dst(
    CISA_GEN_VAR * cisa_decl,VISA_Modifier mod, unsigned short row_offset,
    unsigned char col_offset, unsigned short immedOffset,
    unsigned short horizontal_stride, VISA_Type type)
{
    VISA_VectorOpnd *cisa_opnd = NULL;
    m_kernel->CreateVISAIndirectDstOperand(
        cisa_opnd, (VISA_AddrVar*)cisa_decl, col_offset, immedOffset, horizontal_stride, type);
    return (VISA_opnd *)cisa_opnd;
}

VISA_opnd * CISA_IR_Builder::CISA_create_state_operand(
    const char * var_name, unsigned char offset, int line_no, bool isDst)
{

    CISA_GEN_VAR *decl = m_kernel->getDeclFromName(std::string(var_name));

    MUST_BE_TRUE1(decl != NULL, line_no, "Could not find the Declaration.");

    VISA_VectorOpnd * cisa_opnd = NULL;

    int status = VISA_SUCCESS;
    switch(decl->type)
    {
    case SURFACE_VAR:
        {
            status = m_kernel->CreateVISAStateOperand(cisa_opnd, (VISA_SurfaceVar *)decl, offset, isDst);
            break;
        }
    case SAMPLER_VAR:
        {
            status = m_kernel->CreateVISAStateOperand(cisa_opnd, (VISA_SamplerVar *)decl, offset, isDst);
            break;
        }
    default:
        {
            MUST_BE_TRUE1(false, line_no, "Incorrect declaration type.");
        }
    }

    MUST_BE_TRUE1(status == VISA_SUCCESS, line_no, "Was not able to create State Operand.");
    return (VISA_opnd *)cisa_opnd;
}

VISA_opnd * CISA_IR_Builder::CISA_create_predicate_operand(
    const char * var_name, VISA_Modifier mod, VISA_PREDICATE_STATE state,
    const char * cntrl, int line_no)
{

    VISA_PREDICATE_CONTROL control = PRED_CTRL_NON;
    if(cntrl[0] == 'a' && cntrl[1] == 'n' && cntrl[2] == 'y')
    {
        control = PRED_CTRL_ANY;
    }
    else if(cntrl[0] == 'a' && cntrl[1] == 'l' && cntrl[2] == 'l')
    {
        control = PRED_CTRL_ALL;
    }
    VISA_PredOpnd *cisa_opnd = NULL;
    CISA_GEN_VAR * decl = m_kernel->getDeclFromName(std::string(var_name));
    int status = VISA_SUCCESS;
    m_kernel->CreateVISAPredicateOperand(cisa_opnd, (VISA_PredVar *)decl, state, control);
    MUST_BE_TRUE1((status == VISA_SUCCESS), line_no, "Failed to create predicate operand.");
    return (VISA_opnd *)cisa_opnd;
}

VISA_opnd * CISA_IR_Builder::CISA_create_RAW_NULL_operand(int line_no)
{
    /*
    VISA_opnd * cisa_opnd = (VISA_opnd *) m_mem.alloc(sizeof(VISA_opnd));
    cisa_opnd->opnd_type = CISA_OPND_RAW;
    cisa_opnd->tag = NUM_OPERAND_CLASS;
    cisa_opnd->_opnd.r_opnd.index = 0;
    cisa_opnd->index = 0;
    cisa_opnd->_opnd.r_opnd.offset = 0;
    cisa_opnd->size = sizeof(cisa_opnd->_opnd.r_opnd.index) + sizeof(cisa_opnd->_opnd.r_opnd.offset);
    */

    VISA_RawOpnd *cisa_opnd = NULL;
    int status = VISA_SUCCESS;
    status = m_kernel->CreateVISANullRawOperand(cisa_opnd, true);
    MUST_BE_TRUE1(status == VISA_SUCCESS, line_no, "Was not able to create NULL RAW operand.");
    return (VISA_opnd *)cisa_opnd;

}

VISA_opnd * CISA_IR_Builder::CISA_create_RAW_operand(
    const char * var_name, unsigned short offset, int line_no)
{
    VISA_RawOpnd *cisa_opnd = NULL;
    auto *decl = (VISA_GenVar *)m_kernel->getDeclFromName(var_name);
    MUST_BE_TRUE(decl,"undeclared virtual register");
    int status = m_kernel->CreateVISARawOperand(cisa_opnd, decl, offset);
    MUST_BE_TRUE1(status == VISA_SUCCESS, line_no, "Was not able to create RAW operand.");
    return (VISA_opnd *)cisa_opnd; //delay the decision of src or dst until translate stage
}

void CISA_IR_Builder::CISA_push_decl_scope()
{
    m_kernel->pushIndexMapScopeLevel();
}
void CISA_IR_Builder::CISA_pop_decl_scope()
{
    m_kernel->popIndexMapScopeLevel();
}

unsigned short CISA_IR_Builder::get_hash_key(const char* str)
{
    const char *str_pt = str;
    unsigned short key=0;
    unsigned char c;
    while ((c = *str_pt++) != '\0') key = (key+c)<<1;

    return key % HASH_TABLE_SIZE;
}
string_pool_entry** CISA_IR_Builder::new_string_pool()
{
    string_pool_entry ** sp =
        (string_pool_entry**)m_mem.alloc(sizeof(string_pool_entry *) * HASH_TABLE_SIZE);
    memset(sp, 0, sizeof(string_pool_entry *) * HASH_TABLE_SIZE);

    return sp;
}

string_pool_entry * CISA_IR_Builder::string_pool_lookup(
    string_pool_entry **spool, const char *str)
{
    unsigned short key = 0;
    string_pool_entry* entry;
    char *s;

    key = get_hash_key(str);

    for( entry = spool[key]; entry != NULL; entry = entry->next ){
        s = (char *)entry->value;
        if(!strcmp(s, str))
            return entry;
    }

    return NULL;
}

bool CISA_IR_Builder::string_pool_lookup_and_insert(
    string_pool_entry **spool,
    const char *str,
    Common_ISA_Var_Class type,
    VISA_Type data_type)
{
    unsigned short key = 0;
    string_pool_entry* entry;
    char *s;
    int len = (int) strlen(str);

    key = get_hash_key(str);

    for( entry = spool[key]; entry != NULL; entry = entry->next ){
        s = (char *)entry->value;
        if(!strcmp(s, str))
            return false;
    }

    s = (char*)m_mem.alloc(len + 1);
    memcpy_s(s, len + 1, str, len+1);
    s[len] = '\0';

    entry = (string_pool_entry*)m_mem.alloc(sizeof(string_pool_entry));
    memset(entry, 0, sizeof(*entry));
    entry->value = s;
    entry->type = type;
    entry->data_type = data_type;

    entry->next = spool[key];
    spool[key] = entry;

    return true;
}

Common_ISA_Input_Class CISA_IR_Builder::get_input_class(Common_ISA_Var_Class var_class)
{
    if (var_class == GENERAL_VAR)
        return INPUT_GENERAL;

    if (var_class == SAMPLER_VAR)
        return INPUT_SAMPLER;

    if (var_class == SURFACE_VAR)
        return INPUT_SURFACE;

    return INPUT_UNKNOWN;
}
void CISA_IR_Builder::CISA_post_file_parse()
{
    return;
}

// place it here so that internal Gen_IR files don't have to include VISAKernel.h
std::stringstream& IR_Builder::criticalMsgStream()
{
    return const_cast<CISA_IR_Builder*>(parentBuilder)->criticalMsgStream();
}

