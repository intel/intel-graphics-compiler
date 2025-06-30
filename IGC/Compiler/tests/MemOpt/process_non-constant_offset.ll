;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers %s -S -o - -igc-memopt | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

; CHECK-LABEL: define void @f0
; CHECK: %add1 = add nsw i32 %val1, 1
; CHECK: %sub1 = sub nsw i32 %add1, %val2
; CHECK: %sext1 = sext i32 %sub1 to i64
; CHECK: %gep1 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext1
; CHECK: [[GEPADDR1:%.*]] = addrspacecast ptr addrspace(4) %gep1 to ptr addrspace(1)
; CHECK: [[LD:%.*]] = load <3 x float>, ptr addrspace(1) [[GEPADDR1]], align 4
; ...
; CHECK: %add4 = add nsw i32 %val3, 1
; CHECK: %sub4 = sub nsw i32 %add4, %val2
; CHECK: %sext4 = sext i32 %sub4 to i64
; CHECK: %gep4 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext4
; CHECK: [[GEPADDR4:%.*]] = addrspacecast ptr addrspace(4) %gep4 to ptr addrspace(1)
; CHECK: store <3 x float> [[DATA:%.*]], ptr addrspace(1) [[GEPADDR4]], align 4
; CHECK: ret void
;
define void @f0(i64 %dst, i64 %src, i32 %val1, i32 %val2, i32 %val3) {
entry:
  %dstptr = inttoptr i64 %dst to ptr addrspace(4)
  %srcptr = inttoptr i64 %src to ptr addrspace(4)
  %add1 = add nsw i32 %val1, 1
  %sub1 = sub nsw i32 %add1, %val2
  %sext1 = sext i32 %sub1 to i64
  %gep1 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext1
  %gepaddr1 = addrspacecast ptr addrspace(4) %gep1 to ptr addrspace(1)
  %ld1 = load float, ptr addrspace(1) %gepaddr1, align 4
  %add2 = add nsw i32 %val1, 2
  %sub2 = sub nsw i32 %add2, %val2
  %sext2 = sext i32 %sub2 to i64
  %gep2 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext2
  %gepaddr2 = addrspacecast ptr addrspace(4) %gep2 to ptr addrspace(1)
  %ld2 = load float, float addrspace(1)* %gepaddr2, align 4
  %add3 = add nsw i32 %val1, 3
  %sub3 = sub nsw i32 %add3, %val2
  %sext3 = sext i32 %sub3 to i64
  %gep3 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext3
  %gepaddr3 = addrspacecast ptr addrspace(4) %gep3 to ptr addrspace(1)
  %ld3 = load float, ptr addrspace(1) %gepaddr3, align 4
  %add4 = add nsw i32 %val3, 1
  %sub4 = sub nsw i32 %add4, %val2
  %sext4 = sext i32 %sub4 to i64
  %gep4 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext4
  %gepaddr4 = addrspacecast ptr addrspace(4) %gep4 to ptr addrspace(1)
  store float %ld1, ptr addrspace(1) %gepaddr4, align 4
  %add5 = add nsw i32 %val3, 2
  %sub5 = sub nsw i32 %add5, %val2
  %sext5 = sext i32 %sub5 to i64
  %gep5 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext5
  %gepaddr5 = addrspacecast ptr addrspace(4) %gep5 to ptr addrspace(1)
  store float %ld2, float addrspace(1)* %gepaddr5, align 4
  %add6 = add nsw i32 %val3, 3
  %sub6 = sub nsw i32 %add6, %val2
  %sext6 = sext i32 %sub6 to i64
  %gep6 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext6
  %gepaddr6 = addrspacecast ptr addrspace(4) %gep6 to ptr addrspace(1)
  store float %ld3, ptr addrspace(1) %gepaddr6, align 4
  ret void
}

