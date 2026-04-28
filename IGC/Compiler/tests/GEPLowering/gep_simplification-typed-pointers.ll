;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; ------------------------------------------------
;
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; RUN: igc_opt --typed-pointers -platformpvc --igc-gep-lowering -regkey=EnableGEPSimplification=1,TestGEPSimplification=1 -S %s  | FileCheck %s
; ------------------------------------------------
; GEPLowering/GEP simplification : testing GEP strength reduction
; ------------------------------------------------

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

;
; CHECK-LABEL: define spir_kernel void @test_gep
;

; Function Attrs: convergent nounwind
define spir_kernel void @test_gep(i32 addrspace(1)* %dst, i32 addrspace(1)* %src, i64 %inc0, i64 %inc1, i64 %Offset64, i32 %Offset32) #0 {
;
; case 1 : gep's id is sext/zext
;
; CHECK-LABEL: case1:
; CHECK: [[C1T0:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* %src, i64 %{{.*}}
; CHECK: {{.*}} = getelementptr inbounds i32, i32 addrspace(1)* [[C1T0]], i64 4
; CHECK: {{.*}} = getelementptr inbounds i32, i32 addrspace(1)* [[C1T0]], i64 8
; CHECK: {{.*}} = getelementptr inbounds i32, i32 addrspace(1)* [[C1T0]], i64 12
;
case1:
  %simdLaneId16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %simdLaneId = zext i16 %simdLaneId16 to i32
  %idbase1 = add nsw i32 %Offset32, %simdLaneId
  %id1.1 = zext i32 %idbase1 to i64
  %addr1.1 = getelementptr inbounds i32, i32 addrspace(1)* %src, i64 %id1.1
  %res1.0 = load i32, i32 addrspace(1)* %addr1.1, align 4
  %add11.1 = add nsw i32 %idbase1, 4
  %id1.2 = zext i32 %add11.1 to i64
  %addr1.2 = getelementptr inbounds i32, i32 addrspace(1)* %src, i64 %id1.2
  %res1.1 = load i32, i32 addrspace(1)* %addr1.2, align 4
  %sum1.0 = add nsw i32 %res1.0, %res1.1
  %add11.2 = add nsw i32 %idbase1, 8
  %id1.3 = zext i32 %add11.2 to i64
  %addr1.3 = getelementptr inbounds i32, i32 addrspace(1)* %src, i64 %id1.3
  %res1.2 = load i32, i32 addrspace(1)* %addr1.3, align 4
  %sum1.1 = add nsw i32 %sum1.0, %res1.2
  %add11.3 = add nsw i32 %idbase1, 12
  %id1.4 = zext i32 %add11.3 to i64
  %addr1.4 = getelementptr inbounds i32, i32 addrspace(1)* %src, i64 %id1.4
  %res1.3 = load i32, i32 addrspace(1)* %addr1.4, align 4
  %sum1.2 = add nsw i32 %sum1.1, %res1.3
  %id1.5 = sext i32 %idbase1 to i64
  %addr1.5 = getelementptr inbounds i32, i32 addrspace(1)* %dst, i64 %id1.5
  store i32 %sum1.2, i32 addrspace(1)* %addr1.5, align 4
  br label %case2

;
; case 2 : gep's id is add/sub
;
; CHECK-LABEL: case2:
; CHECK: [[C2T0:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* %src, i64 %{{.*}}
; CHECK: [[C2T1:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* [[C2T0]], i64 10
; CHECK: [[C2T2:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* [[C2T0]], i64 20
; CHECK: [[C2T3:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* [[C2T0]], i64 30
;
case2:
  %simdLaneId64 = zext i16 %simdLaneId16 to i64
  %idbase2 = add i64 %Offset64, %simdLaneId64
  %addr2 = getelementptr inbounds i32, i32 addrspace(1)* %src, i64 %idbase2
  %res2.0 = load i32, i32 addrspace(1)* %addr2, align 4
  %id2.1 = add nsw i64 %idbase2, 10
  %addr2.1 = getelementptr inbounds i32, i32 addrspace(1)* %src, i64 %id2.1
  %res2.1 = load i32, i32 addrspace(1)* %addr2.1, align 4
  %sum2.0 = add nsw i32 %res2.0, %res2.1
  %id2.2 = add nsw i64 %idbase2, 20
  %addr2.2 = getelementptr inbounds i32, i32 addrspace(1)* %src, i64 %id2.2
  %res2.2 = load i32, i32 addrspace(1)* %addr2.2, align 4
  %sum2.1 = add nsw i32 %sum2.0, %res2.2
  %id2.3 = add nsw i64 %idbase2, 30
  %addr2.3 = getelementptr inbounds i32, i32 addrspace(1)* %src, i64 %id2.3
  %res2.3 = load i32, i32 addrspace(1)* %addr2.3, align 4
  %sum2.2 = add nsw i32 %sum2.1, %res2.3
  %addr2.4 = getelementptr inbounds i32, i32 addrspace(1)* %dst, i64 %idbase2
  store i32 %sum2.2, i32 addrspace(1)* %addr2.4, align 4
  br label %case3

;
; case 3 : address inc b/w two GEPs is a variable (BB-level invariant), not a constant
;
; CHECK-LABEL: case3:
; CHECK: [[C3T0:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* %src, i64 %{{.*}}
; CHECK: [[C3T1:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* [[C3T0]], i64 %inc0
;
; // to match add [nsw] i64 %inc1, 1. It is the 2nd add from here
;
; CHECK: %sum3.0 = add
; CHECK: [[C3T2:%.*]] = add
; CHECK-SAME: i64 %inc1, 1
;
; CHECK: [[C3T3:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* [[C3T0]], i64 [[C3T2]]
; CHECK: [[C3T4:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* [[C3T0]], i64 128
;
case3:
  %addr3 = getelementptr inbounds i32, i32 addrspace(1)* %src, i64 %idbase2
  %res3.0 = load i32, i32 addrspace(1)* %addr3, align 4
  %id3.1 = add nsw i64 %idbase2, %inc0
  %addr3.1 = getelementptr inbounds i32, i32 addrspace(1)* %src, i64 %id3.1
  %res3.1 = load i32, i32 addrspace(1)* %addr3.1, align 4
  %sum3.0 = add nsw i32 %res3.0, %res3.1
  %incinc1 = add nsw i64 %inc1, 1
  %id3.2 = add nsw i64 %idbase2, %incinc1
  %addr3.2 = getelementptr inbounds i32, i32 addrspace(1)* %src, i64 %id3.2
  %res3.2 = load i32, i32 addrspace(1)* %addr3.2, align 4
  %sum3.1 = add nsw i32 %sum3.0, %res3.2
  %id3.3 = add nsw i64 %idbase2, 128
  %addr3.3 = getelementptr inbounds i32, i32 addrspace(1)* %src, i64 %id3.3
  %res3.3 = load i32, i32 addrspace(1)* %addr3.3, align 4
  %sum3.2 = add nsw i32 %sum3.1, %res3.3
  %addr3.4 = getelementptr inbounds i32, i32 addrspace(1)* %dst, i64 %idbase2
  store i32 %sum3.2, i32 addrspace(1)* %addr3.4, align 4
;
; CHECK: ret void
;
  ret void
}

; Function Attrs: nounwind readnone
declare i16 @llvm.genx.GenISA.simdLaneId() #1

attributes #0 = { convergent nounwind "less-precise-fpmad"="true" }
attributes #1 = { nounwind readnone }

!igc.functions = !{!0}

!0 = !{void (i32 addrspace(1)*, i32 addrspace(1)*, i64, i64, i64, i32)* @test_gep, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
