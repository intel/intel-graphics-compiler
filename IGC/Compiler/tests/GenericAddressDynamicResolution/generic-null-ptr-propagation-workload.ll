;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Example based on real workload. We want to verify that generic null pointer propagation enables
; private memory resolution pass to sink allocas further down.
;
; REQUIRES: llvm-16-plus, regkeys
;
; RUN: igc_opt -platformdg2 -opaque-pointers %s -S --regkey AllocaRAPressureThreshold=2 -igc-generic-address-dynamic-resolution -igc-generic-null-ptr-propagation --igc-private-mem-resolution | FileCheck %s

; CHECK-LABEL: define hidden spir_func void @_ZN27ShaderNode227_Scalar2Vector15Execute00000000ER19CGlobalArgs_SURFACER21CCommonSharedDataBase17CShaderScratchMem
; CHECK: [[SIMDL:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK: [[SIMDLZ:%.*]] = zext i16 [[SIMDL]] to i32
; CHECK: [[SIMDS:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK-NOT: {{.*}} = mul i32 [[SIMDS]], 2{{$}}
; CHECK-NOT: {{.*}} = mul i32 [[SIMDS]], 3{{$}}
; CHECK-LABEL: LocalBlock
; CHECK-LABEL: lb_alloca2
; CHECK: [[MUL_A2:%.*]] = mul i32 [[SIMDS]], 2{{$}}
; CHECK: [[ADD_A2:%.*]] = add i32 0, [[MUL_A2]]
; CHECK: [[MUL1_A2:%.*]] = mul i32 [[SIMDLZ]], 1
; CHECK: [[ADD1_A2:%.*]] = add i32 [[ADD_A2]], [[MUL1_A2]]
; CHECK: {{.*}} = call ptr @llvm.genx.GenISA.StackAlloca(i32 [[ADD1_A2]])
; CHECK-LABEL: lb_alloca3
; CHECK: [[MUL_A3:%.*]] = mul i32 [[SIMDS]], 3{{$}}
; CHECK: [[ADD_A3:%.*]] = add i32 0, [[MUL_A3]]
; CHECK: [[MUL1_A3:%.*]] = mul i32 [[SIMDLZ]], 1
; CHECK: [[ADD1_A3:%.*]] = add i32 [[ADD_A3]], [[MUL1_A3]]
; CHECK: {{.*}} = call ptr @llvm.genx.GenISA.StackAlloca(i32 [[ADD1_A3]])

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%"class.sycl::_V1::id" = type { %"class.sycl::_V1::detail::array" }
%"class.sycl::_V1::detail::array" = type { [3 x i64] }
%"struct.SharedBackendImpl::VecType.21" = type { %"struct.SharedBackendImpl::details::Vector1Storage" }
%"struct.SharedBackendImpl::details::Vector1Storage" = type { %union.anon.22 }
%union.anon.22 = type { %struct.CRingBufferMemOffset }
%struct.CRingBufferMemOffset = type { i32 }
%"struct.SharedBackendImpl::VecType.12" = type { %"struct.SharedBackendImpl::details::Vector3Storage" }
%"struct.SharedBackendImpl::details::Vector3Storage" = type { %union.anon.13 }
%union.anon.13 = type { %struct.anon.14 }
%struct.anon.14 = type { float, float, float }
%"class.sycl::_V1::nd_item" = type { i8 }
%struct.CShaderScratchMem = type { ptr addrspace(4) }
%struct.CGlobalArgs_SURFACE = type { %struct.CShaderConstants_SURFACE, [28 x i32], %"class.std_replacement::array", %struct.anon.7, ptr addrspace(4), ptr addrspace(4), ptr addrspace(4), %struct.CCommonTextureAndBufferArguments, %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %struct.RSTextureObject2D, %struct.RSTextureObject2D, %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %struct.RSTextureObject2D, %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %struct.RSTextureObject2D, %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32" }
%struct.CShaderConstants_SURFACE = type { [67 x %struct.CCacheOffsetGPU], %struct.CConstParams_SURFACE }
%struct.CCacheOffsetGPU = type { ptr addrspace(4), i32, i16 }
%struct.CConstParams_SURFACE = type { %struct.MatrixGeneric, %struct.MatrixGeneric, %struct.CSamplesOverrides, %struct.MatrixGeneric.3, %"struct.SharedBackendImpl::VecType.4", %"struct.SharedBackendImpl::VecType.4", %"struct.SharedBackendImpl::VecType.12", %"struct.SharedBackendImpl::VecType.12", %"struct.SharedBackendImpl::VecType.12", [4 x i8], %"struct.SharedBackendImpl::VecType.15", %"struct.SharedBackendImpl::VecType.15", %"struct.SharedBackendImpl::VecType.15", %"struct.SharedBackendImpl::VecType.15", float, float, float, i32, i32, i32, float, float, float, float, i32, i32, float, float, float, float, float, float, i32, i32, i32, i32, i32, float, i32, i32, float, float, i16, i16, i16, i16, i16, i16, i16, i16, i16, i16, i16, i16 }
%struct.MatrixGeneric = type { [4 x %"struct.SharedBackendImpl::VecType"] }
%"struct.SharedBackendImpl::VecType" = type { %"struct.SharedBackendImpl::details::Vector4Storage" }
%"struct.SharedBackendImpl::details::Vector4Storage" = type { %union.anon }
%union.anon = type { %struct.anon }
%struct.anon = type { float, float, float, float }
%struct.CSamplesOverrides = type { [7 x %"struct.SharedBackendImpl::VecType.0"], [8 x i8] }
%"struct.SharedBackendImpl::VecType.0" = type { %"struct.SharedBackendImpl::details::Vector2Storage" }
%"struct.SharedBackendImpl::details::Vector2Storage" = type { %union.anon.1 }
%union.anon.1 = type { %struct.anon.2 }
%struct.anon.2 = type { float, float }
%struct.MatrixGeneric.3 = type { [3 x %"struct.SharedBackendImpl::VecType"] }
%"struct.SharedBackendImpl::VecType.4" = type { %"struct.SharedBackendImpl::details::Vector4Storage.5" }
%"struct.SharedBackendImpl::details::Vector4Storage.5" = type { %union.anon.6 }
%union.anon.6 = type { %struct.anon.7 }
%"struct.SharedBackendImpl::VecType.15" = type { %"struct.SharedBackendImpl::details::Vector2Storage.16" }
%"struct.SharedBackendImpl::details::Vector2Storage.16" = type { %union.anon.17 }
%union.anon.17 = type { %struct.anon.18 }
%struct.anon.18 = type { i32, i32 }
%"class.std_replacement::array" = type { [2 x %struct.CJobQueueStats2] }
%struct.CJobQueueStats2 = type { [7 x i32], i32, i32, i32, i32, i32, i32, i32, i32, i32, %struct.CRingBufferMemOffset, i32, i32, i32 }
%struct.anon.7 = type { i32, i32, i32, i32 }
%struct.CCommonTextureAndBufferArguments = type { %struct.RSTextureObject2D, %struct.RSTextureObject2D, %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", ptr addrspace(4), %struct.CRingBufferData, %struct.CRingBufferData, [512 x %struct.CTextureTileBank], ptr addrspace(4), ptr addrspace(4), %struct.RSTextureObject2D, %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", %"struct.SharedBackendImpl::TextureBufferBase.32", ptr addrspace(4), %struct.RSTextureObject2D, %struct.RSTextureObject2D, %struct.RSTextureObject2D, %struct.RSTextureObject2D, %struct.RSTextureObject2D, %struct.RSTextureObject2D, %struct.RSTextureObject2D, %"struct.SharedBackendImpl::TextureBufferBase.32", %struct.CShaderRenderingColorSystem }
%struct.CRingBufferData = type { ptr addrspace(4), i32 }
%struct.CTextureTileBank = type { [7 x %struct.RSTextureObject2D] }
%struct.CShaderRenderingColorSystem = type { %struct.MatrixGeneric.31, %struct.MatrixGeneric.31, %"struct.SharedBackendImpl::VecType.12", i32 }
%struct.MatrixGeneric.31 = type { [3 x %"struct.SharedBackendImpl::VecType.12"] }
%struct.RSTextureObject2D = type { %"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle" }
%"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle" = type { i64 }
%"struct.SharedBackendImpl::TextureBufferBase.32" = type { ptr addrspace(4), %"struct.SharedBackendImpl::VecType.21" }
%struct.CCommonSharedDataBase = type { [128 x i32], [128 x i32], [128 x i32], [128 x i32], [128 x %"struct.SharedBackendImpl::VecType.15"], [128 x i32], [128 x %"struct.SharedBackendImpl::VecType.42"], ptr addrspace(4), i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i16, i32, [8 x i8] }
%"struct.SharedBackendImpl::VecType.42" = type { %"struct.SharedBackendImpl::details::Vector2Storage.43" }
%"struct.SharedBackendImpl::details::Vector2Storage.43" = type { %union.anon.44 }
%union.anon.44 = type { %struct.anon.45 }
%struct.anon.45 = type { i16, i16 }

declare hidden spir_func ptr addrspace(4) @_ZN4sycl3_V16detail5arrayILi3EEixEi(ptr align 8, i32) #0

declare void @llvm.assume(i1 noundef) #1

declare hidden spir_func void @_ZNK4sycl3_V17nd_itemILi3EE12get_local_idEv(ptr noalias sret(%"class.sycl::_V1::id") align 8, ptr addrspace(4) align 1) #0

declare hidden spir_func void @_Z10tex1DfetchIjLy1ELy1ELb0EjEN17SharedBackendImpl7VecTypeIT_XT0_EEERKNS0_17TextureBufferBaseIS2_XT0_EXT1_EXT2_ET3_EEj(ptr noalias sret(%"struct.SharedBackendImpl::VecType.21") align 4, ptr addrspace(4) align 8 dereferenceable(16), i32) #0

declare hidden spir_func i32 @_ZNK17SharedBackendImpl7VecTypeIjLy1EEcvjEv(ptr align 4) #0

declare hidden spir_func void @_Z6Splat3f(ptr noalias sret(%"struct.SharedBackendImpl::VecType.12") align 4, float) #0

declare hidden spir_func %"class.sycl::_V1::nd_item" @_ZN4sycl3_V13ext6oneapi14this_work_item11get_nd_itemILi3EEENS0_7nd_itemIXT_EEEv() #0

declare hidden spir_func float @_Z9EvalParamIfLb0EET_R19CGlobalArgs_SURFACER21CCommonSharedDataBase17CShaderScratchMemjjjjj(ptr addrspace(4) align 16 dereferenceable(32016), ptr addrspace(4) align 16 dereferenceable(4152), %struct.CShaderScratchMem, i32, i32, i32, i32, i32) #0

declare hidden spir_func void @_Z21StackSaveVectorFloat3N17SharedBackendImpl7VecTypeIfLy3EEE17CShaderScratchMemiPhjj(ptr byval(%"struct.SharedBackendImpl::VecType.12") align 4, %struct.CShaderScratchMem, i32, ptr addrspace(4), i32, i32) #0

declare hidden i8 addrspace(3)* @__builtin_IB_to_local(i8 addrspace(4)* noundef) #0

declare hidden i8* @__builtin_IB_to_private(i8 addrspace(4)* noundef) #0

define hidden spir_func i8 addrspace(1)* @_ZN4sycl3_V16detail20dynamic_address_castILNS0_6access13address_spaceE1ELb1EfEEDaPT1_(float addrspace(4)* %Ptr) #2 {
entry:
  %Ptr.addr = alloca float addrspace(4)*, align 8
  %generic_space = alloca i32, align 4
  %global_space = alloca i32, align 4
  %local_space = alloca i32, align 4
  %private_space = alloca i32, align 4
  %global_device = alloca i32, align 4
  %global_host = alloca i32, align 4
  %SrcAS = alloca i32, align 4
  store float addrspace(4)* %Ptr, float addrspace(4)** %Ptr.addr, align 8
  store i32 6, i32* %generic_space, align 4
  store i32 1, i32* %global_space, align 4
  store i32 3, i32* %local_space, align 4
  store i32 0, i32* %private_space, align 4
  store i32 4, i32* %global_device, align 4
  store i32 5, i32* %global_host, align 4
  store i32 6, i32* %SrcAS, align 4
  %0 = load float addrspace(4)*, float addrspace(4)** %Ptr.addr, align 8
  %1 = bitcast float addrspace(4)* %0 to i8 addrspace(4)*
  %2 = call spir_func i8 addrspace(3)* @__builtin_IB_to_local(i8 addrspace(4)* noundef %1) #15
  %3 = icmp eq i8 addrspace(3)* %2, null
  %4 = call spir_func i8* @__builtin_IB_to_private(i8 addrspace(4)* noundef %1) #15
  %5 = icmp eq i8* %4, null
  %6 = and i1 %3, %5
  %7 = addrspacecast i8 addrspace(4)* %1 to i8 addrspace(1)*
  %8 = select i1 %6, i8 addrspace(1)* %7, i8 addrspace(1)* null
  ret i8 addrspace(1)* %8
}

define hidden spir_func void @_ZN27ShaderNode227_Scalar2Vector15Execute00000000ER19CGlobalArgs_SURFACER21CCommonSharedDataBase17CShaderScratchMem(ptr addrspace(4) align 16 dereferenceable(32016) %globalArgs, ptr addrspace(4) align 16 dereferenceable(4152) %s_commonSharedData, %struct.CShaderScratchMem %__internal__queueAndStack) #0 {
entry:
  %0 = alloca %struct.CShaderScratchMem, align 8
  %1 = getelementptr inbounds %struct.CShaderScratchMem, ptr %0, i32 0, i32 0
  %2 = extractvalue %struct.CShaderScratchMem %__internal__queueAndStack, 0
  store ptr addrspace(4) %2, ptr %1, align 8
  %this.addr.i98 = alloca ptr addrspace(4), align 8
  %Dimension.addr.i99 = alloca i32, align 4
  %Id.i100 = alloca i64, align 8
  %ref.tmp.i101 = alloca %"class.sycl::_V1::id", align 8
  %this.addr.i85 = alloca ptr addrspace(4), align 8
  %Dimension.addr.i86 = alloca i32, align 4
  %Id.i87 = alloca i64, align 8
  %ref.tmp.i88 = alloca %"class.sycl::_V1::id", align 8
  %this.addr.i72 = alloca ptr addrspace(4), align 8
  %Dimension.addr.i73 = alloca i32, align 4
  %Id.i74 = alloca i64, align 8
  %ref.tmp.i75 = alloca %"class.sycl::_V1::id", align 8
  %this.addr.i59 = alloca ptr addrspace(4), align 8
  %Dimension.addr.i60 = alloca i32, align 4
  %Id.i61 = alloca i64, align 8
  %ref.tmp.i62 = alloca %"class.sycl::_V1::id", align 8
  %this.addr.i46 = alloca ptr addrspace(4), align 8
  %Dimension.addr.i47 = alloca i32, align 4
  %Id.i48 = alloca i64, align 8
  %ref.tmp.i49 = alloca %"class.sycl::_V1::id", align 8
  %this.addr.i = alloca ptr addrspace(4), align 8
  %Dimension.addr.i = alloca i32, align 4
  %Id.i = alloca i64, align 8
  %ref.tmp.i = alloca %"class.sycl::_V1::id", align 8
  %globalArgs.addr = alloca ptr addrspace(4), align 8
  %s_commonSharedData.addr = alloca ptr addrspace(4), align 8
  %__plugOccupancyMask = alloca [1 x i32], align 4
  %ref.tmp = alloca %"struct.SharedBackendImpl::VecType.21", align 4
  %ref.tmp1 = alloca %"class.sycl::_V1::nd_item", align 1
  %output = alloca %"struct.SharedBackendImpl::VecType.12", align 4
  %input = alloca float, align 4
  %agg.tmp = alloca %struct.CShaderScratchMem, align 8
  %ref.tmp4 = alloca %"class.sycl::_V1::nd_item", align 1
  %ref.tmp8 = alloca %"class.sycl::_V1::nd_item", align 1
  %ref.tmp15 = alloca %"class.sycl::_V1::nd_item", align 1
  %ref.tmp24 = alloca %"struct.SharedBackendImpl::VecType.12", align 4
  %__output_stackOffset = alloca i32, align 4
  %ref.tmp27 = alloca %"struct.SharedBackendImpl::VecType.21", align 4
  %ref.tmp30 = alloca %"class.sycl::_V1::nd_item", align 1
  %agg.tmp37 = alloca %"struct.SharedBackendImpl::VecType.12", align 4
  %agg.tmp38 = alloca %struct.CShaderScratchMem, align 8
  %ref.tmp40 = alloca %"class.sycl::_V1::nd_item", align 1
  %ref.tmp1.ascast = addrspacecast ptr %ref.tmp1 to ptr addrspace(4)
  %ref.tmp4.ascast = addrspacecast ptr %ref.tmp4 to ptr addrspace(4)
  %ref.tmp8.ascast = addrspacecast ptr %ref.tmp8 to ptr addrspace(4)
  %ref.tmp15.ascast = addrspacecast ptr %ref.tmp15 to ptr addrspace(4)
  %ref.tmp30.ascast = addrspacecast ptr %ref.tmp30 to ptr addrspace(4)
  %ref.tmp40.ascast = addrspacecast ptr %ref.tmp40 to ptr addrspace(4)
  store ptr addrspace(4) %globalArgs, ptr %globalArgs.addr, align 8
  store ptr addrspace(4) %s_commonSharedData, ptr %s_commonSharedData.addr, align 8
  %3 = load ptr addrspace(4), ptr %globalArgs.addr, align 8
  %4 = bitcast ptr addrspace(4) %3 to ptr addrspace(4)
  %TEXTUREDATA_MATERIALNODEPARAMS_WORD = getelementptr inbounds %struct.CGlobalArgs_SURFACE, ptr addrspace(4) %4, i32 0, i32 8
  %5 = load ptr addrspace(4), ptr %s_commonSharedData.addr, align 8
  %m_shaderDataOffsetAndExecMode = getelementptr inbounds %struct.CCommonSharedDataBase, ptr addrspace(4) %5, i32 0, i32 0
  %6 = call spir_func %"class.sycl::_V1::nd_item" @_ZN4sycl3_V13ext6oneapi14this_work_item11get_nd_itemILi3EEENS0_7nd_itemIXT_EEEv() #2
  %7 = getelementptr inbounds %"class.sycl::_V1::nd_item", ptr %ref.tmp1, i32 0, i32 0
  %8 = extractvalue %"class.sycl::_V1::nd_item" %6, 0
  store i8 %8, ptr %7, align 1
  store ptr addrspace(4) %ref.tmp1.ascast, ptr %this.addr.i98, align 8
  store i32 2, ptr %Dimension.addr.i99, align 4
  %this1.i107 = load ptr addrspace(4), ptr %this.addr.i98, align 8
  call spir_func void @_ZNK4sycl3_V17nd_itemILi3EE12get_local_idEv(ptr %ref.tmp.i101, ptr addrspace(4) %this1.i107)
  %9 = load i32, ptr %Dimension.addr.i99, align 4
  %_ZN4sycl3_V16detail5arrayILi3EEixEi6 = call spir_func ptr addrspace(4) @_ZN4sycl3_V16detail5arrayILi3EEixEi(ptr %ref.tmp.i101, i32 %9)
  %10 = ptrtoint ptr addrspace(4) %_ZN4sycl3_V16detail5arrayILi3EEixEi6 to i64
  %11 = lshr i64 %10, 61
  switch i64 %11, label %GlobalBlock [
    i64 2, label %LocalBlock
  ]

LocalBlock:
  %12 = addrspacecast ptr addrspace(4) %_ZN4sycl3_V16detail5arrayILi3EEixEi6 to ptr addrspace(3)
  %localLoad = load i64, ptr addrspace(3) %12, align 8
  br label %14

GlobalBlock:
  %13 = addrspacecast ptr addrspace(4) %_ZN4sycl3_V16detail5arrayILi3EEixEi6 to ptr addrspace(1)
  %globalLoad = load i64, ptr addrspace(1) %13, align 8
  br label %14

14:
  %15 = phi i64 [ %localLoad, %LocalBlock ], [ %globalLoad, %GlobalBlock ]
  store i64 %15, ptr %Id.i100, align 8
  %16 = load i64, ptr %Id.i100, align 8
  %17 = icmp ule i64 %16, 2147483647
  call void @llvm.assume(i1 %17)
  %18 = load i64, ptr %Id.i100, align 8
  %arrayidx = getelementptr inbounds [128 x i32], ptr addrspace(4) %m_shaderDataOffsetAndExecMode, i64 0, i64 %18
  %19 = ptrtoint ptr addrspace(4) %arrayidx to i64
  %20 = lshr i64 %19, 61
  switch i64 %20, label %GlobalBlock22 [
    i64 2, label %LocalBlock20
  ]

LocalBlock20:
  %21 = addrspacecast ptr addrspace(4) %arrayidx to ptr addrspace(3)
  %localLoad21 = load i32, ptr addrspace(3) %21, align 4
  br label %lb_alloca2

GlobalBlock22:
  %22 = addrspacecast ptr addrspace(4) %arrayidx to ptr addrspace(1)
  %globalLoad23 = load i32, ptr addrspace(1) %22, align 4
  %23 = freeze i32 8
  br label %lb_alloca2

lb_alloca2:
  %24 = phi i32 [ %localLoad21, %LocalBlock20 ], [ %globalLoad23, %GlobalBlock22 ]
  %and = and i32 %24, 16777215
  %shl = shl i32 %and, 2
  %add = add i32 %shl, 2
  call spir_func void @_Z10tex1DfetchIjLy1ELy1ELb0EjEN17SharedBackendImpl7VecTypeIT_XT0_EEERKNS0_17TextureBufferBaseIS2_XT0_EXT1_EXT2_ET3_EEj(ptr %ref.tmp, ptr addrspace(4) %TEXTUREDATA_MATERIALNODEPARAMS_WORD, i32 %add)
  %_ZNK17SharedBackendImpl7VecTypeIjLy1EEcvjEv1 = call spir_func i32 @_ZNK17SharedBackendImpl7VecTypeIjLy1EEcvjEv(ptr %ref.tmp)
  %arrayidx3 = getelementptr inbounds [1 x i32], ptr %__plugOccupancyMask, i64 0, i64 0
  store i32 %_ZNK17SharedBackendImpl7VecTypeIjLy1EEcvjEv1, ptr %arrayidx3, align 4
  %25 = load ptr addrspace(4), ptr %globalArgs.addr, align 8
  %26 = load ptr addrspace(4), ptr %s_commonSharedData.addr, align 8
  %memcpy_rem = bitcast ptr %0 to ptr
  %memcpy_rem7 = bitcast ptr %agg.tmp to ptr
  %27 = load <2 x i32>, ptr %memcpy_rem, align 8
  store <2 x i32> %27, ptr %memcpy_rem7, align 8
  %28 = load ptr addrspace(4), ptr %s_commonSharedData.addr, align 8
  %m_rayIDZERO = getelementptr inbounds %struct.CCommonSharedDataBase, ptr addrspace(4) %28, i32 0, i32 2
  %29 = call spir_func %"class.sycl::_V1::nd_item" @_ZN4sycl3_V13ext6oneapi14this_work_item11get_nd_itemILi3EEENS0_7nd_itemIXT_EEEv() #2
  %30 = getelementptr inbounds %"class.sycl::_V1::nd_item", ptr %ref.tmp4, i32 0, i32 0
  %31 = extractvalue %"class.sycl::_V1::nd_item" %29, 0
  store i8 %31, ptr %30, align 1
  store ptr addrspace(4) %ref.tmp4.ascast, ptr %this.addr.i85, align 8
  store i32 2, ptr %Dimension.addr.i86, align 4
  %this1.i94 = load ptr addrspace(4), ptr %this.addr.i85, align 8
  call spir_func void @_ZNK4sycl3_V17nd_itemILi3EE12get_local_idEv(ptr %ref.tmp.i88, ptr addrspace(4) %this1.i94)
  %32 = load i32, ptr %Dimension.addr.i86, align 4
  %_ZN4sycl3_V16detail5arrayILi3EEixEi5 = call spir_func ptr addrspace(4) @_ZN4sycl3_V16detail5arrayILi3EEixEi(ptr %ref.tmp.i88, i32 %32)
  %33 = ptrtoint ptr addrspace(4) %_ZN4sycl3_V16detail5arrayILi3EEixEi5 to i64
  %34 = lshr i64 %33, 61
  switch i64 %34, label %GlobalBlock26 [
    i64 2, label %LocalBlock24
  ]

LocalBlock24:
  %35 = addrspacecast ptr addrspace(4) %_ZN4sycl3_V16detail5arrayILi3EEixEi5 to ptr addrspace(3)
  %localLoad25 = load i64, ptr addrspace(3) %35, align 8
  br label %37

GlobalBlock26:
  %36 = addrspacecast ptr addrspace(4) %_ZN4sycl3_V16detail5arrayILi3EEixEi5 to ptr addrspace(1)
  %globalLoad27 = load i64, ptr addrspace(1) %36, align 8
  br label %37

37:
  %38 = phi i64 [ %localLoad25, %LocalBlock24 ], [ %globalLoad27, %GlobalBlock26 ]
  store i64 %38, ptr %Id.i87, align 8
  %39 = load i64, ptr %Id.i87, align 8
  %40 = icmp ule i64 %39, 2147483647
  call void @llvm.assume(i1 %40)
  %41 = load i64, ptr %Id.i87, align 8
  %arrayidx6 = getelementptr inbounds [128 x i32], ptr addrspace(4) %m_rayIDZERO, i64 0, i64 %41
  %42 = ptrtoint ptr addrspace(4) %arrayidx6 to i64
  %43 = lshr i64 %42, 61
  switch i64 %43, label %GlobalBlock30 [
    i64 2, label %LocalBlock28
  ]

LocalBlock28:
  %44 = addrspacecast ptr addrspace(4) %arrayidx6 to ptr addrspace(3)
  %localLoad29 = load i32, ptr addrspace(3) %44, align 4
  br label %46

GlobalBlock30:
  %45 = addrspacecast ptr addrspace(4) %arrayidx6 to ptr addrspace(1)
  %globalLoad31 = load i32, ptr addrspace(1) %45, align 4
  br label %46

46:
  %47 = phi i32 [ %localLoad29, %LocalBlock28 ], [ %globalLoad31, %GlobalBlock30 ]
  %48 = load ptr addrspace(4), ptr %s_commonSharedData.addr, align 8
  %m_NUMBUFFERELEMENTS = getelementptr inbounds %struct.CCommonSharedDataBase, ptr addrspace(4) %48, i32 0, i32 16
  %49 = ptrtoint ptr addrspace(4) %m_NUMBUFFERELEMENTS to i64
  %50 = lshr i64 %49, 61
  switch i64 %50, label %GlobalBlock34 [
    i64 2, label %LocalBlock32
  ]

LocalBlock32:
  %51 = addrspacecast ptr addrspace(4) %m_NUMBUFFERELEMENTS to ptr addrspace(3)
  %localLoad33 = load i32, ptr addrspace(3) %51, align 8
  br label %lb_alloca3

GlobalBlock34:
  %52 = addrspacecast ptr addrspace(4) %m_NUMBUFFERELEMENTS to ptr addrspace(1)
  %globalLoad35 = load i32, ptr addrspace(1) %52, align 8
  %53 = freeze i32 8
  br label %lb_alloca3

lb_alloca3:
  %54 = phi i32 [ %localLoad33, %LocalBlock32 ], [ %globalLoad35, %GlobalBlock34 ]
  %55 = load ptr addrspace(4), ptr %s_commonSharedData.addr, align 8
  %m_shaderDataOffsetAndExecMode7 = getelementptr inbounds %struct.CCommonSharedDataBase, ptr addrspace(4) %55, i32 0, i32 0
  %56 = call spir_func %"class.sycl::_V1::nd_item" @_ZN4sycl3_V13ext6oneapi14this_work_item11get_nd_itemILi3EEENS0_7nd_itemIXT_EEEv() #2
  %57 = getelementptr inbounds %"class.sycl::_V1::nd_item", ptr %ref.tmp8, i32 0, i32 0
  %58 = extractvalue %"class.sycl::_V1::nd_item" %56, 0
  store i8 %58, ptr %57, align 1
  store ptr addrspace(4) %ref.tmp8.ascast, ptr %this.addr.i72, align 8
  store i32 2, ptr %Dimension.addr.i73, align 4
  %this1.i81 = load ptr addrspace(4), ptr %this.addr.i72, align 8
  call spir_func void @_ZNK4sycl3_V17nd_itemILi3EE12get_local_idEv(ptr %ref.tmp.i75, ptr addrspace(4) %this1.i81)
  %59 = load i32, ptr %Dimension.addr.i73, align 4
  %_ZN4sycl3_V16detail5arrayILi3EEixEi4 = call spir_func ptr addrspace(4) @_ZN4sycl3_V16detail5arrayILi3EEixEi(ptr %ref.tmp.i75, i32 %59)
  %60 = ptrtoint ptr addrspace(4) %_ZN4sycl3_V16detail5arrayILi3EEixEi4 to i64
  %61 = lshr i64 %60, 61
  switch i64 %61, label %GlobalBlock38 [
    i64 2, label %LocalBlock36
  ]

LocalBlock36:
  %62 = addrspacecast ptr addrspace(4) %_ZN4sycl3_V16detail5arrayILi3EEixEi4 to ptr addrspace(3)
  %localLoad37 = load i64, ptr addrspace(3) %62, align 8
  br label %64

GlobalBlock38:
  %63 = addrspacecast ptr addrspace(4) %_ZN4sycl3_V16detail5arrayILi3EEixEi4 to ptr addrspace(1)
  %globalLoad39 = load i64, ptr addrspace(1) %63, align 8
  br label %64

64:
  %65 = phi i64 [ %localLoad37, %LocalBlock36 ], [ %globalLoad39, %GlobalBlock38 ]
  store i64 %65, ptr %Id.i74, align 8
  %66 = load i64, ptr %Id.i74, align 8
  %67 = icmp ule i64 %66, 2147483647
  call void @llvm.assume(i1 %67)
  %68 = load i64, ptr %Id.i74, align 8
  %arrayidx10 = getelementptr inbounds [128 x i32], ptr addrspace(4) %m_shaderDataOffsetAndExecMode7, i64 0, i64 %68
  %69 = ptrtoint ptr addrspace(4) %arrayidx10 to i64
  %70 = lshr i64 %69, 61
  switch i64 %70, label %GlobalBlock42 [
    i64 2, label %LocalBlock40
  ]

LocalBlock40:
  %71 = addrspacecast ptr addrspace(4) %arrayidx10 to ptr addrspace(3)
  %localLoad41 = load i32, ptr addrspace(3) %71, align 4
  br label %73

GlobalBlock42:
  %72 = addrspacecast ptr addrspace(4) %arrayidx10 to ptr addrspace(1)
  %globalLoad43 = load i32, ptr addrspace(1) %72, align 4
  br label %73

73:
  %74 = phi i32 [ %localLoad41, %LocalBlock40 ], [ %globalLoad43, %GlobalBlock42 ]
  %and11 = and i32 %74, 16777215
  %shl12 = shl i32 %and11, 2
  %add13 = add i32 %shl12, 0
  %75 = load ptr addrspace(4), ptr %s_commonSharedData.addr, align 8
  %m_shaderDataOffsetAndExecMode14 = getelementptr inbounds %struct.CCommonSharedDataBase, ptr addrspace(4) %75, i32 0, i32 0
  %76 = call spir_func %"class.sycl::_V1::nd_item" @_ZN4sycl3_V13ext6oneapi14this_work_item11get_nd_itemILi3EEENS0_7nd_itemIXT_EEEv() #2
  %77 = getelementptr inbounds %"class.sycl::_V1::nd_item", ptr %ref.tmp15, i32 0, i32 0
  %78 = extractvalue %"class.sycl::_V1::nd_item" %76, 0
  store i8 %78, ptr %77, align 1
  store ptr addrspace(4) %ref.tmp15.ascast, ptr %this.addr.i59, align 8
  store i32 2, ptr %Dimension.addr.i60, align 4
  %this1.i68 = load ptr addrspace(4), ptr %this.addr.i59, align 8
  call spir_func void @_ZNK4sycl3_V17nd_itemILi3EE12get_local_idEv(ptr %ref.tmp.i62, ptr addrspace(4) %this1.i68)
  %79 = load i32, ptr %Dimension.addr.i60, align 4
  %_ZN4sycl3_V16detail5arrayILi3EEixEi3 = call spir_func ptr addrspace(4) @_ZN4sycl3_V16detail5arrayILi3EEixEi(ptr %ref.tmp.i62, i32 %79)
  %80 = ptrtoint ptr addrspace(4) %_ZN4sycl3_V16detail5arrayILi3EEixEi3 to i64
  %81 = lshr i64 %80, 61
  switch i64 %81, label %GlobalBlock46 [
    i64 2, label %LocalBlock44
  ]

LocalBlock44:
  %82 = addrspacecast ptr addrspace(4) %_ZN4sycl3_V16detail5arrayILi3EEixEi3 to ptr addrspace(3)
  %localLoad45 = load i64, ptr addrspace(3) %82, align 8
  br label %84

GlobalBlock46:
  %83 = addrspacecast ptr addrspace(4) %_ZN4sycl3_V16detail5arrayILi3EEixEi3 to ptr addrspace(1)
  %globalLoad47 = load i64, ptr addrspace(1) %83, align 8
  br label %84

84:
  %85 = phi i64 [ %localLoad45, %LocalBlock44 ], [ %globalLoad47, %GlobalBlock46 ]
  store i64 %85, ptr %Id.i61, align 8
  %86 = load i64, ptr %Id.i61, align 8
  %87 = icmp ule i64 %86, 2147483647
  call void @llvm.assume(i1 %87)
  %88 = load i64, ptr %Id.i61, align 8
  %arrayidx17 = getelementptr inbounds [128 x i32], ptr addrspace(4) %m_shaderDataOffsetAndExecMode14, i64 0, i64 %88
  %89 = ptrtoint ptr addrspace(4) %arrayidx17 to i64
  %90 = lshr i64 %89, 61
  switch i64 %90, label %GlobalBlock50 [
    i64 2, label %LocalBlock48
  ]

LocalBlock48:
  %91 = addrspacecast ptr addrspace(4) %arrayidx17 to ptr addrspace(3)
  %localLoad49 = load i32, ptr addrspace(3) %91, align 4
  br label %93

GlobalBlock50:
  %92 = addrspacecast ptr addrspace(4) %arrayidx17 to ptr addrspace(1)
  %globalLoad51 = load i32, ptr addrspace(1) %92, align 4
  br label %93

93:
  %94 = phi i32 [ %localLoad49, %LocalBlock48 ], [ %globalLoad51, %GlobalBlock50 ]
  %and18 = and i32 %94, 16777215
  %shl19 = shl i32 %and18, 2
  %add20 = add i32 %shl19, 4
  %arrayidx21 = getelementptr inbounds [1 x i32], ptr %__plugOccupancyMask, i64 0, i64 0
  %95 = load i32, ptr %arrayidx21, align 4
  %and22 = and i32 %95, 1
  %96 = getelementptr inbounds %struct.CShaderScratchMem, ptr %agg.tmp, i32 0, i32 0
  %97 = load ptr addrspace(4), ptr %96, align 8
  %98 = insertvalue %struct.CShaderScratchMem undef, ptr addrspace(4) %97, 0
  %99 = call spir_func float @_Z9EvalParamIfLb0EET_R19CGlobalArgs_SURFACER21CCommonSharedDataBase17CShaderScratchMemjjjjj(ptr addrspace(4) align 16 dereferenceable(32016) %25, ptr addrspace(4) align 16 dereferenceable(4152) %26, %struct.CShaderScratchMem %98, i32 %47, i32 %54, i32 %add13, i32 %add20, i32 %and22) #2
  store float %99, ptr %input, align 4
  %100 = load float, ptr %input, align 4
  call spir_func void @_Z6Splat3f(ptr %ref.tmp24, float %100)
  %memcpy_rem10 = bitcast ptr %ref.tmp24 to ptr
  %memcpy_rem11 = bitcast ptr %output to ptr
  %101 = load <3 x i32>, ptr %memcpy_rem10, align 4
  store <3 x i32> %101, ptr %memcpy_rem11, align 4
  br label %EndOfFunction

EndOfFunction:
  %arrayidx25 = getelementptr inbounds [1 x i32], ptr %__plugOccupancyMask, i64 0, i64 0
  %102 = load i32, ptr %arrayidx25, align 4
  %and26 = and i32 %102, 2
  %103 = icmp ne i32 %and26, 0
  br i1 %103, label %if.then, label %if.end

if.then:
  %104 = load ptr addrspace(4), ptr %globalArgs.addr, align 8
  %105 = bitcast ptr addrspace(4) %104 to ptr addrspace(4)
  %TEXTUREDATA_MATERIALNODEPARAMS_WORD28 = getelementptr inbounds %struct.CGlobalArgs_SURFACE, ptr addrspace(4) %105, i32 0, i32 8
  %106 = load ptr addrspace(4), ptr %s_commonSharedData.addr, align 8
  %m_shaderDataOffsetAndExecMode29 = getelementptr inbounds %struct.CCommonSharedDataBase, ptr addrspace(4) %106, i32 0, i32 0
  %107 = call spir_func %"class.sycl::_V1::nd_item" @_ZN4sycl3_V13ext6oneapi14this_work_item11get_nd_itemILi3EEENS0_7nd_itemIXT_EEEv() #2
  %108 = getelementptr inbounds %"class.sycl::_V1::nd_item", ptr %ref.tmp30, i32 0, i32 0
  %109 = extractvalue %"class.sycl::_V1::nd_item" %107, 0
  store i8 %109, ptr %108, align 1
  store ptr addrspace(4) %ref.tmp30.ascast, ptr %this.addr.i46, align 8
  store i32 2, ptr %Dimension.addr.i47, align 4
  %this1.i55 = load ptr addrspace(4), ptr %this.addr.i46, align 8
  call spir_func void @_ZNK4sycl3_V17nd_itemILi3EE12get_local_idEv(ptr %ref.tmp.i49, ptr addrspace(4) %this1.i55)
  %110 = load i32, ptr %Dimension.addr.i47, align 4
  %_ZN4sycl3_V16detail5arrayILi3EEixEi2 = call spir_func ptr addrspace(4) @_ZN4sycl3_V16detail5arrayILi3EEixEi(ptr %ref.tmp.i49, i32 %110)
  %111 = ptrtoint ptr addrspace(4) %_ZN4sycl3_V16detail5arrayILi3EEixEi2 to i64
  %112 = lshr i64 %111, 61
  switch i64 %112, label %GlobalBlock54 [
    i64 2, label %LocalBlock52
  ]

LocalBlock52:
  %113 = addrspacecast ptr addrspace(4) %_ZN4sycl3_V16detail5arrayILi3EEixEi2 to ptr addrspace(3)
  %localLoad53 = load i64, ptr addrspace(3) %113, align 8
  br label %115

GlobalBlock54:
  %114 = addrspacecast ptr addrspace(4) %_ZN4sycl3_V16detail5arrayILi3EEixEi2 to ptr addrspace(1)
  %globalLoad55 = load i64, ptr addrspace(1) %114, align 8
  br label %115

115:
  %116 = phi i64 [ %localLoad53, %LocalBlock52 ], [ %globalLoad55, %GlobalBlock54 ]
  store i64 %116, ptr %Id.i48, align 8
  %117 = load i64, ptr %Id.i48, align 8
  %118 = icmp ule i64 %117, 2147483647
  call void @llvm.assume(i1 %118)
  %119 = load i64, ptr %Id.i48, align 8
  %arrayidx32 = getelementptr inbounds [128 x i32], ptr addrspace(4) %m_shaderDataOffsetAndExecMode29, i64 0, i64 %119
  %120 = ptrtoint ptr addrspace(4) %arrayidx32 to i64
  %121 = lshr i64 %120, 61
  switch i64 %121, label %GlobalBlock58 [
    i64 2, label %LocalBlock56
  ]

LocalBlock56:
  %122 = addrspacecast ptr addrspace(4) %arrayidx32 to ptr addrspace(3)
  %localLoad57 = load i32, ptr addrspace(3) %122, align 4
  br label %124

GlobalBlock58:
  %123 = addrspacecast ptr addrspace(4) %arrayidx32 to ptr addrspace(1)
  %globalLoad59 = load i32, ptr addrspace(1) %123, align 4
  br label %124

124:
  %125 = phi i32 [ %localLoad57, %LocalBlock56 ], [ %globalLoad59, %GlobalBlock58 ]
  %and33 = and i32 %125, 16777215
  %shl34 = shl i32 %and33, 2
  %add35 = add i32 %shl34, 1
  call spir_func void @_Z10tex1DfetchIjLy1ELy1ELb0EjEN17SharedBackendImpl7VecTypeIT_XT0_EEERKNS0_17TextureBufferBaseIS2_XT0_EXT1_EXT2_ET3_EEj(ptr %ref.tmp27, ptr addrspace(4) %TEXTUREDATA_MATERIALNODEPARAMS_WORD28, i32 %add35)
  %_ZNK17SharedBackendImpl7VecTypeIjLy1EEcvjEv = call spir_func i32 @_ZNK17SharedBackendImpl7VecTypeIjLy1EEcvjEv(ptr %ref.tmp27)
  store i32 %_ZNK17SharedBackendImpl7VecTypeIjLy1EEcvjEv, ptr %__output_stackOffset, align 4
  %memcpy_rem14 = bitcast ptr %output to ptr
  %memcpy_rem15 = bitcast ptr %agg.tmp37 to ptr
  %126 = load <3 x i32>, ptr %memcpy_rem14, align 4
  store <3 x i32> %126, ptr %memcpy_rem15, align 4
  %memcpy_rem18 = bitcast ptr %0 to ptr
  %memcpy_rem19 = bitcast ptr %agg.tmp38 to ptr
  %127 = load <2 x i32>, ptr %memcpy_rem18, align 8
  store <2 x i32> %127, ptr %memcpy_rem19, align 8
  %128 = load ptr addrspace(4), ptr %s_commonSharedData.addr, align 8
  %m_rayIDZERO39 = getelementptr inbounds %struct.CCommonSharedDataBase, ptr addrspace(4) %128, i32 0, i32 2
  %129 = call spir_func %"class.sycl::_V1::nd_item" @_ZN4sycl3_V13ext6oneapi14this_work_item11get_nd_itemILi3EEENS0_7nd_itemIXT_EEEv() #2
  %130 = getelementptr inbounds %"class.sycl::_V1::nd_item", ptr %ref.tmp40, i32 0, i32 0
  %131 = extractvalue %"class.sycl::_V1::nd_item" %129, 0
  store i8 %131, ptr %130, align 1
  store ptr addrspace(4) %ref.tmp40.ascast, ptr %this.addr.i, align 8
  store i32 2, ptr %Dimension.addr.i, align 4
  %this1.i = load ptr addrspace(4), ptr %this.addr.i, align 8
  call spir_func void @_ZNK4sycl3_V17nd_itemILi3EE12get_local_idEv(ptr %ref.tmp.i, ptr addrspace(4) %this1.i)
  %132 = load i32, ptr %Dimension.addr.i, align 4
  %_ZN4sycl3_V16detail5arrayILi3EEixEi = call spir_func ptr addrspace(4) @_ZN4sycl3_V16detail5arrayILi3EEixEi(ptr %ref.tmp.i, i32 %132)
  %133 = ptrtoint ptr addrspace(4) %_ZN4sycl3_V16detail5arrayILi3EEixEi to i64
  %134 = lshr i64 %133, 61
  switch i64 %134, label %GlobalBlock62 [
    i64 2, label %LocalBlock60
  ]

LocalBlock60:
  %135 = addrspacecast ptr addrspace(4) %_ZN4sycl3_V16detail5arrayILi3EEixEi to ptr addrspace(3)
  %localLoad61 = load i64, ptr addrspace(3) %135, align 8
  br label %137

GlobalBlock62:
  %136 = addrspacecast ptr addrspace(4) %_ZN4sycl3_V16detail5arrayILi3EEixEi to ptr addrspace(1)
  %globalLoad63 = load i64, ptr addrspace(1) %136, align 8
  br label %137

137:
  %138 = phi i64 [ %localLoad61, %LocalBlock60 ], [ %globalLoad63, %GlobalBlock62 ]
  store i64 %138, ptr %Id.i, align 8
  %139 = load i64, ptr %Id.i, align 8
  %140 = icmp ule i64 %139, 2147483647
  call void @llvm.assume(i1 %140)
  %141 = load i64, ptr %Id.i, align 8
  %arrayidx42 = getelementptr inbounds [128 x i32], ptr addrspace(4) %m_rayIDZERO39, i64 0, i64 %141
  %142 = ptrtoint ptr addrspace(4) %arrayidx42 to i64
  %143 = lshr i64 %142, 61
  switch i64 %143, label %GlobalBlock66 [
    i64 2, label %LocalBlock64
  ]

LocalBlock64:
  %144 = addrspacecast ptr addrspace(4) %arrayidx42 to ptr addrspace(3)
  %localLoad65 = load i32, ptr addrspace(3) %144, align 4
  br label %146

GlobalBlock66:
  %145 = addrspacecast ptr addrspace(4) %arrayidx42 to ptr addrspace(1)
  %globalLoad67 = load i32, ptr addrspace(1) %145, align 4
  br label %146

146:
  %147 = phi i32 [ %localLoad65, %LocalBlock64 ], [ %globalLoad67, %GlobalBlock66 ]
  %148 = load ptr addrspace(4), ptr %globalArgs.addr, align 8
  %149 = bitcast ptr addrspace(4) %148 to ptr addrspace(4)
  %m_shaderConstants = getelementptr inbounds %struct.CGlobalArgs_SURFACE, ptr addrspace(4) %149, i32 0, i32 0
  %m_IOBufferOffsets = getelementptr inbounds %struct.CShaderConstants_SURFACE, ptr addrspace(4) %m_shaderConstants, i32 0, i32 0
  %arrayidx43 = getelementptr inbounds [67 x %struct.CCacheOffsetGPU], ptr addrspace(4) %m_IOBufferOffsets, i64 0, i64 34
  %m_bufferPtr = getelementptr inbounds %struct.CCacheOffsetGPU, ptr addrspace(4) %arrayidx43, i32 0, i32 0
  %150 = ptrtoint ptr addrspace(4) %m_bufferPtr to i64
  %151 = lshr i64 %150, 61
  switch i64 %151, label %GlobalBlock70 [
    i64 2, label %LocalBlock68
  ]

LocalBlock68:
  %152 = addrspacecast ptr addrspace(4) %m_bufferPtr to ptr addrspace(3)
  %localLoad69 = load ptr addrspace(4), ptr addrspace(3) %152, align 16
  br label %154

GlobalBlock70:
  %153 = addrspacecast ptr addrspace(4) %m_bufferPtr to ptr addrspace(1)
  %globalLoad71 = load ptr addrspace(4), ptr addrspace(1) %153, align 16
  br label %154

154:
  %155 = phi ptr addrspace(4) [ %localLoad69, %LocalBlock68 ], [ %globalLoad71, %GlobalBlock70 ]
  %156 = load ptr addrspace(4), ptr %s_commonSharedData.addr, align 8
  %m_NUMBUFFERELEMENTS44 = getelementptr inbounds %struct.CCommonSharedDataBase, ptr addrspace(4) %156, i32 0, i32 16
  %157 = ptrtoint ptr addrspace(4) %m_NUMBUFFERELEMENTS44 to i64
  %158 = lshr i64 %157, 61
  switch i64 %158, label %GlobalBlock74 [
    i64 2, label %LocalBlock72
  ]

LocalBlock72:
  %159 = addrspacecast ptr addrspace(4) %m_NUMBUFFERELEMENTS44 to ptr addrspace(3)
  %localLoad73 = load i32, ptr addrspace(3) %159, align 8
  br label %161

GlobalBlock74:
  %160 = addrspacecast ptr addrspace(4) %m_NUMBUFFERELEMENTS44 to ptr addrspace(1)
  %globalLoad75 = load i32, ptr addrspace(1) %160, align 8
  br label %161

161:
  %162 = phi i32 [ %localLoad73, %LocalBlock72 ], [ %globalLoad75, %GlobalBlock74 ]
  %163 = load i32, ptr %__output_stackOffset, align 4
  %164 = getelementptr inbounds %struct.CShaderScratchMem, ptr %agg.tmp38, i32 0, i32 0
  %165 = load ptr addrspace(4), ptr %164, align 8
  %166 = insertvalue %struct.CShaderScratchMem undef, ptr addrspace(4) %165, 0
  call spir_func void @_Z21StackSaveVectorFloat3N17SharedBackendImpl7VecTypeIfLy3EEE17CShaderScratchMemiPhjj(ptr byval(%"struct.SharedBackendImpl::VecType.12") align 4 %agg.tmp37, %struct.CShaderScratchMem %166, i32 %147, ptr addrspace(4) %155, i32 %162, i32 %163) #2
  br label %if.end

if.end:
  ret void
}

attributes #0 = { convergent noinline nounwind optnone "visaStackCall" }
attributes #1 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write) }
attributes #2 = { nounwind }

!igc.functions = !{!0}
!0 = !{void (ptr addrspace(4), ptr addrspace(4), %struct.CShaderScratchMem)* @_ZN27ShaderNode227_Scalar2Vector15Execute00000000ER19CGlobalArgs_SURFACER21CCommonSharedDataBase17CShaderScratchMem, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 2}