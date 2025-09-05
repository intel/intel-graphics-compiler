/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_OPTION_OPTTABLE_H
#define IGCLLVM_OPTION_OPTTABLE_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Option/OptTable.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/ADT/ArrayRef.h>

namespace IGCLLVM {
using GenericOptTable =
#if LLVM_VERSION_MAJOR >= 16
    llvm::opt::GenericOptTable;
#else  // LLVM_VERSION_MAJOR
    llvm::opt::OptTable;
#endif // LLVM_VERSION_MAJOR

inline void printHelp(const llvm::opt::OptTable &Options, llvm::raw_ostream &OS, const char *Usage, const char *Title,
                      unsigned FlagsToInclude, unsigned FlagsToExclude, bool ShowAllAliases) {
  Options.printHelp(OS, Usage, Title, FlagsToInclude, FlagsToExclude, ShowAllAliases);
}

inline void printHelp(const llvm::opt::OptTable &Options, llvm::raw_ostream &OS, const char *Usage, const char *Title,
                      bool ShowHidden = false, bool ShowAllAliases = false) {
  Options.printHelp(OS, Usage, Title, ShowHidden, ShowAllAliases);
}
} // namespace IGCLLVM

#endif
