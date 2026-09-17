/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SpvSubgroupMMAResolution.hpp"

#include <algorithm>
#include <cmath> // for ceil

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallVector.h>
#include "llvm/ADT/StringExtras.h"
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/raw_ostream.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Instructions.h"

#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;
using namespace IGC;

static constexpr StringRef DpasOpName = "__spirv_SubgroupMatrixMultiplyAccumulateINTEL";
static constexpr StringRef BdpasOpName = "__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL";

char SpvSubgroupMMAResolutionLPM::ID = 0;

#define PASS_FLAG "igc-spv-subgroup-mma-resolution"
#define PASS_DESC "Lowering of SPIR-V INTEL subgroup_matrix_multiply_accumulate instructions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
#define DEBUG_TYPE "spv-subgroup-mma-resolution"

IGC_INITIALIZE_PASS_BEGIN(SpvSubgroupMMAResolutionLPM, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(SpvSubgroupMMAResolutionLPM, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

SpvSubgroupMMAResolutionLPM::SpvSubgroupMMAResolutionLPM() : ModulePass(ID) {
  initializeSpvSubgroupMMAResolutionLPMPass(*PassRegistry::getPassRegistry());
}

bool SpvSubgroupMMAResolution::run(Module &M, CodeGenContext *pCtx, IGCMD::MetaDataUtils *pMdUtils) {
  m_BuiltinsToRemove.clear();
  m_Module = &M;
  m_Changed = false;
  m_Ctx = pCtx;
  m_pMdUtils = pMdUtils;
  // The SIMD size policy below assumes hasExecSize16DPAS() - see isDoubleSubgroup().
  m_simdResolver = std::make_unique<KernelSIMDSizeResolver>(
      m_Ctx, m_pMdUtils, [](int32_t SIMDSize) { return SIMDSize == 16 || SIMDSize == 32; },
      [this]() {
        IGC_ASSERT(m_Ctx->platform.hasExecSize16DPAS());
        return 16;
      },
      "subgroup MMA");

  visit(M);

  for (auto &F : m_BuiltinsToRemove)
    F->eraseFromParent();

  return m_Changed;
}

enum {
  None = 0,
  MatrixASignedComponentsINTEL = 0x1,
  MatrixBSignedComponentsINTEL = 0x2,
  MatrixCBFloat16INTEL = 0x4,
  MatrixResultBFloat16INTEL = 0x8,
  MatrixAPackedInt8INTEL = 0x10,
  MatrixBPackedInt8INTEL = 0x20,
  MatrixAPackedInt4INTEL = 0x40,
  MatrixBPackedInt4INTEL = 0x80,
  MatrixATF32INTEL = 0x100,
  MatrixBTF32INTEL = 0x200,
  MatrixAPackedFloat16INTEL = 0x400,
  MatrixBPackedFloat16INTEL = 0x800,
  MatrixAPackedBFloat16INTEL = 0x1000,
  MatrixBPackedBFloat16INTEL = 0x2000,
  MatrixAPackedFloat8E4M3INTEL = 0x4000,
  MatrixBPackedFloat8E4M3INTEL = 0x8000,
  MatrixAPackedFloat8E5M2INTEL = 0x10000,
  MatrixBPackedFloat8E5M2INTEL = 0x20000,
  MatrixAPackedFloat4E2M1INTEL = 0x40000,
  MatrixBPackedFloat4E2M1INTEL = 0x80000,
  ScaleAFloat8E8M0INTEL = 0x100000,
  ScaleBFloat8E8M0INTEL = 0x200000,
};

static std::string GetHumanReadableOperand(uint32_t operand) {
  SmallVector<std::string, 8> operands;

  if (operand & MatrixASignedComponentsINTEL)
    operands.push_back("MatrixASignedComponentsINTEL");
  if (operand & MatrixBSignedComponentsINTEL)
    operands.push_back("MatrixBSignedComponentsINTEL");
  if (operand & MatrixCBFloat16INTEL)
    operands.push_back("MatrixCBFloat16INTEL");
  if (operand & MatrixResultBFloat16INTEL)
    operands.push_back("MatrixResultBFloat16INTEL");
  if (operand & MatrixAPackedInt8INTEL)
    operands.push_back("MatrixAPackedInt8INTEL");
  if (operand & MatrixBPackedInt8INTEL)
    operands.push_back("MatrixBPackedInt8INTEL");
  if (operand & MatrixAPackedInt4INTEL)
    operands.push_back("MatrixAPackedInt4INTEL");
  if (operand & MatrixBPackedInt4INTEL)
    operands.push_back("MatrixBPackedInt4INTEL");
  if (operand & MatrixATF32INTEL)
    operands.push_back("MatrixATF32INTEL");
  if (operand & MatrixBTF32INTEL)
    operands.push_back("MatrixBTF32INTEL");
  if (operand & MatrixAPackedFloat16INTEL)
    operands.push_back("MatrixAPackedFloat16INTEL");
  if (operand & MatrixBPackedFloat16INTEL)
    operands.push_back("MatrixBPackedFloat16INTEL");
  if (operand & MatrixAPackedBFloat16INTEL)
    operands.push_back("MatrixAPackedBFloat16INTEL");
  if (operand & MatrixBPackedBFloat16INTEL)
    operands.push_back("MatrixBPackedBFloat16INTEL");
  if (operand & MatrixAPackedFloat8E4M3INTEL)
    operands.push_back("MatrixAPackedFloat8E4M3INTEL");
  if (operand & MatrixBPackedFloat8E4M3INTEL)
    operands.push_back("MatrixBPackedFloat8E4M3INTEL");
  if (operand & MatrixAPackedFloat8E5M2INTEL)
    operands.push_back("MatrixAPackedFloat8E5M2INTEL");
  if (operand & MatrixBPackedFloat8E5M2INTEL)
    operands.push_back("MatrixBPackedFloat8E5M2INTEL");
  if (operand & MatrixAPackedFloat4E2M1INTEL)
    operands.push_back("MatrixAPackedFloat4E2M1INTEL");
  if (operand & MatrixBPackedFloat4E2M1INTEL)
    operands.push_back("MatrixBPackedFloat4E2M1INTEL");
  if (operand & ScaleAFloat8E8M0INTEL)
    operands.push_back("ScaleAFloat8E8M0INTEL");
  if (operand & ScaleBFloat8E8M0INTEL)
    operands.push_back("ScaleBFloat8E8M0INTEL");

  if (operands.empty())
    return "None";

  return llvm::join(operands, " | ");
}

const SpvSubgroupMMAResolution::SupportedTable &SpvSubgroupMMAResolution::getSimd8Table() {
  static const SupportedTable Table = {
      // 8-bit integer matrix sources (signed and unsigned), 32-bit integer accumulator:
      {{32, ElType::I32, ElType::I32, ElType::I32, MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL}, "u8_u8_"},
      {{32, ElType::I32, ElType::I32, ElType::I32,
        MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL | MatrixASignedComponentsINTEL},
       "s8_u8_"},
      {{32, ElType::I32, ElType::I32, ElType::I32,
        MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL | MatrixBSignedComponentsINTEL},
       "u8_s8_"},
      {{32, ElType::I32, ElType::I32, ElType::I32,
        MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL | MatrixASignedComponentsINTEL | MatrixBSignedComponentsINTEL},
       "s8_s8_"},

      // 4-bit integer matrix sources (signed and unsigned), 32-bit integer accumulator:
      {{64, ElType::I32, ElType::I32, ElType::I32, MatrixAPackedInt4INTEL | MatrixBPackedInt4INTEL}, "u4_u4_"},
      {{64, ElType::I32, ElType::I32, ElType::I32,
        MatrixAPackedInt4INTEL | MatrixBPackedInt4INTEL | MatrixASignedComponentsINTEL},
       "s4_u4_"},
      {{64, ElType::I32, ElType::I32, ElType::I32,
        MatrixAPackedInt4INTEL | MatrixBPackedInt4INTEL | MatrixBSignedComponentsINTEL},
       "u4_s4_"},
      {{64, ElType::I32, ElType::I32, ElType::I32,
        MatrixAPackedInt4INTEL | MatrixBPackedInt4INTEL | MatrixASignedComponentsINTEL | MatrixBSignedComponentsINTEL},
       "s4_s4_"},

      // fp16 matrix sources, fp32 accumulator:
      {{16, ElType::F32, ElType::I32, ElType::I32, MatrixAPackedFloat16INTEL | MatrixBPackedFloat16INTEL}, "hf_hf_"},
      // bf16 matrix sources, fp32 accumulator:
      {{16, ElType::F32, ElType::I32, ElType::I32, MatrixAPackedBFloat16INTEL | MatrixBPackedBFloat16INTEL}, "bf_bf_"},
  };
  return Table;
}

const SpvSubgroupMMAResolution::SupportedTable &SpvSubgroupMMAResolution::getSimd16Table() {
  static const SupportedTable Table = {
      // 8-bit integer matrix sources (signed and unsigned), 32-bit integer accumulator:
      {{32, ElType::I32, ElType::I16, ElType::I32, MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL}, "u8_u8_"},
      {{32, ElType::I32, ElType::I16, ElType::I32,
        MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL | MatrixASignedComponentsINTEL},
       "s8_u8_"},
      {{32, ElType::I32, ElType::I16, ElType::I32,
        MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL | MatrixBSignedComponentsINTEL},
       "u8_s8_"},
      {{32, ElType::I32, ElType::I16, ElType::I32,
        MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL | MatrixASignedComponentsINTEL | MatrixBSignedComponentsINTEL},
       "s8_s8_"},

      // 4-bit integer matrix sources (signed and unsigned), 32-bit integer accumulator:
      {{64, ElType::I32, ElType::I16, ElType::I32, MatrixAPackedInt4INTEL | MatrixBPackedInt4INTEL}, "u4_u4_"},
      {{64, ElType::I32, ElType::I16, ElType::I32,
        MatrixAPackedInt4INTEL | MatrixBPackedInt4INTEL | MatrixASignedComponentsINTEL},
       "s4_u4_"},
      {{64, ElType::I32, ElType::I16, ElType::I32,
        MatrixAPackedInt4INTEL | MatrixBPackedInt4INTEL | MatrixBSignedComponentsINTEL},
       "u4_s4_"},
      {{64, ElType::I32, ElType::I16, ElType::I32,
        MatrixAPackedInt4INTEL | MatrixBPackedInt4INTEL | MatrixASignedComponentsINTEL | MatrixBSignedComponentsINTEL},
       "s4_s4_"},

      // fp16 matrix sources, fp32 accumulator:
      {{16, ElType::F32, ElType::I16, ElType::I32, MatrixAPackedFloat16INTEL | MatrixBPackedFloat16INTEL},
       "f_f_hf_hf_"},
      // bf16 matrix sources, fp32 accumulator:
      {{16, ElType::F32, ElType::I16, ElType::I32, MatrixAPackedBFloat16INTEL | MatrixBPackedBFloat16INTEL},
       "f_f_bf_bf_"},
      // fp16 matrix sources, fp16 accumulator:
      {{16, ElType::F16, ElType::I16, ElType::I32, MatrixAPackedFloat16INTEL | MatrixBPackedFloat16INTEL},
       "hf_hf_hf_hf_"},
      // bf16 matrix sources, bf16 accumulator:
      {{16, ElType::I16, ElType::I16, ElType::I32,
        MatrixResultBFloat16INTEL | MatrixAPackedBFloat16INTEL | MatrixBPackedBFloat16INTEL | MatrixCBFloat16INTEL},
       "bf_bf_bf_bf_"},

      // tf32 matrix sources, fp32 accumulator:
      {{8, ElType::F32, ElType::F32, ElType::F32, MatrixATF32INTEL | MatrixBTF32INTEL}, "f_f_tf32_tf32_"},

      // fp8 matrix sources (hf8 and bf8), fp32 accumulator:
      {{32, ElType::F32, ElType::I16, ElType::I32, MatrixAPackedFloat8E5M2INTEL | MatrixBPackedFloat8E5M2INTEL},
       "f_f_bf8_bf8_"},
      {{32, ElType::F32, ElType::I16, ElType::I32, MatrixAPackedFloat8E4M3INTEL | MatrixBPackedFloat8E5M2INTEL},
       "f_f_hf8_bf8_"},
      {{32, ElType::F32, ElType::I16, ElType::I32, MatrixAPackedFloat8E5M2INTEL | MatrixBPackedFloat8E4M3INTEL},
       "f_f_bf8_hf8_"},
      {{32, ElType::F32, ElType::I16, ElType::I32, MatrixAPackedFloat8E4M3INTEL | MatrixBPackedFloat8E4M3INTEL},
       "f_f_hf8_hf8_"},

      // fp8 matrix sources (hf8 and bf8), bf16 accumulator:
      {{32, ElType::I16, ElType::I16, ElType::I32,
        MatrixResultBFloat16INTEL | MatrixAPackedFloat8E5M2INTEL | MatrixBPackedFloat8E5M2INTEL | MatrixCBFloat16INTEL},
       "bf_bf_bf8_bf8_"},
      {{32, ElType::I16, ElType::I16, ElType::I32,
        MatrixResultBFloat16INTEL | MatrixAPackedFloat8E4M3INTEL | MatrixBPackedFloat8E5M2INTEL | MatrixCBFloat16INTEL},
       "bf_bf_hf8_bf8_"},
      {{32, ElType::I16, ElType::I16, ElType::I32,
        MatrixResultBFloat16INTEL | MatrixAPackedFloat8E5M2INTEL | MatrixBPackedFloat8E4M3INTEL | MatrixCBFloat16INTEL},
       "bf_bf_bf8_hf8_"},
      {{32, ElType::I16, ElType::I16, ElType::I32,
        MatrixResultBFloat16INTEL | MatrixAPackedFloat8E4M3INTEL | MatrixBPackedFloat8E4M3INTEL | MatrixCBFloat16INTEL},
       "bf_bf_hf8_hf8_"},

      // fp4 matrix sources, fp32 accumulator:
      {{64, ElType::F32, ElType::I16, ElType::I32, MatrixAPackedFloat4E2M1INTEL | MatrixBPackedFloat4E2M1INTEL},
       "f_f_e2m1_e2m1_"},

      // fp4 matrix sources, bf16 accumulator:
      {{64, ElType::I16, ElType::I16, ElType::I32,
        MatrixResultBFloat16INTEL | MatrixAPackedFloat4E2M1INTEL | MatrixBPackedFloat4E2M1INTEL | MatrixCBFloat16INTEL},
       "bf_bf_e2m1_e2m1_"},
  };
  return Table;
}

const SpvSubgroupMMAResolution::SupportedTable &SpvSubgroupMMAResolution::getSimd16ScaledTable() {
  static constexpr uint32_t ScaleFlags = ScaleAFloat8E8M0INTEL | ScaleBFloat8E8M0INTEL;
  static const SupportedTable Table = {
      // fp16 matrix sources, fp32 accumulator:
      {{16, ElType::F32, ElType::I16, ElType::I32, MatrixAPackedFloat16INTEL | MatrixBPackedFloat16INTEL | ScaleFlags},
       "f_f_hf_hf_"},
      // bf16 matrix sources, fp32 accumulator:
      {{16, ElType::F32, ElType::I16, ElType::I32,
        MatrixAPackedBFloat16INTEL | MatrixBPackedBFloat16INTEL | ScaleFlags},
       "f_f_bf_bf_"},
      // fp16 matrix sources, fp16 accumulator:
      {{16, ElType::F16, ElType::I16, ElType::I32, MatrixAPackedFloat16INTEL | MatrixBPackedFloat16INTEL | ScaleFlags},
       "hf_hf_hf_hf_"},
      // bf16 matrix sources, bf16 accumulator:
      {{16, ElType::I16, ElType::I16, ElType::I32,
        MatrixResultBFloat16INTEL | MatrixAPackedBFloat16INTEL | MatrixBPackedBFloat16INTEL | MatrixCBFloat16INTEL |
            ScaleFlags},
       "bf_bf_bf_bf_"},

      // fp8 matrix sources (e4m3 and e5m2), fp32 accumulator:
      {{32, ElType::F32, ElType::I16, ElType::I32,
        MatrixAPackedFloat8E5M2INTEL | MatrixBPackedFloat8E5M2INTEL | ScaleFlags},
       "f_f_bf8_bf8_"},
      {{32, ElType::F32, ElType::I16, ElType::I32,
        MatrixAPackedFloat8E4M3INTEL | MatrixBPackedFloat8E5M2INTEL | ScaleFlags},
       "f_f_hf8_bf8_"},
      {{32, ElType::F32, ElType::I16, ElType::I32,
        MatrixAPackedFloat8E5M2INTEL | MatrixBPackedFloat8E4M3INTEL | ScaleFlags},
       "f_f_bf8_hf8_"},
      {{32, ElType::F32, ElType::I16, ElType::I32,
        MatrixAPackedFloat8E4M3INTEL | MatrixBPackedFloat8E4M3INTEL | ScaleFlags},
       "f_f_hf8_hf8_"},

      // fp8 matrix sources (e4m3 and e5m2), bf16 accumulator:
      {{32, ElType::I16, ElType::I16, ElType::I32,
        MatrixResultBFloat16INTEL | MatrixAPackedFloat8E5M2INTEL | MatrixBPackedFloat8E5M2INTEL | MatrixCBFloat16INTEL |
            ScaleFlags},
       "bf_bf_bf8_bf8_"},
      {{32, ElType::I16, ElType::I16, ElType::I32,
        MatrixResultBFloat16INTEL | MatrixAPackedFloat8E4M3INTEL | MatrixBPackedFloat8E5M2INTEL | MatrixCBFloat16INTEL |
            ScaleFlags},
       "bf_bf_hf8_bf8_"},
      {{32, ElType::I16, ElType::I16, ElType::I32,
        MatrixResultBFloat16INTEL | MatrixAPackedFloat8E5M2INTEL | MatrixBPackedFloat8E4M3INTEL | MatrixCBFloat16INTEL |
            ScaleFlags},
       "bf_bf_bf8_hf8_"},
      {{32, ElType::I16, ElType::I16, ElType::I32,
        MatrixResultBFloat16INTEL | MatrixAPackedFloat8E4M3INTEL | MatrixBPackedFloat8E4M3INTEL | MatrixCBFloat16INTEL |
            ScaleFlags},
       "bf_bf_hf8_hf8_"},

      // fp4 matrix sources, fp32 accumulator:
      {{64, ElType::F32, ElType::I16, ElType::I32,
        MatrixAPackedFloat4E2M1INTEL | MatrixBPackedFloat4E2M1INTEL | ScaleFlags},
       "f_f_e2m1_e2m1_"},

      // fp4 matrix sources, bf16 accumulator:
      {{64, ElType::I16, ElType::I16, ElType::I32,
        MatrixResultBFloat16INTEL | MatrixAPackedFloat4E2M1INTEL | MatrixBPackedFloat4E2M1INTEL | MatrixCBFloat16INTEL |
            ScaleFlags},
       "bf_bf_e2m1_e2m1_"},
  };
  return Table;
}

void SpvSubgroupMMAResolution::emitError(const Twine &message, const CallInst &CI) {
  m_Ctx->EmitError(message.str().c_str(), &CI);
}

SpvSubgroupMMAResolution::ElType SpvSubgroupMMAResolution::getElType(const Type *Ty) const {
  if (Ty->isIntegerTy(32))
    return I32;
  if (Ty->isIntegerTy(16))
    return I16;
  if (Ty->isFloatTy())
    return F32;
  if (Ty->isHalfTy())
    return F16;
  return Unknown;
}

StringRef SpvSubgroupMMAResolution::getElTypeStr(const SpvSubgroupMMAResolution::ElType Ty) const {
  switch (Ty) {
  case I32:
    return "int32_t";
  case I16:
    return "int16_t";
  case F32:
    return "float32_t";
  case F16:
    return "float16_t";
  default:
    IGC_ASSERT_MESSAGE(0, "unexpected element type");
    return "Unknown";
  }
}

SpvSubgroupMMAResolution::ElType SpvSubgroupMMAResolution::getValidMatrixType(const Type *Ty) const {
  if (Ty->isFloatingPointTy() || Ty->isIntegerTy())
    return getElType(Ty);

  if (auto *VTy = dyn_cast<FixedVectorType>(Ty))
    return getValidMatrixType(VTy->getElementType());

  return Unknown;
}

bool SpvSubgroupMMAResolution::validateI32Constant(const Value *V, const Twine &ParamName, StringRef BuiltinName,
                                                   const CallInst &CI) {
  if (!isa<ConstantInt>(V) || !V->getType()->isIntegerTy(32)) {
    emitError(Twine(BuiltinName) + ": " + ParamName + " argument must be a constant scalar 32-bit integer", CI);
    return false;
  }
  return true;
}

bool SpvSubgroupMMAResolution::validateCType(const Type *ResultTy, const Type *CType, StringRef BuiltinName,
                                             const CallInst &CI) {
  if (ResultTy == CType)
    return true;

  std::string msg;
  raw_string_ostream rso(msg);
  rso << BuiltinName << ": expected Result type to match type of Matrix C for targeted HW. Result type: ";

  ResultTy->print(rso);
  rso << ", Matrix C type: ";
  CType->print(rso);
  emitError(msg, CI);

  return false;
}

bool SpvSubgroupMMAResolution::validateElementType(const ElType ElemTy, StringRef ParamName, StringRef BuiltinName,
                                                   const CallInst &CI) {
  if (ElemTy != Unknown)
    return true;

  emitError(Twine(BuiltinName) + ": expected " + ParamName +
                " to be a scalar or vector of int32_t, int16_t, float32_t, or float16_t for targeted HW",
            CI);
  return false;
}

int SpvSubgroupMMAResolution::getElemCount(const Type *Ty) const {
  if (auto *VTy = dyn_cast<FixedVectorType>(Ty))
    return VTy->getNumElements();
  return 1;
}

bool SpvSubgroupMMAResolution::validateDpasElemCounts(int M, int AElemCount, int BElemCount, uint32_t Operands,
                                                      CallInst &CI) {
  if (M != 1 && M != 2 && M != 4 && M != 8) {
    emitError(Twine(DpasOpName) + ": M dimension must be 1, 2, 4 or 8 for targeted HW. Actual: " + std::to_string(M),
              CI);
    return false;
  }
  if (Operands & MatrixATF32INTEL) {
    int expected = std::ceil(M / 2.0);
    if (AElemCount != expected) {
      emitError(Twine(DpasOpName) +
                    ": Matrix A argument must have ceil(M/2) components when MatrixATF32INTEL operand is set for "
                    "targeted HW. Expected " +
                    std::to_string(expected) + ". Actual " + std::to_string(AElemCount),
                CI);
      return false;
    }
  } else if (AElemCount != M) {
    emitError(Twine(DpasOpName) + ": Matrix A argument must have size " + std::to_string(M) +
                  " to match M defined by Result type for targeted HW. Actual: " + std::to_string(AElemCount),
              CI);
    return false;
  }
  const int expectedBCount = isDoubleSubgroup(CI) ? 4 : 8;
  if (BElemCount != expectedBCount) {
    emitError(Twine(DpasOpName) + ": Matrix B argument must have " + std::to_string(expectedBCount) +
                  " components for targeted HW. Actual: " + std::to_string(BElemCount),
              CI);
    return false;
  }
  return true;
}

bool SpvSubgroupMMAResolution::validateBdpasElemCounts(int M, int AElemCount, int BElemCount, const CallInst &CI) {
  // bdpas spec fixes M = 8
  if (M != 8) {
    emitError(Twine(BdpasOpName) + ": M dimension must be 8 for targeted HW. Actual: " + std::to_string(M), CI);
    return false;
  }

  if (AElemCount != 8) {
    emitError(Twine(BdpasOpName) +
                  ": Matrix A argument must have 8 components for targeted HW. Actual: " + std::to_string(AElemCount),
              CI);
    return false;
  }

  if (BElemCount != 8) {
    emitError(Twine(BdpasOpName) +
                  ": Matrix B argument must have 8 components for targeted HW. Actual: " + std::to_string(BElemCount),
              CI);
    return false;
  }

  return true;
}

bool SpvSubgroupMMAResolution::validateScaleType(const Value *Scale, StringRef ParamName, int K, const CallInst &CI) {
  const Type *T = Scale->getType();
  const Type *ElemTy = T->getScalarType();
  int NElts = T->isVectorTy() ? (int)cast<FixedVectorType>(T)->getNumElements() : 1;
  int Expected = (K == 64) ? 2 : 1;

  // Scales are uint8_t for K in {16, 32} and 2 x uint8_t for K == 64
  if (!ElemTy->isIntegerTy(8) || NElts != Expected) {
    std::string ExpectedStr = Expected == 1 ? "uint8_t" : "2 x uint8_t";
    emitError(Twine(BdpasOpName) + ": " + ParamName + " must be " + ExpectedStr + " for K Dim = " + std::to_string(K),
              CI);
    return false;
  }
  return true;
}

// Dimension N is platform specific and is directly correlated to minimum subgroup-size for
// given platform. If DPAS with the same M, N, K dimensions is executed within a subgroup
// twice the size of minimum subgroup-size, each work item must contain half of the data
// compared to the minimum subgroup-size.
bool SpvSubgroupMMAResolution::isDoubleSubgroup(CallInst &CI) {
  if (!m_Ctx->platform.hasExecSize16DPAS())
    return false;
  return m_simdResolver->resolve(CI.getFunction()) == 32;
}

const SpvSubgroupMMAResolution::SupportedTable &SpvSubgroupMMAResolution::getSupportedTable() const {
  return m_Ctx->platform.hasExecSize16DPAS() ? getSimd16Table() : getSimd8Table();
}

void SpvSubgroupMMAResolution::diagnoseUnsupportedCombination(const SupportedTableKey &Key, const SupportedTable &Table,
                                                              StringRef BuiltinName, const CallInst &CI) {
  auto sortAndDeduplicate = [](auto &Values) {
    std::ranges::sort(Values);
    Values.erase(std::ranges::unique(Values).begin(), Values.end());
  };

  // Validate K dimension.
  SmallVector<std::string, 8> ValidKDims;
  for (const auto &[Candidate, _] : Table)
    ValidKDims.push_back(std::to_string(Candidate.K));
  sortAndDeduplicate(ValidKDims);

  if (std::ranges::find(ValidKDims, std::to_string(Key.K)) == ValidKDims.end()) {
    emitError(Twine(BuiltinName) + ": expected K Dim = " + llvm::join(ValidKDims, " or ") +
                  " for targeted HW. Actual: " + Twine(Key.K),
              CI);
    return;
  }

  auto getValidTypes = [&](auto MatchesPrefix, auto GetType) {
    SmallVector<std::string, 8> ValidTypes;
    for (const auto &[Candidate, _] : Table) {
      if (MatchesPrefix(Candidate))
        ValidTypes.push_back(getElTypeStr(GetType(Candidate)).str());
    }
    sortAndDeduplicate(ValidTypes);
    return llvm::join(ValidTypes, " or ");
  };

  auto hasMatchingType = [&](auto MatchesPrefix, auto GetType, ElType Type) {
    return std::ranges::any_of(
        Table, [&](const auto &Entry) { return MatchesPrefix(Entry.first) && GetType(Entry.first) == Type; });
  };

  // Validate Result element type.
  auto MatchesK = [&](const SupportedTableKey &Candidate) { return Candidate.K == Key.K; };
  auto GetResultType = [](const SupportedTableKey &Candidate) { return Candidate.ResultElemTy; };
  if (!hasMatchingType(MatchesK, GetResultType, Key.ResultElemTy)) {
    std::string ValidResultTypes = getValidTypes(MatchesK, GetResultType);
    emitError(Twine(BuiltinName) + ": expected Result element type to be " + ValidResultTypes +
                  " for K Dim = " + Twine(Key.K) + " for targeted HW. Actual: " + getElTypeStr(Key.ResultElemTy),
              CI);
    return;
  }

  // Validate Matrix A element type.
  auto MatchesKAndResult = [&](const SupportedTableKey &Candidate) {
    return Candidate.K == Key.K && Candidate.ResultElemTy == Key.ResultElemTy;
  };
  auto GetAType = [](const SupportedTableKey &Candidate) { return Candidate.AElemTy; };
  if (!hasMatchingType(MatchesKAndResult, GetAType, Key.AElemTy)) {
    std::string ValidATypes = getValidTypes(MatchesKAndResult, GetAType);
    emitError(Twine(BuiltinName) + ": expected A element type to be " + ValidATypes + " for K Dim = " + Twine(Key.K) +
                  ", for Result element type " + getElTypeStr(Key.ResultElemTy) +
                  ", for targeted HW. Actual: " + getElTypeStr(Key.AElemTy),
              CI);
    return;
  }

  // Validate Matrix B element type.
  auto MatchesKResultAndA = [&](const SupportedTableKey &Candidate) {
    return Candidate.K == Key.K && Candidate.ResultElemTy == Key.ResultElemTy && Candidate.AElemTy == Key.AElemTy;
  };
  auto GetBType = [](const SupportedTableKey &Candidate) { return Candidate.BElemTy; };
  if (!hasMatchingType(MatchesKResultAndA, GetBType, Key.BElemTy)) {
    std::string ValidBTypes = getValidTypes(MatchesKResultAndA, GetBType);
    emitError(Twine(BuiltinName) + ": expected B element type to be " + ValidBTypes + " for K Dim = " + Twine(Key.K) +
                  ", for Result element type " + getElTypeStr(Key.ResultElemTy) + ", for A element type " +
                  getElTypeStr(Key.AElemTy) + ", for targeted HW. Actual: " + getElTypeStr(Key.BElemTy),
              CI);
    return;
  }

  // Validate operands for the matching dimension and element types.
  SmallVector<uint32_t, 8> ValidOperands;
  SupportedTableKey MatchKey = Key;
  for (const auto &[Candidate, _] : Table) {
    MatchKey.Operands = Candidate.Operands;
    if (Candidate == MatchKey)
      ValidOperands.push_back(Candidate.Operands);
  }
  sortAndDeduplicate(ValidOperands);

  std::stringstream ss;
  ss << BuiltinName.str() << ": expected Operands to be one of these combinations:\n";
  for (uint32_t Operands : ValidOperands)
    ss << Operands << ": " << GetHumanReadableOperand(Operands) << "\n";
  ss << "for K Dim = " << Key.K << ", for Result element type " << getElTypeStr(Key.ResultElemTy).str();
  ss << ", for A element type " << getElTypeStr(Key.AElemTy).str() << ", for B element type "
     << getElTypeStr(Key.BElemTy).str() << ", for targeted HW.\n";
  ss << "Actual: " << Key.Operands << ": " << GetHumanReadableOperand(Key.Operands);

  emitError(ss.str(), CI);
}

void SpvSubgroupMMAResolution::lowerToDpasBuiltin(CallInst &CI, Function *F) {
  int numArgs = IGCLLVM::getNumArgOperands(&CI);
  if (numArgs != 5) {
    emitError(Twine(DpasOpName) + ": invalid number of arguments. Expected 5. Actual " + std::to_string(numArgs), CI);
    return;
  }

  // Get arguments
  Type *ResultTy = CI.getType();
  Value *kDim = CI.getArgOperand(0);
  Value *a = CI.getArgOperand(1);
  Value *b = CI.getArgOperand(2);
  Value *c = CI.getArgOperand(3);
  Value *OpValue = CI.getArgOperand(4);

  if (!validateI32Constant(OpValue, "Operands", DpasOpName, CI))
    return;
  uint32_t Operands = cast<ConstantInt>(OpValue)->getZExtValue();

  if (!validateCType(ResultTy, c->getType(), DpasOpName, CI))
    return;

  ElType ResultElemTy = getValidMatrixType(ResultTy);
  ElType AElemTy = getValidMatrixType(a->getType());
  ElType BElemTy = getValidMatrixType(b->getType());

  if (!validateElementType(ResultElemTy, "Result", DpasOpName, CI))
    return;
  if (!validateElementType(AElemTy, "Matrix A", DpasOpName, CI))
    return;
  if (!validateElementType(BElemTy, "Matrix B", DpasOpName, CI))
    return;

  // The number of components in Result Type defines the M dimension.
  // If Result Type is a scalar type, the M dimension is one.
  int M = getElemCount(ResultTy);
  int AElemCount = getElemCount(a->getType());
  int BElemCount = getElemCount(b->getType());
  if (!validateDpasElemCounts(M, AElemCount, BElemCount, Operands, CI))
    return;

  if (!validateI32Constant(kDim, "K Dim", DpasOpName, CI))
    return;
  int K = cast<ConstantInt>(kDim)->getZExtValue();

  const SupportedTable &Table = getSupportedTable();
  SupportedTableKey Key{static_cast<uint32_t>(K), ResultElemTy, AElemTy, BElemTy, Operands};
  auto SupportedIt = Table.find(Key);
  if (SupportedIt == Table.end()) {
    diagnoseUnsupportedCombination(Key, Table, DpasOpName, CI);
    return;
  }

  // creating IB built-in
  SmallVector<Value *, 3> args({c, a, b});
  SmallVector<Type *, 3> argTypes({c->getType(), a->getType(), b->getType()});
  FunctionType *FT = FunctionType::get(CI.getType(), argTypes, false);

  std::string subgroupSize;
  if (isDoubleSubgroup(CI)) {
    subgroupSize = "32n16";
    M *= 2;
  } else {
    subgroupSize = m_Ctx->platform.hasExecSize16DPAS() ? "16" : "";
  }

  std::stringstream newFuncName;
  newFuncName << "__builtin_IB_sub_group" << subgroupSize;
  newFuncName << "_" << (ResultElemTy == I32 ? "i" : "f");
  newFuncName << "dpas_" << SupportedIt->second.str() << "8_" << M;

  auto newFunc = m_Module->getOrInsertFunction(newFuncName.str(), FT);
  auto newCall = CallInst::Create(newFunc, args, "", IGCLLVM::insertPosition(&CI));

  CI.replaceAllUsesWith(newCall);
  CI.eraseFromParent();
  m_Changed = true;

  if (F->use_empty())
    m_BuiltinsToRemove.insert(F);
}

void SpvSubgroupMMAResolution::lowerToBdpasBuiltin(CallInst &CI, Function *F) {
  // Validate number of arguments: 6 + 1 optional
  int numArgs = IGCLLVM::getNumArgOperands(&CI);
  if (numArgs != 6 && numArgs != 7) {
    emitError(Twine(BdpasOpName) + ": invalid number of arguments. Expected 6 or 7. Actual " + std::to_string(numArgs),
              CI);
    return;
  }

  // Validate platform support
  if (!m_Ctx->platform.isCoreChildOf(IGFX_XE3P_CORE)) {
    emitError(Twine(BdpasOpName) + ": not supported on targeted HW.", CI);
    return;
  }

  // Sub-group size 32 is spec-defined, but not yet implemented in IGC
  if (isDoubleSubgroup(CI)) {
    emitError(Twine(BdpasOpName) + ": sub-group size 32 is not supported yet.", CI);
    return;
  }

  // Get arguments
  Type *ResultTy = CI.getType();
  Value *kDim = CI.getArgOperand(0);
  Value *a = CI.getArgOperand(1);
  Value *b = CI.getArgOperand(2);
  Value *c = CI.getArgOperand(3);
  Value *scaleA = CI.getArgOperand(4);
  Value *scaleB = CI.getArgOperand(5);

  // Get optional Operands argument
  uint32_t Operands = 0;
  if (numArgs == 7) {
    Value *OpValue = CI.getArgOperand(6);
    if (!validateI32Constant(OpValue, "Operands", BdpasOpName, CI))
      return;
    Operands = cast<ConstantInt>(OpValue)->getZExtValue();
  }

  // Validate argument types and values
  if (!validateCType(ResultTy, c->getType(), BdpasOpName, CI))
    return;

  ElType ResultElemTy = getValidMatrixType(ResultTy);
  ElType AElemTy = getValidMatrixType(a->getType());
  ElType BElemTy = getValidMatrixType(b->getType());

  if (!validateElementType(ResultElemTy, "Result", BdpasOpName, CI))
    return;
  if (!validateElementType(AElemTy, "Matrix A", BdpasOpName, CI))
    return;
  if (!validateElementType(BElemTy, "Matrix B", BdpasOpName, CI))
    return;

  int M = getElemCount(ResultTy);
  int AElemCount = getElemCount(a->getType());
  int BElemCount = getElemCount(b->getType());
  if (!validateBdpasElemCounts(M, AElemCount, BElemCount, CI))
    return;

  if (!validateI32Constant(kDim, "K Dim", BdpasOpName, CI))
    return;
  int K = cast<ConstantInt>(kDim)->getZExtValue();

  if (!validateScaleType(scaleA, "Scale A", K, CI))
    return;
  if (!validateScaleType(scaleB, "Scale B", K, CI))
    return;

  const SupportedTable &Table = getSimd16ScaledTable();
  SupportedTableKey Key{static_cast<uint32_t>(K), ResultElemTy, AElemTy, BElemTy, Operands};
  auto SupportedIt = Table.find(Key);
  if (SupportedIt == Table.end()) {
    diagnoseUnsupportedCombination(Key, Table, BdpasOpName, CI);
    return;
  }

  // Create IB built-in
  SmallVector<Value *, 5> args({c, a, b, scaleA, scaleB});
  SmallVector<Type *, 5> argTypes({c->getType(), a->getType(), b->getType(), scaleA->getType(), scaleB->getType()});
  FunctionType *FT = FunctionType::get(CI.getType(), argTypes, false);

  // Per spec, SD=8 and RC=8 are hardcoded
  std::stringstream newFuncName;
  newFuncName << "__builtin_IB_sub_group16_bdpas_" << SupportedIt->second.str() << "8_8";

  auto newFunc = m_Module->getOrInsertFunction(newFuncName.str(), FT);
  auto newCall = CallInst::Create(newFunc, args, "", IGCLLVM::insertPosition(&CI));

  CI.replaceAllUsesWith(newCall);
  CI.eraseFromParent();
  m_Changed = true;

  if (F->use_empty())
    m_BuiltinsToRemove.insert(F);
}

void SpvSubgroupMMAResolution::visitCallInst(CallInst &CI) {
  Function *F = CI.getCalledFunction();
  if (!F)
    return;

  StringRef funcName = F->getName();
  if (funcName.contains(BdpasOpName)) {
    lowerToBdpasBuiltin(CI, F);
    return;
  }

  if (funcName.contains(DpasOpName))
    lowerToDpasBuiltin(CI, F);
}

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses SpvSubgroupMMAResolutionNPM::run(Module &M, ModuleAnalysisManager &AM) {
  auto md = AM.getResult<MetaDataUtilsAnalysis>(M);
  bool changed = SpvSubgroupMMAResolution().run(M, AM.getResult<CodeGenContextAnalysis>(M).Ctx, md.MdUtils);
  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16
