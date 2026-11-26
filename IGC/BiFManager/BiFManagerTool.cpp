/*========================== begin_copyright_notice ============================

Copyright (C) 2024-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/LLVMContext.h>
#include "llvmWrapper/Transforms/IPO/GlobalDCE.h"
#include "llvmWrapper/Transforms/IPO/StripSymbols.h"
#include "llvmWrapper/Transforms/IPO/StripDeadPrototypes.h"
#include "llvmWrapper/ADT/StringRef.h"
#include "BiFManagerTool.hpp"

#include <string>

#include <Probe/Assertion.h>

using namespace std;
using namespace llvm;
using namespace IGC;
using namespace IGC::BiFManager;

BiFManagerTool::BiFManagerTool(LLVMContext &Context) : BiFManagerCommon(Context) {}

BiFManagerTool::~BiFManagerTool() {}

std::unique_ptr<Module> BiFManagerTool::loadModule(const std::string &Path, LLVMContext &ModuleContext) {
  ErrorOr<std::unique_ptr<MemoryBuffer>> FileOrErr = MemoryBuffer::getFile(Path);
  if (!FileOrErr) {
    printf("[BiFManager] - Unable to read %s\n", Path.c_str());
    return nullptr;
  }
  Expected<std::unique_ptr<Module>> M = parseBitcodeFile((*FileOrErr)->getMemBufferRef(), ModuleContext);
  if (!M) {
    std::string Message = toString(M.takeError());
    printf("[BiFManager] - Unable to parse %s: %s\n", Path.c_str(), Message.c_str());
    return nullptr;
  }

  llvm::legacy::PassManager mpm;
  mpm.add(IGCLLVM::createLegacyWrappedGlobalDCEPass());           // Delete unreachable globals.
  mpm.add(IGCLLVM::createLegacyWrappedStripDeadDebugInfoPass());  // Remove dead debug info.
  mpm.add(IGCLLVM::createLegacyWrappedStripDeadPrototypesPass()); // Remove dead func decls.
  mpm.run(**M);
  return std::move(*M);
}

static void writeJSONString(raw_ostream &OS, StringRef S) {
  OS << '"';
  for (char C : S) {
    if (C == '"' || C == '\\')
      OS << '\\' << C;
    else if ((unsigned char)C < 0x20)
      OS << "\\u00" << "0123456789abcdef"[(unsigned char)C >> 4] << "0123456789abcdef"[C & 0xf];
    else
      OS << C;
  }
  OS << '"';
}

static void writeJSONList(raw_ostream &OS, StringRef Key, const std::vector<StringRef> &Names) {
  OS << ",\n   ";
  writeJSONString(OS, Key);
  OS << ": [";
  for (size_t i = 0; i < Names.size(); ++i) {
    OS << (i ? ", " : "");
    writeJSONString(OS, Names[i]);
  }
  OS << "]";
}

bool BiFManagerTool::WriteSymbols(const std::vector<std::string> &MainModules,
                                  const std::vector<std::string> &Size32Modules,
                                  const std::vector<std::string> &Size64Modules, const StringRef SymbolsPath) {
  std::error_code EC;
  raw_fd_ostream OS(SymbolsPath.str(), EC, sys::fs::OF_Text);
  if (EC) {
    printf("[BiFManager] - Unable to write %s\n", SymbolsPath.str().c_str());
    return false;
  }

  const std::pair<const char *, const std::vector<std::string> *> Kinds[] = {
      {"main", &MainModules}, {"size32", &Size32Modules}, {"size64", &Size64Modules}};
  bool First = true;
  OS << "{\"modules\": [";
  for (const auto &Kind : Kinds) {
    for (const auto &Path : *Kind.second) {
      // A context per module frees the memory of its types and constants too.
      LLVMContext ModuleContext;
      IGCLLVM::setOpaquePointers(&ModuleContext);
      std::unique_ptr<Module> M = loadModule(Path, ModuleContext);
      if (!M)
        return false;

      std::vector<StringRef> DefinedFuncs, DeclaredFuncs, DefinedGlobals, DeclaredGlobals;
      for (auto &F : M->functions()) {
        if (!F.isDeclaration() && !F.hasLocalLinkage())
          DefinedFuncs.push_back(F.getName());
        else if (F.isDeclaration() && !F.isIntrinsic())
          DeclaredFuncs.push_back(F.getName());
      }
      for (auto &G : M->globals()) {
        if (!G.isDeclaration() && !G.hasLocalLinkage())
          DefinedGlobals.push_back(G.getName());
        else if (G.isDeclaration())
          DeclaredGlobals.push_back(G.getName());
      }

      OS << (First ? "\n  {" : ",\n  {");
      First = false;
      writeJSONString(OS, "path");
      OS << ": ";
      writeJSONString(OS, Path);
      OS << ", ";
      writeJSONString(OS, "kind");
      OS << ": ";
      writeJSONString(OS, Kind.first);
      writeJSONList(OS, "defined_functions", DefinedFuncs);
      writeJSONList(OS, "declared_functions", DeclaredFuncs);
      writeJSONList(OS, "defined_globals", DefinedGlobals);
      writeJSONList(OS, "declared_globals", DeclaredGlobals);
      OS << "}";
    }
  }
  OS << "\n]}\n";
  printf("[BiFManager] - Listed the symbols of %d modules\n",
         (int)(MainModules.size() + Size32Modules.size() + Size64Modules.size()));
  return true;
}

void BiFManagerTool::markBuiltinFunc(llvm::Module *pM) {
  for (auto &f : pM->functions()) {
    LLVMContext &C = f.getContext();
    MDNode *N = MDNode::get(C, MDString::get(C, "IGC built-in function"));

    f.setMetadata(bifMark, N);
  }

  for (auto &g : pM->globals()) {
    LLVMContext &C = g.getContext();
    MDNode *N = MDNode::get(C, MDString::get(C, "IGC global variable from built-in function"));

    g.setMetadata(bifMark, N);
  }
}

// User SPIR-V globals can have the same names as local BiF globals, e.g. the
// `.str` string literals. Prefix the local BiF globals, so that linking never
// renames a user global. Reserved names can't come from user code and are kept as is.
static void prefixLocalGlobals(llvm::Module &M) {
  for (auto &G : M.globals())
    if (G.hasLocalLinkage() && !IGCLLVM::starts_with(G.getName(), "__"))
      G.setName("__bif" + G.getName());
}

bool BiFManagerTool::MakeBiFPackage(const std::vector<std::string> &Modules, const StringSet<> &SharedGlobals) {
  for (const auto &Path : Modules) {
    LLVMContext ModuleContext;
    IGCLLVM::setOpaquePointers(&ModuleContext);
    std::unique_ptr<Module> M = loadModule(Path, ModuleContext);
    if (!M)
      return false;

    // Mark all built-in functions and global variables
    markBuiltinFunc(M.get());

    prefixLocalGlobals(*M);

    // Make the symbols linkonce, except the local ones and the globals used by other
    // modules, which the handler would erase as unused before linking.
    for (auto &F : M->functions()) {
      if (!F.hasLocalLinkage())
        F.setLinkage(GlobalValue::LinkOnceAnyLinkage);
    }
    for (auto &G : M->globals()) {
      if (!G.hasLocalLinkage() && !SharedGlobals.count(G.getName()))
        G.setLinkage(GlobalValue::LinkOnceAnyLinkage);
    }


    BiFModules.emplace_back();
    BitcodeWriter Writer(BiFModules.back());
    Writer.writeModule(*M, false, nullptr, false, nullptr);
    Writer.writeSymtab();
    Writer.writeStrtab();
  }
  printf("[BiFManager] - Packed %d modules\n", (int)BiFModules.size());
  return true;
}

void BiFManagerTool::WriteBitcode(const llvm::StringRef BitCodePath) {
  std::error_code EC;
  long currentBufferOffset = 0;
  raw_fd_ostream fileData(BitCodePath.str(), EC, sys::fs::FA_Write);
  std::vector<BiFDataRecord> listOfFilePtr;

  printf("[BiFManager] - Start writing bitcode for modules\n");
  for (size_t module = 0; module < BiFModules.size(); ++module) {
    listOfFilePtr.push_back(BiFDataRecord((BiFSectionID)module, currentBufferOffset, BiFModules[module].size()));
    currentBufferOffset += BiFModules[module].size();
  }

  // Entry record keeps offset where first module starts in buffor
  BiFDataRecord entryRecord(-1, 0, sizeof(BiFDataRecord) * (BiFModules.size() + 1));

  // Write entry record
  fileData.write((char *)&entryRecord, sizeof(BiFDataRecord));

  for (unsigned i = 0; i < BiFModules.size(); ++i) {
    // Write records for ptr module in buffer
    fileData.write((char *)&listOfFilePtr[i], sizeof(BiFDataRecord));
  }

  for (unsigned i = 0; i < BiFModules.size(); ++i) {
    // Write modules into stream
    fileData.write(BiFModules[i].data(), BiFModules[i].size());
  }

  printf("[BiFManager] - End of writing bifbc bitcode\n");
}
