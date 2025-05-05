/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AccuracyDecoratedCallsBiFResolution.hpp"

#include "llvmWrapper/IR/Type.h"
#include "llvm/IR/Attributes.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "Probe/Assertion.h"

using namespace IGC;
using namespace llvm;

// Register pass to igc-opt
#define PASS_FLAG "igc-accuracy-decorated-calls-bif-resolution"
#define PASS_DESCRIPTION "Accuracy decorated calls BiF resolution"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(AccuracyDecoratedCallsBiFResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(AccuracyDecoratedCallsBiFResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char AccuracyDecoratedCallsBiFResolution::ID = 0;

static inline const char* toString(Accuracy a)
{
    switch (a)
    {
    default:
        return "";
    case HIGH_ACCURACY:
        return "high accuracy";
    case LOW_ACCURACY:
        return "low accuracy";
    case ENHANCED_PRECISION:
        return "enhanced precision";
    }
}

AccuracyDecoratedCallsBiFResolution::AccuracyDecoratedCallsBiFResolution() : ModulePass(ID)
{
    initializeAccuracyDecoratedCallsBiFResolutionPass(*PassRegistry::getPassRegistry());
}

void AccuracyDecoratedCallsBiFResolution::initNameToBuiltinMap()
{
#define DEF_NAME_TO_BUILTIN(name, accuracy, builtin) \
    m_nameToBuiltin[name][accuracy] = builtin;
#include "NameToBuiltinDef.hpp"
#undef DEF_NAME_TO_BUILTIN
}

bool AccuracyDecoratedCallsBiFResolution::runOnModule(Module& M)
{
    m_Module = static_cast<Module*>(&M);

    initNameToBuiltinMap();

    visit(M);

    return m_changed;
}

void AccuracyDecoratedCallsBiFResolution::visitBinaryOperator(BinaryOperator& inst)
{
    // not supported by BiFModule yet;
    return;

    if (!inst.getType()->isFloatingPointTy())
        return;

    MDNode* MD = inst.getMetadata("fpbuiltin-max-error");
    if (!MD)
        return;

    IGC_ASSERT_MESSAGE(!IGCLLVM::isBFloatTy(inst.getType()), "bfloat type is not supported with fpbuiltin-max-error decoration");
    if (IGCLLVM::isBFloatTy(inst.getType()))
        return;

    std::vector<Value*> args{};
    args.push_back(inst.getOperand(0));
    args.push_back(inst.getOperand(1));

    StringRef maxErrorStr = cast<MDString>(MD->getOperand(0))->getString();
    const std::string oldFuncName = inst.getOpcodeName();

    Instruction* currInst = cast<Instruction>(&inst);
    Function* newFunc = getOrInsertNewFunc(oldFuncName, inst.getType(),
        args, {}, CallingConv::SPIR_FUNC, maxErrorStr, currInst);

    CallInst* newCall = CallInst::Create(newFunc, args, inst.getName(), &inst);
    llvm::Attribute attr = llvm::Attribute::get(inst.getContext(), "fpbuiltin-max-error", maxErrorStr);
    newCall->addFnAttr(attr);

    inst.replaceAllUsesWith(newCall);
    inst.eraseFromParent();
}

void AccuracyDecoratedCallsBiFResolution::visitCallInst(CallInst& callInst)
{
    AttributeList attributeList = callInst.getAttributes();
    AttributeSet callAttributes = attributeList.getFnAttrs();

    if (!callAttributes.hasAttribute("fpbuiltin-max-error"))
        return;

    Function* F = callInst.getCalledFunction();
    IGC_ASSERT_MESSAGE(nullptr != F, "Called function is null");
    if (!F)
        return;

    IGC_ASSERT_MESSAGE(!IGCLLVM::isBFloatTy(F->getType()), "bfloat type is not supported with fpbuiltin-max-error decoration");
    if (IGCLLVM::isBFloatTy(F->getType()))
        return;

    std::vector<Value*> args{};
    for (const auto& arg : callInst.args())
        args.push_back(arg);

    llvm::Attribute maxErrAttr = callAttributes.getAttribute("fpbuiltin-max-error");
    StringRef maxErrorStr = maxErrAttr.getValueAsString();

    Instruction* currInst = cast<Instruction>(&callInst);
    Function* newFunc = getOrInsertNewFunc(F->getName(), F->getReturnType(),
        args, F->getAttributes(), F->getCallingConv(), maxErrorStr, currInst);

    CallInst* newCall = CallInst::Create(newFunc, args, callInst.getName(), &callInst);
    newCall->setCallingConv(callInst.getCallingConv());
    newCall->setAttributes(callInst.getAttributes());

    callInst.replaceAllUsesWith(newCall);
    callInst.eraseFromParent();
}

