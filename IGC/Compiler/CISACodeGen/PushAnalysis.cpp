/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"
#include "common/LLVMUtils.h"
#include "LLVMWarningsPush.hpp"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Function.h"
#include "LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/GenCodeGenModule.h"
#include "Compiler/CISACodeGen/PushAnalysis.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/CISACodeGen.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "common/igc_regkeys.hpp"
#include "Compiler/IGCPassSupport.h"
#include "LLVM3DBuilder/MetadataBuilder.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/debug/Debug.hpp"
#include "Probe/Assertion.h"

/***********************************************************************************
This File contains the logic to decide for each inputs and constant if the data
should be pushed or pull.
In case the data is pushed we remove the load instruction replace the value by
a function argument so that the liveness calculated is correct

************************************************************************************/
using namespace llvm;
using namespace IGC::IGCMD;

namespace IGC
{
#define PASS_FLAG "igc-push-value-info"
#define PASS_DESCRIPTION "hold push value information"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true

    //undef macros to avoid redefinition warnings
#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_ANALYSIS

#define PASS_FLAG "igc-push-analysis"
#define PASS_DESCRIPTION "promotes the values to be arguments"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
    IGC_INITIALIZE_PASS_BEGIN(PushAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(PullConstantHeuristics)
        IGC_INITIALIZE_PASS_END(PushAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

        PushAnalysis::PushAnalysis()
        : ModulePass(ID)
        , m_PDT(nullptr)
        , m_cbToLoad((uint)-1)
        , m_maxStatelessOffset(0)
    {
        initializePushAnalysisPass(*PassRegistry::getPassRegistry());
    }

    const uint32_t PushAnalysis::MaxConstantBufferIndexSize = 256;

    template < typename T > std::string to_string(const T& n)
    {
        std::ostringstream stm;
        stm << n;
        return stm.str();
    }

    Value* PushAnalysis::addArgumentAndMetadata(llvm::Type* pType, std::string argName, IGC::WIAnalysis::WIDependancy dependency)
    {
        auto pArgInfo = m_pFuncUpgrade.AddArgument(argName, pType);
        ModuleMetaData* modMD = nullptr;
        modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
        IGC::ArgDependencyInfoMD argDepInfoMD;
        argDepInfoMD.argDependency = dependency;
        modMD->pushInfo.pushAnalysisWIInfos.push_back(argDepInfoMD);

        //Add it to arglist and increment Argument Index
        m_argList.push_back(pArgInfo);
        m_argIndex++;

        m_funcTypeChanged = true;

        return pArgInfo;
    }

    bool PushAnalysis::IsPushableBindlessLoad(
        Instruction* inst,
        int& grfOffset,
        unsigned int& cbIdx,
        unsigned int& offset)
    {
        LdRawIntrinsic* ldRaw = dyn_cast<LdRawIntrinsic>(inst);
        if (!ldRaw ||
            BINDLESS_CONSTANT_BUFFER != DecodeBufferType(
                ldRaw->getResourceValue()->getType()->getPointerAddressSpace()))
        {
            return false;
        }

        std::function<bool(Value*)> isPushable;
        isPushable = [this, &grfOffset, &cbIdx, &isPushable](Value* v)
        {
            // Matches the following patterns:
            // Pattern 1:
            //   %6 = add i32 %runtime_value_0, 4096
            //   %7 = inttoptr i32 %6 to i8 addrspace(1507328)*
            //   %8 = call <8 x float> @llvm.genx.GenISA.ldrawvector.indexed.v8f32.p1507328i8(i8 addrspace(1507328)* %7, i32 0, i32 4, i1 false)
            // Pattern 2:
            //   %8 = call <8 x float> @llvm.genx.GenISA.ldrawvector.indexed.v8f32.p1507328i8(i8 addrspace(1507328)* %runtime_value_0, i32 0, i32 4, i1 false)
            if (m_bindlessPushArgs.find(v) != m_bindlessPushArgs.end())
            {
                grfOffset = int_cast<int>(m_bindlessPushArgs.find(v)->second);
                return true;
            }
            else if (IntToPtrInst* ptrToInt = dyn_cast<IntToPtrInst>(v))
            {
                return isPushable(ptrToInt->getOperand(0));
            }
            else if (Instruction* instr = dyn_cast<Instruction>(v))
            {
                if (instr->getOpcode() == Instruction::Add &&
                    isa<ConstantInt>(instr->getOperand(1)) &&
                    isPushable(instr->getOperand(0)))
                {
                    ConstantInt* src1 = cast<ConstantInt>(instr->getOperand(1));
                    cbIdx += int_cast<unsigned int>(src1->getZExtValue()) >> m_context->platform.getBSOLocInExtDescriptor();
                    return true;
                }
            }
            return false;
        };

        if (isPushable(ldRaw->getResourceValue()))
        {
            Value* offsetVal = ldRaw->getOffsetValue();
            if (ConstantInt * offsetConstVal = dyn_cast<ConstantInt>(offsetVal))
            {
                offset = int_cast<unsigned int>(offsetConstVal->getZExtValue());
                return true;
            }

            if (m_context->m_DriverInfo.SupportsDynamicUniformBuffers() &&
                IGC_IS_FLAG_DISABLED(DisableSimplePushWithDynamicUniformBuffers) &&
                !m_context->getModuleMetaData()->pushInfo.dynamicBufferInfo.forceDisabled)
            {
                unsigned int relativeOffsetInBytes = 0; // offset in bytes starting from dynamic buffer offset
                if (GetConstantOffsetForDynamicUniformBuffer(offsetVal, relativeOffsetInBytes))
                {
                    offset = relativeOffsetInBytes;
                    return true;
                }
            }
        }
        return false;
    }

    bool PushAnalysis::IsStatelessCBLoad(
        llvm::Instruction* inst,
        int& pushableAddressGrfOffset,
        int& pushableOffsetGrfOffset,
        unsigned int& pushableOffset)
    {
        if (!llvm::isa<llvm::LoadInst>(inst))
            return false;

        // %15 = load <3 x float> addrspace(2)* %14, align 16
        llvm::LoadInst* pLoad = llvm::cast<llvm::LoadInst>(inst);
        uint address_space = pLoad->getPointerAddressSpace();
        if (address_space != ADDRESS_SPACE_CONSTANT)
            return false;

        Value* pAddress = pLoad->getOperand(pLoad->getPointerOperandIndex());
        // skip casts
        if (isa<IntToPtrInst>(pAddress) || isa<PtrToIntInst>(pAddress) || isa<BitCastInst>(pAddress))
        {
            pAddress = cast<Instruction>(pAddress)->getOperand(0);
        }

        int64_t offset = 0;
        SmallVector<Value*, 4> potentialPushableAddresses;
        std::function<void(Value*)> GetPotentialPushableAddresses;
        GetPotentialPushableAddresses = [&potentialPushableAddresses, &offset, &GetPotentialPushableAddresses](
            Value* pAddress)
        {
            BinaryOperator* pAdd = dyn_cast<BinaryOperator>(pAddress);
            if (pAdd && pAdd->getOpcode() == llvm::Instruction::Add)
            {
                GetPotentialPushableAddresses(pAdd->getOperand(0));
                GetPotentialPushableAddresses(pAdd->getOperand(1));
            }
            else if (isa<ZExtInst>(pAddress))
            {
                GetPotentialPushableAddresses(
                    cast<ZExtInst>(pAddress)->getOperand(0));
            }
            else if (isa<ConstantInt>(pAddress))
            {
                ConstantInt* pConst = cast<ConstantInt>(pAddress);
                offset += pConst->getZExtValue();
            }
            else
            {
                potentialPushableAddresses.push_back(pAddress);
            }
        };

        if (GenIntrinsicInst * genIntr = dyn_cast<GenIntrinsicInst>(pAddress))
        {
            /*
            Examples of patterns matched on platforms without 64bit type support:
            1. Pushable 64bit address + immediate offset case:
            %29 = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p2v3f32(<3 x float> addrspace(2)* %runtime_value_6)
            %30 = extractvalue { i32, i32 } %29, 1
            %31 = extractvalue { i32, i32 } %29, 0
            %32 = call <4 x float> addrspace(2)* @llvm.genx.GenISA.pair.to.ptr.p2v4f32(i32 %31, i32 %30)
                      or
            %33 = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p2v4f32(<4 x float> addrspace(2)* %runtime_value_4)
            %34 = extractvalue { i32, i32 } %33, 0
            %35 = extractvalue { i32, i32 } %33, 1
            %36 = call { i32, i32 } @llvm.genx.GenISA.add.pair(i32 %34, i32 %35, i32 368, i32 0)
            %37 = extractvalue { i32, i32 } %36, 0
            %38 = extractvalue { i32, i32 } %36, 1
            %39 = call <2 x i32> addrspace(2)* @llvm.genx.GenISA.pair.to.ptr.p2v2i32(i32 %37, i32 %38)

            2. Pushable 64bit address + pushable 32bit offset + immediate offset pattern:
            %7 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 2)
            %8 = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 0)
            %9 = bitcast i64 %8 to <2 x i32>
            %10 = extractelement <2 x i32> %9, i32 0
            %11 = extractelement <2 x i32> %9, i32 1
            %12 = call { i32, i32 } @llvm.genx.GenISA.add.pair(i32 %10, i32 %11, i32 %1, i32 0)
            %13 = extractvalue { i32, i32 } %12, 0
            %14 = extractvalue { i32, i32 } %12, 1
            %15 = call i8 addrspace(1441792)* addrspace(2)* @llvm.genx.GenISA.pair.to.ptr.p2p1441792i8(i32 %13, i32 %14)
                      or
            %7 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 2)
            %8 = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 0)
            %9 = bitcast i64 %8 to <2 x i32>
            %10 = extractelement <2 x i32> %9, i32 0
            %11 = extractelement <2 x i32> %9, i32 1
            %12 = call { i32, i32 } @llvm.genx.GenISA.add.pair(i32 %10, i32 %11, i32 %1, i32 0)
            %13 = extractvalue { i32, i32 } %12, 0
            %14 = extractvalue { i32, i32 } %12, 1
            %15 = call { i32, i32 } @llvm.genx.GenISA.add.pair(i32 %13, i32 %14, i32 16, i32 0)
            %16 = extractvalue { i32, i32 } %15, 0
            %17 = extractvalue { i32, i32 } %15, 1
            %18 = call i8 addrspace(1441792)* addrspace(2)* @llvm.genx.GenISA.pair.to.ptr.p2p1441792i8(i32 %16, i32 %17)
            */
            if (genIntr->getIntrinsicID() == GenISAIntrinsic::GenISA_pair_to_ptr)
            {
                ExtractValueInst* Lo = dyn_cast<ExtractValueInst>(genIntr->getOperand(0));
                ExtractValueInst* Hi = dyn_cast<ExtractValueInst>(genIntr->getOperand(1));
                if (Lo && Hi && Lo->getOperand(0) == Hi->getOperand(0))
                {
                    if (GenIntrinsicInst * genIntr2 = dyn_cast<GenIntrinsicInst>(Lo->getOperand(0)))
                    {
                        if (genIntr2->getIntrinsicID() == GenISAIntrinsic::GenISA_ptr_to_pair)
                        {
                            offset = 0;
                            pAddress = genIntr2->getOperand(0);
                        }

                        while (genIntr2 &&
                            genIntr2->getIntrinsicID() == GenISAIntrinsic::GenISA_add_pair)
                        {
                            pAddress = nullptr;
                            ExtractValueInst* Lo2 = dyn_cast<ExtractValueInst>(genIntr2->getOperand(0));
                            ExtractValueInst* Hi2 = dyn_cast<ExtractValueInst>(genIntr2->getOperand(1));
                            if (Lo2 && Hi2 && Lo2->getOperand(0) == Hi2->getOperand(0))
                            {
                                if (GenIntrinsicInst* genIntr3 = dyn_cast<GenIntrinsicInst>(Lo2->getOperand(0)))
                                {
                                    if (genIntr3->getIntrinsicID() == GenISAIntrinsic::GenISA_ptr_to_pair)
                                    {
                                        // pattern no. 1
                                        pAddress = genIntr3->getOperand(0);
                                    }
                                    else if (genIntr3->getIntrinsicID() == GenISAIntrinsic::GenISA_add_pair)
                                    {
                                        // multiple add_pair calls, to be
                                        // processed in the next iteration
                                        pAddress = genIntr3;
                                    }
                                }
                            }
                            else
                            {
                                ExtractElementInst* Lo3 = dyn_cast<ExtractElementInst>(genIntr2->getOperand(0));
                                ExtractElementInst* Hi3 = dyn_cast<ExtractElementInst>(genIntr2->getOperand(1));
                                if (Lo3 && Hi3 && Lo3->getOperand(0) == Hi3->getOperand(0))
                                {
                                    // pattern no. 2
                                    pAddress = Lo3->getOperand(0);
                                }
                            }
                            ConstantInt* offsetLo2 = dyn_cast<ConstantInt>(genIntr2->getOperand(2));
                            ConstantInt* offsetHi2 = dyn_cast<ConstantInt>(genIntr2->getOperand(3));
                            if (pAddress && offsetHi2)
                            {
                                // Note that the offset in the add_pair
                                // instruction may be a negative value (MemOpt
                                // creates negative offsets)
                                if (offsetLo2  &&
                                    (offsetHi2->getZExtValue() == 0 || offsetHi2->getZExtValue() == ~0u)) // offset is a 32-bit value
                                {
                                    offset += offsetLo2->getSExtValue();
                                }
                                else if (offsetHi2->getZExtValue() == 0)
                                {
                                    GetPotentialPushableAddresses(genIntr2->getOperand(2)); // offset
                                }
                                genIntr2 = dyn_cast<GenIntrinsicInst>(pAddress);
                            }
                            else
                            {
                                // Not a supported pattern
                                pAddress = nullptr;
                                break;
                            }
                        }
                        if (pAddress)
                        {
                            GetPotentialPushableAddresses(pAddress);
                        }
                    }
                }
            }
        }
        else
        {
            /*
            Examples of patterns matched on platforms with 64bit type support:
            1. Pushable 64bit address + immediate offset case:
               %33 = ptrtoint <4 x float> addrspace(2)* %runtime_value_4 to i64
               %34 = add i64 %33, 368
               %chunkPtr = inttoptr i64 % 34 to <2 x i32> addrspace(2)*

            2. Pushable 64bit address + pushable 32bit offset + immediate
               offset case:
               %0 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 3)
               %1 = add i32 %0, 4
               %2 = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 1)
               %3 = zext i32 %1 to i64
               %4 = add i64 %2, %3
               %5 = inttoptr i64 %4 to i8 addrspace(1507329)* addrspace(2)*
            */

            offset = 0;
            GetPotentialPushableAddresses(pAddress);
        }

        if (offset >= 0 && offset <= std::numeric_limits<uint>::max() && // pushableOffset is a positive 32bit value
            (potentialPushableAddresses.size() == 1 ||
             potentialPushableAddresses.size() == 2))
        {
            for (Value* potentialAddress : potentialPushableAddresses)
            {
                bool isPushable = IsPushableAddress(
                    inst,
                    potentialAddress,
                    pushableAddressGrfOffset,
                    pushableOffsetGrfOffset);
                if (!isPushable)
                {
                    return false;
                }
            }
            IGC_ASSERT(pushableAddressGrfOffset >= 0);
            pushableOffset = int_cast<uint>(offset);
            return true;
        }

        return false;
    }


    bool PushAnalysis::IsPushableAddress(
        llvm::Instruction* inst,
        llvm::Value* pAddress,
        int& pushableAddressGrfOffset,
        int& pushableOffsetGrfOffset)
    {
        // skip casts
        while (
            isa<IntToPtrInst>(pAddress) ||
            isa<PtrToIntInst>(pAddress) ||
            isa<BitCastInst>(pAddress))
        {
            pAddress = cast<Instruction>(pAddress)->getOperand(0);
        }

        llvm::GenIntrinsicInst* pRuntimeVal = llvm::dyn_cast<llvm::GenIntrinsicInst>(pAddress);
        uint runtimeval0 = 0;

        bool isRuntimeValFound = false;
        if (pRuntimeVal == nullptr)
        {
            auto it = std::find(m_argList.begin(), m_argList.end(), pAddress);
            if (it != m_argList.end())
            {
                int argIndex = (int) std::distance(m_argList.begin(), it);
                PushInfo& pushInfo = m_context->getModuleMetaData()->pushInfo;
                for (auto index_it = pushInfo.constantReg.begin(); index_it != pushInfo.constantReg.end(); ++index_it)
                {
                    if (index_it->second == argIndex)
                    {
                        runtimeval0 = index_it->first;
                        isRuntimeValFound = true;
                    }
                }
            }
            if (!isRuntimeValFound)
                return false;
        }
        else if(pRuntimeVal->getIntrinsicID() != llvm::GenISAIntrinsic::GenISA_RuntimeValue)
            return false;
        if (!isRuntimeValFound)
            runtimeval0 = (uint)llvm::cast<llvm::ConstantInt>(pRuntimeVal->getOperand(0))->getZExtValue();

        uint runtimevalSize = GetSizeInBits(pAddress->getType());
        IGC_ASSERT(32 == runtimevalSize ||
            64 == runtimevalSize);
        const bool is64Bit = 64 == runtimevalSize;

        PushInfo& pushInfo = m_context->getModuleMetaData()->pushInfo;

        // then check for static flag so that we can do push safely
        for (const auto& it : pushInfo.pushableAddresses)
        {
            if (runtimeval0 * 4 != it.addressOffset)
            {
                continue;
            }

            if (IGC_IS_FLAG_ENABLED(DisableStaticCheck) ||
                it.isStatic ||
                IsSafeToPushNonStaticBufferLoad(inst))
            {
                // only a single 64bit pushable address and a single 32bit
                // pushable offset is supported.
                if (is64Bit && pushableAddressGrfOffset == -1)
                {
                    pushableAddressGrfOffset = runtimeval0;
                    return true;
                }
                else if (!is64Bit && pushableOffsetGrfOffset == -1)
                {
                    pushableOffsetGrfOffset = runtimeval0;
                    return true;
                }
            }
        }


        return false;
    }

    bool PushAnalysis::IsSafeToPushNonStaticBufferLoad(llvm::Instruction* inst)
    {
        if (!m_PDT)
        {
            m_PDT = &getAnalysis<PostDominatorTreeWrapperPass>(*m_pFunction).getPostDomTree();
        }
        if (!m_DT)
        {
            m_DT = &getAnalysis<DominatorTreeWrapperPass>(*m_pFunction).getDomTree();
        }
        // Find the return BB or the return BB before discard lowering.
        bool searchForRetBBBeforeDiscard = false;
        BasicBlock* retBB = m_PDT->getRootNode()->getBlock();
        if (!retBB)
        {
#if LLVM_VERSION_MAJOR <= 10
            auto& roots = m_PDT->getRoots();
            IGC_ASSERT_MESSAGE(roots.size() == 1, "Unexpected multiple roots");
            retBB = roots[0];
#else
            auto roots = m_PDT->root_begin();
            IGC_ASSERT_MESSAGE(m_PDT->root_size() == 1, "Unexpected multiple roots");
            retBB = *roots;
#endif
        }

        for (auto& II : m_pFunction->getEntryBlock())
        {
            if (isa<GenIntrinsicInst>(&II, GenISAIntrinsic::GenISA_InitDiscardMask))
            {
                searchForRetBBBeforeDiscard = true;
                break;
            }
        }

        if (searchForRetBBBeforeDiscard)
        {
            for (auto it = pred_begin(retBB), ie = pred_end(retBB); it != ie; ++it)
            {
                BasicBlock* predBB = *it;
                BranchInst* br = cast<BranchInst>(predBB->getTerminator());
                if (br->isUnconditional())
                {
                    retBB = predBB;
                    break;
                }
            }
        }

        if (m_DT->dominates(inst->getParent(), retBB))
        {
            return true;
        }

        return false;
    }

    //
    // Calculate the offset in buffer relative to the dynamic uniform buffer offset.
    //
    // Below is an example of pattern we're looking for:
    //
    //   %runtime_value_1 = call fast float @llvm.genx.GenISA.RuntimeValue(i32 1)
    //   %spv.bufferOffset.mdNode0.cr1 = bitcast float %runtime_value_1 to i32
    //   %1 = and i32 %spv.bufferOffset.mdNode1.cr1, -16
    //   %2 = add i32 %1, 24
    //   %fromGBP45 = inttoptr i32 %2 to float addrspace(65536)* // BTI = 0
    //   %ldrawidx46 = load float, float addrspace(65536)* %fromGBP45, align 4
    //
    // For the above example the the function will:
    //  - check if uniform buffer with bti=0 has its dynamic offset in
    //    runtime value 1
    //  - return relativeOffsetInBytes = 24
    // This method will return true if the input UBO is a dynamic uniform buffer
    // and the input offset value is a sum of buffer's dynamic offset and an
    // immediate constant value.
    //
    bool PushAnalysis::GetConstantOffsetForDynamicUniformBuffer(
        Value* offsetValue, // total buffer offset i.e. starting from 0
        uint& relativeOffsetInBytes) // offset in bytes starting from dynamic offset in buffer
    {
        if (ConstantInt * constOffset = dyn_cast<ConstantInt>(offsetValue))
        {
            relativeOffsetInBytes = int_cast<uint>(constOffset->getZExtValue());
            return true;
        }
        else if (Operator::getOpcode(offsetValue) == Instruction::Add)
        {
            const Instruction* addInst = cast<Instruction>(offsetValue);
            uint offset0 = 0, offset1 = 0;

            if (GetConstantOffsetForDynamicUniformBuffer(addInst->getOperand(0), offset0) &&
                GetConstantOffsetForDynamicUniformBuffer(addInst->getOperand(1), offset1))
            {
                relativeOffsetInBytes = offset0 + offset1;
                return true;
            }
        }
        else if (Operator::getOpcode(offsetValue) == Instruction::Mul)
        {
            const Instruction* mulInst = cast<Instruction>(offsetValue);
            uint a = 0, b = 0;

            if (GetConstantOffsetForDynamicUniformBuffer(mulInst->getOperand(0), a) &&
                GetConstantOffsetForDynamicUniformBuffer(mulInst->getOperand(1), b))
            {
                relativeOffsetInBytes = a * b;
                return true;
            }

        }
        else if (Operator::getOpcode(offsetValue) == Instruction::Shl)
        {
            const Instruction* shlInst = cast<Instruction>(offsetValue);
            uint offset = 0, bitShift = 0;

            if (GetConstantOffsetForDynamicUniformBuffer(shlInst->getOperand(0), offset) &&
                GetConstantOffsetForDynamicUniformBuffer(shlInst->getOperand(1), bitShift))
            {
                relativeOffsetInBytes = offset << bitShift;
                return true;
            }
        }
        else if (Operator::getOpcode(offsetValue) == Instruction::And)
        {
            const Instruction* andInst = cast<Instruction>(offsetValue);
            ConstantInt* src1 = dyn_cast<ConstantInt>(andInst->getOperand(1));
            if (src1 &&
                (int_cast<uint>(src1->getZExtValue()) == 0xFFFFFFE0 ||
                 int_cast<uint>(src1->getZExtValue()) == 0xFFFFFFF0))
            {
                uint offset = 0;
                if (GetConstantOffsetForDynamicUniformBuffer(andInst->getOperand(0), offset) &&
                    (offset & int_cast<uint>(src1->getZExtValue())) == offset)
                {
                    relativeOffsetInBytes = offset;
                    return true;
                }
            }
        }
        else if (Operator::getOpcode(offsetValue) == Instruction::Or)
        {
            Instruction* orInst = cast<Instruction>(offsetValue);
            Instruction* src0 = dyn_cast<Instruction>(orInst->getOperand(0));
            ConstantInt* src1 = dyn_cast<ConstantInt>(orInst->getOperand(1));
            if (src1 && src0 &&
                src0->getOpcode() == Instruction::And &&
                isa<ConstantInt>(src0->getOperand(1)))
            {
                uint orOffset = int_cast<uint>(src1->getZExtValue());
                uint andMask = int_cast<uint>(cast<ConstantInt>(src0->getOperand(1))->getZExtValue());
                uint offset = 0;
                if ((orOffset & andMask) == 0 &&
                    GetConstantOffsetForDynamicUniformBuffer(src0->getOperand(0), offset) &&
                    offset == 0)
                {
                    relativeOffsetInBytes = orOffset;
                    return true;
                }
            }
        }
        else if (BitCastInst * bitCast = dyn_cast<BitCastInst>(offsetValue))
        {
            return GetConstantOffsetForDynamicUniformBuffer(bitCast->getOperand(0), relativeOffsetInBytes);
        }
        else if (IntToPtrInst * i2p = dyn_cast<IntToPtrInst>(offsetValue))
        {
            return GetConstantOffsetForDynamicUniformBuffer(i2p->getOperand(0), relativeOffsetInBytes);
        }
        else if (m_dynamicBufferOffsetArgs.find(offsetValue) != m_dynamicBufferOffsetArgs.end())
        {
            relativeOffsetInBytes = 0;
            return true;
        }

        return false;
    }

    /// The constant-buffer id and element id must be compile-time immediate
    /// in order to be added to thread-payload
    bool PushAnalysis::IsPushableShaderConstant(
        Instruction* inst,
        SimplePushInfo& info)
    {
        Value* eltPtrVal = nullptr;
        if (!isa<LoadInst>(inst) && !isa<LdRawIntrinsic>(inst))
            return false;

        Type* loadType = inst->getType();
        Type* elemType = loadType->getScalarType();
        // TODO: enable promotion of 8 bit, 16 bit and 64 bit values
        if (!elemType->isFloatTy() &&
            !elemType->isIntegerTy(32) &&
            !(elemType->isPointerTy() && GetSizeInBits(elemType) == 32))
        {
            return false;
        }

        if (IsPushableBindlessLoad(
            inst,
            info.pushableOffsetGrfOffset,
            info.cbIdx,
            info.offset))
        {
            if ((info.offset % 4) == 0)
            {
                info.isBindless = true;
                return true;
            }
        }
        else if (IsLoadFromDirectCB(
            inst,
            info.cbIdx,
            eltPtrVal))
        {
            if (info.cbIdx == getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData()->pushInfo.inlineConstantBufferSlot)
            {
                return false;
            }
            if (isa<ConstantPointerNull>(eltPtrVal))
            {
                info.offset = 0;
                return true;
            }
            if (IntToPtrInst * i2p = dyn_cast<IntToPtrInst>(eltPtrVal))
            {
                Value* eltIdxVal = i2p->getOperand(0);
                if (ConstantInt * eltIdx = dyn_cast<ConstantInt>(eltIdxVal))
                {
                    info.offset = int_cast<uint>(eltIdx->getZExtValue());
                    if ((info.offset % 4) == 0)
                    {
                        return true;
                    }
                }
            }

            if (m_context->m_DriverInfo.SupportsDynamicUniformBuffers() &&
                IGC_IS_FLAG_DISABLED(DisableSimplePushWithDynamicUniformBuffers) &&
                !m_context->getModuleMetaData()->pushInfo.dynamicBufferInfo.forceDisabled)
            {
                unsigned int relativeOffsetInBytes = 0; // offset in bytes starting from dynamic buffer offset
                if (GetConstantOffsetForDynamicUniformBuffer(eltPtrVal, relativeOffsetInBytes))
                {
                    info.offset = relativeOffsetInBytes;
                    if ((info.offset % 4) == 0)
                    {
                        return true;
                    }
                }
            }
            return false;
        }
        else if (IsStatelessCBLoad(
            inst,
            info.pushableAddressGrfOffset,
            info.pushableOffsetGrfOffset,
            info.offset))
        {
            IGC_ASSERT(info.pushableAddressGrfOffset >= 0);
            info.cbIdx = 0;
            info.isStateless = true;
            return true;
        }
        return false;
    }

    bool PushAnalysis::AreUniformInputsBasedOnDispatchMode()
    {
        return false;
    }

    bool PushAnalysis::DispatchGRFHardwareWAForHSAndGSDisabled()
    {
        // If the WA does not apply, we can push constants.
        if (!m_context->platform.WaDispatchGRFHWIssueInGSAndHSUnit())
        {
            return true;
        }

        return true;
    }



    bool PushAnalysis::CanPushConstants()
    {
        if (IGC_GET_FLAG_VALUE(DisablePushConstant) & 0x1 ||
            IGC_GET_FLAG_VALUE(DisablePushConstant) & (1 << static_cast<unsigned int>(m_context->type)))
        {
            return false;
        }

        if (!m_context->getModuleMetaData()->compOpt.PushConstantsEnable)
        {
            return false;
        }

        if (m_context->m_DriverInfo.WaDisablePushConstantsWithNoPushedAttributes() &&
            m_context->platform.getWATable().Wa_16011983264 &&
            m_context->type == ShaderType::HULL_SHADER)
        {
            Function* URBReadFunction = GenISAIntrinsic::getDeclaration(m_pFunction->getParent(), GenISAIntrinsic::GenISA_URBRead);
            if (GetMaxNumberOfPushedInputs() == 0 ||
                none_of(URBReadFunction->users(), [](const User* user) { return isa<GenIntrinsicInst>(user); }))
            {
                return false;
            }
        }

        return false;
    }

    unsigned int PushAnalysis::GetMaxNumberOfPushedInputs()
    {

        if (IGC_GET_FLAG_VALUE(DisableAttributePush) & 0x1 ||
            IGC_GET_FLAG_VALUE(DisableAttributePush) & static_cast<unsigned int>(m_context->type))
        {
            return 0;
        }

        return 0;
    }

    unsigned int PushAnalysis::GetSizeInBits(Type* type) const
    {
        unsigned int size = (unsigned int)type->getPrimitiveSizeInBits();
        if (type->isPointerTy())
        {
            size = m_DL->getPointerSizeInBits(type->getPointerAddressSpace());
            IGC_ASSERT(size == 32 || size == 64);
        }
        else if (type->isVectorTy() &&
            type->getScalarType()->isPointerTy())
        {
            size = m_DL->getPointerSizeInBits(
                type->getScalarType()->getPointerAddressSpace());
            IGC_ASSERT(size == 32 || size == 64);
            size = size * int_cast<uint>(cast<IGCLLVM::FixedVectorType>(type)->getNumElements());
        }
        return size;
    }

    void PushAnalysis::AllocatePushedConstant(
        Instruction* load,
        const SimplePushInfo& newChunk,
        const unsigned int maxSizeAllowed)
    {
        if (!newChunk.isBindless &&
            newChunk.cbIdx > m_context->m_DriverInfo.MaximumSimplePushBufferID())
        {
            return;
        }
        unsigned int size = GetSizeInBits(load->getType()) / 8;
        IGC_ASSERT_MESSAGE(isa<LoadInst>(load) || isa<LdRawIntrinsic>(load),
            "Expected a load instruction");

        // greedy allocation for now
        // first check if we are already pushing from the buffer
        unsigned int piIndex;
        bool regionFound = false;

        for (piIndex = 0; piIndex < numSimplePush; piIndex++)
        {
            const SimplePushData& info = CollectAllSimplePushInfoArr[piIndex];
            // Stateless load - GRF offsets need to match.
            if (info.isStateless &&
                newChunk.isStateless &&
                info.pushableAddressGrfOffset == newChunk.pushableAddressGrfOffset &&
                info.pushableOffsetGrfOffset == newChunk.pushableOffsetGrfOffset)
            {
                regionFound = true;
                break;
            }
            // Bindless load - GRF offsets and bindless surface state offsets
            // need to match.
            if (info.isBindless &&
                newChunk.isBindless &&
                info.pushableOffsetGrfOffset == newChunk.pushableOffsetGrfOffset &&
                info.cbIdx == newChunk.cbIdx)
            {
                regionFound = true;
                break;
            }
            // Bound load - binding table indices need to match.
            if (!info.isStateless &&
                !newChunk.isStateless &&
                !info.isBindless &&
                !newChunk.isBindless &&
                info.cbIdx == newChunk.cbIdx)
            {
                regionFound = true;
                break;
            }
        }
        if (regionFound)
        {
            SimplePushData& info = CollectAllSimplePushInfoArr[piIndex];
            unsigned int newStartOffset = iSTD::RoundDown(
                std::min(newChunk.offset, info.offset),
                getMinPushConstantBufferAlignmentInBytes());
            unsigned int newEndOffset = iSTD::Round(
                std::max(newChunk.offset + size, info.offset + info.size),
                getMinPushConstantBufferAlignmentInBytes());
            unsigned int newSize = newEndOffset - newStartOffset;

            if (newSize <= maxSizeAllowed)
            {
                info.offset = newStartOffset;
                info.size = newSize;
                info.Load.push_back(std::make_pair(load, newChunk.offset));
            }
        }

        // we couldn't add it to an existing buffer try to add a new one if there is a slot available
        else
        {
            unsigned int newStartOffset = iSTD::RoundDown(newChunk.offset, getMinPushConstantBufferAlignmentInBytes());
            unsigned int newEndOffset = iSTD::Round(newChunk.offset + size, getMinPushConstantBufferAlignmentInBytes());
            unsigned int newSize = newEndOffset - newStartOffset;

            if (newSize <= maxSizeAllowed)
            {
                SimplePushData& info = CollectAllSimplePushInfoArr[numSimplePush];
                info.pushableAddressGrfOffset = newChunk.pushableAddressGrfOffset;
                info.pushableOffsetGrfOffset = newChunk.pushableOffsetGrfOffset;
                info.cbIdx = newChunk.cbIdx;
                info.isStateless = newChunk.isStateless;
                info.isBindless = newChunk.isBindless;
                info.offset = newStartOffset;
                info.size = newSize;
                info.Load.push_back(std::make_pair(load, newChunk.offset));
                numSimplePush++;
            }
        }
        return;
    }

    void PushAnalysis::PromoteLoadToSimplePush(Instruction* load, SimplePushInfo& info, unsigned int offset)
    {
        unsigned int num_elms = 1;
        llvm::Type* pTypeToPush = load->getType();
        if (pTypeToPush->isPointerTy())
        {
            pTypeToPush = IntegerType::get(
                load->getContext(),
                m_DL->getPointerSizeInBits(pTypeToPush->getPointerAddressSpace()));
        }
        llvm::Value* pReplacedInst = nullptr;
        llvm::Type* pScalarTy = pTypeToPush;

        if (pTypeToPush->isVectorTy())
        {
            num_elms = (unsigned)cast<IGCLLVM::FixedVectorType>(pTypeToPush)->getNumElements();
            pTypeToPush = cast<VectorType>(pTypeToPush)->getElementType();
            llvm::Type* pVecTy = IGCLLVM::FixedVectorType::get(pTypeToPush, num_elms);
            pReplacedInst = llvm::UndefValue::get(pVecTy);
            pScalarTy = cast<VectorType>(pVecTy)->getElementType();
        }

        SmallVector< SmallVector<ExtractElementInst*, 1>, 4> extracts(num_elms);
        bool allExtract = VectorUsedByConstExtractOnly(load, extracts);

        for (unsigned int i = 0; i < num_elms; ++i)
        {
            uint address = offset + i * (GetSizeInBits(pScalarTy) / 8);
            auto it = info.simplePushLoads.find(address);
            llvm::Value* value = nullptr;
            if (it != info.simplePushLoads.end())
            {
                // Value is already getting pushed
                IGC_ASSERT_MESSAGE((it->second <= m_argIndex), "Function arguments list and metadata are out of sync!");
                value = m_argList[it->second];
            }
            else
            {
                value = addArgumentAndMetadata(pTypeToPush, VALUE_NAME("cb"), WIAnalysis::UNIFORM_GLOBAL);
                info.simplePushLoads.insert(std::make_pair(address, m_argIndex));
            }
            if (pTypeToPush != value->getType())
            {
                IGC_ASSERT(pTypeToPush->isPointerTy() == value->getType()->isPointerTy());
                value = pTypeToPush->isPointerTy() ?
                    CastInst::CreatePointerBitCastOrAddrSpaceCast(value, pTypeToPush, "", load) :
                    CastInst::CreateZExtOrBitCast(value, pTypeToPush, "", load);
            }

            if (load->getType()->isVectorTy())
            {
                if (!allExtract)
                {
                    pReplacedInst = llvm::InsertElementInst::Create(
                        pReplacedInst, value, llvm::ConstantInt::get(llvm::IntegerType::get(load->getContext(), 32), i), "", load);
                }
                else
                {
                    for (auto II : extracts[i])
                    {
                        II->replaceAllUsesWith(value);
                    }
                }
            }
            else
            {
                if (load->getType()->isPointerTy())
                {
                    value = IntToPtrInst::Create(
                        Instruction::IntToPtr,
                        value,
                        load->getType(),
                        load->getName(),
                        load);
                }
                pReplacedInst = value;
            }

        }
        if (!allExtract)
        {
            load->replaceAllUsesWith(pReplacedInst);
        }
    }


    void PushAnalysis::BlockPushConstants()
    {
        auto& inputs = m_context->getModuleMetaData()->pushInfo.inputs;
        typedef const std::map<unsigned int, SInputDesc>::value_type& inputPairType;
        unsigned int largestIndex = 0;
        auto largestPair = std::max_element(inputs.begin(), inputs.end(),
            [](inputPairType a, inputPairType b) { return a.second.index < b.second.index; });
        largestIndex = largestPair != inputs.end() ? largestPair->second.index : 0;

        uint32_t maxPushedGRFs = m_context->getNumGRFPerThread(false) ? (3 * m_context->getNumGRFPerThread(false)) / 4 :
            m_context->platform.supportsVRT() ? ((3 * 256) / 4) : ((3 * 128) / 4);

        if (largestIndex >= maxPushedGRFs)
        {
            return;
        }
        // push up to 992 bytes of constants (31 32-byte units; this is
        // the maximum value the read length in 3DSTATE_CONSTANT_ALL allows)
        // The maximum number of GRFs used for all pushed data is 96.
        const unsigned int cthreshold =
            std::min(m_pullConstantHeuristics->getPushConstantThreshold(m_pFunction) * getMinPushConstantBufferAlignmentInBytes(),
                     (maxPushedGRFs - largestIndex) * getGRFSize());
        unsigned int sizePushed = 0;

        // Runtime values are changed to intrinsics. So we need to do it before.
        for (auto bb = m_pFunction->begin(), be = m_pFunction->end(); bb != be; ++bb)
        {
            for (auto i = bb->begin(), ie = bb->end(); i != ie; ++i)
            {
                Instruction* const instr = &(*i);
                SimplePushInfo info{};
                bool isPushable = IsPushableShaderConstant(instr, info);
                if(isPushable)
                {
                    AllocatePushedConstant(
                        instr,
                        info,
                        cthreshold); // maxSizeAllowed
                }
            }
        }


        PushInfo& pushInfo = m_context->getModuleMetaData()->pushInfo;
        unsigned int simplePushBufferId = 0;
        while ((pushInfo.simplePushBufferUsed < pushInfo.MaxNumberOfPushedBuffers) && CollectAllSimplePushInfoArr.size())
        {
            unsigned int iter = CollectAllSimplePushInfoArr.begin()->first;
            SimplePushData info;
            if (IGC_IS_FLAG_ENABLED(EnableSimplePushSizeBasedOpimization))
            {
                for (auto I = CollectAllSimplePushInfoArr.begin(), E = CollectAllSimplePushInfoArr.end(); I != E; I++)
                {
                    if (I->second.size > info.size)
                    {
                        info = I->second;
                        iter = I->first;
                    }
                }
            }
            else
            {
                info = CollectAllSimplePushInfoArr[simplePushBufferId];
                iter = simplePushBufferId;
            }
            SimplePushInfo& newChunk = pushInfo.simplePushInfoArr[pushInfo.simplePushBufferUsed];
            if (sizePushed + info.size <= cthreshold)
            {
                newChunk.cbIdx = info.cbIdx;
                newChunk.isBindless = info.isBindless;
                newChunk.isStateless = info.isStateless;
                newChunk.offset = info.offset;
                newChunk.size = info.size;
                newChunk.pushableAddressGrfOffset = info.pushableAddressGrfOffset;
                newChunk.pushableOffsetGrfOffset = info.pushableOffsetGrfOffset;
                for (auto I = info.Load.begin(), E = info.Load.end(); I != E; I++)
                    PromoteLoadToSimplePush(I->first, newChunk, I->second);
                pushInfo.simplePushBufferUsed++;
                sizePushed += info.size;
            }
            CollectAllSimplePushInfoArr.erase(iter);
            simplePushBufferId++;
        }
    }

    PushConstantMode PushAnalysis::GetPushConstantMode()
    {
        PushConstantMode pushConstantMode = PushConstantMode::DEFAULT;

        if(!CanPushConstants())
        {
            // Hardware can not do push constant mode or the registry has been set in a way to disable push altogether
            pushConstantMode = PushConstantMode::NONE;
        }
        else
        {
            // Priority order of how the push constant mode is determined:
            //   1.) Registry Keys
            //   2.) UMD Input
            //   3.) Compiler Input
            //   4.) Default Logic dependent on platform and attributes of the shader

            // 1.) Check registry keys
            if(pushConstantMode == PushConstantMode::DEFAULT)
            {
                if(IGC_IS_FLAG_ENABLED(forcePushConstantMode))
                {
                    pushConstantMode = (PushConstantMode)IGC_GET_FLAG_VALUE(forcePushConstantMode);
                }
            }

            // 2.) Check UMD input
            if (pushConstantMode == PushConstantMode::DEFAULT)
            {
                uint32_t val = m_context->getModuleMetaData()->compOpt.ForcePushConstantMode;
                if (val != 0)
                {
                    pushConstantMode = (PushConstantMode)val;
                }
            }

            // 3.) Check compiler input
            if (pushConstantMode == PushConstantMode::DEFAULT)
            {
                pushConstantMode = m_context->m_pushConstantMode;
            }

            // 4.) Default Logic dependent on platform and attributes of the shader
            if (pushConstantMode == PushConstantMode::DEFAULT)
            {
                if (m_context->m_DriverInfo.SupportsSimplePushOnly())
                {
                    pushConstantMode = PushConstantMode::SIMPLE;
                }
                else if (m_context->platform.supportsHardwareResourceStreamer() || m_context->m_DriverInfo.SupportsGatherConstantOnly())
                {
                    pushConstantMode = PushConstantMode::GATHER;
                }
                else
                {
                    // CPU copy
                    pushConstantMode = PushConstantMode::GATHER;
                }
            }
        }

        IGC_ASSERT_MESSAGE(pushConstantMode != PushConstantMode::DEFAULT, "GetPushConstantMode should resolve a non-DEFAULT mode!");
        return pushConstantMode;
    }

    // process gather constants, update PushInfo.constants
    void PushAnalysis::processGather(Instruction* inst, uint bufId, uint eltId)
    {
        PushInfo& pushInfo = m_context->getModuleMetaData()->pushInfo;

        eltId = eltId >> 2;
        if (!m_context->m_DriverInfo.Uses3DSTATE_DX9_CONSTANT() &&
            (eltId + inst->getType()->getPrimitiveSizeInBits() / 8) >= (MaxConstantBufferIndexSize * 4))
        {
            // Hardware supports pushing more than 256 constants
            // in case 3DSTATE_DX9_CONSTANT mode is used
            return;
        }
        else if (bufId > 15)
        {
            // resource streamer cannot push above buffer slot 15 and driver doesn't allow
            // pushing inlined constant buffer
            // should not be pushed and should always be pulled
            return;
        }

        unsigned num_elms =
            inst->getType()->isVectorTy() ? (unsigned)cast<IGCLLVM::FixedVectorType>(inst->getType())->getNumElements() : 1;
        llvm::Type* pTypeToPush = inst->getType();
        llvm::Value* replaceVector = nullptr;
        unsigned int numberChannelReplaced = 0;
        SmallVector< SmallVector<ExtractElementInst*, 1>, 4> extracts(num_elms);
        bool allExtract = false;
        if (inst->getType()->isVectorTy())
        {
            allExtract = LoadUsedByConstExtractOnly(cast<LoadInst>(inst), extracts);
            pTypeToPush = cast<VectorType>(inst->getType())->getElementType();
        }

        for (unsigned int i = 0; i < num_elms; ++i)
        {
            if (allExtract && extracts[i].empty())
                continue;
            ConstantAddress address;
            address.bufId = bufId;
            address.eltId = eltId + i;

            auto it = pushInfo.constants.find(address);
            if (it != pushInfo.constants.end() ||
                (pushInfo.constantReg.size() + pushInfo.constants.size() < m_pullConstantHeuristics->getPushConstantThreshold(m_pFunction) * 8))
            {
                llvm::Value* value = nullptr;

                // The sum of all the 4 constant buffer read lengths must be <= size of 64 units.
                // Each unit is of size 256-bit units. In some UMDs we program the
                // ConstantRegisters and Constant Buffers in the ConstantBuffer0ReadLength. And this causes
                // shaders to crash when the read length is > 64 units. To be safer we program the total number of
                // GRF's used to 32 registers.
                if (it == pushInfo.constants.end())
                {
                    // We now put the Value as an argument to make sure its liveness starts
                    // at the beginning of the function and then we remove the instruction
                    // We now put the Value as an argument to make sure its liveness starts
                    // at the beginning of the function and then we remove the instruction
                    value = addArgumentAndMetadata(pTypeToPush,
                        VALUE_NAME(std::string("cb_") + to_string(bufId) + std::string("_") + to_string(eltId) + std::string("_elm") + to_string(i)),
                        WIAnalysis::UNIFORM_GLOBAL);
                    if (pTypeToPush != value->getType())
                        value = CastInst::CreateZExtOrBitCast(value, pTypeToPush, "", inst);

                    pushInfo.constants[address] = m_argIndex;
                }
                else
                {
                    IGC_ASSERT_MESSAGE((it->second <= m_argIndex), "Function arguments list and metadata are out of sync!");
                    value = m_argList[it->second];
                    if (pTypeToPush != value->getType())
                        value = CastInst::CreateZExtOrBitCast(value, pTypeToPush, "", inst);
                }

                if (inst->getType()->isVectorTy())
                {
                    if (!allExtract)
                    {
                        numberChannelReplaced++;
                        if (replaceVector == nullptr)
                        {
                            replaceVector = llvm::UndefValue::get(inst->getType());
                        }
                        replaceVector = llvm::InsertElementInst::Create(
                            replaceVector, value, llvm::ConstantInt::get(llvm::IntegerType::get(inst->getContext(), 32), i), "", inst);
                    }
                    else
                    {
                        for (auto II : extracts[i])
                        {
                            II->replaceAllUsesWith(value);
                        }
                    }
                }
                else
                {
                    inst->replaceAllUsesWith(value);
                }
            }
            if (replaceVector != nullptr && numberChannelReplaced == num_elms)
            {
                // for vector replace, we only replace the load if we were going to push
                // all the channels
                inst->replaceAllUsesWith(replaceVector);
            }
        }
    }

    void PushAnalysis::processInput(Instruction* inst, bool gsInstancingUsed)
    {
        PushInfo& pushInfo = m_context->getModuleMetaData()->pushInfo;
        e_interpolation mode = (e_interpolation)llvm::cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue();
        if (
            (mode == EINTERPOLATION_VERTEX || mode == EINTERPOLATION_CONSTANT))
        {
            // inputs which get pushed are set as function arguments in order to have the correct liveness
            if (llvm::ConstantInt * pIndex = llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(0)))
            {
                SInputDesc input;
                input.index = static_cast<uint>(pIndex->getZExtValue());
                input.interpolationMode = mode;
                // if we know input is uniform, update WI analysis results.
                const bool uniformInput =
                    (mode == EINTERPOLATION_CONSTANT) || gsInstancingUsed;

                auto it = pushInfo.inputs.find(input.index);
                if (it == pushInfo.inputs.end())
                {
                    IGC_ASSERT(inst->getType()->isHalfTy() || inst->getType()->isFloatTy());
                    llvm::Type* floatTy = Type::getFloatTy(m_pFunction->getContext());
                    addArgumentAndMetadata(floatTy, VALUE_NAME(std::string("input_") + to_string(input.index)),
                        uniformInput ? WIAnalysis::UNIFORM_GLOBAL : WIAnalysis::RANDOM);
                    input.argIndex = m_argIndex;
                    pushInfo.inputs[input.index] = input;
                }
                else
                {
                    IGC_ASSERT_MESSAGE((it->second.argIndex <= m_argIndex), "Function arguments list and metadata are out of sync!");
                    input.argIndex = it->second.argIndex;
                }
                llvm::Value* replacementValue = m_argList[input.argIndex];
                if (inst->getType()->isHalfTy() && replacementValue->getType()->isFloatTy())
                {
                    // Input is accessed using the half version of intrinsic, e.g.:
                    //     call half @llvm.genx.GenISA.DCL.inputVec.f16 (i32 13, i32 2)
                    replacementValue = CastInst::CreateFPCast(replacementValue, inst->getType(), "", inst);
                }
                inst->replaceAllUsesWith(replacementValue);
            }
        }
    }

    void PushAnalysis::processInputVec(Instruction* inst, bool gsInstancingUsed)
    {
        // Multi-poly shaders cannot have mapped inputs
        if (m_context->type == ShaderType::PIXEL_SHADER && (m_context->platform.hasDualKSPPS() || m_context->platform.supportDualSimd8PS())) return;

        PushInfo& pushInfo = m_context->getModuleMetaData()->pushInfo;

        // If the input index of llvm_shaderinputvec is a constant
        if (llvm::ConstantInt * pIndex = llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(0)))
        {
            uint inputIndex = static_cast<uint>(pIndex->getZExtValue());
            e_interpolation mode = (e_interpolation)llvm::cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue();

            // If the input index of llvm_shaderinputvec is a constant then we pull inputs if inputIndex <= MaxNumOfPushedInputs
            if (pIndex && inputIndex <= MaxNumOfPushedInputs)
            {
                if (mode == EINTERPOLATION_CONSTANT || mode == EINTERPOLATION_VERTEX)
                {
                    for (auto I = inst->user_begin(), E = inst->user_end(); I != E; ++I)
                    {
                        if (llvm::ExtractElementInst * extract = llvm::dyn_cast<llvm::ExtractElementInst>(*I))
                        {
                            SInputDesc input;
                            // if input is i1.xyzw, inputIndex = 1*4, extract->getIndexOperand() is the component
                            input.index = inputIndex * 4 +
                                int_cast<uint>(llvm::cast<ConstantInt>(extract->getIndexOperand())->getZExtValue());
                            input.interpolationMode = mode;
                            // if we know input is uniform, update WI analysis results.
                            const bool uniformInput =
                                (mode == EINTERPOLATION_CONSTANT) || gsInstancingUsed;

                            auto it = pushInfo.inputs.find(input.index);
                            if (it == pushInfo.inputs.end())
                            {
                                addArgumentAndMetadata(extract->getType(), VALUE_NAME(std::string("pushedinput_") + to_string(input.index)),
                                    uniformInput ? WIAnalysis::UNIFORM_GLOBAL : WIAnalysis::RANDOM);
                                input.argIndex = m_argIndex;
                                pushInfo.inputs[input.index] = input;
                            }
                            else
                            {
                                IGC_ASSERT_MESSAGE((it->second.argIndex <= m_argIndex), "Function arguments list and metadata are out of sync!");
                                input.argIndex = it->second.argIndex;
                            }

                            extract->replaceAllUsesWith(m_argList[input.argIndex]);
                        }
                    }
                }
            }
            else
            {
                // This should never happen for geometry shader since we leave GS specific
                // intrinsic if we want pull model earlier in GS lowering.
                IGC_ASSERT(m_context->type != ShaderType::GEOMETRY_SHADER);
            }
        }
    }