; CHECK-LABEL: define void @f1
; CHECK: [[ADD1:%.*]] = add nsw i32 %val1, 1
; CHECK: [[SUB1:%.*]] = sub nsw i32 %val2, [[ADD1]]
; CHECK: [[SEXT1:%.*]] = sext i32 [[SUB1]] to i64
; CHECK: [[GEP1:%.*]] = getelementptr float, ptr addrspace(4) %srcptr, i64 [[SEXT1]]
; CHECK: [[GEPADDR1:%.*]] = addrspacecast ptr addrspace(4) [[GEP1]] to ptr addrspace(1)
; CHECK: [[GEP2:%.*]] = getelementptr float, ptr addrspace(1) [[GEPADDR1]], i64 -2
; CHECK: [[LD:%.*]] = load <3 x float>, ptr addrspace(1) [[GEP2]], align 4
; ...
; CHECK: [[ADD2:%.*]] = add nsw i32 %val3, 3
; CHECK: [[SUB2:%.*]] = sub nsw i32 %val2, [[ADD2]]
; CHECK: [[SEXT2:%.*]] = sext i32 [[SUB2]] to i64
; CHECK: [[GEP3:%.*]] = getelementptr float, ptr addrspace(4) %dstptr, i64 [[SEXT2]]
; CHECK: [[GEPADDR2:%.*]] = addrspacecast ptr addrspace(4) [[GEP3]] to ptr addrspace(1)
; CHECK: store <3 x float> [[DATA:%.*]], ptr addrspace(1) [[GEPADDR2]], align 4
; CHECK: ret void
;
define void @f1(i64 %dst, i64 %src, i32 %val1, i32 %val2, i32 %val3) {
entry:
  %dstptr = inttoptr i64 %dst to float addrspace(4)*
  %srcptr = inttoptr i64 %src to float addrspace(4)*
  %add1 = add nsw i32 %val1, 1
  %sub1 = sub nsw i32 %val2, %add1
  %sext1 = sext i32 %sub1 to i64
  %gep1 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext1
  %gepaddr1 = addrspacecast ptr addrspace(4) %gep1 to ptr addrspace(1)
  %ld1 = load float, ptr addrspace(1) %gepaddr1, align 4
  %add2 = add nsw i32 %val1, 2
  %sub2 = sub nsw i32 %val2, %add2
  %sext2 = sext i32 %sub2 to i64
  %gep2 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext2
  %gepaddr2 = addrspacecast ptr addrspace(4) %gep2 to ptr addrspace(1)
  %ld2 = load float, ptr addrspace(1) %gepaddr2, align 4
  %add3 = add nsw i32 %val1, 3
  %sub3 = sub nsw i32 %val2, %add3
  %sext3 = sext i32 %sub3 to i64
  %gep3 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext3
  %gepaddr3 = addrspacecast ptr addrspace(4) %gep3 to ptr addrspace(1)
  %ld3 = load float, ptr addrspace(1) %gepaddr3, align 4
  %add4 = add nsw i32 %val3, 1
  %sub4 = sub nsw i32 %val2, %add4
  %sext4 = sext i32 %sub4 to i64
  %gep4 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext4
  %gepaddr4 = addrspacecast ptr addrspace(4) %gep4 to ptr addrspace(1)
  store float %ld1, ptr addrspace(1) %gepaddr4, align 4
  %add5 = add nsw i32 %val3, 2
  %sub5 = sub nsw i32 %val2, %add5
  %sext5 = sext i32 %sub5 to i64
  %gep5 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext5
  %gepaddr5 = addrspacecast ptr addrspace(4) %gep5 to ptr addrspace(1)
  store float %ld2, ptr addrspace(1) %gepaddr5, align 4
  %add6 = add nsw i32 %val3, 3
  %sub6 = sub nsw i32 %val2, %add6
  %sext6 = sext i32 %sub6 to i64
  %gep6 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext6
  %gepaddr6 = addrspacecast ptr addrspace(4) %gep6 to ptr addrspace(1)
  store float %ld3, ptr addrspace(1) %gepaddr6, align 4
  ret void
}

