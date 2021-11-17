/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/General/DebugInfo.h"
#include "vc/Utils/GenX/TypeSize.h"

#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>

#include <llvmWrapper/IR/DerivedTypes.h>

#include "Probe/Assertion.h"

#define DEBUG_TYPE "VC_DEBUG_INFO_UTILS"

using namespace llvm;

bool vc::DIBuilder::checkIfModuleHasDebugInfo(const llvm::Module &M) {
  unsigned NumDebugCUs =
      std::distance(M.debug_compile_units_begin(), M.debug_compile_units_end());

  if (NumDebugCUs == 0)
    return false;

  IGC_ASSERT_MESSAGE(NumDebugCUs == 1,
                     "only modules with one CU are supported at the moment:");
  return NumDebugCUs == 1;
}

bool vc::DIBuilder::checkIfFunctionHasDebugInfo(const llvm::Function &F) {
  DISubprogram *SP = F.getSubprogram();
  if (!SP)
    return false;
  IGC_ASSERT(checkIfModuleHasDebugInfo(*F.getParent()));
  return true;
}

llvm::DbgDeclareInst *vc::DIBuilder::createDbgDeclareForLocalizedGlobal(
    llvm::Instruction &Address, const llvm::GlobalVariable &GV,
    llvm::Instruction &InsertPt) {
  auto &Fn = *Address.getFunction();

  if (!vc::DIBuilder::checkIfFunctionHasDebugInfo(Fn))
    return nullptr;

  SmallVector<DIGlobalVariableExpression *, 1> GVEs;
  GV.getDebugInfo(GVEs);
  if (GVEs.empty())
    return nullptr;
  IGC_ASSERT(GVEs.size() == 1);

  DIGlobalVariableExpression &GVE = *GVEs.front();
  const DIGlobalVariable &DIDebugVariable = *GVE.getVariable();
  DIExpression &DIExpr = *GVE.getExpression();

  vc::DIBuilder DBuilder(*Address.getModule());

  auto *DILocalVar = DBuilder.createLocalVariable(
      Fn.getSubprogram(), DIDebugVariable.getName(), DIDebugVariable.getFile(),
      0 /*LineNo*/, DIDebugVariable.getType(), 0 /*ArgNo*/,
      DINode::DIFlags::FlagArtificial, DIDebugVariable.getAlignInBits());
  auto *DILoc = DBuilder.createLocationForFunctionScope(Fn);
  return DBuilder.createDbgDeclare(Address, *DILocalVar, DIExpr, *DILoc,
                                   InsertPt);
}

void vc::DIBuilder::registerNewGlobalVariable(
    llvm::DIGlobalVariableExpression *NewGV) const {
  IGC_ASSERT(NewGV);
  auto *CU = getDICompileUnit();
  IGC_ASSERT(CU);

  auto OldGlobals = CU->getGlobalVariables();
  SmallVector<Metadata *, 4> NewGlobals;
  std::copy(OldGlobals.begin(), OldGlobals.end(),
            std::back_inserter(NewGlobals));
  NewGlobals.push_back(NewGV);

  CU->replaceGlobalVariables(MDTuple::get(M.getContext(), NewGlobals));
}

DILocation *vc::DIBuilder::createLocationForFunctionScope(Function &Fn) const {
  IGC_ASSERT(checkIfFunctionHasDebugInfo(Fn));
  auto *SP = Fn.getSubprogram();
  auto *DILoc = DILocation::get(SP->getContext(), SP->getScopeLine(), 0, SP);
  return DILoc;
}

llvm::DICompileUnit *vc::DIBuilder::getDICompileUnit() const {
  IGC_ASSERT_MESSAGE(checkIfModuleHasDebugInfo(M), "debug info required!");
  return *M.debug_compile_units_begin();
}

DIType *vc::DIBuilder::translateScalarTypeToDIType(Type &Ty) const {
  IGC_ASSERT(!Ty.isVectorTy());
  if (!Ty.isIntegerTy()) {
    LLVM_DEBUG(dbgs() << "ERROR: could not derive DIType for: " << Ty << "\n");
    return nullptr;
  }
  auto SizeInBits = vc::getTypeSize(&Ty, &M.getDataLayout()).inBits();
  return DIBasicType::get(
      M.getContext(), dwarf::DW_TAG_base_type, ("ui" + Twine(SizeInBits)).str(),
      SizeInBits, 0 /*Align*/, dwarf::DW_ATE_unsigned, DINode::FlagZero);
}

