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
#include "common/LLVMUtils.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/ADT/PriorityQueue.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/PreRAScheduler.hpp"
#include "Compiler/CISACodeGen/RegisterEstimator.hpp"
#include "Compiler/CISACodeGen/LivenessAnalysis.hpp"
#include "common/debug/Debug.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::Debug;

namespace IGC {
    class PreRAScheduler : public FunctionPass {
    public:
        /* Assuming we are scheduling for SIMD16 PS, max GRF space available for transformation is 128 GRF's.
        ** Assuming non-uniform LLVM instructions and leaving some room for the payload and fragmentation,
        ** 75 GRF's are used for this transformation.
        */
        unsigned maxPerBBRegisterPressureThreshold = IGC_GET_FLAG_VALUE(MaxPreRASchedulerRegPressureThreshold);
        unsigned m_pSIMDSize = 16;

        static char ID;

        PreRAScheduler() : FunctionPass(ID), m_pLVA(nullptr), m_pDT(), m_pRPE(nullptr) {
            initializePreRASchedulerPass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function& F) override;

        LivenessAnalysis* m_pLVA;
        DominatorTree* m_pDT;
        RegisterEstimator* m_pRPE;

        virtual void releaseMemory() override {
            Allocator.Reset();
            clearDDG();
        }

    private:
        struct Edge
        {
            Instruction* end;
            unsigned delay;
        };

        struct Node
        {
            Instruction* instruction;
            std::vector<Edge*> successors;
            int numPredecessors;
            unsigned nodeDelay;
            unsigned nodeInstrNum;
            unsigned earliestCycle;
            bool scheduled;
        };

        DenseMap<unsigned, Node*> m_pInstToNodeMap;

        struct OrderByLatency
        {
        public:
            bool operator()(Node* A, Node* B) const
            {
                //if (a.latency > b.latency)   a > b; else if (a is sampler and b is alu)   a > b; 
                //else if (a.numSuccs >  b.numSuccs)   a > b; else if (a.origOrder > b.origOrder)   a > b; else   a <= b;
                if (A->nodeDelay < B->nodeDelay)
                {
                    return true;
                }
                else if (A->nodeDelay == B->nodeDelay)
                {
                    if ((A->instruction->getNumUses() + A->successors.size()) < (B->instruction->getNumUses() + B->successors.size()))
                    {
                        return true;
                    }
                    else if ((A->instruction->getNumUses() + A->successors.size()) == (B->instruction->getNumUses() + B->successors.size()))
                    {
                        if (A->nodeInstrNum > B->nodeInstrNum)
                        {
                            return true;
                        }
                    }
                    return false;
                }
                return false;
            }
        };

        llvm::PriorityQueue<Node*, std::vector<Node*>, OrderByLatency> longLatencyDelaySortedReadyQueue;
        DenseMap<int, std::vector<Node*>> longLatencyTextureIdxSortedReadyMap;
        llvm::PriorityQueue<Node*, std::vector<Node*>, OrderByLatency> shortLatencySortedReadyQueue;

        struct OrderByEarliestCycle
        {
        public:
            bool operator()(Node* A, Node* B) const
            {
                if (A->earliestCycle > B->earliestCycle)
                {
                    return true;
                }
                else if (A->earliestCycle == B->earliestCycle)
                {
                    if (A->nodeDelay < B->nodeDelay)
                    {
                        return true;
                    }
                    else if (A->nodeDelay == B->nodeDelay)
                    {
                        if (A->nodeInstrNum > B->nodeInstrNum)
                        {
                            return true;
                        }
                    }
                }

                return false;
            }
        };

        llvm::PriorityQueue<Node*, std::vector<Node*>, OrderByEarliestCycle> readyNodeHoldQueue;

        struct OrderByInstrNumInBB
        {
        public:
            bool operator()(Node* A, Node* B) const
            {
                // no strict weak ordering issue here because no 2 nodes can have the same instruction number
                return (A->nodeInstrNum > B->nodeInstrNum);
            }
        };

        llvm::PriorityQueue<Node*, std::vector<Node*>, OrderByInstrNumInBB> instructionOrderSortedReadyQueue;

        //DenseMap<Value*, unsigned> instructionToNumUses;

        BumpPtrAllocator Allocator;

        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<DominatorTreeWrapperPass>();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<LivenessAnalysis>();
            AU.addRequired<RegisterEstimator>();
        }

        void dumpDDGContents();
        void dumpPriorityQueueContents();

        void clearDDG();

