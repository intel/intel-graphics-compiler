/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __SPIRV_ATOMICS_COMMON_H__
#define __SPIRV_ATOMICS_COMMON_H__

typedef enum
{
    CrossDevice = 0,
    Device      = 1,
    Workgroup   = 2,
    Subgroup    = 3,
    Invocation  = 4
} Scope_t;

typedef enum
{
    Relaxed                = 0x0,
    Acquire                = 0x2,
    Release                = 0x4,
    AcquireRelease         = 0x8,
    SequentiallyConsistent = 0x10,
    UniformMemory          = 0x40,
    SubgroupMemory         = 0x80,
    WorkgroupMemory        = 0x100,
    CrossWorkgroupMemory   = 0x200,
    AtomicCounterMemory    = 0x400,
    ImageMemory            = 0x800
} MemorySemantics_t;

#endif // __SPIRV_ATOMICS_COMMON_H__
