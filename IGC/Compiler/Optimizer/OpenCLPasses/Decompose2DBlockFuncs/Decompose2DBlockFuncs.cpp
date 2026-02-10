/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/Decompose2DBlockFuncs/Decompose2DBlockFuncs.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/Function.h>

#include <algorithm>

#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

#define DEBUG_TYPE "decompose-2d-block-funcs"

namespace {

// LSC 2D block address payload field names for updating only
// (block width/height/numBlock are not updated).
enum class BlockField : unsigned short {
  BASE = 1,
  WIDTH = 2,
  HEIGHT = 3,
  PITCH = 4,
  BLOCKX = 5,
  BLOCKY = 6,
  END = BLOCKY + 1,
  // Exclude the base offset
  // START = BASE,
  START = WIDTH,
};

BlockField &operator++(BlockField &Bf) {
  IGC_ASSERT_MESSAGE(Bf != BlockField::END, "Can't iterate past end of BlockField enum.");
  Bf = static_cast<BlockField>(static_cast<std::underlying_type<BlockField>::type>(Bf) + 1);
  return Bf;
}

/// @brief  IOBlock2DFuncsTranslation pass : translate IGC 2D block intrinsics
/// into several intrinsics which can be optimized further. More
/// specifically, this pass turns LSC2DBlock{Read, Write, Prefetch} into
/// 1. LSC2DBlockCreateAddrPayload: The payload initialization
/// 2. LSC2DBlockSetAddrPayloadField x 2: Set payload X and Y offsets
/// 3. LSC2DBlock{Read, Write, Prefetch}AddrPayload: Perform the
/// read/write/prefetch operation
class Decompose2DBlockFuncs : public FunctionPass, public InstVisitor<Decompose2DBlockFuncs> {
public:
  // Pass identification, replacement for typeid
  static char ID;

  Decompose2DBlockFuncs();

  /// @brief  Provides name of pass
  virtual StringRef getPassName() const override { return "Decompose2DBlockFuncs"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<MetaDataUtilsWrapper>();
    // AU.addRequired<ScalarEvolutionWrapperPass>();
  }

  virtual bool runOnFunction(Function &F) override;

  void visitCallInst(CallInst &CI);

  // Pair storing Value used as a payload argument, and the respective index it
  // should be stored in in the LSC2DBlockCreateAddrPayload intrinsic
  using PayloadParamType = std::pair<Value *, unsigned short>;
  // Maps BlockField type to the corresponding parameter
  using BFArray = std::array<PayloadParamType, static_cast<int>(BlockField::END)>;

private:
  ///////////////////////////////////////////////////////////////////////
  /// Helpers
  ///////////////////////////////////////////////////////////////////////

  //// Gets an i32 with a given value
  Constant *getConstantInt32(int Value, CallInst &CI) const {
    Type *i32{Type::getInt32Ty(CI.getContext())};
    return ConstantInt::get(i32, Value, true);
  }

  Constant *getConstantInt64(int Value, CallInst &CI) const {
    Type *i64{Type::getInt64Ty(CI.getContext())};
    return ConstantInt::get(i64, Value, true);
  }

  Constant *Zero32{nullptr};
  Constant *Zero64{nullptr};

  /// Indicates if the pass changed the processed function
  bool m_changed{};

  CodeGenContext *CGC{nullptr};
  const CPlatform *Platform{nullptr};
  LoopInfo *LI{nullptr};

  // Store the values of the IGC I/O builtins in this class
  class InputValues {
  public:
    Value *ImOffset;
    Value *ImWidth;
    Value *ImHeight;
    Value *ImPitch;
    Value *OffsetX;
    Value *OffsetY;
    Value *ElemSize;
    Value *TileWidth;
    Value *TileHeight;
    Value *VNumBlocks;
    Value *Transpose;
    Value *VNNITrans;
    Value *CacheControls;
    Value *Data;
    Value *IsAddend;

    InputValues() = default;

