/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/BuiltinTypes.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Casting.h>
#include "llvmWrapper/IR/Type.h"
#include <llvm/Transforms/Utils/Cloning.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenPublicEnums.h"
#include "Probe/Assertion.h"

#include <algorithm>
#include <limits>

using namespace llvm;

namespace IGC {
bool isTargetExtTy(const llvm::Type *Ty) {
#if LLVM_VERSION_MAJOR >= 16
  return Ty->isTargetExtTy();
#endif
  return false;
}

bool isImageBuiltinType(const llvm::Type *BuiltinTy) {
  if (BuiltinTy->isPointerTy() && !IGCLLVM::isOpaquePointerTy(BuiltinTy))
    BuiltinTy = IGCLLVM::getNonOpaquePtrEltTy(BuiltinTy);

  if (const StructType *StructTy = dyn_cast<StructType>(BuiltinTy); StructTy && StructTy->isOpaque()) {
    StringRef BuiltinName = StructTy->getName();
    llvm::SmallVector<llvm::StringRef, 3> Buffer;
    BuiltinName.split(Buffer, ".");
    if (Buffer.size() < 2)
      return false;
    bool IsOpenCLImage = Buffer[0].equals("opencl") && Buffer[1].startswith("image") && Buffer[1].endswith("_t");
    bool IsSPIRVImage =
        Buffer[0].equals("spirv") && (Buffer[1].startswith("Image") || Buffer[1].startswith("SampledImage"));

    if (IsOpenCLImage || IsSPIRVImage)
      return true;
  }
#if LLVM_VERSION_MAJOR >= 16
  else if (const TargetExtType *ExtTy = dyn_cast<TargetExtType>(BuiltinTy);
           ExtTy && (ExtTy->getName() == "spirv.Image" || ExtTy->getName() == "spirv.SampledImage")) {
    return true;
  }
#endif

  return false;
}

#if LLVM_VERSION_MAJOR >= 16
static bool isNonOpenCLBuiltinType(const llvm::Type *Ty) {
  const llvm::TargetExtType *TET = dyn_cast<llvm::TargetExtType>(Ty);
  if (!TET)
    return false;

  StringRef Name = TET->getTargetExtName();
  return Name.starts_with("spirv.CooperativeMatrixKHR") || Name.starts_with("spirv.JointMatrixINTEL");
}

static bool isAnyArgOpenCLTargetExtTy(const llvm::Function &F) {
  for (const llvm::Argument &A : F.args()) {
    const Type *ArgTy = A.getType();
    if (isTargetExtTy(ArgTy) && !isNonOpenCLBuiltinType(ArgTy))
      return true;
  }

  return false;
}

static bool isDeclarationWithOpenCLTargetExtTyRet(const llvm::Function &F) {
  const Type *RetTy = F.getReturnType();
  return F.isDeclaration() && isTargetExtTy(RetTy) && !isNonOpenCLBuiltinType(RetTy);
}

static unsigned getAddressSpaceForTargetExtTy(const llvm::TargetExtType *TargetExtTy) {
  StringRef TyName = TargetExtTy->getName();
  if (TyName.startswith("spirv.Queue"))
    return ADDRESS_SPACE_PRIVATE;
  else if (TyName.startswith("spirv.Image"))
    return ADDRESS_SPACE_GLOBAL;
  else if (TyName.startswith("spirv.Sampler"))
    return ADDRESS_SPACE_CONSTANT;

  return 0;
}

static void retypeLocalTargetExtAllocasLoadsStores(Function &F) {
  SmallVector<AllocaInst *, 8> Allocas;
  SmallVector<LoadInst *, 16> Loads;
  SmallVector<StoreInst *, 16> Stores;

  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (auto *AI = dyn_cast<AllocaInst>(&I)) {
        Type *AllocTy = AI->getAllocatedType();
        if (isTargetExtTy(AllocTy) && !isNonOpenCLBuiltinType(AllocTy))
          Allocas.push_back(AI);
      } else if (auto *LI = dyn_cast<LoadInst>(&I)) {
        Type *ValTy = LI->getType();
        if (isTargetExtTy(ValTy) && !isNonOpenCLBuiltinType(ValTy))
          Loads.push_back(LI);
      } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
        Value *Val = SI->getValueOperand();
        Type *ValTy = Val->getType();
        if (isTargetExtTy(ValTy) && !isNonOpenCLBuiltinType(ValTy))
          Stores.push_back(SI);
      }
    }
  }

  DenseMap<Value *, Value *> RetypedValueMap;
  LLVMContext &C = F.getContext();

  // Retype allocas (preserve original names).
  for (AllocaInst *AI : Allocas) {
    std::string OldName = AI->hasName() ? std::string(AI->getName()) : std::string();
    const auto *TET = cast<TargetExtType>(AI->getAllocatedType());
    Type *PtrElemTy = PointerType::get(C, getAddressSpaceForTargetExtTy(cast<TargetExtType>(TET)));

    // Create new alloca without name to avoid temporary suffixing.
    AllocaInst *NewAI = new AllocaInst(PtrElemTy, AI->getAddressSpace(), nullptr, AI->getAlign(), "", AI);
    NewAI->setDebugLoc(AI->getDebugLoc());

    // Copy metadata
    SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
    AI->getAllMetadata(MDs);
    for (auto &MDPair : MDs)
      NewAI->setMetadata(MDPair.first, MDPair.second);

    RetypedValueMap[AI] = NewAI;
    AI->replaceAllUsesWith(NewAI);
    AI->eraseFromParent();

    if (!OldName.empty())
      NewAI->setName(OldName);
  }

  struct RetypedLoadInfo {
    LoadInst *Old;
    LoadInst *New;
    std::string OldName;
  };
  SmallVector<RetypedLoadInfo, 16> RetypedLoads;

  // Create replacement loads (do NOT RAUW directly due to type change).
  for (LoadInst *LI : Loads) {
    std::string OldName = LI->hasName() ? std::string(LI->getName()) : std::string();
    const auto *TET = cast<TargetExtType>(LI->getType());
    Type *NewValTy = PointerType::get(C, getAddressSpaceForTargetExtTy(cast<TargetExtType>(TET)));

    Value *PtrOp = LI->getPointerOperand();
    PointerType *DesiredPtrTy = PointerType::get(NewValTy, PtrOp->getType()->getPointerAddressSpace());
    if (PtrOp->getType() != DesiredPtrTy)
      PtrOp = new BitCastInst(PtrOp, DesiredPtrTy, PtrOp->getName() + ".retycast", LI);

    // Create unnamed new load before old one.
    LoadInst *NewLoad = new LoadInst(NewValTy, PtrOp, "", LI);
    NewLoad->setAlignment(LI->getAlign());
    NewLoad->setDebugLoc(LI->getDebugLoc());

    SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
    LI->getAllMetadata(MDs);
    for (auto &MDPair : MDs)
      NewLoad->setMetadata(MDPair.first, MDPair.second);

    RetypedValueMap[LI] = NewLoad;

    // Update non-store users immediately where the type already matches.
    SmallVector<Use *, 8> UsesToUpdate;
    for (Use &U : LI->uses())
      UsesToUpdate.push_back(&U);

    for (Use *U : UsesToUpdate) {
      User *Usr = U->getUser();
      if (isa<StoreInst>(Usr))
        continue; // Store handled later.
      if (U->get()->getType() == NewLoad->getType())
        *U = NewLoad;
    }

    RetypedLoads.push_back({LI, NewLoad, OldName});
  }

  // Retype stores whose value operand was retyped.
  for (StoreInst *SI : Stores) {
    Value *OldVal = SI->getValueOperand();
    auto *TET = cast<TargetExtType>(OldVal->getType());
    Type *NewValTy = PointerType::get(C, getAddressSpaceForTargetExtTy(cast<TargetExtType>(TET)));

    Value *NewVal = RetypedValueMap.lookup(OldVal);
    if (!NewVal)
      continue; // Producer not transformed, leave store as-is.

    Value *PtrOp = SI->getPointerOperand();
    PointerType *DesiredPtrTy = PointerType::get(NewValTy, PtrOp->getType()->getPointerAddressSpace());
    if (PtrOp->getType() != DesiredPtrTy)
      PtrOp = new BitCastInst(PtrOp, DesiredPtrTy, PtrOp->getName() + ".retycast", SI);

    StoreInst *NewStore = new StoreInst(NewVal, PtrOp, SI);
    NewStore->setAlignment(SI->getAlign());
    NewStore->setDebugLoc(SI->getDebugLoc());

    SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
    SI->getAllMetadata(MDs);
    for (auto &MDPair : MDs)
      NewStore->setMetadata(MDPair.first, MDPair.second);

    SI->eraseFromParent();
  }

  // Now safely remove old loads and restore original names.
  for (auto &RL : RetypedLoads) {
    if (RL.Old->use_empty()) {
      RL.Old->eraseFromParent();
      if (!RL.OldName.empty())
        RL.New->setName(RL.OldName);
    }
  }
}

