/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#ifndef __CM_FC_DEPGRAPH_H__
#define __CM_FC_DEPGRAPH_H__

#include <list>
#include <map>
#include <tuple>
#include <utility>

#include "PatchInfoRecord.h"

namespace cm {
namespace patch {

class DepGraph {
  Collection &C;

  unsigned Policy;

  std::list<DepNode> Nodes;
  std::list<DepEdge> Edges;

  std::map<std::tuple<Binary *, unsigned, bool>, DepNode *> NodeMap;
  std::map<std::pair<DepNode *, DepNode *>, DepEdge *> EdgeMap;

public:
  enum { SWSB_POLICY_0, SWSB_POLICY_1, SWSB_POLICY_2 };
  DepGraph(Collection &_C, unsigned P) : C(_C), Policy(P){};
  void build();
  void resolve();

protected:
  DepNode *getDepNode(Binary *B, unsigned R, bool Barrier);
  DepEdge *getDepEdge(DepNode *From, DepNode *To, bool FromDef);
};

} // End namespace patch
} // End namespace cm

#endif // __CM_FC_DEPGRAPH_H__
