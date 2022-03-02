/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MergeScalars.hpp"

#include <vector>

using namespace vISA;

// This will create a new dcl or reuse the existing one if available.
// The returned dcl will have
//    its Reg's byte address == input's
//    its type == srcType
//    itss numElem == bundleSize.
static G4_Declare* getInputDeclare(
    IR_Builder& builder,
    std::vector<G4_Declare*>& declares,
    G4_Declare* input,
    G4_Type eltType,
    int bundleSize,
    int firstEltOffset)
{
    MUST_BE_TRUE(input->isInput() && input->getAliasDeclare() == NULL, "Expect root input variable");
    for (auto dcl : declares)
    {
        MUST_BE_TRUE(dcl->isInput() && dcl->getAliasDeclare() == NULL, "declare must be root input variable");
        if (dcl->getRegVar()->getByteAddr(builder) == input->getRegVar()->getByteAddr(builder) &&
            dcl->getElemType() == eltType &&
            dcl->getTotalElems() >= bundleSize)
        {
            return dcl;
        }
    }

    // Now, create a new dcl.
    // As the new dcl's offset is in unit of element type, we will convert offset in bytes to
    // offset in elementType.  As the previous check guarantees that the byte offset must be
    // multiple of elementType, here just add additional checks to make sure this is the case.
    uint32_t offset = input->getRegVar()->getPhyRegOff() * input->getElemSize() + firstEltOffset;
    uint32_t eltBytes = TypeSize(eltType);
    MUST_BE_TRUE((offset % eltBytes) == 0, "Offset shoule be mutiple of element size");
    offset = offset / eltBytes;
    const char* name = builder.getNameString(builder.mem, 16, "InputR%d.%d", input->getRegVar()->getPhyReg()->asGreg()->getRegNum(), offset);
    G4_Declare* newInputDcl = builder.createDeclareNoLookup(name, G4_INPUT, (uint16_t)bundleSize, 1,
        eltType);
    newInputDcl->getRegVar()->setPhyReg(input->getRegVar()->getPhyReg(), offset);
    declares.push_back(newInputDcl);
    return newInputDcl;
}

