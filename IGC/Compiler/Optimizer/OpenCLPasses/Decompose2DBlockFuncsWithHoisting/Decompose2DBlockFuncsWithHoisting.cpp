/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/Decompose2DBlockFuncsWithHoisting/Decompose2DBlockFuncsWithHoisting.hpp"

#include <cstddef>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>

using namespace llvm;
using namespace IGC;

#define DEBUG_TYPE "decompose-2d-block-funcs-with-hoisting"

namespace {

/// @brief  IOBlock2DFuncsTranslation pass : translate IGC 2D block intrinsics
/// into several intrinsics which can be optimized further. More
/// specifically, this pass turns LSC2DBlock{Read, Write, Prefetch} into
/// 1. LSC2DBlockCreateAddrPayload: The payload initialization
/// 2. LSC2DBlockSetAddrPayloadField x 2: Set payload X and Y offsets
/// 3. LSC2DBlock{Read, Write, Prefetch}AddrPayload: Perform the
/// read/write/prefetch operation
class Decompose2DBlockFuncsWithHoisting : public FunctionPass, public InstVisitor<Decompose2DBlockFuncsWithHoisting> {
public:
  // Pass identification, replacement for typeid
  static char ID;

  Decompose2DBlockFuncsWithHoisting();

  /// @brief  Provides name of pass
  virtual StringRef getPassName() const override { return "Decompose2DBlockFuncsWithHoisting"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<llvm::LoopInfoWrapperPass>();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<llvm::ScalarEvolutionWrapperPass>();
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
  ScalarEvolution *SE{nullptr};
  const DataLayout *DL{nullptr};
  std::unique_ptr<SCEVExpander> E;
  const CPlatform *Platform{nullptr};
  LoopInfo *LI{nullptr};

  // Store the values of the IGC I/O builtins in this class
  class InputValues {
  public:
    Value *ImageOffset;
    Value *ImageWidth;
    Value *ImageHeight;
    Value *ImagePitch;
    Value *OffsetX;
    Value *OffsetY;
    Value *ElemSize;
    Value *TileWidth;
    Value *TileHeight;
    Value *VNumBlocks;
    Value *Transpose;
    Value *VNNITrans;
    Value *CacheControls;
    Value *Data = nullptr;
    Value *IsAddend;

    InputValues() = default;

    InputValues(const GenIntrinsicInst &GII) {
      const auto GIISize{GII.arg_size()};
      IGC_ASSERT_MESSAGE(GIISize == 13 || GIISize == 14, "LSC blocks have 13 or 14 params.");
      ImageOffset = GII.getArgOperand(0);
      ImageWidth = GII.getArgOperand(1);
      ImageHeight = GII.getArgOperand(2);
      ImagePitch = GII.getArgOperand(3);
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
      Arr[static_cast<int>(BlockField::BASE)] = {ImageOffset, 0};
      Arr[static_cast<int>(BlockField::WIDTH)] = {ImageWidth, 1};
      Arr[static_cast<int>(BlockField::HEIGHT)] = {ImageHeight, 2};
      Arr[static_cast<int>(BlockField::PITCH)] = {ImagePitch, 3};
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

  CallInst *createPayload(Instruction *PlaceToInsert, const PayloadArgsType &BlockCreateAddrPayloadArgs) const;

  void createSetAddrPayloadField(Instruction *PlaceToInsert, Value *OffsetVal, CallInst *Payload,
                                 const BlockField &Field, Value *isAddend) const;

  Instruction *createPayloadIO(GenIntrinsicInst &GII, const InputValues &IV, Instruction *Payload,
                               const std::map<BlockField, SetFieldSpec> &SetFieldsToCreate, Value *ImmOffsetX,
                               Value *ImmOffsetY) const;

  bool checkArg(Value *Val, Loop *L) const;

  /// Analyzes block X/Y value to deconstruct it into { Start, +, Step } form.
  /// Start can be hoisted to loop preheader for payload creation.
  /// Step can be passed as immediate offset to IO intrinsics.
  std::optional<SCEVUtils::DeconstructedSCEV> analyzeBlockXY(Value *XYVal, Loop *L);

  void collectPayloads(GenIntrinsicInst &GII, InputValues &IV, SmallVector<Loop *, 4> &ParentLoops);

  GenIntrinsicInst *checkLSCIntrinsic(CallInst *CI, InputValues &IV, SmallVector<Loop *, 4> &ParentLoops) const;

  void decompose();
  const SmallVector<Loop *, 4> innerToOuterLoops(const BasicBlock &BB) const;

  // Collect LSC2DBlock Read, Write, and Prefetch intrinsics, grouping them by:
  // 1) the insertion point for LSC2DBlockCreateAddrPayload intrinsic,
  // 2) the arguments for LSC2DBlockCreateAddrPayload,
  // 3) the set of LSC2DBlockSetAddrPayloadField intrinsics to be created.
  DecomposeInfoType CreatedPayloads;

  // Owns per-LSC metadata stored as raw pointers in CreatedPayloads.
  std::vector<std::unique_ptr<OldLSCAndRequiredSetFields>> OldLSCEntries;

  bool useImmediateOffset(const Value *V) const {
    // FIXME: determine when the immediate offset is enabled. The following
    // appears to work, but has not yet been rigorously tested. Use a flag to
    // enable
    if (IGC_GET_FLAG_VALUE(AllowImmOff2DBlockFuncsAddrHoisting)) {
      if (const ConstantInt *CI = dyn_cast<ConstantInt>(V)) {
        if (CI->getBitWidth() <= 32) {
          const auto N{CI->getSExtValue()};

          constexpr auto S10Min{-512};
          constexpr auto S10Max{511};
          // FIXME: Should check Platform->LSC2DSupportImmXY()
          bool res = (S10Min <= N && N <= S10Max) && N;
          return res;
        }
      }
    }

    return false;
  }

  template <BlockField F>
  Value *immOffsetValue(const InputValues &IV, const std::map<BlockField, SetFieldSpec> &SetFieldsToCreate,
                        Value *ImmOffset) const {
    if (!ImmOffset)
      return Zero32;

    if (useImmediateOffset(ImmOffset)) {
      LLVM_DEBUG(dbgs() << "Creating immediate offset for field " << static_cast<int>(F) << ".\n");
      return ImmOffset;
    } else {
      LLVM_DEBUG(dbgs() << "Not creating immediate offset for field " << static_cast<int>(F) << ".\n");
      return Zero32;
    }
  }
};
} // namespace

char Decompose2DBlockFuncsWithHoisting::ID{0};

// Register pass to igc-opt
#define PASS_FLAG "decompose-2d-block-funcs-with-hoisting"
#define PASS_DESCRIPTION                                                                                               \
  "Decompose 2D block IO intrinsics into smaller chunks to increase "                                                  \
  "optimization opportunities"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(Decompose2DBlockFuncsWithHoisting, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(Decompose2DBlockFuncsWithHoisting, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

Decompose2DBlockFuncsWithHoisting::Decompose2DBlockFuncsWithHoisting() : FunctionPass(ID) {
  initializeDecompose2DBlockFuncsWithHoistingPass(*PassRegistry::getPassRegistry());
}

bool Decompose2DBlockFuncsWithHoisting::runOnFunction(Function &F) {
  LLVM_DEBUG(dbgs() << "Running " << getPassName() << "\n");

  CreatedPayloads.clear();
  OldLSCEntries.clear();

  CGC = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  SE = &getAnalysis<ScalarEvolutionWrapperPass>().getSE();
  DL = &F.getParent()->getDataLayout();
  LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

  E = std::make_unique<SCEVExpander>(*SE, *DL, "decompose-2d-block-funcs-with-hoisting");
  Platform = &CGC->platform;

  m_changed = false;

  // LSC is not supported/enabled
  if (!Platform->hasLSC()) {
    LLVM_DEBUG(dbgs() << "LSC not supported on this platform.\n");
    return m_changed;
  }

  visit(F);

  decompose();

  return m_changed;
}

CallInst *Decompose2DBlockFuncsWithHoisting::createPayload(Instruction *PlaceToInsert,
                                                           const PayloadArgsType &BlockCreateAddrPayloadArgs) const {
  Function *BlockCreateFunc{
      GenISAIntrinsic::getDeclaration(PlaceToInsert->getModule(), GenISAIntrinsic::GenISA_LSC2DBlockCreateAddrPayload,
                                      Type::getInt32Ty(PlaceToInsert->getContext())->getPointerTo())};

  // FIXME
  // We can only set this since we know with certainty that the associated
  // LSC2DBlockSetAddrPayloadField are not in the mode true : AP[arg1] += arg2
  // (i.e., arg3 = false)
  // if (cast<llvm::ConstantInt>(IV.IsAddend)->isZero()) {
  //   BlockCreateFunc->addFnAttr(llvm::Attribute::Speculatable);
  //   IGCLLVM::setDoesNotAccessMemory(*BlockCreateFunc);
  // }

  CallInst *PayloadInst{
      CallInst::Create(BlockCreateFunc, BlockCreateAddrPayloadArgs, "Block2D_AddrPayload", PlaceToInsert)};

  return PayloadInst;
}

bool Decompose2DBlockFuncsWithHoisting::checkArg(Value *Val, Loop *L) const {
  // If Val is a constant or function argument, it's always safe to hoist
  if (isa<Constant>(Val) || isa<Argument>(Val))
    return true;

  // If Val is an instruction, check if it's defined outside the loop
  // (instructions defined outside the loop are inherently loop-invariant)
  if (Instruction *I = dyn_cast<Instruction>(Val)) {
    if (!L->contains(I->getParent()))
      return true;

    // For bitcasts inside the loop (e.g., bitcast <2 x i32> to i64),
    // check if the operand is loop-invariant. SCEV cannot analyze vector
    // bitcasts, but we can still hoist them if their operands are loop-invariant.
    // IMPORTANT: Only hoist if operand is OUTSIDE the loop, not just recursively
    // hoistable. This avoids the need to clone chains of instructions.
    if (BitCastInst *BC = dyn_cast<BitCastInst>(I)) {
      Value *Op = BC->getOperand(0);
      // Only allow hoisting if operand is constant, argument, or defined outside loop
      if (isa<Constant>(Op) || isa<Argument>(Op))
        return true;
      if (Instruction *OpInst = dyn_cast<Instruction>(Op)) {
        // Operand must be defined outside the loop (not inside)
        return !L->contains(OpInst->getParent());
      }
    }
  }

  // Otherwise, conservatively return false
  return false;
}

std::optional<SCEVUtils::DeconstructedSCEV> Decompose2DBlockFuncsWithHoisting::analyzeBlockXY(Value *XYVal, Loop *L) {
  if (!XYVal || !L || !L->getLoopPreheader() || !E)
    return std::nullopt;

  // Try to get the SCEV for XYVal
  const SCEV *S = SE->getSCEV(XYVal);

  // Check if SCEV is valid
  if (!SCEVUtils::isValidSCEV(S)) {
    return std::nullopt;
  }

  // Try to deconstruct the SCEV into { Start, +, Step } form
  SCEVUtils::DeconstructedSCEV Result;
  if (!SCEVUtils::deconstructSCEV(S, *SE, L, *E, Result))
    return std::nullopt;

  return Result;
}

void Decompose2DBlockFuncsWithHoisting::collectPayloads(GenIntrinsicInst &GII, InputValues &IV,
                                                        SmallVector<Loop *, 4> &ParentLoops) {

  LLVM_DEBUG(dbgs() << "Creating payload.\n");
  SCEVExpander *Expander = E.get();
  if (!Expander) {
    return;
  }

  PayloadArgsType BlockCreateAddrPayloadArgs;
  // FIXME: In cases where these arguments are linear with a constant offset
  // with respect to the induction variable, it may be better to include the
  // constant term here

  // We currently don't want to have the ImageOffset removed from the payload using
  // a LSC2DBlockSetAddrPayloadField intrinsic, since it sometimes causes
  // performance drops.
  BlockCreateAddrPayloadArgs[0] = IV.ImageOffset;
  BlockCreateAddrPayloadArgs[1] = Zero32;
  BlockCreateAddrPayloadArgs[2] = Zero32;
  BlockCreateAddrPayloadArgs[3] = Zero32;
  BlockCreateAddrPayloadArgs[4] = Zero32;
  BlockCreateAddrPayloadArgs[5] = Zero32;
  BlockCreateAddrPayloadArgs[6] = IV.TileWidth;
  BlockCreateAddrPayloadArgs[7] = IV.TileHeight;
  BlockCreateAddrPayloadArgs[8] = IV.VNumBlocks;

  // SCEV info for payload args that need to be expanded at preheader
  // Maps argIdx to DeconstructedSCEV for args that are not directly loop invariant but have zero step
  std::map<size_t, SCEVUtils::DeconstructedSCEV> PayloadArgsSCEVInfo;

  // Instructions inside loop that need to be hoisted to preheader
  // Maps argIdx to the instruction that needs cloning
  std::map<size_t, Instruction *> InstsToHoist;

  // Check if the innermost loop affects the parameters of the payload
  int FirstVariantLoopIdx = -1;

  Loop *InnermostLoop = ParentLoops[0];
  for (size_t argIdx = 0; argIdx < BlockCreateAddrPayloadArgs.size(); ++argIdx) {
    Value *V = BlockCreateAddrPayloadArgs[argIdx];

    // Check if value is loop-invariant or can be used directly in preheader
    if (InnermostLoop->isLoopInvariant(V))
      continue;

    // Check if it's an instruction inside loop that can be hoisted
    // (e.g., bitcast with loop-invariant operands)
    if (checkArg(V, InnermostLoop)) {
      // If checkArg returns true but isLoopInvariant returns false,
      // it means the instruction is inside the loop but can be hoisted
      if (Instruction *I = dyn_cast<Instruction>(V)) {
        if (InnermostLoop->contains(I->getParent())) {
          // Store instruction for hoisting in decompose()
          InstsToHoist[argIdx] = I;
        }
      }
      continue;
    }

    // Try SCEV analysis to check if it's effectively loop invariant (zero step)
    const SCEV *S = SE->getSCEV(V);
    if (SCEVUtils::isValidSCEV(S)) {
      SCEVUtils::DeconstructedSCEV DeconstructedResult;
      if (SCEVUtils::deconstructSCEV(S, *SE, InnermostLoop, *Expander, DeconstructedResult)) {
        // Check if step is zero (effectively loop invariant)
        if (DeconstructedResult.Step && DeconstructedResult.Step->isZero()) {
          // Will check isSafeToExpandAt after we determine the preheader
          SCEVUtils::DeconstructedSCEV Info;
          Info.set(DeconstructedResult.Start, DeconstructedResult.Step);
          PayloadArgsSCEVInfo[argIdx] = Info;
          continue; // Skip marking as variant, we'll handle via SCEV
        }
        return;
      }
    }
    FirstVariantLoopIdx = 0;
    break;
  }

  if (FirstVariantLoopIdx == 0) {
    LLVM_DEBUG(dbgs() << "All loops affect the payload; aborting payload creation.\n");
    return;
  }

  BasicBlock *Preheader = nullptr;
  if (FirstVariantLoopIdx == -1) {
    Preheader = ParentLoops[ParentLoops.size() - 1]->getLoopPreheader();
    FirstVariantLoopIdx = ParentLoops.size() - 1;
  } else {
    Preheader = ParentLoops[FirstVariantLoopIdx - 1]->getLoopPreheader();
  }

  if (!Preheader) {
    return;
  }

  Instruction *PlaceToInsertPayload = Preheader->getTerminator();

  // Verify all collected SCEVs are safe to expand at preheader and store them
  PayloadSCEVArgsType PayloadSCEVArgs;
  for (const auto &Entry : PayloadArgsSCEVInfo) {
    size_t argIdx = Entry.first;
    const SCEVUtils::DeconstructedSCEV &Info = Entry.second;

    if (!Info.isValid() || !Info.isStepZero())
      continue;

    bool IsSafeToExpand = IGCLLVM::isSafeToExpandAt(Info.getStart(), PlaceToInsertPayload, SE, Expander);
    if (IsSafeToExpand) {
      // Map argIdx to BlockField: argIdx 0 -> BASE(1), argIdx 1 -> WIDTH(2), etc.
      // Only indices 0-5 map to BlockField values
      if (argIdx <= 5) {
        BlockField Field = static_cast<BlockField>(argIdx + 1);
        PayloadSCEVArgs[Field] = Info.getStart();
        // Set placeholder value - will be replaced by SCEV expansion in decompose()
        // Index 0 (BASE/ImageOffset) is i64, others are i32
        BlockCreateAddrPayloadArgs[argIdx] = (argIdx == 0) ? Zero64 : Zero32;
      }
    } else {
      return;
    }
  }

  // We want to be able to hoist the payload as much as possible, with using
  // as few "LSC2DBlockSetAddrPayloadField" as possible. If we can set
  // loop-independent values in the payload that will let us avoid making more
  // intrinsics, we should do it.

  // Make sure we start with WIDTH, not with BASE.
  IGC_ASSERT(BlockField::START == BlockField::WIDTH);

  std::set<BlockField> UninitializedPayloadFields;
  for (auto BFType{BlockField::START}; BFType < BlockField::END; ++BFType) {
    Value *IVVal{IV.getVal(BFType)};

    // No need to create a LSC2DBlockSetAddrPayloadField if LSC2DBlockSetAddrPayloadField intrinsic generation
    // was forced for this field or it is a constant value.
    if (isa<Constant>(IVVal)) {
      const auto IVIndex{IV.getPayloadIndex(BFType)};
      BlockCreateAddrPayloadArgs[IVIndex] = IVVal;
      continue;
    }
    UninitializedPayloadFields.insert(BFType);
  }

  std::map<BlockField, SetFieldSpec> SetFieldsToCreate;
  std::map<BlockField, llvm::ConstantInt *> ImmOffsets;
  // Iterate over WIDTH = 2, HEIGHT = 3, PITCH = 4, BLOCKX = 5, BLOCKY = 6
  for (auto BFType : UninitializedPayloadFields) {
    Value *IVVal{IV.getVal(BFType)};

    Instruction *PlaceToInsertSetField = nullptr;
    int FirstVariantLoopIdxForField = -1;
    SCEVUtils::DeconstructedSCEV SCEVInfoPtr;
    SmallVector<Loop *, 4> LoopsToSave;
    // Use the innermost loop (index 0)
    Loop *InnermostLoopForField = ParentLoops[0];
    LLVM_DEBUG(dbgs() << "Need to create LSC2DBlockSetAddrPayloadField for "
                         "non-loop-invariant field "
                      << static_cast<int>(BFType) << "\n");

    LoopsToSave.push_back(InnermostLoopForField);
    PlaceToInsertSetField = &GII;

    auto DeconstructedXY = analyzeBlockXY(IVVal, InnermostLoopForField);
    if (!DeconstructedXY) {
      // SCEV analysis failed; fall back to creating a SetField with the original value
      SetFieldsToCreate.emplace(BFType, SetFieldSpec{IVVal, PlaceToInsertSetField, IV.IsAddend, SCEVInfoPtr,
                                                     FirstVariantLoopIdxForField, LoopsToSave});
      continue;
    }

    SCEVInfoPtr.set(DeconstructedXY->Start, DeconstructedXY->Step);

    if (!SCEVInfoPtr.isValid()) {
      // Invalid SCEV info; fall back to creating a SetField with the original value
      SetFieldsToCreate.emplace(BFType, SetFieldSpec{IVVal, PlaceToInsertSetField, IV.IsAddend, SCEVInfoPtr,
                                                     FirstVariantLoopIdxForField, LoopsToSave});
      continue;
    }

    if (!SCEVInfoPtr.isStepZero())
      FirstVariantLoopIdxForField = 0; // Innermost loop

    bool IsSafeToPromote = IGCLLVM::isSafeToExpandAt(SCEVInfoPtr.getStart(), PlaceToInsertPayload, SE, Expander);
    if (!IsSafeToPromote) {
      // Not safe to expand at preheader; fall back to creating a SetField with the original value
      SetFieldsToCreate.emplace(BFType, SetFieldSpec{IVVal, PlaceToInsertSetField, IV.IsAddend, SCEVInfoPtr,
                                                     FirstVariantLoopIdxForField, LoopsToSave});
      continue;
    }

    if (BFType == BlockField::BLOCKX || BFType == BlockField::BLOCKY) {
      bool IsOffset = false;

      // Helper lambda for type-safe SCEV subtraction.
      // SE->getMinusSCEV requires both operands to have the same type.
      // This helper extends the narrower operand to match the wider one.
      auto getMinusSCEVSafe = [this](const SCEV *LHS, const SCEV *RHS) -> const SCEV * {
        Type *LHSTy = LHS->getType();
        Type *RHSTy = RHS->getType();
        if (LHSTy != RHSTy) {
          Type *WiderTy = SE->getWiderType(LHSTy, RHSTy);
          if (LHSTy != WiderTy)
            LHS = SE->getSignExtendExpr(LHS, WiderTy);
          if (RHSTy != WiderTy)
            RHS = SE->getSignExtendExpr(RHS, WiderTy);
        }
        return SE->getMinusSCEV(LHS, RHS);
      };

      for (auto &Entry : CreatedPayloads) {
        const SCEV *OffsetForField = nullptr;
        if (SCEVInfoPtr.hasStep()) {
          const SCEV *LastSetFieldStartScev = nullptr;
          const SCEV *LastSetFieldStepScev = nullptr;
          for (auto RIt = Entry.second.rbegin(); RIt != Entry.second.rend(); ++RIt) {
            OldLSCAndRequiredSetFields *LSCEntry = *RIt;
            if (!LSCEntry)
              continue;

            auto ItSetField = LSCEntry->SetFields.find(BFType);
            if (ItSetField == LSCEntry->SetFields.end())
              continue;

            LastSetFieldStartScev = ItSetField->second.SCEVInfo.getStart();
            LastSetFieldStepScev = ItSetField->second.SCEVInfo.getStep();
            break;
          }

          if (!LastSetFieldStartScev || !LastSetFieldStepScev)
            continue;

          const SCEV *StartOffsetForField = getMinusSCEVSafe(SCEVInfoPtr.getStart(), LastSetFieldStartScev);
          const SCEV *StepOffsetForField = getMinusSCEVSafe(SCEVInfoPtr.getStep(), LastSetFieldStepScev);
          if (!isa<SCEVConstant>(StartOffsetForField) || !isa<SCEVConstant>(StepOffsetForField))
            continue;

          // Steps must be identical for a constant immediate offset.
          // A non-zero step difference means the offset between the two
          // intrinsics varies per iteration (offset = start_diff + k*step_diff)
          // and cannot be represented as a compile-time constant immediate.
          if (!cast<SCEVConstant>(StepOffsetForField)->getValue()->isZero())
            continue;
          OffsetForField = StartOffsetForField;
        } else {
          auto It = Entry.first.PayloadSCEVArgs.find(BFType);
          if (It == Entry.first.PayloadSCEVArgs.end())
            continue;
          OffsetForField = getMinusSCEVSafe(SCEVInfoPtr.getStart(), It->second);
        }

        if (!OffsetForField || !isa<SCEVConstant>(OffsetForField))
          continue;

        if (PlaceToInsertPayload != Entry.first.PayloadInsertBefore ||
            BlockCreateAddrPayloadArgs != Entry.first.PayloadArgs ||
            UninitializedPayloadFields != Entry.first.PayloadFieldsToUpdate)
          continue;

        ConstantInt *OffsetCI = cast<SCEVConstant>(OffsetForField)->getValue();

        // Check that OffsetCI is within the valid immediate offset range
        int64_t OffsetValue = OffsetCI->getSExtValue();
        if (OffsetValue < -512 || OffsetValue >= 512)
          continue;

        const SCEV *PayloadArgForField = nullptr;
        auto It = Entry.first.PayloadSCEVArgs.find(BFType);
        if (It != Entry.first.PayloadSCEVArgs.end())
          PayloadArgForField = It->second;

        IsOffset = true;
        if (!SCEVInfoPtr.hasStep()) {
          PayloadSCEVArgs[BFType] = PayloadArgForField;
        }
        ImmOffsets[BFType] = OffsetCI;
        PlaceToInsertSetField = nullptr;
        break;
      }

      if (!IsOffset) {
        const SCEV *StartSCEV = SCEVInfoPtr.getStart();
        if (!SCEVInfoPtr.hasStep()) {
          PayloadSCEVArgs[BFType] = StartSCEV;
        }
        ImmOffsets[BFType] = cast<ConstantInt>(Zero32);
      }
    } else {
      if (!SCEVInfoPtr.hasStep()) {
        const SCEV *StartSCEV = SCEVInfoPtr.getStart();
        PayloadSCEVArgs[BFType] = StartSCEV;
        PlaceToInsertSetField = nullptr;
      }
    }

    if (PlaceToInsertSetField) {
      SetFieldsToCreate.emplace(BFType, SetFieldSpec{IVVal, PlaceToInsertSetField, IV.IsAddend, SCEVInfoPtr,
                                                     FirstVariantLoopIdxForField, LoopsToSave});
    }
  }

  PayloadGroupKey Key;
  Key.PayloadInsertBefore = PlaceToInsertPayload;
  Key.PayloadArgs = BlockCreateAddrPayloadArgs;
  Key.PayloadFieldsToUpdate = UninitializedPayloadFields;
  Key.PayloadSCEVArgs = PayloadSCEVArgs;
  Key.InstsToHoist = InstsToHoist;

  auto LSCEntry = std::make_unique<OldLSCAndRequiredSetFields>();
  LSCEntry->LSC = &GII;
  LSCEntry->SetFields = std::move(SetFieldsToCreate);
  LSCEntry->ImmOffsets = std::move(ImmOffsets);

  // If CreatedPayloads already contains this key and has exactly one entry,
  // check if we can remove SetFields that conflict with ImmOffsets
  auto ExistingIt = CreatedPayloads.find(Key);

  if (ExistingIt != CreatedPayloads.end() && ExistingIt->second.size() == 1) {
    const LSCToDecomposeType &ExistingLSCs = ExistingIt->second;

    // Check each ImmOffset field in the new LSCEntry
    for (const auto &ImmOffsetEntry : LSCEntry->ImmOffsets) {
      BlockField Field = ImmOffsetEntry.first;

      // For each existing LSC entry in CreatedPayloads
      for (OldLSCAndRequiredSetFields *ExistingLSCEntry : ExistingLSCs) {
        if (!ExistingLSCEntry)
          continue;

        auto &ExistingSetFields = ExistingLSCEntry->SetFields;
        auto SetFieldIt = ExistingSetFields.find(Field);

        // If there is a conflicting SetField for this field, and its SCEVInfo indicates
        // that it is a step-zero (constant) value, we can remove it
        if (SetFieldIt != ExistingSetFields.end() && SetFieldIt->second.SCEVInfo.isStepZero())
          ExistingSetFields.erase(SetFieldIt);
      }
    }
  }

  // MapVector doesn't have try_emplace, use insert instead
  auto InsertResult = CreatedPayloads.insert(std::make_pair(std::move(Key), LSCToDecomposeType()));
  InsertResult.first->second.push_back(LSCEntry.get());
  OldLSCEntries.push_back(std::move(LSCEntry));
}

void Decompose2DBlockFuncsWithHoisting::createSetAddrPayloadField(Instruction *PlaceToInsert, Value *OffsetVal,
                                                                  CallInst *Payload, const BlockField &Field,
                                                                  Value *isAddend) const {
  LLVM_DEBUG(dbgs() << "Creating LSC2DBlockSetAddrPayloadField intrinsic with type " << static_cast<int>(Field)
                    << "\n");

  if (useImmediateOffset(OffsetVal)) {
    LLVM_DEBUG(dbgs() << "Using immediate offset instead; aborting.\n");
    return;
  }

  std::array<Value *, 4> SetAddrPayloadArgs;
  SetAddrPayloadArgs[0] = cast<Value>(Payload);
  SetAddrPayloadArgs[1] = ConstantInt::get(Type::getInt32Ty(PlaceToInsert->getContext()), static_cast<int>(Field));
  SetAddrPayloadArgs[2] = OffsetVal;
  SetAddrPayloadArgs[3] = isAddend;

  std::array Tys{Payload->getType(), OffsetVal->getType()};
  Function *BlockSetAddrFunc{GenISAIntrinsic::getDeclaration(
      PlaceToInsert->getModule(), GenISAIntrinsic::GenISA_LSC2DBlockSetAddrPayloadField, Tys)};

  // We can only set this since we know with certainty that the associated
  // LSC2DBlockSetAddrPayloadField are not in the mode true : AP[arg1] += arg2
  // (i.e., arg3 = false)
  if (cast<llvm::ConstantInt>(isAddend)->isZero()) {
    BlockSetAddrFunc->addFnAttr(llvm::Attribute::Speculatable);
    IGCLLVM::setOnlyWritesMemory(*BlockSetAddrFunc);
  }

  CallInst::Create(BlockSetAddrFunc, SetAddrPayloadArgs, "", PlaceToInsert);
}

// Create read, write, prefetch
Instruction *
Decompose2DBlockFuncsWithHoisting::createPayloadIO(GenIntrinsicInst &GII, const InputValues &IV, Instruction *Payload,
                                                   const std::map<BlockField, SetFieldSpec> &SetFieldsToCreate,
                                                   Value *ImmOffsetX, Value *ImmOffsetY) const {
  SmallVector<Value *, 10> BlockAddrPayloadArgs;
  BlockAddrPayloadArgs.push_back(Payload);

  // immOffsetValue returns Zero32 if IGC_GET_FLAG_VALUE(allowImmOff2DBlockFuncs) == false.
  // FIXME: Needs to be tested for IGC_GET_FLAG_VALUE(allowImmOff2DBlockFuncs) == true.
  BlockAddrPayloadArgs.push_back(immOffsetValue<BlockField::BLOCKX>(IV, SetFieldsToCreate, ImmOffsetX));
  BlockAddrPayloadArgs.push_back(immOffsetValue<BlockField::BLOCKY>(IV, SetFieldsToCreate, ImmOffsetY));
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

const SmallVector<Loop *, 4> Decompose2DBlockFuncsWithHoisting::innerToOuterLoops(const BasicBlock &BB) const {
  SmallVector<Loop *, 4> Loops;
  Loop *ParentLoop{LI->getLoopFor(&BB)};

  while (ParentLoop != nullptr) {
    Loops.push_back(ParentLoop);
    ParentLoop = ParentLoop->getParentLoop();
  }

  return Loops;
}

void Decompose2DBlockFuncsWithHoisting::visitCallInst(CallInst &CI) {
  /// Process LSC intrinsics
  SmallVector<Loop *, 4> Loops;
  InputValues IV;

  GenIntrinsicInst *GII = checkLSCIntrinsic(&CI, IV, Loops);
  if (!GII)
    return;

  Zero32 = getConstantInt32(0, CI);
  Zero64 = getConstantInt64(0, CI);

  // All reads, writes or prefetches have a payload, so we can build one. If
  // the payload would be loop dependent, we should give up at this point. In
  // this case, we return a nullptr.
  collectPayloads(*GII, IV, Loops);
}

GenIntrinsicInst *Decompose2DBlockFuncsWithHoisting::checkLSCIntrinsic(CallInst *CI, InputValues &IV,
                                                                       SmallVector<Loop *, 4> &ParentLoops) const {
  auto *GII = dyn_cast<GenIntrinsicInst>(CI);

  if (!GII) {
    LLVM_DEBUG(dbgs() << "CI not a GenIntrinsicInst.\n");
    return nullptr;
  }

  Function *CurrInstFunc{CI->getCalledFunction()};
  if (!CurrInstFunc)
    return nullptr;

  LLVM_DEBUG(dbgs() << "Encountered CI " << CurrInstFunc->getName() << ".\n");

  ParentLoops = innerToOuterLoops(*CI->getParent());

  if (ParentLoops.empty()) {
    LLVM_DEBUG(dbgs() << "CI not in loop.\n");
    return nullptr;
  }

  const auto ID = GII->getIntrinsicID();

  // Prefetch decomposition is gated behind AllowPrefetchDecomposeWithHoisting flag
  // since there were performance drops associated with it.
  bool AllowPrefetch = IGC_GET_FLAG_VALUE(AllowPrefetchDecomposeWithHoisting);
  if (ID != GenISAIntrinsic::GenISA_LSC2DBlockRead && ID != GenISAIntrinsic::GenISA_LSC2DBlockWrite &&
      !(AllowPrefetch && ID == GenISAIntrinsic::GenISA_LSC2DBlockPrefetch)) {
    // Not a read, write, or prefetch command so there is nothing to do with
    // this GII
    LLVM_DEBUG(dbgs() << "GenIntrinsicInst not a read, write, or prefetch.\n");
    return nullptr;
  }

  // Grab the args of GII, add to IV
  IV = InputValues(*GII);

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
    return nullptr;
  }

  return GII;
}

void Decompose2DBlockFuncsWithHoisting::decompose() {
  DenseMap<GenIntrinsicInst *, Instruction *> OldToNewInstMap;

  // Iterate over all elements in CreatedPayloads and save keys in variables
  for (auto &PayloadEntry : CreatedPayloads) {
    const PayloadGroupKey &Key = PayloadEntry.first;
    LSCToDecomposeType &OldLSCs = PayloadEntry.second;
    Instruction *PlaceToInsertPayload = Key.PayloadInsertBefore;
    PayloadArgsType BlockCreateAddrPayloadArgs = Key.PayloadArgs;

    // Expand SCEVs for payload creation if needed
    for (const auto &ScevEntry : Key.PayloadSCEVArgs) {
      BlockField Field = ScevEntry.first;
      const SCEV *Scev = ScevEntry.second;

      // BASE (ImageOffset) is i64, all other fields are i32
      Type *TargetType = (Field == BlockField::BASE) ? Type::getInt64Ty(Key.PayloadInsertBefore->getContext())
                                                     : Type::getInt32Ty(Key.PayloadInsertBefore->getContext());
      Value *Expanded = E->expandCodeFor(Scev, TargetType, Key.PayloadInsertBefore);
      BlockCreateAddrPayloadArgs[static_cast<int>(Field) - 1] = Expanded;
    }

    // Clone instructions that need to be hoisted from loop to preheader
    for (const auto &HoistEntry : Key.InstsToHoist) {
      size_t ArgIdx = HoistEntry.first;
      Instruction *OrigInst = HoistEntry.second;

      if (!OrigInst)
        continue;

      // Clone the instruction
      Instruction *ClonedInst = OrigInst->clone();

      // Insert cloned instruction before the payload creation point (in preheader)
      ClonedInst->insertBefore(PlaceToInsertPayload);

      // For instructions like bitcast, we need to ensure operands are available in preheader
      // The operands should already be loop-invariant (verified in checkArg)
      // No operand remapping needed since checkArg verified operands are loop-invariant

      // Update the payload args to use the cloned instruction
      BlockCreateAddrPayloadArgs[ArgIdx] = ClonedInst;
    }

    CallInst *Payload = createPayload(PlaceToInsertPayload, BlockCreateAddrPayloadArgs);

    for (OldLSCAndRequiredSetFields *OldLSCEntry : OldLSCs) {
      if (!OldLSCEntry || !OldLSCEntry->LSC)
        continue;

      GenIntrinsicInst *OldLSC = OldLSCEntry->LSC;
      for (const auto &SF : OldLSCEntry->SetFields) {
        const BlockField Field = SF.first;
        const SetFieldSpec &S = SF.second;
        Value *Val = S.Val;
        Instruction *PlaceToInsert = S.InsertBefore;

        if (!Val || !PlaceToInsert)
          continue;

        Value *IsAddend = S.IsAddend ? S.IsAddend : ConstantInt::get(Type::getInt1Ty(PlaceToInsert->getContext()), 0);
        createSetAddrPayloadField(PlaceToInsert, Val, Payload, Field, IsAddend);
      }

      Value *ImmOffsetX = nullptr;
      Value *ImmOffsetY = nullptr;
      {
        auto ItX = OldLSCEntry->ImmOffsets.find(BlockField::BLOCKX);
        if (ItX != OldLSCEntry->ImmOffsets.end())
          ImmOffsetX = ItX->second;

        auto ItY = OldLSCEntry->ImmOffsets.find(BlockField::BLOCKY);
        if (ItY != OldLSCEntry->ImmOffsets.end())
          ImmOffsetY = ItY->second;
      }

      InputValues IV = InputValues(*OldLSC);
      Instruction *NewIOLSC = createPayloadIO(*OldLSC, IV, Payload, OldLSCEntry->SetFields, ImmOffsetX, ImmOffsetY);

      if (OldLSC->getDebugLoc())
        NewIOLSC->setDebugLoc(OldLSC->getDebugLoc());

      OldToNewInstMap[OldLSC] = NewIOLSC;
    }
  }

  for (auto &OldLSC : OldToNewInstMap) {
    GenIntrinsicInst *GII = OldLSC.first;
    Instruction *NewIOLSC = OldLSC.second;

    // Replace all uses of the old LSC intrinsic with the new one
    GII->replaceAllUsesWith(NewIOLSC);
    GII->eraseFromParent();
  }

  if (!OldToNewInstMap.empty())
    m_changed = true;
}

FunctionPass *IGC::createDecompose2DBlockFuncsWithHoistingPass() { return new Decompose2DBlockFuncsWithHoisting(); }