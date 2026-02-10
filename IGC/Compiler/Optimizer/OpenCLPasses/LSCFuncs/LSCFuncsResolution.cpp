/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/LSCFuncs/LSCFuncsResolution.hpp"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Regex.h>
#include "common/LLVMWarningsPop.hpp"
#include "visa_igc_common_header.h"
#include <limits>
#include <string>
#include "Probe/Assertion.h"

#include <algorithm>
#include <sstream>

using namespace llvm;
using namespace IGC;

namespace {
struct LscTypeInfo {
  LSC_DATA_SIZE dataSize;
  LSC_DATA_ELEMS vectorSize;
  int sizeOfType; // e.g. float4 => sizeof(float4) for D32 V4
};

/// @brief  LSCFuncsTranslation pass : tranlate lsc builtin (__builtin_IB_*lsc*) into igc intrinsic.
///
/// This is not automated like the usual builtins because we have to do type
/// inference and do extra sanity checking here on inputs.
class LSCFuncsResolution : public FunctionPass, public InstVisitor<LSCFuncsResolution> {
public:
  // Pass identification, replacement for typeid
  static char ID;

  LSCFuncsResolution();

  /// @brief  Provides name of pass
  virtual StringRef getPassName() const override { return "LSCFuncsResolution"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
  }

  virtual bool runOnFunction(Function &F) override;

  void visitCallInst(CallInst &CI);

private:
  /// LSC Load intrinsics call method
  Instruction *CreateLSCLoadIntrinsicCallInst(GenISAIntrinsic::ID op, bool isLocalMem);
  Instruction *CreateLSCLoadCmaskIntrinsicCallInst(bool isLocalMem);

  /// LSC Store intrinsics call method
  Instruction *CreateLSCStoreIntrinsicCallInst(GenISAIntrinsic::ID op, bool isLocalMem);
  Instruction *CreateLSCStoreCmaskIntrinsicCallInst(bool isLocalMem);

  Instruction *CreateLSCSimdBlockPrefetchIntrinsicCallInst(llvm::StringRef funcName);

  /// LSC Prefetch and load status intrinsics
  Instruction *CreateLSCLoadStatusPreftchIntrinsicCallInst(GenISAIntrinsic::ID prefetchOp);

  /// LSC block 2d with address payload as a single argument
  Instruction *CreateLSC2DBlockAddressPayload(CallInst &CI);
  Instruction *CopyLSC2DBlockAddressPayload(CallInst &CI);
  Instruction *SetLSC2DBlockAddressPayloadField(CallInst &CI, LSC2DBlockField Field, bool IsAddend);
  Instruction *CreateSubGroup2DBlockOperationAP(CallInst &CI, StringRef funcName, bool isRead);

  /// LSC subgroup 2d block read/write intrinsics
  Instruction *CreateSubGroup2DBlockOperation(llvm::CallInst &CI, llvm::StringRef funcName, bool isRead);

  /// LSC Fence intrinsics call method
  Instruction *CreateLSCFenceIntrinsicCallInst(CallInst &CI);

  Instruction *CreateLSCFenceEvictToMemory();

  /// LSC Atomic intrinsics call method
  Instruction *CreateLSCAtomicIntrinsicCallInst(bool isLocalMem);

  ///////////////////////////////////////////////////////////////////////
  /// Helpers
  ///////////////////////////////////////////////////////////////////////

  /// Decode the data size and vector size from the function name.
  /// Return true if sucessful; false otherwise.
  ///   Suffix's format:  <DS>_<VS>
  ///   DS - dataSize: uchar,ushort,uint,ulong
  ///   VS - vectorSize: <2|3|4|8|16|32|64>
  ///
  LscTypeInfo decodeTypeInfoFromName();

  /// Decode the SFID from the function name.
  /// Return true if sucessful; false otherwise.
  ///     Suffix's format:  <MP>
  ///     MP - memport: ugm,ugml,tgm,slm
  LSC_SFID decodeSfidFromName();

  /// Decode the atomic op from the function name.
  /// Return true if sucessful; false otherwise.
  ///     Suffix's format:  <AOP>
  ///     AOP - atomic operation: FP64 add, FP64 sub
  AtomicOp decodeAtomicOpFromName();

  /// obnoxious that we can't use std::pair or std::tuple and constexpr
  /// (something about compiler toolchain support made use elminate this
  /// in the past)
  struct SymbolMapping {
    const char *symbol;
    int value;
  };

  /// Searches a table of mappings
  template <typename T, int N> bool findFirstInfixMapping(StringRef FN, const SymbolMapping enums[N], T &value) {
    for (int i = 0; i < N && enums[i].symbol; i++)
      if (FN.find(enums[i].symbol) != StringRef::npos) {
        value = static_cast<T>(enums[i].value);
        return true;
      }
    return false;
  }

  //// Gets an i32 with a given value
  Constant *getConstantInt32(int value) {
    Type *i32 = Type::getInt32Ty(m_pCurrInst->getContext());
    return ConstantInt::get(i32, value, true);
  }

  /// E.g. for cache controls, fence options, etc
  Constant *getImmediateEnum(int i, int lo, int hi);

  ///
  /// Fetches and validates the immediate element offset.
  /// Ensures the element offset is immediate and fits in 32b
  // (after scaling by type)
  Constant *getImmediateElementOffset(int ix, LscTypeInfo ti);

  /// Gets an operand as cache control options and sanity checks it.
  /// Atomics have some special constraints.
  Constant *getCacheControlOpts(int i, bool isAtomic = false);

  /// Reports an error in translating the intrinsic
  void reportError(const char *what);

  /// Someone called reportError on the current instruction
  bool hasError() const {
    // ick: tellp is not const
    return m_ErrorMsg.rdbuf() && m_ErrorMsg.rdbuf()->in_avail() > 0;
  }

  /// Indicates if the pass changed the processed function
  bool m_changed{};
  bool isHalfSimdMode{};

  /// state valid under visitCallInst(...)
  std::stringstream m_ErrorMsg;
  CodeGenContext *m_pCtx = nullptr;
  CallInst *m_pCurrInst = nullptr;
  Function *m_pCurrInstFunc = nullptr;
  std::set<Instruction *> m_instsToErase{};

  // For verifying address payload for block 2d read/write.
  llvm::SmallVector<Instruction *, 32> m_lsc2dblock_readwrite;
  void verifyBlock2DAddressPayload();

  static const StringRef PREFIX_LSC_STORE_local;
  static const StringRef PREFIX_LSC_STORE_global;
  static const StringRef PREFIX_LSC_STORE_BLOCK_global;

  static const StringRef PREFIX_LSC_STORE_CMASK_local;
  static const StringRef PREFIX_LSC_STORE_CMASK_global;
  static const StringRef PREFIX_LSC_LOAD_local;
  static const StringRef PREFIX_LSC_LOAD_global;
  static const StringRef PREFIX_LSC_LOAD_BLOCK_global;
  static const StringRef PREFIX_LSC_LOAD_status;

  static const StringRef PREFIX_SUBGROUP_BLOCK_READ_AP;
  static const StringRef PREFIX_SUBGROUP_BLOCK_WRITE_AP;
  static const StringRef PREFIX_SUBGROUP_BLOCK_READ;
  static const StringRef PREFIX_SUBGROUP_BLOCK_WRITE;

  static const StringRef PREFIX_LSC_LOAD_CMASK_local;
  static const StringRef PREFIX_LSC_LOAD_CMASK_global;
  static const StringRef PREFIX_LSC_FENCE;
  static const StringRef PREFIX_LSC_FENCE_EVICT_TO_MEMORY;
  static const StringRef PREFIX_LSC_ATOMIC;
  static const StringRef PREFIX_LSC_PREFETCH;
  static const StringRef PREFIX_LSC_SIMD_BLOCK_PREFETCH;
};
} // namespace

char LSCFuncsResolution::ID = 0;

const StringRef LSCFuncsResolution::PREFIX_LSC_STORE_local = "__builtin_IB_lsc_store_local_";
const StringRef LSCFuncsResolution::PREFIX_LSC_STORE_global = "__builtin_IB_lsc_store_global_";
const StringRef LSCFuncsResolution::PREFIX_LSC_STORE_BLOCK_global = "__builtin_IB_lsc_store_block_global_";

const StringRef LSCFuncsResolution::PREFIX_LSC_STORE_CMASK_local = "__builtin_IB_lsc_store_cmask_local_";
const StringRef LSCFuncsResolution::PREFIX_LSC_STORE_CMASK_global = "__builtin_IB_lsc_store_cmask_global_";
const StringRef LSCFuncsResolution::PREFIX_LSC_LOAD_local = "__builtin_IB_lsc_load_local_";
const StringRef LSCFuncsResolution::PREFIX_LSC_LOAD_global = "__builtin_IB_lsc_load_global_";
const StringRef LSCFuncsResolution::PREFIX_LSC_LOAD_BLOCK_global = "__builtin_IB_lsc_load_block_global_";
const StringRef LSCFuncsResolution::PREFIX_LSC_LOAD_status = "__builtin_IB_lsc_load_status_global_";

// Suffix _AP : builtin with address payload as a single argument
const StringRef LSCFuncsResolution::PREFIX_SUBGROUP_BLOCK_READ_AP = "__builtin_IB_subgroup_block_read_ap";
const StringRef LSCFuncsResolution::PREFIX_SUBGROUP_BLOCK_WRITE_AP = "__builtin_IB_subgroup_block_write_ap";
const StringRef LSCFuncsResolution::PREFIX_SUBGROUP_BLOCK_READ = "__builtin_IB_subgroup_block_read";
const StringRef LSCFuncsResolution::PREFIX_SUBGROUP_BLOCK_WRITE = "__builtin_IB_subgroup_block_write";

const StringRef LSCFuncsResolution::PREFIX_LSC_LOAD_CMASK_local = "__builtin_IB_lsc_load_cmask_local_";
const StringRef LSCFuncsResolution::PREFIX_LSC_LOAD_CMASK_global = "__builtin_IB_lsc_load_cmask_global_";
const StringRef LSCFuncsResolution::PREFIX_LSC_FENCE = "__builtin_IB_lsc_fence_";
const StringRef LSCFuncsResolution::PREFIX_LSC_FENCE_EVICT_TO_MEMORY = "__builtin_IB_lsc_fence_evict_to_memory";
const StringRef LSCFuncsResolution::PREFIX_LSC_ATOMIC = "__builtin_IB_lsc_atomic_";
const StringRef LSCFuncsResolution::PREFIX_LSC_PREFETCH = "__builtin_IB_lsc_prefetch_global_";
const StringRef LSCFuncsResolution::PREFIX_LSC_SIMD_BLOCK_PREFETCH = "__builtin_IB_lsc_simd_block_prefetch_";

