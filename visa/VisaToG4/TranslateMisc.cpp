/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BuildIR.h"

using namespace vISA;

bool IR_Builder::isNoMask(VISA_EMask_Ctrl eMask) {
    switch (eMask) {
    case vISA_EMASK_M1_NM:
    case vISA_EMASK_M2_NM:
    case vISA_EMASK_M3_NM:
    case vISA_EMASK_M4_NM:
    case vISA_EMASK_M5_NM:
    case vISA_EMASK_M6_NM:
    case vISA_EMASK_M7_NM:
    case vISA_EMASK_M8_NM:
        return true;
    default:
        return false;
    }
}

G4_ExecSize IR_Builder::toExecSize(VISA_Exec_Size execSize)
{
    switch (execSize) {
    case EXEC_SIZE_1: return g4::SIMD1;
    case EXEC_SIZE_2: return g4::SIMD2;
    case EXEC_SIZE_4: return g4::SIMD4;
    case EXEC_SIZE_8: return g4::SIMD8;
    case EXEC_SIZE_16: return g4::SIMD16;
    case EXEC_SIZE_32: return g4::SIMD32;
    default:
        MUST_BE_TRUE(false, "illegal common ISA execsize (should be 0..5).");
        return G4_ExecSize(0);
    }
}

// vector scatter messages are either SIMD8/16, so we have to round up
// the exec size
VISA_Exec_Size IR_Builder::roundUpExecSize(VISA_Exec_Size execSize)
{
    // for PVC legacy messages must be SIMD16
    if (getNativeExecSize() == g4::SIMD16)
    {
        return EXEC_SIZE_16;
    }
    if (execSize == EXEC_SIZE_1 || execSize == EXEC_SIZE_2 || execSize == EXEC_SIZE_4)
    {
        return EXEC_SIZE_8;
    }
    return execSize;
}

G4_Declare* IR_Builder::getImmDcl(G4_Imm* val, int numElt)
{
    auto dcl = immPool.addImmVal(val, numElt);
    if (dcl)
    {
        return dcl;
    }
    dcl = createTempVarWithNoSpill(numElt, val->getType(), Any);
    createMov(G4_ExecSize(numElt), createDstRegRegion(dcl, 1), val,
        InstOpt_WriteEnable, true);
    return dcl;
}




/// CopySrcToMsgPayload() performs a single batch of copy source into message
/// payload. If that single batch needs copy more than 2 GRFs, it will be split
/// into 2 parts recursively. That implies the a single batch copy MUST have
/// the size of power-of-2 multiple GRFs.
static void CopySrcToMsgPayload(
    IR_Builder *IRB,
    G4_ExecSize execSize, uint32_t eMask,
    G4_Declare *msg, unsigned msgRegOff,
    G4_SrcRegRegion *src, unsigned srcRegOff)
{
    uint32_t numRegs = (src->getElemSize() * execSize) / IRB->getGRFSize();
    if (numRegs == 0)
    {
        // always copy at least one GRF
        numRegs = 1;
    }

    ASSERT_USER((numRegs & (numRegs - 1)) == 0,
        "The batch size of a source message copy (i.e., native raw "
        "operand size) MUST be power-of-2 multiple of GRFs!");

    if (numRegs > 2) {
        // Copying of 2+ GRFs needs splitting. The splitting algorithm is
        // designed to be as general as possible to cover all possible valid
        // cases for message payload copying, i.e.,
        //
        // <32 x i32> -> 2 * <16 x i32>
        // <16 x i64> -> 2 * < 8 x i64>
        // <32 x i64> -> 2 * <16 x i64> -> 4 * < 8 x i64>
        //
        G4_ExecSize newExecSize {execSize / 2};
        unsigned splitOff = numRegs >> 1;
        uint32_t loEMask = IR_Builder::getSplitLoEMask(execSize, eMask);
        uint32_t hiEMask = IR_Builder::getSplitHiEMask(execSize, eMask);
        // Copy Lo
        CopySrcToMsgPayload(IRB, newExecSize, loEMask,
            msg, msgRegOff,
            src, srcRegOff);
        // Copy Hi
        CopySrcToMsgPayload(IRB, newExecSize, hiEMask,
            msg, msgRegOff + splitOff,
            src, srcRegOff + splitOff);
        return;
    }

    G4_DstRegRegion *dstRegion
        = IRB->createDst(msg->getRegVar(),
        (short)msgRegOff, 0, 1,
            src->getType());
    G4_SrcRegRegion *srcRegion
        = IRB->createSrcRegRegion(src->getModifier(),
            src->getRegAccess(),
            src->getBase(),
            src->getRegOff() + srcRegOff,
            src->getSubRegOff(),
            src->getRegion(),
            src->getType());
    IRB->createMov(execSize, dstRegion, srcRegion, eMask, true);
}

