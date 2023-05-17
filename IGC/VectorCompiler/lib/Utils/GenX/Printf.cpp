/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/GenX/Printf.h"
#include "vc/Utils/General/IRBuilder.h"
#include "vc/Utils/General/RegexIterator.h"
#include "vc/Utils/General/Types.h"

#include "vc/InternalIntrinsics/InternalIntrinsics.h"

#include <llvm/ADT/Optional.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/Regex.h>

#include <algorithm>
#include <iterator>
#include <string>

#include "Probe/Assertion.h"
#include "llvmWrapper/ADT/StringRef.h"
#include "llvmWrapper/IR/Operator.h"
#include "llvmWrapper/Support/Regex.h"

using namespace llvm;
using namespace vc;

// extracts underlying c-string from provided constant
static StringRef extractCStr(const Constant &CStrConst) {
  if (isa<ConstantDataArray>(CStrConst))
    return cast<ConstantDataArray>(CStrConst).getAsCString();
  IGC_ASSERT(isa<ConstantAggregateZero>(CStrConst));
  return "";
}

bool vc::isConstantString(const GlobalVariable &GV) {
  if (!GV.isConstant())
    return false;
  // FIXME: Check namespace, it should be constant or global. Though it is not
  //        possible to check it right now (CM has no addrspaces).
  if (!GV.getValueType()->isArrayTy())
    return false;
  return GV.getValueType()->getArrayElementType()->isIntegerTy(8);
}

bool vc::isConstantString(const Value &V) {
  if (!isa<GlobalVariable>(V))
    return false;
  return isConstantString(cast<GlobalVariable>(V));
}

bool vc::isConstantStringFirstElementGEP(const GEPOperator &GEP) {
  auto *Ptr = GEP.getPointerOperand();
  if (!isConstantString(*Ptr))
    return false;
  if (GEP.getNumIndices() != 2)
    return false;
  return GEP.hasAllZeroIndices();
}

bool vc::isConstantStringFirstElementGEP(const Value &V) {
  if (!isa<GEPOperator>(V))
    return false;
  return isConstantStringFirstElementGEP(cast<GEPOperator>(V));
}

static const Value &ignoreCastToGenericAS(const Value &Op) {
  if (isCastToGenericAS(Op))
    return *cast<IGCLLVM::AddrSpaceCastOperator>(Op).getPointerOperand();
  return Op;
}

const GlobalVariable *vc::getConstStringGVFromOperandOptional(const Value &Op) {
  IGC_ASSERT_MESSAGE(Op.getType()->isPointerTy(),
                     "wrong argument: pointer was expected");
  IGC_ASSERT_MESSAGE(IGCLLVM::getNonOpaquePtrEltTy(Op.getType())->isIntegerTy(8),
                     "wrong argument: i8* value was expected");
  auto &MaybeGEP = ignoreCastToGenericAS(Op);
  if (!isa<GEPOperator>(MaybeGEP))
    return nullptr;
  auto &GEPPtrOp = *cast<GEPOperator>(MaybeGEP).getPointerOperand();
  // FIXME: Check that indices are {0, 0}.
  auto &MaybeGV = ignoreCastToGenericAS(GEPPtrOp);
  if (!isConstantString(MaybeGV))
    return nullptr;
  return &cast<GlobalVariable>(MaybeGV);
}

GlobalVariable *vc::getConstStringGVFromOperandOptional(Value &Op) {
  return const_cast<GlobalVariable *>(
      getConstStringGVFromOperandOptional(static_cast<const Value &>(Op)));
}

const GlobalVariable &vc::getConstStringGVFromOperand(const Value &Op) {
  auto *GV = getConstStringGVFromOperandOptional(Op);
  IGC_ASSERT_MESSAGE(GV,
                     "couldn't reach constexpr string through pointer operand");
  return *GV;
}

GlobalVariable &vc::getConstStringGVFromOperand(Value &Op) {
  return const_cast<GlobalVariable &>(
      getConstStringGVFromOperand(static_cast<const Value &>(Op)));
}

Optional<StringRef> vc::getConstStringFromOperandOptional(const Value &Op) {
  auto *GV = getConstStringGVFromOperandOptional(Op);
  if (!GV)
    return {};
  return extractCStr(*GV->getInitializer());
}

StringRef vc::getConstStringFromOperand(const Value &Op) {
  auto FmtStr = getConstStringFromOperandOptional(Op);
  IGC_ASSERT_MESSAGE(FmtStr.hasValue(),
                     "couldn't reach constexpr string through pointer operand");
  return FmtStr.getValue();
}

