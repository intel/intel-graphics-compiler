/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/DpasFuncs/DpasFuncsResolution.hpp"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "llvmWrapper/IR/DerivedTypes.h"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using IGCLLVM::FixedVectorType;

namespace {
// Types for destination and accumulate.
enum DstAccType {
  DSTACC_UNUSED,
  DSTACC_FLOAT,
  DSTACC_FP16,
  DSTACC_BF16,
  DSTACC_INT32
};

/// @brief  DpasFuncsTranslation pass : tranlate dpas builtin (__builtin_IB_*dpas*) into igc intrinsic.
///         It also may combine several dpas intrinsics into a single one.
class DpasFuncsResolution : public FunctionPass, public InstVisitor<DpasFuncsResolution> {
public:
  // Pass identification, replacement for typeid
  static char ID;

  DpasFuncsResolution();

  ~DpasFuncsResolution() {}

  /// @brief  Provides name of pass
  virtual StringRef getPassName() const override {
    // This string was changed from "DpasFuncsTranslation" due to IP leaks concers.
    return "ArithmeticFuncsTranslation";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
  }

  virtual bool runOnFunction(Function &F) override;

  void visitCallInst(CallInst &CI);

private:
  /// Demangle the suffix of dpas. Return true if sucessful; false otherwise.
  ///   Suffix's format:  [w_][<DstTy>_<AccTy>_][<PA>_<PB>_]<SD>_<RC>  (see below)
  bool demangleSuffix(StringRef FN, int StartPos, bool HasDstAcc, bool IsIDpas, int &DstTy, int &AccTy, int &PA,
                      int &PB, int &SD, int &RC, bool *IsDpasw);

  /// Demangle the suffix of BFCvt. Return true if sucessful; false otherwise.
  ///   Suffix's format:  [<rm>_]<1|2|4|8|16>  (see description below)
  bool demangleFCvtSuffix(StringRef FN, int StartPos, int *pRM, int *pVecLen, bool *pIsSat);

  /// Indicates if the pass changed the processed function
  bool m_changed{};

  CodeGenContext *m_pCtx = nullptr;
  std::string m_ErrorMsg{};

  ///  XeHP_SDV's simd8 intrinsics
  ///
  ///  The dpas builtin function's name has the suffix format as
  ///    <a's precision>_<b's precision>_<systolicDepth>_<repeatCount>
  ///  They are divided into four groups:
  ///    1. Sub group versions (using other simd-lane's data):
  ///      1.1 __builtin_IB_sub_group_idpas[w]_<s|u><2|4|8>_<s|u><2|4|8>_8_<1-8> (acc, a, b)
  ///      1.2 __builtin_IB_sub_group_fdpas[w]_bf_bf_8_<1-8> (acc, a, b)
  ///          __builtin_IB_sub_group_fdpas[w]_hf_hf_8_<1-8> (acc, a, b)
  ///    2. Work-item versions (using its own data, not using cross-lane data)
  ///      2.1 __builtin_IB_idpas[w]_<s|u><2|4|8>_<s|u><2|4|8>_8_<1-8> (acc, a, b)
  ///      2.2 __builtin_IB_fdpas[w]_bf_bf_8_<1-8> (acc, a, b)
  ///          __builtin_IB_fdpas[w]_hf_hf_8_<1-8> (acc, a, b)
  ///
  ///  Note that <a|b|c> denotes one of a, b, or c. "1-8" denotes 1, 2, ..., up to 8.
  ///  And for dpasw, repeat count = 2|4|8 are supported only for now.
  static const StringRef SG_PREFIX_IDPAS;
  static const StringRef SG_PREFIX_FDPAS;
  static const StringRef WI_PREFIX_IDPAS;
  static const StringRef WI_PREFIX_FDPAS;
  /// The following are intrinsic for PVC simd16 only.
  /// __builtin_IB_sub_group16_idpas<suffix>
  ///   <suffix> : _<a's precision>_<b's precision>_<depth>_<rcount>
  ///         ie.  _<u|s><2|4|8>_<u|s><2|4|8>_8_<1-8>
  ///              the same as XeHP_SDV simd8 intrinsic.
  /// __builtin_IB_sub_group16_fdpas<suffux>
  ///   <suffix> : _<retty>_<accty>_<aty>_<bty>_<depth>_<rcount>
  ///        1.   _<f|x>_<f|x>_<x>_<x>_8_<1-8>
  ///                 x:  <hf | bf>
  ///        2.   _f_f_tf32_tf32_8_<1-8>
       ///
  static const StringRef SG_PREFIX_IDPAS16;
  static const StringRef SG_PREFIX_FDPAS16;
  static const StringRef SG_PREFIX_IDPAS32N16;
  static const StringRef SG_PREFIX_FDPAS32N16;
  // PVC+: pure hf/bf dpas builtins
  static const StringRef WI_PREFIX_HFDPAS;
  static const StringRef WI_PREFIX_BFDPAS;
  static const StringRef SG_PREFIX_HFDPAS;
  static const StringRef SG_PREFIX_BFDPAS;
  static const StringRef SG_PREFIX_SDPAS16;
  static const StringRef SG_PREFIX_BDPAS16;

  /// The bf conversion builtin function's name has the format as
  ///    __builtin_IB_<srcType>to<dstType>[_<rm>]_<1|2|3|4|8|16>
  /// where
  ///    srcType/dstType : bf(as short) or f(float).
  ///       Note that 2bf (as int) and 2f are packed cvt from two float to a
  ///       pair of bf.
  ///    <rm> : rtz/rte/rtp/rtn
  ///           If rm is not present, it is default (rte).
  ///    <1|2|3|4|8|16> : vector size of its argument. "1" is for scalar.
  ///
  /// **Note that [_<rm>] denotes _<rm> is optional.**
  ///
  /// Currently, support builtin are:
  ///    __builtin_IB_ftobf[_<rm>]_<1|2|3|4|8|16>
  ///    __builtin_IB_bftof_<1|2|3|4|8|16>         // no RM as it is precise
  ///    __builtin_IB_2fto2bf[_<rm>]_<1|2|3|4|8|16>
  bool processCvt(CallInst &CI);

  ///  Naming convertion of Stochastic rounding builtin
  ///    __builtin_IB_srnd_ftohf_<1|2|3|4|8|16> (a,  r)
  ///    __builtin_IB_srnd_hftobf8_<1|2|3|4|8|16>(a,  r)
  bool processSrnd(CallInst &CI);
  ///  Naming convertion of Lfsr (linear feedback shift register)
  ///  __builtin_IB_lfsr_<b32|b16v2|b8v4> (seed, polynomial)
  bool processLfsr(CallInst &CI);
  ///  Naming convertion of dnscl
  ///  __builtin_IB_dnscl_<bf16|hf16> (src0, src1, convert_to, mode)
  ///  __builtin_IB_dnscl_<bf16|hf16>_srnd (src0, src1, bias, convert_to, mode)
  bool processDnscl(CallInst &CI);
  ///  Naming convertion of sdpas builtin
  /// __builtin_IB_sub_group16_sdpas__<retty>_<accty>_<aty>_<bty>_<depth>_<rcount>
  ///     retty/accty :  f|hf|bf|d
  ///     aty/bty     :  u8|s8|bf|bf8|hf8|hf|tf32
  ///     depth       :  16
  ///                    (b is compressed and is the half of the depth).
  ///     rcount      :  7-8
  ///  Only a limited set of combination of retty/accty/aty/bty is allowed.
  bool processSdpas(CallInst &CI);
  ///  Naming convetion of Bdpas
  ///  __builtin_IB_sub_group16_bdpas_<retty>_<accty>_<aty>_<bty>_<depth>_<rcount>
  ///     retty/accty :  f|hf|bf
  ///     aty/bty     :  bf|hf|e2m1
  ///     depth       :  8
  ///     rcount      :  8
  ///  Only a limited set of combination of retty/accty/aty/bty is allowed.
  bool processBdpas(CallInst &CI);

