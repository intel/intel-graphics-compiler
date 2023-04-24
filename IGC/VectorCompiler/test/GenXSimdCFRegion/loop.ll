;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -simdcf-region -enable-simdcf-transform -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXSimdCFRegion
; ------------------------------------------------
; This test checks that GenXSimdCFRegion generate
; correct for-loop simd llvm-ir (based on ispc test)

; Function Attrs: nounwind
; CHECK: f_f

; CHECK: [[GOTO:%[A-z0-9.]*]] = call {{.*}} @llvm.genx.simdcf.goto
; CHECK: {{.*}} = extractvalue {{.*}} [[GOTO]]

; CHECK: [[JOIN_CALL:%[A-z0-9.]*]] = call {{.*}} @llvm.genx.simdcf.join
; CHECK: %{{.*}} = extractvalue {{.*}} [[JOIN_CALL]]

define spir_kernel void @f_f(i8 addrspace(1)* "VCArgumentIOKind"="0" %RET, i8 addrspace(1)* "VCArgumentIOKind"="0" %aFOO) #1 !intel_reqd_sub_group_size !4 {

allocas:
  %ret_addr = call float* @llvm.genx.address.convert.p0f32.p1i8(i8 addrspace(1)* %RET)
  %data_addr = call float* @llvm.genx.address.convert.p0f32.p1i8(i8 addrspace(1)* %aFOO)
  %i16_allone = call i1 @llvm.genx.any.v16i1(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>)
  br i1 %i16_allone, label %for_loop, label %for_exit

; Loop at depth 1 containing: %for_loop<header>,%vgather_i,%vload_i,%end_loop_exit<latch><exiting>
for_loop:                                         ; preds = %end_loop_exit, %allocas
  %phi_mask = phi <16 x i1> [ %mask_test, %end_loop_exit ], [ <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, %allocas ]
  %phi_inc = phi <16 x i32> [ %increment, %end_loop_exit ], [ <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>, %allocas ]
  %phi_inc_elt = phi i32 [ %increment_elt, %end_loop_exit ], [ 0, %allocas ]
  %phi_sum = phi <16 x float> [ %blend.i, %end_loop_exit ], [ zeroinitializer, %allocas ]
  %i_load6_sext.elt0 = sext i32 %phi_inc_elt to i64
  %ptr = getelementptr float, float* %data_addr, i64 %i_load6_sext.elt0
  %mask_cast = bitcast <16 x i1> %phi_mask to i16
  %mask_cut = and i16 %mask_cast, -32767
  %cmp_and = icmp eq i16 %mask_cut, -32767
  br i1 %cmp_and, label %vload_i, label %vgather_i

vload_i:                                          ; preds = %for_loop
  %bitptr.i.i = bitcast float* %ptr to <16 x float>*
  %res_load = load <16 x float>, <16 x float>* %bitptr.i.i, align 64
  %res_masked = select <16 x i1> %phi_mask, <16 x float> %res_load, <16 x float> undef
  br label %end_loop_exit

vgather_i:                                        ; preds = %for_loop
  %ptr_to_int = ptrtoint float* %ptr to i64
  %base = insertelement <16 x i64> undef, i64 %ptr_to_int, i32 0
  %shuffle = shufflevector <16 x i64> %base, <16 x i64> undef, <16 x i32> zeroinitializer
  %new_offsets = add <16 x i64> %shuffle, <i64 0, i64 4, i64 8, i64 12, i64 16, i64 20, i64 24, i64 28, i64 32, i64 36, i64 40, i64 44, i64 48, i64 52, i64 56, i64 60>
  %res_gather = call <16 x float> @llvm.genx.svm.gather.v16f32.v16i1.v16i64(<16 x i1> %phi_mask, i32 0, <16 x i64> %new_offsets, <16 x float> undef)
  br label %end_loop_exit

end_loop_exit:                                    ; preds = %vgather_i, %vload_i
  %phi_common_ret = phi <16 x float> [ %res_masked, %vload_i ], [ %res_gather, %vgather_i ]
  %sum_exit = fadd <16 x float> %phi_sum, %phi_common_ret
  %blend.i = select <16 x i1> %phi_mask, <16 x float> %sum_exit, <16 x float> %phi_sum
  ; <<<
  %increment_elt = add i32 %phi_inc_elt, 16
  %increment = add <16 x i32> %phi_inc, <i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16>
  %less_i_load_ = icmp ugt <16 x i32> %phi_inc, <i32 2147483631, i32 2147483631, i32 2147483631, i32 2147483631, i32 2147483631, i32 2147483631, i32 2147483631, i32 2147483631, i32 2147483631, i32 2147483631, i32 2147483631, i32 2147483631, i32 2147483631, i32 2147483631, i32 2147483631, i32 2147483631>
  %mask_test = and <16 x i1> %phi_mask, %less_i_load_
  %body_any = call i1 @llvm.genx.any.v16i1(<16 x i1> %mask_test)
  ; <<<  here will be GOTO
  br i1 %body_any, label %for_loop, label %for_exit

; <<< here new bb after loop, but before
; <<< exit-node with JOIN

for_exit:                                         ; preds = %end_loop_exit, %allocas
  %phi_result = phi <16 x float> [ zeroinitializer, %allocas ], [ %blend.i, %end_loop_exit ]
  %v1.i.i = shufflevector <16 x float> %phi_result, <16 x float> undef, <16 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %r.i.i.i = fadd <16 x float> %phi_result, %v1.i.i
  %v2.i.i = shufflevector <16 x float> %r.i.i.i, <16 x float> undef, <16 x i32> <i32 4, i32 5, i32 6, i32 7, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %r.i3.i.i = fadd <16 x float> %r.i.i.i, %v2.i.i
  %v3.i.i = shufflevector <16 x float> %r.i3.i.i, <16 x float> undef, <16 x i32> <i32 2, i32 3, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %r.i2.i.i = fadd <16 x float> %r.i3.i.i, %v3.i.i
  %exit_el0 = extractelement <16 x float> %r.i2.i.i, i32 0
  %exit_el1 = extractelement <16 x float> %r.i2.i.i, i32 1
  %exit_fadd = fadd float %exit_el0, %exit_el1
  %calltmp_broadcast = insertelement <16 x float> undef, float %exit_fadd, i32 0
  %calltmp_broadcast18 = shufflevector <16 x float> %calltmp_broadcast, <16 x float> undef, <16 x i32> zeroinitializer
  %svm_st_ptrtoint = ptrtoint float* %ret_addr to i64
  call void @llvm.genx.svm.block.st.i64.v16f32(i64 %svm_st_ptrtoint, <16 x float> %calltmp_broadcast18)
  ret void
}

declare <16 x float> @llvm.genx.svm.gather.v16f32.v16i1.v16i64(<16 x i1>, i32, <16 x i64>, <16 x float>) #0
declare i1 @llvm.genx.any.v16i1(<16 x i1>) #0
declare void @llvm.genx.svm.block.st.i64.v16f32(i64, <16 x float>) #0
declare float* @llvm.genx.address.convert.p0f32.p1i8(i8 addrspace(1)*) #0

attributes #0 = { "VCFunction" }
attributes #1 = { nounwind "VCFunction" "VCNamedBarrierCount"="0" "VCSLMSize"="0" }

!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!0}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!spirv.Generator = !{!3}

!0 = !{i32 0, i32 0}
!1 = !{i32 1, i32 2}
!2 = !{}
!3 = !{i16 6, i16 14}
!4 = !{i32 1}
