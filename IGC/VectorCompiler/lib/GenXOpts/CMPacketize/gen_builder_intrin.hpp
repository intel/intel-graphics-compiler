/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef GEN_BUILDER_INTRIN_HPP
#define GEN_BUILDER_INTRIN_HPP

Value *CTTZ(Value *A, Value *Flag, const llvm::Twine &Name = "") {
  SmallVector<Type *, 1> Args;
  Args.push_back(A->getType());
  auto Decl = Intrinsic::getDeclaration(M, Intrinsic::cttz, Args);
  return CALL(Decl, std::initializer_list<Value *>{A, Flag}, Name);
}

Value *CTLZ(Value *A, Value *Flag, const llvm::Twine &Name = "") {
  SmallVector<Type *, 1> Args;
  Args.push_back(A->getType());
  auto Decl = Intrinsic::getDeclaration(M, Intrinsic::ctlz, Args);
  return CALL(Decl, std::initializer_list<Value *>{A, Flag}, Name);
}

Value *VSQRTPS(Value *A, const llvm::Twine &Name = "") {
  SmallVector<Type *, 1> Args;
  Args.push_back(A->getType());
  auto Decl = Intrinsic::getDeclaration(M, Intrinsic::sqrt, Args);
  return CALL(Decl, std::initializer_list<Value *>{A}, Name);
}

Value *STACKSAVE(const llvm::Twine &Name = "") {
  auto Decl = Intrinsic::getDeclaration(M, Intrinsic::stacksave);
  return CALL(Decl, std::initializer_list<Value *>{}, Name);
}

Value *STACKRESTORE(Value *A, const llvm::Twine &Name = "") {
  auto Decl = Intrinsic::getDeclaration(M, Intrinsic::stackrestore);
  return CALL(Decl, std::initializer_list<Value *>{A}, Name);
}

Value *VMINPS(Value *A, Value *B, const llvm::Twine &Name = "") {
  SmallVector<Type *, 1> Args;
  Args.push_back(A->getType());
  auto Decl = Intrinsic::getDeclaration(M, Intrinsic::minnum, Args);
  return CALL(Decl, std::initializer_list<Value *>{A, B}, Name);
}

Value *VMAXPS(Value *A, Value *B, const llvm::Twine &Name = "") {
  SmallVector<Type *, 1> Args;
  Args.push_back(A->getType());
  auto Decl = Intrinsic::getDeclaration(M, Intrinsic::maxnum, Args);
  return CALL(Decl, std::initializer_list<Value *>{A, B}, Name);
}

Value *DEBUGTRAP(const llvm::Twine &Name = "") {
  auto Decl = Intrinsic::getDeclaration(M, Intrinsic::debugtrap);
  return CALL(Decl, std::initializer_list<Value *>{}, Name);
}

Value *POPCNT(Value *A, const llvm::Twine &Name = "") {
  SmallVector<Type *, 1> Args;
  Args.push_back(A->getType());
  auto Decl = Intrinsic::getDeclaration(M, Intrinsic::ctpop, Args);
  return CALL(Decl, std::initializer_list<Value *>{A}, Name);
}

Value *LOG2(Value *A, const llvm::Twine &Name = "") {
  SmallVector<Type *, 1> Args;
  Args.push_back(A->getType());
  auto Decl = Intrinsic::getDeclaration(M, Intrinsic::log2, Args);
  return CALL(Decl, std::initializer_list<Value *>{A}, Name);
}

Value *FABS(Value *A, const llvm::Twine &Name = "") {
  SmallVector<Type *, 1> Args;
  Args.push_back(A->getType());
  auto Decl = Intrinsic::getDeclaration(M, Intrinsic::fabs, Args);
  return CALL(Decl, std::initializer_list<Value *>{A}, Name);
}

Value *EXP2(Value *A, const llvm::Twine &Name = "") {
  SmallVector<Type *, 1> Args;
  Args.push_back(A->getType());
  auto Decl = Intrinsic::getDeclaration(M, Intrinsic::exp2, Args);
  return CALL(Decl, std::initializer_list<Value *>{A}, Name);
}

Value *COS(Value *A, const llvm::Twine &Name = "") {
  SmallVector<Type *, 1> Args;
  Args.push_back(A->getType());
  auto Decl = Intrinsic::getDeclaration(M, Intrinsic::cos, Args);
  return CALL(Decl, std::initializer_list<Value *>{A}, Name);
}

Value *SIN(Value *A, const llvm::Twine &Name = "") {
  SmallVector<Type *, 1> Args;
  Args.push_back(A->getType());
  auto Decl = Intrinsic::getDeclaration(M, Intrinsic::sin, Args);
  return CALL(Decl, std::initializer_list<Value *>{A}, Name);
}

Value *FLOOR(Value *A, const llvm::Twine &Name = "") {
  SmallVector<Type *, 1> Args;
  Args.push_back(A->getType());
  auto Decl = Intrinsic::getDeclaration(M, Intrinsic::floor, Args);
  return CALL(Decl, std::initializer_list<Value *>{A}, Name);
}

Value *POW(Value *A, Value *B, const llvm::Twine &Name = "") {
  SmallVector<Type *, 1> Args;
  Args.push_back(A->getType());
  auto Decl = Intrinsic::getDeclaration(M, Intrinsic::pow, Args);
  return CALL(Decl, std::initializer_list<Value *>{A, B}, Name);
}

#endif // GEN_BUILDER_INTRIN_HPP
