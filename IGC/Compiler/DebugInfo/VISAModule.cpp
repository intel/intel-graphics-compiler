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
#include "Compiler/DebugInfo/VISAModule.hpp"
#include "Compiler/DebugInfo/DebugInfoUtils.hpp"
#include "Compiler/DebugInfo/LexicalScopes.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs.hpp"
#include "Compiler/CodeGenPublicEnums.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Instruction.h"
#include "llvmWrapper/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DebugInfo.h"
#include "common/LLVMWarningsPop.hpp"
#include <vector>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

VISAModule::VISAModule(CShader* pShader)
{
    m_pShader = pShader;
    m_pEntryFunc = pShader->entry;
    m_pModule = m_pEntryFunc->getParent();
    isCloned = false;
    UpdateVisaId();
}

VISAModule::~VISAModule()
{
}

VISAModule::const_iterator VISAModule::begin() const
{
    return m_instList.begin();
}

VISAModule::const_iterator VISAModule::end() const
{
    return m_instList.end();
}

void VISAModule::BeginInstruction(Instruction* pInst)
{
    IGC_ASSERT_MESSAGE(!m_instInfoMap.count(pInst), "Instruction emitted twice!");
    // Assume VISA Id was updated by this point, validate that.
    ValidateVisaId();
    unsigned int nextVISAInstId = m_currentVisaId + 1;
    m_instInfoMap[pInst] = InstructionInfo(INVALID_SIZE, nextVISAInstId);
    m_instList.push_back(pInst);
}

void VISAModule::EndInstruction(Instruction* pInst)
{
    IGC_ASSERT_MESSAGE(m_instList.size() > 0, "Trying to end Instruction other than the last one called with begin!");
    IGC_ASSERT_MESSAGE(m_instList.back() == pInst, "Trying to end Instruction other than the last one called with begin!");
    IGC_ASSERT_MESSAGE(m_instInfoMap.count(pInst), "Trying to end instruction more than once!");
    IGC_ASSERT_MESSAGE(m_instInfoMap[pInst].m_size == INVALID_SIZE, "Trying to end instruction more than once!");

    // Assume VISA Id was updated by this point, validate that.
    ValidateVisaId();

    unsigned currInstOffset = m_instInfoMap[pInst].m_offset;
    unsigned nextInstOffset = m_currentVisaId + 1;
    m_instInfoMap[m_instList.back()].m_size = nextInstOffset - currInstOffset;
}

void VISAModule::BeginEncodingMark()
{
    ValidateVisaId();
}

void VISAModule::EndEncodingMark()
{
    UpdateVisaId();
}

unsigned int VISAModule::GetVisaOffset(const llvm::Instruction* pInst) const
{
    InstInfoMap::const_iterator itr = m_instInfoMap.find(pInst);
    IGC_ASSERT_MESSAGE(itr != m_instInfoMap.end(), "Invalid Instruction");
    return itr->second.m_offset;
}

unsigned int VISAModule::GetVisaSize(const llvm::Instruction* pInst) const
{
    InstInfoMap::const_iterator itr = m_instInfoMap.find(pInst);
    IGC_ASSERT_MESSAGE(itr != m_instInfoMap.end(), "Invalid Instruction");
    IGC_ASSERT_MESSAGE(itr->second.m_size != INVALID_SIZE, "Invalid Size");
    return itr->second.m_size;
}

unsigned VISAModule::GetFunctionNumber(const char* name)
{
    for (auto it : FuncIDMap)
    {
        if (it.first->getName().compare(name) == 0)
        {
            return it.second;
        }
    }

    auto md = GetModule()->getNamedMetadata("igc.device.enqueue");
    if (md)
    {
        for (unsigned int i = 0; i < md->getNumOperands(); i++)
        {
            auto mdOpnd = md->getOperand(i);
            auto first = dyn_cast_or_null<MDString>(mdOpnd->getOperand(0));
            if (first &&
                first->getString().equals(name))
            {
                auto second = dyn_cast_or_null<MDString>(mdOpnd->getOperand(1));
                if (second)
                {
                    return GetFunctionNumber(second->getString().data());
                }
            }
        }
    }

    // If name lookup fails then check mapping passed. This is useful
    // for where llvm::Function has a different name than
    // its DISubprogram metdata node.
    for (auto it = DISPToFunc->begin(), itEnd = DISPToFunc->end(); it != itEnd; it++)
    {
        auto item = (*it);
        auto DISP = item.first;
        if (DISP->getName().compare(name) == 0)
        {
            auto func = item.second;
            auto lookup = FuncIDMap.find(func);
            if (lookup != FuncIDMap.end())
                return lookup->second;
            else
            {
                IGC_ASSERT_MESSAGE(0, "Unexpected function number");
                return 0;
            }
        }
    }

    IGC_ASSERT_MESSAGE(0, "Unexpected function number");
    return 0;
}

