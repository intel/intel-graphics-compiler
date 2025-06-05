;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-16-plus

; RUN: igc_opt --opaque-pointers -platformpvc -igc-joint-matrix-resolution -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------
; A shorten version of Matrix/element_wise_all_ops_half.cpp (only compare OP remains)

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%"class.sycl::_V1::range" = type { %"class.sycl::_V1::detail::array" }
%"class.sycl::_V1::detail::array" = type { [2 x i64] }
%"class.sycl::_V1::detail::half_impl::half" = type { half }

; Function Attrs: nounwind
define spir_kernel void @_ZTS5logicILm8ELm16EE(ptr addrspace(1) align 2 %_arg_accA, ptr byval(%"class.sycl::_V1::range") align 8 %_arg_accA2, ptr byval(%"class.sycl::_V1::range") align 8 %_arg_accA3, i64 %_arg_sg_size) #0 !kernel_arg_addr_space !7 !kernel_arg_access_qual !8 !kernel_arg_type !9 !kernel_arg_type_qual !10 !kernel_arg_base_type !9 !kernel_arg_name !374 !spirv.ParameterDecorations !375 {
entry:
  %agg.tmp.i.i = alloca %"class.sycl::_V1::detail::half_impl::half", align 2, !spirv.Decorations !376
  %sub_a.i = alloca target("spirv.CooperativeMatrixKHR", half, 3, 8, 16, 0), align 8, !spirv.Decorations !381
  %0 = bitcast ptr %_arg_accA2 to ptr
  %agg.tmp5.sroa.0.sroa.2.0._arg_accA2.ascast.sroa_idx = getelementptr inbounds i8, ptr %0, i64 8
  %1 = bitcast ptr %agg.tmp5.sroa.0.sroa.2.0._arg_accA2.ascast.sroa_idx to ptr
  %agg.tmp5.sroa.0.sroa.2.0.copyload = load i64, ptr %1, align 8
  %2 = bitcast ptr %_arg_accA3 to ptr
  %agg.tmp6.sroa.0.sroa.0.0.copyload = load i64, ptr %2, align 8
  %3 = bitcast ptr %_arg_accA3 to ptr
  %agg.tmp6.sroa.0.sroa.2.0._arg_accA3.ascast.sroa_idx = getelementptr inbounds i8, ptr %3, i64 8
  %4 = bitcast ptr %agg.tmp6.sroa.0.sroa.2.0._arg_accA3.ascast.sroa_idx to ptr
  %agg.tmp6.sroa.0.sroa.2.0.copyload = load i64, ptr %4, align 8
  %sub_a.ascast.i = addrspacecast ptr %sub_a.i to ptr addrspace(4)
  %5 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 1) #3
  %cmp.i16.i = icmp ult i64 %5, 2147483648
  call void @llvm.assume(i1 %cmp.i16.i)
  %6 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0) #3
  %cmp.i.i = icmp ult i64 %6, 2147483648
  call void @llvm.assume(i1 %cmp.i.i)
  %7 = call spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 1) #3
  %cmp.i24.i = icmp ult i64 %7, 2147483648
  call void @llvm.assume(i1 %cmp.i24.i)
  %8 = call spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 0) #3
  %cmp.i20.i = icmp ult i64 %8, 2147483648

; CHECK: [[ALLOCA1:%.*]] = alloca <8 x i16>, align 8, !spirv.Decorations
; CHECK: [[ASCAST_1:%.*]] = addrspacecast ptr [[ALLOCA1]] to ptr addrspace(4)

  call void @llvm.assume(i1 %cmp.i20.i)
  %9 = bitcast ptr %sub_a.i to ptr
  call void @llvm.lifetime.start.p0(i64 8, ptr %9)
  %10 = bitcast ptr %agg.tmp.i.i to ptr
  call void @llvm.lifetime.start.p0(i64 2, ptr %10)
  %11 = bitcast ptr %agg.tmp.i.i to ptr
  store half 0xH4500, ptr %11, align 2
  %call.i28.i = call spir_func target("spirv.CooperativeMatrixKHR", half, 3, 8, 16, 0) @"_Z26__spirv_CompositeConstructP40class.sycl::_V1::detail::half_impl::half"(ptr %agg.tmp.i.i) #0
  store target("spirv.CooperativeMatrixKHR", half, 3, 8, 16, 0) %call.i28.i, ptr %sub_a.i, align 8
  %12 = bitcast ptr %agg.tmp.i.i to ptr
  call void @llvm.lifetime.end.p0(i64 2, ptr %12)
  br label %for.cond.i

