/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "AdaptorCommon/RayTracing/RTBuilder.h"
#include "Compiler/Optimizer/OpenCLPasses/PrivateMemory/PrivateMemoryResolution.hpp"
#include "Compiler/ModuleAllocaAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs/KernelArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/EmitVISAPass.hpp"
#include "Compiler/CISACodeGen/GenCodeGenModule.h"
#include "Compiler/CISACodeGen/LowerGEPForPrivMem.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Dominators.h"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/IRBuilder.h"
#include <llvmWrapper/ADT/Optional.h>
#include "Probe/Assertion.h"

#include <optional>
#include <utility>

using namespace llvm;
using namespace IGC;

namespace {

/// @brief  PrivateMemoryResolution pass used for resolving private memory alloca instructions.
///         This is done by resolving the alloca instructions.
///         This pass depends on the PrivateMemoryUsageAnalysis and
///         AddImplicitArgs passes running before it.

class PrivateMemoryResolution : public llvm::ModulePass {
public:
  // Pass identification, replacement for typeid
  static char ID;

  /// @brief  Constructor
  PrivateMemoryResolution();

  /// @brief  Destructor
  ~PrivateMemoryResolution() {}

  /// @brief  Provides name of pass
  virtual llvm::StringRef getPassName() const override { return "PrivateMemoryResolution"; }

  /// @brief  Adds the analysis required by this pass
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  /// @brief  Finds all alloca instructions, replaces them with by an llvm sequences.
  ///         and creates for each function a metadata that represents the total
  ///         amount of private memory needed by each work item.
  /// @param  M The Module to process.
  bool runOnModule(llvm::Module &M) override;

  /// @brief  Resolve collected alloca instructions.
  /// @param privateOnStack: whether the private variables are allocated on the stack
  /// @param hasOptNone: whether the function has the optnone attribute
  /// @return true if there were resolved alloca, false otherwise.
  bool resolveAllocaInstructions(bool privateOnStack, bool hasOptNone);

private:
  struct arrayIndex {
    llvm::GetElementPtrInst *gep;
    unsigned int operandIndex;
  };

  void expandPrivateMemoryForVla(uint32_t &maxPrivateMem);
  static bool testTransposedMemory(const Type *pTmpType, const Type *const pTypeOfAccessedObject,
                                   uint64_t tmpAllocaSize, const uint64_t bufferSizeLimit);

  /// @brief  The module level alloca information
  ModuleAllocaAnalysis *m_ModAllocaInfo = nullptr;

  /// @brief - Metadata API
  IGCMD::MetaDataUtils *m_pMdUtils = nullptr;

  /// @brief - Current processed function
  llvm::Function *m_currFunction = nullptr;
};

class TransposePrivMem : public TransposeHelper {
public:
  TransposePrivMem(const DataLayout &DL, Value *B, Value *Simdsize, uint32_t ChunkSize)
      : TransposeHelper(DL, true), m_DL(DL), m_simdSize(Simdsize), m_bufferBase(B), m_chunkBytes(ChunkSize) {
    IGC_ASSERT(isPowerOf2_32(m_chunkBytes));
    m_shtAmt = Log2_32(m_chunkBytes);
    m_offsetMask = maskTrailingOnes<uint32_t>(m_shtAmt);
  }

  void handleLoadInst(LoadInst *pLoad, Value *pScalarizedIdx);
  void handleStoreInst(StoreInst *pStore, Value *pScalarizedIdx);
  void handleLifetimeMark(IntrinsicInst *inst);
  bool useNewAlgo() { return true; }

private:
  const DataLayout &m_DL;
  Value *m_simdSize;
  Value *m_bufferBase;
  uint32_t m_chunkBytes;
  // For calculating chunk no and chunk offset
  uint32_t m_shtAmt;
  uint32_t m_offsetMask;

  // Return chunk number that byteOffset belongs to
  Value *getChunkNum(IGCLLVM::IRBuilder<> &IRB, Value *OffsetInBytes);
  // Return the offset within chunk that byteOffset points to
  Value *getChunkOff(IGCLLVM::IRBuilder<> &IRB, Value *OFfsetInBytes);
};

// helper
//   B: int / ptr;  O: unsigned int
//   Possible cases: ptr64 + i32/i64; ptr32 + i32; i64 + i32/i64; i32 + i32.
Value *addOffset(IGCLLVM::IRBuilder<> &IRB, const DataLayout &pDL, Value *B, Value *O) {
  Type *bTy = B->getType();
  [[maybe_unused]] Type *oTy = O->getType();
  IGC_ASSERT(oTy->isIntegerTy());
  IGC_ASSERT(bTy->isPointerTy() || bTy->isIntegerTy());
  Value *addr;
  if (isa<ConstantInt>(O) && cast<ConstantInt>(O)->isZero()) {
    addr = B;
  } else if (bTy->isPointerTy()) {
    Type *intPtrTy = pDL.getIntPtrType(bTy);
    IGC_ASSERT(intPtrTy->getPrimitiveSizeInBits() >= oTy->getPrimitiveSizeInBits());
    Value *baseAsInt = IRB.CreatePtrToInt(B, intPtrTy, VALUE_NAME("baseAsInt"));
    Value *off = IRB.CreateZExt(O, intPtrTy);
    Value *baseOffAsInt = IRB.CreateAdd(baseAsInt, off, VALUE_NAME("baseOffAsInt"), true, true);
    addr = IRB.CreateIntToPtr(baseOffAsInt, B->getType(), VALUE_NAME("baseAddOffset"));
  } else {
    IGC_ASSERT(bTy->getPrimitiveSizeInBits() >= oTy->getPrimitiveSizeInBits());
    Value *off = IRB.CreateZExt(O, bTy);
    addr = IRB.CreateAdd(B, off, VALUE_NAME("baseAddOffset"), true, true);
  }
  return addr;
};

// Given B, either int or ptr, convert it to ptr to T
Value *convertToPtr(IGCLLVM::IRBuilder<> &IRB, const DataLayout &pDL, Value *B, Type *ptrTy) {
  IGC_ASSERT(ptrTy->isPointerTy());
  Value *R;
  if (B->getType()->isPointerTy()) {
    R = IRB.CreateBitCast(B, ptrTy);
  } else {
    R = IRB.CreateIntToPtr(B, ptrTy);
  }
  return R;
}
} // namespace

ModulePass *IGC::CreatePrivateMemoryResolution() { return new PrivateMemoryResolution(); }

// Register pass to igc-opt
#define PASS_FLAG "igc-private-mem-resolution"
#define PASS_DESCRIPTION "Resolves private memory allocation"
#define PASS_CFG_ONLY true
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PrivateMemoryResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(ModuleAllocaAnalysis)
IGC_INITIALIZE_PASS_END(PrivateMemoryResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char PrivateMemoryResolution::ID = 0;

PrivateMemoryResolution::PrivateMemoryResolution() : ModulePass(ID) {
  initializePrivateMemoryResolutionPass(*PassRegistry::getPassRegistry());
}

void PrivateMemoryResolution::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.setPreservesCFG();
  AU.addRequired<MetaDataUtilsWrapper>();
  AU.addRequired<CodeGenContextWrapper>();
  AU.addRequired<llvm::CallGraphWrapperPass>();
  AU.addRequired<ModuleAllocaAnalysis>();
}

void PrivateMemoryResolution::expandPrivateMemoryForVla(uint32_t &maxPrivateMem) {
  // Add another 4KB if there are VLAs
  maxPrivateMem += 4096;
  std::string maxPrivateMemValue = std::to_string(maxPrivateMem);
  std::string fullWarningMessage =
      "VLA has been detected, the private memory size is set to " + maxPrivateMemValue +
      "B. "
      "You can change this size by setting environmental variable IGC_ForcePerThreadPrivateMemorySize to a value in "
      "range [1024:20480]. "
      "Greater values can affect performance, and lower ones may lead to incorrect results of your program.\n"
      "To make sure your program runs correctly you can use IGC_StackOverflowDetection feature. See documentation:\n"
      "https://github.com/intel/intel-graphics-compiler/tree/master/documentation/igc/StackOverflowDetection/"
      "StackOverflowDetection.md";

  getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->EmitWarning(fullWarningMessage.c_str());
}

