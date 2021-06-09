/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
