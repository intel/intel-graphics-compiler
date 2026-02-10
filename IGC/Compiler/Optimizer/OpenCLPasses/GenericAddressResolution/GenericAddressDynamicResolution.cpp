/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/GenericAddressResolution/GenericAddressDynamicResolution.hpp"
#include "Compiler/CISACodeGen/CastToGASAnalysis.h"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Support/Alignment.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using IGCLLVM::getAlign;

namespace {
class GenericAddressDynamicResolution : public FunctionPass {
public:
  static char ID;

  GenericAddressDynamicResolution() : FunctionPass(ID) {}
  ~GenericAddressDynamicResolution() = default;

  StringRef getPassName() const override { return "GenericAddressDynamicResolution"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<CastToGASAnalysis>();
  }

  bool runOnFunction(Function &F) override;

private:
  bool visitLoadStoreInst(Instruction &I);
  bool visitIntrinsicCall(CallInst &I);

  Module *m_module = nullptr;
  CodeGenContext *m_ctx = nullptr;
  llvm::SmallVector<Instruction *, 32> generatedLoadStores;
  bool m_needPrivateBranches = false;
  bool m_needLocalBranches = false;

  uint64_t m_numAdditionalControlFlows = 0;

  Type *getPointerAsIntType(LLVMContext &Ctx, unsigned AS);
  void emitVerboseWarning(Instruction &I);
  void resolveGAS(Instruction &I, Value *pointerOperand);
  void resolveGASWithoutBranches(Instruction &I, Value *pointerOperand);
};
} // namespace

