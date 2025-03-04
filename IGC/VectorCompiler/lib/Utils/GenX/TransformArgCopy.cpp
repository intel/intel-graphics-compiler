/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define DEBUG_TYPE "vc-transform-arg-copy"

#include "llvmWrapper/Analysis/CallGraph.h"
#include "llvmWrapper/IR/CallSite.h"
#include "llvmWrapper/IR/Function.h"
#include "llvmWrapper/IR/Instructions.h"

#include "Probe/Assertion.h"

#include "vc/Utils/GenX/TransformArgCopy.h"
#include "vc/Utils/GenX/TypeSize.h"
#include "vc/Utils/General/DebugInfo.h"
#include "vc/Utils/General/FunctionAttrs.h"
#include "vc/Utils/General/Types.h"

#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/Twine.h>
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

// Check if the value is only used by simple load or store.
static bool onlyUsedBySimpleValueLoadStore(const Value &Arg) {
  auto UserChecker = [&Arg](const auto &U) {
    auto *I = dyn_cast<Instruction>(U);
    if (!I)
      return false;

    if (auto *LI = dyn_cast<LoadInst>(U))
      return &Arg == LI->getPointerOperand();
    if (auto *SI = dyn_cast<StoreInst>(U))
      return &Arg == SI->getPointerOperand();
    if (auto *GEP = dyn_cast<GetElementPtrInst>(U)) {
      if (&Arg != GEP->getPointerOperand())
        return false;
      if (!GEP->hasAllZeroIndices())
        return false;
      return onlyUsedBySimpleValueLoadStore(*U);
    }

    if (isa<AddrSpaceCastInst>(U) || isa<PtrToIntInst>(U))
      return onlyUsedBySimpleValueLoadStore(*U);

    return false;
  };

  return llvm::all_of(Arg.users(), UserChecker);
}