// Register pass to igc-opt
#define PASS_FLAG "igc-lsc-funcs-translation"
#define PASS_DESCRIPTION "Translate lsc builtin functions into igc intrinsics"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LSCFuncsResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(LSCFuncsResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

LSCFuncsResolution::LSCFuncsResolution() : FunctionPass(ID) {
  initializeLSCFuncsResolutionPass(*PassRegistry::getPassRegistry());
}

bool LSCFuncsResolution::runOnFunction(Function &F) {
  m_pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  int defaultSimdSize = 0;

  switch (m_pCtx->platform.getPlatformInfo().eProductFamily) {
  case IGFX_DG2:
  case IGFX_METEORLAKE:
  case IGFX_ARROWLAKE:
    defaultSimdSize = 16;
    break;
  default:
    defaultSimdSize = 32;
    break;
  }

  auto m_pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  auto funcInfoMD = m_pMdUtils->getFunctionsInfoItem(&F);
  int actualSimdSize = funcInfoMD->getSubGroupSize()->getSIMDSize();
  isHalfSimdMode = defaultSimdSize != actualSimdSize; // SIMD8 on DG2, SIMD16 on PVC

  m_changed = false;

  visit(F);

  for (auto *inst : m_instsToErase) {
    inst->eraseFromParent();
  }
  m_instsToErase.clear();

  verifyBlock2DAddressPayload();

  if (hasError()) {
    m_pCtx->EmitError(m_ErrorMsg.str().c_str(), &F);
    m_ErrorMsg.str(std::string()); // clear stringstream
  }
  return m_changed;
}

void LSCFuncsResolution::visitCallInst(CallInst &CI) {
  /// Process LCS intrinsics
  m_pCurrInstFunc = CI.getCalledFunction();
  if (!m_pCurrInstFunc)
    return;
  m_pCurrInst = &CI;

  StringRef FN = m_pCurrInstFunc->getName();
  Instruction *lscCall = nullptr;

  //////////////
  // loads
  if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_LOAD_global)) {
    lscCall = CreateLSCLoadIntrinsicCallInst(GenISAIntrinsic::GenISA_LSCLoad, false);
  } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_LOAD_BLOCK_global)) {
    lscCall = CreateLSCLoadIntrinsicCallInst(GenISAIntrinsic::GenISA_LSCLoadBlock, false);
  } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_LOAD_local)) {
    lscCall = CreateLSCLoadIntrinsicCallInst(GenISAIntrinsic::GenISA_LSCLoad, true);
  } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_LOAD_CMASK_global)) {
    lscCall = CreateLSCLoadCmaskIntrinsicCallInst(false);
  } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_LOAD_CMASK_local)) {
    lscCall = CreateLSCLoadCmaskIntrinsicCallInst(true);
    //////////////
    // prefetches
  } else if (FN.consume_front(LSCFuncsResolution::PREFIX_LSC_SIMD_BLOCK_PREFETCH)) {
    lscCall = CreateLSCSimdBlockPrefetchIntrinsicCallInst(FN);
  } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_LOAD_status)) {
    lscCall = CreateLSCLoadStatusPreftchIntrinsicCallInst(GenISAIntrinsic::GenISA_LSCLoadStatus);
  } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_PREFETCH)) {
    lscCall = CreateLSCLoadStatusPreftchIntrinsicCallInst(GenISAIntrinsic::GenISA_LSCPrefetch);
    //////////////
    // stores
  } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_STORE_global)) {
    lscCall = CreateLSCStoreIntrinsicCallInst(GenISAIntrinsic::GenISA_LSCStore, false);
  } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_STORE_BLOCK_global)) {
    lscCall = CreateLSCStoreIntrinsicCallInst(GenISAIntrinsic::GenISA_LSCStoreBlock, false);
  } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_STORE_local)) {
    lscCall = CreateLSCStoreIntrinsicCallInst(GenISAIntrinsic::GenISA_LSCStore, true);
  } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_STORE_CMASK_global)) {
    lscCall = CreateLSCStoreCmaskIntrinsicCallInst(false);
  } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_STORE_CMASK_local)) {
    lscCall = CreateLSCStoreCmaskIntrinsicCallInst(true);
    //////////////
    // 2d block intrinsics
  } else if (FN.consume_front("__builtin_IB_subgroup_createBlock2DAddressPayload")) {
    lscCall = CreateLSC2DBlockAddressPayload(CI);
  } else if (FN.consume_front("__builtin_IB_subgroup_copyBlock2DAddressPayload")) {
    lscCall = CopyLSC2DBlockAddressPayload(CI);
  } else if (FN.consume_front("__builtin_IB_subgroup_setBlock2DAddressPayloadBase")) {
    lscCall = SetLSC2DBlockAddressPayloadField(CI, LSC2DBlockField::BASE, false);
  } else if (FN.consume_front("__builtin_IB_subgroup_setBlock2DAddressPayloadWidth")) {
    lscCall = SetLSC2DBlockAddressPayloadField(CI, LSC2DBlockField::WIDTH, false);
  } else if (FN.consume_front("__builtin_IB_subgroup_setBlock2DAddressPayloadHeight")) {
    lscCall = SetLSC2DBlockAddressPayloadField(CI, LSC2DBlockField::HEIGHT, false);
  } else if (FN.consume_front("__builtin_IB_subgroup_setBlock2DAddressPayloadPitch")) {
    lscCall = SetLSC2DBlockAddressPayloadField(CI, LSC2DBlockField::PITCH, false);
  } else if (FN.consume_front("__builtin_IB_subgroup_setBlock2DAddressPayloadBlockX")) {
    lscCall = SetLSC2DBlockAddressPayloadField(CI, LSC2DBlockField::BLOCKX, false);
  } else if (FN.consume_front("__builtin_IB_subgroup_setBlock2DAddressPayloadBlockY")) {
    lscCall = SetLSC2DBlockAddressPayloadField(CI, LSC2DBlockField::BLOCKY, false);
  } else if (FN.consume_front("__builtin_IB_subgroup_addBlock2DAddressPayloadBlockX")) {
    lscCall = SetLSC2DBlockAddressPayloadField(CI, LSC2DBlockField::BLOCKX, true);
  } else if (FN.consume_front("__builtin_IB_subgroup_addBlock2DAddressPayloadBlockY")) {
    lscCall = SetLSC2DBlockAddressPayloadField(CI, LSC2DBlockField::BLOCKY, true);
  } else if (FN.consume_front(LSCFuncsResolution::PREFIX_SUBGROUP_BLOCK_READ_AP)) {
    lscCall = CreateSubGroup2DBlockOperationAP(CI, FN, true);
  } else if (FN.consume_front(LSCFuncsResolution::PREFIX_SUBGROUP_BLOCK_WRITE_AP)) {
    lscCall = CreateSubGroup2DBlockOperationAP(CI, FN, false);
  } else if (FN.consume_front(LSCFuncsResolution::PREFIX_SUBGROUP_BLOCK_READ)) {
    lscCall = CreateSubGroup2DBlockOperation(CI, FN, true);
  } else if (FN.consume_front(LSCFuncsResolution::PREFIX_SUBGROUP_BLOCK_WRITE)) {
    lscCall = CreateSubGroup2DBlockOperation(CI, FN, false);
    //////////////
    // atomics
  } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_ATOMIC)) {
    bool isLocalMem = FN.find("_local_") != StringRef::npos;
    lscCall = CreateLSCAtomicIntrinsicCallInst(isLocalMem);
    //////////////
    // misc stuff
  } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_FENCE_EVICT_TO_MEMORY)) {
    // LSC fence
    lscCall = CreateLSCFenceEvictToMemory();
  } else if (FN.startswith(LSCFuncsResolution::PREFIX_LSC_FENCE)) {
    // LSC fence
    lscCall = CreateLSCFenceIntrinsicCallInst(CI);
  } else {
    // not an LSC message, bail silently
    return;
  }

  // LSC is not supported/enabled
  if (!m_pCtx->platform.isProductChildOf(IGFX_DG2)) {
    IGC_ASSERT_MESSAGE(0, "LSC not supported on this platform");
    reportError("LSC not supported on this platform");
    return;
  }

  if (lscCall != nullptr) {
    lscCall->setDebugLoc(CI.getDebugLoc());
    CI.replaceAllUsesWith(lscCall);
    m_instsToErase.insert(&CI);

    m_changed = true;
  }
}

Instruction *LSCFuncsResolution::CreateLSCLoadIntrinsicCallInst(GenISAIntrinsic::ID op, bool isLocalMem) {
  auto typeInfo = decodeTypeInfoFromName();
  if (hasError()) {
    return nullptr;
  }

  Value *args[5]{m_pCurrInst->getArgOperand(0),          // base address
                 getImmediateElementOffset(1, typeInfo), // imm element offset
                 getConstantInt32(typeInfo.dataSize),    // e.g. D32
                 getConstantInt32(typeInfo.vectorSize),  // e.g. V4
                 isLocalMem ?                            // cache options (default value for SLM)
                     getConstantInt32(LSC_L1DEF_L3DEF)
                            : getCacheControlOpts(2)};

  Type *OvldTys[2]{m_pCurrInstFunc->getReturnType(), args[0]->getType()};
  Function *lscFunc = GenISAIntrinsic::getDeclaration(m_pCurrInstFunc->getParent(), op, OvldTys);
  Instruction *lscCall = CallInst::Create(lscFunc, args, "", m_pCurrInst);
  return lscCall;
}

Instruction *LSCFuncsResolution::CreateLSC2DBlockAddressPayload(CallInst &CI) {
  Value *Base = CI.getArgOperand(0);
  Value *Width = CI.getArgOperand(1);
  Value *Height = CI.getArgOperand(2);
  Value *Pitch = CI.getArgOperand(3);
  Value *BlkX = CI.getArgOperand(4);
  Value *BlkY = CI.getArgOperand(5);
  Value *BlkWidth = CI.getArgOperand(6);
  Value *BlkHeight = CI.getArgOperand(7);
  Value *NumBlks = CI.getArgOperand(8);

  if (!isa<ConstantInt>(BlkWidth) || !isa<ConstantInt>(BlkHeight) || !isa<ConstantInt>(NumBlks)) {
    IGC_ASSERT_MESSAGE(0, "Block2D address payload: block_x, block_y,"
                          " and num of blocks must be constant!");
    return nullptr;
  }

  Value *args[]{Base, Width, Height, Pitch, BlkX, BlkY, BlkWidth, BlkHeight, NumBlks};
  Type *Tys[1] = {CI.getType()};
  Function *Func =
      GenISAIntrinsic::getDeclaration(CI.getModule(), GenISAIntrinsic::GenISA_LSC2DBlockCreateAddrPayload, Tys);
  Instruction *I = CallInst::Create(Func, args, "Block2D_AddrPayload", &CI);
  updateDebugLoc(&CI, I);
  return I;
}

