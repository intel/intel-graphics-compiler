;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers %s -S -o - --igc-lower-byval-attribute | FileCheck %s

; ------------------------------------------------
; LowerByValAttribute
; ------------------------------------------------
;
; The hidden copy that this pass materialises for a `byval` argument must honour
; the alignment stated by the attribute, not just the ABI alignment of the
; pointee type. `%struct.double8` is a struct of doubles, so its ABI alignment is
; only 8, but the argument is declared `align 64` and the callee (and every pass
; downstream) is entitled to rely on that. Creating the alloca with the default
; alignment leaves the copy 8-byte aligned and the callee then reads the wrong
; bytes.

%struct.double8 = type { double, double, double, double, double, double, double, double }
%struct.plain = type { i32, [10 x i32], i32 }

define spir_kernel void @kernel(ptr byval(%struct.double8) align 64 %s, ptr byval(%struct.plain) %p) {
; CHECK-LABEL: @kernel(

; COM: alignment 64 comes from the byval attribute, not from the struct layout
; CHECK: [[ALLOCA0:%.*]] = alloca %struct.double8, align 64
; CHECK: call void @llvm.memcpy.p0.p0.i64(ptr align 64 [[ALLOCA0]], ptr align 64 %s, i64 64, i1 false)
; CHECK: call void @f0(ptr byval(%struct.double8) align 64 [[ALLOCA0]])
  call void @f0(ptr byval(%struct.double8) align 64 %s)

; COM: without an explicit alignment on the attribute nothing changes: the alloca
; COM: keeps the preferred alignment CreateAlloca() gives it (8 for an aggregate
; COM: under the default data layout, even though the ABI alignment is only 4),
; COM: and the source is still only assumed to be ABI aligned.
; CHECK: [[ALLOCA1:%.*]] = alloca %struct.plain, align 8
; CHECK: call void @llvm.memcpy.p0.p0.i64(ptr align 8 [[ALLOCA1]], ptr align 4 %p, i64 48, i1 false)
; CHECK: call void @f1(ptr byval(%struct.plain) [[ALLOCA1]])
  call void @f1(ptr byval(%struct.plain) %p)

  ret void
}

define spir_func void @f0(ptr byval(%struct.double8) align 64 %src) #0 {
  store double 2.000000e+00, ptr %src, align 64
  ret void
}

define spir_func void @f1(ptr byval(%struct.plain) %src) #0 {
  store i32 222, ptr %src, align 4
  ret void
}

attributes #0 = { noinline }