  ///////////////////////////////////////////////////////////////////
  /// StringRef parsing functions' common arguments
  ///   StrRef:  string to be parsed
  ///   StrPos:  the starting position of string.
  ///   StrRem:  the remaining number of chars at StrPos of StrRef.
  ///
  ///   Each function will parse particular patterns. Once found,
  ///   adjust StrPos to point to the next field, and StrRem to
  ///   the number of chars remained unparsed.
  ///
  /// The suffix patterns will be parsed by a sequence of parsing
  /// functions. If one parsing function fails, the parsing functions
  /// following the failing one in the sequence will definitely fails.
  /// With this, we can just check the status of the last parsing function
  /// to see if the entire sequence of parsing functions fail or not.
  ///////////////////////////////////////////////////////////////////

  // Parse type string for destination or accumulate operands
  // Pattern:  "_f" | "_hf" | "_bf"
  DstAccType parseDstAccType(StringRef StrRef, size_t &StrPos, size_t &StrRem);

  // parse wide version of dpas : "[w]"
  // If it is "w", return true; otherwise, return false.
  // (As 'w' is optional, this function never fails.)
  bool parseW(StringRef StrRef, size_t &StrPos, size_t &StrRem);

  //
  // Find the following patterns:
  //
  //    "_bf" | "_hf" | "_<s|u><2|4|8>" | tf32 | bf8 | hf8
  //
  // If success, return the type denoted by this string pattern;
  // otherwise, return PrecisionType::PRECISION_UNUSED.
  //
  PrecisionType parsePrecision(StringRef StrRef, size_t &StrPos, size_t &StrRem);

  // Pattern:  '_8'
  // Return depth if valid, return -1 otherwise.
  int parseDepth(StringRef StrRef, size_t &StrPos, size_t &StrRem);

  // Pattern:  '_<1-8>'
  // Return repeat count if valid, return -1 otherwise.
  int parseRCount(StringRef StrRef, size_t &StrPos, size_t &StrRem);

  inline bool isFP8Prec(int P) const { return P == PrecisionType::BF8 || P == PrecisionType::HF8; }
};
} // namespace

char DpasFuncsResolution::ID = 0;
const StringRef DpasFuncsResolution::SG_PREFIX_IDPAS = "__builtin_IB_sub_group_idpas";
const StringRef DpasFuncsResolution::SG_PREFIX_FDPAS = "__builtin_IB_sub_group_fdpas";
const StringRef DpasFuncsResolution::WI_PREFIX_IDPAS = "__builtin_IB_idpas";
const StringRef DpasFuncsResolution::WI_PREFIX_FDPAS = "__builtin_IB_fdpas";
const StringRef DpasFuncsResolution::SG_PREFIX_IDPAS16 = "__builtin_IB_sub_group16_idpas";
const StringRef DpasFuncsResolution::SG_PREFIX_FDPAS16 = "__builtin_IB_sub_group16_fdpas";
const StringRef DpasFuncsResolution::SG_PREFIX_IDPAS32N16 = "__builtin_IB_sub_group32n16_idpas";
const StringRef DpasFuncsResolution::SG_PREFIX_FDPAS32N16 = "__builtin_IB_sub_group32n16_fdpas";
// PVC+: pure hf/bf dpas builtins
const StringRef DpasFuncsResolution::WI_PREFIX_HFDPAS = "__builtin_IB_hfdpas";
const StringRef DpasFuncsResolution::WI_PREFIX_BFDPAS = "__builtin_IB_bfdpas";
const StringRef DpasFuncsResolution::SG_PREFIX_HFDPAS = "__builtin_IB_sub_group_hfdpas";
const StringRef DpasFuncsResolution::SG_PREFIX_BFDPAS = "__builtin_IB_sub_group_bfdpas";
const StringRef DpasFuncsResolution::SG_PREFIX_SDPAS16 = "__builtin_IB_sub_group16_sdpas";
const StringRef DpasFuncsResolution::SG_PREFIX_BDPAS16 = "__builtin_IB_sub_group16_bdpas";

// Register pass to igc-opt
#define PASS_FLAG                                                                                                      \
  "igc-arith-funcs-translation" // This string was changed from "igc-dpas-funcs-translation" due to IP leaks concers.
