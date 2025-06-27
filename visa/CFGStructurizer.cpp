/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Assertions.h"
#include "BuildIR.h"
#include "FlowGraph.h"

using namespace vISA;
// This is the debugging code.

// Type for ANode (Abstract Node)
typedef enum {
  AN_UNUSED,

  // corresponding to each BB, not necessarily a hammock
  AN_BB,

  // The following are hammock nodes
  AN_HAMMOCK, // temporarily used
  AN_IF_THEN_ENDIF,
  AN_IF_THEN_ELSE_ENDIF,
  AN_DO_WHILE,
  AN_COMPOSITE,
  AN_SEQUENCE
} ANodeType;

// Kind of ANode: indicate the final conversion choice of this ANode.
typedef enum {
  ANKIND_GOTOJOIN, // goto/join
  ANKIND_JMPI,     // scalar jump (uniform branch)
  ANKIND_SCF       // structured CF
} ANodeKind;

namespace {

enum ANodeBBType { ANODEBB_NORMAL, ANODEBB_CALL, ANODEBB_RETURN };

class ControlGraph {
public:
  // forward goto :  goto in beginBB, endBB is target of goto
  // backward goto:  goto in endBB, beginBB is target of goto
  G4_BB *beginBB;
  G4_BB *endBB;
  G4_INST *gotoInst;

  bool isBackward;
  bool isLoopCandidate; // Used if isBackward is true

  ControlGraph(G4_BB *begin, G4_BB *end, G4_INST *inst, bool backward)
      : beginBB(begin), endBB(end), gotoInst(inst), isBackward(backward),
        isLoopCandidate(false) {}
};

// forward declaration
class CFGStructurizer;
class ANode;
class ANodeBB;

typedef std::vector<G4_BB *> BBVector;
typedef std::vector<ANode *> ANVector;
typedef std::list<ANode *> ANList;
typedef std::vector<ControlGraph *> CGVector;
typedef std::list<ControlGraph *> CGList;
typedef std::map<G4_BB *, G4_BB *> BBToBBMap;
typedef std::map<G4_BB *, ANodeBB *> BBToANodeBBMap;
typedef std::map<ANode *, ANode *> ANodeMap;

// ANode base, abstract class
class ANode {
public:
  uint32_t anodeId = 0;
  ANodeType type;
  ANList preds;
  ANList succs;
  ANode *parent;
  ANodeKind kind;

  // Indicate if this node is in ACFG. ACFG changes as BB is
  // processed, and once a group of nodes are condensed and
  // represented by a new, single node, the new node will be
  // in ACFG and all condensed nodes are no longer in ACFG.
  bool inACFG;

  // True if structured CF is allowed for this node (for example,
  // do_while ANode has break inst that we cannot handle for now.
  // Then, set this field to false so we has to use goto/join.)
  bool allowSCF;

  // True if it has break. Note that break isn't represented by
  // a separate ANode type.  So, if AN_BB has this field set to
  // true, its goto is a break. For other ANode, it means it
  // contains some AN_BB that has break.
  bool hasBreak;

  bool visited;

  virtual G4_BB *getBeginBB() const = 0;
  virtual G4_BB *getEndBB() const = 0;
  virtual G4_BB *getExitBB() const = 0;
  virtual void setBeginBB(G4_BB *bb) = 0;
  virtual void setEndBB(G4_BB *bb) = 0;
  virtual void setExitBB(G4_BB *bb) = 0;

  explicit ANode()
      : type(AN_UNUSED), preds(), succs(), parent(nullptr),
        kind(ANKIND_GOTOJOIN), inACFG(true), allowSCF(true), hasBreak(false),
        visited(false) {}
  explicit ANode(ANodeType Ty)
      : type(Ty), preds(), succs(), parent(nullptr), kind(ANKIND_GOTOJOIN),
        inACFG(true), allowSCF(true), hasBreak(false), visited(false) {}
  virtual ~ANode() {}

  ANodeType getType() const { return type; }
  void setType(ANodeType t) { type = t; }
  ANodeKind getKind() const { return kind; }
  void setKind(ANodeKind t) { kind = t; }

  bool isInACFG() const { return inACFG; }
  void setInACFG(bool v) { inACFG = v; }

  // Return true if this node's type is a SCF type.
  bool isSCF() const;
  bool isHammock() const;

  ANode *getCurrentANode() const;
  ANode *getChildANode(ANode *parent);
  bool isMember(ANode *nd) const;
  bool isExitPhySucc() const;
  bool getHasBreak() const { return hasBreak; }
  void setHasBreak(bool v) { hasBreak = v; }
  bool getAllowSCF() const { return allowSCF; }
  void setAllowSCF(bool v) { allowSCF = v; }
  bool isVisited() const { return visited; }
  void setVisited(bool v) { visited = v; }

private:
  // copy-ctor and assignment operator are not allowed
  ANode(const ANode &) = delete;
  ANode &operator=(const ANode &) = delete;
};

// ANodeBB : ANode for a single BB
class ANodeBB : public ANode {
public:
  G4_BB *bb;
  ANodeBBType type;
  // Used during convertPST() to tell if a BB's label has been used as jmpi's
  // target. As we convert PST from the outer to inner. If this is true,
  // endif/join etc (to this label) must need a new BB (new label). For example:
  //       goto label0                    jmpi label0
  //       ...                            ...
  //       (p) goto label0   --->         if (goto)
  //       ...                            ...
  //     label0:                        label_new:
  //                                      endif (join)
  //                                    label0:
  //
  bool isJmpiTarget = false;

  explicit ANodeBB()
      : ANode(AN_BB), bb(nullptr), type(ANODEBB_NORMAL) {}
  explicit ANodeBB(G4_BB *b) : ANode(AN_BB), bb(b), type(ANODEBB_NORMAL) {}
  virtual ~ANodeBB() {}

  void setANodeBBType(ANodeBBType v) { type = v; }

  // ExitBB is the target BB for BB with goto inst or
  // next BB otherwise. If no successor, return nullptr.
  virtual G4_BB *getExitBB() const {
    if (type == ANODEBB_CALL) {
      return bb->getPhysicalSucc();
    }
    return (type == ANODEBB_RETURN || bb->Succs.size() == 0) ? nullptr
                                                             : bb->Succs.back();
  }
  virtual G4_BB *getBeginBB() const { return bb; }
  virtual G4_BB *getEndBB() const { return bb; }

  virtual void setExitBB(G4_BB *b) {
    vISA_ASSERT_UNREACHABLE("setExitBB() not allowed for ANodeBB");
  }
  virtual void setBeginBB(G4_BB *b) {
    vISA_ASSERT_UNREACHABLE("setBeginBB() not allowed for ANodeBB");
  }
  virtual void setEndBB(G4_BB *b) {
    vISA_ASSERT_UNREACHABLE("setEndBB() not allowed for ANodeBB");
  }
};

// Hammock Graph
class ANodeHG : public ANode {
public:
  ANList children; // in order, 1st one is entryNode.

  // This node has BBs from beginBB to endBB, inclusive; and
  // every outgoing edges will go to exitBB. exitBB is not
  // part of this node.
  G4_BB *beginBB;
  G4_BB *endBB;
  G4_BB *exitBB;   // not part of this ANode
  ANode *exitNode; // not used yet

  /* Temporary variables used during ANodeHG constructions */
  bool isLoopCandidate;

  // Index to ANStack (vector) when this node is on ANStack.
  // -1 means it is not on ANStack.
  int32_t ANStackIx;

  explicit ANodeHG(ANodeType t)
      : ANode(t), beginBB(nullptr), endBB(nullptr), exitBB(nullptr),
        exitNode(nullptr), isLoopCandidate(false), ANStackIx(-1) {}
  explicit ANodeHG(ControlGraph *cg, ANodeBB *nodebb);
  virtual ~ANodeHG() {}

  ANode *getEntryNode() const {
    return children.size() > 0 ? children.front() : nullptr;
  }
  ANode *getEndNode() const {
    return children.size() > 0 ? children.back() : nullptr;
  }
  ANode *getExitNode(ANodeBB *exitANBB);
  void setExitNode(ANode *nd) { exitNode = nd; }

  virtual G4_BB *getBeginBB() const { return beginBB; }
  virtual G4_BB *getEndBB() const { return endBB; }
  virtual G4_BB *getExitBB() const { return exitBB; }
  virtual void setBeginBB(G4_BB *b) { beginBB = b; }
  virtual void setEndBB(G4_BB *b) { endBB = b; }
  virtual void setExitBB(G4_BB *b) { exitBB = b; }
};

// CFG Structurizer
//
// [TODO] algo description
//
// For a CFG that has gotos, convert it to structurized control flow
// so that we can use structured control-flow instructions, jmpi,
// and goto/join to replace them.
//
// This algorithm uses the existing order of BBs and will not reorder
// them.
class CFGStructurizer {
private:
  FlowGraph *CFG;
  uint32_t numOfBBs;

  // Used to assign unique id to each ANode
  uint32_t numOfANodes;

  // For new BB, map its ID to its insertion BB,
  // ie, the original BB after which it is inserted.
  BBToBBMap newBBToInsertAfterBB;

  // Given a BB, find its ANodeBB.
  // Access them via setANodeBB()/getANodeBB().
  struct {
    ANodeBB *IDToANodeBB;
    BBToANodeBBMap newBBToANodeBB;
  } anodeBBs;

  // caching the flags
  bool doScalarJmp;
  bool doStructCF;

  // Assume that the entire CFG is the single ANode: Root ANode for
  // short (We don't actually build it, but imagine we have one). A root
  // ANodes are the ANodes whose parent is this Root ANode and who
  // are also hammock. In another word, any BB with goto should belong
  // to one of topANodes(only one of them, not more than 1).
  ANVector topANodes;

  // Used during constructing hammock ANodes.
  ANVector ANStack;

  // BBs is the list of BBs that shows the layout order. Originally,
  // it is the same as the input CFG->BBs. As the algo goes, new BBs
  // can be inserted in the list.
  BB_LIST *BBs;               // ptr to CFG->BBs
  G4_ExecSize kernelExecSize; // default execSize

  void init();
  void fini();
  void deletePST(ANodeHG *node);
  void preProcess();

  // return true if bb is the start of ControlGraph.
  bool getCGBegin(G4_BB *bb, CGList &cgs);
  void insertAtBegin(G4_BB *aBB, G4_INST *anInst);

  // Insert 'cg' in the ordered list 'cgs'
  void cg_oinsert(CGList &cgs, ControlGraph *cg);
  void bb_oinsert(BB_LIST &bblist, G4_BB *bb);
  bool isGotoScalarJmp(G4_INST *gotoInst);
  bool isJoinBB(G4_BB *bb);
  bool canbeJoinBB(G4_BB *bb);
  bool isBBLabelAvailable(G4_BB *bb);
  ANode *ANListGetOther(ANList &anlist, ANode *node);
  void reConnectAllPreds(ANode *from, ANode *to);
  void reConnectAllSuccs(ANode *from, ANode *to);

  // Use ANStack_push/ANStack_pop to push/pop ANStack
  void ANStack_push(ANodeHG *AN) {
    AN->ANStackIx = (uint32_t)ANStack.size();
    ANStack.push_back(AN);
  }

  ANodeHG *ANStack_pop() {
    ANodeHG *AN = (ANodeHG *)ANStack.back();
    ANStack.pop_back();
    AN->ANStackIx = (uint32_t)(-1);
    return AN;
  }

  uint32_t getNextANodeId() { return numOfANodes++; }
  void constructPST(BB_LIST_ITER IB, BB_LIST_ITER IE);
  void reConstructDoWhileforBreak(ANodeHG *whileNode);
  void ANStack_reconstruct(ANodeHG *&node, ANodeBB *ndbb);
  ANodeHG *finalizeHG(ANodeHG *node);
  void condenseNode(ANodeHG *node);

  // Return true if bb0 appears before bb1, false otherwise.
  // (If bb0 is the same as bb1, return false.)
  bool isBefore(G4_BB *bb0, G4_BB *bb1);

  ANodeBB *getANodeBB(G4_BB *b);
  void setANodeBB(ANodeBB *a, G4_BB *b);
  G4_BB *createBBWithLabel();
  G4_BB *getInsertAfterBB(G4_BB *bb);
  void setInsertAfterBB(G4_BB *newbb, G4_BB *insertAfter);
  G4_BB *findInsertAfterBB(G4_BB *bb);
  void adjustBBId(G4_BB *newbb);

  void getNewRange(G4_BB *&end, G4_BB *&exit, G4_BB *end1, G4_BB *exit1);
  void extendNode(ANode *Node, ControlGraph *CG);
  ANodeHG *getEnclosingANodeHG(ANode *AN);
  ANode *mergeWithPred(ANode *AN);
  ANodeBB *addLandingBB(ANodeHG *node, BB_LIST_ITER insertAfterIter,
                        bool updateACFG);
  ANode *addSplitBBAtBegin(G4_BB *splitBB);
  ANode *addSplitBBAtEnd(G4_BB *splitBB);
  bool isANodeOnlyPred(G4_BB *abb, ANode *node);
  void PSTAddANode(ANodeHG *parent, ANode *node, ANode *newNode, bool isAfter);

  bool convertPST(ANode *node, G4_BB *nextJoinBB);
  void setJoinJIP(G4_BB *joinBB, G4_BB *jipBB);
  void convertChildren(ANodeHG *nodehg, G4_BB *nextJoinBB);
  void convertIf(ANodeHG *node, G4_BB *nextJoinBB);
  void convertDoWhile(ANodeHG *node, G4_BB *nextJoinBB);
  void convertGoto(ANodeBB *node, G4_BB *nextJoinBB);
  void generateGotoJoin(G4_BB *gotoBB, G4_BB *jibBB, G4_BB *targetBB);

  // helper
  // return goto inst if any; null otherwise
  G4_INST *getGotoInst(G4_BB *bb);
  G4_INST *getJmpiInst(G4_BB *bb);
  void updateInstDI(G4_INST *I, G4_BB *DIFromBB);

private:
  // Don't implement
  CFGStructurizer(const CFGStructurizer &);
  CFGStructurizer &operator=(const CFGStructurizer &);

public:
  static void ANListReplace(ANList &anlist, ANode *from, ANode *to);
  static void BBListReplace(BB_LIST &bblist, G4_BB *from, G4_BB *to);
  static BB_LIST_ITER findBB(BB_LIST *bblist, G4_BB *bb);
  static ANList::iterator findANode(ANList &anlist, ANode *nd);
  static void ANListErase(ANList &anlist, ANode *toBeErased);
  static void BBListErase(BB_LIST &bblist, G4_BB *toBeErased);

  CFGStructurizer(FlowGraph *cfg) : CFG(cfg) { init(); }

  ~CFGStructurizer() { fini(); }

