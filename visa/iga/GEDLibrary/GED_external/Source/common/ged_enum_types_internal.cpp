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
#include "common/ged_enum_types_internal.h"


const char* gedReturnValueStrings[GED_RETURN_VALUE_SIZE] =
{
    "GED_RETURN_VALUE_SUCCESS",
    "GED_RETURN_VALUE_CYCLIC_DEPENDENCY",
    "GED_RETURN_VALUE_NULL_POINTER",
    "GED_RETURN_VALUE_OPCODE_NOT_SUPPORTED",
    "GED_RETURN_VALUE_NO_COMPACT_FORM",
    "GED_RETURN_VALUE_INVALID_FIELD",
    "GED_RETURN_VALUE_INVALID_VALUE",
    "GED_RETURN_VALUE_INVALID_INTERPRETER",
    "GED_RETURN_VALUE_FILE_OPEN_FAILED",
    "GED_RETURN_VALUE_FILE_READ_FAILED",
    "GED_RETURN_VALUE_INVALID_OPERAND",
    "GED_RETURN_VALUE_BAD_COMPACT_ENCODING",
    "GED_RETURN_VALUE_INVALID_MODEL",
    "GED_RETURN_VALUE_BUFFER_TOO_SHORT",
};


const char* gedReturnValuePadding[GED_RETURN_VALUE_SIZE] =
{
    "             ",
    "   ",
    "        ",
    "",
    "     ",
    "       ",
    "       ",
    " ",
    "    ",
    "    ",
    "     ",
    "",
    "       ",
    "    ",
};

const char* GEDInsTypeStr = "GED_INS_TYPE";
