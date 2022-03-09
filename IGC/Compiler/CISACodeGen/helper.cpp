/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/CISACodeGen.h"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/KnownBits.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/Support/Alignment.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include <llvm/IR/InstIterator.h>
#include "llvm/Analysis/ValueTracking.h"
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "common/secure_mem.h"
#include <stack>
#include "Probe/Assertion.h"
#include "helper.h"

using namespace llvm;
using namespace GenISAIntrinsic;

/************************************************************************
This file contains helper functions for the code generator
Many functions use X-MACRO, that allow us to separate data about encoding
to the logic of the helper functions

************************************************************************/

namespace IGC
{
    typedef union _gfxResourceAddrSpace
    {
        struct _bits
        {
            unsigned int       bufId : 16;
            unsigned int       bufType : 5;
            unsigned int       indirect : 1;     // bool
            unsigned int       reserved : 10;
        } bits;
        uint32_t u32Val;
    } GFXResourceAddrSpace;

    // If 'bufIdx' is a ConstantInt, 'uniqueIndAS' is irrelevant.
    // Otherwise, you should set 'uniqueIndAS' if you want to identify
    // this address space later on.  If not, the default can be used.
    unsigned EncodeAS4GFXResource(
        const llvm::Value& bufIdx,
        BufferType bufType,
        unsigned uniqueIndAS)
    {
        GFXResourceAddrSpace temp;
        static_assert(sizeof(temp) == 4, "Code below may need and update.");
        temp.u32Val = 0;
        IGC_ASSERT((bufType + 1) < BUFFER_TYPE_UNKNOWN + 1);
        temp.bits.bufType = bufType + 1;

        if (bufType == SLM)
        {
            return ADDRESS_SPACE_LOCAL;
        }
        else if (bufType == STATELESS_READONLY)
        {
            return ADDRESS_SPACE_CONSTANT;
        }
        else if (bufType == STATELESS)
        {
            return ADDRESS_SPACE_GLOBAL;
        }
        else if (bufType == STATELESS_A32)
        {
            return ADDRESS_SPACE_THREAD_ARG;
        }
        else if (auto *CI = dyn_cast<ConstantInt>(&bufIdx))
        {
            unsigned int bufId = static_cast<unsigned>(CI->getZExtValue());
            IGC_ASSERT((bufType == BINDLESS_SAMPLER) || (bufId < (1 << 16)));
            temp.bits.bufId = bufId;
            return temp.u32Val;
        }

        // if it is indirect-buf, it is front-end's job to give a proper(unique) address-space per access
        temp.bits.bufId = uniqueIndAS;
        temp.bits.indirect = 1;
        return temp.u32Val;
    }

    ///
    /// if you want resource-dimension, use GetBufferDimension()
    ///
    BufferType DecodeAS4GFXResource(unsigned addrSpace, bool& directIndexing, unsigned& bufId)
    {
        GFXResourceAddrSpace temp;
        temp.u32Val = addrSpace;

        directIndexing = (temp.bits.indirect == 0);
        bufId = temp.bits.bufId;

        if (addrSpace == ADDRESS_SPACE_LOCAL)
        {
            return SLM;
        }
        else if (addrSpace == ADDRESS_SPACE_THREAD_ARG)
        {
            return STATELESS_A32;
        }
        unsigned bufType = temp.bits.bufType - 1;
        if (bufType < BUFFER_TYPE_UNKNOWN)
        {
            return (BufferType)bufType;
        }
        return BUFFER_TYPE_UNKNOWN;
    }

    // Return true if AS is for a stateful surface.
    //    Stateful surface should have an encoded AS that is bigger than
    //    ADDRESS_SPACE_NUM_ADDRESSES.
    bool isStatefulAddrSpace(unsigned AS)
    {
        return AS > ADDRESS_SPACE_NUM_ADDRESSES;
    }

    bool isDummyBasicBlock(llvm::BasicBlock* BB)
    {
        if (BB->size() != 1)
            return false;
        if ((++pred_begin(BB)) != pred_end(BB))
            return false;
        if ((++succ_begin(BB)) != succ_end(BB))
            return false;
        return true;
    }

    unsigned SetBufferAsBindless(unsigned addressSpaceOfPtr, BufferType bufferType)
    {
        GFXResourceAddrSpace temp = {};
        temp.u32Val = addressSpaceOfPtr;

        // Mark buffer as it is bindless for further processing
        switch (bufferType)
        {
        case BufferType::RESOURCE:
            temp.bits.bufType = IGC::BINDLESS_TEXTURE + 1;
            break;
        case BufferType::CONSTANT_BUFFER:
            temp.bits.bufType = IGC::BINDLESS_CONSTANT_BUFFER + 1;
            break;
        case BufferType::UAV:
            temp.bits.bufType = IGC::BINDLESS + 1;
            break;
        case BufferType::SAMPLER:
            temp.bits.bufType = IGC::BINDLESS_SAMPLER + 1;
            break;
        default:
            IGC_ASSERT_MESSAGE(0, "other types of buffers shouldn't reach this part");
            break;
        }
        return temp.u32Val;
    }

    bool UsesTypedConstantBuffer(
        const CodeGenContext* pContext,
        const BufferType bufType)
    {
        IGC_ASSERT(bufType == CONSTANT_BUFFER ||
            bufType == BINDLESS_CONSTANT_BUFFER);

        if (pContext->m_DriverInfo.ForceUntypedBindlessConstantBuffers() &&
            bufType == BINDLESS_CONSTANT_BUFFER)
        {
            return false;
        }

        if (pContext->m_DriverInfo.UsesTypedConstantBuffers3D() &&
            pContext->type != ShaderType::COMPUTE_SHADER)
        {
            return true;
        }
        if (pContext->m_DriverInfo.UsesTypedConstantBuffersGPGPU() &&
            pContext->type == ShaderType::COMPUTE_SHADER)
        {
            return true;
        }
        return false;
    }

    ///
    /// returns buffer type from addressspace
    ///
    BufferType DecodeBufferType(unsigned addrSpace)
    {
        switch (addrSpace)
        {
        case ADDRESS_SPACE_CONSTANT:
            return STATELESS_READONLY;
        case ADDRESS_SPACE_LOCAL:
            return SLM;
        case ADDRESS_SPACE_GLOBAL:
            return STATELESS;
        case ADDRESS_SPACE_THREAD_ARG:
            return STATELESS_A32;
        default:
            break;
        }
        GFXResourceAddrSpace temp;
        temp.u32Val = addrSpace;
        BufferType type = BUFFER_TYPE_UNKNOWN;
        if (addrSpace > ADDRESS_SPACE_NUM_ADDRESSES &&
            (temp.bits.bufType - 1) < BUFFER_TYPE_UNKNOWN)
        {
            type = static_cast<BufferType>(temp.bits.bufType - 1);
        }
        return type;
    }

    ///
    /// returns constant buffer load offset
    ///
    int getConstantBufferLoadOffset(llvm::LoadInst* ld)
    {
        int offset = 0;
        Value* ptr = ld->getPointerOperand();
        if (isa<ConstantPointerNull>(ptr))
        {
            offset = 0;
        }
        else if (IntToPtrInst * itop = dyn_cast<IntToPtrInst>(ptr))
        {
            ConstantInt* ci = dyn_cast<ConstantInt>(
                itop->getOperand(0));
            if (ci)
            {
                offset = int_cast<unsigned>(ci->getZExtValue());
            }
        }
        else if (ConstantExpr * itop = dyn_cast<ConstantExpr>(ptr))
        {
            if (itop->getOpcode() == Instruction::IntToPtr)
            {
                offset = int_cast<unsigned>(
                    cast<ConstantInt>(itop->getOperand(0))->getZExtValue());
            }
        }
        return offset;
    }
    ///
    /// returns info if direct addressing is used
    ///
    bool IsDirectIdx(unsigned addrSpace)
    {
        GFXResourceAddrSpace temp;
        temp.u32Val = addrSpace;
        return (temp.bits.indirect == 0);
    }

    bool isNaNCheck(llvm::FCmpInst& FC)
    {
        Value* Op1 = FC.getOperand(1);
        if (FC.getPredicate() == CmpInst::FCMP_UNO)
        {
            auto CFP = dyn_cast<ConstantFP>(Op1);
            return CFP && CFP->isZero();
        }
        else if (FC.getPredicate() == CmpInst::FCMP_UNE)
        {
            Value* Op0 = FC.getOperand(0);
            return Op0 == Op1;
        }
        return false;
    }

    llvm::LoadInst* cloneLoad(llvm::LoadInst* Orig, llvm::Value* Ptr)
    {
        llvm::LoadInst* LI = new llvm::LoadInst(
            cast<PointerType>(Ptr->getType())->getElementType(),
            Ptr, "", false, Orig);
        LI->setVolatile(Orig->isVolatile());
        LI->setAlignment(IGCLLVM::getCorrectAlign(Orig->getAlignment()));
        if (LI->isAtomic())
        {
            LI->setAtomic(Orig->getOrdering(), Orig->getSyncScopeID());
        }
        // Clone metadata
        llvm::SmallVector<std::pair<unsigned, llvm::MDNode*>, 4> MDs;
        Orig->getAllMetadata(MDs);
        for (llvm::SmallVectorImpl<std::pair<unsigned, llvm::MDNode*> >::iterator
            MI = MDs.begin(), ME = MDs.end(); MI != ME; ++MI)
        {
            LI->setMetadata(MI->first, MI->second);
        }
        return LI;
    }

    llvm::StoreInst* cloneStore(llvm::StoreInst* Orig, llvm::Value* Val, llvm::Value* Ptr)
    {
        llvm::StoreInst* SI = new llvm::StoreInst(Val, Ptr, Orig);
        SI->setVolatile(Orig->isVolatile());
        SI->setAlignment(IGCLLVM::getCorrectAlign(Orig->getAlignment()));
        if (SI->isAtomic())
        {
            SI->setAtomic(Orig->getOrdering(), Orig->getSyncScopeID());
        }
        // Clone metadata
        llvm::SmallVector<std::pair<unsigned, llvm::MDNode*>, 4> MDs;
        Orig->getAllMetadata(MDs);
        for (llvm::SmallVectorImpl<std::pair<unsigned, llvm::MDNode*> >::iterator
            MI = MDs.begin(), ME = MDs.end(); MI != ME; ++MI)
        {
            SI->setMetadata(MI->first, MI->second);
        }
        return SI;
    }

    // Create a ldraw from a load instruction
    LdRawIntrinsic* CreateLoadRawIntrinsic(LoadInst* inst, Value* bufPtr, Value* offsetVal)
    {
        Type* tys[] = {
            inst->getType(),
            bufPtr->getType()
        };

        auto* M = inst->getModule();
        auto& DL = M->getDataLayout();
        Function *func = GenISAIntrinsic::getDeclaration(
            M,
            inst->getType()->isVectorTy() ?
                GenISAIntrinsic::GenISA_ldrawvector_indexed :
                GenISAIntrinsic::GenISA_ldraw_indexed,
            tys);

        unsigned alignment = inst->getAlignment();
        if (alignment == 0)
            alignment = DL.getABITypeAlignment(inst->getType());

        IRBuilder<> builder(inst);

        Value* attr[] =
        {
            bufPtr,
            offsetVal,
            builder.getInt32(alignment),
            builder.getInt1(inst->isVolatile()) // volatile
        };
        auto* ld = builder.CreateCall(func, attr);
        IGC_ASSERT(ld->getType() == inst->getType());
        return cast<LdRawIntrinsic>(ld);
    }

