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

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/LocalBuffers/InlineLocalsResolution.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/DebugInfo/DebugInfoUtils.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;


// Register pass to igc-opt
#define PASS_FLAG "igc-resolve-inline-locals"
#define PASS_DESCRIPTION "Resolve inline local variables/buffers"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(InlineLocalsResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(InlineLocalsResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char InlineLocalsResolution::ID = 0;
const llvm::StringRef BUILTIN_MEMPOOL = "__builtin_IB_AllocLocalMemPool";

InlineLocalsResolution::InlineLocalsResolution() : 
    ModulePass(ID), m_pGV(nullptr)
{
    initializeInlineLocalsResolutionPass(*PassRegistry::getPassRegistry());
}

const unsigned int InlineLocalsResolution::VALID_LOCAL_HIGH_BITS = 0x10000000;

static bool useAsPointerOnly(Value *V) {
    assert(V->getType()->isPointerTy() && "Expect the input value is a pointer!");

    SmallSet<PHINode *, 8> VisitedPHIs;
    SmallVector<Value *, 16> WorkList;
    WorkList.push_back(V);

    StoreInst *ST = nullptr;
    PHINode *PN = nullptr;
    while (!WorkList.empty()) {
        Value *Val = WorkList.pop_back_val();
        for (auto *U : Val->users()) {
            Operator *Op = dyn_cast<Operator>(U);
            if (!Op)
                continue;
            switch (Op->getOpcode()) {
            default:
                // Bail out for unknown operations.
                return false;
            case Instruction::Store:
                ST = cast<StoreInst>(U);
                // Bail out if it's used as the value operand.
                if (ST->getValueOperand() == Val)
                    return false;
                // FALL THROUGH
            case Instruction::Load:
                // Safe use in LD/ST as pointer only.
                continue;
            case Instruction::PHI:
                PN = cast<PHINode>(U);
                // Skip if it's already visited.
                if (VisitedPHIs.count(PN))
                    continue;
                VisitedPHIs.insert(PN);
                // FALL THROUGH
            case Instruction::BitCast:
            case Instruction::Select:
            case Instruction::GetElementPtr:
                // Need to check their usage further.
                break;
            }
            WorkList.push_back(U);
        }
    }

    return true;
}

