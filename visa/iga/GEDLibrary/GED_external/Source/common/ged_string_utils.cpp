/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/ged_base.h"
#include "common/ged_string_utils.h"

using std::stringstream;
using std::hex;
# ifndef __ANDROID__
GEDFORASSERT(using std::llabs);
# endif


const string emptyString = "";


static inline uint64_t ConvertSingleDigitString(const string& str)
{
    if (str[0] < '0' || str[0] > '9')
    {
        GEDERROR("Illegal number for conversion '" + str + "'.");
    }
    return str[0] - '0';
}


static inline uint64_t ConvertHexadecimalString(const string& str)
{
    stringstream strm(str);
    uint64_t num;
    strm >> hex >> num;
    if (strm.fail() || !strm.eof())
    {
        GEDERROR("Illegal number for conversion '" + str + "'.");
    }
    return num;
}


static inline uint64_t ConvertBinaryString(const string& str, const size_t lastChar)
{
    if (GED_QWORD_BITS < lastChar)
    {
        GEDERROR("Illegal number for conversion '" + str + "'.");
    }
    uint64_t num = 0;
    for (size_t i = 0; i < lastChar; ++i)
    {
        num <<= 1;
        if ('1' == str[i])
        {
            num |= 1;
            continue;
        }
        if ('0' != str[i])
        {
            GEDERROR("Illegal number for conversion '" + str + "'.");
        }
    }
    return num;
}


static inline uint64_t ConvertDecimalString(const string& str)
{
    stringstream strm(str);
    uint64_t num;
    strm >> num;
    if (strm.fail() || !strm.eof())
    {
        GEDERROR("Illegal number for conversion '" + str + "'.");
    }
    return num;
}


float ToFloat(const string& str)
{
    char* endPtr = NULL;
    float f = (float)strtod(str.c_str(), &endPtr);
    if (*endPtr != '\0')
    {
        GEDERROR(string("Unable to convert '") + str + "' to float.");
    }
    return f;
}


int64_t ToInt(const string& str)
{
    // Verify that this is not an empty string.
    if (str.empty())
    {
        GEDERROR("An empty string is not a valid input for integer conversion.");
    }

    // Handle negative numbers.
    if ('-' == str[0])
    {
        const int64_t num = (int64_t)ToUint(str.substr(1));
        GEDASSERT(llabs(num) == num);
        return -num;
    }

    // Handle positive numbers.
    return (int64_t)ToUint(str);
}


uint64_t ToUint(const string& str)
{
    // Verify that this is not an empty string.
    if (str.empty())
    {
        GEDERROR("An empty string is not a valid input for unsigned unsigned integer conversion.");
    }

    // Sanity check - make sure that this is not a negative number.
    if ('-' == str[0])
    {
        GEDERROR(string("Expected an unsigned integer but received ") + str + ".");
    }

    // Convert the number.
    const size_t numOfChars = str.size();

    // Handle the single digit case (must be a decimal number).
    if (1 == numOfChars) return ConvertSingleDigitString(str);

    // Handle hexadecimal numbers (should have at least 3 characters).
    if (2 < numOfChars)
    {
        if ('0' == str[0] && ('x' == str[1] || 'X' == str[1]))
        {
            return ConvertHexadecimalString(str);
        }
    }

    // Handle binary numbers.
    const size_t lastChar = str.size() - 1;
    if ('b' == str[lastChar] || 'B' == str[lastChar]) return ConvertBinaryString(str, lastChar);

    // Handle decimal numbers.
    return ConvertDecimalString(str);
}


string HexStr(const uint64_t num)
{
    stringstream strm;
    strm << "0x" << hex << num;
    return strm.str();
}


static string HexToBin(const char c)
{
    switch (c)
    {
    case '0': return "0000";
    case '1': return "0001";
    case '2': return "0010";
    case '3': return "0011";
    case '4': return "0100";
    case '5': return "0101";
    case '6': return "0110";
    case '7': return "0111";
    case '8': return "1000";
    case '9': return "1001";
    case 'a': return "1010";
    case 'b': return "1011";
    case 'c': return "1100";
    case 'd': return "1101";
    case 'e': return "1110";
    case 'f': return "1111";
    default:
        GEDERROR(string("Encountered an invalid character (") + c + ") while converting hex to bin.");
        break;
    }
    return "";
}


string BinStr(const uint64_t num, const size_t size /* = 0 */)
{
    stringstream strm;
    strm << hex << num;
    const string hexStr = strm.str();
    const size_t hexSize = hexStr.size();
    string binStr;
    for (size_t i = 0; i < hexSize; ++i)
    {
        binStr.append(HexToBin(hexStr[i]));
    }

    if (0 == size) return binStr;

    const size_t fullSize = binStr.size();
    GEDASSERT(MAX_UINT8_T >= fullSize); // try to keep the binary representation readable (small)
    if (size > fullSize)
    {
        const string padding(size - fullSize, '0');
        return padding + binStr;
    }
    else if (fullSize > size)
    {
        return binStr.substr(fullSize - size);
    }
    else
    {
        return binStr;
    }
}


string FltStr(const float num)
{
    stringstream strm;
    strm << num;
    return strm.str();
}


void BreakLines(const string& src, vector<string>& lines, const size_t lineLength)
{
    const size_t fullLength = src.size();
    size_t start = 0;
    size_t endLine = src.find('\n');
    if (0 == endLine)
    {
        // Handle the special case of an endline at the beginning of the string.
        lines.push_back("");
        ++start;
        endLine = src.find('\n', endLine + 1);
    }
    while (start < fullLength)
    {
        size_t skip = 0;
        size_t end = start + (size_t)lineLength - 1;
        if (endLine < end)
        {
            // Break at the endline character.
            if (start == endLine)
            {
                // This line contains only the endline character.
                lines.push_back("");
                ++start;
                endLine = src.find('\n', endLine + 1);
                continue;
            }
            end = endLine - 1;
            skip = 1; // in order to start at the next character
            endLine = src.find('\n', endLine + 1);
        }
        else if (endLine > end)
        {
            if (end < fullLength)
            {
                // Break at the end of the previous word.
                end = src.rfind(' ', end);
                if (string::npos == end || end <= start)
                {
                    GEDERROR(string("Encountered an error while parsing the following string:\n") + src);
                }
                --end;
                skip = 1;
            }
        }
        else // (endLine == end)
        {
            endLine = src.find('\n', endLine + 1);
            --end;
            skip = 1;
        }

        GEDASSERT(start <= end);
        size_t currLength = end - start + 1;
        string lineToPush = src.substr(start, currLength);
        if (lineToPush.find_first_not_of(' ') != std::string::npos)
        {
            lines.push_back(src.substr(start, currLength));
        }
        else
        {
            // Line consists only of whitespaces, replace with an empty line
            lines.push_back("");
        }
        start += (currLength + skip);
    }
}
