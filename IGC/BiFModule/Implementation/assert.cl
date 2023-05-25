/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// __devicelib_assert_fail implementation.
// This is a DPCPP device library extension described in
// https://github.com/triSYCL/sycl/blob/sycl/unified/master/sycl/doc/design/Assert.md
// DPCPP assert(expr) macro ends up in call to __devicelib_assert_fail.
// Per design, the implementation needs to:
//    1. Write "1" to flag field in the assert buffer header. The buffer header is defined below.
//    2. Print the assert message to assert buffer. This is done in a similar fashion as "printf".
//       __builtin_IB_printf_to_buffer builtin function is used for this purpose - it basically wraps
//       the printf functionality to a buffer provided as an argument.
//    3. Trigger a software exception by setting appropriate Control Register bits.
//       This is done by __builtin_IB_software_exception builtin.
//    4. We need to ensure that all stores in this function are uncached.
//       This is handled by NontemporalLoadsAndStoresInAssert pass.

#include "IBiF_Header.cl"

__global volatile uchar* __builtin_IB_get_assert_buffer();
void __builtin_IB_software_exception();
int __builtin_IB_printf_to_buffer(global char* buf, global char* currentOffset, int bufSize, ...);

typedef struct {
    int size;
    int flag;
    int begin;
} AssertBufferHeader;

void __devicelib_assert_fail(char *expr, char *file, int line, char *func, long gid0, long gid1, long gid2, long lid0, long lid1, long lid2) {
    AssertBufferHeader* header = (AssertBufferHeader*) __builtin_IB_get_assert_buffer();
    header->flag = 1;
    global char* buf = (global char*) header;
    __builtin_IB_printf_to_buffer(buf, buf + 8, header->size, "%s:%d: %s: global id: [%lu,%lu,%lu], local id: [%lu,%lu,%lu] Assertion `%s` failed\n", file, line, func, gid0, gid1, gid2, lid0, lid1, lid2, expr);
    __builtin_IB_software_exception();
}
