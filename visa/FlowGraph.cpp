/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BitSet.h"
#include "BuildIR.h"
#include "CFGStructurizer.h"
#include "DebugInfo.h"
#include "FlowGraph.h"
#include "G4_Kernel.hpp"
#include "Option.h"
#include "PhyRegUsage.h"
#include "visa_wa.h"

#include "BinaryEncodingIGA.h"
#include "iga/IGALibrary/api/iga.h"
#include "iga/IGALibrary/api/iga.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#if defined(WIN32)
#define CDECLATTRIBUTE __cdecl
#elif __GNUC__
#ifdef __x86_64__
#define CDECLATTRIBUTE
#else
#define CDECLATTRIBUTE                 __attribute__((__cdecl__))
#endif
#endif

using namespace vISA;


void GlobalOpndHashTable::HashNode::insert(uint16_t newLB, uint16_t newRB)
{
    // check if the newLB/RB either subsumes or can be subsumed by an existing bound
    // ToDo: consider merging bound as well
    for (uint32_t& bound : bounds)
    {
        uint16_t nodeLB = getLB(bound);
        uint16_t nodeRB = getRB(bound);
        if (newLB >= nodeLB && newRB <= nodeRB)
        {
            return;
        }
        else if (newLB <= nodeLB && newRB >= nodeRB)
        {
            bound = packBound(newLB, newRB);
            return;
        }
    }
    bounds.push_back(packBound(newLB, newRB));
}

bool GlobalOpndHashTable::HashNode::isInNode(uint16_t lb, uint16_t rb) const
{
    for (uint32_t bound : bounds)
    {
        uint16_t nodeLB = getLB(bound);
        uint16_t nodeRB = getRB(bound);
        if (lb <= nodeLB && rb >= nodeLB)
        {
            return true;
        }
        else if (lb > nodeLB && lb <= nodeRB)
        {
            return true;
        }
    }
    return false;
}

void GlobalOpndHashTable::clearHashTable()
{
    for (auto& iter : globalVars)
    {
        iter.second->~HashNode();
    }
    globalVars.clear();
    globalOpnds.clear();
}

// all of the operand in this table are srcRegion
void GlobalOpndHashTable::addGlobalOpnd(G4_Operand *opnd)
{
    G4_Declare *topDcl = opnd->getTopDcl();

    if (topDcl)
    {
        // global operands must have a declare
        auto entry = globalVars.find(topDcl);
        if (entry != globalVars.end())
        {
            entry->second->insert(
                (uint16_t)opnd->getLeftBound(),
                (uint16_t)opnd->getRightBound());
        }
        else
        {
            HashNode* node = new (mem)HashNode(
                (uint16_t)opnd->getLeftBound(),
                (uint16_t)opnd->getRightBound(),
                private_arena_allocator);
            globalVars[topDcl] = node;
        }
        globalOpnds.push_back(opnd);
    }
}

// if def overlaps with any operand in this table, it is treated as global
bool GlobalOpndHashTable::isOpndGlobal(G4_Operand *opnd) const
{
    G4_Declare * dcl = opnd->getTopDcl();
    if (dcl == NULL)
    {
        return false;
    }
    else if (dcl->getAddressed())
    {
        // Conservatively assume that all address taken
        // virtual registers are global
        return true;
    }
    else
    {
        auto entry = globalVars.find(dcl);
        if (entry == globalVars.end())
        {
            return false;
        }
        HashNode* node = entry->second;
        return node->isInNode(
            (uint16_t)opnd->getLeftBound(),
            (uint16_t)opnd->getRightBound());
    }
}

void GlobalOpndHashTable::dump(std::ostream &os) const
{
    os << "Global variables:\n";
    for (auto&& entry : globalVars)
    {
        G4_Declare* dcl = entry.first;
        dcl->dump();
        if ((dcl->getRegFile() & G4_FLAG) == 0)
        {
            std::vector<bool> globalElt;
            globalElt.resize(dcl->getByteSize(), false);
            auto ranges = entry.second;
            for (auto bound : ranges->bounds)
            {
                uint16_t lb = getLB(bound);
                uint16_t rb = getRB(bound);
                for (int i = lb; i <= rb; ++i)
                {
                    globalElt[i] = true;
                }
            }
            bool inRange = false;
            for (int i = 0, size = (int)globalElt.size(); i < size; ++i)
            {
                if (globalElt[i] && !inRange)
                {
                    // start of new range
                    os << "[" << i << ",";
                    inRange = true;
                }
                else if (!globalElt[i] && inRange)
                {
                    // end of range
                    os << i - 1 << "], ";
                    inRange = false;
                }
            }
            if (inRange)
            {
                // close last range
                os << globalElt.size() - 1 << "]";
            }
        }
        os << "\n";
    }
    os << "Instructions with global operands:\n";
    for (auto opnd : globalOpnds)
    {
        if (opnd->getInst())
        {
            opnd->getInst()->print(os);
        }
    }
}

void FlowGraph::setPhysicalLink(G4_BB* pred, G4_BB* succ)
{
    if (pred)
    {
        pred->setPhysicalSucc(succ);
    }
    if (succ)
    {
        succ->setPhysicalPred(pred);
    }
}

BB_LIST_ITER FlowGraph::insert(BB_LIST_ITER iter, G4_BB* bb)
{
    G4_BB* prev = iter != BBs.begin() ? *std::prev(iter) : nullptr;
    G4_BB* next = iter != BBs.end() ? *iter : nullptr;
    setPhysicalLink(prev, bb);
    setPhysicalLink(bb, next);
    markStale();
    return BBs.insert(iter, bb);
}

void FlowGraph::erase(BB_LIST_ITER iter)
{
    G4_BB* prev = (iter != BBs.begin()) ? *std::prev(iter) : nullptr;
    G4_BB* next = (std::next(iter) != BBs.end()) ? *std::next(iter) : nullptr;
    setPhysicalLink(prev, next);
    BBs.erase(iter);
    markStale();
}

void FlowGraph::addPrologBB(G4_BB* BB)
{
    G4_BB* oldEntry = getEntryBB();
    insert(BBs.begin(), BB);
    addPredSuccEdges(BB, oldEntry);
}

void FlowGraph::append(const FlowGraph& otherFG)
{
    markStale();

    for (auto I = otherFG.cbegin(), E = otherFG.cend(); I != E; ++I)
    {
        auto bb = *I;
        BBs.push_back(bb);
        incrementNumBBs();
    }
}

//
// return label's corresponding BB
// if label's BB is not yet created, then create one and add map label to BB
//
G4_BB* FlowGraph::getLabelBB(Label_BB_Map& map, G4_Label* label)
{
    if (map.find(label) != map.end())
    {
        return map[label];
    }
    else
    {
        G4_BB* bb = createNewBB();
        map[label] = bb;
        return bb;
    }
}

//
// first is the first inst of a BB
//
G4_BB* FlowGraph::beginBB(Label_BB_Map& map, G4_INST* first)
{
    if (first == NULL) return NULL;
    G4_INST* labelInst;
    bool newLabelInst = false;
    if (first->isLabel())
    {
        labelInst = first;
    }
    else
    {
        // no label for this BB, create one!
        std::string prefix = builder->kernel.getName();
        std::string name = prefix + "_auto_" + std::to_string(autoLabelId++);
        G4_Label* label = builder->createLabel(name, LABEL_BLOCK);
        labelInst = createNewLabelInst(label);
        newLabelInst = true;
    }
    G4_BB* bb = getLabelBB(map, labelInst->getLabel());
    push_back(bb); // append to BBs list
    if (newLabelInst)
    {
        bb->push_front(labelInst);
    }
    return bb;
}

G4_INST* FlowGraph::createNewLabelInst(G4_Label* label, int lineNo, int CISAOff)
{
    //srcFileName is NULL
    // TODO: remove this (use createLabelInst)
    return builder->createInternalInst(NULL, G4_label,
        NULL, g4::NOSAT, g4::SIMD1, NULL, label, NULL, 0);
}

void FlowGraph::removePredSuccEdges(G4_BB* pred, G4_BB* succ)
{
    MUST_BE_TRUE(pred != NULL && succ != NULL, ERROR_INTERNAL_ARGUMENT);

    BB_LIST_ITER lt = pred->Succs.begin();
    for (; lt != pred->Succs.end(); ++lt) {
        if ((*lt) == succ) {
            pred->Succs.erase(lt);
            break;
        }
    }

    lt = succ->Preds.begin();
    for (; lt != succ->Preds.end(); ++lt) {
        if ((*lt) == pred) {
            succ->Preds.erase(lt);
            break;
        }
    }

    markStale();
}

G4_BB* FlowGraph::createNewBB(bool insertInFG)
{
    G4_BB* bb = new (mem)G4_BB(instListAlloc, numBBId, this);

    // Increment counter only when new BB is inserted in FlowGraph
    if (insertInFG)
        numBBId++;

    BBAllocList.push_back(bb);
    return bb;
}

G4_BB* FlowGraph::createNewBBWithLabel(const char* LabelPrefix, int Lineno, int CISAoff)
{
    G4_BB* newBB = createNewBB(true);
    std::string name = LabelPrefix + std::to_string(newBB->getId());
    G4_Label* lbl = builder->createLabel(name, LABEL_BLOCK);
    G4_INST* inst = createNewLabelInst(lbl, Lineno, CISAoff);
    newBB->push_back(inst);
    return newBB;
}

static int globalCount = 1;

