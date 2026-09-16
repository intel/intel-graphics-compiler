/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Keep a vISA subroutine with two reachable returns lexically last.
// Return merging creates a trailing label-only synthetic EXIT even without
// dead returns. getGenSize() must use the last encoded instruction instead
// of requiring the final CFG instruction to have a generated offset.
// Check that compilation succeeds and emits a nontrivial kernel symbol size.

// REQUIRES: cri-supported, oneapi-readelf

// RUN: ocloc compile -file %s -device cri -output %t -output_no_suffix
// RUN: oneapi-readelf %t.bin -Ws --sym-base=16 | FileCheck %s

// CHECK: Symbol table '.symtab'
// Sanity-check the symbol size without pinning an exact code-generation size.
// CHECK: {{0x0*[1-9a-fA-F][0-9a-fA-F][0-9a-fA-F]?[0-9a-fA-F]?}} FUNC{{ +}}LOCAL{{ +}}DEFAULT{{ +}}1 test_two_live{{$}}

__attribute__((noinline)) void body_two_live() {
  __asm__ volatile("{\n"
                   "call (M1, 1) foo_two_live\n"
                   "ret (M1, 1)\n"
                   ".function \"foo_two_live\"\n"
                   ".decl P1 v_type=P num_elts=1\n"
                   "foo_two_live:\n"
                   "(P1) goto (M1, 1) SECOND\n"
                   "ret (M1, 1)\n"
                   "SECOND:\n"
                   "ret (M1, 1)\n"
                   "}\n");
}

kernel void test_two_live() { body_two_live(); }