Function* AccuracyDecoratedCallsBiFResolution::getOrInsertNewFunc(
    const StringRef oldFuncName, Type* funcType, const ArrayRef<Value*> args,
    const AttributeList attributes, CallingConv::ID callingConv, const StringRef maxErrorStr,
    const Instruction* currInst)
{
    double maxError = 0;
    maxErrorStr.getAsDouble(maxError);
    double ULPCutOff = funcType->isFloatTy() ? 0x1 << 12 : 0x1 << 26; // 2^12 : 2^26
    const Accuracy accuracy = getAccuracy(maxError, ULPCutOff, currInst);

    std::vector<Type*> argTypes{};
    for (const auto& arg : args)
        argTypes.push_back(arg->getType());

    FunctionType* FT = FunctionType::get(funcType, argTypes, false);
    std::string newFuncName = getFunctionName(oldFuncName, accuracy, currInst);
    Function* newFunc = cast<Function>(m_Module->getOrInsertFunction(newFuncName, FT, attributes).getCallee());
    IGC_ASSERT(nullptr != newFunc);
    newFunc->setCallingConv(callingConv);

    m_changed = true;
    return newFunc;
}

std::string AccuracyDecoratedCallsBiFResolution::getFunctionName(const StringRef oldFuncName, Accuracy accuracy, const Instruction* currInst) const
{
    if (!m_nameToBuiltin.count(oldFuncName.str()))
    {
        // Temporary until we're certain of the mangling used for
        // sinpi cospi tanpi asinpi acospi atanpi sincos sincospi
        size_t idx = oldFuncName.find("_spirv_ocl_");
        IGC_ASSERT(StringRef::npos != idx);
        StringRef croppedName = oldFuncName.substr(idx);

        if (!m_nameToBuiltin.count(croppedName.str()))
            return oldFuncName.str(); // Function not found - don't change the name.

        // Function found in map with cropped name. Get the mapping.
        return getFunctionName(croppedName, accuracy, currInst);
    }

    if (!m_nameToBuiltin.at(oldFuncName.str()).count(accuracy))
    {
        std::string warningMessage = "Built-in function not found for " + oldFuncName.str()
                                   + " at " + toString(accuracy) + ". Choosing higher precision.";
        getAnalysis<CodeGenContextWrapper>().getCodeGenContext()
            ->EmitWarning(warningMessage.c_str(), currInst);

        switch (accuracy)
        {
        default:
            IGC_ASSERT_MESSAGE(false, "Unreachable, NameToBuiltinDef.hpp is likely broken");
            return oldFuncName.str();
        case ENHANCED_PRECISION:
            return getFunctionName(oldFuncName, LOW_ACCURACY, currInst);
        case LOW_ACCURACY:
            return getFunctionName(oldFuncName, HIGH_ACCURACY, currInst);
        }
    }
    return m_nameToBuiltin.at(oldFuncName.str()).at(accuracy);
}

Accuracy AccuracyDecoratedCallsBiFResolution::getAccuracy(double maxError, double cutOff, const Instruction* currInst) const
{
    if (maxError < 1.0)
        getAnalysis<CodeGenContextWrapper>().getCodeGenContext()
            ->EmitError("fpbuiltin-max-error can't have values below 1.0", currInst);

    if (maxError < 4.0)
        return HIGH_ACCURACY;

    if (maxError < cutOff) // 2^12 (for f32) or 2^26 (for f64)
        return LOW_ACCURACY;

    return ENHANCED_PRECISION;
}
