;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-addrspacecast-fix -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll
; ------------------------------------------------
; FixAddrSpaceCast
; ------------------------------------------------
; When a load from a generic PHI is lowered into the incoming blocks, every new
; load has to observe the same memory state as the original load did. That means
; it must be placed at the end of the PHI's incoming block, not next to the
; definition of the incoming addrspacecast/GEP. The definition may live in a
; block that dominates the incoming block (for instance outside of the loop the
; PHI belongs to), or it may be followed by stores to the very location being
; loaded, in which case anchoring the new load at the definition silently hoists
; it above those stores.
; ------------------------------------------------

@slm = internal addrspace(3) global [64 x i64] zeroinitializer, align 8

; A three-edge generic PHI: two Workgroup(3) edges and one Function(0) edge.
; The Function-side addrspacecast is defined in the loop preheader, while the
; accumulator it points at is written on every iteration in %latch. Reloading it
; anywhere but in %fallback reads a stale (here: uninitialized) value.

; CHECK-LABEL: @test_phi_edge_placement(
; CHECK:       entry:
; CHECK-NOT:     load
; CHECK:         br label %loop

; CHECK-LABEL: loop:
; CHECK:         [[SLM1:%.*]] = load i64, ptr addrspace(3) %slm.gep
; CHECK-NEXT:    br i1 %c0, label %latch, label %check

; CHECK-LABEL: check:
; CHECK:         [[SLM2:%.*]] = load i64, ptr addrspace(3) %slm.gep
; CHECK-NEXT:    br i1 %c1, label %latch, label %fallback

; CHECK-LABEL: fallback:
; CHECK-NEXT:    [[PRIV:%.*]] = load i64, ptr %acc
; CHECK-NEXT:    br label %latch

; CHECK-LABEL: latch:
; CHECK-NEXT:    [[MERGE:%.*]] = phi i64 [ [[SLM1]], %loop ], [ [[SLM2]], %check ], [ [[PRIV]], %fallback ]
; CHECK-NEXT:    store i64 [[MERGE]], ptr %acc

define spir_kernel void @test_phi_edge_placement(ptr addrspace(1) %out, i64 %n) #0 {
entry:
  %acc = alloca i64, align 8
  %acc.gas = addrspacecast ptr %acc to ptr addrspace(4)
  store i64 0, ptr %acc, align 8
  br label %loop

loop:                                             ; preds = %latch, %entry
  %iv = phi i64 [ 0, %entry ], [ %iv.next, %latch ]
  %slm.gep = getelementptr inbounds i64, ptr addrspace(3) @slm, i64 %iv
  %slm.gas = addrspacecast ptr addrspace(3) %slm.gep to ptr addrspace(4)
  %c0 = icmp eq i64 %iv, 3
  br i1 %c0, label %latch, label %check

check:                                            ; preds = %loop
  %c1 = icmp eq i64 %iv, 5
  br i1 %c1, label %latch, label %fallback

fallback:                                         ; preds = %check
  br label %latch

latch:                                            ; preds = %fallback, %check, %loop
  %sel = phi ptr addrspace(4) [ %slm.gas, %loop ], [ %slm.gas, %check ], [ %acc.gas, %fallback ]
  %ld = load i64, ptr addrspace(4) %sel, align 8
  store i64 %ld, ptr %acc, align 8
  %iv.next = add i64 %iv, 1
  %more = icmp ult i64 %iv.next, %n
  br i1 %more, label %loop, label %exit

exit:                                             ; preds = %latch
  %res = load i64, ptr %acc, align 8
  store i64 %res, ptr addrspace(1) %out, align 8
  ret void
}

; The lowered load must not be hoisted above a store that the original load was
; ordered after.

; CHECK-LABEL: @test_no_hoist_above_store(
; CHECK-LABEL: true.bb:
; CHECK:         [[GLB:%.*]] = load i32, ptr addrspace(4) %gep.res
; CHECK-NEXT:    br label %merge

; CHECK-LABEL: false.bb:
; CHECK:         store i32 0, ptr addrspace(4) %addr.res
; CHECK-NEXT:    [[LOC:%.*]] = load i32, ptr addrspace(3) %p
; CHECK-NEXT:    br label %merge

; CHECK-LABEL: merge:
; CHECK-NEXT:    [[SEL:%.*]] = phi i32 [ [[GLB]], %true.bb ], [ [[LOC]], %false.bb ]
; CHECK-NEXT:    ret i32 [[SEL]]

define i32 @test_no_hoist_above_store(ptr addrspace(3) %p, ptr addrspace(4) %g, i1 %c) #0 {
entry:
  br i1 %c, label %true.bb, label %false.bb

true.bb:                                          ; preds = %entry
  %gep.res = getelementptr inbounds i8, ptr addrspace(4) %g, i64 16
  br label %merge

false.bb:                                         ; preds = %entry
  %addr.res = addrspacecast ptr addrspace(3) %p to ptr addrspace(4)
  store i32 0, ptr addrspace(4) %addr.res, align 4
  br label %merge

merge:                                            ; preds = %false.bb, %true.bb
  %sel = phi ptr addrspace(4) [ %gep.res, %true.bb ], [ %addr.res, %false.bb ]
  %v = load i32, ptr addrspace(4) %sel, align 4
  ret i32 %v
}

attributes #0 = { noinline nounwind }
