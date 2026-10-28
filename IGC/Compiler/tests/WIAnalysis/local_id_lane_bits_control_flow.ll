;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: system-windows
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --igc-pressure-printer -S --disable-output --regkey=RegPressureVerbocity=1 < %s 2>&1 | FileCheck %s

; A uniform instruction reads its nonuniform operands from the first lane of the thread. The lane
; bits are only used when the first lane is active both where the operands are computed and where
; the instruction is: in blocks that no divergent branch controls. Work-group 256x1x1, SIMD16.

; The first lane is inactive in %then in the thread of work-items 0-15, so nothing there that
; reads a nonuniform operand is uniform, even %te computed in the entry block. In %exit all the
; lanes are active again.
; CHECK-LABEL: block: then function: divergentBranch
; CHECK: N: {{.*}} %r = and i32 %t, 224
; CHECK: N: {{.*}} %re = and i32 %te, 224
; CHECK: N: {{.*}} %ih = lshr i16 %ix, 4
; CHECK-LABEL: block: nested function: divergentBranch
; CHECK: N: {{.*}} %nestedHigh = and i32 %t, 224
; CHECK: N: {{.*}} %entryHigh = and i32 %te, 224
; CHECK-LABEL: block: innerJoin function: divergentBranch
; CHECK: N: {{.*}} %joinHigh = and i32 %t, 224
; CHECK-LABEL: block: exit function: divergentBranch
; CHECK: U: {{.*}} %exitHigh = and i32 %te, 224
define spir_kernel void @divergentBranch(ptr addrspace(1) %out, i32 %u, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
entry:
  %lx = zext i16 %localIdX to i32
  %te = shl i32 %lx, 1
  br label %guard

then:
  %t = shl i32 %lx, 1
  %r = and i32 %t, 224
  %re = and i32 %te, 224
  %ix = call i16 @llvm.genx.GenISA.getLocalID.X()
  %ih = lshr i16 %ix, 4
  %ihz = zext i16 %ih to i32
  %sum = add i32 %r, %re
  %sum2 = add i32 %sum, %ihz
  %dst = getelementptr i32, ptr addrspace(1) %out, i32 %lx
  store i32 %sum2, ptr addrspace(1) %dst
  %enabled = icmp ne i32 %u, 0
  br i1 %enabled, label %nested, label %innerJoin

nested:
  %nestedHigh = and i32 %t, 224
  %entryHigh = and i32 %te, 224
  %nestedSum = add i32 %nestedHigh, %entryHigh
  store volatile i32 %nestedSum, ptr addrspace(1) %dst
  br label %innerJoin

innerJoin:
  %joinHigh = and i32 %t, 224
  store i32 %joinHigh, ptr addrspace(1) %dst
  br label %exit

exit:
  %exitHigh = and i32 %te, 224
  store i32 %exitHigh, ptr addrspace(1) %out
  ret void

guard:
  %c = icmp ugt i32 %lx, 5
  br i1 %c, label %then, label %exit
}

; DeSSA coalesces %lx, %q and %p. %else runs first and writes %q to the first lane, which is
; inactive in %then, so a uniform %hi would read %q instead of %lx.
; CHECK-LABEL: block: then function: phiClobber
; CHECK: N: {{.*}} %hi = lshr i32 %lx, 4
define spir_kernel void @phiClobber(ptr addrspace(1) %out, i32 %u, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
entry:
  %lx = zext i16 %localIdX to i32
  %low = and i32 %lx, 15
  %c = icmp ugt i32 %low, 4
  br i1 %c, label %then, label %else

else:
  %q = mul i32 %lx, 3
  br label %join

then:
  %hi = lshr i32 %lx, 4
  %dst = getelementptr i32, ptr addrspace(1) %out, i32 %lx
  store i32 %hi, ptr addrspace(1) %dst
  br label %join

join:
  %p = phi i32 [ %q, %else ], [ %lx, %then ]
  store i32 %p, ptr addrspace(1) %out
  ret void
}

