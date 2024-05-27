/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This file defines sycl libdevice implementation of functions in libc stdlib.h
// Function naming style following libc function implementation in
// https://github.com/intel/llvm/tree/sycl/libdevice
//

// Thread exit function
void __devicelib_exit();
