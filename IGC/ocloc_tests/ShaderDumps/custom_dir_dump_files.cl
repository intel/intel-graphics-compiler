/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// UNSUPPORTED: system-windows
// REQUIRES: regkeys, bmg-supported

// Test that shader dumps work when using environment variables.

// RUN: rm -rf %t.env_dump_dir
// RUN: IGC_ShaderDumpEnable=1 IGC_DumpToCustomDir=%t.env_dump_dir ocloc compile -device bmg -file %s
// RUN: ls %t.env_dump_dir/*.cl
// RUN: ls %t.env_dump_dir/*beforeUnification*.ll
// RUN: rm -rf %t.env_dump_dir

// Test that shader dumps work when using -igc_opts.

// RUN: rm -rf %t.opts_dump_dir
// RUN: ocloc compile -device bmg -file %s -options "-igc_opts 'ShaderDumpEnable=1, DumpToCustomDir=%t.opts_dump_dir'"
// RUN: ls %t.opts_dump_dir/*.cl
// RUN: ls %t.opts_dump_dir/*beforeUnification*.ll
// RUN: rm -rf %t.opts_dump_dir

__kernel void foo(__global int *Out, int Val) {
  int idx = get_global_id(0);
  Out[idx] = Val + 1;
}