Instruction *LSCFuncsResolution::CopyLSC2DBlockAddressPayload(CallInst &CI) {
  Value *args[]{CI.getArgOperand(0)};
  Function *Func =
      GenISAIntrinsic::getDeclaration(CI.getModule(), GenISAIntrinsic::GenISA_LSC2DBlockCopyAddrPayload, CI.getType());
  Instruction *I = CallInst::Create(Func, args, "Block2D_AddrPayload", &CI);
  updateDebugLoc(&CI, I);
  return I;
}

Instruction *LSCFuncsResolution::SetLSC2DBlockAddressPayloadField(CallInst &CI, LSC2DBlockField Field, bool IsAddend) {
  Value *args[4];
  args[0] = CI.getArgOperand(0);
  args[1] = ConstantInt::get(Type::getInt32Ty(CI.getContext()), Field);
  args[2] = CI.getArgOperand(1);
  args[3] = ConstantInt::get(Type::getInt1Ty(CI.getContext()), IsAddend);
  Type *tys[2] = {args[0]->getType(), args[2]->getType()};
  Function *Func =
      GenISAIntrinsic::getDeclaration(CI.getModule(), GenISAIntrinsic::GenISA_LSC2DBlockSetAddrPayloadField, tys);

  Instruction *I = CallInst::Create(Func, args, "", &CI);
  updateDebugLoc(&CI, I);
  return I;
}

Instruction *LSCFuncsResolution::CreateSubGroup2DBlockOperationAP(CallInst &CI, StringRef funcName, bool isRead) {
  const char *fname = funcName.data();

  bool isPrefetch = funcName.consume_front("_prefetch");
  uint32_t isTranspose = funcName.consume_front("_transpose") ? 1 : 0;
  uint32_t isVnniTransform = funcName.consume_front("_transform") ? 1 : 0;

  uint32_t elemSize = 0;
  uint32_t blkWidth = 0;
  uint32_t blkHeight = 0;
  uint32_t numBlks = 0;
  if (funcName.consume_front("_u8")) {
    elemSize = 8;
  } else if (funcName.consume_front("_u16")) {
    elemSize = 16;
  } else if (funcName.consume_front("_u32")) {
    elemSize = 32;
  } else if (funcName.consume_front("_u64")) {
    elemSize = 64;
  } else {
    IGC_ASSERT_MESSAGE(0, "Invalid or missing element size in: %s\n", fname);
    return nullptr;
  }

  if (funcName.consume_front("_m32")) {
    blkHeight = 32;
  } else if (funcName.consume_front("_m16")) {
    blkHeight = 16;
  } else if (funcName.consume_front("_m8")) {
    blkHeight = 8;
  } else if (funcName.consume_front("_m4")) {
    blkHeight = 4;
  } else if (funcName.consume_front("_m2")) {
    blkHeight = 2;
  } else if (funcName.consume_front("_m1")) {
    blkHeight = 1;
  } else {
    std::stringstream ss;
    ss << "Missing or unsupported m element in : " << fname << "\n";
    reportError(ss.str().c_str());
    IGC_ASSERT_MESSAGE(0, "%s", ss.str().c_str());
    return nullptr;
  }

  if (funcName.consume_front("k64")) {
    blkWidth = 64;
  } else if (funcName.consume_front("k32")) {
    blkWidth = 32;
  } else if (funcName.consume_front("k16")) {
    blkWidth = 16;
  } else if (funcName.consume_front("k8")) {
    blkWidth = 8;
  } else if (funcName.consume_front("k4")) {
    blkWidth = 4;
  } else if (funcName.consume_front("k2")) {
    blkWidth = 2;
  } else if (funcName.consume_front("k1")) {
    blkWidth = 1;
  } else if (isPrefetch && funcName.consume_front("k256")) {
    blkWidth = 256;
  } else if (isPrefetch && funcName.consume_front("k128")) {
    blkWidth = 128;
  } else {
    std::stringstream ss;
    ss << "Missing or unsupported k element in : " << fname << "\n";
    reportError(ss.str().c_str());
    IGC_ASSERT_MESSAGE(0, "Unsupported k element in : %s\n", fname);
    return nullptr;
  }

  if (funcName.consume_front("v1")) {
    numBlks = 1;
  } else if (funcName.consume_front("v2")) {
    numBlks = 2;
  } else if (funcName.consume_front("v4")) {
    numBlks = 4;
  } else {
    std::stringstream ss;
    ss << "Missing or unsupported v element in : " << fname << "\n";
    reportError(ss.str().c_str());
    IGC_ASSERT_MESSAGE(0, "Unsupported v element in : %s\n", fname);
    return nullptr;
  }

  if (!isTranspose && !isVnniTransform) {
    uint32_t rowBytesPerBlk = ((elemSize / 8) * blkWidth);
    if ((rowBytesPerBlk * numBlks) > 64 || rowBytesPerBlk < 4) {
      std::stringstream ss;
      ss << "width x numBlocks > 64 bytes: " << fname << "\n";
      reportError(ss.str().c_str());
      IGC_ASSERT_MESSAGE(0,
                         "width x numBlocks should be no "
                         "larger than 64 bytes : %s\n",
                         fname);
      return nullptr;
    }
  } else if (isTranspose && !isVnniTransform) {
    bool isLegitW8 = false;
    isLegitW8 = m_pCtx->platform.supports2dBlockTranspose64ByteWidth();

    bool isValid64 = (elemSize == 64 && blkHeight == 8 && (blkWidth <= 4 || (blkWidth == 8 && isLegitW8)));
    bool isValid32 = (elemSize == 32 && blkHeight <= 32 && blkWidth <= 8);
    if (numBlks != 1 || !(isValid32 || isValid64)) {
      std::stringstream ss;
      ss << "Unsupported m/k/v transpose combination in: " << fname << "\n";
      reportError(ss.str().c_str());
      IGC_ASSERT_MESSAGE(0, "Unsupported m/k/v transpose combination in : %s\n", fname);
      return nullptr;
    }
  } else if (!isTranspose && isVnniTransform) {
    bool isValid8 = (elemSize == 8 && blkHeight >= 4 && blkWidth >= 4);
    bool isValid16 = (elemSize == 16 && blkHeight >= 2 && blkWidth >= 2 && blkWidth <= 32);
    if (!(isValid8 || isValid16)) {
      std::stringstream ss;
      ss << "Unsupported m/k/v transform combination in: " << fname << "\n";
      reportError(ss.str().c_str());
      IGC_ASSERT_MESSAGE(0, "Unsupported m/k/v transform combination in : %s\n", fname);
      return nullptr;
    }
  } else {
    std::stringstream ss;
    ss << "Transpose and transform are not allowed to be used together : " << fname << "\n";
    reportError(ss.str().c_str());
    IGC_ASSERT_MESSAGE(0,
                       "Transpose and transform are not allowed "
                       "to be used together: %s\n",
                       fname);
    return nullptr;
  }

  Value *AddrPayload = CI.getArgOperand(0);
  Value *ImmX = CI.getArgOperand(1);
  Value *ImmY = CI.getArgOperand(2);
  Value *cacheArg = CI.getArgOperand(isRead ? 3 : 4);
  if (!isa<ConstantInt>(cacheArg)) {
    std::stringstream ss;
    ss << "cacheopts must be an immediate constant : " << fname << "\n";
    reportError(ss.str().c_str());
    IGC_ASSERT_MESSAGE(0, "cacheopts must be an immediate constant: %s\n", fname);
    return nullptr;
  }
  uint32_t cacheOptsArgNum = isRead ? 3 : 4;
  Value *cacheOpts = getCacheControlOpts(cacheOptsArgNum);

  LLVMContext &C = CI.getContext();
  ConstantInt *eltVal = ConstantInt::get((Type::getInt32Ty(C)), elemSize);
  ConstantInt *wVal = ConstantInt::get((Type::getInt32Ty(C)), blkWidth);
  ConstantInt *hVal = ConstantInt::get((Type::getInt32Ty(C)), blkHeight);
  ConstantInt *nblkVal = ConstantInt::get((Type::getInt32Ty(C)), numBlks);
  ConstantInt *tranVal = ConstantInt::get((Type::getInt1Ty(C)), isTranspose);
  ConstantInt *vnniVal = ConstantInt::get((Type::getInt1Ty(C)), isVnniTransform);

  SmallVector<Value *, 16> args{
      AddrPayload, ImmX, ImmY, eltVal, wVal, hVal, nblkVal, tranVal, vnniVal, cacheOpts,
  };

  Function *Func = nullptr;
  if (isRead) {
    if (isPrefetch) {
      Func = GenISAIntrinsic::getDeclaration(CI.getCalledFunction()->getParent(),
                                             GenISAIntrinsic::GenISA_LSC2DBlockPrefetchAddrPayload,
                                             AddrPayload->getType());
    } else {
      Type *tys[2] = {CI.getType(), AddrPayload->getType()};
      Func = GenISAIntrinsic::getDeclaration(CI.getCalledFunction()->getParent(),
                                             GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload, tys);
    }
  } else {
    Value *storedVal = CI.getArgOperand(3);
    args.push_back(storedVal);
    Type *tys[2] = {AddrPayload->getType(), storedVal->getType()};
    Func = GenISAIntrinsic::getDeclaration(CI.getCalledFunction()->getParent(),
                                           GenISAIntrinsic::GenISA_LSC2DBlockWriteAddrPayload, tys);
  }

  Instruction *callI = CallInst::Create(Func, args, "", &CI);
  updateDebugLoc(&CI, callI);

  // For verifying block dimension at the end of runOnFuncion.
  m_lsc2dblock_readwrite.push_back(callI);
  return callI;
}