bool InlineLocalsResolution::runOnModule(Module &M)
{
    MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    // Compute the offset of each inline local in the kernel,
    // and their total size.
    std::map<Function*, unsigned int> sizeMap;
    collectInfoOnSharedLocalMem(M);
    computeOffsetList(M, sizeMap);

    LLVMContext& C = M.getContext();

    for( Module::iterator I = M.begin(), E = M.end(); I != E; ++I )
    {
        Function* pFunc = &(*I);

        if (pFunc->isDeclaration() || !isEntryFunc(pMdUtils, pFunc))
        {
            continue;
        }

        unsigned int totalSize = 0;

        // Get the offset at which local arguments start
        auto sizeIter = sizeMap.find(pFunc);
        if( sizeIter != sizeMap.end() )
        {
            totalSize += sizeIter->second;
        }

        // Set the high 16 bits to a non-0 value.
        totalSize = (totalSize & 0xFFFF);

        bool IsFirstSLMArgument = true;
        for( Function::arg_iterator A = pFunc->arg_begin(), AE = pFunc->arg_end(); A != AE; ++A )
        {
            Argument* arg = &(*A);
            PointerType* ptrType = dyn_cast<PointerType>(arg->getType());
            // Check that this is a pointer
            if( !ptrType )
            {
                continue;
            }

            // To the local address space
            if( ptrType->getAddressSpace() != ADDRESS_SPACE_LOCAL )
            {
                continue;
            }

            // Which is used
            if( arg->use_empty() )
            {
                continue;
            }

            bool UseAsPointerOnly = useAsPointerOnly(arg);
            unsigned Offset = totalSize;
            if (!UseAsPointerOnly)
                Offset |= VALID_LOCAL_HIGH_BITS;

            if (IsFirstSLMArgument) {
                auto BufType = ArrayType::get(Type::getInt8Ty(M.getContext()), 0);
                auto ExtSLM = new GlobalVariable(M, BufType, false, GlobalVariable::ExternalLinkage, nullptr,
                    pFunc->getName() + "-ExtSLM", nullptr, GlobalVariable::ThreadLocalMode::NotThreadLocal,
                    ADDRESS_SPACE_LOCAL);
                auto NewPtr = ConstantExpr::getBitCast(ExtSLM, arg->getType());
                arg->replaceAllUsesWith(NewPtr);
                // Update MD.
                LocalOffsetMD localOffset;
                localOffset.m_Var = ExtSLM;
                localOffset.m_Offset = Offset;
                modMD->FuncMD[pFunc].localOffsets.push_back(localOffset);

                IGC::appendToUsed(M, ExtSLM);
                IsFirstSLMArgument = false;
            } else {
                // FIXME: The following code should be removed as well by
                // populating similar adjustment in prolog during code
                // emission.
                // Ok, now we need to add an offset, in bytes, which is equal to totalSize.
                // Bitcast to i8*, GEP, bitcast back to original type.
                Value* sizeConstant = ConstantInt::get(Type::getInt32Ty(C), Offset);
                SmallVector<Value*, 1> idx(1, sizeConstant);
                Instruction* pInsertBefore = &(*pFunc->begin()->getFirstInsertionPt());
                Type* pLocalCharPtrType = Type::getInt8Ty(C)->getPointerTo(ADDRESS_SPACE_LOCAL);
                Instruction* pCharPtr = BitCastInst::CreatePointerCast(arg, pLocalCharPtrType, "localToChar", pInsertBefore);
                Value* pMovedCharPtr = GetElementPtrInst::Create(nullptr, pCharPtr, idx, "movedLocal", pInsertBefore);

                Value* pMovedPtr = CastInst::CreatePointerCast(pMovedCharPtr, ptrType, "charToLocal", pInsertBefore);
                
                // Running over arg users and use replaceUsesOfWith to fix them is not enough,
                // because it does not cover the usage of arg in metadata (e.g. for debug info intrinsic).
                // Thus, replaceAllUsesWith should be used in order to fix also debug info.
                arg->replaceAllUsesWith(pMovedPtr);
                // The above operation changed also the "arg" operand in "charPtr" to "movedPtr"
                // Thus, we need to fix it back (otherwise the LLVM IR will be invalid)
                pCharPtr->replaceUsesOfWith(pMovedPtr, arg);
            }
        }
    }

    return true;
}

