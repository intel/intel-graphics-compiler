;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-16-plus
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -platformpvc -igc-joint-matrix-resolution -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------
; This is compiled SYCL Matrix/element_wise_ops.cpp test only pvc_uint_8x16x32 case

; CHECK: [[CAST:%.*]] = addrspacecast <8 x i32>* {{.*}} to <8 x i32> addrspace(4)*
; CHECK-LABEL: for.cond.i67
; CHECK: [[IDX_PHI:%.*]] = phi i32
; CHECK: [[IDX:%.*]] = zext i32 [[IDX_PHI]] to i64
; CHECK: [[LOAD_1:%.*]] = load <8 x i32>, <8 x i32> addrspace(4)* [[CAST]], align 8
; CHECK: [[EXTRACT:%.*]] = extractelement <8 x i32> [[LOAD_1]], i64 [[IDX]]
; CHECK: [[SHL:%.*]] = shl nsw i32 [[EXTRACT]], 1
; CHECK: [[LOAD_2:%.*]] = load <8 x i32>, <8 x i32> addrspace(4)* [[CAST]], align 8
; CHECK: [[INSERT:%.*]] = insertelement <8 x i32> [[LOAD_2]], i32 [[SHL]], i64 [[IDX]]
; CHECK: store <8 x i32> [[INSERT]], <8 x i32> addrspace(4)* [[CAST]], align 8

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%"class.sycl::_V1::range" = type { %"class.sycl::_V1::detail::array" }
%"class.sycl::_V1::detail::array" = type { [2 x i64] }

