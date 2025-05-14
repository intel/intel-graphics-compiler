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

%struct._st_foo = type { i32, i32 }

define spir_kernel void @foo() #1 {
; CHECK:      define spir_kernel void @foo()
; CHECK-NEXT:     [[A:%.*]] = alloca %struct._st_foo, align 8
; CHECK-NEXT:     call void @bar_0(ptr sret(%struct._st_foo) [[A]])
; CHECK-NEXT:     call void @consume(ptr [[A]])
; CHECK-NEXT:     ret void
    %a = alloca %struct._st_foo
    call void @bar_0(ptr sret(%struct._st_foo) %a)
    call void @consume(ptr %a)
    ret void
}
; CHECK-NEXT: }

define linkonce_odr spir_func void @bar_0(ptr sret(%struct._st_foo) %out) {
; CHECK:      define linkonce_odr spir_func void @bar_0(ptr sret(%struct._st_foo) %out) {
; CHECK-NEXT: call void @bar_1(ptr sret(%struct._st_foo) %out)
; CHECK-NEXT: ret void
    call void @bar_1(ptr sret(%struct._st_foo) %out)
    ret void
}
; CHECK-NEXT: }

define linkonce_odr spir_func void @bar_1(ptr sret(%struct._st_foo) %out) {
; CHECK:      define linkonce_odr spir_func void @bar_1(ptr sret(%struct._st_foo) %out) {
; CHECK-NEXT:   store %struct._st_foo { i32 1, i32 2 }, ptr %out, align 4
; CHECK-NEXT:   ret void
    store %struct._st_foo { i32 1, i32 2 }, ptr %out, align 4
    ret void
}
; CHECK-NEXT: }

declare spir_func void @consume(ptr)