// Verify the block shape from address payload matches one specified
// in read/write/prefetch builtin.
void LSCFuncsResolution::verifyBlock2DAddressPayload() {
  if (m_lsc2dblock_readwrite.empty()) {
    return;
  }

  // Given the following:
  //  (1)  int* AP = LSC2DBlockCreateAddrPayload(....)
  //       ...
  //  (2)  LSC2DBLockSetBlockXY(AP, ...)
  //       ...
  //  (3)  x = LSC2DBlockReadAddrPayload(AP1, ...)
  // this function verifies that block dimention used in read at (3) is the
  // same as one created at (1) (This is user's responsibility).
  //
  // rootAPMap maps an AP to a root AP. For the above, it maps (2) to (1).
  // (1) is called the root AP.
  std::unordered_map<GenIntrinsicInst *, GenIntrinsicInst *> rootAPMap;

  auto isAPUpdateInst = [](GenIntrinsicInst *aG) {
    switch (aG->getIntrinsicID()) {
    // No longer returns a value
    // case GenISAIntrinsic::GenISA_LSC2DBlockSetAddrPayloadField:
    //
    // Copy does not change block dimention, so it is treated not as
    // a creation for the purpose of this verification
    case GenISAIntrinsic::GenISA_LSC2DBlockCopyAddrPayload:
      return true;
    default:
      break;
    }
    return false;
  };

  auto isAPCreateInst = [](GenIntrinsicInst *aG) {
    switch (aG->getIntrinsicID()) {
    case GenISAIntrinsic::GenISA_LSC2DBlockCreateAddrPayload:
      return true;
    default:
      break;
    }
    return false;
  };

  auto isSameDimension = [](GenIntrinsicInst *RootGII, GenIntrinsicInst *GII, bool isGIIRoot) {
    const int w_no = isGIIRoot ? 6 : 4;
    const int h_no = isGIIRoot ? 7 : 5;
    const int b_no = isGIIRoot ? 8 : 6;
    int width = (int)cast<ConstantInt>(GII->getArgOperand(w_no))->getZExtValue();
    int height = (int)cast<ConstantInt>(GII->getArgOperand(h_no))->getZExtValue();
    int numBlks = (int)cast<ConstantInt>(GII->getArgOperand(b_no))->getZExtValue();
    int rt_width = (int)cast<ConstantInt>(RootGII->getArgOperand(6))->getZExtValue();
    int rt_height = (int)cast<ConstantInt>(RootGII->getArgOperand(7))->getZExtValue();
    int rt_numBlks = (int)cast<ConstantInt>(RootGII->getArgOperand(8))->getZExtValue();
    return (height == rt_height && width == rt_width && numBlks == rt_numBlks);
  };

  for (auto V : m_lsc2dblock_readwrite) {
    GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(V);
    if (!GII)
      continue; // safety check
    auto GID = GII->getIntrinsicID();
    if (GID != GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload &&
        GID != GenISAIntrinsic::GenISA_LSC2DBlockWriteAddrPayload &&
        GID != GenISAIntrinsic::GenISA_LSC2DBlockPrefetchAddrPayload)
      continue; // safety check

    // worklist : list of AP defining insts
    std::list<Value *> worklist;
    Value *aAP = GII->getArgOperand(0);
    worklist.push_back(aAP);

    auto currII = worklist.begin();
    GenIntrinsicInst *rootGII = nullptr;
    for (; currII != worklist.end(); ++currII) {
      Value *V = *currII;
      if (GenIntrinsicInst *pGI = dyn_cast<GenIntrinsicInst>(V)) {
        GenIntrinsicInst *thisRoot = nullptr;
        auto II = rootAPMap.find(GII);
        if (II != rootAPMap.end())
          thisRoot = II->second;
        else if (isAPCreateInst(pGI))
          thisRoot = pGI;

        if (thisRoot != nullptr) {
          if (rootGII == nullptr) {
            rootGII = thisRoot;
          } else if (rootGII != thisRoot && !isSameDimension(rootGII, thisRoot, true)) {
            IGC_ASSERT_MESSAGE(0, "Block2D address payload: "
                                  "addressPayload argument is defined by more than "
                                  "one create builtins with different dimensions");
            return;
          }
          continue;
        }
        if (isAPUpdateInst(pGI)) {
          Value *tV = pGI->getArgOperand(0);
          if (std::find(worklist.begin(), worklist.end(), tV) == worklist.end())
            worklist.push_back(tV);
        } else {
          IGC_ASSERT_MESSAGE(0, "AddressPayload is created with incorrect builtin!");
          return;
        }
      } else if (PHINode *PHI = dyn_cast<PHINode>(V)) {
        for (uint i = 0, e = PHI->getNumOperands(); i != e; ++i) {
          Value *Src = PHI->getOperand(i);
          if (std::find(worklist.begin(), worklist.end(), Src) == worklist.end()) {
            worklist.push_back(Src);
          }
        }
      } else {
        IGC_ASSERT_MESSAGE(0, "Block2D address payload: must be "
                              "created with creation builtin in the current function");
        return;
      }
    }
    if (rootGII == nullptr) {
      IGC_ASSERT_MESSAGE(0, "Block2D address payload: not defined");
      return;
    }

    for (auto V : worklist) {
      GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(V);
      if (!GII)
        continue;
      rootAPMap[GII] = rootGII;
    }

    if (!isSameDimension(rootGII, GII, false)) {
      std::stringstream ss;
      ss << "Block2D address payload: read/write builtins' "
         << "block dimention do not match address payload's\n";
      reportError(ss.str().c_str());
      IGC_ASSERT_MESSAGE(0, "Block2D address payload: read/write builtins' "
                            "block dimention do not match address payload's");
      return;
    }
  }
}

static StringRef consume_number(StringRef name, const std::string &prefix, uint32_t *number) {
  if (!number) {
    IGC_ASSERT_MESSAGE(0, "Expected a valid pointer to number");
    return name;
  }

  *number = 0;
  size_t pos = 0;
  if (name.consume_front(prefix) && name.size() > 0 && isdigit(name[0])) {
    *number = std::stoi(name.str(), &pos);
    return name.drop_front(pos);
  }
  return name;
}

