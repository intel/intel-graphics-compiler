/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenIntrinsicLookup.h"
#include "GenIntrinsicDefinition.h"
#include "GenIntrinsicLookupTable.h"

namespace IGC {

llvm::GenISAIntrinsic::ID LookupIntrinsicId(llvm::StringRef GenISAprefix, llvm::StringRef Name) {

  if (!Name.startswith(GenISAprefix))
    return llvm::GenISAIntrinsic::ID::no_intrinsic;

  static auto LengthTable = GetIntrinsicLookupTable();

  auto IntrinsicName = Name.substr(GenISAprefix.size());
  // Extract the base intrinsic name (everything before the first dot)
  // This is used for the initial binary search to narrow down candidates
  auto BaseName = IntrinsicName.substr(0, IntrinsicName.find('.'));

  // Perform binary search to find the first entry that could match our input
  // The lambda removes the trailing marker from lookup table entries for comparison
  auto it = std::lower_bound(LengthTable.begin(), LengthTable.end(), BaseName, [](const auto &entry, const auto &name) {
    auto str = llvm::StringRef(entry.str).drop_back(1);
    return str < name;
  });

  auto BestId = llvm::GenISAIntrinsic::ID::no_intrinsic;
  size_t BestMatchLength = 0;

  for (auto iter = it; iter != LengthTable.end(); ++iter) {
    auto CurrentStr = llvm::StringRef(iter->str);

    // Count how many leading characters match between input and candidate
    auto MismatchResult =
        std::mismatch(IntrinsicName.begin(), IntrinsicName.end(), CurrentStr.begin(), CurrentStr.end(),
                      [](char a, char b) { return (a == b) || (a == '.' && b == '_'); });
    auto MatchLength = static_cast<size_t>(std::distance(IntrinsicName.begin(), MismatchResult.first));

    // Match doesn't even cover the base name, stop searching
    if (MatchLength < BaseName.size()) {
      break;
    }

    // Matches are getting worse, no need to continue
    if (MatchLength < BestMatchLength) {
      break;
    }

    // Don't count a trailing dot as part of the match to avoid
    // incorrectly favoring matches based on type suffix delimiters
    // e.g., foo.bar.fp32 should prefer foo.bar over foo.bar.baz
    if (IntrinsicName[MatchLength - 1] == '.')
      MatchLength--;
    // Update best match if this candidate has more matching characters
    if (MatchLength > BestMatchLength) {
      BestMatchLength = MatchLength;
      BestId = iter->id;
    }
  }

  return BestId;
}

} // namespace IGC
