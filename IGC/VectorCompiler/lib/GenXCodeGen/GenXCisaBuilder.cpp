/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXCisaBuilder
/// ---------------
///
/// This file contains to passes: GenXCisaBuilder and GenXFinalizer.
///
/// 1. GenXCisaBuilder transforms LLVM IR to CISA IR via Finalizer' public API.
///    It is a FunctionGroupWrapperPass, thus it runs once for each kernel and
///    creates CISA IR for it and all its subroutines. Real building of kernels
///    is performed by the GenXKernelBuilder class. This splitting is necessary
///    because GenXCisaBuilder object lives through all Function Groups, but we
///    don't need to keep all Kernel building specific data in such lifetime.
///
/// 2. GenXFinalizer is a module pass, thus it runs once and all that it does
///    is a running of Finalizer for kernels created in GenXCisaBuilder pass.
///
//===----------------------------------------------------------------------===//

#include "FunctionGroup.h"
#include "GenX.h"
#include "GenXDebugInfo.h"
#include "GenXGotoJoin.h"
#include "GenXIntrinsics.h"
#include "GenXPressureTracker.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"
#include "GenXVisaRegAlloc.h"

#include "vc/Support/BackendConfig.h"
#include "vc/Support/GenXDiagnostic.h"
#include "vc/Support/ShaderDump.h"
#include "vc/Utils/GenX/GlobalVariable.h"
#include "vc/Utils/GenX/Intrinsics.h"
#include "vc/Utils/GenX/IntrinsicsWrapper.h"
#include "vc/Utils/GenX/KernelInfo.h"
#include "vc/Utils/GenX/PredefinedVariable.h"
#include "vc/Utils/GenX/Printf.h"
#include "vc/Utils/GenX/RegCategory.h"
#include "vc/Utils/General/Types.h"

#include "vc/InternalIntrinsics/InternalIntrinsics.h"
#include "llvm/GenXIntrinsics/GenXIntrinsicInst.h"

#include "visaBuilder_interface.h"

#include "llvm/ADT/IndexedMap.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/ScopedPrinter.h"
#include "llvm/Support/StringSaver.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/CallSite.h"
#include "llvmWrapper/IR/Constants.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/InstrTypes.h"
#include "llvmWrapper/IR/Instructions.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace llvm;
using namespace genx;

#define DEBUG_TYPE "GENX_CISA_BUILDER"

static cl::list<std::string>
    FinalizerOpts("finalizer-opts", cl::Hidden, cl::ZeroOrMore,
                  cl::desc("Additional options for finalizer."));

static cl::opt<std::string> AsmNameOpt("asm-name", cl::init(""), cl::Hidden,
    cl::desc("Output assembly code to this file during compilation."));

static cl::opt<bool> ReverseKernels("reverse-kernels", cl::init(false), cl::Hidden,
    cl::desc("Emit the kernel asm name in reversed order (if user asm name presented)."));

static cl::opt<bool>
    PrintFinalizerOptions("cg-print-finalizer-args", cl::init(false), cl::Hidden,
                          cl::desc("Prints options used to invoke finalizer"));

static cl::opt<bool> SkipNoWiden("skip-widen", cl::init(false), cl::Hidden,
                                 cl::desc("Do new emit NoWiden hint"));

static cl::opt<bool> DisableNoMaskWA(
    "vc-cg-disable-no-mask-wa", cl::init(false), cl::Hidden,
    cl::desc("do not apply noMask WA (fusedEU)"));

static cl::opt<bool>
    OptDisableVisaLOC("vc-cg-disable-visa-loc", cl::init(false), cl::Hidden,
                      cl::desc("do not emit LOC and FILE instructions"));

static cl::opt<bool> OptStrictI64Check(
        "genx-cisa-builder-noi64-check", cl::init(false), cl::Hidden,
        cl::desc("strict check to ensure we produce no 64-bit operations"));

STATISTIC(NumVisaInsts, "Number of VISA instructions");
STATISTIC(NumAsmInsts, "Number of Gen asm instructions");
STATISTIC(SpillMemUsed, "Spill memory size used");
STATISTIC(NumFlagSpillStore, "Number of flag spills");
STATISTIC(NumFlagSpillLoad, "Number of flag fills");

/// For VISA_PREDICATE_CONTROL & VISA_PREDICATE_STATE
template <class T> T &operator^=(T &a, T b) {
  using _T = typename std::underlying_type<T>::type;
  static_assert(std::is_integral<_T>::value,
                "Wrong operation for non-integral type");
  a = static_cast<T>(static_cast<_T>(a) ^ static_cast<_T>(b));
  return a;
}

template <class T> T operator|=(T &a, T b) {
  using _T = typename std::underlying_type<T>::type;
  static_assert(std::is_integral<_T>::value,
                "Wrong operation for non-integral type");
  a = static_cast<T>(static_cast<_T>(a) | static_cast<_T>(b));
  return a;
}

struct DstOpndDesc {
  Instruction *WrRegion = nullptr;
  Instruction *GStore = nullptr;
  Instruction *WrPredefReg = nullptr;
  genx::BaleInfo WrRegionBI;
};

namespace {

static VISA_Exec_Size getExecSizeFromValue(unsigned int Size) {
  int Res = genx::log2(Size);
  IGC_ASSERT(std::bitset<sizeof(unsigned int) * 8>(Size).count() <= 1);
  IGC_ASSERT_MESSAGE(Res <= 5,
         "illegal common ISA execsize (should be 1, 2, 4, 8, 16, 32).");
  return Res == -1 ? EXEC_SIZE_ILLEGAL : (VISA_Exec_Size)Res;
}

static VISA_Oword_Num getCisaOwordNumFromNumber(unsigned num) {
  switch (num) {
  case 1:
    return OWORD_NUM_1;
  case 2:
    return OWORD_NUM_2;
  case 4:
    return OWORD_NUM_4;
  case 8:
    return OWORD_NUM_8;
  case 16:
    return OWORD_NUM_16;
  default:
    IGC_ASSERT_MESSAGE(0, "illegal Oword number.");
    return OWORD_NUM_ILLEGAL;
  }
}

VISAChannelMask convertChannelMaskToVisaType(unsigned Mask) {
  switch (Mask & 0xf) {
  case 1:
    return CHANNEL_MASK_R;
  case 2:
    return CHANNEL_MASK_G;
  case 3:
    return CHANNEL_MASK_RG;
  case 4:
    return CHANNEL_MASK_B;
  case 5:
    return CHANNEL_MASK_RB;
  case 6:
    return CHANNEL_MASK_GB;
  case 7:
    return CHANNEL_MASK_RGB;
  case 8:
    return CHANNEL_MASK_A;
  case 9:
    return CHANNEL_MASK_RA;
  case 10:
    return CHANNEL_MASK_GA;
  case 11:
    return CHANNEL_MASK_RGA;
  case 12:
    return CHANNEL_MASK_BA;
  case 13:
    return CHANNEL_MASK_RBA;
  case 14:
    return CHANNEL_MASK_GBA;
  case 15:
    return CHANNEL_MASK_RGBA;
  default:
      IGC_ASSERT_UNREACHABLE(); // Wrong mask
  }
}

CHANNEL_OUTPUT_FORMAT getChannelOutputFormat(uint8_t ChannelOutput) {
  return (CHANNEL_OUTPUT_FORMAT)((ChannelOutput >> 4) & 0x3);
}

static std::string cutString(const Twine &Str) {
  constexpr size_t MaxVisaLabelLength = 1023;
  auto Result = Str.str();
  if (Result.size() > MaxVisaLabelLength)
    Result.erase(MaxVisaLabelLength);
  return Result;
}

void handleCisaCallError(const Twine &Call, LLVMContext &Ctx) {
  vc::fatal(Ctx, "VISA builder API call failed", Call);
}

void handleInlineAsmParseError(const GenXBackendConfig &BC, StringRef VisaErr,
                               StringRef VisaText, LLVMContext &Ctx) {
  std::string ErrMsg;
  raw_string_ostream SS{ErrMsg};
  SS << "Failed to parse inline visa assembly\n";
  if (!VisaErr.empty())
    SS << VisaErr << '\n';
  if (BC.hasShaderDumper() && BC.asmDumpsEnabled()) {
    const char *DumpModuleName = "inline_asm_text.visaasm";
    SS << "Full module dumped as '" << DumpModuleName << "'\n";
    BC.getShaderDumper().dumpText(VisaText, DumpModuleName);
  } else {
    SS << "Enable dumps to see failed visa module\n";
  }

  vc::fatal(Ctx, "GenXCisaBuilder", SS.str().c_str());
}

/***********************************************************************
 * Local function for testing one assertion statement.
 * It returns true if all is ok.
 * A phi node not generates any code.
 * The phi node should has no live range because it is part of an indirected
 * arg/retval in GenXArgIndirection or it is an EM/RM category.
 */
bool testPhiNodeHasNoMismatchedRegs(const llvm::PHINode *const Phi,
  const llvm::GenXLiveness *const Liveness) {
  IGC_ASSERT(Phi);
  IGC_ASSERT(Liveness);
  bool Result = true;
  const size_t Count = Phi->getNumIncomingValues();
  for (size_t i = 0; (i < Count) && Result; ++i) {
    const llvm::Value *const Incoming = Phi->getIncomingValue(i);
    if (!isa<UndefValue>(Incoming)) {
      const genx::SimpleValue SVI(const_cast<llvm::Value *> (Incoming));
      const genx::LiveRange *const LRI = Liveness->getLiveRangeOrNull(SVI);
      if (LRI) {
        if (vc::isRealOrNoneCategory(LRI->getCategory())) {
          const genx::SimpleValue SVP(const_cast<llvm::PHINode *> (Phi));
          const genx::LiveRange *const LRP = Liveness->getLiveRangeOrNull(SVP);
          Result = (LRI == LRP);
          IGC_ASSERT_MESSAGE(Result, "mismatched registers in phi node");
        }
      }
    }
  }
  return Result;
}

/***********************************************************************
 * Local function for testing one assertion statement.
 */
bool testPredicate(const CmpInst *const Cmp, const DstOpndDesc &DstDesc) {
  bool Result = (!DstDesc.WrRegion);
  Result = (Result || (Cmp->getType()->getPrimitiveSizeInBits() != 4));
  Result = (Result || (Cmp->getOperand(0)->getType()->getScalarType()
    ->getPrimitiveSizeInBits() == 64));
  IGC_ASSERT(Result);
  return Result;
}

} // namespace

#define CISA_CALL_CTX(c, ctx)                                                  \
  do {                                                                         \
    auto result = c;                                                           \
    if (result != 0) {                                                         \
      handleCisaCallError(#c, (ctx));                                          \
    }                                                                          \
  } while (0);

#define CISA_CALL(c) CISA_CALL_CTX(c, getContext())

namespace llvm {

static VISA_Type getVisaTypeFromBytesNumber(unsigned BytesNum, bool IsFloat,
                                            genx::Signedness Sign) {
  VISA_Type aliasType;
  if (IsFloat) {
    switch (BytesNum) {
    case 2:
      aliasType = ISA_TYPE_HF;
      break;
    case 4:
      aliasType = ISA_TYPE_F;
      break;
    case 8:
      aliasType = ISA_TYPE_DF;
      break;
    default:
      report_fatal_error("unknown float type");
      break;
    }
  } else {
    switch (BytesNum) {
    case 1:
      aliasType = (Sign == SIGNED) ? ISA_TYPE_B : ISA_TYPE_UB;
      break;
    case 2:
      aliasType = (Sign == SIGNED) ? ISA_TYPE_W : ISA_TYPE_UW;
      break;
    case 4:
      aliasType = (Sign == SIGNED) ? ISA_TYPE_D : ISA_TYPE_UD;
      break;
    case 8:
      aliasType = (Sign == SIGNED) ? ISA_TYPE_Q : ISA_TYPE_UQ;
      break;
    default:
      report_fatal_error("unknown integer type");
      break;
    }
  }
  return aliasType;
}

static VISA_Type llvmToVisaType(Type *Type,
                                genx::Signedness Sign = DONTCARESIGNED) {
  auto T = Type;
  IGC_ASSERT(!T->isAggregateType());
  VISA_Type Result = ISA_TYPE_NUM;
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(T);
      VT && VT->getElementType()->isIntegerTy(1)) {
    IGC_ASSERT(VT->getNumElements() == 8 || VT->getNumElements() == 16 ||
               VT->getNumElements() == 32);
    Result = getVisaTypeFromBytesNumber(VT->getNumElements() / genx::ByteBits,
                                        false, Sign);
  } else {
    if (T->isVectorTy())
      T = cast<VectorType>(T)->getElementType();
    if (T->isPointerTy()) {
      // we might have used DL to get the type size but that'd
      // overcomplicate this function's type unnecessarily
      Result = getVisaTypeFromBytesNumber(visa::BytesPerSVMPtr, false,
                                          DONTCARESIGNED);
    } else {
      IGC_ASSERT(T->isFloatingPointTy() || T->isIntegerTy());
      Result = getVisaTypeFromBytesNumber(T->getScalarSizeInBits() / CHAR_BIT,
                                          T->isFloatingPointTy(), Sign);
    }
  }
  IGC_ASSERT(Result != ISA_TYPE_NUM);
  return Result;
}

static VISA_Type llvmToVisaType(Value *V,
                                genx::Signedness Sign = DONTCARESIGNED) {
  return llvmToVisaType(V->getType(), Sign);
}

// Due to the lack of access to VISA_GenVar internal interfaces (concerning type, size, etc)
// some local DS are required to store such info: CisaVariable and GenericCisaVariable.

//===----------------------------------------------------------------------===//
// CisaVariable
// ------------------
//
// CisaVariable keeps VISA_GenVar of a specific VISA_Type and provides accessors
// to its byte size and number of elements thus emulating some internal vISA machinery.
//
//===----------------------------------------------------------------------===//
class CisaVariable {
  VISA_Type Type;
  unsigned ByteSize = 0;
  VISA_GenVar *VisaVar = nullptr;

public:
  CisaVariable(VISA_Type T, unsigned BS, VISA_GenVar *V)
      : Type(T), ByteSize(BS), VisaVar(V) {}

  VISA_Type getType() const { return Type; }

  VISA_GenVar *getGenVar() { return VisaVar; }

  unsigned getByteSize() const { return ByteSize; }

  unsigned getNumElements() const {
    const int size = CISATypeTable[Type].typeSize;
    IGC_ASSERT(size);
    IGC_ASSERT(!(ByteSize % size));
    return ByteSize / size;
  }
};

//===----------------------------------------------------------------------===//
// GenericCisaVariable
// ------------------
//
// GenericCisaVariable describes vISA value that isn't intended to have matching llvm::Value
// (e.g. stack regs %arg and %retv). It provides interface to get a VisaVar alias with a specific
// vISA type.
//
//===----------------------------------------------------------------------===//
class GenericCisaVariable {
  const char *Name = "";
  VISA_GenVar *VisaVar = nullptr;
  unsigned ByteSize = 0;

  IndexedMap<CisaVariable *> AliasDecls;
  std::list<CisaVariable> Storage;

  unsigned getNumElements(VISA_Type T) const {
    const int size = CISATypeTable[T].typeSize;
    IGC_ASSERT(size);
    IGC_ASSERT(!(ByteSize % size));
    return ByteSize / size;
  }

public:
  GenericCisaVariable(const char *Nm, VISA_GenVar *V, unsigned BS)
      : Name(Nm), VisaVar(V), ByteSize(BS) {
    AliasDecls.grow(ISA_TYPE_NUM);
  }

  CisaVariable *getAlias(Value *V, VISAKernel *K) {
    return getAlias(llvmToVisaType(V), K);
  }

  CisaVariable *getAlias(VISA_Type T, VISAKernel *K) {
    if (!AliasDecls[T]) {
      VISA_GenVar *VV = nullptr;
      K->CreateVISAGenVar(VV, Name, getNumElements(T), T, ALIGN_GRF, VisaVar);
      Storage.push_back(CisaVariable(T, ByteSize, VV));
      AliasDecls[T] = &Storage.back();
    }
    return AliasDecls[T];
  }

  unsigned getByteSize() const { return ByteSize; }
};

//===----------------------------------------------------------------------===//
/// GenXCisaBuilder
/// ------------------
///
/// This class encapsulates a creation of vISA kernels.
/// It is a FunctionGroupWrapperPass, thus it runs once for each kernel and
/// builds vISA kernel via class GenXKernelBuilder.
/// All created kernels are stored in CISA Builder object which is provided
/// by finalizer.
///
//===----------------------------------------------------------------------===//
class GenXCisaBuilder : public FGPassImplInterface,
                        public IDMixin<GenXCisaBuilder> {
  LLVMContext *Ctx = nullptr;

public:
  explicit GenXCisaBuilder() {}

  static StringRef getPassName() { return "GenX CISA construction pass"; }
  static void getAnalysisUsage(AnalysisUsage &AU);
  bool runOnFunctionGroup(FunctionGroup &FG) override;

  LLVMContext &getContext() {
    IGC_ASSERT(Ctx);
    return *Ctx;
  }
};

void initializeGenXCisaBuilderWrapperPass(PassRegistry &);
using GenXCisaBuilderWrapper = FunctionGroupWrapperPass<GenXCisaBuilder>;

//===----------------------------------------------------------------------===//
/// GenXKernelBuilder
/// ------------------
///
/// This class does all the work for creation of vISA kernels.
///
//===----------------------------------------------------------------------===//
class GenXKernelBuilder {
  using Register = GenXVisaRegAlloc::Reg;

  VISAKernel *MainKernel = nullptr;
  VISAFunction *Kernel = nullptr;
  vc::KernelMetadata TheKernelMetadata;
  LLVMContext &Ctx;
  const DataLayout &DL;

  std::map<Function *, VISAFunction *> Func2Kern;

  std::map<std::string, unsigned> StringPool;
  std::vector<VISA_LabelOpnd *> Labels;
  std::map<const Value *, unsigned> LabelMap;

  // loop info for each function
  std::map<Function *, LoopInfoBase<BasicBlock, Loop> *> Loops;
  ValueMap<Function *, bool> IsInLoopCache;

  // whether kernel has barrier or sbarrier instruction
  bool HasBarrier = false;
  bool HasCallable = false;
  bool HasStackcalls = false;
  bool HasAlloca = false;
  bool HasSimdCF = false;
  bool UseNewStackBuilder = false;
  // GRF width in unit of byte
  unsigned GrfByteSize = defaultGRFByteSize;

  // Default stackcall execution size
  VISA_Exec_Size StackCallExecSize = EXEC_SIZE_16;

  // Line in code, filename and dir for emit loc/file in visa
  unsigned LastEmittedVisaLine = 0;
  StringRef LastFilename = "";
  StringRef LastDirectory = "";

  // function currently being written during constructor
  Function *Func = nullptr;
  // function corresponding to VISAKernel currently being written
  Function *KernFunc = nullptr;
  PreDefined_Surface StackSurf;

  std::map<Function *, VISA_GenVar *> FPMap;
  SmallVector<InsertValueInst *, 10> RetvInserts;

  std::map<VISAKernel *, std::map<StringRef, GenericCisaVariable>> CisaVars;

  // The default float control from kernel attribute. Each subroutine may
  // overrride this control mask, but it should revert back to the default float
  // control mask before exiting from the subroutine.
  uint32_t DefaultFloatControl = 0;

  enum CRBits {
    SinglePrecisionMode = 1,
    RoundingMode = 3 << 4,
    DoublePrecisionDenorm = 1 << 6,
    SinglePrecisionDenorm = 1 << 7,
    HalfPrecisionDenorm = 1 << 10,
  };

  uint32_t CRMask = 0;

  // normally false, set to true if there is any SIMD CF in the func or this is
  // (indirectly) called inside any SIMD CF.
  bool NoMask = false;

  genx::AlignmentInfo AI;

  // Map from LLVM Value to pointer to the last used register alias for this
  // Value.
  std::map<Value *, Register *> LastUsedAliasMap;
  unsigned CurrentPadding = 0;

public:
  FunctionGroup *FG = nullptr;
  GenXLiveness *Liveness = nullptr;
  GenXNumbering *Numbering = nullptr;
  GenXVisaRegAlloc *RegAlloc = nullptr;
  FunctionGroupAnalysis *FGA = nullptr;
  GenXModule *GM = nullptr;
  LoopInfoGroupWrapperPass *LIs = nullptr;
  const GenXSubtarget *Subtarget = nullptr;
  const GenXBackendConfig *BackendConfig = nullptr;
  GenXBaling *Baling = nullptr;
  VISABuilder *CisaBuilder = nullptr;

private:
  bool allowI64Ops() const;
  void collectKernelInfo();
  void buildVariables();
  void buildInstructions();

  bool buildInstruction(Instruction *Inst);
  bool buildMainInst(Instruction *Inst, genx::BaleInfo BI, unsigned Mod,
                     const DstOpndDesc &DstDesc);
  void buildControlRegUpdate(unsigned Mask, bool Clear);
  void buildJoin(CallInst *Join, BranchInst *Branch);
  bool buildBranch(BranchInst *Branch);
  void buildIndirectBr(IndirectBrInst *Br);
  void buildIntrinsic(CallInst *CI, unsigned IntrinID, genx::BaleInfo BI,
                      unsigned Mod, const DstOpndDesc &DstDesc);
  void buildInputs(Function *F, bool NeedRetIP);

  void buildFunctionAddr(Instruction *Inst, const DstOpndDesc &DstDesc);
  void buildLoneWrRegion(const DstOpndDesc &Desc);
  void buildLoneWrPredRegion(Instruction *Inst, genx::BaleInfo BI);
  void buildLoneOperand(Instruction *Inst, genx::BaleInfo BI, unsigned Mod,
                        const DstOpndDesc &DstDesc);

  VISA_PredVar *getPredicateVar(Register *Idx);
  VISA_PredVar *getPredicateVar(Value *V);
  VISA_PredVar *getZeroedPredicateVar(Value *V);
  VISA_SurfaceVar *getPredefinedSurfaceVar(GlobalVariable &GV);
  VISA_GenVar *getPredefinedGeneralVar(GlobalVariable &GV);
  VISA_EMask_Ctrl getExecMaskFromWrPredRegion(Instruction *WrPredRegion,
                                                     bool IsNoMask);
  VISA_EMask_Ctrl getExecMaskFromWrRegion(const DstOpndDesc &DstDesc,
                                                 bool IsNoMask = false);
  unsigned getOrCreateLabel(const Value *V, int Kind);
  int getLabel(const Value *V) const;
  void setLabel(const Value *V, unsigned Num);

  void emitOptimizationHints();

  Value *getPredicateOperand(Instruction *Inst, unsigned OperandNum,
                             genx::BaleInfo BI, VISA_PREDICATE_CONTROL &Control,
                             VISA_PREDICATE_STATE &PredField,
                             VISA_EMask_Ctrl *MaskCtrl);
  bool isInLoop(BasicBlock *BB);

  void addLabelInst(const Value *BB);
  void buildPhiNode(PHINode *Phi);
  void buildGoto(CallInst *Goto, BranchInst *Branch);
  void buildCall(CallInst *CI, const DstOpndDesc &DstDesc);
  void buildStackCallLight(CallInst *CI, const DstOpndDesc &DstDesc);
  void buildInlineAsm(CallInst *CI);
  void buildPrintIndex(CallInst *CI, unsigned IntrinID, unsigned Mod,
                       const DstOpndDesc &DstDesc);
  void buildSelectInst(SelectInst *SI, genx::BaleInfo BI, unsigned Mod,
                       const DstOpndDesc &DstDesc);
  void buildBinaryOperator(BinaryOperator *BO, genx::BaleInfo BI, unsigned Mod,
                           const DstOpndDesc &DstDesc);
  void buildUnaryOperator(UnaryOperator *UO, genx::BaleInfo BI, unsigned Mod,
                          const DstOpndDesc &DstDesc);
  void buildBoolBinaryOperator(BinaryOperator *BO);
  void buildSymbolInst(CallInst *GAddrInst, unsigned Mod,
                       const DstOpndDesc &DstDesc);
  void buildCastInst(CastInst *CI, genx::BaleInfo BI, unsigned Mod,
                     const DstOpndDesc &DstDesc);
  void buildConvertAddr(CallInst *CI, genx::BaleInfo BI, unsigned Mod,
                        const DstOpndDesc &DstDesc);
  void buildAlloca(CallInst *CI, unsigned IntrinID, unsigned Mod,
                   const DstOpndDesc &DstDesc);
  void buildLoneReadVariableRegion(CallInst &CI);
  void buildLoneWriteVariableRegion(CallInst &CI);
  void buildWritePredefSurface(CallInst &CI);
  void addWriteRegionLifetimeStartInst(Instruction *WrRegion);
  void addLifetimeStartInst(Instruction *Inst);
  void AddGenVar(Register &Reg);
  void buildRet(ReturnInst *RI);
  void buildNoopCast(CastInst *CI, genx::BaleInfo BI, unsigned Mod,
                     const DstOpndDesc &DstDesc);
  void buildCmp(CmpInst *Cmp, genx::BaleInfo BI, const DstOpndDesc &DstDesc);
  void buildExtractRetv(ExtractValueInst *Inst);
  void buildInsertRetv(InsertValueInst *Inst);

  VISA_VectorOpnd *createState(Register *Reg, unsigned Offset, bool IsDst);
  VISA_Type getVISAImmTy(uint8_t ImmTy);

  VISA_PredOpnd *createPredOperand(VISA_PredVar *PredVar,
                                   VISA_PREDICATE_STATE State,
                                   VISA_PREDICATE_CONTROL Control);

  VISA_VectorOpnd *createCisaSrcOperand(VISA_GenVar *Decl, VISA_Modifier Mod,
                                        unsigned VStride, unsigned Width,
                                        unsigned HStride, unsigned ROffset,
                                        unsigned COffset);

  VISA_VectorOpnd *createCisaDstOperand(VISA_GenVar *Decl, unsigned HStride,
                                        unsigned ROffset, unsigned COffset);

  VISA_VectorOpnd *createDestination(Value *Dest, genx::Signedness Signed,
                                     unsigned Mod, const DstOpndDesc &DstDesc,
                                     genx::Signedness *SignedRes = nullptr,
                                     unsigned *Offset = nullptr);
  VISA_VectorOpnd *createDestination(CisaVariable *Dest,
                                     genx::Signedness Signed,
                                     unsigned *Offset = nullptr);
  VISA_VectorOpnd *createDestination(Value *Dest,
                                     genx::Signedness Signed,
                                     unsigned *Offset = nullptr);
  VISA_VectorOpnd *createSourceOperand(Instruction *Inst,
                                       genx::Signedness Signed,
                                       unsigned OperandNum, genx::BaleInfo BI,
                                       unsigned Mod = 0,
                                       genx::Signedness *SignedRes = nullptr,
                                       unsigned MaxWidth = 16);
  VISA_VectorOpnd *createSource(CisaVariable *V, genx::Signedness Signed,
                                unsigned MaxWidth = 16,
                                unsigned *Offset = nullptr);
  VISA_VectorOpnd *createSource(Value *V, genx::Signedness Signed, bool Baled,
                                unsigned Mod = 0,
                                genx::Signedness *SignedRes = nullptr,
                                unsigned MaxWidth = 16,
                                unsigned *Offset = nullptr, bool IsBF = false);
  VISA_VectorOpnd *createSource(Value *V, genx::Signedness Signed,
                                unsigned MaxWidth = 16,
                                unsigned *Offset = nullptr);

  std::string createInlineAsmOperand(const Instruction *Inst, Register *Reg,
                                     genx::Region *R, bool IsDst,
                                     genx::Signedness Signed,
                                     genx::ConstraintType Ty, unsigned Mod);

  std::string createInlineAsmSourceOperand(const Instruction *Inst, Value *V,
                                           genx::Signedness Signed, bool Baled,
                                           genx::ConstraintType Ty,
                                           unsigned Mod = 0,
                                           unsigned MaxWidth = 16);

  std::string createInlineAsmDestinationOperand(
      const Instruction *Inst, Value *Dest, genx::Signedness Signed,
      genx::ConstraintType Ty, unsigned Mod, const DstOpndDesc &DstDesc);

  VISA_VectorOpnd *createImmediateOperand(Constant *V, genx::Signedness Signed);

  VISA_PredVar *createPredicateDeclFromSelect(Instruction *SI,
                                              genx::BaleInfo BI,
                                              VISA_PREDICATE_CONTROL &Control,
                                              VISA_PREDICATE_STATE &PredField,
                                              VISA_EMask_Ctrl *MaskCtrl);

  VISA_RawOpnd *createRawSourceOperand(const Instruction *Inst,
                                       unsigned OperandNum, genx::BaleInfo BI,
                                       genx::Signedness Signed);
  VISA_RawOpnd *createRawDestination(Value *V, const DstOpndDesc &DstDesc,
                                     genx::Signedness Signed);

  VISA_VectorOpnd *createAddressOperand(Value *V, bool IsDst);

  void emitFileAndLocVisa(Instruction *CurrentInst);

  void addDebugInfo(Instruction *CurrentInst, bool Finalize);

  void deduceRegion(Region *R, bool IsDest, unsigned MaxWidth = 16);

  VISA_VectorOpnd *createGeneralOperand(genx::Region *R, VISA_GenVar *Decl,
                                        genx::Signedness Signed, unsigned Mod,
                                        bool IsDest, unsigned MaxWidth = 16);
  VISA_VectorOpnd *createIndirectOperand(genx::Region *R,
                                         genx::Signedness Signed, unsigned Mod,
                                         bool IsDest, unsigned MaxWidth = 16);
  VISA_VectorOpnd *createRegionOperand(genx::Region *R, VISA_GenVar *Decl,
                                       genx::Signedness Signed, unsigned Mod,
                                       bool IsDest, unsigned MaxWidth = 16);
  VISA_PredOpnd *createPredFromWrRegion(const DstOpndDesc &DstDesc);

  VISA_PredOpnd *createPred(Instruction *Inst, genx::BaleInfo BI,
                            unsigned OperandNum);

  Instruction *getOriginalInstructionForSource(Instruction *CI,
                                               genx::BaleInfo BI);
  void buildConvert(CallInst *CI, genx::BaleInfo BI, unsigned Mod,
                    const DstOpndDesc &DstDesc);
  std::string buildAsmName() const;
  void beginFunctionLight(Function *Func);

  unsigned getFuncArgsSize(Function *F);
  unsigned getValueSize(Type *T, unsigned Mod = 32) const;
  unsigned getValueSize(CisaVariable *V) const {
    return V->getByteSize();
  }
  unsigned getValueSize(Value *V, unsigned Mod = 32) const {
    return getValueSize(V->getType(), Mod);
  }
  GenericCisaVariable *createCisaVariable(VISAKernel *Kernel, const char *Name,
                                   VISA_GenVar *AliasVar, unsigned ByteSize);

  template <typename T1, typename T2>
  void emitVectorCopy(
      T1 *Dst, T2 *Src, unsigned &RowOff, unsigned &ColOff, unsigned &SrcRowOff,
      unsigned &SrcColOff, int TotalSize, bool DoCopy = true);

  void pushStackArg(VISA_StateOpndHandle *Dst, Value *Src, int TotalSz,
                    unsigned &RowOff, unsigned &ColOff, unsigned &SrcRowOff,
                    unsigned &SrcColOff, bool DoCopy = true);
  void popStackArg(Value *Dst, VISA_StateOpndHandle *Src, int TotalSz,
                   unsigned &RowOff, unsigned &ColOff, unsigned &SrcRowOff,
                   unsigned &SrcColOff, int &PrevStackOff);
  Signedness getCommonSignedness(ArrayRef<Value *> Vs) const;

  Register *getLastUsedAlias(Value *V) const;

  template <typename... Args>
  Register *getRegForValueUntypedAndSaveAlias(Args &&... args);
  template <typename... Args>
  Register *getRegForValueOrNullAndSaveAlias(Args &&... args);
  template <typename... Args>
  Register *getRegForValueAndSaveAlias(Args &&... args);

  void runOnKernel();
  void runOnFunction();

public:
  GenXKernelBuilder(FunctionGroup &FG)
      : TheKernelMetadata(FG.getHead()), Ctx(FG.getContext()),
        DL(FG.getModule()->getDataLayout()), FG(&FG) {
    collectKernelInfo();
  }
  ~GenXKernelBuilder() { clearLoops(); }
  void clearLoops() {
    for (auto i = Loops.begin(), e = Loops.end(); i != e; ++i) {
      delete i->second;
      i->second = nullptr;
    }
    Loops.clear();
  }

  bool run();

  LLVMContext &getContext() { return Ctx; }

  unsigned addStringToPool(StringRef Str);
  StringRef getStringByIndex(unsigned Val);
};
ModulePass *createGenXCisaBuilderWrapperPass() {
  initializeGenXCisaBuilderWrapperPass(*PassRegistry::getPassRegistry());
  return new GenXCisaBuilderWrapper();
}

} // end namespace llvm

INITIALIZE_PASS_BEGIN(GenXCisaBuilderWrapper, "GenXCisaBuilderPassWrapper",
                      "GenXCisaBuilderPassWrapper", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoGroupWrapperPassWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXGroupBalingWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXLivenessWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXVisaRegAllocWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXModule)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(GenXCisaBuilderWrapper, "GenXCisaBuilderPassWrapper",
                    "GenXCisaBuilderPassWrapper", false, false)

void GenXCisaBuilder::getAnalysisUsage(AnalysisUsage &AU) {
  AU.addRequired<LoopInfoGroupWrapperPass>();
  AU.addRequired<GenXGroupBaling>();
  AU.addRequired<GenXLiveness>();
  AU.addRequired<GenXVisaRegAlloc>();
  AU.addRequired<GenXModule>();
  AU.addRequired<FunctionGroupAnalysis>();
  AU.addRequired<TargetPassConfig>();
  AU.addRequired<GenXBackendConfig>();
  AU.setPreservesAll();
}

