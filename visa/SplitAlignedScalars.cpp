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

#include "SplitAlignedScalars.h"

using namespace vISA;

bool SplitAlignedScalars::canReplaceDst(G4_INST* inst)
{
    // acc has special rules, so skip optimizing instructions using acc
    if (inst->useAcc())
        return false;

    // whitelist of supported instructions
    if (inst->isMov() ||
        inst->isMath() ||
        inst->isArithmetic() ||
        inst->isLogic() ||
        inst->isCompare())
    {
        return true;
    }

    return false;
}

bool SplitAlignedScalars::canReplaceSrc(G4_INST* inst, unsigned int idx)
{
    auto opcode = inst->opcode();

    if (inst->useAcc())
        return false;

    if (inst->isMov() ||
        inst->isMath() ||
        inst->isArithmetic() ||
        inst->isLogic() ||
        inst->isCompare() ||
        opcode == G4_mad)
    {
        return true;
    }

    return false;
}

void SplitAlignedScalars::gatherCandidates()
{
    unsigned int lexId = 0;
    for (auto bb : kernel.fg)
    {
        for (auto inst : *bb)
        {
            inst->setLexicalId(lexId++);

            auto dst = inst->getDst();
            if (dst && dst->isDstRegRegion())
            {
                auto dstTopDcl = dst->getTopDcl();
                if (dstTopDcl &&
                    isDclCandidate(dstTopDcl))
                {
                    auto& Data = dclData[dstTopDcl];
                    Data.numDefs++;
                    if (Data.firstDef == 0)
                        Data.firstDef = lexId;

                    // disallow cases where scalar def has predicate
                    if (inst->getPredicate())
                    {
                        Data.allowed = false;
                    }

                    // send dst is scalar. disallow it's replacement
                    // if bb has few instructions than threshold as
                    // a copy after the send will cause stalls that
                    // cannot be hidden by scheduler.
                    if (inst->isSend() && bb->size() <= MinBBSize)
                    {
                        Data.allowed = false;
                    }

                    auto dstDcl = dst->asDstRegRegion()->getBase()->asRegVar()->getDeclare();
                    if (dstDcl->getAliasDeclare())
                    {
                        // disallow case where topdcl is scalar, but alias dcl
                        // is not a scalar as it may be smaller in size. for eg,
                        // topdcl may be :uq and alias may be of type :ud.
                        if (dstDcl->getByteSize() != dstTopDcl->getByteSize())
                            Data.allowed = false;
                    }
                }
            }

            for (unsigned int i = 0; i != inst->getNumSrc(); ++i)
            {
                auto src = inst->getSrc(i);
                if (src && src->isSrcRegRegion())
                {
                    auto srcTopDcl = src->getTopDcl();
                    if (srcTopDcl &&
                        isDclCandidate(srcTopDcl))
                    {
                        auto& Data = dclData[srcTopDcl];
                        Data.numSrcs++;
                        if (Data.lastUse < lexId)
                            Data.lastUse = lexId;
                    }
                }
            }
        }
    }
}

bool SplitAlignedScalars::isDclCandidate(G4_Declare* dcl)
{
    if (dcl->getRegFile() == G4_RegFileKind::G4_GRF &&
        dcl->getNumElems() == 1 &&
        !dcl->getAddressed() &&
        !dcl->getIsPartialDcl() &&
        !dcl->getRegVar()->isPhyRegAssigned() &&
        !dcl->getAliasDeclare() &&
        !dcl->isInput() &&
        !dcl->isOutput() &&
        !dcl->isPayloadLiveOut() &&
        !dcl->isDoNotSpill() &&
        gra.getSubRegAlign(dcl) == GRFALIGN)
        return true;
    return false;
}

bool SplitAlignedScalars::heuristic(G4_Declare* dcl, Data& d)
{
    if (d.getDUMaxDist() < MinOptDist)
        return false;

    if (!d.allowed)
    {
        return false;
    }

    return true;
}

// T is either G4_SrcRegRegion or G4_DstRegRegion
template<class T>
G4_Declare* SplitAlignedScalars::getDclForRgn(T* rgn, G4_Declare* newTopDcl)
{
    // newTopDcl is the replacement dcl computed by this pass to replace rgn
    // If rgn's dcl is not aliased then return newTopDcl as new rgn can
    // directly use this.
    // If rgn's dcl is aliased, create a new alias dcl based off newTopDcl
    // with correct offset and return it.
    auto rgnDcl = rgn->getBase()->asRegVar()->getDeclare();

    if (rgn->getTopDcl() == rgnDcl)
        return newTopDcl;

    auto newAliasDcl = kernel.fg.builder->createTempVar(rgnDcl->getNumElems(),
        rgnDcl->getElemType(), rgnDcl->getSubRegAlign());
    newAliasDcl->setAliasDeclare(newTopDcl, rgnDcl->getOffsetFromBase());

    return newAliasDcl;
}