for.cond.i:                                       ; preds = %_ZZ4testIN4sycl3_V16detail9half_impl4halfEfLm8ELm16EEvvENKUlRS4_E3_clES5_.exit, %entry
  %13 = phi target("spirv.CooperativeMatrixKHR", half, 3, 8, 16, 0) [ %call.i28.i, %entry ], [ %.pre, %_ZZ4testIN4sycl3_V16detail9half_impl4halfEfLm8ELm16EEvvENKUlRS4_E3_clES5_.exit ]
  %i.0.i = phi i32 [ 0, %entry ], [ %inc.i, %_ZZ4testIN4sycl3_V16detail9half_impl4halfEfLm8ELm16EEvvENKUlRS4_E3_clES5_.exit ]
  %conv.i = zext i32 %i.0.i to i64
  %call.i23 = call spir_func i64 @_Z34__spirv_CooperativeMatrixLengthKHRPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_16_0(target("spirv.CooperativeMatrixKHR", half, 3, 8, 16, 0) %13) #0
  %cmp.i = icmp ugt i64 %call.i23, %conv.i
  br i1 %cmp.i, label %for.body.i, label %_ZN4sycl3_V13ext6oneapi12experimental6matrix18joint_matrix_applyINS0_9sub_groupENS0_6detail9half_impl4halfELNS4_3useE0ELm8ELm16ELNS4_6layoutE0ERKZ4testIS9_fLm8ELm16EEvvEUlRS9_E3_EEvT_RNS4_12joint_matrixISH_T0_XT1_EXT2_EXT3_EXT4_EEEOT5_.exit

; CHECK: [[LOAD_1:%.*]] = load <8 x i16>, ptr addrspace(4) [[ASCAST_1]]
; CHECK: [[EXTRACT:%.*]] = extractelement <8 x i16> [[LOAD_1]], i64 %{{.*}}
; CHECK: [[BITCAST_1:%.*]] = bitcast i16 [[EXTRACT]] to half, !joint_matrix_apply !{{.*}}
; CHECK: fcmp une half [[BITCAST_1]], 0xH0000

for.body.i:                                       ; preds = %for.cond.i
  %call.i = call spir_func ptr addrspace(4) @_Z19__spirv_AccessChainPU3AS4PU3AS143__spirv_CooperativeMatrixKHR__half_3_8_16_0l(ptr addrspace(4) %sub_a.ascast.i, i64 %conv.i) #0
  %14 = load half, ptr addrspace(4) %call.i, align 2
  %tobool.i = fcmp une half %14, 0xH0000
  br i1 %tobool.i, label %if.then.i, label %_ZZ4testIN4sycl3_V16detail9half_impl4halfEfLm8ELm16EEvvENKUlRS4_E3_clES5_.exit

if.then.i:                                        ; preds = %for.body.i
  %or.cond68.i = fcmp ord half %14, 0xH0000
  %cmp.i52.i = fcmp ole half %14, 0xH4000
  %or.cond69.i = or i1 %or.cond68.i, %cmp.i52.i
  br i1 %or.cond69.i, label %if.then14.i, label %_ZZ4testIN4sycl3_V16detail9half_impl4halfEfLm8ELm16EEvvENKUlRS4_E3_clES5_.exit

if.then14.i:                                      ; preds = %if.then.i
  %cmp.i54.i = fcmp une half %14, 0xH4000
  %15 = fadd half %14, 0xHBC00
  %16 = fadd half %15, 0xH3C00
  %add.i.i.i.i = select i1 %cmp.i54.i, half %16, half 0xH4000
  %cmp.i58.i = fcmp oeq half %14, 0xH4000
  %sub.i.i = fadd half %add.i.i.i.i, 0xHC000
  %mul.i.i = fmul half %sub.i.i, 0xH4200
  %div.i.i = fmul half %mul.i.i, 0xH3800
  %add.i.i = fadd half %add.i.i.i.i, 0xH4000
  %val.sroa.0.1.i = select i1 %cmp.i58.i, half %div.i.i, half %add.i.i
  br label %_ZZ4testIN4sycl3_V16detail9half_impl4halfEfLm8ELm16EEvvENKUlRS4_E3_clES5_.exit

; CHECK: [[LOAD_2:%.*]] = load <8 x i16>, ptr addrspace(4) [[ASCAST_1]]
; CHECK: [[BITCAST_2:%.*]] = bitcast half %{{.*}} to i16
; CHECK: [[INSERT:%.*]] = insertelement <8 x i16> [[LOAD_2]], i16 [[BITCAST_2]], i64 %{{.*}}
; CHECK: store <8 x i16> [[INSERT]], ptr addrspace(4) %{{.*}}

_ZZ4testIN4sycl3_V16detail9half_impl4halfEfLm8ELm16EEvvENKUlRS4_E3_clES5_.exit: ; preds = %if.then14.i, %if.then.i, %for.body.i
  %element.i.sroa.0.0 = phi half [ %val.sroa.0.1.i, %if.then14.i ], [ %14, %if.then.i ], [ %14, %for.body.i ]
  %call.i26 = call spir_func ptr addrspace(4) @_Z19__spirv_AccessChainPU3AS4PU3AS143__spirv_CooperativeMatrixKHR__half_3_8_16_0l(ptr addrspace(4) %sub_a.ascast.i, i64 %conv.i) #0
  store half %element.i.sroa.0.0, ptr addrspace(4) %call.i26, align 2
  %inc.i = add nuw nsw i32 %i.0.i, 1, !spirv.Decorations !382
  %.pre = load target("spirv.CooperativeMatrixKHR", half, 3, 8, 16, 0), ptr %sub_a.i, align 8
  br label %for.cond.i

