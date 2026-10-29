;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers --igc-private-mem-resolution -S %s | FileCheck --check-prefixes=CHECK,CHECK-OPAQUE %s %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers --igc-private-mem-resolution -S %s | FileCheck --check-prefixes=CHECK,CHECK-TYPED %s %}
; ------------------------------------------------
; PrivateMemoryResolution
; ------------------------------------------------

; Checks if allocas uses that are in unreachable basic blocks are
; skipped, when looking for common dominator.

define spir_kernel void @test_skip( i32 %src, i32 %bufferOffset, i32 %bufferOffset1) #0 {
; CHECK-LABEL: @test_skip(
; CHECK:  entry:
; CHECK-TYPED:    [[PRIVATEBUFFER:%.*]] = bitcast i8* {{%.*}} to i32*
; CHECK-OPAQUE:    [[PRIVATEBUFFER:%.*]] = getelementptr i8, ptr {{%.*}}, i64 {{%.*}}
; CHECK-TYPED:    store i32 %src, i32* [[PRIVATEBUFFER]], align 4
; CHECK-OPAQUE:    store i32 %src, ptr [[PRIVATEBUFFER]], align 4
; CHECK-TYPED:    [[LOAD1:%.*]] = load i32, i32* [[PRIVATEBUFFER]], align 4
; CHECK-OPAQUE:    [[LOAD1:%.*]] = load i32, ptr [[PRIVATEBUFFER]], align 4
entry:
  %0 = alloca i32, align 4
  store i32 %src, i32* %0, align 4
  %1 = load i32, i32* %0, align 4
  br label %end

unused:                         ; No predecessors!
; CHECK:  unused:
; CHECK-TYPED:    store i32 %src, i32* [[PRIVATEBUFFER]], align 4
; CHECK-OPAQUE:    store i32 %src, ptr [[PRIVATEBUFFER]], align 4
; CHECK-TYPED:    [[LOAD2:%.*]] = load i32, i32* [[PRIVATEBUFFER]], align 4
; CHECK-OPAQUE:    [[LOAD2:%.*]] = load i32, ptr [[PRIVATEBUFFER]], align 4
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
!12 = !{!"FuncMDValue[0]", !10, !34}
!13 = !{!"MinNOSPushConstantSize", i32 0}
!17 = !{void (i32, i32, i32)* @test_skip, !18}
!18 = !{!19}
!19 = !{!"function_type", i32 1}
!28 = !{!"argId", i32 14}
!29 = !{!"explicitArgNum", i32 0}
!30 = !{!"implicitArgInfoListVec[0]", !28, !29}
!31 = !{!"argId", i32 14}
!32 = !{!"explicitArgNum", i32 1}
!33 = !{!"implicitArgInfoListVec[1]", !31, !32}
!34 = !{!"implicitArgInfoList", !30, !33}