bool PrivateMemoryResolution::runOnModule(llvm::Module &M) {
  // Get the analysis
  m_pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  auto *FGA = getAnalysisIfAvailable<GenXFunctionGroupAnalysis>();
  bool changed = false;

  ModuleMetaData &modMD = *getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

  // This is the only place to initialize and define UseScratchSpacePrivateMemory.
  // we do not use scratch-space if any kernel uses stack-call because,
  // in order to use scratch-space, we change data-layout for the module,
  // change pointer-size of AS-private to 32-bit.
  m_ModAllocaInfo = &getAnalysis<ModuleAllocaAnalysis>();
  bool bRet = m_ModAllocaInfo->safeToUseScratchSpace();
  CodeGenContext &Ctx = *getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  if (!bRet && Ctx.m_DriverInfo.supportsStatelessSpacePrivateMemory()) {
    // MinNOSPushConstantSize is only increased ONCE
    const uint32_t dwordSizeInBits = 32;
    modMD.MinNOSPushConstantSize += Ctx.getRegisterPointerSizeInBits(ADDRESS_SPACE_GLOBAL) / dwordSizeInBits;
  }
  modMD.compOpt.UseScratchSpacePrivateMemory = bRet;

  for (Function &F : M) {
    m_currFunction = &F;
    if (m_currFunction->isDeclaration()) {
      continue;
    }
    if (m_pMdUtils->findFunctionsInfoItem(m_currFunction) == m_pMdUtils->end_FunctionsInfo()) {
      continue;
    }
    auto FG = FGA ? FGA->getGroup(m_currFunction) : nullptr;
    bool hasStackCall = (FG && FG->hasStackCall()) || m_currFunction->hasFnAttribute("visaStackCall");
    bool hasVLA = (FG && FG->hasVariableLengthAlloca()) || m_currFunction->hasFnAttribute("hasVLA");
    bool isIndirectGroup = FG && FGA->isIndirectCallGroup(FG);
    if (Ctx.platform.hasScratchSurface() && modMD.compOpt.UseScratchSpacePrivateMemory) {
      // In this case, we could be generating 0-byte offsets for uniform
      // allocas which would manifest as null pointers.  This is because
      // we represent the r0.5 access later on so the pointer appears
      // to be null for downstream passes.  This attribute says that it's
      // okay so those passes wouldn't optimize away null pointer
      // dereferences because they would have otherwise been undefined
      // behavior.
      F.addFnAttr(llvm::Attribute::NullPointerIsValid);
    }
    // Resolve collected alloca instructions for current function
    changed |= resolveAllocaInstructions(hasStackCall || hasVLA || isIndirectGroup, m_currFunction->hasOptNone());

    // If stackcalls are in use, old FE Frame Pointer is written at the beginning of the stack.
    // FP takes 16 bytes, but padding may be added to satisfy allocas' alignment.
    if (IGC_IS_FLAG_ENABLED(EnableWriteOldFPToStack) && hasStackCall) {
      unsigned privateMemoryAlignment = m_ModAllocaInfo->getPrivateMemAlignment(m_currFunction);
      unsigned allocasExtraOffset = iSTD::Align(SIZE_OWORD, privateMemoryAlignment);
      modMD.FuncMD[m_currFunction].prevFPOffset = allocasExtraOffset;
    }

    // Initialize the stack mem usage per function group to the kernel's privateMemPerWI
    if (isEntryFunc(m_pMdUtils, m_currFunction)) {
      auto funcMD = modMD.FuncMD.find(m_currFunction);
      if (funcMD != modMD.FuncMD.end()) {
        modMD.PrivateMemoryPerFG[m_currFunction] = funcMD->second.privateMemoryPerWI;

        // VLA uses private mem on stack, but if used in a single entry function, may not get FGA.
        // If this is the case, still increase the private mem allocation to account for VLA.
        if (!FGA && hasVLA) {
          uint32_t maxPrivateMem = funcMD->second.privateMemoryPerWI;
          expandPrivateMemoryForVla(maxPrivateMem);

          maxPrivateMem = std::max(maxPrivateMem, Ctx.getPrivateMemoryMinimalSizePerThread());
          maxPrivateMem = std::max(maxPrivateMem, (uint32_t)(IGC_GET_FLAG_VALUE(ForcePerThreadPrivateMemorySize)));
          modMD.PrivateMemoryPerFG[m_currFunction] = maxPrivateMem;

          if (IGC_IS_FLAG_ENABLED(PrintStackCallDebugInfo)) {
            dbgs() << m_currFunction->getName().str()
                   << "\n  PrivateMemoryPerFG (bytes): " << modMD.PrivateMemoryPerFG[m_currFunction] << "\n";
          }
        }
      }
    }
  }

  if (FGA) {
    auto DL = M.getDataLayout();
    auto &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();

    // Map used to store child func's private memory for dynamic programming
    DenseMap<Function *, uint32_t> NestedPrivateMemMap;

    // lambda to recursively calculate the max private memory usage for each call path
    std::function<uint32_t(Function *)> AnalyzeCGPrivateMemUsage = [&AnalyzeCGPrivateMemUsage, &modMD, &CG, &DL, &Ctx,
                                                                    &M, &NestedPrivateMemMap](Function *F) -> uint32_t {
      // Value already calculated
      auto iter = NestedPrivateMemMap.find(F);
      if (iter != NestedPrivateMemMap.end())
        return iter->second;

      // Not a valid function, just return 0
      if (!F || F->isDeclaration())
        return 0;

      // No function metadata found, return 0
      auto funcIt = modMD.FuncMD.find(F);
      if (funcIt == modMD.FuncMD.end())
        return 0;

      uint32_t currFuncPrivateMem = (uint32_t)(funcIt->second.privateMemoryPerWI);
      // Add memory reserved for old FP.
      currFuncPrivateMem += funcIt->second.prevFPOffset;

      CallGraphNode *Node = CG[F];

      // Function has recursion, don't search CG further
      if (F->hasFnAttribute("hasRecursion"))
        return currFuncPrivateMem;

      // Reached a leaf, return the private memory used by the current function
      if (Node->empty())
        return currFuncPrivateMem;

      SmallSet<Function *, 16> childFuncs;
      // Collect the list of all direct callees
      for (auto FI = Node->begin(), FE = Node->end(); FI != FE; ++FI) {
        if (Function *childF = FI->second->getFunction()) {
          childFuncs.insert(childF);
        }
      }

      // Recursively calculate the max private mem usage of all callees
      uint32_t maxSize = 0;
      for (auto childF : childFuncs) {
        IGC_ASSERT(childF);
        // As a conservative measure, assume all stackcall args are stored on private memory
        uint32_t argSize = 0;
        for (auto AI = childF->arg_begin(), AE = childF->arg_end(); AI != AE; ++AI) {
          // Argument offsets are also OWORD aligned
          auto aiTy = AI->getType();

          if (aiTy->isMetadataTy())
            continue;

          argSize += iSTD::Align(static_cast<DWORD>(DL.getTypeAllocSize(aiTy)), SIZE_OWORD);
        }
        // Also do it for return value
        if (!childF->getReturnType()->isVoidTy()) {
          argSize += iSTD::Align(static_cast<DWORD>(DL.getTypeAllocSize(childF->getReturnType())), SIZE_OWORD);
        }

        uint32_t childMem = AnalyzeCGPrivateMemUsage(childF);
        if (NestedPrivateMemMap.find(childF) == NestedPrivateMemMap.end()) {
          // Store the calculated child func's private mem usage
          NestedPrivateMemMap[childF] = childMem;
        }
        uint32_t size = argSize + childMem;
        maxSize = std::max(maxSize, size);
      }
      return currFuncPrivateMem + maxSize;
    };

    // Calculate the max private mem used by each function group
    // by analyzing the call depth. Store this info in the FunctionGroup container.
    // This info is needed in EmitVISAPass to determine how much private memory to allocate
    // per SIMD per thread.
    for (auto GI = FGA->begin(), GE = FGA->end(); GI != GE; ++GI) {
      FunctionGroup *FG = *GI;
      Function *pKernel = FG->getHead();
      uint32_t maxPrivateMem = 0;

      if (FG->hasStackCall()) {
        // Analyze call depth for stack memory required
        maxPrivateMem = AnalyzeCGPrivateMemUsage(pKernel);
        std::string maxPrivateMemValue = std::to_string(maxPrivateMem);
        std::string fullWarningMessage =
            "Stack call has been detected, the private memory size is set to " + maxPrivateMemValue +
            "B. "
            "You can change this size by setting environmental variable IGC_ForcePerThreadPrivateMemorySize to a value "
            "in "
            "range [1024:20480]. "
            "Greater values can affect performance, and lower ones may lead to incorrect results of your program.\n"
            "To make sure your program runs correctly you can use StackOverflowDetection feature. See documentation:\n"
            "https://github.com/intel/intel-graphics-compiler/tree/master/documentation/igc/StackOverflowDetection/"
            "StackOverflowDetection.md";

        getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->EmitWarning(fullWarningMessage.c_str());
      }
      if (((FG->hasIndirectCall() && FG->hasPartialCallGraph()) || FG->hasRecursion()) &&
          Ctx.type != ShaderType::RAYTRACING_SHADER) {
        // If indirect calls or recursions exist, add additional 4KB and hope we don't run out.
        maxPrivateMem += (4 * 1024);
      }
      if (FG->hasVariableLengthAlloca() && Ctx.type != ShaderType::RAYTRACING_SHADER) {
        expandPrivateMemoryForVla(maxPrivateMem);
      }
      maxPrivateMem = std::max(maxPrivateMem, Ctx.getPrivateMemoryMinimalSizePerThread());
      maxPrivateMem = std::max(maxPrivateMem, (uint32_t)(IGC_GET_FLAG_VALUE(ForcePerThreadPrivateMemorySize)));

      if (maxPrivateMem > 0) {
        modMD.PrivateMemoryPerFG[pKernel] = (unsigned)maxPrivateMem;
      }

      if (IGC_IS_FLAG_ENABLED(PrintStackCallDebugInfo)) {
        dbgs() << pKernel->getName().str() << "\n  PrivateMemoryPerFG (bytes): " << modMD.PrivateMemoryPerFG[pKernel]
               << "\n";
      }
    }
  }

  if (changed) {
    m_pMdUtils->save(M.getContext());
  }

  return changed;
}

