/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/ReplaceUnsupportedIntrinsics/ReplaceUnsupportedIntrinsics.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "common/igc_regkeys.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/IR/IRBuilder.h"
#include "llvmWrapper/Support/Alignment.h"
#include "llvmWrapper/Support/TypeSize.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IntrinsicInst.h"
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

#include <map>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using IGCLLVM::getAlign;

namespace
{
    /// ReplaceIntrinsics pass lowers calls to unsupported intrinsics functions.
    // Two llvm instrinsics are replaced llvm.memcpy and llvm.memset. Both appear in SPIR spec.
    class ReplaceUnsupportedIntrinsics : public llvm::FunctionPass, public llvm::InstVisitor<ReplaceUnsupportedIntrinsics>
    {
    public:
        typedef void (ReplaceUnsupportedIntrinsics::* MemFuncPtr_t)(IntrinsicInst*);
        static char ID;

        ReplaceUnsupportedIntrinsics();

        ~ReplaceUnsupportedIntrinsics() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "ReplaceUnsupportedIntrinsics";
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
        }

        void visitIntrinsicInst(llvm::IntrinsicInst& I);

    private:
        CodeGenContext* m_Ctx;
        std::vector<llvm::IntrinsicInst*> m_instsToReplace;

        /// Helper
        ///
        // if the value comes from a bitcast, return the source, otherwise return itself
        Value* SkipBitCast(Value* v) {
            if (BitCastInst* bc = dyn_cast<BitCastInst>(v)) {
                // Don't skip if this is a pointer cast and the addrspace changed
                if (v->getType()->isPointerTy() &&
                    bc->getOperand(0)->getType()->isPointerTy() &&
                    v->getType()->getPointerAddressSpace() != bc->getOperand(0)->getType()->getPointerAddressSpace()) {
                    return v;
                }
                v = bc->getOperand(0);
            }
            return v;
        }

        // Get the largest of power-of-2 value that is <= C AND that can divide C.
        uint32_t getLargestPowerOfTwo(uint32_t C) {
            // If C == 0 (shouldn't happen), return a big one.
            return (C == 0) ? 4096 : (C & (~C + 1));
        }

        MemCpyInst* MemMoveToMemCpy(MemMoveInst* MM);
        Instruction* insertReverseLoop(BasicBlock* Loc, BasicBlock* Post, Value* Length, StringRef BBName);
        Instruction* insertLoop(Instruction* Loc, Value* Length, StringRef BBName);
        Value* replicateScalar(Value* ScalarVal, Type* Ty, Instruction* InsertBefore);
        void generalGroupI8Stream(
            LLVMContext& C, uint32_t NumI8, uint32_t Align,
            uint32_t& NumI32, Type** Vecs, uint32_t& L, uint32_t BaseTypeSize);

        /// replace member function
        void replaceMemcpy(IntrinsicInst* I);
        void replaceMemset(IntrinsicInst* I);
        void replaceMemMove(IntrinsicInst* I);
        void replaceExpect(IntrinsicInst* I);
        void replaceFunnelShift(IntrinsicInst* I);

        static const std::map< Intrinsic::ID, MemFuncPtr_t > m_intrinsicToFunc;
    };
}

// Register pass to igc-opt
#define PASS_FLAG "igc-replace-unsupported-intrinsics"
#define PASS_DESCRIPTION "Replace calls to instrinsics which are not supported by the codegen"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ReplaceUnsupportedIntrinsics, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(ReplaceUnsupportedIntrinsics, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ReplaceUnsupportedIntrinsics::ID = 0;

const std::map< Intrinsic::ID, ReplaceUnsupportedIntrinsics::MemFuncPtr_t > ReplaceUnsupportedIntrinsics::m_intrinsicToFunc =
{
    { Intrinsic::fshl,       &ReplaceUnsupportedIntrinsics::replaceFunnelShift },
    { Intrinsic::fshr,       &ReplaceUnsupportedIntrinsics::replaceFunnelShift },
    { Intrinsic::memcpy,     &ReplaceUnsupportedIntrinsics::replaceMemcpy },
    { Intrinsic::memset,     &ReplaceUnsupportedIntrinsics::replaceMemset },
    { Intrinsic::memmove,    &ReplaceUnsupportedIntrinsics::replaceMemMove },
    { Intrinsic::expect,     &ReplaceUnsupportedIntrinsics::replaceExpect }
};

ReplaceUnsupportedIntrinsics::ReplaceUnsupportedIntrinsics() : FunctionPass(ID)
{
    initializeReplaceUnsupportedIntrinsicsPass(*PassRegistry::getPassRegistry());
}

MemCpyInst* ReplaceUnsupportedIntrinsics::MemMoveToMemCpy(MemMoveInst* MM)
{
    SmallVector<Value*, 5> args;
    for (unsigned i = 0; i < IGCLLVM::getNumArgOperands(MM); i++)
        args.push_back(MM->getArgOperand(i));

    auto* Dst = MM->getRawDest();
    auto* Src = MM->getRawSource();
    auto* Size = MM->getLength();

    Type* Tys[] = { Dst->getType(), Src->getType(), Size->getType() };
    auto* M = MM->getParent()->getParent()->getParent();
    auto TheFn = Intrinsic::getDeclaration(M, Intrinsic::memcpy, Tys);

    return cast<MemCpyInst>(MemCpyInst::Create(TheFn, args));
}

