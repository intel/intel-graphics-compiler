/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define DEBUG_TYPE "vc-transform-arg-copy"

#include "llvmWrapper/Analysis/CallGraph.h"
#include "llvmWrapper/IR/CallSite.h"
#include "llvmWrapper/IR/Instructions.h"

#include "Probe/Assertion.h"

#include "vc/Utils/GenX/TransformArgCopy.h"
#include "vc/Utils/General/DebugInfo.h"
#include "vc/Utils/General/FunctionAttrs.h"
#include "vc/Utils/General/Types.h"

#include <llvm/ADT/STLExtras.h>
#include <llvm/GenXIntrinsics/GenXIntrinsics.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/User.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/Debug.h>

#include <algorithm>
#include <iterator>
#include <numeric>
#include <utility>

using namespace llvm;
using namespace vc;

// Replaces uses of global variables with the corresponding allocas inside a
// specified function. More insts can be rebuild if global variable addrspace
// wasn't private.
void vc::replaceUsesWithinFunction(
    const SmallDenseMap<Value *, Value *> &GlobalsToReplace, Function *F) {
  for (auto &BB : *F) {
    for (auto &Inst : BB) {
      for (unsigned i = 0, e = Inst.getNumOperands(); i < e; ++i) {
        Value *Op = Inst.getOperand(i);
        auto Iter = GlobalsToReplace.find(Op);
        if (Iter != GlobalsToReplace.end()) {
          IGC_ASSERT_MESSAGE(Op->getType() == Iter->second->getType(),
                             "only global variables in private addrspace are "
                             "localized, so types must match");
          Inst.setOperand(i, Iter->second);
        }
      }
    }
  }
}

GlobalArgInfo vc::GlobalArgsInfo::getGlobalInfoForArgNo(int ArgIdx) const {
  IGC_ASSERT_MESSAGE(FirstGlobalArgIdx != UndefIdx,
                     "first global arg index isn't set");
  auto Idx = ArgIdx - FirstGlobalArgIdx;
  IGC_ASSERT_MESSAGE(Idx >= 0, "out of bound access");
  IGC_ASSERT_MESSAGE(Idx < static_cast<int>(Globals.size()),
                     "out of bound access");
  return Globals[ArgIdx - FirstGlobalArgIdx];
}

vc::TransformedFuncInfo::TransformedFuncInfo(
    Function &OrigFunc, SmallPtrSetImpl<Argument *> &ArgsToTransform) {
  fillCopyInOutInfo(OrigFunc, ArgsToTransform);
  std::transform(OrigFunc.arg_begin(), OrigFunc.arg_end(),
                 std::back_inserter(NewFuncType.Args),
                 [&ArgsToTransform](Argument &Arg) {
                   if (ArgsToTransform.count(&Arg))
                     return Arg.getType()->getPointerElementType();
                   return Arg.getType();
                 });
  inheritAttributes(OrigFunc);

  // struct-returns are not supported for transformed functions,
  // so we need to discard the attribute
  if (OrigFunc.hasStructRetAttr() && OrigFunc.hasLocalLinkage())
    discardStructRetAttr(OrigFunc.getContext());

  auto *OrigRetTy = OrigFunc.getFunctionType()->getReturnType();
  if (!OrigRetTy->isVoidTy()) {
    NewFuncType.Ret.push_back(OrigRetTy);
    RetToArg.Map.push_back(RetToArgInfo::OrigRetNoArg);
  }
  appendRetCopyOutInfo();
}

// Whether provided \p GV should be passed by pointer.
static bool passLocalizedGlobalByPointer(const llvm::GlobalValue &GV) {
  auto *Type = GV.getType()->getPointerElementType();
  return Type->isAggregateType();
}

