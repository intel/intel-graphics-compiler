;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers %s -S -o - -igc-promote-constant-structs | FileCheck %s

; Verify that constant struct field was not promoted as pointer to struct can escape

%struct.Ctx = type { i32, i16 }

define spir_kernel void @escapes_via_phi(ptr %slot, i1 %c, ptr %out) {
entry:
  %ctx = alloca %struct.Ctx, align 8
  %f = getelementptr inbounds %struct.Ctx, ptr %ctx, i64 0, i32 1
  store i16 0, ptr %f, align 8
  br i1 %c, label %a, label %b
a:
  br label %join
b:
  br label %join
join:
  %phi = phi ptr [ %ctx, %a ], [ %ctx, %b ]
  %asc = addrspacecast ptr %phi to ptr addrspace(4)
  store ptr addrspace(4) %asc, ptr %slot, align 8   ; potential pointer escape
  call void @opaque()                               ; potential write through the escaped pointer
  %ld = load i16, ptr %f, align 8
  store i16 %ld, ptr %out, align 2
  ret void
}

declare void @opaque()

; CHECK-LABEL: define spir_kernel void @escapes_via_phi
; CHECK: %ld = load i16, ptr %f, align 8
; CHECK: store i16 %ld, ptr %out, align 2
