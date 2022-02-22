/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENX_REGCATEGORY_H
#define VC_UTILS_GENX_REGCATEGORY_H

#include "vc/Utils/General/EnumUtils.h"
#include <llvm/ADT/StringRef.h>

#include <type_traits>
#include <utility>

namespace vc {

// The encoding for register category, used in GenXCategory,
// GenXLiveness and GenXVisaRegAlloc.  It is an anonymous enum inside a class
// rather than a named enum so you don't need to cast to/from int.
//
// Please update the following functions when changing this enum:
// isRealCategory
// isMaskCategory
// getRegCategoryName
// getRegCategoryShortName
enum class RegCategory : uint8_t {
  None,
  General,
  Address,
  Predicate,
  Sampler,
  Surface,
  EM,
  RM,
  LastCategory = RM
};

constexpr size_t numRegCategories() {
  return static_cast<size_t>(RegCategory::LastCategory) + 1;
}

constexpr bool checkRegCategory(RegCategory Category) {
  auto IntCat = static_cast<std::underlying_type_t<RegCategory>>(Category);
  if (IntCat >= vc::numRegCategories())
    return false;
  return true;
}

constexpr bool isRealCategory(RegCategory Category) {
  switch (Category) {
  case vc::RegCategory::General:
  case vc::RegCategory::Address:
  case vc::RegCategory::Predicate:
  case vc::RegCategory::Sampler:
  case vc::RegCategory::Surface:
    return true;
  case vc::RegCategory::None:
  case vc::RegCategory::EM:
  case vc::RegCategory::RM:
    return false;
  }
  IGC_ASSERT_MESSAGE(
      false,
      "New RegCategory was added, but isRealCategory func was not updated");
  return false;
}

constexpr bool isRealOrNoneCategory(RegCategory Category) {
  return isRealCategory(Category) || Category == RegCategory::None;
}

constexpr bool isMaskCategory(RegCategory Category) {
  switch (Category) {
  case vc::RegCategory::None:
  case vc::RegCategory::General:
  case vc::RegCategory::Address:
  case vc::RegCategory::Predicate:
  case vc::RegCategory::Sampler:
  case vc::RegCategory::Surface:
    return false;
  case vc::RegCategory::EM:
  case vc::RegCategory::RM:
    return true;
  }
  IGC_ASSERT_MESSAGE(
      false,
      "New RegCategory was added, but isMaskCategory func was not updated");
  return false;
}

template <typename SequenceContainer>
constexpr auto &&accessContainer(SequenceContainer &&SC, RegCategory Category) {
  return SC[static_cast<size_t>(Category)];
}

using RegCategoryIterator =
    EnumIterator<RegCategory, RegCategory::LastCategory>;

class RegCategoryView final {};

constexpr RegCategoryIterator begin(RegCategoryView) {
  return RegCategory::None;
}

constexpr RegCategoryIterator end(RegCategoryView) { return {}; }

// Get name of register category for debugging purposes.
// For anything unknown "???" is returned.
llvm::StringRef getRegCategoryName(RegCategory Category);

// Get short code of register category for debugging purposes.
// For None "-" is returned, for anything unknown -- "?".
llvm::StringRef getRegCategoryShortName(RegCategory Category);

} // namespace vc

#endif // VC_UTILS_GENX_REGCATEGORY_H