void InlineLocalsResolution::collectInfoOnSharedLocalMem(Module& M)
{

    // first we collect SLM usage on GET_MEMPOOL_PTR
    if (M.getFunction(BUILTIN_MEMPOOL) != nullptr)
    {
        const auto pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        const GT_SYSTEM_INFO platform = pCtx->platform.GetGTSystemInfo();

        SmallVector<CallInst*, 8> callsToReplace;
        unsigned maxBytesOnModule = 0;
        unsigned maxAlignOnModule = 0;

        // scan inst to collect all call instructions

        for (Module::iterator F = M.begin(), FE = M.end(); F != FE; ++F)
        {
            if (F->isDeclaration())
            {
                continue;
            }

            unsigned maxBytesOnFunc = 0;
            for (auto I = inst_begin(&(*F)), IE = inst_end(&(*F)); I != IE; ++I)
            {
                Instruction* inst = &(*I);
                if (CallInst *CI = dyn_cast<CallInst>(inst))
                {
                    Function *pFunc = CI->getCalledFunction();
                    if (pFunc && pFunc->getName().equals(BUILTIN_MEMPOOL))
                    {
                        // should always be called with constant operands
                        assert(isa<ConstantInt>(CI->getArgOperand(0)));
                        assert(isa<ConstantInt>(CI->getArgOperand(1)));
                        assert(isa<ConstantInt>(CI->getArgOperand(2)));

                        const unsigned int allocAllWorkgroups = unsigned(cast<ConstantInt>(CI->getArgOperand(0))->getZExtValue());
                        const unsigned int numAdditionalElements = unsigned(cast<ConstantInt>(CI->getArgOperand(1))->getZExtValue());
                        const unsigned int elementSize = unsigned(cast<ConstantInt>(CI->getArgOperand(2))->getZExtValue());

                        unsigned int numElements = numAdditionalElements;
                        if (allocAllWorkgroups)
                        {
                            if (platform.ThreadCount > 448)
                            {
                                numElements += platform.ThreadCount;
                            }
                            else
                            {
                                numElements += 448;
                            }
                        }
                        const unsigned int size = numElements * elementSize;
                        const unsigned int align = elementSize;

                        maxBytesOnFunc = std::max(maxBytesOnFunc, size);
                        maxBytesOnModule = std::max(maxBytesOnModule, size);
                        maxAlignOnModule = std::max(maxAlignOnModule, align);

                        callsToReplace.push_back(CI);
                    }
                }
            }
            if (maxBytesOnFunc != 0)
            {
                m_FuncToMemPoolSizeMap[&(*F)] = maxBytesOnFunc;
            }
        }

        if (!callsToReplace.empty())
        {

            Type *bufType = ArrayType::get(Type::getInt8Ty(M.getContext()), uint64_t(maxBytesOnModule));

            m_pGV = new GlobalVariable(M, bufType, false,
                GlobalVariable::ExternalLinkage, ConstantAggregateZero::get(bufType),
                "GenSLM.LocalMemPoolOnGetMemPoolPtr",
                nullptr,
                GlobalVariable::ThreadLocalMode::NotThreadLocal,
                ADDRESS_SPACE_LOCAL);

            m_pGV->setAlignment(maxAlignOnModule);

            for (auto call : callsToReplace)
            {
                CastInst *cast =
                    new BitCastInst(
                    m_pGV,
                    call->getCalledFunction()->getReturnType(),
                    "mempoolcast",
                    call);

                cast->setDebugLoc(call->getDebugLoc());

                call->replaceAllUsesWith(cast);
                call->eraseFromParent();
            }
        }
    }

    // let's loop all global variables
    for (Module::global_iterator I = M.global_begin(), E = M.global_end(); I != E; ++I)
    {
        // We only care about global variables, not other globals.
        GlobalVariable* globalVar = dyn_cast<GlobalVariable>(&*I);
        if (!globalVar)
        {
            continue;
        }

        PointerType* ptrType = dyn_cast<PointerType>(globalVar->getType());
        assert(ptrType && "The type of a global variable must be a pointer type");
        if (!ptrType)
        {
            continue;
        }

        // We only care about local address space here.
        if (ptrType->getAddressSpace() != ADDRESS_SPACE_LOCAL)
        {
            continue;
        }

        // For each SLM buffer, make it external and disable dso local
        // to avoid alignment changing by llvm.
        globalVar->setLinkage(GlobalValue::ExternalLinkage);
        globalVar->setDSOLocal(false);

        // Find the functions which this globalVar belongs to.... 
        for (Value::user_iterator U = globalVar->user_begin(), UE = globalVar->user_end(); U != UE; ++U)
        {
            Instruction* user = dyn_cast<Instruction>(*U);
            if (!user)
            {
                continue;
            }

            m_FuncToVarsMap[user->getParent()->getParent()].insert(globalVar);
        }
    }

    // set debugging info, and insert mov inst.
    IF_DEBUG_INFO(for (auto I : m_FuncToVarsMap))
    {
        IF_DEBUG_INFO(Function *userFunc = I.first;)
        IF_DEBUG_INFO(for (auto G : I.second))
        {
            IF_DEBUG_INFO(Instruction* pInsertBefore = &(*userFunc->begin()->getFirstInsertionPt());)
            TODO("Should inline local buffer points to origin offset 'globalVar' or to fixed offset 'pMovedPtr'?");
            IF_DEBUG_INFO(DebugInfoUtils::UpdateGlobalVarDebugInfo(G, G, pInsertBefore, true););
        }
    }
}