// Check if the struct contains only suitable types.
// Currently suitable types are ints/vector of ints, floats/vector of floats,
// pointers/vector of pointers.
static bool containsOnlySuitableTypes(const StructType &StrTy) {
  return llvm::all_of(StrTy.elements(), [](Type *Ty) {
    return Ty->isIntOrIntVectorTy() || Ty->isFPOrFPVectorTy() ||
           Ty->isPtrOrPtrVectorTy();
  });
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

// Arg is a ptr to a vector/struct type. If data is only read using load, then
// false is returned. Otherwise, or if it is not clear, true is returned. This
// is a recursive function. The result may be false positive.
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

// Check if it is safe to pass structure by value.
static bool structSafeToPassByVal(const Argument &Arg, StructType *StrTy) {
  if (!containsOnlySuitableTypes(*StrTy))
    return false;

  // SRet/Byval are safe no matter what happens inside the
  // function according to their langref definitions.
  if (Arg.hasAttribute(Attribute::StructRet) ||
      Arg.hasAttribute(Attribute::ByVal))
    return true;

  auto UserChecker = [&Arg](const auto &U) {
    auto *I = dyn_cast<Instruction>(U);
    if (!I)
      return false;

    if (auto *SI = dyn_cast<StoreInst>(U))
      return &Arg == SI->getPointerOperand();
    if (auto *LI = dyn_cast<LoadInst>(U))
      return &Arg == LI->getPointerOperand();
    if (auto *GEP = dyn_cast<GetElementPtrInst>(U)) {
      IGC_ASSERT(&Arg == GEP->getPointerOperand());
      // Check the first idx is zero.
      Value *Idx = *GEP->idx_begin();
      auto *ConstIdx = dyn_cast<ConstantInt>(Idx);
      return ConstIdx && ConstIdx->getZExtValue() == 0;
    }

    return false;
  };

  // Allow only CopyIn for non-byval/sret structures
  return llvm::all_of(Arg.users(), UserChecker) && !isPtrArgModified(Arg);
}

static Type *getPtrArgElementType(const Argument &PtrArg) {
  auto *PtrArgTy = cast<PointerType>(PtrArg.getType());
  if (!PtrArgTy->isOpaque())
    return IGCLLVM::getNonOpaquePtrEltTy(PtrArgTy);
  if (auto *ByValTy = PtrArg.getParamByValType())
    return ByValTy;
  if (auto *StructRetTy = PtrArg.getParamStructRetType())
    return StructRetTy;
  SmallPtrSet<Type *, 2> ElemTys;
  for (auto *U : PtrArg.users()) {
    if (ElemTys.size() > 1)
      return nullptr;
    auto *I = dyn_cast<Instruction>(U);
    if (!I)
      continue;
    if (auto *LI = dyn_cast<LoadInst>(I)) {
      if (&PtrArg == LI->getPointerOperand())
        ElemTys.insert(LI->getType());
    } else if (auto *SI = dyn_cast<StoreInst>(I)) {
      if (&PtrArg == SI->getPointerOperand())
        ElemTys.insert(SI->getValueOperand()->getType());
    } else if (auto *GEPI = dyn_cast<GetElementPtrInst>(I)) {
      if (&PtrArg == GEPI->getPointerOperand())
        ElemTys.insert(GEPI->getSourceElementType());
    } else if (auto *PTII = dyn_cast<PtrToIntInst>(I)) {
      const Value *Addr = PTII;
      for (auto *AddrUser : Addr->users()) {
        switch (GenXIntrinsic::getAnyIntrinsicID(AddrUser)) {
        case GenXIntrinsic::genx_gather_scaled:
          if (Addr != AddrUser->getOperand(4))
            continue;
          ElemTys.insert(AddrUser->getType());
          break;
        case GenXIntrinsic::genx_scatter_scaled:
          if (Addr != AddrUser->getOperand(4))
            continue;
          ElemTys.insert(AddrUser->getOperand(6)->getType());
          break;
        case GenXIntrinsic::genx_svm_block_ld:
        case GenXIntrinsic::genx_svm_block_ld_unaligned:
          if (Addr != AddrUser->getOperand(0))
            continue;
          ElemTys.insert(AddrUser->getType());
          break;
        case GenXIntrinsic::genx_svm_block_st:
          if (Addr != AddrUser->getOperand(0))
            continue;
          ElemTys.insert(AddrUser->getOperand(1)->getType());
          break;
        default:
          break;
        }
      }
      if (!PTII->hasOneUse())
        continue;
      auto *IEI = dyn_cast<InsertElementInst>(PTII->user_back());
      if (!IEI)
        continue;
      const Value *AddrVec = IEI;
      if (IEI->hasOneUse())
        if (auto *SVI = dyn_cast<ShuffleVectorInst>(IEI->user_back()))
          if (SVI->hasOneUse())
            if (auto *BO = dyn_cast<BinaryOperator>(SVI->user_back()))
              AddrVec = BO;
      for (auto *AddrVecUser : AddrVec->users()) {
        switch (GenXIntrinsic::getAnyIntrinsicID(AddrVecUser)) {
        case GenXIntrinsic::genx_svm_gather:
          if (AddrVec != AddrVecUser->getOperand(2))
            continue;
          ElemTys.insert(AddrVecUser->getType());
          break;
        case GenXIntrinsic::genx_svm_scatter:
          if (AddrVec != AddrVecUser->getOperand(2))
            continue;
          ElemTys.insert(AddrVecUser->getOperand(3)->getType());
          break;
        default:
          break;
        }
      }
    }
  }
  return ElemTys.empty() ? nullptr : *ElemTys.begin();
}

// Check if argument should be transformed.
static Type *argToTransform(const Argument &Arg,
                            vc::TypeSizeWrapper MaxStructSize) {
  if (!isa<PointerType>(Arg.getType()))
    return nullptr;
  auto *ElemTy = getPtrArgElementType(Arg);
  if (!ElemTy)
    return nullptr;
  if (ElemTy->isIntOrIntVectorTy() || ElemTy->isFPOrFPVectorTy()) {
    if (ElemTy->isVectorTy()) {
      for (auto *U : Arg.users()) {
        auto *GEP = dyn_cast<GetElementPtrInst>(U);
        if (!GEP)
          continue;
        if (&Arg != GEP->getPointerOperand())
          continue;
        auto *ConstIdx = dyn_cast<ConstantInt>(*GEP->idx_begin());
        if (!ConstIdx || ConstIdx->getZExtValue() != 0)
          return nullptr;
      }
    } else if (!onlyUsedBySimpleValueLoadStore(Arg))
      return nullptr;
    return ElemTy;
  }
  if (auto *StrTy = dyn_cast<StructType>(ElemTy)) {
    const DataLayout &DL = Arg.getParent()->getParent()->getDataLayout();
    if (structSafeToPassByVal(Arg, StrTy) &&
        vc::getTypeSize(StrTy, &DL) <= MaxStructSize)
      return ElemTy;
  }
  return nullptr;
}

// Collect arguments that should be transformed.
SmallDenseMap<Argument *, Type *>
vc::collectArgsToTransform(Function &F, vc::TypeSizeWrapper MaxStructSize) {
  SmallDenseMap<Argument *, Type *> ArgsToTransform;
  for (auto &Arg : F.args())
    if (auto *ArgElemTy = argToTransform(Arg, MaxStructSize))
      ArgsToTransform.insert(std::make_pair(&Arg, ArgElemTy));
  return ArgsToTransform;
}

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

bool vc::RetToArgLink::isRealIdx(int Idx) {
  bool Res = (Idx != OmittedIdx);
  if (Res)
    IGC_ASSERT_MESSAGE(Idx >= 0, "Not omitted idx is corrupted!");
  return Res;
}

bool vc::RetToArgLink::isOmittedIdx(int Idx) { return !isRealIdx(Idx); }

RetToArgLink vc::RetToArgLink::createForOrigRet() {
  return {OmittedIdx, OmittedIdx};
}

RetToArgLink vc::RetToArgLink::createForGlobalArg(int NewIdx) {
  IGC_ASSERT_MESSAGE(isRealIdx(NewIdx), "Tried to build corrupted link!");
  return {NewIdx, OmittedIdx};
}

RetToArgLink vc::RetToArgLink::createForOmittedArg(int OrigIdx) {
  IGC_ASSERT_MESSAGE(isRealIdx(OrigIdx), "Tried to build corrupted link!");
  return {OmittedIdx, OrigIdx};
}

RetToArgLink vc::RetToArgLink::createForLinkedArgs(int NewIdx, int OrigIdx) {
  IGC_ASSERT_MESSAGE(isRealIdx(NewIdx) && isRealIdx(OrigIdx),
                     "Tried to build corrupted link!");
  return {NewIdx, OrigIdx};
}

bool vc::RetToArgLink::isOrigRet() const {
  return isOmittedIdx(NewIdx) && isOmittedIdx(OrigIdx);
}

bool vc::RetToArgLink::isGlobalArg() const {
  return isRealIdx(NewIdx) && isOmittedIdx(OrigIdx);
}

bool vc::RetToArgLink::isOmittedArg() const {
  return isOmittedIdx(NewIdx) && isRealIdx(OrigIdx);
}

int vc::RetToArgLink::getNewIdx() const {
  IGC_ASSERT_MESSAGE(isRealIdx(NewIdx), "Tried to use bad new index!");
  return NewIdx;
}

int vc::RetToArgLink::getOrigIdx() const {
  IGC_ASSERT_MESSAGE(isRealIdx(OrigIdx), "Tried to use bad orig index!");
  return OrigIdx;
}

vc::OrigArgInfo::OrigArgInfo(Type *TyIn, ArgKind KindIn, int NewIdxIn)
    : TransformedOrigType{TyIn}, Kind{KindIn}, NewIdx{NewIdxIn} {
  IGC_ASSERT_MESSAGE(TyIn, "Bad type provided");
  IGC_ASSERT_MESSAGE(NewIdxIn == OmittedIdx || NewIdxIn >= 0,
                     "Unexpected new index");
}

int vc::OrigArgInfo::getNewIdx() const {
  IGC_ASSERT_MESSAGE(NewIdx >= 0, "Tried to use bad new idx!");
  return NewIdx;
}

vc::TransformedFuncInfo::TransformedFuncInfo(
    Function &OrigFunc, SmallDenseMap<Argument *, Type *> &ArgsToTransform) {
  fillOrigArgInfo(OrigFunc, ArgsToTransform);
  inheritAttributes(OrigFunc);

  // struct-returns are not supported for transformed functions,
  // so we need to discard the attribute
  if (OrigFunc.hasStructRetAttr() && OrigFunc.hasLocalLinkage())
    discardStructRetAttr(OrigFunc.getContext());

  auto *OrigRetTy = OrigFunc.getFunctionType()->getReturnType();
  if (!OrigRetTy->isVoidTy()) {
    NewFuncType.Ret.push_back(OrigRetTy);
    RetToArg.push_back(RetToArgLink::createForOrigRet());
  }
  appendRetCopyOutInfo();
}

// Whether provided \p GV should be passed by pointer.
static bool passLocalizedGlobalByPointer(const GlobalValue &GV) {
  auto *Type = GV.getValueType();
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
      Type *PointeeTy = GV->getValueType();
      NewFuncType.Args.push_back(PointeeTy);
      if (GV->isConstant())
        GlobalArgs.Globals.push_back({GV, GlobalArgKind::ByValueIn});
      else {
        GlobalArgs.Globals.push_back({GV, GlobalArgKind::ByValueInOut});
        NewFuncType.Ret.push_back(PointeeTy);
        RetToArg.push_back(RetToArgLink::createForGlobalArg(ArgIdx));
      }
    }
  }
}