; Function Attrs: nounwind
define spir_kernel void @_ZTSZ4mainE16pvc_uint_8x16x32(i32 addrspace(1)* align 4 %_arg_accC, %"class.sycl::_V1::range"* byval(%"class.sycl::_V1::range") align 8 %_arg_accC2, %"class.sycl::_V1::range"* byval(%"class.sycl::_V1::range") align 8 %_arg_accC3, i64 %_arg_sg_size, i8 addrspace(1)* align 1 %_arg_accA, %"class.sycl::_V1::range"* byval(%"class.sycl::_V1::range") align 8 %_arg_accA5, %"class.sycl::_V1::range"* byval(%"class.sycl::_V1::range") align 8 %_arg_accA6, i8 addrspace(1)* align 1 %_arg_accB, %"class.sycl::_V1::range"* byval(%"class.sycl::_V1::range") align 8 %_arg_accB8, %"class.sycl::_V1::range"* byval(%"class.sycl::_V1::range") align 8 %_arg_accB9) #0 !kernel_arg_addr_space !6 !kernel_arg_access_qual !7 !kernel_arg_type !8 !kernel_arg_type_qual !9 !kernel_arg_base_type !8 !kernel_arg_name !10 !spirv.ParameterDecorations !11 {
entry:
  %0 = alloca target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2), align 8, !spirv.Decorations !20
  %1 = bitcast %"class.sycl::_V1::range"* %_arg_accC2 to i8*
  %agg.tmp11.sroa.0.sroa.2.0._arg_accC2.ascast.sroa_idx = getelementptr inbounds i8, i8* %1, i64 8
  %2 = bitcast i8* %agg.tmp11.sroa.0.sroa.2.0._arg_accC2.ascast.sroa_idx to i64*
  %agg.tmp11.sroa.0.sroa.2.0.copyload = load i64, i64* %2, align 8
  %3 = bitcast %"class.sycl::_V1::range"* %_arg_accC3 to i64*
  %agg.tmp12.sroa.0.sroa.0.0.copyload = load i64, i64* %3, align 8
  %4 = bitcast %"class.sycl::_V1::range"* %_arg_accC3 to i8*
  %agg.tmp12.sroa.0.sroa.2.0._arg_accC3.ascast.sroa_idx = getelementptr inbounds i8, i8* %4, i64 8
  %5 = bitcast i8* %agg.tmp12.sroa.0.sroa.2.0._arg_accC3.ascast.sroa_idx to i64*
  %agg.tmp12.sroa.0.sroa.2.0.copyload = load i64, i64* %5, align 8
  %mul.i6.i.i.i.i = mul i64 %agg.tmp12.sroa.0.sroa.0.0.copyload, %agg.tmp11.sroa.0.sroa.2.0.copyload
  %6 = getelementptr i32, i32 addrspace(1)* %_arg_accC, i64 %mul.i6.i.i.i.i
  %add.ptr.i = getelementptr i32, i32 addrspace(1)* %6, i64 %agg.tmp12.sroa.0.sroa.2.0.copyload
  %7 = bitcast %"class.sycl::_V1::range"* %_arg_accA5 to i8*
  %agg.tmp15.sroa.0.sroa.2.0._arg_accA5.ascast.sroa_idx = getelementptr inbounds i8, i8* %7, i64 8
  %8 = bitcast i8* %agg.tmp15.sroa.0.sroa.2.0._arg_accA5.ascast.sroa_idx to i64*
  %agg.tmp15.sroa.0.sroa.2.0.copyload = load i64, i64* %8, align 8
  %9 = bitcast %"class.sycl::_V1::range"* %_arg_accA6 to i64*
  %agg.tmp16.sroa.0.sroa.0.0.copyload = load i64, i64* %9, align 8
  %10 = bitcast %"class.sycl::_V1::range"* %_arg_accA6 to i8*
  %agg.tmp16.sroa.0.sroa.2.0._arg_accA6.ascast.sroa_idx = getelementptr inbounds i8, i8* %10, i64 8
  %11 = bitcast i8* %agg.tmp16.sroa.0.sroa.2.0._arg_accA6.ascast.sroa_idx to i64*
  %agg.tmp16.sroa.0.sroa.2.0.copyload = load i64, i64* %11, align 8
  %mul.i6.i.i.i.i100 = mul i64 %agg.tmp16.sroa.0.sroa.0.0.copyload, %agg.tmp15.sroa.0.sroa.2.0.copyload
  %12 = getelementptr i8, i8 addrspace(1)* %_arg_accA, i64 %mul.i6.i.i.i.i100
  %add.ptr.i101 = getelementptr i8, i8 addrspace(1)* %12, i64 %agg.tmp16.sroa.0.sroa.2.0.copyload
  %13 = bitcast %"class.sycl::_V1::range"* %_arg_accB8 to i8*
  %agg.tmp19.sroa.0.sroa.2.0._arg_accB8.ascast.sroa_idx = getelementptr inbounds i8, i8* %13, i64 8
  %14 = bitcast i8* %agg.tmp19.sroa.0.sroa.2.0._arg_accB8.ascast.sroa_idx to i64*
  %agg.tmp19.sroa.0.sroa.2.0.copyload = load i64, i64* %14, align 8
  %15 = bitcast %"class.sycl::_V1::range"* %_arg_accB9 to i64*
  %agg.tmp20.sroa.0.sroa.0.0.copyload = load i64, i64* %15, align 8
  %16 = bitcast %"class.sycl::_V1::range"* %_arg_accB9 to i8*
  %agg.tmp20.sroa.0.sroa.2.0._arg_accB9.ascast.sroa_idx = getelementptr inbounds i8, i8* %16, i64 8
  %17 = bitcast i8* %agg.tmp20.sroa.0.sroa.2.0._arg_accB9.ascast.sroa_idx to i64*
  %agg.tmp20.sroa.0.sroa.2.0.copyload = load i64, i64* %17, align 8
  %mul.i6.i.i.i.i116 = mul i64 %agg.tmp20.sroa.0.sroa.0.0.copyload, %agg.tmp19.sroa.0.sroa.2.0.copyload
  %18 = getelementptr i8, i8 addrspace(1)* %_arg_accB, i64 %mul.i6.i.i.i.i116
  %add.ptr.i117 = getelementptr i8, i8 addrspace(1)* %18, i64 %agg.tmp20.sroa.0.sroa.2.0.copyload
  %sub_c.ascast.i = addrspacecast target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2)* %0 to target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2) addrspace(4)*
  %19 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 1) #5
  %cmp.i30 = icmp ult i64 %19, 2147483648
  call void @llvm.assume(i1 %cmp.i30)
  %20 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0) #5
  %cmp.i26 = icmp ult i64 %20, 2147483648
  call void @llvm.assume(i1 %cmp.i26)
  %21 = call spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 1) #5
  %cmp.i37 = icmp ult i64 %21, 2147483648
  call void @llvm.assume(i1 %cmp.i37)
  %sub.i = sub nsw i64 %19, %21, !spirv.Decorations !21
  %22 = call spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 0) #5
  %cmp.i33 = icmp ult i64 %22, 2147483648
  call void @llvm.assume(i1 %cmp.i33)
  %sub5.i = sub nsw i64 %20, %22, !spirv.Decorations !21
  %23 = bitcast target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2)* %0 to i8*
  call void @llvm.lifetime.start.p0i8(i64 8, i8* %23)
  %add.i7.i.i.i.i.i = add i64 %mul.i6.i.i.i.i, %agg.tmp12.sroa.0.sroa.2.0.copyload
  %idx.neg.i.i = sub i64 0, %add.i7.i.i.i.i.i
  %add.ptr.i.i = getelementptr inbounds i32, i32 addrspace(1)* %add.ptr.i, i64 %idx.neg.i.i
  %mul8.i = shl nsw i64 %sub.i, 8, !spirv.Decorations !21
  %add.ptr.i122 = getelementptr inbounds i32, i32 addrspace(1)* %add.ptr.i.i, i64 %mul8.i
  %div.i = udiv i64 %sub5.i, %_arg_sg_size
  %mul9.i = shl i64 %div.i, 4
  %add.ptr.i123 = getelementptr inbounds i32, i32 addrspace(1)* %add.ptr.i122, i64 %mul9.i
  %call3.i = call spir_func target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2) @_Z84__spirv_CooperativeMatrixLoadKHR_RPU3AS142__spirv_CooperativeMatrixKHR__int_3_8_16_2PU3AS1iili(i32 addrspace(1)* %add.ptr.i123, i32 0, i64 32, i32 0) #0
  store target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2) %call3.i, target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2)* %0, align 8
  %add.i7.i.i.i.i.i128 = add i64 %mul.i6.i.i.i.i100, %agg.tmp16.sroa.0.sroa.2.0.copyload
  %idx.neg.i.i129 = sub i64 0, %add.i7.i.i.i.i.i128
  %add.ptr.i.i130 = getelementptr inbounds i8, i8 addrspace(1)* %add.ptr.i101, i64 %idx.neg.i.i129
  %mul15.i = shl nsw i64 %sub.i, 9, !spirv.Decorations !21
  %add.ptr.i131 = getelementptr inbounds i8, i8 addrspace(1)* %add.ptr.i.i130, i64 %mul15.i
  %add.i7.i.i.i.i.i138 = add i64 %mul.i6.i.i.i.i116, %agg.tmp20.sroa.0.sroa.2.0.copyload
  %idx.neg.i.i139 = sub i64 0, %add.i7.i.i.i.i.i138
  %add.ptr.i.i140 = getelementptr inbounds i8, i8 addrspace(1)* %add.ptr.i117, i64 %idx.neg.i.i139
  %mul29.i = shl i64 %div.i, 6
  %invariant.gep = getelementptr i8, i8 addrspace(1)* %add.ptr.i.i140, i64 %mul29.i
  %sub_c.ascast.i.promoted = load target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2), target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2)* %0, align 1
  br label %for.cond.i

