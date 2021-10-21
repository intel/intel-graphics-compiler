/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ConstantEncoder.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/DerivedTypes.h"

using namespace llvm;

static std::pair<APInt, std::vector<vISA::ZERelocEntry>>
encodeConstExprImpl(const llvm::GEPOperator &GEP, const DataLayout &DL) {
  IGC_ASSERT_MESSAGE(!isa<IGCLLVM::FixedVectorType>(GEP.getType()),
                     "vector gep is not yet supported");
  auto [Data, Relocs] = vc::encodeGlobalValueOrConstantExpression(
      *cast<Constant>(GEP.getPointerOperand()), DL);
  APInt Offset{Data.getBitWidth(), 0};
  bool Success = GEP.accumulateConstantOffset(DL, Offset);
  IGC_ASSERT_MESSAGE(Success,
                     "Offset must be constant for GEP constant expression");
  return {Data + Offset, Relocs};
}

static std::pair<APInt, std::vector<vISA::ZERelocEntry>>
encodeGlobalValue(const GlobalValue &GV, const DataLayout &DL) {
  auto RelType = vISA::GenRelocType::R_NONE;
  unsigned Size = DL.getTypeSizeInBits(GV.getType());
  switch (Size) {
  case 32:
    RelType = vISA::GenRelocType::R_SYM_ADDR_32;
    break;
  case 64:
    RelType = vISA::GenRelocType::R_SYM_ADDR;
    break;
  default:
    report_fatal_error("Relocation of the provided pointer is not supported");
  }
  return {APInt::getNullValue(Size),
          {vISA::ZERelocEntry{RelType, 0, GV.getName().str()}}};
}

static std::pair<APInt, std::vector<vISA::ZERelocEntry>>
encodeConstExpr(const ConstantExpr &CExpr, const DataLayout &DL) {
  switch (CExpr.getOpcode()) {
  case llvm::Instruction::GetElementPtr:
    return encodeConstExprImpl(llvm::cast<llvm::GEPOperator>(CExpr), DL);
  default:
    IGC_ASSERT_MESSAGE(0, "Unsupported constant expression");
    return {APInt{}, {}};
  }
}

std::pair<APInt, std::vector<vISA::ZERelocEntry>>
vc::encodeGlobalValueOrConstantExpression(const Constant &C,
                                          const DataLayout &DL) {
  if (isa<GlobalValue>(C))
    return encodeGlobalValue(cast<GlobalValue>(C), DL);
  return encodeConstExpr(cast<ConstantExpr>(C), DL);
}