  void run();

#if 0
        // Forward definition
        class DebugSelector;
        DebugSelector *dbgSel;
#endif

#ifdef _DEBUG
  void doIndent(uint32_t numOfSpaces); // Generate indent
  void dump();                         // Dump the program(kernel/shader)
  void dumpcfg();                      // Dump the CFG only without instructions
  void dump(G4_BB *bb);                // Dump the given bb
  void dump(uint32_t BBId);            // Dump BB with the given BBId
  void dump(char *labelString);        // Dump BB with the given labelString
  void dump(ANode *node, uint32_t indentSpaces); // Dump node recursively
  void dumpacfg();                               // Dump ACFG.
#endif
};
} // namespace

#ifdef _DEBUG

// 0:  default, dumps only CF related instructions (CF instr, label)
// 1:  All instructions
static int dump_level = 0;
static const char *currFileName = nullptr;
static std::ofstream dump_ofs;
static std::ostream *dumpOut = &std::cout; // default

// use this func or debugger to set dump_level
void setDumpLevel(int l) { dump_level = l; }

// Direct output to a given file or std::cout
void resetoutput() { dumpOut = &std::cout; } // restore default
void setoutputfile(const char *filename) {
  if (dump_ofs.is_open())
    dump_ofs.close();

  currFileName = filename;
  dump_ofs.open(filename,
                std::ofstream::out | std::ofstream::ate | std::ofstream::trunc);
  dump_ofs.flush();
  dumpOut = &dump_ofs;
}
void forceFlush() {
  if (dump_ofs.is_open()) {
    dump_ofs.close();
    dump_ofs.open(currFileName, std::ofstream::out | std::ofstream::app);
  }
}

//
// CFG dump routines
//
void dumpbbbase(G4_BB *bb) {
  if (dump_level == 0) {
    for (INST_LIST_ITER it = bb->begin(), ie = bb->end(); it != ie; ++it) {
      G4_INST *inst = *it;
      if (!inst->isLabel() && !inst->isFlowControl()) {
        continue;
      }
      bb->emitInstruction(*dumpOut, it);
    }
  } else {
    bb->emit(*dumpOut);
  }
  dumpOut->flush();
}

void dumpbb(G4_BB *bb) {
  (*dumpOut) << "\n[BB" << bb->getId() << "] ";
  dumpbbbase(bb);
}

void dumpbbid(BB_LIST *bblist, uint32_t BBId) {
  for (BB_LIST_ITER I = bblist->begin(), E = bblist->end(); I != E; ++I) {
    G4_BB *bb = *I;
    if (bb->getId() == BBId) {
      dumpbb(bb);
      break;
    }
  }
}

void dumpbblabel(BB_LIST *bblist, char *labelString) {
  for (BB_LIST_ITER I = bblist->begin(), E = bblist->end(); I != E; ++I) {
    G4_BB *bb = *I;
    G4_Label *label = bb->getLabel();
    if (label && strcmp(label->getLabelName(), labelString) == 0) {
      dumpbb(bb);
      break;
    }
  }
}

void dumpallbbs(BB_LIST *bblist) {
  (*dumpOut) << "Program Dump\n";

  for (BB_LIST_ITER I = bblist->begin(), E = bblist->end(); I != E; ++I) {
    G4_BB *bb = *I;
    dumpbb(bb);
  }
  dumpOut->flush();
}

void dumpbblist(BB_LIST *bblist) {
  (*dumpOut) << "\nCFG dump\n\n";

  for (BB_LIST_ITER I = bblist->begin(), E = bblist->end(); I != E; ++I) {
    G4_BB *bb = *I;
    G4_Label *label = bb->getLabel();
    (*dumpOut) << "  BB(" << bb->getId() << ")";
    if (label) {
      (*dumpOut) << " " << label->getLabelName() << ",";
    }
    (*dumpOut) << "  Preds:";
    for (BB_LIST_ITER I = bb->Preds.begin(), E = bb->Preds.end(); I != E; ++I) {
      G4_BB *pred = *I;
      (*dumpOut) << " " << pred->getId();
    }
    (*dumpOut) << "    Succs:";
    for (BB_LIST_ITER I = bb->Succs.begin(), E = bb->Succs.end(); I != E; ++I) {
      G4_BB *succ = *I;
      (*dumpOut) << " " << succ->getId();
    }
    (*dumpOut) << "\n";
  }
  dumpOut->flush();
}

// CFGStructurizer::dump(), etc cannot be invoked in MSVC's immediate window.
// Use file-scope functions with cfgs prefix as work-around.
void cfgsdump(CFGStructurizer *p) { p->dump(); }

void cfgsdumpcfg(CFGStructurizer *p) { p->dumpcfg(); }

void cfgsdump(CFGStructurizer *p, uint32_t BBId) { p->dump(BBId); }

void cfgsdump(CFGStructurizer *p, G4_BB *bb) { p->dump(bb); }

void cfgsdump(CFGStructurizer *p, char *labelName) { p->dump(labelName); }

void cfgsdump(CFGStructurizer *p, ANode *nd) { p->dump(nd, 0); }

void cfgsdumpacfg(CFGStructurizer *p) { p->dumpacfg(); }

// Debugging functions
const char *getANodeTypeString(ANodeType t) {
  switch (t) {
  case AN_BB:
    return "BB";
  case AN_HAMMOCK:
    return "HAMMOCK";
  case AN_IF_THEN_ENDIF:
    return "IF_THEN_ENDIF";
  case AN_IF_THEN_ELSE_ENDIF:
    return "IF_THEN_ELSE_ENDIF";
  case AN_DO_WHILE:
    return "DO_WHILE";
  case AN_COMPOSITE:
    return "COMPOSITE";
  case AN_SEQUENCE:
    return "SEQUENCE";
  default:
    return " ";
  }
}

const char *getANodeKindString(ANodeKind t) {
  switch (t) {
  case ANKIND_GOTOJOIN:
    return "gotojoin";
  case ANKIND_JMPI:
    return "jmpi";
  case ANKIND_SCF:
    return "structuredCF";
  }
  return " ";
}

void CFGStructurizer::doIndent(uint32_t numOfSpaces) {
  for (uint32_t ix = 0; ix < numOfSpaces; ++ix) {
    (*dumpOut) << " ";
  }
}

void CFGStructurizer::dump(ANode *node, uint32_t level) {
  // Just show the single digit nesting level
  uint32_t showLevel = level % 10;
  uint32_t indentSpaces = level * 4;

  ANodeType Ty = node->getType();
  doIndent(indentSpaces);
  (*dumpOut) << "  " << showLevel << "-node(" << node->anodeId << ") ["
             << getANodeTypeString(Ty) << "]  begin=BB"
             << node->getBeginBB()->getId() << "  end=BB"
             << node->getEndBB()->getId();
  G4_BB *exitbb = node->getExitBB();
  if (exitbb) {
    (*dumpOut) << "  exit=BB" << exitbb->getId();
  } else {
    (*dumpOut) << "  exit=NULL";
  }
  (*dumpOut) << "\n";
  doIndent(indentSpaces);
  (*dumpOut) << "    Attr:";
  if (node->getType() == AN_BB && getGotoInst(node->getEndBB())) {
    (*dumpOut) << " " << getANodeKindString(node->getKind());
  }
  if (node->getHasBreak()) {
    (*dumpOut) << " hasBreak";
  }
  if (node->isSCF()) {
    (*dumpOut) << (node->getAllowSCF() ? " allowSCF" : " notAllowSCF");
  }
  (*dumpOut) << "\n";

  doIndent(indentSpaces);
  (*dumpOut) << "    preds:";
  for (ANList::iterator I = node->preds.begin(), E = node->preds.end(); I != E;
       ++I) {
    ANode *nd = *I;
    (*dumpOut) << " " << nd->anodeId;
  }
  (*dumpOut) << "  succs:";
  for (ANList::iterator I = node->succs.begin(), E = node->succs.end(); I != E;
       ++I) {
    ANode *nd = *I;
    (*dumpOut) << " " << nd->anodeId;
  }
  (*dumpOut) << "\n";
  doIndent(indentSpaces);
  if (node->parent) {
    (*dumpOut) << "    parent: " << node->parent->anodeId << "\n";
  } else {
    (*dumpOut) << "    parent:\n";
  }
  if (node->isHammock()) {
    ANodeHG *ndhg = (ANodeHG *)node;
    doIndent(indentSpaces);
    (*dumpOut) << "    children:\n";
    for (ANList::iterator I = ndhg->children.begin(), E = ndhg->children.end();
         I != E; ++I) {
      ANode *nd = *I;
      dump(nd, level + 1);
    }
  }
  dumpOut->flush();
}

void CFGStructurizer::dump(G4_BB *bb) { dumpbbbase(bb); }

void CFGStructurizer::dump(uint32_t BBId) { dumpbbid(BBs, BBId); }

void CFGStructurizer::dump(char *labelString) { dumpbblabel(BBs, labelString); }

void CFGStructurizer::dump() {
  (*dumpOut) << "Program Dump\n\n";
  dumpallbbs(BBs);
}

void CFGStructurizer::dumpcfg() { dumpbblist(BBs); }

void CFGStructurizer::dumpacfg() {
  (*dumpOut) << "\nACFG dump\n\n";

  BB_LIST_ITER I = BBs->begin();
  BB_LIST_ITER E = BBs->end();
  while (I != E) {
    BB_LIST_ITER nextI = I;
    ++nextI;
    G4_BB *bb = *I;
    ANodeBB *ndbb = getANodeBB(bb);
    if (ndbb) {
      ANode *nd = ndbb->getCurrentANode();
      dump(nd, 0);
      G4_BB *endbb = nd->getEndBB();
      G4_BB *phyNext = endbb->getPhysicalSucc();
      nextI = findBB(BBs, phyNext);
    }
    I = nextI;
  }
  dumpOut->flush();
}

#endif

inline bool ANode::isSCF() const {
  return (type == AN_IF_THEN_ENDIF || type == AN_IF_THEN_ELSE_ENDIF ||
          type == AN_DO_WHILE);
}

inline bool ANode::isHammock() const {
  return (type == AN_IF_THEN_ENDIF || type == AN_IF_THEN_ELSE_ENDIF ||
          type == AN_DO_WHILE || type == AN_COMPOSITE || type == AN_SEQUENCE);
}

inline ANode *ANode::getCurrentANode() const {
  const ANode *nd = this;
  while (nd && !nd->isInACFG()) {
    nd = nd->parent;
  }
  vISA_ASSERT(nd && nd->isInACFG(), "ACFG flag must be set incorrectly");
  return const_cast<ANode *>(nd);
}

inline bool ANode::isMember(ANode *nd) const {
  while (nd && nd != this) {
    nd = nd->parent;
  }
  return nd == this;
}

inline bool ANode::isExitPhySucc() const {
  const G4_BB *endbb = getEndBB();
  const G4_BB *exitbb = getExitBB();
  return exitbb && endbb && endbb->getPhysicalSucc() == exitbb;
}

ANodeHG::ANodeHG(ControlGraph *cg, ANodeBB *nodebb)
    : ANode(AN_HAMMOCK), children() {
  if (nodebb) {
    children.push_back(nodebb);
    nodebb->parent = this;
  }
  exitNode = nullptr;
  isLoopCandidate = false;
  ANStackIx = -1;

  // AN_HAMMOCK is temporary. Its succs/preds won't be availabe until
  // it is converted to a final hammock node, ie, finalized. So, it
  // is actually not in ACFG yet.

  bool isCondGoto = (cg->gotoInst->getPredicate() != NULL);
  setBeginBB(cg->beginBB);

  if (cg->isBackward) {
    // set up loop
    setEndBB(cg->endBB);
    G4_BB *succ0 = cg->endBB->Succs.front();
    G4_BB *bb = isCondGoto ? succ0 : nullptr;
    setExitBB(bb);
  } else {
    setEndBB(cg->beginBB);
    setExitBB(cg->endBB);
  }
}

ANode *ANodeHG::getExitNode(ANodeBB *exitANBB) {
#if 0
    if (exitNode)
    {
        return exitNode;
    }
#endif

  ANode *nd = exitANBB;
  if (nd) {
    nd = nd->getCurrentANode();
  }
  return nd;
}

ANodeHG *getInnerMostWhile(ANode *node) {
  ANode *tmp = node;
  while (tmp && tmp->getType() != AN_DO_WHILE) {
    tmp = tmp->parent;
  }
  return (ANodeHG *)tmp;
}

// If bb is inside this ANode, return the top node (direct child
// node), otherwise, return nullptr.
ANode *ANode::getChildANode(ANode *parent) {
  ANode *nd = this;
  while (nd && nd->parent != parent) {
    nd = nd->parent;
  }
  return nd;
}

