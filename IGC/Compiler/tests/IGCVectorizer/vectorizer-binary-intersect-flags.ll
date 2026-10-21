; REQUIRES: llvm-16-plus
; RUN: igc_opt -S --igc-vectorizer -dce --platformbmg < %s 2>&1 | FileCheck %s

; Only the first lane has the fast flag. The vectorized instruction must
; keep only the flags common to all lanes, so it is created without fast.

; CHECK-LABEL: @k(
; CHECK-NOT: fmul fast
; CHECK: %vectorized_binary = fmul <8 x float> %a, %b
; CHECK-NOT: fmul fast
; CHECK: store <8 x float> %vectorized_binary, ptr addrspace(1) %out

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @k(ptr addrspace(1) %pa, ptr addrspace(1) %pb, ptr addrspace(1) %out) {
entry:
  %a = load <8 x float>, ptr addrspace(1) %pa, align 32
  %b = load <8 x float>, ptr addrspace(1) %pb, align 32
  %ea0 = extractelement <8 x float> %a, i64 0
  %eb0 = extractelement <8 x float> %b, i64 0
  %ea1 = extractelement <8 x float> %a, i64 1
  %eb1 = extractelement <8 x float> %b, i64 1
  %ea2 = extractelement <8 x float> %a, i64 2
  %eb2 = extractelement <8 x float> %b, i64 2
  %ea3 = extractelement <8 x float> %a, i64 3
  %eb3 = extractelement <8 x float> %b, i64 3
  %ea4 = extractelement <8 x float> %a, i64 4
  %eb4 = extractelement <8 x float> %b, i64 4
  %ea5 = extractelement <8 x float> %a, i64 5
  %eb5 = extractelement <8 x float> %b, i64 5
  %ea6 = extractelement <8 x float> %a, i64 6
  %eb6 = extractelement <8 x float> %b, i64 6
  %ea7 = extractelement <8 x float> %a, i64 7
  %eb7 = extractelement <8 x float> %b, i64 7
  %s0 = fmul fast float %ea0, %eb0
  %s1 = fmul float %ea1, %eb1
  %s2 = fmul float %ea2, %eb2
  %s3 = fmul float %ea3, %eb3
  %s4 = fmul float %ea4, %eb4
  %s5 = fmul float %ea5, %eb5
  %s6 = fmul float %ea6, %eb6
  %s7 = fmul float %ea7, %eb7
  %v0 = insertelement <8 x float> undef, float %s0, i64 0
  %v1 = insertelement <8 x float> %v0, float %s1, i64 1
  %v2 = insertelement <8 x float> %v1, float %s2, i64 2
  %v3 = insertelement <8 x float> %v2, float %s3, i64 3
  %v4 = insertelement <8 x float> %v3, float %s4, i64 4
  %v5 = insertelement <8 x float> %v4, float %s5, i64 5
  %v6 = insertelement <8 x float> %v5, float %s6, i64 6
  %v7 = insertelement <8 x float> %v6, float %s7, i64 7
  store <8 x float> %v7, ptr addrspace(1) %out, align 32
  ret void
}


!igc.functions = !{!0}
!0 = !{ptr @k, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", ptr @k}
!7 = !{!"FuncMD", !6, !5}
!8 = !{!"ModuleMD", !7}
!IGCMetadata = !{!8}
