/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "igc/Options/Options.h"

#include <llvm/Option/Option.h>

using namespace IGC::options;
using namespace llvm::opt;

#define PREFIX(NAME, VALUE) static const char *const NAME[] = VALUE;
#include "igc/Options/Options.inc"
#undef PREFIX

static const OptTable::Info InfoTable[] = {
#define OPTION(PREFIX, NAME, ID, KIND, GROUP, ALIAS, ALIASARGS, FLAGS, PARAM,  \
               HELPTEXT, METAVAR, VALUES)                                      \
  {PREFIX, NAME,  HELPTEXT,    METAVAR,     OPT_##ID,  Option::KIND##Class,    \
   PARAM,  FLAGS, OPT_##GROUP, OPT_##ALIAS, ALIASARGS, VALUES},
#include "igc/Options/Options.inc"
#undef OPTION
};

namespace {
class IGCOptTable : public OptTable {
public:
  IGCOptTable() : OptTable(InfoTable) {
    OptTable &Opt = *this;
    (void)Opt;
#define OPTTABLE_ARG_INIT
#include "igc/Options/Options.inc"
#undef OPTTABLE_ARG_INIT
  }
};
} // namespace

static const IGCOptTable OptionsTable;

const OptTable &IGC::getOptTable() { return OptionsTable; }
