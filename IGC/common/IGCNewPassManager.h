/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include "common/LLVMWarningsPop.hpp"

#include "common/LLVMUtils.h"
#include "common/igc_regkeys.hpp"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace llvm {
class Module;
class Function;
class TargetLibraryInfoImpl;
} // namespace llvm

namespace IGC {
namespace IGCMD {
class MetaDataUtils;
}
class CodeGenContext;
struct ModuleMetaData;

namespace detail {
// Detects a static getPassName() (the pass's legacy display name) on an NPM pass wrapper.
template <typename T, typename = void> struct HasGetPassName : std::false_type {};
template <typename T> struct HasGetPassName<T, std::void_t<decltype(T::getPassName())>> : std::true_type {};

// Detects an NPM function pass: one whose run() takes (Function&, FunctionAnalysisManager&). Such a
// pass is wrapped in a module-to-function adaptor automatically by addPass().
template <typename T, typename = void> struct IsFunctionPass : std::false_type {};
template <typename T>
struct IsFunctionPass<T, std::void_t<decltype(std::declval<T &>().run(
                             std::declval<llvm::Function &>(), std::declval<llvm::FunctionAnalysisManager &>()))>>
    : std::true_type {};
} // namespace detail

// New Pass Manager analog of IGC's legacy IGCPassManager.
//
// It wires up the LLVM New Pass Manager analysis managers + PassBuilder, exposes
// the IGC context (CodeGenContext / MetaDataUtils / ModuleMetaData) to ported
// passes via the CodeGenContextAnalysis / MetaDataUtilsAnalysis analyses (seeded
// from the live pointers), and runs a ModulePassManager.
//
// optnone functions are respected via OptNoneInstrumentation. NPM function passes
// that must always run (the ported IGC passes, which never used the legacy
// skipFunction mechanism) opt out of being skipped by defining isRequired().
//
// Per-pass instrumentation from the legacy IGCPassManager is replicated: IR dumps
// (PrintBefore/PrintAfter/ShaderDumpEnableAll) and per-pass timing
// (DumpTimeStatsPerPass) are wired through PassInstrumentationCallbacks in the
// constructor, and per-pass skip handling (ShaderPassDisable, pass toggles, etc.)
// is applied at add time by addPass(). Passes added through
// createModuleToFunctionPassAdaptor(...) are dumped once per pass at the module
// level using the pass name supplied to addPass().
class IGCNewPassManager {
public:
  // If `tlii` is provided, a TargetLibraryAnalysis seeded with it is registered (matching a legacy
  // `new TargetLibraryInfoWrapperPass(TLI)`); otherwise the default TargetLibraryAnalysis is used.
  // The TargetLibraryInfoImpl must outlive run().
  IGCNewPassManager(CodeGenContext *ctx, const char *name = "", const llvm::TargetLibraryInfoImpl *tlii = nullptr);

  // Seed the IGC context analyses with the live pointers used by the pipeline.
  // Must be called before run() if any added pass needs the context.
  void registerContextAnalyses(CodeGenContext *ctx, IGCMD::MetaDataUtils *pMdUtils, ModuleMetaData *modMD);

  // Add a pass to the module pass manager. The pass identifies itself: its registration flag is
  // PassT::name() and (if it exposes one) its legacy display name is PassT::getPassName(). Both are
  // honored for the per-pass skip registry keys (ShaderPassDisable, pass toggles) and recorded as an
  // alias so the IR-dump callbacks (PrintBefore/PrintAfter) match either one — mirroring the legacy
  // IGCPassManager which matched getPassName() or the pass argument. A function pass (run takes a
  // Function&) is wrapped in a module-to-function adaptor automatically, so callers pass the pass
  // itself with no manual adaptor and no separately-spelled name.
  template <typename PassT> void addPass(PassT &&pass) {
    using DecayedPassT = std::decay_t<PassT>;
    const llvm::StringRef passName = DecayedPassT::name();
    llvm::StringRef displayName;
    if constexpr (detail::HasGetPassName<DecayedPassT>::value)
      displayName = DecayedPassT::getPassName();
    const bool hasDistinctDisplay = !displayName.empty() && displayName != passName;
    if (shouldSkipPassNewPM(m_pContext, passName, m_name) ||
        (hasDistinctDisplay && shouldSkipPassNewPM(m_pContext, displayName, m_name)))
      return;
    if (hasDistinctDisplay)
      m_passToggleAlias[std::string(passName)] = std::string(displayName);
    if constexpr (detail::IsFunctionPass<DecayedPassT>::value) {
      // Function pass: record its name so the adaptor's module-level dump callback can be remapped to
      // it, then wrap it in a module-to-function adaptor.
      m_ModuleToFunctionPassNames.emplace_back(passName.str());
      m_MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::forward<PassT>(pass)));
    } else {
      m_MPM.addPass(std::forward<PassT>(pass));
    }
  }

  // Direct access to the module pass manager (skips the per-pass skip handling).
  llvm::ModulePassManager &getMPM() { return m_MPM; }

  void run(llvm::Module &M);

