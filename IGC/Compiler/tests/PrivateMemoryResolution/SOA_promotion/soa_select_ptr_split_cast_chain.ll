;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; SplitSelectsOfAllocaPointers replays the cast chain that sits between the
; select and each load/store on both operands. Here the chain is an addrspacecast
; (private -> generic), and both operands are alloca-derived, so the split leaves
; two SoA-promoted accesses per original access.
;
; The other two functions cover the guards that keep a select untouched: a user
; that is neither a load/store nor a cast, and a select no load or store reaches.
;
; RUN: igc_opt --opaque-pointers --ocl --platformPtl \
; RUN:   --igc-split-selects-of-alloca-pointers --igc-private-mem-resolution \
; RUN:   --regkey EnableSelectOfAllocaPtrSplit=1,EnablePrivMemNewSOAForScalarArrays=1 -S %s | FileCheck %s
;
; With the split turned off nothing is duplicated, so the pointer select blocks SoA
; promotion and both allocas keep the array-of-floats (132-byte per-lane) layout.
;
; RUN: igc_opt --opaque-pointers --ocl --platformPtl \
; RUN:   --igc-split-selects-of-alloca-pointers --igc-private-mem-resolution \
; RUN:   --regkey EnableSelectOfAllocaPtrSplit=0,EnablePrivMemNewSOAForScalarArrays=1 -S %s \
; RUN:   | FileCheck %s --check-prefix=DISABLED
;
; DISABLED-LABEL: @test_select_ascast_chain(
; DISABLED:      mul i32 %{{.*}}, 132
; DISABLED:      [[SEL:%.*]] = select i1 %c, ptr %ga, ptr %gb
; DISABLED:      [[SEL4:%.*]] = addrspacecast ptr [[SEL]] to ptr addrspace(4)
; DISABLED:      load float, ptr addrspace(4) [[SEL4]]
; DISABLED:      store float 1.000000e+00, ptr addrspace(4) [[SEL4]]

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; The load through the select is replaced by one load per operand plus a value
; select, and the store by an if-then-else with one store per operand. Both
; operands are SoA scratch accesses (chunk index from lshr, address scaled by simdSize), and the
; generic-AS pointer the addrspacecast produced is gone.
;
; CHECK-LABEL: @test_select_ascast_chain(
; CHECK:       [[SIMDSIZE:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK:       [[LA:%.*]] = load float, ptr %{{.*}}, align 4
; CHECK:       [[LB:%.*]] = load float, ptr %{{.*}}, align 4
; CHECK:       [[V:%.*]] = select i1 %c, float [[LA]], float [[LB]]
; CHECK:       br i1 %c
; CHECK:       store float 1.000000e+00, ptr %{{.*}}, align 4
; CHECK:       store float 1.000000e+00, ptr %{{.*}}, align 4
; CHECK:       store float [[V]], ptr addrspace(1)
; CHECK-NOT:   select i1 %c, ptr
; CHECK-NOT:   addrspace(4)
define spir_kernel void @test_select_ascast_chain(ptr addrspace(1) nocapture writeonly %d, i1 %c, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) #0 {
entry:
  %a = alloca [33 x float], align 4
  %b = alloca [33 x float], align 4
  %idx = zext i32 %ix to i64
  %ga = getelementptr inbounds [33 x float], ptr %a, i64 0, i64 %idx
  %gb = getelementptr inbounds [33 x float], ptr %b, i64 0, i64 %idx
  %sel = select i1 %c, ptr %ga, ptr %gb
  %sel4 = addrspacecast ptr %sel to ptr addrspace(4)
  %v = load float, ptr addrspace(4) %sel4, align 4
  store float 1.000000e+00, ptr addrspace(4) %sel4, align 4
  %arrayidx = getelementptr inbounds float, ptr addrspace(1) %d, i64 %idx
  store float %v, ptr addrspace(1) %arrayidx, align 4
  ret void
}

; A ptrtoint of the merged pointer is not a load, a store or a cast the split can
; replay, so the select is left alone (and the alloca is not SoA-promoted, since
; SOALayoutChecker rejects the select too).
;
; CHECK-LABEL: @test_select_unsupported_user(
; CHECK:       select i1 %c, ptr
; CHECK:       ptrtoint
define spir_kernel void @test_select_unsupported_user(ptr addrspace(1) nocapture writeonly %d, i1 %c, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) #0 {
entry:
  %a = alloca [33 x float], align 4
  %idx = zext i32 %ix to i64
  %ga = getelementptr inbounds [33 x float], ptr %a, i64 0, i64 %idx
  %sel = select i1 %c, ptr %ga, ptr null
  %sel4 = addrspacecast ptr %sel to ptr addrspace(4)
  %pi = ptrtoint ptr addrspace(4) %sel4 to i64
  %pt = trunc i64 %pi to i32
  %pf = sitofp i32 %pt to float
  %arrayidx = getelementptr inbounds float, ptr addrspace(1) %d, i64 %idx
  store float %pf, ptr addrspace(1) %arrayidx, align 4
  ret void
}

; The cast chain hanging off the select ends without a load or a store, so there
; is nothing to split and the select survives.
;
; CHECK-LABEL: @test_select_no_load_or_store(
; CHECK:       select i1 %c, ptr
; CHECK:       addrspacecast
define spir_kernel void @test_select_no_load_or_store(ptr addrspace(1) nocapture writeonly %d, i1 %c, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) #0 {
entry:
  %a = alloca [33 x float], align 4
  %idx = zext i32 %ix to i64
  %ga = getelementptr inbounds [33 x float], ptr %a, i64 0, i64 %idx
  %sel = select i1 %c, ptr %ga, ptr null
  %sel4 = addrspacecast ptr %sel to ptr addrspace(4)
  %arrayidx = getelementptr inbounds float, ptr addrspace(1) %d, i64 %idx
  store float 2.000000e+00, ptr addrspace(1) %arrayidx, align 4
  ret void
}

; A volatile load must not be duplicated onto both operands, so the select is left
; alone and both allocas keep their array-of-floats (132-byte per-lane) layout.
;
; CHECK-LABEL: @test_select_volatile_load(
; CHECK:       mul i32 %{{.*}}, 132
; CHECK:       select i1 %c, ptr
; CHECK:       load volatile float
define spir_kernel void @test_select_volatile_load(ptr addrspace(1) nocapture writeonly %d, i1 %c, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) #0 {
entry:
  %a = alloca [33 x float], align 4
  %b = alloca [33 x float], align 4
  %idx = zext i32 %ix to i64
  %ga = getelementptr inbounds [33 x float], ptr %a, i64 0, i64 %idx
  %gb = getelementptr inbounds [33 x float], ptr %b, i64 0, i64 %idx
  %sel = select i1 %c, ptr %ga, ptr %gb
  %sel4 = addrspacecast ptr %sel to ptr addrspace(4)
  %v = load volatile float, ptr addrspace(4) %sel4, align 4
  %arrayidx = getelementptr inbounds float, ptr addrspace(1) %d, i64 %idx
  store float %v, ptr addrspace(1) %arrayidx, align 4
  ret void
}

; The merged pointer is also the value being stored, not just the address. Cloning
; the store would leave the select live in the clones' value operand, so the split
; must decline and the select survives.
;
; CHECK-LABEL: @test_select_stored_as_value(
; CHECK:       select i1 %c, ptr
; CHECK:       store ptr
define spir_kernel void @test_select_stored_as_value(ptr addrspace(1) nocapture writeonly %d, i1 %c, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) #0 {
entry:
  %a = alloca [33 x float], align 4
  %slot = alloca ptr, align 8
  %idx = zext i32 %ix to i64
  %ga = getelementptr inbounds [33 x float], ptr %a, i64 0, i64 %idx
  %sel = select i1 %c, ptr %ga, ptr %slot
  store ptr %sel, ptr %sel, align 8
  ret void
}

attributes #0 = { convergent nounwind }

!IGCMetadata = !{!0}
!igc.functions = !{!6, !7, !8, !9, !10}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", ptr @test_select_ascast_chain}
!5 = !{!"FuncMDValue[0]", !2}
!6 = !{ptr @test_select_ascast_chain, !408}
!7 = !{ptr @test_select_unsupported_user, !408}
!8 = !{ptr @test_select_no_load_or_store, !408}
!9 = !{ptr @test_select_volatile_load, !408}
!10 = !{ptr @test_select_stored_as_value, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!417 = !{i32 13}