//#define DEBUG_VERBOSE_ON
//
// merge all instructions in the bundle into a single one, by modifying the first instruction
// in the bundle and delete the rest
// returns true if merge is successful
// merge can fail if a source/dst appears in multiple bundles and has a conflicing alignment
// (this only happens when the operand is in DISJOINT pattern)
//
bool BUNDLE_INFO::doMerge(IR_Builder& builder,
    std::unordered_set<G4_Declare*>& modifiedDcl,
    std::vector<G4_Declare*>& newInputs)
{
    if (size == 1)
    {
        return false;
    }

    // Round down esize to power of 2 to make sure it wouldn't be out of bound.
    int roundDownPow2Size = (int)Round_Down_Pow2(size);
    while (size > roundDownPow2Size)
    {
        deleteLastInst();
    }

    if (!builder.hasAlign1Ternary() && size == 2 && inst[0]->getNumSrc() == 3)
    {
        return false;
    }

    // for distinct patterns (both src and dst), we need to check the variable is only used once in the entire bundle,
    // so that the new variable created for the operand does not conflict with another.
    // this is to prevent coalescing two variables with conflicting life range, e.g.,
    //  add (M1_NM, 1) V66(0,0)<1> V66(0,0)<0;1,0> 0x1:d                             /// $36
    //  add (M1_NM, 1) V67(0,0)<1> V65(0,0)<0;1,0> 0x1:d                             /// $37
    //  or make a variable with two root aliases:
    //  mul (M1_NM, 1) V186(0,0)<1> V184(0,0)<0;1,0> V185(0,0)<0; 1, 0>                /// $206
    //  mul (M1_NM, 1) V187(0,0)<1> V182(0,0)<0;1,0> V182(0,0)<0; 1, 0>                /// $207
    std::set<G4_Declare*> uniqueDeclares;

    // check if merging is legal
    if (dstPattern == OPND_PATTERN::DISJOINT)
    {
        G4_Declare* rootDcl = inst[0]->getDst()->getTopDcl()->getAliasDeclare();
        for (int instId = 0; instId < size; ++instId)
        {
            G4_DstRegRegion* dst = inst[instId]->getDst();
            G4_Declare* dcl = dst->getTopDcl();
            if (dcl->getAliasDeclare() != rootDcl)
            {
                // all dsts in the bundle should have the same base variable (or NULL)
                // this is to avoid the situation where {V1, V2} is in one bundle and {V1, V3}
                // is in another bundle, and we can't optimize both since V2 and V3 may be both live
                return false;
            }
            if (dcl->getAliasDeclare() != NULL)
            {
                if (dcl->getAliasOffset() != dst->getTypeSize() * instId)
                {
                    return false;
                }
            }
            uniqueDeclares.insert(dcl);
        }
    }
    for (int i = 0; i < inst[0]->getNumSrc(); ++i)
    {
        if (srcPattern[i] == OPND_PATTERN::DISJOINT)
        {
            G4_Declare* rootDcl = inst[0]->getSrc(i)->getTopDcl()->getAliasDeclare();
            for (int instId = 0; instId < size; ++instId)
            {
                G4_Operand* src = inst[instId]->getSrc(i);
                G4_Declare* dcl = src->getTopDcl();
                if (dcl->getAliasDeclare() != rootDcl)
                {
                    // all srcs in the bundle, if aliased, should have the same alias variable
                    // see comments from above for dst
                    return false;
                }
                if (dcl->getAliasDeclare() != NULL)
                {
                    if (dcl->getAliasOffset() != src->getTypeSize() * instId)
                    {
                        return false;
                    }
                }
                if (uniqueDeclares.find(dcl) != uniqueDeclares.end())
                {
                    return false;
                }
                uniqueDeclares.insert(dcl);
            }
        }
    }

    G4_ExecSize execSize = (G4_ExecSize)size;
    G4_INST* newInst = inst[0]; //reuse inst[0] as the new merged inst

    // at this point merge will definitely succeed
    // handle merged dst
    if (dstPattern == OPND_PATTERN::DISJOINT)
    {
        G4_Type dstType = newInst->getDst()->getType();
        // create a new declare with bundle size and have the original dsts alias to it
        G4_Declare* newDcl = NULL;
        if (newInst->getDst()->getTopDcl()->getAliasDeclare() != NULL)
        {
            newDcl = newInst->getDst()->getTopDcl()->getAliasDeclare();
        }
        else
        {
            newDcl = builder.createTempVar(execSize, dstType, Eight_Word, "Merged");
        }
        for (int i = 0; i < size; ++i)
        {
            G4_Declare* dstDcl = inst[i]->getDst()->getTopDcl();
            if (dstDcl->getAliasDeclare() == NULL)
            {
                dstDcl->setAliasDeclare(newDcl, i * TypeSize(dstType));
                modifiedDcl.insert(dstDcl);
#ifdef DEBUG_VERBOSE_ON
                cout << "Dcl " << dstDcl->getName() << "--> (" << newDcl->getName() << ", "
                    << i * TypeSize(dstType) << ")\n";
#endif
            }
        }
        G4_DstRegRegion* newDst = builder.createDst(newDcl->getRegVar(), 0, 0, 1, dstType);
        newInst->setDest(newDst);
    }
    else
    {
        MUST_BE_TRUE(dstPattern == OPND_PATTERN::CONTIGUOUS, "unexpected dst pattern");
    }

    for (int i = 0; i < newInst->getNumSrc(); ++i)
    {
        if (srcPattern[i] == OPND_PATTERN::DISJOINT)
        {
            G4_Operand* oldSrc = newInst->getSrc(i);
            G4_Type srcType = oldSrc->getType();
            // create a new declare with bundle size and have the original dsts alias to it
            G4_Declare* newDcl = NULL;
            if (oldSrc->getTopDcl()->getAliasDeclare() != NULL)
            {
                newDcl = oldSrc->getTopDcl()->getAliasDeclare();
            }
            else
            {
                newDcl = builder.createTempVar(execSize, srcType, Eight_Word, "Merged");
            }
            for (int j = 0; j < size; ++j)
            {
                MUST_BE_TRUE(inst[j]->getSrc(i)->isSrcRegRegion(), "Src must be a region");
                G4_SrcRegRegion* src = inst[j]->getSrc(i)->asSrcRegRegion();
                G4_Declare* srcDcl = src->getTopDcl();
                if (srcDcl->getAliasDeclare() == NULL)
                {
                    srcDcl->setAliasDeclare(newDcl, j * TypeSize(srcType));
                    modifiedDcl.insert(srcDcl);
#ifdef DEBUG_VERBOSE_ON
                    cout << "Dcl " << srcDcl->getName() << "--> (" << newDcl->getName() << ", "
                        << i * TypeSize(srcType) << ")\n";
#endif
                }
            }
            G4_SrcRegRegion* newSrc = builder.createSrcRegRegion(
                oldSrc->asSrcRegRegion()->getModifier(), Direct,
                newDcl->getRegVar(), 0, 0, builder.getRegionStride1(), srcType);
            newInst->setSrc(newSrc, i);
        }
        else if (srcPattern[i] == OPND_PATTERN::CONTIGUOUS)
        {
            // update region
            G4_SrcRegRegion* src = newInst->getSrc(i)->asSrcRegRegion();
            G4_Declare* rootDcl = src->getTopDcl()->getRootDeclare();
            if (rootDcl->isInput())
            {
                //check if the existing input is good enough
                bool sameInput = true;
                for (int instId = 1; instId < size; ++instId)
                {
                    auto dcl = inst[instId]->getSrc(i)->getTopDcl()->getRootDeclare();
                    if (rootDcl != dcl)
                    {
                        sameInput = false;
                        break;
                    }
                }

                if (sameInput)
                {
                    src->setRegion(builder, builder.getRegionStride1());
                }
                else
                {
                    // we need a new input variable that covers all inputs used in the bundle
                    // Since rootDcl may have a different type than the operands', need to use
                    // operand's type instead of rootDcl, so we pass srcType into getInputDeclare().
                    G4_Type srcType = newInst->getSrc(i)->getType();
                    int firstEltOffset =
                        src->getRegOff() * builder.numEltPerGRF<Type_UB>() +
                            src->getSubRegOff() * TypeSize(srcType);
                    G4_Declare* newInputDcl = getInputDeclare(builder, newInputs, rootDcl, srcType, size, firstEltOffset);
                    src = builder.createSrcRegRegion(
                        src->getModifier(), Direct, newInputDcl->getRegVar(),
                        0, 0, builder.getRegionStride1(), src->getType());
                    newInst->setSrc(src, i);
                }
            }
            else
            {
                src->setRegion(builder, builder.getRegionStride1());
            }
        }
        else
        {
            MUST_BE_TRUE(srcPattern[i] == OPND_PATTERN::IDENTICAL, "unexpected source pattern");
        }
    }

    newInst->setExecSize(execSize);

    auto iter = startIter;
    ++iter;
    for (int i = 1; i < size; ++i)
    {
        G4_INST* instToDelete = *iter;
        instToDelete->transferUse(newInst, true);
        for (int srcNum = 0, numSrc = instToDelete->getNumSrc(); srcNum < numSrc; ++srcNum)
        {
            Gen4_Operand_Number opndPos = instToDelete->getSrcOperandNum(srcNum);
            instToDelete->transferDef(newInst, opndPos, opndPos);
        }

        iter = bb->erase(iter);
    }

#ifdef DEBUG_VERBOSE_ON
    cout << "new inst after merging:\n";
    newInst->emit(cout);
    cout << "\n";
    newInst->emitDefUse(cout);
    cout << "\n";
#endif

    return true;
}


