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
#include <llvm/Transforms/Utils/Cloning.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Type.h"

#include "Compiler/CodeGenPublicEnums.h"
#include "Probe/Assertion.h"

using namespace llvm;

namespace IGC {
bool isTargetExtTy(const Type *Ty) {
#if LLVM_VERSION_MAJOR >= 16
  return Ty->isTargetExtTy();
#endif
  return false;
}

bool isImageBuiltinType(const Type *BuiltinTy) {
  if (BuiltinTy->isPointerTy() && !IGCLLVM::isPointerTy(BuiltinTy))
    BuiltinTy = IGCLLVM::getNonOpaquePtrEltTy(BuiltinTy);

  if (const StructType *StructTy = dyn_cast<StructType>(BuiltinTy); StructTy && StructTy->isOpaque()) {
    StringRef BuiltinName = StructTy->getName();
    SmallVector<StringRef, 3> Buffer;
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
static bool isNonOpenCLBuiltinType(const Type *Ty) {
  const TargetExtType *TET = dyn_cast<TargetExtType>(Ty);
  if (!TET)
    return false;

  StringRef Name = TET->getTargetExtName();
  return Name.starts_with("spirv.CooperativeMatrixKHR") || Name.starts_with("spirv.JointMatrixINTEL");
}

static bool isOpenCLTargetExtType(const Type *Ty) { return isTargetExtTy(Ty) && !isNonOpenCLBuiltinType(Ty); }

static bool isStructWithOpenCLTargetExtTyInside(const Type *Ty) {
  const StructType *ST = dyn_cast<StructType>(Ty);
  if (!ST || ST->isOpaque())
    return false;

  for (Type *EltTy : ST->elements()) {
    if (isOpenCLTargetExtType(EltTy))
      return true;

    if (auto *NestedST = dyn_cast<StructType>(EltTy))
      if (isStructWithOpenCLTargetExtTyInside(NestedST))
        return true;
  }
  return false;
}

static bool checkIfNeedsRetyping(const Type *Ty) {
  return isOpenCLTargetExtType(Ty) || isStructWithOpenCLTargetExtTyInside(Ty);
}

static bool isAnyArgOpenCLTargetExtTy(const Function &F) {
  for (const Argument &A : F.args()) {
    const Type *ArgTy = A.getType();
    if (checkIfNeedsRetyping(ArgTy))
      return true;

    if (A.hasStructRetAttr() && checkIfNeedsRetyping(A.getParamStructRetType()))
      return true;

    if (A.hasByValAttr() && checkIfNeedsRetyping(A.getParamByValType()))
      return true;

    if (A.hasByRefAttr() && checkIfNeedsRetyping(A.getParamByRefType()))
      return true;
  }

  return false;
}

namespace {
class OpenCLTargetExtTypeMapper : public ValueMapTypeRemapper {
public:
  OpenCLTargetExtTypeMapper(Function &F, DenseMap<StructType *, StructType *> &TETtoRetypedStructs)
      : Fn(F), Ctx(F.getContext()), TETtoRetypedStructs(TETtoRetypedStructs) {}

  Type *remapType(Type *SrcTy) override {
    if (!SrcTy)
      return SrcTy;

    if (auto *FTy = dyn_cast<FunctionType>(SrcTy))
      return remapFunctionType(FTy);

    if (auto *TET = dyn_cast<TargetExtType>(SrcTy))
      return remapTargetExtType(TET);

    if (auto *ST = dyn_cast<StructType>(SrcTy))
      return remapStructType(ST);

    // Possibly no need to retype, otherwise new cases need to be added (above).
    return SrcTy;
  }

private:
  Function &Fn;
  LLVMContext &Ctx;
  DenseMap<StructType *, StructType *> &TETtoRetypedStructs;

  FunctionType *remapFunctionType(FunctionType *FTy) {
    SmallVector<Type *, 6> NewParamTys;
    NewParamTys.reserve(FTy->getNumParams());
    bool AnyChange = false;

    for (Type *ParamTy : FTy->params()) {
      Type *NewParamTy = remapType(ParamTy);
      if (NewParamTy != ParamTy)
        AnyChange = true;
      NewParamTys.push_back(NewParamTy);
    }

    Type *RetType = FTy->getReturnType();
    Type *NewRetTy = remapType(RetType);
    if (NewRetTy != RetType)
      AnyChange = true;

    if (!AnyChange) {
      return FTy;
    }
    return FunctionType::get(NewRetTy, NewParamTys, FTy->isVarArg());
  }

  Type *remapTargetExtType(TargetExtType *TET) {
    if (isNonOpenCLBuiltinType(TET))
      return TET;

    StringRef TyName = TET->getName();
    unsigned AS = ADDRESS_SPACE_PRIVATE;
    if (TyName.startswith("spirv.Image"))
      AS = ADDRESS_SPACE_GLOBAL;
    else if (TyName.startswith("spirv.Sampler"))
      AS = ADDRESS_SPACE_CONSTANT;

    return PointerType::get(Ctx, AS);
  }

  Type *remapStructType(StructType *StructTy) {
    if (!StructTy || StructTy->isOpaque()) {
      return StructTy;
    }

    // Scan first to avoid unnecessary retyping/cloning.
    if (!isStructWithOpenCLTargetExtTyInside(StructTy))
      return StructTy;

    return cloneStructRetyped(StructTy);
  }

  StructType *cloneStructRetyped(StructType *Old) {
    // Reuse mapping if already retyped.
    auto It = TETtoRetypedStructs.find(Old);
    if (It != TETtoRetypedStructs.end()) {
      return It->second;
    }

    std::string OrigName = Old->getName().str();
    Old->setName(OrigName + ".preretype");

    // Early insert placeholder to break cycles.
    StructType *NewST = StructType::create(Ctx, OrigName);
    TETtoRetypedStructs[Old] = NewST;

    SmallVector<Type *, 8> NewElems;
    NewElems.reserve(Old->getNumElements());
    bool AnyChange = false;
    for (Type *Elt : Old->elements()) {
      Type *NewElt = remapType(Elt);
      if (NewElt != Elt)
        AnyChange = true;
      NewElems.push_back(NewElt);
    }

    if (!AnyChange) {
      // No change, reuse original and discard temp.
      TETtoRetypedStructs[Old] = Old;
      return Old;
    }

    NewST->setBody(NewElems, Old->isPacked());
    return NewST;
  }
};
} // namespace

void retypeOpenCLTargetExtTyAsPointers(Module *M) {
  struct FunctionSignatureChange {
    FunctionType *NewFuncTy;
    AttributeList NewAttrs;
  };

  // Global mapping between TargetExtTy structs and their retyped variant (they are shared between functions).
  DenseMap<StructType *, StructType *> TETtoRetypedStructs;
  MapVector<Function *, FunctionSignatureChange> PendingSigChanges;

  // Remap bodies and collect function signature changes.
  for (Function &F : *M) {
    bool ArgsOrRetTypeNeedsRetyping = isAnyArgOpenCLTargetExtTy(F) || checkIfNeedsRetyping(F.getReturnType());

    // Need to process declarations that have TargetExtTy return/args, skip others.
    if (F.isDeclaration() && !ArgsOrRetTypeNeedsRetyping)
      continue;

    // Scan function to see if it uses TargetExtTy.
    bool UsesTargetExt = ArgsOrRetTypeNeedsRetyping;
    if (!UsesTargetExt && !F.isDeclaration()) {
      for (BasicBlock &BB : F) {
        for (Instruction &I : BB) {
          Type *Ty = I.getType();
          if (checkIfNeedsRetyping(Ty)) {
            UsesTargetExt = true;
            break;
          }
          // Also scan operand types (structs carrying TargetExt).
          for (Value *Op : I.operands()) {
            if (auto *OpTy = Op->getType(); checkIfNeedsRetyping(OpTy)) {
              UsesTargetExt = true;
              break;
            }
          }
          if (auto *GEP = dyn_cast<GetElementPtrInst>(&I)) {
            Type *SrcElemTy = GEP->getSourceElementType();
            if (checkIfNeedsRetyping(SrcElemTy)) {
              UsesTargetExt = true;
              break;
            }
          }
          if (auto *AI = dyn_cast<AllocaInst>(&I)) {
            Type *AllocatedTy = AI->getAllocatedType();
            if (checkIfNeedsRetyping(AllocatedTy)) {
              UsesTargetExt = true;
              break;
            }
          }

          if (UsesTargetExt)
            break;
        }
        if (UsesTargetExt)
          break;
      }
    }

    // If neither args/return/instructions use OpenCL TargetExtTy, skip.
    if (!UsesTargetExt)
      continue;

    OpenCLTargetExtTypeMapper Mapper(F, TETtoRetypedStructs);
    ValueToValueMapTy VM;

    // Handle constants of target extension types.
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        for (Use &U : I.operands()) {
          if (Constant *C = dyn_cast<Constant>(U.get())) {
            Type *Ty = C->getType();
            if (checkIfNeedsRetyping(Ty)) {
              Type *NewTy = Mapper.remapType(Ty);
              if (NewTy != Ty) {
                VM[C] = Constant::getNullValue(NewTy);
              }
            }
          }
        }
      }
    }

    RemapFunction(F, VM, RF_IgnoreMissingLocals | RF_ReuseAndMutateDistinctMDs, &Mapper);

    // We only need to replace function whose signature changes.
    if (ArgsOrRetTypeNeedsRetyping) {
      // Remap function argument and return types.
      FunctionType *NewFTy = cast<FunctionType>(Mapper.remapType(F.getFunctionType()));

      // Remap types used in attributes.
      AttributeList OldAttrs = F.getAttributes();
      SmallVector<AttributeSet, 8> NewArgAttrs;
      NewArgAttrs.reserve(OldAttrs.getNumAttrSets());
      bool AttrsChanged = false;

      for (const Argument &Arg : F.args()) {
        AttributeSet Attrs = OldAttrs.getParamAttrs(Arg.getArgNo());
        if (Attrs.hasAttribute(llvm::Attribute::StructRet)) {
          Type *OldSRetTy = Arg.getParamStructRetType();
          Type *NewSRetTy = Mapper.remapType(OldSRetTy);
          if (NewSRetTy != OldSRetTy) {
            AttrBuilder AB(M->getContext(), Attrs);
            AB.removeAttribute(llvm::Attribute::StructRet);
            AB.addStructRetAttr(NewSRetTy);
            Attrs = AttributeSet::get(M->getContext(), AB);
            AttrsChanged = true;
          }
        }
        if (Attrs.hasAttribute(llvm::Attribute::ByVal)) {
          Type *OldByValTy = Arg.getParamByValType();
          Type *NewByValTy = Mapper.remapType(OldByValTy);
          if (NewByValTy != OldByValTy) {
            AttrBuilder AB(M->getContext(), Attrs);
            AB.removeAttribute(llvm::Attribute::ByVal);
            AB.addByValAttr(NewByValTy);
            Attrs = AttributeSet::get(M->getContext(), AB);
            AttrsChanged = true;
          }
        }
        if (Attrs.hasAttribute(llvm::Attribute::ByRef)) {
          Type *OldByRefTy = Arg.getParamByRefType();
          Type *NewByRefTy = Mapper.remapType(OldByRefTy);
          if (NewByRefTy != OldByRefTy) {
            AttrBuilder AB(M->getContext(), Attrs);
            AB.removeAttribute(llvm::Attribute::ByRef);
            AB.addByRefAttr(NewByRefTy);
            Attrs = AttributeSet::get(M->getContext(), AB);
            AttrsChanged = true;
          }
        }
        NewArgAttrs.push_back(Attrs);
      }

      AttributeList NewAttrs =
          AttrsChanged ? AttributeList::get(M->getContext(), OldAttrs.getFnAttrs(), OldAttrs.getRetAttrs(), NewArgAttrs)
                       : OldAttrs;

      PendingSigChanges.insert(std::make_pair(&F, FunctionSignatureChange{NewFTy, NewAttrs}));
    }
  }

  // Replace functions with changed signatures.
  for (auto &KV : PendingSigChanges) {
    Function *OldF = KV.first;
    FunctionSignatureChange Change = KV.second;

    // Preserve original name to restore after erasing OldF.
    std::string OldName = OldF->getName().str();

    // Create new function with same linkage & addr space (temporary unique name).
    Function *NewF = Function::Create(Change.NewFuncTy, OldF->getLinkage(), OldF->getAddressSpace(), OldName, M);

    // Set remapped attributes.
    NewF->setAttributes(Change.NewAttrs);

    // Copy calling convention, comdat.
    NewF->setCallingConv(OldF->getCallingConv());
    if (OldF->getComdat())
      NewF->setComdat(OldF->getComdat());

    // Transfer debug subprogram (if any).
    if (OldF->getSubprogram()) {
      NewF->setSubprogram(OldF->getSubprogram());
      OldF->setSubprogram(nullptr);
    }

    // Copy all function-level metadata (except dbg already handled via Subprogram).
    {
      SmallVector<std::pair<unsigned, MDNode *>, 8> MDs;
      OldF->getAllMetadata(MDs);
      unsigned DbgKind = M->getContext().getMDKindID("dbg");
      for (auto &MDPair : MDs) {
        if (MDPair.first == DbgKind)
          continue; // Avoid duplicating debug info.
        NewF->setMetadata(MDPair.first, MDPair.second);
      }
    }

    // Move body (for definitions).
    if (!OldF->isDeclaration()) {
      NewF->splice(NewF->begin(), OldF);
    }

    // Map arguments.
    auto OldIt = OldF->arg_begin();
    auto NewIt = NewF->arg_begin();
    for (; OldIt != OldF->arg_end(); ++OldIt, ++NewIt) {
      NewIt->takeName(&*OldIt);
      OldIt->replaceAllUsesWith(&*NewIt);
    }

    // Redirect users then remove old.
    OldF->replaceAllUsesWith(NewF);
    OldF->eraseFromParent();

    // Restore original name.
    if (NewF->getName() != OldName)
      NewF->setName(OldName);
  }
}

#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