//  Preprocess CFG so that the structurizer can be simpler without considering
/// those corner cases. Currently, it does:
//  1) avoid sharing target label among forward gotos and backward gotos
//     For example,
//          goto L
//       L:
//          goto L
//
//     changed to
//           goto L0
//       L0:      (new empty BB)
//       L1:
//           goto L1
//  2) avoid a fall-thru BB of a backward goto BB is the target BB of another
//     backward goto (two back-to-back loops). For example,
//          L0:
//            (p0) goto L0
//          L1 :
//            (p1) goto L1
//    changed to
//          L0:
//            (p0) goto L0
//          L :         (new empty BB)
//          L1 :
//            (p1) goto L1
// 3) make sure the last BB's pred is its physical predecessor.
//    In another word, if there is a case:
//          B0 : goto L1
//          ...
//        L0:
//          ...
//          B1:  goto L0
//        L1:
//           ret
//   changed to
//          B0 : goto L
//          ...
//       L0:
//          ...
//          B1: goto L0
//       L:            (new empty BB)
//       L1:
//          ret
//   This case is to guarantee that the last HG has its valid, non-null exit BB
//   except Basic block ANode. (Without this, the last HG isn't handled
//   completely with the current algo.)
void CFGStructurizer::preProcess() {
  bool CFGChanged = false;
  for (BB_LIST_ITER BI = CFG->begin(), BE = CFG->end(); BI != BE; ++BI) {
    G4_BB *B = *BI;
    if (B->getBBType() & (G4_BB_RETURN_TYPE | G4_BB_INIT_TYPE)) {
      // If B is RETURN_BB, don't insert as all cases should not happen,
      // so is INIT BB.
      continue;
    }

    bool insertEmptyBBBefore = false;
    // Both entry's end (with or without subroutine and subroutine's end.
    G4_BB *phySucc = B->getPhysicalSucc();
    bool isLastBB = ((std::next(BI) == BE) ||
                     (B->Succs.empty() && phySucc &&
                      (phySucc->getBBType() & G4_BB_INIT_TYPE)) ||
                     (B->getBBType() & G4_BB_EXIT_TYPE));
    if (isLastBB) {
      for (auto IT = B->Preds.begin(), IE = B->Preds.end(); IT != IE; IT++) {
        G4_BB *P = *IT;
        if (P == B->getPhysicalPred()) {
          // If pred is B's physicalPred, skip. Even if pred actually
          // gotos/jmpi to B, the goto/jmpi shall be removed later.
          continue;
        }
        G4_INST *gotoInstP = getGotoInst(P);
        G4_INST *jmpiInstP = getJmpiInst(P);
        if (!gotoInstP && !jmpiInstP) {
          continue;
        }
        // case 3
        // P must have a forward-goto/jmpi to B (B is the last BB).
        insertEmptyBBBefore = true;
        break;
      }
    }

    if (!insertEmptyBBBefore && B->Preds.size() >= 2) {
      // case 1 & 2
      // Check if this B is a successor of both backward and forward branches.
      bool isForwardTarget = false;
      bool isBackwardTarget = false;
      bool isFallThruOfBackwardGotoBB = false;
      for (auto IT = B->Preds.begin(), IE = B->Preds.end(); IT != IE; IT++) {
        G4_BB *P = *IT;
        G4_INST *gotoInstP = getGotoInst(P);
        if (!gotoInstP) {
          continue;
        }
        if (P->getId() >= B->getId()) {
          isBackwardTarget = true;
          continue;
        }
        // P is a BB before B
        if (P->getPhysicalSucc() != B) {
          isForwardTarget = true;
        } else if (P->getPhysicalSucc() == B &&
                   gotoInstP->asCFInst()->isBackward()) {
          isFallThruOfBackwardGotoBB = true;
        }
      }

      if ((isBackwardTarget && isForwardTarget)                // case 1
          || (isBackwardTarget && isFallThruOfBackwardGotoBB)) // case 2
      {
        insertEmptyBBBefore = true;
      }
    }

    if (!insertEmptyBBBefore) {
      continue;
    }

    // Now, create an empty BB right before "B", and adjust all forward
    // branching to this new BB and leave all backward branching unchanged.
    G4_BB *newBB = createBBWithLabel();
    G4_Label *newLabel = newBB->getLabel();

    // Adjust BB's pred/succs
    BB_LIST_ITER NextIT;
    BB_LIST_ITER IT = B->Preds.begin();
    BB_LIST_ITER IE = B->Preds.end();
    for (NextIT = IT; IT != IE; IT = NextIT) {
      ++NextIT;
      G4_BB *P = *IT;
      if (P->getId() >= B->getId()) {
        // keep the backward branch unchanged.
        continue;
      }
      G4_INST *gotoInst = getGotoInst(P);
      G4_INST *jmpiInst = getJmpiInst(P);
      if (!gotoInst && !jmpiInst        // not jump to B
          && P->getPhysicalSucc() != B) // not fall-thru to B
      {
        // Possible mixed goto/if-endif. Skip non-goto/non-jmpi edges.
        continue;
      }

      // forward branching/fall-thru "P->B" is changed to "P->newBB"
      BBListReplace(P->Succs, B, newBB);
      newBB->Preds.push_back(P);

      // Change this branching
      if (gotoInst && gotoInst->asCFInst()->getUip() == B->getLabel()) {
        gotoInst->asCFInst()->setUip(newLabel);
      } else if (jmpiInst && jmpiInst->getSrc(0) == B->getLabel()) {
        jmpiInst->setSrc(newLabel, 0);
      }

      // the edge from B's Preds
      B->Preds.erase(IT);
    }
    newBB->Succs.push_back(B);
    B->Preds.push_front(newBB);

    // insert it into BBs
    CFG->insert(BI, newBB);
    G4_BB *phyPred = B->getPhysicalPred();
    vISA_ASSERT(phyPred != nullptr, "B should have physical pred!");
    phyPred->setPhysicalSucc(newBB);
    newBB->setPhysicalPred(phyPred);
    newBB->setPhysicalSucc(B);
    B->setPhysicalPred(newBB);

    CFGChanged = true;
  }

  if (CFGChanged) {
    // New BBs generated, reassign block Ids.
    CFG->reassignBlockIDs();
  }
}

void CFGStructurizer::init() {
#if _DEBUG
  // Default dump level.
  // To change dump level while debugging, using setDumpLevel().
  dump_level = 0;
#endif

  // Assume G4_BB's id is set from 0 to numOfBBs - 1. This id
  // is used to map G4_BB to ANodeBB.
  // (Note that removeUnreachableBlocks() right before invoking this
  //  pass will reassign block IDs.)

  // First, preprocess CFG so that the structurizer does not need to handle
  // corner cases. Currently, it does:
  //    1. no forward gotos and backward gotos share the same label (but allow
  //    that
  //       all forward gotos can share the same label, so can all backward
  //       gotos);
  //    2. No fall-thru BB of a backward goto BB is also the target BB of
  //    another
  //       backward goto.
  preProcess();

  BBs = &(CFG->getBBList());
  numOfBBs = CFG->getNumBB();
  numOfANodes = 0;
  kernelExecSize = CFG->getKernel()->getSimdSize();

  // caching the flags
  doScalarJmp = !CFG->builder->noScalarJmp();
  doStructCF = CFG->builder->getOption(vISA_StructurizerCF);

  if (numOfBBs == 0) {
    anodeBBs.IDToANodeBB = nullptr;
    return;
  }

  anodeBBs.IDToANodeBB = new ANodeBB[numOfBBs];

  // CFG at this moment has subroutine calls explicitly linked (between call and
  // callee and callee's return to call's succ). For structurizing purpose,
  // those edges should not be considered. Here ANodeBB will have those edges
  // ignored.
  for (G4_BB *bb : *CFG) {
    uint32_t id = bb->getId();
    ANodeBB *node = &(anodeBBs.IDToANodeBB[id]);
    node->anodeId = getNextANodeId();
    node->bb = bb;
    bool isCallBB = (bb->isEndWithCall() || bb->isEndWithFCall());
    bool isReturnBB = (bb->getLastOpcode() == G4_return || bb->isEndWithFRet());
    if (isCallBB) {
      G4_BB *returnBB = bb->getPhysicalSucc();
      vISA_ASSERT(returnBB != nullptr, "Call BB must have a return BB.");
      ANode *succNode = &(anodeBBs.IDToANodeBB[returnBB->getId()]);
      node->succs.push_back(succNode);
      succNode->preds.push_back(node);

      node->setANodeBBType(ANODEBB_CALL);
    } else if (isReturnBB) {
      node->setANodeBBType(ANODEBB_RETURN);
    } else {
      for (G4_BB *succ : bb->Succs) {
        ANode *succNode = &(anodeBBs.IDToANodeBB[succ->getId()]);
        node->succs.push_back(succNode);
        succNode->preds.push_back(node);
      }
    }
  }
}

ANodeBB *CFGStructurizer::getANodeBB(G4_BB *bb) {
  uint32_t id = bb->getId();
  if (id < numOfBBs) {
    return &(anodeBBs.IDToANodeBB[id]);
  }

  BBToANodeBBMap::iterator I = anodeBBs.newBBToANodeBB.find(bb);
  vISA_ASSERT(I != anodeBBs.newBBToANodeBB.end(),
               "Corresponding ANodeBB isn't set up yet");
  ANodeBB *ndbb = I->second;
  return ndbb;
}

void CFGStructurizer::setANodeBB(ANodeBB *ndbb, G4_BB *bb) {
  uint32_t id = bb->getId();
  if (id < numOfBBs) {
    vISA_ASSERT(false, "ANodeBB has been set up already");
    return;
  }
  vISA_ASSERT(anodeBBs.newBBToANodeBB.find(bb) ==
                   anodeBBs.newBBToANodeBB.end(),
               "ANodeBB has been in map already");
  anodeBBs.newBBToANodeBB[bb] = ndbb;
}

G4_BB *CFGStructurizer::getInsertAfterBB(G4_BB *bb) {
  vISA_ASSERT(newBBToInsertAfterBB.find(bb) != newBBToInsertAfterBB.end(),
               "The BB isn't a new BB or something else is wrong");
  vISA_ASSERT(bb->getId() >= numOfBBs, "The BB isn't a new BB");
  return newBBToInsertAfterBB[bb];
}

void CFGStructurizer::setInsertAfterBB(G4_BB *newbb, G4_BB *insertAfter) {
  vISA_ASSERT(newbb->getId() >= numOfBBs, "The BB isn't a new BB");

  if (insertAfter->getId() < numOfBBs) {
    newBBToInsertAfterBB[newbb] = insertAfter;
  } else {
    newBBToInsertAfterBB[newbb] = getInsertAfterBB(insertAfter);
  }
}

void CFGStructurizer::deletePST(ANodeHG *nodehg) {
  for (ANList::iterator I = nodehg->children.begin(),
                        E = nodehg->children.end();
       I != E; ++I) {
    ANode *tmp = *I;
    if (tmp->getType() != AN_BB) {
      deletePST((ANodeHG *)tmp);
    }
  }
  nodehg->children.clear();
  delete nodehg;
}

// Return goto inst if bb has one, null otherwise.
G4_INST *CFGStructurizer::getGotoInst(G4_BB *bb) {
  G4_INST *inst = bb->size() > 0 ? bb->back() : nullptr;
  return (inst && inst->opcode() == G4_goto) ? inst : nullptr;
}

// Return either jmpi inst if bb has one, null otherwise.
G4_INST *CFGStructurizer::getJmpiInst(G4_BB *bb) {
  G4_INST *inst = bb->size() > 0 ? bb->back() : nullptr;
  return (inst && inst->opcode() == G4_jmpi) ? inst : nullptr;
}

void CFGStructurizer::fini() {
  for (uint32_t i = 0, size = topANodes.size(); i < size; ++i) {
    deletePST((ANodeHG *)topANodes[i]);
  }
  topANodes.clear();

  if (numOfBBs > 0) {
    delete[] anodeBBs.IDToANodeBB;
    anodeBBs.IDToANodeBB = nullptr;

    for (BBToANodeBBMap::iterator I = anodeBBs.newBBToANodeBB.begin(),
                                  E = anodeBBs.newBBToANodeBB.end();
         I != E; ++I) {
      ANodeBB *tmp = I->second;
      delete tmp;
    }
    anodeBBs.newBBToANodeBB.clear();

    newBBToInsertAfterBB.clear();
  }
}

ANode *CFGStructurizer::ANListGetOther(ANList &anlist, ANode *node) {
  if (anlist.size() == 0) {
    return nullptr;
  }
  ANode *nd = anlist.back();
  if (nd == node) {
    nd = anlist.front();
  }
  return (nd == node) ? nullptr : nd;
}

// Insert cg into the ordered CGList in the increasing order
// of cg's endBB.
void CFGStructurizer::cg_oinsert(CGList &cgs, ControlGraph *cg) {
  bool hasInserted = false;
  CGList::iterator I = cgs.begin();
  CGList::iterator E = cgs.end();
  G4_BB *end = cg->endBB;
  while (I != E) {
    CGList::iterator Iter = I;
    ++I;
    ControlGraph *tmp = *Iter;
    if (isBefore(tmp->endBB, end)) {
      continue;
    }
    cgs.insert(Iter, cg);
    hasInserted = true;
    break;
  }
  if (!hasInserted) {
    cgs.push_back(cg);
  }
}

// Insert BB into the ordered BB_List in the increasing order of BB
void CFGStructurizer::bb_oinsert(BB_LIST &bblist, G4_BB *bb) {
  bool hasInserted = false;
  BB_LIST_ITER I = bblist.begin();
  BB_LIST_ITER E = bblist.end();
  while (I != E) {
    BB_LIST_ITER Iter = I;
    ++I;
    G4_BB *tmp = *Iter;
    if (isBefore(tmp, bb)) {
      continue;
    }
    bblist.insert(Iter, bb);
    hasInserted = true;
    break;
  }
  if (!hasInserted) {
    bblist.push_back(bb);
  }
}

// Created all CGs whose beginBB is bb. If any, return true and
// all CGs are returned as cgs. If not, return false.
//
// cgs is in order so that the last one will be outmost block
// and the first one is the innermost block. If bb has goto
// inst, the CG for this goto will be the first one in cgs.
bool CFGStructurizer::getCGBegin(G4_BB *bb, CGList &cgs) {
  if (bb->size() == 0) {
    return false;
  }

  // If bb's goto is backward to a BB prior to bb, this bb cannot
  // be a loop candidate.  For example,
  //
  //       B0:
  //          ......
  //       bb: ..
  //         (p) goto B0
  //       ...
  //       B1:
  //         (p) goto bb
  //  In this case, CG for bb is marked with non loop candidate.
  //
  G4_INST *bbgotoInst = getGotoInst(bb);
  bool isLoopCandidate = true;
  if (bbgotoInst && bbgotoInst->asCFInst()->isBackward() &&
      bb->Succs.size() > 0 && bb->Succs.back() != bb) {
    isLoopCandidate = false;
  }

  ControlGraph *cg;
  bool isBegin = false;
  // Check if bb is a loop head
  for (BB_LIST_ITER I = bb->Preds.begin(), E = bb->Preds.end(); I != E; ++I) {
    G4_BB *pred = *I;
    G4_INST *gotoInst = getGotoInst(pred);
    if (gotoInst && gotoInst->asCFInst()->isBackward() &&
        pred->Succs.back() == bb) {
      cg = new ControlGraph(bb, pred, gotoInst, true);
      cg_oinsert(cgs, cg);
      cg->isLoopCandidate = isLoopCandidate;
      isBegin = true;
    }
  }

  // check if its last inst is a forward goto. Note that all backward
  // gotos have been processed when the target BB of this backward goto
  // is processed.
  G4_INST *gotoInst = getGotoInst(bb);
  if (gotoInst && !gotoInst->asCFInst()->isBackward()) {
    G4_BB *succ = bb->Succs.back();
    cg = new ControlGraph(bb, succ, gotoInst, false);
    // Add this cg into the beginning of cgs.
    cgs.push_front(cg);
    isBegin = true;
  }
  return isBegin;
}

inline bool CFGStructurizer::isGotoScalarJmp(G4_INST *gotoInst) {
  vISA_ASSERT(gotoInst->opcode() == G4_goto, "It should be a goto inst");
  return gotoInst->getExecSize() == g4::SIMD1 ||
         gotoInst->getPredicate() == nullptr ||
         gotoInst->asCFInst()->isUniform();
}

inline bool CFGStructurizer::isJoinBB(G4_BB *bb) {
  G4_INST *joinInst = bb->getFirstInst();
  return joinInst && (joinInst->opcode() == G4_join);
}

inline bool CFGStructurizer::canbeJoinBB(G4_BB *bb) {
  // We treat while/else/endif as the first inst
  // of BB. Whenever a BB has any of them, join
  // cannot be inserted as the first inst.
  G4_INST *firstInst = bb->getFirstInst();
  if (!firstInst) {
    // Emptry BB can always have a join
    return true;
  }
  G4_opcode opc = firstInst->opcode();
  return opc != G4_while && opc != G4_endif && opc != G4_else;
}

// return true if bb's label has not been used by while/else/endif/join
bool CFGStructurizer::isBBLabelAvailable(G4_BB *bb) {
  G4_INST *firstInst = bb->getFirstInst();
  if (!firstInst) {
    return true;
  }
  G4_opcode opc = firstInst->opcode();
  return opc != G4_while && opc != G4_endif && opc != G4_else && opc != G4_join;
}

