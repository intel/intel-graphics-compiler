/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BufferBoundsChecking.hpp"
#include "BufferBoundsCheckingPatcher.hpp"

#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Mangler.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Regex.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/Optimizer/ValueTracker.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-buffer-bounds-checking"
#define PASS_DESCRIPTION "Buffer bounds checking"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(BufferBoundsChecking, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(BufferBoundsChecking, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char BufferBoundsChecking::ID = 0;

BufferBoundsChecking::BufferBoundsChecking() : ModulePass(ID)
{
    initializeBufferBoundsCheckingPass(*PassRegistry::getPassRegistry());
}

bool BufferBoundsChecking::runOnModule(Module& M)
{
    modified = false;

    metadataUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    moduleMetadata = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    auto context = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    compileUnit = nullptr;
    if (M.debug_compile_units().begin() != M.debug_compile_units().end())
    {
        compileUnit = *M.debug_compile_units().begin();
    }

    for (auto& function : M.functions())
    {
        if (!isEntryFunc(metadataUtils, &function))
        {
            continue;
        }

        // Add implicit arg
        ImplicitArg::ArgMap argMap;
        for (const auto& arg : function.args())
        {
            if (!argumentQualifiesForChecking(&arg))
            {
                continue;
            }

            argMap[ImplicitArg::BUFFER_SIZE].insert(arg.getArgNo());
        }

        if (!argMap.empty())
        {
            ImplicitArgs::addNumberedArgs(function, argMap, metadataUtils);
            modified = true;
        }

        // Kernel args
        kernelArgs = new KernelArgs(function, &function.getParent()->getDataLayout(), metadataUtils, moduleMetadata, context->platform.getGRFSize());

        // Local and global Ids
        std::tie(localId0, localId1, localId2) = getLocalIds(function);
        std::tie(globalId0, globalId1, globalId2) = getGlobalIds(function);

        // Collect loads/stores
        loadsAndStoresToCheck.clear();
        visit(function);

        // Handle loads/stores
        for (auto& instruction : loadsAndStoresToCheck)
        {
            handleLoadStore(instruction);
        }
    }

    return modified;
}

void BufferBoundsChecking::visitLoadInst(LoadInst& load)
{
    loadsAndStoresToCheck.push_back(&load);
}

void BufferBoundsChecking::visitStoreInst(StoreInst& store)
{
    loadsAndStoresToCheck.push_back(&store);
}

std::tuple<Value*, Value*, Value*> BufferBoundsChecking::getLocalIds(Function& function)
{
    auto getOrCreateBuiltin = [&function](const char* name)
    {
        return function.getParent()->getOrInsertFunction(
            name,
            FunctionType::get(Type::getInt32Ty(function.getContext()),
            false));
    };

    return {
        CallInst::Create(getOrCreateBuiltin("__builtin_IB_get_local_id_x"), {}, "localId0", function.getEntryBlock().getFirstNonPHI()),
        CallInst::Create(getOrCreateBuiltin("__builtin_IB_get_local_id_y"), {}, "localId1", function.getEntryBlock().getFirstNonPHI()),
        CallInst::Create(getOrCreateBuiltin("__builtin_IB_get_local_id_z"), {}, "localId2", function.getEntryBlock().getFirstNonPHI()),
    };
}

std::tuple<Value*, Value*, Value*> BufferBoundsChecking::getGlobalIds(Function& function)
{
    auto builtin = function.getParent()->getOrInsertFunction(
        "__builtin_IB_get_group_id",
        FunctionType::get(Type::getInt32Ty(function.getContext()), { Type::getInt32Ty(function.getContext()) },
        false));

    return {
        CallInst::Create(builtin, { ConstantInt::get(Type::getInt32Ty(function.getContext()), 0) }, "globalId0", function.getEntryBlock().getFirstNonPHI()),
        CallInst::Create(builtin, { ConstantInt::get(Type::getInt32Ty(function.getContext()), 1) }, "globalId1", function.getEntryBlock().getFirstNonPHI()),
        CallInst::Create(builtin, { ConstantInt::get(Type::getInt32Ty(function.getContext()), 2) }, "globalId2", function.getEntryBlock().getFirstNonPHI()),
    };
}