    InputValues(const GenIntrinsicInst &GII) {
      const auto GIISize{GII.arg_size()};
      IGC_ASSERT_MESSAGE(GIISize == 13 || GIISize == 14, "LSC blocks have 13 or 14 params.");
      ImOffset = GII.getArgOperand(0);
      ImWidth = GII.getArgOperand(1);
      ImHeight = GII.getArgOperand(2);
      ImPitch = GII.getArgOperand(3);
      OffsetX = GII.getArgOperand(4);
      OffsetY = GII.getArgOperand(5);
      ElemSize = GII.getArgOperand(6);
      TileWidth = GII.getArgOperand(7);
      TileHeight = GII.getArgOperand(8);
      VNumBlocks = GII.getArgOperand(9);
      Transpose = GII.getArgOperand(10);
      VNNITrans = GII.getArgOperand(11);
      CacheControls = GII.getArgOperand(12);
      if (GIISize == 14) {
        Data = GII.getArgOperand(13);
      }
      // FIXME: Support using addend in the future
      IsAddend = ConstantInt::get(Type::getInt1Ty(GII.getContext()), false);

      // Map each BlockField to the corresponding Value, and the index of the
      // corresponding argument of LSC2DBlockCreateAddrPayload
      Arr[static_cast<int>(BlockField::BASE)] = {ImOffset, 0};
      Arr[static_cast<int>(BlockField::WIDTH)] = {ImWidth, 1};
      Arr[static_cast<int>(BlockField::HEIGHT)] = {ImHeight, 2};
      Arr[static_cast<int>(BlockField::PITCH)] = {ImPitch, 3};
      Arr[static_cast<int>(BlockField::BLOCKX)] = {OffsetX, 4};
      Arr[static_cast<int>(BlockField::BLOCKY)] = {OffsetY, 5};
    }

    // Get the Value corresponding to the BlockField type
    Value *getVal(const BlockField &V) const { return Arr[static_cast<int>(V)].first; }

    // Given an BlockField type, get the index of the corresponding
    // parameter in the LSC2DBlockCreateAddrPayload intrinsic
    const unsigned short getPayloadIndex(const BlockField &V) const { return Arr[static_cast<int>(V)].second; }

  private:
    BFArray Arr;
  };

  CallBase *createPayload(GenIntrinsicInst &GII, const InputValues &IV, const SmallVector<Loop *, 4> &ParentLoops,
                          SmallSet<BlockField, 6> &SetIntrinsicsToCreate);

  CallInst *createSetAdd(GenIntrinsicInst &GII, const InputValues &IV, Instruction *Payload,
                         const BlockField &Field) const;

  Instruction *createPayloadIO(GenIntrinsicInst &GII, const InputValues &IV, Instruction *Payload,
                               const SmallSet<BlockField, 6> &SetIntrinsicsToCreate) const;

  const SmallVector<Loop *, 4> innerToOuterLoops(const BasicBlock &BB) const;

  // Map the arguments of each created payload to the payload instruction. This
  // is done to facilitate cloning of identical payloads to reduce overhead.
  std::map<std::array<Value *, 9>, CallBase *> CreatedPayloads;

  bool useImmediateOffset(const Value *V) const {
    // FIXME: determine when the immediate offset is enabled. The following
    // appears to work, but has not yet been rigorously tested. Use a flag to
    // enable
    if (IGC_GET_FLAG_VALUE(allowImmOff2DBlockFuncs)) {
      if (const ConstantInt *CI = dyn_cast<ConstantInt>(V)) {
        if (CI->getBitWidth() <= 32) {
          const auto N{CI->getSExtValue()};

          constexpr auto S10Min{-512};
          constexpr auto S10Max{511};
          return (N == 0 || (Platform->LSC2DSupportImmXY() && S10Min <= N && N <= S10Max));
        }
      }
    }
    return false;
  }