unsigned VISAModule::GetFunctionNumber(const llvm::Function* F)
{
    if (FuncIDMap.size() == 0)
    {
        unsigned int id = 0;
        for (auto funcIt = F->getParent()->begin();
            funcIt != F->getParent()->end();
            funcIt++)
        {
            auto func = &(*funcIt);
            FuncIDMap.insert(std::make_pair(func, id++));
        }
    }

    if (isCloned)
    {
        // If function is cloned and has "$dup at the end
        // then replace name without $dup$0.
        auto funcName = F->getName();
        auto match = funcName.rfind("$dup");

        if (match != llvm::StringRef::npos)
        {
            std::string origFuncName = funcName.substr(0, match);

            return GetFunctionNumber(origFuncName.data());
        }
    }

    if (FuncIDMap.find(F) != FuncIDMap.end())
    {
        return FuncIDMap.find(F)->second;
    }

    IGC_ASSERT_MESSAGE(0, "Unexpected function number");

    return 0;
}

bool VISAModule::IsDebugValue(const Instruction* pInst) const
{
    //return isa<DbgDeclareInst>(pInst) || isa<DbgValueInst>(pInst);
    return isa<DbgInfoIntrinsic>(pInst);
}

const MDNode* VISAModule::GetDebugVariable(const Instruction* pInst) const
{
    if (const DbgDeclareInst * pDclInst = dyn_cast<DbgDeclareInst>(pInst))
    {
        return pDclInst->getVariable();
    }
    if (const DbgValueInst * pValInst = dyn_cast<DbgValueInst>(pInst))
    {
        return pValInst->getVariable();
    }
    IGC_ASSERT_MESSAGE(0, "Expected debug info instruction");
    return nullptr;
}

const Argument* VISAModule::GetTracedArgument64Ops(const Value* pVal) const
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

    arg = cast<const Argument>(callinst->getArgOperand(0));

    return arg;
}

const Argument* VISAModule::GetTracedArgument(const Value* pVal, bool isAddress) const
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

