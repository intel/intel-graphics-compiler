/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "Compiler/Optimizer/OpenCLPasses/AggregateArguments/AggregateArguments.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/Function.h"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG1 "igc-agg-arg-analysis"
#define PASS_DESCRIPTION1 "Analyze aggregate arguments"
#define PASS_CFG_ONLY1 false
#define PASS_ANALYSIS1 false
IGC_INITIALIZE_PASS_BEGIN(AggregateArgumentsAnalysis, PASS_FLAG1, PASS_DESCRIPTION1, PASS_CFG_ONLY1, PASS_ANALYSIS1)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(AggregateArgumentsAnalysis, PASS_FLAG1, PASS_DESCRIPTION1, PASS_CFG_ONLY1, PASS_ANALYSIS1)

// Register pass to igc-opt
#define PASS_FLAG2 "igc-agg-arg"
#define PASS_DESCRIPTION2 "Resolve aggregate arguments"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(ResolveAggregateArguments, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(ResolveAggregateArguments, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)

char AggregateArgumentsAnalysis::ID = 0;
char ResolveAggregateArguments::ID = 0;

bool isSupportedAggregateArgument(Argument* arg)
{
    if (arg->getType()->isPointerTy() && arg->hasByValAttr())
    {
        Type* type = arg->getType()->getPointerElementType();

        if (StructType * structType = dyn_cast<StructType>(type))
        {
            return !structType->isOpaque();
        }
    }
    return false;
}

AggregateArgumentsAnalysis::AggregateArgumentsAnalysis() : FunctionPass(ID)
{
    initializeAggregateArgumentsAnalysisPass(*PassRegistry::getPassRegistry());
}

//
// This pass "flattens" aggregate (struct and array, non pointer) kernel
// arguments into multiple implicit basic type arguments.  This pass
// must be run after function inlining.
//
bool AggregateArgumentsAnalysis::runOnFunction(Function& F)
{
    if (F.isDeclaration())
    {
        return false;
    }

    if (!isEntryFunc(getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(), &F))
    {
        return false;
    }

    m_pDL = &F.getParent()->getDataLayout();

    bool changed = false;

    Function::arg_iterator argument = F.arg_begin();
    for (; argument != F.arg_end(); ++argument)
    {
        Argument* arg = &(*argument);

        if (!isSupportedAggregateArgument(arg))
        {
            continue;
        }
        m_argList.clear();

        Type* type = arg->getType()->getPointerElementType();
        IGC_ASSERT(m_pDL->getStructLayout(cast<StructType>(type))->getSizeInBytes() < UINT_MAX);
        addImplictArgs(type, 0);
        ImplicitArgs::addStructArgs(F, arg, m_argList, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());
        changed = true;
    }

    return changed;
}

static uint64_t getNumElements(SequentialType* type)
{
    if (ArrayType * arrayType = dyn_cast<ArrayType>(type))
    {
        return arrayType->getNumElements();
    }
    if (VectorType * vectorType = dyn_cast<VectorType>(type))
    {
        return vectorType->getNumElements();
    }
    IGC_ASSERT_MESSAGE(0, "expected array or vector");
    return 0;
}

void AggregateArgumentsAnalysis::addImplictArgs(Type* type, uint64_t baseAllocaOffset)
{
    IGC_ASSERT(baseAllocaOffset < UINT_MAX);
    // Structs and Unions are StructTypes
    if (StructType * structType = dyn_cast<StructType>(type))
    {
        const StructLayout* layout = m_pDL->getStructLayout(structType);

        unsigned int numElements = structType->getStructNumElements();

        // build the implicit arguments forwards for all elements
        // in the struct
        for (unsigned int i = 0; i < numElements; ++i)
        {
            Type* elementType = structType->getElementType(i);
            uint64_t elementOffsetInStruct = layout->getElementOffset(i);

            addImplictArgs(elementType, baseAllocaOffset + elementOffsetInStruct);
        }
    }
    else if (isa<ArrayType>(type) || isa<VectorType>(type))
    {
        SequentialType* seqType = cast<SequentialType>(type);
        uint64_t numElements = getNumElements(seqType);
        IGC_ASSERT(numElements < UINT_MAX);

        Type* elementType = seqType->getElementType();
        uint64_t elementSize = m_pDL->getTypeStoreSize(elementType);

        // build the implicit arguments forwards for all elements of the
        // array.  If this happens to be an array of struct, the elements
        // of the struct will be handled in the recursive step.
        for (unsigned int i = 0; i < numElements; ++i)
        {
            addImplictArgs(elementType, baseAllocaOffset + i * elementSize);
        }
    }
    else
    {
        // ...finally we have found a basic type contained inside
        // the aggregate.  Add it to the list of implicit args.
        unsigned int elementSize = (unsigned int)type->getPrimitiveSizeInBits();
        if (PointerType *PT = dyn_cast<PointerType>(type)) {
            elementSize = m_pDL->getPointerSize(PT->getAddressSpace()) * 8;
        }

        ImplicitArg::ArgType implicitArgType = ImplicitArg::CONSTANT_REG_DWORD;

        switch (elementSize)
        {
        case 8:
            implicitArgType = ImplicitArg::CONSTANT_REG_BYTE;
            break;
        case 16:
            implicitArgType = ImplicitArg::CONSTANT_REG_WORD;
            break;
        case 32:
            if (type->isFloatTy())
            {
                implicitArgType = ImplicitArg::CONSTANT_REG_FP32;
            }
            else
            {
                implicitArgType = ImplicitArg::CONSTANT_REG_DWORD;
            }
            break;
        case 64:
            implicitArgType = ImplicitArg::CONSTANT_REG_QWORD;
            break;
        default:
            IGC_ASSERT_MESSAGE(0, "unknown primitve type");
            break;
        };

        m_argList.push_back(ImplicitArg::StructArgElement(implicitArgType, static_cast<unsigned int>(baseAllocaOffset)));
    }
}