void vc::TransformedFuncInfo::fillOrigArgInfo(
    Function &OrigFunc, SmallDenseMap<Argument *, Type *> &ArgsToTransform) {
  IGC_ASSERT_MESSAGE(OrigArgs.empty(),
                     "shouldn't be filled before this method");

  auto DetermineArgKind = [&ArgsToTransform](const Argument &Arg) {
    if (!ArgsToTransform.count(&Arg))
      return ArgKind::General;
    if (Arg.hasAttribute(Attribute::StructRet))
      return ArgKind::CopyOut;
    if (Arg.hasAttribute(Attribute::ByVal))
      return ArgKind::CopyIn;
    if (isPtrArgModified(Arg))
      return ArgKind::CopyInOut;
    return ArgKind::CopyIn;
  };

  for (const auto &Arg : OrigFunc.args()) {
    Type *Ty = Arg.getType();
    int NewIdx = NewFuncType.Args.size();
    auto Kind = DetermineArgKind(Arg);

    // Update type for transformed arguments.
    if (Kind != ArgKind::General) {
      auto It = ArgsToTransform.find(&Arg);
      IGC_ASSERT_EXIT(It != ArgsToTransform.end());
      Ty = It->second;
    }

    if (Kind == ArgKind::CopyOut) {
      // Save omitted arg info.
      OrigArgs.push_back({Ty, Kind});
    } else {
      // Save arg info and update argument types.
      NewFuncType.Args.push_back(Ty);
      OrigArgs.push_back({Ty, Kind, NewIdx});
    }
  }
}