bool GenXCisaBuilder::runOnFunctionGroup(FunctionGroup &FG) {
  Ctx = &FG.getContext();
  std::unique_ptr<GenXKernelBuilder> KernelBuilder(new GenXKernelBuilder(FG));
  KernelBuilder->FGA = getAnalysisIfAvailable<FunctionGroupAnalysis>();
  KernelBuilder->GM = getAnalysisIfAvailable<GenXModule>();
  KernelBuilder->CisaBuilder = KernelBuilder->GM->GetCisaBuilder();
  KernelBuilder->RegAlloc = getAnalysisIfAvailable<GenXVisaRegAlloc>();
  KernelBuilder->Baling = &getAnalysis<GenXGroupBaling>();
  KernelBuilder->LIs = &getAnalysis<LoopInfoGroupWrapperPass>();
  KernelBuilder->Liveness = &getAnalysis<GenXLiveness>();
  KernelBuilder->Subtarget = &getAnalysis<TargetPassConfig>()
                                  .getTM<GenXTargetMachine>()
                                  .getGenXSubtarget();
  const auto &BC = getAnalysis<GenXBackendConfig>();
  KernelBuilder->BackendConfig = &BC;

  KernelBuilder->run();

  GenXModule *GM = KernelBuilder->GM;
  if (GM->HasInlineAsm() || !BC.getVISALTOStrings().empty()) {
    auto VISAAsmTextReader = GM->GetVISAAsmReader();
    auto ParseVISA = [&](const std::string& text) {
      const int R = VISAAsmTextReader->ParseVISAText(text, "");
      if (R != 0)
        handleInlineAsmParseError(BC, VISAAsmTextReader->GetCriticalMsg(), text,
                                  *Ctx);
    };
    ParseVISA(KernelBuilder->CisaBuilder->GetAsmTextStream().str());
    for (auto VisaAsm : BC.getVISALTOStrings()) {
      ParseVISA(VisaAsm);
    }
  }

  return false;
}

static bool isDerivedFromUndef(Constant *C) {
  if (isa<UndefValue>(C))
    return true;
  if (!isa<ConstantExpr>(C))
    return false;
  ConstantExpr *CE = cast<ConstantExpr>(C);
  for (auto &Opnd : CE->operands())
    if (isDerivedFromUndef(cast<Constant>(Opnd)))
      return true;
  return false;
}

static unsigned get8bitPackedFloat(float f) {
  union {
    float f;
    unsigned u;
  } u;

  u.f = f;
  unsigned char Sign = (u.u >> 31) << 7;
  unsigned Exp = (u.u >> 23) & 0xFF;
  unsigned Frac = u.u & 0x7FFFFF;
  if (Exp == 0 && Frac == 0)
    return Sign;

  IGC_ASSERT(Exp >= 124);
  IGC_ASSERT(Exp <= 131);
  Exp -= 124;
  IGC_ASSERT((Frac & 0x780000) == Frac);
  Frac >>= 19;
  IGC_ASSERT(!(Exp == 124 && Frac == 0));

  Sign |= (Exp << 4);
  Sign |= Frac;

  return Sign;
}

static Signedness getISatSrcSign(unsigned IID) {
  switch (IID) {
  case GenXIntrinsic::genx_sstrunc_sat:
  case GenXIntrinsic::genx_ustrunc_sat:
    return SIGNED;
  case GenXIntrinsic::genx_sutrunc_sat:
  case GenXIntrinsic::genx_uutrunc_sat:
    return UNSIGNED;
  default:
    return DONTCARESIGNED;
  }
}

static Signedness getISatDstSign(unsigned IID) {
  switch (IID) {
  case GenXIntrinsic::genx_sstrunc_sat:
  case GenXIntrinsic::genx_sutrunc_sat:
    return SIGNED;
  case GenXIntrinsic::genx_ustrunc_sat:
  case GenXIntrinsic::genx_uutrunc_sat:
    return UNSIGNED;
  default:
    return DONTCARESIGNED;
  }
}

static Signedness getISatSrcSign(Value *V) {
  return getISatSrcSign(GenXIntrinsic::getGenXIntrinsicID(V));
}

static Signedness getISatDstSign(Value *V) {
  return getISatDstSign(GenXIntrinsic::getGenXIntrinsicID(V));
}

// isExtOperandBaled : check whether a sext/zext operand is baled.
static bool isExtOperandBaled(Instruction *Inst, unsigned OpIdx,
                              const GenXBaling *Baling) {
  BaleInfo InstBI = Baling->getBaleInfo(Inst);
  if (!InstBI.isOperandBaled(OpIdx))
    return false;

  auto OpInst = cast<Instruction>(Inst->getOperand(OpIdx));
  BaleInfo OpBI = Baling->getBaleInfo(OpInst);
  return OpBI.Type == BaleInfo::ZEXT || OpBI.Type == BaleInfo::SEXT;
}

static bool isExtOperandBaled(Use &U, const GenXBaling *Baling) {
  return isExtOperandBaled(cast<Instruction>(U.get()), U.getOperandNo(),
                           Baling);
}

// Args:
//    HasBarrier - whether kernel has barrier or sbarrier
void addKernelAttrsFromMetadata(VISAKernel &Kernel,
                                const vc::KernelMetadata &KM,
                                const GenXSubtarget *Subtarget,
                                bool HasBarrier) {
  unsigned SLMSizeInKb = divideCeil(KM.getSLMSize(), 1024);
  if (SLMSizeInKb > Subtarget->getMaxSlmSize())
    report_fatal_error("SLM size exceeds target limits");
  if (!Subtarget->isOCLRuntime() && SLMSizeInKb > 255)
    report_fatal_error("SLM size greater than 255KB is not supported by CMRT");
  Kernel.AddKernelAttribute("SLMSize", sizeof(SLMSizeInKb), &SLMSizeInKb);

  // Load thread payload from memory.
  if (Subtarget->hasThreadPayloadInMemory()) {
    // The number of GRFs for per thread inputs (thread local IDs)
    unsigned NumGRFs = 0;
    bool HasImplicit = false;
    for (auto Kind : KM.getArgKinds()) {
      if (Kind & 0x8)
        HasImplicit = true;
    }
    if (Subtarget->isOCLRuntime()) {
      // When CM kernel is run with OCL runtime, it is dispatched in a
      // special "SIMD1" mode (aka "Programmable Media Kernels").
      // This mode implies that we always have a "full" thread payload,
      // even when CM kernel does *not* have implicit arguments.
      // Payload format:
      // | 0-15     | 16 - 31  | 32 - 47  | 46 - 256 |
      // | localIDX | localIDY | localIDZ | unused   |
      NumGRFs = 1;
    } else {
      // One GRF for per thread input size for CM
      NumGRFs = std::max(HasImplicit ? 1U : 0U, NumGRFs);
    }

    uint16_t Bytes = NumGRFs * Subtarget->getGRFByteSize();
    Kernel.AddKernelAttribute("PerThreadInputSize", sizeof(Bytes), &Bytes);
  }

  if (Subtarget->hasNBarrier()) {
    uint8_t BarrierCnt =
        static_cast<uint8_t>(KM.getAlignedBarrierCnt(HasBarrier));
    Kernel.AddKernelAttribute("NBarrierCnt", 1, &BarrierCnt);
  }
}

// Legalize name for using as filename or in visa asm
static std::string legalizeName(std::string Name) {
  std::replace_if(Name.begin(), Name.end(),
                  [](unsigned char c) { return (!isalnum(c) && c != '_'); },
                  '_');
  return Name;
}

std::string GenXKernelBuilder::buildAsmName() const {
  std::string AsmName;
  auto UserAsmName = AsmNameOpt.getValue();
  if (UserAsmName.empty()) {
    AsmName = vc::legalizeShaderDumpName(TheKernelMetadata.getName());
  } else {
    int idx = -1;
    auto *KernelMDs =
        FG->getModule()->getOrInsertNamedMetadata(genx::FunctionMD::GenXKernels);
    unsigned E = KernelMDs->getNumOperands();
    for (unsigned I = 0; I < E; ++I) {
      MDNode *KernelMD = KernelMDs->getOperand(I);
      StringRef KernelName =
          cast<MDString>(KernelMD->getOperand(genx::KernelMDOp::Name).get())
              ->getString();
      if (KernelName == TheKernelMetadata.getName()) {
        idx = I;
        break;
      }
    }
    IGC_ASSERT(idx >= 0);
    // Reverse kernel ASM names during codegen.
    // This provides an option to match the old compiler's output.
    if (ReverseKernels.getValue())
      idx = E - idx - 1;
    AsmName = (UserAsmName + llvm::Twine('_') + llvm::Twine(idx)).str();
  }

  // Currently installed shader dumper can provide its own path for
  // dumps. Prepend it to generated asm name.
  if (!BackendConfig->hasShaderDumper())
    return AsmName;

  vc::ShaderDumper &Dumper = BackendConfig->getShaderDumper();
  return Dumper.composeDumpPath(AsmName);
}

void GenXKernelBuilder::runOnKernel() {
  IGC_ASSERT(TheKernelMetadata.isKernel());

  const std::string KernelName = TheKernelMetadata.getName().str();
  CisaBuilder->AddKernel(MainKernel, KernelName.c_str());
  Kernel = static_cast<VISAFunction *>(MainKernel);
  Func2Kern[Func] = Kernel;

  IGC_ASSERT_MESSAGE(Kernel, "Kernel initialization failed!");
  LLVM_DEBUG(dbgs() << "=== PROCESS KERNEL(" << KernelName << ") ===\n");

  addKernelAttrsFromMetadata(*Kernel, TheKernelMetadata, Subtarget, HasBarrier);

  // Set CM target for all functions produced by VC.
  // See visa spec for CMTarget value (section 4, Kernel).
  const uint8_t CMTarget = 0;
  CISA_CALL(Kernel->AddKernelAttribute("Target", sizeof(CMTarget), &CMTarget));

  bool NeedRetIP = false; // Need special return IP variable for FC.
  // For a kernel, add an attribute for asm filename for the jitter.
  std::string AsmName = buildAsmName();
  StringRef AsmNameRef = AsmName;
  CISA_CALL(Kernel->AddKernelAttribute("OutputAsmPath", AsmNameRef.size(),
                                       AsmNameRef.begin()));
  // Populate variable attributes if any.
  unsigned Idx = 0;
  bool IsComposable = false;
  for (auto &Arg : Func->args()) {
    const char *Kind = nullptr;
    switch (TheKernelMetadata.getArgInputOutputKind(Idx++)) {
    default:
      break;
    case vc::KernelMetadata::ArgIOKind::Input:
      Kind = "Input";
      break;
    case vc::KernelMetadata::ArgIOKind::Output:
      Kind = "Output";
      break;
    case vc::KernelMetadata::ArgIOKind::InputOutput:
      Kind = "Input_Output";
      break;
    }
    if (Kind != nullptr) {
      auto R = getRegForValueUntypedAndSaveAlias(&Arg);
      IGC_ASSERT(R);
      IGC_ASSERT(R->Category == vc::RegCategory::General);
      R->addAttribute(addStringToPool(Kind), "");
      IsComposable = true;
    }
  }
  if (IsComposable)
    CISA_CALL(Kernel->AddKernelAttribute("Composable", 0, ""));
  if (HasCallable) {
    CISA_CALL(Kernel->AddKernelAttribute("Caller", 0, ""));
    NeedRetIP = true;
  }
  if (Func->hasFnAttribute("CMCallable")) {
    CISA_CALL(Kernel->AddKernelAttribute("Callable", 0, ""));
    NeedRetIP = true;
  }
  if (Func->hasFnAttribute("CMEntry")) {
    CISA_CALL(Kernel->AddKernelAttribute("Entry", 0, ""));
  }

  if (NeedRetIP) {
    // Ask RegAlloc to add a special variable RetIP.
    RegAlloc->addRetIPArgument();
    auto R = RegAlloc->getRetIPArgument();
    R->NameStr = "RetIP";
    R->addAttribute(addStringToPool("Input_Output"), "");
  }

  // Emit optimization hints if any.
  emitOptimizationHints();

  // Build variables
  buildVariables();

  // Build input variables
  buildInputs(Func, NeedRetIP);
}

void GenXKernelBuilder::runOnFunction() {
  VISAFunction *visaFunc = nullptr;

  std::string FuncName = Func->getName().str();
  CisaBuilder->AddFunction(visaFunc, FuncName.c_str());
  std::string AsmName = buildAsmName().append("_").append(FuncName);
  CISA_CALL(visaFunc->AddKernelAttribute("OutputAsmPath", AsmName.size(),
                                         AsmName.c_str()));
  IGC_ASSERT(visaFunc);
  Func2Kern[Func] = visaFunc;
  Kernel = visaFunc;
  buildVariables();
}

bool GenXKernelBuilder::run() {
  GrfByteSize = Subtarget ? Subtarget->getGRFByteSize() : defaultGRFByteSize;
  StackSurf = Subtarget ? Subtarget->stackSurface() : PREDEFINED_SURFACE_STACK;

  CRMask = CRBits::SinglePrecisionMode | CRBits::RoundingMode |
           CRBits::DoublePrecisionDenorm | CRBits::SinglePrecisionDenorm |
           CRBits::HalfPrecisionDenorm;

  UseNewStackBuilder =
      BackendConfig->useNewStackBuilder() && Subtarget->isOCLRuntime();
  StackCallExecSize =
      getExecSizeFromValue(BackendConfig->getInteropSubgroupSize());

  IGC_ASSERT(Subtarget);

  HasSimdCF = false;

  Func = FG->getHead();
  if (genx::fg::isGroupHead(*Func))
    runOnKernel();
  else if (genx::fg::isSubGroupHead(*Func))
    runOnFunction();
  else
    llvm_unreachable("unknown function group type");

  // Build instructions
  buildInstructions();

  // Reset Regalloc hook
  RegAlloc->SetRegPushHook(nullptr, nullptr);

  if (!HasSimdCF && HasStackcalls) {
    CISA_CALL(Kernel->AddKernelAttribute("AllLaneActive", 1, nullptr));
    LLVM_DEBUG(dbgs() << " Kernel " << Func->getName() << "has no SIMD CF,\n");
    LLVM_DEBUG(dbgs() << " Enable AllLaneActive for kernel.\n");
  }

  if (TheKernelMetadata.isKernel()) {
    // For a kernel with no barrier instruction, add a NoBarrier attribute.
    if (!HasBarrier && !TheKernelMetadata.hasNBarrier())
      CISA_CALL(Kernel->AddKernelAttribute("NoBarrier", 0, nullptr));
  }

  NumVisaInsts += Kernel->getvIsaInstCount();

  return false;
}

static bool PatchImpArgOffset(Function *F, const GenXSubtarget *ST,
                              const vc::KernelMetadata &KM) {
  IGC_ASSERT(ST);
  if (ST->isOCLRuntime())
    return false;
  if (!ST->hasThreadPayloadInMemory())
    return false;

  unsigned Idx = 0;
  for (auto i = F->arg_begin(), e = F->arg_end(); i != e; ++i, ++Idx) {
    uint8_t Kind = (KM.getArgKind(Idx));
    if (Kind & 0xf8)
      return true;
  }

  return false;
}

static unsigned getStateVariableSizeInBytes(const Type *Ty,
                                            const unsigned ElemSize) {
  auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
  if (!VTy)
    return ElemSize;
  return ElemSize * VTy->getNumElements();
}

static unsigned getInputSizeInBytes(const DataLayout &DL,
                                    const vc::RegCategory ArgCategory,
                                    Type *Ty) {
  switch (ArgCategory) {
  case vc::RegCategory::General:
    return DL.getTypeSizeInBits(Ty) / genx::ByteBits;
  case vc::RegCategory::Sampler:
    return getStateVariableSizeInBytes(Ty, genx::SamplerElementBytes);
  case vc::RegCategory::Surface:
    return getStateVariableSizeInBytes(Ty, genx::SurfaceElementBytes);
  default:
    break;
  }
  IGC_ASSERT_UNREACHABLE(); // Unexpected register category for input
}

void GenXKernelBuilder::buildInputs(Function *F, bool NeedRetIP) {

  IGC_ASSERT_MESSAGE(F->arg_size() == TheKernelMetadata.getNumArgs(),
    "Mismatch between metadata for kernel and number of args");

  // Number of globals to be binded statically.
  std::vector<std::pair<GlobalVariable *, int32_t>> Bindings;
  Module *M = F->getParent();
  for (auto &GV : M->getGlobalList()) {
    int32_t Offset = 0;
    GV.getAttribute(genx::FunctionMD::GenXByteOffset)
        .getValueAsString()
        .getAsInteger(0, Offset);
    if (Offset > 0)
      Bindings.emplace_back(&GV, Offset);
  }
  // Each argument.
  unsigned Idx = 0;
  bool PatchImpArgOff = PatchImpArgOffset(F, Subtarget, TheKernelMetadata);
  for (auto i = F->arg_begin(), e = F->arg_end(); i != e; ++i, ++Idx) {
    if (TheKernelMetadata.shouldSkipArg(Idx))
      continue;
    Argument *Arg = &*i;
    Register *Reg = getRegForValueUntypedAndSaveAlias(Arg);
    IGC_ASSERT(Reg);
    uint8_t Kind = TheKernelMetadata.getArgKind(Idx);
    uint16_t Offset = 0;
    if (!PatchImpArgOff) {
      Offset = TheKernelMetadata.getArgOffset(Idx);
    }
    else {
      if ((Kind >> 3) == 3) {
        Offset = GrfByteSize;
      } else {
        Offset = (TheKernelMetadata.getArgOffset(Idx) + GrfByteSize);
      }
    }
    // Argument size in bytes.
    const unsigned NumBytes = getInputSizeInBytes(
        DL, TheKernelMetadata.getArgCategory(Idx), Arg->getType());

    switch (Kind & 0x7) {
    case visa::VISA_INPUT_GENERAL:
    case visa::VISA_INPUT_SAMPLER:
    case visa::VISA_INPUT_SURFACE:
      CISA_CALL(Kernel->CreateVISAImplicitInputVar(
          Reg->GetVar<VISA_GenVar>(Kernel), Offset, NumBytes, Kind >> 3));
      break;

    default:
      report_fatal_error("Unknown input category");
      break;
    }
  }
  // Add pseudo-input for global variables with offset attribute.
  for (auto &Item : Bindings) {
    // TODO: sanity check. No overlap with other inputs.
    GlobalVariable *GV = Item.first;
    uint16_t Offset = Item.second;
    IGC_ASSERT(Offset > 0);
    uint16_t NumBytes = (GV->getValueType()->getPrimitiveSizeInBits() / 8U);
    uint8_t Kind = vc::KernelMetadata::IMP_PSEUDO_INPUT;
    Register *Reg = getRegForValueUntypedAndSaveAlias(GV);
    CISA_CALL(Kernel->CreateVISAImplicitInputVar(Reg->GetVar<VISA_GenVar>(Kernel),
                                                 Offset, NumBytes, Kind >> 3));
  }
  // Add the special RetIP argument.
  // Current assumption in Finalizer is that RetIP should be the last argument,
  // so we add it after generation of all other arguments.
  if (NeedRetIP) {
    Register *Reg = RegAlloc->getRetIPArgument();
    uint16_t Offset = (127 * GrfByteSize + 6 * 4); // r127.6
    uint16_t NumBytes = (64 / 8);
    uint8_t Kind = vc::KernelMetadata::IMP_PSEUDO_INPUT;
    CISA_CALL(Kernel->CreateVISAImplicitInputVar(Reg->GetVar<VISA_GenVar>(Kernel),
                                                 Offset, NumBytes, Kind >> 3));
  }
}

// FIXME: We should use NM by default once code quality issues are addressed
// in vISA compiler.
static bool setNoMaskByDefault(Function *F,
                               std::unordered_set<Function *> &Visited) {
  // We lower SIMDCF for stackcalls, so only predicates are significant. If VISA
  // makes scalar jmp to goto transformation and goto has width less than 32, NM
  // must be used by default. Otherwise, each legalized instruction that uses
  // M5-M8 will work incorrectly if at least one lane is disabled.
  if (vc::requiresStackCall(F))
    return true;

  for (auto &BB : F->getBasicBlockList())
    if (GotoJoin::isGotoBlock(&BB))
      return true;

  // Check if this is subroutine call.
  for (auto U : F->users()) {
    if (auto CI = dyn_cast<CallInst>(U)) {
      Function *G = CI->getFunction();
      if (Visited.count(G))
        continue;
      Visited.insert(G);
      if (setNoMaskByDefault(G, Visited))
        return true;
    }
  }

  return false;
}

void GenXKernelBuilder::buildInstructions() {
  for (auto It = FG->begin(), E = FG->end(); It != E; ++It) {
    Func = *It;
    LLVM_DEBUG(dbgs() << "Building IR for func " << Func->getName() << "\n");
    NoMask = [this]() {
      std::unordered_set<Function *> Visited;
      return setNoMaskByDefault(Func, Visited);
    }();

    LastUsedAliasMap.clear();

    if (vc::isKernel(Func) || vc::requiresStackCall(Func)) {
      KernFunc = Func;
    } else {
      auto *FuncFG = FGA->getAnyGroup(Func);
      IGC_ASSERT_MESSAGE(FuncFG, "Cannot find the function group");
      KernFunc = FuncFG->getHead();
    }

    IGC_ASSERT(KernFunc);
    Kernel = Func2Kern.at(KernFunc);

    unsigned LabelID = getOrCreateLabel(Func, LABEL_SUBROUTINE);
    CISA_CALL(Kernel->AppendVISACFLabelInst(Labels[LabelID]));
    GM->updateVisaMapping(KernFunc, nullptr, Kernel->getvIsaInstCount(),
                          "SubRoutine");

    beginFunctionLight(Func);
    CurrentPadding = 0;

    // If a float control is specified, emit code to make that happen.
    // Float control contains rounding mode, denorm behaviour and single
    // precision float mode (ALT or IEEE) Relevant bits are already set as
    // defined for VISA control reg in header definition on enums
    if (Func->hasFnAttribute(genx::FunctionMD::CMFloatControl)) {
      uint32_t FloatControl = 0;
      Func->getFnAttribute(genx::FunctionMD::CMFloatControl)
          .getValueAsString()
          .getAsInteger(0, FloatControl);

      // Clear current float control bits to known zero state
      buildControlRegUpdate(CRMask, true);

      // Set rounding mode to required state if that isn't zero
      FloatControl &= CRMask;
      if (FloatControl) {
        if (FG->getHead() == Func)
          DefaultFloatControl = FloatControl;
        buildControlRegUpdate(FloatControl, false);
      }
    }

    // Only output a label for the initial basic block if it is used from
    // somewhere else.
    bool NeedsLabel = !Func->front().use_empty();
    for (Function::iterator fi = Func->begin(), fe = Func->end(); fi != fe;
         ++fi) {
      BasicBlock *BB = &*fi;
      if (!NeedsLabel && BB != &Func->front()) {
        NeedsLabel = !BB->getSinglePredecessor();
        if (!NeedsLabel)
          NeedsLabel = GotoJoin::isJoinLabel(BB);
      }

      if (NeedsLabel) {
        unsigned LabelID = getOrCreateLabel(BB, LABEL_BLOCK);
        CISA_CALL(Kernel->AppendVISACFLabelInst(Labels[LabelID]));
        LLVM_DEBUG(dbgs() << "Append Label visa inst: " << LabelID << "\n");
      }
      NeedsLabel = true;
      for (BasicBlock::iterator bi = BB->begin(), be = BB->end(); bi != be;
           ++bi) {
        Instruction *Inst = &*bi;
        LLVM_DEBUG(dbgs() << "Current Inst to build = " << *Inst << "\n");
        // Fill debug info for current instruction
        addDebugInfo(Inst, false /*do not finalize*/);
        emitFileAndLocVisa(Inst);
        if (Inst->isTerminator()) {
          // Before the terminator inst of a basic block, if there is a single
          // successor and it is the header of a loop, for any vector of at
          // least four GRFs with a phi node where our incoming value is
          // undef, insert a lifetime.start here.
          auto *TI = cast<IGCLLVM::TerminatorInst>(Inst);
          if (TI->getNumSuccessors() == 1) {
            auto Succ = TI->getSuccessor(0);
            if (LIs->getLoopInfo(Succ->getParent())->isLoopHeader(Succ)) {
              for (auto si = Succ->begin();; ++si) {
                auto Phi = dyn_cast<PHINode>(&*si);
                if (!Phi)
                  break;
                if (Phi->getType()->getPrimitiveSizeInBits() >=
                        (GrfByteSize * 8) * 4 &&
                    isa<UndefValue>(
                        Phi->getIncomingValue(Phi->getBasicBlockIndex(BB))))
                  addLifetimeStartInst(Phi);
              }
            }
          }
        }
        // Build the instruction.
        if (!Baling->isBaled(Inst)) {
          if (buildInstruction(Inst))
            NeedsLabel = false;
        } else {
          LLVM_DEBUG(dbgs() << "Skip baled inst: " << *Inst << "\n");
        }
        // Finalize debug info after build instruction
        addDebugInfo(Inst, true /*finalize*/);
      }
    }
  }
}

bool GenXKernelBuilder::buildInstruction(Instruction *Inst) {
  LLVM_DEBUG(dbgs() << "Build inst: " << *Inst << "\n");
  // Process the bale that this is the head instruction of.
  BaleInfo BI = Baling->getBaleInfo(Inst);
  LLVM_DEBUG(dbgs() << "Bale type " << BI.Type << "\n");

  DstOpndDesc DstDesc;
  if (BI.Type == BaleInfo::GSTORE) {
    // Inst is a global variable store. It should be baled into a wrr
    // instruction.
    Bale B;
    Baling->buildBale(Inst, &B);
    // This is an identity bale; no code will be emitted.
    if (isIdentityBale(B))
      return false;

    IGC_ASSERT(BI.isOperandBaled(0));
    DstDesc.GStore = Inst;
    Inst = cast<Instruction>(Inst->getOperand(0));
    BI = Baling->getBaleInfo(Inst);
  }
  if (BI.Type == BaleInfo::REGINTR)
    return false;
  if (BI.Type == BaleInfo::WRREGION || BI.Type == BaleInfo::WRPREDREGION ||
      BI.Type == BaleInfo::WRPREDPREDREGION) {
    // Inst is a wrregion or wrpredregion or wrpredpredregion.
    DstDesc.WrRegion = Inst;
    DstDesc.WrRegionBI = BI;
    auto *CurInst = Inst;
    while (CurInst->hasOneUse() &&
           GenXIntrinsic::isWrRegion(CurInst->user_back()) &&
           CurInst->use_begin()->getOperandNo() ==
               GenXIntrinsic::GenXRegion::OldValueOperandNum)
      CurInst = CurInst->user_back();
    if (CurInst->hasOneUse() &&
        GenXIntrinsic::isWritePredefReg(CurInst->user_back()))
      DstDesc.WrPredefReg = CurInst->user_back();
    if (isa<UndefValue>(Inst->getOperand(0)) && !DstDesc.GStore) {
      // This is a wrregion, probably a partial write, to an undef value.
      // Write a lifetime start if appropriate to help the jitter's register
      // allocator.
      addWriteRegionLifetimeStartInst(DstDesc.WrRegion);
    }
    // See if it bales in the instruction
    // that generates the subregion/element.  That is always operand 1.
    enum { OperandNum = 1 };
    if (!BI.isOperandBaled(OperandNum)) {
      if (BI.Type == BaleInfo::WRPREDREGION) {
        buildLoneWrPredRegion(DstDesc.WrRegion, DstDesc.WrRegionBI);
      } else {
        buildLoneWrRegion(DstDesc);
      }
      return false;
    }
    // Yes, source of wrregion is baled in.
    Inst = cast<Instruction>(DstDesc.WrRegion->getOperand(OperandNum));
    BI = Baling->getBaleInfo(Inst);
  }
  if (BI.Type == BaleInfo::FADDR) {
    buildFunctionAddr(Inst, DstDesc);
    return false;
  }
  unsigned Mod = 0;
  if (BI.Type == BaleInfo::SATURATE) {
    // Inst is a fp saturate. See if it bales in the instruction that
    // generates the value to saturate. That is always operand 0. If
    // not, just treat the saturate as a normal intrinsic.
    if (BI.isOperandBaled(0)) {
      Mod = MODIFIER_SAT;
      Inst = cast<Instruction>(Inst->getOperand(0));
      BI = Baling->getBaleInfo(Inst);
    } else
      BI.Type = BaleInfo::MAININST;
  }
  if (BI.Type == BaleInfo::CMPDST) {
    // Dst of sel instruction is baled in.
    Inst = cast<Instruction>(Inst->getOperand(0));
    IGC_ASSERT_MESSAGE(isa<CmpInst>(Inst), "only bale sel into a cmp instr");
    BI = Baling->getBaleInfo(Inst);
  }
  switch (BI.Type) {
  case BaleInfo::RDREGION:
  case BaleInfo::ABSMOD:
  case BaleInfo::NEGMOD:
  case BaleInfo::NOTMOD:
    // This is a rdregion or modifier not baled in to a main instruction
    // (but possibly baled in to a wrregion or sat modifier).
    buildLoneOperand(Inst, BI, Mod, DstDesc);
    return false;
  }
  IGC_ASSERT(BI.Type == BaleInfo::MAININST || BI.Type == BaleInfo::NOTP ||
         BI.Type == BaleInfo::ZEXT || BI.Type == BaleInfo::SEXT);
  return buildMainInst(Inst, BI, Mod, DstDesc);
}

VISA_PredVar *GenXKernelBuilder::createPredicateDeclFromSelect(
    Instruction *SI, BaleInfo BI, VISA_PREDICATE_CONTROL &Control,
    VISA_PREDICATE_STATE &State, VISA_EMask_Ctrl *MaskCtrl) {
  *MaskCtrl = vISA_EMASK_M1_NM;
  // Get the predicate (mask) operand, scanning through baled in
  // all/any/not/rdpredregion and setting State and MaskCtrl
  // appropriately.
  Value *Mask = getPredicateOperand(SI, 0 /*selector operand in select*/, BI,
                                    Control, State, MaskCtrl);
  IGC_ASSERT(!isa<Constant>(Mask));
  // Variable predicate. Derive the predication field from any baled in
  // all/any/not and the predicate register number.
  Register *Reg = getRegForValueAndSaveAlias(Mask);
  IGC_ASSERT(Reg);
  IGC_ASSERT(Reg->Category == vc::RegCategory::Predicate);
  if (NoMask)
    *MaskCtrl |= vISA_EMASK_M1_NM;
  return getPredicateVar(Reg);
}

VISA_PredOpnd *
GenXKernelBuilder::createPredFromWrRegion(const DstOpndDesc &DstDesc) {
  VISA_PredOpnd *result = nullptr;
  Instruction *WrRegion = DstDesc.WrRegion;
  if (WrRegion) {
    // Get the predicate (mask) operand, scanning through baled in
    // all/any/not/rdpredregion and setting PredField and MaskCtrl
    // appropriately.
    VISA_EMask_Ctrl MaskCtrl;
    VISA_PREDICATE_CONTROL Control;
    VISA_PREDICATE_STATE State;
    Value *Mask =
        getPredicateOperand(WrRegion, 7 /*mask operand in wrregion*/,
                            DstDesc.WrRegionBI, Control, State, &MaskCtrl);
    if (auto C = dyn_cast<Constant>(Mask)) {
      (void)C;
      IGC_ASSERT_MESSAGE(C->isAllOnesValue(),
       "wrregion mask or predication operand must be const 1 or not constant");
    } else {
      // Variable predicate. Derive the predication field from any baled in
      // all/any/not and the predicate register number. If the predicate has
      // not has a register allocated, it must be EM.
      Register *Reg = getRegForValueOrNullAndSaveAlias(Mask);
      if (Reg) {
        IGC_ASSERT(Reg->Category == vc::RegCategory::Predicate);
        result = createPredOperand(getPredicateVar(Reg), State, Control);
      }
    }
  }
  return result;
}

/***********************************************************************
 * createPred : create predication field from an instruction operand
 *
 * Enter:   Inst = the instruction (0 to write an "always true" pred field)
 *          BI = BaleInfo for the instruction, so we can see if there is a
 *                rdpredregion baled in to the mask
 *          OperandNum = operand number in the instruction
 *
 * If the operand is not constant 1, then it must be a predicate register.
 */
VISA_PredOpnd *GenXKernelBuilder::createPred(Instruction *Inst, BaleInfo BI,
                                             unsigned OperandNum) {
  VISA_PredOpnd *ResultOperand = nullptr;
  VISA_PREDICATE_CONTROL PredControl;
  VISA_PREDICATE_STATE Inverse;
  VISA_EMask_Ctrl MaskCtrl;
  Value *Mask = getPredicateOperand(Inst, OperandNum, BI, PredControl, Inverse,
                                    &MaskCtrl);
  if (auto C = dyn_cast<Constant>(Mask)) {
    (void)C;
    IGC_ASSERT_MESSAGE(C->isAllOnesValue(),
      "wrregion mask or predication operand must be const 1 or not constant");
  } else {
    // Variable predicate. Derive the predication field from any baled in
    // all/any/not and the predicate register number. If the predicate has not
    // has a register allocated, it must be EM.
    Register *Reg = getRegForValueOrNullAndSaveAlias(Mask);
    VISA_PredVar *PredVar = nullptr;
    if (Reg) {
      IGC_ASSERT(Reg->Category == vc::RegCategory::Predicate);
      PredVar = getPredicateVar(Reg);
    } else
      return nullptr;
    ResultOperand = createPredOperand(PredVar, Inverse, PredControl);
  }
  return ResultOperand;
}

VISA_VectorOpnd *GenXKernelBuilder::createState(Register *Reg, unsigned Offset,
                                                bool IsDst) {
  uint8_t Size = 0;
  VISA_VectorOpnd *Op = nullptr;

  switch (Reg->Category) {
  case vc::RegCategory::Surface:
    CISA_CALL(Kernel->CreateVISAStateOperand(Op, Reg->GetVar<VISA_SurfaceVar>(Kernel),
                                             Size, Offset, IsDst));
    break;
  case vc::RegCategory::Sampler:
    CISA_CALL(Kernel->CreateVISAStateOperand(Op, Reg->GetVar<VISA_SamplerVar>(Kernel),
                                             Size, Offset, IsDst));
    break;
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "unknown state operand");
  }

  return Op;
}

VISA_VectorOpnd *GenXKernelBuilder::createDestination(CisaVariable *Dest,
                                                      genx::Signedness Signed,
                                                      unsigned *Offset) {
  Region R(IGCLLVM::FixedVectorType::get(
      IntegerType::get(Ctx, CISATypeTable[Dest->getType()].typeSize * CHAR_BIT),
      Dest->getNumElements()));
  if (Offset)
    R.Offset = *Offset;
  return createRegionOperand(&R, Dest->getGenVar(), Signed, 0, true);
}

