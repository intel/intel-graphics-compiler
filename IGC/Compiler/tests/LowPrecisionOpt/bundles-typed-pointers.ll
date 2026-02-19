; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-low-precision-opt -inputps -S < %s | FileCheck %s
; ------------------------------------------------
; LowPrecisionOpt
; ------------------------------------------------
; This test checks that fptrunc bundle works

define void @test_low(float %src) {
; CHECK-LABEL: define void @test_low(
; CHECK-SAME: float [[SRC:%.*]]) {
; CHECK-NEXT:  [[ENTRY:.*:]]
; CHECK-NEXT:    [[TMP0:%.*]] = call float @llvm.genx.GenISA.RuntimeValue.f32(i32 1)
; CHECK-NEXT:    [[TMP1:%.*]] = fptrunc float [[TMP0]] to half
; CHECK-NEXT:    [[TMP2:%.*]] = call float @llvm.genx.GenISA.RuntimeValue.f32(i32 2)
; CHECK-NEXT:    [[TMP3:%.*]] = fptrunc float [[TMP2]] to half
; CHECK-NEXT:    [[TMP4:%.*]] = alloca half, align 4
; CHECK-NEXT:    store half [[TMP1]], half* [[TMP4]], align 2
; CHECK-NEXT:    ret void
;
entry:
; bundles sort GenISA_RuntimeValue + fptrunc
  %0 = alloca half, align 4
  %1 = call float @llvm.genx.GenISA.RuntimeValue.f32(i32 2)
  %2 = fptrunc float %1 to half
  %3 = call float @llvm.genx.GenISA.RuntimeValue.f32(i32 1)
  %4 = fptrunc float %3 to half
  store half %4, half* %0
  ret void
}
declare float @llvm.genx.GenISA.RuntimeValue.f32(i32)

!igc.functions = !{!0}
!0 = !{void (float)* @test_low, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
