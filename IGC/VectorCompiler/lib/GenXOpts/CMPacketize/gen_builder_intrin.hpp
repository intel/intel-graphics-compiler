/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// @file gen_builder_intrin.hpp
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
// Auto-generated llvm intrinsics
//============================================================================
Value* CTTZ(Value* a, Value* flag, const llvm::Twine& name = "")
{
    SmallVector<Type*, 1> args;
    args.push_back(a->getType());
    Function* pFunc = Intrinsic::getDeclaration(mpModule, Intrinsic::cttz, args);
    return CALL(pFunc, std::initializer_list<Value*>{a, flag}, name);
}

Value* CTLZ(Value* a, Value* flag, const llvm::Twine& name = "")
{
    SmallVector<Type*, 1> args;
    args.push_back(a->getType());
    Function* pFunc = Intrinsic::getDeclaration(mpModule, Intrinsic::ctlz, args);
    return CALL(pFunc, std::initializer_list<Value*>{a, flag}, name);
}

Value* VSQRTPS(Value* a, const llvm::Twine& name = "")
{
    SmallVector<Type*, 1> args;
    args.push_back(a->getType());
    Function* pFunc = Intrinsic::getDeclaration(mpModule, Intrinsic::sqrt, args);
    return CALL(pFunc, std::initializer_list<Value*>{a}, name);
}

Value* STACKSAVE(const llvm::Twine& name = "")
{
    Function* pFunc = Intrinsic::getDeclaration(mpModule, Intrinsic::stacksave);
    return CALL(pFunc, std::initializer_list<Value*>{}, name);
}

Value* STACKRESTORE(Value* a, const llvm::Twine& name = "")
{
    Function* pFunc = Intrinsic::getDeclaration(mpModule, Intrinsic::stackrestore);
    return CALL(pFunc, std::initializer_list<Value*>{a}, name);
}

Value* VMINPS(Value* a, Value* b, const llvm::Twine& name = "")
{
    SmallVector<Type*, 1> args;
    args.push_back(a->getType());
    Function* pFunc = Intrinsic::getDeclaration(mpModule, Intrinsic::minnum, args);
    return CALL(pFunc, std::initializer_list<Value*>{a, b}, name);
}

Value* VMAXPS(Value* a, Value* b, const llvm::Twine& name = "")
{
    SmallVector<Type*, 1> args;
    args.push_back(a->getType());
    Function* pFunc = Intrinsic::getDeclaration(mpModule, Intrinsic::maxnum, args);
    return CALL(pFunc, std::initializer_list<Value*>{a, b}, name);
}

Value* DEBUGTRAP(const llvm::Twine& name = "")
{
    Function* pFunc = Intrinsic::getDeclaration(mpModule, Intrinsic::debugtrap);
    return CALL(pFunc, std::initializer_list<Value*>{}, name);
}

Value* POPCNT(Value* a, const llvm::Twine& name = "")
{
    SmallVector<Type*, 1> args;
    args.push_back(a->getType());
    Function* pFunc = Intrinsic::getDeclaration(mpModule, Intrinsic::ctpop, args);
    return CALL(pFunc, std::initializer_list<Value*>{a}, name);
}

Value* LOG2(Value* a, const llvm::Twine& name = "")
{
    SmallVector<Type*, 1> args;
    args.push_back(a->getType());
    Function* pFunc = Intrinsic::getDeclaration(mpModule, Intrinsic::log2, args);
    return CALL(pFunc, std::initializer_list<Value*>{a}, name);
}

Value* FABS(Value* a, const llvm::Twine& name = "")
{
    SmallVector<Type*, 1> args;
    args.push_back(a->getType());
    Function* pFunc = Intrinsic::getDeclaration(mpModule, Intrinsic::fabs, args);
    return CALL(pFunc, std::initializer_list<Value*>{a}, name);
}

Value* EXP2(Value* a, const llvm::Twine& name = "")
{
    SmallVector<Type*, 1> args;
    args.push_back(a->getType());
    Function* pFunc = Intrinsic::getDeclaration(mpModule, Intrinsic::exp2, args);
    return CALL(pFunc, std::initializer_list<Value*>{a}, name);
}

Value* COS(Value* a, const llvm::Twine& name = "")
{
    SmallVector<Type*, 1> args;
    args.push_back(a->getType());
    Function* pFunc = Intrinsic::getDeclaration(mpModule, Intrinsic::cos, args);
    return CALL(pFunc, std::initializer_list<Value*>{a}, name);
}

Value* SIN(Value* a, const llvm::Twine& name = "")
{
    SmallVector<Type*, 1> args;
    args.push_back(a->getType());
    Function* pFunc = Intrinsic::getDeclaration(mpModule, Intrinsic::sin, args);
    return CALL(pFunc, std::initializer_list<Value*>{a}, name);
}

Value* FLOOR(Value* a, const llvm::Twine& name = "")
{
    SmallVector<Type*, 1> args;
    args.push_back(a->getType());
    Function* pFunc = Intrinsic::getDeclaration(mpModule, Intrinsic::floor, args);
    return CALL(pFunc, std::initializer_list<Value*>{a}, name);
}

Value* POW(Value* a, Value* b, const llvm::Twine& name = "")
{
    SmallVector<Type*, 1> args;
    args.push_back(a->getType());
    Function* pFunc = Intrinsic::getDeclaration(mpModule, Intrinsic::pow, args);
    return CALL(pFunc, std::initializer_list<Value*>{a, b}, name);
}

    // clang-format on