VISA_VectorOpnd *GenXKernelBuilder::createDestination(Value *Dest,
                                                      genx::Signedness Signed,
                                                      unsigned *Offset) {
  return createDestination(Dest, Signed, 0, DstOpndDesc(), nullptr, Offset);
}

VISA_VectorOpnd *
GenXKernelBuilder::createDestination(Value *Dest, genx::Signedness Signed,
                                     unsigned Mod, const DstOpndDesc &DstDesc,
                                     Signedness *SignedRes, unsigned *Offset) {
  auto IID = vc::InternalIntrinsic::getInternalIntrinsicID(Dest);
  bool IsBF = IID == vc::InternalIntrinsic::cast_to_bf16;

  LLVM_DEBUG(dbgs() << "createDest for value: "
                    << (IsBF ? " brain " : " non-brain ") << *Dest
                    << ", wrr: ");
  if (DstDesc.WrRegion)
    LLVM_DEBUG(dbgs() << *(DstDesc.WrRegion));
  else
    LLVM_DEBUG(dbgs() << "null");
  LLVM_DEBUG(dbgs() << "\n");
  IGC_ASSERT_MESSAGE(!Dest->getType()->isAggregateType(),
    "cannot create destination register of an aggregate type");
  if (SignedRes)
    *SignedRes = Signed;

  Type *OverrideType = nullptr;
  if (BitCastInst *BCI = dyn_cast<BitCastInst>(Dest)) {
    if (!(isa<Constant>(BCI->getOperand(0))) &&
        !(BCI->getType()->getScalarType()->isIntegerTy(1)) &&
        (BCI->getOperand(0)->getType()->getScalarType()->isIntegerTy(1))) {
      if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(Dest->getType())) {
        unsigned int NumBits = VT->getNumElements() *
                               VT->getElementType()->getPrimitiveSizeInBits();
        OverrideType = IntegerType::get(BCI->getContext(), NumBits);
      }
    }
  }

  // Saturation can also change signedness.
  if (!Dest->user_empty() && GenXIntrinsic::isIntegerSat(Dest->user_back())) {
    Signed = getISatDstSign(Dest->user_back());
  }

  if (!DstDesc.WrRegion) {
    if (Mod) {
      // There is a sat modifier. Either it is an fp saturate, which is
      // represented by its own intrinsic which this instruction is baled
      // into, or it is an int saturate which always comes from this
      // instruction's semantics. In the former case, use the value
      // that is the result of the saturate. But only if this instruction
      // itself is not the sat intrinsic.
      if (Dest->getType()->getScalarType()->isFloatingPointTy() &&
          GenXIntrinsic::getGenXIntrinsicID(Dest) != GenXIntrinsic::genx_sat)
        Dest = cast<Instruction>(Dest->use_begin()->getUser());
    }
    if ((Mod & MODIFIER_SAT) != 0) {
      // Similar for integer saturation.
      if (Dest->getType()->getScalarType()->isIntegerTy() &&
          !GenXIntrinsic::isIntegerSat(Dest) && GenXIntrinsic::isIntegerSat(Dest->user_back()))
        Dest = cast<Instruction>(Dest->user_back());
    }
    Register *Reg =
        getRegForValueAndSaveAlias(Dest, Signed, OverrideType, IsBF);
    if (SignedRes)
      *SignedRes = RegAlloc->getSigned(Reg);
    // Write the vISA general operand:
    if (Reg->Category == vc::RegCategory::General) {
      Region DestR(Dest);
      if (Offset)
        DestR.Offset = *Offset;
      return createRegionOperand(&DestR, Reg->GetVar<VISA_GenVar>(Kernel),
                                 DONTCARESIGNED, Mod, true /*isDest*/);
    } else {
      IGC_ASSERT(Reg->Category == vc::RegCategory::Surface ||
                 Reg->Category == vc::RegCategory::Sampler);

      return createState(Reg, 0 /*Offset*/, true /*IsDst*/);
    }
  }
  // We need to allow for the case that there is no register allocated if it
  // is an indirected arg, and that is OK because the region is indirect so
  // the vISA does not contain the base register.
  Register *Reg;

  Value *V = nullptr;
  if (DstDesc.GStore) {
    auto *GV = vc::getUnderlyingGlobalVariable(DstDesc.GStore->getOperand(1));
    IGC_ASSERT_MESSAGE(GV, "out of sync");
    if (OverrideType == nullptr)
      OverrideType = DstDesc.GStore->getOperand(0)->getType();
    Reg = getRegForValueAndSaveAlias(GV, Signed, OverrideType, IsBF);
    V = GV;
  } else {
    V = DstDesc.WrPredefReg ? DstDesc.WrPredefReg : DstDesc.WrRegion;
    // if (!V->user_empty() && GenXIntrinsic::isWritePredefReg(V->user_back()))
    //   V = V->user_back();
    Reg = getRegForValueOrNullAndSaveAlias(V, Signed, OverrideType, IsBF);
  }

  // Write the vISA general operand with region:
  Region R = makeRegionFromBaleInfo(DstDesc.WrRegion, DstDesc.WrRegionBI);

  if (SignedRes)
    *SignedRes = RegAlloc->getSigned(Reg);

  if (Reg && (Reg->Category == vc::RegCategory::Sampler ||
              Reg->Category == vc::RegCategory::Surface)) {
    return createState(Reg, R.getOffsetInElements(), true /*IsDest*/);
  } else {
    IGC_ASSERT(!Reg || Reg->Category == vc::RegCategory::General);
    auto Decl = Reg ? Reg->GetVar<VISA_GenVar>(Kernel) : nullptr;
    return createRegionOperand(&R, Decl, Signed, Mod, true /*IsDest*/);
  }
}

VISA_VectorOpnd *GenXKernelBuilder::createSourceOperand(
    Instruction *Inst, Signedness Signed, unsigned OperandNum,
    genx::BaleInfo BI, unsigned Mod, Signedness *SignedRes, unsigned MaxWidth) {
  Value *V = Inst->getOperand(OperandNum);
  auto IID = vc::InternalIntrinsic::getInternalIntrinsicID(Inst);
  bool IsBF = IID == vc::InternalIntrinsic::cast_from_bf16;

  return createSource(V, Signed, BI.isOperandBaled(OperandNum), Mod, SignedRes,
                      MaxWidth, nullptr, IsBF);
}

VISA_PredOpnd *
GenXKernelBuilder::createPredOperand(VISA_PredVar *PredVar,
                                     VISA_PREDICATE_STATE State,
                                     VISA_PREDICATE_CONTROL Control) {
  VISA_PredOpnd *PredOperand = nullptr;
  CISA_CALL(
      Kernel->CreateVISAPredicateOperand(PredOperand, PredVar, State, Control));

  return PredOperand;
}

VISA_VectorOpnd *GenXKernelBuilder::createCisaSrcOperand(
    VISA_GenVar *Decl, VISA_Modifier Mod, unsigned VStride, unsigned Width,
    unsigned HStride, unsigned ROffset, unsigned COffset) {
  VISA_VectorOpnd *ResultOperand = nullptr;
  CISA_CALL(Kernel->CreateVISASrcOperand(ResultOperand, Decl, Mod, VStride,
                                         Width, HStride, ROffset, COffset));
  return ResultOperand;
}

VISA_VectorOpnd *GenXKernelBuilder::createCisaDstOperand(VISA_GenVar *Decl,
                                                         unsigned HStride,
                                                         unsigned ROffset,
                                                         unsigned COffset) {
  VISA_VectorOpnd *ResultOperand = nullptr;
  CISA_CALL(Kernel->CreateVISADstOperand(ResultOperand, Decl, HStride, ROffset,
                                         COffset));
  return ResultOperand;
}

/***********************************************************************
 * createAddressOperand : create an address register operand
 */
VISA_VectorOpnd *GenXKernelBuilder::createAddressOperand(Value *V, bool IsDst) {
  VISA_VectorOpnd *ResultOperand = nullptr;
  Register *Reg = getRegForValueAndSaveAlias(V, DONTCARESIGNED);
  IGC_ASSERT(Reg->Category == vc::RegCategory::Address);
  unsigned Width = 1;
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(V->getType()))
    Width = VT->getNumElements();
  if (IsDst) {
    CISA_CALL(Kernel->CreateVISAAddressDstOperand(
        ResultOperand, Reg->GetVar<VISA_AddrVar>(Kernel), 0));
  } else {
    CISA_CALL(Kernel->CreateVISAAddressSrcOperand(
        ResultOperand, Reg->GetVar<VISA_AddrVar>(Kernel), 0, Width));
  }
  return ResultOperand;
}

VISA_Type GenXKernelBuilder::getVISAImmTy(uint8_t ImmTy) {
  return static_cast<VISA_Type>(ImmTy & 0xf);
}

VISA_VectorOpnd *GenXKernelBuilder::createImmediateOperand(Constant *V,
                                                           Signedness Signed) {
  if (isDerivedFromUndef(V))
    V = Constant::getNullValue(V->getType());

  Type *T = V->getType();
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(T)) {
    // Vector constant.
    auto Splat = IGCLLVM::Constant::getSplatValue(V, /* AllowUndefs */ true);
    if (!Splat) {
      // Non-splatted vector constant. Must be a packed vector.
      unsigned NumElements = VT->getNumElements();
      if (VT->getElementType()->isIntegerTy()) {
        // Packed int vector.
        IGC_ASSERT(NumElements <= ImmIntVec::Width);
        unsigned Packed = 0;
        for (unsigned i = 0; i != NumElements; ++i) {
          auto El = dyn_cast<ConstantInt>(V->getAggregateElement(i));
          if (!El)
            continue; // undef element
          int This = El->getSExtValue();
          if (This < ImmIntVec::MinUInt) {
            IGC_ASSERT_MESSAGE(This >= ImmIntVec::MinSInt,
              "too big imm, cannot encode as vector imm");
            Signed = SIGNED;
          } else if (This > ImmIntVec::MaxSInt) {
            IGC_ASSERT_MESSAGE(This <= ImmIntVec::MaxUInt,
              "too big imm, cannot encode as vector imm");
            Signed = UNSIGNED;
          }
          Packed |= (This & ImmIntVec::MaxUInt) << (ImmIntVec::ElemSize * i);
        }
        // For a 2- or 4-wide operand, we need to repeat the vector elements
        // as which ones are used depends on the position of the other
        // operand in its oword.
        switch (NumElements) {
        case 2:
          Packed = Packed * 0x01010101;
          break;
        case 4:
          Packed = Packed * 0x00010001;
          break;
        }
        auto ImmTy =
            static_cast<uint8_t>(Signed == UNSIGNED ? ISA_TYPE_UV : ISA_TYPE_V);
        auto VISAImmTy = getVISAImmTy(ImmTy);
        VISA_VectorOpnd *ImmOp = nullptr;
        CISA_CALL(Kernel->CreateVISAImmediate(ImmOp, &Packed, VISAImmTy));
        return ImmOp;
      }
      // Packed float vector.
      IGC_ASSERT(VT->getElementType()->isFloatTy());
      IGC_ASSERT(NumElements == 1 || NumElements == 2 || NumElements == 4);
      unsigned Packed = 0;
      for (unsigned i = 0; i != 4; ++i) {
        auto CFP =
            dyn_cast<ConstantFP>(V->getAggregateElement(i % NumElements));
        if (!CFP) // Undef
          continue;
        const APFloat &FP = CFP->getValueAPF();
        Packed |= get8bitPackedFloat(FP.convertToFloat()) << (i * 8);
      }
      auto VISAImmTy = getVISAImmTy(ISA_TYPE_VF);
      VISA_VectorOpnd *ImmOp = nullptr;
      CISA_CALL(Kernel->CreateVISAImmediate(ImmOp, &Packed, VISAImmTy));
      return ImmOp;
    }
    // Splatted (or single element) vector. Use the scalar value.
    T = VT->getElementType();
    V = Splat;
  }

  if (isDerivedFromUndef(V))
    V = Constant::getNullValue(V->getType());
  else if (isa<ConstantPointerNull>(V)) {
    T = DL.getIntPtrType(V->getType());
    V = Constant::getNullValue(T);
  }

  // We have a scalar constant.
  if (IntegerType *IT = dyn_cast<IntegerType>(T)) {
    ConstantInt *CI = cast<ConstantInt>(V);
    // I think we need to use the appropriate one of getZExtValue or
    // getSExtValue to avoid an assertion failure on very large 64 bit values...
    int64_t Val = Signed == UNSIGNED ? CI->getZExtValue() : CI->getSExtValue();
    visa::TypeDetails TD(DL, IT, Signed);
    VISA_VectorOpnd *ImmOp = nullptr;
    CISA_CALL(
        Kernel->CreateVISAImmediate(ImmOp, &Val, getVISAImmTy(TD.VisaType)));
    return ImmOp;
  } if (isa<Function>(V)) {
    IGC_ASSERT_MESSAGE(0, "Not baled function address");
    return nullptr;
  } else {
    VISA_VectorOpnd *ImmOp = nullptr;
    ConstantFP *CF = cast<ConstantFP>(V);
    if (T->isFloatTy()) {
      union {
        float f;
        uint32_t i;
      } Val;
      Val.f = CF->getValueAPF().convertToFloat();
      auto VISAImmTy = getVISAImmTy(ISA_TYPE_F);
      CISA_CALL(Kernel->CreateVISAImmediate(ImmOp, &Val.i, VISAImmTy));
    } else if (T->isHalfTy()) {
      uint16_t Val(
          (uint16_t)(CF->getValueAPF().bitcastToAPInt().getZExtValue()));
      auto VISAImmTy = getVISAImmTy(ISA_TYPE_HF);
      auto Val32 = static_cast<uint32_t>(Val);
      CISA_CALL(Kernel->CreateVISAImmediate(ImmOp, &Val32, VISAImmTy));
    } else {
      IGC_ASSERT(T->isDoubleTy());
      union {
        double f;
        uint64_t i;
      } Val;
      Val.f = CF->getValueAPF().convertToDouble();
      auto VISAImmTy = getVISAImmTy(ISA_TYPE_DF);
      CISA_CALL(Kernel->CreateVISAImmediate(ImmOp, &Val.i, VISAImmTy));
    }
    return ImmOp;
  }
}

/***********************************************************************
 * getOriginalInstructionForSource : trace a source operand back through
 *     its bale (if any), given a starting instruction.
 *
 * Enter:   Inst = The instruction to start tracing from.
 *          BI = BaleInfo for Inst
 */
Instruction *
GenXKernelBuilder::getOriginalInstructionForSource(Instruction *Inst,
                                                   BaleInfo BI) {
  while (!isa<Constant>(Inst->getOperand(0)) && BI.isOperandBaled(0)) {
    Inst = cast<Instruction>(Inst->getOperand(0));
    BI = Baling->getBaleInfo(Inst);
  }

  return Inst;
}

void GenXKernelBuilder::buildConvert(CallInst *CI, BaleInfo BI, unsigned Mod,
                                     const DstOpndDesc &DstDesc) {
  Register *DstReg = getRegForValueAndSaveAlias(CI, UNSIGNED);
  if (!isa<Constant>(CI->getOperand(0))) {
    Instruction *OrigInst = getOriginalInstructionForSource(CI, BI);
    Register *SrcReg = getRegForValueAndSaveAlias(OrigInst->getOperand(0));
    const bool SrcCategory = (SrcReg->Category != vc::RegCategory::General);
    const bool DstCategory = (DstReg->Category != vc::RegCategory::General);
    const bool Categories = (SrcCategory || DstCategory);
    IGC_ASSERT_MESSAGE(Categories, "expected a category conversion");
    (void)Categories;
  }

  if (DstReg->Category != vc::RegCategory::Address) {
    // State copy.
    int ExecSize = 1;
    if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(CI->getType())) {
      ExecSize = VT->getNumElements();
    }

    auto ISAExecSize = static_cast<VISA_Exec_Size>(genx::log2(ExecSize));
    auto Dst = createDestination(CI, UNSIGNED, 0, DstDesc);
    auto Src = createSourceOperand(CI, UNSIGNED, 0, BI);
    CISA_CALL(Kernel->AppendVISADataMovementInst(
        ISA_MOVS, nullptr /*Pred*/, false /*Mod*/,
        NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1, ISAExecSize, Dst, Src));
    return;
  }

  // Destination is address register.
  int ExecSize = 1;
  if (VectorType *VT = dyn_cast<VectorType>(CI->getType())) {
    vc::fatal(getContext(), "GenXCisaBuilder",
              "vector of addresses not implemented", CI);
  }

  auto ISAExecSize = static_cast<VISA_Exec_Size>(genx::log2(ExecSize));
  Register *SrcReg = getRegForValueAndSaveAlias(CI->getOperand(0));
  IGC_ASSERT(SrcReg->Category == vc::RegCategory::Address);

  (void)SrcReg;
  // This is an address->address copy, inserted due to coalescing failure of
  // the address for an indirected arg in GenXArgIndirection.
  // (A conversion to address is handled in buildConvertAddr below.)
  // Write the addr_add instruction.
  Value *SrcOp0 = CI->getOperand(0);
  unsigned Src0Width = 1;
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(SrcOp0->getType()))
    Src0Width = VT->getNumElements();

  Register *RegDst = getRegForValueAndSaveAlias(CI, DONTCARESIGNED);
  Register *RegSrc0 = getRegForValueAndSaveAlias(SrcOp0, DONTCARESIGNED);

  VISA_VectorOpnd *Dst = nullptr, *Src0 = nullptr, *Src1 = nullptr;

  CISA_CALL(Kernel->CreateVISAAddressDstOperand(
      Dst, RegDst->GetVar<VISA_AddrVar>(Kernel), 0));
  CISA_CALL(Kernel->CreateVISAAddressSrcOperand(
      Src0, RegSrc0->GetVar<VISA_AddrVar>(Kernel), 0, Src0Width));
  Src1 =
      createImmediateOperand(Constant::getNullValue(CI->getType()), UNSIGNED);

  CISA_CALL(Kernel->AppendVISAAddrAddInst(vISA_EMASK_M1_NM, ISAExecSize, Dst,
                                          Src0, Src1));
}

VISA_VectorOpnd *GenXKernelBuilder::createSource(CisaVariable *V,
                                                 Signedness Signed,
                                                 unsigned MaxWidth,
                                                 unsigned *Offset) {
  Region R(IGCLLVM::FixedVectorType::get(
      IntegerType::get(Ctx, CISATypeTable[V->getType()].typeSize * CHAR_BIT),
      V->getNumElements()));
  if (Offset)
    R.Offset = *Offset;
  return createRegionOperand(&R, V->getGenVar(), Signed, 0, false, MaxWidth);
}

VISA_VectorOpnd *GenXKernelBuilder::createSource(Value *V, Signedness Signed,
                                                 unsigned MaxWidth,
                                                 unsigned *Offset) {
  return createSource(V, Signed, false, 0, nullptr, MaxWidth, Offset);
}

VISA_VectorOpnd *GenXKernelBuilder::createSource(Value *V, Signedness Signed,
                                                 bool Baled, unsigned Mod,
                                                 Signedness *SignedRes,
                                                 unsigned MaxWidth,
                                                 unsigned *Offset, bool IsBF) {
  LLVM_DEBUG(dbgs() << "createSource for " << (Baled ? "baled" : "non-baled")
                    << (IsBF ? " brain" : " non-brain") << " value: ");
  LLVM_DEBUG(V->dump());
  LLVM_DEBUG(dbgs() << "\n");
  if (SignedRes)
    *SignedRes = Signed;
  if (auto C = dyn_cast<Constant>(V)) {
    if (Mod) {
      // Need to negate constant.
      IGC_ASSERT_MESSAGE(Mod == MODIFIER_NEG, "unexpected modifier");
      if (C->getType()->isIntOrIntVectorTy())
        C = ConstantExpr::getNeg(C);
      else
        C = ConstantExpr::getFNeg(C);
    }
    return createImmediateOperand(C, Signed);
  }
  if (!Baled) {
    Register *Reg = getRegForValueAndSaveAlias(V, Signed, nullptr, IsBF);
    IGC_ASSERT(Reg->Category == vc::RegCategory::General ||
               Reg->Category == vc::RegCategory::Surface ||
               Reg->Category == vc::RegCategory::Sampler);
    // Write the vISA general operand.
    Region R(V);
    if (Offset)
      R.Offset = *Offset;
    if (R.NumElements == 1)
      R.VStride = R.Stride = 0;
    if (SignedRes)
      *SignedRes = RegAlloc->getSigned(Reg);
    if (Reg->Category == vc::RegCategory::General) {
      return createRegionOperand(&R, Reg->GetVar<VISA_GenVar>(Kernel), Signed, Mod,
                                 false /*IsDst*/, MaxWidth);
    } else {
      return createState(Reg, R.Offset >> 2, false /*IsDst*/);
    };
  }

  Instruction *Inst = cast<Instruction>(V);
  BaleInfo BI(Baling->getBaleInfo(Inst));
  unsigned Idx = 0;
  switch (BI.Type) {
  case BaleInfo::RDREGION: {
    // The source operand has a rdregion baled in. We need to allow for the
    // case that there is no register allocated if it is an indirected arg,
    // and that is OK because the region is indirect so the vISA does not
    // contain the base register.
    Value *V = Inst->getOperand(0);
    Register *Reg = getRegForValueOrNullAndSaveAlias(V, Signed, nullptr, IsBF);

    // Ensure we pick a non-DONTCARESIGNED signedness here, as, for an
    // indirect region and DONTCARESIGNED, writeRegion arbitrarily picks a
    // signedness as it is attached to the operand, unlike a direct region
    // where it is attached to the vISA register.
    if (Reg)
      Signed = RegAlloc->getSigned(Reg);
    else if (Signed == DONTCARESIGNED)
      Signed = SIGNED;
    // Write the vISA general operand with region.
    Region R = makeRegionFromBaleInfo(Inst, Baling->getBaleInfo(Inst));
    if (Offset)
      R.Offset = *Offset;
    if (R.NumElements == 1)
      R.VStride = 0;
    if (R.Width == 1)
      R.Stride = 0;
    if (!Reg || Reg->Category == vc::RegCategory::General || R.Indirect) {
      if (SignedRes)
        *SignedRes = Signed;
      return createRegionOperand(&R, Reg ? Reg->GetVar<VISA_GenVar>(Kernel) : nullptr,
                                 Signed, Mod, false, MaxWidth);
    } else {
      if (SignedRes)
        *SignedRes = Signed;
      return createState(Reg, R.Offset >> 2, false /*IsDst*/);
    }
  }
  case BaleInfo::ABSMOD:
    Signed = SIGNED;
    Mod |= MODIFIER_ABS;
    break;
  case BaleInfo::NEGMOD:
    if (Inst->getOpcode() == Instruction::FNeg) {
      Mod ^= MODIFIER_NEG;
      break;
    }
    if (!(Mod & MODIFIER_ABS))
      Mod ^= MODIFIER_NEG;
    Idx = 1; // the input we want in "0-x" is x, not 0.
    break;
  case BaleInfo::NOTMOD:
    Mod ^= MODIFIER_NOT;
    break;
  case BaleInfo::ZEXT:
    Signed = UNSIGNED;
    break;
  case BaleInfo::SEXT:
    Signed = SIGNED;
    break;
  default:
    IGC_ASSERT_EXIT_MESSAGE(0, "unknown bale type");
    break;
  }
  return createSource(Inst->getOperand(Idx), Signed, BI.isOperandBaled(Idx),
                      Mod, SignedRes, MaxWidth);
}

static void diagnoseInlineAsm(llvm::LLVMContext &Context,
                              const Instruction *Inst, const std::string Suffix,
                              DiagnosticSeverity DS_type) {
  auto *CI = cast<CallInst>(Inst);
  const InlineAsm *IA = cast<InlineAsm>(IGCLLVM::getCalledValue(CI));
  const std::string Message = '"' + IA->getAsmString() + '"' + Suffix;
  vc::diagnose(Context, "GenXCisaBuilder", Message.c_str(), DS_type,
               vc::WarningName::Generic, Inst);
}

std::string GenXKernelBuilder::createInlineAsmOperand(
    const Instruction *Inst, Register *Reg, genx::Region *R, bool IsDst,
    genx::Signedness Signed, genx::ConstraintType Ty, unsigned Mod) {
  deduceRegion(R, IsDst);

  VISA_VectorOpnd *ResultOperand = nullptr;
  switch (Ty) {
  default: {
    diagnoseInlineAsm(getContext(), Inst,
                      " constraint incorrect in inline assembly", DS_Error);
    break;
  }
  case ConstraintType::Constraint_i: {
    diagnoseInlineAsm(
        getContext(), Inst,
        " immediate constraint in inline assembly was satisfied to value",
        DS_Warning);
    ResultOperand = createGeneralOperand(R, Reg->GetVar<VISA_GenVar>(Kernel),
                                         Signed, Mod, IsDst);
    break;
  }
  case ConstraintType::Constraint_cr: {
    IGC_ASSERT(Reg);
    IGC_ASSERT(Reg->Category == vc::RegCategory::Predicate);
    VISA_PredVar *PredVar = getPredicateVar(Reg);
    VISA_PredOpnd *PredOperand =
        createPredOperand(PredVar, PredState_NO_INVERSE, PRED_CTRL_NON);
    return Kernel->getPredicateOperandName(PredOperand);
  }
  case ConstraintType::Constraint_rw:
    return Kernel->getVarName(Reg->GetVar<VISA_GenVar>(Kernel));
  case ConstraintType::Constraint_r:
    ResultOperand =
        createGeneralOperand(R, Reg->GetVar<VISA_GenVar>(Kernel), Signed, Mod, IsDst);
    break;
  case ConstraintType::Constraint_a:
    if (R->Indirect)
      ResultOperand = createIndirectOperand(R, Signed, Mod, IsDst);
    else
      ResultOperand = createGeneralOperand(R, Reg->GetVar<VISA_GenVar>(Kernel),
                                           Signed, Mod, IsDst);
    break;
  }
  return Kernel->getVectorOperandName(ResultOperand, true);
}

std::string GenXKernelBuilder::createInlineAsmDestinationOperand(
    const Instruction *Inst, Value *Dest, genx::Signedness Signed,
    genx::ConstraintType Ty, unsigned Mod, const DstOpndDesc &DstDesc) {

  Type *OverrideType = nullptr;

  // Saturation can also change signedness.
  if (!Dest->user_empty() && GenXIntrinsic::isIntegerSat(Dest->user_back())) {
    Signed = getISatDstSign(Dest->user_back());
  }

  if (!DstDesc.WrRegion) {
    Register *Reg = getRegForValueAndSaveAlias(Dest, Signed, OverrideType);

    Region DestR(Dest);
    return createInlineAsmOperand(Inst, Reg, &DestR, true /*IsDst*/,
                                  DONTCARESIGNED, Ty, Mod);
  }
  // We need to allow for the case that there is no register allocated if it is
  // an indirected arg, and that is OK because the region is indirect so the
  // vISA does not contain the base register.
  Register *Reg;

  Value *V = nullptr;
  if (DstDesc.GStore) {
    auto *GV = vc::getUnderlyingGlobalVariable(DstDesc.GStore->getOperand(1));
    IGC_ASSERT_MESSAGE(GV, "out of sync");
    if (OverrideType == nullptr)
      OverrideType = DstDesc.GStore->getOperand(0)->getType();
    Reg = getRegForValueAndSaveAlias(GV, Signed, OverrideType);
    V = GV;
  } else {
    V = DstDesc.WrRegion;
    Reg = getRegForValueOrNullAndSaveAlias(V, Signed, OverrideType);
  }

  IGC_ASSERT(!Reg || Reg->Category == vc::RegCategory::General);

  // Write the vISA general operand with region:
  Region R = makeRegionFromBaleInfo(DstDesc.WrRegion, DstDesc.WrRegionBI);

  return createInlineAsmOperand(Inst, Reg, &R, true /*IsDst*/, Signed, Ty, Mod);
}

std::string GenXKernelBuilder::createInlineAsmSourceOperand(
    const Instruction *AsmInst, Value *V, genx::Signedness Signed, bool Baled,
    genx::ConstraintType Ty, unsigned Mod, unsigned MaxWidth) {

  if (auto C = dyn_cast<Constant>(V)) {
    if (Ty != genx::ConstraintType::Constraint_n) {
      if (Mod) {
        // Need to negate constant.
        IGC_ASSERT_MESSAGE(Mod == MODIFIER_NEG, "unexpected modifier");
        if (C->getType()->isIntOrIntVectorTy())
          C = ConstantExpr::getNeg(C);
        else
          C = ConstantExpr::getFNeg(C);
      }
      VISA_VectorOpnd *ImmOp = createImmediateOperand(C, Signed);
      return Kernel->getVectorOperandName(ImmOp, false);
    } else {
      ConstantInt *CI = cast<ConstantInt>(C);
      return llvm::to_string(CI->getSExtValue());
    }
  }

  if (!Baled) {
    Register *Reg = getRegForValueAndSaveAlias(V, Signed);
    Region R(V);
    if (R.NumElements == 1)
      R.VStride = R.Stride = 0;

    return createInlineAsmOperand(AsmInst, Reg, &R, false /*IsDst*/, Signed, Ty,
                                  Mod);
  }

  Instruction *Inst = cast<Instruction>(V);
  BaleInfo BI(Baling->getBaleInfo(Inst));
  IGC_ASSERT(BI.Type == BaleInfo::RDREGION);
  // The source operand has a rdregion baled in. We need to allow for the
  // case that there is no register allocated if it is an indirected arg,
  // and that is OK because the region is indirect so the vISA does not
  // contain the base register.
  V = Inst->getOperand(0);
  Register *Reg = getRegForValueAndSaveAlias(V, Signed);

  // Ensure we pick a non-DONTCARESIGNED signedness here, as, for an
  // indirect region and DONTCARESIGNED, writeRegion arbitrarily picks a
  // signedness as it is attached to the operand, unlike a direct region
  // where it is attached to the vISA register.
  if (Signed == DONTCARESIGNED)
    Signed = SIGNED;
  // Write the vISA general operand with region.
  Region R = makeRegionFromBaleInfo(Inst, Baling->getBaleInfo(Inst));
  if (R.NumElements == 1)
    R.VStride = 0;
  if (R.Width == 1)
    R.Stride = 0;

  IGC_ASSERT(Reg->Category == vc::RegCategory::General || R.Indirect);

  return createInlineAsmOperand(Inst, Reg, &R, false /*IsDst*/, Signed, Ty,
                                Mod);
}

/***********************************************************************
 * getPredicateVar : get predicate var from value
 */
VISA_PredVar *GenXKernelBuilder::getPredicateVar(Value *V) {
  auto Reg = getRegForValueAndSaveAlias(V, DONTCARESIGNED);
  IGC_ASSERT(Reg);
  IGC_ASSERT(Reg->Category == vc::RegCategory::Predicate);
  return getPredicateVar(Reg);
}

/***********************************************************************
 * getZeroedPredicateVar : get predicate var from value with zeroing it
 */
VISA_PredVar *GenXKernelBuilder::getZeroedPredicateVar(Value *V) {
  auto Reg = getRegForValueAndSaveAlias(V, DONTCARESIGNED);
  IGC_ASSERT(Reg);
  IGC_ASSERT(Reg->Category == vc::RegCategory::Predicate);
  auto PredVar = getPredicateVar(Reg);
  unsigned Size = V->getType()->getPrimitiveSizeInBits();
  auto C = Constant::getNullValue(V->getType());
  CISA_CALL(Kernel->AppendVISASetP(
    vISA_EMASK_M1_NM, VISA_Exec_Size(genx::log2(Size)),
    PredVar, createImmediateOperand(C, DONTCARESIGNED)));

  return PredVar;
}

/***********************************************************************
 * getPredicateVar : get predicate var from register
 */
VISA_PredVar *GenXKernelBuilder::getPredicateVar(Register *R) {
  IGC_ASSERT(R);
  return R->Num >= visa::VISA_NUM_RESERVED_PREDICATES
             ? R->GetVar<VISA_PredVar>(Kernel)
             : nullptr;
}

void GenXKernelBuilder::buildSelectInst(SelectInst *SI, BaleInfo BI,
                                        unsigned Mod,
                                        const DstOpndDesc &DstDesc) {
  unsigned ExecSize = 1;
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(SI->getType()))
    ExecSize = VT->getNumElements();
  // Get the predicate (mask) operand, scanning through baled in
  // all/any/not/rdpredregion and setting PredField and MaskCtrl
  // appropriately.
  VISA_EMask_Ctrl MaskCtrl;
  VISA_PREDICATE_CONTROL Control;
  VISA_PREDICATE_STATE State;

  VISA_PredVar *PredDecl =
      createPredicateDeclFromSelect(SI, BI, Control, State, &MaskCtrl);
  VISA_PredOpnd* PredOp = createPredOperand(PredDecl, State, Control);

  genx::Signedness DstSignedness = DONTCARESIGNED;
  genx::Signedness SrcSignedness = DONTCARESIGNED;

  if (Mod & MODIFIER_SAT) {
    IGC_ASSERT(SI->getNumUses() == 1);
    auto *UserCI = dyn_cast<CallInst>(*SI->user_begin());
    IGC_ASSERT(UserCI);

    auto ID = GenXIntrinsic::getGenXIntrinsicID(UserCI);
    switch (ID) {
    case GenXIntrinsic::genx_uutrunc_sat:
      DstSignedness = UNSIGNED;
      SrcSignedness = UNSIGNED;
      break;
    case GenXIntrinsic::genx_ustrunc_sat:
      DstSignedness = UNSIGNED;
      SrcSignedness = SIGNED;
      break;
    case GenXIntrinsic::genx_sutrunc_sat:
      DstSignedness = SIGNED;
      SrcSignedness = UNSIGNED;
      break;
    case GenXIntrinsic::genx_sstrunc_sat:
      DstSignedness = SIGNED;
      SrcSignedness = SIGNED;
      break;
    default:
      IGC_ASSERT_MESSAGE(0, "Unsupported saturation intrinsic");
    }
  }

  VISA_VectorOpnd *Dst = createDestination(SI, DstSignedness, Mod, DstDesc);
  VISA_VectorOpnd *Src0 = createSourceOperand(SI, SrcSignedness, 1, BI);
  VISA_VectorOpnd *Src1 = createSourceOperand(SI, SrcSignedness, 2, BI);

  CISA_CALL(Kernel->AppendVISADataMovementInst(
      ISA_SEL, PredOp, Mod & MODIFIER_SAT, MaskCtrl,
      getExecSizeFromValue(ExecSize), Dst, Src0, Src1));
}