//
// check to see if offset1 + type size == offset2
// if so and pattern is either unknown or contiguous, return true
// if pattern is unknown also change it to contiguous
//
// also check to see if distance between offset2 and origOffset
// (offset of the first instruction found in current bundle)
// exceeds GRF size. If so returns false.
//
//
static bool checkContiguous(
    unsigned offset1, unsigned offset2,
    G4_Type type, OPND_PATTERN& pattern, unsigned origOffset,
    const IR_Builder& builder)
{
    if (origOffset + builder.numEltPerGRF<Type_UB>() <= offset2)
        return false;
    if (offset1 + TypeSize(type) == offset2)
    {
        switch (pattern)
        {
        case OPND_PATTERN::UNKNOWN:
            pattern = OPND_PATTERN::CONTIGUOUS;
            return true;
        case OPND_PATTERN::CONTIGUOUS:
            return true;
        default:
            return false;
        }
    }
    return false;
}

// check if this instruction is eligbile to be merged into a vector instruction
// the conditions are:
// -- arithmetic, logic, mov, math, or pseudo_mad instructions
// -- simd1, NoMask, no predicates or conditional modifier
// -- dst must be direct GRF whose declare has no alias
// -- all sources must be either direct GRF or immediates
bool BUNDLE_INFO::isMergeCandidate(G4_INST* inst, const IR_Builder& builder, bool isInSimdFlow)
{
    // Don't merge mixed mode instructions as SIMD2/4 mixed mode instructions with packed HF is not allowed by HW.
    if (inst->isMixedMode())
        return false;

    if (!inst->isArithmetic() && !inst->isLogic() && !inst->isMath() && !inst->isMov() &&
        inst->opcode() != G4_pseudo_mad)
    {
        return false;
    }

    if (inst->getExecSize() != g4::SIMD1 ||
        (!inst->isWriteEnableInst() && isInSimdFlow))
    {
        return false;
    }

    if (inst->getPredicate() || inst->getCondMod())
    {
        return false;
    }

    MUST_BE_TRUE(inst->getDst() != NULL, "dst must not be NULL");
    if (inst->getDst()->isIndirect())
    {
        return false;
    }

    if (builder.no64bitRegioning() && inst->getDst()->getTypeSize() == 8)
    {
        // applying merge scalar makes code quality worse since we need to insert moves
        // to compensate for the lack of 64-bit regions later
        return false;
    }

    G4_VarBase* dstBase = inst->getDst()->getBase();
    G4_Declare* dstDcl = dstBase->isRegVar() ? dstBase->asRegVar()->getDeclare() : nullptr;
    if (dstDcl != nullptr &&
        (dstDcl->getAliasDeclare() != nullptr ||
            inst->getDst()->getTypeSize() != dstDcl->getElemSize() ||
            !dstDcl->useGRF()))
    {
        return false;
    }

    if (dstDcl != nullptr && (dstDcl->isOutput() || dstDcl->isInput()))
    {
        return false;
    }

    for (int i = 0; i < inst->getNumSrc(); ++i)
    {
        G4_Operand* src = inst->getSrc(i);
        if (src->isSrcRegRegion())
        {
            if (src->asSrcRegRegion()->isIndirect())
            {
                return false;
            }
            G4_Declare* srcDcl = src->getTopDcl();
            if (srcDcl == nullptr)
            {
                return false;
            }

            if (!srcDcl->useGRF() /* || srcDcl->getTotalElems() != 1 */)
            {
                return false;
            }
            // can't do opt if source decl type is inconsistent with its use
            if (TypeSize(srcDcl->getElemType()) != src->getTypeSize())
            {
                return false;
            }
        }
        else if (src->isImm() && !src->isRelocImm())
        {
            // ok
        }
        else
        {
            return false;
        }
    }

    return true;
}

