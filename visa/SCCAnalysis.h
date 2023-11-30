/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef SCC_ANALYSIS
#define SCC_ANALYSIS

#include "FlowGraph.h"

#include <ostream>
#include <stack>
#include <vector>

namespace vISA {
class SCCAnalysis {
  //
  // implements Tarjan's SCC algorithm
  //
  const FlowGraph &cfg;

  // node used during the SCC algorithm
  struct SCCNode {
    G4_BB *bb;
    int index;
    int lowLink;
    bool isOnStack;

    SCCNode(G4_BB *newBB, int curIndex)
        : bb(newBB), index(curIndex), lowLink(curIndex), isOnStack(true) {}
    void dump(std::ostream &os = std::cerr) const;
  };

  int curIndex = 0;

  std::stack<SCCNode *> SCCStack;
  std::vector<SCCNode *>
      SCCNodes; // 1:1 mapping between SCCNode and BB, indexed by BBId

  class SCC {
    G4_BB *root;
    // list of BBs belonging to the SCC (including root as last BB)
    // assumption is SCC is small (10s of BBs) so membership test is cheap
    std::vector<G4_BB *> body;

  public:
    SCC(G4_BB *bb)
        : root(bb) {} // root gets pushed to body just like other BBs in SCC
    void addBB(G4_BB *bb) { body.push_back(bb); }
    std::vector<G4_BB *>::iterator body_begin() { return body.begin(); }
    std::vector<G4_BB *>::iterator body_end() { return body.end(); }
    size_t getSize() const { return body.size(); }
    bool isMember(G4_BB *bb) const;
    // get earliest BB in program order (this is not necessarily the root
    // depending on DFS order) assumption is reassingBBId() is called
    G4_BB *getEarliestBB() const;
    void dump(std::ostream &os = std::cerr) const;
  }; // SCC

  std::vector<SCC> SCCs;

public:
  SCCAnalysis(const FlowGraph &fg) : cfg(fg) {}
  SCCAnalysis(const SCCAnalysis&) = delete;
  SCCAnalysis& operator=(const SCCAnalysis&) = delete;
  ~SCCAnalysis() {
    for (auto node : SCCNodes) {
      delete node;
    }
  }

  void run();
  void findSCC(SCCNode *node);

  SCCNode *createSCCNode(G4_BB *bb);

  std::vector<SCC>::iterator SCC_begin() { return SCCs.begin(); }
  std::vector<SCC>::iterator SCC_end() { return SCCs.end(); }
  size_t getNumSCC() const { return SCCs.size(); }

  void dump(std::ostream &os = std::cerr) const;
}; // class SCCAnalysis
} // namespace vISA
#endif // SCC_ANALYSIS
