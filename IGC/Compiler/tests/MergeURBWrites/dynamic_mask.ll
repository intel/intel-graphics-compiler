;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN:          igc_opt -S --inpuths --platformdg2 --igc-merge-urb-writes %s | FileCheck %s
; RUN:          igc_opt -S --inpuths --platformskl --igc-merge-urb-writes %s | FileCheck %s
;
; CHECK:        call void @llvm.genx.GenISA.URBWrite
; CHECK-SAME:   (i32 0, i32 17, float 0x3FD5555560000000, float undef, float undef, float undef, float 0x3FD5555560000000, float undef, float undef, float undef)
; CHECK:        call void @llvm.genx.GenISA.URBWrite
; CHECK-SAME:   (i32 0, i32 %0, float undef, float 0x3FD5555560000000, float undef, float undef, float undef, float undef, float undef, float undef)
; CHECK:        call void @llvm.genx.GenISA.URBWrite
; CHECK-SAME:   (i32 1, i32 %1, float undef, float 0x3FD5555560000000, float undef, float undef, float undef, float undef, float undef, float undef)
; CHECK-NOT:    call void @llvm.genx.GenISA.URBWrite({{.*}})

define void @entry(i32, i32) #0 {
Label-1:
  call void @llvm.genx.GenISA.URBWrite(i32 0, i32 1, float 0x3FD5555560000000, float undef, float undef, float undef, float undef, float undef, float undef, float undef)
  call void @llvm.genx.GenISA.URBWrite(i32 1, i32 1, float 0x3FD5555560000000, float undef, float undef, float undef, float undef, float undef, float undef, float undef)
  call void @llvm.genx.GenISA.URBWrite(i32 0, i32 %0, float undef, float 0x3FD5555560000000, float undef, float undef, float undef, float undef, float undef, float undef)
  call void @llvm.genx.GenISA.URBWrite(i32 1, i32 %1, float undef, float 0x3FD5555560000000, float undef, float undef, float undef, float undef, float undef, float undef)
  ret void
}

; Function Attrs: nounwind
declare void @llvm.genx.GenISA.URBWrite(i32, i32, float, float, float, float, float, float, float, float) #1

attributes #0 = { "null-pointer-is-valid"="true" }
attributes #1 = { nounwind }