Instruction *LSCFuncsResolution::CreateSubGroup2DBlockOperation(llvm::CallInst &CI, llvm::StringRef funcName,
                                                                bool isRead) {
  IGC::IGCMD::MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  IGC::IGCMD::FunctionInfoMetaDataHandle funcInfoMD = pMdUtils->getFunctionsInfoItem(CI.getParent()->getParent());
  unsigned int subGrpSize = funcInfoMD->getSubGroupSize()->getSIMDSize();

  funcName.consume_front("_flat");
  bool isPrefetch = funcName.consume_front("_prefetch");
  bool hasCacheOpts = funcName.consume_front("_cacheopts") || isPrefetch;
  uint32_t isTranspose = funcName.consume_front("_transpose") ? 1 : 0;
  uint32_t isVnniTransform = funcName.consume_front("_transform") ? 1 : 0;
  hasCacheOpts |= funcName.consume_front("_cacheopts");

  uint32_t elemSize = 0;
  if (funcName.consume_front("_u8")) {
    elemSize = 8;
  } else if (funcName.consume_front("_u16")) {
    elemSize = 16;
  } else if (funcName.consume_front("_u32")) {
    elemSize = 32;
  } else if (funcName.consume_front("_u64")) {
    elemSize = 64;
  }

  if (elemSize == 0) {
    IGC_ASSERT_MESSAGE(0, "Invalid element size settings for subgroup_block_read/write.");
    return nullptr;
  }

  // Optional number of elements per work item. If not present, the value is
  // assumed to be equal to dimension M. The actual value is not needed here;
  // only used to differentiate builtins.
  if (funcName.consume_front("_wi")) {
    funcName = funcName.drop_until([](char c) { return c == '_' || c == '\0'; });
  }

  uint32_t tileWidth = 0;
  uint32_t tileHeight = 0;
  uint32_t numBlocksV = 2;
  if (!isTranspose && !isVnniTransform) {
    // 2x ATile Block Read
    // __builtin_IB_subgroup_block_read_flat_u8_m1k32v2
    // __builtin_IB_subgroup_block_read_flat_u8_m2k32v2
    // __builtin_IB_subgroup_block_read_flat_u8_m4k32v2
    // __builtin_IB_subgroup_block_read_flat_u8_m8k32v2
    // __builtin_IB_subgroup_block_read_flat_u16_m1k16v2
    // __builtin_IB_subgroup_block_read_flat_u16_m2k16v2
    // __builtin_IB_subgroup_block_read_flat_u16_m4k16v2
    // __builtin_IB_subgroup_block_read_flat_u16_m8k16v2
    funcName = consume_number(funcName, "_m", &tileHeight);
    if (tileHeight < 1 || tileHeight > 32) {
      IGC_ASSERT_MESSAGE(0, "Unrecognized m element in __builtin_IB_subgroup_block_read/write.");
      return nullptr;
    }

    if (funcName.consume_front("k64")) {
      tileWidth = 64;
    } else if (funcName.consume_front("k32")) {
      tileWidth = 32;
    } else if (funcName.consume_front("k16")) {
      tileWidth = 16;
    } else if (funcName.consume_front("k8")) {
      tileWidth = 8;
    } else if (funcName.consume_front("k4")) {
      tileWidth = 4;
    } else if (isPrefetch && funcName.consume_front("k256")) {
      tileWidth = 256;
    } else if (isPrefetch && funcName.consume_front("k128")) {
      tileWidth = 128;
    } else {
      IGC_ASSERT_MESSAGE(0, "Unrecognized k element in __builtin_IB_subgroup_block_read/write.");
      return nullptr;
    }

    if (funcName.consume_front("v1")) {
      numBlocksV = 1;
    } else if (isRead && funcName.consume_front("v4")) {
      numBlocksV = 4;
    } else {
      IGC_ASSERT_MESSAGE(funcName.consume_front("v2"),
                         "Unrecognized v element in __builtin_IB_subgroup_block_read/write.");
    }

    // (1) Special handling of the following when GRF size = 64 bytes
    //    intel_sub_group_2d_block_read_8b_1r32x2c  (u8_m1k32v2)
    //    intel_sub_group_2d_block_read_16b_1r16x2c (u16_m1k16v2)
    // They are defined to return 64 bytes, but the HW block read
    // returns 128 bytes (two GRFs, as a block size must be multiple
    // of GRF, unused part is zero-padded. Note that those APIs have
    // their block size to be multiple of GRF (1 GRF) when GRF size
    // is 32 bytes). Additional mov instructions are needed to pack
    // the lower halves of each GRF as the final return value.
    //
    // For those cases, instead of 2 blocks, using equivalent single-block
    // read to avoid those mov instructions by just doubling their width:
    //   8b_m1k32v2  --> 8b_m1k64v1
    //   16b_m1k16v2 --> 16b_m1k32v1
    //
    // (2) The following is an exception:
    //    int2 = intel_sub_group_2d_block_read_32b_1r8x2c  (u32_m1k8v2)
    // it is indeed defined as return 128 bytes. As this 2d read has
    // 64 bytes, only lower 8 lanes have data and upper 8 lanes got zero.
    // No change to this read!
    if (m_pCtx->platform.getGRFSize() == 64 && isRead && numBlocksV == 2 && tileHeight == 1 &&
        (elemSize * tileWidth) == 256 && elemSize != 32 /* exception shown above in (2) */) {
      numBlocksV = 1;
      tileWidth *= 2;
    }
  } else if (isTranspose && !isVnniTransform) {
    if (elemSize == 64) {
      numBlocksV = 1;
      tileHeight = subGrpSize;
      if (funcName.consume_front("_m8")) {
        // For __builtin_IB_subgroup_block_read_cacheopts_transpose_u64_m8k1
        //     __builtin_IB_subgroup_block_read_cacheopts_transpose_u64_m8k2
        //     __builtin_IB_subgroup_block_read_cacheopts_transpose_u64_m8k4
        // and __builtin_IB_subgroup_block_read_cacheopts_transpose_u64_m8k8
        // not tied to subGrpSize
        tileHeight = 8;
      }

      funcName.consume_front("_");
      funcName = consume_number(funcName, "k", &tileWidth);
      if (tileWidth == 4) {
       // __builtin_IB_subgroup_block_read_flat_transpose_u64_k4
       // __builtin_IB_subgroup_block_read_cacheopts_transpose_u64_m8k4
        ;
      } else if (tileHeight == 8 && (tileWidth == 1 || tileWidth == 2)) {
        // For __builtin_IB_subgroup_block_read_cacheopts_transpose_u64_m8k1
        //     __builtin_IB_subgroup_block_read_cacheopts_transpose_u64_m8k2
        ;
      } else if (m_pCtx->platform.supports2dBlockTranspose64ByteWidth() && tileWidth == 8) {
        // __builtin_IB_subgroup_block_read_flat_transpose_u64_k8
        ;
      } else {
        if (m_pCtx->platform.supports2dBlockTranspose64ByteWidth()) {
          IGC_ASSERT_MESSAGE(0, "Transpose with 64 bit element size only supports width: 4, 8");
          return nullptr;
        }
        IGC_ASSERT_MESSAGE(0, "Transpose with 64 bit element size only supports width 1, 2, 4 for height 8.");
        return nullptr;
      }
    } else if (elemSize == 32) {
      // isTranspose, dword elements
      // __builtin_IB_subgroup_block_read_flat_transpose_u32_k8
      // can be used as equivalent of:
      // transpose_transform_u8_k32
      // transpose_transform_u16_k16
      numBlocksV = 1;
      tileHeight = subGrpSize;
      if (funcName.consume_front("_m32")) {
        // not tied to subgroup size,
        // each SIMD lane gets two rows in SIMD16
        tileHeight = 32;
      } else if (funcName.consume_front("_m16")) {
        tileHeight = 16;
      } else if (funcName.consume_front("_m8")) {
        tileHeight = 8;
      }

      tileWidth = 8;
      funcName.consume_front("_");
      funcName = consume_number(funcName, "k", &tileWidth);
      if (tileWidth < 1 ||
          (tileWidth > 8 && (!m_pCtx->platform.supports2dBlockTranspose64ByteWidth() || tileWidth != 16))) {
        if (m_pCtx->platform.supports2dBlockTranspose64ByteWidth()) {
          IGC_ASSERT_MESSAGE(0, "Transpose with 32 bit element size only supports width: 1 - 8, 16");
          return nullptr;
        }
        IGC_ASSERT_MESSAGE(0, "Transpose with 32 bit element size only supports width: 1 - 8.");
        return nullptr;
      }
    }
    else if (elemSize == 16 && m_pCtx->platform.getGRFSize() == 64) {
      // The following are emulated on PVC+
      // (GRF size: 64 bytes, simd size >= simd16)
      //   __builtin_IB_subgroup_block_read_cacheopts_transpose_u16_m16k4
      //   __builtin_IB_subgroup_block_read_cacheopts_transpose_u16_m16k8
      //   __builtin_IB_subgroup_block_read_cacheopts_transpose_u16_m16k16
      numBlocksV = 1;
      if (funcName.consume_front("_m16")) {
        tileHeight = 16;
      } else {
        IGC_ASSERT_MESSAGE(0, "D16 transpose (emulated) supports height=16 only.");
        return nullptr;
      }

      tileWidth = 0;
      funcName = consume_number(funcName, "k", &tileWidth);
      if (tileWidth != 4 && tileWidth != 8 && tileWidth != 16) {
        IGC_ASSERT_MESSAGE(0, "D16 transpose (emulated) supports width 4, 8, 16 only.");
        return nullptr;
      }
    } else if (elemSize == 8 && m_pCtx->platform.getGRFSize() == 64) {
      // The following are emulated on PVC+
      // (GRF size: 64 bytes, simd size >= simd16)
      //   __builtin_IB_subgroup_block_read_cacheopts_transpose_u8_m32k4
      //   __builtin_IB_subgroup_block_read_cacheopts_transpose_u8_m32k8
      //   __builtin_IB_subgroup_block_read_cacheopts_transpose_u8_m32k16
      numBlocksV = 1;
      if (funcName.consume_front("_m32")) {
        tileHeight = 32;
      } else {
        IGC_ASSERT_MESSAGE(0, "D8 transpose (emulated) support height=32 only.");
        return nullptr;
      }

      tileWidth = 0;
      funcName = consume_number(funcName, "k", &tileWidth);
      if (tileWidth != 4 && tileWidth != 8 && tileWidth != 16) {
        IGC_ASSERT_MESSAGE(0, "D8 transpose (emulated) support width 4, 8, 16 only.");
        return nullptr;
      }
    } else {
      IGC_ASSERT_MESSAGE(0, "Transpose only supports elemSize d32, d64.");
      return nullptr;
    }
  } else if (isVnniTransform && !isTranspose) {
    numBlocksV = 1;
    tileWidth = subGrpSize;

    if (elemSize == 8) {
      [[maybe_unused]] bool is32Height = funcName.consume_front("_k32");
      IGC_ASSERT_MESSAGE(is32Height, "Only k32 is supported for 8 bit element size, at the moment.");

      // If sub-group size is 32, we still may want to use width = 16
      // __builtin_IB_subgroup_block_read_flat_cacheopts_transform_u8_wi8_k32n16
      if (funcName.consume_front("n16")) {
        tileWidth = 16;
      }

      // __builtin_IB_subgroup_block_read_flat_transform_u8_k32v2
      if (funcName.consume_front("v2")) {
        numBlocksV = 2;
      }
      // __builtin_IB_subgroup_block_read_flat_transform_u8_k32v4
      else if (funcName.consume_front("v4")) {
        numBlocksV = 4;
      }

      // In sub-group size = 32, block dimensions are the same as sub-group size = 16.
      // __builtin_IB_subgroup_block_read_cacheopts_transform_u8_k32_sg32
      // __builtin_IB_subgroup_block_read_cacheopts_transform_u8_k32n16v2_sg32
      if (funcName.consume_front("_sg32")) {
        tileWidth = 16;
      }

      // __builtin_IB_subgroup_block_read_flat_transform_u8_k32
      tileHeight = 32;
    } else {
      // __builtin_IB_subgroup_block_read_flat_transform_u16_k16
      if (funcName.consume_front("_k16")) {
        tileHeight = 16;
      }
      // __builtin_IB_subgroup_block_read_flat_transform_u16_k32
      else if (funcName.consume_front("_k32")) {
        tileHeight = 32;
      } else {
        IGC_ASSERT_MESSAGE(0, "Unrecognized k element in __builtin_IB_subgroup_block_read/write.");
        return nullptr;
      }

      // If sub-group size is 32, we still may want to use width = 16
      // __builtin_IB_subgroup_block_read_flat_transform_u16_k16n16
      if (funcName.consume_front("n16")) {
        tileWidth = 16;
      }

      // __builtin_IB_subgroup_block_read_flat_transform_u16_k16v2
      // __builtin_IB_subgroup_block_read_flat_transform_u16_k32v2
      if (funcName.consume_front("v2")) {
        numBlocksV = 2;
      }

      // In sub-group size = 32, block imensions are the same as sub-group size = 16.
      // __builtin_IB_subgroup_block_read_cacheopts_transform_u16_k16_sg32
      // __builtin_IB_subgroup_block_read_cacheopts_transform_u16_k32n16v1_sg32
      if (funcName.consume_front("_sg32")) {
        tileWidth = 16;
      }
    }
  } else {
    IGC_ASSERT_MESSAGE(0, "Transpose and transform should not be used together.");
    return nullptr;
  }

  if (tileWidth == 0 || tileHeight == 0) {
    if (subGrpSize == 0) {
      IGC_ASSERT_MESSAGE(0, "Invalid tile width / tile height settings for subgroup_block_read because "
                            "intel_reqd_sub_group_size(16) is not set in the kernel!");
    } else {
      IGC_ASSERT_MESSAGE(0, "Invalid tile width / tile height settings for subgroup_block_read.");
    }
    return nullptr;
  }

  if (isTranspose && isVnniTransform) {
    IGC_ASSERT_MESSAGE(0, "Cannot use both hw transpose and hw vnni at the same time for subgroup_block_read.");
    return nullptr;
  }

  if (((tileWidth * elemSize) % 32) != 0) {
    IGC_ASSERT_MESSAGE(0, "Block width * element size (bytes) must be a multiple of 4 bytes.");
    return nullptr;
  }

  Value *imageResBaseoffset = CI.getArgOperand(0);
  Value *imageResWidth = CI.getArgOperand(1);
  Value *imageResHeight = CI.getArgOperand(2);
  Value *imageResPitch = CI.getArgOperand(3);

  SmallVector<Value *, 14> args;
  args.push_back(imageResBaseoffset);
  args.push_back(imageResWidth);
  args.push_back(imageResHeight);
  args.push_back(imageResPitch);

  LLVMContext &C = CI.getCalledFunction()->getContext();
  ConstantInt *constIndex = ConstantInt::get((Type::getInt32Ty(C)), 0);
  Instruction *xOffset = ExtractElementInst::Create(CI.getArgOperand(4), constIndex, "xOffset", &CI);
  ConstantInt *constIndex2 = ConstantInt::get((Type::getInt32Ty(C)), 1);
  Instruction *yOffset = ExtractElementInst::Create(CI.getArgOperand(4), constIndex2, "yOffset", &CI);
  updateDebugLoc(&CI, xOffset);
  updateDebugLoc(&CI, yOffset);
  args.push_back(xOffset);
  args.push_back(yOffset);

  ConstantInt *elemSizeConstant = ConstantInt::get((Type::getInt32Ty(C)), elemSize);
  ConstantInt *tileWidthConstant = ConstantInt::get((Type::getInt32Ty(C)), tileWidth);
  ConstantInt *tileHeightConstant = ConstantInt::get((Type::getInt32Ty(C)), tileHeight);
  ConstantInt *numBlocksVConstant = ConstantInt::get((Type::getInt32Ty(C)), numBlocksV);
  ConstantInt *isTransposeConstant = ConstantInt::get((Type::getInt1Ty(C)), isTranspose);
  ConstantInt *isVnniTransformConstant = ConstantInt::get((Type::getInt1Ty(C)), isVnniTransform);
  args.push_back(elemSizeConstant);
  args.push_back(tileWidthConstant);
  args.push_back(tileHeightConstant);
  args.push_back(numBlocksVConstant);
  args.push_back(isTransposeConstant);
  args.push_back(isVnniTransformConstant);

  if (hasCacheOpts) {
    unsigned cacheOptsId = isRead ? 5 : 6;
    args.push_back(getCacheControlOpts(cacheOptsId));
  } else {
    args.push_back(getConstantInt32(LSC_L1DEF_L3DEF));
  }

  Function *BlockFunc = nullptr;
  if (isRead) {
    BlockFunc = GenISAIntrinsic::getDeclaration(CI.getCalledFunction()->getParent(),
                                                isPrefetch ? GenISAIntrinsic::GenISA_LSC2DBlockPrefetch
                                                           : GenISAIntrinsic::GenISA_LSC2DBlockRead,
                                                CI.getCalledFunction()->getReturnType());
  } else {
    Value *srcVal = CI.getArgOperand(5);
    args.push_back(srcVal);
    BlockFunc = GenISAIntrinsic::getDeclaration(CI.getCalledFunction()->getParent(),
                                                GenISAIntrinsic::GenISA_LSC2DBlockWrite, srcVal->getType());
  }

  Instruction *BlockOp = CallInst::Create(BlockFunc, args, "", &CI);
  return BlockOp;
}

