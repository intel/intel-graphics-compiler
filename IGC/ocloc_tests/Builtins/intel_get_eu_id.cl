/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The test checks if a call to intel_get_eu_id returns correct sequence of instructions for PVC

// UNSUPPORTED: sys32
// REQUIRES: regkeys, pvc-supported
// RUN: ocloc compile -file %s -options " -igc_opts 'DumpVISAASMToConsole=1'" -device pvc | FileCheck %s --check-prefix=CHECK-VISA

// CHECK-VISA: and (M1_NM, 1) [[VAR0:V[0-9]+]](0,0)<1> %sr0(0,0)<0;1,0> 0x1f0:ud
// CHECK-VISA: shr (M1_NM, 1) [[VAR1:V[0-9]+]](0,0)<1> [[VAR0]](0,0)<0;1,0> 0x4:ud
// CHECK-VISA: and (M1_NM, 1) [[VAR2:V[0-9]+]](0,0)<1> [[VAR1]](0,0)<0;1,0> 0x3:d
// CHECK-VISA: asr (M1_NM, 1) [[VAR3:V[0-9]+]](0,0)<1> [[VAR1]](0,0)<0;1,0> 0x2:d
// CHECK-VISA: bfn.xf8 (M1_NM, 1) [[EUID:V[0-9]+]](0,0)<1> [[VAR3]](0,0)<0;1,0> 0xfffffffc:d [[VAR2]](0,0)<0;1,0>
// CHECK-VISA: mov (M1, 32) Broadcast(0,0)<1> [[EUID]](0,0)<0;1,0>
// CHECK-VISA: lsc_store.ugm (M1, 32)  flat[{{.*}}]:a64  Broadcast:d32

/* Returns GPU EU ID for current subslice */
uint __attribute__((overloadable)) intel_get_eu_id(void);

kernel void test_simple(global uint* out) {
    out[get_global_id(0)] = intel_get_eu_id();
}
