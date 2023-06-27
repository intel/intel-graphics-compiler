/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/IGCSPIRVParser.h"

using namespace IGC;

std::vector<std::string> SPIRVParser::getEntryPointNames(const StringRef binary)
{
    auto byteOffsetInWord = [](uint8_t byteIndex, bool isLittleEndian)
    {
        return isLittleEndian ? byteIndex * 8 : 24 - byteIndex * 8;
    };
    auto getWord = [&binary, &byteOffsetInWord](uint32_t offsetInWords, bool isLittleEndian)
    {
        uint32_t result = 0;
        for (uint8_t i = 0; i < 4; ++i)
        {
            result |= uint32_t(binary[offsetInWords * 4 + i]) << byteOffsetInWord(i, isLittleEndian);
        }
        return result;
    };

    constexpr uint32_t SPIRVMagicNumber = 0x07230203;
    constexpr uint16_t OpEntryPoint = 15;

    uint32_t offsetInWords = 0;

    bool isLittleEndian = getWord(0, true) == SPIRVMagicNumber;
    offsetInWords += 5; // skip header

    std::vector<std::string> entryPointNames;
    while ((offsetInWords + 1) * 4 < binary.size())
    {
        uint32_t opFirstWord = getWord(offsetInWords, isLittleEndian);
        uint16_t opWordsCount = opFirstWord >> 16;
        uint16_t opCode = opFirstWord & 0xffff;

        if (opCode == OpEntryPoint)
        {
            size_t nameLength = 0;
            while (binary.data()[(offsetInWords + 3) * 4 + nameLength])
                nameLength++;
            std::string name = binary.substr((offsetInWords + 3) * 4, nameLength).str();
            entryPointNames.push_back(name);
        }
        offsetInWords += opWordsCount; // skip instruction
    }
    return entryPointNames;
}