void CFGStructurizer::ANListReplace(ANList &anlist, ANode *from, ANode *to) {
  for (ANList::iterator I = anlist.begin(), E = anlist.end(); I != E; ++I) {
    ANode *tmp = *I;
    if (tmp == from) {
      (*I) = to;
      return;
    }
  }
}

void CFGStructurizer::BBListReplace(BB_LIST &bblist, G4_BB *from, G4_BB *to) {
  for (BB_LIST_ITER I = bblist.begin(), E = bblist.end(); I != E; ++I) {
    G4_BB *tmp = *I;
    if (tmp == from) {
      (*I) = to;
      return;
    }
  }
  vISA_ASSERT_UNREACHABLE("BBList should have to-be-replaced element");
}

void CFGStructurizer::ANListErase(ANList &anlist, ANode *toBeErased) {
  ANList::iterator I = findANode(anlist, toBeErased);
  if (I != anlist.end()) {
    anlist.erase(I);
  }
}

void CFGStructurizer::BBListErase(BB_LIST &bblist, G4_BB *toBeErased) {
  BB_LIST_ITER I = findBB(&bblist, toBeErased);
  if (I != bblist.end()) {
    bblist.erase(I);
  }
}

// Given ranges [end, exit] and [end1, exit1], and
//      end < exit && end1 < exit1
// return the last two BBs of them. Note that the order is the
// BB order as given in CFG's BBs list.
//
// Note that it's possible that exit could be nullptr, but end
// will never be nullptr. And under some cases, return exit is
// unknown (set to nullptr).
void CFGStructurizer::getNewRange(G4_BB *&end, G4_BB *&exit, G4_BB *end1,
                                  G4_BB *exit1) {
  G4_BB *e0, *e1;
  G4_BB *newEnd = isBefore(end, end1) ? end1 : end;

  if (exit && exit1) {
    if (exit == exit1) {
      // exit is still the new exit
      end = newEnd;
      return;
    }

    if (isBefore(exit, exit1)) {
      e0 = exit1;
      e1 = exit;
    } else {
      e0 = exit;
      e1 = exit1;
    }

    if (isBefore(newEnd, e1)) {
      newEnd = e1;
    }
    exit = e0;
    end = newEnd;
    return;
  }

  e0 = exit ? exit : exit1;
  if (e0 && (e0 == newEnd || isBefore(e0, newEnd))) {
    e0 = nullptr;
  }

  exit = e0;
  end = newEnd;
}

// Given a cg, if it definitely does not start a HG, no new HG should
// be created.  Instead, the range covered by this cg will be used to
// extend the current Node's range. This function is used to extend
// Node's range from this cg.
void CFGStructurizer::extendNode(ANode *Node, ControlGraph *CG) {
  if (!Node) {
    return;
  }
  // Extending end & exit
  G4_BB *begin = CG->beginBB;
  G4_BB *end = CG->endBB;
  G4_BB *exit;
  if (CG->isBackward) {
    G4_BB *succ0 = end->Succs.front();
    bool isCondGoto = (CG->gotoInst->getPredicate() != NULL);
    exit = isCondGoto ? succ0 : nullptr;
  } else {
    exit = end;
    end = begin;
  }

  getNewRange(end, exit, Node->getEndBB(), Node->getExitBB());
  Node->setEndBB(end);
  Node->setExitBB(exit);
}

// Get the immediate enclosing ANodeHG that contains all the preds of AN
// and all AN's backward succs (due to backward gotos).
ANodeHG *CFGStructurizer::getEnclosingANodeHG(ANode *AN) {
  vISA_ASSERT(!ANStack.empty(), "ANStack should not be empty");

  G4_BB *begin = AN->getBeginBB();
  int32_t ix = (int32_t)ANStack.size() - 1;
  if (ANStack.size() > 1) {
    // As loop's end and begin must be in the same HG. Here, check
    // if AN has a backward goto, if so, merge ANode of the loop's
    // begin with AN.  However, it is possible that the ANode for
    // the loop's begin is not in ANStack anymore (due to previous
    // merging); and in this case, find ANode that contains loop's
    // begin and merge with AN.
    if (AN->getType() == AN_BB) {
      G4_INST *gotoInst = getGotoInst(begin);
      if (gotoInst && gotoInst->asCFInst()->isBackward()) {
        uint32_t j;
        ANodeHG *hg = nullptr;
        for (j = (uint32_t)ANStack.size(); j != 0; --j) {
          ANodeHG *tmp = (ANodeHG *)ANStack[j - 1];
          if (tmp->isLoopCandidate && tmp->endBB == begin) {
            // ANode for innermost loop
            hg = tmp;
            break;
          }
        }
        if (j == 0) {
          // loop's begin ANode is no longer on ANStack. Find ANStack's node
          // that enclose the loop's begin.
          G4_BB *loopBeginBB = begin->Succs.back();
          if (loopBeginBB != begin) {
            hg = (ANodeHG *)(getANodeBB(loopBeginBB)->getCurrentANode());
            // The parent of ACFG node should be on ANStack
            hg = (ANodeHG *)hg->parent;
          }
        }

        if (hg) {
          vISA_ASSERT(hg->ANStackIx >= 0, "ANodeHG's index is wrong");
          if (ix > hg->ANStackIx) {
            ix = hg->ANStackIx;
          }
        }
      }
    }

    for (ANList::iterator II = AN->preds.begin(), IE = AN->preds.end();
         II != IE; ++II) {
      ANode *pred = *II;
      if (pred == AN || isBefore(begin, pred->getBeginBB())) {
        // Skip. For pred == AN, they are already in AN;
        // for backward goto, its pred will be processed later.
        continue;
      }

      ANodeHG *hg = (ANodeHG *)pred->parent;
      if (hg) {
        vISA_ASSERT(hg->ANStackIx >= 0, "ANodeHG's index is wrong");
        if (ix > hg->ANStackIx) {
          ix = hg->ANStackIx;
        }
      } else {
        // Error message
        G4_BB *predBB = pred->getBeginBB();
        [[maybe_unused]] G4_INST *predGoto = getGotoInst(predBB);

        // If predGoto is null, it means that a non-goto branching inst like
        // jmpi jumps into a range of goto instrutions, such as the following:
        //     jmp  A
        //     goto B
        //  A:
        //     ...
        //  B:
        //
        vISA_ASSERT(predGoto != nullptr,
                     "Error: Non-goto (like jmp) and goto crossing !");
        vISA_ASSERT_UNREACHABLE("Error: unknown control flow in the program.");
      }
    }
    // No need to check succs as forward goto will be handled later
    // and backward goto has been handled already!
  }

  return (ANodeHG *)ANStack[ix];
}

// Merge AN with its immediate pred if possible.
//   1. If pred is AN_SEQUENCE, just add AN into pred
//   2. otherwise, create AN_SEQUENCE and include both
// Return the new merged node if merged, or AN otherwise.
ANode *CFGStructurizer::mergeWithPred(ANode *AN) {
  if (AN->parent == nullptr || AN->succs.size() != 1 || AN->preds.size() != 1) {
    return AN;
  }

  // check if it can be merged with the previous ANode. Note that we
  // only need to check its immediate predecessor under the same parent
  // as it is the only candidate for possible merging.
  ANode *node = AN;
  ANodeHG *parent = (ANodeHG *)AN->parent;
  ANode *pred = AN->preds.back();
  if (pred->succs.size() != 1 || (ANodeHG *)pred->parent != parent) {
    return AN;
  }

  // Get AN's physical previous ANode.
  ANList::iterator E = parent->children.end();
  ANList::iterator I = findANode(parent->children, pred);
  vISA_ASSERT(I != E, "Child Node isn't in Parent's children list");
  ++I;
  ANode *predPhySucc = (I != E) ? *I : nullptr;

  ANode *succ = AN->succs.back();
  if (predPhySucc == AN) {
    // check:  do we allow self-looping on SEQ node ?
    if (pred->getType() == AN_SEQUENCE) {
      ANodeHG *predhg = (ANodeHG *)pred;
      ANode *lastNode = predhg->children.back();
      AN->parent = predhg;
      predhg->children.push_back(AN);
      predhg->succs.clear();
      predhg->succs.push_back(succ);
      ANListReplace(succ->preds, AN, predhg);

      lastNode->succs.clear();
      lastNode->succs.push_back(AN);
      AN->preds.clear();
      AN->preds.push_back(lastNode);

      predhg->setEndBB(AN->getEndBB());
      predhg->setExitBB(AN->getExitBB());
      if (succ != pred) {
        predhg->setExitNode(succ);
      }

      ANListErase(parent->children, AN);
      AN->setInACFG(false);
      node = predhg;
    } else {
      ANodeHG *newSeqNode = new ANodeHG(AN_SEQUENCE);
      newSeqNode->anodeId = getNextANodeId();
      newSeqNode->parent = parent;
      pred->parent = newSeqNode;
      AN->parent = newSeqNode;
      ANList::iterator II = pred->preds.begin();
      ANList::iterator IE = pred->preds.end();
      while (II != IE) {
        ANList::iterator Iter = II;
        ++II;
        ANode *tmp = *Iter;
        if (tmp == AN || AN->isMember(tmp)) {
          newSeqNode->succs.push_back(newSeqNode);
          newSeqNode->preds.push_back(newSeqNode);
        } else {
          ANListReplace(tmp->succs, pred, newSeqNode);
          newSeqNode->preds.push_back(tmp);
        }
      }

      // The above loop has handled case of succ == pred already
      if (succ != pred) {
        ANListReplace(succ->preds, AN, newSeqNode);
        newSeqNode->succs.push_back(succ);
      }
      newSeqNode->children.push_back(pred);
      newSeqNode->children.push_back(AN);
      pred->setInACFG(false);
      AN->setInACFG(false);

      newSeqNode->setBeginBB(pred->getBeginBB());
      newSeqNode->setEndBB(AN->getEndBB());
      newSeqNode->setExitBB(AN->getExitBB());

      if (succ != pred) {
        newSeqNode->setExitNode(succ);
      }

      ANListErase(parent->children, pred);
      ANListErase(parent->children, AN);
      parent->children.push_back(newSeqNode);

      node = newSeqNode;
    }
  }
  return node;
}

// Condensing a HG into a single node in ACFG. The children of node is still
// a valid partial ACFG. The current ACFG is the all ANode whose inACFG is true.
void CFGStructurizer::condenseNode(ANodeHG *node) {
  ANode *entryNode = node->getEntryNode();
  G4_BB *exit = node->getExitBB();
  ANList::iterator I = entryNode->preds.begin();
  ANList::iterator E = entryNode->preds.end();
  while (I != E) {
    ANList::iterator Iter = I;
    ++I;
    ANode *pred = *Iter;
    if (node->isMember(pred)) {
      continue;
    }

    node->preds.push_back(pred);
    ANListReplace(pred->succs, entryNode, node);
  }

  if (exit) {
    ANode *exitNode = node->getExitNode(getANodeBB(exit));
    I = exitNode->preds.begin();
    E = exitNode->preds.end();
    while (I != E) {
      ANList::iterator iter = I;
      ++I;
      ANode *pred = *iter;
      if (node->isMember(pred)) {
        exitNode->preds.erase(iter);
      }
    }
    exitNode->preds.push_back(node);
    node->succs.push_back(exitNode);
  }

  // Set children nodes' inAFCG flag to false.
  E = node->children.end();
  for (I = node->children.begin(); I != E; ++I) {
    ANode *nd = *I;
    nd->setInACFG(false);
  }
}

ANodeHG *CFGStructurizer::finalizeHG(ANodeHG *node) {
  // Identify the type of this node
  ANode *entryNode = node->getEntryNode();
  ANode *endNode = node->getEndNode();
  G4_BB *exitBB = node->getExitBB();
  ANode *exitNode = nullptr;
  if (exitBB) {
    exitNode = node->getExitNode(getANodeBB(exitBB));
    if (exitNode) {
      // Set exitNode when node is finalized
      node->setExitNode(exitNode);
    }
  }
  G4_BB *begin = node->getBeginBB();
  G4_INST *gotoInst = begin->back();
  if (endNode->succs.size() > 0 && endNode->succs.back() == entryNode) {
    node->setType(AN_DO_WHILE);
  } else if (entryNode->getType() == AN_BB && gotoInst &&
             gotoInst->getPredicate() && !gotoInst->asCFInst()->isBackward()) {
    ANodeType ty = AN_COMPOSITE;
    if (entryNode->succs.size() == 2) {
      ANode *thenNode = entryNode->succs.front();
      ANode *elseNode = entryNode->succs.back();
      if (thenNode == elseNode) {
        // The CFG is not well formed and has the following
        //     BB0: succs=BB1, BB1
        // Simply make it COMPOSITE as an work-around. Will
        // fix igc to not generate such a case.
        ty = AN_COMPOSITE;
      } else if (thenNode->succs.size() == 1 &&
                 thenNode->succs.front() == elseNode && elseNode == exitNode) {
        ty = AN_IF_THEN_ENDIF;
      } else if (thenNode->succs.size() == 1 && elseNode->succs.size() == 1 &&
                 thenNode->succs.front() == elseNode->succs.front() &&
                 thenNode->succs.front() == exitNode) {
        ty = AN_IF_THEN_ELSE_ENDIF;
      }
    }
    node->setType(ty);
  } else {
    node->setType(AN_COMPOSITE);
  }

  condenseNode(node);

  if (node->getType() == AN_DO_WHILE) {
    reConstructDoWhileforBreak(node);
  }

  return node;
}

BB_LIST_ITER CFGStructurizer::findBB(BB_LIST *bblist, G4_BB *bb) {
  for (BB_LIST_ITER I = bblist->begin(), E = bblist->end(); I != E; ++I) {
    G4_BB *tmp = *I;
    if (tmp == bb) {
      return I;
    }
  }
  return bblist->end();
}

ANList::iterator CFGStructurizer::findANode(ANList &anlist, ANode *nd) {
  for (ANList::iterator I = anlist.begin(), E = anlist.end(); I != E; ++I) {
    ANode *tmp = *I;
    if (tmp == nd) {
      return I;
    }
  }
  return anlist.end();
}

// Return true if all preds of abb is inside node; false otherwise.
bool CFGStructurizer::isANodeOnlyPred(G4_BB *abb, ANode *node) {
  for (BB_LIST_ITER II = abb->Preds.begin(), IE = abb->Preds.end(); II != IE;
       ++II) {
    G4_BB *tmp = *II;
    ANodeBB *ndbb = getANodeBB(tmp);

    // Note that isMember() requires PST is up to date.
    if (!node->isMember(ndbb)) {
      return false;
    }
  }
  return true;
}

