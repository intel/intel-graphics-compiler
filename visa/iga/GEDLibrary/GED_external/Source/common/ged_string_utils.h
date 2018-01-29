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

#ifndef GED_STRING_UTILS_H
#define GED_STRING_UTILS_H

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include "common/ged_basic_types.h"

using std::string;
using std::stringstream;
using std::setw;
using std::vector;


extern const string emptyString;


// Convert the string number to a float.
extern float ToFloat(const string& str);

// Convert the string number to a signed integer.
extern int64_t ToInt(const string& str);

// Convert the string number to an unsigned integer.
extern uint64_t ToUint(const string& str);

// Convert an integer of any size to a decimal representation string.
template<typename T>
string DecStr(const T& num, const unsigned int width = 0)
{
    stringstream strm;
    strm << setw(width) << num;
    return strm.str();
}

// Convert an unsigned integer to a hexadecimal representation string.
extern string HexStr(const uint64_t num);

// Convert an unsigned integer to a binary representation string.
extern string BinStr(const uint64_t num, const size_t size = 0);

// Convert a floating point number to a string.
extern string FltStr(const float num);

// Break the given string into a series of strings no longer than the given lineLength.
extern void BreakLines(const string& src, vector<string>& lines, const size_t lineLength);

#endif // GED_STRING_UTILS_H