        void addNodesToSortedReadyList(Node* readyNode, uint32_t current_cycle = 0);

        Node* FindReadyListWinnerCandidate(unsigned currentBBPressure, uint32_t& current_cycle, Node* prevScheduledNode);

        unsigned instructionLatency(Instruction* inst) const;

        void handleMemoryReadWriteInstructions(
            Node* currInstNode,
            std::list<Node*>& lastLoadNodes,
            Node*& lastStoreNode);

        void buildBasicBlockDDG(
            BasicBlock* BB,
            std::list<Node*>& lastLoadNodes,
            Node*& lastStoreNode);

        unsigned calculateBasicBlockLiveInPressure(BasicBlock* BB);

        bool ScheduleReadyNodes(
            BasicBlock* BB,
            RegPressureTracker& RPTracker);

        Node* FindFirstUnscheduledNodeInLatencyQueue(Node* prevScheduledNode);
        Node* FindFirstUnscheduledNodeInLatencyQueueFromHoldQueue(Node* prevScheduledNode);

        bool fixExtractValue(Function& F) const;
    };

    char PreRAScheduler::ID = 0;
} // End anonymous namespace;

FunctionPass* createPreRASchedulerPass() {
    return new PreRAScheduler();
}

#define PASS_FLAG     "igc-PreRAScheduler"
#define PASS_DESC     "IGC PreRA Scheduler"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PreRAScheduler, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(LivenessAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(RegisterEstimator)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(PreRAScheduler, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

void PreRAScheduler::dumpDDGContents()
{
    IGC_SET_FLAG_VALUE(PrintToConsole, 1);

    DenseMap<unsigned, Node*>::iterator instToNodeMapItBegin = m_pInstToNodeMap.begin(),
        instToNodeMapItEnd = m_pInstToNodeMap.end();
    for (; instToNodeMapItBegin != instToNodeMapItEnd; instToNodeMapItBegin++)
    {
        ods() << "Instruction:  ";
        ods() << "Number of Predecessors: " << instToNodeMapItBegin->second->numPredecessors << "\t";
        ods() << "Node Delay: " << instToNodeMapItBegin->second->nodeDelay << "\t";
        ods() << "Node Instruction Number: " << instToNodeMapItBegin->second->nodeInstrNum << "\t";
        ods() << "Node Earliest Cycle: " << instToNodeMapItBegin->second->earliestCycle << "\t";
        ods() << "Node Scheduled? : " << instToNodeMapItBegin->second->scheduled << "\t";

        std::vector<Edge*>::iterator edgeItBegin = instToNodeMapItBegin->second->successors.begin(),
            edgeItEnd = instToNodeMapItBegin->second->successors.end();

        for (; edgeItBegin != edgeItEnd; edgeItBegin++)
        {
            ods() << " Successor: end ";
            (*edgeItBegin)->end->dump();
            ods() << "\t";
            ods() << " Successor: delay " << (*edgeItBegin)->delay << "\n";
        }

        ods() << " ================ End of NODE data ======================== \n";
    }

    IGC_SET_FLAG_VALUE(PrintToConsole, 0);
}

void PreRAScheduler::clearDDG()
{
    Allocator.Reset();
    // clear all the queues
    longLatencyDelaySortedReadyQueue.clear();
    shortLatencySortedReadyQueue.clear();
    longLatencyTextureIdxSortedReadyMap.clear();
    readyNodeHoldQueue.clear();
    instructionOrderSortedReadyQueue.clear();

    for (auto nodeBegin = m_pInstToNodeMap.begin(), nodeEnd = m_pInstToNodeMap.end();
        nodeBegin != nodeEnd;
        nodeBegin++)
    {
        delete nodeBegin->second;
    }

    m_pInstToNodeMap.clear();
}

void PreRAScheduler::addNodesToSortedReadyList(Node* readyNode, uint32_t current_cycle)
{
    // first add ready nodes to latencySortedReadyQueue or the readyNodeHoldQueue depending on the earliest_cycle count
    if (readyNode->earliestCycle > current_cycle)
    {
        readyNodeHoldQueue.push(readyNode);
    }
    else
    {
        if (isSampleLoadGather4InfoInstruction(readyNode->instruction))
        {
            // push the node in 2 queues. 
            longLatencyDelaySortedReadyQueue.push(readyNode);

            if (IGC_IS_FLAG_ENABLED(EnablePreRASampleCluster))
            {
                longLatencyTextureIdxSortedReadyMap[
                    findSampleInstructionTextureIdx(readyNode->instruction)].push_back(readyNode);
            }
        }
        else
        {
            shortLatencySortedReadyQueue.push(readyNode);

        }
    }

    // next add readyNode to instructionOrderSortedReadyQueue
    instructionOrderSortedReadyQueue.push(readyNode);

}

