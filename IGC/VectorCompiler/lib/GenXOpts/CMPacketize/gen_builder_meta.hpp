/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// @file gen_builder_meta.hpp
//
// @brief auto-generated file
//
// DO NOT EDIT
//
// Generation Command Line:
//  gen_llvm_ir_macros.py
//    --input
//    /cygdrive/d/cm-llvm/llvm/include/llvm/IR/IRBuilder.h
//    --output-dir
//    .
//    --gen_h
//    --gen_meta_h
//    --gen_intrin_h
//
//============================================================================
// clang-format off
#pragma once

//============================================================================
// Auto-generated meta intrinsics
//============================================================================
Value* VGATHERPD(Value* src, Value* pBase, Value* indices, Value* mask, Value* scale, const llvm::Twine& name = "")
{
    SmallVector<Type*, 5> argTypes;
    argTypes.push_back(src->getType());
    argTypes.push_back(pBase->getType());
    argTypes.push_back(indices->getType());
    argTypes.push_back(mask->getType());
    argTypes.push_back(scale->getType());
    FunctionType* pFuncTy = FunctionType::get(src->getType(), argTypes, false);
    Function* pFunc = cast<Function>(mpModule->getOrInsertFunction("meta.intrinsic.VGATHERPD", pFuncTy));
    return CALL(pFunc, std::initializer_list<Value*>{src, pBase, indices, mask, scale}, name);
}

Value* VGATHERPS(Value* src, Value* pBase, Value* indices, Value* mask, Value* scale, const llvm::Twine& name = "")
{
    SmallVector<Type*, 5> argTypes;
    argTypes.push_back(src->getType());
    argTypes.push_back(pBase->getType());
    argTypes.push_back(indices->getType());
    argTypes.push_back(mask->getType());
    argTypes.push_back(scale->getType());
    FunctionType* pFuncTy = FunctionType::get(src->getType(), argTypes, false);
    Function* pFunc = cast<Function>(mpModule->getOrInsertFunction("meta.intrinsic.VGATHERPS", pFuncTy));
    return CALL(pFunc, std::initializer_list<Value*>{src, pBase, indices, mask, scale}, name);
}

Value* VGATHERDD(Value* src, Value* pBase, Value* indices, Value* mask, Value* scale, const llvm::Twine& name = "")
{
    SmallVector<Type*, 5> argTypes;
    argTypes.push_back(src->getType());
    argTypes.push_back(pBase->getType());
    argTypes.push_back(indices->getType());
    argTypes.push_back(mask->getType());
    argTypes.push_back(scale->getType());
    FunctionType* pFuncTy = FunctionType::get(src->getType(), argTypes, false);
    Function* pFunc = cast<Function>(mpModule->getOrInsertFunction("meta.intrinsic.VGATHERDD", pFuncTy));
    return CALL(pFunc, std::initializer_list<Value*>{src, pBase, indices, mask, scale}, name);
}

Value* VRCPPS(Value* a, const llvm::Twine& name = "")
{
    SmallVector<Type*, 1> argTypes;
    argTypes.push_back(a->getType());
    FunctionType* pFuncTy = FunctionType::get(a->getType(), argTypes, false);
    Function* pFunc = cast<Function>(mpModule->getOrInsertFunction("meta.intrinsic.VRCPPS", pFuncTy));
    return CALL(pFunc, std::initializer_list<Value*>{a}, name);
}

Value* VROUND(Value* a, Value* rounding, const llvm::Twine& name = "")
{
    SmallVector<Type*, 2> argTypes;
    argTypes.push_back(a->getType());
    argTypes.push_back(rounding->getType());
    FunctionType* pFuncTy = FunctionType::get(a->getType(), argTypes, false);
    Function* pFunc = cast<Function>(mpModule->getOrInsertFunction("meta.intrinsic.VROUND", pFuncTy));
    return CALL(pFunc, std::initializer_list<Value*>{a, rounding}, name);
}

Value* BEXTR_32(Value* src, Value* control, const llvm::Twine& name = "")
{
    SmallVector<Type*, 2> argTypes;
    argTypes.push_back(src->getType());
    argTypes.push_back(control->getType());
    FunctionType* pFuncTy = FunctionType::get(src->getType(), argTypes, false);
    Function* pFunc = cast<Function>(mpModule->getOrInsertFunction("meta.intrinsic.BEXTR_32", pFuncTy));
    return CALL(pFunc, std::initializer_list<Value*>{src, control}, name);
}

Value* VPSHUFB(Value* a, Value* b, const llvm::Twine& name = "")
{
    SmallVector<Type*, 2> argTypes;
    argTypes.push_back(a->getType());
    argTypes.push_back(b->getType());
    FunctionType* pFuncTy = FunctionType::get(a->getType(), argTypes, false);
    Function* pFunc = cast<Function>(mpModule->getOrInsertFunction("meta.intrinsic.VPSHUFB", pFuncTy));
    return CALL(pFunc, std::initializer_list<Value*>{a, b}, name);
}

Value* VPERMD(Value* a, Value* idx, const llvm::Twine& name = "")
{
    SmallVector<Type*, 2> argTypes;
    argTypes.push_back(a->getType());
    argTypes.push_back(idx->getType());
    FunctionType* pFuncTy = FunctionType::get(a->getType(), argTypes, false);
    Function* pFunc = cast<Function>(mpModule->getOrInsertFunction("meta.intrinsic.VPERMD", pFuncTy));
    return CALL(pFunc, std::initializer_list<Value*>{a, idx}, name);
}

