/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PacketBuilder.h"
#include <llvm/IR/DerivedTypes.h>

// need to disable this to use INFINITY and NAN values
#pragma warning(disable : 4756 4056)

#include <math.h>

namespace pktz {
//////////////////////////////////////////////////////////////////////////
/// @brief Computes log2(A) using either scalar log2 function from the runtime
///        or vector approximation
/// @param A - src float vector
Value *PacketBuilder::VLOG2PS(Value *A) {
  Value *Result = nullptr;
  // fast log2 approximation
  // log2(x) = (x.ExpPart - 127) + log(1.xFracPart)
  auto *AsInt = BITCAST(A, SimdInt32Ty);
  auto *B = SUB(AND(ASHR(AsInt, 23), 255), VIMMED1(127));
  auto *IntermResult = SI_TO_FP(B, SimdFP32Ty);
  auto *Fa = OR(AND(AsInt, VIMMED1(0x007FFFFF)), VIMMED1(127 << 23));
  Fa = BITCAST(Fa, SimdFP32Ty);
  Fa = FSUB(Fa, VIMMED1(1.0f));
  // log(x) = (1.4386183024320163f + (-0.640238532500937f +
  // 0.20444600983623412f*fx)*fx)*fx;
  Result = FMUL(Fa, VIMMED1(0.20444600983623412f));
  Result = FADD(Result, VIMMED1(-0.640238532500937f));
  Result = FMUL(Fa, Result);
  Result = FADD(Result, VIMMED1(1.4386183024320163f));
  Result = FMUL(Result, Fa);
  Result = FADD(Result, IntermResult);
  // handle bad input
  // 0 -> -inf
  auto *ZeroInput = FCMP_OEQ(A, VIMMED1(0.0f));
  Result = SELECT(ZeroInput, VIMMED1(-INFINITY), Result);
  // -F -> NAN
  auto *NegInput = FCMP_OLT(A, VIMMED1(0.0f));
  Result = SELECT(NegInput, VIMMED1(NAN), Result);
  // inf -> inf
  auto *InfInput = FCMP_OEQ(A, VIMMED1(INFINITY));
  Result = SELECT(InfInput, VIMMED1(INFINITY), Result);
  // NAN -> NAN
  auto *NanInput = FCMP_UNO(A, A);
  Result = SELECT(NanInput, VIMMED1(NAN), Result);
  Result->setName("log2.");
  return Result;
}

#define EXP_POLY_DEGREE 3

#define POLY0(x, c0) VIMMED1(c0)
#define POLY1(x, c0, c1) FADD(FMUL(POLY0(x, c1), x), VIMMED1(c0))
#define POLY2(x, c0, c1, c2) FADD(FMUL(POLY1(x, c1, c2), x), VIMMED1(c0))
#define POLY3(x, c0, c1, c2, c3)                                               \
  FADD(FMUL(POLY2(x, c1, c2, c3), x), VIMMED1(c0))
#define POLY4(x, c0, c1, c2, c3, c4)                                           \
  FADD(FMUL(POLY3(x, c1, c2, c3, c4), x), VIMMED1(c0))
#define POLY5(x, c0, c1, c2, c3, c4, c5)                                       \
  FADD(FMUL(POLY4(x, c1, c2, c3, c4, c5), x), VIMMED1(c0))

//////////////////////////////////////////////////////////////////////////
/// @brief Computes 2^A using either scalar pow function from the runtime
///        or vector approximation
/// @param A - src float vector
Value *PacketBuilder::VEXP2PS(Value *A) {
  Value *Result = nullptr;
  // fast exp2 taken from here:
  // http://jrfonseca.blogspot.com/2008/09/fast-sse2-pow-tables-or-polynomials.html
  A = VMINPS(A, VIMMED1(129.0f));
  A = VMAXPS(A, VIMMED1(-126.99999f));
  auto *IPart = FP_TO_SI(FSUB(A, VIMMED1(0.5f)), SimdInt32Ty);
  auto *FPart = FSUB(A, SI_TO_FP(IPart, SimdFP32Ty));
  auto *ExpIPart = BITCAST(SHL(ADD(IPart, VIMMED1(127)), 23), SimdFP32Ty);
#if EXP_POLY_DEGREE == 5
  auto *ExpFPart = POLY5(FPart, 9.9999994e-1f, 6.9315308e-1f, 2.4015361e-1f,
                         5.5826318e-2f, 8.9893397e-3f, 1.8775767e-3f);
#elif EXP_POLY_DEGREE == 4
  auto *ExpFPart = POLY4(FPart, 1.0000026f, 6.9300383e-1f, 2.4144275e-1f,
                         5.2011464e-2f, 1.3534167e-2f);
#elif EXP_POLY_DEGREE == 3
  auto *ExpFPart =
      POLY3(FPart, 9.9992520e-1f, 6.9583356e-1f, 2.2606716e-1f, 7.8024521e-2f);
#elif EXP_POLY_DEGREE == 2
  auto *ExpFPart = POLY2(FPart, 1.0017247f, 6.5763628e-1f, 3.3718944e-1f);
#endif // EXP_POLY_DEGREE
  Result = FMUL(ExpIPart, ExpFPart, "exp2.");
  return Result;
}

Value *PacketBuilder::ADD(Value *LHS, Value *RHS, const Twine &Name,
                          bool HasNUW, bool HasNSW) {
  return IRB->CreateAdd(LHS, RHS, Name, HasNUW, HasNSW);
}

Value *PacketBuilder::AND(Value *LHS, Value *RHS, const Twine &Name) {
  return IRB->CreateAnd(LHS, RHS, Name);
}

Value *PacketBuilder::AND(Value *LHS, uint64_t RHS, const Twine &Name) {
  return IRB->CreateAnd(LHS, RHS, Name);
}

Value *PacketBuilder::ASHR(Value *LHS, uint64_t RHS, const Twine &Name,
                           bool IsExact) {
  return IRB->CreateAShr(LHS, RHS, Name, IsExact);
}

Value *PacketBuilder::EXP2(Value *A, const llvm::Twine &Name) {
  SmallVector<Type *, 1> Args;
  Args.push_back(A->getType());
  auto Decl = llvm::FunctionCallee(Intrinsic::getDeclaration(M, Intrinsic::exp2, Args));
  return CALL(&Decl, std::initializer_list<Value *>{A}, Name);
}

Value *PacketBuilder::FABS(Value *A, const llvm::Twine &Name) {
  SmallVector<Type *, 1> Args;
  Args.push_back(A->getType());
  auto Decl = llvm::FunctionCallee(Intrinsic::getDeclaration(M, Intrinsic::fabs, Args));
  return CALL(&Decl, std::initializer_list<Value *>{A}, Name);
}

Value *PacketBuilder::FADD(Value *LHS, Value *RHS, const Twine &Name,
                           MDNode *FPMathTag) {
  return IRB->CreateFAdd(LHS, RHS, Name, FPMathTag);
}

Value *PacketBuilder::FCMP_OEQ(Value *LHS, Value *RHS, const Twine &Name,
                               MDNode *FPMathTag) {
  return IRB->CreateFCmpOEQ(LHS, RHS, Name, FPMathTag);
}

Value *PacketBuilder::FCMP_OLT(Value *LHS, Value *RHS, const Twine &Name,
                               MDNode *FPMathTag) {
  return IRB->CreateFCmpOLT(LHS, RHS, Name, FPMathTag);
}

Value *PacketBuilder::FCMP_UNO(Value *LHS, Value *RHS, const Twine &Name,
                               MDNode *FPMathTag) {
  return IRB->CreateFCmpUNO(LHS, RHS, Name, FPMathTag);
}

Value *PacketBuilder::FMUL(Value *LHS, Value *RHS, const Twine &Name,
                           MDNode *FPMathTag) {
  return IRB->CreateFMul(LHS, RHS, Name, FPMathTag);
}

Value *PacketBuilder::FP_TO_SI(Value *V, Type *DestTy, const Twine &Name) {
  return IRB->CreateFPToSI(V, DestTy, Name);
}

Value *PacketBuilder::FSUB(Value *LHS, Value *RHS, const Twine &Name,
                           MDNode *FPMathTag) {
  return IRB->CreateFSub(LHS, RHS, Name, FPMathTag);
}

Value *PacketBuilder::MUL(Value *LHS, Value *RHS, const Twine &Name,
                          bool HasNUW, bool HasNSW) {
  return IRB->CreateMul(LHS, RHS, Name, HasNUW, HasNSW);
}

Value *PacketBuilder::NOT(Value *V, const Twine &Name) {
  return IRB->CreateNot(V, Name);
}

Value *PacketBuilder::OR(Value *LHS, Value *RHS, const Twine &Name) {
  return IRB->CreateOr(LHS, RHS, Name);
}

Value *PacketBuilder::SHL(Value *LHS, Value *RHS, const Twine &Name,
                          bool HasNUW, bool HasNSW) {
  return IRB->CreateShl(LHS, RHS, Name, HasNUW, HasNSW);
}

Value *PacketBuilder::SHL(Value *LHS, uint64_t RHS, const Twine &Name,
                          bool HasNUW, bool HasNSW) {
  return IRB->CreateShl(LHS, RHS, Name, HasNUW, HasNSW);
}

Value *PacketBuilder::SI_TO_FP(Value *V, Type *DestTy, const Twine &Name) {
  return IRB->CreateSIToFP(V, DestTy, Name);
}

Value *PacketBuilder::SUB(Value *LHS, Value *RHS, const Twine &Name,
                          bool HasNUW, bool HasNSW) {
  return IRB->CreateSub(LHS, RHS, Name, HasNUW, HasNSW);
}

Value *PacketBuilder::S_EXT(Value *V, Type *DestTy, const Twine &Name) {
  return IRB->CreateSExt(V, DestTy, Name);
}

Value *PacketBuilder::TRUNC(Value *V, Type *DestTy, const Twine &Name) {
  return IRB->CreateTrunc(V, DestTy, Name);
}

Value *PacketBuilder::VMINPS(Value *A, Value *B, const llvm::Twine &Name) {
  SmallVector<Type *, 1> Args;
  Args.push_back(A->getType());
  auto Decl = llvm::FunctionCallee(Intrinsic::getDeclaration(M, Intrinsic::minnum, Args));
  return CALL(&Decl, std::initializer_list<Value *>{A, B}, Name);
}

Value *PacketBuilder::VMAXPS(Value *A, Value *B, const llvm::Twine &Name) {
  SmallVector<Type *, 1> Args;
  Args.push_back(A->getType());
  auto Decl = llvm::FunctionCallee(Intrinsic::getDeclaration(M, Intrinsic::maxnum, Args));
  return CALL(&Decl, std::initializer_list<Value *>{A, B}, Name);
}

Value *PacketBuilder::VSQRTPS(Value *A, const llvm::Twine &Name) {
  SmallVector<Type *, 1> Args;
  Args.push_back(A->getType());
  auto Decl = llvm::FunctionCallee(Intrinsic::getDeclaration(M, Intrinsic::sqrt, Args));
  return CALL(&Decl, std::initializer_list<Value *>{A}, Name);
}

Value *PacketBuilder::UI_TO_FP(Value *V, Type *DestTy, const Twine &Name) {
  return IRB->CreateUIToFP(V, DestTy, Name);
}
} // namespace pktz