void GenXKernelBuilder::buildNoopCast(CastInst *CI, genx::BaleInfo BI,
                                      unsigned Mod, const DstOpndDesc &DstDesc) {
  IGC_ASSERT_MESSAGE(isMaskPacking(CI) || !BI.Bits,
    "non predicate bitcast should not be baled with anything");
  IGC_ASSERT_MESSAGE(isMaskPacking(CI) || !Mod,
    "non predicate bitcast should not be baled with anything");
  IGC_ASSERT_MESSAGE(isMaskPacking(CI) || !DstDesc.WrRegion,
    "non predicate bitcast should not be baled with anything");

  // ignore bitcasts of volatile globals
  // (they used to be a part of load/store as a constexpr)
  if ((isa<GlobalVariable>(CI->getOperand(0)) &&
       cast<GlobalVariable>(CI->getOperand(0))
           ->hasAttribute(VCModuleMD::VCVolatile)))
    return;

  if (CI->getType()->getScalarType()->isIntegerTy(1)) {
    if (CI->getOperand(0)->getType()->getScalarType()->isIntegerTy(1)) {
      if (auto C = dyn_cast<Constant>(CI->getOperand(0))) {
        auto Reg = getRegForValueOrNullAndSaveAlias(CI, DONTCARESIGNED);
        if (!Reg)
          return; // write to EM/RM value, ignore
        // We can move a constant predicate to a predicate register
        // using setp, if we get the constant predicate as a single int.
        unsigned IntVal = getPredicateConstantAsInt(C);
        unsigned Size = C->getType()->getPrimitiveSizeInBits();
        C = ConstantInt::get(
            Type::getIntNTy(CI->getContext(), std::max(Size, 8U)), IntVal);

        CISA_CALL(Kernel->AppendVISASetP(
            vISA_EMASK_M1_NM, VISA_Exec_Size(genx::log2(Size)),
            getPredicateVar(Reg), createSourceOperand(CI, UNSIGNED, 0, BI)));
        return;
      }
      // There does not appear to be a vISA instruction to move predicate
      // to predicate. GenXCoalescing avoids this by moving in two steps
      // via a general register. So the only pred->pred bitcast that arrives
      // here should be one from GenXLowering, and it should have been copy
      // coalesced in GenXCoalescing.
      const Register *const Reg1 =
          getRegForValueAndSaveAlias(CI, DONTCARESIGNED);
      const Register *const Reg2 =
          getRegForValueAndSaveAlias(CI->getOperand(0), DONTCARESIGNED);
      IGC_ASSERT_MESSAGE(Reg1 == Reg2, "uncoalesced phi move of predicate");
      (void) Reg1;
      (void) Reg2;
      return;
    }

    VISA_PredVar *PredVar = getPredicateVar(CI);

    CISA_CALL(Kernel->AppendVISASetP(
        vISA_EMASK_M1_NM,
        VISA_Exec_Size(
            genx::log2(CI->getType()->getPrimitiveSizeInBits())),
        PredVar, createSourceOperand(CI, UNSIGNED, 0, BI)));
    return;
  }
  if (isa<Constant>(CI->getOperand(0))) {
    if (isa<UndefValue>(CI->getOperand(0)))
      return; // undef source, generate no code
    // Source is constant.
    int ExecSize = 1;
    if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(CI->getType()))
      ExecSize = VT->getNumElements();

    VISA_EMask_Ctrl ctrlMask = getExecMaskFromWrRegion(DstDesc);
    VISA_Exec_Size execSize = getExecSizeFromValue(ExecSize);
    CISA_CALL(Kernel->AppendVISADataMovementInst(
        ISA_MOV, createPredFromWrRegion(DstDesc), Mod & MODIFIER_SAT, ctrlMask,
        execSize, createDestination(CI, DONTCARESIGNED, Mod, DstDesc),
        createSourceOperand(CI, DONTCARESIGNED, 0, BI)));
    return;
  }
  if (CI->getOperand(0)->getType()->getScalarType()->isIntegerTy(1)) {
    // Bitcast from predicate to scalar int
    Register *PredReg =
        getRegForValueAndSaveAlias(CI->getOperand(0), DONTCARESIGNED);
    IGC_ASSERT(PredReg->Category == vc::RegCategory::Predicate);
    CISA_CALL(Kernel->AppendVISAPredicateMove(
        createDestination(CI, UNSIGNED, 0, DstDesc),
        PredReg->GetVar<VISA_PredVar>(Kernel)));

    return;
  }

  if (Liveness->isNoopCastCoalesced(CI))
    return; // cast was coalesced away

  // Here we always choose minimal (in size) type in order to avoid issues
  // with alignment. We expect that execution size should still be valid
  Type *Ty = CI->getSrcTy();
  if (Ty->getScalarType()->getPrimitiveSizeInBits() >
      CI->getDestTy()->getScalarType()->getPrimitiveSizeInBits())
    Ty = CI->getDestTy();

  Register *DstReg = getRegForValueAndSaveAlias(CI, DONTCARESIGNED, Ty);
  // Give dest and source the same signedness for byte mov.
  auto Signed = RegAlloc->getSigned(DstReg);
  Register *SrcReg = getRegForValueAndSaveAlias(CI->getOperand(0), Signed, Ty);
  VISA_Exec_Size ExecSize = EXEC_SIZE_1;
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(Ty))
    ExecSize = getExecSizeFromValue(VT->getNumElements());
  IGC_ASSERT_MESSAGE(ExecSize >= EXEC_SIZE_1,
    "illegal exec size in bitcast: should have been coalesced away");
  IGC_ASSERT_MESSAGE(ExecSize <= EXEC_SIZE_32,
    "illegal exec size in bitcast: should have been coalesced away");
  // destination
  Region DestR(CI);
  // source
  Region SourceR(CI->getOperand(0));

  VISA_EMask_Ctrl ctrlMask = NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
  CISA_CALL(Kernel->AppendVISADataMovementInst(
      ISA_MOV, nullptr, Mod, ctrlMask, ExecSize,
      createRegionOperand(&DestR, DstReg->GetVar<VISA_GenVar>(Kernel), DONTCARESIGNED,
                          0, true),
      createRegionOperand(&SourceR, SrcReg->GetVar<VISA_GenVar>(Kernel), Signed, 0,
                          false)));
}

void GenXKernelBuilder::buildFunctionAddr(Instruction *Inst,
                                          const DstOpndDesc &DstDesc) {
  auto *CI = dyn_cast<CallInst>(Inst);
  IGC_ASSERT(CI);
  IGC_ASSERT_MESSAGE(GenXIntrinsic::getGenXIntrinsicID(CI) == GenXIntrinsic::genx_faddr,
    "genx.faddr expected in a FADDR bale");
  auto *Dst = createDestination(Inst, DONTCARESIGNED, MODIFIER_NONE, DstDesc);
  IGC_ASSERT(Dst);
  auto *F = cast<Function>(Inst->getOperand(0));
  CISA_CALL(Kernel->AppendVISACFSymbolInst(F->getName().str(), Dst));
}

/***********************************************************************
 * buildLoneWrRegion : build a lone wrregion
 */
void GenXKernelBuilder::buildLoneWrRegion(const DstOpndDesc &DstDesc) {
  enum { OperandNum = 1 };
  Value *Input = DstDesc.WrRegion->getOperand(OperandNum);
  if (isa<UndefValue>(Input))
    return; // No code if input is undef
  VISA_Exec_Size ExecSize = EXEC_SIZE_1;
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(Input->getType()))
    ExecSize = getExecSizeFromValue(VT->getNumElements());

  VISA_EMask_Ctrl ExecMask = getExecMaskFromWrRegion(DstDesc);

  // TODO: fix signedness of the source
  auto *Src = createSource(Input, DONTCARESIGNED, false, 0);
  auto *Dst = createDestination(Input, DONTCARESIGNED, 0, DstDesc);
  CISA_CALL(Kernel->AppendVISADataMovementInst(
      ISA_MOV, createPredFromWrRegion(DstDesc), false, ExecMask, ExecSize,
      Dst, Src));
}

/***********************************************************************
 * buildLoneWrPredRegion : build a lone wrpredregion
 */
void GenXKernelBuilder::buildLoneWrPredRegion(Instruction *Inst, BaleInfo BI) {
  IGC_ASSERT_MESSAGE(isWrPredRegionLegalSetP(*cast<CallInst>(Inst)),
    "wrpredregion cannot be legally represented as SETP instruction");
  enum { OperandNum = 1 };
  Value *Input = Inst->getOperand(OperandNum);
  IGC_ASSERT_MESSAGE(isa<Constant>(Input), "only immediate case is supported");
  auto *C = cast<Constant>(Input);
  unsigned Size = C->getType()->getPrimitiveSizeInBits();

  VISA_EMask_Ctrl ctrlMask = getExecMaskFromWrPredRegion(Inst, true);
  VISA_Exec_Size execSize = getExecSizeFromValue(Size);

  unsigned IntVal = getPredicateConstantAsInt(C);
  C = ConstantInt::get(Type::getIntNTy(Inst->getContext(), std::max(Size, 8U)),
                       IntVal);
  CISA_CALL(Kernel->AppendVISASetP(ctrlMask, execSize, getPredicateVar(Inst),
                                   createImmediateOperand(C, UNSIGNED)));
}

/***********************************************************************
 * buildLoneOperand : build a rdregion or modifier that is not baled in to
 *                    a main instruction
 *
 * Enter:   Inst = the rdregion or modifier instruction
 *          BI = BaleInfo for Inst
 *          Mod = modifier for destination
 *          WrRegion = 0 else wrregion for destination
 *          WrRegionBI = BaleInfo for WrRegion (possibly baling in
 *              variable index add)
 */
void GenXKernelBuilder::buildLoneOperand(Instruction *Inst, genx::BaleInfo BI,
                                         unsigned Mod,
                                         const DstOpndDesc &DstDesc) {
  Instruction *WrRegion = DstDesc.WrRegion;
  BaleInfo WrRegionBI = DstDesc.WrRegionBI;

  VISA_Exec_Size ExecSize = EXEC_SIZE_1;
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(Inst->getType()))
    ExecSize = getExecSizeFromValue(VT->getNumElements());
  ISA_Opcode Opcode = ISA_MOV;
  bool Baled = true;
  VISA_EMask_Ctrl ExecMask = getExecMaskFromWrRegion(DstDesc);
  // Default source from Inst
  Value *Src = Inst;

  // Give dest and source the same signedness for byte mov.
  auto Signed = DONTCARESIGNED;
  // destination
  auto Dest = createDestination(Inst, Signed, Mod, DstDesc, &Signed);

  // source
  if ((Mod & MODIFIER_SAT) != 0 &&
      Inst->getType()->getScalarType()->isIntegerTy() &&
         GenXIntrinsic::isIntegerSat(Inst->user_back()))
    Signed = getISatSrcSign(Inst->user_back());

  if (BI.Type == BaleInfo::NOTMOD) {
    // A lone "not" is implemented as a not instruction, rather than a mov
    // with a not modifier. A mov only allows an arithmetic modifier.
    Opcode = ISA_NOT;
    Baled = BI.isOperandBaled(0);
    // In this case the src is actually operand 0 of the noti intrinsic
    Src = Inst->getOperand(0);
  } else if (BI.Type == BaleInfo::RDREGION && !Mod) {
    Register *DstReg;
    if (WrRegion) {
      DstReg = getRegForValueOrNullAndSaveAlias(WrRegion, DONTCARESIGNED);
    } else {
      DstReg = getRegForValueAndSaveAlias(Inst, DONTCARESIGNED);
    }
    if (DstReg && (DstReg->Category == vc::RegCategory::Surface ||
                   DstReg->Category == vc::RegCategory::Sampler)) {
      Opcode = ISA_MOVS;
    }
  }
  // TODO: mb need to get signed from dest for src and then modify that
  CISA_CALL(Kernel->AppendVISADataMovementInst(
      Opcode, (Opcode != ISA_MOVS ? createPredFromWrRegion(DstDesc) : nullptr),
      Mod & MODIFIER_SAT, ExecMask, ExecSize, Dest,
      createSource(Src, Signed, Baled, 0)));
}

// FIXME: use vc::TypeSizeWrapper instead.
static unsigned getResultedTypeSize(Type *Ty, const DataLayout& DL) {
  unsigned TySz = 0;
  if (auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty))
    TySz =
        VTy->getNumElements() * getResultedTypeSize(VTy->getElementType(), DL);
  else if (Ty->isArrayTy())
    TySz = Ty->getArrayNumElements() *
           getResultedTypeSize(Ty->getArrayElementType(), DL);
  else if (Ty->isStructTy()) {
    StructType *STy = dyn_cast<StructType>(Ty);
    IGC_ASSERT(STy);
    for (Type *Ty : STy->elements())
      TySz += getResultedTypeSize(Ty, DL);
  } else if (Ty->isPointerTy())
    // FIXME: fix data layout description.
    TySz = IGCLLVM::getNonOpaquePtrEltTy(Ty)->isFunctionTy() ? genx::DWordBytes
                                                       : DL.getPointerSize();
  else {
    TySz = Ty->getPrimitiveSizeInBits() / CHAR_BIT;
    IGC_ASSERT_MESSAGE(TySz, "Ty is not primitive?");
  }

  return TySz;
}

// Check if we're trying to form return value of a structure type
// TODO:  should check full insert/extract chain (for failed coalescing cases),
//        e.g. after failed coalescing we may end up having a bunch of
//        extractvalue, insertvalue and bitcasts inst where only the last one
//        should be actually lowered
static bool checkInsertToRetv(InsertValueInst *Inst) {
  if (auto IVI = dyn_cast<InsertValueInst>(Inst->use_begin()->getUser()))
    return checkInsertToRetv(IVI);

  if (auto RI = dyn_cast<ReturnInst>(Inst->use_begin()->getUser())) {
    const auto *F = RI->getFunction();
    return vc::requiresStackCall(F);
  }

  return false;
}

/***********************************************************************
 * buildMainInst : build a main instruction
 *
 * Enter:   Inst = the main instruction
 *          BI = BaleInfo for Inst
 *          Mod = modifier bits for destination
 *          WrRegion = 0 else wrregion for destination
 *          WrRegionBI = BaleInfo for WrRegion (possibly baling in
 *              variable index add)
 *
 * Return:  true if terminator inst that falls through to following block
 */
bool GenXKernelBuilder::buildMainInst(Instruction *Inst, BaleInfo BI,
                                      unsigned Mod,
                                      const DstOpndDesc &DstDesc) {
  if (PHINode *Phi = dyn_cast<PHINode>(Inst))
    buildPhiNode(Phi);
  else if (ReturnInst *RI = dyn_cast<ReturnInst>(Inst)) {
    buildRet(RI);
  } else if (BranchInst *BR = dyn_cast<BranchInst>(Inst)) {
    return buildBranch(BR);
  } else if (IndirectBrInst *IBR = dyn_cast<IndirectBrInst>(Inst)) {
    buildIndirectBr(IBR);
  } else if (CmpInst *Cmp = dyn_cast<CmpInst>(Inst)) {
    buildCmp(Cmp, BI, DstDesc);
  } else if (BinaryOperator *BO = dyn_cast<BinaryOperator>(Inst)) {
    if (!BO->getType()->getScalarType()->isIntegerTy(1)) {
      buildBinaryOperator(BO, BI, Mod, DstDesc);
    } else {
      IGC_ASSERT(!Mod);
      IGC_ASSERT(!DstDesc.WrRegion);
      IGC_ASSERT(!BI.isOperandBaled(0));
      IGC_ASSERT(!BI.isOperandBaled(1));
      buildBoolBinaryOperator(BO);
    }
  } else if (auto EVI = dyn_cast<ExtractValueInst>(Inst)) {
    if (auto *CI = dyn_cast<CallInst>(Inst->getOperand(0)))
      // translate extraction of structured type from retv
      if (!UseNewStackBuilder && !CI->isInlineAsm() &&
          (vc::requiresStackCall(CI->getCalledFunction()) ||
           CI->isIndirectCall()))
        buildExtractRetv(EVI);
    // no code generated
  } else if (auto IVI = dyn_cast<InsertValueInst>(Inst)) {
    if (!UseNewStackBuilder && checkInsertToRetv(IVI) &&
        // TODO: safely remove this tmp workaround for failed coalescing cases
        // and insert-extract-insert chains
        !isa<BitCastInst>(Inst->getOperand(1)))
      RetvInserts.push_back(IVI);
    // no code generated
  } else if (CastInst *CI = dyn_cast<CastInst>(Inst)) {
    if (genx::isNoopCast(CI))
      buildNoopCast(CI, BI, Mod, DstDesc);
    else
      buildCastInst(CI, BI, Mod, DstDesc);
  } else if (auto SI = dyn_cast<SelectInst>(Inst)) {
    buildSelectInst(SI, BI, Mod, DstDesc);
  } else if (auto LI = dyn_cast<LoadInst>(Inst)) {
    (void)LI; // no code generated
  } else if (auto GEPI = dyn_cast<GetElementPtrInst>(Inst)) {
    // Skip vc.internal.print.format.index GEP here.
    IGC_ASSERT_MESSAGE(
        vc::isLegalPrintFormatIndexGEP(*GEPI),
        "only vc.internal.print.format.index src GEP can still be "
        "present at this stage");
  } else if (UnaryOperator *UO = dyn_cast<UnaryOperator>(Inst)) {
    buildUnaryOperator(UO, BI, Mod, DstDesc);
  } else if (auto *CI = dyn_cast<CallInst>(Inst)) {
    if (CI->isInlineAsm())
      buildInlineAsm(CI);
    else if (CI->isIndirectCall()) {
      IGC_ASSERT_MESSAGE(!Mod,
        "cannot bale subroutine call into anything");
      IGC_ASSERT_MESSAGE(!DstDesc.WrRegion,
        "cannot bale subroutine call into anything");
      buildCall(CI, DstDesc);
    } else {
      Function *Callee = CI->getCalledFunction();
      unsigned IntrinID = vc::getAnyIntrinsicID(Callee);
      switch (IntrinID) {
      case Intrinsic::dbg_value:
      case Intrinsic::dbg_declare:
      case GenXIntrinsic::genx_predefined_surface:
      case GenXIntrinsic::genx_output:
      case GenXIntrinsic::genx_output_1:
      case vc::InternalIntrinsic::jump_table:
        // ignore
        break;
      case GenXIntrinsic::genx_simdcf_goto:
        // A goto that is not baled into a branch (via an extractvalue)
        buildGoto(CI, nullptr);
        break;
      case GenXIntrinsic::genx_simdcf_join:
        // A join that is not baled into a branch (via an extractvalue)
        buildJoin(CI, nullptr);
        break;
      case GenXIntrinsic::genx_convert:
        buildConvert(CI, BI, Mod, DstDesc);
        break;
      case vc::InternalIntrinsic::print_format_index:
        buildPrintIndex(CI, IntrinID, Mod, DstDesc);
        break;
      case GenXIntrinsic::genx_convert_addr:
        buildConvertAddr(CI, BI, Mod, DstDesc);
        break;
      case GenXIntrinsic::genx_alloca:
        if (!UseNewStackBuilder)
          buildAlloca(CI, IntrinID, Mod, DstDesc);
        break;
      case GenXIntrinsic::genx_gaddr:
        buildSymbolInst(CI, Mod, DstDesc);
        break;
      case vc::InternalIntrinsic::read_variable_region:
        buildLoneReadVariableRegion(*CI);
        break;
      case vc::InternalIntrinsic::write_variable_region:
        buildLoneWriteVariableRegion(*CI);
        break;
      case GenXIntrinsic::genx_write_predef_surface:
        buildWritePredefSurface(*CI);
        break;
      case GenXIntrinsic::genx_get_hwid:
        IGC_ASSERT_MESSAGE(0, "genx_get_hwid should be lowered earlier");
        break;
      case GenXIntrinsic::genx_constanti:
      case GenXIntrinsic::genx_constantf:
      case GenXIntrinsic::genx_constantpred:
        if (isa<UndefValue>(CI->getOperand(0)))
          return false; // Omit llvm.genx.constant with undef operand.
        if (!DstDesc.WrRegion && !getRegForValueOrNullAndSaveAlias(CI))
          return false; // Omit llvm.genx.constantpred that is EM or RM and so
                        // does not have a register allocated.
                        // fall through...
      default:
        if (!(CI->user_empty() &&
              vc::getAnyIntrinsicID(CI->getCalledFunction()) ==
                  GenXIntrinsic::genx_any))
          buildIntrinsic(CI, IntrinID, BI, Mod, DstDesc);
          if (vc::getAnyIntrinsicID(CI->getCalledFunction()) == GenXIntrinsic::genx_simdcf_get_em)
            HasSimdCF = true;
        break;
      case GenXIntrinsic::not_any_intrinsic:
        IGC_ASSERT_MESSAGE(!Mod,
          "cannot bale subroutine call into anything");
        IGC_ASSERT_MESSAGE(!DstDesc.WrRegion,
          "cannot bale subroutine call into anything");
        buildCall(CI, DstDesc);
        break;
      }
    }
  } else if (isa<UnreachableInst>(Inst))
    ; // no code generated
  else {
    vc::diagnose(getContext(), "GenXCisaBuilder", "main inst not implemented",
                 Inst);
  }

  return false;
}

/***********************************************************************
 * buildPhiNode : build code for a phi node
 *
 * A phi node generates no code because coalescing has ensured that all
 * incomings and the result are in the same register. This function just
 * asserts that that is the case.
 */
void GenXKernelBuilder::buildPhiNode(PHINode *Phi) {
  IGC_ASSERT(testPhiNodeHasNoMismatchedRegs(Phi, Liveness));
}

/***********************************************************************
 * buildGoto : translate a goto
 *
 * Enter:   Goto = goto instruction that is baled into an extractvalue of
 *                 field 2 (the !any(EM) value), that is baled into Branch
 *          Branch = branch instruction, 0 if this is a goto that is not
 *                   baled into a branch, which happens when the goto is
 *                   followed by a join point so the goto's JIP points there,
 *                   and LLVM changes the resulting conditional branch with
 *                   both successors the same into an unconditional branch
 */
void GenXKernelBuilder::buildGoto(CallInst *Goto, BranchInst *Branch) {
  // GenXSimdCFConformance and GenXTidyControlFlow ensure that we have either
  // 1. a forward goto, where the false successor is fallthrough; or
  // 2. a backward goto, where the UIP (the join whose RM the goto updates)
  //    and the true successor are both fallthrough, and the false successor
  //    is the top of the loop.
  // (1) generates a vISA forward goto, but the condition has the wrong sense
  // so we need to invert it.
  // (2) generates a vISA backward goto.
  HasSimdCF = true;
  Value *BranchTarget = nullptr;
  VISA_PREDICATE_STATE StateInvert = PredState_NO_INVERSE;
  if (!Branch ||
      Branch->getSuccessor(1) == Branch->getParent()->getNextNode()) {
    // Forward goto.  Find the join.
    auto Join = GotoJoin::findJoin(Goto);
    IGC_ASSERT_MESSAGE(Join, "join not found");
    BranchTarget = Join;
    StateInvert = PredState_INVERSE;
  } else {
    IGC_ASSERT_MESSAGE(Branch->getSuccessor(0) == Branch->getParent()->getNextNode(),
      "bad goto structure");
    // Backward branch.
    BranchTarget = Branch->getSuccessor(1);
  }
  // Get the condition.
  VISA_EMask_Ctrl Mask = vISA_EMASK_M1;
  VISA_PREDICATE_CONTROL Control = PRED_CTRL_NON;
  VISA_PREDICATE_STATE State = PredState_NO_INVERSE;

  Value *Pred = getPredicateOperand(
      Goto, 2 /*OperandNum*/, Baling->getBaleInfo(Goto), Control, State, &Mask);
  IGC_ASSERT_MESSAGE(!Mask, "cannot have rdpredregion baled into goto");

  Instruction *Not = dyn_cast<Instruction>(Pred);
  if (Not && isPredNot(Not)) {
    // Eliminate excess NOT
    // %P1 = ...
    // %P2 = not %P1
    // (!%P2) goto
    // Transforms into
    // (%P1) goto
    StateInvert = (StateInvert == PredState_NO_INVERSE) ? PredState_INVERSE
                                                        : PredState_NO_INVERSE;
    Pred = getPredicateOperand(Not, 0 /*OperandNum*/, Baling->getBaleInfo(Not),
                               Control, State, &Mask);
    IGC_ASSERT_MESSAGE(!Mask, "cannot have rdpredregion baled into goto");
  }

  Register *PredReg = nullptr;
  if (auto C = dyn_cast<Constant>(Pred)) {
    (void)C;
    if (StateInvert)
      IGC_ASSERT_MESSAGE(C->isNullValue(),
        "predication operand must be constant 0 or not constant");
    else
      IGC_ASSERT_MESSAGE(C->isAllOnesValue(),
        "predication operand must be constant 1 or not constant");
  } else {
    State ^= StateInvert;
    PredReg = getRegForValueOrNullAndSaveAlias(Pred);
    IGC_ASSERT(PredReg);
    IGC_ASSERT(PredReg->Category == vc::RegCategory::Predicate);
  }

  uint8_t execSize = genx::log2(
      cast<IGCLLVM::FixedVectorType>(Pred->getType())->getNumElements());

  // Visa decoder part
  VISA_EMask_Ctrl emask =
      VISA_EMask_Ctrl((execSize >> 0x4) & 0xF);
  VISA_Exec_Size esize = (VISA_Exec_Size)((execSize)&0xF);

  VISA_PredOpnd *pred = nullptr;
  if (PredReg) {
    VISA_PredVar *Decl = getPredicateVar(PredReg);
    VISA_PredOpnd *opnd = createPredOperand(Decl, State, Control);
    pred = opnd;
  }

  unsigned LabelID = getOrCreateLabel(BranchTarget, LABEL_BLOCK);

  VISA_LabelOpnd *label = Labels[LabelID];
  CISA_CALL(Kernel->AppendVISACFGotoInst(pred, emask, esize, label));
}

// Convert predicate offset to EM offset according to
// vISA spec 3.3.1 Execution Mask.
static VISA_EMask_Ctrl getVisaEMOffset(unsigned PredOffset) {
  switch (PredOffset) {
  case 0:
    return vISA_EMASK_M1;
  case 4:
    return vISA_EMASK_M2;
  case 8:
    return vISA_EMASK_M3;
  case 12:
    return vISA_EMASK_M4;
  case 16:
    return vISA_EMASK_M5;
  case 20:
    return vISA_EMASK_M6;
  case 24:
    return vISA_EMASK_M7;
  case 28:
    return vISA_EMASK_M8;
  }
  IGC_ASSERT_UNREACHABLE(); // Unexpected EM offset
}

/***********************************************************************
 * getPredicateOperand : get predicate operand, scanning through any baled
 *    in rdpredregion, all, any, not instructions to derive the mask control
 *    field and the predication field
 *
 * Enter:   Inst = instruction to get predicate operand from
 *          OperandNum = operand number in Inst
 *          BI = bale info for Inst
 *          *Control = where to write control information about predicate
 *          *State = where to write state information about predicate
 *          *MaskCtrl = where to write mask control field (bits 7..4)
 *
 * Return:  Value of mask after scanning through baled in instructions
 *          *PredField and *MaskCtrl set
 */
Value *GenXKernelBuilder::getPredicateOperand(
    Instruction *Inst, unsigned OperandNum, BaleInfo BI,
    VISA_PREDICATE_CONTROL &Control, VISA_PREDICATE_STATE &State,
    VISA_EMask_Ctrl *MaskCtrl) {
  State = PredState_NO_INVERSE;
  *MaskCtrl = vISA_EMASK_M1;
  Control = PRED_CTRL_NON;
  Value *Mask = Inst->getOperand(OperandNum);
  // Check for baled in all/any/notp/rdpredregion.
  while (BI.isOperandBaled(OperandNum)) {
    Instruction *Inst = dyn_cast<Instruction>(Mask);
    if (isNot(Inst)) {
      if (Control != PRED_CTRL_NON) {
        // switch any<->all as well as invert bit
        Control ^= (VISA_PREDICATE_CONTROL)(PRED_CTRL_ANY | PRED_CTRL_ALL);
        State ^= PredState_INVERSE;
      } else {
        // all/any not set, just invert invert bit
        State ^= PredState_INVERSE;
      }
      OperandNum = 0;
      IGC_ASSERT(Inst);
      Mask = Inst->getOperand(OperandNum);
      BI = Baling->getBaleInfo(Inst);
      continue;
    }
    switch (GenXIntrinsic::getGenXIntrinsicID(Inst)) {
    case GenXIntrinsic::genx_all:
      Control |= PRED_CTRL_ALL; // predicate combine field = "all"
      OperandNum = 0;
      Mask = Inst->getOperand(OperandNum);
      BI = Baling->getBaleInfo(Inst);
      continue;
    case GenXIntrinsic::genx_any:
      Control |= PRED_CTRL_ANY; // predicate combine field = "any"
      OperandNum = 0;
      Mask = Inst->getOperand(OperandNum);
      BI = Baling->getBaleInfo(Inst);
      continue;
    case GenXIntrinsic::genx_rdpredregion: {
      // Baled in rdpredregion. Use its constant offset for the mask control
      // field.
      unsigned MaskOffset =
          cast<ConstantInt>(Inst->getOperand(1))->getSExtValue();
      *MaskCtrl = getVisaEMOffset(MaskOffset);
      Mask = Inst->getOperand(0);
      break;
    }
    default:
      break;
    }
    // Baled shufflepred. Mask offset is deduced from initial value of slice.
    if (auto *SVI = dyn_cast<ShuffleVectorInst>(Inst)) {
      unsigned MaskOffset =
          ShuffleVectorAnalyzer::getReplicatedSliceDescriptor(SVI)
              .InitialOffset;
      *MaskCtrl = getVisaEMOffset(MaskOffset);
      Mask = SVI->getOperand(0);
    }
    break;
  }
  return Mask;
}

void GenXKernelBuilder::AddGenVar(Register &Reg) {
  VISA_GenVar *parentDecl = nullptr;
  VISA_GenVar *Decl = nullptr;

  if (!Reg.AliasTo) {
    LLVM_DEBUG(dbgs() << "GenXKernelBuilder::AddGenVar: "; Reg.print(dbgs()); dbgs() << "\n");
    // This is not an aliased register. Go through all the aliases and
    // determine the biggest alignment required. If the register is at least
    // as big as a GRF, make the alignment GRF.
    unsigned Alignment = getLogAlignment(
        VISA_Align::ALIGN_GRF, Subtarget ? Subtarget->getGRFByteSize()
                                         : defaultGRFByteSize); // GRF alignment
    Type *Ty = Reg.Ty;
    unsigned NBits = Ty->isPointerTy() ? DL.getPointerSizeInBits()
                                       : Ty->getPrimitiveSizeInBits();
    LLVM_DEBUG(dbgs() << "RegTy " << *Ty << ", nbits = " << NBits << "\n");
    if (NBits < GrfByteSize * 8 /* bits in GRF */) {
      Alignment = 0;
      for (Register *AliasReg = &Reg; AliasReg;
           AliasReg = AliasReg->NextAlias) {
        LLVM_DEBUG(dbgs() << "Alias reg " << AliasReg->Num << ", ty "
                          << *(AliasReg->Ty) << "\n");
        Type *AliasTy = AliasReg->Ty->getScalarType();
        unsigned ThisElementBytes = AliasTy->isPointerTy()
                                        ? DL.getPointerTypeSize(AliasTy)
                                        : AliasTy->getPrimitiveSizeInBits() / 8;
        unsigned LogThisElementBytes = genx::log2(ThisElementBytes);
        if (LogThisElementBytes > Alignment)
          Alignment = LogThisElementBytes;
        if (AliasReg->Alignment > Alignment)
          Alignment = AliasReg->Alignment;
      }
    }
    LLVM_DEBUG(dbgs() << "Final alignment of " << Alignment << " for reg "
                      << Reg.Num << "\n");
    for (Register *AliasReg = &Reg; AliasReg; AliasReg = AliasReg->NextAlias) {
      if (AliasReg->Alignment < Alignment) {
        AliasReg->Alignment = Alignment;
        LLVM_DEBUG(dbgs() << "Setting alignment of " << Alignment << " for reg "
                          << AliasReg->Num << "\n");
      }
    }
  } else {
    if (Reg.AliasTo->Num < visa::VISA_NUM_RESERVED_REGS) {
      LLVM_DEBUG(dbgs() << "GenXKernelBuilder::AddGenVar alias: " << Reg.AliasTo->Num << "\n");
      CISA_CALL(Kernel->GetPredefinedVar(parentDecl,
                                         (PreDefined_Vars)Reg.AliasTo->Num));
      IGC_ASSERT_MESSAGE(parentDecl, "Predefeined variable is null");
    } else {
      parentDecl = Reg.AliasTo->GetVar<VISA_GenVar>(Kernel);
      LLVM_DEBUG(dbgs() << "GenXKernelBuilder::AddGenVar decl: " << parentDecl << "\n");
      IGC_ASSERT_MESSAGE(parentDecl, "Refers to undefined var");
    }
  }

  visa::TypeDetails TD(DL, Reg.Ty, Reg.Signed, Reg.IsBF);
  LLVM_DEBUG(dbgs() << "Resulting #of elements: " << TD.NumElements << "\n");

  VISA_Align VA = getVISA_Align(
      Reg.Alignment, Subtarget ? Subtarget->getGRFByteSize() : defaultGRFByteSize);
  CISA_CALL(Kernel->CreateVISAGenVar(Decl, Reg.NameStr.c_str(), TD.NumElements,
                                     static_cast<VISA_Type>(TD.VisaType), VA,
                                     parentDecl, 0));

  Reg.SetVar(Kernel, Decl);
  LLVM_DEBUG(dbgs() << "Resulting decl: " << Decl << "\n");

  for (auto &Attr : Reg.Attributes) {
    CISA_CALL(Kernel->AddAttributeToVar(
        Decl, getStringByIndex(Attr.first).begin(), Attr.second.size(),
        (void *)(Attr.second.c_str())));
  }
}

