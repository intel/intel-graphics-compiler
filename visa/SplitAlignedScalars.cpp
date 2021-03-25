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
    auto opcode = inst->opcode();

    // whitelist of supported instructions
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

bool SplitAlignedScalars::canReplaceSrc(G4_INST* inst, unsigned int idx)
{
    auto opcode = inst->opcode();

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
    if (dcl->getNumElems() == 1 &&
        !dcl->getAddressed() &&
        !dcl->getIsPartialDcl() &&
        !dcl->getRegVar()->isPhyRegAssigned() &&
        !dcl->getAliasDeclare() &&
        gra.getSubRegAlign(dcl) == GRFALIGN)
        return true;
    return false;
}

bool SplitAlignedScalars::heuristic(G4_Declare* dcl, Data& d)
{
    if (d.getDUMaxDist() < MinOptDist)
        return false;

    return true;
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

                auto newDstRgn = kernel.fg.builder->createDst(newTopDcl->getRegVar(), dst->getType());
                newDstRgn->setHorzStride(dst->getHorzStride());
                if (canReplaceDst(inst))
                {
                    MUST_BE_TRUE(inst->getExecSize() == g4::SIMD1, "Expecting scalar instruction");
                    inst->setDest(newDstRgn);
                }
                else
                {
                    // found an instruction where dst has to be GRF aligned,
                    // so we keep old dst but we insert a copy in new dcl
                    auto newAlignedVar = kernel.fg.builder->createTempVar(1, oldTopDcl->getElemType(), oldTopDcl->getSubRegAlign());
                    newAlignedVar->copyAlign(oldTopDcl);
                    auto dstRgn = kernel.fg.builder->createDst(newAlignedVar->getRegVar(), dst->getType());
                    inst->setDest(dstRgn);
                    auto src = kernel.fg.builder->createSrc(inst->getDst()->getBase(), inst->getDst()->getRegOff(),
                        inst->getDst()->getSubRegOff(), kernel.fg.builder->getRegionScalar(), inst->getDst()->getType());
                    auto newInst = kernel.fg.builder->createInternalInst(nullptr, G4_mov, nullptr, g4::NOSAT, g4::SIMD1,
                        newDstRgn, src, nullptr, InstOpt_WriteEnable);
                    newInstIt = bb->insertAfter(instIt, newInst);
                }
            }

            for (unsigned int i = 0; i != inst->getNumSrc(); ++i)
            {
                auto src = inst->getSrc(i);
                if (!src || !src->isSrcRegRegion() || !src->asSrcRegRegion()->isScalar())
                    continue;

                auto srcRgn = src->asSrcRegRegion();

                if (oldNewDcls.find(srcRgn->getTopDcl()) != oldNewDcls.end())
                {
                    auto oldTopDcl = srcRgn->getTopDcl();
                    auto newTopDcl = getNewDcl(oldTopDcl);

                    auto newSrcRgn = kernel.fg.builder->createSrcRegRegion(G4_SrcModifier::Mod_src_undef, srcRgn->getRegAccess(), newTopDcl->getRegVar(),
                        srcRgn->getRegOff(), srcRgn->getSubRegOff(), srcRgn->getRegion(), srcRgn->getType());
                    if (canReplaceSrc(inst, i))
                    {
                        MUST_BE_TRUE(inst->getExecSize() == g4::SIMD1 ||
                            srcRgn->isScalar(), "Expecting a scalar");
                        inst->setSrc(newSrcRgn, i);
                    }
                    else
                    {
                        // create a new aligned tmp
                        auto newAlignedTemp = kernel.fg.builder->createTempVar(1, srcRgn->getType(), oldTopDcl->getSubRegAlign());
                        newAlignedTemp->copyAlign(oldTopDcl);
                        auto newDstRgn = kernel.fg.builder->createDst(newAlignedTemp->getRegVar(), srcRgn->getType());
                        auto copy = kernel.fg.builder->createMov(g4::SIMD1, newDstRgn, newSrcRgn, InstOpt_WriteEnable, false);
                        auto newAlignedSrc = kernel.fg.builder->createSrc(newAlignedTemp->getRegVar(), 0, 0, kernel.fg.builder->getRegionScalar(),
                            srcRgn->getType());
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