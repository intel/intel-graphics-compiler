/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGC_OPTIONS_H
#define IGC_OPTIONS_H

#include <llvm/Option/OptTable.h>

namespace IGC {
namespace options {
// Flags should not overlap with llvm::opt::DriverFlag.
constexpr unsigned FirstNonBuiltinFlagNum = 4;

enum Flags {
  // Vector Compiler backend specific options.
  VectorCompilerApiOption = (1 << FirstNonBuiltinFlagNum),
  VectorCompilerInternalOption = (VectorCompilerApiOption << 1),
  IgcmcApiOption = (VectorCompilerInternalOption << 1),

  // Scalar Compiler backend specific options.
  ScalarCompilerApiOption = (IgcmcApiOption << 1),
  ScalarCompilerInternalOption = (ScalarCompilerApiOption << 1),
};

namespace api {
enum ID {
  OPT_INVALID = 0,
#define OPTION(PREFIX, NAME, ID, KIND, GROUP, ALIAS, ALIASARGS, FLAGS, PARAM,  \
               HELPTEXT, METAVAR, VALUES)                                      \
  OPT_##ID,
#include "igc/Options/ApiOptions.inc"
  LastOption
#undef OPTION
};
} // namespace api

namespace internal {
enum ID {
  OPT_INVALID = 0,
#define OPTION(PREFIX, NAME, ID, KIND, GROUP, ALIAS, ALIASARGS, FLAGS, PARAM,  \
               HELPTEXT, METAVAR, VALUES)                                      \
  OPT_##ID,
#include "igc/Options/InternalOptions.inc"
  LastOption
#undef OPTION
};
} // namespace internal

} // namespace options

const llvm::opt::OptTable &getApiOptTable();
const llvm::opt::OptTable &getInternalOptTable();
} // namespace IGC

#endif