//
//  This function creates a new BB as node's exit BB. Node must be
//  finalized before invoking this function. InsertAfterIter is the
//  iterator to node->getEndBB(), which is provided if the calling
//  context already has it; otherwise, this function will compute
//  this iterator.
//
//  If updateACFG is true, ACFG will be updated. This is used during
//  the construction of ACFG and the new BB will always in ACFG
//  (as ANodeBB).  Note that PST is up to its caller to update.
//
ANodeBB *CFGStructurizer::addLandingBB(ANodeHG *node,
                                       BB_LIST_ITER insertAfterIter,
                                       bool updateACFG) {
  G4_BB *endbb = node->getEndBB();
  if (insertAfterIter == BBs->end()) {
    insertAfterIter = findBB(BBs, endbb);
  }
  G4_BB *insertAfterBB = *insertAfterIter;
  G4_BB *exitBB = node->getExitBB();

  G4_BB *newBB = createBBWithLabel();
  G4_Label *newLabel = newBB->getLabel();
  G4_Label *targetLabel = exitBB->getLabel();
  setInsertAfterBB(newBB, insertAfterBB);

  // Adjust BB's pred/succs
  BB_LIST_ITER BE = exitBB->Preds.end();
  BB_LIST_ITER BI = exitBB->Preds.begin();
  while (BI != BE) {
    BB_LIST_ITER curr = BI;
    ++BI;
    G4_BB *bb = *curr;
    ANode *tmp = getANodeBB(bb);
    if (node->isMember(tmp)) {
      BBListErase(exitBB->Preds, bb);
      BBListReplace(bb->Succs, exitBB, newBB);
      newBB->Preds.push_back(bb);
      G4_INST *gotoInst = getGotoInst(bb);
      if (gotoInst) {
        if (gotoInst->asCFInst()->getUip() == targetLabel) {
          if (bb == insertAfterBB) {
            // remove goto
            bb->pop_back();
          } else {
            // modify the goto to branch to the new target
            gotoInst->asCFInst()->setUip(newLabel);
          }
        }
      } else {
        G4_INST *jmpiInst = getJmpiInst(bb);
        if (jmpiInst) {
          if (jmpiInst->getSrc(0) == targetLabel) {
            if (bb == insertAfterBB) {
              // remove jmpi
              bb->pop_back();
            } else {
              // modify the jmpi to branch to the new target
              jmpiInst->setSrc(newLabel, 0);
            }
          }
        } else if (bb != insertAfterBB) {
          vISA_ASSERT_UNREACHABLE("BB has unknown non-goto/jmpi branch");
        }
      }

      // Update exit nodes
      tmp = tmp->parent;
      while (tmp != node) {
        if (tmp->getExitBB() == exitBB) {
          tmp->setExitBB(newBB);
        }
        tmp = tmp->parent;
      }
    }
  }

  if (insertAfterBB->getPhysicalSucc() != exitBB) {
    G4_INST *gotoInst = CFG->builder->createInternalCFInst(
        NULL, G4_goto, g4::SIMD1, NULL, targetLabel, InstOpt_NoOpt);
    newBB->push_back(gotoInst);

    // Consider newBB is part of exitBB, thus use exitBB's DI
    updateInstDI(gotoInst, exitBB);
  }
  newBB->Succs.push_back(exitBB);
  exitBB->Preds.push_back(newBB);

  // insert it into BBs
  ++insertAfterIter;
  BBs->insert(insertAfterIter, newBB);
  G4_BB *phySucc = insertAfterBB->getPhysicalSucc();
  insertAfterBB->setPhysicalSucc(newBB);
  newBB->setPhysicalPred(insertAfterBB);
  newBB->setPhysicalSucc(phySucc);
  if (phySucc) {
    phySucc->setPhysicalPred(newBB);
  }

  ANodeBB *ndbb = new ANodeBB(newBB);
  ndbb->anodeId = getNextANodeId();
  setANodeBB(ndbb, newBB);

  if (updateACFG) {
    // Node has been finalized. Its children's ACFG is no longer
    // up to date. But within this node, acfg is still good;
    // only edges outside of this node is no longer valid!
    // We will rely on begin/end/exit when doing conversion.
    ANode *exitNode = getANodeBB(exitBB)->getCurrentANode();
    ANListReplace(exitNode->preds, node, ndbb);
    ANListReplace(node->succs, exitNode, ndbb);
    ndbb->succs.push_back(exitNode);
    ndbb->preds.push_back(node);
  }

  node->setExitBB(newBB);
  adjustBBId(newBB);

  return ndbb;
}

// Add a new empty BB as a predecessor of splitBB.
[[maybe_unused]]
ANode *CFGStructurizer::addSplitBBAtBegin(G4_BB *splitBB) {
  G4_BB *newBB = createBBWithLabel();
  G4_Label *newLabel = newBB->getLabel();
  G4_BB *insertAfter = findInsertAfterBB(splitBB->getPhysicalPred());
  setInsertAfterBB(newBB, insertAfter);

  // Adjust BB's pred/succs
  newBB->Preds.splice(newBB->Preds.end(), splitBB->Preds);
  splitBB->Preds.clear();
  splitBB->Preds.push_back(newBB);
  newBB->Succs.push_back(splitBB);
  BB_LIST_ITER BE = newBB->Preds.end();
  BB_LIST_ITER BI = newBB->Preds.begin();
  while (BI != BE) {
    BB_LIST_ITER curr = BI;
    ++BI;
    G4_BB *pred = *curr;
    BBListReplace(pred->Succs, splitBB, newBB);
    G4_INST *gotoInst = getGotoInst(pred);
    if (gotoInst) {
      if (gotoInst->asCFInst()->getUip() == splitBB->getLabel()) {
        // modify the goto to branch to the new target
        gotoInst->asCFInst()->setUip(newLabel);
      }
    }

    // adjust exit BB
    ANode *nd = getANodeBB(pred)->parent;
    while (nd) {
      if (nd->getExitBB() == splitBB) {
        nd->setExitBB(newBB);
      }
      nd = nd->parent;
    }
  }

  // insert it into BBs
  BB_LIST_ITER iter = findBB(BBs, splitBB);
  BBs->insert(iter, newBB);
  G4_BB *phyPred = splitBB->getPhysicalPred();
  phyPred->setPhysicalSucc(newBB);
  newBB->setPhysicalPred(phyPred);
  newBB->setPhysicalSucc(splitBB);
  splitBB->setPhysicalPred(newBB);

  ANodeBB *ndbb = new ANodeBB(newBB);
  ndbb->anodeId = getNextANodeId();
  setANodeBB(ndbb, newBB);

  // newBB's id is the largest at this moment. Make sure it is in order
  adjustBBId(newBB);
  return ndbb;
}

// Add a new BB that only holds the branch instruction of splitBB
// if it has one, otherwise, the new BB is empty.
ANode *CFGStructurizer::addSplitBBAtEnd(G4_BB *splitBB) {
  G4_BB *newBB = createBBWithLabel();
  setInsertAfterBB(newBB, splitBB);

  // Adjust splitBB's pred/succs
  BB_LIST_ITER BE = splitBB->Succs.end();
  BB_LIST_ITER BI = splitBB->Succs.begin();
  for (; BI != BE; ++BI) {
    G4_BB *bb = *BI;
    BBListReplace(bb->Preds, splitBB, newBB);
  }
  newBB->Succs.splice(newBB->Succs.end(), splitBB->Succs);
  newBB->Preds.push_back(splitBB);
  splitBB->Succs.clear();
  splitBB->Succs.push_back(newBB);
  G4_INST *gotoInst = getGotoInst(splitBB);
  if (gotoInst) {
    splitBB->pop_back();
    newBB->push_back(gotoInst);
  }

  // insert it into BBs
  BI = findBB(BBs, splitBB);
  ++BI;
  BBs->insert(BI, newBB);
  G4_BB *phySucc = splitBB->getPhysicalSucc();
  splitBB->setPhysicalSucc(newBB);
  newBB->setPhysicalPred(splitBB);
  newBB->setPhysicalSucc(phySucc);
  if (phySucc) {
    phySucc->setPhysicalPred(newBB);
  }

  ANodeBB *ndbb = new ANodeBB(newBB);
  ndbb->anodeId = getNextANodeId();
  setANodeBB(ndbb, newBB);

  ANode *splitNode = getANodeBB(splitBB);
  ANode *tmp = splitNode->parent;
  while (tmp) {
    if (tmp->getEndBB() == splitBB) {
      tmp->setEndBB(newBB);
    }
    tmp = tmp->parent;
  }

  adjustBBId(newBB);

  return ndbb;
}

// Start from bb (inclusive), search backward to find the first original BB.
G4_BB *CFGStructurizer::findInsertAfterBB(G4_BB *bb) {
  G4_BB *insertAfter = bb;
  while (insertAfter && insertAfter->getId() >= numOfBBs) {
    insertAfter = insertAfter->getPhysicalPred();
  }
  return insertAfter;
}

// Re-connect every predecessors of "from" to its new successor "to". Update
// "to"'s preds and its new predecessors' succs. Note that "from"'s preds are
// left unchanged.
void CFGStructurizer::reConnectAllPreds(ANode *from, ANode *to) {
  for (ANList::iterator I = from->preds.begin(), E = from->preds.end(); I != E;
       ++I) {
    ANode *pred = *I;
    ANListReplace(pred->succs, from, to);
    to->preds.push_back(pred);
  }
}

// Re-connect every successors of "from" to its new predecesor "to". Update
// "to"'s succs and its new successors' preds. Note that "from"'s succs are
// left unchanged.
[[maybe_unused]]
void CFGStructurizer::reConnectAllSuccs(ANode *from, ANode *to) {
  for (ANList::iterator I = from->succs.begin(), E = from->succs.end(); I != E;
       ++I) {
    ANode *succ = *I;
    ANListReplace(succ->preds, from, to);
    to->succs.push_back(succ);
  }
}

// This function is to merge ANStack nodes into the new node during HG
// construction.
void CFGStructurizer::ANStack_reconstruct(ANodeHG *&node, ANodeBB *ndbb) {
  ANodeHG *newNode = getEnclosingANodeHG(ndbb);
  if (newNode->ANStackIx == ANStack.size() - 1) {
    // No need to do ANStack reconstruction
    return;
  }

  // Merge all nodes in ANStack[newNode->ANStackIx+1 : ANStack.size() - 1] into
  // newNode
  G4_BB *exit = newNode->getExitBB();
  G4_BB *end = newNode->getEndBB();

  // Merging from ANStackIx to the top of ANStack is needed to make
  // sure the order of children is correct.
  for (uint32_t i = newNode->ANStackIx + 1, size = ANStack.size(); i < size;
       ++i) {
    ANodeHG *nd = (ANodeHG *)ANStack[i];
    getNewRange(end, exit, nd->getEndBB(), nd->getExitBB());
    for (ANList::iterator I = nd->children.begin(), E = nd->children.end();
         I != E; ++I) {
      ANode *tmp = *I;
      tmp->parent = newNode;
      newNode->children.push_back(tmp);
    }
  }
  newNode->isLoopCandidate = false;
  newNode->setExitBB(exit);
  newNode->setEndBB(end);
  uint32_t n = (uint32_t)ANStack.size() - newNode->ANStackIx - 1;
  if (n > 0) {
    for (uint32_t i = 0; i < n; ++i) {
      ANode *nd = ANStack.back();

      // Make sure to delete from newNode' children
      ANListErase(newNode->children, nd);

      ANStack.pop_back();
      delete nd;
    }
  }

  node = newNode;
  return;
}

// For case like:
//   do: ...
//       if (p) {
//         break;
//       else
//         ...
//       endif
//       .....
//   while
//
//   if-else-endif does not form a hammock because of break (it has two exits,
//   one is the target of break, the other is the BB right after endif). we
//   will need to make break as "no-op" instruction and then re-run
//   constructPST() on the body of the while loop.
//
//  For now, implement a poor-man's break construction. More general code
//  will come after.
//
void CFGStructurizer::reConstructDoWhileforBreak(ANodeHG *whileNode) {
  G4_BB *exitbb = whileNode->getExitBB();
  ANList::iterator IB = whileNode->children.begin();
  ANList::iterator IE = whileNode->children.end();
  ANList::iterator II = IB;
  BB_LIST activeJoinBBs;
  while (II != IE) {
    ANode *nd = *II;
    while (!activeJoinBBs.empty()) {
      G4_BB *bb = activeJoinBBs.front();
      if (isBefore(nd->getBeginBB(), bb)) {
        break;
      }
      activeJoinBBs.pop_front();
    }

    G4_BB *ebb = nd->getExitBB();
    if (ebb != exitbb) {
      if (ebb) {
        bb_oinsert(activeJoinBBs, ebb);
      }
      ++II;
      continue;
    }

    if (nd == whileNode->children.back()) {
      ++II;
      continue;
    }

    whileNode->setHasBreak(true);
    if (nd->getType() != AN_BB) {
      // don't handle it.
      whileNode->setAllowSCF(false);
      break;
    }

    ANodeBB *ndbb = (ANodeBB *)nd;
    if (II == IB || activeJoinBBs.empty()) {
      // Break isn't inside any possible gotos (conservative)
      ndbb->setHasBreak(true);
      ++II;
      continue;
    }

    if (ndbb->preds.size() != 1) {
      whileNode->setAllowSCF(false);
      break;
    }

    ANode *exitNode = ndbb->succs.back();
    if (whileNode->isMember(exitNode)) {
      exitNode = ndbb->succs.front();
    }
    ANode *pred = ndbb->preds.back();
    ANList::iterator IPred = II;
    --IPred;
    ANode *phyPred = *IPred;
    ANList::iterator IPredPred = IPred;
    ANode *phyPredPred = nullptr;
    if (IPredPred != IB) {
      --IPredPred;
      phyPredPred = *IPredPred;
    }

    ANList::iterator ISucc = II;
    ++ISucc;
    ANode *phySucc = (ISucc != IE) ? *ISucc : nullptr;
    ANode *predTarget = ANListGetOther(pred->succs, ndbb);

    // Handle two cases:
    //  Case 1 (if-then-endif with break)
    //    BB0:
    //       [p] goto BB2
    //    BB1:
    //       break;
    //    BB2:
    //
    // Case 2: (if-then-else-endif with break)
    //    BB0:
    //       [p] goto BB2
    //    BB1:
    //       goto BB3;
    //    BB2:
    //       break;
    //    BB3:
    //

    // case 1
    if (phyPred == pred && pred->getType() == AN_BB && predTarget &&
        predTarget == phySucc) {
      bool calNewIB = (IPred == IB);

      ANodeHG *newIf = new ANodeHG(AN_IF_THEN_ENDIF);
      newIf->anodeId = getNextANodeId();

      newIf->parent = whileNode;
      whileNode->children.erase(IPred);
      whileNode->children.erase(II);
      whileNode->children.insert(ISucc, newIf);
      reConnectAllPreds(pred, newIf);
      newIf->succs.push_back(predTarget);
      newIf->succs.push_back(exitNode);
      ANListReplace(exitNode->preds, ndbb, newIf);
      ANListReplace(predTarget->preds, pred, newIf);

      newIf->children.push_back(pred);
      newIf->children.push_back(ndbb);
      newIf->setBeginBB(pred->getBeginBB());
      newIf->setEndBB(ndbb->getEndBB());
      newIf->setExitBB(predTarget->getBeginBB());
      pred->parent = newIf;
      ndbb->parent = newIf;

      ndbb->setHasBreak(true);
      newIf->setHasBreak(true);
      ndbb->setAllowSCF(true);
      newIf->setAllowSCF(true);

      if (calNewIB) {
        IB = whileNode->children.begin();
      }
      II = ISucc;
    }
    // Case 2
    else if (predTarget && phyPredPred && phyPredPred == pred &&
             phyPred == predTarget && pred->getType() == AN_BB &&
             phyPred->succs.size() == 1 && phyPred->succs.front() == phySucc) {
      bool calNewIB = (IPredPred == IB);

      ANodeHG *newIf = new ANodeHG(AN_IF_THEN_ELSE_ENDIF);
      newIf->anodeId = getNextANodeId();

      newIf->parent = whileNode;
      whileNode->children.erase(IPredPred);
      whileNode->children.erase(IPred);
      whileNode->children.erase(II);
      whileNode->children.insert(ISucc, newIf);
      reConnectAllPreds(pred, newIf);
      newIf->succs.push_back(phySucc);
      newIf->succs.push_back(exitNode);
      ANListReplace(exitNode->preds, ndbb, newIf);
      ANListReplace(predTarget->preds, pred, newIf);

      newIf->children.push_back(pred);
      newIf->children.push_back(phyPred);
      newIf->children.push_back(ndbb);
      newIf->setBeginBB(pred->getBeginBB());
      newIf->setEndBB(ndbb->getEndBB());
      newIf->setExitBB(phySucc->getBeginBB());
      pred->parent = newIf;
      phyPred->parent = newIf;
      ndbb->parent = newIf;

      ndbb->setHasBreak(true);
      newIf->setHasBreak(true);
      ndbb->setAllowSCF(true);
      newIf->setAllowSCF(true);

      if (calNewIB) {
        IB = whileNode->children.begin();
      }
      II = ISucc;
    } else {
      whileNode->setAllowSCF(false);
      break;
    }
  }

  if (!whileNode->getAllowSCF()) {
    // If not allowed, propagate this attribute to all
    // its children with break inside.
    ANList::iterator II = IB;
    for (II = IB; II != IE; ++II) {
      ANode *nd = *II;
      if (nd->getHasBreak()) {
        nd->setAllowSCF(false);
      }
    }
  }
}