Instruction *LSCFuncsResolution::CreateLSCLoadCmaskIntrinsicCallInst(bool isLocalMem) {
  auto typeInfo = decodeTypeInfoFromName();
  if (hasError()) {
    return nullptr;
  }

  Value *args[5]{m_pCurrInst->getArgOperand(0),          // base address
                 getImmediateElementOffset(1, typeInfo), // imm element offset
                 getConstantInt32(typeInfo.dataSize),    // e.g. D32
                 getConstantInt32(typeInfo.vectorSize),  // e.g. V4
                 isLocalMem ?                            // cache options (default value for SLM)
                     getConstantInt32(LSC_L1DEF_L3DEF)
                            : getCacheControlOpts(2)};

  Type *OvldTys[2]{m_pCurrInstFunc->getReturnType(), args[0]->getType()};
  Function *lscFunc =
      GenISAIntrinsic::getDeclaration(m_pCurrInstFunc->getParent(), GenISAIntrinsic::GenISA_LSCLoadCmask, OvldTys);
  Instruction *lscCall = CallInst::Create(lscFunc, args, "", m_pCurrInst);
  return lscCall;
}

Instruction *LSCFuncsResolution::CreateLSCSimdBlockPrefetchIntrinsicCallInst(StringRef funcName) {
  Regex pattern1DBlockRead("(uchar|ushort|uint|ulong)(2|4|8|16)?");

  SmallVector<StringRef, 3> matches;
  bool matched = pattern1DBlockRead.match(funcName, &matches);
  IGC_ASSERT_MESSAGE(matched, "Unsupported simd block prefetch!");
  if (!matched)
    return nullptr;

  StringRef elementTypeName = matches[1];
  uint32_t numElements = matches[2] == "" ? 1 : std::stoi(matches[2].str());

  LscTypeInfo typeInfo{};

  if (elementTypeName.equals("uchar")) {
    typeInfo.dataSize = LSC_DATA_SIZE_8b;
  } else if (elementTypeName.equals("ushort")) {
    typeInfo.dataSize = LSC_DATA_SIZE_16b;
  } else if (elementTypeName.equals("uint")) {
    typeInfo.dataSize = LSC_DATA_SIZE_32b;
  } else if (elementTypeName.equals("ulong")) {
    typeInfo.dataSize = LSC_DATA_SIZE_64b;
  }

  switch (numElements) {
  case 1:
    typeInfo.vectorSize = LSC_DATA_ELEMS_1;
    break;
  case 2:
    typeInfo.vectorSize = LSC_DATA_ELEMS_2;
    break;
  case 4:
    typeInfo.vectorSize = LSC_DATA_ELEMS_4;
    break;
  case 8:
    typeInfo.vectorSize = LSC_DATA_ELEMS_8;
    break;
  case 16:
    typeInfo.vectorSize = LSC_DATA_ELEMS_16;
    IGC_ASSERT_MESSAGE(typeInfo.dataSize == LSC_DATA_SIZE_8b || typeInfo.dataSize == LSC_DATA_SIZE_16b,
                       "16-elements vector size is only supported for uchar and ushort!");
    break;
  default:
    IGC_ASSERT_MESSAGE(false, "Unsupported simd block prefetch variant!");
    break;
  }

  Value *args[4]{
      m_pCurrInst->getArgOperand(0), // base address
      getConstantInt32(typeInfo.dataSize), getConstantInt32(typeInfo.vectorSize),
      m_pCurrInst->getArgOperand(1) // cache controls
  };

  Type *OvldTys[1]{
      args[0]->getType(), // only one overloaded type
  };

  Function *lscFunc = GenISAIntrinsic::getDeclaration(m_pCurrInstFunc->getParent(),
                                                      GenISAIntrinsic::GenISA_LSCSimdBlockPrefetch, OvldTys);
  Instruction *lscCall = CallInst::Create(lscFunc, args, "", m_pCurrInst);
  return lscCall;
}

Instruction *LSCFuncsResolution::CreateLSCLoadStatusPreftchIntrinsicCallInst(GenISAIntrinsic::ID prefetchOp) {
  auto typeInfo = decodeTypeInfoFromName();
  if (hasError()) {
    return nullptr;
  }

  // warning this is trusting the user's typing to be correct
  // we end up using args[i]->getType()
  Value *args[5]{
      m_pCurrInst->getArgOperand(0),          // base address
      getImmediateElementOffset(1, typeInfo), // element offset
      getConstantInt32(typeInfo.dataSize), getConstantInt32(typeInfo.vectorSize),
      getCacheControlOpts(2) // cache options
  };

  Type *OvldTys[1]{
      args[0]->getType(), // only one overloaded type
  };
  Function *lscFunc = GenISAIntrinsic::getDeclaration(m_pCurrInstFunc->getParent(), prefetchOp, OvldTys);
  Instruction *lscCall = CallInst::Create(lscFunc, args, "", m_pCurrInst);
  if (prefetchOp == GenISAIntrinsic::GenISA_LSCLoadStatus) {
    // the intrinic treats bool as i1, but OCL treats bools as i8
    Type *i8 = Type::getInt8Ty(m_pCurrInst->getContext());
    lscCall = BitCastInst::CreateZExtOrBitCast(lscCall, i8, "", m_pCurrInst);
  }
  return lscCall;
}

Instruction *LSCFuncsResolution::CreateLSCStoreIntrinsicCallInst(GenISAIntrinsic::ID op, bool isLocalMem) {
  auto typeInfo = decodeTypeInfoFromName();
  if (hasError()) {
    return nullptr;
  }

  Value *args[6]{m_pCurrInst->getArgOperand(0),          // memory address where the data is stored to
                 getImmediateElementOffset(1, typeInfo), // LSC immediate offset
                 m_pCurrInst->getArgOperand(2),          // data to store
                 getConstantInt32(typeInfo.dataSize),
                 getConstantInt32(typeInfo.vectorSize),
                 isLocalMem ? // cache options (must be default for local)
                     getConstantInt32(LSC_L1DEF_L3DEF)
                            : getCacheControlOpts(3)};

  Type *OvldTys[2]{
      args[0]->getType(), // memory addr
      args[2]->getType(), // data to store
  };

  Function *lscFunc = GenISAIntrinsic::getDeclaration(m_pCurrInstFunc->getParent(), op, OvldTys);
  Instruction *lscCall = CallInst::Create(lscFunc, args, "", m_pCurrInst);
  return lscCall;
}

Instruction *LSCFuncsResolution::CreateLSCStoreCmaskIntrinsicCallInst(bool isLocalMem) {
  auto typeInfo = decodeTypeInfoFromName();
  if (hasError()) {
    return nullptr;
  }

  Value *args[6]{m_pCurrInst->getArgOperand(0),          // memory address where the data is stored to
                 getImmediateElementOffset(1, typeInfo), // LSC immediate offset
                 m_pCurrInst->getArgOperand(2),          // data to store
                 getConstantInt32(typeInfo.dataSize),
                 getConstantInt32(typeInfo.vectorSize),
                 isLocalMem ? // cache options (must be default for local)
                     getConstantInt32(LSC_L1DEF_L3DEF)
                            : getCacheControlOpts(3)};

  Type *OvldTys[2]{
      args[0]->getType(), // memory addr
      args[2]->getType(), // data to store
  };

  Function *lscFunc =
      GenISAIntrinsic::getDeclaration(m_pCurrInstFunc->getParent(), GenISAIntrinsic::GenISA_LSCStoreCmask, OvldTys);
  Instruction *lscCall = CallInst::Create(lscFunc, args, "", m_pCurrInst);
  return lscCall;
}

