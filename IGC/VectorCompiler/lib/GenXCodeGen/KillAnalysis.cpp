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
//
// KillAnalysis is an object that can analyze which uses of a value are kills,
// and cache the result.
//
//===----------------------------------------------------------------------===//

#include "KillAnalysis.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Debug.h"

namespace {

// BlockInfo : info for one basic block when calculating the live range for
// one value
struct BlockInfo {
  llvm::Instruction *LastUser;
  bool LiveOut;
  BlockInfo() : LastUser(nullptr), LiveOut(false) {}
};

} // anonymous namespace

using namespace llvm;

/***********************************************************************
 * isKill : determine whether a use is a kill
 *
 * Enter:   U = the use, which must be of an Instruction or Argument
 *
 * Return:  true if this is a kill use (including the case that there are
 *          multiple uses in the same instruction, and no further reachable
 *          uses)
 *
 * This caches the information on which uses of the value are kills. If
 * anything changes to do with the value, such as changing uses or moving
 * code containing uses, or even completely removing the value, then the
 * caller must invalidate the cached information by calling erase(V).
 */
bool KillAnalysis::isKill(Use *U)
{
  SmallVectorImpl<Instruction *> *Kills = getKills(*U);
  for (unsigned i = 0, e = Kills->size(); i != e; ++i)
    if ((*Kills)[i] == U->getUser())
      return true;
  return false;
}

/***********************************************************************
 * getKills : get the kills vector for the value
 *
 * If there is no kills vector already cached for this value, we need to
 * create one by determining its live range and remembering which is the
 * last user in each basic block. Where a use is seen in a basic block,
 * we recursively add its predecessor blocks to the live range, stopping
 * when we get to an already seen block.
 *
 * This is pretty much the same as the algorithm in
 * Appel "Modern Compiler Implementation in C" 19.6.
 *
 */
SmallVectorImpl<Instruction *> *KillAnalysis::getKills(Value *V)
{
  auto MapIter = Map.find(V);
  if (MapIter != Map.end())
    return &MapIter->second;
  // Need to construct live range for this value so we can find the kill uses.
  std::map<BasicBlock *, BlockInfo> Blocks;
  // If the value is an instruction, set up the def as the last user in its
  // basic block. Don't do anything for an argument.
  if (auto Inst = dyn_cast<Instruction>(V))
    Blocks[Inst->getParent()].LastUser = Inst;
  // Trace back from each use.
  for (auto ui = V->use_begin(), ue = V->use_end(); ui != ue; ++ui) {
    auto user = cast<Instruction>(ui->getUser());
    if (auto Phi = dyn_cast<PHINode>(user)) {
      // Use in a phi node. Just mark the incoming block as live out.
      Blocks[Phi->getIncomingBlock(ui->getOperandNo())].LiveOut = true;
      continue;
    }
    auto BB = user->getParent();
    auto BI = &Blocks[BB];
    if (BI->LiveOut)
      continue; // already live out of this block
    if (BI->LastUser == V) {
      // This is the first time we have seen a use in this block, and it is
      // the def block. It is tentatively the last user in the block, and
      // no tracing back is required.
      BI->LastUser = user;
      continue;
    }
    bool LiveIn = false;
    if (!BI->LastUser) {
      // This is the first time we have seen a use in this block. It is
      // tentatively the last user in the block.
      BI->LastUser = user;
      LiveIn = true;
    } else if (BI->LastUser != user) {
      // There was already a tentative last use in this block (in a different
      // instruction to the present use). We need to see which one comes last.
      // To attempt to optimize the case that the two uses are fairly close
      // together in a large basic block, we walk both forwards and backwards
      // at the same time.
      auto Backwards = BI->LastUser;
      auto Forwards = BI->LastUser;
      for (;;) {
        if (Backwards != &BB->front()) {
          Backwards = Backwards->getPrevNode();
          if (Backwards == user) {
            // user is not the last user
            break;
          }
        }
        if (Forwards != &BB->back()) {
          Forwards = Forwards->getNextNode();
          if (Forwards == user) {
            // user is the last user.
            BI->LastUser = user;
            break;
          }
        }
      }
    }
    if (!LiveIn)
      continue;
    // We now need to trace back through predecessors.
    SmallVector<BasicBlock *, 4> Stack;
    for (;;) {
      if (BB) {
        // Push predecessors onto stack.
        for (auto bui = BB->use_begin(), bue = BB->use_end(); bui != bue; ++bui) {
	      Instruction *Inst = cast<Instruction>(bui->getUser());
          assert(Inst && Inst->isTerminator() && "cannot cope with computed goto");
          Stack.push_back(Inst->getParent());
        }
      }
      // Get a predecessor from the stack.
      if (Stack.empty())
        break;
      BB = Stack.back();
      Stack.resize(Stack.size() - 1);
      // Mark it live out. If it is already live out, or we have already seen a
      // use there, we do not need to trace back.
      BI = &Blocks[BB];
      if (BI->LiveOut || BI->LastUser)
        BB = nullptr;
      BI->LiveOut = true;
    }
  }
  // Create a new entry in the map for this value, and populate it with the
  // kill uses.
  // Note that the order in which we populate the kill uses vector depends on
  // memory layout, so if anything starts to depend on it, we should change
  // this code to use a fixed ordering.
  auto MapEntry = &Map[V];
  for (auto i = Blocks.begin(), e = Blocks.end(); i != e; ++i)
    if (!i->second.LiveOut && i->second.LastUser)
      MapEntry->push_back(i->second.LastUser);
  return MapEntry;
}

