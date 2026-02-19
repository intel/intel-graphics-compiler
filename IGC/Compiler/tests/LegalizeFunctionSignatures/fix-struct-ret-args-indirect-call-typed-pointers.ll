;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-legalize-function-signatures -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LegalizeFunctionSignatures
; ------------------------------------------------

; Pseudo-code:
; Before:
; void foo() {
;   st_foo a;
;   void (*func)(st_foo*) = condition ? bar_0 : bar_1;
;   func(&a);
;   consume(&a);
; }
;
; void bar_0(st_foo* out) {
;   *out = {10, 20};
; }
;
; void bar_1(st_foo* out) {
;   *out = {1, 2};
; }
;
; After:
; void foo() {
;   st_foo a;
;   st_foo (*func)() = condition ? bar_0 : bar_1;
;   a = func();
;   consume(&a);
; }
; st_foo bar_0()
; {
;   st_foo out = {10, 20};
;   return out;
; }
; st_foo bar_1()
; {
;   st_foo out = {1, 2};
;   return out;
; }

%struct._st_foo = type { i32, i32 }

define spir_kernel void @foo(i1 %condition) #1 {
; CHECK:      define spir_kernel void @foo(i8 %condition)
; CHECK: [[CONDITION:%.*]] = trunc i8 %condition to i1
    %a = alloca %struct._st_foo
; CHECK-NEXT:     [[A:%.*]] = alloca %struct._st_foo, align 8
    %func_addr = select i1 %condition, void (%struct._st_foo*)* @bar_0, void (%struct._st_foo*)* @bar_1
; CHECK-NEXT: [[FUNC_ADDR_SELECT:%.*]] = select i1 [[CONDITION]], void (%struct._st_foo*)* bitcast (%struct._st_foo ()* @bar_0 to void (%struct._st_foo*)*), void (%struct._st_foo*)* bitcast (%struct._st_foo ()* @bar_1 to void (%struct._st_foo*)*)
; CHECK-NEXT: [[FUNC_ADDR:%.*]] = bitcast void (%struct._st_foo*)* [[FUNC_ADDR_SELECT]] to %struct._st_foo ()*
    call void %func_addr(%struct._st_foo* sret(%struct._st_foo) %a)
; CHECK-NEXT:     [[RET_BAR_0:%.*]] = call %struct._st_foo [[FUNC_ADDR]]()
; CHECK-NEXT:     [[M_0_PTR:%.*]] = getelementptr inbounds %struct._st_foo, %struct._st_foo* [[A]], i32 0, i32 0
; CHECK-NEXT:     [[M_0:%.*]] = extractvalue %struct._st_foo [[RET_BAR_0]], 0
; CHECK-NEXT:     store i32 [[M_0]], i32* [[M_0_PTR]], align 4
; CHECK-NEXT:     [[M_1_PTR:%.*]] = getelementptr inbounds %struct._st_foo, %struct._st_foo* [[A]], i32 0, i32 1
; CHECK-NEXT:     [[M_1:%.*]] = extractvalue %struct._st_foo [[RET_BAR_0]], 1
; CHECK-NEXT:     store i32 [[M_1]], i32* [[M_1_PTR]], align 4
    call void @consume(%struct._st_foo* %a)
; CHECK-NEXT:     call void @consume(%struct._st_foo* [[A]])
    ret void
; CHECK-NEXT:     ret void
}
; CHECK-NEXT: }

define linkonce_odr spir_func void @bar_0(%struct._st_foo* sret(%struct._st_foo) %out) {
; CHECK:      define linkonce_odr spir_func %struct._st_foo @bar_0()
; CHECK-NEXT:   [[OUT:%.*]] = alloca %struct._st_foo, align 8
    store %struct._st_foo { i32 10, i32 20 }, %struct._st_foo* %out, align 4
; CHECK-NEXT:   store %struct._st_foo { i32 10, i32 20 }, %struct._st_foo* [[OUT]], align 4
    ret void
; CHECK-NEXT:   [[M_0_PTR:%.*]] = getelementptr inbounds %struct._st_foo, %struct._st_foo* [[OUT]], i32 0, i32 0
; CHECK-NEXT:   [[M_0:%.*]] = load i32, i32* [[M_0_PTR]], align 4
; CHECK-NEXT:   [[R_0:%.*]] = insertvalue %struct._st_foo undef, i32 [[M_0]], 0
; CHECK-NEXT:   [[M_1_PTR:%.*]] = getelementptr inbounds %struct._st_foo, %struct._st_foo* [[OUT]], i32 0, i32 1
; CHECK-NEXT:   [[M_1:%.*]] = load i32, i32* [[M_1_PTR]], align 4
; CHECK-NEXT:   [[R_1:%.*]] = insertvalue %struct._st_foo [[R_0]], i32 [[M_1]], 1
; CHECK-NEXT:   ret %struct._st_foo [[R_1]]
}
; CHECK-NEXT: }

define linkonce_odr spir_func void @bar_1(%struct._st_foo* sret(%struct._st_foo) %out) {
; CHECK:      define linkonce_odr spir_func %struct._st_foo @bar_1()
; CHECK-NEXT:   [[OUT:%.*]] = alloca %struct._st_foo, align 8
    store %struct._st_foo { i32 1, i32 2 }, %struct._st_foo* %out, align 4
; CHECK-NEXT:   store %struct._st_foo { i32 1, i32 2 },  %struct._st_foo* [[OUT]], align 4
    ret void
; CHECK-NEXT:   [[M_0_PTR:%.*]] = getelementptr inbounds %struct._st_foo,  %struct._st_foo* [[OUT]], i32 0, i32 0
; CHECK-NEXT:   [[M_0:%.*]] = load i32, i32* [[M_0_PTR]], align 4
; CHECK-NEXT:   [[R_0:%.*]] = insertvalue %struct._st_foo undef, i32 [[M_0]], 0
; CHECK-NEXT:   [[M_1_PTR:%.*]] = getelementptr inbounds %struct._st_foo,  %struct._st_foo* [[OUT]], i32 0, i32 1
; CHECK-NEXT:   [[M_1:%.*]] = load i32, i32* [[M_1_PTR]], align 4
; CHECK-NEXT:   [[R_1:%.*]] = insertvalue %struct._st_foo [[R_0]], i32 [[M_1]], 1
; CHECK-NEXT:   ret %struct._st_foo [[R_1]]
}
; CHECK-NEXT: }

declare spir_func void @consume(%struct._st_foo*)