//
// PST : program structure tree.
//         ANode -- PST's node. It's an abstract class.
//           ANodeBB : ANode -- one for each BB (leaf node)
//           ANodeHG : ANode -- Hammock graph (group of ANodeBB)
//                              (non-leaf node)
//   ANodeBB is for BB, while ANodeHG is a Hammock graph that is
//   composed of seveal BBs (mostly more than one, it could be one).
//   The Hammock graph defined in this implementation is the minimum
//   hammock graph for any goto BB (BB that has goto inst). Nesting
//   relation is represented with ANode's parent field and ANodeHG's
//   children (ANodeBB cannot have children, it is a leaf node). Given
//   two hammock graphs, they either have no common ANodes or one is
//   completely enclosed within the other. The Whole program is a Hammock,
//   and this node is the PST's root node.
//
//   In the implementation, we don't really create ANode for the PST's
//   root. Instead, we construct only hammock children of the Root,
//   denoted by topANodes. This topANodes are refered to as top-level
//   nodes. (Note that for any BB that is b/w any goto BB and its target
//   BB in the program order, an ANode is created for it; otherwise, it
//   has no ANode.)
//
//   Note that the children of each ANodeHG is the ordered list of ANode
//   (in BB order).
//
// ACFG : Abstract Control Flow Graph of ANode (ACFG):
//   This is a CFG of ANode, used in the process of constructing PST.
//   It can be viewed as the top-level PST on-the-fly (ignoring any nodes
//   that are children of the top level nodes). By "on-the-fly", it means
//   that the ACFG is constantly changing. Once a group of nodes in ACFG
//   is formed into a single ANodeHG, this group is replaced by this new
//   single ANodeHG, and the group of nodes is no longer in the ACFG. Such
//   node condensing will make finding interesting program structures (if,
//   while, etc.) easier. The process continues until all BBs are processed.
//   All the nodes in the final ACFG is the actually top-level nodes of the PST.
//   (In another word, ACFG will always refect the control flow among the
//    outmost parent nodes of all ANodeBBs.)
//
//   ACFG's control-flow edges : represented by ANode's preds & succs.
//
//   Note that ACFG edges within any ANodeHG via preds & succs should be still
//   valid, but outgoing/incoming edges are no longer valid. To get a right
//   ANode target, using BB's preds/succs as they are always up to date.
//
// input
//    [IB, IE) : denotes BB from IB to IE, containing IB, but not IE.
// Results
//    topANodes
//
void CFGStructurizer::constructPST(BB_LIST_ITER IB, BB_LIST_ITER IE) {
  // This algorithm assumes that IE is loop-invariant. For the code
  // that might insert/delete BB, pay attention to this rule!

  // ANStack is used to keep all pending ANodes. It also reflects
  // possible nesting relation (ANStack[i] could be the parent of
  // ANStack[i+1]).
  ANodeHG *node = nullptr;
  BB_LIST_ITER II = IB;
  while (II != IE) {
    G4_BB *bb = *II;
    ANodeBB *ndbb = getANodeBB(bb);

    // Do the following cases in order. (Note that bb should be part of
    // the current pending ANode (top of ANStack), although it could
    // be the beginning node of the new inner ANode.)
    //
    // 1: If bb is the target of some gotos, that is, the end of
    //    some CGs, merge the current bb into ANStack nodes of
    //    which those CGs belong to (as they should be in the same
    //    hammock).
    //    Also, any backward goto's begin and end BB should be in
    //    the same hammock. Once merging is done (if any), go on to
    //    the next step in the loop.
    //
    // 2: If bb is the begin of CGs, need to create new HG for each
    //    cg that is associated with a backward goto, a forward
    //    conditional goto, and the root (first) forward unconditional
    //    goto.  No new HG is created for a non-root unconditional
    //    goto (see details in code). This is where an inner HG is formed.)
    //
    // 3: Add bb into this node as it is part of node. If bb can
    //    be merged with its pred, merge it. The new node's type
    //    must be type AN_SEQUENCE.
    //
    // 4: Check if bb is the end of the node.  If it is, finalize the
    //    node. If it is not, go on to process the next BB in the next
    //    iteration (This implies that if the next BB should be part
    //    of the current pending HG in ANStack).
    //

    // case 1.
    if (node) {
      ANStack_reconstruct(node, ndbb);
    }

    // case 2
    CGList cgs;
    if (getCGBegin(bb, cgs)) {
      // If bb is the begin of more than one ANode on ANStack, ndbb
      // should be added only into innermost ANode. Keep the
      // following in mind:
      //   1. (a) bb's last inst is a goto.
      //          ndbb should be added to the current node (ANStack's
      //          top) if it exists and the goto is a unconditional goto;
      //          otherwise, a new hammock node is created and ndbb
      //          is added into this new hammock node (thus for both
      //          conditional goto and a top node).
      //      (b) bb's last inst is not a goto
      //          add ndbb in the current node.
      //   2. If there are several backward gotos with bb as the begin
      //      BB, ndbb is added only into the ANode corresponding to the
      //      innermost backward goto. (All backward gotos should have their
      //      new ANodes created.)
      ControlGraph *cg;

      // For backward cg, we will only create ANodeHG if the backward cg
      // is a loop candidate. If it is not candidate, extend the current
      // node with this cg.
      for (uint32_t i = (uint32_t)cgs.size() - 1; i > 0; --i) {
        // This must be non-innermost backward goto, don't add ndbb to
        // the new node, ie. passing nullptr(2nd arg) to ANodeHG's ctor.
        cg = cgs.back();
        vISA_ASSERT(cg->isBackward,
                     "Additional CG must be for backward gotos");
        if (!cg->isLoopCandidate) {
          vISA_ASSERT(node != nullptr, "Node should not be empty here!");
          extendNode(node, cg);
        } else {
          ANodeHG *parent = node;
          node = new ANodeHG(cg, nullptr);
          node->anodeId = getNextANodeId();
          node->parent = parent;
          node->isLoopCandidate = true;
          if (parent) {
            parent->children.push_back(node);
          }
          ANStack_push(node);
        }
        cgs.pop_back();
        delete cg;
      }

      // First one in cgs, it is either an innermost backward goto or a forward
      // goto
      cg = cgs.back();
      cgs.pop_back();
      if (cg->isBackward || cg->gotoInst->getPredicate() != nullptr ||
          node == nullptr) {
        if (cg->isBackward && !cg->isLoopCandidate) {
          vISA_ASSERT(node != nullptr, "Node should not be empty at this point!");
          extendNode(node, cg);
        } else {
          // New HG node created and pushed on ANStack
          ANodeHG *parent = node;
          node = new ANodeHG(cg, ndbb);
          node->anodeId = getNextANodeId();
          node->parent = parent;
          if (cg->isBackward) {
            node->isLoopCandidate = true;
          }
          if (parent) {
            parent->children.push_back(node);
          }
          ANStack_push(node);
        }
      } else {
        // For an unconditional goto, just add it into the current ANodeHG.
        G4_BB *exit = node->getExitBB();
        G4_BB *end = node->getEndBB();
        getNewRange(end, exit, ndbb->getEndBB(), ndbb->getExitBB());
        node->setExitBB(exit);
        node->setEndBB(end);
      }
      delete cg;
    } else if (!node) {
      // Not a goto BB, skip.
      ++II;
      continue;
    }

    // case 3
    // Add this new ANodeBB into node and also check if it can be
    // merged with the previous one to form a new SEQUENCE hammock.
    if (ndbb->parent != node) {
      node->children.push_back(ndbb);
      ndbb->parent = node;
    }
    (void)mergeWithPred(ndbb);

    // Step 4
    if (isBefore(bb, node->getEndBB())) {
      ++II;
      continue;
    }

    // At this point, bb == node->getEndBB().
    // Found a HG if
    //    1. bb is a unconditional backward goto, or
    //    2. bb's fall-thru is the same as node's exitBB
    G4_INST *gotoInst = getGotoInst(bb);
    bool uncondBackwardGoto =
        gotoInst && bb->Succs.size() == 1 && gotoInst->asCFInst()->isBackward();
    if (!(uncondBackwardGoto || ndbb->succs.size() == 0 ||
          node->getExitBB() == nullptr ||
          ndbb->succs.front()->getBeginBB() == node->getExitBB())) {
      G4_BB *next = bb->getPhysicalSucc();
      if (next) {
        node->setEndBB(next);
      }
      ++II;
      continue;
    }

    // At this point, node is a MHG.

    // The top of ANStack is an MHG, poping and condensing it into a new
    // ANodeHG. After condensing, it might expose merging opportunity,
    // if so, merge the newly condensed node with its predecessor. Once
    // merging and/or condensing is performed, check if the new top of
    // ANStack is an MHG, if so, doing the above again. Continue this
    // until the top of ANStack is not a HG.
    //
    // At this stage, if a MHG's exit isn't its physical next,  add a
    // landingBB as its next physical BB.  Note that the landingBB is
    // the new exit BB, which is not part of this MHG. For now, we will
    // only generate a single BB at most after each original BB. In
    // another word, if more than one MHG share the same exit,  it will
    // share this same landingBB (creating more than one BB could make
    // jmpi/goto inefficient - two goto/jmpi instead of one.
    // (May need a cfg simplification after structurizer.)
    //
    bool isTopHG = true;

    // Keep track of the outmost SCF so that we can add a landingBB
    // only for the oustmost SCF if needed.
    ANodeHG *outmostSCF = nullptr;
    ANodeHG *lastHG = nullptr;

    G4_BB *exitBB = node->getExitBB();
    while (isTopHG) {
      // a Hammock has been detected. Set HG to the right type.
      finalizeHG(node);

      // record the outmost structured CF MHG
      if (node->isSCF()) {
        outmostSCF = node;
      }

      // check if it can be merged with the previous ANode. Note that
      // we only need to check its immediate predecessor !
      ANode *nd = mergeWithPred(node);
      lastHG = (ANodeHG *)nd;

      isTopHG = false;
      ANStack_pop();
      if (!ANStack.empty()) {
        node = (ANodeHG *)ANStack.back();
        G4_BB *exit = node->getExitBB();
        G4_BB *end = node->getEndBB();
        getNewRange(end, exit, nd->getEndBB(), nd->getExitBB());
        node->setEndBB(end);
        node->setExitBB(exit);

        if (isBefore(bb, end) || exit != exitBB) {
          // No more HG that ends with this exitBB
          continue;
        }
        isTopHG = true;
      }
    } // while (istopHG)

    if (outmostSCF) {
      // Add a landingBB and adjust control flows (both CFG and ACFG).
      // The new BB will be processed later (so no PST updating is
      // needed here).
#if 0
            // TODO: for performance, we should only add landingBB for outmostSCF,
            // however, doing so would have to special-handling the landingBB as
            // it should merge with outmostSCF, which is not in ACFG anymore (not
            // a top node anymore). So far, the this function is able to handle
            // node in ACFG only.
            if (!outmostSCF->isExitPhySucc())
            {
                (void)addLandingBB(outmostSCF, II, true);
            }
#else
      if (!lastHG->isExitPhySucc() && lastHG->getExitBB()) {
        (void)addLandingBB(lastHG, II, true);
      }
#endif
    }

    if (ANStack.empty() && node) {
      // This is a top ANode, add it.
      topANodes.push_back(node);
      node = nullptr;
    }
    ++II;
  } // while (!ANStack.empty())

  vISA_ASSERT(ANStack.empty(), "ANodeHG stack is incorrectly formed");
#ifdef _DEBUG
#if 0
        dumpcfg();
        dumpacfg();
#endif
#endif
  return;
}

// return true if bb0 is before bb1 in the BB list.
bool CFGStructurizer::isBefore(G4_BB *bb0, G4_BB *bb1) {
  vISA_ASSERT(bb0 && bb1, "BB ptrs should not be null");
  if (bb0 == bb1) {
    return false;
  }

  // common case first
  uint32_t id0 = bb0->getId();
  uint32_t id1 = bb1->getId();
  vISA_ASSERT(id0 != id1, "Two different BBs must have different Ids");

  if (id0 < numOfBBs && id1 < numOfBBs) {
    return id0 < id1;
  }

  uint32_t insertAtId0 = id0 >= numOfBBs ? getInsertAfterBB(bb0)->getId() : id0;
  uint32_t insertAtId1 = id1 >= numOfBBs ? getInsertAfterBB(bb1)->getId() : id1;
  if (insertAtId0 != insertAtId1) {
    return insertAtId0 < insertAtId1;
  }

  // Now, the case for a BB and its new BB or two new BBs with the same
  // insertion position.
  //
  // Since we make sure the larger the id, the later the position during
  // inserting new BBs. We will simply check their ids
  return id0 < id1;
}

// Insert newNode into parent and insertion place is after node
// if isAfter is true, or before node if isAfter is false.
void CFGStructurizer::PSTAddANode(ANodeHG *parent, ANode *node, ANode *newNode,
                                  bool isAfter) {
  newNode->parent = parent;
  if (parent) {
    ANList::iterator I1 = findANode(parent->children, node);
    vISA_ASSERT(I1 != parent->children.end(),
                 "Child node should be in parent's children list");
    if (isAfter) {
      ++I1;
    }
    parent->children.insert(I1, newNode);
  }
}