  template <BlockField F>
  Value *immOffsetValue(const InputValues &IV, const SmallSet<BlockField, 6> &SetIntrinsicsToCreate) const {
    Value *OffsetVal{IV.getVal(F)};
    if (useImmediateOffset(OffsetVal)) {
      LLVM_DEBUG(dbgs() << "Creating immediate offset for field " << static_cast<int>(F) << ".\n");
      IGC_ASSERT_MESSAGE(std::find(SetIntrinsicsToCreate.begin(), SetIntrinsicsToCreate.end(),
                                   F) != SetIntrinsicsToCreate.end(), // SetIntrinsicsToCreate.contains(F)
                         "We want to make an immediate offset in the IO intrinsic, but have "
                         "already set the offset in a LSC2DBlockSetAddrPayloadField "
                         "intrinsic.");
      return OffsetVal;
    } else {
      LLVM_DEBUG(dbgs() << "Not creating immediate offset for field " << static_cast<int>(F) << ".\n");
      return Zero32;
    }
  }
};
} // namespace

char Decompose2DBlockFuncs::ID{0};

// Register pass to igc-opt
#define PASS_FLAG "decompose-2d-block-funcs"
#define PASS_DESCRIPTION                                                                                               \
  "Decompose 2D block IO intrinsics into smaller chunks to increase "                                                  \
  "optimization opportunities"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(Decompose2DBlockFuncs, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(Decompose2DBlockFuncs, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

Decompose2DBlockFuncs::Decompose2DBlockFuncs() : FunctionPass(ID) {
  initializeDecompose2DBlockFuncsPass(*PassRegistry::getPassRegistry());
}

bool Decompose2DBlockFuncs::runOnFunction(Function &F) {
  LLVM_DEBUG(dbgs() << "Running " << getPassName() << "\n");
  CGC = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  Platform = &CGC->platform;
  LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

  m_changed = false;

  visit(F);

  return m_changed;
}

CallBase *Decompose2DBlockFuncs::createPayload(GenIntrinsicInst &GII, const InputValues &IV,
                                               const SmallVector<Loop *, 4> &ParentLoops,
                                               SmallSet<BlockField, 6> &SetIntrinsicsToCreate) {
  IGC_ASSERT_MESSAGE(SetIntrinsicsToCreate.empty(), "Set should be empty, so this func can populate it.");

  LLVM_DEBUG(dbgs() << "Creating payload.\n");
  std::array<Value *, 9> BlockCreateAddrPayloadArgs;
  // FIXME: In cases where these arguments are linear with a constant offset
  // with respect to the induction variable, it may be better to include the
  // constant term here

  // We currently don't want to have the ImOffset removed from the payload using
  // a LSC2DBlockSetAddrPayloadField intrinsic, since it sometimes causes
  // performance drops.
  BlockCreateAddrPayloadArgs[0] = IV.ImOffset;
  BlockCreateAddrPayloadArgs[1] = Zero32;
  BlockCreateAddrPayloadArgs[2] = Zero32;
  BlockCreateAddrPayloadArgs[3] = Zero32;
  BlockCreateAddrPayloadArgs[4] = Zero32;
  BlockCreateAddrPayloadArgs[5] = Zero32;
  BlockCreateAddrPayloadArgs[6] = IV.TileWidth;
  BlockCreateAddrPayloadArgs[7] = IV.TileHeight;
  BlockCreateAddrPayloadArgs[8] = IV.VNumBlocks;

  // Check that innermost loop with GII does not affect the parameters of the
  // payload
  for (auto &V : BlockCreateAddrPayloadArgs) {
    if (!ParentLoops[0]->isLoopInvariant(V)) {
      LLVM_DEBUG(dbgs() << "Early exit creation of payload, since there is a "
                           "loop-variant operation.\n");
      return nullptr;
    }
  }

  // We want to be able to hoist the payload as much as possible, with using
  // as few "LSC2DBlockSetAddrPayloadField" as possible. If we can set
  // loop-independent values in the payload that will let us avoid making more
  // intrinsics, we should do it.

  // Make sure we start with WIDTH, not with BASE.
  static_assert(BlockField::START == BlockField::WIDTH);
  for (auto BFType{BlockField::START}; BFType < BlockField::END; ++BFType) {
    Value *IVVal{IV.getVal(BFType)};
    const auto IVIndex{IV.getPayloadIndex(BFType)};
    // Always create set X and Y blocks, to increase CSE opportunities for the
    // payload for Triton
    const auto SkipBlock{BFType == BlockField::BLOCKX || BFType == BlockField::BLOCKY};

    if (ParentLoops[0]->isLoopInvariant(IVVal) && !SkipBlock) {
      // No need to create a LSC2DBlockSetAddrPayloadField for this operation
      BlockCreateAddrPayloadArgs[IVIndex] = IVVal;
    } else {
      LLVM_DEBUG(dbgs() << "Need to create LSC2DBlockSetAddrPayloadField of BFType " << static_cast<int>(BFType)
                        << "\n");
      SetIntrinsicsToCreate.insert(BFType);
    }
  }

  Function *BlockCreateFunc{GenISAIntrinsic::getDeclaration(GII.getCalledFunction()->getParent(),
                                                            GenISAIntrinsic::GenISA_LSC2DBlockCreateAddrPayload,
                                                            Type::getInt32Ty(GII.getContext())->getPointerTo())};

  // FIXME
  // We can only set this since we know with certainty that the associated
  // LSC2DBlockSetAddrPayloadField are not in the mode true : AP[arg1] += arg2
  // (i.e., arg3 = false)
  if (cast<llvm::ConstantInt>(IV.IsAddend)->isZero()) {
    BlockCreateFunc->addFnAttr(llvm::Attribute::Speculatable);
    IGCLLVM::setDoesNotAccessMemory(*BlockCreateFunc);
  }

  auto *PayloadInst{CallInst::Create(BlockCreateFunc, BlockCreateAddrPayloadArgs, "Block2D_AddrPayload", &GII)};

  // Save payload instruction for the arguments being considered
  CreatedPayloads[BlockCreateAddrPayloadArgs] = PayloadInst;

  return PayloadInst;
}

CallInst *Decompose2DBlockFuncs::createSetAdd(GenIntrinsicInst &GII, const InputValues &IV, Instruction *Payload,
                                              const BlockField &Field) const {
  LLVM_DEBUG(dbgs() << "Creating LSC2DBlockSetAddrPayloadField intrinsic with type " << static_cast<int>(Field)
                    << "\n");

  Value *OffsetVal{IV.getVal(Field)};

  if (useImmediateOffset(OffsetVal)) {
    LLVM_DEBUG(dbgs() << "Using immediate offset instead; aborting.\n");
    return nullptr;
  }

  std::array<Value *, 4> SetAddrPayloadArgs;
  SetAddrPayloadArgs[0] = Payload;
  SetAddrPayloadArgs[1] = ConstantInt::get(Type::getInt32Ty(GII.getContext()), static_cast<int>(Field));
  SetAddrPayloadArgs[2] = OffsetVal;
  SetAddrPayloadArgs[3] = IV.IsAddend;

  std::array Tys{Payload->getType(), OffsetVal->getType()};
  Function *BlockSetAddrFunc{GenISAIntrinsic::getDeclaration(
      GII.getCalledFunction()->getParent(), GenISAIntrinsic::GenISA_LSC2DBlockSetAddrPayloadField, Tys)};

  // We can only set this since we know with certainty that the associated
  // LSC2DBlockSetAddrPayloadField are not in the mode true : AP[arg1] += arg2
  // (i.e., arg3 = false)
  if (cast<llvm::ConstantInt>(IV.IsAddend)->isZero()) {
    BlockSetAddrFunc->addFnAttr(llvm::Attribute::Speculatable);
    IGCLLVM::setOnlyWritesMemory(*BlockSetAddrFunc);
  }

  return CallInst::Create(BlockSetAddrFunc, SetAddrPayloadArgs, "", &GII);
}

// Create read, write, prefetch
Instruction *Decompose2DBlockFuncs::createPayloadIO(GenIntrinsicInst &GII, const InputValues &IV, Instruction *Payload,
                                                    const SmallSet<BlockField, 6> &SetIntrinsicsToCreate) const {
  SmallVector<Value *, 10> BlockAddrPayloadArgs;
  BlockAddrPayloadArgs.push_back(Payload);
  BlockAddrPayloadArgs.push_back(immOffsetValue<BlockField::BLOCKX>(IV, SetIntrinsicsToCreate));
  BlockAddrPayloadArgs.push_back(immOffsetValue<BlockField::BLOCKY>(IV, SetIntrinsicsToCreate));
  BlockAddrPayloadArgs.push_back(IV.ElemSize);
  BlockAddrPayloadArgs.push_back(IV.TileWidth);
  BlockAddrPayloadArgs.push_back(IV.TileHeight);
  BlockAddrPayloadArgs.push_back(IV.VNumBlocks);
  BlockAddrPayloadArgs.push_back(IV.Transpose);
  BlockAddrPayloadArgs.push_back(IV.VNNITrans);
  BlockAddrPayloadArgs.push_back(IV.CacheControls);

  auto IT{GII.getIntrinsicID()};
  // Write intrinsic also takes a "data" parameter
  if (IT == GenISAIntrinsic::GenISA_LSC2DBlockWrite)
    BlockAddrPayloadArgs.push_back(IV.Data);

  CallInst *IOCallInst{};

  switch (IT) {
  case GenISAIntrinsic::GenISA_LSC2DBlockRead: {
    LLVM_DEBUG(dbgs() << "Creating read intrinsic\n");
    std::array<Type *, 2> Tys{GII.getType(), Type::getInt32Ty(GII.getContext())->getPointerTo()};
    Function *BlockAddrFunc{GenISAIntrinsic::getDeclaration(GII.getCalledFunction()->getParent(),
                                                            GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload, Tys)};
    IOCallInst = CallInst::Create(BlockAddrFunc, BlockAddrPayloadArgs, "Block2D_ReadAddrPayload", &GII);
    break;
  }
  case GenISAIntrinsic::GenISA_LSC2DBlockWrite: {
    LLVM_DEBUG(dbgs() << "Creating write intrinsic\n");
    std::array<Type *, 2> Tys{Payload->getType(), IV.Data->getType()};
    Function *BlockAddrFunc{GenISAIntrinsic::getDeclaration(GII.getCalledFunction()->getParent(),
                                                            GenISAIntrinsic::GenISA_LSC2DBlockWriteAddrPayload, Tys)};
    IOCallInst = CallInst::Create(BlockAddrFunc, BlockAddrPayloadArgs, "", &GII);
    break;
  }
  case GenISAIntrinsic::GenISA_LSC2DBlockPrefetch: {
    LLVM_DEBUG(dbgs() << "Creating prefetch intrinsic\n");
    Function *BlockAddrFunc{GenISAIntrinsic::getDeclaration(GII.getCalledFunction()->getParent(),
                                                            GenISAIntrinsic::GenISA_LSC2DBlockPrefetchAddrPayload,
                                                            Payload->getType())};
    IOCallInst = CallInst::Create(BlockAddrFunc, BlockAddrPayloadArgs, "", &GII);

    break;
  }
  default:
    llvm_unreachable("Not read, write, or prefetch!");
  }
  return IOCallInst;
}

const SmallVector<Loop *, 4> Decompose2DBlockFuncs::innerToOuterLoops(const BasicBlock &BB) const {
  SmallVector<Loop *, 4> Loops;
  Loop *ParentLoop{LI->getLoopFor(&BB)};

  while (ParentLoop != nullptr) {
    Loops.push_back(ParentLoop);
    ParentLoop = ParentLoop->getParentLoop();
  }

  return Loops;
}

void Decompose2DBlockFuncs::visitCallInst(CallInst &CI) {
  // LSC is not supported/enabled
  if (!Platform->hasLSC()) {
    LLVM_DEBUG(dbgs() << "LSC not supported on this platform.\n");
    return;
  }

  /// Process LCS intrinsics
  Function *CurrInstFunc{CI.getCalledFunction()};
  if (!CurrInstFunc)
    return;

  LLVM_DEBUG(dbgs() << "Encountered CI " << CurrInstFunc->getName() << ".\n");

  const auto Loops{innerToOuterLoops(*CI.getParent())};

  if (Loops.empty()) {
    LLVM_DEBUG(dbgs() << "CI not in loop.\n");
    return;
  }

  auto *GII{dyn_cast<GenIntrinsicInst>(&CI)};

  if (!GII) {
    LLVM_DEBUG(dbgs() << "CI not a GenIntrinsicInst.\n");
    return;
  }

  Zero32 = getConstantInt32(0, *GII);
  Zero64 = getConstantInt64(0, *GII);

  const auto ID{GII->getIntrinsicID()};

  // FIXME: Currently disabling expanding prefetch instructions, since there
  // are performance drops associated with it.
  if (ID != GenISAIntrinsic::GenISA_LSC2DBlockRead &&
      ID != GenISAIntrinsic::GenISA_LSC2DBlockWrite /*&&
      ID != GenISAIntrinsic::GenISA_LSC2DBlockPrefetch*/) {
    // Not a read, write, or prefetch command so there is nothing to do with
    // this GII
    LLVM_DEBUG(dbgs() << "GenIntrinsicInst not a read, write, or prefetch.\n");
    return;
  }

  // Grab the args of GII, add to IV
  InputValues IV(*GII);

  auto isEmulated = [&IV](GenIntrinsicInst *GII) {
    bool isRead = GII->getIntrinsicID() == GenISAIntrinsic::GenISA_LSC2DBlockRead;
    ConstantInt *TransposeVal = dyn_cast<ConstantInt>(IV.Transpose);
    bool isTranspose = (TransposeVal && !TransposeVal->isZero());
    ConstantInt *EltVal = dyn_cast<ConstantInt>(IV.ElemSize);
    uint32_t EltBits = EltVal ? (uint32_t)EltVal->getZExtValue() : 0;
    if (isRead && isTranspose && (EltBits == 8 || EltBits == 16)) {
      return true;
    }
    return false;
  };

  if (isEmulated(GII)) {
    // Due to separation of payload and read, skip emulated block read for now.
    LLVM_DEBUG(dbgs() << "Emulated d8/d16 transpose 2D block read, not decomposed.\n");
    return;
  }

  // All reads, writes or prefetches have a payload, so we can build one. If
  // the payload would be loop dependent, we should give up at this point. In
  // this case, we return a nullptr.
  SmallSet<BlockField, 6> SetIntrinsicsToCreate;
  CallBase *Payload{createPayload(*GII, IV, Loops, SetIntrinsicsToCreate)};

  if (!Payload) {
    LLVM_DEBUG(dbgs() << "Payload intrinsic would not be loop invariant; no "
                         "need to decompose original intrinsic.\n");
    return;
  }

  const DebugLoc &DL{GII->getDebugLoc()};
  Payload->setDebugLoc(DL);

  // Create LSC2DBlockSetAddrPayloadField which is neccesary to enable payload
  // hoisting
  for (const auto &SetIntrinsicTy : SetIntrinsicsToCreate) {
    CallInst *SetIntrinsCI{createSetAdd(*GII, IV, Payload, SetIntrinsicTy)};
    if (SetIntrinsCI)
      SetIntrinsCI->setDebugLoc(DL);
  }

  Instruction *IOInst{createPayloadIO(*GII, IV, Payload, SetIntrinsicsToCreate)};
  IOInst->setDebugLoc(DL);

  GII->replaceAllUsesWith(IOInst);
  GII->eraseFromParent();
  LLVM_DEBUG(dbgs() << "Done.\n");

  m_changed = true;
}

FunctionPass *IGC::createDecompose2DBlockFuncsPass() { return new Decompose2DBlockFuncs(); }
