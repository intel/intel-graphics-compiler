/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_OPTION_OPTTABLE_H
#define IGCLLVM_OPTION_OPTTABLE_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Option/OptTable.h"
#include "llvm/Support/raw_ostream.h"

namespace IGCLLVM {
inline void printHelp(const llvm::opt::OptTable &Options, llvm::raw_ostream &OS,
                      const char *Usage, const char *Title,
                      unsigned FlagsToInclude, unsigned FlagsToExclude,
                      bool ShowAllAliases) {
  Options.
#if LLVM_VERSION_MAJOR <= 12
      PrintHelp
#else
      printHelp
#endif
      (OS, Usage, Title, FlagsToInclude, FlagsToExclude, ShowAllAliases);
}

inline void printHelp(const llvm::opt::OptTable &Options, llvm::raw_ostream &OS,
                      const char *Usage, const char *Title,
                      bool ShowHidden = false, bool ShowAllAliases = false) {
  Options.
#if LLVM_VERSION_MAJOR <= 12
      PrintHelp
#else
      printHelp
#endif
      (OS, Usage, Title, ShowHidden, ShowAllAliases);
}
} // namespace IGCLLVM

#endif
