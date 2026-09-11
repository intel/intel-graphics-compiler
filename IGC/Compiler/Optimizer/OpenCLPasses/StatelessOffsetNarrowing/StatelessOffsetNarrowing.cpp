/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/IGCPassSupport.h"
#include "Compiler/Optimizer/OpenCLPasses/StatelessOffsetNarrowing/StatelessOffsetNarrowing.hpp"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/Support/KnownBits.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Analysis/ValueTracking.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-stateless-offset-narrowing"
#define PASS_DESCRIPTION "Narrows 64-bit stateless pointer arithmetic to 32-bit offsets"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(StatelessOffsetNarrowing, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(StatelessOffsetNarrowing, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char StatelessOffsetNarrowing::ID = 0;

StatelessOffsetNarrowing::StatelessOffsetNarrowing() : FunctionPass(ID) {
  initializeStatelessOffsetNarrowingPass(*PassRegistry::getPassRegistry());
}

bool StatelessOffsetNarrowing::runOnFunction(Function &F) {
  if (!IGC_IS_FLAG_ENABLED(EnableStatelessOffsetNarrowing))
    return false;

  auto *ModuleMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  if (ModuleMD->compOpt.OptDisable)
    return false;

  auto *MDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  if (!isEntryFunc(MDUtils, &F))
    return false;

  bool Changed = false;

  this->CurrentF = &F;
  this->CurrentAC = IGC_IS_FLAG_ENABLED(EnableCodeAssumption)
                        ? &getAnalysis<AssumptionCacheTracker>().getAssumptionCache(*this->CurrentF)
                        : nullptr;
  this->HasPositivePointerOffset =
      IGC_IS_FLAG_ENABLED(SToSProducesPositivePointer) || ModuleMD->compOpt.HasPositivePointerOffset;

  auto *Context = static_cast<OpenCLProgramContext *>(getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  this->CurrentKernelArgs = new KernelArgs(*this->CurrentF, &(this->CurrentF->getParent()->getDataLayout()), MDUtils,
                                           ModuleMD, Context->platform.getGRFSize());

  // When GreaterThan4GBBufferRequired is false the runtime guarantees that all
  // buffers fit in 4GB, so every byte offset from a kernel-argument Base is
  // known to fit in unsigned 32 bits = no per-access analysis needed.
  // When it is true we fall back to per-access computeKnownBits analysis.
  bool OffsetGuaranteed32Bit = !ModuleMD->compOpt.GreaterThan4GBBufferRequired;

  // We will modify the use-lists so collect load/store instructions first.
  SmallVector<Instruction *, 16> LoadStoreInstructions;
  for (auto &I : llvm::instructions(this->CurrentF))
    if (isa<LoadInst, StoreInst>(&I))
      LoadStoreInstructions.push_back(&I);

  auto *M = this->CurrentF->getParent();
  for (auto *I : LoadStoreInstructions) {
    auto *Pointer = isa<LoadInst>(I) ? cast<LoadInst>(I)->getPointerOperand() : cast<StoreInst>(I)->getPointerOperand();
    SmallVector<GetElementPtrInst *, 4> GEPs;
    Value *Base = isNarrowableStatelessAccess(Pointer, GEPs);
    if (!Base)
      continue;

    // Fast path: runtime guarantees buffers < 4GB, so offset fits in u32.
    // Slow path: use computeKnownBits to prove per-access that the maximum
    // possible byte offset fits in u32.
    if (!OffsetGuaranteed32Bit && !offsetFitsIn32Bits(GEPs))
      continue;

    auto Offset = getRawOffsetFromGEPs(GEPs);

    // Nothing to narrow: no dynamic index, the rewrite would only add an immediate.
    if (!Offset.VariablePart)
      continue;

    const DebugLoc &DL = I->getDebugLoc();
    auto *Int64Ty = Type::getInt64Ty(M->getContext());

    // Splitting needs a non-negative variable part or the zext drops the borrow.
    // HasPositivePointerOffset only promises the total offset is non-negative.
    if (this->HasPositivePointerOffset && Offset.ConstantPart != 0 &&
        !IGCLLVM::computeKnownBits(Offset.VariablePart, M->getDataLayout(), this->CurrentAC).isNonNegative()) {
      auto *Int32Ty = Type::getInt32Ty(M->getContext());
      auto *ConstantI32 = ConstantInt::getSigned(Int32Ty, static_cast<int32_t>(Offset.ConstantPart));
      auto *Folded =
          BinaryOperator::CreateAdd(Offset.VariablePart, ConstantI32, "offset.const.i32", IGCLLVM::insertPosition(I));
      Folded->setDebugLoc(DL);
      Offset.VariablePart = Folded;
      Offset.ConstantPart = 0;
    }

    // Reconstruct the full 64-bit address just before the load/store:
    //   %base    = ptrtoint <base_arg> to i64
    //   %base    = add i64 %base, <constant part>   ; only when non-zero
    //   %offset  = zext i32 <variable part> to i64
    //   %address = add i64 %base, %offset
    //   %pointer = inttoptr i64 %address to <original Pointer type>
    Value *BaseI64 = new PtrToIntInst(Base, Int64Ty, "base.i64", IGCLLVM::insertPosition(I));
    cast<Instruction>(BaseI64)->setDebugLoc(DL);

    if (Offset.ConstantPart != 0) {
      BaseI64 = BinaryOperator::CreateAdd(BaseI64, ConstantInt::getSigned(Int64Ty, Offset.ConstantPart),
                                          "base.offset.i64", IGCLLVM::insertPosition(I));
      cast<Instruction>(BaseI64)->setDebugLoc(DL);
    }

    auto *OffsetI64 = new ZExtInst(Offset.VariablePart, Int64Ty, "offset.i64", IGCLLVM::insertPosition(I));
    OffsetI64->setDebugLoc(DL);

    auto *Address = BinaryOperator::CreateAdd(BaseI64, OffsetI64, "narrow.address", IGCLLVM::insertPosition(I));
    cast<Instruction>(Address)->setDebugLoc(DL);

    auto *NewPointer = new IntToPtrInst(Address, Pointer->getType(), "narrow.ptr", IGCLLVM::insertPosition(I));
    NewPointer->setDebugLoc(DL);

    // Replace the pointer operand of the load/store.
    if (auto *LI = dyn_cast<LoadInst>(I))
      LI->setOperand(LI->getPointerOperandIndex(), NewPointer);
    else if (auto *SI = dyn_cast<StoreInst>(I))
      SI->setOperand(SI->getPointerOperandIndex(), NewPointer);

    Changed = true;
  }

  delete this->CurrentKernelArgs;
  this->CurrentKernelArgs = nullptr;

  return Changed;
}

Value *StatelessOffsetNarrowing::isNarrowableStatelessAccess(Value *Pointer,
                                                             SmallVectorImpl<GetElementPtrInst *> &GEPs) {
  auto *PointerTy = cast<PointerType>(Pointer->getType());
  if (PointerTy->getAddressSpace() != ADDRESS_SPACE_GLOBAL && PointerTy->getAddressSpace() != ADDRESS_SPACE_CONSTANT)
    return nullptr;

  Value *Result = Pointer->stripPointerCasts();
  while (auto *GEP = dyn_cast<GetElementPtrInst>(Result)) {
    GEPs.push_back(GEP);
    Result = GEP->getPointerOperand()->stripPointerCasts();
  }

  if (GEPs.empty() || !getKernelArgFromPtr(*PointerTy, Result))
    return nullptr;

  if (this->HasPositivePointerOffset)
    return Result;

  // All GEP indices must be provably non-negative so that the total byte offset
  // fits in unsigned 32 bits.
  const auto &DL = this->CurrentF->getParent()->getDataLayout();
  for (auto *GEP : GEPs)
    for (const auto &Index : GEP->indices())
      if (!IGCLLVM::computeKnownBits(Index.get(), DL, this->CurrentAC).isNonNegative())
        return nullptr;

  return Result;
}

StatelessOffsetNarrowing::GEPChainOffset
StatelessOffsetNarrowing::getRawOffsetFromGEPs(const SmallVectorImpl<GetElementPtrInst *> &GEPs) {
  const auto *DL = &this->CurrentF->getParent()->getDataLayout();
  auto *Int32Ty = Type::getInt32Ty(this->CurrentF->getContext());
  GEPChainOffset Result;

  // Result.VariablePart += Imm * Var
  const auto AddToVariablePart = [&Int32Ty, &Result](uint64_t Imm, Value *Var, GetElementPtrInst *GEP) {
    IRBuilder<> Builder(GEP);
    Value *Offset = Imm == 1 ? Var : Builder.CreateMul(ConstantInt::get(Int32Ty, Imm), Var);
    Result.VariablePart = Result.VariablePart ? Builder.CreateAdd(Result.VariablePart, Offset) : Offset;
  };

  for (auto *GEP : llvm::reverse(GEPs)) {
    IGC_ASSERT_MESSAGE(isa<PointerType>(GEP->getPointerOperand()->getType()), "Only accept scalar pointer!");

    auto GTI = gep_type_begin(GEP);
    for (const auto &Index : llvm::drop_begin(GEP->operands())) {
      if (GTI.isStruct()) {
        uint32_t Field = cast<ConstantInt>(Index)->getZExtValue();
        Result.ConstantPart += static_cast<int64_t>(DL->getStructLayout(GTI.getStructType())->getElementOffset(Field));
      } else {
        uint64_t TypeAllocSize = DL->getTypeAllocSize(GTI.getIndexedType());
        if (auto *CI = dyn_cast<ConstantInt>(Index)) {
          Result.ConstantPart += static_cast<int64_t>(TypeAllocSize) * CI->getSExtValue();
        } else if (TypeAllocSize != 0) {
          Instruction *IndexI32 = CastInst::CreateTruncOrBitCast(Index, Int32Ty, "", IGCLLVM::insertPosition(GEP));
          IndexI32->setDebugLoc(GEP->getDebugLoc());
          AddToVariablePart(TypeAllocSize, IndexI32, GEP);
        }
      }
      ++GTI;
    }
  }

  return Result;
}

bool StatelessOffsetNarrowing::offsetFitsIn32Bits(const SmallVectorImpl<GetElementPtrInst *> &GEPs) {
  const auto *DL = &this->CurrentF->getParent()->getDataLayout();

  // Conservatively compute the maximum possible total byte offset by
  // summing the worst case contribution of each GEP index.
  uint64_t MaxTotalOffset = 0;
  for (const auto *GEP : llvm::reverse(GEPs)) {
    auto GTI = gep_type_begin(GEP);
    for (const auto &Index : llvm::drop_begin(GEP->operands())) {
      if (GTI.isStruct()) {
        uint32_t Field = cast<ConstantInt>(Index)->getZExtValue();
        MaxTotalOffset += DL->getStructLayout(GTI.getStructType())->getElementOffset(Field);
      } else {
        uint64_t TypeAllocSize = DL->getTypeAllocSize(GTI.getIndexedType());
        if (isa<ConstantInt>(Index)) {
          MaxTotalOffset += TypeAllocSize * cast<ConstantInt>(Index)->getSExtValue();
        } else {
          uint32_t IndexBitWidth = Index->getType()->getScalarSizeInBits();
          KnownBits KB = IGCLLVM::computeKnownBits(Index, *DL, this->CurrentAC);
          unsigned ActiveBits = IndexBitWidth - KB.countMinLeadingZeros();

          if (ActiveBits == 0)
            continue;

          if (ActiveBits > 32)
            return false;

          uint64_t MaxIndex = (1ULL << ActiveBits) - 1;
          if (TypeAllocSize != 0 && MaxIndex > UINT32_MAX / TypeAllocSize)
            return false;

          MaxTotalOffset += MaxIndex * TypeAllocSize;
        }
      }

      if (MaxTotalOffset > UINT32_MAX)
        return false;

      ++GTI;
    }
  }

  return true;
}

const KernelArg *StatelessOffsetNarrowing::getKernelArgFromPtr(const PointerType &PointerTy, Value *Base) {
  IGC_ASSERT_MESSAGE(CurrentKernelArgs, "Should initialize it before use!");

  if (!Base)
    return nullptr;

  if (cast<PointerType>(Base->getType())->getAddressSpace() != PointerTy.getAddressSpace() && !isa<Instruction>(Base))
    return nullptr;

  for (const KernelArg &Arg : *this->CurrentKernelArgs)
    if (Arg.getArg() == Base)
      return &Arg;

  return nullptr;
}
