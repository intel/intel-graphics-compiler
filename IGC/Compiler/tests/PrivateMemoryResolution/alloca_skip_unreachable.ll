;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-private-mem-resolution -S %s | FileCheck %s
; ------------------------------------------------
; PrivateMemoryResolution
; ------------------------------------------------

; Checks if allocas uses that are in unreachable basic blocks are
; skipped, when looking for common dominator.

define spir_kernel void @test_skip( i32 %src, i32 %bufferOffset, i32 %bufferOffset1) #0 {
; CHECK-LABEL: @test_skip(
; CHECK:  entry:
; CHECK:    [[PRIVATEBUFFER:%.*]] = bitcast i8* {{%.*}} to i32*
; CHECK:    store i32 %src, i32* [[PRIVATEBUFFER]], align 4
; CHECK:    [[LOAD1:%.*]] = load i32, i32* [[PRIVATEBUFFER]], align 4
entry:
  %0 = alloca i32, align 4
  store i32 %src, i32* %0, align 4
  %1 = load i32, i32* %0, align 4
  br label %end

unused:                         ; No predecessors!
; CHECK:  unused:
; CHECK:    store i32 %src, i32* [[PRIVATEBUFFER]], align 4
; CHECK:    [[LOAD2:%.*]] = load i32, i32* [[PRIVATEBUFFER]], align 4
  store i32 %src, i32* %0, align 4
  %2 = load i32, i32* %0, align 4
  unreachable

end:
  ret void
}

attributes #0 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" }

!IGCMetadata = !{!3}
!igc.functions = !{!17}

!3 = !{!"ModuleMD", !4, !7, !13, !10}
!4 = !{!"compOpt", !5, !6}
!5 = !{!"OptDisable", i1 true}
!6 = !{!"UseScratchSpacePrivateMemory", i1 true}
!7 = !{!"FuncMD", !11, !12}
!10 = !{!"privateMemoryPerWI", i32 0}
!11 = !{!"FuncMDMap[0]", void (i32, i32, i32)* @test_skip}
!12 = !{!"FuncMDValue[0]", !10}
!13 = !{!"MinNOSPushConstantSize", i32 0}
!17 = !{void (i32, i32, i32)* @test_skip, !18}
!18 = !{!19, !20}
!19 = !{!"function_type", i32 1}
!20 = !{!"implicit_arg_desc", !24, !26}
!21 = !{i32 0}
!22 = !{i32 1}
!23 = !{i32 12}
!24 = !{i32 14, !25}
!25 = !{!"explicit_arg_num", i32 0}
!26 = !{i32 14, !27}
!27 = !{!"explicit_arg_num", i32 1}
