/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, cri-supported
// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1,VectorAlias=1'" \
// RUN:   | FileCheck %s --check-prefix=CHECK-VRA1
// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1,VectorAlias=9'" \
// RUN:   | FileCheck %s --check-prefix=CHECK-VRA9

// Test extractelement aliasing (VariableReuseAnalysis) for a vector that is
// in a DeSSA congruence class (phi) and whose elements are used only by inline
// asm.
//
// `acc` is a <8 x float> vector merged by a phi (load or zero). Each element is
// passed to inline asm.
//
// VectorAlias=1: no extract aliasing, each element is copied with a mov.
// VectorAlias=9: the asm reads the elements of the vector directly.

// CHECK-VRA1:      ({{!P[0-9]+}}) goto (M1, 1) [[ZERO_BB:[A-Za-z0-9_]+]]
// CHECK-VRA1-EMPTY:
// CHECK-VRA1-NEXT: {{^}}{{[A-Za-z0-9_]+}}:
// CHECK-VRA1-NEXT: lsc_load.ugm (M1, 16) [[ACC:[A-Za-z0-9_]+]]:d32x8
// CHECK-VRA1-NEXT: goto (M1, 1) [[MERGE_BB:[A-Za-z0-9_]+]]
// CHECK-VRA1-EMPTY:
// CHECK-VRA1-NEXT: {{^}}[[ZERO_BB]]:
// CHECK-VRA1-NEXT: mov (M1, 16) [[ACC]](0,0)<1> 0x0:f
// CHECK-VRA1-NEXT: mov (M1, 16) [[ACC]](1,0)<1> 0x0:f
// CHECK-VRA1-NEXT: mov (M1, 16) [[ACC]](2,0)<1> 0x0:f
// CHECK-VRA1-NEXT: mov (M1, 16) [[ACC]](3,0)<1> 0x0:f
// CHECK-VRA1-NEXT: mov (M1, 16) [[ACC]](4,0)<1> 0x0:f
// CHECK-VRA1-NEXT: mov (M1, 16) [[ACC]](5,0)<1> 0x0:f
// CHECK-VRA1-NEXT: mov (M1, 16) [[ACC]](6,0)<1> 0x0:f
// CHECK-VRA1-NEXT: mov (M1, 16) [[ACC]](7,0)<1> 0x0:f
// CHECK-VRA1-EMPTY:
// CHECK-VRA1-NEXT: {{^}}[[MERGE_BB]]:
// CHECK-VRA1-NEXT: mov (M1, 16) [[E0:[A-Za-z0-9_]+]](0,0)<1> [[ACC]](0,0)<1;1,0>
// CHECK-VRA1-NEXT: mov (M1, 16) [[E1:[A-Za-z0-9_]+]](0,0)<1> [[ACC]](1,0)<1;1,0>
// CHECK-VRA1-NEXT: mov (M1, 16) [[E2:[A-Za-z0-9_]+]](0,0)<1> [[ACC]](2,0)<1;1,0>
// CHECK-VRA1-NEXT: mov (M1, 16) [[E3:[A-Za-z0-9_]+]](0,0)<1> [[ACC]](3,0)<1;1,0>
// CHECK-VRA1-NEXT: mov (M1, 16) [[E4:[A-Za-z0-9_]+]](0,0)<1> [[ACC]](4,0)<1;1,0>
// CHECK-VRA1-NEXT: mov (M1, 16) [[E5:[A-Za-z0-9_]+]](0,0)<1> [[ACC]](5,0)<1;1,0>
// CHECK-VRA1-NEXT: mov (M1, 16) [[E6:[A-Za-z0-9_]+]](0,0)<1> [[ACC]](6,0)<1;1,0>
// CHECK-VRA1-NEXT: mov (M1, 16) [[E7:[A-Za-z0-9_]+]](0,0)<1> [[ACC]](7,0)<1;1,0>
// CHECK-VRA1:      add (M1_NM, 16) {{V[0-9]+}}(0,0)<1> IN_UD(0,0)<1;1,0> 0x8000:uw
// CHECK-VRA1:      add (M1_NM, 16) {{V[0-9]+}}(0,0)<1> IN_UD(0,0)<1;1,0> 0x8000:uw
// CHECK-VRA1:      add (M1_NM, 16) {{V[0-9]+}}(0,0)<1> IN_UD(0,0)<1;1,0> 0x8000:uw
// CHECK-VRA1:      add (M1_NM, 16) {{V[0-9]+}}(0,0)<1> IN_UD(0,0)<1;1,0> 0x8000:uw
// CHECK-VRA1:      add (M1_NM, 16) {{V[0-9]+}}(0,0)<1> IN_UD(0,0)<1;1,0> 0x8000:uw
// CHECK-VRA1:      add (M1_NM, 16) {{V[0-9]+}}(0,0)<1> IN_UD(0,0)<1;1,0> 0x8000:uw
// CHECK-VRA1:      add (M1_NM, 16) {{V[0-9]+}}(0,0)<1> IN_UD(0,0)<1;1,0> 0x8000:uw
// CHECK-VRA1:      add (M1_NM, 16) {{V[0-9]+}}(0,0)<1> IN_UD(0,0)<1;1,0> 0x8000:uw
// CHECK-VRA1:      ret
// CHECK-VRA1:      // .decl [[ACC]] v_type=G type=f num_elts=128
// CHECK-VRA1:      // .decl IN_UD v_type=G type=ud num_elts=16 alias=<[[E0]], 0>
// CHECK-VRA1:      // .decl IN_UD v_type=G type=ud num_elts=16 alias=<[[E1]], 0>
// CHECK-VRA1:      // .decl IN_UD v_type=G type=ud num_elts=16 alias=<[[E2]], 0>
// CHECK-VRA1:      // .decl IN_UD v_type=G type=ud num_elts=16 alias=<[[E3]], 0>
// CHECK-VRA1:      // .decl IN_UD v_type=G type=ud num_elts=16 alias=<[[E4]], 0>
// CHECK-VRA1:      // .decl IN_UD v_type=G type=ud num_elts=16 alias=<[[E5]], 0>
// CHECK-VRA1:      // .decl IN_UD v_type=G type=ud num_elts=16 alias=<[[E6]], 0>
// CHECK-VRA1:      // .decl IN_UD v_type=G type=ud num_elts=16 alias=<[[E7]], 0>