private:
  CodeGenContext *const m_pContext;
  const std::string m_name;

  // Order matters: these must outlive the pass/analysis managers and run().
  llvm::PassInstrumentationCallbacks m_PIC;
  std::unique_ptr<llvm::OptNoneInstrumentation> m_OptNone;
  std::unique_ptr<llvm::PassBuilder> m_PB;

  llvm::LoopAnalysisManager m_LAM;
  llvm::FunctionAnalysisManager m_FAM;
  llvm::CGSCCAnalysisManager m_CGAM;
  llvm::ModuleAnalysisManager m_MAM;
  llvm::ModulePassManager m_MPM;

  std::vector<std::string> m_ModuleToFunctionPassNames;
  std::optional<std::string> m_ActiveModuleToFunctionPassName;
  size_t m_NextModuleToFunctionPassName = 0;

  // Maps a pass's registration flag (the dump/match PassID after any adaptor remap) to its legacy
  // display name, so PrintBefore/PrintAfter and skip toggles accept either, like the legacy manager.
  std::map<std::string, std::string> m_passToggleAlias;

  // Returns the alias toggle name for a (possibly remapped) PassID, or empty if there is none.
  llvm::StringRef toggleAliasFor(llvm::StringRef passID) const {
    auto it = m_passToggleAlias.find(std::string(passID));
    return it == m_passToggleAlias.end() ? llvm::StringRef() : llvm::StringRef(it->second);
  }

  std::string remapPassIDBefore(llvm::StringRef passID, llvm::Any IR);
  std::string remapPassIDAfter(llvm::StringRef passID, llvm::Any IR);
};
} // namespace IGC

// Per-pass macro to dispatch between New Pass Manager and Legacy Pass Manager.
// Adds a pass to the appropriate pass manager based on LLVM version and the
// EnableOCLNewPassManager runtime flag.
//
// Parameters:
//   npmPM - IGCNewPassManager instance (only referenced when LLVM >= 16)
//   lpmPM - IGCPassManager instance
//   npmPass - NPM pass expression (e.g. MyPassNPM()); names itself via MyPassNPM::name()/getPassName(),
//             and function passes are wrapped in a module adaptor automatically (no manual adaptor)
//   lpmPass - LPM pass expression (e.g. new MyPassLPM())
//
// Usage:
//   IGC_ADD_PASS(npmPM, lpmPM, MyPassNPM(), new MyPassLPM());
//   IGC_RUN_PM(npmPM, lpmPM, *pContext->getModule());
#if LLVM_VERSION_MAJOR >= 16
#define IGC_ADD_PASS(npmPM, lpmPM, npmPass, lpmPass)                                                                   \
  do {                                                                                                                 \
    if (IGC_IS_FLAG_ENABLED(EnableOCLNewPassManager))                                                                  \
      (npmPM).addPass(npmPass);                                                                                        \
    else                                                                                                               \
      (lpmPM).add(lpmPass);                                                                                            \
  } while (0)

#define IGC_RUN_PM(npmPM, lpmPM, module)                                                                               \
  do {                                                                                                                 \
    if (IGC_IS_FLAG_ENABLED(EnableOCLNewPassManager))                                                                  \
      (npmPM).run(module);                                                                                             \
    else                                                                                                               \
      (lpmPM).run(module);                                                                                             \
  } while (0)
#else
#define IGC_ADD_PASS(npmPM, lpmPM, npmPass, lpmPass) (lpmPM).add(lpmPass)

#define IGC_RUN_PM(npmPM, lpmPM, module) (lpmPM).run(module)
#endif

// Convenience macro for passes that follow the common naming convention where
// the NPM pass is PassNameNPM() and the LPM pass is new PassNameLPM().
// Only the base pass name needs to be specified once.
//
// Usage:
//   IGC_ADD_PASS_AUTO(npm, lpm, ResolveInlineSamplerForBindless);
// expands to:
//   IGC_ADD_PASS(npm, lpm, ResolveInlineSamplerForBindlessNPM(), new ResolveInlineSamplerForBindlessLPM());
#define IGC_ADD_PASS_AUTO(npmPM, lpmPM, PassClass) IGC_ADD_PASS(npmPM, lpmPM, PassClass##NPM(), new PassClass##LPM())
