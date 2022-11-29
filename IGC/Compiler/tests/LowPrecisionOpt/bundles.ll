; RUN: igc_opt -igc-low-precision-opt -inputps -S < %s
; ------------------------------------------------
; LowPrecisionOpt
; ------------------------------------------------
; This test checks that fptrunc bundle works
;


; CHECK:   %0 = alloca half, align 4
; CHECK:   %1 = call float @llvm.genx.GenISA.RuntimeValue.f32(i32 2)
; CHECK:   %2 = fptrunc float %1 to half
; CHECK:   %3 = call float @llvm.genx.GenISA.RuntimeValue.f32(i32 1)
; CHECK:   %4 = fptrunc float %3 to half
; CHECK:   store half %4, half* %0

define void @test_low(float %src) {
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