// returns true if dcl is a scalar root variable that is naturally aligned
// Note that this also rules out input
static bool isScalarNaturalAlignedVar(G4_Declare* dcl)
{
    return dcl->getTotalElems() == 1 && dcl->getAliasDeclare() == NULL &&
        dcl->getByteAlignment() == TypeSize(dcl->getElemType()) && !dcl->isInput();
}

//
// Check if the dst can be combined into the bundle
// There are 2 scenarios:
// CONTIGUOUS: the dsts together form a contiguous region
// DISJOINT: all dsts are distinct scalar, naturally aligned root variables
//
bool BUNDLE_INFO::canMergeDst(G4_DstRegRegion* dst, const IR_Builder& builder)
{
    // src must be either Imm or SrcRegRegion

    G4_DstRegRegion* firstDst = inst[0]->getDst();
    G4_DstRegRegion* prevDst = inst[size - 1]->getDst();
    if (prevDst->getType() != dst->getType())
    {
        return false;
    }

    G4_Declare* prevDstDcl = prevDst->getTopDcl();
    G4_Declare* dstDcl = dst->getTopDcl();

    if ((dstDcl == nullptr) || (prevDstDcl == nullptr))
    {
        return false;
    }

    if (prevDstDcl == dstDcl)
    {
        if (dstPattern == OPND_PATTERN::DISJOINT)
        {
            return false;
        }

        if (!checkContiguous(prevDst->getLeftBound(), dst->getLeftBound(),
            dst->getType(), dstPattern, firstDst->getLeftBound(), builder))
        {
            return false;
        }
    }
    else if (prevDstDcl->isInput() && dstDcl->isInput())
    {
        unsigned firstDstGRFOffset = firstDst->getLeftBound() + firstDst->getTopDcl()->getRegVar()->getByteAddr(builder);
        unsigned prevDstGRFOffset = prevDst->getLeftBound() + prevDstDcl->getRegVar()->getByteAddr(builder);
        unsigned dstGRFOffset = dst->getLeftBound() + dstDcl->getRegVar()->getByteAddr(builder);
        if (!checkContiguous(prevDstGRFOffset, dstGRFOffset, dst->getType(), dstPattern, firstDstGRFOffset, builder))
        {
            return false;
        }
    }
    else
    {
        switch (dstPattern)
        {
        case OPND_PATTERN::UNKNOWN:
            //allow if both sources are size 1 root variables with no alignment
            if (isScalarNaturalAlignedVar(prevDstDcl) && isScalarNaturalAlignedVar(dstDcl))
            {
                dstPattern = OPND_PATTERN::DISJOINT;
            }
            else
            {
                return false;
            }
            break;
        case OPND_PATTERN::DISJOINT:
            if (!isScalarNaturalAlignedVar(dstDcl))
            {
                return false;
            }
            //also check to see if dst is the same as any other previous dst
            for (int i = 0; i < size - 1; i++)
            {
                G4_INST* bundleInst = inst[i];
                if (dstDcl == bundleInst->getDst()->getTopDcl())
                {
                    return false;
                }
            }
            break;
        default:
            return false;
        }
    }

    // additionally check if dst has a WAR dependence with one of the source in earlier instructions
    // it may seem that WAR dependencies should not interfere with vectorization since src is always read
    // before the dst; however, the problem is that if src and dst in different instructions refer to
    // the same variable we can't get them to align properly.
    for (int i = 0; i < size; i++)
    {
        for (int srcPos = 0; srcPos < inst[i]->getNumSrc(); ++srcPos)
        {
            G4_Operand* src = inst[i]->getSrc(srcPos);
            // since both are scalar, check is as simple as comparing the dcl
            if (src->getTopDcl() != NULL && src->getTopDcl() == dst->getTopDcl())
            {
                return false;
            }
        }
    }

    return true;
}

