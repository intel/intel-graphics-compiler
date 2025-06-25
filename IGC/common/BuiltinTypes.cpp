/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/BuiltinTypes.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>
#include "llvmWrapper/IR/Type.h"
#include <llvm/Transforms/Utils/Cloning.h>
#include "llvm/IR/IRBuilder.h"
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenPublicEnums.h"
#include "Probe/Assertion.h"

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

  if (const StructType *StructTy = dyn_cast<StructType>(BuiltinTy);
      StructTy && StructTy->isOpaque()) {
    StringRef BuiltinName = StructTy->getName();
    llvm::SmallVector<llvm::StringRef, 3> Buffer;
    BuiltinName.split(Buffer, ".");
    if (Buffer.size() < 2)
      return false;
    bool IsOpenCLImage = Buffer[0].equals("opencl") &&
                         Buffer[1].startswith("image") &&
                         Buffer[1].endswith("_t");
    bool IsSPIRVImage =
        Buffer[0].equals("spirv") &&
        (Buffer[1].startswith("Image") || Buffer[1].startswith("SampledImage"));

    if (IsOpenCLImage || IsSPIRVImage)
      return true;
  }
#if LLVM_VERSION_MAJOR >= 16
  else if (const TargetExtType *ExtTy = dyn_cast<TargetExtType>(BuiltinTy);
           ExtTy && (ExtTy->getName() == "spirv.Image" ||
                     ExtTy->getName() == "spirv.SampledImage")) {
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
  return Name.starts_with("spirv.CooperativeMatrixKHR") ||
         Name.starts_with("spirv.JointMatrixINTEL");
}

static bool isAnyArgOpenCLTargetExtTy(const llvm::Function &F) {
  for (const llvm::Argument &A : F.args()) {
    const Type *ArgTy = A.getType();
    if (isTargetExtTy(ArgTy) && !isNonOpenCLBuiltinType(ArgTy))
      return true;
  }

  return false;
}

static Function *
cloneFunctionWithPtrArgsInsteadTargetExtTy(Function &F, StringRef NameSuffix) {
  Module &M = *F.getParent();
  LLVMContext &C = M.getContext();

  SmallVector<Type *, 8> ParamTys;
  ParamTys.reserve(F.arg_size());
  for (Argument &Arg : F.args()) {
    Type *T = Arg.getType();
    if (isTargetExtTy(T) && !isNonOpenCLBuiltinType(T)) {
      unsigned AS = 0;
      auto *TargetExtTy = cast<llvm::TargetExtType>(T);
      StringRef TyName = TargetExtTy->getName();

      if (TyName.startswith("spirv.Queue"))
        AS = ADDRESS_SPACE_PRIVATE;
      else if (TyName.startswith("spirv.Image"))
        AS = ADDRESS_SPACE_GLOBAL;
      else if (TyName.startswith("spirv.Sampler"))
        AS = ADDRESS_SPACE_CONSTANT;

      T = PointerType::get(C, AS);
    }
    ParamTys.push_back(T);
  }
  FunctionType *NFTy =
      FunctionType::get(F.getReturnType(), ParamTys, F.isVarArg());

  Function *NewF = Function::Create(NFTy, F.getLinkage(), F.getAddressSpace(),
                                    F.getName() + NameSuffix, &M);
  NewF->copyAttributesFrom(&F);

  ValueToValueMapTy VMap;
  auto NI = NewF->arg_begin();
  for (Argument &OI : F.args()) {
    NI->setName(OI.getName());
    VMap[&OI] = &*NI++;
  }

  SmallVector<ReturnInst *, 8> Rets;
  CloneFunctionInto(NewF, &F, VMap, CloneFunctionChangeType::LocalChangesOnly,
                    Rets);

  return NewF;
}

static void replaceFunctionAtCallsites(Function &OldF, Function &NewF) {
  SmallVector<User *, 16> Uses(OldF.users());

  for (User *U : Uses) {
    auto *CB = dyn_cast<CallBase>(U);
    if (!CB)
      continue;

    IRBuilder<> IRB(CB);
    SmallVector<Value *, 8> NewArgs;
    NewArgs.reserve(CB->arg_size());

    unsigned Idx = 0;
    for (Value *Actual : CB->args()) {
      const Argument &Formal = *std::next(NewF.arg_begin(), Idx++);
      Value *V = Actual;

      if (Formal.getType()->isPointerTy()) {
        unsigned FormalAS = Formal.getType()->getPointerAddressSpace();

        if (!V->getType()->isPointerTy()) {
          IRBuilder<> EntryB(&*CB->getFunction()->getEntryBlock().begin());
          AllocaInst *Tmp = EntryB.CreateAlloca(V->getType(), nullptr,
                                                V->getName() + ".addr");
          EntryB.CreateStore(V, Tmp);

          V = Tmp;
        }

        if (V->getType()->getPointerAddressSpace() != FormalAS) {
          V = IRB.CreateAddrSpaceCast(V, Formal.getType());
        }
      }

      NewArgs.push_back(V);
    }

    CallBase *NewCall = IRB.CreateCall(NewF.getFunctionType(), &NewF, NewArgs);
    // TODO: Consider preserving metadata and tail-call kind here.
    // NewCall->setTailCallKind(CB->getTailCallKind());
    NewCall->copyMetadata(*CB);

    CB->replaceAllUsesWith(NewCall);
    CB->eraseFromParent();
  }
}

void retypeOpenCLTargetExtTyArgs(Module *M) {
  constexpr StringLiteral TempSuffix = ".__retype_tmp";
  SmallVector<Function *, 8> RetypedFuncs;

  for (Function &F : *M) {
    // TODO: Handle declarations separately. Skip here.
    // if (F.isDeclaration())
    //   continue;

    if (!isAnyArgOpenCLTargetExtTy(F))
      continue;

    if (Function *NewF =
            cloneFunctionWithPtrArgsInsteadTargetExtTy(F, TempSuffix))
      RetypedFuncs.push_back(NewF);
  }

  for (Function *NewF : RetypedFuncs) {
    StringRef OriginalName = NewF->getName().drop_back(TempSuffix.size());
    Function *OldF = M->getFunction(OriginalName);

    replaceFunctionAtCallsites(*OldF, *NewF);
    Constant *NewFBitCast = ConstantExpr::getBitCast(NewF, OldF->getType());
    OldF->replaceAllUsesWith(NewFBitCast);

    OldF->setName(OriginalName + ".old");
    NewF->setName(OriginalName);
    OldF->eraseFromParent();
  }
}
#endif

} // namespace IGC
