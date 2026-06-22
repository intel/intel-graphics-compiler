/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/IGCNewPassManager.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/TimeStatsCounter.h"
#include "common/debug/Dump.hpp"
#include "common/Stats.hpp"
#include "common/igc_regkeys.hpp"
#include "llvmWrapper/ADT/StringRef.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include "common/LLVMWarningsPop.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>

using namespace llvm;
using namespace IGC;

#if LLVM_VERSION_MAJOR >= 16

namespace {

// Strip non-alphanumeric characters from a pass name (mirrors IGCPassManager).
std::string igcCleanPassName(StringRef passName) {
  std::string newName;
  std::copy_if(passName.begin(), passName.end(), std::back_inserter(newName),
               [](unsigned char c) { return std::isalnum(c); });
  return newName;
}

// True if N is in the comma/semicolon-separated List (mirrors IGCPassManager::isInList).
bool igcIsInList(StringRef N, StringRef List) {
  StringRef Separators(",;");
  size_t startPos = 0;
  while (startPos != StringRef::npos) {
    size_t endPos = List.find_first_of(Separators, startPos);
    size_t len = (endPos != StringRef::npos ? endPos - startPos : endPos);
    StringRef Name = List.substr(startPos, len);
    if (IGCLLVM::equals_insensitive(igcCleanPassName(Name), igcCleanPassName(N)))
      return true;
    startPos = (endPos != StringRef::npos ? endPos + 1 : StringRef::npos);
  }
  return false;
}

bool igcIsPrintBefore(StringRef PN) {
  if (IGC_IS_FLAG_ENABLED(PrintBefore)) {
    StringRef passNameList(IGC_GET_REGKEYSTRING(PrintBefore));
    if (IGCLLVM::equals_insensitive(passNameList, "all") || igcIsInList(PN, passNameList))
      return true;
  }
  return false;
}

bool igcIsPrintAfter(StringRef PN) {
  if (IGC_IS_FLAG_ENABLED(ShaderDumpEnableAll))
    return true;
  if (IGC_IS_FLAG_ENABLED(PrintAfter)) {
    StringRef passNameList(IGC_GET_REGKEYSTRING(PrintAfter));
    if (IGCLLVM::equals_insensitive(passNameList, "all") || igcIsInList(PN, passNameList))
      return true;
  }
  return false;
}

// Dump callbacks operate on module IR. Function passes run via a module adaptor and
// reuse the adaptor's module callback, remapped to the configured inner pass name.
const Module *igcUnwrapModule(const Any &IR) {
  if (const Module *const *M = any_cast<const Module *>(&IR))
    return *M;
  return nullptr;
}

// Dump the module IR to an IGC dump (file, or console when PrintToConsole is set),
// mirroring IGCPassManager::addPrintPass. IGC metadata is serialized into the module
// first so it appears in the dump, matching SerializePrintMetaDataPass.
void igcDumpModuleIR(CodeGenContext *ctx, StringRef pmName, const Module &M, StringRef passName, bool isBefore) {
  std::string fullPassName = (pmName + (isBefore ? "_before_" : "_after_")).str() + igcCleanPassName(passName);

  auto name = Debug::DumpName(Debug::GetShaderOutputName())
                  .ShaderName(ctx->shaderName)
                  .Type(ctx->type)
                  .Hash(ctx->hash)
                  .Pass(fullPassName, std::optional<uint32_t>(ctx->m_numPasses++))
                  .StagedInfo(ctx)
                  .Extension("ll");

  if (!name.allow())
    return;

  Debug::Dump dump(name, Debug::DumpType::PASS_IR_TEXT);

  // The callback hands us a const module, but serializing metadata (as the legacy
  // SerializePrintMetaDataPass does) mutates it. This matches the legacy behavior.
  Module &MM = const_cast<Module &>(M);
  ctx->getMetaDataUtils()->save(MM.getContext());
  serialize(*ctx->getModuleMetaData(), &MM);
  MM.print(dump.stream(), nullptr);
}

} // namespace

std::string IGCNewPassManager::remapPassIDBefore(StringRef passID, const Any &IR) {
  if (passID != "ModuleToFunctionPassAdaptor" || !igcUnwrapModule(IR))
    return passID.str();

  if (m_NextModuleToFunctionPassName >= m_ModuleToFunctionPassNames.size())
    return passID.str();

  m_ActiveModuleToFunctionPassName = m_ModuleToFunctionPassNames[m_NextModuleToFunctionPassName++];
  return *m_ActiveModuleToFunctionPassName;
}

std::string IGCNewPassManager::remapPassIDAfter(StringRef passID, const Any &IR) {
  if (passID != "ModuleToFunctionPassAdaptor" || !igcUnwrapModule(IR) || !m_ActiveModuleToFunctionPassName)
    return passID.str();

  std::string remappedPassID = *m_ActiveModuleToFunctionPassName;
  m_ActiveModuleToFunctionPassName.reset();
  return remappedPassID;
}