ResolveAggregateArguments::ResolveAggregateArguments() : FunctionPass(ID)
{
    initializeResolveAggregateArgumentsPass(*PassRegistry::getPassRegistry());
}

bool ResolveAggregateArguments::runOnFunction(Function& F)
{
    if (!isEntryFunc(getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(), &F))
    {
        return false;
    }

    m_implicitArgs = ImplicitArgs(F, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());

    m_pFunction = &F;

    bool changed = false;
    IRBuilder<> irBuilder(&F.getEntryBlock(), F.getEntryBlock().begin());

    Function::arg_iterator argument = F.arg_begin();
    for (; argument != F.arg_end(); ++argument)
    {
        Argument* arg = &(*argument);

        if (!isSupportedAggregateArgument(arg))
        {
            continue;
        }

        StructType* structType = cast<StructType>(arg->getType()->getPointerElementType());

        // LLVM assumes the caller has create an alloca and pushed the contents
        // of the struct on the stack.  Since we dont have a caller, create
        // the alloca here.
        std::string allocaName = std::string(arg->getName()) + "_alloca";
        llvm::AllocaInst* base = irBuilder.CreateAlloca(structType, 0, allocaName);

        // Now that we have the alloca push the contents of the struct onto the stack
        storeArgument(arg, base, irBuilder);

        arg->replaceAllUsesWith(base);
        changed = true;
    }

    return changed;
}

void ResolveAggregateArguments::storeArgument(const Argument* arg, AllocaInst* base, IRBuilder<>& irBuilder)
{
    unsigned int startArgNo, endArgNo;
    getImplicitArg(arg->getArgNo(), startArgNo, endArgNo);
    unsigned int baseImplicitArg = IGCLLVM::GetFuncArgSize(m_pFunction) - m_implicitArgs.size();

    // Iterate over all function arguments till reach the first implicit argument
    // associated with the explicit given argument.
    Function::arg_iterator implicitArgToStore = m_pFunction->arg_begin();
    for (unsigned int i = 0; i < baseImplicitArg + startArgNo; ++i, ++implicitArgToStore);

    Value* baseAsPtri8 = irBuilder.CreateBitCast(base, Type::getInt8PtrTy(base->getContext(), ADDRESS_SPACE_PRIVATE));

    // Iterate over all base type args of the structure and store them
    // into the correct offset from the alloca.
    for (unsigned int i = startArgNo; i < endArgNo; ++i, ++implicitArgToStore)
    {
        unsigned int baseAllocaOffset = m_implicitArgs.getStructArgOffset(i);

        Value* offsetFromBase = ConstantInt::get(Type::getInt32Ty(base->getContext()), baseAllocaOffset);
        Value* storeAddress = irBuilder.CreateGEP(baseAsPtri8, offsetFromBase);
        Value* offsetAsPointer = irBuilder.CreateBitCast(storeAddress,
            implicitArgToStore->getType()->getPointerTo(ADDRESS_SPACE_PRIVATE));
        irBuilder.CreateStore(&(*implicitArgToStore), offsetAsPointer);
    }

}

void ResolveAggregateArguments::getImplicitArg(unsigned int explicitArgNo, unsigned int& startArgNo, unsigned int& endArgNo)
{
    unsigned int numImplicitArgs = m_implicitArgs.size();

    unsigned int implicitAtgIndex = 0;

    // look for the first implicit arg that maps back to our explicit argument
    for (; implicitAtgIndex < numImplicitArgs; ++implicitAtgIndex)
    {
        // If found first implicit argument associated with given explicit argument index, break.
        if (m_implicitArgs.getExplicitArgNum(implicitAtgIndex) == explicitArgNo) break;
    }
    startArgNo = implicitAtgIndex;

    // look for the last implicit arg that maps back to our explicit argument
    for (; implicitAtgIndex < numImplicitArgs; ++implicitAtgIndex)
    {
        // If passed last implicit argument associated with given explicit argument index, break;.
        if (m_implicitArgs.getExplicitArgNum(implicitAtgIndex) != explicitArgNo) break;
    }
    endArgNo = implicitAtgIndex;
}
