/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/Decompose2DBlockFuncs/Decompose2DBlockFuncs.hpp"

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Pass.h>

#include <algorithm>
#include <limits>
#include <sstream>
#include <string>

#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "IGC/common/StringMacros.hpp"
#include "IGC/common/Types.hpp"
#include "Probe/Assertion.h"
#include "visa_igc_common_header.h"

using namespace llvm;
using namespace IGC;

#define DEBUG_TYPE "decompose-2d-block-funcs"

namespace {

/// @brief  IOBlock2DFuncsTranslation pass : translate IGC 2D block intrinsics
/// into several intrinsics which can be optimized further. More
/// specifically, this pass turns LSC2DBlock{Read, Write, Prefetch} into
/// 1. LSC2DBlockCreateAddrPayload: The payload initialization
/// 2. LSC2DBlockSetAddrPayloadField x 2: Set payload X and Y offsets
/// 3. LSC2DBlock{Read, Write, Prefetch}AddrPayload: Perform the
/// read/write/prefetch operation
class Decompose2DBlockFuncs : public FunctionPass,
                              public InstVisitor<Decompose2DBlockFuncs> {
 public:
  // Pass identification, replacement for typeid
  static char ID;

  Decompose2DBlockFuncs();

  /// @brief  Provides name of pass
  virtual StringRef getPassName() const override {
    return "Decompose2DBlockFuncs";
  }

  void getAnalysisUsage(AnalysisUsage& AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<MetaDataUtilsWrapper>();
    // AU.addRequired<ScalarEvolutionWrapperPass>();
  }

  virtual bool runOnFunction(Function& F) override;

  void visitCallInst(CallInst& CI);

 private:
  ///////////////////////////////////////////////////////////////////////
  /// Helpers
  ///////////////////////////////////////////////////////////////////////

  //// Gets an i32 with a given value
  Constant* getConstantInt32(int Value, CallInst& CI) const {
    Type* i32{Type::getInt32Ty(CI.getContext())};
    return ConstantInt::get(i32, Value, true);
  }

  /// Indicates if the pass changed the processed function
  bool m_changed{};

  CodeGenContext* CGC{};
  LoopInfo* LI{};

  // Store the values of the IGC I/O builtins in this class
  class InputValues {
   public:
    Value* ImOffset;
    Value* ImWidth;
    Value* ImHeight;
    Value* ImPitch;
    Value* OffsetX;
    Value* OffsetY;
    Value* ElemSize;
    Value* TileWidth;
    Value* TileHeight;
    Value* VNumBlocks;
    Value* Transpose;
    Value* VNNITrans;
    Value* CacheControls;
    Value* Data;
    Value* IsAddend;

    InputValues() = default;

    InputValues(const GenIntrinsicInst& GII) {
      const auto GIISize{GII.arg_size()};
      IGC_ASSERT_MESSAGE(GIISize == 13 || GIISize == 14,
                         "LSC blocks have 13 or 14 params.");
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
    }
  };

  CallBase* createPayload(GenIntrinsicInst& GII, const InputValues& IV,
                          const SmallVector<Loop*, 4>& ParentLoops) const;
  template <IGC::BlockField Field>
  CallInst* createSetAdd(GenIntrinsicInst& GII, const InputValues& IV,
                         Instruction* Payload) const;

  Instruction* createPayloadIO(GenIntrinsicInst& GII, const InputValues& IV,
                               Instruction* Payload) const;

  const SmallVector<Loop*, 4> innerToOuterLoops(const BasicBlock& BB) const;
};
}  // namespace

char Decompose2DBlockFuncs::ID{0};