AttributeList
vc::TransformedFuncInfo::gatherAttributes(LLVMContext &Context,
                                          const AttributeList &AL) const {
  AttributeList GatheredAttrs;

  // Gather argument attributes.
  for (auto &OrigArgData : enumerate(OrigArgs)) {
    int OrigIdx = OrigArgData.index();
    const OrigArgInfo &OrigArgInfoEntry = OrigArgData.value();
    if (OrigArgInfoEntry.getKind() == ArgKind::General) {
      IGC_ASSERT_MESSAGE(!OrigArgInfoEntry.isOmittedArg(),
                         "unexpected omitted argument");
      AttrBuilder ArgAttrB(Context, AL.getParamAttrs(OrigIdx));
      GatheredAttrs = GatheredAttrs.addParamAttributes(
          Context, OrigArgInfoEntry.getNewIdx(), ArgAttrB);
    }
  }

  // Gather function attributes.
  AttrBuilder B(Context, AL.getFnAttrs());
  GatheredAttrs = GatheredAttrs.addFnAttributes(Context, B);

  return GatheredAttrs;
}

void vc::TransformedFuncInfo::inheritAttributes(Function &OrigFunc) {
  LLVMContext &Context = OrigFunc.getContext();
  const AttributeList &OrigAttrs = OrigFunc.getAttributes();
  Attrs = gatherAttributes(Context, OrigAttrs);
}

void vc::TransformedFuncInfo::discardStructRetAttr(LLVMContext &Context) {
  constexpr auto SretAttr = Attribute::StructRet;
  for (auto &ArgInfo : enumerate(NewFuncType.Args)) {
    unsigned ParamIndex = ArgInfo.index();
    if (Attrs.hasParamAttr(ParamIndex, SretAttr)) {
      Attrs = Attrs.removeParamAttribute(Context, ParamIndex, SretAttr);
      DiscardedParameterAttrs.push_back({ParamIndex, SretAttr});
    }
  }
}

