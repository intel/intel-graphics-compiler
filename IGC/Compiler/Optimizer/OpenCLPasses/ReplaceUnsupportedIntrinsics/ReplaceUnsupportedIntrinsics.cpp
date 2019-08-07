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

#include "Compiler/Optimizer/OpenCLPasses/ReplaceUnsupportedIntrinsics/ReplaceUnsupportedIntrinsics.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "common/igc_regkeys.hpp"

#include "common/LLVMWarningsPush.hpp"

#include "llvmWrapper/IR/Instructions.h"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/IntrinsicInst.h>
#include "common/LLVMWarningsPop.hpp"

#include <map>

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-replace-unsupported-intrinsics"
#define PASS_DESCRIPTION "Replace calls to instrinsics which are not supported by the codegen"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ReplaceUnsupportedIntrinsics, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(ReplaceUnsupportedIntrinsics, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ReplaceUnsupportedIntrinsics::ID = 0;

ReplaceUnsupportedIntrinsics::ReplaceUnsupportedIntrinsics() : FunctionPass(ID)
{
    initializeReplaceUnsupportedIntrinsicsPass(*PassRegistry::getPassRegistry());
}

namespace {

    // if the value comes from a bitcast, return the source, otherwise return itself
    Value* SkipBitCast(Value* v)
    {
        if (BitCastInst * bc = dyn_cast<BitCastInst>(v))
        {
            v = bc->getOperand(0);
        }
        return v;
    }

    // Get the largest of power-of-2 value that is <= C
    // AND that can divide C.
    uint32_t getLargestPowerOfTwo(uint32_t C) {
        // If C == 0 (shouldn't happen), return a big one.
        return (C == 0) ? 4096 : (C & (~C + 1));
    }

    MemCpyInst* MemMoveToMemCpy(MemMoveInst* MM)
    {
        SmallVector<Value*, 5> args;
        for (unsigned i = 0; i < MM->getNumArgOperands(); i++)
            args.push_back(MM->getArgOperand(i));

        auto* Dst = MM->getRawDest();
        auto* Src = MM->getRawSource();
        auto* Size = MM->getLength();

        Type* Tys[] = { Dst->getType(), Src->getType(), Size->getType() };
        auto* M = MM->getParent()->getParent()->getParent();
        Value* TheFn = Intrinsic::getDeclaration(M, Intrinsic::memcpy, Tys);

        return cast<MemCpyInst>(MemCpyInst::Create(TheFn, args));
    }

    // insertReverseLoop - Insert an empty loop at the end of BB 'Loc'.
    // The loop's induction variable iterates from 'Length'-1 to 0.
    // The return value is the value of the induction variable in the loop's body.
    Instruction* insertReverseLoop(BasicBlock* Loc, BasicBlock* Post, Value* Length, StringRef BBName)
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
            IRBuilder<> B(Pre);
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
            IRBuilder<> B(Body);
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
    Instruction* insertLoop(Instruction* Loc, Value* Length, StringRef BBName)
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
            IRBuilder<> B(Pre);
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
            IRBuilder<> B(Body);
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