static Function *cloneFunctionWithPtrArgsandRetInsteadTargetExtTy(Function &F, StringRef NameSuffix) {
  Module &M = *F.getParent();
  LLVMContext &C = M.getContext();

  SmallVector<Type *, 8> ParamTys;
  ParamTys.reserve(F.arg_size());
  for (Argument &Arg : F.args()) {
    Type *T = Arg.getType();
    if (isTargetExtTy(T) && !isNonOpenCLBuiltinType(T)) {
      T = PointerType::get(C, getAddressSpaceForTargetExtTy(cast<TargetExtType>(T)));
    }
    ParamTys.push_back(T);
  }

  Type *NewRetTy = F.getReturnType();
  if (F.isDeclaration() && isTargetExtTy(NewRetTy) && !isNonOpenCLBuiltinType(NewRetTy))
    NewRetTy = PointerType::get(C, getAddressSpaceForTargetExtTy(cast<TargetExtType>(NewRetTy)));

  FunctionType *NewFTy = FunctionType::get(NewRetTy, ParamTys, F.isVarArg());

  Function *NewF = Function::Create(NewFTy, F.getLinkage(), F.getAddressSpace(), F.getName() + NameSuffix, &M);
  NewF->copyAttributesFrom(&F);

  ValueToValueMapTy VMap;
  auto NI = NewF->arg_begin();
  for (Argument &OI : F.args()) {
    NI->setName(OI.getName());
    VMap[&OI] = &*NI++;
  }

  SmallVector<ReturnInst *, 8> Rets;
  CloneFunctionInto(NewF, &F, VMap, CloneFunctionChangeType::LocalChangesOnly, Rets);

  return NewF;
}

