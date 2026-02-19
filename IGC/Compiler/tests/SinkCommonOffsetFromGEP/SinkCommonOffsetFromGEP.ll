;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-sink-gep-constant | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%struct.foo = type { [2 x i32] }
%struct.bar = type { [2 x [2 x i32]] }


define void @f0(i32* %a, i32* %b, i1 %cond) {
entry:
  br i1 %cond, label %if.cond, label %else.cond
if.cond:
  %0 = getelementptr inbounds i32, i32* %a, i64 0
  %1 = getelementptr inbounds i32, i32* %a, i64 1
  br label %exit
else.cond:
  %2 = getelementptr inbounds i32, i32* %b, i64 0
  %3 = getelementptr inbounds i32, i32* %b, i64 1
  br label %exit
exit:
  %4 = phi i32* [ %0, %if.cond ], [ %2, %else.cond ]
  %5 = phi i32* [ %1, %if.cond ], [ %3, %else.cond ]
  %6 = load i32, i32* %4, align 4
  %7 = load i32, i32* %5, align 4
  ret void
}
 ; CHECK: [[B:%.*]] = phi i32* [ %a, %if.cond ], [ %b, %else.cond ]
 ; CHECK: [[P1:%.*]] = getelementptr inbounds i32, i32* %6, i64 1
 ; CHECK: [[P2:%.*]] = getelementptr inbounds i32, i32* %6, i64 0
 ; CHECK: load i32, i32* [[P2]], align 4
 ; CHECK: load i32, i32* [[P1]], align 4


define void @f1(%struct.foo* %a, %struct.foo* %b, i1 %cond) {
entry:
  br i1 %cond, label %if.cond, label %else.cond
if.cond:
  %0 = getelementptr inbounds %struct.foo, %struct.foo* %a, i64 0, i32 0, i32 0
  %1 = getelementptr inbounds %struct.foo, %struct.foo* %a, i64 0, i32 0, i32 1
  br label %exit
else.cond:
  %2 = getelementptr inbounds %struct.foo, %struct.foo* %b, i64 0, i32 0, i32 0
  %3 = getelementptr inbounds %struct.foo, %struct.foo* %b, i64 0, i32 0, i32 1
  br label %exit
exit:
  %4 = phi i32* [ %0, %if.cond ], [ %2, %else.cond ]
  %5 = phi i32* [ %1, %if.cond ], [ %3, %else.cond ]
  %6 = load i32, i32* %4, align 4
  %7 = load i32, i32* %5, align 4
  ret void
}
 ; CHECK: [[V1:%.*]] = getelementptr %struct.foo, %struct.foo* %a, i64 0, i32 0

 ; CHECK: [[V2:%.*]] = getelementptr %struct.foo, %struct.foo* %b, i64 0, i32 0

 ; CHECK: [[B:%.*]] = phi [2 x i32]* [ [[V1]], %if.cond ], [ [[V2]], %else.cond ]

 ; CHECK: [[P1:%.*]] = getelementptr inbounds [2 x i32], [2 x i32]* [[B]], i32 0, i32 1
 ; CHECK: [[P2:%.*]] = getelementptr inbounds [2 x i32], [2 x i32]* [[B]], i32 0, i32 0
 ; CHECK: load i32, i32* [[P2]], align 4
 ; CHECK: load i32, i32* [[P1]], align 4


