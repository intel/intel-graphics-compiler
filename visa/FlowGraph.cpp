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

#include "FlowGraph.h"
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <set>
#include <iostream>
#include <sstream>
#include <fstream>
#include <functional>
#include <algorithm>
#include "BuildIR.h"
#include "Option.h"
#include "stdlib.h"
#include "queue"
#include "BitSet.h"
#include "PhyRegUsage.h"
#include "visa_wa.h"
#include "CFGStructurizer.h"
#include "DebugInfo.h"
#include <random>
#include <chrono>

#include "BinaryEncodingIGA.h"
#include "iga/IGALibrary/api/iga.h"
#include "iga/IGALibrary/api/iga.hpp"
#include "iga/IGALibrary/api/igaEncoderWrapper.hpp"

#if defined(WIN32)
#define CDECLATTRIBUTE __cdecl
#elif __GNUC__
#ifdef __x86_64__
#define CDECLATTRIBUTE
#else
#define CDECLATTRIBUTE                 __attribute__((__cdecl__))
#endif
#endif


using namespace std;
using namespace vISA;

static _THREAD const char* prevFilename;
static _THREAD int prevSrcLineNo;

bool G4_BB::isSuccBB(G4_BB* succ)
{
    for (std::list<G4_BB*>::iterator it = Succs.begin(), bbEnd = Succs.end(); it != bbEnd; ++it)
    {
        if ((*it) == succ)  return true;
    }
    return false;
}

void G4_BB::removePredEdge(G4_BB* pred)
{
    for (std::list<G4_BB*>::iterator it = Preds.begin(), bbEnd = Preds.end(); it != bbEnd; ++it)
    {
        if (*it != pred) continue;
        // found
        Preds.erase(it);
        return;
    }
    MUST_BE_TRUE(false, ERROR_FLOWGRAPH); // edge is not found
}

void G4_BB::removeSuccEdge(G4_BB* succ)
{
    for (std::list<G4_BB*>::iterator it = Succs.begin(), bbEnd = Succs.end(); it != bbEnd; ++it)
    {
        if (*it != succ) continue;
        // found
        Succs.erase(it);
        return;
    }
    MUST_BE_TRUE(false, ERROR_FLOWGRAPH); // edge is not found
}

//
// find the fall-through BB of the current block.
// if the last inst is a unconditional jump, then the target is not considered a fall-through BB
// NOTE: Pay attention this function is only works after the handleReturn() duo the the conditional CALL
//
G4_BB* G4_BB::fallThroughBB()
{
    G4_INST* last = (!instList.empty()) ? instList.back() : NULL;

    if (last != NULL)
    {
        if (last->opcode() == G4_goto || last->opcode() == G4_join)
        {
            return NULL;
        }
        if (last->isFlowControl())
        {
            // if No successor, return NULL;
            if (Succs.empty())
            {
                return NULL;
            }

            //
            // Instructions    Predicate-On    Predicate-Off    Num of Succ
            // Jmpi                Front                None               >=1
            // CALL             Front                None               >=2     considered the conditional call here
            // while               Front                Front              2 // G4_while considered to fall trhu even without pred, since break jumps to while
            // if, else        Front                Front               2
            // break, cont      Front                None               1,2
            // return              Front                None               >=1
            // do                   Front                Front                1
            // endif                 Front                Front                1
            if (last->isCall())
            {
                return BBAfterCall();
            }
            else if (last->getPredicate() == NULL &&
                // G4_while considered to fall trhu even without pred, since break jumps to while
                (last->opcode() == G4_jmpi || last->opcode() == G4_break || last->opcode() == G4_cont || last->isReturn()))
            {
                return NULL;
            }
            else
            {
                return Succs.front();
            }
        }
    }

    //
    // process other cases
    //
    if (Succs.size() == 0) // exit BB
        return NULL; // no fall-through BB
    else
        return Succs.front();
}

//
// to check if the last instruction in list is EOT
//
bool G4_BB::isLastInstEOT()
{
    if (instList.size() == 0)
    {
        return false;
    }

    G4_INST *i = instList.back();

    if (parent->builder->hasSendShootdown())
    {
        // due to send shootdown, a predicated send may not actually be an EOT
        return i->isEOT() && i->getPredicate() == NULL;
    }
    else
    {
        return i->isEOT();
    }
}

G4_opcode G4_BB::getLastOpcode() const
{
    G4_INST *i = instList.empty() ? nullptr : instList.back();
    if (i)
    {
        return i->opcode();
    }
    else
    {
        return G4_illegal;
    }
}

//
// return label's corresponding BB
// if label's BB is not yet created, then create one and add map label to BB
//
G4_BB* FlowGraph::getLabelBB(Label_BB_Map& map, char const* label)
{
    MUST_BE_TRUE(label != NULL, ERROR_INTERNAL_ARGUMENT);
    std::string label_string = label;
    if (map.find(label_string) != map.end())
        return map[label_string];
    else    // BB does not exist
    {
        G4_BB* bb = createNewBB();
        map[label_string] = bb;             // associate them
        return bb;
    }
}

//
// first is the first inst of a BB
//
G4_BB* FlowGraph::beginBB(Label_BB_Map& map, G4_INST* first)
{
    if (first == NULL) return NULL;
    G4_BB* bb = (first->isLabel()) ? getLabelBB(map, first->getLabelStr()) : createNewBB();
    BBs.push_back(bb); // append to BBs list
    return bb;
}

G4_INST* FlowGraph::createNewLabelInst(G4_Label* label, int lineNo, int CISAOff)
{
    //srcFileName is NULL
    return builder->createInternalInst(NULL, G4_label,
        NULL, false, 1, NULL, label, NULL, 0, lineNo, CISAOff, NULL);
}