static void Copy_Source_To_Payload(
    IR_Builder *IRB, G4_ExecSize batchExSize,
    G4_Declare *msg, unsigned &regOff,
    G4_SrcRegRegion *source, G4_ExecSize execSize,
    uint32_t eMask)
{
    ASSERT_USER(batchExSize == 1 || batchExSize == 2 || batchExSize == 4 ||
        batchExSize == 8 || batchExSize == 16 || batchExSize == 32,
        "Invalid execution size for message payload copy!");

    unsigned srcRegOff = 0;
    G4_ExecSize batchSize = std::min(batchExSize, execSize);
    uint32_t numSrcRegs = (source->getElemSize() * batchSize) / IRB->getGRFSize();
    if (numSrcRegs == 0)
    {
        // always copy at least one GRF
        numSrcRegs = 1;
    }

    for (unsigned i = 0; i < execSize; i += batchSize) {
        if (!source->isNullReg()) {
            CopySrcToMsgPayload(IRB, batchSize, eMask,
                msg, regOff, source, srcRegOff);
        }
        regOff += numSrcRegs;
        srcRegOff += numSrcRegs;
    }
}

void IR_Builder::preparePayload(
    G4_SrcRegRegion *msgs[2],
    unsigned sizes[2],
    G4_ExecSize batchExSize,
    bool splitSendEnabled,
    PayloadSource srcs[], unsigned len)
{
    const G4_Declare *dcls[2] = {0, 0};
    unsigned msgSizes[2] = {0, 0};
    unsigned current = 0;
    unsigned offset = 0;
    unsigned splitPos = 0;

    // Loop through all source regions to check whether they forms one
    // consecutive regions or one/two consecutive regions if splitIndex is
    // non-zero.
    unsigned i;
    for (i = 0; i != len; ++i) {
        G4_SrcRegRegion *srcReg = srcs[i].opnd;

        if (srcReg->isNullReg()) {
            break;
        }

        const G4_Declare *srcDcl = getDeclare(srcReg);
        ASSERT_USER(srcDcl, "Declaration is missing!");

        unsigned regionSize = srcs[i].execSize * srcReg->getTypeSize();

        if (regionSize < getGRFSize()) {
            // FIXME: Need a better solution to decouple the value type from
            // the container type to generate better COPY if required.
            // round up to 1 GRF
            regionSize = getGRFSize();
        }

        if (srcDcl == dcls[current]) {
            unsigned srcOff = getByteOffsetSrcRegion(srcReg);
            // Check offset if they have the same declaration.
            if (offset == srcOff) {
                // Advance offset to next expected one.
                offset += regionSize;
                msgSizes[current] += regionSize;
                continue;
            }
            // Check whether there are overlaps if split-send is enabled.
            if (splitSendEnabled && current == 0 && srcOff < offset) {
                // The source overlaps with the previous sources prepared.
                // Force to copy all sources from the this source for the 2nd
                // part in the split message.
                ++current;

                ASSERT_USER(i > 0, "Split position MUST NOT be at index 0!");
                splitPos = i;
                break;
            }
        }

        if (dcls[current] == 0) {
            // First time checking the current region.
            offset = getByteOffsetSrcRegion(srcReg);
            offset += regionSize;
            msgSizes[current] += regionSize;
            dcls[current] = srcDcl;
            continue;
        }

        // Bail out if more than 1 consecutive regions are needed but
        // split-send is not enabled.
        if (!splitSendEnabled)
            break;

        // Bail out if more than 2 consecutive regions will be needed.
        if (current != 0)
            break;

        // Check one more consecutive regions.
        ++current;

        ASSERT_USER(i > 0, "Split position MUST NOT be at index 0!");

        // Record the 2nd consecutive region.
        splitPos = i;
        offset = getByteOffsetSrcRegion(srcReg);
        offset += regionSize;
        msgSizes[current] += regionSize;
        dcls[current] = srcDcl;
    }

    if (i == len) {
        // All sources are checked and they are fit into one or two consecutive
        // regions.
        msgs[0] = srcs[0].opnd;
        msgs[1] = (splitPos == 0) ? 0 : srcs[splitPos].opnd;
        sizes[0] = msgSizes[0] / numEltPerGRF<Type_UB>();
        sizes[1] = msgSizes[1] / numEltPerGRF<Type_UB>();

        return;
    }

    // Count remaining message size.
    for (; i != len; ++i) {
        G4_SrcRegRegion *srcReg = srcs[i].opnd;
        unsigned regionSize = srcs[i].execSize * srcReg->getTypeSize();
        if (regionSize < getGRFSize()) {
            // FIXME: Need a better solution to decouple the value type from
            // the container type to generate better COPY if required.
            // round up to 1 GRF
            regionSize = getGRFSize();
        }
        msgSizes[current] += regionSize;
    }

    // Allocate a new large enough GPR to copy in the payload.
    G4_Declare *msg =
        createSendPayloadDcl(msgSizes[current]/TypeSize(Type_UD), Type_UD);

    // Copy sources.
    unsigned regOff = 0;
    for (i = splitPos; i != len; ++i)
    {
        Copy_Source_To_Payload(this, batchExSize, msg, regOff, srcs[i].opnd,
            srcs[i].execSize, srcs[i].instOpt);
    }

    i = 0;
    if (current > 0) {
        msgs[i] = srcs[0].opnd;
        sizes[i] = msgSizes[0] / numEltPerGRF<Type_UB>();
        ++i;
    }
    msgs[i] = createSrcRegRegion(msg, getRegionStride1());
    sizes[i] = msgSizes[current] / numEltPerGRF<Type_UB>();
}

