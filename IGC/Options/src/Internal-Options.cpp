/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

This software and the related documents are Intel copyrighted materials,
and your use of them is governed by the express license under which they were
provided to you ("License"). Unless the License provides otherwise,
you may not use, modify, copy, publish, distribute, disclose or transmit this
software or the related documents without Intel's prior written permission.

This software and the related documents are provided as is, with no express or
implied warranties, other than those that are expressly stated in the License.

SPDX-License-Identifier: MIT
============================= end_copyright_notice ===========================*/

#include "igc/Options/Options.h"
#include <llvm/Option/Option.h>
#include <llvmWrapper/Option/OptTable.h>

using namespace IGC::options;
using namespace llvm::opt;
using llvm::raw_ostream;

#if LLVM_VERSION_MAJOR >= 16
#define PREFIX(NAME, VALUE)                                                                                            \
  static constexpr llvm::StringLiteral INTERNAL_INIT_##NAME[] = VALUE;                                                 \
  static llvm::ArrayRef<llvm::StringLiteral> INTERNAL_##NAME(INTERNAL_INIT_##NAME, std::size(INTERNAL_INIT_##NAME) - 1);
#include "igc/Options/InternalOptions.inc"
#undef PREFIX
#else
#define PREFIX(NAME, VALUE) static const char *const INTERNAL_##NAME[] = VALUE;
#include "igc/Options/InternalOptions.inc"
#undef PREFIX
#endif

#if LLVM_VERSION_MAJOR >= 22
static const OptTable::Info InternalInfoTable[] = {
#define OPTION(PREFIX, NAME, ID, KIND, GROUP, ALIAS, ALIASARGS, FLAGS, VISIBILITY, PARAM, HELPTEXT,                    \
               HELPTEXTSFORVARIANTS, METAVAR, VALUES, SUBCOMMANDIDS_OFFSET)                                            \
  {PREFIX,                                                                                                             \
   NAME,                                                                                                               \
   HELPTEXT,                                                                                                           \
   HELPTEXTSFORVARIANTS,                                                                                               \
   METAVAR,                                                                                                            \
   internal::OPT_##ID,                                                                                                 \
   Option::KIND##Class,                                                                                                \
   PARAM,                                                                                                              \
   FLAGS,                                                                                                              \
   VISIBILITY,                                                                                                         \
   internal::OPT_##GROUP,                                                                                              \
   internal::OPT_##ALIAS,                                                                                              \
   ALIASARGS,                                                                                                          \
   VALUES,                                                                                                             \
   SUBCOMMANDIDS_OFFSET},
#include "igc/Options/InternalOptions.inc"
#undef OPTION
};
#else
static const OptTable::Info InternalInfoTable[] = {
#define OPTION(PREFIX, NAME, ID, KIND, GROUP, ALIAS, ALIASARGS, FLAGS, PARAM, HELPTEXT, METAVAR, VALUES)               \
  {INTERNAL_##PREFIX,                                                                                                  \
   NAME,                                                                                                               \
   HELPTEXT,                                                                                                           \
   METAVAR,                                                                                                            \
   internal::OPT_##ID,                                                                                                 \
   Option::KIND##Class,                                                                                                \
   PARAM,                                                                                                              \
   FLAGS,                                                                                                              \
   internal::OPT_##GROUP,                                                                                              \
   internal::OPT_##ALIAS,                                                                                              \
   ALIASARGS,                                                                                                          \
   VALUES},
#include "igc/Options/InternalOptions.inc"
#undef OPTION
};
#endif

#if LLVM_VERSION_MAJOR >= 22
#define OPTTABLE_STR_TABLE_CODE
#include "igc/Options/InternalOptions.inc"
#undef PREFIX
#undef OPTTABLE_STR_TABLE_CODE

#define OPTTABLE_PREFIXES_TABLE_CODE
#include "igc/Options/InternalOptions.inc"
#undef OPTTABLE_PREFIXES_TABLE_CODE
#endif

namespace {
class IGCInternalOptTable : public IGCLLVM::GenericOptTable {
public:
#if LLVM_VERSION_MAJOR >= 22
  IGCInternalOptTable()
      : IGCLLVM::GenericOptTable(OptionStrTable, OptionPrefixesTable, InternalInfoTable){
#else
  IGCInternalOptTable() : IGCLLVM::GenericOptTable(InternalInfoTable) {
#endif
            OptTable &Opt = *this;
  (void)Opt;
#define OPTTABLE_ARG_INIT
#include "igc/Options/InternalOptions.inc"
#undef OPTTABLE_ARG_INIT
}
}; // namespace
} // namespace

static const IGCInternalOptTable InternalOptionsTable;

const OptTable &IGC::getInternalOptTable() { return InternalOptionsTable; }
