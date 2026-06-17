/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/TimeStatsCounter.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

namespace {
// Legacy Pass Manager wrapper.
class TimeStatsCounterLPM : public ModulePass {
public:
  static char ID;

  TimeStatsCounterLPM() : ModulePass(ID) { initializeTimeStatsCounterLPMPass(*PassRegistry::getPassRegistry()); }

  TimeStatsCounterLPM(CodeGenContext *_ctx, COMPILE_TIME_INTERVALS _interval, TimeStatsCounterStartEndMode _mode)
      : ModulePass(ID), m_impl(_ctx, _interval, _mode) {
    initializeTimeStatsCounterLPMPass(*PassRegistry::getPassRegistry());
  }

  TimeStatsCounterLPM(CodeGenContext *_ctx, const std::string &_igcPass, TimeStatsCounterStartEndMode _mode)
      : ModulePass(ID), m_impl(_ctx, _igcPass, _mode) {
    initializeTimeStatsCounterLPMPass(*PassRegistry::getPassRegistry());
  }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.setPreservesAll(); }

  bool runOnModule(Module &) override { return m_impl.run(); }

private:
  TimeStatsCounter m_impl;
};
} // End anonymous namespace

ModulePass *IGC::createTimeStatsCounterPass(CodeGenContext *_ctx, COMPILE_TIME_INTERVALS _interval,
                                            TimeStatsCounterStartEndMode _mode) {
  return new TimeStatsCounterLPM(_ctx, _interval, _mode);
}

ModulePass *IGC::createTimeStatsIGCPass(CodeGenContext *_ctx, std::string _igcPass,
                                        TimeStatsCounterStartEndMode _mode) {
  return new TimeStatsCounterLPM(_ctx, _igcPass, _mode);
}

char TimeStatsCounterLPM::ID = 0;

#define PASS_FLAG "time-stats-counter"
#define PASS_DESC "TimeStatsCounter Start/Stop"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
namespace IGC {
IGC_INITIALIZE_PASS_BEGIN(TimeStatsCounterLPM, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(TimeStatsCounterLPM, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
} // namespace IGC

bool TimeStatsCounter::run() {
  if (type == STATS_COUNTER_ENUM_TYPE) {
    if (mode == STATS_COUNTER_START) {
      COMPILER_TIME_START(ctx, interval);
    } else {
      COMPILER_TIME_END(ctx, interval);
    }
  } else {
    if (mode == STATS_COUNTER_START) {
      COMPILER_TIME_PASS_START(ctx, igcPass);
    } else {
      COMPILER_TIME_PASS_END(ctx, igcPass);
    }
  }
  return false;
}

#if LLVM_VERSION_MAJOR >= 16
llvm::PreservedAnalyses IGC::TimeStatsCounterNPM::run(llvm::Module &M, llvm::ModuleAnalysisManager &AM) {
  m_impl.run();
  return llvm::PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16