Instruction *LSCFuncsResolution::CreateLSCFenceIntrinsicCallInst(CallInst &CI) {
  LSC_SFID memPort = decodeSfidFromName();

  auto context = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  if (hasError()) {
    return nullptr;
  }

  Value *args[3]{
      getConstantInt32(memPort),                                  // immediate sfid
      memPort == LSC_SLM ? getConstantInt32(LSC_SCOPE_GROUP) :    // force SLM to use thread-group scope
          getImmediateEnum(0, LSC_SCOPE_GROUP, LSC_SCOPE_SYSACQ), // immediate scope of the fence
      memPort == LSC_SLM ||
              (memPort == LSC_TGM && context->platform.getPlatformInfo().eRenderCoreFamily == IGFX_XE_HPC_CORE)
          ? getConstantInt32(LSC_FENCE_OP_NONE)
          : getImmediateEnum(1, LSC_FENCE_OP_NONE, LSC_FENCE_OP_FLUSHL3) // immediate flush type
  };

  auto scope = dyn_cast<ConstantInt>(args[1]);

  if (scope && (scope->getZExtValue() == LSC_SCOPE_SYSACQ || scope->getZExtValue() == LSC_SCOPE_SYSREL)) {
    if (context->platform.isIntegratedGraphics() || context->platform.isCoreXE2()) {
      // Global memory fences are not needed on integrated devices or on Xe2 platforms.
      m_instsToErase.insert(&CI);
      return nullptr;
    }
    if (!context->platform.supportSystemFence()) {
      reportError("platform does not support system fence");
    }
  }

  Function *lscFunc =
      GenISAIntrinsic::getDeclaration(m_pCurrInstFunc->getParent(), GenISAIntrinsic::GenISA_LSCFence, {});
  Instruction *lscCall = CallInst::Create(lscFunc, args, "", m_pCurrInst);
  return lscCall;
}

// Resolve __builtin_IB_lsc_fence_evict_to_memory() builtin.
// For XE platforms it is represented by the sequence of
// evict followed by lush_l3 calls.
Instruction *LSCFuncsResolution::CreateLSCFenceEvictToMemory() {
  auto context = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  if (hasError()) {
    return nullptr;
  }

  LSC_SCOPE scope = LSC_SCOPE_GPU;

  if (context->platform.isCoreChildOf(IGFX_XE3_CORE) ||
      (context->platform.getPlatformInfo().eProductFamily == IGFX_LUNARLAKE)) {
    scope = LSC_SCOPE_SYSREL;
  }

  Value *args[3]{
      getConstantInt32(LSC_UGM), // immediate sfid
      getConstantInt32(scope),   // immediate scope of the fence
      (context->platform.getPlatformInfo().eRenderCoreFamily == IGFX_XE_HPC_CORE)
          ? getConstantInt32(LSC_FENCE_OP_NONE)
          : getConstantInt32(LSC_FENCE_OP_EVICT) // immediate flush type
  };

  Function *lscFunc =
      GenISAIntrinsic::getDeclaration(m_pCurrInstFunc->getParent(), GenISAIntrinsic::GenISA_LSCFence, {});
  Instruction *lscCall = CallInst::Create(lscFunc, args, "", m_pCurrInst);

  if (context->platform.getPlatformInfo().eRenderCoreFamily == IGFX_XE_HPG_CORE) {
    args[2] = getConstantInt32(LSC_FENCE_OP_FLUSHL3);

    lscCall = CallInst::Create(lscFunc, args, "", m_pCurrInst);
  }

  // It does not really matter which lscCall is returned, as
  // that pointer is used to call ReplaceAllUsesWith,
  // but these two instructions do not have any users.
  return lscCall;
}

Instruction *LSCFuncsResolution::CreateLSCAtomicIntrinsicCallInst(bool isLocalMem) {
  AtomicOp atomicOp = decodeAtomicOpFromName();
  if (hasError()) {
    return nullptr;
  }

  bool isFP64Atomic = atomicOp == EATOMIC_FADD64 || atomicOp == EATOMIC_FSUB64;
  bool isFP32Atomic = atomicOp == EATOMIC_FCMPWR || atomicOp == EATOMIC_FADD || atomicOp == EATOMIC_FSUB ||
                      atomicOp == EATOMIC_FMIN || atomicOp == EATOMIC_FMAX;
  bool isBF16Atomic = atomicOp == EATOMIC_FCMPWRBF16 || atomicOp == EATOMIC_FADDBF16 || atomicOp == EATOMIC_FSUBBF16 ||
                      atomicOp == EATOMIC_FMINBF16 || atomicOp == EATOMIC_FMAXBF16;
  bool hasSrc1 = atomicOp != EATOMIC_INC && atomicOp != EATOMIC_DEC && atomicOp != EATOMIC_LOAD;
  bool hasSrc2 = atomicOp == EATOMIC_FCMPWR || atomicOp == EATOMIC_CMPXCHG;

  Type *retTy = m_pCurrInstFunc->getReturnType();

  //
  // For unary and binary atomics some the extra atomic operands need to
  // be set to some default value (we use zero); but we have to carefully
  // pick a value with a type that matches the function overload
  auto getZeroArg = [&]() -> Constant * {
    int bitSize = retTy->getScalarSizeInBits();
    if (isFP32Atomic) {
      return ConstantFP::get(Type::getFloatTy(m_pCurrInst->getContext()), 0.0);
    } else if (isFP64Atomic) {
      return ConstantFP::get(Type::getDoubleTy(m_pCurrInst->getContext()), 0.0);
    } else if (bitSize == 64) {
      return ConstantInt::get(Type::getInt64Ty(m_pCurrInst->getContext()), 0, true);
    } else {
      return getConstantInt32(0);
    }
  };
  //
  Value *atomArg1 = hasSrc1 ? m_pCurrInst->getArgOperand(2) : getZeroArg();
  //
  Value *atomArg2 = hasSrc2 ? m_pCurrInst->getArgOperand(3) : getZeroArg();
  //
  const int ccOpndIx = hasSrc2 ? 4 : hasSrc1 ? 3 : 2;
  Value *args[6]{m_pCurrInst->getArgOperand(0), // memory ptr
                 m_pCurrInst->getArgOperand(1), // immediate element offset
                 atomArg1,                      // value or cmp [cmpxchg] or zero if unused
                 atomArg2,                      // value [cmpxchg] or zero if unused
                 getConstantInt32(atomicOp),    // atomic op
                 isLocalMem ?                   // cache options (default for local)
                     getConstantInt32(LSC_L1DEF_L3DEF)
                            : getCacheControlOpts(ccOpndIx, true)};

  GenISAIntrinsic::ID id = isFP64Atomic   ? GenISAIntrinsic::GenISA_LSCAtomicFP64
                           : isFP32Atomic ? GenISAIntrinsic::GenISA_LSCAtomicFP32
                           : isBF16Atomic ? GenISAIntrinsic::GenISA_LSCAtomicBF16
                                          : GenISAIntrinsic::GenISA_LSCAtomicInts;

  Function *lscFunc = nullptr;
  if (!isFP32Atomic && !isFP64Atomic) {
    Type *IntTysOvld[4]{
        retTy,              // anyint (return type)
        args[0]->getType(), // anyptr
        retTy,              // [src1] anyint
        retTy,              // [src2] anyint
    };
    lscFunc = GenISAIntrinsic::getDeclaration(m_pCurrInstFunc->getParent(), id, IntTysOvld);
  } else {
    Type *FltTysOvld[1]{
        args[0]->getType(), // anyptr
    };
    lscFunc = GenISAIntrinsic::getDeclaration(m_pCurrInstFunc->getParent(), id, FltTysOvld);
  }

  Instruction *lscCall = CallInst::Create(lscFunc, args, "", m_pCurrInst);
  return lscCall;
}

LscTypeInfo LSCFuncsResolution::decodeTypeInfoFromName() {
  StringRef FN = m_pCurrInstFunc->getName();
  LscTypeInfo ti{LSC_DATA_SIZE_8b, LSC_DATA_ELEMS_1, 1};

  // first match:
  //   ..load_{global,local,block_global}_uchar_to_uint(...)
  //   ..store_{global,local,block_global}_uchar_from_uint(...)
  // bail early if we get a hit:
  //  prefetch/load_status will show up as non-conversion types since
  //  they don't return data
  // everything else is suffixed by the type and maybe a vector integer

  if ((FN.endswith("uchar_to_uint")) || (FN.endswith("uchar_from_uint"))) {
    ti.dataSize = LSC_DATA_SIZE_8c32b;
    ti.sizeOfType = 1;
    return ti;
  } else if (FN.endswith("ushort_to_uint") || FN.endswith("ushort_from_uint")) {
    ti.dataSize = LSC_DATA_SIZE_16c32b;
    ti.sizeOfType = 2;
    return ti;
  }

  // otherwise fall through and try the regular (non-conversion) types
  // returns true if we matched the string (even if error)
  //         false if mismatched
  auto matchTypeAndVector = [&](const char *name, LSC_DATA_SIZE dsz, int sizeofType) {
    // error already reported
    if (hasError())
      return false;

    // Given "__builtin_IB_lsc_load_global_uint2", find "uint2"
    auto typePos = FN.find(name);
    if (typePos == StringRef::npos) {
      return false;
    }

    // data type matches
    ti.dataSize = dsz;
    ti.sizeOfType = sizeofType;

    // "...uchar16" -> "16"
    size_t vecOff = typePos + strlen(name);

    // if the function name suffix exactly matches (no string allocation)
    auto vectorSuffixMatches = [&](const char *pat) {
      if (vecOff + strlen(pat) != FN.size())
        return false; // suffix is not equal length
                      // equal length and prefix ==> exact match
      return FN.find(pat, vecOff) == vecOff;
    };

    // match the suffix exactly, reject garbage like
    // "uint27" (has prefix "uint2")
    if (vectorSuffixMatches("")) {
      ti.vectorSize = LSC_DATA_ELEMS_1;
    } else if (vectorSuffixMatches("2")) {
      ti.vectorSize = LSC_DATA_ELEMS_2;
      ti.sizeOfType *= 2;
    } else if (vectorSuffixMatches("3") || vectorSuffixMatches("4")) {
      if (vectorSuffixMatches("3")) {
        ti.vectorSize = LSC_DATA_ELEMS_3;
        ti.sizeOfType *= 3;
      } else {
        ti.vectorSize = LSC_DATA_ELEMS_4;
        ti.sizeOfType *= 4;
      }
    } else if (vectorSuffixMatches("8")) {
      ti.vectorSize = LSC_DATA_ELEMS_8;
      ti.sizeOfType *= 8;
    } else if (vectorSuffixMatches("16")) {
      ti.vectorSize = LSC_DATA_ELEMS_16;
      ti.sizeOfType *= 16;
      // we only support up to OpenCL vector length 8
      reportError("invalid vector size for data type");
      return true; // bail to avoid later confusing errors
    } else if (vectorSuffixMatches("32")) {
      ti.vectorSize = LSC_DATA_ELEMS_32;
      ti.sizeOfType *= 32;
      //
      // we only support up to OpenCL vector length 8
      reportError("invalid vector size for data type");
      return true; // bail to avoid later confusing errors
    } else if (vectorSuffixMatches("64")) {
      ti.vectorSize = LSC_DATA_ELEMS_64;
      ti.sizeOfType *= 64;
      //
      // we only support up to OpenCL vector length 8
      reportError("invalid vector size for data type");
      return true; // bail to avoid later confusing errors
    } else {
      // totally bogus vector size
      reportError("invalid vector size");
      return true; // bail to avoid later confusing errors
    }

    // Some sanity checking.
    // The legal prototypes provided in the builtin file constrain
    // most mischief, but remember anyone can write a prototype.
    if (ti.dataSize == LSC_DATA_SIZE_8b || ti.dataSize == LSC_DATA_SIZE_16b) {
      bool isPrefetchOrLoadStatus = FN.startswith(LSCFuncsResolution::PREFIX_LSC_LOAD_status) ||
                                    FN.startswith(LSCFuncsResolution::PREFIX_LSC_PREFETCH);
      if (!isPrefetchOrLoadStatus) {
        // D8 and D16 aren't supported yet in normal (non-prefetch)
        // loads and stores
        reportError("8b and 16b not supported");
        return true;
      } else {
        if (ti.vectorSize != LSC_DATA_ELEMS_1) {
          // because we use widening types to make this work
          reportError("8b and 16b with vector not supported");
          return true;
        }
        // use widening message
        // no data will be returned for prefetch and status will
        // broadcast bits of a single DW
        ti.dataSize = ti.dataSize == LSC_DATA_SIZE_8b ? LSC_DATA_SIZE_8c32b : LSC_DATA_SIZE_16c32b;
        ti.sizeOfType = 4;
      }
    }

    // even if errors were reported above, if we get here, it's a match
    // and we'll stop trying other types
    return true;
  };

  // N.b. certain data size and vector type may or may not exist on given
  // platforms, but we rely on the builtin proto-types to police that.
  // (We parse it successfully.)
  if (!matchTypeAndVector("uchar", LSC_DATA_SIZE_8b, 1) && !matchTypeAndVector("ushort", LSC_DATA_SIZE_16b, 2) &&
      !matchTypeAndVector("uint", LSC_DATA_SIZE_32b, 4) && !matchTypeAndVector("ulong", LSC_DATA_SIZE_64b, 8)) {
    reportError("invalid type for lsc operation");
  }
  return ti;
}