// adjustBBId will make sure that all the new BB's with the same insertion
// position should have ids in the increasing order.
//
// Before calling this function, newBB has to be inserted into
// the BBs and its physicalSucc/Pred has been set up correctly.
void CFGStructurizer::adjustBBId(G4_BB *newbb) {
  // newBB's id is the largest at this moment. Make sure it is in order
  uint32_t id = newbb->getId();
  G4_BB *pred = newbb;
  G4_BB *next = newbb->getPhysicalSucc();
  while (next && next->getId() >= numOfBBs) {
    pred->setId(next->getId());
    pred = next;
    next = next->getPhysicalSucc();
  }
  pred->setId(id);
}

G4_BB *CFGStructurizer::createBBWithLabel() {
  G4_BB *newBB = CFG->createNewBB();

  // Create a label for the new BB
  G4_Label *lab = CFG->builder->createLocalBlockLabel("cf");
  G4_INST *labelInst = CFG->builder->createLabelInst(lab, false);
  newBB->push_front(labelInst);
  return newBB;
}

// Insert an inst at the begin after a label instruction.
void CFGStructurizer::insertAtBegin(G4_BB *aBB, G4_INST *anInst) {
  INST_LIST_ITER I = aBB->begin();
  vISA_ASSERT((*I)->opcode() == G4_label,
               "The 1st inst of BB must be a label inst");
  ++I;
  aBB->insertBefore(I, anInst);
}

void CFGStructurizer::setJoinJIP(G4_BB *joinBB, G4_BB *jipBB) {
  G4_INST *firstInst = joinBB->getFirstInst();
  vISA_ASSERT(firstInst, "Missing a join inst in join BB!");
  if (firstInst &&
      (firstInst->opcode() == G4_join || firstInst->opcode() == G4_endif)) {
    G4_Label *jipLabel = jipBB ? jipBB->getLabel() : nullptr;
    firstInst->asCFInst()->setJip(jipLabel);
    return;
  }
  vISA_ASSERT_UNREACHABLE("Missing Join/endif instruction in join BB!");
}

void CFGStructurizer::convertChildren(ANodeHG *nodehg, G4_BB *nextJoinBB) {
  // JIP setting of Join instruction:
  //   activeJoinBBs : a list of join BBs in the increasing order of BB layout,
  //                   which are after the BB being processed currently.
  //
  //   The join BB is the BB with join instruction as its first instruction.
  //   If the current BB is the first join BB (if exists) in acticveJoinBBs,
  //   its JIP will be set to the 2nd join in activeJoinBBs. Once its JIP is
  //   set, this BB is removed from activeJoinBBs.
  //
  //   activeJoinBBs keeps a list of join BBs whose jips needs to be set.
  //   It works like this: for each child ANode, if its exit BB is a join BB,
  //   add it into activeJoinBBs (we check if a BB is a join BB by checking
  //   if the BB has join instruction in it). As conversion is done bottom-up,
  //   all children must have been converted before their parents. This means
  //   that a join should be inserted in a child's exit BB if that child does
  //   need join. But that join's JIP is not known until the next join is found.
  //   Adding unknown-JIP join into activeJoinBBs so that it will get set when
  //   next next join is reached.
  //
  BB_LIST activeJoinBBs;
  if (nextJoinBB) {
    // The incoming joinBB is a special one, it could
    // be a BB with its else/endif/while/join.
    activeJoinBBs.push_back(nextJoinBB);
  }

  // If nodehg is DO_WHILE, its end should have been processed in
  // convertDoWhile() before invoking this function, hence, skip the end BB.
  ANList::iterator II = nodehg->children.begin();
  ANList::iterator IE = nodehg->children.end();
  if (nodehg->getKind() == ANKIND_SCF && nodehg->getType() == AN_DO_WHILE) {
    if (nodehg->children.size() > 1) {
      --IE;
      ANode *loopWhileNode = *IE;
      loopWhileNode->setVisited(true);
    } else {
      II = IE;
    }
  }

  for (; II != IE; ++II) {
    ANode *nd = *II;
    G4_BB *begin = nd->getBeginBB();

    G4_BB *nextjoin = nullptr;
    while (!activeJoinBBs.empty()) {
      nextjoin = activeJoinBBs.front();
      if (begin == nextjoin || isBefore(nextjoin, begin)) {
        // nextjoin has been reached. Set its JIP.
        activeJoinBBs.pop_front();
        G4_BB *tmp = !activeJoinBBs.empty() ? activeJoinBBs.front() : nullptr;
        setJoinJIP(nextjoin, tmp);
      } else {
        break;
      }
    }

    // For loops, need to handle the backward goto and may insert join
    for (BB_LIST_ITER I1 = begin->Preds.begin(), E1 = begin->Preds.end();
         I1 != E1; ++I1) {
      // Don't process loopWhileNode if it is as it has been processed
      G4_BB *bb = *I1;
      G4_INST *gotoInst = getGotoInst(bb);
      ANodeBB *ndbb = getANodeBB(bb);
      ANode *nd1 = ndbb->getChildANode(nodehg);
      if (nd1 && nd != nd1 && gotoInst && !nd1->isVisited() &&
          gotoInst->asCFInst()->isBackward() && bb->Succs.back() == begin) {
        convertPST(nd1, nullptr);
        nd1->setVisited(true);
        G4_BB *loopJoinBB = bb->getPhysicalSucc();
        if (loopJoinBB && isJoinBB(loopJoinBB)) {
          // Add loop's join bb into activeJoinBBs
          if (findBB(&activeJoinBBs, loopJoinBB) == activeJoinBBs.end()) {
            bb_oinsert(activeJoinBBs, loopJoinBB);
          }
        }
      }
    }

    // Skip backward goto ANode, as it has been handled already
    if (nd->isVisited()) {
      continue;
    }

    nextjoin = activeJoinBBs.empty() ? nullptr : activeJoinBBs.front();

    (void)convertPST(nd, nextjoin);
    nd->setVisited(true);

    G4_BB *joinbb = nd->getExitBB();
    // Special case for ANodeBB of a break.
    //   As its exit BB is still the original target (loop exit), there is
    //   no join instruction needed at the target as it is implicit loop
    //   join place. Here, just set joinbb to nullptr.
    //   (need to refactor the code as SCF (except SEQUENCE) should not
    //    have a join in its exit.)
    if (nd->getType() == AN_BB && nd->getKind() == ANKIND_SCF &&
        nd->getHasBreak()) {
      joinbb = nullptr;
    }

#if 0
        if (nd->getType() == AN_BB && begin->size() > 0)
        {
            G4_INST *gotoInst = begin->back();
            vISA_ASSERT(false, "Backward goto should have ben processed");
            if (gotoInst->opcode() == G4_goto && gotoInst->isBackward())
            {
                //
                vISA_ASSERT(false, "Backward goto should have ben processed");
                joinbb = begin->Succs.front();
            }
        }
#endif
    if (joinbb && isJoinBB(joinbb)) {
      // Add exit into activeJoinBBs
      if (findBB(&activeJoinBBs, joinbb) == activeJoinBBs.end()) {
        bb_oinsert(activeJoinBBs, joinbb);
      }
    }
  }

  uint32_t sz = (uint32_t)activeJoinBBs.size();
#if 0
    // temporarily turn off assert as it is too conservative and miss
    // some cases, thus genrate false assertion!

    // activeJoinBBs should have the following at this moment:
    //    1. nextJoinBB
    //    2. the nodehg's exit(s).
    //       nodehg is a hammock, so it should have a single exit. However,
    //       for something like "if (c) break endif", and nodehg is somehow
    //       decided to be non-SCF (ie converted to goto), nodehg will have
    //       two exits.
    //  [todo: have a simple/better way to handle this]
    //  Note that the outer node of PST may recalculate JIP.
    uint32_t NEntries = nextJoinBB ? 2 : 1;
    if (nodehg->getHasBreak() && !nodehg->getAllowSCF())
    {
        ++NEntries;
    }
    vISA_ASSERT(sz <= NEntries, "Something is wrong!");
#endif
  if (sz > 0) {
    // lastJoin will never be nullptr.
    G4_BB *lastJoin = activeJoinBBs.front();
    activeJoinBBs.pop_front();
    for (--sz; sz > 0; --sz) {
      if (lastJoin == nextJoinBB) {
        // don't need to process it further as the outer
        // ANode will handle nextJoinBB and nodes after it.
        break;
      }
      G4_BB *JipBB = activeJoinBBs.front();
      activeJoinBBs.pop_front();
      setJoinJIP(lastJoin, JipBB);
      lastJoin = JipBB;
    }
    // Handle the last one.
    if (lastJoin != nextJoinBB) {
      // set the last to nullptr
      setJoinJIP(lastJoin, nullptr);
    }
  }
  return;
}

// TODO: more comments
// If then block and else block may have gotos, we will insert join at the end
// of then or else blocks as the new join exit.
void CFGStructurizer::convertIf(ANodeHG *node, G4_BB *nextJoinBB) {

  // #if defined(_DEBUG)
  //     useDebugEnv();
  // #endif

  // nextJoinLabel is used to set JIP for endif instruction
  G4_Label *nextJoinLabel = nextJoinBB ? nextJoinBB->getLabel() : nullptr;
  G4_BB *begin = node->getBeginBB();
  G4_INST *gotoInst = begin->back();
  G4_ExecSize execSize = gotoInst->getExecSize();
  execSize = execSize > 1 ? execSize : kernelExecSize;
  ANList::iterator II = node->children.begin();
  ANode *thenNode = *(++II); // children[1]
  [[maybe_unused]] G4_BB *end = node->getEndBB();
  G4_BB *exit = node->getExitBB();

  vISA_ASSERT(end->getPhysicalSucc() == exit,
               "Landing BB should have been inserted during construction of "
               "hammock graph");

  bool isUniform = isGotoScalarJmp(gotoInst);
  ANodeKind kind = ANKIND_GOTOJOIN;
  if (doScalarJmp && isUniform) {
    kind = ANKIND_JMPI;
  } else if (doStructCF && node->getAllowSCF()) {
    if (node->getHasBreak()) {
      ANodeHG *innermostWhile = getInnerMostWhile(node);
      vISA_ASSERT(innermostWhile, "if-endif with break isn't inside a while");
      if (innermostWhile->getKind() == ANKIND_SCF) {
        kind = ANKIND_SCF;
      }
    } else {
      kind = ANKIND_SCF;
    }
  }
  node->setKind(kind);

  // If this IF-ENDIF does not have break, there must be at least
  // one active channel enterning IF-ENDIF and at least one active
  // channel at the end of "IF-ENDIF". Thus endif's JIP will never
  // be used.
  if (!node->getHasBreak()) {
    nextJoinLabel = nullptr;
  }

  if (node->getHasBreak() && kind == ANKIND_GOTOJOIN) {
    // if break isn't honored, if-endif should not be
    // honored and should be treated as non if/endif node.
    convertChildren(node, nextJoinBB);
    return;
  }

  ANodeBB *exitndbb = getANodeBB(exit);
  if (kind != ANKIND_JMPI && !isANodeOnlyPred(exit, node) &&
      (exitndbb->isJmpiTarget || (kind == ANKIND_GOTOJOIN && !isJoinBB(exit)) ||
       (kind == ANKIND_SCF && !isBBLabelAvailable(exit)))) {
    // To have a new BB for either endif or join
    exitndbb = addLandingBB(node, BBs->end(), false);

    // Add it into PST. Will need {[SEQUENCE] Node, exitndbb} as
    // a new ANode to replace node.
    PSTAddANode((ANodeHG *)node->parent, node, exitndbb, true);

    exit = exitndbb->getBeginBB();
  }
  G4_Label *endifLabel = exit->getLabel();

  switch (node->type) {
  case AN_IF_THEN_ENDIF: {
    if (kind == ANKIND_JMPI) {
      CFG->convertGotoToJmpi(gotoInst);
      exitndbb->isJmpiTarget = true;
      (void)convertPST(thenNode, nextJoinBB);
      return;
    } else if (kind == ANKIND_GOTOJOIN) {
      // Generate goto/join
      generateGotoJoin(begin, exit, exit);
      setJoinJIP(exit, nextJoinBB);
      (void)convertPST(thenNode, node->getHasBreak() ? exit : nullptr);
      return;
    }

    G4_INST *endifInst = CFG->builder->createInternalCFInst(
        NULL, G4_endif, execSize, nextJoinLabel, NULL, gotoInst->getOption());
    // Set CISA index link here explicitly since not doing so causes
    // endif to merge with immediately following line. This prevents
    // debugger from setting bp on src line immediately following
    // the endif.
    endifInst->inheritDIFrom(gotoInst);
    insertAtBegin(exit, endifInst);

    // jip = uip = endif
    G4_Predicate *pred = gotoInst->getPredicate();
    vISA_ASSERT(pred != NULL, "if must have non-null predicate");
    pred->setState(pred->getState() == PredState_Plus ? PredState_Minus
                                                      : PredState_Plus);
    G4_INST *ifInst = CFG->builder->createInternalCFInst(
        pred, G4_if, execSize, endifLabel, endifLabel, gotoInst->getOption());
    ifInst->inheritDIFrom(endifInst);
    begin->pop_back();
    begin->push_back(ifInst);
    if (isUniform) {
      ifInst->asCFInst()->setUniform(true);
    }

    if (thenNode->isHammock()) {
      //  if:
      //       goto endif
      //       ...
      //       goto endif
      //  else
      //       ...
      //  endif:
      //
      //  changed to:
      //  if:
      //      goto joinbb
      //      ...
      //      goto joinbb
      // joinbb:   <--- insert join instruction here
      // else
      //      ...
      // endif:
      //
      ANodeBB *ndbb = addLandingBB((ANodeHG *)thenNode, BBs->end(), false);

      // For now, just insert it into node (should create SEQ)
      PSTAddANode(node, thenNode, ndbb, true);
    }
    (void)convertPST(thenNode, node->getHasBreak() ? exit : nullptr);

    break;
  }

  case AN_IF_THEN_ELSE_ENDIF: {
    ANode *elseNode = *(++II); // children[2];
    G4_BB *thenLastBB = thenNode->getEndBB();
    G4_INST *thenLastInst = thenLastBB->back();

    // May have the case in which thenLastInst isn't goto
    //  if (uniformCond)
    //     return;
    //  else
    //     ...
    //  endif
    vISA_ASSERT(
        (thenLastInst->opcode() != G4_goto && thenLastBB->Succs.size() == 0) ||
            (thenLastInst->opcode() == G4_goto &&
             thenLastInst->getPredicate() == nullptr),
        "Goto in then block should be unconditional");

    if (kind == ANKIND_JMPI) {
      // goto's target is elseBB, and no need to create a new label.
      CFG->convertGotoToJmpi(gotoInst);
      {
        G4_BB *elsebeginbb = elseNode->getBeginBB();
        ANodeBB *elsendbb = getANodeBB(elsebeginbb);
        elsendbb->isJmpiTarget = true;
      }
      if (thenLastInst->opcode() == G4_goto) {
        CFG->convertGotoToJmpi(thenLastInst);
        exitndbb->isJmpiTarget = true;
      }

      (void)convertPST(thenNode, nextJoinBB);
      (void)convertPST(elseNode, nextJoinBB);
      break;
    } else if (kind == ANKIND_GOTOJOIN) {
      // Generate goto/join
      G4_BB *elseFirstBB = elseNode->getBeginBB();
      generateGotoJoin(begin, elseFirstBB, elseFirstBB);

      // convertPST(thenNode, elseFirstBB) will handle this
      //   generateGotoJoin(thenLastBB, elseFirstBB, exit);
      (void)convertPST(thenNode, elseFirstBB);
      (void)convertPST(elseNode, exit);
      setJoinJIP(elseFirstBB, exit);
      setJoinJIP(exit, nextJoinBB);

      return;
    }

    // Add endif instruction into exit BB, else into the beginning
    // BB of else part,  and replace goto in the begin BB with if
    // instruction.
    auto endifInst = CFG->builder->createInternalCFInst(
        NULL, G4_endif, execSize, nextJoinLabel, NULL, gotoInst->getOption());
    endifInst->inheritDIFrom(gotoInst);
    insertAtBegin(exit, endifInst);

    G4_BB *elseFirstBB = elseNode->getBeginBB();
    G4_Label *elseLabel = elseFirstBB->getLabel();
    G4_Predicate *pred = gotoInst->getPredicate();
    vISA_ASSERT(pred != NULL, "if must have non-null predicate");
    pred->setState(pred->getState() == PredState_Plus ? PredState_Minus
                                                      : PredState_Plus);

    // if instruction : jip = else_label, uip = endif
    G4_INST *ifInst = CFG->builder->createInternalCFInst(
        pred, G4_if, execSize, elseLabel, endifLabel, gotoInst->getOption());
    ifInst->inheritDIFrom(endifInst);
    begin->pop_back();
    begin->push_back(ifInst);

    // else instruction: jip = uip = endif
    G4_BB *newThenLastBB = thenLastBB;
    if (thenNode->isHammock()) {
      ANodeBB *ndbb = addLandingBB((ANodeHG *)thenNode, BBs->end(), false);

      // Correct way to update PST is as {[SEQ]thenNode, newThenLastBB}
      // and replace thenNode with this new SEQ node.
      // For now, it does not matter much, just use the following
      PSTAddANode(node, thenNode, ndbb, true);

      newThenLastBB = ndbb->getBeginBB();
    }
    G4_INST *thenGoto = newThenLastBB->back();
    G4_INST *elseInst = CFG->builder->createInternalCFInst(
        NULL, G4_else, execSize, endifLabel, endifLabel, gotoInst->getOption());
    elseInst->inheritDIFrom(endifInst);
    if (thenGoto->opcode() == G4_goto) {
      newThenLastBB->pop_back();
    }
    newThenLastBB->push_back(elseInst);
    if (isUniform) {
      ifInst->asCFInst()->setUniform(true);
    }

    (void)convertPST(thenNode, node->getHasBreak() ? newThenLastBB : nullptr);

    if (elseNode->isHammock()) {
      // Insert a join bb
      ANode *ndbb = addLandingBB((ANodeHG *)elseNode, BBs->end(), false);
      PSTAddANode(node, thenNode, ndbb, true);
    }
    // exit must be the physical next of this if-endif
    (void)convertPST(elseNode, node->getHasBreak() ? exit : nullptr);
    break;
  }
  default:
    vISA_ASSERT_UNREACHABLE("Unreachable, must be a wrong node type");
    break;
  }
  return;
}

