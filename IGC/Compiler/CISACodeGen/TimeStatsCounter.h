/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CodeGenPublic.h"
#include "common/Stats.hpp"
#include <string>

namespace IGC {
enum TimeStatsCounterStartEndMode { STATS_COUNTER_START, STATS_COUNTER_END };

enum TimeStatsCounterType { STATS_COUNTER_LLVM_PASS, STATS_COUNTER_ENUM_TYPE };

// Shared implementation. Holds the timing configuration and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass. run() is defined in the .cpp
// where the COMPILER_TIME_* macros are available.
class TimeStatsCounter {
  CodeGenContext *ctx = nullptr;
  COMPILE_TIME_INTERVALS interval{};
  TimeStatsCounterStartEndMode mode{};
  std::string igcPass{};
  TimeStatsCounterType type{};

public:
  TimeStatsCounter() {}
  TimeStatsCounter(CodeGenContext *_ctx, COMPILE_TIME_INTERVALS _interval, TimeStatsCounterStartEndMode _mode)
      : ctx(_ctx), interval(_interval), mode(_mode), type(STATS_COUNTER_ENUM_TYPE) {}
  TimeStatsCounter(CodeGenContext *_ctx, const std::string &_igcPass, TimeStatsCounterStartEndMode _mode)
      : ctx(_ctx), mode(_mode), igcPass(_igcPass), type(STATS_COUNTER_LLVM_PASS) {}

  bool run();
};

llvm::ModulePass *createTimeStatsCounterPass(CodeGenContext *_ctx, COMPILE_TIME_INTERVALS _interval,
                                             TimeStatsCounterStartEndMode _mode);
llvm::ModulePass *createTimeStatsIGCPass(CodeGenContext *_ctx, std::string _igcPass,
                                         TimeStatsCounterStartEndMode _mode);
void initializeTimeStatsCounterLPMPass(llvm::PassRegistry &);

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class TimeStatsCounterNPM : public llvm::PassInfoMixin<TimeStatsCounterNPM> {
  TimeStatsCounter m_impl;

public:
  TimeStatsCounterNPM(CodeGenContext *_ctx, COMPILE_TIME_INTERVALS _interval, TimeStatsCounterStartEndMode _mode)
      : m_impl(_ctx, _interval, _mode) {}
  TimeStatsCounterNPM(CodeGenContext *_ctx, const std::string &_igcPass, TimeStatsCounterStartEndMode _mode)
      : m_impl(_ctx, _igcPass, _mode) {}

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "time-stats-counter"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // End namespace IGC
