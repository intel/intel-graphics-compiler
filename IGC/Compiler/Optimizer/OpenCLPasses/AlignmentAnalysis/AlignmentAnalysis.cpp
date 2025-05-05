/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AlignmentAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvmWrapper/Support/Alignment.h"
#include "llvmWrapper/IR/Argument.h"
#include "common/LLVMWarningsPop.hpp"
#include <deque>
#include <set>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-fix-alignment"
#define PASS_DESCRIPTION "Fix argument alignments ad alignments of instructions according to OpenCL rules"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(AlignmentAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(AlignmentAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char AlignmentAnalysis::ID = 0;

static const Align MinimumAlignment = Align(1);

AlignmentAnalysis::AlignmentAnalysis() : FunctionPass(ID)
{
    initializeAlignmentAnalysisPass(*PassRegistry::getPassRegistry());
}

// Check if the function has OpenCL metadata that specifies the alignment of
// its arguments. If it does, set the LLVM alignment attribute of the
// arguments accordingly. This is helpful for passes like InferAlignment.
void AlignmentAnalysis::setArgumentAlignmentBasedOnOptionalMetadata(Function& F) {

    if (F.hasMetadata("kernel_arg_type")) {
        auto EmitOptFailure = [&F](const llvm::Twine& Msg) {
            DiagnosticInfoOptimizationFailure Diag(F, F.getSubprogram(), Msg);
            F.getContext().diagnose(Diag);
            };

        auto* MD = F.getMetadata("kernel_arg_type");
        if (F.arg_size() != MD->getNumOperands()) {
            EmitOptFailure(
                "Mismatch between the number of arguments and the number of "
                "kernel_arg_type metadata operands in function " +
                F.getName());
            return;
        }

        for (unsigned OpNumber = 0; OpNumber < MD->getNumOperands(); ++OpNumber) {
            auto* Op = dyn_cast<MDString>(MD->getOperand(OpNumber));
            if (!Op) {
                EmitOptFailure("Expected MDString in kernel_arg_type metadata in "
                    "function " +
                    F.getName());
                return;
            }

            auto* Arg = F.getArg(OpNumber);
            if (!Arg->getType()->isPointerTy())
                continue;

            if (Arg->getParamAlign()) {
                // If the alignment is already set, skip this argument.
                continue;
            }

            if (!Op->getString().endswith("*")) {
                // If the metadata string does not end with '*', skip this argument.
                // This can be e.g. a struct pointer passed byval.
                // DPC++ does not add "*" in this case and we will not be able to
                // set alignment for such arguments.
                return;
            }

            // Remove the trailing '*' from the metadata string
            StringRef KernelArgType = Op->getString().drop_back();

            StringRef ScalarType =
                KernelArgType.take_until([](char C) { return C >= '0' && C <= '9'; });

            auto ScalarAlignment =
                llvm::StringSwitch<std::optional<llvm::Align>>(ScalarType)
                .CasesLower("char", "uchar", llvm::Align(1))
                .CasesLower("short", "ushort", "half", llvm::Align(2))
                .CasesLower("int", "uint", "float", llvm::Align(4))
                .CasesLower("long", "ulong", "double", llvm::Align(8))
                .Default(std::nullopt);

            if (!ScalarAlignment) {
                // If the scalar type is not recognized, skip this argument - this can
                // be e.g. a struct pointer
                continue;
            }

            llvm::Align Alignment = *ScalarAlignment;
            uint64_t VectorSize = 0;
            KernelArgType = KernelArgType.drop_front(ScalarType.size());
            if (!KernelArgType.getAsInteger(10, VectorSize)) {
                if (VectorSize == 3)
                    VectorSize = 4;
                Alignment = Align(VectorSize * Alignment.value());
            }
            assert(Alignment.value() && "Alignment should not be zero!");

            Arg->addAttr(llvm::Attribute::getWithAlignment(F.getContext(), Alignment));
        }
    }
}