G4_SrcRegRegion *IR_Builder::coalescePayload(
    unsigned sourceAlignment,
    unsigned payloadAlignment,
    uint32_t payloadWidth,   // number of elements for one payload in the send.
    uint32_t srcSize,       // number of elements provided by src
    std::initializer_list<G4_SrcRegRegion *> srcs,
    VISA_EMask_Ctrl emask)
{
    MUST_BE_TRUE(sourceAlignment != 0 && payloadAlignment != 0,
        "alignment mustn't be 0");
    MUST_BE_TRUE(payloadAlignment % 4 == 0, // we could relax this with smarter code below
        "result alignment must be multiple of 4");
    MUST_BE_TRUE(srcs.size() > 0,"empty initializer list");

    // First check for trivial cases.  If all are null, then we can
    // return null.  This is the case for operations like load's src1 and
    // atomics with no argument (e.g. atomic increment).
    //
    // If the first src is the only non-null register and it's alignment fits
    // then we can just return that register and call it a day.  This is the
    // common case for things like stores or atomics with a single
    // data parameter (e.g. atomic add).
    bool allNull = true;
    bool onlySrc0NonNull = true;
    int ix = 0;
    for (G4_SrcRegRegion *src : srcs) {
        allNull &= src->isNullReg();
        onlySrc0NonNull &= ix++ == 0 || src->isNullReg();
    }
    G4_SrcRegRegion *src0 = *srcs.begin();
    if (allNull) {
        return src0;
    } else if (onlySrc0NonNull) {
        const G4_Declare *src0Dcl = getDeclare(src0);
        MUST_BE_TRUE(src0Dcl, "declaration missing");
        unsigned src0Size = src0Dcl->getTotalElems()*src0Dcl->getElemSize();
        if (src0Size % sourceAlignment == 0 &&
            src0Size % payloadAlignment == 0)
        {
            return src0;
        }
    }

    // Otherwise, we have to do some copying
    auto alignTo = [] (size_t a, size_t n) {
        return (n + a - 1) - ((n + a - 1)%a);
    };

    int numPayloadGRF =  0;
    // precompute the necessary region size
    for (G4_SrcRegRegion *src : srcs) {
        if (src && !src->isNullReg())
        {
            // ToDo: add D16 support later
            auto laneSize = src->getTypeSize() == 8 ? 8 : 4;
            numPayloadGRF += std::max(1u, (payloadWidth * laneSize) / getGRFSize());
        }
    }

    G4_Declare *payloadDeclUD = createSendPayloadDcl(numPayloadGRF * getGRFSize() / 4, Type_UD);
    payloadDeclUD->setEvenAlign();

    unsigned row = 0;
    for (G4_SrcRegRegion *src : srcs) {
        if (src && !src->isNullReg()) {

            // ToDo: add D16 support later
            auto laneSize = src->getTypeSize() == 8 ? 8 : 4;
            auto totalSize = srcSize * laneSize;

            // for each payload we copy <srcSize> lanes to its corresponding location in payload
            // src must be GRF-aligned per vISA spec requirement
            // Two moves may be necessary for 64-bit types
            auto copyRegion =
                [&] (G4_Type type) {
                uint32_t numMoves = std::max(1u, totalSize / (2 * getGRFSize()));
                auto moveMask = emask;
                G4_ExecSize MAX_SIMD {std::min(srcSize, getNativeExecSize() * (laneSize == 8 ? 1u : 2u))};
                for (unsigned i = 0; i < numMoves; i++) {
                    auto rowOffset = i * 2;
                    unsigned int instOpt = Get_Gen4_Emask(moveMask, MAX_SIMD);
                    G4_DstRegRegion* dstRegion =
                        createDst(
                            payloadDeclUD->getRegVar(),
                            row + rowOffset, 0,
                            1, type);
                    G4_SrcRegRegion* srcRegion =
                        createSrc(
                            src->getTopDcl()->getRegVar(), src->getRegOff() + rowOffset, 0,
                            getRegionStride1(),
                            type);
                    createMov(MAX_SIMD,
                        dstRegion, srcRegion, instOpt, true);
                    moveMask = Get_Next_EMask(moveMask, MAX_SIMD);
                }
            };

            copyRegion(src->getType());

            // advance the payload offset by <payloadWidth> elements
            row += std::max(1u, (payloadWidth * laneSize) / getGRFSize());
        }
    }

    return createSrcRegRegion(payloadDeclUD, getRegionStride1());
}