void InlineLocalsResolution::computeOffsetList(Module& M, std::map<Function*, unsigned int>& sizeMap)
{
    std::map<Function*, std::map<GlobalVariable*, unsigned int>> offsetMap;
    MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    DataLayout DL = M.getDataLayout();
    CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();

    if (m_FuncToVarsMap.empty())
    {
        return;
    }

    // let's travese the CallGraph to calculate the local 
    // variables of kernel from all user functions.
    m_chkSet.clear();
    for (auto &N : CG)
    {
        Function *f = N.second->getFunction();
        if (!f || f->isDeclaration() || m_chkSet.find(f) != m_chkSet.end()) continue;
        traveseCGN(*N.second);
    }

    // set up the offsetMap;
    for (auto I : m_FuncToVarsMap)
    {
        Function* F = I.first;

        // loop all global variables
        for (auto G : I.second)
        {
            // std::map initializes with zero if the value is not present.
            unsigned int offset = sizeMap[F];
            offset = iSTD::Align(offset, DL.getPreferredAlignment(G));

            // Save the offset of the current local
            // (set the high bits to be non-0 here too)
            offsetMap[F][G] = (offset & 0xFFFF);

            // And the total size after this local is added
            PointerType* ptrType = dyn_cast<PointerType>(G->getType());
            Type* varType = ptrType->getElementType();
            if (G == m_pGV)
            {
                // it is GetMemPoolPtr usage
                offset += m_FuncToMemPoolSizeMap[F];
            }
            else
            {
                offset += (unsigned int)DL.getTypeAllocSize(varType);
            }
            sizeMap[F] = offset;
        }
    }

    // Ok, we've collected the information, now write it into the MD.
    for (auto iter = sizeMap.begin(), end = sizeMap.end(); iter != end; ++iter)
    {
        // ignore non-entry functions. 
        if (!isEntryFunc(pMdUtils, iter->first))
        {
            continue;
        }

        // If this function doesn't have any locals, no need for MD.
        if (iter->second == 0)
        {
            continue;
        }

        // We need the total size to have at least 32-byte alignment.
        // This is because right after the space allocated to the inline locals,
        // we are going to have inline parameters. So, we need to make sure the
        // first local parameter is appropriately aligned, which, at worst,
        // can be 256 bits.
        iter->second = iSTD::Align(iter->second, 32);

        // Add the size information of this function
        modMD->FuncMD[iter->first].localSize = iter->second;

        // And now the offsets.
        for (auto offsetIter = offsetMap[iter->first].begin(), offsetEnd = offsetMap[iter->first].end(); offsetIter != offsetEnd; ++offsetIter)
        {
            unsigned Offset = offsetIter->second;
            if (!useAsPointerOnly(offsetIter->first))
                Offset |= VALID_LOCAL_HIGH_BITS;
            
            LocalOffsetMD localOffset;
            localOffset.m_Var = offsetIter->first;
            localOffset.m_Offset = Offset;
            modMD->FuncMD[iter->first].localOffsets.push_back(localOffset);
        }
    }
    pMdUtils->save(M.getContext());
}

void InlineLocalsResolution::traveseCGN(llvm::CallGraphNode &CGN)
{
    Function * f = CGN.getFunction();

    for (auto N : CGN)
    {
        Function *sub = N.second->getFunction();
        if (!sub || sub->isDeclaration()) continue;

        // we reach here, because there is sub-function inside the node
        if (m_chkSet.find(sub) == m_chkSet.end())
        {
            // this sub-routine is not visited before.
            // visit it first
            traveseCGN(*N.second);
        }

        // the sub-routine was visited before, collect information

        // count each global on this sub-routine
        GlobalVariableSet &GS_f = m_FuncToVarsMap[f];
        GlobalVariableSet &GS_sub = m_FuncToVarsMap[sub];
        for (auto I = GS_sub.begin(); I != GS_sub.end(); ++I)
        {
            GS_f.insert(*I);
        }

        // automatic storages
        if (m_FuncToMemPoolSizeMap.find(sub) != m_FuncToMemPoolSizeMap.end())
        {
            // this sub-function has automatic storage
            if (m_FuncToMemPoolSizeMap.find(f) != m_FuncToMemPoolSizeMap.end())
            {
                // caller has its own memory pool size, choose the max  
                m_FuncToMemPoolSizeMap[f] = std::max(m_FuncToMemPoolSizeMap[f], m_FuncToMemPoolSizeMap[sub]);
            }
            else
            {
                m_FuncToMemPoolSizeMap[f] = m_FuncToMemPoolSizeMap[sub];
            }
        }
    }

    // mark this function

    m_chkSet.insert(f);
}
