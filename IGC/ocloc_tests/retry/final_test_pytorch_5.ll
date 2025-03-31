; REQUIRES: regkeys,mtl-supported, test-fix
; RUN: llvm-as %s -o %t.bc

; RUN: ocloc compile -llvm_input -file %t.bc -device mtl -options "-igc_opts 'VISAOptions=-asmToConsole'" &> %t_output.ll
; RUN: FileCheck --input-file %t_output.ll %s

; CHECK://.kernel _ZTSN2at15AtenIpexTypeXPU31FastGroupRadixSelectImplFunctorIixLi1024ELb0ELb1ExLi16ELi4ENS0_19GroupRadixProcesserIiLi1024ELi16ELi4ELb0ExtjLi4EEEjEE
; CHECK://.spill size 4[[B:[0-9]{3}]]{{$}}
; CHECK: end of thread


; CHECK://.kernel _ZTSN2at15AtenIpexTypeXPU31FastGroupRadixSelectImplFunctorIixLi1024ELb0ELb1ExLi16ELi4ENS0_19GroupRadixProcesserIiLi1024ELi16ELi4ELb0ExtjLi4EEEjEE
; CHECK-NOT://.private memory
; CHECK-NOT://.spill size
; CHECK: end of thread

; ModuleID = 'reduced.ll'
source_filename = "reduced.ll"

%"class.sycl::_V1::nd_item" = type { %"class.sycl::_V1::item", %"class.sycl::_V1::item.0", %"class.sycl::_V1::group" }
%"class.sycl::_V1::item" = type { %"struct.sycl::_V1::detail::ItemBase" }
%"struct.sycl::_V1::detail::ItemBase" = type { %"class.sycl::_V1::range", %"class.sycl::_V1::range", %"class.sycl::_V1::range" }
%"class.sycl::_V1::range" = type { %"class.sycl::_V1::detail::array" }
%"class.sycl::_V1::detail::array" = type { [1 x i64] }
%"class.sycl::_V1::item.0" = type { %"struct.sycl::_V1::detail::ItemBase.1" }
%"struct.sycl::_V1::detail::ItemBase.1" = type { %"class.sycl::_V1::range", %"class.sycl::_V1::range" }
%"class.sycl::_V1::group" = type { %"class.sycl::_V1::range", %"class.sycl::_V1::range", %"class.sycl::_V1::range", %"class.sycl::_V1::range" }
%"class.at::AtenIpexTypeXPU::GroupRadixProcesser" = type { i8 addrspace(4)*, i32, i8 addrspace(4)* }

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture) #0

; Function Attrs: inaccessiblememonly nofree nosync nounwind willreturn
declare void @llvm.assume(i1 noundef) #1

declare spir_func void @_Z22__spirv_ControlBarrieriii(i32, i32, i32)

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture) #0

declare spir_func i32 @_Z30__spirv_SubgroupShuffleUpINTELiij(i32, i32, i32)

define linkonce_odr spir_func i32 @_ZN2at15AtenIpexTypeXPU19group_exclusive_sumIjLi8ELi1024ELi16EEET_RAmlT0_T1__S2_RN4sycl3_V17nd_itemILi1EEE(i8 addrspace(4)* %0, i8 addrspace(4)* %1) {
  %3 = alloca [8 x i32], align 4
  %4 = bitcast i8 addrspace(4)* %1 to %"class.sycl::_V1::nd_item" addrspace(4)*
  %5 = getelementptr inbounds %"class.sycl::_V1::nd_item", %"class.sycl::_V1::nd_item" addrspace(4)* %4, i64 0, i32 1, i32 0, i32 1
  %6 = bitcast %"class.sycl::_V1::range" addrspace(4)* %5 to i64 addrspace(4)*
  %7 = load i64, i64 addrspace(4)* %6, align 8
  %8 = trunc i64 %7 to i32
  %9 = call spir_func i32 @_Z40__spirv_BuiltInSubgroupLocalInvocationIdv()
  %10 = call spir_func i32 @_Z25__spirv_BuiltInSubgroupIdv()
  %11 = shl nsw i32 %8, 3
  br label %12

12:                                               ; preds = %26, %2
  %13 = phi i32 [ 0, %2 ], [ %34, %26 ]
  %14 = phi i32 [ 0, %2 ], [ %35, %26 ]
  %15 = icmp ult i32 %14, 8
  br i1 %15, label %26, label %.preheader

.preheader:                                       ; preds = %12, %19
  %16 = phi i32 [ %21, %19 ], [ %13, %12 ]
  %17 = phi i32 [ %22, %19 ], [ 0, %12 ]
  %18 = icmp ult i32 %17, 4
  br i1 %18, label %19, label %23

19:                                               ; preds = %.preheader
  %20 = call spir_func i32 @_Z30__spirv_SubgroupShuffleUpINTELiij(i32 %16, i32 %16, i32 1)
  %21 = add i32 %16, %20
  %22 = add nuw nsw i32 %17, 1
  br label %.preheader

23:                                               ; preds = %.preheader
  %24 = sub i32 %16, %13
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 0, i32 0, i32 0)
  %25 = icmp eq i32 %9, 0
  br i1 %25, label %36, label %40