AtomicOp LSCFuncsResolution::decodeAtomicOpFromName() {
  static const uint32_t numSymbols = 52;
  static const SymbolMapping symbols[numSymbols]{
      // FP 64 (local not suported)
      {"_add_global_double", EATOMIC_FADD64},
      {"_sub_global_double", EATOMIC_FSUB64},
      // BF 16
      {"_add_global_bf16", EATOMIC_FADDBF16},
      {"_add_local_bf16", EATOMIC_FADDBF16},
      {"_sub_global_bf16", EATOMIC_FSUBBF16},
      {"_sub_local_bf16", EATOMIC_FSUBBF16},
      {"_min_global_bf16", EATOMIC_FMINBF16},
      {"_min_local_bf16", EATOMIC_FMINBF16},
      {"_max_global_bf16", EATOMIC_FMAXBF16},
      {"_max_local_bf16", EATOMIC_FMAXBF16},
      {"_cmpxchg_global_bf16", EATOMIC_FCMPWRBF16},
      {"_cmpxchg_local_bf16", EATOMIC_FCMPWRBF16},
      // FP 32
      {"_add_global_float", EATOMIC_FADD},
      {"_add_local_float", EATOMIC_FADD},
      {"_sub_global_float", EATOMIC_FSUB},
      {"_sub_local_float", EATOMIC_FSUB},
      {"_min_global_float", EATOMIC_FMIN},
      {"_min_local_float", EATOMIC_FMIN},
      {"_max_global_float", EATOMIC_FMAX},
      {"_max_local_float", EATOMIC_FMAX},
      {"_cmpxchg_global_float", EATOMIC_FCMPWR},
      {"_cmpxchg_local_float", EATOMIC_FCMPWR},
      /////////////////////////////////////////////////////
      // I16,I32,I64
      {"_add_", EATOMIC_IADD},
      {"_sub_", EATOMIC_SUB},
      // signed min/max
      {"_min_global_short", EATOMIC_MIN},
      {"_min_local_short", EATOMIC_MIN},
      {"_min_global_int", EATOMIC_MIN},
      {"_min_local_int", EATOMIC_MIN},
      {"_min_global_long", EATOMIC_MIN},
      // {"min_local_long", EATOMIC_MIN}, (global only)
      {"_max_global_short", EATOMIC_MAX},
      {"_max_local_short", EATOMIC_MAX},
      {"_max_global_int", EATOMIC_MAX},
      {"_max_local_int", EATOMIC_MAX},
      {"_max_global_long", EATOMIC_MAX},
      // {"max_local_long", EATOMIC_MAX}, (global only)

      // unsigned min/max
      {"_min_global_ushort", EATOMIC_UMIN},
      {"_min_local_ushort", EATOMIC_UMIN},
      {"_min_global_uint", EATOMIC_UMIN},
      {"_min_local_uint", EATOMIC_UMIN},
      {"_min_global_ulong", EATOMIC_UMIN},
      // {"min_local_ulong", EATOMIC_UMIN}, (global only)
      {"_max_global_ushort", EATOMIC_UMAX},
      {"_max_local_ushort", EATOMIC_UMAX},
      {"_max_global_uint", EATOMIC_UMAX},
      {"_max_local_uint", EATOMIC_UMAX},
      {"_max_global_ulong", EATOMIC_UMAX},
      // {"max_local_ulong", EATOMIC_UMAX}, (global only)
      //
      // integer compare and exchange
      {"_cmpxchg_", EATOMIC_CMPXCHG},
      // inc/dec
      {"_inc_", EATOMIC_INC},
      {"_dec_", EATOMIC_DEC},
      //  and/xor/or
      {"_and_", EATOMIC_AND},
      {"_xor_", EATOMIC_XOR},
      {"_or_", EATOMIC_OR},
      // load/store
      {"_load_", EATOMIC_LOAD},
      {"_store_", EATOMIC_STORE},
  };

  // maybe a better way to do this, but the compiler seems to need an
  // explicit size for inference below.
  static_assert(sizeof(symbols) / sizeof(symbols[0]) == numSymbols);

  AtomicOp atomicOp = EATOMIC_IADD;
  StringRef FN = m_pCurrInstFunc->getName();
  if (!findFirstInfixMapping<AtomicOp, numSymbols>(FN, symbols, atomicOp)) {
    reportError("invalid lsc atomic operation");
  }
  return atomicOp;
}

LSC_SFID LSCFuncsResolution::decodeSfidFromName() {
  static const SymbolMapping symbols[4]{
      {"_global_untyped_cross_tile", LSC_UGML},
      {"_global_untyped", LSC_UGM},
      {"_global_typed", LSC_TGM},
      {"_local", LSC_SLM},
  };

  // c.f. reasoning in decodeAtomicOpFromName
  static_assert(sizeof(symbols) / sizeof(symbols[0]) == 4);

  StringRef FN = m_pCurrInstFunc->getName();
  LSC_SFID memPort = LSC_UGM;
  if (!findFirstInfixMapping<LSC_SFID, 4>(FN, symbols, memPort)) {
    reportError("invalid lsc SFID");
  }
  return memPort;
}

Constant *LSCFuncsResolution::getImmediateEnum(int i, int lo, int hi) {
  Value *v = m_pCurrInst->getOperand(i);
  if (ConstantInt *ci = dyn_cast<ConstantInt>(v)) {
    return ci;
  } else {
    std::stringstream ss;
    ss << "operand " << i << " must be immediate";
    reportError(ss.str().c_str());
    return getConstantInt32(lo); // use lo for the error value
  }
}

Constant *LSCFuncsResolution::getImmediateElementOffset(int i, LscTypeInfo ti) {
  Value *v = m_pCurrInst->getOperand(i);
  if (ConstantInt *ci = dyn_cast<ConstantInt>(v)) {
    int64_t scaledValue = ci->getSExtValue() * ti.sizeOfType;
    if (scaledValue < std::numeric_limits<int32_t>::min() || scaledValue > std::numeric_limits<int32_t>::max()) {
      // The vISA LSC API will emulate large offsets,
      // but is only int width
      reportError("scaled element offset too large");
      return getConstantInt32(0);
    }
    return getConstantInt32((int32_t)scaledValue);
  } else {
    reportError("element offset operand must be immediate");
    return getConstantInt32(0);
  }
}

Constant *LSCFuncsResolution::getCacheControlOpts(int i, bool isAtomic) {
  Constant *c = getImmediateEnum(i, LSC_L1DEF_L3DEF, LSC_L1IAR_WB_L3C_WB);

  if (isAtomic) {
    ConstantInt *ci = cast<ConstantInt>(c);
    switch (ci->getZExtValue()) {
    case LSC_L1DEF_L3DEF:
    case LSC_L1UC_L3UC:
    case LSC_L1UC_L3C_WB:
    case LSC_STCC_L1_L2_L3::LSC_STCC_L1_L2_L3_DEF:
    case LSC_STCC_L1_L2_L3::LSC_STCC_L1UC_L2UC_L3UC:
    case LSC_STCC_L1_L2_L3::LSC_STCC_L1UC_L2UC_L3WB:
    case LSC_STCC_L1_L2_L3::LSC_STCC_L1UC_L2WB_L3UC:
      break;
    default:
      reportError("atomic must not use caching on L1");
      c = getConstantInt32(LSC_L1DEF_L3DEF);
    }
  }

  return c;
}

void LSCFuncsResolution::reportError(const char *what) {
  if (hasError())
    m_ErrorMsg << "\n";
  const DebugLoc &loc = m_pCurrInst->getDebugLoc();
  if (loc)
    m_ErrorMsg << "line " << loc.getLine() << ": ";
  m_ErrorMsg << m_pCurrInstFunc->getName().str() << ": " << what;
}

FunctionPass *IGC::createLSCFuncsResolutionPass() { return new LSCFuncsResolution(); }