bool GenXKernelBuilder::allowI64Ops() const {
  IGC_ASSERT(Subtarget);
  if (!Subtarget->hasLongLong())
    return false;
  if (Subtarget->partialI64Emulation())
    return false;
  return true;
}
/**************************************************************************************************
 * Scan ir to collect information about whether kernel has callable function or
 * barrier.
 */
void GenXKernelBuilder::collectKernelInfo() {
  for (auto It = FG->begin(), E = FG->end(); It != E; ++It) {
    auto Func = *It;
    HasStackcalls |= vc::requiresStackCall(Func);
    for (auto &BB : *Func) {
      for (auto &I : BB) {
        if (CallInst *CI = dyn_cast<CallInst>(&I)) {
          if (CI->isInlineAsm())
            continue;
          if (GenXIntrinsicInst *II = dyn_cast<GenXIntrinsicInst>(CI)) {
            auto IID = II->getIntrinsicID();
            if (IID == GenXIntrinsic::genx_barrier ||
                IID == GenXIntrinsic::genx_sbarrier)
              HasBarrier = true;
            else if (IID == GenXIntrinsic::genx_alloca)
              HasAlloca = true;
          } else {
            Function *Callee = CI->getCalledFunction();
            if (Callee && Callee->hasFnAttribute("CMCallable"))
              HasCallable = true;
          }
        }
      }
    }
  }
}
/**************************************************************************************************
 * Build variables
 */
void GenXKernelBuilder::buildVariables() {
  RegAlloc->SetRegPushHook(this, [](void *Object, GenXVisaRegAlloc::Reg &Reg) {
    static_cast<GenXKernelBuilder *>(Object)->AddGenVar(Reg);
  });

  for (auto &It : RegAlloc->getRegStorage()) {
    Register *Reg = &(It);
    switch (Reg->Category) {
    case vc::RegCategory::General:
      if (Reg->Num >= visa::VISA_NUM_RESERVED_REGS)
        AddGenVar(*Reg);
      break;

    case vc::RegCategory::Address: {
      VISA_AddrVar *Decl = nullptr;
      unsigned NumElements = 1;
      if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(Reg->Ty))
        NumElements = VT->getNumElements();
      CISA_CALL(
          Kernel->CreateVISAAddrVar(Decl, Reg->NameStr.c_str(), NumElements));
      Reg->SetVar(Kernel, Decl);
      for (auto &Attr : Reg->Attributes) {
        CISA_CALL(Kernel->AddAttributeToVar(
            Decl, getStringByIndex(Attr.first).begin(), Attr.second.size(),
            (void *)(Attr.second.c_str())));
      }
    } break;

    case vc::RegCategory::Predicate: {
      VISA_PredVar *Decl = nullptr;
      unsigned NumElements = 1;
      if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(Reg->Ty))
        NumElements = VT->getNumElements();
      CISA_CALL(
          Kernel->CreateVISAPredVar(Decl, Reg->NameStr.c_str(), NumElements));
      Reg->SetVar(Kernel, Decl);
      for (auto &Attr : Reg->Attributes) {
        CISA_CALL(Kernel->AddAttributeToVar(
            Decl, getStringByIndex(Attr.first).begin(), Attr.second.size(),
            (void *)(Attr.second.c_str())));
      }
    } break;

    case vc::RegCategory::Sampler: {
      unsigned NumElements = 1;
      if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(Reg->Ty))
        NumElements = VT->getNumElements();
      VISA_SamplerVar *Decl = nullptr;
      CISA_CALL(Kernel->CreateVISASamplerVar(Decl, Reg->NameStr.c_str(),
                                             NumElements));
      Reg->SetVar(Kernel, Decl);
    } break;

    case vc::RegCategory::Surface: {
      VISA_SurfaceVar *Decl = nullptr;
      if (Reg->Num < visa::VISA_NUM_RESERVED_SURFACES) {
        Kernel->GetPredefinedSurface(Decl, (PreDefined_Surface)Reg->Num);
      } else {
        unsigned NumElements = 1;
        if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(Reg->Ty))
          NumElements = VT->getNumElements();

        CISA_CALL(Kernel->CreateVISASurfaceVar(Decl, Reg->NameStr.c_str(),
                                               NumElements));
      }
      Reg->SetVar(Kernel, Decl);
    } break;

    default:
      report_fatal_error("Unknown category for register");
      break;
    }
  }

  VISA_GenVar *ArgDecl = nullptr, *RetDecl = nullptr;
  Kernel->GetPredefinedVar(ArgDecl, PREDEFINED_ARG);
  Kernel->GetPredefinedVar(RetDecl, PREDEFINED_RET);
  createCisaVariable(Kernel, "argv", ArgDecl,
                     visa::ArgRegSizeInGRFs * GrfByteSize);
  createCisaVariable(Kernel, "retv", RetDecl,
                     visa::RetRegSizeInGRFs * GrfByteSize);
}

/***********************************************************************
 * getExecMaskFromWrPredRegion : write exec size field from wrpredregion
 *        or wrpredpredregion instruction
 *
 * Enter:   ExecSize = execution size
 *          WrPredRegion = 0 else wrpredregion instruction
 *
 * The exec size byte includes the mask control field, which we need to set
 * up from the wrpredregion/wrpredpredregion.
 */
VISA_EMask_Ctrl
GenXKernelBuilder::getExecMaskFromWrPredRegion(Instruction *WrPredRegion,
                                               bool IsNoMask) {
  VISA_EMask_Ctrl MaskCtrl =
      (IsNoMask | NoMask) ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
  if (WrPredRegion) {
    // Get the mask control field from the offset in the wrpredregion.
    unsigned MaskOffset =
        cast<ConstantInt>(WrPredRegion->getOperand(2))->getSExtValue();
    IGC_ASSERT_MESSAGE(MaskOffset < 32, "unexpected mask offset");
    MaskCtrl = static_cast<VISA_EMask_Ctrl>(MaskOffset >> 2);
  }

  // Set to NoMask if requested. Otherwise use the default NM mode
  // when WrPredRegion is null.
  if ((IsNoMask && MaskCtrl < vISA_EMASK_M1_NM) ||
      (!WrPredRegion && NoMask && MaskCtrl < vISA_EMASK_M1_NM))
    MaskCtrl = static_cast<VISA_EMask_Ctrl>(
        static_cast<unsigned>(MaskCtrl) + vISA_EMASK_M1_NM);

  return MaskCtrl;
}

/***********************************************************************
 * getExecMaskFromWrRegion : get exec size field from wrregion instruction
 *
 * Enter:   ExecSize = execution size
 *          WrRegion = 0 else wrregion instruction
 *          WrRegionBI = BaleInfo for wrregion, so we can see if there is a
 *                rdpredregion baled in to the mask
 *
 * If WrRegion != 0, and it has a mask that is not constant 1, then the
 * mask must be a predicate register.
 *
 * The exec size byte includes the mask control field, which we need to set
 * up from any rdpredregion baled in to a predicated wrregion.
 *
 * If the predicate has no register allocated, it must be EM, and we set the
 * instruction to be masked. Otherwise we set nomask.
 */
VISA_EMask_Ctrl
GenXKernelBuilder::getExecMaskFromWrRegion(const DstOpndDesc &DstDesc,
                                           bool IsNoMask) {
  // Override mask control if requested.
  auto MaskCtrl = (IsNoMask | NoMask) ? vISA_EMASK_M1_NM : vISA_EMASK_M1;

  if (DstDesc.WrRegion) {
    // Get the predicate (mask) operand, scanning through baled in
    // all/any/not/rdpredregion and setting PredField and MaskCtrl
    // appropriately.
    VISA_PREDICATE_CONTROL Control = PRED_CTRL_NON;
    VISA_PREDICATE_STATE State = PredState_NO_INVERSE;
    Value *Mask =
        getPredicateOperand(DstDesc.WrRegion, 7 /*mask operand in wrregion*/,
                            DstDesc.WrRegionBI, Control, State, &MaskCtrl);
    if ((isa<Constant>(Mask) || getRegForValueOrNullAndSaveAlias(Mask)) &&
        (NoMask || IsNoMask))
      MaskCtrl |= vISA_EMASK_M1_NM;
  }
  return MaskCtrl;
}

/***********************************************************************
 * buildIntrinsic : build code for an intrinsic
 *
 * Enter:   CI = the CallInst
 *          IntrinID = intrinsic ID
 *          BI = BaleInfo for the instruction
 *          Mod = modifier bits for destination
 *          WrRegion = 0 else wrregion for destination
 *          WrRegionBI = BaleInfo for WrRegion
 */
void GenXKernelBuilder::buildIntrinsic(CallInst *CI, unsigned IntrinID,
                                       BaleInfo BI, unsigned Mod,
                                       const DstOpndDesc &DstDesc) {
  using II = GenXIntrinsicInfo;
  LLVM_DEBUG(dbgs() << "buildIntrinsic: " << *CI << "\n");

  int MaxRawOperands = std::numeric_limits<int>::max();

  // TODO: replace lambdas by methods

  auto GetUnsignedValue = [&](II::ArgInfo AI) {
    ConstantInt *Const =
        dyn_cast<ConstantInt>(CI->getArgOperand(AI.getArgIdx()));
    if (!Const) {
      vc::diagnose(getContext(), "GenXCisaBuilder",
                   "Incorrect args to intrinsic call", CI);
    }
    unsigned val = Const->getSExtValue();
    LLVM_DEBUG(dbgs() << "GetUnsignedValue from op #" << AI.getArgIdx()
                      << " yields: " << val << "\n");
    return val;
  };

  auto CreateSurfaceOperand = [&](II::ArgInfo AI) {
    LLVM_DEBUG(dbgs() << "CreateSurfaceOperand\n");
    llvm::Value *Arg = CI->getArgOperand(AI.getArgIdx());
    VISA_SurfaceVar *SurfDecl = nullptr;
    int Index = visa::convertToSurfaceIndex(Arg);
    if (visa::isReservedSurfaceIndex(Index)) {
      Kernel->GetPredefinedSurface(SurfDecl, visa::getReservedSurface(Index));
    } else {
      Register *Reg = getRegForValueAndSaveAlias(Arg);
      IGC_ASSERT_MESSAGE(Reg->Category == vc::RegCategory::Surface,
                         "Expected surface register");
      SurfDecl = Reg->GetVar<VISA_SurfaceVar>(Kernel);
    }
    VISA_StateOpndHandle *ResultOperand = nullptr;
    CISA_CALL(Kernel->CreateVISAStateOperandHandle(ResultOperand, SurfDecl));
    return ResultOperand;
  };

  auto CreatePredefSurfaceOperand = [&](II::ArgInfo AI) {
    LLVM_DEBUG(dbgs() << "CreatePredefinedSurfaceOperand\n");
    auto *Arg = cast<GlobalVariable>(CI->getArgOperand(AI.getArgIdx()));
    VISA_SurfaceVar *SurfVar = getPredefinedSurfaceVar(*Arg);
    VISA_StateOpndHandle *ResultOperand = nullptr;
    CISA_CALL(Kernel->CreateVISAStateOperandHandle(ResultOperand, SurfVar));
    return ResultOperand;
  };

  auto CreateSamplerOperand = [&](II::ArgInfo AI) {
    LLVM_DEBUG(dbgs() << "CreateSamplerOperand\n");
    Register *Reg =
        getRegForValueAndSaveAlias(CI->getArgOperand(AI.getArgIdx()));
    IGC_ASSERT_MESSAGE(Reg->Category == vc::RegCategory::Sampler,
                       "Expected sampler register");
    VISA_StateOpndHandle *ResultOperand = nullptr;
    CISA_CALL(Kernel->CreateVISAStateOperandHandle(
        ResultOperand, Reg->GetVar<VISA_SamplerVar>(Kernel)));
    return ResultOperand;
  };

  auto GetMediaHeght = [&](II::ArgInfo AI) {
    LLVM_DEBUG(dbgs() << "GetMediaHeght\n");
    // constant byte for media height that we need to infer from the
    // media width and the return type or final arg
    ConstantInt *Const =
        dyn_cast<ConstantInt>(CI->getArgOperand(AI.getArgIdx()));
    IGC_ASSERT_MESSAGE(Const, "Incorrect args to intrinsic call");
    unsigned Width = Const->getZExtValue();
    IGC_ASSERT_MESSAGE(Width > 0 && Width <= 64, "Invalid media width");
    unsigned RoundedWidth = roundedVal(Width, 4u);
    Type *DataType = CI->getType();
    if (DataType->isVoidTy())
      DataType = CI->getOperand(IGCLLVM::getNumArgOperands(CI) - 1)->getType();
    unsigned DataSize;
    if (VectorType *VT = dyn_cast<VectorType>(DataType))
      DataSize = DL.getTypeSizeInBits(VT) / genx::ByteBits;
    else
      DataSize = DL.getTypeSizeInBits(DataType) / genx::ByteBits;
    if (DataSize <= RoundedWidth && DataSize >= Width)
      return static_cast<uint8_t>(1);
    IGC_ASSERT_MESSAGE(RoundedWidth && (DataSize % RoundedWidth == 0),
                       "Invalid media width");
    return static_cast<uint8_t>(DataSize / RoundedWidth);
  };

  auto ChooseSign = [&](ArrayRef<unsigned> SrcIdxs) {
    IGC_ASSERT_MESSAGE(!SrcIdxs.empty(), "Expected at least one source index");

    bool hasExt = std::any_of(SrcIdxs.begin(), SrcIdxs.end(),
                              [CI, B = Baling](unsigned Idx) {
                                return isExtOperandBaled(CI, Idx, B);
                              });

    // Keep the old behavior.
    if (hasExt)
      return DONTCARESIGNED;

    SmallVector<Value *, 4> SrcValues;
    std::transform(SrcIdxs.begin(), SrcIdxs.end(),
                   std::back_inserter(SrcValues),
                   [CI](unsigned Idx) { return CI->getOperand(Idx); });

    return getCommonSignedness(SrcValues);
  };

  auto CreateOperand = [&](II::ArgInfo AI, Signedness Signed = DONTCARESIGNED) {
    LLVM_DEBUG(dbgs() << "CreateOperand from arg #" << AI.getArgIdx() << "\n");
    VISA_VectorOpnd *ResultOperand = nullptr;
    IGC_ASSERT_MESSAGE(Signed == DONTCARESIGNED ||
                           !(AI.needsSigned() || AI.needsUnsigned()),
                       "Signedness was set in two different ways.");
    if (AI.needsSigned())
      Signed = SIGNED;
    else if (AI.needsUnsigned())
      Signed = UNSIGNED;
    if (AI.isRet()) {
      if (AI.getSaturation() == II::SATURATION_SATURATE)
        Mod |= MODIFIER_SAT;
      ResultOperand = createDestination(CI, Signed, Mod, DstDesc);
    } else {
      unsigned MaxWidth = 16;
      if (AI.getRestriction() == II::TWICEWIDTH) {
        // For a TWICEWIDTH operand, do not allow width bigger than the
        // execution size.
        MaxWidth =
            cast<IGCLLVM::FixedVectorType>(CI->getType())->getNumElements();
      }
      ResultOperand = createSourceOperand(CI, Signed, AI.getArgIdx(), BI, 0,
                                          nullptr, MaxWidth);
    }
    return ResultOperand;
  };

  auto CreateRawOperand = [&](II::ArgInfo AI) {
    LLVM_DEBUG(dbgs() << "CreateRawOperand from "
                      << (AI.isRet() ? "Dest" : "Src")
                      << " op #" << AI.getArgIdx() << "\n");
    VISA_RawOpnd *ResultOperand = nullptr;
    auto Signed = DONTCARESIGNED;
    if (AI.needsSigned())
      Signed = SIGNED;
    else if (AI.needsUnsigned())
      Signed = UNSIGNED;
    if (AI.isRet()) {
      IGC_ASSERT(!Mod);
      ResultOperand = createRawDestination(CI, DstDesc, Signed);
    } else if (AI.getArgIdx() < MaxRawOperands)
      ResultOperand = createRawSourceOperand(CI, AI.getArgIdx(), BI, Signed);
    return ResultOperand;
  };

  auto CreateRawOperands = [&](II::ArgInfo AI, VISA_RawOpnd **Operands) {
    LLVM_DEBUG(dbgs() << "CreateRawOperands\n");
    IGC_ASSERT_MESSAGE(MaxRawOperands != std::numeric_limits<int>::max(),
      "MaxRawOperands must be defined");
    for (int i = 0; i < AI.getArgIdx() + MaxRawOperands; ++i) {
      Operands[i] = CreateRawOperand(II::ArgInfo(II::RAW | (AI.Info + i)));
    }
  };

  auto GetOwords = [&](II::ArgInfo AI) {
    LLVM_DEBUG(dbgs() << "GetOwords\n");
    // constant byte for log2 number of owords
    Value *Arg = CI;
    if (!AI.isRet())
      Arg = CI->getOperand(AI.getArgIdx());
    auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(Arg->getType());
    if (!VT)
      report_fatal_error("Invalid number of owords");
    int DataSize = VT->getNumElements() *
                   DL.getTypeSizeInBits(VT->getElementType()) / 8;
    DataSize = std::max(0, genx::exactLog2(DataSize) - 4);
    if (DataSize > 4)
      report_fatal_error("Invalid number of words");
    return static_cast<VISA_Oword_Num>(DataSize);
  };

  auto GetExecSize = [&](II::ArgInfo AI, VISA_EMask_Ctrl *Mask) {
    LLVM_DEBUG(dbgs() << "GetExecSize\n");
    int ExecSize = GenXIntrinsicInfo::getOverridedExecSize(CI, Subtarget);
    if (ExecSize == 0) {
      if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(CI->getType())) {
        ExecSize = VT->getNumElements();
      } else {
        ExecSize = 1;
      }
    }
    bool IsNoMask = AI.getCategory() == II::EXECSIZE_NOMASK;
    *Mask = getExecMaskFromWrRegion(DstDesc, IsNoMask);
    return getExecSizeFromValue(ExecSize);
  };

  auto GetBitWidth = [&](II::ArgInfo AI) {
    LLVM_DEBUG(dbgs() << "GetBitWidth\n");
#ifndef NDEBUG
    // Only SVM atomics have this field
    auto ID = GenXIntrinsic::getGenXIntrinsicID(CI);
    switch (ID)
    {
    case llvm::GenXIntrinsic::genx_svm_atomic_add:
    case llvm::GenXIntrinsic::genx_svm_atomic_and:
    case llvm::GenXIntrinsic::genx_svm_atomic_cmpxchg:
    case llvm::GenXIntrinsic::genx_svm_atomic_dec:
    case llvm::GenXIntrinsic::genx_svm_atomic_fcmpwr:
    case llvm::GenXIntrinsic::genx_svm_atomic_fmax:
    case llvm::GenXIntrinsic::genx_svm_atomic_fmin:
    case llvm::GenXIntrinsic::genx_svm_atomic_imax:
    case llvm::GenXIntrinsic::genx_svm_atomic_imin:
    case llvm::GenXIntrinsic::genx_svm_atomic_inc:
    case llvm::GenXIntrinsic::genx_svm_atomic_max:
    case llvm::GenXIntrinsic::genx_svm_atomic_min:
    case llvm::GenXIntrinsic::genx_svm_atomic_or:
    case llvm::GenXIntrinsic::genx_svm_atomic_sub:
    case llvm::GenXIntrinsic::genx_svm_atomic_xchg:
    case llvm::GenXIntrinsic::genx_svm_atomic_xor:
        break;
    default:
      IGC_ASSERT_MESSAGE(0, "Trying to get bit width for non-svm atomic inst");
      break;
    }
#endif // !NDEBUG
    auto* T = AI.isRet() ? CI->getType() : CI->getArgOperand(AI.getArgIdx())->getType();
    unsigned short Width = T->getScalarType()->getPrimitiveSizeInBits();
    return Width;
  };

  auto GetExecSizeFromArg = [&](II::ArgInfo AI,
                                VISA_EMask_Ctrl *ExecMask) {
    LLVM_DEBUG(dbgs() << "GetExecSizeFromArg\n");
    // exec_size inferred from width of predicate arg, defaulting to 16 if
    // it is scalar i1 (as can happen in raw send). Also get M3 etc flag
    // if the predicate has a baled in rdpredregion, and mark as nomask if
    // the predicate is not EM.
    int ExecSize;
    *ExecMask = NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
    // Get the predicate (mask) operand, scanning through baled in
    // all/any/not/rdpredregion and setting PredField and MaskCtrl
    // appropriately.
    VISA_PREDICATE_CONTROL Control;
    VISA_PREDICATE_STATE State;
    Value *Mask =
        getPredicateOperand(CI, AI.getArgIdx(), BI, Control, State, ExecMask);
    if (isa<Constant>(Mask) || getRegForValueOrNullAndSaveAlias(Mask))
      *ExecMask |= NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
    if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(
            CI->getOperand(AI.getArgIdx())->getType()))
      ExecSize = VT->getNumElements();
    else
      ExecSize = GenXIntrinsicInfo::getOverridedExecSize(CI, Subtarget);
    return getExecSizeFromValue(ExecSize ? ExecSize : 1);
  };

  auto GetExecSizeFromByte = [&](II::ArgInfo AI, VISA_EMask_Ctrl *Mask) {
    LLVM_DEBUG(dbgs() << "GetExecSizeFromByte\n");
    ConstantInt *Const =
      dyn_cast<ConstantInt>(CI->getArgOperand(AI.getArgIdx()));
    if (!Const) {
      vc::diagnose(getContext(), "GenXCisaBuilder",
                   "Incorrect args to intrinsic call", CI);
    }
    unsigned Byte = Const->getSExtValue() & 0xFF;
    *Mask = (VISA_EMask_Ctrl)(Byte >> 4);
    unsigned Res = Byte & 0xF;
    if (Res > 5) {
      vc::diagnose(getContext(), "GenXCisaBuilder",
                   "illegal common ISA execsize (should be 1, 2, 4, 8, 16, 32)",
                   CI);
    }
    return (VISA_Exec_Size)Res;
  };

  auto CreateImplicitPredication = [&](II::ArgInfo AI) {
    LLVM_DEBUG(dbgs() << "CreateImplicitPredication\n");
    return createPredFromWrRegion(DstDesc);
  };

  auto CreatePredication = [&](II::ArgInfo AI) {
    LLVM_DEBUG(dbgs() << "CreatePredication\n");
    return createPred(CI, BI, AI.getArgIdx());
  };

  auto GetPredicateVar = [&](II::ArgInfo AI) {
    LLVM_DEBUG(dbgs() << "GetPredicateVar\n");
    if (AI.isRet())
      return getPredicateVar(CI);
    else
      return getPredicateVar(CI->getArgOperand(AI.getArgIdx()));
  };

  auto GetZeroedPredicateVar = [&](II::ArgInfo AI) {
    LLVM_DEBUG(dbgs() << "GetZeroedPredicateVar\n");
    if (AI.isRet())
      return getZeroedPredicateVar(CI);
    else
      return getZeroedPredicateVar(CI->getArgOperand(AI.getArgIdx()));
  };

  auto CreateNullRawOperand = [&](II::ArgInfo AI) {
    LLVM_DEBUG(dbgs() << "CreateNullRawOperand\n");
    VISA_RawOpnd *ResultOperand = nullptr;
    CISA_CALL(Kernel->CreateVISANullRawOperand(ResultOperand, false));
    return ResultOperand;
  };

  auto ProcessTwoAddr = [&](II::ArgInfo AI) {
    LLVM_DEBUG(dbgs() << "ProcessTwoAddr\n");
    if (AI.getCategory() != II::TWOADDR)
      return;
    auto Reg = getRegForValueOrNullAndSaveAlias(CI, DONTCARESIGNED);
    if (isa<UndefValue>(CI->getArgOperand(AI.getArgIdx())) && Reg &&
        isInLoop(CI->getParent()))
      addLifetimeStartInst(CI);
  };

  // Constant vector of i1 (or just scalar i1) as i32 (used in setp)
  auto ConstVi1Asi32 = [&](II::ArgInfo AI) {
    LLVM_DEBUG(dbgs() << "ConstVi1Asi32\n");
    VISA_VectorOpnd *ResultOperand = nullptr;
    auto C = cast<Constant>(CI->getArgOperand(AI.getArgIdx()));
    // Get the bit value of the vXi1 constant.
    unsigned IntVal = getPredicateConstantAsInt(C);
    // unsigned i32 constant source operand
    CISA_CALL(Kernel->CreateVISAImmediate(ResultOperand, &IntVal, ISA_TYPE_UD));
    return ResultOperand;
  };

  auto CreateAddressOperand = [&](II::ArgInfo AI) {
    LLVM_DEBUG(dbgs() << "CreateAddressOperand\n");
    if (AI.isRet())
      return createAddressOperand(CI, true);
    else
      return createAddressOperand(CI->getArgOperand(AI.getArgIdx()), false);
  };

  auto GetArgCount = [&](II::ArgInfo AI) {
    LLVM_DEBUG(dbgs() << "GetArgCount\n");
    auto BaseArg = AI.getArgIdx();
    MaxRawOperands = BaseArg;

    for (unsigned Idx = BaseArg; Idx < IGCLLVM::getNumArgOperands(CI); ++Idx) {
      if (auto CA = dyn_cast<Constant>(CI->getArgOperand(Idx))) {
        if (CA->isNullValue())
          continue;
      }
      MaxRawOperands = Idx + 1;
    }

    if (MaxRawOperands < BaseArg + AI.getArgCountMin())
      MaxRawOperands = BaseArg + AI.getArgCountMin();

    return MaxRawOperands - AI.getArgIdx();
  };

  auto GetNumGrfs = [&](II::ArgInfo AI) {
    LLVM_DEBUG(dbgs() << "GetNumGrfs\n");
    // constant byte for number of GRFs
    Value *Arg = CI;
    if (!AI.isRet())
      Arg = CI->getOperand(AI.getArgIdx());
    auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(Arg->getType());
    if (!VT) {
      vc::diagnose(getContext(), "GenXCisaBuilder", "Invalid number of GRFs",
                   CI);
    }
    int DataSize = VT->getNumElements() *
                   VT->getElementType()->getPrimitiveSizeInBits() / 8;
    return (uint8_t)((DataSize + (GrfByteSize - 1)) / GrfByteSize);
  };

  auto GetSampleChMask = [&](II::ArgInfo AI) {
    LLVM_DEBUG(dbgs() << "GetSampleChMask\n");
    ConstantInt *Const =
        dyn_cast<ConstantInt>(CI->getArgOperand(AI.getArgIdx()));
    if (!Const) {
      vc::diagnose(getContext(), "GenXCisaBuilder",
                   "Incorrect args to intrinsic call", CI);
    }
    unsigned Byte = Const->getSExtValue() & 15;
    // Find the U_offset arg. It is the first vector arg after this one.
    IGCLLVM::FixedVectorType *VT;
    for (unsigned Idx = AI.getArgIdx() + 1;
         !(VT = dyn_cast<IGCLLVM::FixedVectorType>(
               CI->getOperand(Idx)->getType()));
         ++Idx)
      ;
    unsigned Width = VT->getNumElements();
    if (Width != 8 && Width != 16) {
      vc::diagnose(getContext(), "GenXCisaBuilder",
                   "Invalid execution size for load/sample", CI);
    }
    Byte |= Width & 16;
    return Byte;
  };

  auto GetSvmBlockSizeNum = [&](II::ArgInfo Sz, II::ArgInfo Num) {
    LLVM_DEBUG(dbgs() << "SVM gather/scatter element size and num blocks\n");
    // svm gather/scatter "block size" field, set to reflect the element
    // type of the data
    Value *V = CI;
    if (!Sz.isRet())
      V = CI->getArgOperand(Sz.getArgIdx());
    auto *EltType = V->getType()->getScalarType();
    if (auto *MDType = CI->getMetadata(vc::InstMD::SVMBlockType))
      EltType = cast<ValueAsMetadata>(MDType->getOperand(0).get())->getType();
    vc::checkArgOperandIsConstantInt(*CI, Num.getArgIdx(), "log2 num blocks");
    ConstantInt *LogOp = cast<ConstantInt>(CI->getArgOperand(Num.getArgIdx()));
    unsigned LogNum = LogOp->getZExtValue();
    unsigned ElBytes = getResultedTypeSize(EltType, DL);
    switch (ElBytes) {
      // For N = 2 byte data type, use block size 1 and block count x2
      // Otherwise, use block size N and original block count.
    case 2:
      ElBytes = 0;
      IGC_ASSERT(LogNum < 4);
      // This is correct but I can not merge this in while ISPC not fixed
      // LogNum += 1;

      // this is incorrect temporary solution
      LogNum = 1;
      break;
    case 1:
      ElBytes = 0;
      break;
    case 4:
      ElBytes = 1;
      break;
    case 8:
      ElBytes = 2;
      break;
    default:
      vc::diagnose(getContext(), "GenXCisaBuilder",
                   "Bad element type for SVM scatter/gather", CI);
    }
    return std::make_pair(ElBytes, LogNum);
  };

  auto CreateOpndPredefinedSrc = [&](PreDefined_Vars RegId, unsigned ROffset,
                                     unsigned COffset, unsigned VStride,
                                     unsigned Width, unsigned HStride) {
    LLVM_DEBUG(dbgs() << "CreateOpndPredefinedSrc\n");
    VISA_GenVar *Decl = nullptr;
    CISA_CALL(Kernel->GetPredefinedVar(Decl, RegId));
    VISA_VectorOpnd *ResultOperand = nullptr;
    CISA_CALL(Kernel->CreateVISASrcOperand(ResultOperand, Decl,
                                           (VISA_Modifier)Mod, VStride, Width,
                                           HStride, ROffset, COffset));
    return ResultOperand;
  };

  auto CreateOpndPredefinedDst = [&](PreDefined_Vars RegId, unsigned ROffset,
                                     unsigned COffset, unsigned HStride) {
    LLVM_DEBUG(dbgs() << "CreateOpndPredefinedDst\n");
    VISA_GenVar *Decl = nullptr;
    CISA_CALL(Kernel->GetPredefinedVar(Decl, RegId));
    VISA_VectorOpnd *ResultOperand = nullptr;
    CISA_CALL(Kernel->CreateVISADstOperand(ResultOperand, Decl, HStride,
                                           ROffset, COffset));
    return ResultOperand;
  };

  auto CreateImmOpndFromUInt = [&](VISA_Type ImmType, unsigned Val) {
    LLVM_DEBUG(dbgs() << "CreateImmOpndFromUInt\n");
    VISA_VectorOpnd *src = nullptr;
    CISA_CALL(Kernel->CreateVISAImmediate(src, &Val, ImmType));

    return src;
  };

  auto MakeSubbAddcDestination =
      [&](GenXIntrinsic::GenXResult::ResultIndexes MemberIdx) {
        LLVM_DEBUG(dbgs() << "MakeSubbAddcDestination\n");
        IGC_ASSERT(GenXIntrinsic::getGenXIntrinsicID(CI) ==
                       llvm::GenXIntrinsic::genx_addc ||
                   GenXIntrinsic::getGenXIntrinsicID(CI) ==
                       llvm::GenXIntrinsic::genx_subb);
        IGC_ASSERT(IndexFlattener::getNumElements(CI->getType()) == 2);

        auto SV = SimpleValue(CI, MemberIdx);
        auto *DstType = SV.getType();

        IGC_ASSERT(DstType->getScalarType()->isIntegerTy(genx::DWordBits));

        auto *Reg = getRegForValueAndSaveAlias(SV, UNSIGNED);

        const auto TypeSize = CISATypeTable[ISA_TYPE_UD].typeSize;
        auto Elements = 1;
        if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(DstType))
          Elements = VT->getNumElements();

        Region R(IGCLLVM::FixedVectorType::get(
            IntegerType::get(Ctx, TypeSize * genx::ByteBits), Elements));
        return createRegionOperand(&R, Reg->GetVar<VISA_GenVar>(Kernel),
                                   UNSIGNED, Mod, true /* Dst */);
  };


  auto CreateLscUntyped =
      [&](LSC_OP SubOpcode, LSC_SFID LscSfid, VISA_PredOpnd *Pred,
          VISA_Exec_Size ExecSize, VISA_EMask_Ctrl Emask,
          LSC_CACHE_OPTS CacheOpts, LSC_ADDR Addr, LSC_DATA_SHAPE DataShape,
          VISA_VectorOpnd *Surface, VISA_RawOpnd *DstData,
          VISA_RawOpnd *Src0Addr, VISA_RawOpnd *Src1Data,
          VISA_RawOpnd *Src2Data) {
        LLVM_DEBUG(dbgs() << "CreateLscUntyped\n");
        IGC_ASSERT_EXIT(SubOpcode != LSC_LOAD_STRIDED &&
                        SubOpcode != LSC_STORE_STRIDED);
        constexpr unsigned AddressOperandNum = 10;

        auto *AddrV = CI->getArgOperand(AddressOperandNum);
        auto *AddrTy = AddrV->getType();
        if (auto *AddrVTy = dyn_cast<IGCLLVM::FixedVectorType>(AddrTy))
          AddrTy = AddrVTy->getElementType();

        switch (AddrTy->getIntegerBitWidth()) {
        case 32:
          Addr.size = LSC_ADDR_SIZE_32b;
          break;
        case 64:
          IGC_ASSERT_EXIT(LscSfid != LSC_SLM);
          IGC_ASSERT_EXIT(Addr.type == LSC_ADDR_TYPE_FLAT);
          Addr.size = LSC_ADDR_SIZE_64b;
          break;
        default:
          IGC_ASSERT_EXIT_MESSAGE(0, "Unsupported address type");
        }

        Kernel->AppendVISALscUntypedInst(
            SubOpcode, LscSfid, Pred, ExecSize, Emask, CacheOpts, Addr,
            DataShape, Surface, DstData, Src0Addr, Src1Data, Src2Data);
      };

  auto CreateLscUntypedBlock2D =
      [&](LSC_OP SubOpcode, LSC_SFID LscSfid, VISA_PredOpnd *Pred,
          VISA_Exec_Size ExecSize, VISA_EMask_Ctrl Emask,
          LSC_CACHE_OPTS CacheOpts, LSC_DATA_SHAPE_BLOCK2D DataShape,
          VISA_VectorOpnd *Surface, VISA_RawOpnd *DstData,
          VISA_VectorOpnd *Src0AddrY, VISA_VectorOpnd *Src0AddrX,
          VISA_RawOpnd *Src1Data) {
        // FIXME: surface is now FLAT-only
        // FIXME: fill out the nullptr elements
        VISA_VectorOpnd *Src0Addrs[LSC_BLOCK2D_ADDR_PARAMS]{
            nullptr,   // surface base (a 64b scalar addr)
            nullptr,   // surface width (32b)
            nullptr,   // surface height (32b)
            nullptr,   // surface pitch  (32b)
            Src0AddrX, // block x offset (32b)
            Src0AddrY, // block y offset (32b)
        };

        CISA_CALL(Kernel->AppendVISALscUntypedBlock2DInst(
            SubOpcode, LscSfid, Pred, ExecSize, Emask, CacheOpts, DataShape,
            DstData, Src0Addrs, 0, 0, Src1Data));
  };

  auto CreateLscUntypedBlock2DStateless =
      [&](LSC_OP SubOpcode, LSC_SFID LscSfid, VISA_PredOpnd *Pred,
          VISA_Exec_Size ExecSize, VISA_EMask_Ctrl Emask,
          LSC_CACHE_OPTS CacheOpts, LSC_DATA_SHAPE_BLOCK2D DataShape,
          VISA_VectorOpnd *SurfaceBase,  VISA_VectorOpnd *SurfaceWidth,
          VISA_VectorOpnd *SurfaceHeight, VISA_VectorOpnd *SurfacePitch,
          VISA_RawOpnd *DstData, VISA_VectorOpnd *Src0AddrY,
          VISA_VectorOpnd *Src0AddrX, VISA_RawOpnd *Src1Data) {
        // FIXME: surface is now FLAT-only
        VISA_VectorOpnd *Src0Addrs[LSC_BLOCK2D_ADDR_PARAMS]{
            SurfaceBase,   // surface base (a 64b scalar addr)
            SurfaceWidth,  // surface width (32b)
            SurfaceHeight, // surface height (32b)
            SurfacePitch,  // surface pitch  (32b)
            Src0AddrX,     // block x offset (32b)
            Src0AddrY,     // block y offset (32b)
        };

        CISA_CALL(Kernel->AppendVISALscUntypedBlock2DInst(
            SubOpcode, LscSfid, Pred, ExecSize, Emask, CacheOpts, DataShape,
            DstData, Src0Addrs, 0, 0, Src1Data));
  };

  auto CreateLscFence =
      [&](VISA_Exec_Size ExecSize, VISA_PredOpnd *Pred,
          LSC_SFID LscSfid, LSC_FENCE_OP LscFenceOp, LSC_SCOPE LscScope) {
          CISA_CALL(Kernel->AppendVISALscFence(LscSfid, LscFenceOp, LscScope));
  };

  VISA_EMask_Ctrl exec_mask;