    // Creates a storeraw from a store instruction
    StoreRawIntrinsic* CreateStoreRawIntrinsic(StoreInst* inst, Value* bufPtr, Value* offsetVal)
    {
        Module* module = inst->getModule();
        Function* func = nullptr;
        Value* storeVal = inst->getValueOperand();
        auto& DL = module->getDataLayout();
        if (storeVal->getType()->isVectorTy())
        {
            Type* tys[] = {
                bufPtr->getType(),
                storeVal->getType()
            };
            func = GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_storerawvector_indexed, tys);
        }
        else
        {
            Type* dataType = storeVal->getType();
            const uint64_t typeSize = DL.getTypeSizeInBits(dataType);
            IGC_ASSERT(typeSize == 8 || typeSize == 16 || typeSize == 32 || typeSize == 64);

            Type* types[] = {
                bufPtr->getType(),
                storeVal->getType()
            };

            func = GenISAIntrinsic::getDeclaration(module, llvm::GenISAIntrinsic::GenISA_storeraw_indexed, types);
        }
        IRBuilder<> builder(inst);
        unsigned alignment = inst->getAlignment();
        if (alignment == 0)
            alignment = DL.getABITypeAlignment(storeVal->getType());
        Value* attr[] =
        {
            bufPtr,
            offsetVal,
            storeVal,
            builder.getInt32(alignment),
            builder.getInt1(inst->isVolatile()) // volatile
        };
        auto* st = builder.CreateCall(func, attr);
        return cast<StoreRawIntrinsic>(st);
    }

    ///
    /// Tries to trace a resource pointer (texture/sampler/buffer) back to
    /// the pointer source. Also returns a vector of all instructions in the search path
    ///
    Value* TracePointerSource(Value* resourcePtr, bool hasBranching, bool enablePhiLoops, bool fillList,
        std::vector<Value*>& instList, llvm::SmallSet<PHINode*, 8> & visitedPHIs)
    {
        Value* srcPtr = nullptr;
        Value* baseValue = resourcePtr;

        // Returns true if resource pointers describe the same resource.
        auto ResourcePointersEq = [](Value* a, Value* b)->bool
        {
            if (a == b)
            {
                return true;
            }
            if (a->getType()->isPointerTy() && b->getType()->isPointerTy())
            {
                unsigned idxA = 0, idxB = 0;
                BufferType bufA, bufB;
                BufferAccessType accessA, accessB;
                bool needBufferOffsetA = false, needBufferOffsetB = false;

                if (GetResourcePointerInfo(a, idxA, bufA, accessA, needBufferOffsetA) &&
                    GetResourcePointerInfo(b, idxB, bufB, accessB, needBufferOffsetB) &&
                    idxA == idxB &&
                    accessA == accessB &&
                    needBufferOffsetA == needBufferOffsetB)
                {
                    return true;
                }
            }
            return false;
        };

        while (true)
        {
            if (fillList)
            {
                instList.push_back(baseValue);
            }

            if (GenIntrinsicInst * inst = dyn_cast<GenIntrinsicInst>(baseValue))
            {
                // For bindless pointers
                if ((inst->getIntrinsicID() == GenISAIntrinsic::GenISA_RuntimeValue) ||
                    (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_GlobalRootSignatureValue) ||
                    (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_GetBufferPtr))
                {
                    srcPtr = baseValue;
                }
                break;
            }
            else if (isa<Argument>(baseValue))
            {
                // For compute, resource comes from the kernel args
                srcPtr = baseValue;
                break;
            }
            else if (isa<GlobalVariable>(baseValue))
            {
                // Can be an inline sampler/constant buffer
                srcPtr = baseValue;
                break;
            }
            else if (auto allocaInst = dyn_cast<AllocaInst>(baseValue))
            {
                if (allocaInst->getMetadata("igc.read_only_array"))
                {
                    // Found alloca marked as read_only array.
                    srcPtr = baseValue;
                }
                break;
            }
            else if (CastInst * inst = dyn_cast<CastInst>(baseValue))
            {
                baseValue = inst->getOperand(0);
            }
            else if (GetElementPtrInst * inst = dyn_cast<GetElementPtrInst>(baseValue))
            {
                baseValue = inst->getOperand(0);
            }
            else if (BinaryOperator* inst = dyn_cast<BinaryOperator>(baseValue))
            {
                // Assume this is pointer arithmetic, which allows add/sub only.
                // Follow the first operand assuming it's pointer base.
                // Do not check the operand is pointer type now, leave the check
                // until the leaf instruction is found.
                Instruction::BinaryOps Opcode = inst->getOpcode();
                if (Opcode == Instruction::Add || Opcode == Instruction::Sub)
                {
                    baseValue = inst->getOperand(0);
                }
                else
                    break;
            }
            else if (PHINode * inst = dyn_cast<PHINode>(baseValue))
            {
                if (visitedPHIs.count(inst) != 0)
                {
                    // stop if we've seen this phi node before
                    return baseValue;
                }
                visitedPHIs.insert(inst);

                for (unsigned int i = 0; i < inst->getNumIncomingValues(); ++i)
                {
                    // All phi paths must be trace-able and trace back to the same source
                    Value* phiVal = inst->getIncomingValue(i);
                    std::vector<Value*> splitList;
                    Value* phiSrcPtr = TracePointerSource(phiVal, true, enablePhiLoops, fillList, splitList, visitedPHIs);
                    if (phiSrcPtr == nullptr)
                    {
                        // Incoming value not trace-able, bail out.
                        return nullptr;
                    }
                    else if (isa<PHINode>(phiSrcPtr) && phiSrcPtr == baseValue)
                    {
                        // Found a loop in one of the phi paths. We can still trace as long as all the other paths match
                        if (enablePhiLoops)
                            continue;
                        else
                            return nullptr;
                    }
                    else if (srcPtr == nullptr)
                    {
                        // Found a path to the source pointer. We only save the instructions used in this path
                        srcPtr = phiSrcPtr;
                        instList.insert(instList.end(), splitList.begin(), splitList.end());
                    }
                    else if (!ResourcePointersEq(srcPtr , phiSrcPtr))
                    {
                        // The source pointers have diverged. Bail out.
                        return nullptr;
                    }
                }
                break;
            }
            else if (SelectInst * inst = dyn_cast<SelectInst>(baseValue))
            {
                if (hasBranching)
                {
                    // only allow a single branching instruction to be supported for now
                    // if both select and PHI are present, or there are multiples of each, we bail
                    break;
                }
                // Trace both operands of the select instruction. Both have to be traced back to the same
                // source pointer, otherwise we can't determine which one to use.
                Value* selectSrc0 = TracePointerSource(inst->getOperand(1), true, enablePhiLoops, fillList, instList, visitedPHIs);
                Value* selectSrc1 = TracePointerSource(inst->getOperand(2), true, enablePhiLoops, false, instList, visitedPHIs);
                if (selectSrc0 && selectSrc1 &&
                    ResourcePointersEq(selectSrc0, selectSrc1))
                {
                    srcPtr = selectSrc0;
                    break;
                }
                return nullptr;
            }
            else if (LoadInst* inst = dyn_cast<LoadInst>(baseValue))
            {
                if (inst->getPointerAddressSpace() == 0)
                {
                    // May be local array of resources:
                    baseValue = inst->getPointerOperand();
                }
                else
                {
                    break;
                }
            }
            else
            {
                // Unsupported instruction in search chain. Don't continue.
                break;
            }
        }
        return srcPtr;
    }

    ///
    /// Only trace the GetBufferPtr instruction (ignore GetElementPtr)
    ///
    Value* TracePointerSource(Value* resourcePtr)
    {
        std::vector<Value*> tempList; //unused
        llvm::SmallSet<PHINode*, 8> visitedPHIs;
        return TracePointerSource(resourcePtr, false, true, false, tempList, visitedPHIs);
    }

    Value* TracePointerSource(Value* resourcePtr, bool hasBranching, bool enablePhiLoops, bool fillList, std::vector<Value*>& instList)
    {
        llvm::SmallSet<PHINode*, 8> visitedPHIs;
        return TracePointerSource(resourcePtr, hasBranching, enablePhiLoops, fillList, instList, visitedPHIs);
    }

    BufferAccessType getDefaultAccessType(BufferType bufTy)
    {
        switch (bufTy)
        {
        case BufferType::CONSTANT_BUFFER:
        case BufferType::RESOURCE:
        case BufferType::BINDLESS_TEXTURE:
        case BufferType::BINDLESS_CONSTANT_BUFFER:
        case BufferType::STATELESS_READONLY:
        case BufferType::SAMPLER:
            return BufferAccessType::ACCESS_READ;

        case BufferType::UAV:
        case BufferType::SLM:
        case BufferType::POINTER:
        case BufferType::BINDLESS:
        case BufferType::STATELESS:
            return BufferAccessType::ACCESS_READWRITE;

        case BufferType::RENDER_TARGET:
            return BufferAccessType::ACCESS_WRITE;

        default:
            IGC_ASSERT_MESSAGE(0, "Invalid buffer type");
            return BufferAccessType::ACCESS_READWRITE;
        }
    }

    bool GetResourcePointerInfo(Value* srcPtr, unsigned& resID, IGC::BufferType& resTy, BufferAccessType& accessTy, bool& needBufferOffset)
    {
        accessTy = BufferAccessType::ACCESS_READWRITE;
        needBufferOffset = false;
        if (GenIntrinsicInst * inst = dyn_cast<GenIntrinsicInst>(srcPtr))
        {
            // For bindless pointers with encoded metadata
            if (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_RuntimeValue)
            {
                if (inst->hasOperandBundles())
                {
                    auto resIDBundle = inst->getOperandBundle("resID");
                    auto resTyBundle = inst->getOperandBundle("resTy");
                    auto accessTyBundle = inst->getOperandBundle("accessTy");
                    auto needBufferOffsetBundle = inst->getOperandBundle("needBufferOffset");
                    if (resIDBundle && resTyBundle)
                    {
                        resID = (unsigned)(cast<ConstantInt>(resIDBundle->Inputs.front()))->getZExtValue();
                        resTy = (BufferType)(cast<ConstantInt>(resTyBundle->Inputs.front()))->getZExtValue();

                        if (accessTyBundle)
                            accessTy = (BufferAccessType)(cast<ConstantInt>(accessTyBundle->Inputs.front()))->getZExtValue();
                        else
                            accessTy = getDefaultAccessType(resTy);

                        if(needBufferOffsetBundle)
                            needBufferOffset = (bool)(cast<ConstantInt>(needBufferOffsetBundle->Inputs.front()))->getZExtValue();

                        return true;
                    }
                }
            }
            // For GetBufferPtr instructions with buffer info in the operands
            else if (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_GetBufferPtr)
            {
                Value* bufIdV = inst->getOperand(0);
                Value* bufTyV = inst->getOperand(1);
                if (isa<ConstantInt>(bufIdV) && isa<ConstantInt>(bufTyV))
                {
                    resID = (unsigned)(cast<ConstantInt>(bufIdV)->getZExtValue());
                    resTy = (IGC::BufferType)(cast<ConstantInt>(bufTyV)->getZExtValue());
                    accessTy = getDefaultAccessType(resTy);
                    return true;
                }
            }
        }
        return false;
    }

    // Get GRF offset from GenISA_RuntimeValue intrinsic call
    bool GetGRFOffsetFromRTV(Value* pointerSrc, unsigned& GRFOffset)
    {
        if (GenIntrinsicInst * inst = dyn_cast<GenIntrinsicInst>(pointerSrc))
        {
            // For bindless pointers with encoded metadata
            if (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_RuntimeValue)
            {
                GRFOffset = (unsigned)llvm::cast<llvm::ConstantInt>(inst->getOperand(0))->getZExtValue();
                return true;
            }
        }
        return false;
    }

    bool GetGRFOffsetFromGlobalRootSignatureValue(
        Value* pointerSrc, unsigned& GRFOffset)
    {
        if (auto* inst = dyn_cast<GenIntrinsicInst>(pointerSrc))
        {
            if (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_GlobalRootSignatureValue)
            {
                GRFOffset = (unsigned)cast<ConstantInt>(inst->getOperand(0))->getZExtValue();
                bool isConstant =
                    cast<ConstantInt>(inst->getOperand(1))->getZExtValue();
                return isConstant;
            }
        }
        return false;
    }

    bool GetStatelessBufferInfo(Value* pointer, unsigned& bufIdOrGRFOffset,
            BufferType & bufferTy, Value*& bufferSrcPtr, bool& isDirectBuf)
    {
        isDirectBuf = false;
        // If the buffer info is not encoded in the address space, we can still find it by
        // tracing the pointer to where it's created.
        Value * src = IGC::TracePointerSource(pointer);
        BufferAccessType accType;
        bool needBufferOffset;  // Unused
        if (!src)   return false;
        if (IGC::GetResourcePointerInfo(src, bufIdOrGRFOffset, bufferTy, accType, needBufferOffset))
        {
            bufferSrcPtr = src;
            isDirectBuf = true;
            return true;
        }
        else if (GetGRFOffsetFromRTV(src, bufIdOrGRFOffset))
        {
            bufferSrcPtr = src;
            bufferTy = BUFFER_TYPE_UNKNOWN;
            return true;
        }
        else if (GetGRFOffsetFromGlobalRootSignatureValue(src, bufIdOrGRFOffset))
        {
            bufferSrcPtr = src;
            bufferTy = CONSTANT_BUFFER;
            isDirectBuf = true;
            return true;
        }

        return false;
    }

    bool EvalConstantAddress(Value* address, unsigned int& offset, const llvm::DataLayout* pDL, Value* ptrSrc)
    {

        if ((ptrSrc == nullptr && isa<ConstantPointerNull>(address)) ||
            (ptrSrc == address))
        {
            offset = 0;
            return true;
        }
        else if(ConstantInt* eltIdx = dyn_cast<ConstantInt>(address))
        {
            offset = int_cast<int>(eltIdx->getZExtValue());
            return true;
        }
        else if (ConstantExpr * ptrExpr = dyn_cast<ConstantExpr>(address))
        {
            if (ptrExpr->getOpcode() == Instruction::IntToPtr)
            {
                Value* eltIdxVal = ptrExpr->getOperand(0);
                ConstantInt* eltIdx = dyn_cast<ConstantInt>(eltIdxVal);
                if (!eltIdx)
                    return false;
                offset = int_cast<int>(eltIdx->getZExtValue());
                return true;
            }
        }
        else if (Instruction* ptrExpr = dyn_cast<Instruction>(address))
        {
            if (ptrExpr->getOpcode() == Instruction::BitCast ||
                ptrExpr->getOpcode() == Instruction::AddrSpaceCast)
            {
                return EvalConstantAddress(ptrExpr->getOperand(0), offset, pDL, ptrSrc);
            }
            if (ptrExpr->getOpcode() == Instruction::IntToPtr)
            {
                Value * eltIdxVal = ptrExpr->getOperand(0);
                ConstantInt * eltIdx = dyn_cast<ConstantInt>(eltIdxVal);
                if (!eltIdx)
                    return false;
                offset = int_cast<int>(eltIdx->getZExtValue());
                return true;
            }
            if (ptrExpr->getOpcode() == Instruction::PtrToInt)
            {
                offset = 0;
                if (!EvalConstantAddress(ptrExpr->getOperand(0), offset, pDL, ptrSrc))
                {
                    return false;
                }
                return true;
            }
            else if (ptrExpr->getOpcode() == Instruction::GetElementPtr)
            {
                offset = 0;
                if (!EvalConstantAddress(ptrExpr->getOperand(0), offset, pDL, ptrSrc))
                {
                    return false;
                }
                Type * Ty = ptrExpr->getType();
                gep_type_iterator GTI = gep_type_begin(ptrExpr);
                for (auto OI = ptrExpr->op_begin() + 1, E = ptrExpr->op_end(); OI != E; ++OI, ++GTI) {
                    Value * Idx = *OI;
                    if (StructType * StTy = GTI.getStructTypeOrNull()) {
                        unsigned Field = int_cast<unsigned>(cast<ConstantInt>(Idx)->getZExtValue());
                        if (Field) {
                            offset += int_cast<int>(pDL->getStructLayout(StTy)->getElementOffset(Field));
                        }
                        Ty = StTy->getElementType(Field);
                    }
                    else {
                        Ty = GTI.getIndexedType();
                        if (const ConstantInt * CI = dyn_cast<ConstantInt>(Idx)) {
                            offset += int_cast<int>(
                            pDL->getTypeAllocSize(Ty) * CI->getSExtValue());

                        }
                        else
                        {
                            return false;
                        }
                    }
                }
                return true;
            }
        }
        return false;
    }

    // Get constant address from load/ldraw instruction
    bool getConstantAddress(llvm::Instruction& I, ConstantAddress& cl, CodeGenContext* pContext, bool& directBuf, bool& statelessBuf, bool& bindlessBuf)
    {
        // Check if the load instruction is with constant buffer address
        unsigned as;
        Value* ptrVal;
        Value* offsetVal;
        directBuf = false;
        statelessBuf = false;
        bindlessBuf = false;
        bool isPushableAddr = false;
        unsigned int& bufIdOrGRFOffset = cl.bufId;
        unsigned int& eltId = cl.eltId;
        unsigned int& size_in_bytes = cl.size;
        const llvm::DataLayout DL = pContext->getModule()->getDataLayout();

        // Only load and ldRaw instructions handled, rest should return
        if (LoadInst* load = llvm::dyn_cast<LoadInst> (&I))
        {
            as = load->getPointerAddressSpace();
            ptrVal = load->getPointerOperand();
            offsetVal = ptrVal;
            statelessBuf = (as == ADDRESS_SPACE_CONSTANT);
        }
        else if (LdRawIntrinsic* ldRaw = dyn_cast<LdRawIntrinsic>(&I))
        {
            as = ldRaw->getResourceValue()->getType()->getPointerAddressSpace();
            ptrVal = ldRaw->getResourceValue();
            offsetVal = ldRaw->getOffsetValue();
            bindlessBuf = (DecodeBufferType(as) == SSH_BINDLESS_CONSTANT_BUFFER);
        }
        else
            return false;

        size_in_bytes = 0;
        BufferType bufType;
        Value* pointerSrc = nullptr;

        if (statelessBuf || bindlessBuf)
        {
            // If the buffer info is not encoded in the address space, we can still find it by
            // tracing the pointer to where it's created.
            if (!GetStatelessBufferInfo(ptrVal, bufIdOrGRFOffset, bufType, pointerSrc, directBuf))
            {
                return false;
            }
            if (!directBuf)
            {
                // Make sure constant folding is safe by looking up in pushableAddresses
                PushInfo& pushInfo = pContext->getModuleMetaData()->pushInfo;

                for (auto it : pushInfo.pushableAddresses)
                {
                    if ((bufIdOrGRFOffset * 4 == it.addressOffset) && (IGC_IS_FLAG_ENABLED(DisableStaticCheckForConstantFolding) || it.isStatic))
                    {
                        isPushableAddr = true;
                        break;
                    }
                }
            }
        }
        else
        {
            bufType = IGC::DecodeAS4GFXResource(as, directBuf, bufIdOrGRFOffset);
        }
        // If it is statelessBuf, we made sure it is a constant buffer by finding it in pushableAddresses
        if ((directBuf && (bufType == CONSTANT_BUFFER)) ||
            (isPushableAddr && (statelessBuf || bindlessBuf)))
        {
            eltId = 0;
            if (!EvalConstantAddress(offsetVal, eltId, &DL, pointerSrc))
            {
                return false;
            }
        }
        else
        {
            return false;
        }
        size_in_bytes = (unsigned int)I.getType()->getPrimitiveSizeInBits() / 8;
        return true;
    }


    ///
    /// Replaces oldPtr with newPtr in a sample/ld intrinsic's argument list. The new instrinsic will
    /// replace the old one in the module
    ///
    void ChangePtrTypeInIntrinsic(llvm::GenIntrinsicInst*& pIntr, llvm::Value* oldPtr, llvm::Value* newPtr)
    {
        llvm::Module* pModule = pIntr->getParent()->getParent()->getParent();
        llvm::Function* pCalledFunc = pIntr->getCalledFunction();

        // Look at the intrinsic and figure out which pointer to change
        int num_ops = pIntr->getNumArgOperands();
        llvm::SmallVector<llvm::Value*, 5> args;

        for (int i = 0; i < num_ops; ++i)
        {
            if (pIntr->getArgOperand(i) == oldPtr)
                args.push_back(newPtr);
            else
                args.push_back(pIntr->getArgOperand(i));
        }

        llvm::Function* pNewIntr = nullptr;
        llvm::SmallVector<llvm::Type*, 4> overloadedTys;
        GenISAIntrinsic::ID id = pIntr->getIntrinsicID();
        switch (id)
        {
        case llvm::GenISAIntrinsic::GenISA_ldmcsptr:
        {
            llvm::Value* pTextureValue = cast<SamplerLoadIntrinsic>(pIntr)->getTextureValue();
            overloadedTys.push_back(pCalledFunc->getReturnType());
            overloadedTys.push_back(args[0]->getType());
            overloadedTys.push_back(pTextureValue == oldPtr ? newPtr->getType() : pTextureValue->getType());
            break;
        }
        case llvm::GenISAIntrinsic::GenISA_ldptr:
        case llvm::GenISAIntrinsic::GenISA_ldmsptr:
        {
            llvm::Value* pTextureValue = cast<SamplerLoadIntrinsic>(pIntr)->getTextureValue();
            overloadedTys.push_back(pCalledFunc->getReturnType());
            overloadedTys.push_back(pTextureValue == oldPtr ? newPtr->getType() : pTextureValue->getType());
            break;
        }
        case llvm::GenISAIntrinsic::GenISA_resinfoptr:
        case llvm::GenISAIntrinsic::GenISA_readsurfaceinfoptr:
        case llvm::GenISAIntrinsic::GenISA_sampleinfoptr:
            overloadedTys.push_back(newPtr->getType());
            break;
        case llvm::GenISAIntrinsic::GenISA_sampleptr:
        case llvm::GenISAIntrinsic::GenISA_sampleBptr:
        case llvm::GenISAIntrinsic::GenISA_sampleCptr:
        case llvm::GenISAIntrinsic::GenISA_sampleDptr:
        case llvm::GenISAIntrinsic::GenISA_sampleLptr:
        case llvm::GenISAIntrinsic::GenISA_sampleBCptr:
        case llvm::GenISAIntrinsic::GenISA_sampleDCptr:
        case llvm::GenISAIntrinsic::GenISA_sampleLCptr:
        case llvm::GenISAIntrinsic::GenISA_gather4ptr:
        case llvm::GenISAIntrinsic::GenISA_gather4POptr:
        case llvm::GenISAIntrinsic::GenISA_gather4Cptr:
        case llvm::GenISAIntrinsic::GenISA_gather4POCptr:
        case llvm::GenISAIntrinsic::GenISA_lodptr:
        {
            // Figure out the intrinsic operands for texture & sampler
            llvm::Value* pTextureValue = nullptr;
            llvm::Value* pSamplerValue = nullptr;
            IGC::getTextureAndSamplerOperands(
                pIntr,
                pTextureValue,
                pSamplerValue);

            overloadedTys.push_back(pCalledFunc->getReturnType());
            overloadedTys.push_back(pIntr->getOperand(0)->getType());
            overloadedTys.push_back(pTextureValue == oldPtr ? newPtr->getType() : pTextureValue->getType());
            if (pSamplerValue != nullptr)
            {
                // Samplerless messages will not have sampler in signature.
                overloadedTys.push_back(pSamplerValue == oldPtr ? newPtr->getType() : pSamplerValue->getType());
            }
            break;
        }
        case llvm::GenISAIntrinsic::GenISA_typedread:
        case llvm::GenISAIntrinsic::GenISA_typedwrite:
        case llvm::GenISAIntrinsic::GenISA_ldstructured:
        case llvm::GenISAIntrinsic::GenISA_storestructured1:
        case llvm::GenISAIntrinsic::GenISA_storestructured2:
        case llvm::GenISAIntrinsic::GenISA_storestructured3:
        case llvm::GenISAIntrinsic::GenISA_storestructured4:
            overloadedTys.push_back(newPtr->getType());
            break;
        case llvm::GenISAIntrinsic::GenISA_intatomicraw:
        case llvm::GenISAIntrinsic::GenISA_icmpxchgatomicraw:
        case llvm::GenISAIntrinsic::GenISA_intatomicrawA64:
        case llvm::GenISAIntrinsic::GenISA_icmpxchgatomicrawA64:
        case llvm::GenISAIntrinsic::GenISA_floatatomicraw:
        case llvm::GenISAIntrinsic::GenISA_floatatomicrawA64:
        case llvm::GenISAIntrinsic::GenISA_fcmpxchgatomicraw:
        case llvm::GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64:
            overloadedTys.push_back(pIntr->getType());
            overloadedTys.push_back(newPtr->getType());
            if (id == GenISAIntrinsic::GenISA_intatomicrawA64)
            {
                args[0] = args[1];
                args[1] = CastInst::CreatePointerCast(args[1], Type::getInt32Ty(pModule->getContext()), "", pIntr);
                id = GenISAIntrinsic::GenISA_intatomicraw;
            }
            else if (id == GenISAIntrinsic::GenISA_icmpxchgatomicrawA64)
            {
                args[0] = args[1];
                args[1] = CastInst::CreatePointerCast(args[1], Type::getInt32Ty(pModule->getContext()), "", pIntr);
                id = GenISAIntrinsic::GenISA_icmpxchgatomicraw;
            }
            else if (id == GenISAIntrinsic::GenISA_floatatomicrawA64)
            {
                args[0] = args[1];
                args[1] = CastInst::CreatePointerCast(args[1], Type::getFloatTy(pModule->getContext()), "", pIntr);
                id = GenISAIntrinsic::GenISA_floatatomicraw;
            }
            else if (id == GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64)
            {
                args[0] = args[1];
                args[1] = CastInst::CreatePointerCast(args[1], Type::getFloatTy(pModule->getContext()), "", pIntr);
                id = GenISAIntrinsic::GenISA_fcmpxchgatomicraw;
            }
            break;
        case llvm::GenISAIntrinsic::GenISA_dwordatomicstructured:
        case llvm::GenISAIntrinsic::GenISA_floatatomicstructured:
        case llvm::GenISAIntrinsic::GenISA_cmpxchgatomicstructured:
        case llvm::GenISAIntrinsic::GenISA_fcmpxchgatomicstructured:
            overloadedTys.push_back(pIntr->getType());
            overloadedTys.push_back(args[0]->getType());
            break;
        case GenISAIntrinsic::GenISA_intatomictyped:
        case GenISAIntrinsic::GenISA_icmpxchgatomictyped:
            overloadedTys.push_back(pIntr->getType());
            overloadedTys.push_back(newPtr->getType());
            break;
        case GenISAIntrinsic::GenISA_atomiccounterinc:
        case GenISAIntrinsic::GenISA_atomiccounterpredec:
            overloadedTys.push_back(args[0]->getType());
            break;
        case llvm::GenISAIntrinsic::GenISA_ldrawvector_indexed:
        case llvm::GenISAIntrinsic::GenISA_ldraw_indexed:
            overloadedTys.push_back(pCalledFunc->getReturnType());
            overloadedTys.push_back(newPtr->getType());
            break;
        case llvm::GenISAIntrinsic::GenISA_storerawvector_indexed:
        case llvm::GenISAIntrinsic::GenISA_storeraw_indexed:
            overloadedTys.push_back(newPtr->getType());
            overloadedTys.push_back(args[2]->getType());
            break;
        default:
            IGC_ASSERT_MESSAGE(0, "Unknown intrinsic encountered while changing pointer types");
            break;
        }

        pNewIntr = llvm::GenISAIntrinsic::getDeclaration(
            pModule,
            id,
            overloadedTys);

        llvm::CallInst* pNewCall = llvm::CallInst::Create(pNewIntr, args, "", pIntr);
        pNewCall->setDebugLoc(pIntr->getDebugLoc());

        pIntr->replaceAllUsesWith(pNewCall);
        pIntr->eraseFromParent();

        pIntr = llvm::cast<llvm::GenIntrinsicInst>(pNewCall);
    }

    ///
    /// Returns the sampler/texture pointers for resource access intrinsics
    ///
    void getTextureAndSamplerOperands(
        llvm::GenIntrinsicInst* pIntr,
        llvm::Value*& pTextureValue,
        llvm::Value*& pSamplerValue)
    {
        if (llvm::SamplerLoadIntrinsic * pSamplerLoadInst = llvm::dyn_cast<llvm::SamplerLoadIntrinsic>(pIntr))
        {
            pTextureValue = pSamplerLoadInst->getTextureValue();
            pSamplerValue = nullptr;
        }
        else if (llvm::SampleIntrinsic * pSampleInst = llvm::dyn_cast<llvm::SampleIntrinsic>(pIntr))
        {
            pTextureValue = pSampleInst->getTextureValue();
            pSamplerValue = pSampleInst->getSamplerValue();
        }
        else if (llvm::SamplerGatherIntrinsic * pGatherInst = llvm::dyn_cast<llvm::SamplerGatherIntrinsic>(pIntr))
        {
            pTextureValue = pGatherInst->getTextureValue();
            pSamplerValue = pGatherInst->getSamplerValue();
        }
        else
        {
            pTextureValue = nullptr;
            pSamplerValue = nullptr;
            switch (pIntr->getIntrinsicID())
            {
            case llvm::GenISAIntrinsic::GenISA_resinfoptr:
            case llvm::GenISAIntrinsic::GenISA_readsurfaceinfoptr:
            case llvm::GenISAIntrinsic::GenISA_sampleinfoptr:
            case llvm::GenISAIntrinsic::GenISA_typedwrite:
            case llvm::GenISAIntrinsic::GenISA_typedread:
                pTextureValue = pIntr->getOperand(0);
                break;
            default:
                break;
            }
        }
    }

    // Get the buffer pointer operand for supported buffer access instructions
    Value* GetBufferOperand(Instruction* inst)
    {
        Value* pBuffer = nullptr;
        if (LoadInst * load = dyn_cast<LoadInst>(inst))
        {
            pBuffer = load->getPointerOperand();
        }
        else if (StoreInst * store = dyn_cast<StoreInst>(inst))
        {
            pBuffer = store->getPointerOperand();
        }
        else if (GenIntrinsicInst * intr = dyn_cast<GenIntrinsicInst>(inst))
        {
            switch (intr->getIntrinsicID())
            {
            case GenISAIntrinsic::GenISA_storerawvector_indexed:
            case GenISAIntrinsic::GenISA_ldrawvector_indexed:
            case GenISAIntrinsic::GenISA_storeraw_indexed:
            case GenISAIntrinsic::GenISA_ldraw_indexed:
            case GenISAIntrinsic::GenISA_intatomicraw:
            case GenISAIntrinsic::GenISA_intatomictyped:
            case GenISAIntrinsic::GenISA_icmpxchgatomictyped:
            case GenISAIntrinsic::GenISA_floatatomicraw:
            case GenISAIntrinsic::GenISA_icmpxchgatomicraw:
            case GenISAIntrinsic::GenISA_fcmpxchgatomicraw:
            case GenISAIntrinsic::GenISA_simdBlockRead:
            case GenISAIntrinsic::GenISA_simdBlockWrite:
                pBuffer = intr->getOperand(0);
                break;
            case GenISAIntrinsic::GenISA_intatomicrawA64:
            case GenISAIntrinsic::GenISA_floatatomicrawA64:
            case GenISAIntrinsic::GenISA_icmpxchgatomicrawA64:
            case GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64:
                pBuffer = intr->getOperand(1);
                break;
            default:
                break;
            }
        }
        return pBuffer;
    }

    EOPCODE GetOpCode(const llvm::Instruction* inst)
    {
        if (const GenIntrinsicInst * CI = dyn_cast<GenIntrinsicInst>(inst))
        {
            unsigned ID = CI->getIntrinsicID();
            return (EOPCODE)(OPCODE(ID, e_Intrinsic));
        }
        else if (const IntrinsicInst * CI = llvm::dyn_cast<llvm::IntrinsicInst>(inst))
        {
            unsigned ID = CI->getIntrinsicID();
            return (EOPCODE)(OPCODE(ID, e_Intrinsic));
        }
        return (EOPCODE)(OPCODE(inst->getOpcode(), e_Instruction));
    }

    BufferType GetBufferType(uint addrSpace)
    {
        bool directIndexing = false;
        unsigned int bufId = 0;
        return DecodeAS4GFXResource(addrSpace, directIndexing, bufId);
    }

    bool IsReadOnlyLoadDirectCB(llvm::Instruction* pLLVMInst,
        uint& cbId, llvm::Value*& eltPtrVal, BufferType& bufType)
    {
        LoadInst* inst = dyn_cast<LoadInst>(pLLVMInst);
        if (!inst)
        {
            return false;
        }
        bool isInvLoad = inst->getMetadata(LLVMContext::MD_invariant_load) != nullptr;
        unsigned as = inst->getPointerAddressSpace();
        bool directBuf;
        // cbId gets filled in the following call;
        bufType = IGC::DecodeAS4GFXResource(as, directBuf, cbId);
        if ((bufType == CONSTANT_BUFFER || bufType == RESOURCE || isInvLoad) && directBuf)
        {
            Value* ptrVal = inst->getPointerOperand();
            // skip bitcast and find the real address computation
            while (isa<BitCastInst>(ptrVal))
            {
                ptrVal = cast<BitCastInst>(ptrVal)->getOperand(0);
            }
            if (isa<ConstantPointerNull>(ptrVal) ||
                isa<IntToPtrInst>(ptrVal) ||
                isa<GetElementPtrInst>(ptrVal) ||
                isa<ConstantExpr>(ptrVal) ||
                isa<LoadInst>(ptrVal) ||
                isa<Argument>(ptrVal))
            {
                eltPtrVal = ptrVal;
                return true;
            }
        }
        return false;
    }

    bool IsLoadFromDirectCB(llvm::Instruction* pLLVMInst, uint& cbId, llvm::Value*& eltPtrVal)
    {
        BufferType bufType = BUFFER_TYPE_UNKNOWN;
        bool isReadOnly = IsReadOnlyLoadDirectCB(pLLVMInst, cbId, eltPtrVal, bufType);
        return isReadOnly && bufType == CONSTANT_BUFFER;
    }

    /// this is texture-load not buffer-load
    bool isLdInstruction(const llvm::Instruction* inst)
    {
        return isa<SamplerLoadIntrinsic>(inst);
    }

    // function returns the position of the texture operand for sample/ld instructions
    llvm::Value* getTextureIndexArgBasedOnOpcode(llvm::Instruction* inst)
    {
        if (isLdInstruction(inst))
        {
            return cast<SamplerLoadIntrinsic>(inst)->getTextureValue();
        }
        else if (isSampleInstruction(inst))
        {
            return cast<SampleIntrinsic>(inst)->getTextureValue();
        }
        else if (isGather4Instruction(inst))
        {
            return cast<SamplerGatherIntrinsic>(inst)->getTextureValue();
        }

        return nullptr;
    }

    int findSampleInstructionTextureIdx(llvm::Instruction* inst)
    {
        // fetch the textureArgIdx.
        Value* ptr = getTextureIndexArgBasedOnOpcode(inst);
        unsigned textureIdx = -1;

        if (ptr && ptr->getType()->isPointerTy())
        {
            BufferType bufType = BUFFER_TYPE_UNKNOWN;
            if (!(isa<GenIntrinsicInst>(ptr) &&
                cast<GenIntrinsicInst>(ptr)->getIntrinsicID() == GenISAIntrinsic::GenISA_GetBufferPtr))
            {
                uint as = ptr->getType()->getPointerAddressSpace();
                bool directIndexing;
                bufType = DecodeAS4GFXResource(as, directIndexing, textureIdx);
                if (bufType == UAV)
                {
                    // dont do any clustering on read/write images
                    textureIdx = -1;
                }
            }
        }
        else if (ptr)
        {
            if (llvm::dyn_cast<llvm::ConstantInt>(ptr))
            {
                textureIdx = int_cast<unsigned>(GetImmediateVal(ptr));
            }
        }

        return textureIdx;
    }

    bool isSampleLoadGather4InfoInstruction(const llvm::Instruction* inst)
    {
        if (isa<GenIntrinsicInst>(inst))
        {
            switch ((cast<GenIntrinsicInst>(inst))->getIntrinsicID())
            {
            case GenISAIntrinsic::GenISA_sampleptr:
            case GenISAIntrinsic::GenISA_sampleBptr:
            case GenISAIntrinsic::GenISA_sampleCptr:
            case GenISAIntrinsic::GenISA_sampleDptr:
            case GenISAIntrinsic::GenISA_sampleDCptr:
            case GenISAIntrinsic::GenISA_sampleLptr:
            case GenISAIntrinsic::GenISA_sampleLCptr:
            case GenISAIntrinsic::GenISA_sampleBCptr:
            case GenISAIntrinsic::GenISA_lodptr:
            case GenISAIntrinsic::GenISA_ldptr:
            case GenISAIntrinsic::GenISA_ldmsptr:
            case GenISAIntrinsic::GenISA_ldmsptr16bit:
            case GenISAIntrinsic::GenISA_ldmcsptr:
            case GenISAIntrinsic::GenISA_sampleinfoptr:
            case GenISAIntrinsic::GenISA_resinfoptr:
            case GenISAIntrinsic::GenISA_gather4ptr:
            case GenISAIntrinsic::GenISA_gather4Cptr:
            case GenISAIntrinsic::GenISA_gather4POptr:
            case GenISAIntrinsic::GenISA_gather4POCptr:
                return true;
            default:
                return false;
            }
        }

        return false;
    }

    bool isSampleInstruction(const llvm::Instruction* inst)
    {
        return isa<SampleIntrinsic>(inst);
    }

    bool isInfoInstruction(const llvm::Instruction* inst)
    {
        return isa<InfoIntrinsic>(inst);
    }

    bool isGather4Instruction(const llvm::Instruction* inst)
    {
        return isa<SamplerGatherIntrinsic>(inst);
    }

    bool IsMediaIOIntrinsic(const llvm::Instruction* inst)
    {
        if (auto * pGI = dyn_cast<llvm::GenIntrinsicInst>(inst))
        {
            GenISAIntrinsic::ID id = pGI->getIntrinsicID();

            return id == GenISAIntrinsic::GenISA_MediaBlockRead ||
                id == GenISAIntrinsic::GenISA_MediaBlockWrite;
        }

        return false;
    }

    bool IsSIMDBlockIntrinsic(const llvm::Instruction* inst)
    {
        if (auto * pGI = dyn_cast<llvm::GenIntrinsicInst>(inst))
        {
            GenISAIntrinsic::ID id = pGI->getIntrinsicID();

            return id == GenISAIntrinsic::GenISA_simdBlockRead ||
                id == GenISAIntrinsic::GenISA_simdBlockWrite;
        }

        return false;
    }

    bool isSubGroupIntrinsic(const llvm::Instruction* I)
    {
        const GenIntrinsicInst* GII = dyn_cast<GenIntrinsicInst>(I);
        if (!GII)
            return false;

        switch (GII->getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_WaveShuffleIndex:
        case GenISAIntrinsic::GenISA_simdShuffleDown:
        case GenISAIntrinsic::GenISA_simdBlockRead:
        case GenISAIntrinsic::GenISA_simdBlockWrite:
        case GenISAIntrinsic::GenISA_simdMediaBlockRead:
        case GenISAIntrinsic::GenISA_simdMediaBlockWrite:
        case GenISAIntrinsic::GenISA_MediaBlockWrite:
        case GenISAIntrinsic::GenISA_MediaBlockRead:
            return true;
        default:
            return false;
        }
    }

    bool isSubGroupIntrinsicPVC(const llvm::Instruction* I)
    {
        const GenIntrinsicInst* GII = dyn_cast<GenIntrinsicInst>(I);
        if (!GII)
            return false;

        switch (GII->getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_simdMediaBlockRead:
        case GenISAIntrinsic::GenISA_simdMediaBlockWrite:
            return true;
        default:
            return false;
        }
    }

    bool hasSubGroupIntrinsicPVC(llvm::Function& F)
    {
        for (auto& BB : F)
        {
            for (auto& I : BB)
            {
                if (isSubGroupIntrinsicPVC(&I))
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool isURBWriteIntrinsic(const llvm::Instruction* I)
    {
        const GenIntrinsicInst* GII = dyn_cast<GenIntrinsicInst>(I);
        if (!GII)
            return false;

        return GII->getIntrinsicID() == GenISA_URBWrite;

    }

    llvm::Instruction* AdjustSystemValueCall(llvm::GenIntrinsicInst* inst)
    {
        IGC_ASSERT(inst->getIntrinsicID() == GenISAIntrinsic::GenISA_DCL_SystemValue);
        llvm::Module* pModule = inst->getParent()->getParent()->getParent();
        auto CommonConvertFunc = [pModule](llvm::GenIntrinsicInst* inst, llvm::Type* outputType)
        {
            IGC_ASSERT(outputType->isVectorTy() == false);
            IGC_ASSERT(inst->getType()->isVectorTy() == false);
            llvm::Instruction* result = inst;
            if (inst->getType() != outputType)
            {
                llvm::IRBuilder<> builder(inst);
                llvm::Function* systemValueFunc = llvm::GenISAIntrinsic::getDeclaration(pModule, GenISAIntrinsic::GenISA_DCL_SystemValue, outputType);
                llvm::Instruction* sgv = builder.CreateCall(systemValueFunc, inst->getOperand(0));
                // a default system value intrinsic function returns a float value. The returned value is bit casted to an appropriate integer or floating point value
                // in reference to HW specification. Casting from floating point to integer and in the opposite direction is not expected.
                sgv = llvm::cast<llvm::Instruction>(builder.CreateZExtOrTrunc(sgv, builder.getIntNTy((unsigned int)inst->getType()->getPrimitiveSizeInBits())));
                sgv = llvm::cast<llvm::Instruction>(builder.CreateBitCast(sgv, inst->getType()));
                inst->replaceAllUsesWith(sgv);
                inst->eraseFromParent();
                result = sgv;
            }
            return result;
        };

        SGVUsage usage = static_cast<SGVUsage>(llvm::cast<llvm::ConstantInt>(inst->getOperand(0))->getZExtValue());
        llvm::Instruction* result = inst;

        switch (usage)
        {
        case THREAD_ID_IN_GROUP_X:
        case THREAD_ID_IN_GROUP_Y:
        case THREAD_ID_IN_GROUP_Z:
            result = CommonConvertFunc(inst, llvm::IntegerType::get(pModule->getContext(), 16));
            break;
        default:
            break;
        }
        return result;
    }

    bool isReadInput(llvm::Instruction* pLLVMInstr);

#define DECLARE_OPCODE(instName, llvmType, name, modifiers, sat, pred, condMod, mathIntrinsic, atomicIntrinsic, regioning) \
    case name:\
    return modifiers;
    bool SupportsModifier(llvm::Instruction* inst)
    {
        // Special cases
        switch (inst->getOpcode())
        {
        case Instruction::ICmp:
        {
            // icmp supports modifier unless it is unsigned
            CmpInst* cmp = cast<ICmpInst>(inst);
            return !cmp->isUnsigned();
        }
        case Instruction::Mul:
            // integer mul supports modifier if not int64.
            return !inst->getType()->isIntegerTy(64);
        case Instruction::URem:
            // neg mod is negative. Disable it as URem must have positive operands,
            return false;
        default:
            break;
        }

        switch (GetOpCode(inst))
        {
#include "opCode.h"
        default:
            return false;
        }
    }
#undef DECLARE_OPCODE

#define DECLARE_OPCODE(instName, llvmType, name, modifiers, sat, pred, condMod, mathIntrinsic, atomicIntrinsic, regioning) \
    case name:\
    return sat;
    bool SupportsSaturate(llvm::Instruction* inst)
    {
        switch (GetOpCode(inst))
        {
#include "opCode.h"
        default:
            break;
        }
        return false;
    }
#undef DECLARE_OPCODE

#define DECLARE_OPCODE(instName, llvmType, name, modifiers, sat, pred, condMod, mathIntrinsic, atomicIntrinsic, regioning) \
    case name:\
    return pred;
    bool SupportsPredicate(llvm::Instruction* inst)
    {
        switch (GetOpCode(inst))
        {
#include "opCode.h"
        default:
            return false;
        }
    }
#undef DECLARE_OPCODE

#define DECLARE_OPCODE(instName, llvmType, name, modifiers, sat, pred, condMod, mathIntrinsic, atomicIntrinsic, regioning) \
    case name:\
    return condMod;
    bool SupportsCondModifier(llvm::Instruction* inst)
    {
        switch (GetOpCode(inst))
        {
#include "opCode.h"
        default:
            return false;
        }
    }
#undef DECLARE_OPCODE

#define DECLARE_OPCODE(instName, llvmType, name, modifiers, sat, pred, condMod, mathIntrinsic, atomicIntrinsic, regioning) \
    case name:\
    return regioning;
    bool SupportsRegioning(llvm::Instruction* inst)
    {
        switch (GetOpCode(inst))
        {
#include "opCode.h"
        default:
            break;
        }
        return false;
    }
#undef DECLARE_OPCODE

#define DECLARE_OPCODE(instName, llvmType, name, modifiers, sat, pred, condMod, mathIntrinsic, atomicIntrinsic, regioning) \
    case name:\
    return mathIntrinsic;
    bool IsMathIntrinsic(EOPCODE opcode)
    {
        switch (opcode)
        {
#include "opCode.h"
        default:
            return false;
        }
    }
#undef DECLARE_OPCODE

#define DECLARE_OPCODE(instName, llvmType, name, modifiers, sat, pred, condMod, mathIntrinsic, atomicIntrinsic, regioning) \
    case name:\
    return atomicIntrinsic;
    bool IsAtomicIntrinsic(EOPCODE opcode)
    {
        switch (opcode)
        {
#include "opCode.h"
        default:
            return false;
        }
    }
#undef DECLARE_OPCODE

    bool IsExtendedMathInstruction(llvm::Instruction* Inst)
    {
        EOPCODE opcode = GetOpCode(Inst);
        switch (opcode)
        {
        case llvm_fdiv:
        case llvm_sdiv:
        case llvm_udiv:
        case llvm_log:
        case llvm_exp:
        case llvm_sqrt:
        case llvm_sin:
        case llvm_cos:
        case llvm_pow:
            return true;
        default:
            return false;
        }
        return false;
    }
    // for now just include shuffle, reduce and scan,
    // which have simd32 implementations and should not be split into two instances
    bool IsSubGroupIntrinsicWithSimd32Implementation(EOPCODE opcode)
    {
        return (opcode == llvm_waveAll ||
            opcode == llvm_waveClustered ||
            opcode == llvm_wavePrefix ||
            opcode == llvm_waveShuffleIndex ||
            opcode == llvm_simdShuffleDown ||
            opcode == llvm_simdBlockRead||
            opcode == llvm_simdBlockReadBindless);
    }


    bool IsGradientIntrinsic(EOPCODE opcode)
    {
        return(opcode == llvm_gradientX ||
            opcode == llvm_gradientY ||
            opcode == llvm_gradientXfine ||
            opcode == llvm_gradientYfine);
    }

    bool IsStatelessMemLoadIntrinsic(llvm::GenISAIntrinsic::ID id)
    {
        switch(id)
        {
        case GenISAIntrinsic::GenISA_simdBlockRead:
        case GenISAIntrinsic::GenISA_LSC2DBlockRead:
        case GenISAIntrinsic::GenISA_LSCLoad:
        case GenISAIntrinsic::GenISA_LSCLoadBlock:
        case GenISAIntrinsic::GenISA_LSCPrefetch:
                return true;
            default:
                break;
        }
        return false;
    }

    bool IsStatelessMemStoreIntrinsic(llvm::GenISAIntrinsic::ID id)
    {
        switch (id) {
        case GenISAIntrinsic::GenISA_HDCuncompressedwrite:
        case GenISAIntrinsic::GenISA_LSCStore:
        case GenISAIntrinsic::GenISA_LSCStoreBlock:
        case GenISAIntrinsic::GenISA_simdBlockWrite:
            return true;
        default:
            break;
        }
        return false;
    }

    bool IsStatelessMemAtomicIntrinsic(GenIntrinsicInst& inst, GenISAIntrinsic::ID id)
    {
        // This includes:
        // GenISA_intatomicraw
        // GenISA_floatatomicraw
        // GenISA_intatomicrawA64
        // GenISA_floatatomicrawA64
        // GenISA_icmpxchgatomicraw
        // GenISA_fcmpxchgatomicraw
        // GenISA_icmpxchgatomicrawA64
        // GenISA_fcmpxchgatomicrawA64
        if (IsAtomicIntrinsic(GetOpCode(&inst)))
            return true;

        switch (id)
        {
        case GenISAIntrinsic::GenISA_LSCAtomicFP32:
        case GenISAIntrinsic::GenISA_LSCAtomicFP64:
        case GenISAIntrinsic::GenISA_LSCAtomicInts:
            return true;
        default:
            break;
        }
        return false;
    }

    bool ComputesGradient(llvm::Instruction* inst)
    {
        llvm::SampleIntrinsic* sampleInst = dyn_cast<llvm::SampleIntrinsic>(inst);
        if (sampleInst && sampleInst->IsDerivative())
        {
            return true;
        }
        if (IsGradientIntrinsic(GetOpCode(inst)))
        {
            return true;
        }
        return false;
    }

    uint getImmValueU32(const llvm::Value* value)
    {
        const llvm::ConstantInt* cval = llvm::cast<llvm::ConstantInt>(value);
        IGC_ASSERT(nullptr != cval);
        IGC_ASSERT(cval->getBitWidth() == 32);

        uint ival = int_cast<uint>(cval->getZExtValue());
        return ival;
    }

    bool getImmValueBool(const llvm::Value* value)
    {
        const llvm::ConstantInt* cval = llvm::cast<llvm::ConstantInt>(value);
        IGC_ASSERT(nullptr != cval);
        IGC_ASSERT(cval->getBitWidth() == 1);

        return cval->getValue().getBoolValue();
    }

    llvm::Value* ExtractElementFromInsertChain(llvm::Value* inst, int pos)
    {

        llvm::ConstantDataVector* cstV = llvm::dyn_cast<llvm::ConstantDataVector>(inst);
        if (cstV != NULL) {
            return cstV->getElementAsConstant(pos);
        }

        llvm::InsertElementInst* ie = llvm::dyn_cast<llvm::InsertElementInst>(inst);
        while (ie != NULL) {
            int64_t iOffset = llvm::dyn_cast<llvm::ConstantInt>(ie->getOperand(2))->getSExtValue();
            IGC_ASSERT(iOffset >= 0);
            if (iOffset == pos) {
                return ie->getOperand(1);
            }
            llvm::Value* insertBase = ie->getOperand(0);
            ie = llvm::dyn_cast<llvm::InsertElementInst>(insertBase);
        }
        return NULL;
    }

    bool ExtractVec4FromInsertChain(llvm::Value* inst, llvm::Value* elem[4], llvm::SmallVector<llvm::Instruction*, 10> & instructionToRemove)
    {
        llvm::ConstantDataVector* cstV = llvm::dyn_cast<llvm::ConstantDataVector>(inst);
        if (cstV != NULL) {
            IGC_ASSERT(cstV->getNumElements() == 4);
            for (int i = 0; i < 4; i++) {
                elem[i] = cstV->getElementAsConstant(i);
            }
            return true;
        }

        for (int i = 0; i < 4; i++) {
            elem[i] = NULL;
        }

        int count = 0;
        llvm::InsertElementInst* ie = llvm::dyn_cast<llvm::InsertElementInst>(inst);
        while (ie != NULL) {
            int64_t iOffset = llvm::dyn_cast<llvm::ConstantInt>(ie->getOperand(2))->getSExtValue();
            IGC_ASSERT(iOffset >= 0);
            if (elem[iOffset] == NULL) {
                elem[iOffset] = ie->getOperand(1);
                count++;
                if (ie->hasOneUse()) {
                    instructionToRemove.push_back(ie);
                }
            }
            llvm::Value* insertBase = ie->getOperand(0);
            ie = llvm::dyn_cast<llvm::InsertElementInst>(insertBase);
        }
        return (count == 4);
    }

    void VectorToElement(llvm::Value* inst, llvm::Value* elem[], llvm::Type* int32Ty, llvm::Instruction* insert_before, int vsize)
    {
        for (int i = 0; i < vsize; i++) {
            if (elem[i] == nullptr) {
                // Create an ExtractElementInst
                elem[i] = llvm::ExtractElementInst::Create(inst, llvm::ConstantInt::get(int32Ty, i), "", insert_before);
            }
        }
    }

    llvm::Value* ElementToVector(llvm::Value* elem[], llvm::Type* int32Ty, llvm::Instruction* insert_before, int vsize)
    {
        llvm::VectorType* vt = IGCLLVM::FixedVectorType::get(elem[0]->getType(), vsize);
        llvm::Value* vecValue = llvm::UndefValue::get(vt);

        for (int i = 0; i < vsize; ++i)
        {

            vecValue = llvm::InsertElementInst::Create(vecValue, elem[i], llvm::ConstantInt::get(int32Ty, i), "", insert_before);
            ((Instruction*)vecValue)->setDebugLoc(insert_before->getDebugLoc());
        }
        return vecValue;
    }

    llvm::Value* ConvertToFloat(llvm::IRBuilder<>& builder, llvm::Value* val)
    {
        llvm::Value* ret = val;
        llvm::Type* type = val->getType();
        IGC_ASSERT(nullptr != type);
        IGC_ASSERT_MESSAGE(type->isSingleValueType(), "Only scalar data is supported here");
        IGC_ASSERT_MESSAGE(!type->isVectorTy(), "Only scalar data is supported here");
        IGC_ASSERT((type->getTypeID() == Type::FloatTyID) || (type->getTypeID() == Type::HalfTyID) || (type->getTypeID() == Type::IntegerTyID) || (type->getTypeID() == Type::DoubleTyID));

        unsigned dataSize = type->getScalarSizeInBits();
        if (16 == dataSize){
            ret = builder.CreateFPExt(builder.CreateBitCast(val, builder.getHalfTy()), builder.getFloatTy());
        }else if (32 == dataSize){
            ret = builder.CreateBitCast(val, builder.getFloatTy());
        }else if (64 == dataSize){
            llvm::Type* vecType = IGCLLVM::FixedVectorType::get(builder.getFloatTy(), 2);
            ret = builder.CreateBitCast(val, vecType);
        }else{
            IGC_ASSERT_EXIT_MESSAGE(0, "Unsupported type in ConvertToFloat of helper.");
        }

        return ret;
    }

    void ConvertToFloat(llvm::IRBuilder<>& builder, llvm::SmallVectorImpl<llvm::Value*>& instList)
    {
        for (size_t i=0; i<instList.size(); i++)
        {
            llvm::Value* val = ConvertToFloat(builder, instList[i]);
            if (val->getType()->isVectorTy())
            {
                instList[i] = builder.CreateExtractElement(val, static_cast<uint64_t>(0));
                size_t iOld = i;
                for (unsigned j = 1; j < cast<IGCLLVM::FixedVectorType>(val->getType())->getNumElements(); j++)
                {
                    instList.insert(instList.begin()+ iOld +j, builder.CreateExtractElement(val, j));
                    i++;
                }
            }
            else
            {
                instList[i] = val;
            }
        }
    }

    void ScalarizeAggregateMembers(llvm::IRBuilder<>& builder, llvm::Value* val, llvm::SmallVectorImpl<llvm::Value*> & instList)
    {
        llvm::Type* type = val->getType();
        unsigned num = 0;
        switch (type->getTypeID())
        {
        case llvm::Type::FloatTyID:
        case llvm::Type::HalfTyID:
        case llvm::Type::IntegerTyID:
        case llvm::Type::DoubleTyID:
            instList.push_back(val);
            break;
        case llvm::Type::StructTyID:
            num = type->getStructNumElements();
            for (unsigned i = 0; i < num; i++)
            {
                ScalarizeAggregateMembers(builder, builder.CreateExtractValue(val, i), instList);
            }
            break;
        case IGCLLVM::VectorTyID:
            num = (unsigned)cast<IGCLLVM::FixedVectorType>(type)->getNumElements();
            for (unsigned i = 0; i < num; i++)
            {
                ScalarizeAggregateMembers(builder, builder.CreateExtractElement(val, i), instList);
            }
            break;
        case llvm::Type::ArrayTyID:
            num = static_cast<uint32_t>(type->getArrayNumElements());
            for (unsigned i = 0; i < num; i++)
            {
                ScalarizeAggregateMembers(builder, builder.CreateExtractValue(val, i), instList);
            }
            break;
        default:
            IGC_ASSERT_EXIT_MESSAGE(0, "Unsupported type in ScalarizeAggregateMembers of helper! Please enhance this function first.");
            break;
        }
    }

    void ScalarizeAggregateMemberAddresses(IGCLLVM::IRBuilder<>& builder, llvm::Type* type, llvm::Value* val, llvm::SmallVectorImpl<llvm::Value*> & instList, llvm::SmallVector<llvm::Value*, 16> indices)
    {
        unsigned num = 0;
        switch (type->getTypeID())
        {
        case llvm::Type::FloatTyID:
        case llvm::Type::HalfTyID:
        case llvm::Type::IntegerTyID:
        case llvm::Type::DoubleTyID:
            instList.push_back(builder.CreateInBoundsGEP(val, makeArrayRef(indices)));
            break;
        case llvm::Type::StructTyID:
            num = type->getStructNumElements();
            for (unsigned i = 0; i < num; i++)
            {
                indices.push_back(builder.getInt32(i));
                ScalarizeAggregateMemberAddresses(builder, type->getStructElementType(i), val, instList, indices);
                indices.pop_back();
            }
            break;
        case IGCLLVM::VectorTyID:
            num = (unsigned)cast<IGCLLVM::FixedVectorType>(type)->getNumElements();
            for (unsigned i = 0; i < num; i++)
            {
                indices.push_back(builder.getInt32(i));
                ScalarizeAggregateMemberAddresses(builder, cast<VectorType>(type)->getElementType(), val, instList, indices);
                indices.pop_back();
            }
            break;
        case llvm::Type::ArrayTyID:
            //fix this if one API could support an array with length > 2^32
            num = static_cast<uint32_t>(type->getArrayNumElements());
            for (unsigned i = 0; i < num; i++)
            {
                indices.push_back(builder.getInt32(i));
                ScalarizeAggregateMemberAddresses(builder, type->getArrayElementType(), val, instList, indices);
                indices.pop_back();
            }
            break;
        default:
            IGC_ASSERT_EXIT_MESSAGE(0, "Unsupported type in ScalarizeAggregateMemberAddresses of helper! Please enhance this function first.");
            break;
        }
    }

    bool IsUnsignedCmp(const llvm::CmpInst::Predicate Pred)
    {
        switch (Pred) {
        case llvm::CmpInst::ICMP_UGT:
        case llvm::CmpInst::ICMP_UGE:
        case llvm::CmpInst::ICMP_ULT:
        case llvm::CmpInst::ICMP_ULE:
            return true;
        default:
            break;
        }
        return false;
    }

    bool IsSignedCmp(const llvm::CmpInst::Predicate Pred)
    {
        switch (Pred)
        {
        case llvm::CmpInst::ICMP_SGT:
        case llvm::CmpInst::ICMP_SGE:
        case llvm::CmpInst::ICMP_SLT:
        case llvm::CmpInst::ICMP_SLE:
            return true;
        default:
            break;
        }
        return false;
    }

    // isA64Ptr - Queries whether given pointer type requires 64-bit representation in vISA
    bool isA64Ptr(llvm::PointerType* PT, CodeGenContext* pContext)
    {
        return pContext->getRegisterPointerSizeInBits(PT->getAddressSpace()) == 64;
    }

    bool IsBitCastForLifetimeMark(const llvm::Value* V)
    {
        if (!V || !llvm::isa<llvm::BitCastInst>(V))
        {
            return false;
        }
        for (llvm::Value::const_user_iterator it = V->user_begin(), e = V->user_end(); it != e; ++it)
        {
            const llvm::IntrinsicInst* inst = llvm::dyn_cast<const llvm::IntrinsicInst>(*it);
            if (!inst)
            {
                return false;
            }
            llvm::Intrinsic::ID  IID = inst->getIntrinsicID();
            if (IID != llvm::Intrinsic::lifetime_start &&
                IID != llvm::Intrinsic::lifetime_end)
            {
                return false;
            }
        }
        return true;
    }

    Value* mutatePtrType(Value* ptrv, PointerType* newType,
        IRBuilder<>& builder, const Twine&)
    {
        if (isa<ConstantPointerNull>(ptrv))
        {
            return ConstantPointerNull::get(newType);
        }
        else
        {
            if (ConstantExpr * cexpr = dyn_cast<ConstantExpr>(ptrv))
            {
                IGC_ASSERT(cexpr->getOpcode() == Instruction::IntToPtr);
                Value* offset = cexpr->getOperand(0);
                ptrv = builder.CreateIntToPtr(offset, newType);
            }
            else
            {
                ptrv->mutateType(newType);
            }
        }
        return ptrv;
    }

    /*
    cmp.l.f0.0 (8) null:d       r0.0<0;1,0>:w    0x0000:w         { Align1, N1, NoMask, NoCompact }
    (-f0.0) jmpi Test
    (-f0.0) sendc (8) null:ud      r120.0<0;1,0>:f  0x00000025  0x08031400:ud    { Align1, N1, EOT, NoCompact }
    nop
    Test :
    nop

    */

    static const unsigned int CRastHeader_SIMD8[] =
    {
        0x05600010,0x20001a24,0x1e000000,0x00000000,
        0x00110020,0x34000004,0x0e001400,0x00000020,
        0x05710032,0x20003a00,0x06000f00,0x88031400,
        0x00000000,0x00000000,0x00000000,0x00000000,
    };

    /*
    cmp.l.f0.0 (16) null:d       r0.0 < 0; 1, 0 > : w    0x0000 : w{ Align1, N1, NoMask, NoCompact }
    (-f0.0) jmpi(1) Test { Align1, N1, NoMask, NoCompact }
    (-f0.0) sendc(16) null : ud      r120.0 < 0; 1, 0 > : f  0x00000025 0x90031000 : ud{ Align1, N1, EOT, NoCompact }
    nop
    Test :
    nop

    */
    static const unsigned int CRastHeader_SIMD16[] =
    {
        0x05800010, 0x20001A24, 0x1E000000, 0x00000000,
        0x00110020, 0x34000004, 0x0E001400, 0x00000020,
        0x05910032, 0x20003A00, 0x06000F00, 0x90031000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
    };

    /*
    cmp.l.f0.0 (16) null:d       r0.0 < 0; 1, 0 > : w    0x0000 : w{ Align1, N1, NoMask, NoCompact }
    (-f0.0) jmpi Test
    (-f0.0) sendc(16) null : w r120.0 < 0; 1, 0 > : ud  0x00000005 0x10031000 : ud{ Align1, N1, NoCompact }
    (-f0.0) sendc(16) null : w r120.0 < 0; 1, 0 > : f  0x00000025  0x10031800 : ud{ Align1, N5, EOT, NoCompact }
    nop
    Test :
    nop

    */

    static const unsigned int CRastHeader_SIMD32[] =
    {
        0x05800010,0x20001a24,0x1e000000,0x00000000,
        0x00110020,0x34000004,0x0e001400,0x00000020,
        0x05910032,0x20000260,0x06000f00,0x10031000,
        0x05912032,0x20003a60,0x06000f00,0x90031800,
    };


    unsigned int AppendConservativeRastWAHeader(IGC::SProgramOutput* program, SIMDMode simdmode)
    {
        uint32_t headerSize = 0;
        if (program && (program->m_programSize > 0))
        {
            headerSize = AppendConservativeRastWAHeader(program->m_programBin, program->m_programSize, simdmode);
        }
        return headerSize;
    }

    unsigned int AppendConservativeRastWAHeader(void*& pBinary, unsigned int& binarySize, SIMDMode simdmode)
    {
        unsigned int headerSize = 0;
        const unsigned int* pHeader = nullptr;

        if (pBinary && (binarySize > 0))
        {
            switch (simdmode)
            {
            case SIMDMode::SIMD8:
                headerSize = sizeof(CRastHeader_SIMD8);
                pHeader = CRastHeader_SIMD8;
                break;

            case SIMDMode::SIMD16:
                headerSize = sizeof(CRastHeader_SIMD16);
                pHeader = CRastHeader_SIMD16;
                break;

            case SIMDMode::SIMD32:
                headerSize = sizeof(CRastHeader_SIMD32);
                pHeader = CRastHeader_SIMD32;
                break;

            default:
                IGC_ASSERT_MESSAGE(0, "Invalid SIMD Mode for Conservative Raster WA");
                break;
            }

            unsigned int newSize = binarySize + headerSize;
            void* newBinary = IGC::aligned_malloc(newSize, 16);
            memcpy_s(newBinary, newSize, pHeader, headerSize);
            memcpy_s((char*)newBinary + headerSize, newSize, pBinary, binarySize);
            IGC::aligned_free(pBinary);
            pBinary = newBinary;
            binarySize = newSize;
        }
        return headerSize;
    }

    bool DSDualPatchEnabled(class CodeGenContext* ctx)
    {
        return ctx->platform.supportDSDualPatchDispatch() &&
            ctx->platform.WaDisableDSDualPatchMode() &&
            !(ctx->m_DriverInfo.APIDisableDSDualPatchDispatch()) &&
            IGC_IS_FLAG_DISABLED(DisableDSDualPatch);
    }


    Function* getUniqueEntryFunc(const IGCMD::MetaDataUtils* pM, IGC::ModuleMetaData* pModMD)
    {
        Function* entryFunc = nullptr;
        for (auto i = pM->begin_FunctionsInfo(), e = pM->end_FunctionsInfo(); i != e; ++i)
        {
            IGCMD::FunctionInfoMetaDataHandle Info = i->second;
            if (Info->getType() != FunctionTypeMD::KernelFunction)
            {
                continue;
            }

            Function* F = i->first;
            if (!entryFunc)
            {
                entryFunc = F;
            }
            else
            {
                // Multiple entries found, return null since there is no unique entry
                return nullptr;
            }
        }
        return entryFunc;
    }

    // If true, the codegen will likely not emit instruction for this instruction.
    bool isNoOpInst(Instruction* I, CodeGenContext* Ctx)
    {
        if (isa<BitCastInst>(I) ||
            isa<IntToPtrInst>(I) ||
            isa<PtrToIntInst>(I))
        {
            // Don't bother with constant operands
            if (isa<Constant>(I->getOperand(0))) {
                return false;
            }

            Type* dTy = I->getType();
            Type* sTy = I->getOperand(0)->getType();
            PointerType* dPTy = dyn_cast<PointerType>(dTy);
            PointerType* sPTy = dyn_cast<PointerType>(sTy);
            uint32_t dBits = dPTy ? Ctx->getRegisterPointerSizeInBits(dPTy->getAddressSpace())
                : (unsigned int)dTy->getPrimitiveSizeInBits();
            uint32_t sBits = sPTy ? Ctx->getRegisterPointerSizeInBits(sPTy->getAddressSpace())
                : (unsigned int)sTy->getPrimitiveSizeInBits();
            if (dBits == 0 || sBits == 0 || dBits != sBits) {
                // Not primitive type or not equal in size (inttoptr, etc)
                return false;
            }

            IGCLLVM::FixedVectorType* dVTy = dyn_cast<IGCLLVM::FixedVectorType>(dTy);
            IGCLLVM::FixedVectorType* sVTy = dyn_cast<IGCLLVM::FixedVectorType>(sTy);
            int d_nelts = dVTy ? (int)dVTy->getNumElements() : 1;
            int s_nelts = sVTy ? (int)sVTy->getNumElements() : 1;
            if (d_nelts != s_nelts) {
                // Vector relayout bitcast.
                return false;
            }
            return true;
        }
        return false;
    }

    //
    // Given a value, check if it is likely a positive number.
    //
    // This function works best if llvm.assume() is used in the bif libraries to
    // give ValueTracking hints.  ex:
    //
    // size_t get_local_id(uint dim)
    // {
    //    size_t ret = __builtin_IB_get_local_id()
    //    __builtin_assume(ret >= 0);
    //    __builtin_assume(ret <= 0x0000ffff)
    //    return ret;
    // }
    //
    // This implementation relies completly on native llvm functions
    //
    //
    //
    bool valueIsPositive(
        Value* V,
        const DataLayout* DL,
        llvm::AssumptionCache* AC,
        llvm::Instruction* CxtI)
    {
#if LLVM_VERSION_MAJOR == 4
        bool isKnownNegative = false;
        bool isKnownPositive = false;
        llvm::ComputeSignBit(
            V,
            isKnownPositive,
            isKnownNegative,
            *DL,
            0,
            AC,
            CxtI);
        return isKnownPositive;
#elif LLVM_VERSION_MAJOR >= 7
        return computeKnownBits(
            V,
            *DL,
            0,
            AC,
            CxtI).isNonNegative();
#endif
    }

    void appendToUsed(llvm::Module& M, ArrayRef<GlobalValue*> Values)
    {
        std::string Name = "llvm.used";
        GlobalVariable* GV = M.getGlobalVariable(Name);
        SmallPtrSet<Constant*, 16> InitAsSet;
        SmallVector<Constant*, 16> Init;
        if (GV) {
            ConstantArray* CA = dyn_cast<ConstantArray>(GV->getInitializer());
            for (auto& Op : CA->operands()) {
                Constant* C = cast_or_null<Constant>(Op);
                if (InitAsSet.insert(C).second)
                    Init.push_back(C);
            }
            GV->eraseFromParent();
        }

        Type* Int8PtrTy = llvm::Type::getInt8PtrTy(M.getContext());
        for (auto* V : Values) {
            Constant* C = V;
            //llvm will complain if members of llvm.uses doesn't have a name
            if(C->getName().empty())
                C->setName("gVar");

            if (V->getType()->getAddressSpace() != 0)
                C = ConstantExpr::getAddrSpaceCast(V, Int8PtrTy);
            else
                C = ConstantExpr::getBitCast(V, Int8PtrTy);
            if (InitAsSet.insert(C).second)
                Init.push_back(C);
        }

        if (Init.empty())
            return;

        ArrayType* ATy = ArrayType::get(Int8PtrTy, Init.size());
        GV = new llvm::GlobalVariable(M, ATy, false, GlobalValue::AppendingLinkage,
            ConstantArray::get(ATy, Init), Name);
        GV->setSection("llvm.metadata");
    }

    bool safeScheduleUp(llvm::BasicBlock* BB, llvm::Value* V, llvm::Instruction*& InsertPos, llvm::DenseSet<llvm::Instruction*> Scheduled)
    {
        llvm::Instruction* I = llvm::dyn_cast<llvm::Instruction>(V);
        if (!I)
            return false;

        // Skip value defined in other BBs.
        if (I->getParent() != BB)
            return false;

        // Skip phi-node as they are eventually defined in other BBs.
        if (llvm::isa<llvm::PHINode>(I))
            return false;

        // Skip for side effect instructions
        if (I->mayHaveSideEffects())
            return false;

        // Don't re-schedule instruction again.
        if (Scheduled.count(I)) {
            if (InsertPos && !isInstPrecede(I, InsertPos))
                InsertPos = I;
            return false;
        }

        bool Changed = false;

        // Try to schedule all its operands first.
        for (auto OI = I->op_begin(), OE = I->op_end(); OI != OE; ++OI)
            Changed |= safeScheduleUp(BB, OI->get(), InsertPos, Scheduled);

        // Mark this instruction `visited`.
        Scheduled.insert(I);

        // Skip if the instruction is already defined before insertion position.
        if (InsertPos && isInstPrecede(I, InsertPos))
            return Changed;

        // Schedule itself.
        if (InsertPos) {
            I->removeFromParent();
            I->insertAfter(InsertPos);
        }

        InsertPos = I;
        return true;
    }

    ConstantInt* getConstantSInt(
        IRBuilder<>& Builder, const int bitSize, int64_t val)
    {
        ConstantInt* res = nullptr;
        switch (bitSize) {
        case 8: res = Builder.getInt8((uint8_t)val); break;
        case 16: res = Builder.getInt16((uint16_t)val); break;
        case 32: res = Builder.getInt32((uint32_t)val); break;
        case 64: res = Builder.getInt64((uint64_t)val); break;
        default:
            IGC_ASSERT_MESSAGE(0, "invalid bitsize");
        }
        return res;
    }

    ConstantInt* getConstantUInt(
        IRBuilder<>& Builder, const int bitSize, uint64_t val)
    {
        ConstantInt* res = nullptr;
        switch (bitSize) {
        case 8: res = Builder.getInt8((uint8_t)val); break;
        case 16: res = Builder.getInt16((uint16_t)val); break;
        case 32: res = Builder.getInt32((uint32_t)val); break;
        case 64: res = Builder.getInt64(val); break;
        default:
            IGC_ASSERT_MESSAGE(0, "invalid bitsize");
        }
        return res;
    }

    // MulH implementation for 64-bit signed integers
    Value* CreateMulhS64(IRBuilder<>& B, Value* const u, Value* const v) {
        // This comes from Hacker's Delight 8-2.
        // Think of this as elementry schoole multiplication, but base 2^32.
        ConstantInt* const loMask = getConstantSInt(B, 64, 0xFFFFFFFFll);
        ConstantInt* const hiShift = getConstantSInt(B, 64, 32);
        //
        // u64 u0 = u & 0xFFFFFFFF; s64 u1 = u >> 32;
        // u64 v0 = v & 0xFFFFFFFF; s64 v1 = v >> 32;
        Value* const u0 = B.CreateAnd(u, loMask, "u.lo32");
        Value* const u1 = B.CreateAShr(u, hiShift, "u.hi32");
        Value* const v0 = B.CreateAnd(v, loMask, "v.lo32");
        Value* const v1 = B.CreateAShr(v, hiShift, "v.hi32");
        //
        // w = u0*v0
        Value* const w0 = B.CreateMul(u0, v0, "w0");
        //
        // t = u1*v0 + (w0 >> 32)
        Value* const tLHS = B.CreateMul(u1, v0);
        Value* const tRHS = B.CreateLShr(w0, hiShift, "w0.lo32");
        Value* const t = B.CreateAdd(tLHS, tRHS, "t");
        //
        // w1 = u0*v0 + (t >> 32)
        Value* const u0v1 = B.CreateMul(u0, v1);
        Value* const tLO32 = B.CreateAnd(t, loMask, "t.lo32");
        Value* const w1 = B.CreateAdd(u0v1, tLO32, "w1");
        //
        // return u0*v1 + (t >> 32) + (w1 >> 32)
        Value* const u1v1 = B.CreateMul(u1, v1);
        Value* const tHI32 = B.CreateAShr(t, hiShift, "t.hi32");
        Value* const rLHS = B.CreateAdd(u1v1, tHI32);
        Value* const rRHS = B.CreateAShr(w1, hiShift, "w1.lo32");
        Value* const r = B.CreateAdd(rLHS, rRHS, "uv");
        //
        return r;
    }

    // MulH implementation for 64-bit unsigned integers
    Value* CreateMulhU64(IRBuilder<>& B, Value* const u, Value* const v)
    {
        // This is the same as CreateMulhS64, but with all logical shifts.
        ConstantInt* const loMask = getConstantUInt(B, 64, 0xFFFFFFFFull);
        ConstantInt* const hiShift = getConstantUInt(B, 64, 32);
        //
        // u64 u0 = u & 0xFFFFFFFF, u1 = u >> 32;
        // u64 v0 = v & 0xFFFFFFFF, v1 = v >> 32;
        Value* const u0 = B.CreateAnd(u, loMask, "u.lo32");
        Value* const u1 = B.CreateLShr(u, hiShift, "u.hi32");
        Value* const v0 = B.CreateAnd(v, loMask, "v.lo32");
        Value* const v1 = B.CreateLShr(v, hiShift, "v.hi32");
        //
        // w0 = u0*v0
        Value* const w0 = B.CreateMul(u0, v0, "w0");
        //
        // t = u1*v0 + (w0 >> 32)
        Value* const tLHS = B.CreateMul(u1, v0);
        Value* const tRHS = B.CreateLShr(w0, hiShift, "w0.lo32");
        Value* const t = B.CreateAdd(tLHS, tRHS, "t");
        //
        // w1 = u0*v0 + (t >> 32)
        Value* const u0v1 = B.CreateMul(u0, v1);
        Value* const tLO32 = B.CreateAnd(t, loMask, "t.lo32");
        Value* const w1 = B.CreateAdd(u0v1, tLO32, "w1");
        //
        // w1 = u0*v1 + (t >> 32) + (w1 >> 32)
        Value* const u1v1 = B.CreateMul(u1, v1);
        Value* const tHI32 = B.CreateLShr(t, hiShift, "t.hi32");
        Value* const rLHS = B.CreateAdd(u1v1, tHI32);
        Value* const rRHS = B.CreateLShr(w1, hiShift, "w1.lo32");
        Value* const r = B.CreateAdd(rLHS, rRHS, "uv");
        //
        return r;
    }

    // MulH implementation for 32/64 bit integers
    Value* CreateMulh(
        Function& F,
        IRBuilder<>&  B,
        const bool isSigned,
        Value* const u,
        Value* const v)
    {
        Value* res = nullptr;
        IGC_ASSERT(nullptr != u);
        IGC_ASSERT(nullptr != u->getType());
        int bitWidth = u->getType()->getIntegerBitWidth();
        switch(bitWidth)
        {
        case 32:
        {
            // we have a dedicated machine instruction for 32b
            SmallVector<Value*, 2> imulhArgs;
            imulhArgs.push_back(u);
            imulhArgs.push_back(v);
            auto intrinsic = isSigned ?
                GenISAIntrinsic::GenISA_imulH :
                GenISAIntrinsic::GenISA_umulH;
            IGC_ASSERT(nullptr != v);
            Function* const iMulhDecl = llvm::GenISAIntrinsic::getDeclaration(
                F.getParent(),
                intrinsic,
                v->getType());
            res = B.CreateCall(iMulhDecl, imulhArgs, "q_appx");
            break;
        }
        case 64:
            // emulate via 64b arithmetic
            if (isSigned) {
                res = CreateMulhS64(B, u, v);
            }
            else {
                res = CreateMulhU64(B, u, v);
            }
            break;
        default:
            IGC_ASSERT_MESSAGE(0, "CreateMulH must be 32 or 64");
        }
        return res;
    }

    bool hasInlineAsmInFunc(llvm::Function& F)
    {
        for (auto ii = inst_begin(&F), ie = inst_end(&F); ii != ie; ii++)
        {
            if (llvm::CallInst* call = llvm::dyn_cast<llvm::CallInst>(&*ii))
            {
                if (call->isInlineAsm())
                {
                    return true;
                }
            }
        }
        return false;
    }

    // Parses the "vector-variant" attribute string to get a valid function
    // variant symbol string supported by current implementation of IGC.
    //
    // Returns a tuple of values:
    // R(0) is the reformatted variant symbol string.
    // R(1) is the called function's name.
    // R(2) is the required SIMD size.
    // See the spec for Intel Vector Function ABI for parsed symbol meanings.
    std::tuple<std::string, std::string, unsigned> ParseVectorVariantFunctionString(llvm::StringRef varStr)
    {
        unsigned vlen = 0;
        std::stringstream outStr;

        auto pos = varStr.begin();
        auto strEnd = varStr.end();

        // Starts with _ZGV
        IGC_ASSERT(varStr.startswith("_ZGV"));
        outStr << "_ZGV";
        pos += 4;
        // ISA class target processor type
        IGC_ASSERT(*pos == 'x' || *pos == 'y' || *pos == 'Y' || *pos == 'z' || *pos == 'Z');
        outStr << *pos;
        pos++;
        // Mask or NoMask, only support NoMask for now
        IGC_ASSERT(*pos == 'M' || *pos == 'N');
        outStr << 'N';
        pos++;
        // Parse vector length (input can be 1/2/4/8/16/32, output restricted to 8/16/32)
        auto idStart = pos;
        while (*pos >= '0' && *pos <= '9') pos++;
        IGC_ASSERT(StringRef(idStart, pos - idStart).getAsInteger(10, vlen) == 0);
        IGC_ASSERT(vlen == 1 || vlen == 2 || vlen == 4 || vlen == 8 || vlen == 16 || vlen == 32);
        // Set min SIMD width to 8
        vlen = (vlen < 8) ? 8 : vlen;
        outStr << std::to_string(vlen);

        while (pos != strEnd)
        {
            // End of vector properties symbols terminated with '_'
            if (*pos == '_') {
                outStr << *pos++;
                break;
            }
            // Parameter variant type, only support default vector type
            IGC_ASSERT(*pos == 'l' || *pos == 'u' || *pos == 'v' || *pos == 'R' || *pos == 'U' || *pos == 'L');
            outStr << 'v';
            pos++;
            // Ignore alignment properties
            if (*pos == 'a') {
                pos++;
                while (*pos >= '0' && *pos <= '9') pos++;
            }
        }

        // Remaining characters form the function name
        std::string fName = StringRef(pos, strEnd - pos).str();

        return std::make_tuple(outStr.str(), fName, vlen);
    }

    ///
    /// Return base type from more complex type
    ///
    /// Return nullptr if complex type cannot be defined with only one type
    ///
    llvm::Type* GetBaseType(llvm::Type* ProcessedType)
    {
        while (ProcessedType->isArrayTy() || ProcessedType->isStructTy())
        {
            if (ProcessedType->isArrayTy())
                ProcessedType = ProcessedType->getArrayElementType();
            else
            {
                if (ProcessedType->getStructNumElements() != 1)
                    return nullptr;

                ProcessedType = ProcessedType->getStructElementType(0);
            }
        }

        return ProcessedType;
    }

    // Function modifies address space in selected uses of given input value
    void FixAddressSpaceInAllUses(llvm::Value* ptr, uint newAS, uint oldAS)
    {
        IGC_ASSERT(newAS != oldAS);

        for (auto UI = ptr->user_begin(), E = ptr->user_end(); UI != E; ++UI)
        {
            Instruction* inst = dyn_cast<Instruction>(*UI);
            PointerType* instType = nullptr;
            if (isa<BitCastInst>(inst) || isa<GetElementPtrInst>(inst) ||
                isa<AddrSpaceCastInst>(inst) || isa<PHINode>(inst))
            {
                instType = dyn_cast<PointerType>(inst->getType());
            }

            if (instType && instType->getAddressSpace() == oldAS)
            {
                Type* eltType = instType->getElementType();
                PointerType* ptrType = PointerType::get(eltType, newAS);
                inst->mutateType(ptrType);
                FixAddressSpaceInAllUses(inst, newAS, oldAS);
            }
        }
    }

    // This function generates code for sample or gather parameters combining.
    // The first parameter is converted to unsigned integer value and copied
    // into the `numBits` LSB of second param's mantissa.
    // The function is used to combine:
    // - LOD and AI params in sample_l and sample_l_c
    // - BIAS and AI params in sample_b and sample_b_c
    Value* CombineSampleOrGather4Params(
        IRBuilder<>& builder,
        Value* param1, // first param to be copied into the second param
        Value* param2, // second parameter
        uint numBits, // number of bits to use to for the second param
        const std::string& param1Name, // debug name for the first param
        const std::string& param2Name) // debug name for the second param
    {
        Value* maskLo = builder.getInt32(BITMASK(numBits));
        Value* maskHi = builder.getInt32((~(BITMASK(numBits))));

        Function* rdneFunc = GenISAIntrinsic::getDeclaration(
            builder.GetInsertBlock()->getModule(),
            GenISAIntrinsic::GenISA_ROUNDNE);
        Value* intParam1 = builder.CreateFPToUI(
            builder.CreateCall(rdneFunc, param1),
            builder.getInt32Ty(),
            VALUE_NAME(std::string("_int") + param1Name));
        Value* intParam1Lsb = builder.CreateAnd(
            intParam1,
            maskLo,
            VALUE_NAME(intParam1->getName() + "LSB"));
        intParam1Lsb = builder.CreateSelect(
            builder.CreateICmp(CmpInst::Predicate::ICMP_EQ, intParam1, intParam1Lsb),
            intParam1Lsb,
            maskLo,
            VALUE_NAME(intParam1->getName() + "ClampedLSB"));
        Value* param2Int = builder.CreateBitCast(
            param2,
            builder.getInt32Ty(),
            VALUE_NAME(param2Name + "Int"));
        Value* param2IntMsb = builder.CreateAnd(
            param2Int,
            maskHi,
            VALUE_NAME(intParam1->getName() + "MSB"));
        return builder.CreateBitCast(
            builder.CreateOr(intParam1Lsb, param2IntMsb),
            builder.getFloatTy(),
            VALUE_NAME(param1Name + param2Name + "Combined"));
    };

std::pair<Value*, unsigned int> GetURBBaseAndOffset(Value* pUrbOffset)
{
    Value* pBase = pUrbOffset;
    unsigned int offset = 0;

    auto GetConstant = [](Value* pVal)->unsigned int
    {
        IGC_ASSERT(isa<ConstantInt>(pVal));
        ConstantInt* pConst = cast<ConstantInt>(pVal);
        return int_cast<unsigned int>(pConst->getZExtValue());
    };

    if (isa<ConstantInt>(pUrbOffset))
    {
        Value* pNullBase = nullptr;
        return std::make_pair(
            pNullBase,
            GetConstant(pUrbOffset));
    }
    else if (isa<Instruction>(pUrbOffset))
    {
        Instruction* pInstr = cast<Instruction>(pUrbOffset);
        if (pInstr->getOpcode() == Instruction::Add)
        {
            Value* src0 = pInstr->getOperand(0);
            Value* src1 = pInstr->getOperand(1);
            if (isa<ConstantInt>(src1))
            {
                auto baseAndOffset = GetURBBaseAndOffset(src0);
                pBase = baseAndOffset.first;
                offset = GetConstant(src1) + baseAndOffset.second;
            }
            else if (isa<ConstantInt>(src0))
            {
                auto baseAndOffset = GetURBBaseAndOffset(src1);
                pBase = baseAndOffset.first;
                offset = GetConstant(src0) + baseAndOffset.second;
            }
        }
        else if (pInstr->getOpcode() == Instruction::Or)
        {
            // Examples of patterns matched below:
            // 1. shl + or
            //    urbOffset = urbOffset << 1;
            //    urbOffset = urbOffset | 1;
            // 2. mul + or
            //    urbOffset = urbOffset * 2;
            //    urbOffset = urbOffset | 1;
            // 3. two oword urb writes in loop
            //    urbOffset = urbOffset * 2;
            //    for(...) {
            //      {...}
            //      urbOffset = urbOffset | 1;
            //      urbOffset = urbOffset + 2;
            //      {...}
            //    }
            //
            //

            std::function<unsigned int(Value*)> GetAlignment =
                [&GetAlignment, &GetConstant](Value* pUrbOffset)->unsigned int
            {
                unsigned int alignment = 1;
                Instruction* pInstr = dyn_cast<Instruction>(pUrbOffset);
                if (pInstr &&
                    pInstr->getOpcode() == Instruction::Shl &&
                    isa<ConstantInt>(pInstr->getOperand(1)))
                {
                    alignment = GetAlignment(pInstr->getOperand(0)) *
                        (1u << GetConstant(pInstr->getOperand(1)));
                }
                else if (pInstr &&
                    pInstr->getOpcode() == Instruction::Mul &&
                    isa<ConstantInt>(pInstr->getOperand(1)) &&
                    iSTD::IsPowerOfTwo(GetConstant(pInstr->getOperand(1))))
                {
                    alignment = GetAlignment(pInstr->getOperand(0)) *
                        GetConstant(pInstr->getOperand(1));
                }
                else if (pInstr &&
                    pInstr->getOpcode() == Instruction::Mul &&
                    isa<ConstantInt>(pInstr->getOperand(0)) &&
                    iSTD::IsPowerOfTwo(GetConstant(pInstr->getOperand(0))))
                {
                    alignment = GetAlignment(pInstr->getOperand(1)) *
                        GetConstant(pInstr->getOperand(0));
                }
                else if (isa<ConstantInt>(pUrbOffset))
                {
                    alignment = 1 << iSTD::bsf(GetConstant(pUrbOffset));
                }
                return alignment;
            };

            Value* src0 = pInstr->getOperand(0);
            Value* src1 = pInstr->getOperand(1);
            unsigned int alignment = 1;
            if (isa<PHINode>(src0) && isa<ConstantInt>(src1))
            {
                // pattern 3
                PHINode* pPhi = cast<PHINode>(src0);
                alignment = std::numeric_limits<unsigned int>::max();
                for (unsigned int i = 0; i < pPhi->getNumIncomingValues(); i++)
                {
                    Instruction* pIncoming = dyn_cast<Instruction>(pPhi->getIncomingValue(i));
                    if (pIncoming &&
                        pIncoming->getOpcode() == Instruction::Add &&
                        pPhi == pIncoming->getOperand(0) &&
                        isa<ConstantInt>(pIncoming->getOperand(1)) &&
                        iSTD::IsPowerOfTwo(GetConstant(pIncoming->getOperand(1))))
                    {
                        alignment = std::min(alignment, GetConstant(pIncoming->getOperand(1)));
                    }
                    else
                    {
                        alignment = std::min(alignment, GetAlignment(pPhi->getIncomingValue(i)));
                    }
                }
            }
            else
            {
                // patterns 1 and 2
                alignment = GetAlignment(src0);
            }
            if (alignment > GetConstant(src1))
            {
                IGC_ASSERT(iSTD::IsPowerOfTwo(alignment));
                pBase = src0;
                offset = GetConstant(src1);
            }
        }
    }

    return std::make_pair(pBase, offset);
}

std::vector<std::pair<unsigned int, std::string>> GetPrintfStrings(Module &M)
{
    std::vector<std::pair<unsigned int, std::string>> printfStrings;
    std::string MDNodeName = "printf.strings";
    NamedMDNode* printfMDNode = M.getOrInsertNamedMetadata(MDNodeName);

    for (uint i = 0, NumStrings = printfMDNode->getNumOperands();
         i < NumStrings;
         i++)
    {
        MDNode* argMDNode = printfMDNode->getOperand(i);
        ConstantInt* indexOpndVal =
            mdconst::dyn_extract<ConstantInt>(argMDNode->getOperand(0));
        MDString* stringOpndVal =
            dyn_cast<MDString>(argMDNode->getOperand(1));

        printfStrings.push_back({
            int_cast<unsigned int>(indexOpndVal->getZExtValue()),
            stringOpndVal->getString().data()
        });
    }

    return printfStrings;
}

bool PDT_dominates(llvm::PostDominatorTree& PTD,
    const Instruction* I1,
    const Instruction* I2)
{
    IGC_ASSERT_MESSAGE(I1, "Expecting valid I1 and I2");
    IGC_ASSERT_MESSAGE(I2, "Expecting valid I1 and I2");

    const BasicBlock* BB1 = I1->getParent();
    const BasicBlock* BB2 = I2->getParent();

    if (BB1 != BB2)
        return PTD.dominates(BB1, BB2);

    // PHINodes in a block are unordered.
    if (isa<PHINode>(I1) && isa<PHINode>(I2))
        return false;

    // Loop through the basic block until we find I1 or I2.
    BasicBlock::const_iterator I = BB1->begin();
    for (; &*I != I1 && &*I != I2; ++I)
        /*empty*/;

    return &*I == I2;
}

} // namespace IGC
