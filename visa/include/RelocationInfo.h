/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*  ---------------------------------------------------------------------------
**
**  File Name     : RelocationInfo.h
**
**  Abastract     : This file contains the definition of Relocation Table and
**                  Symbol Table which are shared between Compiler and Driver
**  --------------------------------------------------------------------------
 */
#ifndef RELOCATION_INFO_H
#define RELOCATION_INFO_H

#include <cstdint>
#include <string>

namespace vISA {

static const uint32_t MAX_SYMBOL_NAME_LENGTH = 1024;
constexpr const char *CROSS_THREAD_OFF_R0_RELOCATION_NAME =
    "__INTEL_PATCH_CROSS_THREAD_OFFSET_OFF_R0";
constexpr const char *PER_THREAD_OFF_RELOCATION_NAME =
    "__INTEL_PER_THREAD_OFF";

/// GenSymType - Specify the symbol's type
enum GenSymType {
  S_NOTYPE = 0,     // The symbol's type is not specified
  S_UNDEF = 1,      // The symbol is undefined in this module
  S_FUNC = 2,       // The symbol is associated with a function
  S_GLOBAL_VAR = 3, // The symbol is associated with a global variable in global
                    // address space
  S_GLOBAL_VAR_CONST = 4, // The symbol is associated with a global variable in
                          // constant address space
  S_CONST_SAMPLER = 5,    // The symbol is associated with a constant sampler
  S_KERNEL = 6            // The symbol is associated with a kernel function
};

/// GenSymEntry - An symbol table entry
/// Deprecated, use ZESymEntry instead
typedef struct {
  uint32_t s_type;   // The symbol's type
  size_t s_offset; // The binary offset of this symbol. This field is ignored
                     // if s_type is S_UNDEF
  uint32_t s_size;   // The size in bytes of the function binary
  char s_name[MAX_SYMBOL_NAME_LENGTH]; // The symbol's name
} GenSymEntry;

/// GenRelocType - Specify the relocation's type
enum GenRelocType {
  R_NONE = 0,
  R_SYM_ADDR = 1,       // 64-bit type address
  R_SYM_ADDR_32 = 2,    // 32-bit address or lower 32-bit of a 64-bit address.
  R_SYM_ADDR_32_HI = 3, // higher 32 bits of 64-bit address
  R_PER_THREAD_PAYLOAD_OFFSET_32 = 4, // *** Deprecated. Do Not Use. ***
  R_GLOBAL_IMM_32 = 5, // 32-bit global immediate
  R_SEND = 6, // send instruction offset, used for BTI patching
  R_SYM_ADDR_16 = 7 // 16-bit address or immediate
};

/// GenRelocEntry - An relocation table entry
/// Deprecated, use ZERelocEntry instead
typedef struct {
  uint32_t r_type;   // The relocation's type
  uint32_t r_offset; // The binary offset of the relocated target
  char r_symbol[MAX_SYMBOL_NAME_LENGTH]; // The relocation target symbol's name
} GenRelocEntry;

// HostAccessEntry - per global variable host access entry
typedef struct {
  char device_name[MAX_SYMBOL_NAME_LENGTH];
  char host_name[MAX_SYMBOL_NAME_LENGTH];
} HostAccessEntry;

/// FIXME: ZE*Entry information should be moved to upper level (e.g. IGC or
/// runtime interface)

/// ZESymEntry - An symbol entry that will later be transformed to ZE binary
/// format It contains the same information as GenSymEntry, and has the full
/// symbol name with no length limitation
/// FIXME: s_type should be standard ELF symbol type instead of GenSymType
struct ZESymEntry {
  GenSymType s_type;  // The symbol's type
  size_t s_offset;  // The binary offset of this symbol. This field is ignored
                      // if s_type is S_UNDEF
  uint32_t s_size;    // The size in bytes of the function binary
  std::string s_name; // The symbol's name

  ZESymEntry() = default;
  ZESymEntry(GenSymType type, size_t offset, uint32_t size, std::string name)
      : s_type(type), s_offset(offset), s_size(size), s_name(std::move(name)) {}
};

/// ZERelocEntry - A relocation entry that will later be transformed to ZE
/// binary format It contains the same information as GenRelocEntry, and has the
/// full symbol name with no length limitation
/// FIXME: r_type should be standard ELF symbol type instead of GenRelocType
struct ZERelocEntry {
  GenRelocType r_type;  // The relocation's type
  uint32_t r_offset;    // The binary offset of the relocated target
  std::string r_symbol; // The relocation target symbol's name

  ZERelocEntry() = default;
  ZERelocEntry(GenRelocType type, uint32_t offset, std::string targetSymName)
      : r_type(type), r_offset(offset), r_symbol(std::move(targetSymName)) {}
};

/// ZEFuncAttribEntry - A function attribute entry that will later be
/// transformed to ZE binary format
struct ZEFuncAttribEntry {
  uint8_t f_isKernel;             // Is the function a kernel
  uint8_t f_isExternal;           // Is the function external
  uint32_t f_BarrierCount;        // Number of barriers used by the function
  uint32_t f_privateMemPerThread; // Total private memory (in bytes) used by
                                  // this function per thread
  uint32_t f_spillMemPerThread;   // Spill mem used (in bytes) in scratch space
                                  // for this function
  std::string f_name;             // The function's name
  uint8_t f_hasRTCalls;
  uint8_t f_hasPrintfCalls;
  uint8_t f_hasIndirectCalls;

  ZEFuncAttribEntry(uint8_t isKernel, uint8_t isExternal, uint32_t barrierCount,
                    uint32_t privateMemPerThread, uint32_t spillMemPerThread,
                    std::string funcName, uint8_t hasRTCalls,
                    uint8_t hasPrintfCalls, uint8_t hasIndirectCalls)
      : f_isKernel(isKernel), f_isExternal(isExternal),
        f_BarrierCount(barrierCount),
        f_privateMemPerThread(privateMemPerThread),
        f_spillMemPerThread(spillMemPerThread), f_name(std::move(funcName)),
        f_hasRTCalls(hasRTCalls), f_hasPrintfCalls(hasPrintfCalls),
        f_hasIndirectCalls(hasIndirectCalls) {}
};

/// ZEHostAccessEntry - A host access entry that will later be transformed to ZE
/// binary format. It contains a global variable host name that can be used by
/// Runtime to identify a global variable. It gives an ability to read/write
/// from/to global variables from host level.
struct ZEHostAccessEntry {
  std::string device_name;
  std::string host_name;
};

} // namespace vISA
#endif
