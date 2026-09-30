/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PostRACopyProp.hpp"

#include <algorithm>

using namespace vISA;

// Cap on in-flight copies per BB to bound compile time.
static constexpr unsigned MaxActiveCopies = 64;

static G4_Declare* getRootDcl(G4_Operand* opnd) {
    G4_Declare* dcl = opnd->getTopDcl();
    return dcl ? dcl->getRootDeclare() : nullptr;
}

// Direct (non-indirect) GRF src or dst region with a declare.
static bool isDirectGRFOpnd(G4_Operand* opnd) {
    if (!opnd || !(opnd->isSrcRegRegion() || opnd->isDstRegRegion()))
        return false;
    return !opnd->isIndirect() && opnd->isGreg() && opnd->getTopDcl();
}

void PostRACopyProp::countReads() {
    numReads.clear();
    for (G4_BB* bb : kernel.fg) {
        for (G4_INST* inst : *bb) {
            for (unsigned i = 0, e = inst->getNumSrc(); i != e; ++i) {
                G4_Operand* src = inst->getSrc(i);
                if (src && src->isSrcRegRegion() &&
                    !src->asSrcRegRegion()->isIndirect()) {
                    if (G4_Declare* dcl = getRootDcl(src))
                        ++numReads[dcl];
                }
            }
        }
    }
}

// A raw, unpredicated NoMask mov makes dst and src interchangeable for any
// reader regardless of its execution mask.
bool PostRACopyProp::isCandidate(G4_INST* inst) const {
    if (!inst->isRawMov() || inst->getPredicate() ||
        !inst->isWriteEnableInst() || inst->isDoNotDelete() ||
        inst->getNeedPostRA())
        return false;

    G4_DstRegRegion* dst = inst->getDst();
    if (!isDirectGRFOpnd(dst))
        return false;
    // Indirect reads of an address-taken dst are not tracked.
    if (getRootDcl(dst)->getAddressed())
        return false;
    if (dst->getHorzStride() != 1 && inst->getExecSize() != g4::SIMD1)
        return false;

    G4_Operand* src0 = inst->getSrc(0);
    if (!isDirectGRFOpnd(src0))
        return false;
    if (!src0->asSrcRegRegion()->getRegion()->isContiguous(inst->getExecSize()))
        return false;

    unsigned dstStart = dst->getLinearizedStart();
    unsigned dstEnd = dst->getLinearizedEnd();
    unsigned srcStart = src0->getLinearizedStart();
    unsigned srcEnd = src0->getLinearizedEnd();
    // Same type, unit-stride dst and contiguous src imply equal sizes.
    vISA_ASSERT(dstEnd - dstStart == srcEnd - srcStart,
        "raw mov with mismatched dst/src footprints");
    if (srcStart <= dstEnd && dstStart <= srcEnd)
        return false;

    // A whole-GRF delta keeps the readers' sub-register offsets and regions.
    return dstStart % grfSize == srcStart % grfSize;
}

// Instructions that may read registers implicitly (call ABI, return value,
// intrinsics); all in-flight copies are kept across them.
bool PostRACopyProp::isBarrier(G4_INST* inst) const {
    return inst->isCall() || inst->isFCall() || inst->isReturn() ||
        inst->isFReturn() || inst->isIntrinsic();
}

// Readers whose operands must not be renamed. Their reads and writes are still
// tracked, so a copy they observe is kept.
bool PostRACopyProp::canRewriteInto(G4_INST* inst) const {
    // EOT payload must stay in the last GRFs.
    if (inst->isSend())
        return !inst->isEOT();
    // pln: src1 is an implicit register pair with alignment rules.
    // dpas: macro chaining/src reuse depend on exact registers.
    // madw: wide dst is expanded post-schedule.
    // madm, math.invm/rsqrtm: macro sequences tied to mme accumulators.
    if (inst->opcode() == G4_pln || inst->isDpas() ||
        inst->opcode() == G4_madw || inst->opcode() == G4_madm)
        return false;
    if (inst->isMath()) {
        G4_MathOp mathOp = inst->asMathInst()->getMathCtrl();
        if (mathOp == MATH_INVM || mathOp == MATH_RSQRTM)
            return false;
    }
    return true;
}

