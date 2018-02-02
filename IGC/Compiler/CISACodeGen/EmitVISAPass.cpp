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
#include "EmitVISAPass.hpp"
#include "CISABuilder.hpp"
#include "VertexShaderCodeGen.hpp"
#include "GeometryShaderCodeGen.hpp"
#include "PixelShaderCodeGen.hpp"
#include "OpenCLKernelCodeGen.hpp"
#include "ComputeShaderCodeGen.hpp"
#include "HullShaderCodeGen.hpp"
#include "DomainShaderCodeGen.hpp"
#include "DeSSA.hpp"
#include "messageEncoding.hpp"
#include "PayloadMapping.hpp"
#include "VectorProcess.hpp"
#include "ShaderCodeGen.hpp"
#include "common/allocator.h"
#include "common/debug/Dump.hpp"
#include "common/igc_regkeys.hpp"
#include "common/Stats.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "common/secure_mem.h"
#include "iStdLib/File.h"
#include "Compiler/DebugInfo/VISAIDebugEmitter.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;
using namespace std;

char EmitPass::ID = 0;

EmitPass::EmitPass(CShaderProgram::KernelShaderMap &shaders, SIMDMode mode, bool canAbortOnSpill, ShaderDispatchMode shaderMode, PSSignature* pSignature)
    : FunctionPass(ID),
      m_SimdMode(mode),
      m_ShaderMode(shaderMode),
      m_shaders(shaders),
      m_currShader(nullptr),
      m_encoder(nullptr),
      m_canAbortOnSpill(canAbortOnSpill),
      m_roundingMode(CEncoder::RoundingMode::RoundToNearestEven),
      m_pSignature(pSignature)
{
    //Before calling getAnalysisUsage() for EmitPass, the passes that it depends on need to be initialized
    initializeDominatorTreeWrapperPassPass( *PassRegistry::getPassRegistry() );
    initializeWIAnalysisPass( *PassRegistry::getPassRegistry() );
    initializeCodeGenPatternMatchPass( *PassRegistry::getPassRegistry() );
    initializeDeSSAPass( *PassRegistry::getPassRegistry() );
    initializeBlockCoalescingPass( *PassRegistry::getPassRegistry() );
    initializeCoalescingEnginePass( *PassRegistry::getPassRegistry() );
    initializeMetaDataUtilsWrapperPass( *PassRegistry::getPassRegistry() );
    initializeSimd32ProfitabilityAnalysisPass( *PassRegistry::getPassRegistry() );
    initializeVariableReuseAnalysisPass(*PassRegistry::getPassRegistry());
    initializeLiveVariablesPass(*PassRegistry::getPassRegistry());
}

EmitPass::~EmitPass()
{
}

static bool DefReachUseWithinLevel(llvm::Value *def, const llvm::Instruction *use, uint level)
{
    if (level == 0 || !def || !use)
        return false;
    for (auto useIter = def->user_begin(), E = def->user_end(); useIter != E; ++useIter)
    {
        llvm::Instruction *useInst = dyn_cast<llvm::Instruction>(*useIter);
        if (useInst)
        {
            if (useInst == use)
                return true;
            else
            {
                if (DefReachUseWithinLevel(useInst, use, level - 1))
                    return true;
            }
        }
    }
    return false;
}

uint EmitPass::DecideInstanceAndSlice(llvm::BasicBlock &blk, SDAG &sdag, bool &slicing)
{
    m_encoder->SetSubSpanDestination(false);
    uint numInstance = m_currShader->m_numberInstance;
    slicing = (m_SimdMode == SIMDMode::SIMD32);  // set to false if we don't want slicing
    if (sdag.m_root->getType()->getTypeID() != llvm::Type::VoidTyID)
    {
        m_destination = GetSymbol(sdag.m_root);
        if (m_pattern->IsSubspanUse(sdag.m_root))
        {
            m_encoder->SetSubSpanDestination(true);
        }
        numInstance = m_destination->GetNumberInstance();

        if (isa<CmpInst>(sdag.m_root))
        {
            if (DefReachUseWithinLevel(sdag.m_root, blk.getTerminator(), 4))
                slicing = false;
        }
        else if (IsUniformAtomic(sdag.m_root))
        {
            numInstance = 1;
            slicing = false;
        }
        else if (IsAtomicIntrinsic(GetOpCode(sdag.m_root)))
        {
            slicing = false;
        }
        else if (IsMediaIOIntrinsic(sdag.m_root))
        {
            numInstance = 1;
            slicing = false;
        }
        else if (IsSubGroupIntrinsicWithSimd32Implementation(GetOpCode(sdag.m_root)))
        {
            numInstance = 1;
            slicing = false;
        }
        else if (m_destination->IsUniform())
        {
            // if this uniform value is involved in phi-congruent class
            // live-interval changed with slicing. Therefore, we need to stop slicing
            // \todo: is it a good idea to pre-schedule all uniform operations to the beginning of the block?
            if (m_deSSA->getRootValue(sdag.m_root))
                slicing = false;
        }
    }
    else
    {
        if (StoreInst *ST = dyn_cast<StoreInst>(sdag.m_root))
        {
            // Limit to OpenCL so far as it has uniform load/store support.
            if (m_currShader->GetShaderType() == ShaderType::OPENCL_SHADER  &&
                isUniformStoreOCL(ST))
                numInstance = 1;
            slicing = false;
        }
        else if (isa<TerminatorInst>(sdag.m_root))
        {
            numInstance = 1;
            slicing = false;
        }
        else if (m_currShader->GetIsUniform(sdag.m_root))
        {
            numInstance = 1;
            // if this uniform value is involved in phi-congruent class
            // live-interval changed with slicing. Therefore, we need to stop slicing
            // \todo: is it a good idea to pre-schedule all uniform operations to the beginning of the block?
            if (m_deSSA->getRootValue(sdag.m_root))
                slicing = false;
        }
        else if (llvm::GenIntrinsicInst* pIntrinsic = llvm::dyn_cast<llvm::GenIntrinsicInst>(sdag.m_root))
        {
            GenISAIntrinsic::ID id = pIntrinsic->getIntrinsicID();
            if (id == GenISAIntrinsic::GenISA_threadgroupbarrier ||
                id == GenISAIntrinsic::GenISA_memoryfence ||
                id == GenISAIntrinsic::GenISA_flushsampler ||
                id == GenISAIntrinsic::GenISA_vaErode ||
                id == GenISAIntrinsic::GenISA_vaDilate ||
                id == GenISAIntrinsic::GenISA_vaMinMax ||
                id == GenISAIntrinsic::GenISA_vaMinMaxFilter ||
                id == GenISAIntrinsic::GenISA_vaConvolve ||
                id == GenISAIntrinsic::GenISA_vaConvolveGRF_16x1 ||
                id == GenISAIntrinsic::GenISA_vaConvolveGRF_16x4 ||
                id == GenISAIntrinsic::GenISA_vaCentroid ||
                id == GenISAIntrinsic::GenISA_vaBoolSum ||
                id == GenISAIntrinsic::GenISA_vaBoolCentroid ||
                id == GenISAIntrinsic::GenISA_MediaBlockWrite)
            {
                numInstance = 1;
                slicing = false;
            }
        }
    }

    return numInstance;
}

bool EmitPass::setCurrentShader(llvm::Function *F)
{
    llvm::Function *Kernel = F;
    if (m_FGA && m_FGA->getModule())
    {
        // subroutine enabled.
        auto FG = m_FGA->getGroup(F);
        if (!FG)
        {
            return false;
        }
        // Only simd8 will be tried when there are subroutines in this group.
        if (m_SimdMode != SIMDMode::SIMD8 && !FG->isSingle())
        {
            return false;
        }
        Kernel = FG->getHead();
    }
    else
    {
        // no analysis result avaliable.
        m_FGA = nullptr;
    }

    auto Iter = m_shaders.find(Kernel);
    if (Iter == m_shaders.end())
    {
        return false;
    }
    m_currShader = Iter->second->GetOrCreateShader(m_SimdMode, m_ShaderMode);
    m_encoder = &(m_currShader->GetEncoder());
    return true;
}

void EmitPass::CreateKernelShaderMap(CodeGenContext *ctx, MetaDataUtils *pMdUtils, llvm::Function &F)
{
    /* Moving CShaderProgram instantiation to EmitPass from codegen*/
    // Instantiate CShaderProgram and create map only if m_shaders is empty
    if (m_shaders.empty())
    {
        /* OpenCL shader */
        if (ctx->type == ShaderType::OPENCL_SHADER)
        {
            for (auto i = pMdUtils->begin_FunctionsInfo(), e = pMdUtils->end_FunctionsInfo(); i != e; ++i)
            {
                Function *pFunc = i->first;
                // Skip non-kernel functions.
                if (!isOCLKernelFunc(pMdUtils, pFunc))
                    continue;

                if (ctx->m_retryManager.kernelSet.empty() ||
                    ctx->m_retryManager.kernelSet.count(pFunc->getName().str()))
                {
                    m_shaders[pFunc] = new CShaderProgram(ctx, pFunc);
                    COMPILER_SHADER_STATS_INIT(m_shaders[pFunc]->m_shaderStats);
#if defined( _DEBUG ) || defined( _INTERNAL )
                    std::stringstream ss;
                    ss << "kernel_" << pFunc->getName().str();
                    static_cast<OpenCLProgramContext*>(ctx)->SetFuncStr(pFunc, ss.str());
#endif
                }
            }
        }
        /* Pixel Shader */
        else if (ctx->type == ShaderType::PIXEL_SHADER)
        {
            Function* coarsePhase = nullptr;
            Function* pixelPhase = nullptr;
            NamedMDNode* coarseNode = ctx->getModule()->getNamedMetadata("coarse_phase");
            NamedMDNode* pixelNode = ctx->getModule()->getNamedMetadata("pixel_phase");
            if (coarseNode)
            {
                coarsePhase = mdconst::dyn_extract<Function>(coarseNode->getOperand(0)->getOperand(0));
            }
            if (pixelNode)
            {
                pixelPhase = mdconst::dyn_extract<Function>(pixelNode->getOperand(0)->getOperand(0));
            }
            if (coarsePhase && pixelPhase)
            {
                //Multi stage PS
                CShaderProgram* pProgram = new CShaderProgram(ctx, &F);
                CPixelShader* pProgram8 =
                    static_cast<CPixelShader*>(pProgram->GetOrCreateShader(SIMDMode::SIMD8));
                CPixelShader* pProgram16 =
                    static_cast<CPixelShader*>(pProgram->GetOrCreateShader(SIMDMode::SIMD16));
                pProgram8->SetPSSignature(m_pSignature);
                pProgram16->SetPSSignature(m_pSignature);
                m_shaders[&F] = pProgram;
                COMPILER_SHADER_STATS_INIT(pProgram->m_shaderStats);
            }
            else
            {
                // Single PS
                // Assuming single shader information in metadata
                assert(pMdUtils->size_FunctionsInfo() == 1);
                Function *pFunc = pMdUtils->begin_FunctionsInfo()->first;

                CShaderProgram* pProgram = new CShaderProgram(ctx, pFunc);
                m_shaders[pFunc] = pProgram;
                COMPILER_SHADER_STATS_INIT(pProgram->m_shaderStats);

            }
        }
        /* All other shader types */
        else
        {
            for (auto i = pMdUtils->begin_FunctionsInfo(), e = pMdUtils->end_FunctionsInfo(); i != e; ++i)
            {
                Function *pFunc = i->first;
                // Skip non-kernel functions.
                if (!isKernelFunc(pMdUtils, pFunc))
                {
                    continue;
                }

                m_shaders[pFunc] = new CShaderProgram(ctx, pFunc);
                COMPILER_SHADER_STATS_INIT(m_shaders[pFunc]->m_shaderStats);
            }
            assert(m_shaders.size() == 1 && "Shader program is expected to contain 1 shader");
        }
    }
}

bool EmitPass::runOnFunction(llvm::Function &F)
{
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo())
    {
        return false;
    }
    m_moduleMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

    CreateKernelShaderMap(ctx, pMdUtils, F);

    // If the current kernel is deleted from the shader map, skip this function.
    m_FGA = getAnalysisIfAvailable<GenXFunctionGroupAnalysis>();
    if (!setCurrentShader(&F))
    {
        return false;
    }

    bool isCloned = false;
    if (m_currShader->GetContext()->m_instrTypes.hasDebugInfo)
    {
        auto fIT = m_moduleMD->FuncMD.find(&F);
        if (fIT != m_moduleMD->FuncMD.end() &&
            (*fIT).second.isCloned)
        {
            isCloned = true;
        }
    }

    COMPILER_TIME_START(m_currShader->GetContext(), TIME_CG_vISAEmitPass);
    COMPILER_TIME_START(m_currShader->GetContext(), TIME_vISAEmitInit);

    m_DL = &F.getParent()->getDataLayout();
    m_pattern = &getAnalysis<CodeGenPatternMatch>( );
    m_deSSA = &getAnalysis<DeSSA>();
    m_blockCoalescing = &getAnalysis<BlockCoalescing>();
    m_currShader->SetUniformHelper(&getAnalysis<WIAnalysis>());
    m_currShader->SetCodeGenHelper(m_pattern);
    m_currShader->SetDominatorTreeHelper(&getAnalysis<DominatorTreeWrapperPass>().getDomTree());
    m_currShader->SetMetaDataUtils(getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());
    m_currShader->SetShaderSpecificHelper(this);
    m_currShader->SetDataLayout(m_DL);
    m_currShader->SetFunctionGroupAnalysis(m_FGA);
    m_currShader->SetPushInfoHelper(&(m_moduleMD->pushInfo));
    if(IGC_IS_FLAG_DISABLED(DisableDeSSA))
    {
        m_currShader->SetDeSSAHelper(m_deSSA);
    }

    bool ptr64bits = (m_DL->getPointerSizeInBits(ADDRESS_SPACE_PRIVATE) == 64);
    if (!m_FGA || m_FGA->isGroupHead(&F))
    {
        m_currShader->InitEncoder(m_SimdMode, m_canAbortOnSpill, m_ShaderMode);
        // Pre-analysis pass to be executed before call to visa builder so we can pass scratch space offset
        m_currShader->PreAnalysisPass();
        if(!m_currShader->CompileSIMDSize(m_SimdMode, *this, F))
        {
			return false;
        }
        // call builder after pre-analysis pass where scratchspace offset to VISA is calculated
        m_encoder->InitEncoder(m_canAbortOnSpill);
        m_currShader->PreCompile();
        if (m_FGA && m_FGA->getGroup(&F)->hasStackCall())
        {
            m_currShader->InitKernelStack(ptr64bits);
        }
        m_currShader->AddPrologue();
    }
    else
    {
        if(!m_currShader->CompileSIMDSize(m_SimdMode, *this, F))
        {
			return false;
        }
        m_currShader->BeginFunction(&F);
        if (m_FGA && m_FGA->useStackCall(&F))
        {
            emitStackFuncEntry(&F, ptr64bits);
        }
    }

    //Add CCtuple root variables.
    m_CE = &getAnalysis<CoalescingEngine>();
    if(IGC_IS_FLAG_DISABLED(DisablePayloadCoalescing)) {
        m_currShader->SetCoalescingEngineHelper(m_CE);
    }

    auto VRA = &getAnalysis<VariableReuseAnalysis>();
    m_currShader->SetVariableReuseAnalysis(VRA);
    VRA->BeginFunction(&F, numLanes(m_SimdMode));

    if (!m_FGA || m_FGA->isGroupHead(&F))
    {
        IF_DEBUG_INFO(m_pDebugEmitter = IDebugEmitter::Create());
        IF_DEBUG_INFO(m_pDebugEmitter->Initialize(m_currShader, m_currShader->GetContext()->m_instrTypes.hasDebugInfo);)
    }

    if (m_currShader->GetContext()->m_instrTypes.hasDebugInfo)
    {
        IF_DEBUG_INFO(m_pDebugEmitter->ResetVISAModule());
        IF_DEBUG_INFO(m_pDebugEmitter->setFunction(&F, isCloned));
    }

    // We only invoke EndEncodingMark() to update last VISA id.
    IF_DEBUG_INFO(m_pDebugEmitter->EndEncodingMark();)

    COMPILER_TIME_END(m_currShader->GetContext(), TIME_vISAEmitInit);
    COMPILER_TIME_START(m_currShader->GetContext(), TIME_vISAEmitLoop);

    phiMovToBB.clear();
    unsigned int lineNo = 0;
    bool disableSlicing =
        IGC_IS_FLAG_ENABLED(DisableSIMD32Slicing) ||
        !m_currShader->GetContext()->m_retryManager.AllowSimd32Slicing() ||
        m_currShader->GetContext()->m_instrTypes.hasDebugInfo ||
        m_pattern->m_samplertoRenderTargetEnable;

    IGC::Debug::Dump* llvmtoVISADump = nullptr;
    if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
    {
        auto name =
            IGC::Debug::DumpName(IGC::Debug::GetShaderOutputName())
            .Type(m_currShader->GetShaderType())
            .Hash(ctx->hash)
            .SIMDSize(m_SimdMode)
            .Extension("visa.ll");
        llvmtoVISADump = new IGC::Debug::Dump(name, IGC::Debug::DumpType::PASS_IR_TEXT);
        llvmtoVISADump->stream() << F.getName() << "{\n\n";
    }

    DenseMap<Instruction*, uint32_t> rootToVISAId;

    for(uint i = 0; i < m_pattern->m_numBlocks; i++)
    {
        SBasicBlock& block = m_pattern->m_blocks[i];
        if(m_blockCoalescing->IsEmptyBlock(block.bb))
        {
            continue;
        }

        if (i != 0)
        {
            IF_DEBUG_INFO(m_pDebugEmitter->BeginEncodingMark();)
            // create a label
            m_encoder->Label(block.id);
            m_encoder->Push();      
            IF_DEBUG_INFO(m_pDebugEmitter->EndEncodingMark();)
        }
        // insert moves at the top of the block
        MovPhiDestination(block.bb);

        // remove cached per lane offset variables if any.
        PerLaneOffsetVars.clear();

        // Variable reuse per-block states.
        VariableReuseAnalysis::EnterBlockRAII EnterBlock(VRA, block.bb);

        // go through the list in reverse order
        auto I = block.m_dags.rbegin(), E = block.m_dags.rend();
        while (I != E)
        {
            if ((*I).m_root->getDebugLoc())
            {
                unsigned int curLineNumber = (*I).m_root->getDebugLoc().getLine();
                if (curLineNumber != lineNo)
                {
                    IF_DEBUG_INFO(m_pDebugEmitter->BeginEncodingMark();)
                    m_encoder->Loc(curLineNumber);
                    IF_DEBUG_INFO(m_pDebugEmitter->EndEncodingMark();)
                    lineNo = curLineNumber;
                }
            }

            bool slicing = false;
            uint numInstance = DecideInstanceAndSlice(*(block.bb), (*I), slicing);
            assert(numInstance == 1 || numInstance == 2);
            if (slicing && !disableSlicing)
            {
                IF_DEBUG_INFO(m_pDebugEmitter->BeginEncodingMark();)
                I = emitInSlice(block, I);
                IF_DEBUG_INFO(m_pDebugEmitter->EndEncodingMark();)
            }

            if (I != E)
            {
                // before inserting the terminator, initialize constant pool & insert the de-ssa moves
                if (isa<BranchInst>((*I).m_root))
                {
                    m_encoder->SetSecondHalf(false);
                    // insert constant initializations.
                    IF_DEBUG_INFO(m_pDebugEmitter->BeginEncodingMark();)
                    InitConstant(block.bb);
                    IF_DEBUG_INFO(m_pDebugEmitter->EndEncodingMark();)
                    // insert the de-ssa movs.
                    IF_DEBUG_INFO(m_pDebugEmitter->BeginEncodingMark();)
                    MovPhiSources(block.bb);
                    IF_DEBUG_INFO(m_pDebugEmitter->EndEncodingMark();)
                }

                // If slicing happens, then recalculate the number of instances.
                if (slicing)
                {
                    numInstance = DecideInstanceAndSlice(*(block.bb), (*I), slicing);
                }

                if (llvmtoVISADump)
                {
                    rootToVISAId.try_emplace((*I).m_root, m_encoder->GetVISAKernel()->getvIsaInstCount() + 1);
                }

                DstModifier init;
                if (numInstance < 2)
                {
                    m_encoder->SetSecondHalf(false);
                    IF_DEBUG_INFO(m_pDebugEmitter->BeginInstruction((*I).m_root);)
                    (*I).m_pattern->Emit(this, init);
                    IF_DEBUG_INFO(m_pDebugEmitter->EndInstruction((*I).m_root);)
                    ++I;
                }
                else
                {
                    IF_DEBUG_INFO(m_pDebugEmitter->BeginInstruction((*I).m_root);)
                    m_encoder->SetSecondHalf(false);
                    (*I).m_pattern->Emit(this, init);
                    m_encoder->SetSecondHalf(true);
                    (*I).m_pattern->Emit(this, init);
                    IF_DEBUG_INFO(m_pDebugEmitter->EndInstruction((*I).m_root);)
                    ++I;
                }
            }
        }
        if (llvmtoVISADump)
        {
            if (block.bb->hasName())
            {
                llvmtoVISADump->stream() << block.bb->getName() << ":\n";
            }
            else
            {
                llvmtoVISADump->stream() << "; BB" << block.id << ":\n";
            }
            for (auto BBI = block.bb->begin(), BBE = block.bb->end(); BBI != BBE; ++BBI)
            {
                Instruction* inst = &(*BBI);
                inst->print(llvmtoVISADump->stream());
                auto val = rootToVISAId.find(inst);
                if (val != rootToVISAId.end())
                {
                    llvmtoVISADump->stream() << "\t\t; visa id: " << (*val).second;
                }
                llvmtoVISADump->stream() << "\n";
            }
            llvmtoVISADump->stream() << "\n";
        }
    }

    if (llvmtoVISADump)
    {
        llvmtoVISADump->stream() << "}\n";
        delete llvmtoVISADump;
    }

    COMPILER_TIME_END(m_currShader->GetContext(), TIME_vISAEmitLoop);
    COMPILER_TIME_START(m_currShader->GetContext(), TIME_vISAEmitPayloadInputs);

    if (!m_FGA || m_FGA->isGroupHead(&F))
    {
		// Cache the arguments list into a vector for faster access
		m_currShader->CacheArgumentsList();
        // Associates values pushed to CVariable
        m_currShader->MapPushedInputs();
        // Allocate the thread payload
        m_currShader->AllocatePayload();
    }

    COMPILER_TIME_END(m_currShader->GetContext(), TIME_vISAEmitPayloadInputs);

    bool finalize = (!m_FGA || m_FGA->isGroupTail(&F));

    COMPILER_TIME_END(m_currShader->GetContext(), TIME_CG_vISAEmitPass);

    // Compile only when this is the last function for this kernel.
    bool destroyVISABuilder = false;
    if (!m_FGA || m_FGA->isGroupTail(&F))
    {
        destroyVISABuilder = true;
        m_encoder->Compile();
        // if we are doing stack-call, do the following:
        // - Hard-code a large scratch-space for visa
        if (m_FGA && m_FGA->getGroup(&F)->hasStackCall())
        {
            m_currShader->ProgramOutput()->m_scratchSpaceUsedBySpills =
                MAX(m_currShader->ProgramOutput()->m_scratchSpaceUsedBySpills, 32 * 1024);
        }
    }

    EmitDebugInfo(finalize);
    if (finalize)
    {
        IF_DEBUG_INFO(IDebugEmitter::Release(m_pDebugEmitter);)
    }

    if (destroyVISABuilder)
    {
        m_encoder->DestroyVISABuilder();
    }

    if ((m_currShader->GetShaderType() == ShaderType::COMPUTE_SHADER ||
        m_currShader->GetShaderType() == ShaderType::OPENCL_SHADER) &&
        m_currShader->m_Platform->supportDisableMidThreadPreemptionSwitch() &&
        IGC_IS_FLAG_ENABLED(EnableDisableMidThreadPreemptionOpt) &&
        (m_currShader->GetContext()->m_instrTypes.numLoopInsts == 0) &&
        (m_currShader->ProgramOutput()->m_InstructionCount < IGC_GET_FLAG_VALUE(MidThreadPreemptionDisableThreshold)))
    {
        if (m_currShader->GetShaderType() == ShaderType::COMPUTE_SHADER)
        {
            CComputeShader* csProgram = static_cast<CComputeShader*>(m_currShader);
            csProgram->SetDisableMidthreadPreemption();
        }
        else
        {
            COpenCLKernel* kernel = static_cast<COpenCLKernel*>(m_currShader);
            kernel->SetDisableMidthreadPreemption();
        }
    }

    return false;
}

// Emit code in slice starting from (reverse) iterator I. Return the iterator to
// the next pattern to emit.
SBasicBlock::reverse_iterator
EmitPass::emitInSlice(SBasicBlock& block, SBasicBlock::reverse_iterator I)
{
    auto sliceBegin = I;
    auto sliceIter = I;
    auto E = block.m_dags.rend();
    DstModifier init;

    bool slicing = true;
    m_encoder->SetSecondHalf(false);  // the 1st-half slice for simd32
    while (slicing)
    {
        (*sliceIter).m_pattern->Emit(this, init);
        ++sliceIter;
        slicing = false;
        if (sliceIter != E)
        {
            unsigned numInstance = DecideInstanceAndSlice(*(block.bb), (*sliceIter), slicing);
            assert(numInstance == 1 || numInstance == 2);
        }
    }

    // Store the point slicing stops at.
    auto sliceEnd = sliceIter;

    m_encoder->SetSecondHalf(true);  // the 2nd-half slice for simd32
    for (sliceIter = sliceBegin; sliceIter != sliceEnd; ++sliceIter)
    {
        unsigned numInstance = DecideInstanceAndSlice(*(block.bb), (*sliceIter), slicing);
        // uniform op only emit once
        if (numInstance > 1)
        {
            (*sliceIter).m_pattern->Emit(this, init);
        }
    }

    return sliceEnd;
}

/// Insert moves at the beginning of the basic block to replace the phi nodes
void EmitPass::MovPhiDestination(llvm::BasicBlock* bb)
{
    for (BasicBlock::iterator BBI = bb->begin(), BBE = bb->end(); BBI != BBE; ++BBI)
    {
        PHINode *PHI = dyn_cast<PHINode>(BBI);
        if (!PHI)
        {
            break;
        }
        if (PHI->use_empty())
        {
            continue;
        }
        IF_DEBUG_INFO(m_pDebugEmitter->BeginInstruction(PHI);)
        // insert a move from phi-temp to phi-dst
        CVariable* dst = m_currShader->GetSymbol(PHI);
        CVariable* src = m_currShader->GetPhiTemp(PHI);
        if (dst != src) {
            for(uint instance = 0; instance<dst->GetNumberInstance(); instance++)
            {
                m_encoder->SetSecondHalf(instance==1 ? true : false);
                if(VectorType *vTy = dyn_cast<VectorType>(PHI->getType()))
                {
                    emitVectorCopy(dst, src, int_cast<unsigned>(vTy->getNumElements()));
                }
                else
                {
                    m_encoder->Copy(dst, src);
                    m_encoder->Push();
                }
            }
        }
        IF_DEBUG_INFO(m_pDebugEmitter->EndInstruction(PHI);)
    }
}

/// Insert moves at the end of the basic block to replace the phi node of the successors
// This is a special case that we want to relocate the phi-mov's
// unconditionally. Two functions, isCandidateIfStmt() and
// canRelocatePhiMov(), are used to check if this is the special
// case as below:
//
//  x.1 = ...
//  ...
//  H: br i1 %cond, OtherBB, phiMovBB   // target BBs interchangeable
//  OtherBB:
//     x.0 = ...
//     br phiBB
//  phiMovBB:
//     <empty BB>
//     br phiBB
//  phiBB:
//     phi x = [x.0, OtherBB] [ x.1, phiMovBB]
//
// Normally, a phi-mov is to be inserted into phiMovBB.  This optim is to
// relocate the phi-mov to H so that we have if-then-endif other than
// if-then-else-endif. To make it simple and correct, the following
// conditions are required:
//     1. 'if' branch isn't uniform. (If uniform, it is probably not beneficial
//        to move phi-mov to H)
//     2. either x.0 is defined in otherBB or a phi-mov must be inserted
//        in the otherBB.
// With this, phi-mov can be relocated to H without using predicate.
//

// canRelocatePhiMov() checks if all phi-mov to phiMovBB can be relocated.
bool EmitPass::canRelocatePhiMov(
    llvm::BasicBlock *otherBB,
    llvm::BasicBlock *phiMovBB,
    llvm::BasicBlock* phiBB)
{
    // Threshold for phi-mov relocation
    const int CMAX_PHI_COUNT = 6;

    int n = 0;
    for (auto I = phiBB->begin(), E = phiBB->end(); I != E; ++I)
    {
        llvm::PHINode *PN = llvm::dyn_cast<llvm::PHINode>(I);
        if (!PN)
        {
            break;
        }

        CVariable* dst = m_currShader->GetPhiTemp(PN);
        for (uint i = 0, e = PN->getNumOperands(); i != e; ++i)
        {
            Value *V = PN->getOperand(i);
            CVariable* src = m_currShader->GetSymbol(V);
            if (PN->getIncomingBlock(i) == phiMovBB)
            {
                if (dst != src)
                {
                    int numElt = 1;
                    if (VectorType *vTy = dyn_cast<VectorType>(PN->getType()))
                    {
                        numElt = int_cast<int>(vTy->getNumElements());
                    }
                    // Conservatively assume the number of mov's is 'numElt'.
                    n += numElt;
                }
            }
            else
            {
                // For case with PN->getIncomingBlock(i) == otherBB
                Instruction *Inst = dyn_cast<Instruction>(V);
                if (Inst && Inst->getParent() != otherBB && (dst == src))
                {
                    // This is the case that x and x.1 are coalesced, in which
                    // we cannot move phi-mov from emptyBB to H, as doing so
                    // will clobber x.1 (x.1 and x are the same virtual reg).
                    // [Can move it up with predicate always, but need to check
                    //  doing so would give us perf benefit.]
                    //           x.1 = ...
                    //           ...
                    //        H: br c, B0, B1
                    //  otherBB:
                    //           <...>
                    //           br phiBB
                    //  emptyBB:
                    //           br phiBB
                    //    phiBB:
                    //           phi x = [x.0  emptyBB] [x.1 otherBB]
                    return false;
                }
            }
        }
    }
    if (m_currShader->m_dispatchSize == SIMDMode::SIMD32)
    {
        n = (2 * n);
    }
    return (n > 0) && (n < CMAX_PHI_COUNT);
}

// Check if 'ifBB' is the If BB for if-then-else pattern in which both then & else
// are single BBs and one of them is empty. It also make sure the branch is not
// uniform.   If it is such a BB, it returns true with emptyBB and otherBB set to
// then & else.
bool EmitPass::isCandidateIfStmt(
    llvm::BasicBlock *ifBB, llvm::BasicBlock* &otherBB, llvm::BasicBlock* &emptyBB)
{
    llvm::BranchInst *Br = dyn_cast<llvm::BranchInst>(ifBB->getTerminator());
    if (!Br || Br->getNumSuccessors() != 2 ||
        m_currShader->GetIsUniform(Br->getCondition()))
    {
        return false;
    }

    llvm::BasicBlock *S0 = Br->getSuccessor(0), *S1 = Br->getSuccessor(1);
    llvm::TerminatorInst *T0 = S0->getTerminator(), *T1 = S1->getTerminator();
    assert(T1 && T0 && "BB is missing a terminator!");
    bool  isMatch =
        S0->getSinglePredecessor() == ifBB && S1->getSinglePredecessor() == ifBB &&
        T0->getNumSuccessors() == 1 && T1->getNumSuccessors() == 1 &&
        T0->getSuccessor(0) == T1->getSuccessor(0) &&
        (S0->size() > 1 || S1->size() > 1) &&    // only one empty block
        (S0->size() == 1 || S1->size() == 1);
    if (isMatch)
    {
        if (S0->size() == 1)
        {
            emptyBB = S0;
            otherBB = S1;
        }
        else
        {
            emptyBB = S1;
            otherBB = S0;
        }
    }
    return isMatch;
}

/// Insert moves at the end of the basic block to replace the phi node of the successors
void EmitPass::MovPhiSources(llvm::BasicBlock* aBB)
{
    // collect all the src-side phi-moves, then find a good order for emission
    std::vector<std::pair<CVariable*, CVariable*>> phiSrcDstList;
    std::vector<std::pair<CVariable*, CVariable*>> emitList;
    std::map<CVariable*, unsigned int> dstVTyMap;
    llvm::BasicBlock *bb = aBB;
    llvm::TerminatorInst *TI = aBB->getTerminator();
    assert(TI);

    if (IGC_IS_FLAG_ENABLED(EnableOptimPhiMov))
    {
        if (phiMovToBB.find(bb) != phiMovToBB.end())
        {
            // All phi-mov relocated, no phi-mov generated in this bb.
            return;
        }

        // Check if phi-mov's are relocated to 'bb'
        auto II = phiMovToBB.begin(), IE = phiMovToBB.end();
        for (; II != IE; ++II)
        {
            BasicBlock* toBB = II->second;
            if (toBB == bb)
            {
                break;
            }
        }

        // Only handle if-then-else, in which one of then & else is empty.
        llvm::BasicBlock *emptyBB, *otherBB;
        if (II != IE)
        {
            // phi-mov's are moved into this bb. Reset bb & TI to the original
            // BB, and it goes to the main code to generate the mov's.
            bb = II->first;
            TI = bb->getTerminator();
        }
        else if (TI->getNumSuccessors() == 1 && bb->size() == 1)
        {
            // Check if this bb is an empty BB whose phi-mov are relocated.
            // Note that this is unlikely reached as we emit code forward and the "ifBB"
            // shall be processed first, which set phiMovToBB map from this "bb" to
            // "ifBB".  If so,  when this "bb" is processed, it will return immediately as
            // it is in the map (meaning it has been processed already).
            llvm::BasicBlock *ifBB = bb->getSinglePredecessor();
            if (ifBB && isCandidateIfStmt(ifBB, otherBB, emptyBB))
            {
                llvm::BasicBlock *phiB = TI->getSuccessor(0);
                if (canRelocatePhiMov(otherBB, emptyBB, phiB))
                {
                    // Insert it into map so that phi-mov's are generated once ifBB is processed.
                    phiMovToBB.insert(std::pair<llvm::BasicBlock*, llvm::BasicBlock*>(bb, ifBB));
                    return;
                }
            }
        }
        else if (TI->getNumSuccessors() == 2 && isCandidateIfStmt(bb, otherBB, emptyBB))
        {
            // Check if this bb is the BB ("ifBB") that all phi-mov's are relocated to.
            llvm::TerminatorInst *T0 = emptyBB->getTerminator();
            assert(T0->getNumSuccessors() == 1 && "Something wrong in if-then-else pattern!");
            llvm::BasicBlock *phiB = T0->getSuccessor(0);
            if (canRelocatePhiMov(otherBB, emptyBB, phiB))
            {
                phiMovToBB.insert(std::pair<llvm::BasicBlock*, llvm::BasicBlock*>(emptyBB, bb));

                // fall-thru to the main code.
                bb = emptyBB;
                TI = bb->getTerminator();
            }
        }
    }

    // main code to generate phi-mov
    for (unsigned succ = 0, e = TI->getNumSuccessors(); succ != e; ++succ)
    {
        llvm::BasicBlock *Succ = TI->getSuccessor(succ);
        for (auto II = Succ->begin(), IE = Succ->end(); II != IE; ++II)
        {
            llvm::PHINode *PN = llvm::dyn_cast<llvm::PHINode>(II);
            if (!PN)
            {
                break;
            }
            if (PN->use_empty())
            {
                continue;
            }
            for (uint i = 0, e = PN->getNumOperands(); i != e; ++i)
            {
                if (PN->getIncomingBlock(i) == bb)
                {
                    CVariable* dst = m_currShader->GetPhiTemp(PN);
                    CVariable* src = m_currShader->GetSymbol(PN->getOperand(i));
                    if (dst != src)
                    {
                        int numElt = 0;
                        if (VectorType *vTy = dyn_cast<VectorType>(PN->getType()))
                        {
                            numElt = int_cast<int>(vTy->getNumElements());
                        }
                        dstVTyMap.insert(std::pair<CVariable*, unsigned int>(dst, numElt));

                        if (m_deSSA->isPHIIsolated(PN))
                            emitList.push_back(std::pair<CVariable*, CVariable*>(src, dst));
                        else
                            phiSrcDstList.push_back(std::pair<CVariable*, CVariable*>(src, dst));
                    }
                }
            }
        }
    }

    // find a good order for src-side phi-moves
    while (!phiSrcDstList.empty())
    {
        // search should not get into a deadlock, i.e should be able to find one to emit every iteration,
        auto It = phiSrcDstList.begin();
        auto Et = phiSrcDstList.end();
        for (; It != Et; ++It)
        {
            auto Cmp = [=](const std::pair<CVariable *, CVariable *> &val)
            {
                return val.first == It->second;
            };

            if (0 == std::count_if(phiSrcDstList.begin(), phiSrcDstList.end(), Cmp))
            {
                break;
            }
        }
        assert(It != Et);
        emitList.push_back(std::pair<CVariable*, CVariable*>(It->first, It->second));
        phiSrcDstList.erase(It);
    }
    // emit the src-side phi-moves
    for (unsigned i = 0, e = int_cast<unsigned>(emitList.size()); i != e; ++i)
    {
        CVariable* dst = emitList[i].second;
        CVariable* src = emitList[i].first;

        for (uint instance = 0; instance < dst->GetNumberInstance(); instance++)
        {
            m_encoder->SetSecondHalf(instance == 1 ? true : false);
            unsigned int numVTyElt = dstVTyMap[dst];
            if (numVTyElt > 0)
            {
                emitVectorCopy(dst, src, numVTyElt);
            }
            else
            {
                m_encoder->Copy(dst, src);
                m_encoder->Push();
            }
        }
    }
}

void EmitPass::InitConstant(llvm::BasicBlock *BB)
{
    for (auto &I : m_pattern->ConstantPlacement)
    {
        if (I.second != BB)
            continue;
        Constant *C = I.first;
        CVariable *Dst = m_currShader->lookupConstantInPool(C);
        if (Dst)
            continue;
        Dst = m_currShader->GetConstant(C);
        if (!C->getType()->isVectorTy()) {
            CVariable *Imm = Dst;
            Dst = m_currShader->GetNewVector(C);
            m_encoder->Copy(Dst, Imm);
            m_encoder->Push();
        }
        m_currShader->addConstantInPool(C, Dst);
    }
}

std::pair<Value *, Value *> EmitPass::getPairOutput(Value *V) const {
    auto I = m_pattern->PairOutputMap.find(V);
    assert(I != m_pattern->PairOutputMap.end());
    return std::make_pair(I->second.first, I->second.second);
}

void EmitPass::emitGradientX(const SSource& source, const DstModifier& modifier)
{
    CVariable* src = GetSrcVariable(source);
    if(src->IsUniform())
    {
        m_encoder->SetSrcModifier(1, EMOD_NEG);
        m_encoder->Add(m_destination, src, src);
        m_encoder->Push();
    }
    else
    {
        // we need to combine negation with the existing source modifiers
        // to implement subtraction of values correct also for neg, abs, negabs
        const e_modifier src_mod0 = source.mod;
        const e_modifier src_mod1 = CombineModifier(EMOD_NEG, src_mod0);
        m_encoder->SetSrcModifier(0, src_mod0);
        m_encoder->SetSrcModifier(1, src_mod1);
        m_encoder->SetDstModifier(modifier);
        // set the regioning to get isa instruction
        // add dst0.0<1>:f   src0.1<4;4,0>:f   -src0.0<4;4,0>:f
        m_encoder->SetSrcRegion(0, 4, 4, 0);
        m_encoder->SetSrcRegion(1, 4, 4, 0);
        m_encoder->SetSrcSubReg(0, 1);
        m_encoder->SetSrcSubReg(1, 0);
        m_encoder->Add(m_destination, src, src);
        m_encoder->Push();
    }
}

void EmitPass::emitGradientY(const SSource& source, const DstModifier& modifier)
{
    CVariable* src = GetSrcVariable(source);
    if(src->IsUniform())
    {
        m_encoder->SetSrcModifier(1, EMOD_NEG);
        m_encoder->Add(m_destination, src, src);
        m_encoder->Push();
    }
    else
    {
        const e_modifier src_mod0 = source.mod;
        const e_modifier src_mod1 = CombineModifier(EMOD_NEG, src_mod0);
        m_encoder->SetSrcModifier(0, src_mod0);
        m_encoder->SetSrcModifier(1, src_mod1);
        m_encoder->SetDstModifier(modifier);
        // set the regioning to get isa instruction
        // add dst0.0<1>:f   src0.1<4;4,0>:f   -src0.0<4;4,0>:f
        m_encoder->SetSrcRegion(0, 4, 4, 0);
        m_encoder->SetSrcRegion(1, 4, 4, 0);
        m_encoder->SetSrcSubReg(0, 2);
        m_encoder->SetSrcSubReg(1, 0);
        m_encoder->Add(m_destination, src, src);
        m_encoder->Push();
    }
}

void EmitPass::emitGradientXFine(const SSource& source, const DstModifier& modifier)
{
    CVariable* src = GetSrcVariable(source);
    if(src->IsUniform())
    {
        m_encoder->SetSrcModifier(1, EMOD_NEG);
        m_encoder->Add(m_destination, src, src);
        m_encoder->Push();
    }
    else
    {
        const e_modifier src_mod0 = source.mod;
        const e_modifier src_mod1 = CombineModifier(EMOD_NEG, src_mod0);
        m_encoder->SetSrcModifier(0, src_mod0);
        m_encoder->SetSrcModifier(1, src_mod1);
        m_encoder->SetDstModifier(modifier);
        // set the regioning to get isa instruction
        // add dst0.0<1>:f   src0.1<2;2,0>:f   -src0.0<2;2,0>:f
        m_encoder->SetSrcRegion(0, 2, 2, 0);
        m_encoder->SetSrcRegion(1, 2, 2, 0);
        m_encoder->SetSrcSubReg(0, 1);
        m_encoder->SetSrcSubReg(1, 0);
        m_encoder->Add(m_destination, src, src);
        m_encoder->Push();
    }
}

/// Computes derivatives with respect to screen space by subtracting values for
/// adjacent pixels in vertical direction.
/// Consider the following four pixels:
/// +----+----+
/// | P0 | P1 |
/// +----+----+
/// | P2 | P3 |
/// +----+----+
///
/// then gradient_y_fine for scalar attribute A of pixel P0 will be P0.A - P2.A
/// The same value will be used for P2 since the spec leaves the freedom of
/// choosing the quad alignment. The same goes for P1 and P3.
///
/// Now, if we look at the attribute A as laid out in a SIMD register, we have
///
/// src0 =  A : |    |    |    |    | P3.A | P2.A | P1.A | P0.A |
///
/// and the result register should contain
///
/// dst0 = dy : |    |    |    |    |  q   |  t   |   q  |   t  |
///
/// where t = P0.A - P2.A and q = P1.A - P3.A
///
/// The upper half of GRF also contains data for another separate set of four pixels.
///
/// We compute the result by the following sequence of instructions
///
/// add (4)  dst0.0<1>:f src0.0<0; 2, 1>:f -src0.2<0; 2, 1>:f   // lower half
/// add (4)  dst0.4<1>:f src0.4<0; 2, 1>:f -src0.6<0; 2, 1>:f   // upper half
///
/// and if we are in simd16 mode, we need two more instructions
/// if (simd16)
/// {
///    add (4)   dst0.8<1>:f  src0.8<0; 2, 1>:f -src0.10<0; 2, 1>:f
///    add (4)  dst0.12<1>:f src0.12<0; 2, 1>:f -src0.14<0; 2, 1>:f
/// }
///
/// Note: Since the source llvm instruction may contain source modifier (abs, neg, negabs)
/// we need to read them and flip the sign of the second isa source accordingly.
///////////////////////////////////////////////////////////////////////////////
void EmitPass::emitGradientYFine(const SSource& source, const DstModifier& modifier)
{
    CVariable* src = GetSrcVariable(source);
    if(src->IsUniform())
    {
        m_encoder->SetSrcModifier(1, EMOD_NEG);
        m_encoder->Add(m_destination, src, src);
        m_encoder->Push();
    }
    else
    {
        CVariable* temp = m_currShader->GetNewVariable(m_destination);
        const e_modifier src_mod0 = source.mod;
        const e_modifier src_mod1 = CombineModifier(EMOD_NEG, src_mod0);

        m_encoder->SetSimdSize(SIMDMode::SIMD4);
        m_encoder->SetSrcModifier(0, src_mod0);
        m_encoder->SetSrcRegion(0, 0, 2, 1);
        m_encoder->SetSrcSubReg(0, 2);

        m_encoder->SetSrcModifier(1, src_mod1);
        m_encoder->SetSrcRegion(1, 0, 2, 1);
        m_encoder->SetSrcSubReg(1, 0);
        m_encoder->SetNoMask();

        m_encoder->SetDstModifier(modifier);
        m_encoder->SetDstSubReg(0);
        m_encoder->Add(temp, src, src);
        m_encoder->Push();

        m_encoder->SetSimdSize(SIMDMode::SIMD4);
        m_encoder->SetSrcModifier(0, src_mod0);
        m_encoder->SetSrcRegion(0, 0, 2, 1);
        m_encoder->SetSrcSubReg(0, 6);

        m_encoder->SetSrcModifier(1, src_mod1);
        m_encoder->SetSrcRegion(1, 0, 2, 1);
        m_encoder->SetSrcSubReg(1, 4);
        m_encoder->SetNoMask();


        m_encoder->SetDstModifier(modifier);
        m_encoder->SetDstSubReg(4);
        m_encoder->Add(temp, src, src);
        m_encoder->Push();

        if(m_currShader->m_SIMDSize == SIMDMode::SIMD16)
        {
            m_encoder->SetSimdSize(SIMDMode::SIMD4);
            m_encoder->SetSrcModifier(0, src_mod0);
            m_encoder->SetSrcRegion(0, 0, 2, 1);
            m_encoder->SetSrcSubReg(0, 10);

            m_encoder->SetSrcModifier(1, src_mod1);
            m_encoder->SetSrcRegion(1, 0, 2, 1);
            m_encoder->SetSrcSubReg(1, 8);
            m_encoder->SetNoMask();

            m_encoder->SetDstModifier(modifier);
            m_encoder->SetDstSubReg(8);
            m_encoder->Add(temp, src, src);
            m_encoder->Push();

            m_encoder->SetSimdSize(SIMDMode::SIMD4);
            m_encoder->SetSrcModifier(0, src_mod0);
            m_encoder->SetSrcRegion(0, 0, 2, 1);
            m_encoder->SetSrcSubReg(0, 14);

            m_encoder->SetSrcModifier(1, src_mod1);
            m_encoder->SetSrcRegion(1, 0, 2, 1);
            m_encoder->SetSrcSubReg(1, 12);

            m_encoder->SetNoMask();
            m_encoder->SetDstModifier(modifier);
            m_encoder->SetDstSubReg(12);
            m_encoder->Add(temp, src, src);
            m_encoder->Push();
        }
        m_encoder->Copy(m_destination, temp);
        m_encoder->Push();
    }
}

void EmitPass::EmitAluIntrinsic(llvm::CallInst* I, const SSource source[2], const DstModifier& modifier)
{
    if(GenIntrinsicInst *CI = dyn_cast<GenIntrinsicInst>(I))
    {
        switch(CI->getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_GradientX:
            emitGradientX(source[0], modifier);
            break;
        case GenISAIntrinsic::GenISA_GradientXfine:
            emitGradientXFine(source[0], modifier);
            break;
        case GenISAIntrinsic::GenISA_GradientY:
            emitGradientY(source[0], modifier);
            break;
        case GenISAIntrinsic::GenISA_GradientYfine:
            emitGradientYFine(source[0], modifier);
            break;
        default:
            // no special handling
            EmitSimpleAlu(I, source, modifier);
            break;
        }
    }
    else if(IntrinsicInst *CI = dyn_cast<IntrinsicInst>(I))
    {
        switch(CI->getIntrinsicID())
        {
        case Intrinsic::ctlz:
            //Throw away source[1], since for ctlz, this is a flag we don't care about.
            emitCtlz(source[0]);
            break;
        default:
            // no special handling
            EmitSimpleAlu(I, source, modifier);
            break;
        }
    }
}

// Those help functions are used only by this file. If other files use them,
// they should be moved to helper.cpp.
static e_predicate GetPredicate(llvm::CmpInst::Predicate predicate)
{
    switch (predicate)
    {
    case llvm::CmpInst::ICMP_UGT:
    case llvm::CmpInst::ICMP_SGT:
    case llvm::CmpInst::FCMP_UGT:
    case llvm::CmpInst::FCMP_OGT:
        return EPREDICATE_GT;
    case llvm::CmpInst::ICMP_UGE:
    case llvm::CmpInst::ICMP_SGE:
    case llvm::CmpInst::FCMP_UGE:
    case llvm::CmpInst::FCMP_OGE:
        return EPREDICATE_GE;
    case llvm::CmpInst::ICMP_ULT:
    case llvm::CmpInst::ICMP_SLT:
    case llvm::CmpInst::FCMP_ULT:
    case llvm::CmpInst::FCMP_OLT:
        return EPREDICATE_LT;
    case llvm::CmpInst::ICMP_ULE:
    case llvm::CmpInst::ICMP_SLE:
    case llvm::CmpInst::FCMP_ULE:
    case llvm::CmpInst::FCMP_OLE:
        return EPREDICATE_LE;
    case llvm::CmpInst::ICMP_EQ:
    case llvm::CmpInst::FCMP_UEQ:
    case llvm::CmpInst::FCMP_OEQ:
        return EPREDICATE_EQ;
    case llvm::CmpInst::ICMP_NE:
    case llvm::CmpInst::FCMP_UNE:
        return EPREDICATE_NE;
    default:
        break;
    }
    assert(0);
    return EPREDICATE_EQ;
}

static VISA_Type GetUnsignedType(VISA_Type type)
{
    switch(type)
    {
    case ISA_TYPE_Q:
    case ISA_TYPE_UQ:
        return ISA_TYPE_UQ;
    case ISA_TYPE_D:
    case ISA_TYPE_UD:
        return ISA_TYPE_UD;
    case ISA_TYPE_W:
    case ISA_TYPE_UW:
        return ISA_TYPE_UW;
    case ISA_TYPE_B:
    case ISA_TYPE_UB:
        return ISA_TYPE_UB;
    default:
        assert(0);
        break;
    }
    return ISA_TYPE_UD;
}

static VISA_Type GetSignedType(VISA_Type type)
{
    switch (type)
    {
    case ISA_TYPE_Q:
    case ISA_TYPE_UQ:
        return ISA_TYPE_Q;
    case ISA_TYPE_D:
    case ISA_TYPE_UD:
        return ISA_TYPE_D;
    case ISA_TYPE_W:
    case ISA_TYPE_UW:
        return ISA_TYPE_W;
    case ISA_TYPE_B:
    case ISA_TYPE_UB:
        return ISA_TYPE_B;
    default:
        assert(0);
        break;
    }
    return ISA_TYPE_D;
}

static VISA_Type GetUnsignedIntegerType(VISA_Type type)
{
    switch (type)
    {
    case ISA_TYPE_Q:
    case ISA_TYPE_UQ:
        return ISA_TYPE_UQ;
    case ISA_TYPE_D:
    case ISA_TYPE_UD:
        return ISA_TYPE_UD;
    case ISA_TYPE_W:
    case ISA_TYPE_UW:
        return ISA_TYPE_UW;
    case ISA_TYPE_B:
    case ISA_TYPE_UB:
        return ISA_TYPE_UB;
    case ISA_TYPE_DF:
        return ISA_TYPE_UQ;
    case ISA_TYPE_F:
        return ISA_TYPE_UD;
    case ISA_TYPE_HF:
        return ISA_TYPE_UW;
    default:
        assert(0);
        break;
    }
    return ISA_TYPE_UD;
}

static uint64_t getFPOne(VISA_Type Ty)
{
    switch (Ty)
    {
    case ISA_TYPE_DF:   return 0x3FF0000000000000;
    case ISA_TYPE_F:    return 0x3F800000;
    case ISA_TYPE_HF:   return 0x3C00;
    default: break;
    }
    assert(false && "unknown floating type!");
    return ~0U;
}

CVariable* EmitPass::GetSrcVariable(const SSource& source, bool fromConstPool)
{
    CVariable* src = m_currShader->GetSymbol(source.value, fromConstPool);
    // Change the type of source if needed.
    if(source.type != ISA_TYPE_NUM && source.type != src->GetType())
    {
        if (src->IsImmediate()) {
            src = m_currShader->ImmToVariable(src->GetImmediateValue(), source.type);
        }
        else {
            src = m_currShader->GetNewAlias(src, source.type, 0, src->GetNumberElement());
        }
    }
    return src;
}

void EmitPass::SetSourceModifiers(unsigned int sourceIndex, const SSource& source)
{
    m_encoder->SetSrcModifier(sourceIndex, source.mod);  
    int numberOfLanes = 0;
    if (m_currShader->GetIsUniform(source.value))
    {
        numberOfLanes = 1;
    }
    else
    {
        numberOfLanes = numLanes(m_currShader->m_SIMDSize);
    }
    int calculated_offset = source.SIMDOffset * numberOfLanes + source.elementOffset;
    m_encoder->SetSrcSubReg(sourceIndex, calculated_offset);
   
    if(source.region_set)
    {
        m_encoder->SetSrcRegion(sourceIndex, source.region[0], source.region[1], source.region[2]);
    }
}

void EmitPass::EmitSimpleAlu(Instruction* inst, const SSource sources[2], const DstModifier& modifier)
{
    EmitSimpleAlu(GetOpCode(inst), sources, modifier);
}

void EmitPass::EmitSimpleAlu(Instruction* inst, CVariable* dst, CVariable* src0, CVariable* src1)
{
    EmitSimpleAlu(GetOpCode(inst), dst, src0, src1);
}

void EmitPass::EmitSimpleAlu(EOPCODE opCode, const SSource sources[2], const DstModifier& modifier)
{
    CVariable* srcs[2] = {nullptr, nullptr};

    srcs[0] = GetSrcVariable(sources[0]);
    SetSourceModifiers(0, sources[0]);

    if(sources[1].value)
    {
        srcs[1] = GetSrcVariable(sources[1]);
        SetSourceModifiers(1, sources[1]);
    }
    m_encoder->SetDstModifier(modifier);
    EmitSimpleAlu(opCode, m_destination, srcs[0], srcs[1]);
}

void EmitPass::EmitSimpleAlu(EOPCODE opCode, CVariable* dst, CVariable* src0, CVariable* src1)
{
    switch (opCode)
    {
    case llvm_fmul:
    case llvm_mul:
        m_encoder->Mul(dst,src0,src1);
        break;
    case llvm_fdiv:
        m_encoder->Div(dst,src0,src1);
        break;
    case llvm_fadd:
    case llvm_add:
        m_encoder->Add(dst,src0,src1);
        break;
    case llvm_cos:
        m_encoder->Cos(dst,src0);
        break;
    case llvm_sin:
        m_encoder->Sin(dst,src0);
        break;
    case llvm_log:
        m_encoder->Log(dst,src0);
        break;
    case llvm_exp:
        m_encoder->Exp(dst,src0);
        break;
    case llvm_pow:
        m_encoder->Pow(dst, src0, src1);
        break;
    case llvm_sqrt:
        m_encoder->Sqrt(dst,src0);
        break;
    case llvm_rsq:
        m_encoder->Rsqrt(dst, src0);
        break;
    case llvm_floor:
        m_encoder->Floor(dst, src0);
        break;
    case llvm_ceil:
        m_encoder->Ceil(dst, src0);
        break;
    case llvm_round_z:
        m_encoder->Truncate(dst, src0);
        break;
    case llvm_roundne:
        m_encoder->RoundNE(dst, src0);
        break;
    case llvm_imulh:
        m_encoder->MulH( dst, src0, src1 );
        break;
    case llvm_umulh:
        {
            src0 = m_currShader->BitCast(src0, GetUnsignedType(src0->GetType()));
            src1 = m_currShader->BitCast(src1, GetUnsignedType(src1->GetType()));
            dst  = m_currShader->BitCast(dst, GetUnsignedType(dst->GetType()));
            m_encoder->MulH( dst, src0, src1 );
        }
        break;
    case llvm_sext:
        {
            if( src0->GetType() == ISA_TYPE_BOOL )
            {
                CVariable* minusone = m_currShader->ImmToVariable(-1, dst->GetType());
                CVariable* zero = m_currShader->ImmToVariable(0, dst->GetType());
                m_encoder->Select(src0, dst, minusone, zero );
            }
            else
            {
                m_encoder->Cast( dst, src0 );
            }
        }
        break;
    case llvm_zext:
        {
            if( src0->GetType() == ISA_TYPE_BOOL )
            {
                CVariable* one = m_currShader->ImmToVariable(1, dst->GetType());
                CVariable* zero = m_currShader->ImmToVariable(0, dst->GetType());
                m_encoder->Select(src0, dst, one, zero );
            }
            else
            {
                src0 = m_currShader->BitCast(src0, GetUnsignedType(src0->GetType()));
                m_encoder->Cast( dst, src0 );
            }
        }
        break;
    case llvm_trunc:
    case llvm_fptrunc:
    case llvm_fpext:
    case llvm_fptosi:
    case llvm_fptoui:
        if(dst->GetType() == ISA_TYPE_BOOL)
        {
            m_encoder->Cmp(EPREDICATE_NE, dst, src0, m_currShader->ImmToVariable(0, src0->GetType()));
        }
        else
        {
            if(opCode == llvm_fptoui)
            {
                dst = m_currShader->BitCast(dst,GetUnsignedType(dst->GetType()));
            }
            m_encoder->Cast(dst,src0);
        }
        break;
    case llvm_sitofp:
    case llvm_uitofp:
        if( src0->GetType() == ISA_TYPE_BOOL )
        {
            CVariable* one = m_currShader->ImmToVariable(getFPOne(dst->GetType()), dst->GetType());
            CVariable* zero = m_currShader->ImmToVariable(0, dst->GetType());
            m_encoder->Select(src0, dst, one, zero );
        }
        else
        {
            if(opCode == llvm_uitofp)
            {
                src0 = m_currShader->BitCast(src0,GetUnsignedType(src0->GetType()));
            }
            m_encoder->Cast(dst,src0);
        }
        break;
    case llvm_xor:
        m_encoder->Xor(dst, src0, src1);
        break;
    case llvm_or:
        m_encoder->Or(dst, src0, src1);
        break;
    case llvm_and:
        m_encoder->And(dst, src0, src1);
        break;
    case llvm_udiv:
        {
            src0 = m_currShader->BitCast( src0, GetUnsignedType( src0->GetType() ) );
            src1 = m_currShader->BitCast( src1, GetUnsignedType( src1->GetType() ) );
            dst  = m_currShader->BitCast(dst, GetUnsignedType(dst->GetType()));
            m_encoder->Div( dst, src0, src1 );
        }
        break;
    case llvm_sdiv:
        m_encoder->Div( dst, src0, src1 );
        break;
    case llvm_urem:
        {
            src0 = m_currShader->BitCast( src0, GetUnsignedType( src0->GetType() ) );
            src1 = m_currShader->BitCast( src1, GetUnsignedType( src1->GetType() ) );
            dst  = m_currShader->BitCast(dst, GetUnsignedType(dst->GetType()));
            m_encoder->Mod( dst, src0, src1 );
        }
        break;
    case llvm_srem:
        m_encoder->Mod( dst, src0, src1 );
        break;
    case llvm_shl:
        m_encoder->Shl(dst, src0, src1);
        break;
    case llvm_ishr:
        m_encoder->IShr( dst, src0, src1 );
        break;
    case llvm_ushr:
        {
            src0 = m_currShader->BitCast( src0, GetUnsignedType( src0->GetType() ) );
            m_encoder->Shr( dst, src0, src1 );
        }
        break;
    case llvm_min:
        m_encoder->Min( dst, src0, src1 );
        break;
    case llvm_max:
        m_encoder->Max( dst, src0, src1 );
        break;
    case llvm_uaddc:
        {
            src0 = m_currShader->BitCast( src0, GetUnsignedType( src0->GetType() ) );
            src1 = m_currShader->BitCast( src1, GetUnsignedType( src1->GetType() ) );
            dst = m_currShader->BitCast( dst, GetUnsignedType( dst->GetType() ) );
            m_encoder->UAddC( dst, src0, src1 );
        }
        break;
    case llvm_usubb:
        {
            src0 = m_currShader->BitCast( src0, GetUnsignedType( src0->GetType() ) );
            src1 = m_currShader->BitCast( src1, GetUnsignedType( src1->GetType() ) );
            dst = m_currShader->BitCast( dst, GetUnsignedType( dst->GetType() ) );
            m_encoder->USubB( dst, src0, src1 );
        }
        break;
    case llvm_bfrev:
        m_encoder->Bfrev(dst, src0);
        break;
    case llvm_cbit: {
        src0 = m_currShader->BitCast(src0, GetUnsignedType(src0->GetType()));
        dst = m_currShader->BitCast(dst, GetUnsignedType(dst->GetType()));
        if (dst->GetType() == ISA_TYPE_UD) {
            m_encoder->CBit(dst, src0);
            break;
        }
        if (dst->GetType() == ISA_TYPE_UW || dst->GetType() == ISA_TYPE_UB) {
            // vISA only supports UD destination. Need a temporary and truncate
            // from it.
            CVariable* tmp
                = m_currShader->GetNewVariable(dst->GetNumberElement(),
                        ISA_TYPE_UD,
                        dst->IsUniform() ? EALIGN_DWORD : EALIGN_GRF,
                        dst->IsUniform());
            m_encoder->CBit(tmp, src0);
            m_encoder->Push();
            m_encoder->Cast(dst, tmp);
            break;
        }
        assert(dst->GetType() == ISA_TYPE_UQ);
        // TODO: So far, 64-bit popcnt is handled in LLVM IR as follows:
        // dst = popcnt.32(src & 0xFFFFFFFF);
        // dst += popcnt.32(src >> 32);
        // We could do the same thing here if the original sequence in LLVM IR
        // cannot be translated efficienty.
        assert(false && "NOT IMPLEMENTED YET!");
        break;
    }
    case llvm_ieee_sqrt:
        m_encoder->IEEESqrt(dst, src0);
        break;
    case llvm_ieee_divide:
        m_encoder->IEEEDivide(dst, src0, src1);
        break;
    default:
        //need support
        assert(0);
        break;
    }
    m_encoder->Push();
}

void EmitPass::EmitMinMax(bool isMin, bool isUnsigned, const SSource sources[2], const DstModifier& modifier) {
    EOPCODE opCode = isMin ? llvm_min : llvm_max;
    CVariable* srcs[2] = { nullptr, nullptr };
    CVariable* dst = m_destination;
    srcs[0] = GetSrcVariable(sources[0]);
    srcs[1] = GetSrcVariable(sources[1]);
    SetSourceModifiers(0, sources[0]);
    SetSourceModifiers(1, sources[1]);
    m_encoder->SetDstModifier(modifier);
    if (isUnsigned) {
        srcs[0] = m_currShader->BitCast(srcs[0], GetUnsignedType(srcs[0]->GetType()));
        srcs[1] = m_currShader->BitCast(srcs[1], GetUnsignedType(srcs[1]->GetType()));
        dst = m_currShader->BitCast(m_destination, GetUnsignedType(m_destination->GetType()));
    }
    EmitSimpleAlu(opCode, dst, srcs[0], srcs[1]);
}

void EmitPass::EmitFullMul32(bool isUnsigned, const SSource sources[2], const DstModifier& dstMod) {
    CVariable* srcs[2] = {nullptr, nullptr};
    srcs[0] = GetSrcVariable(sources[0]);
    srcs[1] = GetSrcVariable(sources[1]);
    SetSourceModifiers(0, sources[0]);
    SetSourceModifiers(1, sources[1]);
    m_encoder->SetDstModifier(dstMod);
    if (isUnsigned) {
        srcs[0] = m_currShader->BitCast(srcs[0], GetUnsignedType(srcs[0]->GetType()));
        srcs[1] = m_currShader->BitCast(srcs[1], GetUnsignedType(srcs[1]->GetType()));
    }
    // Emit *D x *D -> *Q supported by Gen
    EmitSimpleAlu(llvm_mul, m_destination, srcs[0], srcs[1]);
}

void EmitPass::EmitFPToIntWithSat(bool isUnsigned, bool needBitCast, VISA_Type type, const SSource &source, const DstModifier& dstMod) {
    EOPCODE op = isUnsigned ? llvm_fptoui : llvm_fptosi;

    CVariable *dst = m_destination;
    if (type != m_destination->GetType()) {
        dst = m_currShader->GetNewVariable(dst->GetNumberElement(), type, EALIGN_GRF, dst->IsUniform());
    } else if (needBitCast) {
        dst = m_currShader->BitCast(dst, GetUnsignedIntegerType(dst->GetType()));
    }
    DstModifier satDstMod = dstMod;
    satDstMod.sat = true;
    m_encoder->SetDstModifier(satDstMod);

    CVariable *src = GetSrcVariable(source);
    SetSourceModifiers(0, source);
    EmitSimpleAlu(op, dst, src, nullptr);
    if (type != m_destination->GetType()) {
        CVariable *tmp = m_currShader->BitCast(dst, GetUnsignedType(type));
        dst = m_destination;
        if (needBitCast) {
            dst = m_currShader->BitCast(dst, GetUnsignedIntegerType(dst->GetType()));
        }
        m_encoder->Cast(dst, tmp);
    }
}

void EmitPass::EmitIntegerTruncWithSat(bool isSignedDst, bool isSignedSrc, const SSource &source, const DstModifier &dstMod) {
    CVariable *dst = m_destination;
    if (!isSignedDst) {
        dst = m_currShader->BitCast(dst, GetUnsignedIntegerType(dst->GetType()));
    }
    DstModifier satDstMod = dstMod;
    satDstMod.sat = true;
    m_encoder->SetDstModifier(satDstMod);

    CVariable *src = GetSrcVariable(source);
    if (!isSignedSrc) {
        src = m_currShader->BitCast(src, GetUnsignedIntegerType(src->GetType()));
    }
    m_encoder->SetSrcModifier(0, source.mod);

    m_encoder->Cast(dst, src);
    m_encoder->Push();
}

void EmitPass::EmitAddPair(GenIntrinsicInst *GII, const SSource Sources[4], const DstModifier &DstMod) {
    Value *L, *H;
    std::tie(L, H) = getPairOutput(GII);
    CVariable *Lo = L ? GetSymbol(L) : nullptr;
    CVariable *Hi = H ? GetSymbol(H) : nullptr;
    assert(Lo == m_destination || Hi == m_destination);

    CVariable *L0 = GetSrcVariable(Sources[0]);
    CVariable *H0 = GetSrcVariable(Sources[1]);
    CVariable *L1 = GetSrcVariable(Sources[2]);
    CVariable *H1 = GetSrcVariable(Sources[3]);

    m_encoder->AddPair(Lo, Hi, L0, H0, L1, H1);
    m_encoder->Push();
}

void EmitPass::EmitSubPair(GenIntrinsicInst *GII, const SSource Sources[4], const DstModifier &DstMod) {
    Value *L, *H;
    std::tie(L, H) = getPairOutput(GII);
    CVariable *Lo = L ? GetSymbol(L) : nullptr;
    CVariable *Hi = H ? GetSymbol(H) : nullptr;
    assert(Lo == m_destination || Hi == m_destination);

    CVariable *L0 = GetSrcVariable(Sources[0]);
    CVariable *H0 = GetSrcVariable(Sources[1]);
    CVariable *L1 = GetSrcVariable(Sources[2]);
    CVariable *H1 = GetSrcVariable(Sources[3]);

    m_encoder->SubPair(Lo, Hi, L0, H0, L1, H1);
    m_encoder->Push();
}

void EmitPass::EmitMulPair(GenIntrinsicInst *GII, const SSource Sources[4], const DstModifier &DstMod) {
    Value *L, *H;
    std::tie(L, H) = getPairOutput(GII);
    CVariable *Lo = L ? GetSymbol(L) : nullptr;
    CVariable *Hi = H ? GetSymbol(H) : nullptr;
    assert(Lo == m_destination || Hi == m_destination);

    CVariable *L0 = GetSrcVariable(Sources[0]);
    CVariable *H0 = GetSrcVariable(Sources[1]);
    CVariable *L1 = GetSrcVariable(Sources[2]);
    CVariable *H1 = GetSrcVariable(Sources[3]);

    // Use `UD` for Lo(s).
    if (Lo && Lo->GetType() != ISA_TYPE_UD) Lo = m_currShader->BitCast(Lo, ISA_TYPE_UD);
    if (L0->GetType() != ISA_TYPE_UD) L0 = m_currShader->BitCast(L0, ISA_TYPE_UD);
    if (L1->GetType() != ISA_TYPE_UD) L1 = m_currShader->BitCast(L1, ISA_TYPE_UD);


    if (Lo != nullptr) {
        m_encoder->Mul(Lo, L0, L1);
        m_encoder->Push();
    }

    if (Hi == nullptr) {
        // Skip if Hi is ignored.
        return;
    }

    m_encoder->MulH(Hi, L0, L1);
    m_encoder->Push();

    CVariable *T0 = m_currShader->GetNewVariable(Hi->GetNumberElement(), Hi->GetType(), Hi->GetAlign(), Hi->IsUniform());

    m_encoder->Mul(T0, L0, H1);
    m_encoder->Push();

    m_encoder->Add(Hi, Hi, T0);
    m_encoder->Push();

    m_encoder->Mul(T0, L1, H0);
    m_encoder->Push();

    m_encoder->Add(Hi, Hi, T0);
    m_encoder->Push();
}

void EmitPass::EmitPtrToPair(GenIntrinsicInst *GII, const SSource Sources[1], const DstModifier &DstMod) {
    Value *L, *H;
    std::tie(L, H) = getPairOutput(GII);
    CVariable *Lo = L ? GetSymbol(L) : nullptr;
    CVariable *Hi = H ? GetSymbol(H) : nullptr;
    assert(Lo == m_destination || Hi == m_destination);

    CVariable *Src = GetSrcVariable(Sources[0]);
    Src = m_currShader->BitCast(Src, m_destination->GetType());

    unsigned AS = Sources[0].value->getType()->getPointerAddressSpace();
    bool isPtr32 = m_currShader->GetContext()->getRegisterPointerSizeInBits(AS) == 32;

    if (Lo) {
        if (isPtr32) {
            m_encoder->Cast(Lo, Src);
            m_encoder->Push();
        } else {
            m_encoder->SetSrcRegion(0, 2, 1, 0);
            m_encoder->SetSrcSubReg(0, 0);
            m_encoder->Copy(Lo, Src);
            m_encoder->Push();
        }
    }

    if (Hi) {
        if (isPtr32) {
            Src = m_currShader->ImmToVariable(0, m_destination->GetType());
            m_encoder->Cast(Hi, Src);
            m_encoder->Push();
        } else {
            m_encoder->SetSrcRegion(0, 2, 1, 0);
            m_encoder->SetSrcSubReg(0, 1);
            m_encoder->Copy(Hi, Src);
            m_encoder->Push();
        }
    }
}


void EmitPass::EmitSIToFPZExt(const SSource &source, const DstModifier& dstMod) {
    CVariable *flag = GetSrcVariable(source);
    CVariable *one = m_currShader->ImmToVariable(getFPOne(m_destination->GetType()), m_destination->GetType());
    CVariable *zero = m_currShader->ImmToVariable(0, m_destination->GetType());
    m_encoder->SetDstModifier(dstMod);
    m_encoder->Select(flag, m_destination, one, zero);
    m_encoder->Push();
}

void EmitPass::emitCtlz(const SSource& source)
{
     // This does not go through the standard EmitAluIntrinsic pass because
     // that creates a redundant SetP due to an unused i1 literal.
     CVariable* src = GetSrcVariable(source);
     src = m_currShader->BitCast( src, GetUnsignedType( src->GetType() ) );
     CVariable* dst = m_currShader->BitCast( m_destination, GetUnsignedType( m_destination->GetType() ) );
     SetSourceModifiers(0, source);
     m_encoder->Ctlz(dst, src);
     m_encoder->Push();
}

void EmitPass::emitVMESendIME2(GenIntrinsicInst* inst) {
  CVariable* inputVar = GetSymbol(inst->getArgOperand(0));
  CVariable* srcImgBTI = GetSymbol(inst->getArgOperand(1));
  CVariable* refImgBTI = GetSymbol(inst->getArgOperand(2));
  CVariable* bwdRefImgBTI = GetSymbol(inst->getArgOperand(3));
  const COMMON_ISA_VME_STREAM_MODE streamMode = (COMMON_ISA_VME_STREAM_MODE)(cast<ConstantInt>(inst->getArgOperand(4))->getZExtValue());

  const bool isDualRef = refImgBTI->GetImmediateValue() != bwdRefImgBTI->GetImmediateValue();
  // If the BTIs aren't consecutive then we can't do VME.
  if (isDualRef)
  {
      assert(refImgBTI->GetImmediateValue() + 1 == bwdRefImgBTI->GetImmediateValue() && "refImg BTI and bwdRefImg BTI are not consecutive!");
  }

  uint32_t regs2snd = 4 + 2;
  uint32_t regs2rcv = CShader::GetIMEReturnPayloadSize(inst);

  if ((streamMode == VME_STREAM_IN) || (streamMode == VME_STREAM_IN_OUT))
  {
      regs2snd += 2;
      if (isDualRef)
      {
          regs2snd += 2;
      }
  }

  // TODO: this may waste registers. We can allocate payload during evaluation
  //       stage, but that needs to initialize and copy payload.
  //       Need to revisit when VME initial support is done.
  if (inputVar->GetSize() > (regs2snd * SIZE_GRF))
  {
    inputVar = m_currShader->GetNewAlias(inputVar, ISA_TYPE_UD, 0, regs2snd * 8);
  }

  CVariable *outputVar = m_destination;

  if (outputVar->GetSize() > (regs2rcv * SIZE_GRF))
  {
    outputVar = m_currShader->GetNewAlias(outputVar, ISA_TYPE_UD, 0, regs2rcv * 8);
  }

  const uint32_t desc = VMEDescriptor(streamMode, (uint32_t)(srcImgBTI->GetImmediateValue()),
                                      EU_GEN7_5_VME_MESSAGE_IME, regs2snd, regs2rcv);

  CVariable* messDesc = m_currShader->ImmToVariable(desc, ISA_TYPE_UD);

  m_encoder->Send(outputVar, inputVar, EU_MESSAGE_TARGET_SFID_VME, messDesc, false);
  m_encoder->Push();
}

void EmitPass::emitVMESendIME(GenIntrinsicInst* inst) {
    const bool has_bwd_ref_image = inst->getIntrinsicID() == GenISAIntrinsic::GenISA_vmeSendIME2;
    CVariable* outputVar        = GetSymbol(inst->getArgOperand(0));

    CVariable* uniInputVar      = GetSymbol(inst->getArgOperand(1));
    CVariable* imeInputVar      = GetSymbol(inst->getArgOperand(2));

    CVariable* srcImgBTI        = GetSymbol(inst->getArgOperand(3));
    CVariable* refImgBTI        = GetSymbol(inst->getArgOperand(4));
    CVariable* bwdRefImgBTI     = has_bwd_ref_image ? GetSymbol(inst->getArgOperand(5)) : nullptr;
    // If the BTIs aren't consecutive then we can't do VME.
    assert(srcImgBTI->GetImmediateValue() + 1 == refImgBTI->GetImmediateValue() && "srcImg BTI and refImg BTI are not consecutive!");
    if (bwdRefImgBTI != nullptr) {
      assert(srcImgBTI->GetImmediateValue() + 2 == bwdRefImgBTI->GetImmediateValue() && "srcImg BTI and bwdRefImg BTI are not consecutive!");
    }

    uint rest_opnd_idx_base = has_bwd_ref_image ? 6 : 5;

    CVariable* ref0Var = GetSymbol(inst->getArgOperand(rest_opnd_idx_base));
    CVariable* ref1Var = GetSymbol(inst->getArgOperand(rest_opnd_idx_base + 1));
    CVariable* costCenterVar = GetSymbol(inst->getArgOperand(rest_opnd_idx_base + 2));

    // Those are raw operands, thus make sure they are GRF-aligned
    ref0Var = ReAlignUniformVariable(ref0Var, EALIGN_GRF);
    ref1Var = ReAlignUniformVariable(ref1Var, EALIGN_GRF);

    // costCenterVar needs to be 1 GRF. If it is uniform, extend it to 1 GRF [bdw+]
    if (costCenterVar->IsUniform())
    {
        VISA_Type costVisaTy = costCenterVar->GetType();
        assert(SIZE_DWORD == CEncoder::GetCISADataTypeSize(costVisaTy) &&
            "VME IME's cost center var has wrong type!");
        CVariable* newVar = m_currShader->GetNewVariable(8, ISA_TYPE_UD, EALIGN_GRF);

        m_encoder->SetNoMask();
        m_encoder->SetSimdSize(SIMDMode::SIMD8);
        m_encoder->Copy(newVar, costCenterVar);
        m_encoder->Push();

        costCenterVar = newVar;
    }

    unsigned char streamMode        = VME_STREAM_DISABLE;
    unsigned char searchControlMode = VME_SEARCH_SINGLE_REF_SINGLE_REC_SINGLE_START;

    // Force write the costCenter here.  I'd like to have uniInputVar setup before calling
    // emitVMESendIME so we don't burn movs each time we call this but CM uses it for now.
    // Fix later.
    {
        CVariable* uniAlias = m_currShader->GetNewAlias(uniInputVar, ISA_TYPE_UD, 3 * SIZE_GRF, 8);
        m_encoder->SetNoMask();
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        m_encoder->SetSimdSize(SIMDMode::SIMD8);
        m_encoder->Copy(uniAlias, costCenterVar);
        m_encoder->Push();
    }

    m_encoder->SetNoMask();
    m_encoder->SendVmeIme(srcImgBTI,
                          streamMode,
                          searchControlMode,
                          uniInputVar,
                          imeInputVar,
                          ref0Var,
                          ref1Var,
                          costCenterVar,
                          outputVar);
    m_encoder->Push();
    return;
}

void EmitPass::emitVMESendFBR(GenIntrinsicInst* inst) {
    CVariable* outputVar            = GetSymbol(inst->getArgOperand(0));

    CVariable* uniInputVar          = GetSymbol(inst->getArgOperand(1));
    CVariable* fbrInputVar          = GetSymbol(inst->getArgOperand(2));

    CVariable* srcImgBTI            = GetSymbol(inst->getArgOperand(3));
    CVariable* refImgBTI            = GetSymbol(inst->getArgOperand(4));
    // If the BTIs aren't consecutive then we can't do VME.
    assert(srcImgBTI->GetImmediateValue() + 1 == refImgBTI->GetImmediateValue() && "srcImg BTI and refImg BTI are not consecutive!");

    const uint rest_opnd_idx_base = 5;
    CVariable* FBRMbModeVar = GetSymbol(inst->getArgOperand(rest_opnd_idx_base));
    CVariable* FBRSubMbShapeVar = GetSymbol(inst->getArgOperand(rest_opnd_idx_base + 1));
    CVariable* FBRSubPredModeVar = GetSymbol(inst->getArgOperand(rest_opnd_idx_base + 2));

    m_encoder->SendVmeFbr(srcImgBTI, uniInputVar, fbrInputVar, FBRMbModeVar, FBRSubMbShapeVar, FBRSubPredModeVar, outputVar);
    m_encoder->Push();
    return;
}

void EmitPass::emitVMESendFBR2(GenIntrinsicInst* inst) {
  CVariable* inputVar = GetSymbol(inst->getArgOperand(0));
  CVariable* srcImgBTI = GetSymbol(inst->getArgOperand(1));
  CVariable* refImgBTI = GetSymbol(inst->getArgOperand(2));
  CVariable* bwdRefImgBTI = GetSymbol(inst->getArgOperand(3));

  const bool isDualRef = refImgBTI->GetImmediateValue() != bwdRefImgBTI->GetImmediateValue();
  // If the BTIs aren't consecutive then we can't do VME.
  if (isDualRef)
  {
      assert(refImgBTI->GetImmediateValue() + 1 == bwdRefImgBTI->GetImmediateValue() && "refImg BTI and bwdRefImg BTI are not consecutive!");
  }

  const uint32_t regs2rcv = (7 + 0), regs2snd = (4 + 4);
  const uint32_t desc = VMEDescriptor(VME_STREAM_DISABLE, (uint32_t)(srcImgBTI->GetImmediateValue()),
                                      EU_GEN7_5_VME_MESSAGE_FBR, regs2snd, regs2rcv);

  CVariable* messDesc = m_currShader->ImmToVariable(desc, ISA_TYPE_UD);

  CVariable *outputVar = m_destination;

  if (outputVar->GetSize() > (regs2rcv * SIZE_GRF))
  {
    outputVar = m_currShader->GetNewAlias(outputVar, ISA_TYPE_UD, 0, regs2rcv * 8);
  }

  m_encoder->Send(outputVar, inputVar, EU_MESSAGE_TARGET_SFID_CRE, messDesc, false);
  m_encoder->Push();

  return;
}

void EmitPass::emitVMESendSIC(GenIntrinsicInst* inst)
{
    CVariable* outputVar   = GetSymbol(inst->getArgOperand(0));
    CVariable* uniInputVar = GetSymbol(inst->getArgOperand(1));
    CVariable* sicInputVar = GetSymbol(inst->getArgOperand(2));
    CVariable* srcImgBTI   = GetSymbol(inst->getArgOperand(3));
    CVariable* ref0ImgBTI  = GetSymbol(inst->getArgOperand(4));
    CVariable* ref1ImgBTI  = GetSymbol(inst->getArgOperand(5));
    // If the BTIs aren't consecutive then we can't do VME.
    assert(srcImgBTI->GetImmediateValue() + 1 == ref0ImgBTI->GetImmediateValue() && "srcImg BTI and ref0Img BTI are not consecutive!");
    // In the non-bidirectional case, we just pass the same reference image into the
    // forward and backward slots.
    if (ref0ImgBTI->GetImmediateValue() != ref1ImgBTI->GetImmediateValue())
    {
        assert(ref0ImgBTI->GetImmediateValue() + 1 == ref1ImgBTI->GetImmediateValue() && "ref0Img BTI and ref1Img BTI are not consecutive!");
    }

    m_encoder->SendVmeSic(srcImgBTI, uniInputVar, sicInputVar, outputVar);
    m_encoder->Push();
}

void EmitPass::emitVMESendSIC2(GenIntrinsicInst* inst)
{
  CVariable* inputVar = GetSymbol(inst->getArgOperand(0));
  CVariable* srcImgBTI = GetSymbol(inst->getArgOperand(1));
  CVariable* fwdRefImgBTI = GetSymbol(inst->getArgOperand(2));
  CVariable* bwdRefImgBTI = GetSymbol(inst->getArgOperand(3));

  const bool isDualRef = fwdRefImgBTI->GetImmediateValue() != bwdRefImgBTI->GetImmediateValue();
  // If the BTIs aren't consecutive then we can't do VME.
  if (isDualRef)
  {
      assert(fwdRefImgBTI->GetImmediateValue() + 1 == bwdRefImgBTI->GetImmediateValue() && "refImg BTI and bwdRefImg BTI are not consecutive!");
  }

  // If the BTIs aren't consecutive then we can't do VME. And this only applies to case
  // when either fwdRefImg or bwdRefImg is presented.
  if (srcImgBTI->GetImmediateValue() != fwdRefImgBTI->GetImmediateValue()) {
    assert(srcImgBTI->GetImmediateValue() + 1 == fwdRefImgBTI->GetImmediateValue() && "srcImg BTI and refImg BTI are not consecutive!");
    if (fwdRefImgBTI->GetImmediateValue() != bwdRefImgBTI->GetImmediateValue())
      assert(srcImgBTI->GetImmediateValue() + 2 == bwdRefImgBTI->GetImmediateValue() && "srcImg BTI and bwdRefImg BTI are not consecutive!");
  }

  const uint32_t regs2rcv = (7 + 0), regs2snd = (4 + 4);
  const uint32_t desc = VMEDescriptor(VME_STREAM_DISABLE, (uint32_t)(srcImgBTI->GetImmediateValue()),
                                      EU_GEN7_5_VME_MESSAGE_SIC, regs2snd, regs2rcv);

  CVariable* messDesc = m_currShader->ImmToVariable(desc, ISA_TYPE_UD);

  CVariable *outputVar = m_destination;

  if (outputVar->GetSize() > (regs2rcv * SIZE_GRF))
  {
    outputVar = m_currShader->GetNewAlias(outputVar, ISA_TYPE_UD, 0, regs2rcv * 8);
  }

  m_encoder->Send(outputVar, inputVar, EU_MESSAGE_TARGET_SFID_CRE, messDesc, false);
  m_encoder->Push();

  return;
}

void EmitPass::emitCreateMessagePhases(GenIntrinsicInst* inst) {
    assert((m_destination->GetType() == ISA_TYPE_UD || m_destination->GetType() == ISA_TYPE_D)  && "Destination type is expected to be UD or D!");
    assert(isa<ConstantInt>(inst->getArgOperand(0)) && "Num phases expected to be const!");
    unsigned int numPhases = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(0))->getZExtValue());

    uint16_t     numSimdLanes       = numLanes(m_SimdMode);
    unsigned int numWideSimdIters   = numPhases * 8 / numSimdLanes;
    unsigned int remSimd8Iters      = (numPhases * 8 % numSimdLanes) / 8;

    // Zero as many message phases as possible using the widest SIMD
    for (unsigned int i = 0; i < numWideSimdIters; ++i) {
        CVariable* messagePhase = m_currShader->GetNewAlias(m_destination, ISA_TYPE_UD, i * numSimdLanes * SIZE_DWORD, numSimdLanes);

        m_encoder->SetNoMask();
        m_encoder->SetSimdSize(m_SimdMode);
        m_encoder->Copy(messagePhase, m_currShader->ImmToVariable(0, ISA_TYPE_UD));
        m_encoder->Push();
    }

    // Zero the remaining message phases using SIMD8
    for (unsigned int i = 0; i < remSimd8Iters; ++i) {
        CVariable* messagePhase = m_currShader->GetNewAlias(m_destination, ISA_TYPE_UD, (i * 8 + numWideSimdIters * numSimdLanes) * SIZE_DWORD, numLanes(SIMDMode::SIMD8));

        m_encoder->SetNoMask();
        m_encoder->SetSimdSize(SIMDMode::SIMD8);
        m_encoder->Copy(messagePhase, m_currShader->ImmToVariable(0, ISA_TYPE_UD));
        m_encoder->Push();
    }
}

static VISA_Type GetTypeFromSize(unsigned size)
{
    switch (size)
    {
    case 1:
        return ISA_TYPE_UB;
    case 2:
        return ISA_TYPE_UW;
    case 4:
        return ISA_TYPE_UD;
    case 8:
        return ISA_TYPE_UQ;
    default:
        assert(0 && "unknown size");
        return ISA_TYPE_UD;
    }
}

void EmitPass::emitSimdMediaRegionCopy(llvm::GenIntrinsicInst* inst)
{
    CVariable *pDst      = GetSymbol(inst->getArgOperand(0));
    unsigned dbyteoffset = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(1))->getZExtValue());
    unsigned dstride     = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(2))->getZExtValue());
    unsigned dnumelem    = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(3))->getZExtValue());
    CVariable *pSrc      = GetSymbol(inst->getArgOperand(4));
    Value *sbyteoffset   = inst->getArgOperand(5);
    unsigned vstride     = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(6))->getZExtValue());
    unsigned width       = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(7))->getZExtValue());
    unsigned hstride     = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(8))->getZExtValue());
    unsigned typesize    = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(9))->getZExtValue());
    unsigned execsize    = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(10))->getZExtValue());
    unsigned snumelem    = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(11))->getZExtValue());

    CVariable* pDstOffset = m_currShader->GetNewAlias(pDst, GetTypeFromSize(typesize), (uint16_t)dbyteoffset, (uint16_t)dnumelem);

    auto setup = [&]()
    {
        m_encoder->SetSimdSize(lanesToSIMDMode(execsize));
        m_encoder->SetNoMask();
        m_encoder->SetDstRegion(dstride);
        m_encoder->SetSrcRegion(0, vstride, width, hstride);
    };

    if (isa<ConstantInt>(sbyteoffset))
    {
        CVariable* pSrcOffset = m_currShader->GetNewAlias(
            pSrc,
            GetTypeFromSize(typesize),
            int_cast<uint16_t>(cast<ConstantInt>(sbyteoffset)->getZExtValue()),
            (uint16_t)snumelem);

        setup();
        m_encoder->Copy(pDstOffset, pSrcOffset);
        m_encoder->Push();
    }
    else
    {
        CVariable* pSrcOffset = m_currShader->GetNewAddressVariable(
            1,
            GetTypeFromSize(typesize),
            true,
            false);

        m_encoder->AddrAdd(pSrcOffset, pSrc, m_currShader->BitCast(GetSymbol(sbyteoffset), ISA_TYPE_UW));
        setup();
        m_encoder->Copy(pDstOffset, pSrcOffset);
        m_encoder->Push();
    }
}

void EmitPass::emitExtractMVAndSAD(llvm::GenIntrinsicInst* inst)
{
    CVariable *pMV        = GetSymbol(inst->getArgOperand(0));
    CVariable *pSAD       = GetSymbol(inst->getArgOperand(1));
    CVariable *pResult    = GetSymbol(inst->getArgOperand(2));
    CVariable *pBlockType = GetSymbol(inst->getArgOperand(3));

    // W5.0 - W5.7 from Return Data Message Phases (InterDistortion)
    CVariable* pDist = m_currShader->GetNewAlias(pResult, ISA_TYPE_UW, 5 * SIZE_GRF, 16);
    CVariable* pSADAlias = m_currShader->GetNewAlias(pSAD, ISA_TYPE_UW, 0, 16);

    CVariable* pFlag = m_currShader->GetNewVariable(
        16,
        ISA_TYPE_BOOL,
        IGC::EALIGN_GRF);

    auto EmitCmp = [&](unsigned imm)
    {
        m_encoder->SetSimdSize(SIMDMode::SIMD16);
        m_encoder->SetNoMask();
        m_encoder->Cmp(EPREDICATE_EQ, pFlag, pBlockType, m_currShader->ImmToVariable(imm, ISA_TYPE_UD));
        m_encoder->Push();
    };

    // block type == 0 (16x16)
    EmitCmp(0);


    // Only one SAD, replicate it across.
    // (+f1.1) mov (16) r16.0<1>:uw r73.0<0;1,0>:uw { Align1, H1, NoMask }
    m_encoder->SetPredicate(pFlag);
    m_encoder->SetNoMask();
    m_encoder->SetSrcRegion(0, 0, 1, 0);
    m_encoder->SetSimdSize(SIMDMode::SIMD16);
    m_encoder->Copy(pSADAlias, pDist);
    m_encoder->Push();

    // block type == 1 (8x8)
    EmitCmp(1);

    // 4 SADs, copy each one 4 times.
    // (+f1.1) mov(4) r16.12<1>:uw r73.12<0;1,0>:uw { Align1, Q1, NoMask }
    // (+f1.1) mov(4) r16.8<1>:uw r73.8<0;1,0>:uw { Align1, Q1, NoMask }
    // (+f1.1) mov(4) r16.4<1>:uw r73.4<0;1,0>:uw { Align1, Q1, NoMask }
    // (+f1.1) mov(4) r16.0<1>:uw r73.0<0;1,0>:uw { Align1, Q1, NoMask }
    for (int i = 0; i < 4; i++)
    {
        m_encoder->SetPredicate(pFlag);
        m_encoder->SetNoMask();
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        m_encoder->SetSimdSize(SIMDMode::SIMD4);
        CVariable* pDistOffset = m_currShader->GetNewAlias(pDist, ISA_TYPE_UW, i * 8, 4);
        CVariable* pSADOffset = m_currShader->GetNewAlias(pSADAlias, ISA_TYPE_UW, i * 8, 4);
        m_encoder->Copy(pSADOffset, pDistOffset);
        m_encoder->Push();
    }

    // block type == 2 (4x4)
    EmitCmp(2);

    // All 16 SADs present, copy othem over.
    // (+f1.1) mov (16) r16.0<1>:uw r73.0<8;8,1>:uw {Align1, H1, NoMask}
    m_encoder->SetPredicate(pFlag);
    m_encoder->SetNoMask();
    m_encoder->SetSimdSize(SIMDMode::SIMD16);
    m_encoder->Copy(pSADAlias, pDist);
    m_encoder->Push();

    // Copy over MVs
    for (int i = 0; i < 2; i++)
    {
        CVariable* pResultOffset = m_currShader->GetNewAlias(pResult, ISA_TYPE_UD,
            (1 * SIZE_GRF) + (2 * i * SIZE_GRF),
            16);
        CVariable* pMVOffset = m_currShader->GetNewAlias(pMV, ISA_TYPE_UD,
            2 * i * SIZE_GRF,
            16);
        m_encoder->SetNoMask();
        m_encoder->SetSimdSize(SIMDMode::SIMD16);
        m_encoder->Copy(pMVOffset, pResultOffset);
        m_encoder->Push();
    }
}

void EmitPass::emitCmpSADs(llvm::GenIntrinsicInst* inst)
{
    // When called, this builtin will compare two SAD values
    // and take the minimum of the two.  The minimum MV associated
    // with the minimum SAD is also selected.
    CVariable *pMVCurr  = GetSymbol(inst->getArgOperand(0));
    CVariable *pSADCurr = GetSymbol(inst->getArgOperand(1));
    CVariable *pMVMin   = GetSymbol(inst->getArgOperand(2));
    CVariable *pSADMin  = GetSymbol(inst->getArgOperand(3));

    CVariable* pFlag = m_currShader->GetNewVariable(
        16,
        ISA_TYPE_BOOL,
        IGC::EALIGN_GRF);

    CVariable* pSADCurrAlias = m_currShader->GetNewAlias(pSADCurr, ISA_TYPE_UW, 0, 16);
    CVariable* pSADMinAlias = m_currShader->GetNewAlias(pSADMin, ISA_TYPE_UW, 0, 16);

    m_encoder->SetNoMask();
    m_encoder->SetSimdSize(SIMDMode::SIMD16);
    m_encoder->Cmp(EPREDICATE_LT, pFlag, pSADCurrAlias, pSADMinAlias);
    m_encoder->Push();

    // Collect the SADs
    m_encoder->SetNoMask();
    m_encoder->SetSimdSize(SIMDMode::SIMD16);
    m_encoder->Select(pFlag, pSADMinAlias, pSADCurrAlias, pSADMinAlias);
    m_encoder->Push();

    // Collect the MVs
    if (m_currShader->m_Platform->hasNo64BitInst()) {
        m_encoder->SetNoMask();
        m_encoder->SetSimdSize(SIMDMode::SIMD16);
        m_encoder->Select(pFlag, pMVMin, pMVCurr, pMVMin);
        m_encoder->Push();

        m_encoder->SetNoMask();
        m_encoder->SetSimdSize(SIMDMode::SIMD16);
        m_encoder->SetSrcSubVar(0, 2);
        m_encoder->SetSrcSubVar(1, 2);
        m_encoder->SetDstSubVar(2);
        m_encoder->Select(pFlag, pMVMin, pMVCurr, pMVMin);
        m_encoder->Push();
    } else {
        CVariable* pMVCurrAlias = m_currShader->GetNewAlias(pMVCurr, ISA_TYPE_UQ, 0, 16);
        CVariable* pMVMinAlias = m_currShader->GetNewAlias(pMVMin, ISA_TYPE_UQ, 0, 16);

        m_encoder->SetNoMask();
        m_encoder->SetSimdSize(SIMDMode::SIMD16);
        m_encoder->Select(pFlag, pMVMinAlias, pMVCurrAlias, pMVMinAlias);
        m_encoder->Push();
    }
}

static bool SameVar(CVariable *A, CVariable *B)
{
    A = (A->GetAlias() && A->GetAliasOffset() == 0) ? A->GetAlias() : A;
    B = (B->GetAlias() && B->GetAliasOffset() == 0) ? B->GetAlias() : B;

    return A == B;
}

void EmitPass::emitSimdSetMessagePhase(llvm::GenIntrinsicInst* inst) {
  CVariable* messagePhases = GetSymbol(inst->getArgOperand(0));
  const uint32_t phaseIndex = int_cast<uint32_t>(cast<ConstantInt>(inst->getArgOperand(1))->getZExtValue());
  const uint32_t numPhases  = int_cast<uint32_t>(cast<ConstantInt>(inst->getArgOperand(2))->getZExtValue());
  const uint32_t dstSubReg  = int_cast<uint32_t>(cast<ConstantInt>(inst->getArgOperand(3))->getZExtValue());
  const uint32_t numLanesPerPhase = int_cast<uint32_t>(cast<ConstantInt>(inst->getArgOperand(4))->getZExtValue());
  const SIMDMode simdMode = lanesToSIMDMode(numLanesPerPhase);
  Value *value = inst->getArgOperand(5);
  const uint16_t eltSizeInBytes = (uint16_t)m_DL->getTypeSizeInBits(value->getType()) / 8;
  const uint16_t numEltsPerPhase = SIZE_GRF / eltSizeInBytes;
  const VISA_Type type = GetTypeFromSize(eltSizeInBytes);

  CVariable *val = GetSymbol(value);

  if (!SameVar(m_destination, messagePhases))
  {
      emitCopyAll(m_destination, messagePhases, inst->getArgOperand(0)->getType());
  }

  for (uint32_t i = 0; i < numPhases; ++i) {
    CVariable* src = val->IsUniform() ? val : m_currShader->GetNewAlias(val, type, i * SIZE_GRF, numEltsPerPhase);
    CVariable* dst = m_currShader->GetNewAlias(m_destination, type, (i + phaseIndex) * SIZE_GRF, numEltsPerPhase);

    m_encoder->SetNoMask();
    m_encoder->SetSimdSize(simdMode);
    m_encoder->SetDstSubReg(dstSubReg);
    if (!val->IsUniform())
        m_encoder->SetSrcRegion(0, 0, numEltsPerPhase, 1);
    m_encoder->Copy(dst, src);
    m_encoder->Push();
  }

  return;
}

void EmitPass::emitBroadcastMessagePhase(llvm::GenIntrinsicInst* inst) {
  const uint16_t eltSizeInBytes = (uint16_t)m_DL->getTypeSizeInBits(inst->getType()) / 8;
  const uint32_t width = int_cast<uint32_t>(cast<ConstantInt>(inst->getArgOperand(3))->getZExtValue());
  emitGetMessagePhaseType(inst, GetTypeFromSize(eltSizeInBytes), width);
}

void EmitPass::emitSimdGetMessagePhase(llvm::GenIntrinsicInst* inst) {
  CVariable* messagePhases = GetSymbol(inst->getArgOperand(0));
  const uint32_t phaseIndex = int_cast<uint32_t>(cast<ConstantInt>(inst->getArgOperand(1))->getZExtValue());
  const uint32_t numPhases  = int_cast<uint32_t>(cast<ConstantInt>(inst->getArgOperand(2))->getZExtValue());
  const uint16_t eltSizeInBytes = (uint16_t)m_DL->getTypeSizeInBits(inst->getType()) / 8;
  const uint16_t numEltsPerPhase = SIZE_GRF / eltSizeInBytes;
  const VISA_Type type = GetTypeFromSize(eltSizeInBytes);
  SIMDMode simdMode = SIMDMode::UNKNOWN;

  if (eltSizeInBytes == 8) {
    simdMode = SIMDMode::SIMD4;
  }
  else if (eltSizeInBytes == 4) {
    simdMode = SIMDMode::SIMD8;
  }
  else if (eltSizeInBytes == 2) {
    simdMode = SIMDMode::SIMD16;
  }
  else {
    assert(false && "Unhandled data type");
  }

  for (uint32_t i = 0; i < numPhases; ++i) {
    CVariable* src = m_currShader->GetNewAlias(messagePhases, type, (i + phaseIndex) * SIZE_GRF, numEltsPerPhase);
    CVariable* dst = m_currShader->GetNewAlias(m_destination, type, i * SIZE_GRF, numEltsPerPhase);

    m_encoder->SetNoMask();
    m_encoder->SetSimdSize(simdMode);
    m_encoder->SetSrcRegion(0, 0, numEltsPerPhase, 1);
    m_encoder->Copy(dst, src);
    m_encoder->Push();
  }

  return;
}

void EmitPass::emitGetMessagePhaseType(llvm::GenIntrinsicInst* inst, VISA_Type type, uint32_t width) {
    CVariable* messagePhases = GetSymbol(inst->getArgOperand(0));
    unsigned int phaseIndex = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(1))->getZExtValue());
    unsigned int phaseSubindex = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(2))->getZExtValue());

    assert((phaseIndex * SIZE_GRF < messagePhases->GetSize()) && "out of bounds!");

    CVariable* messagePhaseElem = m_currShader->GetNewAlias(messagePhases, type, phaseIndex * SIZE_GRF, 1);

    m_encoder->SetNoMask();
    m_encoder->SetSrcRegion(0, 0, width, 1);
    m_encoder->SetSrcSubReg(0, phaseSubindex);

    m_encoder->Copy(m_destination, messagePhaseElem);
    m_encoder->Push();
}

void EmitPass::emitGetMessagePhaseX(llvm::GenIntrinsicInst* inst) {
    unsigned size = inst->getType()->getScalarSizeInBits() / 8;
    emitGetMessagePhaseType(inst, GetTypeFromSize(size), /* width */ 1);
}

void EmitPass::emitSetMessagePhaseType_legacy(GenIntrinsicInst* inst, VISA_Type type)
{
    CVariable* messagePhases = GetSymbol(inst->getArgOperand(0));
    unsigned int phaseIndex = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(1))->getZExtValue());
    unsigned int phaseSubindex = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(2))->getZExtValue());
    CVariable* val = GetSymbol(inst->getArgOperand(3));

    CVariable* messagePhaseElem = m_currShader->GetNewAlias(messagePhases, type, phaseIndex * SIZE_GRF, 1);
    m_encoder->SetSimdSize(SIMDMode::SIMD1);
    m_encoder->SetNoMask();
    m_encoder->SetSrcRegion(0, 0, 1, 0);
    m_encoder->SetDstSubReg(phaseSubindex);
    m_encoder->Copy(messagePhaseElem, val);
    m_encoder->Push();
}

void EmitPass::emitSetMessagePhaseType(GenIntrinsicInst* inst, VISA_Type type) {
    CVariable* messagePhases = GetSymbol(inst->getArgOperand(0));
    unsigned int phaseIndex = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(1))->getZExtValue());
    unsigned int phaseSubindex = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(2))->getZExtValue());
    CVariable* val = GetSymbol(inst->getArgOperand(3));

    assert((phaseIndex * SIZE_GRF < messagePhases->GetSize()) && "out of bounds!");

    if (!SameVar(m_destination, messagePhases))
    {
        emitCopyAll(m_destination, messagePhases, inst->getArgOperand(0)->getType());
    }

    CVariable* messagePhaseElem = m_currShader->GetNewAlias(m_destination, type, phaseIndex * SIZE_GRF, 1);
    m_encoder->SetSimdSize(SIMDMode::SIMD1);
    m_encoder->SetNoMask();
    m_encoder->SetSrcRegion(0, 0, 1, 0);
    m_encoder->SetDstSubReg(phaseSubindex);
    m_encoder->Copy(messagePhaseElem, val);
    m_encoder->Push();
}

void EmitPass::emitSetMessagePhaseX_legacy(GenIntrinsicInst* inst)
{
    Type *pTy = inst->getArgOperand(inst->getNumArgOperands() - 1)->getType();
    unsigned size = pTy->getScalarSizeInBits() / 8;
    emitSetMessagePhaseType_legacy(inst, GetTypeFromSize(size));
}

void EmitPass::emitSetMessagePhaseX(GenIntrinsicInst* inst) {
    Type *pTy = inst->getArgOperand(inst->getNumArgOperands() - 1)->getType();
    unsigned size = pTy->getScalarSizeInBits() / 8;
    emitSetMessagePhaseType(inst, GetTypeFromSize(size));
}

void EmitPass::emitGetMessagePhase(llvm::GenIntrinsicInst* inst) {
    if (isa<UndefValue>(inst->getArgOperand(0)))
      return;

    CVariable* messagePhases =  GetSymbol(inst->getArgOperand(0));
    unsigned int phaseIndex = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(1))->getZExtValue());

    assert((phaseIndex * SIZE_GRF < messagePhases->GetSize()) && "out of bounds!");

    CVariable* messagePhase = m_currShader->GetNewAlias(messagePhases, ISA_TYPE_UD, phaseIndex * SIZE_GRF, 8);
    m_encoder->SetSimdSize(SIMDMode::SIMD8);
    m_encoder->SetNoMask();
    m_encoder->Copy(m_destination, messagePhase);
    m_encoder->Push();
}

void EmitPass::emitSetMessagePhase_legacy(llvm::GenIntrinsicInst* inst)
{
    CVariable* messagePhases = GetSymbol(inst->getArgOperand(0));
    unsigned int phaseIndex = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(1))->getZExtValue());
    CVariable* val = GetSymbol(inst->getArgOperand(2));

    CVariable* messagePhase = m_currShader->GetNewAlias(messagePhases, ISA_TYPE_UD, phaseIndex * SIZE_GRF, 8);
    m_encoder->SetSimdSize(SIMDMode::SIMD8);
    m_encoder->SetNoMask();
    m_encoder->Copy(messagePhase, val);
    m_encoder->Push();
}

void EmitPass::emitSetMessagePhase(llvm::GenIntrinsicInst* inst) {
    CVariable* messagePhases = GetSymbol(inst->getArgOperand(0));
    unsigned int phaseIndex = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(1))->getZExtValue());
    CVariable* val = GetSymbol(inst->getArgOperand(2));

    assert((phaseIndex * SIZE_GRF < messagePhases->GetSize()) && "out of bounds!");

    if (!SameVar(m_destination, messagePhases))
    {
        emitCopyAll(m_destination, messagePhases, inst->getArgOperand(0)->getType());
    }

    CVariable* messagePhase = m_currShader->GetNewAlias(m_destination, ISA_TYPE_UD, phaseIndex * SIZE_GRF, 8);
    m_encoder->SetSimdSize(SIMDMode::SIMD8);
    m_encoder->SetNoMask();
    m_encoder->Copy(messagePhase, val);
    m_encoder->Push();
}

// VA
void EmitPass::emitVideoAnalyticSLM( llvm::GenIntrinsicInst* inst, const DWORD responseLen )
{
    int argNum = 0;
    CVariable* outputVar = GetSymbol( inst->getArgOperand( argNum++ ) );
    CVariable* coords = GetSymbol( inst->getArgOperand( argNum++ ) );
    CVariable* size = NULL;

    assert(!(m_currShader->m_dispatchSize == SIMDMode::SIMD32 && m_encoder->IsSecondHalf()) && "VA Intrinsics are simd independent");
    GenISAIntrinsic::ID id = inst->getIntrinsicID();
    if( id == GenISAIntrinsic::GenISA_vaCentroid ||
        id == GenISAIntrinsic::GenISA_vaBoolCentroid ||
        id == GenISAIntrinsic::GenISA_vaBoolSum )
    {
        size = GetSymbol( inst->getArgOperand( argNum++ ) );
    }

    CVariable* srcImg = GetSymbol( inst->getArgOperand( argNum++ ) );

    // So far we support only one VA function per kernel, and other sample
    // messages are not supported when there is VA function within the kernel.
    // So, for now it should be fine to always use sampler 0 for VA functions.
    DWORD samplerIndex = 0;
    CVariable* sampler = m_currShader->ImmToVariable( samplerIndex, ISA_TYPE_UD );

    uint16_t newNumElems = int_cast<uint16_t>(responseLen * SIZE_GRF / SIZE_DWORD);

    CVariable* vaResult = m_currShader->GetNewVariable(
        newNumElems,
        ISA_TYPE_UD,
        outputVar->GetAlign(),
        false );

    if( inst->getIntrinsicID( ) == GenISAIntrinsic::GenISA_vaConvolve )
    {
        CVariable* convResult = m_currShader->GetNewAlias(
            vaResult,
            ISA_TYPE_UW,
            0,
            newNumElems * 2 );

        m_encoder->SendVideoAnalytic( inst, convResult, coords, size, srcImg, sampler );
    }
    else
    {
        m_encoder->SendVideoAnalytic( inst, vaResult, coords, size, srcImg, sampler );
    }
    m_encoder->Push();

    // Data port write msg header:
    DWORD msgLen = 2;
    DWORD resLen = 0;
    bool headerPresent = false;
    bool endOfThread = false;
    DWORD messageSpecificControl = encodeMessageSpecificControlForReadWrite(
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_UNTYPED_SURFACE_WRITE,
        CHANNEL_MASK_R,
        SIMDMode::SIMD8 );
    bool invalidateAfterReadEnable = false;
    DWORD btiIndex = SLM_BTI;

    DWORD descValue = DataPortWrite(
        msgLen,
        resLen,
        headerPresent,
        endOfThread,
        EU_DATA_PORT_WRITE_MESSAGE_TYPE_UNTYPED_SURFACE_WRITE,
        messageSpecificControl,
        invalidateAfterReadEnable,
        btiIndex );

    DWORD exDescValue = EU_GEN7_5_MESSAGE_TARGET_DATA_PORT_DATA_CACHE_1;

    CVariable* desc = m_currShader->ImmToVariable( descValue, ISA_TYPE_UD );
    CVariable* exdesc = m_currShader->ImmToVariable( exDescValue, ISA_TYPE_UD );

    CVariable* storeMessage = m_currShader->GetNewVariable(
        2 * SIZE_GRF / SIZE_DWORD,
        ISA_TYPE_UD,
        outputVar->GetAlign(),
        false );

    m_encoder->SetSimdSize( SIMDMode::SIMD8 );
    m_encoder->SetNoMask( );
    m_encoder->Cast( storeMessage, m_currShader->ImmToVariable( 0x76543210, ISA_TYPE_V ) );
    m_encoder->Shl( storeMessage, storeMessage, m_currShader->ImmToVariable( 2, ISA_TYPE_UD ) );
    m_encoder->Push();

    for( DWORD i = 0; i < responseLen; i++ )
    {
        if( i > 0 )
        {
            m_encoder->SetSimdSize( SIMDMode::SIMD8 );
            m_encoder->SetNoMask( );
            m_encoder->Add( storeMessage, storeMessage, m_currShader->ImmToVariable( 0x20, ISA_TYPE_UD ) );
            m_encoder->Push();
        }

        m_encoder->SetSimdSize( SIMDMode::SIMD8 );
        m_encoder->SetNoMask( );
        m_encoder->SetDstSubVar( 1 );
        m_encoder->SetSrcSubVar( 0, i );
        m_encoder->Copy( storeMessage, vaResult );

        m_encoder->Send( NULL, storeMessage,
            EU_GEN7_5_MESSAGE_TARGET_DATA_PORT_DATA_CACHE_1, exdesc, desc, false );
        m_encoder->Push();
    }

    return;
}

void EmitPass::emitVideoAnalyticGRF(llvm::GenIntrinsicInst* inst, const DWORD responseLen)
{
    CVariable* dst = m_destination;
    int argNum = 0;
    CVariable* coords = GetSymbol(inst->getArgOperand(argNum++));

    // So far we support only one VA function per kernel, and other sample
    // messages are not supported when there is VA function within the kernel.
    // So, for now it should be fine to always use sampler 0 for VA functions.
    CVariable* sampler = m_currShader->ImmToVariable(0, ISA_TYPE_UD);
    CVariable* srcImg = GetSymbol(inst->getArgOperand(argNum++));

    m_encoder->SendVideoAnalytic(inst, dst, coords, nullptr, srcImg, sampler);
    m_encoder->Push();
}

void EmitPass::BinaryUnary(llvm::Instruction* inst, const SSource source[2], const DstModifier& modifier)
{
    switch(inst->getOpcode())
    {
    case Instruction::FCmp:
    case Instruction::ICmp:
        Cmp(cast<CmpInst>(inst)->getPredicate(), source, modifier);
        break;
    case Instruction::Sub:
    case Instruction::FSub:
        Sub(source, modifier);
        break;
    case Instruction::FDiv:
        FDiv(source, modifier);
        break;
    case Instruction::Xor:
        Xor(source, modifier);
        break;
    case Instruction::Mul:
        Mul(source, modifier);
        break;
    case Instruction::Call:
        EmitAluIntrinsic(cast<CallInst>(inst), source, modifier);
        break;
    default:
        // other instruction don't need special handling
        EmitSimpleAlu(inst, source, modifier);
        break;
    }
}

void EmitPass::Sub(const SSource sources[2], const DstModifier &modifier)
{
    CVariable* src0 = GetSrcVariable(sources[0]);
    CVariable* src1 = GetSrcVariable(sources[1]);
    e_modifier mod1 = CombineModifier(EMOD_NEG, sources[1].mod);

    m_encoder->SetDstModifier(modifier);
    SetSourceModifiers(0, sources[0]);
    SetSourceModifiers(1, sources[1]);
    // override modifier of source 1
    m_encoder->SetSrcModifier(1, mod1);
    m_encoder->Add(m_destination, src0, src1);
    m_encoder->Push();

}

void EmitPass::Mul64(CVariable* dst, CVariable* src[2]) const
{

    // Mul64 does not write to m_destination!

    assert((src[1]->GetType() == ISA_TYPE_Q || src[1]->GetType() == ISA_TYPE_UQ)
               && "Cannot multiply a qword by a non-qword type");

    // The signedness of the hi-part type should be the same as that
    // of the original destination type.
    VISA_Type hiType;
    if (dst->GetType() == ISA_TYPE_Q)
      hiType = ISA_TYPE_D;
    else
      hiType = ISA_TYPE_UD;

    // Figure out what the hi and what the lo part of each source is.
    // For non-uniforms, this requires an unpack.
    CVariable *srcLo[2], *srcHi[2];
    for (int i = 0; i < 2; ++i)
    {
        CVariable* srcAsUD = m_currShader->BitCast(src[i], ISA_TYPE_UD);
        if (src[i]->IsUniform())
        {
            if (src[i]->IsImmediate())
            {
                srcLo[i] = m_currShader->ImmToVariable((uint)src[i]->GetImmediateValue(), ISA_TYPE_UD);
                srcHi[i] = m_currShader->ImmToVariable(src[i]->GetImmediateValue() >> 32, hiType);
            }
            else
            {
                srcLo[i] = m_currShader->GetNewAlias(srcAsUD, ISA_TYPE_UD, 0, 1);
                srcHi[i] = m_currShader->GetNewAlias(srcAsUD, hiType, SIZE_DWORD, 1);
            }
        }
        else
        {
            //TODO: Would it be better for these two to be consecutive?
            srcLo[i] = m_currShader->GetNewVariable(src[i]->GetNumberElement(),
                ISA_TYPE_UD, EALIGN_GRF, false);
            srcHi[i] = m_currShader->GetNewVariable(src[i]->GetNumberElement(),
                hiType, EALIGN_GRF, false);

            m_encoder->SetSrcRegion(0, 2, 1, 0);
            m_encoder->Copy(srcLo[i], srcAsUD);
            m_encoder->Push();

            m_encoder->SetSrcSubReg(0, 1);
            m_encoder->SetSrcRegion(0, 2, 1, 0);
            m_encoder->Copy(srcHi[i], srcAsUD);
            m_encoder->Push();

        }
    }

    //Now, generate the required sequence of multiplies and adds
    TODO("Do not generate intermediate multiplies by constant 0 or 1.");
    TODO("Do smarter pattern matching to look for non-constant zexted/sexted sources.");

    CVariable *dstLo, *dstHi, *dstTemp;
    dstLo = m_currShader->GetNewVariable(dst->GetNumberElement(),
                ISA_TYPE_UD, m_destination->GetAlign(), m_destination->IsUniform());
    dstHi = m_currShader->GetNewVariable(dst->GetNumberElement(),
                hiType, m_destination->GetAlign(), m_destination->IsUniform());
    dstTemp = m_currShader->GetNewVariable(dst->GetNumberElement(),
                hiType, m_destination->GetAlign(), m_destination->IsUniform());


    //
    // Algorithm:
    //   - Break the 64 bit sources into 32bit low/high halves.
    //   - Perform multiplication "by hand"
    //
    //    AB   - src1
    //    CD   - src0
    //   ----
    //     E
    //    F
    //    G
    //   H     - 'H' spills into bit 65 - only needed if overflow detection is required
    // --------
    // dstLow = E
    // dstHigh = F + G + carry

    // E = A * B
    m_encoder->Mul(dstLo, srcLo[0], srcLo[1]);
    m_encoder->Push();

    // Cr = carry(A * B)
    m_encoder->MulH(dstHi, srcLo[0], srcLo[1]);
    m_encoder->Push();

    // F = A * D
    m_encoder->Mul(dstTemp, srcLo[0], srcHi[1]);
    m_encoder->Push();

    // dstHigh = Cr + F
    m_encoder->Add(dstHi, dstHi, dstTemp);
    m_encoder->Push();

    // G = B * C
    m_encoder->Mul(dstTemp, srcHi[0], srcLo[1]);
    m_encoder->Push();

    // dstHigh = (Cr + F) + G
    m_encoder->Add(dstHi, dstHi, dstTemp);
    m_encoder->Push();

    //And now, pack the result
    CVariable* dstAsUD = m_currShader->BitCast(dst, ISA_TYPE_UD);
    m_encoder->SetDstRegion(2);
    m_encoder->Copy(dstAsUD, dstLo);
    m_encoder->Push();

    m_encoder->SetDstRegion(2);
    m_encoder->SetDstSubReg(1);
    m_encoder->Copy(dstAsUD, dstHi);
    m_encoder->Push();
}

void EmitPass::Mul(const SSource sources[2], const DstModifier &modifier)
{
    CVariable* src[2];
    for (int i = 0; i < 2; ++i)
    {
        src[i] = GetSrcVariable(sources[i]);
    }

    // Only i64 muls need special handling, otherwise go back to standard flow
    VISA_Type srcType = src[0]->GetType();
    if (srcType != ISA_TYPE_Q && srcType != ISA_TYPE_UQ)
    {
        Binary(EOPCODE_MUL, sources, modifier);
    }
    else
    {
        Mul64(m_destination, src);
    }
}

void EmitPass::FDiv(const SSource sources[2], const DstModifier& modifier)
{
    if(isOne(sources[0].value))
    {
        Unary(EOPCODE_INV, &sources[1], modifier);
    }
    else
    {
        Binary(EOPCODE_DIV, sources, modifier);
    }
}

static inline bool isConstantAllOnes(const Value *V)
{
    if (const Constant *C = dyn_cast<Constant>(V))
        return C->isAllOnesValue();
    return false;
}

void EmitPass::Xor(const SSource sources[2], const DstModifier& modifier)
{
    if (isConstantAllOnes(sources[0].value))
    {
        Unary(EOPCODE_NOT, &sources[1], modifier);
    }
    else if (isConstantAllOnes(sources[1].value))
    {
        Unary(EOPCODE_NOT, &sources[0], modifier);
    }
    else
    {
        Binary(EOPCODE_XOR, sources, modifier);
    }
}

void EmitPass::Cmp(llvm::CmpInst::Predicate pred, const SSource sources[2], const DstModifier& modifier)
{
    assert(modifier.sat == false && modifier.flag == nullptr);
    e_predicate predicate = GetPredicate(pred);

    CVariable* src0 = GetSrcVariable(sources[0]);
    CVariable* src1 = GetSrcVariable(sources[1]);

    if(IsUnsignedCmp(pred))
    {
        src0 = m_currShader->BitCast(src0, GetUnsignedType(src0->GetType()));
        src1 = m_currShader->BitCast(src1, GetUnsignedType(src1->GetType()));
    }
    else if(IsSignedCmp(pred))
    {
        src0 = m_currShader->BitCast(src0, GetSignedType(src0->GetType()));
        src1 = m_currShader->BitCast(src1, GetSignedType(src1->GetType()));
    }

    CVariable* dst = m_destination;
    if(m_destination->GetType() != ISA_TYPE_BOOL && dst->GetType() != src0->GetType())
    {
        assert(CEncoder::GetCISADataTypeSize(dst->GetType()) == CEncoder::GetCISADataTypeSize(src0->GetType()) &&
            "Cmp to GRF must have the same size for source and destination");
        dst = m_currShader->BitCast( m_destination, src0->GetType() );
    }

    SetSourceModifiers(0, sources[0]);
    SetSourceModifiers(1, sources[1]);
    m_encoder->Cmp(predicate,dst,src0,src1);
    m_encoder->Push();
}

void EmitPass::Frc(const SSource& source, const DstModifier& modifier)
{
    Unary(EOPCODE_FRC, &source, modifier);
}

void EmitPass::Mov(const SSource& source, const DstModifier& modifier)
{
    Unary(EOPCODE_MOV, &source, modifier);
}

void EmitPass::Rsqrt(const SSource& source, const DstModifier& modifier)
{
    Unary(EOPCODE_RSQRT, &source, modifier);
}

void EmitPass::Sqrt(const SSource& source, const DstModifier& modifier)
{
    Unary(EOPCODE_SQRT, &source, modifier);
}

void EmitPass::Mad(const SSource sources[3], const DstModifier& modifier)
{
    Tenary(EOPCODE_MAD, sources, modifier);
}

void EmitPass::Lrp(const SSource sources[3], const DstModifier& modifier)
{
    Tenary(EOPCODE_LRP, sources, modifier);
}

void EmitPass::Pow(const SSource sources[2], const DstModifier& modifier)
{
    Binary(EOPCODE_POW, sources, modifier);
}

void EmitPass::Avg(const SSource sources[2], const DstModifier& modifier)
{
    Binary(EOPCODE_AVG, sources, modifier);
}

void EmitPass::Tenary(e_opcode opCode,  const SSource sources[3], const DstModifier& modifier)
{
    Alu<3>(opCode, sources, modifier);
}

void EmitPass::Binary(e_opcode opCode, const SSource sources[2], const DstModifier& modifier)
{
    Alu<2>(opCode, sources, modifier);
}

void EmitPass::Unary(e_opcode opCode, const SSource sources[1], const DstModifier& modifier)
{
    Alu<1>(opCode, sources, modifier);
}

template<int N>
void EmitPass::Alu(e_opcode opCode, const SSource sources[N], const DstModifier& modifier)
{

    CVariable* srcs[3] = {nullptr, nullptr, nullptr};
    for(uint i=0;i<N;i++)
    {
        bool fromConstantPool = sources[i].fromConstantPool;
        srcs[i] = GetSrcVariable(sources[i], fromConstantPool);
        SetSourceModifiers(i, sources[i]);
    }
    m_encoder->SetDstModifier(modifier);
    m_encoder->GenericAlu(opCode, m_destination, srcs[0], srcs[1], srcs[2]);
    m_encoder->Push();
}

void EmitPass::Select(const SSource sources[3], const DstModifier& modifier)
{
    assert(modifier.flag == nullptr && sources[0].mod == EMOD_NONE);
    CVariable* flag = GetSrcVariable(sources[0]);
    CVariable* src0 = GetSrcVariable(sources[1]);;
    CVariable* src1 = GetSrcVariable(sources[2]);;

    SetSourceModifiers(0, sources[1]);
    SetSourceModifiers(1, sources[2]);
    m_encoder->SetDstModifier(modifier);
    m_encoder->SetPredicateMode(modifier.predMode);

    m_encoder->Select(flag, m_destination, src0, src1);
    m_encoder->Push();

}

void EmitPass::emitOutput(llvm::GenIntrinsicInst* inst)
{
    ShaderOutputType outputType =
        (ShaderOutputType)llvm::cast<llvm::ConstantInt>(inst->getOperand(4))->getZExtValue();
    if(outputType == SHADER_OUTPUT_TYPE_OMASK)
    {
        CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);
        assert(psProgram->GetPhase() == PSPHASE_COARSE &&
            "oMask intrinsics should be left only for coarse phase");
        if(!psProgram->IsLastPhase())
        {
            CVariable* oMask = GetSymbol(inst->getOperand(0));
            CVariable* temp =
                m_currShader->GetNewVariable(numLanes(m_SimdMode), oMask->GetType(), EALIGN_GRF);
            m_encoder->Copy(temp, oMask);
            oMask = temp;
            psProgram->SetCoarseoMask(oMask);
        }
    }
    else
    {
        assert(0 && "output not supported");
    }
}

void EmitPass::emitPSInputMAD(llvm::Instruction* inst)
{
    //create the payload and do interpolation
    CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);
    uint setupIndex;
    e_interpolation mode;

    setupIndex = (uint)llvm::cast<llvm::ConstantInt>(inst->getOperand(0))->getZExtValue();
    mode = (e_interpolation)llvm::cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue();
    CVariable* baryVar = nullptr;


    // mov SIMD4 deltas in to tmp
    // mov (4) r0.0<1>:hf r15.0<4;4,1>:f {Align1, Q1, NoMask} // #??:$26:%29
    CVariable *tmpDeltaDst = nullptr;

    //inputVar
    /*
        For SIMD8 mode we generate mix-mode instructions
        so we won't generate down conversion for
        deltas
    */
    if (psProgram->LowerPSInput())
    {
        tmpDeltaDst = psProgram->GetInputDeltaLowered(setupIndex);
        baryVar = psProgram->GetBaryRegLowered(mode);
    }
    else
    {
        tmpDeltaDst = psProgram->GetInputDelta(setupIndex);
        baryVar = psProgram->GetBaryReg(mode);
    }
    //dst:hf = src1 * src0 + src3
    //dst = p    * u    + r
    //mad (16) r20.0.xyzw:hf r0.3.r:hf r0.0.r:hf r12.0.xyzw:hf {Align16, H1} // #??:$31:%209
    m_encoder->SetSrcSubReg(1, 0);
    m_encoder->SetSrcSubReg(2, 3);
    m_encoder->Mad(m_destination, baryVar, tmpDeltaDst, tmpDeltaDst);
    m_encoder->Push();

    //dst:hf = src1 * src0 + src3
    //dst = q    * v    + dst
    //mad(16) r20.0.xyzw:hf r20.0.xyzw : hf r0.1.r : hf r18.0.xyzw : hf{ Align16, H1 } // #??:$32:%210
    //if we down converting bary coordinate values will be packed
    m_encoder->SetSrcSubReg(0, numLanes(m_currShader->m_SIMDSize));    
    m_encoder->SetSrcSubReg(1, 1);

    m_encoder->Mad(m_destination, baryVar, tmpDeltaDst, m_destination);
    m_encoder->Push();
}

void EmitPass::emitPSInputCst(llvm::Instruction* inst)
{
    CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);
    unsigned int setupIndex = (uint)llvm::cast<llvm::ConstantInt>(inst->getOperand(0))->getZExtValue();
    psProgram->MarkConstantInterpolation(setupIndex);
    CVariable* inputVar = psProgram->GetInputDelta(setupIndex);
    // temp variable should be the same type as the destination
    {
        m_encoder->SetSrcRegion(0, 0, 1, 0);
    }
    m_encoder->SetSrcSubReg(0, 3);
    m_encoder->Cast(m_destination, inputVar);
    m_encoder->Push();
}

void EmitPass::emitPSInput(llvm::Instruction* inst)
{
    e_interpolation mode = (e_interpolation)llvm::cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue();
    if(mode == EINTERPOLATION_CONSTANT)
    {
        emitPSInputCst(inst);
    }
    else if(inst->getType()->isHalfTy() 
        )
    {
        emitPSInputMAD(inst);
    }
    else
    {
        emitPSInputPln(inst);
    }
}

void EmitPass::emitPlnInterpolation(CVariable* baryVar, unsigned int delatIndex)
{
    CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);
    unsigned int numPln = 1;
    // temp variable should be the same type as the destination
    CVariable* inputVar = psProgram->GetInputDelta(delatIndex);
    for(unsigned int i = 0; i < numPln; i++)
    {
        // plane will access 4 operands
        m_encoder->SetSrcRegion(0, 0, 4, 1);
        m_encoder->Pln(m_destination, inputVar, baryVar);
        m_encoder->Push();
    }
}

void EmitPass::emitPSInputPln(llvm::Instruction* inst)
{
    //create the payload and do interpolationd
    CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);
    uint setupIndex = (uint)llvm::cast<llvm::ConstantInt>(inst->getOperand(0))->getZExtValue();
    e_interpolation mode = (e_interpolation)llvm::cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue();
    // need to do interpolation unless we do constant interpolation
    CVariable* baryVar = psProgram->GetBaryReg(mode);
    emitPlnInterpolation(baryVar, setupIndex);
}

void EmitPass::emitEvalAttribute(llvm::GenIntrinsicInst* inst)
{
    CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);
    // temp variable should be the same type as the destination
    bool perspective = cast<ConstantInt>(inst->getOperand(inst->getNumArgOperands() - 1))->getZExtValue() != 0;
    EU_PIXEL_INTERPOLATOR_INTERPOLATION_MODE interpolationMode =
        perspective ? EU_PI_MESSAGE_PERSPECTIVE_INTERPOLATION : EU_PI_MESSAGE_LINEAR_INTERPOLATION;
    if(interpolationMode == EU_PI_MESSAGE_LINEAR_INTERPOLATION)
    {
        // workaround driver interface; tell the driver we use noperspective barys to turn on noperspective interpolation
        psProgram->GetBaryReg(EINTERPOLATION_LINEARNOPERSPECTIVE);
    }
    uint exDesc = EU_GEN7_MESSAGE_TARGET_PIXEL_INTERPOLATOR;
    EU_PIXEL_INTERPOLATOR_SIMD_MODE executionMode = pixelInterpolatorSimDMode(m_currShader->m_SIMDSize);
    uint responseLength = executionMode? 4 : 2;
    uint messageLength = 1;
    CVariable* payload = nullptr;
    uint desc = 0;
    CVariable* messDesc = nullptr;
    switch(inst->getIntrinsicID())
    {
    case GenISAIntrinsic::GenISA_PullSampleIndexBarys:
    {
        payload = m_currShader->GetNewVariable(messageLength*(SIZE_GRF >> 2), ISA_TYPE_D, EALIGN_GRF);
        uint sampleindex = 0;
        desc = PixelInterpolator(
            messageLength,
            responseLength,
            m_encoder->IsSecondHalf() ? 1 : 0,
            executionMode,
            EU_PI_MESSAGE_EVAL_SAMPLE_POSITION,
            interpolationMode,
            sampleindex);

        if (ConstantInt* index = dyn_cast<ConstantInt>(inst->getOperand(0)))
        {
            sampleindex = (uint)index->getZExtValue();
            desc = desc | (sampleindex << 4);
            messDesc = psProgram->ImmToVariable(desc, ISA_TYPE_UD);

            m_encoder->Send(m_destination, payload, exDesc, messDesc, false);
            m_encoder->Push();
        }
        else
        {
            ResourceDescriptor resource;
            CVariable* flag = nullptr;
            uint label;
            bool needLoop;
            CVariable* uniformId;

            SamplerDescriptor sampler = getSampleIDVariable(inst->getOperand(0));
            needLoop = ResourceLoopHeader(resource, sampler, flag, label);
            uniformId = sampler.m_sampler;

            messDesc = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true);

            CVariable* idxShift = m_currShader->GetNewVariable(uniformId);
            m_encoder->Shl(idxShift, uniformId, m_currShader->ImmToVariable(4, ISA_TYPE_UD));
            m_encoder->Or(messDesc, m_currShader->ImmToVariable(desc, ISA_TYPE_UD), idxShift);
            m_encoder->Push();

            m_encoder->SetPredicate(flag);
            m_encoder->Send(m_destination, payload, exDesc, messDesc, false);
            m_encoder->Push();

            ResourceLoop(needLoop, flag, label);
        }
    }
    break;

    case GenISAIntrinsic::GenISA_PullSnappedBarys:
        {
            ConstantInt* xCstOffset = llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(0));
            ConstantInt* yCstOffset = llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(1));
            if(xCstOffset && yCstOffset && psProgram->GetPhase() != PSPHASE_COARSE)
            {
                payload = m_currShader->GetNewVariable(messageLength*(SIZE_GRF >> 2), ISA_TYPE_D, EALIGN_GRF);
                desc = PixelInterpolator(
                    messageLength,
                    responseLength,
                    m_encoder->IsSecondHalf() ? 1 : 0,
                    executionMode,
                    EU_PI_MESSAGE_EVAL_PER_MESSAGE_OFFSET,
                    interpolationMode,
                    (uint)xCstOffset->getZExtValue(),
                    (uint)yCstOffset->getZExtValue());
            }
            else
            {
                messageLength = 2 * numLanes(m_currShader->m_SIMDSize) / 8;
                payload = m_currShader->GetNewVariable(messageLength*(SIZE_GRF >> 2), ISA_TYPE_D, EALIGN_GRF);
                desc = PixelInterpolator(
                    messageLength,
                    responseLength,
                    m_encoder->IsSecondHalf() ? 1 : 0,
                    psProgram->GetPhase() == PSPHASE_COARSE,
                    executionMode,
                    EU_PI_MESSAGE_EVAL_PER_SLOT_OFFSET,
                    interpolationMode);
                CVariable* XOffset = GetSymbol(inst->getOperand(0));
                CVariable* YOffset = GetSymbol(inst->getOperand(1));
                m_encoder->Copy(payload, XOffset);
                m_encoder->Push();

                m_encoder->SetDstSubVar(numLanes(m_currShader->m_SIMDSize) / 8);
                m_encoder->Copy(payload, YOffset);
                m_encoder->Push();
            }
            messDesc = psProgram->ImmToVariable(desc, ISA_TYPE_UD);
        }

        m_encoder->Send(m_destination, payload, exDesc, messDesc, false);
        m_encoder->Push();
        break;

    default:
        assert(0);
        break;
    }
}

void EmitPass::emitInterpolate(llvm::GenIntrinsicInst* inst)
{
    CVariable* barys = GetSymbol(inst->getOperand(1));
    uint setupIndex = (uint)llvm::cast<llvm::ConstantInt>(inst->getOperand(0))->getZExtValue();
    emitPlnInterpolation(barys, setupIndex);
}

void EmitPass::emitDSInput(llvm::Instruction* pInst)
{
    assert(m_currShader->GetShaderType()==ShaderType::DOMAIN_SHADER);
    CVariable* dst = m_destination;

    CDomainShader* dsProgram = static_cast<CDomainShader*>(m_currShader);
    // only pulled inputs reach here
    QuadEltUnit globalOffset(0);
    llvm::Value* pPayloadInputIdx = pInst->getOperand(0);
    llvm::ConstantInt* pConstIntPayloadVar = llvm::dyn_cast_or_null<llvm::ConstantInt>(pPayloadInputIdx);
    uint32_t elmIdx = 0;

    if (pConstIntPayloadVar != nullptr)
    {
        elmIdx = int_cast<uint32_t>(cast<llvm::ConstantInt>(pConstIntPayloadVar)->getZExtValue());

        CVariable* inputVar = dsProgram->GetInputDelta(elmIdx);

        if (dsProgram->GetShaderDispatchMode() == ShaderDispatchMode::DUAL_PATCH)
        {
            m_encoder->SetSrcSubReg(0, elmIdx % 4);
            m_encoder->SetSrcRegion(0, 4, 4, 0);
        }
        m_encoder->Copy(dst, inputVar);
        m_encoder->Push();
    }
    else
    {
        assert(false && "Only constant payload input variable index handled");
    }
}

void EmitPass::emitInput(llvm::Instruction* inst)
{
    switch(m_currShader->GetShaderType())
    {
    case ShaderType::PIXEL_SHADER:
        emitPSInput(inst);
        break;
    case ShaderType::DOMAIN_SHADER:
        emitDSInput(inst);
        break;
    default:
        assert(0);
        break;
    }
}

void EmitPass::emitcycleCounter(llvm::Instruction* inst)
{
    CVariable* dst = m_destination;
    m_encoder->Copy(dst, m_currShader->GetTSC());
    m_encoder->Push();
    m_encoder->SetSrcSubReg(0,1);
    m_encoder->SetDstSubReg(1);
    m_encoder->Copy(dst, m_currShader->GetTSC());
    m_encoder->Push();
}

void EmitPass::emitSetDebugReg(llvm::Instruction* inst)
{
    Value* src0 = inst->getOperand(0);
    if (!isa<UndefValue>(src0))
    {
        // write dbg0.0
        CVariable* src = GetSymbol(src0);
        assert(src->IsUniform());
        m_encoder->SetDstSubReg(0);
        m_encoder->Copy(m_currShader->GetDBG(), src);
        m_encoder->Push();
    }

    // read dbg0.1
    m_encoder->SetSrcSubReg(0,1);
    m_encoder->SetSrcRegion(0, 0, 1, 0);
    m_encoder->Copy(m_destination, m_currShader->GetDBG());
    m_encoder->Push();
}

CVariable* EmitPass::ComputeSampleIntOffset(llvm::Instruction* sample, uint sourceIndex)
{
    uint offset = 0;
    bool dynamicOffset = false;
    for(uint i = 0; i < 3; i++)
    {
        if(ConstantInt* immOffset = dyn_cast<ConstantInt>(sample->getOperand(sourceIndex + i)))
        {
            uint channelOffset = static_cast<uint>(immOffset->getZExtValue());
            offset = (offset << 4) | (channelOffset & 0xf);
        }
        else
        {
            dynamicOffset = true;
        }
    }
    CVariable* packedOffset = m_currShader->ImmToVariable(offset, ISA_TYPE_UW);
    if(dynamicOffset)
    {
        CVariable* tempPackedOffset = m_currShader->GetNewVariable(1, ISA_TYPE_UW, EALIGN_WORD, true);
        for(uint i = 0; i < 3; i++)
        {
            if(!isa<ConstantInt>(sample->getOperand(sourceIndex + i)))
            {
                CVariable* offsetV = GetSymbol(sample->getOperand(sourceIndex + i));
                if(!offsetV->IsUniform())
                {
                    offsetV = UniformCopy(offsetV);
                }

                // Offset is only 4 bits, mask off remaining bits
                CVariable* offsetBits = m_currShader->GetNewVariable(1, ISA_TYPE_UW, EALIGN_WORD, true);
                m_encoder->And(offsetBits, offsetV, m_currShader->ImmToVariable(0xF, ISA_TYPE_UW));
                if(i>0)
                {
                    m_encoder->Shl(offsetBits, offsetBits, m_currShader->ImmToVariable(i * 4, ISA_TYPE_UW));
                }
                if(packedOffset->IsImmediate() && packedOffset->GetImmediateValue() == 0)
                {
                    packedOffset = offsetBits;
                }
                else
                {
                    m_encoder->Or(tempPackedOffset, packedOffset, offsetBits);
                    packedOffset = tempPackedOffset;
                }
            }
        }
    }
    return packedOffset;
}

// simple helper to reorder input depending on the generation
uint CorrectLdIndex(uint i, bool oldLoad)
{
    uint index = i;
    if(oldLoad)
    {
        if(i == 1)
        {
            index = 2;
        }
        else if(i == 2)
        {
            index =1;
        }
    }
    return index;
}

CVariable* EmitPass::IndexableResourceIndex(CVariable* indexVar, uint btiIndex)
{
    CVariable* bti = m_currShader->ImmToVariable(btiIndex, ISA_TYPE_UD);
    CVariable* dst = m_currShader->GetNewVariable(indexVar);
    m_encoder->Add(dst, indexVar, bti);
    m_encoder->Push();
    return dst;
}

void EmitPass::PackSIMD8HFRet(CVariable* dst)
{
    // the extra moves will be cleaned up by vISA
    for (uint16_t n = 0; n < m_destination->GetNumberElement() / 8; n++)
    {
        m_encoder->SetDstSubReg(n * 8);
        m_encoder->SetSrcSubReg(0, n * 16);
        m_encoder->Copy(m_destination, dst);
        m_encoder->Push();
    }
}

void EmitPass::emitLdInstruction(llvm::Instruction* inst, bool isPtr)
{
    uint numOperands = inst->getNumOperands();
    assert(numOperands > 7 && numOperands < 10 && "Wrong number of operands");

    uint writeMask = m_currShader->GetExtractMask(inst);
    assert(writeMask!=0 && "Wrong write mask");

    EOPCODE opCode = GetOpCode(inst);
    //Subtract the offsets, resource sources to get
    //the number of texture coordinates and index to texture source
    uint numSources = numOperands - 5;
    uint textureArgIdx = numOperands - 5;

    ResourceDescriptor resource;
    if (isPtr)
    {
        Value* ptr = inst->getOperand(textureArgIdx);
        resource = GetResourceVariable(ptr);
    }
    else
    {
        llvm::Value* texOp = inst->getOperand(textureArgIdx);
        uint bindingTableIndex = m_currShader->m_pBtiLayout->GetTextureIndex(0);
        if(llvm::dyn_cast<llvm::ConstantInt>(texOp))
        {
            uint textureIdx = int_cast<uint>(GetImmediateVal(texOp));
            bindingTableIndex = m_currShader->m_pBtiLayout->GetTextureIndex(textureIdx);
            resource.m_resource = m_currShader->ImmToVariable(bindingTableIndex, ISA_TYPE_UD);
            
            m_currShader->SetBindingTableEntryCountAndBitmap(true, bindingTableIndex);
        }
        else
        {
            //Indexable texture, when index is not an immediate value
            CVariable* bti = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_GRF, true);
            bti = GetSymbol( texOp );
            resource.m_resource = IndexableResourceIndex(bti, bindingTableIndex);
            
            m_currShader->SetBindingTableEntryCountAndBitmap(false);
        }
    }

    uint offsetSourceIndex = numSources+1;
    CVariable* offset = ComputeSampleIntOffset(inst, offsetSourceIndex);

    SmallVector<CVariable*, 4> payload;

    for( uint i = numSources - 1; i > 0; i-- )
    {
        uint index = CorrectLdIndex(i, m_currShader->m_Platform->hasOldLdOrder());
        CVariable* src = GetSymbol( inst->getOperand( index ) );
        if( !(src->IsImmediate() && src->GetImmediateValue() == 0) )
        {
            break;
        }
        numSources--;
    }

    bool zeroLOD = false;
    //SKL+ new message ld_lz
    if(numSources > 2 && m_currShader->m_Platform->supportSampleAndLd_lz())
    {
        // Check if lod is 0
        CVariable* src = GetSymbol(inst->getOperand(2));
        if(src->IsImmediate() && src->GetImmediateValue() == 0)
        {
            zeroLOD = true;
            numSources--;
        }
    }

    //create send payload for numSources
    for( uint i = 0; i < numSources; i++ )
    {
        uint index = i;
        //no difference in ld_lz between SKL+ and BDW
        if(!zeroLOD)
        {
            index = CorrectLdIndex(i, m_currShader->m_Platform->hasOldLdOrder());
        }
        if(zeroLOD && index==2)
        {
            //3D resources skip lod and read z coordinate
            index = 3;
        }
        CVariable* src = GetSymbol(inst->getOperand(index));
        if(src->IsUniform())
        {
            uint16_t size = m_destination->IsUniform() ? 8 : numLanes(m_currShader->m_SIMDSize);
            CVariable* newSource = m_currShader->GetNewVariable(
                size,
                src->GetType(),
                EALIGN_GRF,
                m_destination->IsUniform());
            m_encoder->SetUniformSIMDSize(SIMDMode::SIMD8);
            m_encoder->Copy(newSource, src);
            m_encoder->Push();
            src = newSource;
        }
        payload.push_back(src);

    }

    //When sampler output is 16 bit float, hardware doesnt pack the output in SIMD8 mode.
    //Hence the movs to handle this layout in SIMD8 mode
    bool needPacking = false;
    CVariable* dst = m_destination;
    SIMDMode simdSize = m_currShader->m_SIMDSize;
    if(dst->IsUniform())
    {
        simdSize = SIMDMode::SIMD8;
        unsigned short numberOfElement = dst->GetNumberElement() * 8;
        numberOfElement =
            m_destination->GetType() == ISA_TYPE_HF ? numberOfElement * 2 : numberOfElement;
        dst = m_currShader->GetNewVariable(numberOfElement, dst->GetType(), EALIGN_GRF, dst->IsUniform());
    }
    else
    {
        needPacking = m_SimdMode == SIMDMode::SIMD8 &&
            m_destination->GetType() == ISA_TYPE_HF &&
            !m_destination->isUnpacked();
        if(needPacking)
        {
            dst = m_currShader->GetNewVariable(m_destination->GetNumberElement() * 2, ISA_TYPE_HF, EALIGN_GRF, dst->IsUniform());
        }
    }

    bool feedbackEnable = (writeMask & (1<<4))? true : false;
    uint label = 0;
    CVariable* flag = nullptr;
    bool needLoop = ResourceLoopHeader(resource, flag, label);
    m_encoder->SetPredicate(flag);
    if(m_destination->IsUniform())
    {
        m_encoder->SetUniformSIMDSize(SIMDMode::SIMD8);
    }
    m_encoder->Load(opCode, writeMask, offset, resource, numSources, dst, payload, zeroLOD, feedbackEnable);
    m_encoder->Push();
    if (m_currShader->hasReadWriteImage(*(inst->getParent()->getParent())))
    {
        m_encoder->Copy(m_currShader->GetNULL(), m_destination);
        m_encoder->Push();
        m_encoder->Copy(m_currShader->GetNULL(), m_currShader->GetTSC());
        m_encoder->Push();
    }
    ResourceLoop(needLoop, flag, label);

    if(feedbackEnable)
    {
        CVariable* flag = m_currShader->GetNewVariable(numLanes(m_currShader->m_dispatchSize), ISA_TYPE_BOOL, EALIGN_BYTE);
        uint subvar = numLanes(simdSize) / (SIZE_GRF >> 2) * 4;
        m_encoder->SetSrcSubVar(0, subvar);
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        CVariable* newdestination = m_currShader->BitCast(dst, ISA_TYPE_UD);
        m_encoder->SetP(flag, newdestination);
        m_encoder->Push();

        CVariable* pred = m_currShader->ImmToVariable(0xFFFFFFFF, dst->GetType());
        CVariable* zero = m_currShader->ImmToVariable(0x0, dst->GetType());
        m_encoder->SetDstSubVar(subvar);
        m_encoder->Select(flag, dst, pred, zero);
        m_encoder->Push();
    }

    if(m_destination->IsUniform())
    {
        for(unsigned int i = 0; i < m_destination->GetNumberElement(); i++)
        {
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->SetSrcSubVar(0, i);
            m_encoder->SetDstSubReg(i);
            m_encoder->Copy(m_destination, dst);
            m_encoder->Push();
        }
    }

    if (needPacking)
    {
        PackSIMD8HFRet(dst);
    }
}

/// \brief Returns the offset increment in bytes, given the value's type.
static int GetOffsetIncrement(const DataLayout* m_DL, SIMDMode simdMode, Value* val)
{
    int inc;
    inc = int_cast<int>(numLanes(simdMode) * m_DL->getTypeAllocSize(val->getType()));
    if (val->getType()->isHalfTy() && simdMode == SIMDMode::SIMD8)
    {
        //Since alloc size for half float is = 2 and if we have simd8 mode we'll get offset = 16
        //but need to pad it with extra 16.
        assert(inc <= 16);
        inc *= 2;
    }
    return inc;
}

///
bool EmitPass::interceptRenderTargetWritePayloadCoalescing(
    llvm::RTWritIntrinsic* inst,
    CVariable** src,
    CVariable*& source0Alpha,
    CVariable*& oMaskOpnd,
    CVariable*& outputDepthOpnd,
    CVariable*& vStencilOpnd,
    DenseMap<Value*, CVariable**>& valueToVariableMap)
{
    CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);

    //check coalescing
    CoalescingEngine::CCTuple* ccTuple = nullptr;
    m_CE->SetCurrentPart(inst, 0);
    const uint numOperands = m_CE->GetNumPayloadElements(inst);
    Value* dummyValPtr = nullptr;
    int payloadToCCTupleRelativeOffset = 0;

    ccTuple = m_CE->IsAnyValueCoalescedInCCTuple(inst,
        numOperands,
        //out:
        payloadToCCTupleRelativeOffset,
        dummyValPtr);
    bool payloadCovered = m_CE->IsPayloadCovered(inst, ccTuple, numOperands, payloadToCCTupleRelativeOffset);
    if (!payloadCovered) {
        return false;
    }

    //This check is necessary, since IsPayloadCovered is not checking for non-homogeneous part.
    if (m_CE->HasNonHomogeneousPayloadElements(inst) &&
        !ccTuple->HasNonHomogeneousElements())
    {
        return false;
    }


    if (ccTuple->HasNonHomogeneousElements())
    {
        if (m_CE->GetLeftReservedOffset(ccTuple->GetRoot(), m_currShader->m_SIMDSize) <
            m_CE->GetLeftReservedOffset(inst, m_currShader->m_SIMDSize))
        {
            return false;
        }
		if (payloadToCCTupleRelativeOffset)
		{
			return false;
		}
    }

    assert(ccTuple);
    CVariable* rootPayloadVar = m_currShader->LazyCreateCCTupleBackingVariable(ccTuple);

    //Elements are processed in the payload slot order.
    //Homogeneous part is looked-up through payload coalescing methods.
    //Payload layout: s0Alpha oM[R G B A] sZ oS
    int offset = 0;
    if (RTWriteHasSource0Alpha(inst, m_moduleMD))
    {
        assert(ccTuple->HasNonHomogeneousElements());

        VISA_Type vType = m_currShader->GetType(inst->getSource0Alpha()->getType());

        //CVariable** source0AlphaVarPtr = valueToVariableMap[inst->getSource0Alpha()];
        assert(source0Alpha == nullptr);
        CVariable* temp = m_currShader->GetNewAlias(rootPayloadVar, vType, (uint16_t)offset, 0);
        m_encoder->Copy(temp, GetSymbol(inst->getSource0Alpha()));
        m_encoder->Push();
        source0Alpha = temp;
    }

    //assert(ccTuple->IsFrozen()); //in other words, has non-homogeneous elements

    if (ccTuple->HasNonHomogeneousElements())
    {
        assert(ccTuple->GetRoot()); //in other words, there is a 'supremum' element
        assert(llvm::isa<llvm::RTWritIntrinsic>(ccTuple->GetRoot()));
        if (llvm::RTWritIntrinsic* rtwi = llvm::dyn_cast<llvm::RTWritIntrinsic>(ccTuple->GetRoot()))
        {
            if (RTWriteHasSource0Alpha(rtwi, m_moduleMD))
            {
                //This is a stronger condition than querying 'inst' only, since root represents
                //the whole group of 'non-homogeneous' parts. E.g. it might turn out, that this
                //instruction does not have src0 alpha, but it was coalesced in a group that has
                //at least one src0 alpha. Thus, we need to take that src0 alpha into account
                //when computing 'left' reserved offset.
                offset += GetOffsetIncrement(m_DL, m_currShader->m_SIMDSize, rtwi->getSource0Alpha());
            }
        }
    }

    if (inst->hasMask())
    {
        assert(!DoesRTWriteSrc0AlphaBelongToHomogeneousPart(inst, m_moduleMD));
        assert(oMaskOpnd == nullptr);

        CVariable* oLocalMaskOpnd = GetSymbol(inst->getOMask());
        oLocalMaskOpnd = psProgram->BitCast(oLocalMaskOpnd, ISA_TYPE_UW);

        CVariable* temp = m_currShader->GetNewAlias(rootPayloadVar, ISA_TYPE_D, (uint16_t)offset, 0);
        psProgram->PackAndCopyVariable(temp, oLocalMaskOpnd);
        oMaskOpnd = temp;
    }

    if (ccTuple->HasNonHomogeneousElements())
    {
        assert(ccTuple->GetRoot()); //in other words, there is a 'supremum' element
        assert(llvm::isa<llvm::RTWritIntrinsic>(ccTuple->GetRoot()));
        if (llvm::RTWritIntrinsic* rtwi = llvm::dyn_cast<llvm::RTWritIntrinsic>(ccTuple->GetRoot()))
        {
            //Take left reserved offset from 'root' of the group, not from this instruction.
            offset = m_CE->GetLeftReservedOffset(ccTuple->GetRoot(), m_currShader->m_SIMDSize);
        }
    }

	assert(dummyValPtr);

	offset += payloadToCCTupleRelativeOffset *
		m_CE->GetSingleElementWidth(m_currShader->m_SIMDSize, m_DL, dummyValPtr);


    SmallPtrSet<Value*, 8> touchedValuesSet;
    assert(numOperands == 4);
    for (uint index = 0; index < numOperands; index++)
    {
        Value* val = m_CE->GetPayloadElementToValueMapping(inst, index);
        assert(val && "Val cannot be NULL");
        VISA_Type type = m_currShader->GetType(val->getType());

        if (touchedValuesSet.count(val)){
            src[index] = m_currShader->GetNewAlias(rootPayloadVar, type, (uint16_t)(offset), 0);
            m_encoder->Copy(src[index], GetSymbol(val));
            m_encoder->Push();
            offset += GetOffsetIncrement(m_DL, m_currShader->m_SIMDSize, val);
            continue;
        }
        else {
            touchedValuesSet.insert(val);
        }

        bool needsCopy = false;
        if (m_CE->IsValConstOrIsolated(val)){
            needsCopy = true;
        }
        else
        {
            if (CoalescingEngine::CCTuple* thisCCTuple = m_CE->GetValueCCTupleMapping(val))
            {
                src[index] = GetSymbol(val);
            }
            else
            {
                //this one actually encompasses the case for !getRegRoot(val)
                needsCopy = true;
            }
        }//if constant

        if (needsCopy)
        {
            src[index] = m_currShader->GetNewAlias(rootPayloadVar, type, (uint16_t)offset, 0);
            m_encoder->Copy(src[index], GetSymbol(val));
            m_encoder->Push();
        }
        offset += GetOffsetIncrement(m_DL, m_currShader->m_SIMDSize, val);
    }//for

    if (inst->hasDepth())
    {
        assert(outputDepthOpnd == nullptr);
        CVariable* temp = m_currShader->GetNewAlias(rootPayloadVar, ISA_TYPE_F, (uint16_t)offset, 0);
        m_encoder->Copy(temp, GetSymbol(inst->getDepth()));
        m_encoder->Push();
        outputDepthOpnd = temp;

        assert(inst->getDepth()->getType()->isFloatTy());
    }

    if (ccTuple->HasNonHomogeneousElements())
    {
        assert(ccTuple->GetRoot()); //in other words, there is a 'supremum' element
        assert(llvm::isa<llvm::RTWritIntrinsic>(ccTuple->GetRoot()));

        if (llvm::RTWritIntrinsic* rtwi = llvm::dyn_cast<llvm::RTWritIntrinsic>(ccTuple->GetRoot()))
        {
            if (rtwi->hasDepth())
            {
                offset += GetOffsetIncrement(m_DL, m_currShader->m_SIMDSize, inst->getDepth());
            }
        }
    }


    //Stencil is only supported in SIMD8 mode
    if (inst->hasStencil())
    {
        assert(m_currShader->m_SIMDSize == SIMDMode::SIMD8);
        assert(vStencilOpnd == nullptr);

        CVariable* temp = m_currShader->GetNewAlias(rootPayloadVar, ISA_TYPE_UB, (uint16_t)offset, 0);
        CVariable* ubSrc = m_currShader->BitCast(GetSymbol(inst->getStencil()), ISA_TYPE_UB);
        if (ubSrc->IsUniform())
        {
            m_encoder->SetSrcRegion(0, 0, 1, 0);
        }
        else
        {
            m_encoder->SetSrcRegion(0, 32, 8, 4);
        }
        m_currShader->CopyVariable(temp, ubSrc, 0);
        vStencilOpnd = temp;
    }
    return true;
}

///
void EmitPass::prepareRenderTargetWritePayload(
    llvm::RTWritIntrinsic* inst,
    DenseMap<Value*, CVariable**>& valueToVariableMap,
    Value* color[4],
    //output:
    CVariable** src,
    bool* isUndefined,
    CVariable*& varSource0Alpha,
    CVariable*& varMaskOpnd,
    CVariable*& varDepthOpnd,
    CVariable*& varStencilOpnd )
{
    CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);

    VISA_Type vType = ISA_TYPE_F;

    if (color[0]->getType()->isHalfTy())
    {
        vType = ISA_TYPE_HF;
    }

    for (uint i = 0; i < 4; ++i)
    {
        if (isa<UndefValue>(color[i]))
        {
            isUndefined[i] = true;
        }
    }

    if (interceptRenderTargetWritePayloadCoalescing(
            inst,
            src,
            varSource0Alpha,
            varMaskOpnd,
            varDepthOpnd,
            varStencilOpnd,
            valueToVariableMap))
    {
        return;
    }

    CVariable* var[4];

    var[0] = GetSymbol(color[0]);
    var[1] = GetSymbol(color[1]);
    var[2] = GetSymbol(color[2]);
    var[3] = GetSymbol(color[3]);

    for (uint i = 0; i < 4; ++i)
    {
        if (!isa<UndefValue>(color[i]))
        {
            if (var[i]->IsUniform())
            {
                //if uniform creates a move to payload
                src[i] = m_currShader->GetNewVariable(numLanes(m_currShader->m_SIMDSize), vType, EALIGN_GRF);
                m_encoder->Copy(src[i], var[i]);
                m_encoder->Push();
            }
            else
            {
                src[i] = var[i];
            }
        }
    }

    if (RTWriteHasSource0Alpha(inst, m_moduleMD))
    {
        varSource0Alpha = GetSymbol(inst->getSource0Alpha());
        if (varSource0Alpha->IsUniform())
        {
            CVariable* temp = m_currShader->GetNewVariable(numLanes(m_currShader->m_SIMDSize), vType, EALIGN_GRF);
            m_encoder->Copy(temp, varSource0Alpha);
            m_encoder->Push();
            varSource0Alpha = temp;
        }
    }

    if (inst->hasMask())
    {
        varMaskOpnd = GetSymbol(inst->getOMask());
        //oMask has to be packed since the hardware ignores the upper half
        CVariable* temp = m_currShader->GetNewVariable(numLanes(m_currShader->m_SIMDSize), ISA_TYPE_D, EALIGN_GRF);
        varMaskOpnd = psProgram->BitCast(varMaskOpnd, ISA_TYPE_UW);
        psProgram->PackAndCopyVariable(temp, varMaskOpnd);
        varMaskOpnd = temp;
    }

    if (inst->hasDepth())
    {
        varDepthOpnd = GetSymbol(inst->getDepth());
        if (varDepthOpnd->IsUniform())
        {
            CVariable * temp = m_currShader->GetNewVariable(numLanes(m_currShader->m_SIMDSize), ISA_TYPE_F, EALIGN_GRF);
            m_encoder->Copy(temp, varDepthOpnd);
            m_encoder->Push();
            varDepthOpnd = temp;
        }
    }

    //Stencil is only supported in SIMD8 mode
    if (inst->hasStencil())
    {
        varStencilOpnd = GetSymbol(inst->getStencil());
        /*4 bytes are needed for the final destination per element*/
        CVariable * temp = m_currShader->GetNewVariable(numLanes(m_currShader->m_SIMDSize)*4, ISA_TYPE_UB, EALIGN_GRF);
        CVariable* ubSrc = m_currShader->BitCast(varStencilOpnd, ISA_TYPE_UB);
        if (varStencilOpnd->IsUniform())
        {
            m_encoder->SetSrcRegion(0, 0, 1, 0);
        }
        else
        {
            m_encoder->SetSrcRegion(0, 32, 8, 4);
        }
        m_currShader->CopyVariable(temp, ubSrc, 0);
        varStencilOpnd = temp;
    }

}

// Generate a predicate based on current active channels.  The 'alias' is
// some existing variable in context to be reused only for generating mask,
// to avoid allocating a new variable.

void EmitPass::emitPredicateFromChannelIP(CVariable* dst, CVariable* alias)
{
    CVariable *any;

    if (alias)
    {
        any = m_currShader->GetNewAlias(alias, ISA_TYPE_UD, 0, 1);
    }
    else
    {
        any = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD);
    }

    m_encoder->SetSrcRegion(0, 0, 1, 0);
    m_encoder->SetSrcRegion(1, 0, 1, 0);
    m_encoder->Cmp(EPREDICATE_EQ, dst, any, any);
    m_encoder->Push();
}

void EmitPass::emitRenderTargetWrite(llvm::RTWritIntrinsic* inst, bool fromRet)
{
    assert(m_currShader->GetShaderType() == ShaderType::PIXEL_SHADER);
    CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);

    bool lastRenderTarget = psProgram->IsLastRTWrite(inst);
    bool EOT = lastRenderTarget && (m_encoder->IsSecondHalf() || m_currShader->m_numberInstance == 1);

    int RTIndex = inst->getRTIndexImm();
    bool oMask = inst->hasMask();
    bool outputDepth = inst->hasDepth();
    bool outputStencil = inst->hasStencil();
    bool perSample = inst->perSample();
    Value* vSrc0Alpha = inst->getSource0Alpha();
    Value* vMask = inst->getOMask();
    Value* pMask = inst->getPMask();
    Value* vDepth = inst->getDepth();
    Value* vStencil = inst->getStencil();
    Value* vSample = inst->getSampleIndex();
    Value* vColor[4] = { inst->getRed(), inst->getGreen(), inst->getBlue(), inst->getAlpha() };

    if(outputDepth)
    {
        psProgram->OutputDepth();
    }
    if(outputStencil)
    {
        psProgram->OutputStencil();
    }
    if(oMask)
    {
        psProgram->OutputMask();
    }
    uint bindingTableIndex = 0;
    if(RTIndex != -1)
    {
        bindingTableIndex = m_currShader->m_pBtiLayout->GetRenderTargetIndex(RTIndex);
    }
    else
    {
        if(psProgram->GetPhase() != PSPHASE_LEGACY)
        {
            return;
        }
        bindingTableIndex = m_currShader->m_pBtiLayout->GetNullSurfaceIdx();
    }

    bool directIdx = inst->isImmRTIndex();
    m_currShader->SetBindingTableEntryCountAndBitmap(directIdx, bindingTableIndex);

    if(EOT)
    {
        assert(psProgram->m_hasEOT == false);
        psProgram->m_hasEOT = true;
    }

    //Following variables will receive output from a call
    CVariable* src[4] = { nullptr, nullptr, nullptr, nullptr };
    bool isUndefined[4] = { false, false, false, false };
    CVariable* source0Alpha = nullptr;
    CVariable* oMaskOpnd = nullptr;
    CVariable* outputDepthOpnd = nullptr;
    CVariable* stencilOpnd = nullptr;
    CVariable* pMaskOpnd = nullptr;

    DenseMap<Value*, CVariable**> valueToVariableMap;
    if(!isa<UndefValue>(vSrc0Alpha)) {
        valueToVariableMap[vSrc0Alpha] = &source0Alpha;
    }
    if (oMask) {
        valueToVariableMap[vMask] = &oMaskOpnd;
    }
    if (outputDepth){
        valueToVariableMap[vDepth] = &outputDepthOpnd;
    }
    if (outputStencil){
        valueToVariableMap[vStencil] = &stencilOpnd;
    }

    valueToVariableMap[vColor[0]] = &src[0];
    valueToVariableMap[vColor[1]] = &src[1];
    valueToVariableMap[vColor[2]] = &src[2];
    valueToVariableMap[vColor[3]] = &src[3];

    prepareRenderTargetWritePayload(
        //in:
        inst,
        valueToVariableMap,
        vColor,
        //out:
        src,
        isUndefined,
        source0Alpha,
        oMaskOpnd,
        outputDepthOpnd,
        stencilOpnd );

    CVariable* cpsCounter = nullptr;
    if(psProgram->GetPhase() == PSPHASE_PIXEL)
    {
        cpsCounter = psProgram->GetCurrentPhaseCounter();
    }

    bool coarseMode = false;
    if(psProgram->GetPhase() == PSPHASE_COARSE)
    {
        coarseMode = true;
    }

    CVariable* bti = m_currShader->ImmToVariable(bindingTableIndex, ISA_TYPE_D);

    CVariable* sampleIndex = nullptr;
    if(perSample)
    {
        sampleIndex = GetSymbol(vSample);
        if(!sampleIndex->IsUniform())
        {
            sampleIndex = UniformCopy(sampleIndex);

        }
    }

    if (psProgram->HasDiscard())
    {
        ConstantInt* cv = dyn_cast<ConstantInt>(pMask);
        if (!cv || cv->getZExtValue() == 0)
        {
            pMaskOpnd = GetSymbol(pMask);
            m_encoder->SetPredicate(pMaskOpnd);
        }
    }

    bool isHeaderMaskFromCe0 =
        !isa<ReturnInst>(inst->getParent()->getTerminator()) &&
        pMaskOpnd == nullptr;

    CVariable* rtIndexOpnd;
    if (RTIndex < 0 || (m_moduleMD->psInfo.BlendStateDisabledMask & BIT(RTIndex)))
    {
        // if blending is disabled no need to set the RTIndex in the header
        rtIndexOpnd = m_currShader->ImmToVariable(0, ISA_TYPE_D);
    }
    else
    {
        if (psProgram->IsPerSample())
        {
            rtIndexOpnd = GetSymbol(inst->getBlendStateIndex());
            assert(rtIndexOpnd->IsUniform());
        }
        else
        {
            rtIndexOpnd = m_currShader->ImmToVariable(RTIndex, ISA_TYPE_D);
        }
    }

    m_encoder->RenderTargetWrite(
        src,
        isUndefined,
        lastRenderTarget,
        perSample,
        coarseMode,
        isHeaderMaskFromCe0,
        bti,
        rtIndexOpnd,
        source0Alpha,
        oMaskOpnd,
        outputDepthOpnd,
        stencilOpnd,
        cpsCounter /*cpscounter*/,
        sampleIndex,
        psProgram->GetR1());
    m_encoder->Push();
}

void EmitPass::emitSimdLaneId( llvm::Instruction* inst )
{
    m_currShader->GetSimdOffsetBase( m_destination );
}

void EmitPass::emitPatchInstanceId( llvm::Instruction* inst )
{
    assert(m_currShader->GetShaderType() == ShaderType::HULL_SHADER);
    CHullShader* hsProgram = static_cast<CHullShader*>(m_currShader);

    // Set barrier encountered to true so we can program the instance Count accordingly
    hsProgram->SetBarrierEncountered();

    /*
    **     R0.2       23:17      Instance Number. A patch-relative instance number between 0 and InstanceCount-1. BDW, SKL.
    **     -----------------
    **     R0.2       22:16      Instance Number. A patch-relative instance number between 0 and InstanceCount-1. CNL+.
    */
    unsigned int instanceIdStartBit = m_currShader->m_Platform->getHullShaderThreadInstanceIdBitFieldPosition();
    CVariable* mask7bit = m_currShader->ImmToVariable(0x7f, ISA_TYPE_UD);
    m_encoder->SetSrcRegion(0, 0, 1, 0);
    m_encoder->SetSrcSubReg(0, 2);
    m_encoder->Shr(m_destination, hsProgram->GetR0(), m_currShader->ImmToVariable(instanceIdStartBit, ISA_TYPE_UD));
    m_encoder->SetSrcSubReg(0, 0);
    m_encoder->And(m_destination, m_destination, mask7bit);
    m_encoder->Push();
}

void EmitPass::emitSimdSize( llvm::Instruction* inst )
{
    //CVariable* simdSize = m_currShader->ImmToVariable( numLanes( m_SimdMode ), ISA_TYPE_UD );
    //m_encoder->Cast( m_destination, simdSize );
    //m_encoder->Push();
}

/// Emits VISA instructions for SIMD_SHUFFLE.
void EmitPass::emitSimdShuffle( llvm::Instruction* inst )
{
    CVariable* data        = GetSymbol( inst->getOperand( 0 ) );
    CVariable* simdChannel = GetSymbol( inst->getOperand( 1 ) );

    const bool isSimd32 = (m_currShader->m_dispatchSize == SIMDMode::SIMD32);

    if ( data->IsUniform())
    {
        m_encoder->Copy( m_destination, data );
        m_encoder->Push();
        if (isSimd32 && !m_destination->IsUniform())
        {
            m_encoder->SetSecondHalf(true);
            m_encoder->Copy(m_destination, data);
            m_encoder->Push();
            m_encoder->SetSecondHalf(false);
        }
    }
    else if (simdChannel->IsImmediate())
    {
        uint dataIndex = int_cast<uint>(simdChannel->GetImmediateValue());
        // prevent out of bound access
        dataIndex = dataIndex % numLanes(m_currShader->m_dispatchSize); 
        if (isSimd32)
        {
            const bool isSrcInSecondHalf = dataIndex >= 16;
            dataIndex = dataIndex % 16;

            if (m_destination->IsUniform())
            {
                m_encoder->SetSecondHalf(isSrcInSecondHalf);
                m_encoder->SetSrcRegion(0, 0, 1, 0);
                m_encoder->SetSrcSubReg(0, dataIndex);
                m_encoder->Copy(m_destination, data);
                m_encoder->Push();
                m_encoder->SetSecondHalf(false);
            }
            else
            {
                // Use an intermediate uniform variable
                CVariable* uniformTemp = m_currShader->GetNewVariable(
                    1,
                    data->GetType(),
                    m_encoder->GetCISADataTypeAlignment(data->GetType()),
                    true); // isUniform

                // Copy from source to the uniform temp...
                m_encoder->SetSecondHalf(isSrcInSecondHalf);
                m_encoder->SetSrcRegion(0, 0, 1, 0);
                m_encoder->SetSrcSubReg(0, dataIndex);
                m_encoder->SetNoMask();
                m_encoder->Copy(uniformTemp, data);
                m_encoder->Push();
                m_encoder->SetSecondHalf(false);

                // ...and broadcast.
                m_encoder->Copy(m_destination, uniformTemp);
                m_encoder->Push();
                m_encoder->SetSecondHalf(true);
                m_encoder->Copy(m_destination, uniformTemp);
                m_encoder->SetSecondHalf(false);
                
            }
        }
        else
        {
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->SetSrcSubReg(0, dataIndex);
            m_encoder->Copy(m_destination, data);
            m_encoder->Push();
        }
    }
    else
    {
        // Emits below instructions when simdChannel isn't immediate.
        //shl (16) r8.0<1>:ud r6.0<0;1,0>:d 0x2:uw {Align1, H1, NoMask}
        //add (16) a0.0<1>:uw r8.0<16;8,2>:uw 0x80:uw {Align1, H1, NoMask}
        //mov (16) r10.0<1>:d r[a0.0, 0]<1,0>:d {Align1, H1}

        bool channelUniform = simdChannel->IsUniform();

        assert(m_encoder->GetCISADataTypeSize(simdChannel->GetType()) == 4 &&
            "simdChannel size of simdShuffle should be 4 bytes!");

        // Choose the shift factor.
        int shtAmt;
        switch (m_encoder->GetCISADataTypeSize(m_destination->GetType()))
        {
            case 2:  shtAmt = 1; break;
            case 4:  shtAmt = 2; break;
            case 8:  shtAmt = 3; break;
            default: assert(false && "Unexpected data type size.");
        }

        CVariable* simdChannelUW = m_currShader->BitCast(simdChannel, ISA_TYPE_UW);
        CVariable* pSrcElm = m_currShader->GetNewVariable(
            simdChannel->GetNumberElement(),
            ISA_TYPE_UW,
            EALIGN_GRF,
            channelUniform,
            simdChannel->GetNumberInstance());
        if (!channelUniform)
        {
            m_encoder->SetSrcRegion(0, 16, 8, 2);
        }
        m_encoder->Shl(pSrcElm, simdChannelUW,
            m_currShader->ImmToVariable(shtAmt, ISA_TYPE_UW));
        m_encoder->Push();

        CVariable *src = data;

        if (isSimd32)
        {
            CVariable *contiguousData = nullptr;
            CVariable *upperHalfOfContiguousData = nullptr;
            
            const uint16_t numElements = data->GetNumberElement();
            const VISA_Type dataType = data->GetType();

            assert(numElements == 16);
            assert(!m_encoder->IsSecondHalf() && "This emitter must be called only once for simd32!");

            // Create a 32 element variable and copy both instances of data into it.
            contiguousData = m_currShader->GetNewVariable(
                numElements * 2,
                dataType,
                data->GetAlign(),
                false, // isUniform
                1); // numberInstance

            upperHalfOfContiguousData = m_currShader->GetNewAlias(
                contiguousData,
                dataType,
                numElements * m_encoder->GetCISADataTypeSize(dataType),
                numElements);

            assert(contiguousData && upperHalfOfContiguousData);

            m_encoder->SetSecondHalf(false);
            m_encoder->Copy(contiguousData, data);
            m_encoder->Push();

            m_encoder->SetSecondHalf(true);
            m_encoder->Copy(upperHalfOfContiguousData, data);
            m_encoder->Push();

            if (!channelUniform)
            {
                // also calculate the second half of address
                m_encoder->SetSrcRegion(0, 16, 8, 2);
                m_encoder->Shl(pSrcElm, simdChannelUW,
                    m_currShader->ImmToVariable(shtAmt, ISA_TYPE_UW));
                m_encoder->Push();
            }

            m_encoder->SetSecondHalf(false);

            src = contiguousData;
        }

        uint16_t addrSize = channelUniform ? 1 :
            (m_SimdMode == SIMDMode::SIMD32 ? numLanes(SIMDMode::SIMD16) : numLanes(m_SimdMode));

        // VectorUniform for shuffle is true as all simd lanes will
        // take the same data as the lane 0 !
        CVariable* pDstArrElm = m_currShader->GetNewAddressVariable(
            addrSize,
            m_destination->GetType(),
            channelUniform,
            true);

        m_encoder->AddrAdd(pDstArrElm, src, pSrcElm);
        m_encoder->Push();

        m_encoder->Copy(m_destination, pDstArrElm);
        m_encoder->Push();

        if (isSimd32)
        {
            m_encoder->SetSecondHalf(true);
            m_encoder->AddrAdd(pDstArrElm, src, pSrcElm);
            m_encoder->Push();
            m_encoder->Copy(m_destination, pDstArrElm);
            m_encoder->Push();
            m_encoder->SetSecondHalf(false);
        }
    }
}

void EmitPass::emitSimdShuffleDown( llvm::Instruction* inst )
{
    CVariable* pCurrentData = GetSymbol( inst->getOperand( 0 ) );
    CVariable* pNextData    = GetSymbol( inst->getOperand( 1 ) );
    CVariable* pDelta       = m_currShader->GetSymbol( inst->getOperand( 2 ) );

    // temp size is the sum of src0 and src1
    uint16_t nbElements = numLanes( m_SimdMode ) * 2;

    // Join current and Next Data
    CVariable* pCombinedData = m_currShader->GetNewVariable(
        nbElements,
        m_destination->GetType(),
        m_destination->GetAlign());

    // copy current data
    m_encoder->SetNoMask();
    m_encoder->Copy( pCombinedData, pCurrentData );
    m_encoder->Push();

    // Copy next data
    uint  dstSubRegOffset = numLanes(m_SimdMode);
    m_encoder->SetDstSubReg( dstSubRegOffset );
    m_encoder->SetNoMask();
    m_encoder->Copy( pCombinedData, pNextData );
    m_encoder->Push();

    if (pDelta->IsImmediate())
    {
        bool split = (m_SimdMode == SIMDMode::SIMD16 && m_destination->GetElemSize() == 4);
        uint offset = int_cast<uint>(pDelta->GetImmediateValue());
        SIMDMode mode = (split ? SIMDMode::SIMD8 : m_SimdMode);
        for (int i = 0; i < (split ? 2 : 1); i++)
        {
            m_encoder->SetSrcSubReg(0, offset + 8 * i);
            m_encoder->SetDstSubVar(i);
            m_encoder->SetSrcRegion(0, 1, 1, 0);
            m_encoder->SetSimdSize(mode);
            m_encoder->SetNoMask();
            m_encoder->Copy(m_destination, pCombinedData);
            m_encoder->Push();
        }
        return;
    }

    // Emits below instructions:
    // mov (8) r12.0<1>:w 0x76543210:v {Align1, Q1, NoMask}
    // mov (8) r38.0<1>:ud r12.0<8;8,1>:w {Align1, Q1, NoMask}
    // add (8) r39.0<1>:ud r38.0<8;8,1>:ud 0x8:uw {Align1, Q1, NoMask}
    // add (16) r40.0<1>:ud r14.0<8;8,1>:d r38.0<8;8,1>:ud {Align1, H1, NoMask}
    // shl (16) r42.0<1>:ud r40.0<8;8,1>:ud 0x2:uw {Align1, H1, NoMask}
    // add (16) a0.0<1>:uw r42.0<16;8,2>:uw 0x440:uw {Align1, H1, NoMask}
    // mov (16) r49.0<1>:d r[a0.0, 0]<1,0>:d {Align1, H1}

    CVariable *pLaneId = m_currShader->GetNewVariable(
        numLanes( m_SimdMode ),
        ISA_TYPE_UD,
        EALIGN_GRF );

    m_encoder->SetSimdSize( SIMDMode::SIMD8 );
    m_encoder->SetNoMask();
    CVariable* imm0 = m_currShader->ImmToVariable( 0x76543210, ISA_TYPE_V );
    m_encoder->Cast(pLaneId, imm0 );
    m_encoder->Push();

    if ( m_SimdMode == SIMDMode::SIMD16 )
    {
        m_encoder->SetDstSubVar( 1 );
        m_encoder->SetSimdSize( SIMDMode::SIMD8 );
        m_encoder->SetNoMask();
        CVariable* imm1 = m_currShader->ImmToVariable( 0x8, ISA_TYPE_UD );
        m_encoder->Add(pLaneId, pLaneId, imm1 );
        m_encoder->Push();
    }

    CVariable *pShuffleIdx = m_currShader->GetNewVariable(
        numLanes( m_SimdMode ),
        ISA_TYPE_UD,
        EALIGN_GRF );

    m_encoder->SetNoMask();
    m_encoder->Add( pShuffleIdx, pLaneId, pDelta );
    m_encoder->Push();

    CVariable *pByteOffset = m_currShader->GetNewVariable(
        numLanes( m_SimdMode ),
        ISA_TYPE_UD,
        EALIGN_GRF );

    uint shift = m_destination->GetElemSize() / 2;
    m_encoder->SetNoMask();
    m_encoder->Shl( pByteOffset, pShuffleIdx, m_currShader->ImmToVariable( shift, ISA_TYPE_UD ) );
    m_encoder->Push();

    CVariable* pDstArrElm = m_currShader->GetNewAddressVariable(
        numLanes( m_SimdMode ),
        m_destination->GetType(),
        false,
        false );

    m_encoder->SetNoMask();
    m_encoder->SetSrcRegion( 1, 16, 8, 2 );
    m_encoder->AddrAdd( pDstArrElm, pCombinedData, m_currShader->BitCast( pByteOffset, ISA_TYPE_UW ) );
    m_encoder->Push();

    m_encoder->Copy( m_destination, pDstArrElm );
    m_encoder->Push();
}

void EmitPass::emitSimdBlockWriteGlobal( llvm::Instruction* inst )
{
    Value* llPtr    = inst->getOperand( 0 );
    Value* dataPtr  = inst->getOperand( 1 );

    PointerType* ptrType = cast<PointerType>( llPtr->getType() );
    ResourceDescriptor resource = GetResourceVariable(llPtr);
    if ( isA64Ptr( ptrType, m_currShader->GetContext() ) )
    {
        CVariable* src  = GetSymbol( llPtr );
        CVariable* data = GetSymbol( dataPtr );

        Type* Ty            = dataPtr->getType();
        VectorType* VTy     = dyn_cast< VectorType >( Ty );
        uint32_t nbElements = VTy ? int_cast<uint32_t>(VTy->getNumElements()) : 1;

        uint32_t bytesPerReg    = 32;
        uint32_t totalBytes     = nbElements * ( ( m_SimdMode == SIMDMode::SIMD8 ) ? 1 : 2 ) * bytesPerReg;
        if (dataPtr->getType()->getScalarSizeInBits() == 16) {
            totalBytes /= 2;
        }
        uint32_t bytesRemaining = totalBytes;

        uint32_t srcOffset   = 0;
        uint32_t bytesToRead = 0;

        // Emits instructions generating one or more A64 OWORD block write instructions
        // The amount of data we need to write is n * Component Size OWORDs.
        // We can write 8, 4, or 2 OWORDs at a time. We can also write 1 OWORD,
        // but since this is a SIMD opcode and we're  compiling SIMD8, SIMD16,
        // we don't expect to see a 1 OWORD write.

        m_encoder->SetSimdSize( SIMDMode::SIMD1 );
        m_encoder->SetNoMask();
        m_encoder->SetSrcRegion( 0, 0, 1, 0 );

        CVariable* pTempVar = m_currShader->GetNewVariable(
            numLanes( SIMDMode::SIMD1 ),
            ISA_TYPE_UQ,
            EALIGN_QWORD, true );

        m_encoder->Copy(pTempVar, m_currShader->BitCast(src, ISA_TYPE_UQ));
        m_encoder->Push();

        while ( bytesRemaining )
        {
            if ( bytesRemaining >= 128 )
            {
                bytesRemaining -= 128;
                bytesToRead     = 128;
            }
            else if ( bytesRemaining >= 64 )
            {
                bytesRemaining -= 64;
                bytesToRead     = 64;
            }
            else if ( bytesRemaining >= 32 )
            {
                bytesRemaining -= 32;
                bytesToRead     = 32;
            }
            else  if (bytesRemaining >= 16)
            {
                bytesRemaining -= 16;
                bytesToRead = 16;
            }
            else
            {
                assert( 0 );
            }

            m_encoder->OWStoreA64( data, pTempVar, bytesToRead, srcOffset );
            srcOffset = srcOffset + bytesToRead;
            m_encoder->Push();

            if ( bytesRemaining )
            {
                if (m_currShader->m_Platform->hasNo64BitInst()) {
                    CVariable *ImmVar = m_currShader->ImmToVariable(bytesToRead, ISA_TYPE_UD);
                    emitAddPairWithImm(pTempVar, pTempVar, ImmVar);
                } else {
                    m_encoder->SetSimdSize( SIMDMode::SIMD1 );
                    m_encoder->SetNoMask();
                    m_encoder->SetSrcRegion( 0, 0, 1, 0 );
                    m_encoder->Add( pTempVar, pTempVar, m_currShader->ImmToVariable( ( bytesToRead ), ISA_TYPE_UQ ) );
                    m_encoder->Push();
                }
            }
        }
    }
    else
    {
        CVariable* src  = GetSymbol( llPtr );
        CVariable* data = GetSymbol( dataPtr );

        Type* Ty            = dataPtr->getType();
        VectorType* VTy     = dyn_cast< VectorType >( Ty );
        uint32_t nbElements = VTy ? int_cast<uint32_t>(VTy->getNumElements()) : 1;

        uint32_t bytesPerReg    = 32;
        uint32_t totalBytes     = nbElements * ( ( m_SimdMode == SIMDMode::SIMD8 ) ? 1 : 2 ) * bytesPerReg;
        if (dataPtr->getType()->getScalarSizeInBits() == 16) {
            totalBytes /= 2;
        }
        uint32_t bytesRemaining = totalBytes;

        // Emits instructions generating one or more OWORD block write instructions
        // The amount of data we need to write is n * Component Size OWORDs.
        // We can write 8, 4, or 2 OWORDs at a time. We can also write 1 OWORD,
        // but since this is a SIMD opcode and we're  compiling SIMD8, SIMD16,
        // we don't expect to see a 1 OWORD write.

        // shr( 16 ) r62.0<1>:ud r60.0<0; 1, 0>:ud 0x4:uw{ Align1, H1, NoMask }
        // mov( 1 ) r64.2<1>:ud r62.0<0; 1, 0>:ud{ Align1, NoMask, Compacted }
        // mov( 16 ) r65.0<1>:ud r54.0<8; 8, 1>:ud{ Align1, NoMask, Compacted }
        // and( 1 ) r64.5<1>:ud r0.5<0; 1, 0>:ud 0x3ff:ud{ Align1, NoMask }
        // send( 16 ) null<1>:uw r64 0xa 0x60a03ff:ud{ Align1, NoMask } oword block write

        CVariable* src0shifted = m_currShader->GetNewVariable(
            numLanes( m_SimdMode ),
            ISA_TYPE_UD,
            EALIGN_DWORD );

        m_encoder->SetSimdSize( m_SimdMode );
        m_encoder->SetNoMask();
        m_encoder->SetSrcRegion( 0, 0, 1, 0 );
        m_encoder->Shr( src0shifted, src, m_currShader->ImmToVariable( 4, ISA_TYPE_UD ) );
        m_encoder->Push();

        m_encoder->SetSimdSize( SIMDMode::SIMD1 );
        m_encoder->SetNoMask();
        m_encoder->SetSrcRegion( 0, 0, 1, 0 );

        CVariable* pTempVar = m_currShader->GetNewVariable(
            numLanes( SIMDMode::SIMD1 ),
            ISA_TYPE_UD,
            EALIGN_DWORD );

        m_encoder->Copy( pTempVar, src0shifted );
        m_encoder->Push();

        uint32_t srcOffset   = 0;
        uint32_t bytesToRead = 0;
        while ( bytesRemaining )
        {
            if ( bytesRemaining >= 128 )
            {
                bytesRemaining -= 128;
                bytesToRead     = 128;
            }
            else if ( bytesRemaining >= 64 )
            {
                bytesRemaining -= 64;
                bytesToRead     = 64;
            }
            else if ( bytesRemaining >= 32 )
            {
                bytesRemaining -= 32;
                bytesToRead     = 32;
            }
            else  if (bytesRemaining >= 16)
            {
                bytesRemaining -= 16;
                bytesToRead = 16;
            }
            else
            {
                assert( 0 );
            }

            m_encoder->OWStore( data, resource.m_surfaceType, resource.m_resource, pTempVar, bytesToRead, srcOffset );
            srcOffset = srcOffset + bytesToRead;
            m_encoder->Push();

            if ( bytesRemaining )
            {
                m_encoder->SetSimdSize( SIMDMode::SIMD1 );
                m_encoder->SetNoMask();
                m_encoder->SetSrcRegion( 0, 0, 1, 0 );
                m_encoder->Add( pTempVar, pTempVar, m_currShader->ImmToVariable( ( bytesToRead / 16 ), ISA_TYPE_UD ) ); // (bytesToRead / 16) is units of OWORDS
                m_encoder->Push();
            }
        }
    }
}

void EmitPass::emitSimdBlockReadGlobal( llvm::Instruction* inst )
{
    Value* llPtr          = inst->getOperand( 0 );
    PointerType* ptrType  = cast<PointerType>( llPtr->getType() );
    ResourceDescriptor resource = GetResourceVariable(llPtr);
    if(isA64Ptr(ptrType, m_currShader->GetContext()))
    {
        CVariable* src        = GetSymbol( llPtr );

        Type* Ty            = inst->getType();
        VectorType* VTy     = dyn_cast< VectorType >( Ty );
        uint32_t nbElements = VTy ? int_cast<uint32_t>(VTy->getNumElements()) : 1;

        uint32_t bytesPerReg = 32;
        uint32_t totalBytes  = nbElements * ( ( m_SimdMode == SIMDMode::SIMD8 ) ? 1 : 2 ) * bytesPerReg;
        if (inst->getType()->getScalarSizeInBits() == 16) {
            totalBytes /= 2;
        }

        uint32_t bytesRemaining = totalBytes;
        uint32_t dstSubReg      = 0;
        uint32_t bytesToRead    = 0;

        // Emits instructions generating one or more A64 OWORD block read instructions
        m_encoder->SetSimdSize( SIMDMode::SIMD1 );
        m_encoder->SetNoMask();
        m_encoder->SetSrcRegion( 0, 0, 1, 0 );

        CVariable* pTempVar = m_currShader->GetNewVariable(
            numLanes( SIMDMode::SIMD1 ),
            ISA_TYPE_UQ,
            EALIGN_QWORD, true );

        m_encoder->Copy( pTempVar, src );
        m_encoder->Push();

        while ( bytesRemaining )
        {
            if ( bytesRemaining >= 128 )
            {
                bytesRemaining -= 128;
                bytesToRead = 128;
            }
            else if ( bytesRemaining >= 64 )
            {
                bytesRemaining -= 64;
                bytesToRead = 64;
            }
            else if ( bytesRemaining >= 32 )
            {
                bytesRemaining -= 32;
                bytesToRead = 32;
            }
            else  if ( bytesRemaining >= 16 )
            {
                bytesRemaining -= 16;
                bytesToRead = 16;
            }
            else
            {
                assert( 0 );
            }

            m_encoder->SetDstSubVar( dstSubReg );
            dstSubReg = dstSubReg + 4;
            m_encoder->OWLoadA64( m_destination, pTempVar, bytesToRead );
            m_encoder->Push();

            if ( bytesRemaining )
            {
                if (m_currShader->m_Platform->hasNo64BitInst()) {
                    CVariable *ImmVar = m_currShader->ImmToVariable(bytesToRead, ISA_TYPE_UD);
                    emitAddPairWithImm(pTempVar, pTempVar, ImmVar);
                } else {
                    m_encoder->SetSimdSize( SIMDMode::SIMD1 );
                    m_encoder->SetNoMask();
                    m_encoder->SetSrcRegion( 0, 0, 1, 0 );
                    m_encoder->Add( pTempVar, pTempVar,  m_currShader->ImmToVariable( bytesToRead, ISA_TYPE_UQ ) );
                    m_encoder->Push();
                }
            }
        }
    }
    else
    {
        CVariable* src        = GetSymbol( llPtr );
        Type* Ty            = inst->getType();
        VectorType* VTy     = dyn_cast< VectorType >( Ty );
        uint32_t nbElements = VTy? int_cast<uint32_t>(VTy->getNumElements()) : 1;

        uint32_t bytesPerReg    = 32;
        uint32_t totalBytes     = nbElements * ( ( m_SimdMode == SIMDMode::SIMD8 ) ? 1 : 2 ) * bytesPerReg;
        if (inst->getType()->getScalarSizeInBits() == 16) {
            totalBytes /= 2;
        }

        // Emits below instructions generating one or more OWORD block read instructions:
        // mov (1)   r20.0<1>:ud r5.1<0;1,0>:ud {Align1, Q1, NoMask, Compacted}
        // and (1)   r21.5<1>:ud r0.5<0;1,0>:ud 0x3ff:ud {Align1, NoMask}
        // mov (1)   r21.2<1>:ud r20.0<0;1,0>:ud {Align1, NoMask, Compacted}
        // send (16) r12.0<1>:w  r21 0xa 0x24844ff:ud{Align1, NoMask}// unaligned oword block read

        m_encoder->SetSimdSize( SIMDMode::SIMD1 );
        m_encoder->SetNoMask();
        m_encoder->SetSrcRegion( 0, 0, 1, 0 );

        CVariable* pTempVar = m_currShader->GetNewVariable(
            numLanes( SIMDMode::SIMD1 ),
            ISA_TYPE_UD,
            EALIGN_DWORD );

        m_encoder->Copy( pTempVar, src );
        m_encoder->Push();

        uint32_t dstSubReg      = 0;
        uint32_t bytesToRead    = 0;
        uint32_t bytesRemaining = totalBytes;
        while ( bytesRemaining )
        {
            if ( bytesRemaining >= 128 )
            {
                bytesRemaining -= 128;
                bytesToRead     = 128;
            }
            else if ( bytesRemaining >= 64 )
            {
                bytesRemaining -= 64;
                bytesToRead     = 64;
            }
            else if ( bytesRemaining >= 32 )
            {
                bytesRemaining -= 32;
                bytesToRead     = 32;
            }
            else  if (bytesRemaining >= 16)
            {
                bytesRemaining -= 16;
                bytesToRead = 16;
            }
            else
            {
                assert( 0 );
            }
            m_encoder->SetDstSubVar( dstSubReg );
            dstSubReg = dstSubReg+4;
            m_encoder->OWLoad( m_destination, resource, pTempVar, false, bytesToRead );
            m_encoder->Push();

            if ( bytesRemaining )
            {
                m_encoder->SetSimdSize( SIMDMode::SIMD1 );
                m_encoder->SetNoMask();
                m_encoder->SetSrcRegion( 0, 0, 1, 0 );
                m_encoder->Add( pTempVar, pTempVar,  m_currShader->ImmToVariable(  bytesToRead, ISA_TYPE_UD ) );
                m_encoder->Push();
            }
        }
    }
}

void EmitPass::emitMediaBlockIO(const llvm::GenIntrinsicInst* inst, bool isRead)
{
    uint ImgArgIndex    = (uint)GetImmediateVal(inst->getOperand(0));
    uint isImageTypeUAV = (uint)GetImmediateVal(inst->getOperand(3));

    uint32_t BTI = isImageTypeUAV ?
        m_currShader->m_pBtiLayout->GetUavIndex(ImgArgIndex) :
        m_currShader->m_pBtiLayout->GetTextureIndex(ImgArgIndex);

    bool directIdx = (llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(0))) ? true : false;
    m_currShader->SetBindingTableEntryCountAndBitmap(directIdx, BTI);

    CVariable *pImgBTI = m_currShader->ImmToVariable(BTI, ISA_TYPE_UD);

    // width and height must be supplied as compile time constants.
    uint blockWidth  = (uint)cast<ConstantInt>(inst->getOperand(4))->getZExtValue();
    uint blockHeight = (uint)cast<ConstantInt>(inst->getOperand(5))->getZExtValue();

    auto *pFunc     = inst->getCalledFunction();
    auto *pDataType = isRead ? pFunc->getReturnType() : inst->getOperand(6)->getType();

    uint typeSize = isa<VectorType>(pDataType) ?
        (uint)m_DL->getTypeSizeInBits(cast<VectorType>(pDataType)->getVectorElementType()) / 8 :
        (uint)m_DL->getTypeSizeInBits(pDataType) / 8;

    uint widthInBytes = blockWidth * typeSize;

    CVariable *pXOffset = GetSymbol(inst->getOperand(1));
    CVariable *pYOffset = GetSymbol(inst->getOperand(2));

    CVariable *pDst = nullptr;

    auto *pData = isRead ? m_destination : BroadcastIfUniform(GetSymbol(inst->getOperand(6)));

    // For SIMD32, we need to rearrange the data from both halves
    // into a contiguous block to treat it as one SIMD32 write and
    // we need to split a read back into its two instances after
    // doing the read.
    bool mergeBlock = (m_SimdMode == SIMDMode::SIMD32);
    uint16_t numElements = pData->GetNumberElement();
    VISA_Type dataType = pData->GetType();

    if (mergeBlock)
    {
        // Make a block twice the size to hold data for both
        // halves.
        pDst = m_currShader->GetNewVariable(numElements * 2,
            dataType, pData->GetAlign(), false, 1);
    }
    else
    {
        pDst = pData;
    }

    auto BlockCopy = [&](
        CVariable* pDst1,
        CVariable *pSrc1,
        CVariable *pDst2,
        CVariable *pSrc2,
        uint srcStride,
        uint dstStride)
    {
        auto VecCopy = [&](CVariable *pDst, CVariable *pSrc, uint nElts)
        {
            for (uint32_t i = 0; i < nElts; ++i)
            {
                m_encoder->SetSrcSubReg(0, srcStride*16*i);
                m_encoder->SetDstSubReg(dstStride*16*i);
                m_encoder->Copy(pDst, pSrc);
                m_encoder->Push();
            }
        };

        uint nElts = isa<VectorType>(pDataType) ?
            cast<VectorType>(pDataType)->getVectorNumElements() :
            1;

        // Now, do the copies.
        bool isSecondHalf = m_encoder->IsSecondHalf();

        m_encoder->SetSecondHalf(false);
        VecCopy(pDst1, pSrc1, nElts);

        m_encoder->SetSecondHalf(true);
        VecCopy(pDst2, pSrc2, nElts);

        m_encoder->SetSecondHalf(isSecondHalf);
    };

    CVariable *pSecondHalf = m_currShader->GetNewAlias(pDst, dataType,
        16 * m_encoder->GetCISADataTypeSize(dataType), numElements);

    if (!isRead && mergeBlock)
    {
        BlockCopy(pDst, pData, pSecondHalf, pData, 1, 2);
    }

    m_encoder->MediaBlockMessage(
        isRead ? ISA_Opcode::ISA_MEDIA_LD : ISA_Opcode::ISA_MEDIA_ST,
        pDst,
        ESURFACE_NORMAL,
        pImgBTI,
        pXOffset,
        pYOffset,
        0,
        (unsigned char)widthInBytes,
        (unsigned char)blockHeight,
        0);

    if (isRead && mergeBlock)
    {
        BlockCopy(m_destination, pDst, m_destination, pSecondHalf, 2, 1);
    }
}

void EmitPass::emitMediaBlockRectangleRead(llvm::Instruction* inst)
{
    int SrcImgBTI = int_cast<int>(GetImmediateVal(inst->getOperand(0)));
    int isImageTypeUAV =  int_cast<int>(GetImmediateVal(inst->getOperand(3)));

    CVariable *xOffset = GetSymbol(inst->getOperand(1));
    CVariable *yOffset = GetSymbol(inst->getOperand(2));

    uint32_t bindingTableIndex = isImageTypeUAV ?
        m_currShader->m_pBtiLayout->GetUavIndex(SrcImgBTI) :
        m_currShader->m_pBtiLayout->GetTextureIndex(SrcImgBTI);

    bool directIdx = (llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(0))) ? true : false;
    m_currShader->SetBindingTableEntryCountAndBitmap(directIdx, bindingTableIndex);

    CVariable *srcbti = m_currShader->ImmToVariable(bindingTableIndex, ISA_TYPE_UD);

    CVariable *pDst = GetSymbol(inst->getOperand(6));

    // width and height must be supplied as compile time constants.
    uint64_t blockWidth  = cast<ConstantInt>(inst->getOperand(4))->getZExtValue();
    uint64_t blockHeight = cast<ConstantInt>(inst->getOperand(5))->getZExtValue();

    assert(blockWidth * blockHeight == pDst->GetSize());

    m_encoder->MediaBlockMessage(
        ISA_Opcode::ISA_MEDIA_LD,
        pDst,
        ESURFACE_NORMAL,
        srcbti,
        xOffset,
        yOffset,
        0,
        (unsigned char)blockWidth,
        (unsigned char)blockHeight,
        0);

    m_encoder->Push();
}

void EmitPass::emitSimdMediaBlockRead( llvm::Instruction* inst  )
{
    uint32_t nbElements = 1;
    if (inst->getType()->isVectorTy())
    {
        nbElements = inst->getType()->getVectorNumElements();
    }
    assert(nbElements <= 8 && "InValid Vector Size");

    int SrcImgBTI      = int_cast<int>(GetImmediateVal( inst->getOperand( 0 ) ));
    int isImageTypeUAV = int_cast<int>(GetImmediateVal( inst->getOperand( 3 ) ));

    Value* xOffset = inst->getOperand( 1 );
    Value* yOffset = inst->getOperand( 2 );

    uint32_t   pass              = 0;
    uint32_t   numPasses         = 0;
    uint32_t   bindingTableIndex = 0;

    uint32_t dstSubReg   = 0;
    uint32_t blockWidth  = 0;
    uint32_t blockHeight = nbElements;

    bool  fp16 = inst->getType()->getScalarType()->getScalarSizeInBits() == 16;

    if ( isImageTypeUAV )
    {
        bindingTableIndex = m_currShader->m_pBtiLayout->GetUavIndex( SrcImgBTI );
    }
    else // elseif imageType is Resource
    {
        bindingTableIndex = m_currShader->m_pBtiLayout->GetTextureIndex( SrcImgBTI );
    }

    m_currShader->SetBindingTableEntryCountAndBitmap(true, bindingTableIndex);

    CVariable* srcbti = m_currShader->ImmToVariable( bindingTableIndex, ISA_TYPE_UD );

    switch ( m_SimdMode )
    {
    case SIMDMode::SIMD8:
        blockWidth = fp16 ? 16 : 32;
        numPasses = 1;
        break;

    case SIMDMode::SIMD16:
        // 2 x SIMD8
        numPasses  = fp16 ? 1 : 2;
        blockWidth = 32;
        break;
    default:
        assert( 0 && "Wrong SIMD width" );
        break;
    }

    CVariable* pTempVar0 = nullptr;
    CVariable* pTempVar  = nullptr;

    CVariable* pTempDest = m_currShader->GetNewVariable(
        ( 2*blockHeight ) *numLanes( SIMDMode::SIMD8 ),
        m_destination->GetType(),
        EALIGN_DWORD );

    CVariable* xVar = GetSymbol( xOffset );
    CVariable* yVar = GetSymbol( yOffset );

    // Emits a MEDIA_BLOCK_READ instruction.
    // Considering block width as x-axis and block height as y axis:
    // Pass 0 reads from (xOffset,yOffset ) to (xOffset+31, yOffset+blockheight)
    // Pass 1 reads from (xOffset+32, yOffset) to (xOffset+63, yOffset+blockheight)
    // Instructions generated:
    // mov( 1 ) r36.1<1>:d r16.0<0; 1, 0>:d{ Align1, NoMask }
    // mov( 1 ) r36.2<1>:ud 0x3001f:ud{ Align1, NoMask }
    // mov( 1 ) r36.0<1>:ud r15.0<0; 1, 0>:ud{ Align1, NoMask, Compacted }
    // send( 8 ) r28.0<1>:ud r36 0xc 0x2490000:ud{ Align1, NoMask } // media block read
    // add( 1 ) r36.0<1>:ud r15.0<0; 1, 0>:ud 0x20:uw{ Align1, NoMask }
    // mov( 1 ) r36.1<1>:d r13.1<0; 1, 0>:d{ Align1, NoMask }
    // send( 8 ) r32.0<1>:ud r36 0xc 0x2490000:ud{ Align1, NoMask } // media block read

    for ( pass = 0; pass < numPasses; pass++ )
    {
        if ( pass == 0 )
        {
            m_encoder->SetSimdSize( SIMDMode::SIMD1 );
            m_encoder->SetNoMask();
            m_encoder->SetSrcRegion( 0, 0, 1, 0 );

            pTempVar0 = m_currShader->GetNewVariable(
                numLanes( m_SimdMode ),
                ISA_TYPE_UD,
                EALIGN_DWORD );

            m_encoder->Copy( pTempVar0, xVar );
            m_encoder->Push();

            m_encoder->SetSimdSize( SIMDMode::SIMD1 );
            m_encoder->SetNoMask();
            m_encoder->SetSrcRegion( 0, 0, 1, 0 );

            pTempVar = m_currShader->GetNewVariable(
                numLanes( m_SimdMode ),
                ISA_TYPE_UD,
                EALIGN_DWORD );

            m_encoder->Copy( pTempVar, yVar );
            m_encoder->Push();
        }
        if ( pass == 1 )
        {
            m_encoder->SetSimdSize( SIMDMode::SIMD1 );
            m_encoder->SetNoMask();
            m_encoder->SetSrcRegion( 0, 0, 1, 0 );
            m_encoder->Add( pTempVar0, pTempVar0, m_currShader->ImmToVariable( 32, ISA_TYPE_UD ) );
            m_encoder->Push();
            dstSubReg = dstSubReg + blockHeight;

            m_encoder->SetSimdSize( SIMDMode::SIMD1 );
            m_encoder->SetNoMask();
            m_encoder->SetSrcRegion( 0, 0, 1, 0 );
            pTempVar = m_currShader->GetNewVariable(
                numLanes( m_SimdMode ),
                ISA_TYPE_UD,
                EALIGN_DWORD );

            m_encoder->Copy( pTempVar, yVar );
            m_encoder->Push();
        }

        m_encoder->SetDstSubVar(dstSubReg);

        CVariable * dstVar = numPasses == 1 ? m_destination : pTempDest;

        m_encoder->MediaBlockMessage(
            ISA_Opcode::ISA_MEDIA_LD,
            dstVar,
            ESURFACE_NORMAL,
            srcbti,
            pTempVar0,
            pTempVar,
            0,
            ( unsigned char )blockWidth,
            ( unsigned char )blockHeight,
            0);

        m_encoder->Push();
    }

    if ( numPasses > 1 )
    {
        dstSubReg = 0;

        // Join data obtained from pass 0 and pass 1 to make
        // xOffset contiguous from 0 to 63 bytes (making SIMD 16)
        // mov (8) r20.0<1>:ud r28.0<8;8,1>:ud {Align1, Q1}
        // mov (8) r21.0<1>:ud r32.0<8;8,1>:ud {Align1, Q2}
        // mov (8) r22.0<1>:ud r29.0<8;8,1>:ud {Align1, Q1}
        // mov (8) r23.0<1>:ud r33.0<8;8,1>:ud {Align1, Q2}
        // mov (8) r24.0<1>:ud r30.0<8;8,1>:ud {Align1, Q1}
        // mov (8) r25.0<1>:ud r34.0<8;8,1>:ud {Align1, Q2}
        // mov (8) r26.0<1>:ud r31.0<8;8,1>:ud {Align1, Q1}
        // mov (8) r27.0<1>:ud r35.0<8;8,1>:ud {Align1, Q2}

        for ( uint32_t i = 0; i < blockHeight; i++ )
        {
            m_encoder->SetSimdSize( SIMDMode::SIMD8 );
            m_encoder->SetMask(EMASK_Q1);
            m_encoder->SetSrcSubVar( 0, i );
            m_encoder->SetDstSubVar( dstSubReg++ );
            m_encoder->Copy( m_destination, pTempDest );
            m_encoder->Push();

            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            m_encoder->SetMask(EMASK_Q2);
            m_encoder->SetSrcSubVar(0, i + blockHeight);
            m_encoder->SetDstSubVar(dstSubReg++);
            m_encoder->Copy(m_destination, pTempDest);
            m_encoder->Push();
        }
    }
}

void EmitPass::emitSimdMediaBlockWrite( llvm::Instruction* inst )
{
    int SrcImgBTI = int_cast<int>(GetImmediateVal( inst->getOperand( 0 ) ));
    int isImageTypeUAV = int_cast<int>(GetImmediateVal( inst->getOperand( 3 ) ));

    Value* xOffset = inst->getOperand( 1 );
    Value* yOffset = inst->getOperand( 2 );
    Value* dataPtr = inst->getOperand( 4 );

    uint32_t nbElements = 1;
    if (dataPtr->getType()->isVectorTy())
    {
        nbElements = dataPtr->getType()->getVectorNumElements();
    }
    assert(nbElements <= 8 && "InValid Vector Size");

    CVariable *data = GetSymbol(dataPtr);
    data            = BroadcastIfUniform( data );

    uint32_t   pass      = 0;
    uint32_t   numPasses = 0;

    uint32_t blockWidth        = 0;
    uint32_t blockHeight       = nbElements;
    uint32_t bindingTableIndex = 0;

    bool  fp16 = dataPtr->getType()->getScalarType()->getScalarSizeInBits() == 16;

    if ( isImageTypeUAV )
    {
        bindingTableIndex = m_currShader->m_pBtiLayout->GetUavIndex( SrcImgBTI );
    }
    else // elseif imageType is Resource
    {
        bindingTableIndex = m_currShader->m_pBtiLayout->GetTextureIndex( SrcImgBTI );
    }

    m_currShader->SetBindingTableEntryCountAndBitmap(true, bindingTableIndex);

    CVariable* srcbti = m_currShader->ImmToVariable( bindingTableIndex, ISA_TYPE_UD );

    switch (m_SimdMode)
    {
    case SIMDMode::SIMD8:
        blockWidth = fp16 ? 16 : 32;
        numPasses = 1;
        break;

    case SIMDMode::SIMD16:
        numPasses = fp16 ? 1 : 2;
        blockWidth = 32;
        break;
    default:
        assert(0 && "Wrong SIMD width");
        break;
    }

    CVariable* pTempVar0 = nullptr;
    CVariable* pTempVar  = nullptr;

    uint32_t dstSubReg = 0;

    for ( pass = 0; pass < numPasses; pass++ )
    {
        uint32_t srcSubVar = 0;
        uint32_t dstSubVar = 0;
        if ( pass == 1 )
        {
            srcSubVar = 1;
        }

        CVariable* tempdst = m_currShader->GetNewVariable(
            ( nbElements * numLanes( SIMDMode::SIMD8 ) ),
            data->GetType(),
            EALIGN_GRF );

        // Split the data.
        // mov (8) r22.0<1>:d r14.0<8;8,1>:d {Align1, Q1, Compacted}
        // mov (8) r23.0<1>:d r16.0<8;8,1>:d {Align1, Q1, Compacted}
        // mov (8) r24.0<1>:d r18.0<8;8,1>:d {Align1, Q1, Compacted}
        // mov (8) r25.0<1>:d r20.0<8;8,1>:d {Align1, Q1, Compacted}
        if ( numPasses > 1 )
        {
            for ( uint i = 0; i < nbElements; ++i )
            {
                m_encoder->SetMask( EMASK_Q1 );
                if ( pass == 1 )
                {
                    m_encoder->SetMask( EMASK_Q2 );
                }
                m_encoder->SetSimdSize( SIMDMode::SIMD8 );
                m_encoder->SetSrcSubVar( 0, srcSubVar );
                m_encoder->SetDstSubVar( dstSubVar++ );
                srcSubVar = srcSubVar + 2;
                m_encoder->Copy( tempdst, data );
                m_encoder->Push();
            }
        }
        else
        {
            tempdst = data;
        }
        // Emits a MEDIA_BLOCK_WRITE instruction.
        // Considering block width as x-axis and block height as y axis:
        // Pass 0 writes from (xOffset,yOffset ) to (xOffset+31, yOffset+blockheight)
        // Pass 1 writes from (xOffset+32, yOffset) to (xOffset+63, yOffset+blockheight)
        // mov (8) r28.0<1>:ud r0.0<8;8,1>:ud {Align1, NoMask, Compacted}
        // mov (1) r28.2<1>:ud 0x3001f:ud {Align1, NoMask}
        // mov (1) r28.0<1>:ud r6.0<0;1,0>:d {Align1, NoMask}
        // mov (1) r28.1<1>:ud r7.0<0;1,0>:d {Align1, NoMask}
        // mov (16) r29.0<1>:ud r22.0<8;8,1>:ud {Align1, NoMask, Compacted}
        // mov (16) r31.0<1>:ud r24.0<8;8,1>:ud {Align1, NoMask, Compacted}
        // send (8) null<1>:ud r28 0xc 0xa0a8002:ud{Align1, NoMask} // media block write
        if ( pass == 0 )
        {
            CVariable* xVar = GetSymbol( xOffset );
            CVariable* yVar = GetSymbol( yOffset );
            m_encoder->SetSimdSize( SIMDMode::SIMD1 );
            m_encoder->SetNoMask();
            m_encoder->SetSrcRegion( 0, 0, 1, 0 );

            pTempVar0 = m_currShader->GetNewVariable(
                numLanes( m_SimdMode ),
                ISA_TYPE_D,
                EALIGN_DWORD );

            m_encoder->Cast( pTempVar0, xVar );
            m_encoder->Push();
            m_encoder->SetSimdSize( SIMDMode::SIMD1 );
            m_encoder->SetNoMask();
            m_encoder->SetSrcRegion( 0, 0, 1, 0 );

            pTempVar = m_currShader->GetNewVariable(
                numLanes( m_SimdMode ),
                ISA_TYPE_D,
                EALIGN_DWORD );

            m_encoder->Cast( pTempVar, yVar );
            m_encoder->Push();
        }

        if ( pass == 1 )
        {
            m_encoder->SetSimdSize( SIMDMode::SIMD1 );
            m_encoder->SetNoMask();
            m_encoder->SetSrcRegion( 0, 0, 1, 0 );
            m_encoder->Add( pTempVar0, pTempVar0, m_currShader->ImmToVariable( 32, ISA_TYPE_UD ) );
            m_encoder->Push();
            dstSubReg = dstSubReg + blockHeight;
        }

        m_encoder->SetDstSubVar( dstSubReg );

        m_encoder->MediaBlockMessage(
            ISA_Opcode::ISA_MEDIA_ST,
            tempdst, ESURFACE_NORMAL,
            srcbti,
            pTempVar0,
            pTempVar,
            0,
            (unsigned char)blockWidth,
            (unsigned char)blockHeight,
            0 );
        m_encoder->Push();
    }
}

void EmitPass::emitDualBlendRT(llvm::RTDualBlendSourceIntrinsic* inst, bool fromRet)
{
    assert(m_currShader->GetShaderType() == ShaderType::PIXEL_SHADER);
    CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);

    uint RTIndex = inst->getRTIndexImm();
    bool oMask = inst->hasMask();
    bool outputDepth = inst->hasDepth();
    bool outputStencil = inst->hasStencil();
    Value* vMask = inst->getOMask();
    bool perSample = inst->perSample();
    Value* vSample = inst->getSampleIndex();

    uint bindingTableIndex = m_currShader->m_pBtiLayout->GetRenderTargetIndex(RTIndex);
    bool directIdx = (llvm::dyn_cast<llvm::ConstantInt>(inst->getRTIndex())) ? true : false;
    m_currShader->SetBindingTableEntryCountAndBitmap(directIdx, bindingTableIndex);

    CVariable* pMaskOpnd = nullptr;

    if (psProgram->HasDiscard())
    {
        ConstantInt* cv = dyn_cast<ConstantInt>(inst->getPMask());
        if (!cv || cv->getZExtValue() == 0)
        {
            pMaskOpnd = GetSymbol(inst->getPMask());
        }
    }

    bool isHF = false;
    uint messageLength = 8;

    if (inst->getRed0()->getType()->isHalfTy())
    {
        isHF = true;
        messageLength = 4;
    }

    uint responseLength = 0;
    if(outputDepth)
    {
        messageLength += 1;
    }
    if(outputStencil)
    {
        messageLength += 1;
    }
    if(oMask)
    {
        messageLength += 1;
    }
    // Need a header in case we write per sample
    bool needHeader = perSample;
    if(needHeader)
    {
        messageLength += 2;
    }
    int nbMessage = m_SimdMode == SIMDMode::SIMD8 ? 1 : 2;
    for(int i=0; i<nbMessage; i++)
    {
        uint payloadOffset = 0;
        bool lastRenderTarget = psProgram->IsLastRTWrite(inst);

        bool EOT = lastRenderTarget &&
            i == nbMessage-1 &&
            (m_encoder->IsSecondHalf() || m_currShader->m_numberInstance == 1);

        if (EOT)
        {
            assert(psProgram->m_hasEOT == false);
            psProgram->m_hasEOT = true;
        }

        CVariable* payload =
            m_currShader->GetNewVariable(messageLength*(SIZE_GRF>>2), ISA_TYPE_D, EALIGN_GRF);

        if(needHeader)
        {
            m_encoder->SetNoMask();
            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            m_encoder->Copy(payload, psProgram->GetR0());
            m_encoder->Push();

            m_encoder->SetDstSubVar(1);
            m_encoder->SetNoMask();
            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            m_encoder->Copy(payload, psProgram->GetR1());
            m_encoder->Push();
            if(perSample)
            {
                CVariable* sampleIndex = GetSymbol(vSample);
                if(!sampleIndex->IsUniform())
                {
                    sampleIndex = UniformCopy(sampleIndex);

                }
                CVariable* sampleIndexShifted = m_currShader->GetNewVariable(sampleIndex);
                m_encoder->Shl(sampleIndexShifted, sampleIndex, m_currShader->ImmToVariable(6, ISA_TYPE_D));

                m_encoder->SetNoMask();
                m_encoder->SetSimdSize(SIMDMode::SIMD1);
                m_encoder->SetSrcRegion(0, 0, 1, 0);
                m_encoder->Or(payload, payload, sampleIndexShifted);
                m_encoder->Push();
            }

            CVariable* pixelEnable = m_currShader->GetNewAlias(
                payload, ISA_TYPE_UW, SIZE_GRF + 14 * 2, 1);

            if (pMaskOpnd)
            {
                m_encoder->SetNoMask();
                m_encoder->SetSimdSize(SIMDMode::SIMD1);
                m_encoder->Copy(pixelEnable, pMaskOpnd);
                m_encoder->Push();
            }
            else
            if (!isa<ReturnInst>(inst->getParent()->getTerminator()))
            {
                m_encoder->SetNoMask();
                m_encoder->SetSimdSize(SIMDMode::SIMD1);
                m_encoder->Cast(pixelEnable, m_currShader->GetCE0());
                m_encoder->Push();
            }

            payloadOffset += 2;
        }

        if(oMask)
        {
            //oMask has to be packed since the hardware ignores the upper half
            CVariable* src = GetSymbol(vMask);
            CVariable* payloadUW = psProgram->BitCast(payload, ISA_TYPE_UW);
            src = psProgram->BitCast(src, ISA_TYPE_UW);
            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            m_encoder->SetMask(i == 0 ? EMASK_Q1 : EMASK_Q2);
            m_encoder->SetSrcSubVar(0, i);
            m_encoder->SetSrcRegion(0, 2, 1, 0);
            m_encoder->SetDstSubVar(payloadOffset++);
            m_encoder->SetDstSubReg(i * 8);
            m_encoder->Copy(payloadUW, src);
            m_encoder->Push();
        }

        CVariable* srcPayload = payload;

        if (isHF)
        {
            srcPayload = m_currShader->GetNewAlias(payload, ISA_TYPE_HF, 0, 4 * SIZE_GRF);
        }

        Value* colors[] = {
            inst->getRed0(), inst->getGreen0(), inst->getBlue0(), inst->getAlpha0(),
            inst->getRed1(), inst->getGreen1(), inst->getBlue1(), inst->getAlpha1()
        };

        for(uint srcIdx = 0; srcIdx<8; srcIdx++)
        {
            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            m_encoder->SetMask(i == 0 ? EMASK_Q1 : EMASK_Q2);
            CVariable* src = GetSymbol(colors[srcIdx]);

            if(!src->IsUniform())
            {
                m_encoder->SetSrcSubReg(0, i*8);
            }

            if ( isHF )
            {
                // half message has src0 and src1 interleaved
                if (srcIdx / 4 != 0)
                {
                    m_encoder->SetDstSubReg(8);
                }
                m_encoder->SetDstSubVar(payloadOffset + (srcIdx % 4));
            }
            else
            {
                m_encoder->SetDstSubVar(payloadOffset + srcIdx);
            }

            m_encoder->Copy(srcPayload, src);
            m_encoder->Push();
        }
        payloadOffset += isHF ? 4 : 8;

        if(outputDepth)
        {
            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            m_encoder->SetMask(i == 0 ? EMASK_Q1 : EMASK_Q2);
            CVariable* src = GetSymbol(inst->getDepth());
            if(!src->IsUniform())
            {
                m_encoder->SetSrcSubVar(0, i);
            }
            m_encoder->SetDstSubVar(payloadOffset++);
            m_encoder->Copy(payload, src);
            m_encoder->Push();
        }

        if(outputStencil)
        {
            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            m_encoder->SetMask(i == 0 ? EMASK_Q1 : EMASK_Q2);
            CVariable* src = GetSymbol(inst->getStencil());
            CVariable* ubSrc = m_currShader->BitCast(src, ISA_TYPE_UB);
            m_encoder->SetSrcRegion(0,32,8,4);
            if(!ubSrc->IsUniform())
            {
                m_encoder->SetSrcSubVar(0, i);
            }
            m_encoder->SetDstSubVar(payloadOffset++);
            m_encoder->Copy(payload, ubSrc);
            m_encoder->Push();
        }

        EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL messageType =
            ( i == 0 )
            ? EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL_SIMD8_DUAL_SOURCE_LOW :
            EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_CONTROL_SIMD8_DUAL_SOURCE_HIGH;

        uint Desc = PixelDataPort(
            isHF,
            messageLength,
            responseLength,
            needHeader,
            psProgram->GetPhase() == PSPHASE_COARSE,
            perSample,
            lastRenderTarget,
            m_encoder->IsSecondHalf(),
            messageType,
            bindingTableIndex);


        // TODO create a function to encode extended message
        CVariable* exDesc =
            psProgram->ImmToVariable(EU_MESSAGE_TARGET_DATA_PORT_WRITE | (EOT ? 1 << 5 : 0), ISA_TYPE_UD);
        CVariable* messDesc = psProgram->ImmToVariable(Desc, ISA_TYPE_UD);
        if(psProgram->GetPhase() == PSPHASE_PIXEL)
        {
            CVariable* temp = psProgram->GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true);
            m_encoder->Or(temp, psProgram->GetCurrentPhaseCounter(), exDesc);
            m_encoder->Push();
            exDesc = temp;
        }

        //sendc
        if (pMaskOpnd)
            m_encoder->SetPredicate(pMaskOpnd);
        m_encoder->SetSimdSize(SIMDMode::SIMD8);
        m_encoder->SetMask(i == 0 ? EMASK_Q1 : EMASK_Q2);
        m_encoder->SendC(NULL,payload, EU_MESSAGE_TARGET_DATA_PORT_WRITE, exDesc,messDesc);
        m_encoder->Push();
    }
}

void EmitPass::emitURBRead(llvm::GenIntrinsicInst* inst)
{
    CVariable* pPerSlotOffset = nullptr;
    CVariable* pVertexIndex = GetSymbol(inst->getOperand(0));

    QuadEltUnit globalOffset(0);
    if(ConstantInt* offset = dyn_cast<ConstantInt>(inst->getOperand(1)))
    {
        globalOffset = QuadEltUnit(int_cast<unsigned>(offset->getZExtValue()));
    }
    else
    {
        pPerSlotOffset = m_currShader->GetSymbol(inst->getOperand(1));
    }
    const EltUnit payloadSize( ( pPerSlotOffset != nullptr ) ? 2 : 1 );
    CVariable* pPayload =
        m_currShader->GetNewVariable(payloadSize.Count() * numLanes(m_SimdMode), ISA_TYPE_UD, EALIGN_GRF);

    TODO("Have VISA define the URBRead interface instead of using a raw send");
    Unit<Element> messageLength = payloadSize;
    Unit<Element> responseLength(m_destination->GetNumberElement()/numLanes(m_SimdMode));

    m_encoder->Copy(pPayload, m_currShader->GetURBInputHandle(pVertexIndex));
    m_encoder->Push();

    if( pPerSlotOffset != nullptr  )
    {
        m_encoder->SetDstSubVar(1);
        m_encoder->Copy(pPayload, pPerSlotOffset);
        m_encoder->Push();
    }

    uint desc = UrbMessage(
        messageLength.Count(),
        responseLength.Count(),
        false,
        pPerSlotOffset!=nullptr,
        false,
        m_currShader->GetFinalGlobalOffet(globalOffset).Count(),
        EU_GEN8_URB_OPCODE_SIMD8_READ);

    uint exDesc = EU_MESSAGE_TARGET_URB;
    CVariable* pMessDesc = m_currShader->ImmToVariable(desc, ISA_TYPE_UD);

    m_encoder->Send(m_destination, pPayload, exDesc, pMessDesc);
    m_encoder->Push();

    // mark input to be pulled
    m_currShader->isInputsPulled = true;
}

void EmitPass::emitURBWrite(llvm::GenIntrinsicInst* inst)
{
    // input: GenISA_URBWrite(%offset, %mask, %data0, ..., %data7)

    // If the offset or channel mask is not immediate value we need per-slot offsets and/or channel mask
    // to contain data in all the channels however,
    // if the variable is uniform, uniform analysis makes it a scalar value
    // we need to copy to simd form then.
    CVariable* channelMask = m_currShader->GetSymbol(inst->getOperand(1));
    if (!channelMask->IsImmediate())
    {
        channelMask = BroadcastIfUniform(channelMask);
    }

    CVariable* offset = m_currShader->GetSymbol(inst->getOperand(0));
    if (!offset->IsImmediate())
    {
        offset = BroadcastIfUniform(offset);
    }

    CVariable* URBHandle = m_currShader->GetURBOutputHandle();
    int payloadElementOffset = 0;

    CVariable* payload = m_CE->PrepareExplicitPayload(
        m_currShader,
        m_encoder,
        m_SimdMode,
        m_DL,
        inst,
        payloadElementOffset);

    m_encoder->URBWrite(payload, payloadElementOffset, offset, URBHandle, channelMask);
    m_encoder->Push();
}

void EmitPass::interceptSamplePayloadCoalescing(
    llvm::SampleIntrinsic* inst,
    uint numPart,
    //out:
    SmallVector<CVariable*, 4>& payload,
    bool& payloadCovered)
{
    m_CE->SetCurrentPart(inst, numPart);

    const uint numPayloadOperands = m_CE->GetNumPayloadElements(inst);
    CoalescingEngine::CCTuple* ccTuple = nullptr;
    int payloadToCCTupleRelativeOffset = 0;
    Value* representativeValPtr = nullptr;

    ccTuple = m_CE->IsAnyValueCoalescedInCCTuple(inst,
        numPayloadOperands,
        //out:
        payloadToCCTupleRelativeOffset,
        representativeValPtr
        );

    payloadCovered = m_CE->IsPayloadCovered(inst,
        ccTuple,
        numPayloadOperands,
        payloadToCCTupleRelativeOffset);

    if (payloadToCCTupleRelativeOffset < 0)
    {
        payloadCovered = false;
    }

    //Once we are here, there is no rolling back - all the conditions for preparing
    //a coalesced load/sample are satisfied at this point, so just proceed with
    //preparing one.
    if (!payloadCovered)
    {
        return;
    }
    else
    {
        assert(ccTuple);
        CVariable* rootPayloadVar = m_currShader->LazyCreateCCTupleBackingVariable(ccTuple);

        SmallPtrSet<Value*, 8> touchedValuesSet;

        assert(representativeValPtr);
        assert(payloadToCCTupleRelativeOffset >= 0);
        int byteOffset = payloadToCCTupleRelativeOffset *
            m_CE->GetSingleElementWidth(m_currShader->m_SIMDSize, m_DL, representativeValPtr);

        if(ccTuple->HasNonHomogeneousElements())
        {
            byteOffset += m_CE->GetLeftReservedOffset(ccTuple->GetRoot(), m_currShader->m_SIMDSize);
        }

        for (uint index = 0; index < numPayloadOperands; index++)
        {
            CVariable* src = nullptr;

            Value* val = m_CE->GetPayloadElementToValueMapping(inst, index);
            VISA_Type type = m_currShader->GetType(val->getType());

            bool needsAlias = false;
            if (touchedValuesSet.count(val))
            {
                //We have a copy of an element used at least twice in a payload.        
                src = m_currShader->GetNewAlias(rootPayloadVar, type, (uint16_t)byteOffset, 0);
                if(inst->IsDerivative())
                {
                    m_encoder->SetNoMask();
                }
                m_encoder->Copy(src, GetSymbol(val));
                m_encoder->Push();

                byteOffset += GetOffsetIncrement(m_DL, m_currShader->m_SIMDSize, val);

                assert(src);
                payload.push_back(src);
                continue;

            }
            else
            {
                touchedValuesSet.insert(val);
            }

            if (m_CE->IsValConstOrIsolated(val))
            {
                needsAlias = true;
            }
            else
            {
                if (CoalescingEngine::CCTuple* thisCCTuple = m_CE->GetValueCCTupleMapping(val))
                {
                    src = GetSymbol(val);
                }
                else
                {
                    //this one actually encompasses the case for !getRegRoot(val)
                    needsAlias = true;
                }
            } //if constant

            if (needsAlias)
            {
                src = m_currShader->GetNewAlias(rootPayloadVar, type, (uint16_t)byteOffset, 0);
                //TODO:WARNING: workaround
                if (inst->IsDerivative() /*&& GetSymbol(val)->IsUniform()*/)
                {
                    m_encoder->SetNoMask();
                }
                m_encoder->Copy(src, GetSymbol(val));
                m_encoder->Push();
            }
            assert(src);
            payload.push_back(src);

            byteOffset += GetOffsetIncrement(m_DL, m_currShader->m_SIMDSize, val);


        }

    }

}

ResourceDescriptor EmitPass::GetSampleResourceHelper(SampleIntrinsic* inst)
    {
    ResourceDescriptor resource;
    llvm::Value* texOp = inst->getTextureValue();
    if(texOp->getType()->isPointerTy())
    {
        resource = GetResourceVariable(texOp);
    }
    else
    {
        uint bindingTableIndex = m_currShader->m_pBtiLayout->GetTextureIndex(0);
        if(llvm::dyn_cast<llvm::ConstantInt>(texOp))
        {
            uint textureIdx = int_cast<uint>(GetImmediateVal(texOp));
            bindingTableIndex = m_currShader->m_pBtiLayout->GetTextureIndex(textureIdx);
            resource.m_resource = m_currShader->ImmToVariable(bindingTableIndex, ISA_TYPE_UD);

            m_currShader->SetBindingTableEntryCountAndBitmap(true, bindingTableIndex);
        }
        else
        {
            //Indexable texture, when index is not an immediate value
            CVariable* bti = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_GRF, true);
            bti = GetSymbol(texOp);
            resource.m_resource = IndexableResourceIndex(bti, bindingTableIndex);

            m_currShader->SetBindingTableEntryCountAndBitmap(false);
        }
    }
    return resource;
}

static bool isSIMD8_16bitReturn(CVariable* dst, SIMDMode simdMode)
{
    return simdMode == SIMDMode::SIMD8 &&
        CEncoder::GetCISADataTypeSize(dst->GetType()) == 2 &&
        !dst->isUnpacked();
}

void EmitPass::emitSampleInstruction(SampleIntrinsic* inst)
{
    EOPCODE opCode = GetOpCode(inst);
    ResourceDescriptor resource = GetSampleResourceHelper(inst);

    //Get sampler index in the array of operands
    llvm::Value* samplerOp = inst->getSamplerValue();
    SamplerDescriptor sampler = GetSamplerVariable(samplerOp);

    const uint numOperands = inst->getNumOperands();
    // offset
    CVariable* immOffset = m_currShader->ImmToVariable(0, ISA_TYPE_UW);
    if(!inst->IsLODInst())
    {
        uint offsetSourceIndex = numOperands-4;
        immOffset = ComputeSampleIntOffset(inst, offsetSourceIndex);
    }

    uint writeMask = m_currShader->GetExtractMask(inst);
    assert(writeMask != 0 && "Wrong write mask");

    bool derivativeSample = inst->IsDerivative();

    bool cpsEnable = derivativeSample &&
        m_currShader->GetShaderType() == ShaderType::PIXEL_SHADER &&
        static_cast<CPixelShader*>(m_currShader)->GetPhase() == PSPHASE_COARSE;

    SmallVector<CVariable*, 4>  payload;

    bool doIntercept = true;
    // Skip sample_d* instructions in SIMD16 and SIMD32.
    if ((m_currShader->m_SIMDSize > SIMDMode::SIMD8 &&
        (opCode == llvm_sample_d ||
        opCode == llvm_sample_dptr ||
        opCode == llvm_sample_dc ||
        opCode == llvm_sample_dcptr)))
    {
        doIntercept = false;
    }
    uint numSources = 0;
    const uint numParts = m_CE->GetNumSplitParts(inst);
    for (uint part = 0; part < numParts; part++)
    {
        bool payloadCovered = false;
        m_CE->SetCurrentPart(inst, part);
        const unsigned int numPartSources = m_CE->GetNumPayloadElements(inst);
        numSources += numPartSources;
        if (doIntercept)
        {
            interceptSamplePayloadCoalescing(inst, part, payload, payloadCovered);
        }

        if (!payloadCovered)
        {
            m_CE->SetCurrentPart(inst, part);

            //create send payload for numSources
            for (uint i = 0; i < numPartSources; i++)
            {
                Value* v = m_CE->GetPayloadElementToValueMapping(inst, i);
                CVariable* src = GetSymbol(v);
                if (src->IsUniform())
                {
                    CVariable* srcReg = m_currShader->GetNewVariable(numLanes(m_currShader->m_SIMDSize), src->GetType(), EALIGN_GRF);
                    if (derivativeSample)
                    {
                        m_encoder->SetNoMask();
                    }
                    m_encoder->Copy(srcReg, src);
                    m_encoder->Push();
                    src = srcReg;
                }
                payload.push_back(src);
            }
        }
    }

	// the responses to the sample + killpix and feedback messages have an extra register that contains a mask.
    bool hasMaskResponse = (writeMask & (1 << 4)) ? true : false;

    CVariable* dst =
		m_destination;
    //When sampler output is 16 bit float, hardware doesnt pack the output in SIMD8 mode.
    //Hence the movs to handle this layout in SIMD8 mode
    bool simd8HFRet = 
		isSIMD8_16bitReturn(m_destination, m_SimdMode);

    if (simd8HFRet)
    {
        dst = m_currShader->GetNewVariable(m_destination->GetNumberElement() * 2, ISA_TYPE_HF, EALIGN_GRF, false);
    }
    uint label = 0;
    CVariable* flag = nullptr;
    bool zeroLOD = m_currShader->m_Platform->supportSampleAndLd_lz() && inst->ZeroLOD();
    bool needLoop = ResourceLoopHeader(resource, sampler, flag, label);
    m_encoder->SetPredicate(flag);
    m_encoder->Sample(
        opCode, writeMask, immOffset, resource, sampler,
        numSources, dst, payload,
        zeroLOD, cpsEnable, hasMaskResponse, !sampler.m_sampler->IsUniform());
    m_encoder->Push();

    if (m_currShader->hasReadWriteImage(*(inst->getParent()->getParent()))
		)
    {
        m_encoder->Copy(m_currShader->GetNULL(), m_destination);
        m_encoder->Push();
        m_encoder->Copy(m_currShader->GetNULL(), m_currShader->GetTSC());
        m_encoder->Push();
    }
    ResourceLoop(needLoop, flag, label);

    if (simd8HFRet)
    {
        PackSIMD8HFRet(dst);
    }

    if(hasMaskResponse)
    {
        CVariable* flag = m_currShader->GetNewVariable(numLanes(m_currShader->m_dispatchSize), ISA_TYPE_BOOL, EALIGN_BYTE);
        uint subvar = numLanes(m_currShader->m_SIMDSize) / (SIZE_GRF >> 2) * 4;
        m_encoder->SetSrcSubVar(0, subvar);
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        CVariable* newdestination = m_currShader->BitCast(m_destination, ISA_TYPE_UD);
        m_encoder->SetP(flag, newdestination);
        m_encoder->Push();

        // Use integer types for select in case driver uses alt mode  
        // (0xFFFFFFFF is a NaN value, so the result is always 0).
        VISA_Type dstIntType = GetUnsignedIntegerType(m_destination->GetType());
        CVariable* pred = m_currShader->ImmToVariable(0xFFFFFFFF, dstIntType);
        CVariable* zero = m_currShader->ImmToVariable(0x0, dstIntType);
        CVariable* dstAlias = m_currShader->GetNewAlias(m_destination, dstIntType, 0, m_destination->GetNumberElement());
        m_encoder->SetDstSubVar(subvar);
        m_encoder->Select(flag, dstAlias, pred, zero );
        m_encoder->Push();
    }
}

// Initialize global discard mask as ~dmask.
void EmitPass::emitInitDiscardMask(llvm::GenIntrinsicInst* inst)
{
    if (m_encoder->IsSecondHalf())
        return;

    // (W) not (1|M0) f0.0:uw sr0.2<0;1,0>:ud
    CVariable* t = m_currShader->GetNewVariable(
        numLanes(m_currShader->m_dispatchSize),
        ISA_TYPE_BOOL, EALIGN_BYTE);
    m_encoder->SetNoMask();
    m_encoder->SetSrcSubReg(0, 2);
    m_encoder->SetP(t, m_currShader->GetSR0());
    m_encoder->Push();

    m_encoder->SetNoMask();
    m_encoder->SetSimdSize(m_currShader->m_dispatchSize);
    m_encoder->GenericAlu(EOPCODE_NOT, m_destination, t, nullptr);
    m_encoder->Push();

    assert(m_currShader->GetShaderType() == ShaderType::PIXEL_SHADER);
    CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);
    psProgram->SetDiscardPixelMask(m_destination);
}

// update global discard mask with discard condition
void EmitPass::emitUpdateDiscardMask(llvm::GenIntrinsicInst* inst)
{
    assert(m_currShader->GetShaderType() == ShaderType::PIXEL_SHADER);
    CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);
    CVariable* discardMask;
    CVariable* maskp;

    discardMask = GetSymbol(inst->getArgOperand(0));
    assert(discardMask == psProgram->GetDiscardPixelMask());

    if (ConstantInt* ci = dyn_cast<ConstantInt>(inst->getArgOperand(1)))
    {
        // if it's discard(true), then OR with ce0
        if (m_encoder->IsSecondHalf())
        {
            return;
        }

        if (ci->getZExtValue() == 1)
        {
            maskp = m_currShader->GetNewVariable(
                numLanes(m_currShader->m_dispatchSize),
                ISA_TYPE_BOOL, EALIGN_BYTE);

            m_encoder->SetNoMask();
            m_encoder->SetP(maskp, m_currShader->GetCE0());
            m_encoder->Push();

            m_encoder->SetNoMask();
            m_encoder->SetSimdSize(m_currShader->m_dispatchSize);
            m_encoder->Or(discardMask, discardMask, maskp);
            m_encoder->Push();
        }
        else
        {
            return;
        }
    }
    else
    {
        maskp = GetSymbol(inst->getArgOperand(1));
        m_encoder->Or(discardMask, discardMask, maskp);
        m_encoder->Push();
    }
}

// get live pixel mask for RTWrite from global discard mask
void EmitPass::emitGetPixelMask(llvm::GenIntrinsicInst* inst)
{
    assert(m_currShader->GetShaderType() == ShaderType::PIXEL_SHADER);
    CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);
    CVariable* globalMask;

    globalMask = GetSymbol(inst->getArgOperand(0));
    assert(globalMask == psProgram->GetDiscardPixelMask());

    CVariable* dst = m_destination;
    m_encoder->SetNoMask();
    m_encoder->GenericAlu(EOPCODE_NOT, dst, globalMask, nullptr);
    m_encoder->Push();
}

void EmitPass::emitDiscard(llvm::Instruction* inst)
{
    assert(false && "No codegen for discard intrinsic");
}

void EmitPass::emitInfoInstruction(InfoIntrinsic* inst)
{
    EOPCODE opCode = GetOpCode(inst);

    bool isUAV = false;
    if (opCode == llvm_resinfo)
    {
        isUAV = GetImmediateVal(inst->getOperand(2)) == 0 ? false : true;
    }
    else if (opCode == llvm_sampleinfo)
    {
        isUAV = GetImmediateVal(inst->getOperand(1)) == 0 ? false : true;
    }

    ResourceDescriptor resource;
    llvm::Value* texOp = inst->getOperand(0);

    if (texOp->getType()->isPointerTy())
    {
        resource = GetResourceVariable(texOp);
    }
    else
    {
        uint bindingTableIndex = m_currShader->m_pBtiLayout->GetTextureIndex(0);
        if (llvm::dyn_cast<llvm::ConstantInt>(texOp))
        {
            uint textureIdx = int_cast<uint>(GetImmediateVal(texOp));
            if ((opCode == llvm_sampleinfo || opCode == llvm_sampleinfoptr) &&
                textureIdx == -1)
            {
                //read from rasterizer
                bindingTableIndex = m_currShader->m_pBtiLayout->GetRenderTargetIndex(0);
            }
            else
            {
                if (isUAV)
                {
                    bindingTableIndex = m_currShader->m_pBtiLayout->GetUavIndex(textureIdx);
                }
                else
                {
                    bindingTableIndex = m_currShader->m_pBtiLayout->GetTextureIndex(textureIdx);
                }
            }
            resource.m_resource = m_currShader->ImmToVariable(bindingTableIndex, ISA_TYPE_UD);

            m_currShader->SetBindingTableEntryCountAndBitmap(true, bindingTableIndex);
        }
        else
        {
            //Indexable texture, when index is not an immediate value
            CVariable* bti = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_GRF, true);
            bti = GetSymbol(texOp);
            resource.m_resource = IndexableResourceIndex(bti, bindingTableIndex);

            m_currShader->SetBindingTableEntryCountAndBitmap(false);
        }
    }

    CVariable* lod = nullptr;
    if (opCode != llvm_sampleinfo && opCode != llvm_sampleinfoptr)
    {
        lod = GetSymbol(inst->getOperand(1));
    }
    if(lod && lod->IsUniform())
    {
        CVariable* srcReg = m_currShader->GetNewVariable(
            m_destination->IsUniform() ? 8 : numLanes(m_currShader->m_SIMDSize),
            ISA_TYPE_F,
            EALIGN_GRF,
            m_destination->IsUniform());
        m_encoder->SetUniformSIMDSize(SIMDMode::SIMD8);
        m_encoder->Copy(srcReg, lod);
        m_encoder->Push();
        lod = srcReg;
    }

    CVariable* tempDest = m_destination;
    if(m_destination->IsUniform())
    {
        tempDest = m_currShader->GetNewVariable(m_destination->GetNumberElement() * 8, ISA_TYPE_UD, EALIGN_GRF, true);
        m_encoder->SetUniformSIMDSize(SIMDMode::SIMD8);
    }

    uint label = 0;
    CVariable* flag = nullptr;
    bool needLoop = ResourceLoopHeader(resource, flag, label);
    m_encoder->SetPredicate(flag);

    uint writeMask = m_currShader->GetExtractMask(inst);
    m_encoder->Info(opCode, writeMask, resource, lod, tempDest);
    m_encoder->Push();

    ResourceLoop(needLoop, flag, label);

    if(tempDest != m_destination)
    {
        unsigned int writemask = 0;
        for(auto I = inst->user_begin(), E = inst->user_end(); I != E; ++I)
        {
            if(llvm::ExtractElementInst* extract = llvm::dyn_cast<llvm::ExtractElementInst>(*I))
            {
                if(llvm::ConstantInt* index = llvm::dyn_cast<ConstantInt>(extract->getIndexOperand()))
                {
                    writemask |= BIT(static_cast<uint>(index->getZExtValue()));
                    continue;
                }
            }
            writemask = 0xF;
            break;
        }
        for(uint i = 0; i < 4; i++)
        {
            if(BIT(i) & writemask)
            {
                m_encoder->SetSrcSubVar(0, i);
                m_encoder->SetDstSubReg(i);
                m_encoder->Copy(m_destination, tempDest);
                m_encoder->Push();
            }
        }
    }
}

void EmitPass::emitSurfaceInfo(GenIntrinsicInst* inst)
{
    ResourceDescriptor resource = GetResourceVariable(inst->getOperand(0));
    ForceDMask(false);

    uint messageSpecificControl = DataPortRead(
        1,
        2,
        false,
        EU_DATA_PORT_READ_MESSAGE_TYPE_SURFACE_INFO_READ,
        0,
        false,
        DATA_PORT_TARGET_CONSTANT_CACHE,
        resource.m_surfaceType == ESURFACE_BINDLESS ? BINDLESS_BTI : (uint)resource.m_resource->GetImmediateValue());

    CVariable* pMessDesc = m_currShader->ImmToVariable(messageSpecificControl, ISA_TYPE_D);
    CVariable* exDesc =
        m_currShader->ImmToVariable(EU_GEN6_MESSAGE_TARGET_DATA_PORT_CONSTANT_CACHE, ISA_TYPE_D);
    if(resource.m_surfaceType == ESURFACE_BINDLESS)
    {
        CVariable* temp = m_currShader->GetNewVariable(resource.m_resource);
        m_encoder->Add(temp, resource.m_resource, exDesc);
        m_encoder->Push();
        exDesc = temp;
    }
    uint label = 0;
    CVariable* flag = nullptr;
    bool needLoop = ResourceLoopHeader(resource, flag, label);
    CVariable* payload = m_currShader->GetNewVariable(8, ISA_TYPE_UD, EALIGN_GRF);

    m_encoder->SetSimdSize(SIMDMode::SIMD8);
    m_encoder->SetNoMask();
    m_encoder->Copy(payload, m_currShader->ImmToVariable(0, ISA_TYPE_UD));
    m_encoder->Push();

    m_encoder->SetUniformSIMDSize(SIMDMode::SIMD8);
    m_encoder->SetNoMask();
    m_encoder->Send(m_destination, payload,
        EU_GEN6_MESSAGE_TARGET_DATA_PORT_CONSTANT_CACHE, exDesc, pMessDesc);
    m_encoder->Push();

    assert(m_destination->IsUniform());
    ResourceLoop(needLoop, flag, label);
    ResetVMask(false);
}

void EmitPass::emitGather4Instruction(SamplerGatherIntrinsic* inst)
{
    EOPCODE opCode = GetOpCode(inst);
    uint numOperands = inst->getNumOperands();

    //Subtract the offsets, resource and sampler sources to get
    //the number of texture coordinates, src channel select and index to texture source
    uint numSources = numOperands - 7;

    Value* textureValue = inst->getTextureValue();
    ResourceDescriptor resource;
    if(textureValue->getType()->isPointerTy())
    {
        resource = GetResourceVariable(textureValue);
    }
    else
    {
        uint textureIdx = int_cast<uint>(GetImmediateVal(textureValue));
        uint bindingTableIndex = m_currShader->m_pBtiLayout->GetTextureIndex(textureIdx);
        resource.m_resource = m_currShader->ImmToVariable(bindingTableIndex, ISA_TYPE_UD);

        m_currShader->SetBindingTableEntryCountAndBitmap(true, bindingTableIndex);
    }

    SamplerDescriptor sampler;
    Value* samplerValue = inst->getSamplerValue();

    sampler = GetSamplerVariable(samplerValue);

    //Check for valid number of sources from the end of the list
    for(uint i = (numOperands - 7 - 1); i >= 1 ; i--)
    {
        CVariable* validSrc = GetSymbol(inst->getOperand(i));
        if(validSrc->IsImmediate() &&
            validSrc->GetImmediateValue() == 0)
        {
            numSources--;
        }
        else
        {
            break;
        }
    }

    // offset
    uint offsetSourceIndex = numOperands-5;
    CVariable* offset = ComputeSampleIntOffset(inst, offsetSourceIndex);

    uint channelIndx = numOperands-2;
    uint channel = int_cast<uint>(GetImmediateVal(inst->getOperand(channelIndx)));
    SmallVector<CVariable*, 4> payload;

    //create send payload for numSources
    for( uint i = 0; i < numSources; i++ )
    {
        CVariable* src = GetSymbol(inst->getOperand(i));
        if(src->IsUniform())
        {
            CVariable* srcReg = m_currShader->GetNewVariable(numLanes(m_currShader->m_SIMDSize), ISA_TYPE_F, EALIGN_GRF);
            m_encoder->Copy(srcReg, src);
            m_encoder->Push();
            src = srcReg;
        }
        payload.push_back(src);
    }

    CVariable* dst = m_destination;
    //When sampler output is 16 bit float, hardware doesnt pack the output in SIMD8 mode.
    //Hence the movs to handle this layout in SIMD8 mode
    bool simd8HFRet = isSIMD8_16bitReturn(m_destination, m_SimdMode);
    if (simd8HFRet)
    {
        dst = m_currShader->GetNewVariable(m_destination->GetNumberElement() * 2, ISA_TYPE_HF, EALIGN_GRF, false);
    }

    bool feedbackEnable = (m_destination->GetNumberElement()/numLanes(m_currShader->m_SIMDSize) == 5)? true : false;
    uint label = 0;
    CVariable* flag = nullptr;
    bool needLoop = ResourceLoopHeader(resource, sampler, flag, label);
    m_encoder->SetPredicate(flag);
    m_encoder->Gather4Inst(opCode, offset, resource, sampler, numSources, dst, payload, channel, feedbackEnable);
    m_encoder->Push();
    if (m_currShader->hasReadWriteImage(*(inst->getParent()->getParent())))
    {
        m_encoder->Copy(m_currShader->GetNULL(), m_destination);
        m_encoder->Push();
        m_encoder->Copy(m_currShader->GetNULL(), m_currShader->GetTSC());
        m_encoder->Push();
    }
    ResourceLoop(needLoop, flag, label);

    if (simd8HFRet)
    {
        PackSIMD8HFRet(dst);
    }


    if(feedbackEnable)
    {
        CVariable* flag = m_currShader->GetNewVariable(numLanes(m_currShader->m_dispatchSize), ISA_TYPE_BOOL, EALIGN_BYTE);
        uint subvar = numLanes(m_currShader->m_SIMDSize) / (SIZE_GRF >> 2) * 4;
        m_encoder->SetSrcSubVar(0, subvar);
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        CVariable* newdestination = m_currShader->BitCast(m_destination, ISA_TYPE_UD);
        m_encoder->SetP(flag, newdestination);
        m_encoder->Push();

        CVariable* pred = m_currShader->ImmToVariable(0xFFFFFFFF, m_destination->GetType());
        CVariable* zero = m_currShader->ImmToVariable(0x0, m_destination->GetType());
        m_encoder->SetDstSubVar(subvar);
        m_encoder->Select(flag, m_destination, pred, zero );
        m_encoder->Push();
    }
}

void EmitPass::emitLdmsInstruction(llvm::Instruction* inst)
{
    uint numOperands = inst->getNumOperands();
    EOPCODE opCode = GetOpCode(inst);
    //Subtract the offsets, and texture resource, lod to get
    //the number of texture coordinates and index to texture source
    uint numSources = numOperands - 5;
    uint textureArgIdx = numOperands - 5;

    for(uint i = numSources-1; i > 0 ; i--)
    {
        CVariable* validSrc = GetSymbol(inst->getOperand(i));
        if(!(validSrc->IsImmediate() && validSrc->GetImmediateValue() == 0))
        {
            break;
        }
        numSources--;
    }

    // Figure out the write mask from the size of the destination we want to write
    uint writeMask = m_currShader->GetExtractMask(inst);
    assert(writeMask!=0 && "Wrong write mask");

    ResourceDescriptor resource;
    Value* texOperand = inst->getOperand(textureArgIdx);
    if(texOperand->getType()->isPointerTy())
    {
        resource = GetResourceVariable(texOperand);
    }
    else
    {
        uint textureIdx = int_cast<uint>(GetImmediateVal(texOperand));
        uint bindingTableIndex = m_currShader->m_pBtiLayout->GetTextureIndex(textureIdx);
        resource.m_resource = m_currShader->ImmToVariable(bindingTableIndex, ISA_TYPE_UD);

        m_currShader->SetBindingTableEntryCountAndBitmap(true, bindingTableIndex);
    }

    uint offsetSourceIndex = numOperands-4;
    CVariable* offset = ComputeSampleIntOffset(inst, offsetSourceIndex);

    SmallVector<CVariable*, 4> payload;

    //create send payload for numSources
    for( uint i = 0; i < numSources; i++ )
    {
        CVariable* src = GetSymbol(inst->getOperand(i));
        src = BroadcastIfUniform(src);
        payload.push_back(src);
    }

    CVariable* dst = m_destination;
    //When sampler output is 16 bit float, hardware doesnt pack the output in SIMD8 mode.
    //Hence the movs to handle this layout in SIMD8 mode
    bool simd8HFRet = isSIMD8_16bitReturn(m_destination, m_SimdMode); 
    if (simd8HFRet)
    {
        dst = m_currShader->GetNewVariable(m_destination->GetNumberElement() * 2, ISA_TYPE_HF, EALIGN_GRF, false);
    }

    bool feedbackEnable = (writeMask & (1<<4))? true : false;
    uint label = 0;
    CVariable* flag = nullptr;
    bool needLoop = ResourceLoopHeader(resource, flag, label);
    m_encoder->SetPredicate(flag);
    m_encoder->LoadMS(opCode, writeMask, offset, resource, numSources, dst, payload, feedbackEnable);
    m_encoder->Push();
    if (m_currShader->hasReadWriteImage(*(inst->getParent()->getParent())))
    {
        m_encoder->Copy(m_currShader->GetNULL(), m_destination);
        m_encoder->Push();
        m_encoder->Copy(m_currShader->GetNULL(), m_currShader->GetTSC());
        m_encoder->Push();
    }
    ResourceLoop(needLoop, flag, label);

    if (simd8HFRet)
    {
        PackSIMD8HFRet(dst);
    }

    if(feedbackEnable)
    {
        CVariable* flag = m_currShader->GetNewVariable(numLanes(m_currShader->m_dispatchSize), ISA_TYPE_BOOL, EALIGN_BYTE);
        uint subvar = numLanes(m_currShader->m_SIMDSize) / (SIZE_GRF >> 2) * 4;
        m_encoder->SetSrcSubVar(0, subvar);
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        CVariable* newdestination = m_currShader->BitCast(m_destination, ISA_TYPE_UD);
        m_encoder->SetP(flag, newdestination);
        m_encoder->Push();

        CVariable* pred =  m_currShader->ImmToVariable(0xFFFFFFFF, m_destination->GetType());
        CVariable* zero =  m_currShader->ImmToVariable(0x0, m_destination->GetType());
        m_encoder->SetDstSubVar(subvar);
        m_encoder->Select(flag, m_destination, pred, zero );
        m_encoder->Push();
    }
}

void EmitPass::emitCSSGV(GenIntrinsicInst* inst)
{
    CComputeShader* csProgram = static_cast<CComputeShader*>(m_currShader);
    SGVUsage usage =
        static_cast<SGVUsage>(llvm::cast<llvm::ConstantInt>(inst->getOperand(0))->getZExtValue());
    CVariable* pThreadIdInGroup = nullptr;
    switch(usage)
    {
    case THREAD_GROUP_ID_X:
        {
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            if(csProgram->GetDispatchAlongY())
            {
                m_encoder->SetSrcSubReg(0, 6);
            }
            else
            {
            m_encoder->SetSrcSubReg(0,1);
            }
            m_encoder->Copy(m_destination, csProgram->GetR0());
            m_encoder->Push();
            break;
        }
    case THREAD_GROUP_ID_Y:
        {
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            if(csProgram->GetDispatchAlongY())
            {
                m_encoder->SetSrcSubReg(0, 1);
            }
            else
            {
            m_encoder->SetSrcSubReg(0,6);
            }
            m_encoder->Copy(m_destination, csProgram->GetR0());
            m_encoder->Push();
            break;
        }
    case THREAD_GROUP_ID_Z:
        {
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->SetSrcSubReg(0,7);
            m_encoder->Copy(m_destination, csProgram->GetR0());
            m_encoder->Push();
            break;
        }
    case THREAD_ID_IN_GROUP_X:
        {
            pThreadIdInGroup = csProgram->CreateThreadIDinGroup(0);
            m_currShader->CopyVariable(m_destination, pThreadIdInGroup);
            break;
        }
    case THREAD_ID_IN_GROUP_Y:
        {
            pThreadIdInGroup = csProgram->CreateThreadIDinGroup(1);
            m_currShader->CopyVariable(m_destination, pThreadIdInGroup);
            break;
        }
    case THREAD_ID_IN_GROUP_Z:
        {
            pThreadIdInGroup = csProgram->CreateThreadIDinGroup(2);
            m_currShader->CopyVariable(m_destination, pThreadIdInGroup);
            break;
        }
    default:
        break;
    }
}

void EmitPass::emitPSSGV(GenIntrinsicInst* inst)
{
    CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);
    CVariable* dst = m_destination;
    SGVUsage usage = (SGVUsage) llvm::cast<llvm::ConstantInt>(inst->getOperand(0))->getZExtValue();
    switch(usage)
    {
    case POSITION_Z:
    case POSITION_W:
        {
        if (psProgram->GetPhase() == PSPHASE_PIXEL || psProgram->GetPhase() == PSPHASE_COARSE)
            {
                e_interpolation mode = EINTERPOLATION_LINEAR;
                CVariable* bary = psProgram->GetBaryReg(mode);
                CVariable* delta =
                    (usage == POSITION_Z) ? psProgram->GetZDelta() : psProgram->GetWDelta();
                m_encoder->SetSrcRegion(0, 0, 4, 1);
                m_encoder->Pln(m_destination, delta, bary);
                m_encoder->Push();
            }
            else
            {
                CVariable* pPositionZW =
                    (usage == POSITION_Z) ? psProgram->GetPositionZ() : psProgram->GetPositionW();
                m_encoder->Copy(dst, pPositionZW);
                m_encoder->Push();
            }
            break;
        }
    case POSITION_X_OFFSET:
    case POSITION_Y_OFFSET:
        {
            //This returns to you the register value in the payload containing PSXYPositionOffset
            CVariable* pPositionXYOffset = psProgram->GetPositionXYOffset();
            //Access the correct subregion for the interleaved XY follow Spec
            m_encoder->SetSrcRegion(0, 16, 8, 2);
            m_encoder->SetSrcSubReg(0, usage == POSITION_X_OFFSET ? 0 : 1);

            //U4.4 encoding= upper 4 bits represent integer part and lower 4 bits represent decimal part
            //Extract integer part by AND with 11110000 and right shifting 4 bits
            CVariable* intVal_B = m_currShader->GetNewVariable(
                numLanes(m_currShader->m_SIMDSize),
                ISA_TYPE_B,
                EALIGN_GRF);
            m_encoder->And(intVal_B, pPositionXYOffset, m_currShader->ImmToVariable(0xf0, ISA_TYPE_B));
            m_encoder->Push();

            CVariable* intVal_F = m_currShader->GetNewVariable(
                numLanes(m_currShader->m_SIMDSize),
                ISA_TYPE_F,
                EALIGN_GRF);
            m_encoder->Shr(intVal_B, intVal_B, m_currShader->ImmToVariable(0x04, ISA_TYPE_B));
            m_encoder->Cast(intVal_F, intVal_B);
            m_encoder->Push();

            //Extract decimal part by AND with 00001111 and divide by 16
            CVariable* deciVal_B = m_currShader->GetNewVariable(
                numLanes(m_currShader->m_SIMDSize),
                ISA_TYPE_B,
                EALIGN_GRF);

            m_encoder->SetSrcRegion(0, 16, 8, 2);
            m_encoder->SetSrcSubReg(0, usage == POSITION_X_OFFSET ? 0 : 1);
            m_encoder->And(deciVal_B, pPositionXYOffset, m_currShader->ImmToVariable(0x0f, ISA_TYPE_B));
            m_encoder->Push();

            CVariable* deciVal_F = m_currShader->GetNewVariable(
                numLanes(m_currShader->m_SIMDSize),
                ISA_TYPE_F,
                EALIGN_GRF);
            m_encoder->Cast(deciVal_F, deciVal_B);
            //Divide lower 4 bits decimal value  by 16 = cheaper operation of cheaper Mul Op by 0.0625
            CVariable* temp = m_currShader->GetNewVariable(
                numLanes(m_currShader->m_SIMDSize),
                ISA_TYPE_F,
                EALIGN_GRF);
            m_encoder->Mul(temp, deciVal_F, m_currShader->ImmToVariable(0x3d800000, ISA_TYPE_F));
            m_encoder->Push();

            //Add decimal and integer to compute our PSXYOffsetValue
            m_encoder->Add(dst, intVal_F, temp);
            m_encoder->Push();
        }
        break;
    case RENDER_TARGET_ARRAY_INDEX:
    case VFACE:
        {
        unsigned int numTri = 1;
        SIMDMode simdSize = psProgram->m_SIMDSize;
        unsigned int subReg = 0;
        CVariable* reg = psProgram->GetR0();
        if(usage == VFACE)
        {
            for(unsigned int i = 0; i < numTri; i++)
            {
                CVariable* src = m_currShader->BitCast(reg, ISA_TYPE_W);
                m_encoder->SetSrcSubReg(0, 2*(subReg + i * 5));
                m_encoder->SetSrcRegion(0, 0, 1, 0);
                m_encoder->SetSimdSize(simdSize);
                m_encoder->SetMask(i == 0 ? EMASK_Q1 : EMASK_Q2);
                m_encoder->SetDstSubVar(i);
                m_encoder->Cast(dst, src);
                m_encoder->Push();
            }
        }
        else if(usage == RENDER_TARGET_ARRAY_INDEX)
        {
            dst = m_currShader->BitCast(dst, ISA_TYPE_UD);
            CVariable* temp = m_currShader->GetNewVariable(dst);
            for(unsigned int i = 0; i < numTri; i++)
            {
                m_encoder->SetSrcRegion(0, 0, 1, 0);
                m_encoder->SetSrcSubReg(0, subReg + i * 5);
                m_encoder->SetSimdSize(simdSize);
                m_encoder->SetMask(i == 0 ? EMASK_Q1 : EMASK_Q2);
                m_encoder->SetDstSubVar(i);
                m_encoder->Shr(temp, reg, m_currShader->ImmToVariable(16, ISA_TYPE_UD));
                m_encoder->Push();
            }
            m_encoder->And(dst, temp, m_currShader->ImmToVariable(BITMASK(11), ISA_TYPE_UD));
            m_encoder->Push();
        }
        break;
        }
    case SAMPLEINDEX:
        {
            //Sample index is stored in one half byte per subspan. we shift right
            //each lane with a different value to get the right number for each subspan
            //shr (8) r9.0<1>:uw   r1.0<0;1,0>:uw   0x0c080400:uv    { Align1, NoMask, Q1 }
            //and (16) r9.0<1>:ud   r9.0<1;4,0>:ud   0x0000000f:uw       { Align1, Q1 }
            CVariable* r1 = m_currShader->BitCast(psProgram->GetR1(), ISA_TYPE_UW);
            CVariable* imm = m_currShader->ImmToVariable(0x0C080400, ISA_TYPE_UV);
            CVariable* temp = m_currShader->GetNewVariable(8, ISA_TYPE_UW, EALIGN_GRF);
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            m_encoder->SetNoMask();
            m_encoder->Shr(temp, r1, imm);
            m_encoder->Push();

            dst = m_currShader->BitCast(dst, ISA_TYPE_UD);
            temp = m_currShader->BitCast(temp, ISA_TYPE_UD);

            imm = m_currShader->ImmToVariable(0x0000000F, ISA_TYPE_UD);
            m_encoder->SetSrcRegion(0, 1, 4, 0);
            m_encoder->And(dst,temp,imm);
            m_encoder->Push();
        }
        break;
    case INPUT_COVERAGE_MASK:
        {
             CVariable* pInputCoverageMask = psProgram->GetInputCoverageMask();
             m_encoder->Copy(dst, pInputCoverageMask);
             m_encoder->Push();
        }
        break;
    case ACTUAL_COARSE_SIZE_X:
    case ACTUAL_COARSE_SIZE_Y:
        {
            CVariable* CPSize = m_currShader->BitCast(psProgram->GetR1(), ISA_TYPE_UB);
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->SetSrcSubReg(0, usage==ACTUAL_COARSE_SIZE_X? 0:1);
            m_encoder->Cast(m_destination, CPSize);
            m_encoder->Push();
        }
        break;
    case REQUESTED_COARSE_SIZE_X:
    case REQUESTED_COARSE_SIZE_Y:
        {
            CVariable* requestedSize = (usage == REQUESTED_COARSE_SIZE_X) ?
                psProgram->GetCPSRequestedSizeX() : psProgram->GetCPSRequestedSizeY();
            m_encoder->SetSrcRegion(0, 1, 4, 0);
            m_encoder->Cast(m_destination, requestedSize);
            m_encoder->Push();
        }
        break;
    case MSAA_RATE:
    {
        dst = m_currShader->BitCast(dst, ISA_TYPE_UD);
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        m_encoder->SetSrcSubReg(0, 1);
        m_encoder->And(
            dst, 
            m_currShader->BitCast(psProgram->GetR1(), ISA_TYPE_UW), 
            m_currShader->ImmToVariable(BITMASK(4), ISA_TYPE_UW));
        m_encoder->Push();
    }
        break;
    default:
        assert(0);
        break;
    }
    psProgram->DeclareSGV(usage);
}

void EmitPass::emitDSSGV(llvm::GenIntrinsicInst* pInst)
{
    assert(m_currShader->GetShaderType()==ShaderType::DOMAIN_SHADER);
    CDomainShader* dsProgram = static_cast<CDomainShader*>(m_currShader);
    SGVUsage usage = static_cast<SGVUsage>(llvm::dyn_cast<llvm::ConstantInt>(pInst->getOperand(0))->getZExtValue());
    if(PRIMITIVEID == usage)
    {
        if (dsProgram->m_ShaderDispatchMode == ShaderDispatchMode::DUAL_PATCH)
        {
            m_encoder->SetSrcRegion(0, 4, 4, 0);
            m_encoder->SetSrcSubReg(0, 0);
            m_encoder->Copy(m_destination, dsProgram->GetPrimitiveID());
            m_encoder->Push();
        }
        else
        {
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->SetSrcSubReg(0, 1);
            m_encoder->Copy(m_destination, dsProgram->GetR0());
            m_encoder->Push();
        }
    }
}

void EmitPass::emitHSSGV(llvm::GenIntrinsicInst* pInst)
{
    assert(m_currShader->GetShaderType()==ShaderType::HULL_SHADER);
    CHullShader* hsProgram = static_cast<CHullShader*>(m_currShader);
    SGVUsage usage = static_cast<SGVUsage>(llvm::dyn_cast<llvm::ConstantInt>(pInst->getOperand(0))->getZExtValue());
    if (PRIMITIVEID == usage)
    {
        if(hsProgram->GetShaderDispatchMode() == SINGLE_PATCH_DISPATCH_MODE )
        {
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->SetSrcSubReg(0,1);
            m_encoder->Copy(m_destination, hsProgram->GetR0());
            m_encoder->Push();
        }
        else
        {
            // eight patch dispatch mode
            m_encoder->Copy(m_destination, hsProgram->GetR2());
            m_encoder->Push();
        }
    }
    else
    {
        assert(false && "Hull Shader SGV not supported");
    }
}

void EmitPass::emitPixelPosition(llvm::GenIntrinsicInst* inst)
{
    CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);
    GenISAIntrinsic::ID IID = inst->getIntrinsicID();
    CVariable* imm = m_currShader->ImmToVariable(
        IID == GenISAIntrinsic::GenISA_PixelPositionX ? 0x10101010 : 0x11001100, ISA_TYPE_V);
    CVariable* pixelSize = nullptr;
    if(psProgram->GetPhase() == PSPHASE_COARSE)
    {
        CVariable* CPSize = m_currShader->BitCast(psProgram->GetR1(), ISA_TYPE_UB);
        pixelSize =
            m_currShader->GetNewVariable(numLanes(m_currShader->m_SIMDSize), ISA_TYPE_UW, EALIGN_GRF);
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        m_encoder->SetSrcSubReg(0, IID == GenISAIntrinsic::GenISA_PixelPositionX ? 0 : 1);
        m_encoder->Mul(pixelSize, CPSize, imm);
        m_encoder->Push();
    }
    else
    {
        pixelSize = imm;
    }
    CVariable* position = m_currShader->BitCast(psProgram->GetR1(), ISA_TYPE_UW);
    m_encoder->SetSrcRegion(0, 2, 4, 0);
    // subreg 4 as position_x and subreg 5 as position_y
    m_encoder->SetSrcSubReg(0, IID == GenISAIntrinsic::GenISA_PixelPositionX ? 4 : 5);
    m_encoder->Add(m_destination, position, pixelSize);
    m_encoder->Push();
}

void EmitPass::emitSGV(SGVIntrinsic* inst)
{
    switch(m_currShader->GetShaderType())
    {
    case ShaderType::PIXEL_SHADER:
        emitPSSGV(inst);
        break;
    case ShaderType::COMPUTE_SHADER:
        emitCSSGV(inst);
        break;
    case ShaderType::DOMAIN_SHADER:
        emitDSSGV(inst);
        break;
    case ShaderType::HULL_SHADER:
        emitHSSGV(inst);
        break;
    case ShaderType::GEOMETRY_SHADER:
        emitGS_SGV(inst);
        break;
    default:
        assert(false && "This shader should not have SGV");
        break;
    }
}

void EmitPass::emitAluNoModifier(llvm::GenIntrinsicInst* inst)
{
    CVariable* pSrc0 = GetSymbol(inst->getOperand(0));
    CVariable* pSrc1;
    CVariable* pSrc2;
    CVariable* dst;

    switch(inst->getIntrinsicID())
    {
    case GenISAIntrinsic::GenISA_bfi:
        {
            pSrc1 = GetSymbol(inst->getOperand(1));
            pSrc2 = GetSymbol(inst->getOperand(2));
            CVariable* pSrc3 = GetSymbol(inst->getOperand(3));
            m_encoder->Bfi(m_destination, pSrc0, pSrc1, pSrc2, pSrc3);
        }
        break;
    case GenISAIntrinsic::GenISA_ibfe:
        pSrc1 = GetSymbol(inst->getOperand(1));
        pSrc2 = GetSymbol(inst->getOperand(2));
        m_encoder->Bfe(m_destination, pSrc0, pSrc1, pSrc2);
        break;
    case GenISAIntrinsic::GenISA_ubfe:
        pSrc1 = GetSymbol(inst->getOperand(1));
        pSrc2 = GetSymbol(inst->getOperand(2));
        pSrc0 = m_currShader->BitCast(pSrc0, ISA_TYPE_UD);
        pSrc1 = m_currShader->BitCast(pSrc1, ISA_TYPE_UD);
        pSrc2 = m_currShader->BitCast(pSrc2, ISA_TYPE_UD);
        dst = m_currShader->BitCast(m_destination, GetUnsignedType(m_destination->GetType()));
        m_encoder->Bfe(dst, pSrc0, pSrc1, pSrc2);
        break;
    case GenISAIntrinsic::GenISA_firstbitLo:
        dst = m_currShader->BitCast(m_destination, GetUnsignedType(m_destination->GetType()));
        m_encoder->Fbl(dst, pSrc0);
        break;
    case GenISAIntrinsic::GenISA_firstbitHi:
        pSrc0 = m_currShader->BitCast(pSrc0, ISA_TYPE_UD);
        dst = m_currShader->BitCast(m_destination, GetUnsignedType(m_destination->GetType()));
        m_encoder->Fbh(dst, pSrc0);
        break;
    case GenISAIntrinsic::GenISA_firstbitShi:
        dst = m_currShader->BitCast(m_destination, GetUnsignedType(m_destination->GetType()));
        m_encoder->Fbh(dst, pSrc0);
        break;
    default:
        break;
    }
    m_encoder->Push();
}

void EmitPass::EmitGenIntrinsicMessage(llvm::GenIntrinsicInst* inst)
{
    switch(inst->getIntrinsicID())
    {
    case GenISAIntrinsic::GenISA_OUTPUT:
        emitOutput(inst);
        break;
    case GenISAIntrinsic::GenISA_RTWrite:
        emitRenderTargetWrite(cast<RTWritIntrinsic>(inst), false);
        break;
    case GenISAIntrinsic::GenISA_RTDualBlendSource:
        emitDualBlendRT(cast<RTDualBlendSourceIntrinsic>(inst), false);
        break;
    case GenISAIntrinsic::GenISA_simdLaneId:
        emitSimdLaneId(inst);
        break;
    case GenISAIntrinsic::GenISA_patchInstanceId:
        emitPatchInstanceId(inst);
        break;
    case GenISAIntrinsic::GenISA_simdSize:
        emitSimdSize(inst);
        break;
      break;
    case GenISAIntrinsic::GenISA_simdShuffleDown:
        emitSimdShuffleDown(inst);
        break;
    case GenISAIntrinsic::GenISA_simdBlockReadGlobal:
        emitSimdBlockReadGlobal(inst);
        break;
    case GenISAIntrinsic::GenISA_simdBlockWriteGlobal:
        emitSimdBlockWriteGlobal(inst);
        break;
    case GenISAIntrinsic::GenISA_MediaBlockRead:
        emitMediaBlockIO(inst, true);
        break;
    case GenISAIntrinsic::GenISA_MediaBlockWrite:
        emitMediaBlockIO(inst, false);
        break;
    case GenISAIntrinsic::GenISA_MediaBlockRectangleRead:
        emitMediaBlockRectangleRead(inst);
        break;
    case GenISAIntrinsic::GenISA_simdMediaBlockRead:
        emitSimdMediaBlockRead(inst);
        break;
    case GenISAIntrinsic::GenISA_simdMediaBlockWrite:
        emitSimdMediaBlockWrite(inst);
        break;
    case GenISAIntrinsic::GenISA_RenderTargetRead:
    case GenISAIntrinsic::GenISA_RenderTargetReadSampleFreq:
        emitRenderTargetRead(inst);
        break;
    case GenISAIntrinsic::GenISA_URBWrite:
        emitURBWrite(inst);
        break;
    case GenISAIntrinsic::GenISA_URBRead:
        emitURBRead(inst);
        break;
    case GenISAIntrinsic::GenISA_cycleCounter:
        emitcycleCounter(inst);
        break;
    case GenISAIntrinsic::GenISA_SetDebugReg:
        emitSetDebugReg(inst);
        break;
     case GenISAIntrinsic::GenISA_vmeSendIME:
        emitVMESendIME(inst);
        break;
     case GenISAIntrinsic::GenISA_vmeSendIME2:
       emitVMESendIME2(inst);
       break;
     case GenISAIntrinsic::GenISA_vmeSendFBR:
         emitVMESendFBR(inst);
         break;
     case GenISAIntrinsic::GenISA_vmeSendFBR2:
        emitVMESendFBR2(inst);
        break;
    case GenISAIntrinsic::GenISA_vmeSendSIC2:
        emitVMESendSIC2(inst);
        break;
    case GenISAIntrinsic::GenISA_vmeSendSIC:
        emitVMESendSIC(inst);
        break;
    case GenISAIntrinsic::GenISA_vaErode:
    case GenISAIntrinsic::GenISA_vaDilate:
    case GenISAIntrinsic::GenISA_vaMinMax:
        emitVideoAnalyticSLM( inst, 1 );
        break;
    case GenISAIntrinsic::GenISA_vaMinMaxFilter:
        emitVideoAnalyticSLM( inst, 8 );
        break;
    case GenISAIntrinsic::GenISA_vaConvolve:
    case GenISAIntrinsic::GenISA_vaCentroid:
        emitVideoAnalyticSLM( inst, 4 );
        break;
    case GenISAIntrinsic::GenISA_vaConvolveGRF_16x1:
    case GenISAIntrinsic::GenISA_vaConvolveGRF_16x4:
        emitVideoAnalyticGRF(inst, 1);
        break;
    case GenISAIntrinsic::GenISA_vaBoolSum:
    case GenISAIntrinsic::GenISA_vaBoolCentroid:
        emitVideoAnalyticSLM( inst, 2 );
        break;
    case GenISAIntrinsic::GenISA_createMessagePhasesNoInit:
    case GenISAIntrinsic::GenISA_createMessagePhasesNoInitV:
        break;
    case GenISAIntrinsic::GenISA_createMessagePhases:
    case GenISAIntrinsic::GenISA_createMessagePhasesV:
        emitCreateMessagePhases(inst);
        break;
    case GenISAIntrinsic::GenISA_getMessagePhaseX:
    case GenISAIntrinsic::GenISA_getMessagePhaseXV:
        emitGetMessagePhaseX(inst);
        break;
    case GenISAIntrinsic::GenISA_simdGetMessagePhase:
    case GenISAIntrinsic::GenISA_simdGetMessagePhaseV:
        emitSimdGetMessagePhase(inst);
        break;
    case GenISAIntrinsic::GenISA_broadcastMessagePhase:
    case GenISAIntrinsic::GenISA_broadcastMessagePhaseV:
        emitBroadcastMessagePhase(inst);
        return;
    case GenISAIntrinsic::GenISA_simdSetMessagePhase:
    case GenISAIntrinsic::GenISA_simdSetMessagePhaseV:
        emitSimdSetMessagePhase(inst);
        break;
    case GenISAIntrinsic::GenISA_simdMediaRegionCopy:
        emitSimdMediaRegionCopy(inst);
        break;
    case GenISAIntrinsic::GenISA_extractMVAndSAD:
        emitExtractMVAndSAD(inst);
        break;
    case GenISAIntrinsic::GenISA_cmpSADs:
        emitCmpSADs(inst);
        break;
    case GenISAIntrinsic::GenISA_setMessagePhaseX_legacy:
        emitSetMessagePhaseX_legacy(inst);
        break;
    case GenISAIntrinsic::GenISA_setMessagePhase_legacy:
        emitSetMessagePhase_legacy(inst);
        break;
    case GenISAIntrinsic::GenISA_setMessagePhaseX:
    case GenISAIntrinsic::GenISA_setMessagePhaseXV:
        emitSetMessagePhaseX(inst);
        break;
    case GenISAIntrinsic::GenISA_getMessagePhase:
    case GenISAIntrinsic::GenISA_getMessagePhaseV:
        emitGetMessagePhase(inst);
        break;
    case GenISAIntrinsic::GenISA_setMessagePhase:
    case GenISAIntrinsic::GenISA_setMessagePhaseV:
        emitSetMessagePhase(inst);
        break;
    case GenISAIntrinsic::GenISA_DCL_ShaderInputVec:
    case GenISAIntrinsic::GenISA_DCL_inputVec:
        emitInput(inst);
        break;
    case GenISAIntrinsic::GenISA_PullSampleIndexBarys:
    case GenISAIntrinsic::GenISA_PullSnappedBarys:
        emitEvalAttribute(inst);
        break;
    case GenISAIntrinsic::GenISA_Interpolate:
        emitInterpolate(inst);
        break;
    case GenISAIntrinsic::GenISA_DCL_DSCntrlPtInputVec:
        emitInput(inst);
        break;
    case GenISAIntrinsic::GenISA_ld:
        emitLdInstruction(inst, false);
        break;
    case GenISAIntrinsic::GenISA_ldptr:
        emitLdInstruction(inst, true);
        break;
    case GenISAIntrinsic::GenISA_sample:
    case GenISAIntrinsic::GenISA_sampleptr:
    case GenISAIntrinsic::GenISA_sampleB:
    case GenISAIntrinsic::GenISA_sampleBptr:
    case GenISAIntrinsic::GenISA_sampleC:
    case GenISAIntrinsic::GenISA_sampleCptr:
    case GenISAIntrinsic::GenISA_sampleD:
    case GenISAIntrinsic::GenISA_sampleDptr:
    case GenISAIntrinsic::GenISA_sampleDC:
    case GenISAIntrinsic::GenISA_sampleDCptr:
    case GenISAIntrinsic::GenISA_sampleL:
    case GenISAIntrinsic::GenISA_sampleLptr:
    case GenISAIntrinsic::GenISA_sampleLC:
    case GenISAIntrinsic::GenISA_sampleLCptr:
    case GenISAIntrinsic::GenISA_sampleBC:
    case GenISAIntrinsic::GenISA_sampleBCptr:
    case GenISAIntrinsic::GenISA_lod:
    case GenISAIntrinsic::GenISA_lodptr:
    case GenISAIntrinsic::GenISA_sampleKillPix:
        emitSampleInstruction(cast<SampleIntrinsic>(inst));
        break;
    case GenISAIntrinsic::GenISA_discard:
        emitDiscard(inst);
        break;
    case GenISAIntrinsic::GenISA_resinfo:
    case GenISAIntrinsic::GenISA_resinfoptr:
    case GenISAIntrinsic::GenISA_sampleinfo:
    case GenISAIntrinsic::GenISA_sampleinfoptr:
        emitInfoInstruction(cast<InfoIntrinsic>(inst));
        break;
    case GenISAIntrinsic::GenISA_gather4:
    case GenISAIntrinsic::GenISA_gather4ptr:
    case GenISAIntrinsic::GenISA_gather4C:
    case GenISAIntrinsic::GenISA_gather4Cptr:
    case GenISAIntrinsic::GenISA_gather4PO:
    case GenISAIntrinsic::GenISA_gather4POptr:
    case GenISAIntrinsic::GenISA_gather4POC:
    case GenISAIntrinsic::GenISA_gather4POCptr:
        emitGather4Instruction(cast<SamplerGatherIntrinsic>(inst));
        break;
    case GenISAIntrinsic::GenISA_ldmcs:
    case GenISAIntrinsic::GenISA_ldmcsptr:
    case GenISAIntrinsic::GenISA_ldms:
    case GenISAIntrinsic::GenISA_ldmsptr:
    case GenISAIntrinsic::GenISA_ldmsptr16bit:
        emitLdmsInstruction(inst);
        break;
    case GenISAIntrinsic::GenISA_DCL_SystemValue:
        emitSGV(cast<SGVIntrinsic>(inst));
        break;
    case GenISAIntrinsic::GenISA_PixelPositionX:
    case GenISAIntrinsic::GenISA_PixelPositionY:
        emitPixelPosition(inst);
        break;
    case GenISAIntrinsic::GenISA_DCL_GSsystemValue:
        emitGS_SGV(cast<SGVIntrinsic>(inst));
        break;
    case GenISAIntrinsic::GenISA_SampleOffsetX:
    case GenISAIntrinsic::GenISA_SampleOffsetY:
        emitSampleOffset(inst);
        break;
    case GenISAIntrinsic::GenISA_ldstructured:
        emitLdStructured(inst);
        break;
    case GenISAIntrinsic::GenISA_storestructured1:
    case GenISAIntrinsic::GenISA_storestructured2:
    case GenISAIntrinsic::GenISA_storestructured3:
    case GenISAIntrinsic::GenISA_storestructured4:
        emitStoreStructured(inst);
        break;
    case GenISAIntrinsic::GenISA_typedread:
        emitTypedRead(inst);
        break;
    case GenISAIntrinsic::GenISA_typedwrite:
        emitTypedWrite(inst);
        break;
    case GenISAIntrinsic::GenISA_threadgroupbarrier:
        emitThreadGroupBarrier(inst);
        break;
    case GenISAIntrinsic::GenISA_memoryfence:
        emitMemoryFence(inst);
        break;
    case GenISAIntrinsic::GenISA_flushsampler:
        emitFlushSamplerCache(inst);
        break;
    case GenISAIntrinsic::GenISA_intatomicraw:
    case GenISAIntrinsic::GenISA_floatatomicraw:
    case GenISAIntrinsic::GenISA_intatomicrawA64:
    case GenISAIntrinsic::GenISA_floatatomicrawA64:
    case GenISAIntrinsic::GenISA_icmpxchgatomicraw:
    case GenISAIntrinsic::GenISA_fcmpxchgatomicraw:
    case GenISAIntrinsic::GenISA_icmpxchgatomicrawA64:
    case GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64:
        emitAtomicRaw(inst);
        break;
    case GenISAIntrinsic::GenISA_dwordatomicstructured:
    case GenISAIntrinsic::GenISA_floatatomicstructured:
    case GenISAIntrinsic::GenISA_cmpxchgatomicstructured:
    case GenISAIntrinsic::GenISA_fcmpxchgatomicstructured:
        emitAtomicStructured(inst);
        break;
    case GenISAIntrinsic::GenISA_intatomictyped:
    case GenISAIntrinsic::GenISA_icmpxchgatomictyped:
        emitAtomicTyped(inst);
        break;
    case GenISAIntrinsic::GenISA_atomiccounterinc:
    case GenISAIntrinsic::GenISA_atomiccounterpredec:
        emitAtomicCounter(inst);
        break;
    case GenISAIntrinsic::GenISA_bfi:
    case GenISAIntrinsic::GenISA_ubfe:
    case GenISAIntrinsic::GenISA_ibfe:
    case GenISAIntrinsic::GenISA_firstbitLo:
    case GenISAIntrinsic::GenISA_firstbitHi:
    case GenISAIntrinsic::GenISA_firstbitShi:
        emitAluNoModifier(inst);
        break;
    case GenISAIntrinsic::GenISA_OutputTessFactors:
        emitHSTessFactors(inst);
        break;
    case GenISAIntrinsic::GenISA_DCL_HSPatchConstInputVec:
    case GenISAIntrinsic::GenISA_HSURBPatchHeaderRead:
        emitHSPatchConstantInput(inst);
        break;
    case GenISAIntrinsic::GenISA_DCL_HSOutputCntrlPtInputVec:
        emitHSOutputControlPtInput(inst);
        break;
    case GenISAIntrinsic::GenISA_f32tof16_rtz:
        emitf32tof16_rtz(inst);
        break;
    case GenISAIntrinsic::GenISA_ftoi_rtn:
    case GenISAIntrinsic::GenISA_ftoi_rtp:
    case GenISAIntrinsic::GenISA_ftoi_rte:
    case GenISAIntrinsic::GenISA_ftoui_rtn:
    case GenISAIntrinsic::GenISA_ftoui_rtp:
    case GenISAIntrinsic::GenISA_ftoui_rte:
    case GenISAIntrinsic::GenISA_ftof_rtn:
    case GenISAIntrinsic::GenISA_ftof_rtp:
    case GenISAIntrinsic::GenISA_ftof_rtz:
        emitfptrunc(inst);
        break;
    case GenISAIntrinsic::GenISA_itof_rtn:
    case GenISAIntrinsic::GenISA_itof_rtp:
    case GenISAIntrinsic::GenISA_itof_rtz:
    case GenISAIntrinsic::GenISA_uitof_rtn:
    case GenISAIntrinsic::GenISA_uitof_rtp:
    case GenISAIntrinsic::GenISA_uitof_rtz:
        emititof(inst);
        break;
    case GenISAIntrinsic::GenISA_uavSerializeAll:
    case GenISAIntrinsic::GenISA_uavSerializeOnResID:
        emitUAVSerialize();
        break;
    case GenISAIntrinsic::GenISA_globalSync:
        emitMemoryFence();
        break;
    case GenISAIntrinsic::GenISA_WorkGroupAny:
        emitWorkGroupAny(inst);
        break;
    case GenISAIntrinsic::GenISA_PHASE_OUTPUT:
        emitPhaseOutput(inst);
        break;
    case GenISAIntrinsic::GenISA_PHASE_INPUT:
        emitPhaseInput(inst);
        break;
    case GenISAIntrinsic::GenISA_sqrt:
        emitSqrt(inst);
        break;
    case GenISAIntrinsic::GenISA_ldrawvector_indexed:
    case GenISAIntrinsic::GenISA_ldraw_indexed:
        emitLoadRawIndexed(inst);
        break;
    case GenISAIntrinsic::GenISA_storerawvector_indexed:
    case GenISAIntrinsic::GenISA_storeraw_indexed:
        emitStoreRawIndexed(inst);
        break;
    case GenISAIntrinsic::GenISA_GetBufferPtr:
        emitGetBufferPtr(inst);
        break;
    case GenISAIntrinsic::GenISA_readsurfaceinfoptr:
        emitSurfaceInfo(inst);
        break;

    case GenISAIntrinsic::GenISA_source_value:{
        m_encoder->Copy(m_currShader->GetNULL(), GetSymbol(inst->getOperand(0)));
        m_encoder->Push();
        break;
    }
    case GenISAIntrinsic::GenISA_movcr:{
        m_encoder->SetSrcSubReg(0, static_cast<uint16_t>(GetImmediateVal(inst->getOperand(0))));
        m_encoder->Copy(m_destination, m_currShader->GetCR0());
        m_encoder->Push();
        break;
    }
    case GenISAIntrinsic::GenISA_hw_thread_id:{
        m_encoder->Copy(m_destination, m_currShader->GetHWTID());
        m_encoder->Push();
        break;
    }
    case GenISAIntrinsic::GenISA_slice_id:{
        if (m_currShader->m_Platform->GetPlatformFamily() == IGFX_GEN8_CORE ||
            m_currShader->m_Platform->GetPlatformFamily() == IGFX_GEN9_CORE)
            emitStateRegID(0x0000C000, 0x0000000E);
        else
            emitStateRegID(0x00001C00, 0x0000000A);
        break;
    }
    case GenISAIntrinsic::GenISA_subslice_id:{
        if (m_currShader->m_Platform->GetPlatformFamily() == IGFX_GEN8_CORE ||
            m_currShader->m_Platform->GetPlatformFamily() == IGFX_GEN9_CORE)
            emitStateRegID(0x00003000, 0x0000000C);
        else
            emitStateRegID(0x00000300, 0x00000008);
        break;
    }
    case GenISAIntrinsic::GenISA_eu_id:{
        if (m_currShader->m_Platform->GetPlatformFamily() == IGFX_GEN8_CORE ||
            m_currShader->m_Platform->GetPlatformFamily() == IGFX_GEN9_CORE)
            emitStateRegID(0x00000F00, 0x00000008);
        else
            emitStateRegID(0x000000F0, 0x00000004);
        break;
    }
    case GenISAIntrinsic::GenISA_eu_thread_id:
        emitStateRegID(0x00000007, 0x00000000);
        break;
    case GenISAIntrinsic::GenISA_pair_to_ptr:
        emitPairToPtr(inst);
        break;
    case GenISAIntrinsic::GenISA_StackAlloca:
        emitStackAlloca(inst);
        break;
    case GenISAIntrinsic::GenISA_WaveBallot:
        emitWaveBallot(inst);
        break;
    case GenISAIntrinsic::GenISA_WaveShuffleIndex:
        emitSimdShuffle(inst);
        break;
    case GenISAIntrinsic::GenISA_WavePrefix:
        emitWavePrefix(inst);
        break;
    case GenISAIntrinsic::GenISA_QuadPrefix:
        emitWavePrefix(inst, true);
        break;
    case GenISAIntrinsic::GenISA_WaveAll:
        emitWaveAll(inst);
        break;

    case GenISAIntrinsic::GenISA_OUTPUTPSMASK:
        break;
    case GenISAIntrinsic::GenISA_InitDiscardMask:
        emitInitDiscardMask(inst);
        break;
    case GenISAIntrinsic::GenISA_UpdateDiscardMask:
        emitUpdateDiscardMask(inst);
        break;
    case GenISAIntrinsic::GenISA_GetPixelMask:
        emitGetPixelMask(inst);
        break;
    case GenISAIntrinsic::GenISA_evaluateSampler:
        // nothing to do
        break;
    default:
        // we assume that some of gen-intrinsic should always be pattern-matched away,
        // therefore we do not handle them in visa-emission, cases include:
        // - owordPtr
        // let us know if you see a case that hits this assert by those intrinsics
        inst->print(IGC::Debug::ods());
        assert(0 && "unknown intrinsic");
        break;
    }
}

void EmitPass::EmitIntrinsicMessage(llvm::IntrinsicInst* inst)
{
    switch(inst->getIntrinsicID())
    {
    case Intrinsic::lifetime_start:
    case Intrinsic::lifetime_end:
    case Intrinsic::stacksave:
    case Intrinsic::stackrestore:
    case Intrinsic::fabs:
    case Intrinsic::trap:
        // do nothing
        break;
    case Intrinsic::bswap:
        emitLLVMbswap(inst);
        break;
    default:
        inst->print(IGC::Debug::ods());
        assert(0 && "unknown intrinsic");
        break;
    }
}

CVariable *EmitPass::Mul(CVariable *Src0, CVariable *Src1, const CVariable *DstPrototype)
{
    bool IsSrc0Imm = Src0->IsImmediate();
    bool IsSrc1Imm = Src1->IsImmediate();
    if (IsSrc0Imm && IsSrc1Imm) {
        uint64_t Prod = Src0->GetImmediateValue() * Src1->GetImmediateValue();
        return m_currShader->ImmToVariable(Prod, DstPrototype->GetType());
    }
    if (IsSrc0Imm && !IsSrc1Imm) {
        std::swap(Src0, Src1);
    }
    if (IsSrc1Imm) {
        APInt Imm(APInt(m_DL->getPointerSizeInBits(), Src1->GetImmediateValue()));
        if (Imm == 0) {
            return Src1;
        }
        if (Imm == 1) {
            return Src0;
        }
        if (Imm.isPowerOf2()) {
            unsigned Amt = Imm.logBase2();
            CVariable* VarAmt = m_currShader->ImmToVariable(Amt, ISA_TYPE_UD);
            CVariable *Dst = m_currShader->GetNewVariable(DstPrototype);
            m_encoder->Shl(Dst, Src0, VarAmt);
            m_encoder->Push();
            return Dst;
        }
    }

    CVariable *Dst = m_currShader->GetNewVariable(DstPrototype);
    VISA_Type srcType = Src0->GetType();

    // Only i64 muls need special handling, otherwise go back to standard flow
    if (srcType != ISA_TYPE_Q && srcType != ISA_TYPE_UQ)
    {
        m_encoder->Mul(Dst, Src0, Src1);
        m_encoder->Push();
    } else {
        CVariable *src[] = {Src0, Src1};
        Mul64(Dst, src);
    }
    return Dst;
}

CVariable *EmitPass::Add(CVariable *Src0, CVariable *Src1, const CVariable *DstPrototype)
{
    bool IsSrc0Imm = Src0->IsImmediate();
    bool IsSrc1Imm = Src1->IsImmediate();
    if (IsSrc1Imm && !Src1->GetImmediateValue()) {
        return Src0;
    }
    if (IsSrc0Imm && !Src0->GetImmediateValue()) {
        return Src1;
    }
    if (IsSrc0Imm && IsSrc1Imm) {
        uint64_t Sum = Src0->GetImmediateValue() + Src1->GetImmediateValue();
        return m_currShader->ImmToVariable(Sum, DstPrototype->GetType());
    }
    CVariable *Dst = m_currShader->GetNewVariable(DstPrototype);
    m_encoder->Add(Dst, Src0, Src1);
    m_encoder->Push();
    return Dst;
}

void EmitPass::emitGEP(llvm::Instruction* I)
{
    GetElementPtrInst &GEP = cast<GetElementPtrInst>(*I);
    unsigned AddrSpace = I->getType()->getPointerAddressSpace();
    VISA_Type PtrTy =
        m_currShader->GetContext()->getRegisterPointerSizeInBits(AddrSpace) == 64 ? ISA_TYPE_UQ : ISA_TYPE_UD;

    // First compute the offset from the base to benefit from constant folding,
    // and then add to the base (which is less likely to be a constant).

    // vOffset is the value of the advancing offset in the loop below
    // Use the pre-allocated variable for storage
    CVariable* vOffset = m_destination;
    // vN is the current offset at the begining of each iteration in the loop below
    CVariable *vN = m_currShader->ImmToVariable(0, PtrTy);
    // Note that the pointer operand may be a vector of pointers. Take the scalar
    // element which holds a pointer.
    Type *Ty = GEP.getPointerOperand()->getType()->getScalarType();

    // Prototype temporary used for cloning from
    CVariable* vTmp = m_currShader->GetNewVariable(
        numLanes(m_currShader->m_SIMDSize),
        PtrTy,
        EALIGN_GRF,
        m_destination->IsUniform());

    gep_type_iterator GTI = gep_type_begin(GEP);
    for (auto OI = GEP.op_begin() + 1, E = GEP.op_end(); OI != E; ++OI, ++GTI) {
        Value *Idx = *OI;
        // Offset of element contributed by current index being visited
        CVariable *vElemOffset;
        if (StructType *StTy = GTI.getStructTypeOrNull()) {
            // GEP indices into structs are always constant i32's
            unsigned Field = int_cast<unsigned>(cast<Constant>(Idx)->getUniqueInteger().getZExtValue());
            uint64_t Offset = 0;
            if (Field) {
                Offset = m_DL->getStructLayout(StTy)->getElementOffset(Field);
            }
            vElemOffset = m_currShader->ImmToVariable(Offset, ISA_TYPE_UD);
            Ty = StTy->getElementType(Field);
        } else {
            Ty = GTI.getIndexedType();
            // vElemOffset = vIdx * vElemSize
            CVariable* vElemSize = m_currShader->ImmToVariable(m_DL->getTypeAllocSize(Ty), PtrTy);
            CVariable *vIdx = GetSymbol(Idx);
             // The Mul does a push and takes care of constant folding
            vElemOffset = Mul(vIdx, vElemSize, vTmp);
        }
        // vOffset = vN + vElemOffset
        vOffset = Add(vElemOffset, vN, vTmp); // The Add does a m_encoder->push
        vN = vOffset; // After eating an index operand, advance the current offset
    }

    CVariable* vBasePtr = GetSymbol(GEP.getPointerOperand());
    // GEP = VBasePtrt + VOffset
    vTmp = Add(vBasePtr, vOffset, vTmp);  // The Add does a m_encoder->push
    // Copy the result
    if (CEncoder::GetCISADataTypeSize(vTmp->GetType()) <
        CEncoder::GetCISADataTypeSize(m_destination->GetType()))
    {
        // If both offset and the base are immediates, we may end up with an offset of a smaller
        // type than the destination, due to immediate creation optimizations in the Add.
        m_encoder->Cast(m_destination, vTmp);
    }
    else
    {
        m_encoder->Copy(m_destination, vTmp);
    }
    m_encoder->Push();
}

void EmitPass::emitIntToPtr(llvm::IntToPtrInst* I2P)
{
    CVariable* src = GetSymbol(I2P->getOperand(0));
    CVariable* IntVar = m_currShader->BitCast(src, GetUnsignedType(src->GetType()));
    m_encoder->Cast(m_destination, IntVar);
    m_encoder->Push();
}

void EmitPass::emitBitCast(llvm::BitCastInst* btCst)
{
    Type* srcType = btCst->getOperand(0)->getType();
    Type* dstType = btCst->getType();
    unsigned int numSrcElement = srcType->isVectorTy() ? srcType->getVectorNumElements() : 1;
    unsigned int numDstElement = dstType->isVectorTy() ? dstType->getVectorNumElements() : 1;

    if (srcType->isPointerTy())
    {
        assert(dstType->isPointerTy() && "Expected both src and dst have pointer type.");
    }

    if(btCst->getOperand(0)->getType()->isVectorTy() ||
         btCst->getType()->isVectorTy())
    {
        emitVectorBitCast(btCst);
        return;
    }

    CVariable* src = GetSymbol( btCst->getOperand(0) );
    CVariable* dst = m_destination;
    assert(numSrcElement == 1 && numDstElement == 1 && "vector to vector bitcast not supported");
    src = m_currShader->BitCast(src, dst->GetType());
    m_encoder->Copy(dst, src);
    m_encoder->Push();
}

void EmitPass::emitPtrToInt(llvm::PtrToIntInst* P2I)
{
    CVariable* dst = m_currShader->BitCast(m_destination, GetUnsignedType(m_destination->GetType()));
    CVariable* PtrVar = GetSymbol(P2I->getOperand(0));
    m_encoder->Cast(dst, PtrVar);
    m_encoder->Push();
}

void EmitPass::emitAddrSpaceCast(llvm::AddrSpaceCastInst* addrSpaceCast)
{
    CVariable* srcV = GetSymbol(addrSpaceCast->getOperand(0));
    m_encoder->Cast(m_destination, srcV);
    m_encoder->Push();
}

void EmitPass::emitExtract(llvm::Instruction* inst)
{
    assert(llvm::isa<llvm::ExtractElementInst>(inst));
    llvm::ExtractElementInst *Extract =  llvm::cast<llvm::ExtractElementInst>(inst);
    llvm::Value* vecOperand = Extract->getVectorOperand();
    auto vectorBCI = dyn_cast<BitCastInst>(vecOperand);
    CVariable* vector = m_currShader->GetSymbol(vecOperand, true);

    if (llvm::ConstantInt* pConstElem = llvm::dyn_cast<llvm::ConstantInt>(Extract->getIndexOperand()))
    {
        uint element = m_currShader->AdjustExtractIndex(vecOperand, int_cast<uint16_t>(pConstElem->getZExtValue()));
        // Do not use allocated type to compute the offsets; otherwise the computed
        // offsets may be out-of-bound. The alignment information of the base
        // element type should not impact the offset.
        uint eltBytes = GetScalarTypeSizeInRegister(Extract->getType());
        assert(eltBytes > 0 && "illegal ExtractElement instruction");

        if (m_currShader->CanTreatAsAlias(Extract))
        {
            if (vectorBCI && m_currShader->getCVarForVectorBCI(vectorBCI, element))
            {
                //do nothing as we can reuse the symbol from the vector bitcast
                return;
            }
            uint offset = 0;
            if( m_currShader->GetIsUniform( inst->getOperand(0) ) )
            {
                offset = element * eltBytes;
            }
            else
            {
                offset = vector->getOffsetMultiplier() * element * numLanes(m_currShader->m_SIMDSize) * eltBytes;
            }
            // the symbol table should have coalesced those two values;
            // TODO: clean up when we get generic coalescing
            assert((vector == m_destination->GetAlias() ||
                    vector->GetAlias() == m_destination->GetAlias()) &&
                   m_destination->GetAliasOffset() == offset);
        }
        else
        {
            if (vectorBCI)
            {
                if (auto var = m_currShader->getCVarForVectorBCI(vectorBCI, element))
                { 
                    // use the separate CVar for each index instead
                    m_encoder->Copy(m_destination, var);
                    m_encoder->Push();
                    return;
                }
            }

            if (m_currShader->GetIsUniform(inst->getOperand(0)))
            {
                uint offset = element * eltBytes;
                m_encoder->SetSrcSubVar(0, (offset / SIZE_GRF));
                m_encoder->SetSrcSubReg(0, ((offset % SIZE_GRF) / eltBytes));
            }
            else
            {
                uint offset = vector->getOffsetMultiplier() * element * numLanes(m_currShader->m_SIMDSize) * eltBytes;
                uint subvar = offset / SIZE_GRF;
                m_encoder->SetSrcSubVar(0, subvar);
                m_encoder->SetSrcSubReg(0, ((offset % SIZE_GRF) / eltBytes));
            }
            m_encoder->Copy(m_destination, vector);
            m_encoder->Push();
        }
    }
    else
    {
        // We got an index which is not a value known at compile-time.
        llvm::Value* pIndex = Extract->getIndexOperand();
        llvm::Type* pVecType = vecOperand->getType();

        // When the index type is i32, it is better to create a uw alias since
        // the following address computation will be in uw.
        CVariable *pIndexVar = GetSymbol(pIndex);
        assert(pIndex->getType()->getPrimitiveSizeInBits() <= 32);

        bool DoAliasing = pIndex->getType()->getPrimitiveSizeInBits() == 32;
        if (DoAliasing)
        {
            pIndexVar = m_currShader->BitCast(pIndexVar, ISA_TYPE_UW);
        }

        // size of vector entry
        const uint vectorEntrySimdWidth = vector->IsUniform() ?
            1 : numLanes(m_currShader->m_SIMDSize);

        const uint vecTypeSize = GetScalarTypeSizeInRegister(pVecType);

        const uint offset = vectorEntrySimdWidth * vecTypeSize;

        CVariable* pOffset1 = m_currShader->ImmToVariable(offset, ISA_TYPE_UW);

        // offset2 is the offset within the array expressed in bytes (index*element size in bytes)
        CVariable* pOffset2 = m_currShader->GetNewVariable(
            pIndexVar->IsUniform() ? 1 : numLanes(m_currShader->m_SIMDSize),
            ISA_TYPE_UW,
            pIndexVar->IsUniform() ? EALIGN_WORD : EALIGN_GRF,
            pIndexVar->IsUniform());

        // We bitcast the address as uw so it is an "unpacked" uw
        if (!pIndexVar->IsUniform() && DoAliasing)
        {
            m_encoder->SetSrcRegion(0, 2, 1, 0);
        }

        m_encoder->Mul(pOffset2, pIndexVar, pOffset1);
        m_encoder->Push();

        // if pIndexVar is non-uniform, we will need to use VxH addressing.
        // And if both pIndexVar and pVectorVar are non-uniform, need to add
        // per-element offsets to the content of address register
        CVariable* pOffset3 = nullptr;
        if (!pIndexVar->IsUniform() && !vector->IsUniform())
        {
            pOffset3 = m_currShader->GetNewVariable(
                numLanes(m_currShader->m_SIMDSize),
                ISA_TYPE_UW,
                EALIGN_GRF,
                false);
            CVariable *OffsetVar = getOrCreatePerLaneOffsetVariable(vecTypeSize);
            m_encoder->Add(pOffset3, pOffset2, OffsetVar);
            m_encoder->Push();
        }
        else
        {
            // no need to add per-lane offsets
            pOffset3 = pOffset2;
        }

        // address variable represents register a0
        CVariable* pDstArrElm = m_currShader->GetNewAddressVariable(
            pIndexVar->IsUniform() ? 1 : numLanes(m_currShader->m_SIMDSize),
            m_destination->GetType(),
            pIndexVar->IsUniform(),
            vector->IsUniform());

        // we add offsets to the base that is the beginning of the vector variable
        m_encoder->AddrAdd(pDstArrElm, vector, pOffset3);
        m_encoder->Push();

        // finally, we move the indirectly addressed values to the destination register
        m_encoder->Copy(m_destination, pDstArrElm);
        m_encoder->Push();
    }
}

static bool IsOWordAlignedLoad(LoadInst *inst)
{
    Value *ptrOpnd = inst->getPointerOperand();
    if (ptrOpnd && isa<CallInst>(ptrOpnd) && GetOpCode(cast<CallInst>(ptrOpnd)) == llvm_owordptr)
    {
        assert(inst->getAlignment() == 16);
        return true;
    }
    return false;
}

void EmitPass::emitUAVSerialize()
{
    m_encoder->Wait();
    m_encoder->Push();
}

void EmitPass::emitLoadRawIndexed(GenIntrinsicInst* inst)
{
    Value *buf_ptrv = inst->getOperand(0);
    Value *elem_idxv = inst->getOperand(1);

    ResourceDescriptor resource = GetResourceVariable(buf_ptrv);
    emitLoad3DInner(inst, resource, elem_idxv);
    m_currShader->isMessageTargetDataCacheDataPort = true;
}

void EmitPass::emitLoad3DInner(Instruction* inst, ResourceDescriptor& resource, Value *elem_idxv)
{
    IGC::e_predefSurface predDefSurface = resource.m_surfaceType;
    CVariable* gOffset = m_currShader->ImmToVariable(0x0, ISA_TYPE_UD);

    CVariable* src_offset = GetSymbol( elem_idxv );

    // still collect buffer type here to work around some alignment problem with different messages
    BufferType bufType = GetBufferType(inst->getOperand(0)->getType()->getPointerAddressSpace());

    // generate oword_load if it is uniform
    // otherwise, generate gather/gather4
    if( m_currShader->GetIsUniform( inst ) )
    {
        assert(predDefSurface != ESURFACE_STATELESS);  // scratch cannot be uniform
        Type* loadType = inst->getType();
        uint numElement = loadType->isVectorTy() ? loadType->getVectorNumElements() : 1;
        if (predDefSurface == ESURFACE_SLM)
        {
            assert(numElement <= 4);
            uint numLane = (numElement == 3) ? 4 : numElement;
            // there is no oword-block read for SLM, also we expect loading only up to 4-dwords
            CVariable* imm = m_currShader->ImmToVariable(0x0C840, ISA_TYPE_UV);
            CVariable* srcTmp = m_currShader->GetNewVariable((uint16_t)numLane, ISA_TYPE_UD, EALIGN_GRF, true);
            m_encoder->SetNoMask();
            m_encoder->SetUniformSIMDSize(lanesToSIMDMode(numLane));
            m_encoder->Add(srcTmp, src_offset, imm);
            m_encoder->Push();
            CVariable* dstTmp = m_destination;
            if (numElement != numLane)
            {
                dstTmp = m_currShader->GetNewVariable((uint16_t)numLane, ISA_TYPE_D, EALIGN_GRF, true);
            }
            m_encoder->SetNoMask();
            m_encoder->SetUniformSIMDSize(lanesToSIMDMode(numLane));
            m_encoder->ByteGather(dstTmp, resource, srcTmp, 8, 4);
            m_encoder->Push();

            // generate an extract-element due to dst-size difference when numElement == 3
            // \todo, we should canonicalize <floatx3> to <floatx4> before code-gen to avoid this
            if (dstTmp != m_destination)
            {
                for (uint i = 0; i < numElement; i++)
                {
                    m_encoder->SetSrcSubReg(0, i);
                    m_encoder->SetDstSubReg(i);
                    m_encoder->SetSrcRegion(0, 0, 1, 0);
                    m_encoder->Copy(m_destination, dstTmp);
                    m_encoder->Push();
                }
            }
        }
        else
        {
            bool owordAligned = false;
            if (isa<LoadInst>(inst))
            {
                owordAligned = IsOWordAlignedLoad(cast<LoadInst>(inst));
            }
            // need to clear lower two-bits for unaligned
            CVariable* visaOffset = nullptr;
            if (owordAligned || bufType == CONSTANT_BUFFER)
            {
                visaOffset = src_offset;
            }
            else if (src_offset->IsImmediate())
            {
                // clear lower-two-bits
                visaOffset = m_currShader->ImmToVariable(src_offset->GetImmediateValue() & 0xfffffffc, ISA_TYPE_UD);
            }
            else
            {
                // clear lower-two-bits
                CVariable* masklast2bits = m_currShader->ImmToVariable(0xfffffffc, ISA_TYPE_UD);
                visaOffset = m_currShader->GetNewVariable(src_offset->GetNumberElement(),
                    ISA_TYPE_UD,
                    src_offset->GetAlign(),
                    src_offset->IsUniform());
                m_encoder->And(visaOffset, m_currShader->BitCast(src_offset, ISA_TYPE_UD), masklast2bits);
                m_encoder->Push();
            }
            if (numElement >= 4)
            {
                m_encoder->OWLoad(m_destination, resource, visaOffset, owordAligned, m_destination->GetSize());
                m_encoder->Push();
            }
            else
            {
                assert(inst->getType()->getPrimitiveSizeInBits() < SIZE_DWORD * 8 * 4);
                uint elemSize = m_destination->GetElemSize();
                
                if (elemSize > 0)
                {
                    CVariable* tmp = m_currShader->GetNewVariable(4 * SIZE_DWORD / elemSize, m_destination->GetType(), EALIGN_GRF, m_destination->IsUniform());
                    m_encoder->OWLoad(tmp, resource, visaOffset, owordAligned, tmp->GetSize());
                    m_encoder->Push();

                    // generate an extract-element
                    for (uint i = 0; i < numElement; i++)
                    {
                        m_encoder->SetSrcSubReg(0, i);
                        m_encoder->SetDstSubReg(i);
                        m_encoder->SetSrcRegion(0, 0, 1, 0);
                        m_encoder->Copy(m_destination, tmp);
                        m_encoder->Push();
                    }
                }
            }
        }
    }
    else
    {
        uint label = 0;
        CVariable* flag = nullptr;
        bool needLoop = ResourceLoopHeader(resource, flag, label);
        uint sizeInBits = inst->getType()->getPrimitiveSizeInBits();
        assert((sizeInBits == 8 ||
            sizeInBits == 16 ||
            sizeInBits == 32 ||
            sizeInBits == 64 ||
            sizeInBits == 96 ||
            sizeInBits == 128) && "load type must be 1/2/4/8/12/16 bytes long");
        IGC::CVariable *visaOffset = BroadcastIfUniform(src_offset);
        if (sizeInBits == 32 && resource.m_surfaceType == ESURFACE_STATELESS &&
            m_currShader->m_Platform->getWATable().WaNoA32ByteScatteredStatelessMessages)
        {
            // DWORD gather
            CVariable* shiftedPtr = m_currShader->GetNewVariable(visaOffset);
            m_encoder->Shr(shiftedPtr, visaOffset, m_currShader->ImmToVariable(2, ISA_TYPE_UD));
            m_encoder->Push();
            visaOffset = shiftedPtr;
            m_encoder->Gather(m_destination, resource.m_resource, visaOffset, gOffset, resource.m_surfaceType, 4);
            m_encoder->Push();
        }
        else if (sizeInBits == 32 && (bufType == CONSTANT_BUFFER || resource.m_surfaceType == ESURFACE_STATELESS))
        {
            // uav and resource cannot be changed to this path due to alignment issue encountered in some tests
            uint elementSize = 8;
            uint numElems = 4;
            m_encoder->ByteGather(m_destination, resource, visaOffset, elementSize, numElems);
            m_encoder->Push();
        }
        else if (sizeInBits >= 32)
        {
            // constant-buffer cannot go this way due to driver surface-state setting to RGBA-F32
            if (m_currShader->m_DriverInfo->UsesTypedConstantBuffers())
                assert(bufType != CONSTANT_BUFFER);

            m_encoder->Gather4ScaledNd(m_destination, resource, visaOffset, sizeInBits/32);
            m_encoder->Push();
        }
        else if (sizeInBits == 8 || sizeInBits == 16)
        {
            uint elementSize = 8;
            uint numElems = sizeInBits / 8;
            uint hStride = 32 / sizeInBits;
            uint16_t vStride = numLanes(m_currShader->m_SIMDSize);
            CVariable *gatherDest = m_currShader->GetNewVariable(vStride, ISA_TYPE_UD, EALIGN_GRF);
            m_encoder->ByteGather(gatherDest, resource, visaOffset, elementSize, numElems);
            m_encoder->Push();

            gatherDest = m_currShader->GetNewAlias(gatherDest, m_destination->GetType(), 0, 0);
            m_encoder->SetSrcRegion(0, vStride, vStride / hStride, hStride);
            m_encoder->Cast(m_destination, gatherDest);
            m_encoder->Push();
        }
        ResourceLoop(needLoop, flag, label);
    }
}

/// getMsgBlockSizes() - Return the block size and number of blocks considering
/// the alignment.
static std::pair<unsigned, unsigned>
getMsgBlockSizes(unsigned sizeInBits, unsigned alignInBits) {
    // BYTE
    if (sizeInBits == 8)
        return std::make_pair(8, 1);

    // WORD
    if (sizeInBits == 16) // W or short is supported through <2 x i8>.
        return std::make_pair(8, 2);

    // DWORD/QWORD need alignement checking.
    bool isAligned = (alignInBits == 0) || (sizeInBits <= alignInBits);

    // DWORD
    if (sizeInBits == 32) {
        if (isAligned)
            return std::make_pair(32, 1);
        return std::make_pair(8, 4);
    }

    // QWORD
    if (sizeInBits == 64) {
        if (isAligned)
            return std::make_pair(64, 1);
        return std::make_pair(8, 8);
    }
    assert(false && "Unsupported block size!");
    return std::make_pair(~0U, ~0U);
}

void EmitPass::emitLoad(LoadInst* inst)
{
    emitVectorLoad(inst, inst->getPointerOperand());
}

void EmitPass::EmitNoModifier(llvm::Instruction* inst)
{
    switch(inst->getOpcode())
    {
    case Instruction::Ret:
        emitReturn(cast<ReturnInst>(inst));
        break;
    case Instruction::Call:
        if(GenIntrinsicInst* I = dyn_cast<GenIntrinsicInst>(inst))
        {
            EmitGenIntrinsicMessage(I);
        }
        else if(IntrinsicInst* I = dyn_cast<IntrinsicInst>(inst))
        {
            EmitIntrinsicMessage(I);
        }
        else
        {
            emitCall(cast<CallInst>(inst));
        }
        break;
    case Instruction::Store:
         emitStore(cast<StoreInst>(inst));
         break;
    case Instruction::Load:
        emitLoad(cast<LoadInst>(inst));
        break;
    case Instruction::GetElementPtr:
         emitGEP(cast<GetElementPtrInst>(inst));
         break;
    case Instruction::BitCast:
        emitBitCast(cast<BitCastInst>(inst));
        break;
    case Instruction::PtrToInt:
        emitPtrToInt(cast<PtrToIntInst>(inst));
        break;
    case Instruction::IntToPtr:
        emitIntToPtr(cast<IntToPtrInst>(inst));
        break;
    case Instruction::AddrSpaceCast:
        emitAddrSpaceCast(cast<AddrSpaceCastInst>(inst));
        break;
    case Instruction::InsertElement:
        emitInsert(cast<InsertElementInst>(inst));
        break;
    case Instruction::ExtractElement:
        emitExtract(cast<ExtractElementInst>(inst));
        break;
    case Instruction::Br:
        emitBranch(cast<BranchInst>(inst), nullptr, EPRED_NORMAL);
        break;
    case Instruction::Unreachable:
        break;
    default:
        assert(0 && "need to add code gen support for this instruction");
    }
}

void EmitPass::emitSampleToRTInstruction(llvm::GenIntrinsicInst* inst)
{
    //To enable EUBypass on SIMD32 we need to change order of last two sampler sends,
    //so H1 part will have EOT enabled, and H2 won't. Spec:
    //Slot Group Select is derived from EOT : If EOT is enabled, set to SLOTGRP_LO, otherwise SLOTGRP_HI.
    //Hence change the order of SIMD16 instruction emitted for each slice.
    if (m_currShader->m_dispatchSize == SIMDMode::SIMD32)
    {
        m_encoder->SetSecondHalf(!(m_encoder->IsSecondHalf()));
    }
    uint numOperands = inst->getNumOperands();
    uint numSources = numOperands - 6;
    uint textureArgIdx = numOperands - 6;
    //Get sampler index in the array of operands
    uint samplerArgIdx = numOperands - 5;

    bool noMaskPayload = false;
    GenISAIntrinsic::ID id = inst->getIntrinsicID();
    if(id == GenISAIntrinsic::GenISA_sample ||
       id == GenISAIntrinsic::GenISA_sampleB ||
       id == GenISAIntrinsic::GenISA_sampleC ||
       id == GenISAIntrinsic::GenISA_sampleBC)
    {
        noMaskPayload = true;
    }

    EOPCODE opCode = GetOpCode(inst);

    //Check for valid number of sources from the end of the list
    for(uint i = (numOperands - 6 - 1); i >= 1 ; i--)
    {
        CVariable* validSrc = GetSymbol(inst->getOperand(i));
        if(validSrc->IsImmediate() &&
            validSrc->GetImmediateValue() == 0)
        {
            numSources--;
        }
        else
        {
            break;
        }
    }

    //temp solution to send valid sources but having 0 as their values.
    ValidateNumberofSources(opCode, numSources);

    uint textureIdx = int_cast<uint>(GetImmediateVal(inst->getOperand(textureArgIdx)));
    uint SamplerIdx = int_cast<uint>(GetImmediateVal(inst->getOperand(samplerArgIdx)));

    SIMDMode mode = m_currShader->m_SIMDSize;

    bool FP16Input = false;
    uint8_t adjFactor = 0;


    if (inst->getOperand(0)->getType()->getTypeID() == llvm::Type::HalfTyID)
    {
        FP16Input = true;
        if (mode == SIMDMode::SIMD16)
            adjFactor = 1;
    }

    uint messageLength = numSources * (numLanes(mode) / (SIZE_GRF >> 2)) >> adjFactor;
    //Response Length always 0 for Sampler to RT write
    uint responseLength = 0;

    EU_SAMPLER_SIMD_MODE executionMode = samplerSimdMode(mode);
    EU_GEN6_SAMPLER_MESSAGE_TYPE sampleMessageType = GetSampleMessage(opCode);
    uint bindingTableIndex = m_currShader->m_pBtiLayout->GetTextureIndex(textureIdx);
    m_currShader->SetBindingTableEntryCountAndBitmap(true, bindingTableIndex);

    // offset
    uint offsetSourceIndex = numOperands-4;
    CVariable* offset = ComputeSampleIntOffset(inst, offsetSourceIndex);

    uint headerSize = 1; //Sampler to RT always has a header

    messageLength += headerSize;

    CVariable* payload = m_currShader->GetNewVariable(
        messageLength * (SIZE_GRF >> 2),
        ISA_TYPE_D,
        EALIGN_GRF);

    //Binding table index for render target ; Always RTI is 0
    uint rti = m_currShader->m_pBtiLayout->GetRenderTargetIndex(0);
    m_currShader->SetBindingTableEntryCountAndBitmap(true, rti);

    CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);

    assert(psProgram->m_hasEOT == false);
    if (!(m_encoder->IsSecondHalf()) || m_currShader->m_numberInstance == 1)
    {
        psProgram->m_hasEOT = true;
    }

    if(headerSize)
    {
        m_currShader->SampleHeader(payload, static_cast<unsigned int>(offset->GetImmediateValue()), 0xF, rti);
    }

    for(uint i = 0; i < numSources; i++)
    {
        m_encoder->SetDstSubVar(numLanes(mode) / (SIZE_GRF >> (2 - adjFactor)) * i + headerSize);
        if(noMaskPayload)
        {
            m_encoder->SetNoMask();
        }
        //If the tex coordinates to sampler is fp16 then bitcast payload from D to HF
        if(inst->getOperand(i)->getType()->getTypeID() == llvm::Type::HalfTyID)
        {
            payload = m_currShader->BitCast(payload, ISA_TYPE_HF);
        }
        m_encoder->Copy(payload, GetSymbol(inst->getOperand(i)));
        m_encoder->Push();
    }

    Type* retType = inst->getType();
    bool FP16Return = retType->getTypeID() == llvm::Type::HalfTyID ||
        (retType->isVectorTy() &&
        dyn_cast<VectorType>(retType)->getElementType()->getTypeID() == llvm::Type::HalfTyID);

    //get message descriptor
    uint Desc = Sampler(
        messageLength,
        responseLength,
        headerSize != 0,
        executionMode,
        sampleMessageType,
        SamplerIdx,
        bindingTableIndex,
        true,
        FP16Input,
        FP16Return);

    //Set End of Thread
    uint exDesc = EU_MESSAGE_TARGET_SAMPLER | psProgram->m_hasEOT << 5;
    CVariable* messDesc = m_currShader->ImmToVariable(Desc, ISA_TYPE_UD);
    assert(psProgram->HasDiscard() == false && "discard not supported");

    //send
    m_encoder->SendC(NULL,payload,exDesc,messDesc);
    m_encoder->Push();
}

void EmitPass::emitPairToPtr(GenIntrinsicInst* GII) {
    CVariable *Lo = GetSymbol(GII->getOperand(0));
    CVariable *Hi = GetSymbol(GII->getOperand(1));

    unsigned AS = GII->getType()->getPointerAddressSpace();
    if (m_currShader->GetContext()->getRegisterPointerSizeInBits(AS) == 32) {
        CVariable *Tmp = m_currShader->BitCast(Lo, GetUnsignedType(Lo->GetType()));
        m_encoder->Cast(m_destination, Tmp);
        m_encoder->Push();
        return;
    }

    assert(m_currShader->GetContext()->getRegisterPointerSizeInBits(AS) == 64
           && "Pointer size should be either 32 or 64!");

    CVariable *Dst32 = m_currShader->BitCast(m_destination, ISA_TYPE_UD);
    // Lo
    m_encoder->SetDstRegion(2);
    m_encoder->Copy(Dst32, Lo);
    m_encoder->Push();
    // Hi
    m_encoder->SetDstRegion(2);
    m_encoder->SetDstSubReg(1);
    m_encoder->Copy(Dst32, Hi);
    m_encoder->Push();
}

void EmitPass::emitStackAlloca(GenIntrinsicInst* GII)
{
    CVariable *pSP = m_currShader->GetSP();
    CVariable *pOffset = m_currShader->GetSymbol(GII->getOperand(0));
    m_encoder->Add(m_destination, pSP, pOffset);
    m_encoder->Push();
}

void EmitPass::emitCall(llvm::CallInst* inst)
{
    llvm::Function* F = inst->getCalledFunction();
    assert(F && "indirect call not supported");
    assert(!F->empty() && "unexpanded builtin?");

    if (m_FGA && m_FGA->useStackCall(F))
    {
        emitStackCall(inst);
        return;
    }

    unsigned i = 0;
    for (auto &Arg : F->getArgumentList())
    {
        // Skip unused arguments if any.
        if (Arg.use_empty())
        {
            ++i;
            continue;
        }

        CVariable *Dst = m_currShader->getOrCreateArgumentSymbol(&Arg);
        CVariable *Src = GetSymbol(inst->getArgOperand(i++));

        // When both symbols are the same, then this argument passing has been
        // lifted to use a global vISA variable, just skip the copy.
        if (Dst != Src)
        {
            emitCopyAll(Dst, Src, Arg.getType());
        }
    }
    m_encoder->SubroutineCall(nullptr, F);
    m_encoder->Push();

    // Emit the return value if used.
    if (!inst->use_empty())
    {
        CVariable *Dst = GetSymbol(inst);
        CVariable *Src = m_currShader->getOrCreateReturnSymbol(F);
        emitCopyAll(Dst, Src, inst->getType());
    }
}

void EmitPass::emitReturn(llvm::ReturnInst* inst)
{
    llvm::Function *F = inst->getParent()->getParent();
    MetaDataUtils *pMdUtils = m_currShader->GetMetaDataUtils();

    // return from a function (not a kernel)
    if (!isKernelFunc(pMdUtils, F))
    {
        if (m_FGA && m_FGA->useStackCall(F))
        {
            emitStackFuncExit(inst);
            return;
        }

        llvm::Type *RetTy = F->getReturnType();
        if (!RetTy->isVoidTy())
        {
            CVariable *Dst = m_currShader->getOrCreateReturnSymbol(F);
            CVariable *Src = GetSymbol(inst->getReturnValue());
            emitCopyAll(Dst, Src, RetTy);
        }

        m_encoder->SubroutineRet(nullptr);
        m_encoder->Push();
        return;
    }

    if (m_currShader->GetShaderType() == ShaderType::COMPUTE_SHADER ||
        m_currShader->GetShaderType() == ShaderType::OPENCL_SHADER)
    {
        // GPGPU needs an explicit EOT. For PS and other shader units the EOT has to happen with either
        // the RT write or URB write. The rest of the messages should not encode the EOT and call this
        // function to do the EOT. Since currently, we don't have a vISA message indicating this, hence
        // hard-coding the EOT instructions.
        CVariable* pEOTPayload =  m_currShader->GetNewVariable(
            numLanes(SIMDMode::SIMD8),
            ISA_TYPE_D,
            IGC::EALIGN_GRF);

        m_encoder->SetSimdSize(SIMDMode::SIMD8);
        m_encoder->SetNoMask();
        m_encoder->Copy(pEOTPayload, m_currShader->GetR0());
        m_encoder->Push();

        uint exDesc = 0x00000027;
        CVariable* pMessDesc = m_currShader->ImmToVariable(0x02000010, ISA_TYPE_D);
        m_encoder->SetSimdSize(SIMDMode::SIMD8);
        m_encoder->Send(nullptr, pEOTPayload, exDesc, pMessDesc);
        m_encoder->Push();
    }
    if (m_currShader->GetShaderType() == ShaderType::PIXEL_SHADER)
    {
        CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);
        unsigned nRTWrites = int_cast<unsigned>(psProgram->rtWriteList.size());

        for (unsigned i = 0; i < nRTWrites; i++)
        {
            GenIntrinsicInst* inst;
            bool isSecondHalf;

            inst = cast<GenIntrinsicInst>(psProgram->rtWriteList[i].first);
            isSecondHalf = psProgram->rtWriteList[i].second;
            m_encoder->SetSecondHalf(isSecondHalf);

            switch (inst->getIntrinsicID())
            {
            case GenISAIntrinsic::GenISA_RTWrite:
                emitRenderTargetWrite(cast<RTWritIntrinsic>(inst), true);
                break;
            case GenISAIntrinsic::GenISA_RTDualBlendSource:
                emitDualBlendRT(cast<RTDualBlendSourceIntrinsic>(inst), true);
                break;
            default:
                assert(false && "unknown intrinsic");
                break;
            }
        }
        // restore encoder's second half flag.
        if (psProgram->m_numberInstance == 2)
        {
            m_encoder->SetSecondHalf(false);
        }

        // check to make sure we will have EOT
        assert(psProgram->m_hasEOT || psProgram->GetPhase() != PSPHASE_LEGACY);
    }

    m_currShader->AddEpilogue(inst);
}

/// This function is NOT about the alignment-rule for storing argv into GRF!
/// It is about the alignment-rule when we pack the arguments into a block for stack-call!
uint EmitPass::stackCallArgumentAlignment(CVariable *argv)
{
    if (argv->IsUniform())
    {
        assert(argv->GetType() != ISA_TYPE_BOOL);
        if (argv->GetSize() > SIZE_OWORD)
        {
            return SIZE_GRF;
        }
        else if (argv->GetSize() > SIZE_DWORD)
        {
            return SIZE_OWORD;
        }
        else
        {
            return SIZE_DWORD;
        }
    }
    else
    {
        return SIZE_GRF;
    }
}

void EmitPass::emitStackCall(llvm::CallInst* inst)
{
    llvm::Function* F = inst->getCalledFunction();
    assert(F && "indirect call not supported");
    assert(!F->empty() && "unexpanded builtin?");

    CVariable *ArgBlkVar = m_currShader->GetARGV();
    uint32_t i = 0;
    uint32_t offsetA = 0;  // visa argument offset
    uint32_t offsetS = 0;  // visa stack offset
    SmallVector<std::tuple<CVariable*, uint32_t, uint32_t, uint32_t>, 8> owordWrites;
    for (auto &Arg : F->getArgumentList())
    {
        // Skip unused arguments if any.
        if (Arg.use_empty())
        {
            ++i;
            continue;
        }

        CVariable *ArgCV = m_currShader->getOrCreateArgumentSymbol(&Arg, true);
        CVariable *Src = GetSymbol(inst->getArgOperand(i++));
        if (Src->GetType() == ISA_TYPE_BOOL)
        {
            assert(ArgCV->GetType() == ISA_TYPE_BOOL);
            CVariable *ReplaceArg = m_currShader->GetNewVariable(
                numLanes(m_currShader->m_dispatchSize),
                ISA_TYPE_W,
                EALIGN_GRF, false, 1);
            CVariable* one = m_currShader->ImmToVariable(1, ISA_TYPE_W);
            CVariable* zero = m_currShader->ImmToVariable(0, ISA_TYPE_W);
            m_encoder->Select(Src, ReplaceArg, one, zero);
            Src = ReplaceArg;
        }
        else
        {
            emitCopyAll(ArgCV, Src, Arg.getType());
            Src = ArgCV;
        }
        // adjust offset for alignment
        uint align = stackCallArgumentAlignment(Src);
        offsetA = int_cast<unsigned>(llvm::alignTo(offsetA, align));
        // check if an argument can be written to ARGV based upon offset + arg-size
        bool overflow = ((offsetA + Src->GetSize()) > ArgBlkVar->GetSize());
        if (!overflow)
        {
            CVariable *Dst = ArgBlkVar;
            if (Dst->GetType() != Src->GetType() || 
                offsetA != 0 ||
                Src->IsUniform() != Dst->IsUniform())
            {
                Dst = m_currShader->GetNewAlias(ArgBlkVar, Src->GetType(), (uint16_t)offsetA, Src->GetNumberElement(), Src->IsUniform());
            }
            if (ArgCV->GetType() == ISA_TYPE_BOOL)
            {
                emitVectorCopy(Dst, Src, Src->GetNumberElement());
            }
            else
            {
                emitCopyAll(Dst, Src, Arg.getType());
            }
            offsetA += Src->GetSize();
        }
        else
        {
            // write to stack, stack offset is always oword-aligned
            offsetS = int_cast<unsigned>(llvm::alignTo(offsetS, SIZE_OWORD));
            // emit oword_stores
            int RmnBytes = Src->GetSize();
            uint32_t WrtBytes = 0;
            do
            {
                uint32_t WrtSize = 0;
                if (RmnBytes >= SIZE_OWORD * 8)
                {
                    WrtSize = 8 * SIZE_OWORD;
                }
                else if (RmnBytes >= SIZE_OWORD * 4)
                {
                    WrtSize = 4 * SIZE_OWORD;
                }
                else if (RmnBytes >= SIZE_OWORD * 2)
                {
                    WrtSize = 2 * SIZE_OWORD;
                }
                else
                {
                    WrtSize = 1 * SIZE_OWORD;
                }

                // record list of write
                owordWrites.push_back(std::make_tuple(Src, offsetS, WrtSize, WrtBytes));

                offsetS += WrtSize;
                WrtBytes += WrtSize;
                RmnBytes -= WrtSize;
            } while (RmnBytes > 0);
        }
    }
    if (offsetS > 0)
    {
        // update stack pointer before call
        CVariable *pSP = m_currShader->GetSP();
        CVariable* pPushSize = m_currShader->ImmToVariable(offsetS, ISA_TYPE_UD);
        m_encoder->Add(pSP, pSP, pPushSize);
        m_encoder->Push();

        // emit oword-block-writes
        if (pSP->GetSize() > 4)
        {
            for (auto &I : owordWrites)
            {
                CVariable *Src = std::get<0>(I);
                uint32_t StackOffset = std::get<1>(I);
                uint32_t WrtSize = std::get<2>(I);
                uint32_t SrcOffset = std::get<3>(I);
                CVariable* pTempVar = m_currShader->GetNewVariable(
                    numLanes(SIMDMode::SIMD1),
                    ISA_TYPE_UQ,
                    EALIGN_QWORD, true, 1);
                // emit write address
                CVariable *pStackOffset = m_currShader->ImmToVariable(StackOffset - offsetS, ISA_TYPE_D);
                m_encoder->Add(pTempVar, pSP, pStackOffset);
                m_encoder->Push();
                // emit oword-store, 64-bit address
                m_encoder->OWStoreA64(Src, pTempVar, WrtSize, SrcOffset);
                m_encoder->Push();
            }
        }
        else
        {
            for (auto &I : owordWrites)
            {
                CVariable *Src = std::get<0>(I);
                uint32_t StackOffset = std::get<1>(I);
                uint32_t WrtSize = std::get<2>(I);
                uint32_t SrcOffset = std::get<3>(I);
                CVariable* pTempVar = m_currShader->GetNewVariable(
                    numLanes(SIMDMode::SIMD1),
                    ISA_TYPE_UD,
                    EALIGN_DWORD, true, 1);
                // emit write address
                CVariable *pStackOffset = m_currShader->ImmToVariable(StackOffset - offsetS, ISA_TYPE_D);
                m_encoder->Add(pTempVar, pSP, pStackOffset);
                m_encoder->Push();
                // emit oword-store, 32-bit address
                m_encoder->OWStore(Src, ESURFACE_STATELESS, nullptr, pTempVar, WrtSize, SrcOffset);
                m_encoder->Push();
            }
        }
    }

    uint retSize = 0;
    bool retOnStack = false;
    if (!inst->use_empty())
    {
        CVariable *Dst = GetSymbol(inst);
        if (Dst->GetType() == ISA_TYPE_BOOL)
        {
            retSize = numLanes(m_currShader->m_dispatchSize) * SIZE_WORD;
        }
        else
        {
            retSize = Dst->GetSize();
        }
        CVariable *Src = m_currShader->GetRETV();
        if (retSize > Src->GetSize())
        {
            retOnStack = true;
        }
    }
    unsigned char retSizeInGRF = retOnStack ? 0 : (retSize + SIZE_GRF - 1) / SIZE_GRF;
    m_encoder->StackCall(nullptr, F, (offsetA + SIZE_GRF - 1)/SIZE_GRF, retSizeInGRF);
    m_encoder->Push();

    // Emit the return value if used.
    if (!inst->use_empty())
    {
        CVariable *Dst = GetSymbol(inst);
        if (!retOnStack)
        {
            CVariable *Src = m_currShader->GetRETV();
            if (Dst->GetType() == ISA_TYPE_BOOL)
            {
                CVariable *SrcAlias = m_currShader->GetNewAlias(Src, ISA_TYPE_W, 0, numLanes(m_currShader->m_dispatchSize), false);
                m_encoder->Cmp(EPREDICATE_NE, Dst, SrcAlias, m_currShader->ImmToVariable(0, ISA_TYPE_W));
            }
            else
            {
                assert(Dst->GetSize() <= Src->GetSize());
                if (Dst->GetType() != Src->GetType() || Src->IsUniform() != Dst->IsUniform())
                {
                    Src = m_currShader->GetNewAlias(Src, Dst->GetType(), 0, Dst->GetNumberElement(), Dst->IsUniform());
                }
                emitCopyAll(Dst, Src, inst->getType());
            }
        }
        else
        {
            // read return value from stack, from (SP+n)
            int RmnBytes = Dst->GetSize();
            assert(Dst->GetType() != ISA_TYPE_BOOL);
            uint32_t RdBytes = 0;
            do
            {
                uint RdSize = 0;
                if (RmnBytes >= SIZE_OWORD * 8)
                {
                    RdSize = 8 * SIZE_OWORD;
                }
                else if (RmnBytes >= SIZE_OWORD * 4)
                {
                    RdSize = 4 * SIZE_OWORD;
                }
                else if (RmnBytes >= SIZE_OWORD * 2)
                {
                    RdSize = 2 * SIZE_OWORD;
                }
                else if (RmnBytes >= SIZE_OWORD)
                {
                    RdSize = SIZE_OWORD;
                }
                else
                {
                    RdSize = RmnBytes;
                }

                CVariable *pSP = m_currShader->GetSP();
                // emit oword-block-read
                if (pSP->GetSize() > 4)
                {
                    CVariable* pTempVar = m_currShader->GetNewVariable(
                        numLanes(SIMDMode::SIMD1),
                        ISA_TYPE_UQ,
                        EALIGN_QWORD, true, 1);
                    // emit write address
                    CVariable *pStackOffset = m_currShader->ImmToVariable(RdBytes, ISA_TYPE_UD);
                    m_encoder->Add(pTempVar, pSP, pStackOffset);
                    m_encoder->Push();
                    if (RdSize >= SIZE_OWORD)
                    {
                        m_encoder->OWLoadA64(Dst, pTempVar, RdSize, RdBytes);
                        m_encoder->Push();
                    }
                    else
                    {
                        uint elemSize = Dst->GetElemSize();
                        if (elemSize > 0)
                        {
                            // less than one oword, read one oword, then copy
                            CVariable* pTempDst = m_currShader->GetNewVariable(
                                SIZE_OWORD / elemSize,
                                Dst->GetType(),
                                EALIGN_GRF, true, 1);
                            m_encoder->OWLoadA64(pTempDst, pTempVar, SIZE_OWORD);
                            m_encoder->Push();
                            emitVectorCopy(Dst, pTempDst, RdSize / elemSize, RdBytes, 0);
                        }
                    }
                }
                else
                {
                    CVariable* pTempVar = m_currShader->GetNewVariable(
                        numLanes(SIMDMode::SIMD1),
                        ISA_TYPE_UD,
                        EALIGN_DWORD, true, 1);
                    ResourceDescriptor resource;
                    resource.m_surfaceType = ESURFACE_STATELESS;
                    // emit load address
                    CVariable *pStackOffset = m_currShader->ImmToVariable(RdBytes, ISA_TYPE_UD);
                    m_encoder->Add(pTempVar, pSP, pStackOffset);
                    m_encoder->Push();
                    // emit oword-load, 32-bit address
                    if (RdSize >= SIZE_OWORD)
                    {
                        m_encoder->OWLoad(Dst, resource, pTempVar, false, RdSize, RdBytes);
                        m_encoder->Push();
                    }
                    else
                    {
                        uint dstElemSize = Dst->GetElemSize();

                        if (dstElemSize > 0)
                        {
                            // less than one oword, read one oword, then copy
                            CVariable* pTempDst = m_currShader->GetNewVariable(
                                SIZE_OWORD / dstElemSize,
                                Dst->GetType(),
                                EALIGN_GRF, true, 1);
                            m_encoder->OWLoad(pTempDst, resource, pTempVar, false, RdSize);
                            m_encoder->Push();
                            emitVectorCopy(Dst, pTempDst, RdSize / dstElemSize, RdBytes, 0);
                        }

                    }
                }
                RdBytes += RdSize;
                RmnBytes -= RdSize;
            } while (RmnBytes > 0);
            // end of reading return value from stack
        }
    }
    //  update stack pointer after the call
    
    CVariable *pSP = m_currShader->GetSP();
    CVariable* pPopSize = m_currShader->ImmToVariable((uint64_t)(~offsetS + 1), ISA_TYPE_D);
    m_encoder->Add(pSP, pSP, pPopSize);
    m_encoder->Push();
}

void EmitPass::emitStackFuncEntry(Function *F, bool ptr64bits)
{
    m_currShader->CreateSP(ptr64bits);

    CVariable *ArgBlkVar = m_currShader->GetARGV();
    uint32_t i = 0;
    uint32_t offsetA = 0;  // visa argument offset
    uint32_t offsetS = 0;  // visa stack offset
    SmallVector<std::tuple<CVariable*, uint32_t, uint32_t, uint32_t>, 8> owordReads;
    for (auto &Arg : F->getArgumentList())
    {
        // Skip unused arguments if any.
        if (Arg.use_empty())
        {
            ++i;
            continue;
        }

        CVariable *Dst = m_currShader->getOrCreateArgumentSymbol(&Arg, true);
        // adjust offset for alignment
        uint align = stackCallArgumentAlignment(Dst);
        offsetA = int_cast<unsigned>(llvm::alignTo(offsetA, align));
        uint argSize = Dst->GetSize();
        if (Dst->GetType() == ISA_TYPE_BOOL)
        {
            argSize = numLanes(m_currShader->m_dispatchSize) * SIZE_WORD;
        }
        // check if an argument can be written to ARGV based upon offset + arg-size
        bool overflow = ((offsetA +argSize) > ArgBlkVar->GetSize());
        if (!overflow)
        {
            CVariable *Src = ArgBlkVar;
            if (Dst->GetType() == ISA_TYPE_BOOL)
            {
                Src = m_currShader->GetNewAlias(ArgBlkVar, ISA_TYPE_W, (uint16_t)offsetA, numLanes(m_currShader->m_dispatchSize), false);
                m_encoder->Cmp(EPREDICATE_NE, Dst, Src, m_currShader->ImmToVariable(0, ISA_TYPE_W));
                offsetA += argSize;
            }
            else
            {
                if (Src->GetType() != Dst->GetType() || 
                    offsetA != 0 ||
                    Src->IsUniform() != Dst->IsUniform())
                {
                    Src = m_currShader->GetNewAlias(ArgBlkVar, Dst->GetType(), (uint16_t)offsetA, Dst->GetNumberElement(), Dst->IsUniform());
                }
                emitCopyAll(Dst, Src, Arg.getType());
                offsetA += Dst->GetSize();
            }
        }
        else
        {
            // read from stack, stack offset is always oword-aligned
            offsetS = int_cast<unsigned>(llvm::alignTo(offsetS, SIZE_OWORD));
            // emit oword_loads
            int RmnBytes = Dst->GetSize();
            if (Dst->GetType() == ISA_TYPE_BOOL)
            {
                RmnBytes = numLanes(m_currShader->m_dispatchSize) * SIZE_WORD;
            }
            uint32_t RdBytes = 0;
            do
            {
                uint RdSize = 0;
                if (RmnBytes >= SIZE_OWORD * 8)
                {
                    RdSize = 8 * SIZE_OWORD;
                }
                else if (RmnBytes >= SIZE_OWORD * 4)
                {
                    RdSize = 4 * SIZE_OWORD;
                }
                else if (RmnBytes >= SIZE_OWORD * 2)
                {
                    RdSize = 2 * SIZE_OWORD;
                }
                else if (RmnBytes >= SIZE_OWORD)
                {
                    RdSize = SIZE_OWORD;
                }
                else
                {
                    RdSize = RmnBytes;
                }
                owordReads.push_back(std::make_tuple(Dst, offsetS, RdSize, RdBytes));
                offsetS += RdSize;
                RdBytes += RdSize;
                RmnBytes -= RdSize;
            } while (RmnBytes > 0);
        }
    }
    m_encoder->SetStackFunctionArgSize((offsetA + SIZE_GRF - 1)/SIZE_GRF);
    if (offsetS > 0)
    {
        CVariable *pSP = m_currShader->GetSP();
        // emit oword-block-read
        if (pSP->GetSize() > 4)
        {
            for (auto &I : owordReads)
            {
                CVariable *Dst = std::get<0>(I);
                uint32_t StackOffset = std::get<1>(I);
                uint32_t RdSize = std::get<2>(I);
                uint32_t DstOffset = std::get<3>(I);
                CVariable* pTempVar = m_currShader->GetNewVariable(
                    numLanes(SIMDMode::SIMD1),
                    ISA_TYPE_UQ,
                    EALIGN_QWORD, true, 1);
                // emit write address
                CVariable *pStackOffset = m_currShader->ImmToVariable(StackOffset - offsetS, ISA_TYPE_D);
                m_encoder->Add(pTempVar, pSP, pStackOffset);
                m_encoder->Push();
                CVariable *LdDst = Dst;
                if (Dst->GetType() == ISA_TYPE_BOOL)
                {
                    LdDst = m_currShader->GetNewVariable(
                        numLanes(m_currShader->m_dispatchSize),
                        ISA_TYPE_W,
                        EALIGN_GRF, false, 1);
                }
                if (RdSize >= SIZE_OWORD)
                {
                    m_encoder->OWLoadA64(LdDst, pTempVar, RdSize, DstOffset);
                    m_encoder->Push();
                }
                else
                {
                    uint ldDstElemSize = LdDst->GetElemSize();

                    if (ldDstElemSize > 0)
                    {
                        // less than one oword, read one oword, then copy
                        CVariable* pTempDst = m_currShader->GetNewVariable(
                            SIZE_OWORD / ldDstElemSize,
                            LdDst->GetType(),
                            EALIGN_GRF, true, 1);
                        m_encoder->OWLoadA64(pTempDst, pTempVar, SIZE_OWORD);
                        m_encoder->Push();
                        emitVectorCopy(LdDst, pTempDst, RdSize / ldDstElemSize, DstOffset, 0);
                    }
                }
                if (LdDst != Dst)
                {
                    // only happens to bool
                    m_encoder->Cmp(EPREDICATE_NE, Dst, LdDst, m_currShader->ImmToVariable(0, LdDst->GetType()));
                }
            }
        }
        else
        {
            for (auto &I : owordReads)
            {
                CVariable *Dst = std::get<0>(I);
                uint32_t StackOffset = std::get<1>(I);
                uint32_t RdSize = std::get<2>(I);
                uint32_t DstOffset = std::get<3>(I);
                CVariable* pTempVar = m_currShader->GetNewVariable(
                    numLanes(SIMDMode::SIMD1),
                    ISA_TYPE_UD,
                    EALIGN_DWORD, true, 1);
                ResourceDescriptor resource;
                resource.m_surfaceType = ESURFACE_STATELESS;
                // emit load address
                CVariable *pStackOffset = m_currShader->ImmToVariable(StackOffset - offsetS, ISA_TYPE_D);
                m_encoder->Add(pTempVar, pSP, pStackOffset);
                m_encoder->Push();
                // emit oword-load, 32-bit address
                CVariable *LdDst = Dst;
                if (Dst->GetType() == ISA_TYPE_BOOL)
                {
                    LdDst = m_currShader->GetNewVariable(
                        numLanes(m_currShader->m_dispatchSize),
                        ISA_TYPE_W,
                        EALIGN_GRF, false, 1);
                }
                if (RdSize >= SIZE_OWORD)
                {
                    m_encoder->OWLoad(LdDst, resource, pTempVar, false, RdSize, DstOffset);
                    m_encoder->Push();
                }
                else
                {
                    uint ldDstElemSize = LdDst->GetElemSize();

                    if (ldDstElemSize > 0)
                    {
                        // less than one oword, read one oword, then copy
                        CVariable* pTempDst = m_currShader->GetNewVariable(
                            SIZE_OWORD / ldDstElemSize,
                            LdDst->GetType(),
                            EALIGN_GRF, true, 1);
                        m_encoder->OWLoad(pTempDst, resource, pTempVar, false, RdSize);
                        m_encoder->Push();
                        emitVectorCopy(Dst, pTempDst, RdSize / ldDstElemSize, DstOffset, 0);
                    }
                }
                if (LdDst != Dst)
                {
                    // only happens to bool
                    m_encoder->Cmp(EPREDICATE_NE, Dst, LdDst, m_currShader->ImmToVariable(0, LdDst->GetType()));
                }
            }
        }
    }
    // save SP before allocation
    m_currShader->SaveSP();

    FunctionInfoMetaDataHandle funcInfoMD = m_currShader->GetMetaDataUtils()->getFunctionsInfoItem(F);
    // reserve space for all the alloca in the function subgroup
    if (funcInfoMD->isPrivateMemoryPerWIHasValue())
    {
        if (funcInfoMD->getPrivateMemoryPerWI())
        {
            CVariable *pSP = m_currShader->GetSP();
            unsigned totalAllocaSize = funcInfoMD->getPrivateMemoryPerWI() * numLanes(m_currShader->m_dispatchSize);
            m_encoder->Add(pSP, pSP, m_currShader->ImmToVariable(totalAllocaSize, ISA_TYPE_UD));
            m_encoder->Push();
        }
    }
}

void EmitPass::emitStackFuncExit(llvm::ReturnInst *inst)
{
    // restore SP
    m_currShader->RestoreSP();

    llvm::Function *F = inst->getParent()->getParent();
    llvm::Type *RetTy = F->getReturnType();
    if (!RetTy->isVoidTy())
    {
        CVariable *Dst = m_currShader->GetRETV();
        CVariable *Src = GetSymbol(inst->getReturnValue());
        if (Dst->GetSize() >= Src->GetSize())
        {
            if (Src->GetType() == ISA_TYPE_BOOL)
            {
                CVariable* one = m_currShader->ImmToVariable(1, ISA_TYPE_W);
                CVariable* zero = m_currShader->ImmToVariable(0, ISA_TYPE_W);
                CVariable* DstAlias = m_currShader->GetNewAlias(Dst, ISA_TYPE_W, 0, numLanes(m_currShader->m_dispatchSize), false);
                m_encoder->Select(Src, DstAlias, one, zero);
                uint RetSize = numLanes(m_currShader->m_dispatchSize)*SIZE_WORD;
                m_encoder->SetStackFunctionRetSize((RetSize + SIZE_GRF - 1) / SIZE_GRF);
            }
            else
            {
                if (Dst->GetType() != Src->GetType() || Src->IsUniform() != Dst->IsUniform())
                {
                    Dst = m_currShader->GetNewAlias(Dst, Src->GetType(), 0, Src->GetNumberElement(), Src->IsUniform());
                }
                emitCopyAll(Dst, Src, RetTy);
                m_encoder->SetStackFunctionRetSize((Src->GetSize() + SIZE_GRF - 1) / SIZE_GRF);
            }
        }
        else
        {
            // write return value onto stack at (SP+n)
            // emit oword_stores
            int RmnBytes = Src->GetSize();
            uint32_t WrtBytes = 0;
            do
            {
                uint32_t WrtSize = 0;
                if (RmnBytes >= SIZE_OWORD * 8)
                {
                    WrtSize = 8 * SIZE_OWORD;
                }
                else if (RmnBytes >= SIZE_OWORD * 4)
                {
                    WrtSize = 4 * SIZE_OWORD;
                }
                else if (RmnBytes >= SIZE_OWORD * 2)
                {
                    WrtSize = 2 * SIZE_OWORD;
                }
                else
                {
                    WrtSize = 1 * SIZE_OWORD;
                }

                CVariable *pSP = m_currShader->GetSP();
                // emit oword-block-writes
                if (pSP->GetSize() > 4)
                {
                    CVariable* pTempVar = m_currShader->GetNewVariable(
                        numLanes(SIMDMode::SIMD1),
                        ISA_TYPE_UQ,
                        EALIGN_QWORD, true, 1);
                    // emit write address
                    CVariable *pStackOffset = m_currShader->ImmToVariable(WrtBytes, ISA_TYPE_UD);
                    m_encoder->Add(pTempVar, pSP, pStackOffset);
                    m_encoder->Push();
                    // emit oword-store, 64-bit address
                    m_encoder->OWStoreA64(Src, pTempVar, WrtSize, WrtBytes);
                    m_encoder->Push();
                }
                else
                {
                    CVariable* pTempVar = m_currShader->GetNewVariable(
                        numLanes(SIMDMode::SIMD1),
                        ISA_TYPE_UD,
                        EALIGN_DWORD, true, 1);
                    // emit write address
                    CVariable *pStackOffset = m_currShader->ImmToVariable(WrtBytes, ISA_TYPE_UD);
                    m_encoder->Add(pTempVar, pSP, pStackOffset);
                    m_encoder->Push();
                    // emit oword-store, 32-bit address
                    m_encoder->OWStore(Src, ESURFACE_STATELESS, nullptr, pTempVar, WrtSize, WrtBytes);
                    m_encoder->Push();
                }
                WrtBytes += WrtSize;
                RmnBytes -= WrtSize;
            } while (RmnBytes > 0);
            // end of writing return-value to stack
            m_encoder->SetStackFunctionRetSize(0);
        }
    }
    else
    {
        m_encoder->SetStackFunctionRetSize(0);
    }
    // emit return
    m_encoder->StackRet(nullptr);
    m_encoder->Push();
}

void EmitPass::emitStoreRawIndexed(GenIntrinsicInst *inst)
{
    Value* pBufPtr = inst->getOperand(0);
    Value* pElmIdx = inst->getOperand(1);
    Value* pValToStore = inst->getOperand(2);

    emitStore3DInner(pValToStore, pBufPtr, pElmIdx);

    m_currShader->isMessageTargetDataCacheDataPort = true;
}

void EmitPass::emitStore3D(StoreInst* inst, Value *elmIdxV)
{
    Value *ptrVal = inst->getPointerOperand();
    Value *pllElmIdx = nullptr;
    if (elmIdxV)
    {
        pllElmIdx = elmIdxV;
    }
    else if (isa<ConstantPointerNull>(ptrVal))
    {
        pllElmIdx = ConstantInt::get(Type::getInt32Ty(inst->getContext()), 0);
    }
    else
    {
        pllElmIdx = inst->getPointerOperand();
    }

    // Only support for scratch space added currently during emitStore
    Value* pllValToStore = inst->getValueOperand();
    Value* pllDstPtr = inst->getPointerOperand();

    emitStore3DInner(pllValToStore, pllDstPtr, pllElmIdx);
}

void EmitPass::emitStore3DInner(Value *pllValToStore, Value *pllDstPtr, Value *pllElmIdx)
{
    assert(pllDstPtr != nullptr);

    bool isPrivateMem = pllDstPtr->getType()->getPointerAddressSpace() == ADDRESS_SPACE_PRIVATE;
    if (!isPrivateMem)
    {
        ForceDMask(false);
    }

    ResourceDescriptor resource = GetResourceVariable(pllDstPtr);

    uint sizeInBits = pllValToStore->getType()->getPrimitiveSizeInBits();

    assert((sizeInBits == 8 ||
        sizeInBits == 16 ||
        sizeInBits == 32 ||
        sizeInBits == 64 ||
        sizeInBits == 96 ||
        sizeInBits == 128) && "Stored type must be 1/2/4/8/12/16 bytes long");

    CVariable* storedVal = GetSymbol(pllValToStore);

    assert(pllElmIdx);
    CVariable* ptr = GetSymbol(pllElmIdx);

    assert(pllDstPtr->getType()->isPointerTy());
    if (!IGC::isA64Ptr(cast<PointerType>(pllDstPtr->getType()), m_currShader->GetContext()))
    {
        ptr = TruncatePointer(ptr);
    }

    CVariable* gOffset = m_currShader->ImmToVariable(0x0, ISA_TYPE_UD);

    // The stored value and the ptr must be placed aligned in GRFs, as SIMDSize DWORDs.
    // So if it's not already in this form, bring it to it:
    // Broadcast the value, and extend it (doesn't matter if it's sext, zext, or any
    // other kind of extend).
    storedVal = BroadcastIfUniform(storedVal);
    ptr = BroadcastIfUniform(ptr);

    uint label = 0;
    CVariable* flag = nullptr;
    bool needLoop = ResourceLoopHeader(resource, flag, label);
    if (sizeInBits == 32)
    {
        if (resource.m_surfaceType == ESURFACE_STATELESS &&
            m_currShader->m_Platform->getWATable().WaNoA32ByteScatteredStatelessMessages)
        {
            // DWORD scatter
            CVariable* shiftedPtr = m_currShader->GetNewVariable(ptr);
            m_encoder->Shr(shiftedPtr, ptr, m_currShader->ImmToVariable(2, ISA_TYPE_UD));
            m_encoder->Push();
            ptr = shiftedPtr;
            setPredicateForDiscard();
            m_encoder->Scatter(
                storedVal,
                resource.m_resource,
                ptr,
                gOffset,
                resource.m_surfaceType,
                4);
            m_encoder->Push();
        }
        else
        {
            // using byte scatter
            uint elementSize = 8;
            uint numElems = 4;
            setPredicateForDiscard();
            m_encoder->ByteScatter(
                storedVal,
                resource,
                ptr,
                elementSize,
                numElems);
            m_encoder->Push();
        }
    }
    else if (sizeInBits == 16 || sizeInBits == 8)
    {
        // using byte scatter
        uint elementSize = 8;
        uint numElems = sizeInBits / 8;
        VISA_Type elementType = (sizeInBits == 8) ? ISA_TYPE_UB : ISA_TYPE_UW;
        CVariable *val = m_currShader->GetNewVariable(numLanes(m_currShader->m_SIMDSize), ISA_TYPE_UD, EALIGN_GRF);
        storedVal = m_currShader->GetNewAlias(storedVal, elementType, 0, 0);
        m_encoder->Cast(val, storedVal);
        setPredicateForDiscard();
        m_encoder->ByteScatter(
            val,
            resource,
            ptr,
            elementSize,
            numElems);
        m_encoder->Push();
    }
    else  // (sizeInBits > 32)
    {
        setPredicateForDiscard();
        m_encoder->Scatter4Scaled(storedVal, resource, ptr);
        m_encoder->Push();
    }
    ResourceLoop(needLoop, flag, label);
    if (!isPrivateMem)
    {
        ResetVMask(false);
    }
}

void EmitPass::emitStore(StoreInst* inst)
{
    if(m_currShader->GetShaderType() == ShaderType::OPENCL_SHADER ||
        inst->getPointerAddressSpace() == ADDRESS_SPACE_GLOBAL)
    {
        emitVectorStoreOCL(inst);
        }
    else
    {
        emitStore3D(inst);
    }
}

CVariable* EmitPass::GetSymbol(llvm::Value* v)
{
    return m_currShader->GetSymbol(v);
}

void EmitPass::emitInsert(llvm::Instruction* inst)
{
    auto IEI = llvm::cast<llvm::InsertElementInst>(inst);
    // Skip emit scalar copy if this `insertelement` could be aliased.
    if (m_currShader->CanTreatScalarSourceAsAlias(IEI))
        return;

    llvm::Type *eTy = inst->getOperand(1)->getType();
    // Do not use allocated type to compute the offsets; otherwise the computed
    // offsets may be out-of-bound. The alignment information of the base
    // element type should not impact the offset.
    uint32_t eBytes = GetScalarTypeSizeInRegister(eTy);
    assert(eBytes > 0 && "illegal InsertElementInst instruction");

    llvm::Value* pVec = inst->getOperand(0);
    CVariable* pInstVar = GetSymbol(inst);
    CVariable* pVecVar = nullptr;
    llvm::Type* pVecType = inst->getType();
    if (!isa<UndefValue>(pVec))
    {
        pVecVar = GetSymbol(pVec);
        if (pVecVar != pInstVar)
        {
            emitVectorCopy(pInstVar, pVecVar, int_cast<unsigned>(dyn_cast<VectorType>(pVecType)->getNumElements()));
        }
    }

    if (llvm::ConstantInt* pConstElem = llvm::dyn_cast<llvm::ConstantInt>(IEI->getOperand(2)))
    {
        CVariable* pElm = GetSymbol(inst->getOperand(1));

        uint element = int_cast<uint>(pConstElem->getZExtValue());
        uint eStartBytes;
        if( m_currShader->GetIsUniform( inst ) && m_currShader->GetIsUniform( pVec ) )
        {
            eStartBytes = eBytes * element;
        }
        else
        {
            eStartBytes = numLanes(m_currShader->m_SIMDSize) * eBytes * element;
        }

        uint subVar = (eStartBytes / SIZE_GRF);
        uint subReg = (eStartBytes % SIZE_GRF) / eBytes; // unit of element(eTy)
        m_encoder->SetDstSubVar(subVar);
        m_encoder->SetDstSubReg(subReg);
        m_encoder->Copy(m_destination, pElm);
        m_encoder->Push();
    }
    else
    {
        // the index is not a compile-time constant, we need to use runtime indirect addressing
        llvm::Value* pElement = inst->getOperand(1);       // element to insert
        llvm::Value* pIndex = inst->getOperand(2);         // index to insert at
        CVariable* pIndexVar = m_currShader->BitCast(GetSymbol(pIndex), ISA_TYPE_UW);
        CVariable* pElemVar  = GetSymbol(pElement);

        // size of vector entry
        const uint vectorEntrySimdWidth = pInstVar->IsUniform() ?
            1 : numLanes(m_currShader->m_SIMDSize);

        const uint vecTypeSize = pVecType->getVectorElementType()->getPrimitiveSizeInBits() / 8;

        const uint offset = vectorEntrySimdWidth * vecTypeSize;

        CVariable* pOffset1 = m_currShader->ImmToVariable(offset, ISA_TYPE_UW);

        // offset2 = index * sizeof(vector entry)  <-- offset within the vector counted in bytes
        CVariable* pOffset2 = m_currShader->GetNewVariable(
            pIndexVar->IsUniform() ? 1 : numLanes(m_currShader->m_SIMDSize),
            ISA_TYPE_UW,
            EALIGN_WORD,
            pIndexVar->IsUniform());

        if(!pIndexVar->IsUniform())
        {
            m_encoder->SetSrcRegion(0, 16, 8, 2);
        }
        m_encoder->Mul(pOffset2, pIndexVar, pOffset1);
        m_encoder->Push();

        // a0 = addressof(vector variable) + offset2 <-- address of element to insert at
        CVariable* pDstArrElm = m_currShader->GetNewAddressVariable(
            pIndexVar->IsUniform() ? 1 : numLanes(m_currShader->m_SIMDSize),
            m_destination->GetType(),
            pIndexVar->IsUniform(),
            pInstVar->IsUniform());

        m_encoder->AddrAdd(pDstArrElm, m_destination, pOffset2);
        m_encoder->Push();

        // If pIndexVar is uniform, we are using 1x1 indirect addressing and
        // a single copy is what we need.
        if (pIndexVar->IsUniform())
        {
            m_encoder->Copy(pDstArrElm, pElemVar);
            m_encoder->Push();
        }
        else
        {
            // Handle the case when the index is non-uniform - we need to lookup a different value
            // for each simd lane.
            // Since HW doesn't support writing to more than two consecutive GRFs, we need to simulate
            // scattered write by a sequence of instructions, each one writing to a single simd-lane.
            for(uint lane = 0; lane < numLanes(m_currShader->m_SIMDSize); ++lane)
            {
                CVariable* immMask = m_currShader->ImmToVariable(1ULL << lane, ISA_TYPE_UD);
                CVariable* dstPred = m_currShader->GetNewVariable(
                    numLanes(m_SimdMode),
                    ISA_TYPE_BOOL,
                    EALIGN_BYTE);

                m_encoder->SetP(dstPred, immMask);
                m_encoder->Push();

                m_encoder->SetPredicate(dstPred);
                if (!pElemVar->IsUniform())
                {
                    m_encoder->SetSrcSubReg(0, lane);
                }
                m_encoder->SetSrcRegion(0, 0, 1, 0);
                m_encoder->SetDstSubReg(lane);
                m_encoder->Copy(pDstArrElm, pElemVar);
                m_encoder->Push();
            }
        }
    }
}

void EmitPass::emitBranch(
    llvm::BranchInst* branch,
    const SSource* cond,
    e_predMode predMode)
{
    llvm::BasicBlock* next = m_blockCoalescing->SkipEmptyBasicBlock(branch->getParent()->getNextNode());
    if(branch->isConditional())
    {
        CVariable* flag;
        if (cond)
        {
            flag = GetSrcVariable(*cond);
        }
        else
        {
            flag = m_currShader->GetSymbol(branch->getCondition());
        }
        // if it is not a fallthrough
        BasicBlock* succ0 = m_blockCoalescing->FollowEmptyBlock(branch->getSuccessor(0));
        BasicBlock* succ1 = m_blockCoalescing->FollowEmptyBlock(branch->getSuccessor(1));
        uint label0 = m_pattern->GetBlockId(succ0);
        uint label1 = m_pattern->GetBlockId(succ1);

        m_encoder->SetPredicateMode(predMode);

        if(next == NULL || (next != succ0 && next != succ1))
        {
            // Both succ0 and succ1 are not next. Thus, need one conditional jump and
            // one unconditional jump. There are three cases for selecting the target
            // of the conditional jump:
            //    1. both are backward, select one with the larger ID (closer to branch) as target
            //           L0:
            //              ....
            //           L1:
            //              ...
            //           [+-flag] goto L1
            //           goto L0
            //
            //    2. both are forward,  select one with the larger ID (farther to branch) as target
            //           [+- flag] goto L1
            //            goto L0
            //            ...
            //           L0:
            //              ......
            //           L1:
            //       (making sense in this way ?)
            //    3. one is backward and one is forward, select the backward one as target.
            //
            uint label  = m_pattern->GetBlockId(branch->getParent());
            uint condTarget, uncondTarget;
            if( (label0 <= label && label1 <= label) || (label0 > label && label1 > label) )
            {
                // case 1 & 2
                condTarget = (label0 < label1) ? label1 : label0;
                uncondTarget = (label0 < label1) ? label0 : label1;
            }
            else
            {
                // case 3
                condTarget = (label0 <= label) ? label0 : label1;
                uncondTarget = (label0 <= label) ? label1 : label0;
            }

            if(condTarget == uncondTarget)
            {   // sanity check. label0 == label1 (we don't expect it, but it's legal)
                m_encoder->Jump(condTarget);
                m_encoder->Push();
            }
            else
            {
                if(condTarget != label0)
                {
                    m_encoder->SetInversePredicate(true);
                }
                m_encoder->Jump(flag, condTarget);
                m_encoder->Push();

                m_encoder->Jump(uncondTarget);
                m_encoder->Push();
            }
        }
        else if(next != succ0)
        {
            assert(next == succ1 && "next should be succ1!");

            m_encoder->Jump(flag, label0);
            m_encoder->Push();
        }
        else
        {
            assert(next == succ0 && "next should be succ0");

            m_encoder->SetInversePredicate(true);
            m_encoder->Jump(flag, label1);
            m_encoder->Push();
        }
    }
    else
    {
        BasicBlock* succ = m_blockCoalescing->FollowEmptyBlock(branch->getSuccessor(0));
        if((next == NULL) || (next != succ))
        {
            uint label = m_pattern->GetBlockId(succ);
            m_encoder->Jump(label);
            m_encoder->Push();
        }
    }
}

void EmitPass::emitDiscardBranch(
    BranchInst* branch, const SSource* cond)
{
    if (m_pattern->NeedVMask())
    {
        emitBranch(branch, cond, EPRED_ALL);
    }
    else
    {
        emitBranch(branch, cond, EPRED_NORMAL);
    }
}

void EmitPass::SplitSIMD(llvm::Instruction* inst, uint numSources, uint headerSize, CVariable* payload, SIMDMode mode, uint half)
{
    for(uint i = 0; i < numSources; ++i)
    {
        uint subVarIdx = numLanes(mode) / (SIZE_GRF >> 2) * i + headerSize;

        CVariable* rawDst = payload;
        CVariable* src = GetSymbol(inst->getOperand(i));
        // The source have to match for a raw copy
        if(src->GetType() != payload->GetType())
        {
            rawDst = m_currShader->BitCast(payload, src->GetType());
        }
        m_encoder->SetSimdSize(mode);
        m_encoder->SetDstSubVar(subVarIdx);
        m_encoder->SetSrcSubVar(0,half);
        m_encoder->SetMask(half == 0 ? EMASK_Q1 : EMASK_Q2);
        m_encoder->Copy(rawDst, src);
        m_encoder->Push();
    }
}

void EmitPass::JoinSIMD(CVariable* tempdst[], uint responseLength)
{
    uint iterationCount =  numLanes(m_currShader->m_SIMDSize) / numLanes(SIMDMode::SIMD8);
    for(uint half = 0; half < iterationCount; half++)
    {
        for(uint i = 0; i < responseLength; ++i)
        {
            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            uint subVarIdx = numLanes(SIMDMode::SIMD16) / (SIZE_GRF >> 2) * i;
            m_encoder->SetSrcSubVar(0,i);
            m_encoder->SetDstSubVar(subVarIdx+half);
            m_encoder->SetMask(half == 0 ? EMASK_Q1 : EMASK_Q2);
            m_encoder->Copy(m_destination, tempdst[half]);
            m_encoder->Push();
        }
    }
}

CVariable* EmitPass::BroadcastIfUniform(CVariable* pVar)
{
    assert(pVar && "pVar is null");
    VISA_Type VarT = pVar->GetType();
    bool Need64BitEmu = m_currShader->m_Platform->hasNo64BitInst() &&
        (VarT == ISA_TYPE_Q || VarT == ISA_TYPE_UQ);
    bool IsImm = pVar->IsImmediate();
    if (pVar->IsUniform())
    {
        uint32_t width = numLanes(m_currShader->m_SIMDSize);
        uint elts = IsImm ? 1 : pVar->GetNumberElement();
        CVariable* pBroadcast =
            m_currShader->GetNewVariable(elts * width, pVar->GetType(), EALIGN_GRF);
        CVariable* Dst = pBroadcast;
        CVariable* Src = pVar;
        CVariable *ImmLo = nullptr, *ImmHi = nullptr;
        unsigned Stride = 1;
        if (Need64BitEmu) {
            Dst = m_currShader->GetNewAlias(pBroadcast, ISA_TYPE_UD, 0, 0);
            if (IsImm) {
                uint64_t Imm = pVar->GetImmediateValue();
                ImmLo = m_currShader->ImmToVariable(Imm & 0xFFFFFFFFULL, ISA_TYPE_UD);
                ImmHi = m_currShader->ImmToVariable(Imm >> 32, ISA_TYPE_UD);
            } else {
                Src = m_currShader->GetNewAlias(pVar, ISA_TYPE_UD, 0, 0);
            }
            Stride = 2;
        }

        for (uint i = 0; i < elts; ++i)
        {
            m_encoder->SetSrcSubReg(0, i * Stride);
            if (Stride != 1) m_encoder->SetDstRegion(Stride);
            m_encoder->SetDstSubReg((i * Stride) * width);
            m_encoder->Copy(Dst, ImmLo ? ImmLo : Src);
            m_encoder->Push();
            if (Need64BitEmu) {
                m_encoder->SetSrcSubReg(0, i * Stride + 1);
                if (Stride != 1) m_encoder->SetDstRegion(Stride);
                m_encoder->SetDstSubReg((i * Stride) * width + 1);
                m_encoder->Copy(Dst, ImmHi ? ImmHi : Src);
                m_encoder->Push();
            }
        }

        pVar = pBroadcast;
    }

    return pVar;
}

CVariable* EmitPass::GetExecutionMask()
{
    CVariable *eMask = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true);
    CVariable *dMask = m_currShader->GetSR0();
    if (m_pattern->NeedVMask())
    {
        // use vmask
        m_encoder->SetSrcSubReg(0, 3);
    }
    else
    {
        // use dmask
        m_encoder->SetSrcSubReg(0, 2);
    }

    m_encoder->And(eMask, dMask, m_currShader->GetCE0());
    m_encoder->Push();

    return eMask;
}

/// UniformCopy - Copy a non-uniform source into a uniform variable by copying
/// ANY active elements.

CVariable* EmitPass::UniformCopy(CVariable *var)
{
    CVariable* offset = nullptr;
    return UniformCopy(var, offset);
}

/// Uniform copy allowing to reuse the off calculated by a previous call
/// This allow avoiding redundant code
CVariable* EmitPass::UniformCopy(CVariable *var, CVariable*& off)
{
    assert(!var->IsUniform() && "Expect non-uniform source!");

    if(off == nullptr)
    {
        CVariable* eMask = GetExecutionMask();
        // Get offset to any 1s. For simplicity, use 'fbl' to find the lowest 1s.
        off = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true);
        m_encoder->Fbl(off, eMask);
        m_encoder->Push();

        // Calculate byte offset
        CVariable *shAmt = nullptr;
        switch(var->GetElemSize()) {
        case 1:
            // No need to shift.
            break;
        case 2:
            shAmt = m_currShader->ImmToVariable(1, ISA_TYPE_W);
            break;
        case 4:
            shAmt = m_currShader->ImmToVariable(2, ISA_TYPE_W);
            break;
        case 8:
            shAmt = m_currShader->ImmToVariable(3, ISA_TYPE_W);
            break;
        default:
            assert(false && "Unsupported element size!");
            break;
        }
        if(shAmt) {
            m_encoder->Shl(off, off, shAmt);
            m_encoder->Push();
        }
        off = m_currShader->BitCast(off, ISA_TYPE_UW);
    }
    // Calculate that active lane address.
    CVariable *addr = m_currShader->GetNewAddressVariable(1, var->GetType(), true, true);

    // Now, we need to jump through a few hoops for SIMD32, since the variables
    // representing all of the SIMD lanes may not be consecutive.
    uint8_t numInstances = var->GetNumberInstance();

    if (numInstances == 2)
    {
        uint16_t numElements = var->GetNumberElement();
        VISA_Type dataType = var->GetType();

        // Create a variable into which we'll merge both instances of the original variable,
        // and an alias into the upper half.
        CVariable *merged = m_currShader->GetNewVariable(numElements * numInstances,
            dataType, var->GetAlign(), false, 1);
        CVariable *upperMerged = m_currShader->GetNewAlias(merged, dataType,
            numElements * m_encoder->GetCISADataTypeSize(dataType), numElements);

        // Now, do the copies.
        bool isSecondHalf = m_encoder->IsSecondHalf();

        m_encoder->SetSecondHalf(false);
        m_encoder->Copy(merged, var);
        m_encoder->Push();

        m_encoder->SetSecondHalf(true);
        m_encoder->Copy(upperMerged, var);
        m_encoder->Push();

        m_encoder->SetSecondHalf(false);
        m_encoder->AddrAdd(addr, merged, off);
        m_encoder->Push();
        m_encoder->SetSecondHalf(isSecondHalf);
    }
    else
    {
        m_encoder->AddrAdd(addr, var, off);
        m_encoder->Push();
    }

    // Indirect access to that active scalar register.
    CVariable *exVal = m_currShader->GetNewVariable(1, var->GetType(), CEncoder::GetCISADataTypeAlignment(var->GetType()), true);
    m_encoder->Copy(exVal, addr);

    return exVal;
}

CVariable* EmitPass::ExtendVariable(CVariable *pVar, e_alignment uniformAlign) {
    if (pVar->GetElemSize() >= 4) {
        // There's no need to extend the operand. But, if the variable holding
        // a uniform value is not aligned to GRF, additional copy is required
        // to align it for SIMD1 gather/scatter.
        if (!pVar->IsUniform())
            return pVar;
        if (!pVar->IsImmediate() && pVar->IsGRFAligned(uniformAlign))
            return pVar;
        // Otherwise, we need to re-align the variable holding that uniform value.
    }

    VISA_Type NewType = ISA_TYPE_UD;
    if (pVar->GetElemSize() > 4)
        NewType = ISA_TYPE_UQ;

    // Cast to extend and/or re-align the variable.
    CVariable *NewVar = 0;
    if (pVar->IsUniform()) {
        NewVar = m_currShader->GetNewVariable(1, NewType, uniformAlign, true);
    } else {
        NewVar = m_currShader->GetNewVariable(numLanes(m_currShader->m_SIMDSize),
                                           NewType, EALIGN_GRF);
    }

    if (pVar->IsImmediate()) {
        pVar =
            m_currShader->ImmToVariable(pVar->GetImmediateValue(),
                                        GetUnsignedIntegerType(pVar->GetType()));
    } else {
        pVar =
            m_currShader->GetNewAlias(pVar,
                                      GetUnsignedIntegerType(pVar->GetType()),
                                      0, 0);
    }

    m_encoder->Cast(NewVar, pVar);
    m_encoder->Push();
    return NewVar;
}

CVariable* EmitPass::BroadcastAndExtend(CVariable* pVar)
{
    VISA_Type varType = pVar->GetType();
    const int typeSize = CEncoder::GetCISADataTypeSize(varType);

    if (!pVar->IsUniform() && typeSize >= 4)
    {
        return pVar;
    }

    if (pVar->IsImmediate())
    {
        pVar = m_currShader->ImmToVariable(
            pVar->GetImmediateValue(),
            GetUnsignedIntegerType(pVar->GetType()));
    }
    else
    {
        pVar = m_currShader->GetNewAlias(pVar, GetUnsignedIntegerType(pVar->GetType()), 0, 0);
    }

    const VISA_Type broadcastType = typeSize == 8 ? ISA_TYPE_UQ : ISA_TYPE_UD;

    CVariable* pBroadcast = m_currShader->GetNewVariable(
        numLanes(m_currShader->m_SIMDSize),
        broadcastType,
        EALIGN_GRF);

    m_encoder->Cast(pBroadcast, pVar);
    m_encoder->Push();

    return pBroadcast;
}

CVariable* EmitPass::TruncatePointer(CVariable* pVar) {
    assert((pVar->GetElemSize() == 4 || pVar->GetElemSize() == 8) &&
           "It's expected that pointer is either 32-bit or 64-bit integer!");

    // 32-bit integer
    if (pVar->GetElemSize() == 4) {
        if (!pVar->IsUniform())
            return pVar;
        // For uniform variable, we need to re-align to GRF to ensure it's
        // placed at the 1st element.
        if (!pVar->IsImmediate() && pVar->IsGRFAligned())
            return pVar;
        // Re-align the container of the pointer.
    }

    // Cast to truncate and/or re-align the variable.
    CVariable *NewVar = 0;
    if (pVar->IsUniform()) {
        NewVar = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_GRF, true);
    } else {
        NewVar = m_currShader->GetNewVariable(numLanes(m_currShader->m_SIMDSize),
                                           ISA_TYPE_UD, EALIGN_GRF);
    }
    m_encoder->Cast(NewVar, pVar);
    m_encoder->Push();

    return NewVar;
}

CVariable* EmitPass::ReAlignUniformVariable(CVariable* pVar, e_alignment align) {
    if (!pVar->IsUniform())
        return pVar;

    if (!pVar->IsImmediate() && pVar->IsGRFAligned(align))
        return pVar;

    CVariable *NewVar = m_currShader->GetNewVariable(1, pVar->GetType(), align, true);

    m_encoder->Cast(NewVar, pVar);
    m_encoder->Push();

    return NewVar;
}

CVariable* EmitPass::BroadcastAndTruncPointer(CVariable* pVar)
{
    if (pVar->GetElemSize() == 8)
    {
        // If the pointer is 64-bit, trunc it to 32-bit.
        // Note that we don't care if the pointer is uniform or not,
        // if it's uniform the trunc will also broadcast.
        CVariable* pTrunc = m_currShader->GetNewVariable(
            numLanes(m_currShader->m_SIMDSize),
            ISA_TYPE_UD,
            EALIGN_GRF);

        m_encoder->Cast(pTrunc, pVar);
        m_encoder->Push();
        pVar = pTrunc;
    }
    else
    {
        pVar = BroadcastIfUniform(pVar);
    }

    return pVar;
}

void EmitPass::ValidateNumberofSources(EOPCODE opCode, uint &numberofSrcs)
{
    PayloadMapping::ValidateNumberofSources(opCode, numberofSrcs);
}

// Method used to emit reads from GS SGV variables that are not per-vertex.
// Only two cases exist: PrimitiveID, GSInstanceID
void EmitPass::emitGS_SGV(SGVIntrinsic* pInst)
{
    assert(m_currShader->GetShaderType()==ShaderType::GEOMETRY_SHADER);
    CGeometryShader* gsProgram = static_cast<CGeometryShader*>(m_currShader);
    switch (pInst->getUsage())
    {
    case PRIMITIVEID:
        {
            CVariable* pPrimitiveID = gsProgram->GetPrimitiveID();
            m_currShader->CopyVariable(m_destination, pPrimitiveID);
            break;
        }
    case GS_INSTANCEID:
        {
            CVariable* pInstanceID = gsProgram->GetInstanceID();
            assert (pInstanceID != nullptr);
            m_currShader->CopyVariable(m_destination, pInstanceID);
            break;
        }
    default:
        assert(0 && "This should not happen after lowering to URB reads.");
    }
}

void EmitPass::emitSampleOffset(GenIntrinsicInst* inst)
{
    assert(m_currShader->GetShaderType() == ShaderType::PIXEL_SHADER);
    CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);
    CVariable* offsets = nullptr;
    if(inst->getIntrinsicID() == GenISAIntrinsic::GenISA_SampleOffsetX)
    {
        offsets = psProgram->GetSampleOffsetX();
    }
    else
    {
        offsets = psProgram->GetSampleOffsetY();
    }
    CVariable* pDstArrElm = m_currShader->GetNewAddressVariable(
        numLanes(m_currShader->m_SIMDSize),
        offsets->GetType(),
        false,
        true);

    CVariable* index = GetSymbol(inst->getOperand(0));

    CVariable* pIndexVar = m_currShader->BitCast(index, ISA_TYPE_UW);
    if(!pIndexVar->IsUniform())
    {
        m_encoder->SetSrcRegion(1, 16, 8, 2);
    }
    m_encoder->AddrAdd(pDstArrElm, offsets, pIndexVar);
    m_encoder->Push();

    m_encoder->Cast(m_destination, pDstArrElm);
    m_encoder->Push();

}

CVariable* EmitPass::ReduceHelper(e_opcode op, VISA_Type type, SIMDMode simd, CVariable* var)
{
    CVariable* previousTemp = var;
    auto alignment = type == ISA_TYPE_Q || type == ISA_TYPE_UQ || type == ISA_TYPE_DF ?
        IGC::EALIGN_QWORD : IGC::EALIGN_DWORD;
    CVariable* temp = m_currShader->GetNewVariable(
        numLanes(simd),
        type,
        alignment,
        false);
    m_encoder->SetNoMask();
    m_encoder->SetSimdSize(simd);
    m_encoder->SetSrcSubReg(1, numLanes(simd));
    m_encoder->GenericAlu(op, temp, previousTemp, previousTemp);
    m_encoder->Push();
    return temp;
}

// do reduction 
void EmitPass::emitReductionAll(
    e_opcode op, uint64_t identityValue, VISA_Type type, bool negate, CVariable* pSrc, CVariable* dst)
{
    CVariable* srcH1 = m_currShader->GetNewVariable(
        numLanes(m_currShader->m_SIMDSize),
        type,
        IGC::EALIGN_GRF,
        false);
    CVariable* pIdentityValue = m_currShader->ImmToVariable(identityValue, type);
    m_encoder->SetNoMask();
    m_encoder->Copy(srcH1, pIdentityValue);
    m_encoder->Push();

    if(negate)
    {
        m_encoder->SetSrcModifier(0, EMOD_NEG);
    }
    m_encoder->Copy(srcH1, pSrc);
    m_encoder->Push();
    CVariable* temp = srcH1;
    if(m_currShader->m_dispatchSize == SIMDMode::SIMD32)
    {
        CVariable* srcH2 = m_currShader->GetNewVariable(
            numLanes(m_currShader->m_SIMDSize),
            type,
            IGC::EALIGN_GRF,
            false);
        m_encoder->SetNoMask();
        m_encoder->Copy(srcH2, pIdentityValue);
        m_encoder->Push();

        m_encoder->SetSecondHalf(true);
        if(negate)
        {
            m_encoder->SetSrcModifier(0, EMOD_NEG);
        }
        m_encoder->Copy(srcH2, pSrc);
        m_encoder->Push();
        m_encoder->SetSecondHalf(false);

        temp = m_currShader->GetNewVariable(
            numLanes(SIMDMode::SIMD16),
            type,
            IGC::EALIGN_GRF,
            false);
        m_encoder->SetNoMask();
        m_encoder->SetSimdSize(SIMDMode::SIMD16);
        m_encoder->GenericAlu(op, temp, srcH1, srcH2);
        m_encoder->Push();
    }
    if(m_currShader->m_dispatchSize >= SIMDMode::SIMD16)
    {
        temp = ReduceHelper(op, type, SIMDMode::SIMD8, temp);
    }
    temp = ReduceHelper(op, type, SIMDMode::SIMD4, temp);
    temp = ReduceHelper(op, type, SIMDMode::SIMD2, temp);
    
    m_encoder->SetSrcSubReg(1, 1);
    m_encoder->SetSrcRegion(0, 0, 1, 0);
    m_encoder->SetSrcRegion(1, 0, 1, 0);
    m_encoder->GenericAlu(op, dst, temp, temp);
    m_encoder->Push();
}

void EmitPass::emitPreOrPostFixOp(e_opcode op, uint64_t identityValue, VISA_Type type, bool negateSrc, CVariable* pSrc, CVariable* pSrcsArr[2], bool isPrefix, bool isQuad)
{
    // This is to handle cases when not all lanes are enabled. In that case we fill the lanes with 0.
    bool isSimd32 = (m_currShader->m_dispatchSize == SIMDMode::SIMD32);
    int counter = 1;
    if(isSimd32)
        counter = 2;
    CVariable* maskedSrc[2] = { 0 };
    for(int i = 0; i < counter; ++i)
    {
        CVariable* pSrcCopy = m_currShader->GetNewVariable(
            numLanes(m_currShader->m_SIMDSize),
            type,
            IGC::EALIGN_GRF,
            false);


        // Set the GRF to 0 with no mask. This will set all the registers to 0
        CVariable* pIdentityValue = m_currShader->ImmToVariable(identityValue, type);
        m_encoder->SetNoMask();
        m_encoder->Copy(pSrcCopy, pIdentityValue);
        m_encoder->Push();

        // Now copy the src with a mask so the disabled lanes still keep their 0
        if(negateSrc)
        {
            m_encoder->SetSrcModifier(0, EMOD_NEG);
        }
        m_encoder->SetSecondHalf(i == 1);
        m_encoder->Copy(pSrcCopy, pSrc);
        m_encoder->Push();

        // For case where we need the prefix shift the source by 1 lane
        if(isPrefix)
        {
            maskedSrc[i] = pSrcCopy;
            pSrcCopy = m_currShader->GetNewVariable(pSrcCopy);
            // Copy identity
            m_encoder->SetSimdSize(SIMDMode::SIMD1);
            m_encoder->SetNoMask();
            if(i == 0)
            {
                m_encoder->Copy(pSrcCopy, pIdentityValue);
            }
            else
            {
                m_encoder->SetSrcSubReg(0, 15);
                m_encoder->Copy(pSrcCopy, maskedSrc[i-1]);
            }
            m_encoder->Push();
            // Copy remained data
            unsigned int simdsize = numLanes(m_currShader->m_SIMDSize);
            unsigned int offset = 1;
            while(simdsize > 1)
            {
                simdsize = simdsize >> 1;
                int numInst = m_encoder->GetCISADataTypeSize(type) == 8 &&
                    simdsize == 8 ? 2 : 1;
                for(int instNum = 0; instNum < numInst; ++instNum)
                {
                    m_encoder->SetSimdSize(lanesToSIMDMode(simdsize / numInst));
                    m_encoder->SetDstSubReg(offset + instNum * 4);
                    m_encoder->SetSrcSubReg(0, offset - 1 + instNum * 4);
                    m_encoder->SetNoMask();
                    m_encoder->Copy(pSrcCopy, maskedSrc[i]);
                    m_encoder->Push();
                }
                offset += simdsize;
            }
        }
        pSrcsArr[i] = pSrcCopy;
    }
    for(int i = 0; i < counter; ++i)
    {
        /*
        Copy the adjacent elements.
        for example: r10 be the register
        ____      ____      ____      ____
        __|____|____|____|____|____|____|____|_
        |  7 |  6 |  5 |  4 |  9 |  5 |  3 |  2 |
        ---------------------------------------
        */

        {
            // So then start adding from r10.0 & r10.1
            int numInst = m_encoder->GetCISADataTypeSize(type) == 8 &&
                m_currShader->m_SIMDSize != SIMDMode::SIMD8 ? 2 : 1;
            auto simdSize = m_encoder->GetCISADataTypeSize(type) == 8 ||
                m_currShader->m_SIMDSize == SIMDMode::SIMD8 ? SIMDMode::SIMD4 : SIMDMode::SIMD8;

            for (int instNum = 0; instNum < numInst; ++instNum)
            {
                m_encoder->SetSimdSize(simdSize);
                m_encoder->SetNoMask();
                m_encoder->SetSrcSubVar(0, instNum*2);
                m_encoder->SetSrcRegion(0, 2, 1, 0);
                m_encoder->SetSrcSubVar(1, instNum*2);
                m_encoder->SetSrcRegion(1, 2, 1, 0);
                m_encoder->SetSrcSubReg(1, 1);
                m_encoder->SetDstSubVar(instNum*2);
                m_encoder->SetDstRegion(2);
                m_encoder->SetDstSubReg(1);
                m_encoder->GenericAlu(op, pSrcsArr[i], pSrcsArr[i], pSrcsArr[i]);
                m_encoder->Push();
            }
        }

        /*
        ____                  ____
        _______|____|________________|____|______           ___________________________________________
        |  13 |  6 |  9 |  4 |  14 |  5 |  5 |  2 |    ==>  |  13 |  15 |  9 |  4 |  14 |  10 |  5 |  2 |
        -----------------------------------------           -------------------------------------------
        */
        // Now we have a weird copy happening. This will be done by SIMD 2 instructions.

        {
            int numInst = m_encoder->GetCISADataTypeSize(type) == 8 &&
                m_currShader->m_SIMDSize != SIMDMode::SIMD8 ? 2 : 1;
            auto simdSize = m_encoder->GetCISADataTypeSize(type) == 8 ||
                m_currShader->m_SIMDSize == SIMDMode::SIMD8 ? SIMDMode::SIMD2 : SIMDMode::SIMD4;

            for (int instNum = 0; instNum < numInst; ++instNum)
            {
                m_encoder->SetSimdSize(simdSize);
                m_encoder->SetNoMask();
                m_encoder->SetSrcSubVar(0, instNum*2);
                m_encoder->SetSrcSubReg(0, 2);
                m_encoder->SetSrcSubVar(1, instNum*2);
                m_encoder->SetSrcSubReg(1, 1);
                m_encoder->SetDstSubVar(instNum*2);
                m_encoder->SetDstSubReg(2);
                m_encoder->SetSrcRegion(0, 4, 1, 0);
                m_encoder->SetSrcRegion(1, 4, 1, 0);
                m_encoder->SetDstRegion(4);
                m_encoder->GenericAlu(op, pSrcsArr[i], pSrcsArr[i], pSrcsArr[i]);
                m_encoder->Push();
            }
        }

        /*
        ___________           ___________
        __|___________|_________|___________|______         ___________________________________________
        |  13 |  15 |  9 |  4 |  14 |  10 |  5 |  2 |  ==>  |  22 |  15 |  9 |  4 |  19 |  10 |  5 |  2 |
        -------------------------------------------         -------------------------------------------
        */

        {
            int numInst = m_encoder->GetCISADataTypeSize(type) == 8 &&
                m_currShader->m_SIMDSize != SIMDMode::SIMD8 ? 2 : 1;
            auto simdSize = m_encoder->GetCISADataTypeSize(type) == 8 ||
                m_currShader->m_SIMDSize == SIMDMode::SIMD8 ? SIMDMode::SIMD2 : SIMDMode::SIMD4;

            for (int instNum = 0; instNum < numInst; ++instNum)
            {
                m_encoder->SetSimdSize(simdSize);
                m_encoder->SetNoMask();
                m_encoder->SetSrcSubVar(0, instNum*2);
                m_encoder->SetSrcSubReg(0, 3);
                m_encoder->SetSrcSubVar(1, instNum*2);
                m_encoder->SetSrcSubReg(1, 1);
                m_encoder->SetDstSubVar(instNum*2);
                m_encoder->SetDstSubReg(3);
                m_encoder->SetSrcRegion(0, 4, 1, 0);
                m_encoder->SetSrcRegion(1, 4, 1, 0);
                m_encoder->SetDstRegion(4);
                m_encoder->GenericAlu(op, pSrcsArr[i], pSrcsArr[i], pSrcsArr[i]);
                m_encoder->Push();
            }
        }

        if (isQuad)
        {
            // For quads, we don't want ALU ops across SIMD4 lanes, so stop here
            continue;
        }

        /*
        ____
        __________________|____|_________________         ____________________________________________
        | 22 |  15 |  9 |  4 | 19 |  10 |  5 |  2 |  ==>  |  22 |  15 |  9 |  23 |  19 |  10 |  5 |  2 |
        -----------------------------------------         --------------------------------------------
        _________
        _____________|_________|_________________         _____________________________________________
        | 22 |  15 |  9 |  4 | 19 |  10 |  5 |  2 |  ==>  |  22 |  15 |  28 |  23 |  19 |  10 |  5 |  2 |
        -----------------------------------------         ---------------------------------------------

        ______________
        ________|______________|_________________         _____________________________________________
        | 22 |  15 |  9 |  4 | 19 |  10 |  5 |  2 |  ==>  |  22 |  34 |  28 |  23 |  19 |  10 |  5 |  2 |
        -----------------------------------------         ---------------------------------------------

        ____________________
        __|____________________|_________________         _____________________________________________
        | 22 |  15 |  9 |  4 | 19 |  10 |  5 |  2 |  ==>  |  41 |  34 |  28 |  23 |  19 |  10 |  5 |  2 |
        -----------------------------------------         ---------------------------------------------
        */

        // Because we write continuous elements in the one above, for SIMD16 we have to split into
        // 2 SIMD4's.
        uint numTimesToLoop = numLanes(m_currShader->m_SIMDSize) / numLanes(SIMDMode::SIMD8);

        for(uint loop_counter = 0; loop_counter < numTimesToLoop; ++loop_counter)
        {
            m_encoder->SetSimdSize(SIMDMode::SIMD4);
            m_encoder->SetSrcSubReg(0, loop_counter * 8 + 3);
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->SetSrcSubReg(1, loop_counter * 8 + 4);
            m_encoder->SetSrcRegion(1, 4, 4, 1);
            m_encoder->SetDstSubReg(loop_counter * 8 + 4);
            m_encoder->SetNoMask();
           
            m_encoder->GenericAlu(op, pSrcsArr[i], pSrcsArr[i], pSrcsArr[i]);
            m_encoder->Push();
        }

        if(m_currShader->m_SIMDSize == SIMDMode::SIMD16 || isSimd32)
        {
            // Add the last element of the 1st GRF to all the elements of the 2nd GRF
            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            m_encoder->SetNoMask();
            m_encoder->SetSrcSubReg(0, 7);
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->SetSrcSubReg(1, 8);
            m_encoder->SetSrcRegion(1, 1, 1, 0);
            m_encoder->SetDstSubReg(8);
            m_encoder->GenericAlu(op, pSrcsArr[i], pSrcsArr[i], pSrcsArr[i]);
            m_encoder->Push();
        }
    }
    
    if(isSimd32 && !isQuad)
    {
        // For SIMD32 we need to write the last element of the prev element to the next 16 elements
        m_encoder->SetSimdSize(SIMDMode::SIMD16);
        m_encoder->SetNoMask();
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        m_encoder->SetSrcSubReg(0, numLanes(m_currShader->m_SIMDSize) - 1);
        m_encoder->GenericAlu(op, pSrcsArr[1], pSrcsArr[0], pSrcsArr[1]);
        m_encoder->Push();
    }
    // reset second half state
    m_encoder->SetSecondHalf(false);
}

/*
ScalarAtomics: This optimization attempts to reduce the number of atomic instructions issued when
the destination addresses and the source are both uniform. For example lets say we have an atomic
add happens with destination address as <addr> = constant. <src> = constant too. In this case, lets
say for SIMD8 there are 8 lanes trying to write to the same address. H/W will serialize this to
8 back to back atomic instructions which are extremely slow to execute.
*/
void EmitPass::emitScalarAtomics(
    llvm::Instruction* pInst,
    const ResourceDescriptor& resource,
    AtomicOp atomic_op,
    CVariable* pDstAddr,
    CVariable* pSrc,
    bool isA64,
    bool is16Bit)
{
    e_opcode op = EOPCODE_ADD;
    // find the value for which opcode(x, identity) == x
    unsigned int identityValue = 0;
    switch(atomic_op)
    {
    case EATOMIC_IADD:
    case EATOMIC_SUB:
    case EATOMIC_INC:
    case EATOMIC_DEC:
        identityValue = 0;
        op = EOPCODE_ADD;
        break;
    case EATOMIC_UMAX:
        identityValue = 0;
        op = EOPCODE_MAX;
        break;
    case EATOMIC_IMAX:
        identityValue = 0x80000000;
        op = EOPCODE_MAX;
        break;
    case EATOMIC_UMIN:
        identityValue = 0xFFFFFFFF;
        op = EOPCODE_MIN;
        break;
    case EATOMIC_IMIN:
        identityValue = 0X7FFFFFFF;
        op = EOPCODE_MIN;
        break;
    default:
        assert(0 && "unsupported scalar atomic type");
        break;
    }

    VISA_Type type = is16Bit ? ISA_TYPE_W : ISA_TYPE_D;
    if (atomic_op == EATOMIC_INC || atomic_op == EATOMIC_DEC)
    {
        if (atomic_op == EATOMIC_INC)
        {
            atomic_op = EATOMIC_IADD;
        }
        else
        {
            atomic_op = EATOMIC_SUB;
        }

        pSrc = m_currShader->ImmToVariable(1, type);
    }
    if(atomic_op == EATOMIC_UMAX || atomic_op == EATOMIC_UMIN)
    {
        type = GetUnsignedType(type);
    }
    AtomicOp uniformAtomicOp = atomic_op;
    bool negateSrc = false;
    if(atomic_op == EATOMIC_SUB)
    {
        negateSrc = true;
        uniformAtomicOp = EATOMIC_IADD;
    }
    bool returnsImmValue = (!pInst->use_empty());
    CVariable* pFinalAtomicSrcVal = m_currShader->GetNewVariable(
        1,
        type,
        isA64 ? IGC::EALIGN_2GRF : IGC::EALIGN_GRF,
        true);
    CVariable *pSrcsArr[2] = { nullptr, nullptr };
    if(returnsImmValue)
    {
        // sum all the lanes
        emitPreOrPostFixOp(op, identityValue, type, negateSrc, pSrc, pSrcsArr);

        CVariable *pSrcCopy = pSrcsArr[0];
        if(m_currShader->m_dispatchSize == SIMDMode::SIMD32)
            pSrcCopy = pSrcsArr[1];

        m_encoder->SetSrcRegion(0, 0, 1, 0);
        m_encoder->SetSrcSubReg(0, numLanes(m_currShader->m_SIMDSize) - 1);
        m_encoder->Copy(pFinalAtomicSrcVal, pSrcCopy);
        m_encoder->Push();
    }
    else
    {
        emitReductionAll(op, identityValue, type, negateSrc, pSrc, pFinalAtomicSrcVal);
    }

    if (pDstAddr->IsImmediate())
    {
        CVariable* pDstAddrCopy = m_currShader->GetNewVariable(1, ISA_TYPE_UD, IGC::EALIGN_GRF, true);
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
        m_encoder->Copy(pDstAddrCopy, pDstAddr);
        m_encoder->Push();
        pDstAddr = pDstAddrCopy;
    }

    m_encoder->SetSimdSize(SIMDMode::SIMD1);
    m_encoder->SetNoMask();

    CVariable *pReturnVal = returnsImmValue ?
        m_currShader->GetNewVariable(1, ISA_TYPE_UD, IGC::EALIGN_GRF, true) :
        nullptr;

    if (is16Bit)
    {
        CVariable *pCastAtomicSrcVal =
            m_currShader->GetNewVariable(1, ISA_TYPE_UD, IGC::EALIGN_GRF, true);

        m_encoder->Cast(pCastAtomicSrcVal, pFinalAtomicSrcVal);
        pFinalAtomicSrcVal = pCastAtomicSrcVal;
    }

    if(isA64)
    {
        m_encoder->AtomicRawA64(uniformAtomicOp, pReturnVal, pDstAddr, pFinalAtomicSrcVal, nullptr, is16Bit);
    }
    else
    {
        m_encoder->DwordAtomicRaw(uniformAtomicOp, resource, pReturnVal, pDstAddr, pFinalAtomicSrcVal, nullptr, is16Bit);
    }
    m_encoder->Push();

    if (returnsImmValue)
    {
        unsigned int counter = m_currShader->m_dispatchSize == SIMDMode::SIMD32 ? 2 : 1;
        assert(op == EOPCODE_ADD && "we can only get the return value for add right now");
        for (unsigned int i = 0; i < counter; ++i)
        {
            m_encoder->SetNoMask();
            m_encoder->Add(pSrcsArr[i], pSrcsArr[i], pReturnVal);
            m_encoder->Push();

            if (atomic_op == EATOMIC_IADD)
            {
                m_encoder->SetSrcModifier(1, EMOD_NEG);
            }

            m_encoder->SetSecondHalf(i == 1);
            m_encoder->Add(m_destination, pSrcsArr[i], pSrc);
            m_encoder->Push();
        }
    }
}

bool EmitPass::IsUniformAtomic(llvm::Instruction* pInst)
{
    if (llvm::GenIntrinsicInst* pIntrinsic = llvm::dyn_cast<llvm::GenIntrinsicInst>(pInst))
    {
        GenISAIntrinsic::ID id = pIntrinsic->getIntrinsicID();

        // Dst address in bytes.
        if (id == GenISAIntrinsic::GenISA_intatomicraw ||
            id == GenISAIntrinsic::GenISA_intatomicrawA64)
        {
            if(IGC_IS_FLAG_ENABLED(DisableScalarAtomics) || m_currShader->m_DriverInfo->WASLMPointersDwordUnit())
                return false;
            llvm::Value* pllDstAddr = pInst->getOperand(1);
            CVariable* pDstAddr = GetSymbol(pllDstAddr);
            if (pDstAddr->IsUniform())
            {
                AtomicOp atomic_op = static_cast<AtomicOp>(llvm::cast<llvm::ConstantInt>(pInst->getOperand(3))->getZExtValue());

                bool isAddAtomic = atomic_op == EATOMIC_IADD ||
                    atomic_op == EATOMIC_INC ||
                    atomic_op == EATOMIC_SUB;
                bool isMinMaxAtomic =
                    atomic_op == EATOMIC_UMAX ||
                    atomic_op == EATOMIC_UMIN ||
                    atomic_op == EATOMIC_IMIN ||
                    atomic_op == EATOMIC_IMAX;

                if (isAddAtomic || (isMinMaxAtomic && pInst->use_empty()))
                    return true;
            }
        }
    }

    return false;
}

CVariable *EmitPass::UnpackOrBroadcastIfUniform(CVariable *pVar)
{
    if (pVar->GetElemSize() == 4)
        return BroadcastIfUniform(pVar);

    assert(pVar->GetElemSize() == 2);

    uint16_t elts = numLanes(m_currShader->m_SIMDSize);
    // 16-bit atomics are still aligned at dword boundaries
    // with the upper 16-bits ignored.
    CVariable* pUnpacked =
        m_currShader->GetNewVariable(elts, ISA_TYPE_UD, EALIGN_GRF);

    m_encoder->Cast(pUnpacked, m_currShader->BitCast(pVar, ISA_TYPE_UW));
    return pUnpacked;
}

void EmitPass::emitAtomicRaw(llvm::GenIntrinsicInst* pInsn)
{
    ForceDMask();
    // Currently, Dword Atomics can be called by matching 2 intrinsics. One is the DwordAtomicRaw
    // and AtomicCmpXchg (which has 2 srcs unlike the other atomics).
    assert(pInsn->getNumArgOperands() == 4);

    /// Immediate Atomics return the value before the atomic operation is performed. So that flag
    /// needs to be set for this.
    bool returnsImmValue = !pInsn->use_empty();

    llvm::Value* pllbuffer = pInsn->getOperand(0);
    llvm::Value* pllDstAddr = pInsn->getOperand(1);
    llvm::Value* pllSrc0 = pInsn->getOperand(2);
    ResourceDescriptor resource = GetResourceVariable(pllbuffer);
    AtomicOp atomic_op = EATOMIC_UNDEF;

    CVariable* pSrc0 = nullptr;
    CVariable* pSrc1 = nullptr;
    llvm::GenIntrinsicInst* pIntrinCall = llvm::cast<llvm::GenIntrinsicInst>(pInsn);
    GenISAIntrinsic::ID IID = pIntrinCall->getIntrinsicID();
    if(IID == GenISAIntrinsic::GenISA_icmpxchgatomicraw ||
        IID == GenISAIntrinsic::GenISA_fcmpxchgatomicraw ||
        IID == GenISAIntrinsic::GenISA_icmpxchgatomicrawA64 ||
        IID == GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64)
    {
        llvm::Value* pllSrc1 = pInsn->getOperand(3);
        pSrc1 = GetSymbol(pllSrc1);
        pSrc1 = UnpackOrBroadcastIfUniform(pSrc1);
        if(IID == GenISAIntrinsic::GenISA_fcmpxchgatomicraw ||
            IID == GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64)
        {
            atomic_op = EATOMIC_FCMPWR;
        }
        else
        {
            atomic_op = EATOMIC_CMPXCHG;
        }
    }
    else
    {
        atomic_op = static_cast<AtomicOp>(llvm::cast<llvm::ConstantInt>(pInsn->getOperand(3))->getZExtValue());
    }

    const bool is16Bit = (pInsn->getType()->getScalarSizeInBits() == 16);

    // atomic_inc and atomic_dec don't have both src0 and src1.
    if(atomic_op != EATOMIC_INC && atomic_op != EATOMIC_DEC)
    {
        pSrc0 = GetSymbol(pllSrc0);
    }

    // Dst address in bytes.
    CVariable* pDstAddr = GetSymbol(pllDstAddr);
    // If DisableScalarAtomics regkey is enabled or DisableIGCOptimizations regkey is enabled then
    // don't enable scalar atomics
    if (IsUniformAtomic(pInsn))
    {
            PointerType *PtrTy = dyn_cast<PointerType>(pllDstAddr->getType());
            bool isA64 = PtrTy && isA64Ptr(PtrTy, m_currShader->GetContext());
            e_alignment uniformAlign = isA64 ? EALIGN_2GRF : EALIGN_GRF;
            // Re-align the pointer if it's not GRF aligned.
            pDstAddr = ReAlignUniformVariable(pDstAddr, uniformAlign);
            emitScalarAtomics(pInsn, resource, atomic_op, pDstAddr, pSrc0, isA64, is16Bit);
            ResetVMask();
            return;
    }

    pDstAddr = BroadcastIfUniform(pDstAddr);
    if (pSrc0)
    {
        pSrc0 = UnpackOrBroadcastIfUniform(pSrc0);
    }

    if (m_currShader->GetIsUniform(pInsn))
    {
        assert(0 && "Uniform for Atomic Raw is not implemented");
    }
    else
    {
        CVariable* pDst = returnsImmValue ?
            m_currShader->GetNewVariable(numLanes(m_currShader->m_SIMDSize), ISA_TYPE_UD, EALIGN_GRF) :
            nullptr;

        PointerType *PtrTy = dyn_cast<PointerType>(pllDstAddr->getType());
        bool isA64 = PtrTy && isA64Ptr(PtrTy, m_currShader->GetContext());
        if(isA64)
        {
            m_encoder->AtomicRawA64(atomic_op, pDst, pDstAddr, pSrc0, pSrc1, is16Bit);
            m_encoder->Push();
            if (returnsImmValue)
            {
                m_encoder->Cast(
                    m_currShader->BitCast(m_destination, GetUnsignedIntegerType(m_destination->GetType())),
                    pDst);
                m_encoder->Push();
            }
        }
        else
        {
            // TODO: SEND SLM OFFSET IN BYTES
            CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
            if(resource.m_surfaceType == ESURFACE_SLM && ctx->m_DriverInfo.WASLMPointersDwordUnit())
            {
                CVariable* pDwordAddr =
                    m_currShader->GetNewVariable(numLanes(m_currShader->m_SIMDSize),
                    ISA_TYPE_D, IGC::EALIGN_GRF);

                m_encoder->Shl(pDwordAddr, pDstAddr,
                    m_currShader->ImmToVariable(0x2, ISA_TYPE_D));
                m_encoder->Push();
                pDstAddr = pDwordAddr;
            }
            pDstAddr = m_currShader->BitCast(pDstAddr, ISA_TYPE_UD);

            if (pSrc0)
            {
                pSrc0 = m_currShader->BitCast(pSrc0, ISA_TYPE_UD);
            }

            if (pSrc1)
            {
                pSrc1 = m_currShader->BitCast(pSrc1, ISA_TYPE_UD);
            }
            uint label = 0;
            CVariable* flag = nullptr;
            bool needLoop = ResourceLoopHeader(resource, flag, label);
            m_encoder->DwordAtomicRaw(
                atomic_op,
                resource,
                pDst,
                pDstAddr,
                pSrc0,
                pSrc1,
                is16Bit);
            m_encoder->Push();
            if (returnsImmValue)
            {
                m_encoder->Cast(
                    m_currShader->BitCast(m_destination, GetUnsignedIntegerType(m_destination->GetType())),
                    pDst);
                m_encoder->Push();
            }
            ResourceLoop(needLoop, flag, label);
        }
    }
    ResetVMask();
    m_currShader->isMessageTargetDataCacheDataPort = true;
}

void EmitPass::emitAtomicStructured(llvm::Instruction* pInsn)
{
    ForceDMask();
    // Currently, Dword Atomics can be called by matching 2 intrinsics. One is the DwordAtomicRaw
    // and AtomicCmpXchg (which has 2 srcs unlike the other atomics).

    /// Immediate Atomics return the value before the atomic operation is performed. So that flag
    /// needs to be set for this.
    bool returnsImmValue = !pInsn->use_empty();

    llvm::Value* pllbuffer     = pInsn->getOperand(0);
    llvm::Value* pllArrayIdx   = pInsn->getOperand(1);
    llvm::Value* pllByteOffset = pInsn->getOperand(2);
    llvm::Value* pllSrc0       = pInsn->getOperand(3);

    AtomicOp atomic_op = EATOMIC_UNDEF;

    CVariable* pSrc1 = nullptr;

    llvm::CallInst* pCallInst = llvm::cast<llvm::CallInst>(pInsn);

    GenISAIntrinsic::ID IID  = llvm::cast<llvm::GenIntrinsicInst>(pCallInst)->getIntrinsicID();
    if ( (IID == GenISAIntrinsic::GenISA_cmpxchgatomicstructured)||
        (IID == GenISAIntrinsic::GenISA_fcmpxchgatomicstructured) )
    {
        llvm::Value* pllSrc1 = pInsn->getOperand(4);
        pSrc1 = GetSymbol(pllSrc1);
        pSrc1 = BroadcastIfUniform(pSrc1);
        if(IID == GenISAIntrinsic::GenISA_fcmpxchgatomicstructured)
        {
            atomic_op = EATOMIC_FCMPWR;
        }
        else
        {
            atomic_op = EATOMIC_CMPXCHG;
        }
    }
    else
    {
        atomic_op = static_cast<AtomicOp>(llvm::cast<llvm::ConstantInt>(pInsn->getOperand(4))->getZExtValue());
    }

    CVariable* pSrc0 = GetSymbol(pllSrc0);
    pSrc0 = BroadcastIfUniform(pSrc0);

    uint as = cast<PointerType>(pllbuffer->getType())->getAddressSpace();
    uint bufferIndex;
    bool directIdx;
    BufferType bufType = DecodeAS4GFXResource(as, directIdx, bufferIndex);
    assert(bufType == UAV && "Only UAV's are allowed for Alloc Consume instruction");

    CVariable* pArrIdx = GetSymbol(pllArrayIdx);
    CVariable* pByteOffset = GetSymbol(pllByteOffset);

    uint btiIndex = 0;

    if (bufType == UAV)
    {
        btiIndex = m_currShader->m_pBtiLayout->GetUavIndex(bufferIndex);
        m_currShader->SetBindingTableEntryCountAndBitmap(directIdx, btiIndex);
    }
    else
    {
        assert(0 && "Buffer Type not supported by this dword atomic op");
    }

    if (m_currShader->GetIsUniform(pInsn))
    {
        assert(0 && "Uniform ld structured not implemented yet");
    }
    else
    {
        uint parameterLength;

        if (pSrc1)
        {
            parameterLength = 4;
        }
        else
        {
            parameterLength = 3;
        }

        TODO("Atomic Structured needs to use vISA messages instead of raw_send");
        CVariable* pPayload = m_currShader->GetNewVariable(
            parameterLength * numLanes(m_currShader->m_SIMDSize),
            ISA_TYPE_D,
            IGC::EALIGN_GRF);

        if (m_currShader->m_SIMDSize == SIMDMode::SIMD8)
        {
            m_currShader->CopyVariable(pPayload, pArrIdx, 0);
            m_currShader->CopyVariable(pPayload, pByteOffset, 1);
            m_currShader->CopyVariable(pPayload, pSrc0, 2);

            if (pSrc1)
            {
                m_currShader->CopyVariable(pPayload, pSrc1, 3);
            }
        }
        else
        {
            m_currShader->CopyVariable(pPayload, pArrIdx, 0);
            m_currShader->CopyVariable(pPayload, pByteOffset, 2);
            m_currShader->CopyVariable(pPayload, pSrc0, 4);

            if (pSrc1)
            {
                m_currShader->CopyVariable(pPayload, pSrc1, 6);
            }
        }

        auto hw_atomic_op_enum = getHwAtomicOpEnum(atomic_op);
        uint messageLength = parameterLength * (numLanes(m_currShader->m_SIMDSize)/numLanes(SIMDMode::SIMD8));
        uint responseLength = 0;

        CVariable* pDst = nullptr;
        if (returnsImmValue == true)
        {
            responseLength = (numLanes(m_currShader->m_SIMDSize)/numLanes(SIMDMode::SIMD8));
            pDst = m_destination;
        }

        uint messageSpecificControl = encodeMessageDescriptorForAtomicUnaryOp(
            messageLength,
            responseLength,
            false,
            EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_UNTYPED_ATOMIC_OPERATION,
            returnsImmValue,
            m_currShader->m_SIMDSize,
            hw_atomic_op_enum,
            btiIndex);

        CVariable* pMessDesc = m_currShader->ImmToVariable(messageSpecificControl, ISA_TYPE_D);
        uint exDesc = EU_GEN7_5_MESSAGE_TARGET_DATA_PORT_DATA_CACHE_1;

        m_encoder->Send(pDst, pPayload, exDesc, pMessDesc);
        m_encoder->Push();
    }
    ResetVMask();
    m_currShader->isMessageTargetDataCacheDataPort = true;
}

void EmitPass::emitAtomicTyped(GenIntrinsicInst* pInsn)
{
    ForceDMask();
    // Currently, Dword Atomics can be called by matching 2 intrinsics. One is the DwordAtomicRaw
    // and AtomicCmpXchg (which has 2 srcs unlike the other atomics).
    assert(pInsn->getNumArgOperands() == 6);

    /// Immediate Atomics return the value before the atomic operation is performed. So that flag
    /// needs to be set for this.
    bool returnsImmValue = !pInsn->user_empty();

    llvm::Value* pllbuffer     = pInsn->getOperand(0);
    llvm::Value* pllU          = pInsn->getOperand(1);
    llvm::Value* pllV          = pInsn->getOperand(2);
    llvm::Value* pllR          = pInsn->getOperand(3);
    llvm::Value* pllSrc0       = pInsn->getOperand(4);

    AtomicOp atomic_op = EATOMIC_UNDEF;

    CVariable* pSrc0 = nullptr;
    CVariable* pSrc1 = nullptr;

    if (pInsn->getIntrinsicID() == GenISAIntrinsic::GenISA_icmpxchgatomictyped)
    {
        llvm::Value* pllSrc1 = pInsn->getOperand(5);
        pSrc1 = GetSymbol(pllSrc1);
        pSrc1 = UnpackOrBroadcastIfUniform(pSrc1);
        atomic_op = EATOMIC_CMPXCHG;
    }
    else
    {
        atomic_op = static_cast<AtomicOp>(cast<ConstantInt>(pInsn->getOperand(5))->getZExtValue());
    }

    if (atomic_op != EATOMIC_INC && atomic_op != EATOMIC_DEC)
    {
        pSrc0 = GetSymbol(pllSrc0);
        pSrc0 = UnpackOrBroadcastIfUniform(pSrc0);
    }

    ResourceDescriptor resource = GetResourceVariable(pllbuffer);

    CVariable* pU = GetSymbol(pllU);
    CVariable* pV = GetSymbol(pllV);
    CVariable* pR = GetSymbol(pllR);

    pU = BroadcastIfUniform(pU);
    pV = BroadcastIfUniform(pV);
    pR = BroadcastIfUniform(pR);

    if (m_currShader->GetIsUniform(pInsn))
    {
        assert(0 && "Uniform DWordAtomicTyped not implemented yet");
    }
    else
    {
        uint addrDimension = 3;
        while(addrDimension > 1 && isUndefOrConstInt0(pInsn->getOperand(addrDimension)))
        {
            addrDimension--;
        }

        TODO("Adding headers to atomic typed ops is a workaround, verify if this is needed");
        const bool headerPresent = true;

        const uint parameterLength =
            addrDimension + (pSrc0 != nullptr) + (pSrc1 != nullptr) + headerPresent;

        auto hw_atomic_op_enum = getHwAtomicOpEnum(atomic_op);
        uint responseLength = returnsImmValue;

        unsigned int bti = 0;
        if(resource.m_surfaceType == ESURFACE_BINDLESS)
        {
            bti = BINDLESS_BTI;
        }
        else if(resource.m_resource->IsImmediate())
        {
            bti = (uint)resource.m_resource->GetImmediateValue();
        }

        const auto messageType = EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_TYPED_ATOMIC_OPERATION;

        uint messageSpecificControl = encodeMessageDescriptorForAtomicUnaryOp(
            parameterLength,
            responseLength,
            headerPresent,
            messageType,
            returnsImmValue,
            m_currShader->m_SIMDSize,
            hw_atomic_op_enum,
            bti);

        CVariable* pMessDesc = m_currShader->ImmToVariable(messageSpecificControl, ISA_TYPE_D);
        CVariable* exDesc =
            m_currShader->ImmToVariable(EU_GEN7_5_MESSAGE_TARGET_DATA_PORT_DATA_CACHE_1, ISA_TYPE_D);
        if(resource.m_surfaceType == ESURFACE_BINDLESS)
        {
            CVariable* temp = m_currShader->GetNewVariable(resource.m_resource);
            m_encoder->Add(temp, resource.m_resource, exDesc);
            m_encoder->Push();

            exDesc = temp;
        }
        CVariable* tempdst = returnsImmValue ?
            m_currShader->GetNewVariable(
                numLanes(m_currShader->m_SIMDSize), ISA_TYPE_UD, EALIGN_GRF) :
            nullptr;
        CVariable* pPayload[2] = { nullptr, nullptr };
        uint loopIter = numLanes(m_currShader->m_SIMDSize) / numLanes(SIMDMode::SIMD8);
        for (uint i = 0; i < loopIter; ++i)
        {
            pPayload[i] = m_currShader->GetNewVariable(
                parameterLength * numLanes(SIMDMode::SIMD8),
                ISA_TYPE_F,
                IGC::EALIGN_GRF);

            int writeIndex = 0;
            if (headerPresent)
            {
                m_encoder->SetSimdSize(SIMDMode::SIMD1);
                m_encoder->SetDstSubReg(7);
                m_encoder->SetNoMask();
                m_encoder->Copy(pPayload[i], m_currShader->ImmToVariable(0xFF, ISA_TYPE_D));
                m_encoder->Push();
                ++writeIndex;
            }

            auto CopyVar = [&](CVariable *pVar)
            {
                m_encoder->SetSimdSize(SIMDMode::SIMD8);
                m_encoder->SetMask((i == 0) ? EMASK_Q1 : EMASK_Q2);
                if (!pVar->IsUniform())
                {
                    m_encoder->SetSrcSubVar(0, i);
                }
                m_encoder->SetDstSubVar(writeIndex);
                m_encoder->Copy(pPayload[i], pVar);
                m_encoder->Push();
                ++writeIndex;
            };

            CopyVar(pU);

            if (addrDimension > 1)
                CopyVar(pV);

            if (addrDimension > 2)
                CopyVar(pR);

            if (pSrc0)
                CopyVar(pSrc0);

            if (pSrc1)
                CopyVar(pSrc1);
        }

        uint label = 0;
        CVariable* flag = nullptr;
        bool needLoop = ResourceLoopHeader(resource, flag, label);
        if(resource.m_surfaceType == ESURFACE_NORMAL && !resource.m_resource->IsImmediate())
        {
            CVariable* indirectMess = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true);
            m_encoder->Or(indirectMess, pMessDesc, resource.m_resource);
            m_encoder->Push();
            pMessDesc = indirectMess;
        }
        for(uint i = 0; i < loopIter; ++i)
        {
            m_encoder->SetPredicate(flag);
            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            m_encoder->SetMask((i == 0) ? EMASK_Q1 : EMASK_Q2);
            m_encoder->SetDstSubVar(i);
            m_encoder->Send(tempdst, pPayload[i],
                EU_GEN7_5_MESSAGE_TARGET_DATA_PORT_DATA_CACHE_1, exDesc, pMessDesc);
            m_encoder->Push();
        }
        ResourceLoop(needLoop, flag, label);

        if(returnsImmValue)
        {
            m_encoder->Cast(
                m_currShader->BitCast(m_destination, GetUnsignedIntegerType(m_destination->GetType())),
                tempdst);
            m_encoder->Push();
        }
    }
    ResetVMask();
    m_currShader->isMessageTargetDataCacheDataPort = true;
}

void EmitPass::emitLdStructured(llvm::Instruction* pInsn)
{
    llvm::Value* pllArrIdx     = pInsn->getOperand(1);
    llvm::Value* pllByteOffset = pInsn->getOperand(2);

    llvm::Value* pllbuffer = pInsn->getOperand(0);
    uint as = cast<PointerType>(pllbuffer->getType())->getAddressSpace();
    uint bufferIndex;
    bool directIdx;
    BufferType bufType = DecodeAS4GFXResource(as, directIdx, bufferIndex);

    CVariable* pArrIdx = GetSymbol(pllArrIdx);
    CVariable* pByteOffset = GetSymbol(pllByteOffset);

    pByteOffset = BroadcastIfUniform(pByteOffset);
    pArrIdx     = BroadcastIfUniform(pArrIdx);

    uint btiIndex = 0;
    if (bufType == UAV)
    {
        btiIndex = m_currShader->m_pBtiLayout->GetUavIndex(bufferIndex);
    }
    else if (bufType == RESOURCE)
    {
        btiIndex = m_currShader->m_pBtiLayout->GetTextureIndex(bufferIndex);
    }
    else if (bufType == SLM)
    {
        assert(0 && "SLM should use the llvm load instead of ld structured message");
    }
    else
    {
        assert(0 && "Resource type isn't implemented");
    }
    m_currShader->SetBindingTableEntryCountAndBitmap(directIdx, btiIndex);

    if (m_currShader->GetIsUniform(pInsn))
    {
        assert(0 && "Uniform ld structured not implemented yet");
    }
    else
    {
        TODO("Using a raw_send for ld_structured till we get support from vISA");
        uint parameterLength = 2;
        CVariable* pPayload =
            m_currShader->GetNewVariable(
            (parameterLength) * numLanes(m_currShader->m_SIMDSize),
            ISA_TYPE_D,
            IGC::EALIGN_GRF);

        uint responseLength = m_destination->GetNumberElement()/numLanes(m_currShader->m_SIMDSize);
        if (m_currShader->m_SIMDSize == SIMDMode::SIMD8)
        {
            m_currShader->CopyVariable(pPayload, pArrIdx, 0);
            m_currShader->CopyVariable(pPayload, pByteOffset, 1);
        }
        else if (m_currShader->m_SIMDSize == SIMDMode::SIMD16)
        {
            m_currShader->CopyVariable(pPayload, pArrIdx, 0);
            m_currShader->CopyVariable(pPayload, pByteOffset, 2);
            responseLength *= 2;
        }
        uint writeMask = m_currShader->GetExtractMask(pInsn);
        uint messageSpecificControl = encodeMessageSpecificControlForReadWrite(
            EU_DATA_PORT_READ_MESSAGE_TYPE_UNTYPED_SURFACE_READ,
            ConvertChannelMaskToVisaType(writeMask),
            m_currShader->m_SIMDSize);

        uint desc = DataPortRead(
            parameterLength * (numLanes(m_currShader->m_SIMDSize) / numLanes(SIMDMode::SIMD8)),
            responseLength,
            false,
            EU_DATA_PORT_READ_MESSAGE_TYPE_UNTYPED_SURFACE_READ,
            messageSpecificControl,
            false,
            DATA_PORT_TARGET_DATA_CACHE_1,
            btiIndex);

        uint exDesc = EU_GEN7_5_MESSAGE_TARGET_DATA_PORT_DATA_CACHE_1;

        CVariable* pMessDesc = m_currShader->ImmToVariable(desc, ISA_TYPE_D);
        m_encoder->Send(m_destination, pPayload, exDesc, pMessDesc);
        m_encoder->Push();
    }
    m_currShader->isMessageTargetDataCacheDataPort = true;
}

void EmitPass::emitStoreStructured(llvm::Instruction* pInsn)
{
    ForceDMask(false);

    llvm::GenIntrinsicInst* pIntrinsicInst = llvm::cast<llvm::GenIntrinsicInst>(pInsn);
    llvm::Value* pllArrIdx     = pInsn->getOperand(1);
    llvm::Value* pllByteOffset = pInsn->getOperand(2);

    llvm::Value* pllSrc1       = pInsn->getOperand(3);
    llvm::Value* pllSrc2       = nullptr;
    llvm::Value* pllSrc3       = nullptr;
    llvm::Value* pllSrc4       = nullptr;

    llvm::Value* pllbuffer = pInsn->getOperand(0);
    uint as = cast<PointerType>(pllbuffer->getType())->getAddressSpace();
    uint bufferIndex;
    bool directIdx;
    BufferType bufType = DecodeAS4GFXResource(as, directIdx, bufferIndex);

    CVariable* pArrIdx     = GetSymbol(pllArrIdx);
    CVariable* pByteOffset = GetSymbol(pllByteOffset);
    CVariable* pSrc1       = GetSymbol(pllSrc1);

    pByteOffset = BroadcastIfUniform(pByteOffset);
    pArrIdx     = BroadcastIfUniform(pArrIdx);
    pSrc1       = BroadcastIfUniform(pSrc1);

    CVariable* pSrc2 = nullptr;
    CVariable* pSrc3 = nullptr;
    CVariable* pSrc4 = nullptr;

    GenISAIntrinsic::ID id = pIntrinsicInst->getIntrinsicID();
    if (id == GenISAIntrinsic::GenISA_storestructured2 ||
        id == GenISAIntrinsic::GenISA_storestructured3 ||
        id == GenISAIntrinsic::GenISA_storestructured4)
    {
        pllSrc2 = pInsn->getOperand(4);
        pSrc2 = GetSymbol(pllSrc2);
        pSrc2 = BroadcastIfUniform(pSrc2);
    }

    if (id == GenISAIntrinsic::GenISA_storestructured3 ||
        id == GenISAIntrinsic::GenISA_storestructured4)
    {
        pllSrc3 = pInsn->getOperand(5);
        pSrc3 = GetSymbol(pllSrc3);
        pSrc3 = BroadcastIfUniform(pSrc3);
    }

    if (id == GenISAIntrinsic::GenISA_storestructured4)
    {
        pllSrc4 = pInsn->getOperand(6);
        pSrc4 = GetSymbol(pllSrc4);
        pSrc4 = BroadcastIfUniform(pSrc4);
    }

    uint btiIndex = 0;
    if (bufType == UAV)
    {
        btiIndex = m_currShader->m_pBtiLayout->GetUavIndex(bufferIndex);
    }
    else if (bufType == RESOURCE)
    {
        btiIndex = m_currShader->m_pBtiLayout->GetTextureIndex(bufferIndex);
    }
    else if (bufType == SLM)
    {
        assert(0 && "SLM should use the llvm store instead of store structured message");
    }
    else
    {
        assert(0 && "Resource type isn't implemented");
    }
    m_currShader->SetBindingTableEntryCountAndBitmap(directIdx, btiIndex);

    if (m_currShader->GetIsUniform(pInsn))
    {
        assert(0 && "Uniform ld structured not implemented yet");
    }
    else
    {
        TODO("Using a raw_send for ld_structured till we get support from vISA");

        int srcLength = 1;
        VISAChannelMask channelMask = CHANNEL_MASK_R;

        if (pSrc4 != nullptr)
        {
            srcLength = 4;
            channelMask = CHANNEL_MASK_RGBA;
        }
        else if (pSrc3 != nullptr)
        {
            srcLength = 3;
            channelMask = CHANNEL_MASK_RGB;
        }
        else if (pSrc2 != nullptr)
        {
            srcLength = 2;
            channelMask = CHANNEL_MASK_RG;
        }

        uint parameterLength = (srcLength + 2);

        CVariable* pPayload = m_currShader->GetNewVariable(
            parameterLength * numLanes(m_currShader->m_SIMDSize),
            ISA_TYPE_D,
            IGC::EALIGN_GRF);

        uint writeIndex = 0;

        if (m_currShader->m_SIMDSize == SIMDMode::SIMD8)
        {
            m_currShader->CopyVariable(pPayload, pArrIdx, writeIndex + 0);
            m_currShader->CopyVariable(pPayload, pByteOffset, writeIndex + 1);
            m_currShader->CopyVariable(pPayload, pSrc1, writeIndex + 2);

            if (pSrc2 != nullptr)
            {
                m_currShader->CopyVariable(pPayload, pSrc2, writeIndex + 3);
            }

            if (pSrc3 != nullptr)
            {
                m_currShader->CopyVariable(pPayload, pSrc3, writeIndex + 4);
            }

            if (pSrc4 != nullptr)
            {
                m_currShader->CopyVariable(pPayload, pSrc4, writeIndex + 5);
            }
        }
        else
        {
            TODO("Refactor this to use CreatePayload");
            m_currShader->CopyVariable(pPayload, pArrIdx, writeIndex + 0);
            m_currShader->CopyVariable(pPayload, pByteOffset, writeIndex + 2);
            m_currShader->CopyVariable(pPayload, pSrc1, writeIndex + 4);

            if (pSrc2 != nullptr)
            {
                m_currShader->CopyVariable(pPayload, pSrc2, writeIndex + 6);
            }

            if (pSrc3 != nullptr)
            {
                m_currShader->CopyVariable(pPayload, pSrc3, writeIndex + 8);
            }

            if (pSrc4 != nullptr)
            {
                m_currShader->CopyVariable(pPayload, pSrc4, writeIndex + 10);
            }
        }

        uint responseLength = 0;

        uint messageSpecificControl = encodeMessageSpecificControlForReadWrite(
            EU_DATA_PORT_WRITE_MESSAGE_TYPE_UNTYPED_SURFACE_WRITE,
            channelMask,
            m_currShader->m_SIMDSize);

        uint desc = DataPortWrite(
            parameterLength * (numLanes(m_currShader->m_SIMDSize) / numLanes(SIMDMode::SIMD8)),
            responseLength,
            false,
            false,
            EU_DATA_PORT_WRITE_MESSAGE_TYPE_UNTYPED_SURFACE_WRITE,
            messageSpecificControl,
            false,
            btiIndex);

        uint exDesc = EU_GEN7_5_MESSAGE_TARGET_DATA_PORT_DATA_CACHE_1;

        CVariable* pMessDesc = m_currShader->ImmToVariable(desc, ISA_TYPE_D);

        setPredicateForDiscard();
        m_encoder->Send(nullptr, pPayload, exDesc, pMessDesc);
        m_encoder->Push();
    }
    ResetVMask(false);
    m_currShader->isMessageTargetDataCacheDataPort = true;
}

void EmitPass::emitTypedRead(llvm::Instruction* pInsn)
{
    uint writeMask = m_currShader->GetExtractMask(pInsn);
    assert(writeMask != 0 && "Wrong write mask");

    llvm::Value* pllSrcBuffer  = pInsn->getOperand(0);
    llvm::Value* pllU    = pInsn->getOperand(1);
    llvm::Value* pllV    = pInsn->getOperand(2);
    llvm::Value* pllR    = pInsn->getOperand(3);
    llvm::Value* pllLOD  = pInsn->getOperand(4);

    CVariable* pLOD = isUndefOrConstInt0(pllLOD) ? nullptr : GetSymbol(pllLOD);
    CVariable* pR = (pLOD == nullptr && isUndefOrConstInt0(pllR)) ? nullptr : GetSymbol(pllR);
    CVariable* pV = (pR == nullptr && isUndefOrConstInt0(pllV)) ? nullptr : GetSymbol(pllV);
    CVariable* pU   = GetSymbol(pllU);

    pU   = BroadcastIfUniform(pU);
    pV   = pV ? BroadcastIfUniform(pV) : nullptr;
    pR = pR ? BroadcastIfUniform(pR) : nullptr;
    pLOD = pLOD ? BroadcastIfUniform(pLOD) : nullptr;

    ResourceDescriptor resource = GetResourceVariable(pllSrcBuffer);

    uint numChannels = iSTD::BitCount(writeMask);

    if (m_currShader->GetIsUniform(pInsn))
    {
        assert(0 && "Uniform ld_uav_typed not implemented yet");
    }
    else
    {
        uint label = 0;
        CVariable* flag = nullptr;
        bool needLoop = ResourceLoopHeader(resource, flag, label);
        CVariable* tempdst[4] = { nullptr, nullptr, nullptr, nullptr };
        bool needsSplit = m_currShader->m_SIMDSize == SIMDMode::SIMD16
            && !m_currShader->m_Platform->supportsSIMD16TypedRW();
        if (!needsSplit)
        {
            m_encoder->SetPredicate(flag);
            m_encoder->TypedRead4(resource, pU, pV, pR, pLOD, m_destination, writeMask);
            m_encoder->Push();
        }
        else
        {
            uint splitInstCount = numLanes(m_currShader->m_SIMDSize) / numLanes(SIMDMode::SIMD8);
            for (uint i = 0; i < splitInstCount; ++i)
            {
                tempdst[i] = m_currShader->GetNewVariable(numChannels * numLanes(SIMDMode::SIMD8),
                    ISA_TYPE_F,
                    EALIGN_GRF);

                m_encoder->SetSimdSize(SIMDMode::SIMD8);
                m_encoder->SetMask((i == 0) ? EMASK_Q1 : EMASK_Q2);
                m_encoder->SetSrcSubVar(0, i);
                m_encoder->SetSrcSubVar(1, i);
                m_encoder->SetSrcSubVar(2, i);
                m_encoder->SetPredicate(flag);
                m_encoder->TypedRead4(resource, pU, pV, pR, pLOD, tempdst[i], writeMask);
                m_encoder->Push();
            }
        }
        ResourceLoop(needLoop, flag, label);
        if(m_currShader->m_SIMDSize != SIMDMode::SIMD8)
        {
            JoinSIMD(tempdst, numChannels);
        }
    }
    m_currShader->isMessageTargetDataCacheDataPort = true;
}

void EmitPass::emitTypedWrite(llvm::Instruction* pInsn)
{
    ForceDMask();
    llvm::Value* pllDstBuffer  = pInsn->getOperand(0);
    llvm::Value* pllU          = pInsn->getOperand(1);
    llvm::Value* pllV          = pInsn->getOperand(2);
    llvm::Value* pllR          = pInsn->getOperand(3);
    llvm::Value* pllLOD        = pInsn->getOperand(4);
    llvm::Value* pllSrc_X      = pInsn->getOperand(5);
    llvm::Value* pllSrc_Y      = pInsn->getOperand(6);
    llvm::Value* pllSrc_Z      = pInsn->getOperand(7);
    llvm::Value* pllSrc_W      = pInsn->getOperand(8);

    CVariable* pLOD   = isUndefOrConstInt0(pllLOD) ? nullptr : GetSymbol(pllLOD);
    CVariable* pR     = (pLOD == nullptr && isUndefOrConstInt0(pllR)) ? nullptr : GetSymbol(pllR);
    CVariable* pV     = (pR == nullptr && isUndefOrConstInt0(pllV)) ? nullptr : GetSymbol(pllV);
    CVariable* pU     = GetSymbol(pllU);

    CVariable* pSrc_X = GetSymbol(pllSrc_X);
    CVariable* pSrc_Y = GetSymbol(pllSrc_Y);
    CVariable* pSrc_Z = GetSymbol(pllSrc_Z);
    CVariable* pSrc_W = GetSymbol(pllSrc_W);

    pU     = BroadcastIfUniform(pU);
    pV     = pV ? BroadcastIfUniform(pV) : nullptr;
    pR     = pR ? BroadcastIfUniform(pR) : nullptr;
    pLOD   = pLOD ? BroadcastIfUniform(pLOD) : nullptr;

    ResourceDescriptor resource = GetResourceVariable(pllDstBuffer);

    if (m_currShader->GetIsUniform(pInsn))
    {
        assert(0 && "Uniform store_uav_typed not implemented yet");
    }
    else
    {
        uint label = 0;
        CVariable* flag = nullptr;
        bool needLoop = ResourceLoopHeader(resource, flag, label);
        uint parameterLength = 4;

        bool needsSplit = m_currShader->m_SIMDSize == SIMDMode::SIMD16
            && !m_currShader->m_Platform->supportsSIMD16TypedRW();
        if (!needsSplit)
        {
            CVariable* pPayload = m_currShader->GetNewVariable(
                parameterLength * numLanes(m_currShader->m_SIMDSize),
                ISA_TYPE_F,
                IGC::EALIGN_GRF);
            // pSrcX, Y, Z & W are broadcast to uniform by this function itself.
            m_currShader->CopyVariable(pPayload, pSrc_X,   0);
            m_currShader->CopyVariable(pPayload, pSrc_Y,   1);
            m_currShader->CopyVariable(pPayload, pSrc_Z,   2);
            m_currShader->CopyVariable(pPayload, pSrc_W,   3);
            m_encoder->SetPredicate(flag);
            m_encoder->TypedWrite4(resource, pU, pV, pR, pLOD, pPayload);
            m_encoder->Push();
        }
        else 
        {
            for (uint i = 0; i < 2; ++i)
            {
                CVariable* pPayload = m_currShader->GetNewVariable(
                    parameterLength * numLanes(m_currShader->m_SIMDSize),
                    ISA_TYPE_F,
                    IGC::EALIGN_GRF);
                m_encoder->SetSimdSize(SIMDMode::SIMD8);
                m_encoder->SetMask((i == 0) ? EMASK_Q1 : EMASK_Q2);
                if (!pSrc_X->IsUniform())
                {
                    m_encoder->SetSrcSubVar(0, i);
                }
                m_encoder->SetDstSubVar(0);
                m_encoder->Copy(pPayload, pSrc_X);
                m_encoder->Push();

                m_encoder->SetSimdSize(SIMDMode::SIMD8);
                m_encoder->SetMask((i == 0) ? EMASK_Q1 : EMASK_Q2);
                if (!pSrc_Y->IsUniform())
                {
                    m_encoder->SetSrcSubVar(0, i);
                }
                m_encoder->SetDstSubVar(1);
                m_encoder->Copy(pPayload, pSrc_Y);
                m_encoder->Push();

                m_encoder->SetSimdSize(SIMDMode::SIMD8);
                m_encoder->SetMask((i == 0) ? EMASK_Q1 : EMASK_Q2);
                if (!pSrc_Z->IsUniform())
                {
                    m_encoder->SetSrcSubVar(0, i);
                }
                m_encoder->SetDstSubVar(2);
                m_encoder->Copy(pPayload, pSrc_Z);
                m_encoder->Push();

                m_encoder->SetSimdSize(SIMDMode::SIMD8);
                m_encoder->SetMask((i == 0) ? EMASK_Q1 : EMASK_Q2);
                if (!pSrc_W->IsUniform())
                {
                    m_encoder->SetSrcSubVar(0, i);
                }
                m_encoder->SetDstSubVar(3);
                m_encoder->Copy(pPayload, pSrc_W);
                m_encoder->Push();

                m_encoder->SetSimdSize(SIMDMode::SIMD8);
                m_encoder->SetMask((i == 0) ? EMASK_Q1 : EMASK_Q2);
                m_encoder->SetSrcSubVar(0, i);
                m_encoder->SetSrcSubVar(1, i);
                m_encoder->SetSrcSubVar(2, i);
                m_encoder->SetSrcSubVar(3, i);
                m_encoder->SetPredicate(flag);
                m_encoder->TypedWrite4(resource, pU, pV, pR, pLOD, pPayload);
                m_encoder->Push();
            }
        }
        ResourceLoop(needLoop, flag, label);
    }
    ResetVMask();
    m_currShader->isMessageTargetDataCacheDataPort = true;
}

void EmitPass::emitThreadGroupBarrier(llvm::Instruction* inst)
{
    if (m_currShader->GetShaderType() == ShaderType::HULL_SHADER)
    {
        // set barrier counter bits in R0.2 (for use by VISA barrier instruction)

        CHullShader* hsProgram = static_cast<CHullShader*>(m_currShader);
        int instanceCount = hsProgram->DetermineInstanceCount();
        // This sets the barrier message counter bits which is needed for HS
        unsigned int counterBits = m_currShader->m_Platform->getBarrierCountBits(instanceCount);
        CVariable* tmpVar = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_GRF, true);

        if (m_currShader->m_Platform->needsHSBarrierIDWorkaround())
        {
            // move barrier id into bits 27:24 of R0.2 in the payload to match with GPGPU payload for barrier id
            // VISA assumes barrier id is found in bits 27:24 as in GPGPU payload and to avoid any IGC/VISA change
			// this is a simple WA which needs to be applied 

            CVariable* masklower24bit = m_currShader->ImmToVariable(0xf000000, ISA_TYPE_UD);
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->SetSrcSubReg(0, 2);
            m_encoder->Shl(tmpVar, hsProgram->GetR0(), m_currShader->ImmToVariable(11, ISA_TYPE_UD));
            m_encoder->Push();
            m_encoder->And(tmpVar, tmpVar, masklower24bit);
            m_encoder->Push();
            m_encoder->Or(tmpVar, tmpVar, m_currShader->ImmToVariable(counterBits, ISA_TYPE_UD));
            m_encoder->Push();
        }
        else
        {
			// If barrier - id bits match GPGPU payload
			m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->SetSrcSubReg(0, 2);
            m_encoder->Or(tmpVar, hsProgram->GetR0(), m_currShader->ImmToVariable(counterBits, ISA_TYPE_UD));
            m_encoder->Push();
        }

        m_encoder->SetDstSubReg(2);
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
        m_encoder->Copy(hsProgram->GetR0(), tmpVar);
        m_encoder->Push();
    }

    // OPT: Remove barrier instruction when thread group size is less or equal than simd size.
    bool skipBarrierInstructionInCS = false;
    if (m_currShader->GetShaderType() == ShaderType::COMPUTE_SHADER)
    {
        unsigned int threadGroupSizeCS = (static_cast<CComputeShader*>(m_currShader))->GetThreadGroupSize();
        if (threadGroupSizeCS <= numLanes(m_SimdMode))
        {
            skipBarrierInstructionInCS = true;
        }
    }

    if (!skipBarrierInstructionInCS)
    {
        m_currShader->SetHasBarrier();
        m_encoder->Barrier();
        m_encoder->Push();
    }
}

void EmitPass::emitMemoryFence(llvm::Instruction* inst)
{
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    bool CommitEnable = llvm::cast<llvm::ConstantInt>((inst->getOperand(0)))->getValue().getBoolValue();
    bool L3_Flush_RW_Data = llvm::cast<llvm::ConstantInt>((inst->getOperand(1)))->getValue().getBoolValue();
    bool L3_Flush_Constant_Data = llvm::cast<llvm::ConstantInt>((inst->getOperand(2)))->getValue().getBoolValue();
    bool L3_Flush_Texture_Data = llvm::cast<llvm::ConstantInt>((inst->getOperand(3)))->getValue().getBoolValue();
    bool L3_Flush_Instructions = llvm::cast<llvm::ConstantInt>((inst->getOperand(4)))->getValue().getBoolValue();
    bool Global_Mem_Fence = true;
    bool L1_Invalidate = llvm::cast<llvm::ConstantInt>((inst->getOperand(6)))->getValue().getBoolValue() && ctx->platform.hasL1ReadOnlyCache();

    bool EmitFence = true;
    // If passed a non-constant parameter, be conservative and emit a fence.
    // We really don't want to add control-flow at this point.
    if (ConstantInt* globalConst = llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(5)))
    {
        Global_Mem_Fence = globalConst->getValue().getBoolValue();
        if ( globalConst->isZero() )
        {
            // Check whether we know this is a local fence. If we do, don't emit fence for a BDW+SKL/BXT only.
            // case CLK_LOCAL_MEM_FENCE:
            
            if ( ctx->platform.localMemFenceSupress() )
            {
                EmitFence = false;
            }
        }
    }

    if (EmitFence)
    {
        m_encoder->Fence(CommitEnable,
            L3_Flush_RW_Data,
            L3_Flush_Constant_Data,
            L3_Flush_Texture_Data,
            L3_Flush_Instructions,
            Global_Mem_Fence,
            L1_Invalidate);
        m_encoder->Push();
    }
}

void EmitPass::emitMemoryFence()
{
    m_encoder->Fence(true,
        false,
        false,
        false,
        false,
        true,
        false);
    m_encoder->Push();
}

void EmitPass::emitFlushSamplerCache(llvm::Instruction* inst)
{
    m_encoder->FlushSamplerCache();
    m_encoder->Push();
}

void EmitPass::emitWorkGroupAny(llvm::GenIntrinsicInst* inst)
{
    assert(m_currShader->GetShaderType() == ShaderType::OPENCL_SHADER);
    m_currShader->SetHasBarrier();

    assert(m_currShader->m_Platform->hasPredicatedBarriers());

    CVariable* predicateSource = GetSymbol(inst->getOperand(0));

    CVariable* predicateMask = m_currShader->GetNewVariable(
            numLanes(m_SimdMode),
            ISA_TYPE_BOOL,
            IGC::EALIGN_GRF);

    CVariable* predicateMaskVector = m_currShader->GetNewVariable(
            1,
            ISA_TYPE_UD,
            IGC::EALIGN_GRF,
            true);

    CVariable* zero = m_currShader->ImmToVariable(0, ISA_TYPE_UD);
    CVariable* mask16bit = m_currShader->ImmToVariable(0xffff, ISA_TYPE_UD);

    m_encoder->Cmp(EPREDICATE_NE, predicateMask, predicateSource, zero);
    m_encoder->Push();

    m_encoder->BoolToInt(predicateMaskVector, predicateMask);
    m_encoder->Push();

    m_encoder->PredicatedBarrier(m_destination, predicateMaskVector);
    m_encoder->Push();

    m_encoder->SetSrcRegion(0, 0, 1, 0);
    m_encoder->And(m_destination, m_destination, mask16bit);
    m_encoder->Push();
}

void EmitPass::emitPhaseOutput(llvm::GenIntrinsicInst* inst)
{
    assert(m_currShader->GetShaderType() == ShaderType::PIXEL_SHADER);
    CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);
    assert(psProgram->GetPhase() == PSPHASE_COARSE);

    unsigned int outputIndex = (unsigned int)cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue();
    CVariable* output = GetSymbol(inst->getOperand(0));
    CVariable* temp =
          m_currShader->GetNewVariable(numLanes(m_SimdMode), output->GetType(), EALIGN_GRF);
    m_encoder->Copy(temp, output);
    output = temp;

    psProgram->AddCoarseOutput(output, outputIndex);
}

void EmitPass::emitPhaseInput(llvm::GenIntrinsicInst* inst)
{
    assert(m_currShader->GetShaderType() == ShaderType::PIXEL_SHADER);
    CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);
    assert(psProgram->GetPhase() == PSPHASE_PIXEL);

    unsigned int inputIndex = (unsigned int)cast<llvm::ConstantInt>(inst->getOperand(0))->getZExtValue();
    CVariable* input = psProgram->GetCoarseInput(inputIndex);

    // address variable represents register a0
    CVariable* pDstArrElm = m_currShader->GetNewAddressVariable(
        numLanes(m_currShader->m_SIMDSize),
        input->GetType(),
        false,
        true);

    // we add offsets to the base that is the beginning of the vector variable
    CVariable* index = psProgram->GetCoarseParentIndex();
    CVariable* byteAddress = psProgram->GetNewVariable(numLanes(m_SimdMode), ISA_TYPE_UW, EALIGN_OWORD);
    m_encoder->Shl(byteAddress, index, psProgram->ImmToVariable(2, ISA_TYPE_UW));
    m_encoder->Push();

     m_encoder->AddrAdd(pDstArrElm, input, byteAddress);
     m_encoder->Push();

     m_encoder->Copy(m_destination, pDstArrElm);
     m_encoder->Push();
}

void EmitPass::emitAtomicCounter(llvm::GenIntrinsicInst* pInsn)
{
    ForceDMask();
    assert(pInsn->getNumOperands() == 2);
    GenISAIntrinsic::ID IID = pInsn->getIntrinsicID();
    /// Immediate Atomics return the value before the atomic operation is performed. So that flag
    /// needs to be set for this.
    bool returnsImmValue = !pInsn->user_empty();

    llvm::Value* pllbuffer     = pInsn->getOperand(0);
    ResourceDescriptor resource = GetResourceVariable(pllbuffer);

    EU_DATA_PORT_ATOMIC_OPERATION_TYPE atomicType;
    CVariable* pPayload = nullptr;
    CVariable* prefixVar[2] = { nullptr, nullptr };
    CVariable* dst = m_destination;
    bool uniformAtomic = IsUniformAtomic(pInsn) && 
        (m_currShader->m_SIMDSize != SIMDMode::SIMD8 || !m_currShader->m_Platform->HDCCoalesceAtomicCounterAccess());
    bool hasheader = false;
    unsigned int  num_split = 1;
    if(!uniformAtomic)
    {
        // header
         pPayload = m_currShader->GetNewVariable(
            numLanes(SIMDMode::SIMD8),
            ISA_TYPE_UD,
            IGC::EALIGN_GRF);

        m_encoder->SetNoMask();
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetDstSubReg(7);
        m_encoder->Copy(pPayload, m_currShader->ImmToVariable(0xFFFF, ISA_TYPE_UD));
        m_encoder->Push();
        hasheader = true;
        atomicType = EU_DATA_PORT_ATOMIC_OPERATION_INC;
        num_split = m_currShader->m_SIMDSize == SIMDMode::SIMD16 ? 2 : 1;
        if(IID == GenISAIntrinsic::GenISA_atomiccounterpredec)
        {
            atomicType = EU_DATA_PORT_ATOMIC_OPERATION_PREDEC;
        }
    }
    else
    {
        atomicType = EU_DATA_PORT_ATOMIC_OPERATION_ADD;
        // for SIMD dispatch greater than 8 it is more efficient to emit a SIMD1 atomic
        CVariable* src = m_currShader->ImmToVariable(
            IID == GenISAIntrinsic::GenISA_atomiccounterpredec ? -1 : 1, ISA_TYPE_D);
        emitPreOrPostFixOp(EOPCODE_ADD, 0, ISA_TYPE_D, false, src, prefixVar);
        CVariable *pSrcCopy = prefixVar[0];
        if(m_currShader->m_dispatchSize == SIMDMode::SIMD32)
            pSrcCopy = prefixVar[1];

        pPayload = m_currShader->GetNewVariable(
            8,
            ISA_TYPE_D,
            IGC::EALIGN_GRF,
            true);

        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        m_encoder->SetSrcSubReg(0, numLanes(m_currShader->m_SIMDSize) - 1);
        m_encoder->Copy(pPayload, pSrcCopy);
        m_encoder->Push();
        dst =  m_currShader->GetNewVariable(
            8,
            ISA_TYPE_D,
            IGC::EALIGN_GRF,
            true);
    }
    uint label = 0;
    CVariable* flag = nullptr;
    bool needLoop = ResourceLoopHeader(resource, flag, label);

    uint messageDescriptor = encodeMessageDescriptorForAtomicUnaryOp(
        1,
        returnsImmValue ? 1 : 0,
        hasheader,
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_ATOMIC_COUNTER_OPERATION,
        returnsImmValue,
        SIMDMode::SIMD8,
        atomicType,
        resource.m_surfaceType == ESURFACE_BINDLESS ? BINDLESS_BTI : (uint)resource.m_resource->GetImmediateValue());

    CVariable* pMessDesc = m_currShader->ImmToVariable(messageDescriptor, ISA_TYPE_D);
    CVariable* exDesc =
        m_currShader->ImmToVariable(EU_GEN7_5_MESSAGE_TARGET_DATA_PORT_DATA_CACHE_1, ISA_TYPE_D);

    if(resource.m_surfaceType == ESURFACE_BINDLESS)
    {
        CVariable* temp = m_currShader->GetNewVariable(resource.m_resource);
        m_encoder->Add(temp, resource.m_resource, exDesc);
        m_encoder->Push();

        exDesc = temp;
    }

    for(uint32_t i = 0; i < num_split; ++i)
    {
        if(uniformAtomic)
        {
            m_encoder->SetSimdSize(SIMDMode::SIMD1);
            m_encoder->SetNoMask();
        }
        else
        {
            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            m_encoder->SetDstSubVar(i);
            m_encoder->SetMask((i == 0) ? EMASK_Q1 : EMASK_Q2);
        }

        m_encoder->Send(
            returnsImmValue ? dst : NULL,
            pPayload,
            EU_GEN7_5_MESSAGE_TARGET_DATA_PORT_DATA_CACHE_1,
            exDesc,
            pMessDesc);
        m_encoder->Push();
    }
    
    if(returnsImmValue && prefixVar[0] != nullptr)
    {
        unsigned int counter = m_currShader->m_dispatchSize == SIMDMode::SIMD32 ? 2 : 1;
        for(unsigned int i = 0; i < counter; ++i)
        {
            m_encoder->SetSecondHalf(i == 1);
            m_encoder->Add(m_destination, prefixVar[i], dst);
            m_encoder->Push();
            
            if(IID != GenISAIntrinsic::GenISA_atomiccounterpredec)
            {
                CVariable* src = m_currShader->ImmToVariable(-1, ISA_TYPE_D);
                m_encoder->Add(m_destination, m_destination, src);
                m_encoder->Push();
            }
        }
    }

    ResourceLoop(needLoop, flag, label);
    ResetVMask();
    m_currShader->isMessageTargetDataCacheDataPort = true;
}

void EmitPass::CmpBoolOp(llvm::BinaryOperator* inst,
                         llvm::CmpInst::Predicate predicate,
                         const SSource cmpSources[2],
                         const SSource& bitSource,
                         const DstModifier&  modifier)
{
    CVariable* tempBool = m_currShader->GetNewVector(inst);
    CVariable* dst = m_destination;
    m_destination = tempBool;
    DstModifier init;
    Cmp(predicate, cmpSources, init);

    assert(bitSource.mod == EMOD_NONE);
    CVariable* boolOpSource = GetSrcVariable(bitSource);
    m_encoder->SetDstModifier(modifier);

    EmitSimpleAlu(inst, dst, tempBool, boolOpSource);
    m_destination = dst;
}

void EmitPass::emitAluConditionMod(Pattern* aluPattern, llvm::Instruction* alu, llvm::CmpInst* cmp)
{
    CVariable* temp = m_currShader->GetNewVector(alu);
    CVariable* dst = m_destination;
    m_destination = temp;
    DstModifier init;

    aluPattern->Emit(this, init);

    e_predicate predicate = GetPredicate(cmp->getPredicate());
    if(IsUnsignedCmp(cmp->getPredicate()))
    {
        temp = m_currShader->BitCast(temp, GetUnsignedType(temp->GetType()));
    }
    m_encoder->Cmp(predicate,dst, temp, m_currShader->ImmToVariable(0, temp->GetType()));
    m_encoder->Push();
    m_destination = dst;
}

void EmitPass::emitHSPatchConstantInput(llvm::Instruction* pInst)
{
    assert(m_currShader->GetShaderType()==ShaderType::HULL_SHADER);
    CHullShader* hsProgram = static_cast<CHullShader*>(m_currShader);
    hsProgram->EmitPatchConstantInput(pInst, m_destination);
}

void EmitPass::emitHSOutputControlPtInput(llvm::Instruction* pInst)
{
    assert(m_currShader->GetShaderType()==ShaderType::HULL_SHADER);
    CHullShader* hsProgram = static_cast<CHullShader*>(m_currShader);
    hsProgram->EmitOutputControlPointInput(pInst, m_destination);
}

void EmitPass::emitHSTessFactors(llvm::Instruction* pInst)
{
    assert(m_currShader->GetShaderType()==ShaderType::HULL_SHADER);
    CHullShader* hsProgram = static_cast<CHullShader*>(m_currShader);
    CVariable* payload[8];

    for(uint32_t channel = 2 ;channel < 8; channel++)
    {
        payload[channel] = GetSymbol(pInst->getOperand(channel-2));
    }

    bool endOfThread = llvm::isa<llvm::ReturnInst>(pInst->getNextNode());
    hsProgram->EmitPatchConstantHeader(payload, endOfThread );
}

void EmitPass::emitRenderTargetRead(llvm::GenIntrinsicInst* inst)
{
    assert(m_currShader->GetShaderType() == ShaderType::PIXEL_SHADER);
    CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);
    uint RTIndex = 0;
    bool isRTIndexConstant = false;
    if (llvm::ConstantInt* pRenderTargetCnst = llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(0)))
    {
        RTIndex = (uint)llvm::cast<llvm::ConstantInt>(pRenderTargetCnst)->getZExtValue();
        isRTIndexConstant = true;
    }

    uint bindingTableIndex = m_currShader->m_pBtiLayout->GetRenderTargetIndex(RTIndex);
    m_currShader->SetBindingTableEntryCountAndBitmap(isRTIndexConstant, bindingTableIndex);
    CVariable* pSampleIndexR0 = nullptr;

    uint hasSampleIndex = (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_RenderTargetReadSampleFreq);
    if (hasSampleIndex)
    {
        CVariable* pShiftedSampleIndex = nullptr;
        if (llvm::isa<llvm::ConstantInt>(inst->getOperand(1)))
        {
            uint sampleIndex = int_cast<uint>(cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue());
            pShiftedSampleIndex = m_currShader->ImmToVariable((sampleIndex << 6), ISA_TYPE_D);
        }
        else
        {
            CVariable* SampleIndex = GetSymbol(inst->getOperand(1));
            if(!SampleIndex->IsUniform())
            {
                SampleIndex = UniformCopy(SampleIndex);
            }
            pShiftedSampleIndex = m_currShader->GetNewVariable(SampleIndex);
            m_encoder->Shl(pShiftedSampleIndex, SampleIndex, m_currShader->ImmToVariable(6, ISA_TYPE_D));
            m_encoder->Push();
        }

        // and       (1) r15.0<1>:ud  r0.0<0;1,0>:ud 0xFFFFFC3F:ud
        // or       (1) r16.0<1>:ud  r15.0<0;1,0>:ud  r14.0<0;1,0>:ud
        pSampleIndexR0 = m_currShader->GetNewVariable(numLanes(m_SimdMode), ISA_TYPE_D, EALIGN_GRF);
        m_encoder->SetSimdSize(SIMDMode::SIMD8);
        m_encoder->SetNoMask();
        m_encoder->Copy(pSampleIndexR0, psProgram->GetR0());
        m_encoder->Push();

        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        m_encoder->SetSrcSubReg(0, 0);
        m_encoder->SetNoMask();
        m_encoder->And(pSampleIndexR0, pSampleIndexR0, m_currShader->ImmToVariable(0xFFFFFC3F, ISA_TYPE_UD));
        m_encoder->Push();

        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        m_encoder->SetSrcSubReg(0, 0);
        m_encoder->SetSrcRegion(1, 0, 1, 0);
        m_encoder->SetSrcSubReg(1, 0);
        m_encoder->SetNoMask();
        m_encoder->Or(pSampleIndexR0, pSampleIndexR0, pShiftedSampleIndex);
        m_encoder->Push();
    }

    // RT read header is 2 GRF
    uint messageLength = 2;
    uint responseLength = 4 * numLanes(m_currShader->m_SIMDSize) / 8;
    bool headerRequired = true;

    // We shouldn't need any copies since R0 and R1 are already aligned
    // but we don't want to declare R0 and R1 as one variable in V-ISA
    // The problem could be fixed by moving away from raw_send for this message
    CVariable* payload =
        m_currShader->GetNewVariable(messageLength*(SIZE_GRF>>2), ISA_TYPE_D, EALIGN_GRF);
    m_encoder->SetNoMask();
    m_encoder->SetSimdSize(SIMDMode::SIMD8);
    m_encoder->Copy(payload, (hasSampleIndex ? pSampleIndexR0 : psProgram->GetR0()));
    m_encoder->Push();

    // The following bits must be set to 0 for render target read messages:
    //  Bit 11 - Source0 Alpha Present to Render Target
    //  Bit 12 - oMask to Render Target
    //  Bit 13 - Source Depth Present to Render Target
    //  Bit 14 - Stencil Present to Render Target
    m_encoder->SetSimdSize(SIMDMode::SIMD1);
    m_encoder->SetSrcRegion(0, 0, 1, 0);
    m_encoder->SetSrcSubReg(0, 0);
    m_encoder->SetNoMask();
    m_encoder->And(payload, payload, m_currShader->ImmToVariable(0xFFFF87FF, ISA_TYPE_UD));
    m_encoder->Push();

    m_encoder->SetNoMask();
    m_encoder->SetSimdSize(SIMDMode::SIMD8);
    m_encoder->SetDstSubVar(1);
    m_encoder->Copy(payload, psProgram->GetR1());
    m_encoder->Push();

    uint msgControl =
        ( m_SimdMode == SIMDMode::SIMD8 )
        ? EU_GEN9_DATA_PORT_RENDER_TARGET_READ_CONTROL_SIMD8_SINGLE_SOURCE_LOW
        : EU_GEN9_DATA_PORT_RENDER_TARGET_READ_CONTROL_SIMD16_SINGLE_SOURCE;
    msgControl |=
        m_encoder->IsSecondHalf() ? EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_SLOTGRP_HI : EU_GEN6_DATA_PORT_RENDER_TARGET_WRITE_SLOTGRP_LO;
    uint Desc = DataPortRead(
        messageLength,
        responseLength,
        headerRequired,
        EU_DATA_PORT_READ_MESSAGE_TYPE_RENDER_TARGET_READ,
        msgControl,
        hasSampleIndex ? true : false,
        DATA_PORT_TARGET_RENDER_CACHE,
        bindingTableIndex);

    uint exDesc = EU_MESSAGE_TARGET_DATA_PORT_WRITE;

    CVariable* messDesc;
    if (isRTIndexConstant)
    {
        messDesc = psProgram->ImmToVariable(Desc, ISA_TYPE_UD);
    }
    else
    {
        messDesc = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true);
        // RTIndex is not a constant, so OR the value with desc to get the correct RTIndex
        m_encoder->Add(messDesc, GetSymbol(inst->getOperand(0)), psProgram->ImmToVariable(Desc, ISA_TYPE_UD));
        m_encoder->Push();
    }
    //sendc
    m_encoder->SendC(m_destination,payload,exDesc,messDesc);
    m_encoder->Push();
}

CEncoder::RoundingMode EmitPass::GetRoundingMode(llvm::GenIntrinsicInst* inst)
{
    CEncoder::RoundingMode RM = CEncoder::RoundToNearestEven;
    if (inst)
    {
        switch (inst->getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_f32tof16_rtz:
        case GenISAIntrinsic::GenISA_ftof_rtz:
            RM = CEncoder::RoundToZero;
            break;
        case GenISAIntrinsic::GenISA_itof_rtn:
        case GenISAIntrinsic::GenISA_uitof_rtn:
        case GenISAIntrinsic::GenISA_ftoui_rtn:
        case GenISAIntrinsic::GenISA_ftoi_rtn:
            RM = CEncoder::RoundToNegative_int;
            break;
        case GenISAIntrinsic::GenISA_itof_rtp:
        case GenISAIntrinsic::GenISA_uitof_rtp:
        case GenISAIntrinsic::GenISA_ftoui_rtp:
        case GenISAIntrinsic::GenISA_ftoi_rtp:
            RM = CEncoder::RoundToPositive_int;
            break;
        case GenISAIntrinsic::GenISA_itof_rtz:
        case GenISAIntrinsic::GenISA_uitof_rtz:
            RM = CEncoder::RoundToZero_int;
            break;
        case GenISAIntrinsic::GenISA_ftof_rtn:
            RM = CEncoder::RoundToNegative;
            break;
        case GenISAIntrinsic::GenISA_ftof_rtp:
            RM = CEncoder::RoundToPositive;
            break;
        case GenISAIntrinsic::GenISA_ftoui_rte:
        case GenISAIntrinsic::GenISA_ftoi_rte:
            RM = CEncoder::RoundToNearestEven_int;
            break;
        default:
            break;
        }
    }
    return RM;
}

bool EmitPass::ignoreRoundingMode(llvm::Instruction* inst) const
{
    if (isa<InsertElementInst>(inst) || isa<BitCastInst>(inst))
    {
        // these generate raw moves or no instructions at all
        return true;
    }
    // add more instr as needed
    return false;
}

void EmitPass::ResetRoundingMode(llvm::GenIntrinsicInst* inst)
{
    for (auto nextInst = GetNextInstruction(inst); nextInst != nullptr; nextInst = GetNextInstruction(nextInst))
    {
        if (llvm::GenIntrinsicInst* nextGenIntrinsicInst = dyn_cast<llvm::GenIntrinsicInst>(nextInst))
        {
            if (GetRoundingMode(nextGenIntrinsicInst) == m_roundingMode)
            {
                return;
            }
        }
        else if (ignoreRoundingMode(nextInst))
        {
            continue;
        }
        break;
    }

    m_encoder->SetCr0RneMode();
    m_roundingMode = CEncoder::RoundToNearestEven;
}

void EmitPass::emitf32tof16_rtz(llvm::GenIntrinsicInst* inst)
{
    CVariable* src = GetSymbol(inst->getOperand(0));
    CVariable imm0_hf(0, ISA_TYPE_HF);
    CVariable* dst_hf = m_currShader->BitCast(m_destination, ISA_TYPE_HF);

    if (m_roundingMode != GetRoundingMode(inst))
    {
        m_encoder->SetCr0FromRneModeTo(CEncoder::RoundToZero);
        m_roundingMode = CEncoder::RoundToZero;
    }

    m_encoder->SetDstRegion(2);
    m_encoder->Cast(dst_hf, src);
    m_encoder->Push();

    m_encoder->SetDstRegion(2);
    m_encoder->SetDstSubReg(1);
    m_encoder->Copy(dst_hf, &imm0_hf);
    m_encoder->Push();

    ResetRoundingMode(inst);
}

void EmitPass::emititof(llvm::GenIntrinsicInst* inst)
{
    CVariable* src = GetSymbol(inst->getOperand(0));
    CEncoder::RoundingMode RM = GetRoundingMode(inst);
    CVariable* dst = m_destination;

    GenISAIntrinsic::ID id = inst->getIntrinsicID();
    if (id == GenISAIntrinsic::GenISA_uitof_rtn ||
        id == GenISAIntrinsic::GenISA_uitof_rtp ||
        id == GenISAIntrinsic::GenISA_uitof_rtz)
    {
        src = m_currShader->BitCast(src, GetUnsignedType(src->GetType()));
    }

    if (RM != m_roundingMode)
    {
        m_encoder->SetCr0FromRneModeTo(RM);
        m_roundingMode = RM;
    }
    m_encoder->Cast(dst, src);
    m_encoder->Push();

    ResetRoundingMode(inst);
}

void EmitPass::emitfptrunc(llvm::GenIntrinsicInst* inst)
{
    assert(inst->getOperand(0)->getType()->isFloatingPointTy() && "Unsupported type");
    CVariable* src = GetSymbol(inst->getOperand(0));
    CVariable* dst = m_destination;
    CEncoder::RoundingMode RM = GetRoundingMode(inst);

    GenISAIntrinsic::ID id = inst->getIntrinsicID();
    if (id == GenISAIntrinsic::GenISA_ftoui_rtn ||
        id == GenISAIntrinsic::GenISA_ftoui_rtp ||
        id == GenISAIntrinsic::GenISA_ftoui_rte)
    {
        dst = m_currShader->BitCast(dst, GetUnsignedType(dst->GetType()));
    }

    if (RM != m_roundingMode)
    {
        m_encoder->SetCr0FromRneModeTo(RM);
        m_roundingMode = RM;
    }
    m_encoder->Cast(dst, src);
    m_encoder->Push();

    ResetRoundingMode(inst);
}

// Return true if this store will be emit as uniform store
bool EmitPass::isUniformStoreOCL(llvm::StoreInst* SI)
{
    if (!m_currShader->GetIsUniform(SI->getPointerOperand()))
    {
        return false;
    }

    Type *Ty = SI->getValueOperand()->getType();
    VectorType *VTy = dyn_cast<VectorType>(Ty);
    uint32_t elts = VTy ? int_cast<uint32_t>(VTy->getNumElements()) : 1;
    Type *eltTy = VTy ? VTy->getElementType() : Ty;

    // use TypeSize to be consistent with VectorLoad/Store
    uint32_t totalBytes = elts * ((uint32_t)m_DL->getTypeSizeInBits(eltTy) / 8);

    // Note that when elts > 1, VectorProcess make sure that its element
    // size must be 4 or 8. Also, note that if totalBytes = 4, elts must be 1.
    bool doUniformStore = (elts == 1 ||
        (m_currShader->GetIsUniform(SI->getValueOperand()) &&
         (totalBytes == 8 || totalBytes == 12 || totalBytes == 16)));
    return doUniformStore;
}

void EmitPass::emitVectorBitCast(llvm::BitCastInst* BCI)
{
    uint destMask = m_currShader->GetExtractMask(BCI);

    CVariable* src = GetSymbol( BCI->getOperand(0) );
    llvm::Type *srcTy = BCI->getOperand(0)->getType();
    llvm::Type *dstTy = BCI->getType();
    llvm::Type *srcEltTy, *dstEltTy;
    uint32_t srcNElts, dstNElts;

    assert( (srcTy->isVectorTy() || dstTy->isVectorTy()) &&
             "No vector type !");

    if( srcTy->isVectorTy() )
    {
        srcEltTy = srcTy->getVectorElementType();
        srcNElts  = srcTy->getVectorNumElements();
    }
    else
    {
        srcEltTy = srcTy;
        srcNElts  = 1;
    }
    if( dstTy->isVectorTy() )
    {
        dstEltTy = dstTy->getVectorElementType();
        dstNElts  = dstTy->getVectorNumElements();
    }
    else
    {
        dstEltTy = dstTy;
        dstNElts  = 1;
    }

    if( src->IsImmediate() )
    {
        CVariable *reg = m_currShader->GetNewVariable(
            1,
            src->GetType(),
            m_encoder->GetCISADataTypeAlignment( src->GetType() ),
            true,
            1 );

        m_encoder->Copy( reg, src );
        m_encoder->Push();

        src = reg;
    }

    uint32_t width = numLanes(m_currShader->m_SIMDSize);
    uint32_t dstEltBytes = int_cast<uint32_t>(dstEltTy->getPrimitiveSizeInBits() / 8);
    uint32_t srcEltBytes = int_cast<uint32_t>(srcEltTy->getPrimitiveSizeInBits() / 8);
    bool srcUniform = src->IsUniform();
    bool dstUniform = m_destination->IsUniform();
    if( srcEltBytes == dstEltBytes )
    {
        // This should not happen now, but generate code anyway.
        // CISABuilder does split if there is any spliting.

        // Special case for: 1 element vectors to scalars
        //    %15 = bitcast <1 x i64> %4 to i64
        if( ( srcEltTy == dstEltTy) &&
            ( srcNElts == dstNElts ) && (srcNElts == 1) )
        {
            m_encoder->Copy(m_destination, src);
            m_encoder->Push();
        }
        else if( m_destination != src )
        {
            for( uint32_t i = 0, offset = 0; i < dstNElts; ++i )
            {
                if(BIT(i) & destMask)
                {
                    m_encoder->SetSrcRegion(0,
                        srcUniform ? 0 : 1,
                        srcUniform ? 1 : 1,
                        srcUniform ? 0 : 0);
                    m_encoder->SetSrcSubReg(0, srcUniform ? i : i*width);
                    m_encoder->SetDstRegion(1);
                    m_encoder->SetDstSubReg(dstUniform ? offset : offset*width);
                    m_encoder->Copy(m_destination, src);
                    m_encoder->Push();
                    offset++;
                }
            }
        }
    }
    else if( dstEltBytes > srcEltBytes )
    {
        CVariable *aliasDst =  m_currShader->GetNewAlias(m_destination, src->GetType(), 0, 0);
        uint32_t N = dstEltBytes / srcEltBytes;
        assert( (dstEltBytes % srcEltBytes) == 0 && "Basic types should be power of 2");
        // Since srcEltBytes can be the second largest element type (32bit)
        // and region hstride == 1, Src will not need splitting!
        // Only dst might need spliting.
        bool splitDst = (!dstUniform && (dstEltBytes * width > 64U));
        assert( (!splitDst || (width == 16)) &&
                "Internal Error: Dst needs splitting only under SIMD16!");
        if( N > 4 )
        {
            // Special case for N = 8 as dst's stride can be 1/2/4, not 8.
            //   for example, <1xi64> Y = bitcast <8xi8> X
            // we will do the following (simd8)
            //   .decl X  type=q num_elts=8
            //   .decl Y  type=b num_elts=64
            //   .decl Y_alias type=d num_elts=16 alias=<Y,0>
            //   .decl V0  type=d num_elts=8
            //   .decl V1  type=d num_elts=8
            //   .decl V0_alias type=b num_elts=32 alias=<V0, 0>
            //   .decl V1_alias type=b num_elts=32 alias=<V1, 0>
            //
            //   mov (8) V0_alias.0<4> X(0,0)<8;8:1>
            //   mov (8) V0_alias.1<4> X(0,8)<8;8:1>
            //   mov (8) V0_alias.2<4> X(0,16)<8;8:1>
            //   mov (8) V0_alias.3<4> X(0,24)<8;8:1>
            //   mov (8) V1_alias.0<4> X(1,0)<8;8:1>
            //   mov (8) V1_alias.1<4> X(1,8)<8;8:1>
            //   mov (8) V1_alias.2<4> X(1,16)<8;8:1>
            //   mov (8) V1_alias.3<4> X(1,24)<8;8:1>
            //
            // then, combine V0 and V1 to create Y
            //   mov (8) Y_alias.0<2> V0(0,0)<8;8,1>
            //   mov (8) Y_alias.1<2> V1(0,0)<8;8,1>
            //
            // For SIMD16, the above two movs will span across two GRFs for their
            // dst operands, therefore, they need splitting, that is
            //   mov (16) Y_alias.0<2> V0(0,0)<16;16,1>
            //   mov (16) Y_alias.1<2> V1(0,0)<16;16,1>
            // should be splitted into the following:
            //   mov (8, Q1) Y_alias.0<2>   V0(0,0)<8;8,1>
            //   mov (8, Q2) Y_alias.16<2>  V0(1,0)<8;8,1>
            //   mov (8, Q1) Y_alias.1<2>   V1(0,0)<8;8,1>
            //   mov (8, Q2) Y_alias.17<2>  V1(1,0)<8;8,1>
            //
            assert ((N == 8) && (srcEltBytes == 1) && "Unexpected N value");
            const uint32_t N2 = N / 2; // 4
            VISA_Type TyD = (src->GetType() == ISA_TYPE_UB) ? ISA_TYPE_UD : ISA_TYPE_D;
            CVariable *V0 = m_currShader->GetNewVariable(dstUniform ? 1 : width, TyD, EALIGN_GRF, dstUniform);
            CVariable *V1 = m_currShader->GetNewVariable(dstUniform ? 1 : width, TyD, EALIGN_GRF, dstUniform);
            CVariable *V0_alias = m_currShader->GetNewAlias(V0, src->GetType(), 0, 0);
            CVariable *V1_alias = m_currShader->GetNewAlias(V1, src->GetType(), 0, 0);
            CVariable *dst_alias = m_currShader->GetNewAlias(m_destination, V0->GetType(), 0, 0);
            for(unsigned i = 0, offset = 0; i < dstNElts; ++i)
            {
                if (BIT(i) & destMask)
                {
                    for(unsigned j = 0; j < N; ++j)
                    {
                        bool useV0 = (j < N2);
                        uint32_t oft = useV0 ? j : j - N2;
                        m_encoder->SetSrcRegion(0, srcUniform ? 0 : 1, 1, 0);
                        m_encoder->SetSrcSubReg(0, srcUniform ? (i*N + j) : (width*(i*N + j)));
                        m_encoder->SetDstRegion(dstUniform ? 1 : N2);
                        m_encoder->SetDstSubReg(oft);
                        m_encoder->Copy(useV0 ? V0_alias : V1_alias, src);
                        m_encoder->Push();
                    }
                    // combine V0 and V1 into dst
                    if(splitDst)
                    {
                        // Dst must not be uniform and it must be SIMD16!
                        // first simd8 : dst_alias = V0
                        m_encoder->SetDstRegion(2);
                        m_encoder->SetDstSubReg(2 * offset*width);
                        m_encoder->SetSimdSize(SIMDMode::SIMD8);
                        m_encoder->SetMask(EMASK_Q1);
                        m_encoder->Copy(dst_alias, V0);
                        m_encoder->Push();
                        // second simd8: dst_alias=V0
                        m_encoder->SetSrcSubReg(0, 8);
                        m_encoder->SetDstRegion(2);
                        m_encoder->SetDstSubReg(2 * offset*width + 2 * 8);
                        m_encoder->SetSimdSize(SIMDMode::SIMD8);
                        m_encoder->SetMask(EMASK_Q2);
                        m_encoder->Copy(dst_alias, V0);
                        m_encoder->Push();

                        // first simd8 : dist_alias=V1
                        m_encoder->SetDstRegion(2);
                        m_encoder->SetDstSubReg(2 * offset*width + 1);
                        m_encoder->SetSimdSize(SIMDMode::SIMD8);
                        m_encoder->SetMask(EMASK_Q1);
                        m_encoder->Copy(dst_alias, V1);
                        m_encoder->Push();
                        // first simd8 : dist_alias=V1
                        m_encoder->SetSrcSubReg(0, 8);
                        m_encoder->SetDstRegion(2);
                        m_encoder->SetDstSubReg(2 * offset*width + 2 * 8 + 1);
                        m_encoder->SetSimdSize(SIMDMode::SIMD8);
                        m_encoder->SetMask(EMASK_Q2);
                        m_encoder->Copy(dst_alias, V1);
                        m_encoder->Push();
                    }
                    else
                    {
                        m_encoder->SetDstRegion(dstUniform ? 1 : 2);
                        m_encoder->SetDstSubReg(dstUniform ? (2 * offset) : (2 * offset*width));
                        m_encoder->Copy(dst_alias, V0);
                        m_encoder->Push();
                        m_encoder->SetDstRegion(dstUniform ? 1 : 2);
                        m_encoder->SetDstSubReg(dstUniform ? (2 * offset + 1) : (2 * offset*width + 1));
                        m_encoder->Copy(dst_alias, V1);
                        m_encoder->Push();
                    }
                    offset++;
                }
            }
        }
        else
        {
            for( unsigned i=0, offset=0; i < dstNElts; ++i )
            {
                if(BIT(i) & destMask)
                {
                    for(unsigned j = 0; j < N; ++j)
                    {
                        if(splitDst)
                        {
                            // width must be 16 & !dstUniform
                            // first simd8
                            m_encoder->SetSrcRegion(0, srcUniform ? 0 : 1, 1, 0);
                            m_encoder->SetSrcSubReg(0, srcUniform ? (i*N + j) : (width*(i*N + j)));
                            m_encoder->SetDstRegion(N);
                            m_encoder->SetDstSubReg(offset*N*width + j);
                            m_encoder->SetSimdSize(SIMDMode::SIMD8);
                            m_encoder->SetMask(EMASK_Q1);
                            m_encoder->Copy(aliasDst, src);
                            m_encoder->Push();

                            // second simd8
                            m_encoder->SetSrcRegion(0, srcUniform ? 0 : 1, 1, 0);
                            m_encoder->SetSrcSubReg(0, srcUniform ? (i*N + j) : (width*(i*N + j) + 8));
                            m_encoder->SetDstRegion(N);
                            m_encoder->SetDstSubReg(offset*N*width + N * 8 + j);
                            m_encoder->SetSimdSize(SIMDMode::SIMD8);
                            m_encoder->SetMask(EMASK_Q2);
                            m_encoder->Copy(aliasDst, src);
                            m_encoder->Push();
                        }
                        else
                        {
                            m_encoder->SetSrcRegion(0, srcUniform ? 0 : 1, 1, 0);
                            m_encoder->SetSrcSubReg(0, srcUniform ? (i*N + j) : (width*(i*N + j)));
                            m_encoder->SetDstRegion(dstUniform ? 1 : N);
                            m_encoder->SetDstSubReg(dstUniform ? (offset*N + j) : (offset*N*width + j));
                            m_encoder->Copy(aliasDst, src);
                            m_encoder->Push();
                        }
                    }
                    offset++;
                }
             }
        }
    }
    else
    {   // srcEltBytes > dstEltBytes
        // Create an aliase to src and mov the alias to the dst
        CVariable *aliasSrc =  m_currShader->GetNewAlias(src, m_destination->GetType(), 0, 0);
        uint32_t N = srcEltBytes / dstEltBytes;
        // Similar to dstEltBytes > srcEltBytes, dstEltBytes can be 32bit
        // at most and dst's stride == 1, so it will not need spliting.
        bool splitSrc = (!srcUniform && (srcEltBytes * width > 64U));
        assert( (!splitSrc || (width == 16)) &&
                "Internal Error: Src needs splitting only under SIMD16!");
        assert( (srcEltBytes % dstEltBytes) == 0 && "Basic types should be power of 2");
        // avoid coalescing the dst variable if all of its uses are EEI with constant index
        // this give RA more freedom (e.g. for bank conflict assignments)
        auto allUsesAreEEwithImm = [this](BitCastInst* BCI)
        {
            for (auto I = BCI->user_begin(), E = BCI->user_end(); I != E; ++I)
            {
                if (auto EEInst = dyn_cast<ExtractElementInst>(*I))
                {
                    if (dyn_cast<ConstantInt>(EEInst->getIndexOperand()))
                    {
                        continue;
                    }
                }
                return false;
            }
            return true;
        };

        SmallVector<CVariable*, 8> VectorBCICVars;
        bool useSeparateCVar = m_currShader->m_numberInstance == 1 && 
            !dstUniform && srcNElts == 1 && N <= 8 &&
            allUsesAreEEwithImm(BCI);

        for( unsigned i=0, offset = 0; i < srcNElts; ++i )
        {
            for( unsigned j=0; j < N; ++j )
            {
                if (BIT(i * N + j) & destMask)
                {
                    if (useSeparateCVar)
                    {
                        CVariable* newDst = m_currShader->GetNewVariable(width, m_destination->GetType(),
                            m_destination->GetAlign());
                        VectorBCICVars.push_back(newDst);
                        m_destination = newDst;
                    }
                    if(splitSrc)
                    {
                        // SIMD16 && !srcUniform
                        // first simd8
                        m_encoder->SetSrcRegion(0, N, 1, 0); // = (0, width*N, width, N)
                        m_encoder->SetSrcSubReg(0, i * N * width + j);
                        m_encoder->SetDstSubReg(dstUniform ? offset : (width * offset));
                        m_encoder->SetDstRegion(1);
                        m_encoder->SetSimdSize(SIMDMode::SIMD8);
                        m_encoder->SetMask(EMASK_Q1);
                        m_encoder->Copy(m_destination, aliasSrc);
                        m_encoder->Push();

                        // second simd8
                        m_encoder->SetSrcRegion(0, N, 1, 0); // = (0, width*N, width, N)
                        m_encoder->SetSrcSubReg(0, i * N * width + N * 8 + j);
                        m_encoder->SetDstSubReg(dstUniform ? offset : (width * offset + 8));
                        m_encoder->SetDstRegion(1);
                        m_encoder->SetSimdSize(SIMDMode::SIMD8);
                        m_encoder->SetMask(EMASK_Q2);
                        m_encoder->Copy(m_destination, aliasSrc);
                        m_encoder->Push();
                    }
                    else
                    {
                        m_encoder->SetSrcRegion(0, srcUniform ? 0 : N, 1, 0); // = (0, width*N, width, N)
                        m_encoder->SetSrcSubReg(0, srcUniform ? (i * N + j) : (i * N * width + j));
                        m_encoder->SetDstSubReg(dstUniform ? offset : (width * offset));
                        m_encoder->SetDstRegion(1);
                        m_encoder->Copy(m_destination, aliasSrc);
                        m_encoder->Push();
                    }
                    if (!useSeparateCVar)
                    {
                        // offset stays as zero if we are using distinct variablbes for each EEI
                        offset++;
                    }
                }
            }
        }

        if (useSeparateCVar)
        {
            m_currShader->addCVarsForVectorBC(BCI, VectorBCICVars);
        }
    }
}

unsigned int EmitPass::GetScalarTypeSizeInRegister(Type *Ty) const
{
    unsigned int sizeInBites = Ty->getScalarSizeInBits();
    if(Ty->isPointerTy())
    {
        sizeInBites =
            m_currShader->GetContext()->getRegisterPointerSizeInBits(Ty->getPointerAddressSpace());
    }
    return sizeInBites / 8;
}

void EmitPass::emitVectorLoad(LoadInst* inst, Value* offset)
{
    Value* Ptr = inst->getPointerOperand();
    PointerType* ptrType = cast<PointerType>(Ptr->getType());
    bool useA32 = !IGC::isA64Ptr(ptrType, m_currShader->GetContext());

    ResourceDescriptor resource = GetResourceVariable(Ptr);
    // eOffset is in bytes as 2/19/14
    CVariable *eOffset = GetSymbol(offset);
    if (useA32)
    {
        eOffset = TruncatePointer(eOffset);
    }

    Type *Ty = inst->getType();
    VectorType *VTy = dyn_cast<VectorType>(Ty);
    Type *eltTy = VTy ? VTy->getElementType() : Ty;
    uint32_t eltBytes = GetScalarTypeSizeInRegister(eltTy);
    assert((eltBytes == 1 ||
        eltBytes == 2 ||
        eltBytes == 4 ||
        eltBytes == 8) &&
           "Load's type (element type if vector) must be 1/2/4/8-byte long");

    uint32_t elts = VTy ? int_cast<uint32_t>(VTy->getNumElements()) : 1;
    uint32_t totalBytes = eltBytes * elts;

    bool destUniform = m_destination->IsUniform();
    bool srcUniform = eOffset->IsUniform();
    // Not possible to have uniform dest AND non-uniform src.
    assert(!(destUniform && !srcUniform) &&
           "If ld's dest is uniform, ld's src must be uniform");

    unsigned align = inst->getAlignment();
    VISA_Type destType = m_destination->GetType();
    uint32_t width = numLanes(m_currShader->m_SIMDSize);
    uint bufferIndex;
    bool directIndexing;
    BufferType bufType = DecodeAS4GFXResource(ptrType->getAddressSpace(), directIndexing, bufferIndex);

    // First, special handling for less than 4 bytes of loaded value
    if (totalBytes < 4)
    {
        // totalBytes is either 1 or 2, and it must be scalar or (1-element vector)
        assert((elts == 1) && (totalBytes != 3) &&
               "Internal Error: do not expect <2 x i8> or <3 x i8>");

        uint16_t nbelts = srcUniform ? 1 : width;
        eOffset = ReAlignUniformVariable(eOffset, EALIGN_GRF);

        bool needTemporary = (totalBytes < 4) || !m_destination->IsGRFAligned();
        CVariable *gatherDst = m_destination;
        if(needTemporary)
        {
            gatherDst = m_currShader->GetNewVariable(nbelts, ISA_TYPE_UD, EALIGN_GRF, srcUniform);
        }

        if (srcUniform)
        {
            m_encoder->SetNoMask();
            m_encoder->SetUniformSIMDSize(SIMDMode::SIMD1);
        }
        if (useA32)
        {
            m_encoder->ByteGather(gatherDst, resource, eOffset, 8, totalBytes);
        }
        else
        {
            m_encoder->GatherA64(gatherDst, eOffset, 8, totalBytes);
        }
        m_encoder->Push();

        if(needTemporary)
        {
            gatherDst = m_currShader->GetNewAlias(gatherDst, destType, 0, 0);
            uint32_t vStride = srcUniform ? 0 : ((totalBytes == 1) ? 4 : 2);
            m_encoder->SetSrcRegion(0, vStride, 1, 0);
            m_encoder->Copy(m_destination, gatherDst);
            m_encoder->Push();
        }
        return;
    }

    // generate oword-load if possible
    if (VTy && srcUniform)
    {
        //uint32_t totalBytes = eltBytes * VTy->getNumElements();
        bool rightBlockSize = (totalBytes == 16 || totalBytes == 32 || totalBytes == 64 || totalBytes == 128);
		bool useDWAligned = (resource.m_surfaceType != ESURFACE_SLM && align && align >= 4);
		bool useOWAligned = false;
        if (rightBlockSize && (useDWAligned || useOWAligned))
        {
            bool needTemp = (!destUniform || !m_destination->IsGRFAligned());
            CVariable *loadDest = m_destination;

			if (useOWAligned)
			{
				// Offset needs to be in OW!
				m_encoder->Shr(eOffset, eOffset, m_currShader->ImmToVariable(4, ISA_TYPE_UD));
				m_encoder->Push();
			}
            eOffset = ReAlignUniformVariable(eOffset, EALIGN_GRF);
            if (needTemp)
            {
                loadDest = m_currShader->GetNewVariable(m_destination->GetNumberElement(),
                                                        m_destination->GetType(),
                                                        EALIGN_GRF,
                                                        true);
            }

            if (useA32)
            {
                m_encoder->OWLoad(loadDest, resource, eOffset, useOWAligned, loadDest->GetSize());
            }
            else
            {
				assert(!useOWAligned && "SLM's pointer size must be 32 bit!");
                // emit svm block read
                m_encoder->OWLoadA64(loadDest, eOffset, loadDest->GetSize());
            }
            m_encoder->Push();

            if (needTemp)
            {
                emitVectorCopy(m_destination, loadDest, int_cast<unsigned>(VTy->getNumElements()));
            }
            return;
        }
    }

    // Only handle 4/8/12/16/32 bytes here. For aligned 16/32 bytes, it should've been handled
    // by oword already (except for SLM).  We have 12 bytes for load of int3 (either aligned or
	// unaligned[vload]).
	//
	// Note that for simplicity, don't do it if totalBytes=32 and 64bit integer adds are needed
	// on platform that does not support 64bit integer add.
    if (srcUniform && (totalBytes == 4 || totalBytes == 8 || totalBytes == 12 || totalBytes == 16 ||
		               (totalBytes == 32 && (useA32 || !m_currShader->m_Platform->hasNo64BitInst()))))
    {
        bool needTemp = !destUniform ||
                        !m_destination->IsGRFAligned() ||
                        totalBytes == 12;
        // For uniform src, we can map value to messages (vector re-layout) as follows
        //   1. A64:
        //      <1 x i64> for align=8 && totalBytes=8 (eltBytes == 4 or 8);
        //        [ (blksize, nblk) = (64, 1) ]
        //      <n x i32> for align=4; [ (blksize, nblk) = (32, 1) ]
        //      <n x S> for align < 4,
        //         where S = <8xi8> if eltBytes = 8, or S = <4xi8> othewise;
        //         [ (blksize, nblk) = (8, 8) or (8, 4) ]
        //   2. A32:
        //      <n x S>, where S = <4 x i8>, ie, block size = 8 bits and #blocks = 4
        //         [ (blksize, nblk) = (8, 4) ]
        //   where n is the member of elements

        // use A64 scattered RW with QW block size; Note that totalBytes == 16 with align >=4
        // should be handled by oword already (except for SLM).
        bool useQW = (!useA32) && (totalBytes == 8 || totalBytes == 16) &&
                     (align >= 8 || eltBytes == 8);

        // activelanes is the number of lanes that are needed.
        // nbelts is activelanes rounded up to the power of 2.
        uint16_t activelanes = useQW ? (totalBytes / 8) : (totalBytes / 4);
        uint16_t nbelts = (activelanes == 3 ? 4 : activelanes);

        // For scattered RW
        uint32_t blkBits = useA32 ? 8 : (align < 4 ? 8 : (useQW ? 64 : 32));
        uint32_t nBlks   = useA32 ? 4 : (align < 4 ? (useQW ? 8 : 4) : 1);

        VISA_Type ldType = useQW ? ISA_TYPE_UQ : ISA_TYPE_UD;
        CVariable *gatherDst;
        if (needTemp)
        {
            gatherDst = m_currShader->GetNewVariable(
                nbelts, ldType, EALIGN_GRF, true /*srcUniform*/);
        }
        else
        {
            gatherDst = m_destination;
            if (m_destination->GetType() != ldType)
            {
                gatherDst = m_currShader->GetNewAlias(gatherDst, ldType, 0, nbelts);
            }
        }

        SIMDMode simdmode = lanesToSIMDMode(nbelts);
        eOffset = ReAlignUniformVariable(eOffset, useA32 ? EALIGN_GRF : EALIGN_2GRF);
        CVariable *gatherOff = eOffset;
        if (nbelts > 1)
        {
            gatherOff = m_currShader->GetNewVariable(
                nbelts, eOffset->GetType(), eOffset->GetAlign(), true /*srcUniform*/ );
            // May have the following
            //   lane   0   1   2   3   4    5   6   7
            //   eOff   0   4   8   C   10   14  18  1C // DW per lane
            //   eOff   0   8                           // QW per lane
            // When nbelts = 3, lane 3 is not used. Since we don't have simd3,
            // use simd4 and set lane3 to lane2.
			uint32_t incImm;
			uint32_t incImm1;  // for activelanes=8
			switch (activelanes) {
			default:
				assert(false && "ICE: something wrong happened in computing activelanes!");
				break;
			case 2:
				// only can have QW in this case
				incImm = useQW ? 0x80 : 0x40;
				break;
			case 3:
				// set lane3 to be the same as lane2 (it is 8)
				incImm = 0x8840;
				break;
			case 4:
				incImm = 0xC840;
				break;
			case 8:
				// Make sure incImm + incImm1 = {0  4  8  C  10   14  18  1C}
				incImm  = 0xD951C840;
				incImm1 = 0xFFFF0000;
				break;
			}

            CVariable* immVar = m_currShader->ImmToVariable(incImm, ISA_TYPE_UV);
            if (!useA32 && m_currShader->m_Platform->hasNo64BitInst()) {
                emitAddPairWithImm(gatherOff, eOffset, immVar);
            } else {
                m_encoder->SetNoMask();
                m_encoder->SetUniformSIMDSize(simdmode);
                m_encoder->SetSrcRegion(0, 0, 1, 0);
                m_encoder->Add(gatherOff, eOffset, immVar);
                m_encoder->Push();
            }

			if (activelanes == 8) {
				CVariable* immVar1 = m_currShader->ImmToVariable(incImm1, ISA_TYPE_UV);
				m_encoder->SetNoMask();
				m_encoder->SetUniformSIMDSize(simdmode);
				m_encoder->SetSrcRegion(0, 8, 8, 1);
				m_encoder->Add(gatherOff, gatherOff, immVar1);
				m_encoder->Push();
			}
        }

        m_encoder->SetNoMask();
        m_encoder->SetUniformSIMDSize(simdmode);
        if (useA32)
        {
            m_encoder->SetNoMask();
            m_encoder->SetUniformSIMDSize(simdmode);
            m_encoder->ByteGather(gatherDst, resource, gatherOff, blkBits, nBlks);
        }
        else
        {
            m_encoder->GatherA64(gatherDst, gatherOff, blkBits, nBlks);
        }
        m_encoder->Push();

        if (needTemp)
        {
            CVariable *newDst = m_currShader->GetNewAlias(
                gatherDst, destType, 0, m_destination->GetNumberElement());
            emitVectorCopy(m_destination, newDst, elts);
        }
        return;
    }
    CVariable *subLoadDst;
    CVariable *rawAddrVar;

    // some driver describe constant buffer as typed which forces us to use byte scatter message
    bool forceByteScatteredRW =
        bufType == CONSTANT_BUFFER &&
        m_currShader->m_DriverInfo->UsesTypedConstantBuffers();

    VectorMessage VecMessInfo(this);
    VecMessInfo.getInfo(Ty, align, useA32, forceByteScatteredRW);

    // Handle uniform case in general
    if( srcUniform )
    {
        // Use width of 8 always, and only the value of the first lane is
        // used. Need to set noMask in order to have the valid value in
        // the first lane.
        uint32_t width8 = 8;
        for( uint32_t i=0; i < VecMessInfo.numInsts; ++i )
        {
            // raw operand, eltOffBytes is in bytes.
            uint32_t eltOffBytes = VecMessInfo.insts[i].startByte;
            uint32_t blkInBytes = VecMessInfo.insts[i].blkInBytes;
            uint32_t numBlks = VecMessInfo.insts[i].numBlks;

            uint32_t eltOff = eltOffBytes / eltBytes;  // in unit of element
            uint32_t blkBits = 8 * blkInBytes;
            uint32_t instTotalBytes = blkInBytes * numBlks;
            uint32_t instElts = instTotalBytes / eltBytes;
            uint32_t nbelts = instElts * width8;

            if( i > 0 )
            {
                // Calculate the new element offset
                rawAddrVar = m_currShader->GetNewVariable(eOffset);
                CVariable *ImmVar = m_currShader->ImmToVariable(eltOffBytes, ISA_TYPE_UD);
                if (!useA32 && m_currShader->m_Platform->hasNo64BitInst()) {
                    emitAddPairWithImm(rawAddrVar, eOffset, ImmVar);
                } else {
                    m_encoder->SetNoMask();
                    m_encoder->Add(rawAddrVar, eOffset, ImmVar);
                    m_encoder->Push();
                }
            }
            else
            {
                rawAddrVar = eOffset;
            }
            CVariable* addrVarSIMD8 = m_currShader->GetNewVariable(8, rawAddrVar->GetType(), EALIGN_GRF);
            m_encoder->SetNoMask();
            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            m_encoder->Copy(addrVarSIMD8, rawAddrVar);

            subLoadDst = m_currShader->GetNewVariable((uint16_t)nbelts, destType, EALIGN_GRF);
            m_encoder->SetNoMask();
            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            VectorMessage::MESSAGE_KIND messageType = VecMessInfo.insts[i].kind;
            switch(messageType) {
            case VectorMessage::MESSAGE_A32_BYTE_SCATTERED_RW:
                m_encoder->ByteGather(subLoadDst, resource, addrVarSIMD8, blkBits, numBlks);
                break;
            case VectorMessage::MESSAGE_A32_UNTYPED_SURFACE_RW:
                m_encoder->Gather4Scaled(subLoadDst, resource, addrVarSIMD8);
                break;
            case VectorMessage::MESSAGE_A64_UNTYPED_SURFACE_RW:
                m_encoder->Gather4A64(subLoadDst, addrVarSIMD8);
                break;
            case VectorMessage::MESSAGE_A64_SCATTERED_RW:
                m_encoder->GatherA64(subLoadDst, addrVarSIMD8, blkBits, numBlks);
                break;
            default:
                assert(0 && "Somethings wrong!");
            }
            m_encoder->Push();

            for (uint32_t n = 0; n < instElts; ++n)
            {
                m_encoder->SetSrcRegion(0, 0, 1, 0);
                m_encoder->SetSrcSubReg(0, n * width8);
                m_encoder->SetDstSubReg(eltOff + (destUniform ? n : n * width) );
                m_encoder->Copy(m_destination, subLoadDst);
                m_encoder->Push();
            }
        }

        return;
    }

    // Second, src isn't uniform
    for( uint32_t i=0; i < VecMessInfo.numInsts; ++i )
    {
        // raw operand, eltOffBytes is in bytes.
        uint32_t eltOffBytes = VecMessInfo.insts[i].startByte * width;
        uint32_t blkInBytes = VecMessInfo.insts[i].blkInBytes;
        uint32_t numBlks = VecMessInfo.insts[i].numBlks;
        uint32_t eltOff = eltOffBytes / eltBytes;
        uint32_t blkBits = 8 * blkInBytes;
        uint32_t instTotalBytes = blkInBytes * numBlks;
        uint32_t instElts = instTotalBytes / eltBytes;
        uint32_t nbelts = instElts * width;

        if( i > 0 )
        {
            // Calculate the new element offset
            rawAddrVar = m_currShader->GetNewVariable(eOffset);
            CVariable *ImmVar = m_currShader->ImmToVariable(VecMessInfo.insts[i].startByte, ISA_TYPE_UD);
            if (!useA32 && m_currShader->m_Platform->hasNo64BitInst()) {
                emitAddPairWithImm(rawAddrVar, eOffset, ImmVar);
            } else {
                m_encoder->Add(rawAddrVar, eOffset, ImmVar);
                m_encoder->Push();
            }
        }
        else
        {
            rawAddrVar = eOffset;
        }

        bool needTemp = (!m_destination->IsGRFAligned());
        CVariable *gatherDst;
        if (needTemp)
        {
            gatherDst = m_currShader->GetNewVariable((uint16_t)nbelts, destType, EALIGN_GRF);
        }
        else
        {
            // No need to copy, load directly into m_destination
            gatherDst = m_currShader->GetNewAlias(m_destination,
                destType, (uint16_t)eltOffBytes, (uint16_t)nbelts);
        }
        VectorMessage::MESSAGE_KIND messageType = VecMessInfo.insts[i].kind;
        switch (messageType) {
        case VectorMessage::MESSAGE_A32_BYTE_SCATTERED_RW:
            m_encoder->ByteGather(gatherDst, resource, rawAddrVar, blkBits, numBlks);
            break;
        case VectorMessage::MESSAGE_A32_UNTYPED_SURFACE_RW:
            m_encoder->Gather4Scaled(gatherDst, resource, rawAddrVar);
            break;
        case VectorMessage::MESSAGE_A64_UNTYPED_SURFACE_RW:
            m_encoder->Gather4A64(gatherDst, rawAddrVar);
            break;
        case VectorMessage::MESSAGE_A64_SCATTERED_RW:
            m_encoder->GatherA64(gatherDst, rawAddrVar, blkBits, numBlks);
            break;
        default:
            assert(false && "Internal Error: unexpected message kind for load!");
        }
        m_encoder->Push();

        if (needTemp)
        {
            emitVectorCopy(m_destination, gatherDst, instElts, eltOff, 0);
        }
    }
}

void EmitPass::emitVectorStoreOCL(StoreInst* inst)
{
    Value* Ptr = inst->getPointerOperand();
    PointerType* ptrType = cast<PointerType>(Ptr->getType());

    ResourceDescriptor resource = GetResourceVariable(Ptr);

    // As 2/19/14, eOffset is in bytes !
    CVariable *eOffset = GetSymbol(Ptr);
    if (resource.m_surfaceType == ESURFACE_SLM)
    {
        eOffset = TruncatePointer(eOffset);
    }

    // In case eOffset isn't GRF aligned, need to create a copy
    // For non-uniform variable, it should be already GRF-aligned.
    eOffset = ReAlignUniformVariable(eOffset, EALIGN_GRF);

    Value *storedVal = inst->getValueOperand();
    Type *Ty = storedVal->getType();
    VectorType *VTy = dyn_cast<VectorType>(Ty);
    Type *eltTy = VTy ? VTy->getElementType() : Ty;
    uint32_t eltBytes = GetScalarTypeSizeInRegister(eltTy);

    assert((eltBytes == 1 ||
        eltBytes == 2 ||
        eltBytes == 4 ||
        eltBytes == 8) && "Store type must be 1/2/4/8-bytes long");

    uint32_t elts = VTy ? int_cast<uint32_t>(VTy->getNumElements()) : 1;
    uint32_t totalBytes = elts * eltBytes;
    unsigned align = inst->getAlignment();
    CVariable *storedVar = GetSymbol(storedVal);
    unsigned int width = numLanes(m_currShader->m_SIMDSize);

    bool srcUniform = storedVar->IsUniform();
    bool dstUniform = eOffset->IsUniform();
    bool useA32 = !isA64Ptr(ptrType, m_currShader->GetContext());

    // Handle two cases:
    //   1. less than 4 bytes: need to extend it to 4 bytes
    //   2. destination is scalar and uniform (handle vector if needed)
    if (totalBytes < 4 || isUniformStoreOCL(inst))
    {
        assert((totalBytes == 1 || totalBytes == 2 ||
                totalBytes == 4 || totalBytes == 8 ||
                totalBytes == 12 || totalBytes == 16) && "Wrong total Bytes!");

        SIMDMode simdmode = SIMDMode::SIMD1;
        e_alignment grfAlign = useA32 ? EALIGN_GRF : EALIGN_2GRF;
        uint32_t blkBits, nBlks;
        if (elts > 1)
        {
            // Vector uniform store: handle uniform value only.
            // For elts > 1, the eltBytes must be either 4 or 8; only elts = 2|3|4 are handled.
            assert((eltBytes == 4 || eltBytes == 8) && "ICE: wrong element bytes!");
            assert(dstUniform && srcUniform &&
                "ICE: for vector uniform store, both dst and src must be uniform!");

            // As we use simd8 for vector (SKL HW WA). Converting DW to QW
            // makes sense only if the final is a scalar (a single QW).
            bool useQW = (!useA32) &&
                (eltBytes == 8 ||        // requested by vector layout
                 (eltBytes == 4 && totalBytes == 8 && align >= 8)); // convert DW to QW

            // activelanes is the number of lanes that are needed.
            // nbelts is activelanes rounded up to the power of 2.
            uint16_t activelanes = useQW ? (totalBytes / 8) : (totalBytes / 4);
            uint16_t nbelts = (activelanes == 3 ? 4 : activelanes);

            // Work around of a possible SKL HW bug. Using send(4) for "store <4xi32>v, *p"
            // Therefore, using simd8 for A64 vector store to get around
            // of this issue..

            // This is simdmode we wanted, but we need to work around of A64 HW bug
            SIMDMode simdWanted = lanesToSIMDMode(nbelts);
            uint16_t nbeltsWanted = nbelts;
            if (!useA32 && nbelts > 1) {
                nbelts = 8;
            }
            simdmode = lanesToSIMDMode(nbelts);

            // compute offset
            // We have the following :
            //    lane   0   1   2   3
            //    eOff   0   4   8   C               // DW per lane
            //    eOff   0   8                       // QW per lane
            // When elts = 3, lane 3 is not used. Since we don't have simd3,
            // use simd4 and set lane3 to the same as lane2(8).
            //
            // When using simd8, all unused lanes will be the same as lane0.
            // Make sure offset & stored value are correctly set up.
            if (nbelts > 1)
            {
                CVariable *NewOff = m_currShader->GetNewVariable(
                    nbelts, eOffset->GetType(), grfAlign, true /*dstUniform*/);
                uint32_t incImm =
                    useQW ? 0x80 : (activelanes == 2 ? 0x40 : (activelanes == 3 ? 0x8840 : 0xC840));
                CVariable* immVar = m_currShader->ImmToVariable(incImm, ISA_TYPE_UV);

                // When work-around of A64 SKI Si limitation of SIMD4, we use SIMD8 (nbelts > nbeltsWanted)
                // in which all upper four channels are zero, meaning eOffset[0], Later, stored value
                // must use storvedVar[0] for those extra lanes.
                if (!useA32 && m_currShader->m_Platform->hasNo64BitInst()) {
                    emitAddPairWithImm(NewOff, eOffset, immVar);
                } else {
                    m_encoder->SetNoMask();
                    m_encoder->SetUniformSIMDSize(simdmode);
                    m_encoder->SetSrcRegion(0, 0, 1, 0);
                    m_encoder->Add(NewOff, eOffset, immVar);
                    m_encoder->Push();
                }

                eOffset = NewOff;
            }
            else
            {
                eOffset = ReAlignUniformVariable(eOffset, grfAlign);
            }


            // (We could have useQW == true AND eltBytes == 4. Note that if useQW
            // is false, eltBytes must be 4.)
            assert((useQW || eltBytes == 4) && "ICE: wrong vector element type!");

            // Since we might change element type, need to create copy.
            if (useQW && eltBytes == 4)
            {
                CVariable *tmp = m_currShader->GetNewVariable(
                    nbeltsWanted, ISA_TYPE_UQ, grfAlign, true /*srcUniform*/);
                CVariable *tmpAlias = m_currShader->GetNewAlias(tmp,
                    storedVar->GetType(), 0, 2 * nbeltsWanted);
                assert((2 * nbeltsWanted) == storedVar->GetNumberElement() &&
                    "Mismatch of the number of elements: sth wrong!");
                emitVectorCopy(tmpAlias, storedVar, 2 * nbeltsWanted);
                storedVar = tmp;
            }

            // Prepare stored value
            if (storedVar->IsImmediate() || activelanes < nbelts ||
                !storedVar->IsGRFAligned(grfAlign))
            {
                CVariable *NewVar = m_currShader->GetNewVariable(
                    nbelts, storedVar->GetType(), grfAlign, true /*srcUniform*/);

                // A64 SKL HW issue work-around: set remaining lanes to storedVar[0]
                // as eOffset has been set to the first element already.
                if (nbeltsWanted < nbelts)
                {
                    m_encoder->SetNoMask();
                    m_encoder->SetUniformSIMDSize(simdmode);
                    m_encoder->SetSrcRegion(0, 0, 1, 0);
                    m_encoder->Copy(NewVar, storedVar);
                    m_encoder->Push();
                }

                // Values that we care
                if (activelanes == 3)
                {
                    m_encoder->SetNoMask();
                    m_encoder->SetUniformSIMDSize(SIMDMode::SIMD2);
                    m_encoder->SetSrcRegion(0, 2, 2, 1);
                    m_encoder->Copy(NewVar, storedVar);
                    m_encoder->Push();

                    // offset is 0x8840, so duplicate lane2
                    m_encoder->SetNoMask();
                    m_encoder->SetUniformSIMDSize(SIMDMode::SIMD2);
                    m_encoder->SetDstSubReg(2);
                    m_encoder->SetSrcSubReg(0, 2);
                    m_encoder->SetSrcRegion(0, 0, 1, 0);
                    m_encoder->Copy(NewVar, storedVar);
                    m_encoder->Push();
                }
                else
                {
                    m_encoder->SetNoMask();
                    m_encoder->SetUniformSIMDSize(simdWanted);
                    m_encoder->SetSrcRegion(0, nbeltsWanted, nbeltsWanted, 1);
                    m_encoder->Copy(NewVar, storedVar);
                    m_encoder->Push();
                }
                storedVar = NewVar;
            }

            // each lane will store either DW or QW
            blkBits = useQW ? (align >= 8 ? 64 : 8)
                            : (!useA32 && align >= 4) ? 32 : 8;
            nBlks = useQW ? (64 / blkBits) : (32 / blkBits);
        }
        else
        {
            // scalar case (elts == 1)
            if (dstUniform)
            {
                eOffset = ReAlignUniformVariable(eOffset, grfAlign);
                if (!srcUniform)
                {
                    storedVar = UniformCopy(storedVar);
                }
                else
                {
                    storedVar = ReAlignUniformVariable(storedVar, grfAlign);
                }
                storedVar = ExtendVariable(storedVar, grfAlign);
            }
            else
            {
                storedVar = BroadcastAndExtend(storedVar);
            }


            // use either A32 byte scatter or A64 scatter messages.
            //   A32 should use byte as block size always here.
            //   A64 uses byte/DW/QW as block size based on align and element size.
            // Note that this is for elts = 1, so totalBytes is bytes per-lane.
            blkBits = useA32 ? 8 : ((eltBytes >= 4 && align >= eltBytes) ? eltBytes * 8 : 8);
            nBlks = (totalBytes * 8) / blkBits;
        }

        if (useA32)
        {
            m_encoder->ByteScatter(storedVar, resource, eOffset, blkBits, nBlks);
        }
        else
        {
            m_encoder->ScatterA64(storedVar, eOffset, blkBits, nBlks);
        }
        if (dstUniform)
        {
            m_encoder->SetNoMask();
            m_encoder->SetUniformSIMDSize(simdmode);
        }
        m_encoder->Push();

        return;
    }

    eOffset = BroadcastIfUniform(eOffset);
    storedVar = BroadcastIfUniform(storedVar);

    VectorMessage VecMessInfo(this);
    VecMessInfo.getInfo(Ty, align, useA32);

    for( uint32_t i=0; i < VecMessInfo.numInsts; ++i )
    {
        // raw operand, eltOff is in bytes
        uint32_t eltOffBytes = VecMessInfo.insts[i].startByte * width;
        uint32_t blkInBytes = VecMessInfo.insts[i].blkInBytes;
        uint32_t numBlks = VecMessInfo.insts[i].numBlks;
        uint32_t blkBits = 8 * blkInBytes;
        uint32_t instTotalBytes = blkInBytes * numBlks;
        uint32_t instElts = instTotalBytes / eltBytes;
        uint32_t nbelts = instElts * width;

        CVariable *rawAddrVar;
        if( i > 0 )
        {
            // Calculate the new element offset
            rawAddrVar = m_currShader->GetNewVariable(eOffset);
            CVariable *ImmVar = m_currShader->ImmToVariable(VecMessInfo.insts[i].startByte, ISA_TYPE_UD);
            if (!useA32 && m_currShader->m_Platform->hasNo64BitInst()) {
                emitAddPairWithImm(rawAddrVar, eOffset, ImmVar);
            } else {
                m_encoder->Add(rawAddrVar, eOffset, ImmVar);
                m_encoder->Push();
            }
        }
        else
        {
            rawAddrVar = eOffset;
        }

        VISA_Type storedType = storedVar->GetType();
        assert((eltOffBytes < (UINT16_MAX)) && "eltOffBytes > higher than 64k");
        assert((nbelts < (UINT16_MAX)) && "nbelts > higher than 64k");
        CVariable *subStoredVar = m_currShader->GetNewAlias(storedVar, storedType, (uint16_t)eltOffBytes, (uint16_t)nbelts);
        switch (VecMessInfo.insts[i].kind) {
        case VectorMessage::MESSAGE_A32_BYTE_SCATTERED_RW:
            m_encoder->ByteScatter(subStoredVar, resource, rawAddrVar, blkBits, numBlks);
            break;
        case VectorMessage::MESSAGE_A32_UNTYPED_SURFACE_RW:
            m_encoder->Scatter4Scaled(subStoredVar, resource, rawAddrVar);
            break;
        case VectorMessage::MESSAGE_A64_UNTYPED_SURFACE_RW:
            m_encoder->Scatter4A64(subStoredVar, rawAddrVar);
            break;
        case VectorMessage::MESSAGE_A64_SCATTERED_RW:
            m_encoder->ScatterA64(subStoredVar, rawAddrVar, blkBits, numBlks);
            break;
        default:
            assert(false && "Internal Error: unexpected Message kind for store");
        }
        m_encoder->Push();
    }
}

void EmitPass::emitVectorCopy(CVariable *Dst, CVariable *Src, uint32_t nElts,
                              uint32_t DstSubRegOffset, uint32_t SrcSubRegOffset)
{
    unsigned int width = numLanes(m_currShader->m_SIMDSize);
    bool srcUniform = Src->IsUniform();
    bool dstUniform = Dst->IsUniform();
    unsigned doff = DstSubRegOffset, soff = SrcSubRegOffset;

    // Uniform vector copy.
    if (srcUniform && dstUniform)
    {
        // The starting index of elements to be copied.
        unsigned i = 0;
        auto partialCopy = [=, &i](SIMDMode mod)
        {
            unsigned w = numLanes(mod);
            if (i + w > nElts)
            {
                return false;
            }

            unsigned vStride = (mod == SIMDMode::SIMD1) ? 0 : 1;
            m_encoder->SetUniformSIMDSize(mod);
            m_encoder->SetSrcRegion(0, vStride, 1, 0);
            m_encoder->SetSrcSubReg(0, soff + i);
            m_encoder->SetDstSubReg(doff + i);
            m_encoder->Copy(Dst, Src);
            m_encoder->Push();

            i += w;
            return true;
        };

        // We may select the initial simd size based on the element type.
        while (partialCopy(SIMDMode::SIMD8))
            ;
        partialCopy(SIMDMode::SIMD4);
        partialCopy(SIMDMode::SIMD2);
        partialCopy(SIMDMode::SIMD1);
        return;
    }

    for( uint32_t i = 0; i < nElts; ++i )
    {
        uint SrcSubReg = srcUniform ? soff + i : soff + width*i;
        uint DstSubReg = dstUniform ? doff + i : doff + width*i;

        uint SrcWidth  = srcUniform ? 1 : width;
        uint DstWidth  = dstUniform ? 1 : width;

        if (SrcSubReg >= Src->GetNumberElement() ||
            DstSubReg >= Dst->GetNumberElement())
        {
            break;
        }

        bool SrcOverflow = (SrcSubReg + SrcWidth > Src->GetNumberElement());
        bool DstOverflow = (DstSubReg + DstWidth > Dst->GetNumberElement());

        // This is currently used for VME payloads whose LLVM type doesn't
        // necessarily match the associated CVariable size (the LLVM type
        // will be at least as big as the CVariable). Here, we make sure that,
        // if an entire vector element is not copied, we emit movs to just
        // read or write the appropriate number of bytes.
        if (SrcOverflow || DstOverflow)
        {
            if (srcUniform)
            {
                auto partialCopy = [&](SIMDMode mode)
                {
                    unsigned w = numLanes(mode);

                    if (DstSubReg + w > Dst->GetNumberElement())
                        return;

                    m_encoder->SetSimdSize(mode);
                    m_encoder->SetSrcSubReg(0, SrcSubReg);
                    m_encoder->SetDstSubReg(DstSubReg);
                    m_encoder->Copy(Dst, Src);
                    m_encoder->Push();

                    DstSubReg += w;
                };

                partialCopy(SIMDMode::SIMD8);
                partialCopy(SIMDMode::SIMD4);
                partialCopy(SIMDMode::SIMD2);
                partialCopy(SIMDMode::SIMD1);
            }
            else
            {
                auto partialCopy = [&](SIMDMode mode)
                {
                    unsigned w = numLanes(mode);

                    if (DstSubReg + w > Dst->GetNumberElement() ||
                        SrcSubReg + w > Src->GetNumberElement())
                        return;

                    m_encoder->SetSimdSize(mode);
                    m_encoder->SetSrcSubReg(0, SrcSubReg);
                    m_encoder->SetDstSubReg(DstSubReg);
                    m_encoder->Copy(Dst, Src);
                    m_encoder->Push();

                    DstSubReg += w;
                    SrcSubReg += w;
                };

                partialCopy(SIMDMode::SIMD8);
                partialCopy(SIMDMode::SIMD4);
                partialCopy(SIMDMode::SIMD2);
                partialCopy(SIMDMode::SIMD1);
            }

            break;
        }

        m_encoder->SetSrcSubReg(0, SrcSubReg);
        m_encoder->SetDstSubReg(DstSubReg);
        m_encoder->Copy(Dst, Src);
        m_encoder->Push();
    }
}

/// \brief Emulate the 64-bit addition of uniform `Src` with vector immediate
/// value `Imm`. `Imm` should be in type of Type_UV.
void EmitPass::emitAddPairWithImm(CVariable *Dst, CVariable *Src,
                                  CVariable *Imm) {
    assert(Dst->GetType() == ISA_TYPE_Q || Dst->GetType() == ISA_TYPE_UQ);
    assert(Src->GetType() == ISA_TYPE_Q || Src->GetType() == ISA_TYPE_UQ);

    bool IsUniformSrc = Src->IsUniform();
    bool IsUniformDst = Dst->IsUniform();

    unsigned short NumElts = Dst->GetNumberElement();
    SIMDMode Mode = lanesToSIMDMode(NumElts);

    VISA_Type NewType = ISA_TYPE_UD;
    CVariable *SrcAlias = m_currShader->GetNewAlias(Src, NewType, 0, 0);
    CVariable *L0 = m_currShader->GetNewVariable(NumElts, NewType, EALIGN_GRF, IsUniformDst);
    CVariable *H0 = m_currShader->GetNewVariable(NumElts, NewType, EALIGN_GRF, IsUniformDst);

    // L0 := Offset[0];
    if (IsUniformDst) {
        m_encoder->SetNoMask();
        m_encoder->SetUniformSIMDSize(Mode);
    }
    if (IsUniformSrc)
        m_encoder->SetSrcRegion(0, 0, 1, 0);
    else
        m_encoder->SetSrcRegion(0, 2, 1, 0);
    m_encoder->Copy(L0, SrcAlias);
    m_encoder->Push();
    // H0 := Offset[1];
    if (IsUniformDst) {
        m_encoder->SetNoMask();
        m_encoder->SetUniformSIMDSize(Mode);
    }
    m_encoder->SetSrcSubReg(0, 1);
    if (IsUniformSrc)
        m_encoder->SetSrcRegion(0, 0, 1, 0);
    else
        m_encoder->SetSrcRegion(0, 2, 1, 0);
    m_encoder->Copy(H0, SrcAlias);
    m_encoder->Push();

    CVariable *Lo = m_currShader->GetNewVariable(NumElts, NewType, EALIGN_GRF, IsUniformDst);
    CVariable *Hi = m_currShader->GetNewVariable(NumElts, NewType, EALIGN_GRF, IsUniformDst);
    CVariable *Zero = m_currShader->ImmToVariable(0, ISA_TYPE_UV);
    // (Lo, Hi) := AddPair(L0, H0, ImmLo, ImmHi);
    if (IsUniformDst) {
        m_encoder->SetNoMask();
        m_encoder->SetUniformSIMDSize(Mode);
        m_encoder->SetSrcRegion(0, 1, 1, 0);
        m_encoder->SetSrcRegion(1, 1, 1, 0);
    }
    m_encoder->AddPair(Lo, Hi, L0, H0, Imm, Zero);
    m_encoder->Push();

    CVariable *DstAlias = m_currShader->GetNewAlias(Dst, NewType, 0, 0);
    // Offset[0] := Lo;
    if (IsUniformDst) {
        m_encoder->SetNoMask();
        m_encoder->SetUniformSIMDSize(Mode);
        m_encoder->SetSrcRegion(0, 1, 1, 0);
    }
    m_encoder->SetDstRegion(2);
    m_encoder->Copy(DstAlias, Lo);
    m_encoder->Push();
    // Offset[1] := Hi;
    if (IsUniformDst) {
        m_encoder->SetNoMask();
        m_encoder->SetUniformSIMDSize(Mode);
        m_encoder->SetSrcRegion(0, 1, 1, 0);
    }
    m_encoder->SetDstSubReg(1);
    m_encoder->SetDstRegion(2);
    m_encoder->Copy(DstAlias, Hi);
    m_encoder->Push();
}

/// \brief Copy all values from the src variable to the dst variable.
/// The last argument is the underlying value type.
void EmitPass::emitCopyAll(CVariable *Dst, CVariable *Src, llvm::Type *Ty)
{
    if (Src->GetVarType() == EVARTYPE_PREDICATE)
    {
        assert(!Ty->isVectorTy() && "vector of predicates?");
        assert(Dst->GetVarType() == Src->GetVarType());
        CVariable *Zero = m_currShader->ImmToVariable(0, ISA_TYPE_BOOL);
        m_encoder->Or(Dst, Src, Zero);
        m_encoder->Push();
    }
    else if (Ty->isVectorTy())
    {
        unsigned NElts = Ty->getVectorNumElements();
        emitVectorCopy(Dst, Src, NElts);
    }
    else
    {
        assert(Ty->isSingleValueType() && "not supported");
        m_encoder->Copy(Dst, Src);
        m_encoder->Push();
    }
}

void EmitPass::emitSqrt(Instruction *inst)
{
    GenIntrinsicInst *intrinCall = llvm::cast<GenIntrinsicInst>(inst);
    CVariable *src0 = GetSymbol(intrinCall->getArgOperand(0));
    src0 = BroadcastIfUniform(src0);

    m_encoder->Sqrt(m_destination, src0);
}

// emit llvm.bswap
void EmitPass::emitLLVMbswap(IntrinsicInst* inst)
{
    Type *Ty = inst->getType();
    Value *Arg = inst->getArgOperand(0);
    uint32_t nBytes = int_cast<uint32_t>(m_DL->getTypeSizeInBits(Ty));
    assert(nBytes % 16 == 0 && !Ty->isVectorTy() && "Incorrect llvm.bswap!");
    nBytes >>= 3;  // Now, nBytes are in unit of byte.

    CVariable* Src = GetSymbol(Arg);
    CVariable* Dst = m_destination;
    uint32_t width = numLanes(m_currShader->m_SIMDSize);
    bool srcUniform = Src->IsUniform();
    bool dstUniform = Dst->IsUniform();

    CVariable *SrcB = m_currShader->GetNewAlias(Src, ISA_TYPE_UB, 0, 0);
    if ( nBytes == 2 || nBytes == 4 )
    {
        CVariable *DstB = m_currShader->GetNewAlias(Dst, ISA_TYPE_UB, 0, 0);

        // Generating byte mov
        for(unsigned i = 0; i < nBytes/2; ++i)
        {
            // swap bytes[i] with bytes[j].
            uint32_t j = (nBytes - 1) - i;

            m_encoder->SetSrcSubReg(0, i);
            m_encoder->SetSrcRegion(0, srcUniform ? 0 : nBytes, 1, 0);
            m_encoder->SetDstSubReg(j);
            m_encoder->SetDstRegion(dstUniform ? 1 : nBytes);
            m_encoder->Copy(DstB, SrcB);
            m_encoder->Push();

            m_encoder->SetSrcSubReg(0, j);
            m_encoder->SetSrcRegion(0, srcUniform ? 0 : nBytes, 1, 0);
            m_encoder->SetDstSubReg(i);
            m_encoder->SetDstRegion(dstUniform ? 1 : nBytes);
            m_encoder->Copy(DstB, SrcB);
            m_encoder->Push();
        }
    }
    else if( nBytes == 8 )
    {
        // Need to so lower DW and upper DW separately first.
        m_currShader->GetNewAlias(Src, ISA_TYPE_UD, 0, 0);
        CVariable* DstH = m_currShader->GetNewVariable(Src->GetNumberElement(),
                                                    ISA_TYPE_UD,
                                                    Src->GetAlign(),
                                                    srcUniform);
        CVariable* DstL = m_currShader->GetNewVariable(Src->GetNumberElement(),
                                                    ISA_TYPE_UD,
                                                    Src->GetAlign(),
                                                    srcUniform);
        CVariable *DstHB = m_currShader->GetNewAlias(DstH, ISA_TYPE_UB, 0, 0);
        CVariable *DstLB = m_currShader->GetNewAlias(DstL, ISA_TYPE_UB, 0, 0);

        bool split = (width == 16);
        for(unsigned n=0; n < 2; ++n)
        {
            for(unsigned i=0; i < 4; ++i)
            {
                // swap bytes[i] and bytes[j]
                uint32_t j = 3 - i;
                if( split && !srcUniform)
                {
                    m_encoder->SetSrcSubReg(0, 4*n + i);
                    m_encoder->SetSrcRegion(0, 8, 1, 0);
                    m_encoder->SetDstSubReg(j);
                    m_encoder->SetDstRegion(4);
                    m_encoder->SetSimdSize(SIMDMode::SIMD8);
                    m_encoder->SetMask(EMASK_Q1);
                    m_encoder->Copy(n == 0 ? DstHB : DstLB, SrcB);
                    m_encoder->Push();

                    m_encoder->SetSrcSubReg(0, 2*SIZE_GRF + 4*n + i);
                    m_encoder->SetSrcRegion(0, 8, 1, 0);
                    m_encoder->SetDstSubReg(SIZE_GRF + j);
                    m_encoder->SetDstRegion(4);
                    m_encoder->SetSimdSize(SIMDMode::SIMD8);
                    m_encoder->SetMask(EMASK_Q2);
                    m_encoder->Copy(n == 0 ? DstHB : DstLB, SrcB);
                    m_encoder->Push();
                }
                else
                {
                    // DstH[B]/DstL[B] have the same uniformness as Src !
                    m_encoder->SetSrcSubReg(0, 4*n + i);
                    m_encoder->SetSrcRegion(0, srcUniform ? 0 : 8, 1, 0);
                    m_encoder->SetDstSubReg(j);
                    m_encoder->SetDstRegion(srcUniform ? 1 : 4);
                    m_encoder->Copy(n == 0 ? DstHB : DstLB, SrcB);
                    m_encoder->Push();
                }
            }
        }

        // Now, mov DstH and DstL to Dst
        CVariable* DstD  = m_currShader->GetNewAlias(Dst, ISA_TYPE_UD, 0, 0);

        // When dst is uniform, dst does not cross 2 GRFs, split isn't needed.
        if( split && !dstUniform)
        {
            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            m_encoder->SetMask(EMASK_Q1);
            m_encoder->SetDstRegion(2);
            m_encoder->Copy(DstD, DstL);
            m_encoder->Push();

            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            m_encoder->SetMask(EMASK_Q2);
            m_encoder->SetSrcSubReg(0, srcUniform ? 0 : 8);
            m_encoder->SetDstSubReg(16);
            m_encoder->SetDstRegion(2);
            m_encoder->Copy(DstD, DstL);
            m_encoder->Push();

            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            m_encoder->SetMask(EMASK_Q1);
            m_encoder->SetDstSubReg(1);
            m_encoder->SetDstRegion(2);
            m_encoder->Copy(DstD, DstH);
            m_encoder->Push();

            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            m_encoder->SetMask(EMASK_Q2);
            m_encoder->SetSrcSubReg(0, srcUniform ? 0 : 8);
            m_encoder->SetDstSubReg(17);
            m_encoder->SetDstRegion(2);
            m_encoder->Copy(DstD, DstH);
            m_encoder->Push();
        }
        else
        {
            m_encoder->SetDstRegion(dstUniform ? 1 : 2);
            m_encoder->Copy(DstD, DstL);
            m_encoder->Push();
            m_encoder->SetDstSubReg(1);
            m_encoder->SetDstRegion(dstUniform ? 1 : 2);
            m_encoder->Copy(DstD, DstH);
            m_encoder->Push();
        }
    }
    else
    {
        assert(0 && "Unsupported type for llvm.bswap!");
        return;
    }
}

void EmitPass::setPredicateForDiscard()
{
    if (m_currShader->GetShaderType() == ShaderType::PIXEL_SHADER)
    {
        CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);
        if (psProgram->HasDiscard() && psProgram->GetDiscardPixelMask())
        {
            m_encoder->SetPredicate(psProgram->GetDiscardPixelMask());
            m_encoder->SetInversePredicate(true);
        }
    }
}

void EmitPass::ForceDMask(bool createJmpForDiscard)
{
    if (createJmpForDiscard &&
        m_currShader->GetShaderType() == ShaderType::PIXEL_SHADER)
    {
        CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);
        if (psProgram->HasDiscard() && psProgram->GetDiscardPixelMask())
        {
            m_labelForDMaskJmp = m_encoder->GetNewLabelID();
            m_encoder->Jump(psProgram->GetDiscardPixelMask(),
                m_labelForDMaskJmp);
            m_encoder->Push();
        }
    }

    if (m_pattern->NeedVMask())
    {
        m_encoder->SetVectorMask(false);
    }
}

void EmitPass::ResetVMask(bool createJmpForDiscard)
{
    if (m_pattern->NeedVMask())
    {
        m_encoder->SetVectorMask(true);
    }

    if (createJmpForDiscard &&
        m_currShader->GetShaderType() == ShaderType::PIXEL_SHADER)
    {
        CPixelShader* psProgram = static_cast<CPixelShader*>(m_currShader);
        if (psProgram->HasDiscard() && psProgram->GetDiscardPixelMask())
        {
            m_encoder->Label(m_labelForDMaskJmp);
            m_encoder->Push();
        }
    }
}

void EmitPass::EmitDebugInfo(bool finalize)
{
    unsigned int dbgSize = 0;
    void *dbgInfo = nullptr;
    void *buffer = nullptr;

    IF_DEBUG_INFO(m_pDebugEmitter->Finalize(buffer, dbgSize, finalize);)

    if (dbgSize != 0)
    {
        assert(buffer != nullptr && "Failed to generate VISA debug info");

        dbgInfo = IGC::aligned_malloc(dbgSize, sizeof(void*));

        memcpy_s(dbgInfo, dbgSize, buffer, dbgSize);

        IF_DEBUG_INFO(m_pDebugEmitter->Free(buffer);)

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

void EmitPass::emitGetBufferPtr(GenIntrinsicInst* inst)
{
    Value *buf_idxv = inst->getOperand(0);
    Value *bufTyVal = inst->getOperand(1);
    assert(isa<ConstantInt>(bufTyVal));
    BufferType bufType = (BufferType)(cast<ConstantInt>(bufTyVal)->getZExtValue());

    uint bti = 0;
    switch (bufType)
    {
    case UAV:
        bti = m_currShader->m_pBtiLayout->GetUavIndex(0);
        break;
    case CONSTANT_BUFFER:
        bti = m_currShader->m_pBtiLayout->GetConstantBufferIndex(0);
        break;
    case RESOURCE:
        bti = m_currShader->m_pBtiLayout->GetTextureIndex(0);
        break;
    case RENDER_TARGET:
        bti = m_currShader->m_pBtiLayout->GetRenderTargetIndex(0);
        break;
    case SAMPLER:
        bti = 0;
        break;
    default:
        assert(0 && "unexpect buffer type for GetBufferPtr");
        break;
    }
    CVariable* indexCVar = GetSymbol(buf_idxv);
    
    if (bti)
    {
        CVariable* btiCVar = m_currShader->ImmToVariable(bti, ISA_TYPE_UD);
        m_encoder->Add(m_destination, indexCVar, btiCVar);
    }
    else
    {
        m_encoder->Copy(m_destination, indexCVar);
    }
    m_encoder->Push();

    // Set BTI; BTI equal zero is also a valid value.
    bool directIdx = (llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(0))) ? true : false;
    m_currShader->SetBindingTableEntryCountAndBitmap(directIdx, bti);
}

ResourceDescriptor EmitPass::GetResourceVariable(Value* resourcePtr)
{
    ResourceDescriptor resource;
    BufferType bufType = BUFFER_TYPE_UNKNOWN;
    if (isa<GenIntrinsicInst>(resourcePtr) &&
        cast<GenIntrinsicInst>(resourcePtr)->getIntrinsicID() == GenISAIntrinsic::GenISA_GetBufferPtr)
    {
        // from GetBufferPtr
        Value *bufTyVal = cast<GenIntrinsicInst>(resourcePtr)->getOperand(1);
        assert(isa<ConstantInt>(bufTyVal));
        bufType = (BufferType)(cast<ConstantInt>(bufTyVal)->getZExtValue());
        resource.m_resource = GetSymbol(resourcePtr);
    }
    else
    {
        uint as = resourcePtr->getType()->getPointerAddressSpace();
        uint bufferIndex;
        bool directIndexing;
        bufType = DecodeAS4GFXResource(as, directIndexing, bufferIndex);
        if(IsBindless(bufType) || !directIndexing)
        {
            if (isa<IntToPtrInst>(resourcePtr))
            {
                IntToPtrInst *i2p = dyn_cast<IntToPtrInst>(resourcePtr);
                resource.m_resource = GetSymbol(i2p->getOperand(0));
            }
            else
            {
                resource.m_resource = GetSymbol(resourcePtr);
            }

            if (!directIndexing)
            {
                m_currShader->SetBindingTableEntryCountAndBitmap(false);
            }
        }
        else
        {
            uint bti = 0;
            switch (bufType)
            {
            case UAV:
                bti = m_currShader->m_pBtiLayout->GetUavIndex(bufferIndex);
                break;
            case CONSTANT_BUFFER:
                bti = m_currShader->m_pBtiLayout->GetConstantBufferIndex(bufferIndex);
                m_currShader->ConstantBufferAccesed(bufferIndex);  // return to driver
                break;
            case RESOURCE:
                bti = m_currShader->m_pBtiLayout->GetTextureIndex(bufferIndex);
                break;
            case RENDER_TARGET:
                assert(m_currShader->GetShaderType() == ShaderType::PIXEL_SHADER);
                bti = m_currShader->m_pBtiLayout->GetRenderTargetIndex(bufferIndex);
                break;
            case SLM:
                bti = 254;  // \todo, remove hard-coding
                break;
            default:
                bti = m_currShader->m_pBtiLayout->GetStatelessBindingTableIndex();
                break;
            }
            resource.m_resource = m_currShader->ImmToVariable(bti, ISA_TYPE_UD);
            m_currShader->SetBindingTableEntryCountAndBitmap(directIndexing, bti);
        }
    }

    if (IsBindless(bufType))
    {
        resource.m_surfaceType = ESURFACE_BINDLESS;
    }
    else if(bufType == SLM)
    {
        resource.m_surfaceType = ESURFACE_SLM;
    }
    else if (bufType == CONSTANT_BUFFER || bufType == UAV ||
            bufType == RESOURCE || bufType == RENDER_TARGET)
    {
        resource.m_surfaceType = ESURFACE_NORMAL;
    }
    else
    {
        resource.m_surfaceType = ESURFACE_STATELESS;
    }
    return resource;
}

SamplerDescriptor EmitPass::GetSamplerVariable(Value* sampleOp)
{
    SamplerDescriptor sampler;
    unsigned int samplerIdx;

    if (sampleOp->getType()->isPointerTy())
    {
        BufferType sampType = BUFFER_TYPE_UNKNOWN;

        if (isa<GenIntrinsicInst>(sampleOp)) // from GetBufferPtr
        {
            Value *bufTyVal = cast<GenIntrinsicInst>(sampleOp)->getOperand(1);
            assert(isa<ConstantInt>(bufTyVal));
            sampType = (BufferType)(cast<ConstantInt>(bufTyVal)->getZExtValue());
            sampler.m_sampler = GetSymbol(sampleOp);
            assert(sampType == SAMPLER);
            sampler.m_samplerType = ESAMPLER_NORMAL;
        }
        else
        {
            bool isBindless;
            bool directIdx;

            sampType = DecodeAS4GFXResource(
                sampleOp->getType()->getPointerAddressSpace(),
                directIdx, samplerIdx);
            isBindless = (sampType == BINDLESS_SAMPLER);
            sampler.m_samplerType =
                isBindless ? ESAMPLER_BINDLESS : ESAMPLER_NORMAL;

            if (isBindless || !directIdx)
            {
                sampler.m_sampler = GetSymbol(sampleOp);
            }
            else
            {
                sampler.m_sampler = m_currShader->ImmToVariable(
                    samplerIdx, ISA_TYPE_UD);
            }
        }
    }
    else
    {
        if (isa<ConstantInt>(sampleOp))
        {
            samplerIdx = int_cast<unsigned>(GetImmediateVal(sampleOp));
            sampler.m_sampler = m_currShader->ImmToVariable(
                samplerIdx, ISA_TYPE_UD);
        }
        else
        {
            //Indexable sampler, when sampler index is not immediate
            sampler.m_sampler = GetSymbol(sampleOp);
        }
    }

    return sampler;
}

bool EmitPass::ResourceLoopHeader(
    ResourceDescriptor& resource,
    CVariable*& flag,
    uint& label)
{
    SamplerDescriptor sampler;
    return ResourceLoopHeader(resource, sampler, flag, label);
}

// Insert loop header to handle none-uniform resource and sampler
// This generates sub-optimal code for SIMD32, this can be revisited if we need better code generation
bool EmitPass::ResourceLoopHeader(
    ResourceDescriptor& resource,
    SamplerDescriptor& sampler,
    CVariable*& flag,
    uint& label)
{
    if(resource.m_surfaceType != ESURFACE_BINDLESS && resource.m_surfaceType != ESURFACE_NORMAL)
    {
        // Loop only needed for access with surface state
        return false;
    }
    bool uniformResource = resource.m_resource == nullptr || resource.m_resource->IsUniform();
    bool uniformSampler = sampler.m_sampler == nullptr || sampler.m_sampler->IsUniform();
    if(uniformResource && uniformSampler)
    {
        return false;
    }
    CVariable* resourceFlag = nullptr;
    CVariable* samplerFlag = nullptr;
    CVariable* offset = nullptr;
    label = m_encoder->GetNewLabelID();
    m_encoder->Label(label);
    m_encoder->Push();
    if(!uniformResource)
    {
        ResourceDescriptor uniformResource;
        resourceFlag = m_currShader->GetNewVariable(numLanes(m_SimdMode), ISA_TYPE_BOOL, EALIGN_BYTE);
        uniformResource.m_surfaceType = resource.m_surfaceType;
        uniformResource.m_resource = UniformCopy(resource.m_resource, offset);
        m_encoder->Cmp(EPREDICATE_EQ, resourceFlag, uniformResource.m_resource, resource.m_resource);
        m_encoder->Push();
        resource = uniformResource;
    }
    if(!uniformSampler)
    {
        SamplerDescriptor uniformSampler;
        samplerFlag = m_currShader->GetNewVariable(numLanes(m_SimdMode), ISA_TYPE_BOOL, EALIGN_BYTE);
        uniformSampler.m_samplerType = sampler.m_samplerType;
        uniformSampler.m_sampler = UniformCopy(sampler.m_sampler, offset);
        m_encoder->Cmp(EPREDICATE_EQ, samplerFlag, uniformSampler.m_sampler, sampler.m_sampler);
        m_encoder->Push();
        sampler = uniformSampler;
    }
    if(resourceFlag && samplerFlag)
    {
        flag = m_currShader->GetNewVariable(numLanes(m_SimdMode), ISA_TYPE_BOOL, EALIGN_BYTE);
        m_encoder->And(flag, resourceFlag, samplerFlag);
        m_encoder->Push();
    }
    else
    {
        flag = resourceFlag != nullptr ? resourceFlag : samplerFlag;
    }
    if(m_SimdMode == SIMDMode::SIMD32)
    {
        // For SIMD32 need to initialize to 1 the other half of the flag
        m_encoder->SetSecondHalf(!m_encoder->IsSecondHalf());
        m_encoder->Cmp(EPREDICATE_EQ, flag, m_currShader->GetR0(), m_currShader->GetR0());
        m_encoder->Push();
        m_encoder->SetSecondHalf(!m_encoder->IsSecondHalf());
    }
    return true;
}

void EmitPass::ResourceLoop(bool needLoop, CVariable* flag, uint label)
{
    if(needLoop)
    {
        m_encoder->SetInversePredicate(true);
        m_encoder->Jump(flag, label);
        m_encoder->Push();
    }
}

void EmitPass::emitStateRegID(uint64_t and_imm, uint64_t shr_imm)
{
    m_encoder->And(m_destination, m_currShader->GetSR0(), m_currShader->ImmToVariable(and_imm, ISA_TYPE_UD));
    m_encoder->Shr(m_destination, m_destination, m_currShader->ImmToVariable(shr_imm, ISA_TYPE_UD));
    m_encoder->Push();
}

void EmitPass::emitMulAdd16(Instruction* I, const SSource Sources[2], const DstModifier &DstMod)
{
    CVariable *LVar = GetSrcVariable(Sources[0]);
    CVariable *RVar = GetSrcVariable(Sources[1]);
    VISA_Type LTy = LVar->GetType();
    VISA_Type RTy = RVar->GetType();

    // Use SetSourceModifiers() to set subReg correctly.
    SetSourceModifiers(0, Sources[0]);
    SetSourceModifiers(1, Sources[1]);
    if (!LVar->IsUniform() && (!Sources[0].region_set) &&
        (LTy == ISA_TYPE_W || LTy == ISA_TYPE_UW))
    {
        m_encoder->SetSrcRegion(0, 16, 8, 2);
    }
    if (!RVar->IsUniform() && (!Sources[1].region_set) &&
        (RTy == ISA_TYPE_W || RTy == ISA_TYPE_UW))
    {
        m_encoder->SetSrcRegion(1, 16, 8, 2);
    }

    unsigned opc = I->getOpcode();
    if (opc == Instruction::Mul) {
        m_encoder->Mul(m_destination, LVar, RVar);
    }
    else if (opc == Instruction::Sub) {
        e_modifier mod = CombineModifier(EMOD_NEG, Sources[1].mod);
        m_encoder->SetSrcModifier(1, mod); // override modifier
        m_encoder->Add(m_destination, LVar, RVar);
    }
    else {
        assert(I->getOpcode() == Instruction::Add && "Unknown Opcode.");
        m_encoder->Add(m_destination, LVar, RVar);
    }
    m_encoder->Push();
}

void EmitPass::emitWaveBallot(llvm::GenIntrinsicInst* inst)
{
    CVariable *destination = m_destination;
    if (!m_destination->IsUniform())
    {
        CVariable *pDst = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_GRF, true);
        destination = pDst;
    }

    bool uniform_active_lane = false;
    if (ConstantInt *pConst = dyn_cast<ConstantInt>(inst->getOperand(0)))
    {
        if (pConst->getZExtValue() == 1)
            uniform_active_lane = true;
    }

    /*
    (W)     mov (1|M0) r0.0<1>:ud sr0.2.0<0;1,0>:ud 
    (W)     and (1|M0)   r0.0<1>:ud r0.0<0;1,0>:ud ce0.0<0;1,0>:ud
    (W)     and (1|M0)   r1.0<1>:ud r0.0<0;1,0>:ud f0.0<0;1,0>:uw
    */

    // (W)     mov (1|M0) r0.0<1>:ud sr0.2.0<0;1,0>:ud 
    // (W)     and (1|M0)   r0.0<1>:ud r0.0<0;1,0>:ud ce0.0<0;1,0>:ud
    m_encoder->Copy(destination, GetExecutionMask());
    m_encoder->Push();

    if (!uniform_active_lane)
    {
        // (W)     and (1|M0)   r1.0:ud r0.0<0;1;0>:ud f0.0:uw
        CVariable *f0 = GetSymbol(inst->getOperand(0));
        CVariable* vf0 = m_currShader->GetNewVariable(
            1, ISA_TYPE_UD, EALIGN_GRF, true);
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
        m_encoder->BoolToInt(vf0, f0);
        m_encoder->Push();

        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
        m_encoder->And(destination, destination, vf0);
        m_encoder->Push();
    }

    if (!m_destination->IsUniform())
    {
        m_encoder->Copy(m_destination, destination);
        m_encoder->Push();
    }
}

static void GetReductionOp(WaveOps op, Type* opndTy, uint64_t& identity, e_opcode& opcode, VISA_Type& type)
{
    auto getISAType = [](Type* ty, bool isSigned = true)
    {
        if (ty->isHalfTy())
        {
            return ISA_TYPE_HF;
        }
        if (ty->isFloatTy())
        {
            return ISA_TYPE_F;
        }
        if (ty->isDoubleTy())
        {
            return ISA_TYPE_DF;
        }
        assert(ty->isIntegerTy() && "expect integer type");
        auto width = dyn_cast<IntegerType>(ty)->getBitWidth();
        if (isSigned)
        {
            return width == 64 ? ISA_TYPE_Q : (width == 16 ? ISA_TYPE_W : ISA_TYPE_D);
        }
        else
        {
            return width == 64 ? ISA_TYPE_UQ : (width == 16 ? ISA_TYPE_UW : ISA_TYPE_UD);
        }
    };
    auto getMaxVal = [](VISA_Type ty) -> uint64_t 
    {
        switch (ty)
        {
        case ISA_TYPE_D:
            return std::numeric_limits<int>::max();
        case ISA_TYPE_UD:
            return std::numeric_limits<uint32_t>::max();
        case ISA_TYPE_W:
            return std::numeric_limits<int16_t>::max();
        case ISA_TYPE_UW:
            return std::numeric_limits<uint16_t>::max();
        case ISA_TYPE_Q:
            return std::numeric_limits<int64_t>::max();
        case ISA_TYPE_UQ:
            return std::numeric_limits<uint64_t>::max();
        default:
            assert(false && "unexpected visa type");
            return std::numeric_limits<int>::max();
        }
    };
    auto getMinVal = [](VISA_Type ty) -> uint64_t
    {
        switch (ty)
        {
        case ISA_TYPE_D:
            return std::numeric_limits<int>::min();
        case ISA_TYPE_UD:
            return std::numeric_limits<uint32_t>::min();
        case ISA_TYPE_W:
            return std::numeric_limits<int16_t>::min();
        case ISA_TYPE_UW:
            return std::numeric_limits<uint16_t>::min();
        case ISA_TYPE_Q:
            return std::numeric_limits<int64_t>::min();
        case ISA_TYPE_UQ:
            return std::numeric_limits<uint64_t>::min();
        default:
            assert(false && "unexpected visa type");
            return std::numeric_limits<int>::min();
        }
    };

    switch(op)
    {
    case WaveOps::SUM:
        identity = 0;
        opcode = EOPCODE_ADD;
        type = getISAType(opndTy);
        break;
    case WaveOps::PROD:
        identity = 1;
        opcode = EOPCODE_MUL;
        type = getISAType(opndTy);
        break;
    case WaveOps::UMAX:
        opcode = EOPCODE_MAX;
        type = getISAType(opndTy, false);
        identity = getMinVal(type);
        break;
    case WaveOps::UMIN:
        opcode = EOPCODE_MIN;
        type = getISAType(opndTy, false);
        identity = getMaxVal(type);
        break;
    case WaveOps::IMAX:
        opcode = EOPCODE_MAX;
        type = getISAType(opndTy);
        identity = getMinVal(type);
        break;
    case WaveOps::IMIN:
        opcode = EOPCODE_MIN;
        type = getISAType(opndTy);
        identity = getMaxVal(type);
        break;
    case WaveOps::OR:
        identity = 0;
        opcode = EOPCODE_OR;
        type = getISAType(opndTy, false);
        break;
    case WaveOps::XOR:
        identity = 0;
        opcode = EOPCODE_XOR;
        type = getISAType(opndTy, false);
        break;
    case WaveOps::AND:
        opcode = EOPCODE_AND;
        type = getISAType(opndTy, false);
        identity = dyn_cast<IntegerType>(opndTy)->getBitMask();
        break;
    case WaveOps::FSUM:
        opcode = EOPCODE_ADD;
        type = getISAType(opndTy);
        identity = 0;
        break;
    case WaveOps::FPROD:
        opcode = EOPCODE_MUL;
        type = getISAType(opndTy);
        identity = getFPOne(type);
        break;
    case WaveOps::FMIN:
        opcode = EOPCODE_MIN;
        type = getISAType(opndTy);
        identity = dyn_cast<ConstantFP>(ConstantFP::getInfinity(opndTy))->getValueAPF().bitcastToAPInt().getZExtValue();
        break;
    case WaveOps::FMAX:
        opcode = EOPCODE_MAX;
        type = getISAType(opndTy);
        identity = dyn_cast<ConstantFP>(ConstantFP::getInfinity(opndTy, true))->getValueAPF().bitcastToAPInt().getZExtValue();
        break;
    default:
        assert(0);
    }
}

void EmitPass::emitWavePrefix(llvm::GenIntrinsicInst* inst, bool isQuad)
{
    WaveOps op = static_cast<WaveOps>(cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue());
    bool isInclusiveScan = cast<llvm::ConstantInt>(inst->getOperand(2))->getZExtValue() != 0;
    VISA_Type type;
    e_opcode opCode;
    uint64_t identity = 0;
    GetReductionOp(op, inst->getOperand(0)->getType(), identity, opCode, type);
    CVariable* src = GetSymbol(inst->getOperand(0));
    CVariable *dst[2] = { nullptr, nullptr };
    emitPreOrPostFixOp(opCode, identity, type, false, src, dst, !isInclusiveScan, isQuad);
    m_encoder->Copy(m_destination, dst[0]);
    if (m_currShader->m_dispatchSize == SIMDMode::SIMD32)
    {
        m_encoder->SetSecondHalf(true);
        m_encoder->Copy(m_destination, dst[1]);
    }
    m_encoder->Push();
}

void EmitPass::emitWaveAll(llvm::GenIntrinsicInst* inst)
{
    CVariable* src = GetSymbol(inst->getOperand(0));
    WaveOps op = static_cast<WaveOps>(cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue());
    VISA_Type type;
    e_opcode opCode;
    uint64_t identity = 0;
    GetReductionOp(op, inst->getOperand(0)->getType(), identity, opCode, type);
    CVariable *dst = m_destination;
    emitReductionAll(opCode, identity, type, false, src, dst);
}