void vc::TransformedFuncInfo::appendRetCopyOutInfo() {
  for (auto &OrigArgData : enumerate(OrigArgs)) {
    int OrigIdx = OrigArgData.index();
    const OrigArgInfo &OrigArgInfoEntry = OrigArgData.value();
    switch (OrigArgInfoEntry.getKind()) {
    case ArgKind::CopyInOut:
      NewFuncType.Ret.push_back(OrigArgInfoEntry.getTransformedOrigType());
      RetToArg.push_back(RetToArgLink::createForLinkedArgs(
          OrigArgInfoEntry.getNewIdx(), OrigIdx));
      break;
    case ArgKind::CopyOut:
      NewFuncType.Ret.push_back(OrigArgInfoEntry.getTransformedOrigType());
      RetToArg.push_back(RetToArgLink::createForOmittedArg(OrigIdx));
      break;
    default:
      break;
    }
  }
}

static Type *getRetType(LLVMContext &Context,
                        const TransformedFuncType &TFType) {
  if (TFType.Ret.empty())
    return Type::getVoidTy(Context);
  if (TFType.Ret.size() == 1)
    return TFType.Ret.front();
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

  LLVM_DEBUG(dbgs() << "\nVC-TRANSFORM-ARG-COPY: Transforming From:"
                    << OrigFunc);
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
  [[maybe_unused]] unsigned OmittedCount = 0;
  for (auto &&[OrigArg, OrigArgData] :
       zip(OrigCall.args(), NewFuncInfo.getOrigArgInfo())) {
    auto Kind = OrigArgData.getKind();
    switch (Kind) {
    case ArgKind::General:
      NewCallOps.push_back(OrigArg.get());
      break;
    case ArgKind::CopyOut:
      // The argument is omitted
      ++OmittedCount;
      break;
    default: {
      IGC_ASSERT_MESSAGE(Kind == ArgKind::CopyIn || Kind == ArgKind::CopyInOut,
                         "unexpected arg kind");

      IRBuilder<> Builder(&OrigCall);

      auto *Arg = OrigArg.get();
      Value *Load = nullptr;
      if (auto *G = dyn_cast<GlobalVariable>(Arg);
          G && G->hasAttribute("genx_volatile")) {
        auto *Fn = GenXIntrinsic::getGenXDeclaration(
            G->getParent(), GenXIntrinsic::genx_vload,
            {OrigArgData.getTransformedOrigType(), G->getType()});
        Load = Builder.CreateCall(Fn, G, Arg->getName() + ".val");
      } else {
        Load = Builder.CreateLoad(OrigArgData.getTransformedOrigType(), Arg,
                                  Arg->getName() + ".val");
      }
      NewCallOps.push_back(Load);
      break;
    }
    }
  }

  IGC_ASSERT_MESSAGE(NewCallOps.size() ==
                         OrigCall.arg_size() - OmittedCount,
                     "varargs are unexpected");
  return NewCallOps;
}

static AttributeList
inheritCallAttributes(CallInst &OrigCall, int NumOrigFuncArgs,
                      const TransformedFuncInfo &NewFuncInfo) {
  IGC_ASSERT_MESSAGE(IGCLLVM::getNumArgOperands(&OrigCall) == NumOrigFuncArgs,
                     "varargs aren't supported");

  const AttributeList &CallPAL = OrigCall.getAttributes();
  auto &Context = OrigCall.getContext();
  AttributeList NewCallAttrs = NewFuncInfo.gatherAttributes(Context, CallPAL);

  for (auto &DiscardInfo : NewFuncInfo.getDiscardedParameterAttrs()) {
    NewCallAttrs = NewCallAttrs.removeParamAttribute(
        Context, DiscardInfo.ArgIndex, DiscardInfo.Attr);
  }

  return NewCallAttrs;
}

