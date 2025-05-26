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

%struct = type { i32, i32 }

; CHECK:      define spir_kernel void @foo()
; CHECK-NEXT:     [[A:%.*]] = alloca %struct, align 8
; CHECK-NEXT:     [[B:%.*]] = alloca %struct, align 8

; CHECK-NEXT:     [[PTR0:%.*]]      = getelementptr inbounds %struct, ptr [[B]], i32 0, i32 0
; CHECK-NEXT:     [[VALUE0:%.*]]    = load i32, ptr [[PTR0]], align 4
; CHECK-NEXT:     [[VALUE1:%.*]]    = insertvalue %struct undef, i32 [[VALUE0]], 0

; CHECK-NEXT:     [[PTR1:%.*]]      = getelementptr inbounds %struct, ptr [[B]], i32 0, i32 1
; CHECK-NEXT:     [[VALUE2:%.*]]    = load i32, ptr [[PTR1]], align 4
; CHECK-NEXT:     [[VALUE3:%.*]]    = insertvalue %struct [[VALUE1]], i32 [[VALUE2]], 1

; CHECK-NEXT:     [[CALL1:%.*]]     = call %struct @bar_0(%struct %6)
; CHECK-NEXT:     [[PTR2:%.*]]      = getelementptr inbounds %struct, ptr [[A]], i32 0, i32 0
; CHECK-NEXT:     [[VALUE4:%.*]]    = extractvalue %struct [[CALL1]], 0
; CHECK-NEXT:     store i32 [[VALUE4]], ptr [[PTR2]], align 4

; CHECK-NEXT:     [[PTR3:%.*]]      = getelementptr inbounds %struct, ptr [[A]], i32 0, i32 1
; CHECK-NEXT:     [[VALUE5:%.*]]    = extractvalue %struct [[CALL1]], 1
; CHECK-NEXT:     store i32 [[VALUE5]], ptr [[PTR3]], align 4

; CHECK-NEXT:     [[PTR4:%.*]]      = getelementptr inbounds %struct, ptr %b, i32 0, i32 0
; CHECK-NEXT:     [[VALUE5:%.*]]    = load i32, ptr [[PTR4]], align 4
; CHECK-NEXT:     [[VALUE6:%.*]]    = insertvalue %struct undef, i32 [[VALUE5]], 0
; CHECK-NEXT:     [[PTR5:%.*]]      = getelementptr inbounds %struct, ptr %b, i32 0, i32 1
; CHECK-NEXT:     [[VALUE7:%.*]]    = load i32, ptr [[PTR5]], align 4
; CHECK-NEXT:     [[VALUE8:%.*]]    = insertvalue %struct [[VALUE6]], i32 [[VALUE7]], 1
; CHECK-NEXT:     [[CALL2:%.*]]     = call %struct @bar_declaration_only(%struct [[VALUE8]])
; CHECK-NEXT:     [[PTR6:%.*]]      = getelementptr inbounds %struct, ptr %a, i32 0, i32 0
; CHECK-NEXT:     [[VALUE9:%.*]]    = extractvalue %struct [[CALL2]], 0
; CHECK-NEXT:                         store i32 [[VALUE9]], ptr [[PTR6]], align 4
; CHECK-NEXT:     [[PTR7:%.*]]      = getelementptr inbounds %struct, ptr %a, i32 0, i32 1
; CHECK-NEXT:     [[PTR8:%.*]]      = extractvalue %struct [[CALL2]], 1
; CHECK-NEXT:     store i32 [[PTR8]], ptr [[PTR7]], align 4

; CHECK-NEXT:     ret void

; CHECK-NEXT: }

define spir_kernel void @foo() #1 {
    %a = alloca %struct
    %b = alloca %struct
    call void @bar_0(ptr sret(%struct) %a, ptr byval(%struct) %b)
    call void @bar_declaration_only(ptr sret(%struct) %a, ptr byval(%struct) %b)
    ret void
}

