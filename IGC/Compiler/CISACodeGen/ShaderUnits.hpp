/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

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