/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include <cstring>
#include "common/ged_base.h"
#include "common/ged_string_utils.h"
#include "common/ged_ins_disassembly_table.h"

using std::memcmp;


#if GED_DISASSEMBLY

const char* gedDisassemblyBlockTypeStrings[GED_DISASSEMBLY_BLOCK_TYPE_SIZE] =
{
    "GED_DISASSEMBLY_BLOCK_TYPE_TOKENS",
    "GED_DISASSEMBLY_BLOCK_TYPE_FIELD_LIST",
    "GED_DISASSEMBLY_BLOCK_TYPE_INTERPRETER",
    "GED_DISASSEMBLY_BLOCK_TYPE_NEXT_TABLE",
    "GED_DISASSEMBLY_BLOCK_TYPE_NOT_SUPPORTED",
};


const char* gedDisassemblyBlockTypePadding[GED_DISASSEMBLY_BLOCK_TYPE_SIZE] =
{
    "       ",
    "   ",
    "  ",
    "   ",
    "",
};


const char* gedDisassemblyTokenTypeStrings[GED_DISASSEMBLY_TOKEN_TYPE_SIZE] =
{
    "GED_DISASSEMBLY_TOKEN_TYPE_OPCODE",
    "GED_DISASSEMBLY_TOKEN_TYPE_COMPACTED",
    "GED_DISASSEMBLY_TOKEN_TYPE_CHAR",
    "GED_DISASSEMBLY_TOKEN_TYPE_FIELD",
    "GED_DISASSEMBLY_TOKEN_TYPE_RAW_FIELD",
};


const char* gedDisassemblyTokenTypePadding[GED_DISASSEMBLY_TOKEN_TYPE_SIZE] =
{
    "   ",
    "",
    "     ",
    "    ",
    "",
};


const char* ged_disassembly_token_t_str = "ged_disassembly_token_t";


bool ged_disassembly_token_t::operator==(const ged_disassembly_token_t& rhs) const
{
    GEDASSERT(sizeof(ged_disassembly_token_t) == sizeof(uint32_t));
    return (*reinterpret_cast<const uint32_t*>(this) == *reinterpret_cast<const uint32_t*>(&rhs));
}


bool ged_disassembly_block_t::operator<(const ged_disassembly_block_t& rhs) const
{
    if (_entryType != rhs._entryType) return (_entryType < rhs._entryType);
    return (memcmp(this, &rhs, sizeof(ged_disassembly_block_t)) < 0);
}


bool ged_disassembly_block_t::operator==(const ged_disassembly_block_t& rhs) const
{
    if (_entryType != rhs._entryType) return false;
    return (0 == memcmp(this, &rhs, sizeof(ged_disassembly_block_t)));
}


const char* ged_disassembly_block_t_str = "ged_disassembly_block_t";
const char* ged_disassembly_table_t_str = "ged_disassembly_table_t";
const ged_disassembly_block_t emptyBlock = { GED_DISASSEMBLY_BLOCK_TYPE_NOT_SUPPORTED, 0 };

#endif // GED_DISASSEMBLY