IGCNewPassManager::IGCNewPassManager(CodeGenContext *ctx, const char *name, const llvm::TargetLibraryInfoImpl *tlii)
    : m_pContext(ctx), m_name(name) {
  // Respect optnone functions, mirroring the legacy pass manager's skipFunction
  // behavior. The ported IGC passes opt out (isRequired) so that, like the legacy
  // path, they always run; only "optimization" passes (e.g. DCE) are skipped.
  m_OptNone = std::make_unique<OptNoneInstrumentation>(/*DebugLogging=*/false);
  m_OptNone->registerCallbacks(m_PIC);

  m_PB = std::make_unique<PassBuilder>(/*TM=*/nullptr, PipelineTuningOptions(), /*PGOOpt=*/std::nullopt, &m_PIC);

  // Register the default AA pipeline (BasicAA + TBAA + ScopedNoAlias + ...) BEFORE
  // registerFunctionAnalyses so that our AAManager is the one used (registerFunctionAnalyses
  // would otherwise register a default, empty AAManager and the first registration wins).
  // This matches the AA available in the legacy OCL Unify pipeline, where AAResultsWrapperPass
  // pulls in the standard AA passes for consumers such as GASResolving.
  m_FAM.registerPass([this] { return m_PB->buildDefaultAAPipeline(); });

  // If a configured TargetLibraryInfoImpl was provided (e.g. the OCL Unify pipeline disables all
  // library functions), register a TargetLibraryAnalysis seeded with it BEFORE registerFunctionAnalyses
  // (first-wins) so library-aware passes (InstCombine, the inliner, ...) match the legacy behavior.
  // The TargetLibraryInfoImpl is owned by the caller and outlives run().
  if (tlii)
    m_FAM.registerPass([tlii] { return llvm::TargetLibraryAnalysis(*tlii); });

  // Register the standard analyses (this also auto-registers TargetLibraryAnalysis
  // and the PassInstrumentationAnalysis carrying m_PIC) and cross-register proxies
  // so that, e.g., a function pass can reach module-level analyses.
  m_PB->registerModuleAnalyses(m_MAM);
  m_PB->registerCGSCCAnalyses(m_CGAM);
  m_PB->registerFunctionAnalyses(m_FAM);
  m_PB->registerLoopAnalyses(m_LAM);
  m_PB->crossRegisterProxies(m_LAM, m_FAM, m_CGAM, m_MAM);

  // Per-pass instrumentation: IR dumps (PrintBefore/PrintAfter/ShaderDumpEnableAll)
  // and per-pass timing (DumpTimeStatsPerPass), matching IGCPassManager::add.
  CodeGenContext *pCtx = m_pContext;
  const std::string pmName = m_name;

  m_PIC.registerBeforeNonSkippedPassCallback([this, pCtx, pmName](StringRef PassID, Any IR) {
    const Module *M = igcUnwrapModule(IR);
    if (!M)
      return;
    std::string RemappedPassID = remapPassIDBefore(PassID, IR);
    StringRef FinalPassID(RemappedPassID);
    if (IGC_REGKEY_OR_FLAG_ENABLED(DumpTimeStatsPerPass, TIME_STATS_PER_PASS))
      COMPILER_TIME_PASS_START(pCtx, pmName + "_" + FinalPassID.str());
    StringRef AliasID = toggleAliasFor(FinalPassID);
    if (igcIsPrintBefore(FinalPassID) || (!AliasID.empty() && igcIsPrintBefore(AliasID)))
      igcDumpModuleIR(pCtx, pmName, *M, FinalPassID, /*isBefore=*/true);
  });

  m_PIC.registerAfterPassCallback([this, pCtx, pmName](StringRef PassID, Any IR, const PreservedAnalyses &) {
    const Module *M = igcUnwrapModule(IR);
    if (!M)
      return;
    std::string RemappedPassID = remapPassIDAfter(PassID, IR);
    StringRef FinalPassID(RemappedPassID);
    StringRef AliasID = toggleAliasFor(FinalPassID);
    if (igcIsPrintAfter(FinalPassID) || (!AliasID.empty() && igcIsPrintAfter(AliasID)))
      igcDumpModuleIR(pCtx, pmName, *M, FinalPassID, /*isBefore=*/false);
    if (IGC_REGKEY_OR_FLAG_ENABLED(DumpTimeStatsPerPass, TIME_STATS_PER_PASS))
      COMPILER_TIME_PASS_END(pCtx, pmName + "_" + FinalPassID.str());
  });
}

void IGCNewPassManager::registerContextAnalyses(CodeGenContext *ctx, IGCMD::MetaDataUtils *pMdUtils,
                                                ModuleMetaData *modMD) {
  // Seed the IGC context analyses with the live pointers. Captured by value
  // (pointers), which is safe: they are owned by the CodeGenContext and outlive
  // run(). The analyses do not compute anything from the module.
  m_MAM.registerPass([ctx] { return CodeGenContextAnalysis(ctx); });
  m_MAM.registerPass([pMdUtils, modMD] { return MetaDataUtilsAnalysis(pMdUtils, modMD); });
}

void IGCNewPassManager::run(Module &M) { m_MPM.run(M, m_MAM); }

#else

IGCNewPassManager::IGCNewPassManager(CodeGenContext *ctx, const char *name, const llvm::TargetLibraryInfoImpl *tlii)
    : m_pContext(ctx), m_name(name) {
  (void)tlii;
}

void IGCNewPassManager::registerContextAnalyses(CodeGenContext *ctx, IGCMD::MetaDataUtils *pMdUtils,
                                                ModuleMetaData *modMD) {
  (void)ctx;
  (void)pMdUtils;
  (void)modMD;
}

void IGCNewPassManager::run(Module &M) { (void)M; }

#endif
