;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-legalize-function-signatures -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LegalizeFunctionSignatures
; ------------------------------------------------
; This test is to check if large structures returned by function are consumed as by-ref arguments.


; Pseudo-code:
; Before:
; void foo()
; {
;   st_foo_large a = bar_0();
;   consume_large(a);
;   st_foo_small b = bar_1();
;   consume_small(b);
;   st_foo_complex c = bar_2();
;   consume_small(c);
;   array<i32, 2> d = bar_3();
;   consume_small(d);
;   st_foo_large e = bar_no_stack();
;   consume_large(e);
; }
; After:
; void foo()
; {
;   st_foo_large a;
;   bar_0(&a);
;   consume_large(a);
;   st_foo_small b = bar_1();
;   consume_small(b);
;   st_foo_complex c;
;   bar_2(&c);
;   consume_small(c);
;   array<i32, 2> d;
;   bar_3(&d);
;   consume_array(d);
;   st_foo_large e = bar_no_stack();
;   consume_large(e);
; }
;

%struct._st_foo_large = type { <16 x i32>, <16 x i32>, <16 x i32> }
%struct._st_foo_small = type { i32, i32 }
%struct._st_foo_complex = type { i32, %struct._st_foo_small }

define spir_kernel void @foo() #1 {
; CHECK-LABEL: define spir_kernel void @foo()
    %a = call %struct._st_foo_large @bar_0()
; CHECK:      [[A_ALLOCA:%.*]] = alloca %struct._st_foo_large, align 64
; CHECK-NEXT: call void @bar_0(ptr {{.*}} sret(%struct._st_foo_large) [[A_ALLOCA]])
; CHECK-NEXT: [[A_VAL:%.*]] = load %struct._st_foo_large, ptr [[A_ALLOCA]], align 64

    call void @consume_large(%struct._st_foo_large %a)
; CHECK-NEXT: call void @consume_large(%struct._st_foo_large [[A_VAL]])

    %b = call %struct._st_foo_small @bar_1()
; CHECK-NEXT: [[B:%.*]] = call %struct._st_foo_small @bar_1()

    call void @consume_small(%struct._st_foo_small %b)
; CHECK-NEXT: call void @consume_small(%struct._st_foo_small [[B]])

    %c = call %struct._st_foo_complex @bar_2()
; CHECK-NEXT: [[C_ALLOCA:%.*]] = alloca %struct._st_foo_complex, align 8
; CHECK-NEXT: call void @bar_2(ptr {{.*}} sret(%struct._st_foo_complex) [[C_ALLOCA]])
; CHECK-NEXT: [[C_VAL:%.*]] = load %struct._st_foo_complex, ptr [[C_ALLOCA]], align 4

    call void @consume_complex(%struct._st_foo_complex %c)
; CHECK-NEXT: call void @consume_complex(%struct._st_foo_complex [[C_VAL]])

    %d = call [2 x i32] @bar_3()
; CHECK-NEXT: [[D_ALLOCA:%.*]] = alloca [2 x i32], align 4
; CHECK-NEXT: call void @bar_3(ptr noalias sret([2 x i32]) [[D_ALLOCA]])
; CHECK-NEXT: [[D_VAL:%.*]] = load [2 x i32], ptr [[D_ALLOCA]], align 4

    call void @consume_array([2 x i32] %d)
; CHECK-NEXT: call void @consume_array([2 x i32] [[D_VAL]])

    %e = call %struct._st_foo_large @bar_no_stack()
; CHECK-NEXT: %e = call %struct._st_foo_large @bar_no_stack()
    call void @consume_large(%struct._st_foo_large %a)
; CHECK-NEXT: call void @consume_large(%struct._st_foo_large %2)

    ret void
; CHECK-NEXT: ret void
}

define internal spir_func %struct._st_foo_large @bar_0() #0 {
; CHECK:       define internal spir_func void @bar_0(ptr {{.*}} sret(%struct._st_foo_large) [[BAR_0_ARG:%.*]]) #0 {
  ret %struct._st_foo_large zeroinitializer
; CHECK-NEXT: [[BAR_0_ALLOCA:%.*]] = alloca %struct._st_foo_large, align 64
; CHECK-NEXT: store %struct._st_foo_large zeroinitializer, ptr [[BAR_0_ALLOCA]], align 64
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 8 [[BAR_0_ARG]], ptr align 8 [[BAR_0_ALLOCA]], i64 192, i1 false)
; CHECK-NEXT: ret void
}

define internal spir_func %struct._st_foo_small @bar_1() #0 {
  ret %struct._st_foo_small zeroinitializer
}

define internal spir_func %struct._st_foo_complex @bar_2() #0 {
; CHECK:       define internal spir_func void @bar_2(ptr {{.*}} sret(%struct._st_foo_complex) [[BAR_2_ARG:%.*]]) #0 {
  ret %struct._st_foo_complex zeroinitializer
; CHECK-NEXT: [[BAR_2_ALLOCA:%.*]] = alloca %struct._st_foo_complex, align 8
; CHECK-NEXT: store %struct._st_foo_complex zeroinitializer, ptr [[BAR_2_ALLOCA]], align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 8 [[BAR_2_ARG]], ptr align 8 [[BAR_2_ALLOCA]], i64 12, i1 false)
; CHECK-NEXT: ret void
}

define internal spir_func [2 x i32] @bar_3() #0 {
; CHECK:       define internal spir_func void @bar_3(ptr {{.*}} sret([2 x i32]) [[BAR_3_ARG:%.*]]) #0 {
  ret [2 x i32] zeroinitializer
; CHECK-NEXT: [[BAR_3_ALLOCA:%.*]] = alloca [2 x i32], align 4
; CHECK-NEXT: store [2 x i32] zeroinitializer, ptr [[BAR_3_ALLOCA]], align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 8 [[BAR_3_ARG]], ptr align 8 [[BAR_3_ALLOCA]], i64 8, i1 false)
; CHECK-NEXT: ret void
}

define internal spir_func %struct._st_foo_large @bar_no_stack() {
  ret %struct._st_foo_large zeroinitializer
}

declare spir_func void @consume_large(%struct._st_foo_large)
declare spir_func void @consume_small(%struct._st_foo_small)
declare spir_func void @consume_complex(%struct._st_foo_complex)
declare spir_func void @consume_array([2 x i32])

attributes #0 = { "visaStackCall" }
