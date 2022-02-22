/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/GenX/RegCategory.h"

#include <llvm/ADT/StringRef.h>

using namespace llvm;
using namespace vc;

StringRef vc::getRegCategoryName(RegCategory Category) {
  switch (Category) {
  case vc::RegCategory::None:
    return "none";
  case vc::RegCategory::General:
    return "general";
  case vc::RegCategory::Address:
    return "address";
  case vc::RegCategory::Predicate:
    return "predicate";
  case vc::RegCategory::Sampler:
    return "sampler";
  case vc::RegCategory::Surface:
    return "surface";
  case vc::RegCategory::EM:
    return "em";
  case vc::RegCategory::RM:
    return "rm";
  default:
    return "???";
  }
}

StringRef vc::getRegCategoryShortName(RegCategory Category) {
  switch (Category) {
  case RegCategory::None:
    return "-";
  case RegCategory::General:
    return "v";
  case RegCategory::Address:
    return "a";
  case RegCategory::Predicate:
    return "p";
  case RegCategory::Sampler:
    return "s";
  case RegCategory::Surface:
    return "t";
  case RegCategory::EM:
    return "e";
  case RegCategory::RM:
    return "r";
  default:
    return "?";
  }
}
