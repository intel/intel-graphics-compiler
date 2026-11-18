;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-addrspacecast-fix --regkey AddressSpacePhiPropagation=1 -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll
;
; Checks placement and legality of loads lowered from a PHI of generic pointers into the PHI predecessors.
;

; CHECK-LABEL: define float @test_private_cast_before_store(
; CHECK:       entry:
; CHECK-NEXT:    %priv = alloca float, align 4
; CHECK-NEXT:    store float %identity, ptr %priv, align 4
; CHECK-NEXT:    br i1 %cond, label %slm.bb, label %priv.bb
; CHECK:       slm.bb:
; CHECK-NEXT:    %gep = getelementptr inbounds float, ptr addrspace(3) %slm, i64 1
; CHECK-NEXT:    [[SLM_LD:%.*]] = load float, ptr addrspace(3) %gep, align 4
; CHECK-NEXT:    br label %merge
; CHECK:       priv.bb:
; CHECK-NEXT:    [[PRIV_LD:%.*]] = load float, ptr %priv, align 4
; CHECK-NEXT:    br label %merge
; CHECK:       merge:
; CHECK-NEXT:    [[RES:%.*]] = phi float [ [[SLM_LD]], %slm.bb ], [ [[PRIV_LD]], %priv.bb ]
; CHECK-NEXT:    ret float [[RES]]
define float @test_private_cast_before_store(ptr addrspace(3) %slm, float %identity, i1 %cond) {
entry:
  %priv = alloca float, align 4
  %priv.cast = addrspacecast ptr %priv to ptr addrspace(4)
  store float %identity, ptr %priv, align 4
  br i1 %cond, label %slm.bb, label %priv.bb

slm.bb:
  %gep = getelementptr inbounds float, ptr addrspace(3) %slm, i64 1
  %slm.cast = addrspacecast ptr addrspace(3) %gep to ptr addrspace(4)
  br label %merge

priv.bb:
  br label %merge

merge:
  %ptr = phi ptr addrspace(4) [ %slm.cast, %slm.bb ], [ %priv.cast, %priv.bb ]
  %val = load float, ptr addrspace(4) %ptr, align 4
  ret float %val
}

; Check that instructions that do not write memory between the PHI and the load do not
; block the lowering.

; CHECK-LABEL: define float @test_readonly_between_phi_and_load(
; CHECK:       slm.bb:
; CHECK-NEXT:    [[SLM_LD:%.*]] = load float, ptr addrspace(3) %slm, align 4
; CHECK-NEXT:    br label %merge
; CHECK:       priv.bb:
; CHECK-NEXT:    [[PRIV_LD:%.*]] = load float, ptr %priv, align 4
; CHECK-NEXT:    br label %merge
; CHECK:       merge:
; CHECK-NEXT:    [[RES:%.*]] = phi float [ [[SLM_LD]], %slm.bb ], [ [[PRIV_LD]], %priv.bb ]
; CHECK-NEXT:    %other = load float, ptr addrspace(1) %global, align 4
; CHECK-NOT:     addrspace(4)
define float @test_readonly_between_phi_and_load(ptr addrspace(3) %slm, ptr %priv, ptr addrspace(1) %global, i1 %cond) {
entry:
  br i1 %cond, label %slm.bb, label %priv.bb

slm.bb:
  %slm.cast = addrspacecast ptr addrspace(3) %slm to ptr addrspace(4)
  br label %merge

priv.bb:
  %priv.cast = addrspacecast ptr %priv to ptr addrspace(4)
  br label %merge

merge:
  %ptr = phi ptr addrspace(4) [ %slm.cast, %slm.bb ], [ %priv.cast, %priv.bb ]
  %other = load float, ptr addrspace(1) %global, align 4
  %val = load float, ptr addrspace(4) %ptr, align 4
  %res = fadd float %val, %other
  ret float %res
}

; Check that a store between the PHI and the load prevents lowering.

; CHECK-LABEL: define float @test_store_between_phi_and_load(
; CHECK:       merge:
; CHECK-NEXT:    %ptr = phi ptr addrspace(4) [ %slm.cast, %slm.bb ], [ %priv.cast, %priv.bb ]
; CHECK-NEXT:    store float %x, ptr addrspace(3) %slm, align 4
; CHECK-NEXT:    %val = load float, ptr addrspace(4) %ptr, align 4
define float @test_store_between_phi_and_load(ptr addrspace(3) %slm, ptr %priv, float %x, i1 %cond) {
entry:
  br i1 %cond, label %slm.bb, label %priv.bb

slm.bb:
  %slm.cast = addrspacecast ptr addrspace(3) %slm to ptr addrspace(4)
  br label %merge

priv.bb:
  %priv.cast = addrspacecast ptr %priv to ptr addrspace(4)
  br label %merge

merge:
  %ptr = phi ptr addrspace(4) [ %slm.cast, %slm.bb ], [ %priv.cast, %priv.bb ]
  store float %x, ptr addrspace(3) %slm, align 4
  %val = load float, ptr addrspace(4) %ptr, align 4
  ret float %val
}

; Check that incoming blocks with multiple successors block PHI lowering.
; This prevents loads from executing on path not reaching PHI.

; CHECK-LABEL: define float @test_incoming_block_multiple_successors(
; CHECK:       split:
; CHECK-NEXT:    br i1 %cond, label %slm.bb, label %merge
; CHECK:       merge:
; CHECK-NEXT:    %ptr = phi ptr addrspace(4) [ %slm.cast, %slm.bb ], [ %global.cast, %split ]
; CHECK-NEXT:    %val = load float, ptr addrspace(4) %ptr, align 4
define float @test_incoming_block_multiple_successors(ptr addrspace(3) %slm, ptr addrspace(1) %global, i1 %cond) {
entry:
  %global.cast = addrspacecast ptr addrspace(1) %global to ptr addrspace(4)
  br label %split

split:
  br i1 %cond, label %slm.bb, label %merge

slm.bb:
  %gep = getelementptr inbounds float, ptr addrspace(3) %slm, i64 1
  %slm.cast = addrspacecast ptr addrspace(3) %gep to ptr addrspace(4)
  br label %merge

merge:
  %ptr = phi ptr addrspace(4) [ %slm.cast, %slm.bb ], [ %global.cast, %split ]
  %val = load float, ptr addrspace(4) %ptr, align 4
  ret float %val
}

; Check that switches with multiple edges feeding the PHI block PHI lowering.

; CHECK-LABEL: define float @test_duplicate_incoming_block(
; CHECK:       merge:
; CHECK-NEXT:    %ptr = phi ptr addrspace(4) [ %slm.cast, %switch.bb ], [ %slm.cast, %switch.bb ], [ %priv.cast, %default.bb ]
; CHECK-NEXT:    %val = load float, ptr addrspace(4) %ptr, align 4
define float @test_duplicate_incoming_block(ptr addrspace(3) %slm, ptr %priv, i32 %sel) {
entry:
  %priv.cast = addrspacecast ptr %priv to ptr addrspace(4)
  %slm.cast = addrspacecast ptr addrspace(3) %slm to ptr addrspace(4)
  br label %switch.bb

switch.bb:
  switch i32 %sel, label %default.bb [
    i32 1, label %merge
    i32 2, label %merge
  ]

default.bb:
  br label %merge

merge:
  %ptr = phi ptr addrspace(4) [ %slm.cast, %switch.bb ], [ %slm.cast, %switch.bb ], [ %priv.cast, %default.bb ]
  %val = load float, ptr addrspace(4) %ptr, align 4
  ret float %val
}

attributes #0 = { convergent nounwind }