; CHECK-LABEL: define void @f2
; CHECK: [[SUB1:%.*]] = sub nsw i32 %val1, 1
; CHECK: [[ADD1:%.*]] = add nsw i32 [[SUB1]], %val2
; CHECK: [[SEXT1:%.*]] = sext i32 [[ADD1]] to i64
; CHECK: [[GEP1:%.*]] = getelementptr float, ptr addrspace(4) %srcptr, i64 [[SEXT1]]
; CHECK: [[GEPADDR1:%.*]] = addrspacecast ptr addrspace(4) [[GEP1]] to ptr addrspace(1)
; CHECK: [[GEP2:%.*]] = getelementptr float, ptr addrspace(1) [[GEPADDR1]], i64 -2
; CHECK: [[LD1:%.*]] = load <3 x float>, ptr addrspace(1) [[GEP2]], align 4
; ...
; CHECK: [[SUB2:%.*]] = sub nsw i32 %val3, 3
; CHECK: [[ADD2:%.*]] = add nsw i32 [[SUB2]], %val2
; CHECK: [[SEXT2:%.*]] = sext i32 [[ADD2]] to i64
; CHECK: [[GEP3:%.*]] =  getelementptr float, ptr addrspace(4) %dstptr, i64 [[SEXT2]]
; CHECK: [[GEPADDR2:%.*]] = addrspacecast ptr addrspace(4) [[GEP3]] to ptr addrspace(1)
; CHECK: store <3 x float> [[DATA:%.*]], ptr addrspace(1) [[GEPADDR2]], align 4
; CHECK: ret void
;
define void @f2(i64 %dst, i64 %src, i32 %val1, i32 %val2, i32 %val3) {
entry:
  %dstptr = inttoptr i64 %dst to float addrspace(4)*
  %srcptr = inttoptr i64 %src to float addrspace(4)*
  %sub1 = sub nsw i32 %val1, 1
  %add1 = add nsw i32 %sub1, %val2
  %sext1 = sext i32 %add1 to i64
  %gep1 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext1
  %gepaddr1 = addrspacecast ptr addrspace(4) %gep1 to ptr addrspace(1)
  %ld1 = load float, ptr addrspace(1) %gepaddr1, align 4
  %sub2 = sub nsw i32 %val1, 2
  %add2 = add nsw i32 %sub2, %val2
  %sext2 = sext i32 %add2 to i64
  %gep2 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext2
  %gepaddr2 = addrspacecast ptr addrspace(4) %gep2 to ptr addrspace(1)
  %ld2 = load float, ptr addrspace(1) %gepaddr2, align 4
  %sub3 = sub nsw i32 %val1, 3
  %add3 = add nsw i32 %sub3, %val2
  %sext3 = sext i32 %add3 to i64
  %gep3 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext3
  %gepaddr3 = addrspacecast ptr addrspace(4) %gep3 to ptr addrspace(1)
  %ld3 = load float, ptr addrspace(1) %gepaddr3, align 4
  %sub4 = sub nsw i32 %val3, 1
  %add4 = add nsw i32 %sub4, %val2
  %sext4 = sext i32 %add4 to i64
  %gep4 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext4
  %gepaddr4 = addrspacecast ptr addrspace(4) %gep4 to ptr addrspace(1)
  store float %ld1, ptr addrspace(1) %gepaddr4, align 4
  %sub5 = sub nsw i32 %val3, 2
  %add5 = add nsw i32 %sub5, %val2
  %sext5 = sext i32 %add5 to i64
  %gep5 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext5
  %gepaddr5 = addrspacecast ptr addrspace(4) %gep5 to ptr addrspace(1)
  store float %ld2, ptr addrspace(1) %gepaddr5, align 4
  %sub6 = sub nsw i32 %val3, 3
  %add6 = add nsw i32 %sub6, %val2
  %sext6 = sext i32 %add6 to i64
  %gep6 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext6
  %gepaddr6 = addrspacecast ptr addrspace(4) %gep6 to ptr addrspace(1)
  store float %ld3, ptr addrspace(1) %gepaddr6, align 4
  ret void
}

