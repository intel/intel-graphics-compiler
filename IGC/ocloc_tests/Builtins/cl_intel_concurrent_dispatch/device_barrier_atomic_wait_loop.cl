/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Check that the wait loop in __global_barrier_atomic()
// (Source/IGC/BiFModule/Implementation/barrier.cl) polls the sync variable with an
// L1-uncached load, with no atomic read-modify-write and no L1-invalidating fence inside
// the loop, and that the barrier's other atomics are left alone.
//
// Only the emitted shape of the poll is checked. What that shape protects - arrivals not
// being starved by the poll, and L1 not being flushed on every spin - is many-workgroup
// runtime behaviour an offline compile cannot show.

// REQUIRES: regkeys

// Only PlatformType >= IGFX_NVL takes the atomic implementation (global_barrier() in
// barrier.cl); DG2 stays on __global_barrier_nonatomic() and is the negative control.
// RUN: %if cri-supported %{ ocloc compile -file %s -device cri -options "-cl-std=CL3.0 -igc_opts 'PrintToConsole=1 PrintAfter=Layout'" -internal_options "-cl-ext=-all,+cl_intel_concurrent_dispatch" 2>&1 | FileCheck %s --check-prefix=CHECK-ATOMIC %}
// RUN: %if dg2-supported %{ ocloc compile -file %s -device dg2 -options "-cl-std=CL3.0 -igc_opts 'PrintToConsole=1 PrintAfter=Layout'" -internal_options "-cl-ext=-all,+cl_intel_concurrent_dispatch" 2>&1 | FileCheck %s --check-prefix=CHECK-NONATOMIC %}
//
// Additional vISA check, as the IR checks above cannot show the poll is a loop.
// RUN: %if cri-supported %{ ocloc compile -file %s -device cri -options "-cl-std=CL3.0 -igc_opts 'DumpVISAASMToConsole=1'" -internal_options "-cl-ext=-all,+cl_intel_concurrent_dispatch" | FileCheck %s --check-prefix=CHECK-ISA %}

// CHECK-ATOMIC-LABEL: define spir_kernel void @test
//
// The arrive side stays atomic. The trailing immediate of GenISA.intatomicraw is the
// IGC::AtomicOp (Source/IGC/Compiler/CodeGenPublicEnums.h): 1 = SUB, 2 = INC, 8 = AND,
// 9 = OR. These guard against over-applying the fix to the barrier's other atomics.
// CHECK-ATOMIC:       call i32 @llvm.genx.GenISA.intatomicraw{{.*}}, i32 0, i32 9)
// CHECK-ATOMIC:       call i32 @llvm.genx.GenISA.intatomicraw{{.*}}, i32 0, i32 2)
// CHECK-ATOMIC:       call i32 @llvm.genx.GenISA.intatomicraw{{.*}}, i32 %{{[0-9]+}}, i32 1)
//
// Check that the poll uses LSCLoadWithSideEffects and compares the result with zero.
// Reject atomic RMWs and an acquire-only fence between the arrival and the poll.
// That fence has L1 invalidate=true and evict=false; the surrounding AcquireRelease
// fences also set evict=true and are not excluded.
// Load operands 3/1 encode D32V1; cache control 2 is LSC_LDCC_L1UC_L3C, needed because
// the arrivals land in L3 and a poll that reads L1 can spin on a stale line.
// The side-effecting intrinsic prevents loop-invariant hoisting; the vISA checks
// below verify that the load remains inside the polling loop.
// CHECK-ATOMIC-NOT:   GenISA.intatomicraw
// CHECK-ATOMIC-NOT:   call void @llvm.genx.GenISA.memoryfence(i1 true, i1 true, i1 false, i1 false, i1 false, i1 true, i1 true, i1 false, i32 3)
// CHECK-ATOMIC:       %[[POLL:[0-9]+]] = call i32 @llvm.genx.GenISA.LSCLoadWithSideEffects.{{[^(]*}}({{.*}}addrspace(1){{.*}}, i32 0, i32 3, i32 1, i32 2)
// CHECK-ATOMIC-NEXT:  icmp {{eq|ne}} i32 %[[POLL]], 0
//
// The offset selector flip after the wait stays atomic as well: AND 0 then OR 1.
// CHECK-ATOMIC:       call i32 @llvm.genx.GenISA.intatomicraw{{.*}}, i32 0, i32 8)
// CHECK-ATOMIC:       call i32 @llvm.genx.GenISA.intatomicraw{{.*}}, i32 1, i32 9)

// The same poll in vISA, where the loop itself can be pinned: lsc_atomic_isub is the
// arrival and anchors the scan, then the poll block must be a plain lsc_load (not
// lsc_atomic_*, and not preceded by a fence), a compare against zero, and a branch back
// to its own label. IsaDisassembly.cpp prints cache controls only when they are not the
// default, so the .uc suffix is what shows the load bypasses L1; the rest of the suffix
// differs between the 2-level and 3-level cache encodings and is left unmatched.
// CHECK-ISA-LABEL: .kernel "test"
// CHECK-ISA:       lsc_atomic_isub.ugm
// CHECK-ISA:       {{^}}[[POLL:[_a-zA-Z0-9]+]]:
// CHECK-ISA-NEXT:    lsc_load.ugm.uc
// CHECK-ISA-NEXT:    cmp.eq
// CHECK-ISA-NEXT:    goto {{.*}}[[POLL]]

// __global_barrier_nonatomic() uses no atomics at all - arrival is a flag-byte store and
// the wait a flag load - so pinning it keeps a change to the atomic implementation from
// reaching older platforms. The positive checks keep the CHECK-NOTs from passing
// vacuously on an empty kernel (extension not enabled, or the valid check folded away).
// CHECK-NONATOMIC-LABEL: define spir_kernel void @test
// CHECK-NONATOMIC-NOT:   GenISA.intatomicraw
// CHECK-NONATOMIC:       store volatile i8 1, {{.*}}addrspace(1)
// CHECK-NONATOMIC-NOT:   GenISA.intatomicraw
// CHECK-NONATOMIC:       load volatile i8, {{.*}}addrspace(1)
// CHECK-NONATOMIC-NOT:   GenISA.intatomicraw

kernel void test() {
  if (intel_is_device_barrier_valid()) {
    intel_device_barrier(CLK_GLOBAL_MEM_FENCE, memory_scope_device);
  }
}