// insertReverseLoop - Insert an empty loop at the end of BB 'Loc'.
// The loop's induction variable iterates from 'Length'-1 to 0.
// The return value is the value of the induction variable in the loop's body.
Instruction* ReplaceUnsupportedIntrinsics::insertReverseLoop(
    BasicBlock* Loc, BasicBlock* Post, Value* Length, StringRef BBName)
{
    DebugLoc DL = Loc->getTerminator()->getDebugLoc();
    Function* F = Loc->getParent();
    LLVMContext& C = F->getContext();
    IntegerType* LengthType = cast<IntegerType>(Length->getType());
    // Create an alloca for storing the loop's induction variable
    Value* pIV = new AllocaInst(LengthType, 0, "pIV", &(*F->getEntryBlock().begin()));
    // Split the BB at the location of the call
    BasicBlock* Pre = Loc;
    // Create a new BB for the loop Body
    BasicBlock* Body = BasicBlock::Create(C, Twine(BBName) + ".body", F, Post);
    ConstantInt* Zero = ConstantInt::get(LengthType, 0);
    ConstantInt* One = ConstantInt::get(LengthType, 1);
    {
        // Remove the unconditional 'br' instruction which will be replaced by a conditional 'br'
        Pre->getTerminator()->eraseFromParent();
        IGCLLVM::IRBuilder<> B(Pre);
        B.SetCurrentDebugLocation(DL);
        // Init the IV
        auto* Init = B.CreateSub(Length, One);
        B.CreateStore(Init, pIV);
        Value* IsContinue = B.CreateICmpSGE(Init, Zero);
        B.CreateCondBr(IsContinue, Body, Post);
    }
    // The induction variable's value
    Instruction* IV;
    {
        // Loop body's Basic Block
        IGCLLVM::IRBuilder<> B(Body);
        B.SetCurrentDebugLocation(DL);
        IV = B.CreateLoad(pIV, "IV");
        // User of function will add more instructions at this point ...
        // Decrement the IV and check for end of loop
        Value* Dec = B.CreateSub(IV, One);
        B.CreateStore(Dec, pIV);
        Value* IsContinue = B.CreateICmpSGE(Dec, Zero);
        B.CreateCondBr(IsContinue, Body, Post);
    }
    return IV;
}

// insertLoop - Insert an empty loop before instruction 'Loc'.
// The loop's induction variable iterates from 0 to 'Length'-1.
// The return value is the value of the induction variable in the loop's body.
Instruction* ReplaceUnsupportedIntrinsics::insertLoop(Instruction* Loc, Value* Length, StringRef BBName)
{
    DebugLoc DL = Loc->getDebugLoc();
    Function* F = Loc->getParent()->getParent();
    LLVMContext& C = F->getContext();
    IntegerType* LengthType = cast<IntegerType>(Length->getType());
    // Create an alloca for storing the loop's induction variable
    Value* pIV = new AllocaInst(LengthType, 0, "pIV", &(*F->getEntryBlock().begin()));
    // Split the BB at the location of the call
    BasicBlock* Pre = Loc->getParent();
    BasicBlock* Post = Pre->splitBasicBlock(
        BasicBlock::iterator(Loc), Twine(BBName) + ".post");
    // Create a new BB for the loop Body
    BasicBlock* Body = BasicBlock::Create(C, Twine(BBName) + ".body", F, Post);
    {
        // Remove the unconditional 'br' instruction which will be replaced by a conditional 'br'
        Pre->getTerminator()->eraseFromParent();
        IGCLLVM::IRBuilder<> B(Pre);
        B.SetCurrentDebugLocation(DL);
        ConstantInt* Zero = ConstantInt::get(LengthType, 0);
        // Init the IV
        B.CreateStore(Zero, pIV);
        Value* IsContinue = B.CreateICmpULT(Zero, Length);
        B.CreateCondBr(IsContinue, Body, Post);
    }
    // The induction variable's value
    Instruction* IV;
    {
        // Loop body's Basic Block
        IGCLLVM::IRBuilder<> B(Body);
        B.SetCurrentDebugLocation(DL);
        IV = B.CreateLoad(pIV, "IV");
        // User of function will add more instructions at this point ...
        // Increment the IV and check for end of loop
        Value* Inc = B.CreateAdd(IV, ConstantInt::get(LengthType, 1));
        B.CreateStore(Inc, pIV);
        Value* IsContinue = B.CreateICmpULT(Inc, Length);
        B.CreateCondBr(IsContinue, Body, Post);
    }
    return IV;
}