26:                                               ; preds = %12
  %27 = zext i32 %14 to i64
  %28 = getelementptr inbounds [8 x i32], [8 x i32]* %3, i64 0, i64 %27
  store i32 %13, i32* %28, align 4
  %29 = add nuw nsw i32 %11, %14
  %30 = sext i32 %29 to i64
  %31 = bitcast i8 addrspace(4)* %0 to [8192 x i32] addrspace(4)*
  %32 = getelementptr inbounds [8192 x i32], [8192 x i32] addrspace(4)* %31, i64 0, i64 %30
  %33 = load i32, i32 addrspace(4)* %32, align 4
  %34 = add i32 %13, %33
  %35 = add nuw nsw i32 %14, 1
  br label %12

36:                                               ; preds = %23
  %37 = sext i32 %10 to i64
  %38 = bitcast i8 addrspace(4)* %0 to [8192 x i32] addrspace(4)*
  %39 = getelementptr inbounds [8192 x i32], [8192 x i32] addrspace(4)* %38, i64 0, i64 %37
  store i32 0, i32 addrspace(4)* %39, align 4
  br label %40

40:                                               ; preds = %36, %23
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 0, i32 0, i32 0)
  br label %41

41:                                               ; preds = %48, %40
  %42 = phi i32 [ 0, %40 ], [ %55, %48 ]
  %43 = phi i32 [ 0, %40 ], [ %50, %48 ]
  %44 = phi i32 [ 0, %40 ], [ %56, %48 ]
  %45 = icmp ult i32 %44, 64
  br i1 %45, label %48, label %46

46:                                               ; preds = %41
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 0, i32 0, i32 0)
  %47 = add i32 %24, %43
  br label %57

48:                                               ; preds = %41
  %49 = icmp eq i32 %10, %44
  %50 = select i1 %49, i32 %42, i32 %43
  %51 = zext i32 %44 to i64
  %52 = bitcast i8 addrspace(4)* %0 to [8192 x i32] addrspace(4)*
  %53 = getelementptr inbounds [8192 x i32], [8192 x i32] addrspace(4)* %52, i64 0, i64 %51
  %54 = load i32, i32 addrspace(4)* %53, align 4
  %55 = add i32 %42, %54
  %56 = add nuw nsw i32 %44, 1
  br label %41

57:                                               ; preds = %61, %46
  %58 = phi i32 [ 0, %46 ], [ %70, %61 ]
  %59 = icmp ult i32 %58, 8
  br i1 %59, label %61, label %60

60:                                               ; preds = %57
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 0, i32 0, i32 0)
  ret i32 %42

61:                                               ; preds = %57
  %62 = zext i32 %58 to i64
  %63 = getelementptr inbounds [8 x i32], [8 x i32]* %3, i64 0, i64 %62
  %64 = load i32, i32* %63, align 4
  %65 = add i32 %47, %64
  %66 = add nuw nsw i32 %11, %58
  %67 = sext i32 %66 to i64
  %68 = bitcast i8 addrspace(4)* %0 to [8192 x i32] addrspace(4)*
  %69 = getelementptr inbounds [8192 x i32], [8192 x i32] addrspace(4)* %68, i64 0, i64 %67
  store i32 %65, i32 addrspace(4)* %69, align 4
  %70 = add nuw nsw i32 %58, 1
  br label %57
}

define spir_kernel void @_ZTSN2at15AtenIpexTypeXPU31FastGroupRadixSelectImplFunctorIxxLi1024ELb1ELb1ExLi16ELi4ENS0_19GroupRadixProcesserIxLi1024ELi16ELi4ELb1ExtjLi4EEEyEE(i64 addrspace(1)* %0, i64 addrspace(1)* %1, i64 addrspace(1)* %2, i32 %3, i32 %4, i8 addrspace(3)* %5, i64 %6, i64* %7, i64 %8, i64 addrspace(1)* %9, i64 %10, [4 x i64]* %11, i64 %12, [4 x i64]* %13, i64 %14, i64 %15) !intel_reqd_sub_group_size !0 {
  %17 = alloca %"class.at::AtenIpexTypeXPU::GroupRadixProcesser", align 8
  %18 = alloca [4 x i64], align 8
  %19 = alloca [4 x i64], align 8
  %20 = alloca %"class.sycl::_V1::nd_item", align 8
  %21 = call spir_func i64 @_Z25__spirv_BuiltInGlobalSizei(i32 0)
  %22 = call spir_func i64 @_Z28__spirv_BuiltInWorkgroupSizei(i32 0)
  %23 = call spir_func i64 @_Z28__spirv_BuiltInNumWorkgroupsi(i32 0)
  %24 = call spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi()
  %25 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %26 = call spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi()
  unreachable
}

