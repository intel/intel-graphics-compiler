; REQUIRES: regkeys, mtl-supported, llvm-16-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device mtl -options "-igc_opts 'EnableOpaquePointersBackend=1,DisableCodeScheduling=1,VISAOptions=-asmToConsole'" &> %t_output.ll
; RUN: FileCheck --input-file %t_output.ll %s

; LLVM with typed pointers:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device mtl -options "-igc_opts 'DisableCodeScheduling=1,VISAOptions=-asmToConsole'" &> %t_output.ll
; RUN: FileCheck --input-file %t_output.ll %s

; This test checks that after kernel recompilation there is no more spills

; CHECK://.kernel _ZTSN2at15AtenIpexTypeXPU4impl35FastGroupRadixSortImplKernelFunctorIbxLi1024ELb0ELb1EtLi32ELi4ENS0_19GroupRadixProcesserIbLi1024ELi32ELi4ELb0EttjLi4EEEbEE
; CHECK-NOT://.spill size
; CHECK: end of thread

; CHECK: warning: [RetryManager] Start recompilation of the kernel

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

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture) #0

; Function Attrs: inaccessiblememonly nofree nosync nounwind willreturn
declare void @llvm.assume(i1 noundef) #1

declare spir_func void @_Z22__spirv_ControlBarrieriii(i32, i32, i32)

define linkonce_odr spir_func i32 @_ZN2at15AtenIpexTypeXPU19group_exclusive_sumIjLi8ELi1024ELi32EEET_RAmlT0_T1__S2_RN4sycl3_V17nd_itemILi1EEE([8 x [1024 x i32]] addrspace(4)* %0, %"class.sycl::_V1::nd_item" addrspace(4)* %1) {
  %3 = alloca [8 x i32], align 4
  %4 = getelementptr inbounds %"class.sycl::_V1::nd_item", %"class.sycl::_V1::nd_item" addrspace(4)* %1, i64 0, i32 1, i32 0, i32 1
  %5 = bitcast %"class.sycl::_V1::range" addrspace(4)* %4 to i64 addrspace(4)*
  %6 = load i64, i64 addrspace(4)* %5, align 8
  %7 = trunc i64 %6 to i32
  %8 = call spir_func i32 @_Z40__spirv_BuiltInSubgroupLocalInvocationIdv()
  %9 = call spir_func i32 @_Z25__spirv_BuiltInSubgroupIdv()
  %10 = shl nsw i32 %7, 3
  br label %11

11:                                               ; preds = %29, %2
  %12 = phi i32 [ 0, %2 ], [ %37, %29 ]
  %13 = phi i32 [ 0, %2 ], [ %38, %29 ]
  %14 = icmp ult i32 %13, 8
  br i1 %14, label %29, label %.preheader

.preheader:                                       ; preds = %11
  br label %15

15:                                               ; preds = %19, %.preheader
  %16 = phi i32 [ %24, %19 ], [ %12, %.preheader ]
  %17 = phi i32 [ %25, %19 ], [ 0, %.preheader ]
  %18 = icmp ult i32 %17, 5
  br i1 %18, label %19, label %26

19:                                               ; preds = %15
  %20 = shl nuw nsw i32 1, %17
  %21 = call spir_func i32 @_Z30__spirv_SubgroupShuffleUpINTELiij(i32 %16, i32 %16, i32 %20)
  %22 = icmp ugt i32 %20, %8
  %23 = select i1 %22, i32 0, i32 %21
  %24 = add i32 %16, %23
  %25 = add nuw nsw i32 %17, 1
  br label %15

26:                                               ; preds = %15
  %27 = sub i32 %16, %12
  %28 = icmp eq i32 %8, 1
  br i1 %28, label %39, label %43

29:                                               ; preds = %11
  %30 = zext i32 %13 to i64
  %31 = getelementptr inbounds [8 x i32], [8 x i32]* %3, i64 0, i64 %30
  store i32 %12, i32* %31, align 4
  %32 = add nuw nsw i32 %10, %13
  %33 = sext i32 %32 to i64
  %34 = bitcast [8 x [1024 x i32]] addrspace(4)* %0 to [8192 x i32] addrspace(4)*
  %35 = getelementptr inbounds [8192 x i32], [8192 x i32] addrspace(4)* %34, i64 0, i64 %33
  %36 = load i32, i32 addrspace(4)* %35, align 4
  %37 = add i32 %12, %36
  %38 = add nuw nsw i32 %13, 1
  br label %11

39:                                               ; preds = %26
  %40 = sext i32 %9 to i64
  %41 = bitcast [8 x [1024 x i32]] addrspace(4)* %0 to [8192 x i32] addrspace(4)*
  %42 = getelementptr inbounds [8192 x i32], [8192 x i32] addrspace(4)* %41, i64 0, i64 %40
  store i32 0, i32 addrspace(4)* %42, align 4
  br label %43

43:                                               ; preds = %39, %26
  br label %44

44:                                               ; preds = %51, %43
  %45 = phi i32 [ 0, %43 ], [ %57, %51 ]
  %46 = phi i32 [ 0, %43 ], [ %53, %51 ]
  %47 = phi i32 [ 0, %43 ], [ %58, %51 ]
  %48 = icmp ult i32 %47, 32
  br i1 %48, label %51, label %49

49:                                               ; preds = %44
  %50 = add i32 %27, %46
  br label %59

51:                                               ; preds = %44
  %52 = icmp eq i32 %9, %47
  %53 = select i1 %52, i32 %45, i32 %46
  %54 = bitcast [8 x [1024 x i32]] addrspace(4)* %0 to [8192 x i32] addrspace(4)*
  %55 = getelementptr inbounds [8192 x i32], [8192 x i32] addrspace(4)* %54, i64 0, i64 undef
  %56 = load i32, i32 addrspace(4)* %55, align 4
  %57 = add i32 %45, %56
  %58 = add nuw nsw i32 %47, 1
  br label %44

59:                                               ; preds = %63, %49
  %60 = phi i32 [ 0, %49 ], [ %72, %63 ]
  %61 = icmp ult i32 %60, 8
  br i1 %61, label %63, label %62

62:                                               ; preds = %59
  ret i32 0

63:                                               ; preds = %59
  %64 = zext i32 %60 to i64
  %65 = getelementptr inbounds [8 x i32], [8 x i32]* %3, i64 0, i64 %64
  %66 = load i32, i32* %65, align 4
  %67 = add i32 %50, %66
  %68 = add nuw nsw i32 %7, %60
  %69 = sext i32 %68 to i64
  %70 = bitcast [8 x [1024 x i32]] addrspace(4)* %0 to [8192 x i32] addrspace(4)*
  %71 = getelementptr inbounds [8192 x i32], [8192 x i32] addrspace(4)* %70, i64 0, i64 %69
  store i32 %67, i32 addrspace(4)* %71, align 4
  %72 = add nuw nsw i32 %60, 1
  br label %59
}

