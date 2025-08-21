/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "llvm/Config/llvm-config.h"
#include "AdaptorCommon/RayTracing/RayTracingAddressSpaceAliasAnalysis.h"
#include "Compiler/Optimizer/OpenCLPasses/AddressSpaceAliasAnalysis/AddressSpaceAliasAnalysis.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/Analysis/AliasAnalysis.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

namespace {
class AddressSpaceAAResult : public IGCLLVM::AAResultBaseWrapper<AddressSpaceAAResult> {
  using BaseT = IGCLLVM::AAResultBaseWrapper<AddressSpaceAAResult>;
  friend BaseT;
  const TargetLibraryInfo &TLI;
  const CodeGenContext &CGC;

public:
  explicit AddressSpaceAAResult(const TargetLibraryInfo &TLI, const CodeGenContext &ctx)
      : BaseT(), TLI(TLI), CGC(ctx) {}
  AddressSpaceAAResult(AddressSpaceAAResult &&Arg) : BaseT(std::move(Arg)), TLI(Arg.TLI), CGC(Arg.CGC) {}

  AddressSpaceAAResult(const AddressSpaceAAResult &) = delete;
  AddressSpaceAAResult &operator=(const AddressSpaceAAResult &) = delete;
  AddressSpaceAAResult &operator=(AddressSpaceAAResult &&) = delete;

  IGCLLVM::AliasResultEnum alias(const MemoryLocation &LocA, const MemoryLocation &LocB, AAQueryInfo &AAQI,
#if LLVM_VERSION_MAJOR < 16
                                 const llvm::Instruction *CtxI = nullptr
#else
                                 const llvm::Instruction *CtxI
#endif
  ) {
    // DO NOT strip any casting as the address space is encoded in pointer
    // type. For `addrspacecast`, the current implementation in LLVM is too
    // aggressive to strip them. With address space resolution, we should not
    // have trivial cases where a non-generic pointer is cased into a generic
    // one.

    PointerType *PtrTy1 = dyn_cast<PointerType>(LocA.Ptr->getType());
    PointerType *PtrTy2 = dyn_cast<PointerType>(LocB.Ptr->getType());

    if (!PtrTy1 || !PtrTy2)
      return IGCLLVM::AliasResultEnum::NoAlias;

    unsigned AS1 = PtrTy1->getAddressSpace();
    unsigned AS2 = PtrTy2->getAddressSpace();

    // If LocA and LocB are pointers to different non-generic address spaces
    // then they do not alias.  This is not true in general, however, in OpenCL
    // all address spaces except generic are disjoint.
    //
    // Address spaces greater than ADDRESS_SPACE_NUM_ADDRESSES are used for
    // stateful accesses and may alias.
    //
    if (AS1 < ADDRESS_SPACE_NUM_ADDRESSES && AS2 < ADDRESS_SPACE_NUM_ADDRESSES && AS1 != ADDRESS_SPACE_GENERIC &&
        AS2 != ADDRESS_SPACE_GENERIC && AS1 != AS2) {
      bool isDisjointMemory = true;
      if (CGC.allocatePrivateAsGlobalBuffer() && CGC.getModuleMetaData()->genericAccessesResolved) {
        const bool isPrivateGlobal = (AS1 == ADDRESS_SPACE_PRIVATE && AS2 == ADDRESS_SPACE_GLOBAL) ||
                                     (AS1 == ADDRESS_SPACE_GLOBAL && AS2 == ADDRESS_SPACE_PRIVATE);

        isDisjointMemory = !isPrivateGlobal;
      }

      if (isDisjointMemory)
        return IGCLLVM::AliasResultEnum::NoAlias;
    }

    // Shared local memory doesn't alias any stateful memory.
    if ((AS1 == ADDRESS_SPACE_LOCAL && AS2 > ADDRESS_SPACE_NUM_ADDRESSES) ||
        (AS1 > ADDRESS_SPACE_NUM_ADDRESSES && AS2 == ADDRESS_SPACE_LOCAL)) {
      return IGCLLVM::AliasResultEnum::NoAlias;
    }

    // Private memory doesn't alias any stateful memory
    if ((AS1 == ADDRESS_SPACE_PRIVATE && AS2 > ADDRESS_SPACE_NUM_ADDRESSES) ||
        (AS1 > ADDRESS_SPACE_NUM_ADDRESSES && AS2 == ADDRESS_SPACE_PRIVATE)) {
      return IGCLLVM::AliasResultEnum::NoAlias;
    }

    /// For some client APIs (e.g. vulkan) compiler is free to assume that
    /// resources bound to two different bindings points never alias unless a
    /// resource is explicitly marked as being aliased.
    IGC_ASSERT(CGC.getModuleMetaData());
    if (AS1 > ADDRESS_SPACE_NUM_ADDRESSES && AS2 > ADDRESS_SPACE_NUM_ADDRESSES &&
        CGC.getModuleMetaData()->statefulResourcesNotAliased) {
      bool isDirectAccess[2] = {false, false};
      unsigned resourceIndex[2] = {0, 0};
      BufferType resourceType[2] = {BUFFER_TYPE_UNKNOWN, BUFFER_TYPE_UNKNOWN};

      resourceType[0] = DecodeAS4GFXResource(AS1, isDirectAccess[0], resourceIndex[0]);
      resourceType[1] = DecodeAS4GFXResource(AS2, isDirectAccess[1], resourceIndex[1]);

      /// Returns true if resource type is a texture, constant buffer or a storage buffer.
      auto IsBufferOrTexture = [](BufferType type) {
        return (type == BufferType::CONSTANT_BUFFER || type == BufferType::UAV || type == BufferType::RESOURCE);
      };

      if (IsBufferOrTexture(resourceType[0]) && IsBufferOrTexture(resourceType[1])) {
        if ((resourceType[0] != resourceType[1]) || // different resource types
            (isDirectAccess[0] && isDirectAccess[1] &&
             resourceIndex[0] != resourceIndex[1])) // direct access to different BTIs
        {
          return IGCLLVM::AliasResultEnum::NoAlias;
        }
      }
    }

    return BaseT::alias(LocA, LocB, AAQI, CtxI);
  }

