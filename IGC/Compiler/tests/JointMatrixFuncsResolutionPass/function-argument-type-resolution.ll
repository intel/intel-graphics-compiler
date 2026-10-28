;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -platformpvc -igc-joint-matrix-resolution -S %s 2>&1 | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------
; With opaque pointers the function arguments are plain "ptr" and carry no
; matrix type, so no function gets a resolved clone (*_resolved). Matrix types
; and operations are resolved inside the function bodies instead.

%jm = type { target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2) }

; Test case 1:
; function itself does not have arguments to resolve
; allocas of matrix types before the calls are resolved, and the calls still
; target the original functions because their "ptr" parameters need no resolving
; the matrix loaded from the call result after the calls is resolved
define spir_kernel void @main(i32 %a, i32 %b, ptr %dst0) {
; CHECK-LABEL: define spir_kernel void @main(i32 %a, i32 %b, ptr %dst0) {
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <16 x float>
; CHECK-NEXT:    [[C:%.*]] = alloca ptr addrspace(2)
; CHECK-NEXT:    [[TC1:%.*]] = alloca %jm.resolved
; CHECK-NEXT:    [[TC23:%.*]] = alloca %jm.resolved
; CHECK-NEXT:    [[JUSTINT:%.*]] = alloca i32
; CHECK-NEXT:    [[JUSTINT2:%.*]] = alloca i32
; CHECK-NEXT:    call spir_func void @test(ptr [[TC1]])
; CHECK-NEXT:    call spir_func void @test2(ptr [[TC1]], ptr [[TC23]])
; CHECK-NEXT:    call spir_func void @test3(ptr [[JUSTINT]], ptr [[JUSTINT2]], ptr [[TC23]])
; CHECK-NEXT:    call spir_func void @test3(ptr [[JUSTINT]], ptr [[JUSTINT2]], ptr [[TC23]])
; CHECK-NEXT:    [[R:%.*]] = call spir_func ptr @test4(ptr [[TC1]], ptr [[TC23]], i32 %a, i32 %b)
; CHECK-NEXT:    [[R2:%.*]] = load <16 x float>, ptr [[R]]
; CHECK-NEXT:    store <16 x float> [[R2]], ptr [[TMP1]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_generic_pi64_v8i8(ptr %dst0, ptr [[TMP1]], i64 16, i32 0)
; CHECK-NEXT:    [[TC3:%.*]] = alloca ptr
; CHECK-NEXT:    store ptr [[R]], ptr [[TC3]]
; CHECK-NEXT:    ret void
;
  %c = alloca target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2) addrspace(2)*
  %tC = alloca %jm, align 8
  %tC2 = alloca %jm, align 8
  %justInt = alloca i32, align 8
  %justInt2 = alloca i32, align 8

  call spir_func void @test(ptr %tC)
  call spir_func void @test2(ptr %tC, ptr %tC2)
  call spir_func void @test3(ptr %justInt, ptr %justInt2, ptr %tC2)
  call spir_func void @test3(ptr %justInt, ptr %justInt2, ptr %tC2)

  %r = call spir_func ptr @test4(ptr %tC, ptr %tC2, i32 %a, i32 %b)
  %r2 = load target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2), ptr %r
  call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2liii(ptr %dst0, target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2) %r2, i64 16, i32 0, i32 3, i32 0)
  %tC3 = alloca ptr
  store ptr %r, ptr %tC3

  ret void
}

; Test case 2:
; test that matrix operations inside are resolved while the signature stays unchanged
define spir_func void @test(ptr %this) {
; CHECK-LABEL: define spir_func void @test(ptr %this) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = alloca <16 x float>
; CHECK-NEXT:    [[JM_PTR:%.*]] = alloca ptr
; CHECK-NEXT:    [[INDEX:%.*]] = getelementptr ptr, ptr [[JM_PTR]], i32 5
; CHECK-NEXT:    store ptr %this, ptr [[INDEX]]
; CHECK-NEXT:    [[DST:%.*]] = alloca float
; CHECK-NEXT:    store <16 x float> {{splat \(float 5.000000e\+00\)|<float 5.000000e\+00, .*>}}, ptr [[TMP0]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_generic_pi64_v8i8(ptr [[DST]], ptr [[TMP0]], i64 16, i32 0)
; CHECK-NEXT:    ret void
;
entry:
  %jm.ptr = alloca ptr
  %index = getelementptr ptr, ptr %jm.ptr, i32 5
  store ptr %this, ptr %index
  %0 = call spir_func target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2) @_Z26__spirv_CompositeConstructf(float 5.000000e+00)
  %dst = alloca float
  call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2liii(ptr %dst, target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2) %0, i64 16, i32 0, i32 3, i32 0)
  ret void
}

; Test case 3:
; tests that a function calling another function with "ptr" parameters keeps
; calling the original function and resolves its local matrix alloca
define spir_func void @test2(ptr %this, ptr %this2) {
; CHECK-LABEL: define spir_func void @test2(ptr %this, ptr %this2) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    call spir_func void @test(ptr %this)
; CHECK-NEXT:    [[TC:%.*]] = alloca %jm.resolved
; CHECK-NEXT:    call spir_func void @test(ptr [[TC]])
; CHECK-NEXT:    ret void
;
entry:
  call spir_func void @test(ptr %this)
  %tC = alloca %jm
  call spir_func void @test(ptr %tC)
  ret void
}

; Test case 4:
; functions with only "ptr" parameters keep their signatures
define spir_func void @test3(ptr %this, ptr %this2, ptr %this3) {
; CHECK-LABEL: define spir_func void @test3(ptr %this, ptr %this2, ptr %this3) {
entry:
  ret void
}


define spir_func ptr @test4(ptr %this1, ptr %this2, i32 %a, i32 %b) {
; CHECK-LABEL: define spir_func ptr @test4(ptr %this1, ptr %this2, i32 %a, i32 %b) {
; CHECK:       end:
; CHECK-NEXT:    [[RETVAL:%.*]] = phi ptr [ %this1, %btrue ], [ %this2, %bfalse ]
; CHECK-NEXT:    ret ptr [[RETVAL]]
;
entry:
  %0 = icmp sgt i32 %a, %b
  br i1 %0, label %btrue, label %bfalse

btrue:                                      ; preds = %2
  br label %end

bfalse:                                     ; preds = %2
  br label %end

end:                                     ; preds = %btrue, %bfalse
  %retval = phi ptr [%this1, %btrue], [%this2, %bfalse]
  ret ptr %retval
}

declare spir_func target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2) @_Z26__spirv_CompositeConstructf(float)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2liii(ptr, target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2), i64, i32, i32, i32)

!igc.functions = !{!0}
!0 = !{ptr @main, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