// Register pass to igc-opt
#define PASS_FLAG "igc-generic-address-dynamic-resolution"
#define PASS_DESCRIPTION "Resolve generic address space loads/stores"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(GenericAddressDynamicResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CastToGASAnalysis)
IGC_INITIALIZE_PASS_END(GenericAddressDynamicResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char GenericAddressDynamicResolution::ID = 0;

bool GenericAddressDynamicResolution::runOnFunction(Function &F) {
  m_module = F.getParent();
  m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  m_numAdditionalControlFlows = 0;

  GASInfo &GI = getAnalysis<CastToGASAnalysis>().getGASInfo();
  m_needPrivateBranches = !GI.isPrivateAllocatedInGlobalMemory() && GI.canGenericPointToPrivate(F);
  m_needLocalBranches = !GI.isNoLocalToGenericOptionEnabled() && GI.canGenericPointToLocal(F);

  bool modified = false;

  llvm::SmallVector<Instruction *, 32> callInstructions;
  llvm::SmallVector<Instruction *, 32> loadStoreInstructions;

  for (auto &instruction : llvm::instructions(F)) {

    if (isa<CallInst>(instruction)) {
      callInstructions.push_back(&instruction);
    } else if (isa<LoadInst>(instruction) || isa<StoreInst>(instruction)) {
      loadStoreInstructions.push_back(&instruction);
    }
  }
  // iterate for all the intrinisics used by to_local, to_global, and to_private
  for (auto *callInst : callInstructions) {
    modified |= visitIntrinsicCall(cast<CallInst>(*callInst));
  }

  // iterate over all loads/stores with generic address space pointers
  for (auto *loadStoreInst : loadStoreInstructions) {
    modified |= visitLoadStoreInst(*loadStoreInst);
  }

  // iterate over all newly generated load/stores
  while (!generatedLoadStores.empty()) {
    llvm::SmallVector<Instruction *, 32> newInstructions = generatedLoadStores;
    generatedLoadStores.clear();
    for (auto *loadStoreInst : newInstructions) {
      modified |= visitLoadStoreInst(*loadStoreInst);
    }
  }

  if (m_numAdditionalControlFlows) {
    std::stringstream warningInfo;
    warningInfo << "Adding ";
    warningInfo << m_numAdditionalControlFlows;
    warningInfo << " occurrences of additional control flow due to presence of generic address space operations\n";
    warningInfo << "in function " << F.getName().str();
    warningInfo << " (Enable PrintVerboseGenericControlFlowLog flag to acquire detailed log. Requires debuginfo!)";
    getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->EmitWarning(warningInfo.str().c_str());
  }

  if (modified) {
    m_ctx->getModuleMetaData()->genericAccessesResolved = true;
  }

  return modified;
}

Type *GenericAddressDynamicResolution::getPointerAsIntType(LLVMContext &ctx, const unsigned AS) {
  DataLayout dataLayout = m_module->getDataLayout();
  unsigned ptrBits(dataLayout.getPointerSizeInBits(AS));
  return IntegerType::get(ctx, ptrBits);
}

bool GenericAddressDynamicResolution::visitLoadStoreInst(Instruction &I) {
  bool changed = false;

  Value *pointerOperand = nullptr;
  unsigned int pointerAddressSpace = ADDRESS_SPACE_NUM_ADDRESSES;

  if (auto *load = dyn_cast<LoadInst>(&I)) {
    pointerOperand = load->getPointerOperand();
    pointerAddressSpace = load->getPointerAddressSpace();
  } else if (auto *store = dyn_cast<StoreInst>(&I)) {
    pointerOperand = store->getPointerOperand();
    pointerAddressSpace = store->getPointerAddressSpace();
  } else {
    IGC_ASSERT_EXIT_MESSAGE(0, "Unable to resolve generic address space pointer");
  }

  if (pointerAddressSpace == ADDRESS_SPACE_GENERIC) {
    if (!m_needPrivateBranches && !m_needLocalBranches) {
      resolveGASWithoutBranches(I, pointerOperand);
    } else {
      resolveGAS(I, pointerOperand);
    }
    changed = true;
  }

  return changed;
}

void GenericAddressDynamicResolution::emitVerboseWarning(Instruction &I) {
  std::stringstream warningInfo;
  llvm::DILocation *dbInfo = I.getDebugLoc();
  llvm::Instruction *prevInst = I.getPrevNode();
  bool debugInfoFound = false;

  while (dbInfo == nullptr) {
    if (prevInst == nullptr) {
      break;
    }
    dbInfo = prevInst->getDebugLoc();
    prevInst = prevInst->getPrevNode();
  }
  if (dbInfo != nullptr) {
    warningInfo << "from dir:" << dbInfo->getDirectory().str();
    warningInfo << " from file:" << dbInfo->getFilename().str();
    warningInfo << " line:" << dbInfo->getLine();
    warningInfo << " :";
    debugInfoFound = true;
  }

  warningInfo << "Adding additional control flow due to presence of generic address space operations";

  if (!debugInfoFound)
    warningInfo << "\nVerbose log requires debuginfo!";

  getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->EmitWarning(warningInfo.str().c_str());
}

void GenericAddressDynamicResolution::resolveGAS(Instruction &I, Value *pointerOperand) {
  if (m_ctx->m_instrTypes.hasDebugInfo && IGC_IS_FLAG_ENABLED(PrintVerboseGenericControlFlowLog))
    emitVerboseWarning(I);
  else
    m_numAdditionalControlFlows++;

  // Every time there is a load/store from/to a generic pointer, we have to resolve
  // its corresponding address space by looking at its tag on bits[61:63].
  // First, the generic pointer's tag is obtained to then perform the load/store
  // with the corresponding address space.

  IGCLLVM::IRBuilder<> builder(&I);
  auto *pointerType = dyn_cast<PointerType>(pointerOperand->getType());
  IGC_ASSERT(pointerType != nullptr);
  ConstantInt *privateTag = builder.getInt64(1); // tag 001
  ConstantInt *localTag = builder.getInt64(2);   // tag 010

  Type *intPtrTy = getPointerAsIntType(pointerOperand->getContext(), ADDRESS_SPACE_GENERIC);
  Value *ptrAsInt = PtrToIntInst::Create(Instruction::PtrToInt, pointerOperand, intPtrTy, "", &I);
  // Get actual tag
  Value *tag = builder.CreateLShr(ptrAsInt, ConstantInt::get(ptrAsInt->getType(), 61));

  // Three cases for private, local and global pointers
  BasicBlock *currentBlock = I.getParent();
  BasicBlock *convergeBlock = currentBlock->splitBasicBlock(&I);

  BasicBlock *privateBlock = nullptr;
  BasicBlock *localBlock = nullptr;
  BasicBlock *globalBlock = nullptr;

  Value *localLoad = nullptr;
  Value *privateLoad = nullptr;
  Value *globalLoad = nullptr;

  // GAS needs to resolve to private only if
  //     1) private is NOT allocated in global space; and
  //     2) there is a cast from private to GAS.
  bool needPrivateBranch = m_needPrivateBranches;
  bool needLocalBranch = m_needLocalBranches;

  auto createBlock = [&](const Twine &BlockName, const Twine &LoadName, IGC::ADDRESS_SPACE addressSpace, Value *&load) {
    BasicBlock *BB = BasicBlock::Create(I.getContext(), BlockName, convergeBlock->getParent(), convergeBlock);
    builder.SetInsertPoint(BB);
    PointerType *ptrType = IGCLLVM::get(pointerType, addressSpace);
    Value *ptr = builder.CreateAddrSpaceCast(pointerOperand, ptrType);
    Instruction *generatedLoadStore = nullptr;

    if (auto *LI = dyn_cast<LoadInst>(&I)) {
      load = builder.CreateAlignedLoad(LI->getType(), ptr, getAlign(*LI), LI->isVolatile(), LoadName);
      generatedLoadStore = cast<Instruction>(load);
    } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
      generatedLoadStore = builder.CreateAlignedStore(I.getOperand(0), ptr, getAlign(*SI), SI->isVolatile());
    }
    if (generatedLoadStore != nullptr) {
      generatedLoadStores.push_back(generatedLoadStore);
    }
    builder.CreateBr(convergeBlock);
    return BB;
  };

  // Private branch
  if (needPrivateBranch) {
    privateBlock = createBlock("PrivateBlock", "privateLoad", ADDRESS_SPACE_PRIVATE, privateLoad);
  }

  // Local Branch
  if (needLocalBranch) {
    localBlock = createBlock("LocalBlock", "localLoad", ADDRESS_SPACE_LOCAL, localLoad);
  }

  // Global Branch
  globalBlock = createBlock("GlobalBlock", "globalLoad", ADDRESS_SPACE_GLOBAL, globalLoad);

  currentBlock->getTerminator()->eraseFromParent();
  builder.SetInsertPoint(currentBlock);

  const int numCases = (needPrivateBranch && needLocalBranch) ? 2 : ((needPrivateBranch || needLocalBranch) ? 1 : 0);
  IGC_ASSERT(0 < numCases);

  {
    SwitchInst *switchTag = builder.CreateSwitch(tag, globalBlock, numCases);
    // Based on tag there are two cases 001: private, 010: local, 000/111: global
    if (needPrivateBranch) {
      switchTag->addCase(privateTag, privateBlock);
    }
    if (needLocalBranch) {
      switchTag->addCase(localTag, localBlock);
    }

    if (isa<LoadInst>(&I)) {
      IGCLLVM::IRBuilder<> phiBuilder(&(*convergeBlock->begin()));
      PHINode *phi = phiBuilder.CreatePHI(I.getType(), numCases + 1, I.getName());
      if (privateLoad) {
        phi->addIncoming(privateLoad, privateBlock);
      }
      if (localLoad) {
        phi->addIncoming(localLoad, localBlock);
      }
      phi->addIncoming(globalLoad, globalBlock);
      I.replaceAllUsesWith(phi);
    }
  }

  I.eraseFromParent();
}

