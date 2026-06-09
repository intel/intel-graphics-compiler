;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --opaque-pointers -platformbmg -igc-emit-visa %s -regkey DumpVISAASMToConsole | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; This test checks vISA emitter correctly processes structure passed as kernel argument.

; CHECK-LABEL: .kernel "struct_in_arg_test"
; CHECK: .decl p v_type=G type=b num_elts=8 align=dword
; CHECK: .decl p_0v v_type=G type=f num_elts=2 align=dword alias=<p, 0>
; CHECK: .decl p_1v v_type=G type=d num_elts=2 align=dword alias=<p, 0>
; CHECK: mov (M1_NM, 1) {{.*}}(0,0)<1> p_0v(0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) {{.*}}(0,0)<1> p_1v(0,1)<0;1,0>


%PT = type { float, i32 }

define spir_kernel void @struct_in_arg_test(%PT %p, ptr align 4 %seed_ptr, ptr %out_ptr, <8 x i32> %r0) {
entry:
  %0 = extractvalue %PT %p, 0
  %1 = extractvalue %PT %p, 1
  %py_f = sitofp i32 %1 to float
  %seed = load float, ptr %seed_ptr, align 4
  %dx = fmul float %seed, %py_f
  %dy = fmul float %0, %seed
  %2 = insertelement <2 x float> undef, float %dx, i64 0
  %3 = insertelement <2 x float> %2, float %dy, i64 1
  store <2 x float> %3, ptr %out_ptr, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!14}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", ptr @struct_in_arg_test}
!3 = !{!"FuncMDValue[0]", !4, !27}
!4 = !{!"resAllocMD", !5}
!5 = distinct !{!"argAllocMDList", !6, !10, !11, !5, !6}
!6 = !{!"argAllocMDListVec[0]", !7, !8, !9}
!7 = !{!"type", i32 0}
!8 = !{!"extensionType", i32 -1}
!9 = !{!"indexType", i32 -1}
!10 = !{!"argAllocMDListVec[1]", !7, !8, !9}
!11 = !{!"argAllocMDListVec[2]", !12, !8, !13}
!12 = !{!"type", i32 1}
!13 = !{!"indexType", i32 0}
!14 = !{ptr @struct_in_arg_test, !15}
!15 = !{!16}
!16 = !{!"function_type", i32 0}
!21 = !{!"argId", i32 0}
!22 = !{!"implicitArgInfoListVec[0]", !21}
!23 = !{!"argId", i32 2}
!24 = !{!"implicitArgInfoListVec[1]", !23}
!25 = !{!"argId", i32 13}
!26 = !{!"implicitArgInfoListVec[2]", !25}
!27 = !{!"implicitArgInfoList", !22, !24, !26}
