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

#include "Compiler/Optimizer/OCLBIUtils.h"
#include "Compiler/Optimizer/CodeAssumption.hpp"
#include "Compiler/IGCPassSupport.h"

#include "Compiler/Optimizer/OpenCLPasses/StatelessToStatefull/StatelessToStatefull.hpp"

#include "common/Stats.hpp"
#include "common/secure_string.h"

#include "common/LLVMWarningsPush.hpp"

#include "llvmWrapper/IR/Instructions.h"

#include <llvmWrapper/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/Analysis/ValueTracking.h>
#include "common/LLVMWarningsPop.hpp"

#include <string>

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-stateless-to-statefull-resolution"
#define PASS_DESCRIPTION "Tries to convert stateless to statefull accesses"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(StatelessToStatefull, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
IGC_INITIALIZE_PASS_END(StatelessToStatefull, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

// This pass turns a global/constants address space (stateless) load/store into a statefull a load/store.
//
// The conservative approach is to search for any directly positively-indexed kernels argument, such as:
//
// __kernel void CopyBuffer( __global uint4* dst, __global uint4* src )
// {
//     uint4 data = src[ get_global_id(0) ];
//     dst[ get_global_id(0) ] = data;
// }
//
// ...and trun these accesses into statefull accesses.
//
// This has a several benefits
//  - Statefull pointer size is always 32bit - we always know the base so the binding table is always known
//  - OBus bandwidth is reduced with pointer size reduction
//    - 32bit data type bandwidth increases by ~50%
//  - Pointer math overhead is reduced by 50% on 64bit systems
//  - UMD has ability to set cachability control per surface instead of globally
//
// Limitations:
//   - This is not safe unless the UMD can guarentee allocations can fit in a surface state
//     - linux platforms allow > 4GB  allocations.
//       An internal flag "-cl-intel-greater-than-4GB-buffer-required" is used to pass buffer size
//       info to the compiler. If 4GB buffer is required, this optimization is off.
//   - Does not work for 'system SVM' platforms without knowing extra information about the platform
//   - UMD needs checks to make sure this binary is never saved and later run on a system SVM device
//     - this is not done yet!
//
//  Negative offset
//    This optimization is carried out if the address offset can be proven to be positive. Unless the
//    compiler does a fancy check on this,  it turns out that proving a positive offset would fail most
//    of time, at least this is the case for the current implementation as of 6/1/2017. To overcome
//    this issue, BUFFER_OFFSET implicit kernel arguments are added. With this, the compiler does not
//    need to prove the offset is positive any more.
//
//    The negative offset can happen under the following conditions:
//       1. clSetKernelArgSVMPointer() is used to set a kernel argument
//          with "P + offset", where P is returned from clSVMAlloc()
//       2. Kernel does have negative offset relative to its argument,
//            kernel void test(global float* svmptr,...)
//            {
//                ......  *(svmptr - c) ...   // negative, but (offset + c) >= 0
//            }
//    The compiler needs to handle this even though it rarely happens.  Note that if the svm is
//    the system SVM, "p" can be returned by malloc(), in which we cannot guarantee the 4GB buffer size.
//    Thus, this optimization must be turned off by the runtime by passing the flag to the compiler:
//             -cl-intel-greater-than-4GB-buffer-required"
//
//    We handle this case by passing "offset" in "P + offset" to the kernel, so that compiler
//    will add this offset to the address computation. With the above example,
//         kernel void test(global float* svmptr, int32 svmptr_offset,....)
//             stateless:   address = svmpotr - c
//             statefull:   offset = svmptr_offset - c
//    Note that offset will be in 32 bit integer,  either signed or unsigned, the final result
//    should be correct if the kernel's code does not have out-of-bound memory access (in this case,
//    the kernel code is wrong, and we don't really care what the wrong address will be.).
//
//    To implement this,  the compiler generates a new patch token (DATA_PARAMETER_BUFFER_OFFSET)
//    to the runtime, asking to pass an offset for a kernel pointer argument. (One token for one
//    offset, so, 5 offsets will have 5 tokens). AddImplicitArgs add those implicit arguments to
//    kernel.
//
//    - Flag and keys:
//      a new internal flag:  -cl-intel-has-buffer-offset-arg
//            This is needed as the classic ocl runtime does not need to support it. The presence of
//            this flag means BUFFER_OFFSET is supported.
//      Those three keys are for debugging purpose:
//        igc key: EnableStatelessToStatefull --> to turn this optimization on/off.
//        igc key: EnableSupportBufferOffset
//                 this is the key version of -cl-intel-has-buffer-offset-arg.
//        igc key: SToSProducesPositivePointer
//                 To assume all offsets are positive (all BUFFER_OFFSET = 0). Thus, no need to
//                 have implicit BUFFER_OFFSET arguments at all.
//

// Future things to look out for:
//  - This transformation cannot be done if a pointer is stored to or loaded from memory
//    In general, if an address of load/store cannot be resolevd to the kernel argument, the load/store
//    will still use stateless access. Note that the mix of stateless and statefull accesses is okay
//    in terms of correctness, and it is true even though cachability is set. 
//  - Need to watch out for a final address that less than the address of kenrel argument:
//     example: kernelArg[-2]
//
// 
// Possible Todos:
//  - Fancier back tracing to a kernel argument  
//  - Handle > 2 operand GetElementPtr instructions // DONE!
// 

char StatelessToStatefull::ID = 0;

StatelessToStatefull::StatelessToStatefull(bool hasBufOff)
    : FunctionPass(ID),
      m_hasBufferOffsetArg(hasBufOff),
      m_hasOptionalBufferOffsetArg(false),
      m_ACT(nullptr),
      m_pImplicitArgs(nullptr),
      m_pKernelArgs(nullptr),
      m_changed (false)
{
    initializeStatelessToStatefullPass(*PassRegistry::getPassRegistry());
}

bool StatelessToStatefull::runOnFunction(llvm::Function &F)
{
    MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

    // skip device enqueue tests for now to avoid tracking binding tables acorss
    // enqueued blocks.
    if((F.getParent()->getNamedMetadata("igc.device.enqueue") != nullptr) ||
       !isEntryFunc(pMdUtils, &F))
    {
        return false;
    }

    if (IGC_IS_FLAG_ENABLED(EnableCodeAssumption))
    {
        // Use assumption cache
        m_ACT = &getAnalysis<AssumptionCacheTracker>();
        AssumptionCache& AC = m_ACT->getAssumptionCache(F);
        CodeAssumption::addAssumption(&F, &AC);
    }
    else
    {
        m_ACT = nullptr;
    }

    // Caching arguments during the transformation
    m_hasOptionalBufferOffsetArg =
        (m_hasBufferOffsetArg && IGC_IS_FLAG_ENABLED(EnableOptionalBufferOffset));

    m_pImplicitArgs = new ImplicitArgs(F, pMdUtils);
    m_pKernelArgs = new KernelArgs(F, &(F.getParent()->getDataLayout()), pMdUtils, modMD);

    visit(F);

    finalizeArgInitialValue(&F);
    delete m_pImplicitArgs;
    delete m_pKernelArgs;
    return m_changed;
}

Argument* StatelessToStatefull::getBufferOffsetArg(Function* F, uint32_t ArgNumber)
{
    uint32_t nImplicitArgs = m_pImplicitArgs->size();
    uint32_t totalArgs = (uint32_t)IGCLLVM::GetFuncArgSize(F);
    uint32_t nExplicitArgs = (totalArgs - nImplicitArgs);
    uint32_t implicit_ix = m_pImplicitArgs->getNumberedArgIndex(ImplicitArg::BUFFER_OFFSET, ArgNumber);
    uint32_t arg_ix = nExplicitArgs + implicit_ix;
    Function::arg_iterator AI = F->arg_begin(), AE = F->arg_end();
    for (; AI != AE && AI->getArgNo() != arg_ix; ++AI);
    if (AI == AE)
    {
        assert(false && "Implicit arg for BUFFER_OFFSET is out of range!");
        return nullptr;
    }
    Argument *arg = &*AI;
    return arg;
}

//
// Convert GetElementPtrInst[s] into multiple instructions that compute the byte offset
// from the base represented by these GEP instructions. GEPs vector keeps its elements
// in the reverse order of execution, that is, the last element is the first GEP in the
// execution.
//
// Returns true if the GEP was able to be expanded to multiple instructions.
//
// The final instruction of the expansion is returned in 'offset'
//
bool StatelessToStatefull::getOffsetFromGEP(
    Function* F, SmallVector<GetElementPtrInst*, 4> GEPs,
    uint32_t argNumber, bool isImplicitArg, Value*& offset)
{
    Module* M = F->getParent();
    const DataLayout* DL = &M->getDataLayout();
    Type* int32Ty = Type::getInt32Ty(M->getContext());

    Value *PointerValue;
    // When SToSProducesPositivePointer is set, BUFFER_OFFSET are assumed to be zero,
    // so is that for any implicit argument 
    if (m_hasBufferOffsetArg && !isImplicitArg &&
        IGC_IS_FLAG_DISABLED(SToSProducesPositivePointer))
    {
        PointerValue = getBufferOffsetArg(F, argNumber);
        if (PointerValue == nullptr)
        {
            // Sanity check
            return false;
        }
    }
    else
    {
        // BUFFER_OFFSET are zero.
        PointerValue = ConstantInt::get(int32Ty, 0);
    }

    const int nGEPs = GEPs.size();

    // GEPs is in the reverse order of execution! The last GEP is the first
    // one to execute.  For example:
    //    %37 = getelementptr inbounds float, float addrspace(1)* %signalw, i64 16384
    //    %38 = bitcast float addrspace(1)* %37 to[16 x[32 x[32 x float]]] addrspace(1)*
    //    %39 = getelementptr inbounds[16 x[32 x[32 x float]]], [16 x[32 x[32 x float]]]
    //                        addrspace(1)* %38, i64 0, i64 % 34, i64 % 17, i64 % 18
    //    store float %36, float addrspace(1)* %39, align 4
    //
    //  GEPs = [%39, %37]  // GEPs[0] = %39, GEPs[1] = %37
    //
    for (int i = nGEPs; i > 0; --i)
    {
        GetElementPtrInst* GEP = GEPs[i-1];
        Value *PtrOp = GEP->getPointerOperand();
        PointerType *PtrTy = dyn_cast<PointerType>(PtrOp->getType());

        assert(PtrTy && "Only accept scalar pointer!");

        Type *Ty = PtrTy;
        gep_type_iterator GTI = gep_type_begin(GEP);
        for (auto OI = GEP->op_begin() + 1, E = GEP->op_end(); OI != E; ++OI, ++GTI)
        {
            Value *Idx = *OI;
            if (StructType *StTy = GTI.getStructTypeOrNull())
            {
                unsigned Field = int_cast<unsigned>(cast<ConstantInt>(Idx)->getZExtValue());
                if (Field)
                {
                    uint64_t Offset = DL->getStructLayout(StTy)->getElementOffset(Field);

                    Value *OffsetValue = ConstantInt::get(int32Ty, Offset);

                    PointerValue = BinaryOperator::CreateAdd(PointerValue, OffsetValue, "", GEP);
                }
                Ty = StTy->getElementType(Field);
            }
            else
            {
                Ty = GTI.getIndexedType();
                if (const ConstantInt *CI = dyn_cast<ConstantInt>(Idx))
                {
                    if (!CI->isZero())
                    {
                        uint64_t Offset = DL->getTypeAllocSize(Ty) * CI->getSExtValue();
                        Value *OffsetValue = ConstantInt::get(int32Ty, Offset);

                        PointerValue = BinaryOperator::CreateAdd(PointerValue, OffsetValue, "", GEP);
                    }
                }
                else
                {
                    Value *NewIdx = CastInst::CreateTruncOrBitCast(Idx, int32Ty, "", GEP);

                    APInt ElementSize = APInt(int32Ty->getPrimitiveSizeInBits(), DL->getTypeAllocSize(Ty));

                    if (ElementSize != 1)
                    {
                        NewIdx = BinaryOperator::CreateMul(NewIdx, ConstantInt::get(int32Ty, ElementSize), "", GEP);
                    }

                    PointerValue = BinaryOperator::CreateAdd(PointerValue, NewIdx, "", GEP);
                }
            }
        }
    }
    offset = PointerValue;
    return true;
}

bool StatelessToStatefull::pointerIsPositiveOffsetFromKernelArgument(
    Function* F,Value* V, Value*& offset, unsigned int& argNumber)
{
    AssumptionCache* AC = getAC(F);

    PointerType* ptrType = dyn_cast<PointerType>(V->getType());
    assert(ptrType && "Expected scalar Pointer (No support to vector of pointers");
    if (!ptrType || ( ptrType->getAddressSpace() != ADDRESS_SPACE_GLOBAL &&
        ptrType->getAddressSpace() != ADDRESS_SPACE_CONSTANT ) )
    {
        return false;
    }
    unsigned int ptrAS = ptrType->getAddressSpace();

    SmallVector<GetElementPtrInst*, 4> GEPs;
    Value* base = V->stripPointerCasts();

    // gep : the last gep of pointer address, null if no GEP at all.
    GetElementPtrInst* gep = nullptr;
    while (isa<GetElementPtrInst>(base)) {
        gep = static_cast<GetElementPtrInst*>(base);
        GEPs.push_back(gep);
        base = gep->getPointerOperand()->stripPointerCasts();
    }

    // stripPointerCasts might skip addrSpaceCast, thus check if AS is still
    // the original one. Also, if base is still instruction, skip.
    if (gep && cast<PointerType>(base->getType())->getAddressSpace() == ptrAS && !isa<Instruction>(base))
    {
        if (const KernelArg* arg = getKernelArg(base))
        {
            // base is the argument!
            argNumber = arg->getAssociatedArgNo();
            bool gepProducesPositivePointer = true;

            // If m_hasBufferOffsetArg is true, the offset argument is added to
            // the final offset, and the final offset must be positive. Thus
            // skip checking if an offset is positive.
            // 
            // Note that offset should be positive for any implicit ptr argument
            if (!arg->isImplicitArg() &&
                (!m_hasBufferOffsetArg || m_hasOptionalBufferOffsetArg) &&
                IGC_IS_FLAG_DISABLED(SToSProducesPositivePointer))
            {
                // [This is conservative path]
                // Need to verify if there is a negative offset,
                // If so, no stateful message is generated.
                for (int i=0, sz = GEPs.size(); i < sz; ++i)
                {
                    GetElementPtrInst* tgep = GEPs[i];
                    for (auto U = tgep->idx_begin(), E = tgep->idx_end(); U != E; ++U)
                    {
                        Value *Idx = U->get();
                        gepProducesPositivePointer &=
                            valueIsPositive(Idx, &(F->getParent()->getDataLayout()), AC);
                    }
                }

                if (m_hasOptionalBufferOffsetArg)
                {
                    updateArgInfo(arg, gepProducesPositivePointer);
                }
            }
            if ((gepProducesPositivePointer || m_hasBufferOffsetArg) &&
                getOffsetFromGEP(F, GEPs, argNumber, arg->isImplicitArg(), offset))
            {
                return true;
            }
        }
    }
   
    return false;
}

void StatelessToStatefull::visitCallInst(CallInst &I) 
{
    if (auto Inst = dyn_cast<GenIntrinsicInst>(&I))
    {
        if (Inst->getIntrinsicID() == GenISAIntrinsic::GenISA_simdBlockRead ||
            Inst->getIntrinsicID() == GenISAIntrinsic::GenISA_simdBlockWrite)
        {
            Module *M = Inst->getParent()->getParent()->getParent();
            Function* F = Inst->getParent()->getParent();
            const DebugLoc &DL = Inst->getDebugLoc();
            Type* int32Ty = Type::getInt32Ty(M->getContext());
            Value* ptr = Inst->getOperand(0);
            PointerType* ptrTy = dyn_cast<PointerType>(ptr->getType());
            // If not global/constant, skip.
            if (ptrTy->getPointerAddressSpace() != ADDRESS_SPACE_GLOBAL &&
                ptrTy->getPointerAddressSpace() != ADDRESS_SPACE_CONSTANT) {
                return;
            }

            Value* offset = nullptr;
            unsigned int baseArgNumber = 0;
            if (pointerIsPositiveOffsetFromKernelArgument(F, ptr, offset, baseArgNumber))
            {        
                ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
                FunctionMetaData *funcMD = &modMD->FuncMD[F];
                ResourceAllocMD *resAllocMD = &funcMD->resAllocMD;
                assert(resAllocMD->argAllocMDList.size() > 0 && "ArgAllocMDList is empty.");
                ArgAllocMD *argAlloc = &resAllocMD->argAllocMDList[baseArgNumber];
                
                Constant* resourceNumber = ConstantInt::get(int32Ty, argAlloc->indexType);
                unsigned addrSpace = EncodeAS4GFXResource(*resourceNumber, BufferType::UAV, 0);
                setPointerSizeTo32bit(addrSpace, I.getParent()->getParent()->getParent());

                if (Inst->getIntrinsicID() == GenISAIntrinsic::GenISA_simdBlockRead)
                {
                    PointerType* pTy = PointerType::get(Inst->getType(), addrSpace);
                    Instruction* pPtrToInt = IntToPtrInst::Create(Instruction::IntToPtr, offset, pTy, "", Inst);
                    Function    * simdMediaBlockReadFunc = GenISAIntrinsic::getDeclaration(
                        M,
                        GenISAIntrinsic::GenISA_simdBlockRead,
                        {Inst->getType(),pTy});
                    Instruction * simdMediaBlockRead = CallInst::Create(simdMediaBlockReadFunc, {pPtrToInt}, "", Inst);
                    simdMediaBlockRead->setDebugLoc(DL);
                    Inst->replaceAllUsesWith(simdMediaBlockRead);
                    Inst->eraseFromParent();
                }
                else
                {                
                    PointerType* pTy = PointerType::get(Inst->getOperand(1)->getType(), addrSpace);
                    Instruction* pPtrToInt = IntToPtrInst::Create(Instruction::IntToPtr, offset, pTy, "", Inst);
                    SmallVector<Value*, 2> args;
                    args.push_back(pPtrToInt);
                    args.push_back(Inst->getOperand(1));
                    Function    * simdMediaBlockWriteFunc = GenISAIntrinsic::getDeclaration(
                        M,
                        GenISAIntrinsic::GenISA_simdBlockWrite,
                        {pTy,Inst->getOperand(1)->getType()});
                    Instruction * simdMediaBlockWrite = CallInst::Create(simdMediaBlockWriteFunc, args, "", Inst);
                    simdMediaBlockWrite->setDebugLoc(DL);
                    Inst->replaceAllUsesWith(simdMediaBlockWrite);
                    Inst->eraseFromParent();
                }

                m_changed = true;
            }
        }
    }

}

void StatelessToStatefull::visitLoadInst(LoadInst &I)
{
    Module *M = I.getParent()->getParent()->getParent();
    Function* F = I.getParent()->getParent();
    const DebugLoc &DL = I.getDebugLoc();
    Type* int32Ty = Type::getInt32Ty(M->getContext());
    Value* ptr = I.getPointerOperand();

    Value* offset = nullptr;
    unsigned int baseArgNumber = 0;
    if(pointerIsPositiveOffsetFromKernelArgument(F, ptr, offset, baseArgNumber))
    {
        ModuleMetaData* modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
        FunctionMetaData *funcMD = &modMD->FuncMD[F];
        ResourceAllocMD *resAllocMD = &funcMD->resAllocMD;
        assert(resAllocMD->argAllocMDList.size() > 0 && "ArgAllocMDList is empty.");
        ArgAllocMD *argAlloc = &resAllocMD->argAllocMDList[baseArgNumber];

        Constant* resourceNumber = ConstantInt::get(int32Ty, argAlloc->indexType);

        unsigned addrSpace = EncodeAS4GFXResource(*resourceNumber, BufferType::UAV, 0);
        setPointerSizeTo32bit(addrSpace, I.getParent()->getParent()->getParent());

        PointerType* pTy = PointerType::get(I.getType(), addrSpace);

        Instruction* pPtrToInt = IntToPtrInst::Create(Instruction::IntToPtr, offset, pTy, "", &I);
        pPtrToInt->setDebugLoc(DL);

        Instruction* pLoad = new LoadInst(pPtrToInt, "", I.isVolatile(), I.getAlignment(), I.getOrdering(), IGCLLVM::getSyncScopeID(&I), &I);
        pLoad->setDebugLoc(DL);

        PointerType* ptrType = dyn_cast<PointerType>(ptr->getType());
        if (ptrType && ptrType->getAddressSpace() == ADDRESS_SPACE_CONSTANT)
        {
            LLVMContext& context = I.getContext();
            MDString* const metadataName = MDString::get(context, "invariant.load");
            MDNode* node = MDNode::get(context, metadataName);
            pLoad->setMetadata(LLVMContext::MD_invariant_load, node);
        }

        I.replaceAllUsesWith(pLoad);
        I.eraseFromParent();

        m_changed = true;
    }
}

void StatelessToStatefull::visitStoreInst(StoreInst &I)
{
    Module *M = I.getParent()->getParent()->getParent();
    Function* F = I.getParent()->getParent();
    const DebugLoc &DL = I.getDebugLoc();
    Type* int32Ty = Type::getInt32Ty(M->getContext());
    Value* ptr = I.getPointerOperand();

    Value* offset = nullptr;
    unsigned int baseArgNumber = 0;
    if(pointerIsPositiveOffsetFromKernelArgument(F, ptr, offset, baseArgNumber))
    {
        Value* dataVal = I.getOperand(0);

        if (dataVal != nullptr)
        {
            ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
            FunctionMetaData *funcMD = &modMD->FuncMD[F];
            ResourceAllocMD *resAllocMD = &funcMD->resAllocMD;
            assert(resAllocMD->argAllocMDList.size() > 0 && "ArgAllocMDList is empty.");
            ArgAllocMD *argAlloc = &resAllocMD->argAllocMDList[baseArgNumber];
            Constant* resourceNumber = ConstantInt::get(int32Ty, argAlloc->indexType);

            unsigned addrSpace = EncodeAS4GFXResource(*resourceNumber, BufferType::UAV, 0);
            setPointerSizeTo32bit(addrSpace, I.getParent()->getParent()->getParent());

            PointerType* pTy = PointerType::get(dataVal->getType(), addrSpace);

            Instruction* pPtrToInt = IntToPtrInst::Create(Instruction::IntToPtr, offset, pTy, "", &I);
            pPtrToInt->setDebugLoc(DL);

            Instruction* pStore = new StoreInst(dataVal, pPtrToInt, I.isVolatile(), I.getAlignment(), I.getOrdering(), IGCLLVM::getSyncScopeID(&I), &I);
            pStore->setDebugLoc(DL);

            I.eraseFromParent();

            m_changed = true;
        }
    }
}

CallInst* StatelessToStatefull::createBufferPtr(unsigned addrSpace, Constant* argNumber, Instruction* InsertBefore)
{
    Module *M = InsertBefore->getParent()->getParent()->getParent();

    Type* int32Ty = Type::getInt32Ty(M->getContext());

    Constant* bufTypeVal =  ConstantInt::get(int32Ty, (int)BufferType::UAV);

    PointerType *ptrTy = PointerType::get(int32Ty, addrSpace);
    Function* pFuncGetBufferPtr = GenISAIntrinsic::getDeclaration(
        M,
        GenISAIntrinsic::GenISA_GetBufferPtr, ptrTy);

    Value* getBufferPtrArgs[] =
    {
        argNumber,
        bufTypeVal
    };

    CallInst* pBufferPtrInst = CallInst::Create(pFuncGetBufferPtr, getBufferPtrArgs, InsertBefore->getName(), InsertBefore);

    return pBufferPtrInst;
}

// This is used to set the size for a pointer to a given addrspace, which is created
// and used by and within IGC. As this is a new address space,  all the existing ones
// will not be affected by this at all.  (And it definitely does not change any existing
// memory layout.)
//
// Note this is consistent with CodeGenContext::getRegisterPointerSizeInBits() for now.
void StatelessToStatefull::setPointerSizeTo32bit(int32_t AddrSpace, Module* M)
{
    const DataLayout &DL = M->getDataLayout();

    // If default is 32bit (or it has been set to 32bit already), no need to set it.
    if (DL.getPointerSize(AddrSpace) == 4)
    {
        // Already 4 bytes,
        return;
    }

    const std::string StrDL = DL.getStringRepresentation();
    char data[64];
    if (DL.isDefault())
    {
        sprintf_s(data, sizeof(data), "p%d:32:32:32", AddrSpace);
    }
    else
    {
        // this is a new addrspace, it should not be in the 
        // existing DataLayout, but if it exists, just return.
        // We don't want to change any existing one!
        sprintf_s(data, sizeof(data), "p%d:", AddrSpace);
        if (StrDL.find(data) != std::string::npos)
        {
            return;
        }
        sprintf_s(data, sizeof(data), "-p%d:32:32:32", AddrSpace);
    }

    std::string newStrDL = StrDL + data;
    M->setDataLayout(newStrDL);    
}

void StatelessToStatefull::updateArgInfo(
    const KernelArg *kernelArg, bool isPositive)
{
    auto II = m_argsInfo.find(kernelArg);
    if (II == m_argsInfo.end())
    {
        m_argsInfo[kernelArg] = 1; // default to true
    }
    if (!isPositive)
    {
        m_argsInfo[kernelArg] = 0;
    }
}

void StatelessToStatefull::finalizeArgInitialValue(Function *F)
{
    if (!m_hasOptionalBufferOffsetArg)
    {
        return;
    }

    Module *M = F->getParent();
    Type* int32Ty = Type::getInt32Ty(M->getContext());
    Value *ZeroValue = ConstantInt::get(int32Ty, 0);

    for (auto II : m_argsInfo)
    {
        const KernelArg* kernelArg = II.first;
        int mapVal = II.second;
        bool allOffsetPositive = (mapVal == 1);
        if (allOffsetPositive)
        {
            const KernelArg *offsetArg = getBufferOffsetKernelArg(kernelArg);
            assert(offsetArg && "Missing BufferOffset arg!");
            Value *BufferOffsetArg = const_cast<Argument *>(offsetArg->getArg());
            BufferOffsetArg->replaceAllUsesWith(ZeroValue);
        }
    }
    m_argsInfo.clear();
}
