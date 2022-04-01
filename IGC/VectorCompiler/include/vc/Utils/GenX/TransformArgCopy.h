/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENX_TRANSFORMARGCOPY_H
#define VC_UTILS_GENX_TRANSFORMARGCOPY_H

#include "vc/Utils/GenX/TypeSize.h"

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

// Collect arguments that should be transformed.
llvm::SmallPtrSet<llvm::Argument *, 8>
collectArgsToTransform(llvm::Function &F, vc::TypeSizeWrapper MaxStructSize);

void replaceUsesWithinFunction(
    const llvm::SmallDenseMap<llvm::Value *, llvm::Value *> &GlobalsToReplace,
    llvm::Function *F);

struct TransformedFuncType {
  llvm::SmallVector<llvm::Type *, 8> Ret;
  llvm::SmallVector<llvm::Type *, 8> Args;
};

enum class ArgKind { General, CopyIn, CopyOut, CopyInOut };
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

// Data and helpers for ret to arg (both orig and new ones) connection.
// It doesn't contain ret info itself: ret_id is taken from position in vec.
// Check TransformedFuncInfo for more details.
class RetToArgLink {
  static constexpr int OmittedIdx = -1;
  const int NewIdx;
  const int OrigIdx;

  RetToArgLink(int NewIdxIn, int OrigIdxIn)
      : NewIdx{NewIdxIn}, OrigIdx{OrigIdxIn} {}

  // Check if the given idx is real one.
  static bool isRealIdx(int Idx);
  // Check if the given idx is omitted.
  static bool isOmittedIdx(int Idx);

public:
  // Create RetToArgLink for original return value.
  static RetToArgLink createForOrigRet();
  // Create RetToArgLink for global argument.
  // Global argument is a newly added one, thus it is omitted in
  // original call.
  static RetToArgLink createForGlobalArg(int NewIdx);
  // Create RetToArgLink for omitted argument.
  static RetToArgLink createForOmittedArg(int OrigIdx);
  // Create RetToArgLink with specific indices.
  static RetToArgLink createForLinkedArgs(int NewIdx, int OrigIdx);

  // Check if the link describes the original return value.
  bool isOrigRet() const;
  // Check if the link describes the global argument.
  bool isGlobalArg() const;
  // Check if the link describes the omitted argument.
  bool isOmittedArg() const;
  // Get new argument idx.
  int getNewIdx() const;
  // Get orig argument idx.
  int getOrigIdx() const;
};

// Information for existing original arg. OrigId is not stored here.
// Chech TransformedFuncInfo for more details.
class OrigArgInfo {
  static constexpr int OmittedIdx = -1;
  llvm::Type *TransformedOrigType;
  const ArgKind Kind;
  const int NewIdx;

public:
  OrigArgInfo(llvm::Type *TyIn, ArgKind KindIn, int NewIdxIn = OmittedIdx);

  // Check if the argument is omitted.
  bool isOmittedArg() const { return NewIdx == OmittedIdx; }
  // Get transformed original argument type.
  llvm::Type *getTransformedOrigType() const { return TransformedOrigType; }
  // Get argument kind.
  ArgKind getKind() const { return Kind; }
  // Get new argument idx.
  int getNewIdx() const;
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
  std::vector<ParameterAttrInfo> DiscardedParameterAttrs;
  std::vector<RetToArgLink> RetToArg;
  std::vector<OrigArgInfo> OrigArgs;
  GlobalArgsInfo GlobalArgs;

public:
  TransformedFuncInfo(llvm::Function &OrigFunc,
                      llvm::SmallPtrSetImpl<llvm::Argument *> &ArgsToTransform);
  void appendGlobals(llvm::SetVector<llvm::GlobalVariable *> &Globals);
  // Gather attributes for new function type according to transformations.
  llvm::AttributeList gatherAttributes(llvm::LLVMContext &Context,
                                       const llvm::AttributeList &AL) const;

  const TransformedFuncType &getType() const { return NewFuncType; }
  llvm::AttributeList getAttributes() const { return Attrs; }
  const std::vector<ParameterAttrInfo> &getDiscardedParameterAttrs() const {
    return DiscardedParameterAttrs;
  }
  const std::vector<RetToArgLink> &getRetToArgInfo() const { return RetToArg; }
  const std::vector<OrigArgInfo> &getOrigArgInfo() const { return OrigArgs; }
  const GlobalArgsInfo &getGlobalArgsInfo() const { return GlobalArgs; }

private:
  void
  fillOrigArgInfo(llvm::Function &OrigFunc,
                  llvm::SmallPtrSetImpl<llvm::Argument *> &ArgsToTransform);

  void inheritAttributes(llvm::Function &OrigFunc);

  // Sret may become inapplicable for transformed function.
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