static void replaceFunctionAtCallsites(Function &OldF, Function &NewF) {
  // Helper for finding an earlier load in the same basic block that already loads the retyped (pointer) value from the
  // same address. The non-retyped loads still remain since it is not possible to retype callsites (their users) before
  // retyping function signatures.
  auto findEarlierRetypedLoad = [](LoadInst *OldLI) -> LoadInst * {
    if (!OldLI)
      return nullptr;
    Value *PtrOp = OldLI->getPointerOperand();
    for (Instruction *I = OldLI->getPrevNode(); I; I = I->getPrevNode()) {
      if (auto *LI = dyn_cast<LoadInst>(I)) {
        if (LI->getPointerOperand() == PtrOp && LI->getType()->isPointerTy())
          return LI;
      }
    }
    return nullptr;
  };

  SmallVector<User *, 16> Uses(OldF.users());

  for (User *U : Uses) {
    auto *CB = dyn_cast<CallBase>(U);
    if (!CB)
      continue;

    IRBuilder<> IRB(CB);
    SmallVector<Value *, 8> NewArgs;
    NewArgs.reserve(CB->arg_size());

    SmallVector<LoadInst *, 4> DeadTargetExtLoads;

    unsigned Idx = 0;
    for (Value *Actual : CB->args()) {
      const Argument &Formal = *std::next(NewF.arg_begin(), Idx++);
      Value *V = Actual;

      if (Formal.getType()->isPointerTy()) {
        unsigned FormalAS = Formal.getType()->getPointerAddressSpace();

        if (isTargetExtTy(V->getType())) {
          if (auto *C = dyn_cast<Constant>(V)) {
            if (C->isNullValue()) {
              V = ConstantPointerNull::get(PointerType::get(CB->getContext(), FormalAS));
              NewArgs.push_back(V);
              continue;
            }
          }
        }

        if (!V->getType()->isPointerTy()) {
          if (auto *LI = dyn_cast<LoadInst>(V)) {
            if (isTargetExtTy(LI->getType())) {
              // Try to find the already inserted retyped (pointer) load.
              if (LoadInst *Retyped = findEarlierRetypedLoad(LI)) {
                V = Retyped;
                DeadTargetExtLoads.push_back(LI);
                // Cast to the formal parameter type if needed.
                if (V->getType()->getPointerAddressSpace() != FormalAS)
                  V = IRB.CreateAddrSpaceCast(V, Formal.getType());
                else if (V->getType() != Formal.getType())
                  V = IRB.CreateBitCast(V, Formal.getType());
                NewArgs.push_back(V);
                continue;
              }
              // Fallback, use the address (pointer operand).
              // TODO: Remove this path. This should not be needed if the instruction-level retyping is done correctly.
              // Consider adding an assert.
              Value *PtrOp = LI->getPointerOperand();
              if (PtrOp->getType()->getPointerAddressSpace() != FormalAS) {
                PtrOp = IRB.CreateAddrSpaceCast(PtrOp, Formal.getType());
              } else if (PtrOp->getType() != Formal.getType()) {
                PtrOp = IRB.CreateBitCast(PtrOp, Formal.getType());
              }
              NewArgs.push_back(PtrOp);
              continue;
            }
          }
        }

        if (!V->getType()->isPointerTy()) {
          // TODO: Remove this path. This should not be needed if the instruction-level retyping is done correctly.
          // Consider adding an assert.
          IRBuilder<> EntryB(&*CB->getFunction()->getEntryBlock().begin());
          AllocaInst *Tmp = EntryB.CreateAlloca(V->getType(), nullptr, V->getName() + ".addr");
          EntryB.CreateStore(V, Tmp);
          V = Tmp;
        }

        if (V->getType()->getPointerAddressSpace() != FormalAS) {
          V = IRB.CreateAddrSpaceCast(V, Formal.getType());
        } else if (V->getType() != Formal.getType()) {
          V = IRB.CreateBitCast(V, Formal.getType());
        }
      }

      NewArgs.push_back(V);
    }

    CallBase *NewCall = IRB.CreateCall(NewF.getFunctionType(), &NewF, NewArgs);
    NewCall->copyMetadata(*CB);
    if (CB->getType() != NewCall->getType())
      CB->mutateType(NewCall->getType());
    NewCall->setCallingConv(CB->getCallingConv());
    NewCall->setAttributes(CB->getAttributes());

    CB->replaceAllUsesWith(NewCall);
    CB->eraseFromParent();

    // Remove now-dead TargetExtTy loads.
    for (LoadInst *L : DeadTargetExtLoads) {
      if (L->use_empty())
        L->eraseFromParent();
    }
  }
}

