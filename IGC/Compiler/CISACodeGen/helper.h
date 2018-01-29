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
#pragma once


#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/MetaDataApi/IGCMetaDataDefs.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "common/MDFrameWork.h"
#include "common/Types.hpp"

typedef unsigned int uint;

#define SIZE_WORD   2
#define SIZE_DWORD  4
#define SIZE_OWORD 16
#define SIZE_GRF   32

enum ADDRESS_SPACE : unsigned int;

namespace IGC
{

class CodeGenContext;
struct SProgramOutput;

enum e_llvmType
{
    e_Instruction = 0,
    e_Intrinsic   = 1,
    e_GenISAIntrinsic = 1,
};
#define LLVMTYPEBYTE 24

#define OPCODE(instName,llvmType) \
    instName | llvmType<<LLVMTYPEBYTE

#define DECLARE_OPCODE(instName, llvmType, name, modifiers, sat, pred, condMod, mathIntrinsic, atomicIntrinsic, regioning) \
    name = OPCODE(llvm::llvmType::instName,e_##llvmType),
enum EOPCODE
{
#include "opCode.h"
};
#undef DECLARE_OPCODE

#define DECLARE_OPCODE(instName, llvmType, name, modifiers, sat, pred, condMod, mathIntrinsic, atomicIntrinsic, regioning) \
    static_assert((llvm::llvmType::instName < ( 1 << LLVMTYPEBYTE ) ), "Enum bitfield range check");
#include "opCode.h"
#undef DECLARE_OPCODE

EOPCODE GetOpCode(llvm::Instruction* inst);
bool SupportsModifier(llvm::Instruction* inst);
bool SupportsSaturate(llvm::Instruction* inst);
bool SupportsPredicate(llvm::Instruction* inst);
bool SupportsCondModifier(llvm::Instruction* inst);
bool SupportsRegioning(llvm::Instruction* inst);
bool IsMathIntrinsic(EOPCODE opcode);
bool IsAtomicIntrinsic(EOPCODE opcode);
bool IsGradientIntrinsic(EOPCODE opcode);
bool IsSubGroupIntrinsicWithSimd32Implementation(EOPCODE opcode);

bool ComputesGradient(llvm::Instruction* inst);

BufferType GetBufferType(uint addrSpace);

void VectorToElement(
    llvm::Value *inst, 
    llvm::Value *elem[],
    llvm::Type *int32Ty, 
    llvm::Instruction *insert_before, 
    int vsize=4);
llvm::Value* ElementToVector(
    llvm::Value *elem[],
    llvm::Type *int32Ty, 
    llvm::Instruction *insert_before, 
    int vsize=4);

/// return true if pLLVMInst is load from constant-buffer with immediate constant-buffer index
bool IsLoadFromDirectCB(llvm::Instruction *pLLVMInst, uint& cbId, llvm::Value* &eltPtrVal);
bool IsReadOnlyLoadDirectCB(llvm::Instruction *pLLVMInst, uint& cbId, llvm::Value* &eltPtrVal, BufferType& buftype);

int findSampleInstructionTextureIdx(llvm::Instruction* inst);
llvm::Value* getTextureIndexArgBasedOnOpcode(llvm::Instruction* inst);

llvm::LoadInst* cloneLoad(llvm::LoadInst *Orig, llvm::Value *Ptr);
llvm::StoreInst* cloneStore(llvm::StoreInst *Orig, llvm::Value *Val, llvm::Value *Ptr);

void getTextureAndSamplerOperands(llvm::GenIntrinsicInst *pIntr, llvm::Value*& pTextureValue, llvm::Value*& pSamplerValue);
void ChangePtrTypeInIntrinsic(llvm::GenIntrinsicInst *&pIntr, llvm::Value* oldPtr, llvm::Value* newPtr);

llvm::Value* TracePointerSource(llvm::Value* resourcePtr);
llvm::Value* TracePointerSource(llvm::Value* resourcePtr, bool seenPhi, bool fillList, std::vector<llvm::Value*> &instList);
bool GetResourcePointerInfo(llvm::Value* srcPtr, unsigned &resID, IGC::BufferType &resTy);

bool isSampleLoadGather4InfoInstruction(llvm::Instruction* inst);
bool isSampleInstruction(llvm::Instruction* inst);
bool isInfoInstruction(llvm::Instruction* inst);
bool isLdInstruction(llvm::Instruction* inst);
bool isGather4Instruction(llvm::Instruction* inst);
bool isVectorInputInstruction(llvm::Instruction* inst);

bool IsMediaIOIntrinsic(llvm::Instruction *inst);
bool isSubGroupIntrinsic(const llvm::Instruction *I);

unsigned EncodeAS4GFXResource(
    const llvm::Value& bufIdx,
    BufferType bufType,
    unsigned uniqueIndAS);

BufferType DecodeAS4GFXResource(unsigned addrSpace, bool& directIdx, unsigned& bufId);
int getConstantBufferLoadOffset(llvm::LoadInst *ld);

bool IsDirectIdx(unsigned addrSpace);

inline bool IsBindless(BufferType t) 
{
    return t == BINDLESS || t == BINDLESS_READONLY;
}

bool IsUnsignedCmp(const llvm::CmpInst::Predicate Pred);
bool IsSignedCmp(const llvm::CmpInst::Predicate Pred);

bool IsBitCastForLifetimeMark(const llvm::Value *V);

// isA64Ptr - Queries whether given pointer type requires 64-bit representation in vISA
bool isA64Ptr(llvm::PointerType *PT, CodeGenContext* pContext);

/// \brief Check function's type in the metadata.
inline bool isFuncOfType(IGCMD::MetaDataUtils *pM, llvm::Function *F,
                         IGCMD::FunctionTypeEnum FuncType)
{
    if (F != nullptr && pM->findFunctionsInfoItem(F) != pM->end_FunctionsInfo())
    {
        IGCMD::FunctionInfoMetaDataHandle Info = pM->getFunctionsInfoItem(F);
        return Info->isTypeHasValue() && Info->getType() == FuncType;
    }
    return false;
}

/// \brief Check whether a function is a OCL user function, aka, subroutine.
inline bool isOCLUserFunc(IGCMD::MetaDataUtils *pM, llvm::Function *F)
{
    return isFuncOfType(pM, F, IGCMD::FunctionTypeEnum::OpenCLUserFunctionType);
}

/// \brief Check whether a function is a OCL kernel function.
inline bool isOCLKernelFunc(IGCMD::MetaDataUtils *pM, llvm::Function *F)
{
    return isFuncOfType(pM, F, IGCMD::FunctionTypeEnum::OpenCLKernelFunctionType);
}

/// Determine whether this is a kernel function or not. Each kernel function is
/// the group leader in its group.
inline bool isKernelFunc(IGCMD::MetaDataUtils *pM, llvm::Function *F) {
    // This is OCL specific. Other target should update this check.
    if (isOCLUserFunc(pM, F))
        return false;

    // Nonempty functions are kernels, if not subroutine.
    return !F->empty();
}

// \brief Get next instruction, returning null if it's the last of the BB.
// This is the replacement of Instruction::getNextNode(), since getNextNode()
// on last inst of BB will return sentinel node as instruction, which will
// cause memory corruption.  A better solution is to switch to iterator and
// avoid using getNextNode().
inline llvm::Instruction* GetNextInstruction(llvm::Instruction* inst)
{
    llvm::BasicBlock::iterator I = llvm::BasicBlock::iterator(inst);
    if (++I == inst->getParent()->end())
    {
        return nullptr;
    }
    return &(*I);
}

inline bool RTWriteHasSource0Alpha(
    const llvm::RTWritIntrinsic* rtWrite,
    ModuleMetaData* md)
{
    return !md->psInfo.SkipSrc0Alpha &&
        rtWrite->getRTIndexImm() > 0;
}
inline bool DoesRTWriteSrc0AlphaBelongToHomogeneousPart(
    const llvm::RTWritIntrinsic* rtWrite,
    ModuleMetaData* md)
{
    return !rtWrite->hasMask() && RTWriteHasSource0Alpha(rtWrite, md);
}

inline bool LoadUsedByConstExtractOnly(
    llvm::LoadInst* ld,
    llvm::SmallVector< llvm::SmallVector<llvm::ExtractElementInst*, 1>, 4>& extracts)
{
    for (auto UI = ld->user_begin(), UE = ld->user_end(); UI != UE; ++UI)
    {
        llvm::ExtractElementInst* ei =
            llvm::dyn_cast<llvm::ExtractElementInst>(*UI);
        if (!ei)
        {
            return false;
        }
        else
        {
            llvm::ConstantInt* idxv =
                llvm::dyn_cast<llvm::ConstantInt>(ei->getIndexOperand());
            if (!idxv)
            {
                return false;
            }
            uint idx = (uint)idxv->getZExtValue();
            extracts[idx].push_back(ei);
        }
    }
    return true;
}


llvm::Value* mutatePtrType(llvm::Value* ptrv, llvm::PointerType* newType,
    llvm::IRBuilder<>& builder, const llvm::Twine& name = "");

unsigned int AppendConservativeRastWAHeader(IGC::SProgramOutput* program, SIMDMode simdmode);

bool DSDualPatchEnabled(class CodeGenContext* ctx);

/// \brief Check whether inst precedes given position in one basic block
inline bool isInstPrecede(
    const llvm::Instruction* inst,
    const llvm::Instruction* pos)
{
    // must within same basic block
    assert(inst->getParent() == pos->getParent());
    if (inst == pos)
    {
        return true;
    }

    auto II = inst->getParent()->begin();
    for (; &*II != inst && &*II != pos; ++ II)
        ;
    return &*II == inst;
}

bool valueIsPositive(llvm::Value* V, const llvm::DataLayout *DL);

} // namespace IGC
