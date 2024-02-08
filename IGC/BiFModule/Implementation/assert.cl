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

enum ERROR_TYPE {
  ERROR_TYPE_NONE = 0,
  ERROR_TYPE_ASSERT = 1,
  ERROR_TYPE_STACK_OVERFLOW = 2,
  ERROR_TYPE_BUFFER_OUTOFBOUNDS = 3,
};

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
    header->flag = ERROR_TYPE_ASSERT;
    global char* buf = (global char*) header;
    __builtin_IB_printf_to_buffer(buf, buf + 8, header->size, "%s:%d: %s: global id: [%lu,%lu,%lu], local id: [%lu,%lu,%lu] Assertion `%s` failed.\n", file, line, func, gid0, gid1, gid2, lid0, lid1, lid2, expr);
    __builtin_IB_software_exception();
}

ulong __builtin_IB_get_stack_pointer();
int __builtin_IB_get_stack_size_per_thread();

// This function needs to be inserted in entry points.
// Assert buffer mechanism is used for stack overflow detection, because we can reuse much code this way:
//  - We need some kind of temp buffer to store the stack size per thread and stack base for each thread.
//    Assert buffer is used, so that we don't need to allocate additional one.
//  - Runtime checks for nonzero value in the "flag" of AssertBufferHeader and then aborts.
//    This is what we want for stack overflow scenario as well.
void __stackoverflow_init() {
    int HWTID = __builtin_IB_hw_thread_id();
    global volatile int* buf =  (global volatile int*)__builtin_IB_get_assert_buffer();
    // Go to first int after the AssertBufferHeader
    buf += 3;
    *buf = __builtin_IB_get_stack_size_per_thread();
    buf = buf + 1;
    global ulong* stackBase = ((global ulong*)(buf));
    stackBase[HWTID] = __builtin_IB_get_stack_pointer();
}

// This function needs to be inserted in places where stack pointer is incremented.
void __stackoverflow_detection() {
    int HWTID = __builtin_IB_hw_thread_id();
    // +12 will be stack size per thread.
    global volatile int* buf =  (global volatile int*)__builtin_IB_get_assert_buffer();
    buf += 3;
    int stackSizePerThread = *buf;
    buf += 1;
    ulong stackBase = ((global ulong*)(buf))[HWTID];

    if (__builtin_IB_get_stack_pointer() - stackBase > stackSizePerThread) {
        global volatile AssertBufferHeader* header = (global volatile AssertBufferHeader*) __builtin_IB_get_assert_buffer();
        printf("Stack overflow detected!\n");
        header->flag = ERROR_TYPE_STACK_OVERFLOW;
        __builtin_IB_software_exception();
    }
}

// TODO(mateuszchudyk): Add globalID.
void __bufferoutofbounds_assert(const char* file, int line, int column, const char* bufferName, long bufferOffsetInBytes) {
    printf("[ERROR] Buffer offset is out of bounds!\n"
           "   Location:    %s:%d:%d\n"
           "   Address:     %s + 0x%X\n",
           file, line, column, bufferName, bufferOffsetInBytes);

    AssertBufferHeader* header = __builtin_IB_get_assert_buffer();
    header->flag = ERROR_TYPE_BUFFER_OUTOFBOUNDS;
    __builtin_IB_software_exception();
}

void __bufferoutofbounds_assert_nodebug(long bufferAddress, long bufferOffsetInBytes) {
    printf("[ERROR] Buffer offset is out of bounds!\n"
           "   Address:     0x%X + 0x%X\n",
           bufferAddress, bufferOffsetInBytes);

    AssertBufferHeader* header = __builtin_IB_get_assert_buffer();
    header->flag = ERROR_TYPE_BUFFER_OUTOFBOUNDS;
    __builtin_IB_software_exception();
}