VISAVariableLocation VISAModule::GetVariableLocation(const llvm::Instruction* pInst) const
{
    const Value* pVal = nullptr;
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
            IGC_ASSERT_MESSAGE(!isDbgDclInst, "address cannot be immediate!");
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
        IGC_ASSERT_MESSAGE((pArgument->getParent() == m_pEntryFunc || pArgument->getParent()->hasFnAttribute("IndirectlyCalled")), "Argument does not belong to current processed function");

        const Function* curFunc = pArgument->getParent()->hasFnAttribute("IndirectlyCalled")
            ? pArgument->getParent() : m_pEntryFunc;
        // Check if it is argument of image or sampler
        IGC::IGCMD::MetaDataUtils::FunctionsInfoMap::iterator itr =
            m_pShader->GetMetaDataUtils()->findFunctionsInfoItem(const_cast<Function*>(curFunc));
        CodeGenContext* pCtx = m_pShader->GetContext();
        ModuleMetaData* modMD = pCtx->getModuleMetaData();
        // TODO: ProcessBuiltinMetaData pass needs to be run when stack call/subroutine is enabled as duplicated functions for
        // stackcall need updated metadata.
        if (itr != m_pShader->GetMetaDataUtils()->end_FunctionsInfo()
            && modMD->FuncMD.find(const_cast<Function*>(curFunc)) != modMD->FuncMD.end())
        {
            unsigned int explicitArgsNum = IGCLLVM::GetFuncArgSize(curFunc) - itr->second->size_ImplicitArgInfoList();
            if (pArgument->getArgNo() < explicitArgsNum)
            {
                const std::string typeStr = modMD->FuncMD[const_cast<Function*>(curFunc)].m_OpenCLArgBaseTypes[pArgument->getArgNo()];
                KernelArg::ArgType argType = KernelArg::calcArgType(pArgument, typeStr);
                FunctionMetaData* funcMD = &modMD->FuncMD[const_cast<Function*>(curFunc)];
                ResourceAllocMD* resAllocMD = &funcMD->resAllocMD;
                IGC_ASSERT_MESSAGE(resAllocMD->argAllocMDList.size() == IGCLLVM::GetFuncArgSize(curFunc), "Invalid ArgAllocMDList");
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
                    default:
                        IGC_ASSERT_MESSAGE(0, "Unknown texture resource");
                        return VISAVariableLocation(this);
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
            return VISAVariableLocation(this);
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
            return VISAVariableLocation(surfaceReg, offset, false, isDbgDclInst, 0, false, this);
        }
        return VISAVariableLocation(offset, false, isDbgDclInst, 0, false, false, this);
    }

    // At this point we expect only a register
    auto globalSubCVar = m_pShader->GetGlobalCVar(pValue);

    if (!globalSubCVar && !m_pShader->IsValueUsed(pValue)) {
        return VISAVariableLocation(this);
    }

    CVariable* pVar = nullptr;
    if (globalSubCVar)
        pVar = globalSubCVar;
    else
        pVar = m_pShader->GetSymbol(pValue);
    IGC_ASSERT_MESSAGE(false == pVar->IsImmediate(), "Do not expect an immediate value at this level");

    std::string varName = cast<DIVariable>(pNode)->getName();
    unsigned int reg = 0;
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
            vectorNumElements = pType->getVectorNumElements();
        }
        else if (!pVar->IsUniform())
        {
            vectorNumElements = 1;
        }

        if (isInSurface)
        {
            return VISAVariableLocation(surfaceReg, GENERAL_REGISTER_BEGIN + reg, true, isDbgDclInst, vectorNumElements, !pVar->IsUniform(), this);
        }
        return VISAVariableLocation(GENERAL_REGISTER_BEGIN + reg, true, isDbgDclInst, vectorNumElements, !pVar->IsUniform(), isGlobalAddrSpace, this);
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
    return VISAVariableLocation(this);
}

void VISAModule::GetConstantData(const Constant* pConstVal, DataVector& rawData) const
{
    if (dyn_cast<ConstantPointerNull>(pConstVal))
    {
        DataLayout DL(GetDataLayout());
        rawData.insert(rawData.end(), DL.getPointerSize(), 0);
    }
    else if (const ConstantDataSequential * cds = dyn_cast<ConstantDataSequential>(pConstVal))
    {
        for (unsigned i = 0; i < cds->getNumElements(); i++) {
            GetConstantData(cds->getElementAsConstant(i), rawData);
        }
    }
    else if (const ConstantAggregateZero * cag = dyn_cast<ConstantAggregateZero>(pConstVal))
    {
        // Zero aggregates are filled with, well, zeroes.
        DataLayout DL(GetDataLayout());
        const unsigned int zeroSize = (unsigned int)(DL.getTypeAllocSize(cag->getType()));
        rawData.insert(rawData.end(), zeroSize, 0);
    }
    // If this is an sequential type which is not a CDS or zero, have to collect the values
    // element by element. Note that this is not exclusive with the two cases above, so the
    // order of ifs is meaningful.
    else if (dyn_cast<CompositeType>(pConstVal->getType()))
    {
        const int numElts = pConstVal->getNumOperands();
        for (int i = 0; i < numElts; ++i)
        {
            Constant* C = pConstVal->getAggregateElement(i);
            IGC_ASSERT_MESSAGE(C, "getAggregateElement returned null, unsupported constant");
            // Since the type may not be primitive, extra alignment is required.
            GetConstantData(C, rawData);
        }
    }
    // And, finally, we have to handle base types - ints and floats.
    else
    {
        APInt intVal(32, 0, false);
        if (const ConstantInt * ci = dyn_cast<ConstantInt>(pConstVal))
        {
            intVal = ci->getValue();
        }
        else if (const ConstantFP * cfp = dyn_cast<ConstantFP>(pConstVal))
        {
            intVal = cfp->getValueAPF().bitcastToAPInt();
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "Unsupported constant type");
        }

        const int bitWidth = intVal.getBitWidth();
        IGC_ASSERT_MESSAGE((0 < bitWidth), "Unsupported bitwidth");
        IGC_ASSERT_MESSAGE((bitWidth % 8 == 0), "Unsupported bitwidth");
        IGC_ASSERT_MESSAGE((bitWidth <= 64), "Unsupported bitwidth");

        const uint64_t* val = intVal.getRawData();
        rawData.insert(rawData.end(), (char*)val, ((char*)val) + (bitWidth / 8));
    }
}