DIType *vc::DIBuilder::translateTypeToDIType(Type &Ty) const {
  if (!Ty.isVectorTy())
    return translateScalarTypeToDIType(Ty);

  auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(&Ty);
  if (!VTy) {
    LLVM_DEBUG(dbgs() << "ERROR: could not derive DIType for: " << Ty << "\n");
    return nullptr;
  }
  auto *ScalarTy = VTy->getScalarType();
  auto *ScalarDI = translateScalarTypeToDIType(*ScalarTy);
  if (!ScalarDI) {
    LLVM_DEBUG(dbgs() << "ERROR: could not derive element DIType for: " << *VTy
                      << "\n");
    return nullptr;
  }

  auto &Ctx = M.getContext();
  auto NumElements = VTy->getNumElements();
  auto *SubrangeDI = DISubrange::get(Ctx, NumElements, 0 /*Lo*/);
  auto *Subscripts = MDTuple::get(Ctx, SubrangeDI);
  auto SizeInBits = vc::getTypeSize(VTy, &M.getDataLayout()).inBits();
  auto *CompositeTypeDI = DICompositeType::get(
      Ctx, dwarf::DW_TAG_array_type, "" /*Name*/, nullptr /*File*/, 0 /*Line*/,
      nullptr /*Scope*/, ScalarDI, SizeInBits, 0 /*AlignInBits*/,
      0 /*OfffsetInBits*/, DINode::FlagVector, Subscripts, 0, nullptr);
  return CompositeTypeDI;
}

llvm::DIGlobalVariableExpression *vc::DIBuilder::createGlobalVariableExpression(
    StringRef Name, StringRef LinkageName, DIType *Type) const {
  auto *CU = getDICompileUnit();
  IGC_ASSERT(CU);
  auto &Ctx = M.getContext();

  auto *GV = DIGlobalVariable::getDistinct(
      Ctx, cast_or_null<DIScope>(CU), Name, LinkageName, CU->getFile(),
      0 /*Line No*/, Type, true /*IsLocalToUnit*/, true /*isDefined*/,
      nullptr /*Decl*/, nullptr /*TemplateParams*/, 0 /*AlignInBits*/);
  auto *EmptyExpr = DIExpression::get(Ctx, llvm::None);
  auto *GVE = DIGlobalVariableExpression::get(Ctx, GV, EmptyExpr);

  // all globals should be registered in DICompileUnit::globals
  registerNewGlobalVariable(GVE);

  return GVE;
}

llvm::DbgDeclareInst *
vc::DIBuilder::createDbgDeclare(Value &Address, DILocalVariable &LocalVar,
                                DIExpression &Expr, DILocation &Loc,
                                Instruction &InsertPt) const {
  auto &Ctx = M.getContext();

  Value *DeclareArgs[] = {
      MetadataAsValue::get(Ctx, ValueAsMetadata::get(&Address)),
      MetadataAsValue::get(Ctx, &LocalVar), MetadataAsValue::get(Ctx, &Expr)};

  auto *DbgDeclareFn = Intrinsic::getDeclaration(&M, Intrinsic::dbg_declare);
  IRBuilder<> Builder(&InsertPt);
  Builder.SetCurrentDebugLocation(&Loc);
  auto *DeclareInst = Builder.CreateCall(DbgDeclareFn, DeclareArgs);
  return cast<DbgDeclareInst>(DeclareInst);
}

llvm::DILocalVariable *vc::DIBuilder::createLocalVariable(
    llvm::DILocalScope *Scope, llvm::StringRef Name, llvm::DIFile *File,
    unsigned LineNo, llvm::DIType *Type, unsigned ArgNo,
    llvm::DINode::DIFlags Flags, unsigned AlignInBits) const {
  return DILocalVariable::get(M.getContext(), Scope, Name, File, LineNo, Type,
                              ArgNo, Flags, AlignInBits);
}