; Check that loads/stores are optimized when constant int operands are negative.
; CHECK-LABEL: define void @f3
; CHECK: [[ADD1:%.*]] = add nsw i32 %val1, -1
; CHECK: [[SUB1:%.*]] = sub nsw i32 [[ADD1]], %val2
; CHECK: [[SEXT1:%.*]] = sext i32 [[SUB1]] to i64
; CHECK: [[GEP1:%.*]] = getelementptr float, ptr addrspace(4) %srcptr, i64 [[SEXT1]]
; CHECK: [[GEPADDR1:%.*]] = addrspacecast ptr addrspace(4) [[GEP1]] to ptr addrspace(1)
; CHECK: [[GEP2:%.*]] = getelementptr float, ptr addrspace(1) [[GEPADDR1]], i64 -2
; CHECK: [[LD1:%.*]] = load <3 x float>, ptr addrspace(1) [[GEP2]], align 4
; ...
; CHECK: [[ADD4:%.*]] = add nsw i32 %val3, -6
; CHECK: [[SUB4:%.*]] = sub nsw i32 [[ADD4]], %val2
; CHECK: [[SEXT4:%.*]] = sext i32 [[SUB4]] to i64
; CHECK: [[GEP3:%.*]] = getelementptr float, ptr addrspace(4) %dstptr, i64 [[SEXT4]]
; CHECK: [[GEPADDR3:%.*]] = addrspacecast ptr addrspace(4) [[GEP3]] to ptr addrspace(1)
; CHECK: store <3 x float> [[DATA:%.*]], ptr addrspace(1) [[GEPADDR3]], align 4
; CHECK: ret void
;
define void @f3(i64 %dst, i64 %src, i32 %val1, i32 %val2, i32 %val3) {
entry:
  %dstptr = inttoptr i64 %dst to ptr addrspace(4)
  %srcptr = inttoptr i64 %src to ptr addrspace(4)
  %add1 = add nsw i32 %val1, -1
  %sub1 = sub nsw i32 %add1, %val2
  %sext1 = sext i32 %sub1 to i64
  %gep1 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext1
  %gepaddr1 = addrspacecast ptr addrspace(4) %gep1 to ptr addrspace(1)
  %ld1 = load float, ptr addrspace(1) %gepaddr1, align 4
  %add2 = add nsw i32 %val1, -2
  %sub2 = sub nsw i32 %add2, %val2
  %sext2 = sext i32 %sub2 to i64
  %gep2 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext2
  %gepaddr2 = addrspacecast ptr addrspace(4) %gep2 to ptr addrspace(1)
  %ld2 = load float, float addrspace(1)* %gepaddr2, align 4
  %add3 = add nsw i32 %val1, -3
  %sub3 = sub nsw i32 %add3, %val2
  %sext3 = sext i32 %sub3 to i64
  %gep3 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext3
  %gepaddr3 = addrspacecast ptr addrspace(4) %gep3 to ptr addrspace(1)
  %ld3 = load float, ptr addrspace(1) %gepaddr3, align 4
  %add4 = add nsw i32 %val3, -4
  %sub4 = sub nsw i32 %add4, %val2
  %sext4 = sext i32 %sub4 to i64
  %gep4 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext4
  %gepaddr4 = addrspacecast ptr addrspace(4) %gep4 to ptr addrspace(1)
  store float %ld1, ptr addrspace(1) %gepaddr4, align 4
  %add5 = add nsw i32 %val3, -5
  %sub5 = sub nsw i32 %add5, %val2
  %sext5 = sext i32 %sub5 to i64
  %gep5 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext5
  %gepaddr5 = addrspacecast ptr addrspace(4) %gep5 to ptr addrspace(1)
  store float %ld2, float addrspace(1)* %gepaddr5, align 4
  %add6 = add nsw i32 %val3, -6
  %sub6 = sub nsw i32 %add6, %val2
  %sext6 = sext i32 %sub6 to i64
  %gep6 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext6
  %gepaddr6 = addrspacecast ptr addrspace(4) %gep6 to ptr addrspace(1)
  store float %ld3, ptr addrspace(1) %gepaddr6, align 4
  ret void
}