void BufferBoundsChecking::handleLoadStore(Instruction* instruction)
{
    Value* pointer = nullptr;
    if (auto load = dyn_cast<LoadInst>(instruction))
    {
        pointer = load->getPointerOperand();
    }
    else if (auto store = dyn_cast<StoreInst>(instruction))
    {
        pointer = store->getPointerOperand();
    }
    else
    {
        IGC_ASSERT(0);
    }

    auto accessInfo = getAccessInfo(instruction, pointer);
    if (!accessInfo.bufferOffsetInBytes)
    {
        return;
    }
    createBoundsCheckingCode(instruction, accessInfo);
    modified = true;
}

bool BufferBoundsChecking::argumentQualifiesForChecking(const Argument* argument)
{
    auto pointerType = dyn_cast<PointerType>(argument->getType());
    return pointerType &&
           (pointerType->getPointerAddressSpace() == ADDRESS_SPACE_CONSTANT ||
            pointerType->getPointerAddressSpace() == ADDRESS_SPACE_GLOBAL);
}

Value* BufferBoundsChecking::createBoundsCheckingCondition(const AccessInfo& accessInfo, Instruction* insertBefore)
{
    const auto zero = ConstantInt::get(Type::getInt64Ty(insertBefore->getModule()->getContext()), 0);

    auto bufferSizePlaceholder = createBufferSizePlaceholder(accessInfo.implicitArgBufferSizeIndex, insertBefore);

    auto bufferSizeIsZero = new ICmpInst(insertBefore, ICmpInst::ICMP_EQ, bufferSizePlaceholder, zero);
    auto bufferOffsetIsGreaterOrEqualZero = new ICmpInst(insertBefore, ICmpInst::ICMP_SGE, accessInfo.bufferOffsetInBytes, zero);
    auto upperBound = BinaryOperator::Create(Instruction::Sub, bufferSizePlaceholder, accessInfo.elementSizeInBytes, "", insertBefore);
    auto bufferOffsetIsLessThanSizeMinusElemSize = new ICmpInst(insertBefore, ICmpInst::ICMP_SLT, accessInfo.bufferOffsetInBytes, upperBound);

    return BinaryOperator::Create(Instruction::Or,
        bufferSizeIsZero,
        BinaryOperator::Create(Instruction::And,
            bufferOffsetIsGreaterOrEqualZero,
            bufferOffsetIsLessThanSizeMinusElemSize,
            "",
            insertBefore),
        "",
        insertBefore);
}

Value* BufferBoundsChecking::createLoadStoreReplacement(Instruction* instruction, Instruction* insertBefore)
{
    if (auto load = dyn_cast<LoadInst>(instruction))
    {
        return Constant::getNullValue(instruction->getType());
    }
    else if (auto store = dyn_cast<StoreInst>(instruction))
    {
        return new StoreInst(store->getValueOperand(), ConstantPointerNull::get(dyn_cast<PointerType>(store->getPointerOperandType())), insertBefore);
    }
    else
    {
        IGC_ASSERT(0);
        return nullptr;
    }
}

void BufferBoundsChecking::createAssertCall(const AccessInfo& accessInfo, Instruction* insertBefore)
{
    auto M = insertBefore->getModule();

    auto assertArgs = createAssertArgs(accessInfo, insertBefore);
    auto assertArgsTypes = SmallVector<Type*, 4>{};
    std::transform(assertArgs.begin(), assertArgs.end(), std::back_inserter(assertArgsTypes), [](Value* value) { return value->getType(); });

    auto assert = cast<Function>(M->getOrInsertFunction(
        compileUnit ? "__bufferoutofbounds_assert" : "__bufferoutofbounds_assert_nodebug",
        FunctionType::get(Type::getVoidTy(M->getContext()), assertArgsTypes, false)
    ).getCallee());

    auto call = CallInst::Create(assert, assertArgs, "", insertBefore);
    call->setCallingConv(CallingConv::SPIR_FUNC);
}