Value* ReplaceUnsupportedIntrinsics::replicateScalar(
    Value* ScalarVal, Type* Ty, Instruction* InsertBefore)
{
    IGCLLVM::FixedVectorType* VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
    Type* ETy = VTy ? VTy->getElementType() : Ty;
    uint32_t sBits = (unsigned int)ScalarVal->getType()->getPrimitiveSizeInBits();
    uint32_t nBits = (unsigned int)ETy->getPrimitiveSizeInBits();
    IGC_ASSERT(sBits);
    IGC_ASSERT_MESSAGE((nBits % sBits) == 0, "Type mismatch in replicateScalar!");
    IGC_ASSERT_MESSAGE(nBits <= 64, "Type mismatch in replicateScalar!");
    uint32_t ratio = nBits / sBits;

    IGCLLVM::IRBuilder<> Builder(InsertBefore);
    Value* NewVal;
    if (ratio > 1)
    {
        if (ConstantInt* CI = dyn_cast<ConstantInt>(ScalarVal))
        {
            uint64_t s = CI->getZExtValue();
            uint64_t n = s;
            for (unsigned i = 1; i < ratio; ++i)
            {
                n = (n << sBits) | s;
            }
            NewVal = ConstantInt::get(ETy, n);
        }
        else
        {
            Value* nScalarVal = Builder.CreateZExt(ScalarVal, ETy);
            NewVal = nScalarVal;
            for (unsigned i = 1; i < ratio; ++i)
            {
                NewVal = Builder.CreateShl(NewVal, sBits);
                NewVal = Builder.CreateAdd(NewVal, nScalarVal);
            }
        }
    }
    else
    {
        NewVal = ScalarVal;
    }

    Value* Res;
    if (VTy)
    {
        Res = UndefValue::get(VTy);
        Type* TyI32 = Type::getInt32Ty(ScalarVal->getContext());
        for (unsigned i = 0; i < VTy->getNumElements(); ++i)
        {
            Value* Idx = ConstantInt::get(TyI32, i);
            Res = Builder.CreateInsertElement(Res, NewVal, Idx);
        }
    }
    else
    {
        Res = NewVal;
    }
    return Res;

}

// A help functions to generate vector load or stores for efficient
// memory operations.
// However, if size of base type is to kept the generated vectors will be different
// <8xi32>  for size of the base type = 32
// <16xi16> for size of the base type = 16
//
// generalGroupI8Stream() groups a stream of i8 into a stream of <8xi32> or <16xi16> as
// much as possible. Then for the remaining i8's ( < 32), group them
// into vectors of element type i32(i16) and/or i8. This results in at most
// the following 5 vectors and/or scalars:
//    <4xi32>, <3xi32> or <2xi32>, i32, <2xi8>, i8 or
//    <8xi16>, <4xi16>, <2xi16>, i16, i8
// Note that we will not generate <3xi8> (see also the code for details).
// For example, given 127 i8's, we can
// have:
//   <8xi32>, <8xi32>, <8xi32>, <4xi32>, <3xi32>, <2xi8>, i8
//
// The grouping result are kept in Vecs, L (actual length of Vecs),
// and NumI32 (the number of <8xi32>, ie. the number of Vecs[0]. For all
// the other vectors/scalars, ie Vecs[1 : L-1], the number is always 1).
// For the above case, they are:
//    Vecs[0] = <8xi32>
//    Vecs[1] = <4xi32>
//    Vecs[2] = <3xi32>
//    Vecs[3] = <2xi8>
//    Vecs[4] = i8
//    L = 5;
//    NumI32 = 3;
//
// We may generate <3xi32>, but not <3xi8> as <3xi32> can be loaded
// or stored by a single send instruction, where <3xi8> cannot (even
// <3xi8> can be splitted later in VectorProcessing, but it's better
// not generate <3xi8> vector in the first place).
//
// The same example with given 127 i8's but with keeping size of base
// type of initial vector as 16 we can have:
//   <16xi16>, <16xi16>, <16xi16>, <8xi16>, <4xi16>, <2xi16>, i16, i8
//
// The grouping result are kept in Vecs, L (actual length of Vecs),
// and NumI32 (the number of <16xi16>, ie. the number of Vecs[0]. For all
// the other vectors/scalars, ie Vecs[1 : L-1], the number is always 1).
// For the above case, they are:
//    Vecs[0] = <16xi16>
//    Vecs[1] = <8xi16>
//    Vecs[2] = <4xi16>
//    Vecs[3] = <2xi16>
//    Vecs[4] = i16
//    Vecs[5] = i8
//    L = 6;
//    NumI32 = 3;
//
// Note that Vecs[] should be allocated by callers with enough space
// to hold all vectors (6 should be enough; 1 for <8xi32>(<16xi16>),
// 5 for the others).
// We want from <4x<2xhalf>> [with size of the base type 16(half)]
// generate <8xi16> not <4xi32>
// Default BaseTypeSize=32 means that we don't concern about keeping
// size of the base type
void ReplaceUnsupportedIntrinsics::generalGroupI8Stream(
    LLVMContext& C, uint32_t NumI8, uint32_t Align,
    uint32_t& VectorsNum, Type** Vecs, uint32_t& L, uint32_t BaseTypeSize = 32)
{
    VectorsNum = NumI8 / 32; // size of <8xi32> = 32. count of <8xi32> or <16xi16>
    uint32_t RemI8 = NumI8 % 32;
    uint32_t BaseTypeSizeInBytes = BaseTypeSize / 8;
    uint32_t CntI = RemI8 / BaseTypeSizeInBytes;    // the number of i32(0..7) or i16(0..15)
    uint32_t CntI8 = RemI8 % BaseTypeSizeInBytes;   // remaining number of i8(0-3) - for base_type_size = 32 or
                                                    //                     i8(0-1) - for base_type_size = 16

    // To process all cases (3 for i32 and 4 for i16: it depends of how much CntI do we have)
    uint32_t Power = 256 / BaseTypeSize;    // i32: (256 / 32) = 0b1000 = 8
                                            // i16: (256 / 16) = 0b10000 = 16

    Type* BaseType = Type::getIntNTy(C, BaseTypeSize);
    Type* TyI8 = Type::getInt8Ty(C);

    uint32_t n = 0;
    Vecs[n++] = IGCLLVM::FixedVectorType::get(BaseType, Power);

    while ((Power >>= 1) > 1)
    {
        if (CntI >= Power)
        {
            Vecs[n++] = IGCLLVM::FixedVectorType::get(BaseType, Power);
            CntI -= Power;
        }
        if (CntI == 3 && BaseTypeSize == 32 && Align >= 4) // special case for <8xi32> not to generate <3xi8> but to generate <3xi32>
        {
            Vecs[n++] = IGCLLVM::FixedVectorType::get(BaseType, 3);
            CntI = 0;
            break;
        }
    }
    if (CntI >= 1)
    {
        Vecs[n++] = BaseType;
        CntI -= Power; // Assume that pow should be 1 to generate i32(i16) and not <1xi32>(<1xi16>)
    }
    IGC_ASSERT_MESSAGE(CntI == 0, "Did not handle all types of base_type");

    Power = BaseTypeSize / 4;   // i32: 32 / 8 = 4
                                // i16: 16 / 8 = 2
    while ((Power >>= 1) > 1)
    {
        if (CntI8 >= Power)
        {
            Vecs[n++] = IGCLLVM::FixedVectorType::get(TyI8, Power);
            CntI8 -= Power;
        }
    }
    if (CntI8 >= 1)
    {
        Vecs[n++] = TyI8;
        CntI8 -= Power; // Assume that pow should be 1 to generate i8 not <1xi8>
    }
    IGC_ASSERT_MESSAGE(CntI8 == 0, "Did not handle all types of I8");

    L = n;
}

