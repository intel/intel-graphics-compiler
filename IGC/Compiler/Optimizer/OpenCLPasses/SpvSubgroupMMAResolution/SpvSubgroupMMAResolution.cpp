/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SpvSubgroupMMAResolution.hpp"

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
#include "llvmWrapper/IR/Instructions.h"
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;
using namespace IGC;

char SpvSubgroupMMAResolution::ID = 0;
SpvSubgroupMMAResolution::SupportedTable SpvSubgroupMMAResolution::m_Simd8Table;
SpvSubgroupMMAResolution::SupportedTable SpvSubgroupMMAResolution::m_Simd16Table;

#define PASS_FLAG "igc-spv-subgroup-mma-resolution"
#define PASS_DESC "Lowering of SPIR-V INTEL subgroup_matrix_multiply_accumulate instructions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
#define DEBUG_TYPE "spv-subgroup-mma-resolution"

IGC_INITIALIZE_PASS_BEGIN(SpvSubgroupMMAResolution, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(SpvSubgroupMMAResolution, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

SpvSubgroupMMAResolution::SpvSubgroupMMAResolution() : ModulePass(ID) {
  initializeSpvSubgroupMMAResolutionPass(*PassRegistry::getPassRegistry());
}

bool SpvSubgroupMMAResolution::runOnModule(Module &M) {
  m_BuiltinsToRemove.clear();
  m_Module = &M;
  m_Changed = false;
  m_Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  visit(M);

  for (auto &F : m_BuiltinsToRemove)
    F->eraseFromParent();

  return m_Changed;
}

enum {
  None = 0,
  MatrixASignedComponentsINTEL = 1 << 0,
  MatrixBSignedComponentsINTEL = 1 << 1,
  MatrixCBFloat16INTEL = 1 << 2,
  MatrixResultBFloat16INTEL = 1 << 3,
  MatrixAPackedInt8INTEL = 1 << 4,
  MatrixBPackedInt8INTEL = 1 << 5,
  MatrixAPackedInt4INTEL = 1 << 6,
  MatrixBPackedInt4INTEL = 1 << 7,
  MatrixATF32INTEL = 1 << 8,
  MatrixBTF32INTEL = 1 << 9,
  MatrixAPackedFloat16INTEL = 1 << 10,
  MatrixBPackedFloat16INTEL = 1 << 11,
  MatrixAPackedBFloat16INTEL = 1 << 12,
  MatrixBPackedBFloat16INTEL = 1 << 13,
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

  if (operands.empty())
    return "None";

  return llvm::join(operands, " | ");
}

void SpvSubgroupMMAResolution::populateSimd8Table() {
  // 8-bit integer matrix sources (signed and unsigned), 32-bit integer accumulator:
  m_Simd8Table[32][ElType::I32][ElType::I32][ElType::I32][MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL] = "u8_u8_";
  m_Simd8Table[32][ElType::I32][ElType::I32][ElType::I32]
              [MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL | MatrixASignedComponentsINTEL] = "s8_u8_";
  m_Simd8Table[32][ElType::I32][ElType::I32][ElType::I32]
              [MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL | MatrixBSignedComponentsINTEL] = "u8_s8_";
  m_Simd8Table[32][ElType::I32][ElType::I32][ElType::I32][MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL |
                                                          MatrixASignedComponentsINTEL | MatrixBSignedComponentsINTEL] =
      "s8_s8_";

  // 4-bit integer matrix sources (signed and unsigned), 32-bit integer accumulator:
  m_Simd8Table[64][ElType::I32][ElType::I32][ElType::I32][MatrixAPackedInt4INTEL | MatrixBPackedInt4INTEL] = "u4_u4_";
  m_Simd8Table[64][ElType::I32][ElType::I32][ElType::I32]
              [MatrixAPackedInt4INTEL | MatrixBPackedInt4INTEL | MatrixASignedComponentsINTEL] = "s4_u4_";
  m_Simd8Table[64][ElType::I32][ElType::I32][ElType::I32]
              [MatrixAPackedInt4INTEL | MatrixBPackedInt4INTEL | MatrixBSignedComponentsINTEL] = "u4_s4_";
  m_Simd8Table[64][ElType::I32][ElType::I32][ElType::I32][MatrixAPackedInt4INTEL | MatrixBPackedInt4INTEL |
                                                          MatrixASignedComponentsINTEL | MatrixBSignedComponentsINTEL] =
      "s4_s4_";

  // fp16 matrix sources, fp32 accumulator:
  m_Simd8Table[16][ElType::F32][ElType::I32][ElType::I32][MatrixAPackedFloat16INTEL | MatrixBPackedFloat16INTEL] =
      "hf_hf_";
  // bf16 matrix sources, fp32 accumulator:
  m_Simd8Table[16][ElType::F32][ElType::I32][ElType::I32][MatrixAPackedBFloat16INTEL | MatrixBPackedBFloat16INTEL] =
      "bf_bf_";
}

void SpvSubgroupMMAResolution::populateSimd16Table() {
  // 8-bit integer matrix sources (signed and unsigned), 32-bit integer accumulator:
  m_Simd16Table[32][ElType::I32][ElType::I16][ElType::I32][MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL] = "u8_u8_";
  m_Simd16Table[32][ElType::I32][ElType::I16][ElType::I32]
               [MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL | MatrixASignedComponentsINTEL] = "s8_u8_";
  m_Simd16Table[32][ElType::I32][ElType::I16][ElType::I32]
               [MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL | MatrixBSignedComponentsINTEL] = "u8_s8_";
  m_Simd16Table[32][ElType::I32][ElType::I16][ElType::I32][MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL |
                                                           MatrixASignedComponentsINTEL |
                                                           MatrixBSignedComponentsINTEL] = "s8_s8_";

  // 4-bit integer matrix sources (signed and unsigned), 32-bit integer accumulator:
  m_Simd16Table[64][ElType::I32][ElType::I16][ElType::I32][MatrixAPackedInt4INTEL | MatrixBPackedInt4INTEL] = "u4_u4_";
  m_Simd16Table[64][ElType::I32][ElType::I16][ElType::I32]
               [MatrixAPackedInt4INTEL | MatrixBPackedInt4INTEL | MatrixASignedComponentsINTEL] = "s4_u4_";
  m_Simd16Table[64][ElType::I32][ElType::I16][ElType::I32]
               [MatrixAPackedInt4INTEL | MatrixBPackedInt4INTEL | MatrixBSignedComponentsINTEL] = "u4_s4_";
  m_Simd16Table[64][ElType::I32][ElType::I16][ElType::I32][MatrixAPackedInt4INTEL | MatrixBPackedInt4INTEL |
                                                           MatrixASignedComponentsINTEL |
                                                           MatrixBSignedComponentsINTEL] = "s4_s4_";

  // fp16 matrix sources, fp32 accumulator:
  m_Simd16Table[16][ElType::F32][ElType::I16][ElType::I32][MatrixAPackedFloat16INTEL | MatrixBPackedFloat16INTEL] =
      "f_f_hf_hf_";
  // bf16 matrix sources, fp32 accumulator:
  m_Simd16Table[16][ElType::F32][ElType::I16][ElType::I32][MatrixAPackedBFloat16INTEL | MatrixBPackedBFloat16INTEL] =
      "f_f_bf_bf_";
  // fp16 matrix sources, fp16 accumulator:
  m_Simd16Table[16][ElType::F16][ElType::I16][ElType::I32][MatrixAPackedFloat16INTEL | MatrixBPackedFloat16INTEL] =
      "hf_hf_hf_hf_";
  // bf16 matrix sources, bf16 accumulator:
  m_Simd16Table[16][ElType::I16][ElType::I16][ElType::I32][MatrixResultBFloat16INTEL | MatrixAPackedBFloat16INTEL |
                                                           MatrixBPackedBFloat16INTEL | MatrixCBFloat16INTEL] =
      "bf_bf_bf_bf_";

  // tf32 matrix sources, fp32 accumulator:
  m_Simd16Table[8][ElType::F32][ElType::F32][ElType::F32][MatrixATF32INTEL | MatrixBTF32INTEL] = "f_f_tf32_tf32_";

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

bool SpvSubgroupMMAResolution::validateI32Constant(const Value *V, const Twine &ParamName, const CallInst &CI) {
  if (!isa<ConstantInt>(V) || !V->getType()->isIntegerTy(32)) {
    emitError(Twine("__spirv_SubgroupMatrixMultiplyAccumulateINTEL: ") + ParamName +
                  " argument must be a constant scalar 32-bit integer",
              CI);
    return false;
  }
  return true;
}

bool SpvSubgroupMMAResolution::validateCType(const Type *ResultTy, const Type *CType, const CallInst &CI) {
  if (ResultTy == CType)
    return true;

  std::string msg;
  raw_string_ostream rso(msg);
  rso << "__spirv_SubgroupMatrixMultiplyAccumulateINTEL: expected Result type to match type of Matrix C for targeted "
         "HW. Result type: ";

  ResultTy->print(rso);
  rso << ", Matrix C type: ";
  CType->print(rso);
  emitError(msg, CI);

  return false;
}

bool SpvSubgroupMMAResolution::validateElementType(const ElType ElemTy, StringRef ParamName, const CallInst &CI) {
  if (ElemTy != Unknown)
    return true;

  emitError(Twine("__spirv_SubgroupMatrixMultiplyAccumulateINTEL: expected ") + ParamName +
                " to be a scalar or vector of int32_t, int16_t, float32_t, or float16_t for targeted HW",
            CI);
  return false;
}

int SpvSubgroupMMAResolution::getElemCount(const Type *Ty) const {
  if (auto *VTy = dyn_cast<FixedVectorType>(Ty))
    return VTy->getNumElements();
  return 1;
}

bool SpvSubgroupMMAResolution::validateElemCounts(int M, int AElemCount, int BElemCount, uint32_t Operands,
                                                  CallInst &CI) {
  if (M != 1 && M != 2 && M != 4 && M != 8) {
    emitError(
        "__spirv_SubgroupMatrixMultiplyAccumulateINTEL: M dimension must be 1, 2, 4 or 8 for targeted HW. Actual: " +
            std::to_string(M),
        CI);
    return false;
  }
  if (Operands & MatrixATF32INTEL) {
    int expected = std::ceil(M / 2.0);
    if (AElemCount != expected) {
      emitError("__spirv_SubgroupMatrixMultiplyAccumulateINTEL: Matrix A argument must have ceil(M/2) components "
                "when MatrixATF32INTEL operand is set for targeted HW. Expected " +
                    std::to_string(expected) + ". Actual " + std::to_string(M),
                CI);
      return false;
    }
  } else if (AElemCount != M) {
    emitError("__spirv_SubgroupMatrixMultiplyAccumulateINTEL: Matrix A argument must have size " + std::to_string(M) +
                  " to match M defined by Result type for targeted HW. Actual: " + std::to_string(AElemCount),
              CI);
    return false;
  }
  const int expectedBCount = isDoubleSubgroup(CI) ? 4 : 8;
  if (BElemCount != expectedBCount) {
    emitError("__spirv_SubgroupMatrixMultiplyAccumulateINTEL: Matrix B argument must have " +
                  std::to_string(expectedBCount) +
                  " components for targeted  HW. Actual: " + std::to_string(BElemCount),
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
  return IGC::getSIMDSize(getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(), CI.getParent()->getParent()) == 32;
}

SpvSubgroupMMAResolution::SupportedTable *SpvSubgroupMMAResolution::getSupportedTable() {
  if (m_Ctx->platform.hasExecSize16DPAS()) {
    if (m_Simd16Table.empty())
      populateSimd16Table();
    return &m_Simd16Table;
  }
  if (m_Simd8Table.empty())
    populateSimd8Table();
  return &m_Simd8Table;
}

template <typename T>
bool SpvSubgroupMMAResolution::validateKDimInTable(const T KIt, int K, const SupportedTable *table,
                                                   const CallInst &CI) {
  if (KIt != table->end())
    return true;

  SmallVector<std::string, 8> validKDims;
  for (const auto &it : *table)
    validKDims.push_back(std::to_string(it.first));

  emitError(Twine("__spirv_SubgroupMatrixMultiplyAccumulateINTEL: expected K Dim = ") + llvm::join(validKDims, " or ") +
                " for targeted HW. Actual: " + Twine(K),
            CI);
  return false;
}

template <typename TableType> std::string SpvSubgroupMMAResolution::getValidTypesStr(const TableType &table) const {
  SmallVector<std::string, 8> validTypes;
  for (const auto &it : table)
    validTypes.push_back(getElTypeStr(it.first).str());
  return llvm::join(validTypes, " or ");
}

template <typename T>
bool SpvSubgroupMMAResolution::validateResultElementInTable(const T RIt, int K, ElType ResultElemTy,
                                                            const RTable &table, const CallInst &CI) {
  if (RIt != table.end())
    return true;

  emitError(Twine("__spirv_SubgroupMatrixMultiplyAccumulateINTEL: expected Result element type to be ") +
                getValidTypesStr(table) + " for K Dim = " + Twine(K) +
                " for targeted HW. Actual: " + getElTypeStr(ResultElemTy),
            CI);
  return false;
}

template <typename T>
bool SpvSubgroupMMAResolution::validateAElementInTable(const T AIt, int K, ElType ResultElemTy, ElType AElemTy,
                                                       const ATable &table, const CallInst &CI) {
  if (AIt != table.end())
    return true;

  emitError(Twine("__spirv_SubgroupMatrixMultiplyAccumulateINTEL: expected A element type to be ") +
                getValidTypesStr(table) + " for K Dim = " + Twine(K) + ", for Result element type " +
                getElTypeStr(ResultElemTy) + ", for targeted HW. Actual: " + getElTypeStr(AElemTy),
            CI);
  return false;
}

template <typename T>
bool SpvSubgroupMMAResolution::validateBElementInTable(const T BIt, int K, ElType ResultElemTy, ElType AElemTy,
                                                       ElType BElemTy, const BTable &table, const CallInst &CI) {
  if (BIt != table.end())
    return true;

  emitError(Twine("__spirv_SubgroupMatrixMultiplyAccumulateINTEL: expected B element type to be ") +
                getValidTypesStr(table) + " for K Dim = " + Twine(K) + ", for Result element type " +
                getElTypeStr(ResultElemTy) + ", for A element type " + getElTypeStr(AElemTy) +
                ", for targeted HW. Actual: " + getElTypeStr(BElemTy),
            CI);
  return false;
}

template <typename T>
bool SpvSubgroupMMAResolution::validateOperands(const T OpIt, int K, ElType ResultElemTy, ElType AElemTy,
                                                ElType BElemTy, uint32_t Operands, const OperandsTable &operandMap,
                                                const CallInst &CI) {
  if (OpIt != operandMap.end())
    return true;

  std::stringstream ss;
  ss << "__spirv_SubgroupMatrixMultiplyAccumulateINTEL: expected Operands to be one of these combinations:\n";
  for (const auto &it : operandMap)
    ss << it.first << ": " << GetHumanReadableOperand(it.first) << "\n";
  ss << "for K Dim = " << K << ", for Result element type " << getElTypeStr(ResultElemTy).str();
  ss << ", for A element type " << getElTypeStr(AElemTy).str() << ", for B element type " << getElTypeStr(BElemTy).str()
     << ", for targeted HW.\n";
  ss << "Actual: " << Operands << ": " << GetHumanReadableOperand(Operands);

  emitError(ss.str(), CI);
  return false;
}

void SpvSubgroupMMAResolution::visitCallInst(CallInst &CI) {
  Function *F = CI.getCalledFunction();
  if (!F)
    return;

  StringRef funcName = F->getName();
  if (!funcName.contains("__spirv_SubgroupMatrixMultiplyAccumulateINTEL"))
    return;

  int numArgs = IGCLLVM::getNumArgOperands(&CI);
  if (numArgs != 5) {
    emitError("__spirv_SubgroupMatrixMultiplyAccumulateINTEL: invalid number of arguments. Expected 5. Actual " +
                  std::to_string(numArgs),
              CI);
    return;
  }

  // Get arguments
  Type *ResultTy = CI.getType();
  Value *kDim = CI.getArgOperand(0);
  Value *a = CI.getArgOperand(1);
  Value *b = CI.getArgOperand(2);
  Value *c = CI.getArgOperand(3);
  Value *OpVaue = CI.getArgOperand(4);

  if (!validateI32Constant(OpVaue, "Operands", CI))
    return;
  uint32_t Operands = cast<ConstantInt>(OpVaue)->getZExtValue();

  if (!validateCType(ResultTy, c->getType(), CI))
    return;

  ElType ResultElemTy = getValidMatrixType(ResultTy);
  ElType AElemTy = getValidMatrixType(a->getType());
  ElType BElemTy = getValidMatrixType(b->getType());

  if (!validateElementType(ResultElemTy, "Result", CI))
    return;
  if (!validateElementType(AElemTy, "Matrix A", CI))
    return;
  if (!validateElementType(BElemTy, "Matrix B", CI))
    return;

  // The number of components in Result Type defines the M dimension.
  // If Result Type is a scalar type, the M dimension is one.
  int M = getElemCount(ResultTy);
  int AElemCount = getElemCount(a->getType());
  int BElemCount = getElemCount(b->getType());
  if (!validateElemCounts(M, AElemCount, BElemCount, Operands, CI))
    return;

  if (!validateI32Constant(kDim, "K Dim", CI))
    return;
  int K = cast<ConstantInt>(kDim)->getZExtValue();

  SupportedTable *table = getSupportedTable();
  auto KIt = table->find(K);
  if (!validateKDimInTable(KIt, K, table, CI))
    return;

  auto ResultIt = KIt->second.find(ResultElemTy);
  if (!validateResultElementInTable(ResultIt, K, ResultElemTy, KIt->second, CI))
    return;

  auto AIt = ResultIt->second.find(AElemTy);
  if (!validateAElementInTable(AIt, K, ResultElemTy, AElemTy, ResultIt->second, CI))
    return;

  auto BIt = AIt->second.find(BElemTy);
  if (!validateBElementInTable(BIt, K, ResultElemTy, AElemTy, BElemTy, AIt->second, CI))
    return;

  auto OperandsIt = BIt->second.find(Operands);
  if (!validateOperands(OperandsIt, K, ResultElemTy, AElemTy, BElemTy, Operands, BIt->second, CI))
    return;

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
  newFuncName << "dpas_" << OperandsIt->second.str() << "8_" << M;

  auto newFunc = m_Module->getOrInsertFunction(newFuncName.str(), FT);
  auto newCall = CallInst::Create(newFunc, args, "", &CI);

  CI.replaceAllUsesWith(newCall);
  CI.eraseFromParent();
  m_Changed = true;

  if (F->use_empty())
    m_BuiltinsToRemove.insert(F);
}
