;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus
; UNSUPPORTED: llvm-17-plus

; RUN: igc_opt --typed-pointers -platformpvc -igc-joint-matrix-resolution -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------
; A shorten version of Matrix/element_wise_all_ops_half.cpp (only compare OP remains)

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%"class.sycl::_V1::detail::half_impl::half" = type { half }
%"class.sycl::_V1::range" = type { %"class.sycl::_V1::detail::array" }
%"class.sycl::_V1::detail::array" = type { [2 x i64] }
%spirv.CooperativeMatrixKHR._half_3_8_16_0 = type opaque
%"struct.sycl::_V1::ext::oneapi::experimental::matrix::joint_matrix" = type { %spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)* }
%structtype = type { i16 }

; Function Attrs: nounwind
define spir_kernel void @_ZTS5logicILm8ELm16EE(%"class.sycl::_V1::detail::half_impl::half" addrspace(1)* align 2 %_arg_accA, %"class.sycl::_V1::range"* byval(%"class.sycl::_V1::range") align 8 %_arg_accA2, %"class.sycl::_V1::range"* byval(%"class.sycl::_V1::range") align 8 %_arg_accA4, i64 %_arg_sg_size) !kernel_arg_addr_space !6 !kernel_arg_access_qual !7 !kernel_arg_type !8 !kernel_arg_type_qual !9 !kernel_arg_base_type !8 !kernel_arg_name !10 {
entry:
  %0 = alloca %spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)*, align 8
  %agg.tmp.i26 = alloca %"class.sycl::_V1::detail::half_impl::half", align 2
  %1 = bitcast %"class.sycl::_V1::range"* %_arg_accA2 to i64*
  %agg.tmp7.sroa.0.sroa.2.0._arg_accA2.ascast.sroa_idx = getelementptr inbounds i64, i64* %1, i64 1
  %agg.tmp7.sroa.0.sroa.2.0.copyload = load i64, i64* %agg.tmp7.sroa.0.sroa.2.0._arg_accA2.ascast.sroa_idx, align 8
  %2 = bitcast %"class.sycl::_V1::range"* %_arg_accA4 to i64*
  %agg.tmp8.sroa.0.sroa.0.0.copyload = load i64, i64* %2, align 8
  %3 = bitcast %"class.sycl::_V1::range"* %_arg_accA4 to i64*
  %agg.tmp8.sroa.0.sroa.2.0._arg_accA4.ascast.sroa_idx = getelementptr inbounds i64, i64* %3, i64 1
  %agg.tmp8.sroa.0.sroa.2.0.copyload = load i64, i64* %agg.tmp8.sroa.0.sroa.2.0._arg_accA4.ascast.sroa_idx, align 8
  %4 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 1)
  %cmp.i17 = icmp ult i64 %4, 2147483648
  %5 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %cmp.i = icmp ult i64 %5, 2147483648
  %6 = call spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 1)
  %cmp.i24 = icmp ult i64 %6, 2147483648
  %7 = call spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 0)
  %cmp.i20 = icmp ult i64 %7, 2147483648

; CHECK: [[SLICE:%.*]] = bitcast <8 x i16>* %{{.*}} to %"struct.sycl::_V1::ext::oneapi::experimental::matrix::joint_matrix.resolved"*
; CHECK: [[RES:%.*]] = getelementptr inbounds %"struct.sycl::_V1::ext::oneapi::experimental::matrix::joint_matrix.resolved", %"struct.sycl::_V1::ext::oneapi::experimental::matrix::joint_matrix.resolved"* [[SLICE]], i64 0, i32 0
; CHECK: [[ASCAST_1:%.*]] = addrspacecast <8 x i16>* [[RES]] to <8 x i16> addrspace(4)*
; CHECK: [[ASCAST_2:%.*]] = addrspacecast <8 x i16>* [[RES]] to <8 x i16> addrspace(4)*

  %8 = bitcast %spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)** %0 to i8*
  %9 = bitcast %"class.sycl::_V1::detail::half_impl::half"* %agg.tmp.i26 to i8*
  %Data.i = getelementptr inbounds %"class.sycl::_V1::detail::half_impl::half", %"class.sycl::_V1::detail::half_impl::half"* %agg.tmp.i26, i64 0, i32 0
  store half 0xH4500, half* %Data.i, align 2
  %call.i29 = call spir_func %spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)* @"_Z26__spirv_CompositeConstructP40class.sycl::_V1::detail::half_impl::half"(%"class.sycl::_V1::detail::half_impl::half"* %agg.tmp.i26)
  %10 = bitcast %spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)** %0 to %"struct.sycl::_V1::ext::oneapi::experimental::matrix::joint_matrix"*
  %spvm.i = getelementptr inbounds %"struct.sycl::_V1::ext::oneapi::experimental::matrix::joint_matrix", %"struct.sycl::_V1::ext::oneapi::experimental::matrix::joint_matrix"* %10, i64 0, i32 0
  %11 = addrspacecast %spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)** %spvm.i to %spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)* addrspace(4)*
  %12 = addrspacecast %spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)** %spvm.i to %spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)* addrspace(4)*
  store %spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)* %call.i29, %spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)** %spvm.i, align 8
  %13 = bitcast %"class.sycl::_V1::detail::half_impl::half"* %agg.tmp.i26 to i8*
  br label %for.cond.i