void ReplaceUnsupportedIntrinsics::replaceMemcpy(IntrinsicInst* I)
{
    // The idea is to convert
    //
    //   memcpy (i8* Dst, i8* Src, len)
    //
    // into a vector load and store for cases where "len" is
    // constant. If "len" isn't constant,  just use i8 copy as
    // this should not happen with OCL code (all memcpy is
    // generated by the compiler for cases such as structure
    // assignment, etc.)
    //
    // If len is constant, it will be transferred to
    //
    //   lenv8 = len / 32 (<8xi32>);
    //   len_rem = len % 32;
    //
    //   // main loop
    //   dstV8 = bitcast Dst, <8xi32>*
    //   srcV8 = bitcast Src, <8xi32>*
    //   for(i=0; i < lenv8; ++i)
    //     dstV8[i] = srcV8[i];
    //
    //   // epilog, process remaining elements
    //   for(i=0; i < len_rem; ++i)
    //     Dst[lenv8*32 + i] = Src[lenv8*32 + i];
    //
    //   Note that the above epilog loop is optimized away with
    //   as much as possible <nxi32> and <mxi8> loads and stores
    //   or if we want to keep size of the base type
    //   (for 16bit there will be <nxi16> and <mxi8>)
    //
    // Selecting 8 as vector length or 16 in case of i16 is due to
    // that A64 messages can load eight i32 or sixteen i16 per SIMD channel.
    // A32 will have 2 loads/stores for each vector, which is still efficient.
    // Unaligned vector will be handled correctly and effciently later
    // in vector load and store emit.
    MemCpyInst* MC = cast<MemCpyInst>(I);
    Value* Dst = MC->getRawDest();
    Value* Src = MC->getRawSource();
    Value* LPCount = MC->getLength();
    uint32_t Align = MC->getDestAlignment();
    Align = Align != 0 ? Align : 1;
    uint32_t SrcAlign = MC->getSourceAlignment();
    SrcAlign = SrcAlign != 0 ? SrcAlign : 1;
    const bool IsVolatile = MC->isVolatile();
    const uint32_t SrcAS = MC->getSourceAddressSpace();
    const uint32_t DstAS = MC->getDestAddressSpace();

    LLVMContext& C = MC->getContext();
    Type* TySrcPtrI8 = Type::getInt8PtrTy(C, SrcAS);
    Type* TyDstPtrI8 = Type::getInt8PtrTy(C, DstAS);

    IGCLLVM::IRBuilder<> Builder(MC);

    // BaseSize is flag if we want to handle algorithm in general way
    // or want to keep size of base type to further optimizations
    uint32_t BaseSize = 0;
    Type* RawDstType = Dst->stripPointerCasts()->getType()->getPointerElementType();
    if (Type* BaseType = GetBaseType(RawDstType))
        BaseSize = BaseType->getScalarSizeInBits();

    if (BaseSize != 16)
        // size 32 is equal to size of i32, so general algorithm will be applied
        BaseSize = 32;

    ConstantInt* CI = dyn_cast<ConstantInt>(LPCount);
    if (CI)
    {
        uint32_t Count = (uint32_t)CI->getZExtValue();

        Type* VecTys[8];
        uint32_t Len, NewCount;
        generalGroupI8Stream(C, Count, Align, NewCount, VecTys, Len, BaseSize);

        Value* NewSrc, * NewDst, * vDst, * vSrc;
        uint32_t BOfst = 0; // Byte offset

        // First, insert main loop before MC.
        // Note that if NewCount is small, we may directly generate ld/st
        // without generating the loop.
        if (NewCount > 0)
        {
            vSrc = Builder.CreateBitCast(SkipBitCast(Src), PointerType::get(VecTys[0], SrcAS), "memcpy_vsrc");
            vDst = Builder.CreateBitCast(SkipBitCast(Dst), PointerType::get(VecTys[0], DstAS), "memcpy_vdst");

            // getPrimitiveSizeInBits() should be enough, no need to
            // use DataLayout to get target-dependent size.
            uint32_t SZ = (unsigned int)(VecTys[0]->getPrimitiveSizeInBits() / 8);

            // To set alignment correctly
            uint32_t adjust_align = getLargestPowerOfTwo(SZ);
            Align = adjust_align < Align ? adjust_align : Align;
            SrcAlign = adjust_align < SrcAlign ? adjust_align : SrcAlign;

            // If NewCount is less than 6,  don't generate loop.
            // Note that 6 is just an arbitrary number here.
            if (NewCount < 6)
            {
                for (unsigned i = 0; i < NewCount; ++i)
                {
                    Value* tSrc = Builder.CreateConstGEP1_32(vSrc, i);
                    Value* tDst = Builder.CreateConstGEP1_32(vDst, i);
                    LoadInst* L = Builder.CreateAlignedLoad(tSrc, getAlign(SrcAlign), IsVolatile);
                    (void)Builder.CreateAlignedStore(L, tDst, getAlign(Align), IsVolatile);
                }
            }
            else
            {
                Value* NewLPCount = ConstantInt::get(LPCount->getType(), NewCount);
                Instruction* IV = insertLoop(MC, NewLPCount, "memcpy");
                {
                    IGCLLVM::IRBuilder<> B(&(*++BasicBlock::iterator(IV)));
                    Value* tSrc = B.CreateGEP(vSrc, IV);
                    Value* tDst = B.CreateGEP(vDst, IV);
                    LoadInst* L = B.CreateAlignedLoad(tSrc, getAlign(SrcAlign), IsVolatile);
                    (void)B.CreateAlignedStore(L, tDst, getAlign(Align), IsVolatile);
                }
            }

            BOfst = NewCount * SZ;
        }

        // Second, generate epilog code before MC.
        // Note that as MC has been moved to a different BB by
        //   inserting the main loop! Reset it to MC.
        Builder.SetInsertPoint(MC);
        if (Len > 1)
        {
            Src = Builder.CreateBitCast(SkipBitCast(Src), TySrcPtrI8, "memcpy_src");
            Dst = Builder.CreateBitCast(SkipBitCast(Dst), TyDstPtrI8, "memcpy_dst");
        }
        for (unsigned i = 1; i < Len; ++i)
        {
            uint32_t SZ = (unsigned int)VecTys[i]->getPrimitiveSizeInBits() / 8;
            uint32_t adjust_align = getLargestPowerOfTwo(SZ);
            Align = adjust_align < Align ? adjust_align : Align;
            SrcAlign = adjust_align < SrcAlign ? adjust_align : SrcAlign;
            NewSrc = BOfst > 0 ? Builder.CreateConstGEP1_32(Src, BOfst) : Src;
            NewDst = BOfst > 0 ? Builder.CreateConstGEP1_32(Dst, BOfst) : Dst;
            vSrc = Builder.CreateBitCast(SkipBitCast(NewSrc), PointerType::get(VecTys[i], SrcAS), "memcpy_rem");
            vDst = Builder.CreateBitCast(SkipBitCast(NewDst), PointerType::get(VecTys[i], DstAS), "memcpy_rem");
            LoadInst* L = Builder.CreateAlignedLoad(vSrc, getAlign(SrcAlign), IsVolatile);
            (void)Builder.CreateAlignedStore(L, vDst, getAlign(Align), IsVolatile);
            BOfst += SZ;
        }
    }
    else
    {
        Src = Builder.CreateBitCast(SkipBitCast(Src), TySrcPtrI8, "memcpy_src");
        Dst = Builder.CreateBitCast(SkipBitCast(Dst), TyDstPtrI8, "memcpy_dst");
        // Fall back to i8 copy
        Instruction* IV = insertLoop(MC, LPCount, "memcpy");
        {
            IGCLLVM::IRBuilder<> B(&(*++BasicBlock::iterator(IV)));
            Value* tSrc = B.CreateGEP(Src, IV);
            Value* tDst = B.CreateGEP(Dst, IV);
            LoadInst* L = B.CreateAlignedLoad(tSrc, getAlign(SrcAlign), IsVolatile);
            (void)B.CreateAlignedStore(L, tDst, getAlign(Align), IsVolatile);
        }
    }
    MC->eraseFromParent();
}

