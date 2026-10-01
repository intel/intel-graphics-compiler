;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers --igc-mem-access-throttle -platformCri -regkey MemAccessThrottle=1 -S < %s | FileCheck %s
;
; Opt-in only.
; RUN: igc_opt --opaque-pointers --igc-mem-access-throttle -platformCri -S < %s | FileCheck --check-prefix=OFF --implicit-check-not=eu.thread.pause --implicit-check-not=threadgroupbarrier %s
;
; Regkey force-off wins over the option.
; RUN: igc_opt --opaque-pointers --igc-mem-access-throttle -platformCri -regkey MemAccessThrottle=0 -S < %s | FileCheck --check-prefix=OFF --implicit-check-not=eu.thread.pause --implicit-check-not=threadgroupbarrier %s
;
; CRI only.
; RUN: igc_opt --opaque-pointers --igc-mem-access-throttle -platformbmg -regkey MemAccessThrottle=1 -S < %s | FileCheck --check-prefix=OFF --implicit-check-not=eu.thread.pause --implicit-check-not=threadgroupbarrier %s
; ------------------------------------------------
; MemAccessThrottle
; ------------------------------------------------

; OFF: define spir_kernel void @reduce_and_broadcast

; The leader predicate comes from R0.2[7:0] and is hoisted to the entry block.
define spir_kernel void @reduce_and_broadcast(ptr addrspace(1) %in1, ptr addrspace(1) %in2, ptr addrspace(1) %out1,
                                              ptr addrspace(1) %out2, <8 x i32> %r0) {
; CHECK-LABEL: @reduce_and_broadcast(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[LTID:%.*]] = extractelement <8 x i32> %r0, i32 2
; CHECK-NEXT:    [[MASKED:%.*]] = and i32 [[LTID]], 255
; CHECK-NEXT:    [[LEADER:%.*]] = icmp eq i32 [[MASKED]], 0
;
; CHECK:         store <4 x float> {{.*}}, ptr addrspace(1) %out2
; CHECK-NEXT:    br i1 [[LEADER]], label %[[THEN:.*]], label %[[CONT:.*]]
;
; CHECK:       [[THEN]]:
; CHECK-NEXT:    call void @llvm.genx.GenISA.eu.thread.pause(i32 32)
; CHECK-NEXT:    br label %[[CONT]]
;
; CHECK:       [[CONT]]:
; CHECK-NEXT:    call void @llvm.genx.GenISA.threadgroupbarrier()
entry:
  %a = load <4 x float>, ptr addrspace(1) %in1, align 16
  %b = load <4 x float>, ptr addrspace(1) %in2, align 16
  %sum = fadd <4 x float> %a, %b
  store <4 x float> %sum, ptr addrspace(1) %out1, align 16
  %rl = load <4 x float>, ptr addrspace(1) %out1, align 16
  store <4 x float> %rl, ptr addrspace(1) %out2, align 16
  ret void
}

