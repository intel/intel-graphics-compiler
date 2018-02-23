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
#include <functional>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

#ifdef _MSC_VER
#define strdup _strdup
#endif

#ifdef _MSC_VER
// for MSVC only we use the _s version
#  define MEMCPY(D,S,N) memcpy_s(D,N,S,N)
#else
// Linux, MinGW, the rest of the world are still sane
#  define MEMCPY(D,S,N) memcpy(D,S,N)
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

    // Same as below, but admits a filter predicate which only allows a
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
} // namespace iga


#endif // IGA_STRINGS_HPP