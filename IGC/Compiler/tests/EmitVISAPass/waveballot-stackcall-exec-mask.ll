;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt %s \
; RUN:     --opaque-pointers \
; RUN:     -GenXCodeGenModule \
; RUN:     -igc-emit-visa \
; RUN:     -platformbmg \
; RUN:     -simd-mode 16 \
; RUN:     -regkey DumpVISAASMToConsole \
; RUN:   | FileCheck %s

; A kernel may use the dispatch mask, while a called function must use its
; current execution mask because only some lanes may enter it.

; Kernel uses the whole dispatch mask.
; CHECK-LABEL:  .kernel "test"
; CHECK:        setp (M1_NM, 16) [[KFLAG:P[0-9]+]] 0xffffffff:ud
; CHECK-NEXT:   mov (M1_NM, 1) [[KB:[A-Za-z0-9_]+]](0,0)<1> [[KFLAG]]
; CHECK-NEXT:   and (M1_NM, 1) [[KB]](0,0)<1> %sr0(0,2)<0;1,0> [[KB]](0,0)<0;1,0>

; Global function uses the execution mask, not the whole dispatch mask.
; CHECK-LABEL:  .global_function "stack_callee"
; CHECK:        setp (M1_NM, 16) [[CFLAG:P[0-9]+]] 0x0:ud
; CHECK-NEXT:   cmp.eq (M1, 16) [[CFLAG]] [[CTMP:[A-Za-z0-9_]+]](0,0)<0;1,0> [[CTMP]](0,0)<0;1,0>
; CHECK-NEXT:   mov (M1_NM, 1) {{[A-Za-z0-9_]+}}(0,0)<1> [[CFLAG]]
; CHECK-NOT:    %sr0(0,2)

; Function uses the execution mask, not the whole dispatch mask.
; CHECK-LABEL:  .function "subroutine_callee{{.*}}"
; CHECK:        setp (M1_NM, 16) [[SFLAG:P[0-9]+]] 0x0:ud
; CHECK-NEXT:   cmp.eq (M1, 16) [[SFLAG]] [[STMP:[A-Za-z0-9_]+]](0,0)<0;1,0> [[STMP]](0,0)<0;1,0>
; CHECK-NEXT:   mov (M1_NM, 1) {{[A-Za-z0-9_]+}}(0,0)<1> [[SFLAG]]
; CHECK-NOT:    %sr0(0,2)

define spir_kernel void @test(i32 %s1, <8 x i32> %r0, <8 x i32> %payloadHeader, i32 %bufferOffset) {
entry:
  %kb = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
  store i32 %kb, ptr addrspace(1) null, align 4
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %half = icmp ult i16 %lane, 8
  br i1 %half, label %invoke, label %exit

invoke:
  call spir_func void @stack_callee()
  br label %exit

exit:
  ret void
}

define internal spir_func void @stack_callee() "visaStackCall" {
entry:
  %b = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
  store i32 %b, ptr addrspace(1) null, align 4
  call spir_func void @subroutine_callee()
  ret void
}

define internal spir_func void @subroutine_callee() {
entry:
  %b = call i32 @llvm.genx.GenISA.WaveBallot(i1 true, i32 0)
  store i32 %b, ptr addrspace(1) null, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.WaveBallot(i1, i32)
declare i16 @llvm.genx.GenISA.simdLaneId()

!igc.functions = !{!0, !1, !2}
!0 = !{ptr @test, !3}
!1 = !{ptr @stack_callee, !4}
!2 = !{ptr @subroutine_callee, !4}
!3 = !{!{!"function_type", i32 0}}
!4 = !{!{!"function_type", i32 2}}

!IGCMetadata = !{!5}
!5 = !{!"ModuleMD", !6}
!6 = !{!"FuncMD", !7, !8}
!7 = !{!"FuncMDMap[0]", ptr @test}
!8 = !{!"FuncMDValue[0]", !9}
!9 = !{!"implicitArgInfoList", !10, !11, !12, !13}
!10 = !{!"implicitArgInfoListVec[0]", !{!"argId", i32 0}}
!11 = !{!"implicitArgInfoListVec[1]", !{!"argId", i32 1}}
!12 = !{!"implicitArgInfoListVec[2]", !{!"argId", i32 13}}
!13 = !{!"implicitArgInfoListVec[3]", !{!"argId", i32 15}, !{!"explicitArgNum", i32 2}}
