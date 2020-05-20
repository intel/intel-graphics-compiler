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

#include "WIAnalysis.hpp"

#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/InitializePasses.h"
#include <llvm/IR/CFG.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>

#include "llvmWrapper/IR/InstrTypes.h"


#include <sstream>
#include <stack>
#include <string>

using namespace llvm;

static cl::opt<bool> PrintWiaCheck("print-wia-check", cl::init(true),
                                   cl::Hidden,
                                   cl::desc("Debug wia-check analysis"));

namespace pktz {

WIAnalysis::WIAnalysis() : FunctionPass(ID) {
  initializeWIAnalysisPass(*PassRegistry::getPassRegistry());
}

const unsigned int WIAnalysis::MinIndexBitwidthToPreserve = 16;

void WIAnalysis::print(raw_ostream &OS, const Module *) const {
  DenseMap<BasicBlock *, int> BBIDs;
  int id = 0;
  for (Function::iterator I = m_func->begin(), E = m_func->end(); I != E;
       ++I, ++id) {
    BasicBlock *BB = &*I;
    BBIDs[BB] = id;
  }

  OS << "WIAnalysis: " << m_func->getName().str() << "\n";

  OS << "Args: \n";
  for (Function::arg_iterator I = m_func->arg_begin(), E = m_func->arg_end();
       I != E; ++I) {
    Value *AVal = &*I;
    DenseMap<const Value *, WIDependancy>::const_iterator dep_it =
        m_deps.find(AVal);
    if (dep_it != m_deps.end())
      OS << "    " << "STRIDE:" << dep_it->second << " " << *AVal << "\n";
    else
      OS << "  unknown " << *AVal << "\n";
  }
  OS << "\n";

  for (Function::iterator I = m_func->begin(), E = m_func->end(); I != E; ++I) {
    BasicBlock *BB = &*I;
    OS << "BB:" << BBIDs[BB];
    if (BB->hasName())
      OS << " " << BB->getName();
    OS << "       ; preds =";
    bool isFirst = true;
    for (pred_iterator PI = pred_begin(BB), PE = pred_end(BB); PI != PE; ++PI) {
      BasicBlock *pred = *PI;
      OS << ((isFirst) ? " " : ", ") << "BB:" << BBIDs[pred] << "  ";
      if (pred->hasName())
        OS << pred->getName();
      isFirst = false;
    }
    OS << "\n";
    for (BasicBlock::iterator it = BB->begin(), ie = BB->end(); it != ie;
         ++it) {
      Instruction *I = &*it;
      DenseMap<const Value *, WIDependancy>::const_iterator dep_it =
          m_deps.find(I);
      if (dep_it != m_deps.end()) {
        OS << "  " << "STRIDE:" << dep_it->second << " " << *I;
      } else {
        OS << "  unknown " << *I;
      }
      if (I->isTerminator()) {
        auto TI = cast<IGCLLVM::TerminatorInst>(I);
        OS << " [";
        for (unsigned i = 0, e = TI->getNumSuccessors(); i < e; ++i) {
          BasicBlock *succ = TI->getSuccessor(i);
          OS << " BB:" << BBIDs[succ];
        }
        OS << " ]";
      }
      OS << "\n";
    }
    OS << "\n";
  }
}

bool WIAnalysis::runOnFunction(Function &F) {

  if (!F.hasFnAttribute("CMGenxSIMT"))
    return false;
  m_func = &F;
  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();

  m_deps.clear();
  m_changed1.clear();
  m_changed2.clear();
  m_pChangedNew = &m_changed1;
  m_pChangedOld = &m_changed2;
  m_ctrlBranches.clear();

  initDependency(&F);

  inst_iterator it = inst_begin(F);
  inst_iterator e = inst_end(F);
  for (; it != e; ++it) {
    calculate_dep(&*it);
  }

  // Recursively check if WI-dep changes and if so reclaculates
  // the WI-dep and marks the users for re-checking.
  // This procedure is guranteed to converge since WI-dep can only
  // become less unifrom (uniform->consecutive->ptr->stride->random).
  updateDeps();
  
  if (PrintWiaCheck) {
    print(dbgs());
  }
  return false;
}

void WIAnalysis::updateDeps() {
  // As lonst as we have values to update
  while (!m_pChangedNew->empty()) {
    // swap between changedSet pointers - recheck the newChanged(now old)
    std::swap(m_pChangedNew, m_pChangedOld);
    // clear the newChanged set so it will be filled with the users of
    // instruction which their WI-dep canged during the current iteration
    m_pChangedNew->clear();

    // update all changed values
    std::vector<const Value *>::iterator it = m_pChangedOld->begin();
    std::vector<const Value *>::iterator e = m_pChangedOld->end();
    for (; it != e; ++it) {
      // remove first instruction
      // calculate its new dependencey value
      calculate_dep(*it);
    }
  }
}

bool WIAnalysis::isInstructionSimple(const Instruction *inst) {
  // avoid changing cb load to sampler load, since sampler load
  // has longer latency.
  if (isa<LoadInst>(inst)) {
    return false;
  }

  if (isa<UnaryInstruction>(inst) || isa<BinaryOperator>(inst) ||
      isa<CmpInst>(inst) || isa<SelectInst>(inst)) {
    return true;
  }
  return false;
}

void WIAnalysis::initDependency(llvm::Function *pF) {
  llvm::Function::arg_iterator ai, ae;
  ai = pF->arg_begin();
  ae = pF->arg_end();

  // add all kernel function args as uniform
  for (; ai != ae; ++ai) {
    incUpdateDepend(ai, WIAnalysis::UNIFORM);
  }
}

bool WIAnalysis::validDepend(const llvm::Value *val) {
  return (m_deps.find(val) != m_deps.end());
}

WIAnalysis::WIDependancy WIAnalysis::whichDepend(const Value *val) {
  assert(m_pChangedNew->empty() && "set should be empty before query");
  assert(val && "Bad value");
  if (m_deps.find(val) == m_deps.end()) {
    // We expect all instructions in the map. Otherwise take the safe
    // way return random on release (assert on debug). For non-instruction
    // (arguments, constants) return uniform.
    bool isInst = isa<Instruction>(val);
    if (isInst) {
      return WIAnalysis::RANDOM;
    }
    return WIAnalysis::UNIFORM;
  }
  return m_deps[val];
}

bool WIAnalysis::stayUniformIfUsedAt(const Value *val, BasicBlock *use_blk) {
  const Instruction *inst = dyn_cast<Instruction>(val);
  // if it is a function argument, no problem to use it anywhere inside the
  // function
  if (!inst) {
    return true;
  }
  if (m_deps.find(inst) == m_deps.end()) {
    assert(0 && "trouble, don't have a record");
    return true;
  }
  if (m_deps[inst] != WIAnalysis::UNIFORM) {
    return true;
  }
  const BasicBlock *def_blk = inst->getParent();
  if (m_ctrlBranches.find(def_blk) == m_ctrlBranches.end()) {
    return true;
  }
  if (m_ctrlBranches.find(use_blk) != m_ctrlBranches.end()) {
    return false;
  }
  // every controlling branch of the def block has to be in the set of
  // controlling branches for the use-blk
  for (SmallPtrSet<const Instruction *, 4>::iterator
           I = m_ctrlBranches[def_blk].begin(),
           E = m_ctrlBranches[def_blk].end();
       I != E; ++I) {
    if (!m_ctrlBranches[use_blk].count(*I)) {
      return false;
    }
  }
  return true;
}

void WIAnalysis::invalidateDepend(const Value *val) {
  if (m_deps.find(val) != m_deps.end()) {
    m_deps.erase(val);
  }
}

bool WIAnalysis::isControlFlowUniform(const Function *F) {
  assert(F && "Bad Function");

  /// Place out-masks
  for (Function::const_iterator it = F->begin(), e = F->end(); it != e; ++it) {
    WIAnalysis::WIDependancy dep = whichDepend(it->getTerminator());
    if (dep != WIAnalysis::UNIFORM) {
      // Found a branch which diverges on the input
      return false;
    }
  }
  // All branches are uniform
  return true;
}

WIAnalysis::WIDependancy WIAnalysis::getDependency(const Value *val) {

  if (m_deps.find(val) == m_deps.end()) {
    // Make sure that constants are not added in the map.
    if (!isa<Instruction>(val)) {
      return WIAnalysis::UNIFORM;
    }
    // Don't expect this happens, let's assert in debug build!
    assert(false && "Dependence for 'val' should bave been set already!");
    m_deps[val] = WIAnalysis::UNIFORM;
  }
  return m_deps[val];
}

bool WIAnalysis::hasDependency(const Value *val) {

  if (!isa<Instruction>(val) && !isa<Argument>(val)) {
    return true;
  }
  return (m_deps.count(val) > 0);
}

void WIAnalysis::calculate_dep(const Value *val) {
  assert(val && "Bad value");

  // Not an instruction, must be a constant or an argument
  // Could this vector type be of a constant which
  // is not uniform ?
  assert(isa<Instruction>(val) &&
         "Could we reach here with non instruction value?");

  const Instruction *inst = dyn_cast<Instruction>(val);
  assert(inst && "This Value is not an Instruction");

  bool hasOriginal = hasDependency(inst);
  WIDependancy orig;
  // We only calculate dependency on unset instructions if all their operands
  // were already given dependency. This is good for compile time since these
  // instructions will be visited again after the operands dependency is set.
  // An exception are phi nodes since they can be the ancestor of themselves in
  // the def-use chain. Note that in this case we force the phi to have the
  // pre-header value already calculated.
  if (!hasOriginal) {
    unsigned int unsetOpNum = 0;
    for (unsigned i = 0; i < inst->getNumOperands(); ++i) {
      if (!hasDependency(inst->getOperand(i)))
        unsetOpNum++;
    }
    if (isa<PHINode>(inst)) {
      // We do not calculate PhiNode with all incoming values unset.
      //
      // This seems right as we don't expect a phi that only depends upon other
      // phi's (if it happens, those phis form a cycle dependency) so any phi's
      // calculation will eventually be triggered from calculating a non-phi one
      // which the phi depends upon.
      if (unsetOpNum == inst->getNumOperands())
        return;
    } else {
      // We do not calculate non-PhiNode instruction that have unset operands
      if (unsetOpNum > 0)
        return;
    }
    orig = WIAnalysis::UNIFORM;
  } else {
    orig = m_deps[inst];
    // if inst is already marked random, it cannot get better
    if (orig == WIAnalysis::RANDOM) {
      return;
    }
  }

  WIDependancy dep = orig;

  // LLVM does not have compile time polymorphisms
  // TODO: to make things faster we may want to sort the list below according
  // to the order of their probability of appearance.
  if (const BinaryOperator *BI = dyn_cast<BinaryOperator>(inst))
    dep = calculate_dep(BI);
  else if (const CallInst *CI = dyn_cast<CallInst>(inst))
    dep = calculate_dep(CI);
  else if (isa<CmpInst>(inst))
    dep = calculate_dep_simple(inst);
  else if (isa<ExtractElementInst>(inst))
    dep = calculate_dep_simple(inst);
  else if (const GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(inst))
    dep = calculate_dep(GEP);
  else if (isa<InsertElementInst>(inst))
    dep = calculate_dep_simple(inst);
  else if (isa<InsertValueInst>(inst))
    dep = calculate_dep_simple(inst);
  else if (const PHINode *Phi = dyn_cast<PHINode>(inst))
    dep = calculate_dep(Phi);
  else if (isa<ShuffleVectorInst>(inst))
    dep = calculate_dep_simple(inst);
  else if (isa<StoreInst>(inst))
    dep = RANDOM; // calculate_dep_simple(inst);
  else if (inst->isTerminator())
    dep = calculate_dep(inst);
  else if (const SelectInst *SI = dyn_cast<SelectInst>(inst))
    dep = calculate_dep(SI);
  else if (const AllocaInst *AI = dyn_cast<AllocaInst>(inst))
    dep = calculate_dep(AI);
  else if (const CastInst *CI = dyn_cast<CastInst>(inst))
    dep = calculate_dep(CI);
  else if (isa<ExtractValueInst>(inst))
    dep = calculate_dep_simple(inst);
  else if (const LoadInst *LI = dyn_cast<LoadInst>(inst))
    dep = calculate_dep(LI);
  else if (const VAArgInst *VAI = dyn_cast<VAArgInst>(inst))
    dep = calculate_dep(VAI);

  // If the value was changed in this calculation
  if (!hasOriginal || dep != orig) {
    // Save the new value of this instruction
    updateDepMap(inst, dep);
    // divergent branch, trigger updates due to control-dependence
    if (inst->isTerminator() && dep != WIAnalysis::UNIFORM) {
      update_cf_dep(inst);
    }
  }
}

void WIAnalysis::update_cf_dep(const Instruction *inst) {
  BasicBlock *blk = const_cast<BasicBlock *>(inst->getParent());
  BasicBlock *ipd = PDT->getNode(blk)->getIDom()->getBlock();
  // a branch can have NULL immediate post-dominator when a function
  // has multiple exits in llvm-ir
  // compute influence region and the partial-joins
  assert(inst->isTerminator() && "Expected terminator inst");
  BranchInfo br_info(cast<IGCLLVM::TerminatorInst>(inst), ipd);
  // debug: dump influence region and partial-joins
  // br_info.print(ods());

  // check dep-type for every phi in the full join
  if (ipd) {
    updatePHIDepAtJoin(ipd, &br_info);
  }
  // check dep-type for every phi in the partial-joins
  for (SmallPtrSet<BasicBlock *, 4>::iterator
           join_it = br_info.partial_joins.begin(),
           join_e = br_info.partial_joins.end();
       join_it != join_e; ++join_it) {
    updatePHIDepAtJoin(*join_it, &br_info);
  }

  // walk through all the instructions in the influence-region
  // update the dep-type based upon its uses
  DenseSet<BasicBlock *>::iterator blk_it = br_info.influence_region.begin();
  DenseSet<BasicBlock *>::iterator blk_e = br_info.influence_region.end();
  for (; blk_it != blk_e; ++blk_it) {
    BasicBlock *def_blk = *blk_it;
    // add the branch into the controlling-branch set of the block
    // if the block is in the influence-region, and not a partial join
    bool is_join = (br_info.partial_joins.count(def_blk) > 0);
    if (!is_join) {
      m_ctrlBranches[def_blk].insert(inst);
    }
    // An insight that can speed up the search process is that all the in-region
    // values that are used outside must dominate TI. Therefore, instead of
    // searching every basic blocks in the influence region, we only search the
    // dominators of the current branch
    if (def_blk != blk &&
        !DT->dominates(DT->getNode(def_blk), DT->getNode(blk))) {
      continue;
    }
    for (BasicBlock::iterator I = def_blk->begin(), E = def_blk->end(); I != E;
         ++I) {
      Instruction *defi = &(*I);
      if (hasDependency(defi) && getDependency(defi) == WIAnalysis::RANDOM) {
        continue;
      }
      // look at the uses
      Value::use_iterator use_it = defi->use_begin();
      Value::use_iterator use_e = defi->use_end();
      for (; use_it != use_e; ++use_it) {
        Instruction *user = dyn_cast<Instruction>((*use_it).getUser());
        assert(user);
        BasicBlock *user_blk = user->getParent();
        PHINode *phi = dyn_cast<PHINode>(user);
        if (phi) {
          // another place we assume all critical edges have been split and
          // phi-move will be placed on the blocks created on those
          user_blk = phi->getIncomingBlock(*use_it);
        }
        if (user_blk == def_blk) {
          // local def-use, not related to control-dependence
          continue; // skip
        }
        if (user_blk == br_info.full_join ||
            br_info.partial_joins.count(user_blk) ||
            !br_info.influence_region.count(user_blk)) {
          updateDepMap(defi, WIAnalysis::RANDOM);
          // break out of the use loop
          // since def is changed to RANDOM, all uses will be changed later
          break;
        }
      } // end of usei loop
    }   // end of defi loop within a block
  }     // end of influence-region block loop
}

void WIAnalysis::updatePHIDepAtJoin(BasicBlock *blk, BranchInfo *brInfo) {
  for (BasicBlock::iterator I = blk->begin(), E = blk->end(); I != E; ++I) {
    Instruction *defi = &(*I);
    PHINode *phi = dyn_cast<PHINode>(defi);
    if (!phi) {
      break;
    }
    if (hasDependency(phi) && getDependency(phi) == WIAnalysis::RANDOM) {
      continue;
    }
    Value *trickySrc = nullptr;
    for (unsigned predIdx = 0; predIdx < phi->getNumOperands(); ++predIdx) {
      Value *srcVal = phi->getOperand(predIdx);
      Instruction *defi = dyn_cast<Instruction>(srcVal);
      if (defi && brInfo->influence_region.count(defi->getParent())) {
        updateDepMap(phi, WIAnalysis::RANDOM);
        break;
      } else {
        // if the src is an immed, or an argument, or defined outside,
        // think about the phi-move that can be placed in the incoming block.
        // this phi should be random if we have two different src-values like
        // that. this is one place where we assume all critical edges have been
        // split
        BasicBlock *predBlk = phi->getIncomingBlock(predIdx);
        if (brInfo->influence_region.count(predBlk)) {
          if (!trickySrc) {
            trickySrc = srcVal;
          } else if (trickySrc != srcVal) {
            updateDepMap(phi, WIAnalysis::RANDOM);
            break;
          }
        }
      }
    }
  }
}

void WIAnalysis::updateDepMap(const Instruction *inst,
                              WIAnalysis::WIDependancy dep) {
  // Save the new value of this instruction
  m_deps[inst] = dep;
  // Register for update all of the dependent values of this updated
  // instruction.
  Value::const_user_iterator it = inst->user_begin();
  Value::const_user_iterator e = inst->user_end();
  for (; it != e; ++it) {
    m_pChangedNew->push_back(*it);
  }
}

WIAnalysis::WIDependancy
WIAnalysis::calculate_dep_simple(const Instruction *I) {
  // simply check that all operands are uniform, if so return uniform, else
  // random
  const unsigned nOps = I->getNumOperands();
  for (unsigned i = 0; i < nOps; ++i) {
    const Value *op = I->getOperand(i);
    WIAnalysis::WIDependancy dep = getDependency(op);
    if (dep != WIAnalysis::UNIFORM) {
      return WIAnalysis::RANDOM;
    }
  }
  return WIAnalysis::UNIFORM;
}

WIAnalysis::WIDependancy WIAnalysis::calculate_dep(const LoadInst *inst) {
  return calculate_dep_simple(inst);
}

WIAnalysis::WIDependancy WIAnalysis::calculate_dep(const BinaryOperator *inst) {
  // Calculate the dependency type for each of the operands
  Value *op0 = inst->getOperand(0);
  Value *op1 = inst->getOperand(1);

  WIAnalysis::WIDependancy dep0 = getDependency(op0);
  WIAnalysis::WIDependancy dep1 = getDependency(op1);

  // For whatever binary operation,
  // uniform returns uniform
  if (WIAnalysis::UNIFORM == dep0 && WIAnalysis::UNIFORM == dep1) {
    return WIAnalysis::UNIFORM;
  }

  // FIXME:: assumes that the X value does not cross the +/- border - risky !!!
  // The pattern (and (X, C)), where C preserves the lower k bits of the value,
  // is often used for truncating of numbers in 64bit. We assume that the index
  // properties are not hurt by this.
  if (inst->getOpcode() == Instruction::And) {
    ConstantInt *C0 = dyn_cast<ConstantInt>(inst->getOperand(0));
    ConstantInt *C1 = dyn_cast<ConstantInt>(inst->getOperand(1));
    // Use any of the constants. Instcombine places constants on Op1
    // so try Op1 first.
    if (C1 || C0) {
      ConstantInt *C = C1 ? C1 : C0;
      WIAnalysis::WIDependancy dep = C1 ? dep0 : dep1;
      // Cannot look at bit pattern of huge integers.
      if (C->getBitWidth() < 65) {
        uint64_t val = C->getZExtValue();
        uint64_t ptr_mask = (1 << MinIndexBitwidthToPreserve) - 1;
        // Zero all bits above the lower k bits that we are interested in
        val &= (ptr_mask);
        // Make sure that all of the remaining bits are active
        if (val == ptr_mask) {
          return dep;
        }
      }
    }
  }

  // FIXME:: assumes that the X value does not cross the +/- border - risky !!!
  // The pattern (ashr (shl X, C)C) is used for truncating of numbers in 64bit
  // The constant C must leave at least 32bits of the original number
  if (inst->getOpcode() == Instruction::AShr) {
    BinaryOperator *SHL = dyn_cast<BinaryOperator>(inst->getOperand(0));
    // We also allow add of uniform value between the ashr and shl instructions
    // since instcombine creates this pattern when adding a constant.
    // The shl forces all low bits to be zero, so there can be no carry to the
    // high bits due to the addition. Addition with uniform preservs WI-dep.
    if (SHL && SHL->getOpcode() == Instruction::Add) {
      Value *addedVal = SHL->getOperand(1);
      if (getDependency(addedVal) == WIAnalysis::UNIFORM) {
        SHL = dyn_cast<BinaryOperator>(SHL->getOperand(0));
      }
    }

    if (SHL && SHL->getOpcode() == Instruction::Shl) {
      ConstantInt *c_ashr = dyn_cast<ConstantInt>(inst->getOperand(1));
      ConstantInt *c_shl = dyn_cast<ConstantInt>(SHL->getOperand(1));
      const IntegerType *AshrTy = cast<IntegerType>(inst->getType());
      if (c_ashr && c_shl && c_ashr->getZExtValue() == c_shl->getZExtValue()) {
        // If wordWidth - shift_width >= 32 bits
        if ((AshrTy->getBitWidth() - c_shl->getZExtValue()) >=
            MinIndexBitwidthToPreserve) {
          // return the dep of the original X
          return getDependency(SHL->getOperand(0));
        }
      }
    }
  }

  if (dep0 == WIAnalysis::RANDOM || dep1 == WIAnalysis::RANDOM) {
    return WIAnalysis::RANDOM;
  }
  // stride computation
  switch (inst->getOpcode()) {
    // Addition simply adds the stride value, except for ptr_consecutive
    // which is promoted to strided.
    // Another exception is when we subtract the tid: 1 - X which turns the
    // tid order to random.
  case Instruction::Add: {
    int stride = dep0 + dep1;
    return clampDepend(stride);
  }
  case Instruction::Sub: {
    int stride = dep0 - dep1;
    return clampDepend(stride);
  }
  case Instruction::Mul:
    if (const ConstantInt* ConstOpnd = dyn_cast<ConstantInt>(op0)) {
      const int c = (int)ConstOpnd->getSExtValue();
      return clampDepend(c*dep1);
    }
    else if (const ConstantInt* ConstOpnd = dyn_cast<ConstantInt>(op1)) {
      const int c = (int)ConstOpnd->getSExtValue();
      return clampDepend(c*dep0);
    }
    break;
  case Instruction::Shl:
    if (const ConstantInt* ConstOpnd = dyn_cast<ConstantInt>(op1)) {
      const int c = (int)ConstOpnd->getSExtValue();
      return clampDepend(dep0<<c);
    }
    break;
  default:
    // TODO: Support more arithmetic if needed
    return WIAnalysis::RANDOM;
  }
  return WIAnalysis::RANDOM;
}

WIAnalysis::WIDependancy WIAnalysis::calculate_dep(const CallInst *inst) {
  if (Function *Callee = inst->getCalledFunction()) {
    switch (GenXIntrinsic::getGenXIntrinsicID(Callee)) {
    case GenXIntrinsic::genx_lane_id:
      return (WIAnalysis::WIDependancy)1;
    default:
      break;
    }
  }

  return WIAnalysis::RANDOM;
}

WIAnalysis::WIDependancy
WIAnalysis::calculate_dep(const GetElementPtrInst *inst) {
  // running over the all indices argumets except for the last
  // here we assume the pointer is the first operand
  unsigned num = inst->getNumIndices();
  for (unsigned i = 1; i < num; ++i) {
    const Value *op = inst->getOperand(i);
    WIAnalysis::WIDependancy dep = getDependency(op);
    if (dep != WIAnalysis::UNIFORM) {
      return WIAnalysis::RANDOM;
    }
  }
  const Value *opPtr = inst->getOperand(0);
  WIAnalysis::WIDependancy ptrDep = getDependency(opPtr);

  const Value *lastInd = inst->getOperand(num);
  WIAnalysis::WIDependancy lastIndDep = getDependency(lastInd);
  // \todo
  return clampDepend((int)ptrDep + (int)lastIndDep);
}

WIAnalysis::WIDependancy WIAnalysis::calculate_dep(const PHINode *inst) {
  unsigned num = inst->getNumIncomingValues();
  bool foundFirst = 0;
  WIDependancy totalDep;

  for (unsigned i = 0; i < num; ++i) {
    Value *op = inst->getIncomingValue(i);
    if (hasDependency(op)) {
      if (!foundFirst) {
        totalDep = getDependency(op);
      } else if (totalDep != getDependency(op)) {
        totalDep = WIAnalysis::RANDOM;
      }
      foundFirst = 1;
    }
  }

  assert(foundFirst &&
         "We should not reach here with All incoming values are unset");

  return totalDep;
}

WIAnalysis::WIDependancy WIAnalysis::calculate_dep(const Instruction *inst) {
  // Instruction has no return value
  // Just need to know if this inst is uniform or not
  // because we may want to avoid predication if the control flows
  // in the function are uniform...
  switch (inst->getOpcode()) {
  case Instruction::Br: {
    const BranchInst *brInst = cast<BranchInst>(inst);
    if (brInst->isConditional()) {
      // Conditional branch is uniform, if its condition is uniform
      Value *op = brInst->getCondition();
      WIAnalysis::WIDependancy dep = getDependency(op);
      if (WIAnalysis::UNIFORM == dep) {
        return WIAnalysis::UNIFORM;
      }
      return WIAnalysis::RANDOM;
    }
    // Unconditional branch is non TID-dependent
    return WIAnalysis::UNIFORM;
  }
  // Return instructions are unconditional
  case Instruction::Ret:
    return WIAnalysis::UNIFORM;
  case Instruction::Unreachable:
    return WIAnalysis::UNIFORM;
  case Instruction::IndirectBr:
    return WIAnalysis::RANDOM;
    // TODO: Define the dependency requirements of indirectBr
  case Instruction::Switch:
    return WIAnalysis::RANDOM;
    // TODO: Should this depend only on the condition, like branch?
  default:
    return WIAnalysis::RANDOM;
  }
}

WIAnalysis::WIDependancy WIAnalysis::calculate_dep(const SelectInst *inst) {
  Value *op0 = inst->getOperand(0); // mask
  WIAnalysis::WIDependancy dep0 = getDependency(op0);
  if (WIAnalysis::UNIFORM == dep0) {
    Value *op1 = inst->getOperand(1);
    Value *op2 = inst->getOperand(2);
    WIAnalysis::WIDependancy dep1 = getDependency(op1);
    WIAnalysis::WIDependancy dep2 = getDependency(op2);
    if (dep1 == dep2)
      return dep1;
  }
  return WIAnalysis::RANDOM;
}

WIAnalysis::WIDependancy WIAnalysis::calculate_dep(const AllocaInst *inst) {
  // \todo
  return WIAnalysis::RANDOM;
}

WIAnalysis::WIDependancy WIAnalysis::calculate_dep(const CastInst *inst) {
  Value *op0 = inst->getOperand(0);
  WIAnalysis::WIDependancy dep0 = getDependency(op0);

  // independent remains independent
  if (WIAnalysis::UNIFORM == dep0)
    return dep0;

  switch (inst->getOpcode()) {
  case Instruction::SExt:
  case Instruction::FPTrunc:
  case Instruction::FPExt:
  case Instruction::PtrToInt:
  case Instruction::IntToPtr:
  case Instruction::AddrSpaceCast:
  case Instruction::UIToFP:
  case Instruction::FPToUI:
  case Instruction::FPToSI:
  case Instruction::SIToFP:
    return dep0;
  case Instruction::BitCast:
  case Instruction::ZExt:
    return WIAnalysis::RANDOM;
    // FIXME:: assumes that the value does not cross the +/- border - risky !!!!
  case Instruction::Trunc: {
    const Type *destType = inst->getDestTy();
    const IntegerType *intType = dyn_cast<IntegerType>(destType);
    if (intType && (intType->getBitWidth() >= MinIndexBitwidthToPreserve)) {
      return dep0;
    }
    return WIAnalysis::RANDOM;
  }
  default:
    assert(false && "no such opcode");
    // never get here
    return WIAnalysis::RANDOM;
  }
}

WIAnalysis::WIDependancy WIAnalysis::calculate_dep(const VAArgInst *inst) {
  assert(false && "Are we supporting this ??");
  return WIAnalysis::RANDOM;
}

BranchInfo::BranchInfo(const IGCLLVM::TerminatorInst *inst, const BasicBlock *ipd)
    : cbr(inst), full_join(ipd) {
  assert(cbr == inst->getParent()->getTerminator() && "block terminator mismatch");
  assert(cbr->getNumSuccessors() == 2 && "only for cbr with two successors");

  std::set<BasicBlock *> f_set, t_set;
  std::stack<BasicBlock *> work_set;
  if (cbr->getSuccessor(0) != full_join) {
    work_set.push(cbr->getSuccessor(0));
    while (!work_set.empty()) {
      BasicBlock *cur_blk = work_set.top();
      work_set.pop();
      f_set.insert(cur_blk);
      influence_region.insert(cur_blk);
      for (succ_iterator SI = succ_begin(cur_blk), E = succ_end(cur_blk);
           SI != E; ++SI) {
        BasicBlock *succ_blk = (*SI);
        if (succ_blk != full_join && !f_set.count(succ_blk)) {
          work_set.push(succ_blk);
        }
      }
    }
  }
  if (cbr->getSuccessor(1) != full_join) {
    work_set.push(cbr->getSuccessor(1));
    while (!work_set.empty()) {
      BasicBlock *cur_blk = work_set.top();
      work_set.pop();
      t_set.insert(cur_blk);
      influence_region.insert(cur_blk);
      if (f_set.count(cur_blk)) {
        partial_joins.insert(cur_blk);
      }
      for (succ_iterator SI = succ_begin(cur_blk), E = succ_end(cur_blk);
           SI != E; ++SI) {
        BasicBlock *succ_blk = (*SI);
        if (succ_blk != full_join && !t_set.count(succ_blk)) {
          work_set.push(succ_blk);
        }
      }
    }
  }
}

void BranchInfo::print(raw_ostream &OS) const {
  OS << "\nCBR: " << *cbr;
  OS << "\nIPD: ";
  if (full_join) {
    full_join->print(OS);
  }
  OS << "\nPartial Joins:";
  SmallPtrSet<BasicBlock *, 4>::iterator join_it = partial_joins.begin();
  SmallPtrSet<BasicBlock *, 4>::iterator join_e = partial_joins.end();
  for (; join_it != join_e; ++join_it) {
    BasicBlock *cur_blk = *join_it;
    OS << "\n    ";
    cur_blk->print(OS);
  }
  OS << "\nInfluence Region:";
  DenseSet<BasicBlock *>::const_iterator blk_it = influence_region.begin();
  DenseSet<BasicBlock *>::const_iterator blk_e = influence_region.end();
  for (; blk_it != blk_e; ++blk_it) {
    BasicBlock *cur_blk = *blk_it;
    OS << "\n    ";
    cur_blk->print(OS);
  }
  OS << "\n";
}

char WIAnalysis::ID = 0; // LLVM uses address of ID as the actual ID.

FunctionPass *createWIAnalysisPass() { return new WIAnalysis(); }

} // end of namespace pktz

using namespace pktz;

#define PASS_FLAG "wi-analysis"
#define PASS_DESCRIPTION "WIAnalysis provides work item dependency info"
#define PASS_CFG_ONLY true
#define PASS_ANALYSIS true
INITIALIZE_PASS_BEGIN(WIAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                      PASS_ANALYSIS)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
INITIALIZE_PASS_END(WIAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                    PASS_ANALYSIS)
