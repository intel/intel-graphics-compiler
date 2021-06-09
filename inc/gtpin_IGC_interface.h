/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#ifndef _GPTIN_IGC_INTERFACE_
#define _GPTIN_IGC_INTERFACE_

namespace gtpin
{
namespace igc
{

/*!
 * GTPin <-> IGC driver interface version
 */
static const uint32_t GTPIN_IGC_INTERFACE_VERSION = 0;


/*!
 * Tokens for patches IGC can pass to GTPin
 */
typedef enum
{
    GTPIN_IGC_TOKEN_GRF_INFO
    //TBD - more tokens
} GTPIN_IGC_TOKEN;


/*
 * IGC initialization data
 */
typedef struct igc_init_t
{
    uint32_t    version;
    uint8_t     re_ra;                  // active RERA - on/off
    uint8_t     grf_info;               // active free GRF info - on/off
    uint8_t     srcline_mapping;        // active source line mapping - on/off
} igc_init_t;

/*!
 * General header for patch item passed from IGC to GTPin
 * Visualization example of the patch item representing the free GRF information
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
typedef struct igc_token_header_t
{
    GTPIN_IGC_TOKEN     token;          // Token ID
    uint32_t            token_size;     // Total size of the patch item. Equal to sizeof(token) + sizeof(token_size) + sizeof(*patch*)
} igc_token_header_t;


/*!
 * A single sequence of consecutive bytes, all of which are unused in the current kernel.
 */
typedef struct reg_sequence_t
{
    uint32_t start_byte;            // Starting byte in the GRF table, representing the start of unused bytes in the current kernel
    uint32_t num_consecutive_bytes; // Number of consecutive bytes in the sequence, starting from start_byte
} reg_sequence_t;


/*!
 * Patch item detailing unused registers in the current kernel
 */
typedef struct igc_token_free_reg_info_t : igc_token_header_t
{
    uint32_t        magic_start;                        // Equal to 0xdeadd00d
    uint32_t        num_items;                          // Number of consecutive sequences of unused bytes
    // reg_sequence_t  free_reg_sequence[/* num_items */]; // Inlined array of num_items sequences detailing unused bytes
    //uint32_t      magic_end                           // Equal to 0xdeadbeef
} igc_token_free_reg_info_t;

/**
 * IGC's additional information about the kernel's execution
 */
typedef struct igc_info_s
{
    igc_init_t          init_status;                    // IGC's report regarding the various requested features from GTPin
    uint32_t            num_tokens;                     // Number of patch items passed in the struct.
    //igc_token_header_t  token_list[];                 // Contiguous block of memory containing the patch tokens
} igc_info_t;

} // namespace igc
} // namespace gtpin

#endif
