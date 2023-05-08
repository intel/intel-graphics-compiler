/*========================== begin_copyright_notice ============================
Copyright (C) 2017-2021 Intel Corporation

This software and the related documents are Intel copyrighted materials, and
your use of them is governed by the express license under which they were
provided to you ("License"). Unless the License provides otherwise, you may not
use, modify, copy, publish, distribute, disclose or transmit this software or
the related documents without Intel's prior written permission.

This software and the related documents are provided as is, with no express or
implied warranties, other than those that are expressly stated in the License.
============================= end_copyright_notice ===========================*/

/*!
 * @file GTPin interface with IGC driver
 */

#pragma once

#ifndef _GPTIN_IGC_INTERFACE_
#define _GPTIN_IGC_INTERFACE_

#include <stdint.h>

namespace gtpin {
namespace igc {
/* Please, follow these rules when updating interface:
 *
 * -    The igc_init_t structure should be compatible extension of the
 * igc_info_vN_t structure for each older version 'N' of the interface.
 * Compatible extension means that static_cast of a newer structure to an older
 * one is a valid operation.
 *
 * -    Normally, new interface versions do not modify definitions of existing
 * patch tokens. The recommended paractice is deprecation of an existing token
 * and defining a new one. In case an existing token has to be modified, make
 * sure the new version of the patch item is a compatible extension the older
 * one.
 */

// All structures defined in this header are 1-byte packed
#if defined(_MSC_VER)
#pragma pack(push, 1)
#define GTPIN_IGC_PACK_ATTRIBUTE
#elif defined(__GNUC__)
#define GTPIN_IGC_PACK_ATTRIBUTE __attribute__((__packed__))
#else
#error Unknown compiler
#endif

/*!
 * GTPin <-> IGC driver interface version
 */
static const uint32_t GTPIN_IGC_INTERFACE_VERSION = 5;

/*!
 * Tokens for patches IGC can pass to GTPin
 */
typedef enum {
  GTPIN_IGC_TOKEN_GRF_INFO = 0, // GRF usage in the kernel
  GTPIN_IGC_TOKEN_SCRATCH_AREA_INFO =
      1, // Scratch Space allocated for GTPin; since version 1
  GTPIN_IGC_TOKEN_KERNEL_START_INFO =
      2, // Kernel's payload information; since version 2
  GTPIN_IGC_TOKEN_NUM_GRF_REGS =
      3, // Number of per-thread GRF registers; since version 4
  GTPIN_IGC_TOKEN_INDIRECT_ACCESS_INFO =
      4 // Indirectly accessed GRF registers; since version 5
} GTPIN_IGC_TOKEN;

/*!
 * Structure that holds information (request) passed by GTPin to IGC on context
 * creation. The same structure carries the IGC response, as a part of the
 * igc_info_t block (see below) that is passed back to GTPin on kernel creation.
 * The structure is extendible, assuming that each addition to the structure is
 * reflected in the inteface version update. With any change, the 'version'
 * member should stay first in the structure. Below are supported versions and
 * the corresponding structures: GTPIN_IGC_INTERFACE_VERSION = 0      =>
 * igc_init_v0_t GTPIN_IGC_INTERFACE_VERSION = 1      => igc_init_v1_t
 * GTPIN_IGC_INTERFACE_VERSION = 2      => igc_init_v2_t
 * GTPIN_IGC_INTERFACE_VERSION = 3      => igc_init_v3_t
 * Current GTPIN_IGC_INTERFACE_VERSION  => igc_init_t
 */
typedef struct GTPIN_IGC_PACK_ATTRIBUTE igc_init_v0_s {
  uint32_t version; // Interface version. Must be the first member
  uint8_t re_ra;    // RERA - on/off NOTE: This is deprecated and has no effect.
  uint8_t grf_info; // Free GRF registers and/or indirect GRF accesses - on/off
  uint8_t srcline_mapping; // Source line mapping - on/off
  uint8_t padding[1];      // Alignment padding
} igc_init_v0_t;

typedef struct GTPIN_IGC_PACK_ATTRIBUTE igc_init_v1_s : igc_init_v0_t {
  uint8_t scratch_area_size; // Number of bytes in the Scratch Space to be
                             // reserved for GTPin
  uint8_t padding[3];        // Alignment padding
} igc_init_v1_t;

typedef struct GTPIN_IGC_PACK_ATTRIBUTE igc_init_v2_s : igc_init_v1_t {
} igc_init_v2_t;

typedef struct GTPIN_IGC_PACK_ATTRIBUTE igc_init_v3_s : igc_init_v0_t {
  uint32_t scratch_area_size; // Number of bytes in the Scratch Space to be
                              // reserved for GTPin
  uint32_t igc_init_size;     // Size of the igc_init_t structure in bytes:
                              // sizeof(igc_init_t)
} igc_init_v3_t;

typedef struct GTPIN_IGC_PACK_ATTRIBUTE igc_init_s : igc_init_v3_t {
} igc_init_t;

/*!
 * The information block passed by IGC to GTPin on kernel creation.
 * The block consists of so called "patches" - information items tagged by token
 * IDs The structure itself and the set of patches can be extended, assuming
 * that each such modification is reflected in the inteface version update. With
 * any change, the 'init_status' member should stay first in the structure.
 * Below are supported versions and the corresponding structures:
 * GTPIN_IGC_INTERFACE_VERSION = 0      => igc_info_v0_t
 * GTPIN_IGC_INTERFACE_VERSION = 1      => igc_info_v1_t
 * GTPIN_IGC_INTERFACE_VERSION = 2      => igc_info_v2_t
 * GTPIN_IGC_INTERFACE_VERSION > 2      => igc_info_t
 */
typedef struct GTPIN_IGC_PACK_ATTRIBUTE igc_info_v0_s {
  igc_init_v0_t init_status; // IGC-supported version and status of features
                             // requested by GTPin
  uint32_t num_tokens;       // Number of patch items in the information block
  // igc_token_header_t token_list[];     // Contiguous block of memory
  // containing patch items
} igc_info_v0_t;

typedef struct GTPIN_IGC_PACK_ATTRIBUTE igc_info_v1_s {
  igc_init_v1_t init_status; // IGC-supported version and status of features
                             // requested by GTPin
  uint32_t num_tokens;       // Number of patch items in the information block
  // igc_token_header_t token_list[];     // Contiguous block of memory
  // containing patch items
} igc_info_v1_t;

typedef struct GTPIN_IGC_PACK_ATTRIBUTE igc_info_v2_s {
  igc_init_v2_t init_status; // IGC-supported version and status of features
                             // requested by GTPin
  uint32_t num_tokens;       // Number of patch items in the information block
  // igc_token_header_t token_list[];     // Contiguous block of memory
  // containing patch items
} igc_info_v2_t;

typedef struct GTPIN_IGC_PACK_ATTRIBUTE igc_info_s {
  igc_init_t init_status; // IGC-supported version and status of features
                          // requested by GTPin
  uint32_t num_tokens;    // Number of patch items in the information block
  // igc_token_header_t token_list[];     // Contiguous block of memory
  // containing patch items
} igc_info_t;

/*!
 * Header of a patch item in the igc_info_t information block
 */
typedef struct GTPIN_IGC_PACK_ATTRIBUTE igc_token_header_t {
  uint32_t token;      // Token ID - a GTPIN_IGC_TOKEN value
  uint32_t token_size; // Total size of the patch item, including
                       // sizeof(igc_token_header_t)
} igc_token_header_t;

/*
 * Visualization example of the information block passed from IGC to GTPin.
 * The information block contains a patch item that represents the GRF usage
 * information
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
 * A sequence of consecutive bytes in GRF
 */
typedef struct GTPIN_IGC_PACK_ATTRIBUTE reg_sequence_t {
  uint16_t start_byte; // Index of the starting byte of the sequence in the
                       // linear GRF representation
  uint16_t num_consecutive_bytes; // Number of consecutive bytes in the
                                  // sequence, starting from start_byte
} reg_sequence_t;

/*!
 * A sequence of consecutive bytes in GRF referenced by the instruction
 */
typedef struct GTPIN_IGC_PACK_ATTRIBUTE ins_reg_range_t {
  uint32_t ins_offset;      // Offset of the instruction within the compilation
                            // object (kernel or function)
  reg_sequence_t reg_range; // A GRF range referenced by the instruction. Note,
                            // the instruction may access one or more GRF ranges
} ins_reg_range_t;

/*!
 * Patch item detailing unused registers in a kernel
 * Token ID = GTPIN_IGC_TOKEN_GRF_INFO
 */
typedef struct GTPIN_IGC_PACK_ATTRIBUTE igc_token_free_reg_info_t
    : igc_token_header_t {
  uint32_t magic_start; // Equal to 0xdeadd00d
  uint32_t num_items;   // Number of consecutive sequences of unused bytes
  // reg_sequence_t free_reg_sequence[];  // Array of 'num_items' sequences
  // detailing unused bytes uint32_t      magic_end              // Equal to
  // 0xdeadbeef
} igc_token_free_reg_info_t;

/*!
 * Patch item detailing area in the System Scratch Memory Space reserved for
 * GTPin. Token ID = GTPIN_IGC_TOKEN_SCRATCH_AREA_INFO
 */
typedef struct GTPIN_IGC_PACK_ATTRIBUTE igc_token_scratch_area_info_t
    : igc_token_header_t {
  uint32_t scratch_area_offset; // Offset of the GTPin's scratch area in the
                                // System Scratch Memory Space. It is assumed to
                                // be 32-byte aligned value.
  union { // Actual number of bytes in the Scratch Space, reserved for GTPin
    uint8_t scratch_area_size_v1; // Since version 1
    uint32_t scratch_area_size;   // Since version 3
  };
} igc_token_scratch_area_info_t;

/*!
 * Patch item detailing the sizes of per-thread and cross-thread payload.
 * Token ID = GTPIN_IGC_TOKEN_KERNEL_START_INFO
 */
typedef struct GTPIN_IGC_PACK_ATTRIBUTE igc_token_kernel_start_info_t
    : igc_token_header_t {
  uint32_t per_thread_prolog_size;   // The size of Per-Thread payload Prolog.
  uint32_t cross_thread_prolog_size; // The size of Cross-Thread payload Prolog.
} igc_token_kernel_start_info_t;

/*!
 * Patch item detailing the number of GRF registers allocated for each thread of
 * the kernel. Token ID = GTPIN_IGC_TOKEN_NUM_GRF_REGS
 */
typedef struct GTPIN_IGC_PACK_ATTRIBUTE igc_token_num_grf_regs_t
    : igc_token_header_t {
  uint32_t num_grf_regs; // Number of GRF registers allocated for each thread of
                         // the kernel
} igc_token_num_grf_regs_t;

/*!
 * Patch item that holds information about indirect GRF accesses in the
 * compilation object (kernel or function) Token ID =
 * GTPIN_IGC_TOKEN_INDIRECT_ACCESS_INFO
 */
typedef struct GTPIN_IGC_PACK_ATTRIBUTE igc_token_indirect_access_info_t
    : igc_token_header_t {
  uint32_t num_ranges; // Number of indirectly referenced GRF ranges. Number of
                       // elements in the immediately following 'ins_reg_ranges'
                       // array
  // ins_reg_range_t  ins_reg_ranges[];   // Array of ins_reg_range_t elements
  // detailing indirect GRF acceses in the compilation object (kernel or
  // function)
} igc_token_indirect_access_info_t;

#if defined(_MSC_VER)
#pragma pack(pop)
#undef GTPIN_IGC_PACK_ATTRIBUTE
#elif defined(__GNUC__)
#undef GTPIN_IGC_PACK_ATTRIBUTE
#endif
} // namespace igc
} // namespace gtpin

#endif