define linkonce_odr spir_func void @_ZN2at15AtenIpexTypeXPU19GroupRadixProcesserIxLi1024ELi16ELi4ELb1ExtjLi4EE12select_groupERA4_xS4_iiiPxS5_(%"class.at::AtenIpexTypeXPU::GroupRadixProcesser" addrspace(4)* %0, [4 x i64] addrspace(4)* %1, [4 x i64] addrspace(4)* %2, i32 %3, i32 %4, i32 %5, i8 addrspace(4)* %6, i64 %7, i32 %8, [8 x [1024 x [2 x i16]]] addrspace(4)* %9, i64 %10, i32 %11, i64 %12, [4096 x i64] addrspace(4)* %13, [4096 x i64] addrspace(4)* %14, i64 %15) {
  %17 = alloca [4 x i16 addrspace(4)*], align 8
  %18 = alloca [4 x i32], align 4
  %19 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::GroupRadixProcesser", %"class.at::AtenIpexTypeXPU::GroupRadixProcesser" addrspace(4)* %0, i64 0, i32 2
  %20 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::GroupRadixProcesser", %"class.at::AtenIpexTypeXPU::GroupRadixProcesser" addrspace(4)* %0, i64 0, i32 1
  %21 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::GroupRadixProcesser", %"class.at::AtenIpexTypeXPU::GroupRadixProcesser" addrspace(4)* %0, i64 0, i32 0
  br label %22

22:                                               ; preds = %158, %16
  %23 = phi i32 [ %120, %158 ], [ 0, %16 ]
  %24 = phi i32 [ %75, %158 ], [ 0, %16 ]
  %25 = sub nsw i32 0, %24
  br label %26

26:                                               ; preds = %29, %22
  %27 = phi i32 [ 0, %22 ], [ %36, %29 ]
  %28 = icmp ult i32 %27, 8
  br i1 %28, label %29, label %37

29:                                               ; preds = %26
  %30 = load i8 addrspace(4)*, i8 addrspace(4)* addrspace(4)* %19, align 8
  %31 = zext i32 %27 to i64
  %32 = load i32, i32 addrspace(4)* %20, align 8
  %33 = sext i32 %32 to i64
  %34 = bitcast i8 addrspace(4)* %30 to [8 x [1024 x i32]] addrspace(4)*
  %35 = getelementptr inbounds [8 x [1024 x i32]], [8 x [1024 x i32]] addrspace(4)* %34, i64 0, i64 %31, i64 %33
  store i32 0, i32 addrspace(4)* %35, align 4
  %36 = add nuw nsw i32 %27, 1
  br label %26

37:                                               ; preds = %26, %56
  %38 = phi i32 [ %57, %56 ], [ 0, %26 ]
  %39 = icmp ult i32 %38, 4
  br i1 %39, label %43, label %.preheader5

.preheader5:                                      ; preds = %37
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 0, i32 0, i32 0)
  %40 = load i8 addrspace(4)*, i8 addrspace(4)* addrspace(4)* %19, align 8
  %41 = load i8 addrspace(4)*, i8 addrspace(4)* addrspace(4)* %21, align 8
  %42 = call spir_func i32 @_ZN2at15AtenIpexTypeXPU19group_exclusive_sumIjLi8ELi1024ELi16EEET_RAmlT0_T1__S2_RN4sycl3_V17nd_itemILi1EEE(i8 addrspace(4)* %40, i8 addrspace(4)* %41)
  br label %58

43:                                               ; preds = %37
  %44 = zext i32 %38 to i64
  %45 = getelementptr inbounds [4 x i32], [4 x i32]* %18, i64 0, i64 %44
  store i32 4096, i32* %45, align 4
  %46 = shl nuw nsw i32 1, %38
  %47 = and i32 %46, %23
  %48 = icmp eq i32 %47, 0
  br i1 %48, label %56, label %49

49:                                               ; preds = %43
  %50 = getelementptr inbounds [4 x i64], [4 x i64] addrspace(4)* %1, i64 0, i64 %44
  %51 = load i64, i64 addrspace(4)* %50, align 8
  %52 = trunc i64 %51 to i32
  %53 = sext i32 %52 to i64
  %54 = getelementptr inbounds [8 x [1024 x [2 x i16]]], [8 x [1024 x [2 x i16]]] addrspace(4)* null, i64 0, i64 %7, i64 undef, i64 %53
  %55 = getelementptr inbounds [4 x i16 addrspace(4)*], [4 x i16 addrspace(4)*]* %17, i64 0, i64 %44
  store i16 addrspace(4)* %54, i16 addrspace(4)** %55, align 8
  store i32 0, i32* %45, align 4
  store i16 0, i16 addrspace(4)* null, align 2
  br label %56

56:                                               ; preds = %49, %43
  %57 = add nuw nsw i32 %38, 1
  br label %37

58:                                               ; preds = %62, %.preheader5
  %59 = phi i32 [ %69, %62 ], [ 0, %.preheader5 ]
  %60 = icmp ult i32 %59, 8
  br i1 %60, label %62, label %61

61:                                               ; preds = %58
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 0, i32 0, i32 0)
  br label %70

62:                                               ; preds = %58
  %63 = load i8 addrspace(4)*, i8 addrspace(4)* addrspace(4)* %19, align 8
  %64 = zext i32 %59 to i64
  %65 = load i32, i32 addrspace(4)* %20, align 8
  %66 = sext i32 %65 to i64
  %67 = bitcast i8 addrspace(4)* %63 to [8 x [1024 x i32]] addrspace(4)*
  %68 = getelementptr inbounds [8 x [1024 x i32]], [8 x [1024 x i32]] addrspace(4)* %67, i64 0, i64 %64, i64 %66
  store i32 0, i32 addrspace(4)* %68, align 4
  %69 = add nuw nsw i32 %59, 1
  br label %58