const Module* VISAModule::GetModule() const
{
    return m_pModule;
}

const Function* VISAModule::GetEntryFunction() const
{
    return m_pEntryFunc;
}

const LLVMContext& VISAModule::GetContext() const
{
    return m_pModule->getContext();
}

const std::string VISAModule::GetDataLayout() const
{
    return m_pModule->getDataLayout().getStringRepresentation();
}

const std::string& VISAModule::GetTargetTriple() const
{
    return m_triple;
}

void VISAModule::UpdateVisaId()
{
    auto* Kernel = m_pShader->GetEncoder().GetVISAKernel();
    m_currentVisaId = Kernel->getvIsaInstCount();
}

void VISAModule::ValidateVisaId()
{
    IGC_ASSERT_MESSAGE(m_currentVisaId == m_pShader->GetEncoder().GetVISAKernel()->getvIsaInstCount(), "Missed emitted pattern!");
}

uint16_t VISAModule::GetSIMDSize() const
{
    SIMDMode simdMode = m_pShader->m_dispatchSize;

    return numLanes(simdMode);
}

void VISAModule::Reset()
{
    m_instList.clear();
    isCloned = false;
    UpdateVisaId();
}

void VISAModule::buildDirectElfMaps()
{
    auto co = getCompileUnit();
    VISAIndexToInst.clear();
    VISAIndexToSize.clear();
    for (VISAModule::const_iterator II = begin(), IE = end(); II != IE; ++II)
    {
        const Instruction* pInst = *II;

        InstInfoMap::const_iterator itr = m_instInfoMap.find(pInst);
        if (itr == m_instInfoMap.end())
            continue;

        unsigned int currOffset = itr->second.m_offset;
        VISAIndexToInst.insert(std::make_pair(currOffset, pInst));
        unsigned int currSize = itr->second.m_size;
        for (auto index = currOffset; index != (currOffset + currSize); index++)
            VISAIndexToSize.insert(std::make_pair(index,
                std::make_pair(currOffset, currSize)));
    }

    GenISAToVISAIndex.clear();
    for (auto i = 0;
        i != co->CISAIndexMap.size();
        i++)
    {
        auto& item = co->CISAIndexMap[i];
        GenISAToVISAIndex.push_back(std::make_pair(item.second, item.first));
    }

    // Compute all Gen ISA offsets corresponding to each VISA index
    VISAIndexToAllGenISAOff.clear();
    for (auto& item : co->CISAIndexMap)
    {
        auto VISAIndex = item.first;
        auto GenISAOffset = item.second;
        auto it = VISAIndexToAllGenISAOff.find(VISAIndex);
        if (it != VISAIndexToAllGenISAOff.end())
            it->second.push_back(GenISAOffset);
        else
        {
            std::vector<unsigned int> vec;
            vec.push_back(GenISAOffset);
            VISAIndexToAllGenISAOff[VISAIndex] = vec;
        }
    }

    GenISAInstSizeBytes.clear();
    for (auto i = 0; i != co->CISAIndexMap.size() - 1; i++)
    {
        unsigned int size = GenISAToVISAIndex[i + 1].first - GenISAToVISAIndex[i].first;
        GenISAInstSizeBytes.insert(std::make_pair(GenISAToVISAIndex[i].first, size));
    }
    GenISAInstSizeBytes.insert(std::make_pair(GenISAToVISAIndex[GenISAToVISAIndex.size() - 1].first, 16));
}

