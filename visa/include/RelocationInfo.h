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
**  -------------------------------------------------------------------------- */
#ifndef RELOCATION_INFO_H
#define RELOCATION_INFO_H

#include <string>

namespace vISA {

static const uint32_t MAX_SYMBOL_NAME_LENGTH = 1024;

/// GenSymType - Specify the symbol's type
enum GenSymType {
    S_NOTYPE           = 0,    // The symbol's type is not specified
    S_UNDEF            = 1,    // The symbol is undefined in this module
    S_FUNC             = 2,    // The symbol is associated with a function
    S_GLOBAL_VAR       = 3,    // The symbol is associated with a global variable in global address space
    S_GLOBAL_VAR_CONST = 4,    // The symbol is associated with a global variable in constant address space
    S_CONST_SAMPLER    = 5,    // The symbol is associated with a constant sampler
    S_KERNEL           = 6     // The symbol is associated with a kernel function
};

/// GenSymEntry - An symbol table entry
typedef struct {
    uint32_t   s_type;            // The symbol's type
    uint32_t   s_offset;          // The binary offset of this symbol. This field is ignored if s_type is S_UNDEF
    uint32_t   s_size;            // The size in bytes of the function binary
    char       s_name[MAX_SYMBOL_NAME_LENGTH]; // The symbol's name
} GenSymEntry;

/// GenRelocType - Specify the relocation's type
enum GenRelocType {
    R_NONE = 0,
    R_SYM_ADDR                     = 1, // 64-bit type address
    R_SYM_ADDR_32                  = 2, // 32-bit address or lower 32-bit of a 64-bit address.
    R_SYM_ADDR_32_HI               = 3, // higher 32bits of 64-bit address
    R_PER_THREAD_PAYLOAD_OFFSET_32 = 4, // 32-bit field of payload offset of per-thread data
    R_GLOBAL_IMM_32                = 5  // 32-bit global immediate
};

/// GenRelocEntry - An relocation table entry
typedef struct {
    uint32_t   r_type;        // The relocation's type
    uint32_t   r_offset;      // The binary offset of the relocated target
    char       r_symbol[MAX_SYMBOL_NAME_LENGTH]; // The relocation target symbol's name
} GenRelocEntry;

/// GenFuncAttribEntry - Per-function attribute entry
typedef struct {
    uint8_t    f_isKernel;      // Is the function a kernel
    uint8_t    f_hasBarrier;    // Does the function use barriers
    uint32_t   f_privateMemPerThread; // Total private memory (in bytes) used by this function per thread
    uint32_t   f_spillMemPerThread;  // Spill mem used (in bytes) in scratch space for this function
    char       f_name[MAX_SYMBOL_NAME_LENGTH]; // The function's name
} GenFuncAttribEntry;

// HostAccessEntry - per global variable host access entry
typedef struct {
    char       device_name[MAX_SYMBOL_NAME_LENGTH];
    char       host_name[MAX_SYMBOL_NAME_LENGTH];
} HostAccessEntry;

/// FIXME: ZE*Entry information should be moved to upper level (e.g. IGC or runtime interface)

/// ZESymEntry - An symbol entry that will later be transformed to ZE binary format
/// It contains the same information as GenSymEntry, and has the full symbol name with
/// no length limitation
/// FIXME: s_type should be standard ELF symbol type instead of GenSymType
struct ZESymEntry {
    GenSymType    s_type;            // The symbol's type
    uint32_t      s_offset;          // The binary offset of this symbol. This field is ignored if s_type is S_UNDEF
    uint32_t      s_size;            // The size in bytes of the function binary
    std::string   s_name;            // The symbol's name

    ZESymEntry() = default;
    ZESymEntry(GenSymType type, uint32_t offset, uint32_t size, std::string name)
        : s_type(type), s_offset(offset), s_size(size), s_name(name)
    {}
};

/// ZERelocEntry - A relocation entry that will later be transformed to ZE binary format
/// It contains the same information as GenRelocEntry, and has the full symbol name with
/// no length limitation
/// FIXME: r_type should be standard ELF symbol type instead of GenRelocType
struct ZERelocEntry {
    GenRelocType  r_type;        // The relocation's type
    uint32_t      r_offset;      // The binary offset of the relocated target
    std::string   r_symbol;      // The relocation target symbol's name

    ZERelocEntry() = default;
    ZERelocEntry(GenRelocType type, uint32_t offset, std::string targetSymName)
        : r_type(type), r_offset(offset), r_symbol(targetSymName)
    {}
};

/// ZEFuncAttribEntry - A function attribute entry that will later be transformed to ZE binary format
struct ZEFuncAttribEntry {
    uint8_t     f_isKernel;      // Is the function a kernel
    uint8_t     f_hasBarrier;    // Does the function use barriers
    uint32_t    f_privateMemPerThread; // Total private memory (in bytes) used by this function per thread
    uint32_t    f_spillMemPerThread;  // Spill mem used (in bytes) in scratch space for this function
    std::string f_name; // The function's name

    ZEFuncAttribEntry(uint8_t isKernel, uint8_t hasBarrier, uint32_t privateMemPerThread,
        uint32_t spillMemPerThread, std::string funcName)
        : f_isKernel(isKernel),
          f_hasBarrier(hasBarrier),
          f_privateMemPerThread(privateMemPerThread),
          f_spillMemPerThread(spillMemPerThread),
          f_name(funcName)
    {}
};

/// ZEHostAccessEntry - A host access entry that will later be transformed to ZE binary format.
/// It contains a global variable host name that can be used by Runtime to identify a global variable.
/// It gives an ability to read/write from/to global variables from host level.
struct ZEHostAccessEntry {
    std::string device_name;
    std::string host_name;
};

} //namespace vISA
#endif