declare spir_func i32 @_Z30__spirv_SubgroupShuffleUpINTELiij(i32, i32, i32)

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture) #0

define spir_kernel void @_ZTSN2at15AtenIpexTypeXPU4impl35FastGroupRadixSortImplKernelFunctorIbxLi1024ELb0ELb1EtLi32ELi4ENS0_19GroupRadixProcesserIbLi1024ELi32ELi4ELb0EttjLi4EEEbEE(i8 addrspace(1)* %0, i8 addrspace(1)* %1, i64 addrspace(1)* %2, i32 %3, [8 x [1024 x i32]] addrspace(3)* %4, i64* %5, i8 %6, i64 addrspace(1)* %7, i64 %8) !intel_reqd_sub_group_size !0 {
  %10 = alloca [4 x i16 addrspace(4)*], align 8
  %11 = alloca [4 x i32], align 4
  %12 = alloca [4 x i8], align 1
  %13 = alloca [4 x i16], align 2
  %14 = alloca %"class.sycl::_V1::nd_item", align 8
  %15 = call spir_func i64 @_Z25__spirv_BuiltInGlobalSizei(i32 0)
  %16 = call spir_func i64 @_Z28__spirv_BuiltInWorkgroupSizei(i32 0)
  %17 = call spir_func i64 @_Z28__spirv_BuiltInNumWorkgroupsi(i32 0)
  %18 = call spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi()
  %19 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %20 = call spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 0)
  store i64 %15, i64* undef, align 1
  store i64 %19, i64* undef, align 1
  store i64 undef, i64* undef, align 1
  store i64 %16, i64* %5, align 1
  %21 = bitcast %"class.sycl::_V1::nd_item"* %14 to i64*
  %22 = getelementptr inbounds i64, i64* %21, i64 4
  store i64 %20, i64* %22, align 1
  store i64 %17, i64* %5, align 1
  %23 = addrspacecast %"class.sycl::_V1::nd_item"* %14 to %"class.sycl::_V1::nd_item" addrspace(4)*
  %24 = getelementptr inbounds i8, i8 addrspace(1)* %1, i64 %8
  %25 = trunc i64 %20 to i32
  %26 = shl i32 %25, 2
  br label %27

27:                                               ; preds = %118, %9
  %28 = phi i32 [ 0, %9 ], [ %121, %118 ]
  %29 = icmp ult i32 %28, 4
  br i1 %29, label %110, label %30

30:                                               ; preds = %27
  br label %31

31:                                               ; preds = %109, %30
  %32 = phi i1 [ true, %30 ], [ false, %109 ]
  br label %33

