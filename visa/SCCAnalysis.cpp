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
#include "SCCAnalysis.h"
#include "FlowGraph.h"

#include <algorithm>

using namespace vISA;

void SCCAnalysis::SCCNode::dump(std::ostream &os) const
{
    os << "SCCNode: BB" << bb->getId() << ", (" << index << "," << lowLink << ")\n";
}

bool SCCAnalysis::SCC::isMember(G4_BB* bb) const {
    return std::find(body.begin(), body.end(), bb) != body.end();
}

G4_BB* SCCAnalysis::SCC::getEarliestBB() const
{
    auto result = std::min_element(body.begin(), body.end(),
        [](G4_BB* bb1, G4_BB* bb2) {return bb1->getId() < bb2->getId(); });
    return *result;
}

void SCCAnalysis::SCC::dump(std::ostream &os) const
{
    os << "SCC (root = BB" << root->getId() << ", size = " << getSize() << "):\t";
    for (auto bodyBB : body)
    {
        os << "BB" << bodyBB->getId() << ", ";
    }
    os << "\n";
}

SCCAnalysis::SCCNode* SCCAnalysis::createSCCNode(G4_BB* bb)
{
    assert(SCCNodes[bb->getId()] == nullptr && "SCCNode already exists");
    SCCNode* newNode = new SCCNode(bb, curIndex++);
    SCCNodes[bb->getId()] = newNode;
    return newNode;
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
} // findSCC

void SCCAnalysis::dump(std::ostream &os) const
{
    for (auto node : SCCNodes)
    {
        node->dump(os);
    }
    for (auto SCC : SCCs)
    {
        SCC.dump(os);
    }
}