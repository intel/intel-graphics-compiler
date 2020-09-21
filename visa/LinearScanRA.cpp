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

#include <fstream>
#include <tuple>
#include "DebugInfo.h"
#include "common.h"
#include "SpillManagerGMRF.h"
#include "LocalRA.h"
#include "LinearScanRA.h"
#include "RegAlloc.h"

using namespace std;
using namespace vISA;

extern void getForbiddenGRFs(vector<unsigned int>& regNum, G4_Kernel& kernel, unsigned stackCallRegSize, unsigned reserveSpillSize, unsigned reservedRegNum);

LinearScanRA::LinearScanRA(BankConflictPass& b, GlobalRA& g) :
    kernel(g.kernel), builder(g.builder), mem(g.builder.mem), bc(b), gra(g)
{
}

void LinearScanRA::allocForbiddenVector(LSLiveRange* lr)
{
    unsigned size = kernel.getNumRegTotal();

    if (size > 0)
    {
        bool* forbidden = (bool*)mem.alloc(sizeof(bool)*size);
        memset(forbidden, false, size);
        lr->setForbidden(forbidden);
    }
}

LSLiveRange* LinearScanRA::GetOrCreateLocalLiveRange(G4_Declare* topdcl)
{
    LSLiveRange* lr = gra.getLSLR(topdcl);

    // Check topdcl of operand and setup a new live range if required
    if (!lr)
    {
        lr = new (mem)LSLiveRange(builder) ;
        gra.setLSLR(topdcl, lr);
        allocForbiddenVector(lr);
    }

    MUST_BE_TRUE(lr != NULL, "Local LR could not be created");
    return lr;
}

void LinearScanRA::blockOutputPhyRegs()
{
    for (auto dcl : kernel.Declares)
    {
        if (dcl->isOutput() &&
            dcl->isInput())
        {
            // The live-range may never be referenced in the CFG so we need
            // this special pass to block physical registers corresponding
            // to those declares. This may be true for FC.
            pregs->markPhyRegs(dcl);
        }
    }
}

class isLifetimeCandidateOpCandidateForRemoval
{
public:
    isLifetimeCandidateOpCandidateForRemoval(GlobalRA& g) : gra(g)
    {
    }

    GlobalRA& gra;

    bool operator()(G4_INST* inst)
    {
        if (inst->isPseudoKill() ||
            inst->isLifeTimeEnd())
        {
            G4_Declare* topdcl = nullptr;

            if (inst->isPseudoKill())
            {
                topdcl = GetTopDclFromRegRegion(inst->getDst());
            }
            else
            {
                topdcl = GetTopDclFromRegRegion(inst->getSrc(0));
            }

            if (topdcl &&
                gra.getNumRefs(topdcl) == 0 &&
                (topdcl->getRegFile() == G4_GRF ||
                    topdcl->getRegFile() == G4_INPUT))
            {
                // Remove this lifetime op
                return true;
            }
        }

        return false;
    }
};

void LinearScanRA::removeUnrequiredLifetimeOps()
{
    // Iterate over all instructions and inspect only
    // pseudo_kills/lifetime.end instructions. Remove
    // instructions that have no other useful instruction.

    for (BB_LIST_ITER bb_it = kernel.fg.begin();
        bb_it != kernel.fg.end();
        bb_it++)
    {
        G4_BB* bb = (*bb_it);
        bb->erase(std::remove_if(bb->begin(), bb->end(), isLifetimeCandidateOpCandidateForRemoval(this->gra)),
            bb->end());
    }
}

void LinearScanRA::setLexicalID(bool includePseudo)
{
    unsigned int id = 0;
    for (auto bb : kernel.fg)
    {
        for (auto curInst : *bb)
        {
            if ((!includePseudo) && (curInst->isPseudoKill() ||
                curInst->isLifeTimeEnd()))
            {
                curInst->setLexicalId(id);
            }
            else
            {
                curInst->setLexicalId(id++);
            }
        }
    }
}

