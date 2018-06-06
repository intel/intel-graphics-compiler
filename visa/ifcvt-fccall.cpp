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

#include <tuple>
#include "ifcvt-fccall.h"
#include "common.h"
#include "BuildIR.h"

using namespace vISA;

namespace {

struct FCCallIfConvertible {
  G4_Predicate *pred;
  G4_BB *head, *call, *tail;

  FCCallIfConvertible(G4_Predicate *p, G4_BB *h, G4_BB *c, G4_BB *t)
      : pred(p), head(h), call(c), tail(t) {}
};

class FCCallIfConverter {
  FlowGraph &fg;

  /// getSinglePredecessor - Get the single predecessor or null otherwise.
  G4_BB *getSinglePredecessor(G4_BB *BB, G4_BB *If) const {
    if (BB->Preds.size() != 1) {
      if (BB->Preds.size() == 2) {
        if (BB->Preds.front() == If)
          return BB->Preds.back();
        if (BB->Preds.back() == If)
          return BB->Preds.front();
      }
      return nullptr;
    }
    return BB->Preds.front();
  }

  /// getSingleSuccessor - Get the single successor or null otherwise.
  G4_BB *getSingleSuccessor(G4_BB *BB, G4_BB *Else) const {
    if (BB->Succs.size() != 1) {
      if (BB->Succs.size() == 2) {
        if (BB->Succs.front() == Else)
          return BB->Succs.back();
        if (BB->Succs.back() == Else)
          return BB->Succs.front();
      }
      return nullptr;
    }
    return BB->Succs.front();
  }

  std::tuple<G4_INST * /* last instruction in head */,
             G4_BB * /* if */, G4_BB * /* tail */>
  getInnermostIfBlock(G4_BB *BB) const {
    if (BB->empty())
      return std::make_tuple(nullptr, nullptr, nullptr);

    G4_INST *last = BB->back();

    if (last->getMaskOption() & InstOpt_WriteEnable)
      return std::make_tuple(nullptr, nullptr, nullptr);

    G4_opcode op = last->opcode();
    if (op != G4_jmpi)
      return std::make_tuple(nullptr, nullptr, nullptr);

    if (!last->getPredicate())
      return std::make_tuple(nullptr, nullptr, nullptr);

    // Skip backward jump.
    if (last->isFlowControl() && last->asCFInst()->isBackward())
      return std::make_tuple(nullptr, nullptr, nullptr);

    ASSERT_USER(BB->Succs.size() == 2,
                "'jmpi' should have exactly two successors!");
    ASSERT_USER(last->getPredicate(),
                "'jmpi' should be conditional!");

    G4_BB *s0 = BB->Succs.front();
    G4_BB *s1 = BB->Succs.back();

    G4_BB *t0 = getSingleSuccessor(s0, s1);
    if (!t0)
      return std::make_tuple(nullptr, nullptr, nullptr);

    if (!getSinglePredecessor(s0, s1))
      return std::make_tuple(nullptr, nullptr, nullptr);

    if (t0 != s1)
      return std::make_tuple(nullptr, nullptr, nullptr);

    if (s0->size() != 2)
      return std::make_tuple(nullptr, nullptr, nullptr);

    if (s0->isEndWithFCall())
      return std::make_tuple(nullptr, nullptr, nullptr);

    return std::make_tuple(last, s0, t0);
  }

  /// reversePredicate - Reverse the predicate state.
  void reversePredicate(G4_Predicate *pred) const {
    G4_PredState state = pred->getState();
    switch (state) {
    case PredState_Plus: state = PredState_Minus; break;
    case PredState_Minus: state = PredState_Plus; break;
    default: break;
    }
    pred->setState(state);
  }

  /// An alternative to c++11 standard 'std::to_string' but does not require
  /// c++11. 'T' should be restricted to integer types by using 'enable_if' to
  /// avoid breaking broken c++ support on some platforms.
  template<typename T>
  std::string toString(T v) const {
      std::ostringstream oss;
      oss << v;
      return oss.str();
  }

