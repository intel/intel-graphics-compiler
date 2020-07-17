/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
//
/// GenXCisaBuilder
/// ---------------
///
/// This file contains to passes: GenXCisaBuilder and GenXFinalizer.
///
/// 1. GenXCisaBuilder transforms LLVM IR to CISA IR via Finalizer' public API.
///    It is a FunctionGroupPass, thus it runs once for each kernel and creates
///    CISA IR for it and all its subroutines.
///    Real building of kernels is performed by the GenXKernelBuilder class.
///    This splitting is necessary because GenXCisaBuilder object lives
///    through all Function Groups, but we don't need to keep all Kernel
///    building specific data in such lifetime.
///
/// 2. GenXFinalizer is a module pass, thus it runs once and all that it does
///    is a running of Finalizer for kernels created in GenXCisaBuilder pass.
///
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXGotoJoin.h"
#include "GenXIntrinsics.h"
#include "GenXOCLRuntimeInfo.h"
#include "GenXPressureTracker.h"
#include "GenXRegion.h"
#include "GenXSubtarget.h"
#include "GenXUtil.h"
#include "GenXVisaRegAlloc.h"
#include "common.h"
#include "vc/GenXOpts/Utils/KernelInfo.h"
#include "visaBuilder_interface.h"
#include "llvm/ADT/IndexedMap.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/GenXIntrinsics/GenXIntrinsicInst.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/ScopedPrinter.h"

#include "llvmWrapper/IR/InstrTypes.h"

#include <map>
#include <string>
#include <vector>

// If 1, print VISA instructions after corresponding LLVM instruction.
// Only for debug purposes, uses Finalizer internal API.
#define DUMP_VISA_INTSTRUCTIONS 0

#if DUMP_VISA_INTSTRUCTIONS
#include "Common_ISA_framework.h"
#include "IsaDisassembly.h"
#include "Mem_Manager.h"
#include "VISAKernel.h"
#endif

#ifndef COMMON_ISA_MAX_FILENAME_LENGTH
#define COMMON_ISA_MAX_FILENAME_LENGTH 1023
#endif

using namespace llvm;
using namespace genx;

#define DEBUG_TYPE "GENX_CISA_BUILDER"

static cl::opt<bool> EmitVisa("emit-visa", cl::init(false), cl::Hidden,
                              cl::desc("Generate Visa instead of fat binary."));
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

enum {
  BYTES_PER_OWORD = 16,
  BYTES_PER_FADDR = 8,
  // stackcall ABI related constants
  ARG_SIZE_IN_GRFS = 32,
  RET_SIZE_IN_GRFS = 12,
  STACK_PER_THREAD = 256
};

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
  genx::BaleInfo WrRegionBI;
};

namespace {

// Diagnostic information for errors/warnings in the GEN IR building passes.
class DiagnosticInfoCisaBuild : public DiagnosticInfo {
private:
  std::string Description;
  static int KindID;

  static int getKindID() {
    if (KindID == 0)
      KindID = llvm::getNextAvailablePluginDiagnosticKind();
    return KindID;
  }

public:
  DiagnosticInfoCisaBuild(const Twine &Desc, DiagnosticSeverity Severity)
      : DiagnosticInfo(getKindID(), Severity) {
    Description = (Twine("GENX IR generation error: ") + Desc).str();
  }

  void print(DiagnosticPrinter &DP) const override { DP << Description; }

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == getKindID();
  }
};
int DiagnosticInfoCisaBuild::KindID = 0;


static VISA_Exec_Size getExecSizeFromValue(unsigned int Size) {
  int Res = genx::log2(Size);
  assert(std::bitset<sizeof(unsigned int) * 8>(Size).count() <= 1);
  assert(Res <= 5 &&
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
    MUST_BE_TRUE(false, "illegal Oword number.");
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
    llvm_unreachable("Wrong mask");
  }
}

CHANNEL_OUTPUT_FORMAT getChannelOutputFormat(uint8_t ChannelOutput) {
  return (CHANNEL_OUTPUT_FORMAT)((ChannelOutput >> 4) & 0x3);
}

std::string cutString(std::string Str) {
  // vISA is limited to 64 byte strings. But old fe-compiler seems to ignore
  // that for source filenames.
  if (Str.size() > 64)
    Str.erase(64);
  return Str;
}

void handleCisaCallError(int CallResult, const Twine &Call, LLVMContext &Ctx) {
  StringRef ErrorType;
  switch (CallResult) {
  case VISA_SPILL:
    ErrorType = "register allocation for a kernel failed, even with spill code";
    break;
  case VISA_FAILURE:
    ErrorType = "general failure";
    break;
  default:
    ErrorType = "unknown error";
    break;
  }
#ifndef NDEBUG
  DiagnosticInfoCisaBuild Err(
      "VISA builder API call failed (" + Call + "): " + ErrorType, DS_Error);
#else
  DiagnosticInfoCisaBuild Err("VISA builder API call failed: " + ErrorType,
                              DS_Error);
#endif
  Ctx.diagnose(Err);
}

} // namespace

#define CISA_CALL(c)                                                           \
  do {                                                                         \
    auto result = c;                                                           \
    if (result != VISA_SUCCESS) {                                              \
      handleCisaCallError(result, #c, getContext());                           \
    }                                                                          \
  } while (0);

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
  assert(!T->isAggregateType());
  VISA_Type Result = ISA_TYPE_NUM;
  if (T->isVectorTy() && T->getVectorElementType()->isIntegerTy(1)) {
    switch (Type->getVectorNumElements()) {
    case 8:
      Result = (Sign == SIGNED) ? ISA_TYPE_B : ISA_TYPE_UB;
      break;
    case 16:
      Result = (Sign == SIGNED) ? ISA_TYPE_W : ISA_TYPE_UW;
      break;
    case 32:
      Result = (Sign == SIGNED) ? ISA_TYPE_D : ISA_TYPE_UD;
      break;
    default:
      report_fatal_error("only 8xi1 and 32xi1 are currently supported");
      break;
    }
  } else {
    if (T->isVectorTy())
      T = T->getVectorElementType();
    if (T->isPointerTy() && T->getPointerElementType()->isFunctionTy()) {
      // we might have used DL to get the type size but that'd
      // overcomplicate this function's type unnecessarily
      Result = getVisaTypeFromBytesNumber(BYTES_PER_FADDR, false, DONTCARESIGNED);
    } else {
      assert(T->isFloatingPointTy() || T->isIntegerTy());
      Result = getVisaTypeFromBytesNumber(T->getScalarSizeInBits() / CHAR_BIT,
                                          T->isFloatingPointTy(), Sign);
    }
  }
  assert(Result != ISA_TYPE_NUM);
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
    assert(!(ByteSize % CISATypeTable[Type].typeSize));
    return ByteSize / CISATypeTable[Type].typeSize;
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
    assert(!(ByteSize % CISATypeTable[T].typeSize));
    return ByteSize / CISATypeTable[T].typeSize;
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
      K->CreateVISAGenVar(VV, Name, getNumElements(T), T,
          ALIGN_HWORD,
          VisaVar);
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
/// It is a FunctionGroupPass, thus it runs once for each kernel and
/// builds vISA kernel via class GenXKernelBuilder.
/// All created kernels are stored in CISA Builder object which is provided
/// by finalizer.
///
//===----------------------------------------------------------------------===//
class GenXCisaBuilder : public FunctionGroupPass {
  LLVMContext *Ctx = nullptr;

public:
  static char ID;
  explicit GenXCisaBuilder() : FunctionGroupPass(ID) {}

  virtual StringRef getPassName() const {
    return "GenX CISA construction pass";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const;
  bool runOnFunctionGroup(FunctionGroup &FG);

  LLVMContext &getContext() {
    assert(Ctx);
    return *Ctx;
  }
};

void initializeGenXCisaBuilderPass(PassRegistry &);

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
  genx::KernelMetadata TheKernelMetadata;
  LLVMContext &Ctx;
  const DataLayout &DL;

  std::map<Function *, VISAFunction *> Func2Kern;

  std::map<std::string, unsigned> StringPool;
  std::vector<VISA_LabelOpnd *> Labels;
  std::map<Value *, unsigned> LabelMap;

  // loop info for each function
  std::map<Function *, LoopInfoBase<BasicBlock, Loop> *> Loops;
  ValueMap<Function *, bool> IsInLoopCache;

  bool HasBarrier = false;
  bool HasCallable = false;
  bool HasStackcalls = false;
  bool HasAlloca = false;
  bool UseGlobalMem = false;
  // GRF width in unit of byte
  unsigned GrfByteSize = 32;

  int LastLabel = 0;
  unsigned LastLine = 0;
  unsigned PendingLine = 0;
  StringRef LastFilename;
  StringRef PendingFilename;
  StringRef LastDirectory;
  StringRef PendingDirectory;

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

  static const uint32_t CR_Mask = 0x1 << 10 | 0x3 << 6 | 0x3 << 4 | 0x1;

  // normally false, set to true if there is any SIMD CF in the func or this is
  // (indirectly) called inside any SIMD CF.
  bool NoMask = false;

  genx::AlignmentInfo AI;

public:
  FunctionGroup *FG = nullptr;
  GenXLiveness *Liveness = nullptr;
  GenXNumbering *Numbering = nullptr;
  GenXVisaRegAlloc *RegAlloc = nullptr;
  FunctionGroupAnalysis *FGA = nullptr;
  GenXModule *GM = nullptr;
  DominatorTreeGroupWrapperPass *DTs = nullptr;
  const GenXSubtarget *Subtarget = nullptr;
  GenXBaling *Baling = nullptr;
  VISABuilder *CisaBuilder = nullptr;

private:
  void collectKernelInfo();
  void buildVariables();
  void buildInstructions();

  bool buildInstruction(Instruction *Inst);
  bool buildMainInst(Instruction *Inst, genx::BaleInfo BI, unsigned Mod,
                     const DstOpndDesc &DstDesc);
  void buildControlRegUpdate(unsigned Mask, bool Clear);
  void buildJoin(CallInst *Join, BranchInst *Branch);
  bool buildBranch(BranchInst *Branch);
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
  VISA_EMask_Ctrl getExecMaskFromWrPredRegion(Instruction *WrPredRegion,
                                                     bool IsNoMask);
  VISA_EMask_Ctrl getExecMaskFromWrRegion(const DstOpndDesc &DstDesc,
                                                 bool IsNoMask = false);
  unsigned getOrCreateLabel(Value *V, int Kind);
  int getLabel(Value *V);
  void setLabel(Value *V, unsigned Num);

  void emitOptimizationHints();

  LoopInfoBase<BasicBlock, Loop> *getLoops(Function *F);
  Value *getPredicateOperand(Instruction *Inst, unsigned OperandNum,
                             genx::BaleInfo BI, VISA_PREDICATE_CONTROL &Control,
                             VISA_PREDICATE_STATE &PredField,
                             VISA_EMask_Ctrl *MaskCtrl);
  bool isInLoop(BasicBlock *BB);

  void addLabelInst(Value *BB);
  void buildPhiNode(PHINode *Phi);
  void buildGoto(CallInst *Goto, BranchInst *Branch);
  void buildCall(IGCLLVM::CallInst *CI, const DstOpndDesc &DstDesc);
  void buildStackCall(IGCLLVM::CallInst *CI, const DstOpndDesc &DstDesc);
  void buildInlineAsm(CallInst *CI);
  void buildPrintIndex(CallInst *CI, unsigned IntrinID, unsigned Mod,
                       const DstOpndDesc &DstDesc);
  void buildSelectInst(SelectInst *SI, genx::BaleInfo BI, unsigned Mod,
                       const DstOpndDesc &DstDesc);
  void buildBinaryOperator(BinaryOperator *BO, genx::BaleInfo BI, unsigned Mod,
                           const DstOpndDesc &DstDesc);
#if (LLVM_VERSION_MAJOR > 8)
  void buildUnaryOperator(UnaryOperator *UO, genx::BaleInfo BI, unsigned Mod,
                          const DstOpndDesc &DstDesc);
#endif
  void buildBoolBinaryOperator(BinaryOperator *BO);
  void buildSymbolInst(PtrToIntInst *ptr2Int, unsigned Mod,
                       const DstOpndDesc &DstDesc);
  void buildCastInst(CastInst *CI, genx::BaleInfo BI, unsigned Mod,
                     const DstOpndDesc &DstDesc);
  void buildConvertAddr(CallInst *CI, genx::BaleInfo BI, unsigned Mod,
                        const DstOpndDesc &DstDesc);
  void buildAlloca(CallInst *CI, unsigned IntrinID, unsigned Mod,
                   const DstOpndDesc &DstDesc);
  void addWriteRegionLifetimeStartInst(Instruction *WrRegion);
  void addLifetimeStartInst(Instruction *Inst);
  void AddGenVar(Register &Reg);
  void buildRet(ReturnInst *RI);
  void buildBitCast(CastInst *CI, genx::BaleInfo BI, unsigned Mod,
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
                                unsigned *Offset = nullptr);
  VISA_VectorOpnd *createSource(Value *V, genx::Signedness Signed,
                                unsigned MaxWidth = 16,
                                unsigned *Offset = nullptr);

  std::string createInlineAsmOperand(Register *Reg, genx::Region *R, bool IsDst,
                                     genx::Signedness Signed,
                                     genx::ConstraintType Ty, unsigned Mod);

  std::string createInlineAsmSourceOperand(Value *V, genx::Signedness Signed,
                                           bool Baled, genx::ConstraintType Ty,
                                           unsigned Mod = 0,
                                           unsigned MaxWidth = 16);

  std::string createInlineAsmDestinationOperand(Value *Dest,
                                                genx::Signedness Signed,
                                                genx::ConstraintType Ty,
                                                unsigned Mod,
                                                const DstOpndDesc &DstDesc);

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

  void addDebugInfo();

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
  void beginFunction(Function *Func);
  void endFunction(Function *Func, ReturnInst *RI);

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

  bool run(std::string &KernelNameBuf);

  LLVMContext &getContext() { return Ctx; }

  unsigned addStringToPool(StringRef Str);
  StringRef getStringByIndex(unsigned Val);
};

} // end namespace llvm

char GenXCisaBuilder::ID = 0;
INITIALIZE_PASS_BEGIN(GenXCisaBuilder, "GenXCisaBuilderPass",
                      "GenXCisaBuilderPass", false, false)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeGroupWrapperPass)
INITIALIZE_PASS_DEPENDENCY(GenXGroupBaling)
INITIALIZE_PASS_DEPENDENCY(GenXLiveness)
INITIALIZE_PASS_DEPENDENCY(GenXVisaRegAlloc)
INITIALIZE_PASS_DEPENDENCY(GenXModule)
INITIALIZE_PASS_END(GenXCisaBuilder, "GenXCisaBuilderPass",
                    "GenXCisaBuilderPass", false, false)

FunctionGroupPass *llvm::createGenXCisaBuilderPass() {
  initializeGenXCisaBuilderPass(*PassRegistry::getPassRegistry());
  return new GenXCisaBuilder();
}

