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

#ifndef _GPTIN_IGC_INTERFACE_
#define _GPTIN_IGC_INTERFACE_

#include <stdint.h>

namespace gtpin
{
    namespace igc
    {

        // Possible bug fix (if one would happen): Currently there are no padding rules on the following structs, meaning
        // the compiler could potentially create a non/padded struct which conflicts with our reading of same struct.
        // If that's the case, contact compiler team to explicitly remove padding from the structs (and uncomment the following line).
        //#pragma pack(push, 1) // exact fit - no padding

        /*!
        * GTPin <-> IGC driver interface version
        */
        static const uint32_t GTPIN_IGC_INTERFACE_VERSION = 2;


        /*!
        * Tokens for patches IGC can pass to GTPin
        */
        typedef enum
        {
            GTPIN_IGC_TOKEN_GRF_INFO,
            GTPIN_IGC_TOKEN_SCRATCH_AREA_INFO,
            GTPIN_IGC_TOKEN_KERNEL_START_INFO,
            //TBD - more tokens
        } GTPIN_IGC_TOKEN;


        /*!
        * Structure that holds information (request) passed by GTPin to IGC on context creation.
        * The same structure carries the IGC response, as a part of the igc_info_t block (see below)
        * that is passed back to GTPin on kernel creation.
        * The structure is extendible, assuming that each addition to the structure is
        * reflected in the inteface version update. With any change, the 'version' member should stay
        * first in the structure.
        * Below are supported versions and the corresponding structures:
        * GTPIN_IGC_INTERFACE_VERSION = 0      => igc_init_v0_t
        * GTPIN_IGC_INTERFACE_VERSION = 1      => igc_init_v1_t
        * Current GTPIN_IGC_INTERFACE_VERSION  => igc_init_t
        */
        typedef struct igc_init_v0_s
        {
            uint32_t    version;                // Interface version. Must be the first member
            uint8_t     re_ra;                  // Active RERA - on/off
            uint8_t     grf_info;               // Active free GRF info - on/off
            uint8_t     srcline_mapping;        // Active source line mapping - on/off
        } igc_init_v0_t;

        typedef struct igc_init_v1_s : igc_init_v0_t
        {
            uint8_t     scratch_area_size;      // Number of bytes in the Scratch Space to be reserved for GTPin
        } igc_init_v1_t;

        typedef igc_init_v1_t igc_init_v2_t;

        typedef struct igc_init_s : igc_init_v2_t {} igc_init_t;

        /*!
        * The information block passed by IGC to GTPin on kernel creation.
        * The block consists of so called "patches" - information items tagged by token IDs
        * The structure itself and the set of patches can be extended, assuming that each such
        * modification is reflected in the inteface version update. With any change, the
        * 'init_status' member should stay first in the structure.
        * Below are supported versions and the corresponding structures:
        * GTPIN_IGC_INTERFACE_VERSION = 0      => igc_info_v0_t
        * GTPIN_IGC_INTERFACE_VERSION = 1      => igc_info_v1_t
        * Current GTPIN_IGC_INTERFACE_VERSION  => igc_info_t
        */
        typedef struct igc_info_v0_s
        {
            igc_init_v0_t       init_status;        // IGC-supported version and status of features requested by GTPin
            uint32_t            num_tokens;         // Number of patch items in the information block
                                                    // igc_token_header_t token_list[];     // Contiguous block of memory containing patch items
        } igc_info_v0_t;

        typedef struct igc_info_v1_s
        {
            igc_init_v1_t       init_status;        // IGC-supported version and status of features requested by GTPin
            uint32_t            num_tokens;         // Number of patch items in the information block
                                                    // igc_token_header_t token_list[];     // Contiguous block of memory containing patch items
        } igc_info_v1_t;

        typedef struct igc_info_v2_s
        {
            igc_init_v2_t       init_status;        // IGC-supported version and status of features requested by GTPin
            uint32_t            num_tokens;         // Number of patch items in the information block
            // igc_token_header_t token_list[];     // Contiguous block of memory containing patch items
        } igc_info_v2_t;

        typedef struct igc_info_s : igc_info_v2_t {} igc_info_t;

        /*!
        * Header of a patch item in the igc_info_t information block
        */
        typedef struct igc_token_header_t
        {
            GTPIN_IGC_TOKEN     token;          // Token ID
            uint32_t            token_size;     // Total size of the patch item, including sizeof(igc_token_header_t)
        } igc_token_header_t;

        /*
        * Visualization example of the information block passed from IGC to GTPin.
        * The information block contains a patch item that represents the GRF usage information
        * +---------------------+ igc_info_t +-----------------------+
        * |                                                          |
        * | igc_init_t   init_status                                 |
        * | uint32_t     num_tokens                                  |
        * |                                                          |
        * | +----------------+ igc_token_header_t +------------------+
        * | |                                                        |
        * | |  GTPIN_IGC_TOKEN    token                              |
        * | |  uint32_t           token_size                         |
        * | |                                                        |
        * | |  +------------+ igc_token_free_reg_info_t +------------+
        * | |  |                                                     |
        * | |  | uint32_t        magic_start                         |
        * | |  | uint32_t        num_items                           |
        * | |  | reg_sequence_t  free_reg_sequence[num_items]        |
        * | |  | uint32_t        magic_end                           |
        * | |  |                                                     |
        * +----------------------------------------------------------+
        */

        /*!
        * A sequence of consecutive bytes, all of which are unused in the current kernel.
        */
        typedef struct reg_sequence_t
        {
            uint16_t start_byte;                // Index of the starting byte of the sequence in the linear GRF representation
            uint16_t num_consecutive_bytes;     // Number of consecutive bytes in the sequence, starting from start_byte
        } reg_sequence_t;


        /*!
        * Patch item detailing unused registers in a kernel
        * Token ID = GTPIN_IGC_TOKEN_GRF_INFO
        */
        typedef struct igc_token_free_reg_info_t : igc_token_header_t
        {
            uint32_t        magic_start;            // Equal to 0xdeadd00d
            uint32_t        num_items;              // Number of consecutive sequences of unused bytes
                                                    // reg_sequence_t free_reg_sequence[];  // Array of 'num_items' sequences detailing unused bytes
                                                    // uint32_t      magic_end              // Equal to 0xdeadbeef
        } igc_token_free_reg_info_t;


        /*!
        * Patch item detailing area in the System Scratch Memory Space reserved for GTPin
        * Token ID = GTPIN_IGC_TOKEN_SCRATCH_AREA_INFO
        */
        typedef struct igc_token_scratch_area_info_t : igc_token_header_t
        {
            uint32_t    scratch_area_offset;    // Offset of the GTPin's scratch area in the System Scratch Memory Space.
                                                // It is assumed to be 32-byte aligned value.
            uint8_t     scratch_area_size;      // Actual number of bytes in the Scratch Space, reserved for GTPin
        } igc_token_scratch_area_info_t;

        /*!
         * Patch item detailing the sizes of per-thread and cross-thread dataload.
         * Token ID = GTPIN_IGC_TOKEN_KERNEL_START_INFO
         */
        typedef struct igc_token_kernel_start_info_t : igc_token_header_t
        {
            uint32_t    per_thread_prolog_size;    // The size of Per-Thread DataLoad Prolog.
            uint32_t    cross_thread_prolog_size;  // The size of Cross-Thread DataLoad Prolog.
        } igc_token_kernel_start_info_t;

        // See comment at top of file regarding the commented pragma.
        //#pragma pack(pop)

    } // namespace igc
} // namespace gtpin

#endif