for.cond.i:                                       ; preds = %_ZZ4testIN4sycl3_V16detail9half_impl4halfEfLm8ELm16EEvvENKUlRS4_E_clES5_.exit, %entry
  %14 = phi %spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)* [ %call.i29, %entry ], [ %.pre, %_ZZ4testIN4sycl3_V16detail9half_impl4halfEfLm8ELm16EEvvENKUlRS4_E_clES5_.exit ]
  %i.0.i = phi i32 [ 0, %entry ], [ %inc.i, %_ZZ4testIN4sycl3_V16detail9half_impl4halfEfLm8ELm16EEvvENKUlRS4_E_clES5_.exit ]
  %conv.i34 = sext i32 %i.0.i to i64
  %call.i = call spir_func i64 @_Z34__spirv_CooperativeMatrixLengthKHRPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_16_0(%spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)* %14)
  %cmp.i36 = icmp ugt i64 %call.i, %conv.i34
  br i1 %cmp.i36, label %for.body.i, label %_ZN4sycl3_V13ext6oneapi12experimental6matrix18joint_matrix_applyINS0_9sub_groupENS0_6detail9half_impl4halfELNS4_3useE0ELm8ELm16ELNS4_6layoutE0ERKZ4testIS9_fLm8ELm16EEvvEUlRS9_E_EEvT_RNS4_12joint_matrixISH_T0_XT1_EXT2_EXT3_EXT4_EEEOT5_.exit

; CHECK: [[LOAD_1:%.*]] = load <8 x i16>, <8 x i16> addrspace(4)* [[ASCAST_1]]
; CHECK: [[EXTRACT:%.*]] = extractelement <8 x i16> [[LOAD_1]], i64 %{{.*}}
; CHECK: [[BITCAST_1:%.*]] = bitcast i16 [[EXTRACT]] to half, !joint_matrix_apply !{{.*}}
; CHECK: fcmp une half [[BITCAST_1]], 0xH0000

for.body.i:                                       ; preds = %for.cond.i
  %call.i62 = call spir_func %structtype addrspace(4)* @_Z19__spirv_AccessChainPU3AS4PU3AS143__spirv_CooperativeMatrixKHR__half_3_8_16_0l(%spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)* addrspace(4)* %11, i64 %conv.i34)
  %15 = getelementptr inbounds %structtype, %structtype addrspace(4)* %call.i62, i64 0, i32 0
  %16 = bitcast i16 addrspace(4)* %15 to half addrspace(4)*
  %17 = load half, half addrspace(4)* %16, align 2
  %tobool.i = fcmp une half %17, 0xH0000
  br i1 %tobool.i, label %if.then.i, label %_ZZ4testIN4sycl3_V16detail9half_impl4halfEfLm8ELm16EEvvENKUlRS4_E_clES5_.exit

if.then.i:                                        ; preds = %for.body.i
  %or.cond96.i = fcmp ord half %17, 0xH0000
  %cmp.i62.i = fcmp ole half %17, 0xH4000
  %or.cond97.i = or i1 %or.cond96.i, %cmp.i62.i
  br i1 %or.cond97.i, label %if.then14.i, label %_ZZ4testIN4sycl3_V16detail9half_impl4halfEfLm8ELm16EEvvENKUlRS4_E_clES5_.exit