bool AlignmentAnalysis::runOnFunction(Function& F)
{

    setArgumentAlignmentBasedOnOptionalMetadata(F);

    m_DL = &F.getParent()->getDataLayout();

    // The work-list queue for the data flow algorithm
    std::deque<Instruction*> workList;

    // A helper set, to avoid inserting the same instruction
    // into the worklist several times. (this is the set of "grey" nodes).
    // We could instead perform lookups directly on the worklist, but this
    // is faster.
    std::set<Instruction*> inList;

    // First, initialize the worklist with all of the instructions in the function.
    for (llvm::inst_iterator inst = inst_begin(F), instEnd = inst_end(F); inst != instEnd; ++inst)
    {
        workList.push_back(&*inst);
        inList.insert(&*inst);
    }

    // It is more efficient to handle the earlier instructions first,
    // so we pop from the front.
    while (!inList.empty())
    {
        // Get the next instruction
        Instruction* inst = workList.front();
        workList.pop_front();
        inList.erase(inst);

        // Get the alignment of this instruction
        if (processInstruction(inst))
        {
            // The alignment of inst changed, (re-)process all users, by
            // adding them to the end of the queue.
            for (auto user = inst->user_begin(), userEnd = inst->user_end(); user != userEnd; ++user)
            {
                Instruction* userInst = cast<Instruction>(*user);
                // If the user is already in the queue, no need to add it again
                if (inList.find(userInst) == inList.end())
                {
                    workList.push_back(userInst);
                    inList.insert(userInst);
                }
            }
        }
    }

    // in a second step change the alignment for instructions which have improved
    bool Changed = false;
    for (llvm::inst_iterator inst = inst_begin(F), instEnd = inst_end(F); inst != instEnd; ++inst)
    {
        Changed |= SetInstAlignment(*inst);
    }
    return Changed;
}

Align AlignmentAnalysis::getConstantAlignment(uint64_t C) const
{
    if (C == 0)
    {
        return Align(Value::MaximumAlignment);
    }

    return std::min(Align(Value::MaximumAlignment), Align(1ULL << llvm::countTrailingZeros(C)));
}

Align AlignmentAnalysis::getAlignValue(Value* V) const
{
    const Align MinimumAlignmentValue = static_cast<Align>(MinimumAlignment);
    if (isa<Instruction>(V))
    {
        auto iter = m_alignmentMap.find(V);
        if (iter == m_alignmentMap.end())
        {
            // Instructions are initialize to maximum alignment
            // (this is the "top" value)
            return Align(Value::MaximumAlignment);
        }

        return iter->second;
    }
    else if (dyn_cast<Constant>(V))
    {
        if (ConstantInt * constInt = dyn_cast<ConstantInt>(V))
        {
            return getConstantAlignment(constInt->getZExtValue());
        }
        else if (GlobalVariable * GV = dyn_cast<GlobalVariable>(V))
        {
            Align align = GV->getAlign().valueOrOne();

            // If the globalvariable uses the default alignment, pull it from the datalayout
            if (align.value()==1)
            {
                return m_DL->getABITypeAlign(GV->getValueType());
            }
            else
            {
                return align;
            }
        }

        // Not an int or a globalvariable, be pessimistic.
        return MinimumAlignmentValue;
    }
    else if (Argument * arg = dyn_cast<Argument>(V))
    {
        if (arg->getType()->isPointerTy())
        {

            if (arg->hasAttribute(llvm::Attribute::Alignment)) {
                Align align = arg->getParamAlign().valueOrOne();
                // Note that align 1 has no effect on non-byval, non-preallocated arguments.
                if (align.value() != 1 || arg->hasPreallocatedAttr() || arg->hasByValAttr())
                    return align;
            }

            Type* pointedTo = IGCLLVM::getArgAttrEltTy(arg);
            if (pointedTo == nullptr && !IGCLLVM::isOpaquePointerTy(arg->getType()))
                pointedTo = IGCLLVM::getNonOpaquePtrEltTy(arg->getType());

            // Pointer arguments are guaranteed to be aligned on the ABI alignment
            if (pointedTo != nullptr && pointedTo->isSized())
            {
                return m_DL->getABITypeAlign(pointedTo);
            }
            else
            {
                // We have some pointer-to-opaque-types which are not real pointers -
                // this is used to pass things like images around.
                // Apparently, DataLayout being asked about the ABI alignment of opaque types.
                // So, we don't.
                return MinimumAlignmentValue;
            }
        }
        else
        {
            // We don't know anything about integer arguments.
            return MinimumAlignmentValue;
        }
    }

    // Be pessimistic
    return MinimumAlignmentValue;
}