for.cond.i:                                       ; preds = %for.body.i, %entry
  %call.i62170 = phi target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2) [ %sub_c.ascast.i.promoted, %entry ], [ %call.i62, %for.body.i ]
  %24 = phi target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2) [ %call3.i, %entry ], [ %call.i62, %for.body.i ]
  %k.0.i = phi i32 [ 0, %entry ], [ %add.i, %for.body.i ]
  %cmp.i = icmp ult i32 %k.0.i, 2
  br i1 %cmp.i, label %for.body.i, label %for.cond.i67.preheader

for.cond.i67.preheader:                           ; preds = %for.cond.i
  store target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2) %call.i62170, target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2)* %0, align 1
  br label %for.cond.i67

for.body.i:                                       ; preds = %for.cond.i
  %25 = shl nuw nsw i32 %k.0.i, 5, !spirv.Decorations !23
  %mul17.i = zext i32 %25 to i64
  %add.ptr.i132 = getelementptr inbounds i8, i8 addrspace(1)* %add.ptr.i131, i64 %mul17.i
  %call2.i51 = call spir_func target("spirv.CooperativeMatrixKHR", i8, 3, 8, 32, 0) @_Z85__spirv_CooperativeMatrixLoadKHR_RPU3AS143__spirv_CooperativeMatrixKHR__char_3_8_32_0PU3AS1cili(i8 addrspace(1)* %add.ptr.i132, i32 0, i64 64, i32 0) #0
  %mul25.i = shl nuw nsw i64 %mul17.i, 5, !spirv.Decorations !23
  %gep = getelementptr i8, i8 addrspace(1)* %invariant.gep, i64 %mul25.i
  %call2.i60 = call spir_func target("spirv.CooperativeMatrixKHR", i8, 3, 32, 16, 1) @_Z86__spirv_CooperativeMatrixLoadKHR_RPU3AS144__spirv_CooperativeMatrixKHR__char_3_32_16_1PU3AS1cili(i8 addrspace(1)* %gep, i32 2, i64 128, i32 0) #0
  %call.i62 = call spir_func target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2) @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS143__spirv_CooperativeMatrixKHR__char_3_8_32_0PU3AS144__spirv_CooperativeMatrixKHR__char_3_32_16_1PU3AS142__spirv_CooperativeMatrixKHR__int_3_8_16_2i(target("spirv.CooperativeMatrixKHR", i8, 3, 8, 32, 0) %call2.i51, target("spirv.CooperativeMatrixKHR", i8, 3, 32, 16, 1) %call2.i60, target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2) %24, i32 0) #0
  %add.i = add nuw nsw i32 %k.0.i, 1, !spirv.Decorations !25
  br label %for.cond.i

