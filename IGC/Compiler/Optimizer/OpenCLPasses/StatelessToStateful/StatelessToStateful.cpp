/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/IGCPassSupport.h"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "Compiler/Optimizer/CodeAssumption.hpp"
#include "Compiler/Optimizer/OpenCLPasses/StatelessToStateful/StatelessToStateful.hpp"
#include "common/Stats.hpp"
#include "common/secure_string.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/Transforms/Utils/Local.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/Alignment.h"
#include <string>
#include "Probe/Assertion.h"
#include "PointersSettings.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-stateless-to-stateful-resolution"
#define PASS_DESCRIPTION "Tries to convert stateless to stateful accesses"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(StatelessToStateful, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
IGC_INITIALIZE_PASS_END(StatelessToStateful, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

static cl::opt<TargetAddressing> targetAddressingMode(
    "target-addressing-mode", cl::init(TargetAddressing::BINDFUL), cl::Hidden,
    cl::values(clEnumValN(TargetAddressing::BINDFUL, "bindful", "Set bindful as target addressing mode"),
               clEnumValN(TargetAddressing::BINDLESS, "bindless", "Set bindless as target addressing mode")),
    cl::desc("Set target addressing for stateful promotion"));

// This pass turns a global/constants address space (stateless) load/store into a stateful a load/store.
//
// The conservative approach is to search for any directly positively-indexed kernels argument, such as:
//
// __kernel void CopyBuffer(__global uint4* dst, __global uint4* src)
// {
//     uint4 data = src[ get_global_id(0) ];
//     dst[ get_global_id(0) ] = data;
// }
//
// ...and turn these accesses into stateful accesses.
//
// This has a several benefits
//  - Stateful pointer size is always 32bit - we always know the base so the binding table is always known
//  - OBus bandwidth is reduced with pointer size reduction
//    - 32bit data type bandwidth increases by ~50%
//  - Pointer math overhead is reduced by 50% on 64bit systems
//  - UMD has ability to set cacheability control per surface instead of globally
//
// Limitations:
//   - This is not safe unless the UMD can guarantee allocations can fit in a surface state
//     - Linux platforms allow > 4GB  allocations.
//       An internal flag "-cl-intel-greater-than-4GB-buffer-required" is used to pass buffer size
//       info to the compiler. If 4GB buffer is required, this optimization is off.
//   - Does not work for 'system SVM' platforms without knowing extra information about the platform
//   - UMD needs checks to make sure this binary is never saved and later run on a system SVM device
//     - this is not done yet!
//
//  Negative offset
//    This optimization is carried out if the address offset can be proven to be positive. Unless the
//    compiler does a fancy check on this,  it turns out that proving a positive offset would fail most
//    of time, at least this is the case for the current implementation as of 6/1/2017. To overcome
//    this issue, BUFFER_OFFSET implicit kernel arguments are added. With this, the compiler does not
//    need to prove the offset is positive any more.
//
//    The negative offset can happen under the following conditions:
//       1. clSetKernelArgSVMPointer() is used to set a kernel argument
//          with "P + offset", where P is returned from clSVMAlloc()
//       2. Kernel does have negative offset relative to its argument,
//            kernel void test(global float* svmptr,...)
//            {
//                ......  *(svmptr - c) ...   // negative, but (offset + c) >= 0
//            }
//    The compiler needs to handle this even though it rarely happens.  Note that if the svm is
//    the system SVM, "p" can be returned by malloc(), in which we cannot guarantee the 4GB buffer size.
//    Thus, this optimization must be turned off by the runtime by passing the flag to the compiler:
//             -cl-intel-greater-than-4GB-buffer-required"
//
//    We handle this case by passing "offset" in "P + offset" to the kernel, so that compiler
//    will add this offset to the address computation. With the above example,
//         kernel void test(global float* svmptr, int32 svmptr_offset,....)
//             stateless:   address = svmptr - c
//             stateful:   offset = svmptr_offset - c
//    Note that offset will be in 32 bit integer,  either signed or unsigned, the final result
//    should be correct if the kernel's code does not have out-of-bound memory access (in this case,
//    the kernel code is wrong, and we don't really care what the wrong address will be.).
//
//    To implement this,  the compiler generates a new patch token (DATA_PARAMETER_BUFFER_OFFSET)
//    to the runtime, asking to pass an offset for a kernel pointer argument. (One token for one
//    offset, so, 5 offsets will have 5 tokens). AddImplicitArgs add those implicit arguments to
//    kernel.
//
//    - Flag and keys:
//      a new internal flag:  -cl-intel-has-buffer-offset-arg
//            This is needed as the classic ocl runtime does not need to support it. The presence of
//            this flag means BUFFER_OFFSET is supported.
//      Those three keys are for debugging purpose:
//        igc key: EnableStatelessToStateful --> to turn this optimization on/off.
//        igc key: EnableSupportBufferOffset
//                 this is the key version of -cl-intel-has-buffer-offset-arg.
//        igc key: SToSProducesPositivePointer
//                 To assume all offsets are positive (all BUFFER_OFFSET = 0). Thus, no need to
//                 have implicit BUFFER_OFFSET arguments at all.
//

// Future things to look out for:
//  - This transformation cannot be done if a pointer is stored to or loaded from memory
//    In general, if an address of load/store cannot be resolved to the kernel argument, the load/store
//    will still use stateless access. Note that the mix of stateless and stateful accesses is okay
//    in terms of correctness, and it is true even though cacheability is set.
//  - Need to watch out for a final address that less than the address of kernel argument:
//     example: kernelArg[-2]
//
//
// Possible Todos:
//  - Fancier back tracing to a kernel argument
//  - Handle > 2 operand GetElementPtr instructions // DONE!
//

char StatelessToStateful::ID = 0;

StatelessToStateful::StatelessToStateful() : ModulePass(ID), m_targetAddressing(targetAddressingMode) {}

StatelessToStateful::StatelessToStateful(TargetAddressing addressing) : ModulePass(ID), m_targetAddressing(addressing) {
  initializeStatelessToStatefulPass(*PassRegistry::getPassRegistry());
}

bool StatelessToStateful::runOnModule(llvm::Module &M) {
  m_Module = &M;

  if (m_targetAddressing == TargetAddressing::BINDFUL && getModuleUsesBindless() == true) {
    return false;
  }

  for (auto &it : M.functions()) {
    handleFunction(it);
  }

  return m_changed;
}

void StatelessToStateful::handleFunction(llvm::Function &F) {
  MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

  if (modMD->compOpt.OptDisable) {
    IGC_ASSERT_MESSAGE(0, "StatelessToStateful should be disabled for -O0!");
    return;
  }

  // skip non-entry functions
  if (!isEntryFunc(pMdUtils, &F)) {
    return;
  }

  m_F = &F;

  if (IGC_IS_FLAG_ENABLED(EnableCodeAssumption)) {
    // Use assumption cache
    m_ACT = &getAnalysis<AssumptionCacheTracker>();
    AssumptionCache &AC = m_ACT->getAssumptionCache(F);
    CodeAssumption::addAssumption(&F, &AC);
  } else {
    m_ACT = nullptr;
  }

  // Caching arguments during the transformation
  m_hasBufferOffsetArg = (IGC_IS_FLAG_ENABLED(EnableSupportBufferOffset) || modMD->compOpt.HasBufferOffsetArg);

  m_hasOptionalBufferOffsetArg = (m_hasBufferOffsetArg && (IGC_IS_FLAG_ENABLED(EnableOptionalBufferOffset) ||
                                                           modMD->compOpt.BufferOffsetArgOptional));

  m_hasPositivePointerOffset =
      (IGC_IS_FLAG_ENABLED(SToSProducesPositivePointer) || modMD->compOpt.HasPositivePointerOffset);

  m_pImplicitArgs = new ImplicitArgs(F, pMdUtils);
  m_ctx = static_cast<OpenCLProgramContext *>(getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  m_pKernelArgs = new KernelArgs(F, &(F.getParent()->getDataLayout()), pMdUtils, modMD, m_ctx->platform.getGRFSize());

  m_changed = hoistLoad();

  findPromotableInstructions();
  promote();

  finalizeArgInitialValue(&F);

  delete m_pImplicitArgs;
  delete m_pKernelArgs;
  m_promotionMap.clear();
}

bool StatelessToStateful::canWriteToMemoryTill(Instruction *Till) {
  BasicBlock *BB = Till->getParent();
  for (auto &I : *BB) {
    // Stop checking when we reach the PHINode
    if (&I == Till)
      break;

    if (I.mayWriteToMemory())
      return true;
  }

  return false;
}

/*
This function checks if it is safe to hoist the load instruction over the phi instruction.
It supports the following cases (BB3 contains load and phi instructions):

1)
  BB1  BB2
  |   /
   BB3
Here the function will check that there are no instructions that can write to memory in BB3 (from phi instruction
till load instruction).

2)
 BB1
 |  \
 |   BB2
 |  /
  BB3
Here the function will check that there are no instructions that can write to memory in the whole basic block BB2.
*/
bool StatelessToStateful::isItSafeToHoistLoad(LoadInst *LI, PHINode *Phi) {
  BasicBlock *LoadBB = LI->getParent();

  // Check that the load instruction is not carried over an instruction that can write to memory in basic block with phi
  // instruction.
  if (canWriteToMemoryTill(LI))
    return false;

  // Iterate over incoming basic blocks of the phi instruction.
  for (unsigned i = 0; i < Phi->getNumIncomingValues(); ++i) {
    BasicBlock *IncomingBlock = Phi->getIncomingBlock(i);

    // Skip incoming block if it leads to the load block only.
    if (IncomingBlock->getTerminator()->getNumSuccessors() == 1)
      continue;

    // Get the successor of the incoming block that is not the load block.
    BasicBlock *TheOtherSuccessor = nullptr;
    auto IncomingBlockTerminator = IncomingBlock->getTerminator();
    for (unsigned j = 0; j < IncomingBlockTerminator->getNumSuccessors(); ++j) {
      BasicBlock *Successor = IncomingBlockTerminator->getSuccessor(j);
      if (Successor != LoadBB) {
        TheOtherSuccessor = Successor;
        break;
      }
    }

    // Iterate over phi incoming values again to check that TheOtherSuccessor is incoming block of the phi instruction.
    bool isSuccessorIncoming = false;
    for (unsigned k = 0; k < Phi->getNumIncomingValues(); ++k) {
      BasicBlock *BBToCheck = Phi->getIncomingBlock(k);
      if (BBToCheck != TheOtherSuccessor)
        continue;

      isSuccessorIncoming = true;

      // Check that TheOtherSuccessor does not have any instruction that can write to memory.
      if (canWriteToMemoryTill(TheOtherSuccessor->getTerminator()))
        return false;
    }

    // Check that TheOtherSuccessor is incomimg block of the phi instruction.
    if (!isSuccessorIncoming)
      return false;
  }

  return true;
}

bool StatelessToStateful::hoistLoad() {
  bool Changed = false;
  std::vector<std::tuple<PHINode *, LoadInst *>> Container;
  Container.reserve(128);

  // Collect all load instructions that can be hoisted.
  for (auto &BB : *m_F) {
    for (auto &I : BB) {
      auto *LI = dyn_cast<LoadInst>(&I);
      if (!LI)
        continue;

      Value *Addr = LI->getPointerOperand();
      if (!Addr)
        continue;

      // StatelessToStateful works only with global and constant address space pointers.
      PointerType *PtrType = cast<PointerType>(Addr->getType());
      if (!PtrType || (PtrType->getAddressSpace() != ADDRESS_SPACE_GLOBAL &&
                       PtrType->getAddressSpace() != ADDRESS_SPACE_CONSTANT)) {
        continue;
      }

      // Sometimes result of phi instruction can be casted before load instruction.
      PHINode *Phi = nullptr;
      if (isa<PHINode>(Addr)) {
        Phi = cast<PHINode>(Addr);
      } else if (auto *BCI = dyn_cast<BitCastInst>(Addr)) {
        if (!BCI->hasOneUser())
          continue;

        if (isa<PHINode>(BCI->getOperand(0)))
          Phi = cast<PHINode>(BCI->getOperand(0));
      }

      // Check that PHINode and load instructions are in the same basic block
      if (!Phi || Phi->getParent() != &BB)
        continue;

      if (!Phi->hasOneUser())
        continue;

      // Check that the instruction is not carried over an instruction that can write to memory.
      if (isItSafeToHoistLoad(LI, Phi))
        Container.push_back(std::make_tuple(Phi, LI));
    }
  }

  // Iterate over phi nodes and load instructions.
  // Hoist the load instructions to the incoming BBs.
  // Create new phi instruction with the hoisted load instructions as incoming values.
  // Update uses of the original load instruction with the new phi instruction.
  // Remove the load instruction and the bitcast inst instruction (if it exists) from the original BB.
  for (auto &[Phi, LI] : Container) {
    IRBuilder<> Builder(Phi->getContext());
    Builder.SetInsertPoint(Phi);

    Type *LoadType = LI->getType();
    Align LoadAlign = LI->getAlign();
    PHINode *NewPhi = Builder.CreatePHI(LoadType, Phi->getNumIncomingValues(), "new_phi");

    // Iterate over phi incoming basic blocks.
    for (unsigned i = 0; i < Phi->getNumIncomingValues(); ++i) {
      Value *IncomingValue = Phi->getIncomingValue(i);
      BasicBlock *IncomingBlock = Phi->getIncomingBlock(i);
      Instruction *Terminator = IncomingBlock->getTerminator();

      Builder.SetInsertPoint(Terminator);
      Type *IncomingPtrType = IncomingValue->getType();
      PointerType *LoadPtrType = dyn_cast<PointerType>(LI->getPointerOperand()->getType());

      // Cast the incoming value to the type of the load if it is needed.
      Value *Cast = IncomingValue;
      if (IncomingPtrType != LoadPtrType)
        Cast = Builder.CreateBitCast(IncomingValue, LoadPtrType);

      LoadInst *NewLoad = Builder.CreateAlignedLoad(LoadType, Cast, LoadAlign, "hoisted_" + LI->getName());
      NewPhi->addIncoming(NewLoad, IncomingBlock);
    }

    // Erase all old instructions and update uses.
    LI->replaceAllUsesWith(NewPhi);
    Value *Addr = LI->getPointerOperand();
    LI->eraseFromParent();
    if (auto *BCI = dyn_cast<BitCastInst>(Addr))
      BCI->eraseFromParent();
    Phi->eraseFromParent();
  }

  return Changed;
}

Argument *StatelessToStateful::getBufferOffsetArg(Function *F, uint32_t ArgNumber) {
  uint32_t nImplicitArgs = m_pImplicitArgs->size();
  uint32_t totalArgs = (uint32_t)F->arg_size();
  uint32_t nExplicitArgs = (totalArgs - nImplicitArgs);
  uint32_t implicit_ix = m_pImplicitArgs->getNumberedArgIndex(ImplicitArg::BUFFER_OFFSET, ArgNumber);
  uint32_t arg_ix = nExplicitArgs + implicit_ix;
  Function::arg_iterator AI = F->arg_begin(), AE = F->arg_end();
  for (; AI != AE && AI->getArgNo() != arg_ix; ++AI)
    ;
  if (AI == AE) {
    IGC_ASSERT_MESSAGE(0, "Implicit arg for BUFFER_OFFSET is out of range!");
    return nullptr;
  }
  Argument *arg = &*AI;
  return arg;
}

//
// Convert GetElementPtrInst[s] into multiple instructions that compute the byte offset
// from the base represented by these GEP instructions. GEPs vector keeps its elements
// in the reverse order of execution, that is, the last element is the first GEP in the
// execution.
//
// Returns true if the GEP was able to be expanded to multiple instructions.
//
// The final instruction of the expansion is returned in 'offset'
//
bool StatelessToStateful::getOffsetFromGEP(Function *F, const SmallVector<GetElementPtrInst *, 4> &GEPs,
                                           uint32_t argNumber, bool isImplicitArg, Value *&offset) {
  Module *M = F->getParent();
  const DataLayout *DL = &M->getDataLayout();
  Type *int32Ty = Type::getInt32Ty(M->getContext());

  Value *PointerValue;
  // If m_hasPositivePointerOffset is true, BUFFER_OFFSET are assumed to be zero,
  // so is that for any implicit argument
  if (m_hasBufferOffsetArg && !isImplicitArg && !m_hasPositivePointerOffset) {
    PointerValue = getBufferOffsetArg(F, argNumber);
    if (PointerValue == nullptr) {
      // Sanity check
      return false;
    }
  } else {
    // BUFFER_OFFSET are zero.
    PointerValue = ConstantInt::get(int32Ty, 0);
  }

  const int nGEPs = GEPs.size();

  // GEPs is in the reverse order of execution! The last GEP is the first
  // one to execute.  For example:
  //    %37 = getelementptr inbounds float, float addrspace(1)* %signalw, i64 16384
  //    %38 = bitcast float addrspace(1)* %37 to[16 x[32 x[32 x float]]] addrspace(1)*
  //    %39 = getelementptr inbounds[16 x[32 x[32 x float]]], [16 x[32 x[32 x float]]]
  //                        addrspace(1)* %38, i64 0, i64 % 34, i64 % 17, i64 % 18
  //    store float %36, float addrspace(1)* %39, align 4
  //
  //  GEPs = [%39, %37]  // GEPs[0] = %39, GEPs[1] = %37
  //
  for (int i = nGEPs; i > 0; --i) {
    GetElementPtrInst *GEP = GEPs[i - 1];
    Value *PtrOp = GEP->getPointerOperand();
    [[maybe_unused]] PointerType *PtrTy = dyn_cast<PointerType>(PtrOp->getType());

    IGC_ASSERT_MESSAGE(PtrTy, "Only accept scalar pointer!");

    gep_type_iterator GTI = gep_type_begin(GEP);
    for (auto OI = GEP->op_begin() + 1, E = GEP->op_end(); OI != E; ++OI, ++GTI) {
      Value *Idx = *OI;
      if (StructType *StTy = GTI.getStructTypeOrNull()) {
        unsigned Field = int_cast<unsigned>(cast<ConstantInt>(Idx)->getZExtValue());
        if (Field) {
          uint64_t Offset = DL->getStructLayout(StTy)->getElementOffset(Field);

          Value *OffsetValue = ConstantInt::get(int32Ty, Offset);

          PointerValue = BinaryOperator::CreateAdd(PointerValue, OffsetValue, "", GEP);
          cast<llvm::Instruction>(PointerValue)->setDebugLoc(GEP->getDebugLoc());
        }
      } else {
        Type *Ty = GTI.getIndexedType();
        if (const ConstantInt *CI = dyn_cast<ConstantInt>(Idx)) {
          if (!CI->isZero()) {
            uint64_t Offset = DL->getTypeAllocSize(Ty) * CI->getSExtValue();
            Value *OffsetValue = ConstantInt::get(int32Ty, Offset);

            PointerValue = BinaryOperator::CreateAdd(PointerValue, OffsetValue, "", GEP);
            cast<llvm::Instruction>(PointerValue)->setDebugLoc(GEP->getDebugLoc());
          }
        } else {
          Value *NewIdx = CastInst::CreateTruncOrBitCast(Idx, int32Ty, "", GEP);
          cast<llvm::Instruction>(NewIdx)->setDebugLoc(GEP->getDebugLoc());

          APInt ElementSize = APInt((unsigned int)int32Ty->getPrimitiveSizeInBits(), DL->getTypeAllocSize(Ty));

          if (ElementSize != 1) {
            NewIdx = BinaryOperator::CreateMul(NewIdx, ConstantInt::get(int32Ty, ElementSize), "", GEP);
            cast<llvm::Instruction>(NewIdx)->setDebugLoc(GEP->getDebugLoc());
          }

          PointerValue = BinaryOperator::CreateAdd(PointerValue, NewIdx, "", GEP);
          cast<llvm::Instruction>(PointerValue)->setDebugLoc(GEP->getDebugLoc());
        }
      }
    }
  }
  offset = PointerValue;
  return true;
}

const KernelArg *StatelessToStateful::getKernelArgFromPtr(const PointerType &ptrType, Value *pVal) {
  if (pVal == nullptr)
    return nullptr;
  Value *base = pVal;

  // stripPointerCasts might skip addrSpaceCast, thus check if AS is still
  // the original one.
  unsigned int ptrAS = ptrType.getAddressSpace();
  if (cast<PointerType>(base->getType())->getAddressSpace() == ptrAS && !isa<Instruction>(base)) {
    if (const KernelArg *arg = getKernelArg(base))
      return arg;
  }
  return nullptr;
}

bool StatelessToStateful::pointerIsFromKernelArgument(Value &ptr) {
  // find the last gep
  Value *base = ptr.stripPointerCasts();
  // gep : the last gep of pointer address, null if no GEP at all.
  GetElementPtrInst *gep = nullptr;
  while (isa<GetElementPtrInst>(base)) {
    gep = static_cast<GetElementPtrInst *>(base);
    base = gep->getPointerOperand()->stripPointerCasts();
  }

  if (!m_supportNonGEPPtr && gep == nullptr)
    return false;

  if (getKernelArgFromPtr(*dyn_cast<PointerType>(ptr.getType()), base) != nullptr)
    return true;
  return false;
}

static Align determinePointerAlignmentTypedPointers(Value *ptr, const DataLayout &DL, AssumptionCache *AC,
                                                    Instruction *insertionPt) {
  Align BestAlign = Align(1);

  // 1) Examine uses: look for loads/stores (which may carry explicit
  //    alignment) or a GEP that reveals an ABI alignment from its element
  //    type.
  for (User *U : ptr->users()) {
    if (auto *LI = dyn_cast<LoadInst>(U)) {
      // Load has an explicit alignment.
      Align LdAlign = LI->getAlign();
      if (LdAlign > BestAlign)
        BestAlign = LdAlign;
    } else if (auto *SI = dyn_cast<StoreInst>(U)) {
      // Store sets alignment only if the pointer we store into is Ptr.
      if (SI->getPointerOperand() == ptr) {
        Align StAlign = SI->getAlign();
        if (StAlign > BestAlign)
          BestAlign = StAlign;
      }
    } else if (auto *GEP = dyn_cast<GetElementPtrInst>(U)) {
      // If the GEP's source element type is sized, we can guess an ABI
      // alignment.
      Type *BaseTy = GEP->getSourceElementType();
      if (BaseTy && BaseTy->isSized()) {
        Align GEPAlign = DL.getABITypeAlign(BaseTy);
        if (GEPAlign > BestAlign)
          BestAlign = GEPAlign;
      }
    }
  }

  // 2) If this pointer is actually a function parameter, see if it has an
  //    alignment attribute.
  if (auto *Arg = dyn_cast<Argument>(ptr)) {
    if (Arg->hasAttribute(llvm::Attribute::Alignment)) {
      if (MaybeAlign ArgAlign = Arg->getParamAlign()) {
        Align ArgAlignOrOne = ArgAlign.valueOrOne();
        if (ArgAlignOrOne > BestAlign)
          BestAlign = ArgAlignOrOne;
      }
    }
  }

  // 3) Fallback: use LLVM's built-in assumption-based alignment analysis
  //    (based on a.o. llvm.assume intrinsics).
  Align Known = getKnownAlignment(ptr, DL, insertionPt, AC);
  if (Known > BestAlign)
    BestAlign = Known;

  return BestAlign;
}

static bool determinePointerAlignment(const KernelArg *arg, Value *base, const DataLayout &DL, Value *V,
                                      AssumptionCache *AC, Instruction *insertionPt,
                                      std::optional<llvm::Align> originalInstructionAlignment) {
  if (arg->isImplicitArg())
    return true;

  auto desiredAlignmentLevel = 4;

  // The intent of getKnownAlignment below is to check if any llvm.assume intrinsic provides
  // a hint about the base pointer alignment
  Align knownAlignment = getKnownAlignment(base, DL, insertionPt, AC);

  if (knownAlignment >= desiredAlignmentLevel)
    return true;

  if (AreOpaquePointersEnabled()) {
    if (originalInstructionAlignment.has_value() &&
        IGC::isStatefulAddrSpace(base->getType()->getPointerAddressSpace())) {
      knownAlignment = originalInstructionAlignment.value();
    }
  } else {
    knownAlignment = determinePointerAlignmentTypedPointers(base, DL, AC, insertionPt);
  }

  return knownAlignment >= desiredAlignmentLevel;
}

bool StatelessToStateful::pointerIsPositiveOffsetFromKernelArgument(
    Function *F, Value *V, Value *&offset, unsigned int &argNumber, bool ignoreSyncBuffer,
    std::optional<llvm::Align> OriginalInstructionAlignment) {
  const DataLayout *DL = &F->getParent()->getDataLayout();

  AssumptionCache *AC = getAC(F);

  PointerType *ptrType = dyn_cast<PointerType>(V->getType());
  IGC_ASSERT_MESSAGE(ptrType, "Expected scalar Pointer (No support to vector of pointers");
  if (!ptrType ||
      (ptrType->getAddressSpace() != ADDRESS_SPACE_GLOBAL && ptrType->getAddressSpace() != ADDRESS_SPACE_CONSTANT)) {
    return false;
  }

  SmallVector<GetElementPtrInst *, 4> GEPs;
  Value *base = V->stripPointerCasts();
  // gep : the last gep of pointer address, null if no GEP at all.
  GetElementPtrInst *gep = nullptr;
  while (isa<GetElementPtrInst>(base)) {
    gep = static_cast<GetElementPtrInst *>(base);
    GEPs.push_back(gep);
    base = gep->getPointerOperand()->stripPointerCasts();
  }

  if (!m_supportNonGEPPtr && gep == nullptr) {
    return false;
  }

  // if the base is from kerenl argument
  if (const KernelArg *arg = getKernelArgFromPtr(*ptrType, base)) {
    // base is the argument!
    if (ignoreSyncBuffer && arg->getArgType() == KernelArg::ArgType::IMPLICIT_SYNC_BUFFER)
      return false;

    // skip implicit buffers that are supported only in stateless mode by the runtime.
    if (arg->getArgType() == KernelArg::ArgType::IMPLICIT_RT_GLOBAL_BUFFER ||
        arg->getArgType() == KernelArg::ArgType::IMPLICIT_INDIRECT_DATA_POINTER ||
        arg->getArgType() == KernelArg::ArgType::IMPLICIT_SCRATCH_POINTER ||
        arg->getArgType() == KernelArg::ArgType::IMPLICIT_ASSERT_BUFFER) {
      return false;
    }

    argNumber = arg->getAssociatedArgNo();
    bool gepProducesPositivePointer = true;

    // An address needs to be DW-aligned in order to be a base
    // in a surface state.  In another word, a unaligned argument
    // cannot be used as a surface base unless buffer_offset is
    // used, in which "argument + buffer_offset" is instead used
    // as a surface base. (argument + buffer_offset is the original
    // base of buffer created on host side, the original buffer is
    // guarantted to be DW-aligned.)
    //
    // Note that implicit arg is always aligned.
    auto insertionPt = F->getEntryBlock().getFirstNonPHI();
    bool isAlignedPointee = determinePointerAlignment(arg, base, *DL, V, AC, insertionPt, OriginalInstructionAlignment);

    // If m_hasBufferOffsetArg is true, the offset argument is added to
    // the final offset to make it definitely positive. Thus skip checking
    // if an offset is positive.
    //
    // Howerver, if m_hasoptionalBufferOffsetArg is true, the buffer offset
    // is not generated if all offsets can be proven positive (this has
    // performance benefit as adding buffer offset is an additional add).
    // Also, if an argument is unaligned, buffer offset must be ON and used;
    // otherwise, no stateful conversion for the argument can be carried out.
    //
    // Note that offset should be positive for any implicit ptr argument,
    // so no need to prove it!
    if (!arg->isImplicitArg() && isAlignedPointee && (!m_hasBufferOffsetArg || m_hasOptionalBufferOffsetArg) &&
        !m_hasPositivePointerOffset) {
      // This is for proving that the offset is positive.
      for (int i = 0, sz = GEPs.size(); i < sz; ++i) {
        GetElementPtrInst *tgep = GEPs[i];
        for (auto U = tgep->idx_begin(), E = tgep->idx_end(); U != E; ++U) {
          Value *Idx = U->get();
          gepProducesPositivePointer &= valueIsPositive(Idx, &(F->getParent()->getDataLayout()), AC);
        }
      }

      if (m_hasOptionalBufferOffsetArg) {
        updateArgInfo(arg, gepProducesPositivePointer);
      }
    }

    if ((m_hasBufferOffsetArg || (gepProducesPositivePointer && isAlignedPointee)) &&
        getOffsetFromGEP(F, GEPs, argNumber, arg->isImplicitArg(), offset)) {
      return true;
    }
  }

  return false;
}

bool StatelessToStateful::doPromoteUntypedAtomics(const GenISAIntrinsic::ID intrinID, const GenIntrinsicInst *Inst) {
  // Only promote if oprand0 and oprand1 are the same for 64bit-pointer atomics
  if (intrinID == GenISAIntrinsic::GenISA_intatomicrawA64 || intrinID == GenISAIntrinsic::GenISA_icmpxchgatomicrawA64 ||
      intrinID == GenISAIntrinsic::GenISA_floatatomicrawA64 ||
      intrinID == GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64) {
    if (Inst->getOperand(0) != Inst->getOperand(1)) {
      return false;
    }
  }

  // Qword untyped atomic int only support A64, so can't promote to stateful
  if (Inst->getType()->isIntegerTy() && Inst->getType()->getScalarSizeInBits() == 64) {
    return false;
  }

  return true;
}

bool StatelessToStateful::isUntypedAtomic(const GenISAIntrinsic::ID intrinID) {
  return (
      intrinID == GenISAIntrinsic::GenISA_intatomicraw || intrinID == GenISAIntrinsic::GenISA_floatatomicraw ||
      intrinID == GenISAIntrinsic::GenISA_intatomicrawA64 || intrinID == GenISAIntrinsic::GenISA_floatatomicrawA64 ||
      intrinID == GenISAIntrinsic::GenISA_icmpxchgatomicraw || intrinID == GenISAIntrinsic::GenISA_fcmpxchgatomicraw ||
      intrinID == GenISAIntrinsic::GenISA_icmpxchgatomicrawA64 ||
      intrinID == GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64);
}

unsigned StatelessToStateful::encodeBindfulAddrspace(unsigned uavIndex) {
  auto int32Ty = Type::getInt32Ty(m_Module->getContext());
  auto resourceNumber = ConstantInt::get(int32Ty, uavIndex);

  unsigned addrSpace = EncodeAS4GFXResource(*resourceNumber, BufferType::UAV);
  setPointerSizeTo32bit(addrSpace, m_Module);

  return addrSpace;
}

void StatelessToStateful::promoteIntrinsic(InstructionInfo &II) {
  GenIntrinsicInst *I = cast<GenIntrinsicInst>(II.statelessInst);
  Module *M = m_F->getParent();
  const DebugLoc &DL = I->getDebugLoc();
  GenISAIntrinsic::ID const intrinID = I->getIntrinsicID();
  PointerType *pTy = IGCLLVM::get(dyn_cast<PointerType>(II.ptr->getType()), II.getStatefulAddrSpace());

  if (m_targetAddressing == TargetAddressing::BINDLESS) {
    Argument *srcOffset =
        m_pImplicitArgs->getNumberedImplicitArg(*m_F, ImplicitArg::BINDLESS_OFFSET, II.getBaseArgIndex());
    auto newBasePtr = IntToPtrInst::Create(Instruction::IntToPtr, srcOffset, pTy, "", I);
    if (intrinID == GenISAIntrinsic::GenISA_simdBlockRead) {
      Function *newBlockReadFunc =
          GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_simdBlockReadBindless,
                                          {I->getType(), newBasePtr->getType(), Type::getInt32Ty(M->getContext())});
      Instruction *newBlockRead = CallInst::Create(newBlockReadFunc, {newBasePtr, II.offset}, "", I);
      newBlockRead->setDebugLoc(I->getDebugLoc());
      I->replaceAllUsesWith(newBlockRead);
      I->eraseFromParent();

      setModuleUsesBindless();
    } else if (intrinID == GenISAIntrinsic::GenISA_simdBlockWrite) {
      Function *newBlockWriteFunc = GenISAIntrinsic::getDeclaration(
          M, GenISAIntrinsic::GenISA_simdBlockWriteBindless,
          {newBasePtr->getType(), I->getOperand(1)->getType(), Type::getInt32Ty(M->getContext())});
      Instruction *newBlockWrite =
          CallInst::Create(newBlockWriteFunc, {newBasePtr, I->getOperand(1), II.offset}, "", I);
      newBlockWrite->setDebugLoc(I->getDebugLoc());
      I->replaceAllUsesWith(newBlockWrite);
      I->eraseFromParent();

      setModuleUsesBindless();
    }
    return;
  }

  IGC_ASSERT(m_targetAddressing == TargetAddressing::BINDFUL);

  Instruction *statefulPtr = IntToPtrInst::Create(Instruction::IntToPtr, II.offset, pTy, "", I);
  Instruction *statefulInst = nullptr;

  if (intrinID == GenISAIntrinsic::GenISA_simdBlockRead) {
    Function *simdMediaBlockReadFunc = GenISAIntrinsic::getDeclaration(M, intrinID, {I->getType(), pTy});
    statefulInst = CallInst::Create(simdMediaBlockReadFunc, {statefulPtr}, "", I);
  } else if (intrinID == GenISAIntrinsic::GenISA_simdBlockWrite ||
             intrinID == GenISAIntrinsic::GenISA_HDCuncompressedwrite) {
    SmallVector<Value *, 2> args;
    args.push_back(statefulPtr);
    args.push_back(I->getOperand(1));
    Function *pFunc = GenISAIntrinsic::getDeclaration(M, intrinID, {pTy, I->getOperand(1)->getType()});
    statefulInst = CallInst::Create(pFunc, args, "", I);
  } else if (intrinID == GenISAIntrinsic::GenISA_LSCStoreCmask) {
    SmallVector<Value *, 6> args;
    args.push_back(statefulPtr);
    args.push_back(I->getOperand(1));
    args.push_back(I->getOperand(2));
    args.push_back(I->getOperand(3));
    args.push_back(I->getOperand(4));
    args.push_back(I->getOperand(5));
    Function *pFunc = GenISAIntrinsic::getDeclaration(M, intrinID, {pTy, I->getOperand(2)->getType()});
    statefulInst = CallInst::Create(pFunc, args, "", I);
  } else if (intrinID == GenISAIntrinsic::GenISA_LSCLoadCmask) {
    Function *pCurrInstFunc = I->getCalledFunction();
    SmallVector<Value *, 5> args;
    args.push_back(statefulPtr);
    args.push_back(I->getOperand(1));
    args.push_back(I->getOperand(2));
    args.push_back(I->getOperand(3));
    args.push_back(I->getOperand(4));
    Function *pFunc = GenISAIntrinsic::getDeclaration(M, intrinID, {pCurrInstFunc->getReturnType(), pTy});
    statefulInst = CallInst::Create(pFunc, args, "", I);
  } else if (isUntypedAtomic(intrinID)) {
    if (intrinID == GenISAIntrinsic::GenISA_intatomicrawA64 ||
        intrinID == GenISAIntrinsic::GenISA_icmpxchgatomicrawA64 ||
        intrinID == GenISAIntrinsic::GenISA_floatatomicrawA64 ||
        intrinID == GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64) {
      statefulInst = CallInst::Create(GenISAIntrinsic::getDeclaration(M, intrinID, {I->getType(), pTy, pTy}),
                                      {statefulPtr, statefulPtr, I->getOperand(2), I->getOperand(3)}, "", I);
    } else {
      statefulInst = CallInst::Create(GenISAIntrinsic::getDeclaration(M, intrinID, {I->getType(), pTy}),
                                      {statefulPtr, II.offset, I->getOperand(2), I->getOperand(3)}, "", I);
    }
  }

  IGC_ASSERT(statefulInst);
  statefulInst->setDebugLoc(DL);
  I->replaceAllUsesWith(statefulInst);
  I->eraseFromParent();
}

void StatelessToStateful::promoteLoad(InstructionInfo &II) {
  LoadInst *I = cast<LoadInst>(II.statelessInst);
  PointerType *pTy = PointerType::get(I->getType(), II.getStatefulAddrSpace());

  const DebugLoc &DL = I->getDebugLoc();

  if (m_targetAddressing == TargetAddressing::BINDLESS) {
    Argument *srcOffset =
        m_pImplicitArgs->getNumberedImplicitArg(*m_F, ImplicitArg::BINDLESS_OFFSET, II.getBaseArgIndex());
    auto newBasePtr = IntToPtrInst::Create(Instruction::IntToPtr, srcOffset, pTy, "", I);
    auto bindlessLoad = IGC::CreateLoadRawIntrinsic(I, cast<Instruction>(newBasePtr), II.offset);

    newBasePtr->setDebugLoc(DL);
    bindlessLoad->setDebugLoc(DL);

    I->replaceAllUsesWith(bindlessLoad);
    I->eraseFromParent();
    setModuleUsesBindless();
  } else if (m_targetAddressing == TargetAddressing::BINDFUL) {
    auto newBasePtr = IntToPtrInst::Create(Instruction::IntToPtr, II.offset, pTy, "", I);
    auto bindfulLoad = new LoadInst(I->getType(), newBasePtr, "", I->isVolatile(), IGCLLVM::getAlign(*I),
                                    I->getOrdering(), I->getSyncScopeID(), I);

    newBasePtr->setDebugLoc(DL);
    bindfulLoad->setDebugLoc(DL);

    Value *ptr = I->getPointerOperand();
    PointerType *ptrType = dyn_cast<PointerType>(ptr->getType());
    if (ptrType && ptrType->getAddressSpace() == ADDRESS_SPACE_CONSTANT) {
      LLVMContext &context = I->getContext();
      MDString *const metadataName = MDString::get(context, "invariant.load");
      MDNode *node = MDNode::get(context, metadataName);
      bindfulLoad->setMetadata(LLVMContext::MD_invariant_load, node);
    }

    I->replaceAllUsesWith(bindfulLoad);
    I->eraseFromParent();
  } else {
    IGC_ASSERT_MESSAGE(false, "Unsupported addressing!");
  }
}

void StatelessToStateful::promoteStore(InstructionInfo &II) {
  StoreInst *I = cast<StoreInst>(II.statelessInst);

  Value *dataVal = I->getValueOperand();
  PointerType *pTy = PointerType::get(dataVal->getType(), II.getStatefulAddrSpace());

  const DebugLoc &DL = I->getDebugLoc();

  if (m_targetAddressing == TargetAddressing::BINDLESS) {
    Argument *srcOffset =
        m_pImplicitArgs->getNumberedImplicitArg(*m_F, ImplicitArg::BINDLESS_OFFSET, II.getBaseArgIndex());
    auto newBasePtr = IntToPtrInst::Create(Instruction::IntToPtr, srcOffset, pTy, "", I);
    auto bindlessStore = IGC::CreateStoreRawIntrinsic(I, cast<Instruction>(newBasePtr), II.offset);

    newBasePtr->setDebugLoc(DL);
    bindlessStore->setDebugLoc(DL);

    I->eraseFromParent();
    setModuleUsesBindless();
  } else if (m_targetAddressing == TargetAddressing::BINDFUL) {
    auto newBasePtr = IntToPtrInst::Create(Instruction::IntToPtr, II.offset, pTy, "", I);
    auto bindfulStore = new StoreInst(dataVal, newBasePtr, I->isVolatile(), IGCLLVM::getAlign(*I), I->getOrdering(),
                                      I->getSyncScopeID(), I);

    newBasePtr->setDebugLoc(DL);
    bindfulStore->setDebugLoc(DL);

    I->eraseFromParent();
  } else {
    IGC_ASSERT_MESSAGE(false, "Unsupported addressing!");
  }
}

void StatelessToStateful::promoteInstruction(StatelessToStateful::InstructionInfo &InstInfo) {
  switch (InstInfo.statelessInst->getOpcode()) {
  case Instruction::Load:
    promoteLoad(InstInfo);
    break;
  case Instruction::Store:
    promoteStore(InstInfo);
    break;
  case Instruction::Call:
    promoteIntrinsic(InstInfo);
    break;
  default:
    IGC_ASSERT_MESSAGE(false, "Unsupported instruction!");
  }
}

void StatelessToStateful::promote() {
  if (m_promotionMap.empty())
    return;

  ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  FunctionMetaData *funcMD = &modMD->FuncMD[m_F];
  ResourceAllocMD *resAllocMD = &funcMD->resAllocMD;
  IGC_ASSERT_MESSAGE(resAllocMD->argAllocMDList.size() > 0, "ArgAllocMDList is empty.");

  unsigned bufferPos = 0;
  for (auto &[baseArgIndex, instsToPromote] : m_promotionMap) {
    IGC_ASSERT(bufferPos < maxPromotionCount);

    unsigned statefullAddrspace = 0;
    if (m_targetAddressing == TargetAddressing::BINDLESS) {
      statefullAddrspace =
          IGC::EncodeAS4GFXResource(*UndefValue::get(Type::getInt32Ty(m_Module->getContext())), IGC::BINDLESS);
      setPointerSizeTo32bit(statefullAddrspace, m_Module);
      setModuleUsesBindless();
    } else {
      ArgAllocMD *argAlloc = &resAllocMD->argAllocMDList[baseArgIndex];
      argAlloc->type = ResourceTypeEnum::UAVResourceType;
      argAlloc->indexType = resAllocMD->uavsNumType + bufferPos;
      statefullAddrspace = encodeBindfulAddrspace(argAlloc->indexType);
    }

    for (auto &instInfo : instsToPromote) {
      instInfo.setStatefulAddrspace(statefullAddrspace);
      instInfo.setBaseArgIndex(baseArgIndex);
      promoteInstruction(instInfo);
    }
    bufferPos++;
    m_changed = true;
  }

  resAllocMD->uavsNumType += m_promotionMap.size();
}

void StatelessToStateful::addToPromotionMap(Instruction &I, Value *Ptr,
                                            std::optional<llvm::Align> OriginalInstructionAlignment = std::nullopt) {
  Value *offset = nullptr;
  unsigned baseArgNumber = 0;

  bool isPromotable =
      m_promotionMap.size() < maxPromotionCount &&
      pointerIsPositiveOffsetFromKernelArgument(m_F, Ptr, offset, baseArgNumber, true, OriginalInstructionAlignment);

  if (isPromotable) {
    InstructionInfo II(&I, Ptr, offset);
    m_promotionMap[baseArgNumber].push_back(II);
  }
}

void StatelessToStateful::visitCallInst(CallInst &I) {
  auto Inst = dyn_cast<GenIntrinsicInst>(&I);
  if (!Inst)
    return;

  GenISAIntrinsic::ID const intrinID = Inst->getIntrinsicID();

  if (intrinID == GenISAIntrinsic::GenISA_simdBlockRead || intrinID == GenISAIntrinsic::GenISA_simdBlockWrite ||
      intrinID == GenISAIntrinsic::GenISA_HDCuncompressedwrite ||
      (IGC_IS_FLAG_ENABLED(EnableStatefulAtomic) && isUntypedAtomic(intrinID) &&
       doPromoteUntypedAtomics(intrinID, Inst))) {
    Value *ptr = Inst->getOperand(0);
    PointerType *ptrTy = dyn_cast<PointerType>(ptr->getType());
    // If not global/constant, skip.
    if (ptrTy->getPointerAddressSpace() != ADDRESS_SPACE_GLOBAL &&
        ptrTy->getPointerAddressSpace() != ADDRESS_SPACE_CONSTANT) {
      return;
    }

    addToPromotionMap(I, ptr);
  } else if (intrinID == GenISAIntrinsic::GenISA_LSCLoadCmask || intrinID == GenISAIntrinsic::GenISA_LSCStoreCmask) {
    Value *ptr = Inst->getOperand(0);
    PointerType *ptrTy = dyn_cast<PointerType>(ptr->getType());
    // If not global, skip.
    if (ptrTy->getPointerAddressSpace() != ADDRESS_SPACE_GLOBAL) {
      return;
    }

    addToPromotionMap(I, ptr);
  }

  // check if there's non-kernel-arg load/store
  if (IGC_IS_FLAG_ENABLED(DumpHasNonKernelArgLdSt)) {
    // FIXME: should use the helper functions defined in Compiler/CISACodeGen/helper.h
    auto isLoadIntrinsic = [](const GenISAIntrinsic::ID id) {
      switch (id) {
      case GenISAIntrinsic::GenISA_simdBlockRead:
        // FIXME: GenISA_LSC2DBlockRead is not considered, not sure if its Operand 0
        // is the address
      case GenISAIntrinsic::GenISA_LSCLoad:
      case GenISAIntrinsic::GenISA_LSCLoadBlock:
      case GenISAIntrinsic::GenISA_LSCPrefetch:
      case GenISAIntrinsic::GenISA_LSCLoadCmask:
        return true;
      default:
        break;
      }
      return false;
    };
    auto isStoreIntrinsic = [](const GenISAIntrinsic::ID id) {
      switch (id) {
      case GenISAIntrinsic::GenISA_HDCuncompressedwrite:
      case GenISAIntrinsic::GenISA_LSCStore:
      case GenISAIntrinsic::GenISA_LSCStoreBlock:
      case GenISAIntrinsic::GenISA_simdBlockWrite:
      case GenISAIntrinsic::GenISA_LSCStoreCmask:
        return true;
      default:
        break;
      }
      return false;
    };
    auto isAtomicsIntrinsic = [&](const GenISAIntrinsic::ID id) {
      switch (id) {
      case GenISAIntrinsic::GenISA_LSCAtomicFP32:
      case GenISAIntrinsic::GenISA_LSCAtomicFP64:
      case GenISAIntrinsic::GenISA_LSCAtomicBF16:
      case GenISAIntrinsic::GenISA_LSCAtomicInts:
        return true;
      default:
        break;
      }
      return isUntypedAtomic(id);
    };
    if (isLoadIntrinsic(intrinID) || isStoreIntrinsic(intrinID) || isAtomicsIntrinsic(intrinID)) {
      Value *ptr = Inst->getOperand(0);
      if (!pointerIsFromKernelArgument(*ptr)) {
        ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
        FunctionMetaData *funcMD = &modMD->FuncMD[Inst->getParent()->getParent()];
        if (isStoreIntrinsic(intrinID))
          funcMD->hasNonKernelArgStore = true;
        else if (isLoadIntrinsic(intrinID))
          funcMD->hasNonKernelArgLoad = true;
        else
          funcMD->hasNonKernelArgAtomic = true;
      }
    }
  }
}

void StatelessToStateful::visitLoadInst(LoadInst &I) {
  Value *ptr = I.getPointerOperand();
  addToPromotionMap(I, ptr, I.getAlign());

  // check if there's non-kernel-arg load/store
  if (IGC_IS_FLAG_ENABLED(DumpHasNonKernelArgLdSt) && ptr != nullptr && !pointerIsFromKernelArgument(*ptr)) {
    ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    FunctionMetaData *funcMD = &modMD->FuncMD[m_F];
    funcMD->hasNonKernelArgLoad = true;
  }
}

void StatelessToStateful::visitStoreInst(StoreInst &I) {
  Value *ptr = I.getPointerOperand();
  addToPromotionMap(I, ptr, I.getAlign());

  if (IGC_IS_FLAG_ENABLED(DumpHasNonKernelArgLdSt) && ptr != nullptr && !pointerIsFromKernelArgument(*ptr)) {
    ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    FunctionMetaData *funcMD = &modMD->FuncMD[m_F];
    funcMD->hasNonKernelArgStore = true;
  }
}

void StatelessToStateful::findPromotableInstructions() {
  // fill m_promotionMap
  visit(m_F);
}

// This is used to set the size for a pointer to a given addrspace, which is created
// and used by and within IGC. As this is a new address space,  all the existing ones
// will not be affected by this at all.  (And it definitely does not change any existing
// memory layout.)
//
// Note this is consistent with CodeGenContext::getRegisterPointerSizeInBits() for now.
void StatelessToStateful::setPointerSizeTo32bit(int32_t AddrSpace, Module *M) {
  const DataLayout &DL = M->getDataLayout();

  // If default is 32bit (or it has been set to 32bit already), no need to set it.
  if (DL.getPointerSize(AddrSpace) == 4) {
    // Already 4 bytes,
    return;
  }

  const std::string StrDL = DL.getStringRepresentation();
  char data[64];
  if (DL.isDefault()) {
    sprintf_s(data, sizeof(data), "p%d:32:32:32", AddrSpace);
  } else {
    // this is a new addrspace, it should not be in the
    // existing DataLayout, but if it exists, just return.
    // We don't want to change any existing one!
    sprintf_s(data, sizeof(data), "p%d:", AddrSpace);
    if (StrDL.find(data) != std::string::npos) {
      return;
    }
    sprintf_s(data, sizeof(data), "-p%d:32:32:32", AddrSpace);
  }

  std::string newStrDL = StrDL + data;
  M->setDataLayout(newStrDL);
}

void StatelessToStateful::updateArgInfo(const KernelArg *kernelArg, bool isPositive) {
  auto II = m_argsInfo.find(kernelArg);
  if (II == m_argsInfo.end()) {
    m_argsInfo[kernelArg] = 1; // default to true
  }
  if (!isPositive) {
    m_argsInfo[kernelArg] = 0;
  }
}

void StatelessToStateful::finalizeArgInitialValue(Function *F) {
  if (!m_hasOptionalBufferOffsetArg) {
    return;
  }

  Module *M = F->getParent();
  Type *int32Ty = Type::getInt32Ty(M->getContext());
  Value *ZeroValue = ConstantInt::get(int32Ty, 0);

  for (const auto &II : m_argsInfo) {
    const KernelArg *kernelArg = II.first;
    int mapVal = II.second;
    bool allOffsetPositive = (mapVal == 1);
    if (allOffsetPositive) {
      const KernelArg *offsetArg = getBufferOffsetKernelArg(kernelArg);
      IGC_ASSERT_MESSAGE(offsetArg, "Missing BufferOffset arg!");
      Value *BufferOffsetArg = const_cast<Argument *>(offsetArg->getArg());
      BufferOffsetArg->replaceAllUsesWith(ZeroValue);
    }
  }

  m_argsInfo.clear();

  // Clear add instructions created in StatelessToStateful::getOffsetFromGEP
  DenseSet<Instruction *> AddInstructionsToLower;
  for (auto U : ZeroValue->users())
    if (auto I = dyn_cast<Instruction>(U))
      if (I->getOpcode() == Instruction::Add && I->getOperand(0) == ZeroValue)
        AddInstructionsToLower.insert(I);

  for (auto AddInst : AddInstructionsToLower) {
    AddInst->replaceAllUsesWith(AddInst->getOperand(1));
    AddInst->eraseFromParent();
  }
}

void StatelessToStateful::setModuleUsesBindless() {
  auto MD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  MD->ModuleUsesBindless = true;
  IGC::serialize(*MD, m_Module);
}

bool StatelessToStateful::getModuleUsesBindless() {
  return getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData()->ModuleUsesBindless;
}