#include "GenXIntrinsicsBuildMap.inc"
}

/**************************************************************************************************
 * buildControlRegUpdate : generate an instruction to apply a mask to
 *                         the control register (V14).
 *
 * Enter:   Mask = the mask to apply
 *          Clear = false if bits set in Mask should be set in V14,
 *                  true if bits set in Mask should be cleared in V14.
 */
void GenXKernelBuilder::buildControlRegUpdate(unsigned Mask, bool Clear) {
  ISA_Opcode Opcode;
  // write opcode
  if (Clear) {
    Opcode = ISA_AND;
    Mask = ~Mask;
  } else
    Opcode = ISA_OR;

  Region Single = Region(1, 4);

  VISA_GenVar *Decl = nullptr;
  CISA_CALL(Kernel->GetPredefinedVar(Decl, PREDEFINED_CR0));
  VISA_VectorOpnd *dst =
      createRegionOperand(&Single, Decl, DONTCARESIGNED, 0, true);
  VISA_VectorOpnd *src0 =
      createRegionOperand(&Single, Decl, DONTCARESIGNED, 0, false);

  VISA_VectorOpnd *src1 = nullptr;
  CISA_CALL(Kernel->CreateVISAImmediate(src1, &Mask, ISA_TYPE_UD));

  CISA_CALL(Kernel->AppendVISALogicOrShiftInst(Opcode, nullptr, false,
                                               vISA_EMASK_M1, EXEC_SIZE_1, dst,
                                               src0, src1, nullptr, nullptr));
}

/***********************************************************************
 * buildBranch : build a conditional or unconditional branch
 *
 * Return:  true if fell through to successor
 */
bool GenXKernelBuilder::buildBranch(BranchInst *Branch) {
  BasicBlock *Next = Branch->getParent()->getNextNode();
  if (Branch->isUnconditional()) {
    // Unconditional branch
    if (Branch->getOperand(0) == Next)
      return true; // fall through to successor
    auto labelId = getOrCreateLabel(Branch->getSuccessor(0), LABEL_BLOCK);
    CISA_CALL(Kernel->AppendVISACFJmpInst(nullptr, Labels[labelId]));
    return false;
  }
  // Conditional branch.
  // First check if it is a baled in goto/join, via an extractvalue.
  auto BI = Baling->getBaleInfo(Branch);
  if (BI.isOperandBaled(0 /*condition*/)) {
    if (auto Extract = dyn_cast<ExtractValueInst>(Branch->getCondition())) {
      auto GotoJoin = cast<CallInst>(Extract->getAggregateOperand());
      if (GenXIntrinsic::getGenXIntrinsicID(GotoJoin) == GenXIntrinsic::genx_simdcf_goto) {
        buildGoto(GotoJoin, Branch);
      } else {
        IGC_ASSERT_MESSAGE(GotoJoin::isValidJoin(GotoJoin),
          "extra unexpected code in join block");
        buildJoin(GotoJoin, Branch);
      }
      return true;
    }
  }
  // Normal conditional branch.
  VISA_EMask_Ctrl MaskCtrl;
  VISA_PREDICATE_CONTROL Control = PRED_CTRL_NON;
  VISA_PREDICATE_STATE State = PredState_NO_INVERSE;
  Value *Pred = getPredicateOperand(Branch, 0, BI, Control, State, &MaskCtrl);
  IGC_ASSERT_MESSAGE(!isa<VectorType>(Branch->getCondition()->getType()),
    "branch must have scalar condition");
  BasicBlock *True = Branch->getSuccessor(0);
  BasicBlock *False = Branch->getSuccessor(1);
  if (True == Next) {
    State ^= PredState_INVERSE; // invert bit in predicate field
    True = False;
    False = Next;
  }
  // Write the conditional branch.
  VISA_PredVar *PredVar = getPredicateVar(Pred);
  VISA_PredOpnd *PredOperand = createPredOperand(PredVar, State, Control);
  CISA_CALL(Kernel->AppendVISACFJmpInst(
      PredOperand, Labels[getOrCreateLabel(True, LABEL_BLOCK)]));
  // If the other successor is not the next block, write an unconditional
  // jmp to that.
  if (False == Next)
    return true; // fall through to successor
  CISA_CALL(Kernel->AppendVISACFJmpInst(
      nullptr, Labels[getOrCreateLabel(False, LABEL_BLOCK)]));
  return false;
}

/***********************************************************************
 * buildIndirectBr : build an indirect branch
 *
 * Indirectbr instructions are used only for jump tables.
 *
 * Enter:   Br = indirect branch inst
 */
void GenXKernelBuilder::buildIndirectBr(IndirectBrInst *Br) {
  IGC_ASSERT(Subtarget->hasSwitchjmp());
  Value *Addr = Br->getAddress();
  auto JumpTable = cast<IntrinsicInst>(Addr);
  unsigned IID = vc::getAnyIntrinsicID(JumpTable);
  IGC_ASSERT(IID == vc::InternalIntrinsic::jump_table);
  Value *Idx = JumpTable->getArgOperand(0);

  VISA_VectorOpnd *JMPIdx = createSource(Idx, UNSIGNED);
  unsigned NumDest = Br->getNumDestinations();
  std::vector<VISA_LabelOpnd *> JMPLabels(NumDest, nullptr);
  for (unsigned I = 0; I < NumDest; ++I)
    JMPLabels[I] = Labels[getOrCreateLabel(Br->getDestination(I), LABEL_BLOCK)];

  CISA_CALL(
      Kernel->AppendVISACFSwitchJMPInst(JMPIdx, NumDest, JMPLabels.data()));
}

/***********************************************************************
 * buildJoin : build a join
 *
 * Enter:   Join = join instruction that is baled into an extractvalue of
 *                 field 1 (the !any(EM) value), that is baled into Branch,
 *                 if Branch is non-zero
 *          Branch = branch instruction, or 0 for a join that is not baled
 *                   in to a branch because it always ends up with at least
 *                   one channel enabled
 */
void GenXKernelBuilder::buildJoin(CallInst *Join, BranchInst *Branch) {
  IGC_ASSERT(HasSimdCF);
  // A join needs a label. (If the join is at the start of its block, then
  // this gets merged into the block label.)
  addLabelInst(Join);
  // There is no join instruction in vISA -- the finalizer derives it by
  // looking for gotos targeting the basic block's label.
}

/***********************************************************************
 * buildUnaryOperator : build code for an unary operator
 *
 * Enter:   UO = the UnaryOperator
 *          BI = BaleInfo for UO
 *          Mod = modifier bits for destination
 *          WrRegion = 0 else wrregion for destination
 *          WrRegionBI = BaleInfo for WrRegion
 */
void GenXKernelBuilder::buildUnaryOperator(UnaryOperator *UO, BaleInfo BI,
                                           unsigned Mod,
                                           const DstOpndDesc &DstDesc) {
  ISA_Opcode Opcode = ISA_RESERVED_0;
  Signedness DstSigned = SIGNED;
  Signedness SrcSigned = SIGNED;
  unsigned Mod1 = 0;
  VISA_Exec_Size ExecSize = EXEC_SIZE_1;
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(UO->getType()))
    ExecSize = getExecSizeFromValue(VT->getNumElements());

  switch (UO->getOpcode()) {
    case Instruction::FNeg:
      Opcode = ISA_MOV;
      Mod1 ^= MODIFIER_NEG;
      break;
    default:
      report_fatal_error("buildUnaryOperator: unimplemented unary operator");
  }

  VISA_VectorOpnd *Dst = createDestination(UO, DstSigned, Mod, DstDesc);

  VISA_VectorOpnd *Src0 = nullptr;
  VISA_PredOpnd *Pred = createPredFromWrRegion(DstDesc);

  Src0 = createSourceOperand(UO, SrcSigned, 0, BI, Mod1);

  auto ExecMask = getExecMaskFromWrRegion(DstDesc);

  if (Opcode == ISA_MOV) {
    CISA_CALL(Kernel->AppendVISADataMovementInst(
        ISA_MOV, Pred, Mod1 & MODIFIER_SAT, ExecMask, ExecSize, Dst, Src0, NULL));
    return;
  }
  report_fatal_error("buildUnaryOperator: unimplemented opcode");
}

/***********************************************************************
 * getCommonSignedness : predict the most suitable sign of a instruction based
 *                       on incoming values.
 *
 * Enter:   Vs = incoming values to use for signedness prediction
 */
Signedness GenXKernelBuilder::getCommonSignedness(ArrayRef<Value *> Vs) const {
  // Expect the first value is always set.
  IGC_ASSERT(!Vs.empty());
  std::vector<Register *> Regs;
  std::transform(
      Vs.begin(), Vs.end(), std::back_inserter(Regs), [this](Value *V) { return getLastUsedAlias(V); });
  // If there is no register allocated for Value, getLastUsedAlias returns
  // nullptr. Remove such nodes.
  Regs.erase(std::remove(Regs.begin(), Regs.end(), nullptr), Regs.end());

  if (Regs.empty())
    // Use SIGNED by default if there are no registers for the values.
    return SIGNED;

  bool hasSigned = std::any_of(Regs.begin(), Regs.end(),
                               [](Register *R) { return R->Signed == SIGNED; });
  bool hasUnsigned = std::any_of(Regs.begin(), Regs.end(), [](Register *R) {
    return R->Signed == UNSIGNED;
  });
  // If there is at least one UNSIGNED and others are UNSIGNED or DONTCARESIGNED
  // (absence of a register also means DONTCARESIGNED), UNSIGNED must be used.
  // Otherwise, SIGNED.
  if (hasUnsigned && !hasSigned)
    return UNSIGNED;
  return SIGNED;
}

/***********************************************************************
 * getLastUsedAlias : get the last used alias of a vISA virtual register
 *                    for a value. Nullptr if none.
 */
GenXKernelBuilder::Register *GenXKernelBuilder::getLastUsedAlias(Value *V) const {
  auto Res = LastUsedAliasMap.find(V);
  if (Res == LastUsedAliasMap.end())
    return nullptr;
  return Res->second;
}

/***********************************************************************
 * getRegForValueUntypedAndSaveAlias : a wrapper for
 * GenXVisaRegAlloc::getRegForValueUntyped which also saves the register alias
 * in a special map.
 *
 * Enter:   args = the wrapped function parameters.
 */
template <typename... Args>
GenXKernelBuilder::Register *
GenXKernelBuilder::getRegForValueUntypedAndSaveAlias(Args &&... args) {
  Register *R = RegAlloc->getRegForValueUntyped(std::forward<Args>(args)...);
  SimpleValue SV = std::get<0>(std::make_tuple(args...));
  if (R)
    LastUsedAliasMap[SV.getValue()] = R;
  return R;
}

/***********************************************************************
 * getRegForValueOrNullAndSaveAlias : a wrapper for
 * GenXVisaRegAlloc::getRegForValueOrNull which also saves the register alias in
 * a special map.
 *
 * Enter:   args = the wrapped function parameters.
 */
template <typename... Args>
GenXKernelBuilder::Register *
GenXKernelBuilder::getRegForValueOrNullAndSaveAlias(Args &&... args) {
  Register *R = RegAlloc->getOrCreateRegForValue(std::forward<Args>(args)...);
  SimpleValue SV = std::get<0>(std::make_tuple(args...));
  if (R)
    LastUsedAliasMap[SV.getValue()] = R;
  return R;
}

/***********************************************************************
 * getRegForValueAndSaveAlias : a wrapper for GenXVisaRegAlloc::getRegForValue
 * which also saves the register alias in a special map.
 *
 * Enter:   args = the wrapped function parameters.
 */
template <typename... Args>
GenXKernelBuilder::Register *
GenXKernelBuilder::getRegForValueAndSaveAlias(Args &&...args) {
  Register *R = RegAlloc->getOrCreateRegForValue(std::forward<Args>(args)...);
  SimpleValue SV = std::get<0>(std::make_tuple(args...));
  IGC_ASSERT_MESSAGE(R, "getRegForValue must return non-nullptr register");
  LastUsedAliasMap[SV.getValue()] = R;
  return R;
}

/***********************************************************************
 * buildBinaryOperator : build code for a binary operator
 *
 * Enter:   BO = the BinaryOperator
 *          BI = BaleInfo for BO
 *          Mod = modifier bits for destination
 *          WrRegion = 0 else wrregion for destination
 *          WrRegionBI = BaleInfo for WrRegion
 */
void GenXKernelBuilder::buildBinaryOperator(BinaryOperator *BO, BaleInfo BI,
                                            unsigned Mod,
                                            const DstOpndDesc &DstDesc) {
  bool IsLogic = false;
  ISA_Opcode Opcode = ISA_RESERVED_0;

  Signedness SrcSigned = DONTCARESIGNED;
  Signedness DstSigned = DONTCARESIGNED;
  unsigned Mod1 = 0;
  VISA_Exec_Size ExecSize = EXEC_SIZE_1;
  auto hasConstantIntFitsInWord = [BO]() {
    return std::any_of(BO->op_begin(), BO->op_end(), [](Value *V) {
      auto C = dyn_cast<ConstantInt>(V);
      if (!C)
        return false;
      return C->getValue().getMinSignedBits() <= genx::WordBits;
    });
  };
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(BO->getType()))
    ExecSize = getExecSizeFromValue(VT->getNumElements());
  switch (BO->getOpcode()) {
  case Instruction::Add:
  case Instruction::FAdd:
    Opcode = ISA_ADD;
    break;
  case Instruction::Sub:
  case Instruction::FSub:
    Opcode = ISA_ADD;
    Mod1 ^= MODIFIER_NEG;
    break;
  case Instruction::Mul:
  case Instruction::FMul:
    Opcode = ISA_MUL;
    // Check if there is a possibility to truncate the integer constant further
    // that will help to generate better code. In this case SIGNED type must be
    // used.
    if (hasConstantIntFitsInWord())
      DstSigned = SrcSigned = SIGNED;
    break;
  case Instruction::Shl:
    Opcode = ISA_SHL;
    IsLogic = true;
    break;
  case Instruction::AShr:
    Opcode = ISA_ASR;
    DstSigned = SrcSigned = SIGNED;
    IsLogic = true;
    break;
  case Instruction::LShr:
    Opcode = ISA_SHR;
    DstSigned = SrcSigned = UNSIGNED;
    IsLogic = true;
    break;
  case Instruction::UDiv:
    Opcode = ISA_DIV;
    DstSigned = SrcSigned = UNSIGNED;
    break;
  case Instruction::SDiv:
    Opcode = ISA_DIV;
    DstSigned = SrcSigned = SIGNED;
    break;
  case Instruction::FDiv: {
    Opcode = ISA_DIV;
    if (Constant *Op0 = dyn_cast<Constant>(BO->getOperand(0))) {
      if (Op0->getType()->isVectorTy())
        Op0 = Op0->getSplatValue();
      ConstantFP *CFP = dyn_cast_or_null<ConstantFP>(Op0);
      if (CFP && CFP->isExactlyValue(1.0))
        Opcode = ISA_INV;
    }
  } break;
  case Instruction::URem:
    Opcode = ISA_MOD;
    DstSigned = SrcSigned = UNSIGNED;
    break;
  case Instruction::SRem:
    DstSigned = SrcSigned = SIGNED;
    Opcode = ISA_MOD;
    break;
  case Instruction::FRem:
    Opcode = ISA_MOD;
    break;
  case Instruction::And:
    Opcode = ISA_AND;
    IsLogic = true;
    break;
  case Instruction::Or:
    Opcode = ISA_OR;
    IsLogic = true;
    break;
  case Instruction::Xor:
    Opcode = ISA_XOR;
    IsLogic = true;
    break;
  default:
    report_fatal_error("buildBinaryOperator: unimplemented binary operator");
    break;
  }

  // If signedness wasn't set explicitly earlier and destination modifier isn't
  // set.
  if (SrcSigned == DONTCARESIGNED && DstSigned == DONTCARESIGNED) {

    bool hasExt =
        std::any_of(BO->use_begin(), BO->use_end(),
                    [B = Baling](Use &U) { return isExtOperandBaled(U, B); });

    if (Mod == MODIFIER_NONE && !hasExt) {
      Value *Op0 = BO->getOperand(0);
      Value *Op1 = BO->getOperand(1);
      if (Opcode == ISA_INV)
        SrcSigned = DstSigned = getCommonSignedness({Op1});
      else
        SrcSigned = DstSigned = getCommonSignedness({Op0, Op1});
    } else
      // If the modifier is set or SEXT, ZEXT is baled, use old behavior.
      SrcSigned = DstSigned = SIGNED;
  }

  VISA_VectorOpnd *Dst = createDestination(BO, DstSigned, Mod, DstDesc);

  VISA_VectorOpnd *Src0 = nullptr;
  VISA_VectorOpnd *Src1 = nullptr;
  VISA_PredOpnd *Pred = createPredFromWrRegion(DstDesc);

  if (Opcode == ISA_INV) {
    Src0 = createSourceOperand(BO, SrcSigned, 1, BI, Mod1); // source 0
  } else {
    Src0 = createSourceOperand(BO, SrcSigned, 0, BI);       // source 0
    Src1 = createSourceOperand(BO, SrcSigned, 1, BI, Mod1); // source 1
  }

  auto ExecMask = getExecMaskFromWrRegion(DstDesc);

  if (IsLogic) {
    CISA_CALL(Kernel->AppendVISALogicOrShiftInst(
        Opcode, Pred, Mod, ExecMask, ExecSize, Dst, Src0, Src1, NULL, NULL));
  } else {
    if (Opcode == ISA_ADDC || Opcode == ISA_SUBB) {
        IGC_ASSERT(0);
    } else {
      CISA_CALL(Kernel->AppendVISAArithmeticInst(
          Opcode, Pred, Mod, ExecMask, ExecSize, Dst, Src0, Src1, NULL));
    }
  }
}

/***********************************************************************
 * buildBoolBinaryOperator : build code for a binary operator acting on
 *                           i1 or vector of i1
 *
 * Enter:   BO = the BinaryOperator
 */
void GenXKernelBuilder::buildBoolBinaryOperator(BinaryOperator *BO) {
  VISA_Exec_Size ExecSize = EXEC_SIZE_1;
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(BO->getType()))
    ExecSize = getExecSizeFromValue(VT->getNumElements());
  ISA_Opcode Opcode = ISA_RESERVED_0;
  switch (BO->getOpcode()) {
  case Instruction::And:
    Opcode = ISA_AND;
    break;
  case Instruction::Or:
    Opcode = ISA_OR;
    break;
  case Instruction::Xor:
    Opcode = ISA_XOR;
    if (isNot(BO))
      Opcode = ISA_NOT;
    break;
  default:
    report_fatal_error(
        "buildBoolBinaryOperator: unimplemented binary operator");
    break;
  }

  if (isPredNot(BO) && BO->hasOneUse()) {
    // If this NOT predicate is a goto operand and it has only one use, then we
    // won't emit it. %P1 = ... %P2 = not %P1
    // (!%P2) goto
    // Transforms into
    // (%P1) goto

    auto Goto = dyn_cast<CallInst>(*BO->user_begin());
    if (Goto && GenXIntrinsic::getGenXIntrinsicID(Goto) ==
                    GenXIntrinsic::genx_simdcf_goto)
      return;
  }

  VISA_PredVar *Dst = getPredicateVar(BO);
  VISA_PredVar *Src0 = getPredicateVar(BO->getOperand(0));
  VISA_PredVar *Src1 =
      Opcode != ISA_NOT ? getPredicateVar(BO->getOperand(1)) : nullptr;

  CISA_CALL(Kernel->AppendVISALogicOrShiftInst(
      Opcode, NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1, ExecSize, Dst, Src0,
      Src1));
}

void GenXKernelBuilder::buildSymbolInst(CallInst *GAddrInst, unsigned Mod,
                                        const DstOpndDesc &DstDesc) {
  IGC_ASSERT_MESSAGE(GAddrInst, "wrong argument: nullptr is unallowed");
  IGC_ASSERT_MESSAGE(GenXIntrinsic::getGenXIntrinsicID(GAddrInst) ==
                         GenXIntrinsic::genx_gaddr,
                     "wrong argument: genx.addr intrinsic is expected");
  auto *GV = cast<GlobalValue>(GAddrInst->getOperand(0));
  VISA_VectorOpnd *Dst = createDestination(GAddrInst, UNSIGNED, Mod, DstDesc);
  CISA_CALL(Kernel->AppendVISACFSymbolInst(GV->getName().str(), Dst));
}

/***********************************************************************
 * buildWritePredefSurface : get predefined visa surface variable
 *
 * Enter:   GV = global that denotes predefined variable
 *
 * Return:  visa surface variable, non-null
 *
 */
VISA_SurfaceVar *
GenXKernelBuilder::getPredefinedSurfaceVar(GlobalVariable &GV) {
  StringRef SurfName = GV.getName();
  PreDefined_Surface VisaSurfName =
      StringSwitch<PreDefined_Surface>(SurfName)
          .Case(vc::PredefVar::BSSName, PREDEFINED_SURFACE_T252)
          .Default(PREDEFINED_SURFACE_INVALID);
  IGC_ASSERT_MESSAGE(VisaSurfName != PREDEFINED_SURFACE_INVALID,
                     "Unexpected predefined surface");
  VISA_SurfaceVar *SurfVar = nullptr;
  CISA_CALL(Kernel->GetPredefinedSurface(SurfVar, VisaSurfName));
  return SurfVar;
}

// Returns vISA predefined variable corresponding to the provided global
// variable \p GV.
VISA_GenVar *GenXKernelBuilder::getPredefinedGeneralVar(GlobalVariable &GV) {
  VISA_GenVar *Variable = nullptr;
  auto VariableID =
      StringSwitch<PreDefined_Vars>(GV.getName())
          .Case(vc::PredefVar::ImplicitArgsBufferName,
                PREDEFINED_IMPL_ARG_BUF_PTR)
          .Case(vc::PredefVar::LocalIDBufferName, PREDEFINED_LOCAL_ID_BUF_PTR)
          .Default(PREDEFINED_VAR_INVALID);
  IGC_ASSERT_MESSAGE(VariableID != PREDEFINED_VAR_INVALID,
                     "Unexpected predefined general variable");
  CISA_CALL(Kernel->GetPredefinedVar(Variable, VariableID));
  return Variable;
}

// Creates region based on read_variable_region intrinsic operands.
static Region createRegion(vc::ReadVariableRegion RVR,
                           DataLayout *DL = nullptr) {
  Region R;
  R.NumElements = vc::getNumElements(*RVR.getCallInst().getType());
  R.VStride = RVR.getVStride();
  R.Width = RVR.getWidth();
  R.Stride = RVR.getStride();
  R.Offset = RVR.getOffset().inBytes();
  R.setElementTy(RVR.getElementType(), DL);
  return R;
}

// Creates region based on write_variable_region intrinsic operands.
static Region createRegion(vc::WriteVariableRegion WVR,
                           DataLayout *DL = nullptr) {
  Region R;
  R.NumElements = vc::getNumElements(*WVR.getInput().getType());
  R.Stride = WVR.getStride();
  R.Offset = WVR.getOffset().inBytes();
  R.setElementTy(WVR.getElementType(), DL);

  IGC_ASSERT_MESSAGE(
      R.Stride > 0,
      "write.variable.region must have 1D region which stride cannot be 0");
  return R;
}

// Creates MOV instruction for not baled read_variable_region intrinsic.
void GenXKernelBuilder::buildLoneReadVariableRegion(CallInst &CI) {
  vc::ReadVariableRegion RVR{CI};
  VISA_GenVar *Variable = getPredefinedGeneralVar(RVR.getVariable());
  Region R = createRegion(RVR);
  VISA_VectorOpnd *SrcOp =
      createGeneralOperand(&R, Variable, Signedness::DONTCARESIGNED,
                           MODIFIER_NONE, /* IsDest=*/false);
  VISA_VectorOpnd *DstOp = createDestination(&CI, Signedness::DONTCARESIGNED);
  CISA_CALL(Kernel->AppendVISADataMovementInst(
      ISA_MOV, /*pred=*/nullptr, /*satMod=*/false, vISA_EMASK_M1_NM,
      getExecSizeFromValue(R.NumElements), DstOp, SrcOp));
}

// Creates MOV instruction for not baled write_variable_region intrinsic.
void GenXKernelBuilder::buildLoneWriteVariableRegion(CallInst &CI) {
  vc::WriteVariableRegion WVR{CI};
  IGC_ASSERT_MESSAGE(cast<Constant>(WVR.getMask()).isAllOnesValue(),
                     "predication is not yet allowed");
  VISA_GenVar *Variable = getPredefinedGeneralVar(WVR.getVariable());
  Region R = createRegion(WVR);
  VISA_VectorOpnd *SrcOp =
      createSource(&WVR.getInput(), Signedness::DONTCARESIGNED);
  VISA_VectorOpnd *DstOp =
      createGeneralOperand(&R, Variable, Signedness::DONTCARESIGNED,
                           MODIFIER_NONE, /* IsDest=*/true);
  CISA_CALL(Kernel->AppendVISADataMovementInst(
      ISA_MOV, /*pred=*/nullptr, /*satMod=*/false, vISA_EMASK_M1_NM,
      getExecSizeFromValue(R.NumElements), DstOp, SrcOp));
}

/***********************************************************************
 * buildWritePredefSurface : build code to write to predefined surface
 *
 * Enter:   CI = write_predef_surface intrinsic
 *
 */
void GenXKernelBuilder::buildWritePredefSurface(CallInst &CI) {
  IGC_ASSERT_MESSAGE(GenXIntrinsic::getGenXIntrinsicID(&CI) ==
                         GenXIntrinsic::genx_write_predef_surface,
                     "Expected predefined surface write intrinsic");
  auto *PredefSurf = cast<GlobalVariable>(CI.getArgOperand(0));
  VISA_SurfaceVar *SurfVar = getPredefinedSurfaceVar(*PredefSurf);
  VISA_VectorOpnd *SurfOpnd = nullptr;
  CISA_CALL(Kernel->CreateVISAStateOperand(SurfOpnd, SurfVar, /*offset=*/0,
                                           /*useAsDst=*/true));
  VISA_VectorOpnd *SrcOpnd = createSource(CI.getArgOperand(1), genx::UNSIGNED);
  CISA_CALL(Kernel->AppendVISADataMovementInst(
      ISA_MOVS, /*pred=*/nullptr, /*satMod=*/false, vISA_EMASK_M1_NM,
      EXEC_SIZE_1, SurfOpnd, SrcOpnd));
}

/***********************************************************************
 * buildCastInst : build code for a cast (other than a no-op cast)
 *
 * Enter:   CI = the CastInst
 *          BI = BaleInfo for CI
 *          Mod = modifier bits for destination
 *          WrRegion = 0 else wrregion for destination
 *          WrRegionBI = BaleInfo for WrRegion
 */
void GenXKernelBuilder::buildCastInst(CastInst *CI, BaleInfo BI, unsigned Mod,
                                      const DstOpndDesc &DstDesc) {
  Signedness InSigned = DONTCARESIGNED;
  Signedness OutSigned = DONTCARESIGNED;
  switch (CI->getOpcode()) {
  case Instruction::UIToFP:
    InSigned = UNSIGNED;
    break;
  case Instruction::SIToFP:
    InSigned = SIGNED;
    break;
  case Instruction::FPToUI:
    OutSigned = UNSIGNED;
    break;
  case Instruction::FPToSI:
    OutSigned = SIGNED;
    break;
  case Instruction::ZExt:
    InSigned = UNSIGNED;
    break;
  case Instruction::SExt:
    InSigned = SIGNED;
    break;
  case Instruction::FPTrunc:
  case Instruction::FPExt:
    break;
  case Instruction::PtrToInt:
  case Instruction::IntToPtr:
    break;
  case Instruction::AddrSpaceCast:
    break;
  case Instruction::Trunc:
    break;
  default:
    vc::diagnose(getContext(), "GenXCisaBuilder",
                 "buildCastInst: unimplemented cast", CI);
  }

  VISA_Exec_Size ExecSize = EXEC_SIZE_1;
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(CI->getType()))
    ExecSize = getExecSizeFromValue(VT->getNumElements());

  auto ExecMask = getExecMaskFromWrRegion(DstDesc);

  VISA_PredOpnd *Pred = createPredFromWrRegion(DstDesc);
  // Give dest and source the same signedness for byte mov.
  VISA_VectorOpnd *Dst = createDestination(CI, OutSigned, Mod, DstDesc);

  if (InSigned == DONTCARESIGNED)
    InSigned = OutSigned;
  VISA_VectorOpnd *Src0 = createSourceOperand(CI, InSigned, 0, BI);

  CISA_CALL(Kernel->AppendVISADataMovementInst(
      ISA_MOV, Pred, Mod & MODIFIER_SAT, ExecMask, ExecSize, Dst, Src0, NULL));
}

/***********************************************************************
 * buildCmp : build code for a compare
 *
 * Enter:   Cmp = the compare instruction
 *          BI = BaleInfo for Cmp
 *          WrRegion = 0 else wrpredregion, wrpredpredregion, or wrregion for
 *          destination
 */
void GenXKernelBuilder::buildCmp(CmpInst *Cmp, BaleInfo BI,
                                 const DstOpndDesc &DstDesc) {
  IGC_ASSERT_MESSAGE(testPredicate(Cmp, DstDesc),
    "write predicate size 4 only allowed for double/longlong type");
  Signedness Signed = DONTCARESIGNED;
  VISA_Cond_Mod opSpec;
  switch (Cmp->getPredicate()) {
  case CmpInst::FCMP_ONE:
  case CmpInst::FCMP_ORD:
  case CmpInst::FCMP_UEQ:
  case CmpInst::FCMP_UGT:
  case CmpInst::FCMP_UGE:
  case CmpInst::FCMP_ULT:
  case CmpInst::FCMP_ULE:
  case CmpInst::FCMP_UNO:
    IGC_ASSERT_MESSAGE(0, "unsupported fcmp predicate");
    break;
  case CmpInst::FCMP_OEQ:
  case CmpInst::ICMP_EQ:
    opSpec = ISA_CMP_E;
    break;
  case CmpInst::FCMP_UNE:
  case CmpInst::ICMP_NE:
    opSpec = ISA_CMP_NE;
    break;
  case CmpInst::FCMP_OGT:
    opSpec = ISA_CMP_G;
    break;
  case CmpInst::ICMP_UGT:
    opSpec = ISA_CMP_G;
    Signed = UNSIGNED;
    break;
  case CmpInst::ICMP_SGT:
    opSpec = ISA_CMP_G;
    Signed = SIGNED;
    break;
  case CmpInst::FCMP_OGE:
    opSpec = ISA_CMP_GE;
    break;
  case CmpInst::ICMP_UGE:
    opSpec = ISA_CMP_GE;
    Signed = UNSIGNED;
    break;
  case CmpInst::ICMP_SGE:
    opSpec = ISA_CMP_GE;
    Signed = SIGNED;
    break;
  case CmpInst::FCMP_OLT:
    opSpec = ISA_CMP_L;
    break;
  case CmpInst::ICMP_ULT:
    opSpec = ISA_CMP_L;
    Signed = UNSIGNED;
    break;
  case CmpInst::ICMP_SLT:
    opSpec = ISA_CMP_L;
    Signed = SIGNED;
    break;
  case CmpInst::FCMP_OLE:
    opSpec = ISA_CMP_LE;
    break;
  case CmpInst::ICMP_ULE:
    opSpec = ISA_CMP_LE;
    Signed = UNSIGNED;
    break;
  case CmpInst::ICMP_SLE:
    opSpec = ISA_CMP_LE;
    Signed = SIGNED;
    break;
  default:
    vc::diagnose(getContext(), "GenXCisaBuilder", "unknown predicate", Cmp);
  }

  // Check if this is to write to a predicate desination or a GRF desination.
  bool WriteToPred = true;
  if (Cmp->hasOneUse()) {
    Instruction *UI = Cmp->user_back();
    BaleInfo UserBI = Baling->getBaleInfo(UI);
    if (UserBI.Type == BaleInfo::CMPDST)
      WriteToPred = false;
  }

  VISA_Exec_Size ExecSize = EXEC_SIZE_1;
  VISA_EMask_Ctrl ctrlMask = vISA_EMASK_M1;
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(Cmp->getType()))
    ExecSize = getExecSizeFromValue(VT->getNumElements());

  VISA_VectorOpnd *Dst = nullptr;
  genx::Signedness SignedSrc0;
  VISA_VectorOpnd *Src0 =
      createSourceOperand(Cmp, Signed, 0, BI, 0, &SignedSrc0);
  VISA_VectorOpnd *Src1 = createSourceOperand(Cmp, SignedSrc0, 1, BI);

  if (WriteToPred) {
    ctrlMask = getExecMaskFromWrPredRegion(DstDesc.WrRegion, false);
    VISA_PredVar *PredVar =
        getPredicateVar(DstDesc.WrRegion ? DstDesc.WrRegion : Cmp);
    CISA_CALL(Kernel->AppendVISAComparisonInst(opSpec, ctrlMask, ExecSize,
                                               PredVar, Src0, Src1));
  } else {
    ctrlMask = getExecMaskFromWrRegion(DstDesc);
    Value *Val = DstDesc.WrRegion ? DstDesc.WrRegion : Cmp->user_back();
    Dst = createDestination(Val, Signed, 0, DstDesc);
    CISA_CALL(Kernel->AppendVISAComparisonInst(opSpec, ctrlMask, ExecSize, Dst,
                                               Src0, Src1));
  }
}