static bool structContainsOpenCLTargetExtTy(StructType *ST) {
  if (!ST || ST->isOpaque())
    return false;
  for (Type *Elt : ST->elements()) {
    if (isTargetExtTy(Elt) && !isNonOpenCLBuiltinType(Elt))
      return true;
    if (auto *NestedST = dyn_cast<StructType>(Elt))
      if (structContainsOpenCLTargetExtTy(NestedST))
        return true;
  }
  return false;
}

static Type *retypeElementIfNeeded(Type *EltTy, DenseMap<StructType *, StructType *> &StructMap);

static StructType *getOrCreateRetypedStruct(StructType *Old, DenseMap<StructType *, StructType *> &StructMap) {
  if (auto It = StructMap.find(Old); It != StructMap.end())
    return It->second;

  LLVMContext &C = Old->getContext();
  StructType *New = StructType::create(C, Old->getName());
  StructMap[Old] = New; // Insert early to break cycles.

  SmallVector<Type *, 8> NewElts;
  NewElts.reserve(Old->getNumElements());
  for (Type *Elt : Old->elements()) {
    NewElts.push_back(retypeElementIfNeeded(Elt, StructMap));
  }
  New->setBody(NewElts, Old->isPacked());
  return New;
}

static Type *retypeElementIfNeeded(Type *EltTy, DenseMap<StructType *, StructType *> &StructMap) {
  LLVMContext &C = EltTy->getContext();

  if (isTargetExtTy(EltTy) && !isNonOpenCLBuiltinType(EltTy))
    return PointerType::get(C, getAddressSpaceForTargetExtTy(cast<TargetExtType>(EltTy)));

  if (auto *ST = dyn_cast<StructType>(EltTy)) {
    if (structContainsOpenCLTargetExtTy(ST))
      return getOrCreateRetypedStruct(ST, StructMap);
  }
  return EltTy;
}