void vc::TransformedFuncInfo::appendGlobals(
    SetVector<GlobalVariable *> &Globals) {
  IGC_ASSERT_MESSAGE(GlobalArgs.FirstGlobalArgIdx == GlobalArgsInfo::UndefIdx,
                     "can only be initialized once");
  GlobalArgs.FirstGlobalArgIdx = NewFuncType.Args.size();
  for (auto *GV : Globals) {
    if (passLocalizedGlobalByPointer(*GV)) {
      NewFuncType.Args.push_back(vc::changeAddrSpace(
          cast<PointerType>(GV->getType()), vc::AddrSpace::Private));
      GlobalArgs.Globals.push_back({GV, GlobalArgKind::ByPointer});
    } else {
      int ArgIdx = NewFuncType.Args.size();
      Type *PointeeTy = GV->getType()->getPointerElementType();
      NewFuncType.Args.push_back(PointeeTy);
      if (GV->isConstant())
        GlobalArgs.Globals.push_back({GV, GlobalArgKind::ByValueIn});
      else {
        GlobalArgs.Globals.push_back({GV, GlobalArgKind::ByValueInOut});
        NewFuncType.Ret.push_back(PointeeTy);
        RetToArg.Map.push_back(ArgIdx);
      }
    }
  }
}

// Returns true if data is only read using load-like intrinsics. The result may
// be false negative.
static bool isSinkedToLoadIntrinsics(const Instruction *Inst) {
  if (isa<CallInst>(Inst)) {
    auto *CI = cast<CallInst>(Inst);
    auto IID = GenXIntrinsic::getAnyIntrinsicID(CI->getCalledFunction());
    return IID == GenXIntrinsic::genx_svm_gather ||
           IID == GenXIntrinsic::genx_gather_scaled;
  }
  return std::all_of(Inst->user_begin(), Inst->user_end(), [](const User *U) {
    if (isa<InsertElementInst>(U) || isa<ShuffleVectorInst>(U) ||
        isa<BinaryOperator>(U) || isa<CallInst>(U))
      return isSinkedToLoadIntrinsics(cast<Instruction>(U));
    return false;
  });
}

// Arg is a ptr to a vector type. If data is only read using load, then false is
// returned. Otherwise, or if it is not clear, true is returned. This is a
// recursive function. The result may be false positive.
static bool isPtrArgModified(const Value &Arg) {
  // User iterator returns pointer both for star and arrow operators, because...
  return std::any_of(Arg.user_begin(), Arg.user_end(), [](const User *U) {
    if (isa<LoadInst>(U))
      return false;
    if (isa<AddrSpaceCastInst>(U) || isa<BitCastInst>(U) ||
        isa<GetElementPtrInst>(U))
      return isPtrArgModified(*U);
    if (isa<PtrToIntInst>(U))
      return !isSinkedToLoadIntrinsics(cast<Instruction>(U));
    return true;
  });
}

void vc::TransformedFuncInfo::fillCopyInOutInfo(
    Function &OrigFunc, SmallPtrSetImpl<Argument *> &ArgsToTransform) {
  IGC_ASSERT_MESSAGE(ArgKinds.empty(),
                     "shouldn't be filled before this method");
  llvm::transform(OrigFunc.args(), std::back_inserter(ArgKinds),
                  [&ArgsToTransform](Argument &Arg) {
                    if (!ArgsToTransform.count(&Arg))
                      return ArgKind::General;
                    if (isPtrArgModified(Arg))
                      return ArgKind::CopyInOut;
                    return ArgKind::CopyIn;
                  });
}

void vc::TransformedFuncInfo::inheritAttributes(Function &OrigFunc) {
  LLVMContext &Context = OrigFunc.getContext();
  const AttributeList &OrigAttrs = OrigFunc.getAttributes();

  // Inherit argument attributes
  for (auto ArgInfo : enumerate(ArgKinds)) {
    if (ArgInfo.value() == ArgKind::General) {
      AttributeSet ArgAttrs = OrigAttrs.getParamAttributes(ArgInfo.index());
      if (ArgAttrs.hasAttributes())
        Attrs = Attrs.addParamAttributes(Context, ArgInfo.index(),
                                         AttrBuilder{ArgAttrs});
    }
  }

  // Inherit function attributes.
  AttributeSet FnAttrs = OrigAttrs.getFnAttributes();
  if (FnAttrs.hasAttributes()) {
    AttrBuilder B(FnAttrs);
    Attrs = Attrs.addAttributes(Context, AttributeList::FunctionIndex, B);
  }
}

