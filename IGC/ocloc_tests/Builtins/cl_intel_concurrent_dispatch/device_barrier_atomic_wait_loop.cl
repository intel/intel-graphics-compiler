/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Ensure the wait loop in __global_barrier_atomic()
// (Source/IGC/BiFModule/Implementation/barrier.cl) polls the sync variable with a plain
// volatile load behind an acquire fence, never with an atomic read-modify-write. An RMW
// poll contends for the same address the arriving workgroups need for their atomic_inc,
// and past a few hundred waiters the arrivals stop getting through and the barrier never
// completes. Reproduced on CRI from 256 workgroups upwards.
//
// Only the emitted shape of the poll is pinned here. The livelock itself is a
// many-workgroup runtime property that an offline compile cannot show.

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
// The poll: an acquire fence, then a plain volatile load tested against zero, with
// nothing atomic in between. GenISA.memoryfence operands are (commit, L3 flush
// RW/constant/texture/instructions, global, L1 invalidate, L1 evict, scope) - invalidate
// set with evict clear is the acquire-only fence that lets the plain load see the other
// workgroups' writes, and tells it apart from the AcquireRelease fences the workgroup
// barriers emit around this sequence. A poll that regressed to an atomic RMW would have
// neither this fence nor a volatile load of the sync variable.
// CHECK-ATOMIC-NOT:   GenISA.intatomicraw
// CHECK-ATOMIC:       call void @llvm.genx.GenISA.memoryfence(i1 true, i1 true, i1 false, i1 false, i1 false, i1 true, i1 true, i1 false, i32 3)
// CHECK-ATOMIC-NEXT:  %[[POLL:[0-9]+]] = load volatile i32, {{.*}}addrspace(1)
// CHECK-ATOMIC-NEXT:  icmp eq i32 %[[POLL]], 0
//
// The offset selector flip after the wait stays atomic as well: AND 0 then OR 1.
// CHECK-ATOMIC:       call i32 @llvm.genx.GenISA.intatomicraw{{.*}}, i32 0, i32 8)
// CHECK-ATOMIC:       call i32 @llvm.genx.GenISA.intatomicraw{{.*}}, i32 1, i32 9)

// The same poll in vISA, where the loop itself can be pinned: lsc_atomic_isub is the
// arrival and anchors the scan, then the poll block must be a fence, a plain lsc_load
// (not lsc_atomic_*), a compare against zero, and a branch back to its own label.
// lsc_fence.ugm.invalidate is the acquire-only fence; the barrier fences are .evict.
// CHECK-ISA-LABEL: .kernel "test"
// CHECK-ISA:       lsc_atomic_isub.ugm
// CHECK-ISA:       {{^}}[[POLL:[_a-zA-Z0-9]+]]:
// CHECK-ISA-NEXT:    lsc_fence.ugm.invalidate
// CHECK-ISA-NEXT:    lsc_load.ugm
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