bool AlignmentAnalysis::processInstruction(llvm::Instruction* I)
{
    // Get the currently known alignment of I.
    Align currAlign = getAlignValue(I);

    // Compute the instruction's alignment
    // using the alignment of the arguments.
    uint64_t newAlign = 0;
    if (I->getType()->isPointerTy())
    {
        // If a pointer is specifically given an 'align' field in the MD, use it.
        MDNode* alignmentMD = I->getMetadata("align");
        if (alignmentMD)
            newAlign = mdconst::extract<ConstantInt>(alignmentMD->getOperand(0))->getZExtValue();
    }
    if (!newAlign)
    {
        newAlign = visit(I).value();
    }

    // The new alignment may not be better than the current one,
    // since we're only allowed to go in one direction in the lattice.
    newAlign = std::min(currAlign.value(), newAlign);

    // If the alignment changed, we want to process the users of this
    // value, so return true. Otherwise, this instruction has stabilized
    // (for now).

    if (newAlign != currAlign.value())
    {
        m_alignmentMap[I] = Align(newAlign);
        return true;
    }

    return false;
}

Align AlignmentAnalysis::visitInstruction(Instruction& I)
{
    // The safe thing to do for unknown instructions is to return 1.
    return MinimumAlignment;
}

Align AlignmentAnalysis::visitAllocaInst(AllocaInst& I)
{
    // Return the alignment of the alloca, which ought to be correct
    Align newAlign = I.getAlign();

    // If the alloca uses the default alignment, pull it from the datalayout
    if (!newAlign.value())
    {
        newAlign = m_DL->getABITypeAlign(I.getAllocatedType());
    }

    return newAlign;
}

Align AlignmentAnalysis::visitSelectInst(SelectInst& I)
{
    Value* srcTrue = I.getTrueValue();
    Value* srcFalse = I.getFalseValue();

    // In general this should be the GCD, but because we assume we are always aligned on
    // powers of 2, the GCD is the minimum.
    return iSTD::Min(getAlignValue(srcTrue), getAlignValue(srcFalse));
}

Align AlignmentAnalysis::visitPHINode(PHINode& I)
{
    Align newAlign = Align(Value::MaximumAlignment);

    // The alignment of a PHI is the minimal alignment of any of the
    // incoming values.
    unsigned numVals = I.getNumIncomingValues();
    for (unsigned int i = 0; i < numVals; ++i)
    {
        Value* op = I.getIncomingValue(i);
        newAlign = std::min(newAlign, getAlignValue(op));
    }

    return newAlign;
}

bool AlignmentAnalysis::SetInstAlignment(llvm::Instruction& I)
{
    if (isa<LoadInst>(I))
    {
        return SetInstAlignment(cast<LoadInst>(I));
    }
    else if (isa<StoreInst>(I))
    {
        return SetInstAlignment(cast<StoreInst>(I));
    }
    else if (isa<MemSetInst>(I))
    {
        return SetInstAlignment(cast<MemSetInst>(I));
    }
    else if (isa<MemCpyInst>(I))
    {
        return SetInstAlignment(cast<MemCpyInst>(I));
    }
    else if (isa<MemMoveInst>(I))
    {
        return SetInstAlignment(cast<MemMoveInst>(I));
    }
    return false;
}

bool AlignmentAnalysis::SetInstAlignment(LoadInst& I)
{
    // Set the align attribute of the load according to the detected
    // alignment of its operand.
    Align curAlign = I.getAlign();
    Align newAlign = std::max(curAlign, getAlignValue(I.getPointerOperand()));
    I.setAlignment(newAlign);
    return curAlign != newAlign;
}

bool AlignmentAnalysis::SetInstAlignment(StoreInst& I)
{
    // Set the align attribute of the store according to the detected
    // alignment of its operand.
    Align curAlign = I.getAlign();
    Align newAlign = std::max(I.getAlign(), getAlignValue(I.getPointerOperand()));
    I.setAlignment(newAlign);
    return curAlign != newAlign;
}

