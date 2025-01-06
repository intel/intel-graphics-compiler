/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IGC/common/StringMacros.hpp"
#include "EmitVISAPass.hpp"
#include "OpenCLKernelCodeGen.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/Optimizer/OpenCLPasses/NamedBarriers/NamedBarriersResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/LSCFuncs/LSCFuncsResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/Decompose2DBlockFuncs/Decompose2DBlockFuncs.hpp"
#include "Compiler/CISACodeGen/GenerateFrequencyData.hpp"
#include "AdaptorCommon/RayTracing/RTStackFormat.h"
#include "DeSSA.hpp"
#include "messageEncoding.hpp"
#include "PayloadMapping.hpp"
#include "VectorProcess.hpp"
#include "ShaderCodeGen.hpp"
#include "MemOpt.h"           // helper functions related struct value.
#include "common/debug/Dump.hpp"
#include "common/debug/Dump.hpp"
#include "common/igc_regkeys.hpp"
#include "common/Stats.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/DebugInfo/ScalarVISAModule.h"
#include "DebugInfo/VISAIDebugEmitter.hpp"
#include "DebugInfo/EmitterOpts.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/IR/AssemblyAnnotationWriter.h"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"
#include "Compiler/CISACodeGen/LoopCountAnalysis.hpp"

#include <fstream>

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;
using namespace std;

char EmitPass::ID = 0;

/// Divide N into multiple of M (must be power of two), and the remaining into M/2,
/// M/4, ..., 1. Each sequence takes two elements in execsizeSeq, in which first
/// one has execsize, and the second one the starting offset.
/// For example with M = 16, N = 47,
///  {16, 0}, {16, 16}, {8, 32}, {4, 40}, {2, 44} {1, 45}
static void splitIntoPowerOfTwo(SmallVector<uint32_t, 16>& execsizeSeq, uint32_t N,  uint32_t M)
{
    // Max execution size is 16.
    int n = (int)N / (int)M;
    uint32_t offset = 0;
    for (int i = 0; i < n; ++i) {
        execsizeSeq.push_back(16);
        execsizeSeq.push_back(offset);
        offset += 16;
    }

    int m = (int)(N % M);
    for (uint32_t s = M/2; m > 0; s = s / 2)
    {
        if (m >= (int)s)
        {
            execsizeSeq.push_back(s);
            execsizeSeq.push_back(offset);
            offset += s;
            m -= s;
        }
    }
}

namespace IGC
{
    class VisaIdAnnotator : public llvm::AssemblyAnnotationWriter
    {
        DenseMap<const Value*, uint32_t> m_rootToVISAId;
        DenseMap<const BasicBlock*, uint32_t> m_blockId;

    public:
        VisaIdAnnotator() {}

        void emitBasicBlockStartAnnot(const BasicBlock* BB, formatted_raw_ostream& OS) override
        {
            OS << "; BB";
            if (m_blockId.count(BB)) {
                OS << m_blockId[BB] << " ";
            }
            OS << ":\n";
        }

        void printInfoComment(const Value& V, formatted_raw_ostream& OS) override
        {
            if (m_rootToVISAId.count(&V))
                OS << "\t\t; visa id: " << m_rootToVISAId[&V];
        }

        void trackVisaId(const Instruction* I, uint32_t vid) { m_rootToVISAId[I] = vid; }
        void trackBlockId(const BasicBlock* BB, uint32_t bbid) { m_blockId[BB] = bbid; }
    };

// FIXME: This is a WA used for opt to test EmitPass only. Maybe consider to
// move KernelShaderMap to CodeGenContext.
static CShaderProgram::KernelShaderMap KernelShaderMap;

static cl::opt<SIMDMode>
SimdMode("simd-mode",
         cl::desc("Simd mode used for visa codegen"),
         cl::values(clEnumValN(SIMDMode::UNKNOWN, "unknown", "unknown simd mode"),
                    clEnumValN(SIMDMode::SIMD1,   "1",       "simd 1"),
                    clEnumValN(SIMDMode::SIMD2,   "2",       "simd 2"),
                    clEnumValN(SIMDMode::SIMD4,   "4",       "simd 4"),
                    clEnumValN(SIMDMode::SIMD8,   "8",       "simd 8"),
                    clEnumValN(SIMDMode::SIMD16,  "16",      "simd 16"),
                    clEnumValN(SIMDMode::SIMD32,  "32",      "simd 32")),
         cl::init(SIMDMode::SIMD32),
         cl::Hidden);

static cl::opt<bool>
AbortOnSpill("abort-on-spill",
             cl::desc("Abort on spill"),
             cl::init(false),
             cl::Hidden);

static cl::opt<ShaderDispatchMode>
DispatchMode("dispatch-mode",
             cl::desc("Shader dispatch mode"),
             cl::values(clEnumValN(ShaderDispatchMode::NOT_APPLICABLE, "na",          "dispatch mode not applicable"),
                        clEnumValN(ShaderDispatchMode::SINGLE_PATCH,   "single",      "single-dispatch mode"),
                        clEnumValN(ShaderDispatchMode::DUAL_PATCH,     "dual",        "dual-dispatch mode"),
                        clEnumValN(ShaderDispatchMode::EIGHT_PATCH,    "eight",       "eight-dispatch mode"),
                        clEnumValN(ShaderDispatchMode::DUAL_SIMD8,     "dual-simd8",  "dual-simd8-dispatch mode")),
             cl::init(ShaderDispatchMode::NOT_APPLICABLE),
             cl::Hidden);

#define PASS_FLAG "igc-emit-visa"
#define PASS_DESC "vISA finalizer invocation"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(EmitPass, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(LiveVarsAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenPatternMatch)
IGC_INITIALIZE_PASS_DEPENDENCY(DeSSA)
IGC_INITIALIZE_PASS_DEPENDENCY(BlockCoalescing)
IGC_INITIALIZE_PASS_DEPENDENCY(CoalescingEngine)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(Simd32ProfitabilityAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(VariableReuseAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(CastToGASInfo)
IGC_INITIALIZE_PASS_DEPENDENCY(ResourceLoopAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(CollectLoopCount)
IGC_INITIALIZE_PASS_END(EmitPass, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
}

// Note that the default ctor is intended to be used for opt testing only.
EmitPass::EmitPass() : EmitPass(KernelShaderMap, SimdMode, AbortOnSpill, DispatchMode) {
    // Call LoadRegistryKeys to set up the implied regkeys for testing purpose.
    LoadRegistryKeys();
}

EmitPass::EmitPass(CShaderProgram::KernelShaderMap& shaders, SIMDMode mode, bool canAbortOnSpill, ShaderDispatchMode shaderMode, PSSignature* pSignature)
    : FunctionPass(ID),
    m_SimdMode(mode),
    m_ShaderDispatchMode(shaderMode),
    m_shaders(shaders),
    m_currShader(nullptr),
    m_encoder(nullptr),
    m_canAbortOnSpill(canAbortOnSpill),
    m_roundingMode_FP(ERoundingMode::ROUND_TO_NEAREST_EVEN),
    m_roundingMode_FPCvtInt(ERoundingMode::ROUND_TO_ZERO),
    m_pSignature(pSignature),
    m_isDuplicate(false)
{
    initializeEmitPassPass(*PassRegistry::getPassRegistry());
}

EmitPass::~EmitPass()
{
}

// Switch to payload section
// When switching to payload section, the code redirects vKernel pointing to the payload section
// m_destination (LiveOut of interploation) will be allocated before compiling the kernel.
void EmitPass::ContextSwitchPayloadSection(bool first)
{
    if (m_encoder->IsCodePatchCandidate())
    {
        if (first)
        {
            m_tmpDest = m_destination;
        }
        m_isDuplicate = first ? m_currShader->AppendPayloadSetup(m_destination) : false;
        // When duplication happens, multiple instructions in divergent branches write to the same VR.
        if (m_isDuplicate)
        {
            auto uniformSIMDMode = m_currShader->m_Platform->getMinDispatchMode();
            CVariable* src = m_destination;
            uint16_t size = m_destination->IsUniform() ? numLanes(uniformSIMDMode) :
                numLanes(m_currShader->m_SIMDSize);
            CVariable* newSource = m_currShader->GetNewVariable(
                    size,
                    src->GetType(),
                    EALIGN_GRF,
                    m_destination->IsUniform(),
                    src->getName());
            m_currShader->AppendPayloadSetup(newSource);
            m_destination = newSource;
        }
        m_encoder->SetPayloadSectionAsPrimary();
    }
}

void EmitPass::ContextSwitchShaderBody(bool last)
{
    if (m_encoder->IsCodePatchCandidate())
    {
        m_encoder->SetPayloadSectionAsSecondary();
        if (last && m_isDuplicate)
        {
            m_encoder->Copy(m_tmpDest, m_destination);
            m_encoder->Push();
            m_destination = m_tmpDest;
        }
    }
}

bool EmitPass::isHalfGRFReturn(CVariable* dst, SIMDMode simdMode)
{
    auto typeSize = CEncoder::GetCISADataTypeSize(dst->GetType());
    return simdMode == m_currShader->m_Platform->getMinDispatchMode() &&
        typeSize == 2 && !dst->isUnpacked();
}

static bool DefReachUseWithinLevel(llvm::Value* def, const llvm::Instruction* use, uint level)
{
    if (level == 0 || !def || !use)
        return false;
    for (auto useIter = def->user_begin(), E = def->user_end(); useIter != E; ++useIter)
    {
        llvm::Instruction* useInst = dyn_cast<llvm::Instruction>(*useIter);
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

bool EmitPass::IsNoMaskAllowed(Instruction* inst)
{
    if (auto* I = dyn_cast<LoadInst>(inst))
    {
        if (IGC_IS_FLAG_ENABLED(UseVMaskPredicateForLoads) && shouldGenerateLSC(I))
            return true;

        BufferType bufType = DecodeBufferType(I->getPointerAddressSpace());
        return I->getPointerAddressSpace() != ADDRESS_SPACE_PRIVATE &&
               I->getPointerAddressSpace() != ADDRESS_SPACE_GLOBAL &&
               I->getPointerAddressSpace() != ADDRESS_SPACE_CONSTANT &&
               !(bufType == CONSTANT_BUFFER && m_currShader->GetContext()->getModuleMetaData()->compOpt.WaDisableSubspanUseNoMaskForCB);
    }

    return true;
}

bool EmitPass::IsSubspanDestination(Instruction* inst)
{
    return m_pattern->IsSubspanUse(inst) && IsNoMaskAllowed(inst) &&
           (!m_pattern->IsSourceOfSample(inst) ||
            (m_pattern->IsSourceOfSample(inst) && m_pCtx->getModule()->getNamedMetadata(NAMED_METADATA_COARSE_PHASE) != nullptr) ||
            (m_pattern->IsSourceOfSample(inst) && !m_pattern->NeedVMask()) ||
            (m_pattern->IsSourceOfSample(inst) && m_pattern->NeedVMask() && m_pattern->IsSourceOfSampleUnderCF(inst)));
}

uint EmitPass::DecideInstanceAndSlice(const llvm::BasicBlock& blk, SDAG& sdag, bool& slicing)
{
    m_encoder->SetSubSpanDestination(false);
    uint numInstance = m_currShader->m_numberInstance;

    if (m_pCtx->platform.getMinDispatchMode() == SIMDMode::SIMD16)
    {
        slicing = false;
    }
    else
    {
        slicing = (m_SimdMode == SIMDMode::SIMD32);
    }

    bool hasValidDestination = (sdag.m_root->getType()->getTypeID() != llvm::Type::VoidTyID);

    // Disable for struct type destinations
    if (sdag.m_root->getType()->isStructTy())
    {
        hasValidDestination = false;
    }

    if (hasValidDestination)
    {
        m_destination = GetSymbol(sdag.m_root);
        numInstance = m_destination->GetNumberInstance();

        if (IsSubspanDestination(sdag.m_root))
        {
            m_encoder->SetSubSpanDestination(true);
        }

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
        else if (getGRFSize() != 32 && IsSIMDBlockIntrinsic(sdag.m_root))
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
        else if (llvm::GenIntrinsicInst* pIntrinsic = llvm::dyn_cast<llvm::GenIntrinsicInst>(sdag.m_root))
        {
            GenISAIntrinsic::ID id = pIntrinsic->getIntrinsicID();
            if (id == GenISAIntrinsic::GenISA_dpas)
            {
                numInstance = 1;
                slicing = false;
            }
        }
    }
    else
    {
        m_destination = nullptr;
        if (StoreInst * ST = dyn_cast<StoreInst>(sdag.m_root))
        {
            // Limit to OpenCL so far as it has uniform load/store support.
            if (isUniformStoreOCL(ST))
                numInstance = 1;
            slicing = false;
        }
        else if (sdag.m_root->isTerminator())
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
        else if (llvm::GenIntrinsicInst * pIntrinsic = llvm::dyn_cast<llvm::GenIntrinsicInst>(sdag.m_root))
        {
            GenISAIntrinsic::ID id = pIntrinsic->getIntrinsicID();
            if (id == GenISAIntrinsic::GenISA_threadgroupbarrier ||
                id == GenISAIntrinsic::GenISA_memoryfence ||
                id == GenISAIntrinsic::GenISA_flushsampler ||
                id == GenISAIntrinsic::GenISA_typedmemoryfence ||
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
                id == GenISAIntrinsic::GenISA_MediaBlockWrite ||
                id == GenISAIntrinsic::GenISA_eu_thread_pause ||
                id == GenISAIntrinsic::GenISA_simdBlockWrite ||
                id == GenISAIntrinsic::GenISA_simdBlockWriteBindless)
            {
                numInstance = 1;
                slicing = false;
            }
            else if (
                id == GenISAIntrinsic::GenISA_LSCFence ||
                id == GenISAIntrinsic::GenISA_systemmemoryfence ||
                id == GenISAIntrinsic::GenISA_urbfence ||
                id == GenISAIntrinsic::GenISA_threadgroupnamedbarriers_signal ||
                id == GenISAIntrinsic::GenISA_threadgroupnamedbarriers_wait)
            {
                numInstance = 1;
                slicing = false;
            }
        }
    }

    if (CallInst * callInst = dyn_cast<CallInst>(sdag.m_root))
    {
        // Disable slicing for function calls
        Function* F = dyn_cast<Function>(IGCLLVM::getCalledValue(callInst));
        if (!F || F->hasFnAttribute("visaStackCall"))
        {
            numInstance = 1;
            slicing = false;
        }
    }
    return numInstance;
}

bool EmitPass::IsUndefOrZeroImmediate(const Value* value)
{
    if (isUndefOrConstInt0(value))
    {
        return true;
    }

    if (const llvm::ConstantFP* CFP = llvm::dyn_cast<llvm::ConstantFP>(value))
    {
        APInt api = CFP->getValueAPF().bitcastToAPInt();
        if (api.getZExtValue() == 0)
        {
            return true;
        }
    }
    return false;
}

// check all functions inside function group (current compilation unit)
// to find the one with the highest register pressure
unsigned int EmitPass::getMaxRegPressureInFunctionGroup(llvm::Function* F, const IGCMD::MetaDataUtils* MDU)
{
    unsigned int MaxRegPressure = 0;
    if(!m_FGA)
    {
        auto FuncMD = MDU->getFunctionsInfoItem(F);
        MaxRegPressure = FuncMD->getMaxRegPressure()->getMaxPressure();
        return MaxRegPressure;
    }

    FunctionGroup* Group = m_FGA->getGroup(F);
    if (!Group)
        return 0;

    for (auto it = Group->begin(), ie = Group->end(); it != ie; ++it)
    {
        Function* PtrF = *it;
        if(MDU->findFunctionsInfoItem(PtrF) == MDU->end_FunctionsInfo())
            continue;
        auto FuncMD = MDU->getFunctionsInfoItem(PtrF);
        unsigned int Pressure = FuncMD->getMaxRegPressure()->getMaxPressure();
        MaxRegPressure = std::max(MaxRegPressure, Pressure);
    }
    return MaxRegPressure;
}

bool EmitPass::setCurrentShader(llvm::Function* F)
{
    llvm::Function* Kernel = F;
    if (m_FGA)
    {
        if (!m_FGA->getModule())
        {
            m_FGA->rebuild(F->getParent());
        }
        auto FG = m_FGA->getGroup(F);
        if (!FG)
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
    m_currShader = Iter->second->GetOrCreateShader(m_SimdMode, m_ShaderDispatchMode);

    if (m_CLC && IGC_IS_FLAG_ENABLED(EnableKernelCostInfo)) {

        COpenCLKernel* openCLkernel = static_cast<COpenCLKernel*>(m_currShader);
        std::vector<CollectLoopCount::ArgSym>& loopargs = m_CLC->getloopArgs();
        for (CollectLoopCount::ArgSym argsym : loopargs) {
            zebin::zeInfoKCMArgSym& argSym = openCLkernel->m_kernelCostexpInfo.argsSym.emplace_back();
            argSym.argNo = argsym.argNo;
            argSym.byteOffset = argsym.byteOffset;
            argSym.sizeInBytes = argsym.sizeInBytes;
            argSym.isInDirect = argsym.isInDirect;
        }

        std::vector<CollectLoopCount::LCE>& loopLCE = m_CLC->getLCE();
        for (CollectLoopCount::LCE lce : loopLCE) {
            zebin::zeInfoKCMLoopCountExp& lceInfo = openCLkernel->m_kernelCostexpInfo.loopLCE.emplace_back();
            lceInfo.factor = lce.factor;
            lceInfo.argsym_index = lce.argIndex;
            lceInfo.C = lce.C;
        }
    }
    m_encoder = &(m_currShader->GetEncoder());
    return true;
}

bool EmitPass::isSymbolTableRequired(llvm::Function* F)
{
    bool canOutputSymbolTable = false;
    if (isIntelSymbolTableVoidProgram(F))
    {
        // If the current function is the dummy kernel, we can attach the symbol table to it
        canOutputSymbolTable = true;
    }
    else if (m_pCtx->type != ShaderType::OPENCL_SHADER &&
        F == IGC::getUniqueEntryFunc(m_pCtx->getMetaDataUtils(), m_moduleMD))
    {
        // For non-OCL shaders, we can still output a symbol table for the unique entry function
        canOutputSymbolTable = true;
    }

    if (canOutputSymbolTable)
    {
        // Check has external functions attached to current function group
        if ((m_FGA && m_FGA->getGroup(F) && !m_FGA->getGroup(F)->isSingle()))
        {
            auto FG = m_FGA->getGroup(F);
            for (auto FI = FG->begin(), FE = FG->end(); FI != FE; ++FI)
            {
                if ((*FI)->hasFnAttribute("referenced-indirectly"))
                    return true;
            }
        }
        // Check has global symbols attached
        else if (!m_moduleMD->inlineProgramScopeOffsets.empty())
        {
            for (const auto& it : m_moduleMD->inlineProgramScopeOffsets)
            {
                GlobalVariable* pGlobal = it.first;
                // Export the symbol if global is external/common linkage
                if (m_pCtx->enableTakeGlobalAddress() && (pGlobal->hasCommonLinkage() || pGlobal->hasExternalLinkage()))
                {
                    return true;
                }

                // Remove dead users at this point
                pGlobal->removeDeadConstantUsers();

                // Check if relocation is required by checking uses

                // FIXME: Ideally we should emit symtab for the global if
                // there's an user in both ZEBIN and PT.
                if (m_pCtx->enableZEBinary() && !pGlobal->user_empty())
                    return true;

                for (auto user : pGlobal->users())
                {
                    if (isa<Instruction>(user))
                    {
                        return true;
                    }
                }
            }
        }
        // Check if requiring symbol for imported function calls
        else
        {
            for (auto& FI : F->getParent()->getFunctionList())
            {
                if (FI.isDeclaration() &&
                    FI.hasFnAttribute("referenced-indirectly") &&
                    !FI.use_empty())
                {
                    return true;
                }
            }
        }
    }
    return false;
}

void EmitPass::CreateKernelShaderMap(CodeGenContext* ctx, MetaDataUtils* pMdUtils, llvm::Function& F)
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
                Function* pFunc = i->first;
                // Skip non-kernel functions.
                if (!isEntryFunc(pMdUtils, pFunc))
                    continue;

                if (ctx->m_retryManager.kernelSet.empty() ||
                    ctx->m_retryManager.kernelSet.count(pFunc->getName().str()))
                {
                    m_shaders[pFunc] = new CShaderProgram(ctx, pFunc);
                    COMPILER_SHADER_STATS_INIT(m_shaders[pFunc]->m_shaderStats);
                }
            }
        }
        /* All other shader types */
        else
        {
            for (auto i = pMdUtils->begin_FunctionsInfo(), e = pMdUtils->end_FunctionsInfo(); i != e; ++i)
            {
                Function* pFunc = i->first;
                // Skip non-entry functions.
                if (!isEntryFunc(pMdUtils, pFunc))
                {
                    continue;
                }
                m_shaders[pFunc] = new CShaderProgram(ctx, pFunc);
                COMPILER_SHADER_STATS_INIT(m_shaders[pFunc]->m_shaderStats);
            }
        }
    }
}

bool EmitPass::shouldForceEarlyRecompile(MetaDataUtils *pMdUtils,
                                         llvm::Function *F) {
  // we only skip first compilation stage, if compilation pipeline
  // was configured to start from retry already, otherwise we do nothing
  bool IsFirstStage = m_pCtx->m_retryManager.GetRetryId() == 0;
  if (!isEntryFunc(pMdUtils, F) || IGC_GET_FLAG_VALUE(DisableRecompilation) ||
      !IsFirstStage) {
    return false;
  }
  if (m_currShader->IsRecompilationRequestForced()) {
    return true;
  }
  auto Threshold = IGC_GET_FLAG_VALUE(EarlyRetryRPEThreshold);
  auto GRFPerThread = m_pCtx->getNumGRFPerThread();
  // If we are not in large GRF mode and auto GRF is disabled we can lower the
  // threshold. We also, as a workaround skip lowering the threshold if we have
  // indirect operands in the kernel to avoid cases where recompilaton has
  // higher spill count.
  if (GRFPerThread <= CodeGenContext::DEFAULT_TOTAL_GRF_NUM &&
      !m_pCtx->isAutoGRFSelectionEnabled() &&
      !m_pCtx->m_instrTypes.mayHaveIndirectOperands) {
    Threshold = Threshold / 2;
  }
  auto MaxRegPressure = getMaxRegPressureInFunctionGroup(F, pMdUtils);
  bool PassedThreshold = MaxRegPressure >= Threshold;
  return PassedThreshold;
}

bool EmitPass::runOnFunction(llvm::Function& F)
{
    m_currFuncHasSubroutine = false;

    m_pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo())
    {
        return false;
    }
    m_moduleMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

    const GASInfo& GI = getAnalysis<CastToGASInfo>().getGASInfo();
    m_canGenericPointToPrivate = !GI.isPrivateAllocatedInGlobalMemory() && GI.canGenericPointToPrivate(F);
    m_canGenericPointToLocal = !GI.isNoLocalToGenericOptionEnabled() && GI.canGenericPointToLocal(F);

    CreateKernelShaderMap(m_pCtx, pMdUtils, F);

    m_pCtx->createFunctionIDs();

    m_FGA = getAnalysisIfAvailable<GenXFunctionGroupAnalysis>();

    if ((IsStage1BestPerf(m_pCtx->m_CgFlag, m_pCtx->m_StagingCtx) ||
        IGC_IS_FLAG_ENABLED(ForceBestSIMD)) &&
        m_SimdMode == SIMDMode::SIMD8)
    {
        /* Don't do SIMD8 if SIMD16 has no spill */
        auto Iter = m_shaders.find(&F);
        if (Iter == m_shaders.end())
        {
            return false;
        }

        CShader * simd16Program = Iter->second->GetShader(SIMDMode::SIMD16);
        if (simd16Program &&
            simd16Program->ProgramOutput()->m_programBin != 0 &&
            !m_pCtx->hasSpills(simd16Program->ProgramOutput()->m_scratchSpaceUsedBySpills))
            return false;
    }
    if (IGC_IS_FLAG_ENABLED(EnableKernelCostInfo)) {
        m_CLC = getAnalysisIfAvailable<CollectLoopCount>();
    }

    if (!setCurrentShader(&F))
    {
        return false;
    }

    bool isDummyKernel = IGC::isIntelSymbolTableVoidProgram(&F);
    bool isFuncGroupHead = !m_FGA || m_FGA->isGroupHead(&F);

    // Dummy program is only used for symbol table info, so skip compilation if no symbol table is needed
    if (isDummyKernel && !isSymbolTableRequired(&F) && m_pCtx->type == ShaderType::OPENCL_SHADER)
    {
        return false;
    }

    if (shouldForceEarlyRecompile(pMdUtils, &F))
    {
        // we can't reuse kernelSet because EmitPass assumes that if we have
        // something in kernelSet, it was added by previous compilation stage and passed to this one
        // to compile, we use this set to prepare data structures and we clear it after
        // compilation is done, not propagate it to the next stage
        m_pCtx->m_retryManager.earlyRetryKernelSet.insert(F.getName().str());
        return false;
    }

    // Force SIMD8 on library compilations for non-OCL shaders
    if (m_pCtx->type != ShaderType::OPENCL_SHADER &&
        m_pCtx->getCompilerOption().IsLibraryCompilation &&
        m_SimdMode != SIMDMode::SIMD8)
    {
        return false;
    }

    m_DL = &F.getParent()->getDataLayout();
    m_pattern = &getAnalysis<CodeGenPatternMatch>();
    m_deSSA = &getAnalysis<DeSSA>();
    m_blockCoalescing = &getAnalysis<BlockCoalescing>();
    m_CE = &getAnalysis<CoalescingEngine>();
    m_VRA = &getAnalysis<VariableReuseAnalysis>();
    m_RLA = &getAnalysis<ResourceLoopAnalysis>();

    m_currShader->SetUniformHelper(&getAnalysis<WIAnalysis>());
    m_currShader->SetCodeGenHelper(m_pattern);
    m_currShader->SetDominatorTreeHelper(&getAnalysis<DominatorTreeWrapperPass>().getDomTree());
    m_currShader->SetMetaDataUtils(getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());
    m_currShader->SetShaderSpecificHelper(this);
    m_currShader->SetDataLayout(m_DL);
    m_currShader->SetFunctionGroupAnalysis(m_FGA);
    if (IGC_IS_FLAG_ENABLED(StackOverflowDetection)) {
        if (findStackOverflowDetectionFunction(&F, true)) {
            m_currShader->SetHasStackCalls(true);
        }
    }
    m_currShader->SetPushInfoHelper(&(m_moduleMD->pushInfo));
    m_currShader->SetVariableReuseAnalysis(m_VRA);
    m_currShader->SetResourceLoopAnalysis(m_RLA);
    m_currShader->SetDeSSAHelper(m_deSSA);
    m_currShader->SetEmitPassHelper(this);

    //Add CCtuple root variables.
    if (IGC_IS_FLAG_DISABLED(DisablePayloadCoalescing)) {
        m_currShader->SetCoalescingEngineHelper(m_CE);
    }

    if (IGC_IS_FLAG_ENABLED(DumpUseShorterName))
    {
        Function* entryF = &F;
        if (m_FGA) {
            entryF = m_FGA->getGroupHead(&F);
        }
        m_currShader->setShaderProgramID(m_pCtx->getFunctionID(entryF));
    }

    CShader* prevShader = m_pCtx->m_prevShader;

    if (isFuncGroupHead)
    {
        m_currShader->InitEncoder(m_SimdMode, m_canAbortOnSpill, m_ShaderDispatchMode);
        // Pre-analysis pass to be executed before call to visa builder so we can pass scratch space offset
        m_currShader->PreAnalysisPass();
        if (!m_currShader->CompileSIMDSize(m_SimdMode, *this, F)) {
            if (m_FGA) {
                auto *FG = m_FGA->getGroup(&F);
                if (FG)
                    FG->setSimdModeInvalid(m_SimdMode);
            }
            return false;
        }

        VISAKernel* prevKernel = nullptr;

        if (prevShader &&
            m_currShader->IsPatchablePS() &&
            m_encoder->GetSimdSize() == prevShader->GetEncoder().GetSimdSize() &&
            prevShader->GetEncoder().IsCodePatchCandidate() &&
            prevShader->ProgramOutput()->m_programBin &&
            !m_pCtx->hasSpills(prevShader->ProgramOutput()->m_scratchSpaceUsedBySpills))
        {
            prevKernel = prevShader->GetEncoder().GetVISAKernel();
            m_encoder->SetPayloadEnd(prevShader->GetEncoder().GetPayloadEnd());
        }

        if (IGC_GET_FLAG_VALUE(CodePatch) &&
            ((!m_pCtx->hash.nosHash) || IGC_GET_FLAG_VALUE(CodePatch) > CodePatch_Enable_NoLTO) &&
            m_currShader->IsPatchablePS() &&
            m_SimdMode == SIMDMode::SIMD16 &&
            (m_ShaderDispatchMode != ShaderDispatchMode::NOT_APPLICABLE || prevKernel) &&
            (IGC_GET_FLAG_VALUE(CodePatchLimit) == 0 || 2 <= IGC_GET_FLAG_VALUE(CodePatchLimit)))
        {
            m_encoder->SetIsCodePatchCandidate(true);

            // FIXME: Skip corner cases for now. Remove this later.
            for (uint i = 0; i < m_pattern->m_numBlocks && m_encoder->IsCodePatchCandidate(); i++)
            {
                SBasicBlock& block = m_pattern->m_blocks[i];
                auto I = block.m_dags.rbegin(), E = block.m_dags.rend();
                while (I != E && m_encoder->IsCodePatchCandidate())
                {
                    Instruction* llvmInst = I->m_root;
                    if (llvmInst->getOpcode() == Instruction::Call)
                    {
                        if (GenIntrinsicInst * I = dyn_cast<GenIntrinsicInst>(llvmInst))
                        {
                            switch(I->getIntrinsicID())
                            {
                                case GenISAIntrinsic::GenISA_PullSampleIndexBarys:
                                    {
                                        if (IGC_GET_FLAG_VALUE(CodePatchFilter) & CODE_PATCH_NO_PullSampleIndex) {
                                            m_encoder->SetIsCodePatchCandidate(false);
                                        }
                                    }
                                    break;
                                case GenISAIntrinsic::GenISA_PullSnappedBarys:
                                    {
                                        if (IGC_GET_FLAG_VALUE(CodePatchFilter) & CODE_PATCH_NO_PullSnapped) {
                                            m_encoder->SetIsCodePatchCandidate(false);
                                        }
                                    }
                                    break;
                                case GenISAIntrinsic::GenISA_PullCentroidBarys:
                                    {
                                        if (IGC_GET_FLAG_VALUE(CodePatchFilter) & CODE_PATCH_NO_PullCentroid) {
                                            m_encoder->SetIsCodePatchCandidate(false);
                                        }
                                    }
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                    ++I;
                }
            }
            if ((IGC_GET_FLAG_VALUE(CodePatchFilter) & CODE_PATCH_NO_UNSTABLE_PLATFORM) &&
                (m_pCtx->platform.getPlatformInfo().eProductFamily == IGFX_DG2 ||
                 m_pCtx->platform.getPlatformInfo().eProductFamily == IGFX_ALDERLAKE_P ||
                 m_pCtx->platform.getPlatformInfo().eProductFamily == IGFX_ALDERLAKE_N ||
                 m_pCtx->platform.getPlatformInfo().eProductFamily == IGFX_ALDERLAKE_S ||
                 m_pCtx->platform.getPlatformInfo().eProductFamily == IGFX_METEORLAKE ||
                 m_pCtx->platform.getPlatformInfo().eProductFamily == IGFX_ARROWLAKE)) {
                m_encoder->SetIsCodePatchCandidate(false);
            }
        }
        else
        {
            m_encoder->SetIsCodePatchCandidate(false);
        }

        // Check if the function, or the FG, has inline asm calls.
        // We need this to set the correct builder mode to parse inline asm.
        bool hasInlineAsmCall = m_pCtx->m_instrTypes.hasInlineAsm &&
            m_pCtx->m_DriverInfo.SupportInlineAssembly() &&
            (!m_FGA ? IGC::hasInlineAsmInFunc(F) : m_FGA->getGroup(&F)->hasInlineAsm());

        bool hasAdditionalVisaAsmToLink = false;
        if (m_pCtx->type == ShaderType::OPENCL_SHADER) {
            auto cl_context = static_cast<OpenCLProgramContext*>(m_pCtx);
            if (!cl_context->m_VISAAsmToLink.empty()) {
                hasAdditionalVisaAsmToLink = true;
            }
        }

        uint lowerBoundGRF = 0;
        int numThreadsPerEU = -1;
        if (F.hasFnAttribute("num-thread-per-eu"))
        {
            numThreadsPerEU = std::stoi(
                F.getFnAttribute("num-thread-per-eu").getValueAsString().str());
        }
        // call builder after pre-analysis pass where scratchspace offset to VISA is calculated
        m_encoder->InitEncoder(m_canAbortOnSpill, m_currShader->HasStackCalls(), hasInlineAsmCall, hasAdditionalVisaAsmToLink,
            numThreadsPerEU, lowerBoundGRF, prevKernel);

        if (!m_encoder->IsCodePatchCandidate())
            createVMaskPred(m_vMaskPredForSubplane);

        initDefaultRoundingMode();
        m_currShader->PreCompile();

        if (!isDummyKernel)
        {
            // initialize stack if having stack usage
            bool hasVLA = (m_FGA && m_FGA->getGroup(&F) && m_FGA->getGroup(&F)->hasVariableLengthAlloca()) || F.hasFnAttribute("hasVLA");
            if (m_currShader->HasStackCalls() || hasVLA)
            {
                m_encoder->InitFuncAttribute(&F, true);
                InitializeKernelStack(&F);
            }
        }
        if (m_encoder->IsCodePatchCandidate())
        {
            m_currShader->SplitPayloadFromShader(&F);
        }
        m_currShader->AddPrologue();
    }
    else
    {
        // If kernel function is not compiled for the SIMD size then VISABuilder==nullptr
        if (m_currShader->GetEncoder().GetVISABuilder() == nullptr)
        {
            return false;
        }
        if (!m_currShader->CompileSIMDSize(m_SimdMode, *this, F))
        {
            if (m_FGA) {
                auto *FG = m_FGA->getGroup(&F);
                if (FG)
                    FG->setSimdModeInvalid(m_SimdMode);
            }
            return false;
        }
        m_currShader->BeginFunction(&F);
        if (m_FGA && m_FGA->useStackCall(&F))
        {
            m_encoder->InitFuncAttribute(&F, false);
            emitStackFuncEntry(&F);
        }
    }

    // Only apply WA to OCL shaders with stackcall enabled
    // TODO: Remove this WA once vISA handles the register copy
    bool needKernelArgOverrideWA = isFuncGroupHead && m_currShader->HasStackCalls() && m_currShader->GetShaderType() == ShaderType::OPENCL_SHADER;
    if (needKernelArgOverrideWA)
    {
        // Requires early payload allocation to know the kernel arg offsets
        m_currShader->CacheArgumentsList();
        m_currShader->MapPushedInputs();
        m_currShader->AllocatePayload();

        // This WA copies all kernel args > r26.0 into a temp register when stackcalls are enabled.
        // Since vISA stackcall ABI predefine the argument register to r26.0, if the payload is larger than
        // 26GRFs then doing a stackcall will overwrite the payload registers.
        const int visaStackCallArgRegStart = 26;
        const int64_t maxGRFOffset = visaStackCallArgRegStart * m_currShader->getGRFSize();
        llvm::IRBuilder<> builder(&*F.getEntryBlock().getFirstInsertionPt());
        for (auto& arg : F.args())
        {
            // Skip unused arguments
            if (arg.user_empty()) continue;

            Argument* kernArg = &arg;
            CVariable* kernArgV = m_currShader->GetSymbol(kernArg);
            // Get the allocated payload offset for this kernel arg
            int64_t offset = m_currShader->GetKernelArgOffset(kernArgV);
            // If kernel payload size exceeds maxGRFOffset, we must copy the kernel args into another register.
            if (offset >= maxGRFOffset)
            {
                // Create a dummy instruction using RTV, just so we can use the LLVM replaceAllUsesWith to replace the kernelArg usages.
                Function* pFunc = GenISAIntrinsic::getDeclaration(F.getParent(), GenISAIntrinsic::GenISA_RuntimeValue, kernArg->getType());
                Value* tempCall = builder.CreateCall(pFunc, builder.getInt32(kernArg->getArgNo()), "kernArgCopy");
                kernArg->replaceAllUsesWith(tempCall);

                // Create another CVar to hold the copied kernelArg, and map it to the dummy instruction.
                // When doing vISA codegen, all usages of the dummy instruction will get the value of the copied kernelArg.
                CVariable* copiedArg = m_currShader->GetNewVariable(kernArgV);
                emitCopyAll(copiedArg, kernArgV, kernArg->getType());
                m_currShader->UpdateSymbolMap(tempCall, copiedArg);
                // Temp instruction needs the same uniform analysis attribute as kernel arg
                m_currShader->SetDependency(tempCall, m_currShader->GetDependency(kernArg));
            }
        }
    }

    if (m_moduleMD->NBarrierCnt > 0)
    {
        m_encoder->GetVISAKernel()->AddKernelAttribute(
            "NBarrierCnt", sizeof(m_moduleMD->NBarrierCnt),
            &m_moduleMD->NBarrierCnt);
        m_currShader->m_State.SetBarrierNumber(
            NamedBarriersResolution::AlignNBCnt2BarrierNumber(
                m_moduleMD->NBarrierCnt + 1 /* +1 for TG barrier */));
    }

    if (IGC_IS_FLAG_ENABLED(DumpHasNonKernelArgLdSt)) {
        ModuleMetaData* modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
        FunctionMetaData* funcMD = &modMD->FuncMD[&F];
        if (m_currShader->HasStackCalls() || m_currFuncHasSubroutine) {
            // conservative set the hasNonKernelArgLoad/Store to true
            funcMD->hasNonKernelArgLoad = true;
            funcMD->hasNonKernelArgStore = true;
            funcMD->hasNonKernelArgAtomic = true;
        }
        // then write the result to the shader
        if (m_currShader->GetShaderType() == ShaderType::OPENCL_SHADER) {
            COpenCLKernel* kernel = static_cast<COpenCLKernel*>(m_currShader);
            kernel->m_kernelInfo.m_hasNonKernelArgLoad = funcMD->hasNonKernelArgLoad? 1 : 0;
            kernel->m_kernelInfo.m_hasNonKernelArgStore = funcMD->hasNonKernelArgStore? 1 : 0;
            kernel->m_kernelInfo.m_hasNonKernelArgAtomic = funcMD->hasNonKernelArgAtomic? 1 : 0;
        }
    }

    // Create a symbol relocation entry for each symbol used by F
    emitSymbolRelocation(F);

    m_VRA->BeginFunction(&F, numLanes(m_SimdMode));
    if (isFuncGroupHead)
    {
        Function* Entry = m_currShader->entry;
        // owned by m_pDebugEmitter
        const bool IsPrimary = true;
        auto vMod = IGC::ScalarVisaModule::BuildNew(m_currShader, Entry, IsPrimary);
        IGC::DebugEmitterOpts DebugOpts;
        DebugOpts.DebugEnabled = DebugInfoData::hasDebugInfo(m_currShader);
        DebugOpts.ZeBinCompatible = IGC_IS_FLAG_ENABLED(ZeBinCompatibleDebugging) && m_pCtx->enableZEBinary();
        DebugOpts.EnableRelocation = IGC_IS_FLAG_ENABLED(EnableRelocations) || DebugOpts.ZeBinCompatible;
        DebugOpts.EnforceAMD64Machine = IGC_IS_FLAG_ENABLED(DebugInfoEnforceAmd64EM) || DebugOpts.ZeBinCompatible;
        DebugOpts.EnableDebugInfoValidation = IGC_IS_FLAG_ENABLED(DebugInfoValidation);
        DebugOpts.ScratchOffsetInOW = !m_currShader->m_Platform->isProductChildOf(IGFX_DG2);
        DebugOpts.VISAABIVersion =
            m_currShader->m_Platform->getVISAABIVersion();
        m_pDebugEmitter = IDebugEmitter::Create();
        m_pDebugEmitter->Initialize(std::move(vMod), DebugOpts);
    }

    IGC_ASSERT(m_pDebugEmitter);

    if (DebugInfoData::hasDebugInfo(m_currShader))
    {
        m_currShader->GetDebugInfoData().m_pShader = m_currShader;
        m_currShader->GetDebugInfoData().m_pDebugEmitter = m_pDebugEmitter;

        const bool IsPrimary = isFuncGroupHead;
        bool GroupHasStackCall = m_FGA && m_FGA->getGroup(&F)->hasStackCall();
        m_pDebugEmitter->resetModule(IGC::ScalarVisaModule::BuildNew(
            m_currShader, &F, IsPrimary, GroupHasStackCall));
    }

    // We only invoke EndEncodingMark() to update last VISA id.
    m_pDebugEmitter->EndEncodingMark();

    phiMovToBB.clear();
    unsigned int lineNo = 0;
    bool disableSlicing =
        IGC_IS_FLAG_ENABLED(DisableSIMD32Slicing) ||
        isOptDisabledForModule(m_currShader->GetContext()->getModuleMetaData(), IGCOpts::AllowSimd32Slicing) ||
        isOptDisabledForFunction(m_currShader->GetContext()->getModuleMetaData(), IGCOpts::AllowSimd32Slicing, &F) ||
        m_currShader->GetContext()->getModuleMetaData()->compOpt.OptDisable ||
        m_currShader->GetShaderType() != ShaderType::COMPUTE_SHADER ||
        m_currShader->GetContext()->getModuleMetaData()->csInfo.disableSimd32Slicing ||
        m_pattern->m_samplertoRenderTargetEnable ||
        m_currShader->HasStackCalls() || isDummyKernel;

    IGC::Debug::Dump* llvmtoVISADump = nullptr;
    if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
    {
        auto name = IGC::Debug::GetDumpNameObj(m_currShader, "visa.ll");
        // If the function is in a function group, set the postfix string of
        // DumpName as "entry name" + "_f" + "id".
        if (m_FGA && !m_FGA->isGroupHead(&F)) {
          FunctionGroup* group = m_FGA->getGroup(&F);
          // To align with visa suffixing, make id start from 0.
          unsigned id = -1;
          for (auto it = group->begin(), ie = group->end(); it != ie; ++it)
          {
              if (*it == &F)
                  break;
              ++id;
          }
          std::string kname(group->getHead()->getName().str());
          m_currShader->getShaderFileName(kname);
          std::string postfix = kname + "_f" + std::to_string(id);
          name = name.PostFix(postfix);
        }
        if (name.allow())
            llvmtoVISADump = new IGC::Debug::Dump(name, IGC::Debug::DumpType::PASS_IR_TEXT);
    }
    VisaIdAnnotator VidAnnotator;  // for visa.ll dump
    StringRef curSrcFile, curSrcDir;

    for (uint i = 0; i < m_pattern->m_numBlocks; i++)
    {
        SBasicBlock& block = m_pattern->m_blocks[i];
        block.clearCaching();   // clear for each SIMD size
        m_currentBlock = i;
        atomic_shared_pUVR.clear();
        if (m_blockCoalescing->IsEmptyBlock(block.bb))
        {
            continue;
        }

        if (llvmtoVISADump)
        {
            VidAnnotator.trackBlockId(block.bb, i);
        }

        if (i != 0)
        {
            m_pDebugEmitter->BeginEncodingMark();
            // create a label
            m_encoder->AddLabel(block.id);
            m_encoder->Push();
            m_pDebugEmitter->EndEncodingMark();
        }

        // remove cached per lane offset variables if any.
        PerLaneOffsetVars.clear();

        // Variable reuse per-block states.
        VariableReuseAnalysis::EnterBlockRAII EnterBlock(m_VRA, block.bb);

        // go through the list in reverse order
        auto I = block.m_dags.rbegin(), E = block.m_dags.rend();
        while (I != E)
        {
            Instruction* llvmInst = I->m_root;
            resetCurrInstNumInstances();

            if (llvmInst->getDebugLoc())
            {
                unsigned int curLineNumber = llvmInst->getDebugLoc().getLine();
                auto&& srcFile = llvmInst->getDebugLoc()->getScope()->getFilename();
                auto&& srcDir = llvmInst->getDebugLoc()->getScope()->getDirectory();
                if (!curSrcFile.equals(srcFile) || !curSrcDir.equals(srcDir))
                {
                    curSrcFile = srcFile;
                    curSrcDir = srcDir;
                    m_pDebugEmitter->BeginEncodingMark();
                    llvm::SmallVector<char, 1024> fileName;
                    llvm::sys::path::append(fileName, curSrcDir);
                    llvm::sys::path::append(fileName, curSrcFile);
                    std::string fileNameStr(fileName.begin(), fileName.end());
                    m_encoder->File(fileNameStr);
                    m_pDebugEmitter->EndEncodingMark();
                }
                if (curLineNumber != lineNo)
                {
                     m_pDebugEmitter->BeginEncodingMark();
                     m_encoder->Loc(curLineNumber);
                     m_pDebugEmitter->EndEncodingMark();
                     lineNo = curLineNumber;
                }
            }

            bool slicing = !disableSlicing;
            uint numInstance = DecideInstanceAndSlice(*block.bb, *I, slicing);
            IGC_ASSERT(numInstance == 1 || numInstance == 2);
            // caching the number of instance
            setCurrInstNumInstances(numInstance);

            if (slicing && !disableSlicing)
            {
                 m_pDebugEmitter->BeginEncodingMark();
                 I = emitInSlice(block, I);
                 m_pDebugEmitter->EndEncodingMark();
                 llvmInst = I->m_root;
            }

            if (I != E)
            {
                m_pDebugEmitter->BeginInstruction(llvmInst);

                // before inserting the terminator, initialize constant pool & insert the de-ssa moves
                if (isa<BranchInst>(llvmInst))
                {
                    m_encoder->SetSecondHalf(false);
                    // insert constant initializations.
                    InitConstant(block.bb);
                    // Insert lifetime start if there are any
                    emitLifetimeStartAtEndOfBB(block.bb);
                    // insert the de-ssa movs.
                    MovPhiSources(block.bb);
                    // insert lifetime start for resource loop unroll
                    emitLifetimeStartResourceLoopUnroll(block.bb);
                }

                // If slicing happens, then recalculate the number of instances.
                if (slicing)
                {
                    numInstance = DecideInstanceAndSlice(*block.bb, *I, slicing);
                    setCurrInstNumInstances(numInstance);
                }

                if (llvmtoVISADump)
                {
                    VidAnnotator.trackVisaId(llvmInst, m_encoder->GetVISAKernel()->getvIsaInstCount() + 1);
                }

                // Insert lifetime start if legal. Note taht m_destination
                // shall be nullptr if this instruction has no dst.
                emitLifetimeStart(m_destination, block.bb, llvmInst, true);

                DstModifier init;
                if (numInstance < 2)
                {
                    m_encoder->SetSecondHalf(false);
                    I->m_pattern->Emit(this, init);
                    ++I;
                }
                else
                {
                    m_encoder->SetSecondHalf(false);
                    I->m_pattern->Emit(this, init);
                    m_encoder->SetSecondHalf(true);
                    I->m_pattern->Emit(this, init);
                    ++I;
                }

                //At the end of basic block transfer freq data to all encoded instructions
                auto hasBlockFreq = [&](Instruction *inst) {
                  return inst &&
                         inst->getMetadata("stats.blockFrequency.digits") &&
                         inst->getMetadata("stats.blockFrequency.scale");
                };

                if ((IGC_GET_FLAG_VALUE(StaticProfileGuidedSpillCostAnalysis) &
                     FrequencyDataDS::PGSS_IGC_TRANS) != 0 &&
                    llvmInst->isTerminator() && hasBlockFreq(llvmInst)) {
                    auto getFreqDigit = [&](Instruction *inst) {
                      MDNode *mn_digits =
                          llvmInst->getMetadata("stats.blockFrequency.digits");
                      if (!mn_digits)
                        return (unsigned long long)0;
                      return std::stoull(
                          cast<MDString>(mn_digits->getOperand(0))
                              ->getString()
                              .str());
                    };
                    auto getFreqScale = [&](Instruction *inst) {
                      MDNode *mn_scale =
                          llvmInst->getMetadata("stats.blockFrequency.scale");
                      if (!mn_scale)
                        return (int16_t)0;
                      return (int16_t)std::stoi(
                          cast<MDString>(mn_scale->getOperand(0))
                              ->getString()
                              .str());
                    };
                    m_encoder->GetVISAKernel()->encodeBlockFrequency(
                        getFreqDigit(llvmInst), getFreqScale(llvmInst));
                }
                m_pDebugEmitter->EndInstruction(llvmInst);
            }
        }
    }

    if (llvmtoVISADump)
    {
        F.print(llvmtoVISADump->stream(), &VidAnnotator);
        delete llvmtoVISADump;
    }

    if (m_FGA && IGC_IS_FLAG_ENABLED(ForceSubReturn))
    {
        bool hasReturn = false;
        // Find if any return insts exist in all BBs
        for (auto it = IGCLLVM::rbegin(&F), ie = IGCLLVM::rend(&F); it != ie; it++)
        {
            if (isa_and_nonnull<ReturnInst>(it->getTerminator()))
            {
                hasReturn = true;
                break;
            }
        }
        if (!hasReturn)
        {
            if (!m_FGA->useStackCall(&F))
            {
                // No return, generate dummy return for each subroutine to meet visa requirement.
                m_encoder->SubroutineRet(nullptr, &F);
                m_encoder->Push();
            }
            else if (m_FGA->useStackCall(&F))
            {
                // For stackcalls, we need to restore stack state before terminating
                m_currShader->RestoreStackState();
                m_encoder->SetStackFunctionRetSize(0);
                m_encoder->StackRet(nullptr);
                m_encoder->Push();
            }
        }
    }

    bool skipPrologue = false;

    if (isFuncGroupHead && !isDummyKernel)
    {
        if (!needKernelArgOverrideWA)
        {
            // Cache the arguments list into a vector for faster access
            m_currShader->CacheArgumentsList();
            // Associates values pushed to CVariable
            m_currShader->MapPushedInputs();
            // Allocate the thread payload
            m_currShader->AllocatePayload();
        }

        if (m_encoder->IsCodePatchCandidate())
        {
            if (m_pCtx->hasSpills(m_currShader->ProgramOutput()->m_scratchSpaceUsedBySpills))
            {
                if (IGC_GET_FLAG_VALUE(CodePatchExperiments))
                {
                    errs() << "Skip Prologue : " << m_encoder->GetShaderName() << "\n";
                }
                skipPrologue = true;
            }
            else
            {
                if (IGC_GET_FLAG_VALUE(CodePatchLimit) >= 2)
                {
                    IGC_SET_FLAG_VALUE(CodePatchLimit, IGC_GET_FLAG_VALUE(CodePatchLimit) - 1);
                }
                if (IGC_GET_FLAG_VALUE(CodePatchExperiments))
                {
                    errs() << IGC_GET_FLAG_VALUE(CodePatchLimit) << " Prologue/CodePatch : " << m_encoder->GetShaderName() << "\n";
                }
            }
        }
        else
        {
            if (IGC_GET_FLAG_VALUE(CodePatchExperiments))
            {
                errs() << IGC_GET_FLAG_VALUE(CodePatchLimit) << " not : " << m_encoder->GetShaderName() << "\n";
            }
        }
    }

    // We want to mark "perThreadOffset" and "privateBase"
    // variables as output to keep them alive to the end of the function for O0 runs.
    // This is debuggability improvement, but we want to do that for non-debug compilations,
    // because debug info shouldn't alter generated code.
    if (m_currShader->GetContext()->getModuleMetaData()->compOpt.OptDisable)
    {
        DebugInfoData::saveAndMarkPrivateMemoryVars(F, m_currShader);
    }

    if (m_currShader->GetDebugInfoData().m_pDebugEmitter)
    {
        DebugInfoData::extractAddressClass(F);
        m_currShader->GetDebugInfoData().addVISAModule(
            &F, m_pDebugEmitter->getCurrentVISA());
        m_currShader->GetDebugInfoData().transferMappings(F);
    }

    m_encoder->SetMaxRegForThreadDispatch();

    // Compile only when this is the last function for this kernel.
    bool finalize = (!m_FGA || m_FGA->isGroupTail(&F));
    if (m_FGA) {
        auto *FG = m_FGA->getGroup(&F);
        // If the current SIMD mode for this funtion group is invalid, skip
        // compiling it.
        if (FG)
            finalize = finalize && FG->checkSimdModeValid(m_SimdMode);
    }
    bool destroyVISABuilder = false;
    if (finalize)
    {
        destroyVISABuilder = true;
        // We only need one symbol table per module. If there are multiple entry functions, only create a symbol
        // for the dummy kernel with indirect functions attached.
        bool compileWithSymbolTable = false;
        Function* currHead = m_FGA ? m_FGA->getGroupHead(&F) : &F;
        if (isSymbolTableRequired(currHead))
        {
            compileWithSymbolTable = true;
        }
        if (!skipPrologue)
        {
            m_encoder->Compile(compileWithSymbolTable, m_FGA);
            if (m_pCtx->type == ShaderType::OPENCL_SHADER && IGC_IS_FLAG_ENABLED(EnableKernelCostInfo)) {
                if (m_encoder->kci != nullptr) {
                    COpenCLKernel* openCLkernel = static_cast<COpenCLKernel*>(m_currShader);
                    zebin::KCMLoopCostsTy costInfoVector;
                    const vISA::CostExpr& costExpr = m_encoder->kci->kernelCost;

                    zebin::zeInfoKCMLoopCost costInfo = { (int32_t)costExpr.C.cycles, (int32_t)costExpr.C.loadBytes,
                        (int32_t)costExpr.C.storeBytes, (int32_t)costExpr.loopCosts.size() };
                    costInfoVector.push_back(costInfo);
                    int toplevelLoops = 0;
                    for (const vISA::LoopCostInfo& LCI : m_encoder->kci->allLoopCosts) {
                        int level = LCI.nestingLevel;
                        if (level == 1) {
                            toplevelLoops += 1;
                        }
                        const vISA::CostExpr& loopCostExpression = LCI.loopBodyCost;
                        const vISA::CostMetrics& CM = loopCostExpression.C;
                        costInfo = { (int)CM.cycles, (int)CM.loadBytes, (int)CM.storeBytes, LCI.numChildLoops };
                        costInfoVector.push_back(costInfo);
                    }
                    costInfoVector.at(0).num_loops = toplevelLoops;
                    openCLkernel->m_kernelCostexpInfo.kernelCost = std::move(costInfoVector);
                }
            }
        }
        m_pCtx->m_prevShader = m_currShader;
    }

    if (destroyVISABuilder)
    {
        if (!m_currShader->GetDebugInfoData().m_pDebugEmitter)
        {
            IDebugEmitter::Release(m_pDebugEmitter);
        }

        if (!m_encoder->IsCodePatchCandidate() ||
            m_encoder->HasPrevKernel() ||
            !m_currShader->ProgramOutput()->m_programBin ||
            m_pCtx->hasSpills(m_currShader->ProgramOutput()->m_scratchSpaceUsedBySpills))
        {
            m_pCtx->m_prevShader = nullptr;
            // Postpone destroying VISA builder to
            // after emitting debug info and passing context for code patching
            m_encoder->DestroyVISABuilder();
        }
        if (m_encoder->IsCodePatchCandidate() && m_encoder->HasPrevKernel())
        {
            prevShader->GetEncoder().DestroyVISABuilder();
        }
    }

    if ((m_currShader->GetShaderType() == ShaderType::COMPUTE_SHADER ||
        m_currShader->GetShaderType() == ShaderType::OPENCL_SHADER) &&
        m_currShader->m_State.shouldDisablePreemption(
            m_currShader->ProgramOutput()->m_InstructionCount))
    {

        {
            COpenCLKernel* kernel = static_cast<COpenCLKernel*>(m_currShader);
            kernel->SetDisableMidthreadPreemption();
        }
    }

    if (IGC_IS_FLAG_ENABLED(ForceBestSIMD))
    {
        return false;
    }

    if (m_SimdMode == SIMDMode::SIMD16 &&
        this->m_ShaderDispatchMode == ShaderDispatchMode::NOT_APPLICABLE &&
        IsStage1BestPerf(m_pCtx->m_CgFlag, m_pCtx->m_StagingCtx))
    {
        m_pCtx->m_doSimd32Stage2 = m_currShader->CompileSIMDSize(SIMDMode::SIMD32, *this, F);
    }

    if (m_SimdMode == SIMDMode::SIMD8 &&
        IsStage1FastCompile(m_pCtx->m_CgFlag, m_pCtx->m_StagingCtx))
    {
        m_pCtx->m_doSimd16Stage2 = m_currShader->CompileSIMDSize(SIMDMode::SIMD16, *this, F);
        m_pCtx->m_doSimd32Stage2 = m_currShader->CompileSIMDSize(SIMDMode::SIMD32, *this, F);
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
        emitLifetimeStart(m_destination, block.bb, (*sliceIter).m_root, false);

        (*sliceIter).m_pattern->Emit(this, init);
        ++sliceIter;
        slicing = false;
        if (sliceIter != E)
        {
            unsigned numInstance = DecideInstanceAndSlice(*(block.bb), (*sliceIter), slicing);
            IGC_ASSERT(numInstance == 1 || numInstance == 2);
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
            emitLifetimeStart(m_destination, block.bb, (*sliceIter).m_root, false);

            (*sliceIter).m_pattern->Emit(this, init);
        }
    }

    return sliceEnd;
}

/// Insert moves at the end of the basic block to replace the phi node of the successors
void EmitPass::MovPhiSources(llvm::BasicBlock* aBB)
{
    // collect all the src-side phi-moves, then find a good order for emission
    struct PhiSrcMoveInfo {
        CVariable* dstCVar;
        CVariable* srcCVar;
        Value* dstRootV; // root value of dst (dessa)
        Value* srcRootV; // root value of src (dessa)
    };

    struct VTyInfo {
        unsigned numElt;
        bool isSplat;
        uint64_t splatValue;

        VTyInfo() : numElt(0), isSplat(false), splatValue(0) { }
        explicit VTyInfo(Value * V) : numElt(0), isSplat(false), splatValue(0) {
            if (IGCLLVM::FixedVectorType * vTy = dyn_cast<IGCLLVM::FixedVectorType>(V->getType())) {
                numElt = vTy->getNumElements();
                if (isa<ConstantAggregateZero>(V)) {
                    isSplat = true;
                    splatValue = 0;
                }
                else if (ConstantDataVector * CDV = dyn_cast<ConstantDataVector>(V)) {
                    if (Constant * C = CDV->getSplatValue()) {
                        if (ConstantInt * CInt = dyn_cast<ConstantInt>(C)) {
                            isSplat = true;
                            splatValue = CInt->getZExtValue();
                        }
                        else if (ConstantFP * CFP = dyn_cast<ConstantFP>(C)) {
                            isSplat = true;
                            splatValue = CFP->getValueAPF().bitcastToAPInt().getZExtValue();
                        }
                    }
                }
            }
        }
    };

    BumpPtrAllocator phiAllocator;
    std::list<PhiSrcMoveInfo*> phiSrcDstList;
    std::vector<std::pair<CVariable*, CVariable*>> emitList;
    std::map<CVariable*, VTyInfo> dstVTyMap;
    llvm::BasicBlock* bb = aBB;
    IGCLLVM::TerminatorInst* TI = aBB->getTerminator();
    IGC_ASSERT(nullptr != TI);

    // main code to generate phi-mov
    for (unsigned succ = 0, e = TI->getNumSuccessors(); succ != e; ++succ)
    {
        llvm::BasicBlock* Succ = TI->getSuccessor(succ);
        for (auto II = Succ->begin(), IE = Succ->end(); II != IE; ++II)
        {
            llvm::PHINode* PN = llvm::dyn_cast<llvm::PHINode>(II);
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
                    Value* Src = PN->getOperand(i);

                    if (isa<UndefValue>(Src))
                    {
                        if (IGC_IS_FLAG_ENABLED(AssignZeroToUndefPhiNodes))
                        {
                            Src = Constant::getNullValue(Src->getType());
                        }
                    }

                    Value* dstRootV = m_deSSA ? m_deSSA->getRootValue(PN) : PN;
                    Value* srcRootV = m_deSSA ? m_deSSA->getRootValue(Src) : Src;
                    dstRootV = dstRootV ? dstRootV : PN;
                    srcRootV = srcRootV ? srcRootV : Src;
                    // To check if src-side phi mov is needed, we must use dessa
                    // rootValue instead of CVariable, as value alias in dessa
                    // might have the same variable with two different CVariable.
                    if (dstRootV != srcRootV)
                    {
                        VTyInfo vTyInfo(Src);
                        PhiSrcMoveInfo* phiInfo = new (phiAllocator) PhiSrcMoveInfo();
                        phiInfo->srcCVar = vTyInfo.isSplat ? nullptr : m_currShader->GetSymbol(Src);
                        phiInfo->dstCVar = m_currShader->GetSymbol(PN);
                        phiInfo->dstRootV = dstRootV;
                        phiInfo->srcRootV = srcRootV;
                        phiSrcDstList.push_back(phiInfo);
                        dstVTyMap.insert(std::make_pair(phiInfo->dstCVar, vTyInfo));
                    }

                    // only handled in resource loop emit pass
                    if (PN->getMetadata("MyUniqueExclusiveLoadMetadata") != nullptr)
                    {
                        m_encoder->SetUniqueExcusiveLoad(true);
                    }
                }
            }
        }
    }

    // Find a good order for src-side phi-moves.
    //
    // PHI copies are parallel copy. Here, need to serialize those copies
    // in a way that the dst will not be overwritten by a previous copy.
    //     For example,
    //        (phi_1, phi_2) = (a, phi_1)
    //     ==>
    //        phi_2 = phi_1
    //        phi_1 = a
    // If there is a cycle, have to insert a temp copy to break the cycle (see below)
    while (!phiSrcDstList.empty())
    {
        // search should not get into a deadlock, i.e should be able to find one to emit every iteration,
        auto It = phiSrcDstList.begin();
        auto Et = phiSrcDstList.end();
        for (; It != Et; ++It)
        {
            auto Cmp = [&](const PhiSrcMoveInfo* Val)
            {
                return Val->srcRootV == (*It)->dstRootV;
            };

            if (0 == std::count_if (phiSrcDstList.begin(), phiSrcDstList.end(), Cmp))
            {
                break;
            }
        }
        if (It == Et)
        {
            // Found cyclic phi-move dependency. Pick the first one (anyone
            // should be good) and create a temp to break the dependence cycle.
            // (Note that there is no self-cycle.)
            // For example,
            //    (phi_1, phi_2) = (phi_2, phi_1)
            //  ==>
            //    t = phi_1
            //    phi_1 = phi_2
            //    phi_2 = t

            // After the temp copy of the 1st entry's dst is inserted,
            // the entry becomes the one to be added into emitList.
            It = phiSrcDstList.begin();

            Value* dRootV = (*It)->dstRootV;
            CVariable* D1 = (*It)->dstCVar;
            CVariable* T = m_currShader->GetNewVariable(D1);
            dstVTyMap[T] = dstVTyMap[D1];
            emitList.push_back(std::pair<CVariable*, CVariable*>(D1, T));

            // Replace with T all src that is equal to D1 (start from It+1)
            auto LI = It, LE = phiSrcDstList.end();
            for (++LI; LI != LE; ++LI)
            {
                PhiSrcMoveInfo* phiinfo = *LI;
                if (phiinfo->srcRootV == dRootV) {
                    CVariable* sVar = phiinfo->srcCVar;
                    CVariable* nVar;
                    if (sVar) { // sVar is null if srcCVar is a constant splat vector
                        if (sVar->GetType() != T->GetType()) {
                            nVar = m_currShader->GetNewAlias(
                                T, sVar->GetType(), 0, sVar->GetNumberElement());
                        }
                        else {
                            nVar = T;
                        }
                        phiinfo->srcCVar = nVar;
                    }
                }
            }
        }
        IGC_ASSERT(It != Et);
        if (m_pCtx->getModuleMetaData()->compOpt.initializePhiSampleSourceWA &&
            m_pattern->IsPHISourceOfSampleUnderCF((*It)->dstRootV) &&
            m_alreadyInitializedPHI.find((*It)->dstRootV) == m_alreadyInitializedPHI.end())
        {
            // this is WA for situation where application has early return (not discard) from shader
            // in this case phi node will not set sample coords properly on all simd lanes
            // WA is to initialize register used for phi node with zero, which eliminates corruptions
            m_alreadyInitializedPHI.insert((*It)->dstRootV);
            CVariable* zero = m_currShader->ImmToVariable(0, (*It)->dstCVar->GetType());
            m_encoder->SetNoMask();
            m_encoder->Copy((*It)->dstCVar, zero);
            m_encoder->Push();
        }
        emitList.push_back(std::pair<CVariable*, CVariable*>((*It)->srcCVar, (*It)->dstCVar));
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
            const VTyInfo & vTyInfo = dstVTyMap[dst];
            if (vTyInfo.isSplat) {
                emitConstantVector(dst, vTyInfo.splatValue);
            }
            else if (vTyInfo.numElt > 0) {
                emitVectorCopy(dst, src, vTyInfo.numElt);
            }
            else {
                m_encoder->Copy(dst, src);
                m_encoder->Push();
            }
        }
    }
}

void EmitPass::InitConstant(llvm::BasicBlock* BB)
{
    for (auto& I : m_pattern->ConstantPlacement)
    {
        if (I.second != BB)
            continue;
        Constant* C = I.first;
        CVariable* Dst = m_currShader->lookupConstantInPool(C);
        if (Dst)
            continue;
        Dst = m_currShader->GetConstant(C);
        if (!C->getType()->isVectorTy()) {
            CVariable* Imm = Dst;
            Dst = m_currShader->GetNewVector(C);
            m_encoder->Copy(Dst, Imm);
            m_encoder->Push();
        }
        m_currShader->addConstantInPool(C, Dst);
    }
}

void EmitPass::emitLifetimeStartResourceLoopUnroll(BasicBlock* BB)
{
    // should insert the lifetime.start before the first unroll BB
    if (BasicBlock* bb = BB->getNextNode(); bb)
    {
        ModuleMetaData* modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
        auto& lifeTimeStartMap = modMD->lifeTimeStartMap;

        if (lifeTimeStartMap.count(bb))
        {
            CVariable* dstCVar = m_currShader->GetSymbol(lifeTimeStartMap[bb]);
            m_encoder->Lifetime(LIFETIME_START, dstCVar);
        }
    }
}

void EmitPass::emitLifetimeStartAtEndOfBB(BasicBlock* BB)
{
    if (m_pCtx->getVectorCoalescingControl() == 0) {
        return;
    }

    auto II = m_VRA->m_LifetimeAtEndOfBB.find(BB);
    if (II != m_VRA->m_LifetimeAtEndOfBB.end())
    {
        TinyPtrVector<Value*>& ARVs = II->second;
        for (int i = 0, sz = (int)ARVs.size(); i < sz; ++i)
        {
            Value* RootVal = ARVs[i];
            CVariable* Var = GetSymbol(RootVal);

            // vISA info inst, no m_encoder->Push() needed.
            m_encoder->Lifetime(LIFETIME_START, Var);
        }
    }
}

std::pair<Value*, Value*> EmitPass::getPairOutput(Value* V) const {
    auto I = m_pattern->PairOutputMap.find(V);
    IGC_ASSERT(I != m_pattern->PairOutputMap.end());
    return std::make_pair(I->second.first, I->second.second);
}

void EmitPass::emitGradientX(const SSource& source, const DstModifier& modifier)
{
    CVariable* src = GetSrcVariable(source);
    if (src->IsUniform())
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
    if (src->IsUniform())
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
    if (src->IsUniform())
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
    if (src->IsUniform())
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

        if (m_currShader->m_SIMDSize == SIMDMode::SIMD16 || m_currShader->m_SIMDSize == SIMDMode::SIMD32)
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

        if (m_currShader->m_SIMDSize == SIMDMode::SIMD32)
        {
            m_encoder->SetSimdSize(SIMDMode::SIMD4);
            m_encoder->SetSrcModifier(0, src_mod0);
            m_encoder->SetSrcRegion(0, 0, 2, 1);
            m_encoder->SetSrcSubReg(0, 18);

            m_encoder->SetSrcModifier(1, src_mod1);
            m_encoder->SetSrcRegion(1, 0, 2, 1);
            m_encoder->SetSrcSubReg(1, 16);
            m_encoder->SetNoMask();

            m_encoder->SetDstModifier(modifier);
            m_encoder->SetDstSubReg(16);
            m_encoder->Add(temp, src, src);
            m_encoder->Push();

            m_encoder->SetSimdSize(SIMDMode::SIMD4);
            m_encoder->SetSrcModifier(0, src_mod0);
            m_encoder->SetSrcRegion(0, 0, 2, 1);
            m_encoder->SetSrcSubReg(0, 22);

            m_encoder->SetSrcModifier(1, src_mod1);
            m_encoder->SetSrcRegion(1, 0, 2, 1);
            m_encoder->SetSrcSubReg(1, 20);
            m_encoder->SetNoMask();

            m_encoder->SetDstModifier(modifier);
            m_encoder->SetDstSubReg(20);
            m_encoder->Add(temp, src, src);
            m_encoder->Push();

            m_encoder->SetSimdSize(SIMDMode::SIMD4);
            m_encoder->SetSrcModifier(0, src_mod0);
            m_encoder->SetSrcRegion(0, 0, 2, 1);
            m_encoder->SetSrcSubReg(0, 26);

            m_encoder->SetSrcModifier(1, src_mod1);
            m_encoder->SetSrcRegion(1, 0, 2, 1);
            m_encoder->SetSrcSubReg(1, 24);
            m_encoder->SetNoMask();

            m_encoder->SetDstModifier(modifier);
            m_encoder->SetDstSubReg(24);
            m_encoder->Add(temp, src, src);
            m_encoder->Push();

            m_encoder->SetSimdSize(SIMDMode::SIMD4);
            m_encoder->SetSrcModifier(0, src_mod0);
            m_encoder->SetSrcRegion(0, 0, 2, 1);
            m_encoder->SetSrcSubReg(0, 30);

            m_encoder->SetSrcModifier(1, src_mod1);
            m_encoder->SetSrcRegion(1, 0, 2, 1);
            m_encoder->SetSrcSubReg(1, 28);

            m_encoder->SetNoMask();
            m_encoder->SetDstModifier(modifier);
            m_encoder->SetDstSubReg(28);
            m_encoder->Add(temp, src, src);
            m_encoder->Push();
        }

        m_encoder->Copy(m_destination, temp);
        m_encoder->Push();
    }
}

void EmitPass::EmitAluIntrinsic(llvm::CallInst* I, const SSource source[2], const DstModifier& modifier)
{
    if (GenIntrinsicInst * CI = dyn_cast<GenIntrinsicInst>(I))
    {
        switch (CI->getIntrinsicID())
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
    else if (IntrinsicInst * CI = dyn_cast<IntrinsicInst>(I))
    {
        switch (CI->getIntrinsicID())
        {
        case Intrinsic::ctlz:
            //Throw away source[1], since for ctlz, this is a flag we don't care about.
            emitCtlz(source[0]);
            break;
        case Intrinsic::sqrt:
        {
            // By default, double sqrt is correctly rounded (IEEE sqrt).
            if (CI->getType()->isDoubleTy() && !CI->hasApproxFunc())
            {
                EmitSimpleAlu(llvm_ieee_sqrt, source, modifier);
            }
            else
            {
                EmitSimpleAlu(I, source, modifier);
            }
            break;
        }
        case Intrinsic::powi:
            Powi(source, modifier);
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
    IGC_ASSERT(0);
    return EPREDICATE_EQ;
}

static VISA_Type GetUnsignedType(VISA_Type type)
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
    default:
        IGC_ASSERT(0);
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
        IGC_ASSERT(0);
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
    case ISA_TYPE_BF:
        return ISA_TYPE_UW;
    default:
        IGC_ASSERT(0);
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
    case ISA_TYPE_BF:   return 0x3F80;
    case ISA_TYPE_HF:   return 0x3C00;
    default: break;
    }
    IGC_ASSERT_MESSAGE(0, "unknown floating type!");
    return ~0U;
}

static uint64_t getFPNegOne(VISA_Type Ty)
{
    switch (Ty)
    {
    case ISA_TYPE_DF:   return 0xBFF0000000000000;
    case ISA_TYPE_F:    return 0xBF800000;
    case ISA_TYPE_BF:   return 0xBF80;
    case ISA_TYPE_HF:   return 0xBC00;
    default: break;
    }
    IGC_ASSERT_MESSAGE(0, "unknown floating type!");
    return ~0U;
}

CVariable* EmitPass::GetSrcVariable(const SSource& source, bool fromConstPool)
{
    CVariable* src = m_currShader->GetSymbol(source.value, fromConstPool);
    // Change the type of source if needed.
    if (source.type != ISA_TYPE_NUM && source.type != src->GetType())
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

void EmitPass::SetSourceModifiers(unsigned int sourceIndex, const SSource& source) const
{
    if (source.mod != EMOD_NONE)
    {
        m_encoder->SetSrcModifier(sourceIndex, source.mod);
    }

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

    if (source.region_set)
    {
        m_encoder->SetSrcRegion(sourceIndex, source.region[0], source.region[1], source.region[2], source.instance);
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
    CVariable* srcs[2] = { nullptr, nullptr };

    srcs[0] = GetSrcVariable(sources[0], sources[0].fromConstantPool);
    SetSourceModifiers(0, sources[0]);

    if (sources[1].value)
    {
        srcs[1] = GetSrcVariable(sources[1], sources[1].fromConstantPool);
        SetSourceModifiers(1, sources[1]);
    }
    m_encoder->SetDstModifier(modifier);
    EmitSimpleAlu(opCode, m_destination, srcs[0], srcs[1]);
}

void EmitPass::EmitSimpleAlu(EOPCODE opCode, CVariable* dst, CVariable* src0, CVariable* src1)
{
    bool doEmitCastSelect = (m_currShader->m_Platform->hasPartialEmuI64Enabled() && dst->isQType()) ? true : false;
    bool doEmitCastCmp = (m_currShader->m_Platform->hasPartialEmuI64Enabled() && src0->isQType()) ? true : false;
    switch (opCode)
    {
    case llvm_fmul:
    case llvm_mul:
        m_encoder->Mul(dst, src0, src1);
        break;
    case llvm_fdiv:
        m_encoder->Div(dst, src0, src1);
        break;
    case llvm_fadd:
    case llvm_add:
        m_encoder->Add(dst, src0, src1);
        break;
    case llvm_cos:
        m_encoder->Cos(dst, src0);
        break;
    case llvm_sin:
        m_encoder->Sin(dst, src0);
        break;
    case llvm_log:
        m_encoder->Log(dst, src0);
        break;
    case llvm_exp:
        m_encoder->Exp(dst, src0);
        break;
    case llvm_pow:
        m_encoder->Pow(dst, src0, src1);
        break;
    case llvm_sqrt:
        m_encoder->Sqrt(dst, src0);
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
        m_encoder->MulH(dst, src0, src1);
        break;
    case llvm_umulh:
    {
        src0 = m_currShader->BitCast(src0, GetUnsignedType(src0->GetType()));
        src1 = m_currShader->BitCast(src1, GetUnsignedType(src1->GetType()));
        dst = m_currShader->BitCast(dst, GetUnsignedType(dst->GetType()));
        m_encoder->MulH(dst, src0, src1);
    }
    break;
    case llvm_sext:
    {
        if (src0->GetType() == ISA_TYPE_BOOL)
        {
            CVariable* minusone = m_currShader->ImmToVariable(-1, dst->GetType());
            CVariable* zero = m_currShader->ImmToVariable(0, dst->GetType());
            if (doEmitCastSelect)
                emitCastSelect(src0, dst, minusone, zero);
            else
                m_encoder->Select(src0, dst, minusone, zero);
        }
        else
        {
            m_encoder->Cast(dst, src0);
        }
    }
    break;
    case llvm_zext:
    {
        if (src0->GetType() == ISA_TYPE_BOOL)
        {
            CVariable* one = m_currShader->ImmToVariable(1, dst->GetType());
            CVariable* zero = m_currShader->ImmToVariable(0, dst->GetType());
            if (doEmitCastSelect)
                emitCastSelect(src0, dst, one, zero);
            else
                m_encoder->Select(src0, dst, one, zero);
        }
        else
        {
            src0 = m_currShader->BitCast(src0, GetUnsignedType(src0->GetType()));
            m_encoder->Cast(dst, src0);
        }
    }
    break;
    case llvm_trunc:
    case llvm_fptrunc:
    case llvm_fpext:
    case llvm_fptosi:
    case llvm_fptoui:
        if (dst->GetType() == ISA_TYPE_BOOL)
        {
            if (doEmitCastCmp) { IGC_ASSERT_MESSAGE(0, "NOT IMPLEMENTED YET!"); }
            m_encoder->Cmp(EPREDICATE_NE, dst, src0, m_currShader->ImmToVariable(0, src0->GetType()));
        }
        else
        {
            if (opCode == llvm_fptoui)
            {
                dst = m_currShader->BitCast(dst, GetUnsignedType(dst->GetType()));
            }
            m_encoder->Cast(dst, src0);
        }
        break;
    case llvm_sitofp:
    case llvm_uitofp:
        if (src0->GetType() == ISA_TYPE_BOOL)
        {
            auto val = (opCode == llvm_uitofp) ? getFPOne(dst->GetType()) : getFPNegOne(dst->GetType());
            CVariable* trueVal = m_currShader->ImmToVariable(val, dst->GetType());
            CVariable* zero = m_currShader->ImmToVariable(0, dst->GetType());
            if (doEmitCastSelect)
                emitCastSelect(src0, dst, trueVal, zero);
            else
                m_encoder->Select(src0, dst, trueVal, zero);
        }
        else
        {
            if (opCode == llvm_uitofp)
            {
                src0 = m_currShader->BitCast(src0, GetUnsignedType(src0->GetType()));
            }
            m_encoder->Cast(dst, src0);
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
        src0 = m_currShader->BitCast(src0, GetUnsignedType(src0->GetType()));
        src1 = m_currShader->BitCast(src1, GetUnsignedType(src1->GetType()));
        dst = m_currShader->BitCast(dst, GetUnsignedType(dst->GetType()));
        m_encoder->Div(dst, src0, src1);
    }
    break;
    case llvm_sdiv:
        m_encoder->Div(dst, src0, src1);
        break;
    case llvm_urem:
    {
        src0 = m_currShader->BitCast(src0, GetUnsignedType(src0->GetType()));
        src1 = m_currShader->BitCast(src1, GetUnsignedType(src1->GetType()));
        dst = m_currShader->BitCast(dst, GetUnsignedType(dst->GetType()));
        m_encoder->Mod(dst, src0, src1);
    }
    break;
    case llvm_srem:
        m_encoder->Mod(dst, src0, src1);
        break;
    case llvm_shl:
        m_encoder->Shl(dst, src0, src1);
        break;
    case llvm_ishr:
        m_encoder->IShr(dst, src0, src1);
        break;
    case llvm_ushr:
    {
        // src0 and dst must be unsigned
        src0 = m_currShader->BitCast(src0, GetUnsignedType(src0->GetType()));
        dst = m_currShader->BitCast(dst, GetUnsignedType(dst->GetType()));
        m_encoder->Shr(dst, src0, src1);
    }
    break;
    case llvm_min:
        m_encoder->Min(dst, src0, src1);
        break;
    case llvm_max:
        m_encoder->Max(dst, src0, src1);
        break;
    case llvm_uaddc:
    {
        src0 = m_currShader->BitCast(src0, GetUnsignedType(src0->GetType()));
        src1 = m_currShader->BitCast(src1, GetUnsignedType(src1->GetType()));
        dst = m_currShader->BitCast(dst, GetUnsignedType(dst->GetType()));
        CVariable* dstCarryBorrow = m_currShader->GetNewAlias(dst, dst->GetType(),
            dst->GetSize() / 2, dst->GetNumberElement() / 2);
        m_encoder->UAddC(dst, dstCarryBorrow, src0, src1);
    }
    break;
    case llvm_usubb:
    {
        src0 = m_currShader->BitCast(src0, GetUnsignedType(src0->GetType()));
        src1 = m_currShader->BitCast(src1, GetUnsignedType(src1->GetType()));
        dst = m_currShader->BitCast(dst, GetUnsignedType(dst->GetType()));
        CVariable* dstCarryBorrow = m_currShader->GetNewAlias(dst, dst->GetType(),
            dst->GetSize() / 2, dst->GetNumberElement() / 2);
        m_encoder->USubB(dst, dstCarryBorrow, src0, src1);
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
                = m_currShader->GetNewVariable(
                    dst->GetNumberElement(),
                    ISA_TYPE_UD,
                    dst->IsUniform() ? EALIGN_DWORD : EALIGN_GRF,
                    dst->IsUniform(),
                    dst->getName());
            m_encoder->CBit(tmp, src0);
            m_encoder->Push();
            m_encoder->Cast(dst, tmp);
            break;
        }
        IGC_ASSERT(dst->GetType() == ISA_TYPE_UQ);
        // TODO: So far, 64-bit popcnt is handled in LLVM IR as follows:
        // dst = popcnt.32(src & 0xFFFFFFFF);
        // dst += popcnt.32(src >> 32);
        // We could do the same thing here if the original sequence in LLVM IR
        // cannot be translated efficienty.
        IGC_ASSERT_MESSAGE(0, "NOT IMPLEMENTED YET!");
        break;
    }
    case llvm_ieee_sqrt:
        m_encoder->IEEESqrt(dst, src0);
        break;
    case llvm_ieee_divide:
        m_encoder->IEEEDivide(dst, src0, src1);
        break;
    case llvm_inv:
        m_encoder->Inv(dst, src0);
        break;
    default:
        //need support
        IGC_ASSERT(0);
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

void EmitPass::Powi(const SSource sources[2], const DstModifier& modifier)
{
    CVariable* src0 = GetSrcVariable(sources[0]);
    CVariable* src1Int = GetSrcVariable(sources[1]);
    CVariable* src1Float = nullptr;
    if (src1Int->IsImmediate())
    {
        int32_t immSrc1Int = int_cast<int32_t>(src1Int->GetImmediateValue());
        float immSrc1Float = (float)(immSrc1Int);
        uint32_t immSrc1 = 0;
        std::memcpy(&immSrc1, &immSrc1Float, sizeof(immSrc1));
        src1Float = m_currShader->ImmToVariable(immSrc1, ISA_TYPE_F);
    }
    else
    {
        src1Float = m_currShader->GetNewVariable(src0);
        m_encoder->Cast(src1Float, src1Int);
    }
    m_encoder->SetDstModifier(modifier);
    EmitSimpleAlu(EOPCODE::llvm_pow, m_destination, src0, src1Float);
}

void IGC::EmitPass::EmitUAdd(llvm::BinaryOperator* inst, const DstModifier& modifier)
{
    // the emit function should be called only if saturation is enabled. In other case the signedness of
    // the instruction doesn't play a role in case of computing the instruction.
    IGC_ASSERT(modifier.sat == true);
    CVariable* srcs[2] = { GetSymbol(inst->getOperand(0)), GetSymbol(inst->getOperand(1)) };

    // create new aliases for the operands and the destination
    srcs[0] = m_currShader->BitCast(srcs[0], GetUnsignedType(srcs[0]->GetType()));
    srcs[1] = m_currShader->BitCast(srcs[1], GetUnsignedType(srcs[1]->GetType()));
    CVariable* dst = m_currShader->BitCast(m_destination, GetUnsignedType(m_destination->GetType()));
    m_encoder->SetDstModifier(modifier);

    EmitSimpleAlu(EOPCODE::llvm_add, dst, srcs[0], srcs[1]);
}

void EmitPass::EmitFullMul32(bool isUnsigned, const SSource sources[2], const DstModifier& dstMod) {
    CVariable* srcs[2] = { nullptr, nullptr };
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

void EmitPass::EmitFPToIntWithSat(bool isUnsigned, bool needBitCast, VISA_Type type, const SSource& source, const DstModifier& dstMod) {
    EOPCODE op = isUnsigned ? llvm_fptoui : llvm_fptosi;

    CVariable* dst = m_destination;
    if (type != m_destination->GetType()) {
        dst = m_currShader->GetNewVariable(
            dst->GetNumberElement(), type,
            m_currShader->getGRFAlignment(),
            dst->IsUniform(), m_destination->getName());
    }
    else if (needBitCast) {
        dst = m_currShader->BitCast(dst, GetUnsignedIntegerType(dst->GetType()));
    }
    DstModifier satDstMod = dstMod;
    satDstMod.sat = true;
    m_encoder->SetDstModifier(satDstMod);

    CVariable* src = GetSrcVariable(source);
    SetSourceModifiers(0, source);
    EmitSimpleAlu(op, dst, src, nullptr);
    if (type != m_destination->GetType()) {
        CVariable* tmp = m_currShader->BitCast(dst, GetUnsignedType(type));
        dst = m_destination;
        if (needBitCast) {
            dst = m_currShader->BitCast(dst, GetUnsignedIntegerType(dst->GetType()));
        }
        m_encoder->Cast(dst, tmp);
    }
}

void EmitPass::EmitIntegerTruncWithSat(bool isSignedDst, bool isSignedSrc, const SSource& source, const DstModifier& dstMod) {
    CVariable* dst = m_destination;
    if (!isSignedDst) {
        dst = m_currShader->BitCast(dst, GetUnsignedIntegerType(dst->GetType()));
    }
    DstModifier satDstMod = dstMod;
    satDstMod.sat = true;
    m_encoder->SetDstModifier(satDstMod);

    CVariable* src = GetSrcVariable(source);
    if (!isSignedSrc) {
        src = m_currShader->BitCast(src, GetUnsignedIntegerType(src->GetType()));
    }
    m_encoder->SetSrcModifier(0, source.mod);

    m_encoder->Cast(dst, src);
    m_encoder->Push();
}

// This function generates copy:
//
//       Dst[Dst_off] = Src[Src_off] with type 'Ty'
//
//   If either S or D are not naturally aligned on 'Ty', need to use several
//   smaller & aligned copies (worse case is to use byte-aligned copy).
//
// Unaligned copy exists for insertValue and extractValue on uniform
// packed struct. And this function is used to copy a member field of
// the struct variable.
// Note:
//   Dst and Src are assumed to be aligned (it is guaranteed for struct
//   cvariables) so alignment can be verified by checking Dst_off/Src_off.
void EmitPass::emitMayUnalignedVectorCopy(
    CVariable* Dst, uint32_t Dst_off,
    CVariable* Src, uint32_t Src_off, Type* Ty)
{
    IGC_ASSERT(Ty->isSingleValueType());

    const uint32_t nLanes = numLanes(m_currShader->m_SIMDSize);

    auto v_ty = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
    uint32_t v_nelts = v_ty ? (uint32_t)v_ty->getNumElements() : 1;
    Type* eltTy = Ty->getScalarType();
    uint32_t eltBytes = (uint32_t)m_DL->getTypeStoreSize(eltTy);
    bool D_uniform = Dst->IsUniform();
    const bool S_uniform = Src->IsUniform();
    VISA_Type visaTy = m_currShader->GetType(Ty);

    uint32_t currAlign = (uint32_t)MinAlign(eltBytes, Dst_off);
    currAlign = (uint32_t)MinAlign(currAlign, Src_off);
    if (currAlign >= eltBytes) {
        // aligned copy
        CVariable* D = m_currShader->GetNewAlias(
            Dst, visaTy, Dst_off, v_nelts * (D_uniform ? 1 : nLanes));
        CVariable* S;
        if (Src->IsImmediate()) {
            IGC_ASSERT(v_nelts == 1 && Src_off == 0);
            S = Src;
        }
        else {
            S = m_currShader->GetNewAlias(
                Src, visaTy, Src_off, v_nelts * (S_uniform ? 1 : nLanes));
        }

        emitCopyAll(D, S, Ty);
        return;
    }

    // To be unaligned, at least one of D and S is uniform.
    //   As (D_uniform && !S_uniform) is not supported, S must be uniform.
    //   Keep this in mind when reading the code below.
    IGC_ASSERT(S_uniform);

    // Unaligned copy:
    //   Assuming %a and %b are the same and are assigned to r10,
    //   and %s is assigned to r20.
    //     %a = insertValue <{i8, i16}> %b,  i16 %s, 1
    //
    //  1. Assuming both operands are uniform. "copy %s into %a" is unaligned
    //     and has to be done as below:
    //     mov (1)  r10.1<1>:b r20.0<0;1,0>:b
    //     mov (1)  r10.2<1>:b r20.1<0;1,0>:b
    //
    //  2. Assuming %a is non-uniform and %b is uniform. The code is as below:
    //     mov (16) r10.1<2>:b r20.0<0;1,0>:b
    //     mov (16) r10.2<2>:b r20.1<0;1,0>:b
    //
    //     The 'stride' could be 2, 4, or 8. If it is 8, need a special
    //     handling (it happens only if Dst is non-uniform and elt size
    //     is 8 bytes and Src is uniform with byte alignment).
    CVariable* origDst = Dst;
    if (!D_uniform && eltBytes == 8 && (Src_off & 1) == 1) {
        // If stride is 8, need to move to a uniform temp first and then do
        // aligned-copy to %a to avoid illegal stride=8.
        CVariable* tVar = m_currShader->GetNewVariable(
            v_nelts, visaTy, EALIGN_QWORD, true, Src->getName());
        Dst = tVar;
        D_uniform = true;
    }
    for (uint32_t i = 0; i < v_nelts; ++i) {
        uint32_t off = 0;
        uint32_t doff = Dst_off + i * eltBytes * (D_uniform ? 1 : nLanes);
        uint32_t soff = Src_off + i * eltBytes;

        uint64_t imm = 0;
        if (Src->IsImmediate()) {
            //  It's better set soff to doff than 0.
            soff = doff;
            imm = Src->GetImmediateValue();
        }

        while (off < eltBytes) {
            // Find alignment that is less than the remaining bytes to be copied.
            // This alignment must be aligned for both Dst and Src. The max align
            // starts with element size (tyBytes) and decreases.
            uint32_t remainingBytes = eltBytes - off;
            uint32_t maxAlign = (uint32_t)PowerOf2Floor(remainingBytes);
            uint32_t currAlign = (uint32_t)MinAlign(maxAlign, doff);
            currAlign = (uint32_t)MinAlign(currAlign, soff);

            Type* tyi = IntegerType::get(Ty->getContext(), currAlign * 8);
            VISA_Type visaTyi = m_currShader->GetType(tyi);

            uint32_t stride = eltBytes / currAlign;
            IGC_ASSERT((eltBytes % currAlign) == 0);
            IGC_ASSERT_MESSAGE((D_uniform || stride <= 4),
                "Stride > 4 unsupported!");

            CVariable* S;
            if (Src->IsImmediate()) {
                // Get immediate at offset = off, with size = currAlign
                uint64_t thisImm =
                    ((imm >> (off * 8)) & maskTrailingOnes<uint32_t>(currAlign * 8));
                Constant* Cint = ConstantInt::get(tyi, thisImm);
                S = m_currShader->GetScalarConstant(Cint);
            } else {
                S = m_currShader->GetNewAlias(Src, visaTyi, soff, 1);
            }
            // #elts isn't critical as it extends to the end of Dst anyway for
            // alias, but still make sense to get the correct #elts.
            CVariable* D = m_currShader->GetNewAlias(
                Dst, visaTyi, doff, (D_uniform ? 1 : 1 + (nLanes-1) * stride));

            m_encoder->SetDstRegion(D_uniform ? 1 : stride);
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->Copy(D, S);
            m_encoder->Push();

            off += currAlign;
            doff += currAlign;
            soff += currAlign;
        }
    }

    // If a temp is used, need to copy temp to the original Dst
    if (origDst != Dst) {
        IGC_ASSERT(!origDst->IsUniform());
        CVariable* D = m_currShader->GetNewAlias(
            origDst, Dst->GetType(), Dst_off, nLanes*v_nelts);
        emitVectorCopy(D, Dst, v_nelts);
    }
}

void EmitPass::EmitInsertValueToStruct(InsertValueInst* inst)
{
    if (isa<UndefValue>(inst->getOperand(1)))
    {
        return;
    }

    StructType* sTy = dyn_cast<StructType>(inst->getType());
    IGC_ASSERT_MESSAGE(sTy != nullptr,
        "ICE: invertvalue - only struct type is supported!");

    if (isLayoutStructType(sTy))
    {
        EmitInsertValueToLayoutStruct(inst);
        return;
    }

    const uint32_t nLanes = numLanes(m_currShader->m_SIMDSize);
    Value* src0 = inst->getOperand(0);
    Value* src1 = inst->getOperand(1);
    CVariable* EltV = GetSymbol(src1);
    CVariable* DstV = GetSymbol(inst);

    // For now, do not support uniform dst and non-uniform src1
    IGC_ASSERT_MESSAGE((!EltV->IsUniform() && DstV->IsUniform()) == false,
        "Can't insert vector value into a scalar struct!");

    auto verifyMayUnAlign = [this](CVariable* CVar, uint32_t ByteOffsets,
        StructType* STy, Type* MemberTy) {
#if defined(_DEBUG)
        if (MemberTy->isSingleValueType()) {
            Type* eTy = MemberTy->getScalarType();
            uint32_t eltBytes = (uint32_t)m_DL->getTypeStoreSize(eTy);
            uint32_t align = (uint32_t)MinAlign(eltBytes, ByteOffsets);
            IGC_ASSERT_MESSAGE(align >= eltBytes ||
                CVar->IsUniform() && STy->isPacked(),
                "Unaligned copy can happen only for uniform packed struct!");
        }
#endif
    };

    // Constants:
    //  1.  %8 = insval undef, float %2, 0
    //  2.  %8 = insval { i32 16, i32 undef, i32 undef, i32 undef }, i32 %2, 1
    if (Constant* CSt = dyn_cast<Constant>(src0))
    {
        if (!isa<UndefValue>(CSt))
        {
            const StructLayout* SL = m_DL->getStructLayout(sTy);
            // copy all valid const elements
            for (uint32_t i = 0; i < (uint32_t)sTy->getNumElements(); i++)
            {
                Constant* eCst = CSt->getAggregateElement(i);
                if (isa<UndefValue>(eCst))
                    continue;

                CVariable* eltSrc = GetSymbol(eCst);
                Type* ty = sTy->getElementType(i);
                uint32_t byteOffset = (uint32_t)SL->getElementOffset(i);
                uint32_t cvar_off =
                    byteOffset * (DstV->IsUniform() ? 1 : nLanes);

                emitMayUnalignedVectorCopy(DstV, cvar_off, eltSrc, 0, ty);

                verifyMayUnAlign(DstV, cvar_off, sTy, ty);
            }
        }
    }
    else
    {
        CVariable* SrcV = GetSymbol(src0);
        if (DstV != SrcV)
        {
            // Only copy SrcV's elements that have been initialized already.
            // For the example, DstV = %13, SrcV = %9;
            // 'toBeCopied' will be field {0, 1}, not {0,1,2,3}.
            //
            //  %8 = insertvalue undef, i32 %3, 0
            //  %9 = insertvalue %8, i32 % 4, 1
            //  ......
            //  %13 = insertvalue %9, i32 %scalar40, 2
            //  %14 = insertvalue %13, i32 %scalar41, 3
            //
            std::list<ArrayRef<unsigned>> toBeCopied;
            getAllDefinedMembers(src0, toBeCopied);
            for (const auto& II : toBeCopied)
            {
                // skip one that will be written by this inst
                auto theIdx = inst->getIndices();
                if (theIdx.equals(II)) {
                    continue;
                }

                Type* ty;
                uint32_t byteOffset;
                getStructMemberByteOffsetAndType_1(m_DL, sTy, II, ty, byteOffset);

                uint32_t d_off =
                    byteOffset * (DstV->IsUniform() ? 1 : nLanes);
                uint32_t s_off =
                    byteOffset * (SrcV->IsUniform() ? 1 : nLanes);

                emitMayUnalignedVectorCopy(DstV, d_off, SrcV, s_off, ty);

                verifyMayUnAlign(DstV, d_off, sTy, ty);
                verifyMayUnAlign(SrcV, s_off, sTy, ty);
            }
        }
    }

    Type* ty;
    uint32_t byteOffset;
    getStructMemberByteOffsetAndType_1(m_DL, sTy, inst->getIndices(), ty, byteOffset);
    uint32_t d_off =
        byteOffset * (DstV->IsUniform() ? 1 : nLanes);
    emitMayUnalignedVectorCopy(DstV, d_off, EltV, 0, ty);

    verifyMayUnAlign(DstV, d_off, sTy, ty);
}

void EmitPass::EmitExtractValueFromStruct(llvm::ExtractValueInst* EI)
{
    auto verifyMayUnAlign = [this](CVariable* CVar, uint32_t ByteOffsets,
        StructType* STy, Type* MemberTy) {
#if defined(_DEBUG)
        if (MemberTy->isSingleValueType()) {
            Type* eTy = MemberTy->getScalarType();
            uint32_t eltBytes = (uint32_t)m_DL->getTypeStoreSize(eTy);
            uint32_t align = (uint32_t)MinAlign(eltBytes, ByteOffsets);
            IGC_ASSERT_MESSAGE(align >= eltBytes ||
                CVar->IsUniform() && STy->isPacked(),
                "Unaligned copy can happen only for uniform packed struct!");
        }
#endif
    };

    Value* src0 = EI->getOperand(0);
    IGC_ASSERT(src0->getType()->isStructTy());
    StructType* sTy = cast<StructType>(src0->getType());

    if (isLayoutStructType(sTy))
    {
        EmitExtractValueFromLayoutStruct(EI);
        return;
    }

    auto& DL = m_currShader->entry->getParent()->getDataLayout();
    const StructLayout* SL = DL.getStructLayout(sTy);
    unsigned idx = *EI->idx_begin();
    const unsigned nLanes = numLanes(m_currShader->m_SIMDSize);

    if (Constant* C = dyn_cast<Constant>(src0))
    {
        if (!isa<UndefValue>(C))
        {
            const bool dstUniform = m_destination->IsUniform();
            CVariable* elementSrc = GetSymbol(C->getAggregateElement(idx));
            Type* ty = sTy->getStructElementType(idx);
            emitCopyAll(m_destination, elementSrc, ty);
        }
        return;
    }

    CVariable* SrcV = GetSymbol(EI->getOperand(0));
    const bool srcUniform = SrcV->IsUniform();
    // For now, do not support uniform dst and non-uniform src
    IGC_ASSERT(!(m_destination->IsUniform() && !srcUniform));

    Type* ty = sTy->getStructElementType(idx);
    uint32_t elementOffset = (uint32_t)SL->getElementOffset(idx);

    uint32_t cvar_off = elementOffset * (srcUniform ? 1 : nLanes);
    emitMayUnalignedVectorCopy(m_destination, 0, SrcV, cvar_off, ty);

    verifyMayUnAlign(m_destination, cvar_off, sTy, ty);
}

// Functions:
//   EmitInsertValueToLayoutStruct()
//   EmitExtractValueFromLayoutStruct()
//
// Only support a simple struct, identified by prefix ""__StructSOALayout_",
// whose members are not of aggregate types except a special identfied member
// struct type whose name starts with "__StructAOSLayout_".
//
// Such a struct's direct members all have the same size, either 32bit or
// 64bit. It is created by LdSdCombine optimization pass to generate more
// efficient load and stores. If its member is of struct type, that struct
// member's members must be primitive types (including vector), that is,
// the struct nesting level is at most 2. That struct-typed member is viewed
// either as i32 if its size is 32 bits or as i64 if its size is 64 bits.
// For example,
//    struct __StructSOALayout_ {
//       struct __StructAOSLayout_ { i16 a, i8 b, i8 c}
//    }
//    struct __StructSOALayout_.1 {
//       struct __StructAOSLayout_.2 { i32, i16, i16 }
//    }
//
// The struct layout in GRF is SOA, shown in its name, for all its direct
// members. Its struct-typed member is laid out in AOS, just as they are
// viewed as i32 or i64.
//
void EmitPass::EmitInsertValueToLayoutStruct(InsertValueInst* IVI)
{
    if (isa<UndefValue>(IVI->getOperand(1)))
    {
        return;
    }

    StructType* stTy = dyn_cast<StructType>(IVI->getType());
    IGC_ASSERT_MESSAGE(stTy != nullptr,
        "ICE: invertvalue - only struct type is supported!");
    IGC_ASSERT_MESSAGE(isLayoutStructType(stTy), "ICE: not right struct type!");

    const uint32_t nLanes = numLanes(m_currShader->m_SIMDSize);
    Value* src0 = IVI->getOperand(0);
    Value* src1 = IVI->getOperand(1);
    CVariable* EltV = GetSymbol(src1);
    CVariable* DstV = GetSymbol(IVI);

    // For now, do not support uniform dst and non-uniform src1
    const Type* src1Ty = src1->getType();
    IGC_ASSERT_MESSAGE(src1Ty->isSingleValueType(),
        "Layout struct's members are of single value type.");
    IGC_ASSERT_MESSAGE(!(!EltV->IsUniform() && DstV->IsUniform()),
        "Can't insert non-uniform value into a uniform struct!");

    // Constants:
    //  1.  %8 = insval undef, float %2, 0
    //  2.  %8 = insval <{ i32 16, i32 undef, i32 undef, i32 undef }>, i32 %2, 1
    if (Constant* CSt = dyn_cast<Constant>(src0))
    {
        if (!isa<UndefValue>(CSt))
        {
            emitCopyToOrFromLayoutStruct(IVI, src0);
        }
    }
    else
    {
        CVariable* SrcV = GetSymbol(src0);
        if (DstV != SrcV && DstV->IsUniform() && SrcV->IsUniform())
        {
            emitCopyToOrFromLayoutStruct(IVI, src0);
        }
        else if (DstV != SrcV)
        {
            // Most often, SrcV has just one defined value and calling
            // emitCopyToOrFromLayoutStruct() would copy all, thus special
            // handling here to avoid copy undefined values.
            std::list<ArrayRef<unsigned>> toBeCopied;
            getAllDefinedMembers(src0, toBeCopied);
            for (const auto& II : toBeCopied)
            {
                Type* ty0;
                Type* ty1;
                uint32_t byteOff0, byteOff1;
                getStructMemberOffsetAndType_2(m_DL,
                    stTy, II, ty0, byteOff0, ty1, byteOff1);
                Type* ty = (ty1 == nullptr ? ty0 : ty1);
                auto iVTy = dyn_cast<IGCLLVM::FixedVectorType>(ty);
                uint32_t n = iVTy ? (uint32_t)iVTy->getNumElements() : 1;
                CVariable* eltDst = m_currShader->GetNewAlias(
                    DstV,
                    m_currShader->GetType(ty),
                    (byteOff0 * (DstV->IsUniform() ? 1 : nLanes)) + byteOff1,
                    n * (DstV->IsUniform() ? 1 : nLanes));
                CVariable* eltSrc = m_currShader->GetNewAlias(
                    SrcV,
                    m_currShader->GetType(ty),
                    (byteOff0 * (SrcV->IsUniform() ? 1 : nLanes)) + byteOff1,
                    n * (SrcV->IsUniform() ? 1 : nLanes));
                if (II.size() == 2) {
                    uint32_t AOSStBytes = (uint32_t)m_DL->getTypeStoreSize(ty0);
                    emitVectorCopyToAOS(AOSStBytes, eltDst, eltSrc, n);
                }
                else {
                    emitVectorCopy(eltDst, eltSrc, n);
                }
            }
        }
    }

    Type* ty0;
    Type* ty1;
    uint32_t byteOff0, byteOff1;
    getStructMemberOffsetAndType_2(m_DL,
        stTy, IVI->getIndices(), ty0, byteOff0, ty1, byteOff1);

    Type* ty = (ty1 == nullptr ? ty0 : ty1);
    IGC_ASSERT(ty == src1Ty);
    auto iVTy = dyn_cast<IGCLLVM::FixedVectorType>(ty);
    uint32_t n = iVTy ? (uint32_t)iVTy->getNumElements() : 1;

    CVariable* eltDst = m_currShader->GetNewAlias(
        DstV,
        EltV->GetType(),
        (byteOff0 * (DstV->IsUniform() ? 1 : nLanes)) + byteOff1,
        n * (DstV->IsUniform() ? 1 : nLanes));

    if (IVI->getIndices().size() == 2) {
        uint32_t AOSStBytes = (uint32_t)m_DL->getTypeStoreSize(ty0);
        emitVectorCopyToAOS(AOSStBytes, eltDst, EltV, n);
    }
    else {
        emitVectorCopy(eltDst, EltV, n);
    }
}

void EmitPass::EmitExtractValueFromLayoutStruct(ExtractValueInst* EVI)
{
    Value* VecOprd = EVI->getAggregateOperand();
    StructType* stTy = dyn_cast<StructType>(VecOprd->getType());
    ArrayRef<unsigned> Indices = EVI->getIndices();
    uint32_t nIndices = Indices.size();

    // sanity check
    IGC_ASSERT_MESSAGE((stTy != nullptr && isLayoutStructType(stTy)),
        "ICE: this extractvalue should apply on layout struct type!");
    if (isa<Constant>(VecOprd) || nIndices > 2 || nIndices == 0) {
        IGC_ASSERT_UNREACHABLE();
        return;
    }

    const uint32_t nLanes = numLanes(m_currShader->m_SIMDSize);
    CVariable* SrcV = GetSymbol(VecOprd);
    CVariable* DstV = GetSymbol(EVI);

    // For now, do not support uniform dst and non-uniform src1
    const Type* dstTy = EVI->getType();
    IGC_ASSERT_MESSAGE(dstTy->isSingleValueType(),
        "ExtractValue for Layout struct should be of single value type.");
    IGC_ASSERT_MESSAGE(!(!SrcV->IsUniform() && DstV->IsUniform()),
        "Can't extract value from non-uniform struct into a uniform value!");

    Type* ty0;
    Type* ty1;
    uint32_t byteOff0, byteOff1;
    getStructMemberOffsetAndType_2(m_DL,
        stTy, Indices, ty0, byteOff0, ty1, byteOff1);
    Type* ty = (ty1 == nullptr ? ty0 : ty1);
    IGC_ASSERT(dstTy == ty);
    auto iVTy = dyn_cast<IGCLLVM::FixedVectorType>(ty);
    uint32_t n = iVTy ? (uint32_t)iVTy->getNumElements() : 1;
    CVariable * srcVal = m_currShader->GetNewAlias(
        SrcV,
        DstV->GetType(),
        (byteOff0 * (SrcV->IsUniform() ? 1 : nLanes)) + byteOff1,
        n * (SrcV->IsUniform() ? 1 : nLanes));
    if (nIndices == 2) {
        uint32_t AOSStBytes = (uint32_t)m_DL->getTypeStoreSize(ty0);
        emitVectorCopyFromAOS(AOSStBytes, DstV, srcVal, n);
    }
    else {
        emitVectorCopy(DstV, srcVal, n);
    }
}

void EmitPass::EmitAddPair(GenIntrinsicInst* GII, const SSource Sources[4], const DstModifier& DstMod) {
    auto [L, H] = getPairOutput(GII);
    CVariable* Lo = L ? GetSymbol(L) : nullptr;
    CVariable* Hi = H ? GetSymbol(H) : nullptr;
    IGC_ASSERT(Lo == m_destination || Hi == m_destination);

    CVariable* L0 = GetSrcVariable(Sources[0]);
    CVariable* H0 = GetSrcVariable(Sources[1]);
    CVariable* L1 = GetSrcVariable(Sources[2]);
    CVariable* H1 = GetSrcVariable(Sources[3]);
    for (unsigned srcId = 0; srcId < 4; ++srcId) {
        SetSourceModifiers(srcId, Sources[srcId]);
    }

    m_encoder->AddPair(Lo, Hi, L0, H0, L1, H1);
    m_encoder->Push();
}

void EmitPass::EmitSubPair(GenIntrinsicInst* GII, const SSource Sources[4], const DstModifier& DstMod) {
    auto [L, H] = getPairOutput(GII);
    CVariable* Lo = L ? GetSymbol(L) : nullptr;
    CVariable* Hi = H ? GetSymbol(H) : nullptr;
    IGC_ASSERT(Lo == m_destination || Hi == m_destination);

    CVariable* L0 = GetSrcVariable(Sources[0]);
    CVariable* H0 = GetSrcVariable(Sources[1]);
    CVariable* L1 = GetSrcVariable(Sources[2]);
    CVariable* H1 = GetSrcVariable(Sources[3]);

    m_encoder->SubPair(Lo, Hi, L0, H0, L1, H1);
    m_encoder->Push();
}

void EmitPass::EmitMulPair(GenIntrinsicInst* GII, const SSource Sources[4], const DstModifier& DstMod)
{
    auto [L, H] = getPairOutput(GII);
    CVariable* Lo = L ? GetSymbol(L) : nullptr;
    CVariable* Hi = H ? GetSymbol(H) : nullptr;
    IGC_ASSERT(Lo == m_destination || Hi == m_destination);

    CVariable* L0 = GetSrcVariable(Sources[0]);
    CVariable* H0 = GetSrcVariable(Sources[1]);
    CVariable* L1 = GetSrcVariable(Sources[2]);
    CVariable* H1 = GetSrcVariable(Sources[3]);

    // Use `UD` for Lo(s).
    if (Lo && Lo->GetType() != ISA_TYPE_UD) Lo = m_currShader->BitCast(Lo, ISA_TYPE_UD);
    if (L0->GetType() != ISA_TYPE_UD) L0 = m_currShader->BitCast(L0, ISA_TYPE_UD);
    if (L1->GetType() != ISA_TYPE_UD) L1 = m_currShader->BitCast(L1, ISA_TYPE_UD);

    if (Lo == nullptr && Hi == nullptr)
    {
        return;
    }

    if (Lo != nullptr && Hi == nullptr)
    {
        // Lo = A * B
        SetSourceModifiers(0, Sources[0]);
        SetSourceModifiers(1, Sources[2]);
        m_encoder->Mul(Lo, L0, L1);
        m_encoder->Push();
        return;
    }

    IGC_ASSERT(Hi != nullptr);

    // Algorithm:
    //    AB   - L0, L1
    //    CD   - H0, H1
    //   ----
    //     E
    //    F
    //    G
    //   H     - 'H' spills into bit 65 - only needed if overflow detection is required
    // --------
    // dstLow = E
    // dstHigh = F + G + carry

    CVariable* dstLoTmp = nullptr;
    CVariable* dstHiTmp = m_currShader->GetNewVariable(Hi, CName(Hi->getName(), "int64Hi"));

    if (Lo == nullptr)
    {
        SetSourceModifiers(0, Sources[0]);
        SetSourceModifiers(1, Sources[2]);
        // Cr = carry(A * B)
        m_encoder->MulH(dstHiTmp, L0, L1);
        m_encoder->Push();
    }
    else
    {
        dstLoTmp = m_currShader->GetNewVariable(Lo, CName(Lo->getName(), "int64Lo"));

        // For those platforms natively not support DW-DW multiply, use vISA madw instruction instead of mul/mulh to get better performance.
        if (m_currShader->m_Platform->noNativeDwordMulSupport())
        {
            // (Cr, E) = A * B
            // dst size should be GRF-aligned and doubled as it has both low and high results.
            // We must make the dst element number is numDWPerGRF aligned. For example, if the madw is SIMD1,
            // the dst has only 1 DW as low result in 1 GRF and only 1 DW as high result in another GRF. We should
            // set the dst as (numDWPerGRF * 2) element but not 2 DW elements. This is required by madw.
            auto numDWPerGRF = getGRFSize() / SIZE_DWORD;
            auto numElements = iSTD::Align(Lo->GetNumberElement(), numDWPerGRF);
            CVariable* dstTmp = m_currShader->GetNewVariable(numElements * 2, ISA_TYPE_UD, EALIGN_GRF, Lo->IsUniform(),
                CName(Lo->getName(), "int64Tmp"));
            CVariable* zero = m_currShader->ImmToVariable(0, ISA_TYPE_UD);
            SetSourceModifiers(0, Sources[0]);
            SetSourceModifiers(1, Sources[2]);
            m_encoder->Madw(dstTmp, L0, L1, zero);

            // dstLow = E
            m_encoder->SetSrcRegion(0, 1, 1, 0);
            m_encoder->Copy(dstLoTmp, dstTmp);
            m_encoder->Push();

            // dstHigh = Cr
            uint regOffset = (uint)std::ceil((float)(numElements * CEncoder::GetCISADataTypeSize(ISA_TYPE_UD)) / getGRFSize());
            m_encoder->SetSrcSubVar(0, regOffset);
            m_encoder->SetSrcRegion(0, 1, 1, 0);
            m_encoder->Copy(dstHiTmp, dstTmp);
            m_encoder->Push();
        }
        else
        {
            // E = A * B
            SetSourceModifiers(0, Sources[0]);
            SetSourceModifiers(1, Sources[2]);
            m_encoder->Mul(dstLoTmp, L0, L1);
            m_encoder->Push();

            // Cr = carry(A * B)
            SetSourceModifiers(0, Sources[0]);
            SetSourceModifiers(1, Sources[2]);
            m_encoder->MulH(dstHiTmp, L0, L1);
            m_encoder->Push();
        }
    }

    // F = A * D
    CVariable* T0 = m_currShader->GetNewVariable(Hi, CName(Hi->getName(), "int64HiLH"));
    SetSourceModifiers(0, Sources[0]);
    SetSourceModifiers(1, Sources[3]);
    m_encoder->Mul(T0, L0, H1);
    m_encoder->Push();

    // dstHigh = Cr + F
    m_encoder->Add(dstHiTmp, dstHiTmp, T0);
    m_encoder->Push();

    // G = B * C
    SetSourceModifiers(0, Sources[2]);
    SetSourceModifiers(1, Sources[1]);
    m_encoder->Mul(T0, L1, H0);
    m_encoder->Push();

    if (Lo != nullptr)
    {
        IGC_ASSERT(dstLoTmp);
        m_encoder->SetSrcRegion(0, 1, 1, 0);
        m_encoder->Copy(Lo, dstLoTmp);
        m_encoder->Push();
    }

    // dstHigh = Cr + F + G
    m_encoder->Add(Hi, dstHiTmp, T0);
    m_encoder->Push();
}

void EmitPass::EmitPtrToPair(GenIntrinsicInst* GII, const SSource Sources[1], const DstModifier& DstMod) {
    auto [L, H] = getPairOutput(GII);
    CVariable* Lo = L ? GetSymbol(L) : nullptr;
    CVariable* Hi = H ? GetSymbol(H) : nullptr;
    IGC_ASSERT(Lo == m_destination || Hi == m_destination);

    CVariable* Src = GetSrcVariable(Sources[0]);
    Src = m_currShader->BitCast(Src, m_destination->GetType());

    unsigned AS = Sources[0].value->getType()->getPointerAddressSpace();
    bool isPtr32 = m_currShader->GetContext()->getRegisterPointerSizeInBits(AS) == 32;

    if (Lo) {
        if (isPtr32) {
            m_encoder->Cast(Lo, Src);
            m_encoder->Push();
        }
        else {
            if (!Src->IsUniform())
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
        }
        else {
            if (!Src->IsUniform())
                m_encoder->SetSrcRegion(0, 2, 1, 0);
            m_encoder->SetSrcSubReg(0, 1);
            m_encoder->Copy(Hi, Src);
            m_encoder->Push();
        }
    }
}


void EmitPass::EmitSIToFPZExt(const SSource& source, const DstModifier& dstMod) {
    CVariable* flag = GetSrcVariable(source);
    CVariable* one = m_currShader->ImmToVariable(getFPOne(m_destination->GetType()), m_destination->GetType());
    CVariable* zero = m_currShader->ImmToVariable(0, m_destination->GetType());
    m_encoder->SetDstModifier(dstMod);
    m_encoder->Select(flag, m_destination, one, zero);
    m_encoder->Push();
}

void EmitPass::emitCtlz(const SSource& source)
{
    // This does not go through the standard EmitAluIntrinsic pass because
    // that creates a redundant SetP due to an unused i1 literal.
    CVariable* src = GetSrcVariable(source);
    src = m_currShader->BitCast(src, GetUnsignedType(src->GetType()));
    CVariable* dst = m_currShader->BitCast(m_destination, GetUnsignedType(m_destination->GetType()));
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
        IGC_ASSERT_MESSAGE(refImgBTI->GetImmediateValue() + 1 == bwdRefImgBTI->GetImmediateValue(), "refImg BTI and bwdRefImg BTI are not consecutive!");
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
    if (inputVar->GetSize() > (regs2snd * getGRFSize()))
    {
        inputVar = m_currShader->GetNewAlias(inputVar, ISA_TYPE_UD, 0, regs2snd * 8);
    }

    CVariable* outputVar = m_destination;

    if (outputVar->GetSize() > (regs2rcv * getGRFSize()))
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
    CVariable* outputVar = GetSymbol(inst->getArgOperand(0));

    CVariable* uniInputVar = GetSymbol(inst->getArgOperand(1));
    CVariable* imeInputVar = GetSymbol(inst->getArgOperand(2));

    CVariable* srcImgBTI = GetSymbol(inst->getArgOperand(3));
    CVariable* refImgBTI = GetSymbol(inst->getArgOperand(4));
    CVariable* bwdRefImgBTI = has_bwd_ref_image ? GetSymbol(inst->getArgOperand(5)) : nullptr;
    // If the BTIs aren't consecutive then we can't do VME.
    IGC_ASSERT_MESSAGE(srcImgBTI->GetImmediateValue() + 1 == refImgBTI->GetImmediateValue(), "srcImg BTI and refImg BTI are not consecutive!");
    if (bwdRefImgBTI != nullptr) {
        IGC_ASSERT_MESSAGE(srcImgBTI->GetImmediateValue() + 2 == bwdRefImgBTI->GetImmediateValue(), "srcImg BTI and bwdRefImg BTI are not consecutive!");
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
        IGC_ASSERT_MESSAGE(SIZE_DWORD == CEncoder::GetCISADataTypeSize(costVisaTy),
            "VME IME's cost center var has wrong type!");
        CVariable* newVar = m_currShader->GetNewVariable(8, ISA_TYPE_UD, EALIGN_GRF, CName::NONE);

        m_encoder->SetNoMask();
        m_encoder->SetSimdSize(SIMDMode::SIMD8);
        m_encoder->Copy(newVar, costCenterVar);
        m_encoder->Push();

        costCenterVar = newVar;
    }

    unsigned char streamMode = VME_STREAM_DISABLE;
    unsigned char searchControlMode = VME_SEARCH_SINGLE_REF_SINGLE_REC_SINGLE_START;

    // Force write the costCenter here.  I'd like to have uniInputVar setup before calling
    // emitVMESendIME so we don't burn movs each time we call this but CM uses it for now.
    // Fix later.
    {
        CVariable* uniAlias = m_currShader->GetNewAlias(uniInputVar, ISA_TYPE_UD, 3 * getGRFSize(), 8);
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
    CVariable* outputVar = GetSymbol(inst->getArgOperand(0));

    CVariable* uniInputVar = GetSymbol(inst->getArgOperand(1));
    CVariable* fbrInputVar = GetSymbol(inst->getArgOperand(2));

    CVariable* srcImgBTI = GetSymbol(inst->getArgOperand(3));
    CVariable* refImgBTI = GetSymbol(inst->getArgOperand(4));
    // If the BTIs aren't consecutive then we can't do VME.
    IGC_ASSERT_MESSAGE(srcImgBTI->GetImmediateValue() + 1 == refImgBTI->GetImmediateValue(), "srcImg BTI and refImg BTI are not consecutive!");

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
        IGC_ASSERT_MESSAGE(refImgBTI->GetImmediateValue() + 1 == bwdRefImgBTI->GetImmediateValue(), "refImg BTI and bwdRefImg BTI are not consecutive!");
    }

    const uint32_t regs2rcv = (7 + 0), regs2snd = (4 + 4);
    const uint32_t desc = VMEDescriptor(VME_STREAM_DISABLE, (uint32_t)(srcImgBTI->GetImmediateValue()),
        EU_GEN7_5_VME_MESSAGE_FBR, regs2snd, regs2rcv);

    CVariable* messDesc = m_currShader->ImmToVariable(desc, ISA_TYPE_UD);

    CVariable* outputVar = m_destination;

    if (outputVar->GetSize() > (regs2rcv * getGRFSize()))
    {
        outputVar = m_currShader->GetNewAlias(outputVar, ISA_TYPE_UD, 0, regs2rcv * 8);
    }

    m_encoder->Send(outputVar, inputVar, EU_MESSAGE_TARGET_SFID_CRE, messDesc, false);
    m_encoder->Push();

    return;
}

void EmitPass::emitVMESendSIC(GenIntrinsicInst* inst)
{
    CVariable* outputVar = GetSymbol(inst->getArgOperand(0));
    CVariable* uniInputVar = GetSymbol(inst->getArgOperand(1));
    CVariable* sicInputVar = GetSymbol(inst->getArgOperand(2));
    CVariable* srcImgBTI = GetSymbol(inst->getArgOperand(3));
    CVariable* ref0ImgBTI = GetSymbol(inst->getArgOperand(4));
    CVariable* ref1ImgBTI = GetSymbol(inst->getArgOperand(5));
    // If the BTIs aren't consecutive then we can't do VME.
    IGC_ASSERT_MESSAGE(srcImgBTI->GetImmediateValue() + 1 == ref0ImgBTI->GetImmediateValue(), "srcImg BTI and ref0Img BTI are not consecutive!");
    // In the non-bidirectional case, we just pass the same reference image into the
    // forward and backward slots.
    if (ref0ImgBTI->GetImmediateValue() != ref1ImgBTI->GetImmediateValue())
    {
        IGC_ASSERT_MESSAGE(ref0ImgBTI->GetImmediateValue() + 1 == ref1ImgBTI->GetImmediateValue(), "ref0Img BTI and ref1Img BTI are not consecutive!");
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
        IGC_ASSERT_MESSAGE(fwdRefImgBTI->GetImmediateValue() + 1 == bwdRefImgBTI->GetImmediateValue(), "refImg BTI and bwdRefImg BTI are not consecutive!");
    }

    // If the BTIs aren't consecutive then we can't do VME. And this only applies to case
    // when either fwdRefImg or bwdRefImg is presented.
    if (srcImgBTI->GetImmediateValue() != fwdRefImgBTI->GetImmediateValue())
    {
        IGC_ASSERT_MESSAGE(srcImgBTI->GetImmediateValue() + 1 == fwdRefImgBTI->GetImmediateValue(), "srcImg BTI and refImg BTI are not consecutive!");

        if (fwdRefImgBTI->GetImmediateValue() != bwdRefImgBTI->GetImmediateValue())
        {
            IGC_ASSERT_MESSAGE(srcImgBTI->GetImmediateValue() + 2 == bwdRefImgBTI->GetImmediateValue(), "srcImg BTI and bwdRefImg BTI are not consecutive!");
        }
    }

    const uint32_t regs2rcv = (7 + 0), regs2snd = (4 + 4);
    const uint32_t desc = VMEDescriptor(VME_STREAM_DISABLE, (uint32_t)(srcImgBTI->GetImmediateValue()),
        EU_GEN7_5_VME_MESSAGE_SIC, regs2snd, regs2rcv);

    CVariable* messDesc = m_currShader->ImmToVariable(desc, ISA_TYPE_UD);

    CVariable* outputVar = m_destination;

    if (outputVar->GetSize() > (regs2rcv * getGRFSize()))
    {
        outputVar = m_currShader->GetNewAlias(outputVar, ISA_TYPE_UD, 0, regs2rcv * 8);
    }

    m_encoder->Send(outputVar, inputVar, EU_MESSAGE_TARGET_SFID_CRE, messDesc, false);
    m_encoder->Push();

    return;
}

void EmitPass::emitCreateMessagePhases(GenIntrinsicInst* inst) {
    IGC_ASSERT_MESSAGE((m_destination->GetType() == ISA_TYPE_UD || m_destination->GetType() == ISA_TYPE_D), "Destination type is expected to be UD or D!");
    IGC_ASSERT_MESSAGE(isa<ConstantInt>(inst->getArgOperand(0)), "Num phases expected to be const!");
    unsigned int numPhases = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(0))->getZExtValue());

    const uint16_t numSimdLanes = numLanes(m_SimdMode);
    IGC_ASSERT(0 < numSimdLanes);
    unsigned int numWideSimdIters = numPhases * 8 / numSimdLanes;
    unsigned int remSimd8Iters = (numPhases * 8 % numSimdLanes) / 8;

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
        IGC_ASSERT_MESSAGE(0, "unknown size");
        return ISA_TYPE_UD;
    }
}

void EmitPass::emitSimdMediaRegionCopy(llvm::GenIntrinsicInst* inst)
{
    CVariable* pDst = GetSymbol(inst->getArgOperand(0));
    unsigned dbyteoffset = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(1))->getZExtValue());
    unsigned dstride = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(2))->getZExtValue());
    unsigned dnumelem = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(3))->getZExtValue());
    CVariable* pSrc = GetSymbol(inst->getArgOperand(4));
    Value* sbyteoffset = inst->getArgOperand(5);
    unsigned vstride = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(6))->getZExtValue());
    unsigned width = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(7))->getZExtValue());
    unsigned hstride = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(8))->getZExtValue());
    unsigned typesize = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(9))->getZExtValue());
    unsigned execsize = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(10))->getZExtValue());
    unsigned snumelem = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(11))->getZExtValue());

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
            false,
            inst->getName());

        m_encoder->AddrAdd(pSrcOffset, pSrc, m_currShader->BitCast(GetSymbol(sbyteoffset), ISA_TYPE_UW));
        setup();
        m_encoder->Copy(pDstOffset, pSrcOffset);
        m_encoder->Push();
    }
}

void EmitPass::emitExtractMVAndSAD(llvm::GenIntrinsicInst* inst)
{
    CVariable* pMV = GetSymbol(inst->getArgOperand(0));
    CVariable* pSAD = GetSymbol(inst->getArgOperand(1));
    CVariable* pResult = GetSymbol(inst->getArgOperand(2));
    CVariable* pBlockType = GetSymbol(inst->getArgOperand(3));

    // W5.0 - W5.7 from Return Data Message Phases (InterDistortion)
    CVariable* pDist = m_currShader->GetNewAlias(pResult, ISA_TYPE_UW, 5 * getGRFSize(), 16);
    CVariable* pSADAlias = m_currShader->GetNewAlias(pSAD, ISA_TYPE_UW, 0, 16);

    CVariable* pFlag = m_currShader->GetNewVariable(
        16,
        ISA_TYPE_BOOL,
        EALIGN_GRF,
        CName::NONE);

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
            (1 * getGRFSize()) + (2 * i * getGRFSize()),
            16);
        CVariable* pMVOffset = m_currShader->GetNewAlias(pMV, ISA_TYPE_UD,
            2 * i * getGRFSize(),
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
    CVariable* pMVCurr = GetSymbol(inst->getArgOperand(0));
    CVariable* pSADCurr = GetSymbol(inst->getArgOperand(1));
    CVariable* pMVMin = GetSymbol(inst->getArgOperand(2));
    CVariable* pSADMin = GetSymbol(inst->getArgOperand(3));

    CVariable* pFlag = m_currShader->GetNewVariable(
        16,
        ISA_TYPE_BOOL,
        EALIGN_GRF,
        CName::NONE);

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
    if (m_currShader->m_Platform->hasNoFullI64Support()) {
        CVariable* pMVMinAlias = m_currShader->GetNewAlias(pMVMin, ISA_TYPE_UD, 0, 32);
        CVariable* pMVCurrAlias = m_currShader->GetNewAlias(pMVCurr, ISA_TYPE_UD, 0, 32);

        //(W&fX.X) mov(8|M0) r(DST).0<1>:f    r(SRC).0<2;1,0>:f
        m_encoder->SetNoMask();
        m_encoder->SetSimdSize(SIMDMode::SIMD8);
        m_encoder->SetSrcRegion(0, 2, 1, 0);
        m_encoder->SetSrcRegion(1, 2, 1, 0);
        m_encoder->SetDstRegion(2);
        m_encoder->Select(pFlag, pMVMinAlias, pMVCurrAlias, pMVMinAlias);
        m_encoder->Push();

        //(W&fX.X) mov(8|M0) r(DST).1<1>:f    r(SRC).1<2;1,0>:f
        m_encoder->SetNoMask();
        m_encoder->SetSimdSize(SIMDMode::SIMD8);
        m_encoder->SetSrcRegion(0, 2, 1, 0);
        m_encoder->SetSrcRegion(1, 2, 1, 0);
        m_encoder->SetDstRegion(2);
        m_encoder->SetSrcSubReg(0, 1);
        m_encoder->SetSrcSubReg(1, 1);
        m_encoder->SetDstSubReg(1);
        m_encoder->Select(pFlag, pMVMinAlias, pMVCurrAlias, pMVMinAlias);
        m_encoder->Push();

        //(W&fX.X) mov(8|M8) r(DST+2).0<2>:f    r(SRC+2).0<2;1,0>:f
        m_encoder->SetNoMask();
        m_encoder->SetSimdSize(SIMDMode::SIMD8);
        m_encoder->SetMask(EMASK_Q2);
        m_encoder->SetSrcSubVar(0, 2);
        m_encoder->SetSrcSubVar(1, 2);
        m_encoder->SetDstSubVar(2);
        m_encoder->SetSrcRegion(0, 2, 1, 0);
        m_encoder->SetSrcRegion(1, 2, 1, 0);
        m_encoder->SetDstRegion(2);
        m_encoder->Select(pFlag, pMVMinAlias, pMVCurrAlias, pMVMinAlias);
        m_encoder->Push();

        //(W&fX.X) mov(8|M8) r(DST+2).1<2>:f    r(SRC+2).1<2;1,0>:f
        m_encoder->SetNoMask();
        m_encoder->SetSimdSize(SIMDMode::SIMD8);
        m_encoder->SetMask(EMASK_Q2);
        m_encoder->SetSrcSubVar(0, 2);
        m_encoder->SetSrcSubVar(1, 2);
        m_encoder->SetDstSubVar(2);
        m_encoder->SetSrcRegion(0, 2, 1, 0);
        m_encoder->SetSrcRegion(1, 2, 1, 0);
        m_encoder->SetDstRegion(2);
        m_encoder->SetSrcSubReg(0, 1);
        m_encoder->SetSrcSubReg(1, 1);
        m_encoder->SetDstSubReg(1);
        m_encoder->Select(pFlag, pMVMinAlias, pMVCurrAlias, pMVMinAlias);
        m_encoder->Push();
    }
    else {
        CVariable* pMVCurrAlias = m_currShader->GetNewAlias(pMVCurr, ISA_TYPE_UQ, 0, 16);
        CVariable* pMVMinAlias = m_currShader->GetNewAlias(pMVMin, ISA_TYPE_UQ, 0, 16);

        m_encoder->SetNoMask();
        m_encoder->SetSimdSize(SIMDMode::SIMD16);
        m_encoder->Select(pFlag, pMVMinAlias, pMVCurrAlias, pMVMinAlias);
        m_encoder->Push();
    }
}

static bool SameVar(CVariable* A, CVariable* B)
{
    A = (A->GetAlias() && A->GetAliasOffset() == 0) ? A->GetAlias() : A;
    B = (B->GetAlias() && B->GetAliasOffset() == 0) ? B->GetAlias() : B;

    return A == B;
}

void EmitPass::emitSimdSetMessagePhase(llvm::GenIntrinsicInst* inst) {
    CVariable* messagePhases = GetSymbol(inst->getArgOperand(0));
    const uint32_t phaseIndex = int_cast<uint32_t>(cast<ConstantInt>(inst->getArgOperand(1))->getZExtValue());
    const uint32_t numPhases = int_cast<uint32_t>(cast<ConstantInt>(inst->getArgOperand(2))->getZExtValue());
    const uint32_t dstSubReg = int_cast<uint32_t>(cast<ConstantInt>(inst->getArgOperand(3))->getZExtValue());
    const uint32_t numLanesPerPhase = int_cast<uint32_t>(cast<ConstantInt>(inst->getArgOperand(4))->getZExtValue());
    const SIMDMode simdMode = lanesToSIMDMode(numLanesPerPhase);
    Value* value = inst->getArgOperand(5);
    const uint16_t eltSizeInBytes = (uint16_t)m_DL->getTypeSizeInBits(value->getType()) / 8;
    const uint16_t numEltsPerPhase = getGRFSize() / eltSizeInBytes;
    const VISA_Type type = GetTypeFromSize(eltSizeInBytes);

    CVariable* val = GetSymbol(value);

    if (!SameVar(m_destination, messagePhases))
    {
        emitCopyAll(m_destination, messagePhases, inst->getArgOperand(0)->getType());
    }

    for (uint32_t i = 0; i < numPhases; ++i) {
        CVariable* src = val->IsUniform() ? val : m_currShader->GetNewAlias(val, type, i * getGRFSize(), numEltsPerPhase);
        CVariable* dst = m_currShader->GetNewAlias(m_destination, type, (i + phaseIndex) * getGRFSize(), numEltsPerPhase);

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
    const uint32_t numPhases = int_cast<uint32_t>(cast<ConstantInt>(inst->getArgOperand(2))->getZExtValue());
    const uint16_t eltSizeInBytes = (uint16_t)m_DL->getTypeSizeInBits(inst->getType()) / 8;
    const uint16_t numEltsPerPhase = getGRFSize() / eltSizeInBytes;
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
        IGC_ASSERT_MESSAGE(0, "Unhandled data type");
    }

    for (uint32_t i = 0; i < numPhases; ++i) {
        CVariable* src = m_currShader->GetNewAlias(messagePhases, type, (i + phaseIndex) * getGRFSize(), numEltsPerPhase);
        CVariable* dst = m_currShader->GetNewAlias(m_destination, type, i * getGRFSize(), numEltsPerPhase);

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

    IGC_ASSERT_MESSAGE(phaseIndex * getGRFSize() < messagePhases->GetSize(), "out of bounds!");

    CVariable* messagePhaseElem = m_currShader->GetNewAlias(messagePhases, type, phaseIndex * getGRFSize(), 1);

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

    CVariable* messagePhaseElem = m_currShader->GetNewAlias(messagePhases, type, phaseIndex * getGRFSize(), 1);
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

    IGC_ASSERT_MESSAGE(phaseIndex * getGRFSize() < messagePhases->GetSize(), "out of bounds!");

    if (!SameVar(m_destination, messagePhases))
    {
        emitCopyAll(m_destination, messagePhases, inst->getArgOperand(0)->getType());
    }

    CVariable* messagePhaseElem = m_currShader->GetNewAlias(m_destination, type, phaseIndex * getGRFSize(), 1);
    m_encoder->SetSimdSize(SIMDMode::SIMD1);
    m_encoder->SetNoMask();
    m_encoder->SetSrcRegion(0, 0, 1, 0);
    m_encoder->SetDstSubReg(phaseSubindex);
    m_encoder->Copy(messagePhaseElem, val);
    m_encoder->Push();
}

void EmitPass::emitSetMessagePhaseX_legacy(GenIntrinsicInst* inst)
{
    Type* pTy = inst->getArgOperand(IGCLLVM::getNumArgOperands(inst) - 1)->getType();
    unsigned size = pTy->getScalarSizeInBits() / 8;
    emitSetMessagePhaseType_legacy(inst, GetTypeFromSize(size));
}

void EmitPass::emitSetMessagePhaseX(GenIntrinsicInst* inst) {
    Type* pTy = inst->getArgOperand(IGCLLVM::getNumArgOperands(inst) - 1)->getType();
    unsigned size = pTy->getScalarSizeInBits() / 8;
    emitSetMessagePhaseType(inst, GetTypeFromSize(size));
}

void EmitPass::emitGetMessagePhase(llvm::GenIntrinsicInst* inst) {
    if (isa<UndefValue>(inst->getArgOperand(0)))
        return;

    CVariable* messagePhases = GetSymbol(inst->getArgOperand(0));
    unsigned int phaseIndex = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(1))->getZExtValue());

    IGC_ASSERT_MESSAGE(phaseIndex * getGRFSize() < messagePhases->GetSize(), "out of bounds!");

    CVariable* messagePhase = m_currShader->GetNewAlias(messagePhases, ISA_TYPE_UD, phaseIndex * getGRFSize(), 8);
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

    CVariable* messagePhase = m_currShader->GetNewAlias(messagePhases, ISA_TYPE_UD, phaseIndex * getGRFSize(), 8);
    m_encoder->SetSimdSize(SIMDMode::SIMD8);
    m_encoder->SetNoMask();
    m_encoder->Copy(messagePhase, val);
    m_encoder->Push();
}

void EmitPass::emitSetMessagePhase(llvm::GenIntrinsicInst* inst) {
    CVariable* messagePhases = GetSymbol(inst->getArgOperand(0));
    unsigned int phaseIndex = int_cast<unsigned>(cast<ConstantInt>(inst->getArgOperand(1))->getZExtValue());
    CVariable* val = GetSymbol(inst->getArgOperand(2));

    IGC_ASSERT_MESSAGE(phaseIndex * getGRFSize() < messagePhases->GetSize(), "out of bounds!");

    if (!SameVar(m_destination, messagePhases))
    {
        emitCopyAll(m_destination, messagePhases, inst->getArgOperand(0)->getType());
    }

    CVariable* messagePhase = m_currShader->GetNewAlias(m_destination, ISA_TYPE_UD, phaseIndex * getGRFSize(), 8);
    m_encoder->SetSimdSize(SIMDMode::SIMD8);
    m_encoder->SetNoMask();
    m_encoder->Copy(messagePhase, val);
    m_encoder->Push();
}

// VA
void EmitPass::emitVideoAnalyticSLM(llvm::GenIntrinsicInst* inst, const DWORD responseLen)
{
    int argNum = 0;
    CVariable* outputVar = GetSymbol(inst->getArgOperand(argNum++));
    CVariable* coords = GetSymbol(inst->getArgOperand(argNum++));
    CVariable* size = NULL;

    IGC_ASSERT_MESSAGE(!(m_currShader->m_State.m_dispatchSize == SIMDMode::SIMD32 && m_encoder->IsSecondHalf()), "VA Intrinsics are simd independent");
    GenISAIntrinsic::ID id = inst->getIntrinsicID();
    if (id == GenISAIntrinsic::GenISA_vaCentroid ||
        id == GenISAIntrinsic::GenISA_vaBoolCentroid ||
        id == GenISAIntrinsic::GenISA_vaBoolSum)
    {
        size = GetSymbol(inst->getArgOperand(argNum++));
    }

    CVariable* srcImg = GetSymbol(inst->getArgOperand(argNum++));

    // So far we support only one VA function per kernel, and other sample
    // messages are not supported when there is VA function within the kernel.
    // So, for now it should be fine to always use sampler 0 for VA functions.
    DWORD samplerIndex = 0;
    CVariable* sampler = m_currShader->ImmToVariable(samplerIndex, ISA_TYPE_UD);

    uint16_t newNumElems = int_cast<uint16_t>(responseLen * getGRFSize() / SIZE_DWORD);

    CVariable* vaResult = m_currShader->GetNewVariable(
        newNumElems,
        ISA_TYPE_UD,
        outputVar->GetAlign(),
        false,
        CName::NONE);

    if (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_vaConvolve)
    {
        CVariable* convResult = m_currShader->GetNewAlias(
            vaResult,
            ISA_TYPE_UW,
            0,
            newNumElems * 2);

        m_encoder->SendVideoAnalytic(inst, convResult, coords, size, srcImg, sampler);
    }
    else
    {
        m_encoder->SendVideoAnalytic(inst, vaResult, coords, size, srcImg, sampler);
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
        SIMDMode::SIMD8);
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
        btiIndex);

    DWORD exDescValue = EU_GEN7_5_MESSAGE_TARGET_DATA_PORT_DATA_CACHE_1;

    CVariable* desc = m_currShader->ImmToVariable(descValue, ISA_TYPE_UD);
    CVariable* exdesc = m_currShader->ImmToVariable(exDescValue, ISA_TYPE_UD);

    CVariable* storeMessage = m_currShader->GetNewVariable(
        2 * getGRFSize() / SIZE_DWORD,
        ISA_TYPE_UD,
        outputVar->GetAlign(),
        false,
        CName::NONE);

    m_encoder->SetSimdSize(SIMDMode::SIMD8);
    m_encoder->SetNoMask();
    m_encoder->Cast(storeMessage, m_currShader->ImmToVariable(0x76543210, ISA_TYPE_V));
    m_encoder->Shl(storeMessage, storeMessage, m_currShader->ImmToVariable(2, ISA_TYPE_UD));
    m_encoder->Push();

    for (DWORD i = 0; i < responseLen; i++)
    {
        if (i > 0)
        {
            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            m_encoder->SetNoMask();
            m_encoder->Add(storeMessage, storeMessage, m_currShader->ImmToVariable(0x20, ISA_TYPE_UD));
            m_encoder->Push();
        }

        m_encoder->SetSimdSize(SIMDMode::SIMD8);
        m_encoder->SetNoMask();
        m_encoder->SetDstSubVar(1);
        m_encoder->SetSrcSubVar(0, i);
        m_encoder->Copy(storeMessage, vaResult);

        m_encoder->Send(NULL, storeMessage,
            EU_GEN7_5_MESSAGE_TARGET_DATA_PORT_DATA_CACHE_1, exdesc, desc, false);
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

void EmitPass::EmitGenericPointersCmp(llvm::Instruction* inst,
    const SSource source[2],
    const DstModifier& modifier,
    uint8_t clearTagMask)
{
    Cmp(cast<CmpInst>(inst)->getPredicate(), source, modifier, clearTagMask);
}

void EmitPass::BinaryUnary(llvm::Instruction* inst, const SSource source[2], const DstModifier& modifier)
{
    switch (inst->getOpcode())
    {
    case Instruction::FPTrunc:
        if (inst->getType()->isVectorTy())
            FPTrunc(source, modifier);
        else
            EmitSimpleAlu(inst, source, modifier);
        break;
    case Instruction::FCmp:
    case Instruction::ICmp:
        Cmp(cast<CmpInst>(inst)->getPredicate(), source, modifier);
        break;
    case Instruction::Sub:
    case Instruction::FSub:
        Sub(source, modifier);
        break;
    case Instruction::FDiv:
    {
        bool canUseFast = inst->hasApproxFunc() ||
             (inst->hasAllowReciprocal() && !isOne(source[0].value));

        if (inst->getType()->isDoubleTy() && !canUseFast)
        {   // default : ieee fdiv
            EmitSimpleAlu(llvm_ieee_divide, source, modifier);
        }
        else
        {
            FDiv(source, modifier);
        }
        break;
    }
    case Instruction::Xor:
        Xor(source, modifier);
        break;
    case Instruction::Mul:
        Mul(source, modifier);
        break;
    case Instruction::FMul:
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

void EmitPass::Sub(const SSource sources[2], const DstModifier& modifier)
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

void EmitPass::Mul64(CVariable* dst, CVariable* src[2], SIMDMode simdMode, bool noMask) const
{

    IGC_ASSERT(dst->IsUniform() || dst->GetNumberElement() >= numLanes(simdMode));
    simdMode = dst->IsUniform() ? SIMDMode::SIMD1 : simdMode;

    auto EncoderInit = [this, simdMode, noMask]()
    {
        m_encoder->SetSimdSize(simdMode);
        if (noMask)
        {
            m_encoder->SetNoMask();
        }
    };

    auto UpdateVariableName = [this](const CName& name)->CName
    {
        return CName(name, m_encoder->IsSecondHalf() ? "SecondHalf" : "");
    };

    // Mul64 does not write to m_destination!

    IGC_ASSERT_MESSAGE((src[1]->GetType() == ISA_TYPE_Q) || (src[1]->GetType() == ISA_TYPE_UQ),
        "Cannot multiply a qword by a non-qword type");

    // The signedness of the hi-part type should be the same as that
    // of the original destination type.
    VISA_Type hiType;
    if (dst->GetType() == ISA_TYPE_Q)
        hiType = ISA_TYPE_D;
    else
        hiType = ISA_TYPE_UD;

    // Figure out what the hi and what the lo part of each source is.
    // For non-uniforms, this requires an unpack.
    CVariable* srcLo[2] = {};
    CVariable* srcHi[2] = {};
    for (int i = 0; i < 2; ++i)
    {
        if (src[i]->IsUniform())
        {
            if (src[i]->IsImmediate())
            {
                srcLo[i] = m_currShader->ImmToVariable((uint)src[i]->GetImmediateValue(), ISA_TYPE_UD);
                srcHi[i] = m_currShader->ImmToVariable(src[i]->GetImmediateValue() >> 32, hiType);
            }
            else
            {
                CVariable* srcAsUD = m_currShader->BitCast(src[i], ISA_TYPE_UD);
                srcLo[i] = m_currShader->GetNewAlias(srcAsUD, ISA_TYPE_UD, 0, 1);
                srcHi[i] = m_currShader->GetNewAlias(srcAsUD, hiType, SIZE_DWORD, 1);
            }
        }
        else
        {
            IGC_ASSERT(src[i]->GetNumberElement() >= numLanes(simdMode));

            CVariable* srcAsUD = m_currShader->BitCast(src[i], ISA_TYPE_UD);
            //TODO: Would it be better for these two to be consecutive?
            srcLo[i] = m_currShader->GetNewVariable(numLanes(simdMode), ISA_TYPE_UD, EALIGN_GRF, false,
                UpdateVariableName(CName(src[i]->getName(), i == 0 ? "Lo0" : "Lo1")));
            srcHi[i] = m_currShader->GetNewVariable(numLanes(simdMode), hiType, EALIGN_GRF, false,
                UpdateVariableName(CName(src[i]->getName(), i == 0 ? "Hi0" : "Hi1")));

            EncoderInit();
            m_encoder->SetSrcRegion(0, 2, 1, 0);
            m_encoder->Copy(srcLo[i], srcAsUD);
            m_encoder->Push();

            EncoderInit();
            m_encoder->SetSrcSubReg(0, 1);
            m_encoder->SetSrcRegion(0, 2, 1, 0);
            m_encoder->Copy(srcHi[i], srcAsUD);
            m_encoder->Push();
        }
    }

    //Now, generate the required sequence of multiplies and adds
    TODO("Do not generate intermediate multiplies by constant 0 or 1.");
    TODO("Do smarter pattern matching to look for non-constant zexted/sexted sources.");

    CVariable* dstLo = m_currShader->GetNewVariable(numLanes(simdMode), ISA_TYPE_UD, m_destination->GetAlign(),
        dst->IsUniform(), UpdateVariableName(CName(m_destination->getName(), "int64Lo")));
    CVariable* dstHi = m_currShader->GetNewVariable(numLanes(simdMode), hiType, m_destination->GetAlign(),
        dst->IsUniform(), UpdateVariableName(CName(m_destination->getName(), "int64Hi")));
    CVariable* dstHiTemp = m_currShader->GetNewVariable(numLanes(simdMode), hiType, m_destination->GetAlign(),
        dst->IsUniform(), UpdateVariableName(CName(m_destination->getName(), "int64HiTmp")));

    //
    // Algorithm:
    //   - Break the 64 bit sources into 32bit low/high halves.
    //   - Perform multiplication "by hand"
    //
    //    AB   - srcLo[0], srcLo[1]
    //    CD   - srcHi[0], srcHi[1]
    //   ----
    //     E
    //    F
    //    G
    //   H     - 'H' spills into bit 65 - only needed if overflow detection is required
    // --------
    // dstLow = E
    // dstHigh = F + G + carry

    // For those platforms natively not support DW-DW multiply, use vISA madw instruction instead of mul/mulh
    // to get better performance.
    if (m_currShader->m_Platform->noNativeDwordMulSupport())
    {
        // (Cr, E) = A * B
        // dst size should be GRF-aligned and doubled as it has both low and high results.
        // We must make the dst element number is numDWPerGRF aligned. For example, if the madw is SIMD1,
        // the dst has only 1 DW as low result in 1 GRF and only 1 DW as high result in another GRF. We should
        // set the dst as (numDWPerGRF * 2) element but not 2 DW elements. This is required by madw.
        const auto numDWPerGRF = getGRFSize() / SIZE_DWORD;
        const auto numElements = iSTD::Align(numLanes(simdMode), numDWPerGRF);
        CVariable* dstTmp = m_currShader->GetNewVariable(numElements * 2, ISA_TYPE_UD, EALIGN_GRF, dst->IsUniform(),
            UpdateVariableName(CName(m_destination->getName(), "int64Tmp")));
        CVariable* zero = m_currShader->ImmToVariable(0, ISA_TYPE_UD);

        EncoderInit();
        m_encoder->Madw(dstTmp, srcLo[0], srcLo[1], zero);
        m_encoder->Push();

        // copy low of A*B to dstLo
        EncoderInit();
        m_encoder->SetSrcRegion(0, 1, 1, 0);
        m_encoder->Copy(dstLo, dstTmp);
        m_encoder->Push();

        // copy high of A*B to dstHi
        EncoderInit();
        uint regOffset = (uint)std::ceil((float)(
            numElements * CEncoder::GetCISADataTypeSize(ISA_TYPE_UD)) / getGRFSize());
        m_encoder->SetSrcSubVar(0, regOffset);
        m_encoder->SetSrcRegion(0, 1, 1, 0);
        m_encoder->Copy(dstHi, dstTmp);
        m_encoder->Push();
    }
    else
    {
        // E = A * B
        EncoderInit();
        m_encoder->Mul(dstLo, srcLo[0], srcLo[1]);
        m_encoder->Push();

        // Cr = carry(A * B)
        EncoderInit();
        m_encoder->MulH(dstHi, srcLo[0], srcLo[1]);
        m_encoder->Push();
    }

    // F = A * D
    EncoderInit();
    m_encoder->Mul(dstHiTemp, srcLo[0], srcHi[1]);
    m_encoder->Push();

    // dstHigh = Cr + F
    EncoderInit();
    m_encoder->Add(dstHi, dstHi, dstHiTemp);
    m_encoder->Push();

    // G = C * B
    EncoderInit();
    m_encoder->Mul(dstHiTemp, srcHi[0], srcLo[1]);
    m_encoder->Push();

    // dstHigh = (Cr + F) + G
    EncoderInit();
    m_encoder->Add(dstHi, dstHi, dstHiTemp);
    m_encoder->Push();

    //And now, pack the result
    CVariable* dstAsUD = m_currShader->BitCast(dst, ISA_TYPE_UD);
    EncoderInit();
    m_encoder->SetDstRegion(2);
    m_encoder->Copy(dstAsUD, dstLo);
    m_encoder->Push();

    EncoderInit();
    m_encoder->SetDstRegion(2);
    m_encoder->SetDstSubReg(1);
    m_encoder->Copy(dstAsUD, dstHi);
    m_encoder->Push();
}

static unsigned int getVectorSize(Instruction *I) {
    IGCLLVM::FixedVectorType *VecType =
        llvm::dyn_cast<IGCLLVM::FixedVectorType>(I->getType());
    if (!VecType)
        return 0;
    unsigned int NumElements = VecType->getNumElements();
    return NumElements;
}

void EmitPass::FPTrunc(const SSource sources[2], const DstModifier& modifier) {

    CVariable* src[2];
    src[0] = GetSrcVariable(sources[0]);
    if (IGC_IS_FLAG_ENABLED(EnableVectorEmitter) && sources[0].value->getType()->isVectorTy()) {

        unsigned int VectorSize = 0;
        if (llvm::isa<Instruction>(sources[0].value))
            VectorSize = getVectorSize(llvm::cast<Instruction>(sources[0].value));

        // float is 4 bytes --> divide by 4
        unsigned int NumberOfFloatsThatFitInRegister = m_currShader->getGRFSize()/4;
        for (unsigned int i = 0; i < VectorSize; ++i) {

            // this is FPTrunc we move from 32bits to 16 this gives us factor of 2
            unsigned int row = i / 2;
            // now we can fit twice as many half floats into the same register:
            // .decl vector331 v_type=G type=f num_elts=128 align=wordx32
            // .decl vectorized_cast v_type=G type=hf num_elts=128 align=wordx32
            // mov (M1, 16) vectorized_cast(0,0 <-- this is odd col)<1> vector331(0,0)<1;1,0>
            // mov (M1, 16) vectorized_cast(0,16 <-- this is even col)<1> vector331(1,0)<1;1,0>
            unsigned int col = (i%2) * NumberOfFloatsThatFitInRegister;
            SetSourceModifiers(0, sources[0]);
            if (src[0]->IsUniform()) { m_encoder->SetSrcSubReg(0, i); }
            else m_encoder->SetSrcSubVar(0, i);
            m_encoder->SetDstModifier(modifier);
            m_encoder->SetDstSubVar(row);
            m_encoder->SetDstSubReg(col);
            m_encoder->Cast(m_destination, src[0]);
            m_encoder->Push();
        }
    }
}

void EmitPass::Mul(const SSource sources[2], const DstModifier& modifier)
{
    CVariable* src[2];
    for (int i = 0; i < 2; ++i)
    {
        src[i] = GetSrcVariable(sources[i]);
    }

    if (IGC_IS_FLAG_ENABLED(EnableVectorEmitter) && sources[0].value->getType()->isVectorTy() && sources[1].value->getType()->isVectorTy()) {

        unsigned int VectorSize = 0;
        if (llvm::isa<Instruction>(sources[0].value))
            VectorSize = getVectorSize(llvm::cast<Instruction>(sources[0].value));

        for (unsigned int i = 0; i < VectorSize; ++i) {
            SetSourceModifiers(0, sources[0]);
            SetSourceModifiers(1, sources[1]);

            if (src[0]->IsUniform()) { m_encoder->SetSrcSubReg(0, i); }
            else m_encoder->SetSrcSubVar(0, i);
            if (src[1]->IsUniform()) { m_encoder->SetSrcSubReg(1, i); }
            else m_encoder->SetSrcSubVar(1, i);

            m_encoder->SetDstSubVar(i);
            m_encoder->Mul(m_destination, src[0], src[1]);
            m_encoder->Push();
        }
        return;
    }

    // Only i64 muls need special handling, otherwise go back to standard flow
    VISA_Type srcType = src[0]->GetType();
    if (srcType != ISA_TYPE_Q && srcType != ISA_TYPE_UQ)
    {
        Binary(EOPCODE_MUL, sources, modifier);
    }
    else
    {
        Mul64(m_destination, src, m_currShader->m_SIMDSize);
    }
}

void EmitPass::FDiv(const SSource sources[2], const DstModifier& modifier)
{
    if (isOne(sources[0].value))
    {
        Unary(EOPCODE_INV, &sources[1], modifier);
    }
    else
    {
        Binary(EOPCODE_DIV, sources, modifier);
    }
}

static inline bool isConstantAllOnes(const Value* V)
{
    if (const Constant * C = dyn_cast<Constant>(V))
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

void EmitPass::Cmp(llvm::CmpInst::Predicate pred, const SSource sources[2], const DstModifier& modifier, uint8_t clearTagMask)
{
    IGC_ASSERT(modifier.sat == false);
    IGC_ASSERT(modifier.flag == nullptr);
    IGC_ASSERT(nullptr != m_destination);

    e_predicate predicate = GetPredicate(pred);

    CVariable* src0 = GetSrcVariable(sources[0], sources[0].fromConstantPool);
    CVariable* src1 = GetSrcVariable(sources[1], sources[1].fromConstantPool);

    if (IsUnsignedCmp(pred))
    {
        src0 = m_currShader->BitCast(src0, GetUnsignedType(src0->GetType()));
        src1 = m_currShader->BitCast(src1, GetUnsignedType(src1->GetType()));
    }
    else if (IsSignedCmp(pred))
    {
        src0 = m_currShader->BitCast(src0, GetSignedType(src0->GetType()));
        src1 = m_currShader->BitCast(src1, GetSignedType(src1->GetType()));
    }

    CVariable* dst = m_destination;
    if (m_destination->GetType() != ISA_TYPE_BOOL && dst->GetType() != src0->GetType())
    {
        IGC_ASSERT_MESSAGE(CEncoder::GetCISADataTypeSize(dst->GetType()) == CEncoder::GetCISADataTypeSize(src0->GetType()),
            "Cmp to GRF must have the same size for source and destination");
        dst = m_currShader->BitCast(m_destination, src0->GetType());
    }

    if (clearTagMask)
    {
        auto clearTag = [&](CVariable*& ptr)
        {
            // Don't need to clear a tag on immediate pointers
            if (ptr->IsImmediate()) return;

            CVariable* pTempVar = m_currShader->GetNewVariable(
                numLanes(m_currShader->m_SIMDSize),
                ptr->GetType(), m_currShader->getGRFAlignment(),
                ptr->IsUniform(), CName::NONE);
            // Clear tag in the high part and restore address canonical form
            m_encoder->Shl(pTempVar, ptr, m_currShader->ImmToVariable(4, ISA_TYPE_D));
            m_encoder->IShr(pTempVar, pTempVar, m_currShader->ImmToVariable(4, ISA_TYPE_D));
            m_encoder->Push();
            ptr = pTempVar;
        };

        if(clearTagMask & (1 << 0))
            clearTag(src0);
        if(clearTagMask & (1 << 1))
            clearTag(src1);
    }

    SetSourceModifiers(0, sources[0]);
    SetSourceModifiers(1, sources[1]);
    m_encoder->Cmp(predicate, dst, src0, src1);
    m_encoder->Push();
}

void EmitPass::Frc(const SSource& source, const DstModifier& modifier)
{
    Unary(EOPCODE_FRC, &source, modifier);
}

void EmitPass::Floor(const SSource& source, const DstModifier& modifier)
{
    Unary(EOPCODE_RNDD, &source, modifier);
}

void EmitPass::Mov(const SSource& source, const DstModifier& modifier)
{
    Unary(EOPCODE_MOV, &source, modifier);
}

void EmitPass::Rsqrt(const SSource& source, const DstModifier& modifier)
{
    Unary(EOPCODE_RSQRT, &source, modifier);
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

void EmitPass::Tenary(e_opcode opCode, const SSource sources[3], const DstModifier& modifier)
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
    CVariable* srcs[3] = { nullptr, nullptr, nullptr };
    for (uint i = 0; i < N; i++)
    {
        bool fromConstantPool = sources[i].fromConstantPool;
        srcs[i] = GetSrcVariable(sources[i], fromConstantPool);
        SetSourceModifiers(i, sources[i]);
    }
    m_encoder->SetDstModifier(modifier);
    m_encoder->GenericAlu(opCode, m_destination, srcs[0], srcs[1], srcs[2]);
    m_encoder->Push();
}

void EmitPass::Bfn(uint8_t booleanFuncCtrl, const SSource sources[3], const DstModifier& modifier)
{
    CVariable* srcs[3] = { nullptr, nullptr, nullptr };
    // Currently the BFN must have 3 sources, otherwise we will not generate it. Though BFN can
    // have only 2 sources
    for (uint i = 0; i < 3; i++)
    {
        bool fromConstantPool = sources[i].fromConstantPool;
        srcs[i] = GetSrcVariable(sources[i], fromConstantPool);
        if (sources[i].region_set)
        {
            m_encoder->SetSrcRegion(i, sources[i].region[0], sources[i].region[1], sources[i].region[2], sources[i].instance);
            m_encoder->SetSrcSubReg(i, sources[i].elementOffset);
        }
    }
    m_encoder->SetDstModifier(modifier);
    m_encoder->Bfn(booleanFuncCtrl, m_destination, srcs[0], srcs[1], srcs[2]);
    m_encoder->Push();
}

void EmitPass::CmpBfn(llvm::CmpInst::Predicate predicate, const SSource cmpSources[2], uint8_t booleanFuncCtrl,
    const SSource bfnSources[3], const DstModifier& modifier)
{
    // Cmp
    e_predicate pred = GetPredicate(predicate);
    CVariable* cmpSrc0 = GetSrcVariable(cmpSources[0]);
    CVariable* cmpSrc1 = GetSrcVariable(cmpSources[1]);
    CVariable* cmpDst = m_currShader->GetNewVariable(m_destination);

    if (IsUnsignedCmp(predicate))
    {
        cmpSrc0 = m_currShader->BitCast(cmpSrc0, GetUnsignedType(cmpSrc0->GetType()));
        cmpSrc1 = m_currShader->BitCast(cmpSrc1, GetUnsignedType(cmpSrc1->GetType()));
    }
    else if (IsSignedCmp(predicate))
    {
        cmpSrc0 = m_currShader->BitCast(cmpSrc0, GetSignedType(cmpSrc0->GetType()));
        cmpSrc1 = m_currShader->BitCast(cmpSrc1, GetSignedType(cmpSrc1->GetType()));
    }

    if (cmpDst->GetType() != cmpSrc0->GetType())
    {
        cmpDst = m_currShader->BitCast(cmpDst, cmpSrc0->GetType());
    }

    SetSourceModifiers(0, cmpSources[0]);
    SetSourceModifiers(1, cmpSources[1]);
    m_encoder->Cmp(pred, cmpDst, cmpSrc0, cmpSrc1);
    m_encoder->Push();

    // BFN
    CVariable* bfnSrc1 = GetSrcVariable(bfnSources[1], bfnSources[1].fromConstantPool);
    CVariable* bfnSrc2 = GetSrcVariable(bfnSources[2], bfnSources[2].fromConstantPool);
    if (cmpDst->GetType() != bfnSrc1->GetType())
    {
        cmpDst = m_currShader->BitCast(cmpDst, bfnSrc1->GetType());
    }

    SetSourceModifiers(1, bfnSources[1]);
    SetSourceModifiers(2, bfnSources[2]);
    m_encoder->SetDstModifier(modifier);
    m_encoder->Bfn(booleanFuncCtrl, m_destination, cmpDst, bfnSrc1, bfnSrc2);
    m_encoder->Push();
}

void EmitPass::Select(const SSource sources[3], const DstModifier& modifier)
{
    IGC_ASSERT(modifier.flag == nullptr);
    IGC_ASSERT(sources[0].mod == EMOD_NONE);

    CVariable* flag = GetSrcVariable(sources[0]);

    bool fromConstantPool = sources[1].fromConstantPool;
    CVariable* src0 = GetSrcVariable(sources[1], fromConstantPool);

    fromConstantPool = sources[2].fromConstantPool;
    CVariable* src1 = GetSrcVariable(sources[2], fromConstantPool);

    SetSourceModifiers(0, sources[1]);
    SetSourceModifiers(1, sources[2]);
    m_encoder->SetDstModifier(modifier);
    m_encoder->SetPredicateMode(modifier.predMode);

    m_encoder->Select(flag, m_destination, src0, src1);
    m_encoder->Push();

}

void EmitPass::PredAdd(const SSource& pred, bool invert, const SSource sources[2], const DstModifier& modifier)
{
    IGC_ASSERT(modifier.flag == nullptr);
    CVariable* flag = GetSrcVariable(pred);
    CVariable* src0 = GetSrcVariable(sources[0]);
    CVariable* src1 = GetSrcVariable(sources[1]);

    // base condition
    SetSourceModifiers(0, sources[0]);
    m_encoder->Copy(m_destination, src0);
    m_encoder->Push();

    // predicate add
    SetSourceModifiers(1, sources[1]);
    m_encoder->SetDstModifier(modifier);
    m_encoder->SetPredicateMode(modifier.predMode);
    m_encoder->SetInversePredicate(invert);
    m_encoder->PredAdd(flag, m_destination, m_destination, src1);
    m_encoder->Push();
}

void EmitPass::emitOutput(llvm::GenIntrinsicInst* inst)
{
    {
        IGC_ASSERT_MESSAGE(0, "output not supported");
    }
}

CVariable* EmitPass::GetVMaskPred(CVariable*& predicate)
{
    return predicate;
}

void EmitPass::createVMaskPred(CVariable*& predicate)
{
    if (!m_currShader->IsPatchablePS())
        return;

    if (predicate != nullptr)
        return;

}

void EmitPass::UseVMaskPred()
{
    if (!m_currShader->IsPatchablePS())
        return;

    if (!m_vMaskPredForSubplane)
        return;

}

CVariable* EmitPass::GetCombinedVMaskPred(CVariable* basePredicate)
{
    if (!m_currShader->IsPatchablePS())
        return basePredicate;

    if (!m_vMaskPredForSubplane)
        return basePredicate;

    bool subspan = m_encoder->IsSubSpanDestination();
    if ((IGC_IS_FLAG_ENABLED(UseVMaskPredicate) || IGC_IS_FLAG_ENABLED(UseVMaskPredicateForLoads) || IGC_IS_FLAG_ENABLED(UseVMaskPredicateForIndirectMove)) &&
        subspan && !m_destination->IsUniform())
    {
        CVariable* vmaskPredicate = m_vMaskPredForSubplane;
        if (IGC_IS_FLAG_ENABLED(UseVMaskPredicateForLoads) || IGC_IS_FLAG_ENABLED(UseVMaskPredicateForIndirectMove))
        {
            vmaskPredicate = m_currShader->GetNewVariable(
                numLanes(m_SimdMode),
                ISA_TYPE_BOOL,
                EALIGN_DWORD,
                "");
            m_encoder->SetSrcSubReg(0, 3);
            m_encoder->SetP(vmaskPredicate, m_currShader->GetSR0());
            m_encoder->Push();
        }
        if (basePredicate)
        {
            IGC_ASSERT(basePredicate->GetNumberElement() == vmaskPredicate->GetNumberElement());
            CVariable* combinedPredicate = m_currShader->GetNewVariable(
                numLanes(m_SimdMode),
                ISA_TYPE_BOOL,
                EALIGN_DWORD,
                "");
            m_encoder->And(combinedPredicate, vmaskPredicate, basePredicate);
            m_encoder->Push();
            return combinedPredicate;
        }
        return vmaskPredicate;
    }
    return basePredicate;
}


void EmitPass::emitPlnInterpolation(CVariable* baryVar, CVariable* inputvar)
{
    unsigned int numPln = 1;

    for (unsigned int i = 0; i < numPln; i++)
    {
        UseVMaskPred();

        // plane will access 4 operands
        m_encoder->SetSrcRegion(0, 0, 4, 1);
        m_encoder->Pln(m_destination, inputvar, baryVar);
        m_encoder->Push();
    }
}


void EmitPass::emitcycleCounter(llvm::Instruction* inst)
{
    // Must move 2 DW at the same time, so use "(W) mov (2|M0)"
    CVariable* dst = m_destination;
    CVariable* dst2DW = m_currShader->GetNewAlias(dst, ISA_TYPE_UD, 0, 0);
    m_encoder->SetNoMask();
    m_encoder->SetSrcRegion(0, 1, 1, 0);
    m_encoder->SetUniformSIMDSize(SIMDMode::SIMD2);
    m_encoder->Copy(dst2DW, m_currShader->GetTSC());
    m_encoder->Push();
}

void EmitPass::emitSetDebugReg(llvm::Instruction* inst)
{
    Value* src0 = inst->getOperand(0);
    if (!isa<UndefValue>(src0))
    {
        // write dbg0.0
        CVariable* src = GetSymbol(src0);
        IGC_ASSERT(nullptr != src);
        IGC_ASSERT(src->IsUniform());
        m_encoder->SetDstSubReg(0);
        m_encoder->Copy(m_currShader->GetDBG(), src);
        m_encoder->Push();
    }

    // read dbg0.1
    m_encoder->SetSrcSubReg(0, 1);
    m_encoder->SetSrcRegion(0, 0, 1, 0);
    m_encoder->Copy(m_destination, m_currShader->GetDBG());
    m_encoder->Push();
}

CVariable* EmitPass::ComputeSampleIntOffset(llvm::Instruction* sample, uint sourceIndex)
{
    // The (u,v,r) offsets are encoded in SamplerMessageHeader::DW2
    // as [11:8], [7:4], [3:0] bitfields, respectively. Format: S3.
    uint offset = 0;
    bool dynamicOffset = false;
    for (uint i = 0; i < 3; i++)
    {
        if (ConstantInt * immOffset = dyn_cast<ConstantInt>(sample->getOperand(sourceIndex + i)))
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
    if (dynamicOffset)
    {
        CVariable* tempPackedOffset = m_currShader->GetNewVariable(1, ISA_TYPE_UW, EALIGN_WORD, true, "PackedOffset");
        for (uint i = 0; i < 3; i++)
        {
            if (!isa<ConstantInt>(sample->getOperand(sourceIndex + i)))
            {
                CVariable* offsetV = GetSymbol(sample->getOperand(sourceIndex + i));
                if (!offsetV->IsUniform())
                {
                    offsetV = UniformCopy(offsetV);
                }

                // Offset is only 4 bits, mask off remaining bits
                CVariable* offsetBits = m_currShader->GetNewVariable(1, ISA_TYPE_UW, EALIGN_WORD, true, "PackedOffset");
                m_encoder->And(offsetBits, offsetV, m_currShader->ImmToVariable(0xF, ISA_TYPE_UW));
                if (i != 2)
                {
                    m_encoder->Shl(offsetBits, offsetBits, m_currShader->ImmToVariable(4 * (2 - i), ISA_TYPE_UW));
                }
                if (packedOffset->IsImmediate() && packedOffset->GetImmediateValue() == 0)
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
    if (oldLoad)
    {
        if (i == 1)
        {
            index = 2;
        }
        else if (i == 2)
        {
            index = 1;
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
    SIMDMode simdMode = m_currShader->m_Platform->getMinDispatchMode();
    auto numLanePerChannel = numLanes(simdMode);
    for (uint16_t n = 0; n < m_destination->GetNumberElement() / numLanePerChannel; n++)
    {
        m_encoder->SetDstSubReg(n * numLanePerChannel);
        m_encoder->SetSrcSubReg(0, n * numLanePerChannel * 2);
        m_encoder->Copy(m_destination, dst);
        m_encoder->Push();
    }
}

static bool isUsedOnlyByEval(llvm::User* user)
{
    bool hasUses = user->hasNUsesOrMore(1);
    if (hasUses)
    {
        if (std::all_of(user->user_begin(), user->user_end(),
            [](llvm::User* user)
        {
            llvm::GenIntrinsicInst* evalInst = llvm::dyn_cast<llvm::GenIntrinsicInst>(user);
            return evalInst && evalInst->getIntrinsicID() == GenISAIntrinsic::GenISA_evaluateSampler;
        }))
        {
            return true;
        }

        return std::all_of(user->user_begin(), user->user_end(),
            [](llvm::User* user)
        {
            return (llvm::isa<BitCastInst>(user) || llvm::isa<ExtractElementInst>(user) || llvm::isa<InsertElementInst>(user)) && isUsedOnlyByEval(user);
        });
    }
    return false;
}

void EmitPass::emitLdInstruction(llvm::Instruction* inst)
{
    uint numOperands = inst->getNumOperands();
    IGC_ASSERT_MESSAGE(7 < numOperands, "Wrong number of operands");
    IGC_ASSERT_MESSAGE(numOperands < 11, "Wrong number of operands");

    EOPCODE opCode = GetOpCode(inst);
    //Subtract the offsets, resource sources to get
    //the number of texture coordinates and index to texture source
    uint numSources = numOperands - 6;
    uint textureArgIdx = numOperands - 5;

    uint offsetSourceIndex = numSources + 2;

    SmallVector<CVariable*, 4> payload;

    for (uint i = numSources - 1; i > 0; i--)
    {
        uint index = CorrectLdIndex(i, m_currShader->m_Platform->hasOldLdOrder());
        CVariable* src = GetSymbol(inst->getOperand(index));
        if (!(src->IsImmediate() && src->GetImmediateValue() == 0))
        {
            break;
        }
        numSources--;
    }

    bool zeroLOD = false;

    CVariable* lodSrc =
        opCode == llvm_ldlptr ? GetSymbol(inst->getOperand(3)) : GetSymbol(inst->getOperand(2));

    if (m_currShader->m_Platform->supportSampleAndLd_lz() && lodSrc->IsImmediate() && lodSrc->GetImmediateValue() == 0)
    {
        zeroLOD = true;
    }

    // create send payload for numSources
    for (uint i = 0; i < numSources; i++)
    {
        uint index = i;
        // no difference in ld_lz between SKL+ and BDW
        if (!zeroLOD)
        {
            index = CorrectLdIndex(i, m_currShader->m_Platform->hasOldLdOrder());
        }

        if (opCode == llvm_ld_ptr && index == 2 && zeroLOD)
        {
            // 3D resources skip lod and read z coordinate
            index = 3;
            --numSources;
        }

        CVariable* src = GetSymbol(inst->getOperand(index));
        if (src->IsUniform())
        {
            auto uniformSIMDMode = m_currShader->m_Platform->getMinDispatchMode();
            uint16_t size = m_destination->IsUniform() ? numLanes(uniformSIMDMode) :
                numLanes(m_currShader->m_SIMDSize);
            CVariable* newSource = m_currShader->GetNewVariable(
                size,
                src->GetType(),
                EALIGN_GRF,
                m_destination->IsUniform(),
                src->getName());
            m_encoder->SetUniformSIMDSize(uniformSIMDMode);
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
    {
        if (dst->IsUniform())
        {
            simdSize = m_currShader->m_Platform->getMinDispatchMode();
            unsigned short numberOfElement = dst->GetNumberElement() * numLanes(simdSize);
            numberOfElement = CEncoder::GetCISADataTypeSize(dst->GetType()) == 2 ? numberOfElement * 2 : numberOfElement;
            dst = m_currShader->GetNewVariable(
                numberOfElement, dst->GetType(), EALIGN_GRF, dst->IsUniform(), dst->getName());
        }
        else
        {
            needPacking = isHalfGRFReturn(m_destination, m_SimdMode);
            if (needPacking)
            {
                dst = m_currShader->GetNewVariable(
                    m_destination->GetNumberElement() * 2, m_destination->GetType(), EALIGN_GRF, dst->IsUniform(), dst->getName());
            }
        }
    }

    const CShader::ExtractMaskWrapper writeMask(m_currShader, inst);
    IGC_ASSERT_MESSAGE(writeMask.hasEM() && writeMask.getEM() != 0, "Wrong write mask");

    bool feedbackEnable = writeMask.isSet(4);
    uint label = 0;
    CVariable* flag = nullptr;
    Value* ptr = inst->getOperand(textureArgIdx);
    ResourceDescriptor resource = GetResourceVariable(ptr);
    uint ResourceLoopMarker = m_RLA->GetResourceLoopMarker(inst);
    bool needLoop = ResourceLoopHeader(resource, flag, label, ResourceLoopMarker);
    ResourceLoopSubIteration(resource, flag, label, ResourceLoopMarker);

    m_encoder->SetPredicate(flag);
    if (m_destination->IsUniform())
    {
        m_encoder->SetUniformSIMDSize(m_currShader->m_Platform->getMinDispatchMode());
    }

    SamplerLoadIntrinsic* samplerLoadIntrinsic = llvm::cast<llvm::SamplerLoadIntrinsic>(inst);
    ResourceDescriptor pairedResource = samplerLoadIntrinsic->hasPairedTextureArg() &&
        llvm::isa<llvm::UndefValue>(samplerLoadIntrinsic->getPairedTextureValue()) == false ?
        GetResourceVariable(samplerLoadIntrinsic->getPairedTextureValue()) :
        ResourceDescriptor();

    CVariable* offset = ComputeSampleIntOffset(inst, offsetSourceIndex);

    m_encoder->Load(
        opCode,
        writeMask.getEM(),
        offset,
        resource,
        pairedResource,
        numSources,
        dst,
        payload,
        zeroLOD,
        feedbackEnable);
    m_encoder->Push();
    if (m_currShader->hasReadWriteImage(*(inst->getParent()->getParent())))
    {
        CVariable* tempdest = m_currShader->BitCast(m_destination, GetUnsignedIntegerType(m_destination->GetType()));
        m_encoder->Cast(m_currShader->GetNULL(), tempdest);
        m_encoder->Push();
        m_encoder->Copy(m_currShader->GetNULL(), m_currShader->GetTSC());
        m_encoder->Push();
    }
    ResourceLoopBackEdge(needLoop, flag, label,
                         m_RLA->GetResourceLoopMarker(inst));

    {
        if (m_destination->IsUniform())
        {
            // if dst is uniform, we simply copy the first lane of each channel (including feedback enable if present)
            // to the packed m_destination.
            // Note that there's no need to handle feedback enable specially
            for (unsigned int i = 0; i < m_destination->GetNumberElement(); i++)
            {
                m_encoder->SetSrcRegion(0, 0, 1, 0);
                m_encoder->SetSrcSubVar(0, i);
                m_encoder->SetDstSubReg(i);
                m_encoder->Copy(m_destination, dst);
                m_encoder->Push();
            }
        }
        else
        {
            if (needPacking)
            {
                PackSIMD8HFRet(dst);
            }

            if (feedbackEnable
                )
            {
                emitFeedbackEnable();
            }
        }
    }
}

/// \brief Returns the offset increment in bytes, given the value's type.
static int GetOffsetIncrement(const DataLayout* m_DL, SIMDMode simdMode, Value* val)
{
    int inc;
    inc = int_cast<int>(numLanes(simdMode) * (unsigned int)m_DL->getTypeAllocSize(val->getType()));
    if (val->getType()->isHalfTy() && simdMode == SIMDMode::SIMD8)
    {
        //Since alloc size for half float is = 2 and if we have simd8 mode we'll get offset = 16
        //but need to pad it with extra 16.
        IGC_ASSERT(inc <= 16);
        inc *= 2;
    }
    return inc;
}



void EmitPass::emitSimdLaneId(llvm::Instruction* inst)
{
    m_currShader->GetSimdOffsetBase(m_destination);
}

void EmitPass::emitSimdLaneIdReplicate(llvm::Instruction* inst)
{
    m_currShader->GetSimdOffsetBase(m_destination, true);
}


void EmitPass::emitSimdSize(llvm::Instruction* inst)
{
    //CVariable* simdSize = m_currShader->ImmToVariable(numLanes(m_SimdMode), ISA_TYPE_UD);
    //m_encoder->Cast(m_destination, simdSize);
    //m_encoder->Push();
}

void EmitPass::emitCrossInstanceMov(const SSource& source, const DstModifier& modifier)
{
    IGC_ASSERT(m_currShader->m_numberInstance == 2);
    IGC_ASSERT(m_currShader->m_State.m_dispatchSize == SIMDMode::SIMD32);
    IGC_ASSERT_MESSAGE(!m_encoder->IsSecondHalf(), "This emitter must be called only once for simd32!");

    CVariable* data = GetSymbol(source.value);

    CVariable* contiguousData = nullptr;
    CVariable* upperHalfOfContiguousData = nullptr;

    const uint16_t numElements = data->GetNumberElement();
    IGC_ASSERT(numElements == 16);
    const VISA_Type dataType = data->GetType();

    // Create a 32 element variable and copy both instances of data into it.
    contiguousData = m_currShader->GetNewVariable(
        numElements * 2,
        dataType,
        data->GetAlign(),
        "CrossInstMovTmp");

    upperHalfOfContiguousData = m_currShader->GetNewAlias(
        contiguousData,
        dataType,
        numElements * m_encoder->GetCISADataTypeSize(dataType),
        numElements);

    IGC_ASSERT(contiguousData);
    IGC_ASSERT(upperHalfOfContiguousData);

    m_encoder->SetSecondHalf(false);
    m_encoder->Copy(contiguousData, data);
    m_encoder->Push();

    m_encoder->SetSecondHalf(true);
    m_encoder->Copy(upperHalfOfContiguousData, data);
    m_encoder->Push();

    m_encoder->SetSecondHalf(false);

    // Copy from cross instance data
    bool isSecondHalf = false;
    for (uint32_t i = 0; i < m_currShader->m_numberInstance; i++)
    {
        if (isSecondHalf)
        {
            m_encoder->SetSecondHalf(true);
        }

        // Emit mov
        m_encoder->SetSrcSubReg(0, source.elementOffset + numElements * i);
        m_encoder->SetSrcRegion(0, source.region[0], source.region[1], source.region[2]);
        m_encoder->SetDstModifier(modifier);
        m_encoder->Copy(m_destination, contiguousData);
        m_encoder->Push();

        if (isSecondHalf)
        {
            m_encoder->SetSecondHalf(false);
        }

        isSecondHalf = !isSecondHalf;
    }
}

/// Emits VISA instructions for SIMD_SHUFFLE.
void EmitPass::emitSimdShuffle(llvm::Instruction* inst)
{
    bool disableHelperLanes = int_cast<int>(cast<ConstantInt>(inst->getOperand(2))->getSExtValue()) == 2;
    if (disableHelperLanes)
    {
        ForceDMask();
    }
    CVariable* data = GetSymbol(inst->getOperand(0));
    CVariable* simdChannel = GetSymbol(inst->getOperand(1));

    const bool isSimd32 = (m_currShader->m_State.m_dispatchSize == SIMDMode::SIMD32);

    if (data->IsUniform())
    {
        m_encoder->Copy(m_destination, data);
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
        dataIndex = dataIndex % numLanes(m_currShader->m_State.m_dispatchSize);
        if (isSimd32)
        {
            const bool isSrcInSecondHalf = dataIndex >= 16;
            dataIndex = dataIndex % numLanes(m_encoder->GetSimdSize());

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
                    true, // isUniform
                    "ShuffleTmp");

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
        //
        // 1) GenISA_WaveShuffleIndex:
        //
        //    a) SIMD16
        //      shl (M1, 16) ShuffleTmp(0,0)<1> {{.+}}(0,0)<16;8,2> 0x2:uw
        //      addr_add (M1, 16) A0(0)<1> &{{V[0-9]+}} ShuffleTmp(0,0)<1;1,0>
        //      mov (M1, 16) simdShuffle(0,0)<1> r[A0(0),0]<1,0>:d
        //
        //    b) SIMD32 (two SIMD16 ADDR_ADD instructions must be generated,
        //               because address register has only 16 elements):
        //      shl(M1, 32) V465(0,0)<1> V464(0,0)<16;8,2> 0x2:uw
        //      mov(M1, 32) V466(0,0)<1> V70(0,0)<1;1,0>
        //      addr_add(M1, 16) A0(0)<1> &V466 + 0 V465(0, 0)<1;1,0>
        //      mov(M1, 16) V463(0,0)<1> r[A0(0),0]<1, 0>:f
        //      addr_add(M5, 16) A0(0)<1> &V466 + 0 V465(0,16)<1;1,0>
        //      mov(M5, 16) V463(1,0)<1> r[A0(0),0]<1,0>:f
        //
        // 2) GenISA_WaveBroadcast:
        //
        //   shl (M1_NM, 1) ShuffleTmp(0,0)<1> {{.+}}(0,0)<0;1,0> 0x2:uw
        //   addr_add(M1_NM, 1) A0(0) <1> &{{V[0 - 9]+}} ShuffleTmp(0, 0) < 0;1,0 >
        //   a) SIMD16:
        //      mov(M1, 16) simdBroadcast(0,0) <1> r[A0(0),0] <0;1,0>:d
        //   b) SIMD32 (no need for two SIMD16 instructions, because offset in A0 is uniform):
        //      mov(M1, 32) simdBroadcast(0,0) <1> r[A0(0),0] <0;1,0>:d

        bool channelUniform = simdChannel->IsUniform();

        auto* GII = dyn_cast<GenIntrinsicInst>(inst);
        if (GII && GII->getIntrinsicID() == GenISAIntrinsic::GenISA_WaveBroadcast &&
            !channelUniform)
        {
            // OpGroupBroadcast guarantees that all channels must be enabled and the
            // simdChannel value must be the same for all of them. Therefore, even though
            // it was not possible to deduce, during compilation time, that simdChannel is
            // uniform, let's force it to be uniform by taking it always from the first channel.
            CVariable* valueFromFirstChannel = m_currShader->GetNewVariable(
                numLanes(SIMDMode::SIMD1),
                simdChannel->GetType(),
                simdChannel->GetAlign(), true, CName::NONE);

            m_encoder->SetSimdSize(SIMDMode::SIMD1);
            m_encoder->SetNoMask();
            m_encoder->SetSrcRegion(0, 0, 1, 0);

            m_encoder->Copy(valueFromFirstChannel, simdChannel);
            m_encoder->Push();

            simdChannel = valueFromFirstChannel;
            channelUniform = true;
        }

        IGC_ASSERT_MESSAGE(m_encoder->GetCISADataTypeSize(simdChannel->GetType()) == 4,
            "simdChannel size of simdShuffle should be 4 bytes!");

        // Choose the shift factor.
        int shtAmt = 0;
        switch (m_encoder->GetCISADataTypeSize(m_destination->GetType()))
        {
        case 1:  shtAmt = 0; break;
        case 2:  shtAmt = 1; break;
        case 4:  shtAmt = 2; break;
        case 8:  shtAmt = 3; break;
        default: IGC_ASSERT_MESSAGE(0, "Unexpected data type size.");
        }

        CVariable* simdChannelUW = m_currShader->BitCast(simdChannel, ISA_TYPE_UW);
        CVariable* pSrcElm = m_currShader->GetNewVariable(
            simdChannel->GetNumberElement(),
            ISA_TYPE_UW,
            EALIGN_GRF,
            channelUniform,
            simdChannel->GetNumberInstance(),
            "ShuffleTmp");
        if (!channelUniform)
        {
            m_encoder->SetSrcRegion(0, 16, 8, 2);
        }
        m_encoder->Shl(pSrcElm, simdChannelUW,
            m_currShader->ImmToVariable(shtAmt, ISA_TYPE_UW));
        m_encoder->Push();

        CVariable* src = data;
        if (m_currShader->m_numberInstance == 1 && m_currShader->m_SIMDSize == SIMDMode::SIMD32)
        {
            uint16_t addrSize = channelUniform ? 1 : numLanes(SIMDMode::SIMD16);

            // VectorUniform for shuffle is true as all simd lanes will
            // take the same data as the lane 0 !
            CVariable* pDstArrElm = m_currShader->GetNewAddressVariable(
                addrSize,
                m_destination->GetType(),
                channelUniform,
                true,
                m_destination->getName());

            if (GII && GII->getIntrinsicID() == GenISAIntrinsic::GenISA_WaveBroadcast)
            {
                m_encoder->AddrAdd(pDstArrElm, src, pSrcElm);
                m_encoder->Push();
                m_encoder->Copy(m_destination, pDstArrElm);
                m_encoder->Push();
            }
            else if(GII && GII->getIntrinsicID() == GenISAIntrinsic::GenISA_WaveShuffleIndex)
            {
                if (channelUniform)
                {
                    m_encoder->AddrAdd(pDstArrElm, src, pSrcElm);
                    m_encoder->Push();

                    m_encoder->Copy(m_destination, pDstArrElm);
                    m_encoder->Push();
                }
                else // !channelUniform
                {
                    m_encoder->SetSimdSize(SIMDMode::SIMD16);

                    m_encoder->AddrAdd(pDstArrElm, src, pSrcElm);
                    m_encoder->Push();

                    m_encoder->SetSimdSize(SIMDMode::SIMD16);

                    // If `src` variable is the same as `m_destination` variable, we cannot write the result
                    // for the first 16 channels to `m_destionation` right away, because it may overwrite `src`
                    // values used by the last 16 channels. Instead, the result for the first 16 channels
                    // gets written into a temporary variable, then shuffle index for the last 16 channels
                    // is generated and, at the end, the result for first 16 channels is rewritten from
                    // the temporary variable into the `m_destination`.
                    const bool isSrcSameAsDst = src == m_destination;
                    CVariable* first16LanesResult = nullptr;
                    if (isSrcSameAsDst)
                    {
                        first16LanesResult = m_currShader->GetNewVariable(
                            16,
                            m_destination->GetType(),
                            m_destination->GetAlign(),
                            false, // isUniform
                            "first16LanesResult");

                        m_encoder->Copy(first16LanesResult, pDstArrElm);
                    }
                    else
                    {
                        m_encoder->Copy(m_destination, pDstArrElm);
                    }
                    m_encoder->Push();

                    m_encoder->SetSimdSize(SIMDMode::SIMD16);
                    m_encoder->SetMask(EMASK_H2);
                    m_encoder->SetSrcSubReg(0, 16);
                    m_encoder->SetSrcSubReg(1, 16);
                    m_encoder->AddrAdd(pDstArrElm, src, pSrcElm);
                    m_encoder->Push();

                    m_encoder->SetSimdSize(SIMDMode::SIMD16);

                    m_encoder->SetMask(EMASK_H2);
                    m_encoder->SetDstSubReg(16);
                    m_encoder->Copy(m_destination, pDstArrElm);
                    m_encoder->Push();
                    m_encoder->SetSecondHalf(false);

                    m_encoder->Push();

                    if (isSrcSameAsDst)
                    {
                        IGC_ASSERT(first16LanesResult);
                        m_encoder->SetSimdSize(SIMDMode::SIMD16);
                        m_encoder->Copy(m_destination, first16LanesResult);
                        m_encoder->Push();
                    }
                }
            }
            if (disableHelperLanes)
            {
                ResetVMask();
            }
            return;
        }

        if (isSimd32)
        {
            CVariable* contiguousData = nullptr;
            CVariable* upperHalfOfContiguousData = nullptr;

            const uint16_t numElements = data->GetNumberElement();
            const VISA_Type dataType = data->GetType();

            IGC_ASSERT(numElements == 16);
            IGC_ASSERT_MESSAGE(!m_encoder->IsSecondHalf(), "This emitter must be called only once for simd32!");

            // Create a 32 element variable and copy both instances of data into it.
            contiguousData = m_currShader->GetNewVariable(
                numElements * 2,
                dataType,
                data->GetAlign(),
                false, // isUniform
                1,
                "ShuffleTmp"); // numberInstance

            upperHalfOfContiguousData = m_currShader->GetNewAlias(
                contiguousData,
                dataType,
                numElements * m_encoder->GetCISADataTypeSize(dataType),
                numElements);

            IGC_ASSERT(contiguousData);
            IGC_ASSERT(upperHalfOfContiguousData);

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
            true,
            m_destination->getName());

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
    if (disableHelperLanes)
    {
        ResetVMask();
    }
}

void EmitPass::emitSimdClusteredBroadcast(llvm::Instruction* inst)
{
    CVariable* data = GetSymbol(inst->getOperand(0));

    // If input is uniform, just copy to all lanes.
    if (data->IsUniform())
    {
        m_encoder->Copy(m_destination, data);
        if (!m_destination->IsUniform() && m_currShader->m_numberInstance > 1)
        {
            m_encoder->SetSecondHalf(true);
            m_encoder->Copy(m_destination, data);
            m_encoder->SetSecondHalf(false);
        }
        m_encoder->Push();
        return;
    }

    IGC_ASSERT_MESSAGE(!m_destination->IsUniform(), "Unsupported: dst must be non-uniform");

    IGC_ASSERT_MESSAGE(isa<llvm::ConstantInt>(inst->getOperand(1)), "Unsupported: cluster size must be constant");
    const unsigned int clusterSize = int_cast<uint32_t>(cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue());

    IGC_ASSERT_MESSAGE(isa<llvm::ConstantInt>(inst->getOperand(2)), "Unsupported: cluster lane must be constant");
    const unsigned int clusterLane = int_cast<uint32_t>(cast<llvm::ConstantInt>(inst->getOperand(2))->getZExtValue());

    IGC_ASSERT_MESSAGE(clusterSize <= numLanes(m_currShader->m_State.m_dispatchSize), "cluster size must be smaller or equal to SIMD");
    IGC_ASSERT_MESSAGE(clusterSize % 8 == 0, "cluster size must be 8 or 16");
    IGC_ASSERT_MESSAGE(clusterLane < clusterSize, "cluster lane does not fit in cluster size");

    if (clusterSize == numLanes(m_currShader->m_State.m_dispatchSize))
    {
        // There is actually no cluster, just do subgroup broadcast instead.
        if (m_currShader->m_numberInstance > 1)
        {
            // Use an intermediate uniform variable
            CVariable* uniformTemp = m_currShader->GetNewVariable(
                1,
                data->GetType(),
                m_encoder->GetCISADataTypeAlignment(data->GetType()),
                true, // isUniform
                "ClusteredBroadcastTmp");

            // Copy from source to the uniform temp...
            m_encoder->SetSecondHalf(clusterLane >= 16);
            m_encoder->SetNoMask();
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->SetSrcSubReg(0, clusterLane % numLanes(m_encoder->GetSimdSize()));
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
        else
        {
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->SetSrcSubReg(0, clusterLane);
            m_encoder->Copy(m_destination, data);
            m_encoder->Push();
        }
    }
    else
    {
        // Clustered broadcast.
        m_encoder->SetSrcRegion(0, clusterSize, clusterSize, 0);
        m_encoder->SetSrcSubReg(0, clusterLane);
        m_encoder->Copy(m_destination, data);
        if (m_currShader->m_numberInstance > 1)
        {
            m_encoder->SetSecondHalf(true);
            m_encoder->Copy(m_destination, data);
            m_encoder->SetSecondHalf(false);
        }
        m_encoder->Push();
    }

}

void EmitPass::emitSimdShuffleDown(llvm::Instruction* inst)
{
    CVariable* pCurrentData = GetSymbol(inst->getOperand(0));
    CVariable* pNextData = GetSymbol(inst->getOperand(1));
    CVariable* pDelta = m_currShader->GetSymbol(inst->getOperand(2));

    // temp size is the sum of src0 and src1
    uint16_t nbElements = numLanes(m_SimdMode) * 2;

    // Join current and Next Data
    SBasicBlock::SimdShuffleDownSrcTy srcPair(inst->getOperand(0), inst->getOperand(1));
    SBasicBlock* pCurrBlk = getCurrentBlock();
    CVariable* pCombinedData = pCurrBlk ? pCurrBlk->m_simdShuffleDownSrc[srcPair] : nullptr;
    if (pCombinedData == nullptr)
    {
        pCombinedData = m_currShader->GetNewVariable(
            nbElements,
            m_destination->GetType(),
            m_destination->GetAlign(),
            "ShuffleTmp");

        auto CopyData = [this](CVariable* pDestinationData, CVariable* pSourceData, uint32_t offset)
        {
            for (uint32_t i = 0; i < m_currShader->m_numberInstance; i++)
            {
                IGC_ASSERT_MESSAGE(!m_encoder->IsSecondHalf(), "This emitter must be called only once for simd32!");
                uint32_t currentOffset = offset + numLanes(m_encoder->GetSimdSize()) * i;
                bool isSecondHalf = i == 1;

                if (isSecondHalf)
                {
                    m_encoder->SetSecondHalf(true);
                }

                m_encoder->SetSimdSize(m_encoder->GetSimdSize());
                m_encoder->SetDstSubReg(currentOffset);
                m_encoder->SetNoMask();
                m_encoder->Copy(pDestinationData, pSourceData);
                m_encoder->Push();

                if (isSecondHalf)
                {
                    m_encoder->SetSecondHalf(false);
                }
            }
        };

        CopyData(pCombinedData, pCurrentData, 0);
        CopyData(pCombinedData, pNextData, numLanes(m_encoder->GetSimdSize()) * m_currShader->m_numberInstance);

        // save it for possible re-use later.
        if (pCurrBlk)
        {
            pCurrBlk->m_simdShuffleDownSrc[srcPair] = pCombinedData;
        }
    }

    // Emit mov with direct addressing when delta is a compile-time constant.
    const bool useDirectAddressing = pDelta->IsImmediate()
        && m_currShader->m_Platform->GetPlatformFamily() != IGFX_GEN8_CORE;

    SIMDMode simdMode = m_currShader->m_Platform->getMinDispatchMode();
    auto nativeExecSize = numLanes(simdMode);
    auto width = numLanes(m_SimdMode);
    if (useDirectAddressing && nativeExecSize * 2 >= width)
    {
        const uint dataIndex = pDelta->GetImmediateValue() % nbElements;
        int tripCount = width <= nativeExecSize ? 1 : 2;
        for (int i = 0; i < tripCount; ++i)
        {
            m_encoder->SetSimdSize(simdMode);
            m_encoder->SetSrcRegion(0, 1, 1, 0);
            m_encoder->SetSrcSubReg(0, dataIndex + nativeExecSize * i);
            m_encoder->SetDstSubReg(nativeExecSize * i);
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

    CVariable* pLaneId = m_currShader->GetNewVariable(
        numLanes(m_SimdMode),
        ISA_TYPE_UD,
        EALIGN_GRF,
        "LaneId");

    m_encoder->SetSimdSize(SIMDMode::SIMD8);
    m_encoder->SetNoMask();
    CVariable* imm0 = m_currShader->ImmToVariable(0x76543210, ISA_TYPE_V);
    m_encoder->Cast(pLaneId, imm0);
    m_encoder->Push();

    if (m_SimdMode == SIMDMode::SIMD16 || m_SimdMode == SIMDMode::SIMD32)
    {
        m_encoder->SetDstSubVar(0);
        m_encoder->SetDstSubReg(8);
        m_encoder->SetSimdSize(SIMDMode::SIMD8);
        m_encoder->SetNoMask();
        CVariable* imm1 = m_currShader->ImmToVariable(0x8, ISA_TYPE_UD);
        m_encoder->Add(pLaneId, pLaneId, imm1);
        m_encoder->Push();
    }

    if (m_SimdMode == SIMDMode::SIMD32)
    {
        m_encoder->SetSimdSize(SIMDMode::SIMD16);
        m_encoder->SetDstSubReg(16);
        m_encoder->SetNoMask();
        CVariable* imm1 = m_currShader->ImmToVariable(0x10, ISA_TYPE_UD);
        m_encoder->Add(pLaneId, pLaneId, imm1);
        m_encoder->Push();
    }

    CVariable* pShuffleIdx = m_currShader->GetNewVariable(
        numLanes(m_SimdMode),
        ISA_TYPE_UD,
        EALIGN_GRF,
        "ShuffleIdx");

    for (uint32_t i = 0; i < m_currShader->m_numberInstance; i++)
    {
        IGC_ASSERT_MESSAGE(!m_encoder->IsSecondHalf(), "This emitter must be called only once for simd32!");
        uint32_t offset = numLanes(m_encoder->GetSimdSize()) * i;
        bool isSecondHalf = i == 1;

        if (isSecondHalf)
        {
            m_encoder->SetSecondHalf(true);
        }

        CVariable* pCurrentLaneId = m_currShader->GetNewAlias(
            pLaneId,
            pLaneId->GetType(),
            offset * m_encoder->GetCISADataTypeSize(pLaneId->GetType()),
            numLanes(m_encoder->GetSimdSize()));

        m_encoder->SetSimdSize(m_encoder->GetSimdSize());
        m_encoder->SetDstSubReg(offset);
        m_encoder->SetNoMask();
        m_encoder->Add(pShuffleIdx, pCurrentLaneId, pDelta);
        m_encoder->Push();

        if (isSecondHalf)
        {
            m_encoder->SetSecondHalf(false);
        }
    }

    CVariable* pByteOffset = m_currShader->GetNewVariable(
        numLanes(m_SimdMode),
        ISA_TYPE_UD,
        EALIGN_GRF,
        "ByteOffset");

    uint32_t shift = m_destination->GetElemSize() / 2;

    for (uint32_t i = 0; i < m_currShader->m_numberInstance; i++)
    {
        uint32_t offset = numLanes(m_encoder->GetSimdSize()) * i;

        CVariable* pCurrentShuffleIdx = m_currShader->GetNewAlias(
            pShuffleIdx,
            pShuffleIdx->GetType(),
            offset * m_encoder->GetCISADataTypeSize(pShuffleIdx->GetType()),
            numLanes(m_encoder->GetSimdSize()));

        m_encoder->SetSimdSize(m_encoder->GetSimdSize());
        m_encoder->SetDstSubReg(offset);
        m_encoder->SetNoMask();
        m_encoder->Shl(pByteOffset, pCurrentShuffleIdx, m_currShader->ImmToVariable(shift, ISA_TYPE_UD));
        m_encoder->Push();
    }

    if (m_currShader->m_numberInstance == 1 && m_currShader->m_SIMDSize == SIMDMode::SIMD32)
    {
        // special handling for single SIMD32 since a0 is only SIMD16 wide

        CVariable* pDstArrElm = m_currShader->GetNewAddressVariable(
            16,
            m_destination->GetType(),
            false,
            false,
            m_destination->getName());

        m_encoder->SetSimdSize(SIMDMode::SIMD16);
        m_encoder->SetSrcRegion(1, 16, 8, 2);
        m_encoder->AddrAdd(pDstArrElm, pCombinedData, m_currShader->BitCast(pByteOffset, ISA_TYPE_UW));
        m_encoder->Push();
        m_encoder->SetSimdSize(SIMDMode::SIMD16);
        m_encoder->Copy(m_destination, pDstArrElm);
        m_encoder->Push();

        m_encoder->SetSimdSize(SIMDMode::SIMD16);
        m_encoder->SetMask(EMASK_H2);
        m_encoder->SetSrcSubReg(1, 32);
        m_encoder->SetSrcRegion(1, 16, 8, 2);
        m_encoder->AddrAdd(pDstArrElm, pCombinedData, m_currShader->BitCast(pByteOffset, ISA_TYPE_UW));
        m_encoder->Push();
        m_encoder->SetSimdSize(SIMDMode::SIMD16);
        m_encoder->SetMask(EMASK_H2);
        m_encoder->SetDstSubReg(16);
        m_encoder->Copy(m_destination, pDstArrElm);
        m_encoder->Push();
        m_encoder->SetSecondHalf(false);

        return;
    }

    uint16_t addrSize = m_SimdMode == SIMDMode::SIMD32 ? numLanes(SIMDMode::SIMD16) : numLanes(m_SimdMode);

    CVariable* pDstArrElm = m_currShader->GetNewAddressVariable(
        addrSize,
        m_destination->GetType(),
        false,
        false,
        m_destination->getName());

    for (uint32_t i = 0; i < m_currShader->m_numberInstance; i++)
    {
        IGC_ASSERT_MESSAGE(!m_encoder->IsSecondHalf(), "This emitter must be called only once for simd32!");
        uint32_t offset = numLanes(m_encoder->GetSimdSize()) * i;
        bool isSecondHalf = i == 1;

        CVariable* pCurrentByteOffset = m_currShader->GetNewAlias(
            pByteOffset,
            pByteOffset->GetType(),
            offset * m_encoder->GetCISADataTypeSize(pByteOffset->GetType()),
            numLanes(m_encoder->GetSimdSize()));

        m_encoder->SetNoMask();
        m_encoder->SetSrcRegion(1, 16, 8, 2);
        m_encoder->AddrAdd(pDstArrElm, pCombinedData, m_currShader->BitCast(pCurrentByteOffset, ISA_TYPE_UW));
        m_encoder->Push();

        if (isSecondHalf)
        {
            m_encoder->SetSecondHalf(true);
        }

        m_encoder->Copy(m_destination, pDstArrElm);
        m_encoder->Push();

        if (isSecondHalf)
        {
            m_encoder->SetSecondHalf(false);
        }
    }
}

void EmitPass::emitSimdShuffleXor(llvm::Instruction* inst)
{
    CVariable* pData = m_currShader->GetSymbol(inst->getOperand(0));
    CVariable* pXorValue = m_currShader->GetSymbol(inst->getOperand(1));

    IGC_ASSERT_MESSAGE(pXorValue->IsImmediate(), "simdShuffleXor must have \
        constant xorValue parameter");

    // emit move sequence for 1 bit
    // case 0:  1 2 3 4 5 6 7 8     => 2 1 4 3 6 5 8 7
    // case 1:  1 2 3 4 5 6 7 8     => 3 4 1 2 7 8 5 6
    // case 1:  1 2 3 4 5 6 7 8     => 3 4 1 2 7 8 5 6
    // case 2:  1 2 3 4 5 6 7 8     => 5 6 7 8 1 2 3 4
    // case 3:  1 2 .. 8 9 .. 15 16 => 9 10 .. 15 16 1 2 .. 7 8
    auto emitShuffleXor1Bit = [&](CVariable* pData, uint xorBit) -> CVariable*
    {
        VISA_Type type = pData->GetType();
        bool is64bitType = type == ISA_TYPE_Q || type == ISA_TYPE_UQ || type == ISA_TYPE_DF;

        CVariable* pResult = m_currShader->GetNewVariable(
            pData->GetNumberElement(),
            pData->GetType(),
            pData->GetAlign(),
            false,
            1,
            "simdShuffleXorTmp");

        if (xorBit == 0 || (xorBit == 1 && !is64bitType)) {
            // Use strided access of max possible length
            // For simd16 and xorBit == 0
            //     mov (M1_NM, 8) simdShuffleXorTmp(0,0)<2> V0040(0,1)<2;1,0>                   /// $11
            //     mov (M1_NM, 8) simdShuffleXorTmp(0,1)<2> V0040(0,0)<2;1,0>                   /// $12
            // For 32-bit it will be just 2 movs, for 64-bit double type let the finalizer to split the vars:
            // r10 is the source
            // (W)     mov (16|M0)              r19.0<1>:ud   r10.2<4;2,1>:ud                  {$4.dst}             // $13
            // (W)     mov (8|M0)               r18.0<1>:df   r19.0<1;1,0>:df                  {I@1}                // $13
            // (W)     mov (8|M0)               r12.0<4>:ud   r18.0<2;1,0>:ud                  {Compacted,L@1}      // $13
            // (W)     mov (8|M0)               r12.1<4>:ud   r18.1<2;1,0>:ud                                       // $13
            // (W)     mov (16|M0)              r21.0<1>:ud   r10.0<4;2,1>:ud                                       // $14
            // (W)     mov (8|M0)               r20.0<1>:df   r21.0<1;1,0>:df                  {I@1}                // $14
            // (W)     mov (8|M0)               r12.2<4>:ud   r20.0<2;1,0>:ud                  {Compacted,L@1}      // $14
            // (W)     mov (8|M0)               r12.3<4>:ud   r20.1<2;1,0>:ud                                       // $14

            // For int32 and xorBit == 1
            //     mov (M1_NM, 4) simdShuffleXorTmp(0,0)<4> V0040(0,2)<4;1,0>                   /// $11
            //     mov (M1_NM, 4) simdShuffleXorTmp(0,2)<4> V0040(0,0)<4;1,0>                   /// $12
            //     mov (M1_NM, 4) simdShuffleXorTmp(0,1)<4> V0040(0,3)<4;1,0>                   /// $13
            //     mov (M1_NM, 4) simdShuffleXorTmp(0,3)<4> V0040(0,1)<4;1,0>                   /// $14
            // for xorBit == 1 strided moves are beneficial only if the type is less that 64-bit
            // (fewer moves will be generated)

            // for xorBit > 1 is it always more beneficial to copy with subsequent chunks

            auto stride = (2 * (xorBit + 1));
            auto width = pData->GetNumberElement() / stride;
            auto currentSimdMode = lanesToSIMDMode(width);

            for (uint i = 0; i < xorBit + 1; i++) {
                m_encoder->SetSimdSize(currentSimdMode);
                m_encoder->SetSrcRegion(0, stride, 1, 0);
                m_encoder->SetSrcSubReg(0, i + xorBit + 1);
                m_encoder->SetDstRegion(stride);
                m_encoder->SetDstSubReg(i);
                m_encoder->SetNoMask();
                m_encoder->Copy(pResult, pData);
                m_encoder->Push();

                m_encoder->SetSimdSize(currentSimdMode);
                m_encoder->SetSrcRegion(0, stride, 1, 0);
                m_encoder->SetSrcSubReg(0, i);
                m_encoder->SetDstRegion(stride);
                m_encoder->SetDstSubReg(i + xorBit + 1);
                m_encoder->SetNoMask();
                m_encoder->Copy(pResult, pData);
                m_encoder->Push();
            }
        }
        else if ((xorBit >= 1) && (xorBit <= 3)) {
            // Use subsequent accesses to copy all subsequent chunks
            // for xorBit == 2
            //     mov (M1_NM, 4) simdShuffleXorTmp(0,0)<1> V0043(0,4)<1;1,0>                   /// $13
            //     mov (M1_NM, 4) simdShuffleXorTmp(0,4)<1> V0043(0,0)<1;1,0>                   /// $14
            //     mov (M1_NM, 4) simdShuffleXorTmp(1,0)<1> V0043(1,4)<1;1,0>                   /// $15
            //     mov (M1_NM, 4) simdShuffleXorTmp(1,4)<1> V0043(1,0)<1;1,0>                   /// $16
            // for 64-bit types the accesses will be 2x widened in finalizer
            // (W)     mov (8|M0)               r12.0<1>:ud   r10.8<1;1,0>:ud                  {Compacted,$4.dst}   // $13
            // (W)     mov (8|M0)               r12.8<1>:ud   r10.0<1;1,0>:ud                  {Compacted}          // $14
            // (W)     mov (8|M0)               r13.0<1>:ud   r11.8<1;1,0>:ud                  {Compacted}          // $15
            // (W)     mov (8|M0)               r13.8<1>:ud   r11.0<1;1,0>:ud                  {Compacted}          // $16
            // The number of chunks is larger on the larger SIMD

            auto width = static_cast<int>(std::pow(2, xorBit));
            auto currentSimdMode = lanesToSIMDMode(width);

            for (uint i = 0; i < pData->GetNumberElement(); i += width * 2) {
                m_encoder->SetSimdSize(currentSimdMode);
                m_encoder->SetSrcRegion(0, 1, 1, 0);
                m_encoder->SetSrcSubReg(0, i + width);
                m_encoder->SetDstRegion(1);
                m_encoder->SetDstSubReg(i);
                m_encoder->SetNoMask();
                m_encoder->Copy(pResult, pData);
                m_encoder->Push();

                m_encoder->SetSimdSize(currentSimdMode);
                m_encoder->SetSrcRegion(0, 1, 1, 0);
                m_encoder->SetSrcSubReg(0, i);
                m_encoder->SetDstRegion(1);
                m_encoder->SetDstSubReg(i + width);
                m_encoder->SetNoMask();
                m_encoder->Copy(pResult, pData);
                m_encoder->Push();
            }
        }
        else {
            IGC_ASSERT_MESSAGE(false, "simdShuffleXor is only implemented for 0 <= xor_value <= 15");
        };

        return pResult;
    };

    // just broadcast the value if the value is uniform
    if (pData->IsUniform()) {
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        m_encoder->SetSrcSubReg(0, 0);
        m_encoder->SetDstRegion(1);
        m_encoder->SetDstSubReg(0);
        m_encoder->Copy(m_destination, pData);
        m_encoder->Push();
        return;
    }

    // emit moves for every non-zero bit subsequently
    const auto xorValue = pXorValue->GetImmediateValue();
    CVariable* tempValue = pData;
    for (uint i = 0; i < 5; i++)
    {
        if (((xorValue >> i) & 0x1) == 0x1)
        {
            tempValue = emitShuffleXor1Bit(tempValue, i);
        }
    }

    // final copy, respecting the execution mask if in divergent CF
    if (!m_currShader->InsideDivergentCF(inst))
    {
        m_encoder->SetNoMask();
    }
    m_encoder->SetSrcRegion(0, 1, 1, 0);
    m_encoder->SetSrcSubReg(0, 0);
    m_encoder->SetDstRegion(1);
    m_encoder->SetDstSubReg(0);
    m_encoder->Copy(m_destination, tempValue);
    m_encoder->Push();
}

static uint32_t getBlockMsgSize(uint32_t bytesRemaining, uint32_t maxSize)
{
    uint32_t size = 0;
    if (bytesRemaining >= 256)
    {
        size = 256;
    }
    else if (bytesRemaining >= 128)
    {
        size = 128;
    }
    else if (bytesRemaining >= 64)
    {
        size = 64;
    }
    else if (bytesRemaining >= 32)
    {
        size = 32;
    }
    else
    {
        size = 16;
    }
    return std::min(size, maxSize);
}

static uint32_t getLSCBlockMsgSize(uint32_t bytesRemaining, uint32_t maxSize)
{
    uint32_t size = 0;
    if (bytesRemaining >= 512)
    {
        size = 512;
    }
    else if (bytesRemaining >= 256)
    {
        size = 256;
    }
    else if (bytesRemaining >= 128)
    {
        size = 128;
    }
    else if (bytesRemaining >= 64)
    {
        size = 64;
    }
    else if (bytesRemaining >= 32)
    {
        size = 32;
    }
    else  if (bytesRemaining >= 16)
    {
        size = 16;
    }
    else
    {
        size = 8;
    }
    return std::min(size, maxSize);
}

void EmitPass::emitSimdBlockWrite(llvm::Instruction* inst, llvm::Value* ptrVal)
{
    if (shouldGenerateLSC())
    {
        emitLSCSimdBlockWrite(inst, ptrVal);
        return;
    }
    emitLegacySimdBlockWrite(inst, ptrVal);

}

void EmitPass::emitSimdBlockRead(llvm::Instruction* inst, llvm::Value* ptrVal)
{
    if (shouldGenerateLSC())
    {
        emitLSCSimdBlockRead(inst, ptrVal);
        return;
    }
    emitLegacySimdBlockRead(inst, ptrVal);
}

void EmitPass::emitLegacySimdBlockWrite(llvm::Instruction* inst, llvm::Value* ptrVal)
{
    Value* llPtr = inst->getOperand(0);
    Value* dataPtr = inst->getOperand(1);

    PointerType* ptrType = cast<PointerType>(llPtr->getType());
    ResourceDescriptor resource = GetResourceVariable(llPtr);

    CVariable* src = nullptr;
    if (ptrVal)
    {
        src = GetSymbol(ptrVal);
        src = m_currShader->BitCast(src, GetUnsignedType(src->GetType()));
    }
    else
    {
        src = GetSymbol(llPtr);
    }

    CVariable* data = GetSymbol(dataPtr);
    bool useA64 = isA64Ptr(ptrType, m_currShader->GetContext());

    if (!data->IsUniform() && !IsGRFAligned(data, EALIGN_GRF))
    {
        CVariable* temp =
            m_currShader->GetNewVariable(numLanes(m_SimdMode), data->GetType(), EALIGN_GRF, CName::NONE);
        m_encoder->Copy(temp, data);
        data = temp;
    }

    Type* Ty = dataPtr->getType();
    IGCLLVM::FixedVectorType* VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
    uint32_t nbElements = 0;
    nbElements = VTy ? int_cast<uint32_t>(VTy->getNumElements()) : 1;

    uint32_t typeSizeInBytes = Ty->getScalarSizeInBits() / 8;
    uint32_t totalBytes = nbElements * typeSizeInBytes * numLanes(m_SimdMode);

    bool isSeparated = m_SimdMode == SIMDMode::SIMD32 &&
        m_encoder->GetSimdSize() == SIMDMode::SIMD16;

    // Data has other layout than expecting one by block write instructions in case of multiple instances.
    // The expected layout:
    //  |0th component of data from thread 0-15 |0th component of data from thread 16-31|
    //  |1st component of data from thread 0-15 |1st component of data from thread 16-31|
    // The current layout:
    //  |0th component of data from thread 0-15 |1st component of data from thread 0-15 |
    //  |0th component of data from thread 16-31|1st component of data from thread 16-31|
    if (isSeparated)
    {
        IGC_ASSERT_MESSAGE(!m_encoder->IsSecondHalf(), "This emitter must be called only once for simd32!");
        const uint32_t numVectorElementsPerSimd = numLanes(m_encoder->GetSimdSize());
        CVariable* copiedData = m_currShader->GetNewVariable(
            data->GetNumberElement() * data->GetNumberInstance(),
            data->GetType(),
            data->GetAlign(),
            "");

        for (uint32_t i = 0; i < 2; i++)
        {
            if (i == 1)
            {
                m_encoder->SetSecondHalf(true);
            }

            for (uint32_t elementIndex = 0; elementIndex < nbElements; elementIndex++)
            {
                // Offsets can be deduced from the upper comment.
                CVariable* destinationAlias = m_currShader->GetNewAlias(
                    copiedData,
                    copiedData->GetType(),
                    numVectorElementsPerSimd * (nbElements * elementIndex + i) * m_encoder->GetCISADataTypeSize(copiedData->GetType()),
                    numVectorElementsPerSimd);
                CVariable* sourceAlias = data;
                if (!data->IsUniform())
                {
                    sourceAlias = m_currShader->GetNewAlias(
                        data,
                        data->GetType(),
                        numVectorElementsPerSimd * elementIndex * m_encoder->GetCISADataTypeSize(data->GetType()),
                        numVectorElementsPerSimd);
                }

                m_encoder->SetSimdSize(m_encoder->GetSimdSize());
                m_encoder->SetNoMask();
                m_encoder->Copy(destinationAlias, sourceAlias);
                m_encoder->Push();
            }
        }

        m_encoder->SetSecondHalf(false);
        data = copiedData;
    }
    else
    {
        // Special case for uniform data. data is expected to be non-uniform.
        data = BroadcastIfUniform(data);
    }

    if (m_SimdMode == SIMDMode::SIMD32 && getGRFSize() == 64)
    {
        totalBytes = nbElements * typeSizeInBytes * numLanes(SIMDMode::SIMD16);
    }

    // Special case for simd8 char block write, in which the total bytes = 8.
    // (All the other cases, the total bytes is multiple of 16 (OW).
    if (totalBytes == 8)
    {
        // Use Byte scattered write. If address is aligned at least QW,
        // we should use QW-aligned QW write!
        //    ByteScatterred write:  use (blksizeInBits, nblk) = (8, 4) and two lanes
        //    QW write :             use (blksizeInBits, nblk) = (64, 1) [todo]
        bool useQW = false;
        uint32_t blkBits = useQW ? 64 : 8;
        uint32_t nBlks = useQW ? 1 : 4;

        uint16_t activelanes = useQW ? 1 : 2;
        // lanesToSIMDMode(activelanes);
        SIMDMode simdmode = useQW ? SIMDMode::SIMD1 : SIMDMode::SIMD2;

        CVariable* eOffset = src;
        eOffset = ReAlignUniformVariable(src, m_currShader->getGRFAlignment());
        CVariable* ScatterOff = eOffset;
        if (activelanes > 1)
        {
            IGC_ASSERT_MESSAGE(!useQW, "Only one lane is active when using QW!");

            ScatterOff = m_currShader->GetNewVariable(
                activelanes, eOffset->GetType(), eOffset->GetAlign(), true, "ScatterOff");

            CVariable* immVar = m_currShader->ImmToVariable(0x40, ISA_TYPE_UV);
            if ((!IGC_COUNT_FLAG_IN_LIMIT(DelayEmuInt64AddLimit) ||
                 simdmode != SIMDMode::SIMD1) &&
                useA64 &&
                !m_currShader->m_Platform->hasInt64Add()) {
                emitAddPair(ScatterOff, eOffset, immVar);
            }
            else {
                m_encoder->SetNoMask();
                m_encoder->SetUniformSIMDSize(simdmode);
                m_encoder->SetSrcRegion(0, 0, 1, 0);
                m_encoder->Add(ScatterOff, eOffset, immVar);
                m_encoder->Push();
            }
        }

        m_encoder->SetNoMask();
        m_encoder->SetUniformSIMDSize(simdmode);
        if (useA64)
        {
            emitScatterA64(data, ScatterOff, blkBits, nBlks, true);
        }
        else
        {
            m_encoder->ByteScatter(data, resource, ScatterOff, blkBits, nBlks);
        }
        m_encoder->Push();

        return;
    }

    if (useA64)
    {
        uint32_t bytesRemaining = totalBytes;
        uint32_t srcOffset = 0;
        uint32_t bytesToRead = 0;

        // Emits instructions generating one or more A64 OWORD block write instructions
        // The amount of data we need to write is n * Component Size OWORDs.
        // We can write 8, 4, or 2 OWORDs at a time. We can also write 1 OWORD,
        // but since this is a SIMD opcode and we're  compiling SIMD8, SIMD16,
        // we don't expect to see a 1 OWORD write.

        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
        m_encoder->SetSrcRegion(0, 0, 1, 0);

        CVariable* pTempVar = m_currShader->GetNewVariable(
            numLanes(SIMDMode::SIMD1),
            ISA_TYPE_UQ,
            EALIGN_QWORD, true, CName::NONE);

        m_encoder->Copy(pTempVar, m_currShader->BitCast(src, ISA_TYPE_UQ));
        m_encoder->Push();

        while (bytesRemaining)
        {
            bytesToRead = getBlockMsgSize(bytesRemaining, m_currShader->m_Platform->getMaxBlockMsgSize(false));
            bytesRemaining -= bytesToRead;
            m_encoder->OWStoreA64(data, pTempVar, bytesToRead, srcOffset);

            srcOffset = srcOffset + bytesToRead;
            m_encoder->Push();

            if (bytesRemaining)
            {
                if (!IGC_COUNT_FLAG_IN_LIMIT(DelayEmuInt64AddLimit) && !m_currShader->m_Platform->hasInt64Add()) {
                    CVariable* ImmVar = m_currShader->ImmToVariable(bytesToRead, ISA_TYPE_UD);
                    emitAddPair(pTempVar, pTempVar, ImmVar);
                }
                else {
                    m_encoder->SetSimdSize(SIMDMode::SIMD1);
                    m_encoder->SetNoMask();
                    m_encoder->SetSrcRegion(0, 0, 1, 0);
                    m_encoder->Add(pTempVar, pTempVar, m_currShader->ImmToVariable((bytesToRead), ISA_TYPE_UQ));
                    m_encoder->Push();
                }
            }
        }
    }
    else
    {
        uint32_t bytesRemaining = totalBytes;

        // Emits instructions generating one or more OWORD block write instructions
        // The amount of data we need to write is n * Component Size OWORDs.
        // We can write 8, 4, or 2 OWORDs at a time. We can also write 1 OWORD,
        // but since this is a SIMD opcode and we're  compiling SIMD8, SIMD16,
        // we don't expect to see a 1 OWORD write.

        // shr   (1) r64.2<1>:ud r60.0<0; 1, 0>:ud 0x4:uw{ Align1, H1, NoMask }
        // mov  (16) r65.0<1>:ud r54.0<8; 8, 1>:ud{ Align1, NoMask, Compacted }
        // and   (1) r64.5<1>:ud r0.5<0; 1, 0>:ud 0x3ff:ud{ Align1, NoMask }
        // send (16) null<1>:uw r64 0xa 0x60a03ff:ud{ Align1, NoMask } oword block write

        CVariable* src0shifted = m_currShader->GetNewVariable(
            numLanes(SIMDMode::SIMD1),
            ISA_TYPE_UD,
            EALIGN_DWORD,
            "Src0Shifted");

        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        m_encoder->Shr(src0shifted, src, m_currShader->ImmToVariable(4, ISA_TYPE_UD));
        m_encoder->Push();

        uint32_t srcOffset = 0;
        uint32_t bytesToRead = 0;
        while (bytesRemaining)
        {
            bool isToSLM = ptrType->getPointerAddressSpace() == ADDRESS_SPACE_LOCAL;
            bytesToRead = getBlockMsgSize(bytesRemaining, m_currShader->m_Platform->getMaxBlockMsgSize(isToSLM));
            bytesRemaining -= bytesToRead;

            m_encoder->OWStore(data, resource.m_surfaceType, resource.m_resource, src0shifted, bytesToRead, srcOffset);

            srcOffset = srcOffset + bytesToRead;
            m_encoder->Push();

            if (bytesRemaining)
            {
                m_encoder->SetSimdSize(SIMDMode::SIMD1);
                m_encoder->SetNoMask();
                m_encoder->SetSrcRegion(0, 0, 1, 0);
                m_encoder->Add(src0shifted, src0shifted, m_currShader->ImmToVariable((bytesToRead / 16), ISA_TYPE_UD)); // (bytesToRead / 16) is units of OWORDS
                m_encoder->Push();
            }
        }
    }
}

void EmitPass::emitLegacySimdBlockRead(llvm::Instruction* inst, llvm::Value* ptrVal)
{
    Value* llPtr = inst->getOperand(0);
    PointerType* ptrType = cast<PointerType>(llPtr->getType());
    ResourceDescriptor resource = GetResourceVariable(llPtr);

    CVariable* src = nullptr;
    if (ptrVal)
    {
        src = GetSymbol(ptrVal);
        src = m_currShader->BitCast(src, GetUnsignedType(src->GetType()));
    }
    else
    {
        src = GetSymbol(llPtr);
    }

    // If it is SLM, use OW-aligned OW address. The byte address (default)
    // must be right-shifted by 4 bits to be OW address!
    bool isToSLM = (ptrType->getPointerAddressSpace() == ADDRESS_SPACE_LOCAL);
    bool useA64 = isA64Ptr(ptrType, m_currShader->GetContext());

    Type* Ty = inst->getType();
    IGCLLVM::FixedVectorType* VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
    uint32_t nbElements = VTy ? int_cast<uint32_t>(VTy->getNumElements()) : 1;

    uint32_t typeSizeInBytes = Ty->getScalarSizeInBits() / 8;
    uint32_t totalBytes = nbElements * typeSizeInBytes * numLanes(m_SimdMode);

    if (m_SimdMode == SIMDMode::SIMD32 && getGRFSize() == 64)
    {
        totalBytes = nbElements * typeSizeInBytes * numLanes(SIMDMode::SIMD16);
    }

    bool needsTempDst = m_SimdMode == SIMDMode::SIMD32 &&
        m_encoder->GetSimdSize() == SIMDMode::SIMD16;
    CVariable* dest = needsTempDst ?
        m_currShader->GetNewVariable(
            m_destination->GetNumberElement() * m_destination->GetNumberInstance(),
            m_destination->GetType(),
            m_destination->GetAlign(),
            "") :
        m_destination;

    // Special case for simd8 char block read, in which the total bytes = 8.
    // (All the other cases, the total bytes is multiple of 16 (OW).
    if (totalBytes == 8)
    {
        // Use Byte scattered read. If address is aligned at least QW,
        // we should use QW-aligned QW read!
        //    Byte Scattered read :  use (blksizeInBits, nblk) = (8, 4) and two lanes
        //    QW read :              use (blksizeInBits, nblk) = (64, 1) [todo]
        bool useQW = false;
        uint32_t blkBits = useQW ? 64 : 8;
        uint32_t nBlks = useQW ? 1 : 4;
        CVariable* gatherDst = dest;

        uint16_t activelanes = useQW ? 1 : 2;
        // lanesToSIMDMode(activelanes);
        SIMDMode simdmode = useQW ? SIMDMode::SIMD1 : SIMDMode::SIMD2;

        CVariable* eOffset = src;
        eOffset = ReAlignUniformVariable(src, m_currShader->getGRFAlignment());
        CVariable* gatherOff = eOffset;
        if (activelanes > 1)
        {
            IGC_ASSERT_MESSAGE(!useQW, "Only one lane is active when using QW!");

            gatherOff = m_currShader->GetNewVariable(
                activelanes, eOffset->GetType(), eOffset->GetAlign(), true, "GatherOff");

            CVariable* immVar = m_currShader->ImmToVariable(0x40, ISA_TYPE_UV);
            if ((!IGC_COUNT_FLAG_IN_LIMIT(DelayEmuInt64AddLimit) ||
                 simdmode != SIMDMode::SIMD1) &&
                useA64 &&
                !m_currShader->m_Platform->hasInt64Add()) {
                emitAddPair(gatherOff, eOffset, immVar);
            }
            else {
                m_encoder->SetNoMask();
                m_encoder->SetUniformSIMDSize(simdmode);
                m_encoder->SetSrcRegion(0, 0, 1, 0);
                m_encoder->Add(gatherOff, eOffset, immVar);
                m_encoder->Push();
            }
        }

        m_encoder->SetNoMask();
        m_encoder->SetUniformSIMDSize(simdmode);
        if (useA64)
        {
            emitGatherA64(inst, gatherDst, gatherOff, blkBits, nBlks, true);
        }
        else
        {
            m_encoder->SetNoMask();
            m_encoder->SetUniformSIMDSize(simdmode);
            m_encoder->ByteGather(gatherDst, resource, gatherOff, blkBits, nBlks);
        }
        m_encoder->Push();

        return;
    }

    if (useA64)
    {
        IGC_ASSERT_MESSAGE(!isToSLM, "SLM's ptr size should be 32!");

        uint32_t dstOffset = 0;
        uint32_t bytesRemaining = totalBytes;
        uint32_t bytesToRead = 0;

        // Emits instructions generating one or more A64 OWORD block read instructions
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
        m_encoder->SetSrcRegion(0, 0, 1, 0);

        CVariable* pTempVar = m_currShader->GetNewVariable(
            numLanes(SIMDMode::SIMD1),
            ISA_TYPE_UQ,
            EALIGN_QWORD, true,
            CName::NONE);

        m_encoder->Copy(pTempVar, src);
        m_encoder->Push();

        while (bytesRemaining)
        {
            bytesToRead = getBlockMsgSize(bytesRemaining, m_currShader->m_Platform->getMaxBlockMsgSize(false));
            bytesRemaining -= bytesToRead;
            m_encoder->OWLoadA64(dest, pTempVar, bytesToRead, dstOffset);
            m_encoder->Push();
            dstOffset += bytesToRead;

            if (bytesRemaining)
            {
                if (!IGC_COUNT_FLAG_IN_LIMIT(DelayEmuInt64AddLimit) && !m_currShader->m_Platform->hasInt64Add()) {
                    CVariable* ImmVar = m_currShader->ImmToVariable(bytesToRead, ISA_TYPE_UD);
                    emitAddPair(pTempVar, pTempVar, ImmVar);
                }
                else {
                    m_encoder->SetSimdSize(SIMDMode::SIMD1);
                    m_encoder->SetNoMask();
                    m_encoder->SetSrcRegion(0, 0, 1, 0);
                    m_encoder->Add(pTempVar, pTempVar, m_currShader->ImmToVariable(bytesToRead, ISA_TYPE_UQ));
                    m_encoder->Push();
                }
            }
        }
    }
    else
    {
        // Emits below instructions generating one or more OWORD block read instructions:
        // mov (1)   r20.0<1>:ud r5.1<0;1,0>:ud {Align1, Q1, NoMask, Compacted}
        // and (1)   r21.5<1>:ud r0.5<0;1,0>:ud 0x3ff:ud {Align1, NoMask}
        // mov (1)   r21.2<1>:ud r20.0<0;1,0>:ud {Align1, NoMask, Compacted}
        // send (16) r12.0<1>:w  r21 0xa 0x24844ff:ud{Align1, NoMask}// unaligned oword block read

        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
        m_encoder->SetSrcRegion(0, 0, 1, 0);

        CVariable* pTempVar = m_currShader->GetNewVariable(
            numLanes(SIMDMode::SIMD1),
            ISA_TYPE_UD,
            EALIGN_DWORD,
            CName::NONE);

        if (isToSLM)
        {
            // It is OW-aligned OW address
            m_encoder->Shr(pTempVar, src, m_currShader->ImmToVariable(4, ISA_TYPE_UD));
        }

        m_encoder->Push();

        uint32_t dstOffset = 0;
        uint32_t bytesToRead = 0;
        uint32_t bytesRemaining = totalBytes;
        bool isFirstIter = true;
        while (bytesRemaining)
        {

            bytesToRead = getBlockMsgSize(bytesRemaining, m_currShader->m_Platform->getMaxBlockMsgSize(isToSLM));
            bytesRemaining -= bytesToRead;

            bool useSrc = isFirstIter && !isToSLM;
            m_encoder->OWLoad(dest, resource, useSrc ? src : pTempVar, isToSLM, bytesToRead, dstOffset);
            m_encoder->Push();
            dstOffset += bytesToRead;

            if (bytesRemaining)
            {
                uint32_t offset = (isToSLM ? bytesToRead / 16 : bytesToRead);
                m_encoder->SetSimdSize(SIMDMode::SIMD1);
                m_encoder->SetNoMask();
                m_encoder->SetSrcRegion(0, 0, 1, 0);
                m_encoder->Add(pTempVar, useSrc ? src : pTempVar, m_currShader->ImmToVariable(offset, ISA_TYPE_UD));
                m_encoder->Push();
            }
            isFirstIter = false;
        }
    }

    // Destination has other layout than expecting one by block write instructions in case of multiple instances.
    // The expected layout:
    //  |0th component of data from thread 0-15 |1st component of data from thread 0-15 |
    //  |0th component of data from thread 16-31|1st component of data from thread 16-31|
    // The current layout:
    //  |0th component of data from thread 0-15 |0th component of data from thread 16-31|
    //  |1st component of data from thread 0-15 |1st component of data from thread 16-31|
    if (needsTempDst)
    {
        IGC_ASSERT_MESSAGE(!m_encoder->IsSecondHalf(), "This emitter must be called only once for simd32!");
        const uint32_t numVectorElementsPerSimd = numLanes(m_encoder->GetSimdSize());

        for (uint32_t i = 0; i < 2; i++)
        {
            if (i == 1)
            {
                m_encoder->SetSecondHalf(true);
            }

            for (uint32_t elementIndex = 0; elementIndex < nbElements; elementIndex++)
            {
                // Offsets can be deduced from the upper comment.
                CVariable* destinationAlias = m_currShader->GetNewAlias(
                    m_destination,
                    m_destination->GetType(),
                    numVectorElementsPerSimd * elementIndex * m_encoder->GetCISADataTypeSize(m_destination->GetType()),
                    numVectorElementsPerSimd);
                CVariable* sourceAlias = m_currShader->GetNewAlias(
                    dest,
                    dest->GetType(),
                    numVectorElementsPerSimd * (nbElements * elementIndex + i) * m_encoder->GetCISADataTypeSize(dest->GetType()),
                    numVectorElementsPerSimd);

                m_encoder->SetSimdSize(m_encoder->GetSimdSize());
                m_encoder->SetNoMask();
                m_encoder->Copy(destinationAlias, sourceAlias);
                m_encoder->Push();
            }
        }

        m_encoder->SetSecondHalf(false);
    }
}

// If Block Write/Read was created in GenerateBlockMemOpsPass special alignment can be requiered.
// This function extracts alignment requirements from the attributes of the call instruction.
uint32_t EmitPass::getReqBlkBitsForBlockStLd(CallInst *call) {
    llvm::AttributeList attrs = call->getAttributes();
    llvm::AttributeSet set = attrs.getFnAttrs();

    if (set.hasAttribute("alignmentrequirements")) {
        llvm::Attribute attr = set.getAttribute("alignmentrequirements");
        llvm::StringRef attrValue = attr.getValueAsString();
        // Get required alignment from metadata.
        if (attrValue == "4") {
            return 32;
        } else if (attrValue == "8") {
            return 64;
        }
    }

    return 0;
}

void EmitPass::emitLSCSimdBlockWrite(llvm::Instruction* inst, llvm::Value* ptrVal)
{
    Value* llPtr = inst->getOperand(0);
    Value* dataPtr = inst->getOperand(1);

    PointerType* ptrType = cast<PointerType>(llPtr->getType());
    ResourceDescriptor resource = GetResourceVariable(llPtr);

    CVariable* src = nullptr;
    if (ptrVal)
    {
        src = GetSymbol(ptrVal);
        src = m_currShader->BitCast(src, GetUnsignedType(src->GetType()));
    }
    else
    {
        src = GetSymbol(llPtr);
    }

    CVariable* data = GetSymbol(dataPtr);
    bool useA64 = isA64Ptr(ptrType, m_currShader->GetContext());
    LSC_ADDR_SIZE addrSize = useA64 ? LSC_ADDR_SIZE_64b : LSC_ADDR_SIZE_32b;

    if (!data->IsUniform() && !IsGRFAligned(data, EALIGN_GRF))
    {
        CVariable* temp =
            m_currShader->GetNewVariable(numLanes(m_SimdMode), data->GetType(), EALIGN_GRF, CName::NONE);
        m_encoder->Copy(temp, data);
        data = temp;
    }

    Type* Ty = dataPtr->getType();
    IGCLLVM::FixedVectorType* VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
    uint32_t nbElements = 0;
    nbElements = VTy ? int_cast<uint32_t>(VTy->getNumElements()) : 1;

    uint32_t typeSizeInBytes = Ty->getScalarSizeInBits() / 8;
    uint32_t totalBytes = nbElements * typeSizeInBytes * numLanes(m_SimdMode);

    // Special case for uniform data. data is expected to be non-uniform.
    data = BroadcastIfUniform(data);

    uint32_t bytesRemaining = totalBytes;
    uint32_t srcOffset = 0;
    int32_t immOffset = 0;

    // Emits instructions generating one or more A64 OWORD block write instructions
    // The amount of data we need to write is n * Component Size OWORDs.
    // We can write 8, 4, or 2 OWORDs at a time. We can also write 1 OWORD,
    // but since this is a SIMD opcode and we're  compiling SIMD8, SIMD16,
    // we don't expect to see a 1 OWORD write.

    m_encoder->SetSimdSize(SIMDMode::SIMD1);
    m_encoder->SetNoMask();
    m_encoder->SetSrcRegion(0, 0, 1, 0);

    CVariable* pTempVar = m_currShader->GetNewVariable(
        numLanes(SIMDMode::SIMD1),
        useA64 ? ISA_TYPE_UQ : ISA_TYPE_UD,
        EALIGN_GRF,
        true,
        CName(src->getName(),"_64b"));

    m_encoder->Copy(pTempVar, m_currShader->BitCast(src, pTempVar->GetType()));
    m_encoder->Push();

    while (bytesRemaining)
    {
        uint32_t bytesToRead = getLSCBlockMsgSize(bytesRemaining, m_currShader->m_Platform->getMaxLSCBlockMsgSize());

        uint32_t blkBits = getReqBlkBitsForBlockStLd(cast<CallInst>(inst));
        if (!blkBits)
        {
            blkBits = 64;
        }

        uint32_t nBlks = bytesToRead * 8 / blkBits;

        emitLSCStore(inst, data, pTempVar, blkBits, nBlks, srcOffset, &resource, addrSize, LSC_DATA_ORDER_TRANSPOSE, immOffset, 1);
        m_encoder->Push();

        bytesRemaining -= bytesToRead;
        srcOffset += bytesToRead;
        immOffset += bytesToRead;
    }
}

void EmitPass::emitLSCSimdBlockRead(llvm::Instruction* inst, llvm::Value* ptrVal)
{
    Value* llPtr = inst->getOperand(0);
    PointerType* ptrType = cast<PointerType>(llPtr->getType());
    ResourceDescriptor resource = GetResourceVariable(llPtr);

    bool isPrefetch = m_destination == nullptr;

    CVariable* src = nullptr;
    if (ptrVal)
    {
        src = GetSymbol(ptrVal);
        src = m_currShader->BitCast(src, GetUnsignedType(src->GetType()));
    }
    else
    {
        src = GetSymbol(llPtr);
    }

    bool useA64 = isA64Ptr(ptrType, m_currShader->GetContext());
    LSC_ADDR_SIZE addrSize = useA64 ? LSC_ADDR_SIZE_64b : LSC_ADDR_SIZE_32b;

    uint32_t nbElements = 0;
    uint32_t typeSizeInBytes = 0;
    if (isPrefetch)
    {
        LSC_DATA_SIZE lscDataSize = (LSC_DATA_SIZE)GetImmediateVal(inst->getOperand(1));
        LSC_DATA_ELEMS lscDataElems = (LSC_DATA_ELEMS)GetImmediateVal(inst->getOperand(2));

        switch (lscDataSize)
        {
            case LSC_DATA_SIZE_8b:
                typeSizeInBytes = 1; break;
            case LSC_DATA_SIZE_16b:
                typeSizeInBytes = 2; break;
            case LSC_DATA_SIZE_32b:
                typeSizeInBytes = 4; break;
            case LSC_DATA_SIZE_64b:
                typeSizeInBytes = 8; break;
            default:
                IGC_ASSERT_EXIT_MESSAGE(0, "invalid data type");
        }

        switch (lscDataElems)
        {
            case LSC_DATA_ELEMS_1:
                nbElements = 1; break;
            case LSC_DATA_ELEMS_2:
                nbElements = 2; break;
            case LSC_DATA_ELEMS_4:
                nbElements = 4; break;
            case LSC_DATA_ELEMS_8:
                nbElements = 8; break;
            case LSC_DATA_ELEMS_16:
                nbElements = 16; break;
            default:
                IGC_ASSERT_EXIT_MESSAGE(0, "invalid number of elems");
        }
    }
    else
    {
        Type* Ty = inst->getType();
        IGCLLVM::FixedVectorType* VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
        nbElements = VTy ? int_cast<uint32_t>(VTy->getNumElements()) : 1;
        typeSizeInBytes = Ty->getScalarSizeInBits() / 8;
    }

    IGC_ASSERT(nbElements > 0 && typeSizeInBytes > 0);

    uint32_t totalBytes = nbElements * typeSizeInBytes * numLanes(m_SimdMode);

    uint32_t bytesRemaining = totalBytes;
    uint32_t dstOffset = 0;
    int32_t immOffset = 0;

    m_encoder->SetSimdSize(SIMDMode::SIMD1);
    m_encoder->SetNoMask();
    m_encoder->SetSrcRegion(0, 0, 1, 0);

    CVariable* pTempVar = m_currShader->GetNewVariable(
        numLanes(SIMDMode::SIMD1),
        src->GetType(),
        EALIGN_GRF, true,
        CName(src->getName(),"_64b"));

    m_encoder->Copy(pTempVar, src);
    m_encoder->Push();

    LSC_DOC_ADDR_SPACE addrSpace = m_pCtx->getUserAddrSpaceMD().Get(inst);
    LSC_CACHE_OPTS cacheOpts = isPrefetch ?
        translateLSCCacheControlsFromValue(inst->getOperand(3), true) :
        translateLSCCacheControlsFromMetadata(inst, true);

    // If type size >= 8 bytes, assume 8byte aligned and use D64 Transpose message;
    // otherwise, use D32 transpose message.
    bool isD64 = (typeSizeInBytes >= 8);
    uint32_t blkBits = getReqBlkBitsForBlockStLd(cast<CallInst>(inst));
    if (!blkBits)
    {
        blkBits = isD64 ? 64 : 32;
    }

    while (bytesRemaining)
    {
        uint32_t bytesToRead = getLSCBlockMsgSize(bytesRemaining, m_currShader->m_Platform->getMaxLSCBlockMsgSize(isD64));
        uint32_t nBlks = (bytesToRead * 8) / blkBits;

        emitLSCLoad(cacheOpts, m_destination, pTempVar, blkBits, nBlks, dstOffset, &resource, addrSize, LSC_DATA_ORDER_TRANSPOSE, immOffset, 1, addrSpace);
        m_encoder->Push();

        bytesRemaining -= bytesToRead;
        dstOffset += bytesToRead;
        immOffset += bytesToRead;
    }

    return;
}

void EmitPass::emitMediaBlockIO(const llvm::GenIntrinsicInst* inst, bool isRead)
{
    bool isBindless = !isa<ConstantInt>(inst->getOperand(0));
    uint32_t BTI = 0;

    if (!isBindless)
    {
        uint ImgArgIndex = (uint)GetImmediateVal(inst->getOperand(0));
        uint isImageTypeUAV = (uint)GetImmediateVal(inst->getOperand(3));

        BTI = isImageTypeUAV ?
            m_currShader->m_pBtiLayout->GetUavIndex(ImgArgIndex) :
            m_currShader->m_pBtiLayout->GetTextureIndex(ImgArgIndex);

        bool directIdx = (llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(0))) ? true : false;
        m_currShader->SetBindingTableEntryCountAndBitmap(directIdx, isImageTypeUAV ? UAV : RESOURCE, ImgArgIndex, BTI);
    }

    CVariable* pImg = isBindless ? GetSymbol(inst->getOperand(0)) : m_currShader->ImmToVariable(BTI, ISA_TYPE_UD);

    // width and height must be supplied as compile time constants.
    uint blockWidth = (uint)cast<ConstantInt>(inst->getOperand(4))->getZExtValue();
    uint blockHeight = (uint)cast<ConstantInt>(inst->getOperand(5))->getZExtValue();

    auto* pFunc = inst->getCalledFunction();
    auto* pDataType = isRead ? pFunc->getReturnType() : inst->getOperand(6)->getType();

    uint typeSize = isa<IGCLLVM::FixedVectorType>(pDataType) ?
        (uint)m_DL->getTypeSizeInBits(cast<IGCLLVM::FixedVectorType>(pDataType)->getElementType()) / 8 :
        (uint)m_DL->getTypeSizeInBits(pDataType) / 8;

    uint widthInBytes = blockWidth * typeSize;

    CVariable* pXOffset = GetSymbol(inst->getOperand(1));
    CVariable* pYOffset = GetSymbol(inst->getOperand(2));

    CVariable* pDst = nullptr;

    auto* pData = isRead ? m_destination : BroadcastIfUniform(GetSymbol(inst->getOperand(6)));

    // For SIMD32, we need to rearrange the data from both halves
    // into a contiguous block to treat it as one SIMD32 write and
    // we need to split a read back into its two instances after
    // doing the read.
    bool mergeBlock = (m_SimdMode == SIMDMode::SIMD32);
    uint16_t numElements = pData->GetNumberElement();
    VISA_Type dataType = pData->GetType();

    if (mergeBlock)
    {
        // Make a block twice the size to hold data for both halves
        pDst = m_currShader->GetNewVariable(numElements * 2,
            dataType, pData->GetAlign(), false, 1, CName::NONE);
    }
    else
    {
        pDst = pData;
    }

    auto BlockCopy = [&](
        CVariable* pDst1,
        CVariable* pSrc1,
        CVariable* pDst2,
        CVariable* pSrc2,
        uint srcStride,
        uint dstStride)
    {
        auto VecCopy = [&](CVariable* pDst, CVariable* pSrc, uint nElts)
        {
            for (uint32_t i = 0; i < nElts; ++i)
            {
                m_encoder->SetSrcSubReg(0, srcStride * 16 * i);
                m_encoder->SetDstSubReg(dstStride * 16 * i);
                m_encoder->Copy(pDst, pSrc);
                m_encoder->Push();
            }
        };

        uint nElts = isa<VectorType>(pDataType) ?
            (uint)cast<IGCLLVM::FixedVectorType>(pDataType)->getNumElements() :
            1;

        // Now, do the copies.
        bool isSecondHalf = m_encoder->IsSecondHalf();

        m_encoder->SetSecondHalf(false);
        VecCopy(pDst1, pSrc1, nElts);

        m_encoder->SetSecondHalf(true);
        VecCopy(pDst2, pSrc2, nElts);

        m_encoder->SetSecondHalf(isSecondHalf);
    };

    CVariable* pSecondHalf = m_currShader->GetNewAlias(pDst, dataType,
        16 * m_encoder->GetCISADataTypeSize(dataType), numElements);

    if (!isRead && mergeBlock)
    {
        BlockCopy(pDst, pData, pSecondHalf, pData, 1, 2);
    }

    auto surfaceType = isBindless ? ESURFACE_BINDLESS : ESURFACE_NORMAL;
    if (m_currShader->m_Platform->isCoreChildOf(IGFX_XE2_HPG_CORE))
    {
        m_encoder->LSC_Typed2dBlock(LSC_STORE_BLOCK2D, pDst, surfaceType, pImg, pXOffset, pYOffset, (int)widthInBytes, (int)blockHeight);
    }
    else
    {
        m_encoder->MediaBlockMessage(
            isRead ? ISA_Opcode::ISA_MEDIA_LD : ISA_Opcode::ISA_MEDIA_ST,
            pDst,
            surfaceType,
            pImg,
            pXOffset,
            pYOffset,
            0,
            (unsigned char)widthInBytes,
            (unsigned char)blockHeight,
            0);
    }

    if (isRead && mergeBlock)
    {
        BlockCopy(m_destination, pDst, m_destination, pSecondHalf, 2, 1);
    }
}

void EmitPass::emitMediaBlockRectangleRead(llvm::Instruction* inst)
{
    CVariable* xOffset = GetSymbol(inst->getOperand(1));
    CVariable* yOffset = GetSymbol(inst->getOperand(2));

    uint32_t bindingTableIndex = 0;

    bool isBindless = !isa<ConstantInt>(inst->getOperand(0));

    if (!isBindless)
    {
        int SrcImgBTI = int_cast<int>(GetImmediateVal(inst->getOperand(0)));
        int isImageTypeUAV = int_cast<int>(GetImmediateVal(inst->getOperand(3)));
        bindingTableIndex = isImageTypeUAV ?
            m_currShader->m_pBtiLayout->GetUavIndex(SrcImgBTI) :
            m_currShader->m_pBtiLayout->GetTextureIndex(SrcImgBTI);

        bool directIdx = (llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(0))) ? true : false;
        m_currShader->SetBindingTableEntryCountAndBitmap(directIdx, isImageTypeUAV ? UAV : RESOURCE, SrcImgBTI, bindingTableIndex);
    }

    CVariable* src = isBindless ? GetSymbol(inst->getOperand(0)) : m_currShader->ImmToVariable(bindingTableIndex, ISA_TYPE_UD);

    CVariable* pDst = GetSymbol(inst->getOperand(6));

    // width and height must be supplied as compile time constants.
    uint64_t blockWidth = cast<ConstantInt>(inst->getOperand(4))->getZExtValue();
    uint64_t blockHeight = cast<ConstantInt>(inst->getOperand(5))->getZExtValue();

    IGC_ASSERT(blockWidth * blockHeight == pDst->GetSize());

    auto surfaceType = isBindless ? ESURFACE_BINDLESS : ESURFACE_NORMAL;
    if (m_currShader->m_Platform->isCoreChildOf(IGFX_XE2_HPG_CORE))
    {
        m_encoder->LSC_Typed2dBlock(LSC_LOAD_BLOCK2D, pDst, surfaceType, src, xOffset, yOffset, (int)blockWidth, (int)blockHeight);
    }
    else
    {
        m_encoder->MediaBlockMessage(
            ISA_Opcode::ISA_MEDIA_LD,
            pDst,
            surfaceType,
            src,
            xOffset,
            yOffset,
            0,
            (unsigned char)blockWidth,
            (unsigned char)blockHeight,
            0);
    }

    m_encoder->Push();
}

void EmitPass::emitSimdMediaBlockRead(llvm::Instruction* inst)
{
    uint32_t nbElements = 1;
    if (inst->getType()->isVectorTy())
    {
        nbElements = (uint32_t)cast<IGCLLVM::FixedVectorType>(inst->getType())->getNumElements();
    }
    IGC_ASSERT_MESSAGE(nbElements <= 16, "InValid Vector Size");

    Value* xOffset = inst->getOperand(1);
    Value* yOffset = inst->getOperand(2);

    uint32_t typeSizeInBytes = inst->getType()->getScalarType()->getScalarSizeInBits() / 8;
    uint32_t totalWidth = typeSizeInBytes * numLanes(m_SimdMode);

    uint32_t   pass_axisX = 0;
    uint32_t   numPasses_axisX = 0;

    uint32_t   pass_axisY = 0;
    uint32_t   numPasses_axisY = 0;

    uint32_t   bindingTableIndex = 0;

    uint32_t dstSubReg = 0;
    uint32_t blockWidth = 0;
    uint32_t blockHeight = nbElements;
    uint32_t blockHeight_step = 0;

    bool isBindless = !isa<ConstantInt>(inst->getOperand(0));

    if (!isBindless)
    {
        int SrcImgBTI = int_cast<int>(GetImmediateVal(inst->getOperand(0)));
        int isImageTypeUAV = int_cast<int>(GetImmediateVal(inst->getOperand(3)));
        if (isImageTypeUAV)
        {
            bindingTableIndex = m_currShader->m_pBtiLayout->GetUavIndex(SrcImgBTI);
        }
        else // elseif imageType is Resource
        {
            bindingTableIndex = m_currShader->m_pBtiLayout->GetTextureIndex(SrcImgBTI);
        }

        m_currShader->SetBindingTableEntryCountAndBitmap(true, isImageTypeUAV ? UAV : RESOURCE, SrcImgBTI, bindingTableIndex);
    }

    CVariable* src = isBindless ? GetSymbol(inst->getOperand(0)) : m_currShader->ImmToVariable(bindingTableIndex, ISA_TYPE_UD);
    uint32_t maxWidth = 32;

    if (totalWidth < maxWidth)
    {
        numPasses_axisX = 1;
        blockWidth = totalWidth;
    }
    else
    {
        IGC_ASSERT(maxWidth);
        IGC_ASSERT_MESSAGE(totalWidth % maxWidth == 0, "Total width must be divisible by 32!");
        numPasses_axisX = totalWidth / maxWidth;
        blockWidth = maxWidth;
    }

    if (m_currShader->m_Platform->has64BMediaBlockRW())
    {
        maxWidth = 64;
        if (totalWidth > 32 && blockHeight <= 4 && (totalWidth % maxWidth == 0))
        {
            // do 64 byte wide read
            numPasses_axisX = totalWidth / maxWidth;
            blockWidth = maxWidth;
        }
    }

    CVariable* pTempVar_axis_X_offset = nullptr;
    CVariable* pTempVar_axis_Y_offset = nullptr;

    uint32_t blockRegSize = 0;

    //Following variable declaration is SIMD8 based, UD is used, so blockRegSize is total required registers.
    auto simdMode = lanesToSIMDMode(blockWidth / typeSizeInBytes);
    blockRegSize = numPasses_axisX * blockHeight * numLanes(simdMode);

    CVariable* pTempDest = m_currShader->GetNewVariable(
        blockRegSize,
        m_destination->GetType(),
        m_currShader->getGRFAlignment(),
        CName::NONE);

    CVariable* xVar = GetSymbol(xOffset);
    CVariable* yVar = GetSymbol(yOffset);

    int scale = blockWidth / getGRFSize();

    const int MediaBlockReadByteMax = 256;
    int totalBytesToLoadOneSend = (blockRegSize * typeSizeInBytes) / numPasses_axisX;

    if (totalBytesToLoadOneSend > MediaBlockReadByteMax)
    {
        numPasses_axisY = totalBytesToLoadOneSend / MediaBlockReadByteMax;
        // Slice the block horizontally by number of axisY passes
        blockHeight_step = blockHeight / numPasses_axisY;
    }
    else
    {
        numPasses_axisY = 1;
    }

    int blockHeight2Load = numPasses_axisY > 1 ?
        // For case when one send cannot handle the whole load
        blockHeight_step :
        // For normal case
        blockHeight;

    // Emits a MEDIA_BLOCK_READ instruction.
    // Considering block width as x-axis and block height as y axis:
    // Pass 0 reads from (xOffset,yOffset) to (xOffset+31, yOffset+blockheight)
    // Pass 1 reads from (xOffset+32, yOffset) to (xOffset+63, yOffset+blockheight)
    // Instructions generated:
    // mov(1) r36.1<1>:d r16.0<0; 1, 0>:d{ Align1, NoMask }
    // mov(1) r36.2<1>:ud 0x3001f:ud{ Align1, NoMask }
    // mov(1) r36.0<1>:ud r15.0<0; 1, 0>:ud{ Align1, NoMask, Compacted }
    // send(8) r28.0<1>:ud r36 0xc 0x2490000:ud{ Align1, NoMask } // media block read
    // add(1) r36.0<1>:ud r15.0<0; 1, 0>:ud 0x20:uw{ Align1, NoMask }
    // mov(1) r36.1<1>:d r13.1<0; 1, 0>:d{ Align1, NoMask }
    // send(8) r32.0<1>:ud r36 0xc 0x2490000:ud{ Align1, NoMask } // media block read
    //      -----------------
    //      |       |       |
    //      |       |       |
    //      -----------------
    //      ---------  r28 output
    //      |       |
    //      |       |
    //      ---------  r32
    //      |       |
    //      |       |
    //      ---------
    //  32 or 64 bytes at most, that's the reason simd8 is used.
    for (pass_axisX = 0; pass_axisX < numPasses_axisX; pass_axisX++)
    {
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
        m_encoder->SetSrcRegion(0, 0, 1, 0);

        if (pass_axisX == 0)
        {
            pTempVar_axis_X_offset = m_currShader->GetNewVariable(
                numLanes(m_SimdMode),
                ISA_TYPE_UD,
                EALIGN_DWORD,
                CName::NONE);

            m_encoder->Copy(pTempVar_axis_X_offset, xVar);
        }
        else
        {
            m_encoder->Add(pTempVar_axis_X_offset, pTempVar_axis_X_offset, m_currShader->ImmToVariable(blockWidth, ISA_TYPE_UD));
            uint32_t subOffset = blockWidth * blockHeight2Load;
            subOffset /= getGRFSize();
            dstSubReg = dstSubReg + subOffset;
        }
        m_encoder->Push();

        // In case when we wanted to load more than MediaBlockLoadByteMax=256 data in
        // one send instruction. For such scenario we are slicing the data horizontally, example:
        //
        // media_ld : grf_0  [.........]
        //            grf_1  [.........]
        //            ...
        //            grf_x  [.........]
        //            ...
        //            grf_15 [.........]
        // In such example we wanted to load 512 bytes - the limitation is 256 bytes, so slice
        // input data for two sends:
        //
        // media_ld_0 : grf_0  [.........]
        //              grf_1  [.........]
        //              ...
        //              grf_x  [.........]
        //              ...
        //              grf_7  [.........]
        // media_ld_1 : grf_8  [.........]
        //              grf_9  [.........]
        //              ...
        //              grf_x  [.........]
        //              ...
        //              grf_15 [.........]
        for (pass_axisY = 0; pass_axisY < numPasses_axisY; ++pass_axisY)
        {
            m_encoder->SetSimdSize(SIMDMode::SIMD1);
            m_encoder->SetNoMask();
            m_encoder->SetSrcRegion(0, 0, 1, 0);

            if (pass_axisY == 0)
            {
                pTempVar_axis_Y_offset = m_currShader->GetNewVariable(
                    numLanes(m_SimdMode),
                    ISA_TYPE_UD,
                    EALIGN_DWORD,
                    CName::NONE);

                m_encoder->Copy(pTempVar_axis_Y_offset, yVar);
            }
            else
            {
                m_encoder->Add(pTempVar_axis_Y_offset, pTempVar_axis_Y_offset, m_currShader->ImmToVariable(blockHeight_step, ISA_TYPE_UD));
                uint32_t subOffset = (MediaBlockReadByteMax * pass_axisY) / numPasses_axisX;
                subOffset /= getGRFSize();
                dstSubReg = dstSubReg + subOffset;
            }
            m_encoder->Push();

            m_encoder->SetDstSubVar(dstSubReg);

            CVariable* dstVar = numPasses_axisX == 1 ? m_destination : pTempDest;

            auto surfaceType = isBindless ? ESURFACE_BINDLESS : ESURFACE_NORMAL;
            if (m_currShader->m_Platform->isCoreChildOf(IGFX_XE2_HPG_CORE))
            {
                m_encoder->LSC_Typed2dBlock(
                    LSC_LOAD_BLOCK2D,
                    dstVar,
                    surfaceType,
                    src,
                    pTempVar_axis_X_offset,
                    pTempVar_axis_Y_offset,
                    (int)blockWidth,
                    (int)blockHeight2Load);
            }
            else
            {
                m_encoder->MediaBlockMessage(
                    ISA_Opcode::ISA_MEDIA_LD,
                    dstVar,
                    surfaceType,
                    src,
                    pTempVar_axis_X_offset,
                    pTempVar_axis_Y_offset,
                    0,
                    (unsigned char)blockWidth,
                    (unsigned char)blockHeight2Load,
                    0);
            }
            m_encoder->Push();
        }
    }

    if (numPasses_axisX > 1)
    {
        dstSubReg = 0;

        uint32_t srcSubReg = 0;

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


        //For 64 bytes GRF, 32 bytes will be extended to
        //.....
        //  A0....A1
        //  B0....B1
        //  C0....C1
        //  D0....D1
        //  E0....E1
        //  F0....F1
        //  G0....G1
        //  H0....H1
        //
        //  r20....A0....B0........r30....A1....B1
        //  r21....C0....D0........r31....C1....D1
        //  r22....E0....F0........r32....E1....F1
        //  r23....G0....H0........r33....G1....H1
        //
        //  r40<--r20,....r30
        //  r41<--r20.8,r30.8
        //  r42<--r21,....r31
        //  r43<--r21.8,r31.8
        //  r44<--r22,....r32
        //  r45<--r22.8,r32.8
        //  r46<--r23,....r33
        //  r47<--r23.8,r33.8
        //
        //mov (8) r40.0<1>:ud       r20.0<8;8,1>:ud {Align1, Q1}
        //mov (8) r40.8<1>:ud       r30.0<8;8,1>:ud {Align1, Q1}
        //mov (8) r41<1>:ud         r20.8<8;8,1>:ud {Align1, Q1}
        //mov (8) r41.8<1>:ud       r30.8<8;8,1>:ud {Align1, Q1}

        for (uint32_t i = 0; i < blockHeight; i++) //Height
        {
            uint32_t dstSubRegOffset = 0;
            uint32_t srcSubRegOffset = 0;

            for (uint32_t pass = 0; pass < numPasses_axisX; pass++) //Width
            {
                m_encoder->SetSimdSize(simdMode);
                m_encoder->SetNoMask();

                srcSubReg = ((i + blockHeight * pass) * blockWidth) / getGRFSize();
                srcSubRegOffset = (i * blockWidth) % getGRFSize();

                m_encoder->SetSrcSubVar(0, srcSubReg);
                m_encoder->SetSrcSubReg(0, srcSubRegOffset / typeSizeInBytes);

                m_encoder->SetDstSubVar(dstSubReg);
                m_encoder->SetDstSubReg(dstSubRegOffset / typeSizeInBytes);

                dstSubRegOffset = ((pass + 1) * blockWidth) % getGRFSize();
                if (dstSubRegOffset == 0)
                {
                    dstSubReg += (scale > 0 ? scale : 1);
                }

                m_encoder->Copy(m_destination, pTempDest);
                m_encoder->Push();
            }
        }
    }
}

void EmitPass::emitSimdMediaBlockWrite(llvm::Instruction* inst)
{
    Value* xOffset = inst->getOperand(1);
    Value* yOffset = inst->getOperand(2);
    Value* dataPtr = inst->getOperand(4);

    uint32_t nbElements = 1;
    if (dataPtr->getType()->isVectorTy())
    {
        nbElements = (uint32_t)cast<IGCLLVM::FixedVectorType>(dataPtr->getType())->getNumElements();
    }
    IGC_ASSERT_MESSAGE(nbElements <= 16, "InValid Vector Size");

    CVariable* data = GetSymbol(dataPtr);
    data = BroadcastIfUniform(data);

    uint32_t typeSizeInBytes = dataPtr->getType()->getScalarType()->getScalarSizeInBits() / 8;
    uint32_t totalWidth = typeSizeInBytes * numLanes(m_SimdMode);

    uint32_t   pass_axisX = 0;
    uint32_t   numPasses_axisX = 0;

    uint32_t   pass_axisY = 0;
    uint32_t   numPasses_axisY = 0;

    uint32_t blockWidth = 0;
    uint32_t blockHeight = nbElements;
    uint32_t blockHeight_step = 0;
    uint32_t bindingTableIndex = 0;

    bool isBindless = !isa<ConstantInt>(inst->getOperand(0));

    if (!isBindless)
    {
        int SrcImgBTI = int_cast<int>(GetImmediateVal(inst->getOperand(0)));
        int isImageTypeUAV = int_cast<int>(GetImmediateVal(inst->getOperand(3)));
        if (isImageTypeUAV)
        {
            bindingTableIndex = m_currShader->m_pBtiLayout->GetUavIndex(SrcImgBTI);
        }
        else // elseif imageType is Resource
        {
            bindingTableIndex = m_currShader->m_pBtiLayout->GetTextureIndex(SrcImgBTI);
        }

        m_currShader->SetBindingTableEntryCountAndBitmap(true, isImageTypeUAV ? UAV : RESOURCE, SrcImgBTI, bindingTableIndex);
    }

    CVariable* src = isBindless ? GetSymbol(inst->getOperand(0)) : m_currShader->ImmToVariable(bindingTableIndex, ISA_TYPE_UD);
    uint32_t maxWidth = 32;

    if (totalWidth < maxWidth)
    {
        numPasses_axisX = 1;
        blockWidth = totalWidth;
    }
    else
    {
        IGC_ASSERT(maxWidth);
        IGC_ASSERT_MESSAGE(totalWidth % maxWidth == 0, "Total width must be divisible by 32!");
        numPasses_axisX = totalWidth / maxWidth;
        blockWidth = maxWidth;
    }

    if (m_currShader->m_Platform->has64BMediaBlockRW())
    {
        maxWidth = 64;
        if (totalWidth > 32 && blockHeight <= 4 && totalWidth % maxWidth == 0)
        {
            // do 64 byte wide read
            numPasses_axisX = totalWidth / maxWidth;
            blockWidth = maxWidth;
        }
    }

    CVariable* pTempVar_axis_X_offset = nullptr;
    CVariable* pTempVar_axis_Y_offset = nullptr;

    uint32_t dstSubReg = 0;
    uint32_t srcSubReg = 0;

    int scale = blockWidth / getGRFSize();
    auto simdMode = lanesToSIMDMode(blockWidth / typeSizeInBytes);

    const int MediaBlockStoreByteMax = 256;
    int blockRegSize = numPasses_axisX * blockHeight * numLanes(simdMode);
    int totalBytesToStore = (blockRegSize * typeSizeInBytes) / numPasses_axisX;

    if (totalBytesToStore > MediaBlockStoreByteMax)
    {
        numPasses_axisY = totalBytesToStore / MediaBlockStoreByteMax;
        // Slice the block horizontally by number of axisY passes
        blockHeight_step = blockHeight / numPasses_axisY;
    }
    else
    {
        numPasses_axisY = 1;
    }

    for (pass_axisX = 0; pass_axisX < numPasses_axisX; pass_axisX++)
    {
        uint32_t srcSubVar = pass_axisX * blockWidth / getGRFSize();
        uint32_t dstSubVar = 0;
        uint32_t srcSubRegOffset = (pass_axisX * blockWidth) % getGRFSize();
        uint32_t dstSubRegOffset = 0;

        CVariable* tempdst = nullptr;
        tempdst = m_currShader->GetNewVariable(
            nbElements * numLanes(simdMode),
            data->GetType(),
            m_currShader->getGRFAlignment(),
            CName::NONE);

        // Split the data.
        // mov (8) r22.0<1>:d r14.0<8;8,1>:d {Align1, Q1, Compacted}
        // mov (8) r23.0<1>:d r16.0<8;8,1>:d {Align1, Q1, Compacted}
        // mov (8) r24.0<1>:d r18.0<8;8,1>:d {Align1, Q1, Compacted}
        // mov (8) r25.0<1>:d r20.0<8;8,1>:d {Align1, Q1, Compacted}

        //FOR 64 bytes GRF:
        //    A0....A1....A2....A3........r60....r60.8....r61....r61.8
        //    B0....B1....B2....B3........r62....r62.8....r63....r63.8
        //    C0....C1....C2....C3........r64....r64.8....r65....r65.8
        //    D0....D1....D2....D3........r66....r66.8....r67....r67.8
        //    E0....E1....E2....E3........r68....r68.8....r69....r69.8
        //    F0....F1....F2....F3........r70....r70.8....r71....r71.8
        //    G0....G1....G2....G3........r72....r72.8....r73....r73.8
        //    H0....H1....H2....H3........r74....r74.8....r75....r75.8
        //
        // block 0
        // mov (8) r20.0<1>:d r60.0<8;8,1>:d {Align1, Q1, Compacted}
        // mov (8) r20.8<1>:d r62.0<8;8,1>:d {Align1, Q1, Compacted}
        // mov (8) r21.0<1>:d r64.0<8;8,1>:d {Align1, Q1, Compacted}
        // mov (8) r21.8<1>:d r66.0<8;8,1>:d {Align1, Q1, Compacted}
        // ...
        //block 1
        // mov (8) r30.0<1>:d r60.8<8;8,1>:d {Align1, Q1, Compacted}
        // mov (8) r30.8<1>:d r62.8<8;8,1>:d {Align1, Q1, Compacted}
        // mov (8) r31.0<1>:d r64.8<8;8,1>:d {Align1, Q1, Compacted}
        // mov (8) r31.8<1>:d r66.8<8;8,1>:d {Align1, Q1, Compacted}
        //...

        if (numPasses_axisX > 1)
        {
            for (uint i = 0; i < nbElements; ++i)
            {
                m_encoder->SetSimdSize(simdMode);
                m_encoder->SetNoMask();

                //Src
                m_encoder->SetSrcSubVar(0, srcSubVar);
                m_encoder->SetSrcSubReg(0, srcSubRegOffset / typeSizeInBytes);
                //Dst
                m_encoder->SetDstSubVar(dstSubVar);
                m_encoder->SetDstSubReg(dstSubRegOffset / typeSizeInBytes);
                //Strides for dst and src
                dstSubRegOffset = ((i + 1) * blockWidth) % getGRFSize();
                if (dstSubRegOffset == 0)
                {
                    dstSubVar += scale > 0 ? scale : 1;
                }
                srcSubVar = srcSubVar + (numPasses_axisX * blockWidth / getGRFSize());

                m_encoder->Copy(tempdst, data);
                m_encoder->Push();
            }
        }
        else
        {
            tempdst = data;
        }
        // Emits a MEDIA_BLOCK_WRITE instruction.
        // Considering block width as x-axis and block height as y axis:
        // Pass 0 writes from (xOffset,yOffset) to (xOffset+31, yOffset+blockheight)
        // Pass 1 writes from (xOffset+32, yOffset) to (xOffset+63, yOffset+blockheight)
        // mov (8) r28.0<1>:ud r0.0<8;8,1>:ud {Align1, NoMask, Compacted}
        // mov (1) r28.2<1>:ud 0x3001f:ud {Align1, NoMask}
        // mov (1) r28.0<1>:ud r6.0<0;1,0>:d {Align1, NoMask}
        // mov (1) r28.1<1>:ud r7.0<0;1,0>:d {Align1, NoMask}
        // mov (16) r29.0<1>:ud r22.0<8;8,1>:ud {Align1, NoMask, Compacted}
        // mov (16) r31.0<1>:ud r24.0<8;8,1>:ud {Align1, NoMask, Compacted}
        // send (8) null<1>:ud r28 0xc 0xa0a8002:ud{Align1, NoMask} // media block write
        if (pass_axisX == 0)
        {
            CVariable* xVar = GetSymbol(xOffset);
            m_encoder->SetSimdSize(SIMDMode::SIMD1);
            m_encoder->SetNoMask();
            m_encoder->SetSrcRegion(0, 0, 1, 0);

            pTempVar_axis_X_offset = m_currShader->GetNewVariable(
                numLanes(m_SimdMode),
                ISA_TYPE_D,
                EALIGN_DWORD,
                CName::NONE);

            m_encoder->Cast(pTempVar_axis_X_offset, xVar);
            m_encoder->Push();
        }
        else
        {
            m_encoder->SetSimdSize(SIMDMode::SIMD1);
            m_encoder->SetNoMask();
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->Add(pTempVar_axis_X_offset, pTempVar_axis_X_offset, m_currShader->ImmToVariable(blockWidth, ISA_TYPE_UD));
            m_encoder->Push();
            dstSubReg = dstSubReg + scale * blockHeight;
        }
        // In case when we wanted to store more than MediaBlockStoreByteMax=256 data in
        // one send instruction. For such scenario we are slicing the data horizontally, example:
        //
        // media_st : grf_0  [.........]
        //            grf_1  [.........]
        //            ...
        //            grf_x  [.........]
        //            ...
        //            grf_15 [.........]
        // In such example we wanted to store 512 bytes - the limitation is 256 bytes, so slice
        // input data for two sends:
        //
        // media_st_0 : grf_0  [.........]
        //              grf_1  [.........]
        //              ...
        //              grf_x  [.........]
        //              ...
        //              grf_7  [.........]
        // media_st_1 : grf_8  [.........]
        //              grf_9  [.........]
        //              ...
        //              grf_x  [.........]
        //              ...
        //              grf_15 [.........]
        for (pass_axisY = 0; pass_axisY < numPasses_axisY; ++pass_axisY)
        {
            m_encoder->SetSimdSize(SIMDMode::SIMD1);
            m_encoder->SetNoMask();
            m_encoder->SetSrcRegion(0, 0, 1, 0);

            CVariable* yVar = GetSymbol(yOffset);
            if (pass_axisY == 0)
            {
                pTempVar_axis_Y_offset = m_currShader->GetNewVariable(
                    numLanes(m_SimdMode),
                    ISA_TYPE_D,
                    EALIGN_DWORD,
                    CName::NONE);

                m_encoder->Cast(pTempVar_axis_Y_offset, yVar);
                m_encoder->Push();
            }
            else
            {
                m_encoder->Add(pTempVar_axis_Y_offset, pTempVar_axis_Y_offset, m_currShader->ImmToVariable(blockHeight_step, ISA_TYPE_UD));
                m_encoder->Push();
                uint32_t subOffset = (MediaBlockStoreByteMax * pass_axisY) / numPasses_axisX;
                srcSubReg = srcSubReg + subOffset;

                // Offset the source for next store instr
                tempdst =
                    m_currShader->GetNewAlias(
                        tempdst,
                        tempdst->GetType(),
                        srcSubReg,
                        tempdst->GetNumberElement());
            }

            m_encoder->SetDstSubVar(dstSubReg);

            int blockHeight2Store = numPasses_axisY > 1 ?
                // For case when one send cannot handle the whole store
                blockHeight_step :
                // For normal case
                blockHeight;

            auto surfaceType = isBindless ? ESURFACE_BINDLESS : ESURFACE_NORMAL;
            if (m_currShader->m_Platform->isCoreChildOf(IGFX_XE2_HPG_CORE))
            {
                m_encoder->LSC_Typed2dBlock(
                    LSC_STORE_BLOCK2D,
                    tempdst,
                    surfaceType,
                    src,
                    pTempVar_axis_X_offset,
                    pTempVar_axis_Y_offset,
                    (int)blockWidth,
                    (int)blockHeight2Store);
            }
            else
            {
                m_encoder->MediaBlockMessage(
                    ISA_Opcode::ISA_MEDIA_ST,
                    tempdst,
                    surfaceType,
                    src,
                    pTempVar_axis_X_offset,
                    pTempVar_axis_Y_offset,
                    0,
                    (unsigned char)blockWidth,
                    (unsigned char)blockHeight2Store,
                    0);
            }
            m_encoder->Push();
        }
    }
}


void EmitPass::interceptSamplePayloadCoalescing(
    llvm::SampleIntrinsic* inst,
    uint numPart,
    //out:
    SmallVector<CVariable*, 4> & payload,
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
        IGC_ASSERT(ccTuple);
        CVariable* rootPayloadVar = m_currShader->LazyCreateCCTupleBackingVariable(ccTuple);

        SmallPtrSet<Value*, 8> touchedValuesSet;

        IGC_ASSERT(representativeValPtr);
        IGC_ASSERT(payloadToCCTupleRelativeOffset >= 0);
        int byteOffset = payloadToCCTupleRelativeOffset *
            m_CE->GetSingleElementWidth(m_currShader->m_SIMDSize, m_DL, representativeValPtr);

        if (ccTuple->HasNonHomogeneousElements())
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
                if (inst->IsDerivative())
                {
                    m_encoder->SetNoMask();
                }
                m_encoder->Copy(src, GetSymbol(val));
                m_encoder->Push();

                byteOffset += GetOffsetIncrement(m_DL, m_currShader->m_SIMDSize, val);

                IGC_ASSERT(src);
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
                if (m_CE->GetValueCCTupleMapping(val))
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
            IGC_ASSERT(src);
            payload.push_back(src);

            byteOffset += GetOffsetIncrement(m_DL, m_currShader->m_SIMDSize, val);


        }

    }

}


ResourceDescriptor EmitPass::GetSampleResourceHelper(SampleIntrinsic* inst)
{
    llvm::Value* texOp = inst->getTextureValue();
    ResourceDescriptor resource = GetResourceVariable(texOp);
    return resource;
}

void EmitPass::emitSampleInstruction(SampleIntrinsic* inst)
{
    EOPCODE opCode = GetOpCode(inst);

    m_currShader->m_State.SetHasSample();
    ResourceDescriptor resource = GetSampleResourceHelper(inst);
    bool isEval = isUsedOnlyByEval(inst);
    ResourceDescriptor pairedResource = inst->hasPairedTextureArg() && llvm::isa<llvm::UndefValue>(inst->getPairedTextureValue()) == false ?
        GetResourceVariable(inst->getPairedTextureValue()) :
        ResourceDescriptor();


    //Get sampler index in the array of operands
    llvm::Value* samplerOp = inst->getSamplerValue();
    SamplerDescriptor sampler = GetSamplerVariable(samplerOp);

    const uint numOperands = inst->getNumOperands();
    // offset
    CVariable* immOffset = m_currShader->ImmToVariable(0, ISA_TYPE_UW);
    if (!inst->IsLODInst())
    {
        uint offsetSourceIndex = numOperands - 4;
        immOffset = ComputeSampleIntOffset(inst, offsetSourceIndex);
    }

    const CShader::ExtractMaskWrapper writeMask(m_currShader, inst);
    IGC_ASSERT_MESSAGE(writeMask.hasEM() && writeMask.getEM() != 0, "Wrong write mask");

    bool derivativeSample = inst->IsDerivative();

    bool cpsEnable =
        false;
    cpsEnable &= m_SimdMode != SIMDMode::SIMD32;

    SmallVector<CVariable*, 4>  payload;

    bool doIntercept = true;
    // Skip sample_d* instructions in SIMD16 and SIMD32.
    if ((m_currShader->m_SIMDSize > SIMDMode::SIMD8 &&
        (opCode == llvm_sample_dptr ||
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
                    CVariable* srcReg = m_currShader->GetNewVariable(
                        numLanes(m_currShader->m_SIMDSize), src->GetType(), EALIGN_GRF, CName::NONE);
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
    bool hasMaskResponse = writeMask.isSet(4);

    CVariable* dst = m_destination;
    //When sampler output is 16 bit float, hardware doesnt pack the output in SIMD8 mode.
    //Hence the movs to handle this layout in SIMD8 mode
    bool simd8HFRet = isHalfGRFReturn(m_destination, m_SimdMode);

    bool zeroLOD = m_currShader->m_Platform->supportSampleAndLd_lz() && inst->ZeroLOD() &&
                   !m_currShader->m_Platform->WaDisableSampleLz();

    ModuleMetaData* modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    auto& predicationMap = modMD->predicationMap;

    ResourceLoop(resource, sampler, [&](CVariable* flag, CVariable*& destination,
        ResourceDescriptor resource, bool needLoop) {

        dst = destination;
            if (simd8HFRet)
            {
                dst = m_currShader->GetNewVariable(
                    destination->GetNumberElement() * 2, ISA_TYPE_HF, EALIGN_GRF, false, CName::NONE);
            }

        if (m_currShader->m_Platform->needWaSamplerNoMask())
        {
            m_encoder->SetNoMask();
        }
        else
        {
            if (predicationMap.count(inst))
            {
                m_encoder->SetPredicate(m_currShader->GetSymbol(cast<Instruction>(predicationMap[inst])));
            }
            else
            {
                m_encoder->SetPredicate(flag);
            }
        }
        m_encoder->Sample(
            opCode,
            writeMask.getEM(),
            immOffset,
            resource,
            pairedResource,
            sampler,
            numSources,
            dst,
            payload,
            zeroLOD,
            cpsEnable,
            hasMaskResponse,
            needLoop);
        m_encoder->Push();

        if (m_currShader->hasReadWriteImage(*(inst->getParent()->getParent())))
        {
            CVariable* tempdest = m_currShader->BitCast(destination, GetUnsignedIntegerType(destination->GetType()));
            m_encoder->Cast(m_currShader->GetNULL(), tempdest);
            m_encoder->Push();
            m_encoder->Copy(m_currShader->GetNULL(), m_currShader->GetTSC());
            m_encoder->Push();
        }

        }, m_RLA->GetResourceLoopMarker(inst));

    {
        if (simd8HFRet)
        {
            PackSIMD8HFRet(dst);
        }

        if (hasMaskResponse
            )
        {
            CVariable* flag = m_currShader->GetNewVariable(
                numLanes(m_currShader->m_State.m_dispatchSize), ISA_TYPE_BOOL, EALIGN_BYTE, CName::NONE);
            uint subvar = numLanes(m_currShader->m_SIMDSize) * 4 / (getGRFSize() >> 2);
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
            m_encoder->Select(flag, dstAlias, pred, zero);
            m_encoder->Push();
        }
    }
}


void EmitPass::emitDiscard(llvm::Instruction* inst)
{
    IGC_ASSERT_MESSAGE(0, "No codegen for discard intrinsic");
}

void EmitPass::emitInfoInstruction(InfoIntrinsic* inst)
{
    EOPCODE opCode = GetOpCode(inst);
    llvm::Value* texOp = inst->getOperand(0);

    ResourceDescriptor resource = GetResourceVariable(texOp);

    CVariable* lod = nullptr;
    if (opCode != llvm_sampleinfoptr && opCode != llvm_readsurfacetypeandformat)
    {
        lod = GetSymbol(inst->getOperand(1));
    }
    if (lod && lod->IsUniform())
    {
        auto uniformSIMDMode = m_currShader->m_Platform->getMinDispatchMode();
        CVariable* srcReg = m_currShader->GetNewVariable(
            m_destination->IsUniform() ? numLanes(uniformSIMDMode) : numLanes(m_currShader->m_SIMDSize),
            ISA_TYPE_F,
            EALIGN_GRF,
            m_destination->IsUniform(),
            lod->getName());
        m_encoder->SetUniformSIMDSize(uniformSIMDMode);
        m_encoder->Copy(srcReg, lod);
        m_encoder->Push();
        lod = srcReg;
    }

    CVariable* tempDest = m_destination;
    if (m_destination->IsUniform() || opCode == llvm_readsurfacetypeandformat)
    {
        auto uniformSIMDMode = m_currShader->m_Platform->getMinDispatchMode();
        tempDest = m_currShader->GetNewVariable(
            m_destination->GetNumberElement() * numLanes(uniformSIMDMode),
            ISA_TYPE_UD, EALIGN_GRF, true, m_destination->getName());
        m_encoder->SetUniformSIMDSize(uniformSIMDMode);
    }

    uint label = 0;
    CVariable* flag = nullptr;
    bool needLoop = ResourceLoopHeader(resource, flag, label);
    ResourceLoopSubIteration(resource, flag, label);

    if (opCode == llvm_readsurfacetypeandformat)
    {
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
    }
    else
    {
        m_encoder->SetPredicate(flag);
    }
    const CShader::ExtractMaskWrapper writeMask(m_currShader, inst);
    IGC_ASSERT_MESSAGE(writeMask.hasEM() && writeMask.getEM() != 0, "Wrong write mask");
    m_encoder->Info(opCode, writeMask.getEM(), resource, lod, tempDest);
    m_encoder->Push();

    unsigned int writemask = 0;
    for (auto I = inst->user_begin(), E = inst->user_end(); I != E; ++I)
    {
        if (llvm::ExtractElementInst* extract = llvm::dyn_cast<llvm::ExtractElementInst>(*I))
        {
            if (llvm::ConstantInt* index = llvm::dyn_cast<ConstantInt>(extract->getIndexOperand()))
            {
                writemask |= BIT(static_cast<uint>(index->getZExtValue()));
                continue;
            }
        }
        writemask = 0xF;
        break;
    }

    if (opCode == llvm_readsurfacetypeandformat)
    {
        constexpr uint surfaceInfoSurfaceTypeDword = 4;
        constexpr uint surfaceInfoSurfaceFormatDword = 5;
        // Note: for non-uniform destination, the following copy operations are
        //       done in-place (m_destination == tempDest).
        if (BIT(1) & writemask)
        {
            m_encoder->SetSrcSubReg(0, surfaceInfoSurfaceFormatDword); //surface format
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            if (m_destination->IsUniform())
            {
                m_encoder->SetDstSubReg(1);
            }
            else
            {
                m_encoder->SetDstSubVar((m_encoder->GetSimdSize() == SIMDMode::SIMD32) ? 2 : 1);
            }
            m_encoder->Copy(m_destination, tempDest);
            m_encoder->Push();
        }
        if (BIT(0) & writemask)
        {
            m_encoder->SetSrcSubReg(0, surfaceInfoSurfaceTypeDword); //surface type
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->Copy(m_destination, tempDest);
            m_encoder->Push();
        }
    }
    else if (tempDest != m_destination)
    {
        IGC_ASSERT(m_destination->IsUniform());
        for (uint i = 0; i < 4; i++)
        {
            if (BIT(i) & writemask)
            {
                m_encoder->SetSrcSubVar(0, i);
                m_encoder->SetDstSubReg(i);
                m_encoder->Copy(m_destination, tempDest);
                m_encoder->Push();
            }
        }
    }

    ResourceLoopBackEdge(needLoop, flag, label);
}

void EmitPass::emitSurfaceInfo(GenIntrinsicInst* inst)
{
    ResourceDescriptor resource = GetResourceVariable(inst->getOperand(0));
    ForceDMask(false);

    EU_MESSAGE_TARGET messageTarget = EU_MESSAGE_TARGET_DATA_PORT_DATA_CACHE_READ_ONLY;
    bool isIndirectBti =
        resource.m_surfaceType != ESURFACE_BINDLESS &&
        !resource.m_resource->IsImmediate();
    uint bti = resource.m_surfaceType == ESURFACE_BINDLESS ?
        BINDLESS_BTI :
        (isIndirectBti ? 0 : int_cast<uint>(resource.m_resource->GetImmediateValue()));

    uint messageSpecificControl = DataPortRead(
        1,
        2,
        false,
        EU_DATA_PORT_READ_MESSAGE_TYPE_SURFACE_INFO_READ,
        0,
        false,
        DATA_PORT_TARGET_CONSTANT_CACHE,
        bti);

    CVariable* pMessDesc = m_currShader->ImmToVariable(messageSpecificControl, ISA_TYPE_UD);
    if (isIndirectBti)
    {
        CVariable* immDesc = pMessDesc;
        pMessDesc = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true, CName::NONE);
        m_encoder->SetNoMask();
        m_encoder->Or(pMessDesc, resource.m_resource, immDesc);
        m_encoder->Push();
    }

    CVariable* exDesc =
        m_currShader->ImmToVariable(messageTarget, ISA_TYPE_D);
    if (resource.m_surfaceType == ESURFACE_BINDLESS)
    {
        CVariable* temp = m_currShader->GetNewVariable(resource.m_resource);
        m_encoder->Add(temp, resource.m_resource, exDesc);
        m_encoder->Push();
        exDesc = temp;
    }
    uint label = 0;
    CVariable* flag = nullptr;
    bool needLoop = ResourceLoopHeader(resource, flag, label);
    ResourceLoopSubIteration(resource, flag, label);
    CVariable* payload = m_currShader->GetNewVariable(8, ISA_TYPE_UD, EALIGN_GRF, CName::NONE);

    m_encoder->SetSimdSize(SIMDMode::SIMD8);
    m_encoder->SetNoMask();
    m_encoder->Copy(payload, m_currShader->ImmToVariable(0, ISA_TYPE_UD));
    m_encoder->Push();

    //payload destination size is 16 dwords
    CVariable* tempDest = m_currShader->GetNewVariable(16, ISA_TYPE_UD, EALIGN_GRF, CName::NONE);
    m_encoder->SetSimdSize(SIMDMode::SIMD1);
    m_encoder->SetNoMask();
    m_encoder->Send(tempDest, payload,
        messageTarget, exDesc, pMessDesc);
    m_encoder->Push();

    unsigned int writemask = 0;
    for (auto U : inst->users())
    {
        if (auto extract = dyn_cast<ExtractElementInst>(U))
        {
            if (auto index = dyn_cast<ConstantInt>(extract->getIndexOperand()))
            {
                writemask |= BIT(static_cast<uint>(index->getZExtValue()));
            }
        }
    }
    constexpr uint surfaceInfoSurfaceTypeDword = 12;
    constexpr uint surfaceInfoSurfaceFormatDword = 13;
    if (BIT(0) & writemask)
    {
        m_encoder->SetSrcSubReg(0, surfaceInfoSurfaceTypeDword); //surface type
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        m_encoder->Copy(m_destination, tempDest);
        m_encoder->Push();
    }

    if (BIT(1) & writemask)
    {
        m_encoder->SetSrcSubReg(0, surfaceInfoSurfaceFormatDword); //surface format
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        if (m_destination->IsUniform())
        {
            m_encoder->SetDstSubReg(1);
        }
        else
        {
            m_encoder->SetDstSubVar((m_encoder->GetSimdSize() == SIMDMode::SIMD16) ? 2 : 1);
        }
        m_encoder->Copy(m_destination, tempDest);
        m_encoder->Push();
    }

    ResourceLoopBackEdge(needLoop, flag, label);

    ResetVMask(false);
}

void EmitPass::emitFeedbackEnable()
{
    // if feedback is enabled we always return all 4 channels
    CVariable* flag = m_currShader->GetNewVariable(
        numLanes(m_currShader->m_State.m_dispatchSize), ISA_TYPE_BOOL, EALIGN_BYTE, CName::NONE);
    uint typeSize = CEncoder::GetCISADataTypeSize(m_destination->GetType());
    uint subvar = (numLanes(m_currShader->m_SIMDSize) * typeSize * 4) / getGRFSize();

    m_encoder->SetSrcSubVar(0, subvar);
    m_encoder->SetSrcRegion(0, 0, 1, 0);
    CVariable* newdestination = m_currShader->BitCast(m_destination, ISA_TYPE_UD);
    m_encoder->SetP(flag, newdestination);
    m_encoder->Push();

    CVariable* pred = m_currShader->ImmToVariable(0xFFFFFFFF, m_destination->GetType());
    CVariable* zero = m_currShader->ImmToVariable(0x0, m_destination->GetType());
    m_encoder->SetDstSubVar(subvar);
    m_encoder->Select(flag, m_destination, pred, zero);
    m_encoder->Push();
}

void EmitPass::emitGather4Instruction(SamplerGatherIntrinsic* inst)
{
    EOPCODE opCode = GetOpCode(inst);
    uint numOperands = inst->getNumOperands();

    //Subtract the offsets, resource and sampler sources to get
    //the number of texture coordinates, src channel select and index to texture source
    uint numSources = numOperands - 8;

    Value* textureValue = inst->getTextureValue();
    ResourceDescriptor resource = GetResourceVariable(textureValue);

    bool isEval = isUsedOnlyByEval(inst);
    ResourceDescriptor pairedResource = llvm::isa<llvm::UndefValue>(inst->getPairedTextureValue()) == false ?
        GetResourceVariable(inst->getPairedTextureValue()) :
        ResourceDescriptor();

    SamplerDescriptor sampler;
    Value* samplerValue = inst->getSamplerValue();

    sampler = GetSamplerVariable(samplerValue);

    //Check for valid number of sources from the end of the list
    for (uint i = (numSources - 1); i >= 1; i--)
    {
        CVariable* validSrc = GetSymbol(inst->getOperand(i));
        if (validSrc->IsImmediate() &&
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
    uint offsetSourceIndex = numOperands - 5;
    CVariable* offset = ComputeSampleIntOffset(inst, offsetSourceIndex);

    uint channelIndx = numOperands - 2;
    uint channel = int_cast<uint>(GetImmediateVal(inst->getOperand(channelIndx)));
    SmallVector<CVariable*, 4> payload;

    const bool derivativeSample = inst->IsDerivative();

    //create send payload for numSources
    for (uint i = 0; i < numSources; i++)
    {
        CVariable* src = GetSymbol(inst->getOperand(i));
        if (src->IsUniform())
        {
            CVariable* srcReg = m_currShader->GetNewVariable(
                numLanes(m_currShader->m_SIMDSize), ISA_TYPE_F,
                m_currShader->getGRFAlignment(),
                src->getName());
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

    CVariable* dst = m_destination;
    //When sampler output is 16 bit float, hardware doesnt pack the output in SIMD8 mode.
    //Hence the movs to handle this layout in SIMD8 mode
    bool simd8HFRet = isHalfGRFReturn(m_destination, m_SimdMode);
    if (simd8HFRet)
    {
        dst = m_currShader->GetNewVariable(
            m_destination->GetNumberElement() * 2, ISA_TYPE_HF, EALIGN_GRF, false, CName::NONE);
    }

    bool feedbackEnable = (m_destination->GetNumberElement() / numLanes(m_currShader->m_SIMDSize) == 5) ? true : false;
    uint label = 0;
    CVariable* flag = nullptr;
    bool needLoop = ResourceLoopHeader(resource, sampler, flag, label);
    ResourceLoopSubIteration(resource, sampler, flag, label);
    m_encoder->SetPredicate(flag);
    m_encoder->Gather4Inst(
        opCode,
        offset,
        resource,
        pairedResource,
        sampler,
        numSources,
        dst,
        payload,
        channel,
        feedbackEnable);
    m_encoder->Push();
    if (m_currShader->hasReadWriteImage(*(inst->getParent()->getParent())))
    {
        CVariable* tempdest = m_currShader->BitCast(m_destination, GetUnsignedIntegerType(m_destination->GetType()));
        m_encoder->Cast(m_currShader->GetNULL(), tempdest);
        m_encoder->Push();
        m_encoder->Copy(m_currShader->GetNULL(), m_currShader->GetTSC());
        m_encoder->Push();
    }
    ResourceLoopBackEdge(needLoop, flag, label);

    {
        if (simd8HFRet)
        {
            PackSIMD8HFRet(dst);
        }

        if (feedbackEnable
            )
        {
            emitFeedbackEnable();
        }
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

    for (uint i = numSources - 1; i > 0; i--)
    {
        CVariable* validSrc = GetSymbol(inst->getOperand(i));
        if (!(validSrc->IsImmediate() && validSrc->GetImmediateValue() == 0))
        {
            break;
        }
        numSources--;
    }

    // Figure out the write mask from the size of the destination we want to write
    const CShader::ExtractMaskWrapper writeMask(m_currShader, inst);
    IGC_ASSERT_MESSAGE(writeMask.hasEM() && writeMask.getEM() != 0, "Wrong write mask");

    Value* texOperand = inst->getOperand(textureArgIdx);
    ResourceDescriptor resource = GetResourceVariable(texOperand);

    uint offsetSourceIndex = numOperands - 4;
    CVariable* offset = ComputeSampleIntOffset(inst, offsetSourceIndex);

    SmallVector<CVariable*, 4> payload;

    //create send payload for numSources
    for (uint i = 0; i < numSources; i++)
    {
        CVariable* src = GetSymbol(inst->getOperand(i));
        src = BroadcastIfUniform(src);
        IGC_ASSERT(src->GetAliasOffset() % getGRFSize() == 0);
        payload.push_back(src);
    }

    CVariable* dst = m_destination;
    //When sampler output is 16 bit float, hardware doesnt pack the output in SIMD8 mode.
    //Hence the movs to handle this layout in SIMD8 mode
    bool simd8HFRet = isHalfGRFReturn(m_destination, m_SimdMode);
    if (simd8HFRet)
    {
        dst = m_currShader->GetNewVariable(
            m_destination->GetNumberElement() * 2, ISA_TYPE_HF, EALIGN_GRF, false, CName::NONE);
    }

    bool feedbackEnable = writeMask.isSet(4);
    uint label = 0;
    CVariable* flag = nullptr;
    bool needLoop = ResourceLoopHeader(resource, flag, label);
    ResourceLoopSubIteration(resource, flag, label);
    m_encoder->SetPredicate(flag);
    m_encoder->LoadMS(opCode, writeMask.getEM(), offset, resource, numSources, dst, payload, feedbackEnable);
    m_encoder->Push();
    if (m_currShader->hasReadWriteImage(*(inst->getParent()->getParent())))
    {
        CVariable* tempdest = m_currShader->BitCast(m_destination, GetUnsignedIntegerType(m_destination->GetType()));
        m_encoder->Cast(m_currShader->GetNULL(), tempdest);
        m_encoder->Push();
        m_encoder->Copy(m_currShader->GetNULL(), m_currShader->GetTSC());
        m_encoder->Push();
    }
    ResourceLoopBackEdge(needLoop, flag, label);

    if (simd8HFRet)
    {
        PackSIMD8HFRet(dst);
    }

    if (feedbackEnable
        )
    {
        emitFeedbackEnable();
    }
}


void EmitPass::emitAluNoModifier(llvm::GenIntrinsicInst* inst)
{
    CVariable* pSrc0 = GetSymbol(inst->getOperand(0));
    CVariable* pSrc1;
    CVariable* pSrc2;
    CVariable* dst;
    CVariable* srcUD;

    switch (inst->getIntrinsicID())
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
        srcUD = m_currShader->BitCast(pSrc0, GetUnsignedType(pSrc0->GetType()));
        m_encoder->Fbl(dst, srcUD);
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
    switch (inst->getIntrinsicID())
    {
    case GenISAIntrinsic::GenISA_OUTPUT:
        emitOutput(inst);
        break;
        break;
    case GenISAIntrinsic::GenISA_simdLaneId:
        emitSimdLaneId(inst);
        break;
    case GenISAIntrinsic::GenISA_simdLaneIdReplicate:
        emitSimdLaneIdReplicate(inst);
        break;
    case GenISAIntrinsic::GenISA_simdSize:
        emitSimdSize(inst);
        break;
    case GenISAIntrinsic::GenISA_simdShuffleDown:
        emitSimdShuffleDown(inst);
        break;
    case GenISAIntrinsic::GenISA_simdShuffleXor:
        emitSimdShuffleXor(inst);
        break;
    case GenISAIntrinsic::GenISA_simdBlockRead:
        emitSimdBlockRead(inst);
        break;
    case GenISAIntrinsic::GenISA_simdBlockReadBindless:
        emitSimdBlockRead(inst, inst->getOperand(1));
        break;
    case GenISAIntrinsic::GenISA_simdBlockWrite:
        emitSimdBlockWrite(inst);
        break;
    case GenISAIntrinsic::GenISA_simdBlockWriteBindless:
        emitSimdBlockWrite(inst, inst->getOperand(2));
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
    case GenISAIntrinsic::GenISA_frc:
        emitFrc(inst);
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
        emitVideoAnalyticSLM(inst, 1);
        break;
    case GenISAIntrinsic::GenISA_vaMinMaxFilter:
        emitVideoAnalyticSLM(inst, 8);
        break;
    case GenISAIntrinsic::GenISA_vaConvolve:
    case GenISAIntrinsic::GenISA_vaCentroid:
        emitVideoAnalyticSLM(inst, 4);
        break;
    case GenISAIntrinsic::GenISA_vaConvolveGRF_16x1:
    case GenISAIntrinsic::GenISA_vaConvolveGRF_16x4:
        emitVideoAnalyticGRF(inst, 1);
        break;
    case GenISAIntrinsic::GenISA_vaBoolSum:
    case GenISAIntrinsic::GenISA_vaBoolCentroid:
        emitVideoAnalyticSLM(inst, 2);
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
    case GenISAIntrinsic::GenISA_ldptr:
    case GenISAIntrinsic::GenISA_ldlptr:
        emitLdInstruction(inst);
        break;
    case GenISAIntrinsic::GenISA_sampleptr:
    case GenISAIntrinsic::GenISA_sampleBptr:
    case GenISAIntrinsic::GenISA_sampleCptr:
    case GenISAIntrinsic::GenISA_sampleDptr:
    case GenISAIntrinsic::GenISA_sampleDCptr:
    case GenISAIntrinsic::GenISA_sampleLptr:
    case GenISAIntrinsic::GenISA_sampleLCptr:
    case GenISAIntrinsic::GenISA_sampleBCptr:
    case GenISAIntrinsic::GenISA_lodptr:
    case GenISAIntrinsic::GenISA_sampleKillPix:
    case GenISAIntrinsic::GenISA_sampleMlodptr:
    case GenISAIntrinsic::GenISA_sampleCMlodptr:
    case GenISAIntrinsic::GenISA_sampleBCMlodptr:
    case GenISAIntrinsic::GenISA_sampleDCMlodptr:
    case GenISAIntrinsic::GenISA_samplePOptr:
    case GenISAIntrinsic::GenISA_samplePOBptr:
    case GenISAIntrinsic::GenISA_samplePOLptr:
    case GenISAIntrinsic::GenISA_samplePOCptr:
    case GenISAIntrinsic::GenISA_samplePODptr:
    case GenISAIntrinsic::GenISA_samplePOLCptr:
        emitSampleInstruction(cast<SampleIntrinsic>(inst));
        break;
    case GenISAIntrinsic::GenISA_discard:
        emitDiscard(inst);
        break;
    case GenISAIntrinsic::GenISA_resinfoptr:
    case GenISAIntrinsic::GenISA_sampleinfoptr:
        emitInfoInstruction(cast<InfoIntrinsic>(inst));
        break;
    case GenISAIntrinsic::GenISA_gather4IPOptr:
    case GenISAIntrinsic::GenISA_gather4BPOptr:
    case GenISAIntrinsic::GenISA_gather4LPOptr:
    case GenISAIntrinsic::GenISA_gather4ICPOptr:
    case GenISAIntrinsic::GenISA_gather4LCPOptr:
        IGC_ASSERT_MESSAGE(false, "These Gather4 PO messages are expected to be replaced with their non-PO counterparts.");
        break;
    case GenISAIntrinsic::GenISA_gather4ptr:
    case GenISAIntrinsic::GenISA_gather4Cptr:
    case GenISAIntrinsic::GenISA_gather4POptr:
    case GenISAIntrinsic::GenISA_gather4POCptr:
    case GenISAIntrinsic::GenISA_gather4Iptr:
    case GenISAIntrinsic::GenISA_gather4Bptr:
    case GenISAIntrinsic::GenISA_gather4Lptr:
    case GenISAIntrinsic::GenISA_gather4ICptr:
    case GenISAIntrinsic::GenISA_gather4LCptr:
    case GenISAIntrinsic::GenISA_gather4POPackedptr:
    case GenISAIntrinsic::GenISA_gather4POPackedLptr:
    case GenISAIntrinsic::GenISA_gather4POPackedBptr:
    case GenISAIntrinsic::GenISA_gather4POPackedIptr:
    case GenISAIntrinsic::GenISA_gather4POPackedCptr:
    case GenISAIntrinsic::GenISA_gather4POPackedICptr:
    case GenISAIntrinsic::GenISA_gather4POPackedLCptr:
        emitGather4Instruction(cast<SamplerGatherIntrinsic>(inst));
        break;
    case GenISAIntrinsic::GenISA_ldmcsptr:
    case GenISAIntrinsic::GenISA_ldmsptr:
    case GenISAIntrinsic::GenISA_ldmsptr16bit:
        emitLdmsInstruction(inst);
        break;
    case GenISAIntrinsic::GenISA_typedread:
        if (shouldGenerateLSC(inst, true) &&
            IGC_IS_FLAG_DISABLED(DisableLSCForTypedUAV))
        {
            emitLSCTypedRead(inst);
            break;
        }
        emitTypedRead(inst);
        break;
    case GenISAIntrinsic::GenISA_typedwrite:
        if (shouldGenerateLSC(inst, true) &&
            IGC_IS_FLAG_DISABLED(DisableLSCForTypedUAV))
        {
            emitLSCTypedWrite(inst);
            break;
        }
        emitTypedWrite(inst);
        break;
    case GenISAIntrinsic::GenISA_threadgroupbarrier:
    case GenISAIntrinsic::GenISA_threadgroupbarrier_signal:
    case GenISAIntrinsic::GenISA_threadgroupbarrier_wait:
        emitThreadGroupBarrier(inst);
        break;
    case GenISAIntrinsic::GenISA_memoryfence:
        emitMemoryFence(inst);
        break;
    case GenISAIntrinsic::GenISA_flushsampler:
        emitFlushSamplerCache();
        break;
    case GenISAIntrinsic::GenISA_typedmemoryfence:
        emitTypedMemoryFence(inst);
        break;
    case GenISAIntrinsic::GenISA_systemmemoryfence:
        emitSystemMemoryFence(inst);
        break;
    case GenISAIntrinsic::GenISA_urbfence:
        emitUrbFence();
        break;
    case GenISAIntrinsic::GenISA_HDCuncompressedwrite:
        emitHDCuncompressedwrite(inst);
        break;
    case GenISAIntrinsic::GenISA_threadgroupnamedbarriers_signal:
        emitThreadGroupNamedBarriersSignal(inst);
        break;
    case GenISAIntrinsic::GenISA_threadgroupnamedbarriers_wait:
        emitThreadGroupNamedBarriersWait(inst);
        break;
    case GenISAIntrinsic::GenISA_assume_uniform:
        // nothing to do
        break;
    case GenISAIntrinsic::GenISA_intatomicraw:
    case GenISAIntrinsic::GenISA_floatatomicraw:
    case GenISAIntrinsic::GenISA_intatomicrawA64:
    case GenISAIntrinsic::GenISA_floatatomicrawA64:
    case GenISAIntrinsic::GenISA_icmpxchgatomicraw:
    case GenISAIntrinsic::GenISA_fcmpxchgatomicraw:
    case GenISAIntrinsic::GenISA_icmpxchgatomicrawA64:
    case GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64:
        emitAtomicRaw(inst, inst->getOperand(1));
        break;
    case GenISAIntrinsic::GenISA_intatomictyped:
    case GenISAIntrinsic::GenISA_icmpxchgatomictyped:
    case GenISAIntrinsic::GenISA_floatatomictyped:
    case GenISAIntrinsic::GenISA_fcmpxchgatomictyped:
        if (shouldGenerateLSC(inst))
        {
            emitLSCAtomicTyped(inst);
            break;
        }
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
    case GenISAIntrinsic::GenISA_f32tof16_rtz:
        emitf32tof16_rtz(inst);
        break;
    case GenISAIntrinsic::GenISA_ftoi_rtn:
    case GenISAIntrinsic::GenISA_ftoi_rtp:
    case GenISAIntrinsic::GenISA_ftoi_rte:
    case GenISAIntrinsic::GenISA_ftoui_rtn:
    case GenISAIntrinsic::GenISA_ftoui_rtp:
    case GenISAIntrinsic::GenISA_ftoui_rte:
        emitftoi(inst);
        break;
    case GenISAIntrinsic::GenISA_itof_rtn:
    case GenISAIntrinsic::GenISA_itof_rtp:
    case GenISAIntrinsic::GenISA_itof_rtz:
    case GenISAIntrinsic::GenISA_uitof_rtn:
    case GenISAIntrinsic::GenISA_uitof_rtp:
    case GenISAIntrinsic::GenISA_uitof_rtz:
    case GenISAIntrinsic::GenISA_ftof_rte:
    case GenISAIntrinsic::GenISA_ftof_rtn:
    case GenISAIntrinsic::GenISA_ftof_rtp:
    case GenISAIntrinsic::GenISA_ftof_rtz:
        emitfitof(inst);
        break;
    case GenISAIntrinsic::GenISA_ftobf:
    case GenISAIntrinsic::GenISA_bftof:
    case GenISAIntrinsic::GenISA_2fto2bf:
    case GenISAIntrinsic::GenISA_hftobf8:
    case GenISAIntrinsic::GenISA_bf8tohf:
    case GenISAIntrinsic::GenISA_ftotf32:
        emitfcvt(inst);
        break;
    case GenISAIntrinsic::GenISA_srnd_ftohf:
    case GenISAIntrinsic::GenISA_srnd_hftobf8:
        emitSrnd(inst);
        break;
    case GenISAIntrinsic::GenISA_uavSerializeAll:
    case GenISAIntrinsic::GenISA_uavSerializeOnResID:
        emitUAVSerialize();
        break;
    case GenISAIntrinsic::GenISA_globalSync:
        emitMemoryFence();
        break;
    case GenISAIntrinsic::GenISA_ldrawvector_indexed:
    case GenISAIntrinsic::GenISA_ldraw_indexed:
        emitLoadRawIndexed(
            cast<LdRawIntrinsic>(inst),
            cast<LdRawIntrinsic>(inst)->getOffsetValue(),
            nullptr);
        break;
    case GenISAIntrinsic::GenISA_storerawvector_indexed:
    case GenISAIntrinsic::GenISA_storeraw_indexed:
        emitStoreRawIndexed(
            cast<StoreRawIntrinsic>(inst),
            cast<StoreRawIntrinsic>(inst)->getOffsetValue(),
            nullptr);
        break;
    case GenISAIntrinsic::GenISA_GetBufferPtr:
        emitGetBufferPtr(inst);
        break;
    case GenISAIntrinsic::GenISA_readsurfacetypeandformat:
        if (m_currShader->m_Platform->supportsHDCLegacyDCROMessage())
            emitSurfaceInfo(inst);
        else
            emitInfoInstruction(cast<InfoIntrinsic>(inst));
        break;
    case GenISAIntrinsic::GenISA_mov_identity:
    {
      // Use Or instead of a Copy, as VISA will remove redundant movs.
      auto Var = GetSymbol(inst->getOperand(0));
      CVariable* Zero = m_currShader->ImmToVariable(0, ISA_TYPE_UD);
      m_encoder->Or(Var, Var, Zero);
      m_encoder->Push();
      break;
    }
    case GenISAIntrinsic::GenISA_source_value:
    {
        m_encoder->Copy(m_currShader->GetNULL(), GetSymbol(inst->getOperand(0)));
        m_encoder->Push();
        break;
    }
    case GenISAIntrinsic::GenISA_movcr:
    {
        m_encoder->SetSrcSubReg(0, static_cast<uint16_t>(GetImmediateVal(inst->getOperand(0))));
        m_encoder->Copy(m_destination, m_currShader->GetCR0());
        m_encoder->Push();
        break;
    }
    case GenISAIntrinsic::GenISA_software_exception:
    {
        // Set Software Exception Control
        CVariable *Mask = m_currShader->ImmToVariable(
            1 << 29,
            ISA_TYPE_UD);

        m_encoder->SetSrcSubReg(0, 1);
        m_encoder->SetDstSubReg(1);
        m_encoder->Or(m_currShader->GetCR0(), m_currShader->GetCR0(), Mask);
        m_encoder->Push();
        break;
    }
    case GenISAIntrinsic::GenISA_thread_exit:
    {
        m_currShader->EOTGateway();
        break;
    }
    case GenISAIntrinsic::GenISA_getStackPointer: {
        IGC_ASSERT(m_destination->GetType() == ISA_TYPE_Q);
        IGC_ASSERT(m_currShader->GetSP() &&
                   (m_currShader->GetSP()->GetType() == ISA_TYPE_UQ ||
                    m_currShader->GetSP()->GetType() == ISA_TYPE_UD));
        m_encoder->Cast(m_destination, m_currShader->GetSP());
        m_encoder->Push();
        break;
    }
    case GenISAIntrinsic::GenISA_getStackSizePerThread: {
        CVariable *pSize = getStackSizePerThread(inst->getFunction());
        m_encoder->Copy(m_destination, pSize);
        m_encoder->Push();
        break;
    }
    case GenISAIntrinsic::GenISA_hw_thread_id:
    case GenISAIntrinsic::GenISA_hw_thread_id_alloca:
    {
        m_encoder->Copy(m_destination, m_currShader->GetHWTID());
        m_encoder->Push();
        break;
    }
    case GenISAIntrinsic::GenISA_slice_id:
    {
        if (m_currShader->m_Platform->GetPlatformFamily() == IGFX_GEN8_CORE ||
            m_currShader->m_Platform->GetPlatformFamily() == IGFX_GEN9_CORE)
            emitStateRegID(14, 15);
        else if (m_currShader->m_Platform->GetPlatformFamily() == IGFX_GEN12_CORE ||
            m_currShader->m_Platform->GetPlatformFamily() == IGFX_XE_HP_CORE)
            emitStateRegID(11, 13);
        else if (m_currShader->m_Platform->GetPlatformFamily() == IGFX_XE_HPG_CORE)
            emitStateRegID(11, 13);
        else if (m_currShader->m_Platform->GetPlatformFamily() == IGFX_XE_HPC_CORE)
            emitStateRegID(12, 14);
        else if (m_currShader->m_Platform->GetPlatformFamily() == IGFX_XE2_HPG_CORE)
            emitStateRegID(11, 15);
        else
            emitStateRegID(12, 14);
        break;
    }
    case GenISAIntrinsic::GenISA_subslice_id:
    {
        if (m_currShader->m_Platform->GetPlatformFamily() == IGFX_GEN8_CORE ||
            m_currShader->m_Platform->GetPlatformFamily() == IGFX_GEN9_CORE)
            emitStateRegID(12, 13);
        else if (m_currShader->m_Platform->GetPlatformFamily() ==
                 IGFX_XE_HPC_CORE) {
          emitStateRegID(9, 11);
        }
        else if (m_currShader->m_Platform->GetPlatformFamily() == IGFX_XE2_HPG_CORE)
            emitStateRegID(8, 9);
        else
            emitStateRegID(8, 8);
        break;
    }
    case GenISAIntrinsic::GenISA_logical_subslice_id:
    {
        if (m_currShader->m_Platform->hasLogicalSSID())
        {
            uint32_t and_imm = BITMASK_RANGE(0, 7);
            m_encoder->SetSrcSubReg(0, 0);
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->And(m_destination, m_currShader->GetMSG0(), m_currShader->ImmToVariable(and_imm, ISA_TYPE_UD));
            m_encoder->Push();
        }
        else
        {
            m_pCtx->EmitError("logical SSID is not supported in this platform!", inst);
            IGC_ASSERT_MESSAGE(0, "GenISA_logical_subslice_id not supported in this platform!");
        }
        break;
    }
    case GenISAIntrinsic::GenISA_dual_subslice_id:
    {
        if (m_currShader->m_Platform->GetPlatformFamily() == IGFX_GEN11_CORE ||
            m_currShader->m_Platform->GetPlatformFamily() == IGFX_GEN11LP_CORE ||
            m_currShader->m_Platform->GetPlatformFamily() == IGFX_GEN12LP_CORE)
            emitStateRegID(9, 11);
        else if (m_currShader->m_Platform->GetPlatformFamily() == IGFX_GEN12_CORE ||
                 m_currShader->m_Platform->GetPlatformFamily() == IGFX_XE_HP_CORE)
            emitStateRegID(9, 10);
        else if (m_currShader->m_Platform->GetPlatformFamily() == IGFX_XE_HPC_CORE) {
            emitStateRegID(9, 11);
        }
        else if (m_currShader->m_Platform->GetPlatformFamily() == IGFX_XE_HPG_CORE)
            emitStateRegID(9, 10);
        else
            IGC_ASSERT_MESSAGE(0, "No support for Dual Subslice in current platform");
        break;
    }
    case GenISAIntrinsic::GenISA_eu_id:
    {
        if (m_currShader->m_Platform->GetPlatformFamily() == IGFX_GEN8_CORE ||
            m_currShader->m_Platform->GetPlatformFamily() == IGFX_GEN9_CORE)
            emitStateRegID(8, 11);
        else if (m_currShader->m_Platform->GetPlatformFamily() ==
                 IGFX_XE_HPC_CORE) {
          emitStateRegID(4, 8);
          m_currShader->RemoveBitRange(m_destination, 2, 2);
        }
        else if (m_currShader->m_Platform->GetPlatformFamily() == IGFX_XE2_HPG_CORE)
            emitStateRegID(4, 6);
        else
            emitStateRegID(4, 7);
        break;
    }
    case GenISAIntrinsic::GenISA_getSR0:
    {
        m_encoder->SetSrcSubReg(0, static_cast<uint16_t>(GetImmediateVal(inst->getOperand(0))));
        m_encoder->Copy(m_destination, m_currShader->GetSR0());
        m_encoder->Push();
        break;
    }
    case GenISAIntrinsic::GenISA_getSR0_0:
    {
        m_encoder->SetSrcSubReg(0, 0);
        m_encoder->Copy(m_destination, m_currShader->GetSR0());
        m_encoder->Push();
        break;
    }
    case GenISAIntrinsic::GenISA_eu_thread_id:
        emitStateRegID(0, 2);
        break;
    case GenISAIntrinsic::GenISA_eu_thread_pause:
        emitThreadPause(inst);
        break;
    case GenISAIntrinsic::GenISA_pair_to_ptr:
        emitPairToPtr(inst);
        break;
    case GenISAIntrinsic::GenISA_StackAlloca:
        emitStackAlloca(inst);
        break;
    case GenISAIntrinsic::GenISA_VLAStackAlloca:
        emitVLAStackAlloca(inst);
        break;
    case GenISAIntrinsic::GenISA_WaveBallot:
        emitWaveBallot(inst);
        break;
    case GenISAIntrinsic::GenISA_WaveInverseBallot:
        emitWaveInverseBallot(inst);
        break;
    case GenISAIntrinsic::GenISA_WaveClusteredBallot:
        emitWaveClusteredBallot(inst);
        break;
    case GenISAIntrinsic::GenISA_WaveShuffleIndex:
    case GenISAIntrinsic::GenISA_WaveBroadcast:
        emitSimdShuffle(inst);
        break;
    case GenISAIntrinsic::GenISA_WaveClusteredBroadcast:
        emitSimdClusteredBroadcast(inst);
        break;
    case GenISAIntrinsic::GenISA_WavePrefix:
        emitWavePrefix(cast<WavePrefixIntrinsic>(inst));
        break;
    case GenISAIntrinsic::GenISA_QuadPrefix:
        emitQuadPrefix(cast<QuadPrefixIntrinsic>(inst));
        break;
    case GenISAIntrinsic::GenISA_WaveClusteredPrefix:
        emitWaveClusteredPrefix(inst);
        break;
    case GenISAIntrinsic::GenISA_WaveAll:
        emitWaveAll(inst);
        break;
    case GenISAIntrinsic::GenISA_WaveInterleave:
        emitWaveInterleave(inst);
        break;
    case GenISAIntrinsic::GenISA_WaveClustered:
        emitWaveClustered(inst);
        break;
    case GenISAIntrinsic::GenISA_WaveClusteredInterleave:
        emitWaveClusteredInterleave(inst);
        break;
    case GenISAIntrinsic::GenISA_dp4a_ss:
    case GenISAIntrinsic::GenISA_dp4a_uu:
    case GenISAIntrinsic::GenISA_dp4a_su:
    case GenISAIntrinsic::GenISA_dp4a_us:
        emitDP4A(inst);
        break;
    case GenISAIntrinsic::GenISA_evaluateSampler:
        // nothing to do
        break;
    case GenISAIntrinsic::GenISA_HDCCCSFastClear:
        // nothing to do
        break;
    case GenISAIntrinsic::GenISA_wavebarrier:
    {
        m_encoder->Fence(false,
            false,
            false,
            false,
            false,
            false,
            false,
            true);
        m_encoder->Push();
        break;
    }
    case GenISAIntrinsic::GenISA_mul_rtz:
    case GenISAIntrinsic::GenISA_mul_rte:
    case GenISAIntrinsic::GenISA_mul_rtp:
    case GenISAIntrinsic::GenISA_mul_rtn:
    case GenISAIntrinsic::GenISA_add_rtz:
    case GenISAIntrinsic::GenISA_add_rte:
    case GenISAIntrinsic::GenISA_add_rtp:
    case GenISAIntrinsic::GenISA_add_rtn:
    case GenISAIntrinsic::GenISA_fma_rtz:
    case GenISAIntrinsic::GenISA_fma_rte:
    case GenISAIntrinsic::GenISA_fma_rtp:
    case GenISAIntrinsic::GenISA_fma_rtn:
        emitFPOWithNonDefaultRoundingMode(inst);
        break;
    case GenISAIntrinsic::GenISA_CatchAllDebugLine:
        emitDebugPlaceholder(inst);
        break;
    case GenISAIntrinsic::GenISA_getR0:
    case GenISAIntrinsic::GenISA_getPayloadHeader:
    case GenISAIntrinsic::GenISA_getWorkDim:
    case GenISAIntrinsic::GenISA_getNumWorkGroups:
    case GenISAIntrinsic::GenISA_getLocalSize:
    case GenISAIntrinsic::GenISA_getGlobalSize:
    case GenISAIntrinsic::GenISA_getEnqueuedLocalSize:
    case GenISAIntrinsic::GenISA_getLocalID_X:
    case GenISAIntrinsic::GenISA_getLocalID_Y:
    case GenISAIntrinsic::GenISA_getLocalID_Z:
    case GenISAIntrinsic::GenISA_getPrivateBase:
    case GenISAIntrinsic::GenISA_getPrintfBuffer:
    case GenISAIntrinsic::GenISA_getStageInGridOrigin:
    case GenISAIntrinsic::GenISA_getStageInGridSize:
    case GenISAIntrinsic::GenISA_getSyncBuffer:
    case GenISAIntrinsic::GenISA_getRtGlobalBufferPtr:
    case GenISAIntrinsic::GenISA_getAssertBufferPtr:
        emitImplicitArgIntrinsic(inst);
        break;
    case GenISAIntrinsic::GenISA_RayQueryCheck:
        emitRayQueryCheckRelease(inst, true, false);
        break;
    case GenISAIntrinsic::GenISA_RayQueryRelease:
        emitRayQueryCheckRelease(inst, false, true);
        break;
    case GenISAIntrinsic::GenISA_PreemptionDisable:
        emitPreemptionDisable(cast<PreemptionDisableIntrinsic>(inst));
        break;
    case GenISAIntrinsic::GenISA_PreemptionEnable:
        emitPreemptionEnable(cast<PreemptionEnableIntrinsic>(inst));
        break;
    case GenISAIntrinsic::GenISA_AsyncStackID:
        emitAsyncStackID(inst);
        break;
    case GenISAIntrinsic::GenISA_GlobalBufferPointer:
        emitGlobalBufferPtr(inst);
        break;
    case GenISAIntrinsic::GenISA_LocalBufferPointer:
        emitLocalBufferPtr(inst);
        break;
    case GenISAIntrinsic::GenISA_TraceRaySync:
        emitTraceRay(cast<TraceRayIntrinsic>(inst), true);
        break;
    case GenISAIntrinsic::GenISA_TraceRayAsync:
        emitTraceRay(cast<TraceRayIntrinsic>(inst), false);
        break;
    case GenISAIntrinsic::GenISA_ReadTraceRaySync:
        emitReadTraceRaySync(inst);
        break;
    case GenISAIntrinsic::GenISA_BindlessThreadDispatch:
        emitBindlessThreadDispatch(cast<BTDIntrinsic>(inst));
        break;
    case GenISAIntrinsic::GenISA_StackIDRelease:
        emitStackIDRelease(cast<StackIDReleaseIntrinsic>(inst));
        break;
    case GenISAIntrinsic::GenISA_GetShaderRecordPtr:
        emitGetShaderRecordPtr(cast<GetShaderRecordPtrIntrinsic>(inst));
        break;
    case GenISAIntrinsic::GenISA_InlinedData:
        emitInlinedDataValue(inst);
        break;
    case GenISAIntrinsic::GenISA_LSCStore:
    case GenISAIntrinsic::GenISA_LSCStoreBlock:
    case GenISAIntrinsic::GenISA_LSCLoad:
    case GenISAIntrinsic::GenISA_LSCLoadWithSideEffects:
    case GenISAIntrinsic::GenISA_LSCLoadBlock:
    case GenISAIntrinsic::GenISA_LSCStoreCmask:
    case GenISAIntrinsic::GenISA_LSCLoadCmask:
    case GenISAIntrinsic::GenISA_LSCLoadStatus:
    case GenISAIntrinsic::GenISA_LSCPrefetch:
    case GenISAIntrinsic::GenISA_LSCSimdBlockPrefetch:
    case GenISAIntrinsic::GenISA_LSCFence:
    case GenISAIntrinsic::GenISA_LSCAtomicFP64:
    case GenISAIntrinsic::GenISA_LSCAtomicFP32:
    case GenISAIntrinsic::GenISA_LSCAtomicInts:
    case GenISAIntrinsic::GenISA_LSC2DBlockRead:
    case GenISAIntrinsic::GenISA_LSC2DBlockPrefetch:
    case GenISAIntrinsic::GenISA_LSC2DBlockWrite:
    case GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload:
    case GenISAIntrinsic::GenISA_LSC2DBlockPrefetchAddrPayload:
    case GenISAIntrinsic::GenISA_LSC2DBlockWriteAddrPayload:
    case GenISAIntrinsic::GenISA_LSC2DBlockCreateAddrPayload:
    case GenISAIntrinsic::GenISA_LSC2DBlockCopyAddrPayload:
    case GenISAIntrinsic::GenISA_LSC2DBlockSetAddrPayloadField:
        emitLSCIntrinsic(inst);
        break;
    case GenISAIntrinsic::GenISA_dummyInst:
        emitDummyInst(inst);
        break;
    case GenISAIntrinsic::GenISA_dummyInstID:
        break;  // pseudo instruction, do nothing
    case GenISAIntrinsic::GenISA_launder:
        emitLaunder(inst);
        break;
    case GenISAIntrinsic::GenISA_vectorUniform:
        break;  // pseudo instruction, do nothing
    case GenISAIntrinsic::GenISA_staticConstantPatchValue:
        emitStaticConstantPatchValue(cast<StaticConstantPatchIntrinsic>(inst));
        break;
    case GenISAIntrinsic::GenISA_SetStackCallsBaseAddress:
        emitSetStackCallsBaseAddress(inst);
        break;
    case GenISAIntrinsic::GenISA_SetImplicitBufferPtr:
        emitStoreImplBufferPtr(inst);
        break;
    case GenISAIntrinsic::GenISA_SaveInReservedArgSpace:
        emitSaveInReservedArgSpace(cast<SaveInReservedArgSpaceIntrinsic>(inst));
        break;
    case GenISAIntrinsic::GenISA_ReadFromReservedArgSpace:
        emitReadFromReservedArgSpace(cast<ReadFromReservedArgSpaceIntrinsic>(inst));
        break;
    case GenISAIntrinsic::GenISA_SetLocalIdBufferPtr:
        emitStoreLocalIdBufferPtr(inst);
        break;
    case GenISAIntrinsic::GenISA_GetImplicitBufferPtr:
        emitLoadImplBufferPtr(inst);
        break;
    case GenISAIntrinsic::GenISA_GetLocalIdBufferPtr:
        emitLoadLocalIdBufferPtr(inst);
        break;
    case GenISAIntrinsic::GenISA_SetGlobalBufferArg:
        emitStoreGlobalBufferArg(inst);
        break;
    case GenISAIntrinsic::GenISA_GetGlobalBufferArg:
        emitLoadGlobalBufferArg(inst);
        break;
    case GenISAIntrinsic::GenISA_bitcastfromstruct:
      emitBitcastfromstruct(inst);
      break;
    case GenISAIntrinsic::GenISA_bitcasttostruct:
        emitBitcasttostruct(inst);
      break;
    default:
        // we assume that some of gen-intrinsic should always be pattern-matched away,
        // therefore we do not handle them in visa-emission.
        // let us know if you see a case that hits this assertion by those intrinsics
        inst->print(IGC::Debug::ods());
        IGC_ASSERT_MESSAGE(0, "unknown intrinsic");
        break;
    }
}

void EmitPass::EmitIntrinsicMessage(llvm::IntrinsicInst* inst)
{
    switch (inst->getIntrinsicID())
    {
    case Intrinsic::lifetime_start:
    case Intrinsic::lifetime_end:
    case Intrinsic::fabs:
    case Intrinsic::trap:
    case Intrinsic::experimental_noalias_scope_decl:
        // do nothing
        break;
    case Intrinsic::stacksave:
        // If stack is not initialized (no SP), we can assume there's no VLA.
        // We can ignore llvm.stacksave and llvm.stackrestore intrinsics
        if (m_currShader->hasSP())
            emitLLVMStackSave(inst);
        break;

    case Intrinsic::stackrestore:
        // If stack is not initialized (no SP), we can assume there's no VLA.
        // We can ignore llvm.stacksave and llvm.stackrestore intrinsics
        if (m_currShader->hasSP())
            emitLLVMStackRestore(inst);
        break;

    case Intrinsic::bswap:
        emitLLVMbswap(inst);
        break;

    case Intrinsic::sqrt:
        emitSqrt(inst);
        break;

    case Intrinsic::umax:
        emitUmax(inst);
        break;

    case Intrinsic::smax:
        emitSmax(inst);
        break;

    case Intrinsic::umin:
        emitUmin(inst);
        break;

    case Intrinsic::smin:
        emitSmin(inst);
        break;

    default:
        inst->print(IGC::Debug::ods());
        IGC_ASSERT_MESSAGE(0, "unknown intrinsic");
        break;
    }
}

bool EmitPass::validateInlineAsmConstraints(llvm::CallInst* inst, SmallVector<StringRef, 8> & constraints)
{
    IGC_ASSERT(inst->isInlineAsm());
    InlineAsm* IA = cast<InlineAsm>(IGCLLVM::getCalledValue(inst));
    StringRef constraintStr(IA->getConstraintString());
    if (constraintStr.empty()) return true;

    //lambda for checking constraint types
    auto CheckConstraintTypes = [this](StringRef str, CVariable* cv = nullptr)
    {
        unsigned matchVal;
        if (str.equals("=rw"))
        {
            return true;
        }
        else if (str.equals("rw"))
        {
            return true;
        }
        else if (str.getAsInteger(10, matchVal) == 0)
        {
            // Also allows matching input reg to output reg
            return true;
        }
        else if (str.equals("i"))
        {
            return cv && cv->IsImmediate();
        }
        else if (str.equals("rw.u"))
        {
            return cv && cv->IsUniform();
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "Unsupported constraint type!");
            return false;
        }
    };

    // Get a list of constraint tokens
    constraintStr.split(constraints, ',');

    bool success = true;

    unsigned index = 0;

    // Check the output constraint tokens
    for (; index < constraints.size(); index++)
    {
        StringRef &str = constraints[index];
        if (str.startswith("="))
        {
            success &= CheckConstraintTypes(str);
        }
        else
        {
            break;
        }
    }
    if (success)
    {
        // Check the input constraint tokens
        for (unsigned i = 0; i < IGCLLVM::getNumArgOperands(inst); i++, index++)
        {
            CVariable* cv = GetSymbol(inst->getArgOperand(i));
            success &= CheckConstraintTypes(constraints[index], cv);
        }
    }
    return success;
}

// Parse the inlined asm string to generate VISA operands
// Example: "mul (M1, 16) $0(0, 0)<1> $1(0, 0)<1;1,0> $2(0, 0)<1;1,0>", "=r,r,r"(float %6, float %7)
void EmitPass::EmitInlineAsm(llvm::CallInst* inst)
{
    std::stringstream& str = m_encoder->GetVISABuilder()->GetAsmTextStream();
    InlineAsm* IA = cast<InlineAsm>(IGCLLVM::getCalledValue(inst));
    string asmStr = IA->getAsmString();
    smallvector<CVariable*, 8> opnds;
    SmallVector<StringRef, 8> constraints;
    DenseMap<CVariable*, Instruction*> DstOpndMap;

    if (asmStr.empty())
        return;

    if (!validateInlineAsmConstraints(inst, constraints))
    {
        IGC_ASSERT_MESSAGE(0, "Constraints for inline assembly cannot be validated");
        return;
    }

    if (inst->getType()->isStructTy())
    {
        // Handle multiple outputs
        unsigned numOutputs = inst->getType()->getStructNumElements();
        std::vector<CVariable*> outputs(numOutputs);
        for (auto var : outputs) var = nullptr;

        for (auto user : inst->users())
        {
            ExtractValueInst* ex = dyn_cast<ExtractValueInst>(user);
            IGC_ASSERT_MESSAGE(nullptr != ex, "Invalid user of inline asm call");
            unsigned id = *ex->idx_begin();
            IGC_ASSERT(id < numOutputs);
            IGC_ASSERT(outputs[id] == nullptr);
            outputs[id] = GetSymbol(ex);
            DstOpndMap[outputs[id]] = ex;
        }
        for (auto var : outputs) opnds.push_back(var);
    }
    else if (m_destination)
    {
        opnds.push_back(m_destination);
        DstOpndMap[m_destination] = inst;
    }
    for (unsigned i = 0; i < IGCLLVM::getNumArgOperands(inst); i++)
    {
        CVariable* cv = GetSymbol(inst->getArgOperand(i));
        opnds.push_back(cv);
    }

    IGC_ASSERT(opnds.size() == constraints.size());

    // Check for read/write registers
    if (!inst->getType()->isVoidTy())
    {
        for (unsigned i = 0; i < constraints.size(); i++)
        {
            unsigned destID;
            if (constraints[i].getAsInteger(10, destID) == 0)
            {
                // If input is linked to output reg, move the input value into the output
                CVariable* cv = opnds[i];
                CVariable* dest = opnds[destID];
                if (cv && dest && cv != dest)
                {
                    auto iter = DstOpndMap.find(dest);
                    IGC_ASSERT(iter != DstOpndMap.end());
                    Instruction* I = iter->second;
                    if (I->getType()->isVectorTy())
                    {
                        emitVectorCopy(dest, cv, int_cast<unsigned>(dyn_cast<IGCLLVM::FixedVectorType>(I->getType())->getNumElements()));
                    }
                    else
                    {
                        m_encoder->Copy(dest, cv);
                        m_encoder->Push();
                    }
                }
            }
        }
    }

    for (unsigned i = 0; i < opnds.size(); i++)
    {
        CVariable* opVar = opnds[i];
        StringRef constraint = constraints[i];

        if (!opVar) continue;

        // All uniform variables must be broadcasted if 'rw' constraint was specified
        if (opVar->IsUniform() && constraint.equals("rw"))
        {
            opnds[i] = BroadcastIfUniform(opVar);
        }
        // Special handling if LLVM replaces a variable with an immediate, we need to insert an extra move
        else if (opVar->IsImmediate() && !constraint.equals("i"))
        {
            CVariable* tempMov = m_currShader->GetNewVariable(
                1, opVar->GetType(), EALIGN_GRF, true, opVar->getName());
            m_encoder->Copy(tempMov, opVar);
            m_encoder->Push();
            opnds[i] = tempMov;
        }
        // WA: If the operand is an alias of another variable but gets mapped to the same variable name,
        // we have to copy the alias into another register. This is because regioning info is determined by
        // the user, and two variables that share the base register but reference different regions are not
        // distinguishable to the inline asm string parser. Thus, a variable pointing to a subregion needs
        // to be copied first before being used as an inline asm input.
        // TODO: To avoid the extra move, we need to be able to explicity define an alias variable with offset
        // instead of a region within the base value.
        else if (opVar->GetAlias() && opVar->GetAliasOffset() > 0 &&
            m_encoder->GetVariableName(opVar) == m_encoder->GetVariableName(opVar->GetAlias()))
        {
            CVariable* tempMov = m_currShader->GetNewVariable(
                opVar->GetNumberElement(), opVar->GetType(), EALIGN_GRF, opVar->IsUniform(), "");
            m_encoder->Copy(tempMov, opVar);
            m_encoder->Push();
            opnds[i] = tempMov;
        }
    }

    // Replace all instances of ${:uid} with a label string unique to this asm block.
    // Clang translates the '%=' format string to '${:uid}' in LLVMIR.
    // This option is useful when creating local labels and referring to them multiple times
    // in a single template that generates multiple assembler instructions.
    {
        string hashStr = m_encoder->GetUniqueInlineAsmLabel();
        string uniqueIDStr = "${:uid}";
        size_t pos = 0;
        while (pos < asmStr.size())
        {
            size_t varPos = asmStr.find(uniqueIDStr, pos);
            if (varPos == string::npos)
                break;
            asmStr.replace(varPos, uniqueIDStr.size(), hashStr);
            pos = varPos + hashStr.size();
        }
    }

    str << endl << "/// Inlined ASM" << endl;
    // Look for variables to replace with the VISA variable
    size_t startPos = 0;
    while (startPos < asmStr.size())
    {
        size_t varPos = asmStr.find('$', startPos);
        if (varPos == string::npos)
            break;

        // Find the operand number
        const char* idStart = &(asmStr[varPos + 1]);
        const char* idEnd = idStart;
        while (*idEnd >= '0' && *idEnd <= '9')
            ++idEnd;

        unsigned val = 0;
        if (StringRef(idStart, idEnd - idStart).getAsInteger(10, val))
        {
            IGC_ASSERT_MESSAGE(0, "Invalid operand format");
            return;
        }
        if (val >= opnds.size())
        {
            IGC_ASSERT_MESSAGE(0, "Invalid operand index");
            return;
        }
        string varName = opnds[val] ? m_encoder->GetVariableName(opnds[val]) : "null";
        asmStr.replace(varPos, (idEnd - idStart + 1), varName);

        startPos = varPos + varName.size();
    }

    str << asmStr;
    if (asmStr.back() != '\n') str << endl;
    str << "/// End Inlined ASM" << endl << endl;
}

void EmitPass::EmitInitializePHI(llvm::PHINode* phi)
{
    if (m_destination->IsUniform())
    {
        return;
    }
    if (m_deSSA && m_deSSA->getRootValue(phi) != nullptr)
    {
        // If this phi is not isolated, it can be safely initialized only if no
        // other values it is coalesced with use NoMask on their destinations.
        // NoMask could be used if the other value is:
        //  - uniform(handled above)
        //  - also a phi that is being initialized
        //  - emitted with m_SubSpanDestination set to true
        {
            SmallVector<Value*, 16> coalescedValues;
            m_deSSA->getAllCoalescedValues(phi, coalescedValues);
            for (auto val : coalescedValues)
            {
                if (val != phi &&
                    (isa<PHINode>(val) && m_pattern->IsSourceOfSample(val)) ||
                    (isa<Instruction>(val) && IsSubspanDestination(cast<Instruction>(val))))
                {
                    return;
                }
            }
        }
    }

    if (m_destination->GetType() == ISA_TYPE_BOOL)
    {
        CVariable* initializedTempVar = m_currShader->GetNewVariable(m_destination->GetNumberElement(), ISA_TYPE_UD, EALIGN_GRF, CName::NONE);
        m_encoder->SetNoMask();
        m_encoder->Copy(initializedTempVar, m_currShader->ImmToVariable(0, ISA_TYPE_UD));
        m_encoder->Push();

        m_encoder->Select(m_destination, initializedTempVar, m_currShader->ImmToVariable(0xFFFFFFFFULL, ISA_TYPE_UD), m_currShader->ImmToVariable(0, ISA_TYPE_UD));
        m_encoder->Push();

        CVariable* initializedFlag = m_currShader->GetNewVariable(m_destination);
        VISA_Type type = GetTypeFromSize(m_destination->GetNumberElement() / BITS_PER_BYTE);
        m_encoder->SetNoMask();
        m_encoder->SetP(initializedFlag, m_currShader->ImmToVariable(0, type));
        m_encoder->Push();

        m_encoder->Cmp(EPREDICATE_EQ, initializedFlag, initializedTempVar, m_currShader->ImmToVariable(0xFFFFFFFFULL, ISA_TYPE_UD));
        m_encoder->Push();

        m_encoder->Copy(m_destination, initializedFlag);
        m_encoder->Push();
    }
    else
    {
        VISA_Type unsignedType = GetUnsignedIntegerType(m_destination->GetType());
        CVariable* initializedVar = m_currShader->GetNewVariable(m_destination);
        CVariable* udAlias = m_currShader->GetNewAlias(initializedVar, unsignedType, 0, m_destination->GetNumberElement());
        m_encoder->SetNoMask();
        m_encoder->Copy(udAlias, m_currShader->ImmToVariable(0, unsignedType));
        m_encoder->Push();

        m_encoder->Copy(initializedVar, m_destination);
        m_encoder->Push();

        m_encoder->SetNoMask();
        m_encoder->Copy(m_destination, initializedVar);
        m_encoder->Push();
    }
}

CVariable* EmitPass::Mul(CVariable* Src0, CVariable* Src1, const CVariable* DstPrototype)
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
            CVariable* Dst = m_currShader->GetNewVariable(DstPrototype);
            m_encoder->Shl(Dst, Src0, VarAmt);
            m_encoder->Push();
            return Dst;
        }
    }

    CVariable* Dst = m_currShader->GetNewVariable(DstPrototype);
    VISA_Type srcType = Src0->GetType();

    // Only i64 muls need special handling, otherwise go back to standard flow
    if (srcType != ISA_TYPE_Q && srcType != ISA_TYPE_UQ)
    {
        m_encoder->Mul(Dst, Src0, Src1);
        m_encoder->Push();
    }
    else {
        CVariable* src[] = { Src0, Src1 };
        Mul64(Dst, src, m_currShader->m_SIMDSize);
    }
    return Dst;
}

CVariable* EmitPass::Add(CVariable* Src0, CVariable* Src1, const CVariable* DstPrototype)
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
    CVariable* Dst = m_currShader->GetNewVariable(DstPrototype);
    m_encoder->Add(Dst, Src0, Src1);
    m_encoder->Push();
    return Dst;
}

bool EmitPass::waveShuffleCase(CVariable* Var, BasicBlock* BB, Instruction* I, bool ForAllInstance)
{
    // waveShuffleIndex with indirect index generating a0.0 and preventing vISA from marking the lifetime.
    // Add a lifetime start in this case to avoid keeping a large payload around when not needed.
    bool found = false;
    CoalescingEngine::CCTuple* ct = m_CE->GetValueCCTupleMapping(I);
    if (ct)
    {
        for (auto* UI : I->users()) // UI is the instruction where the payload is used
        {
            for (uint i = 0; i < UI->getNumOperands(); i++)
            {
                if (!dyn_cast<Instruction>(UI->getOperand(i)))
                    continue;

                if (GenIntrinsicInst* WaveShuffleIndexInst = dyn_cast<GenIntrinsicInst>(UI->getOperand(i)))
                {
                    // if some of the payload come from waveShuffleIndex with indirect index, add the lifetimeStart.
                    if (WaveShuffleIndexInst->getIntrinsicID() == GenISAIntrinsic::GenISA_WaveShuffleIndex ||
                        WaveShuffleIndexInst->getIntrinsicID() == GenISAIntrinsic::GenISA_WaveBroadcast)
                    {
                        CVariable* data = GetSymbol(WaveShuffleIndexInst->getOperand(0));
                        CVariable* simdChannel = GetSymbol(WaveShuffleIndexInst->getOperand(1));

                        if (!data->IsUniform() && !simdChannel->IsImmediate())
                        {
                            found = true;
                        }
                    }
                }

                // all payload src need to be in the same basic block
                if (Instruction* payloadSrcInst = dyn_cast<Instruction>(UI->getOperand(i)))
                {
                    if (payloadSrcInst->getParent() != I->getParent())
                    {
                        found = false;
                        break;
                    }
                }
            }
            if (found)
                break;
        }
    }

    // found the case. Start adding lifetimeStart if this is the first occurance of CC_Tuple.
    // The ones already handled are kept in lifetimeStartAdded.
    if (found)
    {
        // if ct is not previous found, insert it into lifetimeStartAdded. Otherwise the lifetime_start is already added and no need to add again.
        if (lifetimeStartAdded.find(ct) == lifetimeStartAdded.end())
        {
            lifetimeStartAdded.insert(ct);
            if (ForAllInstance)
            {
                for (uint instance = 0; instance < Var->GetNumberInstance(); instance++)
                {
                    m_encoder->SetSecondHalf(instance == 0 ? false : true);
                    m_encoder->Lifetime(LIFETIME_START, Var);
                }
            }
            else
            {
                // Current instance, set already in the calling context.
                m_encoder->Lifetime(LIFETIME_START, Var);
            }
        }
    }
    return found;
}

// Insert lifetime start right before instruction I if it is a candidate.
void EmitPass::emitLifetimeStart(CVariable* Var, BasicBlock* BB, Instruction* I, bool ForAllInstance)
{
    if (waveShuffleCase(Var, BB, I, ForAllInstance)) {
        // already insert a Lifetimestart
        return;
    }

    if (m_pCtx->getVectorCoalescingControl() == 0 || Var == nullptr) {
        return;
    }

    // m_LifetimeAt1stDefOfBB uses dessa root of aliasee as its key
    Value* ARV = m_VRA->getAliasRootValue(I);
    ARV = m_VRA->getRootValue(ARV);

    auto II = m_VRA->m_LifetimeAt1stDefOfBB.find(ARV);
    if (II != m_VRA->m_LifetimeAt1stDefOfBB.end())
    {
        // Insert lifetime start on the root value
        // Note that lifetime is a kind of info directive,
        // thus no m_encoder->Push() is needed.
        CVariable* RootVar = GetSymbol(ARV);
        if (ForAllInstance)
        {
            for (uint instance = 0; instance < RootVar->GetNumberInstance(); instance++)
            {
                m_encoder->SetSecondHalf(instance == 0 ? false : true);
                m_encoder->Lifetime(LIFETIME_START, RootVar);
            }
        }
        else {
            // Current instance, set already in the calling context.
            m_encoder->Lifetime(LIFETIME_START, RootVar);
        }

        // Once inserted, remove it from map to
        // prevent from inserting again.
        m_VRA->m_LifetimeAt1stDefOfBB.erase(II);
    }
}

void EmitPass::emitGEP(llvm::Instruction* I)
{
    GetElementPtrInst& GEP = cast<GetElementPtrInst>(*I);
    unsigned AddrSpace = I->getType()->getPointerAddressSpace();
    VISA_Type PtrTy =
        m_currShader->GetContext()->getRegisterPointerSizeInBits(AddrSpace) == 64 ? ISA_TYPE_UQ : ISA_TYPE_UD;

    // First compute the offset from the base to benefit from constant folding,
    // and then add to the base (which is less likely to be a constant).

    // vOffset is the value of the advancing offset in the loop below
    // Use the pre-allocated variable for storage
    CVariable* vOffset = m_destination;
    // vN is the current offset at the begining of each iteration in the loop below
    CVariable* vN = m_currShader->ImmToVariable(0, PtrTy);

    // Prototype temporary used for cloning from
    CVariable* vTmp = m_currShader->GetNewVariable(
        numLanes(m_currShader->m_SIMDSize),
        PtrTy,
        m_currShader->getGRFAlignment(),
        m_destination->IsUniform(),
        CName::NONE);

    gep_type_iterator GTI = gep_type_begin(GEP);
    for (auto OI = GEP.op_begin() + 1, E = GEP.op_end(); OI != E; ++OI, ++GTI) {
        Value* Idx = *OI;
        // Offset of element contributed by current index being visited
        CVariable* vElemOffset;
        if (StructType * StTy = GTI.getStructTypeOrNull()) {
            // GEP indices into structs are always constant i32's
            unsigned Field = int_cast<unsigned>(cast<Constant>(Idx)->getUniqueInteger().getZExtValue());
            uint64_t Offset = 0;
            if (Field) {
                Offset = m_DL->getStructLayout(StTy)->getElementOffset(Field);
            }
            vElemOffset = m_currShader->ImmToVariable(Offset, ISA_TYPE_UD);
        }
        else {
            Type* Ty = GTI.getIndexedType();

            // vElemOffset = vIdx * vElemSize
            CVariable* vElemSize = m_currShader->ImmToVariable(m_DL->getTypeAllocSize(Ty), PtrTy);
            CVariable* vIdx = GetSymbol(Idx);
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
    CVariable* dst = m_destination;
    if (src->GetType() == ISA_TYPE_BOOL)
    {
        // Prior peephole folds can potentially generate:
        // inttoptr i1 %x to i32*
        dst = m_currShader->BitCast(dst, GetUnsignedType(dst->GetType()));
        const bool doEmitCastSelect =
            (m_currShader->m_Platform->hasPartialEmuI64Enabled() && dst->isQType());
        CVariable* one = m_currShader->ImmToVariable(1, dst->GetType());
        CVariable* zero = m_currShader->ImmToVariable(0, dst->GetType());
        if (doEmitCastSelect)
            emitCastSelect(src, dst, one, zero);
        else
            m_encoder->Select(src, dst, one, zero);
    }
    else
    {
        CVariable* IntVar = m_currShader->BitCast(src, GetUnsignedType(src->GetType()));
        m_encoder->Cast(dst, IntVar);
    }
    m_encoder->Push();
}

void EmitPass::emitBitCast(llvm::BitCastInst* btCst)
{
    if (IsBitCastForLifetimeMark(btCst))
    {
        return;
    }
    Type* srcType = btCst->getOperand(0)->getType();
    Type* dstType = btCst->getType();
    unsigned int numSrcElement = srcType->isVectorTy() ? (unsigned)cast<IGCLLVM::FixedVectorType>(srcType)->getNumElements() : 1;
    unsigned int numDstElement = dstType->isVectorTy() ? (unsigned)cast<IGCLLVM::FixedVectorType>(dstType)->getNumElements() : 1;

    if (srcType->isPointerTy())
    {
        IGC_ASSERT_MESSAGE(dstType->isPointerTy(), "Expected both src and dst have pointer type.");
    }

    if (btCst->getOperand(0)->getType()->isVectorTy() ||
        btCst->getType()->isVectorTy())
    {
        emitVectorBitCast(btCst);
        return;
    }
    CVariable* src = GetSymbol(btCst->getOperand(0));
    CVariable* dst = m_destination;
    IGC_ASSERT(nullptr != src);
    IGC_ASSERT(nullptr != dst);
    IGC_ASSERT_MESSAGE(numSrcElement == 1, "vector to vector bitcast not supported");
    IGC_ASSERT_MESSAGE(numDstElement == 1, "vector to vector bitcast not supported");

    src = m_currShader->BitCast(src, dst->GetType());
    m_encoder->Copy(dst, src);
    m_encoder->Push();
}

// Ether struct to int vector or struct to struct copy.
void EmitPass::emitCopyToOrFromLayoutStruct(Value* D, Value* S)
{
    auto& DL = m_currShader->entry->getParent()->getDataLayout();
    Type* dTy = D->getType();
    Type* sTy = S->getType();
    Value* otherVal = nullptr;
    StructType* stTy = nullptr;
    if (sTy->isStructTy()) {
        otherVal = D;
        stTy = cast<StructType>(sTy);
    }
    else if (dTy->isStructTy()) {
        otherVal = S;
        stTy = cast<StructType>(dTy);
    }

    // Sanity check
    if (!stTy || stTy->getNumElements() == 0) {
        IGC_ASSERT_UNREACHABLE();
        return;
    }

    // Use layout struct's member size as element size for copying
    uint32_t stTyBytes = (uint32_t)DL.getTypeStoreSize(stTy);
    // dEltTy : copy type
    //   struct member could be vector, so take its scalar type.
    //   Use int type for copy always.
    Type* dEltTy = stTy->getElementType(0)->getScalarType();
    uint32_t dEltBytes = (uint32_t)DL.getTypeStoreSize(dEltTy);
    // dEltTy could be of struct or float, int, etc. Here, use int type.
    dEltTy = Type::getIntNTy(dEltTy->getContext(), dEltBytes * 8);
    // dNElts in unit of dEltTy.
    //   Note Legal struct has its size be multiple of dEltTy.
    IGC_ASSERT((stTyBytes % dEltBytes) == 0);
    uint32_t dNElts = stTyBytes / dEltBytes;
    IGC_ASSERT(dNElts >= stTy->getNumElements());

#if defined(_DEBUG)
    // Additional sanity checking
    if (otherVal->getType()->isStructTy()) {
        StructType* stTy1 = cast<StructType>(otherVal->getType());
        // Store combining guarantee that the same layout uses the same type.
        if (stTy1 != stTy) {
            IGC_ASSERT_UNREACHABLE();
            return;
        }
    }
    else {
        // vector or just a scalar type
        Type* eTy = otherVal->getType()->getScalarType();
        uint32_t eBytes = (uint32_t)DL.getTypeStoreSize(eTy);
        auto iVTy = dyn_cast<IGCLLVM::FixedVectorType>(otherVal->getType());
        uint32_t n = iVTy ? (uint32_t)iVTy->getNumElements() : 1;

        if (!eTy->isIntegerTy() ||
            n != dNElts || eBytes != dEltBytes || stTyBytes != (n * eBytes)) {
            IGC_ASSERT_UNREACHABLE();
            return;
        }
    }
#endif
    uint32_t nlanes = numLanes(m_currShader->m_SIMDSize);
    CVariable* baseD = GetSymbol(D);
    Constant* C = dyn_cast<Constant>(S);
    if (C != nullptr && sTy->isStructTy())
    {
        IGC_ASSERT(stTy == cast<StructType>(sTy));
        const StructLayout* SL = DL.getStructLayout(stTy);
        // copy all valid struct members.
        for (uint32_t i = 0; i < stTy->getNumElements(); i++)
        {
            Constant* elt = C->getAggregateElement(i);
            if (isa<UndefValue>(elt)) {
                continue;
            }

            Type* ty = stTy->getElementType(i);
            uint32_t byteOffset = (uint32_t)SL->getElementOffset(i);
            IGC_ASSERT_MESSAGE((byteOffset % dEltBytes) == 0,
                "ICE:  mismatched layout b/w int vector and layout struct!");

            if (ty->isVectorTy()) {
                // Using a normal copy instead of emitVectorCopy(), as
                // emitVectorCopy() will result in additional copies for
                // constant copy a = C like the following:
                //   t = C; a = t
                auto iVTy = cast<IGCLLVM::FixedVectorType>(ty);
                uint32_t nelts = (uint32_t)iVTy->getNumElements();

                Type* eltTy = iVTy->getElementType();
                IGC_ASSERT(DL.getTypeStoreSize(eltTy) == dEltBytes);
                VISA_Type visaty = m_currShader->GetType(eltTy);
                for (uint32_t j = 0; j < nelts; ++j)
                {
                    uint32_t offbytes = byteOffset + dEltBytes * j;
                    Constant* Ci = elt->getAggregateElement(j);
                    CVariable* eltSrc = m_currShader->GetSymbol(Ci);
                    CVariable* eltDst = m_currShader->GetNewAlias(
                        baseD, visaty,
                        offbytes * (baseD->IsUniform() ? 1 : nlanes),
                        baseD->IsUniform() ? 1 : nlanes);
                    m_encoder->Copy(eltDst, eltSrc);
                    m_encoder->Push();
                }
            }
            else {
                // ty is scalar or AOS struct
                uint32_t sz = (uint32_t)DL.getTypeStoreSizeInBits(ty);
                uint64_t imm = bitcastToUI64(elt, &DL);
                IGC_ASSERT(sz == 64 || sz == 32 || sz == 16);
                Type* useTy = Type::getIntNTy(ty->getContext(), sz);
                VISA_Type visaty = GetType(useTy, m_pCtx);
                CVariable* eltSrc = m_currShader->ImmToVariable(imm, visaty);
                CVariable* eltDst = m_currShader->GetNewAlias(
                    baseD, visaty,
                    byteOffset * (baseD->IsUniform() ? 1 : nlanes),
                    (baseD->IsUniform() ? 1 : nlanes));
                m_encoder->Copy(eltDst, eltSrc);
                m_encoder->Push();
            }
        }
        return;
    }
    else if (C != nullptr)
    {
        // This part unlikely happens.
        auto iVTy = dyn_cast<IGCLLVM::FixedVectorType>(sTy);
        uint32_t nelts = iVTy ? (uint32_t)iVTy->getNumElements() : 1;
        Type* eltTy = sTy->getScalarType();
        IGC_ASSERT(DL.getTypeStoreSize(eltTy) == dEltBytes);

        VISA_Type visaty = m_currShader->GetType(eltTy);
        for (uint32_t j = 0; j < nelts; ++j)
        {
            uint32_t offbytes = dEltBytes * j;
            Constant* Ci = C->getAggregateElement(j);
            CVariable* eltSrc = m_currShader->GetSymbol(Ci);
            CVariable* eltDst = m_currShader->GetNewAlias(
                baseD, visaty,
                offbytes * (baseD->IsUniform() ? 1 : nlanes),
                baseD->IsUniform() ? 1 : nlanes);
            m_encoder->Copy(eltDst, eltSrc);
            m_encoder->Push();
        }
        return;
    }

    CVariable* baseS = GetSymbol(S);
    if (baseD == baseS)
        return;

    // Make sure to use dEltType as copy type.
    CVariable* DVar = m_currShader->GetNewAlias(baseD, GetType(dEltTy, m_pCtx), 0, 0);
    CVariable* SVar = m_currShader->GetNewAlias(baseS, DVar->GetType(), 0, 0);
    if (SVar->IsUniform() && DVar->IsUniform())
    {
        emitUniformVectorCopy(DVar, SVar, dNElts);
    }
    else
    {
        // No packing as both Src and Dst have the same layout.
        const uint32_t d_i_nelts = (DVar->IsUniform() ? 1 : nlanes);
        const uint32_t s_i_nelts = (SVar->IsUniform() ? 1 : nlanes);
        for (int i = 0; i < (int)dNElts; i++)
        {
            m_encoder->SetDstSubReg(i * d_i_nelts);
            m_encoder->SetSrcSubReg(0, i * s_i_nelts);
            m_encoder->Copy(DVar, SVar);
            m_encoder->Push();
        }
    }
}

void EmitPass::emitBitcastfromstruct(GenIntrinsicInst* I)
{
    // Special bitcast from a layout struct to an int vector.
    //   %b = bitcastfromstruct struct %a to 4xi32
    //   store 4xi32 *p, %b
    // This bitcast intrinsic is generated for store combining in IGC codegen.
    // The struct's GRF layout is exactly the same as the vector's (struct
    // creating must guarantee this).
    //
    // This bitcast should be aliased by DeSSA, so this function will not be
    // invoked. It is added in case the bitcast is not aliased (in case it is
    // a constant src or DeSSA is off).
    //
    Value* S = I->getOperand(0);
    Value* D = I;
    emitCopyToOrFromLayoutStruct(D, S);
}

void EmitPass::emitBitcasttostruct(GenIntrinsicInst* I)
{
    // Special bitcast from an int vector to a layout struct.
    //   %a   load <4xi32> *p
    //   %b = bitcasttostruct <4xi32> %a to struct
    // This bitcast intrinsic is generated for load combining in IGC codegen.
    // The struct's GRF layout is exactly the same as the vector's (struct
    // creating must guarantee this).
    //
    // This bitcast should be aliased by DeSSA, so this function will not be
    // invoked. It is added in case the bitcast is not aliased (in case DeSSA
    // is off).
    //
    Value* S = I->getOperand(0);
    Value* D = I;
    emitCopyToOrFromLayoutStruct(D, S);
}

void EmitPass::emitPtrToInt(llvm::PtrToIntInst* P2I)
{
    CVariable* dst = m_currShader->BitCast(m_destination, GetUnsignedType(m_destination->GetType()));
    CVariable* PtrVar = GetSymbol(P2I->getOperand(0));
    m_encoder->Cast(dst, PtrVar);
    m_encoder->Push();
}

void EmitPass::emitAddrSpaceToGenericCast(llvm::AddrSpaceCastInst* addrSpaceCast, CVariable* srcV, unsigned tag)
{
    if (m_pCtx->m_hasEmu64BitInsts && m_currShader->m_Platform->hasNoFullI64Support())
    {
        if (m_currShader->GetContext()->getRegisterPointerSizeInBits(addrSpaceCast->getSrcAddressSpace()) == 32)
        {
            // Add tag to high part
            CVariable* dstAlias = m_currShader->BitCast(m_destination, ISA_TYPE_UD);
            // Low:
            m_encoder->SetDstRegion(2);
            m_encoder->Copy(dstAlias, srcV);
            m_encoder->Push();
            // High:
            m_encoder->SetDstSubReg(1);
            m_encoder->SetDstRegion(2);
            m_encoder->Copy(dstAlias, m_currShader->ImmToVariable(tag << 29, ISA_TYPE_UD));
            m_encoder->Push();
        }
        else
        {
            // Src
            CVariable* srcAlias = m_currShader->GetNewAlias(srcV, ISA_TYPE_UD, 0, 0);
            CVariable* srcLow = m_currShader->GetNewVariable(
                numLanes(m_currShader->m_SIMDSize),
                ISA_TYPE_UD, EALIGN_GRF, m_destination->IsUniform(),
                CName(srcV->getName(), "Lo"));
            CVariable* srcHigh = m_currShader->GetNewVariable(
                numLanes(m_currShader->m_SIMDSize),
                ISA_TYPE_UD, EALIGN_GRF, m_destination->IsUniform(),
                CName(srcV->getName(), "Hi"));

            // Split Src into {Low, High}
            // Low:
            m_encoder->SetSrcSubReg(0, 0);
            m_encoder->SetSrcRegion(0, 2, 1, 0);
            m_encoder->Copy(srcLow, srcAlias);
            m_encoder->Push();
            // High:
            m_encoder->SetSrcSubReg(0, 1);
            m_encoder->SetSrcRegion(0, 2, 1, 0);
            m_encoder->Copy(srcHigh, srcAlias);
            m_encoder->Push();

            // Add tag to high part

            // The initial address could be in a canonical form, that means bit 47 is replicated
            // to the upper bits. To set a tag, bits [61:63] must get zeroed before setting a tag with OR operation.
            // For more details please refer to section "Generic Pointer Tagging" in
            // documentation directory under igc/generic-pointers/generic-pointers.md
            m_encoder->And(srcHigh, srcHigh, m_currShader->ImmToVariable(0x1fffffff, ISA_TYPE_UD));
            m_encoder->Or(srcHigh, srcHigh, m_currShader->ImmToVariable(tag << 29, ISA_TYPE_UD));
            m_encoder->Push();

            // Copy result to Dst
            CVariable* dstAlias = m_currShader->BitCast(m_destination, ISA_TYPE_UD);
            // Low:
            m_encoder->SetDstRegion(2);
            m_encoder->Copy(dstAlias, srcLow);
            m_encoder->Push();
            // High:
            m_encoder->SetDstSubReg(1);
            m_encoder->SetDstRegion(2);
            m_encoder->Copy(dstAlias, srcHigh);
            m_encoder->Push();
        }
    }
    else
    {
        CVariable* pTempVar = m_currShader->GetNewVariable(
            numLanes(m_currShader->m_SIMDSize),
            ISA_TYPE_UQ, m_currShader->getGRFAlignment(),
            m_destination->IsUniform(), CName::NONE);
        // The initial address could be in a canonical form, that means bit 47 is replicated
        // to the upper bits. To set a tag, bits [61:63] must get zeroed before setting a tag with OR operation.
        // For more details please refer to section "Generic Pointer Tagging" in
        // documentation directory under igc/generic-pointers/generic-pointers.md
        m_encoder->And(pTempVar, srcV, m_currShader->ImmToVariable(0x1fffffffffffffff, ISA_TYPE_UQ));
        m_encoder->Or(pTempVar, srcV, m_currShader->ImmToVariable(static_cast<uint64_t>(tag) << 61, ISA_TYPE_UQ));
        m_encoder->Cast(m_destination, pTempVar);
        m_encoder->Push();
    }
}

void EmitPass::emitAddrSpaceCast(llvm::AddrSpaceCastInst* addrSpaceCast)
{
    // Tags are used to determine the address space of generic pointers
    // casted from private, local or global pointers.
    // Bit[60:63] are used for this purpose. bit[60] is reserved for future use.
    // Address space tag on bit[61:63] can be:
    // 001: private
    // 010: local
    // 000/111: global

    // In platforms that don't support 64bit operations, 64bit pointers are emulated
    // with pair{i32, i32}. So tags on generic pointers are added/removed by using:
    // - 64bit Or/And operations directly in platforms with 64bit operation support.
    // - 32bit Or/And operations on second element of the pair in platforms with no
    //   64bit operation support.

    CVariable* srcV = GetSymbol(addrSpaceCast->getOperand(0));

    bool skipTagging =
        !m_canGenericPointToLocal &&
        !m_canGenericPointToPrivate &&
        !m_pCtx->mustDistinguishBetweenPrivateAndGlobalPtr();

    if (skipTagging)
    {
        m_encoder->Cast(m_destination, srcV);
        m_encoder->Push();
        return;
    }

    if (srcV->IsImmediate() && srcV->GetImmediateValue() == 0x0)
    {
        // If casting from null, don't do tagging
        m_encoder->Cast(m_destination, srcV);
        m_encoder->Push();
        return;
    }

    unsigned sourceAddrSpace = addrSpaceCast->getSrcAddressSpace();
    unsigned destAddrSpace = addrSpaceCast->getDestAddressSpace();

    if (destAddrSpace == ADDRESS_SPACE_GENERIC)
    {
        // Address space cast is in the form of {private, local, global} -> generic
        // A tag is added according to the address space of the source

        if (sourceAddrSpace == ADDRESS_SPACE_PRIVATE && (!m_pCtx->allocatePrivateAsGlobalBuffer() || m_pCtx->mustDistinguishBetweenPrivateAndGlobalPtr()))
        {
            emitAddrSpaceToGenericCast(addrSpaceCast, srcV, 1);
        }
        else if (sourceAddrSpace == ADDRESS_SPACE_LOCAL)
        {
            emitAddrSpaceToGenericCast(addrSpaceCast, srcV, 2);
        }
        else // ADDRESS_SPACE_GLOBAL
        {
            m_encoder->Cast(m_destination, srcV);
            m_encoder->Push();
        }
    }
    else if (sourceAddrSpace == ADDRESS_SPACE_GENERIC &&
            (destAddrSpace == ADDRESS_SPACE_PRIVATE ||
             destAddrSpace == ADDRESS_SPACE_LOCAL ||
             // Clear a tag for addrspacecast from generic to global if explicit casts are used in a kernel.
             // For more details please refer to section "Generic Address Space Explicit Casts" in
             // documentation directory under igc/generic-pointers/generic-pointers.md
             (destAddrSpace == ADDRESS_SPACE_GLOBAL && m_pCtx->mustDistinguishBetweenPrivateAndGlobalPtr())))
    {
        // Address space cast is in the form of generic -> {private, local, global}
        // Tag is removed according to the address space of the destination

        // The initial address could be in canonical form, that means bit 47 is replicated
        // to the upper bits. As bits [60:63] are spoiled already we need to restore the
        // address to the canonical form. This is done by merging bits [56:59], which we
        // assume are in canonical form, into bits [60:63].

        if (m_pCtx->m_hasEmu64BitInsts && m_currShader->m_Platform->hasNoFullI64Support())
        {
            if (m_currShader->GetContext()->getRegisterPointerSizeInBits(destAddrSpace) == 32)
            {
                // Src
                CVariable* srcAlias = m_currShader->GetNewAlias(srcV, ISA_TYPE_UD, 0, 0);
                CVariable* srcLow = m_currShader->GetNewVariable(
                    numLanes(m_currShader->m_SIMDSize),
                    ISA_TYPE_UD, EALIGN_GRF, m_destination->IsUniform(),
                    CName(srcV->getName(), "Lo"));

                // Get low part of srcV
                m_encoder->SetSrcSubReg(0, 0);
                m_encoder->SetSrcRegion(0, 2, 1, 0);
                m_encoder->Copy(srcLow, srcAlias);
                m_encoder->Push();

                // Copy result to Dst
                m_encoder->Cast(m_destination, srcLow);
                m_encoder->Push();
            }
            else
            {
                // Src
                CVariable* srcAlias = m_currShader->GetNewAlias(srcV, ISA_TYPE_UD, 0, 0);
                CVariable* srcLow = m_currShader->GetNewVariable(
                    numLanes(m_currShader->m_SIMDSize),
                    ISA_TYPE_UD, EALIGN_GRF, m_destination->IsUniform(),
                    CName(srcV->getName(), "Lo"));
                CVariable* srcHigh = m_currShader->GetNewVariable(
                    numLanes(m_currShader->m_SIMDSize),
                    ISA_TYPE_UD, EALIGN_GRF, m_destination->IsUniform(),
                    CName(srcV->getName(), "Hi"));
                CVariable* tempVar = m_currShader->GetNewVariable(
                    numLanes(m_currShader->m_SIMDSize),
                    ISA_TYPE_D, EALIGN_GRF, m_destination->IsUniform(),
                    CName::NONE);

                // Split Src into {Low, High}
                // Low:
                m_encoder->SetSrcSubReg(0, 0);
                m_encoder->SetSrcRegion(0, 2, 1, 0);
                m_encoder->Copy(srcLow, srcAlias);
                m_encoder->Push();
                // High:
                m_encoder->SetSrcSubReg(0, 1);
                m_encoder->SetSrcRegion(0, 2, 1, 0);
                m_encoder->Copy(srcHigh, srcAlias);
                m_encoder->Push();

                // Clear tag in the high part and restore address canonical form.
                // For more details please refer to section "Clearing A Generic Pointer Tag" in
                // documentation directory under igc/generic-pointers/generic-pointers.md
                m_encoder->Shl(tempVar, srcHigh, m_currShader->ImmToVariable(4, ISA_TYPE_D));
                m_encoder->IShr(srcHigh, tempVar, m_currShader->ImmToVariable(4, ISA_TYPE_D));
                m_encoder->Push();

                // Copy to Dst
                CVariable* dstAlias = m_currShader->BitCast(m_destination, ISA_TYPE_UD);
                // Low:
                m_encoder->SetDstRegion(2);
                m_encoder->Copy(dstAlias, srcLow);
                m_encoder->Push();
                // High:
                m_encoder->SetDstSubReg(1);
                m_encoder->SetDstRegion(2);
                m_encoder->Copy(dstAlias, srcHigh);
                m_encoder->Push();
            }
        }
        else
        {
            CVariable* pTempVar = m_currShader->GetNewVariable(
                numLanes(m_currShader->m_SIMDSize),
                ISA_TYPE_Q, m_currShader->getGRFAlignment(),
                m_destination->IsUniform(), CName::NONE);
            // Clear tag in the high part and restore address canonical form.
            // For more details please refer to section "Clearing A Generic Pointer Tag" in
            // documentation directory under igc/generic-pointers/generic-pointers.md
            m_encoder->Shl(pTempVar, srcV, m_currShader->ImmToVariable(4, ISA_TYPE_D));
            m_encoder->IShr(pTempVar, pTempVar, m_currShader->ImmToVariable(4, ISA_TYPE_D));
            m_encoder->Cast(m_destination, pTempVar);
            m_encoder->Push();
        }
    }
    else // ADDRESS_SPACE_GLOBAL
    {
        m_encoder->Cast(m_destination, srcV);
        m_encoder->Push();
    }
}

void EmitPass::emitExtract(llvm::Instruction* inst)
{
    IGC_ASSERT(llvm::isa<llvm::ExtractElementInst>(inst));
    llvm::ExtractElementInst* Extract = llvm::cast<llvm::ExtractElementInst>(inst);
    llvm::Value* vecOperand = Extract->getVectorOperand();
    auto vectorBCI = dyn_cast<BitCastInst>(vecOperand);
    CVariable* vector = m_currShader->GetSymbol(vecOperand, true);

    if (llvm::ConstantInt * pConstElem = llvm::dyn_cast<llvm::ConstantInt>(Extract->getIndexOperand()))
    {
        uint element = m_currShader->AdjustExtractIndex(vecOperand, int_cast<uint16_t>(pConstElem->getZExtValue()));
        // Do not use allocated type to compute the offsets; otherwise the computed
        // offsets may be out-of-bound. The alignment information of the base
        // element type should not impact the offset.
        uint eltBytes = GetScalarTypeSizeInRegister(Extract->getType());
        IGC_ASSERT_MESSAGE(eltBytes, "illegal ExtractElement instruction");

        if (m_currShader->CanTreatAsAlias(Extract))
        {
            if (vectorBCI && m_currShader->getCVarForVectorBCI(vectorBCI, element))
            {
                //do nothing as we can reuse the symbol from the vector bitcast
                return;
            }
            uint offset = 0;
            if (m_currShader->GetIsUniform(inst->getOperand(0)))
            {
                offset = element * eltBytes;
            }
            else
            {
                offset = vector->getOffsetMultiplier() * element * numLanes(m_currShader->m_SIMDSize) * eltBytes;
            }
            // the symbol table should have coalesced those two values;
            // TODO: clean up when we get generic coalescing
            IGC_ASSERT(vector == m_destination->GetAlias() || vector->GetAlias() == m_destination->GetAlias());
            IGC_ASSERT(m_destination->GetAliasOffset() == (offset + vector->GetAliasOffset()));
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
                m_encoder->SetSrcSubVar(0, (offset / getGRFSize()));
                m_encoder->SetSrcSubReg(0, ((offset % getGRFSize()) / eltBytes));
            }
            else
            {
                uint offset = vector->getOffsetMultiplier() * element * numLanes(m_currShader->m_SIMDSize) * eltBytes;
                uint subvar = offset / getGRFSize();
                m_encoder->SetSrcSubVar(0, subvar);
                m_encoder->SetSrcSubReg(0, ((offset % getGRFSize()) / eltBytes));
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
        CVariable* pIndexVar = GetSymbol(pIndex);
        IGC_ASSERT(pIndex->getType()->getPrimitiveSizeInBits() <= 64);

        bool DoAliasing = pIndex->getType()->getPrimitiveSizeInBits() >= 32;
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
            pIndexVar->IsUniform() ? EALIGN_WORD : EALIGN_HWORD,
            pIndexVar->IsUniform(),
            CName::NONE);

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
                EALIGN_HWORD,
                false,
                CName::NONE);
            CVariable* OffsetVar = getOrCreatePerLaneOffsetVariable(vecTypeSize);
            m_encoder->Add(pOffset3, pOffset2, OffsetVar);
            m_encoder->Push();
        }
        else
        {
            // no need to add per-lane offsets
            pOffset3 = pOffset2;
        }

        uint16_t dstNElts = m_destination->GetNumberElement();
        if (dstNElts > 16 &&
            m_currShader->m_numberInstance == 1 &&
            m_currShader->m_SIMDSize == SIMDMode::SIMD32 && !pIndexVar->IsUniform())
        {
            CVariable* dstAlias[2];
            CVariable* indexAlias[2];

            // there are only 16 address registers on PVC, so for SIMD32 VxH we have to split
            for (int i = 0; i < 2; i++)
            {
                // address variable represents register a0
                CVariable* pDstArrElm = m_currShader->GetNewAddressVariable(
                    numLanes(SIMDMode::SIMD16),
                    m_destination->GetType(),
                    false,
                    vector->IsUniform(),
                    m_destination->getName());
                // we add offsets to the base that is the beginning of the vector variable
                m_encoder->SetSimdSize(SIMDMode::SIMD16);
                m_encoder->SetMask(i == 0 ? EMASK_H1 : EMASK_H2);
                indexAlias[i] = m_currShader->GetNewAlias(pOffset3, pOffset3->GetType(), (pOffset3->GetNumberElement() / 2) * 2 * i, pOffset3->GetNumberElement() / 2);
                m_encoder->AddrAdd(pDstArrElm, vector, indexAlias[i]);
                m_encoder->Push();

                dstAlias[i] = m_currShader->GetNewAlias(m_destination, m_destination->GetType(), (dstNElts / 2) * vecTypeSize * i, dstNElts / 2);
                CVariable* predicate = IGC_IS_FLAG_ENABLED(UseVMaskPredicateForIndirectMove) ? GetCombinedVMaskPred() : nullptr;

                // finally, we move the indirectly addressed values to the destination register
                m_encoder->SetMask(i == 0 ? EMASK_H1 : EMASK_H2);
                m_encoder->SetSimdSize(SIMDMode::SIMD16);

                // to avoid out-of-bounds indirect access (exceeding the maximum number of GRFs)
                m_encoder->SetPredicate(predicate);

                m_encoder->Copy(dstAlias[i], pDstArrElm);
                m_encoder->Push();
            }
        }
        else
        {
            // address variable represents register a0
            CVariable* pDstArrElm = m_currShader->GetNewAddressVariable(
                pIndexVar->IsUniform() ? 1 : numLanes(m_currShader->m_SIMDSize),
                m_destination->GetType(),
                pIndexVar->IsUniform(),
                vector->IsUniform(),
                m_destination->getName());

            // we add offsets to the base that is the beginning of the vector variable
            m_encoder->AddrAdd(pDstArrElm, vector, pOffset3);
            m_encoder->Push();

            // to avoid out-of-bounds indirect access (exceeding the maximum number of GRFs)
            m_encoder->SetPredicate(IGC_IS_FLAG_ENABLED(UseVMaskPredicateForIndirectMove) ? GetCombinedVMaskPred() : nullptr);

            // finally, we move the indirectly addressed values to the destination register
            m_encoder->Copy(m_destination, pDstArrElm);
            m_encoder->Push();
        }
    }
}

void EmitPass::emitUAVSerialize()
{
    m_encoder->Wait();
    m_encoder->Push();
}

bool EmitPass::useRasterizerOrderedByteAddressBuffer(GenIntrinsicInst* inst)
{
    if (IGC_GET_FLAG_VALUE(RovOpt) == 0)
        return false;

    bool isRov = false;
    std::vector<uint32_t> ROV_RV = m_currShader->m_ModuleMetadata->RasterizerOrderedByteAddressBuffer;

    unsigned calleeArgNo = 0;
    PushInfo& pushInfo = m_currShader->m_ModuleMetadata->pushInfo;

    Value* src = IGC::TracePointerSource(inst->getOperand(0));
    if (src)
    {
        if (Argument* calleeArg = dyn_cast<Argument>(src))
        {
            calleeArgNo = calleeArg->getArgNo();
            for (auto index_it = pushInfo.constantReg.begin(); index_it != pushInfo.constantReg.end(); ++index_it)
            {
                if (index_it->second == calleeArgNo)
                {
                    if (std::find(ROV_RV.begin(), ROV_RV.end(), index_it->first) != ROV_RV.end())
                    {
                        isRov = true;
                        break;
                    }
                }
            }
        }
    }
    return isRov;
}

void EmitPass::setRovCacheCtrl(GenIntrinsicInst* inst)
{
    MDNode* node = MDNode::get(
        inst->getContext(),
        ConstantAsMetadata::get(
            ConstantInt::get(Type::getInt32Ty(inst->getContext()), LSC_L1UC_L3C_WB)));
    inst->setMetadata("lsc.cache.ctrl", node);
}

void EmitPass::emitLoadRawIndexed(
    LdRawIntrinsic * inst, Value * varOffset, ConstantInt * immOffset)
{
    Value* bufPtrv = inst->getResourceValue();

    ResourceDescriptor resource = GetResourceVariable(bufPtrv);
    LSC_DOC_ADDR_SPACE addrSpace = m_pCtx->getUserAddrSpaceMD().Get(inst);
    m_currShader->m_State.isMessageTargetDataCacheDataPort = true;
    if (shouldGenerateLSC(inst))
    {
        if ((IGC_GET_FLAG_VALUE(RovOpt) & 2) && useRasterizerOrderedByteAddressBuffer(inst))
        {
            setRovCacheCtrl(inst);
        }
        LSC_CACHE_OPTS cacheOpts =
            translateLSCCacheControlsFromMetadata(inst, true);
        emitLSCVectorLoad(
            inst,
            bufPtrv,
            varOffset,
            immOffset,
            nullptr,
            cacheOpts,
            addrSpace
          );
        return;
    }
    IGC_ASSERT(immOffset == nullptr);
    emitLoad3DInner(inst, resource, varOffset);
}

void EmitPass::emitLoad3DInner(LdRawIntrinsic* inst, ResourceDescriptor& resource, Value* elem_idxv)
{
    IGC::e_predefSurface predDefSurface = resource.m_surfaceType;
    CVariable* gOffset = m_currShader->ImmToVariable(0x0, ISA_TYPE_UD);

    CVariable* src_offset = GetSymbol(elem_idxv);

    // still collect buffer type here to work around some alignment problem with different messages
    BufferType bufType = GetBufferType(inst->getOperand(0)->getType()->getPointerAddressSpace());

    // generate oword_load if it is uniform
    // otherwise, generate gather/gather4
    if (m_currShader->GetIsUniform(inst))
    {
        IGC_ASSERT_MESSAGE(predDefSurface != ESURFACE_STATELESS, "scratch cannot be uniform");
        Type* loadType = inst->getType();
        uint numElement = loadType->isVectorTy() ? (uint)cast<IGCLLVM::FixedVectorType>(loadType)->getNumElements() : 1;
        if (predDefSurface == ESURFACE_SLM)
        {
            IGC_ASSERT(numElement <= 4);
            uint numLane = (numElement == 3) ? 4 : numElement;
            // there is no oword-block read for SLM, also we expect loading only up to 4-dwords
            CVariable* imm = m_currShader->ImmToVariable(0x0C840, ISA_TYPE_UV);
            CVariable* srcTmp = m_currShader->GetNewVariable(
                (uint16_t)numLane, ISA_TYPE_UD, m_currShader->getGRFAlignment(), true,
                CName(src_offset->getName(), "Broadcast"));
            m_encoder->SetNoMask();
            m_encoder->SetUniformSIMDSize(lanesToSIMDMode(numLane));
            m_encoder->Add(srcTmp, src_offset, imm);
            m_encoder->Push();
            CVariable* dstTmp = m_destination;
            if (numElement != numLane)
            {
                dstTmp = m_currShader->GetNewVariable(
                    (uint16_t)numLane, ISA_TYPE_D, m_currShader->getGRFAlignment(), true,
                    CName::NONE);
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
        else if (predDefSurface == ESURFACE_SCRATCH && m_currShader->m_Platform->hasScratchSurface() && inst->getAlignment() >= 4)
        {
            IGC_ASSERT(numElement <= 8);
            CVariable* tmpAddress = nullptr;
            if (numElement > 1)
            {
                tmpAddress = m_currShader->GetNewVariable(numElement, ISA_TYPE_UD, EALIGN_GRF, true, CName::NONE);
                m_encoder->SetNoMask();
                m_encoder->SetUniformSIMDSize(lanesToSIMDMode(numElement));
                m_encoder->Shl(tmpAddress, m_currShader->ImmToVariable(0x76543210, ISA_TYPE_V), m_currShader->ImmToVariable(2, ISA_TYPE_D));
                m_encoder->Push();
                m_encoder->SetNoMask();
                m_encoder->SetUniformSIMDSize(lanesToSIMDMode(numElement));
                m_encoder->Add(tmpAddress, tmpAddress, src_offset);
                m_encoder->Push();
            }
            else
            {
                tmpAddress = m_currShader->GetNewVariable(numElement, ISA_TYPE_UD, EALIGN_GRF, true, CName::NONE);
                m_encoder->SetNoMask();
                m_encoder->SetUniformSIMDSize(SIMDMode::SIMD1);
                m_encoder->Copy(tmpAddress, src_offset);
                m_encoder->Push();
            }

            bool needsTempDest = numElement < 4;
            CVariable* destination = m_destination;
            if (needsTempDest)
            {
                uint elemSize = m_destination->GetElemSize();
                destination = m_currShader->GetNewVariable(
                    numElement * SIZE_DWORD / elemSize, m_destination->GetType(),
                    EALIGN_GRF, m_destination->IsUniform(), CName::NONE);
            }

            m_encoder->SetNoMask();
            m_encoder->SetUniformSIMDSize(lanesToSIMDMode(numElement));
            m_encoder->Gather4Scaled(destination, resource, tmpAddress);
            m_encoder->Push();
            if (needsTempDest)
            {
                // generate an extract-element
                for (uint i = 0; i < numElement; i++)
                {
                    m_encoder->SetSrcSubReg(0, i);
                    m_encoder->SetDstSubReg(i);
                    m_encoder->SetSrcRegion(0, 0, 1, 0);
                    m_encoder->Copy(m_destination, destination);
                    m_encoder->Push();
                }
            }
        }
        else
        {
            bool owordAligned = false;
            // need to clear lower two-bits for unaligned
            CVariable* visaOffset = nullptr;
            if (bufType == CONSTANT_BUFFER)
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
                visaOffset = m_currShader->GetNewVariable(
                    src_offset->GetNumberElement(),
                    ISA_TYPE_UD,
                    src_offset->GetAlign(),
                    src_offset->IsUniform(),
                    src_offset->getName());
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
                IGC_ASSERT(GetPrimitiveTypeSizeInRegisterInBits(loadType) < SIZE_DWORD * 8 * 4);
                uint elemSize = m_destination->GetElemSize();

                if (elemSize > 0)
                {
                    unsigned int alignment = inst->getAlignment();
                    if (alignment < SIZE_DWORD && !(src_offset->IsImmediate() && src_offset->GetImmediateValue() % SIZE_DWORD == 0))
                    {
                        IGC_ASSERT(alignment == 1 || alignment == 2);
                        IGC_ASSERT(src_offset->IsUniform());
                        uint destSize = m_destination->GetSize();
                        // Alignment can be greater than the destination size. If that happens,
                        // alignment needs to be clamped to the destination size.
                        alignment = (alignment > destSize) ? destSize : alignment;
                        uint numElements = m_destination->GetSize() / alignment;
                        VISA_Type realType = alignment == 1 ? ISA_TYPE_UB : ISA_TYPE_UW;
                        CVariable* tmp = m_currShader->GetNewVariable(
                            numElements * (SIZE_DWORD / alignment), realType, EALIGN_GRF, true, CName::NONE);
                        if (numElements > 1)
                        {
                            IGC_ASSERT(numElements <= 8);
                            CVariable* offsetVector = m_currShader->GetNewVariable(numElements, ISA_TYPE_UD, EALIGN_GRF, CName::NONE);
                            m_encoder->SetSimdSize(lanesToSIMDMode(numElements));
                            m_encoder->SetNoMask();
                            m_encoder->Add(offsetVector, src_offset, m_currShader->ImmToVariable(alignment * 0x76543210, ISA_TYPE_UV));
                            m_encoder->Push();
                            src_offset = offsetVector;
                        }
                        else if (src_offset->IsImmediate() || src_offset->GetAlign() != EALIGN_GRF)
                        {
                            IGC_ASSERT(numElements == 1);
                            CVariable* tmpSrcOffset = m_currShader->GetNewVariable(numElements, ISA_TYPE_UD, EALIGN_GRF, true, CName::NONE);
                            m_encoder->SetSimdSize(lanesToSIMDMode(numElements));
                            m_encoder->SetNoMask();
                            m_encoder->Cast(tmpSrcOffset, src_offset);
                            m_encoder->Push();
                            src_offset = tmpSrcOffset;
                        }
                        m_encoder->SetSimdSize(lanesToSIMDMode(numElements));
                        m_encoder->SetNoMask();
                        m_encoder->ByteGather(tmp, resource, src_offset, 8, alignment);
                        m_encoder->Push();
                        CVariable* dstWordAlias = m_currShader->GetNewAlias(m_destination, realType, 0, 0, false);
                        m_encoder->SetSimdSize(lanesToSIMDMode(numElements));
                        m_encoder->SetNoMask();
                        m_encoder->SetSrcRegion(0, SIZE_DWORD / alignment, 1, 0);
                        m_encoder->Copy(dstWordAlias, tmp);
                        m_encoder->Push();
                    }
                    else
                    {
                        CVariable* tmp = m_currShader->GetNewVariable(
                            4 * SIZE_DWORD / elemSize, m_destination->GetType(), EALIGN_GRF, m_destination->IsUniform(), CName::NONE);
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
    }
    else
    {
        uint label = 0;
        CVariable* flag = nullptr;
        bool needLoop = ResourceLoopHeader(resource, flag, label);
        ResourceLoopSubIteration(resource, flag, label);
        uint sizeInBits = GetPrimitiveTypeSizeInRegisterInBits(inst->getType());
        IGC_ASSERT_MESSAGE((sizeInBits == 8) || (sizeInBits == 16) || (sizeInBits == 32) || (sizeInBits == 64) || (sizeInBits == 96) || (sizeInBits == 128),
            "load type must be 1/2/4/8/12/16 bytes long");
        IGC::CVariable* visaOffset = BroadcastIfUniform(src_offset);
        unsigned int alignment = inst->getAlignment();
        Type* loadType = inst->getType();
        if (sizeInBits == 32 && resource.m_surfaceType == ESURFACE_STATELESS &&
            m_currShader->m_Platform->getWATable().WaNoA32ByteScatteredStatelessMessages)
        {
            // DWORD gather
            CVariable* shiftedPtr = m_currShader->GetNewVariable(visaOffset);
            m_encoder->Shr(shiftedPtr, visaOffset, m_currShader->ImmToVariable(2, ISA_TYPE_UD));
            m_encoder->Push();
            visaOffset = shiftedPtr;
            m_encoder->SetPredicate(flag);
            m_encoder->Gather(m_destination, resource.m_resource, visaOffset, gOffset, resource.m_surfaceType, 4);
            m_encoder->Push();
        }
        else if (sizeInBits == 32 && (bufType == CONSTANT_BUFFER || resource.m_surfaceType == ESURFACE_STATELESS || alignment < 4))
        {
            // uav and resource cannot be changed to this path due to alignment issue encountered in some tests
            uint elementSize = 8;
            uint numElems = 4;
            m_encoder->SetPredicate(flag);
            m_encoder->ByteGather(m_destination, resource, visaOffset, elementSize, numElems);
            m_encoder->Push();
        }
        else if (sizeInBits == 64 && (loadType->getScalarType()->getPrimitiveSizeInBits() == 64))
        {
            // QW load
            IGCLLVM::FixedVectorType* vecType = dyn_cast<IGCLLVM::FixedVectorType>(loadType);
            uint elementSize = 64;
            uint numElems = (uint)(vecType ? vecType->getNumElements() : 1);
            IGC_ASSERT_MESSAGE(numElems == 1, "QW vector load not supported yet!");

            m_encoder->SetPredicate(flag);
            m_encoder->QWGather(m_destination, resource, visaOffset, elementSize, numElems);
            m_encoder->Push();
        }
        else if (sizeInBits >= 32)
        {
            // constant-buffer cannot go this way due to driver surface-state setting to RGBA-F32
            if (bufType == CONSTANT_BUFFER || bufType == BINDLESS_CONSTANT_BUFFER)
            {
                IGC_ASSERT(!UsesTypedConstantBuffer(m_currShader->GetContext(), bufType));
            }

            m_encoder->SetPredicate(flag);
            m_encoder->Gather4ScaledNd(m_destination, resource, visaOffset, sizeInBits / 32);
            m_encoder->Push();
        }
        else if (sizeInBits == 8 || sizeInBits == 16)
        {
            uint elementSize = 8;
            uint numElems = sizeInBits / 8;
            uint hStride = 32 / sizeInBits;
            uint16_t vStride = numLanes(m_currShader->m_SIMDSize);
            CVariable* gatherDest = m_currShader->GetNewVariable(vStride, ISA_TYPE_UD, EALIGN_GRF, CName::NONE);
            m_encoder->SetPredicate(flag);
            m_encoder->ByteGather(gatherDest, resource, visaOffset, elementSize, numElems);
            m_encoder->Push();

            gatherDest = m_currShader->GetNewAlias(gatherDest, m_destination->GetType(), 0, 0);
            m_encoder->SetSrcRegion(0, vStride, vStride / hStride, hStride);
            m_encoder->Cast(m_destination, gatherDest);
            m_encoder->Push();
        }
        ResourceLoopBackEdge(needLoop, flag, label);
    }
}

void EmitPass::emitLoad(
    LoadInst *inst,
    Value *offset,
    ConstantInt *immOffset,
    ConstantInt *immScale
) {
    if (m_pCtx->getModuleMetaData()->isHDCFastClearShader)
    {
        emitFastClear(inst);
        return;
    }
    if (shouldGenerateLSC(inst))
    {
        IGC_ASSERT(offset);
        LSC_CACHE_OPTS cacheOpts =
            translateLSCCacheControlsFromMetadata(inst, true);
        LSC_DOC_ADDR_SPACE addrSpace = m_pCtx->getUserAddrSpaceMD().
            Get(inst);

        emitLSCVectorLoad(
            inst,
            inst->getPointerOperand(),
            offset,
            immOffset,
            immScale,
            cacheOpts,
            addrSpace
        );
        return;
    }
    IGC_ASSERT_MESSAGE(immScale ? immScale->getSExtValue() == 1 : true,
                       "Immediate Scale not supported on non-LSC path!");

    emitVectorLoad(inst, offset, immOffset);
}


void EmitPass::EmitNoModifier(llvm::Instruction* inst)
{
    // This is a single instruction pattern emitter
    // Check if this inst has been turned into noop due to alias.
    // If so, no code shall be emitted for this instruction.
    if (m_currShader->HasBecomeNoop(inst))
    {
        return;
    }

    if (IGC_IS_FLAG_ENABLED(EnableDeSSA) &&
        m_deSSA && m_deSSA->isNoopAliaser(inst))
    {
        return;
    }

    switch (inst->getOpcode())
    {
    case Instruction::Ret:
        emitReturn(cast<ReturnInst>(inst));
        break;
    case Instruction::Call:
        if (GenIntrinsicInst * I = dyn_cast<GenIntrinsicInst>(inst))
        {
            EmitGenIntrinsicMessage(I);
        }
        else if (IntrinsicInst * I = dyn_cast<IntrinsicInst>(inst))
        {
            EmitIntrinsicMessage(I);
        }
        else if (cast<CallInst>(inst)->isInlineAsm())
        {
            EmitInlineAsm(cast<CallInst>(inst));
        }
        else
        {
            emitCall(cast<CallInst>(inst));
        }
        break;
    case Instruction::Store:
        emitStore(cast<StoreInst>(inst),
            cast<StoreInst>(inst)->getPointerOperand(),
            nullptr);
        break;
    case Instruction::Load:
        emitLoad(
            cast<LoadInst>(inst),
            cast<LoadInst>(inst)->getPointerOperand(),
            nullptr);
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
    case Instruction::InsertValue:
        EmitInsertValueToStruct(cast<InsertValueInst>(inst));
        break;
    case Instruction::ExtractValue:
        EmitExtractValueFromStruct(cast<ExtractValueInst>(inst));
        break;
    case Instruction::Unreachable:
        break;
    default:
        IGC_ASSERT_MESSAGE(0, "need to add code gen support for this instruction");
    }
}

void EmitPass::emitPairToPtr(GenIntrinsicInst* GII) {
    CVariable* Lo = GetSymbol(GII->getOperand(0));
    CVariable* Hi = GetSymbol(GII->getOperand(1));

    unsigned AS = GII->getType()->getPointerAddressSpace();
    if (m_currShader->GetContext()->getRegisterPointerSizeInBits(AS) == 32) {
        CVariable* Tmp = m_currShader->BitCast(Lo, GetUnsignedType(Lo->GetType()));
        m_encoder->Cast(m_destination, Tmp);
        m_encoder->Push();
        return;
    }

    IGC_ASSERT_MESSAGE(m_currShader->GetContext()->getRegisterPointerSizeInBits(AS) == 64,
        "Pointer size should be either 32 or 64!");

    CVariable* Dst32 = m_currShader->BitCast(m_destination, ISA_TYPE_UD);
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

void EmitPass::emitLLVMStackSave(llvm::IntrinsicInst* inst) {
    // save current SP
    CVariable* pSP = m_currShader->GetSP();
    m_encoder->Copy(m_destination, pSP);
    m_encoder->Push();
}

void EmitPass::emitLLVMStackRestore(llvm::IntrinsicInst* inst) {
    // restore the SP to arg(0)
    CVariable* pSP = m_currShader->GetSP();
    CVariable* savedSP = m_currShader->GetSymbol(inst->getOperand(0));
    // stacksave and stackrestore are forced to be uniform in WIAnalysis.
    // While here we still set to scalar region just in case
    m_encoder->SetSrcRegion(0, 0, 1, 0);
    m_encoder->Copy(pSP, savedSP);
    m_encoder->Push();
}

void EmitPass::emitVLAStackAlloca(llvm::GenIntrinsicInst* intrinsic)
{
    CVariable* pSP = m_currShader->GetSP();
    CVariable* lane_off = m_currShader->GetSymbol(intrinsic->getOperand(0));
    // m_destination = curr_SP + lane_offset
    emitAddPointer(m_destination, pSP, lane_off);
    m_encoder->Push();

    if (m_currShader->m_numberInstance == 1 || m_encoder->IsSecondHalf()) {
        // SP = SP + vla_size * simdWidth
        CVariable* vla_size = m_currShader->GetSymbol(intrinsic->getOperand(1));
        // vla_size must be uniform, if it's not uniform, set region to take only <0;1,0>
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        m_encoder->Mul(vla_size, vla_size,
            m_currShader->ImmToVariable(numLanes(m_currShader->m_SIMDSize), ISA_TYPE_UW));
        m_encoder->Push();

        m_encoder->SetSrcRegion(1, 0, 1, 0);
        emitAddPointer(pSP, pSP, vla_size);
        m_encoder->Push();
    }

    if (IGC_IS_FLAG_ENABLED(StackOverflowDetection)) {
        emitStackOverflowDetectionCall(intrinsic->getFunction(), false);
    }
}

void EmitPass::emitStackAlloca(GenIntrinsicInst* GII)
{
    // Static private mem access is done through the FP
    CVariable* pFP = m_currShader->GetFP();
    if (IGC_IS_FLAG_ENABLED(EnableWriteOldFPToStack) && m_pCtx->type != ShaderType::RAYTRACING_SHADER)
    {
        // If we have written the previous FP to the current frame's start, the start of
        // private memory will be offset.
        CVariable* tempFP = m_currShader->GetNewVariable(pFP);
        Function* parentFunc = GII->getParent()->getParent();
        auto funcMDItr = m_currShader->m_ModuleMetadata->FuncMD.find(parentFunc);
        IGC_ASSERT(funcMDItr != m_currShader->m_ModuleMetadata->FuncMD.end());
        emitAddPointer(tempFP, pFP, m_currShader->ImmToVariable(funcMDItr->second.prevFPOffset, ISA_TYPE_UD));
        pFP = tempFP;
    }
    CVariable* pOffset = m_currShader->GetSymbol(GII->getOperand(0));
    emitAddPointer(m_destination, pFP, pOffset);
}

void EmitPass::emitCall(llvm::CallInst* inst)
{
    llvm::Function* F = inst->getCalledFunction();
    if (!F || F->hasFnAttribute("referenced-indirectly") || (m_FGA && m_FGA->useStackCall(F)))
    {
        emitStackCall(inst);
        return;
    }

    IGC_ASSERT_MESSAGE(!F->empty(), "unexpanded builtin?");

    unsigned i = 0;
    for (auto& Arg : F->args())
    {
        // Skip unused arguments if any.
        if (Arg.use_empty())
        {
            ++i;
            continue;
        }

        CVariable* Dst = m_currShader->getOrCreateArgumentSymbol(&Arg, true);
        CVariable* Src = GetSymbol(inst->getArgOperand(i++));

        // When both symbols are the same, then this argument passing has been
        // lifted to use a global vISA variable, just skip the copy.
        if (Dst != Src)
        {
            emitCopyAll(Dst, Src, Arg.getType());
        }
    }
    m_currFuncHasSubroutine = true;
    m_encoder->SubroutineCall(nullptr, F);
    m_encoder->Push();

    // Emit the return value if used.
    if (!inst->use_empty())
    {
        CVariable* Dst = GetSymbol(inst);
        CVariable* Src = m_currShader->getOrCreateReturnSymbol(F);
        emitCopyAll(Dst, Src, inst->getType());
    }
}

void EmitPass::emitReturn(llvm::ReturnInst* inst)
{
    llvm::Function* F = inst->getParent()->getParent();
    MetaDataUtils* pMdUtils = m_currShader->GetMetaDataUtils();

    // return from a function (not a kernel)
    if (!isEntryFunc(pMdUtils, F))
    {
        if (m_FGA && m_FGA->useStackCall(F))
        {
            emitStackFuncExit(inst);
            return;
        }

        llvm::Type* RetTy = F->getReturnType();
        if (!RetTy->isVoidTy())
        {
            CVariable* Dst = m_currShader->getOrCreateReturnSymbol(F);
            CVariable* Src = GetSymbol(inst->getReturnValue());
            emitCopyAll(Dst, Src, RetTy);
        }

        m_encoder->SubroutineRet(nullptr, F);
        m_encoder->Push();
        return;
    }

    m_currShader->AddEpilogue(inst);
}

/// Initializes the kernel for stack call by initializing the SP and FP
void EmitPass::InitializeKernelStack(Function* pKernel, CVariable* stackBufferBase)
{
    m_currShader->InitializeStackVariables();
    auto pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    auto pModMD = pCtx->getModuleMetaData();

    auto* pStackBufferBase = stackBufferBase ? stackBufferBase : m_currShader->GetPrivateBase();

    CVariable* pHWTID = m_currShader->GetHWTID();

    IGC_ASSERT(pModMD->FuncMD.find(pKernel) != pModMD->FuncMD.end());
    unsigned kernelAllocaSize = pModMD->FuncMD[pKernel].privateMemoryPerWI;

    CVariable *pSize = getStackSizePerThread(pKernel);

    bool is64bitOffsetPossible = true;
    if (pSize->IsImmediate() && (pSize->GetImmediateValue() * pCtx->platform.getMaxAddressedHWThreads()) <= (uint64_t)UINT32_MAX ) {
        is64bitOffsetPossible = false;
    }
    bool isA64Private = pCtx->getRegisterPointerSizeInBits(ADDRESS_SPACE_PRIVATE) == 64;
    bool is64bitOffset = isA64Private && is64bitOffsetPossible;

    CVariable* pThreadOffset = m_currShader->GetNewVariable(1, is64bitOffset ? ISA_TYPE_UQ : ISA_TYPE_UD,
        is64bitOffset ? EALIGN_QWORD : EALIGN_DWORD, true, 1, CName::NONE);
    if (is64bitOffset && !m_currShader->m_Platform->hasInt64DstMul())
    {
        emitMul64_UDxUD(pThreadOffset, pHWTID, pSize);
    }
    else
    {
        m_encoder->Mul(pThreadOffset, pHWTID, pSize);
        m_encoder->Push();
    }

    unsigned totalAllocaSize = kernelAllocaSize * numLanes(m_currShader->m_State.m_dispatchSize);
    totalAllocaSize += pModMD->FuncMD[pKernel].prevFPOffset;

    // Initialize SP to per-thread kernel stack base
    CVariable* pSP = m_currShader->GetSP();
    emitAddPointer(pSP, pStackBufferBase, pThreadOffset);

    if (IGC_IS_FLAG_ENABLED(StackOverflowDetection)) {
        emitStackOverflowDetectionCall(pKernel, true);
    }

    // Push a new stack frame
    emitPushFrameToStack(pKernel, totalAllocaSize);

    // Set the total alloca size for the entry function
    m_encoder->SetFunctionAllocaStackSize(pKernel, totalAllocaSize);
}

/// Summary:
/// Calculates the block offsets needed for stack read/write instructions for
/// a given vector of CVariables. Used for storing and loading stack-function
/// args/retval to the stack relative to the current Stack Pointer.
///
/// Input: Args - A vector of CVariables containing the registers to be stored/loaded
/// Output: blkData - A data vector containing the following info:
///     The variable to write to stack, or to store the read data from stack
///     The offset relative to the Stack Pointer
///     The block size to be stored/loaded
///     The offset relative to the current variable being processed
///     Boolean to indicate IsSecondHalf for instructions with multiple instances
/// Output: Returns the total byte size required for read/write
///
uint EmitPass::CalculateStackDataBlocks(StackDataBlocks& blkData, std::vector<CVariable*>& Args)
{
    uint32_t offsetS = 0;
    for (auto Arg : Args)
    {
        // stack offset is always oword-aligned
        offsetS = int_cast<unsigned>(llvm::alignTo(offsetS, SIZE_OWORD));
        unsigned numInstance = Arg->GetNumberInstance();

        for (unsigned i = 0; i < numInstance; i++)
        {
            // calculate block sizes for each arg
            int32_t RmnBytes = Arg->GetSize();
            uint32_t ArgOffset = 0;
            do
            {
                uint32_t BlkSize = 0;
                if (shouldGenerateLSC())
                {
                    BlkSize = getLSCBlockMsgSize(RmnBytes, m_currShader->m_Platform->getMaxLSCBlockMsgSize());
                }
                else
                {
                    BlkSize = getBlockMsgSize(RmnBytes, m_currShader->m_Platform->getMaxBlockMsgSize(false));
                }
                bool IsSecondHalf = (i == 1);
                blkData.push_back(std::make_tuple(Arg, offsetS, BlkSize, ArgOffset, IsSecondHalf));

                offsetS += BlkSize;
                ArgOffset += BlkSize;
                RmnBytes -= BlkSize;
            } while (RmnBytes > 0);
        }
    }
    return offsetS;
}

/// Given the data block vector calculated by CalculateStackDataBlocks, for each block
/// read from stack memory starting at (SP - offsetS) up to offsetS bytes.
void EmitPass::ReadStackDataBlocks(StackDataBlocks& blkData, uint offsetS)
{
    // Get current SP
    CVariable* pSP = m_currShader->GetSP();
    bool useA64 = (pSP->GetSize() == 8);

    // Load each OWORD block from stack
    for (auto& [Arg, StackOffset, BlkSize, ArgOffset, IsSecondHalf] : blkData)
    {
        // spOffset is a negative offset from SP
        int32_t spOffset = StackOffset - offsetS;
        IGC_ASSERT(spOffset < 0);

        CVariable* LdDst = Arg;

        ResourceDescriptor resource;
        resource.m_surfaceType = ESURFACE_STATELESS;

        m_encoder->SetSecondHalf(IsSecondHalf);

        int RmnBytes = LdDst->GetSize() - ArgOffset;
        bool needRmCopy = BlkSize == SIZE_OWORD && RmnBytes > 0 && RmnBytes < SIZE_OWORD;
        // LSC Gather message
        if (shouldGenerateLSC())
        {
            pSP = ReAlignUniformVariable(pSP, EALIGN_GRF);
            unsigned blkBits = 64;
            unsigned nBlks = (BlkSize * 8) / 64;
            if (needRmCopy)
            {
                // Fixme: Is it possible for args to be non 8byte aligned?
                IGC_ASSERT_MESSAGE(RmnBytes == 8, "Minimum LSC block size is 8 bytes");
            }
            emitLSCLoad(nullptr, LdDst, pSP, blkBits, nBlks, ArgOffset, &resource, useA64 ? LSC_ADDR_SIZE_64b : LSC_ADDR_SIZE_32b, LSC_DATA_ORDER_TRANSPOSE, spOffset, 1);
            m_encoder->Push();
        }
        else
        {
            // SP offset for each block
            CVariable* pTempSP = m_currShader->GetNewVariable(pSP);
            emitAddPointer(pTempSP, pSP, m_currShader->ImmToVariable(spOffset, ISA_TYPE_D));

            CVariable* offsetShr = nullptr;
            if (!useA64)
            {
                offsetShr = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true, "SPOffset_OW");
                m_encoder->SetSimdSize(SIMDMode::SIMD1);
                m_encoder->SetNoMask();
                m_encoder->SetSrcRegion(0, 0, 1, 0);
                m_encoder->Shr(offsetShr, pTempSP, m_currShader->ImmToVariable(4, ISA_TYPE_UD));
                m_encoder->Push();
            }

            if (!needRmCopy)
            {
                if (useA64)
                    m_encoder->OWLoadA64(LdDst, pTempSP, BlkSize, ArgOffset);
                else
                    m_encoder->OWLoad(LdDst, resource, offsetShr, false, BlkSize, ArgOffset);
                m_encoder->Push();
            }
            else
            {
                // Reading less than one oword, read one oword, then copy
                uint ldDstElemSize = LdDst->GetElemSize();
                if (ldDstElemSize > 0)
                {
                    CVariable* pTempDst = m_currShader->GetNewVariable(SIZE_OWORD / ldDstElemSize, LdDst->GetType(), m_currShader->getGRFAlignment(), true, 1, CName::NONE);
                    if (useA64)
                        m_encoder->OWLoadA64(pTempDst, pTempSP, SIZE_OWORD);
                    else
                        m_encoder->OWLoad(pTempDst, resource, offsetShr, false, SIZE_OWORD);
                    m_encoder->Push();
                    emitVectorCopy(LdDst, pTempDst, RmnBytes / ldDstElemSize, ArgOffset, 0);
                }
            }
        }
    }
    m_encoder->SetSecondHalf(false);
}

/// Given the data block vector calculated by CalculateStackDataBlocks, for each block
/// write to stack memory starting at (SP - offsetS) up to offsetS bytes.
void EmitPass::WriteStackDataBlocks(StackDataBlocks& blkData, uint offsetS)
{
    // Get current SP
    CVariable* pSP = m_currShader->GetSP();
    bool useA64 = (pSP->GetSize() == 8);

    // Load or store each OWORD block to stack
    for (auto& [Arg, StackOffset, BlkSize, ArgOffset, IsSecondHalf] : blkData)
    {
        // spOffset is a negative offset from SP
        int32_t spOffset = StackOffset - offsetS;
        IGC_ASSERT(spOffset < 0);

        m_encoder->SetSecondHalf(IsSecondHalf);

        // LSC Scatter message
        if (shouldGenerateLSC())
        {
            pSP = ReAlignUniformVariable(pSP, EALIGN_GRF);
            ResourceDescriptor resource;
            resource.m_surfaceType = ESURFACE_STATELESS;
            unsigned blkBits = 64;
            unsigned nBlks = (BlkSize * 8) / 64;
            emitLSCStore(nullptr, Arg, pSP, blkBits, nBlks, ArgOffset, &resource, useA64 ? LSC_ADDR_SIZE_64b : LSC_ADDR_SIZE_32b, LSC_DATA_ORDER_TRANSPOSE, spOffset, 1);
            m_encoder->Push();
        }
        else
        {
            // SP offset for each block
            CVariable* pTempSP = m_currShader->GetNewVariable(pSP);
            emitAddPointer(pTempSP, pSP, m_currShader->ImmToVariable(spOffset, ISA_TYPE_D));

            if (useA64)
                m_encoder->OWStoreA64(Arg, pTempSP, BlkSize, ArgOffset);
            else {
                // SP offset is in units of BYTES, but OWStore requires units of OWORDS
                CVariable* offsetShr = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true, "SPOffset_OW");
                m_encoder->SetSimdSize(SIMDMode::SIMD1);
                m_encoder->SetNoMask();
                m_encoder->SetSrcRegion(0, 0, 1, 0);
                m_encoder->Shr(offsetShr, pTempSP, m_currShader->ImmToVariable(4, ISA_TYPE_UD));
                m_encoder->Push();
                m_encoder->OWStore(Arg, ESURFACE_STATELESS, nullptr, offsetShr, BlkSize, ArgOffset);
            }
            m_encoder->Push();
        }
    }
    m_encoder->SetSecondHalf(false);
}

/// This function reads from or writes to a contiguous block of GRFs.
/// The purpose is to read and write arg/retval data for stackcalls in a consistent manner.
///
/// Possible usages are:
/// 1. Write uniform Src to GRFBlk
/// 2. Write non-uniform (1 or 2 instance) Src to GRFBlk
/// 3. Write 2-instance Struct Src to GRFBlk while reshuffling to SoA layout, ie. S0[16a, 16b], S1[16a, 16b] -> GRFBlk[32a, 32b]
/// 4. Read from GRFBlk to non-uniform (1 or 2 instance) Dst
/// 5. Read from GRFBlk to 2-instance Struct Dst while reshuffling from SoA layout, ie. GRFBlk[32a, 32b] -> S0[16a, 16b], S1[16a, 16b]
///
/// Note we cannot read into a uniform Dst
///
void EmitPass::emitCopyGRFBlock(CVariable* Dst, CVariable* Src, Type* type, uint32_t BlkOffset, unsigned numInstance, bool isWriteToBlk)
{
    VISA_Type dataType = m_currShader->GetType(type);
    uint16_t nElts = (uint16_t)m_currShader->GetNumElts(type, false);
    bool hasSecondHalf = (numInstance > 1);

    if (isWriteToBlk)
    {
        IGC_ASSERT(Dst->GetNumberInstance() == 1);
        IGC_ASSERT(!Dst->IsUniform());
        IGC_ASSERT(dataType == Src->GetType());
        if (BlkOffset != 0 || Dst->GetType() != dataType)
            Dst = m_currShader->GetNewAlias(Dst, dataType, BlkOffset, nElts * numInstance, false);
    }
    else
    {
        IGC_ASSERT(Src->GetNumberInstance() == 1);
        IGC_ASSERT(!Src->IsUniform() && !Dst->IsUniform());
        IGC_ASSERT(dataType == Dst->GetType());
        if (BlkOffset != 0 || Src->GetType() != dataType)
            Src = m_currShader->GetNewAlias(Src, dataType, BlkOffset, nElts * numInstance, false);
    }

    if (!type->isStructTy() || !hasSecondHalf)
    {
        // For regular types and non-instanced structs, we can use emitCopyAll
        // This handles scenarios 1, 2 and 4 above.
        m_encoder->SetSecondHalf(false);
        emitCopyAll(Dst, Src, type);

        // Copy second instance
        if (hasSecondHalf)
        {
            m_encoder->SetSecondHalf(true);
            if (isWriteToBlk)
                Dst = m_currShader->GetNewAlias(Dst, dataType, nElts * m_encoder->GetCISADataTypeSize(dataType), nElts, false);
            else
                Src = m_currShader->GetNewAlias(Src, dataType, nElts * m_encoder->GetCISADataTypeSize(dataType), nElts, false);
            emitCopyAll(Dst, Src, type);
        }
        m_encoder->SetSecondHalf(false);
    }
    else
    {
        // SIMD32 structs are a bit tricky, since we have 2 SIMD16 instances of the struct variable,
        // but we need to copy them either via stack memory or via the ARG/RET GRFs, using the SoA layout.
        // Depending on if we read or write, we need to shuffle the struct data to and from SoA layout.
        IGC_ASSERT(type->isStructTy() && numInstance == 2);
        StructType* STy = cast<StructType>(type);
        const StructLayout* SL = m_DL->getStructLayout(STy);
        unsigned nLanes = numLanes(m_currShader->m_SIMDSize);

        for (unsigned i = 0; i < STy->getNumElements(); i++)
        {
            unsigned elementOffset = (unsigned)SL->getElementOffset(i);
            Type* elementType = STy->getElementType(i);
            uint32_t CopyBytes = (uint32_t)m_DL->getTypeAllocSize(elementType) * nLanes;
            if (isWriteToBlk)
            {
                uint32_t SrcOff = elementOffset * (Src->IsUniform() ? 1 : nLanes);
                uint32_t DstOff = elementOffset * nLanes * 2;
                m_encoder->SetSecondHalf(false);
                emitMayUnalignedVectorCopy(Dst, DstOff, Src, SrcOff, elementType);
                if (hasSecondHalf)
                {
                    m_encoder->SetSecondHalf(true);
                    emitMayUnalignedVectorCopy(Dst, DstOff + CopyBytes, Src, SrcOff, elementType);
                }
            }
            else
            {
                uint32_t SrcOff = elementOffset * nLanes * 2;
                uint32_t DstOff = elementOffset * nLanes;
                m_encoder->SetSecondHalf(false);
                emitMayUnalignedVectorCopy(Dst, DstOff, Src, SrcOff, elementType);
                if (hasSecondHalf)
                {
                    m_encoder->SetSecondHalf(true);
                    emitMayUnalignedVectorCopy(Dst, DstOff, Src, SrcOff + CopyBytes, elementType);
                }
            }
        }
        m_encoder->SetSecondHalf(false);
    }
}


void EmitPass::emitStackCall(llvm::CallInst* inst)
{
    llvm::Function* F = inst->getCalledFunction();

    bool isIndirectFCall = !F || F->hasFnAttribute("referenced-indirectly");
    bool isInvokeSIMDTarget = F && F->hasFnAttribute("invoke_simd_target");
    CVariable* ArgBlkVar = m_currShader->GetARGV();
    uint32_t offsetA = 0;  // visa argument offset
    uint32_t offsetS = 0;  // visa stack offset
    std::vector<CVariable*> argsOnStack;
    SmallVector<std::tuple<CVariable*, Type*, uint32_t>, 8> argsOnRegister;

    IGC_ASSERT(!m_encoder->IsSecondHalf());
    bool hasSecondHalf = (m_currShader->m_numberInstance == 2) && (m_currShader->m_State.m_dispatchSize == SIMDMode::SIMD32);


    // Reserve space on ARGV to pass globally accessible args
    CVariable* GlobalBufferArg = m_currShader->GetGlobalBufferArg();
    if (GlobalBufferArg != nullptr)
    {
        offsetA += GlobalBufferArg->GetSize();
    }
    offsetA += m_currShader->GetARGVReservedVariablesTotalSize();
    IGC_ASSERT(offsetA < ArgBlkVar->GetSize());

    for (uint32_t i = 0; i < IGCLLVM::getNumArgOperands(inst); i++)
    {
        Value* operand = inst->getArgOperand(i);
        CVariable* Src = GetSymbol(operand);
        Type* argType = operand->getType();

        if (!isInvokeSIMDTarget && !isIndirectFCall)
        {
            // Skip unused arguments if any for direct call
            auto argIter = F->arg_begin();
            std::advance(argIter, i);
            if (argIter->use_empty()) continue;
        }

        // adjust offset for alignment
        uint align = getGRFSize();
        offsetA = int_cast<unsigned>(llvm::alignTo(offsetA, align));
        // check if an argument can be written to ARGV based upon offset + arg-size
        unsigned argSize = Src->GetSize() * Src->GetNumberInstance();
        // Uniform invoke simd target must not be vectorized - callee is compiled by vector compiler
        // where the size is set explicitly and it is treated as scalar.
        if (Src->IsUniform() && !isInvokeSIMDTarget)
        {
            argSize = Src->GetSize() * numLanes(m_currShader->m_State.m_dispatchSize);
        }
        bool overflow = ((offsetA + argSize) > ArgBlkVar->GetSize());
        if (!overflow)
        {
            argsOnRegister.push_back(std::make_tuple(Src, argType, offsetA));
            offsetA += argSize;
        }
        else
        {
            // Uniform vars needs to be first vectorized before pushing to stack
            // For 2-instance structs, we need to reshuffle the struct layout to be SoA in memory
            if ((Src->IsUniform() && !isInvokeSIMDTarget) || (argType->isStructTy() && hasSecondHalf))
            {
                uint16_t nElts = (uint16_t)m_currShader->GetNumElts(argType, false);
                unsigned numInstance = hasSecondHalf ? 2 : 1;
                CVariable* SrcVec = m_currShader->GetNewVariable(nElts * numInstance, Src->GetType(), m_currShader->getGRFAlignment(), false, 1, Src->getName());
                emitCopyGRFBlock(SrcVec, Src, argType, 0, numInstance, true);
                Src = SrcVec;
            }
            argsOnStack.push_back(Src);
        }
    }

    uint retSize = 0;
    bool returnOnStack = false;

    if (!inst->getType()->isVoidTy())
    {
        CVariable* RetV = GetSymbol(inst);
        IGC_ASSERT(!RetV->IsUniform());
        retSize = RetV->GetSize() * RetV->GetNumberInstance();
        returnOnStack = retSize > m_currShader->GetRETV()->GetSize();
    }

    unsigned char argSizeInGRF = (offsetA + getGRFSize() - 1) / getGRFSize();
    unsigned char retSizeInGRF = ((returnOnStack ? 0 : retSize) + getGRFSize() - 1) / getGRFSize();

    // Calculate memory offsets for arguments stored on stack
    StackDataBlocks argBlkData;
    offsetS = CalculateStackDataBlocks(argBlkData, argsOnStack);

    // Also allocate stack memory for the return value, aligned to OWORD
    if (returnOnStack)
        offsetS += int_cast<unsigned>(llvm::alignTo(retSize, SIZE_OWORD));

    if (offsetS > 0)
    {
        // Push SP by caller-allocated args + retval bytes
        CVariable* pSP = m_currShader->GetSP();
        CVariable* pPushSize = m_currShader->ImmToVariable(offsetS, ISA_TYPE_UD);
        emitAddPointer(pSP, pSP, pPushSize);

        if (IGC_IS_FLAG_ENABLED(StackOverflowDetection)) {
            emitStackOverflowDetectionCall(inst->getFunction(), false);
        }
    }

    // Write stack arguments
    WriteStackDataBlocks(argBlkData, offsetS);

    // lamda to copy arguments to arg register block
    auto CopyArgBlkVariables = [&]()
    {
        if (GlobalBufferArg != nullptr)
        {
            // Copy any implicit args to the start of ARGV
            CVariable* Dst = m_currShader->GetNewAlias(ArgBlkVar, GlobalBufferArg->GetType(), 0, GlobalBufferArg->GetNumberElement(), GlobalBufferArg->IsUniform());
            m_currShader->CopyVariable(Dst, GlobalBufferArg);
        }
        for (auto& [Src, argType, offset] : argsOnRegister)
        {
            emitCopyGRFBlock(ArgBlkVar, Src, argType, offset, hasSecondHalf ? 2 : 1, true);
        }
    };

    // lambda to read the return value
    auto CopyReturnValue = [&](CallInst* inst)
    {
        // Don't copy if it's not used
        if (inst->use_empty()) return;

        CVariable* Dst = GetSymbol(inst);
        Type* dstTy = inst->getType();
        unsigned numInstance = Dst->GetNumberInstance();
        uint32_t dstSize = Dst->GetSize() * numInstance;
        if (!returnOnStack)
        {
            // Read back value from RETV
            emitCopyGRFBlock(Dst, m_currShader->GetRETV(), dstTy, 0, numInstance, false);
        }
        else
        {
            // Copy from stack
            CVariable* origDst = Dst;
            if (dstTy->isStructTy() && numInstance == 2)
            {
                // For 2-instance structs, we need an extra copy. The struct data is stored in memory as SoA, so we need to first
                // read the data in memory into a temp variable, then reshuffle the data into 2 instances of the struct variable.
                // ie. For S = {a, b}, in memory it looks like {32a, 32b}, we then reshuffle to S0{16a, 16b}, S1{16a, 16b}.
                uint32_t nElts = m_currShader->GetNumElts(dstTy, false) * numInstance;
                Dst = m_currShader->GetNewVariable(nElts, Dst->GetType(), m_currShader->getGRFAlignment(), false, 1, "StructV_SoA");
                IGC_ASSERT(Dst->GetSize() == dstSize);
            }
            StackDataBlocks RetBlkData;
            std::vector<CVariable*> retOnStack = { Dst };
            uint offsetS_R = CalculateStackDataBlocks(RetBlkData, retOnStack);
            IGC_ASSERT(offsetS_R == int_cast<unsigned>(llvm::alignTo(dstSize, SIZE_OWORD)));
            ReadStackDataBlocks(RetBlkData, offsetS_R);
            if (origDst != Dst)
            {
                // Copy from SoA struct to 2-instance struct
                emitCopyGRFBlock(origDst, Dst, dstTy, 0, numInstance, false);
            }
        }
    };

    CVariable* funcAddr = GetSymbol(IGCLLVM::getCalledValue(inst));
    if (!isIndirectFCall || isInvokeSIMDTarget)
    {
        CopyArgBlkVariables();
        m_encoder->StackCall(nullptr, F, argSizeInGRF, retSizeInGRF);
        m_encoder->Push();
        CopyReturnValue(inst);
    }
    else
    {
        if (funcAddr->IsUniform() || IGC_IS_FLAG_ENABLED(AssumeUniformIndirectCall))
        {
            CopyArgBlkVariables();
            funcAddr = TruncatePointer(funcAddr);
            m_encoder->IndirectStackCall(nullptr, funcAddr, argSizeInGRF, retSizeInGRF);
            m_encoder->Push();
            CopyReturnValue(inst);
        }
        else
        {
            // If the call is not uniform, we have to make a uniform call per lane
            // First get the execution mask for active lanes
            CVariable* eMask = GetExecutionMask();
            // Create a label for the loop
            uint label = m_encoder->GetNewLabelID("non_unif_call_body");
            m_encoder->AddLabel(label);
            m_encoder->Push();

            // Get the first active lane's function address
            CVariable* offset = nullptr;
            funcAddr = TruncatePointer(funcAddr, true);
            CVariable* uniformAddr = UniformCopy(funcAddr, offset, eMask);
            // Set the predicate to true for all lanes with the same address
            CVariable* callPred = m_currShader->ImmToVariable(0, ISA_TYPE_BOOL);

            for (unsigned i = 0; i < funcAddr->GetNumberInstance(); i++) {
              m_encoder->SetSecondHalf(i == 1);
              m_encoder->Cmp(EPREDICATE_EQ, callPred, uniformAddr, funcAddr);
              m_encoder->Push();
            }
            m_encoder->SetSecondHalf(false);

            uint callLabel = m_encoder->GetNewLabelID("non_unif_call_end");
            m_encoder->SetInversePredicate(true);
            m_encoder->Jump(callPred, callLabel);
            m_encoder->Push();

            // Copy args to ArgBlk on each iteration of the loop, such that arg registers
            // won't be corrupted by previous iterations.
            CopyArgBlkVariables();

            // Indirect call for all lanes set by the flag
            m_encoder->IndirectStackCall(nullptr, uniformAddr, argSizeInGRF, retSizeInGRF);
            m_encoder->Copy(eMask, eMask);
            m_encoder->Push();

            // For non-uniform call, copy the ret inside this loop so that it'll honor the loop mask
            CopyReturnValue(inst);

            // Label for lanes that skipped the call
            m_encoder->AddLabel(callLabel);
            m_encoder->Push();

            // Unset the bits in execution mask for lanes that were called
            CVariable* callMask = m_currShader->GetNewVariable(1, eMask->GetType(), eMask->GetAlign(), true, CName::NONE);
            CVariable* loopPred = m_currShader->ImmToVariable(0, ISA_TYPE_BOOL);
            m_encoder->Cast(callMask, callPred);
            m_encoder->Not(callMask, callMask);
            m_encoder->And(eMask, eMask, callMask);
            m_encoder->Push();
            m_encoder->SetP(loopPred, eMask);
            m_encoder->Push();

            // Loop while there are bits still left in the mask
            m_encoder->Jump(loopPred, label);
            m_encoder->Push();
        }
    }

    if (offsetS > 0)
    {
        // Set the max stack sized pushed in the parent function for this call's args
        m_encoder->SetFunctionMaxArgumentStackSize(inst->getParent()->getParent(), offsetS);

        //  pop stack pointer after the call for callee arg/retval allocation
        CVariable* pSP = m_currShader->GetSP();
        CVariable* pPopSize = m_currShader->ImmToVariable((uint64_t)(~offsetS + 1), ISA_TYPE_D);
        emitAddPointer(pSP, pSP, pPopSize);
    }
}

static inline bool isFuncSRetArg(Argument * arg)
{
    Function * F = arg->getParent();
    return (arg == F->arg_begin() &&
        arg != F->arg_end() &&
        arg->hasStructRetAttr() &&
        F->getReturnType()->isVoidTy());
}

void EmitPass::emitStackFuncEntry(Function* F)
{
    m_encoder->SetDispatchSimdSize();
    m_currShader->InitializeStackVariables();

    if (F->hasFnAttribute("referenced-indirectly"))
    {
        m_encoder->SetExternFunctionFlag();
    }

    CVariable* ArgBlkVar = m_currShader->GetARGV();
    uint32_t offsetA = 0;  // visa argument offset
    uint32_t offsetS = 0;  // visa stack offset
    std::vector<CVariable*> argsOnStack;

    offsetA += m_currShader->GetARGVReservedVariablesTotalSize();
    IGC_ASSERT(offsetA < ArgBlkVar->GetSize());

    IGC_ASSERT(!m_encoder->IsSecondHalf());
    bool hasSecondHalf = (m_currShader->m_numberInstance == 2) && (m_currShader->m_State.m_dispatchSize == SIMDMode::SIMD32);
    SmallVector<std::tuple<CVariable*, CVariable*, Type*>, 8> StructShuffleVector;

    CVariable* GlobalBufferArg = m_currShader->GetGlobalBufferArg();
    if (GlobalBufferArg != nullptr)
    {
        // Copy any implicit args from the start of ARGV to a local temp
        offsetA += GlobalBufferArg->GetSize();
        CVariable* Src = m_currShader->GetNewAlias(ArgBlkVar, GlobalBufferArg->GetType(), 0, GlobalBufferArg->GetNumberElement(), GlobalBufferArg->IsUniform());
        m_currShader->CopyVariable(GlobalBufferArg, Src);
    }

    for (auto& Arg : F->args())
    {
        if (!F->hasFnAttribute("referenced-indirectly"))
        {
            // Skip unused arguments if any for direct call
            if (Arg.use_empty()) continue;
        }

        // adjust offset for alignment
        CVariable* Dst = m_currShader->getOrCreateArgumentSymbol(&Arg, false, true);
        IGC_ASSERT_MESSAGE(!Dst->IsUniform(), "Function args cannot be uniform!");
        uint align = getGRFSize();
        offsetA = int_cast<unsigned>(llvm::alignTo(offsetA, align));
        unsigned numInstance = Dst->GetNumberInstance();
        uint argSize = Dst->GetSize() * numInstance;

        // check if an argument can be written to ARGV based upon offset + arg-size
        bool overflow = ((offsetA + argSize) > ArgBlkVar->GetSize());
        if (!overflow)
        {
            if (!Arg.use_empty())
            {
                // TODO: Is there a way to map arg directly when numInstances > 1?
                if (m_FGA->isLeafFunc(F) && !hasSecondHalf)
                {
                    // Directly map the dst register to an alias of ArgBlkVar, and update symbol mapping for future uses
                    Dst = m_currShader->GetNewAlias(ArgBlkVar, Dst->GetType(), (uint16_t)offsetA, Dst->GetNumberElement(), Dst->IsUniform());
                    m_currShader->UpdateSymbolMap(&Arg, Dst);
                    Value *val = m_deSSA->getRootValue(&Arg);
                    if (val && dyn_cast<Argument>(val) == nullptr)
                    {
                        // When the leading root value is not an argument, it means the argument has been clobbered through phi nodes.
                        // Create mov for the new CVariable
                        CVariable *var = m_currShader->GetNewVariable(Dst);
                        m_encoder->Copy(var, Dst);
                        m_encoder->Push();
                        m_currShader->UpdateSymbolMap(val, var);
                    }
                }
                else
                {
                    // For calls not guaranteed to preserve the ARG register, we copy it first to a temp
                    emitCopyGRFBlock(Dst, ArgBlkVar, Arg.getType(), offsetA, numInstance, false);
                }
            }
            offsetA += argSize;
        }
        else
        {
            if (Arg.getType()->isStructTy() && numInstance == 2)
            {
                // For 2-instance structs, we need an extra copy. The struct data is stored in memory as SoA, so we need to first
                // read the data in memory into a temp variable, then reshuffle the data into 2 instances of the struct variable.
                // ie. For S = {a, b}, in memory it looks like {32a, 32b}, we then reshuffle to S0{16a, 16b}, S1{16a, 16b}.
                uint32_t nElts = m_currShader->GetNumElts(Arg.getType(), false);
                CVariable* TempDst = m_currShader->GetNewVariable(nElts * numInstance, Dst->GetType(), m_currShader->getGRFAlignment(), false, 1, "StructV_SoA");
                IGC_ASSERT(TempDst->GetSize() == argSize);
                StructShuffleVector.push_back(std::make_tuple(Dst, TempDst, Arg.getType()));
                Dst = TempDst;
            }
            argsOnStack.push_back(Dst);
        }

        // Get the symbol for arg0 if it has the "sret" attribute and save it.
        if (isFuncSRetArg(&Arg)) m_currShader->SaveSRet(Dst);
    }
    m_encoder->SetStackFunctionArgSize((offsetA + getGRFSize() - 1) / getGRFSize());

    // Calculate offsets of args written on stack
    StackDataBlocks argBlkData;
    offsetS = CalculateStackDataBlocks(argBlkData, argsOnStack);

    // Also take into account the return value if it was passed on stack
    if (!F->getReturnType()->isVoidTy())
    {
        CVariable* RetVal = m_currShader->getOrCreateReturnSymbol(F);
        bool isRetUniform = RetVal->IsUniform();
        unsigned numInstance = RetVal->GetNumberInstance();
        unsigned RetSize = isRetUniform ? RetVal->GetSize() * numLanes(m_currShader->m_State.m_dispatchSize) : RetVal->GetSize() * numInstance;
        if (RetSize > m_currShader->GetRETV()->GetSize())
            offsetS += int_cast<unsigned>(llvm::alignTo(RetSize, SIZE_OWORD));
    }

    // Read from caller stack back into registers
    ReadStackDataBlocks(argBlkData, offsetS);

    // Reshuffle any struct args from SoA to 2-instance variables for SIMD32
    for (auto& [Dst, Src, argTy] : StructShuffleVector)
    {
        emitCopyGRFBlock(Dst, Src, argTy, 0, Dst->GetNumberInstance(), false);
    }

    unsigned totalAllocaSize = 0;

    // reserve space for all the alloca in the function subgroup
    auto funcMDItr = m_currShader->m_ModuleMetadata->FuncMD.find(F);
    if (funcMDItr != m_currShader->m_ModuleMetadata->FuncMD.end())
    {
        totalAllocaSize += funcMDItr->second.privateMemoryPerWI * numLanes(m_currShader->m_State.m_dispatchSize);
        totalAllocaSize += funcMDItr->second.prevFPOffset;
    }

    // save FP before allocation
    m_currShader->SaveStackState();

    // Push a new stack frame
    emitPushFrameToStack(F, totalAllocaSize);

    // Set the per-function private mem size
    m_encoder->SetFunctionAllocaStackSize(F, totalAllocaSize);
}

void EmitPass::emitStackFuncExit(llvm::ReturnInst* inst)
{
    // restore SP and FP
    m_currShader->RestoreStackState();

    llvm::Function* F = inst->getParent()->getParent();
    llvm::Type* RetTy = F->getReturnType();
    CVariable* RETV = m_currShader->GetRETV();
    if (!RetTy->isVoidTy())
    {
        unsigned nLanes = numLanes(m_currShader->m_SIMDSize);
        CVariable* Src = GetSymbol(inst->getReturnValue());
        bool isSrcUniform = Src->IsUniform();
        unsigned RetSize = isSrcUniform ? Src->GetSize() * nLanes : Src->GetSize() * Src->GetNumberInstance();

        IGC_ASSERT(!m_encoder->IsSecondHalf());
        bool hasSecondHalf = (m_currShader->m_numberInstance == 2) && (m_currShader->m_State.m_dispatchSize == SIMDMode::SIMD32);
        unsigned numInstance = hasSecondHalf ? 2 : 1;

        if (RetSize <= RETV->GetSize())
        {
            // Return on GRF by writing to the RETV register
            emitCopyGRFBlock(RETV, Src, RetTy, 0, numInstance, true);
            m_encoder->SetStackFunctionRetSize((RetSize + getGRFSize() - 1) / getGRFSize());
        }
        else
        {
            // Return on Stack
            // For uniform var, vectorize then push to stack
            // For 2-instance structs, need to reshuffle layout to SoA
            if (isSrcUniform || (RetTy->isStructTy() && hasSecondHalf))
            {
                uint32_t nElts = m_currShader->GetNumElts(RetTy, false);
                CVariable* RetTemp = m_currShader->GetNewVariable(nElts * numInstance, Src->GetType(), m_currShader->getGRFAlignment(), false, 1, "RetVec");
                emitCopyGRFBlock(RetTemp, Src, RetTy, 0, numInstance, true);
                Src = RetTemp;
            }

            StackDataBlocks RetBlkData;
            std::vector<CVariable*> retOnStack = { Src };
            uint offsetS = CalculateStackDataBlocks(RetBlkData, retOnStack);
            IGC_ASSERT(offsetS == int_cast<unsigned>(llvm::alignTo(RetSize, SIZE_OWORD)));

            // Callee's stack frame is already popped, so SP should be pointing to the top of Caller's stack.
            // Write return value to SP - offsetS, which is allocated by caller for retval.
            WriteStackDataBlocks(RetBlkData, offsetS);

            m_encoder->SetStackFunctionRetSize(0);
        }
    }
    else
    {
        // Based on other arch's ABIs, the sret argument is guaranteed to be written to the return register upon function exit.
        // vISA ABI states that the return and argument registers start at the same location. If the function is non-void, %retVal
        // starts at r26. Otherwise, %arg0 will start at r26.
        // Here we write the saved arg0 value back into arg0. Since arg0 has the "sret" attribute, the function is guaranteed to be void,
        // thus writing to %arg0 is the same as writing to %retval.
        // We still set the retSize to 0 to match the LLVM IR function signature, so we avoid writing to vISA's return reg directly.
        // Note: For leaf functions, we don't need to copy since we are guaranteed that %arg0 will not be overwritten.
        CVariable* sretPtr = m_currShader->GetAndResetSRet();
        if (sretPtr && isFuncSRetArg(F->arg_begin()) && !m_FGA->isLeafFunc(F))
        {
            // If the sret value is saved, copy it back into arg0
            CVariable* ArgBlk = m_currShader->GetARGV();
            emitCopyGRFBlock(ArgBlk, sretPtr, F->arg_begin()->getType(), 0, sretPtr->GetNumberInstance(), true);
        }
        m_encoder->SetStackFunctionRetSize(0);
    }
    // emit return
    m_encoder->StackRet(nullptr);
    m_encoder->Push();
}

void EmitPass::emitSymbolRelocation(Function& F)
{
    Module* pModule = F.getParent();

    SmallSetVector<Function*, 16> funcAddrSymbols;
    SmallSetVector<GlobalVariable*, 16> globalAddrSymbols;

    ModuleMetaData* moduleMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

    std::function<void(Value*)> usedValues =
        [&usedValues, &funcAddrSymbols, &globalAddrSymbols, moduleMD, pModule]
    (Value* v)
    {
        if (Function* pFunc = dyn_cast<Function>(v))
        {
            if (pModule == pFunc->getParent() &&
                pFunc->hasFnAttribute("referenced-indirectly"))
                funcAddrSymbols.insert(pFunc);
        }
        else if (GlobalVariable* pGlobal = dyn_cast<GlobalVariable>(v))
        {
            if (pModule == pGlobal->getParent() &&
                moduleMD->inlineProgramScopeOffsets.count(pGlobal) > 0)
                globalAddrSymbols.insert(pGlobal);
        }
        else if (Constant* C = dyn_cast<Constant>(v))
        {
            for (auto it = C->value_op_begin(), end = C->value_op_end(); it != end; it++)
                usedValues(*it);
        }
    };

    for (auto&& BB : F)
    {
        for (auto& I : BB)
        {
            for (auto it = I.value_op_begin(), end = I.value_op_end(); it != end; it++)
                usedValues(*it);
        }
    }

    for (auto pFunc : funcAddrSymbols)
    {
        m_currShader->CreateFunctionSymbol(pFunc);
    }

    for (auto pGlobal : globalAddrSymbols)
    {
        m_currShader->CreateGlobalSymbol(pGlobal);
    }
}

void EmitPass::emitStoreRawIndexed(
    StoreRawIntrinsic* inst, Value* varOffset, ConstantInt* immOffset)
{
    Value* pBufPtr = inst->getResourceValue();
    Value* pValToStore = inst->getStoreValue();

    m_currShader->m_State.isMessageTargetDataCacheDataPort = true;

    if (shouldGenerateLSC(inst))
    {
        LSC_DOC_ADDR_SPACE addrSpace = m_pCtx->getUserAddrSpaceMD().Get(inst);

        LSC_CACHE_OPTS cacheOpts =
            translateLSCCacheControlsFromMetadata(inst, false);
        emitLSCVectorStore(
            pBufPtr,
            varOffset,
            immOffset,
            nullptr,
            pValToStore,
            inst->getParent(),
            cacheOpts,
            inst->getAlignment(),
            false,
            addrSpace
        );
        return;
    }
    IGC_ASSERT(immOffset == nullptr);
    emitStore3DInner(pValToStore, pBufPtr, varOffset);
}

void EmitPass::emitStore3DInner(Value* pllValToStore, Value* pllDstPtr, Value* pllElmIdx)
{
    IGC_ASSERT(pllDstPtr != nullptr);

    bool isPrivateMem = pllDstPtr->getType()->getPointerAddressSpace() == ADDRESS_SPACE_PRIVATE;
    if (!isPrivateMem)
    {
        ForceDMask(false);
    }

    ResourceDescriptor resource = GetResourceVariable(pllDstPtr);

    uint sizeInBits = GetPrimitiveTypeSizeInRegisterInBits(pllValToStore->getType());
    Type* storeType = pllValToStore->getType();

    IGC_ASSERT_MESSAGE((sizeInBits == 8) || (sizeInBits == 16) || (sizeInBits == 32) || (sizeInBits == 64) || (sizeInBits == 96) || (sizeInBits == 128),
        "Stored type must be 1/2/4/8/12/16 bytes long");

    CVariable* storedVal = GetSymbol(pllValToStore);

    IGC_ASSERT(pllElmIdx);
    CVariable* ptr = GetSymbol(pllElmIdx);

    IGC_ASSERT(pllDstPtr->getType()->isPointerTy());
    if (!IGC::isA64Ptr(cast<PointerType>(pllDstPtr->getType()), m_currShader->GetContext()))
    {
        ptr = TruncatePointer(ptr);
    }

    CVariable* gOffset = m_currShader->ImmToVariable(0x0, ISA_TYPE_UD);

    // The stored value and the ptr must be placed aligned in GRFs, as SIMDSize DWORDs.
    // So if it's not already in this form, bring it to it:
    // Broadcast the value, and extend it (doesn't matter if it's sext, zext, or any
    // other kind of extend).

    CVariable* storedValOriginal = storedVal;
    CVariable* ptrOriginal = ptr;

    storedVal = BroadcastIfUniform(storedVal);
    ptr = BroadcastIfUniform(ptr);

    uint label = 0;
    CVariable* flag = nullptr;
    bool needLoop = ResourceLoopHeader(resource, flag, label);
    ResourceLoopSubIteration(resource, flag, label);
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
            setPredicateForDiscard(flag);
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
            if (m_currShader->m_Platform->emulateByteScraterMsgForSS() &&
                (ESURFACE_SCRATCH == resource.m_surfaceType))
            {
                bool isUniformInst = (ptrOriginal->IsUniform() && storedValOriginal->IsUniform());
                ptrOriginal = (isUniformInst ? ReAlignUniformVariable(ptrOriginal, EALIGN_GRF) : ptr);
                storedValOriginal = (isUniformInst ? ReAlignUniformVariable(storedValOriginal, EALIGN_GRF) : storedVal);
                m_encoder->Scatter4Scaled(storedValOriginal, resource, ptrOriginal);
            }
            else
            {
                // using byte scatter
                uint elementSize = 8;
                uint numElems = 4;
                setPredicateForDiscard(flag);
                m_encoder->ByteScatter(
                    storedVal,
                    resource,
                    ptr,
                    elementSize,
                    numElems);
            }
            m_encoder->Push();
        }
    }
    else if (sizeInBits == 16 || sizeInBits == 8)
    {
        // using byte scatter
        uint elementSize = 8;
        uint numElems = sizeInBits / 8;
        VISA_Type elementType = (sizeInBits == 8) ? ISA_TYPE_UB : ISA_TYPE_UW;
        CVariable* val = m_currShader->GetNewVariable(
            numLanes(m_currShader->m_SIMDSize), ISA_TYPE_UD, EALIGN_GRF, CName::NONE);
        storedVal = m_currShader->GetNewAlias(storedVal, elementType, 0, 0);
        m_encoder->Cast(val, storedVal);
        // no need for discard predicate if we are writing to scratch - this is our
        // internal memory, shader output remain the same, but we avoid problems
        // when, for example, texture coordinates are spilled
        if (resource.m_surfaceType != ESURFACE_SCRATCH)
        {
            setPredicateForDiscard(flag);
        }
        m_encoder->ByteScatter(
            val,
            resource,
            ptr,
            elementSize,
            numElems);
        m_encoder->Push();
    }
    else if (sizeInBits == 64 && storeType->getScalarType()->getPrimitiveSizeInBits() == 64)
    {
        // QW scatter
        IGCLLVM::FixedVectorType* vecType = dyn_cast<IGCLLVM::FixedVectorType>(storeType);
        uint32_t numElems = (uint32_t)(vecType ? vecType->getNumElements() : 1);
        IGC_ASSERT_MESSAGE(numElems == 1, "QW vector store not supported yet!");
        // no need for discard predicate if we are writing to scratch - this is our
        // internal memory, shader output remain the same, but we avoid problems
        // when, for example, texture coordinates are spilled
        if (resource.m_surfaceType != ESURFACE_SCRATCH)
        {
            setPredicateForDiscard(flag);
        }
        m_encoder->QWScatter(storedVal, resource, ptr, sizeInBits, numElems);
        m_encoder->Push();
    }
    else  // (sizeInBits > 32)
    {
        // no need for discard predicate if we are writing to scratch - this is our
        // internal memory, shader output remain the same, but we avoid problems
        // when, for example, texture coordinates are spilled
        if (resource.m_surfaceType != ESURFACE_SCRATCH)
        {
            setPredicateForDiscard(flag);
        }
        m_encoder->Scatter4Scaled(storedVal, resource, ptr);
        m_encoder->Push();
    }
    ResourceLoopBackEdge(needLoop, flag, label);
    if (!isPrivateMem)
    {
        ResetVMask(false);
    }
}

void EmitPass::emitStore(
    StoreInst *inst,
    Value *varOffset,
    ConstantInt *immOffset,
    ConstantInt *immScale
) {
    if (shouldGenerateLSC(inst))
    {
        LSC_DOC_ADDR_SPACE addrSpace = m_pCtx->getUserAddrSpaceMD().Get(inst);

        LSC_CACHE_OPTS cacheOpts =
            translateLSCCacheControlsFromMetadata(inst, false);
        emitLSCVectorStore(
            inst->getPointerOperand(),
            varOffset,
            immOffset,
            immScale,
            inst->getValueOperand(),
            inst->getParent(),
            cacheOpts,
            IGCLLVM::getAlignmentValue(inst),
            inst->getMetadata("enable.vmask"),
            addrSpace
          );
        return;
    }
    IGC_ASSERT_MESSAGE(immScale ? immScale->getSExtValue() == 1 : true,
        "Immediate Scale not supported on non-LSC path!");
    emitVectorStore(inst, varOffset, immOffset);
}

CVariable* EmitPass::GetSymbol(llvm::Value* v) const
{
    return m_currShader->GetSymbol(v);
}

void EmitPass::CountStatelessIndirectAccess(llvm::Value* pointer, ResourceDescriptor resource)
{
    instrMap.clear();
    IGC_ASSERT_MESSAGE(isa<PointerType>(pointer->getType()), "Value should be a pointer");
    if (resource.m_surfaceType == ESURFACE_STATELESS && IsIndirectAccess(pointer))
    {
        m_currShader->IncIndirectStatelessCount();
    }
}

/// IMPORTANT!!!
/// Please bump up INDIRECT_ACCESS_DETECTION_VERSION when introducing any functional fixes in below function.
bool EmitPass::IsIndirectAccess(llvm::Value* pointer)
{
    Instruction* inst = dyn_cast<Instruction>(pointer);
    if (inst == nullptr)
    {
        return false;
    }

    // we cache the instructions
    // when we meet the instruction again know it has already been checked
    if (instrMap.count(inst))
    {
        return instrMap.lookup(inst);
    }

    bool isIndirect = false;
    instrMap.try_emplace(inst, isIndirect);

    if (LoadInst* loadInst = dyn_cast<LoadInst>(inst))
    {
        isIndirect = true;
    }
    else if (CallInst* callInstr = dyn_cast<CallInst>(inst))
    {
        // if the call instruction isn't intrinsic we assume that it should be indirect
        // because intrinsic is rather the simple arithmetic
        GenIntrinsicInst* pIntrinsic = dyn_cast<GenIntrinsicInst>(callInstr);
        if (pIntrinsic == nullptr)
        {
            isIndirect = true;
        }
        else
        {
            // If this is intrinsic, check if this is memory access.
            isIndirect = IsMemLoadIntrinsic(pIntrinsic->getIntrinsicID());
        }
    }

    if (!isIndirect)
    {
        for (unsigned int i = 0; i < inst->getNumOperands(); i++)
        {
            if (IsIndirectAccess(inst->getOperand(i)))
            {
                isIndirect = true;
                break;
            }
        }
    }
    instrMap.insert(std::make_pair(inst, isIndirect));
    return isIndirect;
}

void EmitPass::emitInsert(llvm::Instruction* inst)
{
    auto IEI = llvm::cast<llvm::InsertElementInst>(inst);
    // Skip emit scalar copy if this `insertelement` could be aliased.
    if (m_currShader->CanTreatScalarSourceAsAlias(IEI))
        return;

    llvm::Type* eTy = inst->getOperand(1)->getType();
    // Do not use allocated type to compute the offsets; otherwise the computed
    // offsets may be out-of-bound. The alignment information of the base
    // element type should not impact the offset.
    uint32_t eBytes = GetScalarTypeSizeInRegister(eTy);
    IGC_ASSERT_MESSAGE(eBytes, "illegal InsertElementInst instruction");

    llvm::Value* pVec = inst->getOperand(0);
    CVariable* pInstVar = GetSymbol(inst);
    CVariable* pVecVar = nullptr;
    llvm::Type* pVecType = inst->getType();
    if (!isa<UndefValue>(pVec))
    {
        if (isa<ConstantVector>(pVec))
        {
            auto CV = cast<ConstantVector>(pVec);
            pInstVar = m_currShader->GetConstant(CV, pInstVar);
        }
        else
        {
            pVecVar = GetSymbol(pVec);
            if (pVecVar != pInstVar)
            {
                emitVectorCopy(pInstVar, pVecVar, int_cast<unsigned>(dyn_cast<IGCLLVM::FixedVectorType>(pVecType)->getNumElements()));
            }
        }
    }

    if (llvm::ConstantInt * pConstElem = llvm::dyn_cast<llvm::ConstantInt>(IEI->getOperand(2)))
    {
        CVariable* pElm = GetSymbol(inst->getOperand(1));

        uint element = int_cast<uint>(pConstElem->getZExtValue());
        uint eStartBytes;
        if (m_currShader->GetIsUniform(inst) && m_currShader->GetIsUniform(pVec))
        {
            eStartBytes = eBytes * element;
        }
        else
        {
            eStartBytes = numLanes(m_currShader->m_SIMDSize) * eBytes * element;
        }

        uint subVar = (eStartBytes / getGRFSize());
        uint subReg = (eStartBytes % getGRFSize()) / eBytes; // unit of element(eTy)
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
        CVariable* pElemVar = GetSymbol(pElement);

        // size of vector entry
        const uint vectorEntrySimdWidth = pInstVar->IsUniform() ?
            1 : numLanes(m_currShader->m_SIMDSize);

        const uint vecTypeSize =
            GetPrimitiveTypeSizeInRegister(cast<VectorType>(pVecType)->getElementType());

        const uint offset = vectorEntrySimdWidth * vecTypeSize;

        CVariable* pOffset1 = m_currShader->ImmToVariable(offset, ISA_TYPE_UW);

        // offset2 = index * sizeof(vector entry)  <-- offset within the vector counted in bytes
        CVariable* pOffset2 = m_currShader->GetNewVariable(
            pIndexVar->IsUniform() ? 1 : numLanes(m_currShader->m_SIMDSize),
            ISA_TYPE_UW,
            EALIGN_WORD,
            pIndexVar->IsUniform(), CName::NONE);

        if (!pIndexVar->IsUniform())
        {
            m_encoder->SetSrcRegion(0, 16, 8, 2);
        }
        m_encoder->Mul(pOffset2, pIndexVar, pOffset1);
        m_encoder->Push();

        // a0 = addressof(vector variable) + offset2 <-- address of element to insert at
        if (pIndexVar->IsUniform())
        {
            CVariable* pDstArrElm =
                m_currShader->GetNewAddressVariable(
                    1,
                    m_destination->GetType(),
                    true,
                    pInstVar->IsUniform(),
                    m_destination->getName());
            m_encoder->AddrAdd(pDstArrElm, m_destination, pOffset2);
            m_encoder->Push();
            m_encoder->Copy(pDstArrElm, pElemVar);
            m_encoder->Push();
        }
        else
        {
            // Lower execution size to avoid complains of indirectly addressing across more than two GRFs.
            // One example is below:
            //(W)     mov (1|M0)              f1.1<1>:uw    0x100:uw
            //(f1.1)  mov(16|M0)              r[a0.8]<1>:f  r63.0 < 0; 1, 0 >:f
            //will be changed to
            //(W)     mov (1|M0)              f1.1<1>:uw    0x100:uw
            //(f1.1)  mov(8|M8)              r[a0.8+0x20]<1>:f  r63.0 < 0; 1, 0 >:f
            // To avoid complains, we limit the execSizeNew*datatypesize to the same memory size of getMinDispatchMode()
            // In above example, say, getMinDispatchMode()==8, that means the execSizeNew should be 8
            // because 8 * SIZE_DWORD = getMinDispatchMode() * SIZE_DWORD
            // But if datatype is 64bit, then, execSizeNew should be 4
            // because 4 * SIZE_QWORD = getMinDispatchMode() * SIZE_DWORD
            // Changing to simd1 needs more work and might cause extra overhead as well.
            // indirect address, emaskoffset should be offsetted correspondingly
            SIMDMode simdMode = std::min(m_currShader->m_SIMDSize, SIMDMode::SIMD16);
            SIMDMode minDispatchMode = m_currShader->m_Platform->getMinDispatchMode();
            SIMDMode execSizeNew = minDispatchMode;

            bool hasSecondHalf = (m_currShader->m_State.m_dispatchSize == SIMDMode::SIMD32) && (m_currShader->m_numberInstance == 1);
            bool bWAMultiGRF = false;
            if (!pInstVar->IsUniform() && m_currShader->m_Platform->enableMultiGRFAccessWA())
            {
                uint32_t dataTypeSize = GetScalarTypeSizeInRegisterInBits(pElement->getType());
                uint32_t memSizeToUse = numLanes(simdMode) * dataTypeSize / 8;
                uint32_t memSizeMinDisp = numLanes(minDispatchMode) * SIZE_DWORD;
                bWAMultiGRF = (memSizeToUse > memSizeMinDisp);
                if (bWAMultiGRF)
                {
                    execSizeNew = lanesToSIMDMode(memSizeMinDisp * 8 / dataTypeSize);
                    uint32_t lanesNew = numLanes(execSizeNew);

                    auto generateAddforAddresRegister = [&](uint32_t laneIdx, e_mask mask, uint32_t imm)
                    {
                        CVariable* pOffset1_2ndHalf = m_currShader->ImmToVariable(imm, ISA_TYPE_UW);
                        CVariable* pOffset2_2ndHalf = m_currShader->GetNewAlias(pOffset2, ISA_TYPE_UW, laneIdx * SIZE_WORD, 0);
                        m_encoder->SetSrcRegion(0, lanesNew, lanesNew, 1);
                        m_encoder->SetSimdSize(execSizeNew);
                        m_encoder->SetMask(mask);
                        m_encoder->SetSecondNibble((laneIdx / 4) % 2 ? true : false);
                        m_encoder->Add(pOffset2_2ndHalf, pOffset2_2ndHalf, pOffset1_2ndHalf);
                        m_encoder->Push();
                    };

                    int cnt = memSizeToUse / memSizeMinDisp;
                    for (int i=1; i<cnt; i++)
                    {
                        uint32_t laneIdx = lanesNew * i;
                        e_mask mask = (laneIdx / 8) % 2 ? EMASK_Q2 : EMASK_Q1;
                        generateAddforAddresRegister(laneIdx, mask, memSizeMinDisp * i);
                    }

                    // When datatype is 64 bits and hasSecondHalf is true it should be another "add" generated to corretly
                    // create same offset for second loopCount iteration.
                    if (dataTypeSize == 64 && hasSecondHalf)
                    {
                        generateAddforAddresRegister(lanesNew * 3, EMASK_Q4, memSizeMinDisp);
                    }
                    m_encoder->SetSecondNibble(false);
                }
            }

            int loopCount = hasSecondHalf ? 2 : 1;
            for (int i = 0; i < loopCount; ++i)
            {
                CVariable* dst = m_destination;
                if (i == 1)
                {
                    IGC_ASSERT_MESSAGE(getGRFSize() == 64, "This code should execute for 64 byte GRF register size device");
                    // explicitly set second half as we are manually splitting
                    m_encoder->SetSecondHalf(true);
                    m_encoder->SetSrcSubReg(1, 16);
                    dst = m_currShader->GetNewAlias(dst, dst->GetType(), 16 * dst->GetElemSize(), 0);
                }
                CVariable* pDstArrElm = m_currShader->GetNewAddressVariable(
                    numLanes(simdMode),
                    m_destination->GetType(),
                    false,
                    pInstVar->IsUniform(),
                    m_destination->getName());

                m_encoder->SetSimdSize(simdMode);
                m_encoder->AddrAdd(pDstArrElm, dst, pOffset2);
                m_encoder->Push();

                // Handle the case when the index is non-uniform - we need to lookup a different value
                // for each simd lane.
                // Since HW doesn't support scattered GRF writes, we need to simulate
                // scattered write by a sequence of instructions, each one writing to a single simd-lane.
                for (uint lane = 0; lane < numLanes(simdMode); ++lane)
                {
                    uint position = lane + i * 16;
                    // write to uniform-vector has no-mask and no-predicate
                    if (!pInstVar->IsUniform())
                    {
                        CVariable* immMask = m_currShader->ImmToVariable(1ULL << lane, ISA_TYPE_UD);
                        CVariable* dstPred = m_currShader->GetNewVariable(
                            numLanes(m_SimdMode),
                            ISA_TYPE_BOOL,
                            EALIGN_BYTE,
                            CName::NONE);

                        m_encoder->SetSimdSize(simdMode);
                        m_encoder->SetP(dstPred, immMask);
                        m_encoder->Push();
                        m_encoder->SetPredicate(dstPred);
                    }
                    if (!pElemVar->IsUniform())
                    {
                        m_encoder->SetSrcSubReg(0, position);
                    }
                    m_encoder->SetSrcRegion(0, 0, 1, 0);
                    m_encoder->SetDstSubReg(lane);
                    if (bWAMultiGRF)
                    {
                        m_encoder->SetMask((lane / 8) % 2 ? EMASK_Q2 : EMASK_Q1);
                        if (execSizeNew == SIMDMode::SIMD4)
                        {
                            m_encoder->SetSecondNibble((lane / 4) % 2 ? true : false);
                        }
                        m_encoder->SetSimdSize(execSizeNew);
                    }
                    else if (pInstVar->IsUniform())
                    {
                        m_encoder->SetSimdSize(SIMDMode::SIMD1);
                        m_encoder->SetNoMask();
                    }
                    else
                    {
                        m_encoder->SetSimdSize(simdMode);
                    }
                    m_encoder->Copy(pDstArrElm, pElemVar);
                    m_encoder->Push();
                    m_encoder->SetSecondNibble(false);
                }
            }
        }
    }
}

void EmitPass::emitBranch(llvm::BranchInst* branch, const SSource& cond, e_predMode predMode)
{
    llvm::BasicBlock* next = m_blockCoalescing->SkipEmptyBasicBlock(branch->getParent()->getNextNode());
    if (branch->isConditional())
    {
        CVariable* flag = GetSrcVariable(cond);
        bool inversePred = cond.mod == EMOD_NOT;;
        // if it is not a fallthrough
        BasicBlock* succ0 = m_blockCoalescing->FollowEmptyBlock(branch->getSuccessor(0));
        BasicBlock* succ1 = m_blockCoalescing->FollowEmptyBlock(branch->getSuccessor(1));
        uint label0 = m_pattern->GetBlockId(succ0);
        uint label1 = m_pattern->GetBlockId(succ1);

        m_encoder->SetPredicateMode(predMode);
        m_encoder->SetInversePredicate(inversePred);

        if (next == NULL || (next != succ0 && next != succ1))
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
            uint label = m_pattern->GetBlockId(branch->getParent());
            uint condTarget, uncondTarget;
            if ((label0 <= label && label1 <= label) || (label0 > label && label1 > label))
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

            if (condTarget == uncondTarget)
            {   // sanity check. label0 == label1 (we don't expect it, but it's legal)
                m_encoder->Jump(condTarget);
                m_encoder->Push();
            }
            else
            {
                if (condTarget != label0)
                {
                    m_encoder->SetInversePredicate(!inversePred);
                }
                m_encoder->Jump(flag, condTarget);
                m_encoder->Push();

                m_encoder->Jump(uncondTarget);
                m_encoder->Push();
            }
        }
        else if (next != succ0)
        {
            IGC_ASSERT_MESSAGE(next == succ1, "next should be succ1!");

            m_encoder->Jump(flag, label0);
            m_encoder->Push();
        }
        else
        {
            IGC_ASSERT_MESSAGE(next == succ0, "next should be succ0");

            m_encoder->SetInversePredicate(!inversePred);
            m_encoder->Jump(flag, label1);
            m_encoder->Push();
        }
    }
    else
    {
        BasicBlock* succ = m_blockCoalescing->FollowEmptyBlock(branch->getSuccessor(0));
        if ((next == NULL) || (next != succ))
        {
            uint label = m_pattern->GetBlockId(succ);
            m_encoder->Jump(label);
            m_encoder->Push();
        }
    }
}

void EmitPass::emitDiscardBranch(
    BranchInst* branch, const SSource& cond)
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
    for (uint i = 0; i < numSources; ++i)
    {
        const unsigned int GRFSizeBy4 = (getGRFSize() >> 2);
        IGC_ASSERT(GRFSizeBy4);

        uint subVarIdx = numLanes(mode) / GRFSizeBy4 * i + headerSize;

        CVariable* rawDst = payload;
        CVariable* src = GetSymbol(inst->getOperand(i));
        // The source have to match for a raw copy
        if (src->GetType() != payload->GetType())
        {
            rawDst = m_currShader->BitCast(payload, src->GetType());
        }
        m_encoder->SetSimdSize(mode);
        m_encoder->SetDstSubVar(subVarIdx);
        m_encoder->SetSrcSubVar(0, half);
        m_encoder->SetMask(half == 0 ? EMASK_Q1 : EMASK_Q2);
        m_encoder->Copy(rawDst, src);
        m_encoder->Push();
    }
}

template<size_t N>
void EmitPass::JoinSIMD(CVariable* (&tempdst)[N], uint responseLength, SIMDMode mode)
{
    auto origMode = mode == SIMDMode::SIMD8 ? SIMDMode::SIMD16 : SIMDMode::SIMD32;
    uint iterationCount = numLanes(m_currShader->m_SIMDSize) / numLanes(mode);
    for (uint half = 0; half < iterationCount; half++)
    {
        for (uint i = 0; i < responseLength; ++i)
        {
            const unsigned int GRFSizeBy4 = (getGRFSize() >> 2);
            IGC_ASSERT(GRFSizeBy4);
            m_encoder->SetSimdSize(mode);
            const unsigned int subVarIdx = numLanes(origMode) / GRFSizeBy4 * i;
            m_encoder->SetSrcSubVar(0, i);
            m_encoder->SetDstSubVar(subVarIdx + half);
            m_encoder->SetMask(half == 0 ? (mode == SIMDMode::SIMD8 ? EMASK_Q1 : EMASK_H1) :
                (mode == SIMDMode::SIMD8 ? EMASK_Q2 : EMASK_H2));
            IGC_ASSERT(half < ARRAY_COUNT(tempdst));
            m_encoder->Copy(m_destination, tempdst[half]);
            m_encoder->Push();
        }
    }
}

CVariable* EmitPass::BroadcastIfUniform(CVariable* pVar, bool nomask)
{
    IGC_ASSERT_MESSAGE(nullptr != pVar, "pVar is null");
    VISA_Type VarT = pVar->GetType();
    bool Need64BitEmu = m_currShader->m_Platform->hasNoFullI64Support() &&
        (VarT == ISA_TYPE_Q || VarT == ISA_TYPE_UQ);
    bool IsImm = pVar->IsImmediate();
    if (pVar->IsUniform())
    {
        uint32_t width = numLanes(m_currShader->m_SIMDSize);
        uint elts = IsImm ? 1 : pVar->GetNumberElement();
        CVariable* pBroadcast =
            m_currShader->GetNewVariable(elts * width, pVar->GetType(),
                EALIGN_GRF, CName(pVar->getName(), "Broadcast"));
        CVariable* Dst = pBroadcast;
        CVariable* Src = pVar;
        CVariable* ImmLo = nullptr, * ImmHi = nullptr;
        unsigned Stride = 1;
        if (Need64BitEmu) {
            Dst = m_currShader->GetNewAlias(pBroadcast, ISA_TYPE_UD, 0, 0);
            if (IsImm) {
                uint64_t Imm = pVar->GetImmediateValue();
                ImmLo = m_currShader->ImmToVariable(Imm & 0xFFFFFFFFULL, ISA_TYPE_UD);
                ImmHi = m_currShader->ImmToVariable(Imm >> 32, ISA_TYPE_UD);
            }
            else {
                Src = m_currShader->GetNewAlias(pVar, ISA_TYPE_UD, 0, 0);
            }
            Stride = 2;
        }

        for (uint i = 0; i < elts; ++i)
        {
            if (nomask)
                m_encoder->SetNoMask();
            m_encoder->SetSrcSubReg(0, i * Stride);
            if (Stride != 1) m_encoder->SetDstRegion(Stride);
            m_encoder->SetDstSubReg((i * Stride) * width);
            m_encoder->Copy(Dst, ImmLo ? ImmLo : Src);
            m_encoder->Push();
            if (Need64BitEmu) {
                if (nomask)
                    m_encoder->SetNoMask();
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

CVariable* EmitPass::tryReusingXYZWPayload(Value* storedVal, BasicBlock* BB,
    unsigned numElems, VISA_Type type, CVariable* pSrc_X, CVariable* pSrc_Y,
    CVariable* pSrc_Z, CVariable* pSrc_W, const unsigned int numEltGRF)
{
    if (IGC_IS_FLAG_DISABLED(EnableReusingXYZWStoreConstPayload))
        return nullptr;

    Constant* constantStoredVal = dyn_cast<Constant>(storedVal);
    if (!constantStoredVal ||
        !(constantStoredVal->getType()->isFPOrFPVectorTy() ||
          constantStoredVal->getType()->isIntOrIntVectorTy()))
    {
        return nullptr;
    }

    ConstVectorStoreData& storeData = m_constantVectorStores[constantStoredVal];
    if (storeData.BB != BB)
    {
        // Make an entry of this store data with BB in the constant vector stores
        storeData = {
            m_currShader->GetNewVariable(
            numElems,
            type,
            EALIGN_GRF,
            CName::NONE), {nullptr, nullptr}, BB};

        m_currShader->CopyVariable(storeData.var, pSrc_X, 0);
        m_currShader->CopyVariable(storeData.var, pSrc_Y, numEltGRF);
        m_currShader->CopyVariable(storeData.var, pSrc_Z, 2 * numEltGRF);
        m_currShader->CopyVariable(storeData.var, pSrc_W, 3 * numEltGRF);
    }

    CVariable* broadcastedVar = storeData.broadcastedVar[m_encoder->IsSecondHalf()];
    if (!broadcastedVar)
    {
        broadcastedVar = BroadcastIfUniform(storeData.var);
        storeData.broadcastedVar[m_encoder->IsSecondHalf()] = broadcastedVar;
    }
    return broadcastedVar;
}

// If constant vector is stored and there is already var instance for it
// try reusing it (if it was defined in the same basic block)
// or create a new var instance and make it available for reusing in further stores
CVariable* EmitPass::tryReusingConstVectorStoreData(Value* storedVal, BasicBlock* BB, bool isBroadcast)
{
    if (IGC_IS_FLAG_DISABLED(EnableReusingLSCStoreConstPayload))
        return nullptr;

    Constant* constantStoredVal = dyn_cast<Constant>(storedVal);
    if (!constantStoredVal ||
        !(constantStoredVal->getType()->isFPOrFPVectorTy() ||
          constantStoredVal->getType()->isIntOrIntVectorTy()))
    {
        return nullptr;
    }

    ConstVectorStoreData& storeData = m_constantVectorStores[constantStoredVal];
    if (storeData.BB != BB)
    {
        // the variable is not yet created or has different BB, create the new instance
        storeData = {GetSymbol(constantStoredVal), {nullptr, nullptr}, BB};
    }

    if (!isBroadcast)
    {
        return storeData.var;
    }

    CVariable* broadcastedVar = storeData.broadcastedVar[m_encoder->IsSecondHalf()];
    if (!broadcastedVar)
    {
        broadcastedVar = BroadcastIfUniform(storeData.var);
        storeData.broadcastedVar[m_encoder->IsSecondHalf()] = broadcastedVar;
    }
    return broadcastedVar;
}

// Get either the 1st/2nd of the execution mask based on whether IsSecondHalf() is set
// Note that for SIMD32 kernels we always return UD with one half zeroed-out
CVariable* EmitPass::GetHalfExecutionMask()
{
    auto currBlock = getCurrentBlock();
    IGC_ASSERT(currBlock);
    // WA: Always compute the EMask for the current BB if inlineAsm calls are present,
    // since they may introduce control flow (via labels) LLVM is not able to track.
    if (!currBlock->m_activeMask || m_pCtx->m_instrTypes.hasInlineAsm)
    {
        bool isSecondHalf = m_encoder->IsSecondHalf();
        bool isSubSpanDst = m_encoder->IsSubSpanDestination();
        m_encoder->SetSecondHalf(false);
        m_encoder->SetSubSpanDestination(false);
        CVariable* flag = m_currShader->ImmToVariable(0, ISA_TYPE_BOOL);
        CVariable* dummyVar = m_currShader->GetNewVariable(1, ISA_TYPE_UW, EALIGN_WORD, true, CName::NONE);
        m_encoder->Cmp(EPREDICATE_EQ, flag, dummyVar, dummyVar);
        m_encoder->Push();

        if (m_currShader->m_State.m_dispatchSize > SIMDMode::SIMD16)
        {
            m_encoder->SetSecondHalf(true);
            m_encoder->Cmp(EPREDICATE_EQ, flag, dummyVar, dummyVar);
            m_encoder->Push();
        }
        m_encoder->SetSecondHalf(isSecondHalf);
        m_encoder->SetSubSpanDestination(isSubSpanDst);
        currBlock->m_activeMask = flag;
    }

    VISA_Type maskType = m_currShader->m_State.m_dispatchSize > SIMDMode::SIMD16 ? ISA_TYPE_UD : ISA_TYPE_UW;
    CVariable* eMask = m_currShader->GetNewVariable(1, maskType, EALIGN_DWORD, true, CName::NONE);
    m_encoder->SetNoMask();
    m_encoder->Cast(eMask, currBlock->m_activeMask);
    m_encoder->Push();

    // for SIMD32, clear out the other half
    if (maskType == ISA_TYPE_UD)
    {
        CVariable* halfMask = m_currShader->GetNewVariable(1, maskType, EALIGN_DWORD, true, CName::NONE);
        m_encoder->SetNoMask();
        m_encoder->And(halfMask, eMask, m_currShader->ImmToVariable(m_encoder->IsSecondHalf() ? 0xFFFF0000 : 0xFFFF, ISA_TYPE_UD));
        m_encoder->Push();
        return halfMask;
    }

    return eMask;
}

// Execution mask for the entire dispatch size, not for either first or second half instance.
CVariable* EmitPass::GetExecutionMask(CVariable*& vecMaskVar)
{
    bool isSecondHalf = m_encoder->IsSecondHalf();
    bool isSubSpanDst = m_encoder->IsSubSpanDestination();
    m_encoder->SetSecondHalf(false);
    m_encoder->SetSubSpanDestination(false);
    CVariable* flag = m_currShader->ImmToVariable(0, ISA_TYPE_BOOL);

    CVariable* dummyVar = m_currShader->GetNewVariable(1, ISA_TYPE_UW, EALIGN_WORD, true, CName::NONE);
    if (m_currShader->m_State.m_dispatchSize > SIMDMode::SIMD16)
    {
        // Make sure to use simd32 always.
        m_encoder->SetSimdSize(SIMDMode::SIMD32);
    }
    m_encoder->Cmp(EPREDICATE_EQ, flag, dummyVar, dummyVar);
    m_encoder->Push();

    m_encoder->SetSecondHalf(isSecondHalf);
    m_encoder->SetSubSpanDestination(isSubSpanDst);
    vecMaskVar = flag;

    VISA_Type maskType = m_currShader->m_State.m_dispatchSize > SIMDMode::SIMD16 ? ISA_TYPE_UD : ISA_TYPE_UW;
    CVariable* eMask = m_currShader->GetNewVariable(1, maskType, EALIGN_DWORD, true, CName::NONE);
    m_encoder->SetNoMask();
    m_encoder->Cast(eMask, flag);
    m_encoder->Push();

    return eMask;
}

CVariable* EmitPass::GetExecutionMask()
{
    CVariable* vecMask = nullptr;
    return GetExecutionMask(vecMask);
}

CVariable* EmitPass::GetNumActiveLanes()
{
    // The numbef of active lanes is generated by the following sequence:
    //    (W) mov (16|M0)  f0.0<1>:uw  0:uw
    //        cmp.eq.f0.0  (16|M0)  dummy:uw dummy:uw
    //    (W) mov (1|M0)   r2.0<1>:uw   f0.0:uw
    //    (W) cbit (1|M0)  r1.0:uw  r2.0:uw
    SBasicBlock* currBlk = getCurrentBlock();
    CVariable* numActiveLanes = currBlk ? currBlk->m_numActiveLanes : nullptr;
    if (numActiveLanes == nullptr)
    {
        CVariable* emask = GetExecutionMask();  // execution mask for the entire dispatch size
        // Count the number of '1' bits we have in the execmask to get the number of active lanes.
        // For example, given emask = 1011011000100010b,  numActiveLanes = 7
        // This will handle cases in which not all lanes are active.
        numActiveLanes = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true, CName::NONE);
        m_encoder->CBit(numActiveLanes, emask);
        m_encoder->Push();

        // save it for possible re-use later.
        if (currBlk)
        {
            currBlk->m_numActiveLanes = numActiveLanes;
        }
    }
    return numActiveLanes;
}

/// UniformCopy - Copy a non-uniform source into a uniform variable by copying
/// ANY active elements.

CVariable* EmitPass::UniformCopy(CVariable* var, bool doSub)
{
    CVariable* offset = nullptr;
    CVariable* eMask = nullptr;
    return UniformCopy(var, offset, eMask, doSub);
}

/// Uniform copy allowing to reuse the off calculated by a previous call
/// This allow avoiding redundant code
CVariable* EmitPass::UniformCopy(CVariable* var, CVariable*& off, CVariable* eMask, bool doSub, bool safeGuard)
{
    IGC_ASSERT_MESSAGE(!var->IsUniform(), "Expect non-uniform source!");

    if (eMask == nullptr)
    {
        eMask = GetExecutionMask();
    }
    if (off == nullptr)
    {
        // Get offset to any 1s. For simplicity, use 'fbl' to find the lowest 1s.
        off = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true, CName::NONE);
        if (doSub && m_encoder->IsSecondHalf())
        {
            // here our eMask is UD but we only want the upper 16-bit
            // use an UW alias to the high 16-bit instead
            auto uwMask = m_currShader->GetNewAlias(eMask, ISA_TYPE_UW, 2, 1);
            m_encoder->Fbl(off, uwMask);
        }
        else
        {
            m_encoder->Fbl(off, eMask);
        }

        if (safeGuard)
        {
            m_encoder->And(off, off, m_currShader->ImmToVariable(numLanes(m_currShader->m_SIMDSize) - 1, ISA_TYPE_UD));
        }

        m_encoder->Push();

        // Calculate byte offset
        CVariable* shAmt = nullptr;
        switch (var->GetElemSize()) {
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
            IGC_ASSERT_MESSAGE(0, "Unsupported element size!");
            break;
        }
        if (shAmt) {
            m_encoder->Shl(off, off, shAmt);
            m_encoder->Push();
        }
        off = m_currShader->BitCast(off, ISA_TYPE_UW);
    }
    // Calculate that active lane address.
    CVariable* addr =
        m_currShader->GetNewAddressVariable(1, var->GetType(), true, true, var->getName());

    // Now, we need to jump through a few hoops for SIMD32, since the variables
    // representing all of the SIMD lanes may not be consecutive.
    uint8_t numInstances = var->GetNumberInstance();

    if (numInstances == 2)
    {
        uint16_t numElements = var->GetNumberElement();
        VISA_Type dataType = var->GetType();

        // Create a variable into which we'll merge both instances of the original variable,
        // and an alias into the upper half.
        CVariable* merged = m_currShader->GetNewVariable(numElements * numInstances,
            dataType, var->GetAlign(), false, 1, CName(var->getName(), "Merged"));
        CVariable* upperMerged = m_currShader->GetNewAlias(merged, dataType,
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
    CVariable* exVal = m_currShader->GetNewVariable(
        1, var->GetType(), CEncoder::GetCISADataTypeAlignment(var->GetType()), true, CName::NONE);
    m_encoder->Copy(exVal, addr);

    return exVal;
}

CVariable* EmitPass::ExtendVariable(CVariable* pVar, e_alignment uniformAlign) {
    if (pVar->GetElemSize() >= 4) {
        // There's no need to extend the operand. But, if the variable holding
        // a uniform value is not aligned to GRF, additional copy is required
        // to align it for SIMD1 gather/scatter.
        if (!pVar->IsUniform())
            return pVar;
        if (!pVar->IsImmediate() && IsGRFAligned(pVar, EALIGN_GRF))
            return pVar;
        // Otherwise, we need to re-align the variable holding that uniform value.
    }

    VISA_Type NewType = ISA_TYPE_UD;
    if (pVar->GetElemSize() > 4)
        NewType = ISA_TYPE_UQ;

    // Cast to extend and/or re-align the variable.
    CVariable* NewVar = 0;
    if (pVar->IsUniform()) {
        NewVar = m_currShader->GetNewVariable(1, NewType, uniformAlign, true, pVar->getName());
    }
    else {
        NewVar = m_currShader->GetNewVariable(
            numLanes(m_currShader->m_SIMDSize), NewType, EALIGN_GRF, pVar->getName());
    }

    if (pVar->IsImmediate()) {
        pVar =
            m_currShader->ImmToVariable(
                pVar->GetImmediateValue(),
                GetUnsignedIntegerType(pVar->GetType()));
    }
    else {
        pVar =
            m_currShader->GetNewAlias(
                pVar, GetUnsignedIntegerType(pVar->GetType()), 0, 0);
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
        EALIGN_GRF,
        CName(pVar->getName(), "Broadcast"));

    m_encoder->Cast(pBroadcast, pVar);
    m_encoder->Push();

    return pBroadcast;
}

CVariable* EmitPass::TruncatePointer(CVariable* pVar, bool TruncBothHalves) {
    // Truncate pointer is used to prepare pointers for A32 and A64
    // messages and in stateful loads and stores to prepare the
    // offset value.
    // For stateless messages pointer data type can only be 32 or 64 bits wide.
    // For stateful messages offset data type can be 8, 16, 32 or 64 bits wide.

    // 32-bit integer
    if (pVar->GetElemSize() == 4) {
        if (!pVar->IsUniform())
            return pVar;
        // For uniform variable, we need to re-align to GRF to ensure it's
        // placed at the 1st element.
        if (!pVar->IsImmediate() && IsGRFAligned(pVar, EALIGN_GRF))
            return pVar;
        // Re-align the container of the pointer.
    }

    // Cast to truncate and/or re-align the variable.
    CVariable* NewVar = 0;
    if (pVar->IsUniform()) {
        NewVar = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_GRF, true, CName(pVar->getName(), "Trunc"));
        m_encoder->Cast(NewVar, pVar);
        m_encoder->Push();
    }
    else {
        unsigned numInstance = TruncBothHalves ? pVar->GetNumberInstance() : 1;
        NewVar = m_currShader->GetNewVariable(
            numLanes(m_currShader->m_SIMDSize), ISA_TYPE_UD, EALIGN_GRF, false, numInstance, CName(pVar->getName(), "Trunc"));

        if (TruncBothHalves && numInstance > 1)
        {
            // Truncate both halves for SIMD32
            bool isSecondHalf = m_encoder->IsSecondHalf();
            m_encoder->SetSecondHalf(false);
            m_encoder->Cast(NewVar, pVar);
            m_encoder->Push();
            m_encoder->SetSecondHalf(true);
            m_encoder->Cast(NewVar, pVar);
            m_encoder->Push();
            m_encoder->SetSecondHalf(isSecondHalf);
        }
        else
        {
            m_encoder->Cast(NewVar, pVar);
            m_encoder->Push();
        }
    }

    return NewVar;
}

CVariable* EmitPass::ReAlignUniformVariable(CVariable* pVar, e_alignment align) {
    if (!pVar->IsUniform())
        return pVar;

    if (!pVar->IsImmediate() && IsGRFAligned(pVar, EALIGN_GRF))
        return pVar;

    CVariable* NewVar = m_currShader->GetNewVariable(
        1, pVar->GetType(), align, true, pVar->getName());

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
            m_currShader->getGRFAlignment(),
            CName(pVar->getName(),"Broadcast64b"));

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


bool EmitPass::ScanReduceIs64BitType(VISA_Type type)
{
    return type == ISA_TYPE_Q || type == ISA_TYPE_UQ || type == ISA_TYPE_DF;
}

bool EmitPass::ScanReduceIsInt64Mul(e_opcode op, VISA_Type type)
{
    return ScanReduceIs64BitType(type) && CEncoder::IsIntegerType(type) && op == EOPCODE_MUL;
}

// True if int64 emulation is required for subgroup/wave; it includes int64 mul case.
bool EmitPass::ScanReduceIsInt64EmulationNeeded(e_opcode op, VISA_Type type)
{
    const bool isInt64Type = ScanReduceIs64BitType(type) && CEncoder::IsIntegerType(type);
    const bool isAndHasInt64Add = op == EOPCODE_ADD && m_currShader->m_Platform->hasInt64Add();
    return ScanReduceIsInt64Mul(op, type) ||
        (isInt64Type && m_currShader->m_Platform->hasNoFullI64Support() && !isAndHasInt64Add);
}


// Copy identity value to dst with NoMask, then src to dst with mask. Notes:
// * dst may be nullptr - it will be created then
// * actual second half setting is preserved
CVariable* EmitPass::ScanReducePrepareSrc(VISA_Type type, uint64_t identityValue, bool negate, bool secondHalf,
    CVariable* src, CVariable* dst, CVariable* flag)
{
    if (!dst)
    {
        const CName name = CName(CName(CName(src->getName().empty() ? "reduceSrc" : "reduceSrc_"),
            src->getName().getCString()), secondHalf ? "SecondHalf" : "");
        dst = m_currShader->GetNewVariable(numLanes(m_currShader->m_SIMDSize), type, EALIGN_GRF, false, name);
    }
    else
    {
        IGC_ASSERT(0 < dst->GetElemSize());
        IGC_ASSERT(numLanes(m_currShader->m_SIMDSize) == dst->GetNumberElement());
        IGC_ASSERT(dst->GetType() == type);
        IGC_ASSERT(dst->GetAlign() == EALIGN_GRF);
        IGC_ASSERT(!dst->IsUniform());
    }
    IGC_ASSERT(src->IsUniform() || dst->GetNumberElement() <= src->GetNumberElement());

    const bool savedSecondHalf = m_encoder->IsSecondHalf();
    m_encoder->SetSecondHalf(secondHalf);

    // Set the GRF to <identity> with no mask. This will set all the registers to <identity>
    CVariable* pIdentityValue = m_currShader->ImmToVariable(identityValue, type);
    m_encoder->SetNoMask();
    m_encoder->Copy(dst, pIdentityValue);
    m_encoder->Push();

    // Now copy the src with a mask so the disabled lanes still keep their <identity>
    if (negate)
    {
        m_encoder->SetSrcModifier(0, EMOD_NEG);
    }
    if (flag)
    {
        m_encoder->SetPredicate(flag);
    }
    m_encoder->Copy(dst, src);
    m_encoder->Push();

    m_encoder->SetSecondHalf(savedSecondHalf);

    return dst;
}

// Reduction all reduce helper: dst_lane{k} = src_lane{simd + k} OP src_lane{k}, k = 0..(simd-1)
CVariable* EmitPass::ReductionReduceHelper(e_opcode op, VISA_Type type, SIMDMode simd, CVariable* src, CVariable* srcSecondHalf)
{
    const bool isInt64Mul = ScanReduceIsInt64Mul(op, type);
    const bool int64EmulationNeeded = ScanReduceIsInt64EmulationNeeded(op, type);

    if (simd == SIMDMode::SIMD16 && m_currShader->m_numberInstance > 1)
    {
        IGC_ASSERT(srcSecondHalf);

        CVariable* temp = m_currShader->GetNewVariable(
            numLanes(simd),
            type,
            EALIGN_GRF,
            false,
            CName("reduceDstSecondHalf"));

        if (!int64EmulationNeeded)
        {
            m_encoder->SetNoMask();
            m_encoder->SetSimdSize(simd);
            m_encoder->GenericAlu(op, temp, src, srcSecondHalf);
            m_encoder->Push();
        }
        else
        {
            if (isInt64Mul)
            {
                CVariable* tmpMulSrc[2] = { src, srcSecondHalf };
                Mul64(temp, tmpMulSrc, simd, true /* noMask */);
            }
            else
            {
                IGC_ASSERT_MESSAGE(0, "Unsupported");
            }
        }

        return temp;
    }

    const bool is64bitType = ScanReduceIs64BitType(type);
    const auto alignment = is64bitType ? IGC::EALIGN_QWORD : IGC::EALIGN_DWORD;
    CVariable* temp = m_currShader->GetNewVariable(
        numLanes(simd),
        type,
        alignment,
        false,
        CName("reduceDst_SIMD", std::to_string(numLanes(simd)).c_str()));

    if (!int64EmulationNeeded)
    {
        m_encoder->SetNoMask();
        m_encoder->SetSimdSize(simd);
        m_encoder->SetSrcSubReg(1, numLanes(simd));
        m_encoder->GenericAlu(op, temp, src, src);
        m_encoder->Push();
    }
    else
    {
        if (isInt64Mul)
        {
            m_encoder->SetSimdSize(simd);
            m_encoder->SetNoMask();
            m_encoder->SetSrcSubReg(0, numLanes(simd));
            m_encoder->Copy(temp, src);
            m_encoder->Push();
            CVariable* pMulSrc[2] = { src, temp };
            Mul64(temp, pMulSrc, simd, true /*noMask*/);
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "Unsupported");
        }
    }

    return temp;
}

// Reduction all expand helper: dst_lane{0..(simd-1)} = src_lane{0} OP src_lane{1}
void EmitPass::ReductionExpandHelper(e_opcode op, VISA_Type type, CVariable* src, CVariable* dst)
{
    const bool isInt64Mul = ScanReduceIsInt64Mul(op, type);
    const bool int64EmulationNeeded = ScanReduceIsInt64EmulationNeeded(op, type);

    if (!int64EmulationNeeded)
    {
        m_encoder->SetSrcSubReg(1, 1);
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        m_encoder->SetSrcRegion(1, 0, 1, 0);
        m_encoder->GenericAlu(op, dst, src, src);
        m_encoder->Push();
    }
    else
    {
        if (isInt64Mul)
        {
            CVariable* tmpMulSrc[2] = {};
            tmpMulSrc[0] = m_currShader->GetNewAlias(src, type, 0, 1, true);
            tmpMulSrc[1] = m_currShader->GetNewAlias(src, type, sizeof(QWORD), 1, true);
            Mul64(dst, tmpMulSrc, m_currShader->m_SIMDSize, false /* noMask */);
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "Unsupported");
        }
    }
}

// Reduction clustered: rearrange src by copying src data elements from even subregisters
// to adjacent subregisters of a new variable. Then do the same for odd src subregisters.
// Rearranged src is a pair of the new variables.
// Notes:
// * numLanes refers to the number of elements of each of new variables (same as dst variable used for reduction)
// * numInst cannot be deduced from numLanes and type
// * second half setting is not preserved by this function
void EmitPass::ReductionClusteredSrcHelper(CVariable* (&pSrc)[2], CVariable* src, uint16_t numLanes,
    VISA_Type type, uint numInst, bool secondHalf)
{
    IGC_ASSERT(numInst > 0);

    const bool is64bitType = ScanReduceIs64BitType(type);
    const auto alignment = is64bitType ? IGC::EALIGN_QWORD : IGC::EALIGN_DWORD;

    const CName name = CName(CName(CName(src->getName().empty() ? "rearrangeSrc" : "rearrangeSrc_"),
        src->getName().getCString()), secondHalf ? "SecondHalf" : "");
    pSrc[0] = m_currShader->GetNewVariable(numLanes, type, alignment, false, name);
    pSrc[1] = m_currShader->GetNewVariable(pSrc[0]);

    CVariable* srcTmp = src;
    CVariable* pSrcTmp[2] = { pSrc[0], pSrc[1] };
    const bool isFloatOrHalfFloat = type == ISA_TYPE_F || type == ISA_TYPE_HF;
    if (m_currShader->m_Platform->doScalar64bScan() && isFloatOrHalfFloat)
    {
        VISA_Type tmpType = (type == ISA_TYPE_F) ? ISA_TYPE_UD : (type == ISA_TYPE_HF) ? ISA_TYPE_UW : type;
        srcTmp = m_currShader->GetNewAlias(srcTmp, tmpType, 0, 0);
        pSrcTmp[0] = m_currShader->GetNewAlias(pSrc[0], tmpType, 0, 0);
        pSrcTmp[1] = m_currShader->GetNewAlias(pSrc[1], tmpType, 0, 0);
    }

    IGC_ASSERT(nullptr != m_encoder);
    m_encoder->SetSecondHalf(secondHalf);
    for (uint i = 0; i < numInst; ++i)
    {
        const e_mask mask = secondHalf ? (i == 1 ? EMASK_Q4 : EMASK_Q3) : (i == 1 ? EMASK_Q2 : EMASK_Q1);

        for (uint j = 0; j < 2; ++j)
        {
            m_encoder->SetSimdSize(lanesToSIMDMode(numLanes / numInst));
            m_encoder->SetNoMask();
            m_encoder->SetMask(mask);
            m_encoder->SetSrcRegion(0, 2, 1, 0);
            m_encoder->SetSrcSubReg(0, j);
            m_encoder->SetSrcSubVar(0, 2 * i);
            m_encoder->SetDstSubVar(i);
            m_encoder->Copy(pSrcTmp[j], srcTmp);
            m_encoder->Push();
        }
    }
    m_encoder->SetSecondHalf(false);
}

// Reduction clustered reduce helper: dst_lane{k} = src_lane{2k} OP src_lane{2k+1}, k = 0..(simd-1)
// For certain opcodes src must be rearranged, to move operation's arguments to the same subreg of different regs.
// Notes:
// * simd is SIMD mode after reduction
// * second half setting is not preserved by this function
// * src and dst may be the same variable
CVariable* EmitPass::ReductionClusteredReduceHelper(e_opcode op, VISA_Type type, SIMDMode simd, bool secondHalf,
    CVariable* src, CVariable* dst)
{
    const bool is64bitType = ScanReduceIs64BitType(type);
    const bool isInt64Mul = ScanReduceIsInt64Mul(op, type);
    const uint numInst = is64bitType && simd == (getGRFSize() > 32 ? SIMDMode::SIMD16 : SIMDMode::SIMD8) ? 2 : 1;

    IGC_ASSERT(simd == SIMDMode::SIMD2 || simd == SIMDMode::SIMD4 || simd == SIMDMode::SIMD8 ||
        (simd == SIMDMode::SIMD16 && getGRFSize() > 32));

    // The op is performed on pairs of adjacent src data elements.
    // In certain cases it is mandatory or might be beneficial for performance reasons
    // to ensure that for each such pair the src data elements are in separate GRFs
    // and that their regioning patterns match.
    bool isRearrangementRequired = isInt64Mul;
    // For XeHP (DG2, XeHP_SDV, PVC)
    // 1. In case where source or destination datatype is 64b or operation is integer DWord multiply:
    //    Register Regioning patterns where register data bit locations are changed between source and destination
    //    are not supported on Src0 and Src1 except for broadcast of a scalar.
    // 2. In case of Float or Half-float data used in destination:
    //    Register Regioning patterns where register data bit locations are changed between source and destination
    //    are not supported on Src0 and Src1 except for broadcast of a scalar.
    const bool isIntegerDwordMultiply = (op == EOPCODE_MUL) && (type == ISA_TYPE_D || type == ISA_TYPE_UD);
    const bool isFloatOrHalfFloat = type == ISA_TYPE_F || type == ISA_TYPE_HF;
    const bool isInt64EmulationNeeded = ScanReduceIsInt64EmulationNeeded(op, type);

    isRearrangementRequired |= m_currShader->m_Platform->doScalar64bScan() &&
        (is64bitType || isIntegerDwordMultiply || isFloatOrHalfFloat);
    isRearrangementRequired |= isInt64EmulationNeeded;

    if (isRearrangementRequired)
    {
        // Rearrange src
        CVariable* pSrc[2] = {};
        ReductionClusteredSrcHelper(pSrc, src, numLanes(simd), type, numInst, secondHalf);

        // Perform reduction with op
        m_encoder->SetSecondHalf(secondHalf);
        if (!isInt64EmulationNeeded)
        {
            m_encoder->SetSimdSize(simd);
            m_encoder->SetNoMask();
            m_encoder->GenericAlu(op, dst, pSrc[0], pSrc[1]);
            m_encoder->Push();
        }
        else
        {
            if (isInt64Mul)
            {
                Mul64(dst, pSrc, simd, true /* noMask */);
            }
            else
            {
                IGC_ASSERT_MESSAGE(0, "Unsupported");
            }
        }
        m_encoder->SetSecondHalf(false);
    }
    else
    {
        m_encoder->SetSecondHalf(secondHalf);
        for (uint i = 0; i < numInst; ++i)
        {
            IGC_ASSERT(numInst);
            m_encoder->SetSimdSize(lanesToSIMDMode(numLanes(simd) / numInst));
            m_encoder->SetNoMask();
            const e_mask mask = secondHalf ? (i == 1 ? EMASK_Q4 : EMASK_Q3) : (i == 1 ? EMASK_Q2 : EMASK_Q1);
            m_encoder->SetMask(mask);
            m_encoder->SetSrcRegion(0, 2, 1, 0);
            m_encoder->SetSrcSubVar(0, 2 * i);
            m_encoder->SetSrcSubReg(0, 0);
            m_encoder->SetSrcRegion(1, 2, 1, 0);
            m_encoder->SetSrcSubVar(1, 2 * i);
            m_encoder->SetSrcSubReg(1, 1);
            m_encoder->SetDstSubVar(i);
            m_encoder->GenericAlu(op, dst, src, src);
            m_encoder->Push();
        }
        m_encoder->SetSecondHalf(false);
    }

    return dst;
}

// Final reduction and expansion clustered expand helper: for each cluster reduce one pair of values to one value,
// and broadcast it to the whole cluster.
// For certain opcodes the src must be rearranged, to keep operation's arguments in the same subreg of different regs.
// Notes:
// * simd is shader's SIMD size
// * second half setting is not preserved by this function
// * src and dst may be the same variable
void EmitPass::ReductionClusteredExpandHelper(e_opcode op, VISA_Type type, SIMDMode simd, const uint clusterSize,
    bool secondHalf, CVariable* src, CVariable* dst)
{
    const bool is64bitType = ScanReduceIs64BitType(type);
    const bool isInt64Mul = ScanReduceIsInt64Mul(op, type);
    const uint numInst = is64bitType && simd == (getGRFSize() > 32 ? SIMDMode::SIMD32 : SIMDMode::SIMD16) ? 2 : 1;
    IGC_ASSERT(clusterSize == 2 || clusterSize == 4 || clusterSize == 8 || clusterSize == 16);
    IGC_ASSERT_MESSAGE(clusterSize * CEncoder::GetCISADataTypeSize(type) <= int_cast<uint>(2 * getGRFSize()),
        "Will generate instructions that cross 2 GRFs boundary.");

    // For information on rearrangement see EmitPass::ReductionClusteredReduceHelper()
    bool isRearrangementRequired = isInt64Mul;
    const bool isIntegerDwordMultiply = (op == EOPCODE_MUL) && (type == ISA_TYPE_D || type == ISA_TYPE_UD);
    const bool isFloatOrHalfFloat = type == ISA_TYPE_F || type == ISA_TYPE_HF;
    const bool isInt64EmulationNeeded = ScanReduceIsInt64EmulationNeeded(op, type);
    isRearrangementRequired |= m_currShader->m_Platform->doScalar64bScan() &&
        (is64bitType || isIntegerDwordMultiply || isFloatOrHalfFloat);
    isRearrangementRequired |= isInt64EmulationNeeded;

    if (isRearrangementRequired)
    {
        const SIMDMode tmpSimd = lanesToSIMDMode(numLanes(simd) / clusterSize);
        // Rearrange src
        CVariable* pSrc[2] = {};
        // For src the 2 grf boundary may be crossed for 2-clusters only in SIMD16 for 64-bit types.
        const uint srcNumInst = clusterSize == 2 ? numInst : 1;
        ReductionClusteredSrcHelper(pSrc, src, numLanes(tmpSimd), type, srcNumInst, secondHalf);

        // Perform reduction with op
        CVariable* tempDst = m_currShader->GetNewVariable(dst);
        m_encoder->SetSecondHalf(secondHalf);
        if (!isInt64EmulationNeeded)
        {
            m_encoder->SetSimdSize(tmpSimd);
            m_encoder->SetNoMask();
            m_encoder->GenericAlu(op, tempDst, pSrc[0], pSrc[1]);
            m_encoder->Push();
        }
        else
        {
            if (isInt64Mul)
            {
                Mul64(tempDst, pSrc, tmpSimd, true /* noMask */);
            }
            else
            {
                IGC_ASSERT_MESSAGE(0, "Unsupported");
            }
        }
        m_encoder->SetSecondHalf(false);

        // In certain cases a 64-bit move may need to be split into two 32-bit uint moves
        const bool use32BitMov = m_currShader->m_Platform->doScalar64bScan() && is64bitType;

        // Broadcast to clusters
        // Example for a 4-clusters of QWORDs:
        // * with 64-bit MOVs:
        // mov (8|M8)               r11.0<1>:uq   r21.2<1;4,0>:uq
        // mov (8|M0)               r35.0<1>:uq   r21.0<1;4,0>:uq
        // * with 32-bit MOVs:
        // mov (8|M8)               r33.0<2>:ud   r21.4<2;4,0>:ud
        // mov (8|M8)               r33.1<2>:ud   r21.5<2;4,0>:ud
        // mov (8|M0)               r31.0<2>:ud   r21.0<2;4,0>:ud
        // mov (8|M0)               r31.1<2>:ud   r21.1<2;4,0>:ud
        m_encoder->SetSecondHalf(secondHalf);
        for (uint i = numInst; i-- != 0;)
        {
            const uint numMovPerElement = use32BitMov ? 2u : 1u;
            for (uint j = 0; j < numMovPerElement; ++j)
            {
                // Outer loop is for 64-bit types in SIMD16 only (cluster size is always <= 8)
                // to broadcast data to upper dst's half which crosses 2-grf boundary.
                // The inner is for movement splitting: one 64-bit to a pair of 32-bit.
                IGC_ASSERT(numInst);
                uint lanes = numLanes(simd) / numInst;
                IGC_ASSERT(clusterSize);
                uint clustersPerInst = lanes / clusterSize;
                uint srcSubReg = i * clustersPerInst * numMovPerElement + j;
                const e_mask mask = simd == SIMDMode::SIMD32 ? (i == 1 ? EMASK_H2 : EMASK_H1) :
                                    secondHalf ? (i == 1 ? EMASK_Q4 : EMASK_Q3) : (i == 1 ? EMASK_Q2 : EMASK_Q1);

                m_encoder->SetSimdSize(lanesToSIMDMode(lanes));
                m_encoder->SetMask(mask);
                m_encoder->SetSrcRegion(0, numMovPerElement, clusterSize, 0);
                m_encoder->SetSrcSubReg(0, srcSubReg);
                m_encoder->SetSrcSubVar(0, 0);
                m_encoder->SetDstRegion(numMovPerElement);
                m_encoder->SetDstSubReg(j);
                m_encoder->SetDstSubVar(2 * i);

                CVariable* broadcastSrc = tempDst;
                CVariable* broadcastDst = dst;
                if (m_currShader->m_Platform->doScalar64bScan() && isFloatOrHalfFloat)
                {
                    VISA_Type tmpType = (type == ISA_TYPE_F) ? ISA_TYPE_UD : (type == ISA_TYPE_HF) ? ISA_TYPE_UW : type;
                    broadcastSrc = m_currShader->GetNewAlias(broadcastSrc, tmpType, 0, 0);
                    broadcastDst = m_currShader->GetNewAlias(broadcastDst, tmpType, 0, 0);
                }
                else if (use32BitMov)
                {
                    broadcastSrc = m_currShader->GetNewAlias(broadcastSrc, VISA_Type::ISA_TYPE_UD, 0, 0);
                    broadcastDst = m_currShader->GetNewAlias(broadcastDst, VISA_Type::ISA_TYPE_UD, 0, 0);
                }
                m_encoder->Copy(broadcastDst, broadcastSrc);
                m_encoder->Push();
            }
        }
        m_encoder->SetSecondHalf(false);
    }
    else
    {
        m_encoder->SetSecondHalf(secondHalf);
        for (uint i = numInst; i-- > 0;)
        {
            const uint srcSubVar = i * (4 / clusterSize);
            const uint srcSubReg = i * (clusterSize == 8 ? 2 : 0);

            m_encoder->SetSimdSize(lanesToSIMDMode(numLanes(simd) / numInst));
            m_encoder->SetNoMask();
            const e_mask mask = secondHalf ? (i == 1 ? EMASK_Q4 : EMASK_Q3) : (i == 1 ? EMASK_Q2 : EMASK_Q1);
            m_encoder->SetMask(mask);
            m_encoder->SetSrcRegion(0, 2, clusterSize, 0);
            m_encoder->SetSrcSubReg(0, srcSubReg);
            m_encoder->SetSrcSubVar(0, srcSubVar);
            m_encoder->SetSrcRegion(1, 2, clusterSize, 0);
            m_encoder->SetSrcSubReg(1, srcSubReg + 1);
            m_encoder->SetSrcSubVar(1, srcSubVar);
            m_encoder->SetDstSubVar(2 * i);
            m_encoder->GenericAlu(op, dst, src, src);
            m_encoder->Push();
        }
        m_encoder->SetSecondHalf(false);
    }
}

void EmitPass::emitReductionTree( e_opcode op, VISA_Type type, CVariable* src, CVariable* dst )
{
    const bool isInt64Mul = ScanReduceIsInt64Mul( op, type );
    const bool int64EmulationNeeded = ScanReduceIsInt64EmulationNeeded( op, type );

    uint16_t srcElementCount = src->GetNumberElement(); // total elements in reduction tree
    uint16_t reductionElementCount = srcElementCount / dst->GetNumberElement(); // number of elements participating per reduction
    // Build reduction tree layers
    while( srcElementCount > dst->GetNumberElement() )
    {
        // Each layer operation merges multiple separate reduction intermediary steps
        // Calculate max lanes per operation and number of merged reduction operations for current layer
        SIMDMode maxSimdMode = ( m_currShader->m_State.m_dispatchSize == SIMDMode::SIMD32 && m_currShader->m_numberInstance > 1 ) ? SIMDMode::SIMD16 : m_currShader->m_State.m_dispatchSize;
        SIMDMode layerMaxSimdMode = lanesToSIMDMode( min( numLanes( maxSimdMode ), (uint16_t)( srcElementCount >> 1 ) ) );
        uint16_t layerMaxSimdLanes = numLanes( layerMaxSimdMode );
        uint16_t src1Offset = reductionElementCount >> 1;
        unsigned int numIterations = srcElementCount / ( 2 * layerMaxSimdLanes ); // number of reduction operations for current layer
        for( unsigned int i = 0; i < numIterations; i++ )
        {
            // Get alias for src0, src1, and dst based on offsets and SIMD size
            auto* layerSrc0 = m_currShader->GetNewAlias( src, type, i * 2 * layerMaxSimdLanes * m_encoder->GetCISADataTypeSize( type ), layerMaxSimdLanes );
            auto* layerSrc1 = m_currShader->GetNewAlias( src, type, ( i * 2 * layerMaxSimdLanes + src1Offset ) * m_encoder->GetCISADataTypeSize( type ), layerMaxSimdLanes );
            auto* layerDst = m_currShader->GetNewAlias( src, type, i * layerMaxSimdLanes * m_encoder->GetCISADataTypeSize( type ), layerMaxSimdLanes );

            if( !int64EmulationNeeded )
            {
                m_encoder->SetNoMask();
                m_encoder->SetSimdSize( layerMaxSimdMode );
                // Set up correct vertical stride and width
                m_encoder->SetSrcRegion( 0, reductionElementCount, ( reductionElementCount >> 1 ), 1 );
                m_encoder->SetSrcRegion( 1, reductionElementCount, ( reductionElementCount >> 1 ), 1 );
                m_encoder->GenericAlu( op, layerDst, layerSrc0, layerSrc1 );
                m_encoder->Push();
            }
            else
            {
                if( isInt64Mul )
                {
                    CVariable* tempMulSrc[ 2 ] = { layerSrc0, layerSrc1 };
                    Mul64( layerDst, tempMulSrc, layerMaxSimdMode, true /*noMask*/ );
                }
                else
                {
                    IGC_ASSERT_MESSAGE( 0, "Unsupported" );
                }
            }
        }

        // Layer complete, total numer of elements and number of elements participating per reduction halved
        srcElementCount >>= 1;
        reductionElementCount >>= 1;
    }

    // copy fully reduced elements from src to dst
    auto* finalLayerDst = m_currShader->GetNewAlias( src, type, 0, dst->GetNumberElement() );
    m_encoder->SetNoMask();
    m_encoder->SetSimdSize( lanesToSIMDMode( dst->GetNumberElement() ) );
    m_encoder->Copy( dst, finalLayerDst );
    m_encoder->Push();
}

// Recursive function that emits one or more joint reduction trees based on the joint output width
void EmitPass::emitReductionTrees( e_opcode op, VISA_Type type, SIMDMode simdMode, CVariable* src, CVariable* dst, unsigned int startIdx, unsigned int endIdx )
{
    unsigned int numGroups = endIdx - startIdx + 1;
    // lanes for final joint reduction
    uint16_t simdLanes = numLanes( simdMode );
    if( numGroups >= simdLanes )
    {
        // Do full tree reduction
        unsigned int reductionElements = src->GetNumberElement() / dst->GetNumberElement();
        unsigned int groupReductionElementCount = reductionElements * simdLanes;
        CVariable* srcAlias = m_currShader->GetNewAlias( src, type, startIdx * reductionElements * m_encoder->GetCISADataTypeSize( type ), groupReductionElementCount );
        CVariable* dstAlias = m_currShader->GetNewAlias( dst, type, startIdx * m_encoder->GetCISADataTypeSize( type ), simdLanes);
        emitReductionTree( op, type, srcAlias, dstAlias );
        // Start new recursive tree if any elements are left
        if ( numGroups > simdLanes )
        {
            emitReductionTrees( op, type, simdMode, src, dst, startIdx + simdLanes, endIdx );
        }
    }
    else
    {
        // Overshoot, try lower SIMD for the final reduction op
        // TODO: Instead of trying lower SIMD, could generate simdLanes wide final join instruction, and pass in identity/0/don't care values for unused joins
        //       However, this will require a change to WaveAllJointReduction to generate intrinsic calls with fixed vector width to ensure the vector source variable used is generated with the proper bounds
        //       or logic to copy the vector source variable to a simdLane * simdLane sized variable along with logic to generate only the necessary operation on that varaible
        switch( simdMode )
        {
        case SIMDMode::SIMD32:
            return emitReductionTrees( op, type, SIMDMode::SIMD16, src, dst, startIdx, endIdx );
        case SIMDMode::SIMD16:
            return emitReductionTrees( op, type, SIMDMode::SIMD8, src, dst, startIdx, endIdx );
        case SIMDMode::SIMD8:
            return emitReductionTrees( op, type, SIMDMode::SIMD4, src, dst, startIdx, endIdx );
        case SIMDMode::SIMD4:
            return emitReductionTrees( op, type, SIMDMode::SIMD2, src, dst, startIdx, endIdx );
        case SIMDMode::SIMD2:
        default:
            return emitReductionTrees( op, type, SIMDMode::SIMD1, src, dst, startIdx, endIdx );
        }
    }
}

// do reduction and accumulate all the activate channels, return a uniform
void EmitPass::emitReductionAll(
    e_opcode op, uint64_t identityValue, VISA_Type type, bool negate, CVariable* src, CVariable* dst)
{
    CVariable* srcH1 = ScanReducePrepareSrc(type, identityValue, negate, false /* secondHalf */,
        src, nullptr /* dst */);
    CVariable* temp = srcH1;
    if (m_currShader->m_State.m_dispatchSize == SIMDMode::SIMD32)
    {
        if (m_currShader->m_numberInstance == 1)
        {
            temp = ReductionReduceHelper(op, type, SIMDMode::SIMD16, temp);
        }
        else
        {
            CVariable* srcH2 = ScanReducePrepareSrc(type, identityValue, negate, true /* secondHalf */,
                src, nullptr /* dst */);

            temp = ReductionReduceHelper(op, type, SIMDMode::SIMD16, temp, srcH2);
        }
    }
    if (m_currShader->m_State.m_dispatchSize >= SIMDMode::SIMD16)
    {
        temp = ReductionReduceHelper(op, type, SIMDMode::SIMD8, temp);
    }
    temp = ReductionReduceHelper(op, type, SIMDMode::SIMD4, temp);
    temp = ReductionReduceHelper(op, type, SIMDMode::SIMD2, temp);
    ReductionExpandHelper(op, type, temp, dst);
}

// for all the active channels within each cluster do reduction and accumulate, return a non-uniform
void EmitPass::emitReductionClustered(const e_opcode op, const uint64_t identityValue, const VISA_Type type,
    const bool negate, const unsigned int clusterSize, CVariable* const src, CVariable* const dst)
{
    const bool isInt64Type = type == ISA_TYPE_Q || type == ISA_TYPE_UQ;
    const bool isFP64Type = type == ISA_TYPE_DF;
    const bool is64bitType = isInt64Type || isFP64Type;
    const bool isInt64Mul = ScanReduceIsInt64Mul(op, type);

    IGC_ASSERT_MESSAGE(iSTD::BitCount(clusterSize) == 1, "Cluster size must be a power of two.");
    IGC_ASSERT_MESSAGE(!is64bitType || CEncoder::GetCISADataTypeSize(type) == 8, "Unsupported 64-bit type.");

    IGC_ASSERT_MESSAGE(!isFP64Type || !m_currShader->m_Platform->hasNoFP64Inst(), "FP64 emulation is not supported.");
    // Src might be uniform, as its value will be broadcasted during src preparation.
    // Dst uniformness depends on actual support in WIAnalysis, so far implemented for 32-clusters only.
    IGC_ASSERT(!dst->IsUniform() || clusterSize == 32);

    const unsigned int dispatchSize = numLanes(m_currShader->m_State.m_dispatchSize);
    const bool useReduceAll = clusterSize >= dispatchSize;

    if (clusterSize == 1)
    {
        IGC_ASSERT_MESSAGE(0, "Simple copy. For performance reasons handle it somehow at earlier stage.");
        for (uint half = 0; half < m_currShader->m_numberInstance; ++half)
        {
            const bool secondHalf = half > 0;
            m_encoder->SetSecondHalf(secondHalf);
            if (negate)
            {
                m_encoder->SetSrcModifier(0, EMOD_NEG);
            }
            m_encoder->Copy(dst, src);
            m_encoder->Push();
            m_encoder->SetSecondHalf(false);
        }
    }
    else if (useReduceAll)
    {
        // TODO: consider if it is possible to detect and handle this case in frontends
        // and emit GenISA_WaveAll there, to enable optimizations specific to the ReduceAll intrinsic.
        emitReductionAll(op, identityValue, type, negate, src, dst);
    }
    else
    {
        for (uint half = 0; half < m_currShader->m_numberInstance; ++half)
        {
            const bool secondHalf = half > 0;

            // Use the "ReduceAll()" approach if code generated by the
            // "optimized path" would generate instructions that cross 2-GRF
            // boundary. The "optimized path" is code generated by
            // ReductionClusteredReduceHelper() + ReductionClusteredExpandHelper().
            const bool mayCross2GRFs =
                clusterSize * CEncoder::GetCISADataTypeSize(type) > int_cast<uint>(2 * getGRFSize());
            if (mayCross2GRFs)
            {
                CVariable* temp = ScanReducePrepareSrc(type, identityValue, negate, secondHalf, src, nullptr);
                // Two halves, for each half src and dst cross 2 grf boundary - "ReduceAll" approach.
                m_encoder->SetSecondHalf(secondHalf);
                IGC_ASSERT(clusterSize == 16);
                temp = ReductionReduceHelper(op, type, SIMDMode::SIMD8, temp);
                temp = ReductionReduceHelper(op, type, SIMDMode::SIMD4, temp);
                temp = ReductionReduceHelper(op, type, SIMDMode::SIMD2, temp);
                ReductionExpandHelper(op, type, temp, dst);
                m_encoder->SetSecondHalf(false);
            }
            else
            {
                // For certain types it is more beneficial (e.g. due to HW restrictions) to perform clustered
                // operations on values converted to another type.
                VISA_Type tmpType = type;
                CVariable* tmpSrc = src;
                CVariable* tmpDst = dst;
                uint64_t tmpIdentityValue = identityValue;
                if (type == VISA_Type::ISA_TYPE_B || type == VISA_Type::ISA_TYPE_UB)
                {
                    const bool isSigned = type == VISA_Type::ISA_TYPE_B;
                    tmpType = isSigned ? VISA_Type::ISA_TYPE_W : VISA_Type::ISA_TYPE_UW;
                    tmpSrc = m_currShader->GetNewVariable(
                        src->GetNumberElement(),
                        tmpType,
                        IGC::EALIGN_DWORD,
                        false,
                        src->getName());
                    m_encoder->SetSecondHalf(secondHalf);
                    m_encoder->Cast(tmpSrc, src);
                    m_encoder->Push();
                    m_encoder->SetSecondHalf(false);
                    tmpDst = m_currShader->GetNewVariable(
                        dst->GetNumberElement(),
                        tmpType,
                        IGC::EALIGN_DWORD,
                        false,
                        CName::NONE);
                    switch (op)
                    {
                    case EOPCODE_MAX:
                        tmpIdentityValue = isSigned ? std::numeric_limits<int16_t>::min() :
                            std::numeric_limits<uint16_t>::min();
                        break;
                    case EOPCODE_MIN:
                        tmpIdentityValue = isSigned ? std::numeric_limits<int16_t>::max() :
                            std::numeric_limits<uint16_t>::max();
                        break;
                    case EOPCODE_AND:
                        tmpIdentityValue = 0xFFFF;
                        break;
                    default:
                        break;
                    }
                }

                CVariable* temp = ScanReducePrepareSrc(tmpType, tmpIdentityValue, negate, secondHalf, tmpSrc, nullptr);

                SIMDMode simd = secondHalf ? SIMDMode::SIMD16 : m_currShader->m_SIMDSize;

                // Reduce with op: SIMDN -> SIMD2; that is, N/2 value pairs -> 1 value pair
                for (uint32_t reducedClusterSize = clusterSize;
                    reducedClusterSize > 2; reducedClusterSize /= 2)
                {
                    simd = lanesToSIMDMode(numLanes(simd) / 2);
                    ReductionClusteredReduceHelper(op, tmpType, simd, secondHalf, temp, temp);
                }

                ReductionClusteredExpandHelper(op, tmpType, m_currShader->m_SIMDSize, clusterSize, secondHalf, temp, tmpDst);

                if (type == VISA_Type::ISA_TYPE_B || type == VISA_Type::ISA_TYPE_UB)
                {
                    m_encoder->SetSecondHalf(secondHalf);
                    m_encoder->Cast(dst, tmpDst);
                    m_encoder->Push();
                    m_encoder->SetSecondHalf(false);
                }
            }
        }
    }
}

// Emits interleave reduction, first preparing the input data. This guarantees to produce
// correct result even if not all lanes are active.
void EmitPass::emitReductionInterleave(const e_opcode op, const uint64_t identityValue, const VISA_Type type,
    const bool negate, const unsigned int step, CVariable* const src, CVariable* const dst)
{
    if (step == 1)
    {
        // TODO: consider if it is possible to detect and handle this case in frontends
        // and emit GenISA_WaveAll there, to enable optimizations specific to the ReduceAll intrinsic.
        return emitReductionAll(op, identityValue, type, negate, src, dst);
    }

    const uint16_t firstStep = numLanes(m_currShader->m_State.m_dispatchSize) / 2;

    IGC_ASSERT_MESSAGE(!dst->IsUniform(), "Unsupported: dst must be non-uniform");
    IGC_ASSERT_MESSAGE(step % 2 == 0 && step <= firstStep, "Invalid reduction interleave step");

    CVariable* srcH1 = ScanReducePrepareSrc(type, identityValue, negate, false /* secondHalf */,
        src, nullptr /* dst */);

    CVariable* srcH2 = nullptr;
    if (firstStep == 16 && m_currShader->m_numberInstance > 1)
    {
        srcH2 = ScanReducePrepareSrc(type, identityValue, negate, true /* secondHalf */,
            src, nullptr /* dst */);
    }

    emitReductionInterleave(op, type, m_currShader->m_SIMDSize, step, false, srcH1, srcH2, dst);
}

// Directly emits interleave reduction on input data, without preparing the input.
void EmitPass::emitReductionInterleave(const e_opcode op, const VISA_Type type, const SIMDMode simd,
    const unsigned int step, const bool noMaskBroadcast, CVariable* const src1, CVariable* const src2, CVariable* const dst)
{
    const uint16_t firstStep = m_currShader->m_numberInstance * numLanes(simd) / 2;

    CVariable* temp = src1;

    // Implementation is similar to emitReductionAll(), but we stop reduction before reaching SIMD1.
    for (unsigned int currentStep = firstStep; currentStep >= step; currentStep >>= 1)
    {
        if (currentStep == 16 && m_currShader->m_numberInstance > 1)
        {
            temp = ReductionReduceHelper(op, type, SIMDMode::SIMD16, temp, src2);
        }
        else
        {
            temp = ReductionReduceHelper(op, type, lanesToSIMDMode(currentStep), temp);
        }
    }

    // Broadcast result
    if (noMaskBroadcast)
        m_encoder->SetNoMask();

    // For XeHP, for low interleave step, broadcast of 64-bit result
    // can be optimized as a separate mov of low/high 32-bit.
    bool use32bitMove = ScanReduceIs64BitType(type) && m_currShader->m_Platform->doScalar64bScan() && m_currShader->m_numberInstance == 1;
    if (use32bitMove && (step == 2 || step == 4))
    {
        CVariable* result32b = m_currShader->GetNewAlias(temp, ISA_TYPE_UD, 0, 2 * step);
        CVariable* dst32b = m_currShader->GetNewAlias(dst, ISA_TYPE_UD, 0, 2 * numLanes(simd));

        m_encoder->SetSimdSize(simd);
        m_encoder->SetSrcRegion(0, 0, step, 2);
        m_encoder->SetDstRegion(2);
        m_encoder->Copy(dst32b, result32b);

        m_encoder->SetSrcSubReg(0, 1);
        m_encoder->SetDstSubReg(1);
        m_encoder->Copy(dst32b, result32b);

        m_encoder->Push();
        return;
    }

    m_encoder->SetSimdSize(simd);
    m_encoder->SetSrcRegion(0, 0, step, 1);
    m_encoder->Copy(dst, temp);
    if (m_currShader->m_numberInstance > 1)
    {
        m_encoder->SetSecondHalf(true);
        m_encoder->Copy(dst, temp);
        m_encoder->SetSecondHalf(false);
    }
    m_encoder->Push();
}

void EmitPass::emitReductionClusteredInterleave(const e_opcode op, const uint64_t identityValue, const VISA_Type type,
    const bool negate, const unsigned int clusterSize, const unsigned int interleaveStep, CVariable* const src, CVariable* const dst)
{
    IGC_ASSERT_MESSAGE(!dst->IsUniform(), "Unsupported: dst must be non-uniform");

    auto simd = m_currShader->m_SIMDSize;
    auto dataSizeInBytes = CEncoder::GetCISADataTypeSize(type);

    // If src spans 4 GRFs and cluster spans 2 GRFs (2 clusters total), then WaveClusterInterleave can be expressed
    // as 2 x WaveInterleave, one for each pair of GRFs.
    if (m_currShader->m_numberInstance == 1 && 2 * clusterSize == numLanes(simd) &&
        numLanes(simd) * dataSizeInBytes == 4 * m_currShader->getGRFSize())
    {
        auto interleaveLanes = numLanes(simd) / 2;
        SIMDMode interleaveSIMD = lanesToSIMDMode(interleaveLanes);

        for (int i = 0; i < 2; ++i)
        {
            CVariable* srcAlias = m_currShader->GetNewAlias(src, type, i * interleaveLanes * dataSizeInBytes, interleaveLanes);
            CVariable* dstAlias = m_currShader->GetNewAlias(dst, type, i * interleaveLanes * dataSizeInBytes, interleaveLanes);

            emitReductionInterleave(op, type, interleaveSIMD, interleaveStep, true, srcAlias, nullptr, dstAlias);
        }

        return;
    }

    // Implementation for each case is custom, with no general solution.

    if (m_currShader->m_numberInstance == 1 && simd == SIMDMode::SIMD32 && dataSizeInBytes == 4 && clusterSize == 16 && interleaveStep == 2)
    {
        CVariable* temp = m_currShader->GetNewVariable(numLanes(simd), type, EALIGN_GRF, false, "reduceSrc");

        // Reorder input. Spread every value by two lanes.
        //
        // |  0 | 16 |  1 | 17 |  2 | 18 | ... | 15 | 31 |
        for (int i = 0; i < 2; ++i)
        {
            m_encoder->SetNoMask();
            m_encoder->SetSimdSize(SIMDMode::SIMD16);
            m_encoder->SetSrcRegion(0, 1, 1, 0);
            m_encoder->SetSrcSubReg(0, 16 * i);
            m_encoder->SetDstRegion(2);
            m_encoder->SetDstSubReg(i);
            m_encoder->Copy(temp, src);
            m_encoder->Push();
        }

        // Reduce.
        temp = ReductionReduceHelper(op, type, SIMDMode::SIMD16, temp);
        temp = ReductionReduceHelper(op, type, SIMDMode::SIMD8, temp);
        temp = ReductionReduceHelper(op, type, SIMDMode::SIMD4, temp);

        // Propagate output. Repeat each value 8 times.
        // temp: | a | b | c | d |
        // dst:  | a | c | a | c | a | c | a | c | ... | b | d | b | d | b | d | b | d |
        for (int i = 0; i < 2; ++i)
        {
            m_encoder->SetNoMask();
            m_encoder->SetSimdSize(SIMDMode::SIMD16);
            m_encoder->SetSrcRegion(0, 1, 8, 0);
            m_encoder->SetSrcSubReg(0, 2 * i);
            m_encoder->SetDstRegion(2);
            m_encoder->SetDstSubReg(i);
            m_encoder->Copy(dst, temp);
            m_encoder->Push();
        }
    }
    else if (m_currShader->m_numberInstance == 1 && simd == SIMDMode::SIMD32 && dataSizeInBytes == 4 && clusterSize == 8 && interleaveStep == 2)
    {
        CVariable* temp = m_currShader->GetNewVariable(numLanes(simd), type, EALIGN_GRF, false, "reduceSrc");

        // Reorder input. Spread every next two values by 8 lanes:
        //
        // |  0 |  1 |  8 |  9 |  16 | 17 | ... | 14 | 15 | 22 | 23 | 30 | 31 |
        for (int i = 0; i < 4; ++i)
        {
            m_encoder->SetNoMask();
            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            m_encoder->SetSrcRegion(0, 8, 2, 1);
            m_encoder->SetSrcSubReg(0, 2 * i);
            m_encoder->SetDstRegion(1);
            m_encoder->SetDstSubReg(8 * i);
            m_encoder->Copy(temp, src);
            m_encoder->Push();
        }

        // Reduce.
        temp = ReductionReduceHelper(op, type, SIMDMode::SIMD16, temp);
        temp = ReductionReduceHelper(op, type, SIMDMode::SIMD8, temp);

        // Propagate output. Repeat each pair of values 4 times.
        //
        // temp: | a | b | c | d | e | f | g | h |
        // dst:  | a | b | a | b | a | b | a | b | ... | g | h | g | h | g | h | g | h |
        for (int i = 0; i < 2; ++i)
        {
            m_encoder->SetNoMask();
            m_encoder->SetSimdSize(SIMDMode::SIMD16);
            m_encoder->SetSrcRegion(0, 2, 4, 0);
            m_encoder->SetSrcSubReg(0, i);
            m_encoder->SetDstRegion(2);
            m_encoder->SetDstSubReg(i);
            m_encoder->Copy(dst, temp);
            m_encoder->Push();
        }
    }
    else
    {
        IGC_ASSERT_MESSAGE(false, "Invalid WaveClusteredInterleave.");
    }
}

// do prefix op across all activate channels
void EmitPass::emitPreOrPostFixOp(
    e_opcode op, uint64_t identityValue, VISA_Type type, bool negateSrc,
    CVariable* pSrc, CVariable* pSrcsArr[2], CVariable* Flag,
    bool isPrefix, bool isQuad, int clusterSize)
{
    // TODO Arguments isQuad and clusterSize have similar function: both split subgroup into
    // smaller sets of lanes processed separately. isQuad could be considered clusterSize == 4,
    // but there is a significant difference in implementation: when shifting input by one lane
    // to the right for exclusive scan (isPrefix == true), isQuad inserts identity value only
    // to the first lane in subgroup, where clusterSize == 8/16 inserts identity value to the
    // first lane of each cluster.
    //
    // isQuad/clusterSize could be replaced with one argument, but the code must be refactored
    // to not break QuadPrefix intrinsic.

    const bool isInt64Mul = ScanReduceIsInt64Mul(op, type);
    const bool int64EmulationNeeded = ScanReduceIsInt64EmulationNeeded(op, type);

    if ((m_currShader->m_Platform->doScalar64bScan() && CEncoder::GetCISADataTypeSize(type) == 8 && !isQuad) ||
        int64EmulationNeeded)
    {
        emitPreOrPostFixOpScalar(
            op, identityValue, type, negateSrc,
            pSrc, pSrcsArr, Flag,
            isPrefix, clusterSize);
        return;
    }

    bool isSimd32AsTwoInstances = m_currShader->m_numberInstance == 2;
    int counter = isSimd32AsTwoInstances ? 2 : 1;

    CVariable* maskedSrc[2] = { 0 };
    for (int i = 0; i < counter; ++i)
    {
        // This is to handle cases when not all lanes are enabled. In that case we fill the lanes with identity.
        CVariable* pSrcCopy = ScanReducePrepareSrc(type, identityValue, negateSrc, i == 1 /*secondHalf*/,
            pSrc, nullptr /*dst*/, Flag);

        m_encoder->SetSecondHalf(i == 1);

        // For case where we need the prefix shift the source by 1 lane
        if (isPrefix)
        {
            maskedSrc[i] = pSrcCopy;
            pSrcCopy = m_currShader->GetNewVariable(pSrcCopy);
            // Copy identity
            m_encoder->SetSimdSize(SIMDMode::SIMD1);
            m_encoder->SetNoMask();
            // Before shift, insert identity value to the first lane
            // in subgroup (or cluster).
            if (i == 0 || clusterSize > 0)
            {
                CVariable* pIdentityValue = m_currShader->ImmToVariable(identityValue, type);
                m_encoder->Copy(pSrcCopy, pIdentityValue);
            }
            else
            {
                m_encoder->SetSrcSubReg(0, 15);
                m_encoder->Copy(pSrcCopy, maskedSrc[i - 1]);
            }
            m_encoder->Push();
            // Copy remained data
            unsigned int simdsize = numLanes(m_currShader->m_SIMDSize);
            unsigned int offset = 1;
            while (simdsize > 1)
            {
                simdsize = simdsize >> 1;
                int numInst = m_encoder->GetCISADataTypeSize(type) == 8 &&
                    simdsize == 8 ? 2 : 1;
                for (int instNum = 0; instNum < numInst; ++instNum)
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

            // After shifting the input by one lane, in each cluster that starts in
            // the middle of GRF, set the first lane to the identity value.
            if (clusterSize > 0)
            {
                m_encoder->SetSimdSize(SIMDMode::SIMD1);
                m_encoder->SetNoMask();
                CVariable* pIdentityValue = m_currShader->ImmToVariable(identityValue, type);

                for (int i = clusterSize; i < pSrcCopy->GetNumberElement(); i += clusterSize)
                {
                    m_encoder->SetDstSubReg(i);
                    m_encoder->Copy(pSrcCopy, pIdentityValue);
                }

                m_encoder->Push();
            }
        }

        pSrcsArr[i] = pSrcCopy;
    }

    auto CreateAlu = [this, op, type, isInt64Mul](
        const SIMDMode simdSize,
        const uint numInst,
        CVariable* pDst,
        CVariable* pSrc0,
        CVariable* pSrc1,
        const uint src0SubReg,
        const uint src0Region[3],
        const uint src1SubReg,
        const uint src1Region[3],
        const uint dstSubReg,
        const uint dstRegion)
    {
        if (isInt64Mul)
        {
            // 64 bit integer multiply case is done in 3 steps:
            // - copy source data to temporary registers to apply
            //   sources regioning and subregister values
            // - call Mul64() emulation usig temporary sources and
            //   a temporary destination
            // - copy the result from the temporary destination
            //   and apply destination regioning and subregister
            //   values
            // Note: Consider passing regioning information
            // directly to the Mul64() emulation function instead
            // of using the temporary registers.
            CVariable* pMulSrc[2] = {};
            const uint16_t maxNumLanes = numLanes(simdSize);
            pMulSrc[0] = m_currShader->GetNewVariable(
                maxNumLanes,
                type,
                EALIGN_GRF,
                false,
                pSrc0->getName());
            pMulSrc[1] = m_currShader->GetNewVariable(
                maxNumLanes,
                type,
                EALIGN_GRF,
                false,
                pSrc1->getName());
            CVariable* pMulDst = m_currShader->GetNewVariable(
                maxNumLanes,
                type,
                EALIGN_GRF,
                false,
                pDst->getName());

            for (uint instNum = 0; instNum < numInst; ++instNum)
            {
                // copy sources with regioning
                m_encoder->SetSimdSize(simdSize);
                m_encoder->SetNoMask();
                m_encoder->SetSrcSubVar(0, instNum * 2);
                m_encoder->SetSrcRegion(0, src0Region[0], src0Region[1], src0Region[2]);
                m_encoder->SetSrcSubReg(0, src0SubReg);
                m_encoder->Copy(pMulSrc[0], pSrc0);
                m_encoder->SetSrcRegion(0, src1Region[0], src1Region[1], src1Region[2]);
                m_encoder->SetSrcSubReg(0, src1SubReg);
                m_encoder->Copy(pMulSrc[1], pSrc1);
                m_encoder->Push();
                // create emulation code
                Mul64(pMulDst, pMulSrc, simdSize, true /* noMask */);
                // copy destination with regioning
                m_encoder->SetSimdSize(simdSize);
                m_encoder->SetNoMask();
                m_encoder->SetDstSubVar(instNum * 2);
                m_encoder->SetDstRegion(dstRegion);
                m_encoder->SetDstSubReg(dstSubReg);
                m_encoder->Copy(pDst, pMulDst);
                m_encoder->Push();
            }
        }
        else
        {
            for (uint instNum = 0; instNum < numInst; ++instNum)
            {
                m_encoder->SetSimdSize(simdSize);
                m_encoder->SetNoMask();
                m_encoder->SetSrcSubVar(0, instNum * 2);
                m_encoder->SetSrcRegion(0, src0Region[0], src0Region[1], src0Region[2]);
                m_encoder->SetSrcSubReg(0, src0SubReg);
                m_encoder->SetSrcSubVar(1, instNum * 2);
                m_encoder->SetSrcRegion(1, src1Region[0], src1Region[1], src1Region[2]);
                m_encoder->SetSrcSubReg(1, src1SubReg);
                m_encoder->SetDstSubVar(instNum * 2);
                m_encoder->SetDstRegion(dstRegion);
                m_encoder->SetDstSubReg(dstSubReg);
                m_encoder->GenericAlu(op, pDst, pSrc0, pSrc1);
                m_encoder->Push();
            }
        }
    };

    if (m_currShader->m_State.m_dispatchSize == SIMDMode::SIMD32 && !isSimd32AsTwoInstances)
    {
        // handling the single SIMD32 size case in PVC
        // the logic is mostly similar to the legacy code sequence below, except that
        // we have a single SIMD32 variable instead of two SIMD16 variables

        // 64-bit types is handled using the scalar version as there's no regioning support
        IGC_ASSERT_MESSAGE(m_encoder->GetCISADataTypeSize(type) < 8, "64-bit type is not supported for this function");

        {
            uint numInst = 1;
            auto simdSize = SIMDMode::SIMD16;
            const uint srcRegion[3] = { 2, 1, 0 };
            CreateAlu(
                simdSize, numInst, pSrcsArr[0], pSrcsArr[0], pSrcsArr[0],
                0 /*src0 subreg*/, srcRegion /*src0 region*/,
                1 /*src1 subreg*/, srcRegion /*src1 region*/,
                1 /*dst subreg*/, 2 /*dst region*/);
        }

        {
            uint numInst = 1;
            auto simdSize = SIMDMode::SIMD8;
            const uint srcRegion[3] = { 4, 1, 0 };
            CreateAlu(
                simdSize, numInst, pSrcsArr[0], pSrcsArr[0], pSrcsArr[0],
                2 /*src0 subreg*/, srcRegion /*src0 region*/,
                1 /*src1 subreg*/, srcRegion /*src1 region*/,
                2 /*dst subreg*/, 4 /*dst region*/);
        }

        {
            uint numInst = 1;
            auto simdSize = SIMDMode::SIMD8;
            const uint srcRegion[3] = { 4, 1, 0 };
            CreateAlu(
                simdSize, numInst, pSrcsArr[0], pSrcsArr[0], pSrcsArr[0],
                3 /*src0 subreg*/, srcRegion /*src0 region*/,
                1 /*src1 subreg*/, srcRegion /*src1 region*/,
                3 /*dst subreg*/, 4 /*dst region*/);
        }

        // Merge: 4 SIMD4's to get 4 SIMD8 prefix sequences
        for (uint loop_counter = 0; loop_counter < 4; ++loop_counter)
        {
            const uint src0Region[3] = { 0, 1, 0 };
            const uint src1Region[3] = { 4, 4, 1 };
            CreateAlu(
                SIMDMode::SIMD4, 1, pSrcsArr[0], pSrcsArr[0], pSrcsArr[0],
                (loop_counter * 8 + 3) /*src0 subreg*/, src0Region /*src0 region*/,
                (loop_counter * 8 + 4) /*src1 subreg*/, src1Region /*src1 region*/,
                (loop_counter * 8 + 4) /*dst subreg*/, 1 /*dst region*/);
        }

        if (clusterSize == 8)
        {
            // With SIMD8 clusters, stop at SIMD8 prefix.
            return;
        }

        // Merge: 2 SIMD8's to get 2 SIMD16 prefix sequence
        for (uint loop_counter = 0; loop_counter < 2; ++loop_counter)
        {
            const uint src0Region[3] = { 0, 1, 0 };
            const uint src1Region[3] = { 1, 1, 0 };
            CreateAlu(
                SIMDMode::SIMD8, 1, pSrcsArr[0], pSrcsArr[0], pSrcsArr[0],
                loop_counter * 16 + 7 /*src0 subreg*/, src0Region /*src0 region*/,
                loop_counter * 16 + 8 /*src1 subreg*/, src1Region /*src1 region*/,
                loop_counter * 16 + 8 /*dst subreg*/, 1 /*dst region*/);
        }

        if (clusterSize == 16)
        {
            // With SIMD16 clusters, stop at SIMD16 prefix.
            return;
        }

        // final merge to get 1 SIMD32 prefix sequence and viola!
        {
            const uint src0Region[3] = { 0, 1, 0 };
            const uint src1Region[3] = { 1, 1, 0 };
            CreateAlu(
                SIMDMode::SIMD16, 1, pSrcsArr[0], pSrcsArr[0], pSrcsArr[0],
                15 /*src0 subreg*/, src0Region /*src0 region*/,
                16 /*src1 subreg*/, src1Region /*src1 region*/,
                16 /*dst subreg*/, 1 /*dst region*/);
        }
        return;
    }

    for (int i = 0; i < counter; ++i)
    {
        /*
        Copy the adjacent elements.
        for example: let r10 be the register
        Assume we are performing addition for this example
           ____      ____      ____      ____
        __|____|____|____|____|____|____|____|_
        |  7 |  6 |  5 |  4 |  9 |  5 |  3 |  2 |
        ---------------------------------------
        */

        {
            // So then start adding from r10.0 & r10.1
            uint numInst = m_encoder->GetCISADataTypeSize(type) == 8 &&
                m_currShader->m_SIMDSize != SIMDMode::SIMD8 ? 2 : 1;
            auto simdSize = m_encoder->GetCISADataTypeSize(type) == 8 ||
                m_currShader->m_SIMDSize == SIMDMode::SIMD8 ? SIMDMode::SIMD4 : SIMDMode::SIMD8;
            const uint srcRegion[3] = { 2, 1, 0 };
            CreateAlu(
                simdSize, numInst, pSrcsArr[i], pSrcsArr[i], pSrcsArr[i],
                0 /*src0 subreg*/, srcRegion /*src0 region*/,
                1 /*src1 subreg*/, srcRegion /*src1 region*/,
                1 /*dst subreg*/, 2 /*dst region*/);
        }

        /*
                ____                  ____
        _______|____|________________|____|______            ___________________________________________
        |  13 |  6 |  9 |  4 |  14 |  5 |  5 |  2 |    ==>  |  13 |  15 |  9 |  4 |  14 |  10 |  5 |  2 |
         -----------------------------------------           -------------------------------------------
        */
        // Now we have a weird copy happening. This will be done by SIMD 2 instructions.

        {
            uint numInst = m_encoder->GetCISADataTypeSize(type) == 8 &&
                m_currShader->m_SIMDSize != SIMDMode::SIMD8 ? 2 : 1;
            auto simdSize = m_encoder->GetCISADataTypeSize(type) == 8 ||
                m_currShader->m_SIMDSize == SIMDMode::SIMD8 ? SIMDMode::SIMD2 : SIMDMode::SIMD4;
            const uint srcRegion[3] = { 4, 1, 0 };
            CreateAlu(
                simdSize, numInst, pSrcsArr[i], pSrcsArr[i], pSrcsArr[i],
                2 /*src0 subreg*/, srcRegion /*src0 region*/,
                1 /*src1 subreg*/, srcRegion /*src1 region*/,
                2 /*dst subreg*/, 4 /*dst region*/);
        }

        /*
           ___________           ___________
        __|___________|_________|___________|______         ___________________________________________
        |  13 |  15 |  9 |  4 |  14 |  10 |  5 |  2 |  ==>  |  22 |  15 |  9 |  4 |  19 |  10 |  5 |  2 |
        -------------------------------------------         -------------------------------------------
        */

        {
            uint numInst = m_encoder->GetCISADataTypeSize(type) == 8 &&
                m_currShader->m_SIMDSize != SIMDMode::SIMD8 ? 2 : 1;
            auto simdSize = m_encoder->GetCISADataTypeSize(type) == 8 ||
                m_currShader->m_SIMDSize == SIMDMode::SIMD8 ? SIMDMode::SIMD2 : SIMDMode::SIMD4;
            const uint srcRegion[3] = { 4, 1, 0 };
            CreateAlu(
                simdSize, numInst, pSrcsArr[i], pSrcsArr[i], pSrcsArr[i],
                3 /*src0 subreg*/, srcRegion /*src0 region*/,
                1 /*src1 subreg*/, srcRegion /*src1 region*/,
                3 /*dst subreg*/, 4 /*dst region*/);
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
        const unsigned int numLanesForSimd8 = numLanes(SIMDMode::SIMD8);
        IGC_ASSERT(numLanesForSimd8);
        const unsigned int numTimesToLoop = numLanes(m_currShader->m_SIMDSize) / numLanesForSimd8;

        for (uint loop_counter = 0; loop_counter < numTimesToLoop; ++loop_counter)
        {
            const uint src0Region[3] = { 0, 1, 0 };
            const uint src1Region[3] = { 4, 4, 1 };
            CreateAlu(
                SIMDMode::SIMD4, 1, pSrcsArr[i], pSrcsArr[i], pSrcsArr[i],
                (loop_counter * 8 + 3) /*src0 subreg*/, src0Region /*src0 region*/,
                (loop_counter * 8 + 4) /*src1 subreg*/, src1Region /*src1 region*/,
                (loop_counter * 8 + 4) /*dst subreg*/, 1 /*dst region*/);
        }

        if (clusterSize == 8)
        {
            // Stop ALU ops at SIMD8 lanes.
            continue;
        }

        if (m_currShader->m_SIMDSize == SIMDMode::SIMD16 || isSimd32AsTwoInstances)
        {
            // Add the last element of the 1st GRF to all the elements of the 2nd GRF
            const uint src0Region[3] = { 0, 1, 0 };
            const uint src1Region[3] = { 1, 1, 0 };
            CreateAlu(
                SIMDMode::SIMD8, 1, pSrcsArr[i], pSrcsArr[i], pSrcsArr[i],
                7 /*src0 subreg*/, src0Region /*src0 region*/,
                8 /*src1 subreg*/, src1Region /*src1 region*/,
                8 /*dst subreg*/, 1 /*dst region*/);
        }
    }

    bool hasClusters = isQuad || clusterSize > 0;
    if (isSimd32AsTwoInstances && !hasClusters)
    {
        // For SIMD32 we need to write the last element of the prev element to the next 16 elements
        const uint src0Region[3] = { 0, 1, 0 };
        const uint src1Region[3] = { 1, 1, 0 };
        CreateAlu(
            SIMDMode::SIMD16, 1, pSrcsArr[1], pSrcsArr[0], pSrcsArr[1],
            (numLanes(m_currShader->m_SIMDSize) - 1) /*src0 subreg*/, src0Region /*src0 region*/,
            0 /*src1 subreg*/, src1Region /*src1 region*/,
            0 /*dst subreg*/, 1 /*dst region*/);
    }
    // reset second half state
    m_encoder->SetSecondHalf(false);
}

// scalar version of the scan operation for 64b types
void EmitPass::emitPreOrPostFixOpScalar(
    e_opcode op,
    uint64_t identityValue,
    VISA_Type type,
    bool negateSrc,
    CVariable* src,
    CVariable* result[2],
    CVariable* Flag,
    bool isPrefix,
    int clusterSize)
{
    const bool isInt64Mul = ScanReduceIsInt64Mul(op, type);
    const bool int64EmulationNeeded = ScanReduceIsInt64EmulationNeeded(op, type);

    bool isSimd32AsTwoInstances = m_currShader->m_numberInstance == 2;
    int counter = isSimd32AsTwoInstances ? 2 : 1;
    CVariable* pSrcCopy[2] = {};
    for (int i = 0; i < counter; ++i)
    {
        // This is to handle cases when not all lanes are enabled. In that case we fill the lanes with identity.
        pSrcCopy[i] = ScanReducePrepareSrc(type, identityValue, negateSrc, i == 1 /*secondHalf*/,
            src, nullptr /*dst*/, Flag);

        result[i] = m_currShader->GetNewVariable(
            numLanes(m_currShader->m_SIMDSize),
            type,
            EALIGN_GRF,
            false,
            "scalarResult");

        m_encoder->SetSecondHalf(i == 1);

        int srcIdx = 0;
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
        if (isPrefix)
        {
            // For case where we need the prefix shift the source by 1 lane.
            if (i == 0 || clusterSize == 8 || clusterSize == 16)
            {
                // (W) mov (1) result[0] identity
                CVariable* pIdentityValue = m_currShader->ImmToVariable(identityValue, type);
                m_encoder->Copy(result[i], pIdentityValue);
            }
            else
            {
                // (W) mov (1) result[16] srcCopy[15]
                m_encoder->SetSrcSubReg(0, 15);
                m_encoder->SetSrcRegion(0, 0, 1, 0);
                m_encoder->Copy(result[i], pSrcCopy[0]);
            }
        }
        else
        {
            // (W) mov (1) result[0/16] srcCopy[0/16]
            m_encoder->SetSrcSubReg(0, 0);
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->Copy(result[i], pSrcCopy[i]);
            srcIdx = 1;
        }
        m_encoder->Push();

        const SIMDMode simd = SIMDMode::SIMD1;
        CVariable* tmpDst = (isInt64Mul || int64EmulationNeeded) ?
            m_currShader->GetNewVariable(
                numLanes(simd),
                type,
                EALIGN_GRF,
                true,
                CName(result[0]->getName(), "Tmp")) : nullptr;

        for (int dstIdx = 1; dstIdx < numLanes(m_currShader->m_SIMDSize); ++dstIdx, ++srcIdx)
        {
            // Scan is done one by one. With clusters, start each cluster with
            // initial value.
            if ((clusterSize == 8 || clusterSize == 16) && dstIdx % clusterSize == 0)
            {
                // For case where we need the prefix, start cluster with
                // identity value.
                if (isPrefix)
                {
                    m_encoder->SetSimdSize(SIMDMode::SIMD1);
                    m_encoder->SetNoMask();
                    m_encoder->SetDstSubReg(dstIdx);
                    CVariable* pIdentityValue = m_currShader->ImmToVariable(identityValue, type);
                    m_encoder->Copy(result[i], pIdentityValue);
                    continue;
                }
            }

            // do the scan one by one
            // (W) op (1) result[dstIdx] srcCopy[srcIdx] result[dstIdx-1]
            if (!int64EmulationNeeded)
            {
                m_encoder->SetSimdSize(simd);
                m_encoder->SetNoMask();
                m_encoder->SetSrcSubReg(0, srcIdx);
                m_encoder->SetSrcRegion(0, 0, 1, 0);
                m_encoder->SetSrcRegion(1, 0, 1, 0);
                m_encoder->SetSrcSubReg(1, dstIdx - 1);
                m_encoder->SetDstSubReg(dstIdx);
                m_encoder->GenericAlu(op, result[i], pSrcCopy[i], result[i]);
                m_encoder->Push();
            }
            else
            {
                if (isInt64Mul)
                {
                    CVariable* pSrc[2] = {
                        m_currShader->GetNewAlias(pSrcCopy[i], type, srcIdx * sizeof(QWORD), 1, true),
                        m_currShader->GetNewAlias(result[i], type, (dstIdx - 1) * sizeof(QWORD), 1, true) };
                    Mul64(tmpDst, pSrc, simd, true /* noMask */);
                }
                else
                {
                    IGC_ASSERT_MESSAGE(0, "Unsupported");
                }

                // (W) mov (1) result[dstIdx] tmpDst
                m_encoder->SetSimdSize(simd);
                m_encoder->SetNoMask();
                m_encoder->SetDstSubReg(dstIdx);
                m_encoder->Copy(result[i], tmpDst);
                m_encoder->Push();
            }
        }

        m_encoder->SetSecondHalf(false);
    }

    if (isSimd32AsTwoInstances && !clusterSize)
    {
        const SIMDMode simd = SIMDMode::SIMD16;

        m_encoder->SetSecondHalf(true);

        // For SIMD32 we need to write the last element of the prev element to the next 16 elements.
        if (!int64EmulationNeeded)
        {
            m_encoder->SetSimdSize(simd);
            m_encoder->SetNoMask();
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->SetSrcSubReg(0, 15);
            m_encoder->GenericAlu(op, result[1], result[0], result[1]);
            m_encoder->Push();
        }
        else
        {
            if (isInt64Mul)
            {
                CVariable* pSrc[2] = { m_currShader->GetNewAlias(result[0], type, 15 * sizeof(QWORD), 1, true), result[1] };
                Mul64(result[1], pSrc, simd, true /* noMask */);
            }
            else
            {
                IGC_ASSERT_MESSAGE(0, "Unsupported");
            }
        }

        m_encoder->SetSecondHalf(false);
    }
}

static const LSC_CACHE_OPTS LSC_DEFAULT_CACHING {
    LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT
};

/*
ScalarAtomics: This optimization attempts to reduce the number of atomic instructions issued when
the destination addresses and the source are both uniform. For example lets say we have an atomic
add happens with destination address as <addr> = constant. <src> = constant too. In this case, lets
say for SIMD8 there are 8 lanes trying to write to the same address. H/W will serialize this to
8 back to back atomic instructions which are extremely slow to execute.
*/
void EmitPass::emitScalarAtomics(llvm::Instruction *pInst,
                                 ResourceDescriptor &resource,
                                 AtomicOp atomic_op,
                                 CVariable *pDstAddr, CVariable *pU,
                                 CVariable *pV, CVariable *pR, CVariable *pSrc,
                                 bool isA64, int bitWidth, int immOffset,
                                 int immScale, LSC_ADDR_SIZE addrSize) {
    e_opcode op = EOPCODE_ADD;
    // find the value for which opcode(x, identity) == x
    unsigned int identityValue = 0;
    switch (atomic_op)
    {
    case EATOMIC_IADD:
    case EATOMIC_SUB:
    case EATOMIC_IADD64:
    case EATOMIC_SUB64:
    case EATOMIC_FADD64:
    case EATOMIC_FSUB64:
    case EATOMIC_INC:
    case EATOMIC_DEC:
    case EATOMIC_FADD:
    case EATOMIC_FSUB:
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
    case EATOMIC_OR:
        identityValue = 0;
        op = EOPCODE_OR;
        break;
    case EATOMIC_AND:
        identityValue = 0xFFFFFFFF;
        op = EOPCODE_AND;
        break;
    case EATOMIC_XOR:
        identityValue = 0;
        op = EOPCODE_XOR;
        break;
    default:
        IGC_ASSERT_MESSAGE(0, "unsupported scalar atomic type");
        break;
    }

    VISA_Type type;
    if (atomic_op == EATOMIC_FADD || atomic_op == EATOMIC_FSUB)
    {
        type = ISA_TYPE_F;
    }
    else if (atomic_op == EATOMIC_FADD64 || atomic_op == EATOMIC_FSUB64)
    {
        type = ISA_TYPE_DF;
    }
    else
    {
        type =
            bitWidth == 16 ? ISA_TYPE_W :
            bitWidth == 32 ? ISA_TYPE_D :
                        ISA_TYPE_Q;
    }

    IGC_ASSERT_MESSAGE((bitWidth == 16) || (bitWidth == 32) || (bitWidth == 64), "invalid bitsize");
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
    if (atomic_op == EATOMIC_UMAX || atomic_op == EATOMIC_UMIN)
    {
        type = GetUnsignedType(type);
    }
    AtomicOp uniformAtomicOp = atomic_op;
    bool negateSrc = false;
    if (atomic_op == EATOMIC_SUB)
    {
        negateSrc = true;
        uniformAtomicOp = EATOMIC_IADD;
    }
    else if (atomic_op == EATOMIC_SUB64)
    {
        negateSrc = true;
        uniformAtomicOp = EATOMIC_IADD64;
    }
    else if (atomic_op == EATOMIC_FSUB)
    {
        negateSrc = true;
        uniformAtomicOp = EATOMIC_FADD;
    }
    else if (atomic_op == EATOMIC_FSUB64)
    {
        negateSrc = true;
        uniformAtomicOp = EATOMIC_FADD64;
    }

    bool returnsImmValue = (!pInst->use_empty());
    CVariable* pFinalAtomicSrcVal;
    CVariable* pSrcsArr[2] = { nullptr, nullptr };

    if (op == EOPCODE_ADD && bitWidth == 32 && pSrc->IsUniform() &&
        getCurrInstNumInstances() == 1 && !returnsImmValue)
    {
        // Special case for uniform DW src (like atomic_add(1) without return value.
        // Note: limit this code for a single instance for now as scalar atomic must have
        //       instance = 1 (see DecideInstanceAndSlice()).
        //
        // The following sequence will be generated:
        //    (W) mov (16|M0)  f0.0<1>:uw  0:uw
        //        cmp.eq.f0.0  (16|M0)  dummy:uw dummy:uw
        //    (W) mov (1|M0)   r2.0<1>:uw   f0.0:uw
        //    (W) cbit (1|M0)  r1.0:uw  r2.0:uw       <-- all insts up to here are generated by GetNumActiveLanes()
        //    (W) mul  (1|M0)  r10:ud   pSrc  r1.0:uw
        CVariable* numActiveLanes = GetNumActiveLanes();

        // pFinalAtomicSrcVal is used in msg's payload and thus needs to be GRF-aligned
        pFinalAtomicSrcVal = m_currShader->GetNewVariable(1, type, EALIGN_GRF, true, CName::NONE);
        if ((type != ISA_TYPE_F) && pSrc->IsImmediate() && pSrc->GetImmediateValue() == 1)
        {
            if (negateSrc)
            {
                m_encoder->SetSrcModifier(0, EMOD_NEG);
            }
            m_encoder->Cast(pFinalAtomicSrcVal, numActiveLanes);
            m_encoder->Push();
        }
        else
        {
            CVariable* castNumActiveLanes = nullptr;
            if (type == ISA_TYPE_F)
            {
                castNumActiveLanes = m_currShader->GetNewVariable(1, ISA_TYPE_F, EALIGN_GRF, true, CName::NONE);
                m_encoder->Cast(castNumActiveLanes, numActiveLanes);
            }
            else
            {
                // Use UW for numActiveLanes as it results in less insts.
                castNumActiveLanes = m_currShader->GetNewAlias(numActiveLanes, ISA_TYPE_UW, 0, 1);
            }
            m_encoder->Mul(pFinalAtomicSrcVal, pSrc, castNumActiveLanes);
            m_encoder->Push();

            // using neg srcmod with mul will end up with more insts, thus using srcmod on mov
            if (negateSrc)
            {
                m_encoder->SetSrcModifier(0, EMOD_NEG);
            }
            m_encoder->Copy(pFinalAtomicSrcVal, pFinalAtomicSrcVal);
            m_encoder->Push();
        }
    }
    else if ((op == EOPCODE_AND || op == EOPCODE_OR) && pSrc->IsUniform())
    {
        pFinalAtomicSrcVal = ReAlignUniformVariable(pSrc, EALIGN_GRF);
    }
    else
    {
        // general case
        pFinalAtomicSrcVal = m_currShader->GetNewVariable(
            1,
            type,
            isA64 ? EALIGN_2GRF : EALIGN_GRF,
            true,
            CName::NONE);

        if (returnsImmValue)
        {
            // sum all the lanes
            emitPreOrPostFixOp(op, identityValue, type, negateSrc, pSrc, pSrcsArr);

            CVariable* pSrcCopy = pSrcsArr[0];
            if (m_currShader->m_numberInstance == 2)
            {
                pSrcCopy = pSrcsArr[1];
            }

            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->SetSrcSubReg(0, numLanes(m_currShader->m_SIMDSize) - 1);
            m_encoder->Copy(pFinalAtomicSrcVal, pSrcCopy);
            m_encoder->Push();
        }
        else
        {
            emitReductionAll(op, identityValue, type, negateSrc, pSrc, pFinalAtomicSrcVal);
        }
    }

    auto moveToReg = [&](CVariable*& pVar)
    {
        CVariable* pVarCopy = m_currShader->GetNewVariable(1, pVar->GetType(), EALIGN_GRF, true, pVar->getName());
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
        m_encoder->Copy(pVarCopy, pVar);
        m_encoder->Push();
        pVar = pVarCopy;
    };

    if (pDstAddr && pDstAddr->IsImmediate())
    {
        moveToReg(pDstAddr);
    }

    if (pU && pU->IsImmediate())
    {
        moveToReg(pU);
    }

    if (pV && pV->IsImmediate())
    {
        moveToReg(pV);
    }

    if (pR && pR->IsImmediate())
    {
        moveToReg(pR);
    }

    m_encoder->SetSimdSize(SIMDMode::SIMD1);
    m_encoder->SetNoMask();

    VISA_Type pReturnValType;
    if (type == ISA_TYPE_Q)
    {
        pReturnValType = ISA_TYPE_UQ;
    }
    else if (type == ISA_TYPE_DF)
    {
        pReturnValType = ISA_TYPE_DF;
    }
    else if (type == ISA_TYPE_F)
    {
        pReturnValType = ISA_TYPE_F;
    }
    else
    {
        pReturnValType = ISA_TYPE_UD;
    }

    CVariable* pReturnVal = returnsImmValue ?
        m_currShader->GetNewVariable(
            1, pReturnValType, EALIGN_GRF, true, CName::NONE) :
        nullptr;

    if (bitWidth == 16)
    {
        CVariable* pCastAtomicSrcVal =
            m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_GRF, true, CName::NONE);

        m_encoder->Cast(pCastAtomicSrcVal, pFinalAtomicSrcVal);
        pFinalAtomicSrcVal = pCastAtomicSrcVal;
    }
    if (shouldGenerateLSC(pInst))
    {
        auto cacheOpts = LSC_DEFAULT_CACHING;
        if (pU || pV || pR)
        {
            m_encoder->LSC_TypedAtomic(uniformAtomicOp, &resource, pU, pV, pR,
                                       pFinalAtomicSrcVal, nullptr /*src1*/,
                                       pReturnVal, bitWidth, LSC_ADDR_SIZE_32b, cacheOpts);
        }
        else
        {
            m_encoder->LSC_AtomicRaw(
                uniformAtomicOp,
                pReturnVal,
                pDstAddr,
                pFinalAtomicSrcVal,
                nullptr,
                bitWidth,
                &resource,
                addrSize,
                immOffset,
                immScale,
                cacheOpts);
        }
    }
    else
    {
        if (isA64)
        {
            m_encoder->AtomicRawA64(
                uniformAtomicOp, resource,
                pReturnVal, pDstAddr,
                pFinalAtomicSrcVal, nullptr,
                bitWidth);
        }
        else if (pU || pV || pR)
        {
            m_encoder->TypedAtomic(uniformAtomicOp, pReturnVal, resource, pU, pV, pR,
                pFinalAtomicSrcVal, nullptr /*src1*/, nullptr /*lod*/, bitWidth == 16);
        }
        else
        {
            m_encoder->DwordAtomicRaw(
                uniformAtomicOp, resource,
                pReturnVal, pDstAddr,
                pFinalAtomicSrcVal,
                nullptr, bitWidth == 16);
        }
    }
    m_encoder->Push();

    if (returnsImmValue)
    {
        unsigned int counter = m_currShader->m_numberInstance;
        IGC_ASSERT_MESSAGE(op == EOPCODE_ADD, "we can only get the return value for add right now");
        for (unsigned int i = 0; i < counter; ++i)
        {
            m_encoder->SetNoMask();
            m_encoder->Add(pSrcsArr[i], pSrcsArr[i], pReturnVal);
            m_encoder->Push();

            if (atomic_op == EATOMIC_IADD ||
                atomic_op == EATOMIC_IADD64 ||
                atomic_op == EATOMIC_FADD ||
                atomic_op == EATOMIC_FADD64)
            {
                m_encoder->SetSrcModifier(1, EMOD_NEG);
            }

            m_encoder->SetSecondHalf(i == 1);
            m_encoder->Add(m_destination, pSrcsArr[i], pSrc);
            m_encoder->Push();
        }
    }
}

//
// We emulate an atomic_load with an atomic_or with zero.
// when the atomic is uniform we can directly generate a SIMD1 atomic_or
//
void EmitPass::emitScalarAtomicLoad(
    llvm::Instruction* pInst,
    ResourceDescriptor& resource,
    CVariable* pDstAddr,
    CVariable* pU,
    CVariable* pV,
    CVariable* pR,
    CVariable* pSrc,
    bool isA64,
    int bitWidth,
    int immOffset,
    int immScale,
    LSC_ADDR_SIZE addrSize)
{
    auto moveToReg = [&](CVariable*& pVar)
    {
        CVariable* pVarCopy = m_currShader->GetNewVariable(1, pVar->GetType(), EALIGN_GRF, true, pVar->getName());
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
        m_encoder->Copy(pVarCopy, pVar);
        m_encoder->Push();
        pVar = pVarCopy;
    };

    if (pDstAddr && pDstAddr->IsImmediate())
    {
        moveToReg(pDstAddr);
    }

    if (pU && pU->IsImmediate())
    {
        moveToReg(pU);
    }

    if (pV && pV->IsImmediate())
    {
        moveToReg(pV);
    }

    if (pR && pR->IsImmediate())
    {
        moveToReg(pR);
    }

    {
        // pSrc is imm zero
        moveToReg(pSrc);
    }

    m_encoder->SetSimdSize(SIMDMode::SIMD1);
    m_encoder->SetNoMask();

    CVariable* atomicDst = !pInst->use_empty() ?
        m_currShader->GetNewVariable(
            1,
            GetTypeFromSize(bitWidth / 8),
            isA64 ? EALIGN_2GRF : EALIGN_GRF,
            true,
            pDstAddr ? pDstAddr->getName() : CName::NONE) : nullptr;
    if (shouldGenerateLSC(pInst))
    {
        auto cacheOpts = LSC_DEFAULT_CACHING;
        if (pU || pV || pR)
        {
            m_encoder->LSC_TypedAtomic(
                EATOMIC_OR,
                &resource,
                pU,
                pV,
                pR,
                pSrc,
                nullptr /*src1*/,
                atomicDst,
                bitWidth,
                LSC_ADDR_SIZE_32b,
                cacheOpts);
        }
        else
        {
            m_encoder->LSC_AtomicRaw(
                EATOMIC_OR,
                atomicDst,
                pDstAddr,
                pSrc,
                nullptr,
                bitWidth,
                &resource,
                addrSize,
                immOffset,
                immScale,
                cacheOpts);
        }
    }
    else
    {

        if (isA64)
        {
            m_encoder->AtomicRawA64(
                EATOMIC_OR, resource,
                atomicDst, pDstAddr,
                pSrc, nullptr,
                bitWidth);
        }
        else if (pU || pV || pR)
        {
            m_encoder->TypedAtomic(EATOMIC_OR, atomicDst, resource, pU, pV, pR,
                pSrc, nullptr /*src1*/, nullptr /*lod*/, bitWidth == 16);
        }
        else
        {
            m_encoder->DwordAtomicRaw(
                EATOMIC_OR, resource,
                atomicDst, pDstAddr,
                pSrc,
                nullptr, bitWidth == 16);
        }
    }
    m_encoder->Push();

    if (!pInst->use_empty())
    {
        // we need to broadcast the return value
        // ToDo: change divergence analysis to mark scalar atomic load as uniform
        unsigned int counter = m_currShader->m_numberInstance;
        for (unsigned int i = 0; i < counter; ++i)
        {
            m_encoder->SetSecondHalf(i == 1);
            m_encoder->Copy(m_destination, atomicDst);
            m_encoder->Push();
        }
    }
}

bool static OrWith0Atomic(llvm::Instruction* pInst, const uint op)
{
    return (isa<ConstantInt>(pInst->getOperand(op)) &&
        cast<ConstantInt>(pInst->getOperand(op))->isZero());
}

bool EmitPass::IsUniformAtomic(llvm::Instruction* pInst)
{
    if (llvm::GenIntrinsicInst * pIntrinsic = llvm::dyn_cast<llvm::GenIntrinsicInst>(pInst))
    {
        GenISAIntrinsic::ID id = pIntrinsic->getIntrinsicID();

        // Dst address in bytes.
        if (id == GenISAIntrinsic::GenISA_intatomicraw ||
            id == GenISAIntrinsic::GenISA_intatomicrawA64 ||
            id == GenISAIntrinsic::GenISA_floatatomicrawA64)
        {
            Function* F = pInst->getParent()->getParent();

            unsigned short bitwidth = pInst->getType()->getScalarSizeInBits();
            //We cannot optimize float and double atomics if the flag "unsafe-fp-math" was not passed.
            if (id == GenISAIntrinsic::GenISA_floatatomicrawA64)
            {
                if (!F->hasFnAttribute("unsafe-fp-math") || !(F->getFnAttribute("unsafe-fp-math").getValueAsString() == "true") || bitwidth == 16)
                {
                    return false;
                }
            }

            if (bitwidth == 64)
            {
                AtomicOp atomic_op = static_cast<AtomicOp>(llvm::cast<llvm::ConstantInt>(pInst->getOperand(3))->getZExtValue());

                bool is_i64 = (atomic_op == EATOMIC_IADD64 || atomic_op == EATOMIC_SUB64);
                bool is_f64 = (atomic_op == EATOMIC_FADD64 || atomic_op == EATOMIC_FSUB64);

                if ((!is_i64 || !m_currShader->m_Platform->hasInt64Add()) && (!is_f64 || !m_currShader->m_Platform->hasFP64GlobalAtomicAdd()))
                {
                    return false;
                }
            }

            if (IGC_IS_FLAG_ENABLED(DisableScalarAtomics) ||
                F->hasFnAttribute("KMPLOCK") ||
                m_currShader->m_DriverInfo->WASLMPointersDwordUnit())
                return false;
            llvm::Value* pllbuffer = pInst->getOperand(0);
            llvm::Value* pllDstAddr = pInst->getOperand(1);

            if (m_currShader->GetIsUniform(pllDstAddr) && m_currShader->GetIsUniform(pllbuffer))
            {
                AtomicOp atomic_op = static_cast<AtomicOp>(llvm::cast<llvm::ConstantInt>(pInst->getOperand(3))->getZExtValue());

                bool isAtomicAdd =
                    atomic_op == EATOMIC_IADD ||
                    atomic_op == EATOMIC_IADD64 ||
                    atomic_op == EATOMIC_SUB64 ||
                    atomic_op == EATOMIC_FADD64 ||
                    atomic_op == EATOMIC_FSUB64 ||
                    atomic_op == EATOMIC_INC ||
                    atomic_op == EATOMIC_SUB ||
                    atomic_op == EATOMIC_DEC ||
                    atomic_op == EATOMIC_FADD ||
                    atomic_op == EATOMIC_FSUB;
                bool isAtomicMinMax =
                    atomic_op == EATOMIC_UMAX ||
                    atomic_op == EATOMIC_UMIN ||
                    atomic_op == EATOMIC_IMIN ||
                    atomic_op == EATOMIC_IMAX;
                bool isAtomicOr  = (atomic_op == EATOMIC_OR);
                bool isAtomicAnd = (atomic_op == EATOMIC_AND);
                bool isAtomicXor = (atomic_op == EATOMIC_XOR);
                if (isAtomicAdd ||
                    (pInst->use_empty() &&
                        (isAtomicMinMax || isAtomicOr || isAtomicAnd || isAtomicXor)))
                    return true;
            }
        }
        if (id == GenISAIntrinsic::GenISA_intatomictyped)
        {
            if (IGC_IS_FLAG_DISABLED(EnableScalarTypedAtomics))
                return false;

            llvm::Value* pllbuffer = pInst->getOperand(0);
            llvm::Value* pllU      = pInst->getOperand(1);
            llvm::Value* pllV      = pInst->getOperand(2);
            llvm::Value* pllR      = pInst->getOperand(3);

            CVariable* pDstBuffer = GetSymbol(pllbuffer);
            CVariable* pU         = GetSymbol(pllU);
            CVariable* pV         = GetSymbol(pllV);
            CVariable* pR         = GetSymbol(pllR);

            // mostly care for pU nonzero, rest undef; but other's are good as well
            if (pDstBuffer->IsUniform() &&
                (pU->IsUniform() || pU->IsUndef()) &&
                (pV->IsUniform() || pV->IsUndef()) &&
                (pR->IsUniform() || pR->IsUndef()))
            {
                AtomicOp atomic_op = static_cast<AtomicOp>(llvm::cast<llvm::ConstantInt>(pInst->getOperand(5))->getZExtValue());

                bool isAddAtomic =
                    atomic_op == EATOMIC_IADD ||
                    atomic_op == EATOMIC_INC ||
                    atomic_op == EATOMIC_SUB ||
                    atomic_op == EATOMIC_DEC;
                bool isMinMaxAtomic =
                    atomic_op == EATOMIC_UMAX ||
                    atomic_op == EATOMIC_UMIN ||
                    atomic_op == EATOMIC_IMIN ||
                    atomic_op == EATOMIC_IMAX;

                bool isOrAtomic = (atomic_op == EATOMIC_OR);
                if (isAddAtomic || (isMinMaxAtomic && pInst->use_empty()) ||
                    (isOrAtomic && pInst->use_empty()))
                    return true;
            }
        }
        if (id == GenISAIntrinsic::GenISA_atomiccounterinc ||
            id == GenISAIntrinsic::GenISA_atomiccounterpredec)
        {
            if (m_currShader->m_Platform->supportsBinaryAtomicCounterMessage())
            {
                return true;
            }
        }
    }

    return false;
}

CVariable* EmitPass::UnpackOrBroadcastIfUniform(CVariable* pVar)
{
    if (pVar->GetElemSize() == 4 || pVar->GetElemSize() == 8)
        return BroadcastIfUniform(pVar);

    IGC_ASSERT(pVar->GetElemSize() == 2);

    uint16_t elts = numLanes(m_currShader->m_SIMDSize);
    // 16-bit atomics are still aligned at dword boundaries
    // with the upper 16-bits ignored.
    CVariable* pUnpacked =
        m_currShader->GetNewVariable(elts, ISA_TYPE_UD, EALIGN_GRF, CName(pVar->getName(), "Unpacked"));

    m_encoder->Cast(pUnpacked, m_currShader->BitCast(pVar, ISA_TYPE_UW));
    return pUnpacked;
}

void EmitPass::emitAtomicRaw(llvm::GenIntrinsicInst *pInst, Value *dstAddr,
                             ConstantInt *immOffset, ConstantInt *immScale
  )
{
    ForceDMask();
    // Currently, Dword Atomics can be called by matching 2 intrinsics. One is the DwordAtomicRaw
    // and AtomicCmpXchg (which has 2 srcs unlike the other atomics).
    IGC_ASSERT(IGCLLVM::getNumArgOperands(pInst) == 4);

    /// Immediate Atomics return the value before the atomic operation is performed. So that flag
    /// needs to be set for this.
    bool returnsImmValue = !pInst->use_empty();

    llvm::Value* pllbuffer = pInst->getOperand(0);
    if (!dstAddr) dstAddr = pInst->getOperand(1);
    llvm::Value* pllSrc0 = pInst->getOperand(2);
    ResourceDescriptor resource = GetResourceVariable(pllbuffer);
    CountStatelessIndirectAccess(pllbuffer, resource);
    AtomicOp atomic_op = EATOMIC_UNDEF;

    if (pllbuffer->getType()->getPointerAddressSpace() == ADDRESS_SPACE_GLOBAL)
    {
        m_currShader->SetHasGlobalAtomics();

        // Global atomics with nullptr lead to GPU hang. Emit warning to avoid time-consuming debugging process.
        if (resource.m_surfaceType == ESURFACE_STATELESS && isa<llvm::ConstantPointerNull>(pllbuffer))
        {
            m_pCtx->EmitWarning("Possible null pointer dereference! Atomic instruction operates on a nullptr.", pInst);
        }
    }

    CVariable* pSrc0 = nullptr;
    CVariable* pSrc1 = nullptr;
    llvm::GenIntrinsicInst* pIntrinCall = llvm::cast<llvm::GenIntrinsicInst>(pInst);
    GenISAIntrinsic::ID IID = pIntrinCall->getIntrinsicID();
    if (IID == GenISAIntrinsic::GenISA_icmpxchgatomicraw ||
        IID == GenISAIntrinsic::GenISA_fcmpxchgatomicraw ||
        IID == GenISAIntrinsic::GenISA_icmpxchgatomicrawA64 ||
        IID == GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64)
    {
        llvm::Value* pllSrc1 = pInst->getOperand(3);
        pSrc1 = GetSymbol(pllSrc1);

        Function* F = pInst->getParent()->getParent();
        if (F->hasFnAttribute("KMPLOCK") && m_currShader->GetIsUniform(pInst))
        {
            m_encoder->SetSimdSize(SIMDMode::SIMD1);
            m_encoder->SetNoMask();
        }

        pSrc1 = UnpackOrBroadcastIfUniform(pSrc1);
        if (IID == GenISAIntrinsic::GenISA_fcmpxchgatomicraw ||
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
        atomic_op = static_cast<AtomicOp>(llvm::cast<llvm::ConstantInt>(pInst->getOperand(3))->getZExtValue());
    }


    unsigned short bitwidth = pInst->getType()->getScalarSizeInBits();
    const bool is16Bit = (bitwidth == 16);

    if (is16Bit)
    {
        IGC_ASSERT_MESSAGE(m_currShader->m_Platform->support16BitAtomics(), "16-bit atomics are not supported on this platform!");
    }

    // atomic_inc and atomic_dec don't have both src0 and src1.
    if (atomic_op != EATOMIC_INC && atomic_op != EATOMIC_DEC &&
        atomic_op != EATOMIC_INC64 && atomic_op != EATOMIC_DEC64 &&
        atomic_op != EATOMIC_PREDEC && atomic_op != EATOMIC_PREDEC64)
    {
        pSrc0 = GetSymbol(pllSrc0);
    }

    // Dst address in bytes.
    CVariable* pDstAddr = GetSymbol(dstAddr);


    PointerType *PtrTy = dyn_cast<PointerType>(dstAddr->getType());
    bool isA64 = PtrTy && isA64Ptr(PtrTy, m_currShader->GetContext());
    LSC_ADDR_SIZE addrSize = isA64 ? LSC_ADDR_SIZE_64b : LSC_ADDR_SIZE_32b;

    const int immOffsetVal =
        immOffset ? static_cast<int>(immOffset->getSExtValue()) : 0;
    const int immScaleVal =
        immScale ? static_cast<int>(immScale->getSExtValue()) : 1;

    // If DisableScalarAtomics regkey is enabled or DisableIGCOptimizations regkey is enabled then
    // don't enable scalar atomics
    if (IsUniformAtomic(pInst))
    {
        e_alignment uniformAlign = isA64 ? EALIGN_2GRF : EALIGN_GRF;
        // Re-align the pointer if it's not GRF aligned.
        pDstAddr = ReAlignUniformVariable(pDstAddr, uniformAlign);
        if (atomic_op == EATOMIC_OR && OrWith0Atomic(pInst, 2))
        {
            // special case of atomic_load
            emitScalarAtomicLoad(pInst, resource,
                                 pDstAddr, nullptr /*u*/, nullptr /*v*/,
                                 nullptr /*r*/, pSrc0, isA64, bitwidth,
                                 immOffsetVal, immScaleVal, addrSize);
        }
        else {
            emitScalarAtomics(pInst, resource, atomic_op,
                              pDstAddr, nullptr /*u*/, nullptr /*v*/,
                              nullptr /*r*/, pSrc0, isA64, bitwidth,
                              immOffsetVal, immScaleVal, addrSize);
            ResetVMask();
        }
        return;
    }

    Function* F = pInst->getParent()->getParent();
    if (F->hasFnAttribute("KMPLOCK") && m_currShader->GetIsUniform(pInst))
    {
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
    }
    pDstAddr = BroadcastIfUniform(pDstAddr);

    if (F->hasFnAttribute("KMPLOCK") && m_currShader->GetIsUniform(pInst))
    {
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
    }
    if (pSrc0)
    {
        pSrc0 = UnpackOrBroadcastIfUniform(pSrc0);
    }

    if (F->hasFnAttribute("KMPLOCK") && m_currShader->GetIsUniform(pInst))
    {
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
    }

    {
        CVariable* pDst = returnsImmValue ?
            m_currShader->GetNewVariable(
                numLanes(m_currShader->m_SIMDSize),
                bitwidth != 64 ? ISA_TYPE_UD : ISA_TYPE_UQ,
                EALIGN_GRF, CName::NONE) :
            nullptr;

        bool extendPointer = (bitwidth == 64 && !isA64);
        // DG2 onward with LSC we do not have to extend an A32 pointer to an
        // A64 pointer for 64bit atomics
        extendPointer = extendPointer && !shouldGenerateLSC();
        if (isA64 || extendPointer)
        {
            if (extendPointer)
            {
                pDstAddr = m_currShader->BitCast(pDstAddr, GetUnsignedIntegerType(pDstAddr->GetType()));
                CVariable* pDstAddr2 = m_currShader->GetNewVariable(
                    pDstAddr->GetNumberElement(), ISA_TYPE_UQ, EALIGN_GRF, CName::NONE);
                m_encoder->Cast(pDstAddr2, pDstAddr);
                m_encoder->AtomicRawA64(atomic_op, resource, pDst, pDstAddr2, pSrc0, pSrc1, bitwidth);
                m_encoder->Push();
            }
            else
            {
                if (shouldGenerateLSC()) {
                    auto cacheOpts = LSC_DEFAULT_CACHING;
                    m_encoder->LSC_AtomicRaw(atomic_op, pDst,
                                             pDstAddr, pSrc0, pSrc1, bitwidth,
                                             &resource, addrSize, immOffsetVal,
                                             immScaleVal, cacheOpts);
                }
                else
                {
                    m_encoder->AtomicRawA64(atomic_op, resource, pDst, pDstAddr, pSrc0, pSrc1, bitwidth);
                }
                m_encoder->Push();
            }

            if (returnsImmValue) //This is needed for repacking of 16bit atomics otherwise it will be a vanilla mov
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
            if (resource.m_surfaceType == ESURFACE_SLM && ctx->m_DriverInfo.WASLMPointersDwordUnit())
            {
                CVariable* pDwordAddr =
                    m_currShader->GetNewVariable(numLanes(m_currShader->m_SIMDSize),
                        ISA_TYPE_D, EALIGN_GRF, CName::NONE);

                m_encoder->Shl(pDwordAddr, pDstAddr,
                    m_currShader->ImmToVariable(0x2, ISA_TYPE_D));
                m_encoder->Push();
                pDstAddr = pDwordAddr;
            }
            pDstAddr = m_currShader->BitCast(pDstAddr, ISA_TYPE_UD);

            if (pSrc0)
            {
                pSrc0 = m_currShader->BitCast(pSrc0, bitwidth != 64 ? ISA_TYPE_UD : ISA_TYPE_UQ);
            }

            if (pSrc1)
            {
                pSrc1 = m_currShader->BitCast(pSrc1, bitwidth != 64 ? ISA_TYPE_UD : ISA_TYPE_UQ);
            }
            uint label = 0;
            CVariable* flag = nullptr;
            bool needLoop = ResourceLoopHeader(resource, flag, label);
            ResourceLoopSubIteration(resource, flag, label);
            if (shouldGenerateLSC(pInst)) {
                auto cacheOpts = LSC_DEFAULT_CACHING;
                m_encoder->LSC_AtomicRaw(atomic_op, pDst,
                                         pDstAddr, pSrc0, pSrc1, bitwidth,
                                         &resource, addrSize, immOffsetVal,
                                         immScaleVal, cacheOpts);
            }
            else
            {
                IGC_ASSERT_MESSAGE(!immScale && !immOffset, "Scale and offset not supported on non-LSC path!");
                m_encoder->DwordAtomicRaw(
                    atomic_op,
                    resource,
                    pDst,
                    pDstAddr,
                    pSrc0,
                    pSrc1,
                    is16Bit);
            }
            m_encoder->Push();
            if (returnsImmValue)
            {
                m_encoder->Cast(
                    m_currShader->BitCast(m_destination, GetUnsignedIntegerType(m_destination->GetType())),
                    pDst);
                m_encoder->Push();
            }
            ResourceLoopBackEdge(needLoop, flag, label);
        }


    }
    ResetVMask();
    m_currShader->m_State.isMessageTargetDataCacheDataPort = true;
}

void EmitPass::emitAtomicTyped(GenIntrinsicInst* pInsn)
{
    ForceDMask();
    // Currently, Dword Atomics can be called by matching 2 intrinsics. One is the DwordAtomicRaw
    // and AtomicCmpXchg (which has 2 srcs unlike the other atomics).
    IGC_ASSERT(IGCLLVM::getNumArgOperands(pInsn) == 6);

    /// Immediate Atomics return the value before the atomic operation is performed. So that flag
    /// needs to be set for this.
    bool returnsImmValue = !pInsn->user_empty();

    llvm::Value* pllbuffer = pInsn->getOperand(0);
    llvm::Value* pllU = pInsn->getOperand(1);
    llvm::Value* pllV = pInsn->getOperand(2);
    llvm::Value* pllR = pInsn->getOperand(3);
    llvm::Value* pllSrc0 = pInsn->getOperand(4);

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
    }

    ResourceDescriptor resource = GetResourceVariable(pllbuffer);

    CVariable* pU = GetSymbol(pllU);
    CVariable* pV = GetSymbol(pllV);
    CVariable* pR = GetSymbol(pllR);

    unsigned bitwidth = pInsn->getType()->getScalarSizeInBits();

    if (IsUniformAtomic(pInsn))
    {
        pU = ReAlignUniformVariable(pU, EALIGN_GRF);
        pV = ReAlignUniformVariable(pV, EALIGN_GRF);
        pR = ReAlignUniformVariable(pR, EALIGN_GRF);

        if (atomic_op == EATOMIC_OR && OrWith0Atomic(pInsn, 4))
        {
            // special case of atomic_load
                emitScalarAtomicLoad(pInsn, resource,
                                     nullptr /*pDstAddr*/, pU, pV, pR, pSrc0,
                                     false /*isA64*/, bitwidth, 0, 1,
                                     LSC_ADDR_SIZE_32b);
        }
        else
        {
            emitScalarAtomics(pInsn, resource, atomic_op,
                              nullptr /*pDstAddr*/, pU, pV, pR, pSrc0,
                              false /*isA64*/, bitwidth, 0, 1,
                              LSC_ADDR_SIZE_32b);
        }
    }
    else
    {
        pU = BroadcastIfUniform(pU);
        pV = BroadcastIfUniform(pV);
        pR = BroadcastIfUniform(pR);

        if (pSrc0)
        {
            pSrc0 = UnpackOrBroadcastIfUniform(pSrc0);
        }

        uint addrDimension = 3;
        while (addrDimension > 1 && isUndefOrConstInt0(pInsn->getOperand(addrDimension)))
        {
            addrDimension--;
        }

        const bool is16Bit = (bitwidth == 16);

        CVariable* tempdst = returnsImmValue ?
            m_currShader->GetNewVariable(
                numLanes(m_currShader->m_SIMDSize), ISA_TYPE_UD, EALIGN_GRF, CName::NONE) :
            nullptr;

        CVariable* splitpU[2] = { nullptr, nullptr };
        CVariable* splitpV[2] = { nullptr, nullptr };
        CVariable* splitpR[2] = { nullptr, nullptr };
        CVariable* splitpSrc0[2] = { nullptr, nullptr };
        CVariable* splitpSrc1[2] = { nullptr, nullptr };

        const unsigned int numLanesForSimd8 = numLanes(SIMDMode::SIMD8);
        IGC_ASSERT(numLanesForSimd8);
        const unsigned int loopIter = numLanes(m_currShader->m_SIMDSize) / numLanesForSimd8;

        for (uint i = 0; i < loopIter; ++i)
        {
            auto CopyVar = [&](CVariable* pVar, CVariable* dst)
            {
                m_encoder->SetSimdSize(SIMDMode::SIMD8);
                m_encoder->SetMask((i == 0) ? EMASK_Q1 : EMASK_Q2);
                if (!pVar->IsUniform())
                {
                    m_encoder->SetSrcSubVar(0, i);
                }
                m_encoder->Copy(dst, pVar);
                m_encoder->Push();
            };

            splitpU[i] = m_currShader->GetNewVariable(
                numLanes(SIMDMode::SIMD8),
                ISA_TYPE_UD,
                EALIGN_GRF,
                CName::NONE);

            CopyVar(pU, splitpU[i]);

            if (addrDimension > 1)
            {
                splitpV[i] = m_currShader->GetNewVariable(
                    numLanes(SIMDMode::SIMD8),
                    ISA_TYPE_UD,
                    EALIGN_GRF,
                    CName::NONE);

                CopyVar(pV, splitpV[i]);
            }

            if (addrDimension > 2)
            {
                splitpR[i] = m_currShader->GetNewVariable(
                    numLanes(SIMDMode::SIMD8),
                    ISA_TYPE_UD,
                    EALIGN_GRF,
                    CName::NONE);

                CopyVar(pR, splitpR[i]);
            }

            if (pSrc0)
            {
                splitpSrc0[i] = m_currShader->GetNewVariable(
                    numLanes(SIMDMode::SIMD8),
                    ISA_TYPE_UD,
                    EALIGN_GRF,
                    CName::NONE);

                CopyVar(pSrc0, splitpSrc0[i]);
            }

            if (pSrc1)
            {
                splitpSrc1[i] = m_currShader->GetNewVariable(
                    numLanes(SIMDMode::SIMD8),
                    ISA_TYPE_UD,
                    EALIGN_GRF,
                    CName::NONE);

                CopyVar(pSrc1, splitpSrc1[i]);
            }
        }

        uint label = 0;
        CVariable* flag = nullptr;
        bool needLoop = ResourceLoopHeader(resource, flag, label);
        ResourceLoopSubIteration(resource, flag, label);
        for (uint i = 0; i < loopIter; ++i)
        {
            m_encoder->SetPredicate(flag);
            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            m_encoder->SetMask((i == 0) ? EMASK_Q1 : EMASK_Q2);
            m_encoder->SetDstSubVar(i);
            m_encoder->TypedAtomic(atomic_op, tempdst, resource,
                splitpU[i], splitpV[i], splitpR[i], splitpSrc0[i], splitpSrc1[i], nullptr /*lod*/, is16Bit);
            m_encoder->Push();
        }
        ResourceLoopBackEdge(needLoop, flag, label);

        if (returnsImmValue)
        {
            m_encoder->Cast(
                m_currShader->BitCast(m_destination, GetUnsignedIntegerType(m_destination->GetType())),
                tempdst);
            m_encoder->Push();
        }
    }
    ResetVMask();
    m_currShader->m_State.isMessageTargetDataCacheDataPort = true;
}

void setSIMDSizeMask(CEncoder* m_encoder, const CShader* m_currShader, int i)
{
    m_encoder->SetSimdSize(SIMDMode::SIMD8);
    m_encoder->SetMask((i == 0) ? EMASK_Q1 : EMASK_Q2);

    if (m_currShader->m_numberInstance == 1 &&
        m_currShader->m_SIMDSize == SIMDMode::SIMD32)
    {
        m_encoder->SetSimdSize(SIMDMode::SIMD16);
        m_encoder->SetMask((i == 0) ? EMASK_H1 : EMASK_H2);
    }
}

void EmitPass::emitTypedRead(llvm::Instruction* pInsn)
{
    const CShader::ExtractMaskWrapper writeMask(m_currShader, pInsn);
    IGC_ASSERT_MESSAGE(writeMask.hasEM() && writeMask.getEM() != 0, "Wrong write mask");

    llvm::Value* pllSrcBuffer = pInsn->getOperand(0);
    llvm::Value* pllU = pInsn->getOperand(1);
    llvm::Value* pllV = pInsn->getOperand(2);
    llvm::Value* pllR = pInsn->getOperand(3);
    llvm::Value* pllLOD = getOperandIfExist(pInsn, 4);

    CVariable* pLOD = isUndefOrConstInt0(pllLOD) ? nullptr : GetSymbol(pllLOD);
    CVariable* pR = (pLOD == nullptr && isUndefOrConstInt0(pllR)) ? nullptr : GetSymbol(pllR);
    CVariable* pV = (pR == nullptr && isUndefOrConstInt0(pllV)) ? nullptr : GetSymbol(pllV);
    CVariable* pU = GetSymbol(pllU);

    pU = BroadcastIfUniform(pU, m_currShader->GetIsUniform(pInsn));
    pV = pV ? BroadcastIfUniform(pV, m_currShader->GetIsUniform(pInsn)) : nullptr;
    pR = pR ? BroadcastIfUniform(pR, m_currShader->GetIsUniform(pInsn)) : nullptr;
    pLOD = pLOD ? BroadcastIfUniform(pLOD, m_currShader->GetIsUniform(pInsn)) : nullptr;

    ResourceDescriptor resource = GetResourceVariable(pllSrcBuffer);
    LSC_CACHE_OPTS cacheOpts = translateLSCCacheControlsFromMetadata(pInsn, true, true);

    uint numChannels = iSTD::BitCount(writeMask.getEM());
    auto doLSC = shouldGenerateLSC(pInsn, true);
    doLSC = doLSC && IGC_IS_FLAG_DISABLED(DisableLSCForTypedUAV);
    LSC_ADDR_SIZE addrSize = LSC_ADDR_SIZE_32b;
    if (m_currShader->GetIsUniform(pInsn))
    {
        SIMDMode nativeDispatchMode = m_currShader->m_Platform->getMinDispatchMode();
        CVariable* tempdst = nullptr;
        tempdst = m_currShader->GetNewVariable(
            numChannels * numLanes(nativeDispatchMode),
            ISA_TYPE_F,
            EALIGN_GRF,
            CName("tyReadDest"));
        m_encoder->SetSimdSize(nativeDispatchMode);
        m_encoder->SetPredicate(nullptr);
        m_encoder->SetNoMask();

        if (doLSC)
        {
            m_encoder->LSC_TypedReadWrite(LSC_LOAD_QUAD, &resource, pU, pV, pR, pLOD, tempdst, 4 * 8,
                numLanes(nativeDispatchMode), addrSize, writeMask.getEM(), cacheOpts);
        }
        else
        {
            m_encoder->TypedRead4(resource, pU, pV, pR, pLOD, tempdst, writeMask.getEM());
        }

        m_encoder->Push();

        // Mov the required channel values to m_destination
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();

        for (uint i = 0; i < numChannels; ++i)
        {
            m_encoder->SetSrcSubReg(0, i * numLanes(nativeDispatchMode));
            m_encoder->SetDstSubReg(i);
            m_encoder->Copy(m_destination, tempdst);
            m_encoder->Push();
        }
    }
    else
    {
        uint label = 0;
        CVariable* flag = nullptr;
        bool needLoop = ResourceLoopHeader(resource, flag, label);
        ResourceLoopSubIteration(resource, flag, label);
        CVariable* tempdst[4] = { nullptr, nullptr, nullptr, nullptr };
        SIMDMode instWidth = std::min(
            m_currShader->m_Platform->supportsSIMD16TypedRW() ? SIMDMode::SIMD16 : SIMDMode::SIMD8,
            m_currShader->m_SIMDSize);
        bool needsSplit = m_currShader->m_SIMDSize > instWidth;
        if (doLSC)
        {
            needsSplit = m_currShader->m_SIMDSize == SIMDMode::SIMD32;
            instWidth = SIMDMode::SIMD16;
        }

        if (!needsSplit)
        {
            m_encoder->SetPredicate(flag);

            if (doLSC)
            {
                m_encoder->LSC_TypedReadWrite(LSC_LOAD_QUAD, &resource, pU, pV, pR, pLOD, m_destination, 4 * 8,
                    numLanes(SIMDMode::SIMD16), addrSize, writeMask.getEM(), cacheOpts);
            }
            else
            {
                m_encoder->TypedRead4(resource, pU, pV, pR, pLOD, m_destination, writeMask.getEM());
            }

            m_encoder->Push();
        }
        else
        {
            const unsigned int numLanesForInstWidth = numLanes(instWidth);
            IGC_ASSERT(numLanesForInstWidth);
            const unsigned int splitInstCount = numLanes(m_currShader->m_SIMDSize) / numLanesForInstWidth;
            IGC_ASSERT(splitInstCount <= ARRAY_COUNT(tempdst));

            for (uint i = 0; i < splitInstCount; ++i)
            {
                tempdst[i] = m_currShader->GetNewVariable(
                    numChannels * numLanes(instWidth),
                    ISA_TYPE_F,
                    EALIGN_GRF,
                    CName::NONE);

                setSIMDSizeMask(m_encoder, m_currShader, i);
                m_encoder->SetSrcSubVar(0, i);
                m_encoder->SetSrcSubVar(1, i);
                m_encoder->SetSrcSubVar(2, i);
                m_encoder->SetSrcSubVar(3, i);
                m_encoder->SetPredicate(flag);

                if (doLSC)
                {
                    m_encoder->LSC_TypedReadWrite(LSC_LOAD_QUAD, &resource, pU, pV, pR, pLOD, tempdst[i], 4 * 8,
                        numLanes(SIMDMode::SIMD16), addrSize, writeMask.getEM(), cacheOpts);
                }
                else
                {
                    m_encoder->TypedRead4(resource, pU, pV, pR, pLOD, tempdst[i], writeMask.getEM());
                }

                m_encoder->Push();
            }
        }
        ResourceLoopBackEdge(needLoop, flag, label);

        if (m_currShader->m_SIMDSize != instWidth)
        {
            JoinSIMD(tempdst, numChannels, instWidth);
        }
    }
    m_currShader->m_State.isMessageTargetDataCacheDataPort = true;
}

void EmitPass::emitTypedWrite(llvm::Instruction* pInsn)
{
    ForceDMask();
    llvm::Value* pllDstBuffer = pInsn->getOperand(0);
    llvm::Value* pllU = pInsn->getOperand(1);
    llvm::Value* pllV = pInsn->getOperand(2);
    llvm::Value* pllR = pInsn->getOperand(3);
    llvm::Value* pllLOD = pInsn->getOperand(4);
    llvm::Value* pllSrc_X = pInsn->getOperand(5);
    llvm::Value* pllSrc_Y = pInsn->getOperand(6);
    llvm::Value* pllSrc_Z = pInsn->getOperand(7);
    llvm::Value* pllSrc_W = pInsn->getOperand(8);

    CVariable* pLOD = isUndefOrConstInt0(pllLOD) ? nullptr : GetSymbol(pllLOD);
    CVariable* pR = (pLOD == nullptr && isUndefOrConstInt0(pllR)) ? nullptr : GetSymbol(pllR);
    CVariable* pV = (pR == nullptr && isUndefOrConstInt0(pllV)) ? nullptr : GetSymbol(pllV);
    CVariable* pU = GetSymbol(pllU);

    CVariable* pSrc_X = GetSymbol(pllSrc_X);
    CVariable* pSrc_Y = GetSymbol(pllSrc_Y);
    CVariable* pSrc_Z = GetSymbol(pllSrc_Z);
    CVariable* pSrc_W = GetSymbol(pllSrc_W);

    pU = BroadcastIfUniform(pU);
    pV = pV ? BroadcastIfUniform(pV) : nullptr;
    pR = pR ? BroadcastIfUniform(pR) : nullptr;
    pLOD = pLOD ? BroadcastIfUniform(pLOD) : nullptr;

    uint writeMask =
        (!llvm::isa<UndefValue>(pllSrc_X) ? 1 : 0) |
        (!llvm::isa<UndefValue>(pllSrc_Y) ? 2 : 0) |
        (!llvm::isa<UndefValue>(pllSrc_Z) ? 4 : 0) |
        (!llvm::isa<UndefValue>(pllSrc_W) ? 8 : 0);

    ResourceDescriptor resource = GetResourceVariable(pllDstBuffer);
    LSC_CACHE_OPTS cacheOpts = translateLSCCacheControlsFromMetadata(pInsn, false, true);
    m_currShader->m_State.HasLscStoreCacheControls(cacheOpts);
    LSC_ADDR_SIZE addrSize = LSC_ADDR_SIZE_32b;
    if (m_currShader->GetIsUniform(pInsn))
    {
        IGC_ASSERT_MESSAGE(0, "Uniform store_uav_typed not implemented yet");
    }
    else
    {
        uint label = 0;
        CVariable* flag = nullptr;
        bool needLoop = ResourceLoopHeader(resource, flag, label);
        ResourceLoopSubIteration(resource, flag, label);
        uint parameterLength = 4;

        SIMDMode instWidth = std::min(
            m_currShader->m_Platform->supportsSIMD16TypedRW() ? SIMDMode::SIMD16 : SIMDMode::SIMD8,
            m_currShader->m_SIMDSize);
        bool needsSplit = m_currShader->m_SIMDSize > instWidth;

        auto doLSC = shouldGenerateLSC(pInsn, true);
        doLSC &= IGC_IS_FLAG_DISABLED(DisableLSCForTypedUAV);

        if (doLSC)  // Reset if the SIMD32 is enabled
        {
            needsSplit = m_currShader->m_SIMDSize == SIMDMode::SIMD32;
            instWidth = SIMDMode::SIMD16;
        }

        if (!needsSplit)
        {
            Constant* p_X = dyn_cast<Constant>(pllSrc_X);
            Constant* p_Y = dyn_cast<Constant>(pllSrc_Y);
            Constant* p_Z = dyn_cast<Constant>(pllSrc_Z);
            Constant* p_W = dyn_cast<Constant>(pllSrc_W);

            CVariable* pPayload = nullptr;
            if (p_X && p_Y && p_Z && p_W) {
                // if x, y, z, w are constants, find opportunities to reuse
                std::vector<Constant*> xyzwPayload{ p_X, p_Y, p_Z, p_W };
                llvm::Constant* payloadConstant = llvm::ConstantVector::get(xyzwPayload);
                pPayload = tryReusingXYZWPayload(payloadConstant, pInsn->getParent(),
                parameterLength * numLanes(m_currShader->m_SIMDSize), ISA_TYPE_F,
                pSrc_X, pSrc_Y, pSrc_Z, pSrc_W, 1);
            }

            if (!pPayload) {
                pPayload = m_currShader->GetNewVariable(
                    parameterLength * numLanes(m_currShader->m_SIMDSize),
                    ISA_TYPE_F,
                    EALIGN_GRF,
                    CName::NONE);
                // pSrcX, Y, Z & W are broadcast to uniform by this function itself.
                m_currShader->CopyVariable(pPayload, pSrc_X, 0);
                m_currShader->CopyVariable(pPayload, pSrc_Y, 1);
                m_currShader->CopyVariable(pPayload, pSrc_Z, 2);
                m_currShader->CopyVariable(pPayload, pSrc_W, 3);
            }
            m_encoder->SetPredicate(flag);
            if (doLSC)
            {
                m_encoder->LSC_TypedReadWrite(LSC_STORE_QUAD, &resource, pU, pV, pR, pLOD, pPayload, 4 * 8, parameterLength * numLanes(SIMDMode::SIMD16), addrSize, writeMask, cacheOpts);
            }
            else
            {
                m_encoder->TypedWrite4(resource, pU, pV, pR, pLOD, pPayload, writeMask);
            }

            m_encoder->Push();
        }
        else
        {
            IGC_ASSERT(instWidth == SIMDMode::SIMD8 ||
                instWidth == SIMDMode::SIMD16);
            IGC_ASSERT(m_currShader->m_SIMDSize > instWidth);
            const uint numInst = numLanes(m_currShader->m_SIMDSize) / numLanes(instWidth);
            std::vector<CVariable*> pPayload(numInst);
            for (uint i = 0; i < numInst; ++i)
            {
                pPayload[i] = m_currShader->GetNewVariable(
                    parameterLength * numLanes(instWidth),
                    ISA_TYPE_F,
                    EALIGN_GRF, CName::NONE);
                setSIMDSizeMask(m_encoder, m_currShader, i);
                if (!pSrc_X->IsUniform())
                {
                    m_encoder->SetSrcSubVar(0, i);
                }
                m_encoder->SetDstSubVar(0);
                m_encoder->Copy(pPayload[i], pSrc_X);
                m_encoder->Push();

                setSIMDSizeMask(m_encoder, m_currShader, i);
                if (!pSrc_Y->IsUniform())
                {
                    m_encoder->SetSrcSubVar(0, i);
                }
                m_encoder->SetDstSubVar(1);
                m_encoder->Copy(pPayload[i], pSrc_Y);
                m_encoder->Push();

                setSIMDSizeMask(m_encoder, m_currShader, i);
                if (!pSrc_Z->IsUniform())
                {
                    m_encoder->SetSrcSubVar(0, i);
                }
                m_encoder->SetDstSubVar(2);
                m_encoder->Copy(pPayload[i], pSrc_Z);
                m_encoder->Push();

                setSIMDSizeMask(m_encoder, m_currShader, i);
                if (!pSrc_W->IsUniform())
                {
                    m_encoder->SetSrcSubVar(0, i);
                }
                m_encoder->SetDstSubVar(3);
                m_encoder->Copy(pPayload[i], pSrc_W);
                m_encoder->Push();
                if (!m_currShader->m_Platform->canFuseTypedWrite())
                {
                    setSIMDSizeMask(m_encoder, m_currShader, i);
                    m_encoder->SetSrcSubVar(0, i);
                    m_encoder->SetSrcSubVar(1, i);
                    m_encoder->SetSrcSubVar(2, i);
                    m_encoder->SetSrcSubVar(3, i);
                    m_encoder->SetPredicate(flag);
                    if (doLSC)
                    {
                        m_encoder->LSC_TypedReadWrite(LSC_STORE_QUAD, &resource, pU, pV, pR, pLOD, pPayload[i], 4 * 8, parameterLength * numLanes(SIMDMode::SIMD16), addrSize, writeMask, cacheOpts);
                    }
                    else
                    {
                        m_encoder->TypedWrite4(resource, pU, pV, pR, pLOD, pPayload[i], writeMask);
                    }
                    m_encoder->Push();
                }
            }
            if (m_currShader->m_Platform->canFuseTypedWrite())
            {
                for (uint i = 0; i < numInst; ++i)
                {
                    setSIMDSizeMask(m_encoder, m_currShader, i);
                    m_encoder->SetSrcSubVar(0, i);
                    m_encoder->SetSrcSubVar(1, i);
                    m_encoder->SetSrcSubVar(2, i);
                    m_encoder->SetSrcSubVar(3, i);
                    m_encoder->SetPredicate(flag);
                    if (doLSC)
                    {
                        m_encoder->LSC_TypedReadWrite(LSC_STORE_QUAD, &resource, pU, pV, pR, pLOD, pPayload[i], 4 * 8, parameterLength * numLanes(SIMDMode::SIMD16), addrSize, writeMask, cacheOpts);
                    }
                    else
                    {
                        m_encoder->TypedWrite4(resource, pU, pV, pR, pLOD, pPayload[i], writeMask);
                    }
                    m_encoder->Push();
                }
            }
        }
        ResourceLoopBackEdge(needLoop, flag, label);
    }
    ResetVMask();
    m_currShader->m_State.isMessageTargetDataCacheDataPort = true;
}

void EmitPass::emitThreadGroupNamedBarriersSignal(llvm::Instruction* inst)
{
    CVariable* barrierID = m_currShader->ImmToVariable(
        cast<llvm::ConstantInt>(inst->getOperand(0))->getSExtValue(), ISA_TYPE_UD);
    CVariable* barrierType = m_currShader->ImmToVariable(
        cast<llvm::ConstantInt>(inst->getOperand(1))->getSExtValue(), ISA_TYPE_UW);
    CVariable* producerCnt = m_currShader->ImmToVariable(
        cast<llvm::ConstantInt>(inst->getOperand(2))->getSExtValue(), ISA_TYPE_UD);
    CVariable* consumerCnt = m_currShader->ImmToVariable(
        cast<llvm::ConstantInt>(inst->getOperand(3))->getSExtValue(), ISA_TYPE_UD);
    m_encoder->NamedBarrier(EBARRIER_SIGNAL, barrierID, barrierType, producerCnt, consumerCnt);

    m_encoder->Push();
}

void EmitPass::emitThreadGroupNamedBarriersWait(llvm::Instruction* inst)
{
    CVariable* barrierID = m_currShader->ImmToVariable(
        cast<llvm::ConstantInt>(inst->getOperand(0))->getSExtValue(), ISA_TYPE_UD);

    m_encoder->NamedBarrier(EBARRIER_WAIT, barrierID, nullptr, nullptr, nullptr);
    m_encoder->Push();
}

static void divergentBarrierCheck(
    const CShader* Shader, const CodeGenContext &Ctx, const Instruction* I)
{
    if (IGC_IS_FLAG_DISABLED(EnableDivergentBarrierCheck))
        return;

    if (Shader->InsideWorkgroupDivergentCF(I))
    {
        Debug::DumpName name =
            IGC::Debug::GetDumpNameObj(Shader, "divergent_barrier.log");
        std::string Path = name.str();
        std::ofstream OS(Path, std::ios::app);
        if (OS.is_open())
        {
            std::string Repr;
            raw_string_ostream SS(Repr);
            I->print(SS, true);
            SS.flush();
            OS << '\n' << Repr;
            OS << "\nerror: ";
            Ctx.EmitMessage(OS, "Possible divergent barrier found", I);
            OS << "\nerror: backend compiler failed build.\n";
        }
    }
}

void EmitPass::emitThreadGroupBarrier(llvm::Instruction* inst)
{

    // OPT: Remove barrier instruction when thread group size is less or equal than simd size.
    bool skipBarrierInstructionInCS = false;
    if (m_currShader->GetShaderType() == ShaderType::OPENCL_SHADER) {
        Function* F = inst->getParent()->getParent();
        MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
        uint32_t sz = IGCMetaDataHelper::getThreadGroupSize(*pMdUtils, F);
        if (sz != 0 && sz <= numLanes(m_SimdMode)) {
            skipBarrierInstructionInCS = true;
        }
    }

    if (!skipBarrierInstructionInCS)
    {
        e_barrierKind BarrierKind = EBARRIER_NORMAL; // default
        GenIntrinsicInst* geninst = cast<GenIntrinsicInst>(inst);
        if (geninst->getIntrinsicID() == GenISAIntrinsic::GenISA_threadgroupbarrier_signal) {
            BarrierKind = EBARRIER_SIGNAL;
        }
        else if (geninst->getIntrinsicID() == GenISAIntrinsic::GenISA_threadgroupbarrier_wait) {
            BarrierKind = EBARRIER_WAIT;
        }
        m_encoder->Barrier(BarrierKind);
        m_encoder->Push();

        // Set if barrier was used for this function
        m_encoder->SetFunctionHasBarrier(inst->getFunction());

        divergentBarrierCheck(m_currShader, *m_pCtx, inst);
    }
}

LSC_FENCE_OP EmitPass::getLSCMemoryFenceOp(bool IsGlobalMemFence, bool InvalidateL1, bool EvictL1) const
{
    LSC_FENCE_OP op = LSC_FENCE_OP_NONE;
    if (InvalidateL1)
    {
        op = LSC_FENCE_OP_INVALIDATE;
    }

    if (EvictL1)
    {
        op = LSC_FENCE_OP_EVICT;
    }

    // For experiment on XeHP SDV
    if (op == LSC_FENCE_OP_NONE && IsGlobalMemFence && IGC_IS_FLAG_ENABLED(EnableL3FlushForGlobal))
    {
        op = LSC_FENCE_OP_FLUSHL3;
    }
    return op;
}

void EmitPass::emitMemoryFence(llvm::Instruction* inst)
{
    static constexpr int ExpectedNumberOfBoolArguments = 8;
    static constexpr int ScopeArgumentNumber = 8;
    IGC_ASSERT(IGCLLVM::getNumArgOperands(cast<CallInst>(inst)) == (ExpectedNumberOfBoolArguments + 1));
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    // If passed a non-constant value for any of the parameters,
    // be conservative and assume the default value defined below.
    // This could happen in "optnone" scenarios.
    bool CommitEnable = true;
    bool L3_Flush_RW_Data = true;
    bool L3_Flush_Constant_Data = true;
    bool L3_Flush_Texture_Data = true;
    bool L3_Flush_Instructions = true;
    bool Global_Mem_Fence = true;
    bool L1_Invalidate = ctx->platform.hasL1ReadOnlyCache();
    // Note: this flag is respected only for LSC case currently.
    // TODO: add support for non-LSC and typed fence.
    bool L1_Evict = true;
    LSC_SCOPE Scope = LSC_SCOPE_GPU;

    std::array<reference_wrapper<bool>, ExpectedNumberOfBoolArguments> MemFenceArguments{
      CommitEnable,
      L3_Flush_RW_Data,
      L3_Flush_Constant_Data,
      L3_Flush_Texture_Data,
      L3_Flush_Instructions,
      Global_Mem_Fence,
      L1_Invalidate,
      L1_Evict
    };

    for (size_t i = 0; i < MemFenceArguments.size(); ++i) {
        if (ConstantInt* CI = llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(i)))
        {
            MemFenceArguments[i] &= CI->getValue().getBoolValue();
        }
    }

    if (ConstantInt* CI = llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(ScopeArgumentNumber)))
    {
        Scope = (LSC_SCOPE) CI->getValue().getZExtValue();
    }

    bool EmitFence = true;

    // Check whether we know this is a local fence. If we do, don't emit fence for a BDW+SKL/BXT only.
    if (!Global_Mem_Fence) {
        if (ctx->platform.localMemFenceSupress()) {
            EmitFence = false;
        }
    }

    if (!(ctx->platform.hasMultiTile() && !ctx->platform.L3CacheCoherentCrossTiles()) ||
        (IGC_IS_FLAG_DISABLED(EnableL3FlushForGlobal) && Global_Mem_Fence))
    {
        // no need to flush L3
        L3_Flush_RW_Data = false;
        L3_Flush_Constant_Data = false;
        L3_Flush_Texture_Data = false;
        L3_Flush_Instructions = false;
    }
    if (L3_Flush_RW_Data)
    {
        // dont flush L1 if L3 is also being flushed
        L1_Invalidate = false;
    }

    if (shouldGenerateLSC() && EmitFence)
    {
        // tgm should use GenISA_typedmemoryfence
        LSC_SFID sfid = Global_Mem_Fence ? LSC_UGM : LSC_SLM;
        // SLM only gets .group and .none
        if(!Global_Mem_Fence && Scope >= LSC_SCOPE_LOCAL)
        {
            Scope = LSC_SCOPE_GROUP;
        }
        // Do L1 evict only when default L1 cache policy is write-back.
        if (L1_Evict && m_pCtx->type == ShaderType::OPENCL_SHADER)
        {
            auto CLCtx = static_cast<const OpenCLProgramContext*>(m_pCtx);
            if (CLCtx->m_InternalOptions.StoreCacheDefault != -1)
                L1_Evict = static_cast<LSC_L1_L3_CC>(CLCtx->m_InternalOptions.StoreCacheDefault) == LSC_L1IAR_WB_L3C_WB;
        }
        // Change the scope from `GPU` or wider to `Tile` on single-tile platforms to avoid L3 flush on DG2 and MTL and ARL.
        if (Scope >= LSC_SCOPE_GPU &&
            !m_currShader->m_Platform->hasMultiTile() &&
            m_currShader->m_Platform->hasL3FlushOnGPUScopeInvalidate() &&
            IGC_IS_FLAG_DISABLED(EnableGPUFenceScopeOnSingleTileGPUs))
        {
            Scope = LSC_SCOPE_TILE;
        }

        // On some products cache is not flushed for scope < tile.
        if (Global_Mem_Fence && (L1_Invalidate || L1_Evict) &&
            (Scope < LSC_SCOPE_TILE) && (
               m_currShader->m_Platform->getPlatformInfo().eProductFamily == IGFX_DG2
            || m_currShader->m_Platform->getPlatformInfo().eProductFamily == IGFX_PVC
            || m_currShader->m_Platform->getPlatformInfo().eProductFamily == IGFX_METEORLAKE
            || m_currShader->m_Platform->getPlatformInfo().eProductFamily == IGFX_ARROWLAKE
            ))
        {
            Scope = LSC_SCOPE_TILE;
        }

        LSC_FENCE_OP op = getLSCMemoryFenceOp(Global_Mem_Fence, L1_Invalidate, L1_Evict);
        if (inst->getMetadata("forceFlushNone") || sfid == LSC_SLM)
        {
            op = LSC_FENCE_OP_NONE;
        }
        m_encoder->LSC_Fence(sfid, Scope, op);
        m_encoder->Push();
        return;
    }

    m_encoder->Fence(CommitEnable,
        L3_Flush_RW_Data,
        L3_Flush_Constant_Data,
        L3_Flush_Texture_Data,
        L3_Flush_Instructions,
        Global_Mem_Fence,
        L1_Invalidate,
        !EmitFence);

    m_encoder->Push();
}

void EmitPass::emitMemoryFence()
{
    m_encoder->Fence(true,
        false,
        false,
        false,
        false,
        true,
        false,
        false);
    m_encoder->Push();
}

void EmitPass::emitTypedMemoryFence(llvm::Instruction* inst)
{
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    bool CommitEnable = true;
    bool L3_Flush_RW_Data = m_currShader->m_Platform->flushL3ForTypedMemory();
    bool L3_Flush_Constant_Data = false;
    bool L3_Flush_Texture_Data = false;
    bool L3_Flush_Instructions = false;
    bool Global_Mem_Fence = true;
    bool L1_Invalidate = ctx->platform.hasL1ReadOnlyCache();

    // If passed a non-constant parameter, be conservative and assume that the parameter is true
    if (ConstantInt* globalConst = llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(0)))
    {
        L1_Invalidate &= globalConst->getValue().getBoolValue();
    }


    if (shouldGenerateLSC())
    {
        auto flushOpt = getLSCMemoryFenceOp(true, L1_Invalidate, m_currShader->m_Platform->hasSamplerSupport());
        LSC_SCOPE scope = LSC_SCOPE_GPU;
        if (!m_currShader->m_Platform->hasMultiTile() &&
            m_currShader->m_Platform->hasL3FlushOnGPUScopeInvalidate() &&
            IGC_IS_FLAG_DISABLED(EnableGPUFenceScopeOnSingleTileGPUs))
        {
            scope = LSC_SCOPE_TILE;
        }

        m_encoder->LSC_Fence(LSC_TGM, scope, flushOpt);
        m_encoder->Push();
        if (m_currShader->m_Platform->hasSamplerSupport())
        {
            emitFlushSamplerCache();
        }
        return;
    }

    m_encoder->Fence(CommitEnable,
        L3_Flush_RW_Data,
        L3_Flush_Constant_Data,
        L3_Flush_Texture_Data,
        L3_Flush_Instructions,
        Global_Mem_Fence,
        L1_Invalidate,
        false);
    emitFlushSamplerCache();
}


void EmitPass::emitFlushSamplerCache()
{
    m_encoder->FlushSamplerCache();
    m_encoder->Push();
}


void EmitPass::emitUniformAtomicCounter(llvm::GenIntrinsicInst* pInsn)
{
    if (m_currShader->m_Platform->isCoreChildOf(IGFX_XE2_HPG_CORE))
    {
        return emitLscUniformAtomicCounter(pInsn);
    }
    ForceDMask();
    IGC_ASSERT(pInsn->getNumOperands() == 2);
    GenISAIntrinsic::ID IID = pInsn->getIntrinsicID();
    /// Immediate Atomics return the value before the atomic operation is performed. So that flag
    /// needs to be set for this.
    bool returnsImmValue = !pInsn->user_empty();

    llvm::Value* pllbuffer = pInsn->getOperand(0);
    ResourceDescriptor resource = GetResourceVariable(pllbuffer);
    uint binding_table_index = 0;

    CVariable* prefixVar[2] = { nullptr, nullptr };
    CVariable* dst = m_destination;
    bool hasheader = m_currShader->m_Platform->needsHeaderForAtomicCounter();

    EU_DATA_PORT_ATOMIC_OPERATION_TYPE atomicType = EU_DATA_PORT_ATOMIC_OPERATION_ADD;
    // for SIMD dispatch greater than 8 it is more efficient to emit a SIMD1 atomic
    CVariable* src = m_currShader->ImmToVariable(
        IID == GenISAIntrinsic::GenISA_atomiccounterinc ? 1 : -1, ISA_TYPE_D);
    emitPreOrPostFixOp(EOPCODE_ADD, 0, ISA_TYPE_D, false, src, prefixVar);
    CVariable* pSrcCopy = prefixVar[0];
    if (m_currShader->m_numberInstance == 2)
    {
        pSrcCopy = prefixVar[1];
    }

    CVariable* pHeader = nullptr;
    if (hasheader)
    {
        pHeader = m_currShader->GetNewVariable(
            numLanes(SIMDMode::SIMD8),
            ISA_TYPE_UD,
            EALIGN_GRF, CName::NONE);

        m_encoder->SetNoMask();
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetDstSubReg(7);
        m_encoder->Copy(pHeader, m_currShader->ImmToVariable(0xFFFF, ISA_TYPE_UD));
        m_encoder->Push();
    }

    CVariable* pPayload = m_currShader->GetNewVariable(
        8,
        ISA_TYPE_D,
        EALIGN_GRF,
        true, CName::NONE);
    m_encoder->SetSimdSize(SIMDMode::SIMD1);
    m_encoder->SetSrcRegion(0, 0, 1, 0);
    m_encoder->SetSrcSubReg(0, numLanes(m_currShader->m_SIMDSize) - 1);
    m_encoder->Copy(pPayload, pSrcCopy);
    m_encoder->Push();
    dst = m_currShader->GetNewVariable(
        8,
        ISA_TYPE_D,
        EALIGN_GRF,
        true, CName::NONE);

    if (resource.m_surfaceType == ESURFACE_SSHBINDLESS)
        binding_table_index = SSH_BINDLESS_BTI;
    else if (resource.m_surfaceType == ESURFACE_BINDLESS)
        binding_table_index = BINDLESS_BTI;
    else
        binding_table_index = (uint)resource.m_resource->GetImmediateValue();

    uint messageDescriptor = encodeMessageDescriptorForAtomicUnaryOp(
        1,
        returnsImmValue ? 1 : 0,
        hasheader,
        EU_GEN7_5_DATA_CACHE_1_MESSAGE_TYPE_ATOMIC_COUNTER_OPERATION,
        returnsImmValue,
        SIMDMode::SIMD8,
        atomicType,
        binding_table_index);

    CVariable* pMessDesc = m_currShader->ImmToVariable(messageDescriptor, ISA_TYPE_D);
    // src1 len = 1, SFID = DC1
    uint32_t src1Len = hasheader ? 1 : 0;
    //src1Len is not encoded in ext descriptor in case of 26bit bso
    if (m_currShader->m_Platform->support26BitBSOFormat() &&
       (resource.m_surfaceType == ESURFACE_BINDLESS || resource.m_surfaceType == ESURFACE_SCRATCH))
    {
        src1Len = 0;
    }
    uint32_t exDescVal = (src1Len << 6) | EU_GEN7_5_MESSAGE_TARGET_DATA_PORT_DATA_CACHE_1;
    CVariable* exDesc =
        m_currShader->ImmToVariable(exDescVal, ISA_TYPE_D);

    if (resource.m_surfaceType == ESURFACE_BINDLESS || resource.m_surfaceType == ESURFACE_SSHBINDLESS)
    {
        CVariable* temp = m_currShader->GetNewVariable(resource.m_resource);
        m_encoder->Add(temp, resource.m_resource, exDesc);
        m_encoder->Push();
        exDesc = temp;
    }

    m_encoder->SetSimdSize(SIMDMode::SIMD1);
    m_encoder->SetNoMask();

    if (hasheader)
    {
        m_encoder->Sends(returnsImmValue ? dst : nullptr, pHeader, pPayload,
            EU_GEN7_5_MESSAGE_TARGET_DATA_PORT_DATA_CACHE_1, exDesc, pMessDesc);
    }
    else
    {
        m_encoder->Send(
            returnsImmValue ? dst : NULL,
            pPayload,
            EU_GEN7_5_MESSAGE_TARGET_DATA_PORT_DATA_CACHE_1,
            exDesc,
            pMessDesc);
    }
    m_encoder->Push();

    if (returnsImmValue)
    {
        unsigned int counter = m_currShader->m_numberInstance;
        for (unsigned int i = 0; i < counter; ++i)
        {
            m_encoder->SetSecondHalf(i == 1);
            m_encoder->Add(m_destination, prefixVar[i], dst);
            m_encoder->Push();

            if (IID == GenISAIntrinsic::GenISA_atomiccounterinc)
            {
                CVariable* src = m_currShader->ImmToVariable(-1, ISA_TYPE_D);
                m_encoder->Add(m_destination, m_destination, src);
                m_encoder->Push();
            }
        }
    }

    ResetVMask();
    m_currShader->m_State.isMessageTargetDataCacheDataPort = true;
}

void EmitPass::emitAtomicCounter(llvm::GenIntrinsicInst* pInsn)
{

    IGC_ASSERT(pInsn->getNumOperands() == 2);

    bool uniformAtomic = IsUniformAtomic(pInsn) &&
        (m_currShader->m_SIMDSize != SIMDMode::SIMD8 || !m_currShader->m_Platform->HDCCoalesceAtomicCounterAccess());
    if (uniformAtomic)
    {
        emitUniformAtomicCounter(pInsn);
        return;
    }

    ForceDMask();
    GenISAIntrinsic::ID IID = pInsn->getIntrinsicID();
    /// Immediate Atomics return the value before the atomic operation is performed. So that flag
    /// needs to be set for this.
    bool returnsImmValue = !pInsn->user_empty();

    llvm::Value* pllbuffer = pInsn->getOperand(0);
    ResourceDescriptor resource = GetResourceVariable(pllbuffer);

    CVariable* dst = m_destination;

    bool hasheader = true;
    unsigned int num_split = m_currShader->m_SIMDSize == SIMDMode::SIMD16 ? 2 : 1;

    // header
    CVariable* pPayload = m_currShader->GetNewVariable(
        numLanes(SIMDMode::SIMD8),
        ISA_TYPE_UD,
        EALIGN_GRF, CName::NONE);
    m_encoder->SetNoMask();
    m_encoder->SetSimdSize(SIMDMode::SIMD1);
    m_encoder->SetDstSubReg(7);
    m_encoder->Copy(pPayload, m_currShader->ImmToVariable(0xFFFF, ISA_TYPE_UD));
    m_encoder->Push();

    EU_DATA_PORT_ATOMIC_OPERATION_TYPE atomicType = EU_DATA_PORT_ATOMIC_OPERATION_INC;
    if (IID == GenISAIntrinsic::GenISA_atomiccounterpredec)
    {
        atomicType = m_currShader->m_Platform->hasAtomicPreDec() ?
            EU_DATA_PORT_ATOMIC_OPERATION_PREDEC : EU_DATA_PORT_ATOMIC_OPERATION_DEC;
    }

    uint label = 0;
    CVariable* flag = nullptr;
    bool needLoop = ResourceLoopHeader(resource, flag, label);
    ResourceLoopSubIteration(resource, flag, label);

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

    if (resource.m_surfaceType == ESURFACE_BINDLESS)
    {
        CVariable* temp = m_currShader->GetNewVariable(resource.m_resource);
        m_encoder->Add(temp, resource.m_resource, exDesc);
        m_encoder->Push();

        exDesc = temp;
    }

    for (uint32_t i = 0; i < num_split; ++i)
    {
        m_encoder->SetSimdSize(SIMDMode::SIMD8);
        m_encoder->SetDstSubVar(i);
        m_encoder->SetMask((i == 0) ? EMASK_Q1 : EMASK_Q2);

        m_encoder->Send(
            returnsImmValue ? dst : NULL,
            pPayload,
            EU_GEN7_5_MESSAGE_TARGET_DATA_PORT_DATA_CACHE_1,
            exDesc,
            pMessDesc);
        m_encoder->Push();
    }

    if (IID == GenISAIntrinsic::GenISA_atomiccounterpredec &&
        !m_currShader->m_Platform->hasAtomicPreDec())
    {
        unsigned int counter = m_currShader->m_numberInstance;
        for (unsigned int i = 0; i < counter; ++i)
        {
            m_encoder->SetSecondHalf(i == 1);
            CVariable* src = m_currShader->ImmToVariable(-1, ISA_TYPE_D);
            m_encoder->Add(m_destination, m_destination, src);
            m_encoder->Push();
        }
    }

    ResourceLoopBackEdge(needLoop, flag, label);
    ResetVMask();
    m_currShader->m_State.isMessageTargetDataCacheDataPort = true;
}

void EmitPass::CmpBoolOp(Pattern* cmpPattern,
    llvm::BinaryOperator* inst,
    const SSource& bitSource,
    const DstModifier& modifier)
{
    DstModifier init;
    cmpPattern->Emit(this, init);

    IGC_ASSERT(bitSource.mod == EMOD_NONE);
    CVariable* boolOpSource = GetSrcVariable(bitSource);
    m_encoder->SetDstModifier(modifier);

    EmitSimpleAlu(inst, m_destination, m_destination, boolOpSource);
}

void EmitPass::emitAluConditionMod(Pattern* aluPattern, Instruction* alu, CmpInst* cmp, int aluOprdNum)
{
    CVariable* temp = m_currShader->GetNewVector(alu);
    CVariable* dst = m_destination;
    m_destination = temp;
    DstModifier init;

    aluPattern->Emit(this, init);

    // condMod is in the form of "alu cmpOp 0". If pattern is in the form of
    // "0 cmpOp alu", cmp's predicate should be swapped. aluOprdNum indicates
    // which form this pattern is.
    auto llvmPredicate = (aluOprdNum == 0 ? cmp->getPredicate() : cmp->getSwappedPredicate());
    e_predicate predicate = GetPredicate(llvmPredicate);
    if (IsUnsignedCmp(llvmPredicate))
    {
        temp = m_currShader->BitCast(temp, GetUnsignedType(temp->GetType()));
    }
    m_encoder->Cmp(predicate, dst, temp, m_currShader->ImmToVariable(0, temp->GetType()));
    m_encoder->Push();
    m_destination = dst;
}


void EmitPass::initDefaultRoundingMode()
{
    const ERoundingMode defaultRM_FP = static_cast<ERoundingMode>(m_pCtx->getModuleMetaData()->compOpt.FloatRoundingMode);
    const ERoundingMode defaultRM_FPCvtInt = static_cast<ERoundingMode>(m_pCtx->getModuleMetaData()->compOpt.FloatCvtIntRoundingMode);

    // Rounding modes must meet the following restrictions
    // in order to be used as default:
    //   1. if FPCvtInt's RM is rtz, FP's RM can be any;
    //   2. otherwise, FPCvtIn's RM must be the same as FP's RM
    const bool supportedDefaultRoundingModes =
        ((defaultRM_FPCvtInt == ERoundingMode::ROUND_TO_ZERO) ||
        (defaultRM_FPCvtInt == defaultRM_FP));

    IGC_ASSERT_EXIT(supportedDefaultRoundingModes);

    m_roundingMode_FPCvtInt = defaultRM_FPCvtInt;
    m_roundingMode_FP = defaultRM_FP;
}

void EmitPass::SetRoundingMode_FP(ERoundingMode newRM_FP)
{
    if (newRM_FP != ERoundingMode::ROUND_TO_ANY &&
        newRM_FP != m_roundingMode_FP)
    {
        m_encoder->SetRoundingMode_FP(m_roundingMode_FP, newRM_FP);
        m_roundingMode_FP = newRM_FP;

        if (m_roundingMode_FPCvtInt != ERoundingMode::ROUND_TO_ZERO)
        {
            // If FPCvtInt's RM is not RTZ, it must be the same as FP's
            m_roundingMode_FPCvtInt = m_roundingMode_FP;
        }
    }
}

void EmitPass::SetRoundingMode_FPCvtInt(ERoundingMode newRM_FPCvtInt)
{
    if (newRM_FPCvtInt != ERoundingMode::ROUND_TO_ANY &&
        newRM_FPCvtInt != m_roundingMode_FPCvtInt)
    {
        m_encoder->SetRoundingMode_FPCvtInt(m_roundingMode_FPCvtInt, newRM_FPCvtInt);
        m_roundingMode_FPCvtInt = newRM_FPCvtInt;

        if (m_roundingMode_FPCvtInt != ERoundingMode::ROUND_TO_ZERO)
        {
            // If FPCvtInt's RM is not RTZ, it must be the same as FP's
            m_roundingMode_FP = m_roundingMode_FPCvtInt;
        }
    }
}

void EmitPass::ResetRoundingMode(Instruction* inst)
{
    // Reset rounding modes to default if they are not. Howerver, if
    // next inst requires non-default, which requires to set
    // RM explicitly, don't set default rounding modes and let the next
    // inst to set it explicitly.
    const ERoundingMode defaultRoundingMode_FP = static_cast<ERoundingMode>(
        m_pCtx->getModuleMetaData()->compOpt.FloatRoundingMode);
    const ERoundingMode defaultRoundingMode_FPCvtInt = static_cast<ERoundingMode>(
        m_pCtx->getModuleMetaData()->compOpt.FloatCvtIntRoundingMode);

    if (m_roundingMode_FP == defaultRoundingMode_FP &&
        m_roundingMode_FPCvtInt == defaultRoundingMode_FPCvtInt)
    {
        // Already in default mode.
        return;
    }

    // Those two variables are set to true if default RM is required before the next
    // explicit-RM setting instruction (genintrinsic).
    bool nextImplicitFPCvtInt = false;
    bool nextImplicitFP = false;
    for (auto nextInst = inst->getNextNonDebugInstruction();
         nextInst != nullptr;
         nextInst = nextInst->getNextNonDebugInstruction())
    {
        if (ignoresRoundingMode(nextInst))
        {
            continue;
        }
        if (setsRMExplicitly(nextInst))
        {
            // As nextInst will set RM explicitly, no need to go further.
            break;
        }

        // At this point, a default RM is needed. For FPCvtInt, we know
        // precisely whether FPCvtInt RM is needed or not; but for FP, we
        // do it conservatively as we do not scan all instructions here.
        ERoundingMode intRM = GetRoundingMode_FPCvtInt(m_pCtx->getModuleMetaData(), nextInst);

        // If it is not ROUND_TO_ANY, it uses FPCvtInt RM;
        // otherwise, it does not use FPCvtInt RM.
        if (intRM != ERoundingMode::ROUND_TO_ANY) {
            nextImplicitFPCvtInt = true;
        }
        else {
            // Conservatively assume FP default RM is used.
            nextImplicitFP = true;
        }

        if (nextImplicitFPCvtInt && nextImplicitFP) {
            break;
        }
    }

    if (nextImplicitFPCvtInt && !nextImplicitFP)
    {
        SetRoundingMode_FPCvtInt(defaultRoundingMode_FPCvtInt);
    }
    else if (nextImplicitFP && !nextImplicitFPCvtInt)
    {
        SetRoundingMode_FP(defaultRoundingMode_FP);
    }
    else  if (nextImplicitFP  && nextImplicitFPCvtInt)
    {
        // Need to set default for both
        if (defaultRoundingMode_FPCvtInt == ERoundingMode::ROUND_TO_ZERO)
        {
            SetRoundingMode_FP(defaultRoundingMode_FP);
        }
        else
        {
            SetRoundingMode_FPCvtInt(defaultRoundingMode_FPCvtInt);
        }
    }
}

void EmitPass::emitf32tof16_rtz(llvm::GenIntrinsicInst* inst)
{
    CVariable* src = GetSymbol(inst->getOperand(0));
    CVariable imm0_hf(0, ISA_TYPE_HF);
    CVariable* dst_hf = m_currShader->BitCast(m_destination, ISA_TYPE_HF);

    // The upper 16bit is required to be set to zero based on:
    //   microsoft.github.io/DirectX-Specs/d3d/archive/
    //   D3D11_3_FunctionalSpec.htm#22.13.2%20f32tof16
    // If the lower is used only, we can safely skip setting upper 16bit.
    bool setUpper16b = true;
    if (inst->hasOneUse() && inst->user_back()->hasOneUse()) {
        //  %16 = call fast float @llvm.genx.GenISA.f32tof16.rtz(float %14)
        //  %17 = bitcast float% 16 to <2xhalf>
        //  %18 = extractelement <2xhalf> %17, i32 0
        Value* U = inst->user_back();
        BitCastInst* BCI = dyn_cast<BitCastInst>(U);
        ExtractElementInst* EEI = dyn_cast<ExtractElementInst>(U->user_back());
        ConstantInt* Ix =
            (EEI ? dyn_cast<ConstantInt>(EEI->getIndexOperand()) : nullptr);
        if (BCI && EEI && Ix && BCI->getType()->isVectorTy() &&
            BCI->getType()->getScalarType()->isHalfTy() &&
            BCI == EEI->getVectorOperand() && Ix->isZero()) {
            setUpper16b = false;
        }
    }

    SetRoundingMode_FP(ERoundingMode::ROUND_TO_ZERO);

    m_encoder->SetDstRegion(2);
    m_encoder->Cast(dst_hf, src);
    m_encoder->Push();

    if (setUpper16b) {
        m_encoder->SetDstRegion(2);
        m_encoder->SetDstSubReg(1);
        m_encoder->Copy(dst_hf, &imm0_hf);
        m_encoder->Push();
    }

    ResetRoundingMode(inst);
}

void EmitPass::emitfitof(llvm::GenIntrinsicInst* inst)
{
    CVariable* src = GetSymbol(inst->getOperand(0));
    ERoundingMode RM = GetRoundingMode_FP(m_pCtx->getModuleMetaData(), inst);
    CVariable* dst = m_destination;

    GenISAIntrinsic::ID id = inst->getIntrinsicID();
    if (id == GenISAIntrinsic::GenISA_uitof_rtn ||
        id == GenISAIntrinsic::GenISA_uitof_rtp ||
        id == GenISAIntrinsic::GenISA_uitof_rtz)
    {
        src = m_currShader->BitCast(src, GetUnsignedType(src->GetType()));
    }

    SetRoundingMode_FP(RM);

    m_encoder->Cast(dst, src);
    m_encoder->Push();

    ResetRoundingMode(inst);
}

// Emit FP Operations (FPO) using a non-default rounding mode
void EmitPass::emitFPOWithNonDefaultRoundingMode(llvm::GenIntrinsicInst* inst)
{
    IGC_ASSERT_MESSAGE(IGCLLVM::getNumArgOperands(inst) >= 2, "ICE: incorrect gen intrinsic");

    GenISAIntrinsic::ID GID = inst->getIntrinsicID();
    CVariable* src0 = GetSymbol(inst->getOperand(0));
    CVariable* src1 = GetSymbol(inst->getOperand(1));
    CVariable* dst = m_destination;

    SetRoundingMode_FP(GetRoundingMode_FP(m_pCtx->getModuleMetaData(), inst));

    switch (GID)
    {
    default:
        IGC_ASSERT_MESSAGE(0, "ICE: unexpected Gen Intrinsic");
        break;
    case GenISAIntrinsic::GenISA_mul_rtz:
    case GenISAIntrinsic::GenISA_mul_rte:
    case GenISAIntrinsic::GenISA_mul_rtp:
    case GenISAIntrinsic::GenISA_mul_rtn:
        m_encoder->Mul(dst, src0, src1);
        m_encoder->Push();
        break;
    case GenISAIntrinsic::GenISA_add_rtz:
    case GenISAIntrinsic::GenISA_add_rte:
    case GenISAIntrinsic::GenISA_add_rtp:
    case GenISAIntrinsic::GenISA_add_rtn:
        m_encoder->Add(dst, src0, src1);
        m_encoder->Push();
        break;
    case GenISAIntrinsic::GenISA_fma_rtz:
    case GenISAIntrinsic::GenISA_fma_rte:
    case GenISAIntrinsic::GenISA_fma_rtp:
    case GenISAIntrinsic::GenISA_fma_rtn:
    {
        CVariable* src2 = GetSymbol(inst->getOperand(2));
        m_encoder->Mad(dst, src0, src1, src2);
        m_encoder->Push();
        break;
    }
    }

    ResetRoundingMode(inst);
}

void EmitPass::emitftoi(llvm::GenIntrinsicInst* inst)
{
    IGC_ASSERT_MESSAGE(inst->getOperand(0)->getType()->isFloatingPointTy(), "Unsupported type");
    CVariable* src = GetSymbol(inst->getOperand(0));
    CVariable* dst = m_destination;
    ERoundingMode RM = GetRoundingMode_FPCvtInt(m_pCtx->getModuleMetaData(), inst);
    IGC_ASSERT_MESSAGE(RM != ERoundingMode::ROUND_TO_ANY, "Not valid FP->int rounding mode!");

    GenISAIntrinsic::ID id = inst->getIntrinsicID();
    if (id == GenISAIntrinsic::GenISA_ftoui_rtn ||
        id == GenISAIntrinsic::GenISA_ftoui_rtp ||
        id == GenISAIntrinsic::GenISA_ftoui_rte)
    {
        dst = m_currShader->BitCast(dst, GetUnsignedType(dst->GetType()));
    }

    SetRoundingMode_FPCvtInt(RM);

    m_encoder->Cast(dst, src);
    m_encoder->Push();

    ResetRoundingMode(inst);
}

bool EmitPass::isUniformStoreOCL(Value* ptr, Value* storeVal)
{
    if (m_currShader->GetShaderType() != ShaderType::OPENCL_SHADER ||
        !m_currShader->GetIsUniform(ptr))
    {
        return false;
    }

    Type* Ty = storeVal->getType();
    IGCLLVM::FixedVectorType* VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
    uint32_t elts = VTy ? int_cast<uint32_t>(VTy->getNumElements()) : 1;
    Type* eltTy = VTy ? VTy->getElementType() : Ty;

    // use TypeSize to be consistent with VectorLoad/Store
    uint32_t totalBytes = elts * ((uint32_t)m_DL->getTypeSizeInBits(eltTy) / 8);

    // Note that when elts > 1, VectorProcess make sure that its element
    // size must be 4 or 8. Also, note that if totalBytes = 4, elts must be 1.
    bool doUniformStore = (elts == 1 ||
        (m_currShader->GetIsUniform(storeVal) &&
            (totalBytes == 8 || totalBytes == 12 || totalBytes == 16)));
    if (shouldGenerateLSC())
    {
        if (totalBytes < 4 || m_currShader->GetIsUniform(storeVal))
        {
            doUniformStore = true;
        }
        else
        {
            doUniformStore = false;
        }
    }
    return doUniformStore;
}

// Return true if this store will be emit as uniform store
bool EmitPass::isUniformStoreOCL(llvm::StoreInst* SI)
{
    return isUniformStoreOCL(SI->getPointerOperand(), SI->getValueOperand());
}

void EmitPass::emitVectorBitCast(llvm::BitCastInst* BCI)
{
    const CShader::ExtractMaskWrapper destMask(m_currShader, BCI);

    CVariable* src = GetSymbol(BCI->getOperand(0));
    llvm::Type* srcTy = BCI->getOperand(0)->getType();
    llvm::Type* dstTy = BCI->getType();
    llvm::Type* srcEltTy, * dstEltTy;
    uint32_t srcNElts, dstNElts;

    IGC_ASSERT_MESSAGE((srcTy->isVectorTy() || dstTy->isVectorTy()), "No vector type !");

    if (srcTy->isVectorTy())
    {
        srcEltTy = cast<VectorType>(srcTy)->getElementType();
        srcNElts = (uint32_t)cast<IGCLLVM::FixedVectorType>(srcTy)->getNumElements();
    }
    else
    {
        srcEltTy = srcTy;
        srcNElts = 1;
    }
    if (dstTy->isVectorTy())
    {
        dstEltTy = cast<VectorType>(dstTy)->getElementType();
        dstNElts = (uint32_t)cast<IGCLLVM::FixedVectorType>(dstTy)->getNumElements();
    }
    else
    {
        dstEltTy = dstTy;
        dstNElts = 1;
    }

    if (src->IsImmediate())
    {
        CVariable* reg = m_currShader->GetNewVariable(
            1,
            src->GetType(),
            m_encoder->GetCISADataTypeAlignment(src->GetType()),
            true,
            1, CName::NONE);

        m_encoder->Copy(reg, src);
        m_encoder->Push();

        src = reg;
    }

    uint32_t width = numLanes(m_currShader->m_SIMDSize);
    uint32_t dstEltBytes = GetPrimitiveTypeSizeInRegister(dstEltTy);
    uint32_t srcEltBytes = GetPrimitiveTypeSizeInRegister(srcEltTy);
    bool srcUniform = src->IsUniform();
    bool dstUniform = m_destination->IsUniform();

    if (srcUniform && dstUniform &&
        (dstNElts == 2 || dstNElts == 4 || dstNElts == 8 ||  dstNElts == 16 || dstNElts == 32 ) &&
         dstNElts <= width && (dstEltBytes * dstNElts) <= 2 * m_currShader->getGRFSize() &&
        m_destination != src &&
        destMask.getEM() == ((1ULL << dstNElts) - 1)/* Full mask */ &&
        /* If alignment of source is safe to be aliased to the dst type. */
        src->GetAlign() >= CEncoder::GetCISADataTypeAlignment(m_destination->GetType()) &&
        /* Exclude bitcast from/to 16-bit */
        srcEltBytes != 2 && dstEltBytes != 2) {
        // TODO; Add uniform vector bitcast support. A simple copy is enough but
        // the ideal resolution is to teach DeSSA to handle that.
        CVariable* dst = m_destination;
        src = m_currShader->BitCast(src, dst->GetType());
        m_encoder->SetNoMask();
        m_encoder->SetUniformSIMDSize(lanesToSIMDMode(dstNElts));
        m_encoder->SetSrcRegion(0, 1, 1, 0);
        m_encoder->Copy(dst, src);
        m_encoder->Push();
        return;
    }
    if (srcEltBytes == dstEltBytes)
    {
        // This should not happen now, but generate code anyway.
        // CISABuilder does split if there is any splitting.

        // Special case for: 1 element vectors to scalars
        //    %15 = bitcast <1 x i64> %4 to i64
        if ((srcEltTy == dstEltTy) &&
            (srcNElts == dstNElts) && (srcNElts == 1))
        {
            m_encoder->Copy(m_destination, src);
            m_encoder->Push();
        }
        else if (m_destination != src)
        {
            for (uint32_t i = 0, offset = 0; i < dstNElts; ++i)
            {
                if (destMask.isSet(i))
                {
                    m_encoder->SetSrcRegion(0, (srcUniform ? 0 : 1), 1, 0);
                    m_encoder->SetSrcSubReg(0, srcUniform ? i : i * width);
                    m_encoder->SetDstRegion(1);
                    m_encoder->SetDstSubReg(dstUniform ? offset : offset * width);
                    m_encoder->Copy(m_destination, src);
                    m_encoder->Push();
                    offset++;
                }
            }
        }
    }
    else if (dstEltBytes > srcEltBytes)
    {
        IGC_ASSERT(0 < srcEltBytes);
        CVariable* aliasDst = m_currShader->GetNewAlias(m_destination, src->GetType(), 0, 0);
        uint32_t N = dstEltBytes / srcEltBytes;
        IGC_ASSERT_MESSAGE((dstEltBytes % srcEltBytes) == 0, "Basic types should be power of 2");
        // Since srcEltBytes can be the second largest element type (32bit)
        // and region hstride == 1, Src will not need splitting!
        // Only dst might need splitting.
        bool splitDst = (!dstUniform && (dstEltBytes * width > m_currShader->getGRFSize() * 2));
        IGC_ASSERT_MESSAGE((!splitDst || (width == 16) || (width == 32)),
            "Internal Error: Dst needs splitting only under SIMD16!");

        // kind of a special case, such as with TotalGRFNum4RQ=256
        //
        // from .ll:
        // %.bc = bitcast <64 x i32> %BlockLoad to <32 x i64>
        // in .visaasm, there 64 mov(M1_NM, 1):
        // .decl V0042 v_type=G type=d num_elts=64 align=wordx32
        // .decl _bc v_type=G type=q num_elts=32 align=qword
        // .decl _bc_0 v_type=G type=d num_elts=64 align=qword alias=<_bc, 0>
        // ......
        // mov(M1_NM, 1) _bc_0(0,0)<1> V0042(0,0)<0;1,0>
        // mov(M1_NM, 1) _bc_0(0,1)<1> V0042(0,1)<0;1,0>
        // ......
        // mov(M1_NM, 1) _bc_0(3,14)<1> V0042(3,14)<0;1,0>
        // mov(M1_NM, 1) _bc_0(3,15)<1> V0042(3,15)<0;1,0>
        //
        // this is changed to 8 mov(M1_NM, 8) if simd8, or
        // 4 mov(M1_NM, 16) if simd16
        //
        // .decl V0042 v_type=G type=d num_elts=64 align=wordx32
        // .decl _bc v_type=G type=q num_elts=32 align=qword
        // .decl _bc_0 v_type=G type=d num_elts=64 align=qword alias=<_bc, 0>
        // ......
        // mov(M1_NM, 16) _bc_0(0,0)<1> V0042(0,0)<1;1,0>
        // mov(M1_NM, 16) _bc_0(1,0)<1> V0042(1,0)<1;1,0>
        // mov(M1_NM, 16) _bc_0(2,0)<1> V0042(2,0)<1;1,0>
        // mov(M1_NM, 16) _bc_0(3,0)<1> V0042(3,0)<1;1,0>

        if (srcUniform && dstUniform &&
            srcNElts == 64 && dstNElts == 32 &&
            m_destination->GetType() == ISA_TYPE_Q &&
            src->GetType() == ISA_TYPE_D &&
            m_currShader->m_Platform->canDoMultipleLineMOVOpt())
        {
            IGC_ASSERT(N == 2);
            IGC_ASSERT(srcEltBytes == 4);

            // re-route to emitVectorCopy
            emitVectorCopy(aliasDst, src, srcNElts, 0, 0,
                (m_currShader->m_SIMDSize > SIMDMode::SIMD8));
            return;
        }
        else if (N > 4)
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
            IGC_ASSERT(N == 8);
            IGC_ASSERT(srcEltBytes == 1);
            const uint32_t N2 = N / 2; // 4
            VISA_Type TyD = (src->GetType() == ISA_TYPE_UB) ? ISA_TYPE_UD : ISA_TYPE_D;
            CVariable* V0 = m_currShader->GetNewVariable(dstUniform ? 1 : width, TyD, EALIGN_GRF, dstUniform, CName::NONE);
            CVariable* V1 = m_currShader->GetNewVariable(dstUniform ? 1 : width, TyD, EALIGN_GRF, dstUniform, CName::NONE);
            CVariable* V0_alias = m_currShader->GetNewAlias(V0, src->GetType(), 0, 0);
            CVariable* V1_alias = m_currShader->GetNewAlias(V1, src->GetType(), 0, 0);
            CVariable* dst_alias = m_currShader->GetNewAlias(m_destination, V0->GetType(), 0, 0);
            for (unsigned i = 0, offset = 0; i < dstNElts; ++i)
            {
                if (destMask.isSet(i))
                {
                    for (unsigned j = 0; j < N; ++j)
                    {
                        bool useV0 = (j < N2);
                        uint32_t oft = useV0 ? j : j - N2;
                        m_encoder->SetSrcRegion(0, srcUniform ? 0 : 1, 1, 0);
                        m_encoder->SetSrcSubReg(0, srcUniform ? (i * N + j) : (width * (i * N + j)));
                        m_encoder->SetDstRegion(dstUniform ? 1 : N2);
                        m_encoder->SetDstSubReg(oft);
                        m_encoder->Copy(useV0 ? V0_alias : V1_alias, src);
                        m_encoder->Push();
                    }
                    // combine V0 and V1 into dst
                    if (splitDst)
                    {
                        SIMDMode simdSize = (m_currShader->m_SIMDSize == SIMDMode::SIMD32 && m_currShader->getGRFSize() == 64) ? SIMDMode::SIMD16 : SIMDMode::SIMD8;
                        int exSize = simdSize == SIMDMode::SIMD16 ? 16 : 8;
                        // Dst must not be uniform and it must be SIMD16!
                        // first simd8/simd16 : dst_alias = V0
                        m_encoder->SetDstRegion(2);
                        m_encoder->SetDstSubReg(2 * offset * width);
                        m_encoder->SetSimdSize(simdSize);
                        m_encoder->SetMask(simdSize != SIMDMode::SIMD16 ? EMASK_Q1 : EMASK_H1);
                        m_encoder->Copy(dst_alias, V0);
                        m_encoder->Push();
                        // second simd8/simd16: dst_alias=V0
                        m_encoder->SetSrcSubReg(0, exSize);
                        m_encoder->SetDstRegion(2);
                        m_encoder->SetDstSubReg(2 * offset * width + 2 * exSize);
                        m_encoder->SetSimdSize(simdSize);
                        m_encoder->SetMask(simdSize != SIMDMode::SIMD16 ? EMASK_Q2 : EMASK_H2);
                        m_encoder->Copy(dst_alias, V0);
                        m_encoder->Push();

                        // first simd8/simd16 : dist_alias=V1
                        m_encoder->SetDstRegion(2);
                        m_encoder->SetDstSubReg(2 * offset * width + 1);
                        m_encoder->SetSimdSize(simdSize);
                        m_encoder->SetMask(simdSize != SIMDMode::SIMD16 ? EMASK_Q1 : EMASK_H1);
                        m_encoder->Copy(dst_alias, V1);
                        m_encoder->Push();
                        // first simd8/simd16 : dist_alias=V1
                        m_encoder->SetSrcSubReg(0, exSize);
                        m_encoder->SetDstRegion(2);
                        m_encoder->SetDstSubReg(2 * offset * width + 2 * exSize + 1);
                        m_encoder->SetSimdSize(simdSize);
                        m_encoder->SetMask(simdSize != SIMDMode::SIMD16 ? EMASK_Q2 : EMASK_H2);
                        m_encoder->Copy(dst_alias, V1);
                        m_encoder->Push();
                    }
                    else
                    {
                        m_encoder->SetDstRegion(dstUniform ? 1 : 2);
                        m_encoder->SetDstSubReg(dstUniform ? (2 * offset) : (2 * offset * width));
                        m_encoder->Copy(dst_alias, V0);
                        m_encoder->Push();
                        m_encoder->SetDstRegion(dstUniform ? 1 : 2);
                        m_encoder->SetDstSubReg(dstUniform ? (2 * offset + 1) : (2 * offset * width + 1));
                        m_encoder->Copy(dst_alias, V1);
                        m_encoder->Push();
                    }
                    offset++;
                }
            }
        }
        else
        {
            for (unsigned i = 0, offset = 0; i < dstNElts; ++i)
            {
                if (destMask.isSet(i))
                {
                    for (unsigned j = 0; j < N; ++j)
                    {
                        if (splitDst)
                        {
                            // !dstUniform
                            // first half
                            SIMDMode mode = m_currShader->m_SIMDSize == SIMDMode::SIMD32 ? SIMDMode::SIMD16 : SIMDMode::SIMD8;
                            int exSize = mode == SIMDMode::SIMD16 ? 16 : 8;
                            m_encoder->SetSrcRegion(0, srcUniform ? 0 : 1, 1, 0);
                            m_encoder->SetSrcSubReg(0, srcUniform ? (i * N + j) : (width * (i * N + j)));
                            m_encoder->SetDstRegion(N);
                            m_encoder->SetDstSubReg(offset * N * width + j);
                            m_encoder->SetSimdSize(mode);
                            m_encoder->SetMask(mode == SIMDMode::SIMD16 ? EMASK_H1 : EMASK_Q1);
                            m_encoder->Copy(aliasDst, src);
                            m_encoder->Push();

                            // second half
                            m_encoder->SetSrcRegion(0, srcUniform ? 0 : 1, 1, 0);
                            m_encoder->SetSrcSubReg(0, srcUniform ? (i * N + j) : (width * (i * N + j) + exSize));
                            m_encoder->SetDstRegion(N);
                            m_encoder->SetDstSubReg(offset * N * width + N * exSize + j);
                            m_encoder->SetSimdSize(mode);
                            m_encoder->SetMask(mode == SIMDMode::SIMD16 ? EMASK_H2 : EMASK_Q2);
                            m_encoder->Copy(aliasDst, src);
                            m_encoder->Push();
                        }
                        else
                        {
                            m_encoder->SetSrcRegion(0, srcUniform ? 0 : 1, 1, 0);
                            m_encoder->SetSrcSubReg(0, srcUniform ? (i * N + j) : (width * (i * N + j)));
                            m_encoder->SetDstRegion(dstUniform ? 1 : N);
                            m_encoder->SetDstSubReg(dstUniform ? (offset * N + j) : (offset * N * width + j));
                            m_encoder->Copy(aliasDst, src);
                            m_encoder->Push();
                        }
                    }
                    offset++;
                }
            }
        }
    }
    else // (dstEltBytes < srcEltBytes)
    {
        IGC_ASSERT(0 < dstEltBytes);
        // Create an aliase to src and mov the alias to the dst
        CVariable* aliasSrc = m_currShader->GetNewAlias(src, m_destination->GetType(), 0, 0);
        uint32_t N = srcEltBytes / dstEltBytes;
        // Similar to dstEltBytes > srcEltBytes, dstEltBytes can be 32bit
        // at most and dst's stride == 1, so it will not need splitting.
        bool splitSrc = (!srcUniform && (srcEltBytes * width > m_currShader->getGRFSize() * 2));
        IGC_ASSERT_MESSAGE((!splitSrc || (width == 16) || (width == 32)),
            "Internal Error: Src needs splitting only under SIMD16!");
        IGC_ASSERT_MESSAGE((srcEltBytes % dstEltBytes) == 0, "Basic types should be power of 2");
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

        // Once BCI has been coalesced, don't separate CVar for BCI
        // [todo evaluate the performance impact and let alias handle it
        //  if needed]
        if (m_currShader->IsCoalesced(BCI))
            useSeparateCVar = false;

        for (unsigned i = 0, offset = 0; i < srcNElts; ++i)
        {
            for (unsigned j = 0; j < N; ++j)
            {
                if (destMask.isSet(i * N + j))
                {
                    if (useSeparateCVar)
                    {
                        CVariable* newDst = m_currShader->GetNewVariable(
                            width, m_destination->GetType(),
                            m_destination->GetAlign(),
                            CName::NONE);
                        VectorBCICVars.push_back(newDst);
                        m_destination = newDst;
                    }
                    if (splitSrc)
                    {
                        // !srcUniform
                        // first half
                        SIMDMode mode = m_currShader->m_SIMDSize == SIMDMode::SIMD32 ? SIMDMode::SIMD16 : SIMDMode::SIMD8;
                        int exSize = mode == SIMDMode::SIMD16 ? 16 : 8;
                        m_encoder->SetSrcRegion(0, N, 1, 0); // = (0, width*N, width, N)
                        m_encoder->SetSrcSubReg(0, i * N * width + j);
                        m_encoder->SetDstSubReg(dstUniform ? offset : (width * offset));
                        m_encoder->SetDstRegion(1);
                        m_encoder->SetSimdSize(mode);
                        m_encoder->SetMask(mode == SIMDMode::SIMD16 ? EMASK_H1 : EMASK_Q1);
                        m_encoder->Copy(m_destination, aliasSrc);
                        m_encoder->Push();

                        // second half
                        m_encoder->SetSrcRegion(0, N, 1, 0); // = (0, width*N, width, N)
                        m_encoder->SetSrcSubReg(0, i * N * width + N * exSize + j);
                        m_encoder->SetDstSubReg(dstUniform ? offset : (width * offset + exSize));
                        m_encoder->SetDstRegion(1);
                        m_encoder->SetSimdSize(mode);
                        m_encoder->SetMask(mode == SIMDMode::SIMD16 ? EMASK_H2 : EMASK_Q2);
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

unsigned int EmitPass::GetPrimitiveTypeSizeInRegisterInBits(const Type* Ty) const
{
    return m_currShader->GetPrimitiveTypeSizeInRegisterInBits(Ty);
}

unsigned int EmitPass::GetPrimitiveTypeSizeInRegister(const Type* Ty) const
{
    return m_currShader->GetPrimitiveTypeSizeInRegister(Ty);
}

unsigned int EmitPass::GetScalarTypeSizeInRegisterInBits(const Type* Ty) const
{
    return m_currShader->GetScalarTypeSizeInRegisterInBits(Ty);
}

unsigned int EmitPass::GetScalarTypeSizeInRegister(const Type* Ty) const
{
    return m_currShader->GetScalarTypeSizeInRegister(Ty);
}

void EmitPass::A64LSLoopHead(
    CVariable* addr, CVariable*& curMask, CVariable*& lsPred, uint& label)
{
    // Create a loop to calculate LS's pred (lsPred) that make sure for every active lane of the LS,
    // the address hi part must be the same
    //
    // pseudo code (including A64LSLoopHead and A64LSLoopTail):
    //          addrHigh = packed addr hi part
    //          curMask = executionMask
    //      label:
    //          uniformAddrHi = the_first_active_lane_of_CurMask(addrHigh)
    //          lsPred = cmp(uniformAddrHi, addrHigh)
    //          (lsPred) send // the original LS instruction
    //          lsPred = ~lsPred
    //          CurMask = lsPred & CurMask
    //          lsPred = CurMask
    //          (lsPred) jmp label

    SIMDMode simdMode = m_encoder->GetSimdSize();
    uint16_t execSize = numLanes(simdMode);
    IGC_ASSERT(simdMode == SIMDMode::SIMD8 || simdMode == SIMDMode::SIMD16);

    // get address hi part
    CVariable* addrAlias = m_currShader->GetNewAlias(addr, ISA_TYPE_UD, 0, execSize * 2);
    CVariable* addrHigh = m_currShader->GetNewVariable(
        execSize, ISA_TYPE_UD, EALIGN_GRF, false, CName::NONE);
    m_encoder->SetSrcSubReg(0, 1);
    m_encoder->SetSrcRegion(0, 2, 1, 0);
    m_encoder->Copy(addrHigh, addrAlias);
    m_encoder->Push();

    curMask = GetHalfExecutionMask();

    // create loop
    label = m_encoder->GetNewLabelID("a64_loop");
    m_encoder->AddLabel(label);
    m_encoder->Push();

    // Get the first active lane's address-hi
    CVariable* ufoffset = nullptr;
    CVariable* uniformAddrHi = UniformCopy(addrHigh, ufoffset, curMask, true);

    // Set the predicate lsPred to true for all lanes with the same address_hi
    lsPred = m_currShader->GetNewVariable(
        numLanes(m_currShader->m_State.m_dispatchSize), ISA_TYPE_BOOL, EALIGN_BYTE, CName::NONE);
    m_encoder->Cmp(EPREDICATE_EQ, lsPred, uniformAddrHi, addrHigh);
    m_encoder->Push();
}

void EmitPass::A64LSLoopTail(CVariable* curMask, CVariable* lsPred, uint label)
{
    // Unset the bits in the mask for lanes that were executed
    bool tmpSh = m_encoder->IsSecondHalf();
    m_encoder->SetSecondHalf(false);

    CVariable* tmpLsPred = m_currShader->GetNewVariable(1, curMask->GetType(), curMask->GetAlign(), true, CName::NONE);
    m_encoder->Cast(tmpLsPred, lsPred);

    m_encoder->SetSrcModifier(1, EMOD_NOT);
    m_encoder->And(curMask, curMask, tmpLsPred);
    m_encoder->Push();
    m_encoder->SetP(lsPred, curMask);
    m_encoder->Push();
    m_encoder->Jump(lsPred, label);
    m_encoder->Push();

    m_encoder->SetSecondHalf(tmpSh);
}

bool EmitPass::hasA64WAEnable() const
{
    // Check WA table entry for current platform.
    if (!m_currShader->m_Platform->WaEnableA64WA())
        return false;

    // -intel-force-enable-a64WA
    if (m_pCtx->getModuleMetaData()->compOpt.ForceEnableA64WA)
        return true;

    // -intel-disable-a64WA
    if (m_pCtx->getModuleMetaData()->compOpt.DisableA64WA)
        return false;

    // Disable A64WA for kernels which specify work_group_size_hint(1, 1, 1).
    MetaDataUtils* pMdUtils =  m_currShader->GetMetaDataUtils();
    uint32_t WGSize = IGCMetaDataHelper::getThreadGroupSizeHint(*pMdUtils, m_currShader->entry);
    if (WGSize == 1)
        return false;

    return true;
}

void EmitPass::emitGatherA64(Value* loadInst, CVariable* dst, CVariable* offset, unsigned elemSize, unsigned numElems, bool addrUniform)
{
    if (hasA64WAEnable() && !offset->IsUniform() && !addrUniform) {
        CVariable* curMask = nullptr;
        CVariable* lsPred = nullptr;
        uint label = 0;
        A64LSLoopHead(offset, curMask, lsPred, label);

        // do send with pred
        if (isa<LoadInst>(loadInst) && !m_currShader->IsCoalesced(loadInst))
        {
            // load inst is the single def of the vISA variable and therefore a kill
            m_encoder->Lifetime(LIFETIME_START, dst);
        }
        m_encoder->SetPredicate(lsPred);
        m_encoder->GatherA64(dst, offset, elemSize, numElems);
        m_encoder->Push();

        A64LSLoopTail(curMask, lsPred, label);

    } else {
        m_encoder->GatherA64(dst, offset, elemSize, numElems);
    }
}

void EmitPass::emitGather4A64(Value* loadInst, CVariable* dst, CVariable* offset, bool addrUniform)
{
    if (hasA64WAEnable() && !offset->IsUniform() && !addrUniform) {
        CVariable* curMask = nullptr;
        CVariable* lsPred = nullptr;
        uint label = 0;
        A64LSLoopHead(offset, curMask, lsPred, label);

        // do send with pred
        if (isa<LoadInst>(loadInst) && !m_currShader->IsCoalesced(loadInst))
        {
            // load inst is the single def of the vISA variable and therefore a kill
            m_encoder->Lifetime(LIFETIME_START, dst);
        }
        m_encoder->SetPredicate(lsPred);
        m_encoder->Gather4A64(dst, offset);
        m_encoder->Push();

        A64LSLoopTail(curMask, lsPred, label);

    }
    else {
        m_encoder->Gather4A64(dst, offset);
    }
}

void EmitPass::emitScatterA64(CVariable* val, CVariable* offset, unsigned elementSize, unsigned numElems, bool addrUniform)
{
    if (hasA64WAEnable() && !offset->IsUniform() && !addrUniform) {
        CVariable* curMask = nullptr;
        CVariable* lsPred = nullptr;
        uint label = 0;
        A64LSLoopHead(offset, curMask, lsPred, label);

        // do send with pred
        m_encoder->SetPredicate(lsPred);
        m_encoder->ScatterA64(val, offset, elementSize, numElems);
        m_encoder->Push();

        A64LSLoopTail(curMask, lsPred, label);

    }
    else {
        m_encoder->ScatterA64(val, offset, elementSize, numElems);
    }
}

void EmitPass::emitScatter4A64(CVariable* src, CVariable* offset, bool addrUniform)
{
    if (hasA64WAEnable() && !offset->IsUniform() && !addrUniform) {
        CVariable* curMask = nullptr;
        CVariable* lsPred = nullptr;
        uint label = 0;
        A64LSLoopHead(offset, curMask, lsPred, label);

        // do send with pred
        m_encoder->SetPredicate(lsPred);
        m_encoder->Scatter4A64(src, offset);
        m_encoder->Push();

        A64LSLoopTail(curMask, lsPred, label);

    }
    else {
        m_encoder->Scatter4A64(src, offset);
    }
}

void EmitPass::emitFastClear(llvm::LoadInst* inst)
{
    for (auto u : inst->users())
    {
        if (llvm::GenIntrinsicInst* genISA_user = dyn_cast<llvm::GenIntrinsicInst>(u))
        {
            if (genISA_user->getIntrinsicID() == GenISAIntrinsic::GenISA_HDCCCSFastClear)
            {
                emitFastClearSend(inst);
                return;
            }
        }
    }

}

void EmitPass::emitFastClearSend(llvm::Instruction* pInst)
{
    LoadInst* inst = dyn_cast<LoadInst>(pInst);
    Value* Ptr = inst->getPointerOperand();
    CVariable* eOffset = GetSymbol(Ptr);

    CVariable* pPayload = nullptr;
    pPayload = m_currShader->GetNewVariable(
        8,
        ISA_TYPE_UQ,
        EALIGN_GRF,
        true, CName::NONE);
    m_encoder->SetSimdSize(SIMDMode::SIMD1);
    m_encoder->Copy(pPayload, eOffset);
    m_encoder->Push();

    uint messageDescriptor = 0x020DC0FF;
    CVariable* pMessDesc = m_currShader->ImmToVariable(messageDescriptor, ISA_TYPE_D);

    uint32_t exDescVal = EU_MESSAGE_TARGET_DATA_PORT_DATA_CACHE_READ_ONLY;
    CVariable* exDesc =
        m_currShader->ImmToVariable(exDescVal, ISA_TYPE_D);

    m_encoder->SetSimdSize(SIMDMode::SIMD1);
    m_encoder->SetNoMask();

    m_encoder->Send(
        NULL,
        pPayload,
        EU_MESSAGE_TARGET_DATA_PORT_DATA_CACHE_READ_ONLY,
        exDesc,
        pMessDesc);

    m_encoder->Push();
}


void EmitPass::emitVectorLoad(LoadInst* inst, Value* offset, ConstantInt* immOffset)
{
    int immOffsetInt = 0;
    if (immOffset)
        immOffsetInt = static_cast<int>(immOffset->getSExtValue());

    Value* Ptr = inst->getPointerOperand();
    PointerType* ptrType = cast<PointerType>(Ptr->getType());
    bool useA32 = !IGC::isA64Ptr(ptrType, m_currShader->GetContext());

    ResourceDescriptor resource = GetResourceVariable(Ptr);
    CountStatelessIndirectAccess(Ptr, resource);
    // eOffset is in bytes
    // offset corresponds to Int2Ptr operand obtained during pattern matching
    CVariable* eOffset = GetSymbol(immOffset ? offset : Ptr);
    if (useA32)
    {
        eOffset = TruncatePointer(eOffset);
    }

    Type* Ty = inst->getType();
    IGCLLVM::FixedVectorType* VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
    Type* eltTy = VTy ? VTy->getElementType() : Ty;
    uint32_t eltBytes = GetScalarTypeSizeInRegister(eltTy);
    IGC_ASSERT_MESSAGE((eltBytes == 1) || (eltBytes == 2) || (eltBytes == 4) || (eltBytes == 8),
        "Load's type (element type if vector) must be 1/2/4/8-byte long");

    uint32_t elts = VTy ? int_cast<uint32_t>(VTy->getNumElements()) : 1;
    uint32_t totalBytes = eltBytes * elts;

    bool destUniform = m_destination->IsUniform();
    bool srcUniform = eOffset->IsUniform();
    // Not possible to have uniform dest AND non-uniform src.
    IGC_ASSERT_MESSAGE(!(destUniform && !srcUniform),
        "If ld's dest is uniform, ld's src must be uniform");

    auto align = IGCLLVM::getAlignmentValue(inst);
    VISA_Type destType = m_destination->GetType();
    uint32_t width = numLanes(m_currShader->m_SIMDSize);
    uint bufferIndex = 0;
    bool directIndexing = false;
    BufferType bufType = DecodeAS4GFXResource(ptrType->getAddressSpace(), directIndexing, bufferIndex);

    if (bufType == STATELESS_A32)
    {
        // Lower addressspace (5) loads to A32 oword ld
        CVariable* loadDest = m_destination;
        uint size = loadDest->GetSize();
        auto newDest = loadDest;
        if (bufType == STATELESS_A32)
        {
            auto r0 = m_currShader->GetR0();
            m_encoder->SetSimdSize(SIMDMode::SIMD1);
            m_encoder->SetNoMask();
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->SetSrcSubReg(0, 0);
            auto dst = m_currShader->GetNewVariable(1, VISA_Type::ISA_TYPE_D, eOffset->GetAlign(), true, "OWOff");
            m_encoder->And(dst, r0, m_currShader->ImmToVariable(0xffffffe0, ISA_TYPE_UD));
            m_encoder->Push();
            m_encoder->SetSimdSize(SIMDMode::SIMD1);
            m_encoder->SetNoMask();
            m_encoder->Add(dst, dst, eOffset);
            m_encoder->Push();
            eOffset = dst;
            if (!iSTD::IsPowerOfTwo(size) || size < SIZE_OWORD)
            {
                // Ensure payload size is power of 2 or at least 16
                if (size < SIZE_OWORD)
                {
                    size = std::max<unsigned int>(size, SIZE_OWORD);
                }
                else if (!iSTD::IsPowerOfTwo(size))
                {
                    // llvm optimizer converts vector load <i64 x 4> in to <i64 x 3> if
                    // last element isnt used. Recompute size to next higher power of 2.
                    size = (uint)std::pow(2, std::ceil(std::log2(size)));
                }
                newDest = m_currShader->GetNewVariable(size / loadDest->GetElemSize(), loadDest->GetType(), EALIGN_GRF, true, CName::NONE);
            }
        }
        m_encoder->OWLoad(newDest, resource, eOffset, false, size);
        if (newDest != loadDest)
        {
            emitVectorCopy(loadDest, newDest, loadDest->GetNumberElement());
        }
        return;
    }

    // First, special handling for less than 4 bytes of loaded value
    if (totalBytes < 4)
    {
        // totalBytes is either 1 or 2, and it must be scalar or (1-element vector)
        // do not expect <2 x i8> or <3 x i8>
        IGC_ASSERT(elts == 1);
        IGC_ASSERT(totalBytes != 3);

        uint16_t nbelts = srcUniform ? 1 : width;
        e_alignment align = EALIGN_GRF;

        eOffset = ReAlignUniformVariable(eOffset, align);

        CVariable* gatherDst = m_currShader->GetNewVariable(nbelts, ISA_TYPE_UD, align, srcUniform, CName::NONE);

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
            emitGatherA64(inst, gatherDst, eOffset, 8, totalBytes, srcUniform);
        }

        m_encoder->Push();

        gatherDst = m_currShader->GetNewAlias(gatherDst, destType, 0, 0);
        uint32_t vStride = srcUniform ? 0 : ((totalBytes == 1) ? 4 : 2);
        m_encoder->SetSrcRegion(0, vStride, 1, 0);
        m_encoder->Copy(m_destination, gatherDst);
        m_encoder->Push();
        return;
    }

    bool bEmulateDWAligned = false;

    // generate oword-load if possible
    if (VTy && srcUniform)
    {
        //uint32_t totalBytes = eltBytes * VTy->getNumElements();
        bool rightBlockSize = (totalBytes == 16 || totalBytes == 32 || totalBytes == 64 || totalBytes == 128);
        bool useDWAligned = (resource.m_surfaceType != ESURFACE_SLM && align && align >= 4);
        //if originally, unalignedDW is used for SSS in XeHP_SDV and above, emulate it with Gather4Scaled
        bEmulateDWAligned = (rightBlockSize && useDWAligned &&
            m_currShader->m_Platform->hasScratchSurface() && resource.m_surfaceType == ESURFACE_SCRATCH && align && align >= 4);
        useDWAligned &= (!bEmulateDWAligned);
        bool useOWAligned = (resource.m_surfaceType == ESURFACE_SLM && align && align >= 16 &&
            m_currShader->m_Platform->supportSLMBlockMessage());

        if (rightBlockSize && (useDWAligned || useOWAligned))
        {
            bool needTemp = (!destUniform || !IsGRFAligned(m_destination, EALIGN_GRF));
            CVariable * loadDest = m_destination;

            if (useOWAligned)
            {
                // Offset needs to be in OW!
                // Need to create a new cvar as eOffset could be used by others.

                CVariable* tmp = m_currShader->GetNewVariable(eOffset);
                m_encoder->Shr(tmp, eOffset, m_currShader->ImmToVariable(4, ISA_TYPE_UD));
                m_encoder->Push();
                eOffset = tmp;
            }
            eOffset = ReAlignUniformVariable(eOffset, EALIGN_GRF);
            if (needTemp)
            {
                loadDest = m_currShader->GetNewVariable(
                    int_cast<uint16_t>(VTy->getNumElements()),
                    m_destination->GetType(),
                    EALIGN_GRF,
                    true, CName::NONE);
            }

            if (useA32)
            {
                m_encoder->OWLoad(loadDest, resource, eOffset, useOWAligned, loadDest->GetSize());
            }
            else
            {
                IGC_ASSERT_MESSAGE(!useOWAligned, "SLM's pointer size must be 32 bit!");
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
    //Note: it doesn't seem to be necessary to check hasNoFP64Inst() here.
    if (srcUniform && (totalBytes == 4 || totalBytes == 8 || totalBytes == 12 || totalBytes == 16 ||
        (totalBytes == 32 && (useA32 || !m_currShader->m_Platform->hasNoFullI64Support()))))
    {
        bool needTemp = !destUniform ||
            !IsGRFAligned(m_destination, EALIGN_GRF) ||
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
        uint32_t nBlks = useA32 ? 4 : (align < 4 ? (useQW ? 8 : 4) : 1);

        VISA_Type ldType = useQW ? ISA_TYPE_UQ : ISA_TYPE_UD;
        CVariable* gatherDst;
        if (needTemp)
        {
            gatherDst = m_currShader->GetNewVariable(
                nbelts, ldType, EALIGN_GRF, true /*srcUniform*/, CName::NONE);
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
        CVariable* gatherOff = eOffset;
        if (nbelts > 1)
        {
            gatherOff = m_currShader->GetNewVariable(
                nbelts, eOffset->GetType(), eOffset->GetAlign(), true /*srcUniform*/, CName::NONE);
            // May have the following
            //   lane   0   1   2   3   4    5   6   7
            //   eOff   0   4   8   C   10   14  18  1C // DW per lane
            //   eOff   0   8                           // QW per lane
            // When nbelts = 3, lane 3 is not used. Since we don't have simd3,
            // use simd4 and set lane3 to lane2.
            uint32_t incImm = 0;
            uint32_t incImm1 = 0;  // for activelanes=8
            switch (activelanes) {
            default:
                IGC_ASSERT_MESSAGE(0, "ICE: something wrong happened in computing activelanes!");
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
                incImm = 0xD951C840;
                incImm1 = 0xFFFF0000;
                break;
            }

            CVariable* immVar = m_currShader->ImmToVariable(incImm, ISA_TYPE_UV);
            if (!useA32 && !m_currShader->m_Platform->hasInt64Add()) {
                emitAddPair(gatherOff, eOffset, immVar);
            }
            else {
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
            if (m_currShader->m_Platform->hasScratchSurface() &&
                align >= 4 &&
                (m_currShader->m_Platform->emulateByteScraterMsgForSS() || bEmulateDWAligned) &&
                (ESURFACE_SCRATCH == resource.m_surfaceType))
            {
                m_encoder->Gather4Scaled(gatherDst, resource, gatherOff);
            }
            else
            {
                m_encoder->ByteGather(gatherDst, resource, gatherOff, blkBits, nBlks);
            }
        }
        else
        {
            emitGatherA64(inst, gatherDst, gatherOff, blkBits, nBlks, srcUniform);
        }
        m_encoder->Push();

        if (needTemp)
        {
            CVariable* newDst = m_currShader->GetNewAlias(
                gatherDst, destType, 0, m_destination->GetNumberElement());
            emitVectorCopy(m_destination, newDst, elts);
        }
        return;
    }
    CVariable* subLoadDst;
    CVariable* rawAddrVar;

    // some driver describe constant buffer as typed which forces us to use byte scatter message
    bool forceByteScatteredRW =
        bufType == CONSTANT_BUFFER &&
        UsesTypedConstantBuffer(m_currShader->GetContext(), bufType);

    VectorMessage VecMessInfo(this);
    VecMessInfo.getInfo(Ty, align, useA32, forceByteScatteredRW);

    // Handle uniform case in general
    if (srcUniform)
    {
        // Use width of 8 always, and only the value of the first lane is
        // used. Need to set noMask in order to have the valid value in
        // the first lane.
        uint32_t width8 = getGRFSize() / 4;
        for (uint32_t i = 0; i < VecMessInfo.numInsts; ++i)
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

            if (i > 0)
            {
                // Calculate the new element offset
                rawAddrVar = m_currShader->GetNewVariable(eOffset);
                CVariable* ImmVar = m_currShader->ImmToVariable(eltOffBytes, ISA_TYPE_UD);
                if (!useA32 && !m_currShader->m_Platform->hasInt64Add()) {
                    emitAddPair(rawAddrVar, eOffset, ImmVar);
                }
                else {
                    m_encoder->SetNoMask();
                    m_encoder->Add(rawAddrVar, eOffset, ImmVar);
                    m_encoder->Push();
                }
            }
            else
            {
                rawAddrVar = eOffset;
            }
            CVariable* addrVarSIMD8 = m_currShader->GetNewVariable(
                getGRFSize() / 4, rawAddrVar->GetType(), EALIGN_GRF, CName::NONE);
            m_encoder->SetNoMask();
            m_encoder->SetSimdSize(lanesToSIMDMode(addrVarSIMD8->GetNumberElement()));
            m_encoder->Copy(addrVarSIMD8, rawAddrVar);

            subLoadDst = m_currShader->GetNewVariable(
                (uint16_t)nbelts, destType, EALIGN_GRF, CName::NONE);
            m_encoder->SetNoMask();
            m_encoder->SetSimdSize(lanesToSIMDMode(addrVarSIMD8->GetNumberElement()));
            VectorMessage::MESSAGE_KIND messageType = VecMessInfo.insts[i].kind;
            switch (messageType) {
            case VectorMessage::MESSAGE_A32_BYTE_SCATTERED_RW:
                m_encoder->ByteGather(subLoadDst, resource, addrVarSIMD8, blkBits, numBlks);
                break;
            case VectorMessage::MESSAGE_A32_UNTYPED_SURFACE_RW:
                m_encoder->Gather4Scaled(subLoadDst, resource, addrVarSIMD8);
                break;
            case VectorMessage::MESSAGE_A64_UNTYPED_SURFACE_RW:
                emitGather4A64(inst, subLoadDst, addrVarSIMD8, true);
                break;
            case VectorMessage::MESSAGE_A64_SCATTERED_RW:
                emitGatherA64(inst, subLoadDst, addrVarSIMD8, blkBits, numBlks, true);
                break;
            case VectorMessage::MESSAGE_A32_QWORD_SCATTERED_RW:
                m_encoder->QWGather(subLoadDst, resource, addrVarSIMD8, blkBits, numBlks);
                break;
            default:
                IGC_ASSERT_MESSAGE(0, "Somethings wrong!");
            }
            m_encoder->Push();

            for (uint32_t n = 0; n < instElts; ++n)
            {
                m_encoder->SetSrcRegion(0, 0, 1, 0);
                m_encoder->SetSrcSubReg(0, n * width8);
                m_encoder->SetDstSubReg(eltOff + (destUniform ? n : n * width));
                m_encoder->Copy(m_destination, subLoadDst);
                m_encoder->Push();
            }
        }

        return;
    }

    auto [HasEM, Mask] = m_currShader->getExtractMask(inst);
    if (!HasEM) {
        // Ensure 'Mask' is cleared.
        Mask = 0;
    }

    // Second, src isn't uniform
    for (uint32_t i = 0; i < VecMessInfo.numInsts; ++i)
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

        if (i > 0)
        {
            // Calculate the new element offset
            rawAddrVar = m_currShader->GetNewVariable(eOffset);
            CVariable* ImmVar = m_currShader->ImmToVariable(VecMessInfo.insts[i].startByte, ISA_TYPE_UD);
            if (!useA32 && !m_currShader->m_Platform->hasInt64Add()) {
                emitAddPair(rawAddrVar, eOffset, ImmVar);
            }
            else {
                m_encoder->Add(rawAddrVar, eOffset, ImmVar);
                m_encoder->Push();
            }
        }
        else
        {
            rawAddrVar = eOffset;
        }

        bool needTemp = (!IsGRFAligned(m_destination, EALIGN_GRF));
        CVariable* gatherDst;
        if (needTemp)
        {
            gatherDst = m_currShader->GetNewVariable(
                (uint16_t)nbelts, destType, EALIGN_GRF, CName::NONE);
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
            m_encoder->Gather4Scaled(gatherDst, resource, rawAddrVar, Mask);
            break;
        case VectorMessage::MESSAGE_A64_UNTYPED_SURFACE_RW:
            emitGather4A64(inst, gatherDst, rawAddrVar, false);
            break;
        case VectorMessage::MESSAGE_A64_SCATTERED_RW:
            emitGatherA64(inst, gatherDst, rawAddrVar, blkBits, numBlks, false);
            break;
        case VectorMessage::MESSAGE_A32_QWORD_SCATTERED_RW:
            m_encoder->QWGather(gatherDst, resource, rawAddrVar, blkBits, numBlks);
            break;
        default:
            IGC_ASSERT_MESSAGE(0, "Internal Error: unexpected message kind for load!");
        }
        m_encoder->Push();

        if (needTemp)
        {
            emitVectorCopy(m_destination, gatherDst, instElts, eltOff, 0);
        }
    }
}

void EmitPass::emitVectorStore(StoreInst* inst, Value* offset, ConstantInt* immOffset)
{
    int immOffsetInt = 0;
    if (immOffset)
        immOffsetInt = static_cast<int>(immOffset->getSExtValue());

    Value* Ptr = inst->getPointerOperand();
    PointerType* ptrType = cast<PointerType>(Ptr->getType());
    BasicBlock* BB = inst->getParent();

    ResourceDescriptor resource = GetResourceVariable(Ptr);
    CountStatelessIndirectAccess(Ptr, resource);
    if (ptrType->getPointerAddressSpace() != ADDRESS_SPACE_PRIVATE)
    {
        ForceDMask(false);
    }
    // eOffset is in bytes
    // offset corresponds to Int2Ptr operand obtained during pattern matching
    CVariable* eOffset = GetSymbol(immOffset ? offset : Ptr);
    bool useA32 = !isA64Ptr(ptrType, m_currShader->GetContext());
    if (useA32)
    {
        eOffset = TruncatePointer(eOffset);
    }

    // In case eOffset isn't GRF aligned, need to create a copy
    // For non-uniform variable, it should be already GRF-aligned.
    eOffset = ReAlignUniformVariable(eOffset, EALIGN_GRF);

    Value* storedVal = inst->getValueOperand();
    uint32_t eltBytes, elts;
    Type* Ty = storedVal->getType();

    CVariable* storedVar = tryReusingConstVectorStoreData(storedVal, BB, false);
    if (!storedVar) storedVar = GetSymbol(storedVal);

    if (Ty->isStructTy())
    {
        eltBytes = 4;
        elts = 8;
        storedVar = m_currShader->GetNewAlias(storedVar, ISA_TYPE_UD, 0, elts);
    }
    else
    {
        IGCLLVM::FixedVectorType* VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
        Type* eltTy = VTy ? VTy->getElementType() : Ty;
        eltBytes = GetScalarTypeSizeInRegister(eltTy);
        elts = VTy ? int_cast<uint32_t>(VTy->getNumElements()) : 1;
    }

    IGC_ASSERT_MESSAGE((eltBytes == 1) || (eltBytes == 2) || (eltBytes == 4) || (eltBytes == 8),
        "Store type must be 1/2/4/8-bytes long");

    uint32_t totalBytes = elts * eltBytes;
    auto align = IGCLLVM::getAlignmentValue(inst);
    unsigned int width = numLanes(m_currShader->m_SIMDSize);

    bool srcUniform = storedVar->IsUniform();
    bool dstUniform = eOffset->IsUniform();

    // Handle two cases:
    //   1. less than 4 bytes: need to extend it to 4 bytes
    //   2. destination is scalar and uniform (handle vector if needed)
    if (totalBytes < 4 || isUniformStoreOCL(inst))
    {
        IGC_ASSERT_MESSAGE((totalBytes == 1) || (totalBytes == 2) || (totalBytes == 4) || (totalBytes == 8) || (totalBytes == 12) || (totalBytes == 16),
            "Wrong total Bytes!");

        SIMDMode simdmode = SIMDMode::SIMD1;
        e_alignment grfAlign = useA32 ? EALIGN_GRF : EALIGN_2GRF;
        uint32_t blkBits, nBlks;
        if (elts > 1)
        {
            // Vector uniform store: handle uniform value only.
            // For elts > 1, the eltBytes must be either 4 or 8; only elts = 2|3|4 are handled.
            IGC_ASSERT_MESSAGE((eltBytes == 4) || (eltBytes == 8), "ICE: wrong element bytes!");
            IGC_ASSERT_MESSAGE(dstUniform, "ICE: for vector uniform store, both dst and src must be uniform!");
            IGC_ASSERT_MESSAGE(srcUniform, "ICE: for vector uniform store, both dst and src must be uniform!");

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
                CVariable* NewOff = m_currShader->GetNewVariable(
                    nbelts, eOffset->GetType(), grfAlign, true /*dstUniform*/, CName::NONE);
                uint32_t incImm =
                    useQW ? 0x80 : (activelanes == 2 ? 0x40 : (activelanes == 3 ? 0x8840 : 0xC840));
                CVariable* immVar = m_currShader->ImmToVariable(incImm, ISA_TYPE_UV);

                // When work-around of A64 SKL Si limitation of SIMD4, we use SIMD8 (nbelts > nbeltsWanted)
                // in which all upper four channels are zero, meaning eOffset[0], Later, stored value
                // must use storvedVar[0] for those extra lanes.
                if (!useA32 && !m_currShader->m_Platform->hasInt64Add()) {
                    emitAddPair(NewOff, eOffset, immVar);
                }
                else {
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
            IGC_ASSERT_MESSAGE(useQW || (eltBytes == 4), "ICE: wrong vector element type!");

            // Since we might change element type, need to create copy.
            if (useQW && eltBytes == 4)
            {
                CVariable* tmp = m_currShader->GetNewVariable(
                    nbeltsWanted, ISA_TYPE_UQ, grfAlign, true /*srcUniform*/, CName::NONE);
                CVariable* tmpAlias = m_currShader->GetNewAlias(tmp,
                    storedVar->GetType(), 0, 2 * nbeltsWanted);
                IGC_ASSERT_MESSAGE((2 * nbeltsWanted) == storedVar->GetNumberElement(),
                    "Mismatch of the number of elements: sth wrong!");
                emitVectorCopy(tmpAlias, storedVar, 2 * nbeltsWanted);
                storedVar = tmp;
            }

            // Prepare stored value
            if (storedVar->IsImmediate() || activelanes < nbelts ||
                !IsGRFAligned(storedVar, grfAlign))
            {
                CVariable* NewVar = m_currShader->GetNewVariable(
                    nbelts, storedVar->GetType(), grfAlign, true /*srcUniform*/, CName::NONE);

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

        // no need for discard predicate if we are writing to scratch - this is our
        // internal memory, shader output remain the same, but we avoid problems
        // when, for example, texture coordinates are spilled
        if (resource.m_surfaceType != ESURFACE_SCRATCH)
        {
            setPredicateForDiscard();
        }

        if (useA32)
        {
            m_encoder->ByteScatter(storedVar, resource, eOffset, blkBits, nBlks);
        }
        else
        {
            emitScatterA64(storedVar, eOffset, blkBits, nBlks, true);
        }

        if (dstUniform)
        {
            m_encoder->SetNoMask();
            m_encoder->SetUniformSIMDSize(simdmode);
        }
        m_encoder->Push();
    }
    else
    {
        eOffset = BroadcastIfUniform(eOffset);

        CVariable* broadcastedVar = tryReusingConstVectorStoreData(storedVal, BB, true);
        storedVar = broadcastedVar ? broadcastedVar : BroadcastIfUniform(storedVar);

        VectorMessage VecMessInfo(this);
        VecMessInfo.getInfo(Ty, align, useA32);

        for (uint32_t i = 0; i < VecMessInfo.numInsts; ++i)
        {
            // raw operand, eltOff is in bytes
            uint32_t eltOffBytes = VecMessInfo.insts[i].startByte * width;
            uint32_t blkInBytes = VecMessInfo.insts[i].blkInBytes;
            uint32_t numBlks = VecMessInfo.insts[i].numBlks;
            uint32_t blkBits = 8 * blkInBytes;
            uint32_t instTotalBytes = blkInBytes * numBlks;
            uint32_t instElts = instTotalBytes / eltBytes;
            uint32_t nbelts = instElts * width;

            CVariable* rawAddrVar;
            if (i > 0)
            {
                // Calculate the new element offset
                rawAddrVar = m_currShader->GetNewVariable(eOffset);
                CVariable* ImmVar = m_currShader->ImmToVariable(VecMessInfo.insts[i].startByte, ISA_TYPE_UD);
                if (!useA32 && !m_currShader->m_Platform->hasInt64Add()) {
                    emitAddPair(rawAddrVar, eOffset, ImmVar);
                }
                else {
                    m_encoder->Add(rawAddrVar, eOffset, ImmVar);
                    m_encoder->Push();
                }
            }
            else
            {
                rawAddrVar = eOffset;
            }

            // no need for discard predicate if we are writing to scratch - this is our
            // internal memory, shader output remain the same, but we avoid problems
            // when, for example, texture coordinates are spilled
            if (resource.m_surfaceType != ESURFACE_SCRATCH)
            {
                setPredicateForDiscard();
            }
            VISA_Type storedType = storedVar->GetType();
            IGC_ASSERT_MESSAGE((eltOffBytes < (UINT16_MAX)), "eltOffBytes > higher than 64k");
            IGC_ASSERT_MESSAGE((nbelts < (UINT16_MAX)), "nbelts > higher than 64k");
            CVariable* subStoredVar = m_currShader->GetNewAlias(storedVar, storedType, (uint16_t)eltOffBytes, (uint16_t)nbelts);
            switch (VecMessInfo.insts[i].kind) {
            case VectorMessage::MESSAGE_A32_BYTE_SCATTERED_RW:
                m_encoder->ByteScatter(subStoredVar, resource, rawAddrVar, blkBits, numBlks);
                break;
            case VectorMessage::MESSAGE_A32_UNTYPED_SURFACE_RW:
                m_encoder->Scatter4Scaled(subStoredVar, resource, rawAddrVar);
                break;
            case VectorMessage::MESSAGE_A64_UNTYPED_SURFACE_RW:
                emitScatter4A64(subStoredVar, rawAddrVar, false);
                break;
            case VectorMessage::MESSAGE_A64_SCATTERED_RW:
                emitScatterA64(subStoredVar, rawAddrVar, blkBits, numBlks, false);
                break;
            case VectorMessage::MESSAGE_A32_QWORD_SCATTERED_RW:
                m_encoder->QWScatter(subStoredVar, resource, rawAddrVar, blkBits, numBlks);
                break;
            default:
                IGC_ASSERT_MESSAGE(0, "Internal Error: unexpected Message kind for store");
            }
            m_encoder->Push();
        }
    }
    if (ptrType->getPointerAddressSpace() != ADDRESS_SPACE_PRIVATE)
    {
        ResetVMask(false);
    }
}

// prepareAddressForUniform():       for both load and store
// prepareDataForUniform():          for store only
//    Unaligned (less than 4 bytes) uniform load/store. One for address payload,
//    and the other for data payload.
//
//  Example 1:  "store <4xi32> V,  <4xi32>* P, align 2"
//     A new pointer pVar is create with 4 elements.
//
//     add  (4|M0_NM) pVar<1>:ud  P<0;1,0>:UD 0xC840:UV
//     send (4|M0_NM) pVar   V
//
//         prepareAddressForUniform() : create pVar
//         prepareDataForUniform() : return V (assuming V can be used directly)
//
//  Example 2:  "store <3xi32> V,  <3xi32>* P, align 2"
//     Non-power of 2 vector size is rounded up to the next power of 2.
//     Additional elements are duplicated with the first vector element.

//     add  (4|M0_NM) pVar<1>:ud  P<0;1,0>:UD 0x0840:UV
//     mov  (4|M0_NM) vVar<1>:ud  V<0;1,0>:ud
//     mov  (2|M0_NM) vVar<1>:ud  V<1;1,0>:ud
//     mov  (1|M0_NM) vVar.2<1>:ud  V.2<1;1,0>:ud
//     send (4|M0_NM) vVar  pVar
//
//         prepareAddressForUniform() : create pVar
//         prepareDataForUniform() : return vVar
//
// This function handles vector size up to 8. It also handles QW element size.
// When vector size > 4, it uses 0x76543210, left-shifted by 2 (DW) or 3 (QW)
// as an immediate to be added to 'AddrVar' to form a new address var.
//
// In addition, if 64bit add is not supported, emitAddPair() will be used to
// use 32bit add/addc to emulate 64bit add.
//
// Note that argument 'AddrVar' in prepareAddressForUniform() is uniform, so is
// its return var. The argument 'DataVar' in prepareDataForUniform() is uniform,
// so is its return var.
//
CVariable* EmitPass::prepareAddressForUniform(
    CVariable* AddrVar, uint32_t EltBytes, uint32_t NElts, uint32_t RequiredNElts, e_alignment Align)
{
    // If RequiredNElts == 0,  use next power of 2 of NElts as return var's num of elements.
    //    otherwise,           user RequiredNElts as return var's num of elements.
    uint32_t pow2NElts = (uint32_t)PowerOf2Ceil(NElts);
    uint32_t allocNElts = (RequiredNElts > 0 ? RequiredNElts : pow2NElts);
    IGC_ASSERT(NElts <= 8 && (EltBytes == 4 || EltBytes == 8));
    IGC_ASSERT(allocNElts >= pow2NElts);
    if (allocNElts == NElts && NElts == 1 && IsGRFAligned(AddrVar, Align))
    {
        // No need to create a new var.
        return AddrVar;
    }
    bool isA64 = (AddrVar->GetElemSize() == 8);
    SIMDMode simdmode = lanesToSIMDMode(pow2NElts);
    CVariable* newVar = m_currShader->GetNewVariable(allocNElts, AddrVar->GetType(), Align, true, CName::NONE);

    CVariable* off;
    uint32_t incImm = (0x76543210 & maskTrailingOnes<uint32_t>(NElts * 4));
    if ((pow2NElts <= 4 && EltBytes == 4) || (pow2NElts <= 2 && EltBytes == 8))
    {
        // This case needs a single UV immediate
        incImm = incImm << (EltBytes == 4 ? 2 : 3);
        off = m_currShader->ImmToVariable(incImm, ISA_TYPE_UV);
    }
    else
    {
        // Need a temporary var to calculate offsets.
        // (Note that the temp is non-uniform, otherwise emitAddrPair() won't work.)
        off = m_currShader->GetNewVariable(pow2NElts, ISA_TYPE_UD, EALIGN_DWORD, false, CName::NONE);

        // Need a mov and mul
        m_encoder->SetNoMask();
        m_encoder->SetSimdSize(simdmode);
        m_encoder->Copy(off, m_currShader->ImmToVariable(incImm, ISA_TYPE_UV));
        m_encoder->Push();

        m_encoder->SetNoMask();
        m_encoder->SetSimdSize(simdmode);
        m_encoder->SetSrcRegion(0, 1, 1, 0);
        m_encoder->SetSrcRegion(1, 0, 1, 0);
        m_encoder->Mul(off, off, m_currShader->ImmToVariable(EltBytes, ISA_TYPE_UW));
        m_encoder->Push();
    }

    // Only need to initialize pow2NElts elements.
    if (allocNElts > pow2NElts)
    {
        newVar = m_currShader->GetNewAlias(newVar, newVar->GetType(), 0, pow2NElts);
    }

    // Currently, it's impossible to split because of NElts <= 8. In the future, NElts
    // could be 32 and we could need to split.
    bool needSplit = ((pow2NElts * newVar->GetElemSize()) > (2 * (uint32_t)getGRFSize()));
    if (needSplit)
    {
        IGC_ASSERT(!off->IsImmediate());
        uint32_t halfNElts = pow2NElts / 2;
        uint32_t bytes1 = halfNElts * newVar->GetElemSize();
        uint32_t bytes2 = halfNElts * off->GetElemSize();
        CVariable* newVarHi = m_currShader->GetNewAlias(newVar, newVar->GetType(), 0, halfNElts);
        CVariable* newVarLo = m_currShader->GetNewAlias(newVar, newVar->GetType(), bytes1, halfNElts);
        CVariable* offHi = m_currShader->GetNewAlias(off, off->GetType(), 0, halfNElts);
        CVariable* offLo = m_currShader->GetNewAlias(off, off->GetType(), bytes2, halfNElts);

        if (isA64 && !m_currShader->m_Platform->hasInt64Add())
        {
            emitAddPair(newVarHi, AddrVar, offHi);
            emitAddPair(newVarLo, AddrVar, offLo);
        }
        else
        {
            SIMDMode sm = lanesToSIMDMode(halfNElts);
            m_encoder->SetNoMask();
            m_encoder->SetUniformSIMDSize(sm);
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->SetSrcRegion(1, 1, 1, 0);
            m_encoder->Add(newVarHi, AddrVar, offHi);
            m_encoder->Push();

            m_encoder->SetNoMask();
            m_encoder->SetUniformSIMDSize(sm);
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->SetSrcRegion(1, 1, 1, 0);
            m_encoder->Add(newVarLo, AddrVar, offLo);
            m_encoder->Push();
        }
    }
    else if (isA64 && !m_currShader->m_Platform->hasInt64Add() && pow2NElts > 1)
    {
        emitAddPair(newVar, AddrVar, off);
    }
    else
    {
        m_encoder->SetNoMask();
        m_encoder->SetUniformSIMDSize(simdmode);
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        m_encoder->SetSrcRegion(1, 1, 1, 0);
        if (pow2NElts > 1) {
            m_encoder->Add(newVar, AddrVar, off);
        }
        else {
            m_encoder->Copy(newVar, AddrVar);
        }
        m_encoder->Push();
    }
    return newVar;
}

CVariable* EmitPass::prepareDataForUniform(
    CVariable* DataVar, uint32_t RequiredNElts, e_alignment Align)
{
    uint32_t NElts = DataVar->GetNumberElement();
    uint32_t EltBytes = DataVar->GetElemSize();
    uint32_t pow2NElts = (uint32_t)(uint32_t)PowerOf2Ceil(NElts);
    uint32_t allocNElts = RequiredNElts > 0 ? RequiredNElts : pow2NElts;
    IGC_ASSERT(allocNElts >= pow2NElts && NElts <= 8 && (EltBytes == 4 || EltBytes == 8));
    if (NElts == allocNElts && !DataVar->IsImmediate() && IsGRFAligned(DataVar, Align))
    {
        return DataVar;
    }
    CVariable* newVar = m_currShader->GetNewVariable(allocNElts, DataVar->GetType(), Align, true, CName::NONE);

    // Need to return a var with pow2NElts elements
    if (allocNElts > pow2NElts)
    {
        newVar = m_currShader->GetNewAlias(newVar, newVar->GetType(), 0, pow2NElts);
    }

    // Initialize to DataVar's first element (set Elts from NElts and up to the first element).
    bool initWithElem0 = (pow2NElts > NElts);
    bool needSplit = ((pow2NElts *newVar->GetElemSize()) > (2 * (uint32_t)getGRFSize()));
    if (initWithElem0)
    {
        if (needSplit)
        {
            uint32_t esz = pow2NElts / 2;
            uint32_t bytes = esz * newVar->GetElemSize();
            CVariable* newVarHi = m_currShader->GetNewAlias(newVar, newVar->GetType(), 0, esz);
            CVariable* newVarLo = m_currShader->GetNewAlias(newVar, newVar->GetType(), bytes, esz);

            m_encoder->SetNoMask();
            m_encoder->SetUniformSIMDSize(lanesToSIMDMode(esz));
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->Copy(newVarHi, DataVar);
            m_encoder->Push();

            m_encoder->SetNoMask();
            m_encoder->SetUniformSIMDSize(lanesToSIMDMode(esz));
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->Copy(newVarLo, DataVar);
            m_encoder->Push();
        }
        else
        {
            m_encoder->SetNoMask();
            m_encoder->SetUniformSIMDSize(lanesToSIMDMode(pow2NElts));
            m_encoder->SetSrcRegion(0, 0, 1, 0);
            m_encoder->Copy(newVar, DataVar);
            m_encoder->Push();
        }
    }

    if (!initWithElem0 || NElts != 1)
    {
        emitVectorCopy(newVar, DataVar, NElts);
    }
    return newVar;
}

void EmitPass::emitLSCVectorLoad_subDW(LSC_CACHE_OPTS CacheOpts, bool UseA32,
                                       ResourceDescriptor &Resource,
                                       CVariable *Dest,
                                       CVariable *Offset, int ImmOffset,
                                       int ImmScale, uint32_t NumElts,
                                       uint32_t EltBytes,
                                       LSC_DOC_ADDR_SPACE AddrSpace,
                                       LSC_ADDR_SIZE AddrSize
  ) {
    // NumElts must be 1 !
    IGC_ASSERT(NumElts == 1 && (EltBytes == 1 || EltBytes == 2));

    // No case such as that destUniform is true, but srcUniform is false !
    bool srcUniform = Offset->IsUniform();
    bool resourceUniform = Resource.m_resource->IsUniform();
    bool doUniformLoad = srcUniform && resourceUniform;
    CVariable* eOffset = Offset;

    uint32_t width = numLanes(m_currShader->m_SIMDSize);
    uint16_t nbelts = doUniformLoad ? 1 : width;
    e_alignment addrAlign = UseA32 ? EALIGN_GRF : EALIGN_2GRF;
    uint32_t alloc_nbelts = nbelts;

    // Special handling.
    bool need4GRFAlign = (!UseA32 && doUniformLoad && !m_currShader->m_Platform->hasHalfSIMDLSC());
    if (need4GRFAlign)
    {
        // vISA has problem with A64 SIMD1 non-tranpose load due to alignment issues.
        // Here, we allocate a full payload to make its alignment correct.
        alloc_nbelts = width;

        // Once here, EALIGN_GRF technically should be 4GRF, but it does not matter here as
        // prepareAddressForUniform() ignores it when width > powerOf2(1)!
        eOffset = prepareAddressForUniform(eOffset, 4, 1, alloc_nbelts, EALIGN_GRF);
    }
    else if (doUniformLoad)
    {
        eOffset = ReAlignUniformVariable(eOffset, addrAlign);
    }
    else
    {
        eOffset = BroadcastIfUniform(eOffset);
    }

    // Need a temp as Dest is smaller than DW.
    e_alignment dataAlign = ((4 * alloc_nbelts) <= (uint32_t)getGRFSize()) ? EALIGN_GRF : EALIGN_2GRF;
    CVariable* gatherDst =
        m_currShader->GetNewVariable(alloc_nbelts, ISA_TYPE_UD, dataAlign, doUniformLoad, CName::NONE);
    if (alloc_nbelts > nbelts)
    {
        // This is for need4GRFAlign = true
        gatherDst = m_currShader->GetNewAlias(gatherDst, gatherDst->GetType(), 0, nbelts);
    }

    SamplerDescriptor sampler;
    ResourceLoop(Resource, sampler, [&](CVariable* flag, CVariable*& destination,
        ResourceDescriptor resource, bool needloop) {
        if (doUniformLoad)
            m_encoder->SetNoMask();
        else
            m_encoder->SetPredicate(IGC_IS_FLAG_ENABLED(UseVMaskPredicateForLoads) ? GetCombinedVMaskPred(flag) : flag);

        emitLSCLoad(CacheOpts, gatherDst,
                    eOffset, EltBytes * 8, 1, 0, &Resource, AddrSize,
                    LSC_DATA_ORDER_NONTRANSPOSE, ImmOffset, ImmScale, AddrSpace);
        m_encoder->Push();
        });

    // Copy from gatherDst to original destination
    gatherDst = m_currShader->GetNewAlias(gatherDst, Dest->GetType(), 0, 0);
    uint32_t vStride = doUniformLoad ? 0 : ((EltBytes == 1) ? 4 : 2);
    m_encoder->SetSrcRegion(0, vStride, 1, 0);
    m_encoder->Copy(Dest, gatherDst);
    m_encoder->Push();
    return;
}

// Handle uniform load (address and resource are uniform). This is a sub-function
// of emitLSCVectorLoad(). VectorPreProcess will guarantee the following
// vector size:
//   1. for dataSize-aligned load,  vectorSize is 1|2|3|4|8|16|32|64
//      (max size = UQ x 64 = 512 bytes)
//   2. sub-DW-aligned load,  vectorSize is 1|2|3|4|8
//      (max size = UQ x 8 = 64 bytes)
void EmitPass::emitLSCVectorLoad_uniform(
    LSC_CACHE_OPTS CacheOpts, bool UseA32, ResourceDescriptor &Resource,
    CVariable *Dest,
    CVariable *Offset, int ImmOffset, int ImmScale, uint32_t NumElts,
    uint32_t EltBytes, uint64_t Align, uint32_t Addrspace,
    LSC_DOC_ADDR_SPACE UserAddrSpace, LSC_ADDR_SIZE AddrSize) {

    IGC_ASSERT(Offset->IsUniform() && (EltBytes == 4 || EltBytes == 8));
    CVariable *eOffset = Offset;
    CVariable* ldDest = Dest;
    uint32_t dSize = EltBytes;  // lsc's data size
    uint32_t vSize = NumElts;   // lsc's vector size
    if (dSize == 4 && (vSize > 64 || vSize == 6) && Align >= 8)
    {
        dSize = 8;
        vSize = vSize / 2;
        ldDest = m_currShader->GetNewAlias(ldDest, ISA_TYPE_UQ, 0, 0);
    }
    else if (dSize == 8 && vSize < 64 && Align == 4)
    {
        dSize = 4;
        vSize = vSize * 2;
        ldDest = m_currShader->GetNewAlias(ldDest, ISA_TYPE_UD, 0, 0);
    }

    bool destUniform = Dest->IsUniform();
    IGC_ASSERT((vSize <= 64 && (vSize <= 8 || isPowerOf2_32(vSize))));

    // 1. Do a SIMT1 transposed load
    if ((isPowerOf2_32(vSize) || vSize == 3) && vSize <= 64 &&
        ((Align >= 8 && dSize == 8) || (Align >= 4 && dSize == 4)))
    {
        // vSize = 1|2|3|4|8|16|32|64
        bool needTemp = (!destUniform || !IsGRFAligned(ldDest, EALIGN_GRF));

        eOffset = ReAlignUniformVariable(eOffset, EALIGN_GRF);
        CVariable* tDest = ldDest;
        if (needTemp)
        {
            tDest = m_currShader->GetNewVariable(
                vSize, tDest->GetType(), EALIGN_GRF, true, tDest->getName());
        }

        m_encoder->SetNoMask();
        emitLSCLoad(CacheOpts, tDest,
                    eOffset, dSize * 8, vSize, 0, &Resource, AddrSize,
                    LSC_DATA_ORDER_TRANSPOSE, ImmOffset, ImmScale,
                    UserAddrSpace);
        m_encoder->Push();

        if (needTemp)
        {
            emitVectorCopy(ldDest, tDest, vSize);
        }
        return;
    }

    // 2. Sub-DW aligned. Use non-transpose messages (need one address payload per each
    //    simd lane).  Each lane will load one element of this vector.
    IGC_ASSERT(vSize <= 8);
    e_alignment addrAlign = (UseA32 ? EALIGN_GRF : EALIGN_2GRF);
    e_alignment dataAlign = (dSize == 4 ? EALIGN_GRF : EALIGN_2GRF);
    uint32_t requiredVSize = 0;  // 0 means no special requirement.

    // Special handling.
    bool need4GRFAlign = (!UseA32 && !m_currShader->m_Platform->hasHalfSIMDLSC());
    if (need4GRFAlign)
    {
        // Use non-transpose message, so we extend addresses to simdsize
        uint32_t width = numLanes(m_currShader->m_SIMDSize);
        requiredVSize = width;
    }

    CVariable* nEOff = prepareAddressForUniform(eOffset, dSize, vSize, requiredVSize, addrAlign);
    IGC_ASSERT(vSize <= nEOff->GetNumberElement());
    uint32_t elts = requiredVSize > 0 ? requiredVSize : nEOff->GetNumberElement();
    bool needTemp = !destUniform || !IsGRFAligned(ldDest, dataAlign) || (vSize != elts);
    CVariable* tDest = ldDest;
    if (needTemp)
    {
        tDest = m_currShader->GetNewVariable(elts, tDest->GetType(), dataAlign, true, CName::NONE);
    }

    m_encoder->SetNoMask();
    emitLSCLoad(CacheOpts, tDest,
                nEOff, dSize * 8, 1, 0, &Resource,
                AddrSize,
                LSC_DATA_ORDER_NONTRANSPOSE, ImmOffset, ImmScale,
                UserAddrSpace);
    m_encoder->Push();

    if (needTemp)
    {
        emitVectorCopy(ldDest, tDest, vSize);
    }
    return;
}

void EmitPass::emitLSCVectorLoad(Instruction* inst,
                                 Value *Ptr,
                                 Value *varOffset,
                                 ConstantInt *immOffset,
                                 ConstantInt *immScale,
                                 LSC_CACHE_OPTS cacheOpts,
                                 LSC_DOC_ADDR_SPACE addrSpace
) {

    Type *Ty = inst->getType();
    uint64_t align = 0;
    if (auto LI = dyn_cast<LoadInst>(inst))
        align = IGCLLVM::getAlignmentValue(LI);
    else if (auto CI = dyn_cast<LdRawIntrinsic>(inst))
        align = CI->getAlignment();
    PointerType* ptrType = cast<PointerType>(Ptr->getType());
    ResourceDescriptor resource = GetResourceVariable(Ptr);
    bool useA32 = !IGC::isA64Ptr(ptrType, m_currShader->GetContext());
    LSC_ADDR_SIZE addrSize = useA32 ? LSC_ADDR_SIZE_32b : LSC_ADDR_SIZE_64b;
    IGCLLVM::FixedVectorType* VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
    Type* eltTy = VTy ? VTy->getElementType() : Ty;
    uint32_t eltBytes = GetScalarTypeSizeInRegister(eltTy);
    uint32_t elts = VTy ? int_cast<uint32_t>(VTy->getNumElements()) : 1;

    CountStatelessIndirectAccess(Ptr, resource);
    // eOffset is in bytes
    // offset corresponds to Int2Ptr operand obtained during pattern matching
    CVariable* eOffset = GetSymbol(varOffset);
    if (useA32)
    {
        eOffset = TruncatePointer(eOffset);
    }
    CVariable* destCVar = m_destination;
    bool destUniform = destCVar->IsUniform();
    bool srcUniform = eOffset->IsUniform();

    // Not possible to have uniform dest AND non-uniform src.
    IGC_ASSERT_MESSAGE(!(destUniform && !srcUniform),
                       "Unexpected ld: uniform dest and non-uniform src!");

    const int immOffsetInt =
        immOffset ? static_cast<int>(immOffset->getSExtValue()) : 0;
    const int immScaleInt =
        immScale ? static_cast<int>(immScale->getSExtValue()) : 1;

    // 1. handle cases eltBytes < 4
    if (eltBytes < 4)
    {
        IGC_ASSERT(elts == 1);
      emitLSCVectorLoad_subDW(cacheOpts, useA32, resource, destCVar,
                              eOffset, immOffsetInt, immScaleInt, 1, eltBytes,
                              addrSpace, addrSize);
        return;
    }

    // 2. Handle uniform load
    if (srcUniform && resource.m_resource->IsUniform()) {
        emitLSCVectorLoad_uniform(
            cacheOpts, useA32, resource, destCVar,
            eOffset, immOffsetInt, immScaleInt, elts, eltBytes, align,
            ptrType->getPointerAddressSpace(), addrSpace, addrSize);
        return;
    }

    // 3. generic handling of load (both dest and src are not uniform)
    VISA_Type dVisaTy = destCVar->GetType();
    uint32_t width = numLanes(m_currShader->m_SIMDSize);
    VectorMessage VecMessInfo(this);
    VecMessInfo.getLSCInfo(Ty, align, m_currShader->GetContext(), useA32, false);

    eOffset = BroadcastIfUniform(eOffset);

    ModuleMetaData* modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    auto& predicationMap = modMD->predicationMap;

    SamplerDescriptor sampler;
    ResourceLoop(resource, sampler, [&](CVariable* flag, CVariable*& destination,
        ResourceDescriptor resource, bool needLoop) {
        for (uint32_t i = 0; i < VecMessInfo.numInsts; ++i)
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

            destCVar = destination;
            dVisaTy = destCVar->GetType();

            CVariable* rawAddrVar;
            int currImmOffsetInt = immOffsetInt;
            if (i > 0)
            {
                // Calculate the new element offset
              {
                rawAddrVar = m_currShader->GetNewVariable(eOffset);
                CVariable *ImmVar = m_currShader->ImmToVariable(
                    VecMessInfo.insts[i].startByte, ISA_TYPE_UD);
                if (!useA32 && !m_currShader->m_Platform->hasInt64Add()) {
                  emitAddPair(rawAddrVar, eOffset, ImmVar);
                } else {
                  m_encoder->Add(rawAddrVar, eOffset, ImmVar);
                  m_encoder->Push();
                }
              }
            }
            else
            {
                rawAddrVar = eOffset;
            }

            bool needTemp = (!IsGRFAligned(destCVar, EALIGN_GRF));

            CVariable* gatherDst = nullptr;
            if (needTemp)
            {
                gatherDst = m_currShader->GetNewVariable(
                    (uint16_t)nbelts, dVisaTy, EALIGN_GRF, CName(destCVar->getName(), "Tmp"));
                destination = gatherDst;
            }
            else
            {
                // No need to copy, load directly into destCVar
                gatherDst = m_currShader->GetNewAlias(destCVar,
                    dVisaTy, (uint16_t)eltOffBytes, (uint16_t)nbelts);
            }

            if (predicationMap.count(inst))
            {
                m_encoder->SetPredicate(m_currShader->GetSymbol(cast<Instruction>(predicationMap[inst])));
            }
            else
            {
                m_encoder->SetPredicate(IGC_IS_FLAG_ENABLED(UseVMaskPredicateForLoads) ? GetCombinedVMaskPred(flag) : flag);
            }

            VectorMessage::MESSAGE_KIND messageType = VecMessInfo.insts[i].kind;
            IGC_ASSERT_MESSAGE(
                messageType == VectorMessage::MESSAGE_A32_LSC_RW ||
                messageType == VectorMessage::MESSAGE_A64_LSC_RW,
                "Internal Error: unexpected message kind for load!");

            emitLSCLoad(cacheOpts, gatherDst,
                        rawAddrVar, blkBits, numBlks, 0, &resource,
                        addrSize, LSC_DATA_ORDER_NONTRANSPOSE,
                        currImmOffsetInt, immScaleInt, addrSpace);
            m_encoder->Push();

            if (needTemp)
            {
                emitVectorCopy(destCVar, gatherDst, instElts, eltOff, 0);
            }
        }
    }, m_RLA->GetResourceLoopMarker(inst));
}

// Sub-function of emitLSCVectorStore()
void EmitPass::emitLSCVectorStore_subDW(LSC_CACHE_OPTS CacheOpts, bool UseA32,
                                        ResourceDescriptor &Resource,
                                        CVariable *StoreVar, CVariable *Offset,
                                        int ImmOffset, int ImmScale,
                                        uint32_t NumElts, uint32_t EltBytes,
                                        alignment_t Alignment,
                                        LSC_DOC_ADDR_SPACE AddrSpace,
                                        LSC_ADDR_SIZE AddrSize
) {
    // NumElts must be 1!
    IGC_ASSERT_MESSAGE(NumElts == 1 && (EltBytes == 1 || EltBytes == 2),
        "Number of elements must be 1 for an 8bit or 16bit data type in a non-transposed LSC store.");

    uint32_t width = numLanes(m_currShader->m_SIMDSize);

    // Non-transpose LSC: address is 2GRF aligned; data (DW) is GRF aligned.
    e_alignment addrAlign = UseA32 ? EALIGN_GRF : EALIGN_2GRF;
    e_alignment dataAlign = ((4 * width) <= (uint32_t)getGRFSize()) ? EALIGN_GRF : EALIGN_2GRF;
    CVariable* eOffset = Offset;
    CVariable* stVar = StoreVar;
    bool dstUniform = eOffset->IsUniform();
    bool srcUniform = stVar->IsUniform();

    // We generate a non-transposed LSC store
    bool doUniformStore = dstUniform;
    if (doUniformStore)
    {
        if (!srcUniform)
        {
            stVar = UniformCopy(stVar);
        }

        bool need4GRFAlign = (!UseA32 && !m_currShader->m_Platform->hasHalfSIMDLSC());
        if (need4GRFAlign)
        {
            // Due to the lack of 4GRF alignment in visa, need to have a full payload
            // in order to use non-transpose LSC
            eOffset = prepareAddressForUniform(eOffset, 4, 1, width, EALIGN_GRF /*dontcare*/);
            stVar = ExtendVariable(stVar, EALIGN_DWORD);
            stVar = prepareDataForUniform(stVar, width, dataAlign /*dontcare*/);
        }
        else
        {
            eOffset = ReAlignUniformVariable(eOffset, addrAlign);
            stVar = ExtendVariable(stVar, dataAlign);
        }
    }
    else
    {
        stVar = BroadcastAndExtend(stVar);
    }

    SamplerDescriptor sampler;
    ResourceLoop(Resource, sampler, [&](CVariable* flag, CVariable*& destination,
        ResourceDescriptor resource, bool needLoop) {
        // no need for discard predicate if we are writing to scratch - this is our
        // internal memory, shader output remain the same, but we avoid problems
        // when, for example, texture coordinates are spilled
        if (Resource.m_surfaceType != ESURFACE_SCRATCH)
        {
            setPredicateForDiscard(doUniformStore ? nullptr : flag);
        }

        if (doUniformStore)
        {
            m_encoder->SetNoMask();
        }
      // NumElts = 1
      emitLSCStore(CacheOpts,
                   stVar, eOffset, EltBytes * 8, 1, 0, &Resource, AddrSize,
                   LSC_DATA_ORDER_NONTRANSPOSE, ImmOffset, ImmScale, AddrSpace);
      m_encoder->Push();
    });

    return;
}

// Handle uniform Store (both storeVal and ptr are uniform). This is a
// sub-function of emitLSCVectorStore(). VectorPreProcess will guarantee
// the following vector size:
//   1. for dataSize-aligned store,  vectorSize is 1|2|3|4|8|16|32|64
//      (max size = UQ x 64 = 512 bytes)
//   2. sub-DW-aligned store,  vectorSize is 1|2|3|4|8
//      (max size = UQ x 8 = 64 bytes)
void EmitPass::emitLSCVectorStore_uniform(LSC_CACHE_OPTS CacheOpts, bool UseA32,
                                          ResourceDescriptor &Resource,
                                          CVariable *StoreVar,
                                          CVariable *Offset, int ImmOffset,
                                          int ImmScale, uint32_t NumElts,
                                          uint32_t EltBytes, alignment_t Align,
                                          LSC_DOC_ADDR_SPACE AddrSpace,
                                          LSC_ADDR_SIZE AddrSize
) {
    // If needed, can handle non-uniform StoreVar.
    IGC_ASSERT(StoreVar->IsUniform() && Offset->IsUniform() && (EltBytes == 4 || EltBytes == 8));

    CVariable* eOffset = Offset;
    CVariable* stVar = StoreVar;
    uint32_t dSize = EltBytes;
    uint32_t vSize = NumElts;
    if (dSize == 4 && (vSize > 64 || vSize == 6) && Align >= 8)
    {
        dSize = 8;
        vSize = vSize / 2;
        stVar = m_currShader->GetNewAlias(stVar, ISA_TYPE_UQ, 0, 0);
    }
    else if (dSize == 8 && vSize < 64 && Align == 4)
    {
        dSize = 4;
        vSize = vSize * 2;
        stVar = m_currShader->GetNewAlias(stVar, ISA_TYPE_UD, 0, 0);
    }


    IGC_ASSERT(vSize <= 64 && (vSize < 8 || isPowerOf2_32(vSize)));
    if (Align < 4 || (dSize == 8 && Align < 8) || !(isPowerOf2_32(vSize) || vSize == 3))
    {
        IGC_ASSERT_MESSAGE(vSize <= 8, "unexpected vSize, check vectorPreProcess!");
        // Sub-DW aligned. Use non-transpose messages with execsize = powerof2(vSize).
        e_alignment addrAlign = (UseA32 ? EALIGN_GRF : EALIGN_2GRF);
        e_alignment dataAlign = (dSize == 4 ? EALIGN_GRF : EALIGN_2GRF);
        uint32_t requiredVSize = 0;  // 0 means no special requirement.

        // Special handling.
        bool need4GRFAlign = (!UseA32 && !m_currShader->m_Platform->hasHalfSIMDLSC());
        if (need4GRFAlign)
        {
            // Cannot do simd1 non-transpose store as visa has no 4GRF alignment.
            // Extend addresses instead.
            uint32_t width = numLanes(m_currShader->m_SIMDSize);
            requiredVSize = width;
        }

        CVariable* new_eoff = prepareAddressForUniform(eOffset, dSize, vSize, requiredVSize, addrAlign);
        CVariable* new_stVar = prepareDataForUniform(stVar, requiredVSize, dataAlign);

        SamplerDescriptor sampler;
        ResourceLoop(Resource, sampler, [&](CVariable* /*flag*/, CVariable*& /*destination*/,
            ResourceDescriptor /*resource*/, bool /*needLoop*/) {
            m_encoder->SetNoMask();
          emitLSCStore(CacheOpts,
                       new_stVar, new_eoff, dSize * 8, 1, 0, &Resource,
                       AddrSize, LSC_DATA_ORDER_NONTRANSPOSE, ImmOffset,
                       ImmScale, AddrSpace);
          m_encoder->Push();
        });
        return;
    }

    // Use SIMT1 transpose message (GRF align for both addr/data payload is good enough)
    if (stVar->IsImmediate() || !IsGRFAligned(stVar, EALIGN_GRF))
    {
        CVariable* tempVar = m_currShader->GetNewVariable(
            vSize, stVar->GetType(), EALIGN_GRF, true, CName(stVar->getName(), "Tmp"));
        emitVectorCopy(tempVar, stVar, vSize);
        stVar = tempVar;
    }
    eOffset = ReAlignUniformVariable(eOffset, EALIGN_GRF);

    SamplerDescriptor sampler;
    ResourceLoop(Resource, sampler, [&](CVariable* /*flag*/, CVariable*& /*destination*/,
        ResourceDescriptor /*resource*/, bool /*needLoop*/) {
        m_encoder->SetUniformSIMDSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
        emitLSCStore(CacheOpts,
                     stVar, eOffset, dSize * 8, vSize, 0, &Resource, AddrSize,
                     LSC_DATA_ORDER_TRANSPOSE, ImmOffset, ImmScale, AddrSpace);
        m_encoder->Push();
    });
    return;
}

void EmitPass::emitLSCVectorStore(Value *Ptr,
                                  Value *varOffset, ConstantInt *immOffset,
                                  ConstantInt *immScale, Value *storedVal,
                                  BasicBlock *BB, LSC_CACHE_OPTS cacheOpts,
                                  alignment_t align, bool dontForceDmask,
                                  LSC_DOC_ADDR_SPACE addrSpace

) {

    PointerType* ptrType = cast<PointerType>(Ptr->getType());
    Type* Ty = storedVal->getType();
    IGCLLVM::FixedVectorType* VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
    Type* eltTy = VTy ? VTy->getElementType() : Ty;
    uint32_t eltBytes = GetScalarTypeSizeInRegister(eltTy);
    uint32_t elts = VTy ? int_cast<uint32_t>(VTy->getNumElements()) : 1;

    unsigned int width = numLanes(m_currShader->m_SIMDSize);

    ResourceDescriptor resource = GetResourceVariable(Ptr);
    CountStatelessIndirectAccess(Ptr, resource);
    if (ptrType->getPointerAddressSpace() != ADDRESS_SPACE_PRIVATE &&
        !dontForceDmask)
    {
        ForceDMask(false);
    }

    // offset corresponds to Int2Ptr operand obtained during pattern matching
    CVariable* eOffset = GetSymbol(varOffset);
    bool useA32 = !isA64Ptr(ptrType, m_currShader->GetContext());
    LSC_ADDR_SIZE addrSize = useA32 ? LSC_ADDR_SIZE_32b : LSC_ADDR_SIZE_64b;
    if (useA32)
    {
        eOffset = TruncatePointer(eOffset);
    }

    // todo: simplilfy close-tie b/w instances and uniform store.
    // For now, make sure simd instance setting is consistent with store here,
    // that is, must do uniform store if isUniformStoreOCL() returns true
    auto checkOCLUniformStore = [&](bool doUniformStore, bool doUniformSimdInstance) {
        if (m_currShader->GetShaderType() != ShaderType::OPENCL_SHADER) return;
        IGC_ASSERT((doUniformStore && doUniformSimdInstance) ||
                   (!doUniformStore && !doUniformSimdInstance));
        return;
    };

    // when true, one simd instance is generated only!
    bool isOCLUniform = isUniformStoreOCL(varOffset, storedVal);

    CVariable* storedVar = tryReusingConstVectorStoreData(storedVal, BB, false);
    if (!storedVar) storedVar = GetSymbol(storedVal);

    // In case eOffset isn't GRF aligned, need to create a copy
    // For non-uniform variable, it should be already GRF-aligned.
    eOffset = ReAlignUniformVariable(eOffset, EALIGN_GRF);
    bool srcUniform = storedVar->IsUniform();
    bool dstUniform = eOffset->IsUniform();

    const int immOffsetVal =
        immOffset ? int_cast<int>(immOffset->getSExtValue()) : 0;
    const int immScaleVal =
        immScale ? int_cast<int>(immScale->getSExtValue()) : 1;

    // 1. handle cases eltBytes < 4
    if (eltBytes < 4)
    {
        checkOCLUniformStore(dstUniform, isOCLUniform);

        IGC_ASSERT(elts == 1);
        emitLSCVectorStore_subDW(cacheOpts, useA32, resource,
                                 storedVar, eOffset, immOffsetVal, immScaleVal,
                                 1, eltBytes, align, addrSpace, addrSize);
        return;
    }

    checkOCLUniformStore(dstUniform && srcUniform, isOCLUniform);

    // 2. Handle uniform Store
    if (dstUniform && srcUniform)
    {
        emitLSCVectorStore_uniform(cacheOpts, useA32, resource,
                                   storedVar, eOffset, immOffsetVal,
                                   immScaleVal, elts, eltBytes, align,
                                   addrSpace, addrSize);
        return;
    }

    VectorMessage VecMessInfo(this);
    VecMessInfo.getLSCInfo(Ty, align, m_currShader->GetContext(), useA32, false);

    eOffset = BroadcastIfUniform(eOffset);

    CVariable* broadcastedVar = tryReusingConstVectorStoreData(storedVal, BB, true);
    storedVar = broadcastedVar ? broadcastedVar : BroadcastIfUniform(storedVar);

    SamplerDescriptor sampler;
    ResourceLoop(resource, sampler, [&](CVariable* flag, CVariable*& destination,
        ResourceDescriptor resource, bool needLoop) {
        for (uint32_t i = 0; i < VecMessInfo.numInsts; ++i)
        {
            // raw operand, eltOff is in bytes
            uint32_t eltOffBytes = VecMessInfo.insts[i].startByte * width;
            uint32_t blkInBytes = VecMessInfo.insts[i].blkInBytes;
            uint32_t numBlks = VecMessInfo.insts[i].numBlks;
            uint32_t blkBits = 8 * blkInBytes;
            uint32_t instTotalBytes = blkInBytes * numBlks;
            uint32_t instElts = instTotalBytes / eltBytes;
            uint32_t nbelts = instElts * width;

            CVariable* rawAddrVar;
            int currImmOffsetVal = immOffsetVal;
            if (i > 0)
            {
                // Calculate the new element offset
              {
                rawAddrVar = m_currShader->GetNewVariable(eOffset);
                CVariable *ImmVar = m_currShader->ImmToVariable(
                    VecMessInfo.insts[i].startByte, ISA_TYPE_UD);
                if (!useA32 && !m_currShader->m_Platform->hasInt64Add()) {
                  emitAddPair(rawAddrVar, eOffset, ImmVar);
                } else {
                  m_encoder->Add(rawAddrVar, eOffset, ImmVar);
                  m_encoder->Push();
                }
              }
            }
            else
            {
                rawAddrVar = eOffset;
            }
            // no need for discard predicate if we are writing to internal memory surfaces
            // those include scratch or raytracing-related allocations
            if (resource.m_surfaceType != ESURFACE_SCRATCH && !dontForceDmask)
            {
                setPredicateForDiscard(flag);
            }

            VISA_Type storedType = storedVar->GetType();
            IGC_ASSERT_MESSAGE(eltOffBytes < (UINT16_MAX), "eltOffBytes > higher than 64k");
            IGC_ASSERT_MESSAGE(nbelts < (UINT16_MAX), "nbelts > higher than 64k");
            CVariable* subStoredVar = m_currShader->GetNewAlias(
                storedVar, storedType, (uint16_t)eltOffBytes, (uint16_t)nbelts);

            VectorMessage::MESSAGE_KIND messageType = VecMessInfo.insts[i].kind;
            IGC_ASSERT_MESSAGE(
                messageType == VectorMessage::MESSAGE_A32_LSC_RW ||
                    messageType == VectorMessage::MESSAGE_A64_LSC_RW,
                "Internal Error: unexpected message kind for load!");

            emitLSCStore(cacheOpts,
                         subStoredVar, rawAddrVar, blkBits, numBlks, 0,
                         &resource, addrSize, LSC_DATA_ORDER_NONTRANSPOSE,
                         currImmOffsetVal, immScaleVal, addrSpace);

            m_encoder->Push();
        }
    });

    if (ptrType->getPointerAddressSpace() != ADDRESS_SPACE_PRIVATE &&
        !dontForceDmask)
    {
        ResetVMask(false);
    }
}

void EmitPass::emitLSCTypedRead(llvm::Instruction* pInsn)
{
    const CShader::ExtractMaskWrapper writeMask(m_currShader, pInsn);
    IGC_ASSERT_MESSAGE(writeMask.hasEM() && writeMask.getEM() != 0, "Wrong write mask");

    llvm::Value* pllSrcBuffer = pInsn->getOperand(0);
    llvm::Value* pllU = pInsn->getOperand(1);
    llvm::Value* pllV = pInsn->getOperand(2);
    llvm::Value* pllR = pInsn->getOperand(3);
    llvm::Value* pllLOD = getOperandIfExist(pInsn, 4);

    CVariable* pLOD = isUndefOrConstInt0(pllLOD) ? nullptr : GetSymbol(pllLOD);
    CVariable* pR = (pLOD == nullptr && isUndefOrConstInt0(pllR)) ? nullptr : GetSymbol(pllR);
    CVariable* pV = (pR == nullptr && isUndefOrConstInt0(pllV)) ? nullptr : GetSymbol(pllV);
    CVariable* pU = GetSymbol(pllU);

    pU = BroadcastIfUniform(pU, m_currShader->GetIsUniform(pInsn));
    pV = pV ? BroadcastIfUniform(pV, m_currShader->GetIsUniform(pInsn)) : nullptr;
    pR = pR ? BroadcastIfUniform(pR, m_currShader->GetIsUniform(pInsn)) : nullptr;
    pLOD = pLOD ? BroadcastIfUniform(pLOD, m_currShader->GetIsUniform(pInsn)) : nullptr;

    ResourceDescriptor resource = GetResourceVariable(pllSrcBuffer);

    LSC_CACHE_OPTS cacheOpts = translateLSCCacheControlsFromMetadata(pInsn, true, true);

    uint numChannels = iSTD::BitCount(writeMask.getEM());
    auto eltBitSize = 4 * 8;
    LSC_ADDR_SIZE addrSize = LSC_ADDR_SIZE_32b;
    if (m_currShader->GetIsUniform(pInsn))
    {
        SIMDMode nativeDispatchMode = m_currShader->m_Platform->getMinDispatchMode();
        CVariable* tempdst = nullptr;
        tempdst = m_currShader->GetNewVariable(
            numChannels * numLanes(nativeDispatchMode),
            ISA_TYPE_F,
            EALIGN_GRF,
            CName("tyReadDest"));
        m_encoder->SetSimdSize(nativeDispatchMode);
        m_encoder->SetPredicate(nullptr);
        m_encoder->SetNoMask();
        m_encoder->LSC_TypedReadWrite(LSC_LOAD_QUAD, &resource, pU, pV, pR, pLOD, tempdst, eltBitSize,
            numLanes(nativeDispatchMode), addrSize, writeMask.getEM(), cacheOpts);
        m_encoder->Push();

        // Mov the required channel values to m_destination
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();

        for (uint i = 0; i < numChannels; ++i)
        {
            m_encoder->SetSrcSubReg(0, i * numLanes(nativeDispatchMode));
            m_encoder->SetDstSubReg(i);
            m_encoder->Copy(m_destination, tempdst);
            m_encoder->Push();
        }
    }
    else
    {
        uint label = 0;
        CVariable* flag = nullptr;
        bool needLoop = ResourceLoopHeader(resource, flag, label);
        ResourceLoopSubIteration(resource, flag, label);
        CVariable* tempdst[4] = { nullptr, nullptr, nullptr, nullptr };
        auto instWidth = m_currShader->m_Platform->getMaxLSCTypedMessageSize();
        bool needsSplit = m_currShader->m_SIMDSize > instWidth;

        if (!needsSplit)
        {
            m_encoder->SetPredicate(flag);
            m_encoder->LSC_TypedReadWrite(LSC_LOAD_QUAD, &resource, pU, pV, pR, pLOD, m_destination, eltBitSize,
                numLanes(instWidth), addrSize, writeMask.getEM(), cacheOpts);
            m_encoder->Push();
        }
        else
        {
            const unsigned int numLanesForInstWidth = numLanes(instWidth);
            IGC_ASSERT(numLanesForInstWidth);
            const unsigned int splitInstCount = numLanes(m_currShader->m_SIMDSize) / numLanesForInstWidth;

            for (uint i = 0; i < splitInstCount; ++i)
            {
                tempdst[i] = m_currShader->GetNewVariable(
                    numChannels * numLanes(instWidth),
                    ISA_TYPE_F,
                    EALIGN_GRF,
                    CName::NONE);

                setSIMDSizeMask(m_encoder, m_currShader, i);
                m_encoder->SetSrcSubVar(0, i);
                m_encoder->SetSrcSubVar(1, i);
                m_encoder->SetSrcSubVar(2, i);
                m_encoder->SetSrcSubVar(3, i);
                m_encoder->SetPredicate(flag);
                m_encoder->LSC_TypedReadWrite(LSC_LOAD_QUAD, &resource, pU, pV, pR, pLOD, tempdst[i], eltBitSize,
                    numLanes(instWidth), addrSize, writeMask.getEM(), cacheOpts);
                m_encoder->Push();
            }
        }
        ResourceLoopBackEdge(needLoop, flag, label);

        if (m_currShader->m_SIMDSize != instWidth)
        {
            JoinSIMD(tempdst, numChannels, instWidth);
        }
    }
    m_currShader->m_State.isMessageTargetDataCacheDataPort = true;
}

void EmitPass::emitLSCTypedWrite(llvm::Instruction* pInsn)
{
    ForceDMask();
    llvm::Value* pllDstBuffer = pInsn->getOperand(0);
    llvm::Value* pllU = pInsn->getOperand(1);
    llvm::Value* pllV = pInsn->getOperand(2);
    llvm::Value* pllR = pInsn->getOperand(3);
    llvm::Value* pllLOD = pInsn->getOperand(4);
    llvm::Value* pllSrc_X = pInsn->getOperand(5);
    llvm::Value* pllSrc_Y = pInsn->getOperand(6);
    llvm::Value* pllSrc_Z = pInsn->getOperand(7);
    llvm::Value* pllSrc_W = pInsn->getOperand(8);

    CVariable* pLOD = isUndefOrConstInt0(pllLOD) ? nullptr : GetSymbol(pllLOD);
    CVariable* pR = (pLOD == nullptr && isUndefOrConstInt0(pllR)) ? nullptr : GetSymbol(pllR);
    CVariable* pV = (pR == nullptr && isUndefOrConstInt0(pllV)) ? nullptr : GetSymbol(pllV);
    CVariable* pU = GetSymbol(pllU);

    CVariable* pSrc_X = GetSymbol(pllSrc_X);
    CVariable* pSrc_Y = GetSymbol(pllSrc_Y);
    CVariable* pSrc_Z = GetSymbol(pllSrc_Z);
    CVariable* pSrc_W = GetSymbol(pllSrc_W);

    pU = BroadcastIfUniform(pU);
    pV = pV ? BroadcastIfUniform(pV) : nullptr;
    pR = pR ? BroadcastIfUniform(pR) : nullptr;
    pLOD = pLOD ? BroadcastIfUniform(pLOD) : nullptr;

    ResourceDescriptor resource = GetResourceVariable(pllDstBuffer);
    LSC_ADDR_SIZE addrSize = LSC_ADDR_SIZE_32b;
    LSC_CACHE_OPTS cacheOpts = translateLSCCacheControlsFromMetadata(pInsn, false, true);
    m_currShader->m_State.HasLscStoreCacheControls(cacheOpts);

    if (m_currShader->GetIsUniform(pInsn))
    {
        IGC_ASSERT_MESSAGE(0, "Uniform store_uav_typed not implemented yet");
        return;
    }

    uint label = 0;
    CVariable* flag = nullptr;
    bool needLoop = ResourceLoopHeader(resource, flag, label);
    ResourceLoopSubIteration(resource, flag, label);
    uint parameterLength = 4;

    bool needsSplit = m_currShader->m_SIMDSize > m_currShader->m_Platform->getMaxLSCTypedMessageSize();
    auto eltBitSize = 4 * 8;
    // exec size for each lsc store
    auto instWidth = std::min(m_currShader->m_SIMDSize, m_currShader->m_Platform->getMaxLSCTypedMessageSize());

    if (!needsSplit)
    {
        const unsigned int GRFSizeBy4 = (getGRFSize() / 4);
        IGC_ASSERT(GRFSizeBy4);
        const unsigned int numEltGRF = numLanes(instWidth) / GRFSizeBy4;

        Constant* p_X = dyn_cast<Constant>(pllSrc_X);
        Constant* p_Y = dyn_cast<Constant>(pllSrc_Y);
        Constant* p_Z = dyn_cast<Constant>(pllSrc_Z);
        Constant* p_W = dyn_cast<Constant>(pllSrc_W);

        CVariable* pPayload = nullptr;
        if (p_X && p_Y && p_Z && p_W) {
          // if x, y, z, w are constants, find opportunities to reuse
          std::vector<Constant*> xyzwPayload{ p_X, p_Y, p_Z, p_W };
          llvm::Constant* payloadConstant = llvm::ConstantVector::get(xyzwPayload);
          pPayload = tryReusingXYZWPayload(payloadConstant, pInsn->getParent(),
                parameterLength * numLanes(m_currShader->m_SIMDSize), ISA_TYPE_F,
                pSrc_X, pSrc_Y, pSrc_Z, pSrc_W, numEltGRF);
        }

        if (!pPayload) {
          // create payload variable
          pPayload = m_currShader->GetNewVariable(
                      parameterLength * numLanes(m_currShader->m_SIMDSize),
                      ISA_TYPE_F,
                      EALIGN_GRF,
                      CName::NONE);

          m_currShader->CopyVariable(pPayload, pSrc_X, 0);
          m_currShader->CopyVariable(pPayload, pSrc_Y, numEltGRF);
          m_currShader->CopyVariable(pPayload, pSrc_Z, 2 * numEltGRF);
          m_currShader->CopyVariable(pPayload, pSrc_W, 3 * numEltGRF);
        }

        m_encoder->SetPredicate(flag);
        m_encoder->LSC_TypedReadWrite(LSC_STORE_QUAD, &resource, pU, pV, pR, pLOD, pPayload, eltBitSize, parameterLength, addrSize, 0xF, cacheOpts);
        m_encoder->Push();
    }
    else
    {
        for (uint i = 0; i < 2; ++i)
        {
            CVariable* pPayload = nullptr;
            pPayload = m_currShader->GetNewVariable(
                parameterLength * numLanes(instWidth),
                ISA_TYPE_F,
                EALIGN_GRF, CName::NONE);
            setSIMDSizeMask(m_encoder, m_currShader, i);
            if (!pSrc_X->IsUniform())
            {
                m_encoder->SetSrcSubVar(0, i);
            }
            m_encoder->SetDstSubVar(0);
            m_encoder->Copy(pPayload, pSrc_X);
            m_encoder->Push();

            setSIMDSizeMask(m_encoder, m_currShader, i);
            if (!pSrc_Y->IsUniform())
            {
                m_encoder->SetSrcSubVar(0, i);
            }
            m_encoder->SetDstSubVar(1);
            m_encoder->Copy(pPayload, pSrc_Y);
            m_encoder->Push();

            setSIMDSizeMask(m_encoder, m_currShader, i);
            if (!pSrc_Z->IsUniform())
            {
                m_encoder->SetSrcSubVar(0, i);
            }
            m_encoder->SetDstSubVar(2);
            m_encoder->Copy(pPayload, pSrc_Z);
            m_encoder->Push();

            setSIMDSizeMask(m_encoder, m_currShader, i);
            if (!pSrc_W->IsUniform())
            {
                m_encoder->SetSrcSubVar(0, i);
            }
            m_encoder->SetDstSubVar(3);
            m_encoder->Copy(pPayload, pSrc_W);
            m_encoder->Push();

            setSIMDSizeMask(m_encoder, m_currShader, i);
            m_encoder->SetSrcSubVar(0, i);
            m_encoder->SetSrcSubVar(1, i);
            m_encoder->SetSrcSubVar(2, i);
            m_encoder->SetSrcSubVar(3, i);
            m_encoder->SetPredicate(flag);
            m_encoder->LSC_TypedReadWrite(LSC_STORE_QUAD, &resource, pU, pV, pR, pLOD, pPayload, eltBitSize, parameterLength, addrSize, 0xF, cacheOpts);
            m_encoder->Push();
        }
    }
    ResourceLoopBackEdge(needLoop, flag, label);

    ResetVMask();
    m_currShader->m_State.isMessageTargetDataCacheDataPort = true;
}


tuple<CVariable*, CVariable*, CVariable*> EmitPass::addToCachedPayloadUVR(CVariable* pU, CVariable* pV, CVariable* pR)
{
    // call this function when the uvr tuple is not seen before.
    CVariable* uAlias = nullptr;
    CVariable* vAlias = nullptr;
    CVariable* rAlias = nullptr;

    int dim = pR ? 3 : (pV ? 2 : 1);

    // create a space with the size of the payload.
    CVariable* UVRpayload = m_currShader->GetNewVariable(
        (getGRFSize() * (dim - 1) + m_destination->GetElemSize()) / m_destination->GetElemSize(),
        pU->GetType(), EALIGN_32WORD, true, CName::NONE);

    // copy U into this UVRpayload and create a uAlias with size 1 since the functions we are calling only take single Variable.
    // The aliasing to UVRpayload can avoid extra movs for creating payload into continuous registers in vISA
    m_encoder->SetSimdSize(SIMDMode::SIMD1);
    m_encoder->SetDstSubVar(0);
    m_encoder->Cast(UVRpayload, pU);
    m_encoder->Push();
    uAlias = m_currShader->GetNewAlias(UVRpayload, pU->GetType(), 0, 0);

    if (pV)
    {
        // handle pV if it exists
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetDstSubVar(1);
        m_encoder->Cast(UVRpayload, pV);
        m_encoder->Push();
        vAlias = m_currShader->GetNewAlias(UVRpayload, pV->GetType(), getGRFSize(), 0);
    }
    if (pR)
    {
        // handle pR if it exists
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetDstSubVar(2);
        m_encoder->Cast(UVRpayload, pR);
        m_encoder->Push();
        rAlias = m_currShader->GetNewAlias(UVRpayload, pR->GetType(), getGRFSize() * 2, 0);
    }

    return make_tuple(uAlias, vAlias, rAlias);
}

void EmitPass::emitLSCAtomicTyped(llvm::GenIntrinsicInst* inst)
{
    ForceDMask();
    // Currently, Dword Atomics can be called by matching 2 intrinsics. One is the DwordAtomicRaw
    // and AtomicCmpXchg (which has 2 srcs unlike the other atomics).
    IGC_ASSERT(IGCLLVM::getNumArgOperands(inst) == 6);

    /// Immediate Atomics return the value before the atomic operation is performed. So that flag
    /// needs to be set for this.
    bool returnsImmValue = !inst->user_empty();

    llvm::Value* pllbuffer = inst->getOperand(0);
    llvm::Value* pllU = inst->getOperand(1);
    llvm::Value* pllV = inst->getOperand(2);
    llvm::Value* pllR = inst->getOperand(3);
    llvm::Value* pllSrc0 = inst->getOperand(4);

    CVariable* pSrc0 = nullptr;
    CVariable* pSrc1 = nullptr;
    AtomicOp atomic_op = EATOMIC_UNDEF;
    if (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_icmpxchgatomictyped ||
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_fcmpxchgatomictyped)
    {
        llvm::Value* pllSrc1 = inst->getOperand(5);
        pSrc1 = GetSymbol(pllSrc1);
        pSrc1 = UnpackOrBroadcastIfUniform(pSrc1);
        atomic_op = EATOMIC_CMPXCHG;
        atomic_op = inst->getIntrinsicID() == GenISAIntrinsic::GenISA_icmpxchgatomictyped ? EATOMIC_CMPXCHG : EATOMIC_FCMPWR;
    }
    else
    {
        atomic_op = static_cast<AtomicOp>(cast<ConstantInt>(inst->getOperand(5))->getZExtValue());
    }

    const bool hasNoDataOperand =
        atomic_op == EATOMIC_INC64 || atomic_op == EATOMIC_DEC64 ||
        atomic_op == EATOMIC_INC || atomic_op == EATOMIC_DEC;
    if (hasNoDataOperand == false)
    {
        pSrc0 = GetSymbol(pllSrc0);
    }

    ResourceDescriptor resource = GetResourceVariable(pllbuffer);

    CVariable* pR = isUndefOrConstInt0(pllR) ? nullptr : GetSymbol(pllR);
    CVariable* pV = (pR == nullptr && isUndefOrConstInt0(pllV)) ? nullptr : GetSymbol(pllV);
    CVariable* pU = GetSymbol(pllU);
    auto eltSizeInBits = inst->getType()->getScalarSizeInBits();

    if (IsUniformAtomic(inst))
    {
        // check if the UVR payload is reused. If so get the CVariable from cached tuple
        // this can reduce the mov instructions for creating UVR payload.
        tuple<CVariable*, CVariable*, CVariable*> cachedCVar;
        tuple<Value*, Value*, Value*> uvr = make_tuple(pllU, pllV, pllR);
        if (auto It = atomic_shared_pUVR.find(uvr); It != atomic_shared_pUVR.end())
        {
            cachedCVar = It->second;
        }
        else
        {
            pU = ReAlignUniformVariable(pU, EALIGN_GRF);
            pV = pV ? ReAlignUniformVariable(pV, EALIGN_GRF) : pV;
            pR = pR ? ReAlignUniformVariable(pR, EALIGN_GRF) : pR;
            cachedCVar = addToCachedPayloadUVR(pU, pV, pR);
            atomic_shared_pUVR[uvr] = cachedCVar;
        }

        if (atomic_op == EATOMIC_OR && OrWith0Atomic(inst, 4))
        {
            // special case of atomic_load
            emitScalarAtomicLoad(inst, resource,
                                 nullptr /*pDstAddr*/, get<0>(cachedCVar), get<1>(cachedCVar), get<2>(cachedCVar), pSrc0,
                                 false /*isA64*/, eltSizeInBits, 0, 1,
                                 LSC_ADDR_SIZE_32b);
        }
        else
        {
            emitScalarAtomics(inst, resource, atomic_op,
                              nullptr /*pDstAddr*/, get<0>(cachedCVar), get<1>(cachedCVar), get<2>(cachedCVar), pSrc0,
                              false /*isA64*/, eltSizeInBits, 0, 1,
                              LSC_ADDR_SIZE_32b);
        }
    }
    else
    {
        pU = BroadcastIfUniform(pU);
        pV = pV ? BroadcastIfUniform(pV) : nullptr;
        pR = pR ? BroadcastIfUniform(pR) : nullptr;

        if (pSrc0)
        {
            pSrc0 = UnpackOrBroadcastIfUniform(pSrc0);
        }

        uint label = 0;
        CVariable* flag = nullptr;
        bool needLoop = ResourceLoopHeader(resource, flag, label);
        ResourceLoopSubIteration(resource, flag, label);
        CVariable* tempdst[4] = { nullptr, nullptr, nullptr, nullptr };
        auto instWidth = m_currShader->m_Platform->getMaxLSCTypedMessageSize();
        bool needsSplit = m_currShader->m_SIMDSize > instWidth;
        uint regsPerAddrChannel = 32 * numLanes(instWidth) / 8 / getGRFSize();
        uint regsPerDataChannel = eltSizeInBits * numLanes(instWidth) / 8 / getGRFSize();
        LSC_ADDR_SIZE addrSize = LSC_ADDR_SIZE_32b;
        auto cacheOpts = LSC_DEFAULT_CACHING;
        if (!needsSplit)
        {
            m_encoder->SetPredicate(flag);
            m_encoder->LSC_TypedAtomic(atomic_op, &resource, pU, pV, pR, pSrc0, pSrc1, returnsImmValue ? m_destination : nullptr, eltSizeInBits, addrSize, cacheOpts);
            m_encoder->Push();
        }
        else
        {
            const unsigned int numLanesForInstWidth = numLanes(instWidth);
            IGC_ASSERT(numLanesForInstWidth);
            const unsigned int splitInstCount = numLanes(m_currShader->m_SIMDSize) / numLanesForInstWidth;

            for (uint i = 0; i < splitInstCount; ++i)
            {
                tempdst[i] = returnsImmValue ? m_currShader->GetNewVariable(
                    numLanes(instWidth),
                    eltSizeInBits != 64 ? ISA_TYPE_UD : ISA_TYPE_UQ,
                    EALIGN_GRF,
                    CName::NONE) : nullptr;

                m_encoder->SetPredicate(flag);
                setSIMDSizeMask(m_encoder, m_currShader, i);
                m_encoder->SetSrcSubVar(0, i * regsPerAddrChannel);
                m_encoder->SetSrcSubVar(1, i * regsPerDataChannel);
                m_encoder->LSC_TypedAtomic(atomic_op, &resource, pU, pV, pR, pSrc0, pSrc1, tempdst[i], eltSizeInBits, addrSize, cacheOpts);
                m_encoder->Push();
            }
        }

        ResourceLoopBackEdge(needLoop, flag, label);

        if (returnsImmValue && m_currShader->m_SIMDSize != instWidth)
        {
            JoinSIMD(tempdst, regsPerDataChannel, instWidth);
        }
    }
    ResetVMask();
    m_currShader->m_State.isMessageTargetDataCacheDataPort = true;
}

void EmitPass::emitLscUniformAtomicCounter(llvm::GenIntrinsicInst* pInst)
{
    ForceDMask();
    IGC_ASSERT(pInst->getNumOperands() == 2);

    GenISAIntrinsic::ID IID = pInst->getIntrinsicID();

    llvm::Value* pllbuffer = pInst->getOperand(0);
    ResourceDescriptor resource = GetResourceVariable(pllbuffer);

    // It is more efficient to emit a SIMD1 atomic
    CVariable* prefixVar[2] = { nullptr, nullptr };
    CVariable* src = m_currShader->ImmToVariable(1, ISA_TYPE_D);
    emitPreOrPostFixOp(EOPCODE_ADD, 0, ISA_TYPE_D, false, src, prefixVar);
    CVariable* pSrcCopy = prefixVar[0];
    if (m_currShader->m_numberInstance == 2)
    {
        pSrcCopy = prefixVar[1];
    }

    // prepare data payload
    CVariable* pPayload = m_currShader->GetNewVariable(1, ISA_TYPE_D, EALIGN_GRF,true, CName::NONE);
    m_encoder->SetSimdSize(SIMDMode::SIMD1);
    m_encoder->SetSrcRegion(0, 0, 1, 0);
    m_encoder->SetSrcSubReg(0, numLanes(m_currShader->m_SIMDSize) - 1);
    m_encoder->Copy(pPayload, pSrcCopy);
    m_encoder->Push();

    // prepare dst if requires return value
    CVariable* dst = nullptr;
    bool returnsImmValue = !pInst->user_empty();
    if (returnsImmValue)
    {
        dst = m_currShader->GetNewVariable(1, ISA_TYPE_D, EALIGN_GRF, true, CName::NONE);
    }

    m_encoder->SetSimdSize(SIMDMode::SIMD1);
    m_encoder->SetNoMask();
    m_encoder->LSC_UntypedAppendCounterAtomic(
        IID == GenISAIntrinsic::GenISA_atomiccounterinc ? LSC_APNDCTR_ATOMIC_ADD : LSC_APNDCTR_ATOMIC_SUB,
        &resource,
        dst,
        pPayload);
    m_encoder->Push();

    if (returnsImmValue)
    {
        unsigned int counter = m_currShader->m_numberInstance;
        for (unsigned int i = 0; i < counter; ++i)
        {
            m_encoder->SetSecondHalf(i == 1);

            if (IID == GenISAIntrinsic::GenISA_atomiccounterinc)
            {
                m_encoder->Add(m_destination, prefixVar[i], dst);
                m_encoder->Push();

                CVariable* src = m_currShader->ImmToVariable(-1, ISA_TYPE_D);
                m_encoder->Add(m_destination, m_destination, src);
                m_encoder->Push();
            }
            else if (IID == GenISAIntrinsic::GenISA_atomiccounterpredec)
            {
                m_encoder->SetSrcModifier(1, EMOD_NEG);
                m_encoder->Add(m_destination, dst, prefixVar[i]);
                m_encoder->Push();
            }
        }
    }

    ResetVMask();
    m_currShader->m_State.isMessageTargetDataCacheDataPort = true;
}

// DstSubRegOffset and SrcSubRegOffset are in unit of element size.
void EmitPass::emitUniformVectorCopy(CVariable* Dst, CVariable* Src, uint32_t nElts,
    uint32_t DstSubRegOffset, uint32_t SrcSubRegOffset, bool allowLargerSIMDSize)
{
    IGC_ASSERT(Dst->IsUniform() && Src->IsUniform());

    uint32_t width = numLanes(m_currShader->m_SIMDSize);
    unsigned doff = DstSubRegOffset, soff = SrcSubRegOffset;

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

    // allowLargerSIMDSize: True, allow simd32/simd16; otherwise, no simd32/simd16.

    // Start with the max execution size that is legal for the vector element
    // and is no greater than the current simdsize.
    uint32_t maxNumElts = (2 * getGRFSize()) / Dst->GetElemSize();
    uint32_t maxSimd = std::min(width, (uint32_t)PowerOf2Floor(maxNumElts));
    if (maxSimd == 32 && allowLargerSIMDSize) {
        while (partialCopy(SIMDMode::SIMD32))
            ;
        partialCopy(SIMDMode::SIMD16);
        partialCopy(SIMDMode::SIMD8);
    }
    else if (maxSimd == 16 && allowLargerSIMDSize) {
        while (partialCopy(SIMDMode::SIMD16))
            ;
        partialCopy(SIMDMode::SIMD8);
    }
    else {
        while (partialCopy(SIMDMode::SIMD8))
            ;
    }
    partialCopy(SIMDMode::SIMD4);
    partialCopy(SIMDMode::SIMD2);
    partialCopy(SIMDMode::SIMD1);
}

void EmitPass::emitPredicatedVectorCopy(CVariable* Dst, CVariable* Src, CVariable* pred)
{
    unsigned int width = numLanes(m_currShader->m_SIMDSize);
    unsigned doff = 0, soff = 0;

    IGC_ASSERT(IsGRFAligned(Dst, EALIGN_GRF));
    IGC_ASSERT(IsGRFAligned(Src, EALIGN_GRF));

    uint32_t nElts = Src->GetSize() / getGRFSize();

    for (uint32_t i = 0; i < nElts; ++i)
    {
        uint SrcSubReg = soff + width * i;
        uint DstSubReg = doff + width * i;

        m_encoder->SetPredicate(pred);
        m_encoder->SetSrcSubReg(0, SrcSubReg);
        m_encoder->SetDstSubReg(DstSubReg);
        m_encoder->Copy(Dst, Src);
        m_encoder->Push();
    }
}

void EmitPass::emitPredicatedVectorSelect(CVariable* Dst, CVariable* Src0, CVariable* Src1, CVariable* pred)
{
    unsigned int width = numLanes(m_currShader->m_SIMDSize);
    unsigned doff = 0, soff = 0;

    IGC_ASSERT(IsGRFAligned(Dst, EALIGN_GRF));

    uint32_t nElts = Dst->GetSize() / getGRFSize();

    for (uint32_t i = 0; i < nElts; ++i)
    {
        uint SrcSubReg = soff + width * i;
        uint DstSubReg = doff + width * i;

        m_encoder->SetNoMask();
        m_encoder->SetSrcSubReg(0, SrcSubReg);
        m_encoder->SetSrcSubReg(1, SrcSubReg);
        m_encoder->SetDstSubReg(DstSubReg);
        m_encoder->Select(pred, Dst, Src0, Src1);
        m_encoder->Push();
    }
}

// DstSubRegOffset and SrcSubRegOffset are in unit of element size.
void EmitPass::emitVectorCopy(CVariable* Dst, CVariable* Src, uint32_t nElts,
    uint32_t DstSubRegOffset, uint32_t SrcSubRegOffset, bool allowLargerSIMDSize)
{
    bool srcUniform = Src->IsUniform();
    bool dstUniform = Dst->IsUniform();

    // Uniform vector copy.
    if (srcUniform && dstUniform) {
        emitUniformVectorCopy(
            Dst, Src, nElts, DstSubRegOffset, SrcSubRegOffset,
            allowLargerSIMDSize);
        return;
    }

    unsigned int width = numLanes(m_currShader->m_SIMDSize);
    unsigned doff = DstSubRegOffset, soff = SrcSubRegOffset;
    for (uint32_t i = 0; i < nElts; ++i)
    {
        uint SrcSubReg = srcUniform ? soff + i : soff + width * i;
        uint DstSubReg = dstUniform ? doff + i : doff + width * i;

        uint SrcWidth = srcUniform ? 1 : width;
        uint DstWidth = dstUniform ? 1 : width;

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

void EmitPass::emitConstantVector(CVariable* Dst, uint64_t value)
{
    uint16_t width = Dst->IsUniform() ? 1 : numLanes(m_currShader->m_SIMDSize);
    CVariable * constant = m_currShader->ImmToVariable(value, Dst->GetType());
    for (uint16_t i = 0; width * i < Dst->GetNumberElement(); ++i)
    {
        m_encoder->SetDstSubReg(width * i);
        m_encoder->Copy(Dst, constant);
        m_encoder->Push();
    }
}

//emitVectorCopyToOrFromAOS()
// To implement the follow two functions.
//   emitVectorCopyToAOS():   IsToAOS = true
//     Copy from SOA vector to a AOS vectors in a struct.
//   emitVectorCopyFromAOS(): IsToAOS = false
//     Copy from AOS vector in a struct to a SOA vector.
//
// The size of 'struct' is given as 'AOSBytes'. For example, 'AOSBytes'
// is 4 bytes in the following example:
//
// __StructSOALayout_ {
//    __StructAOSLayout_ {
//        <4xi8> s0;          <-- indices: <0,0>
//    }
//    i32 s1
// }
//
// Insert to AOS field:
//   x = insertvalue <__StructSOALayout_> %x0, <4xi8> b, 0, 0
// Extract from AOS field:
//   x = extractvalue <__StructSOALayout_> %stVal, 0, 0
//
void EmitPass::emitVectorCopyToOrFromAOS(uint32_t AOSBytes,
    CVariable* Dst, CVariable* Src, uint32_t nElts,
    uint32_t DstSubRegOffset, uint32_t SrcSubRegOffset,
    bool IsToAOS)
{
    bool srcUniform = Src->IsUniform();
    bool dstUniform = Dst->IsUniform();

    // Uniform vector copy.
    if (srcUniform && dstUniform) {
        emitUniformVectorCopy(
            Dst, Src, nElts, DstSubRegOffset, SrcSubRegOffset);
        return;
    }

    const uint32_t nLanes = numLanes(m_currShader->m_SIMDSize);
    unsigned doff = DstSubRegOffset, soff = SrcSubRegOffset;
    uint32_t eltBytes = Dst->GetElemSize();
    uint32_t stride = AOSBytes / eltBytes;
    IGC_ASSERT(stride <= 4 && stride > 0);
    IGC_ASSERT((AOSBytes % eltBytes) == 0);
    if (IsToAOS) {
        // Copy to AOS field
        uint DstVStride = dstUniform ? 1 : stride;
        for (uint32_t i = 0; i < nElts; ++i)
        {
            uint SrcSubReg = soff + i * (srcUniform ? 1 : nLanes);
            uint DstSubReg = doff + i;

            m_encoder->SetDstRegion(DstVStride);
            m_encoder->SetSrcSubReg(0, SrcSubReg);
            m_encoder->SetDstSubReg(DstSubReg);
            m_encoder->Copy(Dst, Src);
            m_encoder->Push();
        }
    }
    else {
        // Copy from AOS field
        uint SrcVStride = srcUniform ? 0 : stride;
        for (uint32_t i = 0; i < nElts; ++i)
        {
            uint SrcSubReg = soff + i;
            uint DstSubReg = doff + i * (dstUniform ? 1 : nLanes);

            m_encoder->SetSrcSubReg(0, SrcSubReg);
            m_encoder->SetSrcRegion(0, SrcVStride, 1, 0);
            m_encoder->SetDstSubReg(DstSubReg);
            m_encoder->Copy(Dst, Src);
            m_encoder->Push();
        }
    }
}

// Push a new frame onto the stack by:
//  Update FP to the current SP
//  Increment SP by pushSize
//  Store value of previous frame's FP to the address of updated FP (for stack-walk)
void EmitPass::emitPushFrameToStack(Function* ParentFunction, unsigned& pushSize)
{
    CVariable* pFP = m_currShader->GetFP();
    CVariable* pSP = m_currShader->GetSP();

    // Set FP = SP
    m_encoder->Copy(pFP, pSP);
    m_encoder->Push();

    // Since we use unaligned oword writes, pushSize should be OW aligned address
    pushSize = int_cast<unsigned>(llvm::alignTo(pushSize, SIZE_OWORD));

    if (pushSize != 0)
    {
        // Update SP by pushSize
        emitAddPointer(pSP, pSP, m_currShader->ImmToVariable(pushSize, ISA_TYPE_UD));

        if (IGC_IS_FLAG_ENABLED(StackOverflowDetection)) {
            emitStackOverflowDetectionCall(ParentFunction, false);
        }

        if (IGC_IS_FLAG_ENABLED(EnableWriteOldFPToStack) && m_pCtx->type != ShaderType::RAYTRACING_SHADER) // RTX provides own stack memory, storing the FP at this point is invalid
        {
            // Store old FP value to current FP
            CVariable* pOldFP = m_currShader->GetPrevFP();
            // If previous FP is null (for kernel frame), we initialize it to 0
            if (pOldFP == nullptr)
            {
                pOldFP = m_currShader->GetNewVariable(pFP);
                m_encoder->Copy(pOldFP, m_currShader->ImmToVariable(0, pOldFP->GetType()));
                m_encoder->Push();
            }

            pFP = ReAlignUniformVariable(pFP, EALIGN_GRF);
            pOldFP = ReAlignUniformVariable(pOldFP, EALIGN_GRF);
            bool useA64 = (pFP->GetSize() == 8);
            if (shouldGenerateLSC())
            {
                ResourceDescriptor resource;
                resource.m_surfaceType = ESURFACE_STATELESS;
                emitLSCStore(nullptr, pOldFP, pFP, 64, 1, 0, &resource, (useA64 ? LSC_ADDR_SIZE_64b : LSC_ADDR_SIZE_32b), LSC_DATA_ORDER_TRANSPOSE, 0, 1);
                m_encoder->Push();
            }
            else
            {
                if (useA64)
                    m_encoder->OWStoreA64(pOldFP, pFP, SIZE_OWORD, 0);
                else {
                    // FP is in units of BYTES, but OWStore requires units of OWORDS
                    CVariable* offsetShr = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true, "FPOffset_OW");
                    m_encoder->SetSimdSize(SIMDMode::SIMD1);
                    m_encoder->SetNoMask();
                    m_encoder->SetSrcRegion(0, 0, 1, 0);
                    m_encoder->Shr(offsetShr, pFP, m_currShader->ImmToVariable(4, ISA_TYPE_UD));
                    m_encoder->Push();
                    m_encoder->OWStore(pOldFP, ESURFACE_STATELESS, nullptr, offsetShr, SIZE_OWORD, 0);
                }
                m_encoder->Push();
            }
        }
    }
}

void EmitPass::emitMul64_UDxUD(CVariable* Dst, CVariable* Src0, CVariable* Src1)
{
    IGC_ASSERT(Dst->GetType() == ISA_TYPE_UQ);
    IGC_ASSERT(Src0->GetType() == ISA_TYPE_UD);
    IGC_ASSERT(Src1->GetType() == ISA_TYPE_UD);

    CVariable* dstLo = m_currShader->GetNewVariable(Dst->GetNumberElement(),
        ISA_TYPE_UD, Dst->GetAlign(), Dst->IsUniform(),
        CName(Dst->getName(), "int64Lo"));
    CVariable* dstHi = m_currShader->GetNewVariable(Dst->GetNumberElement(),
        ISA_TYPE_UD, Dst->GetAlign(), Dst->IsUniform(),
        CName(Dst->getName(), "int64Hi"));

    m_encoder->Mul(dstLo, Src0, Src1);
    m_encoder->Push();

    m_encoder->MulH(dstHi, Src0, Src1);
    m_encoder->Push();

    //And now, pack the result
    CVariable* dstAsUD = m_currShader->BitCast(Dst, ISA_TYPE_UD);
    m_encoder->SetDstRegion(2);
    m_encoder->Copy(dstAsUD, dstLo);
    m_encoder->Push();

    m_encoder->SetDstRegion(2);
    m_encoder->SetDstSubReg(1);
    m_encoder->Copy(dstAsUD, dstHi);
    m_encoder->Push();
}

void EmitPass::emitAddPointer(CVariable* Dst, CVariable* Src, CVariable* offset)
{
    if ((!IGC_COUNT_FLAG_IN_LIMIT(DelayEmuInt64AddLimit) ||
         lanesToSIMDMode(Dst->GetNumberElement()) != SIMDMode::SIMD1) &&
        !m_currShader->m_Platform->hasInt64Add() &&
        (Dst->GetType() == ISA_TYPE_Q || Dst->GetType() == ISA_TYPE_UQ) &&
        (Src->GetType() == ISA_TYPE_Q || Src->GetType() == ISA_TYPE_UQ))
    {
        emitAddPair(Dst, Src, offset);
    }
    else
    {
        m_encoder->Add(Dst, Src, offset);
        m_encoder->Push();
    }
}

void EmitPass::emitAddPair(CVariable* Dst, CVariable* Src0, CVariable* Src1) {
    IGC_ASSERT(Dst->GetType() == ISA_TYPE_Q || Dst->GetType() == ISA_TYPE_UQ);
    IGC_ASSERT(Src0->GetType() == ISA_TYPE_Q || Src0->GetType() == ISA_TYPE_UQ);
    IGC_ASSERT(Src1->GetType() == ISA_TYPE_Q || Src1->GetType() == ISA_TYPE_UQ ||
        Src1->GetType() == ISA_TYPE_UV || Src1->GetType() == ISA_TYPE_UD ||
        Src1->GetType() == ISA_TYPE_D);

    bool IsUniformDst = Dst->IsUniform();

    unsigned short NumElts = Dst->GetNumberElement();
    SIMDMode Mode = lanesToSIMDMode(NumElts);
    bool isScalar = Mode == SIMDMode::SIMD1;

    VISA_Type NewType = ISA_TYPE_UD;
    CVariable* Src0Alias = m_currShader->GetNewAlias(Src0, NewType, 0, 0);
    CVariable* newVar = isScalar ?
        m_currShader->GetNewVariable(NumElts * 2, NewType, EALIGN_GRF, IsUniformDst, CName(Src0->getName(), "HiLo32")) :
        nullptr;
    CVariable* L0 = isScalar ?
        m_currShader->GetNewAlias(newVar, NewType, 0, 0) :
        m_currShader->GetNewVariable(NumElts, NewType, EALIGN_GRF, IsUniformDst, CName(Src0->getName(), "Lo32"));
    CVariable* H0 = isScalar ?
        m_currShader->GetNewAlias(newVar, NewType, sizeof(uint32_t) * NumElts, 0) :
        m_currShader->GetNewVariable(NumElts, NewType, EALIGN_GRF, IsUniformDst, CName(Src0->getName(), "Hi32"));

    if (isScalar)
    {
        if (IsUniformDst) {
            m_encoder->SetNoMask();
            m_encoder->SetUniformSIMDSize(lanesToSIMDMode(NumElts * 2));
        }
        m_encoder->SetSrcRegion(0, 1, 1, 0);
        m_encoder->Copy(newVar, Src0Alias);
        m_encoder->Push();
    }
    else
    {
        // Split Src0 into L0 and H0
        // L0 := Offset[0];
        if (IsUniformDst) {
            m_encoder->SetNoMask();
            m_encoder->SetUniformSIMDSize(Mode);
        }
        if (Src0->IsUniform())
            m_encoder->SetSrcRegion(0, 0, 1, 0);
        else
            m_encoder->SetSrcRegion(0, 2, 1, 0);
        m_encoder->Copy(L0, Src0Alias);
        m_encoder->Push();
        // H0 := Offset[1];
        if (IsUniformDst) {
            m_encoder->SetNoMask();
            m_encoder->SetUniformSIMDSize(Mode);
        }
        m_encoder->SetSrcSubReg(0, 1);
        if (Src0->IsUniform())
            m_encoder->SetSrcRegion(0, 0, 1, 0);
        else
            m_encoder->SetSrcRegion(0, 2, 1, 0);
        m_encoder->Copy(H0, Src0Alias);
        m_encoder->Push();
    }

    // If Src1 is D type value, extend it to L1 and H1, if Src1 is UD type value
    // we can ignore its high-32 bit part, which will be all zeros.
    CVariable* L1 = nullptr;
    CVariable* H1 = nullptr;
    if (Src1->GetType() == ISA_TYPE_D) {
         newVar = isScalar ?
             m_currShader->GetNewVariable(NumElts * 2, NewType, EALIGN_GRF, IsUniformDst, CName(Src1->getName(), "HiLo32")) :
             nullptr;
         L1 = isScalar ?
             m_currShader->GetNewAlias(newVar, NewType, 0, 0) :
             m_currShader->GetNewVariable(NumElts, NewType, EALIGN_GRF, IsUniformDst, CName(Src1->getName(), "Lo32"));
         H1 = isScalar ?
             m_currShader->GetNewAlias(newVar, NewType, sizeof(uint32_t) * NumElts, 0) :
             m_currShader->GetNewVariable(NumElts, NewType, EALIGN_GRF, IsUniformDst, CName(Src1->getName(), "Hi32"));

         // L1 := Offset[0];
         if (IsUniformDst) {
             m_encoder->SetNoMask();
             m_encoder->SetUniformSIMDSize(Mode);
         }
         if (Src1->IsUniform())
             m_encoder->SetSrcRegion(0, 0, 1, 0);
         else
             m_encoder->SetSrcRegion(0, 1, 1, 0);
         m_encoder->Copy(L1, Src1);
         m_encoder->Push();
         // H1 := Offset[1];
         if (IsUniformDst) {
             m_encoder->SetNoMask();
             m_encoder->SetUniformSIMDSize(Mode);
         }
         if (Src1->IsUniform())
             m_encoder->SetSrcRegion(0, 0, 1, 0);
         else
             m_encoder->SetSrcRegion(0, 1, 1, 0);
         m_encoder->IShr(H1, Src1, m_currShader->ImmToVariable(31, ISA_TYPE_UD));
         m_encoder->Push();
    }
    else if (Src1->GetType() == ISA_TYPE_Q || Src1->GetType() == ISA_TYPE_UQ)
    {
        CVariable* Src1Alias = m_currShader->GetNewAlias(Src1, NewType, 0, 0);
        CVariable* newVar = isScalar ?
            m_currShader->GetNewVariable(NumElts * 2, NewType, EALIGN_GRF, IsUniformDst, CName(Src1->getName(), "HiLo32")) :
            nullptr;
        L1 = isScalar ?
            m_currShader->GetNewAlias(newVar, NewType, 0, 0) :
            m_currShader->GetNewVariable(NumElts, NewType, EALIGN_GRF, IsUniformDst, CName(Src1->getName(), "Lo32"));
        H1 = isScalar ?
            m_currShader->GetNewAlias(newVar, NewType, sizeof(uint32_t) * NumElts, 0) :
            m_currShader->GetNewVariable(NumElts, NewType, EALIGN_GRF, IsUniformDst, CName(Src1->getName(), "Hi32"));

        if (isScalar)
        {
            if (IsUniformDst) {
                m_encoder->SetNoMask();
                m_encoder->SetUniformSIMDSize(lanesToSIMDMode(NumElts * 2));
            }
            m_encoder->SetSrcRegion(0, 1, 1, 0);
            m_encoder->Copy(newVar, Src1Alias);
            m_encoder->Push();
        }
        else
        {
            // Split Src1 into L1 and H1
            // L1 := Offset[0];
            if (IsUniformDst) {
                m_encoder->SetNoMask();
                m_encoder->SetUniformSIMDSize(Mode);
            }
            if (Src1->IsUniform())
                m_encoder->SetSrcRegion(0, 0, 1, 0);
            else
                m_encoder->SetSrcRegion(0, 2, 1, 0);
            m_encoder->Copy(L1, Src1Alias);
            m_encoder->Push();
            // H1 := Offset[1];
            if (IsUniformDst) {
                m_encoder->SetNoMask();
                m_encoder->SetUniformSIMDSize(Mode);
            }
            m_encoder->SetSrcSubReg(0, 1);
            if (Src1->IsUniform())
                m_encoder->SetSrcRegion(0, 0, 1, 0);
            else
                m_encoder->SetSrcRegion(0, 2, 1, 0);
            m_encoder->Copy(H1, Src1Alias);
            m_encoder->Push();
        }
    }

    newVar = isScalar ?
        m_currShader->GetNewVariable(NumElts * 2, NewType, EALIGN_GRF, IsUniformDst, CName(Dst->getName(), "HiLo32")) :
        nullptr;
    CVariable* Lo = isScalar ?
        m_currShader->GetNewAlias(newVar, NewType, 0, 0) :
        m_currShader->GetNewVariable(NumElts, NewType, EALIGN_GRF, IsUniformDst, CName(Dst->getName(), "Lo32"));
    CVariable* Hi = isScalar ?
        m_currShader->GetNewAlias(newVar, NewType, sizeof(uint32_t) * NumElts, 0) :
        m_currShader->GetNewVariable(NumElts, NewType, EALIGN_GRF, IsUniformDst, CName(Dst->getName(), "Lo32"));
    // (Lo, Hi) := AddPair(L0, H0, ImmLo, ImmHi);
    if (IsUniformDst) {
        m_encoder->SetNoMask();
        m_encoder->SetUniformSIMDSize(Mode);
        m_encoder->SetSrcRegion(0, 1, 1, 0);
        m_encoder->SetSrcRegion(1, 1, 1, 0);
    }
    if (L1 != nullptr)
        m_encoder->AddPair(Lo, Hi, L0, H0, L1, H1);
    else
        m_encoder->AddPair(Lo, Hi, L0, H0, Src1);
    m_encoder->Push();

    CVariable* DstAlias = m_currShader->GetNewAlias(Dst, NewType, 0, 0);
    if (isScalar)
    {
        if (IsUniformDst) {
            m_encoder->SetNoMask();
            m_encoder->SetUniformSIMDSize(lanesToSIMDMode(NumElts * 2));
            m_encoder->SetSrcRegion(0, 1, 1, 0);
        }
        m_encoder->SetDstRegion(1);
        m_encoder->Copy(DstAlias, newVar);
        m_encoder->Push();
    }
    else
    {
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
}

/// \brief Copy all values from the src variable to the dst variable.
/// The last argument is the underlying value type.
void EmitPass::emitCopyAll(CVariable* Dst, CVariable* Src, llvm::Type* Ty)
{
    if (Src->GetVarType() == EVARTYPE_PREDICATE)
    {
        IGC_ASSERT_MESSAGE(!Ty->isVectorTy(), "vector of predicates?");
        IGC_ASSERT(Dst->GetVarType() == Src->GetVarType());
        CVariable* Zero = m_currShader->ImmToVariable(0, ISA_TYPE_BOOL);
        m_encoder->Or(Dst, Src, Zero);
        m_encoder->Push();
    }
    else if (Ty->isVectorTy())
    {
        unsigned NElts = (unsigned)cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements();
        emitVectorCopy(Dst, Src, NElts);
    }
    else if (Ty->isStructTy())
    {
        IGC_ASSERT(Dst->GetType() == ISA_TYPE_B);
        IGC_ASSERT(Src->GetType() == ISA_TYPE_B);

        if (!Src->IsUniform() && Dst->IsUniform())
        {
            IGC_ASSERT_MESSAGE(0, "Does not support non-uniform to uniform struct copy");
        }

        StructType* STy = cast<StructType>(Ty);
        const StructLayout* SL = m_DL->getStructLayout(STy);
        unsigned srcLanes = Src->IsUniform() ? 1 : numLanes(m_currShader->m_SIMDSize);
        unsigned dstLanes = Dst->IsUniform() ? 1 : numLanes(m_currShader->m_SIMDSize);
        for (unsigned i = 0; i < STy->getNumElements(); i++)
        {
            unsigned elementOffset = (unsigned)SL->getElementOffset(i);
            Type* elementType = STy->getElementType(i);

            emitMayUnalignedVectorCopy(Dst, elementOffset * dstLanes,
                Src, elementOffset * srcLanes, elementType);
        }
    }
    else
    {
        IGC_ASSERT_MESSAGE(Ty->isSingleValueType(), "not supported");
        m_encoder->Copy(Dst, Src);
        m_encoder->Push();
    }
}

void EmitPass::emitSqrt(Instruction* inst)
{
    GenIntrinsicInst* intrinCall = llvm::cast<GenIntrinsicInst>(inst);
    CVariable* src0 = GetSymbol(intrinCall->getArgOperand(0));
    src0 = BroadcastIfUniform(src0);

    m_encoder->Sqrt(m_destination, src0);
}

void EmitPass::emitUmin(llvm::IntrinsicInst* inst)
{
    CVariable* srcs[2] = { nullptr, nullptr };
    CVariable* dst = m_destination;
    srcs[0] = GetSymbol(inst->getArgOperand(0));
    srcs[1] = GetSymbol(inst->getArgOperand(1));
    srcs[0] = m_currShader->BitCast(srcs[0], GetUnsignedType(srcs[0]->GetType()));
    srcs[1] = m_currShader->BitCast(srcs[1], GetUnsignedType(srcs[1]->GetType()));
    dst = m_currShader->BitCast(m_destination, GetUnsignedType(m_destination->GetType()));

    m_encoder->Min(dst, srcs[0], srcs[1]);
}

void EmitPass::emitSmin(llvm::IntrinsicInst* inst)
{
    m_encoder->Min(m_destination, GetSymbol(inst->getArgOperand(0)), GetSymbol(inst->getArgOperand(1)));
}

void EmitPass::emitUmax(llvm::IntrinsicInst* inst)
{
    CVariable* srcs[2] = { nullptr, nullptr };
    CVariable* dst = m_destination;
    srcs[0] = GetSymbol(inst->getArgOperand(0));
    srcs[1] = GetSymbol(inst->getArgOperand(1));
    srcs[0] = m_currShader->BitCast(srcs[0], GetUnsignedType(srcs[0]->GetType()));
    srcs[1] = m_currShader->BitCast(srcs[1], GetUnsignedType(srcs[1]->GetType()));
    dst = m_currShader->BitCast(m_destination, GetUnsignedType(m_destination->GetType()));

    m_encoder->Max(dst, srcs[0], srcs[1]);
}

void EmitPass::emitSmax(llvm::IntrinsicInst* inst)
{
    m_encoder->Max(m_destination, GetSymbol(inst->getArgOperand(0)), GetSymbol(inst->getArgOperand(1)));
}

void EmitPass::emitFrc(llvm::GenIntrinsicInst* inst)
{
    CVariable* src0 = GetSymbol(inst->getArgOperand(0));
    src0 = BroadcastIfUniform(src0);

    m_encoder->Frc(m_destination, src0);
}

void IGC::EmitPass::emitCanonicalize(llvm::Instruction* inst, const DstModifier& modifier)
{
    // Force to flush denormal fp value to zero. Select one of two possible solutions:
    // 1. add inputVal, -0.0
    // 2. mul inputVal, 1.0
    // A normalized fp value isn't changed.
    // The operation is done only if particular flags are set.
    // If the instruction should be emitted anyway, flushing a subnormal to zero has to implemented in other way.
    CodeGenContext* pCodeGenContext = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    bool flushVal = pCodeGenContext->m_floatDenormMode16 == ::IGC::FLOAT_DENORM_FLUSH_TO_ZERO && inst->getType()->isHalfTy();
    flushVal = flushVal || (pCodeGenContext->m_floatDenormMode32 == ::IGC::FLOAT_DENORM_FLUSH_TO_ZERO && inst->getType()->isFloatTy());
    flushVal = flushVal || (pCodeGenContext->m_floatDenormMode64 == ::IGC::FLOAT_DENORM_FLUSH_TO_ZERO && inst->getType()->isDoubleTy());
    if (flushVal || modifier.sat)
    {
        CVariable* inputVal = GetSymbol(inst->getOperand(0));
        CVariable* negativeZero = m_currShader->GetScalarConstant(llvm::ConstantFP::get(inst->getType(), -0.0));
        m_encoder->SetDstModifier(modifier);
        m_encoder->Add(m_destination, inputVal, negativeZero);
        m_encoder->Push();
    }
}

void IGC::EmitPass::emitStaticConstantPatchValue(llvm::StaticConstantPatchIntrinsic* staticConstantPatch32)
{
    std::string patchName = staticConstantPatch32->getPatchName().str();
    m_encoder->AddVISASymbol(patchName, m_destination);
}

// emit llvm.bswap
void EmitPass::emitLLVMbswap(IntrinsicInst* inst)
{
    Type* Ty = inst->getType();
    Value* Arg = inst->getArgOperand(0);
    uint32_t nBytes = int_cast<uint32_t>(m_DL->getTypeSizeInBits(Ty));
    IGC_ASSERT_MESSAGE(nBytes % 16 == 0, "Incorrect llvm.bswap");
    IGC_ASSERT_MESSAGE(!Ty->isVectorTy(), "Incorrect llvm.bswap");
    nBytes >>= 3;  // Now, nBytes are in unit of byte.

    CVariable* Src = GetSymbol(Arg);
    CVariable* Dst = m_destination;
    uint32_t width = numLanes(m_currShader->m_SIMDSize);
    bool srcUniform = Src->IsUniform();
    bool dstUniform = Dst->IsUniform();

    CVariable* SrcB = m_currShader->GetNewAlias(Src, ISA_TYPE_UB, 0, 0);
    if (nBytes == 2 || nBytes == 4)
    {
        CVariable* DstB = m_currShader->GetNewAlias(Dst, ISA_TYPE_UB, 0, 0);

        // Generating byte mov
        for (unsigned i = 0; i < nBytes / 2; ++i)
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
    else if (nBytes == 8)
    {
        // Need to so lower DW and upper DW separately first.
        m_currShader->GetNewAlias(Src, ISA_TYPE_UD, 0, 0);
        CVariable* DstH = m_currShader->GetNewVariable(
            Src->GetNumberElement(),
            ISA_TYPE_UD,
            Src->GetAlign(),
            srcUniform,
            CName::NONE);
        CVariable* DstL = m_currShader->GetNewVariable(
            Src->GetNumberElement(),
            ISA_TYPE_UD,
            Src->GetAlign(),
            srcUniform,
            CName::NONE);
        CVariable* DstHB = m_currShader->GetNewAlias(DstH, ISA_TYPE_UB, 0, 0);
        CVariable* DstLB = m_currShader->GetNewAlias(DstL, ISA_TYPE_UB, 0, 0);

        bool split = (width == 16);
        for (unsigned n = 0; n < 2; ++n)
        {
            for (unsigned i = 0; i < 4; ++i)
            {
                // swap bytes[i] and bytes[j]
                uint32_t j = 3 - i;
                if (split && !srcUniform)
                {
                    m_encoder->SetSrcSubReg(0, 4 * n + i);
                    m_encoder->SetSrcRegion(0, 8, 1, 0);
                    m_encoder->SetDstSubReg(j);
                    m_encoder->SetDstRegion(4);
                    m_encoder->SetSimdSize(SIMDMode::SIMD8);
                    m_encoder->SetMask(EMASK_Q1);
                    m_encoder->Copy(n == 0 ? DstHB : DstLB, SrcB);
                    m_encoder->Push();

                    m_encoder->SetSrcSubReg(0, 2 * getGRFSize() + 4 * n + i);
                    m_encoder->SetSrcRegion(0, 8, 1, 0);
                    m_encoder->SetDstSubReg(getGRFSize() + j);
                    m_encoder->SetDstRegion(4);
                    m_encoder->SetSimdSize(SIMDMode::SIMD8);
                    m_encoder->SetMask(EMASK_Q2);
                    m_encoder->Copy(n == 0 ? DstHB : DstLB, SrcB);
                    m_encoder->Push();
                }
                else
                {
                    // DstH[B]/DstL[B] have the same uniformness as Src !
                    m_encoder->SetSrcSubReg(0, 4 * n + i);
                    m_encoder->SetSrcRegion(0, srcUniform ? 0 : 8, 1, 0);
                    m_encoder->SetDstSubReg(j);
                    m_encoder->SetDstRegion(srcUniform ? 1 : 4);
                    m_encoder->Copy(n == 0 ? DstHB : DstLB, SrcB);
                    m_encoder->Push();
                }
            }
        }

        // Now, mov DstH and DstL to Dst
        CVariable* DstD = m_currShader->GetNewAlias(Dst, ISA_TYPE_UD, 0, 0);

        // When dst is uniform, dst does not cross 2 GRFs, split isn't needed.
        if (split && !dstUniform)
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
        IGC_ASSERT_MESSAGE(0, "Unsupported type for llvm.bswap!");
        return;
    }
}

void EmitPass::setPredicateForDiscard(CVariable* pPredicate)
{
    // Input predicate parameter is used when resource variable is non-uniform
    // and compiler needs to create the resource loop.
    bool isInversePredicate = false;
    if (pPredicate != nullptr)
    {
        m_encoder->SetPredicate(pPredicate);
        m_encoder->SetInversePredicate(isInversePredicate);
    }
}

void EmitPass::ForceDMask(bool createJmpForDiscard)
{

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

}

void EmitPass::emitGetBufferPtr(GenIntrinsicInst* inst)
{
    Value* buf_idxv = inst->getOperand(0);
    Value* bufTyVal = inst->getOperand(1);
    IGC_ASSERT(isa<ConstantInt>(bufTyVal));
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
        IGC_ASSERT_MESSAGE(0, "unexpect buffer type for GetBufferPtr");
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
    m_currShader->SetBindingTableEntryCountAndBitmap(directIdx, bufType, 0, bti);
}

ResourceDescriptor EmitPass::GetResourceVariable(Value* resourcePtr)
{
    ResourceDescriptor resource;
    BufferType bufType = BUFFER_TYPE_UNKNOWN;
    uint as = 0;
    if (auto *GII = dyn_cast<GenIntrinsicInst>(resourcePtr); GII &&
        GII->getIntrinsicID() == GenISAIntrinsic::GenISA_GetBufferPtr)
    {
        // from GetBufferPtr
        Value* bufTyVal = GII->getOperand(1);
        IGC_ASSERT(isa<ConstantInt>(bufTyVal));
        bufType = (BufferType)(cast<ConstantInt>(bufTyVal)->getZExtValue());
        resource.m_resource = GetSymbol(resourcePtr);
    }
    else
    {
        as = resourcePtr->getType()->getPointerAddressSpace();
        uint bufferIndex = 0;
        bool directIndexing = false;

        bufType = DecodeAS4GFXResource(as, directIndexing, bufferIndex);

        if (as == ADDRESS_SPACE_THREAD_ARG)
            resource.m_isThreadArg = true;

        if (IsBindless(bufType) ||
            !directIndexing)
        {
            if (isa<IntToPtrInst>(resourcePtr))
            {
                IntToPtrInst* i2p = dyn_cast<IntToPtrInst>(resourcePtr);
                resource.m_resource = GetSymbol(i2p->getOperand(0));
            }
            else
            {
                resource.m_resource = GetSymbol(resourcePtr);
            }

            if (resource.m_resource->GetElemSize() < 4)
            {
                // vISA assumes all BTIs to be 32 bit. Need to cast, otherwise higher bits would be uninitialized.
                unsigned numInstance = resource.m_resource->GetNumberInstance();
                CVariable* newResource = m_currShader->GetNewVariable(
                    resource.m_resource->GetNumberElement(),
                    ISA_TYPE_UD,
                    resource.m_resource->IsUniform() ? EALIGN_DWORD : EALIGN_GRF,
                    resource.m_resource->IsUniform(),
                    numInstance,
                    CName::NONE);

                m_encoder->Cast(newResource, resource.m_resource);

                if (numInstance == 2)
                {
                    m_encoder->SetSecondHalf(!m_encoder->IsSecondHalf());
                    m_encoder->Cast(newResource, resource.m_resource);
                    m_encoder->SetSecondHalf(!m_encoder->IsSecondHalf());
                }

                resource.m_resource = newResource;
            }

            if (!directIndexing)
            {
                m_currShader->SetBindingTableEntryCountAndBitmap(false, bufType, 0, 0);
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
                break;
            case RESOURCE:
                bti = m_currShader->m_pBtiLayout->GetTextureIndex(bufferIndex);
                break;
            case SLM:
                bti = 254;  // \todo, remove hard-coding
                break;
            default:
                bti = m_currShader->m_pBtiLayout->GetStatelessBindingTableIndex();
                break;
            }
            resource.m_resource = m_currShader->ImmToVariable(bti, ISA_TYPE_UD);
            m_currShader->SetBindingTableEntryCountAndBitmap(directIndexing, bufType, bufferIndex, bti);
        }
    }

    if ((bufType == BINDLESS_CONSTANT_BUFFER) ||
        (bufType == CONSTANT_BUFFER) ||
        (as == ADDRESS_SPACE_CONSTANT))
    {
        resource.m_isConstant = true;
    }

    if (IsBindless(bufType))
    {
        resource.m_surfaceType = ESURFACE_BINDLESS;
    }
    else if (IsSSHbindless(bufType))
    {
        resource.m_surfaceType = ESURFACE_SSHBINDLESS;
    }
    else if (bufType == SLM)
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
        if (m_currShader->m_Platform->hasScratchSurface() &&
            resourcePtr->getType()->getPointerAddressSpace() == ADDRESS_SPACE_PRIVATE)
        {
            ModuleMetaData* modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
            if (modMD && modMD->compOpt.UseScratchSpacePrivateMemory)
            {
                resource.m_surfaceType = ESURFACE_SCRATCH;
                return resource;
            }
        }
        resource.m_surfaceType = ESURFACE_STATELESS;
    }
    return resource;
}

SamplerDescriptor EmitPass::GetSamplerVariable(Value* sampleOp)
{
    SamplerDescriptor sampler;
    unsigned int samplerIdx = 0;
    BufferType sampType = BUFFER_TYPE_UNKNOWN;

    if (GenIntrinsicInst* sample = dyn_cast<GenIntrinsicInst>(sampleOp))
    {
        if (sample->getIntrinsicID() == GenISAIntrinsic::GenISA_GetBufferPtr)
        {
            Value* bufTyVal = cast<GenIntrinsicInst>(sampleOp)->getOperand(1);
            IGC_ASSERT(isa<ConstantInt>(bufTyVal));
            sampType = (BufferType)(cast<ConstantInt>(bufTyVal)->getZExtValue());
            sampler.m_sampler = GetSymbol(sampleOp);
            IGC_ASSERT(sampType == SAMPLER);
            sampler.m_samplerType = ESAMPLER_NORMAL;
            return sampler;
        }
    }

    bool isBindless = false;
    bool directIdx = false;

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
    return sampler;
}

bool EmitPass::ResourceLoopHeader(
    ResourceDescriptor& resource,
    CVariable*& flag,
    uint& label,
    uint ResourceLoopMarker,
    int* subInteration)
{
    SamplerDescriptor sampler;
    return ResourceLoopHeader(resource, sampler, flag, label, ResourceLoopMarker, subInteration);
}

// Insert loop header to handle non-uniform resource and sampler
// This generates sub-optimal code for SIMD32, this can be revisited if we need better code generation
bool EmitPass::ResourceLoopHeader(
    ResourceDescriptor& resource,
    SamplerDescriptor& sampler,
    CVariable*& flag,
    uint& label,
    uint ResourceLoopMarker,
    int* subInteration)
{
    if (resource.m_surfaceType != ESURFACE_BINDLESS &&
        resource.m_surfaceType != ESURFACE_SSHBINDLESS &&
        resource.m_surfaceType != ESURFACE_NORMAL)
    {
        // Loop only needed for access with surface state
        IGC_ASSERT(ResourceLoopMarker == 0);
        return false;
    }
    bool uniformResource = resource.m_resource == nullptr || resource.m_resource->IsUniform();
    bool uniformSampler = sampler.m_sampler == nullptr || sampler.m_sampler->IsUniform();
    if (uniformResource && uniformSampler)
    {
        IGC_ASSERT(ResourceLoopMarker == 0);
        return false;
    }
    // cannot fuse resource loop in simd32 mode when there are
    // two separated loops for 1st-half and 2nd-half.
    if (m_SimdMode == SIMDMode::SIMD32 && m_currShader->m_numberInstance >= 2)
    {
        ResourceLoopMarker = 0;
    }
    if (ResourceLoopMarker & ResourceLoopAnalysis::MarkResourceLoopInside)
    {
        resource = m_RLA->GetResourceLoopResource();
        sampler = m_RLA->GetResourceLoopSampler();
        flag = m_RLA->GetResourceLoopFlag();
        return true;
    }
    m_currShader->IncNumSampleBallotLoops();

    label = m_encoder->GetNewLabelID("_opt_resource_loop");
    m_encoder->AddDivergentResourceLoopLabel(label);
    m_encoder->Push();

    if (subInteration && m_currShader->m_Platform->hasSlowSameSBIDLoad())
    {
        // return with res loop label added and ready for the first unroll
        if (*subInteration == 0)
        {
            *subInteration = 1;
        }
    }

    return true;
}

bool EmitPass::ResourceLoopNeedsLoop(
    ResourceDescriptor& resource,
    SamplerDescriptor& sampler,
    CVariable*& flag,
    uint ResourceLoopMarker)
{
    if (resource.m_surfaceType != ESURFACE_BINDLESS &&
        resource.m_surfaceType != ESURFACE_SSHBINDLESS &&
        resource.m_surfaceType != ESURFACE_NORMAL)
    {
        // Loop only needed for access with surface state
        IGC_ASSERT(ResourceLoopMarker == 0);
        return false;
    }
    bool uniformResource = resource.m_resource == nullptr || resource.m_resource->IsUniform();
    bool uniformSampler = sampler.m_sampler == nullptr || sampler.m_sampler->IsUniform();
    if (uniformResource && uniformSampler)
    {
        IGC_ASSERT(ResourceLoopMarker == 0);
        return false;
    }

    if (m_SimdMode == SIMDMode::SIMD32 && m_currShader->m_numberInstance >= 2)
    {
        ResourceLoopMarker = 0;
    }
    if (ResourceLoopMarker & ResourceLoopAnalysis::MarkResourceLoopInside)
    {
        resource = m_RLA->GetResourceLoopResource();
        sampler = m_RLA->GetResourceLoopSampler();
        flag = m_RLA->GetResourceLoopFlag();
    }

    return true;
}

bool EmitPass::ResourceLoopSubIteration(
    ResourceDescriptor& resource,
    CVariable*& flag,
    uint& label,
    uint ResourceLoopMarker,
    int iteration,
    CVariable* prevFlag)
{
    SamplerDescriptor sampler;
    return ResourceLoopSubIteration(resource, sampler, flag, label, ResourceLoopMarker, iteration, prevFlag);
}

bool EmitPass::ResourceLoopSubIteration(
    ResourceDescriptor& resource,
    SamplerDescriptor& sampler,
    CVariable*& flag,
    uint& label,
    uint ResourceLoopMarker,
    int iteration,
    CVariable* prevFlag)
{
    // Even though the check is done in header setup,
    // recheck again to make sure only handle non-uniform
    if (resource.m_surfaceType != ESURFACE_BINDLESS &&
        resource.m_surfaceType != ESURFACE_SSHBINDLESS &&
        resource.m_surfaceType != ESURFACE_NORMAL)
    {
        // Loop only needed for access with surface state
        IGC_ASSERT(ResourceLoopMarker == 0);
        return false;
    }
    bool uniformResource = resource.m_resource == nullptr || resource.m_resource->IsUniform();
    bool uniformSampler = sampler.m_sampler == nullptr || sampler.m_sampler->IsUniform();
    if (uniformResource && uniformSampler)
    {
        IGC_ASSERT(ResourceLoopMarker == 0);
        return false;
    }

    // only for second and next unrolled iterations we need to handle this (first is guaranteed to have nonzero emask)
    bool needSafeguard = (iteration > 1);

    CVariable* resourceFlag = nullptr;
    CVariable* samplerFlag = nullptr;
    CVariable* offset = nullptr;

    if (!uniformResource)
    {
        ResourceDescriptor uniformResource;
        resourceFlag = m_currShader->GetNewVariable(numLanes(m_SimdMode), ISA_TYPE_BOOL, EALIGN_BYTE, CName::NONE);
        uniformResource.m_surfaceType = resource.m_surfaceType;
        uniformResource.m_resource = UniformCopy(resource.m_resource, offset, prevFlag, false, needSafeguard);
        uniformResource.m_isConstant = resource.m_isConstant;
        {
            m_encoder->Cmp(EPREDICATE_EQ, resourceFlag, uniformResource.m_resource, resource.m_resource);
            m_encoder->Push();
        }
        resource = uniformResource;
    }

    if (!uniformSampler)
    {
        SamplerDescriptor uniformSampler;
        samplerFlag = m_currShader->GetNewVariable(numLanes(m_SimdMode), ISA_TYPE_BOOL, EALIGN_BYTE, CName::NONE);
        uniformSampler.m_samplerType = sampler.m_samplerType;
        uniformSampler.m_sampler = UniformCopy(sampler.m_sampler, offset);
        {
            m_encoder->Cmp(EPREDICATE_EQ, samplerFlag, uniformSampler.m_sampler, sampler.m_sampler);
            m_encoder->Push();
        }
        sampler = uniformSampler;
    }
    if (resourceFlag && samplerFlag)
    {
        flag = m_currShader->GetNewVariable(numLanes(m_SimdMode), ISA_TYPE_BOOL, EALIGN_BYTE, CName::NONE);
        m_encoder->And(flag, resourceFlag, samplerFlag);
        m_encoder->Push();
    }
    else
    {
        flag = resourceFlag != nullptr ? resourceFlag : samplerFlag;
    }
    if (m_SimdMode == SIMDMode::SIMD32 && m_currShader->m_numberInstance == 2)
    {
        // For SIMD32 need to initialize to 1 the other half of the flag
        // ToDo: check if this is actually necessary, as the other half should not get used
        m_encoder->SetSecondHalf(!m_encoder->IsSecondHalf());
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        m_encoder->SetSrcRegion(1, 0, 1, 0);
        m_encoder->Cmp(EPREDICATE_EQ, flag, m_currShader->GetR0(), m_currShader->GetR0());
        m_encoder->Push();
        m_encoder->SetSecondHalf(!m_encoder->IsSecondHalf());
    }
    if (ResourceLoopMarker & ResourceLoopAnalysis::MarkResourceLoopStart)
    {
        m_RLA->SaveStateLoopStart(resource, sampler, flag, label);
    }
    return true;
}

void EmitPass::ResourceLoopBackEdge(
    bool needLoop,
    CVariable* flag,
    uint label,
    uint ResourceLoopMarker)
{
    // cannot fuse resource loop in simd32 mode when there are
    // two separated loops for 1st-half and 2nd-half.
    if (m_SimdMode == SIMDMode::SIMD32 && m_currShader->m_numberInstance >= 2)
    {
        ResourceLoopMarker = 0;
    }
    if (ResourceLoopMarker & ResourceLoopAnalysis::MarkResourceLoopEnd)
    {
        flag = m_RLA->GetResourceLoopFlag();
        label = m_RLA->GetResourceLoopLabel();
        IGC_ASSERT(flag && label);
        m_encoder->SetInversePredicate(true);
        m_encoder->Jump(flag, label);
        m_encoder->Push();
        m_RLA->ClearStateLoopEnd();
    }
    else if(needLoop && !ResourceLoopMarker)
    {
        m_encoder->SetInversePredicate(true);
        m_encoder->Jump(flag, label);
        m_encoder->Push();
    }
}

void EmitPass::emitStateRegID(uint32_t BitStart, uint32_t BitEnd)
{
    // For example, emitStateRegID(14, 18) would return the value in the
    // range [18:14].
    uint32_t and_imm = BITMASK_RANGE(BitStart, BitEnd);
    uint32_t shr_imm = BitStart;
    m_encoder->And(m_destination, m_currShader->GetSR0(), m_currShader->ImmToVariable(and_imm, ISA_TYPE_UD));
    m_encoder->Shr(m_destination, m_destination, m_currShader->ImmToVariable(shr_imm, ISA_TYPE_UD));
    m_encoder->Push();
}

void EmitPass::emitMulAdd16(Instruction* I, const SSource Sources[2], const DstModifier& DstMod)
{
    CVariable* LVar = GetSrcVariable(Sources[0]);
    CVariable* RVar = GetSrcVariable(Sources[1]);
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
        IGC_ASSERT_MESSAGE(I->getOpcode() == Instruction::Add, "Unknown Opcode.");
        m_encoder->Add(m_destination, LVar, RVar);
    }
    m_encoder->Push();
}

void EmitPass::emitThreadPause(llvm::GenIntrinsicInst* inst)
{
    CVariable* TSC_reg = m_currShader->GetTSC();
    CVariable* TSC_pause = m_currShader->GetNewAlias(TSC_reg, ISA_TYPE_UD, 16, 1);
    uint64_t var = GetImmediateVal(inst->getOperand(0));
    if (var >= 32)
        var = 0x03E0;
    else if (var <= 4)
        var = 0x0080;
    else
        var <<= 5;
    m_encoder->Copy(TSC_pause, m_currShader->ImmToVariable(var, ISA_TYPE_UD));
    m_encoder->Push();
}


void EmitPass::emitWaveBallot(llvm::GenIntrinsicInst* inst)
{
    bool disableHelperLanes = int_cast<int>(cast<ConstantInt>(inst->getArgOperand(1))->getSExtValue()) == 2;
    if (disableHelperLanes)
    {
        ForceDMask();
    }
    // If single user of this opt is firstbitLo intrinsic, than cast the destination type to UD
    if (inst->hasOneUse())
    {
        if (GenIntrinsicInst * userInst = dyn_cast<GenIntrinsicInst>(inst->user_back()))
        {
            if (userInst->getIntrinsicID() == GenISAIntrinsic::GenISA_firstbitLo)
            {
                m_destination = m_currShader->BitCast( m_destination, GetUnsignedType( m_destination->GetType() ) );
            }
        }
    }

    CVariable* destination = m_destination;
    if (!m_destination->IsUniform())
    {
        destination = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_GRF, true, CName::NONE);
    }

    emitBallotUniform(inst, &destination, disableHelperLanes);

    if (destination != m_destination)
    {
        m_encoder->Cast(m_destination, destination);
        m_encoder->Push();
    }
    if (disableHelperLanes)
    {
        ResetVMask();
    }
}

void EmitPass::emitBallotUniform(llvm::GenIntrinsicInst* inst, CVariable** destination, bool disableHelperLanes)
{
    IGC_ASSERT_MESSAGE((*destination)->IsUniform(), "Unsupported: dst must be uniform");

    bool uniform_active_lane = false;
    if (ConstantInt * pConst = dyn_cast<ConstantInt>(inst->getOperand(0)))
    {
        if (pConst->getZExtValue() == 1)
            uniform_active_lane = true;
    }


    if (!m_currShader->InsideDivergentCF(inst))
    {
        CVariable* f0 = GetSymbol(inst->getOperand(0));

        if (m_currShader->m_State.m_dispatchSize == SIMDMode::SIMD8 && m_currShader->HasFullDispatchMask())
        {
            // for SIMD8 make sure the higher 8 bits of the flag are not copied
            *destination = m_currShader->GetNewVariable(1, ISA_TYPE_UB, EALIGN_BYTE, true, CName::NONE);
        }
        m_encoder->BoolToInt(*destination, f0);
        if (!m_currShader->HasFullDispatchMask())
        {
            CVariable* dispatchMask = m_currShader->GetNewAlias(
                m_currShader->GetSR0(),
                ISA_TYPE_UD,
                (m_pattern->NeedVMask() && !disableHelperLanes ? 3 : 2) * SIZE_DWORD,
                1);
            m_encoder->And(*destination, dispatchMask, *destination);
        }
    }
    else
    {
        CVariable* exeMask = GetExecutionMask();
        if (!uniform_active_lane)
        {
            // (W)     and (1|M0)   r1.0:ud r0.0<0;1;0>:ud f0.0:uw
            CVariable* f0 = GetSymbol(inst->getOperand(0));
            CVariable* vf0 = m_currShader->GetNewVariable(
                1, ISA_TYPE_UD, EALIGN_GRF, true, CName::NONE);
            m_encoder->SetSimdSize(SIMDMode::SIMD1);
            m_encoder->SetNoMask();
            m_encoder->BoolToInt(vf0, f0);
            m_encoder->Push();

            m_encoder->SetSimdSize(SIMDMode::SIMD1);
            m_encoder->SetNoMask();
            m_encoder->And(*destination, exeMask, vf0);
            m_encoder->Push();
        }
        else
        {
            m_encoder->Cast(*destination, exeMask);
            m_encoder->Push();
        }
    }
}

void EmitPass::emitWaveClusteredBallot(llvm::GenIntrinsicInst* inst)
{
    IGC_ASSERT_MESSAGE(!m_destination->IsUniform(), "Unsupported: dst must be non-uniform");

    IGC_ASSERT_MESSAGE(isa<llvm::ConstantInt>(inst->getOperand(1)), "Unsupported: cluster size must be constant");
    const unsigned int clusterSize = int_cast<uint32_t>(cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue());

    IGC_ASSERT_MESSAGE(clusterSize <= numLanes(m_currShader->m_State.m_dispatchSize), "cluster size must be smaller or equal to SIMD");
    IGC_ASSERT_MESSAGE(clusterSize % 8 == 0, "cluster size must be 8/16/32");

    bool disableHelperLanes = int_cast<int>(cast<ConstantInt>(inst->getArgOperand(2))->getSExtValue()) == 2;
    if (disableHelperLanes)
    {
        ForceDMask();
    }

    // Run ballot.
    CVariable* ballotResult = m_currShader->GetNewVariable(1, ISA_TYPE_UD, EALIGN_GRF, true, "ballotResult");
    emitBallotUniform(inst, &ballotResult, disableHelperLanes);

    // In case cluster takes full SIMD size, then just propagate result.
    if (clusterSize == numLanes(m_currShader->m_State.m_dispatchSize))
    {
        m_encoder->Copy(m_destination, ballotResult);
        if (m_currShader->m_numberInstance > 1)
        {
            m_encoder->SetSecondHalf(true);
            m_encoder->Copy(m_destination, ballotResult);
            m_encoder->SetSecondHalf(false);
        }
        m_encoder->Push();
        return;
    }

    // ballotResult contains result from all lanes. Cluster can be either 8 or 16 lanes, so clusters in
    // ballotResult are byte-aligned. Extract clusters from the result.

    CVariable* zero = m_currShader->ImmToVariable(0, ISA_TYPE_UD);
    m_encoder->Copy(m_destination, zero);
    if (m_currShader->m_numberInstance > 1)
    {
        m_encoder->SetSecondHalf(true);
        m_encoder->Copy(m_destination, zero);
        m_encoder->SetSecondHalf(false);
    }
    m_encoder->Push();

    if (clusterSize == 8)
    {
        CVariable* ballotAlias = m_currShader->GetNewAlias(ballotResult, ISA_TYPE_B, 0, 4, false);
        CVariable* dstAlias = m_currShader->GetNewAlias(m_destination, ISA_TYPE_B, 0, numLanes(m_currShader->m_SIMDSize) * 4);

        m_encoder->SetSrcRegion(0, 1, 8, 0);
        m_encoder->SetDstRegion(4);
        m_encoder->Copy(dstAlias, ballotAlias);
        if (m_currShader->m_numberInstance > 1)
        {
            m_encoder->SetSecondHalf(true);
            m_encoder->SetSrcSubReg(0, 2);
            m_encoder->Copy(dstAlias, ballotAlias);
            m_encoder->SetSecondHalf(false);
        }
        m_encoder->Push();
    }
    else if (clusterSize == 16)
    {
        CVariable* ballotAlias = m_currShader->GetNewAlias(ballotResult, ISA_TYPE_UW, 0, 2, false);
        CVariable* dstAlias = m_currShader->GetNewAlias(m_destination, ISA_TYPE_UW, 0, numLanes(m_currShader->m_SIMDSize) * 2);

        m_encoder->SetSrcRegion(0, 1, 16, 0);
        m_encoder->SetDstRegion(2);
        m_encoder->Copy(dstAlias, ballotAlias);
        if (m_currShader->m_numberInstance > 1)
        {
            m_encoder->SetSecondHalf(true);
            m_encoder->SetSrcSubReg(0, 1);
            m_encoder->Copy(dstAlias, ballotAlias);
            m_encoder->SetSecondHalf(false);
        }
        m_encoder->Push();
    }

    if (disableHelperLanes)
    {
        ResetVMask();
    }
}

void EmitPass::emitWaveInverseBallot(llvm::GenIntrinsicInst* inst)
{
    bool disableHelperLanes = int_cast<int>(cast<ConstantInt>(inst->getArgOperand(1))->getSExtValue()) == 2;
    CVariable* Mask = GetSymbol(inst->getOperand(0));

    if (Mask->IsUniform())
    {
        if (m_encoder->IsSecondHalf())
            return;

        if (disableHelperLanes)
        {
            ForceDMask();
        }
        m_encoder->SetP(m_destination, Mask);
        if (disableHelperLanes)
        {
            ResetVMask();
        }
        return;
    }
    if (disableHelperLanes)
    {
        ForceDMask();
    }

    // The uniform case should by far be the most common.  Otherwise,
    // fall back and compute:
    //
    // (val & (1 << id)) != 0
    CVariable* Temp = m_currShader->GetNewVariable(
        numLanes(m_currShader->m_SIMDSize), ISA_TYPE_UD, EALIGN_GRF, CName::NONE);

    m_currShader->GetSimdOffsetBase(Temp);
    m_encoder->Shl(Temp, m_currShader->ImmToVariable(1, ISA_TYPE_UD), Temp);
    m_encoder->And(Temp, Mask, Temp);
    m_encoder->Cmp(EPREDICATE_NE,
        m_destination, Temp, m_currShader->ImmToVariable(0, ISA_TYPE_UD));
    if (disableHelperLanes)
    {
        ResetVMask();
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
        IGC_ASSERT_MESSAGE(ty->isIntegerTy(), "expect integer type");
        auto width = dyn_cast<IntegerType>(ty)->getBitWidth();
        IGC_ASSERT(width == 8 || width == 16 || width == 32 || width == 64);
        if (isSigned)
        {
            return width == 64 ? ISA_TYPE_Q : (width == 16 ? ISA_TYPE_W : (width == 8 ? ISA_TYPE_B : ISA_TYPE_D));
        }
        else
        {
            return width == 64 ? ISA_TYPE_UQ : (width == 16 ? ISA_TYPE_UW : (width == 8 ? ISA_TYPE_UB : ISA_TYPE_UD));
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
        case ISA_TYPE_B:
            return std::numeric_limits<int8_t>::max();
        case ISA_TYPE_UB:
            return std::numeric_limits<uint8_t>::max();
        case ISA_TYPE_W:
            return std::numeric_limits<int16_t>::max();
        case ISA_TYPE_UW:
            return std::numeric_limits<uint16_t>::max();
        case ISA_TYPE_Q:
            return std::numeric_limits<int64_t>::max();
        case ISA_TYPE_UQ:
            return std::numeric_limits<uint64_t>::max();
        default:
            IGC_ASSERT_MESSAGE(0, "unexpected visa type");
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
        case ISA_TYPE_B:
            return std::numeric_limits<int8_t>::min();
        case ISA_TYPE_UB:
            return std::numeric_limits<uint8_t>::min();
        case ISA_TYPE_W:
            return std::numeric_limits<int16_t>::min();
        case ISA_TYPE_UW:
            return std::numeric_limits<uint16_t>::min();
        case ISA_TYPE_Q:
            return std::numeric_limits<int64_t>::min();
        case ISA_TYPE_UQ:
            return std::numeric_limits<uint64_t>::min();
        default:
            IGC_ASSERT_MESSAGE(0, "unexpected visa type");
            return std::numeric_limits<int>::min();
        }
    };

    switch (op)
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
        identity = cast<IntegerType>(opndTy)->getBitMask();
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
        identity = cast<ConstantFP>(ConstantFP::getInfinity(opndTy))->getValueAPF().bitcastToAPInt().getZExtValue();
        break;
    case WaveOps::FMAX:
        opcode = EOPCODE_MAX;
        type = getISAType(opndTy);
        identity = cast<ConstantFP>(ConstantFP::getInfinity(opndTy, true))->getValueAPF().bitcastToAPInt().getZExtValue();
        break;
    default:
        IGC_ASSERT(0);
    }
}

void EmitPass::emitWavePrefix(WavePrefixIntrinsic* I)
{
    auto helperLanes = int_cast<int>( cast<ConstantInt>( I->getArgOperand( 4 ) )->getSExtValue() );
    bool disableHelperLanes = (helperLanes == 2);
    bool getFullPrefix = ( helperLanes == 4 );

    if (disableHelperLanes)
    {
        ForceDMask();
    }
    Value* Mask = I->getMask();
    if (auto * CI = dyn_cast<ConstantInt>(Mask))
    {
        // If the mask is all set, then we just pass a null
        // mask to emitScan() indicating we don't want to
        // emit any predication.
        if (CI->isAllOnesValue())
            Mask = nullptr;
    }
    m_encoder->SetSubSpanDestination(false);
    emitScan(
        I->getSrc(), I->getOpKind(), I->isInclusiveScan(), Mask, false, getFullPrefix );
    if (disableHelperLanes)
    {
        ResetVMask();
    }
}

void EmitPass::emitQuadPrefix(QuadPrefixIntrinsic* I)
{
    emitScan(
        I->getSrc(), I->getOpKind(), I->isInclusiveScan(), nullptr, true);
}

void EmitPass::emitScan(
    Value* Src, IGC::WaveOps Op,
    bool isInclusiveScan, Value* Mask, bool isQuad, bool noMask)
{
    VISA_Type type;
    e_opcode opCode;
    uint64_t identity = 0;
    GetReductionOp(Op, Src->getType(), identity, opCode, type);
    CVariable* src = GetSymbol(Src);
    CVariable* dst[2] = { nullptr, nullptr };
    CVariable* Flag = Mask ? GetSymbol(Mask) : nullptr;

    emitPreOrPostFixOp(
        opCode, identity, type,
        false, src, dst, Flag,
        !isInclusiveScan, isQuad);

    if(noMask)
    {
        m_encoder->SetNoMask();
    }
    else
    {
        // Now that we've computed the result in temporary registers,
        // make sure we only write the results to lanes participating in the
        // scan as specified by 'mask'.
        if (Flag)
            m_encoder->SetPredicate(Flag);
    }

    m_encoder->Copy(m_destination, dst[0]);
    if (m_currShader->m_numberInstance == 2)
    {
        m_encoder->SetSecondHalf(true);
        m_encoder->Copy(m_destination, dst[1]);
    }
    m_encoder->Push();
}

void EmitPass::emitWaveClusteredPrefix(GenIntrinsicInst* I)
{
    auto helperLanes = int_cast<int>(cast<ConstantInt>(I->getArgOperand(3))->getSExtValue());
    bool disableHelperLanes = (helperLanes == 2);

    IGC_ASSERT_MESSAGE(isa<llvm::ConstantInt>(I->getOperand(2)), "Unsupported: cluster size must be constant");
    const unsigned int clusterSize = int_cast<uint32_t>(cast<llvm::ConstantInt>(I->getOperand(2))->getZExtValue());

    IGC_ASSERT_MESSAGE(clusterSize <= numLanes(m_currShader->m_State.m_dispatchSize), "Cluster size must be smaller or equal to SIMD");
    IGC_ASSERT_MESSAGE(clusterSize == 8 || clusterSize == 16 || clusterSize == 32, "Cluster size must be 8/16/32");

    IGC::WaveOps Op = static_cast<IGC::WaveOps>(I->getImm64Operand(1));
    IGC_ASSERT_MESSAGE(Op == IGC::WaveOps::SUM || Op == IGC::WaveOps::FSUM, "Unsupported op type");

    if (disableHelperLanes)
    {
        ForceDMask();
    }

    Value* Src = I->getOperand(0);

    if (clusterSize == numLanes(m_currShader->m_State.m_dispatchSize))
    {
        // If cluster size is equal to SIMD size, just run normal scan.
        emitScan(Src, Op, false, nullptr, false);
    }
    else
    {
        // Run scan with clusters.

        VISA_Type type;
        e_opcode opCode;
        uint64_t identity = 0;
        GetReductionOp(Op, Src->getType(), identity, opCode, type);

        IGC_ASSERT_MESSAGE((CEncoder::GetCISADataTypeSize(type) == 8 && ScanReduceIsInt64EmulationNeeded(opCode, type)) == false,
            "Unsupported: 64b data type");

        CVariable* src = GetSymbol(Src);
        CVariable* dst[2] = { nullptr, nullptr };

        emitPreOrPostFixOp(
            opCode, identity, type,
            false, src, dst, nullptr,
            true, false, clusterSize);

        m_encoder->Copy(m_destination, dst[0]);
        if (m_currShader->m_numberInstance == 2)
        {
            m_encoder->SetSecondHalf(true);
            m_encoder->Copy(m_destination, dst[1]);
            m_encoder->SetSecondHalf(false);
        }
        m_encoder->Push();
    }

    if (disableHelperLanes)
    {
        ResetVMask();
    }
}

void EmitPass::emitWaveAll(llvm::GenIntrinsicInst* inst)
{
    bool disableHelperLanes = int_cast<int>(cast<ConstantInt>(inst->getArgOperand(2))->getSExtValue()) == 2;
    if (disableHelperLanes)
    {
        ForceDMask();
    }
    m_encoder->SetSubSpanDestination( false );
    CVariable* src = GetSymbol(inst->getOperand(0));
    CVariable* dst = m_destination;
    const WaveOps op = static_cast<WaveOps>(cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue());
    VISA_Type type;
    e_opcode opCode;
    uint64_t identity = 0;
    if( inst->getOperand( 0 )->getType()->isVectorTy() )
    {
        // Joint Reduction optimzation from multiple consecutive independent wave ops, can construct wider reduction tree
        GetReductionOp( op, cast<VectorType>( inst->getOperand( 0 )->getType() )->getElementType(), identity, opCode, type );

        if( m_currShader->m_State.m_dispatchSize == SIMDMode::SIMD32 && m_currShader->m_numberInstance > 1 )
        {
            // Dual SIMD16 mode, use 1 SIMD16 inst per reduction for first layer to reduce 32 elements down to 16
            CVariable* reduceSrc = m_currShader->GetNewVariable( src->GetNumberElement(), type, src->GetAlign(), CName( CName( "reduceSrc_" ), src->getName().getCString() ) );
            CVariable* reduceSrcSecondHalf = m_currShader->GetNewVariable( src->GetNumberElement(), type, src->GetAlign(), CName( CName( "reduceSrcSecondHalf_" ), src->getName().getCString() ) );

            const bool isInt64Mul = ScanReduceIsInt64Mul( opCode, type );
            const bool int64EmulationNeeded = ScanReduceIsInt64EmulationNeeded( opCode, type );

            // Explicitly generate First layer (Technically 0th layer since no operations are joint yet, we are still operating within a single reduction op)
            for( uint16_t i = 0; i < dst->GetNumberElement(); i++ )
            {
                // Prepare reduceSrc
                CVariable* srcAlias = m_currShader->GetNewAlias( src, type, i * numLanes( m_currShader->m_SIMDSize ) * m_encoder->GetCISADataTypeSize( type ), numLanes( m_currShader->m_SIMDSize ) );
                CVariable* reduceSrcAlias = m_currShader->GetNewAlias( reduceSrc, type, i * numLanes( m_currShader->m_SIMDSize ) * m_encoder->GetCISADataTypeSize( type ), numLanes( m_currShader->m_SIMDSize ) );
                ScanReducePrepareSrc( type, identity, false, false, srcAlias, reduceSrcAlias );

                // Prepare reduceSrcSecondHalf
                CVariable* srcSecondHalfAlias = m_currShader->GetNewAlias( src, type, i * numLanes( m_currShader->m_SIMDSize ) * m_encoder->GetCISADataTypeSize( type ), numLanes( m_currShader->m_SIMDSize ) );
                CVariable* reduceSrcSecondHalfAlias = m_currShader->GetNewAlias( reduceSrcSecondHalf, type, i * numLanes( m_currShader->m_SIMDSize ) * m_encoder->GetCISADataTypeSize( type ), numLanes( m_currShader->m_SIMDSize ) );
                ScanReducePrepareSrc( type, identity, false, true, srcSecondHalfAlias, reduceSrcSecondHalfAlias );

                // Emit correct operations
                if( !int64EmulationNeeded )
                {
                    m_encoder->SetNoMask();
                    m_encoder->SetSimdSize( SIMDMode::SIMD16 );
                    m_encoder->GenericAlu( opCode, reduceSrcAlias, reduceSrcAlias, reduceSrcSecondHalfAlias );
                    m_encoder->Push();
                }
                else
                {
                    if( isInt64Mul )
                    {
                        CVariable* tmpMulSrc[ 2 ] = { reduceSrcAlias, reduceSrcSecondHalfAlias };
                        Mul64( reduceSrcAlias, tmpMulSrc, SIMDMode::SIMD16, true );
                    }
                    else
                    {
                        IGC_ASSERT_MESSAGE( 0, "Unsupported" );
                    }
                }
            }

            // Now that 32 elements per reduction have been reduced to 16 in layer 0, can proceed with regular reduction tree implementation using SIMD16
            emitReductionTrees( opCode, type, SIMDMode::SIMD16, reduceSrc, dst, 0, dst->GetNumberElement() - 1 );
        }
        else
        {
            CVariable* reduceSrc = m_currShader->GetNewVariable( src->GetNumberElement(), type, src->GetAlign(), CName( CName( "reduceSrc_" ), src->getName().getCString() ) );
            // Prepare reduceSrc for all elements
            for( int i = 0; i < dst->GetNumberElement(); i++ )
            {
                CVariable* srcAlias = m_currShader->GetNewAlias( src, type, i * numLanes( m_currShader->m_SIMDSize ) * m_encoder->GetCISADataTypeSize( type ), numLanes( m_currShader->m_SIMDSize ) );
                CVariable* reduceSrcAlias = m_currShader->GetNewAlias( reduceSrc, type, i * numLanes( m_currShader->m_SIMDSize ) * m_encoder->GetCISADataTypeSize( type ), numLanes( m_currShader->m_SIMDSize ) );
                ScanReducePrepareSrc( type, identity, false, false, srcAlias, reduceSrcAlias );
            }

            emitReductionTrees( opCode, type, m_currShader->m_State.m_dispatchSize, reduceSrc, dst, 0, dst->GetNumberElement() - 1 );
        }
    }
    else
    {
        // Single WaveAll, emit base reduction tree
        GetReductionOp( op, inst->getOperand( 0 )->getType(), identity, opCode, type );
        emitReductionAll( opCode, identity, type, false, src, dst );
    }

    if (disableHelperLanes)
    {
        ResetVMask();
    }
}

void EmitPass::emitWaveClustered(llvm::GenIntrinsicInst* inst)
{
    bool disableHelperLanes = int_cast<int>(cast<ConstantInt>(inst->getArgOperand(3))->getSExtValue()) == 2;
    if (disableHelperLanes)
    {
        ForceDMask();
    }
    CVariable* src = GetSymbol(inst->getOperand(0));
    const WaveOps op = static_cast<WaveOps>(cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue());
    const unsigned int clusterSize = int_cast<uint32_t>(cast<llvm::ConstantInt>(inst->getOperand(2))->getZExtValue());
    VISA_Type type;
    e_opcode opCode;
    uint64_t identity = 0;
    GetReductionOp(op, inst->getOperand(0)->getType(), identity, opCode, type);
    CVariable *dst = m_destination;
    m_encoder->SetSubSpanDestination(false);
    emitReductionClustered(opCode, identity, type, false, clusterSize, src, dst);
    if (disableHelperLanes)
    {
        ResetVMask();
    }
}

void EmitPass::emitWaveInterleave(llvm::GenIntrinsicInst* inst)
{
    bool disableHelperLanes = int_cast<int>(cast<ConstantInt>(inst->getArgOperand(3))->getSExtValue()) == 2;
    if (disableHelperLanes)
    {
        ForceDMask();
    }
    CVariable* src = GetSymbol(inst->getOperand(0));
    const WaveOps op = static_cast<WaveOps>(cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue());
    const unsigned int step = int_cast<uint32_t>(cast<llvm::ConstantInt>(inst->getOperand(2))->getZExtValue());
    VISA_Type type;
    e_opcode opCode;
    uint64_t identity = 0;
    GetReductionOp(op, inst->getOperand(0)->getType(), identity, opCode, type);
    CVariable* dst = m_destination;
    m_encoder->SetSubSpanDestination(false);
    emitReductionInterleave(opCode, identity, type, false, step, src, dst);
    if (disableHelperLanes)
    {
        ResetVMask();
    }
}

void EmitPass::emitWaveClusteredInterleave(llvm::GenIntrinsicInst* inst)
{
    bool disableHelperLanes = int_cast<int>(cast<ConstantInt>(inst->getArgOperand(3))->getSExtValue()) == 2;
    if (disableHelperLanes)
    {
        ForceDMask();
    }
    CVariable* src = GetSymbol(inst->getOperand(0));
    const WaveOps op = static_cast<WaveOps>(cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue());
    const unsigned int clusterSize = int_cast<uint32_t>(cast<llvm::ConstantInt>(inst->getOperand(2))->getZExtValue());
    const unsigned int interleaveStep = int_cast<uint32_t>(cast<llvm::ConstantInt>(inst->getOperand(3))->getZExtValue());
    VISA_Type type;
    e_opcode opCode;
    uint64_t identity = 0;
    GetReductionOp(op, inst->getOperand(0)->getType(), identity, opCode, type);
    CVariable* dst = m_destination;
    m_encoder->SetSubSpanDestination(false);
    emitReductionClusteredInterleave(opCode, identity, type, false, clusterSize, interleaveStep, src, dst);
    if (disableHelperLanes)
    {
        ResetVMask();
    }
}

void EmitPass::emitDP4A(GenIntrinsicInst* GII, const SSource* Sources, const DstModifier& modifier, bool isAccSigned) {
    GenISAIntrinsic::ID GIID = GII->getIntrinsicID();
    CVariable* dst = m_destination;
    CVariable *src0, *src1, *src2;

    // Check if Sources was set in PatternMatch
    if (!Sources)
    {
        src0 = GetSymbol(GII->getOperand(0));
        src1 = GetSymbol(GII->getOperand(1));
        src2 = GetSymbol(GII->getOperand(2));
    }
    else
    {
        m_encoder->SetSrcRegion(1, Sources[1].region[0], Sources[1].region[1], Sources[1].region[2]);
        src0 = GetSrcVariable(Sources[0]);
        src1 = GetSrcVariable(Sources[1]);
        src2 = GetSrcVariable(Sources[2]);
    }

    // Set correct signedness of src0 and dst.
    if (modifier.sat)
    {
        if (isAccSigned)
        {
            dst = m_currShader->BitCast(dst, GetSignedType(dst->GetType()));
            src0 = m_currShader->BitCast(src0, GetSignedType(src0->GetType()));
        }
        else
        {
            dst = m_currShader->BitCast(dst, GetUnsignedType(dst->GetType()));
            src0 = m_currShader->BitCast(src0, GetUnsignedType(src0->GetType()));
        }
    }

    // Set correct signedness of src1.
    if (GIID == GenISAIntrinsic::GenISA_dp4a_ss ||
        GIID == GenISAIntrinsic::GenISA_dp4a_su)
        src1 = m_currShader->BitCast(src1, GetSignedType(src1->GetType()));
    if (GIID == GenISAIntrinsic::GenISA_dp4a_uu ||
        GIID == GenISAIntrinsic::GenISA_dp4a_us)
        src1 = m_currShader->BitCast(src1, GetUnsignedType(src1->GetType()));

    // Set correct signedness of src2.
    if (GIID == GenISAIntrinsic::GenISA_dp4a_ss ||
        GIID == GenISAIntrinsic::GenISA_dp4a_us)
        src2 = m_currShader->BitCast(src2, GetSignedType(src2->GetType()));
    if (GIID == GenISAIntrinsic::GenISA_dp4a_uu ||
        GIID == GenISAIntrinsic::GenISA_dp4a_su)
        src2 = m_currShader->BitCast(src2, GetUnsignedType(src2->GetType()));

    // Emit dp4a.
    m_encoder->SetDstModifier(modifier);
    m_encoder->dp4a(dst, src0, src1, src2);
    m_encoder->Push();
}

void EmitPass::emitUnmaskedRegionBoundary(bool start)
{
    if (start) {
      m_encoder->BeginForcedNoMaskRegion();
    } else {
      m_encoder->EndForcedNoMaskRegion();
    }
}

void EmitPass::emitDebugPlaceholder(llvm::GenIntrinsicInst* I)
{
    m_encoder->Loc(I->getDebugLoc().getLine());
    m_encoder->DebugLinePlaceholder();
}

// Dummy instruction that won't be optimized away.
void EmitPass::emitDummyInst(llvm::GenIntrinsicInst* GII)
{
    CVariable* dst = m_currShader->GetNULL();
    CVariable* src = m_currShader->GetR0();
    m_encoder->Copy(dst, src);
    m_encoder->Push();
}
void EmitPass::emitLaunder(GenIntrinsicInst* GII)
{
    CVariable* src = GetSymbol(GII->getArgOperand(0));
    m_encoder->Copy(m_destination, src);
    m_encoder->Push();
}

void EmitPass::emitImplicitArgIntrinsic(llvm::GenIntrinsicInst* I)
{
    Function* parentFunc = I->getParent()->getParent();
    MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    // We can just drop the intrinsic if there are no uses for it.
    // It should have been lowered in LowerImplicitArgIntrinsics pass, but did not get cleaned up.
    if (I->use_empty()) return;

    Function* groupHead = nullptr;
    if (!m_FGA || m_FGA->isGroupHead(parentFunc)) {
        groupHead = parentFunc;
    }
    else {
        groupHead = m_FGA->getSubGroupMap(parentFunc);
    }

    if (isEntryFunc(pMdUtils, groupHead))
    {
        // Map to the root kernel's implicit arg symbol
        ImplicitArgs IAS(*groupHead, pMdUtils);
        ImplicitArg::ArgType IAtype = ImplicitArgs::getArgType(I->getIntrinsicID());
        Argument* arg = IAS.getImplicitArg(*groupHead, IAtype);
        IGC_ASSERT_MESSAGE(arg, "Implicit argument not found!");
        CVariable* Src = m_currShader->getOrCreateArgumentSymbol(arg, false);
        CVariable* Dst = GetSymbol(I);

        if (IAtype == ImplicitArg::ArgType::R0 ||
            IAtype == ImplicitArg::ArgType::PAYLOAD_HEADER ||
            IAtype == ImplicitArg::ArgType::WORK_DIM ||
            IAtype == ImplicitArg::ArgType::NUM_GROUPS ||
            IAtype == ImplicitArg::ArgType::GLOBAL_SIZE ||
            IAtype == ImplicitArg::ArgType::LOCAL_SIZE ||
            IAtype == ImplicitArg::ArgType::ENQUEUED_LOCAL_WORK_SIZE ||
            IAtype == ImplicitArg::ArgType::CONSTANT_BASE ||
            IAtype == ImplicitArg::ArgType::GLOBAL_BASE ||
            IAtype == ImplicitArg::ArgType::PRIVATE_BASE ||
            IAtype == ImplicitArg::ArgType::PRINTF_BUFFER)
        {
            // Map directly to the kernel's arguments
            m_currShader->UpdateSymbolMap(I, Src);
        }
        else
        {
            // Otherwise copy the kernel arg value into the intrinsic variable
            emitCopyAll(Dst, Src, I->getType());
        }
    }
    else if (groupHead->hasFnAttribute("visaStackCall"))
    {
        if (I->getIntrinsicID() == GenISAIntrinsic::ID::GenISA_getR0)
        {
            // The R0 intrinsic will not be lowered by LowerImplicitArgIntrinsics.
            // Map to the predefined R0 register
            m_currShader->GetEncoder().GetVISAPredefinedVar(GetSymbol(I), PREDEFINED_R0);
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "Intrinsics used in stackcalls has not been lowered!");
        }
    }
}

void EmitPass::emitStoreImplBufferPtr(llvm::GenIntrinsicInst* I)
{
    if (m_currShader->HasStackCalls() &&
        !m_encoder->IsSecondHalf())
        m_currShader->CopyVariable(m_currShader->GetImplArgBufPtr(), GetSymbol(I->getArgOperand(0)));
}

void EmitPass::emitSetStackCallsBaseAddress(llvm::GenIntrinsicInst* I)
{
    InitializeKernelStack(I->getFunction(), GetSymbol(I->getArgOperand(0)));
}

void EmitPass::emitSaveInReservedArgSpace(llvm::SaveInReservedArgSpaceIntrinsic* I)
{
    if (m_encoder->IsSecondHalf())
        return;

    m_encoder->Copy(
        m_currShader->createAliasIfNeeded(I->getData(), m_currShader->GetARGVReservedVariable(I->getSlot())),
        GetSymbol(I->getData())
    );
    m_encoder->Push();
}

void EmitPass::emitReadFromReservedArgSpace(llvm::ReadFromReservedArgSpaceIntrinsic* I)
{
    if (m_encoder->IsSecondHalf())
        return;

    m_encoder->Copy(
        m_destination,
        m_currShader->createAliasIfNeeded(I, m_currShader->GetARGVReservedVariable(I->getSlot()))
    );
    m_encoder->Push();
}

void EmitPass::emitStoreLocalIdBufferPtr(llvm::GenIntrinsicInst* I)
{
    if (m_currShader->HasStackCalls() &&
        !m_encoder->IsSecondHalf())
        m_currShader->CopyVariable(m_currShader->GetLocalIdBufPtr(), GetSymbol(I->getArgOperand(0)));
}

void EmitPass::emitStoreGlobalBufferArg(llvm::GenIntrinsicInst* I)
{
    CVariable* GlobalBufferArg = m_currShader->GetGlobalBufferArg();
    IGC_ASSERT(GlobalBufferArg);
    if(m_currShader->HasStackCalls() &&
        !m_encoder->IsSecondHalf())
        m_currShader->CopyVariable(GlobalBufferArg, GetSymbol(I->getArgOperand(0)));
}

void EmitPass::emitLoadImplBufferPtr(llvm::GenIntrinsicInst* I)
{
    m_encoder->SetUniformSIMDSize(lanesToSIMDMode(1));
    m_encoder->SetNoMask();
    m_encoder->SetSrcSubReg(0, 0);
    m_encoder->Copy(m_destination, m_currShader->GetImplArgBufPtr());
    m_encoder->Push();
}

void EmitPass::emitLoadLocalIdBufferPtr(llvm::GenIntrinsicInst* I)
{
    m_encoder->SetUniformSIMDSize(lanesToSIMDMode(1));
    m_encoder->SetNoMask();
    m_encoder->SetSrcSubReg(0, 0);
    m_encoder->Copy(m_destination, m_currShader->GetLocalIdBufPtr());
    m_encoder->Push();
}

void EmitPass::emitLoadGlobalBufferArg(llvm::GenIntrinsicInst* I)
{
    CVariable* GlobalBufferArg = m_currShader->GetGlobalBufferArg();
    IGC_ASSERT(GlobalBufferArg);
    m_encoder->SetUniformSIMDSize(lanesToSIMDMode(1));
    m_encoder->SetNoMask();
    m_encoder->SetSrcSubReg(0, 0);
    m_encoder->Copy(m_destination, GlobalBufferArg);
    m_encoder->Push();
}




void EmitPass::emitDpas(GenIntrinsicInst* GII, const SSource* Sources, const DstModifier& modifier)
{
    // Note that in intrinsic's arguments, activation goes before weight;
    // But in visa (gen isa), weight goes before activation.
    CVariable* dst = m_destination;
    CVariable* activation = GetSrcVariable(Sources[1]);
    CVariable* weight = GetSrcVariable(Sources[2]);

    // input could be null if it is integer 0 or float positive 0.0f
    CVariable* input = nullptr;
    Constant* CSTVal = dyn_cast<Constant>(Sources[0].value);
    if (!(CSTVal && CSTVal->isNullValue()))
    {
        input = GetSrcVariable(Sources[0]);
    }

    // float dpas uses short as bfloat16 for either input or dst.
    ConstantInt* pa = cast<ConstantInt>(GII->getOperand(3)); // Activation's precision
    ConstantInt* pb = cast<ConstantInt>(GII->getOperand(4)); // Weight's precision
    ConstantInt* sdepth = cast<ConstantInt>(GII->getOperand(5));
    ConstantInt* rcount = cast<ConstantInt>(GII->getOperand(6));
    ConstantInt* dpasw = cast<ConstantInt>(GII->getOperand(7));
    int PA = (int)pa->getSExtValue();
    int PB = (int)pb->getSExtValue();
    int SD = (int)sdepth->getSExtValue();
    int RC = (int)rcount->getSExtValue();
    bool IsDpasw = dpasw->getValue().getBoolValue();

    activation = BroadcastIfUniform(activation);
    weight = BroadcastIfUniform(weight);
    if (input) {
        input = BroadcastIfUniform(input);
        if (input->GetType() == ISA_TYPE_UW || input->GetType() == ISA_TYPE_W)
        {
            input = m_currShader->GetNewAlias(input, ISA_TYPE_BF, 0, 0);
        }
    }
    if (dst->GetType() == ISA_TYPE_UW || dst->GetType() == ISA_TYPE_W)
    {
        dst = m_currShader->GetNewAlias(dst, ISA_TYPE_BF, 0, 0);
    }

    // Sanity: Make sure that activation and weight are D/UD always
    if (activation->GetType() != ISA_TYPE_UD && activation->GetType() != ISA_TYPE_D)
    {
        activation = m_currShader->GetNewAlias(activation, ISA_TYPE_UD, 0, 0);
    }
    if (weight->GetType() != ISA_TYPE_UD && weight->GetType() != ISA_TYPE_D)
    {
        weight = m_currShader->GetNewAlias(weight, ISA_TYPE_UD, 0, 0);
    }

    m_encoder->dpas(dst, input, weight, (PrecisionType)PB, activation, (PrecisionType)PA,
        (uint8_t)SD, (uint8_t)RC, IsDpasw);
    m_encoder->Push();
}

// Conversion between float types
void EmitPass::emitfcvt(llvm::GenIntrinsicInst* GII)
{
    /// Divide N into multiple of 16 and the remaining into 8, 4, 2, 1
    /// Each sequence takes two elements in execsizeSeq, in which first
    /// one has execsize, and the second one the starting offset.
    auto getAllExecsize = [=](SmallVector<uint32_t, 16> & execsizeSeq, uint32_t N) {
        // Max execution size is 16.
        int n = (int)N / 16;
        uint32_t offset = 0;
        for (int i = 0; i < n; ++i) {
            execsizeSeq.push_back(16);
            execsizeSeq.push_back(offset);
            offset += 16;
        }

        int m = (int)N % 16;
        for (uint32_t s = 8; m > 0; s = s / 2)
        {
            if (m >= (int)s)
            {
                execsizeSeq.push_back(s);
                execsizeSeq.push_back(offset);
                offset += s;
                m -= s;
            }
        }
    };

    Value* sVal = GII->getOperand(0);
    CVariable* src = GetSymbol(sVal);
    CVariable* dst = m_destination;

    Type* dTy = GII->getType();
    IGCLLVM::FixedVectorType* dVTy = dyn_cast<IGCLLVM::FixedVectorType>(dTy);
    Type* sTy = sVal->getType();
    IGCLLVM::FixedVectorType* sVTy = dyn_cast<IGCLLVM::FixedVectorType>(sTy);
    int nelts = dVTy ? (int)dVTy->getNumElements() : 1;
    int src_nelts = sVTy ? (int)sVTy->getNumElements() : 1;
    if (nelts != src_nelts)
    {
        IGC_ASSERT_MESSAGE(0, "Different #elements in src and dst of conversion intrinsic!");
        return;
    }

    bool isSrcUniform = src->IsUniform();
    bool isDstUniform = dst->IsUniform();
    uint16_t nsimdsize = numLanes(m_currShader->m_SIMDSize);
    GenISAIntrinsic::ID id = GII->getIntrinsicID();

    ERoundingMode FP_RM = static_cast<ERoundingMode>(
        m_pCtx->getModuleMetaData()->compOpt.FloatRoundingMode);
    if (id == GenISAIntrinsic::GenISA_ftobf) {
        ConstantInt* CI = cast<ConstantInt>(GII->getOperand(1));
        FP_RM = (ERoundingMode)CI->getZExtValue();
    }
    else if (id == GenISAIntrinsic::GenISA_2fto2bf)
    {
        ConstantInt* CI = cast<ConstantInt>(GII->getOperand(2));
        FP_RM = (ERoundingMode)CI->getZExtValue();
    }
    else if (id == GenISAIntrinsic::GenISA_hftobf8) {
        ConstantInt* CI = cast<ConstantInt>(GII->getOperand(1));
        FP_RM = (ERoundingMode)CI->getZExtValue();
    }
    else {
        FP_RM = ERoundingMode::ROUND_TO_ANY;
    }

    if (FP_RM != ERoundingMode::ROUND_TO_ANY)
        SetRoundingMode_FP(FP_RM);

    // saturation
    bool isSat = false;
    switch (id)
    {
    case GenISAIntrinsic::GenISA_hftobf8:
    {
        ConstantInt* CI = cast<ConstantInt>(GII->getOperand(2));
        isSat = CI->getValue().getBoolValue();
    }
    default:
        break;
    }

    // vISA instruction doesn't support immediate source of type BF
    if (id == GenISAIntrinsic::GenISA_bftof && src->IsImmediate())
    {
        uint32_t imm32 = ((uint32_t)src->GetImmediateValue()) & 0xFFFF;
        imm32 = imm32 << 16; // make it as float immediate
        CVariable* fSrc = m_currShader->ImmToVariable((uint64_t)imm32, ISA_TYPE_F);
        m_encoder->Copy(dst, fSrc);
        m_encoder->Push();
        return;
    }

    if (id == GenISAIntrinsic::GenISA_ftobf ||
        id == GenISAIntrinsic::GenISA_bftof ||
        id == GenISAIntrinsic::GenISA_hftobf8 ||
        id == GenISAIntrinsic::GenISA_bf8tohf ||
        id == GenISAIntrinsic::GenISA_ftotf32)
    {
        CVariable* tDst = nullptr, *tSrc = nullptr;
        if (id == GenISAIntrinsic::GenISA_ftobf) {
            tDst = m_currShader->GetNewAlias(dst, ISA_TYPE_BF, 0, 0);
            tSrc = src;
        }
        else if (id == GenISAIntrinsic::GenISA_bftof) {
            tDst = dst;
            tSrc = m_currShader->GetNewAlias(src, ISA_TYPE_BF, 0, 0);
        }
        /// Use UB as we are not exposing BF8, UD for TF32
        else if (id == GenISAIntrinsic::GenISA_hftobf8) {
            tDst = m_currShader->GetNewAlias(dst, ISA_TYPE_UB, 0, 0);
            tSrc = src;
        }
        else if (id == GenISAIntrinsic::GenISA_bf8tohf) {
            tDst = dst;
            tSrc = m_currShader->GetNewAlias(src, ISA_TYPE_UB, 0, 0);
        }
        else if (id == GenISAIntrinsic::GenISA_ftotf32) {
            tDst = m_currShader->GetNewAlias(dst, ISA_TYPE_UD, 0, 0);
            // Does not support immediate source of type float, therefore we
            // need a temporary "general" variable and copy the immediate
            // value to that temporary variable first.  Then we can use this
            // temporary as an operand of fcvt.
            if (src->IsImmediate()) {
                CVariable *tfSrc = m_currShader->GetNewVariable(
                    1, ISA_TYPE_F, EALIGN_GRF, true /*uniform*/, "tmp_cvt");
                m_encoder->Copy(tfSrc, src);
                m_encoder->Push();
                tSrc = tfSrc;
            } else {
                tSrc = src;
            }
        }
        else {
            IGC_ASSERT_EXIT_MESSAGE(0, "Something wrong in cvt!");
        }

        if (isSrcUniform && isDstUniform)
        {
            SmallVector<uint32_t, 16> insts;
            getAllExecsize(insts, nelts);
            for (int i = 0, s = (int)insts.size(); i < s; i += 2)
            {
                uint32_t esize = insts[i];
                SIMDMode simdMode = lanesToSIMDMode(esize);
                uint32_t offset = insts[i + 1];

                if (isSat) {
                    m_encoder->SetDstModifier(EMOD_SAT);
                }
                m_encoder->SetNoMask();
                m_encoder->SetUniformSIMDSize(simdMode);
                m_encoder->SetDstSubReg(offset);
                m_encoder->SetSrcSubReg(0, offset);
                // by default, uniform's region is (0, 1, 0)
                if (esize > 1) {
                    uint32_t stride = (esize >= 8 ? 8 : esize);
                    m_encoder->SetSrcRegion(0, stride, stride, 1);
                }
                if (id == GenISAIntrinsic::GenISA_hftobf8 ||
                    id == GenISAIntrinsic::GenISA_bf8tohf ||
                    id == GenISAIntrinsic::GenISA_ftotf32
                    )
                {
                    m_encoder->fcvt(tDst, tSrc);
                }
                else
                {
                    m_encoder->Cast(tDst, tSrc);
                }
                m_encoder->Push();
            }
        }
        else
        {
            uint32_t dstOff = 0, srcOff = 0;
            for (int i = 0; i < nelts; ++i)
            {
                if (isSat) {
                    m_encoder->SetDstModifier(EMOD_SAT);
                }
                m_encoder->SetDstSubReg(dstOff);
                m_encoder->SetSrcSubReg(0, srcOff);
                if (id == GenISAIntrinsic::GenISA_hftobf8 ||
                    id == GenISAIntrinsic::GenISA_bf8tohf ||
                    id == GenISAIntrinsic::GenISA_ftotf32
                   )
                {
                    switch (id)
                    {
                    case GenISAIntrinsic::GenISA_hftobf8:
                    {
                        // HW requires NoMask
                        m_encoder->SetNoMask();
                        break;
                    }
                    default:
                        break;
                    }
                    m_encoder->fcvt(tDst, tSrc);
                }
                else
                {
                    m_encoder->Cast(tDst, tSrc);
                }
                m_encoder->Push();

                dstOff += (isDstUniform ? 1 : nsimdsize);
                srcOff += (isSrcUniform ? 1 : nsimdsize);
            }
        }
    }
    else if (id == GenISAIntrinsic::GenISA_2fto2bf)
    {
        CVariable* srcs[2];
        srcs[0] = src;
        srcs[1] = GetSymbol(GII->getOperand(1));
        CVariable* tDst = m_currShader->GetNewAlias(dst, ISA_TYPE_BF, 0, 0);
        SmallVector<uint32_t, 16> insts;
        getAllExecsize(insts, nelts);
        for (int e = 0; e < 2; ++e)
        {
            CVariable* tSrc = srcs[e];
            isSrcUniform = tSrc->IsUniform();
            if (isSrcUniform && isDstUniform)
            {
                for (int i = 0, s = (int)insts.size(); i < s; i += 2)
                {
                    uint32_t esize = insts[i];
                    SIMDMode simdMode = lanesToSIMDMode(esize);
                    uint32_t offset = insts[i + 1];

                    if (isSat) {
                        m_encoder->SetDstModifier(EMOD_SAT);
                    }
                    m_encoder->SetNoMask();
                    m_encoder->SetUniformSIMDSize(simdMode);
                    m_encoder->SetDstSubReg(2 * offset + e);
                    m_encoder->SetDstRegion(2);
                    m_encoder->SetSrcSubReg(0, offset);
                    // by default, uniform's region is (0, 1, 0)
                    if (esize > 1) {
                        uint32_t stride = (esize >= 8 ? 8 : esize);
                        m_encoder->SetSrcRegion(0, stride, stride, 1);
                    }
                    m_encoder->Cast(tDst, tSrc);
                    m_encoder->Push();
                }
            }
            else
            {
                uint32_t dstOff = 0, srcOff = 0;
                for (int i = 0; i < nelts; ++i)
                {
                    if (isSat) {
                        m_encoder->SetDstModifier(EMOD_SAT);
                    }
                    m_encoder->SetDstSubReg(2 * dstOff + e);
                    m_encoder->SetDstRegion(2);
                    m_encoder->SetSrcSubReg(0, srcOff);
                    m_encoder->Cast(tDst, tSrc);
                    m_encoder->Push();

                    dstOff += (isDstUniform ? 1 : nsimdsize);
                    srcOff += (isSrcUniform ? 1 : nsimdsize);
                }
            }
        }
    }
    else
    {
        IGC_ASSERT_MESSAGE(0, "ICE: unhandled gen intrinsic within cvt!");
    }

    if (FP_RM != ERoundingMode::ROUND_TO_ANY) {
        ResetRoundingMode(GII);
    }
}

void EmitPass::emitSrnd(llvm::GenIntrinsicInst* GII)
{
    CVariable* dst = m_destination;
    CVariable* src0 = GetSymbol(GII->getOperand(0));
    CVariable* src1 = GetSymbol(GII->getOperand(1));
    ConstantInt* CI = cast<ConstantInt>(GII->getOperand(2));
    bool isSat = CI->getValue().getBoolValue();
    GenISAIntrinsic::ID GID = GII->getIntrinsicID();

    // set dst types
    if (GID == GenISAIntrinsic::GenISA_srnd_ftohf)
    {
        if (dst->GetType() != ISA_TYPE_HF)
            dst = m_currShader->GetNewAlias(dst, ISA_TYPE_HF, 0, 0);
    }
    if (GID == GenISAIntrinsic::GenISA_srnd_hftobf8
        )
    {
        if (dst->GetType() != ISA_TYPE_UB) // Use UB for bf8
            dst = m_currShader->GetNewAlias(dst, ISA_TYPE_UB, 0, 0);
    }

    // set src0 types
    if (GID == GenISAIntrinsic::GenISA_srnd_ftohf)
    {
        if (src0->GetType() != ISA_TYPE_F)
            src0 = m_currShader->GetNewAlias(src0, ISA_TYPE_F, 0, 0);
    }
    if (GID == GenISAIntrinsic::GenISA_srnd_hftobf8
        )
    {
        if (src0->GetType() != ISA_TYPE_HF)
            src0 = m_currShader->GetNewAlias(src0, ISA_TYPE_HF, 0, 0);
    }

    // set src1 types
    if (!src1->IsImmediate())
    {
        if (src1->GetType() == ISA_TYPE_W)
            src1 = m_currShader->GetNewAlias(src1, ISA_TYPE_UW, 0, 0);
        if (src1->GetType() == ISA_TYPE_B)
            src1 = m_currShader->GetNewAlias(src1, ISA_TYPE_UB, 0, 0);
    }

    uint16_t nsimdsize = numLanes(m_currShader->m_SIMDSize);
    Type* dTy = GII->getType();
    IGCLLVM::FixedVectorType* dVTy = dyn_cast<IGCLLVM::FixedVectorType>(dTy);
    int nelts = dVTy ? (int)dVTy->getNumElements() : 1;
    if (dst->IsUniform() && src0->IsUniform() && src1->IsUniform())
    {
        SmallVector<uint32_t, 16> insts;
        splitIntoPowerOfTwo(insts, nelts, nsimdsize);
        for (int i = 0, s = (int)insts.size(); i < s; i += 2)
        {
            uint32_t esize = insts[i];
            SIMDMode simdMode = lanesToSIMDMode(esize);
            uint32_t offset = insts[i + 1];

            if (isSat) {
                m_encoder->SetDstModifier(EMOD_SAT);
            }
            m_encoder->SetNoMask();
            m_encoder->SetUniformSIMDSize(simdMode);
            m_encoder->SetDstSubReg(offset);
            m_encoder->SetSrcSubReg(0, offset);
            m_encoder->SetSrcRegion(0, 1, 1, 0);
            m_encoder->SetSrcSubReg(1, offset);
            m_encoder->SetSrcRegion(1, 1, 1, 0);
            m_encoder->srnd(dst, src0, src1);
            m_encoder->Push();
        }
    }
    else
    {
        uint32_t dstOff = 0, s0Off = 0, s1Off = 0;
        for (int i = 0; i < nelts; ++i)
        {
            if (isSat) {
                m_encoder->SetDstModifier(EMOD_SAT);
            }
            m_encoder->SetNoMask();
            m_encoder->SetDstSubReg(dstOff);
            m_encoder->SetSrcSubReg(0, s0Off);
            m_encoder->SetSrcSubReg(1, s1Off);
            m_encoder->srnd(dst, src0, src1);
            m_encoder->Push();

            dstOff += (dst->IsUniform() ? 1 : nsimdsize);
            s0Off += (src0->IsUniform() ? 1 : nsimdsize);
            s1Off += (src1->IsUniform() ? 1 : nsimdsize);
        }
    }
}

LSC_CACHE_OPTS EmitPass::translateLSCCacheControlsEnum(
    LSC_L1_L3_CC l1l3cc, bool isLoad, const Value* warningContextValue) const
{
    if (!m_pCtx->platform.isSupportedLSCCacheControlsEnum(l1l3cc, isLoad)) {
        m_pCtx->EmitWarning("Unsupported cache controls configuration requested. Applying default configuration.", warningContextValue);
        l1l3cc = LSC_L1DEF_L3DEF;
    }

    LSC_CACHE_OPTS cacheOpts {LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT};
    switch (l1l3cc)
    {
    case LSC_L1DEF_L3DEF:
        cacheOpts = {LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT};
        break;
    case LSC_L1UC_L3UC:
        cacheOpts = {LSC_CACHING_UNCACHED, LSC_CACHING_UNCACHED};
        break;
    case LSC_L1UC_L3C_WB:
        cacheOpts = {
            LSC_CACHING_UNCACHED,
            isLoad ? LSC_CACHING_CACHED : LSC_CACHING_WRITEBACK
        };
        break;
    case LSC_L1C_WT_L3UC:
        cacheOpts = {
            isLoad ? LSC_CACHING_CACHED : LSC_CACHING_WRITETHROUGH,
            LSC_CACHING_UNCACHED
        };
        break;
    case LSC_L1C_WT_L3C_WB:
        if (isLoad)
            cacheOpts = {LSC_CACHING_CACHED, LSC_CACHING_CACHED};
        else
            cacheOpts = {LSC_CACHING_WRITETHROUGH, LSC_CACHING_WRITEBACK};
        break;
    case LSC_L1S_L3UC:
        cacheOpts = {LSC_CACHING_STREAMING, LSC_CACHING_UNCACHED};
        break;
    case LSC_L1S_L3C_WB:
        cacheOpts = {
            LSC_CACHING_STREAMING,
            isLoad ? LSC_CACHING_CACHED : LSC_CACHING_WRITEBACK
        };
        break;
    case LSC_L1IAR_WB_L3C_WB:
        if (isLoad)
            cacheOpts = {LSC_CACHING_READINVALIDATE, LSC_CACHING_CACHED};
        else
            cacheOpts = {LSC_CACHING_WRITEBACK, LSC_CACHING_WRITEBACK};
        break;
    case LSC_L1UC_L3CC:
        if (isLoad) {
            cacheOpts = {LSC_CACHING_UNCACHED,
                LSC_CACHING_CONSTCACHED};
            break;
        }
    case LSC_L1C_L3CC:
        if (isLoad) {
            cacheOpts = {LSC_CACHING_CACHED,
                LSC_CACHING_CONSTCACHED};
            break;
        }
    case LSC_L1IAR_L3IAR:
        if (isLoad) {
            cacheOpts = {LSC_CACHING_READINVALIDATE, LSC_CACHING_READINVALIDATE};
            break;
        }
    default:
        IGC_ASSERT_EXIT_MESSAGE(0, "unsupported caching option");
        break;
    }
    return cacheOpts;
}

LSC_CACHE_OPTS EmitPass::translateLSCCacheControlsFromValue(
    llvm::Value *value, bool isLoad) const
{
    return translateLSCCacheControlsEnum(
        static_cast<LSC_L1_L3_CC>(cast<ConstantInt>(value)->getSExtValue()),
        isLoad, value);
}

std::optional<LSC_CACHE_OPTS>
EmitPass::cacheOptionsForConstantBufferLoads(Instruction* inst, LSC_L1_L3_CC Ctrl) const
{
    if (const Value* resourcePointer = GetBufferOperand(inst))
    {
        uint addressSpace = resourcePointer->getType()->getPointerAddressSpace();
        BufferType bufferType = GetBufferType(addressSpace);
        if ((addressSpace == ADDRESS_SPACE_CONSTANT) ||
            (bufferType == CONSTANT_BUFFER)          ||
            (bufferType == BINDLESS_CONSTANT_BUFFER) ||
            (bufferType == SSH_BINDLESS_CONSTANT_BUFFER))
        {
            return translateLSCCacheControlsEnum(Ctrl, true, inst);
        }
    }
    return std::nullopt;
}

std::optional<LSC_CACHE_OPTS>
EmitPass::cacheOptionsForConstantBufferLoads(Instruction* inst) const
{
    std::optional<LSC_CACHE_OPTS> cacheOpts;
    if (IGC_IS_FLAG_DISABLED(DisableSystemMemoryCachingInGPUForConstantBuffers) &&
        m_currShader->m_Platform->isCoreChildOf(IGFX_XE2_HPG_CORE))
    {
        if (auto Opts = cacheOptionsForConstantBufferLoads(inst, LSC_L1C_L3CC))
            cacheOpts = *Opts;
    }
    if (m_pCtx->type == ShaderType::RAYTRACING_SHADER &&
        IGC_IS_FLAG_ENABLED(ForceRTConstantBufferCacheCtrl))
    {
        auto Ctrl = (LSC_L1_L3_CC)IGC_GET_FLAG_VALUE(RTConstantBufferCacheCtrl);
        if (auto Opts = cacheOptionsForConstantBufferLoads(inst, Ctrl))
            cacheOpts = *Opts;
    }
    return cacheOpts;
}

bool EmitPass::tryOverrideCacheOpts(LSC_CACHE_OPTS& cacheOpts, bool isLoad, bool isTGM,
    const Value* warningContextValue, CacheControlOverride m_CacheControlOption) const
{
    uint32_t l1l3CacheVal = 0;

    if (isTGM)
    {
        l1l3CacheVal = isLoad ?
            (m_CacheControlOption.TgmLoadCacheControlOverride | IGC_GET_FLAG_VALUE(TgmLoadCacheControlOverride)) :
            (m_CacheControlOption.TgmStoreCacheControlOverride | IGC_GET_FLAG_VALUE(TgmStoreCacheControlOverride));
    }
    else
    {
        l1l3CacheVal = isLoad ?
            (m_CacheControlOption.LscLoadCacheControlOverride | IGC_GET_FLAG_VALUE(LscLoadCacheControlOverride)) :
            (m_CacheControlOption.LscStoreCacheControlOverride | IGC_GET_FLAG_VALUE(LscStoreCacheControlOverride));
    }

    if (l1l3CacheVal != 0)
    {
        cacheOpts = translateLSCCacheControlsEnum(static_cast<LSC_L1_L3_CC>(l1l3CacheVal), isLoad, warningContextValue);
    }
    return l1l3CacheVal != 0;
}

LSC_CACHE_OPTS EmitPass::translateLSCCacheControlsFromMetadata(
    Instruction* inst, bool isLoad, bool isTGM) const
{
    LSC_CACHE_OPTS cacheOpts{ LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT };

    if (inst &&
        !m_currShader->m_ModuleMetadata->RasterizerOrderedViews.empty())
    {
        const auto& rasterizerOrderedViews = m_currShader->m_ModuleMetadata->RasterizerOrderedViews;
        if (rasterizerOrderedViews.find(inst->getOperand(0)->getType()->getPointerAddressSpace()) != rasterizerOrderedViews.end())
        {
            // Do not cache ROV in non-coherent cache
            MDNode* node = MDNode::get(
                inst->getContext(),
                ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(inst->getContext()), LSC_L1UC_L3C_WB)));
            inst->setMetadata("lsc.cache.ctrl", node);
        }
    }

    if (isTGM)
    {
        if (m_pCtx->platform.supportsNonDefaultLSCCacheSetting())
        {
            if (tryOverrideCacheOpts(cacheOpts, isLoad, isTGM, inst, m_currShader->m_ModuleMetadata->m_CacheControlOption))
            {
                // global override cache settings have highest priority
                return cacheOpts;
            }

            const MDNode* node = inst ? inst->getMetadata("lsc.cache.ctrl") : nullptr;
            if (node)
            {
                ConstantAsMetadata* MD = cast<ConstantAsMetadata>(node->getOperand(0));
                cacheOpts = translateLSCCacheControlsFromValue(MD->getValue(), isLoad);
            }
        }

        return cacheOpts;
    }

    // To get rid of UGM-before-EOT fence, NEO changes the default and passes it to IGC.
    // Here, set per-inst cache control explicitly to the given default to skip visa's
    // fence WA. And this is done if fence WA is needed.
    if (m_pCtx->type == ShaderType::OPENCL_SHADER &&
        IGC_IS_FLAG_ENABLED(EnableLSCFenceUGMBeforeEOT) && m_pCtx->platform.NeedsLSCFenceUGMBeforeEOT())
    {
        if (isLoad && m_pCtx->getModuleMetaData()->compOpt.LoadCacheDefault != -1)
        {   // load
            LSC_L1_L3_CC L1L3Val = static_cast<LSC_L1_L3_CC>(m_pCtx->getModuleMetaData()->compOpt.LoadCacheDefault);
            cacheOpts = translateLSCCacheControlsEnum(L1L3Val, true, inst);
        }
        else if (!isLoad && m_pCtx->getModuleMetaData()->compOpt.StoreCacheDefault != -1)
        {   // store
            LSC_L1_L3_CC L1L3Val = static_cast<LSC_L1_L3_CC>(m_pCtx->getModuleMetaData()->compOpt.StoreCacheDefault);
            cacheOpts = translateLSCCacheControlsEnum(L1L3Val, false, inst);
        }
    }

    // inst could be nullptr when this function is called
    if (inst)
    {
        // change default setting for private-memory and constant access
        if (auto LI = dyn_cast<LoadInst>(inst))
        {
            auto Ptr = LI->getPointerOperand();
            if (Ptr->getType()->getPointerAddressSpace() == ADDRESS_SPACE_PRIVATE ||
                Ptr->getType()->getPointerAddressSpace() == ADDRESS_SPACE_CONSTANT)
                cacheOpts = { LSC_CACHING_CACHED, LSC_CACHING_CACHED };
        }
        else if (auto SI = dyn_cast<StoreInst>(inst))
        {
            auto Ptr = SI->getPointerOperand();
            if (Ptr->getType()->getPointerAddressSpace() == ADDRESS_SPACE_PRIVATE)
                cacheOpts = { LSC_CACHING_WRITEBACK, LSC_CACHING_WRITEBACK };
        }
    }

    if (tryOverrideCacheOpts(cacheOpts, isLoad, isTGM, inst, m_currShader->m_ModuleMetadata->m_CacheControlOption))
    {
        // global override cache settings have highest priority
        return cacheOpts;
    }
    // next check for MD nodes. Internal lsc.cache.ctrl gets higher priority as they are more precise
    const MDNode* node = inst ? inst->getMetadata("lsc.cache.ctrl") : nullptr;
    if (node) {
        ConstantAsMetadata* MD =
            cast<ConstantAsMetadata>(node->getOperand(0));
        return translateLSCCacheControlsFromValue(MD->getValue(), isLoad);
    }
    node = inst ? inst->getMetadata(LLVMContext::MD_nontemporal) : nullptr;
    if (node)
    {
        return {LSC_CACHING_UNCACHED, LSC_CACHING_UNCACHED};
    }

    // RT specific handling
    bool hasRQCall = (m_pCtx->type == ShaderType::RAYTRACING_SHADER);
    if (!hasRQCall && inst)
    {
        hasRQCall = m_pCtx->hasSyncRTCalls(inst->getFunction());
    }
    if (hasRQCall)
    {
        cacheOpts = getDefaultRaytracingCachePolicy(isLoad);
    }

    if (inst && isLoad)
    {
        if (auto Opts = cacheOptionsForConstantBufferLoads(inst))
            cacheOpts = *Opts;
    }

    return cacheOpts;
}

static int toBytesGrf(LSC_DATA_SIZE dsz) {
    return
        dsz == LSC_DATA_SIZE_8b ? 1 :
        dsz == LSC_DATA_SIZE_16b ? 2 :
        dsz == LSC_DATA_SIZE_64b ? 8 : 4;
}
static int toBytesMem(LSC_DATA_SIZE dsz) {
    return
        dsz == LSC_DATA_SIZE_8b || dsz == LSC_DATA_SIZE_8c32b ? 1 :
        dsz == LSC_DATA_SIZE_16b || dsz == LSC_DATA_SIZE_16c32b || dsz == LSC_DATA_SIZE_16c32bH ? 2 :
        dsz == LSC_DATA_SIZE_64b ? 8 :
        4;
}
static int toCount(LSC_DATA_ELEMS elems)
{
    int dataElems = 1;
    switch (elems) {
    case LSC_DATA_ELEMS_1: dataElems = 1; break;
    case LSC_DATA_ELEMS_2: dataElems = 2; break;
    case LSC_DATA_ELEMS_3: dataElems = 3; break;
    case LSC_DATA_ELEMS_4: dataElems = 4; break;
    case LSC_DATA_ELEMS_8: dataElems = 8; break;
    case LSC_DATA_ELEMS_16: dataElems = 16; break;
    case LSC_DATA_ELEMS_32: dataElems = 32; break;
    case LSC_DATA_ELEMS_64: dataElems = 64; break;
    default:
        IGC_ASSERT_EXIT_MESSAGE(0, "invalid number of elems");
    }
    return dataElems;
}

EmitPass::LscMessageFragmentInfo EmitPass::checkForLscMessageFragmentation(
    LSC_DATA_SIZE dataSize, LSC_DATA_ELEMS dataElems) const
{
    const int dataSizeRegBytes = toBytesGrf(dataSize);
    const int dataSizeMemBytes = toBytesMem(dataSize);
    // const int dataElemsCount = toCount(dataElems);
    const bool isDg2 = (m_currShader->m_Platform->getPlatformInfo().eProductFamily == IGFX_DG2
        || m_currShader->m_Platform->getPlatformInfo().eProductFamily == IGFX_METEORLAKE
        || m_currShader->m_Platform->getPlatformInfo().eProductFamily == IGFX_ARROWLAKE);
    const bool isPvcPlus = !isDg2;
    const int simdElems = numLanes(m_currShader->m_SIMDSize);
    bool halfSimdMode =
        (isPvcPlus && m_currShader->m_SIMDSize == SIMDMode::SIMD16) ||
        (isDg2 && m_currShader->m_SIMDSize == SIMDMode::SIMD8);

    // first assume no split
    LscMessageFragmentInfo fi { };
    fi.fragElemCount = 1;
    fi.fragElem = dataElems;
    fi.lastIsV1 = false;

    auto setFragments = [&] (int numFrags, LSC_DATA_ELEMS frag, bool lastIsV1 = false) {
        fi.fragElem = frag;
        fi.fragElemCount = numFrags;
        fi.addrOffsetDelta = toCount(frag) * dataSizeMemBytes;
        fi.grfOffsetDelta = toCount(frag) * simdElems * dataSizeRegBytes;
        fi.lastIsV1 = lastIsV1;
    };

    switch (dataSize) {
    case LSC_DATA_SIZE_8b:
    case LSC_DATA_SIZE_16b:
        // no split
        break;
    case LSC_DATA_SIZE_64b:
        switch (dataElems) {
        case LSC_DATA_ELEMS_1:
        case LSC_DATA_ELEMS_2:
            // no split
            break;
        case LSC_DATA_ELEMS_3:
        case LSC_DATA_ELEMS_4:
            if (!halfSimdMode) {
                setFragments(2, LSC_DATA_ELEMS_2, dataElems == LSC_DATA_ELEMS_3);
            } // else no split needed
            break;
        case LSC_DATA_ELEMS_8: // D64 V8
            if (halfSimdMode) {
                setFragments(2, LSC_DATA_ELEMS_4);
            } else {
                setFragments(4, LSC_DATA_ELEMS_2);
            }
            break;
        case LSC_DATA_ELEMS_16: // D64 V16
            if (halfSimdMode) {
                setFragments(4, LSC_DATA_ELEMS_4);
            } else {
                setFragments(8, LSC_DATA_ELEMS_2);
            }
            break;
        default:
            IGC_ASSERT_EXIT_MESSAGE(0, "vector size too large for intrinsic");
        }
        break;
    default: // 32b cases (including conversion types)
        switch (dataElems) {
        case LSC_DATA_ELEMS_1:
        case LSC_DATA_ELEMS_2:
        case LSC_DATA_ELEMS_3:
        case LSC_DATA_ELEMS_4:
            // no split
            break;
        case LSC_DATA_ELEMS_8:
            if (!halfSimdMode) {
                setFragments(2, LSC_DATA_ELEMS_4);
            } // else no split
            break;
        case LSC_DATA_ELEMS_16:
            if (halfSimdMode) {
                setFragments(2, LSC_DATA_ELEMS_8);
            } else {
                setFragments(4, LSC_DATA_ELEMS_4);
            }
            break;
        default:
            IGC_ASSERT_EXIT_MESSAGE(0, "vector size too large for SIMT intrinsic");
        }
        break;
    }
    return fi;
}

void EmitPass::emitLscIntrinsicFragments(
    CVariable* dataVarBase,
    LSC_DATA_SIZE dataSize,
    LSC_DATA_ELEMS dataElems,
    int immOffset,
    const LscIntrinsicFragmentEmitter &emitter)
{
    const auto fi = checkForLscMessageFragmentation(dataSize, dataElems);
    int visaElemsPerFragment =
        toCount(fi.fragElem) * numLanes(m_currShader->m_SIMDSize);

    CVariable *dataVar = dataVarBase;
    for (int fragIx = 0; fragIx < fi.fragElemCount; fragIx++) {
        LSC_DATA_ELEMS fragElems =
            fragIx == fi.fragElemCount - 1 && fi.lastIsV1 ?
                LSC_DATA_ELEMS_1 : fi.fragElem;

        const int fragImmOffset = immOffset + fragIx * fi.addrOffsetDelta;
        // overflow
        IGC_ASSERT_EXIT_MESSAGE(fragImmOffset >= immOffset, "imm offset overflows");

        emitter(dataVar, fragIx, fragElems, fragImmOffset);

        if (fragIx != fi.fragElemCount - 1) {
            // walk destination forward through the payload to target the
            // next chunk
            if (fragIx == fi.fragElemCount - 2 && fi.lastIsV1) {
                // e.g. V3 splits to V2, V1
                visaElemsPerFragment = numLanes(m_currShader->m_SIMDSize); // V1
            }
            if (dataVarBase != nullptr)
                dataVar = m_currShader->GetNewAlias(
                    dataVarBase,
                    dataVarBase->GetType(),
                    (uint16_t)((fragIx + 1) * fi.grfOffsetDelta),
                    (uint16_t)visaElemsPerFragment);
        }
    }
}

void EmitPass::emitLscIntrinsicLoad(llvm::GenIntrinsicInst* inst)
{
    // Intrinsic format:
    //  gatherDst - [non-uniform] data,vector read/prefetch from memory
    //              ugm - flat,stateless,buffer,scratch,null
    //              slm - flat
    //
    //  Operand 0 -
    //     LSCLoad - [non-uniform] memory address where the data,vector is read/prefetch from
    //     LSCLoadBlock - [uniform] ...
    //  Operand 1 - [immediate] bytes offset (an int)
    //  Operand 2 - [immediate] data size,    (LSC_DATA_SIZE enum)
    //  Operand 3 - [immediate] vector size,  (LSC_DATA_ELEMS enum)
    //  Operand 4 - [immediate] cache options (LSC_CACHE_OPT enum)
    IGC_ASSERT(inst);
    Value* Ptr = inst->getArgOperand(0);

    PointerType* ptrType = cast<PointerType>(Ptr->getType());

    ResourceDescriptor resource = GetResourceVariable(Ptr);
    CVariable* offset = GetSymbol(Ptr);
    bool useA32 = !isA64Ptr(ptrType, m_currShader->GetContext());
    LSC_ADDR_SIZE addrSize = useA32 ? LSC_ADDR_SIZE_32b : LSC_ADDR_SIZE_64b;
    offset = useA32 ? TruncatePointer(offset) : offset;
    bool isBlockLoad = inst->getIntrinsicID() == GenISAIntrinsic::GenISA_LSCLoadBlock;
    if (isBlockLoad) {
        // TODO: could just use low channel
        IGC_ASSERT_EXIT_MESSAGE(offset->IsUniform(), "block operation base must be uniform");
    } else {
        offset = BroadcastIfUniform(offset);
    }

    const int immOffset = int_cast<int>(cast<ConstantInt>(inst->getOperand(1))->getSExtValue());
    auto dataSize = (LSC_DATA_SIZE)cast<ConstantInt>(inst->getOperand(2))->getZExtValue();
    auto dataElems = (LSC_DATA_ELEMS)cast<ConstantInt>(inst->getOperand(3))->getZExtValue();

    LSC_CACHE_OPTS cacheOpts = translateLSCCacheControlsFromValue(inst->getOperand(4), true);
    LSC_DOC_ADDR_SPACE addrSpace = m_pCtx->getUserAddrSpaceMD().Get(inst);

    if (auto Opts = cacheOptionsForConstantBufferLoads(inst))
        cacheOpts = *Opts;

    emitLscIntrinsicFragments(m_destination, dataSize, dataElems, immOffset,
        [&] (CVariable* gatherDst, int fragIx, LSC_DATA_ELEMS fragElems, int fragImmOffset) {
            if (isBlockLoad) {
                m_encoder->LSC_LoadBlock1D(
                    gatherDst, offset,
                    dataSize, fragElems, &resource,
                    addrSize,
                    fragImmOffset,
                    cacheOpts);
            } else {
                m_encoder->LSC_LoadGather(
                    LSC_LOAD, gatherDst,
                    offset, dataSize, fragElems, 0, &resource,
                    addrSize,
                    LSC_DATA_ORDER_NONTRANSPOSE, fragImmOffset, 1, cacheOpts, addrSpace);
            }
            m_encoder->Push();
        });
}

void EmitPass::emitLscIntrinsicLoadCmask(llvm::GenIntrinsicInst* inst)
{
    // Intrinsic format:
    //  gatherDst - [non-uniform] data,vector read/prefetch from memory
    //              tgm - flat,stateless,buffer,scratch,null
    //
    //  Operand 0 - [non-uniform] memory address where the data,vector is read/prefetch from
    //  Operand 1 - [immediate] bytes offset (an int)
    //  Operand 2 - [immediate] data size,    (LSC_DATA_SIZE enum)
    //  Operand 3 - [immediate] vector size,  (LSC_DATA_ELEMS enum)
    //  Operand 4 - [immediate] cache options (LSC_CACHE_OPT enum)

    Value* Ptr = inst->getArgOperand(0);
    PointerType* ptrType = cast<PointerType>(Ptr->getType());

    ResourceDescriptor resource = GetResourceVariable(Ptr);
    CVariable* offset = GetSymbol(Ptr);
    bool useA32 = !isA64Ptr(ptrType, m_currShader->GetContext());

    // load_cmask is tgm;
    // tgm requires a32;
    useA32 = true;

    offset = useA32 ? TruncatePointer(offset) : offset;
    auto addrSize = useA32 ? LSC_ADDR_SIZE_32b : LSC_ADDR_SIZE_64b;
    const int immOffset = int_cast<int>(cast<ConstantInt>(inst->getOperand(1))->getSExtValue());

    auto dataSize = (LSC_DATA_SIZE)cast<ConstantInt>(inst->getOperand(2))->getZExtValue();

    // it's mask rather than size
    auto dataElems = (LSC_DATA_ELEMS)cast<ConstantInt>(inst->getOperand(3))->getZExtValue();
    uint32_t dataElemNum = m_encoder->LSC_GetElementNum( dataElems );

    const LSC_CACHE_OPTS cacheOpts =
        translateLSCCacheControlsFromValue(inst->getOperand(4), true);


    CVariable* tempdst[4] = { nullptr, nullptr, nullptr, nullptr };
    auto instWidth = m_currShader->m_Platform->getMaxLSCTypedMessageSize();
    bool needsSplit = m_currShader->m_SIMDSize > instWidth;

    const CShader::ExtractMaskWrapper writeMask( m_currShader, inst );
    uint numChannels = iSTD::BitCount( writeMask.getEM() );

    auto eltBitSize = 8;
    switch (dataSize)
    {
    case LSC_DATA_SIZE_16b:
        eltBitSize *= 2;
        break;
    case LSC_DATA_SIZE_32b:
        eltBitSize *= 4;
        break;
    case LSC_DATA_SIZE_64b:
        eltBitSize *= 8;
        break;
    default:
        break;
    }

    if (!needsSplit)
    {
        m_encoder->LSC_TypedReadWrite( LSC_LOAD_QUAD, &resource,
            offset + immOffset, nullptr, nullptr, nullptr,
            m_destination, eltBitSize,
            dataElems, // function header expects a mask
            addrSize,
            0x1 // mask correlated with dataElemNum
        );

        m_encoder->Push();
    }
    else
    {
        const unsigned int numLanesForInstWidth = numLanes(instWidth);
        IGC_ASSERT(numLanesForInstWidth);
        const unsigned int splitInstCount
            = numLanes(m_currShader->m_SIMDSize) / numLanesForInstWidth;

        for (uint i = 0; i < splitInstCount; ++i)
        {
            tempdst[i] = m_currShader->GetNewVariable(
                numLanesForInstWidth * dataElemNum, // size in # of elems
                ISA_TYPE_F,
                EALIGN_GRF,
                CName::NONE);

            setSIMDSizeMask(m_encoder, m_currShader, i);
            m_encoder->SetSrcSubVar(0, i);

            m_encoder->LSC_TypedReadWrite( LSC_LOAD_QUAD, &resource,
                offset + immOffset, nullptr, nullptr, nullptr,
                tempdst[ i ], eltBitSize,
                dataElems, // function header expects a mask
                addrSize,
                1 );

            m_encoder->Push();
        }
    }

    if (m_currShader->m_SIMDSize != instWidth)
    {
        JoinSIMD(tempdst, numChannels, instWidth);
    }
}

void EmitPass::emitLscIntrinsicPrefetch(llvm::GenIntrinsicInst* inst)
{
    // Intrinsic format:
    //  gatherDst - [non-uniform] data,vector read/prefetch from memory
    //              ugm - flat,stateless,buffer,scratch,null
    //              slm - flat
    //
    //  Operand 0 - [non-uniform] memory address where the data,vector is read/prefetch from
    //  Operand 1 - [uniform] immediate offset (an int)
    //  Operand 2 - [uniform] data size,    (LSC_DATA_SIZE enum)
    //  Operand 3 - [uniform] vector size,  (LSC_DATA_ELEMS enum)
    //  Operand 4 - [uniform] cache options (LSC_CACHE_OPT enum)

    CVariable* gatherDst =
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_LSCPrefetch ?
            nullptr : m_destination;
    Value* Ptr = inst->getArgOperand(0);
    PointerType* ptrType = cast<PointerType>(Ptr->getType());

    ResourceDescriptor resource = GetResourceVariable(Ptr);
    CVariable* offset = GetSymbol(Ptr);
    bool useA32 = !isA64Ptr(ptrType, m_currShader->GetContext());
    LSC_ADDR_SIZE addrSize = useA32 ? LSC_ADDR_SIZE_32b : LSC_ADDR_SIZE_64b;
    offset = useA32 ? TruncatePointer(offset) : offset;

    auto dataSize = (LSC_DATA_SIZE)cast<ConstantInt>(inst->getOperand(2))->getZExtValue();
    auto dataElems = (LSC_DATA_ELEMS)cast<ConstantInt>(inst->getOperand(3))->getZExtValue();
    int immOffset = int_cast<int>(cast<ConstantInt>(inst->getOperand(1))->getSExtValue());

    LSC_CACHE_OPTS cacheOpts = translateLSCCacheControlsFromValue(inst->getOperand(4), true);

    if (cacheOpts.l1 == LSC_CACHING_READINVALIDATE && cacheOpts.l3 == LSC_CACHING_CACHED)
    {
        // These cache opts are not allowed for prefetch message.
        cacheOpts.l1 = cacheOpts.l3 = LSC_CACHING_CACHED;
        IGC_ASSERT_EXIT_MESSAGE(0, "read-invalidate not permitted on prefetch");
    }

    if (cacheOpts.l1 == LSC_CACHING_DEFAULT &&
        cacheOpts.l3 == LSC_CACHING_DEFAULT &&
        m_currShader->m_Platform->getPlatformInfo().eProductFamily == IGFX_PVC &&
        // needed for PVC XL A0 RevID==0x0, fixed from PVC XT A0 RevID==0x3==REVISION_B
        m_currShader->m_Platform->getPlatformInfo().usRevId < REVISION_B)
    {
        // [PVC XL A0 RevID=0x0] Default cache opts are not allowed for a Prefetch message.
        cacheOpts.l1 = cacheOpts.l3 = LSC_CACHING_CACHED;
        IGC_ASSERT_EXIT_MESSAGE(0, "invalid cache opts for prefetch (must be non-default)");
    }

    // use LSC_LOAD will null dst for prefetch
    LSC_OP lscOp =
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_LSCLoadStatus ?
            LSC_LOAD_STATUS : LSC_LOAD;
    //
    LSC_DOC_ADDR_SPACE addrSpace = m_pCtx->getUserAddrSpaceMD().Get(inst);

    emitLscIntrinsicFragments(gatherDst, dataSize, dataElems, immOffset,
        [&] (CVariable* fragDst, int fragIx, LSC_DATA_ELEMS fragElems, int fragImmOffset) {
          m_encoder->LSC_LoadGather(
              lscOp, fragDst,
              offset, dataSize, fragElems, 0, &resource,
              addrSize,
              LSC_DATA_ORDER_NONTRANSPOSE, fragImmOffset, 1, cacheOpts, addrSpace);
            m_encoder->Push();
        });
    //
    if (lscOp == LSC_LOAD_STATUS)
    {
        // GenISA_LSCLoadGlobalStatus: returns a single DW in the low DW of the
        // return register with 1 bit per result.  We must broadcast each bit
        // to a truth value per workitem.
        //
        //       (W)    mov (1)  flag:ud  dst.0<0,1,0>:ud
        //       (flag) sel (32) dst:ud   0x1:ud      0x0:ud
        //
        // CF. ~4060 (broadcasting a sampler's returned status word)
        SIMDMode simdSize = m_currShader->m_SIMDSize;
        CVariable* flag = m_currShader->GetNewVariable(
            numLanes(m_currShader->m_State.m_dispatchSize),
            ISA_TYPE_BOOL,
            EALIGN_BYTE,
            CName::NONE);

        const unsigned int GRFSizeBy4 = getGRFSize() / 4;
        IGC_ASSERT(GRFSizeBy4);
        const unsigned int subvar = numLanes(simdSize) / GRFSizeBy4 * 4;

        m_encoder->SetSrcSubVar(0, subvar);
        m_encoder->SetSrcRegion(0, 0, 1, 0);
        IGC_ASSERT(nullptr != gatherDst);
        CVariable* newDst = m_currShader->BitCast(gatherDst, ISA_TYPE_UD);
        m_encoder->SetP(flag, newDst);
        m_encoder->Push();
        //
        CVariable* pred = m_currShader->ImmToVariable(0x1, gatherDst->GetType());
        CVariable* zero = m_currShader->ImmToVariable(0x0, gatherDst->GetType());
        m_encoder->SetDstSubVar(subvar);
        m_encoder->Select(flag, gatherDst, pred, zero);
        m_encoder->Push();
    }
}

void EmitPass::emitLscSimdBlockPrefetch(llvm::GenIntrinsicInst* inst)
{
    // SIMD Block Prefetch is just block read with null dst, so it can be handled
    // with the same function as simdBlockRead.
    emitLSCSimdBlockRead(inst);
}

void EmitPass::emitLscIntrinsicStore(llvm::GenIntrinsicInst* inst)
{
    // Intrinsic format:
    //  Operand 0 - [non-uniform] memory address where the data is stored
    //              [uniform] for
    //              ugm - flat,stateless,buffer,scratch
    //              slm - flat
    //  Operand 1 - [immediate] element offset (in elements)
    //  Operand 2 - [non-uniform] data,vector to be stored into memory from registers
    //  Operand 3 - [immediate] data size,    (LSC_DATA_SIZE enum)
    //  Operand 4 - [immediate] vector size,  (LSC_DATA_ELEMS enum)
    //  Operand 5 - [immediate] cache options (LSC_CACHE_OPT enum)

    Value* Ptr  = inst->getArgOperand(0);
    int immOffset = int_cast<int>(cast<ConstantInt>(inst->getOperand(1))->getSExtValue());
    Value* storedVal = inst->getArgOperand(2);
    CVariable* storedVar = GetSymbol(storedVal);
    storedVar = BroadcastIfUniform(storedVar);
    LSC_DOC_ADDR_SPACE addrspace = m_pCtx->getUserAddrSpaceMD().Get(inst);

    ResourceDescriptor resource = GetResourceVariable(Ptr);
    PointerType* ptrType = cast<PointerType>(Ptr->getType());
    CVariable* offset = GetSymbol(Ptr);
    bool useA32 = !isA64Ptr(ptrType, m_currShader->GetContext());
    LSC_ADDR_SIZE addrSize = useA32 ? LSC_ADDR_SIZE_32b : LSC_ADDR_SIZE_64b;
    offset = useA32 ? TruncatePointer(offset) : offset;
    bool isBlockStore = inst->getIntrinsicID() == GenISAIntrinsic::GenISA_LSCStoreBlock;
    if (isBlockStore) {
        // TODO: could just use low channel
        IGC_ASSERT_EXIT_MESSAGE(offset->IsUniform(), "block operation base must be uniform");
    } else {
        offset = BroadcastIfUniform(offset);
    }
    // In case eOffset isn't GRF aligned, need to create a copy
    // For non-uniform variable, it should be already GRF-aligned.
    offset = ReAlignUniformVariable(offset, EALIGN_GRF);

    auto dataSize = (LSC_DATA_SIZE)cast<ConstantInt>(inst->getOperand(3))->getZExtValue();
    auto dataElems = (LSC_DATA_ELEMS)cast<ConstantInt>(inst->getOperand(4))->getZExtValue();

    LSC_CACHE_OPTS cacheOpts =
        translateLSCCacheControlsFromValue(inst->getOperand(5), false);
    m_currShader->m_State.HasLscStoreCacheControls(cacheOpts);
    emitLscIntrinsicFragments(storedVar, dataSize, dataElems, immOffset,
        [&] (CVariable* fragData, int fragIx, LSC_DATA_ELEMS fragElems, int fragImmOffset) {
            if (isBlockStore) {
                m_encoder->LSC_StoreBlock1D(
                    fragData, offset,
                    dataSize, fragElems,
                    &resource,
                    addrSize,
                    fragImmOffset,
                    cacheOpts);
            } else {
                m_encoder->LSC_StoreScatter(
                    LSC_STORE,
                    fragData, offset,
                    dataSize, fragElems,
                    0, &resource,
                    addrSize,
                    LSC_DATA_ORDER_NONTRANSPOSE,
                    fragImmOffset,
                    1,
                    cacheOpts,
                    addrspace);
            }
            m_encoder->Push();
        });
}

void EmitPass::emitLscIntrinsicStoreCmask(llvm::GenIntrinsicInst* inst)
{
    // Intrinsic format:
    //  Operand 0 - [non-uniform] memory address where the data,vector is stored
    //              ugm - flat,stateless,buffer,scratch
    //              slm - flat
    //  Operand 1 - [immediate] element offset (in elements)
    //  Operand 2 - [non-uniform] data,vector to be stored into memory from registers
    //  Operand 3 - [immediate] data size,    (LSC_DATA_SIZE enum)
    //  Operand 4 - [immediate] vector size,  (LSC_DATA_ELEMS enum)
    //  Operand 5 - [immediate] cache options (LSC_CACHE_OPT enum)

    Value* Ptr = inst->getArgOperand(0);
    int immOffset = int_cast<int>(cast<ConstantInt>(inst->getOperand(1))->getSExtValue());

    Value* storedVal = inst->getArgOperand(2);
    CVariable* storedVar = GetSymbol(storedVal);
    storedVar = BroadcastIfUniform(storedVar);

    ResourceDescriptor resource = GetResourceVariable(Ptr);
    PointerType* ptrType = cast<PointerType>(Ptr->getType());
    CVariable* offset = GetSymbol(Ptr);
    bool useA32 = !isA64Ptr(ptrType, m_currShader->GetContext());

    // load_cmask is tgm; tgm requires a32;
    useA32 = true;

    offset = useA32 ? TruncatePointer(offset) : offset;
    // In case eOffset isn't GRF aligned, need to create a copy
    // For non-uniform variable, it should be already GRF-aligned.
    offset = ReAlignUniformVariable(offset, EALIGN_GRF);

    auto addrSize = useA32 ? LSC_ADDR_SIZE_32b : LSC_ADDR_SIZE_64b;
    auto dataSize = (LSC_DATA_SIZE)cast<ConstantInt>(inst->getOperand(3))->getZExtValue();

    // cmask
    auto dataElems = (LSC_DATA_ELEMS)cast<ConstantInt>(inst->getOperand(4))->getZExtValue();
    uint32_t dataElemNum = m_encoder->LSC_GetElementNum( dataElems );

    LSC_CACHE_OPTS cacheOpts =
        translateLSCCacheControlsFromValue(inst->getOperand(5), false);

    CVariable* pSrc_X = storedVar;

    auto instWidth = m_currShader->m_Platform->getMaxLSCTypedMessageSize();
    bool needsSplit = m_currShader->m_SIMDSize > instWidth;

    auto eltBitSize = 8;
    switch (dataSize)
    {
    case LSC_DATA_SIZE_16b:
        eltBitSize *= 2;
        break;
    case LSC_DATA_SIZE_32b:
        eltBitSize *= 4;
        break;
    case LSC_DATA_SIZE_64b:
        eltBitSize *= 8;
        break;
    default:
        break;
    }

    if (!needsSplit)
    {
        CVariable* pPayload = m_currShader->GetNewVariable(
            dataElemNum * numLanes( m_currShader->m_SIMDSize ),
            ISA_TYPE_F,
            EALIGN_GRF,
            CName::NONE );
        m_currShader->CopyVariable( pPayload, pSrc_X, 0 );
        m_encoder->LSC_TypedReadWrite( LSC_STORE_QUAD, &resource,
            offset + immOffset, nullptr, nullptr, nullptr,
            pPayload, eltBitSize,
            dataElems, // mask
            addrSize,
            0x1 );
        m_encoder->Push();
    }
    else
    {
        const unsigned int numLanesForInstWidth = numLanes( instWidth );

        for( uint i = 0; i < 2; ++i )
        {
            CVariable* pPayload = nullptr;
            pPayload = m_currShader->GetNewVariable(
                numLanesForInstWidth * dataElemNum, // size in # of elems
                ISA_TYPE_F,
                EALIGN_GRF, CName::NONE );

            setSIMDSizeMask( m_encoder, m_currShader, i );
            if( !pSrc_X->IsUniform() )
            {
                m_encoder->SetSrcSubVar( 0, i );
            }
            m_encoder->SetDstSubVar( 0 );
            m_encoder->Copy( pPayload, pSrc_X );
            m_encoder->Push();

            setSIMDSizeMask( m_encoder, m_currShader, i );
            m_encoder->SetSrcSubVar( 0, i );
            m_encoder->LSC_TypedReadWrite( LSC_STORE_QUAD, &resource,
                offset + immOffset, nullptr, nullptr, nullptr,
                pPayload, eltBitSize,
                dataElems, // function header expects a mask
                addrSize,
                0x1 );
            m_encoder->Push();
        }
    }
}

void EmitPass::emitLSCLoad(
    Instruction* inst,
    CVariable* dst,
    CVariable* offset,
    unsigned elemSize,
    unsigned numElems,
    unsigned blockOffset,
    ResourceDescriptor* resource,
    LSC_ADDR_SIZE addr_size,
    LSC_DATA_ORDER data_order,
    int immOffset,
    int immScale)
{
    LSC_CACHE_OPTS cacheOpts =
        translateLSCCacheControlsFromMetadata(inst, true);
    LSC_DOC_ADDR_SPACE addrSpace = m_pCtx->getUserAddrSpaceMD().Get(inst);
    emitLSCLoad(cacheOpts, dst, offset, elemSize, numElems, blockOffset,
                resource, addr_size, data_order, immOffset, immScale, addrSpace);
}

void EmitPass::emitLSCLoad(
    LSC_CACHE_OPTS cacheOpts,
    CVariable* dst,
    CVariable* offset,
    unsigned elemSize,
    unsigned numElems,
    unsigned blockOffset,
    ResourceDescriptor* resource,
    LSC_ADDR_SIZE addr_size,
    LSC_DATA_ORDER data_order,
    int immOffset,
    int immScale,
    LSC_DOC_ADDR_SPACE addrSpace)
{
    LSC_DATA_SIZE elemSizeEnum = m_encoder->LSC_GetElementSize(elemSize);
    LSC_DATA_ELEMS numElemsEnum = m_encoder->LSC_GetElementNum(numElems);
    m_encoder->LSC_LoadGather(LSC_LOAD, dst,
                              offset, elemSizeEnum, numElemsEnum, blockOffset,
                              resource, addr_size, data_order, immOffset, immScale,
                              cacheOpts, addrSpace);
}



void EmitPass::emitLSCStore(
    Instruction* inst,
    CVariable* src,
    CVariable* offset,
    unsigned elemSize,
    unsigned numElems,
    unsigned blockOffset,
    ResourceDescriptor* resource,
    LSC_ADDR_SIZE addr_size,
    LSC_DATA_ORDER data_order,
    int immOffset,
    int immScale)
{
    LSC_DOC_ADDR_SPACE addrSpace = m_pCtx->getUserAddrSpaceMD().Get(inst);
    LSC_CACHE_OPTS cacheOpts = translateLSCCacheControlsFromMetadata(inst, false);
    emitLSCStore(cacheOpts, src, offset, elemSize, numElems, blockOffset,
                 resource, addr_size, data_order, immOffset, immScale, addrSpace);
}

void EmitPass::emitLSCStore(
    LSC_CACHE_OPTS cacheOpts,
    CVariable* src,
    CVariable* offset,
    unsigned elemSize,
    unsigned numElems,
    unsigned blockOffset,
    ResourceDescriptor* resource,
    LSC_ADDR_SIZE addr_size,
    LSC_DATA_ORDER data_order,
    int immOffset,
    int immScale,
    LSC_DOC_ADDR_SPACE addrSpace)
{
    LSC_DATA_SIZE elemSizeEnum = m_encoder->LSC_GetElementSize(elemSize);
    LSC_DATA_ELEMS numElemsEnum = m_encoder->LSC_GetElementNum(numElems);
    m_currShader->m_State.HasLscStoreCacheControls(cacheOpts);
    m_encoder->LSC_StoreScatter(LSC_STORE,
                                src, offset, elemSizeEnum, numElemsEnum,
                                blockOffset, resource, addr_size, data_order,
                                immOffset, immScale, cacheOpts, addrSpace);
}

void EmitPass::emitLSC2DBlockOperation(llvm::GenIntrinsicInst* inst)
{
    bool isRead = inst->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockRead;
    const bool isPrefetch = inst->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockPrefetch;
    isRead |= isPrefetch;

    CVariable* pFlatImageBaseoffset = GetSymbol(inst->getOperand(0));
    CVariable* pFlatImageWidth = GetSymbol(inst->getOperand(1));
    CVariable* pFlatImageHeight = GetSymbol(inst->getOperand(2));
    CVariable* pFlatImagePitch = GetSymbol(inst->getOperand(3));
    CVariable* pXOffset = GetSymbol(inst->getOperand(4));
    CVariable* pYOffset = GetSymbol(inst->getOperand(5));

    // element size, width and height must be supplied as compile time constants.
    uint elemSizeInBits = (uint)cast<ConstantInt>(inst->getOperand(6))->getZExtValue();
    uint blockWidth = (uint)cast<ConstantInt>(inst->getOperand(7))->getZExtValue();
    uint blockHeight = (uint)cast<ConstantInt>(inst->getOperand(8))->getZExtValue();
    uint numBlocksV = (uint)cast<ConstantInt>(inst->getOperand(9))->getZExtValue();
    bool isTranspose = (uint)cast<ConstantInt>(inst->getOperand(10))->getZExtValue();
    bool isVnni = (uint)cast<ConstantInt>(inst->getOperand(11))->getZExtValue();

    CVariable* destination = m_destination;
    if (numBlocksV == 2 && blockHeight == 1 &&
        !isPrefetch &&
        elemSizeInBits * blockWidth == 256 &&
        m_currShader->m_Platform->getPlatformInfo().eProductFamily >= IGFX_PVC)
    {
        // m1 v2 block read has grf aligned V blocks,
        // variable should be 2x larger
        destination = m_currShader->GetNewVariable(
            m_destination->GetNumberElement() * 2,
            m_destination->GetType(),
            m_destination->GetAlign(),
            m_destination->IsUniform(),
            m_destination->GetNumberInstance(),
            CName::NONE);
    }
    LSC_CACHE_OPTS cacheOpts = translateLSCCacheControlsFromValue(inst->getOperand(12), isRead);

    if (isRead == false)
    {
        const uint storeDestinationOperandId = 13;
        destination = BroadcastIfUniform(GetSymbol(inst->getOperand(storeDestinationOperandId)));
    }

    m_encoder->LSC_2DBlockMessage(
        isRead ? LSC_LOAD_BLOCK2D : LSC_STORE_BLOCK2D,
        nullptr,
        destination,
        nullptr, //pImgBTI - not needed for read
        pXOffset,
        pYOffset,
        blockWidth,  //elements based
        blockHeight,
        elemSizeInBits,
        numBlocksV,
        isTranspose,
        isVnni,
        pFlatImageBaseoffset,
        pFlatImageWidth,
        pFlatImageHeight,
        pFlatImagePitch,
        cacheOpts);
    m_encoder->Push();

    if (isRead &&
        !isPrefetch &&
        destination != m_destination)
    {
        // m1 v2 block read
        m_encoder->Copy(m_destination, destination);
        m_encoder->Push();
        m_encoder->SetSrcSubVar(0, 1);
        m_encoder->SetDstSubReg(m_destination->GetNumberElement() / 2);
        m_encoder->Copy(m_destination, destination);
        m_encoder->Push();
    }
}

void EmitPass::emitLSC2DBlockAddrPayload(GenIntrinsicInst* GII)
{
    if (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockCopyAddrPayload)
    {
        CVariable* srcAP = GetSymbol(GII->getOperand(0));
        IGC_ASSERT(m_destination != srcAP);
        m_encoder->SetUniformSIMDSize(SIMDMode::SIMD8);
        m_encoder->SetDstSubReg(0);
        m_encoder->SetSrcRegion(0, 1, 1, 0);
        m_encoder->Copy(m_destination, srcAP);
        m_encoder->Push();
        return;
    }

    // Surface parameters
    CVariable* Base = GetSymbol(GII->getOperand(0));
    CVariable* WidthM1 = GetSymbol(GII->getOperand(1));
    CVariable* HeightM1 = GetSymbol(GII->getOperand(2));
    CVariable* PitchM1 = GetSymbol(GII->getOperand(3));
    // Block parameters
    CVariable* XOffset = GetSymbol(GII->getOperand(4));
    CVariable* YOffset = GetSymbol(GII->getOperand(5));
    uint32_t bWidth = (uint32_t)cast<ConstantInt>(GII->getOperand(6))->getZExtValue();
    uint32_t bHeight = (uint32_t)cast<ConstantInt>(GII->getOperand(7))->getZExtValue();
    uint32_t numBlks = (uint32_t)cast<ConstantInt>(GII->getOperand(8))->getZExtValue();
    uint32_t blkDim = (((numBlks-1) << 16) | ((bHeight-1) << 8) | (bWidth-1));

    CVariable* dstQW = m_currShader->GetNewAlias(m_destination, ISA_TYPE_Q, 0, 1, true);
    CVariable* blkDimVar = m_currShader->ImmToVariable(blkDim, ISA_TYPE_D);
    m_encoder->SetUniformSIMDSize(SIMDMode::SIMD1);
    m_encoder->SetDstSubReg(0);
    m_encoder->Copy(dstQW, Base);
    m_encoder->Push();
    m_encoder->SetUniformSIMDSize(SIMDMode::SIMD1);
    m_encoder->SetDstSubReg(2);
    m_encoder->Copy(m_destination, WidthM1);
    m_encoder->Push();
    m_encoder->SetUniformSIMDSize(SIMDMode::SIMD1);
    m_encoder->SetDstSubReg(3);
    m_encoder->Copy(m_destination, HeightM1);
    m_encoder->Push();
    m_encoder->SetUniformSIMDSize(SIMDMode::SIMD1);
    m_encoder->SetDstSubReg(4);
    m_encoder->Copy(m_destination, PitchM1);
    m_encoder->Push();
    m_encoder->SetUniformSIMDSize(SIMDMode::SIMD1);
    m_encoder->SetDstSubReg(5);
    m_encoder->Copy(m_destination, XOffset);
    m_encoder->Push();
    m_encoder->SetUniformSIMDSize(SIMDMode::SIMD1);
    m_encoder->SetDstSubReg(6);
    m_encoder->Copy(m_destination, YOffset);
    m_encoder->Push();
    m_encoder->SetUniformSIMDSize(SIMDMode::SIMD1);
    m_encoder->SetDstSubReg(7);
    m_encoder->Copy(m_destination, blkDimVar);
    m_encoder->Push();
}

void EmitPass::emitLSC2DBlockSetAddrPayloadField(GenIntrinsicInst* GII)
{
    CVariable* AP = GetSymbol(GII->getOperand(0));
    int tval = (int)cast<ConstantInt>(GII->getOperand(1))->getZExtValue();
    LSC2DBlockField Field = (LSC2DBlockField)tval;
    CVariable* cV = GetSymbol(GII->getOperand(2));
    uint32_t IsAddend = (uint32_t)cast<ConstantInt>(GII->getOperand(3))->getZExtValue();

    switch (Field) {
    case LSC2DBlockField::BASE:
    {
        CVariable* baseQW = m_currShader->GetNewAlias(AP, ISA_TYPE_Q, 0, 1, true);
        m_encoder->SetUniformSIMDSize(SIMDMode::SIMD1);
        m_encoder->SetDstSubReg(0);
        m_encoder->Copy(baseQW, cV);
        m_encoder->Push();
        break;
    }
    case LSC2DBlockField::BLOCKX:
    case LSC2DBlockField::BLOCKY:
    {
        const uint32_t OprdNum = (Field == LSC2DBlockField::BLOCKX ? 5 : 6);
        m_encoder->SetUniformSIMDSize(SIMDMode::SIMD1);
        m_encoder->SetDstSubReg(OprdNum);
        if (IsAddend > 0) {
            m_encoder->SetSrcSubReg(0, OprdNum);
            m_encoder->Add(AP, AP, cV);
        }
        else {
            m_encoder->Copy(AP, cV);
        }
        m_encoder->Push();
        break;
    }
    case LSC2DBlockField::WIDTH:
    case LSC2DBlockField::HEIGHT:
    case LSC2DBlockField::PITCH:
    {
        const uint32_t OprdNum = (Field == LSC2DBlockField::WIDTH ? 2
            : (Field == LSC2DBlockField::HEIGHT ? 3 : 4));
        m_encoder->SetUniformSIMDSize(SIMDMode::SIMD1);
        m_encoder->SetDstSubReg(OprdNum);
        m_encoder->Copy(AP, cV);
        m_encoder->Push();
        break;
    }
    default:
        IGC_ASSERT_MESSAGE(0, "Unknown LSC2DBlock field enum!");
    }
}

void EmitPass::emitLSC2DBlockReadWriteWithAddrPayload(GenIntrinsicInst* GII)
{
    const bool isPrefetch =
        (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockPrefetchAddrPayload);
    const bool isRead = (isPrefetch ||
        (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload));


    CVariable* AP = GetSymbol(GII->getOperand(0));
    CVariable* AddrPayload = AP;

    // Expect ImmX/ImmY to be immediate; but don't force it for ease of use.
    auto isImmLegit = [this](int N) {
        constexpr int S10Min = -512;
        constexpr int S10Max = 511;
        return  (N == 0 ||
            (m_pCtx->platform.LSC2DSupportImmXY() &&
             S10Min <= N && N <= S10Max));
    };

    Value* ImmXVal = GII->getOperand(1);
    Value* ImmYVal = GII->getOperand(2);
    ConstantInt* ImmXC = dyn_cast<ConstantInt>(ImmXVal);
    ConstantInt* ImmYC = dyn_cast<ConstantInt>(ImmYVal);
    int32_t ImmX = ImmXC ? (uint32_t)(ImmXC->getSExtValue()) : 0;
    int32_t ImmY = ImmYC ? (uint32_t)(ImmYC->getSExtValue()) : 0;
    // If ImmX/ImmY are not immediate, recreate a new payload
    const bool isXValid = (ImmXC && isImmLegit(ImmX));
    const bool isYValid = (ImmYC && isImmLegit(ImmY));
    if (!isXValid || !isYValid) {
        CVariable* APcopy = m_currShader->GetNewVariable(AP, "addrPayloadCopy");
        m_encoder->SetUniformSIMDSize(SIMDMode::SIMD8);
        m_encoder->SetSrcRegion(0, 1, 1, 0);
        m_encoder->Copy(APcopy, AP);
        m_encoder->Push();

        if (!isXValid) {
            CVariable* xVal =
                ImmXC ? m_currShader->ImmToVariable(ImmX, ISA_TYPE_D)
                      : GetSymbol(ImmXVal);
            m_encoder->SetUniformSIMDSize(SIMDMode::SIMD1);
            m_encoder->SetDstSubReg(5);
            m_encoder->SetSrcSubReg(0, 5);
            m_encoder->Add(APcopy, APcopy, xVal);
            m_encoder->Push();

            ImmX = 0;
        }

        if (!isYValid) {
            CVariable* yVal =
                ImmYC ? m_currShader->ImmToVariable(ImmY, ISA_TYPE_D)
                : GetSymbol(ImmYVal);
            m_encoder->SetUniformSIMDSize(SIMDMode::SIMD1);
            m_encoder->SetDstSubReg(6);
            m_encoder->SetSrcSubReg(0, 6);
            m_encoder->Add(APcopy, APcopy, yVal);
            m_encoder->Push();

            ImmY = 0;
        }
        AddrPayload = APcopy;
    }

    uint32_t eltBits = (uint32_t)cast<ConstantInt>(GII->getOperand(3))->getZExtValue();

    // Block width/height/numBlk not needed as they are already in AP.
    // But passing them for ease of reading.
    uint32_t blkWidth = (uint32_t)cast<ConstantInt>(GII->getOperand(4))->getZExtValue();
    uint32_t blkHeight = (uint32_t)cast<ConstantInt>(GII->getOperand(5))->getZExtValue();
    uint32_t numBlks = (uint32_t)cast<ConstantInt>(GII->getOperand(6))->getZExtValue();

    bool isTranspose = cast<ConstantInt>(GII->getOperand(7))->getZExtValue();
    bool isVnni = cast<ConstantInt>(GII->getOperand(8))->getZExtValue();

    ConstantInt* CacheVal = cast<ConstantInt>(GII->getOperand(9));
    LSC_CACHE_OPTS cacheOpts = translateLSCCacheControlsFromValue(CacheVal, isRead);

    CVariable* Src = nullptr;
    if (!isRead) {
        Src = BroadcastIfUniform(GetSymbol(GII->getOperand(10)));
    }

    m_encoder->LSC_2DBlockMessage(
        isRead ? LSC_LOAD_BLOCK2D : LSC_STORE_BLOCK2D,
        m_destination,
        AddrPayload,
        Src,
        ImmX, ImmY,
        eltBits,
        blkWidth,
        blkHeight,
        numBlks,
        isTranspose,
        isVnni,
        cacheOpts);
    m_encoder->Push();

}

void EmitPass::emitLSCFence(llvm::GenIntrinsicInst* inst)
{
    // Intrinsic format:
    //  Operand 0 - [immediate] memoryPort, (LSC_SFID enum)
    //                  ugm, ugml, tgm, slm
    //  Operand 1 - [immediate] scope of the fence, (LSC_SCOPE enum)
    //                  threadgroup, local, tile, GPU, all GPUs, system Release, system Acquire
    //  Operand 2 - [immediate] flush type, (LSC_FENCE_OP enum)
    //                  evict, invalidate, discard, clean, flushl3

    LSC_SFID memoryPort     = static_cast<LSC_SFID>(cast<ConstantInt>(inst->getOperand(0))->getZExtValue());
    LSC_SCOPE scope         = static_cast<LSC_SCOPE>(cast<ConstantInt>(inst->getOperand(1))->getZExtValue());
    LSC_FENCE_OP flushType  = static_cast<LSC_FENCE_OP>(cast<ConstantInt>(inst->getOperand(2))->getZExtValue());

    if (memoryPort == LSC_SLM)
    {
        if (m_currShader->GetShaderType() == ShaderType::OPENCL_SHADER) {
            Function* F = inst->getParent()->getParent();
            MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
            uint32_t sz = IGCMetaDataHelper::getThreadGroupSize(*pMdUtils, F);
            if (sz != 0 && sz <= numLanes(m_SimdMode)) {
                return;
            }
        }
        /*
        else if (m_currShader->GetShaderType() == ShaderType::MESH_SHADER ||
            m_currShader->GetShaderType() == ShaderType::TASK_SHADER)
        {
            unsigned int threadGroupSize =
                static_cast<MeshShaderContext*>(m_currShader->GetContext())->GetThreadGroupSize();
            if (threadGroupSize <= numLanes(m_SimdMode))
            {
                return;
            }
        }*/
    }

    if ((memoryPort != LSC_UGM) && (memoryPort != LSC_TGM) &&
        (scope == LSC_SCOPE_SYSREL || scope == LSC_SCOPE_SYSACQ))
    {
        // Does this restriction apply also to LSC_UGML ?
        IGC_ASSERT_EXIT_MESSAGE(0, "LSC system scope is available for UGM data-port only.");
    }

    // Change the scope from `GPU` to `Tile` on single-tile platforms to avoid L3 flush on DG2 and MTL and ARL.
    if (scope == LSC_SCOPE_GPU &&
        !m_currShader->m_Platform->hasMultiTile() &&
        m_currShader->m_Platform->hasL3FlushOnGPUScopeInvalidate() &&
        IGC_IS_FLAG_DISABLED(EnableGPUFenceScopeOnSingleTileGPUs))
    {
        scope = LSC_SCOPE_TILE;
    }

    m_encoder->LSC_Fence(memoryPort, scope, flushType);
    m_encoder->Push();
}

unsigned short getLSCAtomicBitWidth(llvm::GenIntrinsicInst* inst)
{
    llvm::StringRef name = inst->getCalledFunction()->getName();
    unsigned short bitwidth = 0;
    if (name.startswith("llvm.genx.GenISA.LSCAtomicInts.i64") || name.startswith("llvm.genx.GenISA.LSCAtomicInts.u64")
        || name.startswith("llvm.genx.GenISA.LSCAtomicFP64"))
        bitwidth = 64;
    else if (name.startswith("llvm.genx.GenISA.LSCAtomicInts.i32") || name.startswith("llvm.genx.GenISA.LSCAtomicInts.u32")
        || name.startswith("llvm.genx.GenISA.LSCAtomicFP32"))
        bitwidth = 32;
    else if (name.startswith("llvm.genx.GenISA.LSCAtomicInts.i16") || name.startswith("llvm.genx.GenISA.LSCAtomicInts.u16")
        )
        bitwidth = 16;
    else
        IGC_ASSERT_MESSAGE(0, "Intrinsic support is not implemented.");
    return bitwidth;
}

void EmitPass::emitLSCAtomic(llvm::GenIntrinsicInst* inst)
{
    // Intrinsic format:
    //  oldValue  - [non-uniform] data read from memory
    //
    //  Operand 0 - [non-uniform] data,vector to be stored into memory from registers
    //  Operand 1 - [immediate] element offset
    //  Operand 2 - [non-uniform] value, cmpval(cmpxchg)
    //  Operand 3 - [non-uniform] value (cmpxchg)
    //  Operand 4 - [immediate] atomic operation (AtomicOp add,sub,etc.)
    //  Operand 5 - [immediate] cache controls
    CVariable* pOldValue = (inst->use_empty()) ? nullptr : m_destination;

    Value* Ptr = inst->getArgOperand(0);
    CVariable* pDstAddr = GetSymbol(Ptr);

    PointerType* ptrType = cast<PointerType>(Ptr->getType());
    bool isA64 = isA64Ptr(ptrType, m_currShader->GetContext());
    LSC_ADDR_SIZE addrSize = isA64 ? LSC_ADDR_SIZE_64b : LSC_ADDR_SIZE_32b;
    ResourceDescriptor resource = GetResourceVariable(Ptr);
    if (Ptr->getType()->getPointerAddressSpace() == ADDRESS_SPACE_GLOBAL)
    {
        m_currShader->SetHasGlobalAtomics();
    }

    int immOff = (int)cast<ConstantInt>(inst->getOperand(1))->getSExtValue();

    AtomicOp atomicOp = static_cast<AtomicOp>(cast<ConstantInt>(inst->getOperand(4))->getZExtValue());
    bool isNoSrc1Atomic = ((atomicOp == EATOMIC_INC) || (atomicOp == EATOMIC_DEC) || (atomicOp == EATOMIC_LOAD));

    CVariable* pAtomicVal = (!isNoSrc1Atomic) ? GetSymbol(inst->getArgOperand(2)) : nullptr;
    pAtomicVal = (pAtomicVal != nullptr) ? BroadcastIfUniform(pAtomicVal) : pAtomicVal;

    if (atomicOp == EATOMIC_FADD64 || atomicOp == EATOMIC_FSUB64)
    {
        IGC_ASSERT_EXIT_MESSAGE(isA64, "For LSC Atomics FADD64,FSUB64 address size must be 64");
    }

    CVariable* pAtomicCmp = (atomicOp == EATOMIC_CMPXCHG || atomicOp == EATOMIC_FCMPWR) ?
        GetSymbol(inst->getArgOperand(3)) : nullptr;
    pAtomicCmp = (pAtomicCmp != nullptr) ? BroadcastIfUniform(pAtomicCmp) : pAtomicCmp;

    // take the bitwidth from the intrinsic name, the return type might
    // differ; e.g. uint lsc_atomic_add(ushort *, uint) D16U32
    unsigned short bitwidth = getLSCAtomicBitWidth(inst);
    pDstAddr = ReAlignUniformVariable(pDstAddr, EALIGN_GRF);

    auto cacheOpts = translateLSCCacheControlsFromValue(inst->getOperand(5), false);

    m_encoder->LSC_AtomicRaw(atomicOp, pOldValue,
                             pDstAddr, pAtomicVal, pAtomicCmp, bitwidth,
                             &resource, addrSize, immOff, 1, cacheOpts);
    m_encoder->Push();
}

static bool isLSCAtomic(llvm::GenISAIntrinsic::ID id)
{
    return ((id == GenISAIntrinsic::GenISA_LSCAtomicFP64) ||
            (id == GenISAIntrinsic::GenISA_LSCAtomicFP32) ||
            (id == GenISAIntrinsic::GenISA_LSCAtomicInts));
}

void EmitPass::emitLSCIntrinsic(llvm::GenIntrinsicInst* GII)
{
    GenISAIntrinsic::ID iid = GII->getIntrinsicID();
    switch (iid) {
    case GenISAIntrinsic::GenISA_LSCLoad:
    case GenISAIntrinsic::GenISA_LSCLoadWithSideEffects:
    case GenISAIntrinsic::GenISA_LSCLoadBlock:
        emitLscIntrinsicLoad(GII);
        break;
    case GenISAIntrinsic::GenISA_LSCLoadCmask:
        emitLscIntrinsicLoadCmask(GII);
        break;
    case GenISAIntrinsic::GenISA_LSCPrefetch:
    case GenISAIntrinsic::GenISA_LSCLoadStatus:
        emitLscIntrinsicPrefetch(GII);
        break;
        //
    case GenISAIntrinsic::GenISA_LSCSimdBlockPrefetch:
        emitLscSimdBlockPrefetch(GII);
        break;
    case GenISAIntrinsic::GenISA_LSCStore:
    case GenISAIntrinsic::GenISA_LSCStoreBlock:
        emitLscIntrinsicStore(GII);
        break;
    case GenISAIntrinsic::GenISA_LSCStoreCmask:
        emitLscIntrinsicStoreCmask(GII);
        break;
    case GenISAIntrinsic::GenISA_LSCFence:
        emitLSCFence(GII);
        break;
    case GenISAIntrinsic::GenISA_LSC2DBlockRead:
    case GenISAIntrinsic::GenISA_LSC2DBlockPrefetch:
    case GenISAIntrinsic::GenISA_LSC2DBlockWrite:
        emitLSC2DBlockOperation(GII);
        break;
    case GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload:
    case GenISAIntrinsic::GenISA_LSC2DBlockPrefetchAddrPayload:
    case GenISAIntrinsic::GenISA_LSC2DBlockWriteAddrPayload:
        emitLSC2DBlockReadWriteWithAddrPayload(GII);
        break;
    case GenISAIntrinsic::GenISA_LSC2DBlockCreateAddrPayload:
    case GenISAIntrinsic::GenISA_LSC2DBlockCopyAddrPayload:
        emitLSC2DBlockAddrPayload(GII);
        break;
    case GenISAIntrinsic::GenISA_LSC2DBlockSetAddrPayloadField:
        emitLSC2DBlockSetAddrPayloadField(GII);
        break;
    default:
        if (isLSCAtomic(iid)) { ////// GenISA_LSCAtomic*
            emitLSCAtomic(GII);
        } else {
            IGC_ASSERT_EXIT_MESSAGE(0, "unmapped intrinsic");
        }
    }
}
LSC_CACHE_OPTS EmitPass::getDefaultRaytracingCachePolicy(bool isLoad) const
{
    LSC_CACHE_OPTS DefaultCacheCtrl {LSC_CACHING_DEFAULT, LSC_CACHING_DEFAULT};

    if (IGC_IS_FLAG_ENABLED(ForceGenMemDefaultCacheCtrl))
        return DefaultCacheCtrl;

    LSC_L1_L3_CC Opts;

    if (isLoad)
    {
        Opts = IGC_IS_FLAG_ENABLED(ForceGenMemLoadCacheCtrl) ?
            (LSC_L1_L3_CC)IGC_GET_FLAG_VALUE(GenMemLoadCacheCtrl) :
            LSC_L1C_WT_L3C_WB;
    }
    else
    {
        Opts = IGC_IS_FLAG_ENABLED(ForceGenMemStoreCacheCtrl) ?
            (LSC_L1_L3_CC)IGC_GET_FLAG_VALUE(GenMemStoreCacheCtrl) :
            LSC_L1IAR_WB_L3C_WB;
    }

    return translateLSCCacheControlsEnum(Opts, isLoad, nullptr);
}

void EmitPass::emitAsyncStackID(llvm::GenIntrinsicInst *I)
{
    IGC_ASSERT(m_currShader->GetShaderType() == ShaderType::RAYTRACING_SHADER);
}

void EmitPass::emitGlobalBufferPtr(llvm::GenIntrinsicInst* I)
{
    IGC_ASSERT(m_currShader->GetShaderType() == ShaderType::RAYTRACING_SHADER);
}

void EmitPass::emitLocalBufferPtr(llvm::GenIntrinsicInst* I)
{
    IGC_ASSERT(m_currShader->GetShaderType() == ShaderType::RAYTRACING_SHADER);
}

void EmitPass::emitInlinedDataValue(llvm::GenIntrinsicInst* I)
{
    IGC_ASSERT(m_currShader->GetShaderType() == ShaderType::RAYTRACING_SHADER);
}

void EmitPass::emitTraceRay(TraceRayIntrinsic* I, bool RayQueryEnable)
{

    // We emit a SW fence here to prevent motion of other sends across this
    // send.rta until we have VISA support. An actual fence was previously
    // inserted in the RayTracingShaderLowering pass
    m_encoder->Fence(false,
        false,
        false,
        false,
        false,
        false,
        false,
        true);
    m_encoder->Push();

    const uint32_t NumSend =
        (m_currShader->m_SIMDSize == SIMDMode::SIMD32
            ) ? 2 : 1;

    for (uint32_t Cnt = 0; Cnt < NumSend; Cnt++)
    {
        // 'payload' contains bvhLevel, traceRayCtrl, and stackID as per
        // TraceRayMessage::Payload in RTStackFormat.h.
        CVariable* payload = BroadcastIfUniform(GetSymbol(I->getPayload()));
        CVariable* globalBufferPtr = GetSymbol(I->getGlobalBufferPointer());
        if (!globalBufferPtr->IsUniform())
        {
            // since the global pointer has been split between the upper and
            // lower lanes in the SIMD32 case, we need to extract each half out.
            CVariable* TmpGP = globalBufferPtr;
            if (globalBufferPtr->GetNumberInstance() == 2)
            {
                TmpGP = m_currShader->GetNewVariable(
                    globalBufferPtr->GetNumberElement(),
                    globalBufferPtr->GetType(),
                    EALIGN_GRF,
                    "TmpGP");
                m_encoder->Copy(TmpGP, globalBufferPtr);
                globalBufferPtr = UniformCopy(TmpGP, true);
            }
            else if (m_currShader->m_SIMDSize == SIMDMode::SIMD32
                )
            {
                auto *Src = m_currShader->GetNewAlias(
                    globalBufferPtr,
                    globalBufferPtr->GetType(),
                    CEncoder::GetCISADataTypeSize(
                        globalBufferPtr->GetType()) * 16 * Cnt,
                    16);
                auto* eMask = GetExecutionMask();
                eMask = m_currShader->GetNewAlias(
                    eMask, ISA_TYPE_UW, 2 * Cnt, 1);
                CVariable* off = nullptr;
                globalBufferPtr = UniformCopy(Src, off, eMask);
            }
            else
            {
                globalBufferPtr = UniformCopy(TmpGP);
            }
        }

        CVariable* header = m_currShader->GetNewVariable(
            getGRFSize() / SIZE_DWORD, ISA_TYPE_UD, EALIGN_GRF, "RTHeader");

        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
        m_encoder->Copy(
            m_currShader->BitCast(header, globalBufferPtr->GetType()),
            globalBufferPtr);
        m_encoder->Push();

        {
            // Initialize RayQuery Enable bit
            constexpr uint32_t RayQueryDword =
                offsetof(RTStackFormat::TraceRayMessage::Header, rayQueryLocation) / sizeof(DWORD);
            static_assert(RayQueryDword == 4, "header change?");

            uint64_t rayQueryHeader = 0x0;

            rayQueryHeader |= RayQueryEnable ? 1 : 0;

            CVariable* RayQueryVal =
                m_currShader->ImmToVariable(rayQueryHeader, ISA_TYPE_UD);

            m_encoder->SetSimdSize(SIMDMode::SIMD1);
            m_encoder->SetNoMask();
            m_encoder->SetDstSubReg(RayQueryDword);
            m_encoder->Copy(header, RayQueryVal);
            m_encoder->Push();
        }

        const bool WideTraceRay =
            m_currShader->m_Platform->getPlatformInfo().eRenderCoreFamily == IGFX_XE_HPC_CORE ||
            (getGRFSize() == 64 && IGC_IS_FLAG_ENABLED(DisableWideTraceRay));

        if (WideTraceRay)
        {
            // PVC SIMD16 TraceRay payload needs to be expanded to two registers
            // since RTA expects the same payload format as DG2 (which has 32-byte
            // registers).
            //
            if (Cnt == 1)
                m_encoder->SetSrcSubVar(0, 1);
            IGC_ASSERT_MESSAGE(payload->GetNumberElement() >= 16, "not simd16?");
            CVariable* expandedPayload = m_currShader->GetNewVariable(
                getGRFSize() * 2 / SIZE_DWORD, ISA_TYPE_UD, EALIGN_GRF,
                "ExpandedPayload");
            m_encoder->SetSimdSize(SIMDMode::SIMD8);
            m_encoder->Copy(expandedPayload, payload);

            m_encoder->SetSrcSubReg(0, 8);
            m_encoder->SetDstSubVar(1);
            m_encoder->SetMask(EMASK_Q2);
            m_encoder->Copy(expandedPayload, payload);
            m_encoder->Push();

            payload = expandedPayload;
        }

        const unsigned int extDescriptor = EU_MESSAGE_TARGET_SFID_RTA;
        CVariable* exDesc = m_currShader->ImmToVariable(extDescriptor, ISA_TYPE_UD);

        uint messageSpecificControl = BindlessThreadDispatch(
            1,
            m_currShader->m_SIMDSize >= SIMDMode::SIMD16 ? 1 : 0,
            true,
            RayQueryEnable);

        CVariable* pMessDesc =
            m_currShader->ImmToVariable(messageSpecificControl, ISA_TYPE_UD);

        CVariable* Dst = RayQueryEnable ? m_destination : nullptr;
        if (m_currShader->m_SIMDSize == SIMDMode::SIMD32
            )
        {
            m_encoder->SetSimdSize(SIMDMode::SIMD16);
            if (Cnt == 1)
                m_encoder->SetMask(EMASK_H2);

            if (!WideTraceRay)
            {
                payload = m_currShader->GetNewAlias(
                    payload,
                    payload->GetType(),
                    CEncoder::GetCISADataTypeSize(payload->GetType()) * 16 * Cnt,
                    16);
            }

            if (Dst)
            {
                Dst = m_currShader->GetNewAlias(
                    Dst,
                    Dst->GetType(),
                    CEncoder::GetCISADataTypeSize(Dst->GetType()) * 16 * Cnt,
                    16);
            }
        }

        m_encoder->Sends(
            Dst,
            header,
            payload,
            extDescriptor,
            exDesc,
            pMessDesc,
            false);

        m_encoder->Push();
    }

    // Insert a software fence after the send.rta so no IO operations get
    // scheduled across the send from below.  We should be able to remove this
    // once we have VISA support for raytracing rather than emitting a
    // raw_sends.
    m_encoder->Fence(false,
        false,
        false,
        false,
        false,
        false,
        false,
        true);
    m_encoder->Push();
}


void EmitPass::emitReadTraceRaySync(llvm::GenIntrinsicInst* I)
{
    // Insert a software fence before and after the ReadTraceRaySync so no IO operations get
    // scheduled across it.
    m_encoder->Fence(false,
        false,
        false,
        false,
        false,
        false,
        false,
        true);
    m_encoder->Push();

    auto Var = GetSymbol(I->getOperand(0));
    m_encoder->SetUniformSIMDSize(m_currShader->m_SIMDSize);
    m_encoder->Cast(m_currShader->GetNULL(), Var);
    m_encoder->Push();

    m_encoder->Fence(false,
        false,
        false,
        false,
        false,
        false,
        false,
        true);
    m_encoder->Push();
}

void EmitPass::emitRayQueryCheckRelease(
    GenIntrinsicInst* I,
    bool RayQueryCheckEnable,
    bool RayQueryReleaseEnable)
{

    // We emit a SW fence here to prevent motion of other sends across this
    // send.rta until we have VISA support. An actual fence was previously
    // inserted in the RayTracingShaderLowering pass.
    m_encoder->Fence(false,
        false,
        false,
        false,
        false,
        false,
        false,
        true);
    m_encoder->Push();

    CVariable* header = m_currShader->GetNewVariable(
        getGRFSize() / SIZE_DWORD, ISA_TYPE_UD, EALIGN_GRF, "RTHeader");

    {
        // Initialize RayQuery Enable bit
        constexpr uint32_t RayQueryDword =
            offsetof(RTStackFormat::TraceRayMessage::Header, rayQueryLocation) / sizeof(DWORD);
        static_assert(RayQueryDword == 4, "header change?");

        uint64_t rayQueryHeader = 0x1; // RayQueryEnable bit.

        rayQueryHeader |= RayQueryCheckEnable ? 0x2 : 0;
        rayQueryHeader |= RayQueryReleaseEnable ? 0x4 : 0;

        CVariable* RayQueryVal =
            m_currShader->ImmToVariable(rayQueryHeader, ISA_TYPE_UD);

        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
        m_encoder->SetDstSubReg(RayQueryDword);
        m_encoder->Copy(header, RayQueryVal);
        m_encoder->Push();
    }

    const unsigned int extDescriptor = EU_MESSAGE_TARGET_SFID_RTA;
    CVariable* exDesc = m_currShader->ImmToVariable(extDescriptor, ISA_TYPE_UD);

    uint messageSpecificControl = BindlessThreadDispatch(
        1,
        m_currShader->m_SIMDSize >= SIMDMode::SIMD16 ? 1 : 0,
        true,
        false
        // If this is RayQuery Release message, there is no grf
        // returned.
        && !RayQueryReleaseEnable
    );

    CVariable* pMessDesc =
        m_currShader->ImmToVariable(messageSpecificControl, ISA_TYPE_UD);

    // Messages to rta unit require 2 sources. But Check/Release
    // messages do not send any data is src1.
    // To handle this, a dummy source is created.
    CVariable* dummySource = m_currShader->GetNewVariable(
        getGRFSize() / SIZE_DWORD, ISA_TYPE_UD, EALIGN_GRF, "dummySource");

    m_encoder->Lifetime(LIFETIME_START, dummySource);


    CVariable* flag = nullptr;

    if (auto* CI = dyn_cast<ConstantInt>(I->getOperand(0));
        !CI || !CI->isAllOnesValue())
    {
        flag = GetSymbol(I->getOperand(0));
    }

    m_encoder->SetPredicate(flag);

    m_encoder->Sends(
        nullptr,
        header,
        dummySource,
        extDescriptor,
        exDesc,
        pMessDesc);

    m_encoder->Push();

    // Insert a software fence after the send.rta so no IO operations get
    // scheduled across the send from below.  We should be able to remove this
    // once we have VISA support for raytracing rather than emitting a
    // raw_sends.
    m_encoder->Fence(false,
        false,
        false,
        false,
        false,
        false,
        false,
        true);
    m_encoder->Push();
}


EmitPass::PreemptionEncoding
EmitPass::getEncoderPreemptionMode(EPreemptionMode preemptionMode)
{
    switch (preemptionMode)
    {
    default:
        break;
    case PREEMPTION_ENABLED:
        return PreemptionEncoding::PreemptionEnabled;
    case PREEMPTION_DISABLED:
        return PreemptionEncoding::PreemptionDisabled;
    }

    return PreemptionEncoding::PreemptionEnabled;
}

void EmitPass::emitPreemptionDisable(PreemptionDisableIntrinsic* PDI)
{
    CVariable *Mask = m_currShader->ImmToVariable(
        ~getEncoderPreemptionMode(PREEMPTION_ENABLED),
        ISA_TYPE_UD);

    m_encoder->And(m_currShader->GetCR0(), m_currShader->GetCR0(), Mask);
    m_encoder->Push();
}

void EmitPass::emitPreemptionEnable(PreemptionEnableIntrinsic* PEI)
{
    if (m_currShader->GetShaderType() == ShaderType::OPENCL_SHADER)
    {
        COpenCLKernel* kernel = static_cast<COpenCLKernel*>(m_currShader);
        if (kernel->GetDisableMidThreadPreemption())
            return;
    }

    CVariable* Flag = nullptr;
    if (auto* CI = dyn_cast<ConstantInt>(PEI->getFlag()))
    {
        if (CI->isZero())
            return;
    }
    else
    {
        Flag = GetSymbol(PEI->getFlag());
    }

    CVariable *Mask = m_currShader->ImmToVariable(
        getEncoderPreemptionMode(PREEMPTION_ENABLED),
        ISA_TYPE_UD);

    m_encoder->SetPredicate(Flag);
    m_encoder->Or(m_currShader->GetCR0(), m_currShader->GetCR0(), Mask);
    m_encoder->Push();
}

void EmitPass::emitBTD(
    CVariable* GlobalBufferPtr,
    CVariable* StackID,
    CVariable* ShaderRecord,
    CVariable* Flag,
    bool releaseStackID)
{


    CVariable* payload = m_currShader->GetNewVariable(
        2 * getGRFSize() / SIZE_DWORD, ISA_TYPE_UD, EALIGN_GRF, CName::NONE);

    // Global Pointer [63:6]
    if (GlobalBufferPtr)
    {
        if (!GlobalBufferPtr->IsUniform())
            GlobalBufferPtr = UniformCopy(GlobalBufferPtr);
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
        m_encoder->Copy(
            m_currShader->BitCast(payload, GlobalBufferPtr->GetType()),
            GlobalBufferPtr);
        m_encoder->Push();
    }

    if (!releaseStackID && m_currShader->m_Platform->canSupportWMTP())
    {

        // We just need to initialize this to 0 on a normal spawn.
        CVariable* Zero = m_currShader->ImmToVariable(0x0, ISA_TYPE_UD);
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
        m_encoder->SetDstSubReg(2);
        m_encoder->Copy(payload, Zero);
        m_encoder->Push();
    }

    StackID = BroadcastIfUniform(StackID);
    // StackID[15:0]
    m_encoder->SetDstSubVar(1);
    m_encoder->Copy(m_currShader->BitCast(payload, ISA_TYPE_W), StackID);
    m_encoder->Push();

    // the lsb of the global pointer set to '1' indicates the stack ID
    // should be released.
    if (releaseStackID)
    {
        // Preempted StackID Release [1] (Xe2)
        // StackID Release           [0]
        CVariable* Alias = m_currShader->GetNewAlias(payload, ISA_TYPE_UD, 0, 1);
        CVariable* Bit = m_currShader->ImmToVariable(0x1, ISA_TYPE_UD);
        m_encoder->SetSimdSize(SIMDMode::SIMD1);
        m_encoder->SetNoMask();
        m_encoder->Copy(Alias, Bit);
        m_encoder->Push();
    }

    uint messageSpecificControl = BindlessThreadDispatch(
        2,
        m_currShader->m_State.m_dispatchSize == SIMDMode::SIMD16 ? 1 : 0,
        false,
        false);
    CVariable* pMessDesc = m_currShader->ImmToVariable(messageSpecificControl, ISA_TYPE_UD);

    unsigned int extDescriptor = EU_MESSAGE_TARGET_SFID_BTD;
    CVariable* exDesc = m_currShader->ImmToVariable(extDescriptor, ISA_TYPE_UD);

    if (ShaderRecord)
    {
        ShaderRecord = BroadcastIfUniform(ShaderRecord);
    }
    else
    {
        ShaderRecord = m_currShader->GetNewVariable(
            numLanes(m_currShader->m_SIMDSize), ISA_TYPE_UQ, EALIGN_GRF, CName::NONE);
    }

    // We emit a SW fence here to prevent motion of other sends across this
    // send.btd until we have VISA support. An actual fence was previously
    // inserted in the RayTracingShaderLowering pass.
    m_encoder->Fence(false,
        false,
        false,
        false,
        false,
        false,
        false,
        true);
    m_encoder->Push();

    m_encoder->SetPredicate(Flag);
    m_encoder->Sends(
        nullptr,
        payload,
        ShaderRecord,
        EU_MESSAGE_TARGET_SFID_BTD,
        exDesc,
        pMessDesc);
    m_encoder->Push();

    // Insert a software fence after the send.btd so no IO operations get
    // scheduled across the send from below.  We should be able to remove this
    // once we have VISA support for raytracing rather than emitting a
    // raw_sends.
    m_encoder->Fence(false,
        false,
        false,
        false,
        false,
        false,
        false,
        true);
    m_encoder->Push();
}


void EmitPass::emitBindlessThreadDispatch(BTDIntrinsic* I)
{
    CVariable* globalBufferPtr = GetSymbol(I->getGlobalBufferPointer());
    CVariable* stackID = GetSymbol(I->getStackID());
    CVariable* shaderRecord = GetSymbol(I->getShaderRecordAddress());

    emitBTD(
        globalBufferPtr,
        stackID,
        shaderRecord,
        nullptr,
        false);
}

void EmitPass::emitStackIDRelease(StackIDReleaseIntrinsic* I)
{
    CVariable* stackID = GetSymbol(I->getStackID());
    CVariable* flag = nullptr;

    if (auto* CI = dyn_cast<ConstantInt>(I->getPredicate());
        !CI || !CI->isAllOnesValue())
    {
        flag = GetSymbol(I->getPredicate());
    }

    emitBTD(
        nullptr,
        stackID,
        nullptr,
        flag,
        true);
}

void EmitPass::emitGetShaderRecordPtr(GetShaderRecordPtrIntrinsic* I)
{
    // Functions currently reside in the default address space (0) which, for
    // DX, has 32-bit pointers.  LLVM 8 introduced the ability to set the
    // address space for functions.  We may move to using that once we upgrade.
    // For now, we go through this intrinsic to ensure that we have a 64-bit
    // value going out so the to 32-bits are not truncated away.
    auto* Fn = I->getContinuationFn();
    auto* Var = m_currShader->CreateFunctionSymbol(Fn);

    IGC_ASSERT(Var->GetType() == ISA_TYPE_UQ);
    IGC_ASSERT(m_destination->GetType() == ISA_TYPE_UQ);

    m_encoder->Copy(m_destination, Var);
    m_encoder->Push();
}

void EmitPass::emitSystemMemoryFence(llvm::GenIntrinsicInst* inst)
{
    IGC_ASSERT_MESSAGE(shouldGenerateLSC(), "LSC must be enabled for system memory fence");
    bool fenceTGM = llvm::cast<llvm::ConstantInt>((inst->getOperand(0)))->getValue().getBoolValue();

    if (fenceTGM)
    {
        // first fence TGM with GPU scope
        auto flushOpt = getLSCMemoryFenceOp(true, false, m_currShader->m_Platform->hasSamplerSupport());
        m_encoder->LSC_Fence(LSC_TGM, LSC_SCOPE_GPU, flushOpt);
        m_encoder->Push();
        // then emit the regular UGM fence
    }

    m_encoder->LSC_Fence(LSC_UGM, LSC_SCOPE_SYSREL, LSC_FENCE_OP_EVICT);
    m_encoder->Push();
}

void EmitPass::emitUrbFence()
{
    if (CVariable* const writeback = m_currShader->URBFence())
    {
        CVariable* dstnull = m_currShader->GetNULL();
        m_encoder->Copy(dstnull, writeback);
    }
}

void EmitPass::emitHDCuncompressedwrite(llvm::GenIntrinsicInst* inst)
{
    if (shouldGenerateLSC())
    {
        emitLSCSimdBlockWrite(inst);
        return;
    }
    emitLegacySimdBlockWrite(inst);
}

bool EmitPass::forceCacheCtrl(llvm::Instruction* inst)
{
    return m_currShader->forceCacheCtrl(inst);
}

// This function may be used in earlier passes to determine whether a given
// instruction will generate an LSC message. If it returns Unknown or False, you
// should conservatively assume that you don't know what will be generated. If
// this returns True, it is guaranteed that an LSC message will result.
Tristate EmitPass::shouldGenerateLSCQuery(
    const CodeGenContext& Ctx,
    Instruction* vectorLdStInst,
    SIMDMode Mode)
{
    return CShader::shouldGenerateLSCQuery(Ctx, vectorLdStInst, Mode);
}

// Note that if LSCEnabled() returns true, load/store instructions must be
// generated with LSC; but some intrinsics are still generated with legacy.
bool EmitPass::shouldGenerateLSC(llvm::Instruction* vectorLdStInst, bool isTGM)
{
    return m_currShader->shouldGenerateLSC(vectorLdStInst, isTGM);
} // shouldGenerateLSC

uint32_t EmitPass::totalBytesToStoreOrLoad(llvm::Instruction* vectorLdStInst)
{
    return m_currShader->totalBytesToStoreOrLoad(vectorLdStInst);
} // totalBytesToStoreOrLoad


void EmitPass::emitCastSelect(CVariable* flag, CVariable* dst, CVariable* src0, CVariable* src1)
{
    // For partialEmuI64 enabled and I64 dst type do the below sequence
    //      1.cast I64 dst to I32 dst
    //      2.select with I32 dst
    //      3.cast the result back to I64 type
    CVariable* tmpDst = m_currShader->GetNewVariable(dst);
    tmpDst = m_currShader->BitCast(tmpDst, dst->GetDTypeFromQType());
    m_encoder->Select(flag, tmpDst, src0, src1);
    m_encoder->Push();
    m_encoder->Cast(dst, tmpDst);
}


CVariable* EmitPass::getStackSizePerThread(Function* parentFunc) {
    CVariable *pSize = nullptr;
    auto pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    auto pModMD = pCtx->getModuleMetaData();

    Function *groupHead = nullptr;
    if (!m_FGA || m_FGA->isGroupHead(parentFunc)) {
        groupHead = parentFunc;
    } else {
        groupHead = m_FGA->getSubGroupMap(parentFunc);
    }

    Function *pKernel = groupHead;
    IGC_ASSERT(pModMD->FuncMD.find(pKernel) != pModMD->FuncMD.end());

    auto stackMemIter = pModMD->PrivateMemoryPerFG.find(pKernel);
    IGC_ASSERT(stackMemIter != pModMD->PrivateMemoryPerFG.end());
    unsigned MaxPrivateSize = stackMemIter->second;

    // hard-code per-workitem private-memory size to max size
    pSize = m_currShader->ImmToVariable(
        MaxPrivateSize * numLanes(m_currShader->m_State.m_dispatchSize),
        ISA_TYPE_UD);

    return pSize;
}

Function* EmitPass::findStackOverflowDetectionFunction(Function* ParentFunction, bool FindInitFunction) {
    const char *FunctionNames[] = {
        "__stackoverflow_init",
        "__stackoverflow_detection"
    };
    const char *FunctionName = (FindInitFunction ? FunctionNames[0] : FunctionNames[1]);

    auto FG = m_FGA->getGroup(ParentFunction);
    Function *StackOverflowFunction = nullptr;
    // Function subgroup can contain clones of the subroutine.
    for (auto F : *FG) {
        if (F->getName().startswith(FunctionName) &&
            m_FGA->getSubGroupMap(ParentFunction) ==
                m_FGA->getSubGroupMap(F)) {
            StackOverflowFunction = F;
            break;
        }
    }
    return StackOverflowFunction;
}

void EmitPass::emitStackOverflowDetectionCall(Function* ParentFunction, bool EmitInitFunction) {
    Function *StackOverflowFunction = findStackOverflowDetectionFunction(ParentFunction, EmitInitFunction);

    if (StackOverflowFunction) {
        m_currFuncHasSubroutine = true;

        m_encoder->SubroutineCall(nullptr, StackOverflowFunction);
        m_encoder->Push();
    } else if (!EmitInitFunction) {
        IGC_ASSERT_MESSAGE(false,
            "Couldn't find __stackoverflow_detection function in the module!");
    }
}