// Sink allocas into its first dominating use if possible. Alloca instructions
// are placed in the first basic block which dominates all other blocks. During
// alloca resolution, all address computations are done in the first block. And
// the address objects are live from the starting point. E.g.
//
//  int i = x;
//  foo(i);
//  int j = y;
//  bar(j);
//
// Variables i, j do not overlap in the source. When i and j are both in
// memory (optimizations disabled), llvm IR looks like
//
// [0] alloca i
// [1] alloca j
// [2] store x into &i
// [3] load i
// [4] foo(i)
// [5] store y into &j
// [6] load j
// [7] bar(j)
// Notice that address &i and &j overlap, [0-4) and [1-7) resp. Sinking allocas
// i and j to their lifetime start alleviates this issue.
//
// [0] alloca i
// [1] store x into &i
// [2] load i
// [3] foo(i)
// [4] alloca j
// [5] store y into &j
// [6] load j
// [7] bar(j)
//
static void sinkAllocas(SmallVectorImpl<AllocaInst *> &Allocas, bool highAllocaRAPressure) {
  IGC_ASSERT(false == Allocas.empty());
  DominatorTree DT;
  llvm::LoopInfoBase<llvm::BasicBlock, llvm::Loop> LI;
  bool Calcuated = false;

  // For each alloca, sink it if it has a use that dominates all other uses.
  // This use is called the dominating use.
  for (auto AI : Allocas) {
    if (AI->user_empty())
      continue;

    // If an alloca is used other than in an instruction, skip it.
    bool Skip = false;
    SmallVector<Instruction *, 8> UInsts;
    for (auto U : AI->users()) {
      auto UI = dyn_cast<Instruction>(U);
      // can't sink the alloca in the same BB where a PHI node exists
      // As it will violate the basic block structure, since phi nodes
      // will always be at the beginging of a BB
      if (!UI || isa<PHINode>(UI)) {
        Skip = true;
        break;
      }
      UInsts.push_back(UI);
    }

    if (Skip)
      continue;

    // Compute dominator tree lazily.
    if (!Calcuated) {
      Function *F = AI->getParent()->getParent();
      DT.recalculate(*F);
      LI.releaseMemory();
      LI.analyze(DT);
      Calcuated = true;
    }

    // Find the Nearest Common Denominator for all the uses
    Instruction *DomUse = UInsts[0];
    BasicBlock *DomBB = DomUse->getParent();
    for (unsigned i = 1; i < UInsts.size(); ++i) {
      Instruction *Use = UInsts[i];
      BasicBlock *UseBB = Use->getParent();

      // skip unreachable BB
      if (UseBB->use_empty() && !UseBB->isEntryBlock())
        continue;

      DomBB = DT.findNearestCommonDominator(DomBB, UseBB);
      if (!DomBB) {
        break;
      }
    }

    // Find the nearest Denominator outside loops to prevent multiple allocations
    // In case when we have too many allocas that increase RegPressure, skip this optimization
    if (!highAllocaRAPressure) {
      BasicBlock *CurBB = AI->getParent();
      while (DomBB && DomBB != CurBB && LI.getLoopFor(DomBB) != nullptr) {
        DomBB = DT.getNode(DomBB)->getIDom()->getBlock();
      }
    }

    if (DomBB) {
      // If DomBB has a use in it, insert it just before the first use.
      // Otherwise, append it to the end of the block, to reduce register pressure.
      Instruction *InsertPt = DomBB->getTerminator();
      for (Instruction *Use : UInsts) {
        if (DomBB == Use->getParent() && DT.dominates(Use, InsertPt)) {
          InsertPt = Use;
        }
      }
      AI->moveBefore(InsertPt);
    }
  }
}

static void sinkAllocaSingleUse(SmallVectorImpl<AllocaInst *> &Allocas) {
  IGC_ASSERT(false == Allocas.empty());
  DominatorTree DT;
  bool Calcuated = false;

  // For each alloca's use, sink it if it has a use that dominates all other uses.
  // This use is called the dominating use.
  for (auto AI : Allocas) {
    if (AI->user_empty())
      continue;

    for (auto A : AI->users()) {
      bool Skip = false;
      SmallVector<Instruction *, 8> UInsts;
      auto UI = cast<Instruction>(A);
      // can't sink phi nodes to other BBs
      // can't sink loads since we don't check for stores on the way
      if (isa<PHINode>(UI) || UI->mayReadFromMemory())
        continue;

      for (auto U : UI->users()) {
        auto UUI = dyn_cast<Instruction>(U);
        // can't sink the use in the same BB where a PHI node exists
        // As it will violate the basic block structure, since phi nodes
        // will always be at the beginging of a BB
        if (!UUI || isa<PHINode>(UUI)) {
          Skip = true;
          break;
        }
        UInsts.push_back(UUI);
      }
      if (Skip || UInsts.size() == 0)
        continue;
      // Compute dominator tree lazily.
      if (!Calcuated) {
        Function *F = AI->getParent()->getParent();
        DT.recalculate(*F);
        Calcuated = true;
      }

      // Find the Nearest Common Denominator for all the uses
      Instruction *DomUse = UInsts[0];
      BasicBlock *DomBB = DomUse->getParent();
      for (unsigned i = 1; i < UInsts.size(); ++i) {
        Instruction *Use = UInsts[i];
        BasicBlock *UseBB = Use->getParent();

        // skip unreachable BB
        if (UseBB->use_empty() && !UseBB->isEntryBlock())
          continue;

        DomBB = DT.findNearestCommonDominator(DomBB, UseBB);
        if (!DomBB) {
          break;
        }
      }

      if (DomBB) {
        // If DomBB has a use in it, insert it just before the first use.
        // Otherwise, append it to the end of the block, to reduce register pressure.
        Instruction *InsertPt = DomBB->getTerminator();
        for (Instruction *Use : UInsts) {
          if (DomBB == Use->getParent() && DT.dominates(Use, InsertPt)) {
            InsertPt = Use;
          }
        }
        UI->moveBefore(InsertPt);
      }
    }
  }
}

