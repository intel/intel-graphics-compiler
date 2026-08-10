; RUN: igc_opt --opaque-pointers --igc-private-mem-resolution --platformbmg -S %s | FileCheck %s

; Byte-offset GEP into an alloca whose SOA base type is a 4-byte vector.
; The scalarized index must be 12/4 = 3, not 12.

; CHECK: mul i32 %{{.*}}, 4
; CHECK-NOT: mul i32 12,
; CHECK: mul i32 3,

define spir_kernel void @test() {
  %a = alloca [32 x <4 x i8>], align 4
  %p = getelementptr inbounds i8, ptr %a, i64 12
  %v = load <4 x i8>, ptr %p, align 4
  ret void
}

!igc.functions = !{!1}
!1 = !{ptr @test, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}