    void PushAnalysis::processURBRead(Instruction* inst, bool gsInstancingUsed,
        bool vsHasConstantBufferIndexedWithInstanceId,
        uint32_t vsUrbReadIndexForInstanceIdSGV)
    {
        PushInfo& pushInfo = m_context->getModuleMetaData()->pushInfo;

        if (llvm::ConstantInt * pVertexIndex = llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(0)))
        {
            uint vertexIndex = static_cast<uint>(pVertexIndex->getZExtValue());
            uint numberOfElementsPerVertexThatAreGoingToBePushed = GetMaxNumberOfPushedInputs();

            // If the input index of llvm_shaderinputvec is a constant
            if (llvm::ConstantInt * pElementIndex = llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(1)))
            {
                int urbOffset = static_cast<int>(pElementIndex->getZExtValue());
                int urbReadOffsetForPush = 0;


                bool pushCondition = ((urbOffset >= urbReadOffsetForPush) && ((urbOffset - urbReadOffsetForPush) < (int)numberOfElementsPerVertexThatAreGoingToBePushed));

                // If the attribute index of URBRead is a constant then we pull
                // inputs if elementIndex <= numberOfElementsPerVertexThatAreGoingToBePushed
                if (pElementIndex && pushCondition)
                {
                    uint elementIndex = urbOffset - urbReadOffsetForPush;
                    uint currentElementIndex = (vertexIndex * numberOfElementsPerVertexThatAreGoingToBePushed * 4) + (elementIndex * 4);

                    for (auto I = inst->user_begin(), E = inst->user_end(); I != E; ++I)
                    {
                        llvm::ExtractElementInst* extract = llvm::dyn_cast<llvm::ExtractElementInst>(*I);
                        if (extract && llvm::isa<ConstantInt>(extract->getIndexOperand()))
                        {
                            SInputDesc input;
                            // if input is i1.xyzw, elementIndex = 1*4, extract->getIndexOperand() is the component
                            input.index =
                                currentElementIndex +
                                int_cast<uint>(cast<ConstantInt>(extract->getIndexOperand())->getZExtValue());
                            input.interpolationMode = AreUniformInputsBasedOnDispatchMode() ?
                                EINTERPOLATION_CONSTANT : EINTERPOLATION_VERTEX;

                            const bool uniformInput =
                                AreUniformInputsBasedOnDispatchMode() ||
                                (vsHasConstantBufferIndexedWithInstanceId &&
                                    vsUrbReadIndexForInstanceIdSGV == input.index) ||
                                gsInstancingUsed;

                            auto it = pushInfo.inputs.find(input.index);
                            if (it == pushInfo.inputs.end())
                            {
                                addArgumentAndMetadata(extract->getType(), VALUE_NAME(std::string("urb_read_") + to_string(input.index)),
                                    uniformInput ? WIAnalysis::UNIFORM_GLOBAL : WIAnalysis::RANDOM);
                                input.argIndex = m_argIndex;
                                pushInfo.inputs[input.index] = input;
                            }
                            else
                            {
                                IGC_ASSERT_MESSAGE((it->second.argIndex <= m_argIndex), "Function arguments list and metadata are out of sync!");
                                input.argIndex = it->second.argIndex;
                            }
                            extract->replaceAllUsesWith(m_argList[input.argIndex]);
                        }
                    }
                }
            }
        }
    }

    // process runtime value, update PushInfo.constantReg
    void PushAnalysis::processRuntimeValue(GenIntrinsicInst* intrinsic)
    {
        PushInfo& pushInfo = m_context->getModuleMetaData()->pushInfo;

        uint index = (uint)llvm::cast<llvm::ConstantInt>(intrinsic->getOperand(0))->getZExtValue();
        auto it = pushInfo.constantReg.find(index);
        Value* arg = nullptr;
        Value* runtimeValue = intrinsic;
        if (it == pushInfo.constantReg.end())
        {
            while (runtimeValue->hasOneUse() &&
                (isa<BitCastInst>(runtimeValue->user_back()) ||
                 isa<IntToPtrInst>(runtimeValue->user_back())))
            {
                Value* castInst = runtimeValue->user_back();
                if ((index % 2) != 0 &&
                    (64 == GetSizeInBits(castInst->getType())) &&
                    !castInst->getType()->isVectorTy())
                {
                    // 64bit runtime values must be 8B aligned in GRFs.
                    break;
                }
                runtimeValue = castInst;
            }
            arg = addArgumentAndMetadata(
                runtimeValue->getType(),
                VALUE_NAME(std::string("runtime_value_") + to_string(index)),
                WIAnalysis::UNIFORM_GLOBAL);
            pushInfo.constantReg[index] = m_argIndex;

            if (m_context->m_DriverInfo.SupportsDynamicUniformBuffers() &&
                IGC_IS_FLAG_DISABLED(DisableSimplePushWithDynamicUniformBuffers) &&
                !pushInfo.dynamicBufferInfo.forceDisabled &&
                pushInfo.dynamicBufferInfo.numOffsets > 0 &&
                index >= pushInfo.dynamicBufferInfo.firstIndex &&
                index < pushInfo.dynamicBufferInfo.firstIndex + pushInfo.dynamicBufferInfo.numOffsets)
            {
                m_dynamicBufferOffsetArgs.insert(arg);
            }

            for (unsigned int& runtimeValueIndex : pushInfo.bindlessPushInfo)
            {
                if (index == runtimeValueIndex)
                {
                    m_bindlessPushArgs[arg] = index;
                }
            }
        }
        else
        {
            IGC_ASSERT_MESSAGE(it->second <= m_argIndex, "Function arguments list and metadata are out of sync!");
            arg = m_argList[it->second];
            while (arg->getType() != runtimeValue->getType() &&
                runtimeValue->hasOneUse() &&
                (isa<BitCastInst>(runtimeValue->user_back()) ||
                    isa<IntToPtrInst>(runtimeValue->user_back())))
            {
                runtimeValue = runtimeValue->user_back();
            }
            if (arg->getType() != runtimeValue->getType())
            {
                arg = CastInst::CreateBitOrPointerCast(
                    arg, runtimeValue->getType(), "", cast<Instruction>(runtimeValue));
            }
        }
        runtimeValue->replaceAllUsesWith(arg);
    }

    //// Max number of control point inputs in 8 patch beyond which we always pull
    /// and do not try to use a hybrid approach of pull and push
    void PushAnalysis::ProcessFunction()
    {
        // if it's GS, get the properties object and find out if we use instancing
        // since then payload is laid out differently.
        const bool gsInstancingUsed =
            false;

        uint32_t vsUrbReadIndexForInstanceIdSGV = 0;
        bool vsHasConstantBufferIndexedWithInstanceId = false;

        m_funcTypeChanged = false;    // Reset flag at the beginning of processing every function

        PushConstantMode pushConstantMode = GetPushConstantMode();


        for (auto BB = m_pFunction->begin(), E = m_pFunction->end(); BB != E; ++BB)
        {
            for (auto &instRef : *BB)
            {
                auto inst = &instRef;
                // skip dead instructions
                if (inst->use_empty())
                {
                    continue;
                }
                // code to push constant-buffer value into thread-payload
                SimplePushInfo info{};
                if (pushConstantMode == PushConstantMode::GATHER &&
                    IsPushableShaderConstant(inst, info) &&
                    !info.isBindless &&
                    !info.isStateless)
                {
                    processGather(inst, info.cbIdx, info.offset);
                }
                else if (GetOpCode(inst) == llvm_input)
                {
                    processInput(inst, gsInstancingUsed);
                }
                else if (GetOpCode(inst) == llvm_shaderinputvec)
                {
                    processInputVec(inst, gsInstancingUsed);
                }
                else if (GenIntrinsicInst * intrinsic = dyn_cast<GenIntrinsicInst>(inst))
                {
                    if (DispatchGRFHardwareWAForHSAndGSDisabled() &&
                        (intrinsic->getIntrinsicID() == GenISAIntrinsic::GenISA_URBRead) &&
                        (m_context->type != ShaderType::DOMAIN_SHADER))
                    {
                        processURBRead(inst, gsInstancingUsed,
                            vsHasConstantBufferIndexedWithInstanceId,
                            vsUrbReadIndexForInstanceIdSGV);
                    }
                    else if (intrinsic->getIntrinsicID() == GenISAIntrinsic::GenISA_RuntimeValue)
                    {
                        processRuntimeValue(intrinsic);
                    }
                }
            }
        }

        if (pushConstantMode == PushConstantMode::SIMPLE)
        {
            BlockPushConstants();
        }

        // WA: Gen11+ HW doesn't work correctly if doubles are on vertex shader input and the input has unused components,
        // so ElementComponentEnableMask is not full => packing occurs
        // Code below fills gaps in inputs, so the ElementComponentEnableMask if full even if we don't use all
        // components in shader. This is needed, because in case of doubles there can be no URBRead if xy or zw
        // components are not used.
        // Right now only OGL is affected, so there is special disableVertexComponentPacking flag set by GLSL FE
        // if there is double on input to vertex shader
        if (!m_context->getModuleMetaData()->pushInfo.inputs.empty() &&
            m_context->type == ShaderType::VERTEX_SHADER &&
            m_context->getModuleMetaData()->compOpt.disableVertexComponentPacking)
        {
            PushInfo& pushInfo = m_context->getModuleMetaData()->pushInfo;
            int maxIndex = pushInfo.inputs.rbegin()->first;
            for (int i = 0; i < maxIndex; ++i)
            {
                if (pushInfo.inputs.find(i) == pushInfo.inputs.end())
                {
                    SInputDesc input;
                    // if input is i1.xyzw, elementIndex = 1*4, extract->getIndexOperand() is the component
                    input.index = i;
                    input.interpolationMode = EINTERPOLATION_VERTEX;

                    const bool uniformInput =
                        AreUniformInputsBasedOnDispatchMode() ||
                        (vsHasConstantBufferIndexedWithInstanceId &&
                            vsUrbReadIndexForInstanceIdSGV == input.index) ||
                        gsInstancingUsed;

                    addArgumentAndMetadata(Type::getFloatTy(m_pFunction->getContext()), VALUE_NAME(std::string("urb_read_") + to_string(input.index)),
                        uniformInput ? WIAnalysis::UNIFORM_GLOBAL : WIAnalysis::RANDOM);
                    input.argIndex = m_argIndex;
                    pushInfo.inputs[input.index] = input;
                }
            }
        }

        if (m_funcTypeChanged)
        {
            m_isFuncTypeChanged[m_pFunction] = true;
        }
        else
        {
            m_isFuncTypeChanged[m_pFunction] = false;
        }

        m_pMdUtils->save(m_pFunction->getContext());
    }

    bool PushAnalysis::runOnModule(llvm::Module& M)
    {
        m_DL = &M.getDataLayout();
        m_pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
        m_pullConstantHeuristics = &getAnalysis<PullConstantHeuristics>();
        m_context = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

        MapList<Function*, Function*> funcsMapping;
        bool retValue = false;

        for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
        {
            Function* pFunc = &(*I);

            // Only handle functions defined in this module
            if (pFunc->isDeclaration() || !isEntryFunc(m_pMdUtils, pFunc))
                continue;

            m_pFuncUpgrade.SetFunctionToUpgrade(pFunc);

            AnalyzeFunction(pFunc);

            // Find out if function pFunc's type/signature changed
            if (m_isFuncTypeChanged[pFunc])
            {
                retValue = true;

                // Create the new function body and insert it into the module
                Function* pNewFunc = m_pFuncUpgrade.RebuildFunction();


                // Map old func to new func
                funcsMapping[pFunc] = pNewFunc;

                // This is a kernel function, so there should not be any call site
                IGC_ASSERT(pFunc->use_empty());
            }
            m_pFuncUpgrade.Clean();
        }

        // Update IGC Metadata and shaders map
        // Function declarations are changing, this needs to be reflected in the metadata.
        MetadataBuilder mbuilder(&M);
        for (const auto& i : funcsMapping)
        {
            IGCMD::IGCMetaDataHelper::moveFunction(
                *m_pMdUtils, *m_context->getModuleMetaData(), i.first, i.second);
            mbuilder.UpdateShadingRate(i.first, i.second);
            auto& privateMemoryPerFG = m_context->getModuleMetaData()->PrivateMemoryPerFG;
            privateMemoryPerFG[i.second] = privateMemoryPerFG[i.first];
            privateMemoryPerFG.erase(i.first);
        }
        m_pMdUtils->save(M.getContext());

        GenXFunctionGroupAnalysis* FGA = getAnalysisIfAvailable<GenXFunctionGroupAnalysis>();

        // Go over all changed functions
        for (MapList<Function*, Function*>::const_iterator I = funcsMapping.begin(), E = funcsMapping.end(); I != E; ++I)
        {
            Function* pFunc = I->first;

            IGC_ASSERT_MESSAGE(pFunc->use_empty(), "Assume all user function are inlined at this point");

            if (FGA && FGA->getModule()) {
                FGA->replaceEntryFunc(pFunc, I->second);
            }
            // Now, after changing funciton signature,
            // and validate there are no calls to the old function we can erase it.
            pFunc->eraseFromParent();
        }

        DumpLLVMIR(m_context, "push_analysis");

        return retValue;

    }
    char PushAnalysis::ID = 0;

}//namespace IGC