Align AlignmentAnalysis::visitAdd(BinaryOperator& I)
{
    // Addition can not improve the alignment.
    // In a more precise analysis, it could (e.g. 3 + 1 = 4)
    // but here, we only keep track of the highest power of 2
    // factor.
    // So, the best case scenario is that the alignment stays
    // the same if you add two values with the same alignment.
    // Note that it can not grow even in this case, because
    // we keep an underapproximation. That is:
    // If we know x and y were divisible by 4 but *not* 8,
    // then x + y would be divisible by 8. However, if x is divisible by 4
    // and y is divisible by 8, then x + y is only divisible by 4.
    Value* op0 = I.getOperand(0);
    Value* op1 = I.getOperand(1);

    return iSTD::Min(getAlignValue(op0), getAlignValue(op1));
}

Align AlignmentAnalysis::visitMul(BinaryOperator& I)
{
    // Because we are dealing with powers of 2,
    // align(x * y) = align(x) * align(y)
    Value* op0 = I.getOperand(0);
    Value* op1 = I.getOperand(1);

    return Align(
        assumeAligned(std::min(Value::MaximumAlignment,
            SaturatingMultiply(getAlignValue(op0).value(),
                getAlignValue(op1).value()))));
}

Align AlignmentAnalysis::visitShl(BinaryOperator& I)
{
    // If we are shifting left by a constant, we know the
    // alignment improves according to that value.
    // In any case, it can not drop.
    Value* op0 = I.getOperand(0);
    Value* op1 = I.getOperand(1);

    if (ConstantInt * constOp1 = dyn_cast<ConstantInt>(op1))
    {
        auto oldAlignVal = getAlignValue(op0).value();
        IGC_ASSERT_MESSAGE(oldAlignVal, "Alignment should not be zero!");
        uint64_t shiftVal = constOp1->getZExtValue();
        if (shiftVal >= std::numeric_limits<uint64_t>::digits) {
            // This means that the shl will overflow, so we should return the maximum
            // alignment.
            return Align(Value::MaximumAlignment);
        }

        auto newAlignVal = SaturatingMultiply(oldAlignVal, (uint64_t)1 << shiftVal);
        return Align(assumeAligned(std::min(Value::MaximumAlignment, newAlignVal)));
    }
    else
    {
        return getAlignValue(op0);
    }
}

Align AlignmentAnalysis::visitAnd(BinaryOperator& I)
{
    Value* op0 = I.getOperand(0);
    Value* op1 = I.getOperand(1);

    // If one of the operands has trailing zeroes up to some point,
    // then so will the result. So, the alignment is at least the maximum
    // of the operands.
    return iSTD::Max(getAlignValue(op0),
        getAlignValue(op1));
}

Align AlignmentAnalysis::visitGetElementPtrInst(GetElementPtrInst& I)
{
    // The alignment can never be better than the alignment of the base pointer
    Align newAlign = getAlignValue(I.getPointerOperand());

    gep_type_iterator GTI = gep_type_begin(&I);
    for (auto op = I.op_begin() + 1, opE = I.op_end(); op != opE; ++op, ++GTI)
    {
        uint64_t offset = 0;
        if (StructType * StTy = GTI.getStructTypeOrNull())
        {
            auto Field = static_cast<unsigned>((cast<Constant>(*op))->getUniqueInteger().getZExtValue());
            offset = static_cast<uint64_t>(m_DL->getStructLayout(StTy)->getElementOffset(Field));
        }
        else
        {
            Type* Ty = GTI.getIndexedType();
            auto multiplier = static_cast<uint64_t>(m_DL->getTypeAllocSize(Ty));
            offset = multiplier * getAlignValue(*op).value();
        }

        // It's possible offset is not a power of 2, because struct fields
        // may be aligned on all sorts of weird values. So we can not just
        // take the minimum between newAlign and offset, we need the
        // highest power of 2 that divides both.

        // x | y has trailing 0s exactly where both x and y have trailing 0s.
        newAlign = getConstantAlignment(newAlign.value() | offset);
    }

    return newAlign;
}

// Casts don't change the alignment.
// Technically we could do better (a trunc or an extend may improve alignment)
// but this doesn't seem important enough.
Align AlignmentAnalysis::visitBitCastInst(BitCastInst& I)
{
    return getAlignValue(I.getOperand(0));
}

