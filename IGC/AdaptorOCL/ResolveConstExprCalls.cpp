/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ResolveConstExprCalls.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"

#include "llvmWrapper/IR/Type.h"
#include "llvmWrapper/IR/Function.h"

#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"

#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-resolve-constexpr-calls"
#define PASS_DESCRIPTION "Resolve pseudo indirect calls"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ResolveConstExprCalls, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(ResolveConstExprCalls, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ResolveConstExprCalls::ID = 0;

ResolveConstExprCalls::ResolveConstExprCalls() : ModulePass(ID) {
    initializeResolveConstExprCallsPass(*PassRegistry::getPassRegistry());
}

/// Return the specified type promoted as it would be to pass though a va_arg
/// area.
static Type* getPromotedType(Type* Ty) {
    if (IntegerType* ITy = dyn_cast<IntegerType>(Ty)) {
        if (ITy->getBitWidth() < 32)
            return Type::getInt32Ty(Ty->getContext());
    }
    return Ty;
}

/// This function is the simplified version of one from LLVM InstCombineCalls.cpp file
/// We do not consider the cases with different number of arguments and different return values
/// If the callee is a constexpr cast of a function, attempt to move the cast to
/// the arguments of the call.
bool transformConstExprCastCall(CallInst& Call) {
    auto* Callee = dyn_cast<Function>(Call.getCalledOperand()->stripPointerCasts());
    if (!Callee)
        return false;

    // If this is a call to a thunk function, don't remove the cast. Thunks are
    // used to transparently forward all incoming parameters and outgoing return
    // values, so it's important to leave the cast in place.
    if (Callee->hasFnAttribute("thunk"))
        return false;

    // If this is a musttail call, the callee's prototype must match the caller's
    // prototype with the exception of pointee types. The code below doesn't
    // implement that, so we can't do this transform.
    // TODO: Do the transform if it only requires adding pointer casts.
    if (Call.isMustTailCall())
        return false;

    Instruction* Caller = &Call;
    const AttributeList& CallerPAL = Call.getAttributes();

    // Okay, this is a cast from a function to a different type.  Unless doing so
    // would cause a type conversion of one of our arguments, change this call to
    // be a direct call with arguments casted to the appropriate types.
    FunctionType* FT = Callee->getFunctionType();
    Type* OldRetTy = Caller->getType();
    Type* NewRetTy = FT->getReturnType();
    auto& DL = Call.getFunction()->getParent()->getDataLayout();

    // Check to see if we are changing the return type...
    if (OldRetTy != NewRetTy)
        return false;   // We'll skip this case

    unsigned NumActualArgs = Call.arg_size();
    if (NumActualArgs != FT->getNumParams())
        return false;  // We'll skip this case

    // Prevent us turning:
    // declare void @takes_i32_inalloca(i32* inalloca)
    //  call void bitcast (void (i32*)* @takes_i32_inalloca to void (i32)*)(i32 0)
    //
    // into:
    //  call void @takes_i32_inalloca(i32* null)
    //
    //  Similarly, avoid folding away bitcasts of byval calls.
    if (Callee->getAttributes().hasAttrSomewhere(llvm::Attribute::InAlloca))
        return false;

    auto AI = Call.arg_begin();
    for (unsigned i = 0, e = NumActualArgs; i != e; ++i, ++AI) {
        Type* ParamTy = FT->getParamType(i);
        Type* ActTy = (*AI)->getType();

        if (!CastInst::isBitOrNoopPointerCastable(ActTy, ParamTy, DL))
            return false;   // Cannot transform this parameter value.

        AttrBuilder AB(FT->getContext(), CallerPAL.getParamAttrs(i));
        if (AB.overlaps(AttributeFuncs::typeIncompatible(ParamTy)))
            return false;   // Attribute not compatible with transformed value.

        if (Call.isInAllocaArgument(i))
            return false;   // Cannot transform to and from inalloca.

        if (CallerPAL.hasParamAttr(i, llvm::Attribute::ByVal) !=
            Callee->getAttributes().hasParamAttr(i, llvm::Attribute::ByVal))
            return false; // Cannot transform to or from byval.

        // If the parameter is passed as a byval argument, then we have to have a
        // sized type and the sized type has to have the same size as the old type.
        if (ParamTy != ActTy && CallerPAL.hasParamAttr(i, llvm::Attribute::ByVal)) {
            PointerType* ParamPTy = dyn_cast<PointerType>(ParamTy);
            if (!ParamPTy || !IGCLLVM::getArg(*Callee, i)->getParamByValType()->isSized())
                return false;

            Type* CurElTy = Call.getParamByValType(i);
            if (DL.getTypeAllocSize(CurElTy) !=
                DL.getTypeAllocSize(IGCLLVM::getArg(*Callee, i)->getParamByValType()))
                return false;
        }
    }

    if (Callee->isDeclaration()) {
        // If the callee is just a declaration, don't change the varargsness of the
        // call.  We don't want to introduce a varargs call where one doesn't
        // already exist.
        if (FT->isVarArg() != Call.getFunctionType()->isVarArg())
            return false;

        // If both the callee and the cast type are varargs, we still have to make
        // sure the number of fixed parameters are the same or we have the same
        // ABI issues as if we introduce a varargs call.
        if (FT->isVarArg() && Call.getFunctionType()->isVarArg() &&
            FT->getNumParams() != Call.getFunctionType()->getNumParams())
            return false;
    }

    // Okay, we decided that this is a safe thing to do: go ahead and start
    // inserting cast instructions as necessary.
    SmallVector<Value*, 8> Args;
    SmallVector<AttributeSet, 8> ArgAttrs;
    Args.reserve(NumActualArgs);
    ArgAttrs.reserve(NumActualArgs);

    // Get any return attributes.
    AttrBuilder RAttrs(FT->getContext(), CallerPAL.getRetAttrs());
    // If the return value is not being used, the type may not be compatible
    // with the existing attributes.  Wipe out any problematic attributes.
    RAttrs.remove(AttributeFuncs::typeIncompatible(NewRetTy));

    LLVMContext& Ctx = Call.getContext();
    AI = Call.arg_begin();
    IRBuilder<> Builder(&Call);
    for (unsigned i = 0; i != NumActualArgs; ++i, ++AI) {
        Type* ParamTy = FT->getParamType(i);

        Value* NewArg = *AI;
        if ((*AI)->getType() != ParamTy)
            NewArg = Builder.CreateBitOrPointerCast(*AI, ParamTy);
        Args.push_back(NewArg);

        // Add any parameter attributes.
        AttrBuilder AB(Ctx, CallerPAL.getParamAttrs(i));
        if (CallerPAL.hasParamAttr(i, llvm::Attribute::ByVal)) {
            AB.addByValAttr(Callee->getArg(i)->getParamByValType());
        }
        ArgAttrs.push_back(AttributeSet::get(Ctx, AB));
    }

    AttributeSet FnAttrs = CallerPAL.getFnAttrs();

    if (NewRetTy->isVoidTy())
        Caller->setName("");   // Void type should not have a name.

    IGC_ASSERT_MESSAGE((ArgAttrs.size() == FT->getNumParams() || FT->isVarArg()),
        "missing argument attributes");
    AttributeList NewCallerPAL = AttributeList::get(
        Ctx, FnAttrs, AttributeSet::get(Ctx, RAttrs), ArgAttrs);

    SmallVector<OperandBundleDef, 1> OpBundles;
    Call.getOperandBundlesAsDefs(OpBundles);

    CallBase* NewCall;
    NewCall = Builder.CreateCall(Callee, Args, OpBundles);
    cast<CallInst>(NewCall)->setTailCallKind(
        cast<CallInst>(Caller)->getTailCallKind());

    NewCall->takeName(Caller);
    NewCall->setCallingConv(Call.getCallingConv());
    NewCall->setAttributes(NewCallerPAL);

    // Preserve prof metadata if any.
    NewCall->copyMetadata(*Caller, { LLVMContext::MD_prof });

    Value* NV = NewCall;

    if (!Caller->use_empty())
        Caller->replaceAllUsesWith(NV);
    else if (Caller->hasValueHandle())
        ValueHandleBase::ValueIsRAUWd(Caller, NV);

    Caller->eraseFromParent();
    return true;
}

bool ResolveConstExprCalls::runOnModule(Module& M) {
    // Process through all functions and transform all constexpr cast calls
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
        Function* F = &(*I);

        SmallVector<CallInst*, 8> TransformList;
        SmallVector<ConstantExpr*, 8> DropList;

        // Collect all constexpr cast calls into list
        for (auto u = F->user_begin(), e = F->user_end(); u != e; u++) {
            CallInst* call = dyn_cast<CallInst>(*u);
            if (!call || call->getCalledOperand() != F) {
                auto CE = dyn_cast<ConstantExpr>(*u);
                if (CE) {
                    for (auto CEuser : CE->users())
                        if (CallInst* CallCE = dyn_cast<CallInst>(CEuser))
                            TransformList.push_back(CallCE);
                    DropList.push_back(CE);
                }
            }
        }

        for (auto I : TransformList)
            transformConstExprCastCall(*I);

        // If constexpr without uses drop all references on it
        for (auto I : DropList)
            if (I->user_empty())
                I->dropAllReferences();
    }

    return true;
}