; CHECK:      define linkonce_odr spir_func %struct @bar_0(%struct %byval) {
    ; CHECK-NEXT:     [[ALLOCA1:%.*]] = alloca %struct, align 8
    ; CHECK-NEXT:     [[ALLOCA2:%.*]] = alloca %struct, align 8

    ; CHECK-NEXT:     [[PTR0:%.*]] = getelementptr inbounds %struct, ptr [[ALLOCA2]], i32 0, i32 0
    ; CHECK-NEXT:     [[VALUE0:%.*]] = extractvalue %struct %byval, 0
    ; CHECK-NEXT:     store i32 [[VALUE0]], ptr [[PTR0]], align 4

    ; CHECK-NEXT:     [[PTR1:%.*]] = getelementptr inbounds %struct, ptr [[ALLOCA2]], i32 0, i32 1
    ; CHECK-NEXT:     [[VALUE0:%.*]] = extractvalue %struct %byval, 1
    ; CHECK-NEXT:     store i32 [[VALUE0]], ptr [[PTR1]], align 4

    ; CHECK-NEXT:     [[CALL1:%.*]] = call %struct @bar_1()
    ; CHECK-NEXT:     [[PTR2:%.*]] = getelementptr inbounds %struct, ptr [[ALLOCA1]], i32 0, i32 0
    ; CHECK-NEXT:     [[VALUE1:%.*]] = extractvalue %struct [[CALL1]], 0
    ; CHECK-NEXT:     store i32 [[VALUE1]], ptr [[PTR2]], align 4

    ; CHECK-NEXT:     [[PTR3:%.*]] = getelementptr inbounds %struct, ptr [[ALLOCA1]], i32 0, i32 1
    ; CHECK-NEXT:     [[VALUE2:%.*]] = extractvalue %struct [[CALL1]], 1
    ; CHECK-NEXT:     store i32 [[VALUE2]], ptr [[PTR3]], align 4

    ; CHECK-NEXT:     [[ALLOCA3:%.*]] = alloca ptr, align 8
    ; CHECK-NEXT:     store ptr [[ALLOCA2]], ptr [[ALLOCA3]], align 8
    ; CHECK-NEXT:     [[LOAD1:%.*]] = load ptr, ptr [[ALLOCA3]], align 8

    ; CHECK-NEXT:     [[PTR4:%.*]] = getelementptr inbounds %struct, ptr [[LOAD1]], i32 0, i32 1
    ; CHECK-NEXT:     [[LOAD2:%.*]] = load i32, ptr [[PTR4]], align 4
    ; CHECK-NEXT:     [[CALL2:%.*]] = call noundef i32 @consume2(i32 noundef [[LOAD2]])

    ; CHECK-NEXT:     [[PTR5:%.*]] = getelementptr inbounds %struct, ptr [[ALLOCA1]], i32 0, i32 0
    ; CHECK-NEXT:     [[LOAD3:%.*]] = load i32, ptr [[PTR5]], align 4
    ; CHECK-NEXT:     [[VALUE3:%.*]] = insertvalue %struct undef, i32 [[LOAD3]], 0

    ; CHECK-NEXT:     [[PTR6:%.*]] = getelementptr inbounds %struct, ptr [[ALLOCA1]], i32 0, i32 1
    ; CHECK-NEXT:     [[LOAD4:%.*]] = load i32, ptr [[PTR6]], align 4
    ; CHECK-NEXT:     [[LAST_INSERT:%.*]] = insertvalue %struct [[VALUE3]], i32 [[LOAD4]], 1

    ; CHECK-NEXT:     ret %struct [[LAST_INSERT]]
; CHECK-NEXT: }

define linkonce_odr spir_func void @bar_0(ptr sret(%struct) %out, ptr byval(%struct) %byval) {
    call void @bar_1(ptr sret(%struct) %out)
    %1 = alloca ptr, align 8
    store ptr %byval, ptr %1, align 8
    %2 = load ptr, ptr %1, align 8
    %3 = getelementptr inbounds %struct, ptr %2, i32 0, i32 1
    %4 = load i32, ptr %3, align 4
    %5 = call noundef i32 @consume2(i32 noundef %4)
    ret void
}

; CHECK:      define linkonce_odr spir_func %struct @bar_1() {
; CHECK-NEXT:     [[ALLOCA1:%.*]] = alloca %struct, align 8

; CHECK-NEXT:     store %struct { i32 1, i32 2 }, ptr [[ALLOCA1]], align 4

; CHECK-NEXT:     [[M_0_PTR:%.*]] = getelementptr inbounds %struct, ptr [[ALLOCA1]], i32 0, i32 0
; CHECK-NEXT:     [[LOAD1:%.*]] = load i32, ptr [[M_0_PTR]], align 4
; CHECK-NEXT:     [[INSERT1:%.*]] = insertvalue %struct undef, i32 [[LOAD1]], 0

; CHECK-NEXT:     [[M_1_PTR:%.*]] = getelementptr inbounds %struct, ptr [[ALLOCA1]], i32 0, i32 1
; CHECK-NEXT:     [[LOAD2:%.*]] = load i32, ptr [[M_1_PTR]], align 4
; CHECK-NEXT:     [[INSERT2:%.*]] = insertvalue %struct [[INSERT1]], i32 [[LOAD2]], 1

; CHECK-NEXT:     ret %struct [[INSERT2]]

define linkonce_odr spir_func void @bar_1(ptr sret(%struct) %out) {
    store %struct { i32 1, i32 2 }, ptr %out, align 4
    ret void
}

; CHECK:      declare spir_func %struct @bar_declaration_only(%struct) #0
declare spir_func void @bar_declaration_only(ptr sret(%struct) %a, ptr byval(%struct) %b) #5

declare spir_func void @consume(ptr)
declare spir_func void @consume2(i32)

attributes #5 = { nounwind "referenced-indirectly" "visaStackCall" }