for.cond.i67:                                     ; preds = %for.body.i70, %for.cond.i67.preheader
  %26 = phi target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2) [ %.pre, %for.body.i70 ], [ %24, %for.cond.i67.preheader ]
  %i.0.i = phi i32 [ %inc.i, %for.body.i70 ], [ 0, %for.cond.i67.preheader ]
  %conv.i = zext i32 %i.0.i to i64
  %call.i = call spir_func i64 @_Z34__spirv_CooperativeMatrixLengthKHRPU3AS142__spirv_CooperativeMatrixKHR__int_3_8_16_2(target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2) %26) #0
  %cmp.i69 = icmp ugt i64 %call.i, %conv.i
  br i1 %cmp.i69, label %for.body.i70, label %_ZN4sycl3_V13ext6oneapi12experimental6matrix18joint_matrix_applyINS0_9sub_groupEiLNS4_3useTESTLm8ELm16ELNS4_6layoutE3EZZZ15matrix_multiplyILm8ELm16ELm32ELm4EZ4mainE16pvc_uint_8x16x32ihLm16ELm32ELm64EEvR10big_matrixIT4_XT6_EXT7_EERSB_IT5_XT6_EXT8_EERSB_ISF_XdvT8_T2_EXmlT7_T2_EEENKUlRNS0_7handlerEE_clESL_ENKUlNS0_7nd_itemILi2EEEE_clESO_EUlRiE_EEvT_RNS4_12joint_matrixISS_T0_XT1_EXT2_EXT3_EXT4_EEEOSF_.exit

