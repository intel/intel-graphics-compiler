;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-lower-byval-attribute -S %s | FileCheck %s

%struct.double8 = type { double, double, double, double, double, double, double, double }

define spir_kernel void @kernel(ptr byval(%struct.double8) align 64 %src1, ptr byval(%struct.double8) %src2) {
; CHECK: [[ALLOCA:%.*]] = alloca %struct.double8, align 64
; CHECK: call void @llvm.memcpy.p0.p0.i64(ptr align 64 [[ALLOCA]], ptr align 64 %src1, i64 64, i1 false)
; CHECK: [[ALLOCA2:%.*]] = alloca %struct.double8
; CHECK: call void @llvm.memcpy.p0.p0.i64(ptr align 8 [[ALLOCA2]], ptr align 8 %src2, i64 64, i1 false)
  call void @f0(ptr byval(%struct.double8) align 64 %src1)
  call void @f0(ptr byval(%struct.double8) %src2)
  ret void
}

define spir_func void @f0(ptr byval(%struct.double8) align 64 %src) #0 {
  store double 2.0, ptr %src, align 64
  ret void
}

attributes #0 = { noinline }
