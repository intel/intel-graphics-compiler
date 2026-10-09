// REQUIRES: regkeys, dg2-supported

// RUN: ocloc compile -file %s -device dg2 -options "-igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s

void __attribute__((overloadable)) intel_eu_thread_pause(uint value);

__kernel void test(uint pause_value) { intel_eu_thread_pause(pause_value); }

__kernel void test_const() {
  intel_eu_thread_pause(2u);
  intel_eu_thread_pause(31u);
  intel_eu_thread_pause(32u);
  intel_eu_thread_pause(8u);
}

// CHECK-LABEL: .kernel "test"
// CHECK: min (M1_NM, 1) [[DURATION:[A-Za-z0-9_]+]](0,0)<1> {{.*}} 0x1f:ud
// CHECK: max (M1_NM, 1) [[DURATION]](0,0)<1> [[DURATION]](0,0)<0;1,0> 0x4:ud
// CHECK: shl (M1_NM, 1) %tsc(0,4)<1> [[DURATION]](0,0)<0;1,0> 0x5:ud

// CHECK-LABEL: .kernel "test_const"
// CHECK: mov (M1_NM, 1) %tsc(0,4)<1> 0x80:ud
// CHECK: mov (M1_NM, 1) %tsc(0,4)<1> 0x3e0:ud
// CHECK: mov (M1_NM, 1) %tsc(0,4)<1> 0x3e0:ud
// CHECK: mov (M1_NM, 1) %tsc(0,4)<1> 0x100:ud