void GenXCisaBuilder::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<DominatorTreeGroupWrapperPass>();
  AU.addRequired<GenXGroupBaling>();
  AU.addRequired<GenXLiveness>();
  AU.addRequired<GenXVisaRegAlloc>();
  AU.addRequired<GenXModule>();
  AU.addRequired<FunctionGroupAnalysis>();
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
  KernelBuilder->DTs = &getAnalysis<DominatorTreeGroupWrapperPass>();
  KernelBuilder->Liveness = &getAnalysis<GenXLiveness>();
  auto P = getAnalysisIfAvailable<GenXSubtargetPass>();
  KernelBuilder->Subtarget = P ? P->getSubtarget() : nullptr;

  std::string KernelName;
  KernelBuilder->run(KernelName);

  GenXModule *GM = KernelBuilder->GM;
  VISABuilder *VisaBuilder = GM->GetCisaBuilder();
  if (GM->HasInlineAsm()) {
    CISA_CALL(KernelBuilder->CisaBuilder->WriteVISAHeader());
    auto VISAAsmTextReader = GM->GetVISAAsmReader();
    auto VISATextHeader =
        KernelBuilder->CisaBuilder->GetAsmTextHeaderStream().str();
    auto VISAText = KernelBuilder->CisaBuilder->GetAsmTextStream().str();
    CISA_CALL(VISAAsmTextReader->ParseVISAText(VISATextHeader, VISAText, ""));
    VisaBuilder = VISAAsmTextReader;
  }
  for (auto &F : FG) {
    if (genx::isKernel(F)) {
      VISAKernel *BuiltKernel = VisaBuilder->GetVISAKernel(KernelName.c_str());
      GM->saveVisaKernel(F, BuiltKernel);
    } else if (F->hasFnAttribute(genx::FunctionMD::CMStackCall)) {
      VISAKernel *BuiltKernel = VisaBuilder->GetVISAKernel(F->getName());
      GM->saveVisaKernel(F, BuiltKernel);
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

  assert(Exp >= 124 && Exp <= 131);
  Exp -= 124;
  assert((Frac & 0x780000) == Frac);
  Frac >>= 19;
  assert(!(Exp == 124 && Frac == 0));

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

void addKernelAttrsFromMetadata(VISAKernel &Kernel, const KernelMetadata &KM,
                                const GenXSubtarget* Subtarget) {
  unsigned Val = KM.getSLMSize();
  if (Val) {
    // Compute the slm size in KB and roundup to power of 2.
    Val = alignTo(Val, 1024) / 1024;
    if (!isPowerOf2_64(Val))
      Val = NextPowerOf2(Val);
    unsigned MaxSLMSize = 64;
    if (Val > MaxSLMSize)
      report_fatal_error("slm size must not exceed 64KB");
    else {
      // For pre-SKL, valid values are {0, 4, 8, 16, 32, 64}.
      // For SKL+, valid values are {0, 1, 2, 4, 8, 16, 32, 64}.
      // FIXME: remove the following line for SKL+.
      Val = (Val < 4) ? 4 : Val;
      uint8_t SLMSize = static_cast<uint8_t>(Val);
      Kernel.AddKernelAttribute("SLMSize", 1, &SLMSize);
    }
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
  assert(TheKernelMetadata.isKernel());
  std::string AsmName;
  auto UserAsmName = AsmNameOpt.getValue();
  if (UserAsmName.empty()) {
    AsmName = legalizeName(TheKernelMetadata.getName());
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
    assert(idx >= 0);
    // Reverse kernel ASM names during codegen.
    // This provides an option to match the old compiler's output.
    if (ReverseKernels.getValue())
      idx = E - idx - 1;
    AsmName = (UserAsmName + llvm::Twine('_') + llvm::Twine(idx)).str();
  }
  return AsmName;
}


bool GenXKernelBuilder::run(std::string &KernelNameBuf) {
  GrfByteSize = Subtarget ? Subtarget->getGRFWidth() : 32;
  StackSurf = Subtarget ? Subtarget->stackSurface() : PREDEFINED_SURFACE_STACK;
  StringRef Name = TheKernelMetadata.getName();
  if (!Name.size()) {
    // If it is not a kernel, or no metadata was found, then set the
    // name to the IR name.
    Name = FG->getHead()->getName();
  }

  // Cut kernel name to fit vISA name size
  auto Size = (Name.size() > COMMON_ISA_MAX_FILENAME_LENGTH)
                  ? (COMMON_ISA_MAX_FILENAME_LENGTH)
                  : Name.size();
  KernelNameBuf.insert(0, Name.begin(), Size);
  KernelNameBuf[Size] = 0;
  if (TheKernelMetadata.isKernel()) {
    CisaBuilder->AddKernel(MainKernel, KernelNameBuf.c_str());
    Kernel = static_cast<VISAFunction *>(MainKernel);
    Func2Kern[FG->getHead()] = Kernel;
  } else {
    CisaBuilder->AddFunction(Kernel, KernelNameBuf.c_str());
  }

  assert(Kernel && "Kernel initialization failed!");
  LLVM_DEBUG(dbgs() << "=== PROCESS KERNEL(" << TheKernelMetadata.getName()
                    << ") ===\n");

  assert(Subtarget);
  addKernelAttrsFromMetadata(*Kernel, TheKernelMetadata, Subtarget);

  bool NeedRetIP = false; // Need special return IP variable for FC.
  if (TheKernelMetadata.isKernel()) {
    // For a kernel, add an attribute for asm filename for the jitter.
    std::string AsmName = buildAsmName();
    StringRef AsmNameRef = AsmName;
    CISA_CALL(Kernel->AddKernelAttribute("OutputAsmPath", AsmNameRef.size(),
                                         AsmNameRef.begin()));

    // Populate variable attributes if any.
    unsigned Idx = 0;
    bool IsComposable = false;
    for (auto &Arg : FG->getHead()->args()) {
      const char *Kind = nullptr;
      switch (TheKernelMetadata.getArgInputOutputKind(Idx++)) {
      default:
        break;
      case KernelMetadata::IO_INPUT:
        Kind = "Input";
        break;
      case KernelMetadata::IO_OUTPUT:
        Kind = "Output";
        break;
      case KernelMetadata::IO_INPUT_OUTPUT:
        Kind = "Input_Output";
        break;
      }
      if (Kind != nullptr) {
        auto R = RegAlloc->getRegForValueUntyped(FG->getHead(), &Arg);
        assert(R && R->Category == RegCategory::GENERAL);
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
    if (FG->getHead()->hasFnAttribute("CMCallable")) {
      CISA_CALL(Kernel->AddKernelAttribute("Callable", 0, ""));
      NeedRetIP = true;
    }
    if (FG->getHead()->hasFnAttribute("CMEntry")) {
      CISA_CALL(Kernel->AddKernelAttribute("Entry", 0, ""));
    }
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

  Func = FG->getHead();
  // Build variables
  buildVariables();

  // Build input variables
  buildInputs(FG->getHead(), NeedRetIP);

  for (auto &F : *FG) {
    Func = F;
    if (F->hasFnAttribute(genx::FunctionMD::CMStackCall) ||
        F->hasFnAttribute(genx::FunctionMD::ReferencedIndirectly)) {
      VISAFunction *stackFunc = nullptr;
      CisaBuilder->AddFunction((VISAFunction *&)stackFunc, F->getName().data());
      assert(stackFunc);
      Func2Kern[F] = stackFunc;
      Kernel = stackFunc;
      buildVariables();
      Kernel = static_cast<VISAFunction *>(MainKernel);
    }
  }

  // Build instructions
  buildInstructions();

  // Reset Regalloc hook
  RegAlloc->SetRegPushHook(nullptr, nullptr);

  if (TheKernelMetadata.isKernel()) {
    // For a kernel with no barrier instruction, add a NoBarrier attribute.
    if (!HasBarrier)
      CISA_CALL(Kernel->AddKernelAttribute("NoBarrier", 0, nullptr));
  }

  return false;
}

static bool PatchImpArgOffset(Function *F, const GenXSubtarget *ST,
                              const KernelMetadata &KM) {
  return false;
}

void GenXKernelBuilder::buildInputs(Function *F, bool NeedRetIP) {

  assert(F->arg_size() == TheKernelMetadata.getNumArgs() &&
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
    Register *Reg = RegAlloc->getRegForValueUntyped(F, Arg);
    assert(Reg);
    uint8_t Kind = TheKernelMetadata.getArgKind(Idx);
    uint16_t Offset;
    if (!PatchImpArgOff) {
      Offset = TheKernelMetadata.getArgOffset(Idx);
    }
    // Argument size in bytes.
    auto &DL = F->getParent()->getDataLayout();
    Type *Ty = Arg->getType();
    uint16_t NumBytes = Ty->isPointerTy() ? DL.getPointerTypeSize(Ty)
                                          : (Ty->getPrimitiveSizeInBits() / 8U);

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
  // Add the special RetIP argument.
  if (NeedRetIP) {
    Register *Reg = RegAlloc->getRetIPArgument();
    uint16_t Offset = (127 * GrfByteSize + 6 * 4); // r127.6
    uint16_t NumBytes = (64 / 8);
    CISA_CALL(Kernel->CreateVISAImplicitInputVar(Reg->GetVar<VISA_GenVar>(Kernel),
                                                 Offset, NumBytes, 0));
  }
  // Add pseudo-input for global variables with offset attribute.
  for (auto &Item : Bindings) {
    // TODO: sanity check. No overlap with other inputs.
    GlobalVariable *GV = Item.first;
    uint16_t Offset = Item.second;
    assert(Offset > 0);
    uint16_t NumBytes = (GV->getValueType()->getPrimitiveSizeInBits() / 8U);
    uint8_t Kind = KernelMetadata::IMP_PSEUDO_INPUT;
    Register *Reg = RegAlloc->getRegForValueUntyped(F, GV);
    CISA_CALL(Kernel->CreateVISAImplicitInputVar(Reg->GetVar<VISA_GenVar>(Kernel),
                                                 Offset, NumBytes, Kind >> 3));
  }
}

// FIXME: We should use NM by default once code quality issues are addressed
// in vISA compiler.
static bool setNoMaskByDefault(Function *F) {
  for (auto &BB : F->getBasicBlockList())
    if (GotoJoin::isGotoBlock(&BB))
      return true;

  // Check if this is subroutine call.
  for (auto U : F->users()) {
    if (auto CI = dyn_cast<CallInst>(U)) {
      Function *G = CI->getParent()->getParent();
      if (G == F)
        return false;
      if (setNoMaskByDefault(G))
        return true;
    }
  }

  return false;
}

void GenXKernelBuilder::buildInstructions() {
  for (auto It = FG->begin(), E = FG->end(); It != E; ++It) {
    Func = *It;
    LLVM_DEBUG(dbgs() << "Building IR for func " << Func->getName().data()
                      << "\n");
    NoMask = setNoMaskByDefault(Func);

    if (Func->hasFnAttribute(genx::FunctionMD::CMGenXMain) ||
        Func->hasFnAttribute(genx::FunctionMD::CMStackCall) ||
        Func->hasFnAttribute(genx::FunctionMD::ReferencedIndirectly)) {
      KernFunc = Func;
    } else {
      KernFunc = FGA->getSubGroup(Func) ? FGA->getSubGroup(Func)->getHead()
                                        : FGA->getGroup(Func)->getHead();
    }
    assert(KernFunc);
    Kernel = Func2Kern.at(KernFunc);

    unsigned LabelID = getOrCreateLabel(Func, LABEL_SUBROUTINE);
    CISA_CALL(Kernel->AppendVISACFLabelInst(Labels[LabelID]));

    beginFunction(Func);

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
      buildControlRegUpdate(CR_Mask, true);

      // Set rounding mode to required state if that isn't zero
      FloatControl &= CR_Mask;
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
      }
      NeedsLabel = true;
      for (BasicBlock::iterator bi = BB->begin(), be = BB->end(); bi != be;
           ++bi) {
        Instruction *Inst = &*bi;
        if (Inst->isTerminator()) {
          // Before the terminator inst of a basic block, if there is a single
          // successor and it is the header of a loop, for any vector of at
          // least four GRFs with a phi node where our incoming value is
          // undef, insert a lifetime.start here.
          auto TI = cast<IGCLLVM::TerminatorInst>(Inst);
          if (TI->getNumSuccessors() == 1) {
            auto Succ = TI->getSuccessor(0);
            if (getLoops(Succ->getParent())->isLoopHeader(Succ)) {
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
#if DUMP_VISA_INTSTRUCTIONS
          errs() << *Inst << '\n';
          auto CisaInstCount = Kernel->getvIsaInstCount();
#endif
          if (ReturnInst *RI = dyn_cast<ReturnInst>(Inst))
            endFunction(Func, RI);
          if (buildInstruction(Inst))
            NeedsLabel = false;
#if DUMP_VISA_INTSTRUCTIONS
          VISAKernelImpl *KernelImpl = (VISAKernelImpl *)Kernel;
          if (CisaInstCount != Kernel->getvIsaInstCount()) {
            VISAKernel_format_provider fmt(KernelImpl);
            auto It = KernelImpl->getInstructionListBegin(),
                 ItEnd = KernelImpl->getInstructionListEnd();
            for (int Idx = 0; It != ItEnd; ++It, ++Idx) {
              if (Idx >= CisaInstCount + 1) {
                errs() << printInstruction(&fmt, (*It)->getCISAInst(),
                                           KernelImpl->getOptions())
                       << "\n\n";
              }
            }
          }
#endif
        }
      }
    }
  }
}

bool GenXKernelBuilder::buildInstruction(Instruction *Inst) {
  // Make the source location pending, so it is output as vISA FILE and LOC
  // instructions next time an opcode is written.
  const DebugLoc &DL = Inst->getDebugLoc();
  if (DL) {
    StringRef Filename = DL->getFilename();
    if (Filename != "") {
      PendingFilename = Filename;
      PendingDirectory = DL->getDirectory();
    }
    PendingLine = DL.getLine();
  }
  // Process the bale that this is the head instruction of.
  BaleInfo BI = Baling->getBaleInfo(Inst);

  DstOpndDesc DstDesc;
  if (BI.Type == BaleInfo::GSTORE) {
    // Inst is a global variable store. It should be baled into a wrr
    // instruction.
    Bale B;
    Baling->buildBale(Inst, &B);
    // This is an identity bale; no code will be emitted.
    if (isIdentityBale(B))
      return false;

    assert(BI.isOperandBaled(0));
    DstDesc.GStore = Inst;
    Inst = cast<Instruction>(Inst->getOperand(0));
    BI = Baling->getBaleInfo(Inst);
  }

  if (BI.Type == BaleInfo::WRREGION || BI.Type == BaleInfo::WRPREDREGION ||
      BI.Type == BaleInfo::WRPREDPREDREGION) {
    // Inst is a wrregion or wrpredregion or wrpredpredregion.
    DstDesc.WrRegion = Inst;
    DstDesc.WrRegionBI = BI;
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
    assert(isa<CmpInst>(Inst) && "Only bale sel into a cmp instruction");
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
  assert(BI.Type == BaleInfo::MAININST || BI.Type == BaleInfo::NOTP ||
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
  assert(!isa<Constant>(Mask));
  // Variable predicate. Derive the predication field from any baled in
  // all/any/not and the predicate register number.
  Register *Reg = RegAlloc->getRegForValue(KernFunc, Mask);
  assert(Reg && Reg->Category == RegCategory::PREDICATE);
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
      assert(C->isAllOnesValue() && "wrregion mask or predication operand must "
                                    "be constant 1 or not constant");
    } else {
      // Variable predicate. Derive the predication field from any baled in
      // all/any/not and the predicate register number. If the predicate has
      // not has a register allocated, it must be EM.
      Register *Reg = RegAlloc->getRegForValueOrNull(KernFunc, Mask);
      if (Reg) {
        assert(Reg->Category == RegCategory::PREDICATE);
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
    assert(C->isAllOnesValue() && "wrregion mask or predication operand must "
                                  "be constant 1 or not constant");
  } else {
    // Variable predicate. Derive the predication field from any baled in
    // all/any/not and the predicate register number. If the predicate has not
    // has a register allocated, it must be EM.
    Register *Reg = RegAlloc->getRegForValueOrNull(KernFunc, Mask);
    VISA_PredVar *PredVar = nullptr;
    if (Reg) {
      assert(Reg->Category == RegCategory::PREDICATE);
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
  case RegCategory::SURFACE:
    CISA_CALL(Kernel->CreateVISAStateOperand(Op, Reg->GetVar<VISA_SurfaceVar>(Kernel),
                                             Size, Offset, IsDst));
    break;
  case RegCategory::SAMPLER:
    CISA_CALL(Kernel->CreateVISAStateOperand(Op, Reg->GetVar<VISA_SamplerVar>(Kernel),
                                             Size, Offset, IsDst));
    break;
  default:
    llvm_unreachable("unknown state operand");
  }

  return Op;
}

VISA_VectorOpnd *GenXKernelBuilder::createDestination(CisaVariable *Dest,
                                                      genx::Signedness Signed,
                                                      unsigned *Offset) {
  Region R(VectorType::get(
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
  assert(!Dest->getType()->isAggregateType() &&
         "cannot create destination register of an aggregate type");
  if (SignedRes)
    *SignedRes = Signed;

  Type *OverrideType = nullptr;
  if (BitCastInst *BCI = dyn_cast<BitCastInst>(Dest)) {
    if (!(isa<Constant>(BCI->getOperand(0))) &&
        !(BCI->getType()->getScalarType()->isIntegerTy(1)) &&
        (BCI->getOperand(0)->getType()->getScalarType()->isIntegerTy(1))) {
      if (VectorType *VT = dyn_cast<VectorType>(Dest->getType())) {
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
    Register *Reg = RegAlloc->getRegForValue(KernFunc, Dest, Signed, OverrideType);
    if (SignedRes)
      *SignedRes = RegAlloc->getSigned(Reg);
    // Write the vISA general operand:
    if (Reg->Category == RegCategory::GENERAL) {
      Region DestR(Dest);
      if (Offset)
        DestR.Offset = *Offset;
      return createRegionOperand(&DestR, Reg->GetVar<VISA_GenVar>(Kernel),
                                 DONTCARESIGNED, Mod, true /*isDest*/);
    } else {
      assert(Reg->Category == RegCategory::SURFACE ||
             Reg->Category == RegCategory::VME ||
             Reg->Category == RegCategory::SAMPLER);

      return createState(Reg, 0 /*Offset*/, true /*IsDst*/);
    }
  }
  // We need to allow for the case that there is no register allocated if it
  // is an indirected arg, and that is OK because the region is indirect so
  // the vISA does not contain the base register.
  Register *Reg;

  Value *V = nullptr;
  if (DstDesc.GStore) {
    auto GV = getUnderlyingGlobalVariable(DstDesc.GStore->getOperand(1));
    assert(GV && "out of sync");
    if (OverrideType == nullptr)
      OverrideType = DstDesc.GStore->getOperand(0)->getType();
    Reg = RegAlloc->getRegForValue(KernFunc, GV, Signed, OverrideType);
    V = GV;
  } else {
    V = DstDesc.WrRegion;
    Reg = RegAlloc->getRegForValueOrNull(KernFunc, V, Signed, OverrideType);
  }

  assert(!Reg || Reg->Category == RegCategory::GENERAL ||
         Reg->Category == RegCategory::SAMPLER ||
         Reg->Category == RegCategory::SURFACE ||
         Reg->Category == RegCategory::VME);

  // Write the vISA general operand with region:
  Region R(DstDesc.WrRegion, DstDesc.WrRegionBI);

  if (SignedRes)
    *SignedRes = RegAlloc->getSigned(Reg);

  if (Reg && (Reg->Category == RegCategory::SAMPLER ||
              Reg->Category == RegCategory::SURFACE ||
              Reg->Category == RegCategory::VME)) {
    return createState(Reg, R.Offset / R.ElementBytes, true /*IsDest*/);
  } else {
    auto Decl = Reg ? Reg->GetVar<VISA_GenVar>(Kernel) : nullptr;
    return createRegionOperand(&R, Decl, Signed, Mod, true /*IsDest*/);
  }
}

VISA_VectorOpnd *GenXKernelBuilder::createSourceOperand(
    Instruction *Inst, Signedness Signed, unsigned OperandNum,
    genx::BaleInfo BI, unsigned Mod, Signedness *SignedRes, unsigned MaxWidth) {
  Value *V = Inst->getOperand(OperandNum);
  return createSource(V, Signed, BI.isOperandBaled(OperandNum), Mod, SignedRes,
                      MaxWidth);
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
  Register *Reg = RegAlloc->getRegForValue(KernFunc, V, DONTCARESIGNED);
  assert(Reg->Category == RegCategory::ADDRESS);
  unsigned Width = 1;
  if (VectorType *VT = dyn_cast<VectorType>(V->getType()))
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
  if (VectorType *VT = dyn_cast<VectorType>(T)) {
    // Vector constant.
    auto Splat = V->getSplatValue();
    if (!Splat) {
      // Non-splatted vector constant. Must be a packed vector.
      unsigned NumElements = VT->getNumElements();
      if (VT->getElementType()->isIntegerTy()) {
        // Packed int vector.
        assert(NumElements <= ImmIntVec::Width);
        unsigned Packed = 0;
        for (unsigned i = 0; i != NumElements; ++i) {
          auto El = dyn_cast<ConstantInt>(V->getAggregateElement(i));
          if (!El)
            continue; // undef element
          int This = El->getSExtValue();
          if (This < ImmIntVec::MinUInt) {
            assert(This >= ImmIntVec::MinSInt &&
                "too big imm, cannot encode as vector imm");
            Signed = SIGNED;
          } else if (This > ImmIntVec::MaxSInt) {
            assert(This <= ImmIntVec::MaxUInt &&
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
      assert(VT->getElementType()->isFloatTy() &&
             (NumElements == 1 || NumElements == 2 || NumElements == 4));
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
    const DataLayout &DL = Func->getParent()->getDataLayout();
    T = DL.getIntPtrType(V->getType());
    V = Constant::getNullValue(T);
  }

  // We have a scalar constant.
  if (IntegerType *IT = dyn_cast<IntegerType>(T)) {
    ConstantInt *CI = cast<ConstantInt>(V);
    // I think we need to use the appropriate one of getZExtValue or
    // getSExtValue to avoid an assert on very large 64 bit values...
    int64_t Val = Signed == UNSIGNED ? CI->getZExtValue() : CI->getSExtValue();
    visa::TypeDetails TD(Func->getParent()->getDataLayout(), IT, Signed);
    VISA_VectorOpnd *ImmOp = nullptr;
    CISA_CALL(
        Kernel->CreateVISAImmediate(ImmOp, &Val, getVISAImmTy(TD.VisaType)));
    return ImmOp;
  } if (isa<Function>(V)) {
    assert(0 && "Not baled function address");
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
      assert(T->isDoubleTy());
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
  Register *DstReg = RegAlloc->getRegForValue(KernFunc, CI, UNSIGNED);
  if (!isa<Constant>(CI->getOperand(0))) {
    Instruction *OrigInst = getOriginalInstructionForSource(CI, BI);
    Register *SrcReg = RegAlloc->getRegForValue(KernFunc, OrigInst->getOperand(0));
    (void)SrcReg;
    assert((SrcReg->Category != RegCategory::GENERAL ||
            DstReg->Category != RegCategory::GENERAL) &&
           "expected a category conversion");
  }

  if (DstReg->Category != RegCategory::ADDRESS) {
    // State copy.
    int ExecSize = 1;
    if (VectorType *VT = dyn_cast<VectorType>(CI->getType())) {
      ExecSize = VT->getNumElements();
    }

    auto ISAExecSize = static_cast<VISA_Exec_Size>(genx::log2(ExecSize));
    auto Dst = createDestination(CI, UNSIGNED, 0, DstDesc);
    auto Src = createSourceOperand(CI, UNSIGNED, 0, BI);
    addDebugInfo();
    CISA_CALL(Kernel->AppendVISADataMovementInst(
        ISA_MOVS, nullptr /*Pred*/, false /*Mod*/,
        NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1, ISAExecSize, Dst, Src));
    return;
  }

  // Destination is address register.
  int ExecSize = 1;
  if (VectorType *VT = dyn_cast<VectorType>(CI->getType())) {
    report_fatal_error("vector of addresses not implemented");
  }

  auto ISAExecSize = static_cast<VISA_Exec_Size>(genx::log2(ExecSize));
  Register *SrcReg = RegAlloc->getRegForValue(KernFunc, CI->getOperand(0));
  assert(SrcReg->Category == RegCategory::ADDRESS);

  (void)SrcReg;
  // This is an address->address copy, inserted due to coalescing failure of
  // the address for an indirected arg in GenXArgIndirection.
  // (A conversion to address is handled in buildConvertAddr below.)
  // Write the addr_add instruction.
  Value *SrcOp0 = CI->getOperand(0);
  unsigned Src0Width = 1;
  if (VectorType *VT = dyn_cast<VectorType>(SrcOp0->getType()))
    Src0Width = VT->getNumElements();

  Register *RegDst = RegAlloc->getRegForValue(KernFunc, CI, DONTCARESIGNED);
  Register *RegSrc0 = RegAlloc->getRegForValue(KernFunc, SrcOp0, DONTCARESIGNED);

  VISA_VectorOpnd *Dst = nullptr, *Src0 = nullptr, *Src1 = nullptr;

  CISA_CALL(Kernel->CreateVISAAddressDstOperand(
      Dst, RegDst->GetVar<VISA_AddrVar>(Kernel), 0));
  CISA_CALL(Kernel->CreateVISAAddressSrcOperand(
      Src0, RegSrc0->GetVar<VISA_AddrVar>(Kernel), 0, Src0Width));
  Src1 =
      createImmediateOperand(Constant::getNullValue(CI->getType()), UNSIGNED);

  addDebugInfo();
  CISA_CALL(Kernel->AppendVISAAddrAddInst(vISA_EMASK_M1_NM, ISAExecSize, Dst,
                                          Src0, Src1));
}

VISA_VectorOpnd *GenXKernelBuilder::createSource(CisaVariable *V,
                                                 Signedness Signed,
                                                 unsigned MaxWidth,
                                                 unsigned *Offset) {
  Region R(VectorType::get(
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
                                                 unsigned *Offset) {
  if (SignedRes)
    *SignedRes = Signed;
  if (auto C = dyn_cast<Constant>(V)) {
    if (Mod) {
      // Need to negate constant.
      assert(Mod == MODIFIER_NEG && "unexpected modifier");
      if (C->getType()->isIntOrIntVectorTy())
        C = ConstantExpr::getNeg(C);
      else
        C = ConstantExpr::getFNeg(C);
    }
    return createImmediateOperand(C, Signed);
  }
  if (!Baled) {
    Register *Reg = RegAlloc->getRegForValue(KernFunc, V, Signed);
    assert(Reg->Category == RegCategory::GENERAL ||
           Reg->Category == RegCategory::SURFACE ||
           Reg->Category == RegCategory::SAMPLER ||
           Reg->Category == RegCategory::VME);
    // Write the vISA general operand.
    Region R(V);
    if (Offset)
      R.Offset = *Offset;
    if (R.NumElements == 1)
      R.VStride = R.Stride = 0;
    if (SignedRes)
      *SignedRes = RegAlloc->getSigned(Reg);
    if (Reg->Category == RegCategory::GENERAL) {
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
    Register *Reg = RegAlloc->getRegForValueOrNull(KernFunc, V, Signed);

    // Ensure we pick a non-DONTCARESIGNED signedness here, as, for an
    // indirect region and DONTCARESIGNED, writeRegion arbitrarily picks a
    // signedness as it is attached to the operand, unlike a direct region
    // where it is attached to the vISA register.
    if (Reg)
      Signed = RegAlloc->getSigned(Reg);
    else if (Signed == DONTCARESIGNED)
      Signed = SIGNED;
    // Write the vISA general operand with region.
    Region R(Inst, Baling->getBaleInfo(Inst));
    if (Offset)
      R.Offset = *Offset;
    if (R.NumElements == 1)
      R.VStride = 0;
    if (R.Width == 1)
      R.Stride = 0;
    if (!Reg || Reg->Category == RegCategory::GENERAL || R.Indirect) {
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
    llvm_unreachable("unknown bale type");
    break;
  }
  return createSource(Inst->getOperand(Idx), Signed, BI.isOperandBaled(Idx),
                      Mod, SignedRes, MaxWidth);
}

std::string GenXKernelBuilder::createInlineAsmOperand(
    Register *Reg, genx::Region *R, bool IsDst, genx::Signedness Signed,
    genx::ConstraintType Ty, unsigned Mod) {
  deduceRegion(R, IsDst);

  VISA_VectorOpnd *ResultOperand = nullptr;
  switch (Ty) {
  default:
    llvm_unreachable("constraint unhandled");
  case ConstraintType::Constraint_cr: {
    assert(Reg && Reg->Category == RegCategory::PREDICATE);
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
    if (!R->Indirect)
      report_fatal_error("Inline asm operand can'be indirected here");
    ResultOperand = createIndirectOperand(R, Signed, Mod, IsDst);
    break;
  }
  return Kernel->getVectorOperandName(ResultOperand, true);
}

std::string GenXKernelBuilder::createInlineAsmDestinationOperand(
    Value *Dest, genx::Signedness Signed, genx::ConstraintType Ty, unsigned Mod,
    const DstOpndDesc &DstDesc) {

  Type *OverrideType = nullptr;

  // Saturation can also change signedness.
  if (!Dest->user_empty() && GenXIntrinsic::isIntegerSat(Dest->user_back())) {
    Signed = getISatDstSign(Dest->user_back());
  }

  if (!DstDesc.WrRegion) {
    Register *Reg = RegAlloc->getRegForValue(KernFunc, Dest, Signed, OverrideType);

    Region DestR(Dest);
    return createInlineAsmOperand(Reg, &DestR, true /*IsDst*/, DONTCARESIGNED,
                                  Ty, Mod);
  }
  // We need to allow for the case that there is no register allocated if it is
  // an indirected arg, and that is OK because the region is indirect so the
  // vISA does not contain the base register.
  Register *Reg;

  Value *V = nullptr;
  if (DstDesc.GStore) {
    auto GV = getUnderlyingGlobalVariable(DstDesc.GStore->getOperand(1));
    assert(GV && "out of sync");
    if (OverrideType == nullptr)
      OverrideType = DstDesc.GStore->getOperand(0)->getType();
    Reg = RegAlloc->getRegForValue(KernFunc, GV, Signed, OverrideType);
    V = GV;
  } else {
    V = DstDesc.WrRegion;
    Reg = RegAlloc->getRegForValueOrNull(KernFunc, V, Signed, OverrideType);
  }

  assert(!Reg || Reg->Category == RegCategory::GENERAL);

  // Write the vISA general operand with region:
  Region R(DstDesc.WrRegion, DstDesc.WrRegionBI);

  return createInlineAsmOperand(Reg, &R, true /*IsDst*/, Signed, Ty, Mod);
}

std::string GenXKernelBuilder::createInlineAsmSourceOperand(
    Value *V, genx::Signedness Signed, bool Baled, genx::ConstraintType Ty,
    unsigned Mod, unsigned MaxWidth) {

  if (auto C = dyn_cast<Constant>(V)) {
    if (Ty != genx::ConstraintType::Constraint_n) {
      if (Mod) {
        // Need to negate constant.
        assert(Mod == MODIFIER_NEG && "unexpected modifier");
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
    Register *Reg = RegAlloc->getRegForValue(KernFunc, V, Signed);
    Region R(V);
    if (R.NumElements == 1)
      R.VStride = R.Stride = 0;

    return createInlineAsmOperand(Reg, &R, false /*IsDst*/, Signed, Ty, Mod);
  }

  Instruction *Inst = cast<Instruction>(V);
  BaleInfo BI(Baling->getBaleInfo(Inst));
  assert(BI.Type == BaleInfo::RDREGION);
  // The source operand has a rdregion baled in. We need to allow for the
  // case that there is no register allocated if it is an indirected arg,
  // and that is OK because the region is indirect so the vISA does not
  // contain the base register.
  V = Inst->getOperand(0);
  Register *Reg = RegAlloc->getRegForValue(KernFunc, V, Signed);

  // Ensure we pick a non-DONTCARESIGNED signedness here, as, for an
  // indirect region and DONTCARESIGNED, writeRegion arbitrarily picks a
  // signedness as it is attached to the operand, unlike a direct region
  // where it is attached to the vISA register.
  if (Signed == DONTCARESIGNED)
    Signed = SIGNED;
  // Write the vISA general operand with region.
  Region R(Inst, Baling->getBaleInfo(Inst));
  if (R.NumElements == 1)
    R.VStride = 0;
  if (R.Width == 1)
    R.Stride = 0;

  assert(Reg->Category == RegCategory::GENERAL || R.Indirect);

  return createInlineAsmOperand(Reg, &R, false /*IsDst*/, Signed, Ty, Mod);
}

/***********************************************************************
 * getPredicateVar : get predicate var from value
 */
VISA_PredVar *GenXKernelBuilder::getPredicateVar(Value *V) {
  auto Reg = RegAlloc->getRegForValue(KernFunc, V, DONTCARESIGNED);
  assert(Reg && Reg->Category == RegCategory::PREDICATE);
  return getPredicateVar(Reg);
}

/***********************************************************************
 * getZeroedPredicateVar : get predicate var from value with zeroing it
 */
VISA_PredVar *GenXKernelBuilder::getZeroedPredicateVar(Value *V) {
  auto Reg = RegAlloc->getRegForValue(KernFunc, V, DONTCARESIGNED);
  assert(Reg && Reg->Category == RegCategory::PREDICATE);
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
  assert(R);
  return R->Num >= visa::VISA_NUM_RESERVED_PREDICATES
             ? R->GetVar<VISA_PredVar>(Kernel)
             : nullptr;
}

void GenXKernelBuilder::buildSelectInst(SelectInst *SI, BaleInfo BI,
                                        unsigned Mod,
                                        const DstOpndDesc &DstDesc) {
  unsigned ExecSize = 1;
  if (VectorType *VT = dyn_cast<VectorType>(SI->getType()))
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

  VISA_VectorOpnd *Dst = createDestination(SI, DONTCARESIGNED, Mod, DstDesc);
  VISA_VectorOpnd *Src0 = createSourceOperand(SI, DONTCARESIGNED, 1, BI);
  VISA_VectorOpnd *Src1 = createSourceOperand(SI, DONTCARESIGNED, 2, BI);

  addDebugInfo();
  CISA_CALL(Kernel->AppendVISADataMovementInst(
      ISA_SEL, PredOp, Mod & MODIFIER_SAT, MaskCtrl,
      getExecSizeFromValue(ExecSize), Dst, Src0, Src1));
}

void GenXKernelBuilder::buildBitCast(CastInst *CI, genx::BaleInfo BI,
                                     unsigned Mod, const DstOpndDesc &DstDesc) {
  if (!isMaskPacking(CI))
    assert(!BI.Bits && !Mod && !DstDesc.WrRegion &&
           "non predicate bitcast should not be baled with anything");

  if (CI->getType()->getScalarType()->isIntegerTy(1)) {
    if (CI->getOperand(0)->getType()->getScalarType()->isIntegerTy(1)) {
      if (auto C = dyn_cast<Constant>(CI->getOperand(0))) {
        auto Reg = RegAlloc->getRegForValueOrNull(KernFunc, CI, DONTCARESIGNED);
        if (!Reg)
          return; // write to EM/RM value, ignore
        // We can move a constant predicate to a predicate register
        // using setp, if we get the constant predicate as a single int.
        unsigned IntVal = getPredicateConstantAsInt(C);
        unsigned Size = C->getType()->getPrimitiveSizeInBits();
        C = ConstantInt::get(
            Type::getIntNTy(CI->getContext(), std::max(Size, 8U)), IntVal);

        addDebugInfo();
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
      assert(RegAlloc->getRegForValue(KernFunc, CI, DONTCARESIGNED) ==
                 RegAlloc->getRegForValue(KernFunc, CI->getOperand(0), DONTCARESIGNED) &&
             "uncoalesced phi move of predicate");
      return;
    }

    VISA_PredVar *PredVar = getPredicateVar(CI);

    addDebugInfo();
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
    if (VectorType *VT = dyn_cast<VectorType>(CI->getType()))
      ExecSize = VT->getNumElements();

    VISA_EMask_Ctrl ctrlMask = getExecMaskFromWrRegion(DstDesc, true);
    VISA_Exec_Size execSize = getExecSizeFromValue(ExecSize);
    addDebugInfo();
    CISA_CALL(Kernel->AppendVISADataMovementInst(
        ISA_MOV, createPredFromWrRegion(DstDesc), Mod & MODIFIER_SAT, ctrlMask,
        execSize, createDestination(CI, DONTCARESIGNED, Mod, DstDesc),
        createSourceOperand(CI, DONTCARESIGNED, 0, BI)));
    return;
  }
  if (CI->getOperand(0)->getType()->getScalarType()->isIntegerTy(1)) {
    // Bitcast from predicate to scalar int
    Register *PredReg =
        RegAlloc->getRegForValue(KernFunc, CI->getOperand(0), DONTCARESIGNED);
    assert(PredReg->Category == RegCategory::PREDICATE);
    addDebugInfo();
    CISA_CALL(Kernel->AppendVISAPredicateMove(
        createDestination(CI, UNSIGNED, 0, DstDesc),
        PredReg->GetVar<VISA_PredVar>(Kernel)));

    return;
  }

  // Real bitcast with possibly different types. Use whichever type has the
  // largest element size, so we minimize the number of channels used in the
  // move.
  Type *Ty = CI->getOperand(0)->getType();
  if (Ty->getScalarType()->getPrimitiveSizeInBits() <
      CI->getType()->getScalarType()->getPrimitiveSizeInBits())
    Ty = CI->getType();
  if (Liveness->isBitCastCoalesced(cast<BitCastInst>(CI)))
    return; // bitcast was coalesced away
  Register *DstReg = RegAlloc->getRegForValue(KernFunc, CI, DONTCARESIGNED, Ty);
  // Give dest and source the same signedness for byte mov.
  auto Signed = RegAlloc->getSigned(DstReg);
  Register *SrcReg = RegAlloc->getRegForValue(KernFunc, CI->getOperand(0), Signed, Ty);
  VISA_Exec_Size ExecSize = EXEC_SIZE_1;
  if (VectorType *VT = dyn_cast<VectorType>(Ty))
    ExecSize = getExecSizeFromValue(VT->getNumElements());
  assert(ExecSize >= EXEC_SIZE_1 && ExecSize <= EXEC_SIZE_32 &&
         "illegal exec size in bitcast: should have been coalesced away");
  // destination
  Region DestR(CI);
  // source
  Region SourceR(CI->getOperand(0));

  VISA_EMask_Ctrl ctrlMask = NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
  addDebugInfo();
  CISA_CALL(Kernel->AppendVISADataMovementInst(
      ISA_MOV, nullptr, Mod, ctrlMask, ExecSize,
      createRegionOperand(&DestR, DstReg->GetVar<VISA_GenVar>(Kernel), DONTCARESIGNED,
                          0, true),
      createRegionOperand(&SourceR, SrcReg->GetVar<VISA_GenVar>(Kernel), Signed, 0,
                          false)));
}

void GenXKernelBuilder::buildFunctionAddr(Instruction *Inst,
                                          const DstOpndDesc &DstDesc) {

  auto *Dst = createDestination(Inst, DONTCARESIGNED, MODIFIER_NONE, DstDesc);
  assert(Dst);
  auto *F = cast<Function>(cast<PtrToIntInst>(Inst)->getPointerOperand());
  CISA_CALL(Kernel->AppendVISACFSymbolInst(F->getName(), Dst));
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
  if (VectorType *VT = dyn_cast<VectorType>(Input->getType()))
    ExecSize = getExecSizeFromValue(VT->getNumElements());

  VISA_EMask_Ctrl ExecMask = getExecMaskFromWrRegion(DstDesc, true);

  // TODO: fix signedness of the source
  addDebugInfo();
  CISA_CALL(Kernel->AppendVISADataMovementInst(
      ISA_MOV, createPredFromWrRegion(DstDesc), false, ExecMask, ExecSize,
      createDestination(Input, DONTCARESIGNED, 0, DstDesc),
      createSource(Input, DONTCARESIGNED, false, 0)));
}

/***********************************************************************
 * buildLoneWrPredRegion : build a lone wrpredregion
 */
void GenXKernelBuilder::buildLoneWrPredRegion(Instruction *Inst, BaleInfo BI) {
  enum { OperandNum = 1 };
  Value *Input = Inst->getOperand(OperandNum);
  assert(isa<Constant>(Input));
  auto C = dyn_cast<Constant>(Input);
  assert(C);
  unsigned Size = C->getType()->getPrimitiveSizeInBits();

  VISA_EMask_Ctrl ctrlMask = getExecMaskFromWrPredRegion(Inst, true);
  VISA_Exec_Size execSize = getExecSizeFromValue(Size);

  unsigned IntVal = getPredicateConstantAsInt(C);
  C = ConstantInt::get(Type::getIntNTy(Inst->getContext(), std::max(Size, 8U)),
                       IntVal);
  addDebugInfo();
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
  if (VectorType *VT = dyn_cast<VectorType>(Inst->getType()))
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
      DstReg = RegAlloc->getRegForValueOrNull(KernFunc, WrRegion, DONTCARESIGNED);
    } else {
      DstReg = RegAlloc->getRegForValue(KernFunc, Inst, DONTCARESIGNED);
    }
    if (DstReg && (DstReg->Category == RegCategory::SURFACE ||
                   DstReg->Category == RegCategory::SAMPLER ||
                   DstReg->Category == RegCategory::VME)) {
      Opcode = ISA_MOVS;
    }
  }
  // TODO: mb need to get signed from dest for src and then modify that
  addDebugInfo();
  CISA_CALL(Kernel->AppendVISADataMovementInst(
      Opcode, (Opcode != ISA_MOVS ? createPredFromWrRegion(DstDesc) : nullptr),
      Mod & MODIFIER_SAT, ExecMask, ExecSize, Dest,
      createSource(Src, Signed, Baled, 0)));
}

static unsigned getResultedTypeSize(Type *Ty) {
  unsigned TySz = 0;
  if (Ty->isVectorTy())
    TySz = Ty->getVectorNumElements() *
           getResultedTypeSize(Ty->getVectorElementType());
  else if (Ty->isArrayTy())
    TySz = Ty->getArrayNumElements() *
           getResultedTypeSize(Ty->getArrayElementType());
  else if (Ty->isStructTy()) {
    StructType *STy = dyn_cast<StructType>(Ty);
    assert(STy);
    for (Type *Ty : STy->elements())
      TySz += getResultedTypeSize(Ty);
  } else if (Ty->isPointerTy() && Ty->getPointerElementType()->isFunctionTy()) {
    TySz = BYTES_PER_FADDR;
  } else {
    TySz = Ty->getPrimitiveSizeInBits() / CHAR_BIT;
    assert(TySz && "Ty is not primitive?");
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
  else if (auto RI = dyn_cast<ReturnInst>(Inst->use_begin()->getUser()))
    return RI->getFunction()->hasFnAttribute(genx::FunctionMD::CMStackCall) ||
           RI->getFunction()->hasFnAttribute(
               genx::FunctionMD::ReferencedIndirectly);
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
  } else if (CmpInst *Cmp = dyn_cast<CmpInst>(Inst)) {
    buildCmp(Cmp, BI, DstDesc);
  } else if (BinaryOperator *BO = dyn_cast<BinaryOperator>(Inst)) {
    if (!BO->getType()->getScalarType()->isIntegerTy(1)) {
      buildBinaryOperator(BO, BI, Mod, DstDesc);
    } else {
      assert(!Mod && !DstDesc.WrRegion && !BI.isOperandBaled(0) &&
             !BI.isOperandBaled(1));
      buildBoolBinaryOperator(BO);
    }
  } else if (auto EVI = dyn_cast<ExtractValueInst>(Inst)) {
    if (auto *CI = dyn_cast<IGCLLVM::CallInst>(Inst->getOperand(0)))
      // translate extraction of structured type from retv
      if (!CI->isInlineAsm() && (CI->getCalledFunction()->hasFnAttribute(
                                     genx::FunctionMD::CMStackCall) ||
                                 CI->isIndirectCall()))
        buildExtractRetv(EVI);
    // no code generated
  } else if (auto IVI = dyn_cast<InsertValueInst>(Inst)) {
    if (checkInsertToRetv(IVI)
        // TODO: safely remove this tmp workaround for failed coalescing cases
        // and insert-extract-insert chains
        && !isa<BitCastInst>(Inst->getOperand(1)))
      RetvInserts.push_back(IVI);
    // no code generated
  } else if (BitCastInst *BCI = dyn_cast<BitCastInst>(Inst)) {
    buildBitCast(BCI, BI, Mod, DstDesc);
  } else if (CastInst *CI = dyn_cast<CastInst>(Inst)) {
    auto ptr2Int = dyn_cast<PtrToIntInst>(CI);
    if (ptr2Int && isa<GlobalVariable>(CI->getOperand(0))) {
      buildSymbolInst(ptr2Int, Mod, DstDesc);
    } else {
      buildCastInst(CI, BI, Mod, DstDesc);
    }
  } else if (auto SI = dyn_cast<SelectInst>(Inst)) {
    buildSelectInst(SI, BI, Mod, DstDesc);
  } else if (auto LI = dyn_cast<LoadInst>(Inst)) {
    (void)LI; // no code generated
  } else if (auto GEPI = dyn_cast<GetElementPtrInst>(Inst)) {
    // check if gepi def is used in intrinsic, otherwise report error
    auto GepiChecker = [](Use &ui) {
      auto ci = cast<CallInst>(ui.getUser());
      Function *Callee = ci->getCalledFunction();
      unsigned IntrinID = GenXIntrinsic::getAnyIntrinsicID(Callee);
      return (IntrinID == GenXIntrinsic::genx_print_format_index);
    };
    if (!std::all_of(GEPI->use_begin(), GEPI->use_end(), GepiChecker)) {
      report_fatal_error("gep is supported only for printf");
    }
#if (LLVM_VERSION_MAJOR > 8)
  } else if (UnaryOperator *UO = dyn_cast<UnaryOperator>(Inst)) {
    buildUnaryOperator(UO, BI, Mod, DstDesc);
#endif
  } else if (auto *CI = dyn_cast<IGCLLVM::CallInst>(Inst)) {
    if (CI->isInlineAsm())
      buildInlineAsm(CI);
    else if (CI->isIndirectCall()) {
      assert(!Mod && !DstDesc.WrRegion &&
             "cannot bale subroutine call into anything");
      buildCall(CI, DstDesc);
    } else {
      Function *Callee = CI->getCalledFunction();
      unsigned IntrinID = GenXIntrinsic::getAnyIntrinsicID(Callee);
      switch (IntrinID) {
      case Intrinsic::dbg_value:
      case Intrinsic::dbg_declare:
      case GenXIntrinsic::genx_predefined_surface:
      case GenXIntrinsic::genx_output:
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
      case GenXIntrinsic::genx_print_format_index:
        buildPrintIndex(CI, IntrinID, Mod, DstDesc);
        break;
      case GenXIntrinsic::genx_convert_addr:
        buildConvertAddr(CI, BI, Mod, DstDesc);
        break;
      case GenXIntrinsic::genx_alloca:
        buildAlloca(CI, IntrinID, Mod, DstDesc);
        break;
      case GenXIntrinsic::genx_constanti:
      case GenXIntrinsic::genx_constantf:
      case GenXIntrinsic::genx_constantpred:
        if (isa<UndefValue>(CI->getOperand(0)))
          return false; // Omit llvm.genx.constant with undef operand.
        if (!DstDesc.WrRegion && !RegAlloc->getRegForValueOrNull(KernFunc, CI))
          return false; // Omit llvm.genx.constantpred that is EM or RM and so
                        // does not have a register allocated.
                        // fall through...
      case GenXIntrinsic::genx_barrier:
        HasBarrier = true;
      default:
        if (!(CI->user_empty() &&
              GenXIntrinsic::getAnyIntrinsicID(CI->getCalledFunction()) ==
                  GenXIntrinsic::genx_any))
          buildIntrinsic(CI, IntrinID, BI, Mod, DstDesc);
        break;
      case GenXIntrinsic::not_any_intrinsic:
        assert(!Mod && !DstDesc.WrRegion &&
               "cannot bale subroutine call into anything");
        buildCall(CI, DstDesc);
        break;
      }
    }
  } else if (isa<UnreachableInst>(Inst))
    ; // no code generated
  else
    report_fatal_error("main inst not implemented");

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
#ifndef NDEBUG
  for (unsigned i = 0, e = Phi->getNumIncomingValues(); i != e; ++i) {
    Value *Incoming = Phi->getIncomingValue(i);
    // This assert has to cope with the case that the phi node has no live
    // range because it is part of an indirected arg/retval in
    // GenXArgIndirection, or it is an EM/RM category.
    if (!isa<UndefValue>(Incoming))
      if (auto LR = Liveness->getLiveRangeOrNull(Incoming))
        if (LR->getCategory() < RegCategory::NUMREALCATEGORIES)
          assert(LR == Liveness->getLiveRangeOrNull(Phi) &&
                 "mismatched registers in phi node");
  }
#endif
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
  Value *BranchTarget = nullptr;
  VISA_PREDICATE_STATE StateInvert = PredState_NO_INVERSE;
  if (!Branch ||
      Branch->getSuccessor(1) == Branch->getParent()->getNextNode()) {
    // Forward goto.  Find the join.
    auto Join = GotoJoin::findJoin(Goto);
    assert(Join && "join not found");
    BranchTarget = Join;
    StateInvert = PredState_INVERSE;
  } else {
    assert(Branch->getSuccessor(0) == Branch->getParent()->getNextNode() &&
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
  assert(!Mask && "cannot have rdpredregion baled into goto");

  Register *PredReg = nullptr;
  if (auto C = dyn_cast<Constant>(Pred)) {
    (void)C;
    if (StateInvert)
      assert(C->isNullValue() &&
             "predication operand must be constant 0 or not constant");
    else
      assert(C->isAllOnesValue() &&
             "predication operand must be constant 1 or not constant");
  } else {
    State ^= StateInvert;
    PredReg = RegAlloc->getRegForValueOrNull(KernFunc, Pred);
    assert(PredReg && PredReg->Category == RegCategory::PREDICATE);
  }

  uint8_t execSize = genx::log2(Pred->getType()->getVectorNumElements());

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
  addDebugInfo();
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
  llvm_unreachable("Unexpected EM offset");
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
      assert(Inst);
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
  auto &DL = FG->getModule()->getDataLayout();

  VISA_GenVar *parentDecl = nullptr;
  VISA_GenVar *Decl = nullptr;

  if (!Reg.AliasTo) {
    // This is not an aliased register. Go through all the aliases and
    // determine the biggest alignment required. If the register is at least
    // as big as a GRF, make the alignment GRF.
    unsigned Alignment = getLogAlignment(
        VISA_Align::ALIGN_GRF, Subtarget ? Subtarget->getGRFWidth()
                                         : defaultGRFWidth); // GRF alignment
    Type *Ty = Reg.Ty;
    unsigned NBits = Ty->isPointerTy() ? DL.getPointerSizeInBits()
                                       : Ty->getPrimitiveSizeInBits();
    if (NBits < GrfByteSize * 8 /* bits in GRF */) {
      Alignment = 0;
      for (Register *AliasReg = &Reg; AliasReg;
           AliasReg = AliasReg->NextAlias[KernFunc]) {
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
    for (Register *AliasReg = &Reg; AliasReg; AliasReg = AliasReg->NextAlias[KernFunc]) {
      if (AliasReg->Alignment < Alignment)
        AliasReg->Alignment = Alignment;
    }
  } else {
    if (Reg.AliasTo->Num < visa::VISA_NUM_RESERVED_REGS) {
      CISA_CALL(Kernel->GetPredefinedVar(parentDecl,
                                         (PreDefined_Vars)Reg.AliasTo->Num));
      assert(parentDecl && "Predefeined variable is null");
    } else {
      parentDecl = Reg.AliasTo->GetVar<VISA_GenVar>(Kernel);
      assert(parentDecl && "Refers to undefined var");
    }
  }

  visa::TypeDetails TD(DL, Reg.Ty, Reg.Signed);

  VISA_Align VA = getVISA_Align(
      Reg.Alignment, Subtarget ? Subtarget->getGRFWidth() : defaultGRFWidth);
  CISA_CALL(Kernel->CreateVISAGenVar(Decl, Reg.NameStr.c_str(), TD.NumElements,
                                     static_cast<VISA_Type>(TD.VisaType), VA,
                                     parentDecl, 0));

  Reg.SetVar(Kernel, Decl);

  for (auto &Attr : Reg.Attributes) {
    CISA_CALL(Kernel->AddAttributeToVar(
        Decl, getStringByIndex(Attr.first).begin(), Attr.second.size(),
        (void *)(Attr.second.c_str())));
  }
}
/**************************************************************************************************
 * Scan ir to collect information about whether kernel has callable function or
 * barrier.
 */
void GenXKernelBuilder::collectKernelInfo() {
  UseGlobalMem |=
      (FG->getModule()->getModuleFlag("genx.useGlobalMem") != nullptr);
  for (auto It = FG->begin(), E = FG->end(); It != E; ++It) {
    auto Func = *It;
    HasStackcalls |=
        Func->hasFnAttribute(genx::FunctionMD::CMStackCall) ||
        Func->hasFnAttribute(genx::FunctionMD::ReferencedIndirectly);
    for (auto &BB : *Func) {
      for (auto &I : BB) {
        if (CallInst *CI = dyn_cast<CallInst>(&I)) {
          if (CI->isInlineAsm())
            continue;
          if (GenXIntrinsicInst *II = dyn_cast<GenXIntrinsicInst>(CI)) {
            auto IID = II->getIntrinsicID();
            if (IID == GenXIntrinsic::genx_barrier)
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
    case RegCategory::GENERAL:
      AddGenVar(*Reg);
      break;

    case RegCategory::ADDRESS: {
      VISA_AddrVar *Decl = nullptr;
      unsigned NumElements = 1;
      if (VectorType *VT = dyn_cast<VectorType>(Reg->Ty))
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

    case RegCategory::PREDICATE: {
      VISA_PredVar *Decl = nullptr;
      unsigned NumElements = 1;
      if (VectorType *VT = dyn_cast<VectorType>(Reg->Ty))
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

    case RegCategory::SAMPLER: {
      unsigned NumElements = 1;
      if (VectorType *VT = dyn_cast<VectorType>(Reg->Ty))
        NumElements = VT->getNumElements();
      VISA_SamplerVar *Decl = nullptr;
      CISA_CALL(Kernel->CreateVISASamplerVar(Decl, Reg->NameStr.c_str(),
                                             NumElements));
      Reg->SetVar(Kernel, Decl);
    } break;

    case RegCategory::SURFACE: {
      VISA_SurfaceVar *Decl = nullptr;
      if (Reg->Num < visa::VISA_NUM_RESERVED_SURFACES) {
        Kernel->GetPredefinedSurface(Decl, (PreDefined_Surface)Reg->Num);
      } else {
        unsigned NumElements = 1;
        if (VectorType *VT = dyn_cast<VectorType>(Reg->Ty))
          NumElements = VT->getNumElements();

        CISA_CALL(Kernel->CreateVISASurfaceVar(Decl, Reg->NameStr.c_str(),
                                               NumElements));
      }
      Reg->SetVar(Kernel, Decl);
    } break;

    case RegCategory::VME:
      report_fatal_error("VME variable is no longer supported");
      break;

    default:
      report_fatal_error("Unknown category for register");
      break;
    }
  }

  VISA_GenVar *ArgDecl = nullptr, *RetDecl = nullptr;
  Kernel->GetPredefinedVar(ArgDecl, PREDEFINED_ARG);
  Kernel->GetPredefinedVar(RetDecl, PREDEFINED_RET);
  createCisaVariable(Kernel, "argv", ArgDecl, ARG_SIZE_IN_GRFS * GrfByteSize);
  createCisaVariable(Kernel, "retv", RetDecl, RET_SIZE_IN_GRFS * GrfByteSize);
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
    assert(MaskOffset < 32 && "unexpected mask offset");
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
    if ((isa<Constant>(Mask) || RegAlloc->getRegForValueOrNull(KernFunc, Mask)) && NoMask)
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
    if (!Const)
      report_fatal_error("Incorrect args to intrinsic call");
    return static_cast<unsigned>(Const->getSExtValue());
  };

  auto CreateSurfaceOperand = [&](II::ArgInfo AI) {
    llvm::Value *Arg = CI->getArgOperand(AI.getArgIdx());
    VISA_SurfaceVar *SurfDecl = nullptr;
    int Index = visa::convertToSurfaceIndex(Arg);
    if (visa::isReservedSurfaceIndex(Index)) {
      Kernel->GetPredefinedSurface(SurfDecl, visa::getReservedSurface(Index));
    } else {
      Register *Reg = RegAlloc->getRegForValue(KernFunc, Arg);
      assert(Reg->Category == RegCategory::SURFACE &&
             "Expected surface register");
      SurfDecl = Reg->GetVar<VISA_SurfaceVar>(Kernel);
    }
    VISA_StateOpndHandle *ResultOperand = nullptr;
    CISA_CALL(Kernel->CreateVISAStateOperandHandle(ResultOperand, SurfDecl));
    return ResultOperand;
  };

  auto CreateSamplerOperand = [&](II::ArgInfo AI) {
    Register *Reg = RegAlloc->getRegForValue(KernFunc, CI->getArgOperand(AI.getArgIdx()));
    assert(Reg->Category == RegCategory::SAMPLER &&
           "Expected sampler register");
    VISA_StateOpndHandle *ResultOperand = nullptr;
    CISA_CALL(Kernel->CreateVISAStateOperandHandle(
        ResultOperand, Reg->GetVar<VISA_SamplerVar>(Kernel)));
    return ResultOperand;
  };

  auto GetMediaHeght = [&](II::ArgInfo AI) {
    // constant byte for media height that we need to infer from the
    // media width and the return type or final arg
    ConstantInt *Const =
        dyn_cast<ConstantInt>(CI->getArgOperand(AI.getArgIdx()));
    if (!Const)
      report_fatal_error("Incorrect args to intrinsic call");
    unsigned Width = Const->getZExtValue();
    if (Width == 0 || Width > 64)
      report_fatal_error("Invalid media width");
    unsigned RoundedWidth = 1 << genx::log2(Width);
    if (RoundedWidth < Width)
      RoundedWidth *= 2;
    if (RoundedWidth < 4)
      RoundedWidth = 4;
    Type *DataType = CI->getType();
    if (DataType->isVoidTy())
      DataType = CI->getOperand(CI->getNumArgOperands() - 1)->getType();
    unsigned DataSize;
    if (VectorType *VT = dyn_cast<VectorType>(DataType))
      DataSize = VT->getElementType()->getPrimitiveSizeInBits() / 8 *
                 VT->getNumElements();
    else
      DataSize = DataType->getPrimitiveSizeInBits() / 8;
    if (DataSize <= RoundedWidth && DataSize >= Width)
      return static_cast<uint8_t>(1);
    if (DataSize % RoundedWidth)
      report_fatal_error("Invalid media width");
    return static_cast<uint8_t>(DataSize / RoundedWidth);
  };

  auto CreateOperand = [&](II::ArgInfo AI) {
    VISA_VectorOpnd *ResultOperand = nullptr;
    Signedness Signed = DONTCARESIGNED;
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
        MaxWidth = CI->getType()->getVectorNumElements();
      }
      ResultOperand = createSourceOperand(CI, Signed, AI.getArgIdx(), BI, 0,
                                          nullptr, MaxWidth);
    }
    return ResultOperand;
  };

  auto CreateRawOperand = [&](II::ArgInfo AI) {
    VISA_RawOpnd *ResultOperand = nullptr;
    auto Signed = DONTCARESIGNED;
    if (AI.needsSigned())
      Signed = SIGNED;
    else if (AI.needsUnsigned())
      Signed = UNSIGNED;
    if (AI.isRet()) {
      assert(!Mod);
      ResultOperand = createRawDestination(CI, DstDesc, Signed);
    } else if (AI.getArgIdx() < MaxRawOperands)
      ResultOperand = createRawSourceOperand(CI, AI.getArgIdx(), BI, Signed);
    return ResultOperand;
  };

  auto CreateRawOperands = [&](II::ArgInfo AI, VISA_RawOpnd **Operands) {
    assert(MaxRawOperands != std::numeric_limits<int>::max() &&
           "MaxRawOperands must be defined");
    for (int i = 0; i < AI.getArgIdx() + MaxRawOperands; ++i) {
      Operands[i] = CreateRawOperand(II::ArgInfo(II::RAW | (AI.Info + i)));
    }
  };

  auto GetOwords = [&](II::ArgInfo AI) {
    // constant byte for log2 number of owords
    Value *Arg = CI;
    if (!AI.isRet())
      Arg = CI->getOperand(AI.getArgIdx());
    VectorType *VT = dyn_cast<VectorType>(Arg->getType());
    if (!VT)
      report_fatal_error("Invalid number of owords");
    int DataSize = VT->getNumElements() *
                   DL.getTypeSizeInBits(VT->getElementType()) / 8;
    DataSize = genx::exactLog2(DataSize) - 4;
    if (DataSize < 0 || DataSize > 4)
      report_fatal_error("Invalid number of words");
    return static_cast<VISA_Oword_Num>(DataSize);
  };

  auto GetExecSize = [&](II::ArgInfo AI, VISA_EMask_Ctrl *Mask) {
    int ExecSize = GenXIntrinsicInfo::getOverridedExecSize(CI, Subtarget);
    if (ExecSize == 0) {
      if (VectorType *VT = dyn_cast<VectorType>(CI->getType())) {
        ExecSize = VT->getNumElements();
      } else {
        ExecSize = 1;
      }
    }
    bool IsNoMask = AI.getCategory() == II::EXECSIZE_NOMASK;
    *Mask = getExecMaskFromWrRegion(DstDesc, IsNoMask);
    return getExecSizeFromValue(ExecSize);
  };

  auto GetExecSizeFromArg = [&](II::ArgInfo AI,
                                VISA_EMask_Ctrl *ExecMask) {
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
    if (isa<Constant>(Mask) || RegAlloc->getRegForValueOrNull(KernFunc, Mask))
      *ExecMask |= NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;
    if (auto VT =
            dyn_cast<VectorType>(CI->getOperand(AI.getArgIdx())->getType()))
      ExecSize = VT->getNumElements();
    else
      ExecSize = GenXIntrinsicInfo::getOverridedExecSize(CI, Subtarget);
    return getExecSizeFromValue(ExecSize);
  };

  auto GetExecSizeFromByte = [&](II::ArgInfo AI, VISA_EMask_Ctrl *Mask) {
    ConstantInt *Const =
      dyn_cast<ConstantInt>(CI->getArgOperand(AI.getArgIdx()));
    if (!Const)
      report_fatal_error("Incorrect args to intrinsic call");
    unsigned Byte = Const->getSExtValue() & 15;
    *Mask = (VISA_EMask_Ctrl)(Byte >> 4);
    unsigned Res = Byte & 0xF;
    assert(Res <= 5 &&
        "illegal common ISA execsize (should be 1, 2, 4, 8, 16, 32).");
    return (VISA_Exec_Size)Res;
  };

  auto CreateImplicitPredication = [&](II::ArgInfo AI) {
    return createPredFromWrRegion(DstDesc);
  };

  auto CreatePredication = [&](II::ArgInfo AI) {
    return createPred(CI, BI, AI.getArgIdx());
  };

  auto GetPredicateVar = [&](II::ArgInfo AI) {
    if (AI.isRet())
      return getPredicateVar(CI);
    else
      return getPredicateVar(CI->getArgOperand(AI.getArgIdx()));
  };

  auto GetZeroedPredicateVar = [&](II::ArgInfo AI) {
    if (AI.isRet())
      return getZeroedPredicateVar(CI);
    else
      return getZeroedPredicateVar(CI->getArgOperand(AI.getArgIdx()));
  };

  auto CreateNullRawOperand = [&](II::ArgInfo AI) {
    VISA_RawOpnd *ResultOperand = nullptr;
    CISA_CALL(Kernel->CreateVISANullRawOperand(ResultOperand, false));
    return ResultOperand;
  };

  auto ProcessTwoAddr = [&](II::ArgInfo AI) {
    if (AI.getCategory() != II::TWOADDR)
      return;
    auto Reg = RegAlloc->getRegForValueOrNull(KernFunc, CI, DONTCARESIGNED);
    if (isa<UndefValue>(CI->getArgOperand(AI.getArgIdx())) && Reg &&
        isInLoop(CI->getParent()))
      addLifetimeStartInst(CI);
  };

  // Constant vector of i1 (or just scalar i1) as i32 (used in setp)
  auto ConstVi1Asi32 = [&](II::ArgInfo AI) {
    VISA_VectorOpnd *ResultOperand = nullptr;
    auto C = cast<Constant>(CI->getArgOperand(AI.getArgIdx()));
    // Get the bit value of the vXi1 constant.
    unsigned IntVal = getPredicateConstantAsInt(C);
    // unsigned i32 constant source operand
    CISA_CALL(Kernel->CreateVISAImmediate(ResultOperand, &IntVal, ISA_TYPE_UD));
    return ResultOperand;
  };

  auto CreateAddressOperand = [&](II::ArgInfo AI) {
    if (AI.isRet())
      return createAddressOperand(CI, true);
    else
      return createAddressOperand(CI->getArgOperand(AI.getArgIdx()), false);
  };

  auto GetArgCount = [&](II::ArgInfo AI) {
    auto BaseArg = AI.getArgIdx();
    MaxRawOperands = BaseArg;

    for (unsigned Idx = BaseArg; Idx < CI->getNumArgOperands(); ++Idx) {
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
    // constant byte for number of GRFs
    Value *Arg = CI;
    if (!AI.isRet())
      Arg = CI->getOperand(AI.getArgIdx());
    VectorType *VT = dyn_cast<VectorType>(Arg->getType());
    if (!VT)
      report_fatal_error("Invalid number of GRFs");
    int DataSize = VT->getNumElements() *
                   VT->getElementType()->getPrimitiveSizeInBits() / 8;
    return (uint8_t)((DataSize + (GrfByteSize - 1)) / GrfByteSize);
  };

  auto GetSampleChMask = [&](II::ArgInfo AI) {
    ConstantInt *Const =
        dyn_cast<ConstantInt>(CI->getArgOperand(AI.getArgIdx()));
    if (!Const)
      report_fatal_error("Incorrect args to intrinsic call");
    unsigned Byte = Const->getSExtValue() & 15;
    // Find the U_offset arg. It is the first vector arg after this one.
    VectorType *VT;
    for (unsigned Idx = AI.getArgIdx() + 1;
         !(VT = dyn_cast<VectorType>(CI->getOperand(Idx)->getType())); ++Idx)
      ;
    unsigned Width = VT->getNumElements();
    if (Width != 8 && Width != 16)
      report_fatal_error("Invalid execution size for load/sample");
    Byte |= Width & 16;
    return Byte;
  };

  auto GetSvmGatherBlockSize = [&](II::ArgInfo AI) {
    // svm gather/scatter "block size" field, set to reflect the element
    // type of the data
    Value *V = CI;
    if (!AI.isRet())
      V = CI->getArgOperand(AI.getArgIdx());
    unsigned ElBytes =
        V->getType()->getScalarType()->getPrimitiveSizeInBits() / 8;
    switch (ElBytes) {
      // For N = 2 byte data type, use block size 1 and block count 2.
      // Otherwise, use block size N and block count 1.
    case 2:
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
      report_fatal_error("Bad element type for SVM scatter/gather");
    }
    return ElBytes;
  };

  auto CreateOpndPredefinedSrc = [&](PreDefined_Vars RegId, unsigned ROffset,
                                     unsigned COffset, unsigned VStride,
                                     unsigned Width, unsigned HStride) {
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
    VISA_GenVar *Decl = nullptr;
    CISA_CALL(Kernel->GetPredefinedVar(Decl, RegId));
    VISA_VectorOpnd *ResultOperand = nullptr;
    CISA_CALL(Kernel->CreateVISADstOperand(ResultOperand, Decl, HStride,
                                           ROffset, COffset));
    return ResultOperand;
  };

  auto CreateImmOpndFromUInt = [&](VISA_Type ImmType, unsigned Val) {
    VISA_VectorOpnd *src = nullptr;
    CISA_CALL(Kernel->CreateVISAImmediate(src, &Val, ImmType));

    return src;
  };


  VISA_EMask_Ctrl exec_mask;
  addDebugInfo();
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

  addDebugInfo();
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
    addDebugInfo();
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
        assert(GotoJoin::isValidJoin(GotoJoin) &&
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
  assert(!isa<VectorType>(Branch->getCondition()->getType()) &&
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
  VISA_PredOpnd* PredOperand = createPredOperand(PredVar, State, Control);
  addDebugInfo();
  CISA_CALL(Kernel->AppendVISACFJmpInst(
      PredOperand, Labels[getOrCreateLabel(True, LABEL_BLOCK)]));
  // If the other successor is not the next block, write an unconditional
  // jmp to that.
  if (False == Next)
    return true; // fall through to successor
  addDebugInfo();
  CISA_CALL(Kernel->AppendVISACFJmpInst(
      nullptr, Labels[getOrCreateLabel(False, LABEL_BLOCK)]));
  return false;
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
  // A join needs a label. (If the join is at the start of its block, then
  // this gets merged into the block label.)
  addLabelInst(Join);
  // There is no join instruction in vISA -- the finalizer derives it by
  // looking for gotos targeting the basic block's label.
}

#if (LLVM_VERSION_MAJOR > 8)
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
  VectorType *VT = dyn_cast<VectorType>(UO->getType());
  if (VT != nullptr)
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

  addDebugInfo();

  if (Opcode == ISA_MOV) {
    CISA_CALL(Kernel->AppendVISADataMovementInst(
        ISA_MOV, Pred, Mod1 & MODIFIER_SAT, ExecMask, ExecSize, Dst, Src0, NULL));
    return;
  }
  report_fatal_error("buildUnaryOperator: unimplemented opcode");
}
#endif

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
  Signedness DstSigned = SIGNED;
  Signedness SrcSigned = SIGNED;
  unsigned Mod1 = 0;
  VISA_Exec_Size ExecSize = EXEC_SIZE_1;
  if (VectorType *VT = dyn_cast<VectorType>(BO->getType()))
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
    break;
  case Instruction::Shl:
    Opcode = ISA_SHL;
    IsLogic = true;
    break;
  case Instruction::AShr:
    Opcode = ISA_ASR;
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

  addDebugInfo();
  if (IsLogic) {
    CISA_CALL(Kernel->AppendVISALogicOrShiftInst(
        Opcode, Pred, Mod, ExecMask, ExecSize, Dst, Src0, Src1, NULL, NULL));
  } else {
    if (Opcode == ISA_ADDC || Opcode == ISA_SUBB) {
      CISA_CALL(Kernel->AppendVISAArithmeticInst(
          Opcode, Pred, ExecMask, ExecSize, Dst, Src0, Src1, NULL));
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
  if (VectorType *VT = dyn_cast<VectorType>(BO->getType()))
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

  VISA_PredVar *Dst = getPredicateVar(BO);
  VISA_PredVar *Src0 = getPredicateVar(BO->getOperand(0));
  VISA_PredVar *Src1 =
      Opcode != ISA_NOT ? getPredicateVar(BO->getOperand(1)) : nullptr;

  addDebugInfo();
  CISA_CALL(Kernel->AppendVISALogicOrShiftInst(
      Opcode, NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1, ExecSize, Dst, Src0,
      Src1));
}

void GenXKernelBuilder::buildSymbolInst(PtrToIntInst *ptr2Int, unsigned Mod,
                                        const DstOpndDesc &DstDesc) {
  auto GV = cast<GlobalValue>(ptr2Int->getOperand(0));
  VISA_VectorOpnd *Dst = createDestination(ptr2Int, UNSIGNED, Mod, DstDesc);
  CISA_CALL(Kernel->AppendVISACFSymbolInst(GV->getName().str(), Dst));
}

/***********************************************************************
 * buildCastInst : build code for a cast (other than a bitcast)
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
  default:
    report_fatal_error("buildCastInst: unimplemented cast");
    break;
  }

  VISA_Exec_Size ExecSize = EXEC_SIZE_1;
  if (VectorType *VT = dyn_cast<VectorType>(CI->getType()))
    ExecSize = getExecSizeFromValue(VT->getNumElements());

  auto ExecMask = getExecMaskFromWrRegion(DstDesc);

  VISA_PredOpnd *Pred = createPredFromWrRegion(DstDesc);
  // Give dest and source the same signedness for byte mov.
  VISA_VectorOpnd *Dst = createDestination(CI, OutSigned, Mod, DstDesc);

  if (InSigned == DONTCARESIGNED)
    InSigned = OutSigned;
  VISA_VectorOpnd *Src0 = createSourceOperand(CI, InSigned, 0, BI);

  addDebugInfo();
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
  assert((!DstDesc.WrRegion || Cmp->getType()->getPrimitiveSizeInBits() != 4 ||
          Cmp->getOperand(0)
                  ->getType()
                  ->getScalarType()
                  ->getPrimitiveSizeInBits() == 64) &&
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
    assert(false && "unsupported fcmp predicate");
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
    report_fatal_error("unknown predicate");
    opSpec = ISA_CMP_E;
    break;
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
  if (VectorType *VT = dyn_cast<VectorType>(Cmp->getType()))
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
    addDebugInfo();
    CISA_CALL(Kernel->AppendVISAComparisonInst(opSpec, ctrlMask, ExecSize,
                                               PredVar, Src0, Src1));
  } else {
    ctrlMask = getExecMaskFromWrRegion(DstDesc);
    Value *Val = DstDesc.WrRegion ? DstDesc.WrRegion : Cmp->user_back();
    Dst = createDestination(Val, Signed, 0, DstDesc);
    addDebugInfo();
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
  assert(!DstDesc.WrRegion);
  Value *Base = Liveness->getAddressBase(CI);
  VISA_Exec_Size ExecSize = EXEC_SIZE_1;
  VISA_EMask_Ctrl MaskCtrl = NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1;

  if (VectorType *VT = dyn_cast<VectorType>(CI->getType()))
    ExecSize = getExecSizeFromValue(VT->getNumElements());
  // If the offset is less aligned than the base register element type, then
  // we need a different type.
  Type *OverrideTy = nullptr;
  Type *BaseTy = Base->getType();
  if (BaseTy->isPointerTy())
    BaseTy = BaseTy->getPointerElementType();
  unsigned ElementBytes =
      BaseTy->getScalarType()->getPrimitiveSizeInBits() >> 3;
  int Offset = cast<ConstantInt>(CI->getArgOperand(1))->getSExtValue();
  if ((ElementBytes - 1) & Offset) {
    OverrideTy = VectorType::get(Type::getInt8Ty(CI->getContext()),
                                 BaseTy->getVectorNumElements() * ElementBytes);
    ElementBytes = 1;
  }
  Register *BaseReg =
      RegAlloc->getRegForValue(KernFunc, Base, DONTCARESIGNED, OverrideTy);

  VISA_VectorOpnd *Dst = createAddressOperand(CI, true);
  VISA_VectorOpnd *Src1 = nullptr;

  if (BaseReg->Category == RegCategory::SURFACE ||
      BaseReg->Category == RegCategory::SAMPLER) {
    uint8_t offset = Offset >> 2;
    switch (BaseReg->Category) {
    case RegCategory::SURFACE: {
      VISA_SurfaceVar *Decl = BaseReg->GetVar<VISA_SurfaceVar>(Kernel);
      unsigned int offsetB = offset * 2; // 2 is bytes size of UW
      CISA_CALL(Kernel->CreateVISAAddressOfOperand(Src1, Decl, offsetB));
      break;
    }
    case RegCategory::SAMPLER: {
      VISA_SurfaceVar *Decl = BaseReg->GetVar<VISA_SurfaceVar>(Kernel);
      unsigned int offsetB = offset * 2; // 2 is bytes size of UW
      CISA_CALL(Kernel->CreateVISAAddressOfOperand(Src1, Decl, offsetB));
      break;
    }
    default:
      report_fatal_error("Invalid state operand class: only surface, vme, and "
                         "sampler are supported.");
      break;
    }
  } else {
    uint8_t rowOffset = Offset >> genx::log2(GrfByteSize);
    uint8_t colOffset = (Offset & (GrfByteSize - 1)) >> Log2_32(ElementBytes);
    VISA_GenVar *Decl = BaseReg->GetVar<VISA_GenVar>(Kernel);
    auto TypeSize = BaseReg->Ty->getScalarType()->getPrimitiveSizeInBits() >> 3;
    unsigned int offset = colOffset * TypeSize + rowOffset * GrfByteSize;
    CISA_CALL(Kernel->CreateVISAAddressOfOperand(Src1, Decl, offset));
  }
  VISA_VectorOpnd *Src2 = createSourceOperand(CI, UNSIGNED, 0, BI);
  addDebugInfo();
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
  VISA_GenVar *Sp = nullptr;
  CISA_CALL(Kernel->GetPredefinedVar(Sp, PreDefined_Vars::PREDEFINED_FE_SP));

  VISA_VectorOpnd *SpSrc = nullptr;
  CISA_CALL(
      Kernel->CreateVISASrcOperand(SpSrc, Sp, MODIFIER_NONE, 0, 1, 0, 0, 0));

  Value *AllocaOff = CI->getOperand(0);
  Type *AllocaOffTy = AllocaOff->getType();
  unsigned OffVal = getResultedTypeSize(AllocaOffTy);

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

// extracts underlying c-string from provided constant
static StringRef extractCStr(const Constant &CStrConst) {
  if (isa<ConstantDataArray>(CStrConst))
    return cast<ConstantDataArray>(CStrConst).getAsCString();
  assert(isa<ConstantAggregateZero>(CStrConst));
  return "";
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
  LLVMContext& Context = CI->getContext();
  ImmutableCallSite CallSite(CI);
  const Value *Val = CallSite.getArgument(0);
  const Instruction *Gep = cast<Instruction>(Val);
  Val = Gep->getOperand(0);
  StringRef UnderlyingCStr =
    extractCStr(*cast<GlobalVariable>(Val)->getInitializer());

  // store metadata
  MDNode* N = MDNode::get(Context, MDString::get(Context, UnderlyingCStr));
  NMD->addOperand(N);
}

void GenXKernelBuilder::deduceRegion(Region *R, bool IsDest,
                                     unsigned MaxWidth) {
  assert(Subtarget);
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
      Region AddrR(AddrRdR, BaleInfo());
      assert(!AddrR.Indirect &&
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
  assert(Decl && "no register allocated for this value");
  if (!IsDest) {
    ResultOperand = createCisaSrcOperand(
        Decl, static_cast<VISA_Modifier>(Mod), R->VStride, R->Width, R->Stride,
        R->Offset >> genx::log2(GrfByteSize),
        (R->Offset & (GrfByteSize - 1)) / R->ElementBytes);
  } else {
    ResultOperand = createCisaDstOperand(
        Decl, R->Stride, R->Offset >> genx::log2(GrfByteSize),
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
  Register *IdxReg = RegAlloc->getRegForValue(KernFunc, Indirect, DONTCARESIGNED);
  assert(IdxReg->Category == RegCategory::ADDRESS);

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
  visa::TypeDetails TD(Func->getParent()->getDataLayout(), R->ElementTy,
                       Signed);
  unsigned VStride = R->VStride;
  if (isa<VectorType>(R->Indirect->getType()))
    // multi indirect (vector index), set vstride
    VStride = 0x8000; // field to null
  VISA_AddrVar *AddrDecl = IdxReg->GetVar<VISA_AddrVar>(Kernel);
  if (IsDest) {
    CISA_CALL(Kernel->CreateVISAIndirectDstOperand(
        ResultOperand, AddrDecl, R->IndirectAddrOffset, R->Offset, R->Stride,
        (VISA_Type)TD.VisaType));
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
  if (getLoops(BB->getParent())->getLoopFor(BB))
    return true; // inside loop in this function
  // Now we need to see if this function is called from inside a loop.
  // First check the cache.
  auto i = IsInLoopCache.find(BB->getParent());
  if (i != IsInLoopCache.end())
    return i->second;
  // Now check all call sites. This recurses as deep as the depth of the call
  // graph, which must be acyclic as GenX does not allow recursion.
  bool InLoop = false;
  for (auto ui = BB->getParent()->use_begin(), ue = BB->getParent()->use_end();
       ui != ue; ++ui) {
    auto CI = dyn_cast<CallInst>(ui->getUser());
    if (!CI)
      continue;
    assert(ui->getOperandNo() == CI->getNumArgOperands());
    if (CI->getFunction() == BB->getParent())
      continue;
    if (isInLoop(CI->getParent())) {
      InLoop = true;
      break;
    }
  }
  IsInLoopCache[BB->getParent()] = InLoop;
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
  unsigned TotalNumElements = WrRegion->getType()->getVectorNumElements();
  Instruction *ThisWr = WrRegion;
  for (;;) {
    Region R(ThisWr, BaleInfo());
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
  auto Reg = RegAlloc->getRegForValueOrNull(KernFunc, Inst);
  if (!Reg)
    return; // no register allocated such as being indirected.

  switch (Reg->Category) {
  case RegCategory::GENERAL:
    opnd = createCisaDstOperand(Reg->GetVar<VISA_GenVar>(Kernel), 1, 0, 0);
    break;
  case RegCategory::ADDRESS:
    CISA_CALL(Kernel->CreateVISAAddressDstOperand(
        opnd, Reg->GetVar<VISA_AddrVar>(Kernel), 0));
    break;
#if 0  // Not currently used.
    case RegCategory::PREDICATE:
      break;
#endif // 0
  default:
    report_fatal_error("createLifetimeStartInst: Invalid register category");
    break;
  }
  addDebugInfo();
  CISA_CALL(Kernel->AppendVISALifetime(LIFETIME_START, opnd));
}

/***********************************************************************
 * addDebugInfo : add debug infromation
 */
void GenXKernelBuilder::addDebugInfo() {
  // Ensure that the last label does not get merged with the next one now we
  // know that there is code in between.
  LastLabel = -1;
  // Check if we have a pending debug location.
  if (PendingLine) {
    // Do the source location debug info with vISA FILE and LOC instructions.
    if (PendingFilename != "" && (PendingFilename != LastFilename ||
                                  PendingDirectory != LastDirectory)) {
      SmallString<256> Filename;
      // Bodge here to detect Windows absolute path even when built on cygwin.
      if (sys::path::is_absolute(PendingFilename) ||
          (PendingFilename.size() > 2 && PendingFilename[1] == ':'))
        Filename = PendingFilename;
      else {
        Filename = PendingDirectory;
        sys::path::append(Filename, PendingFilename);
      }
      CISA_CALL(Kernel->AppendVISAMiscFileInst(Filename.c_str()));
      LastDirectory = PendingDirectory;
      LastFilename = PendingFilename;
    }
    if (PendingLine != LastLine) {
      CISA_CALL(Kernel->AppendVISAMiscLOC(PendingLine));
      LastLine = PendingLine;
      PendingLine = 0;
    }
  }
}

void GenXKernelBuilder::emitOptimizationHints() {
  if (skipOptWithLargeBlock(*FG))
    return;

  // Track rp considering byte variable widening.
  PressureTracker RP(*FG, Liveness, /*ByteWidening*/ true);
  const std::vector<genx::LiveRange *> &WidenLRs = RP.getWidenVariables();

  if (!SkipNoWiden) {
    for (auto LR : WidenLRs) {
      SimpleValue SV = *LR->value_begin();
      auto *R = RegAlloc->getRegForValueOrNull(FG->getHead(), SV);
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
void GenXKernelBuilder::addLabelInst(Value *BB) {
  // Skip this for now, because we don't know how to patch labels of branches.
  if (0) { // LastLabel >= 0) {
    // There has been no code since the last label, so use the same label
    // for this basic block.
    setLabel(BB, LastLabel);
  } else {
    // Need a new label.
    LastLabel = getOrCreateLabel(BB, LABEL_BLOCK);
    CISA_CALL(Kernel->AppendVISACFLabelInst(Labels[LastLabel]));
  }
}

/***********************************************************************
 * getOrCreateLabel : get/create label number for a Function or BasicBlock
 */
unsigned GenXKernelBuilder::getOrCreateLabel(Value *V, int Kind) {
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
      NameBuf = legalizeName(N);
      N = NameBuf;
    }
    CISA_CALL(Kernel->CreateVISALabelVar(
        Decl,
        cutString((Twine(N) + Twine("_BB_") + Twine(Labels.size())).str())
            .c_str(),
        VISA_Label_Kind(Kind)));
    Labels.push_back(Decl);
  } else if (Kind == LABEL_BLOCK) {
    CISA_CALL(Kernel->CreateVISALabelVar(
        Decl, cutString((Twine("BB_") + Twine(Labels.size())).str()).c_str(),
        VISA_Label_Kind(Kind)));
    Labels.push_back(Decl);
  } else if (Kind == LABEL_FC) {
    assert(isa<Function>(V));
    auto F = cast<Function>(V);
    StringRef N = F->getFnAttribute("CMCallable").getValueAsString();
    CISA_CALL(Kernel->CreateVISALabelVar(
        Decl, cutString(Twine(N).str()).c_str(), VISA_Label_Kind(Kind)));
    Labels.push_back(Decl);
  } else {
    StringRef N = V->getName();
    CISA_CALL(Kernel->CreateVISALabelVar(
        Decl,
        cutString(
            (Twine("_") + Twine(N) + Twine("_") + Twine(Labels.size())).str())
            .c_str(),
        VISA_Label_Kind(Kind)));
    Labels.push_back(Decl);
  }
  return Num;
}

void GenXKernelBuilder::buildInlineAsm(CallInst *CI) {
  assert(CI->isInlineAsm() && "Inline asm expected");
  InlineAsm *IA = dyn_cast<InlineAsm>(CI->getCalledValue());
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
            InlasmOp, DONTCARESIGNED, Info.getConstraintType(), 0, DstDesc);
      } else {
        // Can't deduce output operand because there are no users
        // but we have register allocated. If region is needed we can use
        // default one based one type.
        SimpleValue SV(CI, ArgNo);
        Register *Reg = RegAlloc->getRegForValue(KernFunc, SV, DONTCARESIGNED);
        Region R(SV.getType());
        InlasmOpAsString =
            createInlineAsmOperand(Reg, &R, true /*IsDst*/, DONTCARESIGNED,
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
          InlasmOp, DONTCARESIGNED, IsBaled, Info.getConstraintType());
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

void GenXKernelBuilder::buildCall(IGCLLVM::CallInst *CI,
                                  const DstOpndDesc &DstDesc) {
  LLVM_DEBUG(dbgs() << CI << "\n");
  Function *Callee = CI->getCalledFunction();

  if (!Callee || Callee->hasFnAttribute(genx::FunctionMD::CMStackCall)) {
    buildStackCall(CI, DstDesc);
    return;
  }

  unsigned LabelKind = LABEL_SUBROUTINE;
  if (Callee->hasFnAttribute("CMCallable"))
    LabelKind = LABEL_FC;
  else
    assert(FG == FG->getParent()->getGroup(Callee) &&
           "unexpected call to outside FunctionGroup");

  // Check whether the called function has a predicate arg that is EM.
  int EMOperandNum = -1;
  for (auto ai = Callee->arg_begin(), ae = Callee->arg_end(); ai != ae; ++ai) {
    auto Arg = &*ai;
    if (!Arg->getType()->getScalarType()->isIntegerTy(1))
      continue;
    if (Liveness->getLiveRange(Arg)->getCategory() == RegCategory::EM) {
      EMOperandNum = Arg->getArgNo();
      break;
    }
  }

  if (EMOperandNum < 0) {
    addDebugInfo();
    // Scalar calls must be marked with NoMask
    CISA_CALL(Kernel->AppendVISACFCallInst(
        nullptr, vISA_EMASK_M1_NM, EXEC_SIZE_1,
        Labels[getOrCreateLabel(Callee, LabelKind)]));
  } else {
    auto PredicateOpnd = NoMask ? nullptr : createPred(CI, BaleInfo(), EMOperandNum);
    addDebugInfo();
    CISA_CALL(Kernel->AppendVISACFCallInst(
        PredicateOpnd, vISA_EMASK_M1,
        getExecSizeFromValue(
            CI->getArgOperand(EMOperandNum)->getType()->getVectorNumElements()),
        Labels[getOrCreateLabel(Callee, LabelKind)]));
  }
}

void GenXKernelBuilder::buildRet(ReturnInst *RI) {
  uint32_t FloatControl = 0;
  auto F = RI->getFunction();
  F->getFnAttribute(genx::FunctionMD::CMFloatControl)
      .getValueAsString()
      .getAsInteger(0, FloatControl);
  FloatControl &= CR_Mask;
  if (FloatControl != DefaultFloatControl) {
    buildControlRegUpdate(CR_Mask, true);
    if (DefaultFloatControl)
      buildControlRegUpdate(DefaultFloatControl, false);
  }
  addDebugInfo();
  if (!isKernel(F) &&
      (F->hasFnAttribute(genx::FunctionMD::CMStackCall) ||
       F->hasFnAttribute(genx::FunctionMD::ReferencedIndirectly))) {
    CISA_CALL(Kernel->AppendVISACFFunctionRetInst(nullptr, vISA_EMASK_M1,
                                                  EXEC_SIZE_16));
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
    if (Baling->getBaleInfo(Inst).isOperandBaled(OperandNum)) {
      Instruction *RdRegion = cast<Instruction>(V);
      Region R(RdRegion, BaleInfo());
      ByteOffset = R.Offset;
      V = RdRegion->getOperand(0);
    }
    Register *Reg = RegAlloc->getRegForValue(KernFunc, V, Signed);
    assert(Reg->Category == RegCategory::GENERAL);
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
    Region R(DstDesc.WrRegion, BaleInfo());
    ByteOffset = R.Offset;
  }
  Type *OverrideType = nullptr;
  if (DstDesc.GStore) {
    V = getUnderlyingGlobalVariable(DstDesc.GStore->getOperand(1));
    assert(V && "out of sync");
    OverrideType = DstDesc.GStore->getOperand(0)->getType();
  }
  Register *Reg = RegAlloc->getRegForValueOrNull(KernFunc, V, Signed, OverrideType);
  if (!Reg) {
    // No register assigned. This happens to an unused raw result where the
    // result is marked as RAW_NULLALLOWED in GenXIntrinsics.
    CISA_CALL(Kernel->CreateVISANullRawOperand(ResultOperand, true));
  } else {
    assert(Reg->Category == RegCategory::GENERAL);
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
int GenXKernelBuilder::getLabel(Value *V) {
  std::map<Value *, unsigned>::iterator i = LabelMap.find(V);
  if (i != LabelMap.end())
    return i->second;
  return -1;
}

/***********************************************************************
 * setLabel : set the label number for a Function or BasicBlock
 */
void GenXKernelBuilder::setLabel(Value *V, unsigned Num) { LabelMap[V] = Num; }

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
  llvm_unreachable("Can't find string by index.");
}

/***********************************************************************
 * GenXKernelBuilder::getLoops : get loop info for given function, cacheing in
 *      Loops map
 */
LoopInfoBase<BasicBlock, Loop> *GenXKernelBuilder::getLoops(Function *F) {
  auto LoopsEntry = &Loops[F];
  if (!*LoopsEntry) {
    auto DT = DTs->getDomTree(F);
    *LoopsEntry = new LoopInfoBase<BasicBlock, Loop>;
    (*LoopsEntry)->analyze(*DT);
  }
  return *LoopsEntry;
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
    Result = FG->getModule()->getDataLayout().getTypeSizeInBits(T) / 8;
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
  assert(V->getType() < ISA_TYPE_NUM);
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
  auto partCopy = [&](int Sz) {
    int ByteSz = Sz * deduceByteSize(Dst, DL);
    assert(ByteSz);

    unsigned Start = SrcRowOff;
    unsigned End =
        (SrcRowOff * getGRFSize() + SrcColOff + ByteSz) / getGRFSize();

    // mov is prohibited to span across >2 GRF
    if (End - Start >= 2) {
      assert(Sz > 1);
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
    int ByteSz = Sz * BYTES_PER_OWORD;
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
      if (UseGlobalMem)
        OffVal *= BYTES_PER_OWORD;
      CISA_CALL(Kernel->CreateVISAImmediate(Imm, &OffVal, ISA_TYPE_UD));
      VISA_RawOpnd *RawSrc = nullptr;
      CISA_CALL(
          Kernel->CreateVISARawOperand(RawSrc, TmpVar->getGenVar(), RawOff));
      RawOff += Sz * BYTES_PER_OWORD;

      if (DoCopy) {
        CISA_CALL(Kernel->AppendVISADataMovementInst(ISA_MOV, nullptr, false,
                                                     vISA_EMASK_M1, EXEC_SIZE_1,
                                                     TmpOffDst, SpOpSrc1));
        if (UseGlobalMem) {
          CISA_CALL(Kernel->AppendVISASvmBlockStoreInst(
              getCisaOwordNumFromNumber(Sz), true, TmpOffSrc, RawSrc));
        } else {
          CISA_CALL(Kernel->AppendVISASurfAccessOwordLoadStoreInst(
              ISA_OWORD_ST, vISA_EMASK_M1, Dst, getCisaOwordNumFromNumber(Sz),
              TmpOffSrc, RawSrc));
        }
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
    int ByteSz = Sz * BYTES_PER_OWORD;
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
      if (UseGlobalMem)
        OffVal *= BYTES_PER_OWORD;
      CISA_CALL(Kernel->CreateVISAImmediate(Imm, &OffVal, ISA_TYPE_UD));
      PrevStackOff += Sz;
      VISA_RawOpnd *RawSrc = nullptr;
      CISA_CALL(Kernel->CreateVISARawOperand(RawSrc, TmpVar->getGenVar(), 0));

      CISA_CALL(Kernel->AppendVISAArithmeticInst(ISA_ADD, nullptr, false,
                                                 vISA_EMASK_M1, EXEC_SIZE_1,
                                                 TmpOffDst, SpOpSrc, Imm));
      if (UseGlobalMem) {
        CISA_CALL(Kernel->AppendVISASvmBlockLoadInst(
            getCisaOwordNumFromNumber(Sz), false, TmpOffSrc, RawSrc));
      } else {
        CISA_CALL(Kernel->AppendVISASurfAccessOwordLoadStoreInst(
            ISA_OWORD_LD, vISA_EMASK_M1, Src, getCisaOwordNumFromNumber(Sz),
            TmpOffSrc, RawSrc));
      }

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

/**************************************************************************************************
 * beginFunction : emit function prologue and arguments passing code
 *
 * Emit stack-related function prologue if Func is a kernel and there're
 * stackcalls or Func is a stack function.
 *
 * Prologue performs Sp and Fp initialization (both for kernel and stack
 * function). For stack functions arguments passing code is generated as well,
 * %arg and stackmem passing is supported.
 */
void GenXKernelBuilder::beginFunction(Function *Func) {
  VISA_GenVar *Sp = nullptr, *Fp = nullptr, *Hwtid = nullptr;
  CISA_CALL(Kernel->GetPredefinedVar(Sp, PREDEFINED_FE_SP));
  CISA_CALL(Kernel->GetPredefinedVar(Fp, PREDEFINED_FE_FP));
  CISA_CALL(Kernel->GetPredefinedVar(Hwtid, PREDEFINED_HW_TID));

  VISA_VectorOpnd *SpOpSrc = nullptr;
  VISA_VectorOpnd *SpOpSrc1 = nullptr;
  VISA_VectorOpnd *SpOpDst = nullptr;
  VISA_VectorOpnd *SpOpDst1 = nullptr;
  VISA_VectorOpnd *FpOpDst = nullptr;
  VISA_VectorOpnd *FpOpSrc = nullptr;
  VISA_VectorOpnd *Imm = nullptr;

  CISA_CALL(Kernel->CreateVISADstOperand(SpOpDst, Sp, 1, 0, 0));
  CISA_CALL(Kernel->CreateVISADstOperand(SpOpDst1, Sp, 1, 0, 0));
  CISA_CALL(Kernel->CreateVISADstOperand(FpOpDst, Fp, 1, 0, 0));

  CISA_CALL(
      Kernel->CreateVISASrcOperand(SpOpSrc, Sp, MODIFIER_NONE, 0, 1, 0, 0, 0));
  CISA_CALL(
      Kernel->CreateVISASrcOperand(SpOpSrc1, Sp, MODIFIER_NONE, 0, 1, 0, 0, 0));

  CISA_CALL(
      Kernel->CreateVISASrcOperand(FpOpSrc, Fp, MODIFIER_NONE, 0, 1, 0, 0, 0));

  if (isKernel(Func) && (HasStackcalls || HasAlloca)) {
    // init kernel stack
    VISA_GenVar *Hwtid = nullptr;
    CISA_CALL(Kernel->GetPredefinedVar(Hwtid, PREDEFINED_HW_TID));

    VISA_VectorOpnd *HwtidOp = nullptr;
    uint32_t Val = STACK_PER_THREAD;

    CISA_CALL(Kernel->CreateVISAImmediate(Imm, &Val, ISA_TYPE_UD));
    CISA_CALL(Kernel->CreateVISASrcOperand(HwtidOp, Hwtid, MODIFIER_NONE, 0, 1,
                                           0, 0, 0));

    if (StackSurf == PREDEFINED_SURFACE_STACK) {
      CISA_CALL(Kernel->AppendVISAArithmeticInst(
          ISA_MUL, nullptr, false, (NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1),
          EXEC_SIZE_1, SpOpDst, HwtidOp, Imm));
    } else {
      VISA_GenVar *Tmp = nullptr;
      CISA_CALL(
          Kernel->CreateVISAGenVar(Tmp, "SpOff", 1, ISA_TYPE_UQ, ALIGN_DWORD));

      VISA_VectorOpnd *OffOpDst = nullptr;
      VISA_VectorOpnd *OffOpSrc = nullptr;
      CISA_CALL(Kernel->CreateVISADstOperand(OffOpDst, Tmp, 1, 0, 0));
      CISA_CALL(Kernel->CreateVISASrcOperand(OffOpSrc, Tmp, MODIFIER_NONE, 0, 1,
                                             0, 0, 0));
      CISA_CALL(Kernel->AppendVISAArithmeticInst(
          ISA_MUL, nullptr, false, (NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1),
          EXEC_SIZE_1, OffOpDst, HwtidOp, Imm));

      VISA_VectorOpnd *OpSrc = nullptr;
      if (UseGlobalMem) {
        assert(Func->arg_size() > 0);
        Value &PrivBase = *(Func->arg_end() - 1);
        genx::KernelArgInfo AI(TheKernelMetadata.getArgKind(Func->arg_size() - 1));
        assert(AI.isPrivateBase());
        OpSrc = createSource(&PrivBase, DONTCARESIGNED);
      } else {
        VISA_GenVar *R0 = nullptr;
        CISA_CALL(Kernel->GetPredefinedVar(R0, PREDEFINED_R0));

        CISA_CALL(Kernel->CreateVISASrcOperand(OpSrc, R0, MODIFIER_NONE, 0, 1,
                                               0, 0, 5));
      }
      CISA_CALL(Kernel->AppendVISADataMovementInst(
          ISA_MOV, nullptr, false, (NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1),
          EXEC_SIZE_1, SpOpDst, OpSrc));
      Kernel->AppendVISAArithmeticInst(
          ISA_ADD, nullptr, false, (NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1),
          EXEC_SIZE_1, SpOpDst1, SpOpSrc1, OffOpSrc);
    }
    CISA_CALL(Kernel->AppendVISADataMovementInst(
        ISA_MOV, nullptr, false, (NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1),
        EXEC_SIZE_1, FpOpDst, SpOpSrc));
    // use the max available for now
    unsigned SMO = Subtarget ? Subtarget->stackSurfaceMaxSize() : 8192;
    Kernel->AddKernelAttribute("SpillMemOffset", 4, &SMO);
  } else if (Func->hasFnAttribute(genx::FunctionMD::CMStackCall) ||
             Func->hasFnAttribute(genx::FunctionMD::ReferencedIndirectly)) {
    if (Func->hasFnAttribute(genx::FunctionMD::ReferencedIndirectly)) {
      int ExtVal = 1;
      Kernel->AddKernelAttribute("Extern", 4, &ExtVal);
    }
    // stack function prologue
    VISA_GenVar *FpTmp = nullptr;

    auto *ArgVar = &CisaVars[Kernel].at("argv");
    auto *RetVar = &CisaVars[Kernel].at("retv");

    if (FPMap.count(Func) == 0) {
      CISA_CALL(
          Kernel->CreateVISAGenVar(FpTmp, "tmp", 1, ISA_TYPE_UD, ALIGN_DWORD));
      FPMap.insert(std::pair<Function *, VISA_GenVar *>(Func, FpTmp));
    } else
      FpTmp = FPMap[Func];

    // init func stack pointers
    VISA_VectorOpnd *TmpOp = nullptr;
    CISA_CALL(Kernel->CreateVISADstOperand(TmpOp, FpTmp, 1, 0, 0));

    Kernel->AppendVISADataMovementInst(
        ISA_MOV, nullptr, false, (NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1),
        EXEC_SIZE_1, TmpOp, FpOpSrc);
    Kernel->AppendVISADataMovementInst(
        ISA_MOV, nullptr, false, (NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1),
        EXEC_SIZE_1, FpOpDst, SpOpSrc);

    // unpack args
    int Sz = 0, StackOff = 0;
    unsigned RowOff = 0, ColOff = 0, SrcRowOff = 0, SrcColOff = 0;
    bool StackStarted = false;
    unsigned NoStackSize = 0;
    // NOTE: using reverse iterators for args would be much better we don't have
    // any though
    for (auto &FArg : Func->args()) {
      if (Liveness->getLiveRange(&FArg) &&
          Liveness->getLiveRange(&FArg)->getCategory() == RegCategory::EM)
        continue;

      RowOff = 0, ColOff = 0;
      unsigned ArgSize = getValueSize(FArg.getType());
      if (SrcColOff &&
          (FArg.getType()->isVectorTy() || ArgSize > (GrfByteSize - ColOff))) {
        SrcRowOff++;
        SrcColOff = 0;
        NoStackSize++;
      }
      if (Liveness->getLiveRange(&FArg)->getCategory() ==
          RegCategory::PREDICATE) {
        VISA_VectorOpnd *argSrc = nullptr;
        Kernel->CreateVISASrcOperand(
            argSrc,
            ArgVar->getAlias(llvmToVisaType(FArg.getType()), Kernel)
                ->getGenVar(),
            MODIFIER_NONE, 0, 1, 0, SrcRowOff, SrcColOff);
        auto *PReg =
            RegAlloc->getRegForValueOrNull(KernFunc, SimpleValue(&FArg));
        assert(PReg);
        Kernel->AppendVISASetP(vISA_EMASK_M1_NM, EXEC_SIZE_32,
                               PReg->GetVar<VISA_PredVar>(Kernel), argSrc);
      } else {
        if ((int)ArgVar->getByteSize() - SrcRowOff * GrfByteSize >= ArgSize &&
            !StackStarted) {
          emitVectorCopy(&FArg, ArgVar->getAlias(&FArg, Kernel), RowOff, ColOff,
                         SrcRowOff, SrcColOff, getValueSize(&FArg));
          NoStackSize = RowOff;
        } else {
          StackStarted = true;
          VISA_StateOpndHandle *stackSurf = nullptr;
          VISA_SurfaceVar *stackSurfVar = nullptr;
          CISA_CALL(Kernel->GetPredefinedSurface(stackSurfVar, StackSurf));
          CISA_CALL(
              Kernel->CreateVISAStateOperandHandle(stackSurf, stackSurfVar));
          popStackArg(&FArg, stackSurf, ArgSize, RowOff, ColOff, SrcRowOff,
                      SrcColOff, StackOff);
        }
      }
      Sz += ArgSize;
    }
    if (!StackStarted && ColOff)
      NoStackSize++;
    auto *StackCallee = Func2Kern[Func];
    auto *FuncTy = Func->getFunctionType();
    int RetSize =
        (FuncTy->getReturnType()->isVoidTy() ||
         getValueSize(FuncTy->getReturnType()) > RetVar->getByteSize())
            ? 0
            : (getValueSize(FuncTy->getReturnType()) + GrfByteSize - 1) /
                  GrfByteSize;

    StackCallee->SetFunctionInputSize(NoStackSize);
    StackCallee->SetFunctionReturnSize(RetSize);
    StackCallee->AddKernelAttribute("ArgSize", 1, &NoStackSize);
    StackCallee->AddKernelAttribute("RetValSize", 1, &RetSize);
  }
}

/**************************************************************************************************
 * endFunction : emit function epilogue and return value passing code
 *
 * Emit stack-related function epilogue if Func is a stack function.
 *
 * Epilogue restores Sp and Fp. Return value may be passed either visa %retval
 * arg or stackmem, both scalar/vector and aggregate types are supported (please
 * also see build[Extract|Insert]Value).
 */
void GenXKernelBuilder::endFunction(Function *Func, ReturnInst *RI) {
  if (!isKernel(Func) &&
      (Func->hasFnAttribute(genx::FunctionMD::CMStackCall) ||
       Func->hasFnAttribute(genx::FunctionMD::ReferencedIndirectly))) {
    VISA_GenVar *Sp = nullptr, *Fp = nullptr;
    CISA_CALL(Kernel->GetPredefinedVar(Sp, PREDEFINED_FE_SP));
    CISA_CALL(Kernel->GetPredefinedVar(Fp, PREDEFINED_FE_FP));

    VISA_VectorOpnd *SpOpSrc = nullptr;
    VISA_VectorOpnd *SpOpDst = nullptr;
    VISA_VectorOpnd *FpOpDst = nullptr;
    VISA_VectorOpnd *FpOpSrc = nullptr;

    CISA_CALL(Kernel->CreateVISADstOperand(SpOpDst, Sp, 1, 0, 0));
    CISA_CALL(Kernel->CreateVISADstOperand(FpOpDst, Fp, 1, 0, 0));
    CISA_CALL(Kernel->CreateVISASrcOperand(SpOpSrc, Sp, MODIFIER_NONE, 0, 1,
                                           0, 0, 0));
    CISA_CALL(Kernel->CreateVISASrcOperand(FpOpSrc, Fp, MODIFIER_NONE, 0, 1,
                                           0, 0, 0));

    VISA_VectorOpnd *TmpOp = nullptr;
    CISA_CALL(Kernel->CreateVISASrcOperand(TmpOp, FPMap[Func], MODIFIER_NONE,
                                           0, 1, 0, 0, 0));

    Kernel->AppendVISADataMovementInst(
        ISA_MOV, nullptr, false, (NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1),
        EXEC_SIZE_1, SpOpDst, FpOpSrc);
    Kernel->AppendVISADataMovementInst(
        ISA_MOV, nullptr, false, (NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1),
        EXEC_SIZE_1, FpOpDst, TmpOp);

    VISA_GenVar *Ret = nullptr;
    CISA_CALL(Kernel->GetPredefinedVar(Ret, PREDEFINED_RET));

    if (!Func->getReturnType()->isVoidTy() &&
        !Func->getReturnType()->isAggregateType() &&
        Liveness->getLiveRange(RI->getReturnValue()) &&
        (Liveness->getLiveRange(RI->getReturnValue())->getCategory() !=
             RegCategory::EM &&
         Liveness->getLiveRange(RI->getReturnValue())->getCategory() !=
             RegCategory::PREDICATE)) {
      GenericCisaVariable *RetVar = &CisaVars[Kernel].at("retv");
      assert(!Func->getReturnType()->isAggregateType());

      // pack retval
      unsigned RowOff = 0, ColOff = 0, SrcRowOff = 0, SrcColOff = 0;
      if (getValueSize(Func->getReturnType()) <=
          RetVar->getByteSize()) {
        unsigned RowOff = 0, ColOff = 0, SrcRowOff = 0, SrcColOff = 0;
        emitVectorCopy(RetVar->getAlias(RI->getReturnValue(), Kernel), RI->getReturnValue(),
          RowOff, ColOff, SrcRowOff,
                       SrcColOff, getValueSize(RI->getReturnValue()));
      } else {
        VISA_StateOpndHandle *StackSurfOp = nullptr;
        VISA_SurfaceVar *StackSurfVar = nullptr;
        CISA_CALL(Kernel->GetPredefinedSurface(StackSurfVar,
                                               StackSurf));
        CISA_CALL(
            Kernel->CreateVISAStateOperandHandle(StackSurfOp, StackSurfVar));
        pushStackArg(StackSurfOp, RI->getReturnValue(),
                     getValueSize(Func->getReturnType()), RowOff, ColOff,
                     SrcRowOff, SrcColOff);
      }
    }
    for (auto II : RetvInserts)
      buildInsertRetv(II);
    RetvInserts.clear();
  }
}

void GenXKernelBuilder::buildExtractRetv(ExtractValueInst *Inst) {
  auto T = Inst->getOperand(0)->getType();
  auto *RetVar = &CisaVars[Kernel].at("retv");

  bool UseStack = getValueSize(T) > RetVar->getByteSize();

  auto Index = Inst->getIndices().front();
  if (T->getContainedType(Index)->isVectorTy() &&
      T->getContainedType(Index)->getVectorElementType()->isIntegerTy(1))
    // elements of <N x i1> type should be ignored
    return;

  unsigned RowOff = 0, ColOff = 0;
  unsigned SrcRowOff = 0, SrcColOff = 0;
  for (unsigned i = 0; i < Index; i++) {
    int Mod = UseStack ? BYTES_PER_OWORD : GrfByteSize;
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
      T->getContainedType(Index)->getVectorElementType()->isIntegerTy(1)) {
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

void GenXKernelBuilder::buildStackCall(IGCLLVM::CallInst *CI,
                                       const DstOpndDesc &DstDesc) {
  LLVM_DEBUG(dbgs() << "Build stack call\n"; CI->print(dbgs()); dbgs() << "\n");
  Function *Callee = CI->getCalledFunction();
  auto *FuncTy = CI->getFunctionType();
  auto *StackCallee = Func2Kern[Callee];
  assert(CI->isIndirectCall() || StackCallee);

  // Check whether the called function has a predicate arg that is EM.
  int EMOperandNum = -1, EMIdx = -1;
  for (auto &Arg : CI->arg_operands()) {
    ++EMIdx;
    if (!Arg->getType()->getScalarType()->isIntegerTy(1))
      continue;
    if (Liveness->getLiveRange(Arg)->getCategory() == RegCategory::EM) {
      EMOperandNum = EMIdx;
      break;
    }
  }

  int TotalArgSize = 0;
  for (auto &CallArg : CI->arg_operands())
    TotalArgSize += getValueSize(CallArg->getType());

  VISA_GenVar *Sp = nullptr, *Arg = nullptr, *Ret = nullptr;
  CISA_CALL(Kernel->GetPredefinedVar(Sp, PREDEFINED_FE_SP));
  CISA_CALL(Kernel->GetPredefinedVar(Arg, PREDEFINED_ARG));
  CISA_CALL(Kernel->GetPredefinedVar(Ret, PREDEFINED_RET));

  unsigned ColOff = 0, RowOff = 0, SrcRowOff = 0, SrcColOff = 0;

  int Sz = 0, NoStackSize = 0, StackArgSz = 0;
  uint64_t StackOff = 0;
  bool StackStarted = false;
  // pack arguments
  for (auto &CallArg : CI->arg_operands()) {
    auto *CallArgLR = Liveness->getLiveRangeOrNull(CallArg.get());
    if (CallArgLR && CallArgLR->getCategory() == RegCategory::EM)
      continue;

    assert(!CallArg->getType()->isAggregateType());
    SrcRowOff = 0, SrcColOff = 0;
    unsigned ArgSize = getValueSize(CallArg->getType());

    if (ColOff && (CallArg->getType()->isVectorTy() ||
                   ArgSize > (GrfByteSize - ColOff))) {
      RowOff++;
      ColOff = 0;
      // adjust size if we use only a part the last used GRF
      NoStackSize++;
    }

    bool IsUndef = isa<UndefValue>(CallArg);
    auto *ArgVar = &CisaVars[Kernel].at("argv");
    if ((int)ArgVar->getByteSize() - RowOff * GrfByteSize >= ArgSize &&
        !StackStarted) {
      assert(ArgSize <= Sz - ArgVar->getByteSize() &&
             "cannot pass arg via stack and %arg as well");

      SrcRowOff = 0, SrcColOff = 0;
      if (!IsUndef && CallArgLR->getCategory() == RegCategory::PREDICATE) {
        VISA_VectorOpnd *PredDst = nullptr;
        Kernel->CreateVISADstOperand(
            PredDst,
            ArgVar->getAlias(llvmToVisaType(CallArg->getType()), Kernel)
                ->getGenVar(),
            1, RowOff, ColOff);
        auto PReg =
            RegAlloc->getRegForValueOrNull(KernFunc, SimpleValue(CallArg));
        assert(PReg);
        Kernel->AppendVISAPredicateMove(PredDst,
                                        PReg->GetVar<VISA_PredVar>(Kernel));
        ColOff += ArgSize;
      } else
        emitVectorCopy<CisaVariable, Value>(
            ArgVar->getAlias(CallArg, Kernel), CallArg, RowOff, ColOff,
            SrcRowOff, SrcColOff, getValueSize(CallArg), !IsUndef);
      Sz += ArgSize;
      NoStackSize = RowOff;
    } else {
      StackStarted = true;
      RowOff = ColOff = 0;
      SrcRowOff = SrcColOff = 0;
      VISA_StateOpndHandle *StackSurfOp = nullptr;
      VISA_SurfaceVar *StackSurfVar = nullptr;
      CISA_CALL(
          Kernel->GetPredefinedSurface(StackSurfVar, StackSurf));
      CISA_CALL(Kernel->CreateVISAStateOperandHandle(StackSurfOp, StackSurfVar));
      pushStackArg(StackSurfOp, CallArg, ArgSize, RowOff, ColOff, SrcRowOff,
                   SrcColOff, !IsUndef);

      StackArgSz += (ArgSize / BYTES_PER_OWORD) + (ArgSize % BYTES_PER_OWORD ? 1 : 0);
      StackOff = -StackArgSz;
    }
  }
  if (!StackStarted && ColOff)
    NoStackSize++;

  VISA_VectorOpnd *SpOpSrc = nullptr, *SpOpDst = nullptr, *Imm = nullptr;
  if (StackOff) {
    CISA_CALL(Kernel->CreateVISADstOperand(SpOpDst, Sp, 1, 0, 0));
    CISA_CALL(Kernel->CreateVISASrcOperand(SpOpSrc, Sp, MODIFIER_NONE, 0, 1, 0,
                                           0, 0));

    if (UseGlobalMem)
      StackOff *= BYTES_PER_OWORD;
    CISA_CALL(Kernel->CreateVISAImmediate(Imm, &StackOff, ISA_TYPE_UQ));
    CISA_CALL(Kernel->AppendVISAArithmeticInst(
        ISA_ADD, nullptr, false, (NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1),
        EXEC_SIZE_1, SpOpDst, SpOpSrc, Imm));
  }

  VISA_PredOpnd *Pred = nullptr;
  VISA_Exec_Size Esz = EXEC_SIZE_16;
  if (EMOperandNum >= 0) {
    Pred = createPred(CI, BaleInfo(), EMOperandNum);
    Esz = getExecSizeFromValue(
        CI->getArgOperand(EMOperandNum)->getType()->getVectorNumElements());
  }
  addDebugInfo();

  auto *RetVar = &CisaVars[Kernel].at("retv");
  bool ProcessRet =
      !FuncTy->getReturnType()->isVoidTy() &&
      !FuncTy->getReturnType()->isAggregateType() &&
      !(FuncTy->getReturnType()->isVectorTy() &&
        FuncTy->getReturnType()->getVectorElementType()->isIntegerTy(1));

  // cannot use processRet here since aggr/em args should be co
  int RetSize =
      (FuncTy->getReturnType()->isVoidTy() ||
       getValueSize(FuncTy->getReturnType()) > RetVar->getByteSize())
          ? 0
          : (getValueSize(FuncTy->getReturnType()) + GrfByteSize - 1) /
                GrfByteSize;
  if (Callee) {
    CISA_CALL(Kernel->AppendVISACFFunctionCallInst(
        Pred, (NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1), EXEC_SIZE_16,
        Callee->getName(), NoStackSize, RetSize));
  } else {
    auto *FuncAddr = createSource(CI->getCalledValue(), DONTCARESIGNED);
    assert(FuncAddr);
    CISA_CALL(Kernel->AppendVISACFIndirectFuncCallInst(
        Pred, (NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1), EXEC_SIZE_16,
        FuncAddr, NoStackSize, RetSize));
  }

  unsigned StackRetSz = 0;
  if (!FuncTy->getReturnType()->isVoidTy() &&
      getValueSize(FuncTy->getReturnType()) > RetVar->getByteSize())
    StackRetSz = (getValueSize(FuncTy->getReturnType(), BYTES_PER_OWORD) / BYTES_PER_OWORD +
                  ((getValueSize(FuncTy->getReturnType(), BYTES_PER_OWORD) % BYTES_PER_OWORD) ? 1 : 0));
  // unpack retval
  if (ProcessRet && Liveness->getLiveRange(CI) &&
      Liveness->getLiveRange(CI)->getCategory() != RegCategory::EM) {
    unsigned RowOff = 0, ColOff = 0, SrcRowOff = 0, SrcColOff = 0;
    if (getValueSize(FuncTy->getReturnType()) <= RetVar->getByteSize()) {
      emitVectorCopy(CI, RetVar->getAlias(CI, Kernel), RowOff, ColOff,
                     SrcRowOff, SrcColOff, getValueSize(CI));
    } else {
      int StackOffVal = -StackRetSz;
      VISA_StateOpndHandle *StackSurfOp = nullptr;
      VISA_SurfaceVar *StackSurfVar = nullptr;
      CISA_CALL(
          Kernel->GetPredefinedSurface(StackSurfVar, StackSurf));
      CISA_CALL(Kernel->CreateVISAStateOperandHandle(StackSurfOp, StackSurfVar));
      popStackArg(CI, StackSurfOp, getValueSize(Callee->getReturnType()), RowOff,
                  ColOff, SrcRowOff, SrcColOff, StackOffVal);
    }
  }
  // restore Sp
  CISA_CALL(
      Kernel->CreateVISASrcOperand(SpOpSrc, Sp, MODIFIER_NONE, 0, 1, 0, 0, 0));
  CISA_CALL(Kernel->CreateVISADstOperand(SpOpDst, Sp, 1, 0, 0));
  uint64_t OffVal = -StackRetSz;
  CISA_CALL(Kernel->CreateVISAImmediate(Imm, &OffVal, ISA_TYPE_UQ));
  CISA_CALL(Kernel->AppendVISAArithmeticInst(
      ISA_ADD, nullptr, false, (NoMask ? vISA_EMASK_M1_NM : vISA_EMASK_M1),
      EXEC_SIZE_1, SpOpDst, SpOpSrc, Imm));
}


namespace {

class GenXFinalizer : public ModulePass {
  raw_pwrite_stream &Out;
  LLVMContext *Ctx = nullptr;

public:
  static char ID;
  explicit GenXFinalizer(raw_pwrite_stream &o) : ModulePass(ID), Out(o) {}

  virtual StringRef getPassName() const { return "GenX Finalizer"; }

  LLVMContext &getContext() {
    assert(Ctx);
    return *Ctx;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<GenXModule>();
    AU.addRequired<FunctionGroupAnalysis>();
    AU.addRequired<GenXSubtargetPass>();
    AU.setPreservesAll();
  }

  void fillOCLRuntimeInfo(GenXOCLRuntimeInfo &Info, GenXModule &GM,
                          FunctionGroupAnalysis &FGA, const GenXSubtarget &ST);

  bool runOnModule(Module &M) {
    Ctx = &M.getContext();

    GenXModule &GM = getAnalysis<GenXModule>();
    FunctionGroupAnalysis &FGA = getAnalysis<FunctionGroupAnalysis>();
    GenXOCLRuntimeInfo *OCLInfo = getAnalysisIfAvailable<GenXOCLRuntimeInfo>();
    const GenXSubtarget &ST = *getAnalysis<GenXSubtargetPass>().getSubtarget();

    std::stringstream ss;
    auto *CisaBuilder = GM.GetCisaBuilder();
    if (GM.HasInlineAsm()) {
      auto VISAAsmTextReader = GM.GetVISAAsmReader();
      CISA_CALL(VISAAsmTextReader->Compile("genxir", &ss, EmitVisa));
    } else
      CISA_CALL(CisaBuilder->Compile("genxir", &ss, EmitVisa));
    if (OCLInfo)
      fillOCLRuntimeInfo(*OCLInfo, GM, FGA, ST);
    dbgs() << CisaBuilder->GetCriticalMsg();
    GM.DestroyCISABuilder();
    GM.DestroyVISAAsmReader();
    Out << ss.str();
    return false;
  }
};
} // end anonymous namespace.

char GenXFinalizer::ID = 0;

ModulePass *llvm::createGenXFinalizerPass(raw_pwrite_stream &o) {
  return new GenXFinalizer(o);
}

static void constructSymbolTable(FunctionGroup &FG, GenXModule &GM,
                                 void *&Buffer, unsigned &ByteSize,
                                 unsigned &NumEntries) {
  NumEntries = std::count_if(FG.begin(), FG.end(), [](Function *F) {
    return F->hasFnAttribute("referenced-indirectly");
  });
  ByteSize = NumEntries * sizeof(vISA::GenSymEntry);
  // this will be eventually freed in AdaptorOCL
  Buffer = new vISA::GenSymEntry[NumEntries];
  auto *Entry = static_cast<vISA::GenSymEntry *>(Buffer);
  for (auto &F : FG)
    if (F->hasFnAttribute("referenced-indirectly")) {
      assert(F->getName().size() <= vISA::MAX_SYMBOL_NAME_LENGTH);
      strcpy_s(Entry->s_name, vISA::MAX_SYMBOL_NAME_LENGTH,
               F->getName().str().c_str());
      VISAFunction *Func = static_cast<VISAFunction *>(GM.getVISAKernel(F));
      Entry->s_type = vISA::GenSymType::S_FUNC;
      Entry->s_offset = Func->getGenOffset();
      Entry->s_size = Func->getGenSize();
      Entry++;
    }
}

void GenXFinalizer::fillOCLRuntimeInfo(GenXOCLRuntimeInfo &OCLInfo,
                                       GenXModule &GM,
                                       FunctionGroupAnalysis &FGA,
                                       const GenXSubtarget &ST) {
  using KernelInfo = GenXOCLRuntimeInfo::KernelInfo;
  using CompiledKernel = GenXOCLRuntimeInfo::CompiledKernel;
  using TableInfo = GenXOCLRuntimeInfo::TableInfo;
  for (auto *FG : FGA) {
    // Compiler info.
    KernelInfo Info{*FG, ST};

    // Finalizer info (jitter struct and gen binary).
    VISAKernel *BuiltKernel = GM.getVISAKernel(FG->getHead());
    assert(BuiltKernel);
    FINALIZER_INFO *JitInfo = nullptr;
    BuiltKernel->GetJitInfo(JitInfo);
    assert(JitInfo && "Jit info is not set by finalizer");
    void *GenBin = nullptr;
    int GenBinSize = 0; // Finalizer uses signed int for size...
    BuiltKernel->GetGenxBinary(GenBin, GenBinSize);
    assert(GenBin && GenBinSize &&
           "Unexpected null buffer or zero-sized kernel (compilation failed?)");
    TableInfo &RTable = Info.getRelocationTable();
    CISA_CALL(BuiltKernel->GetGenRelocEntryBuffer(RTable.Buffer, RTable.Size, RTable.Entries));
    TableInfo &STable = Info.getSymbolTable();
    constructSymbolTable(*FG, GM, STable.Buffer, STable.Size, STable.Entries);

    // Save it all here.
    CompiledKernel FullInfo{std::move(Info), *JitInfo,
                            ArrayRef<char>{static_cast<char *>(GenBin),
                                           static_cast<size_t>(GenBinSize)}};
    OCLInfo.saveCompiledKernel(std::move(FullInfo));

    freeBlock(GenBin);
  }
}

void GenXModule::clearFinalizerArgs(std::vector<const char*>& Owner) const {
  std::for_each(Owner.begin(), Owner.end(), [](const char* a) { delete []a; });
  Owner.clear();
}

void GenXModule::collectFinalizerArgs(std::vector<const char*> &Owner) const {
  clearFinalizerArgs(Owner);

  auto grantArgument = [](const std::string& ArgString,
                          std::vector<const char*> &Owner) {
    const size_t BufferSize = ArgString.size() + 1;
    char* ArgCopyBuff = new char [BufferSize];
    std::copy(ArgString.data(), ArgString.data() + BufferSize, ArgCopyBuff);
    Owner.push_back(ArgCopyBuff);
  };

  grantArgument("-dumpvisa", Owner);
  for (const auto& Fos: FinalizerOpts) {
    // Add additional arguments if specified
    std::istringstream f(Fos);
    std::string s;
    while (getline(f, s, ' ')) {
      grantArgument(s, Owner);
    }
  }
  Owner.push_back(nullptr);
}

LLVMContext &GenXModule::getContext() {
  assert(Ctx);
  return *Ctx;
}

void GenXModule::InitCISABuilder() {
  assert(ST);
  auto Platform = ST->getVisaPlatform();
  // Use SKL for unknown platforms
  if (Platform == GENX_NONE)
    Platform = GENX_SKL;

  // Prepare array of arguments for Builder API.
  collectFinalizerArgs(CISA_Args);

  if (PrintFinalizerOptions.getValue()) {
    outs() << "Finalizer Parameters:\n\t" << " -platform " << ST->getCPU();
    std::for_each(CISA_Args.begin(), CISA_Args.end(),
                  [](const char* Arg) { outs() << " " << Arg; });
    outs() << "\n";
  }

  CISA_CALL(CreateVISABuilder(CisaBuilder,
                              HasInlineAsm() ? vISA_ASM_WRITER : vISA_MEDIA,
                              EmitVisa ? VISA_BUILDER_VISA : VISA_BUILDER_BOTH,
                              Platform, CISA_Args.size() - 1, CISA_Args.data(),
                              WaTable));
  assert(CisaBuilder && "Failed to create VISABuilder!");
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
  assert(ST);
  auto Platform = ST->getVisaPlatform();
  // Use SKL for unknown platforms
  if (Platform == GENX_NONE)
    Platform = GENX_SKL;

  // Prepare array of arguments for Builder API.
  collectFinalizerArgs(VISA_Args);

  // Prepare array of arguments for Builder API.
  if (PrintFinalizerOptions.getValue()) {
    outs() << "Finalizer Parameters:\n\t" << " -platform " << ST->getCPU();
    std::for_each(VISA_Args.begin(), VISA_Args.end(),
                  [](const char* Arg) { outs() << " " << Arg; });
    outs() << "\n";
  }

  CISA_CALL(CreateVISABuilder(VISAAsmTextReader, vISA_ASM_READER,
                              VISA_BUILDER_BOTH, Platform,
                              VISA_Args.size() - 1, VISA_Args.data(),
                              WaTable));
  assert(VISAAsmTextReader && "Failed to create VISAAsmTextReader!");
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