/***********************************************************************
 * buildConvertAddr : build code for conversion to address
 *
 * Enter:   CI = the CallInst
 *          BI = BaleInfo for CI
 *          Mod = modifier bits for destination
 *          WrRegion = 0 else wrregion for destination
 *          WrRegionBI = BaleInfo for WrRegion
 */
void GenXKernelBuilder::buildConvertAddr(CallInst *CI, genx::BaleInfo BI,
                                         unsigned Mod,
                                         const DstOpndDesc &DstDesc) {
  IGC_ASSERT(!DstDesc.WrRegion);
  Value *Base = Liveness->getAddressBase(CI);
  VISA_Exec_Size ExecSize = EXEC_SIZE_1;
  VISA_EMask_Ctrl MaskCtrl = NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;

  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(CI->getType()))
    ExecSize = getExecSizeFromValue(VT->getNumElements());
  // If the offset is less aligned than the base register element type, then
  // we need a different type.
  Type *OverrideTy = nullptr;
  Type *BaseTy = Base->getType();
  if (BaseTy->isPointerTy())
    BaseTy = IGCLLVM::getNonOpaquePtrEltTy(BaseTy);
  unsigned ElementBytes =
      BaseTy->getScalarType()->getPrimitiveSizeInBits() >> 3;
  int Offset = cast<ConstantInt>(CI->getArgOperand(1))->getSExtValue();
  if ((ElementBytes - 1) & Offset) {
    OverrideTy = IGCLLVM::FixedVectorType::get(
        Type::getInt8Ty(CI->getContext()),
        cast<IGCLLVM::FixedVectorType>(BaseTy)->getNumElements() *
            ElementBytes);
    ElementBytes = 1;
  }
  Register *BaseReg =
      getRegForValueAndSaveAlias(Base, DONTCARESIGNED, OverrideTy);

  VISA_VectorOpnd *Dst = createAddressOperand(CI, true);
  VISA_VectorOpnd *Src1 = nullptr;

  if (BaseReg->Category == vc::RegCategory::Surface ||
      BaseReg->Category == vc::RegCategory::Sampler) {
    uint8_t offset = Offset >> 2;
    switch (BaseReg->Category) {
    case vc::RegCategory::Surface: {
      VISA_SurfaceVar *Decl = BaseReg->GetVar<VISA_SurfaceVar>(Kernel);
      unsigned int offsetB = offset * 2; // 2 is bytes size of UW
      CISA_CALL(Kernel->CreateVISAAddressOfOperand(Src1, Decl, offsetB));
      break;
    }
    case vc::RegCategory::Sampler: {
      VISA_SurfaceVar *Decl = BaseReg->GetVar<VISA_SurfaceVar>(Kernel);
      unsigned int offsetB = offset * 2; // 2 is bytes size of UW
      CISA_CALL(Kernel->CreateVISAAddressOfOperand(Src1, Decl, offsetB));
      break;
    }
    default:
      vc::diagnose(getContext(), "GenXCisaBuilder",
                   "Invalid state operand class: only surface, vme, and "
                   "sampler are supported.",
                   CI);
    }
  } else {
    uint8_t rowOffset = Offset >> genx::log2(GrfByteSize);
    uint8_t colOffset = (Offset & (GrfByteSize - 1)) >> Log2_32(ElementBytes);
    auto TypeSize = BaseReg->Ty->getScalarType()->getPrimitiveSizeInBits() >> 3;
    unsigned int offset = colOffset * TypeSize + rowOffset * GrfByteSize;

    if (BaseReg->Category == vc::RegCategory::Address) {
      VISA_AddrVar *Decl = BaseReg->GetVar<VISA_AddrVar>(Kernel);
      unsigned Width = 1;
      CISA_CALL(Kernel->CreateVISAAddressSrcOperand(Src1, Decl, offset, Width));
    } else {
      VISA_GenVar *Decl = BaseReg->GetVar<VISA_GenVar>(Kernel);
      CISA_CALL(Kernel->CreateVISAAddressOfOperand(Src1, Decl, offset));
    }
  }
  VISA_VectorOpnd *Src2 = createSourceOperand(CI, UNSIGNED, 0, BI);
  CISA_CALL(Kernel->AppendVISAAddrAddInst(MaskCtrl, ExecSize, Dst, Src1, Src2));
}

/***********************************************************************
 * buildAlloca : build code for allocating in thread-private memory
 *
 * Enter:   CI = the CallInst
 *
 */
void GenXKernelBuilder::buildAlloca(CallInst *CI, unsigned IntrinID,
                                    unsigned Mod, const DstOpndDesc &DstDesc) {
  LLVM_DEBUG(dbgs() << "Building alloca " << *CI << "\n");
  VISA_GenVar *Sp = nullptr;
  CISA_CALL(Kernel->GetPredefinedVar(Sp, PreDefined_Vars::PREDEFINED_FE_SP));
  if (!allowI64Ops())
    CISA_CALL(Kernel->CreateVISAGenVar(Sp, "Sp", 1, ISA_TYPE_UD, ALIGN_DWORD, Sp));

  Value *AllocaOff = CI->getOperand(0);
  Type *AllocaOffTy = AllocaOff->getType();

  if (CurrentPadding) {
    // padd the current alloca the comply with gather/scatter alignment rules
    // unsigned LastOff = getResultedTypeSize(LastAlloca->getOperand(0)->getType(), DL);
    auto *AllocaEltTy = AllocaOffTy->getScalarType();
    if (AllocaOffTy->isArrayTy())
      AllocaEltTy = AllocaOffTy->getArrayElementType();
    unsigned Padding = DL.getTypeSizeInBits(AllocaEltTy) / genx::ByteBits;
    Padding = (Padding - CurrentPadding) % Padding;
    if (Padding) {
      VISA_VectorOpnd *SpSrc = nullptr;
      CISA_CALL(Kernel->CreateVISASrcOperand(SpSrc, Sp, MODIFIER_NONE, 0, 1, 0,
                                             0, 0));
      VISA_VectorOpnd *PaddImm = nullptr;
      CISA_CALL(Kernel->CreateVISAImmediate(PaddImm, &Padding, ISA_TYPE_D));
      VISA_VectorOpnd *DstSp = nullptr;
      CISA_CALL(Kernel->CreateVISADstOperand(
          DstSp, static_cast<VISA_GenVar *>(Sp), 1, 0, 0));

      CISA_CALL(Kernel->AppendVISAArithmeticInst(ISA_ADD, nullptr, false,
                                                 vISA_EMASK_M1, EXEC_SIZE_1,
                                                 DstSp, SpSrc, PaddImm));
      CurrentPadding += Padding;
    }
  }

  VISA_VectorOpnd *SpSrc = nullptr;
  CISA_CALL(
      Kernel->CreateVISASrcOperand(SpSrc, Sp, MODIFIER_NONE, 0, 1, 0, 0, 0));

  unsigned OffVal = getResultedTypeSize(AllocaOffTy, DL);
  CurrentPadding = (CurrentPadding + OffVal) %
                   (DL.getLargestLegalIntTypeSizeInBits() / genx::ByteBits);

  VISA_VectorOpnd *Imm = nullptr;
  CISA_CALL(Kernel->CreateVISAImmediate(Imm, &OffVal, ISA_TYPE_D));

  if (IntrinID == llvm::GenXIntrinsic::genx_alloca) {
    VISA_VectorOpnd *Src = nullptr;
    CISA_CALL(Kernel->CreateVISASrcOperand(Src, static_cast<VISA_GenVar *>(Sp),
                                           MODIFIER_NONE, 0, 1, 0, 0, 0));
    VISA_VectorOpnd *Dst = createDestination(CI, DONTCARESIGNED, Mod, DstDesc);
    CISA_CALL(Kernel->AppendVISADataMovementInst(
        ISA_MOV, nullptr, false, vISA_EMASK_M1, EXEC_SIZE_1, Dst, Src));
  }

  VISA_VectorOpnd *DstSp = nullptr;
  CISA_CALL(Kernel->CreateVISADstOperand(DstSp, static_cast<VISA_GenVar *>(Sp),
                                         1, 0, 0));

  CISA_CALL(Kernel->AppendVISAArithmeticInst(
      ISA_ADD, nullptr, false, vISA_EMASK_M1, EXEC_SIZE_1, DstSp, SpSrc, Imm));
}

/***********************************************************************
 * buildPrintIndex : build code for storing constant format strins as metadata
 *                   and returning idx for that string
 *
 * Enter:   CI = the CallInst
 *
 */
void GenXKernelBuilder::buildPrintIndex(CallInst *CI, unsigned IntrinID,
                                        unsigned Mod,
                                        const DstOpndDesc &DstDesc) {
  // create move with constant
  VISA_VectorOpnd *Imm = nullptr;
  Module* M = CI->getModule();
  NamedMDNode *NMD = M->getOrInsertNamedMetadata("cm_print_strings");
  unsigned NumOp  = NMD->getNumOperands();
  CISA_CALL(Kernel->CreateVISAImmediate(Imm, &NumOp, ISA_TYPE_UD));
  VISA_VectorOpnd *Dst = createDestination(CI, DONTCARESIGNED, Mod, DstDesc);
  CISA_CALL(Kernel->AppendVISADataMovementInst(
            ISA_MOV, nullptr, false, vISA_EMASK_M1_NM,
            EXEC_SIZE_1, Dst, Imm));

  // access string
  StringRef UnderlyingCStr =
      vc::getConstStringFromOperand(*CI->getArgOperand(0));

  // store metadata
  LLVMContext &Context = CI->getContext();
  MDNode* N = MDNode::get(Context, MDString::get(Context, UnderlyingCStr));
  NMD->addOperand(N);
}

void GenXKernelBuilder::deduceRegion(Region *R, bool IsDest,
                                     unsigned MaxWidth) {
  IGC_ASSERT(Subtarget);
  if (!IsDest && !R->is2D() && R->Indirect &&
      Subtarget->hasIndirectGRFCrossing()) {
    // For a source 1D indirect region that might possibly cross a GRF
    // (because we are on SKL+ so a single GRF crossing is allowed), make it
    // Nx1 instead of 1xN to avoid crossing a GRF within a row.
    R->VStride = R->Stride;
    R->Width = 1;
    R->Stride = 0;
  }
  // another case of converting to <N;1,0> region format
  if (!IsDest &&
      (R->VStride == (int)R->Width * R->Stride || R->Width == R->NumElements)) {
    R->Width = 1;
    R->VStride = R->Stride;
    R->Stride = 0;
  } else if (R->Width > MaxWidth) {
    // A Width of more than 16 (or whatever MaxWidth is) is not allowed. If it
    // is more than 16, then legalization has ensured that either there is one
    // row or the rows are contiguous (VStride == Width * Stride) and we can
    // increase the number of rows.  (Note that Width and VStride are ignored
    // in a destination operand; legalization ensures that there is only one
    // row.)
    R->Width = MaxWidth;
    R->VStride = R->Width * R->Stride;
  }

  if (R->Width == R->NumElements) {
    // Use VStride 0 on a 1D region. This is necessary for src0 in line or
    // pln, so we may as well do it for everything.
    R->VStride = 0;
  }

  if (R->Indirect) {
    R->IndirectAddrOffset = 0;
    if (GenXIntrinsic::isRdRegion(R->Indirect)) {
      auto AddrRdR = cast<Instruction>(R->Indirect);
      Region AddrR = makeRegionFromBaleInfo(AddrRdR, BaleInfo());
      IGC_ASSERT_MESSAGE(!AddrR.Indirect,
        "cannot have address rdregion that is indirect");
      R->IndirectAddrOffset =
          AddrR.Offset / 2; // address element is always 2 byte
    }
  }
}

VISA_VectorOpnd *
GenXKernelBuilder::createGeneralOperand(Region *R, VISA_GenVar *Decl,
                                        Signedness Signed, unsigned Mod,
                                        bool IsDest, unsigned MaxWidth) {
  VISA_VectorOpnd *ResultOperand = nullptr;
  // Write the vISA general operand, canonicalizing the
  // region parameters where applicable.
  IGC_ASSERT_MESSAGE(Decl, "no register allocated for this value");
  if (!IsDest) {
    ResultOperand = createCisaSrcOperand(
        Decl, static_cast<VISA_Modifier>(Mod), R->VStride, R->Width, R->Stride,
        R->Offset >> genx::log2(GrfByteSize),
        (R->Offset & (GrfByteSize - 1)) / R->ElementBytes);
  } else {
    ResultOperand = createCisaDstOperand(
        Decl, R->getDstStride(), R->Offset >> genx::log2(GrfByteSize),
        (R->Offset & (GrfByteSize - 1)) / R->ElementBytes);
  }
  return ResultOperand;
}

VISA_VectorOpnd *GenXKernelBuilder::createIndirectOperand(Region *R,
                                                          Signedness Signed,
                                                          unsigned Mod,
                                                          bool IsDest,
                                                          unsigned MaxWidth) {
  VISA_VectorOpnd *ResultOperand = nullptr;
  // Check if the indirect operand is a baled in rdregion.
  Value *Indirect = R->Indirect;
  if (GenXIntrinsic::isRdRegion(Indirect)) {
    auto AddrRdR = cast<Instruction>(Indirect);
    Indirect = AddrRdR->getOperand(0);
  }
  // Write the vISA indirect operand.
  Register *IdxReg = getRegForValueAndSaveAlias(Indirect, DONTCARESIGNED);
  IGC_ASSERT(IdxReg->Category == vc::RegCategory::Address);

  bool NotCrossGrf = !(R->Offset & (GrfByteSize - 1));
  if (!NotCrossGrf) {
    // Determine the NotCrossGrf bit setting (whether we can guarantee
    // that adding an indirect region's constant offset does not cause
    // a carry out of bit 4)
    // by looking at the partial constant for the index
    // before the constant is added on.
    // This only works for a scalar index.
    if (auto IndirInst = dyn_cast<Instruction>(R->Indirect)) {
      auto A = AI.get(IndirInst);
      unsigned Mask = (1U << std::min(5U, A.getLogAlign())) - 1;
      if (Mask) {
        if ((A.getExtraBits() & Mask) + (R->Offset & Mask) <= Mask &&
            (unsigned)(R->Offset & (GrfByteSize - 1)) <= Mask) {
          // The alignment and extrabits are such that adding R->Offset
          // cannot cause a carry from bit 4 to bit 5.
          NotCrossGrf = true;
        }
      }
    }
  }
  visa::TypeDetails TD(DL, R->ElementTy, Signed);
  unsigned VStride = R->VStride;
  if (isa<VectorType>(R->Indirect->getType()))
    // multi indirect (vector index), set vstride
    VStride = 0x8000; // field to null
  VISA_AddrVar *AddrDecl = IdxReg->GetVar<VISA_AddrVar>(Kernel);
  if (IsDest) {
    CISA_CALL(Kernel->CreateVISAIndirectDstOperand(
        ResultOperand, AddrDecl, R->IndirectAddrOffset, R->Offset,
        R->getDstStride(), (VISA_Type)TD.VisaType));
  } else {
    CISA_CALL(Kernel->CreateVISAIndirectSrcOperand(
        ResultOperand, AddrDecl, static_cast<VISA_Modifier>(Mod),
        R->IndirectAddrOffset, R->Offset, VStride, R->Width, R->Stride,
        (VISA_Type)TD.VisaType));
  }
  return ResultOperand;
}


/***********************************************************************
 * createRegionOperand : create a vISA region operand
 *
 * Enter:   R = Region
 *          RegNum = vISA register number (ignored if region is indirect)
 *          Signed = whether signed or unsigned required (only used for
 *                   indirect operand)
 *          Mod = modifiers
 *          IsDest = true if destination operand
 *          MaxWidth = maximum width (used to stop TWICEWIDTH operand
 *                     getting a width bigger than the execution size, but
 *                     for other uses defaults to 16)
 */
VISA_VectorOpnd *
GenXKernelBuilder::createRegionOperand(Region *R, VISA_GenVar *Decl,
                                       Signedness Signed, unsigned Mod,
                                       bool IsDest, unsigned MaxWidth) {
  deduceRegion(R, IsDest, MaxWidth);

  if (R->Indirect)
    return createIndirectOperand(R, Signed, Mod, IsDest, MaxWidth);
  else
    return createGeneralOperand(R, Decl, Signed, Mod, IsDest, MaxWidth);
}


bool GenXKernelBuilder::isInLoop(BasicBlock *BB) {
  Function *BBFunc = BB->getParent();
  // Cannot predict for stack calls and indirectly called functions.
  // Let's assume the function is in a loop.
  if (vc::requiresStackCall(BBFunc))
    return true;

  IGC_ASSERT(LIs->getLoopInfo(BBFunc));
  if (LIs->getLoopInfo(BBFunc)->getLoopFor(BB))
    return true; // inside loop in this function
  // Now we need to see if this function is called from inside a loop.
  // First check the cache.
  auto i = IsInLoopCache.find(BBFunc);
  if (i != IsInLoopCache.end())
    return i->second;
  // Now check all call sites. This recurses as deep as the depth of the call
  // graph, which must be acyclic as GenX does not allow recursion.
  bool InLoop = false;
  for (auto ui = BBFunc->use_begin(), ue = BBFunc->use_end(); ui != ue; ++ui) {
    auto CI = dyn_cast<CallInst>(ui->getUser());
    if (!checkFunctionCall(CI, BBFunc))
      continue;
    IGC_ASSERT(ui->getOperandNo() == IGCLLVM::getNumArgOperands(CI));
    if (CI->getFunction() == BBFunc)
      continue;
    if (isInLoop(CI->getParent())) {
      InLoop = true;
      break;
    }
  }
  IsInLoopCache[BBFunc] = InLoop;
  return InLoop;
}

void GenXKernelBuilder::addWriteRegionLifetimeStartInst(Instruction *WrRegion) {
  if (!GenXIntrinsic::isWrRegion(WrRegion))
    return; // No lifetime start for wrpredregion.
  // See if the wrregion is in a loop.
  auto BB = WrRegion->getParent();
  if (!isInLoop(BB))
    return; // not in loop
  // See if the wrregion is the first of a sequence in the same basic block
  // that together write the whole register. We assume that each region is
  // contiguous, and the regions are written in ascending offset order, as
  // that is what legalization does if the original write was to the whole
  // register.
  unsigned NumElementsSoFar = 0;
  unsigned TotalNumElements = 1;
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(WrRegion->getType()))
    TotalNumElements = VT->getNumElements();
  Instruction *ThisWr = WrRegion;
  for (;;) {
    Region R = makeRegionFromBaleInfo(ThisWr, BaleInfo());
    if (R.Indirect)
      break;
    if ((unsigned)R.Offset != NumElementsSoFar * R.ElementBytes)
      break;
    if (R.Stride != 1 && R.Width != 1)
      break;
    if (R.Width != R.NumElements)
      break;
    NumElementsSoFar += R.NumElements;
    if (NumElementsSoFar == TotalNumElements)
      return; // whole register is written
    // Go on to next wrregion in the same basic block if any.
    if (!ThisWr->hasOneUse())
      break;
    ThisWr = cast<Instruction>(ThisWr->use_begin()->getUser());
    if (!GenXIntrinsic::isWrRegion(ThisWr))
      break;
    if (ThisWr->getParent() != BB)
      break;
  }
  // The wrregion is in a loop and is not the first in a sequence in the same
  // basic block that writes the whole register. Write a lifetime start.
  addLifetimeStartInst(WrRegion);
}

/**************************************************************************************************
 * addLifetimeStartInst : add a lifetime.start instruction
 *
 * Enter:   Inst = value to use in lifetime.start
 */
void GenXKernelBuilder::addLifetimeStartInst(Instruction *Inst) {
  VISA_VectorOpnd *opnd = nullptr;
  auto Reg = getRegForValueOrNullAndSaveAlias(Inst);
  if (!Reg)
    return; // no register allocated such as being indirected.

  switch (Reg->Category) {
  case vc::RegCategory::General:
    opnd = createCisaDstOperand(Reg->GetVar<VISA_GenVar>(Kernel), 1, 0, 0);
    break;
  case vc::RegCategory::Address:
    CISA_CALL(Kernel->CreateVISAAddressDstOperand(
        opnd, Reg->GetVar<VISA_AddrVar>(Kernel), 0));
    break;
#if 0  // Not currently used.
    case vc::RegCategory::Predicate:
      break;
#endif // 0
  default:
    report_fatal_error("createLifetimeStartInst: Invalid register category");
    break;
  }
  CISA_CALL(Kernel->AppendVISALifetime(LIFETIME_START, opnd));
}

/***********************************************************************
 * emitFileAndLocVisa : emit special visa instructions LOC and FILE
 *
 */
void GenXKernelBuilder::emitFileAndLocVisa(Instruction *CurrentInst) {
  if (OptDisableVisaLOC)
    return;
  // Make the source location pending, so it is output as vISA FILE and LOC
  // instructions next time an opcode is written.
  const DebugLoc &DL = CurrentInst->getDebugLoc();
  if (!DL || isa<DbgInfoIntrinsic>(CurrentInst))
    return;

  StringRef Filename = DL->getFilename();
  if (Filename != "") {
    // Check if we have a pending debug location.
    StringRef PendingDirectory = DL->getDirectory();
    // Do the source location debug info with vISA FILE and LOC instructions.
    if (Filename != LastFilename || PendingDirectory != LastDirectory) {
      SmallString<256> EmittedFilename = Filename;
      if (!sys::path::is_absolute(Filename)) {
        EmittedFilename = PendingDirectory;
        sys::path::append(EmittedFilename, Filename);
      }
      LLVM_DEBUG(dbgs() << "FILENAME instruction append " << EmittedFilename
                        << "\n");
      CISA_CALL(Kernel->AppendVISAMiscFileInst(EmittedFilename.c_str()));
      LastDirectory = PendingDirectory;
      LastFilename = Filename;
    }
  }

  unsigned PendingLine = DL.getLine();
  if (PendingLine != LastEmittedVisaLine) {
    LLVM_DEBUG(dbgs() << "LOC instruction appended:" << PendingLine << "\n");
    CISA_CALL(Kernel->AppendVISAMiscLOC(PendingLine));
    LastEmittedVisaLine = PendingLine;
  }

  LLVM_DEBUG(dbgs() << "Visa inst current count = "
                    << Kernel->getvIsaInstCount() << "\n");
}

/***********************************************************************
 * addDebugInfo : add debug infromation
 *
 * Enter:   CurrentInst = llvm-instruction for add to mapping
 *          Finalize = if true - updating only count of instructions for
 *                     current visa-llvm mapping, else create new
 *                     visa-llvm mapping element
 */
void GenXKernelBuilder::addDebugInfo(Instruction *CurrentInst, bool Finalize) {
  LLVM_DEBUG(dbgs() << " ----- gen VisaDebug Info for visa inst count = "
                    << Kernel->getvIsaInstCount() << " Finalize = " << Finalize
                    << "\n");

  // +1 since we update debug info BEFORE appending the instruction
  auto CurrentCount = Kernel->getvIsaInstCount() + 1;

  IGC_ASSERT(CurrentInst);
  auto Reason = CurrentInst->getName();
  if (!Finalize) {
    GM->updateVisaMapping(KernFunc, CurrentInst, CurrentCount, Reason);
    return;
  }
  // Do not modify debug-instructions count
  if (isa<DbgInfoIntrinsic>(CurrentInst)) {
    return;
  }
  LLVM_DEBUG(dbgs() << "Update visa map for next inst with id = "
                    << CurrentCount << "\n");
  GM->updateVisaCountMapping(KernFunc, CurrentInst, CurrentCount, Reason);
}

void GenXKernelBuilder::emitOptimizationHints() {
  if (skipOptWithLargeBlock(*FG))
    return;

  // Track rp considering byte variable widening.
  PressureTracker RP(DL, *FG, Liveness, /*ByteWidening*/ true);
  const std::vector<genx::LiveRange *> &WidenLRs = RP.getWidenVariables();

  if (!SkipNoWiden) {
    for (auto LR : WidenLRs) {
      SimpleValue SV = *LR->value_begin();
      auto *R = getRegForValueOrNullAndSaveAlias(SV);
      // This variable is being used in or crossing a high register pressure
      // region. Set an optimization hint not to widen it.
      if (R && RP.intersectWithRedRegion(LR)) {
        R->addAttribute(addStringToPool("NoWidening"), "");
        RP.decreasePressure(LR);
      }
    }
  }
}

/***********************************************************************
 * addLabelInst : add a label instruction for a basic block or join
 */
void GenXKernelBuilder::addLabelInst(const Value *BB) {
  auto LabelID = getOrCreateLabel(BB, LABEL_BLOCK);
  IGC_ASSERT(LabelID < Labels.size());
  CISA_CALL(Kernel->AppendVISACFLabelInst(Labels[LabelID]));
}

/***********************************************************************
 * getOrCreateLabel : get/create label number for a Function or BasicBlock
 */
unsigned GenXKernelBuilder::getOrCreateLabel(const Value *V, int Kind) {
  int Num = getLabel(V);
  if (Num >= 0)
    return Num;
  Num = Labels.size();
  setLabel(V, Num);
  VISA_LabelOpnd *Decl = nullptr;

  // Replicate the functionality of the old compiler and make the first label
  // for a function contain the name (makes sure the function label is unique)
  // It's not clear this is strictly necessary any more (but doesn't do any
  // harm and may even make reading the intermediate forms easier)
  if (Kind == LABEL_SUBROUTINE) {
    StringRef N = TheKernelMetadata.getName();
    std::string NameBuf;
    if (V != FG->getHead()) {
      // This is a subroutine, not the kernel/function at the head of the
      // FunctionGroup. Use the name of the subroutine.
      N = V->getName();
    } else {
      // For a kernel/function name, fix illegal characters. The jitter uses
      // the same name for the label in the .asm file, and aubload does not
      // like the illegal characters.
      NameBuf = legalizeName(N.str());
      N = NameBuf;
    }
    auto SubroutineLabel =
        cutString(Twine(N) + Twine("_BB_") + Twine(Labels.size()));
    LLVM_DEBUG(dbgs() << "creating SubroutineLabel: " << SubroutineLabel
                      << "\n");
    CISA_CALL(Kernel->CreateVISALabelVar(Decl, SubroutineLabel.c_str(),
                                         VISA_Label_Kind(Kind)));
  } else if (Kind == LABEL_BLOCK) {
    auto BlockLabel = cutString(Twine("BB_") + Twine(Labels.size()));
    LLVM_DEBUG(dbgs() << "creating BlockLabel: " << BlockLabel << "\n");
    CISA_CALL(Kernel->CreateVISALabelVar(Decl, BlockLabel.c_str(),
                                         VISA_Label_Kind(Kind)));
  } else if (Kind == LABEL_FC) {
    const auto *F = cast<Function>(V);
    IGC_ASSERT(F->hasFnAttribute("CMCallable"));
    StringRef N(F->getName());
    auto FCLabel = cutString(Twine(N));
    LLVM_DEBUG(dbgs() << "creating FCLabel: " << FCLabel << "\n");
    CISA_CALL(Kernel->CreateVISALabelVar(Decl, FCLabel.c_str(),
                                         VISA_Label_Kind(Kind)));
  } else {
    StringRef N = V->getName();
    auto Label =
        cutString(Twine("_") + Twine(N) + Twine("_") + Twine(Labels.size()));
    LLVM_DEBUG(dbgs() << "creating Label: " << Label << "\n");
    CISA_CALL(
        Kernel->CreateVISALabelVar(Decl, Label.c_str(), VISA_Label_Kind(Kind)));
  }
  IGC_ASSERT(Decl);
  Labels.push_back(Decl);
  return Num;
}

void GenXKernelBuilder::buildInlineAsm(CallInst *CI) {
  IGC_ASSERT_MESSAGE(CI->isInlineAsm(), "Inline asm expected");
  InlineAsm *IA = dyn_cast<InlineAsm>(IGCLLVM::getCalledValue(CI));
  std::string AsmStr(IA->getAsmString());
  std::stringstream &AsmTextStream = CisaBuilder->GetAsmTextStream();

  // Nothing to substitute if no constraints provided
  if (IA->getConstraintString().empty()) {
    AsmTextStream << AsmStr << std::endl;
    return;
  }

  unsigned NumOutputs = genx::getInlineAsmNumOutputs(CI);
  auto ConstraintsInfo = genx::getGenXInlineAsmInfo(CI);

  // Scan asm string in reverse direction to match larger numbers first
  for (int ArgNo = ConstraintsInfo.size() - 1; ArgNo >= 0; ArgNo--) {
    // Regexp to match number of operand
    Regex R("\\$+" + llvm::to_string(ArgNo));
    if (!R.match(AsmStr))
      continue;
    // Operand that must be substituded into inline assembly string
    Value *InlasmOp = nullptr;
    std::string InlasmOpAsString;
    // For output collect destination descriptor with
    // baling info and WrRegion instruction
    DstOpndDesc DstDesc;
    auto Info = ConstraintsInfo[ArgNo];
    if (Info.isOutput()) {
      // If result is a struct than inline assembly
      // instruction has multiple outputs
      if (isa<StructType>(CI->getType())) {
        // Go through all users of a result and find extractelement with
        // ArgNo indice: ArgNo is a number of a constraint in constraint
        // list
        for (auto ui = CI->use_begin(), ue = CI->use_end(); ui != ue; ++ui) {
          auto EV = dyn_cast<ExtractValueInst>(ui->getUser());
          if (EV && (EV->getIndices()[0] == ArgNo)) {
            InlasmOp = EV;
            break;
          }
        }
      } else
        // Single output
        InlasmOp = CI;

      if (InlasmOp) {
        Instruction *Inst = cast<Instruction>(InlasmOp);
        Instruction *Head = Baling->getBaleHead(Inst);
        BaleInfo BI = Baling->getBaleInfo(Head);
        // If head is g_store than change head to store's
        //  operand and check if it's baled wrr
        if (BI.Type == BaleInfo::GSTORE) {
          DstDesc.GStore = Head;
          Head = cast<Instruction>(Head->getOperand(0));
          BI = Baling->getBaleInfo(Head);
        }
        if (BI.Type == BaleInfo::WRREGION) {
          DstDesc.WrRegion = Head;
          DstDesc.WrRegionBI = BI;
        }
        InlasmOpAsString = createInlineAsmDestinationOperand(
            CI, InlasmOp, DONTCARESIGNED, Info.getConstraintType(), 0, DstDesc);
      } else {
        // Can't deduce output operand because there are no users
        // but we have register allocated. If region is needed we can use
        // default one based one type.
        SimpleValue SV(CI, ArgNo);
        Register *Reg = getRegForValueAndSaveAlias(SV, DONTCARESIGNED);
        Region R(SV.getType());
        InlasmOpAsString =
            createInlineAsmOperand(CI, Reg, &R, true /*IsDst*/, DONTCARESIGNED,
                                   Info.getConstraintType(), 0);
      }
    } else {
      // Input of inline assembly
      InlasmOp = CI->getArgOperand(ArgNo - NumOutputs);
      bool IsBaled = false;
      if (GenXIntrinsic::isRdRegion(InlasmOp)) {
        Instruction *RdR = cast<Instruction>(InlasmOp);
        IsBaled = Baling->isBaled(RdR);
      }
      InlasmOpAsString = createInlineAsmSourceOperand(
          CI, InlasmOp, DONTCARESIGNED, IsBaled, Info.getConstraintType());
    }
    // Substitute string name of the variable until
    // there are no possible sustitutions. Do-while
    // since first match was checked in the beginning
    // of the loop.
    do {
      AsmStr = R.sub(InlasmOpAsString, AsmStr);
    } while (R.match(AsmStr));
  }

  AsmTextStream << "\n// INLASM BEGIN\n"
                << AsmStr << "\n// INLASM END\n"
                << std::endl;
}

void GenXKernelBuilder::buildCall(CallInst *CI, const DstOpndDesc &DstDesc) {
  LLVM_DEBUG(dbgs() << CI << "\n");
  Function *Callee = CI->getCalledFunction();
  if (!Callee || vc::requiresStackCall(Callee)) {
    buildStackCallLight(CI, DstDesc);
    return;
  }

  unsigned LabelKind = LABEL_SUBROUTINE;
  if (Callee->hasFnAttribute("CMCallable"))
    LabelKind = LABEL_FC;
  else
    IGC_ASSERT_MESSAGE(FG == FG->getParent()->getAnyGroup(Callee),
                       "unexpected call to outside FunctionGroup");

  // Check whether the called function has a predicate arg that is EM.
  int EMOperandNum = -1;
  for (auto ai = Callee->arg_begin(), ae = Callee->arg_end(); ai != ae; ++ai) {
    auto Arg = &*ai;
    if (!Arg->getType()->getScalarType()->isIntegerTy(1))
      continue;
    if (Liveness->getLiveRange(Arg)->getCategory() == vc::RegCategory::EM) {
      EMOperandNum = Arg->getArgNo();
      break;
    }
  }

  if (EMOperandNum < 0) {
    // Scalar calls must be marked with NoMask
    CISA_CALL(Kernel->AppendVISACFCallInst(
        nullptr, vISA_EMASK_M1_NM, EXEC_SIZE_1,
        Labels[getOrCreateLabel(Callee, LabelKind)]));
  } else {
    auto PredicateOpnd = NoMask ? nullptr : createPred(CI, BaleInfo(), EMOperandNum);
    auto *VTy = cast<IGCLLVM::FixedVectorType>(
        CI->getArgOperand(EMOperandNum)->getType());
    VISA_Exec_Size ExecSize = getExecSizeFromValue(VTy->getNumElements());
    CISA_CALL(Kernel->AppendVISACFCallInst(
        PredicateOpnd, vISA_EMASK_M1, ExecSize,
        Labels[getOrCreateLabel(Callee, LabelKind)]));
  }
}