70:                                               ; preds = %85, %61
  %71 = phi i32 [ 0, %61 ], [ %94, %85 ]
  %72 = icmp ult i32 %71, 4
  br i1 %72, label %85, label %73

73:                                               ; preds = %70
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 0, i32 0, i32 0)
  br label %74

74:                                               ; preds = %83, %73
  %75 = phi i32 [ 0, %73 ], [ %81, %83 ]
  %76 = phi i32 [ 0, %73 ], [ %84, %83 ]
  %77 = icmp ult i32 %76, 16
  br i1 %77, label %78, label %.preheader2

78:                                               ; preds = %74
  %79 = getelementptr inbounds [2 x i16], [2 x i16] addrspace(4)* null, i64 0, i64 %12
  %80 = load i16, i16 addrspace(4)* %79, align 2
  %81 = zext i16 %80 to i32
  %82 = icmp slt i32 %25, %3
  br i1 %82, label %.preheader2, label %83

83:                                               ; preds = %78
  %84 = add nuw nsw i32 %76, 1
  br label %74

85:                                               ; preds = %70
  %86 = zext i32 %71 to i64
  %87 = getelementptr inbounds [4 x i16 addrspace(4)*], [4 x i16 addrspace(4)*]* %17, i64 0, i64 %86
  %88 = load i16 addrspace(4)*, i16 addrspace(4)** %87, align 8
  %89 = load i16, i16 addrspace(4)* %88, align 2
  %90 = zext i16 %89 to i32
  %91 = getelementptr inbounds [4 x i32], [4 x i32]* %18, i64 0, i64 %86
  %92 = load i32, i32* %91, align 4
  %93 = add nsw i32 %92, %90
  store i32 %93, i32* %91, align 4
  %94 = add nuw nsw i32 %71, 1
  br label %70

.preheader2:                                      ; preds = %78, %74
  %95 = phi i32 [ 1, %78 ], [ 0, %74 ]
  br label %96

96:                                               ; preds = %108, %.preheader2
  %97 = phi i32 [ %109, %108 ], [ 0, %.preheader2 ]
  %98 = icmp ult i32 %97, 4
  br i1 %98, label %99, label %.preheader1

99:                                               ; preds = %96
  %100 = zext i32 %97 to i64
  %101 = getelementptr inbounds [4 x i32], [4 x i32]* %18, i64 0, i64 %100
  %102 = load i32, i32* %101, align 4
  %103 = icmp slt i32 %102, 1
  br i1 %103, label %104, label %108

104:                                              ; preds = %99
  %105 = getelementptr inbounds [4 x i64], [4 x i64] addrspace(4)* %1, i64 0, i64 %100
  %106 = load i64, i64 addrspace(4)* %105, align 8
  %107 = bitcast i8 addrspace(4)* %6 to i64 addrspace(4)*
  store i64 %106, i64 addrspace(4)* %107, align 8
  br label %108

108:                                              ; preds = %104, %99
  %109 = add nuw nsw i32 %97, 1
  br label %96

.preheader1:                                      ; preds = %96
  %110 = icmp eq i32 %75, 0
  br i1 %110, label %159, label %.preheader

.preheader:                                       ; preds = %.preheader1, %115
  %111 = phi i32 [ %118, %115 ], [ 0, %.preheader1 ]
  %112 = icmp ult i32 %111, 4
  br i1 %112, label %115, label %113

113:                                              ; preds = %.preheader
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 0, i32 0, i32 0)
  %114 = load i32, i32 addrspace(4)* %20, align 8
  br label %119

115:                                              ; preds = %.preheader
  %116 = getelementptr inbounds [4 x i64], [4 x i64] addrspace(4)* %1, i64 0, i64 0
  %117 = load i64, i64 addrspace(4)* %116, align 8
  store i64 %117, i64 addrspace(4)* null, align 8
  %118 = add nuw nsw i32 %111, 1
  br label %.preheader

119:                                              ; preds = %123, %113
  %120 = phi i32 [ 0, %113 ], [ %spec.select, %123 ]
  %121 = phi i32 [ 0, %113 ], [ 1, %123 ]
  %122 = icmp ult i32 %121, 1
  br i1 %122, label %123, label %125

123:                                              ; preds = %119
  %124 = icmp slt i32 %114, %75
  %spec.select = select i1 %124, i32 -1, i32 0
  br label %119

125:                                              ; preds = %119
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 0, i32 0, i32 0)
  br label %126

126:                                              ; preds = %140, %125
  %127 = phi i32 [ 0, %125 ], [ %141, %140 ]
  %128 = icmp ult i32 %127, 4
  br i1 %128, label %132, label %129

129:                                              ; preds = %126
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 0, i32 0, i32 0)
  %130 = load i32, i32 addrspace(4)* %20, align 8
  %131 = shl nsw i32 %130, 2
  br label %142

132:                                              ; preds = %126
  %133 = zext i32 %127 to i64
  %134 = getelementptr inbounds [4 x i32], [4 x i32]* %18, i64 0, i64 %133
  %135 = load i32, i32* %134, align 4
  %136 = icmp slt i32 %135, %95
  br i1 %136, label %137, label %140