void GenericAddressDynamicResolution::resolveGASWithoutBranches(Instruction &I, Value *pointerOperand) {
  IGCLLVM::IRBuilder<> builder(&I);
  auto *pointerType = dyn_cast<PointerType>(pointerOperand->getType());
  IGC_ASSERT(pointerType != nullptr);

  Value *nonLocalLoad = nullptr;

  PointerType *ptrType = IGCLLVM::get(pointerType, ADDRESS_SPACE_GLOBAL);
  Value *globalPtr = builder.CreateAddrSpaceCast(pointerOperand, ptrType);
  Instruction *generatedLoadStore = nullptr;

  if (auto *LI = dyn_cast<LoadInst>(&I)) {
    nonLocalLoad =
        builder.CreateAlignedLoad(LI->getType(), globalPtr, getAlign(*LI), LI->isVolatile(), "globalOrPrivateLoad");
    generatedLoadStore = cast<Instruction>(nonLocalLoad);
  } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
    generatedLoadStore = builder.CreateAlignedStore(I.getOperand(0), globalPtr, getAlign(*SI), SI->isVolatile());
  }
  if (generatedLoadStore != nullptr) {
    generatedLoadStores.push_back(generatedLoadStore);
  }
  if (nonLocalLoad != nullptr) {
    I.replaceAllUsesWith(nonLocalLoad);
  }
  I.eraseFromParent();
}