void vc::TransformedFuncInfo::discardStructRetAttr(LLVMContext &Context) {
  constexpr auto SretAttr = Attribute::StructRet;
  for (auto ArgInfo : enumerate(ArgKinds)) {
    unsigned ParamIndex = ArgInfo.index();
    if (Attrs.hasParamAttr(ParamIndex, SretAttr)) {
      Attrs = Attrs.removeParamAttribute(Context, ParamIndex, SretAttr);
      DiscardedParameterAttrs.push_back({ParamIndex, SretAttr});
    }
  }
}

void vc::TransformedFuncInfo::appendRetCopyOutInfo() {
  for (auto ArgInfo : enumerate(ArgKinds)) {
    if (ArgInfo.value() == ArgKind::CopyInOut) {
      NewFuncType.Ret.push_back(NewFuncType.Args[ArgInfo.index()]);
      RetToArg.Map.push_back(ArgInfo.index());
    }
  }
}

static Type *getRetType(LLVMContext &Context,
                        const TransformedFuncType &TFType) {
  if (TFType.Ret.empty())
    return Type::getVoidTy(Context);
  return StructType::get(Context, TFType.Ret);
}

Function *vc::createTransformedFuncDecl(Function &OrigFunc,
                                        const TransformedFuncInfo &TFuncInfo) {
  LLVMContext &Context = OrigFunc.getContext();
  // Construct the new function type using the new arguments.
  FunctionType *NewFuncTy = FunctionType::get(
      getRetType(Context, TFuncInfo.getType()), TFuncInfo.getType().Args,
      OrigFunc.getFunctionType()->isVarArg());

  // Create the new function body and insert it into the module.
  Function *NewFunc =
      Function::Create(NewFuncTy, OrigFunc.getLinkage(), OrigFunc.getName());

  LLVM_DEBUG(dbgs() << "\nCMABI: Transforming From:" << OrigFunc);
  vc::transferNameAndCCWithNewAttr(TFuncInfo.getAttributes(), OrigFunc,
                                   *NewFunc);
  OrigFunc.getParent()->getFunctionList().insert(OrigFunc.getIterator(),
                                                 NewFunc);
  vc::transferDISubprogram(OrigFunc, *NewFunc);
  LLVM_DEBUG(dbgs() << "  --> To: " << *NewFunc << "\n");

  return NewFunc;
}

static std::vector<Value *>
getTransformedFuncCallArgs(CallInst &OrigCall,
                           const TransformedFuncInfo &NewFuncInfo) {
  std::vector<Value *> NewCallOps;

  // Loop over the operands, inserting loads in the caller.
  for (auto &&[OrigArg, Kind] :
       zip(IGCLLVM::args(OrigCall), NewFuncInfo.getArgKinds())) {
    switch (Kind) {
    case ArgKind::General:
      NewCallOps.push_back(OrigArg.get());
      break;
    default: {
      IGC_ASSERT_MESSAGE(Kind == ArgKind::CopyIn || Kind == ArgKind::CopyInOut,
                         "unexpected arg kind");
      LoadInst *Load =
          new LoadInst(OrigArg.get()->getType()->getPointerElementType(),
                       OrigArg.get(), OrigArg.get()->getName() + ".val",
                       /* isVolatile */ false, &OrigCall);
      NewCallOps.push_back(Load);
      break;
    }
    }
  }

  IGC_ASSERT_MESSAGE(NewCallOps.size() == IGCLLVM::arg_size(OrigCall),
                     "varargs are unexpected");
  return std::move(NewCallOps);
}

static AttributeList
inheritCallAttributes(CallInst &OrigCall, int NumOrigFuncArgs,
                      const TransformedFuncInfo &NewFuncInfo) {
  IGC_ASSERT_MESSAGE(OrigCall.getNumArgOperands() == NumOrigFuncArgs,
                     "varargs aren't supported");
  AttributeList NewCallAttrs;

  const AttributeList &CallPAL = OrigCall.getAttributes();
  auto &Context = OrigCall.getContext();
  for (auto ArgInfo : enumerate(NewFuncInfo.getArgKinds())) {
    if (ArgInfo.value() == ArgKind::General) {
      AttributeSet attrs =
          OrigCall.getAttributes().getParamAttributes(ArgInfo.index());
      if (attrs.hasAttributes()) {
        AttrBuilder B(attrs);
        NewCallAttrs =
            NewCallAttrs.addParamAttributes(Context, ArgInfo.index(), B);
      }
    }
  }

  for (auto DiscardInfo : NewFuncInfo.getDiscardedParameterAttrs()) {
    NewCallAttrs = NewCallAttrs.removeParamAttribute(
        Context, DiscardInfo.ArgIndex, DiscardInfo.Attr);
  }

  // Add any function attributes.
  if (CallPAL.hasAttributes(AttributeList::FunctionIndex)) {
    AttrBuilder B(CallPAL.getFnAttributes());
    NewCallAttrs =
        NewCallAttrs.addAttributes(Context, AttributeList::FunctionIndex, B);
  }

  return std::move(NewCallAttrs);
}

