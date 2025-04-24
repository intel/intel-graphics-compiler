/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/DebugInfo/ScalarVISAModule.h"
#include "Compiler/DebugInfo/Utils.h"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs/KernelArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/PrivateMemory/PrivateMemoryToSLM.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/debug/Debug.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "common/LLVMWarningsPop.hpp"

#include "Probe/Assertion.h"

using namespace llvm;

namespace IGC {

/*static*/ bool DebugMetadataInfo::hasDashGOption(CodeGenContext* ctx)
{
    return ctx->getModuleMetaData()->compOpt.DashGSpecified;
}

/*static*/ bool DebugMetadataInfo::hasAnyDebugInfo(CodeGenContext* ctx, bool& fullDebugInfo, bool& lineNumbersOnly)
{
    Module* module = ctx->getModule();
    bool hasFullDebugInfo = false;
    fullDebugInfo = false;
    lineNumbersOnly = false;

    if (Utils::HasDebugInfo(*ctx->getModule()))
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

ScalarVisaModule::ScalarVisaModule(CShader *TheShader,
                                   llvm::Function *TheFunction, bool IsPrimary,
                                   bool IsStackCallContext)
    : m_pShader(TheShader),
      VISAModule(TheFunction, IsPrimary, IsStackCallContext) {
    UpdateVisaId();
}

std::unique_ptr<IGC::VISAModule>
ScalarVisaModule::BuildNew(CShader *S, llvm::Function *F, bool IsPrimary,
                           bool IsStackCallContext) {
    auto *n = new ScalarVisaModule(S, F, IsPrimary, IsStackCallContext);
    return std::unique_ptr<IGC::VISAModule>(n);
}

unsigned ScalarVisaModule::getPrivateBaseReg() const
{
    auto pVar = privateBase;
    unsigned privateBaseRegNum = m_pShader->GetDebugInfoData().getVISADclId(pVar, 0);
    return privateBaseRegNum;;
}

int ScalarVisaModule::getPTOReg() const {
    IGC_ASSERT_MESSAGE(hasPTO(), "PTO instruction required");

    IGC_ASSERT_MESSAGE(m_perThreadOffset, "Per Thread Offset variable does not exist");
    IGC_ASSERT_MESSAGE(m_perThreadOffset->GetVarType() == EVARTYPE_GENERAL, "Unexpected VISA register type!");

    int regPTO = getDeclarationID(m_perThreadOffset, false);
    return regPTO;
}
int ScalarVisaModule::getFPReg() const {
    CVariable *framePtr = getFramePtr();
    // TBD: IGC_ASSERT_MESSAGE(framePtr, "Frame Pointer does not exist");
    int regFP = getDeclarationID(framePtr, false);
    return regFP;
}

llvm::StringRef ScalarVisaModule::GetVISAFuncName() const
{
    // since device enqueue no longer used, llvm's function name
    // matches that used by VISA.
    return getFunction()->getName();
}
uint64_t ScalarVisaModule::getFPOffset() const {
    auto funcMDItr = m_pShader->m_ModuleMetadata->FuncMD.find(getFunction());
    IGC_ASSERT(funcMDItr != m_pShader->m_ModuleMetadata->FuncMD.end());
    return funcMDItr->second.prevFPOffset;
}

bool ScalarVisaModule::usesSlot1ScratchSpill() const {
    return m_pShader->ProgramOutput()->getScratchSpaceUsageInSlot1();
}

unsigned ScalarVisaModule::getPointerSize() const {
    return IGC::getPointerSize((llvm::Module &)(*GetModule()));
}

uint64_t ScalarVisaModule::getTypeSizeInBits(Type* Ty) const
{
    IGC_ASSERT(getFunction());
    // TODO: looks like data layout for function pointers is not set
    // correctly. According to the current data layout all pointers are of
    // 64 bits, while vISA/genIsa function pointers are deemed to be 32 bits.
    // Double-check if this is an issue.
    return getFunction()->getParent()->getDataLayout().getTypeSizeInBits(Ty);
}

void ScalarVisaModule::UpdateVisaId()
{
    auto* Kernel = m_pShader->GetEncoder().GetVISAKernel();
    SetVISAId(Kernel->getvIsaInstCount());
}

void ScalarVisaModule::ValidateVisaId()
{
    IGC_ASSERT_MESSAGE(GetCurrentVISAId() == m_pShader->GetEncoder().GetVISAKernel()->getvIsaInstCount(), "Missed emitted pattern!");
}

uint16_t ScalarVisaModule::GetSIMDSize() const
{
    SIMDMode simdMode = m_pShader->m_State.m_dispatchSize;

    return numLanes(simdMode);
}

const Argument* ScalarVisaModule::GetTracedArgument64Ops(const Value* pVal) const
{
    /*  %.privateBuffer111 = bitcast i8* %23 to %opencl.image2d_t addrspace(1)**
    %24 = bitcast %opencl.image2d_t addrspace(1)** %.privateBuffer111 to i64*
    %25 = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p1opencl.image2d_t(%opencl.image2d_t addrspace(1)* %Input)
    %26 = extractvalue { i32, i32 } %25, 0
    %27 = extractvalue { i32, i32 } %25, 1
    %28 = insertelement <2 x i32> undef, i32 %26, i32 0
    %29 = insertelement <2 x i32> %28, i32 %27, i32 1
    %30 = bitcast i64* %24 to <2 x i32>*
    store <2 x i32> %29, <2 x i32>* %30, align 8
    call void @llvm.dbg.declare(metadata %opencl.image2d_t addrspace(1)** %.privateBuffer111, metadata !82, metadata !80), !dbg !83*/

    const Argument* arg = nullptr;
    const Value* pBaseValue = pVal;

    bool found = false;
    // Get to bitcast from privateBuffer111
    // % 24 = bitcast %opencl.image2d_t addrspace(1)** %.privateBuffer111 to i64*
    for (auto i = pBaseValue->user_begin(), e = pBaseValue->user_end(); i != e; ++i)
    {
        const Value* pUser = *i;

        if (isa<const BitCastInst>(pUser))
        {
            pBaseValue = pUser;
            found = true;
            break;
        }
    }

    if (!found) return arg;

    // Get to bitcast from bitcast
    // %30 = bitcast i64* %24 to <2 x i32>*
    found = false;
    for (auto i = pBaseValue->user_begin(), e = pBaseValue->user_end(); i != e; ++i)
    {
        const Value* pUser = *i;

        if (isa<const BitCastInst>(pUser))
        {
            pBaseValue = pUser;
            found = true;
            break;
        }
    }

    if (!found) return arg;

    // Get to store from bitcast
    // store <2 x i32> %29, <2 x i32>* %30, align 8
    found = false;
    for (auto i = pBaseValue->user_begin(), e = pBaseValue->user_end(); i != e; ++i)
    {
        const Value* pUser = *i;

        if (isa<const StoreInst>(pUser))
        {
            pBaseValue = pUser;
            found = true;
            break;
        }
    }

    if (!found) return arg;

    // Get to insertelement from store
    // %29 = insertelement <2 x i32> %28, i32 %27, i32 1
    if (!isa<const StoreInst>(pBaseValue))
    {
        return arg;
    }
    auto storeInst = cast<const StoreInst>(pBaseValue);
    pBaseValue = storeInst->getValueOperand();

    // Get to extractvalue from insertelement
    // %27 = extractvalue { i32, i32 } %25, 1
    if (!isa<const InsertElementInst>(pBaseValue))
    {
        return arg;
    }
    auto insertelement = cast<const InsertElementInst>(pBaseValue);
    pBaseValue = insertelement->getOperand(1);

    // Get to call from extractvalue
    // %25 = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p1opencl.image2d_t(%opencl.image2d_t addrspace(1)* %Input)
    if (!isa<const ExtractValueInst>(pBaseValue))
    {
        return arg;
    }
    auto extractvalue = cast<const ExtractValueInst>(pBaseValue);
    pBaseValue = extractvalue->getOperand(0);

    // Get arg0 of intrinsic
    if (!isa<const CallInst>(pBaseValue))
    {
        return arg;
    }
    auto callinst = cast<const CallInst>(pBaseValue);

    if (!isa<const Argument>(callinst->getArgOperand(0)))
    {
        return arg;
    }
    arg = cast<const Argument>(callinst->getArgOperand(0));

    return arg;
}

const Argument* ScalarVisaModule::GetTracedArgument(const Value* pVal, bool isAddress) const
{
    const Value* pBaseValue = pVal;
    while (true)
    {
        if (isAddress)
        {
            const StoreInst* pStore = nullptr;
            // Alloca used to store image or sampler, assumed to have usages:
            //   1. as many loads as needed.
            //   2. One and only one store.
            for (auto i = pBaseValue->user_begin(), e = pBaseValue->user_end(); i != e; ++i)
            {
                const Value* pUser = *i;
                if (isa<const LoadInst>(pUser))
                {
                    // Found a load, ignore it.
                    continue;
                }
                // Not a load, must be the one and only store.
                if (!isa<const StoreInst>(pUser) || pStore)
                {
                    // Is not traceable to argument, break.
                    pStore = nullptr;
                    break;
                }
                pStore = cast<StoreInst>(pUser);
            }
            // Check that store instruction was found.
            if (!pStore)
            {
                // Is not traceable to argument, break.
                break;
            }
            // Update the baseValue and repeat the check.
            pBaseValue = pStore->getValueOperand();
            isAddress = false;
        }
        if (const Argument * pArg = dyn_cast<const Argument>(pBaseValue))
        {
            // Reached an Argument, return it.
            return pArg;
        }
        else if (const CastInst * pInst = dyn_cast<const CastInst>(pBaseValue))
        {
            // Reached a CastInst (could happen for image).
            // Update the baseValue and repeat the check.
            pBaseValue = pInst->getOperand(0);
        }
        else
        {
            // Is not traceable to argument, break.
            break;
        }
    }
    // If reach this point. Return nullptr.
    return nullptr;
}

VISAVariableLocation
ScalarVisaModule::GetVariableLocation(const llvm::Instruction* pInst) const
{
    Value* pVal = nullptr;
    MDNode* pNode = nullptr;
    bool isDbgDclInst = false;
    if (const DbgDeclareInst * pDbgAddrInst = dyn_cast<DbgDeclareInst>(pInst))
    {
        pVal = pDbgAddrInst->getAddress();
        pNode = pDbgAddrInst->getVariable();
        isDbgDclInst = true;
    }
    else if (const DbgValueInst * pDbgValInst = dyn_cast<DbgValueInst>(pInst))
    {
        pVal = pDbgValInst->getValue();
        pNode = pDbgValInst->getVariable();
    }
    else
    {
        IGC_ASSERT_MESSAGE(0, "Expected debug info instruction");
    }

    if (!pVal || isa<UndefValue>(pVal))
    {
        // No debug info value, return empty location!
        return VISAVariableLocation(this);
    }

    if (const Constant * pConstVal = dyn_cast<Constant>(pVal))
    {
        if (!isa<GlobalVariable>(pVal) && !isa<ConstantExpr>(pVal))
        {
            return VISAVariableLocation(pConstVal, this);
        }
    }

    // Try trace value to an argument
    const Argument* pArgument = GetTracedArgument(pVal, isDbgDclInst);

    if (!pArgument
        && isDbgDclInst)
    {
        // Check for special pattern when Emu64Ops pass is run
        pArgument = GetTracedArgument64Ops(pVal);
    }

    if (pArgument)
    {
        IGC_ASSERT_MESSAGE((pArgument->getParent() == GetEntryFunction() || pArgument->getParent()->hasFnAttribute("referenced-indirectly")), "Argument does not belong to current processed function");

        const Function* curFunc = pArgument->getParent()->hasFnAttribute("referenced-indirectly")
            ? pArgument->getParent() : GetEntryFunction();
        // Check if it is argument of image or sampler
        IGC::IGCMD::MetaDataUtils::FunctionsInfoMap::iterator itr =
            m_pShader->GetMetaDataUtils()->findFunctionsInfoItem(const_cast<Function*>(curFunc));
        CodeGenContext* pCtx = m_pShader->GetContext();
        ModuleMetaData* modMD = pCtx->getModuleMetaData();

        if (itr != m_pShader->GetMetaDataUtils()->end_FunctionsInfo()
            && modMD->FuncMD.find(const_cast<Function*>(curFunc)) != modMD->FuncMD.end())
        {
            unsigned int explicitArgsNum = curFunc->arg_size() - itr->second->size_ImplicitArgInfoList();
        if (pArgument->getArgNo() < explicitArgsNum &&
                modMD->FuncMD[const_cast<Function*>(curFunc)].m_OpenCLArgBaseTypes.size() > pArgument->getArgNo())
            {
                const std::string typeStr = modMD->FuncMD[const_cast<Function*>(curFunc)].m_OpenCLArgBaseTypes[pArgument->getArgNo()];
                KernelArg::ArgType argType = KernelArg::calcArgType(pArgument, typeStr);
                if (argType == KernelArg::ArgType::SAMPLER)
                {
                    // SAMPLER and NOT_TO_ALLOCATE have same enum values so disambiguate these
                    auto pr = KernelArg::getBufferType(pArgument, typeStr);
                    if(!pr.isSampler)
                    {
                        // type is actually NOT_TO_ALLOCATE
                        argType = KernelArg::ArgType::End;
                    }
                }
                FunctionMetaData* funcMD = &modMD->FuncMD[const_cast<Function*>(curFunc)];
                ResourceAllocMD* resAllocMD = &funcMD->resAllocMD;
                IGC_ASSERT_MESSAGE(resAllocMD->argAllocMDList.size() == curFunc->arg_size(), "Invalid ArgAllocMDList");
                ArgAllocMD* argAlloc = &resAllocMD->argAllocMDList[pArgument->getArgNo()];
                unsigned int index = argAlloc->indexType;

                switch (argType)
                {
                default:
                    break;
                case KernelArg::ArgType::SAMPLER:
                    IGC_ASSERT_MESSAGE(index < SAMPLER_REGISTER_NUM, "Bad sampler index");
                    return VISAVariableLocation(SAMPLER_REGISTER_BEGIN + index, this);
                case KernelArg::ArgType::IMAGE_1D:
                case KernelArg::ArgType::IMAGE_1D_BUFFER:
                case KernelArg::ArgType::IMAGE_2D:
                case KernelArg::ArgType::IMAGE_2D_DEPTH:
                case KernelArg::ArgType::IMAGE_2D_MSAA:
                case KernelArg::ArgType::IMAGE_2D_MSAA_DEPTH:
                case KernelArg::ArgType::IMAGE_3D:
                case KernelArg::ArgType::IMAGE_1D_ARRAY:
                case KernelArg::ArgType::IMAGE_2D_ARRAY:
                case KernelArg::ArgType::IMAGE_2D_DEPTH_ARRAY:
                case KernelArg::ArgType::IMAGE_2D_MSAA_ARRAY:
                case KernelArg::ArgType::IMAGE_2D_MSAA_DEPTH_ARRAY:
                    // Found write image
                    switch (argAlloc->type)
                    {
                    case UAVResourceType:
                        // Found write image
                        index = m_pShader->m_pBtiLayout->GetUavIndex(index);
                        IGC_ASSERT_MESSAGE(index < TEXTURE_REGISTER_NUM, "Bad texture index");
                        return VISAVariableLocation(TEXTURE_REGISTER_BEGIN + index, this);
                    case SRVResourceType:
                        // Found read image
                        index = m_pShader->m_pBtiLayout->GetTextureIndex(index);
                        IGC_ASSERT_MESSAGE(index < TEXTURE_REGISTER_NUM, "Bad texture index");
                        return VISAVariableLocation(TEXTURE_REGISTER_BEGIN + index, this);
                    case BindlessUAVResourceType:
                        IGC_ASSERT_MESSAGE(index < TEXTURE_REGISTER_NUM, "Bad texture index");
                        return VISAVariableLocation(TEXTURE_REGISTER_BEGIN + index, this);
                    default:
                        IGC_ASSERT_MESSAGE(0, "Unknown texture resource");
                        return VISAVariableLocation(this);
                    }
                }
            }
        }
    }

    Type* pType = pVal->getType();

    if (isDbgDclInst)
    {
        if (!pType->isPointerTy()) {
            // TODO: Re-enable this assert once -O2 fixes bug where llvm.dbg.declare points
            // to a non-address value.
            //IGC_ASSERT_MESSAGE(0, "DBG declare intrinsic must point to an address");
            return VISAVariableLocation(this);
        }
    }

    bool isGlobalAddrSpace = false;
    if (pType->isPointerTy())
    {
        unsigned int addrSpace = pType->getPointerAddressSpace();
        if (addrSpace == ADDRESS_SPACE_GLOBAL)
        {
            isGlobalAddrSpace = true;
        }
    }

    // SLM global variable
    if (isa<GlobalVariable>(pVal))
    {
        unsigned int offset = m_pShader->GetSLMMappingValue(pVal);
        offset |= VALID_LOCAL_HIGH_BITS;
        return VISAVariableLocation(offset, true, this);
    }

    // At this point we expect only a register
    CVariable* pVar = nullptr;
    auto globalSubCVar = m_pShader->GetGlobalCVar(pVal);

    if (!globalSubCVar) {
        pVar = m_pShader->GetDebugInfoData().getMapping(*pInst->getFunction(), pVal);
        if (!pVar)
        {
            return VISAVariableLocation(this);
        }
    }
    else
        pVar = globalSubCVar;

    IGC_ASSERT_MESSAGE(false == pVar->IsImmediate(), "Do not expect an immediate value at this level");

    std::string varName = cast<DIVariable>(pNode)->getName().str();
    unsigned int vectorNumElements = 0;

    switch (pVar->GetVarType()) {
    case EVARTYPE_GENERAL:
    {
        // We want to attach "Output" attribute to all src variables
        // so that finalizer can extend their liveness to end of
        // the program. This will help debugger examine their
        // values anywhere in the code till they are in scope.
        unsigned int reg = m_pShader->GetDebugInfoData().getVISADclId(pVar, 0);

        if (pType->isVectorTy())
        {
            vectorNumElements = (unsigned)cast<IGCLLVM::FixedVectorType>(pType)->getNumElements();
        }
        else if (!pVar->IsUniform())
        {
            vectorNumElements = 1;
        }
        VISAVariableLocation genReg(reg, true /*isRegister*/, isDbgDclInst, vectorNumElements,
                                    !pVar->IsUniform(), isGlobalAddrSpace, this);
        // SIMD32 locations can't into one register. See VISAVariableLocation::m_locationSecondReg field description for more information
        if (GetSIMDSize() == 32 && pVar->visaGenVariable[1] && !pVar->IsUniform())
        {
            unsigned int reg2 = m_pShader->GetDebugInfoData().getVISADclId(pVar, 1);
            genReg.AddSecondReg(reg2);
        }
        return genReg;
    }
    case EVARTYPE_ADDRESS:
    case EVARTYPE_PREDICATE:
    case EVARTYPE_SURFACE:
    case EVARTYPE_SAMPLER:
        // TODO: Handle case where variable is mapped to flag/address register
        return VISAVariableLocation(this);
    default:
        IGC_ASSERT_MESSAGE(0, "Unhandled VISA register type!");
        break;
    }

    IGC_ASSERT_MESSAGE(0, "Empty variable location");
    return VISAVariableLocation(this);
}

bool ScalarVisaModule::IsCatchAllIntrinsic(const llvm::Instruction* pInst) const
{
    return ((isa<GenIntrinsicInst>(pInst) &&
        cast<GenIntrinsicInst>(pInst)->getIntrinsicID() == GenISAIntrinsic::GenISA_CatchAllDebugLine));
}

// OpenCL keyword constant is used as qualifier to variables whose values remain the
// same throughout the program. clang inlines constants in to LLVM IR and no metadata
// is emitted to LLVM IR for such constants. This function iterates over all globals
// and constants to emit metadata per function.
void insertOCLMissingDebugConstMetadata(CodeGenContext* ctx)
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
                    Utils::UpdateGlobalVarDebugInfo(g, init, &func.getEntryBlock().front(), false);
                }
            }
        }
    }
}

bool ScalarVisaModule::IsIntelSymbolTableVoidProgram() const
{
    return IGC::isIntelSymbolTableVoidProgram(const_cast<llvm::Function*>(GetEntryFunction()));
}

CVariable* ScalarVisaModule::GetSymbol(const llvm::Instruction* pInst, llvm::Value* pValue) const
{
    // CShader's symbols are emptied before compiling a new function.
    // Whereas debug info emission starts after compilation of all functions.
    return m_pShader->GetDebugInfoData().getMapping(*pInst->getFunction(), pValue);
}

int ScalarVisaModule::getDeclarationID(CVariable* pVar, bool isSecondSimd32Instruction) const
{
    int varId = isSecondSimd32Instruction ? 1 : 0;
    if (isSecondSimd32Instruction) {
        if (!((GetSIMDSize() == 32 && pVar->visaGenVariable[1] && !pVar->IsUniform()))) {
            IGC_ASSERT_MESSAGE(0, "Cannot get 2nd variable in SIMD32 (?) mode");
            return -1; // Cannot get 2nd variable in SIMD32 (?) mode
        }
    }
    return m_pShader->GetDebugInfoData().getVISADclId(pVar, varId);
}

} // namespace IGC