SmallVector<Value*, 4> BufferBoundsChecking::createAssertArgs(const AccessInfo& accessInfo, llvm::Instruction* insertBefore)
{
    auto createGEP = [insertBefore](GlobalVariable* globalVariable)
    {
        const auto zero = ConstantInt::getSigned(Type::getInt32Ty(globalVariable->getParent()->getContext()), 0);
        auto result = GetElementPtrInst::Create(
            globalVariable->getValueType(),
            globalVariable,
            { zero, zero },
            "",
            insertBefore
        );
        result->setIsInBounds(true);
        return result;
    };

    SmallVector<Value*, 4> result;
    if (compileUnit)
    {
        result.push_back(createGEP(getOrCreateGlobalConstantString(insertBefore->getModule(), accessInfo.filename)));
        result.push_back(ConstantInt::getSigned(Type::getInt32Ty(insertBefore->getContext()), accessInfo.line));
        result.push_back(ConstantInt::getSigned(Type::getInt32Ty(insertBefore->getContext()), accessInfo.column));
        result.push_back(createGEP(getOrCreateGlobalConstantString(insertBefore->getModule(), accessInfo.bufferName)));
    }
    else
    {
        result.push_back(accessInfo.bufferAddress);
    }
    result.push_back(accessInfo.bufferOffsetInBytes);
    result.push_back(createBufferSizePlaceholder(accessInfo.implicitArgBufferSizeIndex, insertBefore));
    result.push_back(localId0);
    result.push_back(localId1);
    result.push_back(localId2);
    result.push_back(globalId0);
    result.push_back(globalId1);
    result.push_back(globalId2);

    return result;
}

GlobalVariable* BufferBoundsChecking::getOrCreateGlobalConstantString(Module* M, StringRef str)
{
    if (stringsCache.count(str) == 0)
    {
        stringsCache[str] = new GlobalVariable(
            *M,
            ArrayType::get(Type::getInt8Ty(M->getContext()), str.size() + 1),
            true,
            GlobalValue::InternalLinkage,
            ConstantDataArray::getString(M->getContext(), str, true),
            "",
            nullptr,
            GlobalValue::ThreadLocalMode::NotThreadLocal,
            ADDRESS_SPACE_CONSTANT);
        stringsCache[str]->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);
    }

    return stringsCache[str];
}

void BufferBoundsChecking::createBoundsCheckingCode(Instruction* instruction, const AccessInfo& accessInfo)
{
    auto condition = createBoundsCheckingCondition(accessInfo, instruction);

    // Generate if-then-else
    Instruction* thenTerminator = nullptr;
    Instruction* elseTerminator = nullptr;
    SplitBlockAndInsertIfThenElse(condition, instruction, &thenTerminator, &elseTerminator);

    // Merge block
    auto mergeBlock = instruction->getParent();
    mergeBlock->setName("bufferboundschecking.end");

    // Valid offset
    auto thenBlock = thenTerminator->getParent();
    thenBlock->setName("bufferboundschecking.valid");
    instruction->moveBefore(thenTerminator);

    // Invalid offset
    auto elseBlock = elseTerminator->getParent();
    elseBlock->setName("bufferboundschecking.invalid");
    createAssertCall(accessInfo, elseTerminator);
    auto replacement = createLoadStoreReplacement(instruction, elseTerminator);

    // PhiNode
    if (isa<LoadInst>(instruction))
    {
        PHINode* phi = PHINode::Create(instruction->getType(), 2, "", &mergeBlock->front());
        instruction->replaceUsesOutsideBlock(phi, thenBlock);
        phi->addIncoming(instruction, thenBlock);
        phi->addIncoming(replacement, elseBlock);
    }
}

const KernelArg* BufferBoundsChecking::getKernelArg(Value* value)
{
    for (const KernelArg& arg : *kernelArgs)
    {
        if (arg.getArg() == value)
        {
            return &arg;
        }
    }
    return nullptr;
}

