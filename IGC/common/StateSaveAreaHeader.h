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


#pragma once

namespace SIP
{

    struct alignas(4) version {
        uint8_t major;
        uint8_t minor;
        uint16_t patch;
    };

    struct alignas(8) StateSaveArea {
        char magic[8] = "tssarea";
        uint64_t reserved1;
        struct version version;
        uint32_t size;
        uint8_t reserved2[11];
    };

    // Debug SIP will dump this data in debug surface.
    struct sr_ident {
        char magic[8] = "srmagic";
        struct version version;
    };

    struct alignas(8) regset_desc {
        uint32_t offset;
        uint16_t num;
        uint16_t bits;
        uint16_t bytes;
    };

    struct alignas(8) intelgt_state_save_area {
        uint32_t num_slices;
        uint32_t num_subslices_per_slice;
        uint32_t num_eus_per_subslice;
        uint32_t num_threads_per_eu;
        uint32_t state_area_offset;
        uint32_t state_save_size;
        uint32_t slm_area_offset;
        uint32_t slm_bank_size;
        uint32_t slm_bank_valid;
        uint32_t sr_magic_offset;
        struct regset_desc grf;
        struct regset_desc addr;
        struct regset_desc flag;
        struct regset_desc emask;
        struct regset_desc sr;
        struct regset_desc cr;
        struct regset_desc notification;
        struct regset_desc tdr;
        struct regset_desc acc;
        struct regset_desc mme;
        struct regset_desc tm0;
        struct regset_desc ce;
        struct regset_desc sp;
        struct regset_desc dbg;
        struct regset_desc cmd;
    };

    struct StateSaveAreaHeader {
        struct StateSaveArea versionHeader;
        struct intelgt_state_save_area regHeader;
    };
} // namespace SIP
