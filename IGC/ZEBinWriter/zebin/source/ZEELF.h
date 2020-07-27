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

// ELF32_Ehdr::e_flags
struct TargetFlags {

    // bit[7:0]: dedicated for specific generator (meaning based on generatorId)
    enum GeneratorSpecificFlags {
        NONE = 0
    };
    // bit[23:21]: generator of this device binary
    enum GeneratorId {
        UNREGISTERED = 0,
        IGC          = 1
    };

    union {
        struct {
            // bit[7:0]: dedicated for specific generator (meaning based on generatorId)
            GeneratorSpecificFlags generatorSpecificFlags : 8;
            // bit[12:8]: values [0-31], min compatbile device revision Id (stepping)
            uint8_t minHwRevisionId : 5;
            // bit[13:13]:
            // 0 - full validation during decoding (safer decoding)
            // 1 - no validation (faster decoding - recommended for known generators)
            bool validateRevisionId : 1;
            // bit[14:14]:
            // 0 - ignore minHwRevisionId and maxHwRevisionId
            // 1 - underlying device must match specified revisionId info
            bool disableExtendedValidation : 1;
            // bit[15:15]:
            // 0 - elfFileHeader::machine is PRODUCT_FAMILY
            // 1 - elfFileHeader::machine is GFXCORE_FAMILY
            bool machineEntryUsesGfxCoreInsteadOfProductFamily : 1;
            // bit[20:16]:  max compatbile device revision Id (stepping)
            uint8_t maxHwRevisionId : 5;
            // bit[23:21]: generator of this device binary
            GeneratorId generatorId : 3;
            // bit[31:24]: MBZ, reserved for future use
            uint8_t reserved : 8;
        };
        uint32_t packed = 0U;
    };
};

} // namespace zebin

#endif // ZE_ELF_H