static void buildStructRetypeMap(Module &M, DenseMap<StructType *, StructType *> &StructMap) {
  std::vector<StructType *> Structs = M.getIdentifiedStructTypes();
  for (StructType *ST : Structs) {
    if (!ST || ST->isOpaque())
      continue;
    if (structContainsOpenCLTargetExtTy(ST))
      (void)getOrCreateRetypedStruct(ST, StructMap);
  }
}

static void replaceStructTypeUsesInFunction(Function &F, const DenseMap<StructType *, StructType *> &StructMap) {
  SmallVector<Instruction *, 32> ToErase;

  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (auto *GEP = dyn_cast<GetElementPtrInst>(&I)) {
        Type *SrcTy = GEP->getSourceElementType();
        auto *OldST = dyn_cast<StructType>(SrcTy);
        if (!OldST)
          continue;
        auto It = StructMap.find(OldST);
        if (It == StructMap.end())
          continue;

        SmallVector<Value *, 8> Indices(GEP->idx_begin(), GEP->idx_end());
        GetElementPtrInst *NewGEP =
            GetElementPtrInst::Create(It->second, GEP->getPointerOperand(), Indices, GEP->getName(), GEP);
        NewGEP->setIsInBounds(GEP->isInBounds());
        NewGEP->setDebugLoc(GEP->getDebugLoc());
        SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
        GEP->getAllMetadata(MDs);
        for (auto &P : MDs)
          NewGEP->setMetadata(P.first, P.second);

        GEP->replaceAllUsesWith(NewGEP);
        ToErase.push_back(GEP);
        continue;
      }

      if (auto *AI = dyn_cast<AllocaInst>(&I)) {
        auto *OldST = dyn_cast<StructType>(AI->getAllocatedType());
        if (!OldST)
          continue;
        auto It = StructMap.find(OldST);
        if (It == StructMap.end())
          continue;

        AllocaInst *NewAI =
            new AllocaInst(It->second, AI->getAddressSpace(), AI->getArraySize(), AI->getAlign(), AI->getName(), AI);
        NewAI->setDebugLoc(AI->getDebugLoc());
        SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
        AI->getAllMetadata(MDs);
        for (auto &P : MDs)
          NewAI->setMetadata(P.first, P.second);

        AI->replaceAllUsesWith(NewAI);
        ToErase.push_back(AI);
        continue;
      }

      if (auto *LI = dyn_cast<LoadInst>(&I)) {
        auto *OldST = dyn_cast<StructType>(LI->getType());
        if (!OldST)
          continue;
        auto It = StructMap.find(OldST);
        if (It == StructMap.end())
          continue;

        LoadInst *NewLoad = new LoadInst(It->second, LI->getPointerOperand(), LI->getName(), LI);
        NewLoad->setAlignment(LI->getAlign());
        NewLoad->setDebugLoc(LI->getDebugLoc());
        SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
        LI->getAllMetadata(MDs);
        for (auto &P : MDs)
          NewLoad->setMetadata(P.first, P.second);

        LI->replaceAllUsesWith(NewLoad);
        ToErase.push_back(LI);
        continue;
      }

      if (auto *SI = dyn_cast<StoreInst>(&I)) {
        auto *OldST = dyn_cast<StructType>(SI->getValueOperand()->getType());
        if (!OldST)
          continue;
        auto It = StructMap.find(OldST);
        if (It == StructMap.end())
          continue;

        // Only handle simple constant zero/undef cases safely.
        Value *V = SI->getValueOperand();
        Value *NewV = nullptr;
        if (auto *C = dyn_cast<Constant>(V)) {
          if (C->isNullValue())
            NewV = Constant::getNullValue(It->second);
          else if (isa<UndefValue>(C))
            NewV = UndefValue::get(It->second);
        }
        if (!NewV)
          continue; // TODO: Support complex producer chains.

        StoreInst *NewStore = new StoreInst(NewV, SI->getPointerOperand(), SI);
        NewStore->setAlignment(SI->getAlign());
        NewStore->setDebugLoc(SI->getDebugLoc());
        SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
        SI->getAllMetadata(MDs);
        for (auto &P : MDs)
          NewStore->setMetadata(P.first, P.second);

        ToErase.push_back(SI);
        continue;
      }
    }
  }

  for (Instruction *I : ToErase)
    I->eraseFromParent();
}