//
// Check if this src at srcPos can be combined into the bundle
// There are 3 scenarios:
// IDENTICAL: all sources in the bundle are the same scalar variable
// CONTIGUOUS: the sources together form a contiguous region
// DISJOINT: all sources are distinct scalar, naturally aligned root variables
//
bool BUNDLE_INFO::canMergeSource(G4_Operand* src, int srcPos, const IR_Builder& builder)
{
    // src must be either Imm or SrcRegRegion

    MUST_BE_TRUE(srcPos < maxNumSrc, "invalid srcPos");

    if (src->isRelocImm())
    {
        return false;
    }

    if ((inst[0]->isMath() && inst[0]->asMathInst()->isOneSrcMath()) && srcPos == 1)
    {
        // null source is always legal
        srcPattern[srcPos] = OPND_PATTERN::IDENTICAL;
        return true;
    }

    G4_Operand* firstSrc = inst[0]->getSrc(srcPos);
    G4_Operand* prevSrc = inst[size - 1]->getSrc(srcPos);
    if (prevSrc->getType() != src->getType())
    {
        return false;
    }
    if (prevSrc->isImm())
    {
        // ToDo: can add packed vector support later
        if (!src->isImm() ||
            prevSrc->asImm()->getImm() != src->asImm()->getImm())
        {
            return false;
        }
        srcPattern[srcPos] = OPND_PATTERN::IDENTICAL;
    }
    else
    {
        if (!src->isSrcRegRegion())
        {
            return false;
        }
        G4_SrcRegRegion* prevSrcRegion = prevSrc->asSrcRegRegion();
        G4_SrcRegRegion* srcRegion = src->asSrcRegRegion();
        if (prevSrcRegion->getModifier() != srcRegion->getModifier())
        {
            return false;
        }

        G4_Declare* prevSrcDcl = prevSrc->getTopDcl();
        G4_Declare* srcDcl = src->getTopDcl();
        if (prevSrcDcl == srcDcl)
        {
            if (srcPattern[srcPos] == OPND_PATTERN::DISJOINT)
            {
                return false;
            }

            if (prevSrc->getLeftBound() == src->getLeftBound())
            {
                // the case where we have identical source for each instruction in the bundle
                switch (srcPattern[srcPos])
                {
                case OPND_PATTERN::UNKNOWN:
                    srcPattern[srcPos] = OPND_PATTERN::IDENTICAL;
                    break;
                case OPND_PATTERN::IDENTICAL:
                    // do nothing
                    break;
                default:
                    return false;
                }
            }
            else if (!checkContiguous(prevSrc->getLeftBound(), src->getLeftBound(),
                src->getType(), srcPattern[srcPos], firstSrc->getLeftBound(), builder))
            {
                return false;
            }
        }
        else if (prevSrcDcl->isInput() && srcDcl->isInput())
        {
            unsigned firstSrcGRFOffset = firstSrc->getLeftBound() + firstSrc->getTopDcl()->getRegVar()->getByteAddr(builder);
            unsigned prevSrcGRFOffset = prevSrc->getLeftBound() + prevSrcDcl->getRegVar()->getByteAddr(builder);
            unsigned srcGRFOffset = src->getLeftBound() + srcDcl->getRegVar()->getByteAddr(builder);
            if (!checkContiguous(prevSrcGRFOffset, srcGRFOffset, src->getType(), srcPattern[srcPos], firstSrcGRFOffset, builder))
            {
                return false;
            }
            else if (prevSrcGRFOffset / builder.numEltPerGRF<Type_UB>() != srcGRFOffset / 32)
            {
                // resulting input would cross GRF boundary, and our RA does not like it one bit
                return false;
            }

            if (inst[0]->getNumSrc() == 3)
            {
                // for 3src inst, we can't merge if inst0's src is not oword-aligned
                if ((prevSrcGRFOffset & 0xF) != 0)
                {
                    return false;
                }
            }
        }
        else
        {
            switch (srcPattern[srcPos])
            {
            case OPND_PATTERN::UNKNOWN:
                //allow if both sources are size 1 root variables with no alignment
                if (isScalarNaturalAlignedVar(prevSrcDcl) && isScalarNaturalAlignedVar(srcDcl))
                {
                    srcPattern[srcPos] = OPND_PATTERN::DISJOINT;
                }
                else
                {
                    return false;
                }
                break;
            case OPND_PATTERN::DISJOINT:
                if (!isScalarNaturalAlignedVar(srcDcl))
                {
                    return false;
                }
                //also check to see if src is the same as any other previous sources
                for (int i = 0; i < size - 1; i++)
                {
                    G4_INST* bundleInst = inst[i];
                    if (srcDcl == bundleInst->getSrc(srcPos)->getTopDcl())
                    {
                        return false;
                    }
                }
                break;
            default:
                return false;
            }
        }
    }

    if (src->isSrcRegRegion())
    {
        // we additionally have to check if src has a RAW dependency with one of the
        // previous dst in the bundle
        for (int i = 0; i < size; i++)
        {
            G4_DstRegRegion* dst = inst[i]->getDst();
            // since both are scalar, check is as simple as comparing the dcl
            if (src->getTopDcl() == dst->getTopDcl())
            {
                return false;
            }
        }
    }

    return true;
}