class TransposeHelperPrivateMem : public TransposeHelper {
public:
  Value *simdSize;
  Value *base;
  unsigned int elementSize;
  bool vectorIO;
  TransposeHelperPrivateMem(const DataLayout &DL, Value *b, Value *size, unsigned int eltSize, bool vectorType)
      : TransposeHelper(DL, vectorType) {
    simdSize = size;
    base = b;
    elementSize = eltSize;
    vectorIO = vectorType;
  }
  void handleLoadInst(LoadInst *pLoad, Value *pScalarizedIdx) {
    IGC_ASSERT(nullptr != pLoad);
    IGC_ASSERT(pLoad->isSimple());
    IGCLLVM::IRBuilder<> IRB(pLoad);
    if (isa<Instruction>(pLoad->getPointerOperand())) {
      IRB.SetInsertPoint(cast<Instruction>(pLoad->getPointerOperand()));
    }
    Value *eltSize = IRB.getInt32(elementSize);
    Value *stride = IRB.CreateMul(simdSize, eltSize);
    Value *address = IRB.CreateMul(pScalarizedIdx, stride);
    address = IRB.CreateAdd(base, address);
    IRB.SetInsertPoint(pLoad);
    if (!vectorIO && pLoad->getType()->isVectorTy()) {
      Type *scalarType = pLoad->getType()->getScalarType();
      IGC_ASSERT(nullptr != scalarType);
      Type *scalarptrTy = PointerType::get(scalarType, pLoad->getPointerAddressSpace());
      IGC_ASSERT(scalarType->getPrimitiveSizeInBits() / 8 == elementSize);
      Value *vec = UndefValue::get(pLoad->getType());
      auto pLoadVT = cast<IGCLLVM::FixedVectorType>(pLoad->getType());
      for (unsigned i = 0, e = (unsigned)pLoadVT->getNumElements(); i < e; ++i) {
        Value *ptr = IRB.CreateIntToPtr(address, scalarptrTy);
        Value *v = IRB.CreateLoad(scalarType, ptr);
        vec = IRB.CreateInsertElement(vec, v, IRB.getInt32(i));
        address = IRB.CreateAdd(address, stride);
      }
      pLoad->replaceAllUsesWith(vec);
      pLoad->eraseFromParent();
    } else {
      Value *ptr = IRB.CreateIntToPtr(address, pLoad->getPointerOperand()->getType());
      pLoad->setOperand(0, ptr);
    }
  }
  void handleStoreInst(StoreInst *pStore, Value *pScalarizedIdx) {
    IGC_ASSERT(nullptr != pStore);
    IGC_ASSERT(pStore->isSimple());
    IGCLLVM::IRBuilder<> IRB(pStore);
    if (isa<Instruction>(pStore->getPointerOperand())) {
      IRB.SetInsertPoint(cast<Instruction>(pStore->getPointerOperand()));
    }
    Value *eltSize = IRB.getInt32(elementSize);
    Value *stride = IRB.CreateMul(simdSize, eltSize);
    Value *address = IRB.CreateMul(pScalarizedIdx, stride);
    address = IRB.CreateAdd(base, address);
    IRB.SetInsertPoint(pStore);
    if (!vectorIO && pStore->getValueOperand()->getType()->isVectorTy()) {
      Type *scalarType = pStore->getValueOperand()->getType()->getScalarType();
      IGC_ASSERT(nullptr != scalarType);
      Type *scalarptrTy = PointerType::get(scalarType, pStore->getPointerAddressSpace());
      IGC_ASSERT(scalarType->getPrimitiveSizeInBits() / 8 == elementSize);
      Value *vec = pStore->getValueOperand();

      unsigned vecNumElts = (unsigned)cast<IGCLLVM::FixedVectorType>(vec->getType())->getNumElements();
      for (unsigned i = 0; i < vecNumElts; ++i) {
        Value *ptr = IRB.CreateIntToPtr(address, scalarptrTy);
        IRB.CreateStore(IRB.CreateExtractElement(vec, IRB.getInt32(i)), ptr);
        address = IRB.CreateAdd(address, stride);
      }
      pStore->eraseFromParent();
    } else {
      Value *ptr = IRB.CreateIntToPtr(address, pStore->getPointerOperand()->getType());
      pStore->setOperand(1, ptr);
    }
  }
  void handleLifetimeMark(IntrinsicInst *inst) {
    IGC_ASSERT(nullptr != inst);
    IGC_ASSERT((inst->getIntrinsicID() == llvm::Intrinsic::lifetime_start) ||
               (inst->getIntrinsicID() == llvm::Intrinsic::lifetime_end));
    inst->eraseFromParent();
  }
};

Value *TransposePrivMem::getChunkNum(IGCLLVM::IRBuilder<> &IRB, Value *OffsetInBytes) {
  Value *chunkNo = IRB.CreateLShr(OffsetInBytes, m_shtAmt);
  return chunkNo;
}

Value *TransposePrivMem::getChunkOff(IGCLLVM::IRBuilder<> &IRB, Value *OffsetInBytes) {
  Value *chunkOffset = IRB.CreateAnd(OffsetInBytes, m_offsetMask);
  return chunkOffset;
}

void TransposePrivMem::handleLoadInst(LoadInst *pLoad, Value *pScalarizedIdx) {
  IGC_ASSERT(nullptr != pLoad);
  IGC_ASSERT(pLoad->isSimple());
  IGCLLVM::IRBuilder<> IRB(pLoad);
  uint32_t bytes = (uint32_t)m_DL.getTypeStoreSize(pLoad->getType());
  IGC_ASSERT(bytes <= m_chunkBytes);
  IGC_ASSERT(isPowerOf2_32(bytes));

  Value *chunkNo = getChunkNum(IRB, pScalarizedIdx);
  Value *chunkVal = IRB.getInt32(m_chunkBytes);
  Value *chunkBytesPerWI = IRB.CreateMul(chunkNo, chunkVal);
  Value *chunkByteOffset = IRB.CreateMul(m_simdSize, chunkBytesPerWI);
  Value *addr = addOffset(IRB, m_DL, m_bufferBase, chunkByteOffset);

  Value *eltIx;
  if (bytes == m_chunkBytes) {
    // Each type is naturally aligned -> chunk offset = 0
    eltIx = IRB.getInt32(0);
  } else {
    // (a / bytes) == (a >> Log2_32(bytes))
    uint32_t lshrAmt = Log2_32(bytes);
    Value *chunkOff = getChunkOff(IRB, pScalarizedIdx);
    if (bytes == 1)
      eltIx = chunkOff;
    else
      eltIx = IRB.CreateLShr(chunkOff, IRB.getInt32(lshrAmt));
  }

  Value *addrInElt = convertToPtr(IRB, m_DL, addr, pLoad->getPointerOperandType());
  Value *gep = IRB.CreateGEP(pLoad->getType(), addrInElt, eltIx, VALUE_NAME(pLoad->getName() + ".SOAPrivMemGEP"));
  Value *val =
      IRB.CreateAlignedLoad(cast<GetElementPtrInst>(gep)->getResultElementType(), gep, IGCLLVM::getAlign(*pLoad));

  pLoad->replaceAllUsesWith(val);
  pLoad->eraseFromParent();
}

void TransposePrivMem::handleStoreInst(StoreInst *pStore, Value *pScalarizedIdx) {
  IGC_ASSERT(nullptr != pStore);
  IGC_ASSERT(pStore->isSimple());
  IGCLLVM::IRBuilder<> IRB(pStore);
  Type *valTy = pStore->getValueOperand()->getType();
  uint32_t bytes = (uint32_t)m_DL.getTypeStoreSize(valTy);
  IGC_ASSERT(bytes <= m_chunkBytes);
  IGC_ASSERT(isPowerOf2_32(bytes));

  Value *chunkNo = getChunkNum(IRB, pScalarizedIdx);
  Value *chunkVal = IRB.getInt32(m_chunkBytes);
  Value *chunkBytesPerWI = IRB.CreateMul(chunkNo, chunkVal);
  Value *chunkByteOffset = IRB.CreateMul(m_simdSize, chunkBytesPerWI);
  Value *addr = addOffset(IRB, m_DL, m_bufferBase, chunkByteOffset);

  Value *eltIx;
  if (bytes == m_chunkBytes) {
    // Each type is naturally aligned -> chunk offset = 0
    eltIx = IRB.getInt32(0);
  } else {
    // (a / bytes) == (a >> Log2_32(bytes))
    uint32_t lshrAmt = Log2_32(bytes);
    Value *chunkOff = getChunkOff(IRB, pScalarizedIdx);
    if (bytes == 1)
      eltIx = chunkOff;
    else
      eltIx = IRB.CreateLShr(chunkOff, IRB.getInt32(lshrAmt));
  }

  Value *addrInElt = convertToPtr(IRB, m_DL, addr, pStore->getPointerOperandType());
  Value *gep = IRB.CreateGEP(pStore->getValueOperand()->getType(), addrInElt, eltIx,
                             VALUE_NAME(pStore->getName() + ".SOAPrivMemGEP"));
  IRB.CreateAlignedStore(pStore->getValueOperand(), gep, IGCLLVM::getAlign(*pStore));

  pStore->eraseFromParent();
}