static AttributeList rewriteAttrListWithStructMap(LLVMContext &C, const AttributeList &AL, unsigned NumParams,
                                                  const DenseMap<StructType *, StructType *> &StructMap) {
  // Function and return attribute sets are preserved as-is.
  AttributeSet FnSet = AL.getFnAttrs();
  AttributeSet RetSet = AL.getRetAttrs();

  SmallVector<AttributeSet, 16> ParamSets;
  ParamSets.reserve(NumParams);

  for (unsigned I = 0; I < NumParams; ++I) {
    AttributeSet AS = AL.getParamAttrs(I);
    if (!AS.hasAttributes()) {
      ParamSets.emplace_back();
      continue;
    }

    SmallVector<llvm::Attribute, 8> NewAttrs;
    for (llvm::Attribute A : AS) {
      if (A.isTypeAttribute()) {
        Type *Ty = A.getValueAsType();
        if (auto *OldST = dyn_cast<StructType>(Ty)) {
          if (auto It = StructMap.find(OldST); It != StructMap.end()) {
            switch (A.getKindAsEnum()) {
            case llvm::Attribute::ByVal:
              A = llvm::Attribute::get(C, llvm::Attribute::ByVal, It->second);
              break;
            case llvm::Attribute::StructRet:
              A = llvm::Attribute::get(C, llvm::Attribute::StructRet, It->second);
              break;
            case llvm::Attribute::ByRef:
              A = llvm::Attribute::get(C, llvm::Attribute::ByRef, It->second);
              break;
            default:
              break;
            }
          }
        }
      }
      NewAttrs.push_back(A);
    }
    ParamSets.push_back(AttributeSet::get(C, NewAttrs));
  }

  return AttributeList::get(C, FnSet, RetSet, ParamSets);
}

static void rewriteFunctionSRetByValAttrs(Function &F, const DenseMap<StructType *, StructType *> &StructMap) {
  AttributeList Old = F.getAttributes();
  AttributeList New = rewriteAttrListWithStructMap(F.getContext(), Old, F.arg_size(), StructMap);
  if (Old != New)
    F.setAttributes(New);
}

