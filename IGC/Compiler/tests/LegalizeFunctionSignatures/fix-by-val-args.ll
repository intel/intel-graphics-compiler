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


; Pseudo-code:
; void foo(bool condition)
; {
;   st_foo a{1, 2};
;   int(*(st_foo*)) func = condition ? bar_0 : bar_1;
;   consume(func(&a));
;   consume(bar_2(&a));
;   consume(bar_3(&a));
; }
;
; void bar_0(st_foo* in)
; {
;   return in->a + in->b;
; }
;
; void bar_1(st_foo* in)
; {
;   return in->a + in->b;
; }
;
; void bar_2(st_foo* in);
;
; void bar_3(st_foo* in)
; {
;   return in->a + in->b;
; }

%struct._st_foo = type { i32, i32 }

define spir_kernel void @foo(i1 %condition) #1 {
; CHECK: define spir_kernel void @foo(i8 %condition)
; CHECK: [[CONDITION:%.*]] = trunc i8 %condition to i1
    %a = alloca %struct._st_foo
; CHECK-NEXT: [[A:%.*]] = alloca %struct._st_foo
    store %struct._st_foo { i32 1, i32 2 }, ptr %a, align 4
; CHECK-NEXT: store %struct._st_foo { i32 1, i32 2 }, ptr [[A]], align 4
    %func_addr = select i1 %condition, ptr @bar_0, ptr @bar_1
; CHECK-NEXT: [[FUNC_ADDR:%.*]] = select i1 [[CONDITION]], ptr @bar_0, ptr @bar_1
    %res_indirect = call i32 %func_addr(ptr byval(%struct._st_foo) %a)
; Prepare struct values
; CHECK-NEXT: [[MEMBER_0_ADDR_0:%.*]] = getelementptr inbounds %struct._st_foo, ptr [[A]], i32 0, i32 0
; CHECK-NEXT: [[MEMBER_0_VAL_0:%.*]] = load i32, ptr [[MEMBER_0_ADDR_0]], align 4
; CHECK-NEXT: [[STRUCT_VAL_0_0:%.*]] = insertvalue %struct._st_foo undef, i32 [[MEMBER_0_VAL_0]], 0
; CHECK-NEXT: [[MEMBER_1_ADDR_0:%.*]] = getelementptr inbounds %struct._st_foo, ptr [[A]], i32 0, i32 1
; CHECK-NEXT: [[MEMBER_1_VAL_0:%.*]] = load i32, ptr [[MEMBER_1_ADDR_0]], align 4
; CHECK-NEXT: [[STRUCT_VAL_1_0:%.*]] = insertvalue %struct._st_foo [[STRUCT_VAL_0_0]], i32 [[MEMBER_1_VAL_0]], 1
; call fixed function
; CHECK-NEXT: [[RES_VAL_0:%.*]] = call i32  [[FUNC_ADDR]](%struct._st_foo [[STRUCT_VAL_1_0]])
    call void @consume(i32 %res_indirect)
; CHECK-NEXT: call void @consume(i32 [[RES_VAL_0]])
    %res_direct_0 = call i32 @bar_2(ptr byval(%struct._st_foo) %a)
; CHECK-NEXT: %res_direct_0 = call i32 @bar_2(ptr byval(%struct._st_foo) %a)
    call void @consume(i32 %res_direct_0)
; CHECK-NEXT: call void @consume(i32 %res_direct_0)
    %res_direct_1 = call i32 @bar_3(ptr byval(%struct._st_foo) %a)
; Prepare struct values
; CHECK-NEXT: [[MEMBER_0_ADDR_1:%.*]] = getelementptr inbounds %struct._st_foo, ptr [[A]], i32 0, i32 0
; CHECK-NEXT: [[MEMBER_0_VAL_1:%.*]] = load i32, ptr [[MEMBER_0_ADDR_1]], align 4
; CHECK-NEXT: [[STRUCT_VAL_0_1:%.*]] = insertvalue %struct._st_foo undef, i32 [[MEMBER_0_VAL_1]], 0
; CHECK-NEXT: [[MEMBER_1_ADDR_1:%.*]] = getelementptr inbounds %struct._st_foo, ptr [[A]], i32 0, i32 1
; CHECK-NEXT: [[MEMBER_1_VAL_1:%.*]] = load i32, ptr [[MEMBER_1_ADDR_1]], align 4
; CHECK-NEXT: [[STRUCT_VAL_1_1:%.*]] = insertvalue %struct._st_foo [[STRUCT_VAL_0_1]], i32 [[MEMBER_1_VAL_1]], 1
; call fixed function
; CHECK-NEXT: [[RES_VAL_1:%.*]] = call i32 @bar_3(%struct._st_foo [[STRUCT_VAL_1_1]])
    call void @consume(i32 %res_direct_1)
; CHECK-NEXT: call void @consume(i32 [[RES_VAL_1]])

    ret void
}