void ReplaceUnsupportedIntrinsics::replaceMemMove(IntrinsicInst* I)
{
    MemMoveInst* MM = cast<MemMoveInst>(I);
    Value* Dst = MM->getRawDest();
    Value* Src = MM->getRawSource();
    Value* LPCount = MM->getLength();
    uint32_t Align = MM->getDestAlignment();
    if (Align == 0)
        Align = 1;
    const bool IsVolatile = MM->isVolatile();
    const uint32_t SrcAS = MM->getSourceAddressSpace();
    const uint32_t DstAS = MM->getDestAddressSpace();

    // If non-generic address spaces mismatch, they can't alias
    // and we can do a memcpy.

    if (SrcAS < ADDRESS_SPACE_NUM_ADDRESSES &&
        DstAS < ADDRESS_SPACE_NUM_ADDRESSES &&
        SrcAS != ADDRESS_SPACE_GENERIC &&
        DstAS != ADDRESS_SPACE_GENERIC &&
        SrcAS != DstAS)
    {
        auto* MemCpy = MemMoveToMemCpy(MM);
        MemCpy->insertBefore(MM);
        replaceMemcpy(MemCpy);
        MM->eraseFromParent();
        return;
    }

    LLVMContext& C = MM->getContext();
    Type* TySrcPtrI8 = Type::getInt8PtrTy(C, SrcAS);
    Type* TyDstPtrI8 = Type::getInt8PtrTy(C, DstAS);

    auto* F = MM->getParent()->getParent();

    IGCLLVM::IRBuilder<> B(MM);

    auto* i8Src = B.CreateBitCast(SkipBitCast(Src), TySrcPtrI8, "memcpy_src");
    auto* i8Dst = B.CreateBitCast(SkipBitCast(Dst), TyDstPtrI8, "memcpy_dst");

    // Setup control flow to do:
    // if (Src < Dst)
    //   reverse copy data
    // else
    //   normal copy (such as memcpy())

    // Src < Dst
    Value* pCmp = nullptr;
    {
        auto* cmpCastSrc = (DstAS == ADDRESS_SPACE_GENERIC) ?
            B.CreateAddrSpaceCast(i8Src, TyDstPtrI8) : i8Src;
        auto* cmpCastDst = (SrcAS == ADDRESS_SPACE_GENERIC) ?
            B.CreateAddrSpaceCast(i8Dst, TySrcPtrI8) : i8Dst;

        pCmp = B.CreateICmpULT(cmpCastSrc, cmpCastDst);
    }

    auto* Pre = MM->getParent();
    auto* Post = Pre->splitBasicBlock(MM, "memmove.post");

    Pre->getTerminator()->eraseFromParent();

    auto* BBTrue = BasicBlock::Create(C, "memmove.true", F, Post);
    auto* BBFalse = BasicBlock::Create(C, "memmove.false", F, Post);

    B.SetInsertPoint(Pre);
    B.CreateCondBr(pCmp, BBTrue, BBFalse);

    B.SetInsertPoint(BBTrue);
    B.CreateBr(Post);
    B.SetInsertPoint(BBFalse);
    B.CreateBr(Post);

    auto* CI = dyn_cast<ConstantInt>(LPCount);
    if (CI)
    {
        uint32_t Count = (uint32_t)CI->getZExtValue();

        // noop
        if (Count == 0)
        {
            MM->eraseFromParent();
            return;
        }

        Type* VecTys[8];
        uint32_t Len, NewCount;
        generalGroupI8Stream(C, Count, Align, NewCount, VecTys, Len);

        // for true block (Src < Dst), do a reverse copy.
        {
            B.SetInsertPoint(BBTrue->getTerminator());

            // calculate byte offsets so we can walk backwards through them
            SmallVector<uint, 8> byteOffsets{ 0 };

            {
                uint32_t SZ = (unsigned int)(VecTys[0]->getPrimitiveSizeInBits() / 8);
                uint32_t BOfst = NewCount * SZ;

                for (unsigned i = 1; i < Len; i++)
                {
                    byteOffsets.push_back(BOfst);
                    uint32_t SZ = (unsigned int)(VecTys[i]->getPrimitiveSizeInBits() / 8);
                    BOfst += SZ;
                }
            }

            // emit the smaller than <8 x i32> stores
            for (unsigned i = Len - 1; i >= 1; i--)
            {
                uint offset = byteOffsets[i];
                uint32_t newAlign = getLargestPowerOfTwo(Align + offset);
                auto* tSrc = B.CreateConstGEP1_32(i8Src, offset);
                auto* tDst = B.CreateConstGEP1_32(i8Dst, offset);

                auto* vSrc = B.CreateBitCast(SkipBitCast(tSrc), PointerType::get(VecTys[i], SrcAS), "memcpy_rem");
                auto* vDst = B.CreateBitCast(SkipBitCast(tDst), PointerType::get(VecTys[i], DstAS), "memcpy_rem");
                LoadInst* L = B.CreateAlignedLoad(vSrc, getAlign(newAlign), IsVolatile);
                (void)B.CreateAlignedStore(L, vDst, getAlign(newAlign), IsVolatile);
            }

            // now emit the <8 x i32> stores
            auto* vSrc = B.CreateBitCast(SkipBitCast(Src), PointerType::get(VecTys[0], SrcAS), "memcpy_vsrc");
            auto* vDst = B.CreateBitCast(SkipBitCast(Dst), PointerType::get(VecTys[0], DstAS), "memcpy_vdst");
            // If NewCount is less than 6,  don't generate loop.
            // Note that 6 is just an arbitrary number here.
            uint32_t SZ = (unsigned int)(VecTys[0]->getPrimitiveSizeInBits() / 8);
            uint32_t newAlign = getLargestPowerOfTwo(Align + SZ);
            if (NewCount < 6)
            {
                for (unsigned i = 0; i < NewCount; i++)
                {
                    unsigned idx = NewCount - 1 - i;
                    auto* tSrc = B.CreateConstGEP1_32(vSrc, idx);
                    auto* tDst = B.CreateConstGEP1_32(vDst, idx);
                    LoadInst* L = B.CreateAlignedLoad(tSrc, getAlign(newAlign), IsVolatile);
                    (void)B.CreateAlignedStore(L, tDst, getAlign(newAlign), IsVolatile);
                }
            }
            else
            {
                auto* NewLPCount = ConstantInt::get(LPCount->getType(), NewCount);
                Instruction* IV = insertReverseLoop(BBTrue, Post, NewLPCount, "memmmove");
                {
                    IGCLLVM::IRBuilder<> B(&(*++BasicBlock::iterator(IV)));
                    Value* tSrc = B.CreateGEP(vSrc, IV);
                    Value* tDst = B.CreateGEP(vDst, IV);
                    LoadInst* L = B.CreateAlignedLoad(tSrc, getAlign(newAlign), IsVolatile);
                    (void)B.CreateAlignedStore(L, tDst, getAlign(newAlign), IsVolatile);
                }
            }
        }

        // for false block (Src >= Dst), just a plain memcpy.
        {
            auto* MemCpy = MemMoveToMemCpy(MM);
            MemCpy->insertBefore(BBFalse->getTerminator());
            replaceMemcpy(MemCpy);
        }
    }
    else
    {
        // (Src < Dst)
        {
            B.SetInsertPoint(BBTrue->getTerminator());
            // Fall back to i8 copy
            Instruction* IV = insertReverseLoop(BBTrue, Post, LPCount, "memmove");
            {
                IGCLLVM::IRBuilder<> B(&(*++BasicBlock::iterator(IV)));
                Value* tSrc = B.CreateGEP(i8Src, IV);
                Value* tDst = B.CreateGEP(i8Dst, IV);
                LoadInst* L = B.CreateAlignedLoad(tSrc, getAlign(1), IsVolatile);
                (void)B.CreateAlignedStore(L, tDst, getAlign(1), IsVolatile);
            }
        }

        // for false block (Src >= Dst), just a plain memcpy.
        {
            auto* MemCpy = MemMoveToMemCpy(MM);
            MemCpy->insertBefore(BBFalse->getTerminator());
            replaceMemcpy(MemCpy);
        }
    }

    MM->eraseFromParent();
}