void SplitAlignedScalars::run()
{
    auto getNewDcl = [&](G4_Declare* oldDcl)
    {
        auto newTopDcl = oldNewDcls[oldDcl];
        if (newTopDcl == nullptr)
        {
            newTopDcl = kernel.fg.builder->createTempVar(oldDcl->getNumElems(),
                oldDcl->getElemType(), G4_SubReg_Align::Any);
            oldNewDcls[oldDcl] = newTopDcl;
        }
        return newTopDcl;
    };

    gatherCandidates();

    for (auto dcl : kernel.Declares)
    {
        dcl = dcl->getRootDeclare();
        if (isDclCandidate(dcl))
        {
            auto& Data = dclData[dcl];
            if(!heuristic(dcl, Data))
                continue;
            // store all candidates
            oldNewDcls[dcl] = nullptr;
            numDclsReplaced++;
        }
    }

    for (auto bb : kernel.fg)
    {
        for (auto instIt = bb->begin(), instItEnd = bb->end(); instIt != instItEnd;)
        {
            auto newInstIt = instIt;
            auto inst = (*instIt);
            auto dst = inst->getDst();
            if (dst &&
                oldNewDcls.find(dst->getTopDcl()) != oldNewDcls.end())
            {
                auto oldTopDcl = dst->getTopDcl();
                auto newTopDcl = getNewDcl(oldTopDcl);
                auto newDcl = getDclForRgn(dst, newTopDcl);

                if (canReplaceDst(inst))
                {
                    auto newDstRgn = kernel.fg.builder->createDst(newDcl->getRegVar(), dst->getRegOff(), dst->getSubRegOff(),
                        dst->getHorzStride(), dst->getType());
                    inst->setDest(newDstRgn);
                }
                else
                {
                    // found an instruction where dst has to be GRF aligned,
                    // so we keep old dst but we insert a copy in new dcl
                    auto newAlignedTmpTopDcl = kernel.fg.builder->createTempVar(oldTopDcl->getNumElems(), oldTopDcl->getElemType(),
                        oldTopDcl->getSubRegAlign());
                    newAlignedTmpTopDcl->copyAlign(oldTopDcl);
                    auto newAlignedVar = getDclForRgn(dst, newAlignedTmpTopDcl);
                    auto dstRgn = kernel.fg.builder->createDst(newAlignedVar->getRegVar(), dst->getRegOff(), dst->getSubRegOff(),
                        dst->getHorzStride(), dst->getType());
                    inst->setDest(dstRgn);

                    // emit copy to store data to original non-aligned scalar
                    auto src = kernel.fg.builder->createSrc(dstRgn->getBase(), dstRgn->getRegOff(),
                        dstRgn->getSubRegOff(), inst->getExecSize() == g4::SIMD1 ? kernel.fg.builder->getRegionScalar() :
                        kernel.fg.builder->createRegionDesc(dstRgn->getHorzStride() * inst->getExecSize(),
                            dstRgn->getHorzStride(), inst->getExecSize()),
                        dstRgn->getType());
                    auto dstRgnOfCopy = kernel.fg.builder->createDst(newDcl->getRegVar(),
                        dst->getRegOff(), dst->getSubRegOff(), dst->getHorzStride(), dst->getType());

                    G4_Predicate* dupPred = nullptr;
                    if(inst->getPredicate())
                        dupPred = kernel.fg.builder->createPredicate(*inst->getPredicate());
                    auto newInst = kernel.fg.builder->createInternalInst(dupPred, G4_mov, nullptr,
                        g4::NOSAT, inst->getExecSize(), dstRgnOfCopy, src, nullptr, InstOpt_WriteEnable);
                    newInstIt = bb->insertAfter(instIt, newInst);
                }
            }

            for (unsigned int i = 0; i != inst->getNumSrc(); ++i)
            {
                auto src = inst->getSrc(i);
                if (!src || !src->isSrcRegRegion())
                    continue;

                auto srcRgn = src->asSrcRegRegion();

                if (oldNewDcls.find(srcRgn->getTopDcl()) != oldNewDcls.end())
                {
                    auto oldTopDcl = srcRgn->getTopDcl();
                    auto newTopDcl = getNewDcl(oldTopDcl);
                    auto newDcl = getDclForRgn(srcRgn, newTopDcl);

                    if (canReplaceSrc(inst, i))
                    {
                        auto newSrcRgn = kernel.fg.builder->createSrcRegRegion(srcRgn->getModifier(),
                            srcRgn->getRegAccess(), newDcl->getRegVar(),
                            srcRgn->getRegOff(), srcRgn->getSubRegOff(), srcRgn->getRegion(), srcRgn->getType());
                        inst->setSrc(newSrcRgn, i);
                    }
                    else
                    {
                        // create a new aligned tmp
                        auto newAlignedTmpTopDcl = kernel.fg.builder->createTempVar(oldTopDcl->getNumElems(), oldTopDcl->getElemType(),
                            oldTopDcl->getSubRegAlign());
                        newAlignedTmpTopDcl->copyAlign(oldTopDcl);

                        // copy oldDcl in to newAlignedTmpTopDcl
                        auto tmpDst = kernel.fg.builder->createDst(newAlignedTmpTopDcl->getRegVar(), oldTopDcl->getElemType());
                        auto src = kernel.fg.builder->createSrc(newTopDcl->getRegVar(), 0, 0, kernel.fg.builder->getRegionScalar(),
                            oldTopDcl->getElemType());
                        auto copy = kernel.fg.builder->createMov(g4::SIMD1, tmpDst, src, InstOpt_WriteEnable, false);

                        // now create src out of tmpDst
                        auto dclToUse = getDclForRgn(srcRgn, newAlignedTmpTopDcl);

                        auto newAlignedSrc = kernel.fg.builder->createSrcRegRegion(srcRgn->getModifier(), srcRgn->getRegAccess(),
                            dclToUse->getRegVar(), srcRgn->getRegOff(), srcRgn->getSubRegOff(), srcRgn->getRegion(), srcRgn->getType());
                        inst->setSrc(newAlignedSrc, i);
                        bb->insertBefore(instIt, copy);
                    }
                }
            }

            instIt = ++newInstIt;
        }
    }
}

void SplitAlignedScalars::dump(std::ostream& os)
{
    os << "# GRF aligned scalar dcls replaced: " << numDclsReplaced << std::endl;
}