std::vector<std::pair<unsigned int, unsigned int>> VISAModule::getGenISARange(const InsnRange& Range)
{
    // Given a range, return vector of start-end range for corresponding Gen ISA instructions
    auto start = Range.first;
    auto end = Range.second;

    // Range consists of a sequence of LLVM IR instructions. This function needs to return
    // a range of corresponding Gen ISA instructions. Instruction scheduling in Gen ISA
    // means several independent sub-ranges will be present.
    std::vector<std::pair<unsigned int, unsigned int>> GenISARange;
    bool endNextInst = false;

    auto getNextInst = [](const llvm::Instruction* start)
    {
        // Return consecutive instruction in llvm IR.
        // Iterate to next BB if required.
        if (start->getNextNode())
            return start->getNextNode();
        else if (start->getParent()->getNextNode())
            return &(start->getParent()->getNextNode()->front());
        return (const llvm::Instruction*)nullptr;
    };

    while (1)
    {
        if (!start || !end || endNextInst)
            break;

        if (start == end)
            endNextInst = true;

        // Get VISA index/size for "start" LLVM IR inst
        InstInfoMap::const_iterator itr = m_instInfoMap.find(start);
        if (itr == m_instInfoMap.end())
        {
            start = getNextInst(start);
            continue;
        }
        auto startVISAOffset = itr->second.m_offset;
        // VISASize indicated # of VISA insts emitted for this
        // LLVM IR inst
        auto VISASize = GetVisaSize(start);

        for (unsigned int i = 0; i != VISASize; i++)
        {
            auto VISAIndex = startVISAOffset + i;
            auto it = VISAIndexToAllGenISAOff.find(VISAIndex);
            if (it != VISAIndexToAllGenISAOff.end())
            {
                int lastEnd = -1;
                for (auto& genInst : it->second)
                {
                    unsigned int sizeGenInst = GenISAInstSizeBytes[genInst];

                    if (GenISARange.size() > 0)
                        lastEnd = GenISARange.back().second;

                    if (lastEnd == genInst)
                    {
                        GenISARange.back().second += sizeGenInst;
                    }
                    else
                    {
                        GenISARange.push_back(std::make_pair(genInst, genInst + sizeGenInst));
                    }
                    lastEnd = GenISARange.back().second;
                }
            }
        }

        start = getNextInst(start);
    }

    class Comp
    {
    public:
        bool operator()(const std::pair<unsigned int, unsigned int>& a,
            const std::pair<unsigned int, unsigned int>& b)
        {
            return a.first < b.first;
        }
    } Comp;

    if (GenISARange.size() == 0)
        return GenISARange;

    std::sort(GenISARange.begin(), GenISARange.end(), Comp);

    for (unsigned int i = 0; i != GenISARange.size() - 1; i++)
    {
        if (GenISARange[i].first == (unsigned int)-1 && GenISARange[i].second == (unsigned int)-1)
            continue;

        for (unsigned int j = i + 1; j != GenISARange.size(); j++)
        {
            if (GenISARange[j].first == (unsigned int)-1 && GenISARange[j].second == (unsigned int)-1)
                continue;

            if (GenISARange[j].first == GenISARange[i].second)
            {
                GenISARange[i].second = GenISARange[j].second;
                GenISARange[j].first = (unsigned int)-1;
                GenISARange[j].second = (unsigned int)-1;
            }
        }
    }


    for (auto it = GenISARange.begin(); it != GenISARange.end();)
    {
        if ((*it).first == (unsigned int)-1 && (*it).second == (unsigned int)-1)
        {
            it = GenISARange.erase(it);
            continue;
        }
        it++;
    }

    return GenISARange;
}

bool VISAVariableLocation::IsSampler() const
{
    if (!HasSurface())
        return false;

    auto surface = GetSurface();
    if (surface >= VISAModule::SAMPLER_REGISTER_BEGIN &&
        surface < VISAModule::SAMPLER_REGISTER_BEGIN + VISAModule::SAMPLER_REGISTER_NUM)
        return true;
    return false;
}

bool VISAVariableLocation::IsTexture() const
{
    if (!HasSurface())
        return false;

    auto surface = GetSurface();
    if (surface >= VISAModule::TEXTURE_REGISTER_BEGIN &&
        surface < VISAModule::TEXTURE_REGISTER_BEGIN + VISAModule::TEXTURE_REGISTER_NUM)
        return true;
    return false;
}

bool VISAVariableLocation::IsSLM() const
{
    if (!HasSurface())
        return false;

    auto surface = GetSurface();
    if (surface == VISAModule::LOCAL_SURFACE_BTI + VISAModule::TEXTURE_REGISTER_BEGIN)
        return true;
    return false;
}