; Check that loads/stores are optimized when constant int operands are negative.
; CHECK-LABEL: define void @f4
; CHECK: [[ADD1:%.*]] = add nsw i32 -1, %val1
; CHECK: [[SUB1:%.*]] = sub nsw i32 %val2, [[ADD1]]
; CHECK: [[SEXT1:%.*]] = sext i32 [[SUB1]] to i64
; CHECK: [[GEP1:%.*]] = getelementptr float, ptr addrspace(4) %srcptr, i64 [[SEXT1]]
; CHECK: [[GEPADDR1:%.*]] = addrspacecast ptr addrspace(4) [[GEP1]] to ptr addrspace(1)
; CHECK: [[LD1:%.*]] = load <3 x float>, ptr addrspace(1) [[GEPADDR1]], align 4
; ...
; CHECK: [[ADD4:%.*]] = add nsw i32 -4, %val3
; CHECK: [[SUB4:%.*]] = sub nsw i32 %val2, [[ADD4]]
; CHECK: [[SEXT4:%.*]] = sext i32 [[SUB4]] to i64
; CHECK: [[GEP3:%.*]] = getelementptr float, ptr addrspace(4) %dstptr, i64 [[SEXT4]]
; CHECK: [[GEPADDR3:%.*]] = addrspacecast ptr addrspace(4) [[GEP3]] to ptr addrspace(1)
; CHECK: store <3 x float> [[DATA:%.*]], ptr addrspace(1) [[GEPADDR3]], align 4
; CHECK: ret void
;
define void @f4(i64 %dst, i64 %src, i32 %val1, i32 %val2, i32 %val3) {
entry:
  %dstptr = inttoptr i64 %dst to ptr addrspace(4)
  %srcptr = inttoptr i64 %src to ptr addrspace(4)
  %add1 = add nsw i32 -1, %val1
  %sub1 = sub nsw i32 %val2, %add1
  %sext1 = sext i32 %sub1 to i64
  %gep1 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext1
  %gepaddr1 = addrspacecast ptr addrspace(4) %gep1 to ptr addrspace(1)
  %ld1 = load float, ptr addrspace(1) %gepaddr1, align 4
  %add2 = add nsw i32 -2, %val1
  %sub2 = sub nsw i32 %val2, %add2
  %sext2 = sext i32 %sub2 to i64
  %gep2 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext2
  %gepaddr2 = addrspacecast ptr addrspace(4) %gep2 to ptr addrspace(1)
  %ld2 = load float, float addrspace(1)* %gepaddr2, align 4
  %add3 = add nsw i32 -3, %val1
  %sub3 = sub nsw i32 %val2, %add3
  %sext3 = sext i32 %sub3 to i64
  %gep3 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext3
  %gepaddr3 = addrspacecast ptr addrspace(4) %gep3 to ptr addrspace(1)
  %ld3 = load float, ptr addrspace(1) %gepaddr3, align 4
  %add4 = add nsw i32 -4, %val3
  %sub4 = sub nsw i32 %val2, %add4
  %sext4 = sext i32 %sub4 to i64
  %gep4 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext4
  %gepaddr4 = addrspacecast ptr addrspace(4) %gep4 to ptr addrspace(1)
  store float %ld1, ptr addrspace(1) %gepaddr4, align 4
  %add5 = add nsw i32 -5, %val3
  %sub5 = sub nsw i32 %val2, %add5
  %sext5 = sext i32 %sub5 to i64
  %gep5 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext5
  %gepaddr5 = addrspacecast ptr addrspace(4) %gep5 to ptr addrspace(1)
  store float %ld2, float addrspace(1)* %gepaddr5, align 4
  %add6 = add nsw i32 -6, %val3
  %sub6 = sub nsw i32 %val2, %add6
  %sext6 = sext i32 %sub6 to i64
  %gep6 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext6
  %gepaddr6 = addrspacecast ptr addrspace(4) %gep6 to ptr addrspace(1)
  store float %ld3, ptr addrspace(1) %gepaddr6, align 4
  ret void
}

; Don't optimize loads/stores if nsw flag is not set.
; CHECK-LABEL: define void @f5
; CHECK-NOT: load <3 x i32>
; CHECK-NOT: store <3 x i32>
define void @f5(i64 %dst, i64 %src, i32 %val1, i32 %val2, i32 %val3) {
entry:
  %dstptr = inttoptr i64 %dst to ptr addrspace(4)
  %srcptr = inttoptr i64 %src to ptr addrspace(4)
  %add1 = add i32 %val1, 1
  %sub1 = sub i32 %add1, %val2
  %sext1 = sext i32 %sub1 to i64
  %gep1 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext1
  %gepaddr1 = addrspacecast ptr addrspace(4) %gep1 to ptr addrspace(1)
  %ld1 = load float, ptr addrspace(1) %gepaddr1, align 4
  %add2 = add i32 %val1, 2
  %sub2 = sub i32 %add2, %val2
  %sext2 = sext i32 %sub2 to i64
  %gep2 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext2
  %gepaddr2 = addrspacecast ptr addrspace(4) %gep2 to ptr addrspace(1)
  %ld2 = load float, ptr addrspace(1) %gepaddr2, align 4
  %add3 = add i32 %val1, 3
  %sub3 = sub i32 %add3, %val2
  %sext3 = sext i32 %sub3 to i64
  %gep3 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext3
  %gepaddr3 = addrspacecast ptr addrspace(4) %gep3 to ptr addrspace(1)
  %ld3 = load float, ptr addrspace(1) %gepaddr3, align 4
  %add4 = add i32 %val3, 1
  %sub4 = sub i32 %add4, %val2
  %sext4 = sext i32 %sub4 to i64
  %gep4 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext4
  %gepaddr4 = addrspacecast ptr addrspace(4) %gep4 to ptr addrspace(1)
  store float %ld1, ptr addrspace(1) %gepaddr4, align 4
  %add5 = add i32 %val3, 2
  %sub5 = sub i32 %add5, %val2
  %sext5 = sext i32 %sub5 to i64
  %gep5 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext5
  %gepaddr5 = addrspacecast ptr addrspace(4) %gep5 to ptr addrspace(1)
  store float %ld2, ptr addrspace(1) %gepaddr5, align 4
  %add6 = add i32 %val3, 3
  %sub6 = sub i32 %add6, %val2
  %sext6 = sext i32 %sub6 to i64
  %gep6 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext6
  %gepaddr6 = addrspacecast ptr addrspace(4) %gep6 to ptr addrspace(1)
  store float %ld3, ptr addrspace(1) %gepaddr6, align 4
  ret void
}

