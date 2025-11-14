/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, llvm-16-plus
// RUN: ocloc compile -file %s -device dg2 -options " -igc_opts 'PrintToConsole=1 PrintAfter=StatelessToStateful'" 2>&1 | FileCheck %s

// This test verifies whether the bufferOffset argument is used when the base pointer is not guaranteed to be aligned to 4 bytes.
// Our hardware requires the base pointer to be aligned to 4 bytes for stateful accesses.
// Therefore, to enable stateful addressing for accesses on char and short pointers,
// the UMD aligns the base pointer to 4 bytes and passes the difference between the original pointer and the aligned pointer in the bufferOffset argument.
// This test is an edge case where a char pointer is used simultaneously by loads that access both char and int types.
// The intention is to confirm that the int load does not interfere with our alignment analysis—meaning
// the base pointer should not be treated as 4-byte aligned, and thus bufferOffset should always be used.
// Otherwise, we risk passing a base pointer that is not 4-byte aligned to the hardware, which is not supported.

__kernel void store_to_buffer(__global char* buffer, char a, char b)
{
    // Check if the buffer address itself is 4-byte aligned
    if (((uintptr_t)buffer & 0x3) == 0)
    {
        // Aligned: store an int
        *(__global int*)buffer = 0x12345678;
    } else
    {
        // Unaligned: store 4 chars
        buffer[0] = a;
        buffer[10] = b;
        buffer[444] = b+1;
        buffer[999] = a+2;
    }
}

// CHECK: define spir_kernel void @store_to_buffer({{ptr|i8}} addrspace(1){{.*}} align 1 %buffer
// CHECK: add i32 %bufferOffset, 10
// CHECK: add i32 %bufferOffset, 444
// CHECK: add i32 %bufferOffset, 999