137:                                              ; preds = %132
  %138 = getelementptr inbounds [4 x i64], [4 x i64] addrspace(4)* %2, i64 0, i64 %133
  %139 = load i64, i64 addrspace(4)* %138, align 8
  store i64 %139, i64 addrspace(4)* null, align 8
  br label %140

140:                                              ; preds = %137, %132
  %141 = add nuw nsw i32 %127, 1
  br label %126

142:                                              ; preds = %156, %129
  %143 = phi i32 [ 0, %129 ], [ %157, %156 ]
  %144 = icmp ult i32 %143, 4
  br i1 %144, label %145, label %158

145:                                              ; preds = %142
  %146 = add nuw nsw i32 %131, %143
  %147 = icmp slt i32 %146, %75
  br i1 %147, label %148, label %156

148:                                              ; preds = %145
  %149 = load i8 addrspace(4)*, i8 addrspace(4)* addrspace(4)* %19, align 8
  %150 = sext i32 %146 to i64
  %151 = bitcast i8 addrspace(4)* %149 to [4096 x i64] addrspace(4)*
  %152 = getelementptr inbounds [4096 x i64], [4096 x i64] addrspace(4)* %151, i64 0, i64 %150
  %153 = load i64, i64 addrspace(4)* %152, align 8
  %154 = zext i32 %143 to i64
  %155 = getelementptr inbounds [4 x i64], [4 x i64] addrspace(4)* %2, i64 0, i64 %154
  store i64 %153, i64 addrspace(4)* %155, align 8
  br label %156

156:                                              ; preds = %148, %145
  %157 = add nuw nsw i32 %143, 1
  br label %142

158:                                              ; preds = %142
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 0, i32 0, i32 0)
  br label %22

159:                                              ; preds = %.preheader1
  ret void
}

define spir_kernel void @_ZTSN2at15AtenIpexTypeXPU31FastGroupRadixSelectImplFunctorIixLi1024ELb0ELb1ExLi16ELi4ENS0_19GroupRadixProcesserIiLi1024ELi16ELi4ELb0ExtjLi4EEEjEE(i32 addrspace(1)* %0, i32 addrspace(1)* %1, i64 addrspace(1)* %2, i32 %3, i32 %4, i8 addrspace(3)* %5, i32 %6) !intel_reqd_sub_group_size !0 {
  %8 = alloca %"class.at::AtenIpexTypeXPU::GroupRadixProcesser", align 8
  %9 = alloca [4 x i32], align 4
  %10 = alloca [4 x i64], align 8
  %11 = alloca %"class.sycl::_V1::nd_item", align 8
  %12 = call spir_func i64 @_Z25__spirv_BuiltInGlobalSizei(i32 0)
  %13 = call spir_func i64 @_Z28__spirv_BuiltInWorkgroupSizei(i32 0)
  %14 = call spir_func i64 @_Z28__spirv_BuiltInNumWorkgroupsi(i32 0)
  %15 = call spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi()
  %16 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %17 = call spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi()
  %18 = bitcast %"class.sycl::_V1::nd_item"* %11 to i64*
  store i64 %12, i64* %18, align 1
  store i64 %16, i64* inttoptr (i64 8 to i64*), align 1
  store i64 %13, i64* inttoptr (i64 24 to i64*), align 1
  %19 = getelementptr inbounds i64, i64* %18, i64 4
  store i64 %17, i64* %19, align 1
  store i64 0, i64* inttoptr (i64 56 to i64*), align 1
  %20 = addrspacecast %"class.at::AtenIpexTypeXPU::GroupRadixProcesser"* %8 to %"class.at::AtenIpexTypeXPU::GroupRadixProcesser" addrspace(4)*
  %21 = addrspacecast [4 x i32]* %9 to [4 x i32] addrspace(4)*
  %22 = addrspacecast [4 x i64]* %10 to [4 x i64] addrspace(4)*
  %23 = addrspacecast %"class.sycl::_V1::nd_item"* %11 to %"class.sycl::_V1::nd_item" addrspace(4)*
  %24 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 undef
  %25 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::GroupRadixProcesser", %"class.at::AtenIpexTypeXPU::GroupRadixProcesser"* %8, i64 0, i32 0
  %26 = bitcast i8 addrspace(4)** %25 to %"class.sycl::_V1::nd_item" addrspace(4)**
  store %"class.sycl::_V1::nd_item" addrspace(4)* %23, %"class.sycl::_V1::nd_item" addrspace(4)** %26, align 8
  %27 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::GroupRadixProcesser", %"class.at::AtenIpexTypeXPU::GroupRadixProcesser"* %8, i64 0, i32 1
  %28 = trunc i64 %17 to i32
  store i32 %28, i32* %27, align 8
  %29 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::GroupRadixProcesser", %"class.at::AtenIpexTypeXPU::GroupRadixProcesser"* %8, i64 0, i32 2
  %30 = addrspacecast i8 addrspace(3)* %5 to i8 addrspace(4)*
  store i8 addrspace(4)* %30, i8 addrspace(4)** %29, align 8
  br label %31

31:                                               ; preds = %38, %7
  %32 = phi i32 [ %39, %38 ], [ 0, %7 ]
  %33 = icmp slt i32 %32, %3
  br i1 %33, label %34, label %54

34:                                               ; preds = %31
  call spir_func void @_ZN2at15AtenIpexTypeXPU19GroupRadixProcesserIiLi1024ELi16ELi4ELb0ExtjLi4EE12select_groupERA4_iRA4_xiiiPiPx(%"class.at::AtenIpexTypeXPU::GroupRadixProcesser" addrspace(4)* %20, [4 x i32] addrspace(4)* %21, [4 x i64] addrspace(4)* %22, i32 0, i32 0, i32 %4, i8 addrspace(4)* null, i8 addrspace(4)* undef, i64 undef, [4096 x i64] addrspace(4)* undef)
  br label %35

35:                                               ; preds = %52, %34
  %36 = phi i32 [ 0, %34 ], [ %53, %52 ]
  %37 = icmp ult i32 %36, 4
  br i1 %37, label %40, label %38

38:                                               ; preds = %35
  %39 = sub i32 %32, 1
  br label %31

40:                                               ; preds = %35
  %41 = zext i32 %3 to i64
  %42 = add nuw i32 %28, 1
  %43 = icmp slt i32 %42, %3
  br i1 %43, label %44, label %49

44:                                               ; preds = %40
  %45 = sext i32 %28 to i64
  %46 = getelementptr inbounds i32, i32 addrspace(3)* null, i64 %45
  %47 = load i32, i32 addrspace(3)* %46, align 4
  %48 = getelementptr inbounds [4 x i32], [4 x i32]* %9, i64 0, i64 %41
  store i32 %47, i32* %48, align 4
  br label %52

49:                                               ; preds = %40
  %50 = load i32, i32 addrspace(1)* %24, align 4
  %51 = getelementptr inbounds [4 x i32], [4 x i32]* %9, i64 0, i64 %41
  store i32 %50, i32* %51, align 4
  br label %52

52:                                               ; preds = %49, %44
  %53 = add nuw nsw i32 %36, 1
  br label %35

54:                                               ; preds = %31
  call spir_func void @_ZN2at15AtenIpexTypeXPU19GroupRadixProcesserIiLi1024ELi16ELi4ELb0ExtjLi4EE12select_groupERA4_iRA4_xiiiPiPx(%"class.at::AtenIpexTypeXPU::GroupRadixProcesser" addrspace(4)* %20, [4 x i32] addrspace(4)* %21, [4 x i64] addrspace(4)* %22, i32 0, i32 0, i32 %4, i8 addrspace(4)* null, i8 addrspace(4)* null, i64 undef, [4096 x i64] addrspace(4)* undef)
  ret void
}

