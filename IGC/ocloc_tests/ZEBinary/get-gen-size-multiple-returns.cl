/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// A subroutine with one live and one unreachable return produces a synthetic
// label-only merged EXIT that can remain after unreachable-block removal.
// getGenSize() must skip trailing unencoded instructions rather than assume
// that the last CFG instruction has a valid generated offset.
// Check that compilation succeeds and emits a nontrivial kernel symbol size.

// REQUIRES: cri-supported, oneapi-readelf

// RUN: ocloc compile -file %s -device cri -output %t -output_no_suffix
// RUN: oneapi-readelf %t.bin -Ws --sym-base=16 | FileCheck %s

// CHECK: Symbol table '.symtab'
// Sanity-check the symbol size without pinning an exact code-generation size.
// CHECK: {{0x0*[1-9a-fA-F][0-9a-fA-F][0-9a-fA-F]?[0-9a-fA-F]?}} FUNC{{ +}}LOCAL{{ +}}DEFAULT{{ +}}1 test{{$}}

__attribute__((noinline)) void body() {
  __asm__ volatile("{\n"
                   "call (M1, 1) foo\n"
                   "call (M1, 1) __inline_asm_end\n"
                   "ret (M1, 1)\n"
                   ".function \"foo\"\n"
                   "foo:\n"
                   "ret (M1, 1)\n"
                   "ret (M1, 1)\n"
                   "}\n"
                   ".function \"__inline_asm_end\"\n"
                   "__inline_asm_end:\n");
}

kernel void test() { body(); }
