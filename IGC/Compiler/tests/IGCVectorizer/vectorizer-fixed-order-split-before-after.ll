; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --igc-vectorizer --regkey=VectorizerAllowEXP2=0 --regkey=VectorizerEnablePartialVectorization=0 --regkey=VectorizerDepWindowMultiplier=6 --platformbmg < %s 2>&1 | FileCheck %s

; Loads and stores inside the slice scope are rescheduled into two groups
; while keeping their relative order:
;   %l1           - slice doesn't depend on it, located before the flip -> before
;   %l2           - slice depends on it                                -> before
;   store %p      - depends on the slice (flip)                        -> after
;   %l3, store %q - located after the flip                             -> after
; %x uses both %l1 (before) and %l3 (after), so it goes after together with %l3.

; CHECK-LABEL: loop:
; CHECK: %l1 = load float, ptr addrspace(1) %a
; CHECK-NEXT: %l2 = load float, ptr addrspace(1) %b
; CHECK: %vectorized_binary = fmul fast <8 x float>
; CHECK: [[EXT:%.*]] = extractelement <8 x float> %vectorized_binary, i32 0
; CHECK: fmul fast float %8, %vector_extract
; CHECK-NEXT: fmul fast float %9, %vector_extract1
; CHECK-NEXT: fmul fast float %l2, %vector_extract2
; CHECK-NEXT: fmul fast float %11, %vector_extract3
; CHECK-NEXT: fmul fast float %12, %vector_extract4
; CHECK-NEXT: fmul fast float %13, %vector_extract5
; CHECK-NEXT: fmul fast float %14, %vector_extract6
; CHECK-NEXT: fmul fast float %15, %vector_extract7
; CHECK-NEXT: store float [[EXT]], ptr addrspace(1) %p
; CHECK-NEXT: %l3 = load float, ptr addrspace(1) %q
; CHECK-NEXT: %x = fadd float %l3, %l1
; CHECK-NEXT: store float %x, ptr addrspace(1) %q
; CHECK-NEXT: store float %l1, ptr addrspace(1) %a

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @quux(ptr addrspace(1) %p, ptr addrspace(1) %q, ptr addrspace(1) %a, ptr addrspace(1) %b) #0 {
entry:
  br label %loop

loop:
  %0 = phi float [ 0.000000e+00, %entry ], [ %34, %loop ]
  %1 = phi float [ 0.000000e+00, %entry ], [ %35, %loop ]
  %2 = phi float [ 0.000000e+00, %entry ], [ %36, %loop ]
  %3 = phi float [ 0.000000e+00, %entry ], [ %37, %loop ]
  %4 = phi float [ 0.000000e+00, %entry ], [ %38, %loop ]
  %5 = phi float [ 0.000000e+00, %entry ], [ %39, %loop ]
  %6 = phi float [ 0.000000e+00, %entry ], [ %40, %loop ]
  %7 = phi float [ 0.000000e+00, %entry ], [ %41, %loop ]
  %8 = call float @llvm.exp2.f32(float 0.000000e+00)
  %9 = call float @llvm.exp2.f32(float 0.000000e+00)
  %10 = call float @llvm.exp2.f32(float 0.000000e+00)
  %11 = call float @llvm.exp2.f32(float 0.000000e+00)
  %12 = call float @llvm.exp2.f32(float 0.000000e+00)
  %13 = call float @llvm.exp2.f32(float 0.000000e+00)
  %14 = call float @llvm.exp2.f32(float 0.000000e+00)
  %15 = call float @llvm.exp2.f32(float 0.000000e+00)
  %16 = fmul fast float %8, %0
  %l1 = load float, ptr addrspace(1) %a, align 4
  %17 = fmul fast float %9, %1
  %l2 = load float, ptr addrspace(1) %b, align 4
  %18 = fmul fast float %l2, %2
  store float %16, ptr addrspace(1) %p, align 4
  %19 = fmul fast float %11, %3
  %l3 = load float, ptr addrspace(1) %q, align 4
  %x = fadd float %l3, %l1
  %20 = fmul fast float %12, %4
  store float %x, ptr addrspace(1) %q, align 4
  %21 = fmul fast float %13, %5
  %22 = fmul fast float %14, %6
  %23 = fmul fast float %15, %7
  store float %l1, ptr addrspace(1) %a, align 4
  %24 = insertelement <8 x float> zeroinitializer, float %16, i64 0
  %25 = insertelement <8 x float> %24, float %17, i64 1
  %26 = insertelement <8 x float> %25, float %18, i64 2
  %27 = insertelement <8 x float> %26, float %19, i64 3
  %28 = insertelement <8 x float> %27, float %20, i64 4
  %29 = insertelement <8 x float> %28, float %21, i64 5
  %30 = insertelement <8 x float> %29, float %22, i64 6
  %31 = insertelement <8 x float> %30, float %23, i64 7
  %32 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %31, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %33 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %32, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %34 = extractelement <8 x float> %33, i64 0
  %35 = extractelement <8 x float> %33, i64 1
  %36 = extractelement <8 x float> %33, i64 2
  %37 = extractelement <8 x float> %33, i64 3
  %38 = extractelement <8 x float> %33, i64 4
  %39 = extractelement <8 x float> %33, i64 5
  %40 = extractelement <8 x float> %33, i64 6
  %41 = extractelement <8 x float> %33, i64 7
  br label %loop
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #1
declare float @llvm.exp2.f32(float) #2

attributes #0 = { convergent nounwind }
attributes #1 = { convergent nounwind readnone willreturn }
attributes #2 = { nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{!0}
!0 = !{ptr @quux, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", ptr @quux}
!7 = !{!"FuncMD", !6, !5}
!8 = !{!"ModuleMD", !7}
!IGCMetadata = !{!8}