void TransposePrivMem::handleLifetimeMark(IntrinsicInst *inst) {
  IGC_ASSERT(nullptr != inst);
  IGC_ASSERT((inst->getIntrinsicID() == llvm::Intrinsic::lifetime_start) ||
             (inst->getIntrinsicID() == llvm::Intrinsic::lifetime_end));
  inst->eraseFromParent();
}

[[maybe_unused]] bool PrivateMemoryResolution::testTransposedMemory(const Type *pTmpType,
                                                                    const Type *const pTypeOfAccessedObject,
                                                                    uint64_t tmpAllocaSize,
                                                                    const uint64_t bufferSizeLimit) {
  // verify that the size of transposed memory fits into the allocated scratch region

  bool ok = true;

  if (ok) {
    ok = (nullptr != pTmpType);
    IGC_ASSERT(ok);
  }

  if (ok) {
    ok = (nullptr != pTypeOfAccessedObject);
    IGC_ASSERT(ok);
  }

  if (ok) {
    ok = (0 < tmpAllocaSize);
    IGC_ASSERT(ok);
  }

  if (ok) {
    ok = (tmpAllocaSize <= bufferSizeLimit);
    IGC_ASSERT(ok);
  }

  while (ok && (pTypeOfAccessedObject != pTmpType)) {
    if (pTmpType->isStructTy() && (pTmpType->getStructNumElements() == 1)) {
      pTmpType = pTmpType->getStructElementType(0);
      ok = (nullptr != pTmpType);
      IGC_ASSERT(ok);
    } else if (pTmpType->isArrayTy()) {
      tmpAllocaSize *= pTmpType->getArrayNumElements();
      pTmpType = pTmpType->getContainedType(0);
      ok = (nullptr != pTmpType);
      IGC_ASSERT(ok);
    } else if (pTmpType->isVectorTy()) {
      auto pTmpVType = cast<IGCLLVM::FixedVectorType>(pTmpType);
      tmpAllocaSize *= pTmpVType->getNumElements();
      pTmpType = pTmpType->getContainedType(0);
      ok = (nullptr != pTmpType);
      IGC_ASSERT(ok);
    } else {
      // unsupported type for memory transposition
      ok = false;
      IGC_ASSERT(ok);
    }
  }

  if (ok) {
    ok = (0 < tmpAllocaSize);
    IGC_ASSERT(ok);
  }

  if (ok) {
    ok = (tmpAllocaSize <= bufferSizeLimit);
    IGC_ASSERT(ok);
  }

  return ok;
}

