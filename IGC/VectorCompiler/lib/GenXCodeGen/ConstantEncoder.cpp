/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ConstantEncoder.h"

#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/TypeSize.h"

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
encodeConstExprImpl(const PtrToIntOperator &P2I, const DataLayout &DL) {
  IGC_ASSERT_MESSAGE(!isa<IGCLLVM::FixedVectorType>(P2I.getType()),
                     "vector ptrtoint is not yet supported");
  auto [Data, Relocs] = vc::encodeGlobalValueOrConstantExpression(
      *cast<Constant>(P2I.getPointerOperand()), DL);
  auto DstSize = vc::getTypeSize(P2I.getType(), &DL);
  vc::TypeSizeWrapper SrcSize{Data.getBitWidth()};
  if (DstSize == SrcSize)
    // Nop ptrtoint just return the pointer operand encoding.
    return {Data, Relocs};

  if (DstSize > SrcSize)
    // zext ptrtoint case, just append zeros to data.
    return {Data.zext(DstSize.inBits()), Relocs};

  // The only supported case of truncation.
  if (DstSize.inBits() == 32 && SrcSize.inBits() == 64) {
    IGC_ASSERT_MESSAGE(
        Relocs.size() == 1 &&
            Relocs.front().r_type == vISA::GenRelocType::R_SYM_ADDR,
        "only 64-bit relocation is supported for a truncating ptrtoint");
    Relocs.front().r_type = vISA::GenRelocType::R_SYM_ADDR_32;
    return {Data.trunc(32), Relocs};
  }

  vc::diagnose(P2I.getContext(), "ConstantEncoder", &P2I,
               "such ptrtoint constant expression is not supported");
  return {Data, Relocs};
}

static std::pair<APInt, std::vector<vISA::ZERelocEntry>>
encodeGlobalValue(const GlobalValue &GV, const DataLayout &DL) {
  auto RelType = vISA::GenRelocType::R_NONE;
  auto Size = vc::getTypeSize(GV.getType(), &DL);
  switch (Size.inBits()) {
  case 32:
    RelType = vISA::GenRelocType::R_SYM_ADDR_32;
    break;
  case 64:
    RelType = vISA::GenRelocType::R_SYM_ADDR;
    break;
  default:
    report_fatal_error("Relocation of the provided pointer is not supported");
  }
  return {APInt::getNullValue(Size.inBits()),
          {vISA::ZERelocEntry{RelType, 0, GV.getName().str()}}};
}

static std::pair<APInt, std::vector<vISA::ZERelocEntry>>
encodeConstExpr(const ConstantExpr &CExpr, const DataLayout &DL) {
  switch (CExpr.getOpcode()) {
  case Instruction::GetElementPtr:
    return encodeConstExprImpl(cast<GEPOperator>(CExpr), DL);
  case Instruction::PtrToInt:
    return encodeConstExprImpl(cast<PtrToIntOperator>(CExpr), DL);
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
