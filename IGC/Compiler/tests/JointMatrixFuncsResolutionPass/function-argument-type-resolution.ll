;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
; This software and the related documents are Intel copyrighted materials,
; and your use of them is governed by the express license under which they were
; provided to you ("License"). Unless the License provides otherwise,
; you may not use, modify, copy, publish, distribute, disclose or transmit this
; software or the related documents without Intel's prior written permission.
;
; This software and the related documents are provided as is, with no express or
; implied warranties, other than those that are expressly stated in the License.
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -platformpvc -igc-joint-matrix-resolution -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

%spirv.JointMatrixINTEL._float_8_8_3_3_2 = type opaque
%jm = type { %spirv.JointMatrixINTEL._float_8_8_3_3_2 addrspace(1)* }

define spir_func void @test(%jm* %this) {
entry:
    ret void
}

define spir_func void @test2(%jm* %this, %jm* %this2) {
entry:
    ret void
}

define spir_func void @test3(i32* %this, i32* %this2, %jm* %this3) {
entry:
    ret void
}

define spir_func %jm* @test4(%jm* %this1, %jm* %this2, i32 %a, i32 %b) {
entry:
    %0 = icmp sgt i32 %a, %b
    br i1 %0, label %btrue, label %bfalse

btrue:                                      ; preds = %2
    br label %end

bfalse:                                     ; preds = %2
    br label %end

end:                                     ; preds = %btrue, %bfalse
  %retval = phi %jm* [%this1, %btrue], [%this2, %bfalse]
  ret %jm* %retval
}

define spir_kernel void @main(i32 %a, i32 %b) {
    %tC = alloca %jm, align 8
    %tC2 = alloca %jm, align 8
    %justInt = alloca i32, align 8
    %justInt2 = alloca i32, align 8

    call spir_func void @test(%jm* %tC)
    call spir_func void @test2(%jm* %tC, %jm* %tC2)
    call spir_func void @test3(i32* %justInt, i32* %justInt2, %jm* %tC2)
    call spir_func void @test3(i32* %justInt, i32* %justInt2, %jm* %tC2)

    call spir_func %jm* @test4(%jm* %tC, %jm* %tC2, i32 %a, i32 %b)

    ret void
}

; CHECK: define spir_kernel void @main(i32 %a, i32 %b) {
; CHECK: define spir_func void @test_resolved(%jm.resolved* %this) {
; CHECK: define spir_func void @test2_resolved(%jm.resolved* %this, %jm.resolved* %this2) {
; CHECK: define spir_func void @test3_resolved(i32* %this, i32* %this2, %jm.resolved* %this3) {
; CHECK: define spir_func %jm.resolved* @test4_resolved(%jm.resolved* %this1, %jm.resolved* %this2, i32 %a, i32 %b) {
; CHECK: [[RETVAL:%.*]] = phi %jm.resolved* [ %this1, %btrue ], [ %this2, %bfalse ]
; CHECK: ret %jm.resolved* [[RETVAL]]