void ReplaceUnsupportedIntrinsics::replaceMemset(IntrinsicInst* I)
{
    // Same idea as replaceMemcpy (see comment of replaceMemcpy).
    MemSetInst* MS = cast<MemSetInst>(I);
    Value* Dst = MS->getRawDest();
    Value* Src = MS->getValue();
    Value* LPCount = MS->getLength();
    uint32_t Align = MS->getDestAlignment();
    const bool IsVolatile = MS->isVolatile();
    const uint32_t AS = MS->getDestAddressSpace();

    LLVMContext& C = MS->getContext();
    Type* TyPtrI8 = Type::getInt8PtrTy(C, AS);

    IGCLLVM::IRBuilder<> Builder(MS);

    ConstantInt* CI = dyn_cast<ConstantInt>(LPCount);
    if (CI)
    {
        uint32_t Count = (uint32_t)CI->getZExtValue();

        Type* VecTys[8];
        uint32_t Len, NewCount;
        generalGroupI8Stream(C, Count, Align, NewCount, VecTys, Len);

        Value* NewDst, * vDst, * vSrc;
        uint32_t BOfst = 0; // Byte offset

        // First, insert main loop before MC.
        if (NewCount > 0)
        {
            PointerType* PTy = PointerType::get(VecTys[0], AS);
            vSrc = replicateScalar(Src, VecTys[0], MS);
            vDst = Builder.CreateBitCast(SkipBitCast(Dst), PTy, "memset_vdst");

            // getPrimitiveSizeInBits() should be enough, no need to
            // use DataLayout to get target-dependent size.
            uint32_t SZ = (unsigned int)(VecTys[0]->getPrimitiveSizeInBits() / 8);

            // To set alignment correctly
            uint32_t adjust_align = getLargestPowerOfTwo(SZ);
            Align = adjust_align < Align ? adjust_align : Align;

            // If NewCount is less than 6,  don't generate loop.
            // Note that 6 is just an arbitrary number here.
            if (NewCount < 6)
            {
                for (unsigned i = 0; i < NewCount; ++i)
                {
                    Value* tDst = Builder.CreateConstGEP1_32(vDst, i);
                    (void)Builder.CreateAlignedStore(vSrc, tDst, getAlign(Align), IsVolatile);
                }
            }
            else
            {
                Value* NewLPCount = ConstantInt::get(LPCount->getType(), NewCount);
                Instruction* IV = insertLoop(MS, NewLPCount, "memset");
                {
                    IGCLLVM::IRBuilder<> B(&(*++BasicBlock::iterator(IV)));
                    Value* tDst = B.CreateGEP(vDst, IV);
                    (void)B.CreateAlignedStore(vSrc, tDst, getAlign(Align), IsVolatile);
                }
            }

            // Set offset for the remaining elements
            BOfst = NewCount * SZ;
        }

        // Second, generate epilog code before MS.
        // Note that as MC has been moved to a different BB by
        //   inserting the main loop! Reset it to MS.
        Builder.SetInsertPoint(MS);
        if (Len > 1)
        {
            Dst = Builder.CreateBitCast(SkipBitCast(Dst), TyPtrI8, "memset_dst");
        }
        for (unsigned i = 1; i < Len; ++i)
        {
            uint32_t SZ = (unsigned int)VecTys[i]->getPrimitiveSizeInBits() / 8;
            uint32_t adjust_align = getLargestPowerOfTwo(SZ);
            Align = adjust_align < Align ? adjust_align : Align;
            PointerType* PTy = PointerType::get(VecTys[i], AS);
            NewDst = BOfst > 0 ? Builder.CreateConstGEP1_32(Dst, BOfst) : Dst;
            vSrc = replicateScalar(Src, VecTys[i], MS);
            vDst = Builder.CreateBitCast(SkipBitCast(NewDst), PTy, "memset_rem");
            (void)Builder.CreateAlignedStore(vSrc, vDst, getAlign(Align), IsVolatile);
            BOfst += SZ;
        }
    }
    else
    {
        Dst = Builder.CreateBitCast(SkipBitCast(Dst), TyPtrI8, "memset_dst");
        // Fall back to i8 copy
        Instruction* IV = insertLoop(MS, LPCount, "memset");
        {
            IGCLLVM::IRBuilder<> B(&(*++BasicBlock::iterator(IV)));
            Value* tDst = B.CreateGEP(Dst, IV);
            (void)B.CreateAlignedStore(Src, tDst, getAlign(Align), IsVolatile);
        }
    }
    MS->eraseFromParent();
}

