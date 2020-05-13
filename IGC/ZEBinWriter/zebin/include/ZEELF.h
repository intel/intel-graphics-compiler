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
//===- ZEELF.hpp ------------------------------------------------*- C++ -*-===//
// ZE Binary Utilitis
//
// \file
// The file define the special enum value for ZE binary
//===----------------------------------------------------------------------===//

#ifndef ZE_ELF_H
#define ZE_ELF_H

#include <cstdint>

namespace zebin {

// ELF file type for ELF32_Ehdr::e_type
enum ELF_TYPE_ZEBIN
{
    ET_ZEBIN_NONE = 0x0,

    ET_ZEBIN_REL  = 0xff11,     // A relocatable ZE binary file
    ET_ZEBIN_EXE  = 0xff12,     // An executable ZE binary file
    ET_ZEBIN_DYN  = 0xff13,     // A shared object ZE binary file
};

// ELF section type for ELF32_Shdr::sh_type
enum SHT_ZEBIN
{
    SHT_ZEBIN_SPIRV  = 0xff000009, // .spv.kernel section, value the same as SHT_OPENCL_SPIRV
    SHT_ZEBIN_ZEINFO = 0xff000011  // .ze.info section
};

// ELF relocation type for ELF32_Rel::ELF32_R_TYPE
enum R_TYPE_ZEBIN
{
    R_ZE_NONE           = 0,
    R_ZE_SYM_ADDR       = 1, // 64-bit type address
    R_ZE_SYM_ADDR_32    = 2, // lower 32-bit of 64-bit address.
    R_ZE_SYM_ADDR_32_HI = 3  // higher 32bits of 64-bit address
};

// Standard ELF Symbol bindings (do not change)
enum S_BINDING_ELF {
    STB_LOCAL      = 0,  // Local symbol, not visible outside obj file containing def
    STB_GLOBAL     = 1,  // Global symbol, visible to all object files being combined
    STB_WEAK       = 2,  // Weak symbol, like global but lower-precedence
    STB_LOOS       = 10, // Lowest operating system-specific binding type
    STB_HIOS       = 12, // Highest operating system-specific binding type
    STB_LOPROC     = 13, // Lowest processor-specific binding type
    STB_HIPROC     = 15  // Highest processor-specific binding type
};

// Standard ELF Symbol types (do not change)
enum S_TYPE_ELF {
    STT_NOTYPE = 0,     // Symbol's type is not specified
    STT_OBJECT = 1,     // Symbol is a data object (variable, array, etc.)
    STT_FUNC = 2,       // Symbol is executable code (function, etc.)
    STT_SECTION = 3,    // Symbol refers to a section
    STT_FILE = 4,       // Local, absolute symbol that refers to a file
    STT_COMMON = 5,     // An uninitialized common block
    STT_TLS = 6,        // Thread local data object
    STT_GNU_IFUNC = 10, // GNU indirect function
    STT_LOOS = 10,      // Lowest operating system-specific symbol type
    STT_HIOS = 12,      // Highest operating system-specific symbol type
    STT_LOPROC = 13,    // Lowest processor-specific symbol type
    STT_HIPROC = 15,    // Highest processor-specific symbol type
};

// ELF32_Ehdr::e_flags
struct TargetFlags {
    union {
        struct {
            uint8_t generatorSpecificFlags : 8;   // bit offset : 0, 8 bits dedicated for specific generator (meaning based on generatorId)
            uint8_t minHwRevisionId : 5;          // bit offset : 8,  values [0-31], min compatbile device revision Id
            bool disableExtendedValidation : 1;   // bit offset : 13, values {0, 1}, 0 - full validation during decoding (safer decoding), 1 - no validation (faster decoding - recommended for known generators)
            bool validateRevisionId : 1;          // bit offset : 14, values {0, 1}, 0 - ignore minHwRevisionId and maxHwRevisionId, 1 - underlying device must match specified revisionId info
            bool machineEntryUsesGfxCoreInsteadOfProductFamily : 1; // bit offset : 15, values {0, 1}, 0 - elfFileHeader::machine is PRODUCT_FAMILY, 1 - elfFileHeader::machine is GFXCORE_FAMILY
            uint8_t maxHwRevisionId : 5;          // bit offset : 16, values [0-31], max compatbile device revision Id
            uint8_t generatorId : 3;              // bit offset : 21, values [0-7], generator of this device binary, unregisted = 0, igc = 1, etc.
            uint8_t reserved : 8;                 // bit offset : 24, 0, for future use
        };
        uint32_t packed = 0U;
    };
};

} // namespace zebin

#endif // ZE_ELF_H