PreRAScheduler::Node* PreRAScheduler::FindFirstUnscheduledNodeInLatencyQueue(Node* prevScheduledNode)
{
    Node* longLatencyTopNode = nullptr;
    Node* shortLatencyTopNode = nullptr;
    while (!longLatencyDelaySortedReadyQueue.empty() && ((longLatencyDelaySortedReadyQueue.top()->scheduled) == true))
    {
        longLatencyDelaySortedReadyQueue.pop();
    }

    while (!shortLatencySortedReadyQueue.empty() && ((shortLatencySortedReadyQueue.top()->scheduled) == true))
    {
        shortLatencySortedReadyQueue.pop();
    }

    // map contents
    if (IGC_IS_FLAG_ENABLED(EnablePreRASampleCluster))
    {
        for (DenseMap<int, std::vector<Node*>>::iterator readyMapIt = longLatencyTextureIdxSortedReadyMap.begin();
            readyMapIt != longLatencyTextureIdxSortedReadyMap.end();
            ++readyMapIt)
        {
            for (std::vector<Node*>::iterator readyMapNodeIt = readyMapIt->second.begin();
                readyMapNodeIt != readyMapIt->second.end();)
            {
                if ((*readyMapNodeIt)->scheduled == true)
                {
                    readyMapNodeIt = readyMapIt->second.erase(readyMapNodeIt);
                }
                else
                {
                    ++readyMapNodeIt;
                }
            }
        }

        /* if longLatencyTextureIdxSortedReadyMap is not empty and the prevScheduledNode is also a long latency node,
        ** then find the textureIdx of the prevScheduledNode and longLatencyTextureIdxSortedReadyMap top node.
        ** this will help achieve some clustering effect
        */
        if (!longLatencyTextureIdxSortedReadyMap.empty() &&
            prevScheduledNode &&
            isSampleLoadGather4InfoInstruction(prevScheduledNode->instruction))
        {
            std::vector<Node*>& it = longLatencyTextureIdxSortedReadyMap[
                findSampleInstructionTextureIdx(prevScheduledNode->instruction)];
            if (!it.empty())
            {
                Node* readyNodeInMap = it.front();
                //it.pop_front();
                return readyNodeInMap;
            }
        }
    }

    // now find out which queue we want to schedule from, 
    // if top element in longLatencyDelaySortedReadyQueue has higher nodeDelay than top element in shortLatencySortedReadyQueue, schedule that node
    if (!longLatencyDelaySortedReadyQueue.empty() && !shortLatencySortedReadyQueue.empty())
    {
        longLatencyTopNode = longLatencyDelaySortedReadyQueue.top();
        shortLatencyTopNode = shortLatencySortedReadyQueue.top();
        if (longLatencyTopNode->nodeDelay > shortLatencyTopNode->nodeDelay)
        {
            longLatencyDelaySortedReadyQueue.pop();
            return longLatencyTopNode;
        }
        else
        {
            shortLatencySortedReadyQueue.pop();
            return shortLatencyTopNode;
        }
    }
    else if (!shortLatencySortedReadyQueue.empty())
    {
        shortLatencyTopNode = shortLatencySortedReadyQueue.top();
        shortLatencySortedReadyQueue.pop();
        return shortLatencyTopNode;
    }
    else if (!longLatencyDelaySortedReadyQueue.empty())
    {
        longLatencyTopNode = longLatencyDelaySortedReadyQueue.top();
        longLatencyDelaySortedReadyQueue.pop();
        return longLatencyTopNode;
    }

    return nullptr;
}