static Value *extractValueFromRet(Value &RetVal, int RetIdx,
                                  IRBuilder<> &Builder,
                                  const TransformedFuncInfo &NewFuncInfo,
                                  const Twine &Name = "") {
  if (NewFuncInfo.getRetToArgInfo().size() == 1) {
    // Structure of one element, omit struct.
    return &RetVal;
  }
  IGC_ASSERT_MESSAGE(NewFuncInfo.getRetToArgInfo().size() > 1,
                     "Unexpected types number");
  return Builder.CreateExtractValue(&RetVal, RetIdx, Name);
}

static void handleRetValuePortion(int RetIdx, RetToArgLink ArgInfo,
                                  CallInst &OrigCall, CallInst &NewCall,
                                  IRBuilder<> &Builder,
                                  const TransformedFuncInfo &NewFuncInfo) {
  // Original return value.
  if (ArgInfo.isOrigRet()) {
    IGC_ASSERT_MESSAGE(RetIdx == 0, "only zero element of returned value can "
                                    "be original function argument");
    auto *ExtractedVal =
        extractValueFromRet(NewCall, RetIdx, Builder, NewFuncInfo, "ret");
    OrigCall.replaceAllUsesWith(ExtractedVal);
    return;
  }
  Value *OutVal = extractValueFromRet(NewCall, RetIdx, Builder, NewFuncInfo);
  if (ArgInfo.isGlobalArg()) {
    // Globals are at new indices.
    int NewIdx = ArgInfo.getNewIdx();
    IGC_ASSERT_MESSAGE(NewIdx >=
                           NewFuncInfo.getGlobalArgsInfo().FirstGlobalArgIdx,
                       "Corrupted global arg position!");
    auto Kind =
        NewFuncInfo.getGlobalArgsInfo().getGlobalInfoForArgNo(NewIdx).Kind;
    IGC_ASSERT_MESSAGE(
        Kind == GlobalArgKind::ByValueInOut,
        "only passed by value localized global should be copied-out");
    auto *G = NewFuncInfo.getGlobalArgsInfo().getGlobalForArgNo(NewIdx);
    IGC_ASSERT_MESSAGE(!G->hasAttribute("genx_volatile"),
                       "genx_volatile is not expected");
    Builder.CreateStore(OutVal, G);
  } else {
    // Use orig index: working with orig call's argument
    int OrigArgIdx = ArgInfo.getOrigIdx();
    auto Kind = NewFuncInfo.getOrigArgInfo()[OrigArgIdx].getKind();
    IGC_ASSERT_MESSAGE(Kind == ArgKind::CopyInOut || Kind == ArgKind::CopyOut,
                       "only copy (in-)out args are expected");
    auto *Arg = OrigCall.getArgOperand(OrigArgIdx);
    if (auto *G = dyn_cast<GlobalVariable>(Arg);
        G && G->hasAttribute("genx_volatile")) {
      auto *Fn = GenXIntrinsic::getGenXDeclaration(
          G->getParent(), GenXIntrinsic::genx_vstore,
          {OutVal->getType(), G->getType()});
      Builder.CreateCall(Fn, {OutVal, G});
    } else {
      Builder.CreateStore(OutVal, Arg);
    }
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

static Value *insertValueToRet(Value &Val, Value &RetVal, int RetIdx,
                               IRBuilder<> &Builder,
                               const TransformedFuncInfo &NewFuncInfo) {
  if (NewFuncInfo.getRetToArgInfo().size() == 1) {
    // Structure of one element, omit struct.
    return &Val;
  }
  IGC_ASSERT_MESSAGE(NewFuncInfo.getRetToArgInfo().size() > 1,
                     "Unexpected types number");
  return Builder.CreateInsertValue(&RetVal, &Val, RetIdx);
}

static Value *appendTransformedFuncRetPortion(
    Value &NewRetVal, int RetIdx, RetToArgLink ArgInfo, ReturnInst &OrigRet,
    IRBuilder<> &Builder, const TransformedFuncInfo &NewFuncInfo,
    const std::vector<Value *> &OrigArgReplacements,
    std::vector<Value *> &LocalizedGlobals) {
  if (ArgInfo.isOrigRet()) {
    IGC_ASSERT_MESSAGE(RetIdx == 0,
                       "original return value must be at zero index");
    Value *OrigRetVal = OrigRet.getReturnValue();
    IGC_ASSERT_MESSAGE(OrigRetVal, "type unexpected");
    IGC_ASSERT_MESSAGE(OrigRetVal->getType()->isSingleValueType(),
                       "type unexpected");
    return insertValueToRet(*OrigRetVal, NewRetVal, RetIdx, Builder,
                            NewFuncInfo);
  }
  if (ArgInfo.isGlobalArg()) {
    // Globals are at new indices.
    int NewIdx = ArgInfo.getNewIdx();
    IGC_ASSERT_MESSAGE(NewIdx >=
                           NewFuncInfo.getGlobalArgsInfo().FirstGlobalArgIdx,
                       "Corrupted global arg position!");
    auto Kind =
        NewFuncInfo.getGlobalArgsInfo().getGlobalInfoForArgNo(NewIdx).Kind;
    IGC_ASSERT_MESSAGE(
        Kind == GlobalArgKind::ByValueInOut,
        "only passed by value localized global should be copied-out");
    Value *LocalizedGlobal =
        LocalizedGlobals[NewIdx -
                         NewFuncInfo.getGlobalArgsInfo().FirstGlobalArgIdx];
    IGC_ASSERT_MESSAGE(
        isa<AllocaInst>(LocalizedGlobal),
        "an alloca is expected when pass localized global by value");
    Value *LocalizedGlobalVal = Builder.CreateLoad(
        cast<AllocaInst>(LocalizedGlobal)->getAllocatedType(), LocalizedGlobal);
    return insertValueToRet(*LocalizedGlobalVal, NewRetVal, RetIdx, Builder,
                            NewFuncInfo);
  }
  // Use orig index: working with orig call's argument replacement.
  int OrigIdx = ArgInfo.getOrigIdx();
  auto Kind = NewFuncInfo.getOrigArgInfo()[OrigIdx].getKind();
  IGC_ASSERT_MESSAGE(Kind == ArgKind::CopyInOut || Kind == ArgKind::CopyOut,
                     "Only copy (in-)out values are expected");
  Value *CurRetByPtr = OrigArgReplacements[OrigIdx];
  IGC_ASSERT_MESSAGE(isa<PointerType>(CurRetByPtr->getType()),
                     "a pointer is expected");
  if (isa<AddrSpaceCastInst>(CurRetByPtr))
    CurRetByPtr = cast<AddrSpaceCastInst>(CurRetByPtr)->getOperand(0);
  IGC_ASSERT_MESSAGE(isa<AllocaInst>(CurRetByPtr),
                     "corresponding alloca is expected");
  Value *CurRetByVal = Builder.CreateLoad(
      cast<AllocaInst>(CurRetByPtr)->getAllocatedType(), CurRetByPtr);
  return insertValueToRet(*CurRetByVal, NewRetVal, RetIdx, Builder,
                          NewFuncInfo);
}

// Add some additional code before \p OrigCall to pass localized global value
// \p GAI to the transformed function.
// An argument corresponding to \p GAI is returned.
static Value *passGlobalAsCallArg(GlobalArgInfo GAI, CallInst &OrigCall) {
  // We should should load the global first to pass it by value.
  if (GAI.Kind == GlobalArgKind::ByValueIn ||
      GAI.Kind == GlobalArgKind::ByValueInOut)
    return new LoadInst(GAI.GV->getValueType(), GAI.GV, GAI.GV->getName() + ".val",
                        /* isVolatile */ false, &OrigCall);
  IGC_ASSERT_MESSAGE(
      GAI.Kind == GlobalArgKind::ByPointer,
      "localized global can be passed only by value or by pointer");
  auto *GVTy = cast<PointerType>(GAI.GV->getType());
  // No additional work when addrspaces match
  if (GVTy->getAddressSpace() == vc::AddrSpace::Private)
    return GAI.GV;
  // Need to add a temporary cast inst to match types.
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
  auto CalleeNode =
      static_cast<IGCLLVM::CallGraphNode *>(CG[OrigCall.getFunction()]);
  CalleeNode->replaceCallEdge(OrigCall, *NewCall, &NewFuncCGN);

  IRBuilder<> Builder(&OrigCall);
  for (auto &RetToArg : enumerate(NewFuncInfo.getRetToArgInfo()))
    handleRetValuePortion(RetToArg.index(), RetToArg.value(), OrigCall,
                          *NewCall, Builder, NewFuncInfo);
  return NewCall;
}

void vc::FuncUsersUpdaterNewPM::run() {
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

CallInst *vc::FuncUsersUpdaterNewPM::updateFuncDirectUser(CallInst &OrigCall) {
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

  IRBuilder<> Builder(&OrigCall);
  for (auto &RetToArg : enumerate(NewFuncInfo.getRetToArgInfo()))
    handleRetValuePortion(RetToArg.index(), RetToArg.value(), OrigCall,
                          *NewCall, Builder, NewFuncInfo);
  return NewCall;
}

void vc::FuncBodyTransfer::run() {
  // Since we have now created the new function, splice the body of the old
  // function right into the new function.
  IGCLLVM::splice(&NewFunc, NewFunc.begin(), &OrigFunc);

  std::vector<Value *> OrigArgReplacements = handleTransformedFuncArgs();
  std::vector<Value *> LocalizedGlobals =
      handleGlobalArgs(NewFunc, NewFuncInfo.getGlobalArgsInfo());

  handleTransformedFuncRets(OrigArgReplacements, LocalizedGlobals);
}

std::vector<Value *> vc::FuncBodyTransfer::handleTransformedFuncArgs() {
  std::vector<Value *> OrigArgReplacements;
  Instruction *InsertPt = &*(NewFunc.begin()->getFirstInsertionPt());

  std::transform(
      NewFuncInfo.getOrigArgInfo().begin(), NewFuncInfo.getOrigArgInfo().end(),
      std::back_inserter(OrigArgReplacements),
      [InsertPt, this](const auto &OrigArgData) -> Value * {
        switch (OrigArgData.getKind()) {
        case ArgKind::CopyIn:
        case ArgKind::CopyInOut: {
          auto *NewArg = IGCLLVM::getArg(NewFunc, OrigArgData.getNewIdx());
          auto *Alloca = new AllocaInst(NewArg->getType(),
                                        vc::AddrSpace::Private, "", InsertPt);
          new StoreInst{NewArg, Alloca, InsertPt};
          return Alloca;
        }
        case ArgKind::CopyOut: {
          IGC_ASSERT_MESSAGE(OrigArgData.isOmittedArg(),
                             "Unexpected existing arg");
          return new AllocaInst(OrigArgData.getTransformedOrigType(),
                                AddrSpace::Private, "", InsertPt);
        }
        default:
          IGC_ASSERT_MESSAGE(OrigArgData.getKind() == ArgKind::General,
                             "unexpected argument kind");
          return IGCLLVM::getArg(NewFunc, OrigArgData.getNewIdx());
        }
      });

  std::transform(
      OrigArgReplacements.begin(), OrigArgReplacements.end(),
      OrigFunc.arg_begin(), OrigArgReplacements.begin(),
      [InsertPt](Value *Replacement, Argument &OrigArg) -> Value * {
        if (Replacement->getType() == OrigArg.getType())
          return Replacement;
        IGC_ASSERT_MESSAGE(isa<PointerType>(Replacement->getType()),
                           "only pointers can possibly mismatch");
        IGC_ASSERT_MESSAGE(isa<PointerType>(OrigArg.getType()),
                           "only pointers can possibly mismatch");
        IGC_ASSERT_MESSAGE(
            Replacement->getType()->getPointerAddressSpace() !=
                OrigArg.getType()->getPointerAddressSpace(),
            "pointers should have different addr spaces when they mismatch");
        return new AddrSpaceCastInst(Replacement, OrigArg.getType(), "",
                                     InsertPt);
      });
  for (auto &&[OrigArg, OrigArgReplacement] :
       zip(OrigFunc.args(), OrigArgReplacements)) {
    OrigArgReplacement->takeName(&OrigArg);
    OrigArg.replaceAllUsesWith(OrigArgReplacement);
  }

  return OrigArgReplacements;
}

void vc::FuncBodyTransfer::handleTransformedFuncRet(
    ReturnInst &OrigRet, const std::vector<Value *> &OrigArgReplacements,
    std::vector<Value *> &LocalizedGlobals) {
  Type *NewRetTy = NewFunc.getReturnType();
  IRBuilder<> Builder(&OrigRet);
  auto &&RetToArg = enumerate(NewFuncInfo.getRetToArgInfo());
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
