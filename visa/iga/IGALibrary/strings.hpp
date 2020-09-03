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

#ifndef IGA_STRINGS_HPP
#define IGA_STRINGS_HPP

#include <cstdarg>
#include <cstdio>
#include <cstdint>
#include <functional>
#include <iostream>
#include <ostream>
#include <string>
#include <sstream>
#include <vector>

#ifdef _MSC_VER
#define strdup _strdup
#endif

#ifdef _MSC_VER
// MSVC has different semantics with vsnprintf given NULL.
#define VSCPRINTF(PAT,VA) \
    _vscprintf(PAT,VA)
#else
// The rest of the world can use this form of vsnprintf
#define VSCPRINTF(PAT,VA) \
    vsnprintf(NULL, 0, PAT, VA)
#endif
#ifdef _MSC_VER
#define VSPRINTF(B,BLEN,...) \
       vsprintf_s(B, BLEN, __VA_ARGS__)
#else
// Linux, MinGW, and the rest of the world choose this.
// Warning, although MinGW also has vsprintf_s, it's signature is
// inconsistent with the MSVC version (has extra param).
// Thankfully, they don't whine about bogus security issues and we
// can use the old version.
#define VSPRINTF(B,BLEN,...) \
       vsnprintf(B, BLEN, __VA_ARGS__)
#endif


#define IGA_MODEL_STRING(X) X


namespace iga {
    // takes a printf-style pattern and converts it to a string
    std::string format(const char *pat, ...);
    std::string formatv(const char *pat, va_list &va);
    // emits a printf-style string to the given output stream
    // returns the nubmer of characters written
    size_t formatTo(std::ostream &os, const char *patt, ...);
    size_t formatvTo(std::ostream &os, const char *patt, va_list &va);
    // emits to a buffer of a given size
    // returns the result of vsnprintf (or the equivalent) ... num chars needed
    size_t formatTo(char *buf, size_t bufLen, const char *patt, ...);
    size_t formatvTo(char *buf, size_t bufLen, const char *patt, va_list &va);

    // copies the contents of 'os' into buf (safely)
    // returns the required string size
    size_t copyOut(char *buf, size_t bufCap, std::iostream &ios);

    // formats a value into binary padding with 0's for a given width w
    // (the value of 0 auto computes the minimal width)
    // without affecting os's stream state
    //   e.g. fmtBinaryDigits(os,0xB,5) => emits 01011
    //   e.g. fmtBinaryDigits(os,0xB,0) => emits  1011
    void fmtBinaryDigits(std::ostream &os, uint64_t val, int w = 0);
    // same as fmtBinaryDigits, but prefixes an additional "0b"
    // is careful to not muck up os's state
    // w doesn't count the 0b prefix as part of the width
    // e.g. fmtBinary(os,0xB,0) -> 0b1101
    void fmtBinary(std::ostream &os, uint64_t val, int w = 0);
    //
    // formats to uppercase hex for a given width
    // without affecting os's stream state
    void fmtHexDigits(std::ostream &os, uint64_t val, int w = 0);
    // same as fmtHexDigits, but prefixes an 0x
    // does not change os's stream state
    void fmtHex(std::ostream &os, uint64_t val, int w = 0);
    // a helper that returns a string version
    std::string fmtHex(uint64_t val, int w = 0);

    // This class simplifies formatting loops by dropping the first comma.
    // One calls insert *before* each element being formatted.
    //
    // EXAMPLE:
    //   Intercalator comma(os, ",");
    //   for (...) {
    //     comma.insert();
    //     emitElement(...);
    //   }
    class Intercalator {
        std::ostream &os;
        bool first = true;
        const char *sep;
    public:
        Intercalator(std::ostream &_os, const char *_sep)
            : os(_os), sep(_sep) { }
        void insert() {
            if (first) {
                first = false;
            } else {
                os << sep;
            }
        }
    };

    // Same as below, but permits a filter predicate which only allows a
    // subset of elements (identified by that predicate).
    template <typename Container, typename Predicate, typename Formatter>
    static void intercalate(
        std::ostream &os,
        const char *sep,
        const Container &ts,
        const Predicate &filterT,
        const Formatter &formatT)
    {
        Intercalator separator(os, sep ? sep : "");
        for (const auto &t : ts) {
            if (filterT(t)) {
                separator.insert();
                formatT(t);
            }
        }
    }
    // This emits a container of elements to an output stream by calling
    // a given formatter function, while automatically adding separators
    // between elements.
    //
    // EXAMPLE: let foos be a std::vector<Foo> and os be a std::ostream
    // Then we might have:
    //   intercalate(os, ",", foos,
    //      [&] (const Foo &f) {
    //         os << ... format f ...
    //      });
    //
    template <typename Container, typename Formatter>
    static void intercalate(
        std::ostream &os,
        const char *sep,
        const Container &ts,
        const Formatter &formatT)
    {
        Intercalator separator(os, sep ? sep : "");
        for (const auto &t : ts) {
            separator.insert();
            formatT(t);
        }
    }


    // Emits something like:
    //   "foo"
    //   "foo and bar"
    //   "foo, bar, and baz"
    template <typename T, typename FormatT>
    static void commafyList(
        const char *conjunction, // e.g. "and" or "or"
        std::ostream &os,
        const std::vector<T> &elems,
        const FormatT &formatElem) // [](std::ostream &os, const T &elem) {...}
    {
        switch (elems.size()) {
        case 1:
            formatElem(os, elems[0]);
            break;
        case 2:
            formatElem(os, elems[0]);
            os << " " << conjunction << " ";
            formatElem(os, elems[1]);
            break;
        default:
            for (size_t i = 0; i < elems.size(); i++) {
                if (i > 0)
                    os << ", ";
                if (i == elems.size() - 1)
                    os << conjunction << " ";
                formatElem(os, elems[i]);
            }
        }
    }
    template <typename T, typename FormatT>
    static void commafyList(
        std::ostream &os,
        const std::vector<T> &elems,
        const FormatT &formatElem)
    {
        commafyList("and", os, elems, formatElem);
    }
    template <typename T>
    std::string PadR(size_t k, const T& t)
    {
        std::stringstream ssCol;
        ssCol << t;
        for (size_t i = (size_t)ssCol.tellp(); i < k; i++)
            ssCol << ' ';
        return ssCol.str();
    }
    template <typename T>
    std::string PadL(size_t k, const T& t)
    {
        std::stringstream ssc;
        ssc << t;
        std::string col = ssc.str();

        std::stringstream ss;
        int n = (int)k - (int)col.size();
        for (int i = 0; i < n; i++) {
            ss << ' ';
        }
        ss << col;
        return ss.str();
    }
} // namespace iga


#endif // IGA_STRINGS_HPP
