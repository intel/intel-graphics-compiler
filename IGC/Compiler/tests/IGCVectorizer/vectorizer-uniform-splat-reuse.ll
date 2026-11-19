;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers -platformbmg -rev-id B -has-emulated-64-bit-insts --igc-vectorizer -dce -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 < %s 2>&1 | FileCheck %s

; CHECK-LABEL: .kernel "splat_reused_payload"
; CHECK: lsc_store_block2d.ugm
; CHECK: mov (M1, 16) [[PAYLOAD:[A-Za-z0-9_]+]](7,0)<1>
; CHECK: lsc_store_block2d.ugm {{.*}} [[PAYLOAD]]:d32.16x8nn

; CHECK-LABEL: .kernel "splat_reused_extract"
; CHECK-NOT: {{[[:space:]]s\(0,[1-9]}}
; CHECK: lsc_store.ugm
; CHECK-NOT: {{[[:space:]]s\(0,[1-9]}}
; CHECK: ret (M1, 1)

; CHECK-LABEL: .kernel "splat_reused_phi"
; CHECK-NOT: (M1_NM, 8) s(0,0)<1>
; CHECK: add (M1_NM, 8) [[ACC:[A-Za-z0-9_]+]](0,0)<1> [[ACC]](0,0)<1;1,0>
; CHECK-NOT: (M1_NM, 8) s(0,0)<1>
; CHECK: ret (M1, 1)

define spir_kernel void @splat_reused_payload(float %a, float %b, i32 %n) #0 {
entry:
  %s = fadd float %a, 1.000000e+00
  %x = fadd float %b, 2.000000e+00
  %x0 = fadd float %x, 1.000000e+00
  %m0 = fmul float %x0, %s
  %x1 = fadd float %x, 2.000000e+00
  %m1 = fmul float %x1, %s
  %x2 = fadd float %x, 3.000000e+00
  %m2 = fmul float %x2, %s
  %x3 = fadd float %x, 4.000000e+00
  %m3 = fmul float %x3, %s
  %x4 = fadd float %x, 5.000000e+00
  %m4 = fmul float %x4, %s
  %x5 = fadd float %x, 6.000000e+00
  %m5 = fmul float %x5, %s
  %x6 = fadd float %x, 7.000000e+00
  %m6 = fmul float %x6, %s
  %x7 = fadd float %x, 8.000000e+00
  %m7 = fmul float %x7, %s
  %mv0 = insertelement <8 x float> undef, float %m0, i64 0
  %mv1 = insertelement <8 x float> %mv0, float %m1, i64 1
  %mv2 = insertelement <8 x float> %mv1, float %m2, i64 2
  %mv3 = insertelement <8 x float> %mv2, float %m3, i64 3
  %mv4 = insertelement <8 x float> %mv3, float %m4, i64 4
  %mv5 = insertelement <8 x float> %mv4, float %m5, i64 5
  %mv6 = insertelement <8 x float> %mv5, float %m6, i64 6
  %mv7 = insertelement <8 x float> %mv6, float %m7, i64 7
  %mbc = bitcast <8 x float> %mv7 to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 -1, i32 -1, i32 -1, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %mbc)
  %sv0 = insertelement <8 x float> undef, float %s, i64 0
  %sv1 = insertelement <8 x float> %sv0, float %s, i64 1
  %sv2 = insertelement <8 x float> %sv1, float %s, i64 2
  %sv3 = insertelement <8 x float> %sv2, float %s, i64 3
  %sv4 = insertelement <8 x float> %sv3, float %s, i64 4
  %sv5 = insertelement <8 x float> %sv4, float %s, i64 5
  %sv6 = insertelement <8 x float> %sv5, float %s, i64 6
  %sv7 = insertelement <8 x float> %sv6, float %s, i64 7
  %sbc = bitcast <8 x float> %sv7 to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 -1, i32 -1, i32 -1, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %sbc)
  ret void
}

define spir_kernel void @splat_reused_extract(float %a, float %b, i32 %n, ptr addrspace(1) %out) #0 {
entry:
  %s = fadd float %a, 1.000000e+00
  %x = fadd float %b, 2.000000e+00
  %x0 = fadd float %x, 1.000000e+00
  %m0 = fmul float %x0, %s
  %x1 = fadd float %x, 2.000000e+00
  %m1 = fmul float %x1, %s
  %x2 = fadd float %x, 3.000000e+00
  %m2 = fmul float %x2, %s
  %x3 = fadd float %x, 4.000000e+00
  %m3 = fmul float %x3, %s
  %x4 = fadd float %x, 5.000000e+00
  %m4 = fmul float %x4, %s
  %x5 = fadd float %x, 6.000000e+00
  %m5 = fmul float %x5, %s
  %x6 = fadd float %x, 7.000000e+00
  %m6 = fmul float %x6, %s
  %x7 = fadd float %x, 8.000000e+00
  %m7 = fmul float %x7, %s
  %mv0 = insertelement <8 x float> undef, float %m0, i64 0
  %mv1 = insertelement <8 x float> %mv0, float %m1, i64 1
  %mv2 = insertelement <8 x float> %mv1, float %m2, i64 2
  %mv3 = insertelement <8 x float> %mv2, float %m3, i64 3
  %mv4 = insertelement <8 x float> %mv3, float %m4, i64 4
  %mv5 = insertelement <8 x float> %mv4, float %m5, i64 5
  %mv6 = insertelement <8 x float> %mv5, float %m6, i64 6
  %mv7 = insertelement <8 x float> %mv6, float %m7, i64 7
  %mbc = bitcast <8 x float> %mv7 to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 -1, i32 -1, i32 -1, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %mbc)
  %sv0 = insertelement <8 x float> undef, float %s, i64 0
  %sv1 = insertelement <8 x float> %sv0, float %s, i64 1
  %sv2 = insertelement <8 x float> %sv1, float %s, i64 2
  %sv3 = insertelement <8 x float> %sv2, float %s, i64 3
  %sv4 = insertelement <8 x float> %sv3, float %s, i64 4
  %sv5 = insertelement <8 x float> %sv4, float %s, i64 5
  %sv6 = insertelement <8 x float> %sv5, float %s, i64 6
  %sv7 = insertelement <8 x float> %sv6, float %s, i64 7
  %e = extractelement <8 x float> %sv7, i64 3
  store float %e, ptr addrspace(1) %out
  ret void
}