bool PrivateMemoryResolution::resolveAllocaInstructions(bool privateOnStack, bool hasOptNone) {
  CodeGenContext &Ctx = *getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  // It is possible that there is no alloca instruction in the caller but there
  // is alloca in the callee. Save the total private memory to the metadata.
  unsigned int totalPrivateMemPerWI = m_ModAllocaInfo->getTotalPrivateMemPerWI(m_currFunction);

  // This change is only till the FuncMD is ported to new MD framework
  ModuleMetaData *const modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  IGC_ASSERT(nullptr != modMD);
  modMD->FuncMD[m_currFunction].privateMemoryPerWI = totalPrivateMemPerWI;

  SmallVector<AllocaInst *, 8> &allocaInsts = m_ModAllocaInfo->getAllocaInsts(m_currFunction);
  if (allocaInsts.empty()) {
    // No alloca instructions to process.
    return false;
  }

  for (AllocaInst *aInst : allocaInsts) {
    auto allocationSize =
        IGCLLVM::makeOptional(aInst->getAllocationSizeInBits(m_currFunction->getParent()->getDataLayout()));

    if (allocationSize.has_value()) {
      auto scratchUsage_i = Ctx.m_ScratchSpaceUsage.find(m_currFunction);
      if (scratchUsage_i == Ctx.m_ScratchSpaceUsage.end()) {
        Ctx.m_ScratchSpaceUsage.insert({m_currFunction, 0});
      }
      Ctx.m_ScratchSpaceUsage[m_currFunction] += allocationSize.value() / 8;
    }
  }

  if (Ctx.m_instrTypes.numAllocaInsts > IGC_GET_FLAG_VALUE(AllocaRAPressureThreshold)) {
    sinkAllocaSingleUse(allocaInsts);
  }

  // if we have privateOnStack, then address rematerialization will not work for allocas, in this case
  // we need to sink aggressively, even into loops
  // TODO: combine this and the upper heuristic into one independent from number of allocas, but from RegPressure
  auto totalAllocaThreshold =
      (!hasOptNone ? IGC_GET_FLAG_VALUE(AllocaRAPressureThreshold)
                   : std::max(static_cast<int>(IGC_GET_FLAG_VALUE(AllocaRAPressureThreshold)) -
                                  static_cast<int>(IGC_GET_FLAG_VALUE(AllocaSinkingOptNoneAllowance)),
                              0));
  bool highAllocaRAPressure = allocaInsts.size() > totalAllocaThreshold && privateOnStack;
  sinkAllocas(allocaInsts, highAllocaRAPressure);

  // Each AllocaInst creates a buffer and all buffers are put together
  // sequentially like the following:
  //     [buffer0, buffer1, buffer2, ..., bufferN]
  // For M+1 threads (hardware threads), their memory layout will look like
  // this:
  //     [buffer0 thread0][buffer1 thread0]...[bufferN thread0]
  //     [buffer0 thread1][buffer1 thread1]...[bufferN thread1]
  //     ...
  //     [buffer0 threadM][buffer1 threadM]...[bufferN threadM]
  // Each row represents total private memory per thread, and all buffers
  // share the same base address (or offset). For example, [buffer0 thread0]
  // can be laid out in two ways, assuming SIMD width is K+1:
  //   (1) in a single consecutive chunk like the following:
  //       [buffer0 lane0][buffer0 lane1]...[buffer0 laneK]
  //   (2) could be divided into several small chunks via SOA transpose
  //       optimization if this optimization can apply
  //          buffer0 -> {chunk0, chunk1, ..., }
  //       [buffer0 thread0] can be laid out like this:
  //         [chunk0 lane0][chunk0 lane1] .... [chunk0 laneK]
  //         [chunk1 lane0][chunk1 lane1] .... [chunk1 laneK]
  //         ...
  //       This optimization is for better cache utilization.
  //
  // To get buffer i of thread j we need to calculate:
  // {buffer i ptr} = privateBase +
  //                  {thread j's offset} +
  //                  {buffer offset} +
  //                  {per lane offset}
  //
  // Where:
  //   privateBase                    = implicit argument, points to [buffer0 thread0]
  //                                    (for scratch, it is an offset 0)
  //   {thread j's offset}            = {threadId of j} * {total private mem per thread}
  //   {total private mem per thread} = simdSize * {total private mem per WI}
  //   {buffer offset}                = simdSize * {buffer i offset per WI}
  //   {per lane offset}              = simdLaneId * sizeof(buffer i)
  //                                    (different if tranpose is on)
  //   ({SIMD buffer offse} = {buffer offset} + {per lane offset})
  //
  // simdSize and simdOffsetBase are calculated using intrinsics that are planted in this pass
  // and resolved in the code gen. Offset could be either 32 bit or 64 bit. If the total
  // private memory for a kernel is larger than 4GB, offset should be 64 bit.

  LLVMContext &C = m_currFunction->getContext();
  const DataLayout &DL = m_currFunction->getParent()->getDataLayout();

  IntegerType *typeInt32 = Type::getInt32Ty(C);
  IntegerType *typeInt64 = Type::getInt64Ty(C);
  // Creates intrinsics that will be lowered in the CodeGen and will handle the simd lane id
  Function *simdLaneIdFunc =
      GenISAIntrinsic::getDeclaration(m_currFunction->getParent(), GenISAIntrinsic::GenISA_simdLaneId);
  // Creates intrinsics that will be lowered in the CodeGen and will handle the simd size
  Function *simdSizeFunc =
      GenISAIntrinsic::getDeclaration(m_currFunction->getParent(), GenISAIntrinsic::GenISA_simdSize);

  IGCLLVM::IRBuilder<> entryBuilder(&*m_currFunction->getEntryBlock().getFirstInsertionPt());
  ImplicitArgs implicitArgs(*m_currFunction, m_pMdUtils);

  // Construct an empty DebugLoc.
  DebugLoc entryDebugLoc;
  entryBuilder.SetCurrentDebugLocation(entryDebugLoc);

  // Creates intrinsics that will be lowered in the CodeGen and will handle the stack-pointer
  Instruction *simdLaneId16 = entryBuilder.CreateCall(simdLaneIdFunc, {}, VALUE_NAME("simdLaneId16"));
  Value *simdLaneId = entryBuilder.CreateIntCast(simdLaneId16, typeInt32, false, VALUE_NAME("simdLaneId"));
  Instruction *simdSize = entryBuilder.CreateCall(simdSizeFunc, {}, VALUE_NAME("simdSize"));

  // The implementation in this function implies that the total private memory
  // for each thread is no larger than 4 GB, which implies that the total private
  // memory for WI is no larger than 128MB. But the total private memory for a kernel
  // could be larger than 4 GB, which needs to use 64bit offset from private base.
  //
  // 32 is max simd width
  IGC_ASSERT_MESSAGE((totalPrivateMemPerWI * 32ull) <= (uint64_t)UINT32_MAX,
                     "Total private memory per work-item should not be over 128MB!");
  bool safe32bitOffset =
      m_currFunction->getParent()->getDataLayout().getPointerSize() < 8 ||
      (totalPrivateMemPerWI * 32ull * Ctx.platform.getMaxAddressedHWThreads()) <= (uint64_t)UINT32_MAX;

  // Return thread offset. As it is per-thread, the calculation should be done in entry BB.
  auto createThreadOffset = [simdSize, typeInt32, typeInt64,
                             safe32bitOffset](IGCLLVM::IRBuilder<> &IRB, Value *ThreadID, uint32_t TotalPrivMemPerWI) {
    // Total PM per thread is no larger than 4GB
    ConstantInt *totalPM = ConstantInt::get(typeInt32, TotalPrivMemPerWI);
    Value *totalPMPerThread = IRB.CreateMul(simdSize, totalPM, VALUE_NAME("totalPrivateMemPerThread"));
    if (!safe32bitOffset) {
      totalPMPerThread = IRB.CreateZExt(totalPMPerThread, typeInt64);
      ThreadID = IRB.CreateZExt(ThreadID, typeInt64);
    }
    Value *perThreadOffset = IRB.CreateMul(ThreadID, totalPMPerThread, VALUE_NAME("perThreadOffset"));
    return perThreadOffset;
  };

  if (privateOnStack) {
    for (auto pAI : allocaInsts) {
      bool isUniform = pAI->getMetadata("uniform") != nullptr;
      IGCLLVM::IRBuilder<> builder(pAI);
      builder.SetCurrentDebugLocation(entryDebugLoc);

      // buffer of this private var
      Value *privateBuffer = nullptr;
      if (!isa<ConstantInt>(pAI->getArraySize())) {
        // vla array must be AOS layout on stack
        Value *increment = isUniform ? builder.getInt32(0) : simdLaneId;
        // truncate alloca size to i32
        Value *arraySize = builder.CreateTrunc(pAI->getArraySize(), increment->getType(), VALUE_NAME("TruncVLASize"));
        Value *sizeWithType = builder.CreateMul(
            arraySize,
            builder.getInt32(static_cast<uint32_t>(
                m_currFunction->getParent()->getDataLayout().getTypeAllocSize(pAI->getAllocatedType()))),
            VALUE_NAME("VLASizeWithType"));
        Value *perLaneOffset = builder.CreateMul(increment, sizeWithType, VALUE_NAME("VLAPerLaneOffset"));
        // Create VLAStackAlloca intrinsic which will set private buffer offset to "SP + laneOffset",
        // and set SP to "SP + buffer_size" in visa emitPass
        Value *intrinArgs[] = {perLaneOffset, sizeWithType};
        Function *stackAllocaFunc =
            GenISAIntrinsic::getDeclaration(m_currFunction->getParent(), GenISAIntrinsic::GenISA_VLAStackAlloca);
        Value *stackAlloca = builder.CreateCall(stackAllocaFunc, intrinArgs, VALUE_NAME("VLAStackAlloca"));
        privateBuffer =
            builder.CreatePointerCast(stackAlloca, pAI->getType(), VALUE_NAME(pAI->getName() + ".privateBuffer"));
      } else {
        Value *totalOffset = m_ModAllocaInfo->getOffset(builder, pAI, simdSize, simdLaneId);
        Function *stackAllocaFunc =
            GenISAIntrinsic::getDeclaration(m_currFunction->getParent(), GenISAIntrinsic::GenISA_StackAlloca);
        Value *stackAlloca = builder.CreateCall(stackAllocaFunc, totalOffset, VALUE_NAME("stackAlloca"));
        privateBuffer =
            builder.CreatePointerCast(stackAlloca, pAI->getType(), VALUE_NAME(pAI->getName() + ".privateBuffer"));

        // Attaching this metadata is crucial to both properly interpret this locations as stack based ond to inline it.
        // Because these are stack locations we can safely inline them even with optimizations disabled (O0).
        auto DbgDcls = llvm::FindDbgDeclareUses(pAI);
        for (auto DbgDcl : DbgDcls) {
          unsigned scalarBufferOffset = m_ModAllocaInfo->getBufferOffset(pAI);
          unsigned bufferSize = m_ModAllocaInfo->getBufferStride(pAI);

          // Attach metadata to instruction containing offset of storage
          auto OffsetMD =
              MDNode::get(builder.getContext(), ConstantAsMetadata::get(builder.getInt32(scalarBufferOffset)));
          DbgDcl->setMetadata("StorageOffset", OffsetMD);
          auto SizeMD = MDNode::get(builder.getContext(), ConstantAsMetadata::get(builder.getInt32(bufferSize)));
          DbgDcl->setMetadata("StorageSize", SizeMD);
        }
      }
      // Replace all uses of original alloca with the bitcast
      pAI->replaceAllUsesWith(privateBuffer);
      pAI->eraseFromParent();
    }
    return true;
  }

  // What is the size limit of this scratch memory? If we use >= 128 KB for private data, then we have
  // no space left for later spilling.
  bool useStateless = false;

  if (Ctx.type != ShaderType::OPENCL_SHADER) {
    useStateless = Ctx.m_DriverInfo.supportsStatelessSpacePrivateMemory();
  }

  // NOTE: Below if block logic is used either for SSS RW or non-OCL stateless RW
  if (modMD->compOpt.UseScratchSpacePrivateMemory || useStateless) {
    // We want to use this pass to lower alloca instruction
    // to remove some redundant instruction caused by alloca. For original approach,
    // different threads use the same private base. While for this approach, each
    // thread has its own private base, so we don't have to calculate the
    // private base from threadid as we did previously.  In this case, we only need
    // PrivateMemoryUsageAnalysis pass, no need to run AddImplicitArgs pass.
    Value *privateBase = nullptr;
    Value *threadBase = nullptr;
    ADDRESS_SPACE scratchMemoryAddressSpace = ADDRESS_SPACE_PRIVATE;
    if (modMD->compOpt.UseScratchSpacePrivateMemory) {
      if (Ctx.platform.hasScratchSurface()) {
        // when we use per-thread scratch-surface with SSH bindless
        // R0_5[32:10] is the offset of the surface-state for scratch
        // surface slot#0, NOT the offset into the surface.
        privateBase = entryBuilder.getInt32(0);
      } else { // the old mechanism
        Value *r0Val = implicitArgs.getImplicitArgValue(*m_currFunction, ImplicitArg::R0, m_pMdUtils);
        Value *r0_5 = entryBuilder.CreateExtractElement(r0Val, ConstantInt::get(typeInt32, 5), VALUE_NAME("r0.5"));
        privateBase = entryBuilder.CreateAnd(r0_5, ConstantInt::get(typeInt32, 0xFFFFFC00), VALUE_NAME("privateBase"));
      }
      threadBase = privateBase;
    } else {
      scratchMemoryAddressSpace = ADDRESS_SPACE_GLOBAL;
      modMD->compOpt.UseStatelessforPrivateMemory = true;

      const uint32_t dwordSizeInBits = 32;
      const uint32_t pointerSizeInDwords =
          Ctx.getRegisterPointerSizeInBits(scratchMemoryAddressSpace) / dwordSizeInBits;
      IGC_ASSERT(pointerSizeInDwords <= 2);
      llvm::Type *resultType = entryBuilder.getInt32Ty();
      if (pointerSizeInDwords > 1) {
        resultType = IGCLLVM::FixedVectorType::get(resultType, 2);
      }
      if (Ctx.type == ShaderType::RAYTRACING_SHADER) {
        RTBuilder rtBuilder(m_currFunction->getContext(), Ctx);
        rtBuilder.SetInsertPoint(entryBuilder.GetInsertBlock(), entryBuilder.GetInsertPoint());
        privateBase = rtBuilder.getStatelessScratchPtr();
        entryBuilder.SetInsertPoint(rtBuilder.GetInsertBlock(), rtBuilder.GetInsertPoint());
      } else {
        Function *pFunc = GenISAIntrinsic::getDeclaration(m_currFunction->getParent(),
                                                          GenISAIntrinsic::GenISA_RuntimeValue, resultType);
        privateBase =
            entryBuilder.CreateCall(pFunc, entryBuilder.getInt32(modMD->MinNOSPushConstantSize - pointerSizeInDwords));
      }
      if (privateBase->getType()->isVectorTy()) {
        privateBase = entryBuilder.CreateBitCast(privateBase, entryBuilder.getInt64Ty());
      }

      Function *pHWTIDFunc = GenISAIntrinsic::getDeclaration(
          m_currFunction->getParent(), GenISAIntrinsic::GenISA_hw_thread_id_alloca, Type::getInt32Ty(C));
      Value *threadId = entryBuilder.CreateCall(pHWTIDFunc);
      Value *perThreadOffset = createThreadOffset(entryBuilder, threadId, totalPrivateMemPerWI);
      threadBase = addOffset(entryBuilder, DL, privateBase, perThreadOffset);
    }

    for (auto pAI : allocaInsts) {
      bool isUniform = pAI->getMetadata("uniform") != nullptr;
      IGCLLVM::IRBuilder<> builder(pAI);
      // Post upgrade to LLVM 3.5.1, it was found that inliner propagates debug info of callee
      // in to the alloca. Further, those allocas are somehow hoisted to the top of program.
      // When those allocas are lowered to below sequence, they result in prologue instructions
      // pointing to a much later line of code. This causes a single src line to now have
      // multiple VISA offset mappings and prevents debugger from setting breakpoints
      // correctly. So instead, we set DebugLoc for the instructions generated by lowering
      // alloca to mark that they are part of the prologue.
      // Note: As per Amjad, later LLVM version has a fix for this in llvm/lib/Transforms/Utils/InlineFunction.cpp.
      builder.SetCurrentDebugLocation(pAI->getDebugLoc());

      // If we can use SOA layout transpose the memory
      IGC::SOALayoutChecker SOAChecker(*pAI, Ctx.type == ShaderType::OPENCL_SHADER,
                                       UseScratchSpacePrivateMemoryOrUseStatelessStrategy);
      IGC::SOALayoutInfo SOAInfo = SOAChecker.getOrGatherInfo();
      // TransposeMemLayout is not prepared to work on 64-bit pointers (originally, the private address space is
      // expressed by 32-bit pointers). Address space casting
      bool TransposeMemLayout = ADDRESS_SPACE_PRIVATE == scratchMemoryAddressSpace && SOAInfo.canUseSOALayout &&
                                !m_ModAllocaInfo->isUniform(pAI);
      Type *pTypeOfAccessedObject = SOAInfo.baseType;
      bool allUsesAreVector = SOAInfo.allUsesAreVector;
      Value *privateBufferPTR;

      // New Algo handles both 64bit ptr and 32bi ptr.
      if (!isUniform && SOAInfo.canUseSOALayout && SOAChecker.useNewAlgo(pTypeOfAccessedObject)) {
        Value *simdBufferOffset =
            m_ModAllocaInfo->getOffset(builder, pAI, simdSize, simdLaneId, SOAInfo.SOAPartitionBytes);
        Value *bufferBase = addOffset(builder, DL, threadBase, simdBufferOffset);
        privateBufferPTR =
            builder.CreateIntToPtr(bufferBase, pAI->getAllocatedType()->getPointerTo(scratchMemoryAddressSpace),
                                   VALUE_NAME(pAI->getName() + ".privateBufferPTR"));

        TransposePrivMem helper(DL, bufferBase, simdSize, SOAInfo.SOAPartitionBytes);
        Value *Idx = builder.getInt32(0);
        helper.HandleAllocaSources(pAI, Idx);
        helper.EraseDeadCode();
      } else {
        if (TransposeMemLayout && IGC_IS_FLAG_ENABLED(EnableSOAPromotionDisablingHeuristic)) {
          // SOA layout can be not beneficial for the alloca, even is it is possible
          // For example, if it is not vectorSOA, but all uses are vectors
          // (in case of vectors of different size of elements number) we close the
          // possibility to use large loads/stores, so cancel the transformation.
          // Currently it is only enabled by option
          bool isSOABeneficial = pTypeOfAccessedObject->isVectorTy() || !allUsesAreVector;
          Type *allocaType = pAI->getAllocatedType();
          if (VectorType *vectorType = dyn_cast<VectorType>(allocaType)) {
            bool baseTypeIsSmall = (unsigned)(vectorType->getElementType()->getScalarSizeInBits()) < 32;
            isSOABeneficial |= baseTypeIsSmall;
          }
          TransposeMemLayout &= isSOABeneficial;
        }

        unsigned int bufferSize = 0;
        if (TransposeMemLayout) {
          bufferSize = (unsigned)DL.getTypeAllocSize(pTypeOfAccessedObject);
          IGC_ASSERT(testTransposedMemory(pAI->getAllocatedType(), pTypeOfAccessedObject, bufferSize,
                                          (m_ModAllocaInfo->getBufferStride(pAI))));
        } else {
          bufferSize = m_ModAllocaInfo->getBufferStride(pAI);
        }

        Value *simdBufferOffset = m_ModAllocaInfo->getOffset(builder, pAI, simdSize, simdLaneId, bufferSize);
        Value *bufferBase = addOffset(builder, DL, threadBase, simdBufferOffset);
        privateBufferPTR =
            builder.CreateIntToPtr(bufferBase, pAI->getAllocatedType()->getPointerTo(scratchMemoryAddressSpace),
                                   VALUE_NAME(pAI->getName() + ".privateBufferPTR"));
        // privateBuffer = builder.CreatePointerCast(privateBufferPTR, pAI->getType(), VALUE_NAME(pAI->getName() +
        // ".privateBuffer"));

        if (TransposeMemLayout) {
          TransposeHelperPrivateMem helper(DL, bufferBase, simdSize, bufferSize, pTypeOfAccessedObject->isVectorTy());
          Value *Idx = builder.getInt32(0);
          helper.HandleAllocaSources(pAI, Idx);
          helper.EraseDeadCode();
        }
      }

      // Replace all uses of original alloca with the bitcast
      Value *privateBuffer =
          builder.CreatePointerCast(privateBufferPTR, pAI->getType(), VALUE_NAME(pAI->getName() + ".privateBuffer"));
      pAI->replaceAllUsesWith(privateBuffer);
      pAI->eraseFromParent();

      if (scratchMemoryAddressSpace == ADDRESS_SPACE_GLOBAL) {
        // Fix address space in uses of privateBufferPTR, ADDRESS_SPACE_PRIVATE => ADDRESS_SPACE_GLOBAL
        FixAddressSpaceInAllUses(privateBufferPTR, ADDRESS_SPACE_GLOBAL, ADDRESS_SPACE_PRIVATE);
        privateBuffer->replaceAllUsesWith(privateBufferPTR);
        if (Instruction *inst = dyn_cast<Instruction>(privateBuffer)) {
          inst->eraseFromParent();
        }
      }
    }

    return true;
  }

  // Only OCL is supposed to reach here.
  IGC_ASSERT_EXIT(ShaderType::OPENCL_SHADER == Ctx.type);

  // Save the insert point to ensure appropriate sequence of instructions after getImplicitArgValue(),
  // because getImplicitArgValue() can move instructions, and it means that the insert point will be moved too.
  Instruction *pointInstr = &*entryBuilder.GetInsertPoint();
  if (pointInstr->isDebugOrPseudoInst())
    pointInstr = pointInstr->getNextNonDebugInstruction();
  if (GenIntrinsicInst *inst = dyn_cast_or_null<GenIntrinsicInst>(pointInstr)) {
    if (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_getR0 ||
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_getPrivateBase)
      pointInstr = inst->getNextNonDebugInstruction();
  }

  // Find the implicit argument representing r0 and the private memory base.
  Value *r0Val = implicitArgs.getImplicitArgValue(*m_currFunction, ImplicitArg::R0, m_pMdUtils);
  Value *privateMemPtr = implicitArgs.getImplicitArgValue(*m_currFunction, ImplicitArg::PRIVATE_BASE, m_pMdUtils);

  // Restore insert point saved before getImplicitArgValue()
  entryBuilder.SetInsertPoint(pointInstr);

  // Note: for debugging purposes privateMemPtr will be marked as Output to keep its liveness all time

  // Resolve the call

  // Receives:
  // %privateMem = alloca ...

  // Create a GEP to get to the right offset from the private memory base implicit arg:

  // %simdLaneId16                = call i16 @llvm.gen.simdLaneId()
  // %simdLaneId                  = zext i16 simdLaneId16 to i32
  // %simdSize                    = call i32 @llvm.gen.simdSize()
  // %totalPrivateMemPerThread    = mul i32 %simdSize, <totalPrivateMemPerWI>
  // %totalPrivateMemPerThread    = zext i32 %totalPrivateMemPerThread to i64

  // %r0.5                        = extractelement <8 x i32> %r0, i32 5
  // %threadId                    = and i32 %r0.5, 0x1FF|0x3FF   (Thread ID is in the lower 9 bits or 10 bit(KBL & CNL+)
  // of r0.5) %threadId                    = zext i32 %threadId to i64 %perThreadOffset             = mul i64 %threadId,
  // %totalPrivateMemPerThread
  Function *pHWTIDFunc = GenISAIntrinsic::getDeclaration(
      m_currFunction->getParent(), GenISAIntrinsic::GenISA_hw_thread_id_alloca, Type::getInt32Ty(C));
  Value *threadId = entryBuilder.CreateCall(pHWTIDFunc);

  if (Ctx.platform.supportTwoStackTSG() && IGC_IS_FLAG_ENABLED(EnableGen11TwoStackTSG)) {
    // Gen11 , 2 - stack configuration : (FFTID[9:0] << 1) | FFSID[0]) * scratch_size
    Value *shlThreadID = entryBuilder.CreateShl(threadId, ConstantInt::get(typeInt32, 1), VALUE_NAME("shlThreadID"));

    // FFSID - r0.0 bit 16
    Value *r0_0 = entryBuilder.CreateExtractElement(r0Val, ConstantInt::get(typeInt32, 0), VALUE_NAME("r0.0"));
    Value *FFSIDbit = entryBuilder.CreateLShr(r0_0, ConstantInt::get(typeInt32, 16), VALUE_NAME("FFSIDbit"));
    Value *FFSID = entryBuilder.CreateAnd(FFSIDbit, ConstantInt::get(typeInt32, 1), VALUE_NAME("FFSID"));

    threadId = entryBuilder.CreateOr(FFSID, shlThreadID, VALUE_NAME("threadId"));
  }
  Value *perThreadOffset = createThreadOffset(entryBuilder, threadId, totalPrivateMemPerWI);

  auto perThreadOffsetInst = dyn_cast_or_null<Instruction>(perThreadOffset);
  IGC_ASSERT_MESSAGE(perThreadOffsetInst, "perThreadOffset will not be marked as Output");
  if (perThreadOffsetInst) {
    // We add "perThreadOffset" metadata, so emitter will mark it as output to
    // extend it's living time to the end of the function. PrivateBase will be marked too.
    auto perThreadOffsetMD =
        MDNode::get(entryBuilder.getContext(), nullptr); // ConstantAsMetadata::get(entryBuilder.getInt32(1)));
    perThreadOffsetInst->setMetadata("perThreadOffset", perThreadOffsetMD);
  }

  for (auto pAI : allocaInsts) {
    // %bufferOffset                = mul i32 %simdSize, <scalarBufferOffset>
    // %bufferOffset                = zext i32 %bufferOffset to i64
    // %perLaneOffset               = mul i32 %simdLaneId, <bufferSize>
    // %perLaneOffset               = zext i32 %perLaneOffset to i64
    // %totalOffset                 = add i64 %bufferOffset, %perLaneOffset
    // %privateBufferGEP            = getelementptr i8* %threadBase, i64 %totalOffset
    // %privateBuffer               = bitcast i8* %privateBufferGEP to <buffer type>

    IGCLLVM::IRBuilder<> builder(pAI);
    builder.SetCurrentDebugLocation(entryDebugLoc);
    bool isUniform = pAI->getMetadata("uniform") != nullptr;
    // Get buffer information from the analysis
    Value *bufferBase;

    // If we can use SOA layout transpose the memory
    IGC::SOALayoutChecker SOAChecker(*pAI, Ctx.type == ShaderType::OPENCL_SHADER, OpenCLShaderStrategy);
    IGC::SOALayoutInfo SOAInfo = SOAChecker.getOrGatherInfo();
    if (!isUniform && SOAInfo.canUseSOALayout && SOAChecker.useNewAlgo(SOAInfo.baseType)) {
      uint32_t chunksize = SOAInfo.SOAPartitionBytes;
      Value *SIMDBufferOffset = m_ModAllocaInfo->getOffset(builder, pAI, simdSize, simdLaneId, chunksize);
      Value *totalOffset = addOffset(builder, DL, perThreadOffset, SIMDBufferOffset);

      // using offset, rather than ptr, is more efficient.
      Type *bTy = privateMemPtr->getType();
      IGC_ASSERT(bTy->isPointerTy());
      Value *baseAsInt = builder.CreatePtrToInt(privateMemPtr, DL.getIntPtrType(bTy), "privBaseInt");

      Value *baseOff = addOffset(builder, DL, baseAsInt, totalOffset);
      TransposePrivMem helper(DL, baseOff, simdSize, SOAInfo.SOAPartitionBytes);
      Value *Idx = builder.getInt32(0);
      helper.HandleAllocaSources(pAI, Idx);
      helper.EraseDeadCode();

      bufferBase = convertToPtr(builder, DL, baseOff, bTy);
    } else {
      Value *SIMDBufferOffset = m_ModAllocaInfo->getOffset(builder, pAI, simdSize, simdLaneId);
      Value *totalOffset = addOffset(builder, DL, perThreadOffset, SIMDBufferOffset);
      if (m_currFunction->getParent()->getDataLayout().getPointerSize() == 8) {
        // Manually zero-extend the offset to 64-bits to prevent it from being sign-extended by InstructionCombining
        totalOffset = builder.CreateZExt(totalOffset, typeInt64);
      }
      bufferBase = builder.CreateGEP(builder.getInt8Ty(), privateMemPtr, totalOffset,
                                     VALUE_NAME(pAI->getName() + ".privateBufferGEP"));
    }
    Value *privateBuffer =
        builder.CreatePointerCast(bufferBase, pAI->getType(), VALUE_NAME(pAI->getName() + ".privateBuffer"));

    // Attaching this metadata will make this location be inlined.
    // We can only safely inline such locations with optimizations disabled.
    // On O2 we have no guarantee the offsets in registers are gonna be valid throughout the entire variable lifetime.
    if (modMD->compOpt.OptDisable) {
      auto DbgDcls = llvm::FindDbgDeclareUses(pAI);
      for (auto DbgDcl : DbgDcls) {
        // Attach metadata to instruction containing offset of storage
        unsigned int scalarBufferOffset = m_ModAllocaInfo->getBufferOffset(pAI);
        auto OffsetMD =
            MDNode::get(builder.getContext(), ConstantAsMetadata::get(builder.getInt32(scalarBufferOffset)));
        DbgDcl->setMetadata("StorageOffset", OffsetMD);
      }
    }

    // Replace all uses of original alloca with the bitcast
    pAI->replaceAllUsesWith(privateBuffer);
    pAI->eraseFromParent();
  }

  return true;
}