static void handleRetValuePortion(int RetIdx, int ArgIdx, CallInst &OrigCall,
                                  CallInst &NewCall, IRBuilder<> &Builder,
                                  const TransformedFuncInfo &NewFuncInfo) {
  // Original return value.
  if (ArgIdx == RetToArgInfo::OrigRetNoArg) {
    IGC_ASSERT_MESSAGE(RetIdx == 0, "only zero element of returned value can "
                                    "be original function argument");
    OrigCall.replaceAllUsesWith(
        Builder.CreateExtractValue(&NewCall, RetIdx, "ret"));
    return;
  }
  Value *OutVal = Builder.CreateExtractValue(&NewCall, RetIdx);
  if (ArgIdx >= NewFuncInfo.getGlobalArgsInfo().FirstGlobalArgIdx) {
    auto Kind =
        NewFuncInfo.getGlobalArgsInfo().getGlobalInfoForArgNo(ArgIdx).Kind;
    IGC_ASSERT_MESSAGE(
        Kind == GlobalArgKind::ByValueInOut,
        "only passed by value localized global should be copied-out");
    Builder.CreateStore(
        OutVal, NewFuncInfo.getGlobalArgsInfo().getGlobalForArgNo(ArgIdx));
  } else {
    IGC_ASSERT_MESSAGE(NewFuncInfo.getArgKinds()[ArgIdx] == ArgKind::CopyInOut,
                       "only copy in-out args are expected");
    Builder.CreateStore(OutVal, OrigCall.getArgOperand(ArgIdx));
  }
}

static std::vector<Value *> handleGlobalArgs(Function &NewFunc,
                                             const GlobalArgsInfo &GlobalArgs) {
  // Collect all globals and their corresponding allocas.
  std::vector<Value *> LocalizedGloabls;
  Instruction *InsertPt = &*(NewFunc.begin()->getFirstInsertionPt());

  llvm::transform(drop_begin(NewFunc.args(), GlobalArgs.FirstGlobalArgIdx),
                  std::back_inserter(LocalizedGloabls),
                  [InsertPt](Argument &GVArg) -> Value * {
                    if (GVArg.getType()->isPointerTy())
                      return &GVArg;
                    AllocaInst *Alloca = new AllocaInst(
                        GVArg.getType(), vc::AddrSpace::Private, "", InsertPt);
                    new StoreInst(&GVArg, Alloca, InsertPt);
                    return Alloca;
                  });
  // Fancy naming and debug info.
  for (auto &&[GAI, GVArg, MaybeAlloca] :
       zip(GlobalArgs.Globals,
           drop_begin(NewFunc.args(), GlobalArgs.FirstGlobalArgIdx),
           LocalizedGloabls)) {
    GVArg.setName(GAI.GV->getName() + ".in");
    if (!GVArg.getType()->isPointerTy()) {
      IGC_ASSERT_MESSAGE(
          isa<AllocaInst>(MaybeAlloca),
          "an alloca is expected when pass localized global by value");
      MaybeAlloca->setName(GAI.GV->getName() + ".local");

      vc::DIBuilder::createDbgDeclareForLocalizedGlobal(
          *cast<AllocaInst>(MaybeAlloca), *GAI.GV, *InsertPt);
    }
  }

  SmallDenseMap<Value *, Value *> GlobalsToReplace;
  for (auto &&[GAI, LocalizedGlobal] :
       zip(GlobalArgs.Globals, LocalizedGloabls))
    GlobalsToReplace.insert(std::make_pair(GAI.GV, LocalizedGlobal));
  // Replaces all globals uses within this new function.
  replaceUsesWithinFunction(GlobalsToReplace, &NewFunc);
  return LocalizedGloabls;
}