define linkonce_odr spir_func void @_ZN2at15AtenIpexTypeXPU19GroupRadixProcesserIiLi1024ELi16ELi4ELb0ExtjLi4EE12select_groupERA4_iRA4_xiiiPiPx(%"class.at::AtenIpexTypeXPU::GroupRadixProcesser" addrspace(4)* %0, [4 x i32] addrspace(4)* %1, [4 x i64] addrspace(4)* %2, i32 %3, i32 %4, i32 %5, i8 addrspace(4)* %6, i8 addrspace(4)* %7, i64 %8, [4096 x i64] addrspace(4)* %9) {
  %11 = alloca [4 x i16 addrspace(4)*], align 8
  %12 = alloca [4 x i32], align 4
  br label %13

13:                                               ; preds = %20, %10
  %14 = phi i32 [ 0, %10 ], [ %23, %20 ]
  %15 = icmp ult i32 %14, 4
  br i1 %15, label %20, label %16

16:                                               ; preds = %13
  %17 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::GroupRadixProcesser", %"class.at::AtenIpexTypeXPU::GroupRadixProcesser" addrspace(4)* %0, i64 0, i32 2
  %18 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::GroupRadixProcesser", %"class.at::AtenIpexTypeXPU::GroupRadixProcesser" addrspace(4)* %0, i64 0, i32 1
  %19 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::GroupRadixProcesser", %"class.at::AtenIpexTypeXPU::GroupRadixProcesser" addrspace(4)* %0, i64 0, i32 0
  br label %24

20:                                               ; preds = %13
  %21 = zext i32 %14 to i64
  %22 = getelementptr inbounds [4 x i32], [4 x i32] addrspace(4)* %1, i64 0, i64 %21
  store i32 0, i32 addrspace(4)* %22, align 4
  %23 = add nuw nsw i32 %14, 1
  br label %13

24:                                               ; preds = %135, %16
  %25 = phi i32 [ 0, %16 ], [ %116, %135 ]
  %26 = phi i32 [ 0, %16 ], [ 1, %135 ]
  %27 = sub nsw i32 %26, 1
  br label %28

28:                                               ; preds = %32, %24
  %29 = phi i32 [ 0, %24 ], [ %39, %32 ]
  %30 = icmp ult i32 %29, 8
  br i1 %30, label %32, label %31

31:                                               ; preds = %28
  call spir_func void @_Z22__spirv_ControlBarrieriii(i32 0, i32 0, i32 0)
  br label %40

32:                                               ; preds = %28
  %33 = load i8 addrspace(4)*, i8 addrspace(4)* addrspace(4)* %17, align 8
  %34 = zext i32 %29 to i64
  %35 = load i32, i32 addrspace(4)* %18, align 8
  %36 = sext i32 %35 to i64
  %37 = bitcast i8 addrspace(4)* %33 to [8 x [1024 x i32]] addrspace(4)*
  %38 = getelementptr inbounds [8 x [1024 x i32]], [8 x [1024 x i32]] addrspace(4)* %37, i64 0, i64 %34, i64 %36
  store i32 0, i32 addrspace(4)* %38, align 4
  %39 = add nuw nsw i32 %29, 1
  br label %28

40:                                               ; preds = %61, %31
  %41 = phi i32 [ 0, %31 ], [ %62, %61 ]
  %42 = icmp ult i32 %41, 4
  br i1 %42, label %46, label %.preheader5

.preheader5:                                      ; preds = %40
  %43 = load i8 addrspace(4)*, i8 addrspace(4)* addrspace(4)* %17, align 8
  %44 = load i8 addrspace(4)*, i8 addrspace(4)* addrspace(4)* %19, align 8
  %45 = call spir_func i32 @_ZN2at15AtenIpexTypeXPU19group_exclusive_sumIjLi8ELi1024ELi16EEET_RAmlT0_T1__S2_RN4sycl3_V17nd_itemILi1EEE(i8 addrspace(4)* %43, i8 addrspace(4)* %44)
  br label %63

46:                                               ; preds = %40
  %47 = zext i32 %41 to i64
  %48 = getelementptr inbounds [4 x i32], [4 x i32]* %12, i64 0, i64 %47
  store i32 0, i32* %48, align 4
  %49 = shl nuw nsw i32 1, %41
  %50 = and i32 %49, %25
  %51 = icmp eq i32 %50, 0
  br i1 %51, label %61, label %52

52:                                               ; preds = %46
  %53 = getelementptr inbounds [4 x i32], [4 x i32] addrspace(4)* %1, i64 0, i64 %47
  %54 = load i32, i32 addrspace(4)* %53, align 4
  %55 = lshr i32 %54, 1
  %56 = load i8 addrspace(4)*, i8 addrspace(4)* addrspace(4)* %17, align 8
  %57 = zext i32 %55 to i64
  %58 = bitcast i8 addrspace(4)* %56 to [8 x [1024 x [2 x i16]]] addrspace(4)*
  %59 = getelementptr inbounds [8 x [1024 x [2 x i16]]], [8 x [1024 x [2 x i16]]] addrspace(4)* %58, i64 0, i64 %57, i64 undef, i64 undef
  %60 = getelementptr inbounds [4 x i16 addrspace(4)*], [4 x i16 addrspace(4)*]* %11, i64 0, i64 %47
  store i16 addrspace(4)* %59, i16 addrspace(4)** %60, align 8
  store i32 %5, i32* %48, align 4
  br label %61

61:                                               ; preds = %52, %46
  %62 = add nuw nsw i32 %41, 1
  br label %40

63:                                               ; preds = %66, %.preheader5
  %64 = phi i32 [ %73, %66 ], [ 0, %.preheader5 ]
  %65 = icmp ult i32 %64, 8
  br i1 %65, label %66, label %74

66:                                               ; preds = %63
  %67 = load i8 addrspace(4)*, i8 addrspace(4)* addrspace(4)* %17, align 8
  %68 = zext i32 %64 to i64
  %69 = load i32, i32 addrspace(4)* %18, align 8
  %70 = sext i32 %69 to i64
  %71 = bitcast i8 addrspace(4)* %67 to [8 x [1024 x i32]] addrspace(4)*
  %72 = getelementptr inbounds [8 x [1024 x i32]], [8 x [1024 x i32]] addrspace(4)* %71, i64 0, i64 %68, i64 %70
  store i32 %45, i32 addrspace(4)* %72, align 4
  %73 = add nuw nsw i32 %64, 1
  br label %63

74:                                               ; preds = %63, %77
  %75 = phi i32 [ %86, %77 ], [ 0, %63 ]
  %76 = icmp ult i32 %75, 4
  br i1 %76, label %77, label %.loopexit3

77:                                               ; preds = %74
  %78 = zext i32 %75 to i64
  %79 = getelementptr inbounds [4 x i16 addrspace(4)*], [4 x i16 addrspace(4)*]* %11, i64 0, i64 %78
  %80 = load i16 addrspace(4)*, i16 addrspace(4)** %79, align 8
  %81 = load i16, i16 addrspace(4)* %80, align 2
  %82 = zext i16 %81 to i32
  %83 = getelementptr inbounds [4 x i32], [4 x i32]* %12, i64 0, i64 %78
  %84 = load i32, i32* %83, align 4
  %85 = add nsw i32 %84, %82
  store i32 %85, i32* %83, align 4
  %86 = add nuw nsw i32 %75, 1
  br label %74

.loopexit3:                                       ; preds = %74
  %87 = icmp eq i32 %27, 0
  %88 = select i1 %87, i32 %5, i32 undef
  %89 = icmp sgt i32 %88, 0
  br i1 %89, label %.preheader2, label %.loopexit

.preheader2:                                      ; preds = %.loopexit3, %92
  %90 = phi i32 [ %99, %92 ], [ 0, %.loopexit3 ]
  %91 = icmp ult i32 %90, 4
  br i1 %91, label %92, label %.preheader1

92:                                               ; preds = %.preheader2
  %93 = getelementptr inbounds [4 x i32], [4 x i32]* %12, i64 0, i64 0
  %94 = load i32, i32* %93, align 4
  %95 = getelementptr inbounds [4 x i32], [4 x i32] addrspace(4)* %1, i64 0, i64 0
  %96 = load i32, i32 addrspace(4)* %95, align 4
  %97 = sext i32 %94 to i64
  %98 = getelementptr inbounds i32, i32 addrspace(4)* null, i64 %97
  store i32 %96, i32 addrspace(4)* %98, align 4
  %99 = add nuw nsw i32 %90, 1
  br label %.preheader2

.preheader1:                                      ; preds = %.preheader2, %111
  %100 = phi i32 [ %112, %111 ], [ 0, %.preheader2 ]
  %101 = icmp ult i32 %100, 4
  br i1 %101, label %102, label %.loopexit

102:                                              ; preds = %.preheader1
  %103 = zext i32 %100 to i64
  %104 = getelementptr inbounds [4 x i32], [4 x i32]* %12, i64 0, i64 %103
  %105 = load i32, i32* %104, align 4
  %106 = icmp slt i32 %105, %88
  br i1 %106, label %107, label %111

107:                                              ; preds = %102
  %108 = getelementptr inbounds [4 x i64], [4 x i64] addrspace(4)* %2, i64 0, i64 %103
  %109 = load i64, i64 addrspace(4)* %108, align 8
  %110 = bitcast i8 addrspace(4)* %7 to i64 addrspace(4)*
  store i64 %109, i64 addrspace(4)* %110, align 8
  br label %111

111:                                              ; preds = %107, %102
  %112 = add nuw nsw i32 %100, 1
  br label %.preheader1

.loopexit:                                        ; preds = %.preheader1, %.loopexit3
  %113 = icmp eq i32 %88, 0
  br i1 %113, label %145, label %.preheader

.preheader:                                       ; preds = %.loopexit
  %114 = load i32, i32 addrspace(4)* %18, align 8
  br label %115

115:                                              ; preds = %129, %.preheader
  %116 = phi i32 [ 0, %.preheader ], [ %130, %129 ]
  %117 = phi i32 [ 0, %.preheader ], [ %131, %129 ]
  %118 = icmp ult i32 %117, 4
  br i1 %118, label %119, label %132

119:                                              ; preds = %115
  %120 = add nuw nsw i32 %114, %117
  %121 = icmp slt i32 %120, %45
  br i1 %121, label %122, label %129

122:                                              ; preds = %119
  %123 = load i8 addrspace(4)*, i8 addrspace(4)* addrspace(4)* %17, align 8
  %124 = sext i32 %120 to i64
  %125 = bitcast i8 addrspace(4)* %123 to [4096 x i32] addrspace(4)*
  %126 = getelementptr inbounds [4096 x i32], [4096 x i32] addrspace(4)* %125, i64 0, i64 %124
  %127 = load i32, i32 addrspace(4)* %126, align 4
  %128 = getelementptr inbounds [4 x i32], [4 x i32] addrspace(4)* %1, i64 0, i64 0
  store i32 %127, i32 addrspace(4)* %128, align 4
  br label %129

129:                                              ; preds = %122, %119
  %130 = phi i32 [ %5, %122 ], [ 0, %119 ]
  %131 = add nuw nsw i32 %117, 1
  br label %115

132:                                              ; preds = %115
  %133 = load i32, i32 addrspace(4)* %18, align 8
  %134 = shl nsw i32 %133, 2
  br label %135

135:                                              ; preds = %143, %132
  %136 = phi i32 [ 0, %132 ], [ %144, %143 ]
  %137 = icmp ult i32 %136, 4
  br i1 %137, label %138, label %24

138:                                              ; preds = %135
  %139 = icmp slt i32 %134, %45
  br i1 %139, label %140, label %143

140:                                              ; preds = %138
  %141 = zext i32 %136 to i64
  %142 = getelementptr inbounds [4 x i64], [4 x i64] addrspace(4)* %2, i64 0, i64 %141
  store i64 0, i64 addrspace(4)* %142, align 8
  br label %143

143:                                              ; preds = %140, %138
  %144 = add nuw nsw i32 %136, 1
  br label %135

145:                                              ; preds = %.loopexit
  ret void
}

declare spir_func i32 @_Z40__spirv_BuiltInSubgroupLocalInvocationIdv()

declare spir_func i32 @_Z25__spirv_BuiltInSubgroupIdv()

declare spir_func i64 @_Z25__spirv_BuiltInGlobalSizei(i32)

declare spir_func i64 @_Z28__spirv_BuiltInNumWorkgroupsi(i32)

declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

declare spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi()

declare spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi()

declare spir_func i64 @_Z28__spirv_BuiltInWorkgroupSizei(i32)

attributes #0 = { argmemonly nofree nosync nounwind willreturn }
attributes #1 = { inaccessiblememonly nofree nosync nounwind willreturn }

!0 = !{i32 16}