// CHECK-VRA9:      ({{!P[0-9]+}}) goto (M1, 1) [[ZERO_BB:[A-Za-z0-9_]+]]
// CHECK-VRA9-EMPTY:
// CHECK-VRA9-NEXT: {{^}}{{[A-Za-z0-9_]+}}:
// CHECK-VRA9-NEXT: lsc_load.ugm (M1, 16) [[ACC:[A-Za-z0-9_]+]]:d32x8
// CHECK-VRA9-NEXT: goto (M1, 1) [[MERGE_BB:[A-Za-z0-9_]+]]
// CHECK-VRA9-EMPTY:
// CHECK-VRA9-NEXT: {{^}}[[ZERO_BB]]:
// CHECK-VRA9-NEXT: mov (M1, 16) [[ACC]](0,0)<1> 0x0:f
// CHECK-VRA9-NEXT: mov (M1, 16) [[ACC]](1,0)<1> 0x0:f
// CHECK-VRA9-NEXT: mov (M1, 16) [[ACC]](2,0)<1> 0x0:f
// CHECK-VRA9-NEXT: mov (M1, 16) [[ACC]](3,0)<1> 0x0:f
// CHECK-VRA9-NEXT: mov (M1, 16) [[ACC]](4,0)<1> 0x0:f
// CHECK-VRA9-NEXT: mov (M1, 16) [[ACC]](5,0)<1> 0x0:f
// CHECK-VRA9-NEXT: mov (M1, 16) [[ACC]](6,0)<1> 0x0:f
// CHECK-VRA9-NEXT: mov (M1, 16) [[ACC]](7,0)<1> 0x0:f
// CHECK-VRA9-EMPTY:
// CHECK-VRA9-NEXT: {{^}}[[MERGE_BB]]:
// CHECK-VRA9-NOT:  mov
// CHECK-VRA9:      add (M1_NM, 16) {{V[0-9]+}}(0,0)<1> IN_UD(0,0)<1;1,0> 0x8000:uw
// CHECK-VRA9:      add (M1_NM, 16) {{V[0-9]+}}(0,0)<1> IN_UD(0,0)<1;1,0> 0x8000:uw
// CHECK-VRA9:      add (M1_NM, 16) {{V[0-9]+}}(0,0)<1> IN_UD(0,0)<1;1,0> 0x8000:uw
// CHECK-VRA9:      add (M1_NM, 16) {{V[0-9]+}}(0,0)<1> IN_UD(0,0)<1;1,0> 0x8000:uw
// CHECK-VRA9:      add (M1_NM, 16) {{V[0-9]+}}(0,0)<1> IN_UD(0,0)<1;1,0> 0x8000:uw
// CHECK-VRA9:      add (M1_NM, 16) {{V[0-9]+}}(0,0)<1> IN_UD(0,0)<1;1,0> 0x8000:uw
// CHECK-VRA9:      add (M1_NM, 16) {{V[0-9]+}}(0,0)<1> IN_UD(0,0)<1;1,0> 0x8000:uw
// CHECK-VRA9:      add (M1_NM, 16) {{V[0-9]+}}(0,0)<1> IN_UD(0,0)<1;1,0> 0x8000:uw
// CHECK-VRA9-NOT:  mov
// CHECK-VRA9:      ret
// CHECK-VRA9:      // .decl [[ACC]] v_type=G type=f num_elts=128
// CHECK-VRA9:      // .decl IN_UD v_type=G type=ud num_elts=16 alias=<[[ACC]], 0>
// CHECK-VRA9:      // .decl [[ACC1:[A-Za-z0-9_]+]] v_type=G type=f {{.*}} alias=<[[ACC]], 64>
// CHECK-VRA9:      // .decl IN_UD v_type=G type=ud num_elts=16 alias=<[[ACC1]], 0>
// CHECK-VRA9:      // .decl [[ACC2:[A-Za-z0-9_]+]] v_type=G type=f {{.*}} alias=<[[ACC]], 128>
// CHECK-VRA9:      // .decl IN_UD v_type=G type=ud num_elts=16 alias=<[[ACC2]], 0>
// CHECK-VRA9:      // .decl [[ACC3:[A-Za-z0-9_]+]] v_type=G type=f {{.*}} alias=<[[ACC]], 192>
// CHECK-VRA9:      // .decl IN_UD v_type=G type=ud num_elts=16 alias=<[[ACC3]], 0>
// CHECK-VRA9:      // .decl [[ACC4:[A-Za-z0-9_]+]] v_type=G type=f {{.*}} alias=<[[ACC]], 256>
// CHECK-VRA9:      // .decl IN_UD v_type=G type=ud num_elts=16 alias=<[[ACC4]], 0>
// CHECK-VRA9:      // .decl [[ACC5:[A-Za-z0-9_]+]] v_type=G type=f {{.*}} alias=<[[ACC]], 320>
// CHECK-VRA9:      // .decl IN_UD v_type=G type=ud num_elts=16 alias=<[[ACC5]], 0>
// CHECK-VRA9:      // .decl [[ACC6:[A-Za-z0-9_]+]] v_type=G type=f {{.*}} alias=<[[ACC]], 384>
// CHECK-VRA9:      // .decl IN_UD v_type=G type=ud num_elts=16 alias=<[[ACC6]], 0>
// CHECK-VRA9:      // .decl [[ACC7:[A-Za-z0-9_]+]] v_type=G type=f {{.*}} alias=<[[ACC]], 448>
// CHECK-VRA9:      // .decl IN_UD v_type=G type=ud num_elts=16 alias=<[[ACC7]], 0>

__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_inline_asm_extract(global float8 *in, global uint *out, int n) {
  int id = get_global_id(0);
  float8 acc = 0.0f;
  if (n > 0)
    acc = in[id];
  float a[8] = {acc.s0, acc.s1, acc.s2, acc.s3, acc.s4, acc.s5, acc.s6, acc.s7};
  for (int j = 0; j < 8; ++j) {
    uint r;
    __asm__("{\n"
            ".decl IN_UD v_type=G type=UD num_elts=16 alias=<%1,0>\n"
            "add (M1_NM, 16) %0(0,0)<1> IN_UD(0,0)<1;1,0> 0x8000:uw\n"
            "}\n"
            : "=rw"(r)
            : "rw"(a[j]));
    out[id * 8 + j] = r;
  }
}
