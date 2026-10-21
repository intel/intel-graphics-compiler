; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --igc-vectorizer --regkey=VectorizerAllowEXP2=0 --regkey=VectorizerEnablePartialVectorization=0 --regkey=VectorizerDepWindowMultiplier=6 --platformbmg < %s 2>&1 | FileCheck %s

; The store inside the slice scope depends on the slice, so all fixed order
; instructions (store, load) are moved after the slice. %x uses the load and
; doesn't depend on the slice, it has to be moved after together with the load,
; otherwise it ends up before its definition.

; CHECK-LABEL: @quux(
; CHECK: %vectorized_binary = fmul fast <8 x float>
; CHECK: store float {{.*}}, ptr addrspace(1) %p
; CHECK: %l = load float, ptr addrspace(1) %q
; CHECK: %x = fadd float %l, 1.000000e+00
; CHECK: store float %x, ptr addrspace(1) %q

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @quux(ptr addrspace(1) %p, ptr addrspace(1) %q) #0 {
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
  store float %16, ptr addrspace(1) %p, align 4
  %l = load float, ptr addrspace(1) %q, align 4
  %x = fadd float %l, 1.000000e+00
  %17 = fmul fast float %9, %1
  %18 = fmul fast float %10, %2
  %19 = fmul fast float %11, %3
  %20 = fmul fast float %12, %4
  %21 = fmul fast float %13, %5
  %22 = fmul fast float %14, %6
  %23 = fmul fast float %15, %7
  store float %x, ptr addrspace(1) %q, align 4
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