void ReplaceUnsupportedIntrinsics::replaceExpect(IntrinsicInst* MS)
{
    MS->replaceAllUsesWith(MS->getOperand(0));
    MS->eraseFromParent();

}

/*
  Replaces llvm.fshl.* and llvm.fshr.* funnel shift intrinsics.
  E.g. for fshl we would produce a following sequence:
  %r = call i8 @llvm.fshl.i8(i8 %a, i8 %b, i8 %c) =>
  %modRes = urem i8 %c, 8        // get the modulo of shift value
  %subRes = sub i8 8, %modRes    // subtract from the type's number of bits
  %shlRes = shl i8 %a, %modRes   // shift the bits according to instruction spec
  %shrRes = lshr i8 %b, %subRes
  %r = or i8 %shlRes, %shrRes    // compose the final result
*/
void ReplaceUnsupportedIntrinsics::replaceFunnelShift(IntrinsicInst* I) {
    IGC_ASSERT(I->getIntrinsicID() == Intrinsic::fshl ||
        I->getIntrinsicID() == Intrinsic::fshr);
    IGCLLVM::IRBuilder<> Builder(I);
    unsigned sizeInBits = I->getArgOperand(0)->getType()->getScalarSizeInBits();

    // Don't replace rotate
    if (I->getArgOperand(0) == I->getArgOperand(1) && !I->getType()->isVectorTy() &&
        m_Ctx->platform.supportRotateInstruction())
    {
        if (m_Ctx->platform.supportQWRotateInstructions() && sizeInBits == 64) {
            return;
        }
        if (sizeInBits == 16 || sizeInBits == 32) {
            return;
        }
    }

    Value* numBits = Builder.getIntN(sizeInBits, sizeInBits);
    if (auto IVT = dyn_cast<IGCLLVM::FixedVectorType>(I->getType())) {
        numBits = ConstantVector::getSplat(IGCLLVM::getElementCount((uint32_t)IVT->getNumElements()), cast<Constant>(numBits));
    }
    auto shiftModulo = Builder.CreateURem(I->getArgOperand(2), numBits);
    auto negativeShift = Builder.CreateSub(numBits, shiftModulo);
    if (I->getIntrinsicID() == Intrinsic::fshr) {
        std::swap(shiftModulo, negativeShift);
    }
    auto upperShifted = Builder.CreateShl(I->getArgOperand(0), shiftModulo);
    auto lowerShifted = Builder.CreateLShr(I->getArgOperand(1), negativeShift);
    auto result = Builder.CreateOr(upperShifted, lowerShifted);

    I->replaceAllUsesWith(result);
    I->eraseFromParent();
}

void ReplaceUnsupportedIntrinsics::visitIntrinsicInst(IntrinsicInst& I) {
    if (m_intrinsicToFunc.find(I.getIntrinsicID()) != m_intrinsicToFunc.end()) {
        m_instsToReplace.push_back(&I);
    }
}

bool ReplaceUnsupportedIntrinsics::runOnFunction(Function& F)
{
    m_Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_instsToReplace.clear();
    visit(F);
    for (auto I : m_instsToReplace) {
        (this->*m_intrinsicToFunc.at(I->getIntrinsicID())) (I);
    }
    return !m_instsToReplace.empty();
}

FunctionPass* IGC::createReplaceUnsupportedIntrinsicsPass()
{
    return new ReplaceUnsupportedIntrinsics();
}
