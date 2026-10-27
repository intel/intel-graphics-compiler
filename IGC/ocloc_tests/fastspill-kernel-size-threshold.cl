/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, bmg-supported
// UNSUPPORTED: system-windows

// Test that -fasterRA (vISA_FastSpill) is enabled for OpenCL kernels whose post-trimming expanded size
// exceeds FastSpillKernelSizeThreshold and only for those.

// RUN: rm -rf %t.low %t.high %t.low.out %t.high.out

// Threshold below the kernel size -> -fasterRA.
// RUN: env IGC_FastSpillKernelSizeThreshold=1 IGC_ShaderDumpEnable=1 IGC_DumpToCustomDir=%t.low \
// RUN: IGC_VISAOptions=-dumpVisaOptionsAll ocloc compile -file %s -device bmg -out_dir %t.low.out
// RUN: cat %t.low/*.visaasm | FileCheck %s --check-prefix=CHECK-ON

// RUN: env IGC_FastSpillKernelSizeThreshold=100000 IGC_ShaderDumpEnable=1 IGC_DumpToCustomDir=%t.high \
// RUN: IGC_VISAOptions=-dumpVisaOptionsAll ocloc compile -file %s -device bmg -out_dir %t.high.out
// RUN: cat %t.high/*.visaasm | FileCheck %s --check-prefix=CHECK-OPTS
// RUN: cat %t.high/*.visaasm | not grep -q -e "-fasterRA"

// CHECK-OPTS: //Build option: "
// CHECK-ON: //Build option: "{{.*}}-fasterRA
__kernel void foo(__global int *a) { a[get_global_id(0)] += 1; }
