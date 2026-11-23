;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; RUN: split-file %s %t
; RUN: igc_opt --opaque-pointers %t/owned.ll -S -o - -igc-clone-address-arithmetic --regkey=RematRPELimit=0 --regkey=RematRetryRollback=0 --dce | FileCheck %s --check-prefix=OWNED
; RUN: igc_opt --opaque-pointers %t/shared-one.ll -S -o - -igc-clone-address-arithmetic --regkey=RematRPELimit=0 --regkey=RematRetryRollback=0 --dce | FileCheck %s --check-prefix=SHARED-ONE
; RUN: igc_opt --opaque-pointers %t/shared-two.ll -S -o - -igc-clone-address-arithmetic --regkey=RematRPELimit=0 --regkey=RematRetryRollback=0 --dce | FileCheck %s --check-prefix=SHARED-TWO
; RUN: igc_opt --opaque-pointers %t/shared-two.ll -S -o - -igc-clone-address-arithmetic --regkey=RematRPELimit=0 --regkey=RematRetryRollback=0 --regkey=RematSingleFlowOwnedChains=0 --dce | FileCheck %s --check-prefix=SHARED-TWO-OFF
; RUN: igc_opt --opaque-pointers %t/diamond.ll -S -o - -igc-clone-address-arithmetic --regkey=RematRPELimit=0 --regkey=RematRetryRollback=0 --dce | FileCheck %s --check-prefix=DIAMOND
; RUN: igc_opt --opaque-pointers %t/twice.ll -S -o - -igc-clone-address-arithmetic --regkey=RematRPELimit=0 --regkey=RematRetryRollback=0 --dce | FileCheck %s --check-prefix=TWICE

; The single flow stage clones a chain operand with other users only if it keeps at most one more value live.

; OWNED: store float %w
; OWNED: [[A:%.*]] = fadd float %x, 1.000000e+00, !remat
; OWNED: [[S:%.*]] = fadd float [[A]], 2.000000e+00, !remat
; OWNED: store float [[S]]

; SHARED-ONE: %a = fadd float %x, 1.000000e+00
; SHARED-ONE: store float %w
; SHARED-ONE: [[A:%.*]] = fadd float %x, 1.000000e+00, !remat
; SHARED-ONE: [[S:%.*]] = fadd float [[A]], 2.000000e+00, !remat
; SHARED-ONE: store float [[S]]

; Cloning %a would keep %x and %v live next to %a.
; SHARED-TWO: %a = fadd float %x, %v
; SHARED-TWO-NOT: fadd float %x, %v, !remat
; SHARED-TWO-OFF: fadd float %x, %v, !remat

; %a has several users, but all of them are in the chain.
; DIAMOND: [[A:%.*]] = fmul float %x, %y, !remat
; DIAMOND: fadd float [[A]], 1.000000e+00, !remat
; TWICE: [[A:%.*]] = fmul float %x, %y, !remat
; TWICE: fadd float [[A]], [[A]], !remat

;--- owned.ll
define spir_kernel void @owned(ptr addrspace(1) %p, ptr addrspace(1) %q, float %w) {
  %x = load float, ptr addrspace(1) %q, align 4
  %a = fadd float %x, 1.0
  %s = fadd float %a, 2.0
  store float %w, ptr addrspace(1) %q, align 4
  store float %s, ptr addrspace(1) %p, align 4
  ret void
}

;--- shared-one.ll
define spir_kernel void @shared_one(ptr addrspace(1) %p, ptr addrspace(1) %q, float %w) {
  %x = load float, ptr addrspace(1) %q, align 4
  %a = fadd float %x, 1.0
  %s = fadd float %a, 2.0
  store float %w, ptr addrspace(1) %q, align 4
  store float %s, ptr addrspace(1) %p, align 4
  call void @use(float %a)
  ret void
}

declare void @use(float)

;--- shared-two.ll
define spir_kernel void @shared_two(ptr addrspace(1) %p, ptr addrspace(1) %q, float %y, <8 x i32> %r0, i1 %c) {
entry:
  %tid = extractelement <8 x i32> %r0, i64 1
  %x0 = sitofp i32 %tid to float
  br i1 %c, label %t, label %j
t:
  %x1 = fmul float %x0, %y
  br label %j
j:
  %x = phi float [ %x0, %entry ], [ %x1, %t ]
  %v = phi float [ %y, %entry ], [ %x0, %t ]
  %a = fadd float %x, %v
  %l = load float, ptr addrspace(1) %q, align 4
  %s = fadd float %a, %l
  store float %y, ptr addrspace(1) %q, align 4
  store float %s, ptr addrspace(1) %p, align 4
  call void @use(float %a)
  ret void
}

declare void @use(float)

;--- diamond.ll
define spir_kernel void @diamond(ptr addrspace(1) %p, ptr addrspace(1) %q, float %w, float %y0, i1 %c0) {
entry:
  %x0 = load float, ptr addrspace(1) %q, align 4
  br i1 %c0, label %t, label %j
t:
  br label %j
j:
  %x = phi float [ %x0, %entry ], [ %y0, %t ]
  %y = phi float [ %y0, %entry ], [ %x0, %t ]
  %a = fmul float %x, %y
  %b = fadd float %a, 1.0
  %c = fmul float %a, 3.0
  %s = fadd float %b, %c
  store float %w, ptr addrspace(1) %q, align 4
  store float %s, ptr addrspace(1) %p, align 4
  ret void
}

;--- twice.ll
define spir_kernel void @twice(ptr addrspace(1) %p, ptr addrspace(1) %q, float %w, float %y0, i1 %c0) {
entry:
  %x0 = load float, ptr addrspace(1) %q, align 4
  br i1 %c0, label %t, label %j
t:
  br label %j
j:
  %x = phi float [ %x0, %entry ], [ %y0, %t ]
  %y = phi float [ %y0, %entry ], [ %x0, %t ]
  %a = fmul float %x, %y
  %s = fadd float %a, %a
  store float %w, ptr addrspace(1) %q, align 4
  store float %s, ptr addrspace(1) %p, align 4
  ret void
}