33:                                               ; preds = %37, %31
  %34 = phi i32 [ 0, %31 ], [ %40, %37 ]
  %35 = icmp ult i32 %34, 8
  br i1 %35, label %37, label %36

36:                                               ; preds = %33
  br label %41

37:                                               ; preds = %33
  %38 = zext i32 %34 to i64
  %39 = getelementptr inbounds [8 x [1024 x i32]], [8 x [1024 x i32]] addrspace(3)* %4, i64 0, i64 %38, i64 %20
  store i32 0, i32 addrspace(3)* %39, align 4
  %40 = add nuw nsw i32 %34, 1
  br label %33

41:                                               ; preds = %46, %36
  %42 = phi i32 [ 0, %36 ], [ %59, %46 ]
  %43 = icmp ult i32 %42, 4
  br i1 %43, label %46, label %44

44:                                               ; preds = %41
  %45 = call spir_func i32 @_ZN2at15AtenIpexTypeXPU19group_exclusive_sumIjLi8ELi1024ELi32EEET_RAmlT0_T1__S2_RN4sycl3_V17nd_itemILi1EEE([8 x [1024 x i32]] addrspace(4)* addrspacecast ([8 x [1024 x i32]] addrspace(3)* null to [8 x [1024 x i32]] addrspace(4)*), %"class.sycl::_V1::nd_item" addrspace(4)* %23)
  br label %60

46:                                               ; preds = %41
  %47 = zext i32 %42 to i64
  %48 = getelementptr inbounds [4 x i8], [4 x i8]* %12, i64 0, i64 %47
  %49 = load i8, i8* %48, align 1
  %50 = zext i8 %49 to i32
  %51 = zext i32 %50 to i64
  %52 = bitcast [8 x [1024 x i32]] addrspace(3)* %4 to [8 x [1024 x [2 x i16]]] addrspace(3)*
  %53 = getelementptr inbounds [8 x [1024 x [2 x i16]]], [8 x [1024 x [2 x i16]]] addrspace(3)* %52, i64 0, i64 %51, i64 %20, i64 0
  %54 = addrspacecast i16 addrspace(3)* %53 to i16 addrspace(4)*
  %55 = getelementptr inbounds [4 x i16 addrspace(4)*], [4 x i16 addrspace(4)*]* %10, i64 0, i64 %47
  store i16 addrspace(4)* %54, i16 addrspace(4)** %55, align 8
  %56 = load i16, i16 addrspace(3)* %53, align 2
  %57 = zext i16 %56 to i32
  %58 = getelementptr inbounds [4 x i32], [4 x i32]* %11, i64 0, i64 %47
  store i32 %57, i32* %58, align 4
  %59 = add nuw nsw i32 %42, 1
  br label %41

60:                                               ; preds = %61, %44
  br i1 false, label %61, label %.preheader1

.preheader1:                                      ; preds = %60
  br label %62

61:                                               ; preds = %60
  br label %60

62:                                               ; preds = %66, %.preheader1
  %63 = phi i32 [ %69, %66 ], [ 0, %.preheader1 ]
  %64 = icmp ult i32 %63, 8
  br i1 %64, label %66, label %65

65:                                               ; preds = %62
  br label %70

66:                                               ; preds = %62
  %67 = zext i32 %63 to i64
  %68 = getelementptr inbounds [8 x [1024 x i32]], [8 x [1024 x i32]] addrspace(3)* %4, i64 0, i64 %67, i64 %20
  store i32 0, i32 addrspace(3)* %68, align 4
  %69 = add nuw nsw i32 %63, 1
  br label %62

70:                                               ; preds = %73, %65
  %71 = phi i32 [ 0, %65 ], [ %82, %73 ]
  %72 = icmp ult i32 %71, 4
  br i1 %72, label %73, label %83

73:                                               ; preds = %70
  %74 = zext i32 %71 to i64
  %75 = getelementptr inbounds [4 x i16 addrspace(4)*], [4 x i16 addrspace(4)*]* %10, i64 0, i64 %74
  %76 = load i16 addrspace(4)*, i16 addrspace(4)** %75, align 8
  %77 = load i16, i16 addrspace(4)* %76, align 2
  %78 = zext i16 %77 to i32
  %79 = getelementptr inbounds [4 x i32], [4 x i32]* %11, i64 0, i64 %74
  %80 = load i32, i32* %79, align 4
  %81 = add nsw i32 %80, %78
  store i32 %81, i32* %79, align 4
  %82 = add nuw nsw i32 %71, 1
  br label %70

83:                                               ; preds = %70
  br label %84

84:                                               ; preds = %88, %83
  %85 = phi i32 [ 0, %83 ], [ %96, %88 ]
  %86 = icmp ult i32 %85, 4
  br i1 %86, label %88, label %87