#define PASS_DESCRIPTION                                                                                               \
  "Translate arithmetic builtin functions into igc intrinsics" // This string was changed from "Translate dpas builtin
                                                               // functions into igc intrinsics" due to IP leaks
                                                               // concers.
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(DpasFuncsResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(DpasFuncsResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

DpasFuncsResolution::DpasFuncsResolution(void) : FunctionPass(ID) {
  initializeDpasFuncsResolutionPass(*PassRegistry::getPassRegistry());
}

bool DpasFuncsResolution::runOnFunction(Function &F) {
  m_pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  m_changed = false;

  visit(F);

  if (!m_ErrorMsg.empty()) {
    m_pCtx->EmitError(m_ErrorMsg.c_str(), &F);
    m_ErrorMsg.clear();
  }
  return m_changed;
}

void DpasFuncsResolution::visitCallInst(CallInst &CI) {
  // Skip if there is any error
  if (!m_ErrorMsg.empty()) {
    return;
  }

  if (processSrnd(CI)) {
    return;
  }
  if (processLfsr(CI)) {
    return;
  }
  if (processDnscl(CI)) {
    return;
  }
  if (processBdpas(CI)) {
    return;
  }
  // Handle bf cvt if it is.
  if (processCvt(CI)) {
    return;
  }

  /// Process DPAS intrinsics
  Function *func = CI.getCalledFunction();
  if (!func)
    return;
  StringRef funcName = func->getName();
  LLVMContext &Ctx = CI.getContext();
  Type *intTy = Type::getInt32Ty(Ctx);
  Type *boolTy = Type::getInt1Ty(Ctx);

  bool IsDpasw = false;
  bool IsIDpas = false;
  // Dimension N is platform specific and is directly correlated to minimum subgroup-size for
  // given platform. If DPAS with the same M, N, K dimensions is executed within a subgroup
  // twice the size of minimum subgroup-size, each work item must contain half of the data
  // compared to the minimum subgroup-size.
  bool IsDoubleSubgroup = false;
  int DstTy, AccTy, PA, PB, SD, RC;
  GenISAIntrinsic::ID iid = GenISAIntrinsic::no_intrinsic;
  bool doVerify = false;
#if defined(_DEBUG)
  doVerify = true;
#endif
  if (m_pCtx->platform.hasExecSize16DPAS()) {
    // PVC
    if (funcName.startswith(DpasFuncsResolution::SG_PREFIX_IDPAS16)) {
      const int SG_PREFIX_LEN = DpasFuncsResolution::SG_PREFIX_IDPAS16.size();
      IsIDpas = true;
      if (!demangleSuffix(funcName, SG_PREFIX_LEN, false, IsIDpas, DstTy, AccTy, PA, PB, SD, RC, nullptr))
        return;
      iid = GenISAIntrinsic::GenISA_sub_group_dpas;
    } else if (funcName.startswith(DpasFuncsResolution::SG_PREFIX_IDPAS32N16)) {
      const int SG_PREFIX_LEN = DpasFuncsResolution::SG_PREFIX_IDPAS32N16.size();
      IsIDpas = true;
      IsDoubleSubgroup = true;
      if (!demangleSuffix(funcName, SG_PREFIX_LEN, false, IsIDpas, DstTy, AccTy, PA, PB, SD, RC, nullptr))
        return;
      iid = GenISAIntrinsic::GenISA_sub_group_dpas;
    } else if (funcName.startswith(DpasFuncsResolution::SG_PREFIX_FDPAS16)) {
      const int SG_PREFIX_LEN = DpasFuncsResolution::SG_PREFIX_FDPAS16.size();
      IsIDpas = false;
      if (!demangleSuffix(funcName, SG_PREFIX_LEN, true, IsIDpas, DstTy, AccTy, PA, PB, SD, RC, nullptr))
        return;
      iid = GenISAIntrinsic::GenISA_sub_group_dpas;
    } else if (funcName.startswith(DpasFuncsResolution::SG_PREFIX_FDPAS32N16)) {
      const int SG_PREFIX_LEN = DpasFuncsResolution::SG_PREFIX_FDPAS32N16.size();
      IsIDpas = false;
      IsDoubleSubgroup = true;
      if (!demangleSuffix(funcName, SG_PREFIX_LEN, true, IsIDpas, DstTy, AccTy, PA, PB, SD, RC, nullptr))
        return;
      iid = GenISAIntrinsic::GenISA_sub_group_dpas;
    }
    else {
      return;
    }
  } else {
    if (funcName.startswith(DpasFuncsResolution::SG_PREFIX_IDPAS)) {
      const int SG_PREFIX_LEN = DpasFuncsResolution::SG_PREFIX_IDPAS.size();
      IsIDpas = true;
      if (!demangleSuffix(funcName, SG_PREFIX_LEN, false, IsIDpas, DstTy, AccTy, PA, PB, SD, RC, &IsDpasw))
        return;
      iid = GenISAIntrinsic::GenISA_sub_group_dpas;
    } else if (funcName.startswith(DpasFuncsResolution::SG_PREFIX_FDPAS)) {
      const int SG_PREFIX_LEN = DpasFuncsResolution::SG_PREFIX_FDPAS.size();
      IsIDpas = false;
      if (!demangleSuffix(funcName, SG_PREFIX_LEN, false, IsIDpas, DstTy, AccTy, PA, PB, SD, RC, &IsDpasw))
        return;
      iid = GenISAIntrinsic::GenISA_sub_group_dpas;
    } else if (funcName.startswith(DpasFuncsResolution::WI_PREFIX_IDPAS)) {
      const int WI_PREFIX_LEN = DpasFuncsResolution::WI_PREFIX_IDPAS.size();
      IsIDpas = true;
      if (!demangleSuffix(funcName, WI_PREFIX_LEN, false, IsIDpas, DstTy, AccTy, PA, PB, SD, RC, &IsDpasw))
        return;
      iid = GenISAIntrinsic::GenISA_dpas;
    } else if (funcName.startswith(DpasFuncsResolution::WI_PREFIX_FDPAS)) {
      const int WI_PREFIX_LEN = DpasFuncsResolution::WI_PREFIX_FDPAS.size();
      IsIDpas = false;
      if (!demangleSuffix(funcName, WI_PREFIX_LEN, false, IsIDpas, DstTy, AccTy, PA, PB, SD, RC, &IsDpasw))
        return;
      iid = GenISAIntrinsic::GenISA_dpas;
    } else if (funcName.startswith(DpasFuncsResolution::SG_PREFIX_HFDPAS) ||
               funcName.startswith(DpasFuncsResolution::SG_PREFIX_BFDPAS)) {
      const int SG_PREFIX_HF_LEN = DpasFuncsResolution::SG_PREFIX_HFDPAS.size();
      IsIDpas = false;
      if (!demangleSuffix(funcName, SG_PREFIX_HF_LEN, false, IsIDpas, DstTy, AccTy, PA, PB, SD, RC, &IsDpasw))
        return;
      iid = GenISAIntrinsic::GenISA_sub_group_dpas;
    } else if (funcName.startswith(DpasFuncsResolution::WI_PREFIX_HFDPAS) ||
               funcName.startswith(DpasFuncsResolution::WI_PREFIX_BFDPAS)) {
      const int WI_PREFIX_HF_LEN = DpasFuncsResolution::WI_PREFIX_HFDPAS.size();
      IsIDpas = false;
      if (!demangleSuffix(funcName, WI_PREFIX_HF_LEN, false, IsIDpas, DstTy, AccTy, PA, PB, SD, RC, &IsDpasw))
        return;
      iid = GenISAIntrinsic::GenISA_dpas;
    } else {
      return;
    }
  }

#if defined(_DEBUG) || defined(_INTERNAL)
  // verify that intrinsic is valid
  if (!IsDpasw && !m_pCtx->platform.supportDpasInstruction()) {
    m_ErrorMsg = "Dpas instruction not supported!";
    IGC_ASSERT_MESSAGE(0, "Dpas instruction not supported!");
    return;
  }
  if (IsDpasw && !m_pCtx->platform.supportDpaswInstruction()) {
    m_ErrorMsg = "Dpasw instruction not supported!";
    IGC_ASSERT_MESSAGE(0, "Dpasw instruction not supported!");
    return;
  }

  if (doVerify) {
    // Additional intrinsic checks
    Value *ACC = CI.getArgOperand(0);
    Value *A = CI.getArgOperand(1);
    Value *B = CI.getArgOperand(2);

    Type *DTy = CI.getType();
    Type *ACCTy = ACC->getType();
    Type *ATy = A->getType();
    Type *BTy = B->getType();
    int D_nelts = DTy->isVectorTy() ? (int)cast<FixedVectorType>(DTy)->getNumElements() : 1;
    int ACC_nelts = ACCTy->isVectorTy() ? (int)cast<FixedVectorType>(ACCTy)->getNumElements() : 1;
    int A_nelts = ATy->isVectorTy() ? (int)cast<FixedVectorType>(ATy)->getNumElements() : 1;
    int B_nelts = BTy->isVectorTy() ? (int)cast<FixedVectorType>(BTy)->getNumElements() : 1;
    Type *D_BaseTy = DTy->getScalarType();
    Type *ACC_BaseTy = ACCTy->getScalarType();
    Type *A_BaseTy = ATy->getScalarType();
    Type *B_BaseTy = BTy->getScalarType();

    if (IsDoubleSubgroup) {
      IGC_ASSERT_MESSAGE(RC >= 2, "ICE: repeat count of DPAS for double subgroup-size must be >= 2!");
      D_nelts *= 2;
      ACC_nelts *= 2;
      A_nelts *= 2;
      B_nelts *= 2;
    }

    if (IsIDpas) {
      uint32_t Abits = getPrecisionInBits((PrecisionType)PA);
      uint32_t Bbits = getPrecisionInBits((PrecisionType)PB);
      bool is_2xint8 = (Abits != 8 && Bbits != 8);
      uint32_t AbitsPerDepth = Abits * (is_2xint8 ? 8 : 4);
      uint32_t BbitsPerDepth = Bbits * (is_2xint8 ? 8 : 4);
      uint32_t B_nDW = (BbitsPerDepth * SD) / 32;
      if (m_pCtx->platform.hasExecSize16DPAS()) {
        // depth is still 8, the subgroup intrinsic will get
        // one-depth data from two work-items.
        AbitsPerDepth = AbitsPerDepth / 2;
      }

      if (DstTy != DSTACC_INT32 || AccTy != DSTACC_INT32 || D_nelts != RC || ACC_nelts != RC || B_nelts != B_nDW ||
          RC != (IsDpasw ? 2 * A_nelts : A_nelts)) {
        IGC_ASSERT_MESSAGE(0, "ICE: invalid integer dpas instructions!");
      }
      IGC_ASSERT_MESSAGE(A_BaseTy->isIntegerTy(AbitsPerDepth), "ICE: type of dpas[w]'s A wrong!");
      IGC_ASSERT_MESSAGE(B_BaseTy->isIntegerTy(32), "ICE: type of dpas[w]'s B should be int32!");
      IGC_ASSERT_MESSAGE(D_BaseTy->isIntegerTy(32), "ICE: type of dpas[w]'s D should int32!");
      IGC_ASSERT_MESSAGE(ACC_BaseTy->isIntegerTy(32), "ICE: type of dpas[w]'s ACC should int32!");
    }
    else if (PA == PrecisionType::BF8 || PA == PrecisionType::HF8 || PB == PrecisionType::BF8 ||
             PB == PrecisionType::HF8) {
      // Only BF8/HF8 combinations allowed
      const bool isAllowedFP8 = ((PA == PrecisionType::BF8 || PA == PrecisionType::HF8) &&
                                 (PB == PrecisionType::BF8 || PB == PrecisionType::HF8));
    } else if (PA == PrecisionType::E2M1) { // fdpas fp4
      IGC_ASSERT_MESSAGE(D_nelts == RC, "ICE: dpas intrinsic has mismatched vector sizes of arguments!");
      IGC_ASSERT_MESSAGE(ACC_nelts == RC, "ICE: dpas intrinsic has mismatched vector sizes of arguments!");
      IGC_ASSERT_MESSAGE(B_nelts == SD, "ICE: dpas intrinsic has mismatched vector sizes of arguments!");
      IGC_ASSERT_MESSAGE(SD == 8, "ICE: depth of DPAS for FP4 precision must be 8!");
      IGC_ASSERT_MESSAGE(DstTy == DSTACC_BF16 || DstTy == DSTACC_FLOAT, "ICE: wrong type of dpas dst!");
      IGC_ASSERT_MESSAGE(AccTy == DSTACC_BF16 || AccTy == DSTACC_FLOAT, "ICE: wrong type of dpas acc!");
      IGC_ASSERT_MESSAGE(PB == PrecisionType::E2M1, "ICE: wrong type of dpas B!");
    } else { // fdpas
      bool precOk = (PA == PB);
      const bool isFP8Dpas = isFP8Prec(PA) || isFP8Prec(PB);
      if (isFP8Dpas) {
        // any of BF8/HF8 combinations is allowed
        precOk = isFP8Prec(PA) && isFP8Prec(PB);
      }
      IGC_ASSERT_MESSAGE(D_nelts == RC, "ICE: dpas intrinsic has mismatched vector sizes of arguments!");
      IGC_ASSERT_MESSAGE(ACC_nelts == RC, "ICE: dpas intrinsic has mismatched vector sizes of arguments!");
      IGC_ASSERT_MESSAGE(B_nelts == SD, "ICE: dpas intrinsic has mismatched vector sizes of arguments!");
      IGC_ASSERT_MESSAGE(precOk, "ICE: dpas's A and B have illegal type combination!");
      IGC_ASSERT_MESSAGE(B_BaseTy->isIntegerTy(32) || (PB == PrecisionType::TF32 && B_BaseTy->isFloatTy()),
                         "ICE: dpas's arg B shall have base type int32 or float!");
      IGC_ASSERT_MESSAGE(
          (RC == (IsDpasw ? 2 * A_nelts : A_nelts) || (PA == PrecisionType::TF32 && (RC == 2 * A_nelts))),
          "ICE: dpas's arg A has wrong element size!");

      uint32_t AbitsPerDepth = 32;
      if (m_pCtx->platform.hasExecSize16DPAS()) {
        AbitsPerDepth = AbitsPerDepth / 2;
      }

      IGC_ASSERT_MESSAGE(A_BaseTy->isIntegerTy(AbitsPerDepth) || (PA == PrecisionType::TF32 && A_BaseTy->isFloatTy()),
                         "ICE: dpas intrinsic's A has wrong base type!");
      if (isFP8Dpas) {
        IGC_ASSERT_MESSAGE(DstTy == DSTACC_FLOAT && AccTy == DSTACC_FLOAT, "ICE: wrong type of dst for FP8 dpas!");
      } else if (PA == PrecisionType::TF32) {
        if (!(DstTy == DSTACC_FLOAT && AccTy == DSTACC_FLOAT)) {
          IGC_ASSERT_MESSAGE(false, "ICE: wrong type of dst/acc for TF32 dpas!");
        }
      }

      bool typeOK = false;
      if (DstTy == DSTACC_BF16 || AccTy == DSTACC_BF16) {
        typeOK = (typeOK || PA == PrecisionType::BF16);
        IGC_ASSERT_MESSAGE(typeOK, "ICE: wrong type of dpas dst/acc!");
      } else if (DstTy == DSTACC_FP16 || AccTy == DSTACC_FP16) {
        typeOK = (typeOK || PA == PrecisionType::FP16);
        IGC_ASSERT_MESSAGE(typeOK, "ICE: wrong type of dpas dst/acc!");
      }
    }
  }

#endif

  Value *args[8];
  args[0] = CI.getArgOperand(0);
  args[1] = CI.getArgOperand(1);

  Value *B = CI.getArgOperand(2);
  Type *BTy = B->getType();
  if (FixedVectorType *BVecTy = dyn_cast<FixedVectorType>(BTy); BVecTy && BTy->getScalarType()->isFloatTy()) {
    B = CastInst::Create(Instruction::CastOps::BitCast, B,
                         FixedVectorType::get(intTy, (unsigned)BVecTy->getNumElements()), B->getName() + ".cast", &CI);
  }
  args[2] = B;

  args[3] = ConstantInt::get(intTy, PA);
  args[4] = ConstantInt::get(intTy, PB);
  args[5] = ConstantInt::get(intTy, SD);
  args[6] = ConstantInt::get(intTy, RC);
  args[7] = ConstantInt::get(boolTy, IsDpasw);

  // ITys: overload types for this intrinsic
  Type *ITys[4] = {func->getReturnType(), args[0]->getType(), args[1]->getType(), args[2]->getType()};
  Function *dpasFunc = GenISAIntrinsic::getDeclaration(func->getParent(), iid, ITys);

  Instruction *dpasCall = CallInst::Create(dpasFunc, args, VALUE_NAME("dpas"), &CI);

  updateDebugLoc(&CI, dpasCall);

  CI.replaceAllUsesWith(dpasCall);
  CI.eraseFromParent();

  m_changed = true;
}

bool DpasFuncsResolution::processCvt(CallInst &CI) {
  Function *func = CI.getCalledFunction();
  if (!func)
    return false;
  StringRef funcName = func->getName();
  LLVMContext &Ctx = CI.getContext();
  Type *intTy = Type::getInt32Ty(Ctx);
  Type *boolTy = Type::getInt1Ty(Ctx);

  int FP_RM = ROUND_TO_NEAREST_EVEN; // default
  int VecLen;
  bool isSat;
  GenISAIntrinsic::ID iid;
  Value *args[3];
  uint32_t argslen;
  if (funcName.startswith("__builtin_IB_ftobf_")) {
    if (!demangleFCvtSuffix(funcName, (int)sizeof("__builtin_IB_ftobf_") - 1, &FP_RM, &VecLen, nullptr))
      return false;

    iid = GenISAIntrinsic::GenISA_ftobf;
    args[0] = CI.getArgOperand(0);            // value to be converted
    args[1] = ConstantInt::get(intTy, FP_RM); // rounding mode
    argslen = 2;
  } else if (funcName.startswith("__builtin_IB_bftof_")) {
    // It is a precise conversion, no RM needed!
    // Note that sizeof() includes the ending '\0', so need to do -1!
    if (!demangleFCvtSuffix(funcName, (int)sizeof("__builtin_IB_bftof_") - 1, nullptr, &VecLen, nullptr))
      return false;

    iid = GenISAIntrinsic::GenISA_bftof;
    args[0] = CI.getArgOperand(0);
    argslen = 1;
  } else if (funcName.startswith("__builtin_IB_2fto2bf_")) {
    if (!demangleFCvtSuffix(funcName, (int)sizeof("__builtin_IB_2fto2bf_") - 1, &FP_RM, &VecLen, nullptr))
      return false;

    iid = GenISAIntrinsic::GenISA_2fto2bf;
    args[0] = CI.getArgOperand(0);            // value to be converted
    args[1] = CI.getArgOperand(1);            // value to be converted
    args[2] = ConstantInt::get(intTy, FP_RM); // rounding mode
    argslen = 3;
  } else if (funcName.startswith("__builtin_IB_hftobf8_")) {
    int sz = (int)sizeof("__builtin_IB_hftobf8_");
    if (!demangleFCvtSuffix(funcName, sz - 1, nullptr, &VecLen, &isSat))
      return false;

    iid = GenISAIntrinsic::GenISA_hftobf8;
    args[0] = CI.getArgOperand(0);             // value to be converted
    args[1] = ConstantInt::get(intTy, FP_RM);  // rounding mode
    args[2] = ConstantInt::get(boolTy, isSat); // saturation
    argslen = 3;
  } else if (funcName.startswith("__builtin_IB_bf8tohf_")) {
    int sz = (int)sizeof("__builtin_IB_bf8tohf_");
    // It is a precise conversion, no RM needed!
    // Note that sizeof() includes the ending '\0', so need to do -1!
    if (!demangleFCvtSuffix(funcName, sz - 1, nullptr, &VecLen, nullptr))
      return false;

    iid = GenISAIntrinsic::GenISA_bf8tohf;
    args[0] = CI.getArgOperand(0);
    argslen = 1;
  } else if (funcName.startswith("__builtin_IB_hftohf8_")) {
    int sz = (int)sizeof("__builtin_IB_hftohf8_");
    if (!demangleFCvtSuffix(funcName, sz - 1, nullptr, &VecLen, &isSat))
      return false;

    iid = GenISAIntrinsic::GenISA_hftohf8;
    args[0] = CI.getArgOperand(0);             // value to be converted
    args[1] = ConstantInt::get(intTy, FP_RM);  // rounding mode
    args[2] = ConstantInt::get(boolTy, isSat); // saturation
    argslen = 3;
  } else if (funcName.startswith("__builtin_IB_hf8tohf_")) {
    int sz = (int)sizeof("__builtin_IB_hf8tohf_");
    // It is a precise conversion, no RM needed!
    // Note that sizeof() includes the ending '\0', so need to do -1!
    if (!demangleFCvtSuffix(funcName, sz - 1, nullptr, &VecLen, nullptr))
      return false;

    iid = GenISAIntrinsic::GenISA_hf8tohf;
    args[0] = CI.getArgOperand(0);
    argslen = 1;
  } else if (funcName.startswith("__builtin_IB_ftotf32_")) {
    if (!demangleFCvtSuffix(funcName, (int)sizeof("__builtin_IB_ftotf32_") - 1, nullptr, &VecLen, nullptr))
      return false;

    iid = GenISAIntrinsic::GenISA_ftotf32;
    args[0] = CI.getArgOperand(0);            // value to be converted
    args[1] = ConstantInt::get(intTy, FP_RM); // rounding mode
    argslen = 2;
  } else {
    return false;
  }

  // Sanity check
  if (!m_pCtx->platform.supportDpasInstruction()) {
    m_ErrorMsg = "bf conversion instruction not supported!";
    IGC_ASSERT_MESSAGE(0, "bf conversion instruction not supported!");
    return true;
  }
  Type *Ty = CI.getType();
  FixedVectorType *VTy = dyn_cast<FixedVectorType>(Ty);
  Type *ETy = VTy ? VTy->getElementType() : Ty;
  Type *Opnd0Ty = CI.getArgOperand(0)->getType();
  FixedVectorType *VOpnd0Ty = dyn_cast<FixedVectorType>(Opnd0Ty);
  Type *EOpnd0Ty = VOpnd0Ty ? VOpnd0Ty->getElementType() : Opnd0Ty;
  uint32_t n = VTy ? (uint32_t)VTy->getNumElements() : 1;
  uint32_t n0 = VOpnd0Ty ? (uint32_t)VOpnd0Ty->getNumElements() : 1;
  switch (iid) {
  case GenISAIntrinsic::GenISA_ftobf:
  case GenISAIntrinsic::GenISA_2fto2bf:
  case GenISAIntrinsic::GenISA_bftof: {
    if ((n != n0 || n != VecLen) ||
        (iid == GenISAIntrinsic::GenISA_ftobf && !(EOpnd0Ty->isFloatTy() && ETy->isIntegerTy(16))) ||
        (iid == GenISAIntrinsic::GenISA_2fto2bf && !(EOpnd0Ty->isFloatTy() && ETy->isIntegerTy(32))) ||
        (iid == GenISAIntrinsic::GenISA_bftof && !(EOpnd0Ty->isIntegerTy(16) && ETy->isFloatTy()))) {
      m_ErrorMsg = "Wrong argument types in bf conversion functions!";
      IGC_ASSERT_MESSAGE(0, "Wrong argument types in bf conversion functions!");
      return true;
    }
    break;
  }
  case GenISAIntrinsic::GenISA_hftobf8:
  case GenISAIntrinsic::GenISA_bf8tohf: {
    if ((n != n0 || n != VecLen) ||
        (iid == GenISAIntrinsic::GenISA_hftobf8 && !(EOpnd0Ty->isHalfTy() && ETy->isIntegerTy(8))) ||
        (iid == GenISAIntrinsic::GenISA_bf8tohf && !(EOpnd0Ty->isIntegerTy(8) && ETy->isHalfTy()))) {
      m_ErrorMsg = "Wrong argument types in bf8 conversion functions!";
      IGC_ASSERT_MESSAGE(0, "Wrong argument types in bf8 conversion functions!");
      return true;
    }
    break;
  }
  case GenISAIntrinsic::GenISA_ftotf32: {
    if ((n != n0 || n != VecLen) ||
        (iid == GenISAIntrinsic::GenISA_ftotf32 && !(EOpnd0Ty->isFloatTy() && ETy->isFloatTy()))) {
      m_ErrorMsg = "Wrong argument types in tf32 conversion functions!";
      IGC_ASSERT_MESSAGE(0, "Wrong argument types in tf32 conversion functions!");
      return true;
    }
    break;
  }
  default:
    break;
  }

  ArrayRef<Value *> ii_args(args, argslen);

  // Only need to specify retType and 1st arg's type.
  Type *ITys[2] = {func->getReturnType(), args[0]->getType()};
  Function *cvtFunc = GenISAIntrinsic::getDeclaration(func->getParent(), iid, ITys);
  const char *cvt = "bf_cvt";
  if (iid == GenISAIntrinsic::GenISA_hftobf8 || iid == GenISAIntrinsic::GenISA_bf8tohf) {
    cvt = "bf8_cvt";
  } else if (iid == GenISAIntrinsic::GenISA_hftohf8 || iid == GenISAIntrinsic::GenISA_hf8tohf) {
    cvt = "hf8_cvt";
  } else if (iid == GenISAIntrinsic::GenISA_ftotf32) {
    cvt = "tf32_cvt";
  }
  Instruction *cvtCall = CallInst::Create(cvtFunc, ii_args, cvt, &CI);

  updateDebugLoc(&CI, cvtCall);

  CI.replaceAllUsesWith(cvtCall);
  CI.eraseFromParent();

  m_changed = true;
  return true;
}

bool DpasFuncsResolution::processSrnd(CallInst &CI) {
  Function *func = CI.getCalledFunction();
  if (!func)
    return false;

  StringRef funcName = func->getName();

  int VecLen;
  bool isSat = false;
  GenISAIntrinsic::ID iid;
  if (funcName.consume_front("__builtin_IB_srnd_ftohf_")) {
    if (!demangleFCvtSuffix(funcName, 0, nullptr, &VecLen, nullptr))
      return false;
    iid = GenISAIntrinsic::GenISA_srnd_ftohf;
  } else if (funcName.consume_front("__builtin_IB_srnd_hftobf8_")) {
    if (!demangleFCvtSuffix(funcName, 0, nullptr, &VecLen, &isSat))
      return false;
    iid = GenISAIntrinsic::GenISA_srnd_hftobf8;
  } else if (funcName.consume_front("__builtin_IB_srnd_hftohf8_")) {
    if (!demangleFCvtSuffix(funcName, 0, nullptr, &VecLen, &isSat))
      return false;
    iid = GenISAIntrinsic::GenISA_srnd_hftohf8;
  } else if (funcName.consume_front("__builtin_IB_srnd_bftobf8_")) {
    if (!demangleFCvtSuffix(funcName, 0, nullptr, &VecLen, &isSat))
      return false;
    iid = GenISAIntrinsic::GenISA_srnd_bftobf8;
  } else if (funcName.consume_front("__builtin_IB_srnd_bftohf8_")) {
    if (!demangleFCvtSuffix(funcName, 0, nullptr, &VecLen, &isSat))
      return false;
    iid = GenISAIntrinsic::GenISA_srnd_bftohf8;
  } else {
    return false;
  }

  Type *boolTy = Type::getInt1Ty(CI.getContext());
  Value *args[3] = {CI.getArgOperand(0), CI.getArgOperand(1), ConstantInt::get(boolTy, isSat)};
  ArrayRef<Value *> ii_args(args, 3);

  Type *ITys[4] = {func->getReturnType(), args[0]->getType(), args[1]->getType(), boolTy};
  Function *srndFunc = GenISAIntrinsic::getDeclaration(func->getParent(), iid, ITys);
  Instruction *srndCall = CallInst::Create(srndFunc, ii_args, VALUE_NAME("srnd"), &CI);

#if defined(_DEBUG)
  { // Verify arguments
    Type *Ty = CI.getType();
    FixedVectorType *VTy = dyn_cast<FixedVectorType>(Ty);
    Type *ETy = VTy ? VTy->getElementType() : Ty;
    Type *Opnd0Ty = CI.getArgOperand(0)->getType();
    Type *Opnd1Ty = CI.getArgOperand(1)->getType();
    FixedVectorType *VOpnd1Ty = dyn_cast<FixedVectorType>(Opnd1Ty);
    Type *EOpnd1Ty = VOpnd1Ty ? VOpnd1Ty->getElementType() : Opnd1Ty;
    FixedVectorType *VOpnd0Ty = dyn_cast<FixedVectorType>(Opnd0Ty);
    Type *EOpnd0Ty = VOpnd0Ty ? VOpnd0Ty->getElementType() : Opnd0Ty;
    uint32_t n = VTy ? (uint32_t)VTy->getNumElements() : 1;
    uint32_t n0 = VOpnd0Ty ? (uint32_t)VOpnd0Ty->getNumElements() : 1;

    bool supported = false;
    supported |= (ETy->isHalfTy() && EOpnd0Ty->isFloatTy() && EOpnd1Ty->isIntegerTy(16));
    supported |= (ETy->isIntegerTy(8) && EOpnd0Ty->isHalfTy() && EOpnd1Ty->isIntegerTy(8));
    supported |= (ETy->isIntegerTy(8) && EOpnd0Ty->isIntegerTy(16) && EOpnd1Ty->isIntegerTy(8));

    if (n != n0 || n != VecLen || !supported) {
      m_ErrorMsg = "Wrong argument types in srnd builtin!";
      IGC_ASSERT_MESSAGE(0, "Wrong argument types in srnd builtin!");
      return true;
    }
  }
#endif
  updateDebugLoc(&CI, srndCall);
  CI.replaceAllUsesWith(srndCall);
  CI.eraseFromParent();

  m_changed = true;
  return true;
}

bool DpasFuncsResolution::processLfsr(CallInst &CI) {
  Function *func = CI.getCalledFunction();
  if (!func)
    return false;

  StringRef funcName = func->getName();
  if (!funcName.consume_front("__builtin_IB_lfsr_"))
    return false;

  int operationMode = 0;
  if (funcName.consume_front("b32")) {
    operationMode = 0; // b32 - single 32-bit seed/polynomial
  } else if (funcName.consume_front("b16v2")) {
    operationMode = 1; // b16v2 - two 16-bit seeds/polynomials packed in 32 bits
  } else if (funcName.consume_front("b8v4")) {
    operationMode = 2; // b8v4 - four 8-bit seeds/polynomials packed in 32 bits
  } else {
    IGC_ASSERT_MESSAGE(false, "Unexpected __builtin_IB_lfsr_ mode postfix");
    return false;
  }

  GenISAIntrinsic::ID iid = GenISAIntrinsic::GenISA_lfsr;
  Type *int32Ty = Type::getInt32Ty(CI.getContext());
  Value *args[3] = {CI.getArgOperand(0), CI.getArgOperand(1), ConstantInt::get(int32Ty, operationMode)};

  Type *ITys[4] = {func->getReturnType(), args[0]->getType(), args[1]->getType(), int32Ty};
  Function *lfsrFunc = GenISAIntrinsic::getDeclaration(func->getParent(), iid, ITys);
  Instruction *lfsrCall = CallInst::Create(lfsrFunc, args, VALUE_NAME("lfsr"), &CI);

  updateDebugLoc(&CI, lfsrCall);
  CI.replaceAllUsesWith(lfsrCall);
  CI.eraseFromParent();

  m_changed = true;
  return true;
}

bool DpasFuncsResolution::processDnscl(CallInst &CI) {
  Function *func = CI.getCalledFunction();
  if (!func)
    return false;

  StringRef funcName = func->getName();
  if (!funcName.consume_front("__builtin_IB_dnscl_"))
    return false;

  bool isHalf = false;
  if (funcName.consume_front("hf16")) {
    isHalf = true;
  } else if (funcName.consume_front("bf16")) {
    isHalf = false;
  } else {
    IGC_ASSERT_MESSAGE(false, "Unknown dnscl builtin type");
    return false;
  }

  bool stochastic = funcName.consume_front("_srnd");
  if (funcName.size() > 0) {
    IGC_ASSERT_MESSAGE(false, "Unknown postfix in dnscl builtin");
  }

  int argIndexBias = 2;
  int argIndexConvertToType = argIndexBias + (stochastic ? 1 : 0);
  int argIndexPackingMode = argIndexConvertToType + 1;

  unsigned convertToType = (unsigned)cast<ConstantInt>(CI.getArgOperand(argIndexConvertToType))->getZExtValue();
  unsigned fullConversionType = (isHalf ? 3 : 0) + convertToType; // convert into visa DNSCL_CONVERT_TYPE enum
  unsigned packingMode = (unsigned)cast<ConstantInt>(CI.getArgOperand(argIndexPackingMode))->getZExtValue();

  GenISAIntrinsic::ID iid = GenISAIntrinsic::GenISA_dnscl;
  Type *int32Ty = Type::getInt32Ty(CI.getContext());
  Value *args[6] = {
      CI.getArgOperand(0),
      CI.getArgOperand(1),                                                          // src0, src1
      (stochastic ? CI.getArgOperand(argIndexBias) : ConstantInt::get(int32Ty, 0)), // bias
      ConstantInt::get(int32Ty, fullConversionType),                                // conversion type
      ConstantInt::get(int32Ty, packingMode),                                       // packing mode
      ConstantInt::get(int32Ty, (stochastic ? 0 : 1))                               // rounding mode
  };

  Function *dnsclFunc = GenISAIntrinsic::getDeclaration(func->getParent(), iid);
  Instruction *dnsclCall = CallInst::Create(dnsclFunc, args, VALUE_NAME("dnscl"), &CI);

  updateDebugLoc(&CI, dnsclCall);
  CI.replaceAllUsesWith(dnsclCall);
  CI.eraseFromParent();

  m_changed = true;
  return true;
}


bool DpasFuncsResolution::processBdpas(CallInst &CI) {
  Function *Func = CI.getCalledFunction();
  if (!Func)
    return false;

  StringRef FuncName = Func->getName();
  LLVMContext &Ctx = CI.getContext();
  Type *IntTy = Type::getInt32Ty(Ctx);

  int DstTy, AccTy, PA, PB, SD, RC;
  if (!m_pCtx->platform.hasExecSize16DPAS() || !FuncName.startswith(DpasFuncsResolution::SG_PREFIX_BDPAS16)) {
    return false;
  }

  const int SG_PREFIX_LEN = DpasFuncsResolution::SG_PREFIX_BDPAS16.size();
  if (!demangleSuffix(FuncName, SG_PREFIX_LEN, true, false /*don't care */, DstTy, AccTy, PA, PB, SD, RC, nullptr)) {
    return false;
  }

#if defined(_DEBUG)
  // Additional intrinsic checks
  Value *ACC = CI.getArgOperand(0);
  Value *A = CI.getArgOperand(1);
  Value *B = CI.getArgOperand(2);
  Value *ScaleA = CI.getArgOperand(3);
  Value *ScaleB = CI.getArgOperand(4);

  Type *DTy = CI.getType();
  Type *ACCTy = ACC->getType();
  Type *ATy = A->getType();
  Type *BTy = B->getType();
  Type *ScaleATy = ScaleA->getType();
  Type *ScaleBTy = ScaleB->getType();
  int D_nelts = DTy->isVectorTy() ? (int)cast<FixedVectorType>(DTy)->getNumElements() : 1;
  int ACC_nelts = ACCTy->isVectorTy() ? (int)cast<FixedVectorType>(ACCTy)->getNumElements() : 1;
  int A_nelts = ATy->isVectorTy() ? (int)cast<FixedVectorType>(ATy)->getNumElements() : 1;
  int B_nelts = BTy->isVectorTy() ? (int)cast<FixedVectorType>(BTy)->getNumElements() : 1;
  int ScaleA_nelts = ScaleATy->isVectorTy() ? (int)cast<FixedVectorType>(ScaleATy)->getNumElements() : 1;
  int ScaleB_nelts = ScaleBTy->isVectorTy() ? (int)cast<FixedVectorType>(ScaleBTy)->getNumElements() : 1;
  Type *D_BaseTy = DTy->getScalarType();
  Type *Acc_BaseTy = ACCTy->getScalarType();
  Type *A_BaseTy = ATy->getScalarType();
  Type *B_BaseTy = BTy->getScalarType();

  IGC_ASSERT_MESSAGE(RC == 8, "ICE: bdpas repeat count must be 8!");
  IGC_ASSERT_MESSAGE(SD == 8, "ICE: bdpas systolic depth must be 8!");
  IGC_ASSERT_MESSAGE(D_nelts == 8, "ICE: bdpas dst vector size must be 8!");
  IGC_ASSERT_MESSAGE(ACC_nelts == 8, "ICE: bdpas acc vector size must be 8!");
  IGC_ASSERT_MESSAGE(A_nelts == 8, "ICE: bdpas A vector size must be 8!");
  IGC_ASSERT_MESSAGE(B_nelts == 8, "ICE: bdpas B vector size must be 8!");
  IGC_ASSERT_MESSAGE(A_BaseTy->isIntegerTy(16), "ICE: bdpas A shall have base type short!");
  IGC_ASSERT_MESSAGE(B_BaseTy->isIntegerTy(32), "ICE: bdpas B shall have base type int!");
  IGC_ASSERT_MESSAGE(ScaleATy->getScalarType()->isIntegerTy(8), "ICE: bdpas scale A shall have base type uchar!");
  IGC_ASSERT_MESSAGE(ScaleBTy->getScalarType()->isIntegerTy(8), "ICE: bdpas scale B shall have base type uchar!");

  if (PA == PrecisionType::FP16) {
    IGC_ASSERT_MESSAGE(PA == PB, "ICE: bdpas's A and B must have the same type!");
    IGC_ASSERT_MESSAGE(DstTy == DSTACC_FLOAT || DstTy == DSTACC_FP16, "ICE: wrong type of dst for FP16 bdpas!");
    IGC_ASSERT_MESSAGE(AccTy == DSTACC_FLOAT || AccTy == DSTACC_FP16, "ICE: wrong type of acc for FP16 bdpas!");
    IGC_ASSERT_MESSAGE(D_BaseTy->isFloatTy() || D_BaseTy->isHalfTy(), "ICE: wrong type of dst for FP16 bdpas!");
    IGC_ASSERT_MESSAGE(Acc_BaseTy->isFloatTy() || Acc_BaseTy->isHalfTy(), "ICE: wrong type of acc for FP16 bdpas!");
    IGC_ASSERT_MESSAGE(!ScaleATy->isVectorTy() || !ScaleBTy->isVectorTy(),
                       "ICE: scales have to be scalars for FP16 bdpas!");
  } else if (PA == PrecisionType::BF16) {
    IGC_ASSERT_MESSAGE(PA == PB, "ICE: bdpas's A and B must have the same type!");
    IGC_ASSERT_MESSAGE(DstTy == DSTACC_FLOAT || DstTy == DSTACC_BF16, "ICE: wrong type of dst for BF16 bdpas!");
    IGC_ASSERT_MESSAGE(AccTy == DSTACC_FLOAT || AccTy == DSTACC_BF16, "ICE: wrong type of acc for BF16 bdpas!");
    IGC_ASSERT_MESSAGE(D_BaseTy->isFloatTy() || D_BaseTy->isIntegerTy(16), "ICE: wrong type of dst for BF16 bdpas!");
    IGC_ASSERT_MESSAGE(Acc_BaseTy->isFloatTy() || Acc_BaseTy->isIntegerTy(16),
                       "ICE: wrong type of acc for BF16 bdpas!");
    IGC_ASSERT_MESSAGE(!ScaleATy->isVectorTy() || !ScaleBTy->isVectorTy(),
                       "ICE: scales have to be scalars for BF16 bdpas!");
  } else if (PA == PrecisionType::BF8 || PA == PrecisionType::HF8) {
    IGC_ASSERT_MESSAGE(PB == PrecisionType::BF8 || PB == PrecisionType::HF8,
                       "ICE: bdpas's A and B must be both BF8/HF8 type!");
    IGC_ASSERT_MESSAGE(DstTy == DSTACC_FLOAT || DstTy == DSTACC_BF16, "ICE: wrong type of dst for BF8/HF8 bdpas!");
    IGC_ASSERT_MESSAGE(AccTy == DSTACC_FLOAT || AccTy == DSTACC_BF16, "ICE: wrong type of acc for BF8/HF8 bdpas!");
    IGC_ASSERT_MESSAGE(D_BaseTy->isFloatTy() || D_BaseTy->isIntegerTy(16), "ICE: wrong type of dst for BF8/HF8 bdpas!");
    IGC_ASSERT_MESSAGE(Acc_BaseTy->isFloatTy() || Acc_BaseTy->isIntegerTy(16),
                       "ICE: wrong type of acc for BF8/HF8 bdpas!");
    IGC_ASSERT_MESSAGE(!ScaleATy->isVectorTy() || !ScaleBTy->isVectorTy(),
                       "ICE: scales have to be scalars for BF8/HF8 bdpas!");
  } else if (PA == PrecisionType::E2M1) {
    IGC_ASSERT_MESSAGE(PB == PrecisionType::E2M1, "ICE: bdpas's A and B must be both E2M1 type!");
    IGC_ASSERT_MESSAGE(DstTy == DSTACC_FLOAT || DstTy == DSTACC_BF16, "ICE: wrong type of dst for E2M1 bdpas!");
    IGC_ASSERT_MESSAGE(AccTy == DSTACC_FLOAT || AccTy == DSTACC_BF16, "ICE: wrong type of acc for E2M1 bdpas!");
    IGC_ASSERT_MESSAGE(D_BaseTy->isFloatTy() || D_BaseTy->isIntegerTy(16), "ICE: wrong type of dst for E2M1 bdpas!");
    IGC_ASSERT_MESSAGE(Acc_BaseTy->isFloatTy() || Acc_BaseTy->isIntegerTy(16),
                       "ICE: wrong type of acc for E2M1 bdpas!");
    IGC_ASSERT_MESSAGE(ScaleA_nelts == 2 && ScaleB_nelts == 2,
                       "ICE: scales have to be 2 element vectors for E2M1 bdpas!");
  }
#endif // _DEBUG

  Value *Args[7] = {CI.getArgOperand(0), // Acc
                    CI.getArgOperand(1), // A
                    CI.getArgOperand(2), // B
                    CI.getArgOperand(3), // ScaleA
                    CI.getArgOperand(4), // ScaleB
                    ConstantInt::get(IntTy, PA), ConstantInt::get(IntTy, PB)};

  // ITys: overload types for this intrinsic
  Type *ITys[6] = {Func->getReturnType(), Args[0]->getType(), Args[1]->getType(),
                   Args[2]->getType(),    Args[3]->getType(), Args[4]->getType()};
  GenISAIntrinsic::ID IID = GenISAIntrinsic::GenISA_sub_group_bdpas;
  Function *BdpasFunc = GenISAIntrinsic::getDeclaration(Func->getParent(), IID, ITys);

  Instruction *BdpasCall = CallInst::Create(BdpasFunc, Args, VALUE_NAME("bdpas"), &CI);

  updateDebugLoc(&CI, BdpasCall);

  CI.replaceAllUsesWith(BdpasCall);
  CI.eraseFromParent();

  m_changed = true;
  return true;
}

//
// FN pattern:
//    [w]_<dstty>_<accty>_<a's precision>_<b's precision>_<depth>_<rcount>
//      <a's precision>
//      <b's precision>
//          1. float version:   <bf|hf>_
//          2. integer version: <u|s><2|4|8>_
//      dstty/accty:
//          1. float version:   f
//          2. integer version: int32
// If [w] is present, it is dpasw.
//
// PVC supports:
//      additional dstty/accty:  bf|hf
//      additional precision : tf32
//
bool DpasFuncsResolution::demangleSuffix(StringRef FN, int StartPos, bool HasDstAcc, bool IsIDpas, int &DstTy,
                                         int &AccTy, int &PA, int &PB, int &SD, int &RC, bool *IsDpasw) {
  size_t sz = FN.size();
  size_t rem = sz - StartPos;
  size_t i = StartPos;

  // Check if it is wide version of dpas
  if (IsDpasw != nullptr) {
    *IsDpasw = parseW(FN, i, rem);
  }

  if (HasDstAcc) {
    DstTy = parseDstAccType(FN, i, rem);
    AccTy = parseDstAccType(FN, i, rem);
  } else {
    DstTy = IsIDpas ? DstAccType::DSTACC_INT32 : DstAccType::DSTACC_FLOAT;
    AccTy = DstTy;
  }

  bool supportDeprecated = true;
  if (!IsIDpas && !HasDstAcc && supportDeprecated && rem == 4) {
    // deprecated format _8_<1-8>
    PA = PrecisionType::BF16;
    PB = PA;
  } else {
    // parse precisions
    PA = parsePrecision(FN, i, rem);
    PB = parsePrecision(FN, i, rem);
  }

  // depth and repeat count
  SD = parseDepth(FN, i, rem);
  RC = parseRCount(FN, i, rem);

  if (RC == -1) {
    return false;
  }
  return true;
}

bool DpasFuncsResolution::demangleFCvtSuffix(StringRef FN, int StartPos, int *pRM, int *pVecLen, bool *pIsSat) {
  int sz = (int)FN.size();
  int rem = sz - StartPos;
  int RM = ROUND_TO_NEAREST_EVEN;
  int VecLen = 1;
  bool isSat = false;

  int i = StartPos;
  if (rem >= 5 && pRM != nullptr) {
    // if it is a valid intrinsic, it must be <rm>_<1|2|4|8|16>[_sat]
    // <rm> is rte|rtp|rtn|rtz.
    if (FN[i] != 'r' || FN[i + 1] != 't' || FN[i + 3] != '_') {
      return false;
    }
    switch (FN[i + 2]) {
    default:
      return false;
    case 'e':
      RM = ROUND_TO_NEAREST_EVEN;
      break;
    case 'p':
      RM = ROUND_TO_POSITIVE;
      break;
    case 'n':
      RM = ROUND_TO_NEGATIVE;
      break;
    case 'z':
      RM = ROUND_TO_ZERO;
      break;
    }

    i += 4;
    rem -= 4;
  }

  int c = (FN[i] - '0');
  int c1 = (rem >= 2 ? (FN[i + 1] - '0') : 0);

  // relax vector size to be 1-16 here.
  if (rem >= 2 && c == 1 && c1 >= 0 && c1 <= 6) {
    VecLen = 10 + c1;
    i += 2;
    rem -= 2;
  } else if (rem >= 1 && c >= 0 && c <= 9) {
    VecLen = c;
    i += 1;
    rem -= 1;
  } else {
    // missing veclen
    return false;
  }

  // saturation
  if (pIsSat) {
    if (rem >= 1 && FN[i] == '_') {
      ++i;
      --rem;
    }

    if (rem == 3 && FN[i] == 's' && FN[i + 1] == 'a' && FN[i + 2] == 't') {
      i += 3;
      rem -= 3;
      isSat = true;
    }
  }

  if (rem != 0) {
    return false;
  }

  if (pRM) {
    *pRM = RM;
  }
  *pVecLen = VecLen;
  if (pIsSat) {
    *pIsSat = isSat;
  }
  return true;
}

DstAccType DpasFuncsResolution::parseDstAccType(StringRef StrRef, size_t &StrPos, size_t &StrRem) {
  DstAccType ty = DSTACC_UNUSED;
  if (StrPos != StringRef::npos && StrRem >= 2) {
    char c0 = StrRef[StrPos];
    char c1 = StrRef[StrPos + 1];
    char c2 = StrRem >= 3 ? StrRef[StrPos + 2] : 0;
      if (c0 == '_' && c1 == 'd') { // "_d"
        ty = DSTACC_INT32;
        StrPos += 2;
        StrRem -= 2;
      } else if (c0 == '_' && c1 == 'f') { // "_f"
        ty = DSTACC_FLOAT;
        StrPos += 2;
        StrRem -= 2;
      } else if (c0 == '_' && (c1 == 'b' || c1 == 'h') && c2 == 'f') { // "_bf" or "_hf"
        ty = (c1 == 'b' ? DSTACC_BF16 : DSTACC_FP16);
        StrPos += 3;
        StrRem -= 3;
      }
  }
  if (ty == DSTACC_UNUSED) {
    // Not valid type
    StrPos = StringRef::npos;
    StrRem = 0;
  }
  return ty;
}

bool DpasFuncsResolution::parseW(StringRef StrRef, size_t &StrPos, size_t &StrRem) {
  if (StrPos != StringRef::npos && StrRem >= 1) {
    char c0 = StrRef[StrPos];
    if (c0 == 'w') {
      StrPos += 1;
      StrRem -= 1;
      return true;
    }
  }
  return false;
}

PrecisionType DpasFuncsResolution::parsePrecision(StringRef StrRef, size_t &StrPos, size_t &StrRem) {
  PrecisionType ty = PrecisionType::PRECISION_UNUSED;
  if (StrPos != StringRef::npos && StrRem >= 3) {
    char c0 = StrRef[StrPos];
    char c1 = StrRef[StrPos + 1];
    char c2 = StrRef[StrPos + 2];
    char c3 = StrRem >= 4 ? StrRef[StrPos + 3] : 0;
    char c4 = StrRem >= 5 ? StrRef[StrPos + 4] : 0;
    if (c0 == '_' && c1 == 'b' && c2 == 'f' && c3 == '8') { // "_bf8"
      ty = PrecisionType::BF8;
      StrPos += 4;
      StrRem -= 4;
    } else if (c0 == '_' && c1 == 'h' && c2 == 'f' && c3 == '8') { // "_bf8"
      ty = PrecisionType::HF8;
      StrPos += 4;
      StrRem -= 4;
    } else if (c0 == '_' && c1 == 't' && c2 == 'f' && c3 == '3' && c4 == '2') { // "_tf32"
      ty = PrecisionType::TF32;
      StrPos += 5;
      StrRem -= 5;
    } else if (c0 == '_' && c1 == 'b' && c2 == 'f') { // "_bf"
      ty = PrecisionType::BF16;
      StrPos += 3;
      StrRem -= 3;
    } else if (c0 == '_' && c1 == 'h' && c2 == 'f') { // "_hf"
      ty = PrecisionType::FP16;
      StrPos += 3;
      StrRem -= 3;
    } else if (c0 == '_' && c1 == 'u' && (c2 == '2' || c2 == '4' || c2 == '8')) { // "_u<2|4|8>"
      ty = (c2 == '2' ? PrecisionType::U2 : (c2 == '4' ? PrecisionType::U4 : PrecisionType::U8));
      StrPos += 3;
      StrRem -= 3;
    } else if (c0 == '_' && c1 == 's' && (c2 == '2' || c2 == '4' || c2 == '8')) { // "s<2|4|8>_"
      ty = (c2 == '2' ? PrecisionType::S2 : (c2 == '4' ? PrecisionType::S4 : PrecisionType::S8));
      StrPos += 3;
      StrRem -= 3;
    } else if (c0 == '_' && c1 == 'd' && c2 == 'f') { // "_df"
      ty = PrecisionType::DF;
      StrPos += 3;
      StrRem -= 3;
    } else if (c0 == '_' && c1 == 'e' && c2 == '2' && c3 == 'm' && c4 == '1') { // "_e2m1"
      ty = PrecisionType::E2M1;
      StrPos += 5;
      StrRem -= 5;
    }
  }
  if (ty == PRECISION_UNUSED) {
    // Not a valid precision
    StrPos = StringRef::npos;
    StrRem = 0;
  }
  return ty;
};

int DpasFuncsResolution::parseDepth(StringRef StrRef, size_t &StrPos, size_t &StrRem) {
  if (StrPos != StringRef::npos && StrRem >= 2) {
    char c0 = StrRef[StrPos];
    char c1 = StrRef[StrPos + 1];
    if (c0 == '_' && c1 == '8') {
      StrPos += 2;
      StrRem -= 2;
      return 8;
    }
    if (StrRem >= 3 && c0 == '_' && c1 == '1' && StrRef[StrPos + 2] == '6') {
      StrPos += 3;
      StrRem -= 3;
      return 16;
    }
  }
  StrPos = StringRef::npos;
  StrRem = 0;
  return -1;
}

int DpasFuncsResolution::parseRCount(StringRef StrRef, size_t &StrPos, size_t &StrRem) {
  if (StrPos != StringRef::npos && StrRem >= 2) {
    char c0 = StrRef[StrPos];
    char c1 = StrRef[StrPos + 1];
    int rc = c1 - '0';
    if (c0 == '_' && rc >= 1 && rc <= 8) {
      StrPos += 2;
      StrRem -= 2;
      return rc;
    }
  }
  StrPos = StringRef::npos;
  StrRem = 0;
  return -1;
}

FunctionPass *IGC::createDpasFuncsResolutionPass() { return new DpasFuncsResolution(); }