bool PostRACopyProp::isRewriteLegal(G4_INST* inst, unsigned srcIdx,
    const Footprint& newFp,
    const G4_Declare* oldDcl) const {
    // Honor explicitly requested even alignment.
    if (oldDcl->isEvenAlign() && (newFp.start / grfSize) % 2 != 0)
        return false;

    auto overlapsGRF = [this](const Footprint& a, const Footprint& b) {
        return a.start / grfSize <= b.end / grfSize &&
            b.start / grfSize <= a.end / grfSize;
        };
    auto isSingleGRF = [this](const Footprint& fp) {
        return fp.start / grfSize == fp.end / grfSize;
        };

    Footprint dstFp;
    if (getWriteFootprint(inst, dstFp) && overlapsGRF(newFp, dstFp)) {
        // Only single-GRF dst/src overlap is always legal.
        if (inst->isSend() || !isSingleGRF(newFp) || !isSingleGRF(dstFp))
            return false;
    }

    if (inst->isSend()) {
        // Send payloads must not overlap each other.
        for (unsigned i = 0, e = inst->getNumSrc(); i != e; ++i) {
            G4_Operand* other = inst->getSrc(i);
            if (i == srcIdx || !isDirectGRFOpnd(other))
                continue;
            Footprint otherFp{ other->getLinearizedStart(),
                              other->getLinearizedEnd() };
            if (overlapsGRF(newFp, otherFp))
                return false;
        }
    }
    return true;
}

bool PostRACopyProp::getWriteFootprint(G4_INST* inst, Footprint& fp) const {
    G4_DstRegRegion* dst = inst->getDst();
    if (!isDirectGRFOpnd(dst))
        return false;
    fp.start = dst->getLinearizedStart();
    fp.end = dst->getLinearizedEnd();
    if (inst->isSend()) {
        // The message may write past the declare-clamped dst.
        unsigned bytes = (unsigned)inst->asSendInst()->getMsgDesc()->getDstLenBytes();
        if (bytes > 0)
            fp.end = std::max(fp.end, fp.start + bytes - 1);
    }
    return true;
}

bool PostRACopyProp::isFullKill(G4_INST* inst, const Footprint& wFp,
    const Copy& c) const {
    if (inst->getPredicate() || !inst->isWriteEnableInst())
        return false;
    if (!inst->isSend() && inst->getDst()->getHorzStride() != 1 &&
        inst->getExecSize() != g4::SIMD1)
        return false;
    return wFp.contains(c.dst);
}

// True if zero remaining reads of dcl means its value is dead. numReads is
// kernel-wide, so any read in another BB keeps the mov. Declares that can be
// read implicitly (ABI, inputs/outputs, addressed) are excluded.
bool PostRACopyProp::canIgnoreLiveOut(G4_Declare* dcl) const {
    return dcl && dcl->getRegFile() == G4_GRF && !dcl->isInput() &&
        !dcl->isOutput() && !dcl->isPreDefinedVar() &&
        !dcl->getAddressed() && !builder.isPreDefArg(dcl) &&
        !builder.isPreDefRet(dcl) && !builder.isPreDefFEStackVar(dcl) &&
        !builder.isPreDefSpillHeader(dcl);
}

// Reads are scanned even when inst can't be rewritten: an unrewritten read of a
// copy's dst marks it observed so its mov is kept.
void PostRACopyProp::processReads(G4_INST* inst) {
    bool rewritable = canRewriteInto(inst);
    for (unsigned i = 0, e = inst->getNumSrc(); i != e; ++i) {
        G4_Operand* opnd = inst->getSrc(i);
        if (!isDirectGRFOpnd(opnd))
            continue;
        G4_SrcRegRegion* src = opnd->asSrcRegRegion();
        Footprint fp{ src->getLinearizedStart(), src->getLinearizedEnd() };

        // At most one forwardable copy covers a read.
        Copy* owner = nullptr;
        if (rewritable) {
            for (auto it = active.rbegin(), ie = active.rend(); it != ie; ++it) {
                if (it->canForward && it->dst.contains(fp)) {
                    owner = &*it;
                    break;
                }
            }
        }

        if (owner) {
            Footprint newFp{ fp.start - owner->dst.start + owner->src.start,
                            fp.end - owner->dst.start + owner->src.start };
            if (isRewriteLegal(inst, i, newFp, getRootDcl(src))) {
                G4_Declare* root = owner->srcDcl;
                unsigned off = newFp.start - root->getGRFOffsetFromR0();
                vISA_ASSERT(newFp.end - root->getGRFOffsetFromR0() <
                    root->getByteSize(),
                    "rewritten operand is out of bounds of its declare");
                G4_SrcRegRegion* newSrc = builder.createSrcRegRegion(
                    src->getModifier(), Direct, root->getRegVar(),
                    (short)(off / grfSize),
                    (short)((off % grfSize) / src->getTypeSize()), src->getRegion(),
                    src->getType(), src->getAccRegSel());
                inst->setSrc(newSrc, i);
                vISA_ASSERT(newSrc->getLinearizedStart() == newFp.start,
                    "unexpected rewritten operand location");
                owner->rewrites.push_back({ inst, i, src });
                --numReads[owner->dstDcl];
                ++numReads[root];
                fp = newFp;
            }
            else {
                owner = nullptr;
            }
        }

        for (Copy& c : active) {
            if (&c != owner && c.dst.overlaps(fp))
                c.observed = true;
        }
    }
}