Align AlignmentAnalysis::visitPtrToIntInst(PtrToIntInst& I)
{
    return getAlignValue(I.getOperand(0));
}

Align AlignmentAnalysis::visitIntToPtrInst(IntToPtrInst& I)
{
    return getAlignValue(I.getOperand(0));
}

Align AlignmentAnalysis::visitTruncInst(TruncInst& I)
{
    return getAlignValue(I.getOperand(0));
}

Align AlignmentAnalysis::visitZExtInst(ZExtInst& I)
{
    return getAlignValue(I.getOperand(0));
}

Align AlignmentAnalysis::visitSExtInst(SExtInst& I)
{
    return getAlignValue(I.getOperand(0));
}

Align AlignmentAnalysis::visitCallInst(CallInst& I)
{
    // Handle alignment for memcpy and memset
    Function* callee = I.getCalledFunction();
    // Skip indirect call!
    if (!callee)
        // return value does not matter
        return MinimumAlignment;

    MemIntrinsic* memIntr = dyn_cast<MemIntrinsic>(&I);
    if (!memIntr)
        return MinimumAlignment;

    Align alignment = MinimumAlignment;
    llvm::Intrinsic::ID ID = memIntr->getIntrinsicID();

    if (ID == Intrinsic::memcpy) {
        MemCpyInst* memCpy = dyn_cast<MemCpyInst>(&I);
        IGC_ASSERT(memCpy);
        if (memCpy) {
            alignment = Align(std::min(memCpy->getDestAlign().valueOrOne().value(),
                memCpy->getSourceAlign().valueOrOne().value()));
        }
    } else if (ID == Intrinsic::memset) {
        alignment = Align(std::max(memIntr->getDestAlign().valueOrOne(),
            MinimumAlignment));
    }

    return alignment;

}

bool AlignmentAnalysis::SetInstAlignment(MemSetInst& I)
{
    // Set the align attribute of the memset according to the detected
    // alignment of its operand.
    auto curAlign = I.getDestAlign();
    Align alignment = std::max(IGCLLVM::getDestAlign(I), getAlignValue(I.getRawDest()));
    I.setDestAlignment(alignment);
    return curAlign != alignment;

}

bool AlignmentAnalysis::SetInstAlignment(MemCpyInst& I)
{
    std::function<bool(Value*)> isConstGlobalZero = [&](Value* V)
    {
        if (auto* GEP = dyn_cast<GetElementPtrInst>(V))
            return isConstGlobalZero(GEP->getPointerOperand());

        if (auto* GV = dyn_cast<GlobalVariable>(V))
        {
            if (!GV->isConstant())
                return false;

            if (auto* initializer = GV->getInitializer())
                return initializer->isZeroValue();
        }

        return false;
    };

    MaybeAlign curAlign = I.getDestAlign();

    // If memcpy source is zeroinitialized constant global, memcpy is equivalent to memset to zero.
    // In this case, we can set the alignment of memcpy to the alignment of its destination only.
    if (isConstGlobalZero(I.getRawSource()))
    {
        Align alignment = std::max(IGCLLVM::getDestAlign(I), getAlignValue(I.getRawDest()));
        I.setDestAlignment(alignment);
        return curAlign != alignment;
    }

    // Set the align attribute of the memcpy based on the minimum alignment of its source and dest fields
    Align minRawAlignment = std::min(getAlignValue(I.getRawDest()), getAlignValue(I.getRawSource()));
    Align alignment = std::max(IGCLLVM::getDestAlign(I), minRawAlignment);
    I.setDestAlignment(alignment);
    return curAlign != alignment;
}

bool AlignmentAnalysis::SetInstAlignment(MemMoveInst& I)
{
    // Set the align attribute of the memmove based on the minimum alignment of its source and dest fields
    Align minRawAlignment = std::min(getAlignValue(I.getRawDest()), getAlignValue(I.getRawSource()));
    Align alignment = std::max(IGCLLVM::getDestAlign(I), minRawAlignment);
    MaybeAlign curAlign = I.getDestAlign();
    I.setDestAlignment(alignment);
    return curAlign != alignment;
}