G4_BB* FlowGraph::createNewBB(bool insertInFG)
{
    G4_BB* bb = new (mem)G4_BB(instListAlloc, numBBId, this);

    // Increment counter only when new BB is inserted in FlowGraph
    if (insertInFG)
        numBBId++;

    if (builder->getOptions()->getTarget() == VISA_3D)
    {
        // all 3D bbs are considered to be in SIMD CF since the dispatch mask may
        // have some channels off.
        bb->setInSimdFlow(true);
    }

    BBAllocList.push_back(bb);
    return bb;
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
            G4_INST* movInst = builder->createInternalInst(nullptr, G4_mov, nullptr, false, 1,
                nullDst, randImm, nullptr, 0);

            auto instItEnd = bb->end();
            for (auto it = bb->begin();
                it != instItEnd;
                it++)
            {
                if ((*it)->isLabel())
                {
                    bb->insert(++it, movInst);
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
        std::string createdLabel = (builder->getIsKernel() ? "k" : "f") +
            std::to_string(builder->getCUnitId()) + "_AUTO_GENERATED_IF_LABEL_" +
            std::to_string(sn);
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
                std::string createdLabel = (builder->getIsKernel() ? "k" : "f") +
                    std::to_string(builder->getCUnitId()) + "__AUTO_GENERATED_ELSE_LABEL__" +
                    std::to_string(sn);
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
                // For GT, if/else/endif are different from Gen4:
                // (1). if - endif
                //          if endif_label
                //          ...
                //          endif_label:
                //          endif
                // (2). if - else - endif
                //          if else_label
                //          ...
                //          else endif_label
                //          else_label:
                //          ...
                //          endif_label:     // this is different from Gen4
                //          endif
                //

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
    for (auto it1 = instlist.begin(), itEnd = instlist.end(); it1 != itEnd; ++it1)
    {
        G4_INST* i = *it1;
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
    for (auto I = instlist.begin(), E = instlist.end(); I != E; ++I)
    {
        G4_INST* i = *I;
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

void FlowGraph::NormalizeFlowGraph()
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
                std::string name = (builder->getIsKernel() ? "L_AUTO_k" : "L_AUTO_f") +
                    std::to_string(builder->getCUnitId()) + "_" + std::to_string(newNode->getId());
                G4_Label* lbl = builder->createLabel(name, LABEL_BLOCK);
                G4_INST* inst = createNewLabelInst(lbl, lInst->getLineNo(), lInst->getCISAOff());
                newNode->push_back(inst);
                BBs.insert(++it, newNode);
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
    std::map<std::string, G4_BB*> labelMap;
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
                        addPredSuccEdges(curr_BB, getLabelBB(labelMap, i->getLabelStr()));
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
                        for (std::list<G4_Label*>::const_iterator it = jmpTargets.begin(), jmpTrgEnd = jmpTargets.end(); it != jmpTrgEnd; ++it)
                        {
                            G4_INST* jmpInst = builder->createInst(NULL, G4_jmpi, NULL, false, 1, NULL, *it, NULL, 0);
                            indirectJmpTarget.emplace(jmpInst);
                            INST_LIST_ITER jmpInstIter = builder->instList.end();
                            curr_BB->splice(curr_BB->end(), builder->instList, --jmpInstIter);
                            addPredSuccEdges(curr_BB, getLabelBB(labelMap, (*it)->getLabel()));
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
                    }
                    else if (i->getPredicate())
                    {
                        // add fall through edge
                        addUniquePredSuccEdges(curr_BB, next_BB);
                    }
                }    // if (i->opcode()
                else if (i->opcode() == G4_if || i->opcode() == G4_while ||
                    i->opcode() == G4_else)
                {
                    hasSIMDCF = true;
                    if (i->asCFInst()->getJip()->isLabel())
                    {
                        if (i->opcode() == G4_else || i->opcode() == G4_while)
                        {
                            // for G4_while, jump no matter predicate
                            addPredSuccEdges(curr_BB, getLabelBB(labelMap, i->asCFInst()->getJipLabelStr()));
                        }
                        else if ((i->getPredicate() != NULL) ||
                            ((i->getCondMod() != NULL) &&
                            (i->getSrc(0) != NULL) &&
                                (i->getSrc(1) != NULL)))
                        {
                            addPredSuccEdges(curr_BB, getLabelBB(labelMap, i->asCFInst()->getJipLabelStr()));
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
                    addPredSuccEdges(curr_BB, getLabelBB(labelMap, ((G4_Label *)i->asCFInst()->getJip())->getLabel()));

                    if (strcmp(i->asCFInst()->getJipLabelStr(), i->asCFInst()->getUipLabelStr()) != 0)
                        addPredSuccEdges(curr_BB, getLabelBB(labelMap, ((G4_Label *)i->asCFInst()->getUip())->getLabel()));

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
                    addPredSuccEdges(curr_BB, getLabelBB(labelMap, i->asCFInst()->getUipLabelStr()));

                    if (i->getPredicate())
                    {
                        // fall through, but check if goto target is same as fall-thru
                        // FIXME: replace all addPredSuccEdges with addUniquePredSuccEdges?
                        addUniquePredSuccEdges(curr_BB, next_BB);
                    }
                }
                else
                {
                    MUST_BE_TRUE1(false, i->getLineNo(),
                        ERROR_INVALID_G4INST); // not yet handled
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

    if (builder->getOption(vISA_DumpDotAll))
    {
        pKernel->dumpDotFile("beforeRemoveRedundantLabels");
    }

    removeRedundantLabels();

    if (builder->getOption(vISA_DumpDotAll))
    {
        pKernel->dumpDotFile("afterRemoveRedundantLabels");
    }

    // Ensure each block other than entry starts with a label.
    for (auto bb : BBs)
    {
        if (bb != getEntryBB() && !bb->empty())
        {
            G4_INST *inst = bb->front();
            if (inst->isLabel())
                continue;

            std::string name = "_AUTO_LABEL_" + std::to_string(autoLabelId++);
            G4_Label *label = builder->createLabel(name, LABEL_BLOCK);
            G4_INST *labelInst = builder->createInst(
                nullptr, G4_label, nullptr, false, UNDEFINED_EXEC_SIZE, nullptr,
                label, nullptr, 0);
            bb->push_front(labelInst);
        }
    }

    handleExit(subroutineStartBB.size() > 1 ? subroutineStartBB[1] : nullptr);

    handleReturn(labelMap, funcInfoHashTable);
    mergeReturn(labelMap, funcInfoHashTable);
    normalizeSubRoutineBB(funcInfoHashTable);

    handleWait();

    if (isStackCallFunc)
    {
        mergeFReturns();
    }

    if (builder->getOption(vISA_DumpDotAll))
    {
        pKernel->dumpDotFile("beforeRemoveUnreachableBlocks");
    }
    removeUnreachableBlocks();

    if (builder->getOption(vISA_DumpDotAll))
    {
        pKernel->dumpDotFile("afterRemoveUnreachableBlocks");
    }

    //
    // build the table of function info nodes
    //
    funcInfoTable.resize(funcInfoHashTable.size());

    for (FuncInfoHashTable::iterator it = funcInfoHashTable.begin(), end = funcInfoHashTable.end(); it != end; ++it) {
        FuncInfo* funcInfo = (*it).second;
        funcInfo->getInitBB()->setFuncInfo(funcInfo);
        funcInfo->getExitBB()->setFuncInfo(funcInfo);
        funcInfoTable[funcInfo->getId()] = funcInfo;
    }

    setPhysicalPredSucc();
    if (hasGoto)
    {
        // Structurizer requires that the last BB has no goto (ie, the
        // last BB is either a return or an exit).
        if (builder->getOption(vISA_EnableStructurizer) &&
            !endWithGotoInLastBB())
        {
            if (builder->getOption(vISA_DumpDotAll))
            {
                pKernel->dumpDotFile("before_doCFGStructurizer");
            }

            doCFGStructurize(this);

            if (builder->getOption(vISA_DumpDotAll))
            {
                pKernel->dumpDotFile("after_doCFGStructurizer");
            }
        }
        else
        {
            if (builder->getOption(vISA_DumpDotAll))
            {
                pKernel->dumpDotFile("beforeProcessGoto");
            }
            processGoto(hasSIMDCF);
            if (builder->getOption(vISA_DumpDotAll))
            {
                pKernel->dumpDotFile("afterProcessGoto");
            }
        }
    }

    // This finds back edges and populates blocks into function info objects.
    // The latter will be used to mark SIMD blocks. Prior to RA, no transformation
    // shall invalidate back edges (G4_BB -> G4_BB).
    reassignBlockIDs();
    findBackEdges();

    if (hasSIMDCF && builder->getOptions()->getTarget() == VISA_CM)
    {
        markSimdBlocks(labelMap, funcInfoHashTable);
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

    builder->materializeGlobalImm(getEntryBB());
    normalizeRegionDescriptors();
    localDataFlowAnalysis();
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
                RegionDesc *desc = srcRegion->getRegion();
                auto normDesc = builder->getNormalizedRegion(execSize, desc);
                if (normDesc && normDesc != desc)
                {
                    srcRegion->setRegion(normDesc, /*invariant*/ true);
                }
            }
        }
    }
}

// materialize the values in global Imm at entry BB
void IR_Builder::materializeGlobalImm(G4_BB* entryBB)
{
    for (int i = 0, numImm = immPool.size(); i < numImm; ++i)
    {
        auto&& immVal = immPool.getImmVal(i);
        auto dcl = immPool.getImmDcl(i);
        G4_INST* inst = createInternalInst(nullptr, G4_mov, nullptr, false, immVal.numElt,
            Create_Dst_Opnd_From_Dcl(dcl, 1), immVal.imm, nullptr, InstOpt_WriteEnable);
        auto iter = std::find_if(entryBB->begin(), entryBB->end(),
            [](G4_INST* inst) { return !inst->isLabel(); });
        entryBB->insert(iter, inst);
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
                        nextInst->setOpcode(nextInst->isSplitSend() ? G4_sendsc : G4_sendc);
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
                    bool commitEnable = builder->needsFenceCommitEnable();
                    G4_INST* fenceInst = builder->createFenceInstruction(0, commitEnable, false, true);
                    bb->insert(iter, fenceInst);
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

// Add a sampler cache flush with null return before the EOT send
// bb must end with an EOT send
void G4_BB::addSamplerFlushBeforeEOT()
{
    assert(this->isLastInstEOT() && "last instruction must be EOT");
    auto builder = parent->builder;
    int samplerFlushOpcode = 0x1F;
    int samplerFlushFC = (SamplerSIMDMode::SIMD32 << 17) +
        (samplerFlushOpcode << 12);
    int desc = G4_SendMsgDescriptor::createDesc(samplerFlushFC, true,
        1, 0);
    G4_SrcRegRegion* sendMsgOpnd = builder->Create_Src_Opnd_From_Dcl(
        builder->getBuiltinR0(),
        builder->getRegionStride1());
    auto msgDesc = builder->createSendMsgDesc(desc, SFIDtoInt(SFID::SAMPLER), true, true);
    G4_INST* samplerFlushInst = builder->createSendInst(nullptr, G4_send,
        8, builder->createNullDst(Type_UD), sendMsgOpnd,
        builder->createImm(desc, Type_UD),
        0, msgDesc, 0);
    auto iter = std::prev(this->end());
    this->insert(iter, samplerFlushInst);
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
            if (lastInst->getExecSize() == 1 &&
                !(builder->getFCPatchInfo() && builder->getFCPatchInfo()->getFCComposableKernel() == true))
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
                            !(secondToLastInst->getMsgDesc()->extMessageLength() > 2 &&
                                VISA_WA_CHECK(builder->getPWaTable(), WaSendsSrc1SizeLimitWhenEOT)))
                        {
                            secondToLastInst->getMsgDesc()->setEOT();
                            if (secondToLastInst->isSplitSend())
                            {
                                if (secondToLastInst->getSrc(3)->isImm())
                                {
                                    secondToLastInst->setSrc(builder->createImm(secondToLastInst->getMsgDesc()->getExtendedDesc(), Type_UD), 3);
                                }
                            }
                            needsEOTSend = false;
                            if (builder->getHasNullReturnSampler())
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
            BBs.push_back(exitBB);
        }
        else
        {
            BBs.insert(iter, exitBB);
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

            if (retInst->getExecSize() == 1)
            {
                G4_INST* jmpInst = builder->createInternalInst(retInst->getPredicate(), G4_jmpi,
                    NULL, false, 1, NULL, exitLabel, NULL, InstOpt_NoOpt);
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
    for (BB_LIST_ITER iter = BBs.begin(), iterEnd = BBs.end(); iter != iterEnd; ++iter)
    {
        G4_BB* bb = *iter;
        if (bb->size() == 0)
        {
            continue;
        }
        G4_INST* lastInst = bb->back();
        if (lastInst->opcode() == G4_pseudo_exit)
        {
            MUST_BE_TRUE(false, "All pseudo exits should be removed at this point");
        }
    }

#endif
}

//
// This phase iterates each BB and checks if the last inst of a BB is a "call foo".  If yes,
// the algorith traverses from the block of foo to search for RETURN and link the block of
// RETURN with the block of the return address
//

void FlowGraph::handleReturn(std::map<std::string, G4_BB*>& labelMap, FuncInfoHashTable& funcInfoHashTable)
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

                // find the  subroutine BB and return Addr BB
                std::string subName = last->getLabelStr();
                G4_BB* subBB = labelMap[subName];
                //
                // the fall through BB must be the front
                //
                G4_BB* retAddr = bb->Succs.front();
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
                    unsigned funcId = (unsigned int)funcInfoHashTable.size();
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

                // set up BB after CALL link
                bb->setBBAfterCall(retAddr);
                retAddr->setBBBeforeCall(bb);
                retAddr->setBBType(G4_BB_RETURN_TYPE);
            }
        }
    }
    //
    // remove <CALL, return addr> link when it is not a conditional call
    //
    for (std::list<G4_BB*>::iterator it = BBs.begin(), itEnd = BBs.end(); it != itEnd; ++it)
    {
        G4_BB* bb = (*it);

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
void FlowGraph::mergeReturn(Label_BB_Map& map, FuncInfoHashTable& funcInfoHashTable)
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
            (*calleeInfoLoc).second->getExitBB()->unsetBBType(G4_BB_EXIT_TYPE);
            mergedExitBB->setBBType(G4_BB_EXIT_TYPE);
            (*calleeInfoLoc).second->updateExitBB(mergedExitBB);
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
        BBs.insert(BBs.end(), newBB);
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
        for (auto it = retBBList.begin(); it != retBBList.end(); ++it)
        {
            G4_BB * retBB = (*it);
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
    BBs.insert(BBs.end(), newInitBB);

    FuncInfoHashTable::iterator old_iter = funcInfoHashTable.find(oldInitBB->getId());
    MUST_BE_TRUE(old_iter != funcInfoHashTable.end(), " Function info is not in hashtable.");
    G4_BB* exitBB = (*old_iter).second->getExitBB();
    unsigned funcId = (*old_iter).second->getId();

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

            // update info in func table
            FuncInfoHashTable::iterator calleeInfoLoc = funcInfoHashTable.find(newInitBB->getId());

            if (calleeInfoLoc != funcInfoHashTable.end()) {
                (*calleeInfoLoc).second->incrementCallCount();
                (*kt)->setCalleeInfo((*calleeInfoLoc).second);
            }
            else {
                FuncInfo *funcInfo = new (mem)FuncInfo(
                    funcId, newInitBB, exitBB);
                std::pair<FuncInfoHashTable::iterator, bool> loc =
                    funcInfoHashTable.insert(
                        std::make_pair(newInitBB->getId(), funcInfo));
                MUST_BE_TRUE(loc.second, ERROR_FLOWGRAPH);
                (*kt)->setCalleeInfo((*(loc.first)).second);
            }

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

    // Erase item from unordered_map using
    // key rather than iterator since iterator may be
    // invalid due to insert operation since last find.
    {
        FuncInfoHashTable::iterator calleeInfoLoc = funcInfoHashTable.find(oldInitBB->getId());
        if (calleeInfoLoc != funcInfoHashTable.end()) {
            (*calleeInfoLoc).second->~FuncInfo();
        }
        funcInfoHashTable.erase(oldInitBB->getId());
    }

    oldInitBB->unsetBBType(G4_BB_INIT_TYPE);
    newInitBB->setBBType(G4_BB_INIT_TYPE);
    addPredSuccEdges(newInitBB, oldInitBB);

    std::string blockName = "LABEL__EMPTYBB__" + std::to_string(newInitBB->getId());
    G4_Label* label = builder->createLabel(blockName, LABEL_BLOCK);
    G4_INST* labelInst = createNewLabelInst(label);
    newInitBB->push_back(labelInst);
}


void FlowGraph::decoupleExitBlock(G4_BB* bb)
{
    G4_BB* oldExitBB = bb;
    G4_BB* newExitBB = createNewBB();
    BBs.insert(BBs.end(), newExitBB);

    BB_LIST_ITER kt = oldExitBB->Succs.begin();

    while (kt != oldExitBB->Succs.end())
    {
        // the succs of this new EXIT BB are all call ret
        if ((*kt)->getBBType() & G4_BB_RETURN_TYPE) {

            newExitBB->Succs.push_back((*kt));

            BB_LIST_ITER jt = (*kt)->Preds.begin();
            while (jt != (*kt)->Preds.end()) {
                if ((*jt) == oldExitBB)
                {
                    break;
                }
                jt++;
            }
            MUST_BE_TRUE(jt != (*kt)->Preds.end(), ERROR_FLOWGRAPH);
            (*kt)->Preds.insert(jt, newExitBB);
            (*kt)->Preds.erase(jt);

            (*kt)->BBBeforeCall()->getCalleeInfo()->updateExitBB(newExitBB);

            BB_LIST_ITER tmp_kt = kt;
            ++kt;
            // erase this succ from old EXIT BB's succs
            oldExitBB->Succs.erase(tmp_kt);
        }
        else {
            ++kt;
        }
    }

    oldExitBB->unsetBBType(G4_BB_EXIT_TYPE);
    newExitBB->setBBType(G4_BB_EXIT_TYPE);
    addPredSuccEdges(oldExitBB, newExitBB);

    std::string str = "LABEL__EMPTYBB__" + std::to_string(newExitBB->getId());
    G4_Label* label = builder->createLabel(str, LABEL_BLOCK);
    G4_INST* labelInst = createNewLabelInst(label);
    newExitBB->push_back(labelInst);
}

void FlowGraph::decoupleReturnBlock(G4_BB* bb)
{
    G4_BB* oldRetBB = bb;
    G4_BB* newRetBB = createNewBB();
    BBs.insert(BBs.end(), newRetBB);
    G4_BB* itsExitBB = oldRetBB->BBBeforeCall()->getCalleeInfo()->getExitBB();

    BB_LIST_ITER jt = itsExitBB->Succs.begin();
    BB_LIST_ITER jtEnd = itsExitBB->Succs.end();

    for (; jt != jtEnd; ++jt)
    {
        if ((*jt) == oldRetBB)
        {
            break;
        }
    }

    MUST_BE_TRUE(jt != itsExitBB->Succs.end(), ERROR_FLOWGRAPH);

    itsExitBB->Succs.insert(jt, newRetBB);
    itsExitBB->Succs.erase(jt);
    newRetBB->Preds.push_back(itsExitBB);
    newRetBB->Succs.push_back(oldRetBB);

    BB_LIST_ITER kt = oldRetBB->Preds.begin();
    BB_LIST_ITER ktEnd = oldRetBB->Preds.end();

    for (; kt != ktEnd; ++kt)
    {
        if ((*kt) == itsExitBB)
        {
            break;
        }
    }

    MUST_BE_TRUE(kt != oldRetBB->Preds.end(), ERROR_FLOWGRAPH);

    oldRetBB->Preds.insert(kt, newRetBB);
    oldRetBB->Preds.erase(kt);
    oldRetBB->Preds.unique();


    oldRetBB->unsetBBType(G4_BB_RETURN_TYPE);
    newRetBB->setBBType(G4_BB_RETURN_TYPE);

    newRetBB->setBBBeforeCall(oldRetBB->BBBeforeCall());
    oldRetBB->BBBeforeCall()->setBBAfterCall(newRetBB);

    bb->setBBBeforeCall(NULL);

    std::string str = "LABEL__EMPTYBB__" + std::to_string(newRetBB->getId());
    G4_Label* label = builder->createLabel(str, LABEL_BLOCK);
    G4_INST* labelInst = createNewLabelInst(label);
    newRetBB->push_back(labelInst);
}

void FlowGraph::normalizeSubRoutineBB(FuncInfoHashTable& funcInfoTable)
{
    for (BB_LIST_ITER it = BBs.begin(); it != BBs.end(); ++it)
    {
        if (((*it)->getBBType() & G4_BB_CALL_TYPE))
        {
            G4_BB* callBB = (*it);

            if (callBB->getBBType() & G4_BB_INIT_TYPE)
            {
                decoupleInitBlock(callBB, funcInfoTable);
            }
            if (callBB->getBBType() & G4_BB_EXIT_TYPE)
            {
                decoupleExitBlock(callBB);
            }

            if (callBB->getBBType() & G4_BB_RETURN_TYPE)
            {
                decoupleReturnBlock(callBB);
            }
        }
        else if (((*it)->getBBType() & G4_BB_INIT_TYPE))
        {
            G4_BB* initBB = (*it);
            if (initBB->getBBType() != G4_BB_INIT_TYPE)
            {
                decoupleInitBlock(initBB, funcInfoTable);
            }
        }
        else if (((*it)->getBBType() & G4_BB_EXIT_TYPE))
        {
            G4_BB* exitBB = (*it);

            if (exitBB->getBBType() & G4_BB_INIT_TYPE)
            {
                decoupleInitBlock(exitBB, funcInfoTable);
            }
            if (exitBB->getBBType() & G4_BB_CALL_TYPE)
            {
                decoupleExitBlock(exitBB);
            }

            if (exitBB->getBBType() & G4_BB_RETURN_TYPE)
            {
                decoupleReturnBlock(exitBB);
            }
        }
        else if (((*it)->getBBType() & G4_BB_RETURN_TYPE))
        {
            G4_BB* retBB = (*it);

            if (retBB->getBBType() & G4_BB_INIT_TYPE)
            {
                MUST_BE_TRUE(false, ERROR_FLOWGRAPH);
            }
            if (retBB->getBBType() & G4_BB_EXIT_TYPE)
            {
                MUST_BE_TRUE(!(retBB->getBBType() & G4_BB_CALL_TYPE), ERROR_FLOWGRAPH);
                decoupleReturnBlock(retBB);
            }
            else if (retBB->getBBType() & G4_BB_CALL_TYPE)
            {
                decoupleReturnBlock(retBB);
            }
            else if (retBB->Preds.size() > 1)
            {
                decoupleReturnBlock(retBB);
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

        BB_LIST_ITER IE = currBB->Succs.end();
        for (BB_LIST_ITER it = currBB->Succs.begin(); it != IE; ++it)
        {
            G4_BB* tmp = *it;
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
void FlowGraph::removeUnreachableBlocks()
{
    unsigned preId = 0;
    std::vector<bool> canRemove(BBs.size(), false);

    //
    // initializations
    //
    for (std::list<G4_BB*>::iterator it = BBs.begin(), itEnd = BBs.end(); it != itEnd; ++it)
    {
        (*it)->setPreId(UINT_MAX);
    }
    //
    // assign DFS based pre/rpost ids to all blocks in the main program
    //
    doDFS(getEntryBB(), preId);

    for (BB_LIST_ITER it = BBs.begin(), itEnd = BBs.end(); it != itEnd; ++it)
    {
        if ((*it)->getPreId() == UINT_MAX)
        {
            // Entire function is unreachable. So it should be ok
            // to delete the return as well.
            canRemove[(*it)->getId()] = true;
        }

    }

    //
    // Basic blocks with preId/rpostId set to UINT_MAX are unreachable
    //
    BB_LIST_ITER it = BBs.begin();
    while (it != BBs.end())
    {
        G4_BB* bb = (*it);

        if (bb->getPreId() == UINT_MAX)
        {
            //leaving dangling BBs with return/EOT in for now.
            //workaround to handle unreachable return
            //for example return after infinite loop.
            if (((bb->isEndWithFRet() || (bb->size() > 0 && (G4_INST*)bb->back()->isReturn()))) ||
                (bb->size() > 0 && bb->back()->isEOT()))
            {
                it++;
                continue;
            }

            while (bb->Succs.size() > 0)
            {
                removePredSuccEdges(bb, bb->Succs.front());
            }

            BB_LIST_ITER prev = it;
            prev++;
            BBs.erase(it);
            it = prev;
        }
        else
        {
            it++;
        }
    }

    reassignBlockIDs();
}

void FlowGraph::AssignDFSBasedIds(G4_BB* bb, unsigned &preId, unsigned &postId, std::list<G4_BB*>& rpoBBList)
{
    bb->setPreId(preId++);
    //
    // perform a context-sensitive (actually just call-sensitive) traversal.
    // if this is CALL block then we need to resume DFS at the corresponding RETURN block
    //
    if (bb->getBBType() & G4_BB_CALL_TYPE)
    {
        G4_BB* returnBB = bb->BBAfterCall();
        MUST_BE_TRUE(returnBB->getPreId() == UINT_MAX, ERROR_FLOWGRAPH);
        MUST_BE_TRUE(bb->Succs.front()->getBBType() & G4_BB_INIT_TYPE, ERROR_FLOWGRAPH);
        MUST_BE_TRUE(bb->Succs.size() == 1, ERROR_FLOWGRAPH);
        AssignDFSBasedIds(returnBB, preId, postId, rpoBBList);
    }
    //
    // if this is the EXIT block of subroutine then just return. the CALL block
    // will ensure traversal of the RETURN block.
    //
    else if (bb->getBBType() & G4_BB_EXIT_TYPE)
    {
        // do nothing
    }
    else {
        std::list<G4_BB*> ordered_succs;
        G4_INST* last = (bb->empty()) ? NULL : bb->back();
        //
        // visit "else" branches before "then" branches so that "then" block get a
        // lower rpo number than "else" blocks.
        //
        if (last && last->getPredicate() &&
            (last->opcode() == G4_jmpi || last->opcode() == G4_if) &&
            bb->Succs.size() == 2)
        {
            G4_BB* true_branch = bb->Succs.front();
            G4_BB* false_branch = bb->Succs.back();
            ordered_succs.push_back(false_branch);
            ordered_succs.push_back(true_branch);
        }

        std::list<G4_BB*>& succs = (ordered_succs.empty()) ? bb->Succs : ordered_succs;
        //
        // visit all successors
        //
        for (std::list<G4_BB*>::iterator it = succs.begin(), itEnd = succs.end(); it != itEnd; ++it)
        {
            G4_BB* succBB = *it;
            //
            // visit unmarked successors
            //
            if (succBB->getPreId() == UINT_MAX) {
                AssignDFSBasedIds(*it, preId, postId, rpoBBList);
            }
            //
            // track back-edges
            //
            else if (succBB->getRPostId() == UINT_MAX) {
                backEdges.push_back(Edge(bb, succBB));
            }
        }
    }
    //
    // Set the post id in the rpostid field. The caller will update the field to the
    // actual postid number.
    //
    bb->setRPostId(postId++);

    rpoBBList.push_front(bb);
}

// prevent overwriting dump file and indicate compilation order with dump serial number
static _THREAD int dotDumpCount = 0;

//
// Remove the fall through edges between subroutine and its non-caller preds
// Remove basic blocks that only contain a label, funcation lebels are untouched.
//
void FlowGraph::removeRedundantLabels()
{
    for (BB_LIST_ITER it = BBs.begin(); it != BBs.end();)
    {
        G4_BB* bb = *it;
        if (bb == getEntryBB())
        {
            it++;
            continue;
        }
        if (bb->Succs.size() == 0 && bb->Preds.size() == 0) {
            //leaving dangling BBs with return in for now.
            //workaround to handle unreachable return
            //for example return after infinite loop.
            if (bb->isEndWithFRet() || (bb->size() > 0 && ((G4_INST*)bb->back())->isReturn()))
            {
                it++;
                continue;
            }

            bb->clear();
            BB_LIST_ITER rt = it++;
            BBs.erase(rt);

            continue;
        }
        //
        // The removal candidates will have a single successor and a single inst
        //
        if (bb->Succs.size() == 1 && bb->size() == 1)
        {
            G4_INST* removedBlockInst = bb->front();
            if (removedBlockInst->isLabel() == false ||
                strncmp(removedBlockInst->getLabelStr(), "LABEL__EMPTYBB", 14) == 0 ||
                strncmp(removedBlockInst->getLabelStr(), "__AUTO_GENERATED_DUMMY_LAST_BB", 30) == 0)
            {
                ++it;
                continue;
            }

            // check if the label is a function label
            unsigned int numNonCallerPreds = 0;
            bool isFuncLabel = true;
            G4_BB* pred_bb = NULL;
            for (auto pred : bb->Preds)
            {
                if (!pred->isEndWithCall())
                {
                    if (numNonCallerPreds > 0)
                    {
                        isFuncLabel = false;
                        break;
                    }
                    numNonCallerPreds++;
                    pred_bb = pred;
                }
                else
                {
                    G4_INST *i = pred->back();
                    if (i->getSrc(0)->isLabel())
                    {
                        if (i->getSrc(0) != removedBlockInst->getLabel())
                        {
                            if (numNonCallerPreds > 0)
                            {
                                isFuncLabel = false;
                                break;
                            }
                            numNonCallerPreds++;
                            pred_bb = pred;
                        }
                    }
                }
            }

            // keep the function label there such that we have an empty init BB for this subroutine.
            if (isFuncLabel && numNonCallerPreds < bb->Preds.size())
            {
                // remove fall through edge.
                if (pred_bb)
                {
                    removePredSuccEdges(pred_bb, bb);
                }
                removedBlockInst->getLabel()->setFuncLabel(true);
                ++it;
                continue;
            }

            G4_Label *succ_label = bb->Succs.front()->front()->getLabel();

            // check if the last inst of pred is a control flow inst
            for (auto pred : bb->Preds)
            {
                auto jt = std::find(pred->Succs.begin(), pred->Succs.end(), bb);

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
                                // for G4_if , jump only when it has predictate; if no predicate, no jump
                                // this rule changed in GT as below
                                // [(<pred>)] if (<exec_size>) null null null <JIP>
                                // if[.<cmod>] (<exec_size>) null <src0> <src1> <JIP>
                                else if ((i->getPredicate() != NULL) ||
                                    ((i->getCondMod() != NULL) &&
                                    (i->getSrc(0) != NULL) &&
                                        (i->getSrc(1) != NULL))) {
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
            BB_LIST_ITER kt = bb->Succs.front()->Preds.begin();

            for (; kt != bb->Succs.front()->Preds.end(); ++kt)
            {
                if ((*kt) == bb)
                {
                    break;
                }
            }

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
            //
            // Update the call graph if this is a RETURN node.
            //
            if (bb->BBBeforeCall())
            {
                bb->BBBeforeCall()->setBBAfterCall(bb->Succs.front());
                bb->Succs.front()->setBBBeforeCall(bb->BBBeforeCall());
            }
            //
            // A CALL node should never be empty.
            //
            else if (bb->BBBeforeCall())
            {
                MUST_BE_TRUE(false, ERROR_FLOWGRAPH);
            }

            bb->Succs.clear();
            bb->Preds.clear();
            bb->clear();

            BB_LIST_ITER rt = it++;
            BBs.erase(rt);
        }
        else
        {
            ++it;
        }
    }

    reassignBlockIDs();
}

//
// remove any mov with the same src and dst opnds
//
void FlowGraph::removeRedundMov()
{
    for (std::list<G4_BB*>::iterator it = BBs.begin(); it != BBs.end(); ++it)
    {
        G4_BB* bb = (*it);

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
                            RegionDesc *rd = srcRgn->getRegion();
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

                for (auto predBB : bb->Preds)
                {
                    //
                    // Replace the predecessors successor links to the removed block's unique successor.
                    //
                    BB_LIST_ITER jt = predBB->Succs.begin();
                    BB_LIST_ITER jtEnd = predBB->Succs.end();

                    for (; jt != jtEnd; ++jt)
                    {
                        if ((*jt) == bb)
                        {
                            break;
                        }
                    }

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
                    BB_LIST_ITER kt = succBB->Preds.begin();
                    BB_LIST_ITER ktEnd = succBB->Preds.end();

                    for (; kt != ktEnd; ++kt)
                    {
                        if ((*kt) == bb)
                        {
                            break;
                        }
                    }

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
                    //
                    // Update the call graph if this is a RETURN node.
                    //
                    if (bb->BBBeforeCall())
                    {
                        bb->BBBeforeCall()->setBBAfterCall(succBB);
                        succBB->setBBBeforeCall(bb->BBBeforeCall());
                    }
                    else if (bb->BBBeforeCall())
                    {
                        //
                        // A CALL node should never be empty.
                        //
                        MUST_BE_TRUE(false, ERROR_FLOWGRAPH);
                    }
                }
                //
                // Remove the block to be removed.
                //
                bb->Succs.clear();
                bb->Preds.clear();
                bb->clear();

                BB_LIST_ITER rt = it++;
                BBs.erase(rt);
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

    for (BB_LIST_ITER bb_it = BBs.begin(), bb_itEnd = BBs.end();
        bb_it != bb_itEnd;
        bb_it++)
    {
        G4_BB* cur = (*bb_it);

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
            std::string str = "__MERGED_FRET_EXIT_BLOCK_f" + std::to_string(builder->getCUnitId());
            dumLabel = builder->createLabel(str, LABEL_BLOCK);
            G4_INST* label = createNewLabelInst(dumLabel);
            newExit->push_back(label);
            G4_INST* fret = builder->createInst(NULL, G4_pseudo_fret, NULL, false, 1, NULL, NULL, NULL, 0);
            newExit->push_back(fret);
            BBs.push_back(newExit);
            candidateFretBB = newExit;
        }

        for (BB_LIST_ITER it = exitBBs.begin(); it != exitBBs.end(); ++it)
        {
            G4_BB* cur = (*it);

            if (cur != candidateFretBB)
            {
                G4_INST* last = cur->back();
                addPredSuccEdges(cur, candidateFretBB);

                last->setOpcode(G4_jmpi);
                last->setSrc(dumLabel, 0);
                last->setExecSize(1);
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
    for (std::list<G4_BB*>::iterator it = BBs.begin(); it != BBs.end(); ++it)
    {
        G4_BB *bb = *it;
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

        for (std::list<G4_BB*>::iterator it = exitBBs.begin(), itEnd = exitBBs.end(); it != itEnd; ++it)
        {
            G4_BB *bb = *it;
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
    std::list<G4_BB*> function_start_list;
    unsigned int i = 0;
    for (BB_LIST_ITER it = BBs.begin(), itEnd = BBs.end(); it != itEnd; ++it)
    {
        G4_BB* bb = *it;
        bb->setId(i);
        i++;
        MUST_BE_TRUE(i <= getNumBB(), ERROR_FLOWGRAPH);
    }

    numBBId = i;
}

//
// given a label string, find its BB and the label's offset in the BB
// label_offset is the offset of label in BB, since there may be nop insterted before the label
//
G4_BB *FlowGraph::findLabelBB(char *label, int &label_offset)
{
    MUST_BE_TRUE(label, ERROR_INTERNAL_ARGUMENT);

    for (BB_LIST_ITER it = BBs.begin(), itEnd = BBs.end(); it != itEnd; ++it)
    {
        G4_BB* bb = *it;
        G4_INST *first = bb->empty() ? NULL : bb->front();

        const char *label_t = NULL;

        if (first && first->isLabel())
        {
            label_t = first->getLabelStr();
            label_offset = 0;
        }
        if (label_t == NULL)
            continue;

        if (strcmp(label, label_t) == 0)
            return bb;
    }

    return NULL;
}

/*
*  Mark blocks that are nested in SIMD control flow.
*  Only structured CF is handled here, SIMD BBs due to goto/join
*  are marked in processGoto()
*
*  This function also sets the JIP of the endif to its enclosing endif/while, if it exists
*  Note: we currently do not consider goto/join when adding the JIP for endif,
*  since structure analysis should not allow goto into/out of if-endif.  This means
*  we only need to set the the JIP of the endif to its immediately enclosing endif/while
*
*  The simd control flow blocks must be well-structured
*
*/
void FlowGraph::markSimdBlocks(std::map<std::string, G4_BB*>& labelMap, FuncInfoHashTable &FuncInfoMap)
{
    std::stack<StructuredCF*> ifAndLoops;
    std::vector<StructuredCF*> structuredSimdCF;

    for (BB_LIST_ITER it = BBs.begin(), itEnd = BBs.end(); it != itEnd; ++it)
    {
        G4_BB* bb = *it;
        for (INST_LIST_ITER it = bb->begin(), _itEnd = bb->end(); it != _itEnd; ++it)
        {
            G4_INST* inst = *it;
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
        for (BB_LIST_ITER preds = bb->Preds.begin(), predsEnd = bb->Preds.end(); preds != predsEnd; ++preds)
        {
            G4_BB* predBB = *preds;
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

        if (ifAndLoops.size() > 0)
        {
            bb->setInSimdFlow(true);
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

    for (int i = 0, size = (int)structuredSimdCF.size(); i < size; i++)
    {
        StructuredCF* cf = structuredSimdCF[i];
        if (cf->mType == STRUCTURED_CF_IF && cf->enclosingCF != NULL)
        {
            setJIPForEndif(cf->mEndInst, cf->enclosingCF->mEndInst, cf->enclosingCF->mEndBB);
        }
    }

    // Visit the call graph, and mark simd blocks in subroutines.
    std::set<FuncInfo*> Visited;
    std::queue<FuncInfo*> Funcs;

    // Starting with kernel.
    Funcs.push(kernelInfo);

    // Now process all subroutines called in a simd-cf block.
    while (!Funcs.empty())
    {
        FuncInfo* CallerInfo = Funcs.front();
        Funcs.pop();

        // Skip if this is already visited.
        if (!Visited.insert(CallerInfo).second)
            continue;

        for (auto BB : CallerInfo->getBBList())
        {
            if (BB->isInSimdFlow() && BB->isEndWithCall())
            {
                G4_INST *CI = BB->back();
                if (CI->getSrc(0)->isLabel())
                {
                    G4_Label* Callee = CI->getSrc(0)->asLabel();
                    G4_BB* CalleeEntryBB = labelMap[Callee->getLabel()];
                    FuncInfo* CalleeInfo = CalleeEntryBB->getFuncInfo();
                    Funcs.push(CalleeInfo);

                    // Mark all blocks in this subroutine.
                    for (auto BB1 : CalleeInfo->getBBList())
                    {
                        BB1->setInSimdFlow(true);
                    }
                }
            }
        }
    }
}

/*
* Insert a join at the beginning of this basic block, immediately after the label
* If a join is already present, nothing will be done
*/
void FlowGraph::insertJoinToBB(G4_BB* bb, uint8_t execSize, G4_Label* jip)
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
        bb->push_back(jInst);
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
            G4_INST* jInst = builder->createInternalCFInst(NULL, G4_join, execSize, jip, NULL, InstOpt_NoOpt,
                secondInst->getLineNo(), secondInst->getCISAOff(), secondInst->getSrcFilename());
            bb->insert(iter, jInst);
        }
    }
}

typedef std::pair<G4_BB*, int> BlockSizePair;

static void addBBToActiveJoinList(std::list<BlockSizePair>& activeJoinBlocks, G4_BB* bb, int execSize)
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

G4_Label* FlowGraph::insertEndif(G4_BB* bb, unsigned char execSize, bool createLabel)
{
    // endif is placed immediately after the label
    G4_INST* endifInst = builder->createInternalCFInst(NULL, G4_endif, execSize, NULL, NULL, InstOpt_NoOpt);
    INST_LIST_ITER iter = bb->begin();
    MUST_BE_TRUE(iter != bb->end(), "empty BB");
    iter++;
    bb->insert(iter, endifInst);

    // this block may be a target of multiple ifs, in which case we will need to insert
    // one endif for each if.  The innermost endif will use the BB label, while for the other
    // endifs a new label will be created for each of them.
    if (createLabel)
    {
        std::string name = "_AUTO_LABEL_%d" + std::to_string(autoLabelId++);
        G4_Label* label = builder->createLabel(name, LABEL_BLOCK);
        endifWithLabels.emplace(endifInst, label);
        return label;
    }
    else
    {
        return bb->getLabel();
    }
}

/*
*  This function set the JIP of the endif to the target instruction (either endif or while)
*
*/
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
            for (INST_LIST_ITER it = targetBB->begin(), itEnd = targetBB->end();
                it != itEnd; ++it)
            {
                G4_INST* inst = *it;
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
            std::string name = "_AUTO_LABEL_" + std::to_string(autoLabelId++);
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
    // list of active blocks where a join needs to be inserted, sorted in lexical order
    std::list<BlockSizePair> activeJoinBlocks;
    bool doScalarJmp = !builder->noScalarJmp();

    for (BB_LIST_ITER it = BBs.begin(), itEnd = BBs.end(); it != itEnd; ++it)
    {
        G4_BB* bb = *it;
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
                int execSize = activeJoinBlocks.front().second;
                G4_Label* joinJIP = NULL;

                activeJoinBlocks.pop_front();
                if (activeJoinBlocks.size() > 0)
                {
                    //set join JIP to the next active join
                    G4_BB* joinBlock = activeJoinBlocks.front().first;
                    joinJIP = joinBlock->getLabel();
                }

                insertJoinToBB(bb, (uint8_t)execSize, joinJIP);
            }
        }

        // check to see if this block is the target of one (or more) backward goto
        // If so, we process the backward goto and push its fall-thru block to the
        // active join list
        for (std::list<G4_BB*>::iterator iter = bb->Preds.begin(), iterEnd = bb->Preds.end(); iter != iterEnd; ++iter)
        {
            G4_BB* predBB = *iter;
            G4_INST *lastInst = predBB->back();
            if (lastInst->opcode() == G4_goto && lastInst->asCFInst()->isBackward() &&
                lastInst->asCFInst()->getUip() == bb->getLabel())
            {
                // backward goto
                bool isUniform = lastInst->getExecSize() == 1 || lastInst->getPredicate() == NULL;
                if (isUniform && doScalarJmp)
                {
                    // can always convert a uniform backward goto into a jmp
                    convertGotoToJmpi(lastInst);

                    // No need to do the following for scalar jump. If there are gotos inside loop,
                    // the join point will be updated when handling gotos.

                    // we still have to add a join point at the BB immediately after the back edge,
                    // since there may be subsequent loop breaks that are waiting there.
                    // example:
                    // L1:
                    // (P1) goto exit
                    // (P2) goto L2
                    // goto L1
                    // L2:
                    // ...
                    // exit:
                    //
                    // In this case the goto exit's JIP should be set to L2 as there may be channels
                    // waiting there due to "goto L2"
                    G4_BB* loopExitBB = predBB->getPhysicalSucc();
                    // loop exit may be null if the loop is the outer-most one
                    // (i.e., the loop has no breaks but only EOT sends)
                    if (loopExitBB != NULL)
                    {
                        addBBToActiveJoinList(activeJoinBlocks, loopExitBB, lastInst->getExecSize());
                    }

                }
                else
                {
                    uint8_t eSize = lastInst->getExecSize() > 1 ? lastInst->getExecSize() : pKernel->getSimdSize();
                    if (lastInst->getExecSize() == 1)
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

        // at this point if there are active join blocks, we are in SIMD control flow
        // FIXME: This is over pessimistic for kernels with actual simd cf.
        if (HasSIMDCF && !activeJoinBlocks.empty())
        {
            bb->setInSimdFlow(true);
        }

        G4_INST* lastInst = bb->back();
        if (lastInst->opcode() == G4_goto && !lastInst->asCFInst()->isBackward())
        {
            // forward goto
            // the last Succ BB is our goto target
            G4_BB* gotoTargetBB = bb->Succs.back();
            bool isUniform = lastInst->getExecSize() == 1 || lastInst->getPredicate() == NULL;

            if (isUniform && doScalarJmp &&
                (activeJoinBlocks.size() == 0 || activeJoinBlocks.front().first->getId() > gotoTargetBB->getId()))
            {
                // can convert goto into a scalar jump to UIP, if the jmp will not make us skip any joins
                // CFG itself does not need to be updated
                convertGotoToJmpi(lastInst);
            }
            else
            {
                //set goto JIP to the first active block
                uint8_t eSize = lastInst->getExecSize() > 1 ? lastInst->getExecSize() : pKernel->getSimdSize();
                addBBToActiveJoinList(activeJoinBlocks, gotoTargetBB, eSize);
                G4_BB* joinBlock = activeJoinBlocks.front().first;
                if (lastInst->getExecSize() == 1)
                {   // For simd1 goto, convert it to a goto with the right execSize.
                    lastInst->setExecSize(eSize);
                    lastInst->setOptions(InstOpt_M0);
                }
                lastInst->asCFInst()->setJip(joinBlock->getLabel());

                if (!builder->gotoJumpOnTrue())
                {
                    // For BDW/SKL goto, the false channels are the ones that actually will take the jump,
                    // and we thus have to flip the predicate
                    G4_Predicate *pred = lastInst->getPredicate();
                    if (pred != NULL)
                    {
                        pred->setState(pred->getState() == PredState_Plus ? PredState_Minus : PredState_Plus);
                    }
                    else
                    {
                        // if there is no predicate, generate a predicate with all 0s.
                        // if predicate is SIMD32, we have to use a :ud dst type for the move
                        uint8_t execSize = lastInst->getExecSize() > 16 ? 2 : 1;
                        G4_Declare* tmpFlagDcl = builder->createTempFlag(execSize);
                        G4_DstRegRegion* newPredDef = builder->createDstRegRegion(Direct, tmpFlagDcl->getRegVar(), 0, 0, 1, execSize == 2 ? Type_UD : Type_UW);
                        G4_INST *predInst = builder->createInternalInst(NULL, G4_mov, NULL, false, 1,
                            newPredDef, builder->createImm(0, Type_UW), NULL,
                            InstOpt_WriteEnable, lastInst->getLineNo(), lastInst->getCISAOff(), lastInst->getSrcFilename());
                        INST_LIST_ITER iter = bb->end();
                        iter--;
                        bb->insert(iter, predInst);

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

//
// Evaluate AddrExp/AddrExpList to Imm
//
void G4_Kernel::evalAddrExp()
{
    for (std::list<G4_BB*>::iterator it = fg.begin(), itEnd = fg.end(); it != itEnd; ++it)
    {
        G4_BB* bb = (*it);

        for (INST_LIST_ITER i = bb->begin(), iEnd = bb->end(); i != iEnd; i++)
        {
            G4_INST* inst = (*i);

            //
            // process each source operand
            //
            for (unsigned j = 0; j < G4_MAX_SRCS; j++)
            {
                G4_Operand* opnd = inst->getSrc(j);

                if (opnd == NULL) continue;

                if (opnd->isAddrExp())
                {
                    int val = opnd->asAddrExp()->eval();
                    G4_Type ty = opnd->asAddrExp()->getType();

                    G4_Imm* imm = fg.builder->createImm(val, ty);
                    inst->setSrc(imm, j);
                }
            }
        }
    }
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
        scratchRegDcl = builder.createDeclareNoLookup("SR", G4_GRF, 8, 2, Type_UD);
        scratchRegDcl->getRegVar()->setPhyReg(regPool.getGreg(builder.kernel.getStackCallStartReg() + 1), 0);
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
    if (callSites.size() <= pseudoVCADclList.size())
    {
        std::vector<G4_Declare*>::iterator it = pseudoVCADclList.begin();
        for (auto callsite : callSites)
        {
            (*it)->getRegVar()->setPhyReg(NULL, 0);
            callsite->asCFInst()->setAssocPseudoVCA((*it)->getRegVar());
            ++it;
        }
    }
    else
    {
        INST_LIST_ITER it = callSites.begin();
        INST_LIST_ITER itEnd = callSites.end();
        for (auto pseudoVCADcl : pseudoVCADclList)
        {
            MUST_BE_TRUE(it != callSites.end(), "incorrect call sites");
            pseudoVCADcl->getRegVar()->setPhyReg(NULL, 0);
            (*it)->asCFInst()->setAssocPseudoVCA(pseudoVCADcl->getRegVar());
            ++it;
        }
        for (unsigned id = (unsigned)pseudoVCADclList.size(); it != itEnd; ++it, ++id)
        {
            const char* nameBase = "VCA_SAVE";
            const int maxIdLen = 3;
            MUST_BE_TRUE(id < 1000, ERROR_FLOWGRAPH);
            const char *name = builder.getNameString(mem, strlen(nameBase) + maxIdLen + 1, "%s_%d", nameBase, id);
            pseudoVCADclList.push_back(builder.createDeclareNoLookup(name, G4_GRF, 8, 59, Type_UD));
            (*it)->asCFInst()->setAssocPseudoVCA(pseudoVCADclList.back()->getRegVar());
        }
    }
    //
    // VCE_SAVE (r60.0-r125.0) [r125-127 is reserved]
    //
    if (pseudoVCEDcl == NULL)
    {
        unsigned int numRowsVCE = getKernel()->getNumCalleeSaveRegs();
        pseudoVCEDcl = builder.createDeclareNoLookup("VCE_SAVE", G4_GRF, 8, static_cast<unsigned short>(numRowsVCE), Type_UD);
    }
    else
    {
        pseudoVCEDcl->getRegVar()->setPhyReg(NULL, 0);
    }

    //
    // Insert caller save decls for A0
    //
    unsigned int i = 0;
    for (auto callSite : callSites)
    {
        const char* name = builder.getNameString(mem, 50, builder.getIsKernel() ? "k%d_SA0_%d" : "f%d_SA0_%d", builder.getCUnitId(), i);
        G4_Declare* saveA0 = builder.createDeclareNoLookup(name, G4_ADDRESS, (unsigned short)getNumAddrRegisters(), 1, Type_UW);
        pseudoA0DclList.push_back(saveA0);
        callSite->asCFInst()->setAssocPseudoA0Save(saveA0->getRegVar());
        i++;
    }

    //
    // Insert caller save decls for flag
    //
    unsigned int j = 0;
    for (auto callSite : callSites)
    {
        const char *name = builder.getNameString(mem, 64, builder.getIsKernel() ? "k%d_SFLAG_%d" : "f%d_SFLAG_%d", builder.getCUnitId(), j);
        G4_Declare* saveFLAG = builder.createDeclareNoLookup(name, G4_FLAG, (uint16_t)builder.getNumFlagRegisters(), 1, Type_UW);
        pseudoFlagDclList.push_back(saveFLAG);
        callSite->asCFInst()->setAssocPseudoFlagSave(saveFLAG->getRegVar());
        j++;
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

// Dump the instructions into a file
void G4_Kernel::dumpPassInternal(const char* appendix)
{
    MUST_BE_TRUE(appendix != NULL, ERROR_INTERNAL_ARGUMENT);
    if (!m_options->getOption(vISA_DumpPasses))  // skip dumping dot files
        return;


    char fileName[256];
    MUST_BE_TRUE(strlen(appendix) < 40, ERROR_INVALID_VISA_NAME(appendix));
    if (name != NULL)
    {
        MUST_BE_TRUE(strlen(name) < 206, ERROR_INVALID_VISA_NAME(name));
        SNPRINTF(fileName, sizeof(fileName), "%s.%03d.%s.dump", name, dotDumpCount++, appendix);
    }
    else
    {
        SNPRINTF(fileName, sizeof(fileName), "%s.%03d.%s.dump", "UnknownKernel", dotDumpCount++, appendix);
    }

    std::string fname(fileName);
    fname = sanitizePathString(fname);

    fstream ofile(fname, ios::out);
    if (!ofile)
    {
        MUST_BE_TRUE(false, ERROR_FILE_READ(fileName));
    }

    const char* asmFileName = NULL;
    m_options->getOption(VISA_AsmFileName, asmFileName);
    if (asmFileName == NULL)
        ofile << "UnknownKernel" << std::endl << std::endl;
    else
        ofile << asmFileName << std::endl << std::endl;

    for (std::list<G4_BB*>::iterator it = fg.begin();
        it != fg.end(); ++it)
    {
        // Emit BB number
        G4_BB* bb = (*it);
        bb->writeBBId(ofile);
        ofile << "\tPreds: ";
        for (auto pred : bb->Preds)
        {
            pred->writeBBId(ofile);
            ofile << " ";
        }
        ofile << "\tSuccs: ";
        for (auto succ : bb->Succs)
        {
            succ->writeBBId(ofile);
            ofile << " ";
        }
        ofile << "\n";

        bb->emit(ofile);
        ofile << "\n\n";
    } // bbs

    ofile.close();
}

//
// This routine dumps out the dot file of the control flow graph along with instructions.
// dot is drawing graph tool from AT&T.
//
void G4_Kernel::dumpDotFileInternal(const char* appendix)
{
    MUST_BE_TRUE(appendix != NULL, ERROR_INTERNAL_ARGUMENT);
    if (!m_options->getOption(vISA_DumpDot))  // skip dumping dot files
        return;

    //
    // open the dot file
    //
    char fileName[256];
    MUST_BE_TRUE(strlen(appendix) < 40, ERROR_INVALID_VISA_NAME(appendix));
    if (name != NULL)
    {
        MUST_BE_TRUE(strlen(name) < 206, ERROR_INVALID_VISA_NAME(name));
        SNPRINTF(fileName, sizeof(fileName), "%s.%03d.%s.dot", name, dotDumpCount++, appendix);
    }
    else
    {
        SNPRINTF(fileName, sizeof(fileName), "%s.%03d.%s.dot", "UnknownKernel", dotDumpCount++, appendix);
    }

    std::string fname(fileName);
    fname = sanitizePathString(fname);

    fstream ofile(fname, ios::out);
    if (!ofile)
    {
        MUST_BE_TRUE(false, ERROR_FILE_READ(fname));
    }
    //
    // write digraph KernelName {"
    //          size = "8, 10";
    //
    const char* asmFileName = NULL;
    m_options->getOption(VISA_AsmFileName, asmFileName);
    if (asmFileName == NULL)
        ofile << "digraph UnknownKernel" << " {" << std::endl;
    else
        ofile << "digraph " << asmFileName << " {" << std::endl;
    //
    // keep the graph width 8, estimate a reasonable graph height
    //
    const unsigned itemPerPage = 64;                                        // 60 instructions per Letter page
    unsigned totalItem = (unsigned)Declares.size();
    for (std::list<G4_BB*>::iterator it = fg.begin(); it != fg.end(); ++it)
        totalItem += ((unsigned int)(*it)->size());
    totalItem += (unsigned)fg.size();
    float graphHeight = (float)totalItem / itemPerPage;
    graphHeight = graphHeight < 100.0f ? 100.0f : graphHeight;    // minimal size: Letter
    ofile << endl << "\t// Setup" << endl;
    ofile << "\tsize = \"80.0, " << graphHeight << "\";\n";
    ofile << "\tpage= \"80.5, 110\";\n";
    ofile << "\tpagedir=\"TL\";\n";
    //
    // dump out declare information
    //     Declare [label="
    //
    //if (name == NULL)
    //  ofile << "\tDeclares [shape=record, label=\"{kernel:UnknownKernel" << " | ";
    //else
    //  ofile << "\tDeclares [shape=record, label=\"{kernel:" << name << " | ";
    //for (std::list<G4_Declare*>::iterator it = Declares.begin(); it != Declares.end(); ++it)
    //{
    //  (*it)->emit(ofile, true, Options::symbolReg);   // Solve the DumpDot error on representing <>
    //
    //  ofile << "\\l";  // left adjusted
    //}
    //ofile << "}\"];" << std::endl;
    //
    // dump out flow graph
    //
    for (std::list<G4_BB*>::iterator it = fg.begin(); it != fg.end(); ++it)
    {
        G4_BB* bb = (*it);
        //
        // write:   BB0 [shape=plaintext, label=<
        //                      <TABLE BORDER="0" CELLBORDER="1" CELLSPACING="0">
        //                          <TR><TD ALIGN="CENTER">BB0: TestRA_Dot</TD></TR>
        //                          <TR><TD>
        //                              <TABLE BORDER="0" CELLBORDER="0" CELLSPACING="0">
        //                                  <TR><TD ALIGN="LEFT">TestRA_Dot:</TD></TR>
        //                                  <TR><TD ALIGN="LEFT"><FONT color="red">add (8) Region(0,0)[1] Region(0,0)[8;8,1] PAYLOAD(0,0)[8;8,1] [NoMask]</FONT></TD></TR>
        //                              </TABLE>
        //                          </TD></TR>
        //                      </TABLE>>];
        // print out label if the first inst is a label inst
        //
        ofile << "\t";
        bb->writeBBId(ofile);
        ofile << " [shape=plaintext, label=<" << std::endl;
        ofile << "\t\t\t    <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\">" << std::endl;
        ofile << "\t\t\t\t<TR><TD ALIGN=\"CENTER\">";
        bb->writeBBId(ofile);
        ofile << ": ";

        if (!bb->empty() && bb->front()->isLabel())
        {
            bb->front()->getSrc(0)->emit(ofile);
        }
        ofile << "</TD></TR>" << std::endl;
        //emit all instructions within basic block
        ofile << "\t\t\t\t<TR><TD>" << std::endl;

        if (!bb->empty())
        {
            ofile << "\t\t\t\t\t    <TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"0\">" << std::endl;
            for (INST_LIST_ITER i = bb->begin(); i != bb->end(); i++)
            {
                //
                // detect if there is spill code first, set different color for it
                //
                std::string fontColor = "black";
                //
                // emit the instruction
                //
                ofile << "\t\t\t\t\t\t<TR><TD ALIGN=\"LEFT\"><FONT color=\"" << fontColor << "\">";
                std::ostringstream os;
                (*i)->emit(os, m_options->getOption(vISA_SymbolReg), true);
                std::string dotStr(os.str());
                //TODO: dot doesn't like '<', '>', '{', or '}' (and '&') this code below is a hack. need to replace with delimiters.
                std::replace_if(dotStr.begin(), dotStr.end(), bind2nd(equal_to<char>(), '<'), '[');
                std::replace_if(dotStr.begin(), dotStr.end(), bind2nd(equal_to<char>(), '>'), ']');
                std::replace_if(dotStr.begin(), dotStr.end(), bind2nd(equal_to<char>(), '{'), '[');
                std::replace_if(dotStr.begin(), dotStr.end(), bind2nd(equal_to<char>(), '}'), ']');
                std::replace_if(dotStr.begin(), dotStr.end(), bind2nd(equal_to<char>(), '&'), '$');
                ofile << dotStr;

                ofile << "</FONT></TD></TR>" << std::endl;
                //ofile << "\\l"; // left adjusted
            }
            ofile << "\t\t\t\t\t    </TABLE>" << std::endl;
        }

        ofile << "\t\t\t\t</TD></TR>" << std::endl;
        ofile << "\t\t\t    </TABLE>>];" << std::endl;
        //
        // dump out succ edges
        // BB12 -> BB10
        //
        for (std::list<G4_BB*>::iterator sit = bb->Succs.begin(); sit != bb->Succs.end(); ++sit)
        {
            bb->writeBBId(ofile);
            ofile << " -> ";
            (*sit)->writeBBId(ofile);
            ofile << std::endl;
        }
    }
    //
    // write "}" to end digraph
    //
    ofile << std::endl << " }" << std::endl;
    //
    // close dot file
    //
    ofile.close();
}

// Wrapper function
void G4_Kernel::dumpDotFile(const char* appendix)
{
    if (m_options->getOption(vISA_DumpDot))
        dumpDotFileInternal(appendix);
    if (m_options->getOption(vISA_DumpPasses))
        dumpPassInternal(appendix);
}

static const char* const RATypeString[] =
{
    RA_TYPE(STRINGIFY)
};

static iga_gen_t getIGAPlatform()
{
    iga_gen_t platform = IGA_GEN_INVALID;
    switch (getGenxPlatform())
    {
    case GENX_BDW:
        platform = IGA_GEN8;
        break;
    case GENX_CHV:
        platform = IGA_GEN8lp;
        break;
    case GENX_SKL:
        platform = IGA_GEN9;
        break;
    case GENX_BXT:
        platform = IGA_GEN9lp;
        break;
    case GENX_CNL:
        platform = IGA_GEN10;
        break;
    case GENX_ICL:
    case GENX_ICLLP:
        platform = IGA_GEN11;
        break;
    default:
        break;
    }

    return platform;
}

vector<string>
split(const string & str, const char * delimiter) {
    vector<string> v;
    string::size_type start = 0;

    for (auto pos = str.find_first_of(delimiter, start); pos != string::npos; start = pos + 1, pos = str.find_first_of(delimiter, start))
    {
        if (pos != start)
        {
            v.emplace_back(str, start, pos - start);
        }


    }

    if (start < str.length())
        v.emplace_back(str, start, str.length() - start);
    return v;
}
#ifdef DEBUG_VERBOSE_ON
static int noBankCount = 0;
#endif
void G4_Kernel::emit_asm(std::ostream& output, bool beforeRegAlloc, void * binary, uint32_t binarySize)
{
    //
    // for GTGPU lib release, don't dump out asm
    //
#ifdef NDEBUG
#ifdef GTGPU_LIB
    return;
#endif
#endif
    bool newAsm = false;
    if (m_options->getOption(vISA_dumpNewSyntax) && !(binary == NULL || binarySize == 0))
    {
        newAsm = true;
    }

    if (!m_options->getOption(vISA_StripComments))
    {
        output << "//.kernel ";
        if (name != NULL)
        {
            // some 3D kernels do not have name
            output << name;
        }

        output << "\n" << "//.platform " << platformString[getGenxPlatform()];
        output << "\n" << "//.stepping " << GetSteppingString();
        output << "\n" << "//.CISA version " << (unsigned int)major_version
            << "." << (unsigned int)minor_version;
        output << "\n" << "//.options " << m_options->getArgString().str();
        output << "\n" << "//.instCount " << asmInstCount;
        output << "\n//.RA type\t" << RATypeString[RAType];

        if (auto jitInfo = fg.builder->getJitInfo())
        {
            if (jitInfo->numGRFUsed != 0)
            {
                output << "\n" << "//.GRF count " << jitInfo->numGRFUsed;
            }
            if (jitInfo->spillMemUsed > 0)
            {
                output << "\n" << "//.spill size " << jitInfo->spillMemUsed;
            }
            if (jitInfo->numGRFSpillFill > 0)
            {
                output << "\n" << "//.spill GRF ref count " << jitInfo->numGRFSpillFill;
            }
            if (jitInfo->numFlagSpillStore > 0)
            {
                output << "\n//.spill flag store " << jitInfo->numFlagSpillStore;
                output << "\n//.spill flag load " << jitInfo->numFlagSpillLoad;
            }
        }

        output << "\n\n";

        //Step2: emit declares (as needed)
        //
        // firstly, emit RA declare as comments or code depends on Options::symbolReg
        // we check if the register allocation is successful here
        //

        for (auto dcl : Declares)
        {
            dcl->emit(output, false, m_options->getOption(vISA_SymbolReg));
            output << "\n";
        }

        // emit input location and size
        output << "//.kernel_reordering_info_start" << std::endl;
        output << "//id\tbyte_offset\tbyte_size\tkind\timplicit_kind" << std::endl;

        unsigned int inputCount = fg.builder->getInputCount();
        for (unsigned int id = 0; id < inputCount; id++)
        {
            input_info_t* input_info = fg.builder->getInputArg(id);
            output << "//.arg_" << (id + 1) << "\t" << input_info->offset
                << "\t" << input_info->size << "\t"
                << (int)input_info->getInputClass() << "\t"
                << (int)input_info->getImplicitKind() << std::endl;
        }

        output << "//.kernel_reordering_info_end" << std::endl;
        fg.BCStats.clear();
    }


    // Set this to NULL to always print filename for each kernel
    prevFilename = nullptr;
    prevSrcLineNo = 0;

    if (!newAsm)
    {
        //Step3: emit code and subroutines
        output << std::endl << ".code";
    }

    if (newAsm)
    {
        char stringBuffer[512];
        uint32_t pc = 0;
        output << std::endl;
        bool dissasemblyFailed = false;
#define ERROR_STRING_MAX_LENGTH 1024*16
        char* errBuf = new char[ERROR_STRING_MAX_LENGTH];

        KernelView kView(getIGAPlatform(), binary, binarySize,
                         errBuf, ERROR_STRING_MAX_LENGTH);
        dissasemblyFailed = !kView.decodeSucceeded();

        std::string igaErrMsgs;
        std::vector<std::string> igaErrMsgsVector;
        std::map<int, std::string> errorToStringMap;
        if (dissasemblyFailed)
        {
            std::cerr << "Failed to decode binary for asm output. Please report the issue and try disabling IGA disassembler for now." << std::endl;
            igaErrMsgs = std::string(errBuf);
            igaErrMsgsVector = split(igaErrMsgs, "\n");
            for (auto msg : igaErrMsgsVector)
            {
                auto pos = msg.find("ERROR");
                if (pos != string::npos)
                {
                    std::cerr << msg.c_str() << std::endl;
                    std::vector<string> aString = split(msg, " ");
                    for (auto token : aString)
                    {
                        if (token.find_first_of("0123456789") != string::npos)
                        {
                            int errorPC = std::atoi(token.c_str());
                            errorToStringMap[errorPC] = msg;
                            break;
                        }
                    }
                }
            }
        }

        //
        // For label, generate a label with uniqueLabel as prefix (required by some tools).
        // We do so by using labeler callback.  If uniqueLabels is not present, use iga's
        // default label.  For example,
        //   Without option -uniqueLabels:
        //      generating default label,   L1234
        //   With option -uniqueLabels <sth>:
        //      generating label with <sth> as prefix, <sth>_L1234
        //
        const char* labelPrefix = nullptr;
        if (m_options->getOption(vISA_UniqueLabels))
        {
            m_options->getOption(vISA_LabelStr, labelPrefix);
        }
        typedef struct {
            char m_labelString[128]; // label string for uniqueLabels
            char* m_labelPrefix;    // label prefix
            char m_tmpString[64];   // tmp storage, default label
            KernelView *m_pkView;   // handle to KernelView object.
        } lambdaArg_t;
        lambdaArg_t lambdaArg;
        lambdaArg.m_labelPrefix = const_cast<char*>(labelPrefix);
        lambdaArg.m_pkView = &kView;

        // Labeler callback function.
        auto labelerLambda = [](int32_t pc, void *data) -> const char*
        {
            lambdaArg_t *pArg = (lambdaArg_t *)data;
            char* tmpString = pArg->m_tmpString;
            char* labelString = pArg->m_labelString;

            pArg->m_pkView->getDefaultLabelName(pc, tmpString, 64);
            const char *retString;
            if (pArg->m_labelPrefix != nullptr)
            {
                SNPRINTF(labelString, 128, "%s_%s", (const char*)pArg->m_labelPrefix, tmpString);
                retString = labelString;
            }
            else
            {
                retString = tmpString;
            }
            return retString;
        };

        int suppressRegs[5];
        int lastRegs[3];
        for (int i = 0; i < 3; i++)
        {
            suppressRegs[i] = -1;
            lastRegs[i] = -1;
        }

        suppressRegs[4] = 0;

        uint32_t lastLabelPC = 0;
        for (BB_LIST_ITER itBB = fg.begin(); itBB != fg.end(); ++itBB)
        {
            for (INST_LIST_ITER itInst = (*itBB)->begin(); itInst != (*itBB)->end(); ++itInst)
            {

                bool isInstTarget = kView.isInstTarget(pc);
                if (isInstTarget)
                {
                    const char* stringLabel = labelerLambda(pc, (void *)&lambdaArg);

                    if ((*itInst)->isLabel())
                    {
                        output << "\n\n//" << (*itInst)->getLabelStr() << ":";
                        //handling the case where there is an empty block with just label.
                        //this way we don't print IGA label twice
                        if ((*itBB)->size() == 1)
                        {
                            break;
                        }
                    }

                    //preventing the case where there are two labels in G4 IR so duplicate IGA labels are printed
                    //then parser asserts.
                    /*
                        //label_cf_20_particle:
                        L3152:

                        //label12_particle:
                        L3152:

                        endif (32|M0)                        L3168                            // [3152]: #218 //:$239:%332
                        */
                    if (pc != lastLabelPC || pc == 0)
                    {
                        output << "\n" << stringLabel << ":" << std::endl;
                        lastLabelPC = pc;
                    }

                    if ((*itInst)->isLabel())
                    {
                        ++itInst;
                        //G4_IR has instruction for label.
                        if (itInst == (*itBB)->end())
                        {
                            break;
                        }
                    }
                }
                else if ((*itInst)->isLabel())
                {
                    output << "\n\n//" << (*itInst)->getLabelStr() << ":";
                    continue;
                }

                (*itBB)->emitInstructionInfo(output, itInst);
                output << std::endl;

                auto errString = errorToStringMap.find(pc);
                if (errString != errorToStringMap.end())
                {
                    output << "// " << errString->second.c_str() << std::endl;
                    output << "// Text representation might not be correct" << std::endl;
                }

                kView.getInstSyntax(pc, stringBuffer, 512, labelerLambda, (void*)&lambdaArg);
                pc += kView.getInstSize(pc);

                (*itBB)->emitBasicInstructionIga(stringBuffer, output, itInst, suppressRegs, lastRegs);
            }
        }

        delete [] errBuf;
    }
    else
    {
        for (BB_LIST_ITER it = fg.begin(); it != fg.end(); ++it)
        {
            output << std::endl;
            (*it)->emit(output);

        }
    }
#ifdef DEBUG_VERBOSE_ON
    printf("noBankCount: %d\n", noBankCount);
#endif
    if (!newAsm)
    {
        //Step4: emit clean-up.
        output << std::endl;
        output << ".end_code" << std::endl;
        output << ".end_kernel" << std::endl;
        output << std::endl;
    }
    if (newAsm)
    {
        output << "// Bank Conflict Statistics: \n";
        output << "// -- GOOD: " << fg.BCStats.NumOfGoodInsts << "\n";
        output << "// --  BAD: " << fg.BCStats.NumOfBadInsts << "\n";
        output << "// --   OK: " << fg.BCStats.NumOfOKInsts << "\n";

    }
}

//
//  Add an EOT send to the end of this BB.
//
void G4_BB::addEOTSend(G4_INST* lastInst)
{
    // mov (8) r1.0<1>:ud r0.0<8;8,1>:ud {NoMask}
    // send (8) null r1 0x27 desc
    IR_Builder* builder = parent->builder;
    G4_Declare *dcl = builder->Create_MRF_Dcl(NUM_DWORDS_PER_GRF, Type_UD);
    G4_DstRegRegion* movDst = builder->Create_Dst_Opnd_From_Dcl(dcl, 1);
    G4_SrcRegRegion* r0Src = builder->Create_Src_Opnd_From_Dcl(
        builder->getBuiltinR0(), builder->getRegionStride1());
    G4_INST *movInst = builder->createInternalInst(NULL, G4_mov, NULL, false, NUM_DWORDS_PER_GRF,
        movDst, r0Src, NULL, InstOpt_WriteEnable, 0, lastInst ? lastInst->getCISAOff() : -1, 0);
    if (lastInst)
    {
        movInst->setLocation(lastInst->getLocation());
    }
    instList.push_back(movInst);

    auto EOT_SFID = builder->getEOTSFID();

    int exdesc = (0x1 << 5) + SFIDtoInt(EOT_SFID);
    // response len = 0, msg len = 1
    int desc = (0x1 << 25) + (0x1 << 4);

    G4_SrcRegRegion* sendSrc = builder->Create_Src_Opnd_From_Dcl(
        dcl, builder->getRegionStride1());

    G4_DstRegRegion *sendDst = builder->createNullDst(Type_UD);

    auto msgDesc = builder->createSendMsgDesc(desc, exdesc, false, true);
    G4_INST* sendInst = builder->createSendInst(
        NULL,
        G4_send,
        8,
        sendDst,
        sendSrc,
        builder->createImm(desc, Type_UD),
        InstOpt_WriteEnable,
        msgDesc,
        0);
    // need to make sure builder list is empty since later phases do a splice on the entire list
    builder->instList.pop_back();
    // createSendInst incorrectly sets its cisa offset to the last value of the counter.
    sendInst->setCISAOff(movInst->getCISAOff());
    sendInst->setLocation(movInst->getLocation());
    instList.push_back(sendInst);

    if (builder->getHasNullReturnSampler())
    {
        addSamplerFlushBeforeEOT();
    }
}

void G4_BB::emitInstructionInfo(std::ostream& output, INST_LIST_ITER &it)
{
    bool emitFile = false, emitLineNo = false;
    const char* curFilename = (*it)->getSrcFilename();
    int curSrcLineNo = (*it)->getLineNo();

    if ((*it)->isLabel())
    {
        return;
    }

    if (curFilename && (prevFilename == nullptr || strcmp(prevFilename, curFilename) != 0))
    {
        emitFile = true;
    }

    if (prevSrcLineNo != curSrcLineNo && curSrcLineNo != 0)
    {
        emitLineNo = true;
    }

    if (emitFile)
    {
        output << "// File: " << curFilename << "\n";
    }

    auto getSrcLine = [](std::string fileName, int srcLine)
    {
        std::ifstream ifs(fileName);
        if (!ifs)
        {
            return std::string("Can't find src file");
        }
        std::string line;
        int i = 0;
        for (; i < srcLine && std::getline(ifs, line); i++)
        {
        }
        return i == srcLine ? line : "Invalid line no";
    };

    if (emitLineNo)
    {
        output << "\n// Line " << curSrcLineNo << ":\t";
        if (curFilename)
        {
            std::string curLine = getSrcLine(std::string(curFilename), curSrcLineNo);
            auto isNotSpace = [](int ch) { return !std::isspace(ch); };
            curLine.erase(curLine.begin(), std::find_if(curLine.begin(), curLine.end(), isNotSpace));
            curLine.erase(std::find_if(curLine.rbegin(), curLine.rend(), isNotSpace).base(), curLine.end());
            output << curLine << "\n";
        }
    }

    if (emitFile)
    {
        prevFilename = curFilename;
    }

    if (emitLineNo)
    {
        prevSrcLineNo = curSrcLineNo;
    }
}

void G4_BB::emitBankConflict(std::ostream& output, G4_INST *inst)
{
    int regNum[2][G4_MAX_SRCS];
    int execSize[G4_MAX_SRCS];
    int regSrcNum = 0;


    if (inst->getNumSrc() == 3 && !inst->isSend())
    {
        for (unsigned i = 0; i < G4_Inst_Table[inst->opcode()].n_srcs; i++)
        {
            G4_Operand * srcOpnd = inst->getSrc(i);
            regNum[1][i] = -1;
            if (srcOpnd)
            {
                if (srcOpnd->isSrcRegRegion() &&
                    srcOpnd->asSrcRegRegion()->getBase() &&
                    srcOpnd->asSrcRegRegion()->getBase()->isRegVar())
                {
                    G4_RegVar* baseVar = static_cast<G4_RegVar*>(srcOpnd->asSrcRegRegion()->getBase());
                    if (baseVar->isGreg()) {
                        uint32_t byteAddress = srcOpnd->getLinearizedStart();
                        if (byteAddress != 0) {
                            regNum[0][i] = byteAddress / GENX_GRF_REG_SIZ;
                        }
                        else {
                            // before RA, use the value in Greg directly
                            regNum[0][i] = baseVar->getPhyReg()->asGreg()->getRegNum();
                        }
                        regNum[1][i] = regNum[0][i];
                        regSrcNum++;
                    }
                    execSize[i] = srcOpnd->getLinearizedEnd() - srcOpnd->getLinearizedStart();
                }
            }
        }
    }


    if (regSrcNum == 3)
    {
        int maxGRFNum = 0;
        output << " {";
        if (parent->builder->oneGRFBankDivision())
        {//EVEN/ODD
            for (int i = 0; i < 3; i++)
            {
                output << i << "=";
                if (!(regNum[0][i] % 2) && regNum[0][i] < SECOND_HALF_BANK_START_GRF)
                {
                    output << "EL, ";
                }
                if (regNum[0][i] % 2 && regNum[0][i] < SECOND_HALF_BANK_START_GRF)
                {
                    output << "OL, ";
                }
                if (!(regNum[0][i] % 2) && regNum[0][i] >= SECOND_HALF_BANK_START_GRF)
                {
                    output << "EH, ";
                }
                if (regNum[0][i] % 2 && regNum[0][i] >= SECOND_HALF_BANK_START_GRF)
                {
                    output << "OH, ";
                }
            }
        }
        else
        { //EVEN EVEN/ODD ODD
            for (int i = 0; i < 3; i++)
            {
                output << i << "=";
                for (int j = 0; j < (execSize[i] + GENX_GRF_REG_SIZ - 1) / GENX_GRF_REG_SIZ; j++)
                {
                    int reg_num = regNum[0][i] + j;
                    if (!(reg_num & 0x02) && reg_num < SECOND_HALF_BANK_START_GRF)
                    {
                        output << "EL, ";
                    }
                    if ((reg_num & 0x02) && reg_num < SECOND_HALF_BANK_START_GRF)
                    {
                        output << "OL, ";
                    }
                    if (!(reg_num & 0x02) && reg_num >= SECOND_HALF_BANK_START_GRF)
                    {
                        output << "EH, ";
                    }
                    if ((reg_num & 0x02) && reg_num >= SECOND_HALF_BANK_START_GRF)
                    {
                        output << "OH, ";
                    }
                    if (j > 1)
                    {
                        regNum[1][i] = reg_num;
                    }
                }
                maxGRFNum = ((execSize[i] + GENX_GRF_REG_SIZ - 1) / GENX_GRF_REG_SIZ) > maxGRFNum ?
                    ((execSize[i] + GENX_GRF_REG_SIZ - 1) / GENX_GRF_REG_SIZ) : maxGRFNum;
            }

#ifdef DEBUG_VERBOSE_ON
            if (((regNum[0][1] & 0x02) == (regNum[0][2] & 0x02)) &&
                ((regNum[0][1] >= SECOND_HALF_BANK_START_GRF && regNum[0][2] < SECOND_HALF_BANK_START_GRF) ||
                (regNum[0][1] < SECOND_HALF_BANK_START_GRF && regNum[0][2] >= SECOND_HALF_BANK_START_GRF)))
            {
                noBankCount++;
            }
#endif
        }
        output << "BC=";
        if (!parent->builder->twoSourcesCollision())
        {
            if (!parent->builder->oneGRFBankDivision())
            { //EVEN EVEN/ODD ODD
                ASSERT_USER(maxGRFNum < 3, "Not supporting register size > 2");
                if (maxGRFNum == 2)
                {
                    for (int i = 0; i < maxGRFNum; i++)
                    {
                        if ((regNum[i][1] & 0x02) == (regNum[i][2] & 0x02))
                        {
                            if ((regNum[i][1] < SECOND_HALF_BANK_START_GRF &&
                                regNum[i][2] < SECOND_HALF_BANK_START_GRF) ||
                                (regNum[i][1] >= SECOND_HALF_BANK_START_GRF &&
                                    regNum[i][2] >= SECOND_HALF_BANK_START_GRF))
                            {
                                parent->BCStats.addBad();
                                output << "BAD,";
                            }
                            else
                            {
                                parent->BCStats.addOK();
                                output << "OK,";
                            }
                        }
                        else
                        {
                            parent->BCStats.addGood();
                            output << "GOOD,";
                        }
                    }
                }
                else
                {
                    for (int i = 0; i < maxGRFNum; i++)
                    {
                        if (((regNum[i][1] & 0x02) == (regNum[i][2] & 0x02)) &&
                            ((regNum[i][0] & 0x02) == (regNum[i][1] & 0x02)))
                        {
                            if ((regNum[i][0] < SECOND_HALF_BANK_START_GRF &&
                                regNum[i][1] < SECOND_HALF_BANK_START_GRF &&
                                regNum[i][2] < SECOND_HALF_BANK_START_GRF) ||
                                (regNum[i][0] >= SECOND_HALF_BANK_START_GRF &&
                                    regNum[i][1] >= SECOND_HALF_BANK_START_GRF &&
                                    regNum[i][2] >= SECOND_HALF_BANK_START_GRF))
                            {
                                parent->BCStats.addBad();
                                output << "BAD,";
                            }
                            else
                            {
                                parent->BCStats.addOK();
                                output << "OK,";
                            }
                        }
                        else
                        {
                            parent->BCStats.addGood();
                            output << "GOOD,";
                        }
                    }
                }
            }
            else
            {  //EVEN/ODD
                if ((regNum[0][1] % 2) != (regNum[0][2] % 2) ||
                    (regNum[0][0] % 2) != (regNum[0][1] % 2) ||
                    (regNum[0][1] == regNum[0][2]))
                {
                    parent->BCStats.addGood();
                    output << "GOOD";
                }
                else
                {
                    if ((regNum[0][0] < SECOND_HALF_BANK_START_GRF &&
                        regNum[0][1] < SECOND_HALF_BANK_START_GRF &&
                        regNum[0][2] < SECOND_HALF_BANK_START_GRF) ||
                        (regNum[0][0] >= SECOND_HALF_BANK_START_GRF &&
                            regNum[0][1] >= SECOND_HALF_BANK_START_GRF &&
                            regNum[0][2] >= SECOND_HALF_BANK_START_GRF))
                    {
                        parent->BCStats.addBad();
                        output << "BAD";
                    }
                    else
                    {
                        parent->BCStats.addOK();
                        output << "OK";
                    }
                }
            }
        }
        else  //Two source
        {  //   EVEN/ODD
            if ((regNum[0][1] != regNum[0][2]) &&
                ((regNum[0][1] % 2) == (regNum[0][2] % 2)))
            {
                if ((regNum[0][1] < SECOND_HALF_BANK_START_GRF &&
                    regNum[0][2] < SECOND_HALF_BANK_START_GRF) ||
                    (regNum[0][1] >= SECOND_HALF_BANK_START_GRF &&
                        regNum[0][2] >= SECOND_HALF_BANK_START_GRF))
                {
                    parent->BCStats.addBad();
                    output << "BAD";
                }
                else
                {
                    parent->BCStats.addOK();
                    output << "OK";
                }
            }
            else
            {
                parent->BCStats.addGood();
                output << "GOOD";
            }
        }
        output << "}";
    }
}



static void emitInstId(std::ostream& output, int srcLine, int vISAId, uint32_t genId, uint64_t pc)
{
    if (srcLine != 0)
    {
        output << "#" << srcLine << ":";
    }
    if (vISAId != -1)
    {
        output << "$" << vISAId << ":";
    }
    if (genId != -1)
    {
        output << "&" << genId;
    }

    if (pc != 0xffffffff)
    {
        output << ":%" << pc;
    }
}

void G4_BB::emitBasicInstructionIga(char* instSyntax, std::ostream& output, INST_LIST_ITER &it, int *suppressRegs, int *lastRegs)
{
    G4_INST* inst = *it;

    output << instSyntax;
    if (!inst->isLabel() && inst->opcode() < G4_NUM_OPCODE)
    {
        output << " //";
        emitInstId(output, inst->getLineNo(), inst->getCISAOff(), inst->getLexicalId(), inst->getGenOffset());

         emitBankConflict(output, inst);
    }


}
void G4_BB::emitBasicInstruction(std::ostream& output, INST_LIST_ITER &it)
{
    if ((*it)->isSend())
    {
        //
        // emit send instruction
        //
        G4_InstSend* SendInst = (*it)->asSendInst();
        SendInst->emit_send(output);

        output << " //";
        emitInstId(output, SendInst->getLineNo(), SendInst->getCISAOff(), SendInst->getLexicalId(), SendInst->getGenOffset());
        SendInst->emit_send_desc(output);
    }
    else
    {
        //
        // emit label and instruction
        //
        G4_INST *inst = *it;
        inst->emit(output, parent->builder->getOption(vISA_SymbolReg));
        if ((*it)->isLabel() == false)
        {
            output << " //";
            emitInstId(output, inst->getLineNo(), inst->getCISAOff(), inst->getLexicalId(), inst->getGenOffset());
            emitBankConflict(output, inst);
        }
    }

}
void G4_BB::emitInstruction(std::ostream& output, INST_LIST_ITER &it)
{
    //prints out instruction line
    emitInstructionInfo(output, it);

    emitBasicInstruction(output, it);

    output << std::endl;
}
void G4_BB::emit(std::ostream& output)
{

    for (INST_LIST_ITER it = instList.begin(); it != instList.end(); ++it)
    {
        emitInstruction(output, it);
    }
}

void G4_BB::resetLocalId()
{
    int i = 0;

    for (INST_LIST_ITER iter = instList.begin(), end = instList.end();
        iter != end;
        ++iter, ++i)
    {
        (*iter)->setLocalId(i);
    }
}

void G4_BB::dump(bool printCFG = false) const
{
    if (printCFG)
    {
        std::cerr << "BB" << getId() << "\n";
        std::cerr << "Pred: ";
        for (auto pred : Preds)
        {
            std::cerr << pred->getId() << " ";
        }
        std::cerr << "\nSucc: ";
        for (auto succ : Succs)
        {
            std::cerr << succ->getId() << " ";
        }
        std::cerr << "\n";
        if (getBBType())
        {
            std::cerr << "BB type: " << getBBType() << "\n";
        }
    }
    for (auto& x : instList)
        x->dump();
    std::cerr << "\n";
}

void G4_BB::dumpDefUse() const
{
    for (auto& x : instList)
    {
        x->dump();
        if (x->def_size() > 0 || x->use_size() > 0)
        {
            x->dumpDefUse();
            std::cerr << "\n\n\n";
        }
    }
}

// all of the operand in this table are srcRegion
void GlobalOpndHashTable::addGlobalOpnd(G4_Operand *opnd)
{
    G4_Declare *topDcl = opnd->getTopDcl();

    if (topDcl != NULL)
    {

        // global operands must have a declare
        auto entry = globalOperands.find(topDcl);
        if (entry != globalOperands.end())
        {
            entry->second->insert((uint16_t)opnd->getLeftBound(), (uint16_t)opnd->getRightBound());
        }
        else
        {
            HashNode* node = new (mem)HashNode(
                (uint16_t)opnd->getLeftBound(),
                (uint16_t)opnd->getRightBound(),
                private_arena_allocator);
            globalOperands[topDcl] = node;
        }
    }
}

// if def overlaps with any operand in this table, it is treated as global
bool GlobalOpndHashTable::isOpndGlobal(G4_Operand *opnd)
{

    G4_Declare* dcl = opnd->getTopDcl();
    if (dcl == NULL)
    {
        return false;
    }
    else if (dcl->getAddressed() == true)
    {
        // Conservatively assume that all address taken
        // virtual registers are global
        return true;
    }
    else
    {
        auto entry = globalOperands.find(dcl);
        if (entry == globalOperands.end())
        {
            return false;
        }
        HashNode* node = entry->second;
        return node->isInNode((uint16_t)opnd->getLeftBound(), (uint16_t)opnd->getRightBound());
    }
}

void GlobalOpndHashTable::dump()
{
    for (auto&& entry : globalOperands)
    {
        G4_Declare* dcl = entry.first;
        dcl->emit(std::cerr, false, false);
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
                    std::cerr << "[" << i << ",";
                    inRange = true;
                }
                else if (!globalElt[i] && inRange)
                {
                    // end of range
                    std::cerr << i - 1 << "], ";
                    inRange = false;
                }
            }
            if (inRange)
            {
                // close last range
                std::cerr << globalElt.size() - 1 << "]";
            }
        }
        std::cerr << "\n";
    }
}

void G4_Kernel::calculateSimdSize()
{
    // Iterate over all instructions in kernel to check
    // whether default execution size of kernel is
    // SIMD8/16. This is required for knowing alignment
    // to use for GRF candidates.

    // only do it once per kernel, as we should not introduce inst with larger simd size than in the input
    if (simdSize != 0)
    {
        return;
    }

    simdSize = 8;

    for (auto bb : fg)
    {
        for (auto inst : *bb)
        {
            // do not consider send since for certain messages we have to set its execution size
            // to 16 even in simd8 shaders
            if (!inst->isLabel() && !inst->isSend())
            {
                uint32_t size = inst->getMaskOffset() + inst->getExecSize();
                if (size > 16)
                {
                    simdSize = 32;
                    return;
                }
                else if (size > 8)
                {
                    simdSize = 16;
                }
            }
        }
    }
}

void G4_Kernel::dump() const
{
    std::cerr << "G4_Kernel: " << this->name << "\n";
    for (auto I = fg.cbegin(), E = fg.cend(); I != E; ++I)
    {
        auto& B = *I;
        B->dump();
    }
}

//
// Perform DFS traversal on the flow graph (do not enter subroutine, but mark subroutine blocks
// so that they will be processed independently later)
//
void FlowGraph::DFSTraverse(G4_BB* startBB, unsigned &preId, unsigned &postId, FuncInfo* fn)
{
    MUST_BE_TRUE(fn != NULL, "Invalid func info");
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
            MUST_BE_TRUE(bb->Succs.front()->getBBType() & G4_BB_INIT_TYPE, ERROR_FLOWGRAPH);
            MUST_BE_TRUE(bb->Succs.size() == 1, ERROR_FLOWGRAPH);

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
                if (!loopBlock->BBBeforeCall()->isInNaturalLoop())
                {
                    loopBlocks.push_front(loopBlock->BBBeforeCall());
                    loopBody.insert(loopBlock->BBBeforeCall());
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

        naturalLoops.insert(pair<Edge, Blocks>(backEdge, loopBody));
    }
}

void FlowGraph::traverseFunc(FuncInfo* func, unsigned int *ptr)
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
    unsigned int visitID = 1;
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
                (*predMapIter).second.insert(func);
            }
        }
    }

    bool changed = false;
    do
    {
        changed = false;

        unsigned int funcTableSize = static_cast<unsigned int> (sortedFuncTable.size());
        unsigned int funcID = funcTableSize - 1;
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

                std::vector<unsigned int> funcVec(funcTableSize);
                for (unsigned int i = 0; i < funcTableSize; i++)
                {
                    funcVec[i] = 0;
                }

                std::set<FuncInfo*>& predSet = (*predMapIter).second;
                for (auto pred : predSet)
                {
                    for (auto predDom : (*domMap.find(pred)).second)
                    {
                        unsigned int domID = (predDom->getScopeID() == UINT_MAX) ? funcTableSize - 1 : predDom->getScopeID() - 1;
                        funcVec[domID]++;
                    }
                }

                unsigned int predSetSize = static_cast<unsigned int> (predSet.size());
                for (unsigned int i = 0; i < funcTableSize; i++)
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
unsigned int FlowGraph::resolveVarScope(G4_Declare* dcl, FuncInfo* func)
{
    unsigned int oldID = dcl->getScopeID();
    unsigned int newID = func->getScopeID();

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
            unsigned int start = (newID > oldID) ? newID : oldID;
            unsigned int end = static_cast<unsigned int> (sortedFuncTable.size());

            for (unsigned int funcID = start; funcID != end; funcID++)
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
                unsigned int scopeID = resolveVarScope(dcl, func);
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
                        unsigned int scopeID = resolveVarScope(dcl, func);
                        dcl->updateScopeID(scopeID);
                    }
                    else if (src->isAddrExp() &&
                        src->asAddrExp()->getRegVar())
                    {
                        G4_Declare* dcl = src->asAddrExp()->getRegVar()->getDeclare()->getRootDeclare();
                        unsigned int scopeID = resolveVarScope(dcl, func);
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
    G4_Predicate_Control pCtrl)
{
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
                    auto pDst = builder->createDstRegRegion(
                        G4_RegAccess::Direct, newDcl->getRegVar(), 0, 0, 1, DstTy);
                    auto pSrc0 = builder->createSrcRegRegion(
                        G4_SrcModifier::Mod_src_undef, G4_RegAccess::Direct,
                        pred->getBase(), 0, 0, builder->getRegionScalar(), SrcTy);

                    auto truncMask = [](uint32_t mask, G4_Type Ty) -> uint64_t
                    {
                        return (Ty == Type_UW) ? uint16_t(mask) : mask;
                    };

                    if (pCtrl == G4_Predicate_Control::PRED_DEFAULT)
                    {
                        // P = P & 1
                        auto pSrc1 = builder->createImm(1, Type_UW);
                        auto pInst = builder->createInternalInst(
                            nullptr, G4_and, nullptr, false, 1, pDst, pSrc0, pSrc1,
                            InstOpt_M0 | InstOpt_WriteEnable);
                        bb->insert(I, pInst);
                    }
                    else if (G4_Predicate::isAnyH(pCtrl))
                    {
                        // P = P & mask
                        uint32_t mask = getFlagMask(pCtrl);
                        auto pSrc1 = builder->createImm(truncMask(mask, DstTy), DstTy);
                        auto pInst = builder->createInternalInst(
                            nullptr, G4_and, nullptr, false, 1, pDst, pSrc0, pSrc1,
                            InstOpt_M0 | InstOpt_WriteEnable);
                        bb->insert(I, pInst);
                    }
                    else
                    {
                        // AllH
                        // P = P | mask
                        uint32_t mask = getFlagMask(pCtrl);
                        auto pSrc1 = builder->createImm(truncMask(mask, DstTy), DstTy);
                        auto pInst = builder->createInternalInst(
                            nullptr, G4_or, nullptr, false, 1, pDst, pSrc0, pSrc1,
                            InstOpt_M0 | InstOpt_WriteEnable);
                        bb->insert(I, pInst);
                    }

                    // Adjust pred control to the new execution size and build the
                    // new predicate.
                    pCtrl = getPredCtrl(predSize, pCtrl);
                    newPred = builder->createPredicate(
                        pred->getState(), newDcl->getRegVar(), 0, pCtrl);
                }
            }

            // (!P) jmpi L
            // becomes:
            // P = P & MASK
            // (!P.anyN) goto (N) L
            inst->setOpcode(G4_goto);
            inst->setExecSize((unsigned char)predSize);
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

FlowGraph::~FlowGraph()
{
    // even though G4_BBs are allocated in a mem pool and freed in one shot,
    // we must call each BB's desstructor explicitly to free up the memory used
    // by the STL objects(list, vector, etc.) in each BB
    for (unsigned i = 0, size = (unsigned)BBAllocList.size(); i < size; i++)
    {
        G4_BB* bb = BBAllocList[i];
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

KernelDebugInfo* G4_Kernel::getKernelDebugInfo()
{
    if (kernelDbgInfo == nullptr)
    {
        kernelDbgInfo = new(fg.mem)KernelDebugInfo();
    }

    return kernelDbgInfo;
}

G4_Kernel::~G4_Kernel()
{
    if (kernelDbgInfo)
    {
        kernelDbgInfo->~KernelDebugInfo();
    }

    if (gtPinInfo)
    {
        gtPinInfo->~gtPinData();
    }

    Declares.clear();
}

//
// rename non-root declares to their root decl name to make
// it easier to read IR dump
//
void G4_Kernel::renameAliasDeclares()
{
#if _DEBUG
    for (auto dcl : Declares)
    {
        if (dcl->getAliasDeclare())
        {
            uint32_t offset = 0;
            G4_Declare* rootDcl = dcl->getRootDeclare(offset);
            std::string newName(rootDcl->getName());
            if (rootDcl->getElemType() != dcl->getElemType())
            {
                newName += "_";
                newName += G4_Type_Table[dcl->getElemType()].str;
            }
            if (offset != 0)
            {
                newName += "_" + to_string(offset);
            }
            dcl->setName(fg.builder->getNameString(fg.mem, 64, newName.c_str()));
        }
    }
#endif
}

void gtPinData::setGTPinInit(void* buffer)
{
    MUST_BE_TRUE(sizeof(gtpin::igc::igc_init_t) <= 200, "Check size of igc_init_t");
    gtpin_init = (gtpin::igc::igc_init_t*)buffer;

    if (gtpin_init->re_ra)
        kernel.getOptions()->setOption(vISA_ReRAPostSchedule, true);
    if (gtpin_init->grf_info)
        kernel.getOptions()->setOption(vISA_GetFreeGRFInfo, true);
}

template<typename T>
void writeBuffer(std::vector<unsigned char>& buffer, unsigned int& bufferSize, const T* t, unsigned int numBytes)
{
    const unsigned char* data = (const unsigned char*)t;
    for (unsigned int i = 0; i != numBytes; i++)
    {
        buffer.push_back(data[i]);
    }
    bufferSize += numBytes;
}

void* gtPinData::getGTPinInfoBuffer(unsigned int &bufferSize)
{
    gtpin::igc::igc_init_t t;
    std::vector<unsigned char> buffer;
    unsigned int numTokens = 0;
    bufferSize = 0;

    memset(&t, 0, sizeof(t));

    t.version = gtpin::igc::GTPIN_IGC_INTERFACE_VERSION;
    if (gtpin_init->grf_info)
    {
        t.grf_info = 1;
        numTokens++;
    }

    if (gtpin_init->re_ra)
        t.re_ra = 1;

    if (gtpin_init->srcline_mapping && kernel.getOptions()->getOption(vISA_GenerateDebugInfo))
        t.srcline_mapping = 1;

    if (gtpin_init->scratch_area_size > 0)
    {
        t.scratch_area_size = gtpin_init->scratch_area_size;
        numTokens++;
    }

    writeBuffer(buffer, bufferSize, &t, sizeof(t));
    writeBuffer(buffer, bufferSize, &numTokens, sizeof(uint32_t));

    if (t.grf_info)
    {
        // create token
        void* rerabuffer = nullptr;
        unsigned int rerasize = 0;

        rerabuffer = getFreeGRFInfo(rerasize);

        gtpin::igc::igc_token_header_t th;
        th.token = gtpin::igc::GTPIN_IGC_TOKEN::GTPIN_IGC_TOKEN_GRF_INFO;
        th.token_size = sizeof(gtpin::igc::igc_token_header_t) + rerasize;

        // write token and data to buffer
        writeBuffer(buffer, bufferSize, &th, sizeof(th));
        writeBuffer(buffer, bufferSize, rerabuffer, rerasize);

        free(rerabuffer);
    }

    if (t.scratch_area_size)
    {
        gtpin::igc::igc_token_scratch_area_info_t scratchSlotData;
        scratchSlotData.scratch_area_size = t.scratch_area_size;
        scratchSlotData.scratch_area_offset = nextScratchFree;

        // gtpin scratch slots are beyond spill memory
        scratchSlotData.token = gtpin::igc::GTPIN_IGC_TOKEN_SCRATCH_AREA_INFO;
        scratchSlotData.token_size = sizeof(scratchSlotData);

        writeBuffer(buffer, bufferSize, &scratchSlotData, sizeof(scratchSlotData));
    }

    void* gtpinBuffer = allocCodeBlock(bufferSize);

    memcpy_s(gtpinBuffer, bufferSize, (const void*)(buffer.data()), bufferSize);

    return gtpinBuffer;
}

void gtPinData::markInsts()
{
    // Take a snapshot of instructions in kernel.
    for (auto bb : kernel.fg)
    {
        for (auto inst : *bb)
        {
            markedInsts.insert(inst);
        }
    }
}

bool isMarked(G4_INST* inst, std::set<G4_INST*>& insts)
{
    if (insts.find(inst) == insts.end())
    {
        return false;
    }
    return true;
}

void gtPinData::removeUnmarkedInsts()
{
    if (!kernel.fg.getIsStackCallFunc() &&
        !kernel.fg.getHasStackCalls())
    {
        // Marked instructions correspond to caller/callee save
        // and FP/SP manipulation instructions.
        return;
    }

    MUST_BE_TRUE(whichRAPass == ReRAPass, "Unexpectedly removing unmarked instructions in first RA pass");
    // Instructions not seen in "marked" snapshot will be removed by this function.
    for (auto bb : kernel.fg)
    {
        for (auto it = bb->begin(), itEnd = bb->end();
            it != itEnd;)
        {
            auto inst = (*it);

            if (markedInsts.find(inst) == markedInsts.end())
            {
                it = bb->erase(it);
                continue;
            }
            it++;
        }
    }
}

unsigned int G4_Kernel::calleeSaveStart()
{
    return getCallerSaveLastGRF() + 1;
}

unsigned int G4_Kernel::getStackCallStartReg()
{
    // Last 3 GRFs to be used as scratch
    unsigned int totalGRFs = getNumRegTotal();
    unsigned int startReg = totalGRFs - getNumScratchRegs();
    return startReg;
}

unsigned int G4_Kernel::getNumCalleeSaveRegs()
{
    unsigned int totalGRFs = getNumRegTotal();
    return totalGRFs - calleeSaveStart() - getNumScratchRegs();
}

void RelocationEntry::doRelocation(const G4_Kernel& kernel, void* binary, uint32_t binarySize)
{
    // FIXME: nothing to do here
    // we have only dynamic relocations now, which cannot be resolved at compilation time
}

void RelocationEntry::dump() const
{
    std::cerr << "Relocation entry: " << getTypeString() << "\n";
    std::cerr << "\t";
    inst->dump();
    switch (relocType)
    {
        case RelocationType::R_NONE:
            std::cerr << "R_NONE: symbol name = " << symName;
            break;
        case RelocationType::R_SYM_ADDR:
            std::cerr << "R_SYM_ADDR: symbol name = " << symName;
            break;
        case RelocationType::R_SYM_ADDR_32:
            std::cerr << "R_SYM_ADDR_32: symbol name = " << symName;
            break;
        case RelocationType::R_SYM_ADDR_32_HI:
            std::cerr << "R_SYM_ADDR_32_HI: symbol name = " << symName;
            break;
    }
    std::cerr << "\n";
}

//
// perform relocation for every entry in the allocation table
//
void G4_Kernel::doRelocation(void* binary, uint32_t binarySize)
{
    for (auto&& entry : relocationTable)
    {
        entry.doRelocation(*this, binary, binarySize);
    }
}

G4_INST* G4_Kernel::getFirstNonLabelInst() const
{
    for (auto I = fg.cbegin(), E = fg.cend(); I != E; ++I)
    {
        auto bb = *I;
        G4_INST* firstInst = bb->getFirstInst();
        if (firstInst)
        {
            return firstInst;
        }
    }
    // empty kernel
    return nullptr;
}

void SCCAnalysis::run()
{
    SCCNodes.resize(cfg.getNumBB());
    for (auto I = cfg.cbegin(), E = cfg.cend(); I != E; ++I)
    {
        auto BB = *I;
        if (!SCCNodes[BB->getId()])
        {
            findSCC(createSCCNode(BB));
        }
    }
}

void SCCAnalysis::findSCC(SCCNode* node)
{
    SCCStack.push(node);
    for (auto succBB : node->bb->Succs)
    {
        if (succBB == node->bb)
        {
            // no self loop
            continue;
        }
        else if (node->bb->isEndWithCall())
        {
            // ignore call edges and replace it with physical succ instead
            succBB = node->bb->getPhysicalSucc();
            if (!succBB)
            {
                continue;
            }
        }
        else if (node->bb->getBBType() & G4_BB_RETURN_TYPE)
        {
            // stop at return BB
            // ToDo: do we generate (P) ret?
            continue;
        }
        SCCNode* succNode = SCCNodes[succBB->getId()];
        if (!succNode)
        {
            succNode = createSCCNode(succBB);
            findSCC(succNode);
            node->lowLink = std::min(node->lowLink, succNode->lowLink);
        }
        else if (succNode->isOnStack)
        {
            node->lowLink = std::min(node->lowLink, succNode->index);
        }
    }

    // root of SCC
    if (node->lowLink == node->index)
    {
        SCC newSCC(node->bb);
        SCCNode* bodyNode = nullptr;
        do
        {
            bodyNode = SCCStack.top();
            SCCStack.pop();
            bodyNode->isOnStack = false;
            newSCC.addBB(bodyNode->bb);
        } while (bodyNode != node);
        SCCs.push_back(newSCC);
    }
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

PostDom::PostDom(G4_Kernel& k) : kernel(k)
{
    auto numBBs = k.fg.size();
    postDoms.resize(numBBs);
    immPostDoms.resize(numBBs);
}

void PostDom::run()
{
    exitBB = nullptr;
    for (auto bb_rit = kernel.fg.rbegin(); bb_rit != kernel.fg.rend(); bb_rit++)
    {
        auto bb = *bb_rit;
        if (bb->size() > 0)
        {
            auto lastInst = bb->back();
            if (lastInst->isEOT())
            {
                exitBB = bb;
                break;
            }
        }
    }

    MUST_BE_TRUE(exitBB != nullptr, "Exit BB not found!");

    postDoms[exitBB->getId()] = { exitBB };
    std::unordered_set<G4_BB*> allBBs;
    for (auto I = kernel.fg.cbegin(), E = kernel.fg.cend(); I != E; ++I)
    {
        auto bb = *I;
        allBBs.insert(bb);
    }

    for (auto I = kernel.fg.cbegin(), E = kernel.fg.cend(); I != E; ++I)
    {
        auto bb = *I;
        if (bb != exitBB)
        {
            postDoms[bb->getId()] = allBBs;
        }
    }

    // Actual post dom computation
    bool change = true;
    while (change)
    {
        change = false;
        for (auto I = kernel.fg.cbegin(), E = kernel.fg.cend(); I != E; ++I)
        {
            auto bb = *I;
            if (bb == exitBB)
                continue;

            std::unordered_set<G4_BB*> tmp = { bb };
            // Compute intersection of pdom of successors
            std::unordered_map<G4_BB*, unsigned int> numInstances;
            for (auto succs : bb->Succs)
            {
                auto& pdomSucc = postDoms[succs->getId()];
                for (auto pdomSuccBB : pdomSucc)
                {
                    auto it = numInstances.find(pdomSuccBB);
                    if (it == numInstances.end())
                        numInstances.insert(std::make_pair(pdomSuccBB, 1));
                    else
                        it->second = it->second + 1;
                }
            }

            // Common BBs appear in numInstances map with second value == bb->Succs count
            for (auto commonBBs : numInstances)
            {
                if (commonBBs.second == bb->Succs.size())
                    tmp.insert(commonBBs.first);
            }

            // Check if postDom set changed for bb in current iter
            if (tmp.size() != postDoms[bb->getId()].size())
            {
                postDoms[bb->getId()] = tmp;
                change = true;
                continue;
            }
            else
            {
                auto& pdomBB = postDoms[bb->getId()];
                for (auto tmpBB : tmp)
                {
                    if (pdomBB.find(tmpBB) == pdomBB.end())
                    {
                        postDoms[bb->getId()] = tmp;
                        change = true;
                        break;
                    }
                    if (change)
                        break;
                }
            }
        }
    }

    updateImmPostDom();
}

std::unordered_set<G4_BB*>& PostDom::getPostDom(G4_BB* bb)
{
    return postDoms[bb->getId()];
}

void PostDom::dumpImmDom()
{
    for (auto I = kernel.fg.cbegin(), E = kernel.fg.cend(); I != E; ++I)
    {
        auto bb = *I;
        printf("BB%d - ", bb->getId());
        auto& pdomBBs = immPostDoms[bb->getId()];
        for (auto pdomBB : pdomBBs)
        {
            printf("BB%d", pdomBB->getId());
            if (pdomBB->getLabel())
            {
                printf(" (%s)", pdomBB->getLabel()->getLabel());
            }
            printf(", ");
        }
        printf("\n");
    }
}

std::vector<G4_BB*>& PostDom::getImmPostDom(G4_BB* bb)
{
    return immPostDoms[bb->getId()];
}

void PostDom::updateImmPostDom()
{
    // Update immPostDom vector with correct ordering
    for (auto I = kernel.fg.cbegin(), E = kernel.fg.cend(); I != E; ++I)
    {
        auto bb = *I;
        {
            auto& postDomBBs = postDoms[bb->getId()];
            auto& immPostDomBB = immPostDoms[bb->getId()];
            immPostDomBB.resize(postDomBBs.size());
            immPostDomBB[0] = bb;

            for (auto pdomBB : postDomBBs)
            {
                if (pdomBB == bb)
                    continue;

                immPostDomBB[postDomBBs.size() - postDoms[pdomBB->getId()].size()] = pdomBB;
            }
        }
    }
}

G4_BB* PostDom::getCommonImmDom(std::unordered_set<G4_BB*>& bbs)
{
    if (bbs.size() == 0)
        return nullptr;

    unsigned int maxId = (*bbs.begin())->getId();

    auto commonImmDoms = getImmPostDom(*bbs.begin());
    for (auto bb : bbs)
    {
        if (bb->getId() > maxId)
            maxId = bb->getId();

        auto& postDomBB = postDoms[bb->getId()];
        for (unsigned int i = 0, size = commonImmDoms.size(); i != size; i++)
        {
            if (commonImmDoms[i])
            {
                if (postDomBB.find(commonImmDoms[i]) == postDomBB.end())
                {
                    commonImmDoms[i] = nullptr;
                }
            }
        }
    }

    // Return first imm dom that is not a BB from bbs set
    for (unsigned int i = 0, size = commonImmDoms.size(); i != size; i++)
    {
        if (commonImmDoms[i] &&
            // Common imm pdom must be lexically last BB
            commonImmDoms[i]->getId() >= maxId &&
            ((commonImmDoms[i]->size() > 1 && commonImmDoms[i]->front()->isLabel()) ||
            (commonImmDoms[i]->size() > 0 && !commonImmDoms[i]->front()->isLabel())))
        {
            return commonImmDoms[i];
        }
    }

    return exitBB;
}