bool LinearScanRA::hasDstSrcOverlapPotential(G4_DstRegRegion* dst, G4_SrcRegRegion* src)
{
    int dstOpndNumRows = 0;

    if (dst->getBase()->isRegVar())
    {
        G4_Declare* dstDcl = dst->getBase()->asRegVar()->getDeclare();
        if (dstDcl != nullptr)
        {
            int dstOffset = (dstDcl->getOffsetFromBase() + dst->getLeftBound()) / numEltPerGRF(Type_UB);
            G4_DstRegRegion* dstRgn = dst;
            dstOpndNumRows = dstRgn->getSubRegOff() + dstRgn->getLinearizedEnd() - dstRgn->getLinearizedStart() + 1 > numEltPerGRF(Type_UB);

            if (src != NULL &&
                src->isSrcRegRegion() &&
                src->asSrcRegRegion()->getBase()->isRegVar())
            {
                G4_SrcRegRegion* srcRgn = src->asSrcRegRegion();
                G4_Declare* srcDcl = src->getBase()->asRegVar()->getDeclare();
                int srcOffset = (srcDcl->getOffsetFromBase() + src->getLeftBound()) / numEltPerGRF(Type_UB);
                bool srcOpndNumRows = srcRgn->getSubRegOff() + srcRgn->getLinearizedEnd() - srcRgn->getLinearizedStart() + 1 > numEltPerGRF(Type_UB);

                if (dstOpndNumRows || srcOpndNumRows)
                {
                    if (!(gra.isEvenAligned(dstDcl) && gra.isEvenAligned(srcDcl) &&
                        srcOffset % 2 == dstOffset % 2 &&
                        dstOpndNumRows && srcOpndNumRows))
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

void LinearScanRA::getRowInfo(int size, int& nrows, int& lastRowSize)
{
    if (size <= (int)numEltPerGRF(Type_UW))
    {
        nrows = 1;
    }
    else
    {
        // nrows is total number of rows, including last row even if it is partial
        nrows = size / numEltPerGRF(Type_UW);
        // lastrowsize is number of words actually used in last row
        lastRowSize = size % numEltPerGRF(Type_UW);

        if (size % numEltPerGRF(Type_UW) != 0)
        {
            nrows++;
        }

        if (lastRowSize == 0)
        {
            lastRowSize = numEltPerGRF(Type_UW);
        }
    }

    return;
}

unsigned int LinearScanRA::convertSubRegOffFromWords(G4_Declare* dcl, int subregnuminwords)
{
    // Return subreg offset in units of dcl's element size.
    // Input is subregnum in word units.
    unsigned int subregnum;

    subregnum = (subregnuminwords * 2) / dcl->getElemSize();

    return subregnum;
}

void LSLiveRange::recordRef(G4_BB* bb)
{
    if (bb != prevBBRef)
        numRefsInFG++;

    prevBBRef = bb;
}

bool LSLiveRange::isGRFRegAssigned()
{
    MUST_BE_TRUE(topdcl != NULL, "Top dcl not set");
    G4_RegVar* rvar = topdcl->getRegVar();
    bool isPhyRegAssigned = false;

    if (rvar)
    {
        if (rvar->isPhyRegAssigned())
            isPhyRegAssigned = true;
    }

    return isPhyRegAssigned;
}

unsigned int LSLiveRange::getSizeInWords()
{
    int nrows = getTopDcl()->getNumRows();
    int elemsize = getTopDcl()->getElemSize();
    int nelems = getTopDcl()->getNumElems();
    int words = 0;

    if (nrows > 1)
    {
        // If sizeInWords is set, use it otherwise consider entire row reserved
        unsigned int sizeInWords = getTopDcl()->getWordSize();

        if (sizeInWords > 0)
            words = sizeInWords;
        else
            words = nrows * numEltPerGRF(Type_UW);
    }
    else if (nrows == 1)
    {
        int nbytesperword = 2;
        words = (nelems * elemsize + 1) / nbytesperword;
    }

    return words;
}

unsigned short globalLinearScan::getOccupiedBundle(G4_Declare* dcl)
{
    unsigned short occupiedBundles = 0;
    unsigned bundleNum = 0;


    for (size_t i = 0, dclConflictSize = gra.getBundleConflictDclSize(dcl); i < dclConflictSize; i++)
    {
        int offset = 0;
        G4_Declare* bDcl = gra.getBundleConflictDcl(dcl, i, offset);
        LSLiveRange* lr = gra.getLSLR(bDcl);
        G4_VarBase* preg;
        int  subregnum;
        preg = lr->getPhyReg(subregnum);

        if (preg != NULL)
        {
            unsigned int reg = preg->asGreg()->getRegNum();
            unsigned int bundle = gra.get_bundle(reg, offset);
            unsigned int bundle1 = gra.get_bundle(reg, offset + 1);
            if (!(occupiedBundles & ((unsigned short)1 << bundle)))
            {
                bundleNum++;
            }
            occupiedBundles |= (unsigned short)1 << bundle;
            occupiedBundles |= (unsigned short)1 << bundle1;
        }
    }
    if (bundleNum > 12)
    {
        occupiedBundles = 0;
    }

    return occupiedBundles;
}

// Mark physical register allocated to range lr as not busy
void globalLinearScan::freeAllocedRegs(LSLiveRange* lr, bool setInstID)
{
    int sregnum;

    G4_VarBase* preg = lr->getPhyReg(sregnum);

    MUST_BE_TRUE(preg != NULL,
        "Physical register not assigned to live range. Cannot free regs.");

    unsigned int idx = 0;
    if (setInstID)
    {
        lr->getLastRef(idx);
    }

    if (!lr->isUseUnAvailableReg())
    {
        pregManager.freeRegs(preg->asGreg()->getRegNum(),
            sregnum,
            lr->getSizeInWords(),
            idx);
    }
}

// idx is the current position being processed
// The function will expire active ranges that end at or before idx
void globalLinearScan::expireRanges(unsigned int idx)
{
    //active list is sorted in ascending order of starting index

    while (active.size() > 0)
    {
        unsigned int endIdx;
        LSLiveRange* lr = active.front();

        lr->getLastRef(endIdx);

        if (endIdx <= idx)
        {
            G4_VarBase* preg;
            int subregnumword, subregnum;

            preg = lr->getPhyReg(subregnumword);

            if (preg)
            {
                subregnum = LocalRA::convertSubRegOffFromWords(lr->getTopDcl(), subregnumword);

                // Mark the RegVar object of dcl as assigned to physical register
                lr->getTopDcl()->getRegVar()->setPhyReg(preg, subregnum);
                lr->setAssigned(true);
            }

            // Free physical regs marked for this range
            freeAllocedRegs(lr, true);

#ifdef DEBUG_VERBOSE_ON
            DEBUG_VERBOSE("Expiring range " << lr->getTopDcl()->getName() << std::endl);
#endif

            // Remove range from active list
            active.pop_front();
        }
        else
        {
            // As soon as we find first range that ends after ids break loop
            break;
        }
    }
}

void globalLinearScan::printActives()
{
    std::cout << "====================ACTIVATE START===================" << std::endl;
    for (auto lr : active)
    {
        unsigned int start, end;

        lr->getFirstRef(start);
        lr->getLastRef(end);

        int startregnum, endregnum, startsregnum, endsregnum;
        G4_VarBase* op;
        op = lr->getPhyReg(startsregnum);

        startregnum = endregnum = op->asGreg()->getRegNum();
        endsregnum = startsregnum + (lr->getTopDcl()->getNumElems() * lr->getTopDcl()->getElemSize() / 2) - 1;

        if (lr->getTopDcl()->getNumRows() > 1) {
            endregnum = startregnum + lr->getTopDcl()->getNumRows() - 1;

            if (lr->getTopDcl()->getWordSize() > 0)
            {
                endsregnum = lr->getTopDcl()->getWordSize() % numEltPerGRF(Type_UW) - 1;
                if (endsregnum < 0) endsregnum = 15;
            }
            else
                endsregnum = 15; // last word in GRF
        }
        if (lr->hasIndirectAccess())
        {
            std::cout << "INDIR: ";
        }
        else
        {
            std::cout << "DIR  : ";
        }
        if (lr->getPreAssigned())
        {
            std::cout << "\tPRE: ";
        }
        else
        {
            std::cout << "\tNOT: ";
        }

        std::cout << lr->getTopDcl()->getName() << "(" << start << ", " << end << ", " << lr->getTopDcl()->getByteSize() << ")";
        std::cout << " (r" << startregnum << "." << startsregnum << ":w - " <<
            "r" << endregnum << "." << endsregnum << ":w)";
        std::cout << std::endl;
    }
    for (int i = 0; i < (int)(numRegLRA); i++)
    {
        std::cout << "\nR" << i << ":";

        if (activeGRF[i].activeInput.size())
        {
            for (auto lr : activeGRF[i].activeLV)
            {
                int startregnum, endregnum, startsregnum, endsregnum;
                G4_VarBase* op;
                op = lr->getPhyReg(startsregnum);

                startregnum = endregnum = op->asGreg()->getRegNum();
                endsregnum = startsregnum + (lr->getTopDcl()->getNumElems() * lr->getTopDcl()->getElemSize() / 2) - 1;

                if (lr->getTopDcl()->getNumRows() > 1) {
                    endregnum = startregnum + lr->getTopDcl()->getNumRows() - 1;

                    if (lr->getTopDcl()->getWordSize() > 0)
                    {
                        endsregnum = lr->getTopDcl()->getWordSize() % numEltPerGRF(Type_UW) - 1;
                        if (endsregnum < 0) endsregnum = 15;
                    }
                    else
                        endsregnum = 15; // last word in GRF
                }

                std::cout << "\tIN: " << lr->getTopDcl()->getName();
                std::cout << "(r" << startregnum << "." << startsregnum << ":w - " <<
                    "r" << endregnum << "." << endsregnum << ":w)";
                //std::cout << std::endl;
            }
        }

        if (activeGRF[i].activeLV.size())
        {
            // There may be multiple variables take same register with different offsets
            for (auto lr : activeGRF[i].activeLV)
            {
                int startregnum, endregnum, startsregnum, endsregnum;
                G4_VarBase* op;
                op = lr->getPhyReg(startsregnum);

                startregnum = endregnum = op->asGreg()->getRegNum();
                endsregnum = startsregnum + (lr->getTopDcl()->getNumElems() * lr->getTopDcl()->getElemSize() / 2) - 1;

                if (lr->getTopDcl()->getNumRows() > 1) {
                    endregnum = startregnum + lr->getTopDcl()->getNumRows() - 1;

                    if (lr->getTopDcl()->getWordSize() > 0)
                    {
                        endsregnum = lr->getTopDcl()->getWordSize() % numEltPerGRF(Type_UW) - 1;
                        if (endsregnum < 0) endsregnum = 15;
                    }
                    else
                        endsregnum = 15; // last word in GRF
                }

                std::cout << "\t" << lr->getTopDcl()->getName();
                std::cout << "(r" << startregnum << "." << startsregnum << ":w - " <<
                    "r" << endregnum << "." << endsregnum << ":w)";
            }
        }
    }
    std::cout << "====================ACTIVATE END===================" << std::endl;
}

void globalLinearScan::expireAllActive()
{
    if (active.size() > 0)
    {
        // Expire any remaining ranges
        LSLiveRange* lastActive = active.back();
        unsigned int endIdx;

        lastActive->getLastRef(endIdx);

        expireRanges(endIdx);
    }
}

void LinearScanRA::linearScanMarkReferencesInOpnd(G4_Operand* opnd, bool isEOT, INST_LIST_ITER inst_it,
    unsigned int pos)
{
    G4_Declare* topdcl = NULL;

    if ((opnd->isSrcRegRegion() || opnd->isDstRegRegion()))
    {
        topdcl = GetTopDclFromRegRegion(opnd);

        if (topdcl &&
            (topdcl->getRegFile() == G4_GRF || topdcl->getRegFile() == G4_INPUT))
        {
            // Handle GRF here
            MUST_BE_TRUE(topdcl->getAliasDeclare() == NULL, "Not topdcl");
            LSLiveRange* lr = GetOrCreateLocalLiveRange(topdcl);

            lr->recordRef(curBB);
            if (isEOT)
            {
                lr->markEOT();
            }
            gra.recordRef(topdcl);
            if (topdcl->getRegVar()
                && topdcl->getRegVar()->isPhyRegAssigned()
                && topdcl->getRegVar()->getPhyReg()->isGreg())
            {
                lr->setPreAssigned(true);
            }
        }
    }
    else if (opnd->isAddrExp())
    {
        G4_AddrExp* addrExp = opnd->asAddrExp();

        topdcl = addrExp->getRegVar()->getDeclare();
        while (topdcl->getAliasDeclare() != NULL)
            topdcl = topdcl->getAliasDeclare();

        MUST_BE_TRUE(topdcl != NULL, "Top dcl was null for addr exp opnd");

        LSLiveRange* lr = GetOrCreateLocalLiveRange(topdcl);

        lr->recordRef(curBB);
        lr->markIndirectRef();
        gra.recordRef(topdcl);
        if (topdcl->getRegVar()
            && topdcl->getRegVar()->isPhyRegAssigned()
            && topdcl->getRegVar()->getPhyReg()->isGreg())
        {
            lr->setPreAssigned(true);
        }
    }

}

void LinearScanRA::linearScanMarkReferencesInInst(INST_LIST_ITER inst_it)
{
    auto inst = (*inst_it);

    if (inst->getNumDst() > 0)
    {
        // Scan dst
        G4_Operand* dst = inst->getDst();

        if (dst)
        {
            linearScanMarkReferencesInOpnd(dst, false, inst_it, 0);
        }
    }

    // Scan srcs
    for (int i = 0, nSrcs = inst->getNumSrc(); i < nSrcs; i++)
    {
        G4_Operand* src = inst->getSrc(i);

        if (src)
        {
            linearScanMarkReferencesInOpnd(src, inst->isEOT(), inst_it, i);
        }
    }
}

bool LSLiveRange::isLiveRangeGlobal()
{
    if (isIndirectAccess == true || (numRefsInFG > 1) ||
        topdcl->isInput() == true || topdcl->isOutput() == true)
    {
        return true;
    }

    return false;
}

void LinearScanRA::linearScanMarkReferences(unsigned int& numRowsEOT)
{
    unsigned int id = 0;
    // Iterate over all BBs
    for (auto _curBB : kernel.fg)
    {
        curBB = _curBB;
        // Iterate over all insts
        for (INST_LIST_ITER inst_it = curBB->begin(), inst_end = curBB->end(); inst_it != inst_end; ++inst_it)
        {
            G4_INST* curInst = (*inst_it);

            if (curInst->isPseudoKill() ||
                curInst->isLifeTimeEnd())
            {
                curInst->setLexicalId(id);
                if (curInst->isLifeTimeEnd())
                {
                    linearScanMarkReferencesInInst(inst_it);
                }
                continue;
            }

            // set lexical ID
            curInst->setLexicalId(id++);

            if (curInst->isEOT() && kernel.fg.builder->hasEOTGRFBinding())
            {
                numRowsEOT += curInst->getSrc(0)->getTopDcl()->getNumRows();

                if (curInst->isSplitSend() && !curInst->getSrc(1)->isNullReg())
                {
                    // both src0 and src1 have to be >=r112
                    numRowsEOT += curInst->getSrc(1)->getTopDcl()->getNumRows();
                }
            }

            linearScanMarkReferencesInInst(inst_it);
        }
    }

    for (DECLARE_LIST_ITER dcl_it = kernel.Declares.begin();
        dcl_it != kernel.Declares.end();
        dcl_it++)
    {
        G4_Declare* dcl = (*dcl_it);
        LSLiveRange* lr = gra.getLSLR(dcl);

        if (lr && lr->isLiveRangeGlobal())
        {
            globalDeclares.push_back(dcl);
        }
    }
}

void LinearScanRA::preRAAnalysis()
{
    int numGRF = kernel.getNumRegTotal();

    // Clear LSLiveRange* computed preRA
    gra.clearStaleLiveRanges();

    // Mark those physical registers busy that are declared with Output attribute
    // The live interval will gurantee they are not reused.
    blockOutputPhyRegs();

    // Mark references made to decls
    linearScanMarkReferences(numRowsEOT);

    // Check whether pseudo_kill/lifetime.end are only references
    // for their respective variables. Remove them if so. Doing this
    // helps reduce number of variables in symbol table increasing
    // changes of skipping global RA.
    removeUnrequiredLifetimeOps();

    // Remove unreferenced dcls
    gra.removeUnreferencedDcls();

    numRegLRA = numGRF;

    unsigned int reservedGRFNum = builder.getOptions()->getuInt32Option(vISA_ReservedGRFNum);
    if (reservedGRFNum || builder.getOption(vISA_Debug))
    {
        vector<unsigned int> forbiddenRegs;
        getForbiddenGRFs(forbiddenRegs, kernel, 0, 0, reservedGRFNum);
        for (unsigned int i = 0, size = forbiddenRegs.size(); i < size; i++)
        {
            unsigned int regNum = forbiddenRegs[i];
            pregs->setGRFUnavailable(regNum); //un-available will always be there, if it's conflict with input or pre-assigned, it's still un-available.
        }

        if (builder.getOption(vISA_Debug))
        {
            // Since LinearScanRA is not undone when debug info generation is required,
            // for keeping compile time low, we allow fewer physical registers
            // as assignable candidates. Without this, we could run in to a
            // situation where very few physical registers are available for GRA
            // and it is unable to assign registers even with spilling.
#define USABLE_GRFS_WITH_DEBUG_INFO 80
            int maxSendReg = 0;
            for (auto bb : kernel.fg)
            {
                for (auto inst : *bb)
                {
                    if (inst->isSend() || inst->isSplitSend())
                    {
                        maxSendReg = (inst->getMsgDesc()->ResponseLength() > maxSendReg) ?
                            (inst->getMsgDesc()->ResponseLength()) : maxSendReg;
                        maxSendReg = (inst->getMsgDesc()->MessageLength() > maxSendReg) ?
                            (inst->getMsgDesc()->MessageLength()) : maxSendReg;
                        maxSendReg = (inst->getMsgDesc()->extMessageLength() > maxSendReg) ?
                            (inst->getMsgDesc()->extMessageLength()) : maxSendReg;
                    }
                }
            }

            int maxRegsToUse = USABLE_GRFS_WITH_DEBUG_INFO;
            if (maxSendReg > (numGRF - USABLE_GRFS_WITH_DEBUG_INFO))
            {
                maxRegsToUse = (numGRF - maxSendReg) - 10;
            }

            // Also check max size of addressed GRF
            unsigned int maxAddressedRows = 0;
            for (auto dcl : kernel.Declares)
            {
                if (dcl->getAddressed() &&
                    maxAddressedRows < dcl->getNumRows())
                {
                    maxAddressedRows = dcl->getNumRows();
                }
            }

            // Assume indirect operand of maxAddressedRows exists
            // on dst, src0, src1. This is overly conservative but
            // should work for general cases.
            if ((numGRF - maxRegsToUse) / 3 < (int)maxAddressedRows)
            {
                maxRegsToUse = (numGRF - (maxAddressedRows * 3));

                if (maxRegsToUse < 0)
                    maxRegsToUse = 0;
            }

            for (int i = maxRegsToUse; i < numGRF; i++)
            {
                pregs->setGRFUnavailable(i);
            }
        }
    }
    else
    {
        pregs->setSimpleGRFAvailable(true);
        const Options* opt = builder.getOptions();
        if (kernel.getInt32KernelAttr(Attributes::ATTR_Target) != VISA_3D ||
            opt->getOption(vISA_enablePreemption) ||
            (kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc()) ||
            opt->getOption(vISA_ReserveR0))
        {
            pregs->setR0Forbidden();
        }

        if (opt->getOption(vISA_enablePreemption))
        {
            pregs->setR1Forbidden();
        }
    }

    return;
}

void LinearScanRA::saveRegs(
    unsigned startReg, unsigned owordSize, G4_Declare* scratchRegDcl, G4_Declare* framePtr,
    unsigned frameOwordOffset, G4_BB* bb, INST_LIST_ITER insertIt)
{

    assert(builder.getPlatform() >= GENX_SKL && "stack call only supported on SKL+");

    if (owordSize == 8 || owordSize == 4 || owordSize == 2)
    {
        // add (1) r126.2<1>:ud    r125.7<0;1,0>:ud    0x2:ud
        // sends (8) null<1>:ud    r126.0    r1.0 ...
        G4_ExecSize execSize = (owordSize > 2) ? g4::SIMD16 : g4::SIMD8;
        unsigned messageLength = ROUND(owordSize, 2) / 2;
        G4_Declare* msgDcl = builder.createTempVar(messageLength * GENX_DATAPORT_IO_SZ,
            Type_UD, GRFALIGN, "StackCall");
        msgDcl->getRegVar()->setPhyReg(builder.phyregpool.getGreg(startReg), 0);
        auto sendSrc2 = builder.createSrcRegRegion(Mod_src_undef, Direct, msgDcl->getRegVar(), 0, 0,
            builder.getRegionStride1(), Type_UD);
        G4_DstRegRegion* dst = builder.createNullDst((execSize > 8) ? Type_UW : Type_UD);
        G4_INST* spillIntrinsic = nullptr;
            spillIntrinsic = builder.createSpill(dst, sendSrc2, execSize, messageLength, frameOwordOffset / 2, framePtr, InstOpt_WriteEnable);
        bb->insertBefore(insertIt, spillIntrinsic);
    }
    else if (owordSize > 8)
    {
        saveRegs(startReg, 8, scratchRegDcl, framePtr, frameOwordOffset, bb, insertIt);
        saveRegs(startReg + GlobalRA::owordToGRFSize(8), owordSize - 8, scratchRegDcl, framePtr, frameOwordOffset + 8, bb, insertIt);
    }
    //
    // Split into chunks of sizes 4 and remaining owords.
    //
    else if (owordSize > 4)
    {
        saveRegs(startReg, 4, scratchRegDcl, framePtr, frameOwordOffset, bb, insertIt);
        saveRegs(startReg + GlobalRA::owordToGRFSize(4), owordSize - 4, scratchRegDcl, framePtr, frameOwordOffset + 4, bb, insertIt);
    }
    //
    // Split into chunks of sizes 2 and remaining owords.
    //
    else if (owordSize > 2)
    {
        saveRegs(startReg, 2, scratchRegDcl, framePtr, frameOwordOffset, bb, insertIt);
        saveRegs(startReg + GlobalRA::owordToGRFSize(2), owordSize - 2, scratchRegDcl, framePtr, frameOwordOffset + 2, bb, insertIt);
    }
    else
    {
        MUST_BE_TRUE(false, ERROR_REGALLOC);
    }
}

//
// Generate the save code for the i/p saveRegs.
//
void LinearScanRA::saveActiveRegs(
    std::vector<bool>& saveRegs, unsigned startReg, unsigned frameOffset,
    G4_BB* bb, INST_LIST_ITER insertIt)
{
    G4_Declare* scratchRegDcl = builder.kernel.fg.scratchRegDcl;
    G4_Declare* framePtr = builder.kernel.fg.framePtrDcl;

    unsigned frameOwordPos = frameOffset;
    unsigned startPos = 0;

    while (startPos < saveRegs.size())
    {
        for (; startPos < saveRegs.size() && saveRegs[startPos] == false; startPos++);
        if (startPos < saveRegs.size() && saveRegs[startPos]) {
            unsigned endPos = startPos + 1;
            for (; endPos < saveRegs.size() && saveRegs[endPos] == true; endPos++);
            unsigned owordSize = (endPos - startPos) * GlobalRA::GRFSizeToOwords(1);
            owordSize = std::max(owordSize, GlobalRA::GRFSizeToOwords(1));
            this->saveRegs(startPos + startReg, owordSize, scratchRegDcl, framePtr, frameOwordPos, bb, insertIt);
            frameOwordPos += owordSize;
            startPos = endPos;
        }
    }
}

//
// Generate the restore code for the i/p restoreRegs.
//
void LinearScanRA::restoreActiveRegs(
    std::vector<bool>& restoreRegs, unsigned startReg, unsigned frameOffset,
    G4_BB* bb, INST_LIST_ITER insertIt)
{
    G4_Declare* scratchRegDcl = builder.kernel.fg.scratchRegDcl;
    G4_Declare* framePtr = builder.kernel.fg.framePtrDcl;

    unsigned frameOwordPos = frameOffset;
    unsigned startPos = 0;

    while (startPos < restoreRegs.size())
    {
        for (; startPos < restoreRegs.size() && restoreRegs[startPos] == false; startPos++);
        if (startPos < restoreRegs.size() && restoreRegs[startPos]) {
            unsigned endPos = startPos + 1;
            for (; endPos < restoreRegs.size() && restoreRegs[endPos] == true; endPos++);
            unsigned owordSize = (endPos - startPos) * GlobalRA::GRFSizeToOwords(1);
            owordSize = std::max(owordSize, GlobalRA::GRFSizeToOwords(1));
            this->restoreRegs(startPos + startReg, owordSize, scratchRegDcl, framePtr, frameOwordPos, bb, insertIt);
            frameOwordPos += owordSize;
            startPos = endPos;
        }
    }
}

//
// Generate the restore code for startReg to startReg+owordSize/2.
//
void LinearScanRA::restoreRegs(
    unsigned startReg, unsigned owordSize, G4_Declare* scratchRegDcl, G4_Declare* framePtr,
    unsigned frameOwordOffset, G4_BB* bb, INST_LIST_ITER insertIt)
{
    //
    // Process chunks of size 8, 4, 2 and 1.
    //
    if (owordSize == 8 || owordSize == 4 || owordSize == 2)
    {
        G4_ExecSize execSize = (owordSize > 2) ? g4::SIMD16 : g4::SIMD8;
        unsigned responseLength = ROUND(owordSize, 2) / 2;
        G4_Declare* dstDcl = builder.createTempVar(responseLength * GENX_DATAPORT_IO_SZ,
            Type_UD, GRFALIGN, "StackCall");
        dstDcl->getRegVar()->setPhyReg(builder.phyregpool.getGreg(startReg), 0);
        G4_DstRegRegion* dstRgn = builder.createDst(dstDcl->getRegVar(), 0, 0, 1, (execSize > 8) ? Type_UW : Type_UD);
        G4_INST* fillIntrinsic = nullptr;
            fillIntrinsic = builder.createFill(dstRgn, execSize, responseLength, frameOwordOffset / 2, framePtr, InstOpt_WriteEnable);
        bb->insertBefore(insertIt, fillIntrinsic);
    }
    //
    // Split into chunks of sizes 8 and remaining owords.
    //
    else if (owordSize > 8)
    {
        restoreRegs(startReg, 8, scratchRegDcl, framePtr, frameOwordOffset, bb, insertIt);
        restoreRegs(startReg + GlobalRA::owordToGRFSize(8), owordSize - 8, scratchRegDcl, framePtr, frameOwordOffset + 8, bb, insertIt);
    }
    //
    // Split into chunks of sizes 4 and remaining owords.
    //
    else if (owordSize > 4)
    {
        restoreRegs(startReg, 4, scratchRegDcl, framePtr, frameOwordOffset, bb, insertIt);
        restoreRegs(startReg + GlobalRA::owordToGRFSize(4), owordSize - 4, scratchRegDcl, framePtr, frameOwordOffset + 4, bb, insertIt);
    }
    //
    // Split into chunks of sizes 2 and remaining owords.
    //
    else if (owordSize > 2)
    {
        restoreRegs(startReg, 2, scratchRegDcl, framePtr, frameOwordOffset, bb, insertIt);
        restoreRegs(startReg + GlobalRA::owordToGRFSize(2), owordSize - 2, scratchRegDcl, framePtr, frameOwordOffset + 2, bb, insertIt);
    }
    else
    {
        MUST_BE_TRUE(false, ERROR_REGALLOC);
    }
}

void LinearScanRA::OptimizeActiveRegsFootprint(std::vector<bool>& saveRegs)
{
    unsigned startPos = 0;
    while (startPos < saveRegs.size())
    {
        for (; startPos < saveRegs.size() && !saveRegs[startPos]; ++startPos);
        if (startPos == saveRegs.size())
        {
            break;
        }
        if (startPos + 4 <= saveRegs.size())
        {
            if (saveRegs[startPos] & saveRegs[startPos + 2] & !saveRegs[startPos + 3])
            {
                saveRegs[startPos + 1] = saveRegs[startPos + 3] = true;
            }
            else if (saveRegs[startPos] & saveRegs[startPos + 3])
            {
                if (startPos + 4 < saveRegs.size())
                {
                    if (!saveRegs[startPos + 4])
                    {
                        saveRegs[startPos + 1] = saveRegs[startPos + 2] = true;
                    }
                }
                else
                {
                    saveRegs[startPos + 1] = saveRegs[startPos + 2] = true;
                }
            }
        }
        unsigned winBound = (unsigned)((saveRegs.size() < startPos + 4) ? saveRegs.size() : startPos + 4);
        for (; startPos < winBound && saveRegs[startPos]; ++startPos);
    }
}

void LinearScanRA::OptimizeActiveRegsFootprint(std::vector<bool>& saveRegs, std::vector<bool>& retRegs)
{
    unsigned startPos = 0;
    while (startPos < saveRegs.size())
    {
        for (; startPos < saveRegs.size() && !saveRegs[startPos]; ++startPos);
        if (startPos == saveRegs.size())
        {
            break;
        }
        if (startPos + 4 <= saveRegs.size())
        {
            if (saveRegs[startPos] & saveRegs[startPos + 2])
            {
                if (!saveRegs[startPos + 1] & !retRegs[startPos + 1])
                {
                    saveRegs[startPos + 1] = true;
                }
                if (!saveRegs[startPos + 3] & !retRegs[startPos + 3])
                {
                    saveRegs[startPos + 3] = true;
                }
            }
            else if (saveRegs[startPos] & saveRegs[startPos + 3])
            {
                if (startPos + 4 < saveRegs.size())
                {
                    if (!saveRegs[startPos + 4])
                    {
                        if (!saveRegs[startPos + 1] & !retRegs[startPos + 1])
                        {
                            saveRegs[startPos + 1] = true;
                        }
                        if (!saveRegs[startPos + 2] & !retRegs[startPos + 2])
                        {
                            saveRegs[startPos + 2] = true;
                        }
                    }
                }
                else
                {
                    if (!saveRegs[startPos + 1] & !retRegs[startPos + 1])
                    {
                        saveRegs[startPos + 1] = true;
                    }
                    if (!saveRegs[startPos + 2] & !retRegs[startPos + 2])
                    {
                        saveRegs[startPos + 2] = true;
                    }
                }
            }
        }
        unsigned winBound = (unsigned)((saveRegs.size() < startPos + 4) ? saveRegs.size() : startPos + 4);
        for (; startPos < winBound && saveRegs[startPos]; ++startPos);
    }
}

void LinearScanRA::addCallerSaveRestoreCode()
{

    uint32_t maxCallerSaveSize = 0;
    unsigned int callerSaveNumGRF = builder.kernel.getCallerSaveLastGRF() + 1;

    for (BB_LIST_ITER it = builder.kernel.fg.begin(); it != builder.kernel.fg.end(); ++it)
    {
        if ((*it)->isEndWithFCall())
        {
            //
            // Determine the caller-save registers per call site.
            //
            std::vector<bool> callerSaveRegs(callerSaveNumGRF, false);
            std::vector<bool> retRegs(callerSaveNumGRF, false);
            unsigned callerSaveRegCount = 0;
            G4_INST* callInst = (*it)->back();
            ASSERT_USER((*it)->Succs.size() == 1, "fcall basic block cannot have more than 1 successor");
            G4_BB* afterFCallBB = (*it)->Succs.front();
            G4_Declare* dcl = builder.kernel.fg.fcallToPseudoDclMap[callInst->asCFInst()].VCA->getRegVar()->getDeclare();
            LSLiveRange* lr = gra.getLSLR(dcl);

            for (auto it = lr->getForbiddenGRF().begin(); it != lr->getForbiddenGRF().end(); ++it)
            {
                unsigned int regNum = (*it);
                unsigned int startCalleeSave = 1;
                unsigned int endCalleeSave = startCalleeSave + builder.kernel.getCallerSaveLastGRF();

                if (regNum >= startCalleeSave && regNum <= endCalleeSave)
                {
                    callerSaveRegs[regNum] = true;
                    callerSaveRegCount++;
                }
            }

            //ret
            for (auto it = lr->getRetGRFs().begin(); it != lr->getRetGRFs().end(); ++it)
            {
                unsigned int regNum = (*it);
                unsigned int startCalleeSave = 1;
                unsigned int endCalleeSave = startCalleeSave + builder.kernel.getCallerSaveLastGRF();

                if (regNum >= startCalleeSave && regNum <= endCalleeSave)
                {
                    retRegs[regNum] = true;
                }
            }

            OptimizeActiveRegsFootprint(callerSaveRegs, retRegs);

            unsigned callerSaveRegsWritten = 0;
            for (std::vector<bool>::iterator vit = callerSaveRegs.begin(), vitend = callerSaveRegs.end();
                vit != vitend;
                vit++)
                callerSaveRegsWritten += ((*vit) ? 1 : 0);

            INST_LIST_ITER insertSaveIt = (*it)->end();
            --insertSaveIt, --insertSaveIt;
            while ((*insertSaveIt)->isPseudoKill())
            {
                --insertSaveIt;
            }
            MUST_BE_TRUE((*insertSaveIt)->isCallerSave(), ERROR_REGALLOC);
            INST_LIST_ITER rmIt = insertSaveIt;
            if (insertSaveIt == (*it)->begin())
            {
                insertSaveIt = (*it)->end();
            }

            if (insertSaveIt != (*it)->end())
            {
                ++insertSaveIt;
            }
            else
            {
                insertSaveIt = (*it)->begin();
            }
            if (callerSaveRegCount > 0)
            {
                if (builder.kernel.getOption(vISA_GenerateDebugInfo))
                {
                    builder.kernel.getKernelDebugInfo()->clearOldInstList();
                    builder.kernel.getKernelDebugInfo()->setOldInstList
                    ((*it));
                }

                saveActiveRegs(callerSaveRegs, 0, builder.kernel.fg.callerSaveAreaOffset,
                    (*it), insertSaveIt);

                if (builder.kernel.getOption(vISA_GenerateDebugInfo))
                {
                    auto deltaInstList = builder.kernel.getKernelDebugInfo()->getDeltaInstructions
                    ((*it));
                    auto fcallInst = (*it)->back();
                    for (auto it : deltaInstList)
                    {
                        builder.kernel.getKernelDebugInfo()->addCallerSaveInst(fcallInst, it);
                    }
                }
            }
            (*it)->erase(rmIt);
            INST_LIST_ITER insertRestIt = afterFCallBB->begin();
            for (; !(*insertRestIt)->isCallerRestore(); ++insertRestIt);
            if (callerSaveRegCount > 0)
            {
                if (builder.kernel.getOption(vISA_GenerateDebugInfo))
                {
                    builder.kernel.getKernelDebugInfo()->clearOldInstList();
                    builder.kernel.getKernelDebugInfo()->setOldInstList
                    (afterFCallBB);
                }

                restoreActiveRegs(callerSaveRegs, 0, builder.kernel.fg.callerSaveAreaOffset,
                    afterFCallBB, insertRestIt);

                if (builder.kernel.getOption(vISA_GenerateDebugInfo))
                {
                    auto deltaInsts = builder.kernel.getKernelDebugInfo()->getDeltaInstructions
                    (afterFCallBB);
                    auto fcallInst = (*it)->back();
                    for (auto it : deltaInsts)
                    {
                        builder.kernel.getKernelDebugInfo()->addCallerRestoreInst
                        (fcallInst, it);
                    }
                }
            }
            afterFCallBB->erase(insertRestIt);

            maxCallerSaveSize = std::max(maxCallerSaveSize, callerSaveRegsWritten * getGRFSize());

            if (builder.kernel.getOption(vISA_OptReport))
            {
                std::ofstream optreport;
                getOptReportStream(optreport, builder.kernel.getOptions());
                optreport << "Caller save size: " << callerSaveRegCount * getGRFSize() <<
                    " bytes for fcall at cisa id " <<
                    (*it)->back()->getCISAOff() << std::endl;
                closeOptReportStream(optreport);
            }
        }
    }

    auto byteOffset = builder.kernel.fg.callerSaveAreaOffset * 16 + maxCallerSaveSize;
    builder.kernel.fg.frameSizeInOWord = ROUND(byteOffset, 64) / 16;

    builder.instList.clear();
}

void LinearScanRA::addGenxMainStackSetupCode()
{
    uint32_t fpInitVal = (uint32_t)kernel.getInt32KernelAttr(Attributes::ATTR_SpillMemOffset);
    // FIXME: a potential failure here is that frameSizeInOword is already the offset based on
    // GlobalSratchOffset, which is the value of fpInitVal. So below we generate code to do
    // SP = fpInitVal + frameSize, which does not make sense. It is correct now since when there's stack call,
    // IGC will not use scratch, so fpInitVal will be 0.
    unsigned frameSize = builder.kernel.fg.frameSizeInOWord;
    G4_Declare* framePtr = builder.kernel.fg.framePtrDcl;
    G4_Declare* stackPtr = builder.kernel.fg.stackPtrDcl;

    auto entryBB = builder.kernel.fg.getEntryBB();
    auto insertIt = std::find_if(entryBB->begin(), entryBB->end(), [](G4_INST* inst) { return !inst->isLabel(); });
    //
    // FP = spillMemOffset
    //
    {
        G4_DstRegRegion* dst = builder.createDst(framePtr->getRegVar(), 0, 0, 1, Type_UD);
        G4_Imm * src = builder.createImm(fpInitVal, Type_UD);
        G4_INST* fpInst = builder.createMov(g4::SIMD1, dst, src, InstOpt_WriteEnable, false);
        insertIt = entryBB->insertBefore(insertIt, fpInst);

        if (builder.kernel.getOption(vISA_GenerateDebugInfo))
        {
            builder.kernel.getKernelDebugInfo()->setBEFPSetupInst(fpInst);
            builder.kernel.getKernelDebugInfo()->setFrameSize(frameSize * 16);
        }
    }
    //
    // SP = FP + FrameSize (overflow-area offset + overflow-area size)
    //
    {
        G4_DstRegRegion* dst = builder.createDst(stackPtr->getRegVar(), 0, 0, 1, Type_UD);
        G4_Imm * src = builder.createImm(fpInitVal + frameSize, Type_UD);
        G4_INST* spIncInst = builder.createMov(g4::SIMD1, dst, src, InstOpt_WriteEnable, false);
        entryBB->insertBefore(++insertIt, spIncInst);
    }

    if (builder.kernel.getOption(vISA_OptReport))
    {
        std::ofstream optreport;
        getOptReportStream(optreport, builder.kernel.getOptions());
        optreport << "Total frame size: " << frameSize * 16 << " bytes" << std::endl;
        closeOptReportStream(optreport);
    }
}

//
// Add code to setup the stack frame in callee.
//
void LinearScanRA::addCalleeStackSetupCode()
{
    int frameSize = (int)builder.kernel.fg.frameSizeInOWord;
    G4_Declare* framePtr = builder.kernel.fg.framePtrDcl;
    G4_Declare* stackPtr = builder.kernel.fg.stackPtrDcl;

    if (frameSize == 0)
    {
        // Remove store/restore_be_fp because a new frame is not needed
        G4_BB* entryBB = builder.kernel.fg.getEntryBB();
        auto insertIt = std::find(entryBB->begin(), entryBB->end(), gra.getSaveBE_FPInst());
        builder.kernel.fg.getEntryBB()->erase(insertIt);

        insertIt = builder.kernel.fg.getUniqueReturnBlock()->end();
        for (--insertIt; (*insertIt) != gra.getRestoreBE_FPInst(); --insertIt)
        {   /* void */
        };
        builder.kernel.fg.getUniqueReturnBlock()->erase(insertIt);

        return;
    }
    //
    // BE_FP = BE_SP
    // BE_SP += FrameSize
    //
    {
        G4_DstRegRegion* dst = builder.createDst(stackPtr->getRegVar(), 0, 0, 1, Type_UD);
        G4_DstRegRegion* fp_dst = builder.createDst(framePtr->getRegVar(), 0, 0, 1, Type_UD);
        const RegionDesc* rDesc = builder.getRegionScalar();
        G4_Operand* src0 = builder.createSrcRegRegion(
            Mod_src_undef, Direct, stackPtr->getRegVar(), 0, 0, rDesc, Type_UD);
        G4_Operand* sp_src = builder.createSrcRegRegion(Mod_src_undef, Direct, stackPtr->getRegVar(), 0, 0, rDesc, Type_UD);
        G4_Imm * src1 = builder.createImm(frameSize, Type_UD);
        auto createBEFP = builder.createMov(g4::SIMD1, fp_dst, sp_src, InstOpt_WriteEnable, false);
        auto addInst = builder.createBinOp(G4_add, g4::SIMD1,
            dst, src0, src1, InstOpt_WriteEnable, false);
        G4_BB* entryBB = builder.kernel.fg.getEntryBB();
        auto insertIt = std::find(entryBB->begin(), entryBB->end(), gra.getSaveBE_FPInst());
        MUST_BE_TRUE(insertIt != entryBB->end(), "Can't find BE_FP store inst");

        if (builder.kernel.getOption(vISA_GenerateDebugInfo))
        {
            G4_INST* callerFPSave = (*insertIt);
            builder.kernel.getKernelDebugInfo()->setBEFPSetupInst(createBEFP);
            builder.kernel.getKernelDebugInfo()->setCallerBEFPSaveInst(callerFPSave);
            builder.kernel.getKernelDebugInfo()->setFrameSize(frameSize * 16);
        }

        insertIt++;
        entryBB->insertBefore(insertIt, createBEFP);
        entryBB->insertBefore(insertIt, addInst);
    }
    //
    // BE_SP = BE_FP
    // BE_FP = oldFP (dst of pseudo_restore_be_fp)
    //
    {
        G4_DstRegRegion* sp_dst = builder.createDst(stackPtr->getRegVar(), 0, 0, 1, Type_UD);
        const RegionDesc* rDesc = builder.getRegionScalar();
        G4_Operand* fp_src = builder.createSrcRegRegion(
            Mod_src_undef, Direct, framePtr->getRegVar(), 0, 0, rDesc, Type_UD);
        G4_INST* spRestore = builder.createMov(g4::SIMD1, sp_dst, fp_src, InstOpt_WriteEnable, false);
        INST_LIST_ITER insertIt = builder.kernel.fg.getUniqueReturnBlock()->end();
        for (--insertIt; (*insertIt) != gra.getRestoreBE_FPInst(); --insertIt);

        if (builder.kernel.getOption(vISA_GenerateDebugInfo))
        {
            G4_INST* callerFPRestore = (*insertIt);
            builder.kernel.getKernelDebugInfo()->setCallerSPRestoreInst(spRestore);
            builder.kernel.getKernelDebugInfo()->setCallerBEFPRestoreInst(callerFPRestore);
        }
        builder.kernel.fg.getUniqueReturnBlock()->insertBefore(insertIt, spRestore);
    }
    builder.instList.clear();

    if (builder.kernel.getOption(vISA_OptReport))
    {
        std::ofstream optreport;
        getOptReportStream(optreport, builder.kernel.getOptions());
        optreport << std::endl << "Total frame size: "
            << frameSize * 16 << " bytes" << std::endl;
        closeOptReportStream(optreport);
    }
}

void LinearScanRA::stackCallProlog()
{
    // mov (8) r126.0<1>:ud    r0.0<8;8,1>:ud
    // This sets up the header for oword block r/w used for caller/callee-save
    // ToDo: check if this move is actually necessary
    auto dstRgn = builder.Create_Dst_Opnd_From_Dcl(builder.kernel.fg.scratchRegDcl, 1);
    auto srcRgn = builder.Create_Src_Opnd_From_Dcl(builder.getBuiltinR0(), builder.getRegionStride1());

    G4_INST* mov = builder.createMov(G4_ExecSize(numEltPerGRF(Type_UD)), dstRgn, srcRgn, InstOpt_WriteEnable, false);

    G4_BB* entryBB = builder.kernel.fg.getEntryBB();
    auto iter = std::find_if(entryBB->begin(), entryBB->end(), [](G4_INST* inst) { return !inst->isLabel(); });
    entryBB->insertBefore(iter, mov);
}

//
// Add callee save/restore code at stack call function entry/exit.
//
void LinearScanRA::addCalleeSaveRestoreCode()
{
    unsigned int callerSaveNumGRF = builder.kernel.getCallerSaveLastGRF() + 1;
    // Determine the callee-save registers.
    unsigned int numCalleeSaveRegs = builder.kernel.getNumCalleeSaveRegs();
    std::vector<bool> calleeSaveRegs(numCalleeSaveRegs, false);
    unsigned calleeSaveRegCount = 0;

    G4_Declare *dcl = builder.kernel.fg.pseudoVCEDcl;
    LSLiveRange* lr = gra.getLSLR(dcl);

    for (auto it = lr->getForbiddenGRF().begin(); it != lr->getForbiddenGRF().end(); ++it)
    {
        unsigned int regNum = (*it);
        unsigned int startCalleeSave = builder.kernel.getCallerSaveLastGRF() + 1;
        unsigned int endCalleeSave = startCalleeSave + builder.kernel.getNumCalleeSaveRegs() - 1;

        if (regNum >= startCalleeSave && regNum <= endCalleeSave)
        {
            calleeSaveRegs[regNum] = true;
            calleeSaveRegCount++;
        }
    }

    OptimizeActiveRegsFootprint(calleeSaveRegs);
    unsigned calleeSaveRegsWritten = 0;
    for (std::vector<bool>::iterator vit = calleeSaveRegs.begin(), vitend = calleeSaveRegs.end();
        vit != vitend;
        vit++)
        calleeSaveRegsWritten += ((*vit) ? 1 : 0);

    INST_LIST_ITER insertSaveIt = builder.kernel.fg.getEntryBB()->end();
    for (--insertSaveIt; !(*insertSaveIt)->isCalleeSave(); --insertSaveIt);
    if (calleeSaveRegCount > 0)
    {
        if (builder.kernel.getOption(vISA_GenerateDebugInfo))
        {
            // Store old inst list so we can separate callee save
            // instructions that get inserted.
            builder.kernel.getKernelDebugInfo()->clearOldInstList();
            builder.kernel.getKernelDebugInfo()->setOldInstList
            (builder.kernel.fg.getEntryBB());
        }
        saveActiveRegs(calleeSaveRegs, callerSaveNumGRF, builder.kernel.fg.calleeSaveAreaOffset,
            builder.kernel.fg.getEntryBB(), insertSaveIt);

        if (builder.kernel.getOption(vISA_GenerateDebugInfo))
        {
            // Delta of oldInstList and current instList are all
            // callee save instructions.
            auto instList = builder.kernel.getKernelDebugInfo()->getDeltaInstructions
            (builder.kernel.fg.getEntryBB());
            for (auto inst : instList)
            {
                builder.kernel.getKernelDebugInfo()->addCalleeSaveInst(inst);
            }
        }
    }
    builder.kernel.fg.getEntryBB()->erase(insertSaveIt);
    INST_LIST_ITER insertRestIt = builder.kernel.fg.getUniqueReturnBlock()->end();
    for (--insertRestIt; !(*insertRestIt)->isCalleeRestore(); --insertRestIt);
    INST_LIST_ITER eraseIt = insertRestIt++;
    if (calleeSaveRegCount > 0)
    {
        if (builder.kernel.getOption(vISA_GenerateDebugInfo))
        {
            // Store old inst list so we can separate callee save
            // instructions that get inserted.
            builder.kernel.getKernelDebugInfo()->clearOldInstList();
            builder.kernel.getKernelDebugInfo()->setOldInstList
            (builder.kernel.fg.getUniqueReturnBlock());
        }

        restoreActiveRegs(calleeSaveRegs, callerSaveNumGRF, builder.kernel.fg.calleeSaveAreaOffset,
            builder.kernel.fg.getUniqueReturnBlock(), insertRestIt);

        if (builder.kernel.getOption(vISA_GenerateDebugInfo))
        {
            auto instList = builder.kernel.getKernelDebugInfo()->getDeltaInstructions
            (builder.kernel.fg.getUniqueReturnBlock());
            for (auto inst : instList)
            {
                builder.kernel.getKernelDebugInfo()->addCalleeRestoreInst(inst);
            }
        }
    }
    builder.kernel.fg.getUniqueReturnBlock()->erase(eraseIt);

    builder.instList.clear();

    // caller-save starts after callee-save and is 64-byte aligned
    auto byteOffset = builder.kernel.fg.calleeSaveAreaOffset * 16 + calleeSaveRegsWritten * getGRFSize();
    builder.kernel.fg.callerSaveAreaOffset = ROUND(byteOffset, 64) / 16;
}


void LinearScanRA::addSaveRestoreCode(unsigned localSpillAreaOwordSize)
{
    if (builder.getIsKernel())
    {
        builder.kernel.fg.callerSaveAreaOffset = localSpillAreaOwordSize;
    }
    else
    {
        builder.kernel.fg.calleeSaveAreaOffset = localSpillAreaOwordSize;
        addCalleeSaveRestoreCode();
    }
    addCallerSaveRestoreCode();

    if (builder.getIsKernel())
    {
        addGenxMainStackSetupCode();
    }
    else
    {
        addCalleeStackSetupCode();
    }
    stackCallProlog();

    builder.instList.clear();

    if (kernel.getOption(vISA_GenerateDebugInfo))
    {
        kernel.getKernelDebugInfo()->computeGRFToStackOffset();
    }
}

bool LinearScanRA::linearScanRA()
{
    LivenessAnalysis liveAnalysis(gra, G4_GRF | G4_INPUT);
    liveAnalysis.computeLiveness();

    std::map<unsigned, std::list<vISA::G4_BB*>> regions;
    std::map<unsigned, std::list<vISA::G4_BB*>>::iterator regionIt;
    G4_BB* entryBB = nullptr;
    for (auto bb : kernel.fg)
    {
        if (entryBB == nullptr || //bb->getBBType() == G4_BB_INIT_TYPE ||
            bb->getId() == 0)
        {
            entryBB = bb;
        }
        regions[entryBB->getId()].push_back(bb);
    }

    std::list<LSLiveRange*> spillLRs;
    int iterator = 0;
    uint32_t GRFSpillFillCount = 0;
    do {
        spillLRs.clear();

        std::vector<LSLiveRange*> eotLiveIntervals;
        inputIntervals.clear();
        setLexicalID(false);

#ifdef DEBUG_VERBOSE_ON
        COUT_ERROR << "=============  ITERATION: " << iterator << "============" << std::endl;
#endif

        for (regionIt = regions.begin(); regionIt != regions.end(); regionIt++)
        {
            //Input
            PhyRegsLocalRA initPregs = (*pregs);
            inputIntervals.clear();

#ifdef DEBUG_VERBOSE_ON
            COUT_ERROR << "===== REGION: " << (*regionIt).first << "============" << std::endl;
#endif
            calculateInputIntervalsGlobal(&liveAnalysis, initPregs, (*regionIt).second);
#ifdef DEBUG_VERBOSE_ON
            printInputLiveIntervalsGlobal();
#endif

            globalLiveIntervals.clear();
            preAssignedLiveIntervals.clear();
            eotLiveIntervals.clear();
            regionID = (*regionIt).first;
            unsigned latestLexID = 0;

            for (auto bb : (*regionIt).second)
            {
                calculateLiveIntervalsGlobal(&liveAnalysis, bb, globalLiveIntervals, eotLiveIntervals);
                latestLexID = bb->back()->getLexicalId() * 2;
            }
#ifdef DEBUG_VERBOSE_ON
            printLiveIntervals(globalLiveIntervals);
#endif

            if (eotLiveIntervals.size())
            {
                assignEOTLiveRanges(builder, eotLiveIntervals);
                for (auto lr : eotLiveIntervals)
                {
                    preAssignedLiveIntervals.push_back(lr);
                }
            }

            PhyRegsManager pregManager(initPregs, doBCR);
            globalLinearScan ra(gra, &liveAnalysis, globalLiveIntervals, &preAssignedLiveIntervals, inputIntervals, pregManager,
                mem, numRegLRA, numRowsEOT, latestLexID,
                doBCR, highInternalConflict);
            if (!ra.runLinearScan(builder, globalLiveIntervals, spillLRs))
            {
                undoLinearScanRAAssignments();
                return false;
            }
        }

        if (spillLRs.size())
        {
            SpillManagerGRF spillGRF(gra,
                nextSpillOffset,
                liveAnalysis.getNumSelectedVar(),
                &liveAnalysis,
                &spillLRs,
                true);

            spillGRF.spillLiveRanges(&kernel);
            nextSpillOffset = spillGRF.getNextOffset();;

            for (auto lr : spillLRs)
            {
                GRFSpillFillCount += gra.getNumRefs(lr->getTopDcl());
            }

            // update jit metadata information for spill
            if (auto jitInfo = builder.getJitInfo())
            {
                jitInfo->isSpill = nextSpillOffset > 0;
                jitInfo->hasStackcalls = kernel.fg.getHasStackCalls();

                if (builder.kernel.fg.frameSizeInOWord != 0) {
                    // jitInfo->spillMemUsed is the entire visa stack size. Consider the caller/callee
                    // save size if having caller/callee save
                    // globalScratchOffset in unit of byte, others in Oword
                    //
                    //                               vISA stack
                    //  globalScratchOffset     -> ---------------------
                    //  FIXME: should be 0-based   |  spill            |
                    //                             |                   |
                    //  calleeSaveAreaOffset    -> ---------------------
                    //                             |  callee save      |
                    //  callerSaveAreaOffset    -> ---------------------
                    //                             |  caller save      |
                    //  paramOverflowAreaOffset -> ---------------------
                    jitInfo->spillMemUsed =
                        builder.kernel.fg.frameSizeInOWord * 16;

                    // reserve spillMemUsed #bytes before 8kb boundary
                    kernel.getGTPinData()->setScratchNextFree(8 * 1024 - kernel.getGTPinData()->getNumBytesScratchUse());
                }
                else {
                    jitInfo->spillMemUsed = nextSpillOffset;
                    kernel.getGTPinData()->setScratchNextFree(nextSpillOffset);
                }
                jitInfo->numGRFSpillFill = GRFSpillFillCount;
            }

            undoLinearScanRAAssignments();
            numRowsEOT = 0;
            linearScanMarkReferences(numRowsEOT);
        }

        if (builder.getOption(vISA_RATrace))
        {
            std::cout << "\titeration: " << iterator << "\n";
            std::cout << "\t\tnextSpillOffset: " << nextSpillOffset << "\n";
            std::cout << "\t\tGRFSpillFillCount: " << GRFSpillFillCount << "\n";
        }
        //kernel.dump();
        iterator++;
    } while (spillLRs.size() && iterator < 10);

    if (spillLRs.size())
    {
        spillLRs.clear();
        return false;
    }

    if (kernel.fg.getHasStackCalls() || kernel.fg.getIsStackCallFunc())
    {
        unsigned localSpillAreaOwordSize = ROUND(nextSpillOffset, 64) / 16;
        addSaveRestoreCode(localSpillAreaOwordSize);
    }

    return true;
}

bool LinearScanRA::doLinearScanRA()
{
    if (builder.getOption(vISA_RATrace))
    {
        std::cout << "--Global linear Scan RA--\n";
    }

    bool reduceBCInRR = false;
    bool reduceBCInTAandFF = false;
    if (builder.getOption(vISA_LocalBankConflictReduction) && builder.hasBankCollision())
    {
        reduceBCInRR = bc.setupBankConflictsForKernel(false, reduceBCInTAandFF, numRegLRA, highInternalConflict);
    }

    if (reduceBCInTAandFF || reduceBCInRR)
    {
        doBCR = true;
    }

    //Initial pregs which will be used in the preRAAnalysis
    PhyRegsLocalRA phyRegs(&builder, kernel.getNumRegTotal());
    pregs = &phyRegs;
    preRAAnalysis();

    bool success = linearScanRA();

    if (success)
    {
        if (doBCR)
        {
            kernel.setRAType(RA_Type::GLOBAL_LINEAR_SCAN_BC_RA);
        }
        else
        {
            kernel.setRAType(RA_Type::GLOBAL_LINEAR_SCAN_RA);
        }
    }

    return success;
}

void LinearScanRA::undoLinearScanRAAssignments()
{
    // Undo all assignments made by local RA
    for (auto dcl : kernel.Declares)
    {
        LSLiveRange* lr = gra.getLSLR(dcl);
        if (lr != NULL)
        {
            if (lr->getAssigned() == true)
            {
                // Undo the assignment
                lr->setAssigned(false);
                if (lr->getTopDcl()->getRegFile() != G4_INPUT &&
                    !lr->getPreAssigned())
                {
                    lr->getTopDcl()->getRegVar()->resetPhyReg();
                }
                lr->resetPhyReg();
            }

            lr->setFirstRef(NULL, 0);
            lr->setLastRef(NULL, 0);
            lr->clearForbiddenGRF();
            lr->setRegionID(-1);
        }
        gra.resetLSLR(dcl);
    }
}

void LinearScanRA::setPreAssignedLR(LSLiveRange* lr, std::vector<LSLiveRange*> & preAssignedLiveIntervals)
{
    int subreg = 0;
    G4_VarBase* reg = lr->getPhyReg(subreg);
    unsigned regnum = lr->getTopDcl()->getRegVar()->getPhyReg()->asGreg()->getRegNum();
    if (reg == nullptr)
    {
        unsigned int subReg = lr->getTopDcl()->getRegVar()->getPhyRegOff();
        unsigned int subRegInWord = subReg * lr->getTopDcl()->getRegVar()->getDeclare()->getElemSize() / 2;

        lr->setPhyReg(builder.phyregpool.getGreg(regnum), subRegInWord);
    }
    lr->setAssigned(true);
    lr->setUseUnAvailableReg(isUseUnAvailableRegister(regnum, lr->getTopDcl()->getNumRows()));

    //Insert into preAssgined live intervals
    //If the pre-assigned register is marked as unavailable, not join the live range
    //FIXME: What about partial overlap?
    if (std::find(preAssignedLiveIntervals.begin(), preAssignedLiveIntervals.end(), lr) == preAssignedLiveIntervals.end())
    {
        preAssignedLiveIntervals.push_back(lr);
    }

    return;
}

void LinearScanRA::setDstReferences(G4_BB* bb, INST_LIST_ITER inst_it, G4_Declare *dcl, std::vector<LSLiveRange*>& liveIntervals, std::vector<LSLiveRange*>& eotLiveIntervals)
{
    G4_INST* curInst = (*inst_it);
    LSLiveRange* lr = gra.getLSLR(dcl);

    if (!lr && dcl->getRegFile() == G4_GRF) //The new variables generated by spill/fill, mark reference should handle it
    {
        assert(0);
    }

    if (lr == nullptr ||
        dcl->getRegFile() == G4_INPUT ||
        (lr->isGRFRegAssigned() && (!dcl->getRegVar()->isGreg())))  //ARF
    {
        return;
    }

    // Check whether local LR is a candidate
    if (lr->isGRFRegAssigned() == false)
    {
        if (lr->getRegionID() != regionID)
        {
            liveIntervals.push_back(lr);
            lr->setRegionID(regionID);
        }

        unsigned int startIdx;
        if (lr->getFirstRef(startIdx) == NULL && startIdx == 0)
        {
            lr->setFirstRef(curInst, curInst->getLexicalId() * 2);
        }
        lr->setLastRef(curInst, curInst->getLexicalId() * 2);
    }
    else if (dcl->getRegVar()->getPhyReg()->isGreg()) //Assigned already and is GRF
    { //Such as stack call varaibles
        unsigned int startIdx;
        if (lr->getRegionID() != regionID)
        {
            liveIntervals.push_back(lr);
            lr->setRegionID(regionID);

            //Mark live range as assigned
            setPreAssignedLR(lr, preAssignedLiveIntervals);
        }

        if (lr->getFirstRef(startIdx) == NULL && startIdx == 0)
        {
            lr->setFirstRef(curInst, curInst->getLexicalId() * 2);
        }
        lr->setLastRef(curInst, curInst->getLexicalId() * 2);
    }

    if (lr->isEOT() && std::find(eotLiveIntervals.begin(), eotLiveIntervals.end(), lr) == eotLiveIntervals.end())
    {
        eotLiveIntervals.push_back(lr);
    }

    return;
}

void LinearScanRA::setSrcReferences(G4_BB* bb, INST_LIST_ITER inst_it, int srcIdx, G4_Declare* dcl, std::vector<LSLiveRange*>& liveIntervals, std::vector<LSLiveRange*>& eotLiveIntervals)
{
    G4_INST* curInst = (*inst_it);
    LSLiveRange* lr = gra.getLSLR(dcl);

    if (lr == nullptr ||
        dcl->getRegFile() == G4_INPUT ||
        (lr->isGRFRegAssigned() && (!dcl->getRegVar()->isGreg())))  //ARF
    {
        return;
    }

    if (lr->getRegionID() != regionID)
    {
        liveIntervals.push_back(lr);
        lr->setRegionID(regionID);
        gra.addUndefinedDcl(dcl);
    }

    unsigned int startIdx;
    if (lr->getFirstRef(startIdx) == NULL && startIdx == 0)
    {  //Since we scan from front to end, not referenced before means not defined.
        lr->setFirstRef(curInst, curInst->getLexicalId() * 2);
    }

    lr->setLastRef(curInst, curInst->getLexicalId() * 2);

    if ((builder.WaDisableSendSrcDstOverlap() &&
        ((curInst->isSend() && srcIdx == 0) ||
            (curInst->isSplitSend() && srcIdx == 1)))
        || (builder.avoidDstSrcOverlap() && curInst->getDst() != NULL && hasDstSrcOverlapPotential(curInst->getDst(), curInst->getSrc(srcIdx)->asSrcRegRegion()))
        )
    {
        lr->setLastRef(curInst, curInst->getLexicalId() * 2 + 1);
    }

    if (lr->isEOT() && std::find(eotLiveIntervals.begin(), eotLiveIntervals.end(), lr) == eotLiveIntervals.end())
    {
        eotLiveIntervals.push_back(lr);
    }

    return;
}

void LinearScanRA::generateInputIntervals(G4_Declare *topdcl, G4_INST* inst, std::vector<uint32_t> &inputRegLastRef, PhyRegsLocalRA& initPregs, bool avoidSameInstOverlap)
{
    unsigned int instID = inst->getLexicalId();
    G4_RegVar* var = topdcl->getRegVar();
    unsigned int regNum = var->getPhyReg()->asGreg()->getRegNum();
    unsigned int regOff = var->getPhyRegOff();
    unsigned int idx = regNum * numEltPerGRF(Type_UW) +
        (regOff * G4_Type_Table[topdcl->getElemType()].byteSize) / G4_WSIZE + topdcl->getWordSize() - 1;

    unsigned int numWords = topdcl->getWordSize();
    for (int i = numWords - 1; i >= 0; --i, --idx)
    {
        if ((inputRegLastRef[idx] == UINT_MAX || inputRegLastRef[idx] < instID) &&
            initPregs.isGRFAvailable(idx / numEltPerGRF(Type_UW)))
        {
            inputRegLastRef[idx] = instID;
            if (avoidSameInstOverlap)
            {
                inputIntervals.push_front(new (mem)LSInputLiveRange(idx, instID * 2 + 1));
            }
            else
            {
                inputIntervals.push_front(new (mem)LSInputLiveRange(idx, instID * 2));
            }

            if (kernel.getOptions()->getOption(vISA_GenerateDebugInfo))
            {
                updateDebugInfo(kernel, topdcl, 0, inst->getCISAOff());
            }
        }
    }

    initPregs.markPhyRegs(topdcl);

    return;
}

// Generate the input intervals for current BB.
// The input live ranges either live through current BB or killed by current BB.
// So, it's enough we check the live out of the BB and the BB it's self
void LinearScanRA::calculateInputIntervalsGlobal(LivenessAnalysis* l, PhyRegsLocalRA &initPregs, std::list<vISA::G4_BB*> &bbList)
{
    int numGRF = kernel.getNumRegTotal();
    std::vector<uint32_t> inputRegLastRef(numGRF * numEltPerGRF(Type_UW), UINT_MAX);

    for (BB_LIST_RITER bb_it = bbList.rbegin(), bb_rend = bbList.rend();
        bb_it != bb_rend;
        bb_it++)
    {
        G4_BB* bb = (*bb_it);

        //@ the end of BB
        for (auto dcl : globalDeclares)
        {
            if (dcl->getAliasDeclare() != NULL ||
                dcl->isSpilled())
                continue;

            if (dcl->getRegFile() == G4_INPUT &&
                !(dcl->getRegVar()->getPhyReg()->isAreg()) && //Filter out the architecture registers
                dcl->isOutput() == false &&  //Input and out should be marked as unavailable
                !builder.isPreDefArg(dcl) &&  //Not stack call associated variables
                l->isLiveAtExit(bb, dcl->getRegVar()->getId()))
            {
                MUST_BE_TRUE(dcl->getRegVar()->isPhyRegAssigned(), "Input variable has no pre-assigned physical register");
                generateInputIntervals(dcl, bb->getInstList().back(), inputRegLastRef, initPregs, false);
            }
        }

        //@BB
        for (INST_LIST_RITER inst_it = bb->rbegin(), inst_rend = bb->rend();
            inst_it != inst_rend;
            inst_it++)
        {
            G4_INST* curInst = (*inst_it);

            G4_Declare* topdcl = NULL;
            LSLiveRange* lr = NULL;

            // scan dst operand (may be unnecessary but added for safety)
            if (curInst->getDst() != NULL)
            {
                // Scan dst
                G4_DstRegRegion* dst = curInst->getDst();

                topdcl = GetTopDclFromRegRegion(dst);

                if (topdcl && (lr = gra.getLSLR(topdcl)))
                {
                    if (topdcl->getRegFile() == G4_INPUT &&
                        !(dst->isAreg()) &&
                        topdcl->isOutput() == false &&
                        lr->hasIndirectAccess() == false &&
                        !builder.isPreDefArg(topdcl))
                    {
                        MUST_BE_TRUE(lr->isGRFRegAssigned(), "Input variable has no pre-assigned physical register");
                        generateInputIntervals(topdcl, curInst, inputRegLastRef, initPregs, false);
                    }
                }
            }

            // Scan src operands
            for (int i = 0, nSrcs = curInst->getNumSrc(); i < nSrcs; i++)
            {
                G4_Operand* src = curInst->getSrc(i);

                if (src == nullptr || src->isNullReg())
                {
                    continue;
                }

                if (src->getTopDcl())
                {
                    topdcl = GetTopDclFromRegRegion(src);

                    if (topdcl && (lr = gra.getLSLR(topdcl)))
                    {
                        // Check whether it is input
                        if (topdcl->getRegFile() == G4_INPUT &&
                            !(src->isAreg()) &&
                            topdcl->isOutput() == false &&
                            lr->hasIndirectAccess() == false &&
                            !builder.isPreDefArg(topdcl))
                        {
                            MUST_BE_TRUE(lr->isGRFRegAssigned(), "Input variable has no pre-assigned physical register");
                            if (builder.avoidDstSrcOverlap() &&
                                curInst->getDst() != NULL &&
                                hasDstSrcOverlapPotential(curInst->getDst(), src->asSrcRegRegion()))
                            {
                                generateInputIntervals(topdcl, curInst, inputRegLastRef, initPregs, true);
                            }
                            else
                            {
                                generateInputIntervals(topdcl, curInst, inputRegLastRef, initPregs, false);
                            }
                        }
                    }
                }
                else if (src->isAddrExp())
                {
                    G4_AddrExp* addrExp = src->asAddrExp();

                    topdcl = addrExp->getRegVar()->getDeclare();
                    while (topdcl->getAliasDeclare() != NULL)
                        topdcl = topdcl->getAliasDeclare();

                    MUST_BE_TRUE(topdcl != NULL, "Top dcl was null for addr exp opnd");

                    LSLiveRange* lr = gra.getLSLR(topdcl);
                    if (topdcl->getRegFile() == G4_INPUT &&
                        !(src->isAreg()) &&
                        topdcl->isOutput() == false &&
                        !builder.isPreDefArg(topdcl))
                    {
                        MUST_BE_TRUE(lr->isGRFRegAssigned(), "Input variable has no pre-assigned physical register");
                        MUST_BE_TRUE(lr->hasIndirectAccess(), "Should be indirectly accessed Input variable");
                        generateInputIntervals(topdcl, curInst, inputRegLastRef, initPregs, false);
                    }
                }
            }
        }
    }

    return;
}

//
//@ the entry of BB
//
void LinearScanRA::calculateLiveInIntervals(LivenessAnalysis* l, G4_BB* bb, std::vector<LSLiveRange*>& liveIntervals)
{
    //FIXME: The complexity is "block_num * declare_num"
    std::vector<LSLiveRange*> preAssignedLiveIntervals;

    for (auto dcl: globalDeclares)
    {
        if (dcl->getAliasDeclare() != NULL ||
            dcl->getRegFile() == G4_INPUT ||
            dcl->isSpilled())
        {
            continue;
        }

        LSLiveRange* lr = gra.getLSLR(dcl);
        if (lr &&
            l->isLiveAtEntry(bb, dcl->getRegVar()->getId()))
        {
            if (lr->getRegionID() != regionID)
            {
                if (lr->isGRFRegAssigned() && dcl->getRegVar()->isGreg())
                {
                    setPreAssignedLR(lr, preAssignedLiveIntervals);
                }
                else
                {
                    liveIntervals.push_back(lr);
                }
                lr->setRegionID(regionID);
            }

            unsigned curIdx = 0;
            if (lr->getFirstRef(curIdx) == NULL && curIdx == 0) //not referenced before, assigned or not assigned?
            {
                lr->setFirstRef((*bb->begin()), (*bb->begin())->getLexicalId() * 2);
            }
        }
    }

    if (preAssignedLiveIntervals.size()) //Should happen in the entry BB
    {
        liveIntervals.insert(liveIntervals.begin(), preAssignedLiveIntervals.begin(), preAssignedLiveIntervals.end());
    }

    return;
}

void LinearScanRA::calculateCurrentBBLiveIntervals(LivenessAnalysis* l, G4_BB* bb, std::vector<LSLiveRange*>& liveIntervals, std::vector<LSLiveRange*>& eotLiveIntervals)
{
    for (INST_LIST_ITER inst_it = bb->begin(), bbend = bb->end();
        inst_it != bbend;
        inst_it++, globalIndex += 2)
    {
        G4_INST* curInst = (*inst_it);
        G4_Declare* topdcl = NULL;

        if (curInst->isPseudoKill() ||
            curInst->isLifeTimeEnd() ||
            curInst->isLabel())
        {
            continue;
        }

        // Scan srcs
        for (int i = 0, nSrcs = curInst->getNumSrc(); i < nSrcs; i++)
        {
            G4_Operand* src = curInst->getSrc(i);

            if (src == nullptr || src->isNullReg())
            {
                continue;
            }

            if (src && src->isSrcRegRegion())
            {
                if (src->asSrcRegRegion()->isIndirect())
                {
                    auto pointsToSet = l->getPointsToAnalysis().getAllInPointsTo(src->getBase()->asRegVar());
                    for (auto var : *pointsToSet)
                    {
                        if (var->isRegAllocPartaker())
                        {
                            G4_Declare* dcl = var->getDeclare();
                            while (dcl->getAliasDeclare())
                            {
                                dcl = dcl->getAliasDeclare();
                            }
                            setSrcReferences(bb, inst_it, i, dcl, liveIntervals, eotLiveIntervals);
                        }
                    }
                }
                else
                {
                    // Scan all srcs
                    topdcl = GetTopDclFromRegRegion(src);
                    if (topdcl)
                    {
                        setSrcReferences(bb, inst_it, i, topdcl, liveIntervals, eotLiveIntervals);
                    }
                }
            }
        }

        // Scan dst
        G4_DstRegRegion* dst = curInst->getDst();
        if (dst)
        {
            if (dst->isIndirect())
            {
                auto pointsToSet = l->getPointsToAnalysis().getAllInPointsTo(dst->getBase()->asRegVar());
                for (auto var : *pointsToSet)
                {
                    if (var->isRegAllocPartaker())
                    {
                        G4_Declare* dcl = var->getDeclare();
                        while (dcl->getAliasDeclare())
                        {
                            dcl = dcl->getAliasDeclare();
                        }
                        setDstReferences(bb, inst_it, dcl, liveIntervals, eotLiveIntervals);
                    }
                }
            }
            else
            {
                topdcl = GetTopDclFromRegRegion(dst);
                if (topdcl)
                {
                    setDstReferences(bb, inst_it, topdcl, liveIntervals, eotLiveIntervals);
                }
            }
        }
    }

    return;
}

void LinearScanRA::calculateLiveOutIntervals(LivenessAnalysis* l, G4_BB* bb, std::vector<LSLiveRange*>& liveIntervals)
{
    for (auto dcl : globalDeclares)
    {
        if (dcl->getAliasDeclare() != NULL ||
            dcl->getRegFile() == G4_INPUT ||
            dcl->isSpilled())
            continue;

        LSLiveRange* lr = gra.getLSLR(dcl);
        if (lr &&
            l->isLiveAtExit(bb, dcl->getRegVar()->getId()))
        {
            lr->setLastRef(bb->getInstList().back(), bb->getInstList().back()->getLexicalId() * 2 + 1);
        }
    }

    return;
}

//
// Live intervals:
// 1. not input variables
// 2. variables without assigned value: normal Intervals.
// 3. variables without assigned value, without define: wired, added by front end. Such as cmp f1.0,  v11, v11. @BB only
// 4. variables which are pre-defined with registers: such as stack call pre-defined varaibles. @BB only
// 5. variables which are pre-defined but will not be assigned with registers: such as %null.  exclusive
// 6. variables which are assigned in previuos region (BB, BBs or function, ..).  //@entry BB
// 7. live in of region: pre-assigned, or not.
// 8. live out of region: set the last reference.
//
void LinearScanRA::calculateLiveIntervalsGlobal(LivenessAnalysis* l, G4_BB* bb, std::vector<LSLiveRange*>& liveIntervals, std::vector<LSLiveRange*>& eotLiveIntervals)
{
    //@ the entry of BB
    calculateLiveInIntervals(l, bb, liveIntervals);

    //@ BB
    calculateCurrentBBLiveIntervals(l, bb, liveIntervals, eotLiveIntervals);

    //@ the exit of BB
    calculateLiveOutIntervals(l, bb, liveIntervals);

    return;
}

void LinearScanRA::printLiveIntervals(std::vector<LSLiveRange*>& liveIntervals)
{
    for (auto lr : liveIntervals)
    {
        unsigned int start, end;

        lr->getFirstRef(start);
        lr->getLastRef(end);

        std::cout << lr->getTopDcl()->getName() << "(" << start << ", " << end << ", " << lr->getTopDcl()->getByteSize() << ")";
        std::cout << std::endl;
    }

    std::cout << std::endl;
}

void LinearScanRA::printInputLiveIntervalsGlobal()
{
    COUT_ERROR << std::endl << "Input Live intervals " << std::endl;

    for (std::list<LSInputLiveRange*>::iterator it = inputIntervals.begin();
        it != inputIntervals.end();
        it++)
    {
        unsigned int regWordIdx, lrEndIdx, regNum, subRegInWord;

        LSInputLiveRange* lr = (*it);

        regWordIdx = lr->getRegWordIdx();
        regNum = regWordIdx / numEltPerGRF(Type_UW);
        subRegInWord = regWordIdx % numEltPerGRF(Type_UW);
        lrEndIdx = lr->getLrEndIdx();

        COUT_ERROR << "r" << regNum << "." << subRegInWord << " " << lrEndIdx;
        COUT_ERROR << std::endl;
    }

    COUT_ERROR << std::endl;
}

globalLinearScan::globalLinearScan(GlobalRA& g, LivenessAnalysis* l, std::vector<LSLiveRange*>& lv, std::vector<LSLiveRange*>* assignedLiveIntervals,
    std::list<LSInputLiveRange*, std_arena_based_allocator<LSInputLiveRange*>>& inputLivelIntervals,
    PhyRegsManager& pregMgr, Mem_Manager& memmgr,
    unsigned int numReg, unsigned int numEOT, unsigned int lastLexID, bool bankConflict,
    bool internalConflict)
    : gra(g)
    , builder(g.builder)
    , mem(memmgr)
    , pregManager(pregMgr)
    , liveIntervals(lv)
    , preAssignedIntervals(assignedLiveIntervals)
    , inputIntervals(inputLivelIntervals)
    , numRowsEOT(numEOT)
    , lastLexicalID(lastLexID)
    , numRegLRA(numReg)
    , doBankConflict(bankConflict)
    , highInternalConflict(internalConflict)
{
    startGRFReg = 0;
    activeGRF.resize(g.kernel.getNumRegTotal());
    for (auto lr : inputLivelIntervals)
    {
        unsigned int regnum = lr->getRegWordIdx() / numEltPerGRF(Type_UW);
        activeGRF[regnum].activeInput.push_back(lr);
    }
}

void globalLinearScan::getCalleeSaveGRF(vector<unsigned int>& regNum, G4_Kernel* kernel)
{
    unsigned int startCallerSave = kernel->calleeSaveStart();
    unsigned int endCallerSave = startCallerSave + kernel->getNumCalleeSaveRegs();

    for (auto active_it = active.begin();
        active_it != active.end();
        active_it++)
    {
        LSLiveRange* lr = (*active_it);

        G4_VarBase* op;
        int startsregnum = 0;
        op = lr->getPhyReg(startsregnum);
        unsigned startregnum = op->asGreg()->getRegNum();
        unsigned endregnum = startregnum + lr->getTopDcl()->getNumRows() - 1;

        for (unsigned i = startregnum; i <= endregnum; i++)
        {
            if (i >= startCallerSave && i <= endCallerSave)
            {
                regNum.push_back(i);
            }
        }
    }

    return;
}

void globalLinearScan::getCallerSaveGRF(vector<unsigned int>& regNum, vector<unsigned int>& retRegNum, G4_Kernel* kernel)
{
    unsigned int startCalleeSave = 1;
    unsigned int endCalleeSave = startCalleeSave + kernel->getCallerSaveLastGRF();

    for (auto active_it = active.begin();
        active_it != active.end();
        active_it++)
    {
        LSLiveRange* lr = (*active_it);
        G4_Declare* dcl = lr->getTopDcl();

        if (!builder.kernel.fg.isPseudoVCEDcl(dcl) &&
            !builder.isPreDefArg(dcl))
        {
            G4_VarBase* op;
            int startsregnum = 0;
            op = lr->getPhyReg(startsregnum);
            unsigned startregnum = op->asGreg()->getRegNum();
            unsigned endregnum = startregnum + lr->getTopDcl()->getNumRows() - 1;

            for (unsigned i = startregnum; i <= endregnum; i++)
            {
                if (i >= startCalleeSave && i <= endCalleeSave)
                {
                    if (builder.isPreDefRet(dcl))
                    {
                        retRegNum.push_back(i);
                    }
                    else
                    {
                        regNum.push_back(i);
                    }
                }
            }
        }
    }
}

bool LinearScanRA::isUseUnAvailableRegister(uint32_t startReg, uint32_t regNum)
{
    for (uint32_t i = startReg; i < startReg + regNum; ++i)
    {
        if (!pregs->isGRFAvailable(i))
        {
            return true;
        }
    }

    return false;
}

bool LinearScanRA::assignEOTLiveRanges(IR_Builder& builder, std::vector<LSLiveRange*>& liveIntervals)
{
#ifdef DEBUG_VERBOSE_ON
    COUT_ERROR << "--------------------------------- " << std::endl;
#endif
    uint32_t nextEOTGRF = numRegLRA - numRowsEOT;
    for (auto lr : liveIntervals)
    {
        assert(lr->isEOT());
        G4_Declare* dcl = lr->getTopDcl();
        G4_Greg* phyReg = builder.phyregpool.getGreg(nextEOTGRF);
        dcl->getRegVar()->setPhyReg(phyReg, 0);
        lr->setPhyReg(phyReg, 0);
        lr->setAssigned(true);
        lr->setUseUnAvailableReg(isUseUnAvailableRegister(nextEOTGRF, dcl->getNumRows()));
        nextEOTGRF += dcl->getNumRows();
        if (nextEOTGRF > numRegLRA)
        {
            assert(0);
        }
#ifdef DEBUG_VERBOSE_ON
        int startregnum, endregnum, startsregnum, endsregnum;
        G4_VarBase* op;
        op = lr->getPhyReg(startsregnum);

        startregnum = endregnum = op->asGreg()->getRegNum();
        endsregnum = startsregnum + (lr->getTopDcl()->getNumElems() * lr->getTopDcl()->getElemSize() / 2) - 1;

        if (lr->getTopDcl()->getNumRows() > 1) {
            endregnum = startregnum + lr->getTopDcl()->getNumRows() - 1;

            if (lr->getTopDcl()->getWordSize() > 0)
            {
                endsregnum = lr->getTopDcl()->getWordSize() % numEltPerGRF(Type_UW) - 1;
                if (endsregnum < 0) endsregnum = 15;
            }
            else
                endsregnum = 15; // last word in GRF
        }
        COUT_ERROR << "Assigned physical register to " << lr->getTopDcl()->getName() <<
            " (r" << startregnum << "." << startsregnum << ":w - " <<
            "r" << endregnum << "." << endsregnum << ":w)" << std::endl;
#endif
    }

    return true;
}

bool globalLinearScan::runLinearScan(IR_Builder& builder, std::vector<LSLiveRange*>& liveIntervals, std::list<LSLiveRange*>& spillLRs)
{
    unsigned int idx = 0;
    bool allocateRegResult = false;

#ifdef DEBUG_VERBOSE_ON
    COUT_ERROR << "--------------------------------- " <<  std::endl;
#endif

    for (auto lr : liveIntervals)
    {
        lr->getFirstRef(idx);
        if (!lr->isEOT() && !lr->getAssigned())
        {
            //Add forbidden for EOT registers if there is overlap between lr and EOT lr
            for (auto preAssginedLI : *preAssignedIntervals)
            {
                unsigned firstIdx;
                preAssginedLI->getFirstRef(firstIdx);
                G4_VarBase* preg;
                int subregnumword;

                preg = preAssginedLI->getPhyReg(subregnumword);
                unsigned reg = preg->asGreg()->getRegNum();
                unsigned rowNum = preAssginedLI->getTopDcl()->getNumRows();

                unsigned lastIdx = 0;
                lr->getLastRef(lastIdx);
                if (lastIdx > firstIdx)
                {
                    for (unsigned k = 0; k < rowNum; k++)
                    {
                        lr->addForbidden(reg + k);
                    }
                }
            }
        }

#ifdef DEBUG_VERBOSE_ON
        COUT_ERROR << "-------- IDX: " << idx << "---------" << std::endl;
#endif

        //Expire the live ranges ended befoe idx
        expireGlobalRanges(idx);
        expireInputRanges(idx);

        G4_Declare* dcl = lr->getTopDcl();
        if (builder.kernel.fg.isPseudoVCADcl(dcl))
        {
            vector<unsigned int> callerSaveRegs;
            vector<unsigned int> regRegs;
            getCallerSaveGRF(callerSaveRegs, regRegs, &gra.kernel);
            for (unsigned int i = 0; i < callerSaveRegs.size(); i++)
            {
                unsigned int callerSaveReg = callerSaveRegs[i];
                lr->addForbidden(callerSaveReg);
            }
            for (unsigned int i = 0; i < regRegs.size(); i++)
            {
                unsigned int callerSaveReg = regRegs[i];
                lr->addRetRegs(callerSaveReg);
            }
            continue;
        }
        else if (builder.kernel.fg.isPseudoVCEDcl(dcl))
        {
            vector<unsigned int> calleeSaveRegs;
            getCalleeSaveGRF(calleeSaveRegs, &gra.kernel);
            for (unsigned int i = 0; i < calleeSaveRegs.size(); i++)
            {
                unsigned int calleeSaveReg = calleeSaveRegs[i];
                lr->addForbidden(calleeSaveReg);
            }
            continue;
        }
        else if (!lr->getAssigned())
        {
            if (dcl == gra.getOldFPDcl())
            {
                vector<unsigned int> callerSaveRegs;
                vector<unsigned int> regRegs;
                getCallerSaveGRF(callerSaveRegs, regRegs, &gra.kernel);
                for (unsigned int i = 0; i < callerSaveRegs.size(); i++)
                {
                    unsigned int callerSaveReg = callerSaveRegs[i];
                    lr->addForbidden(callerSaveReg);
                }
                for (unsigned int i = 0; i < regRegs.size(); i++)
                {
                    unsigned int callerSaveReg = regRegs[i];
                    lr->addRetRegs(callerSaveReg);
                }
            }

            startGRFReg = 0;
            allocateRegResult = allocateRegsLinearScan(lr, builder);
            if (allocateRegResult)
            {
#ifdef DEBUG_VERBOSE_ON
                int startregnum, endregnum, startsregnum, endsregnum;
                G4_VarBase* op;
                op = lr->getPhyReg(startsregnum);

                startregnum = endregnum = op->asGreg()->getRegNum();
                endsregnum = startsregnum + (lr->getTopDcl()->getNumElems() * lr->getTopDcl()->getElemSize() / 2) - 1;

                if (lr->getTopDcl()->getNumRows() > 1) {
                    endregnum = startregnum + lr->getTopDcl()->getNumRows() - 1;

                    if (lr->getTopDcl()->getWordSize() > 0)
                    {
                        endsregnum = lr->getTopDcl()->getWordSize() % numEltPerGRF(Type_UW) - 1;
                        if (endsregnum < 0) endsregnum = 15;
                    }
                    else
                        endsregnum = 15; // last word in GRF
                }
                COUT_ERROR << "Assigned physical register to " << lr->getTopDcl()->getName() <<
                    " (r" << startregnum << "." << startsregnum << ":w - " <<
                    "r" << endregnum << "." << endsregnum << ":w)" << std::endl;
#endif
            }
        }
        else
        {
            allocateRegResult = true;
            int startregnum, subregnum, endsregnum;
            G4_VarBase* op;
            op = lr->getPhyReg(subregnum);

            startregnum = op->asGreg()->getRegNum();
            endsregnum = subregnum + (lr->getTopDcl()->getNumElems() * lr->getTopDcl()->getElemSize() / 2) - 1;
            int nrows = 0;
            int lastRowSize = 0;
            int size = lr->getSizeInWords();
            LinearScanRA::getRowInfo(size, nrows, lastRowSize);

            if (!lr->isUseUnAvailableReg())
            {
                if ((unsigned)size >= numEltPerGRF(Type_UW))
                {
                    if (size % numEltPerGRF(Type_UW) == 0)
                    {
                        pregManager.getAvaialableRegs()->setGRFBusy(startregnum, lr->getTopDcl()->getNumRows());
                    }
                    else
                    {
                        pregManager.getAvaialableRegs()->setGRFBusy(startregnum, lr->getTopDcl()->getNumRows() - 1);
                        pregManager.getAvaialableRegs()->setWordBusy(startregnum + lr->getTopDcl()->getNumRows() - 1, 0, lastRowSize);
                    }
                }
                else
                {
                    pregManager.getAvaialableRegs()->setWordBusy(startregnum, subregnum, size);
                }
            }
        }

        if (allocateRegResult)
        {
            updateGlobalActiveList(lr);
        }
        else //Spill
        {
            if (!spillFromActiveList(lr, spillLRs))
            {
#ifdef DEBUG_VERBOSE_ON
                COUT_ERROR << "Failed to get spill candidates" << std::endl;
#endif
                return false;
            }

            //Fixme: get the start GRF already, can allocate immediately
            allocateRegResult = allocateRegsLinearScan(lr, builder);
            if (!allocateRegResult)
            {
#ifdef DEBUG_VERBOSE_ON
                COUT_ERROR << "Failed assigned physical register to " << lr->getTopDcl()->getName() << std::endl;
#endif
                return false;
            }
            else
            {
                updateGlobalActiveList(lr);

#ifdef DEBUG_VERBOSE_ON
                int startregnum, startsregnum;
                G4_VarBase* op;
                op = lr->getPhyReg(startsregnum);

                startregnum = op->asGreg()->getRegNum();
                COUT_ERROR << "After spill: Assigned physical register to " << lr->getTopDcl()->getName() << " GRF: " << startregnum << std::endl;
#endif
            }
        }
    }

    int totalGRFNum = builder.kernel.getNumRegTotal();
    for (int i = 0; i < totalGRFNum; i++)
    {
        activeGRF[i].activeLV.clear();
        activeGRF[i].activeInput.clear();
    }

    //Assign the registers for the live out ones
    expireAllActive();

    return true;
}

void globalLinearScan::updateGlobalActiveList(LSLiveRange* lr)
{
    bool done = false;
    unsigned int newlr_end;

    lr->getLastRef(newlr_end);

    for (auto active_it = active.begin();
        active_it != active.end();
        active_it++)
    {
        unsigned int end_idx;
        LSLiveRange* active_lr = (*active_it);

        active_lr->getLastRef(end_idx);

        if (end_idx > newlr_end)
        {
            active.insert(active_it, lr);
            done = true;
            break;
        }
    }

    if (done == false)
        active.push_back(lr);

#ifdef DEBUG_VERBOSE_ON
    COUT_ERROR << "Add active " << lr->getTopDcl()->getName() << std::endl;
#endif

    G4_VarBase* op;
    int startsregnum = 0;
    op = lr->getPhyReg(startsregnum);
    unsigned startregnum = op->asGreg()->getRegNum();
    unsigned endregnum = startregnum + lr->getTopDcl()->getNumRows() - 1;
    for (unsigned i = startregnum; i <= endregnum; i++)
    {
        activeGRF[i].activeLV.push_back(lr);
#ifdef DEBUG_VERBOSE_ON
        COUT_ERROR << "Add activeGRF " << lr->getTopDcl()->getName() << " Reg: " << i << std::endl;
#endif
    }
}

bool globalLinearScan::insertLiveRange(std::list<LSLiveRange*>* liveIntervals, LSLiveRange* lr)
{
    unsigned int idx = 0;
    lr->getFirstRef(idx);
    std::list<LSLiveRange*>::iterator it = liveIntervals->begin();
    while (it != liveIntervals->end())
    {
        LSLiveRange* curLR = (*it);
        unsigned curIdx = 0;
        curLR->getFirstRef(curIdx);
        if (curIdx > idx)
        {
            liveIntervals->insert(it, lr);
            return true;
        }

        it++;
    }

    return false;
}

bool globalLinearScan::shouldSpillRegisterLinearScan(G4_Declare* dcl) const
{
    if (dcl->getRegFile() == G4_INPUT)
        return false;

    if (dcl->getRegVar()->getId() == UNDEFINED_VAL)
        return false;
    else if (dcl->getRegVar()->isRegVarTransient() || dcl->getRegVar()->isRegVarTmp())
        return false;

    else if (builder.kernel.fg.isPseudoVCADcl(dcl) ||
        builder.kernel.fg.isPseudoVCEDcl(dcl))
        return false;

    return true;
}

bool globalLinearScan::canBeSpilledLR(LSLiveRange* tlr, LSLiveRange* lr, int GRFNum)
{
    unsigned int startIdx = 0;
    lr->getFirstRef(startIdx);
    if (startIdx < 2)  //Input or the variable without define
    {
        return false;
    }

    if (lr->isUseUnAvailableReg())
    {
        return false;
    }

    if (lr->getTopDcl()->getAddressed())
    {
        return false;
    }

    if (lr->getTopDcl() == gra.getOldFPDcl())
    {
        return false;
    }

    if (!lr->isEOT() &&
        tlr->getForbiddenGRF().find(GRFNum) == tlr->getForbiddenGRF().end() &&
        shouldSpillRegisterLinearScan(lr->getTopDcl()))
    {
        return true;
    }

    return false;
}

int globalLinearScan::findSpillCandidate(LSLiveRange* tlr)
{
    unsigned short requiredRows = tlr->getTopDcl()->getNumRows();
    int referenceCount = 0;
    int startGRF = -1;
    float spillCost = (float)(int)0x7FFFFFFF;
    unsigned lastIdxs = 1;
    unsigned tStartIdx = 0;

    tlr->getFirstRef(tStartIdx);
    BankAlign bankAlign = getBankAlign(tlr);
    for (int i = 0; i < (int)(numRegLRA - requiredRows); i++)
    {
        unsigned endIdx = 0;
        bool canBeFree = true;
        std::vector<LSLiveRange*> lv;

        pregManager.getAvaialableRegs()->findRegisterCandiateWithAlignForward(i, bankAlign, false);

        // Check the following adjacent registers
        for (int k = i; k < i + requiredRows; k++)
        {
            if (activeGRF[k].activeInput.size())
            {
                i = k;
                canBeFree = false;
                break;
            }

            if (activeGRF[k].activeLV.size())
            {
                // There may be multiple variables take same register with different offsets
                for (auto lr : activeGRF[k].activeLV)
                {
                    if (std::find(lv.begin(), lv.end(), lr) != lv.end()) // one LV may occupy multiple registers
                    {
                        continue;
                    }

                    if (!canBeSpilledLR(tlr, lr, k))
                    {
                        int startsregnum = 0;
                        G4_VarBase* op = lr->getPhyReg(startsregnum);
                        unsigned startregnum = op->asGreg()->getRegNum();

                        canBeFree = false;
                        i = startregnum + lr->getTopDcl()->getNumRows() - 1; // Jump to k + rows - 1 to avoid unnecessory analysis.

                        break;
                    }
                    if (!canBeFree)
                    {
                        break;
                    }

                    int startsregnum = 0;
                    G4_VarBase* op = lr->getPhyReg(startsregnum);
                    int startregnum = op->asGreg()->getRegNum();
                    unsigned effectGRFNum = startregnum > i ? lr->getTopDcl()->getNumRows() : lr->getTopDcl()->getNumRows() - (i - startregnum);
                    lr->getLastRef(endIdx);
                    lastIdxs += (endIdx - tStartIdx) * effectGRFNum;
                    referenceCount += gra.getNumRefs(lr->getTopDcl());
                    lv.push_back(lr);
                }
                if (!canBeFree)
                {
                    break;
                }
            }
            else if (pregManager.getAvaialableRegs()->isGRFAvailable(k) && !pregManager.getAvaialableRegs()->isGRFBusy(k))
            {
                lastIdxs += lastLexicalID - tStartIdx;
            }
            else //Reserved regsiters
            {
                i = k;
                canBeFree = false;
                break;
            }
        }

        if (canBeFree)
        {
            //Spill cost
            float currentSpillCost = (float)referenceCount / lastIdxs;

            if (currentSpillCost < spillCost)
            {
                startGRF = i;
                spillCost = currentSpillCost;
            }
        }

        lastIdxs = 1;
        lv.clear();
        referenceCount = 0;
    }

    return startGRF;
}

void globalLinearScan::freeSelectedRegistsers(int startGRF, LSLiveRange* tlr, std::list<LSLiveRange*>& spillLRs)
{
    unsigned short requiredRows = tlr->getTopDcl()->getNumRows();
#ifdef DEBUG_VERBOSE_ON
    COUT_ERROR << "Required GRF size for spill: " << requiredRows << std::endl;
#endif

        //Free registers.
    for (int k = startGRF; k < startGRF + requiredRows; k++)
    {
#ifdef DEBUG_VERBOSE_ON
        if (!activeGRF[k].activeLV.size())
        {
            COUT_ERROR << "Pick free GRF for spill: " << " GRF:" << k << std::endl;
        }
#endif

        while (activeGRF[k].activeLV.size())
        {
            LSLiveRange* lr = activeGRF[k].activeLV.front();

            G4_VarBase* op;
            int startsregnum = 0;
            op = lr->getPhyReg(startsregnum);
            unsigned startregnum = op->asGreg()->getRegNum();
            unsigned endregnum = startregnum + lr->getTopDcl()->getNumRows() - 1;

            assert(startregnum <= (unsigned)k);
            assert(lr->getTopDcl()->getRegFile() != G4_INPUT);

#ifdef DEBUG_VERBOSE_ON
            if (!strcmp(lr->getTopDcl()->getName(), "TV0"))
            {
                printf("Reach\n");
            }
#endif
            //Free from the register buckect array
            for (unsigned s = startregnum; s <= endregnum; s++)
            {
                std::vector<LSLiveRange*>::iterator it = std::find(activeGRF[s].activeLV.begin(), activeGRF[s].activeLV.end(), lr);
                if (it != activeGRF[s].activeLV.end())
                {
#ifdef DEBUG_VERBOSE_ON
                    COUT_ERROR << "SPILL: Free activeGRF from : " << (lr)->getTopDcl()->getName() << " GRF:" << s << std::endl;
#endif
                    activeGRF[s].activeLV.erase(it);
                }
            }

            //Free the allocated register
            freeAllocedRegs(lr, true);
#ifdef DEBUG_VERBOSE_ON
            COUT_ERROR << "    Free LV : " << (lr)->getTopDcl()->getName() << std::endl;
#endif

            //Record spilled live range
            if (std::find(spillLRs.begin(), spillLRs.end(), lr) == spillLRs.end())
            {
                spillLRs.push_back(lr);
            }

            //Remove spilled live range from active list
            std::list<LSLiveRange*>::iterator activeListIter = active.begin();
            while (activeListIter != active.end())
            {
                std::list<LSLiveRange*>::iterator nextIt = activeListIter;
                nextIt++;

                if ((*activeListIter) == lr)
                {
#ifdef DEBUG_VERBOSE_ON
                    COUT_ERROR << "SPILL: Free active lr: " << (*activeListIter)->getTopDcl()->getName() << std::endl;
#endif
                    active.erase(activeListIter);
                    break;
                }
                activeListIter = nextIt;
            }
        }
    }
}

bool globalLinearScan::spillFromActiveList(LSLiveRange* tlr, std::list<LSLiveRange*>& spillLRs)
{
#ifdef DEBUG_VERBOSE_ON
    printActives();
#endif
    int startGRF = findSpillCandidate(tlr);

    if (startGRF == -1)
    {
        return false;
    }

    freeSelectedRegistsers(startGRF, tlr, spillLRs);

    return true;
}

void globalLinearScan::expireGlobalRanges(unsigned int idx)
{
    //active list is sorted in ascending order of starting index

    while (active.size() > 0)
    {
        unsigned int endIdx;
        LSLiveRange* lr = active.front();

        lr->getLastRef(endIdx);

        if (endIdx <= idx)
        {
            G4_VarBase* preg;
            int subregnumword, subregnum;

            preg = lr->getPhyReg(subregnumword);

            if (preg)
            {
                subregnum = LinearScanRA::convertSubRegOffFromWords(lr->getTopDcl(), subregnumword);

                // Mark the RegVar object of dcl as assigned to physical register
                lr->getTopDcl()->getRegVar()->setPhyReg(preg, subregnum);
                lr->setAssigned(true);
            }

#ifdef DEBUG_VERBOSE_ON
            COUT_ERROR << "Expiring range " << lr->getTopDcl()->getName() << " With GRF size: " << lr->getTopDcl()->getNumRows() << std::endl;
#endif
            if (preg)
            {
                unsigned startregnum = preg->asGreg()->getRegNum();
                unsigned endregnum = startregnum + lr->getTopDcl()->getNumRows() - 1;
                for (unsigned i = startregnum; i <= endregnum; i++)
                {
                    std::vector<LSLiveRange*>::iterator activeListIter = activeGRF[i].activeLV.begin();
                    while (activeListIter != activeGRF[i].activeLV.end())
                    {
                        std::vector<LSLiveRange*>::iterator nextIt = activeListIter;
                        nextIt++;
                        if ((*activeListIter) == lr)
                        {
                            activeGRF[i].activeLV.erase(activeListIter);
#ifdef DEBUG_VERBOSE_ON
                            COUT_ERROR << "Remove range " << lr->getTopDcl()->getName() << " from activeGRF: " << i << std::endl;
#endif
                        }
                        activeListIter = nextIt;
                    }
                }
            }

            // Free physical regs marked for this range
            freeAllocedRegs(lr, true);

            // Remove range from active list
            active.pop_front();
        }
        else
        {
            // As soon as we find first range that ends after ids break loop
            break;
        }
    }
}

void globalLinearScan::expireInputRanges(unsigned int global_idx)
{
    while (inputIntervals.size() > 0)
    {
        LSInputLiveRange* lr = inputIntervals.front();
        unsigned int endIdx = lr->getLrEndIdx();

        if (endIdx <= global_idx)
        {
            unsigned int regnum = lr->getRegWordIdx() / numEltPerGRF(Type_UW);
            unsigned int subRegInWord = lr->getRegWordIdx() % numEltPerGRF(Type_UW);

            // Free physical regs marked for this range
            pregManager.freeRegs(regnum, subRegInWord, 1, endIdx);

#ifdef DEBUG_VERBOSE_ON
            COUT_ERROR << "Expiring input r" << regnum << "." << subRegInWord << std::endl;
#endif

            // Remove range from inputIntervals list
            inputIntervals.pop_front();
            assert(lr == activeGRF[regnum].activeInput.front());
            activeGRF[regnum].activeInput.erase(activeGRF[regnum].activeInput.begin());
        }
        else
        {
            // As soon as we find first range that ends after ids break loop
            break;
        }
    }
}

BankAlign globalLinearScan::getBankAlign(LSLiveRange* lr)
{
    G4_Declare* dcl = lr->getTopDcl();
    BankAlign bankAlign = gra.isEvenAligned(dcl) ? BankAlign::Even : BankAlign::Either;
    if (bankAlign == BankAlign::Either)
    {
        if (doBankConflict &&
            gra.getBankConflict(lr->getTopDcl()) != BANK_CONFLICT_NONE)
        {
            bankAlign = gra.getBankAlign(lr->getTopDcl());
        }
    }

    if (gra.getVarSplitPass()->isPartialDcl(lr->getTopDcl()))
    {
        // Special alignment is not needed for var split intrinsic
        bankAlign = BankAlign::Either;
    }

    return bankAlign;
}

bool globalLinearScan::allocateRegsLinearScan(LSLiveRange* lr, IR_Builder& builder)
{
    int regnum, subregnum;
    unsigned int localRABound = 0;
    unsigned int instID;

    lr->getFirstRef(instID);
    // Let local RA allocate only those ranges that need < 10 GRFs
    // Larger ranges are not many and are best left to global RA
    // as it can make a better judgement by considering the
    // spill cost.
    int nrows = 0;
    int size = lr->getSizeInWords();
    G4_Declare* dcl = lr->getTopDcl();
    G4_SubReg_Align subalign = gra.getSubRegAlign(dcl);
    unsigned short occupiedBundles = getOccupiedBundle(dcl);
    localRABound = numRegLRA - 1;  //-1, localRABound will be counted in findFreeRegs()

    BankAlign bankAlign = getBankAlign(lr);
    nrows = pregManager.findFreeRegs(size,
        bankAlign,
        subalign,
        regnum,
        subregnum,
        startGRFReg,
        localRABound,
        occupiedBundles,
        instID,
        lr->getForbidden());

    if (nrows)
    {
#ifdef DEBUG_VERBOSE_ON
        COUT_ERROR << lr->getTopDcl()->getName() << ":r" << regnum << "  BANK: " << (int)bankAlign << std::endl;
#endif
        lr->setPhyReg(builder.phyregpool.getGreg(regnum), subregnum);
        return true;
    }
#ifdef DEBUG_VERBOSE_ON
    COUT_ERROR << lr->getTopDcl()->getName() << ": failed to allocate" << std::endl;
#endif

    return false;
}

bool PhyRegsLocalRA::findFreeMultipleRegsForward(int regIdx, BankAlign align, int& regnum, int nrows, int lastRowSize, int endReg, unsigned short occupiedBundles, int instID, const bool* forbidden)
{
    int foundItem = 0;
    int startReg = 0;
    int i = regIdx;
    int grfRows = 0;
    bool multiSteps = nrows > 1;

    if (lastRowSize % numEltPerGRF(Type_UW) == 0)
    {
        grfRows = nrows;
    }
    else
    {
        grfRows = nrows - 1;
    }

    findRegisterCandiateWithAlignForward(i, align, multiSteps);
    i = findBundleConflictFreeRegister(i, endReg, occupiedBundles, align, multiSteps);

    startReg = i;
    while (i <= endReg + nrows - 1)
    {
        if (isGRFAvailable(i) && !forbidden[i] &&
            regBusyVector[i] == 0)
        {
            foundItem++;
        }
        else if (foundItem < grfRows)
        {
            foundItem = 0;
            i++;
            findRegisterCandiateWithAlignForward(i, align, multiSteps);
            i = findBundleConflictFreeRegister(i, endReg, occupiedBundles, align, multiSteps);
            startReg = i;
            continue;
        }

        if (foundItem == grfRows)
        {
            if (lastRowSize % numEltPerGRF(Type_UW) == 0)
            {
                regnum = startReg;
                return true;
            }
            else
            {
                if (i + 1 <= endReg + nrows - 1 &&
                    isGRFAvailable(i + 1) && !forbidden[i + 1] &&
                    (isWordBusy(i + 1, 0, lastRowSize) == false))
                {
                    regnum = startReg;
                    return true;
                }
                else
                {
                    foundItem = 0;
                    i++;
                    findRegisterCandiateWithAlignForward(i, align, multiSteps);
                    i = findBundleConflictFreeRegister(i, endReg, occupiedBundles, align, multiSteps);
                    startReg = i;
                    continue;
                }
            }
        }

        i++;
    }

    return false;
}

bool PhyRegsLocalRA::findFreeSingleReg(int regIdx, int size, BankAlign align, G4_SubReg_Align subalign, int& regnum, int& subregnum, int endReg, const bool* forbidden)
{
    int i = regIdx;
    bool found = false;

    while (!found)
    {
        if (i > endReg) //<= works
            break;

        // Align GRF
        if ((align == BankAlign::Even) && (i % 2 != 0))
        {
            i++;
            continue;
        }
        else if ((align == BankAlign::Odd) && (i % 2 == 0))
        {
            i++;
            continue;
        }
        else if ((align == BankAlign::Even2GRF) && ((i % 4 >= 2)))
        {
            i++;
            continue;
        }
        else if ((align == BankAlign::Odd2GRF) && ((i % 4 < 2)))
        {
            i++;
            continue;
        }

        if (isGRFAvailable(i, 1) && !forbidden[i])
        {
            found = findFreeSingleReg(i, subalign, regnum, subregnum, size);
            if (found)
            {
                return true;
            }
        }
        i++;
    }

    return false;
}

int PhyRegsManager::findFreeRegs(int size, BankAlign align, G4_SubReg_Align subalign, int& regnum, int& subregnum,
    int startRegNum, int endRegNum, unsigned short occupiedBundles, unsigned int instID, const bool* forbidden)
{
    int nrows = 0;
    int lastRowSize = 0;
    LocalRA::getRowInfo(size, nrows, lastRowSize);

    int startReg = startRegNum;
    int endReg = endRegNum - nrows + 1;

    bool found = false;

    if (size >= (int)numEltPerGRF(Type_UW))
    {
        found = availableRegs.findFreeMultipleRegsForward(startReg, align, regnum, nrows, lastRowSize, endReg, occupiedBundles, instID, forbidden);
        if (found)
        {
            subregnum = 0;
            if (size % numEltPerGRF(Type_UW) == 0)
            {
                availableRegs.setGRFBusy(regnum, nrows);
            }
            else
            {
                availableRegs.setGRFBusy(regnum, nrows - 1);
                availableRegs.setWordBusy(regnum + nrows - 1, 0, lastRowSize);
            }
        }
    }
    else
    {
        found = availableRegs.findFreeSingleReg(startReg, size, align, subalign, regnum, subregnum, endReg, forbidden);
        if (found)
        {
            availableRegs.setWordBusy(regnum, subregnum, size);
        }
    }

    if (found)
    {
        return nrows;
    }

    return 0;
}
