/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/IGCSPIRVParser.h"

#include "Probe/Assertion.h"

using namespace IGC;

std::vector<std::string> SPIRVParser::getEntryPointNames(const StringRef binary) {
  auto getWord = [&binary](uint32_t offsetInWords) {
    IGC_ASSERT(offsetInWords < (binary.size() / 4));
    const uint32_t result = *(reinterpret_cast<const uint32_t *>(binary.data() + offsetInWords * 4));
    return result;
  };

  [[maybe_unused]] constexpr uint32_t SPIRVMagicNumber = 0x07230203;
  constexpr uint16_t OpEntryPoint = 15;

  uint32_t offsetInWords = 0;

  offsetInWords += 5; // skip header

  std::vector<std::string> entryPointNames;
  while ((offsetInWords + 1) * 4 < binary.size()) {
    uint32_t opFirstWord = getWord(offsetInWords);
    uint16_t opWordsCount = opFirstWord >> 16;
    uint16_t opCode = opFirstWord & 0xffff;

    if (opCode == OpEntryPoint) {
      size_t nameLength = 0;
      while (binary.data()[(offsetInWords + 3) * 4 + nameLength])
        nameLength++;
      std::string name = binary.substr((offsetInWords + 3) * 4, nameLength).str();
      entryPointNames.push_back(std::move(name));
    }
    offsetInWords += opWordsCount; // skip instruction
  }
  return entryPointNames;
}