    Value* replicateScalar(
        Value* ScalarVal, Type* Ty, Instruction* InsertBefore)
    {
        VectorType* VTy = dyn_cast<VectorType>(Ty);
        Type* ETy = VTy ? VTy->getElementType() : Ty;
        uint32_t sBits = ScalarVal->getType()->getPrimitiveSizeInBits();
        uint32_t nBits = ETy->getPrimitiveSizeInBits();
        assert((nBits % sBits) == 0 && nBits <= 64 && "Type mismatch in replicateScalar!");
        uint32_t ratio = nBits / sBits;

        IRBuilder<> Builder(InsertBefore);
        Value* NewVal;
        if (ratio > 1)
        {
            if (ConstantInt * CI = dyn_cast<ConstantInt>(ScalarVal))
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
    //
    // groupI8Stream() groups a stream of i8 into a stream of <8xi32> as
    // much as possible. Then for the remaining i8's ( < 32), group them
    // into vectors of element type i32 and/or i8. This results in at most
    // the following 5 vectors and/or scalars:
    //    <4xi32>, <3xi32> or <2xi32>, i32, <2xi8>, i8
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

    // Note that Vecs[] should be allocated by callers with enough space
    // to hold all vectors (6 should be enough; 1 for <8xi32>, 5 for the
    // others). 
    void groupI8Stream(
        LLVMContext& C, uint32_t NumI8, uint32_t Align,
        uint32_t& NumI32, Type** Vecs, uint32_t& L)
    {
        NumI32 = NumI8 / 32; // size of <8xi32> = 32
        uint32_t RemI8 = NumI8 % 32;
        uint32_t CntI32 = RemI8 / 4; // the number of i32
        uint32_t CntI8 = RemI8 % 4; // remaining number of i8(0-3)

        Type* TyI32 = Type::getInt32Ty(C);
        Type* TyI8 = Type::getInt8Ty(C);

        uint32_t n = 0;
        Vecs[n++] = VectorType::get(TyI32, 8);
        // CntI32 range [0, 7]
        if (CntI32 >= 4)
        {
            Vecs[n++] = VectorType::get(TyI32, 4);
            CntI32 -= 4;
        }
        if (CntI32 == 3 && Align >= 4)
        {
            Vecs[n++] = VectorType::get(TyI32, 3);
            CntI32 -= 3;
        }
        if (CntI32 >= 2)
        {
            Vecs[n++] = VectorType::get(TyI32, 2);
            CntI32 -= 2;
        }
        if (CntI32 > 0)
        {
            Vecs[n++] = TyI32;
            CntI32 -= 1;
        }
        assert(CntI32 == 0 && "Did not handle all types of i32");

        // CntI8 range [0, 3]
        if (CntI8 >= 2)
        {
            Vecs[n++] = VectorType::get(TyI8, 2);
            CntI8 -= 2;
        }
        if (CntI8 > 0)
        {
            Vecs[n++] = TyI8;
            CntI8 -= 1;
        }
        assert(CntI8 == 0 && "Did not handle all types of i8");

        L = n;
    }

    void replaceMemcpy(IntrinsicInst* I)
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
        //   as much as possible <nxi32> and <mxi8> loads and stores.
        //
        // Selecting 8 as vector length is due to that A64 messages can
        // load eight i32 per SIMD channel. A32 will have 2 loads/stores
        // for each eight i32, which is still efficient. Unaligned vector
        // will be handled correctly and effciently later in vector load
        // and store emit.
        MemCpyInst* MC = cast<MemCpyInst>(I);
        Value* Dst = MC->getRawDest();
        Value* Src = MC->getRawSource();
        Value* LPCount = MC->getLength();
        uint32_t Align = IGCLLVM::getDestAlignment(MC);
        const bool IsVolatile = MC->isVolatile();
        const uint32_t SrcAS = MC->getSourceAddressSpace();
        const uint32_t DstAS = MC->getDestAddressSpace();

        LLVMContext& C = MC->getContext();
        Type* TySrcPtrI8 = Type::getInt8PtrTy(C, SrcAS);
        Type* TyDstPtrI8 = Type::getInt8PtrTy(C, DstAS);

        IRBuilder<> Builder(MC);

        ConstantInt* CI = dyn_cast<ConstantInt>(LPCount);
        if (CI)
        {
            uint32_t Count = (uint32_t)CI->getZExtValue();

            Type* VecTys[8];
            uint32_t Len, NewCount;
            groupI8Stream(C, Count, Align, NewCount, VecTys, Len);

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
                uint32_t SZ = (VecTys[0]->getPrimitiveSizeInBits() / 8);

                // To set alignment correctly
                uint32_t adjust_align = getLargestPowerOfTwo(SZ);
                Align = adjust_align < Align ? adjust_align : Align;

                // If NewCount is less than 6,  don't generate loop.
                // Note that 6 is just an arbitrary number here.
                if (NewCount < 6)
                {
                    for (unsigned i = 0; i < NewCount; ++i)
                    {
                        Value* tSrc = Builder.CreateConstGEP1_32(vSrc, i);
                        Value* tDst = Builder.CreateConstGEP1_32(vDst, i);
                        LoadInst* L = Builder.CreateAlignedLoad(tSrc, Align, IsVolatile);
                        (void)Builder.CreateAlignedStore(L, tDst, Align, IsVolatile);
                    }
                }
                else
                {
                    Value* NewLPCount = ConstantInt::get(LPCount->getType(), NewCount);
                    Instruction* IV = insertLoop(MC, NewLPCount, "memcpy");
                    {
                        IRBuilder<> B(&(*++BasicBlock::iterator(IV)));
                        Value* tSrc = B.CreateGEP(vSrc, IV);
                        Value* tDst = B.CreateGEP(vDst, IV);
                        LoadInst* L = B.CreateAlignedLoad(tSrc, Align, IsVolatile);
                        (void)B.CreateAlignedStore(L, tDst, Align, IsVolatile);
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
                uint32_t SZ = VecTys[i]->getPrimitiveSizeInBits() / 8;
                uint32_t adjust_align = getLargestPowerOfTwo(SZ);
                Align = adjust_align < Align ? adjust_align : Align;
                NewSrc = BOfst > 0 ? Builder.CreateConstGEP1_32(Src, BOfst) : Src;
                NewDst = BOfst > 0 ? Builder.CreateConstGEP1_32(Dst, BOfst) : Dst;
                vSrc = Builder.CreateBitCast(SkipBitCast(NewSrc), PointerType::get(VecTys[i], SrcAS), "memcpy_rem");
                vDst = Builder.CreateBitCast(SkipBitCast(NewDst), PointerType::get(VecTys[i], DstAS), "memcpy_rem");
                LoadInst* L = Builder.CreateAlignedLoad(vSrc, Align, IsVolatile);
                (void)Builder.CreateAlignedStore(L, vDst, Align, IsVolatile);
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
                IRBuilder<> B(&(*++BasicBlock::iterator(IV)));
                Value* tSrc = B.CreateGEP(Src, IV);
                Value* tDst = B.CreateGEP(Dst, IV);
                LoadInst* L = B.CreateAlignedLoad(tSrc, Align, IsVolatile);
                (void)B.CreateAlignedStore(L, tDst, Align, IsVolatile);
            }
        }
        MC->eraseFromParent();
    }

    void replaceMemMove(IntrinsicInst* I)
    {
        MemMoveInst* MM = cast<MemMoveInst>(I);
        Value* Dst = MM->getRawDest();
        Value* Src = MM->getRawSource();
        Value* LPCount = MM->getLength();
        uint32_t Align = IGCLLVM::getDestAlignment(MM);
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

        IRBuilder<> B(MM);

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
            groupI8Stream(C, Count, Align, NewCount, VecTys, Len);

            // for true block (Src < Dst), do a reverse copy.
            {
                B.SetInsertPoint(BBTrue->getTerminator());

                // calculate byte offsets so we can walk backwards through them
                SmallVector<uint, 8> byteOffsets{ 0 };

                {
                    uint32_t SZ = (VecTys[0]->getPrimitiveSizeInBits() / 8);
                    uint32_t BOfst = NewCount * SZ;

                    for (unsigned i = 1; i < Len; i++)
                    {
                        byteOffsets.push_back(BOfst);
                        uint32_t SZ = (VecTys[i]->getPrimitiveSizeInBits() / 8);
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
                    LoadInst* L = B.CreateAlignedLoad(vSrc, newAlign, IsVolatile);
                    (void)B.CreateAlignedStore(L, vDst, newAlign, IsVolatile);
                }

                // now emit the <8 x i32> stores
                auto* vSrc = B.CreateBitCast(SkipBitCast(Src), PointerType::get(VecTys[0], SrcAS), "memcpy_vsrc");
                auto* vDst = B.CreateBitCast(SkipBitCast(Dst), PointerType::get(VecTys[0], DstAS), "memcpy_vdst");
                // If NewCount is less than 6,  don't generate loop.
                // Note that 6 is just an arbitrary number here.
                uint32_t SZ = (VecTys[0]->getPrimitiveSizeInBits() / 8);
                uint32_t newAlign = getLargestPowerOfTwo(Align + SZ);
                if (NewCount < 6)
                {
                    for (unsigned i = 0; i < NewCount; i++)
                    {
                        unsigned idx = NewCount - 1 - i;
                        auto* tSrc = B.CreateConstGEP1_32(vSrc, idx);
                        auto* tDst = B.CreateConstGEP1_32(vDst, idx);
                        LoadInst* L = B.CreateAlignedLoad(tSrc, newAlign, IsVolatile);
                        (void)B.CreateAlignedStore(L, tDst, newAlign, IsVolatile);
                    }
                }
                else
                {
                    auto* NewLPCount = ConstantInt::get(LPCount->getType(), NewCount);
                    Instruction* IV = insertReverseLoop(BBTrue, Post, NewLPCount, "memmmove");
                    {
                        IRBuilder<> B(&(*++BasicBlock::iterator(IV)));
                        Value* tSrc = B.CreateGEP(vSrc, IV);
                        Value* tDst = B.CreateGEP(vDst, IV);
                        LoadInst* L = B.CreateAlignedLoad(tSrc, newAlign, IsVolatile);
                        (void)B.CreateAlignedStore(L, tDst, newAlign, IsVolatile);
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
                    IRBuilder<> B(&(*++BasicBlock::iterator(IV)));
                    Value* tSrc = B.CreateGEP(i8Src, IV);
                    Value* tDst = B.CreateGEP(i8Dst, IV);
                    LoadInst* L = B.CreateAlignedLoad(tSrc, 1, IsVolatile);
                    (void)B.CreateAlignedStore(L, tDst, 1, IsVolatile);
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

    void replaceMemset(IntrinsicInst* I)
    {
        // Same idea as replaceMemcpy (see comment of replaceMemcpy). 
        MemSetInst* MS = cast<MemSetInst>(I);
        Value* Dst = MS->getRawDest();
        Value* Src = MS->getValue();
        Value* LPCount = MS->getLength();
        uint32_t Align = IGCLLVM::getDestAlignment(MS);
        const bool IsVolatile = MS->isVolatile();
        const uint32_t AS = MS->getDestAddressSpace();

        LLVMContext& C = MS->getContext();
        Type* TyPtrI8 = Type::getInt8PtrTy(C, AS);

        IRBuilder<> Builder(MS);

        ConstantInt* CI = dyn_cast<ConstantInt>(LPCount);
        if (CI)
        {
            uint32_t Count = (uint32_t)CI->getZExtValue();

            Type* VecTys[8];
            uint32_t Len, NewCount;
            groupI8Stream(C, Count, Align, NewCount, VecTys, Len);

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
                uint32_t SZ = (VecTys[0]->getPrimitiveSizeInBits() / 8);

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
                        (void)Builder.CreateAlignedStore(vSrc, tDst, Align, IsVolatile);
                    }
                }
                else
                {
                    Value* NewLPCount = ConstantInt::get(LPCount->getType(), NewCount);
                    Instruction* IV = insertLoop(MS, NewLPCount, "memset");
                    {
                        IRBuilder<> B(&(*++BasicBlock::iterator(IV)));
                        Value* tDst = B.CreateGEP(vDst, IV);
                        (void)B.CreateAlignedStore(vSrc, tDst, Align, IsVolatile);
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
                uint32_t SZ = VecTys[i]->getPrimitiveSizeInBits() / 8;
                uint32_t adjust_align = getLargestPowerOfTwo(SZ);
                Align = adjust_align < Align ? adjust_align : Align;
                PointerType* PTy = PointerType::get(VecTys[i], AS);
                NewDst = BOfst > 0 ? Builder.CreateConstGEP1_32(Dst, BOfst) : Dst;
                vSrc = replicateScalar(Src, VecTys[i], MS);
                vDst = Builder.CreateBitCast(SkipBitCast(NewDst), PTy, "memset_rem");
                (void)Builder.CreateAlignedStore(vSrc, vDst, Align, IsVolatile);
                BOfst += SZ;
            }
        }
        else
        {
            Dst = Builder.CreateBitCast(SkipBitCast(Dst), TyPtrI8, "memset_dst");
            // Fall back to i8 copy
            Instruction* IV = insertLoop(MS, LPCount, "memset");
            {
                IRBuilder<> B(&(*++BasicBlock::iterator(IV)));
                Value* tDst = B.CreateGEP(Dst, IV);
                (void)B.CreateAlignedStore(Src, tDst, Align, IsVolatile);
            }
        }
        MS->eraseFromParent();
    }

    void replaceExpect(IntrinsicInst* MS)
    {
        MS->replaceAllUsesWith(MS->getOperand(0));
        MS->eraseFromParent();
    }

#if LLVM_VERSION_MAJOR >= 8
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
    void replaceFunnelShift(IntrinsicInst * I) {
        assert(I->getIntrinsicID() == Intrinsic::fshl ||
            I->getIntrinsicID() == Intrinsic::fshr);
        IRBuilder<> Builder(I);
        unsigned sizeInBits = I->getArgOperand(0)->getType()->getScalarSizeInBits();
        Value* numBits = Builder.getIntN(sizeInBits, sizeInBits);
        if (I->getType()->isVectorTy()) {
            numBits = ConstantVector::getSplat(I->getType()->getVectorNumElements(), cast<Constant>(numBits));
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
#endif

    std::map<Intrinsic::ID, std::function<void(IntrinsicInst*)>> intrinsicToFunc = {
  #if LLVM_VERSION_MAJOR >= 8
      { Intrinsic::fshl, replaceFunnelShift },
      { Intrinsic::fshr, replaceFunnelShift },
  #endif
      { Intrinsic::memcpy, replaceMemcpy },
      { Intrinsic::memset, replaceMemset },
      { Intrinsic::memmove, replaceMemMove },
      { Intrinsic::expect, replaceExpect }
    };
} // end anonymous namespace

void ReplaceUnsupportedIntrinsics::visitIntrinsicInst(IntrinsicInst& I) {
    if (intrinsicToFunc.find(I.getIntrinsicID()) != intrinsicToFunc.end()) {
        m_instsToReplace.push_back(&I);
    }
}

bool ReplaceUnsupportedIntrinsics::runOnFunction(Function& F)
{
    m_instsToReplace.clear();
    visit(F);
    for (auto I : m_instsToReplace) {
        intrinsicToFunc.at(I->getIntrinsicID())(I);
    }
    return !m_instsToReplace.empty();
}