PreRAScheduler::Node* PreRAScheduler::FindFirstUnscheduledNodeInLatencyQueueFromHoldQueue(Node* prevScheduledNode)
{
    Node* scheduled = nullptr;
    // First find the topmost non scheduled node from the readyNodeHoldQueue
    while (!readyNodeHoldQueue.empty() && ((readyNodeHoldQueue.top()->scheduled) == true))
    {
        readyNodeHoldQueue.pop();
    }

    if (!readyNodeHoldQueue.empty())
    {
        // next determine the cycle number of the top of the the readyNodeHoldQueue. 
        Node* earliestCycleNode = readyNodeHoldQueue.top();

        // next move all the elements with the same cycle number from readyNodeHoldQueue to latencySortedReadyQueue
        while (!readyNodeHoldQueue.empty() && (readyNodeHoldQueue.top()->earliestCycle == earliestCycleNode->earliestCycle))
        {
            Node* readyNode = readyNodeHoldQueue.top();
            if (isSampleLoadGather4InfoInstruction(readyNode->instruction))
            {
                // push node in 2 queues
                longLatencyDelaySortedReadyQueue.push(readyNode);
                if (IGC_IS_FLAG_ENABLED(EnablePreRASampleCluster))
                {
                    longLatencyTextureIdxSortedReadyMap[
                        findSampleInstructionTextureIdx(readyNode->instruction)].push_back(readyNode);
                }
            }
            else
            {
                shortLatencySortedReadyQueue.push(readyNode);
            }
            readyNodeHoldQueue.pop();
        }

        // now that latencySortedReadyQueue is not empty, get the first element from the latencySortedReadyQueue
        // which has not yet been scheduled
        if ((scheduled = FindFirstUnscheduledNodeInLatencyQueue(prevScheduledNode)) != nullptr)
        {
            return scheduled;
        }
    }

    assert(0 && "We should never reach here");
    return scheduled;
}

PreRAScheduler::Node* PreRAScheduler::FindReadyListWinnerCandidate(unsigned currentBBPressure, uint32_t& current_cycle, Node* prevScheduledNode)
{
    // TODO:: improve the winner candidate choice for the readyList
    // if the pressure is low, schedule for latency
    Node* scheduled = nullptr;
    if (currentBBPressure < maxPerBBRegisterPressureThreshold)
    {
        // if latencySortedReadyQueue is not empty, get the first element from the latency sorted ready queue which has not yet been scheduled
        if ((scheduled = FindFirstUnscheduledNodeInLatencyQueue(prevScheduledNode)) != nullptr)
        {
            return scheduled;
        }
        else if ((scheduled = FindFirstUnscheduledNodeInLatencyQueueFromHoldQueue(prevScheduledNode)) != nullptr)
        {
            // Else, latencySortedReadyQueue is empty, we need to populate elements from readyNodeHoldQueue to latencySortedReadyQueue
            return scheduled;
        }

        assert(0 && "We should never reach here");
        return scheduled;
    }
    else
    {
        // register pressure is too high, schedule with instruction order
        // TODO:: This is not currently optimal use a better heuristics

        // we removed scheduled nodes from top of instructionOrderSortedReadyQueue after every schedule of a node.
        // so no need to check for top node of instructionOrderSortedReadyQueue being scheduled
        if (!instructionOrderSortedReadyQueue.empty())
        {
            scheduled = instructionOrderSortedReadyQueue.top();
            instructionOrderSortedReadyQueue.pop();
            return scheduled;
        }

        assert(0 && "We should never reach here");
        return scheduled;
    }
}

void PreRAScheduler::handleMemoryReadWriteInstructions(
    Node* currInstNode,
    std::list<Node*>& lastLoadNodes,
    Node*& lastStoreNode)
{
    // Edge information only added for instructions associated with each other due to alias-ing. 
    // No edge information added for other instructions connected by use-def chain
    if (currInstNode->instruction->mayReadOrWriteMemory())
    {
        if (currInstNode->instruction->mayWriteToMemory())
        {
            if (lastStoreNode)
            {
                // add an edge from the currentInstruction to the last lastStoreNode
                struct Edge* instEdge = new (Allocator)Edge();
                instEdge->end = lastStoreNode->instruction;

                // latency in number of instructions. We are creating an artificial dependence between the memory read/write instructions
                instEdge->delay = 0;

                lastStoreNode->numPredecessors++;

                // now add the edge information between the lastStoreNode and currInstNode
                currInstNode->successors.push_back(instEdge);
            }

            lastStoreNode = currInstNode;

            for (auto lastLoadNodeIt : lastLoadNodes)
            {
                // add an edge from the currInstNode to the last lastLoadNodeIt
                struct Edge* instEdge = new (Allocator)Edge();
                instEdge->end = lastLoadNodeIt->instruction;

                // latency in number of instructions. We are creating an artificial dependence between the memory read/write instructions
                instEdge->delay = 0;

                lastLoadNodeIt->numPredecessors++;

                // now add the edge information between the lastStoreNode and currInstNode
                currInstNode->successors.push_back(instEdge);
            }

            lastLoadNodes.clear();
        }

        if (currInstNode->instruction->mayReadFromMemory() && !currInstNode->instruction->mayWriteToMemory())
        {
            lastLoadNodes.push_back(currInstNode);
            if (lastStoreNode)
            {
                // add an edge from the currentInstruction to the last lastStoreNode
                struct Edge* instEdge = new (Allocator)Edge();
                instEdge->end = lastStoreNode->instruction;

                // latency in number of instructions. We are creating an artificial dependence between the memory read/write instructions
                instEdge->delay = 0;

                lastStoreNode->numPredecessors++;

                // now add the edge information between the lastStoreNode and currInstNode
                currInstNode->successors.push_back(instEdge);
            }
        }
    }
}