static Value *
appendTransformedFuncRetPortion(Value &NewRetVal, int RetIdx, int ArgIdx,
                                ReturnInst &OrigRet, IRBuilder<> &Builder,
                                const TransformedFuncInfo &NewFuncInfo,
                                const std::vector<Value *> &OrigArgReplacements,
                                std::vector<Value *> &LocalizedGlobals) {
  if (ArgIdx == RetToArgInfo::OrigRetNoArg) {
    IGC_ASSERT_MESSAGE(RetIdx == 0,
                       "original return value must be at zero index");
    Value *OrigRetVal = OrigRet.getReturnValue();
    IGC_ASSERT_MESSAGE(OrigRetVal, "type unexpected");
    IGC_ASSERT_MESSAGE(OrigRetVal->getType()->isSingleValueType(),
                       "type unexpected");
    return Builder.CreateInsertValue(&NewRetVal, OrigRetVal, RetIdx);
  }
  if (ArgIdx >= NewFuncInfo.getGlobalArgsInfo().FirstGlobalArgIdx) {
    auto Kind =
        NewFuncInfo.getGlobalArgsInfo().getGlobalInfoForArgNo(ArgIdx).Kind;
    IGC_ASSERT_MESSAGE(
        Kind == GlobalArgKind::ByValueInOut,
        "only passed by value localized global should be copied-out");
    Value *LocalizedGlobal =
        LocalizedGlobals[ArgIdx -
                         NewFuncInfo.getGlobalArgsInfo().FirstGlobalArgIdx];
    IGC_ASSERT_MESSAGE(
        isa<AllocaInst>(LocalizedGlobal),
        "an alloca is expected when pass localized global by value");
    Value *LocalizedGlobalVal = Builder.CreateLoad(
        LocalizedGlobal->getType()->getPointerElementType(), LocalizedGlobal);
    return Builder.CreateInsertValue(&NewRetVal, LocalizedGlobalVal, RetIdx);
  }
  IGC_ASSERT_MESSAGE(NewFuncInfo.getArgKinds()[ArgIdx] == ArgKind::CopyInOut,
                     "Only copy in-out values are expected");
  Value *CurRetByPtr = OrigArgReplacements[ArgIdx];
  IGC_ASSERT_MESSAGE(isa<PointerType>(CurRetByPtr->getType()),
                     "a pointer is expected");
  if (isa<AddrSpaceCastInst>(CurRetByPtr))
    CurRetByPtr = cast<AddrSpaceCastInst>(CurRetByPtr)->getOperand(0);
  IGC_ASSERT_MESSAGE(isa<AllocaInst>(CurRetByPtr),
                     "corresponding alloca is expected");
  Value *CurRetByVal = Builder.CreateLoad(
      CurRetByPtr->getType()->getPointerElementType(), CurRetByPtr);
  return Builder.CreateInsertValue(&NewRetVal, CurRetByVal, RetIdx);
}

// Add some additional code before \p OrigCall to pass localized global value
// \p GAI to the transformed function.
// An argument corresponding to \p GAI is returned.
static Value *passGlobalAsCallArg(GlobalArgInfo GAI, CallInst &OrigCall) {
  // We should should load the global first to pass it by value.
  if (GAI.Kind == GlobalArgKind::ByValueIn ||
      GAI.Kind == GlobalArgKind::ByValueInOut)
    return new LoadInst(GAI.GV->getType()->getPointerElementType(), GAI.GV,
                        GAI.GV->getName() + ".val",
                        /* isVolatile */ false, &OrigCall);
  IGC_ASSERT_MESSAGE(
      GAI.Kind == GlobalArgKind::ByPointer,
      "localized global can be passed only by value or by pointer");
  auto *GVTy = cast<PointerType>(GAI.GV->getType());
  // No additional work when addrspaces match
  if (GVTy->getAddressSpace() == vc::AddrSpace::Private)
    return GAI.GV;
  // Need to add a temprorary cast inst to match types.
  // When this switch to the caller, it'll remove this cast.
  return new AddrSpaceCastInst{
      GAI.GV, vc::changeAddrSpace(GVTy, vc::AddrSpace::Private),
      GAI.GV->getName() + ".tmp", &OrigCall};
}