; Don't optimize loads/stores if constant offset can't be derived.
; CHECK-LABEL: define void @f6
; CHECK: %ld1 = load float, ptr addrspace(1) %gepaddr1, align 4
; CHECK: %ld2 = load float, ptr addrspace(1) %gepaddr2, align 4
; CHECK: %ld3 = load float, ptr addrspace(1) %gepaddr3, align 4
; CHECK: store float %ld1, ptr addrspace(1) %gepaddr4, align 4
; CHECK: store float %ld2, ptr addrspace(1) %gepaddr5, align 4
; CHECK: store float %ld3, ptr addrspace(1) %gepaddr6, align 4
; CHECK: ret void
;
define void @f6(i64 %dst, i64 %src, i32 %val1, i32 %val2, i32 %val3, i32 %val4, i32 %val5, i32 %val6) {
entry:
  %dstptr = inttoptr i64 %dst to ptr addrspace(4)
  %srcptr = inttoptr i64 %src to ptr addrspace(4)
  %add1 = add nsw i32 %val4, 80
  %sub1 = sub nsw i32 %add1, %val2
  %sext1 = sext i32 %sub1 to i64
  %gep1 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext1
  %gepaddr1 = addrspacecast ptr addrspace(4) %gep1 to ptr addrspace(1)
  %ld1 = load float, ptr addrspace(1) %gepaddr1, align 4
  %add2 = add nsw i32 %val5, 80
  %sub2 = sub nsw i32 %add2, %val2
  %sext2 = sext i32 %sub2 to i64
  %gep2 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext2
  %gepaddr2 = addrspacecast ptr addrspace(4) %gep2 to ptr addrspace(1)
  %ld2 = load float, ptr addrspace(1) %gepaddr2, align 4
  %add3 = add nsw i32 %val6, 80
  %sub3 = sub nsw i32 %add3, %val2
  %sext3 = sext i32 %sub3 to i64
  %gep3 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext3
  %gepaddr3 = addrspacecast ptr addrspace(4) %gep3 to ptr addrspace(1)
  %ld3 = load float, ptr addrspace(1) %gepaddr3, align 4
  %add4 = add nsw i32 %val4, 80
  %sub4 = sub nsw i32 %add4, %val2
  %sext4 = sext i32 %sub4 to i64
  %gep4 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext4
  %gepaddr4 = addrspacecast ptr addrspace(4) %gep4 to ptr addrspace(1)
  store float %ld1, ptr addrspace(1) %gepaddr4, align 4
  %add5 = add nsw i32 %val5, 80
  %sub5 = sub nsw i32 %add5, %val2
  %sext5 = sext i32 %sub5 to i64
  %gep5 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext5
  %gepaddr5 = addrspacecast ptr addrspace(4) %gep5 to ptr addrspace(1)
  store float %ld2, ptr addrspace(1) %gepaddr5, align 4
  %add6 = add nsw i32 %val6, 80
  %sub6 = sub nsw i32 %add6, %val2
  %sext6 = sext i32 %sub6 to i64
  %gep6 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext6
  %gepaddr6 = addrspacecast ptr addrspace(4) %gep6 to ptr addrspace(1)
  store float %ld3, ptr addrspace(1) %gepaddr6, align 4
  ret void
}

