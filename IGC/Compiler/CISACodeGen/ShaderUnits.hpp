/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/Units.hpp"
namespace IGC
{

    struct BitUnit
    {
        static const int ratio = 1;
        inline static const char* name(bool plural) { return plural ? "bits" : "bit"; }
    };

    /// Represents one element of the GS payload / URB write that keeps e.g. single color component
    struct Element
    {
        static const int ratio = 32 * BitUnit::ratio; // 32 bits, aka DWORD
        inline static const char* name(bool plural) { return plural ? "elements" : "element"; }
    };

    /// Represents the size of one attribute (that consists of four elements).
    struct Attribute
    {
        static const int ratio = 4 * Element::ratio; // 128 bits
        inline static const char* name(bool plural) { return plural ? "attributes" : "attribute"; }
    };

    /// Represents the size of four elements.
    struct QuadElement
    {
        static const int ratio = 4 * Element::ratio; // 128 bits
        inline static const char* name(bool plural) { return plural ? "quad attributes" : "quad attribute"; }
    };

    /// Represents the size of eight elements.
    struct OctElement
    {
        static const int ratio = 8 * Element::ratio; // 256 bits
        inline static const char* name(bool plural) { return plural ? "oct attributes" : "oct attribute"; }
    };

    // URB Allocation size is counted in 512 bit units (cache line size)
    struct URBAllocation
    {
        static const int ratio = 16 * Element::ratio;
        inline static const char* name(bool plural) { return plural ? "URBAllocations" : "URBAllocation"; }
    };

    /// Shorthands for type names
    typedef Unit<Element>       EltUnit;
    typedef Unit<QuadElement>   QuadEltUnit;
    typedef Unit<OctElement>    OctEltUnit;
    typedef Unit<URBAllocation> URBAllocationUnit;

}