_ZN4sycl3_V13ext6oneapi12experimental6matrix18joint_matrix_applyINS0_9sub_groupENS0_6detail9half_impl4halfELNS4_3useE0ELm8ELm16ELNS4_6layoutE0ERKZ4testIS9_fLm8ELm16EEvvEUlRS9_E3_EEvT_RNS4_12joint_matrixISH_T0_XT1_EXT2_EXT3_EXT4_EEEOT5_.exit: ; preds = %for.cond.i
  %sub5.i = sub nsw i64 %6, %8, !spirv.Decorations !385
  %sub.i = sub nsw i64 %5, %7, !spirv.Decorations !385
  %mul.i6.i.i.i.i = mul i64 %agg.tmp6.sroa.0.sroa.0.0.copyload, %agg.tmp5.sroa.0.sroa.2.0.copyload
  %17 = getelementptr %"class.sycl::_V1::detail::half_impl::half", ptr addrspace(1) %_arg_accA, i64 %mul.i6.i.i.i.i
  %add.ptr.i = getelementptr %"class.sycl::_V1::detail::half_impl::half", ptr addrspace(1) %17, i64 %agg.tmp6.sroa.0.sroa.2.0.copyload
  %add.i7.i.i.i.i.i = add i64 %mul.i6.i.i.i.i, %agg.tmp6.sroa.0.sroa.2.0.copyload
  %idx.neg.i.i = sub i64 0, %add.i7.i.i.i.i.i
  %add.ptr.i.i = getelementptr inbounds %"class.sycl::_V1::detail::half_impl::half", ptr addrspace(1) %add.ptr.i, i64 %idx.neg.i.i
  %add.ptr.i27.idx = shl nsw i64 %sub.i, 9, !spirv.Decorations !385
  %18 = bitcast ptr addrspace(1) %add.ptr.i.i to ptr addrspace(1)
  %add.ptr.i27 = getelementptr inbounds i8, ptr addrspace(1) %18, i64 %add.ptr.i27.idx
  %div.i = udiv i64 %sub5.i, %_arg_sg_size
  %add.ptr.i28.idx = shl i64 %div.i, 5
  %add.ptr.i28 = getelementptr inbounds i8, ptr addrspace(1) %add.ptr.i27, i64 %add.ptr.i28.idx
  %19 = load target("spirv.CooperativeMatrixKHR", half, 3, 8, 16, 0), ptr %sub_a.i, align 8
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1cPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_16_0ili(ptr addrspace(1) %add.ptr.i28, target("spirv.CooperativeMatrixKHR", half, 3, 8, 16, 0) %19, i32 0, i64 32, i32 0) #0
  %20 = bitcast ptr %sub_a.i to ptr
  call void @llvm.lifetime.end.p0(i64 8, ptr %20)
  ret void
}

; Function Attrs: nounwind
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1cPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_16_0ili(ptr addrspace(1), target("spirv.CooperativeMatrixKHR", half, 3, 8, 16, 0), i32, i64, i32) #0


; Function Attrs: nounwind
declare spir_func i64 @_Z34__spirv_CooperativeMatrixLengthKHRPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_16_0(target("spirv.CooperativeMatrixKHR", half, 3, 8, 16, 0)) #0

; Function Attrs: nounwind
declare spir_func ptr addrspace(4) @_Z19__spirv_AccessChainPU3AS4PU3AS143__spirv_CooperativeMatrixKHR__half_3_8_16_0l(ptr addrspace(4), i64) #0

; Function Attrs: nounwind willreturn memory(none)
declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32) #3

; Function Attrs: nounwind willreturn memory(none)
declare spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32) #3

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: readwrite)
declare void @llvm.assume(i1 noundef) #1

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture) #2

; Function Attrs: nounwind
declare spir_func target("spirv.CooperativeMatrixKHR", half, 3, 8, 16, 0) @"_Z26__spirv_CompositeConstructP40class.sycl::_V1::detail::half_impl::half"(ptr) #0

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture) #2


!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!spirv.Generator = !{!2}
!igc.functions = !{}

!0 = !{i32 2, i32 2}
!1 = !{i32 4, i32 100000}
!2 = !{i16 6, i16 14}
!4 = !{!5}
!5 = !{!"function_type", i32 0}
!6 = !{i32 1, i32 0, i32 0, i32 0}
!7 = !{!"none", !"none", !"none", !"none"}
!8 = !{!"class.sycl::_V1::detail::half_impl::half*", !"class.sycl::_V1::range", !"class.sycl::_V1::range", !"long"}
!9 = !{!"", !"", !"", !""}
!10 = !{!"_arg_accA", !"_arg_accA2", !"_arg_accA4", !"_arg_sg_size"}
!374 = !{!375, !377, !377, !4}
!375 = !{!376}
!376 = !{i32 44, i32 2}
!377 = !{!378, !379}
!378 = !{i32 38, i32 2}
!379 = !{i32 44, i32 8}
!380 = !{!379}
!381 = !{!382, !383}
!382 = !{i32 4469}
!383 = !{i32 4470}
!384 = !{!382}
!385 = !{!383}