; Stores sharing one uniformly executed region share a single throttle.
define spir_kernel void @two_patterns(ptr addrspace(1) %in1, ptr addrspace(1) %in2, ptr addrspace(1) %out1,
                                      ptr addrspace(1) %out2, ptr addrspace(1) %out3, ptr addrspace(1) %out4,
                                      <8 x i32> %r0) {
; CHECK-LABEL: @two_patterns(
; CHECK:         store <4 x float> {{.*}}, ptr addrspace(1) %out4
; CHECK-NEXT:    br i1 {{.*}}, label %[[T:.*]], label %[[C:.*]]
; CHECK:       [[T]]:
; CHECK-NEXT:    call void @llvm.genx.GenISA.eu.thread.pause(i32 32)
; CHECK-NEXT:    br label %[[C]]
; CHECK:       [[C]]:
; CHECK-NEXT:    call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-NOT:     call void @llvm.genx.GenISA.eu.thread.pause
entry:
  %a = load <4 x float>, ptr addrspace(1) %in1, align 16
  %b = load <4 x float>, ptr addrspace(1) %in2, align 16
  %sum = fadd <4 x float> %a, %b
  store <4 x float> %sum, ptr addrspace(1) %out1, align 16
  %rl1 = load <4 x float>, ptr addrspace(1) %out1, align 16
  store <4 x float> %rl1, ptr addrspace(1) %out2, align 16
  store <4 x float> %sum, ptr addrspace(1) %out3, align 16
  %rl2 = load <4 x float>, ptr addrspace(1) %out3, align 16
  store <4 x float> %rl2, ptr addrspace(1) %out4, align 16
  ret void
}

; No store/reload pair.
define spir_kernel void @no_reload(ptr addrspace(1) %in1, ptr addrspace(1) %in2, ptr addrspace(1) %out1,
                                   ptr addrspace(1) %out2, <8 x i32> %r0) {
; CHECK-LABEL: @no_reload(
; CHECK-NOT:     call void @llvm.genx.GenISA.eu.thread.pause
; CHECK-NOT:     call void @llvm.genx.GenISA.threadgroupbarrier
entry:
  %a = load <4 x float>, ptr addrspace(1) %in1, align 16
  %b = load <4 x float>, ptr addrspace(1) %in2, align 16
  %sum = fadd <4 x float> %a, %b
  store <4 x float> %sum, ptr addrspace(1) %out1, align 16
  store <4 x float> %sum, ptr addrspace(1) %out2, align 16
  ret void
}

; A staged copy with no arithmetic is throttled too.
define spir_kernel void @copy_then_broadcast(ptr addrspace(1) %in1, ptr addrspace(1) %out1, ptr addrspace(1) %out2,
                                             <8 x i32> %r0) {
; CHECK-LABEL: @copy_then_broadcast(
; CHECK:         store <4 x float> {{.*}}, ptr addrspace(1) %out2
; CHECK-NEXT:    br i1 {{.*}}, label %[[T2:.*]], label %[[C2:.*]]
; CHECK:       [[T2]]:
; CHECK-NEXT:    call void @llvm.genx.GenISA.eu.thread.pause(i32 32)
; CHECK:       [[C2]]:
; CHECK-NEXT:    call void @llvm.genx.GenISA.threadgroupbarrier()
entry:
  %a = load <4 x float>, ptr addrspace(1) %in1, align 16
  store <4 x float> %a, ptr addrspace(1) %out1, align 16
  %rl = load <4 x float>, ptr addrspace(1) %out1, align 16
  store <4 x float> %rl, ptr addrspace(1) %out2, align 16
  ret void
}

; Both stores share a base, so not a second buffer.
define spir_kernel void @self_copy(ptr addrspace(1) %in1, ptr addrspace(1) %in2, ptr addrspace(1) %out1,
                                   <8 x i32> %r0) {
; CHECK-LABEL: @self_copy(
; CHECK-NOT:     call void @llvm.genx.GenISA.eu.thread.pause
; CHECK-NOT:     call void @llvm.genx.GenISA.threadgroupbarrier
entry:
  %a = load <4 x float>, ptr addrspace(1) %in1, align 16
  %b = load <4 x float>, ptr addrspace(1) %in2, align 16
  %sum = fadd <4 x float> %a, %b
  store <4 x float> %sum, ptr addrspace(1) %out1, align 16
  %rl = load <4 x float>, ptr addrspace(1) %out1, align 16
  store <4 x float> %rl, ptr addrspace(1) %out1, align 16
  ret void
}

; Below the d32x4 minimum.
define spir_kernel void @too_small(ptr addrspace(1) %in1, ptr addrspace(1) %in2, ptr addrspace(1) %out1,
                                   ptr addrspace(1) %out2, <8 x i32> %r0) {
; CHECK-LABEL: @too_small(
; CHECK-NOT:     call void @llvm.genx.GenISA.eu.thread.pause
; CHECK-NOT:     call void @llvm.genx.GenISA.threadgroupbarrier
entry:
  %a = load float, ptr addrspace(1) %in1, align 4
  %b = load float, ptr addrspace(1) %in2, align 4
  %sum = fadd float %a, %b
  store float %sum, ptr addrspace(1) %out1, align 4
  %rl = load float, ptr addrspace(1) %out1, align 4
  store float %rl, ptr addrspace(1) %out2, align 4
  ret void
}

; SLM is out of scope.
define spir_kernel void @slm(ptr addrspace(3) %in1, ptr addrspace(3) %in2, ptr addrspace(3) %out1,
                             ptr addrspace(3) %out2, <8 x i32> %r0) {
; CHECK-LABEL: @slm(
; CHECK-NOT:     call void @llvm.genx.GenISA.eu.thread.pause
; CHECK-NOT:     call void @llvm.genx.GenISA.threadgroupbarrier
entry:
  %a = load <4 x float>, ptr addrspace(3) %in1, align 16
  %b = load <4 x float>, ptr addrspace(3) %in2, align 16
  %sum = fadd <4 x float> %a, %b
  store <4 x float> %sum, ptr addrspace(3) %out1, align 16
  %rl = load <4 x float>, ptr addrspace(3) %out1, align 16
  store <4 x float> %rl, ptr addrspace(3) %out2, align 16
  ret void
}

; The throttle is hoisted out of the bounds check to the join, which the whole
; work-group reaches. A barrier inside the guard would hang the last partial
; work-group.
define spir_kernel void @bounds_guarded(ptr addrspace(1) %in1, ptr addrspace(1) %in2, ptr addrspace(1) %out1,
                                        ptr addrspace(1) %out2, i32 %n, <8 x i32> %r0, i16 %localIdX) {
; CHECK-LABEL: @bounds_guarded(
; CHECK:       body:
; CHECK-NOT:     call void @llvm.genx.GenISA.threadgroupbarrier
; CHECK:         store <4 x float> {{.*}}, ptr addrspace(1) %out2
; CHECK-NEXT:    br label %done
; CHECK:       done:
; CHECK-NEXT:    br i1 {{.*}}, label %[[T:.*]], label %[[C:.*]]
; CHECK:       [[T]]:
; CHECK-NEXT:    call void @llvm.genx.GenISA.eu.thread.pause(i32 32)
; CHECK:       [[C]]:
; CHECK-NEXT:    call void @llvm.genx.GenISA.threadgroupbarrier()
entry:
  %lid = zext i16 %localIdX to i32
  %cmp = icmp slt i32 %lid, %n
  br i1 %cmp, label %body, label %done
body:
  %a = load <4 x float>, ptr addrspace(1) %in1, align 16
  %b = load <4 x float>, ptr addrspace(1) %in2, align 16
  %sum = fadd <4 x float> %a, %b
  store <4 x float> %sum, ptr addrspace(1) %out1, align 16
  %rl = load <4 x float>, ptr addrspace(1) %out1, align 16
  store <4 x float> %rl, ptr addrspace(1) %out2, align 16
  br label %done
done:
  ret void
}

; A thread-varying trip count leaves no safe point inside the loop.
define spir_kernel void @nonuniform_loop(ptr addrspace(1) %in1, ptr addrspace(1) %in2, ptr addrspace(1) %out1,
                                         ptr addrspace(1) %out2, <8 x i32> %r0, i16 %localIdX) {
; CHECK-LABEL: @nonuniform_loop(
; CHECK-NOT:     call void @llvm.genx.GenISA.eu.thread.pause
; CHECK-NOT:     call void @llvm.genx.GenISA.threadgroupbarrier
entry:
  %bound = zext i16 %localIdX to i32
  br label %loop
loop:
  %i = phi i32 [ 0, %entry ], [ %inext, %loop ]
  %a = load <4 x float>, ptr addrspace(1) %in1, align 16
  %b = load <4 x float>, ptr addrspace(1) %in2, align 16
  %sum = fadd <4 x float> %a, %b
  store <4 x float> %sum, ptr addrspace(1) %out1, align 16
  %rl = load <4 x float>, ptr addrspace(1) %out1, align 16
  store <4 x float> %rl, ptr addrspace(1) %out2, align 16
  %inext = add i32 %i, 1
  %c = icmp slt i32 %inext, %bound
  br i1 %c, label %loop, label %exit
exit:
  ret void
}

; Two staged copies in two branches share the throttle at reconvergence, as in
; the oneCCL reduce_copy vector and tail paths.
define spir_kernel void @two_branches(ptr addrspace(1) %in1, ptr addrspace(1) %in2, ptr addrspace(1) %o1,
                                      ptr addrspace(1) %o2, ptr addrspace(1) %t1, ptr addrspace(1) %t2,
                                      i32 %packed_count, i32 %count, <8 x i32> %r0, i16 %localIdX) {
; CHECK-LABEL: @two_branches(
; CHECK:       vec:
; CHECK-NOT:     call void @llvm.genx.GenISA.threadgroupbarrier
; CHECK:       tail:
; CHECK-NOT:     call void @llvm.genx.GenISA.threadgroupbarrier
; CHECK:       done:
; CHECK-NEXT:    br i1 {{.*}}, label %[[T:.*]], label %[[C:.*]]
; CHECK:       [[T]]:
; CHECK-NEXT:    call void @llvm.genx.GenISA.eu.thread.pause(i32 32)
; CHECK:       [[C]]:
; CHECK-NEXT:    call void @llvm.genx.GenISA.threadgroupbarrier()
; CHECK-NOT:     call void @llvm.genx.GenISA.eu.thread.pause
entry:
  %idx = zext i16 %localIdX to i32
  %c1 = icmp slt i32 %idx, %packed_count
  br i1 %c1, label %vec, label %chk2
vec:
  %a = load <4 x float>, ptr addrspace(1) %in1, align 16
  %b = load <4 x float>, ptr addrspace(1) %in2, align 16
  %sum = fadd <4 x float> %a, %b
  store <4 x float> %sum, ptr addrspace(1) %o1, align 16
  %rl1 = load <4 x float>, ptr addrspace(1) %o1, align 16
  store <4 x float> %rl1, ptr addrspace(1) %o2, align 16
  br label %done
chk2:
  %c2 = icmp slt i32 %idx, %count
  br i1 %c2, label %tail, label %done
tail:
  %c = load <4 x float>, ptr addrspace(1) %in1, align 16
  %d = load <4 x float>, ptr addrspace(1) %in2, align 16
  %sum2 = fadd <4 x float> %c, %d
  store <4 x float> %sum2, ptr addrspace(1) %t1, align 16
  %rl2 = load <4 x float>, ptr addrspace(1) %t1, align 16
  store <4 x float> %rl2, ptr addrspace(1) %t2, align 16
  br label %done
done:
  ret void
}

!igc.functions = !{!0, !3, !4, !5, !6, !7, !8, !9, !40, !51}
!IGCMetadata = !{!20}

!0 = !{ptr @reduce_and_broadcast, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @no_reload, !1}
!4 = !{ptr @copy_then_broadcast, !1}
!5 = !{ptr @too_small, !1}
!6 = !{ptr @slm, !1}
!7 = !{ptr @self_copy, !1}
!8 = !{ptr @two_patterns, !1}
!9 = !{ptr @bounds_guarded, !1}
!40 = !{ptr @nonuniform_loop, !1}
!51 = !{ptr @two_branches, !1}

!10 = !{!"argId", i32 0}
!11 = !{!"implicitArgInfoListVec[0]", !10}
!12 = !{!"implicitArgInfoList", !11}
; kernels that also take an implicit local id
!41 = !{!"argId", i32 8}
!42 = !{!"implicitArgInfoListVec[1]", !41}
!43 = !{!"implicitArgInfoList", !11, !42}

!20 = !{!"ModuleMD", !21}
!21 = !{!"FuncMD", !22, !23, !24, !25, !26, !27, !28, !29, !30, !31, !32, !33, !34, !35, !36, !37, !38, !39, !54, !55}
!22 = !{!"FuncMDMap[0]", ptr @reduce_and_broadcast}
!23 = !{!"FuncMDValue[0]", !12}
!24 = !{!"FuncMDMap[1]", ptr @no_reload}
!25 = !{!"FuncMDValue[1]", !12}
!26 = !{!"FuncMDMap[2]", ptr @copy_then_broadcast}
!27 = !{!"FuncMDValue[2]", !12}
!28 = !{!"FuncMDMap[3]", ptr @too_small}
!29 = !{!"FuncMDValue[3]", !12}
!30 = !{!"FuncMDMap[4]", ptr @slm}
!31 = !{!"FuncMDValue[4]", !12}
!32 = !{!"FuncMDMap[5]", ptr @self_copy}
!33 = !{!"FuncMDValue[5]", !12}
!34 = !{!"FuncMDMap[6]", ptr @two_patterns}
!35 = !{!"FuncMDValue[6]", !12}
!36 = !{!"FuncMDMap[7]", ptr @bounds_guarded}
!37 = !{!"FuncMDValue[7]", !43}
!38 = !{!"FuncMDMap[8]", ptr @nonuniform_loop}
!39 = !{!"FuncMDValue[8]", !43}
!54 = !{!"FuncMDMap[9]", ptr @two_branches}
!55 = !{!"FuncMDValue[9]", !43}
