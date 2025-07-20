/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace SIP {
struct alignas(4) version {
  uint8_t major;
  uint8_t minor;
  uint16_t patch;
};

struct alignas(8) StateSaveArea {
  char magic[8] = "tssarea";
  uint64_t reserved1;
  struct version version;
  uint8_t size;
  uint8_t reserved2[3];
};

// Debug SIP will dump this data in debug surface.
struct sr_ident {
  char magic[8] = "srmagic";
  struct version version;
  uint8_t count;
  uint8_t reserved[7];
};

struct alignas(8) regset_desc {
  uint32_t offset;
  uint16_t num;
  uint16_t bits;
  uint16_t bytes;
};

#define EXCHANGE_BUFFER_SIZE 112

struct sip_command {
  uint32_t command;
  uint32_t size;
  uint64_t offset;
  uint8_t buffer[EXCHANGE_BUFFER_SIZE];
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
  struct regset_desc ce;
  struct regset_desc sp;
  struct regset_desc cmd;
  struct regset_desc tm;
  struct regset_desc fc;
  struct regset_desc dbg;
  struct regset_desc ctx;
  struct regset_desc dbg_reg;
};

struct StateSaveAreaHeader {
  struct StateSaveArea versionHeader;
  struct intelgt_state_save_area regHeader;
};

struct fifo_node {
  uint8_t valid;
  uint8_t thread_id;
  uint8_t eu_id;
  uint8_t subslice_id;
  uint8_t slice_id;
  uint8_t reserved[3];
};

enum SIP_FLAGS { SIP_FLAG_HEAPLESS = 1 };

struct alignas(8) intelgt_state_save_area_V3 {
  uint32_t num_slices;
  uint32_t num_subslices_per_slice;
  uint32_t num_eus_per_subslice;
  uint32_t num_threads_per_eu;
  uint32_t state_area_offset;
  uint32_t state_save_size;
  uint32_t slm_area_offset;
  uint32_t slm_bank_size;
  uint32_t reserved0;
  uint32_t sr_magic_offset;
  uint32_t fifo_offset;
  uint32_t fifo_size;
  uint32_t fifo_head;
  uint32_t fifo_tail;
  uint32_t fifo_version;
  uint32_t reserved1[10];
  uint32_t sip_flags;
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
  struct regset_desc ce;
  struct regset_desc sp;
  struct regset_desc cmd;
  struct regset_desc tm;
  struct regset_desc fc;
  struct regset_desc dbg;
  struct regset_desc ctx;
  struct regset_desc dbg_reg;
  struct regset_desc scalar;
  struct regset_desc msg;
};

// versionHeader.version.major = 3
struct StateSaveAreaHeaderV3 {
  struct StateSaveArea versionHeader;
  struct intelgt_state_save_area_V3 regHeader;
};

// versionHeader.version.major = 4
struct StateSaveAreaHeaderV4 {
  struct StateSaveArea versionHeader;
  uint64_t total_wmtp_data_size;
};

} // namespace SIP
