;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

%struct1 = type { <256 x float> }
%struct2 = type { %struct1 }
%struct3 = type { %struct2 }
%struct4 = type { i32, i32, [1 x [7 x %struct3]] }

; CHECK: [[ALLOCA_F:[^ ]+]] = alloca [1 x [7 x %struct3]], align 1024
; CHECK: [[GEP3_SPLIT:[^ ]+]] = getelementptr [1 x [7 x %struct3]], [1 x [7 x %struct3]]* [[ALLOCA_F]], i64 0, i64 0, i64 %arg1
; CHECK: [[GEP4:[^ ]+]] = getelementptr inbounds %struct3, %struct3* [[GEP3_SPLIT]], i64 0, i32 0, i32 0, i32 0
; CHECK: store <256 x float> zeroinitializer, <256 x float>* [[GEP4]], align 1024
define void @test(i64 %arg1) {
  %alloca = alloca %struct4, align 1024
  %gep1 = getelementptr inbounds %struct4, %struct4* %alloca, i64 0, i32 0
  store i32 0, i32 * %gep1, align 1024
  %gep2 = getelementptr inbounds %struct4, %struct4* %alloca, i64 0, i32 1
  store i32 0, i32* %gep2, align 4
  %gep3 = getelementptr inbounds %struct4, %struct4* %alloca, i64 0, i32 2, i64 0, i64 %arg1
  %gep4 = getelementptr inbounds %struct3, %struct3* %gep3, i64 0, i32 0, i32 0, i32 0
  store <256 x float> zeroinitializer, <256 x float>* %gep4, align 1024
  ret void
}