bool GenericAddressDynamicResolution::visitIntrinsicCall(CallInst &I) {
  bool changed = false;
  Function *pCalledFunc = I.getCalledFunction();
  if (pCalledFunc == nullptr) {
    // Indirect call
    return false;
  }

  StringRef funcName = pCalledFunc->getName();

  if ((funcName == "__builtin_IB_to_private") || (funcName == "__builtin_IB_to_local") ||
      (funcName == "__builtin_IB_to_global")) {
    IGC_ASSERT(IGCLLVM::getNumArgOperands(&I) == 1);
    Value *arg = I.getArgOperand(0);
    auto *dstType = dyn_cast<PointerType>(I.getType());
    IGC_ASSERT(dstType != nullptr);
    const unsigned targetAS = cast<PointerType>(I.getType())->getAddressSpace();

    IGCLLVM::IRBuilder<> builder(&I);
    auto *pointerType = dyn_cast<PointerType>(arg->getType());
    IGC_ASSERT(pointerType != nullptr);
    ConstantInt *globalTag = builder.getInt64(0);  // tag 000/111
    ConstantInt *privateTag = builder.getInt64(1); // tag 001
    ConstantInt *localTag = builder.getInt64(2);   // tag 010

    Type *intPtrTy = getPointerAsIntType(arg->getContext(), ADDRESS_SPACE_GENERIC);
    Value *ptrAsInt = PtrToIntInst::Create(Instruction::PtrToInt, arg, intPtrTy, "", &I);
    // Get actual tag
    Value *tag = builder.CreateLShr(ptrAsInt, ConstantInt::get(ptrAsInt->getType(), 61));

    Value *newPtr = nullptr;
    Value *newPtrNull = nullptr;
    Value *cmpTag = nullptr;

    if (targetAS == ADDRESS_SPACE_GLOBAL || targetAS == ADDRESS_SPACE_PRIVATE) {
      // Force distinguishing private and global pointers if a kernel uses explicit casts.
      // For more details please refer to section "Generic Address Space Explicit Casts" in
      // documentation directory under igc/generic-pointers/generic-pointers.md
      auto *ClContext = static_cast<OpenCLProgramContext *>(m_ctx);
      ClContext->setDistinguishBetweenPrivateAndGlobalPtr(true);
    }

    // Tag was already obtained from GAS pointer, now we check its address space (AS)
    // and the target AS for this intrinsic call
    if (targetAS == ADDRESS_SPACE_PRIVATE)
      cmpTag = builder.CreateICmpEQ(tag, privateTag, "cmpTag");
    else if (targetAS == ADDRESS_SPACE_LOCAL)
      cmpTag = builder.CreateICmpEQ(tag, localTag, "cmpTag");
    else if (targetAS == ADDRESS_SPACE_GLOBAL)
      cmpTag = builder.CreateICmpEQ(tag, globalTag, "cmpTag");

    // Two cases:
    // 1: Generic pointer's AS matches with instrinsic's target AS
    //    So we create the address space cast
    // 2: Generic pointer's AS does not match with instrinsic's target AS
    //    So the instrinsic call returns NULL
    BasicBlock *currentBlock = I.getParent();
    BasicBlock *convergeBlock = currentBlock->splitBasicBlock(&I);
    BasicBlock *ifBlock = BasicBlock::Create(I.getContext(), "IfBlock", convergeBlock->getParent(), convergeBlock);
    BasicBlock *elseBlock = BasicBlock::Create(I.getContext(), "ElseBlock", convergeBlock->getParent(), convergeBlock);

    // If Block
    {
      IRBuilder<> ifBuilder(ifBlock);
      PointerType *ptrType = IGCLLVM::get(pointerType, targetAS);
      newPtr = ifBuilder.CreateAddrSpaceCast(arg, ptrType);
      ifBuilder.CreateBr(convergeBlock);
    }

    // Else Block
    {
      IRBuilder<> elseBuilder(elseBlock);
      Value *ptrNull = Constant::getNullValue(I.getType());
      newPtrNull = elseBuilder.CreatePointerCast(ptrNull, dstType, "");
      elseBuilder.CreateBr(convergeBlock);
    }

    currentBlock->getTerminator()->eraseFromParent();
    builder.SetInsertPoint(currentBlock);
    builder.CreateCondBr(cmpTag, ifBlock, elseBlock);

    IRBuilder<> phiBuilder(&(*convergeBlock->begin()));
    PHINode *phi = phiBuilder.CreatePHI(I.getType(), 2, I.getName());
    phi->addIncoming(newPtr, ifBlock);
    phi->addIncoming(newPtrNull, elseBlock);
    I.replaceAllUsesWith(phi);
    I.eraseFromParent();
    changed = true;
  }

  return changed;
}

namespace IGC {
FunctionPass *createGenericAddressDynamicResolutionPass() { return new GenericAddressDynamicResolution; }
} // namespace IGC