Value* VPERMPS(Value* idx, Value* a, const llvm::Twine& name = "")
{
    SmallVector<Type*, 2> argTypes;
    argTypes.push_back(idx->getType());
    argTypes.push_back(a->getType());
    FunctionType* pFuncTy = FunctionType::get(a->getType(), argTypes, false);
    Function* pFunc = cast<Function>(mpModule->getOrInsertFunction("meta.intrinsic.VPERMPS", pFuncTy));
    return CALL(pFunc, std::initializer_list<Value*>{idx, a}, name);
}

Value* VCVTPD2PS(Value* a, const llvm::Twine& name = "")
{
    SmallVector<Type*, 1> argTypes;
    argTypes.push_back(a->getType());
    FunctionType* pFuncTy = FunctionType::get(IGCLLVM::FixedVectorType::get(mFP32Ty, cast<IGCLLVM::FixedVectorType>(a->getType())->getNumElements()), argTypes, false);
    Function* pFunc = cast<Function>(mpModule->getOrInsertFunction("meta.intrinsic.VCVTPD2PS", pFuncTy));
    return CALL(pFunc, std::initializer_list<Value*>{a}, name);
}

Value* VCVTPH2PS(Value* a, const llvm::Twine& name = "")
{
    SmallVector<Type*, 1> argTypes;
    argTypes.push_back(a->getType());
    FunctionType* pFuncTy = FunctionType::get(IGCLLVM::FixedVectorType::get(mFP32Ty, cast<IGCLLVM::FixedVectorType>(a->getType())->getNumElements()), argTypes, false);
    Function* pFunc = cast<Function>(mpModule->getOrInsertFunction("meta.intrinsic.VCVTPH2PS", pFuncTy));
    return CALL(pFunc, std::initializer_list<Value*>{a}, name);
}

Value* VCVTPS2PH(Value* a, Value* round, const llvm::Twine& name = "")
{
    SmallVector<Type*, 2> argTypes;
    argTypes.push_back(a->getType());
    argTypes.push_back(round->getType());
    FunctionType* pFuncTy = FunctionType::get(mSimdInt16Ty, argTypes, false);
    Function* pFunc = cast<Function>(mpModule->getOrInsertFunction("meta.intrinsic.VCVTPS2PH", pFuncTy));
    return CALL(pFunc, std::initializer_list<Value*>{a, round}, name);
}

Value* VHSUBPS(Value* a, Value* b, const llvm::Twine& name = "")
{
    SmallVector<Type*, 2> argTypes;
    argTypes.push_back(a->getType());
    argTypes.push_back(b->getType());
    FunctionType* pFuncTy = FunctionType::get(a->getType(), argTypes, false);
    Function* pFunc = cast<Function>(mpModule->getOrInsertFunction("meta.intrinsic.VHSUBPS", pFuncTy));
    return CALL(pFunc, std::initializer_list<Value*>{a, b}, name);
}

Value* VPTESTC(Value* a, Value* b, const llvm::Twine& name = "")
{
    SmallVector<Type*, 2> argTypes;
    argTypes.push_back(a->getType());
    argTypes.push_back(b->getType());
    FunctionType* pFuncTy = FunctionType::get(mInt32Ty, argTypes, false);
    Function* pFunc = cast<Function>(mpModule->getOrInsertFunction("meta.intrinsic.VPTESTC", pFuncTy));
    return CALL(pFunc, std::initializer_list<Value*>{a, b}, name);
}

Value* VPTESTZ(Value* a, Value* b, const llvm::Twine& name = "")
{
    SmallVector<Type*, 2> argTypes;
    argTypes.push_back(a->getType());
    argTypes.push_back(b->getType());
    FunctionType* pFuncTy = FunctionType::get(mInt32Ty, argTypes, false);
    Function* pFunc = cast<Function>(mpModule->getOrInsertFunction("meta.intrinsic.VPTESTZ", pFuncTy));
    return CALL(pFunc, std::initializer_list<Value*>{a, b}, name);
}

Value* VFMADDPS(Value* a, Value* b, Value* c, const llvm::Twine& name = "")
{
    SmallVector<Type*, 3> argTypes;
    argTypes.push_back(a->getType());
    argTypes.push_back(b->getType());
    argTypes.push_back(c->getType());
    FunctionType* pFuncTy = FunctionType::get(a->getType(), argTypes, false);
    Function* pFunc = cast<Function>(mpModule->getOrInsertFunction("meta.intrinsic.VFMADDPS", pFuncTy));
    return CALL(pFunc, std::initializer_list<Value*>{a, b, c}, name);
}

Value* VPHADDD(Value* a, Value* b, const llvm::Twine& name = "")
{
    SmallVector<Type*, 2> argTypes;
    argTypes.push_back(a->getType());
    argTypes.push_back(b->getType());
    FunctionType* pFuncTy = FunctionType::get(a->getType(), argTypes, false);
    Function* pFunc = cast<Function>(mpModule->getOrInsertFunction("meta.intrinsic.VPHADDD", pFuncTy));
    return CALL(pFunc, std::initializer_list<Value*>{a, b}, name);
}

Value* PDEP32(Value* a, Value* b, const llvm::Twine& name = "")
{
    SmallVector<Type*, 2> argTypes;
    argTypes.push_back(a->getType());
    argTypes.push_back(b->getType());
    FunctionType* pFuncTy = FunctionType::get(a->getType(), argTypes, false);
    Function* pFunc = cast<Function>(mpModule->getOrInsertFunction("meta.intrinsic.PDEP32", pFuncTy));
    return CALL(pFunc, std::initializer_list<Value*>{a, b}, name);
}

Value* RDTSC(const llvm::Twine& name = "")
{
    FunctionType* pFuncTy = FunctionType::get(mInt64Ty, {}, false);
    Function* pFunc = cast<Function>(mpModule->getOrInsertFunction("meta.intrinsic.RDTSC", pFuncTy));
    return CALL(pFunc, std::initializer_list<Value*>{}, name);
}

    // clang-format on