/*
** Returns the latency associated with this instruction in number of instructions, assuming 7 threads per EU
** Sample instruction has a latency of 200 cycles and minimum number of instructions need to hide this latency is about
** 21 instructions. Currently the instructionLatency method only returns latency for Sample/Gather4/Lod instructions
*/
unsigned PreRAScheduler::instructionLatency(Instruction* inst) const
{
    // TODO:: add latency information for other instructions
    unsigned latencyInInstructions = 0;
    if (isSampleLoadGather4InfoInstruction(inst))
    {
        latencyInInstructions = 21;
    }
    else
    {
        latencyInInstructions = 1;
    }
    return latencyInInstructions;
}

void PreRAScheduler::buildBasicBlockDDG(
    BasicBlock* BB,
    std::list<Node*>& lastLoadNodes,
    Node*& lastStoreNode)
{
    BasicBlock::iterator I = BB->end();
    --I;
    bool processedBegin = false;

    do
    {
        Instruction* BI = &(*I);
        struct Node* currInstNode = nullptr;
        processedBegin = (I == BB->begin());
        if (!processedBegin)
            --I;

        // skip terminator instructions
        if (BI->isTerminator())
        {
            continue;
        }

        // Skip debugging intrinsic calls.
        if (isa<DbgInfoIntrinsic>(BI))
        {
            continue;
        }

        if (isa<PHINode>(BI))
        {
            continue;
        }

        // first create a node for the current instruction if it does not already exist
        auto currInstFound = m_pInstToNodeMap.find(m_pLVA->ValueIds[BI]);
        if (currInstFound == m_pInstToNodeMap.end())
        {
            currInstNode = new Node();
            currInstNode->instruction = BI;
            currInstNode->numPredecessors = 0;
            currInstNode->nodeDelay = 0; // do not associate latency with the instruction yet. Wait to see the instruction's users
            currInstNode->nodeInstrNum = m_pLVA->ValueIds[BI]; // this is to schedule nodes with instruction order
            currInstNode->earliestCycle = 0;
            currInstNode->scheduled = false;

            m_pInstToNodeMap.insert(std::make_pair(m_pLVA->ValueIds[BI], currInstNode));
        }
        else
        {
            currInstNode = currInstFound->second;
        }

        if (!BI->use_empty())
        {
            //Add an edge from dest to all its users
            for (auto UI = BI->user_begin(), UE = BI->user_end(); UI != UE; ++UI)
            {
                Instruction* useInst = dyn_cast<Instruction>(*UI);

                if (isa<PHINode>(useInst) || useInst->isTerminator())
                {
                    continue; //ignore PHI and terminator instruction uses
                }

                // ignore users from other BB's as we are doing local list scheduling
                if (useInst->getParent() == BB)
                {
                    // Edge information only added for instructions associated with each other due to alias-ing. 
                 // No edge information added for other instructions connected by use-def chain
                 // first create a node for the user instruction if it does not already exist
                    struct Node* useInstNode = nullptr;
                    auto useInstFound = m_pInstToNodeMap.find(m_pLVA->ValueIds[useInst]);
                    if (useInstFound == m_pInstToNodeMap.end())
                    {
                        // successors or uses are processed before the def. Hence assert if a node is not found
                        assert(0 && "user node not found");
                    }
                    else
                    {
                        useInstNode = useInstFound->second;
                    }

                    // increment the number of predecessor instructions for the user instruction
                    useInstNode->numPredecessors++;

                    // update the latency associated with the currentInstruction
                    // go through all the successor nodes and update the final latency associated with currInstruction
                    currInstNode->nodeDelay = std::max(currInstNode->nodeDelay, instructionLatency(BI) + useInstNode->nodeDelay);
                }
            }
        }
        handleMemoryReadWriteInstructions(currInstNode, lastLoadNodes, lastStoreNode);
    } while (!processedBegin);

    //dumpDDGContents();
}