for.body.i70:                                     ; preds = %for.cond.i67
  %call.i145 = call spir_func i32 addrspace(4)* @_Z19__spirv_AccessChainPU3AS4PU3AS142__spirv_CooperativeMatrixKHR__int_3_8_16_2l(target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2) addrspace(4)* %sub_c.ascast.i, i64 %conv.i) #0
  %27 = load i32, i32 addrspace(4)* %call.i145, align 4
  %mul.i = shl nsw i32 %27, 1, !spirv.Decorations !21
  %call.i148 = call spir_func i32 addrspace(4)* @_Z19__spirv_AccessChainPU3AS4PU3AS142__spirv_CooperativeMatrixKHR__int_3_8_16_2l(target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2) addrspace(4)* %sub_c.ascast.i, i64 %conv.i) #0
  store i32 %mul.i, i32 addrspace(4)* %call.i148, align 4
  %inc.i = add nuw nsw i32 %i.0.i, 1, !spirv.Decorations !25
  %.pre = load target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2), target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2)* %0, align 8
  br label %for.cond.i67

_ZN4sycl3_V13ext6oneapi12experimental6matrix18joint_matrix_applyINS0_9sub_groupEiLNS4_3useTESTLm8ELm16ELNS4_6layoutE3EZZZ15matrix_multiplyILm8ELm16ELm32ELm4EZ4mainE16pvc_uint_8x16x32ihLm16ELm32ELm64EEvR10big_matrixIT4_XT6_EXT7_EERSB_IT5_XT6_EXT8_EERSB_ISF_XdvT8_T2_EXmlT7_T2_EEENKUlRNS0_7handlerEE_clESL_ENKUlNS0_7nd_itemILi2EEEE_clESO_EUlRiE_EEvT_RNS4_12joint_matrixISS_T0_XT1_EXT2_EXT3_EXT4_EEEOSF_.exit: ; preds = %for.cond.i67
  %28 = load target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2), target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2)* %0, align 8
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS142__spirv_CooperativeMatrixKHR__int_3_8_16_2ili(i32 addrspace(1)* %add.ptr.i123, target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2) %28, i32 0, i64 32, i32 0) #0
  %29 = bitcast target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2)* %0 to i8*
  call void @llvm.lifetime.end.p0i8(i64 8, i8* %29)
  ret void
}

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.start.p0i8(i64 immarg %0, i8* nocapture %1) #2

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.end.p0i8(i64 immarg %0, i8* nocapture %1) #2

; Function Attrs: inaccessiblememonly nofree nosync nounwind willreturn
declare void @llvm.assume(i1 noundef %0) #4

; Function Attrs: nounwind
declare spir_func target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2) @_Z84__spirv_CooperativeMatrixLoadKHR_RPU3AS142__spirv_CooperativeMatrixKHR__int_3_8_16_2PU3AS1iili(i32 addrspace(1)* %0, i32 %1, i64 %2, i32 %3) #0

; Function Attrs: nounwind
declare spir_func target("spirv.CooperativeMatrixKHR", i8, 3, 8, 32, 0) @_Z85__spirv_CooperativeMatrixLoadKHR_RPU3AS143__spirv_CooperativeMatrixKHR__char_3_8_32_0PU3AS1cili(i8 addrspace(1)* %0, i32 %1, i64 %2, i32 %3) #0

; Function Attrs: nounwind
declare spir_func target("spirv.CooperativeMatrixKHR", i8, 3, 32, 16, 1) @_Z86__spirv_CooperativeMatrixLoadKHR_RPU3AS144__spirv_CooperativeMatrixKHR__char_3_32_16_1PU3AS1cili(i8 addrspace(1)* %0, i32 %1, i64 %2, i32 %3) #0

; Function Attrs: nounwind
declare spir_func target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2) @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS143__spirv_CooperativeMatrixKHR__char_3_8_32_0PU3AS144__spirv_CooperativeMatrixKHR__char_3_32_16_1PU3AS142__spirv_CooperativeMatrixKHR__int_3_8_16_2i(target("spirv.CooperativeMatrixKHR", i8, 3, 8, 32, 0) %0, target("spirv.CooperativeMatrixKHR", i8, 3, 32, 16, 1) %1, target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2) %2, i32 %3) #0

