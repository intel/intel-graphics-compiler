; REQUIRES: llvm-16-plus
; RUN: igc_opt -S --igc-vectorizer -dce --platformbmg < %s 2>&1 | FileCheck %s

; An already vectorized value can be reused as an operand of the next slice only
; if lane i of the operand slice is lane i of that vector. Otherwise the operand
; has to be built from the extracts in the order the slice needs them.

; operand 1 is the fmul result rotated by one lane -> rebuilt, not reused
; CHECK-LABEL: @k(
; CHECK: %vectorized_binary = fmul <8 x float> %0, %1
; CHECK: [[E0:%.*]] = extractelement <8 x float> %vectorized_binary, i32 0
; CHECK-NEXT: [[E1:%.*]] = extractelement <8 x float> %vectorized_binary, i32 1
; CHECK-NEXT: [[E2:%.*]] = extractelement <8 x float> %vectorized_binary, i32 2
; CHECK-NEXT: [[E3:%.*]] = extractelement <8 x float> %vectorized_binary, i32 3
; CHECK-NEXT: [[E4:%.*]] = extractelement <8 x float> %vectorized_binary, i32 4
; CHECK-NEXT: [[E5:%.*]] = extractelement <8 x float> %vectorized_binary, i32 5
; CHECK-NEXT: [[E6:%.*]] = extractelement <8 x float> %vectorized_binary, i32 6
; CHECK-NEXT: [[E7:%.*]] = extractelement <8 x float> %vectorized_binary, i32 7
; CHECK-NEXT: [[R0:%.*]] = insertelement <8 x float> undef, float [[E1]], i32 0
; CHECK-NEXT: [[R1:%.*]] = insertelement <8 x float> [[R0]], float [[E2]], i32 1
; CHECK-NEXT: [[R2:%.*]] = insertelement <8 x float> [[R1]], float [[E3]], i32 2
; CHECK-NEXT: [[R3:%.*]] = insertelement <8 x float> [[R2]], float [[E4]], i32 3
; CHECK-NEXT: [[R4:%.*]] = insertelement <8 x float> [[R3]], float [[E5]], i32 4
; CHECK-NEXT: [[R5:%.*]] = insertelement <8 x float> [[R4]], float [[E6]], i32 5
; CHECK-NEXT: [[R6:%.*]] = insertelement <8 x float> [[R5]], float [[E7]], i32 6
; CHECK-NEXT: [[R7:%.*]] = insertelement <8 x float> [[R6]], float [[E0]], i32 7
; CHECK-NEXT: [[ADD:%.*]] = fadd <8 x float> %vectorized_binary, [[R7]]
; CHECK-NEXT: store <8 x float> [[ADD]], ptr addrspace(1) %out

; same lane order -> the fmul result is reused as is for both operands
; CHECK-LABEL: @same_order(
; CHECK: %vectorized_binary = fmul <4 x float> %va, %vb
; CHECK-NEXT: [[ADD:%.*]] = fadd <4 x float> %vectorized_binary, %vectorized_binary
; CHECK-NEXT: store <4 x float> [[ADD]], ptr addrspace(1) %out

; operand 1 rotated: <m1, m2, m3, m0>
; CHECK-LABEL: @rotate(
; CHECK: %vectorized_binary = fmul <4 x float> %va, %vb
; CHECK: [[E0:%.*]] = extractelement <4 x float> %vectorized_binary, i32 0
; CHECK-NEXT: [[E1:%.*]] = extractelement <4 x float> %vectorized_binary, i32 1
; CHECK-NEXT: [[E2:%.*]] = extractelement <4 x float> %vectorized_binary, i32 2
; CHECK-NEXT: [[E3:%.*]] = extractelement <4 x float> %vectorized_binary, i32 3
; CHECK-NEXT: [[R0:%.*]] = insertelement <4 x float> undef, float [[E1]], i32 0
; CHECK-NEXT: [[R1:%.*]] = insertelement <4 x float> [[R0]], float [[E2]], i32 1
; CHECK-NEXT: [[R2:%.*]] = insertelement <4 x float> [[R1]], float [[E3]], i32 2
; CHECK-NEXT: [[R3:%.*]] = insertelement <4 x float> [[R2]], float [[E0]], i32 3
; CHECK-NEXT: [[ADD:%.*]] = fadd <4 x float> %vectorized_binary, [[R3]]
; CHECK-NEXT: store <4 x float> [[ADD]], ptr addrspace(1) %out

; operand 0 rotated: <m1, m2, m3, m0>. The fmul slice is taken in the order
; operand 0 uses it, so its extracts are rotated too and are gathered through
; extract interfaces. Operand 0 of fadd reuses the fmul result as is,
; operand 1 is rebuilt in the order <m0, m1, m2, m3>
; CHECK-LABEL: @rotate_op0(
; CHECK: [[A0:%.*]] = extractelement <4 x float> %va, i64 0
; CHECK-NEXT: [[A1:%.*]] = extractelement <4 x float> %va, i64 1
; CHECK-NEXT: [[A2:%.*]] = extractelement <4 x float> %va, i64 2
; CHECK-NEXT: [[A3:%.*]] = extractelement <4 x float> %va, i64 3
; CHECK-NEXT: [[VA0:%.*]] = insertelement <4 x float> undef, float [[A1]], i32 0
; CHECK-NEXT: [[VA1:%.*]] = insertelement <4 x float> [[VA0]], float [[A2]], i32 1
; CHECK-NEXT: [[VA2:%.*]] = insertelement <4 x float> [[VA1]], float [[A3]], i32 2
; CHECK-NEXT: [[VA3:%.*]] = insertelement <4 x float> [[VA2]], float [[A0]], i32 3
; CHECK-NEXT: [[B0:%.*]] = extractelement <4 x float> %vb, i64 0
; CHECK-NEXT: [[B1:%.*]] = extractelement <4 x float> %vb, i64 1
; CHECK-NEXT: [[B2:%.*]] = extractelement <4 x float> %vb, i64 2
; CHECK-NEXT: [[B3:%.*]] = extractelement <4 x float> %vb, i64 3
; CHECK-NEXT: [[VB0:%.*]] = insertelement <4 x float> undef, float [[B1]], i32 0
; CHECK-NEXT: [[VB1:%.*]] = insertelement <4 x float> [[VB0]], float [[B2]], i32 1
; CHECK-NEXT: [[VB2:%.*]] = insertelement <4 x float> [[VB1]], float [[B3]], i32 2
; CHECK-NEXT: [[VB3:%.*]] = insertelement <4 x float> [[VB2]], float [[B0]], i32 3
; CHECK-NEXT: %vectorized_binary = fmul <4 x float> [[VA3]], [[VB3]]
; CHECK-NEXT: [[E0:%.*]] = extractelement <4 x float> %vectorized_binary, i32 0
; CHECK-NEXT: [[E1:%.*]] = extractelement <4 x float> %vectorized_binary, i32 1
; CHECK-NEXT: [[E2:%.*]] = extractelement <4 x float> %vectorized_binary, i32 2
; CHECK-NEXT: [[E3:%.*]] = extractelement <4 x float> %vectorized_binary, i32 3
; CHECK-NEXT: [[R0:%.*]] = insertelement <4 x float> undef, float [[E3]], i32 0
; CHECK-NEXT: [[R1:%.*]] = insertelement <4 x float> [[R0]], float [[E0]], i32 1
; CHECK-NEXT: [[R2:%.*]] = insertelement <4 x float> [[R1]], float [[E1]], i32 2
; CHECK-NEXT: [[R3:%.*]] = insertelement <4 x float> [[R2]], float [[E2]], i32 3
; CHECK-NEXT: [[ADD:%.*]] = fadd <4 x float> %vectorized_binary, [[R3]]
; CHECK-NEXT: store <4 x float> [[ADD]], ptr addrspace(1) %out

; operand 1 reversed: <m3, m2, m1, m0>
; CHECK-LABEL: @reverse(
; CHECK: %vectorized_binary = fmul <4 x float> %va, %vb
; CHECK: [[E0:%.*]] = extractelement <4 x float> %vectorized_binary, i32 0
; CHECK-NEXT: [[E1:%.*]] = extractelement <4 x float> %vectorized_binary, i32 1
; CHECK-NEXT: [[E2:%.*]] = extractelement <4 x float> %vectorized_binary, i32 2
; CHECK-NEXT: [[E3:%.*]] = extractelement <4 x float> %vectorized_binary, i32 3
; CHECK-NEXT: [[R0:%.*]] = insertelement <4 x float> undef, float [[E3]], i32 0
; CHECK-NEXT: [[R1:%.*]] = insertelement <4 x float> [[R0]], float [[E2]], i32 1
; CHECK-NEXT: [[R2:%.*]] = insertelement <4 x float> [[R1]], float [[E1]], i32 2
; CHECK-NEXT: [[R3:%.*]] = insertelement <4 x float> [[R2]], float [[E0]], i32 3
; CHECK-NEXT: [[ADD:%.*]] = fadd <4 x float> %vectorized_binary, [[R3]]
; CHECK-NEXT: store <4 x float> [[ADD]], ptr addrspace(1) %out

; fmul operand 1 is a swizzle of the loaded vector: %vb must not be used as is,
; it is gathered in the order <b1, b0, b3, b2> through an extract interface
; CHECK-LABEL: @swizzled_load(
; CHECK-NOT: <4 x float> %va, %vb
; CHECK: [[B0:%.*]] = extractelement <4 x float> %vb, i64 0
; CHECK-NEXT: [[B1:%.*]] = extractelement <4 x float> %vb, i64 1
; CHECK-NEXT: [[B2:%.*]] = extractelement <4 x float> %vb, i64 2
; CHECK-NEXT: [[B3:%.*]] = extractelement <4 x float> %vb, i64 3
; CHECK-NEXT: [[V0:%.*]] = insertelement <4 x float> undef, float [[B1]], i32 0
; CHECK-NEXT: [[V1:%.*]] = insertelement <4 x float> [[V0]], float [[B0]], i32 1
; CHECK-NEXT: [[V2:%.*]] = insertelement <4 x float> [[V1]], float [[B3]], i32 2
; CHECK-NEXT: [[V3:%.*]] = insertelement <4 x float> [[V2]], float [[B2]], i32 3
; CHECK-NEXT: %vectorized_binary = fmul <4 x float> %va, [[V3]]
; CHECK-NEXT: [[ADD:%.*]] = fadd <4 x float> %vectorized_binary, %vectorized_binary
; CHECK-NEXT: store <4 x float> [[ADD]], ptr addrspace(1) %out

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @k(ptr addrspace(1) align 32 %a, ptr addrspace(1) align 32 %b, ptr addrspace(1) align 32 %out) {
entry:
  %0 = load <8 x float>, ptr addrspace(1) %a, align 32
  %.scalar = extractelement <8 x float> %0, i64 0
  %.scalar5 = extractelement <8 x float> %0, i64 1
  %.scalar6 = extractelement <8 x float> %0, i64 2
  %.scalar7 = extractelement <8 x float> %0, i64 3
  %.scalar8 = extractelement <8 x float> %0, i64 4
  %.scalar9 = extractelement <8 x float> %0, i64 5
  %.scalar10 = extractelement <8 x float> %0, i64 6
  %.scalar11 = extractelement <8 x float> %0, i64 7
  %1 = load <8 x float>, ptr addrspace(1) %b, align 32
  %.scalar12 = extractelement <8 x float> %1, i64 0
  %.scalar13 = extractelement <8 x float> %1, i64 1
  %.scalar14 = extractelement <8 x float> %1, i64 2
  %.scalar15 = extractelement <8 x float> %1, i64 3
  %.scalar16 = extractelement <8 x float> %1, i64 4
  %.scalar17 = extractelement <8 x float> %1, i64 5
  %.scalar18 = extractelement <8 x float> %1, i64 6
  %.scalar19 = extractelement <8 x float> %1, i64 7
  %mul.i20 = fmul float %.scalar, %.scalar12
  %mul.i21 = fmul float %.scalar5, %.scalar13
  %mul.i22 = fmul float %.scalar6, %.scalar14
  %mul.i23 = fmul float %.scalar7, %.scalar15
  %mul.i24 = fmul float %.scalar8, %.scalar16
  %mul.i25 = fmul float %.scalar9, %.scalar17
  %mul.i26 = fmul float %.scalar10, %.scalar18
  %mul.i27 = fmul float %.scalar11, %.scalar19
  %add.i28 = fadd float %mul.i20, %mul.i21
  %add.i29 = fadd float %mul.i21, %mul.i22
  %add.i30 = fadd float %mul.i22, %mul.i23
  %add.i31 = fadd float %mul.i23, %mul.i24
  %add.i32 = fadd float %mul.i24, %mul.i25
  %add.i33 = fadd float %mul.i25, %mul.i26
  %add.i34 = fadd float %mul.i26, %mul.i27
  %add.i35 = fadd float %mul.i27, %mul.i20
  %add.i.assembled.vect = insertelement <8 x float> poison, float %add.i28, i64 0
  %add.i.assembled.vect36 = insertelement <8 x float> %add.i.assembled.vect, float %add.i29, i64 1
  %add.i.assembled.vect37 = insertelement <8 x float> %add.i.assembled.vect36, float %add.i30, i64 2
  %add.i.assembled.vect38 = insertelement <8 x float> %add.i.assembled.vect37, float %add.i31, i64 3
  %add.i.assembled.vect39 = insertelement <8 x float> %add.i.assembled.vect38, float %add.i32, i64 4
  %add.i.assembled.vect40 = insertelement <8 x float> %add.i.assembled.vect39, float %add.i33, i64 5
  %add.i.assembled.vect41 = insertelement <8 x float> %add.i.assembled.vect40, float %add.i34, i64 6
  %add.i.assembled.vect42 = insertelement <8 x float> %add.i.assembled.vect41, float %add.i35, i64 7
  store <8 x float> %add.i.assembled.vect42, ptr addrspace(1) %out, align 32
  ret void
}

define spir_kernel void @same_order(ptr addrspace(1) align 16 %a, ptr addrspace(1) align 16 %b, ptr addrspace(1) align 16 %out) {
entry:
  %va = load <4 x float>, ptr addrspace(1) %a, align 16
  %vb = load <4 x float>, ptr addrspace(1) %b, align 16
  %a0 = extractelement <4 x float> %va, i64 0
  %a1 = extractelement <4 x float> %va, i64 1
  %a2 = extractelement <4 x float> %va, i64 2
  %a3 = extractelement <4 x float> %va, i64 3
  %b0 = extractelement <4 x float> %vb, i64 0
  %b1 = extractelement <4 x float> %vb, i64 1
  %b2 = extractelement <4 x float> %vb, i64 2
  %b3 = extractelement <4 x float> %vb, i64 3
  %m0 = fmul float %a0, %b0
  %m1 = fmul float %a1, %b1
  %m2 = fmul float %a2, %b2
  %m3 = fmul float %a3, %b3
  %s0 = fadd float %m0, %m0
  %s1 = fadd float %m1, %m1
  %s2 = fadd float %m2, %m2
  %s3 = fadd float %m3, %m3
  %v0 = insertelement <4 x float> poison, float %s0, i64 0
  %v1 = insertelement <4 x float> %v0, float %s1, i64 1
  %v2 = insertelement <4 x float> %v1, float %s2, i64 2
  %v3 = insertelement <4 x float> %v2, float %s3, i64 3
  store <4 x float> %v3, ptr addrspace(1) %out, align 16
  ret void
}

define spir_kernel void @rotate(ptr addrspace(1) align 16 %a, ptr addrspace(1) align 16 %b, ptr addrspace(1) align 16 %out) {
entry:
  %va = load <4 x float>, ptr addrspace(1) %a, align 16
  %vb = load <4 x float>, ptr addrspace(1) %b, align 16
  %a0 = extractelement <4 x float> %va, i64 0
  %a1 = extractelement <4 x float> %va, i64 1
  %a2 = extractelement <4 x float> %va, i64 2
  %a3 = extractelement <4 x float> %va, i64 3
  %b0 = extractelement <4 x float> %vb, i64 0
  %b1 = extractelement <4 x float> %vb, i64 1
  %b2 = extractelement <4 x float> %vb, i64 2
  %b3 = extractelement <4 x float> %vb, i64 3
  %m0 = fmul float %a0, %b0
  %m1 = fmul float %a1, %b1
  %m2 = fmul float %a2, %b2
  %m3 = fmul float %a3, %b3
  %s0 = fadd float %m0, %m1
  %s1 = fadd float %m1, %m2
  %s2 = fadd float %m2, %m3
  %s3 = fadd float %m3, %m0
  %v0 = insertelement <4 x float> poison, float %s0, i64 0
  %v1 = insertelement <4 x float> %v0, float %s1, i64 1
  %v2 = insertelement <4 x float> %v1, float %s2, i64 2
  %v3 = insertelement <4 x float> %v2, float %s3, i64 3
  store <4 x float> %v3, ptr addrspace(1) %out, align 16
  ret void
}

define spir_kernel void @rotate_op0(ptr addrspace(1) align 16 %a, ptr addrspace(1) align 16 %b, ptr addrspace(1) align 16 %out) {
entry:
  %va = load <4 x float>, ptr addrspace(1) %a, align 16
  %vb = load <4 x float>, ptr addrspace(1) %b, align 16
  %a0 = extractelement <4 x float> %va, i64 0
  %a1 = extractelement <4 x float> %va, i64 1
  %a2 = extractelement <4 x float> %va, i64 2
  %a3 = extractelement <4 x float> %va, i64 3
  %b0 = extractelement <4 x float> %vb, i64 0
  %b1 = extractelement <4 x float> %vb, i64 1
  %b2 = extractelement <4 x float> %vb, i64 2
  %b3 = extractelement <4 x float> %vb, i64 3
  %m0 = fmul float %a0, %b0
  %m1 = fmul float %a1, %b1
  %m2 = fmul float %a2, %b2
  %m3 = fmul float %a3, %b3
  %s0 = fadd float %m1, %m0
  %s1 = fadd float %m2, %m1
  %s2 = fadd float %m3, %m2
  %s3 = fadd float %m0, %m3
  %v0 = insertelement <4 x float> poison, float %s0, i64 0
  %v1 = insertelement <4 x float> %v0, float %s1, i64 1
  %v2 = insertelement <4 x float> %v1, float %s2, i64 2
  %v3 = insertelement <4 x float> %v2, float %s3, i64 3
  store <4 x float> %v3, ptr addrspace(1) %out, align 16
  ret void
}

define spir_kernel void @reverse(ptr addrspace(1) align 16 %a, ptr addrspace(1) align 16 %b, ptr addrspace(1) align 16 %out) {
entry:
  %va = load <4 x float>, ptr addrspace(1) %a, align 16
  %vb = load <4 x float>, ptr addrspace(1) %b, align 16
  %a0 = extractelement <4 x float> %va, i64 0
  %a1 = extractelement <4 x float> %va, i64 1
  %a2 = extractelement <4 x float> %va, i64 2
  %a3 = extractelement <4 x float> %va, i64 3
  %b0 = extractelement <4 x float> %vb, i64 0
  %b1 = extractelement <4 x float> %vb, i64 1
  %b2 = extractelement <4 x float> %vb, i64 2
  %b3 = extractelement <4 x float> %vb, i64 3
  %m0 = fmul float %a0, %b0
  %m1 = fmul float %a1, %b1
  %m2 = fmul float %a2, %b2
  %m3 = fmul float %a3, %b3
  %s0 = fadd float %m0, %m3
  %s1 = fadd float %m1, %m2
  %s2 = fadd float %m2, %m1
  %s3 = fadd float %m3, %m0
  %v0 = insertelement <4 x float> poison, float %s0, i64 0
  %v1 = insertelement <4 x float> %v0, float %s1, i64 1
  %v2 = insertelement <4 x float> %v1, float %s2, i64 2
  %v3 = insertelement <4 x float> %v2, float %s3, i64 3
  store <4 x float> %v3, ptr addrspace(1) %out, align 16
  ret void
}

define spir_kernel void @swizzled_load(ptr addrspace(1) align 16 %a, ptr addrspace(1) align 16 %b, ptr addrspace(1) align 16 %out) {
entry:
  %va = load <4 x float>, ptr addrspace(1) %a, align 16
  %vb = load <4 x float>, ptr addrspace(1) %b, align 16
  %a0 = extractelement <4 x float> %va, i64 0
  %a1 = extractelement <4 x float> %va, i64 1
  %a2 = extractelement <4 x float> %va, i64 2
  %a3 = extractelement <4 x float> %va, i64 3
  %b0 = extractelement <4 x float> %vb, i64 0
  %b1 = extractelement <4 x float> %vb, i64 1
  %b2 = extractelement <4 x float> %vb, i64 2
  %b3 = extractelement <4 x float> %vb, i64 3
  %m0 = fmul float %a0, %b1
  %m1 = fmul float %a1, %b0
  %m2 = fmul float %a2, %b3
  %m3 = fmul float %a3, %b2
  %s0 = fadd float %m0, %m0
  %s1 = fadd float %m1, %m1
  %s2 = fadd float %m2, %m2
  %s3 = fadd float %m3, %m3
  %v0 = insertelement <4 x float> poison, float %s0, i64 0
  %v1 = insertelement <4 x float> %v0, float %s1, i64 1
  %v2 = insertelement <4 x float> %v1, float %s2, i64 2
  %v3 = insertelement <4 x float> %v2, float %s3, i64 3
  store <4 x float> %v3, ptr addrspace(1) %out, align 16
  ret void
}

!igc.functions = !{!0, !1, !2, !3, !4, !5}
!0 = !{ptr @k, !6}
!1 = !{ptr @same_order, !6}
!2 = !{ptr @rotate, !6}
!3 = !{ptr @rotate_op0, !6}
!4 = !{ptr @reverse, !6}
!5 = !{ptr @swizzled_load, !6}
!6 = !{!7}
!7 = !{!"function_type", i32 0}
!8 = !{!"requiredSubGroupSize", i32 16}
!9 = !{!"FuncMDMap[0]", ptr @k}
!10 = !{!"FuncMDValue[0]", !8}
!11 = !{!"FuncMDMap[1]", ptr @same_order}
!12 = !{!"FuncMDValue[1]", !8}
!13 = !{!"FuncMDMap[2]", ptr @rotate}
!14 = !{!"FuncMDValue[2]", !8}
!15 = !{!"FuncMDMap[3]", ptr @rotate_op0}
!16 = !{!"FuncMDValue[3]", !8}
!17 = !{!"FuncMDMap[4]", ptr @reverse}
!18 = !{!"FuncMDValue[4]", !8}
!19 = !{!"FuncMDMap[5]", ptr @swizzled_load}
!20 = !{!"FuncMDValue[5]", !8}
!21 = !{!"FuncMD", !9, !10, !11, !12, !13, !14, !15, !16, !17, !18, !19, !20}
!22 = !{!"ModuleMD", !21}
!IGCMetadata = !{!22}