void vc::FuncUsersUpdater::run() {
  std::vector<CallInst *> DirectUsers;

  for (auto *U : OrigFunc.users()) {
    IGC_ASSERT_MESSAGE(
        isa<CallInst>(U),
        "the transformation is not applied to indirectly called functions");
    DirectUsers.push_back(cast<CallInst>(U));
  }

  std::vector<CallInst *> NewDirectUsers;
  // Loop over all of the callers of the function, transforming the call sites
  // to pass in the loaded pointers.
  for (auto *OrigCall : DirectUsers) {
    IGC_ASSERT(OrigCall->getCalledFunction() == &OrigFunc);
    auto *NewCall = updateFuncDirectUser(*OrigCall);
    NewDirectUsers.push_back(NewCall);
  }

  for (auto *OrigCall : DirectUsers)
    OrigCall->eraseFromParent();
}

CallInst *vc::FuncUsersUpdater::updateFuncDirectUser(CallInst &OrigCall) {
  std::vector<Value *> NewCallOps =
      getTransformedFuncCallArgs(OrigCall, NewFuncInfo);

  AttributeList NewCallAttrs = inheritCallAttributes(
      OrigCall, OrigFunc.getFunctionType()->getNumParams(), NewFuncInfo);

  // Push any localized globals.
  IGC_ASSERT_MESSAGE(NewCallOps.size() ==
                         NewFuncInfo.getGlobalArgsInfo().FirstGlobalArgIdx,
                     "call operands and called function info are inconsistent");
  llvm::transform(NewFuncInfo.getGlobalArgsInfo().Globals,
                  std::back_inserter(NewCallOps),
                  [&OrigCall](GlobalArgInfo GAI) {
                    return passGlobalAsCallArg(GAI, OrigCall);
                  });

  IGC_ASSERT_EXIT_MESSAGE(!isa<InvokeInst>(OrigCall),
                          "InvokeInst not supported");

  CallInst *NewCall = CallInst::Create(&NewFunc, NewCallOps, "", &OrigCall);
  IGC_ASSERT(nullptr != NewCall);
  NewCall->setCallingConv(OrigCall.getCallingConv());
  NewCall->setAttributes(NewCallAttrs);
  if (cast<CallInst>(OrigCall).isTailCall())
    NewCall->setTailCall();
  NewCall->setDebugLoc(OrigCall.getDebugLoc());
  NewCall->takeName(&OrigCall);

  // Update the callgraph to know that the callsite has been transformed.
  auto CalleeNode = static_cast<IGCLLVM::CallGraphNode *>(
      CG[OrigCall.getParent()->getParent()]);
  CalleeNode->replaceCallEdge(
#if LLVM_VERSION_MAJOR <= 10
      CallSite(&OrigCall), NewCall,
#else
      OrigCall, *NewCall,
#endif
      &NewFuncCGN);

  IRBuilder<> Builder(&OrigCall);
  for (auto RetToArg : enumerate(NewFuncInfo.getRetToArgInfo().Map))
    handleRetValuePortion(RetToArg.index(), RetToArg.value(), OrigCall,
                          *NewCall, Builder, NewFuncInfo);
  return NewCall;
}

void vc::FuncBodyTransfer::run() {
  // Since we have now created the new function, splice the body of the old
  // function right into the new function.
  NewFunc.getBasicBlockList().splice(NewFunc.begin(),
                                     OrigFunc.getBasicBlockList());

  std::vector<Value *> OrigArgReplacements = handleTransformedFuncArgs();
  std::vector<Value *> LocalizedGlobals =
      handleGlobalArgs(NewFunc, NewFuncInfo.getGlobalArgsInfo());

  handleTransformedFuncRets(OrigArgReplacements, LocalizedGlobals);
}