// Given that \p ArgDesc describes integer conversion with signedness equal to
// \p IsSigned, defines which particular integer type is provided.
static PrintfArgInfo parseIntLengthModifier(StringRef ArgDesc, bool IsSigned) {
  std::string Suffix{1u, ArgDesc.back()};
  if (ArgDesc.endswith("hh" + Suffix))
    return {PrintfArgInfo::Char, IsSigned};
  if (ArgDesc.endswith("h" + Suffix))
    return {PrintfArgInfo::Short, IsSigned};
  if (ArgDesc.endswith("ll" + Suffix))
    // TOTHINK: maybe we need a separate type ID for long long.
    return {PrintfArgInfo::Long, IsSigned};
  if (ArgDesc.endswith("l" + Suffix))
    return {PrintfArgInfo::Long, IsSigned};
  return {PrintfArgInfo::Int, IsSigned};
}

// \p ArgDesc is a format string conversion specifier matched by a regex
// (some string that starts with % and ends with d,i,f,...).
static PrintfArgInfo parseArgDesc(StringRef ArgDesc) {
  if (ArgDesc.endswith("c"))
    // FIXME: support %lc
    return {PrintfArgInfo::Int, /* IsSigned */ true};
  if (ArgDesc.endswith("s"))
    // FIXME: support %ls
    return {PrintfArgInfo::String, /* IsSigned */ false};
  if (ArgDesc.endswith("d") || ArgDesc.endswith("i"))
    return parseIntLengthModifier(ArgDesc, /* IsSigned */ true);
  if (ArgDesc.endswith("o") || ArgDesc.endswith("u") ||
      IGCLLVM::endswith_insensitive(ArgDesc, "x"))
    return parseIntLengthModifier(ArgDesc, /* IsSigned */ false);
  if (IGCLLVM::endswith_insensitive(ArgDesc, "f") ||
      IGCLLVM::endswith_insensitive(ArgDesc, "e") ||
      IGCLLVM::endswith_insensitive(ArgDesc, "a") ||
      IGCLLVM::endswith_insensitive(ArgDesc, "g"))
    return {PrintfArgInfo::Double, /* IsSigned */ true};
  IGC_ASSERT_MESSAGE(ArgDesc.endswith("p"), "unexpected conversion specifier");
  return {PrintfArgInfo::Pointer, /* IsSigned */ false};
}

PrintfArgInfoSeq vc::parseFormatString(StringRef FmtStr) {
  PrintfArgInfoSeq Args;
  Regex ArgDescRegEx{"%(%|[^%csdioxXufFeEaAgGp]*[csdioxXufFeEaAgGp])"};
  IGC_ASSERT_MESSAGE(IGCLLVM::isValid(ArgDescRegEx),
                     "an error during regex parsing");
  using ArgDescRegExIter = RegexIterator<2>;
  auto &&ArgDescs = make_filter_range(
      make_range(ArgDescRegExIter{FmtStr, ArgDescRegEx}, ArgDescRegExIter{}),
      [](ArgDescRegExIter::reference Match) { return Match[0] != "%%"; });
  transform(
      ArgDescs, std::back_inserter(Args),
      [](ArgDescRegExIter::reference Match) { return parseArgDesc(Match[0]); });
  return Args;
}

bool vc::isPrintFormatIndex(const User &Usr) {
  return vc::InternalIntrinsic::getInternalIntrinsicID(&Usr) ==
         vc::InternalIntrinsic::print_format_index;
}

CallInst &vc::createPrintFormatIndex(Value &Pointer, Instruction &InsertionPt) {
  IGC_ASSERT_MESSAGE(Pointer.getType()->isPointerTy(),
                     "wrong argument: @llvm.vc.internal.print.format.index "
                     "operand must be a pointer");
  IRBuilder<> IRB{&InsertionPt};
  auto *Decl = vc::InternalIntrinsic::getInternalDeclaration(
      IRB.GetInsertBlock()->getModule(),
      vc::InternalIntrinsic::print_format_index, Pointer.getType());
  return *IRB.CreateCall(Decl, &Pointer);
}

bool vc::isLegalPrintFormatIndexGEP(const GEPOperator &GEP) {
  if (!isConstantStringFirstElementGEP(GEP))
    return false;
  if (GEP.user_empty())
    return false;
  return std::all_of(GEP.user_begin(), GEP.user_end(),
                     [](const User *Usr) { return isPrintFormatIndex(*Usr); });
}

bool vc::isLegalPrintFormatIndexGEP(const Value &V) {
  if (!isa<GEPOperator>(V))
    return false;
  return isLegalPrintFormatIndexGEP(cast<GEPOperator>(V));
}

bool vc::isPrintFormatIndexGEP(const GEPOperator &GEP) {
  if (!isConstantStringFirstElementGEP(GEP))
    return false;
  return std::any_of(GEP.user_begin(), GEP.user_end(),
                     [](const User *Usr) { return isPrintFormatIndex(*Usr); });
}

bool vc::isPrintFormatIndexGEP(const Value &V) {
  if (!isa<GEPOperator>(V))
    return false;
  return isPrintFormatIndexGEP(cast<GEPOperator>(V));
}