void CFGStructurizer::convertDoWhile(ANodeHG *node, G4_BB *nextJoinBB) {

  // #if defined(_DEBUG)
  //     useDebugEnv();
  // #endif

  G4_BB *end = node->getEndBB();
  G4_INST *gotoInst = end->back();
  G4_ExecSize execSize = gotoInst->getExecSize();
  execSize = execSize > g4::SIMD1 ? execSize : kernelExecSize;
  G4_BB *head = node->getBeginBB();
  G4_BB *exit = node->getExitBB();

  // For break/continue(no continue for now), unfortunately hammock does
  // not work. Need to specialize it. A generic approach is to
  // change all break/continue into a single ANode with its targets ignored
  // and just use its physical next as its next. Then re-do hammock only for
  // loop's children (no while BB). If every single break/continue are
  // within the structured no-loop CF, then it can be break/continue.
  // Note that mix of goto-join with break/continue may be not working, so
  // need to check it out to avoid generating them.
  //

  bool isUniform = isGotoScalarJmp(gotoInst);
  ANodeKind ndkind = ANKIND_GOTOJOIN;
  if (doScalarJmp && isUniform && (!doStructCF || !node->getHasBreak())) {
    // If loop has break, favor structured CF, not jmpi
    ndkind = ANKIND_JMPI;
  } else if (doStructCF && node->getAllowSCF()) {
    ndkind = ANKIND_SCF;
  }
  node->setKind(ndkind);
  if (ndkind == ANKIND_JMPI) {
    // goto's target is head.
    CFG->convertGotoToJmpi(gotoInst);
    convertChildren(node, nextJoinBB);
  } else if (ndkind == ANKIND_SCF) {
    G4_Label *doLabel = head->getLabel();

    // Create a new BB to keep while. This new BB
    // would be in the same PST ANode as the original end.
    ANode *ndbb = addSplitBBAtEnd(end);
    node->children.push_back(ndbb);
    ndbb->parent = node;

    end = ndbb->getBeginBB();

    // jip = uip = do
    G4_INST *whileInst = CFG->builder->createInternalCFInst(
        gotoInst->getPredicate(), G4_while, execSize, doLabel, doLabel,
        InstOpt_NoOpt);
    whileInst->inheritDIFrom(end->back());
    end->pop_back();
    end->push_back(whileInst);
    if (isUniform) {
      whileInst->asCFInst()->setUniform(true);
    }

    convertChildren(node, node->getHasBreak() ? end : nullptr);
  } else {
    if (!isJoinBB(exit) && !canbeJoinBB(exit)) {
      ANode *ndbb = addLandingBB(node, BBs->end(), false);

      // Make it simply by adding newexitbb into node->parent.
      // (May use {[SEQ] node, newexitbb} to replace.)
      PSTAddANode((ANodeHG *)node->parent, node, ndbb, true);

      exit = ndbb->getBeginBB();
    }
    generateGotoJoin(end, exit, exit);
    setJoinJIP(exit, nextJoinBB);
    getANodeBB(end)->setVisited(true);

    convertChildren(node, exit);
  }
}

void CFGStructurizer::generateGotoJoin(G4_BB *gotoBB, G4_BB *jibBB,
                                       G4_BB *joinBB) {
  G4_INST *gotoInst = gotoBB->back();
  vISA_ASSERT(gotoInst && gotoInst->opcode() == G4_goto,
               "gotoBB should have goto instruction");
  G4_Label *nextJoinLabel = jibBB ? jibBB->getLabel() : nullptr;
  G4_ExecSize execSize = gotoInst->getExecSize();
  execSize = execSize > g4::SIMD1 ? execSize : kernelExecSize;

  if (gotoInst->getExecSize() == g4::SIMD1) { // For simd1 goto, convert it to a
                                              // goto with the right execSize.
    gotoInst->setExecSize(execSize);
    gotoInst->setOptions(InstOpt_M0);
  }
  gotoInst->asCFInst()->setJip(nextJoinLabel);

  // set join's JIP to null as it will be handled later
  CFG->insertJoinToBB(joinBB, execSize, nullptr);

  if (!gotoInst->asCFInst()->isBackward() && !CFG->builder->gotoJumpOnTrue()) {
    // For BDW/SKL, need to flip goto's pred as false condition will jump.
    G4_Predicate *pred = gotoInst->getPredicate();
    if (pred) {
      pred->setState(pred->getState() == PredState_Plus ? PredState_Minus
                                                        : PredState_Plus);
    } else {
      // if there is no predicate, generate a predicate with all 0s.
      // if predicate is SIMD32, we have to use a :ud dst type for the move
      uint8_t numFlags = gotoInst->getExecSize() > g4::SIMD16 ? 2 : 1;
      G4_Declare *tmpFlagDcl = CFG->builder->createTempFlag(numFlags);
      G4_DstRegRegion *newPredDef = CFG->builder->createDst(
          tmpFlagDcl->getRegVar(), 0, 0, 1, numFlags == 2 ? Type_UD : Type_UW);
      G4_INST *predInst = CFG->builder->createMov(
          g4::SIMD1, newPredDef, CFG->builder->createImm(0, Type_UW),
          InstOpt_WriteEnable, false);
      INST_LIST_ITER iter = gotoBB->end();
      iter--;
      gotoBB->insertBefore(iter, predInst);

      pred = CFG->builder->createPredicate(PredState_Plus,
                                           tmpFlagDcl->getRegVar(), 0);
      gotoInst->setPredicate(pred);
    }
  }
}

void CFGStructurizer::convertGoto(ANodeBB *node, G4_BB *nextJoinBB) {
  G4_BB *beginbb = node->getEndBB();
  G4_INST *gotoInst = getGotoInst(beginbb);
  if (!gotoInst || node->isVisited()) {
    return;
  }
  node->setVisited(true);

  bool isUniform = isGotoScalarJmp(gotoInst);
  G4_BB *exitbb = node->getExitBB();
  ANodeHG *innermostWhile = getInnerMostWhile(node);
  ANodeKind kind = ANKIND_GOTOJOIN;
  vISA_ASSERT(innermostWhile || (!innermostWhile && !node->getHasBreak()),
               "Break isn't inside a while");
  if (innermostWhile && node->getHasBreak() &&
      innermostWhile->getKind() == ANKIND_SCF) {
    kind = ANKIND_SCF;
  } else {
    // nextJoinBB, if any, must be after getBeginBB(). So, if nextJoinBB
    // is before exitbb, nextJoinBB is in the middle of [beginbb, exitbb]
    bool hasJoinInMiddle = nextJoinBB && isBefore(nextJoinBB, exitbb);
    if (doScalarJmp && isUniform &&
        (gotoInst->asCFInst()->isBackward() || !hasJoinInMiddle)) {
      kind = ANKIND_JMPI;
    }
  }
  node->setKind(kind);

  //
  // generate goto/join
  //
  G4_ExecSize execSize = gotoInst->getExecSize();
  execSize = execSize > g4::SIMD1 ? execSize : kernelExecSize;

  if (kind == ANKIND_JMPI) {
    CFG->convertGotoToJmpi(gotoInst);
    {
      ANodeBB *exitndbb = getANodeBB(exitbb);
      exitndbb->isJmpiTarget = true;
    }

    if (beginbb == exitbb || isBefore(exitbb, beginbb)) {
      // TODO: Should find the 1st join BB out of this loop instead
      //       of using the next BB as join BB (see comments in
      //       convertChildren().
      G4_BB *joinbb = beginbb->getPhysicalSucc();

      // set join's JIP to null as it will be handled later
      CFG->insertJoinToBB(joinbb, execSize, nullptr);
    }
    return;
  }

  if (kind == ANKIND_SCF) {
    G4_BB *whilebb = innermostWhile->getEndBB();
    G4_BB *innerBlock = nextJoinBB ? nextJoinBB : whilebb;

    G4_INST *breakInst = CFG->builder->createInternalCFInst(
        gotoInst->getPredicate(), G4_break, execSize, innerBlock->getLabel(),
        whilebb->getLabel(), InstOpt_NoOpt);
    breakInst->inheritDIFrom(gotoInst);
    beginbb->pop_back();
    beginbb->push_back(breakInst);
    if (isUniform) {
      breakInst->asCFInst()->setUniform(true);
    }
    return;
  }

  G4_BB *joinbb = exitbb;
  G4_BB *JipBB = joinbb;
  if (beginbb == exitbb || isBefore(exitbb, beginbb)) {
    joinbb = beginbb->getPhysicalSucc();
    JipBB = joinbb;
  } else {
    if (nextJoinBB && isBefore(nextJoinBB, JipBB)) {
      JipBB = nextJoinBB;
    }
  }
  generateGotoJoin(beginbb, JipBB, joinbb);
}

bool CFGStructurizer::convertPST(ANode *node, G4_BB *nextJoinBB) {
  bool change = false;
  ANodeType ty = node->getType();
  if (ty == AN_IF_THEN_ENDIF || ty == AN_IF_THEN_ELSE_ENDIF ||
      ty == AN_DO_WHILE || ty == AN_SEQUENCE || ty == AN_COMPOSITE) {
    ANodeHG *nodehg = (ANodeHG *)node;
    if (node->type == AN_DO_WHILE) {
      // For a while loop, it must have at least one active
      // channel at exit. Thus, passing nullptr as nextJoinBB!
      convertDoWhile(nodehg, nullptr);
    } else if (node->type == AN_IF_THEN_ENDIF ||
               node->type == AN_IF_THEN_ELSE_ENDIF) {
      convertIf(nodehg, nextJoinBB);
    } else {
      convertChildren(nodehg, nextJoinBB);
    }
    change = true;
  } else if (ty == AN_BB) {
    convertGoto((ANodeBB *)node, nextJoinBB);
    change = true;
  } else {
    vISA_ASSERT_UNREACHABLE("Wrong ANode type");
  }

  return change;
}

// Get the debug info from DIFromBB and use it as I's debug info
void CFGStructurizer::updateInstDI(G4_INST *I, G4_BB *DIFromBB) {
  // sanity check
  if (DIFromBB == nullptr)
    return;

  // If DIFromBB is empty, skip setting.
  G4_INST *DIFromInst = nullptr;
  for (auto II = DIFromBB->begin(), IE = DIFromBB->end(); II != IE; ++II) {
    G4_INST *Inst = (*II);
    if (Inst->isLabel() || Inst->isLifeTimeEnd() || Inst->isPseudoKill() ||
        Inst->isPseudoUse() || Inst->isSpillIntrinsic() ||
        Inst->isFillIntrinsic()) {
      continue;
    }
    DIFromInst = Inst;
    break;
  }
  if (DIFromInst) {
    I->inheritDIFrom(DIFromInst);
  }
}

void CFGStructurizer::run() {
  constructPST(CFG->begin(), CFG->end());

  if (topANodes.size() == 0) {
    return;
  }

  // Now, convert ANodes
  if (topANodes.size() > 0) {
    for (uint32_t i = 0, size = topANodes.size(); i < size; ++i) {
      (void)convertPST(topANodes[i], nullptr);
    }
  }

  if (anodeBBs.newBBToANodeBB.size() > 0) {
    // New BBs generated, reassign block Ids.
    CFG->reassignBlockIDs();
  }
}

void doCFGStructurize(FlowGraph *FG) {
  CFGStructurizer S(FG);

  S.run();
}