static void rewriteCallSRetByValAttrs(CallBase &CB, const DenseMap<StructType *, StructType *> &StructMap) {
  AttributeList Old = CB.getAttributes();
  AttributeList New = rewriteAttrListWithStructMap(CB.getContext(), Old, CB.arg_size(), StructMap);
  if (Old != New)
    CB.setAttributes(New);
}

static void retypeStructsWithTargetExt(Module &M) {
  // Build struct retyping map for the whole module.
  DenseMap<StructType *, StructType *> StructMap;
  buildStructRetypeMap(M, StructMap);
  if (StructMap.empty())
    return;

  // Update instructions first (alloca/GEP/etc).
  for (Function &F : M) {
    if (F.isDeclaration())
      continue;
    replaceStructTypeUsesInFunction(F, StructMap);
  }

  // Fix sret/byval/byref attributes on function defs and call sites.
  for (Function &F : M) {
    rewriteFunctionSRetByValAttrs(F, StructMap);
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        if (auto *CB = dyn_cast<CallBase>(&I))
          rewriteCallSRetByValAttrs(*CB, StructMap);
      }
    }
  }

  // Old identified struct types should become unused and disappear from IR, no explicit erasure is required.
}

void retypeOpenCLTargetExtTyAsPointers(Module *M) {
  // Step 1: Retype local allocas/loads/stores of OpenCL TargetExtTy to use pointer types instead.
  for (Function &F : *M) {
    if (F.isDeclaration())
      continue;
    retypeLocalTargetExtAllocasLoadsStores(F);
  }

  constexpr StringLiteral TempSuffix = ".__retype_tmp";
  SmallVector<Function *, 8> RetypedFuncs;

  // Step 2: Retype function signatures of functions that use OpenCL TargetExtTy in args or return type to use pointer
  // types instead.
  for (Function &F : *M) {
    if (!isAnyArgOpenCLTargetExtTy(F) && !isDeclarationWithOpenCLTargetExtTyRet(F))
      continue;

    if (Function *NewF = cloneFunctionWithPtrArgsandRetInsteadTargetExtTy(F, TempSuffix))
      RetypedFuncs.push_back(NewF);
  }

  // Schedule replacement in call-site order to keep IR stable.
  DenseMap<const Function *, size_t> FirstUseIndex;
  size_t InstIndex = 0;
  for (Function &F : *M) {
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        if (auto *CB = dyn_cast<CallBase>(&I))
          if (const Function *Callee = CB->getCalledFunction())
            if (FirstUseIndex.find(Callee) == FirstUseIndex.end())
              FirstUseIndex[Callee] = InstIndex;
        ++InstIndex;
      }
    }
  }

  auto idxOf = [&](Function *NewF) {
    StringRef OrigName = NewF->getName().drop_back(TempSuffix.size());
    const Function *OldF = M->getFunction(OrigName);
    auto It = FirstUseIndex.find(OldF);
    return It == FirstUseIndex.end() ? std::numeric_limits<size_t>::max() : It->second;
  };

  std::stable_sort(RetypedFuncs.begin(), RetypedFuncs.end(),
                   [&](Function *A, Function *B) { return idxOf(A) < idxOf(B); });

  for (Function *NewF : RetypedFuncs) {
    std::string OriginalName = NewF->getName().drop_back(TempSuffix.size()).str();
    Function *OldF = M->getFunction(OriginalName);

    replaceFunctionAtCallsites(*OldF, *NewF);

    // Keep the original symbol name by swapping.
    Constant *NewFBitCast = ConstantExpr::getBitCast(NewF, OldF->getType());
    OldF->replaceAllUsesWith(NewFBitCast);

    OldF->setName(OriginalName + ".old");
    NewF->setName(OriginalName);
    OldF->eraseFromParent();
  }

  // Step 3: Retype struct types that contain OpenCL TargetExtTy fields and update users.
  retypeStructsWithTargetExt(*M);
}

#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
