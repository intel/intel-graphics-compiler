;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-legalization -verify -S %s -o - | FileCheck %s

; Test checks that igc-legalization lowers llvm.bswap of an illegal bit width by
; extending to the next legal type (16, 32 or 64 bits), performing the byte swap
; and then shifting and truncating back. Note: llvm.bswap requires an even
; number of bytes, so i48 is the only illegal width in the 16..64 bit range that
; is valid LLVM IR.

define i48 @f_i48(i48 %a) #0 {
  %r = call i48 @llvm.bswap.i48(i48 %a)
  ret i48 %r
}

; CHECK-LABEL: define i48 @f_i48
; CHECK: %1 = zext i48 %a to i64
; CHECK: %2 = call i64 @llvm.bswap.i64(i64 %1)
; CHECK: %3 = lshr i64 %2, 16
; CHECK: %4 = trunc i64 %3 to i48
; CHECK: ret i48 %4

define <4 x i48> @f_v4i48(<4 x i48> %a) #0 {
  %r = call <4 x i48> @llvm.bswap.v4i48(<4 x i48> %a)
  ret <4 x i48> %r
}

; CHECK-LABEL: define <4 x i48> @f_v4i48
; CHECK: %1 = extractelement <4 x i48> %a, i32 0
; CHECK: %2 = zext i48 %1 to i64
; CHECK: %3 = call i64 @llvm.bswap.i64(i64 %2)
; CHECK: %4 = lshr i64 %3, 16
; CHECK: %5 = trunc i64 %4 to i48

declare i48 @llvm.bswap.i48(i48) #1

declare <4 x i48> @llvm.bswap.v4i48(<4 x i48>) #1

attributes #0 = { nounwind }
attributes #1 = { nounwind readnone speculatable }

!igc.functions = !{!0, !1}

!0 = !{i48 (i48)* @f_i48, !5}
!1 = !{<4 x i48> (<4 x i48>)* @f_v4i48, !5}
!5 = !{}