; Function Attrs: nounwind
declare spir_func i64 @_Z34__spirv_CooperativeMatrixLengthKHRPU3AS142__spirv_CooperativeMatrixKHR__int_3_8_16_2(target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2) %0) #0

; Function Attrs: nounwind
declare spir_func i32 addrspace(4)* @_Z19__spirv_AccessChainPU3AS4PU3AS142__spirv_CooperativeMatrixKHR__int_3_8_16_2l(target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2) addrspace(4)* %0, i64 %1) #0

; Function Attrs: nounwind
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS142__spirv_CooperativeMatrixKHR__int_3_8_16_2ili(i32 addrspace(1)* %0, target("spirv.CooperativeMatrixKHR", i32, 3, 8, 16, 2) %1, i32 %2, i64 %3, i32 %4) #0

; Function Attrs: nounwind readnone willreturn
declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 %0) #5

; Function Attrs: nounwind readnone willreturn
declare spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 %0) #5

; Function Attrs: nounwind readnone willreturn
declare spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32 %0) #5

; Function Attrs: nounwind readnone willreturn
declare spir_func i64 @_Z29__spirv_BuiltInGlobalLinearIdv() #5

; Function Attrs: nounwind readnone willreturn
declare spir_func i64 @_Z28__spirv_BuiltInWorkgroupSizei(i32 %0) #5

attributes #0 = { nounwind }
attributes #1 = { alwaysinline nounwind }
attributes #2 = { argmemonly nofree nosync nounwind willreturn }
attributes #3 = { noinline nounwind optnone }
attributes #4 = { inaccessiblememonly nofree nosync nounwind willreturn }
attributes #5 = { nounwind readnone willreturn }

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!spirv.Generator = !{!2}
!igc.functions = !{!3}

!0 = !{i32 2, i32 2}
!1 = !{i32 4, i32 100000}
!2 = !{i16 6, i16 14}
!3 = !{void (i32 addrspace(1)*, %"class.sycl::_V1::range"*, %"class.sycl::_V1::range"*, i64, i8 addrspace(1)*, %"class.sycl::_V1::range"*, %"class.sycl::_V1::range"*, i8 addrspace(1)*, %"class.sycl::_V1::range"*, %"class.sycl::_V1::range"*)* @_ZTSZ4mainE16pvc_uint_8x16x32, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 0}
!6 = !{i32 1, i32 0, i32 0, i32 0, i32 1, i32 0, i32 0, i32 1, i32 0, i32 0}
!7 = !{!"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none", !"none"}
!8 = !{!"int*", !"class.sycl::_V1::range", !"class.sycl::_V1::range", !"long", !"char*", !"class.sycl::_V1::range", !"class.sycl::_V1::range", !"char*", !"class.sycl::_V1::range", !"class.sycl::_V1::range"}
!9 = !{!"", !"", !"", !"", !"", !"", !"", !"", !"", !""}
!10 = !{!"_arg_accC", !"_arg_accC2", !"_arg_accC3", !"_arg_sg_size", !"_arg_accA", !"_arg_accA5", !"_arg_accA6", !"_arg_accB", !"_arg_accB8", !"_arg_accB9"}
!11 = !{!12, !14, !14, !17, !18, !14, !14, !18, !14, !14}
!12 = !{!13}
!13 = !{i32 44, i32 4}
!14 = !{!15, !16}
!15 = !{i32 38, i32 2}
!16 = !{i32 44, i32 8}
!17 = !{}
!18 = !{!19}
!19 = !{i32 44, i32 1}
!20 = !{!16}
!21 = !{!22}
!22 = !{i32 4469}
!23 = !{!22, !24}
!24 = !{i32 4470}
!25 = !{!22, !22, !24, !24}
