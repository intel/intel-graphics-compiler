/*========================== begin_copyright_notice ============================

Copyright (c) 2010-2021 Intel Corporation

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

#include "Compiler/DebugInfo/ScalarVISAModule.h"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/DebugInfo.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/debug/Debug.hpp"

#include "DebugInfo/DebugInfoUtils.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
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

ScalarVisaModule::ScalarVisaModule(CShader* TheShader, Function *TheFunction)
  : m_pShader(TheShader), VISAModule(TheFunction) {
  UpdateVisaId();
}

std::unique_ptr<IGC::VISAModule> ScalarVisaModule::BuildNew(CShader* S, llvm::Function *F)
{
    auto n = new ScalarVisaModule(S, F);

    if (n->m_pShader->GetContext()->m_DriverInfo.SupportElfFormat() ||
        isLineTableOnly(S) ||
        IGC_GET_FLAG_VALUE(EnableOneStepElf))
    {
        n->isDirectElfInput = true;
    }

    return std::unique_ptr<IGC::VISAModule>(n);
}

int ScalarVisaModule::getPTOReg() const {
    IGC_ASSERT_MESSAGE(hasPTO(), "PTO instruction required");
    Value* pValPTO = dyn_cast_or_null<Value>(m_perThreadOffset);
    IGC_ASSERT_MESSAGE(pValPTO, "pValPTO error");
    // At this point we expect only a register
    CVariable* pVarPTO = GetSymbol(m_perThreadOffset, pValPTO);

    IGC_ASSERT_MESSAGE(pVarPTO, "Per Thread Offset variable does not exist");
    IGC_ASSERT_MESSAGE(pVarPTO->GetVarType() == EVARTYPE_GENERAL, "Unexpected VISA register type!");

    int regPTO = getDeclarationID(pVarPTO, false);
    return regPTO;
}
int ScalarVisaModule::getFPReg() const {
    CVariable *framePtr = getFramePtr();
    // TBD: IGC_ASSERT_MESSAGE(framePtr, "Frame Pointer does not exist");
    int regFP = getDeclarationID(framePtr, false);
    return regFP;
}

llvm::StringRef ScalarVisaModule::GetVISAFuncName(llvm::StringRef OldName) const
{
    // when igc.device.enqueue metadata is used, function name
    // doesnt match between llvm::Function and VISA. this
    // lambda checks whether device enqueue is used and if so,
    // it returns function name that VISA uses. this is required
    // to lookup symbol table of the function.
    //
    // when device enqueue is not used, llvm's function name
    // matches that used by VISA.
    auto& Module = *getFunction()->getParent();

    // check if llvm function name is different than VISA function name
    // due to igc.device.enqueue named MD
    auto EnqueueNamedMD = Module.getNamedMetadata("igc.device.enqueue");
    if (EnqueueNamedMD)
    {
        // !igc.device.enqueue = !{!307}
        // !307 = !{!"__ParentKernel_block_invoke_kernel", !"ParentKernel_dispatch_0"}
        auto NumOpnds = EnqueueNamedMD->getNumOperands();
        for (unsigned int I = 0; I != NumOpnds; ++I)
        {
            auto Pair = EnqueueNamedMD->getOperand(I);
            auto& First = Pair->getOperand(0);
            auto& Second = Pair->getOperand(1);
            if (isa<MDString>(First) && isa<MDString>(Second))
            {
                auto Str = cast<MDString>(Second)->getString();
                if (Str.equals(OldName))
                {
                    return cast<MDString>(First)->getString();
                }
            }
        }
    }

    return OldName;
}
uint64_t ScalarVisaModule::getFPOffset() const {
    return EmitPass::getFPOffset();
}
unsigned ScalarVisaModule::getPointerSize() const {
    return IGC::getPointerSize((llvm::Module &)(*GetModule()));
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
    SIMDMode simdMode = m_pShader->m_dispatchSize;

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
std::vector<VISAVariableLocation>
ScalarVisaModule::GetVariableLocation(const llvm::Instruction* pInst) const
{
    const Value* pVal = nullptr;
    MDNode* pNode = nullptr;
    bool isDbgDclInst = false;
    std::vector<VISAVariableLocation> ret;
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
        ret.push_back(VISAVariableLocation(this));
        return ret;
    }

    if (const Constant * pConstVal = dyn_cast<Constant>(pVal))
    {
        if (!isa<GlobalVariable>(pVal) && !isa<ConstantExpr>(pVal))
        {
            IGC_ASSERT_MESSAGE(!isDbgDclInst, "address cannot be immediate!");
            ret.push_back(VISAVariableLocation(pConstVal, this));
            return ret;
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
        IGC_ASSERT_MESSAGE((pArgument->getParent() == GetEntryFunction() || pArgument->getParent()->hasFnAttribute("IndirectlyCalled")), "Argument does not belong to current processed function");

        const Function* curFunc = pArgument->getParent()->hasFnAttribute("IndirectlyCalled")
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
                    ret.push_back(VISAVariableLocation(SAMPLER_REGISTER_BEGIN + index, this));
                    return ret;
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
                        ret.push_back(VISAVariableLocation(TEXTURE_REGISTER_BEGIN + index, this));
                        return ret;
                    case SRVResourceType:
                        // Found read image
                        index = m_pShader->m_pBtiLayout->GetTextureIndex(index);
                        IGC_ASSERT_MESSAGE(index < TEXTURE_REGISTER_NUM, "Bad texture index");
                        ret.push_back(VISAVariableLocation(TEXTURE_REGISTER_BEGIN + index, this));
                        return ret;
                    default:
                        IGC_ASSERT_MESSAGE(0, "Unknown texture resource");
                        ret.push_back(VISAVariableLocation(this));
                        return ret;
                    }
                }
            }
        }
    }

    Value* pValue = const_cast<Value*>(pVal);

    Type* pType = pValue->getType();

    if (isDbgDclInst)
    {
        if (!pType->isPointerTy()) {
            IGC_ASSERT_MESSAGE(0, "DBG declare intrinsic must point to an address");
            ret.push_back(VISAVariableLocation(this));
            return ret;
        }
        pType = pType->getPointerElementType();
    }

    bool isInSurface = false;
    bool isGlobalAddrSpace = false;
    unsigned int surfaceReg = 0;
    if (pType->isPointerTy())
    {
        unsigned int addrSpace = pType->getPointerAddressSpace();
        if (addrSpace == ADDRESS_SPACE_LOCAL)
        {
            isInSurface = true;
            surfaceReg = TEXTURE_REGISTER_BEGIN + LOCAL_SURFACE_BTI;
        }
        if (addrSpace == ADDRESS_SPACE_GLOBAL)
        {
            isGlobalAddrSpace = true;
        }
    }
    else if (pVal->getType()->isPointerTy())
    {
        unsigned int addrSpace = pVal->getType()->getPointerAddressSpace();
        if (addrSpace == ADDRESS_SPACE_LOCAL)
        {
            isInSurface = true;
            surfaceReg = TEXTURE_REGISTER_BEGIN + LOCAL_SURFACE_BTI;
        }
    }

    if (isa<GlobalVariable>(pValue))
    {
        unsigned int offset = m_pShader->GetGlobalMappingValue(pValue);
        if (isInSurface)
        {
            ret.push_back(VISAVariableLocation(surfaceReg, offset, false, isDbgDclInst, 0, false, this));
            return ret;
        }
        ret.push_back(VISAVariableLocation(offset, false, isDbgDclInst, 0, false, false, this));
        return ret;
    }

    // At this point we expect only a register
    CVariable* pVar = nullptr;
    auto globalSubCVar = m_pShader->GetGlobalCVar(pValue);

    if (!globalSubCVar) {
        pVar = m_pShader->GetDebugInfoData()->getMapping(*pInst->getFunction(), pValue);
        if (!pVar)
        {
            ret.push_back(VISAVariableLocation(this));
            return ret;
        }
    }
    else
        pVar = globalSubCVar;

    IGC_ASSERT_MESSAGE(false == pVar->IsImmediate(), "Do not expect an immediate value at this level");

    std::string varName = cast<DIVariable>(pNode)->getName().str();
    unsigned int reg = 0, reg2 = 0;
    unsigned int vectorNumElements = 0;

    switch (pVar->GetVarType()) {
    case EVARTYPE_GENERAL:
        // We want to attach "Output" attribute to all src variables
        // so that finalizer can extend their liveness to end of
        // the program. This will help debugger examine their
        // values anywhere in the code till they are in scope.
        reg = m_pShader->GetEncoder().GetVISAKernel()->getDeclarationID(pVar->visaGenVariable[0]);
        IGC_ASSERT_MESSAGE(reg < GENERAL_REGISTER_NUM, "Bad VISA general register");

        if (pType->isVectorTy())
        {
            vectorNumElements = (unsigned)cast<IGCLLVM::FixedVectorType>(pType)->getNumElements();
        }
        else if (!pVar->IsUniform())
        {
            vectorNumElements = 1;
        }

        if (isInSurface)
        {
            ret.push_back(VISAVariableLocation(surfaceReg, GENERAL_REGISTER_BEGIN + reg, true, isDbgDclInst, vectorNumElements, !pVar->IsUniform(), this));
            return ret;
        }
        ret.push_back(VISAVariableLocation(GENERAL_REGISTER_BEGIN + reg, true, isDbgDclInst, vectorNumElements, !pVar->IsUniform(), isGlobalAddrSpace, this));
        if (GetSIMDSize() == 32 && pVar->visaGenVariable[1] && !pVar->IsUniform())
        {
            reg2 = m_pShader->GetEncoder().GetVISAKernel()->getDeclarationID(pVar->visaGenVariable[1]);
            ret.push_back(VISAVariableLocation(GENERAL_REGISTER_BEGIN + reg2, true, isDbgDclInst, vectorNumElements, !pVar->IsUniform(), isGlobalAddrSpace, this));
        }
        return ret;
    case EVARTYPE_ADDRESS:
    case EVARTYPE_PREDICATE:
    case EVARTYPE_SURFACE:
    case EVARTYPE_SAMPLER:
        IGC_ASSERT_MESSAGE(0, "Unexpected VISA register type!");
        break;
    default:
        IGC_ASSERT_MESSAGE(0, "Unhandled VISA register type!");
        break;
    }

    IGC_ASSERT_MESSAGE(0, "Empty variable location");
    ret.push_back(VISAVariableLocation(this));
    return ret;
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
                    DebugInfoUtils::UpdateGlobalVarDebugInfo(g, init, &func.getEntryBlock().getInstList().front(), false);
                }
            }
        }
    }
}

bool ScalarVisaModule::IsIntelSymbolTableVoidProgram() const
{
    return IGC::isIntelSymbolTableVoidProgram(const_cast<llvm::Function*>(GetEntryFunction()));
}

} // namespace IGC

