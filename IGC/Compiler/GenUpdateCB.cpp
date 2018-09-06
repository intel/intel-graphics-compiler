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

#include "Compiler/GenUpdateCB.h"
#include "Compiler/CISACodeGen/helper.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include <iStdLib/MemCopy.h>

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/MemoryBuffer.h>
#include "common/LLVMWarningsPop.hpp"


char IGC::GenUpdateCB::ID = 0;

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
IGC_INITIALIZE_PASS_BEGIN(GenUpdateCB, "GenUpdateCB", "GenUpdateCB", false, false)
IGC_INITIALIZE_PASS_END(GenUpdateCB, "GenUpdateCB", "GenUpdateCB", false, false)

bool GenUpdateCB::isConstantBufferLoad(LoadInst* inst, unsigned &bufId)
{
    if (!inst)
        return false;

    m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    ModuleMetaData *modMD = m_ctx->getModuleMetaData();
    unsigned as = inst->getPointerAddressSpace();
    bool directBuf;
    BufferType bufType = IGC::DecodeAS4GFXResource(as, directBuf, bufId);
    if (bufType == CONSTANT_BUFFER && directBuf && bufId<15 && bufId != modMD->pushInfo.inlineConstantBufferSlot)
    {
        if (IntToPtrInst *itop = dyn_cast<IntToPtrInst>(inst->getOperand(0)))
        {
            if (!dyn_cast<Constant>(itop->getOperand(0)))
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool GenUpdateCB::allSrcConstantOrImm(Instruction* inst)
{
    uint i = 0;
    for (i = 0; i < inst->getNumOperands(); i++)
    {
        if (dyn_cast<Constant>(inst->getOperand(i)))
        {
            continue;
        }

        LoadInst* loadInst = llvm::dyn_cast<llvm::LoadInst>(inst->getOperand(i));
        unsigned bufId = 0;

        if (loadInst && isConstantBufferLoad(loadInst, bufId))
        {
            continue;
        }

        break;
    }
    return (i == inst->getNumOperands());
}

bool GenUpdateCB::updateCbAllowedInst(Instruction* inst)
{
    if (!inst)
        return false;

    switch (inst->getOpcode())
    {
    case Instruction::Add:
    case Instruction::FAdd:
    case Instruction::Sub:
    case Instruction::FSub:
    case Instruction::Mul:
    case Instruction::FMul:
    case Instruction::UDiv:
    case Instruction::SDiv:
    case Instruction::FDiv:
    case Instruction::URem:
    case Instruction::SRem:
    case Instruction::Shl:
    case Instruction::LShr:
    case Instruction::AShr:
    case Instruction::And:
    case Instruction::Or:
    case Instruction::Xor:
        return true;
    default:;
    }

    if (CallInst *callI = dyn_cast<CallInst>(inst))
    {
        ConstantFP *C0 = dyn_cast<ConstantFP>(inst->getOperand(0));
        switch (GetOpCode(callI))
        {
        case llvm_log:
        case llvm_sqrt:
            if (C0 && C0->getValueAPF().convertToFloat() > 0)
            {
                return true;
            }
            break;
        case llvm_pow:
        case llvm_cos:
        case llvm_sin:
        case llvm_exp:
        case llvm_floor:
        case llvm_ceil:
        case llvm_fabs:
        case llvm_max:
        case llvm_min:
        case llvm_rsq:
        case llvm_fsat:
            return true;
        default:
            return false;
        }
    }
    return false;
}

void GenUpdateCB::InsertInstTree(Instruction *inst, Instruction *pos)
{
    if (!inst || dyn_cast<Constant>(inst) || vmap[inst])
    {
        return;
    }

    unsigned bufId = 0;
    if (isConstantBufferLoad(dyn_cast<LoadInst>(inst), bufId))
    {
        Instruction *Clone = inst->clone();
        vmap[inst] = Clone;
        Clone->insertBefore(pos);
        m_ConstantBufferUsageMask |= (1 << bufId);
        return;
    }

    for (uint i = 0; i < inst->getNumOperands(); i++)
    {
        InsertInstTree(dyn_cast<Instruction>(inst->getOperand(i)), pos);
    }

    Instruction *Clone = inst->clone();
    vmap[inst] = Clone;

    for (uint i = 0; i < Clone->getNumOperands(); i++)
    {
        if (vmap[inst->getOperand(i)])
        {
            Clone->setOperand(i, vmap[inst->getOperand(i)]);
        }
    }
    Clone->insertBefore(pos);

    // update declare
    llvm::Function *pfunc = nullptr;
    if (CallInst *callI = dyn_cast<CallInst>(Clone))
    {
        switch (GetOpCode(callI))
        {
        case llvm_log:
            pfunc = llvm::Intrinsic::getDeclaration(m_ConstantBufferReplaceShaderPatterns,
                Intrinsic::log2,
                llvm::ArrayRef<llvm::Type*>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
            break;
        case llvm_sqrt:
            pfunc = llvm::Intrinsic::getDeclaration(m_ConstantBufferReplaceShaderPatterns,
                Intrinsic::sqrt,
                llvm::ArrayRef<llvm::Type*>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
            break;
        case llvm_pow:
            pfunc = llvm::Intrinsic::getDeclaration(m_ConstantBufferReplaceShaderPatterns,
                Intrinsic::pow,
                llvm::ArrayRef<llvm::Type*>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
            break;
        case llvm_cos:
            pfunc = llvm::Intrinsic::getDeclaration(m_ConstantBufferReplaceShaderPatterns,
                Intrinsic::cos,
                llvm::ArrayRef<llvm::Type*>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
            break;
        case llvm_sin:
            pfunc = llvm::Intrinsic::getDeclaration(m_ConstantBufferReplaceShaderPatterns,
                Intrinsic::sin,
                llvm::ArrayRef<llvm::Type*>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
            break;
        case llvm_exp:
            pfunc = llvm::Intrinsic::getDeclaration(m_ConstantBufferReplaceShaderPatterns,
                Intrinsic::exp2,
                llvm::ArrayRef<llvm::Type*>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
            break;

        case llvm_floor:
            pfunc = llvm::Intrinsic::getDeclaration(m_ConstantBufferReplaceShaderPatterns,
                Intrinsic::floor,
                llvm::ArrayRef<llvm::Type*>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
            break;
        case llvm_ceil:
            pfunc = llvm::Intrinsic::getDeclaration(m_ConstantBufferReplaceShaderPatterns,
                Intrinsic::ceil,
                llvm::ArrayRef<llvm::Type*>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
            break;
        case llvm_fabs:
            pfunc = llvm::Intrinsic::getDeclaration(m_ConstantBufferReplaceShaderPatterns,
                Intrinsic::fabs,
                llvm::ArrayRef<llvm::Type*>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
            break;
        case llvm_max:
            pfunc = llvm::Intrinsic::getDeclaration(m_ConstantBufferReplaceShaderPatterns,
                Intrinsic::maxnum,
                llvm::ArrayRef<llvm::Type*>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
            break;
        case llvm_min:
            pfunc = llvm::Intrinsic::getDeclaration(m_ConstantBufferReplaceShaderPatterns,
                Intrinsic::minnum,
                llvm::ArrayRef<llvm::Type*>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
            break;
        case llvm_rsq:
            pfunc = llvm::GenISAIntrinsic::getDeclaration(m_ConstantBufferReplaceShaderPatterns,
                GenISAIntrinsic::GenISA_rsq,
                llvm::ArrayRef<llvm::Type*>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
            break;
        case llvm_fsat:
            pfunc = llvm::GenISAIntrinsic::getDeclaration(m_ConstantBufferReplaceShaderPatterns,
                GenISAIntrinsic::GenISA_fsat,
                llvm::ArrayRef<llvm::Type*>(Type::getFloatTy(m_ConstantBufferReplaceShaderPatterns->getContext())));
            break;
        default:
            assert(0 && "Intrinsic not supported");
        }
        callI->setCalledFunction(pfunc);
    }
}

Instruction* GenUpdateCB::CreateModule(Module* M)
{
    llvm::IRBuilder<> cb_builder(M->getContext());
    m_ConstantBufferReplaceShaderPatterns = new Module("CB", M->getContext());
    m_ConstantBufferReplaceShaderPatterns->setDataLayout(M->getDataLayout());
    Function* entry = Function::Create(FunctionType::get(cb_builder.getVoidTy(), false),
        GlobalValue::ExternalLinkage,
        "CBEntry",
        m_ConstantBufferReplaceShaderPatterns);
    BasicBlock* pEntryBlock = BasicBlock::Create(M->getContext(), "entry", entry);
    cb_builder.SetInsertPoint(pEntryBlock);

    Instruction* ret = cb_builder.CreateRetVoid();

    cb_builder.SetInsertPoint(ret);

    return ret;
}

bool GenUpdateCB::runOnFunction(Function &F)
{
    m_CbUpdateMap.clear();
    bool foundCases = false;

    // travel through instructions and mark the ones that are calculated with CB or imm
    DominatorTree &dom_tree = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

    bool hasChange = true;
    while (hasChange)
    {
        hasChange = false;
        for (df_iterator<DomTreeNode*> DI = df_begin(dom_tree.getRootNode()),
            dom_end = df_end(dom_tree.getRootNode()); DI != dom_end; ++DI)
        {
            BasicBlock* BB = DI->getBlock();
            for (auto BI = BB->begin(), BE = BB->end(); BI != BE;)
            {
                Instruction *inst = llvm::dyn_cast<Instruction>(&(*BI++));
                unsigned bufId = 0;

                if (m_CbUpdateMap.count(inst) != 0)
                {
                    continue;
                }
                else if (isConstantBufferLoad(dyn_cast<LoadInst>(inst), bufId))
                {
                    m_CbUpdateMap.insert(inst);
                }
                else
                {
                    bool foundNewInst = 0;
                    for (uint i = 0; i < inst->getNumOperands(); i++)
                    {
                        if (dyn_cast<Constant>(inst->getOperand(i)))
                        {
                            ;
                        }
                        else if (!updateCbAllowedInst(inst) ||
                            m_CbUpdateMap.count(inst->getOperand(i)) == 0)
                        {
                            foundNewInst = 0;
                            break;
                        }
                        else
                        {
                            foundNewInst = 1;
                        }
                    }

                    if (foundNewInst && m_CbUpdateMap.count(inst) == 0)
                    {
                        hasChange = true;
                        foundCases = true;
                        m_CbUpdateMap.insert(inst);
                    }
                }
            }
        }
    }

    // look for cases to create mini-shader
    uint counter = 0;
    m_maxCBcases = 1;
    if (foundCases)
    {
        Instruction *ret = nullptr;
        llvm::IRBuilder<> orig_builder(F.getContext());

        for (auto iter = m_CbUpdateMap.begin(); iter != m_CbUpdateMap.end(); iter++)
        {
            if (counter >= m_maxCBcases)
            {
                break;
            }

            Instruction *inst = llvm::dyn_cast<Instruction>(*iter);
            unsigned bufId = 0;
            bool lastInstUsed = false;

            // !allSrcConstantOrImm() check skips the simple cases so it doesn't get triggered too many times.
            if (!isConstantBufferLoad(dyn_cast<LoadInst>(inst), bufId) &&
                !allSrcConstantOrImm(inst) &&
                inst->getType()->getScalarSizeInBits() == 32 &&
                inst->getType()->isFloatTy()) // last check needs to be removed to enable integer cases
            {
                for (auto nextInst = inst->user_begin(); nextInst != inst->user_end(); nextInst++)
                {
                    if (m_CbUpdateMap.count(*nextInst) == 0)
                    {
                        lastInstUsed = true;
                        break;
                    }
                }
            }

            if (lastInstUsed)
            {
                if (!m_ConstantBufferReplaceShaderPatterns)
                {
                    ret = CreateModule(F.getParent());
                }
                llvm::IRBuilder<> cb_builder(m_ConstantBufferReplaceShaderPatterns->getContext());

                // add inst and its sources to the CB mini shader
                AllocaInst* storeAlloca = cb_builder.CreateAlloca(inst->getType());
                StoreInst *store = cb_builder.CreateStore(inst, storeAlloca);
                storeAlloca->insertBefore(ret);
                store->insertBefore(ret);
                InsertInstTree(inst, store);
                store->setOperand(0, vmap[inst]);

                // replace original shader with read from runtime
                llvm::Function* runtimeFunc = llvm::GenISAIntrinsic::getDeclaration(F.getParent(), GenISAIntrinsic::GenISA_RuntimeValue);
                Instruction* pValue = orig_builder.CreateCall(runtimeFunc, orig_builder.getInt32(counter));
                pValue->insertBefore(inst);

                if (inst->getType()->isIntegerTy())
                {
                    pValue = llvm::cast<llvm::Instruction>(orig_builder.CreateBitCast(pValue, orig_builder.getInt32Ty()));
                    pValue->insertBefore(inst);
                }

                inst->replaceAllUsesWith(pValue);
                counter++;
            }
        }

        if (m_ConstantBufferReplaceShaderPatterns)
        {
            // write the minishader Module to memory
            llvm::SmallVector<char, 4> bitcodeSV;
            llvm::raw_svector_ostream bitcodeSS(bitcodeSV);
            llvm::WriteBitcodeToFile(m_ConstantBufferReplaceShaderPatterns, bitcodeSS);

            size_t bufferSize = bitcodeSS.str().size();
            void* CBPatterns = aligned_malloc(bufferSize, 16);

            iSTD::MemCopy(
                CBPatterns,
                const_cast<char*>(bitcodeSS.str().data()),
                bufferSize);

            // return 
            m_ctx->m_ConstantBufferReplaceShaderPatterns = CBPatterns;
            m_ctx->m_ConstantBufferReplaceShaderPatternsSize = bufferSize;
            m_ctx->m_ConstantBufferUsageMask = m_ConstantBufferUsageMask;
            m_ctx->m_ConstantBufferReplaceSize = iSTD::Align(counter, 8)/8;
        }
    }

    return false;
}

namespace IGC
{
    union
    {
        float f;
        int i;
        uint u;
    } ftod0, ftod1, ftodTemp;

    uint lookupValue(Value* op, DenseMap<Value*, uint> &CalculatedValue)
    {

        if (ConstantFP *c = dyn_cast<ConstantFP>(op))
        {
            float floatValue = c->getValueAPF().convertToFloat();
            ftodTemp.f = floatValue;
            return ftodTemp.u;
        }
        else if (ConstantInt *c = dyn_cast<ConstantInt>(op))
        {
            ftodTemp.i = (int)(c->getSExtValue());
            return ftodTemp.u;
        }
        else
        {
            if (CalculatedValue.find(op) == CalculatedValue.end())
            {
                assert(0 && "can't find matching cb value");
            }
            return CalculatedValue[op];
        }
        return 0;
    }

    void ConstantFolder(
        char*       bitcode,
        uint        bitcodeSize,
        void*       CBptr[15],
        uint*       pNewCB)
    {
        // load module from memory
        llvm::Module* M = NULL;

        llvm::StringRef bitRef(bitcode, bitcodeSize);
        std::unique_ptr<llvm::MemoryBuffer> bitcodeMem =
            llvm::MemoryBuffer::getMemBuffer(bitRef, "", /* Null Term  = */ false);

        bool isBitCode = llvm::isBitcode(
            (const unsigned char*)bitcodeMem->getBufferStart(),
            (const unsigned char*)bitcodeMem->getBufferEnd());

        LLVMContext context;
        if (isBitCode)
        {

			llvm::Expected<std::unique_ptr<llvm::Module>>  ModuleOrErr =
                llvm::parseBitcodeFile(bitcodeMem->getMemBufferRef(), context);

            if (llvm::Error EC = ModuleOrErr.takeError())
            {
                assert(0 && "parsing bitcode failed");
            }
            else
            {
                M = ModuleOrErr.get().release();
            }
        }
        else
        {
            assert(0 && "parsing bitcode failed");
        }

        // start constant folding
        DenseMap<Value*, uint> CalculatedValue;
        int newCBIndex = 0;
        BasicBlock *BB = &M->getFunctionList().begin()->getEntryBlock();
        for (auto II = BB->begin(), IE = BB->end(); II != IE; ++II)
        {
            Instruction *inst = &(*II);
            if (dyn_cast<AllocaInst>(inst) || dyn_cast<ReturnInst>(inst))
            {
                continue;
            }
            else if (LoadInst *ld = dyn_cast<LoadInst>(inst))
            {
                bool directBuf;
                unsigned bufId;
                IGC::DecodeAS4GFXResource(ld->getPointerAddressSpace(), directBuf, bufId);
                int offset = IGC::getConstantBufferLoadOffset(ld);
                CalculatedValue[ld] = (uint)(*(DWORD*)((char*)CBptr[bufId] + offset));
            }
            else if (StoreInst *store = dyn_cast<StoreInst>(inst))
            {
                pNewCB[newCBIndex] = (uint)(CalculatedValue[inst->getOperand(0)]);
                newCBIndex++;
            }
            else
            {
                ftod0.u = lookupValue(inst->getOperand(0), CalculatedValue);

                if (CallInst *callI = dyn_cast<CallInst>(inst))
                {
                    switch (GetOpCode(callI))
                    {
                    case llvm_cos:
                        ftodTemp.f = cosf(ftod0.f);
                        break;
                    case llvm_sin:
                        ftodTemp.f = sinf(ftod0.f);
                        break;
                    case llvm_log:
                        ftodTemp.f = log10f(ftod0.f) / log10f(2.0f);
                        break;
                    case llvm_exp:
                        ftodTemp.f = powf(2.0f, ftod0.f);
                        break;
                    case llvm_sqrt:
                        ftodTemp.f = sqrtf(ftod0.f);
                        break;
                    case llvm_floor:
                        ftodTemp.f = floorf(ftod0.f);
                        break;
                    case llvm_ceil:
                        ftodTemp.f = ceilf(ftod0.f);
                        break;
                    case llvm_fabs:
                        ftodTemp.f = fabs(ftod0.f);
                        break;
                    case llvm_pow:
                        ftod1.u = lookupValue(inst->getOperand(1), CalculatedValue);
                        ftodTemp.f = powf(ftod0.f, ftod1.f);
                        break;
                    case llvm_max:
                        ftod1.u = lookupValue(inst->getOperand(1), CalculatedValue);
                        ftodTemp.f = std::max(ftod0.f, ftod1.f);
                        break;
                    case llvm_min:
                        ftod1.u = lookupValue(inst->getOperand(1), CalculatedValue);
                        ftodTemp.f = std::min(ftod0.f, ftod1.f);
                        break;
                    case llvm_rsq:
                        ftodTemp.f = 1.0f / sqrt(ftod0.f);
                        break;
                    case llvm_fsat:
                        ftodTemp.f = ftod0.f > 1.0f ? 1.0f : ftod0.f;
                        ftodTemp.f = ftodTemp.f < 0.0f ? 0.0f : ftod0.f;
                        break;
                    default:
                        assert(0);
                        break;
                    }
                }
                else
                {
                    ftod1.u = lookupValue(inst->getOperand(1), CalculatedValue);

                    switch (inst->getOpcode())
                    {
                    case Instruction::Add:
                        ftodTemp.i = ftod0.i + ftod1.i;
                        break;
                    case Instruction::FAdd:
                        ftodTemp.f = ftod0.f + ftod1.f;
                        break;
                    case Instruction::Sub:
                        ftodTemp.i = ftod0.i + ftod1.i;
                        break;
                    case Instruction::FSub:
                        ftodTemp.f = ftod0.f - ftod1.f;
                        break;
                    case Instruction::Mul:
                        ftodTemp.i = ftod0.i * ftod1.i;
                        break;
                    case Instruction::FMul:
                        ftodTemp.f = ftod0.f * ftod1.f;
                        break;
                    case Instruction::UDiv:
                        ftodTemp.u = ftod0.u / ftod1.u;
                        break;
                    case Instruction::SDiv:
                        ftodTemp.i = ftod0.i / ftod1.i;
                        break;
                    case Instruction::FDiv:
                        ftodTemp.f = ftod0.f / ftod1.f;
                        break;
                    case Instruction::URem:
                        ftodTemp.u = ftod0.u % ftod1.u;
                        break;
                    case Instruction::SRem:
                        ftodTemp.i = ftod0.i % ftod1.i;
                        break;
                    case Instruction::Shl:
                        ftodTemp.i = ftod0.i << ftod1.i;
                        break;
                    case Instruction::LShr:
                        ftodTemp.u = ftod0.u >> ftod1.u;
                        break;
                    case Instruction::AShr:
                        ftodTemp.i = (ftod0.i < 0) ?
                            (-1 * (abs(ftod0.i) >> ftod1.i)) :
                            ((ftod0.i) >> ftod1.i);
                        break;
                    case Instruction::And:
                        ftodTemp.u = ftod0.u & ftod1.u;
                        break;
                    case Instruction::Or:
                        ftodTemp.u = ftod0.u | ftod1.u;
                        break;
                    case Instruction::Xor:
                        ftodTemp.u = ftod0.u ^ ftod1.u;
                        break;
                    default:
                        assert(0);
                        break;
                    }
                }

                CalculatedValue[inst] = ftodTemp.u;
            }
        }
    }
}