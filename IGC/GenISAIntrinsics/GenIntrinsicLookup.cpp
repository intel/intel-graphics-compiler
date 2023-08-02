/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenIntrinsicLookup.h"
#include "GenIntrinsicDefinition.h"
#include "GenIntrinsicLookupTable.h"

namespace IGC
{

llvm::GenISAIntrinsic::ID LookupIntrinsicId(const char* pName)
{
    static auto LengthTable = GetIntrinsicLookupTable();

    std::string input_name(pName);
    unsigned start = 0;
    unsigned end = LengthTable.size();
    unsigned initial_size = end;
    unsigned cur_pos = (start + end) / 2;
    char letter;
    char input_letter;
    bool isError = false;
    bool bump = false;
    unsigned start_index = scIntrinsicPrefix.size();
    for (unsigned i = 0; i < input_name.length(); i++)
    {
        input_letter = input_name[start_index + i];
        unsigned counter = 0;
        while (1)
        {
            if (counter == initial_size || cur_pos >= initial_size)
            {
                isError = true;
                break;
            }
            counter++;
            letter = LengthTable[cur_pos].str[i];
            if (letter == input_letter)
            {
                if (LengthTable[cur_pos].num == i)
                    return LengthTable[cur_pos].id;
                bump = true;
                break;
            }
            else if (input_letter == '\0' && letter == '@')
                return LengthTable[cur_pos].id;
            else if (input_letter == '.' && letter == '_')
                break;
            else if (input_letter == '.' && letter == '@')
            {
                unsigned original_cur_pos = cur_pos;
                while (1)
                {
                    if (cur_pos >= initial_size || LengthTable[cur_pos].num < i)
                        return LengthTable[original_cur_pos].id;
                    if (LengthTable[cur_pos].str[i] == '_')
                        break;
                    cur_pos += 1;
                }
                break;
            }
            else if ((bump && letter < input_letter) || letter == '@')
            {
                cur_pos += 1;
                continue;
            }
            else if (bump && letter > input_letter)
            {
                cur_pos -= 1;
                continue;
            }
            else if (letter < input_letter)
                start = cur_pos;
            else
                end = cur_pos;
            cur_pos = (start + end) / 2;
        }
        if (isError)
            break;
    }

    return llvm::GenISAIntrinsic::ID::no_intrinsic;
}

} // namespace IGC
