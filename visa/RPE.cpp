/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file contains implementation of Register Pressure Estimator.

#include "RPE.h"
#include "GraphColor.h"
#include "Timer.h"

namespace vISA
{
    RPE::RPE(const GlobalRA& g, const LivenessAnalysis* l) : m(1024), gra(g), liveAnalysis(l), live(l->getNumSelectedVar()),
        vars(l->vars)
    {
        options = g.kernel.getOptions();
    }

    void RPE::run()
    {
        TIME_SCOPE(RPE);
        if (!vars.empty())
        {
            for (auto& bb : gra.kernel.fg)
            {
                runBB(bb);
            }
        }
    }

    void RPE::runBB(G4_BB* bb)
    {
        G4_Declare* topdcl = nullptr;
        unsigned int id = 0;

        // Compute reg pressure at BB exit
        regPressureBBExit(bb);

        auto updateLivenessForLLR = [this](LocalLiveRange* LLR, bool val)
        {
            int numRows = LLR->getTopDcl()->getNumRows();
            int sreg;
            G4_VarBase* preg = LLR->getPhyReg(sreg);
            int startGRF = preg->asGreg()->getRegNum();
            for (int i = startGRF; i < startGRF + numRows; ++i)
            {
                G4_Declare* GRFDcl = gra.getGRFDclForHRA(i);
                updateLiveness(live, GRFDcl->getRegVar()->getId(), val);
            }
        };

        // Iterate in bottom-up order to analyze register usage (similar to intf graph construction)
        for (auto rInst = bb->rbegin(), rEnd = bb->rend(); rInst != rEnd; rInst++)
        {
            auto inst = (*rInst);
            auto dst = inst->getDst();

            rp[inst] = (uint32_t) regPressure;
            LocalLiveRange* LLR = nullptr;
            if (dst && (topdcl = dst->getTopDcl()))
            {
                if (topdcl->getRegVar()->isRegAllocPartaker())
                {
                    // Check if dst is killed
                    if (liveAnalysis->writeWholeRegion(bb, inst, dst, options) ||
                        inst->isPseudoKill())
                    {
                        id = topdcl->getRegVar()->getId();
                        updateLiveness(live, id, false);
                    }
                }
                else if ((LLR = gra.getLocalLR(topdcl)) && LLR->getAssigned())
                {
                    uint32_t firstRefIdx;
                    if (LLR->getFirstRef(firstRefIdx) == inst ||
                        liveAnalysis->writeWholeRegion(bb, inst, dst, options))
                    {
                        updateLivenessForLLR(LLR, false);
                    }
                }
            }

            for (unsigned int i = 0; i < G4_MAX_SRCS; i++)
            {
                auto src = inst->getSrc(i);
                G4_RegVar* regVar = nullptr;

                if (!src)
                    continue;

                if (!src->isSrcRegRegion() || !src->getTopDcl())
                    continue;

                if (!src->asSrcRegRegion()->isIndirect())
                {
                    if ((regVar = src->getTopDcl()->getRegVar()) &&
                        regVar->isRegAllocPartaker())
                    {
                        unsigned int id = regVar->getId();
                        updateLiveness(live, id, true);
                    }
                    else if ((LLR = gra.getLocalLR(src->getTopDcl())) && LLR->getAssigned())
                    {
                        updateLivenessForLLR(LLR, true);
                    }
                }
                else if (src->asSrcRegRegion()->isIndirect())
                {
                    // make every var in points-to set live
                    const REGVAR_VECTOR& pointsToSet = liveAnalysis->getPointsToAnalysis().getAllInPointsToOrIndrUse(src, bb);
                    for (auto pt : pointsToSet)
                    {
                        if (pt.var->isRegAllocPartaker())
                        {
                            updateLiveness(live, pt.var->getId(), true);
                        }
                    }
                }
            }
        }
    }

    void RPE::regPressureBBExit(G4_BB* bb)
    {
        live.clear();
        live = liveAnalysis->use_out[bb->getId()];
        live &= liveAnalysis->def_out[bb->getId()];

        // Iterate over all live variables and add up numRows required
        // for each. For scalar variables, add them up separately.
        regPressure = 0;
        unsigned int numScalars = 0;
        for (auto LI = live.begin(), LE = live.end(); LI != LE; ++LI) {
            unsigned i = *LI;
            {
                auto range = vars[i];
                G4_Declare* rootDcl = range->getDeclare()->getRootDeclare();
                if (rootDcl->getNumElems() > 1)
                {
                    regPressure += rootDcl->getNumRows();
                }
                else
                {
                    numScalars++;
                }
            }
        }

        regPressure += numScalars / 8.0;
    }

    void RPE::updateLiveness(SparseBitSet& live, uint32_t id, bool val)
    {
        auto oldVal = live.getElt(id / NUM_BITS_PER_ELT);
        live.set(id, val);
        auto newVal = live.getElt(id / NUM_BITS_PER_ELT);
        updateRegisterPressure(oldVal, newVal, id);
    }

    void RPE::updateRegisterPressure(unsigned int before, unsigned int after, unsigned int id)
    {
        auto change = before^after;
        if (change)
        {
            // For <1 GRF variable we have to take alignment into consideration as well when computing register pressure.
            // For now we double each <1GRF variable's size if its alignment also exceeds its size.
            // Alternative is to simply take the alignment as the size, but it might cause performance regressions
            // due to being too conservative (i.e., a GRF-aligned variable may share physical GRF with several other
            auto dclSize = vars[id]->getDeclare()->getByteSize();
            if (dclSize < gra.builder.getGRFSize() && dclSize < static_cast<uint32_t>(vars[id]->getDeclare()->getSubRegAlign()) * 2)
            {
                dclSize *= 2;
            }

            double delta = dclSize < gra.builder.getGRFSize() ?
                dclSize / (double) gra.builder.getGRFSize() : (double) vars[id]->getDeclare()->getNumRows();
            if (before & change)
            {
                if (regPressure < delta)
                {
                    regPressure = 0;
                }
                else
                {
                    regPressure -= delta;
                }
            }
            else
            {
                regPressure += delta;
            }
        }
        maxRP = std::max(maxRP, (uint32_t) regPressure);
    }

    void RPE::recomputeMaxRP()
    {
        maxRP = 0;
        // Find max register pressure over all entries in map
        for (auto item : rp)
        {
            maxRP = std::max(maxRP, item.second);
        }
    }

    void RPE::dump() const
    {
        std::cerr << "Max pressure = " << maxRP << "\n";
        for (auto& bb : gra.kernel.fg)
        {
            for (auto inst : *bb)
            {
                std::cerr << "[";
                if (rp.count(inst))
                {
                    std::cerr << rp.at(inst);
                }
                else
                {
                    std::cerr << "??";
                }
                std::cerr << "]";
                inst->dump();
            }
            std::cerr << "\n";
        }
    }
}