int64_t FlowGraph::insertDummyUUIDMov()
{
    // Here when -addKernelId is passed
    if (builder->getIsKernel())
    {
        // Add mov inst as first instruction in kernel. This
        // has to be *first* instruction in the kernel so debugger
        // can detect the pattern.
        //
        // 32-bit random number is generated and set as src0 operand.
        // Earlier nop was being generated with 64-bit UUID overloading
        // MBZ bits and this turned out to be a problem for the
        // debugger (random clobbering of r0).

        for (auto bb : BBs)
        {
            uint32_t seed = (uint32_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();
            std::mt19937 mt_rand(seed * globalCount);
            globalCount++;

            G4_DstRegRegion* nullDst = builder->createNullDst(Type_UD);
            int64_t uuID = (int64_t)mt_rand();
            G4_Operand* randImm = (G4_Operand*)builder->createImm(uuID, Type_UD);
            G4_INST* movInst = builder->createMov(g4::SIMD1, nullDst, randImm, InstOpt_NoOpt, false);

            auto instItEnd = bb->end();
            for (auto it = bb->begin();
                it != instItEnd;
                it++)
            {
                if ((*it)->isLabel())
                {
                    bb->insertBefore(++it, movInst);
                    return uuID;
                }

                bb->push_front(movInst);
                return uuID;
            }
        }
    }
    return 0;
}

//
// (1) check if-else-endif and iff-endif pairs
// (2) add label for those omitted ones
//
bool FlowGraph::matchBranch(int &sn, INST_LIST& instlist, INST_LIST_ITER &it)
{
    G4_INST* inst = *it;
    //
    // process if-endif or if-else-endif
    //
    if (inst->opcode() == G4_if)
    {
        // check label / add label
        G4_Label *if_label = NULL;
        G4_Label *else_label = NULL;
        G4_Label *endif_label = NULL;   // the label immediately before the endif
        G4_INST* ifInst = inst;
        assert(inst->asCFInst()->getJip() == nullptr && "IF should not have a label at this point");

        // create if_label
        std::string createdLabel = "_AUTO_GENERATED_IF_LABEL_" + std::to_string(sn);
        sn++;
        if_label = builder->createLabel(createdLabel, LABEL_BLOCK);
        inst->asCFInst()->setJip(if_label);

        // look for else/endif
        int elseCount = 0;
        it++;
        while (it != instlist.end())
        {
            inst = *it;
            // meet iff or if
            if (inst->opcode() == G4_if)
            {
                if (!matchBranch(sn, instlist, it))
                    return false;
            }
            else if (inst->opcode() == G4_else)
            {
                if (elseCount != 0)
                {
                    MUST_BE_TRUE(false, "ERROR: Mismatched if-else: more than one else following if!");
                    return false;
                }
                elseCount++;
                INST_LIST_ITER it1 = it;
                it1++;

                // add endif label to "else"
                std::string createdLabel = "__AUTO_GENERATED_ELSE_LABEL__" + std::to_string(sn);
                sn++;
                else_label = builder->createLabel(createdLabel, LABEL_BLOCK);
                inst->asCFInst()->setJip(else_label);

                // insert if-else label
                G4_INST* label = createNewLabelInst(if_label, inst->getLineNo(), inst->getCISAOff());
                instlist.insert(it1, label);

                // Uip must be the same as Jip for else instructions.
                inst->asCFInst()->setUip(else_label);
            }
            else if (inst->opcode() == G4_endif)
            {
                if (elseCount == 0)                 // if-endif case
                {
                    // insert endif label
                    G4_INST* label = createNewLabelInst(if_label, inst->getLineNo(), inst->getCISAOff());
                    instlist.insert(it, label);
                    endif_label = if_label;
                }
                else                                    // if-else-endif case
                {
                    // insert endif label
                    G4_INST* label = createNewLabelInst(else_label, inst->getLineNo(), inst->getCISAOff());
                    instlist.insert(it, label);
                    endif_label = else_label;
                }

                //we must also set the UIP of if to point to its corresponding endif
                ifInst->asCFInst()->setUip(endif_label);
                return true;
            }   // if opcode
            if (it == instlist.end())
            {
                MUST_BE_TRUE(false, "ERROR: Can not find endif for if!");
                return false;
            }
            it++;
        }   // while
    }
    //
    // process iff-endif
    //
    else
        MUST_BE_TRUE(false, ERROR_FLOWGRAPH);

    return false;
}

//
//  HW Rules about the jip and uip for the control flow instructions
//  if:
//      <branch_ctrl> set
//          jip = first join in the if block
//          uip = endif
//      <branch_ctrl> not set
//          jip = instruction right after the else, or the endif if else doesn't exist
//          uip = endif
//  else:
//      <branch_ctrl> set
//          jip = first join in the else block
//          uip = endif
//      <branch_ctrl> not set
//          jip = uip = endif
//  endif:
//          jip = uip = next enclosing endif/while
//  while:
//          jip = loop head label
//          uip = don't care
//  break:
//          jip = end of innermost CF block (else/endif/while)
//          uip = while
//  cont:
//          jip = end of innermost CF block (else/endif/while)
//          uip = while
//

//
// Preprocess the instruction and kernel list, including:
// 1. Check if the labels and kernel names are unique
// 2. Check if all the labels used by jmp, CALL, cont, break, goto is defined, determine if goto is forward or backward
// 3. Process the non-label "if-else-endif" cases
//
void FlowGraph::preprocess(INST_LIST& instlist)
{

    std::unordered_set<G4_Label*> labels;   // label inst we have seen so far

    // Mark goto/jmpi/call as either forward or backward
    for (G4_INST* i : instlist)
    {
        if (i->isLabel())
        {
            labels.emplace(i->getLabel());
        }
        else if (i->opcode() == G4_goto)
        {
            if (labels.count(i->asCFInst()->getUip()->asLabel()))
            {
                i->asCFInst()->setBackward(true);
            }
        }
        else if ((i->opcode() == G4_jmpi || i->isCall()) && i->getSrc(0) && i->getSrc(0)->isLabel())
        {
            if (labels.count(i->getSrc(0)->asLabel()))
            {
                i->asCFInst()->setBackward(true);
            }
        }
    }

#ifdef _DEBUG
    // sanity check to make sure we don't have undefined labels
    for (G4_INST* i : instlist)
    {
        if (i->opcode() == G4_goto)
        {
            G4_Label* target = i->asCFInst()->getUip()->asLabel();
            assert(labels.count(target) && "undefined goto label");
        }
        else if ((i->opcode() == G4_jmpi || i->isCall()) && i->getSrc(0) && i->getSrc(0)->isLabel())
        {
            assert(labels.count((G4_Label *)i->getSrc(0)) && "undefined jmpi/call label");
        }
    }
#endif

    //
    // Process the non-label "if-else-endif" cases as following.
    // ToDo: remove this once we stop generating if-else-endif for the IEEE macros
    //
    {
        int sn = 0;
        for (INST_LIST_ITER it = instlist.begin(), instlistEnd = instlist.end(); it != instlistEnd; ++it)
        {
            G4_INST *inst = *it;
            if (inst->opcode() == G4_if)
            {
                if (!matchBranch(sn, instlist, it))
                {
                    MUST_BE_TRUE2(false, "ERROR: mismatched if-branch", inst);
                    break;
                }
            }   // fi
        }   // for
    }
}

void FlowGraph::normalizeFlowGraph()
{
    // For CISA 3 flowgraph there will be pseudo_fcall instructions to invoke stack functions.
    // Save/restore code will be added around this at the time of register allocation.
    // If fcall's successor has more than 1 predecessor then it will create problems inserting
    // code. This function handles such patterns by creating new basic block and guaranteeing
    // that fcall's successor has a single predecessor.
    for (BB_LIST_ITER it = BBs.begin(); it != BBs.end(); it++)
    {
        G4_BB* bb = *it;
        if (bb->isEndWithFCall())
        {
            G4_BB* retBB = bb->Succs.front();
            G4_INST *lInst = retBB->front();
            if (retBB->Preds.size() > 1)
            {

                // To insert restore code we need to guarantee that save code has
                // been executed. If retBB has multiple preds, this may not be
                // guaranteed, so insert a new node.
                G4_BB* newNode = createNewBB();

                // Remove old edge
                removePredSuccEdges(bb, retBB);

                // Add new edges
                addPredSuccEdges(bb, newNode);
                addPredSuccEdges(newNode, retBB);

                // Create and insert label inst
                std::string name = "L_AUTO_" + std::to_string(newNode->getId());
                G4_Label* lbl = builder->createLabel(name, LABEL_BLOCK);
                G4_INST* inst = createNewLabelInst(lbl, lInst->getLineNo(), lInst->getCISAOff());
                newNode->push_back(inst);
                insert(++it, newNode);
                it--;

                retBB = newNode;
            }
        }
    }
}

//
// build a control flow graph from the inst list
// we want to keep the original BB order (same as shown in assembly code) for linear scan
// register allocation. We use beginBB to indicate that we encounter the beginning
// a BB and add the BB to the BB list and use getLabelBB to create a block for a label but
// not to add to the BB list.For instance, when we come across a "jmp target", getLabelBB
// is called to create a BB for target but the target BB is added into the list later (
// assume forward jmp) as the target label is visited.
//
//
void FlowGraph::constructFlowGraph(INST_LIST& instlist)
{
    MUST_BE_TRUE(!instlist.empty(), ERROR_SYNTAX("empty instruction list"));

    if (builder->hasScratchSurface() && (isStackCallFunc || hasStackCalls))
    {
        // unfortunately we can't put this in stack call prolog since this has to be done before RA
        // ToDo: just hard-wire the scratch-surface offset register?
        builder->initScratchSurfaceOffset();
    }
    if (builder->hasFusedEU() &&
        !builder->getOption(vISA_KeepScalarJmp) &&
        getKernel()->getInt32KernelAttr(Attributes::ATTR_Target) == VISA_CM)
    {
        getKernel()->getOptions()->setOptionInternally(vISA_EnableScalarJmp, false);
    }

    pKernel->renameAliasDeclares();
    //
    // The funcInfoHashTable maintains a map between the id of the function's INIT block
    // to its FuncInfo definition.
    //
    FuncInfoHashTable funcInfoHashTable;

    preprocess(instlist);

    //
    // map label to its corresponding BB
    //
    std::unordered_map<G4_Label*, G4_BB*> labelMap;
    //
    // create the entry block of the flow graph
    //
    G4_BB* curr_BB = beginBB(labelMap, instlist.front());

    kernelInfo = new (mem)FuncInfo(UINT_MAX, curr_BB, NULL);

    std::vector<G4_BB*> subroutineStartBB; // needed by handleExit()

    bool hasSIMDCF = false, hasNoUniformGoto = false;
    G4_Label* currSubroutine = nullptr;

    auto addToSubroutine = [this](G4_BB* bb, G4_Label* currSubroutine)
    {
        if (currSubroutine)
        {
            subroutines[currSubroutine].push_back(bb);
        }
    };

    while (!instlist.empty())
    {
        INST_LIST_ITER iter = instlist.begin();
        G4_INST* i = *iter;

        MUST_BE_TRUE(curr_BB != NULL, "Current BB must not be empty");
        //
        // inst i belongs to the current BB
        // remove inst i from instlist and relink it to curr_BB's instList
        //
        curr_BB->splice(curr_BB->end(), instlist, iter);
        G4_INST* next_i = (instlist.empty()) ? nullptr : instlist.front();

        if (i->isSend())
        {
            curr_BB->setSendInBB(true);
        }

        if (i->isLabel() && i->getLabel()->isFuncLabel())
        {
            std::vector<G4_BB*> bbvec;
            subroutines[i->getLabel()] = bbvec;
            currSubroutine = i->getLabel();
            subroutineStartBB.push_back(curr_BB);
        }

        //
        // do and endif do not have predicate and jump-to label,so we treated them as non-control instruction
        // the labels around them will decides the beginning of a new BB
        //
        if (i->isFlowControl() && i->opcode() != G4_endif)
        {
            addToSubroutine(curr_BB, currSubroutine);
            G4_BB* next_BB = beginBB(labelMap, next_i);

            // next_BB may be null if the kernel ends on an CF inst (e.g., backward goto/jmpi)
            // This should be ok because we should not fall-through to next_BB in such case
            // (i.e., goto/jmpi must not be predicated)
            {
                if (i->opcode() == G4_jmpi || i->isCall())
                {
                    //
                    // add control flow edges <curr_BB,target>
                    //
                    if (i->getSrc(0)->isLabel())
                    {
                        addPredSuccEdges(curr_BB, getLabelBB(labelMap, i->getSrc(0)->asLabel()));
                    }
                    else if (i->asCFInst()->isIndirectJmp())
                    {
                        // indirect jmp
                        // For each label in the switch table, we insert a jmpi
                        // to that label. We keep the jmpi in the same
                        // block as the indirect jmp instead of creaing a new block for
                        // each of them, as the offset actually determines which jmpi
                        // instruction we will hit.
                        // FIXME: We may want to delay the emission of the jmps to
                        // each individual labels, so that we still maintain the property
                        // that every basic block ends with a control flow instruction
                        const std::list<G4_Label*>& jmpTargets = i->asCFInst()->getIndirectJmpLabels();
                        for (auto it = jmpTargets.begin(), jmpTrgEnd = jmpTargets.end(); it != jmpTrgEnd; ++it)
                        {
                            G4_INST* jmpInst = builder->createJmp(nullptr, *it, InstOpt_NoOpt, true);
                            indirectJmpTarget.emplace(jmpInst);
                            INST_LIST_ITER jmpInstIter = builder->instList.end();
                            curr_BB->splice(curr_BB->end(), builder->instList, --jmpInstIter);
                            addPredSuccEdges(curr_BB, getLabelBB(labelMap, (*it)));
                        }
                    }

                    if (i->isCall())
                    {
                        //
                        // the <CALL,return addr> link is added temporarily to facilitate linking
                        // RETURN with return addresses. The link will be removed after it
                        // serves its purpose
                        // NOTE: No matter it has predicate, we add this link. We will check the predicate in handleReturn()
                        // and only remove the link when it is not a conditional call
                        //
                        addPredSuccEdges(curr_BB, next_BB);
                        if (i->getSrc(0)->isLabel())
                        {
                            i->getSrc(0)->asLabel()->setFuncLabel(true);
                        }
                    }
                    else if (i->getPredicate())
                    {
                        // add fall through edge
                        addUniquePredSuccEdges(curr_BB, next_BB);
                    }
                }    // if (i->opcode()
                else if (i->opcode() == G4_if || i->opcode() == G4_while || i->opcode() == G4_else)
                {
                    hasSIMDCF = true;
                    if (i->asCFInst()->getJip()->isLabel())
                    {
                        if (i->opcode() == G4_else || i->opcode() == G4_while)
                        {
                            // for G4_while, jump no matter predicate
                            addPredSuccEdges(curr_BB, getLabelBB(labelMap, i->asCFInst()->getJip()));
                        }
                        else if ((i->getPredicate() != NULL) ||
                            ((i->getCondMod() != NULL) &&
                            (i->getSrc(0) != NULL) &&
                                (i->getSrc(1) != NULL)))
                        {
                            addPredSuccEdges(curr_BB, getLabelBB(labelMap, i->asCFInst()->getJip()));
                        }
                    }
                    if (i->opcode() == G4_while)
                    {
                        // always do this since break jumps to while, otherwise code after while without predicate appears unreachable.
                        // add fall through edge if while is not the last instruction
                        if (next_BB)
                        {
                            addPredSuccEdges(curr_BB, next_BB);
                        }
                    }
                    else
                    {
                        //  always fall through
                        addPredSuccEdges(curr_BB, next_BB);     // add fall through edge
                    }
                }
                else if (i->opcode() == G4_break || i->opcode() == G4_cont || i->opcode() == G4_halt)
                {
                    // JIP and UIP must have been computed at this point
                    MUST_BE_TRUE(i->asCFInst()->getJip() != NULL && i->asCFInst()->getUip() != NULL,
                        "null JIP or UIP for break/cont instruction");
                    addPredSuccEdges(curr_BB, getLabelBB(labelMap, i->asCFInst()->getJip()));

                    if (strcmp(i->asCFInst()->getJipLabelStr(), i->asCFInst()->getUipLabelStr()) != 0)
                        addPredSuccEdges(curr_BB, getLabelBB(labelMap, i->asCFInst()->getUip()));

                    //
                    // pred means conditional branch
                    //
                    if (i->getPredicate() != NULL)
                    {
                        // add fall through edge
                        addPredSuccEdges(curr_BB, next_BB);
                    }
                }
                else if (i->isReturn() || i->opcode() == G4_pseudo_exit)
                {
                    // does nothing for unconditional return;
                    // later phase will link the return and the return address
                    if (i->getPredicate() != NULL)
                    {
                        // add fall through edge
                        addPredSuccEdges(curr_BB, next_BB);
                    }
                }
                else if (i->opcode() == G4_pseudo_fcall || i->opcode() == G4_pseudo_fc_call)
                {
                    addPredSuccEdges(curr_BB, next_BB);
                }
                else if (i->opcode() == G4_pseudo_fret || i->opcode() == G4_pseudo_fc_ret)
                {
                    if (i->getPredicate())
                    {
                        // need to add fall through edge
                        addPredSuccEdges(curr_BB, next_BB);
                    }
                }
                else if (i->opcode() == G4_goto)
                {
                    hasNoUniformGoto = true;
                    hasSIMDCF = true;
                    addPredSuccEdges(curr_BB, getLabelBB(labelMap, i->asCFInst()->getUip()));

                    if (i->getPredicate())
                    {
                        // fall through, but check if goto target is same as fall-thru
                        // FIXME: replace all addPredSuccEdges with addUniquePredSuccEdges?
                        addUniquePredSuccEdges(curr_BB, next_BB);
                    }
                }
                else
                {
                    assert(false && "should not reach here");
                }
            } // need edge
            curr_BB = next_BB;
        }  // flow control
        else if (curr_BB->isLastInstEOT())
        {
            addToSubroutine(curr_BB, currSubroutine);
            //this is a send instruction that marks end of thread.
            //the basic block will end here with no outgoing links
            curr_BB = beginBB(labelMap, next_i);
        }
        else if (next_i && next_i->isLabel())
        {
            addToSubroutine(curr_BB, currSubroutine);
            G4_BB* next_BB = beginBB(labelMap, next_i);
            addPredSuccEdges(curr_BB, next_BB);
            curr_BB = next_BB;
        }
    }
    if (curr_BB)
    {
        addToSubroutine(curr_BB, currSubroutine);
    }

    // we can do this only after fg is constructed
    pKernel->calculateSimdSize();

    // Jmpi instruction cannot be used when EU Fusion is enabled.
    bool hasGoto = hasNoUniformGoto;
    if (builder->noScalarJmp())
    {
        // No jmpi should be inserted after this point.
        hasGoto |= convertJmpiToGoto();
    }

    pKernel->dumpToFile("after.CFGConstruction");

    removeRedundantLabels();

    pKernel->dumpToFile("after.RemoveRedundantLabels");

    handleExit(subroutineStartBB.size() > 1 ? subroutineStartBB[1] : nullptr);

    handleReturn(labelMap, funcInfoHashTable);
    mergeReturn(funcInfoHashTable);
    normalizeSubRoutineBB(funcInfoHashTable);

    handleWait();

    if (isStackCallFunc)
    {
        mergeFReturns();
    }

    setPhysicalPredSucc();
    removeUnreachableBlocks(funcInfoHashTable);

    pKernel->dumpToFile("after.RemoveUnreachableBlocks");

    //
    // build the table of function info nodes
    //

    for (FuncInfoHashTable::iterator it = funcInfoHashTable.begin(), end = funcInfoHashTable.end(); it != end; ++it) {
        FuncInfo* funcInfo = (*it).second;
        funcInfo->getInitBB()->setFuncInfo(funcInfo);
        funcInfo->getExitBB()->setFuncInfo(funcInfo);
        funcInfoTable.push_back(funcInfo);
    }

    if (hasGoto)
    {
        // Structurizer requires that the last BB has no goto (ie, the
        // last BB is either a return or an exit).
        if (builder->getOption(vISA_EnableStructurizer) &&
            !endWithGotoInLastBB())
        {
            doCFGStructurize(this);
            pKernel->dumpToFile("after.CFGStructurizer");

            removeRedundantLabels();
            pKernel->dumpToFile("after.PostStructurizerRedundantLabels");
        }
        else
        {
            processGoto(hasSIMDCF);
            pKernel->dumpToFile("after.ProcessGoto");
        }
    }

    // This finds back edges and populates blocks into function info objects.
    // The latter will be used to mark SIMD blocks. Prior to RA, no transformation
    // shall invalidate back edges (G4_BB -> G4_BB).
    reassignBlockIDs();
    findBackEdges();

    if (hasSIMDCF && pKernel->getInt32KernelAttr(Attributes::ATTR_Target) == VISA_CM)
    {
        processSCF(funcInfoHashTable);
        addSIMDEdges();
    }

    // patch the last BB for the kernel
    if (funcInfoTable.size() > 0)
    {
        kernelInfo->updateExitBB(subroutineStartBB[1]->getPhysicalPred());
        topologicalSortCallGraph();
    }
    else
    {
        kernelInfo->updateExitBB(BBs.back());
    }

    // For non-kernel function, always invoke markDivergentBBs to
    // conservatively assume divergence.
    if ((hasSIMDCF || hasGoto || builder->getIsFunction()) &&
        builder->getOption(vISA_divergentBB))
    {
        markDivergentBBs();
    }

    builder->materializeGlobalImm(getEntryBB());
    normalizeRegionDescriptors();
    localDataFlowAnalysis();

    markStale();
}

void FlowGraph::normalizeRegionDescriptors()
{
    for (auto bb : BBs)
    {
        for (auto inst : *bb)
        {
            uint16_t execSize = inst->getExecSize();
            for (unsigned i = 0, e = (unsigned)inst->getNumSrc(); i < e; ++i)
            {
                G4_Operand *src = inst->getSrc(i);
                if (!src || !src->isSrcRegRegion())
                    continue;

                G4_SrcRegRegion *srcRegion = src->asSrcRegRegion();
                const RegionDesc *desc = srcRegion->getRegion();
                auto normDesc = builder->getNormalizedRegion(execSize, desc);
                if (normDesc && normDesc != desc)
                {
                    srcRegion->setRegion(*builder, normDesc, /*invariant*/ true);
                }
            }
        }
    }
}

// materialize the values in global Imm at entry BB
void IR_Builder::materializeGlobalImm(G4_BB* entryBB)
{
    InstSplitPass instSplitter(this);
    for (int i = 0, numImm = immPool.size(); i < numImm; ++i)
    {
        auto&& immVal = immPool.getImmVal(i);
        auto dcl = immPool.getImmDcl(i);
        G4_INST* inst = createMov(
            G4_ExecSize((unsigned)immVal.numElt),
            createDstRegRegion(dcl, 1), immVal.imm, InstOpt_WriteEnable, false);
        auto iter = std::find_if(entryBB->begin(), entryBB->end(),
            [](G4_INST* inst) { return !inst->isLabel(); });
        INST_LIST_ITER newMov = entryBB->insertBefore(iter, inst);
        instSplitter.splitInstruction(newMov, entryBB->getInstList());
    }
}

//
// attempt to sink the pseudo-wait into the immediate succeeding send instruction (in same BB)
// by changing it into a sendc.
// If sinking fails, generate a fence with sendc.
//
void FlowGraph::handleWait()
{
    for (auto bb : BBs)
    {
        auto iter = bb->begin(), instEnd = bb->end();
        while (iter != instEnd)
        {
            auto inst = *iter;
            if (inst->isIntrinsic() &&
                inst->asIntrinsicInst()->getIntrinsicId() == Intrinsic::Wait)
            {
                bool sunk = false;
                auto nextIter = iter;
                ++nextIter;
                while (nextIter != instEnd)
                {
                    G4_INST* nextInst = *nextIter;
                    if (nextInst->isSend())
                    {
                        nextInst->asSendInst()->setSendc();
                        sunk = true;
                        break;
                    }
                    else if (nextInst->isOptBarrier())
                    {
                        break;
                    }
                    ++nextIter;
                }
                if (!sunk)
                {
                    auto fenceInst = builder->createSLMFence();
                    auto sendInst = fenceInst->asSendInst();
                    if (sendInst != NULL)
                    {
                        sendInst->setSendc();
                        bb->insertBefore(iter, fenceInst);
                    }
                }
                iter = bb->erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }
}

//
// Each g4_pseudo_exit instruction will be translated into one of the following:
// -- a unconditional simd1 ret: translated into a EOT send (may be optionally merged with
//    an immediately preceding send)
// -- a conditional simd1 ret: translated into a jmpi to the exit block
// -- a non-uniforom ret: translated into a halt to the exit block
// for case 2 and 3 an exit block will be inserted, and it will consist of a EOT send
// plus a join if it's targeted by another goto instruction
//
void FlowGraph::handleExit(G4_BB* firstSubroutineBB)
{

    // blocks that end with non-uniform or conditional return
    std::vector<G4_BB*> exitBlocks;
    BB_LIST_ITER iter = BBs.begin(), iterEnd = BBs.end();
    for (; iter != iterEnd; ++iter)
    {
        G4_BB* bb = *iter;
        if (bb->size() == 0)
        {
            continue;
        }

        if (bb == firstSubroutineBB)
        {
            // we've reached the first subroutine's entry BB,
            break;
        }

        G4_INST* lastInst = bb->back();
        if (lastInst->opcode() == G4_pseudo_exit)
        {
            if (lastInst->getExecSize() == g4::SIMD1 &&
                !(builder->getFCPatchInfo() && builder->getFCPatchInfo()->getFCComposableKernel()))
            {
                //uniform return
                if (lastInst->getPredicate() != NULL)
                {
                    exitBlocks.push_back(bb);
                }
                else
                {
                    //generate EOT send
                    G4_INST* lastInst = bb->back();
                    bb->pop_back();
                    bool needsEOTSend = true;
                    // the EOT may be folded into the BB's last instruction if it's a send
                    // that supports EOT
                    if (builder->getOption(vISA_foldEOTtoPrevSend) && bb->size() > 1)
                    {
                        G4_InstSend* secondToLastInst = bb->back()->asSendInst();
                        if (secondToLastInst && secondToLastInst->canBeEOT() &&
                            !(secondToLastInst->getMsgDesc()->getSrc1LenRegs() > 2 &&
                                VISA_WA_CHECK(builder->getPWaTable(), WaSendsSrc1SizeLimitWhenEOT)))
                        {
                            G4_SendDescRaw *rawDesc = secondToLastInst->getMsgDescRaw();
                            MUST_BE_TRUE(rawDesc, "expected raw descriptor");
                            rawDesc->setEOT();
                            if (secondToLastInst->isSplitSend())
                            {
                                if (secondToLastInst->getSrc(3)->isImm())
                                {
                                    secondToLastInst->setSrc(
                                        builder->createImm(rawDesc->getExtendedDesc(), Type_UD), 3);
                                }
                            }
                            needsEOTSend = false;
                            if (builder->getHasNullReturnSampler() && VISA_WA_CHECK(builder->getPWaTable(), Wa_1607871015))
                            {
                                bb->addSamplerFlushBeforeEOT();
                            }
                        }
                    }

                    if (needsEOTSend)
                    {
                        bb->addEOTSend(lastInst);
                    }
                }
            }
            else
            {
                exitBlocks.push_back(bb);
            }
        }
    }

    // create an exit BB
    if (exitBlocks.size() > 0)
    {
        G4_BB *exitBB = createNewBB();

        if (builder->getFCPatchInfo() &&
            builder->getFCPatchInfo()->getFCComposableKernel())
        {
            // For FC composable kernels insert exitBB as
            // last BB in BBs list. This automatically does
            // jump threading.
            push_back(exitBB);
        }
        else
        {
            insert(iter, exitBB);
        }

        std::string exitBBStr("__EXIT_BB");
        G4_Label *exitLabel = builder->createLabel(exitBBStr, LABEL_BLOCK);
        G4_INST* label = createNewLabelInst(exitLabel);
        exitBB->push_back(label);

        if (!(builder->getFCPatchInfo() &&
            builder->getFCPatchInfo()->getFCComposableKernel()))
        {
            // Dont insert EOT send for FC composable kernels
            exitBB->addEOTSend();
        }

        for (int i = 0, size = (int)exitBlocks.size(); i < size; i++)
        {
            G4_BB* retBB = exitBlocks[i];
            addPredSuccEdges(retBB, exitBB, false);
            G4_INST* retInst = retBB->back();
            retBB->pop_back();

            // Insert jump only if newly inserted exitBB is not lexical
            // successor of current BB
            auto lastBBIt = BBs.end();
            lastBBIt--;
            if ((*lastBBIt) == exitBB)
            {
                lastBBIt--;
                if ((*lastBBIt) == retBB)
                {
                    // This condition is BB layout dependent.
                    // However, we dont change BB layout in JIT
                    // and in case we do it in future, we
                    // will need to insert correct jumps
                    // there to preserve correctness.
                    continue;
                }
            }

            if (retInst->getExecSize() == g4::SIMD1)
            {
                G4_INST* jmpInst = builder->createJmp(retInst->getPredicate(), exitLabel, InstOpt_NoOpt, false);
                retBB->push_back(jmpInst);
            }
            else
            {
                // uip for goto will be fixed later
                G4_INST* gotoInst = builder->createInternalCFInst(retInst->getPredicate(),
                    G4_goto, retInst->getExecSize(), exitLabel, exitLabel, InstOpt_NoOpt);
                retBB->push_back(gotoInst);
            }
        }
    }

#ifdef _DEBUG

    // sanity check
    for (const G4_BB* bb : BBs)
    {
        if (bb->size() == 0)
        {
            continue;
        }
        const G4_INST* lastInst = bb->back();
        if (lastInst->opcode() == G4_pseudo_exit)
        {
            MUST_BE_TRUE(false, "All pseudo exits should be removed at this point");
        }
    }

#endif
}

//
// This phase iterates each BB and checks if the last inst of a BB is a "call foo".  If yes,
// the algorithm traverses from the block of foo to search for RETURN and link the block of
// RETURN with the block of the return address
//
void FlowGraph::handleReturn(Label_BB_Map& labelMap, FuncInfoHashTable& funcInfoHashTable)
{
    for (std::list<G4_BB*>::iterator it = BBs.begin(), itEnd = BBs.end(); it != itEnd; ++it)
    {
        G4_BB* bb = (*it);

        if (bb->isEndWithCall())
        {
            bb->setBBType(G4_BB_CALL_TYPE);
            G4_INST* last = bb->back();
            if (last->getSrc(0)->isLabel())
            {
                // make sure bb has only two successors, one subroutine and one return addr
                MUST_BE_TRUE1(bb->Succs.size() == 2, last->getLineNo(),
                    ERROR_FLOWGRAPH);

                // find the subroutine BB and return Addr BB
                G4_BB* subBB = labelMap[last->getSrc(0)->asLabel()];
                //
                // the fall through BB must be the front
                //
                G4_BB* retAddr = bb->Succs.front();
                if (retAddr->Preds.size() > 1)
                {
                    // Create an empty BB as a new RETURN BB as we want RETURN BB
                    // to be reached only by CALL BB.  Note that at this time, call
                    // return edge has not been added yet.
                    G4_INST* I0 = retAddr->getFirstInst();
                    if (I0 == nullptr) I0 = last;
                    G4_BB* newRetBB = createNewBBWithLabel("Label_return_BB", I0->getLineNo(), I0->getCISAOff());
                    bb->removeSuccEdge(retAddr);
                    retAddr->removePredEdge(bb);
                    addPredSuccEdges(bb, newRetBB, true);
                    addPredSuccEdges(newRetBB, retAddr, true);

                    insert(std::next(it), newRetBB);
                    retAddr = newRetBB;
                }
                // Add EXIT->RETURN edge.
                linkReturnAddr(subBB, retAddr);

                // set callee info for CALL
                FuncInfoHashTable::iterator calleeInfoLoc = funcInfoHashTable.find(subBB->getId());

                if (calleeInfoLoc != funcInfoHashTable.end())
                {
                    (*calleeInfoLoc).second->incrementCallCount();
                    bb->setCalleeInfo((*calleeInfoLoc).second);
                    doIPA = true;
                }
                else
                {
                    unsigned funcId = (unsigned)funcInfoHashTable.size();
                    FuncInfo *funcInfo = new (mem)FuncInfo(
                        funcId, subBB, retAddr->Preds.front());
                    std::pair<FuncInfoHashTable::iterator, bool> loc =
                        funcInfoHashTable.insert(
                            std::make_pair(subBB->getId(), funcInfo));
                    subBB->setBBType(G4_BB_INIT_TYPE);
                    retAddr->Preds.front()->setBBType(G4_BB_EXIT_TYPE);
                    MUST_BE_TRUE(loc.second, ERROR_FLOWGRAPH);
                    bb->setCalleeInfo((*(loc.first)).second);
                }
                retAddr->setBBType(G4_BB_RETURN_TYPE);
            }
        }

        if (bb->isEndWithFCall())
        {
            // normalizeFlowGraph() process FCALL BB. (Maybe should be processed here?)
            // Here just set BB type
            bb->setBBType(G4_BB_FCALL_TYPE);
        }
    }
    //
    // remove <CALL, return addr> link when it is not a conditional call
    //
    for (G4_BB* bb : BBs)
    {
        if (bb->isEndWithCall())
        {
            G4_INST* last = bb->back();
            if (last->getPredicate() == NULL)
            {
                MUST_BE_TRUE(!bb->Succs.empty(), ERROR_FLOWGRAPH);
                G4_BB* retAddr = bb->Succs.front();     // return BB must be the front
                bb->removeSuccEdge(retAddr);
                retAddr->removePredEdge(bb);
            }
        }
    }
}

void FlowGraph::linkReturnAddr(G4_BB* entryBB, G4_BB* returnAddr)
{

    assert(entryBB->size() > 0 && entryBB->front()->isLabel() && "BB should start with a label");
    auto label = entryBB->front()->getLabel();
    assert(subroutines.count(label) && "can't find subroutine label");
    auto subroutineBBs = subroutines[label];

    for (auto bb : subroutineBBs)
    {
        if (!bb->empty() && bb->back()->isReturn())
        {
            //
            // check the direct recursive call here!
            //
            if (bb == returnAddr && hasStackCalls == false)
            {
                MUST_BE_TRUE(false,
                    "ERROR: Do not support recursive subroutine call!");
            }
            addPredSuccEdges(bb, returnAddr, false); // IMPORTANT: add to the back to allow fall through edge is in the front, which is used by fallThroughBB()
        }
    }
}


//
// This phase iterates each BB and checks if the last inst of a BB is a "call foo".  If yes,
// the algorith traverses from the block of foo to search for RETURN. If multiple returns exist,
// the algorithm will merge returns into one
// [Reason]:to handle the case when spill codes are needed between multiple return BBs of one subroutine
//          and the afterCAll BB. It is impossible to insert spill codes of different return BBs all before
//          afterCall BB.
//
void FlowGraph::mergeReturn(FuncInfoHashTable& funcInfoHashTable)
{

    for (auto I = subroutines.begin(), E = subroutines.end(); I != E; ++I)
    {
        auto label = I->first;
        G4_BB* subBB = I->second.front();
        G4_BB* mergedExitBB = mergeSubRoutineReturn(label);

        // update callee exit block info
        FuncInfoHashTable::iterator calleeInfoLoc = funcInfoHashTable.find(subBB->getId());
        // it's possible for the subroutine to never be called (e.g., kernel function)
        if (calleeInfoLoc != funcInfoHashTable.end() && mergedExitBB)
        {
            calleeInfoLoc->second->getExitBB()->unsetBBType(G4_BB_EXIT_TYPE);
            mergedExitBB->setBBType(G4_BB_EXIT_TYPE);
            calleeInfoLoc->second->updateExitBB(mergedExitBB);
        }
    }
}

G4_BB* FlowGraph::mergeSubRoutineReturn(G4_Label* subroutineLabel)
{

    G4_BB* newBB = nullptr;

    auto subroutineBB = subroutines[subroutineLabel];

    std::vector<G4_BB*> retBBList;
    for (auto bb : subroutineBB)
    {
        if (!bb->empty() && bb->back()->isReturn())
        {
            retBBList.push_back(bb);
        }
    }

    if (retBBList.size() > 1)     // For return number >1, need to merge returns
    {
        builder->instList.clear();

        //
        // insert newBB to fg.BBs list
        //
        newBB = createNewBB();
        insert(BBs.end(), newBB);
        // choose the last BB in retBBList as a candidate
        G4_BB* candidateBB = *(retBBList.rbegin());
        // Add <newBB, succBB> edges
        G4_INST* last = candidateBB->back();
        BB_LIST_ITER succIt = (last->getPredicate() == NULL) ? candidateBB->Succs.begin() : (++candidateBB->Succs.begin());
        BB_LIST_ITER succItEnd = candidateBB->Succs.end();
        // link new ret BB with each call site
        for (; succIt != succItEnd; ++succIt) {
            addPredSuccEdges(newBB, (*succIt), false);
        }

        //
        // Create a label for the newBB and insert return to new BB
        //
        std::string str = "LABEL__" + std::to_string(newBB->getId());
        G4_Label* lab = builder->createLabel(str, LABEL_BLOCK);
        G4_INST* labelInst = createNewLabelInst(lab);

        // exitBB is really just a dummy one for analysis, and does not need a return
        // we will instead leave the return in each of its predecessors
        newBB->push_back(labelInst);

        //
        // Deal with all return BBs
        //
        for (G4_BB * retBB : retBBList)
        {
            if (retBB->getId() == newBB->getId())
            {
                continue;
            }
            last = retBB->back();
            // remove <retBB,its succBB> edges, do not remove the fall through edge if predicated
            BB_LIST retSuccsList;
            retSuccsList.assign(retBB->Succs.begin(), retBB->Succs.end());
            for (BB_LIST_ITER retSuccIt = retSuccsList.begin(); retSuccIt != retSuccsList.end(); ++retSuccIt)
            {
                G4_BB * retSuccBB = (*retSuccIt);
                if (last->getPredicate() != NULL && retSuccIt == retSuccsList.begin()) continue;
                retBB->removeSuccEdge(retSuccBB);
                retSuccBB->removePredEdge(retBB);
            }
            // Add <retBB,newBB> edges
            addPredSuccEdges(retBB, newBB, false);
        }
    }

    return newBB;
}

void FlowGraph::decoupleInitBlock(G4_BB* bb, FuncInfoHashTable& funcInfoHashTable)
{
    G4_BB* oldInitBB = bb;
    G4_BB* newInitBB = createNewBB();
    BB_LIST_ITER insertBefore = std::find(BBs.begin(), BBs.end(), bb);
    MUST_BE_TRUE(insertBefore != BBs.end(), ERROR_FLOWGRAPH);
    insert(insertBefore, newInitBB);

    BB_LIST_ITER kt = oldInitBB->Preds.begin();
    while (kt != oldInitBB->Preds.end())
    {
        // the pred of this new INIT BB are all call BB
        if ((*kt)->getBBType() & G4_BB_CALL_TYPE) {

            newInitBB->Preds.push_back((*kt));

            BB_LIST_ITER jt = (*kt)->Succs.begin();
            while (jt != (*kt)->Succs.end()) {
                if ((*jt) == oldInitBB)
                {
                    break;
                }
                jt++;
            }
            MUST_BE_TRUE(jt != (*kt)->Succs.end(), ERROR_FLOWGRAPH);
            (*kt)->Succs.insert(jt, newInitBB);
            (*kt)->Succs.erase(jt);

            BB_LIST_ITER tmp_kt = kt;
            ++kt;
            // erase this pred from old INIT BB's pred
            oldInitBB->Preds.erase(tmp_kt);
        }
        else
        {
            ++kt;
        }
    }

    FuncInfoHashTable::iterator old_iter = funcInfoHashTable.find(oldInitBB->getId());
    MUST_BE_TRUE(old_iter != funcInfoHashTable.end(), " Function info is not in hashtable.");
    FuncInfo* funcInfo = old_iter->second;

    // Erase the old item from unordered_map and add the new one.
    funcInfo->updateInitBB(newInitBB);
    funcInfoHashTable.erase(old_iter);
    funcInfoHashTable.insert(std::make_pair(newInitBB->getId(), funcInfo));

    oldInitBB->unsetBBType(G4_BB_INIT_TYPE);
    newInitBB->setBBType(G4_BB_INIT_TYPE);
    addPredSuccEdges(newInitBB, oldInitBB);

    std::string blockName = "LABEL__EMPTYBB__" + std::to_string(newInitBB->getId());
    G4_Label* label = builder->createLabel(blockName, LABEL_BLOCK);
    G4_INST* labelInst = createNewLabelInst(label);
    newInitBB->push_back(labelInst);
}

void FlowGraph::decoupleReturnBlock(G4_BB* bb)
{
    G4_BB* oldRetBB = bb;
    G4_BB* itsExitBB = oldRetBB->BBBeforeCall()->getCalleeInfo()->getExitBB();
    G4_BB* newRetBB = createNewBB();
    BB_LIST_ITER insertBefore = std::find(BBs.begin(), BBs.end(), bb);
    MUST_BE_TRUE(insertBefore != BBs.end(), ERROR_FLOWGRAPH);
    insert(insertBefore, newRetBB);

    std::replace(itsExitBB->Succs.begin(), itsExitBB->Succs.end(), oldRetBB, newRetBB);
    newRetBB->Preds.push_back(itsExitBB);
    newRetBB->Succs.push_back(oldRetBB);

    std::replace(oldRetBB->Preds.begin(), oldRetBB->Preds.end(), itsExitBB, newRetBB);
    oldRetBB->Preds.unique();

    oldRetBB->unsetBBType(G4_BB_RETURN_TYPE);
    newRetBB->setBBType(G4_BB_RETURN_TYPE);

    std::string str = "LABEL__EMPTYBB__" + std::to_string(newRetBB->getId());
    G4_Label* label = builder->createLabel(str, LABEL_BLOCK);
    G4_INST* labelInst = createNewLabelInst(label);
    newRetBB->push_back(labelInst);
}

// Make sure that a BB is at most one of CALL/RETURN/EXIT/INIT. If any
// BB is both, say INIT and CALL, decouple them by inserting a new BB.
// [The above comments are post-added from reading the code.]
//
// The inserted BB must be in the original place so that each subroutine
// has a list of consecutive BBs.
void FlowGraph::normalizeSubRoutineBB(FuncInfoHashTable& funcInfoTable)
{
    setPhysicalPredSucc();
    for (G4_BB* bb : BBs)
    {
        if ((bb->getBBType() & G4_BB_CALL_TYPE))
        {
            if (bb->getBBType() & G4_BB_EXIT_TYPE)
            {
                // As call BB has RETURN BB as fall-thru, cannot be EXIT
                MUST_BE_TRUE(false, ERROR_FLOWGRAPH);
            }

            // BB could be either INIT or RETURN, but not both.
            if (bb->getBBType() & G4_BB_INIT_TYPE)
            {
                decoupleInitBlock(bb, funcInfoTable);
            }
            else if (bb->getBBType() & G4_BB_RETURN_TYPE)
            {
                decoupleReturnBlock(bb);
            }
        }
        else if ((bb->getBBType() & G4_BB_INIT_TYPE))
        {
            if (bb->getBBType() & G4_BB_RETURN_TYPE)
            {
                // As retrun BB must have a pred, it cannot be init BB
                MUST_BE_TRUE(false, ERROR_FLOWGRAPH);
            }

            // Two possible combinations: INIT & CALL, or INIT & EXIT. INIT & CALL has
            // been processed in the previous IF, here only INIT and EXIT is possible.
            if (bb->getBBType() & G4_BB_EXIT_TYPE)
            {
                decoupleInitBlock(bb, funcInfoTable);
            }
        }
        else if ((bb->getBBType() & G4_BB_EXIT_TYPE))
        {
            // Only EXIT & RETURN are possible. (INIT & EXIT
            // has been processed)
            if (bb->getBBType() & G4_BB_RETURN_TYPE)
            {
                decoupleReturnBlock(bb);
            }
        }
        else if ((bb->getBBType() & G4_BB_RETURN_TYPE))
        {
            // Do we need to do this ?
            if (bb->Preds.size() > 1)
            {
                decoupleReturnBlock(bb);
            }
        }
    }
}

//
// This function does a DFS and any blocks that get visited will have their
// preId set according to the ordering. Blocks that never get visited will
// have their preId unmodified.
//
void doDFS(G4_BB* startBB, unsigned int p)
{
    unsigned int currP = p;
    std::stack<G4_BB*> traversalStack;
    traversalStack.push(startBB);

    while (!traversalStack.empty())
    {
        G4_BB* currBB = traversalStack.top();
        traversalStack.pop();
        if (currBB->getPreId() != UINT_MAX)
        {
            continue;
        }
        currBB->setPreId(currP++);

        for (G4_BB* tmp : currBB->Succs)
        {
            if (tmp->getPreId() == UINT_MAX)
            {
                traversalStack.push(tmp);
            }
        }
    }
}

//
// The optimization pass will remove unreachable blocks from functions. Later compilation phases
// assume that the only unreachable code that can exist in a function is the block with
// return/pseudo_fret instruction. All other unreachable code should be removed. The only time
// blocks with return/pseudo_fret will be removed is when the header of that function itself
// is deemed unreachable.
//
void FlowGraph::removeUnreachableBlocks(FuncInfoHashTable& funcInfoHT)
{
    unsigned preId = 0;
    //
    // initializations
    //
    for (G4_BB* bb : BBs)
    {
        bb->setPreId(UINT_MAX);
    }
    //
    // assign DFS based pre/rpost ids to all blocks in the main program
    //
    doDFS(getEntryBB(), preId);

    //
    // Basic blocks with preId/rpostId set to UINT_MAX are unreachable.
    // Special handling:
    //   1. If CALL_BB isn't dead, don't remove RETURN_BB;
    //   2. If INIT_BB isn't dead, don't remove EXIT_BB.
    //   3. For BB with EOT, don't remove it.
    // (Note that in BB list (BBs), CALL_BB appears before RETURN_BB and INIT_BB
    //  appears before EXIT_BB. The algo works under this assumpion.)
    BB_LIST_ITER it = BBs.begin();
    while (it != BBs.end())
    {
        G4_BB* bb = (*it);
        if (bb->getPreId() == UINT_MAX)
        {
            if (bb->getBBType() & G4_BB_INIT_TYPE)
            {
                // Remove it from funcInfoHT.
                int funcId = bb->getId();
                unsigned numErased = funcInfoHT.erase(funcId);
                assert(numErased == 1);
            }
            else if (bb->getBBType() & G4_BB_CALL_TYPE)
            {
                // If call bb is removed, its return BB shuld be removed as well.
                G4_BB* retBB = bb->getPhysicalSucc();
                assert(retBB && "vISA ICE: missing Return BB");
                if (retBB->getPreId() != UINT_MAX)
                {
                    retBB->setPreId(UINT_MAX);
                }
            }

            // EOT should be in entry function, we keep it for now.
            if (bb->size() > 0 && bb->back()->isEOT())
            {
                ++it;
                continue;
            }

            // Now, remove bb.
            while (bb->Succs.size() > 0)
            {
                removePredSuccEdges(bb, bb->Succs.front());
            }
            if (bb->getBBType() & G4_BB_RETURN_TYPE)
            {
                // As callee may be live, need to remove pred edges from exit BB.
                for (auto& II : bb->Preds)
                {
                    G4_BB* exitBB = II;
                    if (exitBB->getBBType() & G4_BB_EXIT_TYPE)
                    {
                        removePredSuccEdges(exitBB, bb);
                        break;
                    }
                }
            }

            BB_LIST_ITER prev = it;
            ++it;
            erase(prev);
        }
        else
        {
            if (bb->getBBType() & G4_BB_CALL_TYPE)
            {
                // CALL BB isn't dead, don't remove return BB.
                G4_BB* retBB = bb->getPhysicalSucc();
                assert(retBB && (retBB->getBBType() & G4_BB_RETURN_TYPE) &&
                    "vISA ICE: missing RETURN BB");
                if (retBB->getPreId() == UINT_MAX)
                {
                    // For example,
                    //    CALL_BB  : call A   succ: init_BB
                    //    RETURN_BB: pred: exit_BB
                    //
                    //    // subroutine A
                    //    init_BB:
                    //       while (true) ...;  // inifinite loop
                    //    exit_BB:  succ : RETURN_BB
                    // Both RETURN_BB and exit_BB are unreachable, but
                    // we keep both!
                    retBB->setPreId(UINT_MAX - 1);
                }
            }
            else if (bb->getBBType() & G4_BB_INIT_TYPE)
            {
                // function isn't dead, don't remove exit BB.
                int funcId = bb->getId();
                auto entry = funcInfoHT.find(funcId);
                assert(entry != funcInfoHT.end());
                FuncInfo* finfo = entry->second;
                G4_BB* exitBB = finfo->getExitBB();
                assert(exitBB && "vISA ICE: missing exit BB");
                if (exitBB->getPreId() == UINT_MAX)
                {
                    // See the example above for G4_BB_CALL_TYPE
                    exitBB->setPreId(UINT_MAX - 1);
                }
            }
            it++;
        }
    }
    reassignBlockIDs();
    setPhysicalPredSucc();
}

//
// Remove redundant goto/jmpi
// Remove the fall through edges between subroutine and its non-caller preds
// Remove basic blocks that only contain a label, function labels are untouched.
// Remove empty blocks.
//
void FlowGraph::removeRedundantLabels()
{
    // first,  remove redundant goto
    //    goto L0
    //      ....
    //    goto L1     <-- redundant goto
    // L0: <empty BB>
    // L1:
    BB_LIST_ITER Next = BBs.begin(), IE = BBs.end();
    for (BB_LIST_ITER II = Next; II != IE; II = Next)
    {
        ++Next;

        G4_BB* BB = *II;
        G4_opcode lastop = BB->getLastOpcode();
        if (BB->Succs.size() == 1 && (lastop == G4_goto || lastop == G4_jmpi))
        {
            G4_BB* SuccBB = BB->Succs.back();
            G4_BB* BB1 = nullptr;
            // Skip over empty BBs
            for (auto iter = Next; iter != IE; ++iter)
            {
                BB1 = *iter;
                if (BB1 != SuccBB &&
                    (BB1->empty() ||
                     (BB1->size() == 1 && BB1->getLastOpcode() == G4_label)))
                {
                    continue;
                }
                break;
            }
            if (BB1 && SuccBB == BB1)
            {
                // Remove goto and its fall-thru will be its succ.
                G4_BB* fallThruBB = *Next;
                if (fallThruBB != SuccBB)
                {
                    // Reconnect succ/pred for BB
                    removePredSuccEdges(BB, SuccBB);
                    addPredSuccEdges(BB, fallThruBB);
                }
                BB->pop_back();
            }
        }
    }

    for (BB_LIST_ITER nextit = BBs.begin(), ite = BBs.end(); nextit != ite; )
    {
        BB_LIST_ITER it = nextit++;
        G4_BB* bb = *it;

        if (bb == getEntryBB())
        {
            continue;
        }
        if (bb->Succs.size() == 0 && bb->Preds.size() == 0) {
            //leaving dangling BBs with return in for now.
            //workaround to handle unreachable return
            //for example return after infinite loop.
            if (bb->isEndWithFRet() || (bb->size() > 0 && ((G4_INST*)bb->back())->isReturn()))
            {
                continue;
            }

            bb->clear();
            erase(it);

            continue;
        }

        assert(bb->size() > 0 && bb->front()->isLabel() &&
               "Every BB should at least have a label inst!");

        // Possible kernel's entry, don't delete.
        if (strcmp(bb->front()->getLabelStr(), "per-thread-prolog") == 0)
        {
            continue;
        }

        if (bb->getBBType() &
            (G4_BB_CALL_TYPE | G4_BB_EXIT_TYPE | G4_BB_INIT_TYPE | G4_BB_RETURN_TYPE))
        {
            // Keep those BBs
            continue;
        }

        //
        // The removal candidates will have a single successor and a single inst
        //
        if (bb->Succs.size() == 1 && bb->size() == 1)
        {
            G4_INST* removedBlockInst = bb->front();
            if (removedBlockInst->getLabel()->isFuncLabel() ||
                strncmp(removedBlockInst->getLabelStr(), "LABEL__EMPTYBB", 14) == 0 ||
                strncmp(removedBlockInst->getLabelStr(), "__AUTO_GENERATED_DUMMY_LAST_BB", 30) == 0)
            {
                continue;
            }

            G4_Label *succ_label = bb->Succs.front()->front()->getLabel();

            // check if the last inst of pred is a control flow inst
            for (auto pred : bb->Preds)
            {
                auto jt = std::find(pred->Succs.begin(), pred->Succs.end(), bb);
                if (jt == pred->Succs.end())
                {
                    // Just skip!
                    //
                    // Each unique pred is processed just once.  If a pred appears
                    // more than once in bb->Preds, it is only handled the first time
                    // the pred is processed.
                    //   Note that if jt == end(), it means that this pred appears
                    //   more than once in the Preds list AND it has been handled
                    //   before (see code at the end of this loop). So, it is safe to skip.
                    continue;
                }

                G4_INST *i = pred->back();
                // replace label in instructions
                if (i->isFlowControl())
                {
                    if (isIndirectJmpTarget(i))
                    {
                        // due to the switchjmp we may have multiple jmpi
                        // at the end of a block.
                        bool foundMatchingJmp = false;
                        for (INST_LIST::iterator iter = --pred->end();
                            iter != pred->begin(); --iter)
                        {
                            i = *iter;
                            if (i->opcode() == G4_jmpi)
                            {
                                if (i->getSrc(0)->isLabel() &&
                                    i->getSrc(0) == removedBlockInst->getLabel())
                                {
                                    i->setSrc(succ_label, 0);
                                    foundMatchingJmp = true;
                                    break;
                                }
                            }
                            else
                            {
                                break;
                            }
                        }
                        MUST_BE_TRUE(foundMatchingJmp, "Can't find the matching jmpi to the given label");
                    }
                    else if (i->opcode() == G4_jmpi || i->isCall())
                    {
                        if (i->getSrc(0)->isLabel())
                        {
                            if (i->getSrc(0) == removedBlockInst->getLabel())
                            {
                                i->setSrc(succ_label, 0);
                            }
                        }
                    }
                    else if (i->opcode() == G4_if || i->opcode() == G4_while ||
                        i->opcode() == G4_else)
                    {
                        if (i->asCFInst()->getJip()->isLabel())
                        {
                            if (i->asCFInst()->getJip() == removedBlockInst->getLabel())
                            {
                                if (i->opcode() == G4_else || i->opcode() == G4_while)
                                {
                                    // for G4_while, jump no matter predicate
                                    i->asCFInst()->setJip(succ_label);
                                }
                                // for G4_if, jump only when it has predictate; if no predicate, no jump
                                else if (i->getPredicate())
                                {
                                    i->asCFInst()->setJip(succ_label);
                                }
                            }
                        }
                    }
                    else if (i->opcode() == G4_break || i->opcode() == G4_cont || i->opcode() == G4_halt)
                    {
                        // JIP and UIP must have been computed at this point
                        MUST_BE_TRUE(i->asCFInst()->getJip() != NULL && i->asCFInst()->getUip() != NULL,
                            "null JIP or UIP for break/cont instruction");
                        if (i->asCFInst()->getJip() == removedBlockInst->getLabel())
                        {
                            i->asCFInst()->setJip(succ_label);
                        }

                        if (i->asCFInst()->getUip() == removedBlockInst->getLabel())
                        {
                            i->asCFInst()->setUip(succ_label);
                        }
                    }
                    else if (i->opcode() == G4_goto)
                    {
                        // UIP must have been computed at this point
                        MUST_BE_TRUE(i->asCFInst()->getUip() != NULL,
                            "null UIP for goto instruction");
                        if (i->asCFInst()->getUip() == removedBlockInst->getLabel())
                        {
                            i->asCFInst()->setUip(succ_label);
                        }
                        if (i->asCFInst()->getUip() == removedBlockInst->getLabel())
                        {
                            i->asCFInst()->setUip(succ_label);
                        }
                    }
                }

                pred->Succs.insert(jt, bb->Succs.front());
                pred->Succs.erase(jt);

                // [Bug1915]: In rare case the precessor may have more than one Succ edge pointing
                // to the same BB, due to empty block being eliminated.  For example, with
                // BB1:
                // (P) jmp BB3
                // BB2:
                // BB3:
                // BB4:
                // ...
                // After BB2 is eliminated BB1's two succ will both point to BB3.
                // When we get rid of BB3,
                // we have to make sure we update both Succ edges as we'd otherwise create an
                // edge to a non-existing BB.  Note that we don't just delete the edge because
                // elsewhere there may be assumptions that if a BB ends with a jump it must have
                // two successors
                {
                    BB_LIST_ITER succs = pred->Succs.begin();
                    BB_LIST_ITER end = pred->Succs.end();
                    while (succs != end)
                    {
                        BB_LIST_ITER iter = succs;
                        ++succs;
                        if ((*iter) == bb)
                        {
                            pred->Succs.insert(iter, bb->Succs.front());
                            pred->Succs.erase(iter);
                        }
                    }
                }
            }

            //
            // Replace the unique successor's predecessor links with the removed block's predessors.
            //
            BB_LIST_ITER kt = std::find(bb->Succs.front()->Preds.begin(), bb->Succs.front()->Preds.end(), bb);

            BB_LIST_ITER mt = bb->Preds.begin();

            for (; mt != bb->Preds.end(); ++mt)
            {
                bb->Succs.front()->Preds.insert(kt, *mt);
            }

            bb->Succs.front()->Preds.erase(kt);
            bb->Succs.front()->Preds.unique();
            //
            // Propagate the removed block's type to its unique successor.
            //
            bb->Succs.front()->setBBType(bb->getBBType());

            bb->Succs.clear();
            bb->Preds.clear();
            bb->clear();

            erase(it);
        }
        else if (bb->Preds.size() == 1 && bb->Preds.front()->Succs.size() == 1)
        {
            // Merge bb into singlePred and delete bb if all the following are true:
            //   1. singlePred has no control-flow inst (at the end),
            //   2. bb's label is not used at all.
            //
            //     singlePred:
            //        ....
            //     bb:
            //        ....
            //
            // If singlePred does not end with a control-flow inst, bb's label is not used except
            // bb ends with while. For while bb, we need further to check if any break uses label.
            // As break should jump to while bb's fall-thru (not while bb), we need to follow all
            // preds of while's fall-thru BB to see if any pred has a break.
            //
            G4_BB* singlePred = bb->Preds.front();
            G4_INST* labelInst = bb->front();
            assert(labelInst->isLabel());
            if (!singlePred->back()->isFlowControl() &&
                singlePred->getPhysicalSucc() == bb /* sanity */ &&
                !labelInst->getLabel()->isFuncLabel() /* skip special bb */ &&
                bb != singlePred /* [special] skip dead single-BB loop */)
            {
                bool doMerging = true;
                G4_INST* whileInst = bb->back();
                if (whileInst->opcode() == G4_while)
                {
                    // If there is any break inst for this while, the break uses the label. No merging.
                    // Note that any break inst of this while will jump to the BB right after while BB
                    // (this BB is the fall-thru BB of while if while BB has fall-thru, or just its physical
                    // succ if it has no fall-thru).
                    G4_BB* whilePhySucc = bb->getPhysicalSucc();
                    if (whilePhySucc)
                    {
                        for (auto breakPred : whilePhySucc->Preds)
                        {
                            if (breakPred->getLastOpcode() == G4_break) {
                                doMerging = false;
                                break;
                            }
                        }
                    }
                }

                if (doMerging)
                {
                    removePredSuccEdges(singlePred, bb);
                    assert(singlePred->Succs.size() == 0);
                    std::vector<G4_BB*> allSuccBBs(bb->Succs.begin(), bb->Succs.end());
                    for (auto S : allSuccBBs)
                    {
                        removePredSuccEdges(bb, S);
                        addPredSuccEdges(singlePred, S, false);
                    }

                    // remove bb's label before splice
                    bb->remove(labelInst);
                    singlePred->getInstList().splice(singlePred->end(), bb->getInstList());

                    bb->Succs.clear();
                    bb->Preds.clear();

                    erase(it);
                }
            }
        }
    }

    reassignBlockIDs();
}

//
// remove any mov with the same src and dst opnds
//
void FlowGraph::removeRedundMov()
{
    for (G4_BB* bb : BBs)
    {
        INST_LIST_ITER curr_iter = bb->begin();
        while (curr_iter != bb->end())
        {
            G4_INST* inst = (*curr_iter);
            if (inst->isRawMov())
            {
                G4_Operand *src = inst->getSrc(0);
                G4_DstRegRegion *dst = inst->getDst();

                if (src->isSrcRegRegion())
                {
                    G4_SrcRegRegion* srcRgn = (G4_SrcRegRegion*)src;
                    if (!dst->isIndirect() &&
                        !srcRgn->isIndirect() &&
                        dst->isGreg() &&
                        src->isGreg())
                    {
                        if (dst->getLinearizedStart() == srcRgn->getLinearizedStart() &&
                            dst->getLinearizedEnd() == srcRgn->getLinearizedEnd())
                        {
                            uint16_t stride = 0;
                            const RegionDesc *rd = srcRgn->getRegion();
                            unsigned ExSize = inst->getExecSize();
                            if (ExSize == 1 ||
                                (rd->isSingleStride(ExSize, stride) &&
                                (dst->getHorzStride() == stride)))
                            {
                                curr_iter = bb->erase(curr_iter);
                                continue;
                            }
                        }
                    }
                }
            }
            ++curr_iter;
        }
    }
}

//
// Remove any placeholder empty blocks that could have been inserted to aid analysis
//
void FlowGraph::removeEmptyBlocks()
{
    bool changed = true;

    while (changed)
    {
        changed = false;
        for (BB_LIST_ITER it = BBs.begin(); it != BBs.end();)
        {
            G4_BB* bb = *it;

            //
            // The removal candidates will have a unique successor and a single label
            // starting with LABEL__EMPTYBB as the only instruction besides a JMP.
            //
            if (bb->size() > 0 && bb->size() < 3)
            {
                INST_LIST::iterator removedBlockInst = bb->begin();

                if ((*removedBlockInst)->isLabel() == false ||
                    strncmp((*removedBlockInst)->getLabelStr(),
                        "LABEL__EMPTYBB", 14) != 0)
                {
                    ++it;
                    continue;
                }

                ++removedBlockInst;

                if (removedBlockInst != bb->end())
                {
                    // if the BB is not empty, it must end with a unconditional jump
                    if ((*removedBlockInst)->opcode() != G4_jmpi || bb->Succs.size() > 1)
                    {
                        ++it;
                        continue;
                    }
                }

                // remove redundant succs and preds for the empty block
                bb->Succs.unique();
                bb->Preds.unique();

                for (auto predBB : bb->Preds)
                {
                    //
                    // Replace the predecessors successor links to the removed block's unique successor.
                    //
                    BB_LIST_ITER jt = std::find(predBB->Succs.begin(), predBB->Succs.end(), bb);

                    for (auto succBB : bb->Succs)
                    {
                        predBB->Succs.insert(jt, succBB);
                    }
                    predBB->Succs.erase(jt);
                    predBB->Succs.unique();
                }

                for (auto succBB : bb->Succs)
                {
                    //
                    // Replace the unique successor's predecessor links with the removed block's predessors.
                    //
                    BB_LIST_ITER kt = std::find(succBB->Preds.begin(), succBB->Preds.end(), bb);

                    if (kt != succBB->Preds.end())
                    {
                        for (auto predBB : bb->Preds)
                        {
                            succBB->Preds.insert(kt, predBB);
                        }

                        succBB->Preds.erase(kt);
                        succBB->Preds.unique();

                        //
                        // Propagate the removed block's type to its unique successor.
                        //
                        succBB->setBBType(bb->getBBType());
                    }
                }
                //
                // Remove the block to be removed.
                //
                bb->Succs.clear();
                bb->Preds.clear();
                bb->clear();

                BB_LIST_ITER rt = it++;
                erase(rt);
                changed = true;
            }
            else
            {
                ++it;
            }
        }
    }
}

//
// If multiple freturns exist in a flowgraph create a new basic block
// with an freturn. Replace all freturns with jumps.
//
void FlowGraph::mergeFReturns()
{
    std::list<G4_BB*> exitBBs;
    G4_BB* candidateFretBB = NULL;
    G4_Label *dumLabel = NULL;

    for (G4_BB* cur : BBs)
    {
        if (cur->size() > 0 && cur->back()->isFReturn())
        {
            exitBBs.push_back(cur);

            if (cur->size() == 2 && cur->front()->isLabel())
            {
                // An fret already exists that can be shared among all
                // so skip creating a new block with fret.
                dumLabel = cur->front()->getSrc(0)->asLabel();
                candidateFretBB = cur;
            }
        }
    }

    if (exitBBs.size() > 1)
    {
        if (candidateFretBB == NULL)
        {
            G4_BB* newExit = createNewBB();
            assert(!builder->getIsKernel() && "Not expecting fret in kernel");
            std::string str = "__MERGED_FRET_EXIT_BLOCK";
            dumLabel = builder->createLabel(str, LABEL_BLOCK);
            G4_INST* label = createNewLabelInst(dumLabel);
            newExit->push_back(label);
            G4_INST* fret = builder->createInternalCFInst(
                nullptr, G4_pseudo_fret, g4::SIMD1, nullptr, nullptr, InstOpt_NoOpt);
            newExit->push_back(fret);
            BBs.push_back(newExit);
            candidateFretBB = newExit;
        }

        for (G4_BB* cur : exitBBs)
        {
            if (cur != candidateFretBB)
            {
                G4_INST* last = cur->back();
                addPredSuccEdges(cur, candidateFretBB);

                last->setOpcode(G4_jmpi);
                last->setSrc(dumLabel, 0);
                last->setExecSize(g4::SIMD1);
            }
        }
    }
}


//
// Add a dummy BB for multiple-exit flow graph
// The criteria of a valid multiple-exit flow graph is:
//  (1). equal or less than one BB w/o successor has non-EOT last instruction;
//  (2). Other BBs w/o successor must end with EOT
//
void FlowGraph::linkDummyBB()
{
    //
    // check the flow graph to find if it satify the criteria and record the exit BB
    //
    std::list<G4_BB*> exitBBs;
    int nonEotExitBB = 0;
    for (G4_BB *bb : BBs)
    {
        if (bb->Succs.empty())
        {
            exitBBs.push_back(bb);      // record exit BBs
            G4_INST* last = bb->back();
            if (last == NULL)
            {
                MUST_BE_TRUE(false, "ERROR: Invalid flow graph with empty exit BB!");
            }
            if (!bb->isLastInstEOT())
            {
                nonEotExitBB++;
                if (nonEotExitBB > 1)
                {
                    MUST_BE_TRUE(false,
                        "ERROR: Invalid flow graph with more than one exit BB not end with EOT!");
                }
            }
        }
    }

    //
    // create the dummy BB and link the exit BBs to it
    //
    if (nonEotExitBB == 1 && exitBBs.size() > 1)
    {
        G4_BB *dumBB = createNewBB();
        MUST_BE_TRUE(dumBB != NULL, ERROR_FLOWGRAPH);
        std::string str("__AUTO_GENERATED_DUMMY_LAST_BB");
        G4_Label *dumLabel = builder->createLabel(str, LABEL_BLOCK);
        G4_INST* label = createNewLabelInst(dumLabel);
        dumBB->push_back(label);
        BBs.push_back(dumBB);

        for (G4_BB *bb : exitBBs)
        {
            dumBB->Preds.push_back(bb);
            bb->Succs.push_back(dumBB);
        }
    }
}

//
// Re-assign block ID so that we can use id to determine the ordering of two blocks in the code layout
//
void FlowGraph::reassignBlockIDs()
{
    //
    // re-assign block id so that we can use id to determine the ordering of
    // two blocks in the code layout; namely which one is ahead of the other.
    // Important: since we re-assign id, there MUST NOT exist any instruction
    // that depends on BB id. Or the code will be incorrect once we reassign id.
    //
    unsigned i = 0;
    for (G4_BB* bb : BBs)
    {
        bb->setId(i);
        i++;
        MUST_BE_TRUE(i <= getNumBB(), ERROR_FLOWGRAPH);
    }

    numBBId = i;
}

G4_BB* FlowGraph::findLabelBB(
    BB_LIST_ITER StartIter, BB_LIST_ITER EndIter, const char* Label)
{
    for (BB_LIST_ITER it = StartIter; it != EndIter; ++it)
    {
        G4_BB* bb = *it;
        G4_INST* first = bb->empty() ? NULL : bb->front();

        if (first && first->isLabel())
        {
            const char* currLabel = first->getLabelStr();
            if (strcmp(Label, currLabel) == 0)
            {
                return bb;
            }
        }
    }
    return NULL;
}


typedef enum
{
    STRUCTURED_CF_IF = 0,
    STRUCTURED_CF_LOOP = 1
} STRUCTURED_CF_TYPE;

struct StructuredCF
{
    STRUCTURED_CF_TYPE mType;
    // for if this is the block that ends with if
    // for while this is the loop block
    G4_BB* mStartBB;
    // for if this is the endif block
    // for while this is the block that ends with while
    G4_BB* mEndBB;
    // it's possible for a BB to have multiple endifs, so we need
    // to know which endif corresponds to this CF
    G4_INST* mEndInst;

    StructuredCF* enclosingCF;

    // ToDo: can add more information (else, break, cont, etc.) as needed later

    // endBB is set when we encounter the endif/while
    StructuredCF(STRUCTURED_CF_TYPE type, G4_BB* startBB) :
        mType(type), mStartBB(startBB), mEndBB(NULL), mEndInst(NULL), enclosingCF(NULL) {}

    void *operator new(size_t sz, vISA::Mem_Manager& m) { return m.alloc(sz); }

    void setEnd(G4_BB* endBB, G4_INST* endInst)
    {
        mEndBB = endBB;
        mEndInst = endInst;
    }
}; // StructuredCF

/*
*  This function sets the JIP for Structured CF (SCF) instructions, Unstructured
*  Control Flow (UCF) instructions (goto) are handled in processGoto().
*
*  Note: we currently do not consider goto/join when adding the JIP for endif,
*  since structure analysis should not allow goto into/out of if-endif.  This means
*  we only need to set the the JIP of the endif to its immediately enclosing endif/while
*
*  The simd control flow blocks must be well-structured
*
*/
void FlowGraph::processSCF(FuncInfoHashTable &FuncInfoMap)
{
    std::stack<StructuredCF*> ifAndLoops;
    std::vector<StructuredCF*> structuredSimdCF;

    for (G4_BB* bb : BBs)
    {
        for (G4_INST* inst : *bb)
        {
            // check if first non-label inst is an endif
            if (inst->opcode() != G4_label && inst->opcode() != G4_join)
            {
                if (inst->opcode() == G4_endif)
                {

                    MUST_BE_TRUE(ifAndLoops.size() > 0, "endif without matching if");
                    StructuredCF* cf = ifAndLoops.top();
                    MUST_BE_TRUE(cf->mType == STRUCTURED_CF_IF, "ill-formed if");
                    cf->setEnd(bb, inst);
                    ifAndLoops.pop();
                    if (ifAndLoops.size() > 0)
                    {
                        cf->enclosingCF = ifAndLoops.top();
                    }
                }
                else
                {
                    // stop at the first non-endif instruction (there may be multiple endifs)
                    break;
                }
            }
        }

        // check if bb is SIMD loop head
        for (G4_BB* predBB : bb->Preds)
        {
            // check if one of the pred ends with a while
            if (predBB->getId() >= bb->getId())
            {
                if (predBB->size() != 0 &&
                    predBB->back()->opcode() == G4_while)
                {
                    StructuredCF* cf = new (mem)StructuredCF(STRUCTURED_CF_LOOP, bb);
                    structuredSimdCF.push_back(cf);
                    ifAndLoops.push(cf);
                }
            }
        }

        if (bb->size() > 0)
        {
            G4_INST* lastInst = bb->back();
            if (lastInst->opcode() == G4_if)
            {
                StructuredCF* cf = new (mem)StructuredCF(STRUCTURED_CF_IF, bb);
                structuredSimdCF.push_back(cf);
                ifAndLoops.push(cf);
            }
            else if (lastInst->opcode() == G4_while)
            {
                MUST_BE_TRUE(ifAndLoops.size() > 0, "while without matching do");
                StructuredCF* cf = ifAndLoops.top();
                MUST_BE_TRUE(cf->mType == STRUCTURED_CF_LOOP, "ill-formed while loop");
                cf->setEnd(bb, lastInst);
                ifAndLoops.pop();
                if (ifAndLoops.size() > 0)
                {
                    cf->enclosingCF = ifAndLoops.top();
                }
            }
        }
    }

    MUST_BE_TRUE(ifAndLoops.size() == 0, "not well-structured SIMD CF");

    for (StructuredCF* cf : structuredSimdCF)
    {
        if (cf->mType == STRUCTURED_CF_IF && cf->enclosingCF != NULL)
        {
            setJIPForEndif(cf->mEndInst, cf->enclosingCF->mEndInst, cf->enclosingCF->mEndBB);
        }
    }
}

// markDivergentBBs()
//    If BB is on the divergent path, mark it as divergent.
// Divergent:
//    If all active simd lanes on entry to shader/kernel are
//    active in a BB,  that BB is NOT divergent;  otherwise,
//    it is divergent.
//
//    Note that this does not require the number of all the
//    active simd lanes equals to the simd dispatch width!
//    For example,  simd8 kernel might have 4 active lanes
//    on entry to the kernel, thus if any BB of the kernel
//    has less than 4 active lanes,  it is divergent! As
//    matter of fact, the entry BB must not be divergent.
//
void FlowGraph::markDivergentBBs()
{
    if (BBs.empty())
    {
        // Sanity check
        return;
    }

    // fcall'ed Function:  to be conservative and assume that any function
    // that is fcall'ed is divergent on entry
    // (Note that each function has its own CFG)
    if (builder->getIsFunction())
    {
        for (G4_BB* BB : BBs)
        {
            BB->setDivergent(true);
        }
        return;
    }

    //  Now, handle kernel function.
    //       1.  It would be perfered to have a single return/exit for kernel and
    //           and all its subroutines in the last BB (IGC does, cm does not),
    //           although the code  can handle return in the middle of kernel.
    //       2.  The entry function will appear first in the BB list, and if there
    //           is a call from A to B,   subroutine A shall appear prior to
    //           subroutine B in BB list.
    //
    // Required:  need to set BB id.
    //
    // Key variables:
    //   LastJoinBB:
    //     LastJoinBB is the fartherest joinBB of any goto/break/if/while
    //     so far, as described below in the algorithm.
    //   LastJoinBBId:  Id(LastJoinBB).
    //
    // The algorithm initializes LastJoinBBId to -1 and scans all BBs in order.
    // It checks control-flow instructions to see if it diverges (has join).
    // If so, the algorithm sets LastJoinBBId to be the larger one of its join BB
    // and LastJoinBBId; For a non-negative LastJoinBBId, it means that there is
    // an active join in that BB, and therefore, all BBs from the current BB to
    // that LastJoinBB (LastJoinBB not included) are divergent.
    //
    // The algorithm checks the following cases and their join BBs are:
    //     case 1: cf inst = goto
    //          <currBB>    [(p)] goto L
    //                           ...
    //          <joinBB>    L:
    //     case 2: cf inst = if
    //          <currBB>    if
    //                          ...
    //          <joinBB>    endif
    //
    //     case 3:  cf inst = break
    //          <currBB>   break
    //                     ...
    //                     [(p)] while
    //          <joinBB>
    //     case 4:
    //          <currBB>  L:
    //                     ....
    //                    [(p)] while/goto L
    //          <joinBB>
    //
    //  The presence of loop makes the checking complicated. Before checking the
    //  divergence of a loop head, we need to know if the loop has ANY NON-UNIFORM
    //  OUT-OF-LOOP BRANCH or OUT-OF-LOOP BRANCH in DIVERGENT BB, which includes
    //  checking the loop's backedge and any out-going branches inside the loop body.
    //  For example,
    //      L0:
    //          B0: (uniform cond) goto B1
    //               ......
    //          B1:
    //             B2:
    //      L1:
    //             B3: goto OUT
    //             B4: (non-uniform cond) goto  L1
    //             B5:
    //          B6: (uniform cond) goto L0
    //
    //     OUT: B7:
    //
    //  At B0, we don't know whether B0 is divergent even though B0's goto is uniform. Once
    //  we find out that B3 is divergent, "goto OUT" will make Loop L0 divergent, which means
    //  any of L0's BBs, including B0, is divergent. In order to know B3 is divergent, we need
    //  to check the back branch of loop L1. The fact that L1's backedge is non-uniform indicates
    //  L1 is divergent, therefore B3 is divergent. This means that WE NEED TO DO PRE_SCAN in
    //  order to know whether any BBs of a loop is divergent.
    //
    int LastJoinBBId;

    auto pushJoin = [&](G4_BB* joinBB) {
        LastJoinBBId = std::max(LastJoinBBId, (int)joinBB->getId());
    };
    auto popJoinIfMatch = [&](G4_BB* BB) {
        if ((int)BB->getId() == LastJoinBBId) {
            LastJoinBBId = -1;
        }
    };
    auto isPriorToLastJoin = [&](G4_BB* aBB) ->bool {
        return (int)aBB->getId() < LastJoinBBId;
    };

    reassignBlockIDs();

    // Analyze function in topological order. As there is no recursion
    // and no indirect call,  a function will be analyzed only if all
    // its callers have been analyzed.
    //
    // If no subroutine, sortedFuncTable is empty. Here keep all functions
    // in a vector first (it works with and without subroutines), then scan
    // functions in topological order.
    struct StartEndIter {
        BB_LIST_ITER StartI;
        BB_LIST_ITER EndI;
        bool  InvokedFromDivergentBB;
    };
    int numFuncs = (int)sortedFuncTable.size();
    std::vector<StartEndIter> allFuncs;
    std::unordered_map<FuncInfo*, uint32_t> funcInfoIndex;
    if (numFuncs == 0)
    {
        numFuncs = 1;
        allFuncs.resize(1);
        allFuncs[0].StartI = BBs.begin();
        allFuncs[0].EndI = BBs.end();
        allFuncs[0].InvokedFromDivergentBB = false;
    }
    else
    {
        allFuncs.resize(numFuncs);
        for (int i = numFuncs; i > 0; --i)
        {
            uint32_t ix = (uint32_t)(i - 1);
            FuncInfo* pFInfo = sortedFuncTable[ix];
            G4_BB* StartBB = pFInfo->getInitBB();
            G4_BB* EndBB = pFInfo->getExitBB();
            uint32_t ui = (uint32_t)(numFuncs - i);
            allFuncs[ui].StartI = std::find(BBs.begin(), BBs.end(), StartBB);
            auto nextI = std::find(BBs.begin(), BBs.end(), EndBB);
            assert(nextI != BBs.end() && "ICE: subroutine's end BB not found!");
            allFuncs[ui].EndI = (++nextI);
            allFuncs[ui].InvokedFromDivergentBB = false;

            funcInfoIndex[pFInfo] = ui;
        }
    }

    // Update LastJoin for the backward branch of BB referred to by IT.
    auto updateLastJoinForBwdBrInBB = [&](BB_LIST_ITER& IT)
    {
        G4_BB* predBB = *IT;
        G4_INST* bInst = predBB->back();
        assert(bInst->isCFInst() &&
            (bInst->opcode() == G4_while || bInst->asCFInst()->isBackward()) &&
            "ICE: expected backward branch/while!");

        // joinBB of a loop is the BB right after tail BB
        BB_LIST_ITER loopJoinIter = IT;
        ++loopJoinIter;
        if (loopJoinIter == BBs.end())
        {
            // Loop end is the last BB (no Join BB). This happens in the
            // following cases:
            //    1. For CM, CM's return is in the middle of code! For IGC,
            //       this never happen as a return, if present, must be in
            //       the last BB.
            //    2. The last segment code is an infinite loop (static infinite
            //       loop so that compiler knows it is an infinite loop).
            // In either case, no join is needed.
            return;
        }

        // Update LastJoin if the branch itself is divergent.
        // (todo: checking G4_jmpi is redundant as jmpi must have isUniform()
        //  return true.)
        if ((!bInst->asCFInst()->isUniform() && bInst->opcode() != G4_jmpi))
        {
            G4_BB* joinBB = *loopJoinIter;
            pushJoin(joinBB);
        }
        return;
    };

    // Update LastJoin for BB referred to by IT. It is called either from normal
    // scan (setting BB's divergent field) or from loop pre-scan (no setting to
    // BB's divergent field).  IsPreScan indicates whether it is called from
    // the pre-scan or not.
    //
    // The normal scan (IsPreScan is false), this function also update the divergence
    // info for any called subroutines.
    //
    auto updateLastJoinForFwdBrInBB = [&](BB_LIST_ITER& IT, bool IsPreScan)
    {
        G4_BB* aBB = *IT;
        if (aBB->size() == 0)
        {
            return;
        }

        // Using isPriorToLastJoin() works for both loop pre-scan and normal scan.
        // (aBB->isDivergent() works for normal scan only.)
        bool isBBDivergent = isPriorToLastJoin(aBB);

        G4_INST* lastInst = aBB->back();
        if (((lastInst->opcode() == G4_goto && !lastInst->asCFInst()->isBackward()) ||
             lastInst->opcode() == G4_break) &&
            (!lastInst->asCFInst()->isUniform() || isBBDivergent))
        {
            // forward goto/break : the last Succ BB is our target BB
            // For break, it should be the BB right after while inst.
            G4_BB* joinBB = aBB->Succs.back();
            pushJoin(joinBB);
        }
        else if (lastInst->opcode() == G4_if &&
            (!lastInst->asCFInst()->isUniform() || isBBDivergent))
        {
            G4_Label* labelInst = lastInst->asCFInst()->getUip();
            G4_BB* joinBB = findLabelBB(IT, BBs.end(), labelInst->getLabel());
            assert(joinBB && "ICE(vISA) : missing endif label!");
            pushJoin(joinBB);
        }
        else if (!IsPreScan && lastInst->opcode() == G4_call)
        {
            // If this function is already in divergent branch, the callee
            // must be in a divergent branch!.
            if (isBBDivergent || lastInst->getPredicate() != nullptr)
            {
                FuncInfo* calleeFunc = aBB->getCalleeInfo();
                if (funcInfoIndex.count(calleeFunc))
                {
                    int ix = funcInfoIndex[calleeFunc];
                    allFuncs[ix].InvokedFromDivergentBB = true;
                }
            }
        }
        return;
    };


    // Now, scan all subroutines (kernels or functions)
    for (int i = 0; i < numFuncs; ++i)
    {
        // each function: [IT, IE)
        BB_LIST_ITER& IS = allFuncs[i].StartI;
        BB_LIST_ITER& IE = allFuncs[i].EndI;
        if (IS == IE)
        {
            // sanity check
            continue;
        }

        if (allFuncs[i].InvokedFromDivergentBB)
        {
            // subroutine's divergent on entry. Mark every BB as divergent
            for (auto IT = IS; IT != IE; ++IT)
            {
                G4_BB* BB = *IT;

                BB->setDivergent(true);
                if (BB->size() == 0)
                {
                    // sanity check
                    continue;
                }
                if (BB->isEndWithCall() || BB->isEndWithFCall())
                {
                    FuncInfo* calleeFunc = BB->getCalleeInfo();
                    if (funcInfoIndex.count(calleeFunc))
                    {
                        int ix = funcInfoIndex[calleeFunc];
                        allFuncs[ix].InvokedFromDivergentBB = true;
                    }
                }
            }
            // continue for next subroutine
            continue;
        }

        // Scaning all BBs of a single subroutine (or kernel, or function).
        LastJoinBBId = -1;
        for (auto IT = IS; IT != IE; ++IT)
        {
            G4_BB* BB = *IT;

            // join could be: explicit (join inst) or implicit (endif/while)
            popJoinIfMatch(BB);

            // Handle loop
            //    Loop needs to be scanned twice in order to get an accurate marking.
            //    In pre-scan (1st scan), it finds out divergent out-of-loop branch.
            //    If found,  it updates LastJoin to the target of that out-of-loop branch
            //    and restart the normal scan. If not, it restarts the normal scan with
            //    the original LastJoin unchanged.

            // BB could be head of several loops, and the following does pre-scan for
            //  every one of those loops.
            for (G4_BB* predBB : BB->Preds)
            {
                if (!isBackwardBranch(predBB, BB)) {
                    G4_opcode t_opc = predBB->getLastOpcode();
                    bool isBr = (t_opc == G4_goto || t_opc == G4_jmpi);
                    assert((!isBr || (BB->getId() > predBB->getId())) && "backward Branch did not set correctly!");
                    continue;
                }
                assert(BB->getId() <= predBB->getId() && "Branch incorrectly set to be backward!");

                BB_LIST_ITER LoopITEnd = std::find(BBs.begin(), BBs.end(), predBB);
                updateLastJoinForBwdBrInBB(LoopITEnd);

                // If lastJoin is already after loop end, no need to scan loop twice
                // as all BBs in the loop must be divergent
                if (isPriorToLastJoin(predBB))
                {
                    continue;
                }

                // pre-scan loop (BB, predBB)
                //
                //    Observation:
                //        pre-scan loop once and as a BB is scanned. Each backward
                //        branch to this BB and a forward branch from this BB are processed.
                //        Doing so finds out any divergent out-of-loop branch iff the
                //        loop has one. In addition, if a loop has more than one divergent
                //        out-of-loop branches, using any of those branches would get us
                //        precise divergence during the normal scan.
                //
                // LastJoinBBId will be updated iff there is an out-of-loop branch that is
                // is also divergent. For example,
                //  a)  "goto OUT" is out-of-loop branch, but not divergent. Thus, LastJoinBB
                //      will not be updated.
                //
                //      L :
                //           B0
                //           if (uniform cond) goto OUT;
                //           if  (non-uniform cond)
                //              B1
                //           else
                //              B2
                //           B3
                //           (p) jmpi L
                //      OUT:
                //
                //  b)  "goto OUT" is out-of-loop branch and divergent. Thus, update LastJoinBB.
                //      Note that "goto OUT" is divergent because loop L1 is divergent, which
                //      makes every BB in L1 divergent. And any branch from a divergent BB
                //      must be divergent.
                //
                //      L :
                //           B0
                //      L1:
                //           B1
                //           if (uniform cond) goto OUT;
                //           if  (cond)
                //              B2
                //           else
                //              B3
                //           if (non-uniform cond) goto L1
                //           B4
                //           (uniform cond) while L
                //      OUT:
                //
                int orig_LastJoinBBId = LastJoinBBId;
                for (auto LoopIT = IT; LoopIT != LoopITEnd; ++LoopIT)
                {
                    if (LoopIT != IT)
                    {
                        // Check loops that are fully inside the current loop.
                        G4_BB* H = *LoopIT;
                        for (G4_BB* T : H->Preds)
                        {
                            if (!isBackwardBranch(T, H)) {
                                continue;
                            }
                            assert(H->getId() <= T->getId() && "Branch incorrectly set to be backward!");
                            BB_LIST_ITER TIter = std::find(BBs.begin(), BBs.end(), T);
                            updateLastJoinForBwdBrInBB(TIter);
                        }
                    }
                    updateLastJoinForFwdBrInBB(LoopIT, true);
                }

                // After scan, if no branch out of loop, restore the original LastJoinBBId
                if (!isPriorToLastJoin(predBB))
                {   // case a) above.
                    LastJoinBBId = orig_LastJoinBBId;
                }
            }

            // normal scan of BB
            if (isPriorToLastJoin(BB)) {
                BB->setDivergent(true);
            }
            // Temporary for debugging, toBeDelete!
            // if (pKernel->getIntKernelAttribute(Attributes::ATTR_Target) == VISA_CM)
            // {
            //    assert((BB->isDivergent() && BB->isInSimdFlow() ||
            //        (!BB->isDivergent() && !BB->isInSimdFlow())) && " isDivergent != isInSimdFlow!");
            // }
            updateLastJoinForFwdBrInBB(IT, false);
        }
    }
    return;
}

G4_BB* FlowGraph::getUniqueReturnBlock()
{
    // Return block that has a return instruction
    // Return NULL if multiple return instructions found
    G4_BB* uniqueReturnBlock = NULL;

    for (G4_BB* curBB : BBs) {
        if (!curBB->empty())
        {
            G4_INST* last_inst = curBB->back();

            if (last_inst->opcode() == G4_pseudo_fret) {
                if (uniqueReturnBlock == NULL) {
                    uniqueReturnBlock = curBB;
                }
                else {
                    uniqueReturnBlock = NULL;
                    break;
                }
            }
        }
    }

    return uniqueReturnBlock;
}

/*
* Insert a join at the beginning of this basic block, immediately after the label
* If a join is already present, nothing will be done
*/
void FlowGraph::insertJoinToBB(G4_BB* bb, G4_ExecSize execSize, G4_Label* jip)
{
    MUST_BE_TRUE(bb->size() > 0, "empty block");
    INST_LIST_ITER iter = bb->begin();

    // Skip label if any.
    if ((*iter)->isLabel())
    {
        iter++;
    }

    if (iter == bb->end())
    {
        // insert join at the end
        G4_INST* jInst = builder->createInternalCFInst(NULL, G4_join, execSize, jip, NULL, InstOpt_NoOpt);
        bb->push_back(jInst, false);
    }
    else
    {
        G4_INST* secondInst = *iter;

        if (secondInst->opcode() == G4_join)
        {
            if (execSize > secondInst->getExecSize())
            {
                secondInst->setExecSize(execSize);
            }
        }
        else
        {
            G4_INST* jInst = builder->createInternalCFInst(NULL, G4_join, execSize, jip, NULL, InstOpt_NoOpt);
            bb->insertBefore(iter, jInst, false);
        }
    }
}

typedef std::pair<G4_BB*, G4_ExecSize> BlockSizePair;

static void addBBToActiveJoinList(std::list<BlockSizePair>& activeJoinBlocks, G4_BB* bb, G4_ExecSize execSize)
{
    // add goto target to list of active blocks that need a join
    std::list<BlockSizePair>::iterator listIter;
    for (listIter = activeJoinBlocks.begin(); listIter != activeJoinBlocks.end(); ++listIter)
    {
        G4_BB* aBB = (*listIter).first;
        if (aBB->getId() == bb->getId())
        {
            // block already in list, update exec size if necessary
            if (execSize > (*listIter).second)
            {
                (*listIter).second = execSize;
            }
            break;
        }
        else if (aBB->getId() > bb->getId())
        {
            activeJoinBlocks.insert(listIter, BlockSizePair(bb, execSize));
            break;
        }
    }

    if (listIter == activeJoinBlocks.end())
    {
        activeJoinBlocks.push_back(BlockSizePair(bb, execSize));
    }
}

void FlowGraph::setPhysicalPredSucc()
{
    BB_LIST_CITER it = BBs.cbegin();
    BB_LIST_CITER cend = BBs.cend();
    if (it != cend)
    {
        // first, set up head BB
        G4_BB* pred = *it;
        pred->setPhysicalPred(NULL);

        for (++it; it != cend; ++it)
        {
            G4_BB* bb = *it;
            bb->setPhysicalPred(pred);
            pred->setPhysicalSucc(bb);
            pred = bb;
        }

        // last, set up the last BB
        pred->setPhysicalSucc(NULL);
    }
}

G4_Label* FlowGraph::insertEndif(G4_BB* bb, G4_ExecSize execSize, bool createLabel)
{
    // endif is placed immediately after the label
    G4_INST* endifInst = builder->createInternalCFInst(NULL, G4_endif, execSize, NULL, NULL, InstOpt_NoOpt);
    INST_LIST_ITER iter = bb->begin();
    MUST_BE_TRUE(iter != bb->end(), "empty BB");
    iter++;
    bb->insertBefore(iter, endifInst);

    // this block may be a target of multiple ifs, in which case we will need to insert
    // one endif for each if.  The innermost endif will use the BB label, while for the other
    // endifs a new label will be created for each of them.
    if (createLabel)
    {
        std::string name = "_auto" + std::to_string(autoLabelId++);
        G4_Label* label = builder->createLabel(name, LABEL_BLOCK);
        endifWithLabels.emplace(endifInst, label);
        return label;
    }
    else
    {
        return bb->getLabel();
    }
}

// This function set the JIP of the endif to the target instruction (either endif or while)
void FlowGraph::setJIPForEndif(G4_INST* endif, G4_INST* target, G4_BB* targetBB)
{
    MUST_BE_TRUE(endif->opcode() == G4_endif, "must be an endif instruction");
    G4_Label* label = getLabelForEndif(target);
    if (label)
    {
        // see if there's another label before the inst that we can reuse
        // FIXME: we really should associate labels with inst instead of having special label inst,
        // so we can avoid ugly code like this
        G4_INST* prevInst = NULL;
        if (target->opcode() == G4_endif)
        {
            for (G4_INST* inst : *targetBB)
            {
                if (inst == target)
                {
                    if (prevInst != NULL && prevInst->isLabel())
                    {
                        label = prevInst->getLabel();
                    }
                    break;
                }
                prevInst = inst;
            }
        }
        else
        {
            MUST_BE_TRUE(target->opcode() == G4_while, "must be a while instruction");
            INST_LIST_RITER it = ++(targetBB->rbegin());
            if (it != targetBB->rend())
            {
                G4_INST* inst = *it;
                if (inst->isLabel())
                {
                    label = inst->getLabel();
                }
            }
        }

        if (label == NULL)
        {
            std::string name = "_auto" + std::to_string(autoLabelId++);
            label = builder->createLabel(name, LABEL_BLOCK);
            endifWithLabels.emplace(target, label);
        }
    }
    endif->asCFInst()->setJip(label);

#ifdef DEBUG_VERBOSE_ON
    cout << "set JIP for: \n";
    endif->emit(cout);
    cout << "\n";
#endif
}

void FlowGraph::convertGotoToJmpi(G4_INST *gotoInst)
{
    gotoInst->setOpcode(G4_jmpi);
    gotoInst->setSrc(gotoInst->asCFInst()->getUip(), 0);
    gotoInst->asCFInst()->setJip(NULL);
    gotoInst->asCFInst()->setUip(NULL);
    gotoInst->setExecSize(g4::SIMD1);
    gotoInst->setOptions(InstOpt_NoOpt | InstOpt_WriteEnable);
}

/*
*  This function generates UCF (unstructurized control flow, that is, goto/join/jmpi)
*    This function inserts the join for each goto as well as compute the JIP for the
*    goto and join instructions. It additionally converts uniform (simd1 or non-predicated)
*    gotos into scalar jumps. If it's a forward goto, it may be converted into a jmpi only
*    if it is uniform and it does not overlap with another divergent goto.  All uniform
*    backward gotos may be converted into scalar jumps.
*
*  - This function does *not* alter the CFG.
*  - This function does *not* consider SCF (structured control flow), as a well-formed
*    vISA program should not have overlapped goto and structured CF instructions.
*
*/
void FlowGraph::processGoto(bool HasSIMDCF)
{
    // For all BBs in [StartBB, EndBB) (including StartBB, but not including EndBB) that
    // jump after EndBB (forward jump), return the earliest (closest to EndBB). If no such
    // BB, return nullptr.
    // Assumption:  1. BB's id is in the increasing order; and 2) physical succ has been set.
    auto getEarliestJmpOutBB = [](
        const std::list<BlockSizePair> &activeJoins,
        const G4_BB* StartBB,
        const G4_BB* EndBB) -> G4_BB*
    {
        // Existing active joins must be considered. For example,
        //    goto L0  (non-uniform)
        //    ...
        //  Loop:
        //     ...
        //     goto L1 (uniform goto)
        //     ...
        //  L0:
        //     ...
        //     goto Loop
        //     ...
        //  L1:
        // Since 'goto L0' is non-uniform, 'goto L1' must be tranlated into goto and a join must
        // be inserted at L1.  If goto L0 is not considered (no active join at L0) and 'goto L1' can
        // be converted into jmpi (thus no join will be inserted at L1, which is wrong.
        std::list<BlockSizePair> tmpActiveJoins(activeJoins);

        const G4_BB* bb = StartBB;
        while (bb != EndBB)
        {
            const G4_BB* currBB = bb;
            if (!tmpActiveJoins.empty())
            {
                // adjust active join lists if a join is reached.
                if (bb == tmpActiveJoins.front().first)
                {
                    tmpActiveJoins.pop_front();
                }
            }

            bb = bb->getPhysicalSucc();
            if (currBB->empty())
            {
                continue;
            }

            const G4_INST* lastInst = currBB->back();
            if (lastInst->opcode() != G4_goto || lastInst->asCFInst()->isBackward())
            {
                continue;
            }
            G4_BB* targetBB = currBB->Succs.back();
            assert(lastInst->asCFInst()->getUip() == targetBB->getLabel());
            if (lastInst->asCFInst()->isUniform() &&
                (tmpActiveJoins.empty() || tmpActiveJoins.front().first->getId() >= targetBB->getId()))
            {
                // Non-crossing uniform goto will be jmpi, and thus no join needed.
                //
                // For the crossing gotos, a uniform goto cannot be translated to jmpi.
                // For example:
                //     goto L0:        // non-uniform goto (need a join)
                //      ....
                //     uniform-goto L1:
                //      ...
                //    L0:
                //      ...
                //    L1:
                //  Here, uniform-goto L1 is uniform. Since it crosses the non-uniform joins at L0,
                //  it must be translated to goto, not jmpi.  Thus, join is needed at L1.
                continue;
            }
            // Here, use SIMD1 as execsize does not matter here.
            addBBToActiveJoinList(tmpActiveJoins, targetBB, g4::SIMD1);
        }

        // Need to remove join at EndBB if present (looking for joins after EndBB)
        if (!tmpActiveJoins.empty() && tmpActiveJoins.front().first == EndBB)
        {
            tmpActiveJoins.pop_front();
        }

        if (tmpActiveJoins.empty())
        {
            // no new join
            return nullptr;
        }
        return tmpActiveJoins.front().first;
    };

    // list of active blocks where a join needs to be inserted, sorted in lexical order
    std::list<BlockSizePair> activeJoinBlocks;
    bool doScalarJmp = !builder->noScalarJmp();

    for (G4_BB* bb : BBs)
    {
        if (bb->size() == 0)
        {
            continue;
        }

        if (activeJoinBlocks.size() > 0)
        {
            if (bb == activeJoinBlocks.front().first)
            {
                // This block is the target of one or more forward goto,
                // or the fall-thru of a backward goto, needs to insert a join
                G4_ExecSize execSize = activeJoinBlocks.front().second;
                G4_Label* joinJIP = NULL;

                activeJoinBlocks.pop_front();
                if (activeJoinBlocks.size() > 0)
                {
                    //set join JIP to the next active join
                    G4_BB* joinBlock = activeJoinBlocks.front().first;
                    joinJIP = joinBlock->getLabel();
                }

                insertJoinToBB(bb, execSize, joinJIP);
            }
        }

        // check to see if this block is the target of one (or more) backward goto
        // If so, we process the backward goto and push its fall-thru block to the
        // active join list
        for (std::list<G4_BB*>::iterator iter = bb->Preds.begin(), iterEnd = bb->Preds.end(); iter != iterEnd; ++iter)
        {
            G4_BB* predBB = *iter;
            G4_INST* lastInst = predBB->back();
            if (lastInst->opcode() == G4_goto && lastInst->asCFInst()->isBackward() &&
                lastInst->asCFInst()->getUip() == bb->getLabel())
            {
                // backward goto
                G4_ExecSize eSize = lastInst->getExecSize() > g4::SIMD1 ?
                    lastInst->getExecSize() : pKernel->getSimdSize();
                bool isUniform = lastInst->getExecSize() == g4::SIMD1 || lastInst->getPredicate() == NULL;
                if (isUniform && doScalarJmp)
                {
                    // can always convert a uniform backward goto into a jmp
                    convertGotoToJmpi(lastInst);

                    // we still have to add a join at the BB immediately after the back edge,
                    // since there may be subsequent loop breaks that are waiting there.
                    // example:
                    // L1:
                    // (P1) goto exit
                    // (P2) goto L2
                    // goto L1
                    // L2:                <-- earliest after-loop goto target
                    // ...
                    // exit:
                    //
                    // In this case, L2 shall be the 1st after-loop join (not necessarily the one immediately
                    // after the backward BB). The goto exit's JIP should be set to L2.
                    //
                    // Here, we will add the 1st after-loop join so that any out-loop JIP (either goto or
                    // join) within the loop body will has its JIP set to this join.
                    if (G4_BB* afterLoopJoinBB = getEarliestJmpOutBB(activeJoinBlocks, bb, predBB))
                    {
                        addBBToActiveJoinList(activeJoinBlocks, afterLoopJoinBB, eSize);
                    }
                }
                else
                {
                    if (lastInst->getExecSize() == g4::SIMD1)
                    {   // For simd1 goto, convert it to a goto with the right execSize.
                        lastInst->setExecSize(eSize);
                        // This should have noMask removed if any
                        lastInst->setOptions(InstOpt_M0);
                    }
                    // add join to the fall-thru BB
                    if (G4_BB* fallThruBB = predBB->getPhysicalSucc())
                    {
                        addBBToActiveJoinList(activeJoinBlocks, fallThruBB, eSize);
                        lastInst->asCFInst()->setJip(fallThruBB->getLabel());
                    }
                }
            }
        }

        G4_INST* lastInst = bb->back();
        if (lastInst->opcode() == G4_goto && !lastInst->asCFInst()->isBackward())
        {
            // forward goto
            // the last Succ BB is our goto target
            G4_BB* gotoTargetBB = bb->Succs.back();
            bool isUniform = lastInst->getExecSize() == g4::SIMD1 || lastInst->getPredicate() == NULL;

            if (isUniform && doScalarJmp &&
                (activeJoinBlocks.size() == 0 || activeJoinBlocks.front().first->getId() > gotoTargetBB->getId()))
            {
                // can convert goto into a scalar jump to UIP, if the jmp will not make us skip any joins
                // CFG itself does not need to be updated
                convertGotoToJmpi(lastInst);
            }
            else
            {
                // set goto JIP to the first active block
                G4_ExecSize eSize = lastInst->getExecSize() > g4::SIMD1 ?
                    lastInst->getExecSize() : pKernel->getSimdSize();
                addBBToActiveJoinList(activeJoinBlocks, gotoTargetBB, eSize);
                G4_BB* joinBlock = activeJoinBlocks.front().first;
                if (lastInst->getExecSize() == g4::SIMD1)
                {   // For simd1 goto, convert it to a goto with the right execSize.
                    lastInst->setExecSize(eSize);
                    lastInst->setOptions(InstOpt_M0);
                }
                lastInst->asCFInst()->setJip(joinBlock->getLabel());

                if (!builder->gotoJumpOnTrue())
                {
                    // For BDW/SKL goto, the false channels are the ones that actually will take the jump,
                    // and we thus have to flip the predicate
                    G4_Predicate* pred = lastInst->getPredicate();
                    if (pred != NULL)
                    {
                        pred->setState(pred->getState() == PredState_Plus ? PredState_Minus : PredState_Plus);
                    }
                    else
                    {
                        // if there is no predicate, generate a predicate with all 0s.
                        // if predicate is SIMD32, we have to use a :ud dst type for the move
                        G4_ExecSize execSize = lastInst->getExecSize() > g4::SIMD16 ? g4::SIMD2 : g4::SIMD1;
                        G4_Declare* tmpFlagDcl = builder->createTempFlag(execSize);
                        G4_DstRegRegion* newPredDef = builder->createDst(
                            tmpFlagDcl->getRegVar(), 0, 0, 1, execSize == g4::SIMD2 ? Type_UD : Type_UW);
                        G4_INST* predInst = builder->createMov(g4::SIMD1, newPredDef, builder->createImm(0, Type_UW),
                            InstOpt_WriteEnable, false);
                        INST_LIST_ITER iter = bb->end();
                        iter--;
                        bb->insertBefore(iter, predInst);

                        pred = builder->createPredicate(
                            PredState_Plus,
                            tmpFlagDcl->getRegVar(),
                            0);
                        lastInst->setPredicate(pred);
                    }
                }
            }
        }
    }
}

// TGL NoMask WA : to identify which BB needs WA
//
// nestedDivergentBBs[BB] = 2;
//      BB is in a nested divergent branch
// nestedDivergentBBs[BB] = 1;
//      BB is not in a nested divergent branch, but in a divergent branch.
void FlowGraph::findNestedDivergentBBs(std::unordered_map<G4_BB*, int>& nestedDivergentBBs)
{
    // Control-Flow state
    //    Used for keeping the current state of control flow during
    //    traversing BBs in the layout order. Assuming the current
    //    BB is 'currBB', this class shows whether currBB is in any
    //    divergent branch/nested divergent branch.
    class CFState
    {
        // If currBB is the head of loop or currBB has a cf inst such as goto/break/if,
        // the code will diverge from currBB to the joinBB.  The following cases shows where
        // joinBB is:
        //     case 1: cf inst = goto
        //          <currBB>    [(p)] goto L
        //                           ...
        //          <joinBB>    L:
        //     case 2: cf inst = if
        //          <currBB>    if
        //                          ...
        //          <joinBB>    endif
        //
        //         Note that else does not increase nor decrease divergence level.
        //     case 3:  cf inst = break
        //          <currBB>   break
        //                     ...
        //                     [(p)] while
        //          <joinBB>
        //     case 4:
        //          <currBB>  L:
        //                     ....
        //                    [(p)] while/goto L
        //          <joinBB>
        // Case 1/2/3 will increase divergence level, while case 4 does not.
        //
        // The following are used for tracking nested divergence
        //    LastDivergentBBId:  Id(LastDivergentBB).
        //        LastDivergentBB is the fartherest joinBB of any goto/break/if/while
        //        so far, as described above.  That is, [currBB, LastDivergentBB) is
        //        a divergent code path. LastDivergentBB is not included in this
        //        divergent path.
        //    LastNestedBBId:  Id(LastNestedBB).
        //        LastNestedBB is the current fartherest join BB that is inside
        //        [currBB, LastDivergentBB). We have LastNestedBB if a goto/break/if (no
        //        loop) is encountered inside an already divergent code path. It is also
        //        called nested divergent (or just nested).
        //
        // The following is always true:
        //    LastDivergentBBId >= LastNestedBBId
        int LastDivergentBBId;
        int LastNestedBBId;

        void setLastDivergentBBId(G4_BB* toBB)
        {
            LastDivergentBBId = std::max(LastDivergentBBId, (int)toBB->getId());
        }

        void setLastNestedBBId(G4_BB* toBB)
        {
            LastNestedBBId = std::max(LastNestedBBId, (int)toBB->getId());
            setLastDivergentBBId(toBB);
        }

        // Increase divergence level
        void incDivergenceLevel(G4_BB* toBB)
        {
            if (isInDivergentBranch())
            {
                setLastNestedBBId(toBB);
            }
            else
            {
                setLastDivergentBBId(toBB);
            }
        }

    public:
        CFState() : LastNestedBBId(-1), LastDivergentBBId(-1) {}

        bool isInDivergentBranch() const { return (LastDivergentBBId > 0);  }
        bool isInNestedDivergentBranch() const { return LastNestedBBId > 0; }

        //  pushLoop: for case 4
        //  pushJoin: for case 1/2/3
        void pushLoop(G4_BB* joinBB) { setLastDivergentBBId(joinBB); }
        void pushJoin(G4_BB* joinBB) { incDivergenceLevel(joinBB); }
        void popJoinIfMatching(G4_BB* currBB)
        {
            if ((int)currBB->getId() == LastNestedBBId)
            {
                LastNestedBBId = -1;
            }
            if ((int)currBB->getId() == LastDivergentBBId)
            {
                LastDivergentBBId = -1;
            }
            assert(!(LastDivergentBBId == -1 && LastNestedBBId >= 0) &&
                   "ICE:  something wrong in setting divergent BB Id!");
        }
    };

    // For loop with backedge Tail->Head, if Tail is divergent, its divergence should be
    // propagated to the entire loop as Tail jumps to head, which could go all BBs in the loop.
    //
    // In another word, the whole loop's divergence level should be at least the same as Tail's.
    // Once the entire loop has been handled and Tail's divergence is known, invoking this lambda
    // function to carry out propagation.
    //
    // An example to show WA is needed (nested divergence for Tail):
    //      Head:                   // initial fuseMask = 11;  2nd iter: fuseMask = 11 (should be 01)
    //                              // Need to propagae nested divergence to the entire loop!
    //         if (...) goto Tail;  // fuseMask = 11
    //                              // After if, fusedMask = 10 (bigEU is Off)
    //         ...
    //         goto out             // fuseMask = 10 (BigEU off, SmallEU on)
    //                              // after goto, fuseMask = 00, but HW remains 10
    //      Tail:
    //         join                 // after join, bigEU is on again.  fusedMask == 11. It should be 01
    //         goto Head            // fuseMask should 01, but HW remains 11, and jump to Head at 2nd iter
    //      out:
    //         join
    auto propLoopDivergence = [&](G4_BB* LoopTail)
    {
        // LoopTail must be divergent.
        std::vector<G4_BB*> workset;
        workset.push_back(LoopTail);
        while (!workset.empty())
        {
            std::vector<G4_BB*> newWorkset;
            for (auto iter : workset)
            {
                G4_BB* Tail = iter;
                G4_InstCF* cfInst = Tail->back()->asCFInst();

                assert(nestedDivergentBBs.count(Tail) > 0 &&
                       "Only divergent Tail shall invoke this func!");

                // Find loop head
                G4_BB* Head = nullptr;
                for (G4_BB* succBB : Tail->Succs)
                {
                    if ((cfInst->opcode() == G4_goto && cfInst->getUip() == succBB->getLabel()) ||
                        (cfInst->opcode() == G4_while && cfInst->getJip() == succBB->getLabel()) ||
                        (cfInst->opcode() == G4_jmpi && cfInst->getSrc(0) == succBB->getLabel()))
                    {
                        Head = succBB;
                        break;
                    }
                }
                assert(Head != nullptr);

                // If Head's divergence level is already higher than Tail, no propagation needed
                // as Head's divergence has been propagated already.
                if (nestedDivergentBBs.count(Head) > 0 &&
                    nestedDivergentBBs[Head] >= nestedDivergentBBs[Tail])
                {
                    continue;
                }

                // Follow physical succs to visit all BBs in this loop
                for (G4_BB* tBB = Head; tBB != nullptr && tBB != Tail; tBB = tBB->getPhysicalSucc())
                {
                    auto miter = nestedDivergentBBs.find(tBB);
                    if (miter == nestedDivergentBBs.end() || miter->second < nestedDivergentBBs[Tail])
                    {
                        // Propagate Tail's divergence. If the new BB is a tail (another loop),
                        // add it to workset for the further propagation.
                        nestedDivergentBBs[tBB] = nestedDivergentBBs[Tail];
                        auto lastOp = tBB->getLastOpcode();
                        if (lastOp == G4_while ||
                            ((lastOp == G4_goto || lastOp == G4_jmpi) &&
                             tBB->back()->asCFInst()->isBackward()))
                        {
                            newWorkset.push_back(tBB);
                        }
                    }
                }
            }

            workset = newWorkset;
        }
    };

    if (BBs.empty())
    {
        // Sanity check
        return;
    }

    // If -noMaskWAOnStackCall is prsent, all BBs inside stack functions are
    // assumed to need NoMaskWA.
    if (builder->getOption(vISA_noMaskWAOnFuncEntry) && builder->getIsFunction())
    {
        for (auto bb : BBs)
        {
            nestedDivergentBBs[bb] = 2;
            bb->setBBType(G4_BB_NM_WA_TYPE);
        }
        return;
    }

    // Analyze subroutines in topological order. As there is no recursion
    // and no indirect call,  a subroutine will be analyzed only if all
    // its callers have been analyzed.
    //
    // If no subroutine, sortedFuncTable is empty. Here keep the entry and
    // subroutines in a vector first (it works with and without subroutines),
    // then scan subroutines in topological order.
    struct StartEndIter {
        BB_LIST_ITER StartI;
        BB_LIST_ITER EndI;

        // When a subroutine is entered, the fusedMask must not be 00, but it
        // could be 01. This field shows it could be 01 if set to true.
        bool  isInDivergentBranch;
    };
    int numFuncs = (int)sortedFuncTable.size();
    std::vector<StartEndIter> allFuncs;
    std::unordered_map<FuncInfo*, uint32_t> funcInfoIndex;
    if (numFuncs == 0)
    {
        numFuncs = 1;
        allFuncs.resize(1);
        allFuncs[0].StartI = BBs.begin();
        allFuncs[0].EndI = BBs.end();
        allFuncs[0].isInDivergentBranch = false;
    }
    else
    {
        allFuncs.resize(numFuncs);
        for (int i = numFuncs; i > 0; --i)
        {
            uint32_t ix = (uint32_t)(i - 1);
            FuncInfo* pFInfo = sortedFuncTable[ix];
            G4_BB* StartBB = pFInfo->getInitBB();
            G4_BB* EndBB = pFInfo->getExitBB();
            uint32_t ui = (uint32_t)(numFuncs - i);
            allFuncs[ui].StartI = std::find(BBs.begin(), BBs.end(), StartBB);
            auto nextI = std::find(BBs.begin(), BBs.end(), EndBB);
            assert(nextI != BBs.end() && "ICE: subroutine's end BB not found!");
            allFuncs[ui].EndI = (++nextI);
            allFuncs[ui].isInDivergentBranch = false;

            funcInfoIndex[pFInfo] = ui;
        }
    }

    for (int i = 0; i < numFuncs; ++i)
    {
        // each function: [IT, IE)
        BB_LIST_ITER& IB = allFuncs[i].StartI;
        BB_LIST_ITER& IE = allFuncs[i].EndI;
        if (IB == IE)
        {
            // Sanity check
            continue;
        }

        CFState cfs;
        if (allFuncs[i].isInDivergentBranch)
        {
            // subroutine's divergent on entry. Mark [StartBB, EndBB) as divergent
            auto prevI = IE;
            --prevI;
            G4_BB* EndBB = *prevI;
            cfs.pushJoin(EndBB);
        }

        // FusedMask does not correct itself. Once the fusedMask goes wrong, it stays
        // wrong until the control-flow reaches non-divergent BB. Thus, some BBs, which
        // are not nested-divergent, may need NoMask WA as they might inherit the wrong
        // fusedMask from the previous nested-divergent BBs.
        //
        // Here, we make adjustment for this reason.
        bool seenNestedDivergent = false;
        for (BB_LIST_ITER IT = IB; IT != IE; ++IT)
        {
            G4_BB* BB = *IT;

            // This handles cases in which BB has endif/while/join as well as others
            // so we don't need to explicitly check whether BB has endif/while/join, etc.
            cfs.popJoinIfMatching(BB);

            G4_INST* firstInst = BB->getFirstInst();
            if (firstInst == nullptr)
            {
                // empty BB or BB with only label inst
                continue;
            }

            // Handle loop
            for (G4_BB* predBB : BB->Preds)
            {
                G4_INST* lastInst = predBB->back();
                if ((lastInst->opcode() == G4_while &&
                      lastInst->asCFInst()->getJip() == BB->getLabel()) ||
                     (lastInst->opcode() == G4_goto && lastInst->asCFInst()->isBackward() &&
                      lastInst->asCFInst()->getUip() == BB->getLabel()))
                {
                    // joinBB is the BB right after goto/while
                    BB_LIST_ITER predIter = std::find(BBs.begin(), BBs.end(), predBB);
                    ++predIter;
                    // if predBB is the last BB then joinBB is not available
                    if (predIter == BBs.end())
                        continue;
                    G4_BB* joinBB = *predIter;
                    cfs.pushLoop(joinBB);
                }
            }

            if (cfs.isInNestedDivergentBranch())
            {
                nestedDivergentBBs[BB] = 2;
                seenNestedDivergent = true;
            }
            else if (cfs.isInDivergentBranch())
            {
                if (seenNestedDivergent)
                {
                    // divergent and might inherit wrong fusedMask
                    // set it to nested divergent so we can apply WA.
                    nestedDivergentBBs[BB] = 2;
                }
                else
                {
                    nestedDivergentBBs[BB] = 1;
                }
            }
            else
            {
                // Reach non-divergent BB
                seenNestedDivergent = false;
            }

            G4_INST* lastInst = BB->back();

            // Need to check whether to propagate WA marking to entire loop!
            if (nestedDivergentBBs.count(BB) > 0 && nestedDivergentBBs[BB] >= 2)
            {
                if (lastInst->opcode() == G4_while ||
                    ((lastInst->opcode() == G4_goto || lastInst->opcode() == G4_jmpi) &&
                     lastInst->asCFInst()->isBackward()))
                {
                    propLoopDivergence(BB);
                }
            }

            if ((lastInst->opcode() == G4_goto && !lastInst->asCFInst()->isBackward()) ||
                lastInst->opcode() == G4_break)
            {
                // forward goto/break : the last Succ BB is our target BB
                // For break, it should be the BB right after while inst.
                G4_BB* joinBB = BB->Succs.back();
                cfs.pushJoin(joinBB);
            }
            else if (lastInst->opcode() == G4_if)
            {
                G4_Label* labelInst = lastInst->asCFInst()->getUip();
                G4_BB* joinBB = findLabelBB(IT, IE, labelInst->getLabel());
                assert(joinBB && "ICE(vISA) : missing endif label!");
                cfs.pushJoin(joinBB);
            }
            else if (lastInst->opcode() == G4_call)
            {
                // If this function is already in divergent branch, the callee
                // must be in a divergent branch!.
                if (allFuncs[i].isInDivergentBranch ||
                    cfs.isInDivergentBranch() ||
                    lastInst->getPredicate() != nullptr)
                {
                    FuncInfo* calleeFunc = BB->getCalleeInfo();
                    if (funcInfoIndex.count(calleeFunc))
                    {
                        int ix = funcInfoIndex[calleeFunc];
                        allFuncs[ix].isInDivergentBranch = true;
                    }
                }
            }
        }

        // Once all BBs are precessed, cfs should be clear
        assert((!cfs.isInDivergentBranch()) &&
               "ICE(vISA): there is an error in divergence tracking!");
    }

    for (auto MI : nestedDivergentBBs)
    {
        G4_BB* bb = MI.first;
        if (MI.second >= 2)
        {
            bb->setBBType(G4_BB_NM_WA_TYPE);
        }
    }
    return;
}

//
// Add declares for the stack and frame pointers.
//
void FlowGraph::addFrameSetupDeclares(IR_Builder& builder, PhyRegPool& regPool)
{
    if (framePtrDcl == NULL)
    {
        framePtrDcl = builder.getBEFP();
    }
    if (stackPtrDcl == NULL)
    {
        stackPtrDcl = builder.getBESP();
    }
    if (scratchRegDcl == NULL)
    {
        scratchRegDcl = builder.createDeclareNoLookup("SR", G4_GRF, builder.numEltPerGRF<Type_UD>(), 1, Type_UD);
        scratchRegDcl->getRegVar()->setPhyReg(regPool.getGreg(builder.kernel.getSpillHeaderGRF()), 0);
    }
}

//
// Insert pseudo dcls to represent the caller-save and callee-save registers.
// This is only required when there is more than one graph cut due to the presence
// of function calls using a stack calling convention.
//
void FlowGraph::addSaveRestorePseudoDeclares(IR_Builder& builder)
{
    //
    // VCE_SAVE (r60.0-r125.0) [r125-127 is reserved]
    //
    if (pseudoVCEDcl == NULL)
    {
        unsigned numRowsVCE = getKernel()->getNumCalleeSaveRegs();
        pseudoVCEDcl = builder.createDeclareNoLookup("VCE_SAVE", G4_GRF, builder.numEltPerGRF<Type_UD>(), static_cast<unsigned short>(numRowsVCE), Type_UD);
    }
    else
    {
        pseudoVCEDcl->getRegVar()->setPhyReg(NULL, 0);
    }

    //
    // Insert caller save decls for VCA/A0/Flag (one per call site)
    // VCA_SAVE (r1.0-r60.0) [r0 is reserved] - one required per stack call,
    // but will be reused across cuts.
    //
    INST_LIST callSites;
    for (auto bb : builder.kernel.fg)
    {
        if (bb->isEndWithFCall())
        {
            callSites.push_back(bb->back());
        }
    }

    unsigned i = 0;
    for (auto callSite : callSites)
    {
        const char* nameBase = "VCA_SAVE";  // sizeof(nameBase) = 9, including ending 0
        const int maxIdLen = 3;
        const char* name = builder.getNameString(mem, sizeof(nameBase) + maxIdLen, "%s_%d", nameBase, i);
        G4_Declare* VCA = builder.createDeclareNoLookup(name, G4_GRF, builder.numEltPerGRF<Type_UD>(), builder.kernel.getCallerSaveLastGRF(), Type_UD);
        name = builder.getNameString(mem, 50, "SA0_%d", i);
        G4_Declare* saveA0 = builder.createDeclareNoLookup(name, G4_ADDRESS, (uint16_t)getNumAddrRegisters(), 1, Type_UW);
        name = builder.getNameString(mem, 64, "SFLAG_%d", i);
        G4_Declare* saveFLAG = builder.createDeclareNoLookup(name, G4_FLAG, (uint16_t) builder.getNumFlagRegisters(), 1, Type_UW);
        fcallToPseudoDclMap[callSite->asCFInst()] = { VCA, saveA0, saveFLAG };
        i++;
    }
}

//
// Since we don't do SIMD augmentation in RA for CM, we have to add an edge
// between the then and else block of an if branch to ensure that liveness is
// computed correctly, if conservatively. This also applies to any goto BB and
// its JIP join block
//
void FlowGraph::addSIMDEdges()
{
    std::map<G4_Label*, G4_BB*> joinBBMap;
    for (auto bb : BBs)
    {
        if (bb->size() > 0 && bb->back()->opcode() == G4_else)
        {
            addUniquePredSuccEdges(bb, bb->getPhysicalSucc());
        }
        else
        {
            // check goto case
            auto instIter = std::find_if(bb->begin(), bb->end(),
                [](G4_INST* inst) { return !inst->isLabel(); });
            if (instIter != bb->end() && (*instIter)->opcode() == G4_join)
            {
                G4_INST* firstInst = bb->front();
                if (firstInst->isLabel())
                {
                    joinBBMap[firstInst->getLabel()] = bb;
                }
            }
        }
    }

    if (!joinBBMap.empty())
    {
        for (auto bb : BBs)
        {
            if (bb->isEndWithGoto())
            {
                G4_INST* gotoInst = bb->back();
                auto iter = joinBBMap.find(gotoInst->asCFInst()->getJip()->asLabel());
                if (iter != joinBBMap.end())
                {
                    addUniquePredSuccEdges(bb, iter->second);
                }
            }
        }
    }
}

//
// Perform DFS traversal on the flow graph (do not enter subroutine, but mark subroutine blocks
// so that they will be processed independently later)
//
void FlowGraph::DFSTraverse(G4_BB* startBB, unsigned &preId, unsigned &postId, FuncInfo* fn)
{
    MUST_BE_TRUE(fn, "Invalid func info");
    std::stack<G4_BB*> traversalStack;
    traversalStack.push(startBB);

    while (!traversalStack.empty())
    {
        G4_BB*  bb = traversalStack.top();
        if (bb->getPreId() != UINT_MAX)
        {
            // Pre-processed already and continue to the next one.
            // Before doing so, set postId if not set before.
            traversalStack.pop();
            if (bb->getRPostId() == UINT_MAX)
            {
                // All bb's succ has been visited (PreId is set) at this time.
                // if any of its succ has not been finished (RPostId not set),
                // bb->succ forms a backedge.
                //
                // Note: originally, CALL and EXIT will not check back-edges, here
                //       we skip checking for them as well. (INIT & RETURN should
                //       be checked as well ?)
                if (!(bb->getBBType() & (G4_BB_CALL_TYPE | G4_BB_EXIT_TYPE)))
                {
                    for (auto succBB : bb->Succs)
                    {
                        if (succBB->getRPostId() == UINT_MAX)
                        {
                            backEdges.push_back(Edge(bb, succBB));
                        }
                    }
                }

                // Need to keep this after backedge checking so that self-backedge
                // (single-bb loop) will not be missed.
                bb->setRPostId(postId++);
            }
            continue;
        }

        fn->addBB(bb);
        bb->setPreId(preId++);

        if (bb->getBBType() & G4_BB_CALL_TYPE)
        {
            G4_BB* returnBB = bb->BBAfterCall();
            // If call is predicated, first item in Succs is physically consecutive BB and second (or last)
            // item is sub-routine entry BB.
            MUST_BE_TRUE(bb->Succs.back()->getBBType() & G4_BB_INIT_TYPE, ERROR_FLOWGRAPH);
            // bb->Succs size may be 2 if call is predicated.
            MUST_BE_TRUE(bb->Succs.size() == 1 || bb->Succs.size() == 2, ERROR_FLOWGRAPH);

            {
                bool found = false;
                for (auto func : fn->getCallees())
                {
                    if (func == bb->getCalleeInfo())
                        found = true;
                }
                if (!found)
                {
                    fn->addCallee(bb->getCalleeInfo());
                }
            }

            if (returnBB->getPreId() == UINT_MAX)
            {
                traversalStack.push(returnBB);
            }
            else
            {
                MUST_BE_TRUE(false, ERROR_FLOWGRAPH);
            }
        }
        else if (bb->getBBType() & G4_BB_EXIT_TYPE)
        {
            // Skip
        }
        else
        {
            // To be consistent with previous behavior, use reverse_iter.
            BB_LIST_RITER RIE = bb->Succs.rend();
            for (BB_LIST_RITER rit = bb->Succs.rbegin(); rit != RIE; ++rit)
            {
                G4_BB* succBB = *rit;
                if (succBB->getPreId() == UINT_MAX)
                {
                    traversalStack.push(succBB);
                }
            }
        }
        // As the top of stack may be different than that at the
        // beginning of this iteration, cannot do pop here. Instead,
        // do pop and set RPostId at the beginning of each iteration.
        //
        // traversalStack.pop();
        // bb->setRPostId(postId++);
    }
}

void vISA::FlowGraph::markStale()
{
    // invoked whenever CFG structures changes.
    // structural changes include addition/removal of BB or edge.
    // mark analysis passes as stale so getters lazily re-run
    // analysis when queried.
    immDom.setStale();
    pDom.setStale();
    loops.setStale();

    // any other analysis that becomes stale when FlowGraph changes
    // should be marked as stale here.
}

void FlowGraph::markRPOTraversal()
{
    MUST_BE_TRUE(numBBId == BBs.size(), ERROR_FLOWGRAPH);

    unsigned postID = 0;
    backEdges.clear();

    for (auto curBB : BBs)
    {
        curBB->setRPostId(postID++);

        if (curBB->size() > 0)
        {
            if (curBB->getBBType() & G4_BB_CALL_TYPE)
            {
                // skip
            }
            else if (curBB->getBBType() & G4_BB_EXIT_TYPE)
            {
                // Skip
            }
            else
            {
                for (auto succBB : curBB->Succs)
                {
                    if (curBB->getId() >= succBB->getId())
                    {
                        backEdges.push_back(Edge(curBB, succBB));
                    }
                }
            }
        }
    }
}

//
// Find back-edges in the flow graph.
//
void FlowGraph::findBackEdges()
{
    MUST_BE_TRUE(numBBId == BBs.size(), ERROR_FLOWGRAPH);

    for (auto bb : BBs)
    {
        bb->setPreId(UINT_MAX);
        bb->setRPostId(UINT_MAX);
    }

    unsigned preId = 0;
    unsigned postID = 0;
    backEdges.clear();

    setPhysicalPredSucc();
    DFSTraverse(getEntryBB(), preId, postID, kernelInfo);

    for (auto fn : funcInfoTable)
    {
        DFSTraverse(fn->getInitBB(), preId, postID, fn);
    }
}

//
// Find natural loops in the flow graph.
// Assumption: the input FG is reducible.
//
void FlowGraph::findNaturalLoops()
{
    setPhysicalPredSucc();
    for (auto&& backEdge : backEdges)
    {
        G4_BB* head = backEdge.second;
        G4_BB* tail = backEdge.first;
        std::list<G4_BB*> loopBlocks;
        Blocks loopBody;
        loopBlocks.push_back(tail);
        loopBody.insert(tail);

        while (!loopBlocks.empty())
        {
            G4_BB* loopBlock = loopBlocks.front();
            loopBlocks.pop_front();
            loopBlock->setInNaturalLoop(true);
            loopBlock->setNestLevel();

            if ((loopBlock == head) || (loopBlock->getBBType() & G4_BB_INIT_TYPE))
            {
                // Skip
            }
            else if (loopBlock->getBBType() & G4_BB_RETURN_TYPE)
            {
                auto callBB = loopBlock->BBBeforeCall();
                if (!callBB->isInNaturalLoop())
                {
                    loopBlocks.push_front(callBB);
                    loopBody.insert(callBB);
                }
            }
            else
            {
                auto entryBB = getEntryBB();
                for (auto predBB : loopBlock->Preds)
                {
                    if (!predBB->isInNaturalLoop())
                    {
                        if (predBB == entryBB && head != entryBB)
                        {
                            // graph is irreducible, punt natural loop detection for entire CFG
                            this->reducible = false;
                            naturalLoops.clear();
                            for (auto BB : BBs)
                            {
                                BB->setInNaturalLoop(false);
                                BB->resetNestLevel();
                            }
                            return;
                        }
                        MUST_BE_TRUE(predBB != entryBB || head == entryBB, ERROR_FLOWGRAPH);
                        loopBlocks.push_front(predBB);
                        loopBody.insert(predBB);
                    }
                }
            }
        }

        for (auto loopBB : loopBody)
        {
            loopBB->setInNaturalLoop(false);
        }

        naturalLoops.emplace(backEdge, loopBody);
    }
}

void FlowGraph::traverseFunc(FuncInfo* func, unsigned *ptr)
{
    func->setPreID((*ptr)++);
    func->setVisited();
    for (auto callee : func->getCallees())
    {
        if (!(callee->getVisited()))
        {
            traverseFunc(callee, ptr);
        }
    }
    sortedFuncTable.push_back(func);
    func->setPostID((*ptr)++);
}

//
// Sort subroutines in topological order based on DFS
// a topological order is guaranteed as recursion is not allowed for subroutine calls
// results are stored in sortedFuncTable in reverse topological order
//
void FlowGraph::topologicalSortCallGraph()
{
    unsigned visitID = 1;
    traverseFunc(kernelInfo, &visitID);
}

//
// This should be called only after pre-/post-visit ID are set
//
static bool checkVisitID(FuncInfo* func1, FuncInfo* func2)
{
    if (func1->getPreID() < func2->getPreID() &&
        func1->getPostID() > func2->getPostID())
    {
        return true;
    }
    else
    {
        return false;
    }
}

//
// Find dominators for each function
//
void FlowGraph::findDominators(std::map<FuncInfo*, std::set<FuncInfo*>>& domMap)
{
    std::map<FuncInfo*, std::set<FuncInfo*>> predMap;

    for (auto func : sortedFuncTable)
    {
        if (func == kernelInfo)
        {
            std::set<FuncInfo*> initSet;
            initSet.insert(kernelInfo);
            domMap.insert(std::make_pair(kernelInfo, initSet));
        }
        else
        {
            std::set<FuncInfo*> initSet;
            for (auto funcTmp : sortedFuncTable)
            {
                initSet.insert(funcTmp);
            }
            domMap.insert(std::make_pair(func, initSet));
        }

        for (auto callee : func->getCallees())
        {
            std::map<FuncInfo*, std::set<FuncInfo*>>::iterator predMapIter = predMap.find(callee);
            if (predMapIter == predMap.end())
            {
                std::set<FuncInfo*> initSet;
                initSet.insert(func);
                predMap.insert(std::make_pair(callee, initSet));
            }
            else
            {
                predMapIter->second.insert(func);
            }
        }
    }

    bool changed = false;
    do
    {
        changed = false;

        unsigned funcTableSize = static_cast<unsigned> (sortedFuncTable.size());
        unsigned funcID = funcTableSize - 1;
        do
        {
            funcID--;
            FuncInfo* func = sortedFuncTable[funcID];

            std::map<FuncInfo*, std::set<FuncInfo*>>::iterator predMapIter = predMap.find(func);
            if (predMapIter != predMap.end())
            {
                std::set<FuncInfo*>& domSet = (*domMap.find(func)).second;
                std::set<FuncInfo*> oldDomSet = domSet;
                domSet.clear();
                domSet.insert(func);

                std::vector<unsigned> funcVec(funcTableSize);
                for (unsigned i = 0; i < funcTableSize; i++)
                {
                    funcVec[i] = 0;
                }

                std::set<FuncInfo*>& predSet = (*predMapIter).second;
                for (auto pred : predSet)
                {
                    for (auto predDom : (*domMap.find(pred)).second)
                    {
                        unsigned domID = (predDom->getScopeID() == UINT_MAX) ? funcTableSize - 1 : predDom->getScopeID() - 1;
                        funcVec[domID]++;
                    }
                }

                unsigned predSetSize = static_cast<unsigned> (predSet.size());
                for (unsigned i = 0; i < funcTableSize; i++)
                {
                    if (funcVec[i] == predSetSize)
                    {
                        FuncInfo* newFunc = sortedFuncTable[i];
                        domSet.insert(newFunc);
                        if (oldDomSet.find(newFunc) == oldDomSet.end())
                            changed = true;
                    }
                }

                if (oldDomSet.size() != domSet.size())
                {
                    changed = true;
                }
            }
        } while (funcID != 0);

    } while (changed);
}

//
// Check if func1 is a dominator of func2
//
static bool checkDominator(FuncInfo* func1, FuncInfo* func2, std::map<FuncInfo*, std::set<FuncInfo*>>& domMap)
{
    std::map<FuncInfo*, std::set<FuncInfo*>>::iterator domMapIter = domMap.find(func2);

    if (domMapIter != domMap.end())
    {
        std::set<FuncInfo*> domSet = (*domMapIter).second;
        std::set<FuncInfo*>::iterator domSetIter = domSet.find(func1);

        if (domSetIter != domSet.end())
        {
            return true;
        }
    }

    return false;
}

//
// Determine the scope of a varaible based on different contexts
//
unsigned FlowGraph::resolveVarScope(G4_Declare* dcl, FuncInfo* func)
{
    unsigned oldID = dcl->getScopeID();
    unsigned newID = func->getScopeID();

    if (oldID == newID)
    {
        return oldID;
    }
    else if (oldID == 0)
    {
        return newID;
    }
    else if (oldID == UINT_MAX ||
        newID == UINT_MAX)
    {
        return UINT_MAX;
    }
    else if (builder->getOption(vISA_EnableGlobalScopeAnalysis))
    {
        // This is safe if the global variable usage is
        // self-contained under the calling function
        std::map<FuncInfo*, std::set<FuncInfo*>> domMap;

        findDominators(domMap);

        FuncInfo* oldFunc = sortedFuncTable[oldID - 1];

        if (checkVisitID(func, oldFunc) &&
            checkDominator(func, oldFunc, domMap))
        {
            return newID;
        }
        else if (checkVisitID(oldFunc, func) &&
            checkDominator(oldFunc, func, domMap))
        {
            return oldID;
        }
        else
        {
            unsigned start = (newID > oldID) ? newID : oldID;
            unsigned end = static_cast<unsigned> (sortedFuncTable.size());

            for (unsigned funcID = start; funcID != end; funcID++)
            {
                FuncInfo* currFunc = sortedFuncTable[funcID];
                if (checkVisitID(currFunc, func) &&
                    checkDominator(currFunc, func, domMap) &&
                    checkVisitID(currFunc, oldFunc) &&
                    checkDominator(currFunc, oldFunc, domMap))
                {
                    return currFunc->getScopeID();
                }
            }
        }
    }

    return UINT_MAX;
}

//
// Visit all operands referenced in a function and update the varaible scope
//
void FlowGraph::markVarScope(std::vector<G4_BB*>& BBList, FuncInfo* func)
{
    for (auto bb : BBList)
    {
        for (auto it = bb->begin(), end = bb->end(); it != end; it++)
        {
            G4_INST* inst = (*it);

            G4_DstRegRegion* dst = inst->getDst();

            if (dst &&
                !dst->isAreg() &&
                dst->getBase())
            {
                G4_Declare* dcl = GetTopDclFromRegRegion(dst);
                unsigned scopeID = resolveVarScope(dcl, func);
                dcl->updateScopeID(scopeID);
            }

            for (int i = 0; i < G4_MAX_SRCS; i++)
            {
                G4_Operand* src = inst->getSrc(i);

                if (src && !src->isAreg())
                {
                    if (src->isSrcRegRegion() &&
                        src->asSrcRegRegion()->getBase())
                    {
                        G4_Declare* dcl = GetTopDclFromRegRegion(src);
                        unsigned scopeID = resolveVarScope(dcl, func);
                        dcl->updateScopeID(scopeID);
                    }
                    else if (src->isAddrExp() &&
                        src->asAddrExp()->getRegVar())
                    {
                        G4_Declare* dcl = src->asAddrExp()->getRegVar()->getDeclare()->getRootDeclare();
                        unsigned scopeID = resolveVarScope(dcl, func);
                        dcl->updateScopeID(scopeID);
                    }
                }
            }
        }
    }
}

//
// Traverse the call graph and mark varaible scope
//
void FlowGraph::markScope()
{
    if (funcInfoTable.size() == 0)
    {
        // no subroutines
        return;
    }
    unsigned id = 1;
    std::vector<FuncInfo *>::iterator kernelIter = sortedFuncTable.end();
    kernelIter--;
    for (std::vector<FuncInfo *>::iterator funcIter = sortedFuncTable.begin();
        funcIter != sortedFuncTable.end();
        ++funcIter)
    {
        if (funcIter == kernelIter)
        {
            id = UINT_MAX;
        }

        FuncInfo* func = (*funcIter);
        func->setScopeID(id);

        for (auto bb : func->getBBList())
        {
            bb->setScopeID(id);
        }

        id++;
    }

    for (auto func : sortedFuncTable)
    {
        markVarScope(func->getBBList(), func);
    }
}

// Return the mask for anyH or allH predicate control in the goto to be emitted.
static uint32_t getFlagMask(G4_Predicate_Control pCtrl)
{
    switch (pCtrl)
    {
    case G4_Predicate_Control::PRED_ALL2H:
        return 0xFFFFFFFC;
    case G4_Predicate_Control::PRED_ALL4H:
        return 0xFFFFFFF0;
    case G4_Predicate_Control::PRED_ALL8H:
        return 0xFFFFFF00;
    case G4_Predicate_Control::PRED_ALL16H:
        return 0xFFFF0000;
    case G4_Predicate_Control::PRED_ALL32H:
        return 0x00000000;
    case G4_Predicate_Control::PRED_ANY2H:
        return 0x00000003;
    case G4_Predicate_Control::PRED_ANY4H:
        return 0x0000000F;
    case G4_Predicate_Control::PRED_ANY8H:
        return 0x000000FF;
    case G4_Predicate_Control::PRED_ANY16H:
        return 0x0000FFFF;
    case G4_Predicate_Control::PRED_ANY32H:
        return 0xFFFFFFFF;
    default:
        MUST_BE_TRUE(false, "only for AllH or AnyH predicate control");
        break;
    }
    return 0;
}

// Given a predicate ctrl for jmpi, return the adjusted predicate ctrl in a new
// simd size.
static G4_Predicate_Control getPredCtrl(unsigned simdSize,
    G4_Predicate_Control pCtrl, bool predCtrlHasWidth)
{
    if (!predCtrlHasWidth)
    {
        return G4_Predicate::isAllH(pCtrl) ? PRED_ALL_WHOLE : PRED_ANY_WHOLE;
    }

    if (G4_Predicate::isAllH(pCtrl))
        return (simdSize == 8)
        ? PRED_ALL8H
        : (simdSize == 16) ? PRED_ALL16H
        : G4_Predicate_Control::PRED_ALL32H;

    // Any or default
    return (simdSize == 8)
        ? PRED_ANY8H
        : (simdSize == 16) ? PRED_ANY16H
        : G4_Predicate_Control::PRED_ANY32H;
}

// If BB has only one predecessor, return it;
// if BB has two predecessors and one is ExcludedPred, return the other predecessor;
// otherwise, return nullptr.
G4_BB* FlowGraph::getSinglePredecessor(G4_BB* BB, G4_BB* ExcludedPred) const
{
    if (BB->Preds.size() != 1) {
        if (BB->Preds.size() == 2) {
            if (BB->Preds.front() == ExcludedPred)
                return BB->Preds.back();
            if (BB->Preds.back() == ExcludedPred)
                return BB->Preds.front();
        }
        return nullptr;
    }
    return BB->Preds.front();
}

// Convert jmpi to goto. E.g.
//
// Case1:
// .decl P1 v_type=P num_elts=2
//
// cmp.ne (M1, 2) P1 V33(0,0)<2;2,1> 0x0:f
// (!P1.any) jmpi (M1, 1) BB1
//
// ===>
//
// cmp.ne (M1, 2) P1 V33(0,0)<2;2,1> 0x0:f
// and (1) P1 P1 0b00000011
// (!P2.any) goto (M1, 8) BB1
//
// Case2:
// .decl P1 v_type=P num_elts=2
//
//  cmp.ne (M1, 2) P1 V33(0,0)<2;2,1> 0x0:f
// (!P1.all) jmpi (M1, 1) BB1
//
// ===>
//
// cmp.ne (M1, 2) P1 V33(0,0)<2;2,1> 0x0:f
// or (1) P1 P1 0b11111100
// (!P1.all) goto (M1, 8) BB1
//
bool FlowGraph::convertJmpiToGoto()
{
    bool Changed = false;
    for (auto bb : BBs)
    {
        for (auto I = bb->begin(), IEnd = bb->end(); I != IEnd; ++I)
        {
            G4_INST *inst = *I;
            if (inst->opcode() != G4_jmpi)
                continue;

            unsigned predSize = pKernel->getSimdSize();
            G4_Predicate *newPred = nullptr;

            if (G4_Predicate *pred = inst->getPredicate())
            {
                // The number of bool elements in vISA decl.
                unsigned nElts = pred->getTopDcl()->getNumberFlagElements();

                // Since we need to turn this into goto, set high bits properly.
                if (nElts != predSize)
                {
                    // The underlying dcl type is either uw or ud.
                    G4_Type SrcTy = pred->getTopDcl()->getElemType();
                    G4_Type DstTy = (predSize > 16) ? Type_UD : Type_UW;

                    G4_Predicate_Control pCtrl = pred->getControl();
                    MUST_BE_TRUE(nElts == 1 ||
                        G4_Predicate::isAnyH(pCtrl) ||
                        G4_Predicate::isAllH(pCtrl),
                        "predicate control not handled yet");

                    // Common dst and src0 operand for flag.
                    G4_Declare *newDcl = builder->createTempFlag(predSize > 16 ? 2 : 1);
                    auto pDst = builder->createDst(newDcl->getRegVar(), 0, 0, 1, DstTy);
                    auto pSrc0 = builder->createSrc(
                        pred->getBase(), 0, 0, builder->getRegionScalar(), SrcTy);

                    auto truncMask = [](uint32_t mask, G4_Type Ty) -> uint64_t
                    {
                        return (Ty == Type_UW) ? uint16_t(mask) : mask;
                    };

                    if (pCtrl == G4_Predicate_Control::PRED_DEFAULT)
                    {
                        // P = P & 1
                        auto pSrc1 = builder->createImm(1, Type_UW);
                        auto pInst = builder->createBinOp(
                            G4_and, g4::SIMD1, pDst, pSrc0, pSrc1,
                            InstOpt_M0 | InstOpt_WriteEnable, false);
                        bb->insertBefore(I, pInst);
                    }
                    else if (G4_Predicate::isAnyH(pCtrl))
                    {
                        // P = P & mask
                        uint32_t mask = getFlagMask(pCtrl);
                        auto pSrc1 = builder->createImm(truncMask(mask, DstTy), DstTy);
                        auto pInst = builder->createBinOp(
                            G4_and, g4::SIMD1, pDst, pSrc0, pSrc1,
                            InstOpt_M0 | InstOpt_WriteEnable, false);
                        bb->insertBefore(I, pInst);
                    }
                    else
                    {
                        // AllH
                        // P = P | mask
                        uint32_t mask = getFlagMask(pCtrl);
                        auto pSrc1 = builder->createImm(truncMask(mask, DstTy), DstTy);
                        auto pInst = builder->createBinOp(
                            G4_or, g4::SIMD1, pDst, pSrc0, pSrc1,
                            InstOpt_M0 | InstOpt_WriteEnable, false);
                        bb->insertBefore(I, pInst);
                    }

                    // Adjust pred control to the new execution size and build the
                    // new predicate.
                    pCtrl = getPredCtrl(predSize, pCtrl, builder->predCtrlHasWidth());
                    newPred = builder->createPredicate(
                        pred->getState(), newDcl->getRegVar(), 0, pCtrl);
                }
            }

            // (!P) jmpi L
            // becomes:
            // P = P & MASK
            // (!P.anyN) goto (N) L
            inst->setOpcode(G4_goto);
            inst->setExecSize(G4_ExecSize(predSize));
            if (newPred)
                inst->setPredicate(newPred);
            inst->asCFInst()->setUip(inst->getSrc(0)->asLabel());
            inst->setSrc(nullptr, 0);
            inst->setOptions(InstOpt_M0);
            Changed = true;
        }
    }
    return Changed;
}

void FlowGraph::print(std::ostream& OS) const
{
    const char* kname = nullptr;
    if (getKernel()) {
        kname = getKernel()->getName();
    }
    kname = kname ? kname : "unnamed";
    OS  << "\n\nCFG: "
        << kname
        << (builder->getIsKernel() ? " [kernel]" : " [non-kernel function]")
        << "\n\n";
    for (auto BB : BBs) {
        BB->print(OS);
    }
}

void FlowGraph::dump() const
{
    print(std::cerr);
}

FlowGraph::~FlowGraph()
{
    // even though G4_BBs are allocated in a mem pool and freed in one shot,
    // we must call each BB's desstructor explicitly to free up the memory used
    // by the STL objects(list, vector, etc.) in each BB
    for (G4_BB* bb : BBAllocList)
    {
        bb->~G4_BB();
    }
    BBAllocList.clear();
    globalOpndHT.clearHashTable();
    for (auto funcInfo : funcInfoTable)
    {
        funcInfo->~FuncInfo();
    }
    if (kernelInfo)
    {
        kernelInfo->~FuncInfo();
    }
    for (auto summary : localRASummaries)
    {
        summary->~PhyRegSummary();
    }
}

RelocationEntry& RelocationEntry::createRelocation(
    G4_Kernel& kernel, G4_INST& inst,
    int opndPos, const std::string& symbolName,
    RelocationType type)
{
    kernel.getRelocationTable().emplace_back(
        RelocationEntry(&inst, opndPos, type, symbolName));
    return kernel.getRelocationTable().back();
}

void RelocationEntry::doRelocation(
    const G4_Kernel& kernel, void* binary, uint32_t binarySize)
{
    // FIXME: nothing to do here
    // we have only dynamic relocations now, which cannot be resolved at compilation time
}

uint32_t RelocationEntry::getTargetOffset(const IR_Builder& builder) const
{
    // instruction being relocated must not be compacted, or the offset need to be re-adjusted
    // FIXME: This only check if vISA force to compact the instruction, it cannot make sure
    // the Binary encoder won't compact it
    assert(inst->isCompactedInst() == false);

    G4_Operand* target_operand = inst->getSrc(opndPos);
    assert(target_operand->isRelocImm());

    switch (inst->opcode()) {
    case G4_mov:
        // When src0 type is 64 bits:
        //  On Pre-Xe:
        //   Src0.imm[31:0] mapped to Instruction [95:64]
        //   Src0.imm[63:32] mapped to Instruction [127:96]
        //  On Xe+:
        //   Src0.imm[31:0] mapped to Instruction [127:96]
        //   Src0.imm[63:32] mapped to Instruction [95:64]
        // When src0 type is 32 bits:
        //   Src0.imm[31:0] mapped to instruction [127:96]
        assert((target_operand->getType() == Type_UD) || (target_operand->getType() == Type_D) ||
            (target_operand->getType() == Type_UQ) || (target_operand->getType() == Type_Q));
        assert(opndPos == 0);
        return (target_operand->getType() == Type_UD || target_operand->getType() == Type_D) ? 12 : 8;

    case G4_add:
        // add instruction cannot have 64-bit imm
        assert(relocType != R_SYM_ADDR && relocType != R_SYM_ADDR_32_HI);
        assert(opndPos == 1);
        // Src1.imm[31:0] mapped to Instruction [127:96]
        return 12;
    default:
        break;
    }

    // currently we only support relocation on mov or add instruction
    assert(false && "Unreachable");
    return 0;
}

const char * RelocationEntry::getTypeString() const
{
    switch (relocType)
    {
    case RelocationType::R_SYM_ADDR:
        return "R_SYM_ADDR";
    case RelocationType::R_SYM_ADDR_32:
        return "R_SYM_ADDR_32";
    case RelocationType::R_SYM_ADDR_32_HI:
        return "R_SYM_ADDR_32_HI";
    case RelocationType::R_PER_THREAD_PAYLOAD_OFFSET_32:
        return "R_PER_THREAD_PAYLOAD_OFFSET_32";
    case RelocationType::R_GLOBAL_IMM_32:
        return "R_GLOBAL_IMM_32";
    default:
        assert(false && "unhandled relocation type");
        return "";
    }
}

void RelocationEntry::dump(std::ostream &os) const
{
    os << "Relocation entry: " << getTypeString() << "\n";
    os << "\t";
    inst->dump();
    switch (relocType)
    {
        case RelocationType::R_NONE:
            os << "R_NONE: symbol name = " << symName;
            break;
        case RelocationType::R_SYM_ADDR:
            os << "R_SYM_ADDR: symbol name = " << symName;
            break;
        case RelocationType::R_SYM_ADDR_32:
            os << "R_SYM_ADDR_32: symbol name = " << symName;
            break;
        case RelocationType::R_SYM_ADDR_32_HI:
            os << "R_SYM_ADDR_32_HI: symbol name = " << symName;
            break;
        case RelocationType::R_PER_THREAD_PAYLOAD_OFFSET_32:
            os << "R_PER_THREAD_PAYLOAD_OFFSET_32: symbol name = " << symName;
            break;
        case RelocationType::R_GLOBAL_IMM_32:
            os << "R_GLOBAL_IMM_32: symbol name = " << symName;
            break;
    }
    os << "\n";
}

void FuncInfo::dump() const
{
    std::cerr << "subroutine " << getId() << "(" << getInitBB()->front()->getLabelStr() << ")\n";
    std::cerr << "\tentryBB=" << getInitBB()->getId() << ", exitBB=" << getExitBB()->getId() << "\n";
    std::cerr << "\tCallees: ";
    for (auto callee : callees)
    {
        std::cerr << callee->getId() << " ";
    }
    std::cerr << "\n\tBB list: ";
    for (auto bb : BBList)
    {
        std::cerr << bb->getId() << " ";
    }
    std::cerr << "\n";
}