87:                                               ; preds = %84
  br label %97

88:                                               ; preds = %84
  %89 = zext i32 %85 to i64
  %90 = getelementptr inbounds [4 x i16], [4 x i16]* %13, i64 0, i64 %89
  %91 = load i16, i16* %90, align 2
  %92 = getelementptr inbounds [4 x i32], [4 x i32]* %11, i64 0, i64 %89
  %93 = load i32, i32* %92, align 4
  %94 = sext i32 %93 to i64
  %95 = getelementptr inbounds [4096 x i16], [4096 x i16] addrspace(3)* null, i64 0, i64 %94
  store i16 %91, i16 addrspace(3)* %95, align 2
  %96 = add nuw nsw i32 %85, 1
  br label %84

97:                                               ; preds = %100, %87
  %98 = phi i32 [ 0, %87 ], [ %108, %100 ]
  %99 = icmp ult i32 %98, 4
  br i1 %99, label %100, label %109

100:                                              ; preds = %97
  %101 = add nuw nsw i32 %25, %98
  %102 = sext i32 %101 to i64
  %103 = bitcast [8 x [1024 x i32]] addrspace(3)* %4 to [4096 x i16] addrspace(3)*
  %104 = getelementptr inbounds [4096 x i16], [4096 x i16] addrspace(3)* %103, i64 0, i64 %102
  %105 = load i16, i16 addrspace(3)* %104, align 2
  %106 = zext i32 %98 to i64
  %107 = getelementptr inbounds [4 x i16], [4 x i16]* %13, i64 0, i64 %106
  store i16 %105, i16* %107, align 2
  %108 = add nuw nsw i32 %98, 1
  br label %97

109:                                              ; preds = %97
  br i1 %32, label %31, label %.preheader

.preheader:                                       ; preds = %109
  br label %122

110:                                              ; preds = %27
  %111 = zext i32 %28 to i64
  %112 = add nuw i32 %26, %28
  %113 = icmp slt i32 %112, 4096
  br i1 %113, label %114, label %118

114:                                              ; preds = %110
  %115 = sext i32 %112 to i64
  %116 = trunc i64 %115 to i16
  %117 = getelementptr inbounds [4 x i16], [4 x i16]* %13, i64 0, i64 %111
  store i16 %116, i16* %117, align 2
  br label %118

118:                                              ; preds = %114, %110
  %119 = phi i8 [ %6, %114 ], [ 0, %110 ]
  %120 = getelementptr inbounds [4 x i8], [4 x i8]* %12, i64 0, i64 %111
  store i8 %119, i8* %120, align 1
  %121 = add nuw nsw i32 %28, 1
  br label %27

122:                                              ; preds = %138, %.preheader
  %123 = phi i32 [ %139, %138 ], [ 0, %.preheader ]
  %124 = icmp ult i32 %123, 4
  br i1 %124, label %125, label %140

125:                                              ; preds = %122
  %126 = add nuw i32 %26, %123
  %127 = icmp slt i32 %126, 4096
  br i1 %127, label %128, label %138

128:                                              ; preds = %125
  %129 = zext i32 %123 to i64
  %130 = getelementptr inbounds [4 x i8], [4 x i8]* %12, i64 0, i64 %129
  %131 = load i8, i8* %130, align 1
  %132 = sext i32 %126 to i64
  %133 = getelementptr inbounds i8, i8 addrspace(1)* %24, i64 %132
  store i8 %131, i8 addrspace(1)* %133, align 1
  %134 = getelementptr inbounds [4 x i16], [4 x i16]* %13, i64 0, i64 %129
  %135 = load i16, i16* %134, align 2
  %136 = zext i16 %135 to i64
  %137 = getelementptr inbounds i64, i64 addrspace(1)* %7, i64 %8
  store i64 %136, i64 addrspace(1)* %137, align 8
  br label %138

138:                                              ; preds = %128, %125
  %139 = add nuw nsw i32 %123, 1
  br label %122

140:                                              ; preds = %122
  ret void
}

declare spir_func i32 @_Z40__spirv_BuiltInSubgroupLocalInvocationIdv()

declare spir_func i32 @_Z25__spirv_BuiltInSubgroupIdv()

declare spir_func i64 @_Z25__spirv_BuiltInGlobalSizei(i32)

declare spir_func i64 @_Z28__spirv_BuiltInNumWorkgroupsi(i32)

declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

declare spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32)

declare spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi()

declare spir_func i64 @_Z28__spirv_BuiltInWorkgroupSizei(i32)

attributes #0 = { argmemonly nofree nosync nounwind willreturn }
attributes #1 = { inaccessiblememonly nofree nosync nounwind willreturn }

!igc.functions = !{}

!0 = !{i32 32}