//
// check if inst can be successfully appended to the bundle
// For this to be successful:
// -- inst must itself be a merge candidate (checked by caller)
// -- inst must have same opcode/dst modifier/src modifier as all other instructions in the bundle
// -- dst and src operand for the inst must form one of the legal patterns with the instructions in the bundle
//
bool BUNDLE_INFO::canMerge(G4_INST* inst, const IR_Builder& builder)
{
    G4_INST* firstInst = this->inst[0];
    if (firstInst->opcode() != inst->opcode())
    {
        return false;
    }
    if (firstInst->isBfn() && firstInst->asBfnInst()->getBooleanFuncCtrl() != inst->asBfnInst()->getBooleanFuncCtrl())
    {
        return false;
    }

    if (inst->isMath())
    {
        G4_MathOp firstMathOp = MATH_RESERVED;
        if (firstInst->isMath())
        {
            firstMathOp = firstInst->asMathInst()->getMathCtrl();
        }

        if (firstMathOp != inst->asMathInst()->getMathCtrl())
        {
            return false;
        }
    }

    if (firstInst->getSaturate() != inst->getSaturate())
    {
        return false;
    }

    if (!canMergeDst(inst->getDst(), builder))
    {
        return false;
    }


    for (int i = 0; i < inst->getNumSrc(); ++i)
    {
        if (!canMergeSource(inst->getSrc(i), i, builder))
        {
            return false;
        }
    }

    // append instruction to bundle
    appendInst(inst);
    return true;

}

//
// iter is advanced to the next instruction not belonging to the handle
//
void BUNDLE_INFO::findInstructionToMerge(
    INST_LIST_ITER& iter, const IR_Builder& builder)
{

    for (; iter != bb->end() && this->size < this->sizeLimit; ++iter)
    {
        G4_INST* nextInst = *iter;
        if (!BUNDLE_INFO::isMergeCandidate(nextInst, builder, !bb->isAllLaneActive()))
        {
            break;
        }

        if (!canMerge(nextInst, builder))
        {
            break;
        }
    }
}
