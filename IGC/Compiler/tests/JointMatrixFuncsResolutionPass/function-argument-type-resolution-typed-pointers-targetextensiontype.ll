;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --typed-pointers -platformpvc -igc-joint-matrix-resolution -S %s 2>&1 | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

%jm = type { target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2) }

; Test case 1:
; function itself does not have arguments to resolve
; there are instructions before calls of functions, which require resolving
; function parameters needs resolving and are using values from these instructions
; there are other instructions after calls of resolved functions which are using result of resolved function
define spir_kernel void @main(i32 %a, i32 %b, float addrspace(1)* %dst0) {
; CHECK: define spir_kernel void @main(i32 %a, i32 %b, float addrspace(1)* %dst0) {
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <16 x float>
; CHECK-NEXT:    [[TC1:%.*]] = alloca [[JM_RESOLVED:%.*]]
; CHECK-NEXT:    [[TC23:%.*]] = alloca [[JM_RESOLVED]]
; CHECK-NEXT:    [[JUSTINT:%.*]] = alloca i32
; CHECK-NEXT:    [[JUSTINT2:%.*]] = alloca i32
; CHECK-NEXT:    call spir_func void @test_resolved(%jm.resolved* [[TC1]])
; CHECK-NEXT:    call spir_func void @test2_resolved(%jm.resolved* [[TC1]], %jm.resolved* [[TC23]])
; CHECK-NEXT:    call spir_func void @test3_resolved(i32* [[JUSTINT]], i32* [[JUSTINT2]], %jm.resolved* [[TC23]])
; CHECK-NEXT:    call spir_func void @test3_resolved(i32* [[JUSTINT]], i32* [[JUSTINT2]], %jm.resolved* [[TC23]])
; CHECK-NEXT:    [[R5:%.*]] = call spir_func %jm.resolved* @test4_resolved(%jm.resolved* [[TC1]], %jm.resolved* [[TC23]], i32 [[A:%.*]], i32 [[B:%.*]])
; CHECK-NEXT:    [[R17:%.*]] = bitcast %jm.resolved* [[R5]] to <16 x float>*
; CHECK-NEXT:    [[R29:%.*]] = load <16 x float>, <16 x float>* [[R17]]
; CHECK-NEXT:    store <16 x float> [[R29]], <16 x float>* [[TMP1]]
; CHECK-NEXT:    [[TMP2:%.*]] = bitcast <16 x float>* [[TMP1]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_global_pi64_v8i8(float addrspace(1)* [[DST0:%.*]], i8* [[TMP2]], i64 16, i32 0)
; CHECK-NEXT:    [[TC311:%.*]] = alloca %jm.resolved*
; CHECK-NEXT:    store %jm.resolved* [[R5]], %jm.resolved** [[TC311]]
; CHECK-NEXT:    ret void
;
  %tC = alloca %jm, align 8
  %tC2 = alloca %jm, align 8
  %justInt = alloca i32, align 8
  %justInt2 = alloca i32, align 8

  call spir_func void @test(%jm* %tC)
  call spir_func void @test2(%jm* %tC, %jm* %tC2)
  call spir_func void @test3(i32* %justInt, i32* %justInt2, %jm* %tC2)
  call spir_func void @test3(i32* %justInt, i32* %justInt2, %jm* %tC2)

  %r = call spir_func %jm* @test4(%jm* %tC, %jm* %tC2, i32 %a, i32 %b)
  %r1 = bitcast %jm* %r to target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2)*
  %r2 = load target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2), target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2)* %r1
  call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2liii(float addrspace(1)* %dst0, target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2) %r2, i64 16, i32 0, i32 3, i32 0)
  %tC3 = alloca %jm*
  store %jm* %r, %jm** %tC3

  ret void
}

; Test case 2:
; test that calls inside were resolved correctly when signature was resolved
define spir_func void @test(%jm* %this) {
; CHECK: define spir_func void @test_resolved(%jm.resolved* %this) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = alloca <16 x float>
; CHECK-NEXT:    [[JM_PTR2:%.*]] = alloca %jm.resolved*
; CHECK-NEXT:    [[INDEX3:%.*]] = getelementptr %jm.resolved*, %jm.resolved** [[JM_PTR2]], i32 5
; CHECK-NEXT:    store %jm.resolved* [[THIS:%.*]], %jm.resolved** [[INDEX3]]
; CHECK-NEXT:    [[DST:%.*]] = alloca float
; CHECK-NEXT:    [[DST0:%.*]] = addrspacecast float* [[DST]] to float addrspace(1)*
; CHECK-NEXT:    store <16 x float> <float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00>, <16 x float>* [[TMP0]]
; CHECK-NEXT:    [[TMP1:%.*]] = bitcast <16 x float>* [[TMP0]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_global_pi64_v8i8(float addrspace(1)* [[DST0]], i8* [[TMP1]], i64 16, i32 0)
; CHECK-NEXT:    ret void
;
entry:
  %jm.ptr = alloca %jm*
  %index = getelementptr %jm*, %jm** %jm.ptr, i32 5
  store %jm* %this, %jm** %index
  %0 = call spir_func target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2) @_Z26__spirv_CompositeConstructf(float 5.000000e+00)
  %dst = alloca float
  %dst0 = addrspacecast float* %dst to float addrspace(1)*
  call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2liii(float addrspace(1)* %dst0, target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2) %0, i64 16, i32 0, i32 3, i32 0)
  ret void
}

; Test case 3:
; tests that function with resolved signature which calls function with resolved signature
; is resolved correctly
define spir_func void @test2(%jm* %this, %jm* %this2) {
; CHECK: define spir_func void @test2_resolved(%jm.resolved* %this, %jm.resolved* %this2) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    call spir_func void @test_resolved(%jm.resolved* [[THIS:%.*]])
; CHECK-NEXT:    [[TC:%.*]] = alloca %jm.resolved
; CHECK-NEXT:    call spir_func void @test_resolved(%jm.resolved* [[TC]])
; CHECK-NEXT:    ret void
;
entry:
  call spir_func void @test(%jm* %this)
  %tC = alloca %jm
  call spir_func void @test(%jm* %tC)
  ret void
}

; CHECK: define spir_func void @test3_resolved(i32* %this, i32* %this2, %jm.resolved* %this3) {
define spir_func void @test3(i32* %this, i32* %this2, %jm* %this3) {
entry:
  ret void
}


define spir_func %jm* @test4(%jm* %this1, %jm* %this2, i32 %a, i32 %b) {
; CHECK: define spir_func %jm.resolved* @test4_resolved(%jm.resolved* %this1, %jm.resolved* %this2, i32 %a, i32 %b) {
; CHECK:       end:
; CHECK-NEXT:    [[RETVAL1:%.*]] = phi %jm.resolved* [ [[THIS1:%.*]], %btrue ], [ [[THIS2:%.*]], %bfalse ]
; CHECK-NEXT:    ret %jm.resolved* [[RETVAL1]]
;
entry:
  %0 = icmp sgt i32 %a, %b
  br i1 %0, label %btrue, label %bfalse

btrue:                                      ; preds = %2
  br label %end

bfalse:                                     ; preds = %2
  br label %end

end:                                     ; preds = %btrue, %bfalse

  %retval = phi %jm* [%this1, %btrue], [%this2, %bfalse]

  ret %jm* %retval
}

declare spir_func target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2) @_Z26__spirv_CompositeConstructf(float)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2liii(float addrspace(1)*, target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2), i64, i32, i32, i32)

!igc.functions = !{!0}
!0 = !{void (i32 , i32 , float addrspace(1)* )* @main, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
