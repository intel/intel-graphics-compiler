;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
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
; Before:
;   void foo()
;   {
;     st_foo a;
;     bar_0(&a)
;     consume(&a);
;   }
;
;   void bar_0(st_foo* out)
;   {
;     bar_1(out);
;   }
;
;   void bar_1(st_foo* out)
;   {
;     *out = {1, 2};
;   }
; After:
;   void foo()
;   {
;     st_foo a;
;     a = bar_0();
;     consume(&a);
;   }
;
;   st_foo bar_0()
;   {
;     return bar_1();
;   }
;
;   st_foo bar_1(st_foo* in)
;   {
;     st_foo out = {1, 2};
;     return out;
;   }

%struct._st_foo = type { i32, i32 }

define spir_kernel void @foo() #1 {
; CHECK:      define spir_kernel void @foo()
    %a = alloca %struct._st_foo
; CHECK-NEXT:     [[A:%.*]] = alloca %struct._st_foo, align 8
    call void @bar_0(ptr sret(%struct._st_foo) %a)
; CHECK-NEXT:     [[RET_BAR_0:%.*]] = call %struct._st_foo @bar_0()
; CHECK-NEXT:     [[M_0_PTR:%.*]] = getelementptr inbounds %struct._st_foo, ptr [[A]], i32 0, i32 0
; CHECK-NEXT:     [[M_0:%.*]] = extractvalue %struct._st_foo [[RET_BAR_0]], 0
; CHECK-NEXT:     store i32 [[M_0]], ptr [[M_0_PTR]], align 4
; CHECK-NEXT:     [[M_1_PTR:%.*]] = getelementptr inbounds %struct._st_foo, ptr [[A]], i32 0, i32 1
; CHECK-NEXT:     [[M_1:%.*]] = extractvalue %struct._st_foo [[RET_BAR_0]], 1
; CHECK-NEXT:     store i32 [[M_1]], ptr [[M_1_PTR]], align 4
    call void @consume(ptr %a)
; CHECK-NEXT:     call void @consume(ptr [[A]])
    ret void
; CHECK-NEXT:     ret void
}
; CHECK-NEXT: }

define linkonce_odr spir_func void @bar_0(ptr sret(%struct._st_foo) %out) {
; CHECK:      define linkonce_odr spir_func %struct._st_foo @bar_0()
; CHECK-NEXT:   [[OUT:%.*]] = alloca %struct._st_foo, align 8
    call void @bar_1(ptr sret(%struct._st_foo) %out)
; CHECK-NEXT:     [[RET_BAR_1:%.*]] = call %struct._st_foo @bar_1()
; CHECK-NEXT:     [[M_0_PTR:%.*]] = getelementptr inbounds %struct._st_foo, ptr [[OUT]], i32 0, i32 0
; CHECK-NEXT:     [[M_0:%.*]] = extractvalue %struct._st_foo [[RET_BAR_1]], 0
; CHECK-NEXT:     store i32 [[M_0]], ptr [[M_0_PTR]], align 4
; CHECK-NEXT:     [[M_1_PTR:%.*]] = getelementptr inbounds %struct._st_foo, ptr [[OUT]], i32 0, i32 1
; CHECK-NEXT:     [[M_1:%.*]] = extractvalue %struct._st_foo [[RET_BAR_1]], 1
; CHECK-NEXT:     store i32 [[M_1]], ptr [[M_1_PTR]], align 4
    ret void
; CHECK-NEXT:   [[M_0_PTR:%.*]] = getelementptr inbounds %struct._st_foo, ptr [[OUT]], i32 0, i32 0
; CHECK-NEXT:   [[M_0:%.*]] = load i32, ptr [[M_0_PTR]], align 4
; CHECK-NEXT:   [[R_0:%.*]] = insertvalue %struct._st_foo undef, i32 [[M_0]], 0
; CHECK-NEXT:   [[M_1_PTR:%.*]] = getelementptr inbounds %struct._st_foo, ptr [[OUT]], i32 0, i32 1
; CHECK-NEXT:   [[M_1:%.*]] = load i32, ptr [[M_1_PTR]], align 4
; CHECK-NEXT:   [[R_1:%.*]] = insertvalue %struct._st_foo [[R_0]], i32 [[M_1]], 1
; CHECK-NEXT:   ret %struct._st_foo [[R_1]]
}
; CHECK-NEXT: }

define linkonce_odr spir_func void @bar_1(ptr sret(%struct._st_foo) %out) {
; CHECK:      define linkonce_odr spir_func %struct._st_foo @bar_1()
; CHECK-NEXT:   [[OUT:%.*]] = alloca %struct._st_foo, align 8
    store %struct._st_foo { i32 1, i32 2 }, ptr %out, align 4
; CHECK-NEXT:   store %struct._st_foo { i32 1, i32 2 }, ptr [[OUT]], align 4
    ret void
; CHECK-NEXT:   [[M_0_PTR:%.*]] = getelementptr inbounds %struct._st_foo, ptr [[OUT]], i32 0, i32 0
; CHECK-NEXT:   [[M_0:%.*]] = load i32, ptr [[M_0_PTR]], align 4
; CHECK-NEXT:   [[R_0:%.*]] = insertvalue %struct._st_foo undef, i32 [[M_0]], 0
; CHECK-NEXT:   [[M_1_PTR:%.*]] = getelementptr inbounds %struct._st_foo, ptr [[OUT]], i32 0, i32 1
; CHECK-NEXT:   [[M_1:%.*]] = load i32, ptr [[M_1_PTR]], align 4
; CHECK-NEXT:   [[R_1:%.*]] = insertvalue %struct._st_foo [[R_0]], i32 [[M_1]], 1
; CHECK-NEXT:   ret %struct._st_foo [[R_1]]
}
; CHECK-NEXT: }

declare spir_func void @consume(ptr)