bool PreRAScheduler::ScheduleReadyNodes(
    BasicBlock* BB,
    RegPressureTracker& RPTracker)
{
    bool Changed = false;
    /* Location where the instruction scheduling will begin.
    ** This is either after the last PHI instruction in the block or before the first
    ** instruction of the BB.
    */

    //  Special case: if no PHINode it will insert after the first instruction. Note
    //  that the first instruction will also be scheduled later.
    Instruction* positionToInsert = &(*BB->begin());

    // First, skip phi nodes if any; but need to update register pressure.
    for (BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI)
    {
        Instruction* I = &(*BI);
        if (!isa<PHINode>(I))
        {
            break;
        }
        // keep track of the last PHINode
        positionToInsert = I;

        // Keep track register pressure.
        RPTracker.advance(I);
    }

    uint32_t current_cycle = 0;
    Node* prevScheduledNode = nullptr;

    while (!instructionOrderSortedReadyQueue.empty())
    {
        Node* scheduleNode = nullptr;
        // set Changed to true since we change instruction order
        Changed = true;

        /* Use critical path heuristic to schedule a node until pressure for the BB is low
        ** If pressure is low, schedule the sample instructions first since we want to hoist them to the beginning of the block
        ** Else pick instruction with highest latency. If we have contention pick instruction with maximum number of successors
        ** If we are high on pressure schedule in instruction order
        */
        if ((scheduleNode = FindReadyListWinnerCandidate(
            RPTracker.getCurrNumGRF((uint16_t)m_pSIMDSize), current_cycle, prevScheduledNode)) == nullptr)
        {
            // we have scheduled all the nodes or its an error
            assert(0 && "should not reach here");
            break;
        }

        // set the node to be scheduled so we can
        // find it when latencySortedReadyQueue, readyNodeHoldQueue and instructionOrderReadyList are being processed
        scheduleNode->scheduled = true;

        // Update register pressure
        RPTracker.advance(scheduleNode->instruction);

        // now, go through the list of the scheduled node's successors
        std::vector<Edge*>::iterator edgeItBegin = scheduleNode->successors.begin(),
            edgeItEnd = scheduleNode->successors.end();
        for (; edgeItBegin != edgeItEnd; edgeItBegin++)
        {
            // Decrease the number of predecessors not scheduled for the successor nodes.
            auto succNodeIt = m_pInstToNodeMap.find(m_pLVA->ValueIds[(*edgeItBegin)->end]);
            if (succNodeIt != m_pInstToNodeMap.end())
            {
                Node* SuccNode = succNodeIt->second;

                SuccNode->numPredecessors--;
                SuccNode->earliestCycle = std::max(SuccNode->earliestCycle, (*edgeItBegin)->delay + current_cycle);

                if (SuccNode->numPredecessors == 0)
                {
                    // add ready instructions to 2 lists one sorted on latency and the other on instruction order
                    addNodesToSortedReadyList(SuccNode, current_cycle);
                }
            }
        }

        // Edge information only added for instructions associated with each other due to alias-ing. 
        // No edge information added for other instructions connected by use-def chain
        for (auto schNodeUserBegin = scheduleNode->instruction->user_begin(), schNodeUserEnd = scheduleNode->instruction->user_end();
            schNodeUserBegin != schNodeUserEnd;
            ++schNodeUserBegin)
        {
            Instruction* useInst = dyn_cast<Instruction>(*schNodeUserBegin);
            if (isa<PHINode>(useInst) || useInst->isTerminator())
            {
                continue; //ignore PHI and terminator instruction uses
            }

            // ignore users from other BB's as we are doing local list scheduling
            if (useInst->getParent() == BB)
            {
                // Decrease the number of predecessors not scheduled for the successor nodes.
                auto succNodeIt = m_pInstToNodeMap.find(m_pLVA->ValueIds[useInst]);
                if (succNodeIt != m_pInstToNodeMap.end())
                {
                    Node* SuccNode = succNodeIt->second;

                    SuccNode->numPredecessors--;
                    SuccNode->earliestCycle = std::max(SuccNode->earliestCycle, instructionLatency(scheduleNode->instruction) + current_cycle);

                    if (SuccNode->numPredecessors == 0)
                    {
                        // add ready instructions to 2 lists one sorted on latency and the other on instruction order
                        addNodesToSortedReadyList(SuccNode, current_cycle);
                    }
                }
            }
        }

        // Special case in which there is no PHINode and the first
        // instruction is selected first, this check makes sure that
        // this case is handled correctly.
        if (scheduleNode->instruction != positionToInsert)
        {
            scheduleNode->instruction->removeFromParent();
            scheduleNode->instruction->insertAfter(positionToInsert);
        }

        // update insert position
        positionToInsert = scheduleNode->instruction;

        // bump up the current_cycle count
        current_cycle++;

        // we may have scheduled current instruction by moving it from hold_queue to ready_queue, 
        // in that case set current_cycle to the scheduleNode->earliestCycle
        if (current_cycle < scheduleNode->earliestCycle)
        {
            current_cycle = scheduleNode->earliestCycle;
        }

        // move instructions from hold_queue to latency_queue which have earliest_cycle <= current_cycle
        while (!readyNodeHoldQueue.empty() && (readyNodeHoldQueue.top()->earliestCycle <= current_cycle))
        {
            Node* readyNode = readyNodeHoldQueue.top();
            if (isSampleLoadGather4InfoInstruction(readyNode->instruction))
            {
                // push node in 2 queues
                longLatencyDelaySortedReadyQueue.push(readyNode);
                if (IGC_IS_FLAG_ENABLED(EnablePreRASampleCluster))
                {
                    longLatencyTextureIdxSortedReadyMap[
                        findSampleInstructionTextureIdx(readyNode->instruction)].push_back(readyNode);
                }
            }
            else
            {
                shortLatencySortedReadyQueue.push(readyNode);
            }
            readyNodeHoldQueue.pop();
        }

        // get rid of scheduled instructions from top of instructionOrderSortedReadyQueue (helps us know if the queue is empty)
        while (!instructionOrderSortedReadyQueue.empty() && ((instructionOrderSortedReadyQueue.top()->scheduled) == true))
        {
            instructionOrderSortedReadyQueue.pop();
        }
        //dumpPriorityQueueContents();

        // keep track of the previously scheduled node
        prevScheduledNode = scheduleNode;
    }

    return Changed;
}