define void @f2(%struct.bar* %a, %struct.bar* %b, i1 %cond) {
entry:
  br i1 %cond, label %if.cond, label %else.cond
if.cond:
  %0 = getelementptr inbounds %struct.bar, %struct.bar* %a, i32 0, i32 0, i32 0, i32 0
  %1 = getelementptr inbounds %struct.bar, %struct.bar* %a, i32 0, i32 0, i32 0, i32 1
  %2 = getelementptr inbounds %struct.bar, %struct.bar* %a, i32 0, i32 0, i32 1, i32 0
  %3 = getelementptr inbounds %struct.bar, %struct.bar* %a, i32 0, i32 0, i32 1, i32 1
  br label %exit
else.cond:
  %4 = getelementptr inbounds %struct.bar, %struct.bar* %b, i32 0, i32 0, i32 0, i32 0
  %5 = getelementptr inbounds %struct.bar, %struct.bar* %b, i32 0, i32 0, i32 0, i32 1
  %6 = getelementptr inbounds %struct.bar, %struct.bar* %b, i32 0, i32 0, i32 1, i32 0
  %7 = getelementptr inbounds %struct.bar, %struct.bar* %b, i32 0, i32 0, i32 1, i32 1
  br label %exit
exit:
  %8 = phi i32* [ %0, %if.cond ], [ %4, %else.cond ]
  %9 = phi i32* [ %1, %if.cond ], [ %5, %else.cond ]
  %10 = phi i32* [ %2, %if.cond ], [ %6, %else.cond ]
  %11 = phi i32* [ %3, %if.cond ], [ %7, %else.cond ]
  %12 = load i32, i32* %8, align 4
  %13 = load i32, i32* %9, align 4
  %14 = load i32, i32* %10, align 4
  %15 = load i32, i32* %11, align 4
  ret void
}
 ; CHECK: [[V1:%.*]] = getelementptr %struct.bar, %struct.bar* %a, i32 0, i32 0, i32 0
 ; CHECK: [[V2:%.*]] = getelementptr %struct.bar, %struct.bar* %a, i32 0, i32 0, i32 1

 ; CHECK: [[V3:%.*]] = getelementptr %struct.bar, %struct.bar* %b, i32 0, i32 0, i32 0
 ; CHECK: [[V4:%.*]] = getelementptr %struct.bar, %struct.bar* %b, i32 0, i32 0, i32 1

 ; CHECK: [[B1:%.*]] = phi [2 x i32]* [ [[V1]], %if.cond ], [ [[V3]], %else.cond
 ; CHECK: [[B2:%.*]] = phi [2 x i32]* [ [[V2]], %if.cond ], [ [[V4]], %else.cond ]

 ; CHECK: [[P1:%.*]] = getelementptr inbounds [2 x i32], [2 x i32]* [[B2]], i32 0, i32 1
 ; CHECK: [[P2:%.*]] = getelementptr inbounds [2 x i32], [2 x i32]* [[B2]], i32 0, i32 0
 ; CHECK: [[P3:%.*]] = getelementptr inbounds [2 x i32], [2 x i32]* [[B1]], i32 0, i32 1
 ; CHECK: [[P4:%.*]] = getelementptr inbounds [2 x i32], [2 x i32]* [[B1]], i32 0, i32
 ; CHECK: load i32, i32* [[P4]], align 4
 ; CHECK: load i32, i32* [[P3]], align 4
 ; CHECK: load i32, i32* [[P2]], align 4
 ; CHECK: load i32, i32* [[P1]], align 4


define void @f3(%struct.foo* %a, %struct.foo* %b, %struct.foo* %c, i1 %cond1, i1 %cond2) {
entry:
  br i1 %cond1, label %if.cond, label %edge
edge:
  %cond.or = or i1 %cond1, %cond2
  br i1 %cond.or, label %else1.cond, label %else2.cond
if.cond:
  %0 = getelementptr inbounds %struct.foo, %struct.foo* %a, i64 0, i32 0, i32 0
  %1 = getelementptr inbounds %struct.foo, %struct.foo* %a, i64 0, i32 0, i32 1
  br label %exit
else1.cond:
  %2 = getelementptr inbounds %struct.foo, %struct.foo* %b, i64 0, i32 0, i32 0
  %3 = getelementptr inbounds %struct.foo, %struct.foo* %b, i64 0, i32 0, i32 1
  br label %exit
else2.cond:
  %4 = getelementptr inbounds %struct.foo, %struct.foo* %c, i64 0, i32 0, i32 0
  %5 = getelementptr inbounds %struct.foo, %struct.foo* %c, i64 0, i32 0, i32 1
  br label %exit
exit:
  %6 = phi i32* [ %0, %if.cond ], [ %2, %else1.cond ], [ %4, %else2.cond ]
  %7 = phi i32* [ %1, %if.cond ], [ %3, %else1.cond ], [ %5, %else2.cond ]
  %8 = load i32, i32* %6, align 4
  %9 = load i32, i32* %7, align 4
  ret void
}
 ; CHECK: [[V1:%.*]] = getelementptr %struct.foo, %struct.foo* %a, i64 0, i32 0

 ; CHECK: [[V2:%.*]] = getelementptr %struct.foo, %struct.foo* %b, i64 0, i32 0

 ; CHECK: [[V3:%.*]] = getelementptr %struct.foo, %struct.foo* %c, i64 0, i32 0

 ; CHECK: [[B:%.*]] =  phi [2 x i32]* [ [[V1]], %if.cond ], [ [[V2]], %else1.cond ], [ [[V3]], %else2.cond ]

 ; CHECK: [[P1:%.*]] = getelementptr inbounds [2 x i32], [2 x i32]* [[B]], i32 0, i32 1
 ; CHECK: [[P2:%.*]] = getelementptr inbounds [2 x i32], [2 x i32]* [[B]], i32 0, i32 0
 ; CHECK: load i32, i32* [[P2]], align 4
 ; CHECK: load i32, i32* [[P1]], align 4


define void @f4(i32* %a, i32* %b, i32* %c, i1 %cond1, i1 %cond2) {
entry:
  br i1 %cond1, label %if.cond, label %edge
edge:
  %cond.or = or i1 %cond1, %cond2
  br i1 %cond.or, label %else1.cond, label %else2.cond
if.cond:
  %0 = getelementptr inbounds i32, i32* %a, i64 0
  %1 = getelementptr inbounds i32, i32* %a, i64 1
  br label %exit
else1.cond:
  %2 = getelementptr inbounds i32, i32* %b, i64 0
  %3 = getelementptr inbounds i32, i32* %b, i64 1
  br label %exit
else2.cond:
  %4 = getelementptr inbounds i32, i32* %c, i64 0
  %5 = getelementptr inbounds i32, i32* %c, i64 1
  br label %exit
exit:
  %6 = phi i32* [ %0, %if.cond ], [ %2, %else1.cond ], [ %4, %else2.cond ]
  %7 = phi i32* [ %1, %if.cond ], [ %3, %else1.cond ], [ %5, %else2.cond ]
  %8 = load i32, i32* %6, align 4
  %9 = load i32, i32* %7, align 4
  ret void
}
 ; CHECK: [[B:%.*]] = phi i32* [ %a, %if.cond ], [ %b, %else1.cond ], [ %c, %else2.cond ]
 ; CHECK: [[P1:%.*]] = getelementptr inbounds i32, i32* [[B]], i64 1
 ; CHECK: [[P2:%.*]] = getelementptr inbounds i32, i32* [[B]], i64 0
 ; CHECK: load i32, i32* [[P2]], align 4
 ; CHECK: load i32, i32* [[P1]], align 4


define void @f5(%struct.foo* %a, %struct.foo* %b, i1 %cond) {
entry:
  %0 = getelementptr inbounds %struct.foo, %struct.foo* %a, i64 0, i32 0, i32 0
  %1 = getelementptr inbounds %struct.foo, %struct.foo* %a, i64 0, i32 0, i32 1
  %2 = getelementptr inbounds %struct.foo, %struct.foo* %b, i64 0, i32 0, i32 0
  %3 = getelementptr inbounds %struct.foo, %struct.foo* %b, i64 0, i32 0, i32 1
  br i1 %cond, label %if.cond, label %else.cond
if.cond:
  br label %exit
else.cond:
  br label %exit
exit:
  %4 = phi i32* [ %0, %if.cond ], [ %2, %else.cond ]
  %5 = phi i32* [ %1, %if.cond ], [ %3, %else.cond ]
  %6 = load i32, i32* %4, align 4
  %7 = load i32, i32* %5, align 4
  ret void
}
 ; CHECK: [[V1:%.*]] = getelementptr %struct.foo, %struct.foo* %a, i64 0, i32 0

 ; CHECK: [[V2:%.*]] = getelementptr %struct.foo, %struct.foo* %b, i64 0, i32 0

 ; CHECK: [[B:%.*]] = phi [2 x i32]* [ [[V1]], %if.cond ], [ [[V2]], %else.cond ]

 ; CHECK: [[P1:%.*]] = getelementptr inbounds [2 x i32], [2 x i32]* [[B]], i32 0, i32 1
 ; CHECK: [[P2:%.*]] = getelementptr inbounds [2 x i32], [2 x i32]* [[B]], i32 0, i32 0
 ; CHECK: load i32, i32* [[P2]], align 4
 ; CHECK: load i32, i32* [[P1]], align 4


define void @f6(i32* %a, i32* %b, i1 %cond) {
entry:
  %0 = getelementptr inbounds i32, i32* %a, i64 0
  %1 = getelementptr inbounds i32, i32* %a, i64 1
  %2 = getelementptr inbounds i32, i32* %b, i64 0
  %3 = getelementptr inbounds i32, i32* %b, i64 1
  br i1 %cond, label %if.cond, label %else.cond
if.cond:
  br label %exit
else.cond:
  br label %exit
exit:
  %4 = phi i32* [ %0, %if.cond ], [ %2, %else.cond ]
  %5 = phi i32* [ %1, %if.cond ], [ %3, %else.cond ]
  %6 = load i32, i32* %4, align 4
  %7 = load i32, i32* %5, align 4
  ret void
}
 ; CHECK: [[B:%.*]] = phi i32* [ %a, %if.cond ], [ %b, %else.cond ]
 ; CHECK: [[P1:%.*]] = getelementptr inbounds i32, i32* %6, i64 1
 ; CHECK: [[P2:%.*]] = getelementptr inbounds i32, i32* %6, i64 0
 ; CHECK: load i32, i32* [[P2]], align 4
 ; CHECK: load i32, i32* [[P1]], align 4


define void @f7(%struct.foo* %a, %struct.foo* %b, i1 %cond) {
entry:
  br i1 %cond, label %if.cond, label %else.cond
if.cond:
  %0 = getelementptr inbounds %struct.foo, %struct.foo* %a, i64 0, i32 0, i32 0
  %1 = addrspacecast i32* %0 to i32 addrspace(1)*
  %2 = getelementptr inbounds %struct.foo, %struct.foo* %a, i64 0, i32 0, i32 1
  %3 = addrspacecast i32* %2 to i32 addrspace(1)*
  br label %exit
else.cond:
  %4 = getelementptr inbounds %struct.foo, %struct.foo* %b, i64 0, i32 0, i32 0
  %5 = addrspacecast i32* %4 to i32 addrspace(1)*
  %6 = getelementptr inbounds %struct.foo, %struct.foo* %b, i64 0, i32 0, i32 1
  %7 = addrspacecast i32* %6 to i32 addrspace(1)*
  br label %exit
exit:
  %8 = phi i32 addrspace(1)* [ %1, %if.cond ], [ %5, %else.cond ]
  %9 = phi i32 addrspace(1)* [ %3, %if.cond ], [ %7, %else.cond ]
  %10 = load i32, i32 addrspace(1)* %8, align 4
  %11 = load i32, i32 addrspace(1)* %9, align 4
  ret void
}
 ; CHECK: [[V1:%.*]] = getelementptr %struct.foo, %struct.foo* %a, i64 0, i32
 ; CHECK: [[A1:%.*]] = addrspacecast [2 x i32]* [[V1]] to [2 x i32] addrspace(1)*

 ; CHECK: [[V2:%.*]] = getelementptr %struct.foo, %struct.foo* %b, i64 0, i32
 ; CHECK: [[A2:%.*]] = addrspacecast [2 x i32]* [[V2]] to [2 x i32] addrspace(1)*

 ; CHECK: [[B:%.*]] = phi [2 x i32] addrspace(1)* [ [[A1]], %if.cond ], [ [[A2]], %else.cond ]

 ; CHECK: [[P1:%.*]] = getelementptr inbounds [2 x i32], [2 x i32] addrspace(1)* [[B]], i32 0, i32 1
 ; CHECK: [[P2:%.*]] = getelementptr inbounds [2 x i32], [2 x i32] addrspace(1)* [[B]], i32 0, i32 0
 ; CHECK: load i32, i32 addrspace(1)* [[P2]], align 4
 ; CHECK: load i32, i32 addrspace(1)* [[P1]], align 4


define void @f8(%struct.bar* %a, %struct.bar* %b, i1 %cond) {
entry:
  br i1 %cond, label %if.cond, label %else.cond
if.cond:
  %0 = getelementptr inbounds %struct.bar, %struct.bar* %a, i32 0, i32 0, i32 0, i32 0
  %1 = getelementptr inbounds %struct.bar, %struct.bar* %a, i32 0, i32 0, i32 1, i32 1
  br label %exit
else.cond:
  %2 = getelementptr inbounds %struct.bar, %struct.bar* %b, i32 0, i32 0, i32 0, i32 0
  %3 = getelementptr inbounds %struct.bar, %struct.bar* %b, i32 0, i32 0, i32 1, i32 1
  br label %exit
exit:
  %4 = phi i32* [ %0, %if.cond ], [ %2, %else.cond ]
  %5 = phi i32* [ %1, %if.cond ], [ %3, %else.cond ]
  %6 = load i32, i32* %4, align 4
  %7 = load i32, i32* %5, align 4
  ret void
}
 ; CHECK-LABEL: define void @f8
 ; CHECK-LABEL: entry:
 ; CHECK:   br i1 %cond, label %if.cond, label %else.cond
 ; CHECK-LABEL: if.cond:
 ; CHECK:   %0 = getelementptr inbounds %struct.bar, %struct.bar* %a, i32 0, i32 0, i32 0, i32 0
 ; CHECK:   %1 = getelementptr inbounds %struct.bar, %struct.bar* %a, i32 0, i32 0, i32 1, i32 1
 ; CHECK:   br label %exit
 ; CHECK-LABEL: else.cond:
 ; CHECK:   %2 = getelementptr inbounds %struct.bar, %struct.bar* %b, i32 0, i32 0, i32 0, i32 0
 ; CHECK:   %3 = getelementptr inbounds %struct.bar, %struct.bar* %b, i32 0, i32 0, i32 1, i32 1
 ; CHECK:   br label %exit
 ; CHECK-LABEL: exit:
 ; CHECK:   %4 = phi i32* [ %0, %if.cond ], [ %2, %else.cond ]
 ; CHECK:   %5 = phi i32* [ %1, %if.cond ], [ %3, %else.cond ]
 ; CHECK:   %6 = load i32, i32* %4, align 4
 ; CHECK:   %7 = load i32, i32* %5, align 4
 ; CHECK:   ret void


define void @f9(i32 %a, i32 %b, i1 %cond) {
entry:
  br i1 %cond, label %if.cond, label %else.cond
if.cond:
  %0 = inttoptr i32 %a to i32*
  %1 = add i32 %a, 4
  %2 = inttoptr i32 %1 to i32*
  br label %exit
else.cond:
  %3 = inttoptr i32 %b to i32*
  %4 = add i32 %b, 4
  %5 = inttoptr i32 %4 to i32*
  br label %exit
exit:
  %6 = phi i32* [ %0, %if.cond ], [ %3, %else.cond ]
  %7 = phi i32* [ %2, %if.cond ], [ %5, %else.cond ]
  %8 = load i32, i32* %6, align 4
  %9 = load i32, i32* %7, align 4
  ret void
}
 ; CHECK-LABEL: define void @f9
 ; CHECK-LABEL: entry:
 ; CHECK:   br i1 %cond, label %if.cond, label %else.cond
 ; CHECK-LABEL: if.cond:
 ; CHECK:   %0 = inttoptr i32 %a to i32*
 ; CHECK:   %1 = add i32 %a, 4
 ; CHECK:   %2 = inttoptr i32 %1 to i32*
 ; CHECK:   br label %exit
 ; CHECK-LABEL: else.cond:
 ; CHECK:   %3 = inttoptr i32 %b to i32*
 ; CHECK:   %4 = add i32 %b, 4
 ; CHECK:   %5 = inttoptr i32 %4 to i32*
 ; CHECK:   br label %exit
 ; CHECK-LABEL: exit:
 ; CHECK:   %6 = phi i32* [ %0, %if.cond ], [ %3, %else.cond ]
 ; CHECK:   %7 = phi i32* [ %2, %if.cond ], [ %5, %else.cond ]
 ; CHECK:   %8 = load i32, i32* %6, align 4
 ; CHECK:   %9 = load i32, i32* %7, align 4
 ; CHECK:   ret void

define void @f10(i32* %a, %struct.foo* %b, i1 %cond) {
entry:
  br i1 %cond, label %if.cond, label %else.cond
if.cond:
  %0 = getelementptr inbounds i32, i32* %a, i64 0
  %1 = getelementptr inbounds i32, i32* %a, i64 1
  br label %exit
else.cond:
  %2 = getelementptr inbounds %struct.foo, %struct.foo* %b, i64 0, i32 0, i32 0
  %3 = getelementptr inbounds %struct.foo, %struct.foo* %b, i64 0, i32 0, i32 1
  br label %exit
exit:
  %4 = phi i32* [ %0, %if.cond ], [ %2, %else.cond ]
  %5 = phi i32* [ %1, %if.cond ], [ %3, %else.cond ]
  %6 = load i32, i32* %4, align 4
  %7 = load i32, i32* %5, align 4
  ret void
}