define internal spir_func i32 @bar_0(ptr byval(%struct._st_foo) %s) #1 {
; CHECK: define internal spir_func i32 @bar_0(%struct._st_foo [[ARG_0:%.*]])
entry:
; an internal variable
; CHECK: [[A_0:%.*]] = alloca %struct._st_foo, align 8
; store the argument to the internal variable
; CHECK-NEXT: [[B0_MEMBER_0_ADDR:%.*]] = getelementptr inbounds %struct._st_foo, ptr [[A_0]], i32 0, i32 0
; CHECK-NEXT: [[B0_MEMBER_0_VAL:%.*]] = extractvalue %struct._st_foo [[ARG_0]], 0
; CHECK-NEXT: store i32 [[B0_MEMBER_0_VAL]], ptr [[B0_MEMBER_0_ADDR]], align 4
; CHECK-NEXT: [[B0_MEMBER_1_ADDR:%.*]] = getelementptr inbounds %struct._st_foo, ptr [[A_0]], i32 0, i32 1
; CHECK-NEXT: [[B0_MEMBER_1_VAL:%.*]] = extractvalue %struct._st_foo [[ARG_0]], 1
; CHECK-NEXT: store i32 [[B0_MEMBER_1_VAL]], ptr [[B0_MEMBER_1_ADDR]], align 4
  %a = getelementptr inbounds %struct._st_foo, ptr %s, i32 0, i32 0
; CHECK: %a = getelementptr inbounds %struct._st_foo, ptr [[A_0]], i32 0, i32 0
  %0 = load i32, ptr %a, align 4
  %b = getelementptr inbounds %struct._st_foo, ptr %s, i32 0, i32 1
; CHECK: %b = getelementptr inbounds %struct._st_foo, ptr [[A_0]], i32 0, i32 1
  %1 = load i32, ptr %b, align 4
  %add = add nsw i32 %0, %1
  ret i32 %add
}

define internal spir_func i32 @bar_1(ptr byval(%struct._st_foo) %s) #2 {
; CHECK: define internal spir_func i32 @bar_1(%struct._st_foo [[ARG_1:%.*]])
entry:
; an internal variable
; CHECK: [[A_1:%.*]] = alloca %struct._st_foo, align 8
; store the argument to the internal variable
; CHECK-NEXT: [[B1_MEMBER_0_ADDR:%.*]] = getelementptr inbounds %struct._st_foo, ptr [[A_1]], i32 0, i32 0
; CHECK-NEXT: [[B1_MEMBER_0_VAL:%.*]] = extractvalue %struct._st_foo [[ARG_1]], 0
; CHECK-NEXT: store i32 [[B1_MEMBER_0_VAL]], ptr [[B1_MEMBER_0_ADDR]], align 4
; CHECK-NEXT: [[B1_MEMBER_1_ADDR:%.*]] = getelementptr inbounds %struct._st_foo, ptr [[A_1]], i32 0, i32 1
; CHECK-NEXT: [[B1_MEMBER_1_VAL:%.*]] = extractvalue %struct._st_foo [[ARG_1]], 1
; CHECK-NEXT: store i32 [[B1_MEMBER_1_VAL]], ptr [[B1_MEMBER_1_ADDR]], align 4
  %a = getelementptr inbounds %struct._st_foo, ptr %s, i32 0, i32 0
; CHECK: %a = getelementptr inbounds %struct._st_foo, ptr [[A_0]], i32 0, i32 0
  %0 = load i32, ptr %a, align 4
  %b = getelementptr inbounds %struct._st_foo, ptr %s, i32 0, i32 1
; CHECK: %b = getelementptr inbounds %struct._st_foo, ptr [[A_0]], i32 0, i32 1
  %1 = load i32, ptr %b, align 4
  %add = add nsw i32 %0, %1
  ret i32 %add
}

declare spir_func i32 @bar_2(ptr byval(%struct._st_foo) %s) #0

define internal spir_func i32 @bar_3(ptr byval(%struct._st_foo) %s) {
; CHECK: define internal spir_func i32 @bar_3(%struct._st_foo [[ARG_3:%.*]])
entry:
; an internal variable
; CHECK: [[A_3:%.*]] = alloca %struct._st_foo, align 8
; store the argument to the internal variable
; CHECK-NEXT: [[B3_MEMBER_0_ADDR:%.*]] = getelementptr inbounds %struct._st_foo, ptr [[A_3]], i32 0, i32 0
; CHECK-NEXT: [[B3_MEMBER_0_VAL:%.*]] = extractvalue %struct._st_foo [[ARG_3]], 0
; CHECK-NEXT: store i32 [[B3_MEMBER_0_VAL]], ptr [[B3_MEMBER_0_ADDR]], align 4
; CHECK-NEXT: [[B3_MEMBER_1_ADDR:%.*]] = getelementptr inbounds %struct._st_foo, ptr [[A_3]], i32 0, i32 1
; CHECK-NEXT: [[B3_MEMBER_1_VAL:%.*]] = extractvalue %struct._st_foo [[ARG_3]], 1
; CHECK-NEXT: store i32 [[B3_MEMBER_1_VAL]], ptr [[B3_MEMBER_1_ADDR]], align 4
  %a = getelementptr inbounds %struct._st_foo, ptr %s, i32 0, i32 0
; CHECK: %a = getelementptr inbounds %struct._st_foo, ptr [[A_0]], i32 0, i32 0
  %0 = load i32, ptr %a, align 4
  %b = getelementptr inbounds %struct._st_foo, ptr %s, i32 0, i32 1
; CHECK: %b = getelementptr inbounds %struct._st_foo, ptr [[A_0]], i32 0, i32 1
  %1 = load i32, ptr %b, align 4
  %add = add nsw i32 %0, %1
  ret i32 %add
}

declare spir_func void @consume(i32)


attributes #0 = { "visaStackCall" }
attributes #1 = { "referenced-indirectly" }
attributes #2 = { "referenced-indirectly" "visaStackCall" }
