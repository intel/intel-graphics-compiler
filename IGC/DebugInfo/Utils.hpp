/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "Probe/Assertion.h"

namespace IGC {

template <class ContainerType, class BinaryFunction,
          class Sorter = std::less<typename ContainerType::key_type>>
static void OrderedTraversal(const ContainerType &Data, BinaryFunction Visit,
                             Sorter SortProcedure = {}) {
  std::vector<typename ContainerType::key_type> Keys;
  std::transform(Data.begin(), Data.end(), std::back_inserter(Keys),
                 [](const auto &KV) { return KV.first; });
  std::sort(Keys.begin(), Keys.end(), SortProcedure);
  for (const auto &Key : Keys) {
    auto FoundIt = Data.find(Key);
    IGC_ASSERT(FoundIt != Data.end());
    const auto &Val = FoundIt->second;
    Visit(Key, Val);
  }
}

} // namespace IGC