void PreRAScheduler::dumpPriorityQueueContents()
{
    llvm::PriorityQueue<Node*, std::vector<Node*>, PreRAScheduler::OrderByLatency> longLatencyQueueCopy = longLatencyDelaySortedReadyQueue;
    llvm::PriorityQueue<Node*, std::vector<Node*>, PreRAScheduler::OrderByEarliestCycle> holdQueueCopy = readyNodeHoldQueue;
    llvm::PriorityQueue<Node*, std::vector<Node*>, PreRAScheduler::OrderByLatency> shortLatencyQueueCopy = shortLatencySortedReadyQueue;

    IGC_SET_FLAG_VALUE(PrintToConsole, 1);

    ods() << "Begin of longLatencyDelaySortedReadyQueue contents" << "\n";
    while (!longLatencyQueueCopy.empty())
    {
        longLatencyQueueCopy.top()->instruction->dump();
        ods() << longLatencyQueueCopy.top()->nodeDelay << "\n";
        ods() << longLatencyQueueCopy.top()->earliestCycle << "\n";
        ods() << longLatencyQueueCopy.top()->successors.size() << "\n";
        ods() << longLatencyQueueCopy.top()->nodeInstrNum << "\n";

        longLatencyQueueCopy.pop();
    }
    longLatencyQueueCopy.clear();
    ods() << "End of longLatencyDelaySortedReadyQueue contents" << "\n";

    ods() << "Begin of map contents" << "\n";
    // map contents
    for (DenseMap<int, std::vector<Node*>>::iterator readyMapIt = longLatencyTextureIdxSortedReadyMap.begin();
        readyMapIt != longLatencyTextureIdxSortedReadyMap.end();
        readyMapIt++)
    {
        for (std::vector<Node*>::iterator readyMapNodeIt = readyMapIt->second.begin();
            readyMapNodeIt != readyMapIt->second.end();
            readyMapNodeIt++)
        {
            (*readyMapNodeIt)->instruction->dump();
            ods() << (*readyMapNodeIt)->nodeDelay << "\n";
            ods() << (*readyMapNodeIt)->earliestCycle << "\n";
            ods() << (*readyMapNodeIt)->successors.size() << "\n";
            ods() << (*readyMapNodeIt)->nodeInstrNum << "\n";
        }
    }
    ods() << "End of map contents" << "\n";

    ods() << "Begin of shortLatencySortedReadyQueue contents" << "\n";
    while (!shortLatencyQueueCopy.empty())
    {
        shortLatencyQueueCopy.top()->instruction->dump();
        ods() << shortLatencyQueueCopy.top()->nodeDelay << "\n";
        ods() << shortLatencyQueueCopy.top()->earliestCycle << "\n";
        ods() << shortLatencyQueueCopy.top()->successors.size() << "\n";
        ods() << shortLatencyQueueCopy.top()->nodeInstrNum << "\n";

        shortLatencyQueueCopy.pop();
    }
    shortLatencyQueueCopy.clear();
    ods() << "End of shortLatencySortedReadyQueue contents" << "\n";

    ods() << "Begin of readyNodeHoldQueue contents" << "\n";
    while (!holdQueueCopy.empty())
    {
        holdQueueCopy.top()->instruction->dump();
        ods() << holdQueueCopy.top()->nodeDelay << "\n";
        ods() << holdQueueCopy.top()->earliestCycle << "\n";
        ods() << holdQueueCopy.top()->successors.size() << "\n";
        ods() << holdQueueCopy.top()->nodeInstrNum << "\n";

        holdQueueCopy.pop();
    }
    holdQueueCopy.clear();
    ods() << "End of readyNodeHoldQueue contents" << "\n";

    IGC_SET_FLAG_VALUE(PrintToConsole, 0);
}

