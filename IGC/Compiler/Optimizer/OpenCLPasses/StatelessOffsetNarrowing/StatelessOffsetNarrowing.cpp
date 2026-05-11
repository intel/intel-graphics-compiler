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

    auto *Offset = getRawOffsetFromGEPs(GEPs);
    if (!Offset)
      continue;

    // Skip trivial case where the offset is constant zero.
    if (auto *CI = dyn_cast<ConstantInt>(Offset))
      if (CI->isZero())
        continue;

    const DebugLoc &DL = I->getDebugLoc();

    // Reconstruct the full 64-bit address just before the load/store:
    //   %base    = ptrtoint <base_arg> to i64
    //   %offset  = zext i32 <offset> to i64
    //   %address = add i64 %base, %offset
    //   %pointer = inttoptr i64 %address to <original Pointer type>
    auto *BaseI64 = new PtrToIntInst(Base, Type::getInt64Ty(M->getContext()), "base.i64", I);
    BaseI64->setDebugLoc(DL);

    auto *OffsetI64 = new ZExtInst(Offset, Type::getInt64Ty(M->getContext()), "offset.i64", I);
    OffsetI64->setDebugLoc(DL);

    auto *Address = BinaryOperator::CreateAdd(BaseI64, OffsetI64, "narrow.address", I);
    cast<Instruction>(Address)->setDebugLoc(DL);

    auto *NewPointer = new IntToPtrInst(Address, Pointer->getType(), "narrow.ptr", I);
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
      if (!computeKnownBits(Index.get(), DL, 0, this->CurrentAC).isNonNegative())
        return nullptr;

  return Result;
}

Value *StatelessOffsetNarrowing::getRawOffsetFromGEPs(const SmallVectorImpl<GetElementPtrInst *> &GEPs) {
  const auto *DL = &this->CurrentF->getParent()->getDataLayout();
  auto *Int32Ty = Type::getInt32Ty(this->CurrentF->getContext());
  Value *Result = ConstantInt::get(Int32Ty, 0);

  // Result += Imm * Var
  const auto AddToResult = [&Int32Ty, &Result](uint32_t Imm, Value *Var, GetElementPtrInst *GEP) {
    if (Imm == 0)
      return;

    IRBuilder<> Builder(GEP);
    Value *Offset = nullptr;
    if (!Var) {
      Offset = ConstantInt::get(Int32Ty, Imm);
    } else if (Imm == 1) {
      Offset = Var;
    } else {
      Offset = Builder.CreateMul(ConstantInt::get(Int32Ty, Imm), Var);
    }
    Result = Builder.CreateAdd(Result, Offset);
  };

  for (auto *GEP : llvm::reverse(GEPs)) {
    IGC_ASSERT_MESSAGE(isa<PointerType>(GEP->getPointerOperand()->getType()), "Only accept scalar pointer!");

    auto GTI = gep_type_begin(GEP);
    for (const auto &Index : llvm::drop_begin(GEP->operands())) {
      if (GTI.isStruct()) {
        uint32_t Field = cast<ConstantInt>(Index)->getZExtValue();
        uint32_t Offset = DL->getStructLayout(GTI.getStructType())->getElementOffset(Field);
        AddToResult(Offset, nullptr, GEP);
      } else {
        uint32_t TypeAllocSize = DL->getTypeAllocSize(GTI.getIndexedType());
        if (isa<ConstantInt>(Index)) {
          uint32_t Offset = TypeAllocSize * cast<ConstantInt>(Index)->getSExtValue();
          AddToResult(Offset, nullptr, GEP);
        } else {
          Instruction *IndexI32 = CastInst::CreateTruncOrBitCast(Index, Int32Ty, "", GEP);
          IndexI32->setDebugLoc(GEP->getDebugLoc());
          AddToResult(TypeAllocSize, IndexI32, GEP);
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
          KnownBits KB = computeKnownBits(Index, *DL, 0, this->CurrentAC);
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