void IR_Builder::Copy_SrcRegRegion_To_Payload(
    G4_Declare* payload, unsigned int& regOff, G4_SrcRegRegion* src,
    G4_ExecSize execSize, uint32_t emask)
{
    auto payloadDstRgn = createDst(payload->getRegVar(), (short)regOff, 0, 1, payload->getElemType());

    G4_SrcRegRegion* srcRgn = createSrcRegRegion(*src);
    srcRgn->setType(*this, payload->getElemType());
    createMov(execSize, payloadDstRgn, srcRgn, emask, true);
    if (TypeSize(payload->getElemType()) == 2)
    {
        // for half float each source occupies 1 GRF regardless of execution size
        regOff++;
    }
    else
    {
        regOff += execSize / getNativeExecSize();
    }
}

unsigned int IR_Builder::getByteOffsetSrcRegion(G4_SrcRegRegion* srcRegion)
{
    unsigned int offset =
        (srcRegion->getRegOff() * numEltPerGRF<Type_UB>()) +
        (srcRegion->getSubRegOff() * srcRegion->getTypeSize());

    if (srcRegion->getBase() &&
        srcRegion->getBase()->isRegVar())
    {
        G4_Declare* dcl = srcRegion->getBase()->asRegVar()->getDeclare();

        if (dcl != NULL)
        {
            while (dcl->getAliasDeclare() != NULL)
            {
                offset += dcl->getAliasOffset();
                dcl = dcl->getAliasDeclare();
            }
        }
    }

    return offset;
}

bool IR_Builder::checkIfRegionsAreConsecutive(
    G4_SrcRegRegion* first, G4_SrcRegRegion* second, G4_ExecSize execSize)
{
    if (first == NULL || second == NULL)
    {
        return true;
    }

    return checkIfRegionsAreConsecutive(first, second, execSize, first->getType());
}

bool IR_Builder::checkIfRegionsAreConsecutive(
    G4_SrcRegRegion* first, G4_SrcRegRegion* second, G4_ExecSize execSize, G4_Type type)
{
    bool isConsecutive = false;

    if (first == NULL || second == NULL)
    {
        isConsecutive = true;
    }
    else
    {
        G4_Declare* firstDcl = getDeclare(first);
        G4_Declare* secondDcl = getDeclare(second);

        unsigned int firstOff = getByteOffsetSrcRegion(first);
        unsigned int secondOff = getByteOffsetSrcRegion(second);

        if (firstDcl == secondDcl)
        {
            if (firstOff + execSize * TypeSize(type) == secondOff)
            {
                isConsecutive = true;
            }
        }
    }

    return isConsecutive;
}

int IR_Builder::generateDebugInfoPlaceholder()
{
    debugInfoPlaceholder = curCISAOffset;
    return VISA_SUCCESS;
}


int IR_Builder::translateVISALifetimeInst(uint8_t properties, G4_Operand* var)
{
    // Lifetime.start/end are two variants of this instruction
    createImm(properties & 0x1, Type_UB);

    if ((properties & 0x1) == LIFETIME_START)
    {
        G4_DstRegRegion* varDstRgn = createDst(var->getBase(), 0, 0, 1, Type_UD);
        createIntrinsicInst(
            nullptr, Intrinsic::PseudoKill, g4::SIMD1,
            varDstRgn, createImm((unsigned int)PseudoKillType::Src),
            nullptr, nullptr, InstOpt_WriteEnable, true);
    }
    else
    {
        G4_SrcRegRegion* varSrcRgn = createSrc(var->getBase(), 0, 0, getRegionScalar(), Type_UD);
        createIntrinsicInst(nullptr, Intrinsic::PseudoUse, g4::SIMD1, nullptr, varSrcRgn,
            nullptr, nullptr, InstOpt_WriteEnable, true);
    }

    // We dont treat lifetime.end specially for now because lifetime.start
    // is expected to halt propagation of liveness upwards. lifetime.start
    // would prevent loop local variables/sub-rooutine local variables
    // from being live across entire loop/sub-routine.

    return VISA_SUCCESS;
}
