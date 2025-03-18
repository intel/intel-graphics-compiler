;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers --regkey EnablePrivMemNewSOATranspose=0 --igc-private-mem-resolution -S < %s 2>&1 | FileCheck %s

; reduced from the following:
; int a[1000];
; int* b = &a[0] - 1;
; for (size_t i = 1; i != 1001; i++)
;   b[i] = 0;

define spir_kernel void @test_pmem(i32 addrspace(1)* %dst, i32 addrspace(1)* %src, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1) #0 {
entry:
  %arr = alloca [1000 x i32], align 16
  %invariant.gep = getelementptr [1000 x i32], [1000 x i32]* %arr, i64 0, i64 -1
  br label %loop

loop:
  %iter = phi i64 [ 1, %entry ], [ %incr, %loop ]
; add nuw %x, -1 gets optimized to just -1, because -1 == 0xFFFFFFFF,
; so if add is tagged nuw, %x MUST be 0, so as not to cause an overflow.
; That's a problem because in this case it causes us to lose the address.
; CHECK-NOT: add nuw i32 %{{.*}}, -1
; CHECK: add i32 %{{.*}}, -1
  %gep = getelementptr i32, i32* %invariant.gep, i64 %iter
  store i32 zeroinitializer, i32* %gep , align 32
  %incr = add nuw nsw i64 %iter, 1
  %exitcond = icmp eq i64 %incr, 1001
  br i1 %exitcond, label %exit, label %loop

exit:
  ret void
}

attributes #0 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" }

!IGCMetadata = !{!3}
!igc.functions = !{!17}

!3 = !{!"ModuleMD", !4, !7, !13, !10}
!4 = !{!"compOpt", !5, !6}
!5 = !{!"OptDisable", i1 false}
!6 = !{!"UseScratchSpacePrivateMemory", i1 true}
!7 = !{!"FuncMD", !11, !12}
!10 = !{!"privateMemoryPerWI", i32 0}
!11 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8*, i32, i32)* @test_pmem}
!12 = !{!"FuncMDValue[0]", !10}
!13 = !{!"MinNOSPushConstantSize", i32 0}
!17 = !{void (i32 addrspace(1)*, i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8*, i32, i32)* @test_pmem, !18}
!18 = !{!19, !20}
!19 = !{!"function_type", i32 0}
!20 = !{!"implicit_arg_desc", !21, !22, !23, !24, !26}
!21 = !{i32 0}
!22 = !{i32 1}
!23 = !{i32 13}
!24 = !{i32 15, !25}
!25 = !{!"explicit_arg_num", i32 0}
!26 = !{i32 15, !27}
!27 = !{!"explicit_arg_num", i32 1}