const KernelArg* BufferBoundsChecking::getKernelArgFromPtr(const PointerType& pointerType, Value* value)
{
    if (value == nullptr)
        return nullptr;

    // stripPointerCasts might skip addrSpaceCast, thus check if AS is still
    // the original one.
    unsigned int ptrAS = pointerType.getAddressSpace();
    if (cast<PointerType>(value->getType())->getAddressSpace() == ptrAS && !isa<Instruction>(value))
    {
        if (const KernelArg* arg = getKernelArg(value))
            return arg;
    }
    return nullptr;
}

BufferBoundsChecking::AccessInfo BufferBoundsChecking::getAccessInfo(Instruction* instruction, Value* value)
{
    auto pointerType = dyn_cast<PointerType>(value->getType());
    IGC_ASSERT_MESSAGE(pointerType, "Expected scalar Pointer (No support to vector of pointers");
    if (!pointerType || (pointerType->getAddressSpace() != ADDRESS_SPACE_GLOBAL &&
        pointerType->getAddressSpace() != ADDRESS_SPACE_CONSTANT))
    {
        return AccessInfo{};
    }

    // Track value
    auto base = ValueTracker::track(value, instruction->getFunction(), metadataUtils, moduleMetadata);
    const auto* arg = getKernelArgFromPtr(*pointerType, base);
    if (!arg)
    {
        return AccessInfo{};
    }

    auto baseAddress = new PtrToIntInst(base, Type::getInt64Ty(instruction->getContext()), "", instruction);
    auto address = new PtrToIntInst(value, Type::getInt64Ty(instruction->getContext()), "", instruction);

    auto debugLoc = instruction->getDebugLoc();

    AccessInfo result;
    result.filename = compileUnit ? compileUnit->getFilename() : "";
    result.line = debugLoc ? debugLoc->getLine() : 0;
    result.column = debugLoc ? debugLoc->getColumn() : 0;
    result.bufferName = arg->getArg()->getName();
    result.bufferAddress = baseAddress;
    result.bufferOffsetInBytes = BinaryOperator::CreateSub(address, baseAddress, "", instruction);
    result.implicitArgBufferSizeIndex = (int)std::count_if(
        instruction->getFunction()->arg_begin(),
        instruction->getFunction()->arg_begin() + arg->getAssociatedArgNo(),
        [this](const Argument& arg)
        {
            return argumentQualifiesForChecking(&arg);
        });

    Type* type = nullptr;
    if (auto load = dyn_cast<LoadInst>(instruction))
    {
        type = instruction->getType();
    }
    else if (auto store = dyn_cast<StoreInst>(instruction))
    {
        type = store->getValueOperand()->getType();
    }
    else
    {
        IGC_ASSERT(0);
    }
    result.elementSizeInBytes = ConstantInt::get(Type::getInt64Ty(instruction->getContext()),
                                          instruction->getModule()->getDataLayout().getTypeSizeInBits(type) / 8);

    return result;
}

Value* BufferBoundsChecking::createBufferSizePlaceholder(uint32_t implicitArgBufferSizeIndex, Instruction* insertBefore)
{
    if (!bufferSizePlaceholderFunction)
    {
        auto M = insertBefore->getModule();
        bufferSizePlaceholderFunction = cast<Function>(M->getOrInsertFunction(
            BufferBoundsCheckingPatcher::BUFFER_SIZE_PLACEHOLDER_FUNCTION_NAME,
            FunctionType::get(Type::getInt64Ty(M->getContext()), {Type::getInt64Ty(M->getContext())}, false)
        ).getCallee());
    }

    auto result = CallInst::Create(bufferSizePlaceholderFunction, {
            ConstantInt::getSigned(Type::getInt64Ty(insertBefore->getContext()), implicitArgBufferSizeIndex),
        }, "", insertBefore);
    result->setCallingConv(CallingConv::SPIR_FUNC);
    return result;
}