define spir_kernel void @splat_reused_phi(float %a, float %b, i32 %n) #0 {
entry:
  %s = fadd float %a, 1.000000e+00
  %x = fadd float %b, 2.000000e+00
  %x0 = fadd float %x, 1.000000e+00
  %m0 = fmul float %x0, %s
  %x1 = fadd float %x, 2.000000e+00
  %m1 = fmul float %x1, %s
  %x2 = fadd float %x, 3.000000e+00
  %m2 = fmul float %x2, %s
  %x3 = fadd float %x, 4.000000e+00
  %m3 = fmul float %x3, %s
  %x4 = fadd float %x, 5.000000e+00
  %m4 = fmul float %x4, %s
  %x5 = fadd float %x, 6.000000e+00
  %m5 = fmul float %x5, %s
  %x6 = fadd float %x, 7.000000e+00
  %m6 = fmul float %x6, %s
  %x7 = fadd float %x, 8.000000e+00
  %m7 = fmul float %x7, %s
  %mv0 = insertelement <8 x float> undef, float %m0, i64 0
  %mv1 = insertelement <8 x float> %mv0, float %m1, i64 1
  %mv2 = insertelement <8 x float> %mv1, float %m2, i64 2
  %mv3 = insertelement <8 x float> %mv2, float %m3, i64 3
  %mv4 = insertelement <8 x float> %mv3, float %m4, i64 4
  %mv5 = insertelement <8 x float> %mv4, float %m5, i64 5
  %mv6 = insertelement <8 x float> %mv5, float %m6, i64 6
  %mv7 = insertelement <8 x float> %mv6, float %m7, i64 7
  %mbc = bitcast <8 x float> %mv7 to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 -1, i32 -1, i32 -1, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %mbc)
  %sv0 = insertelement <8 x float> undef, float %s, i64 0
  %sv1 = insertelement <8 x float> %sv0, float %s, i64 1
  %sv2 = insertelement <8 x float> %sv1, float %s, i64 2
  %sv3 = insertelement <8 x float> %sv2, float %s, i64 3
  %sv4 = insertelement <8 x float> %sv3, float %s, i64 4
  %sv5 = insertelement <8 x float> %sv4, float %s, i64 5
  %sv6 = insertelement <8 x float> %sv5, float %s, i64 6
  %sv7 = insertelement <8 x float> %sv6, float %s, i64 7
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %acc = phi <8 x float> [ %sv7, %entry ], [ %acc.next, %loop ]
  %acc.next = fadd <8 x float> %acc, <float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.000000e+00, float 5.000000e+00, float 6.000000e+00, float 7.000000e+00, float 8.000000e+00>
  %i.next = add i32 %i, 1
  %c = icmp slt i32 %i.next, %n
  br i1 %c, label %loop, label %exit

exit:
  %abc = bitcast <8 x float> %acc.next to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 -1, i32 -1, i32 -1, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %abc)
  ret void
}

declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)

attributes #0 = { convergent }

!igc.functions = !{!0, !3, !4}

!0 = !{ptr @splat_reused_payload, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @splat_reused_extract, !1}
!4 = !{ptr @splat_reused_phi, !1}

!5 = !{!"requiredSubGroupSize", i32 16}
!6 = !{!"FuncMDMap[0]", ptr @splat_reused_payload}
!7 = !{!"FuncMDValue[0]", !5}
!8 = !{!"FuncMDMap[1]", ptr @splat_reused_extract}
!9 = !{!"FuncMDValue[1]", !5, !14}
!10 = !{!"FuncMDMap[2]", ptr @splat_reused_phi}
!11 = !{!"FuncMDValue[2]", !5}
!12 = !{!"FuncMD", !6, !7, !8, !9, !10, !11}
!13 = !{!"ModuleMD", !12}
!IGCMetadata = !{!13}
!14 = !{!"resAllocMD", !15}
!15 = !{!"argAllocMDList", !19, !20, !21, !22}
!16 = !{!"type", i32 0}
!17 = !{!"extensionType", i32 -1}
!18 = !{!"indexType", i32 -1}
!19 = !{!"argAllocMDListVec[0]", !16, !17, !18}
!20 = !{!"argAllocMDListVec[1]", !16, !17, !18}
!21 = !{!"argAllocMDListVec[2]", !16, !17, !18}
!22 = !{!"argAllocMDListVec[3]", !16, !17, !18}
