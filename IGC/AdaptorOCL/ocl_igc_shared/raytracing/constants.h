/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file stores constants that we use in OpenCL-C builtins for Raytracing and share with Neo.

#pragma once

// Dispatch globals passed as an array will be aligned up to page size = 64 kilobytes.
const int DISPATCH_GLOBALS_STRIDE = 65536;