bool PreRAScheduler::runOnFunction(Function& F) {
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_pLVA = &getAnalysis<LivenessAnalysis>();
    m_pDT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    m_pRPE = &getAnalysis<RegisterEstimator>();

    // Register pressure tracker for tracking the number of GRFs needed.
    RegPressureTracker RPTracker(m_pRPE);

    bool Changed = false;

    if (!IGC_IS_FLAG_ENABLED(SetMaxPreRASchedulerRegPressureThreshold))
    {
        if (ctx->m_instrTypes.hasDiscard ||
            (ctx->m_instrTypes.numBB == 1 && ctx->m_instrTypes.numSample && ctx->m_instrTypes.numInsts / ctx->m_instrTypes.numSample < 30) ||
            (ctx->m_instrTypes.numBB != 1 && ctx->m_instrTypes.numBB && ctx->m_instrTypes.numInsts / ctx->m_instrTypes.numBB > 100))
        {
            maxPerBBRegisterPressureThreshold = 110;
        }
    }

    // Run one pass through all the instructions in the BB
    for (df_iterator<DomTreeNode*> DI = df_begin(m_pDT->getRootNode()),
        DE = df_end(m_pDT->getRootNode()); DI != DE; ++DI)
    {
        struct Node* lastStoreNode = nullptr; // used to track the last store node to add dependency between the loads and stores
        std::list<Node*> lastLoadNodes; // used to track the list of load nodes before a store node is encountered so they can be aliased
        lastLoadNodes.clear();
        clearDDG();

        BasicBlock* BB = DI->getBlock();

        // Start tracking register pressure for this BB
        RPTracker.init(BB, true);

        buildBasicBlockDDG(BB, lastLoadNodes, lastStoreNode);
        //dumpDDGContents();

        // identify nodes with no predecessors and add them to readyList sorted in ascending order of latency and instruction order
        for (auto instToNodeIt : m_pInstToNodeMap)
        {
            if (instToNodeIt.second->numPredecessors == 0)
            {
                // add ready instructions to 2 lists one sorted on latency and the other on instruction order
                //instToNodeIt.second->instruction->dump();
                addNodesToSortedReadyList(instToNodeIt.second);
            }
        }

        //dumpPriorityQueueContents();
        // Schedule until we process the entire DDG
        Changed |= ScheduleReadyNodes(BB, RPTracker);
    }

    if (Changed)
        Changed |= fixExtractValue(F);

    DumpLLVMIR(ctx, "AfterPreRAScheduler");
    return Changed;
}

bool PreRAScheduler::fixExtractValue(Function& F) const {
    bool Changed = false;
    // Find 'extractvalue' and pull them just after the definition of their
    // aggregation source.
    for (auto& BB : F) {
        for (auto BI = BB.begin(), BE = BB.end(); BI != BE; /*EMPTY*/) {
            auto EVI = dyn_cast<ExtractValueInst>(&*BI++);
            if (!EVI)
                continue;
            Instruction* I = dyn_cast<Instruction>(EVI->getAggregateOperand());
            if (!I || I->getParent() != EVI->getParent())
                continue;
            // Move this 'extractvalue' just after the aggregate value.
            EVI->moveBefore(&*std::next(I->getIterator()));
            Changed = true;
        }
    }
    return Changed;
}