; The lanes leave the loop at different times, so nothing in it that reads a nonuniform operand is
; uniform. After the loop all the lanes are active again, but %t was computed in the loop.
; CHECK-LABEL: block: loop function: loop
; CHECK: N: {{.*}} %r = and i32 %t, 224
; CHECK: N: {{.*}} %re = and i32 %te, 224
; CHECK-LABEL: block: exit function: loop
; CHECK: U: {{.*}} %xe = and i32 %te, 224
; CHECK: N: {{.*}} %xt = and i32 %t, 224
define spir_kernel void @loop(ptr addrspace(1) %out, i32 %u, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
entry:
  %lx = zext i16 %localIdX to i32
  %te = shl i32 %lx, 1
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %inc, %loop ]
  %t = shl i32 %lx, 1
  %r = and i32 %t, 224
  %re = and i32 %te, 224
  %sum = add i32 %r, %re
  %dst = getelementptr i32, ptr addrspace(1) %out, i32 %i
  store i32 %sum, ptr addrspace(1) %dst
  %inc = add i32 %i, 1
  %more = icmp ult i32 %inc, %lx
  br i1 %more, label %loop, label %exit

exit:
  %xe = and i32 %te, 224
  %xt = and i32 %t, 224
  %xs = add i32 %xe, %xt
  store i32 %xs, ptr addrspace(1) %out
  ret void
}

; A uniform branch and a uniform loop keep all the lanes together, so %t and %tl are computed by
; the first lane.
; CHECK-LABEL: block: uloop function: uniformControlFlow
; CHECK: U: {{.*}} %r = and i32 %t, 224
; CHECK: U: {{.*}} %rl = and i32 %tl, 120
define spir_kernel void @uniformControlFlow(ptr addrspace(1) %out, i32 %u, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
entry:
  %c = icmp sgt i32 %u, 0
  br i1 %c, label %then, label %exit

then:
  %lx = zext i16 %localIdX to i32
  %t = shl i32 %lx, 1
  br label %uloop

uloop:
  %i = phi i32 [ 0, %then ], [ %inc, %uloop ]
  %tl = lshr i32 %lx, 1
  %r = and i32 %t, 224
  %rl = and i32 %tl, 120
  %sum = add i32 %r, %rl
  %dst = getelementptr i32, ptr addrspace(1) %out, i32 %i
  store i32 %sum, ptr addrspace(1) %dst
  %inc = add i32 %i, 1
  %more = icmp ult i32 %inc, %u
  br i1 %more, label %uloop, label %exit

exit:
  ret void
}

declare i16 @llvm.genx.GenISA.getLocalID.X()

; The loop branch becomes divergent only after %p does, which takes several rounds of the fixed
; point. %rb was already decided by then, so it must be rechecked when its block is found divergent.
; CHECK-LABEL: block: body function: lateDivergence
; CHECK: N: {{.*}} %rb = and i32 %te, 224
define spir_kernel void @lateDivergence(ptr addrspace(1) %out, i32 %u, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
entry:
  %lx = zext i16 %localIdX to i32
  %te = shl i32 %lx, 1
  br label %loop

loop:
  %p = phi i32 [ 0, %entry ], [ %q, %body ]
  %c = icmp ult i32 %p, %u
  br i1 %c, label %body, label %exit

body:
  %rb = and i32 %te, 224
  %dst = getelementptr i32, ptr addrspace(1) %out, i32 %p
  store i32 %rb, ptr addrspace(1) %dst
  %q = add i32 %p, %lx
  br label %loop

exit:
  ret void
}

; The loop branch turns divergent only after %p does. %xb, after the loop, was already decided
; by then; its operand %tb is in the loop header, so it must be rechecked as a user.
; CHECK-LABEL: block: exit function: operandInLateDivergentLoop
; CHECK: N: {{.*}} %xb = and i32 %tb, 224
define spir_kernel void @operandInLateDivergentLoop(ptr addrspace(1) %out, i32 %u, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
entry:
  %lx = zext i16 %localIdX to i32
  br label %loop
loop:
  %p = phi i32 [ 0, %entry ], [ %q, %body ]
  %tb = shl i32 %lx, 1
  %c = icmp ult i32 %p, %u
  br i1 %c, label %body, label %exit
body:
  %dst = getelementptr i32, ptr addrspace(1) %out, i32 %p
  store i32 %p, ptr addrspace(1) %dst
  %q = add i32 %p, %lx
  br label %loop
exit:
  %xb = and i32 %tb, 224
  store i32 %xb, ptr addrspace(1) %out
  ret void
}