  bool pointsToConstantMemory(const llvm::MemoryLocation &Loc, AAQueryInfo &AAQI, bool OrLocal) {
    // Pointers to constant address space memory, well, point to constant memory
    PointerType *ptrType = dyn_cast<PointerType>(Loc.Ptr->getType());
    if (ptrType) {
      if (ptrType->getAddressSpace() == ADDRESS_SPACE_CONSTANT)
        return true;

      bool DirectIdx;
      unsigned BufId;
      BufferType BufTy = DecodeAS4GFXResource(ptrType->getAddressSpace(), DirectIdx, BufId);
      if (BufTy == CONSTANT_BUFFER)
        return true;
    }

    return BaseT::pointsToConstantMemory(Loc, AAQI, OrLocal);
  }
};

class AddressSpaceAAWrapperPass : public ImmutablePass {
  std::unique_ptr<AddressSpaceAAResult> Result;

public:
  static char ID;

  AddressSpaceAAWrapperPass();

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
    AU.addRequired<TargetLibraryInfoWrapperPass>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  bool doInitialization(Module &M) override {
    if (M.size() > 0) {
      auto &F = *M.begin(); // see llvmWrapper/Analysis/TargetLibraryInfo.h
      Result.reset(new AddressSpaceAAResult(getAnalysis<TargetLibraryInfoWrapperPass>().getTLI(),
                                            *getAnalysis<CodeGenContextWrapper>().getCodeGenContext()));
    }
    return false;
  }

  bool doFinalization(Module &M) override {
    Result.reset();
    return false;
  }

  AddressSpaceAAResult &getResult() { return *Result; }
  const AddressSpaceAAResult &getResult() const { return *Result; }
};

} // End anonymous namespace

#define PASS_FLAG "igc-address-space-alias-analysis"
#define PASS_DESC "Address space alias analysis"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(AddressSpaceAAWrapperPass, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_END(AddressSpaceAAWrapperPass, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

char AddressSpaceAAWrapperPass::ID = 0;

AddressSpaceAAWrapperPass::AddressSpaceAAWrapperPass() : ImmutablePass(ID) {
  initializeAddressSpaceAAWrapperPassPass(*PassRegistry::getPassRegistry());
}

ImmutablePass *IGC::createAddressSpaceAAWrapperPass() { return new AddressSpaceAAWrapperPass(); }

void IGC::addJointAddressSpaceAAResults(Pass &P, Function &F, AAResults &AAR) {
  if (auto *WrapperPass = P.getAnalysisIfAvailable<AddressSpaceAAWrapperPass>())
    AAR.addAAResult(WrapperPass->getResult());
  addRayTracingAddressSpaceAAResult(P, F, AAR);
}

// Wrapper around LLVM's ExternalAAWrapperPass so that the default constructor
// gets the callback.
class IGCExternalAAWrapper : public ExternalAAWrapperPass {
public:
  static char ID;

  IGCExternalAAWrapper() : ExternalAAWrapperPass(&addJointAddressSpaceAAResults) {}
};

ImmutablePass *IGC::createIGCExternalAAWrapper() { return new IGCExternalAAWrapper(); }

char IGCExternalAAWrapper::ID = 0;

IGC_INITIALIZE_PASS_BEGIN(IGCExternalAAWrapper, "igc-aa-wrapper", "IGC Address space alias analysis Wrapper",
                          PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(IGCExternalAAWrapper, "igc-aa-wrapper", "IGC Address space alias analysis Wrapper",
                        PASS_CFG_ONLY, PASS_ANALYSIS)
