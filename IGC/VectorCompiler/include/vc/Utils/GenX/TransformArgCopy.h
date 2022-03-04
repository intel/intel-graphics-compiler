/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENX_TRANSFORMARGCOPY_H
#define VC_UTILS_GENX_TRANSFORMARGCOPY_H

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include <vector>

namespace vc {

void replaceUsesWithinFunction(
    const llvm::SmallDenseMap<llvm::Value *, llvm::Value *> &GlobalsToReplace,
    llvm::Function *F);

struct TransformedFuncType {
  llvm::SmallVector<llvm::Type *, 8> Ret;
  llvm::SmallVector<llvm::Type *, 8> Args;
};

enum class ArgKind { General, CopyIn, CopyInOut };
enum class GlobalArgKind { ByValueIn, ByValueInOut, ByPointer };

struct GlobalArgInfo {
  llvm::GlobalVariable *GV;
  GlobalArgKind Kind;
};

struct GlobalArgsInfo {
  static constexpr int UndefIdx = -1;
  std::vector<GlobalArgInfo> Globals;
  int FirstGlobalArgIdx = UndefIdx;

  GlobalArgInfo getGlobalInfoForArgNo(int ArgIdx) const;
  llvm::GlobalVariable *getGlobalForArgNo(int ArgIdx) const {
    return getGlobalInfoForArgNo(ArgIdx).GV;
  }
};

struct RetToArgInfo {
  static constexpr int OrigRetNoArg = -1;
  std::vector<int> Map;
};

struct ParameterAttrInfo {
  unsigned ArgIndex;
  llvm::Attribute::AttrKind Attr;
};

// Computing a new prototype for the function. E.g.
//
// i32 @foo(i32, <8 x i32>*) becomes {i32, <8 x i32>} @bar(i32, <8 x i32>)
//
class TransformedFuncInfo {
  TransformedFuncType NewFuncType;
  llvm::AttributeList Attrs;
  std::vector<ArgKind> ArgKinds;
  std::vector<ParameterAttrInfo> DiscardedParameterAttrs;
  RetToArgInfo RetToArg;
  GlobalArgsInfo GlobalArgs;

public:
  TransformedFuncInfo(llvm::Function &OrigFunc,
                      llvm::SmallPtrSetImpl<llvm::Argument *> &ArgsToTransform);
  void appendGlobals(llvm::SetVector<llvm::GlobalVariable *> &Globals);

  const TransformedFuncType &getType() const { return NewFuncType; }
  llvm::AttributeList getAttributes() const { return Attrs; }
  const std::vector<ArgKind> &getArgKinds() const { return ArgKinds; }
  const std::vector<ParameterAttrInfo> &getDiscardedParameterAttrs() const {
    return DiscardedParameterAttrs;
  }
  const GlobalArgsInfo &getGlobalArgsInfo() const { return GlobalArgs; }
  const RetToArgInfo &getRetToArgInfo() const { return RetToArg; }

private:
  void
  fillCopyInOutInfo(llvm::Function &OrigFunc,
                    llvm::SmallPtrSetImpl<llvm::Argument *> &ArgsToTransform);

  void inheritAttributes(llvm::Function &OrigFunc);

  void discardStructRetAttr(llvm::LLVMContext &Context);

  void appendRetCopyOutInfo();
};

llvm::Function *createTransformedFuncDecl(llvm::Function &OrigFunc,
                                          const TransformedFuncInfo &TFuncInfo);

class FuncUsersUpdater {
  llvm::Function &OrigFunc;
  llvm::Function &NewFunc;
  const TransformedFuncInfo &NewFuncInfo;
  llvm::CallGraphNode &NewFuncCGN;
  llvm::CallGraph &CG;

public:
  FuncUsersUpdater(llvm::Function &OrigFuncIn, llvm::Function &NewFuncIn,
                   const TransformedFuncInfo &NewFuncInfoIn,
                   llvm::CallGraphNode &NewFuncCGNIn, llvm::CallGraph &CGIn)
      : OrigFunc{OrigFuncIn}, NewFunc{NewFuncIn}, NewFuncInfo{NewFuncInfoIn},
        NewFuncCGN{NewFuncCGNIn}, CG{CGIn} {}

  void run();

private:
  llvm::CallInst *updateFuncDirectUser(llvm::CallInst &OrigCall);
};

class FuncBodyTransfer {
  llvm::Function &OrigFunc;
  llvm::Function &NewFunc;
  const TransformedFuncInfo &NewFuncInfo;

public:
  FuncBodyTransfer(llvm::Function &OrigFuncIn, llvm::Function &NewFuncIn,
                   const TransformedFuncInfo &NewFuncInfoIn)
      : OrigFunc{OrigFuncIn}, NewFunc{NewFuncIn}, NewFuncInfo{NewFuncInfoIn} {}
  void run();

private:
  std::vector<llvm::Value *> handleTransformedFuncArgs();
  void handleTransformedFuncRet(
      llvm::ReturnInst &OrigRet,
      const std::vector<llvm::Value *> &OrigArgReplacements,
      std::vector<llvm::Value *> &LocalizedGlobals);
  void handleTransformedFuncRets(
      const std::vector<llvm::Value *> &OrigArgReplacements,
      std::vector<llvm::Value *> &LocalizedGlobals);
};

} // namespace vc

#endif // VC_UTILS_GENX_TRANSFORMARGCOPY_H