!igc.functions = !{!0, !1, !2, !3, !4, !5}
!0 = !{ptr @divergentBranch, !926}
!1 = !{ptr @loop, !926}
!2 = !{ptr @uniformControlFlow, !926}
!3 = !{ptr @phiClobber, !926}
!4 = !{ptr @lateDivergence, !926}
!5 = !{ptr @operandInLateDivergentLoop, !926}
!IGCMetadata = !{!100}
!100 = !{!"ModuleMD", !101}
!101 = !{!"FuncMD", !200, !201, !210, !211, !220, !221, !230, !231, !240, !241, !250, !251}
!200 = !{!"FuncMDMap[0]", ptr @divergentBranch}
!201 = !{!"FuncMDValue[0]", !920, !202, !924, !917}
!202 = !{!"threadGroupSize", !203, !204, !205}
!203 = !{!"dim0", i32 256}
!204 = !{!"dim1", i32 1}
!205 = !{!"dim2", i32 1}
!210 = !{!"FuncMDMap[1]", ptr @loop}
!211 = !{!"FuncMDValue[1]", !920, !212, !924, !917}
!212 = !{!"threadGroupSize", !213, !214, !215}
!213 = !{!"dim0", i32 256}
!214 = !{!"dim1", i32 1}
!215 = !{!"dim2", i32 1}
!220 = !{!"FuncMDMap[2]", ptr @uniformControlFlow}
!221 = !{!"FuncMDValue[2]", !920, !222, !924, !917}
!222 = !{!"threadGroupSize", !223, !224, !225}
!223 = !{!"dim0", i32 256}
!224 = !{!"dim1", i32 1}
!225 = !{!"dim2", i32 1}
!230 = !{!"FuncMDMap[3]", ptr @phiClobber}
!231 = !{!"FuncMDValue[3]", !920, !232, !924, !917}
!232 = !{!"threadGroupSize", !233, !234, !235}
!233 = !{!"dim0", i32 256}
!234 = !{!"dim1", i32 1}
!235 = !{!"dim2", i32 1}
!240 = !{!"FuncMDMap[4]", ptr @lateDivergence}
!241 = !{!"FuncMDValue[4]", !920, !242, !924, !917}
!242 = !{!"threadGroupSize", !243, !244, !245}
!243 = !{!"dim0", i32 256}
!244 = !{!"dim1", i32 1}
!245 = !{!"dim2", i32 1}
!250 = !{!"FuncMDMap[5]", ptr @operandInLateDivergentLoop}
!251 = !{!"FuncMDValue[5]", !920, !252, !924, !917}
!252 = !{!"threadGroupSize", !253, !254, !255}
!253 = !{!"dim0", i32 256}
!254 = !{!"dim1", i32 1}
!255 = !{!"dim2", i32 1}
!900 = !{!"argId", i32 0}
!901 = !{!"implicitArgInfoListVec[0]", !900}
!902 = !{!"argId", i32 1}
!903 = !{!"implicitArgInfoListVec[1]", !902}
!904 = !{!"argId", i32 6}
!905 = !{!"implicitArgInfoListVec[2]", !904}
!906 = !{!"argId", i32 8}
!907 = !{!"implicitArgInfoListVec[3]", !906}
!908 = !{!"argId", i32 9}
!909 = !{!"implicitArgInfoListVec[4]", !908}
!910 = !{!"argId", i32 10}
!911 = !{!"implicitArgInfoListVec[5]", !910}
!912 = !{!"argId", i32 15}
!913 = !{!"explicitArgNum", i32 0}
!914 = !{!"implicitArgInfoListVec[6]", !912, !913}
!917 = !{!"implicitArgInfoList", !901, !903, !905, !907, !909, !911, !914}
!920 = !{!"workGroupWalkOrder", !921, !922, !923}
!921 = !{!"dim0", i32 0}
!922 = !{!"dim1", i32 1}
!923 = !{!"dim2", i32 2}
!924 = !{!"requiredSubGroupSize", i32 16}
!925 = !{!"function_type", i32 0}
!926 = !{!925}