std::vector<Value *> vc::FuncBodyTransfer::handleTransformedFuncArgs() {
  std::vector<Value *> OrigArgReplacements;
  Instruction *InsertPt = &*(NewFunc.begin()->getFirstInsertionPt());

  std::transform(
      NewFuncInfo.getArgKinds().begin(), NewFuncInfo.getArgKinds().end(),
      NewFunc.arg_begin(), std::back_inserter(OrigArgReplacements),
      [InsertPt](ArgKind Kind, Argument &NewArg) -> Value * {
        switch (Kind) {
        case ArgKind::CopyIn:
        case ArgKind::CopyInOut: {
          auto *Alloca = new AllocaInst(NewArg.getType(),
                                        vc::AddrSpace::Private, "", InsertPt);
          new StoreInst{&NewArg, Alloca, InsertPt};
          return Alloca;
        }
        default:
          IGC_ASSERT_MESSAGE(Kind == ArgKind::General,
                             "unexpected argument kind");
          return &NewArg;
        }
      });

  std::transform(
      OrigArgReplacements.begin(), OrigArgReplacements.end(),
      OrigFunc.arg_begin(), OrigArgReplacements.begin(),
      [InsertPt](Value *Replacement, Argument &OrigArg) -> Value * {
        if (Replacement->getType() == OrigArg.getType())
          return Replacement;
        IGC_ASSERT_MESSAGE(isa<PointerType>(Replacement->getType()),
                           "only pointers can posibly mismatch");
        IGC_ASSERT_MESSAGE(isa<PointerType>(OrigArg.getType()),
                           "only pointers can posibly mismatch");
        IGC_ASSERT_MESSAGE(
            Replacement->getType()->getPointerAddressSpace() !=
                OrigArg.getType()->getPointerAddressSpace(),
            "pointers should have different addr spaces when they mismatch");
        IGC_ASSERT_MESSAGE(
            Replacement->getType()->getPointerElementType() ==
                OrigArg.getType()->getPointerElementType(),
            "pointers must have same element type when they mismatch");
        return new AddrSpaceCastInst(Replacement, OrigArg.getType(), "",
                                     InsertPt);
      });
  for (auto &&[OrigArg, OrigArgReplacement] :
       zip(OrigFunc.args(), OrigArgReplacements)) {
    OrigArgReplacement->takeName(&OrigArg);
    OrigArg.replaceAllUsesWith(OrigArgReplacement);
  }

  return std::move(OrigArgReplacements);
}

void vc::FuncBodyTransfer::handleTransformedFuncRet(
    ReturnInst &OrigRet, const std::vector<Value *> &OrigArgReplacements,
    std::vector<Value *> &LocalizedGlobals) {
  Type *NewRetTy = NewFunc.getReturnType();
  IRBuilder<> Builder(&OrigRet);
  auto &&RetToArg = enumerate(NewFuncInfo.getRetToArgInfo().Map);
  Value *NewRetVal = std::accumulate(
      RetToArg.begin(), RetToArg.end(), cast<Value>(UndefValue::get(NewRetTy)),
      [&OrigRet, &Builder, &OrigArgReplacements, &LocalizedGlobals,
       this](Value *NewRet, auto NewRetPortionInfo) {
        return appendTransformedFuncRetPortion(
            *NewRet, NewRetPortionInfo.index(), NewRetPortionInfo.value(),
            OrigRet, Builder, NewFuncInfo, OrigArgReplacements,
            LocalizedGlobals);
      });
  Builder.CreateRet(NewRetVal);
  OrigRet.eraseFromParent();
}

void vc::FuncBodyTransfer::handleTransformedFuncRets(
    const std::vector<Value *> &OrigArgReplacements,
    std::vector<Value *> &LocalizedGlobals) {
  Type *NewRetTy = NewFunc.getReturnType();
  if (NewRetTy->isVoidTy())
    return;
  std::vector<ReturnInst *> OrigRets;
  llvm::transform(make_filter_range(
                      instructions(NewFunc),
                      [](Instruction &Inst) { return isa<ReturnInst>(Inst); }),
                  std::back_inserter(OrigRets),
                  [](Instruction &RI) { return &cast<ReturnInst>(RI); });

  for (ReturnInst *OrigRet : OrigRets)
    handleTransformedFuncRet(*OrigRet, OrigArgReplacements, LocalizedGlobals);
}