; Don't optimize loads/stores if constant offset can't be derived.
; CHECK-LABEL: define void @f7
; CHECK: %ld1 = load float, ptr addrspace(1) %gepaddr1, align 4
; CHECK: %ld2 = load float, ptr addrspace(1) %gepaddr2, align 4
; CHECK: %ld3 = load float, ptr addrspace(1) %gepaddr3, align 4
; CHECK: store float %ld1, ptr addrspace(1) %gepaddr4, align 4
; CHECK: store float %ld2, ptr addrspace(1) %gepaddr5, align 4
; CHECK: store float %ld3, ptr addrspace(1) %gepaddr6, align 4
; CHECK: ret void
;
define void @f7(i64 %dst, i64 %src, i32 %val1, i32 %val2, i32 %val3, i32 %val4, i32 %val5, i32 %val6) {
entry:
  %dstptr = inttoptr i64 %dst to ptr addrspace(4)
  %srcptr = inttoptr i64 %src to ptr addrspace(4)
  %add1 = add nsw i32 %val4, %val1
  %sub1 = sub nsw i32 %add1, %val2
  %sext1 = sext i32 %sub1 to i64
  %gep1 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext1
  %gepaddr1 = addrspacecast ptr addrspace(4) %gep1 to ptr addrspace(1)
  %ld1 = load float, ptr addrspace(1) %gepaddr1, align 4
  %add2 = add nsw i32 %val5, %val1
  %sub2 = sub nsw i32 %add2, %val2
  %sext2 = sext i32 %sub2 to i64
  %gep2 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext2
  %gepaddr2 = addrspacecast ptr addrspace(4) %gep2 to ptr addrspace(1)
  %ld2 = load float, ptr addrspace(1) %gepaddr2, align 4
  %add3 = add nsw i32 %val6, %val1
  %sub3 = sub nsw i32 %add3, %val2
  %sext3 = sext i32 %sub3 to i64
  %gep3 = getelementptr float, ptr addrspace(4) %srcptr, i64 %sext3
  %gepaddr3 = addrspacecast ptr addrspace(4) %gep3 to ptr addrspace(1)
  %ld3 = load float, ptr addrspace(1) %gepaddr3, align 4
  %add4 = add nsw i32 %val4, %val3
  %sub4 = sub nsw i32 %add4, %val2
  %sext4 = sext i32 %sub4 to i64
  %gep4 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext4
  %gepaddr4 = addrspacecast ptr addrspace(4) %gep4 to ptr addrspace(1)
  store float %ld1, ptr addrspace(1) %gepaddr4, align 4
  %add5 = add nsw i32 %val5, %val3
  %sub5 = sub nsw i32 %add5, %val2
  %sext5 = sext i32 %sub5 to i64
  %gep5 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext5
  %gepaddr5 = addrspacecast ptr addrspace(4) %gep5 to ptr addrspace(1)
  store float %ld2, ptr addrspace(1) %gepaddr5, align 4
  %add6 = add nsw i32 %val6, %val3
  %sub6 = sub nsw i32 %add6, %val2
  %sext6 = sext i32 %sub6 to i64
  %gep6 = getelementptr float, ptr addrspace(4) %dstptr, i64 %sext6
  %gepaddr6 = addrspacecast ptr addrspace(4) %gep6 to ptr addrspace(1)
  store float %ld3, ptr addrspace(1) %gepaddr6, align 4
  ret void
}

!igc.functions = !{!0, !3, !5, !7, !9, !11, !13, !15}

!0 = !{void (i64, i64, i32, i32, i32)* @f0, !1}
!3 = !{void (i64, i64, i32, i32, i32)* @f1, !1}
!5 = !{void (i64, i64, i32, i32, i32)* @f2, !1}
!7 = !{void (i64, i64, i32, i32, i32)* @f3, !1}
!9 = !{void (i64, i64, i32, i32, i32)* @f4, !1}
!11 = !{void (i64, i64, i32, i32, i32)* @f5, !1}
!13 = !{void (i64, i64, i32, i32, i32, i32, i32, i32)* @f6, !1}
!15 = !{void (i64, i64, i32, i32, i32, i32, i32, i32)* @f7, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}