void GenXKernelBuilder::buildRet(ReturnInst *RI) {
  uint32_t FloatControl = 0;
  auto F = RI->getFunction();
  F->getFnAttribute(genx::FunctionMD::CMFloatControl)
      .getValueAsString()
      .getAsInteger(0, FloatControl);
  FloatControl &= CRMask;
  if (FloatControl != DefaultFloatControl) {
    buildControlRegUpdate(CRMask, true);
    if (DefaultFloatControl)
      buildControlRegUpdate(DefaultFloatControl, false);
  }
  if (vc::requiresStackCall(Func)) {
    CISA_CALL(Kernel->AppendVISACFFunctionRetInst(nullptr, vISA_EMASK_M1,
                                                  StackCallExecSize));
  } else {
    CISA_CALL(Kernel->AppendVISACFRetInst(nullptr, vISA_EMASK_M1, EXEC_SIZE_1));
  }
}

/***********************************************************************
 * createRawSourceOperand : create raw source operand of instruction
 *
 * Enter:   Inst = instruction to get source operand from
 *          OperandNum = operand number
 *          BI = BaleInfo for Inst (so we can tell whether a rdregion
 *                  or modifier is bundled in)
 */
VISA_RawOpnd *GenXKernelBuilder::createRawSourceOperand(const Instruction *Inst,
                                                        unsigned OperandNum,
                                                        BaleInfo BI,
                                                        Signedness Signed) {
  VISA_RawOpnd *ResultOperand = nullptr;
  Value *V = Inst->getOperand(OperandNum);
  if (isa<UndefValue>(V)) {
    CISA_CALL(Kernel->CreateVISANullRawOperand(ResultOperand, false));
  } else {
    unsigned ByteOffset = 0;
    bool Baled = Baling->getBaleInfo(Inst).isOperandBaled(OperandNum);
    if (Baled) {
      Instruction *RdRegion = cast<Instruction>(V);
      Region R = makeRegionFromBaleInfo(RdRegion, BaleInfo());
      ByteOffset = R.Offset;
      V = RdRegion->getOperand(0);
    }
    LLVM_DEBUG(dbgs() << "createRawSourceOperand for "
                      << (Baled ? "baled" : "non-baled") << " value: ");
    LLVM_DEBUG(V->dump());
    LLVM_DEBUG(dbgs() << "\n");
    Register *Reg = getRegForValueAndSaveAlias(V, Signed);
    IGC_ASSERT(Reg->Category == vc::RegCategory::General);
    LLVM_DEBUG(dbgs() << "CreateVISARawOperand: "; Reg->print(dbgs()); dbgs() << "\n");
    CISA_CALL(Kernel->CreateVISARawOperand(
        ResultOperand, Reg->GetVar<VISA_GenVar>(Kernel), ByteOffset));
  }
  return ResultOperand;
}

/***********************************************************************
 * createRawDestination : create raw destination operand
 *
 * Enter:   Inst = destination value
 *          WrRegion = 0 else wrregion that destination is baled into
 *
 * A raw destination can be baled into a wrregion, but only if the region
 * is direct and its start index is GRF aligned.
 */
VISA_RawOpnd *
GenXKernelBuilder::createRawDestination(Value *V, const DstOpndDesc &DstDesc,
                                        Signedness Signed) {
  VISA_RawOpnd *ResultOperand = nullptr;
  unsigned ByteOffset = 0;
  if (DstDesc.WrRegion) {
    V = DstDesc.WrRegion;
    Region R = makeRegionFromBaleInfo(DstDesc.WrRegion, BaleInfo());
    ByteOffset = R.Offset;
  }
  Type *OverrideType = nullptr;
  if (DstDesc.GStore) {
    V = vc::getUnderlyingGlobalVariable(DstDesc.GStore->getOperand(1));
    IGC_ASSERT_MESSAGE(V, "out of sync");
    OverrideType = DstDesc.GStore->getOperand(0)->getType();
  }
  LLVM_DEBUG(dbgs() << "createRawDestination for "
                    << (DstDesc.GStore ? "global" : "non-global") << " value: ");
  LLVM_DEBUG(V->dump());
  LLVM_DEBUG(dbgs() << "\n");
  if (DstDesc.WrPredefReg)
    V = DstDesc.WrPredefReg;
  Register *Reg = getRegForValueOrNullAndSaveAlias(V, Signed, OverrideType);
  if (!Reg) {
    // No register assigned. This happens to an unused raw result where the
    // result is marked as RAW_NULLALLOWED in GenXIntrinsics.
    CISA_CALL(Kernel->CreateVISANullRawOperand(ResultOperand, true));
  } else {
    IGC_ASSERT(Reg->Category == vc::RegCategory::General);
    LLVM_DEBUG(dbgs() << "CreateVISARawOperand: "; Reg->print(dbgs()); dbgs() << "\n");
    CISA_CALL(Kernel->CreateVISARawOperand(
        ResultOperand, Reg->GetVar<VISA_GenVar>(Kernel), ByteOffset));
  }
  return ResultOperand;
}

/***********************************************************************
 * getLabel : get label number for a Function or BasicBlock
 *
 * Return:  label number, -1 if none found
 */
int GenXKernelBuilder::getLabel(const Value *V) const {
  auto It = LabelMap.find(V);
  if (It != LabelMap.end())
    return It->second;
  return -1;
}

/***********************************************************************
 * setLabel : set the label number for a Function or BasicBlock
 */
void GenXKernelBuilder::setLabel(const Value *V, unsigned Num) {
  LabelMap[V] = Num;
}

unsigned GenXKernelBuilder::addStringToPool(StringRef Str) {
  auto val = std::pair<std::string, unsigned>(Str.begin(), StringPool.size());
  auto Res = StringPool.insert(val);
  return Res.first->second;
}

StringRef GenXKernelBuilder::getStringByIndex(unsigned Val) {
  for (const auto &it : StringPool) {
    if (it.second == Val)
      return it.first;
  }
  IGC_ASSERT_UNREACHABLE(); // Can't find string by index.
}

/***********************************************************************
 * Get size of the argument of type 'type' in bytes considering layout of
 * subtypes of aggregate type in units of size 'mod'
 * mod is typically 32 (GRF) or 16 (oword)
 */
unsigned GenXKernelBuilder::getValueSize(Type *T, unsigned Mod) const {
  unsigned Result = 0;
  if (T->isAggregateType()) {
    for (unsigned i = 0; i < T->getStructNumElements(); i++) {
      Result += getValueSize(T->getContainedType(i)) / Mod +
                (getValueSize(T->getContainedType(i)) % Mod ? 1 : 0);
    }
    Result *= Mod;
  } else
    Result = DL.getTypeSizeInBits(T) / 8;
  return Result;
}

unsigned GenXKernelBuilder::getFuncArgsSize(llvm::Function *F) {
  unsigned Result = 0;
  for (auto &Arg : F->args())
    Result += getValueSize(&Arg);
  return Result;
}

GenericCisaVariable *
GenXKernelBuilder::createCisaVariable(VISAKernel *Kernel, const char *Name,
                                      VISA_GenVar *AliasVar,
                                      unsigned ByteSize) {
  auto it = CisaVars[Kernel].find(Name);
  if (it != CisaVars[Kernel].end())
    it->second = GenericCisaVariable(Name, AliasVar, ByteSize);
  else
    CisaVars[Kernel].insert(
        std::make_pair(Name, GenericCisaVariable(Name, AliasVar, ByteSize)));
  return &(CisaVars[Kernel].at(Name));
}

static unsigned deduceByteSize(Value *V, const DataLayout &DL) {
  return DL.getTypeSizeInBits(V->getType()->getScalarType()) / 8;
}

static unsigned deduceByteSize(CisaVariable *V, const DataLayout &DL) {
  IGC_ASSERT(V->getType() < ISA_TYPE_NUM);
  return CISATypeTable[V->getType()].typeSize;
}

/**************************************************************************************************
 * emitVectorCopy : emit vISA that performs copying form Dst to Src
 *
 * Emit sufficient amount of MOVs from Dst to Src picking size in a greedy manner
 *
 * T1 and T2 should be llvm::Value and CisaVariable or vice-versa,
 * CisaVariable=>CisaVariable or Value=>Value copying is not supported here
 *
 */
template <typename T1, typename T2>
void GenXKernelBuilder::emitVectorCopy(T1 *Dst, T2 *Src, unsigned &RowOff,
                                       unsigned &ColOff, unsigned &SrcRowOff,
                                       unsigned &SrcColOff, int TotalSize,
                                       bool DoCopy) {
  IGC_ASSERT(Subtarget);
  auto partCopy = [&, GRFWidth = Subtarget->getGRFByteSize()](int Sz) {
    int ByteSz = Sz * deduceByteSize(Dst, DL);
    IGC_ASSERT(ByteSz);

    unsigned Start = SrcRowOff;
    unsigned End = (SrcRowOff * GRFWidth + SrcColOff + ByteSz) / GRFWidth;

    // mov is prohibited to span across >2 GRF
    if (End - Start >= 2) {
      IGC_ASSERT(Sz > 1);
      return;
    }

    while (TotalSize >= ByteSz) {
      VISA_VectorOpnd *ArgSrc = nullptr, *ArgDst = nullptr;
      unsigned Offset = SrcRowOff * GrfByteSize + SrcColOff;
      ArgSrc = createSource(Src, UNSIGNED, Sz, &Offset);
      SrcRowOff += (SrcColOff + ByteSz) / GrfByteSize;
      SrcColOff = (SrcColOff + ByteSz) % GrfByteSize;

      Offset = RowOff * GrfByteSize + ColOff;
      ArgDst = createDestination(Dst, UNSIGNED, &Offset);
      RowOff += (ColOff + ByteSz) / GrfByteSize;
      ColOff = (ColOff + ByteSz) % GrfByteSize;

      if (DoCopy)
        CISA_CALL(Kernel->AppendVISADataMovementInst(
            ISA_MOV, nullptr, false,
            (NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1),
            getExecSizeFromValue(Sz), ArgDst, ArgSrc));
      TotalSize -= ByteSz;
    }
  };
  partCopy(16);
  partCopy(8);
  partCopy(4);
  partCopy(2);
  partCopy(1);
}

void GenXKernelBuilder::pushStackArg(VISA_StateOpndHandle *Dst, Value *Src,
                                     int TotalSz, unsigned &RowOff,
                                     unsigned &ColOff, unsigned &SrcRowOff,
                                     unsigned &SrcColOff, bool DoCopy) {
  VISA_GenVar *StackOff = nullptr, *Sp = nullptr;

  auto StackTmp = createCisaVariable(Kernel, "stackTmp", nullptr, TotalSz);

  auto TmpType = llvmToVisaType(Src->getType());
  auto TmpVar = StackTmp->getAlias(TmpType, Kernel);

  CISA_CALL(Kernel->CreateVISAGenVar(StackOff, "stackOff", 1, ISA_TYPE_UQ,
                                     ALIGN_OWORD));
  unsigned RawOff = 0;
  auto partCopy = [&](int Sz) {
    // TODO: mb we have some constant for oword size
    int ByteSz = Sz * visa::BytesPerOword;
    int CopySz = std::min(ByteSz, TotalSz);

    while (TotalSz - ByteSz >= 0 || (TotalSz > 0 && Sz == 1)) {
      CISA_CALL(Kernel->GetPredefinedVar(Sp, PREDEFINED_FE_SP));
      VISA_VectorOpnd *SpOpSrc1 = nullptr;
      VISA_VectorOpnd *SpOpSrc2 = nullptr;
      VISA_VectorOpnd *SpOpDst = nullptr;
      CISA_CALL(Kernel->CreateVISADstOperand(SpOpDst, Sp, 1, 0, 0));
      CISA_CALL(Kernel->CreateVISASrcOperand(SpOpSrc1, Sp, MODIFIER_NONE, 0, 1,
                                             0, 0, 0));
      CISA_CALL(Kernel->CreateVISASrcOperand(SpOpSrc2, Sp, MODIFIER_NONE, 0, 1,
                                             0, 0, 0));

      VISA_VectorOpnd *TmpOffDst = nullptr, *TmpOffSrc = nullptr;
      CISA_CALL(Kernel->CreateVISADstOperand(TmpOffDst, StackOff, 1, 0, 0));
      CISA_CALL(Kernel->CreateVISASrcOperand(TmpOffSrc, StackOff, MODIFIER_NONE,
                                             0, 1, 0, 0, 0));

      emitVectorCopy(TmpVar, Src, RowOff, ColOff, SrcRowOff, SrcColOff, CopySz,
                     DoCopy);
      VISA_VectorOpnd *Imm = nullptr;
      unsigned OffVal = Sz;
      CISA_CALL(Kernel->CreateVISAImmediate(Imm, &OffVal, ISA_TYPE_UD));
      VISA_RawOpnd *RawSrc = nullptr;
      CISA_CALL(
          Kernel->CreateVISARawOperand(RawSrc, TmpVar->getGenVar(), RawOff));
      RawOff += Sz * visa::BytesPerOword;

      if (DoCopy) {
        CISA_CALL(Kernel->AppendVISADataMovementInst(ISA_MOV, nullptr, false,
                                                     vISA_EMASK_M1, EXEC_SIZE_1,
                                                     TmpOffDst, SpOpSrc1));
        CISA_CALL(Kernel->AppendVISASurfAccessOwordLoadStoreInst(
            ISA_OWORD_ST, vISA_EMASK_M1, Dst, getCisaOwordNumFromNumber(Sz),
            TmpOffSrc, RawSrc));
      }
      CISA_CALL(Kernel->AppendVISAArithmeticInst(ISA_ADD, nullptr, false,
                                                 vISA_EMASK_M1, EXEC_SIZE_1,
                                                 SpOpDst, SpOpSrc2, Imm));
      TotalSz -= ByteSz;
    }
  };

  partCopy(8);
  partCopy(4);
  partCopy(2);
  partCopy(1);
}

void GenXKernelBuilder::popStackArg(llvm::Value *Dst, VISA_StateOpndHandle *Src,
                                    int TotalSz, unsigned &RowOff,
                                    unsigned &ColOff, unsigned &SrcRowOff,
                                    unsigned &SrcColOff, int &PrevStackOff) {
  VISA_GenVar *StackOff = nullptr, *Sp = nullptr;

  auto StackTmp = createCisaVariable(Kernel, "stackTmp", nullptr, TotalSz);

  auto TmpType = llvmToVisaType(Dst->getType());
  auto TmpVar = StackTmp->getAlias(TmpType, Kernel);

  CISA_CALL(Kernel->CreateVISAGenVar(StackOff, "stackOff", 1, ISA_TYPE_UQ,
                                     ALIGN_OWORD));
  auto partCopy = [&](int Sz) {
    // TODO: mb we have some constant for oword size
    int ByteSz = Sz * visa::BytesPerOword;
    while (TotalSz - ByteSz >= 0 || (TotalSz > 0 && Sz == 1)) {
      CISA_CALL(Kernel->GetPredefinedVar(Sp, PREDEFINED_FE_SP));
      VISA_VectorOpnd *SpOpSrc = nullptr;
      CISA_CALL(Kernel->CreateVISASrcOperand(SpOpSrc, Sp, MODIFIER_NONE, 0, 1,
                                             0, 0, 0));

      VISA_VectorOpnd *TmpOffDst = nullptr;
      VISA_VectorOpnd *TmpOffSrc = nullptr;
      CISA_CALL(Kernel->CreateVISADstOperand(TmpOffDst, StackOff, 1, 0, 0));
      CISA_CALL(Kernel->CreateVISASrcOperand(TmpOffSrc, StackOff, MODIFIER_NONE,
                                             0, 1, 0, 0, 0));

      VISA_VectorOpnd *Imm = nullptr;
      int OffVal = PrevStackOff;
      CISA_CALL(Kernel->CreateVISAImmediate(Imm, &OffVal, ISA_TYPE_UD));
      PrevStackOff += Sz;
      VISA_RawOpnd *RawSrc = nullptr;
      CISA_CALL(Kernel->CreateVISARawOperand(RawSrc, TmpVar->getGenVar(), 0));

      CISA_CALL(Kernel->AppendVISAArithmeticInst(ISA_ADD, nullptr, false,
                                                 vISA_EMASK_M1, EXEC_SIZE_1,
                                                 TmpOffDst, SpOpSrc, Imm));
      CISA_CALL(Kernel->AppendVISASurfAccessOwordLoadStoreInst(
          ISA_OWORD_LD, vISA_EMASK_M1, Src, getCisaOwordNumFromNumber(Sz),
          TmpOffSrc, RawSrc));
      int CopySz = std::min(ByteSz, TotalSz);
      SrcRowOff = SrcColOff = 0;
      emitVectorCopy(Dst, TmpVar, RowOff, ColOff, SrcRowOff, SrcColOff, CopySz);
      TotalSz -= ByteSz;
    }
    SrcRowOff = SrcColOff = 0;
  };

  partCopy(8);
  partCopy(4);
  partCopy(2);
  partCopy(1);
}

void GenXKernelBuilder::beginFunctionLight(Function *Func) {
  if (vc::isKernel(Func))
    return;
  if (!vc::requiresStackCall(Func))
    return;
  if (vc::isIndirect(Func) && !BackendConfig->directCallsOnly(Func->getName())) {
    int ExtVal = 1;
    Kernel->AddKernelAttribute("Extern", 4, &ExtVal);
  }
  // stack function prologue
  auto *MDArg = Func->getMetadata(vc::InstMD::FuncArgSize);
  auto *MDRet = Func->getMetadata(vc::InstMD::FuncRetSize);
  IGC_ASSERT(MDArg && MDRet);
  auto ArgSize =
      cast<ConstantInt>(
          cast<ConstantAsMetadata>(MDArg->getOperand(0).get())->getValue())
          ->getZExtValue();
  auto RetSize =
      cast<ConstantInt>(
          cast<ConstantAsMetadata>(MDRet->getOperand(0).get())->getValue())
          ->getZExtValue();

  auto *StackCallee = Func2Kern[Func];
  StackCallee->SetFunctionInputSize(ArgSize);
  StackCallee->SetFunctionReturnSize(RetSize);
  StackCallee->AddKernelAttribute("ArgSize", 1, &ArgSize);
  StackCallee->AddKernelAttribute("RetValSize", 1, &RetSize);
}

void GenXKernelBuilder::buildExtractRetv(ExtractValueInst *Inst) {
  auto T = Inst->getOperand(0)->getType();
  auto *RetVar = &CisaVars[Kernel].at("retv");

  bool UseStack = getValueSize(T) > RetVar->getByteSize();

  auto Index = Inst->getIndices().front();
  if (T->getContainedType(Index)->isVectorTy() &&
      cast<VectorType>(T->getContainedType(Index))
          ->getElementType()
          ->isIntegerTy(1))
    // elements of <N x i1> type should be ignored
    return;

  unsigned RowOff = 0, ColOff = 0;
  unsigned SrcRowOff = 0, SrcColOff = 0;
  for (unsigned i = 0; i < Index; i++) {
    int Mod = UseStack ? visa::BytesPerOword : GrfByteSize;
    SrcRowOff += (getValueSize(T->getContainedType(i)) + Mod - 1) / Mod;
  }

  if (UseStack) {
    int Prev = SrcRowOff;
    VISA_StateOpndHandle *StackSurfOp = nullptr;
    VISA_SurfaceVar *StackSurfVar = nullptr;
    CISA_CALL(
        Kernel->GetPredefinedSurface(StackSurfVar, StackSurf));
    CISA_CALL(Kernel->CreateVISAStateOperandHandle(StackSurfOp, StackSurfVar));
    popStackArg(Inst, StackSurfOp, getValueSize(T->getContainedType(Index)),
                RowOff, ColOff, SrcRowOff, SrcColOff, Prev);
  } else
    emitVectorCopy(Inst, RetVar->getAlias(Inst, Kernel), RowOff, ColOff,
                   SrcRowOff, SrcColOff, getValueSize(Inst));
}

void GenXKernelBuilder::buildInsertRetv(InsertValueInst *Inst) {
  auto T = Inst->getOperand(0)->getType();
  auto *RetVar = &CisaVars[Kernel].at("retv");

  bool UseStack = getValueSize(T) > RetVar->getByteSize();

  auto Index = Inst->getIndices().front();
  if (T->getContainedType(Index)->isVectorTy() &&
      cast<VectorType>(T->getContainedType(Index))
          ->getElementType()
          ->isIntegerTy(1)) {
    // elements of <N x i1> type should be ignored
    return;
  }

  unsigned RowOff = 0, ColOff = 0;
  unsigned SrcRowOff = 0, SrcColOff = 0;

  if (!UseStack)
    for (unsigned i = 0; i < Index; i++)
      RowOff += (getValueSize(T->getContainedType(i)) + GrfByteSize - 1) /
                GrfByteSize;

  if (UseStack) {
    VISA_StateOpndHandle *StackSurfOp = nullptr;
    VISA_SurfaceVar *StackSurfVar = nullptr;
    CISA_CALL(
        Kernel->GetPredefinedSurface(StackSurfVar, StackSurf));
    CISA_CALL(Kernel->CreateVISAStateOperandHandle(StackSurfOp, StackSurfVar));
    pushStackArg(StackSurfOp, Inst->getOperand(1),
                 getValueSize(T->getContainedType(Index)), RowOff, ColOff,
                 SrcRowOff, SrcColOff);
  } else
    emitVectorCopy(RetVar->getAlias(Inst->getOperand(1), Kernel),
                   Inst->getOperand(1), RowOff, ColOff, SrcRowOff, SrcColOff,
                   getValueSize(Inst->getOperand(1)));
}

void GenXKernelBuilder::buildStackCallLight(CallInst *CI,
                                            const DstOpndDesc &DstDesc) {
  LLVM_DEBUG(dbgs() << "Build stack call " << *CI << "\n");
  Function *Callee = CI->getCalledFunction();

  VISA_PredOpnd *Pred = nullptr;
  auto *MDArg = CI->getMetadata(vc::InstMD::FuncArgSize);
  auto *MDRet = CI->getMetadata(vc::InstMD::FuncRetSize);
  IGC_ASSERT(MDArg && MDRet);
  auto ArgSize =
      cast<ConstantInt>(
          cast<ConstantAsMetadata>(MDArg->getOperand(0).get())->getValue())
          ->getZExtValue();
  auto RetSize =
      cast<ConstantInt>(
          cast<ConstantAsMetadata>(MDRet->getOperand(0).get())->getValue())
          ->getZExtValue();
  if (Callee) {
    CISA_CALL(Kernel->AppendVISACFFunctionCallInst(
        Pred, (NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1), StackCallExecSize,
        Callee->getName().str(), ArgSize, RetSize));
  } else {
    auto *FuncAddr = createSource(IGCLLVM::getCalledValue(CI), DONTCARESIGNED);
    IGC_ASSERT(FuncAddr);
    CISA_CALL(Kernel->AppendVISACFIndirectFuncCallInst(
        Pred, (NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1), StackCallExecSize, FuncAddr,
        ArgSize, RetSize));
  }
}

static void dumpGlobalAnnotations(Module &M) {
  auto *GV = M.getGlobalVariable("llvm.global.annotations");
  if (!GV)
    return;
  auto *Array = dyn_cast<ConstantArray>(GV->getOperand(0));
  for (const auto &Op : Array->operands()) {
    auto *Struct = dyn_cast<ConstantStruct>(Op.get());
    auto *Func = dyn_cast<Function>(Struct->getOperand(0)->getOperand(0));
    if (!Func)
      continue;
    auto FuncName = Func->getName();
    assert(!FuncName.empty() && "Annotated function must have a name");
    auto *GlobalStr =
        dyn_cast<GlobalVariable>(Struct->getOperand(1)->getOperand(0));
    auto *Str = dyn_cast<ConstantDataArray>(GlobalStr->getInitializer());
    auto FuncAnnotation = Str->getAsString();
    errs() << "Warning: Annotation \"" << FuncAnnotation << "\" for function "
           << FuncName << " is ignored\n";
  }
}

namespace {

void initializeGenXFinalizerPass(PassRegistry &);

class GenXFinalizer : public ModulePass {
  LLVMContext *Ctx = nullptr;

public:
  static char ID;
  explicit GenXFinalizer() : ModulePass(ID) {}

  StringRef getPassName() const override { return "GenX Finalizer"; }

  LLVMContext &getContext() {
    IGC_ASSERT(Ctx);
    return *Ctx;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<GenXModule>();
    AU.addRequired<FunctionGroupAnalysis>();
    AU.addRequired<TargetPassConfig>();
    AU.addRequired<GenXBackendConfig>();
    AU.setPreservesAll();
  }

  bool runOnModule(Module &M) override {
    Ctx = &M.getContext();

    auto BC = &getAnalysis<GenXBackendConfig>();
    auto &FGA = getAnalysis<FunctionGroupAnalysis>();
    auto &GM = getAnalysis<GenXModule>();
    std::stringstream ss;
    VISABuilder *CisaBuilder = GM.GetCisaBuilder();
    if (GM.HasInlineAsm() || !BC->getVISALTOStrings().empty())
      CisaBuilder = GM.GetVISAAsmReader();
    CISA_CALL(CisaBuilder->Compile(
        BC->isaDumpsEnabled() && BC->hasShaderDumper()
            ? BC->getShaderDumper().composeDumpPath("final.isa").c_str()
            : "",
        nullptr, BC->emitVisaOnly()));

    if (!BC->isDisableFinalizerMsg())
      dbgs() << CisaBuilder->GetCriticalMsg();

    dumpGlobalAnnotations(M);

    // Collect some useful statistics
    for (auto *FG: FGA) {
      VISAKernel *Kernel = CisaBuilder->GetVISAKernel(FG->getName().str());
      IGC_ASSERT(Kernel);
      vISA::FINALIZER_INFO *jitInfo = nullptr;
      CISA_CALL(Kernel->GetJitInfo(jitInfo));
      IGC_ASSERT(jitInfo);
      NumAsmInsts += jitInfo->stats.numAsmCountUnweighted;
      SpillMemUsed += jitInfo->stats.spillMemUsed;
      NumFlagSpillStore += jitInfo->stats.numFlagSpillStore;
      NumFlagSpillLoad += jitInfo->stats.numFlagSpillLoad;
    }
    return false;
  }
};
} // end anonymous namespace.

char GenXFinalizer::ID = 0;

INITIALIZE_PASS_BEGIN(GenXFinalizer, "GenXFinalizer",
                      "GenXFinalizer", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_DEPENDENCY(FunctionGroupAnalysis)
INITIALIZE_PASS_DEPENDENCY(GenXModule)
INITIALIZE_PASS_END(GenXFinalizer, "GenXFinalizer",
                    "GenXFinalizer", false, false)

ModulePass *llvm::createGenXFinalizerPass() {
  return new GenXFinalizer();
}

static SmallVector<const char *, 8>
collectFinalizerArgs(StringSaver &Saver, const GenXSubtarget &ST,
                     GenXModule::InfoForFinalizer Info,
                     const GenXBackendConfig &BC) {
  SmallVector<const char *, 8> Argv;
  auto addArgument = [&Argv, &Saver](StringRef Arg) {
    // String saver guarantees that string is null-terminated.
    Argv.push_back(Saver.save(Arg).data());
  };

  const WA_TABLE* WATable = BC.getWATable();
  // enable preemption if subtarget supports it
  if (ST.hasPreemption())
    addArgument("-enablePreemption");

  if (ST.hasHalfSIMDLSC())
    addArgument("-enableHalfLSC");

  for (const auto &Fos : FinalizerOpts)
    cl::TokenizeGNUCommandLine(Fos, Saver, Argv);

  if (Info.EmitDebugInformation)
    addArgument("-generateDebugInfo");
  if (Info.EmitCrossThreadOffsetRelocation)
    addArgument("-emitCrossThreadOffR0Reloc");
  if (Info.DisableFinalizerOpts)
    addArgument("-debug");

  bool usingL0DbgApi = ST.getVisaPlatform() >= TARGET_PLATFORM::Xe_DG2;
  if (BC.emitBreakpointAtKernelEntry() && !usingL0DbgApi) {
    addArgument("-addKernelID");
    addArgument("-setstartbp");
  }
  if (BC.asmDumpsEnabled()) {
    addArgument("-dumpcommonisa");
    addArgument("-output");
    addArgument("-binary");
  }
  if (ST.needsWANoMaskFusedEU() && !DisableNoMaskWA)
    addArgument("-noMaskWA");

  unsigned GRFSize = BC.getGRFSize();
  if (GRFSize > 0) {
    addArgument("-TotalGRFNum");
    addArgument(to_string(GRFSize));
  }

  if (ST.hasFusedEU()) {
    addArgument("-fusedCallWA");
    if (WATable && WATable->Wa_14016243945)
      addArgument("2");
    else
      addArgument("1");
  }
  if (BC.getBinaryFormat() == vc::BinaryKind::ZE) {
    addArgument("-abiver");
    addArgument("2");
  }
  if (WATable && WATable->Wa_14012437816)
    addArgument("-LSCFenceWA");

  if (BC.isHashMovsEnabled()) {
    uint64_t Hash = BC.getAsmHash();
    uint32_t HashLo = Hash;
    uint32_t HashHi = Hash >> 32;

    addArgument("-hashmovs");
    addArgument(to_string(HashHi));
    addArgument(to_string(HashLo));

    if (BC.isHashMovsAtPrologueEnabled())
      addArgument("-hashatprologue");
  }

  return Argv;
}

static void dumpFinalizerArgs(const SmallVectorImpl<const char *> &Argv,
                              StringRef CPU) {
  // NOTE: CPU is not the Platform used by finalizer
  // The mapping is described by getVisaPlatform from GenXSubtarget.h
  outs() << "GenXCpu: " << CPU << "\n";
  outs() << "Finalizer Parameters:\n\t";
  std::for_each(Argv.begin(), Argv.end(),
                [](const char *Arg) { outs() << " " << Arg; });
  outs() << "\n";
}

LLVMContext &GenXModule::getContext() {
  IGC_ASSERT(Ctx);
  return *Ctx;
}

static VISABuilder *createVISABuilder(const GenXSubtarget &ST,
                                      const GenXBackendConfig &BC,
                                      GenXModule::InfoForFinalizer Info,
                                      vISABuilderMode Mode, LLVMContext &Ctx,
                                      BumpPtrAllocator &Alloc) {
  auto Platform = ST.getVisaPlatform();
  // Fail for unknown platforms
  if (Platform == TARGET_PLATFORM::GENX_NONE)
    report_fatal_error("Platform unknown");

  // Prepare array of arguments for Builder API.
  StringSaver Saver{Alloc};
  SmallVector<const char *, 8> Argv = collectFinalizerArgs(Saver, ST, Info, BC);

  if (PrintFinalizerOptions)
    dumpFinalizerArgs(Argv, ST.getCPU());

  // Special error processing here related to strange case where on Windows
  // machines only we had failures, reproducible only when shader dumps are
  // off. This code is to diagnose such cases simpler.
  VISABuilder *VB = nullptr;
  int Result = CreateVISABuilder(
      VB, Mode, VISA_BUILDER_BOTH, Platform,
      Argv.size(), Argv.data(), BC.getWATable());
  if (Result != 0 || VB == nullptr) {
    std::string Str;
    llvm::raw_string_ostream Os(Str);
    Os << "VISA builder creation failed\n";
    Os << "Mode: " << Mode << "\n";
    Os << "Args:\n";
    for (const char *Arg : Argv)
      Os << Arg << " ";
    Os << "Visa only: " << (BC.emitVisaOnly() ? "yes" : "no") << "\n";
    Os << "Platform: " << ST.getVisaPlatform() << "\n";
    vc::diagnose(Ctx, "GenXCisaBuilder", Os.str().c_str());
  }

  std::unordered_set<std::string> DirectCallFunctions;
  for (auto& FuncName : BC.getDirectCallFunctionsSet())
  {
    DirectCallFunctions.insert(FuncName.getKey().str());
  }
  VB->SetDirectCallFunctionSet(DirectCallFunctions);

  return VB;
}

void GenXModule::InitCISABuilder() {
  IGC_ASSERT(ST);
  const vISABuilderMode Mode =
    (HasInlineAsm() || !BC->getVISALTOStrings().empty())
      ? vISA_ASM_WRITER
      : vISA_DEFAULT;
  CisaBuilder = createVISABuilder(*ST, *BC, getInfoForFinalizer(), Mode,
                                  getContext(), ArgStorage);
}

VISABuilder *GenXModule::GetCisaBuilder() {
  if (!CisaBuilder)
    InitCISABuilder();
  return CisaBuilder;
}

void GenXModule::DestroyCISABuilder() {
  if (CisaBuilder) {
    CISA_CALL(DestroyVISABuilder(CisaBuilder));
    CisaBuilder = nullptr;
  }
}

void GenXModule::InitVISAAsmReader() {
  IGC_ASSERT(ST);
  VISAAsmTextReader =
      createVISABuilder(*ST, *BC, getInfoForFinalizer(), vISA_ASM_READER,
                        getContext(), ArgStorage);
}

VISABuilder *GenXModule::GetVISAAsmReader() {
  if (!VISAAsmTextReader)
    InitVISAAsmReader();
  return VISAAsmTextReader;
}

void GenXModule::DestroyVISAAsmReader() {
  if (VISAAsmTextReader) {
    CISA_CALL(DestroyVISABuilder(VISAAsmTextReader));
    VISAAsmTextReader = nullptr;
  }
}