if.then14.i:                                      ; preds = %if.then.i
  %cmp.i75.i = fcmp oeq half %17, 0xH4000
  %18 = fmul reassoc nsz arcp contract half %17, 0xH3E00
  %div.i.i = fadd reassoc nsz arcp contract half %18, 0xHC200
  %add.i.i = fadd reassoc nsz arcp contract half %17, 0xH4000
  %19 = select i1 %cmp.i75.i, half %div.i.i, half %add.i.i
  br label %_ZZ4testIN4sycl3_V16detail9half_impl4halfEfLm8ELm16EEvvENKUlRS4_E_clES5_.exit

; CHECK: [[LOAD_2:%.*]] = load <8 x i16>, <8 x i16> addrspace(4)* [[ASCAST_2]]
; CHECK: [[BITCAST_2:%.*]] = bitcast half %{{.*}} to i16
; CHECK: [[INSERT:%.*]] = insertelement <8 x i16> [[LOAD_2]], i16 [[BITCAST_2]], i64 %{{.*}}
; CHECK: store <8 x i16> [[INSERT]], <8 x i16> addrspace(4)* %{{.*}}

_ZZ4testIN4sycl3_V16detail9half_impl4halfEfLm8ELm16EEvvENKUlRS4_E_clES5_.exit: ; preds = %if.then14.i, %if.then.i, %for.body.i
  %element.i.sroa.0.0 = phi half [ %19, %if.then14.i ], [ %17, %if.then.i ], [ %17, %for.body.i ]
  %call.i69 = call spir_func %structtype addrspace(4)* @_Z19__spirv_AccessChainPU3AS4PU3AS143__spirv_CooperativeMatrixKHR__half_3_8_16_0l(%spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)* addrspace(4)* %12, i64 %conv.i34)
  %20 = bitcast %structtype addrspace(4)* %call.i69 to half addrspace(4)*
  store half %element.i.sroa.0.0, half addrspace(4)* %20, align 2
  %inc.i = add nuw nsw i32 %i.0.i, 1
  %.pre = load %spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)*, %spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)** %spvm.i, align 8
  br label %for.cond.i

_ZN4sycl3_V13ext6oneapi12experimental6matrix18joint_matrix_applyINS0_9sub_groupENS0_6detail9half_impl4halfELNS4_3useE0ELm8ELm16ELNS4_6layoutE0ERKZ4testIS9_fLm8ELm16EEvvEUlRS9_E_EEvT_RNS4_12joint_matrixISH_T0_XT1_EXT2_EXT3_EXT4_EEEOT5_.exit: ; preds = %for.cond.i
  ret void
}

; Function Attrs: nounwind
declare spir_func %spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)* @"_Z26__spirv_CompositeConstructP40class.sycl::_V1::detail::half_impl::half"(%"class.sycl::_V1::detail::half_impl::half"* %0)

; Function Attrs: nounwind
declare spir_func i64 @_Z34__spirv_CooperativeMatrixLengthKHRPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_16_0(%spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)* %0)

; Function Attrs: nounwind
declare spir_func %structtype addrspace(4)* @_Z19__spirv_AccessChainPU3AS4PU3AS143__spirv_CooperativeMatrixKHR__half_3_8_16_0l(%spirv.CooperativeMatrixKHR._half_3_8_16_0 addrspace(1)* addrspace(4)* %0, i64 %1)

; Function Attrs: nounwind readnone willreturn
declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 %0)

; Function Attrs: nounwind readnone willreturn
declare spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 %0)

; Function Attrs: nounwind readnone willreturn
declare spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32 %0)

; Function Attrs: nounwind readnone willreturn
declare spir_func i64 @_Z29__spirv_BuiltInGlobalLinearIdv()

; Function Attrs: nounwind readnone willreturn
declare spir_func i64 @_Z28__spirv_BuiltInWorkgroupSizei(i32 %0)

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!spirv.Generator = !{!2}
!igc.functions = !{!3}

!0 = !{i32 2, i32 2}
!1 = !{i32 4, i32 100000}
!2 = !{i16 6, i16 14}
!3 = !{void (%"class.sycl::_V1::detail::half_impl::half" addrspace(1)*, %"class.sycl::_V1::range"*, %"class.sycl::_V1::range"*, i64)* @_ZTS5logicILm8ELm16EE, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 0}
!6 = !{i32 1, i32 0, i32 0, i32 0}
!7 = !{!"none", !"none", !"none", !"none"}
!8 = !{!"class.sycl::_V1::detail::half_impl::half*", !"class.sycl::_V1::range", !"class.sycl::_V1::range", !"long"}
!9 = !{!"", !"", !"", !""}
!10 = !{!"_arg_accA", !"_arg_accA2", !"_arg_accA4", !"_arg_sg_size"}