// Register pass to igc-opt
#define PASS_FLAG "decompose-2d-block-funcs"
#define PASS_DESCRIPTION                                              \
  "Decompose 2D block IO intrinsics into smaller chunks to increase " \
  "optimization opportunities"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(Decompose2DBlockFuncs, PASS_FLAG, PASS_DESCRIPTION,
                          PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(Decompose2DBlockFuncs, PASS_FLAG, PASS_DESCRIPTION,
                        PASS_CFG_ONLY, PASS_ANALYSIS)

Decompose2DBlockFuncs::Decompose2DBlockFuncs() : FunctionPass(ID) {
  initializeDecompose2DBlockFuncsPass(*PassRegistry::getPassRegistry());
}

bool Decompose2DBlockFuncs::runOnFunction(Function& F) {
  LLVM_DEBUG(dbgs() << "Running " << getPassName() << "\n");
  CGC = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

  m_changed = false;

  visit(F);

  return m_changed;
}

CallBase* Decompose2DBlockFuncs::createPayload(
    GenIntrinsicInst& GII, const InputValues& IV,
    const SmallVector<Loop*, 4>& ParentLoops) const {
  LLVM_DEBUG(dbgs() << "Creating payload.\n");
  std::array<Value*, 9> BlockCreateAddrPayloadArgs;
  BlockCreateAddrPayloadArgs[0] = IV.ImOffset;
  BlockCreateAddrPayloadArgs[1] = IV.ImWidth;
  BlockCreateAddrPayloadArgs[2] = IV.ImHeight;
  BlockCreateAddrPayloadArgs[3] = IV.ImPitch;
  BlockCreateAddrPayloadArgs[4] =
      getConstantInt32(0, GII);  // FIXME: Could be better X offset
  BlockCreateAddrPayloadArgs[5] =
      getConstantInt32(0, GII);  // FIXME: Could be better Y offset
  BlockCreateAddrPayloadArgs[6] = IV.TileWidth;
  BlockCreateAddrPayloadArgs[7] = IV.TileHeight;
  BlockCreateAddrPayloadArgs[8] = IV.VNumBlocks;

  // Check that innermost loop with GII does not affect the parameters of the
  // payload
  for (auto V : BlockCreateAddrPayloadArgs) {
    if (!ParentLoops[0]->isLoopInvariant(V)) {
      LLVM_DEBUG(dbgs() << "Early exit creation of payload, since there is a "
                           "loop-variant operation.\n");
      return nullptr;
    }
  }

  // FIXME: Unify calls with identical parameters using the BlockCopyAddrPayload
  // intrinsic

  Function* BlockCreateFunc{GenISAIntrinsic::getDeclaration(
      GII.getCalledFunction()->getParent(),
      GenISAIntrinsic::GenISA_LSC2DBlockCreateAddrPayload,
      Type::getInt32Ty(GII.getContext())->getPointerTo())};

  // FIXME
  // We can only set this since we know with certainty that the associated
  // LSC2DBlockSetAddrPayloadField are not in the mode true : AP[arg1] += arg2
  // (i.e., arg3 = false)
  if (dyn_cast<llvm::ConstantInt>(IV.IsAddend)->isZero()) {
    BlockCreateFunc->removeFnAttr(llvm::Attribute::WriteOnly);
    BlockCreateFunc->removeFnAttr(llvm::Attribute::InaccessibleMemOnly);

    BlockCreateFunc->addFnAttr(
        llvm::Attribute::ReadNone);  // = setDoesNotAccessMemory();
  }

  return CallInst::Create(BlockCreateFunc, BlockCreateAddrPayloadArgs,
                          "Block2D_AddrPayload", &GII);
}

template <IGC::BlockField Field>
CallInst* Decompose2DBlockFuncs::createSetAdd(GenIntrinsicInst& GII,
                                              const InputValues& IV,
                                              Instruction* Payload) const {
  static_assert(Field == IGC::BlockField::BLOCKX ||
                Field == IGC::BlockField::BLOCKY);
  constexpr bool IsBlockX{Field == IGC::BlockField::BLOCKX};
  LLVM_DEBUG(
      dbgs() << "Creating LSC2DBlockSetAddrPayloadField intrinsic for block "
             << (IsBlockX ? "x" : "y") << "\n");

  Value* OffsetVal{nullptr};

  if constexpr (IsBlockX)
    OffsetVal = IV.OffsetX;
  else
    OffsetVal = IV.OffsetY;

  std::array<Value*, 4> SetAddrPayloadArgs;
  SetAddrPayloadArgs[0] = Payload;
  SetAddrPayloadArgs[1] = ConstantInt::get(Type::getInt32Ty(GII.getContext()),
                                           static_cast<int>(Field));
  SetAddrPayloadArgs[2] = OffsetVal;

  SetAddrPayloadArgs[3] = IV.IsAddend;

  std::array Tys{Payload->getType(), OffsetVal->getType()};
  Function* BlockSetAddrFunc{GenISAIntrinsic::getDeclaration(
      GII.getCalledFunction()->getParent(),
      GenISAIntrinsic::GenISA_LSC2DBlockSetAddrPayloadField, Tys)};

  // We can only set this since we know with certainty that the associated
  // LSC2DBlockSetAddrPayloadField are not in the mode true : AP[arg1] += arg2
  // (i.e., arg3 = false)
  if (dyn_cast<llvm::ConstantInt>(IV.IsAddend)->isZero()) {
    BlockSetAddrFunc->removeFnAttr(llvm::Attribute::ReadOnly);
    BlockSetAddrFunc->addFnAttr(
        llvm::Attribute::WriteOnly);  // = setOnlyWritesMemory();
  }

  return CallInst::Create(BlockSetAddrFunc, SetAddrPayloadArgs, "", &GII);
}

// Create read, write, prefetch
Instruction* Decompose2DBlockFuncs::createPayloadIO(
    GenIntrinsicInst& GII, const InputValues& IV, Instruction* Payload) const {
  SmallVector<Value*, 10> BlockAddrPayloadArgs;
  BlockAddrPayloadArgs.push_back(Payload);
  // FIXME: This can be more optimal by not always setting these to zero
  BlockAddrPayloadArgs.push_back(getConstantInt32(0, GII));
  BlockAddrPayloadArgs.push_back(getConstantInt32(0, GII));
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

  CallInst* IOCallInst{};

  switch (IT) {
    case GenISAIntrinsic::GenISA_LSC2DBlockRead: {
      LLVM_DEBUG(dbgs() << "Creating read intrinsic\n");
      std::array<Type*, 2> Tys{
          GII.getType(), Type::getInt32Ty(GII.getContext())->getPointerTo()};
      Function* BlockAddrFunc{GenISAIntrinsic::getDeclaration(
          GII.getCalledFunction()->getParent(),
          GenISAIntrinsic::GenISA_LSC2DBlockReadAddrPayload, Tys)};
      IOCallInst = CallInst::Create(BlockAddrFunc, BlockAddrPayloadArgs,
                                    "Block2D_ReadAddrPayload", &GII);
      break;
    }
    case GenISAIntrinsic::GenISA_LSC2DBlockWrite: {
      LLVM_DEBUG(dbgs() << "Creating write intrinsic\n");
      std::array<Type*, 2> Tys{Payload->getType(), IV.Data->getType()};
      Function* BlockAddrFunc{GenISAIntrinsic::getDeclaration(
          GII.getCalledFunction()->getParent(),
          GenISAIntrinsic::GenISA_LSC2DBlockWriteAddrPayload, Tys)};
      IOCallInst =
          CallInst::Create(BlockAddrFunc, BlockAddrPayloadArgs, "", &GII);
      break;
    }
    case GenISAIntrinsic::GenISA_LSC2DBlockPrefetch: {
      LLVM_DEBUG(dbgs() << "Creating prefetch intrinsic\n");
      Function* BlockAddrFunc{GenISAIntrinsic::getDeclaration(
          GII.getCalledFunction()->getParent(),
          GenISAIntrinsic::GenISA_LSC2DBlockPrefetchAddrPayload,
          Payload->getType())};
      IOCallInst =
          CallInst::Create(BlockAddrFunc, BlockAddrPayloadArgs, "", &GII);

      break;
    }
    default:
      llvm_unreachable("Not read, write, or prefetch!");
  }
  return IOCallInst;
}

const SmallVector<Loop*, 4> Decompose2DBlockFuncs::innerToOuterLoops(
    const BasicBlock& BB) const {
  SmallVector<Loop*, 4> Loops;
  Loop* ParentLoop{LI->getLoopFor(&BB)};

  while (ParentLoop != nullptr) {
    Loops.push_back(ParentLoop);
    ParentLoop = ParentLoop->getParentLoop();
  }

  return Loops;
}

void Decompose2DBlockFuncs::visitCallInst(CallInst& CI) {
  // LSC is not supported/enabled
  if (!CGC->platform.hasLSC()) {
    LLVM_DEBUG(dbgs() << "LSC not supported on this platform.\n");
    return;
  }

  /// Process LCS intrinsics
  Function* CurrInstFunc{CI.getCalledFunction()};
  if (!CurrInstFunc) return;

  LLVM_DEBUG(dbgs() << "Encountered CI " << CurrInstFunc->getName() << ".\n");

  const auto Loops{innerToOuterLoops(*CI.getParent())};

  if (Loops.empty()) {
    LLVM_DEBUG(dbgs() << "CI not in loop.\n");
    return;
  }

  auto* GII{dyn_cast<GenIntrinsicInst>(&CI)};

  if (!GII) {
    LLVM_DEBUG(dbgs() << "CI not a GenIntrinsicInst.\n");
    return;
  }

  auto ID{GII->getIntrinsicID()};

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

  // All reads, writes or prefetches have a payload, so we can build one. If the
  // payload would be loop dependent, we should give up at this point. In this
  // case, we return a nullptr.
  CallBase* Payload{createPayload(*GII, IV, Loops)};
  if (!Payload) {
    LLVM_DEBUG(dbgs() << "Payload intrinsic would not be loop invariant; no "
                         "need to decompose original intrinsic.\n");
    return;
  }

  CallInst* SetAddX{createSetAdd<IGC::BlockField::BLOCKX>(*GII, IV, Payload)};
  CallInst* SetAddY{createSetAdd<IGC::BlockField::BLOCKY>(*GII, IV, Payload)};
  Instruction* IOInst{createPayloadIO(*GII, IV, Payload)};

  const DebugLoc& DL{GII->getDebugLoc()};
  Payload->setDebugLoc(DL);
  if (SetAddX) SetAddX->setDebugLoc(DL);
  if (SetAddY) SetAddY->setDebugLoc(DL);
  IOInst->setDebugLoc(DL);

  GII->replaceAllUsesWith(IOInst);
  GII->eraseFromParent();
  LLVM_DEBUG(dbgs() << "Done.\n");

  m_changed = true;
}

FunctionPass* IGC::createDecompose2DBlockFuncsPass() {
  return new Decompose2DBlockFuncs();
}