  /// markEmptyBB - Mark the given BB as empty.
  void markEmptyBB(IR_Builder *IRB, G4_BB *BB) const {
    ASSERT_USER(BB->empty(), "BB to be marked empty is not empty!");

    std::string id = "LABEL__EMPTYBB__" + toString(BB->getId());
    G4_Label *label = IRB->createLabel(id, LABEL_BLOCK);
    G4_INST *inst = IRB->createInst(nullptr, G4_label, nullptr, false,
                                    UNDEFINED_EXEC_SIZE, nullptr, label,
                                    nullptr, 0);
    BB->push_back(inst);
  }

public:
  FCCallIfConverter(FlowGraph &g) : fg(g) {}

  void analyze(std::vector<FCCallIfConvertible> &);
  void convert(FCCallIfConvertible &);
};

} // End anonymous namespace

void runIfCvtFCCall(vISA::FlowGraph &fg) {
  if (!fg.builder->getFCPatchInfo()->getHasFCCalls())
    return;
  FCCallIfConverter cvt(fg);

  std::vector<FCCallIfConvertible> ifList;
  cvt.analyze(ifList);

  for (auto II = ifList.rbegin(), IE = ifList.rend(); II != IE; ++II)
    cvt.convert(*II);
}

void FCCallIfConverter::analyze(std::vector<FCCallIfConvertible> &L) {
  for (auto *BB : fg.BBs) {
    G4_INST *ifInst;
    G4_BB *s0, *t;
    std::tie(ifInst, s0, t) = getInnermostIfBlock(BB);

    if (!ifInst)
      continue;

    G4_Predicate *pred = ifInst->getPredicate();
    L.push_back(FCCallIfConvertible(pred, BB, s0, t));
  }
}

void FCCallIfConverter::convert(FCCallIfConvertible &FC) {
  G4_Predicate &pred = *FC.pred;
  G4_BB *head = FC.head;
  G4_BB *call = FC.call;
  G4_BB *tail = FC.tail;

  G4_INST *def = pred.getInst()->getSingleDef(Opnd_pred, true);
  // Widen the predication calculation from SIMD1 to SIMD2.
  if (!def || def->getExecSize() != 1 || !def->getDst()->isNullReg())
    return;
  G4_CondMod *cond = def->getCondMod();
  G4_Declare *flag = fg.builder->createTempFlag(1, "pred");
  G4_CondMod *ncond =
      fg.builder->createCondMod(cond->getMod(), flag->getRegVar(), 0);
  def->removeDefUse(Opnd_condMod);
  def->setCondMod(ncond);
  def->setExecSize(2);
  // Create the new predicate.
  G4_Predicate *npred =
      fg.builder->createPredicate(pred.getState(), flag->getRegVar(), 0);

  INST_LIST_ITER pos = std::prev(head->end());
  for (/* EMPTY*/; !call->empty(); call->pop_front()) {
    auto I = call->front();
    if (I->opcode() == G4_label)
      continue;
    G4_Predicate *neg = fg.builder->createPredicate(*npred);
    reversePredicate(neg);
    I->setPredicate(neg);
    I->setOptionOn(InstOpt_WriteEnable);
    head->insert(pos, I);
  }
  markEmptyBB(fg.builder, call);
  // Remove its use on the original condmod.
  head->back()->removeDefUse(Opnd_pred);
  head->erase(pos);

  if (tail->Preds.size() != 2)
    return;

  // Remove 'label' and 'endif'/'join' instructions in tail.
  ASSERT_USER(tail->front()->opcode() == G4_label,
              "BB is not started with 'label'!");
  tail->pop_front();
  if (!tail->empty()) {
    ASSERT_USER(tail->front()->opcode() == G4_endif ||
                tail->front()->opcode() == G4_join,
                "Convertible if is not ended with 'endif'!");
    tail->pop_front();
  }
  // Merge head and tail to get more code scheduling chance.
  head->splice(head->end(), tail);
  markEmptyBB(fg.builder, tail);
}