void PostRACopyProp::processWrites(G4_INST* inst) {
    G4_DstRegRegion* dst = inst->getDst();
    if (!dst || dst->isNullReg())
        return;

    if (dst->isIndirect()) {
        // May write any addressed variable.
        for (Copy& c : active)
            c.canForward = false;
        return;
    }

    Footprint wFp;
    if (!getWriteFootprint(inst, wFp))
        return;

    for (unsigned i = 0; i < active.size();) {
        Copy& c = active[i];
        if (wFp.overlaps(c.src) || wFp.overlaps(c.dst))
            c.canForward = false;
        if (isFullKill(inst, wFp, c)) {
            finalize(c, true);
            active.erase(active.begin() + i);
            continue;
        }
        ++i;
    }
}

void PostRACopyProp::finalize(Copy& c, bool killed) {
    bool removable = !c.observed && (killed || (canIgnoreLiveOut(c.dstDcl) &&
        numReads[c.dstDcl] == 0));
    if (removable) {
        toErase.push_back(c.movIt);
        erased.insert(c.mov);
        // The mov's src may have been restored by another copy's rollback.
        --numReads[getRootDcl(c.mov->getSrc(0))];
        return;
    }

    for (auto it = c.rewrites.rbegin(), ie = c.rewrites.rend(); it != ie; ++it) {
        // Removed movs need no restore.
        if (erased.count(it->inst))
            continue;
        it->inst->setSrc(it->oldOpnd, it->srcIdx);
        --numReads[c.srcDcl];
        ++numReads[c.dstDcl];
    }
}

void PostRACopyProp::finalizeAll() {
    for (Copy& c : active)
        finalize(c, false);
    active.clear();
}

void PostRACopyProp::runOnBB(G4_BB* bb) {
    active.clear();
    toErase.clear();

    for (auto it = bb->begin(), ie = bb->end(); it != ie; ++it) {
        G4_INST* inst = *it;

        if (isBarrier(inst)) {
            for (Copy& c : active)
                c.observed = true;
            finalizeAll();
            continue;
        }

        processReads(inst);
        processWrites(inst);

        // An observed copy can't be removed; roll it back early.
        for (unsigned i = 0; i < active.size();) {
            Copy& c = active[i];
            if (c.observed) {
                finalize(c, false);
                active.erase(active.begin() + i);
                continue;
            }
            ++i;
        }

        if (isCandidate(inst)) {
            if (active.size() == MaxActiveCopies) {
                active.front().observed = true;
                finalize(active.front(), false);
                active.erase(active.begin());
            }
            Copy c;
            c.movIt = it;
            c.mov = inst;
            c.dst = { inst->getDst()->getLinearizedStart(),
                     inst->getDst()->getLinearizedEnd() };
            c.src = { inst->getSrc(0)->getLinearizedStart(),
                     inst->getSrc(0)->getLinearizedEnd() };
            c.dstDcl = getRootDcl(inst->getDst());
            c.srcDcl = getRootDcl(inst->getSrc(0));
            active.push_back(std::move(c));
        }
    }

    finalizeAll();

    for (INST_LIST_ITER it : toErase)
        bb->erase(it);
}

void PostRACopyProp::run() {
    countReads();
    erased.clear();
    for (G4_BB* bb : kernel.fg)
        runOnBB(bb);
}
