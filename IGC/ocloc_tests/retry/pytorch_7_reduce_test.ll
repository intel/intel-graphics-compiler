; REQUIRES: regkeys,pvc-supported, test-fix
; RUN: llvm-as %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options "-igc_opts 'VISAOptions=-asmToConsole'" &> %t_output.ll
; RUN: FileCheck --input-file %t_output.ll %s

; ATTENTION: if your change lowers spill size significantly congratulations! just adjust the numbers

; normal version
; CHECK://.kernel _ZTSN2at15AtenIpexTypeXPU13reduce_kernelILi1ENS0_8ReduceOpIN3c107complexIdEENS0_14func_wrapper_tIS5_NS0_13ReduceProdOpsIS5_EEEEjS5_Li4EEEEE
; I'm trying to match 5 consecutive numbers starting with 2: 23477 for example, not 234770 and not 2347,
; lower boundary is set by {4} and upper boundary by matching EOL character {{$}}
; CHECK://.spill size 1[[A:[0-9]{4}]]{{$}}
; CHECK: end of thread

; retry version
; CHECK://.kernel _ZTSN2at15AtenIpexTypeXPU13reduce_kernelILi1ENS0_8ReduceOpIN3c107complexIdEENS0_14func_wrapper_tIS5_NS0_13ReduceProdOpsIS5_EEEEjS5_Li4EEEEE
; CHECK://.spill size 5[[B:[0-9]{3}]]{{$}}
; CHECK: end of thread

; ModuleID = 'reduced.ll'
source_filename = "reduced.ll"

%"struct.at::AtenIpexTypeXPU::ReduceOp.61" = type { %"class.std::tuple.59", [15 x i8], %"struct.c10::complex", %"struct.at::AtenIpexTypeXPU::ReduceConfig", %struct.OffsetCalculator.505, %struct.OffsetCalculator, i8 addrspace(4)*, [2 x i8 addrspace(4)*], i8 addrspace(4)*, i8 addrspace(4)*, i8 addrspace(4)*, i64, i8, i8, i32 }
%"class.std::tuple.59" = type { %"struct.std::_Tuple_val.60" }
%"struct.std::_Tuple_val.60" = type { i8 }
%"struct.c10::complex" = type { double, double }
%"struct.at::AtenIpexTypeXPU::ReduceConfig" = type { i32, i32, i32, i32, i32, i32, [3 x i32], [2 x i32], i32, i32, i32, i8, i32, i32 }
%struct.OffsetCalculator.505 = type { i32, [12 x %struct.IntDivider], [12 x [1 x i32]] }
%struct.IntDivider = type { i32, i32, i32 }
%struct.OffsetCalculator = type { i32, [12 x %struct.IntDivider], [12 x [2 x i32]] }
%"class.sycl::_V1::nd_item.1231" = type { %"class.sycl::_V1::item.0.1229", %"class.sycl::_V1::item.5", %"class.sycl::_V1::group.1230" }
%"class.sycl::_V1::item.0.1229" = type { %"struct.sycl::_V1::detail::ItemBase.1.1228" }
%"struct.sycl::_V1::detail::ItemBase.1.1228" = type { %"class.sycl::_V1::range.2", %"class.sycl::_V1::range.2", %"class.sycl::_V1::range.2" }
%"class.sycl::_V1::range.2" = type { %"struct.at::native::Memory::aligned_vector_loop.396" }
%"struct.at::native::Memory::aligned_vector_loop.396" = type { [2 x i64] }
%"class.sycl::_V1::item.5" = type { %"struct.sycl::_V1::detail::ItemBase.6" }
%"struct.sycl::_V1::detail::ItemBase.6" = type { %"class.sycl::_V1::range.2", %"class.sycl::_V1::range.2" }
%"class.sycl::_V1::group.1230" = type { %"class.sycl::_V1::range.2", %"class.sycl::_V1::range.2", %"class.sycl::_V1::range.2", %"class.sycl::_V1::range.2" }
%structtype = type { i64, i64 }
%"struct.at::native::Memory::aligned_vector_loop.65" = type { [16 x %"struct.c10::complex"] }
%"struct.at::native::Memory::aligned_vector_loop.66.2511" = type { [8 x %"struct.c10::complex"] }
%"struct.at::AtenIpexTypeXPU::__generated_ReduceOp.288" = type { %"class.std::tuple.59", [15 x i8], %"struct.c10::complex", %"struct.at::AtenIpexTypeXPU::ReduceConfig", %struct.OffsetCalculator.505, %struct.OffsetCalculator, i8 addrspace(1)*, [2 x i8 addrspace(1)*], i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, i64, i8, i8, i32 }
%"class.at::AtenIpexTypeXPU::reduce_kernel.289" = type { %"struct.at::AtenIpexTypeXPU::ReduceOp.61", %"class.sycl::_V1::local_accessor", %"class.sycl::_V1::local_accessor" }
%"class.sycl::_V1::local_accessor" = type { %"class.sycl::_V1::local_accessor_base" }
%"class.sycl::_V1::local_accessor_base" = type { %"struct.sycl::_V1::detail::ItemBase", i8 addrspace(3)* }
%"struct.sycl::_V1::detail::ItemBase" = type { %"class.sycl::_V1::range", %"class.sycl::_V1::range", %"class.sycl::_V1::range" }
%"class.sycl::_V1::range" = type { %"class.sycl::_V1::detail::array" }
%"class.sycl::_V1::detail::array" = type { [1 x i64] }
%"struct.at::native::Memory::aligned_vector_loop" = type { [1 x %"struct.c10::complex"] }
%"struct.xpu::dpcpp::Array.5" = type { [2 x i32], [8 x i8] }

@.str.76 = external dso_local addrspace(1) constant [44 x i8]
@0 = external dso_local addrspace(2) constant [16 x i8]
@1 = external dso_local addrspace(2) constant [16 x i8]
@2 = external dso_local addrspace(2) constant [16 x i8]
@3 = external dso_local addrspace(2) constant [16 x i8]

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture) #0

; Function Attrs: argmemonly nofree nounwind willreturn
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* noalias nocapture writeonly, i8* noalias nocapture readonly, i64, i1 immarg) #1

; Function Attrs: argmemonly nofree nounwind willreturn
declare void @llvm.memcpy.p0i8.p2i8.i64(i8* noalias nocapture writeonly, i8 addrspace(2)* noalias nocapture readonly, i64, i1 immarg) #1

; Function Attrs: inaccessiblememonly nofree nosync nounwind willreturn
declare void @llvm.assume(i1 noundef) #2

declare spir_func i32 @_Z20__spirv_ocl_u_mul_hijj(i32, i32)

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture) #0

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare i32 @llvm.fshl.i32(i32, i32, i32) #3

define linkonce_odr spir_func void @_ZNK2at15AtenIpexTypeXPU8ReduceOpIN3c107complexIdEENS0_14func_wrapper_tIS4_NS0_13ReduceProdOpsIS4_EEEEjS4_Li4EE33input_vectorized_item_reduce_implILi16EEES4_N4sycl3_V17nd_itemILi2EEEPKS4_(%"struct.c10::complex" addrspace(4)* %0, %"struct.at::AtenIpexTypeXPU::ReduceOp.61" addrspace(4)* %1, %"class.sycl::_V1::nd_item.1231"* %2, i8 addrspace(4)* %3, %"struct.c10::complex" addrspace(4)* %4, i64 %5, i64* %6, %"struct.c10::complex" addrspace(4)* %7, %structtype addrspace(4)* %8, %structtype addrspace(4)* %9, %structtype addrspace(4)* %10) {
  %12 = alloca [16 x %"struct.c10::complex"], align 16
  %13 = alloca %"struct.at::native::Memory::aligned_vector_loop.65", align 256
  %14 = load i32, i32 addrspace(4)* null, align 4
  %15 = ptrtoint i8 addrspace(4)* %3 to i64
  %16 = trunc i64 %15 to i32
  %17 = and i32 %16, 1
  %18 = icmp eq i32 %17, 0
  br i1 %18, label %23, label %19

19:                                               ; preds = %11
  %20 = getelementptr inbounds %"class.sycl::_V1::nd_item.1231", %"class.sycl::_V1::nd_item.1231"* %2, i64 0, i32 1, i32 0, i32 1, i32 0, i32 0, i64 1
  %21 = load i64, i64* %20, align 8
  %22 = icmp ult i64 %21, 1
  br i1 %22, label %common.ret, label %23

common.ret:                                       ; preds = %19, %122
  ret void

23:                                               ; preds = %19, %11
  %24 = phi i64 [ undef, %11 ], [ %21, %19 ]
  %25 = phi i32 [ %14, %11 ], [ 0, %19 ]
  %26 = phi i8 addrspace(4)* [ %3, %11 ], [ null, %19 ]
  %27 = bitcast %"class.sycl::_V1::nd_item.1231"* %2 to i64*
  %28 = getelementptr inbounds i64, i64* %27, i64 8
  %29 = load i64, i64* %28, align 8
  %30 = trunc i64 %24 to i32
  %31 = trunc i64 %29 to i32
  %32 = mul nsw i32 %14, %30
  %33 = getelementptr inbounds %"struct.at::AtenIpexTypeXPU::ReduceOp.61", %"struct.at::AtenIpexTypeXPU::ReduceOp.61" addrspace(4)* %1, i64 0, i32 3, i32 6, i64 1
  %34 = load i32, i32 addrspace(4)* %33, align 4
  %35 = mul nsw i32 %34, %31
  %36 = add nsw i32 %32, %35
  %37 = getelementptr inbounds [16 x %"struct.c10::complex"], [16 x %"struct.c10::complex"]* %12, i64 0, i64 0
  %38 = getelementptr inbounds [16 x %"struct.c10::complex"], [16 x %"struct.c10::complex"]* %12, i64 1
  br label %39

39:                                               ; preds = %39, %23
  %40 = phi %"struct.c10::complex"* [ %37, %23 ], [ %43, %39 ]
  %41 = getelementptr inbounds %"struct.c10::complex", %"struct.c10::complex"* %40, i64 0, i32 0
  %42 = bitcast double* %41 to i8*
  call void @llvm.memcpy.p0i8.p2i8.i64(i8* %42, i8 addrspace(2)* undef, i64 16, i1 false)
  %43 = getelementptr inbounds %"struct.c10::complex", %"struct.c10::complex"* %40, i64 1
  %44 = bitcast [16 x %"struct.c10::complex"]* %38 to %"struct.c10::complex"*
  %45 = ptrtoint %"struct.c10::complex"* %40 to i64
  %46 = ptrtoint %"struct.c10::complex"* %44 to i64
  %47 = icmp eq i64 %45, %46
  br i1 %47, label %48, label %39

48:                                               ; preds = %39
  %49 = bitcast [16 x %"struct.c10::complex"]* %12 to %structtype*
  %50 = getelementptr inbounds %structtype, %structtype* %49, i64 0, i32 0
  store i64 0, i64* %50, align 8
  store i64 0, i64* inttoptr (i64 8 to i64*), align 8
  br label %51

51:                                               ; preds = %61, %48
  %52 = phi i32 [ 0, %48 ], [ %67, %61 ]
  %53 = icmp ult i32 %52, 16
  br i1 %53, label %61, label %54

54:                                               ; preds = %51, %54
  %55 = phi i64 [ %59, %54 ], [ 0, %51 ]
  %56 = bitcast %"struct.at::native::Memory::aligned_vector_loop.65"* %13 to %"struct.c10::complex"*
  %57 = getelementptr inbounds %"struct.c10::complex", %"struct.c10::complex"* %56, i64 %55
  %58 = bitcast %"struct.c10::complex"* %57 to i8*
  call void @llvm.memcpy.p0i8.p2i8.i64(i8* %58, i8 addrspace(2)* null, i64 1, i1 false)
  %59 = add nuw nsw i64 %55, 1
  %60 = icmp eq i64 %55, 16
  br i1 %60, label %.preheader, label %54

61:                                               ; preds = %51
  %62 = zext i32 %52 to i64
  %63 = getelementptr inbounds [16 x %"struct.c10::complex"], [16 x %"struct.c10::complex"]* %12, i64 0, i64 %62
  %64 = bitcast %"struct.c10::complex"* %63 to %structtype*
  %65 = getelementptr inbounds %structtype, %structtype* %64, i64 0, i32 0
  store i64 0, i64* %65, align 8
  %66 = getelementptr inbounds %structtype, %structtype* %64, i64 0, i32 1
  store i64 0, i64* %66, align 8
  %67 = add nuw nsw i32 %52, 1
  br label %51, !llvm.loop !0

.preheader:                                       ; preds = %54, %79
  %68 = phi i32 [ %80, %79 ], [ %36, %54 ]
  %69 = icmp ult i32 %68, %25
  br i1 %69, label %70, label %106

70:                                               ; preds = %.preheader
  %71 = zext i32 %68 to i64
  %72 = bitcast i8 addrspace(4)* %26 to %"struct.at::native::Memory::aligned_vector_loop.65" addrspace(4)*
  %73 = getelementptr inbounds %"struct.at::native::Memory::aligned_vector_loop.65", %"struct.at::native::Memory::aligned_vector_loop.65" addrspace(4)* %72, i64 %71
  %74 = bitcast %"struct.at::native::Memory::aligned_vector_loop.65"* %13 to i8*
  %75 = bitcast %"struct.at::native::Memory::aligned_vector_loop.65" addrspace(4)* %73 to i8 addrspace(4)*
  call void @llvm.memcpy.p0i8.p4i8.i64(i8* %74, i8 addrspace(4)* align 256 %75, i64 256, i1 false)
  br label %76

76:                                               ; preds = %81, %70
  %77 = phi i32 [ 0, %70 ], [ %105, %81 ]
  %78 = icmp ult i32 %77, 16
  br i1 %78, label %81, label %79

79:                                               ; preds = %76
  %80 = add i32 %68, %14
  br label %.preheader

81:                                               ; preds = %76
  %82 = zext i32 %77 to i64
  %83 = getelementptr inbounds [16 x %"struct.c10::complex"], [16 x %"struct.c10::complex"]* %12, i64 0, i64 %82
  %84 = bitcast %"struct.c10::complex"* %83 to %structtype*
  %85 = getelementptr inbounds %structtype, %structtype* %84, i64 0, i32 0
  %86 = bitcast i64* %85 to double*
  %87 = load double, double* %86, align 8
  %88 = getelementptr inbounds %structtype, %structtype* %84, i64 0, i32 1
  %89 = bitcast i64* %88 to double*
  %90 = load double, double* %89, align 8
  %91 = getelementptr inbounds %"struct.at::native::Memory::aligned_vector_loop.65", %"struct.at::native::Memory::aligned_vector_loop.65"* %13, i64 0, i32 0, i64 %82
  %92 = bitcast %"struct.c10::complex"* %91 to %structtype*
  %93 = getelementptr inbounds %structtype, %structtype* %92, i64 0, i32 0
  %94 = bitcast i64* %93 to double*
  %95 = load double, double* %94, align 8
  %96 = getelementptr inbounds %structtype, %structtype* %92, i64 0, i32 1
  %97 = bitcast i64* %96 to double*
  %98 = load double, double* %97, align 8
  %99 = fmul fast double %87, %95
  %100 = fmul fast double %90, %98
  %101 = fsub fast double %99, %100
  %102 = fmul fast double %87, %98
  %103 = fmul fast double %90, %95
  %104 = fadd fast double %102, %103
  store double %101, double* %86, align 8
  store double %104, double* %89, align 8
  %105 = add nuw nsw i32 %77, 1
  br label %76, !llvm.loop !2

106:                                              ; preds = %.preheader
  %107 = icmp eq i32 %34, 0
  br i1 %107, label %108, label %113

108:                                              ; preds = %106
  %109 = and i32 %14, 1
  %110 = add i32 %109, %30
  %111 = icmp ult i32 %110, 1
  %112 = xor i1 %111, true
  call void @llvm.assume(i1 %112)
  br label %113

113:                                              ; preds = %108, %106
  %114 = bitcast i64* %50 to double*
  %115 = load double, double* %114, align 8
  %116 = load double, double* inttoptr (i64 8 to double*), align 8
  br label %117

117:                                              ; preds = %123, %113
  %118 = phi double [ %116, %113 ], [ %138, %123 ]
  %119 = phi double [ %115, %113 ], [ %135, %123 ]
  %120 = phi i32 [ 0, %113 ], [ %139, %123 ]
  %121 = icmp ult i32 %120, 16
  br i1 %121, label %123, label %122

122:                                              ; preds = %117
  store double %119, double addrspace(4)* null, align 8
  br label %common.ret

123:                                              ; preds = %117
  %124 = zext i32 %120 to i64
  %125 = getelementptr inbounds [16 x %"struct.c10::complex"], [16 x %"struct.c10::complex"]* %12, i64 0, i64 %124
  %126 = bitcast %"struct.c10::complex"* %125 to %structtype*
  %127 = getelementptr inbounds %structtype, %structtype* %126, i64 0, i32 0
  %128 = bitcast i64* %127 to double*
  %129 = load double, double* %128, align 8
  %130 = getelementptr inbounds %structtype, %structtype* %126, i64 0, i32 1
  %131 = bitcast i64* %130 to double*
  %132 = load double, double* %131, align 8
  %133 = fmul fast double %119, %129
  %134 = fmul fast double %118, %132
  %135 = fsub fast double %133, %134
  %136 = fmul fast double %115, %132
  %137 = fmul fast double %118, %129
  %138 = fadd fast double %136, %137
  %139 = add nuw nsw i32 %120, 1
  br label %117, !llvm.loop !3
}

; Function Attrs: argmemonly nofree nounwind willreturn
declare void @llvm.memcpy.p0i8.p4i8.i64(i8* noalias nocapture writeonly, i8 addrspace(4)* noalias nocapture readonly, i64, i1 immarg) #1

define linkonce_odr spir_func void @_ZNK2at15AtenIpexTypeXPU8ReduceOpIN3c107complexIdEENS0_14func_wrapper_tIS4_NS0_13ReduceProdOpsIS4_EEEEjS4_Li4EE33input_vectorized_item_reduce_implILi8EEES4_N4sycl3_V17nd_itemILi2EEEPKS4_(%"struct.c10::complex" addrspace(4)* %0, %"struct.at::AtenIpexTypeXPU::ReduceOp.61" addrspace(4)* %1, %"class.sycl::_V1::nd_item.1231"* %2, i8 addrspace(4)* %3, %structtype addrspace(4)* %4, i64* %5, %structtype* %6) {
  %8 = alloca [8 x %"struct.c10::complex"], align 16
  %9 = alloca %"struct.at::native::Memory::aligned_vector_loop.66.2511", align 128
  %10 = load i32, i32 addrspace(4)* null, align 4
  %11 = ptrtoint i8 addrspace(4)* %3 to i64
  %12 = trunc i64 %11 to i32
  %13 = and i32 %12, 1
  %14 = icmp eq i32 %13, 0
  br i1 %14, label %18, label %15

15:                                               ; preds = %7
  %16 = getelementptr inbounds %"class.sycl::_V1::nd_item.1231", %"class.sycl::_V1::nd_item.1231"* %2, i64 0, i32 1, i32 0, i32 1, i32 0, i32 0, i64 1
  %17 = load i64, i64* %16, align 8
  br label %18

18:                                               ; preds = %7, %15
  %19 = phi i64 [ %17, %15 ], [ 0, %7 ]
  %20 = phi i32 [ 0, %15 ], [ %10, %7 ]
  %21 = bitcast %"class.sycl::_V1::nd_item.1231"* %2 to i64*
  %22 = getelementptr inbounds i64, i64* %21, i64 8
  %23 = load i64, i64* %22, align 8
  %24 = trunc i64 %23 to i32
  %25 = mul nsw i32 %10, %24
  %26 = getelementptr inbounds [8 x %"struct.c10::complex"], [8 x %"struct.c10::complex"]* %8, i64 0, i64 0
  %27 = getelementptr inbounds [8 x %"struct.c10::complex"], [8 x %"struct.c10::complex"]* %8, i64 1
  br label %28

28:                                               ; preds = %28, %18
  %29 = phi %"struct.c10::complex"* [ %26, %18 ], [ %32, %28 ]
  %30 = getelementptr inbounds %"struct.c10::complex", %"struct.c10::complex"* %29, i64 0, i32 0
  %31 = bitcast double* %30 to i8*
  call void @llvm.memcpy.p0i8.p2i8.i64(i8* %31, i8 addrspace(2)* undef, i64 16, i1 false)
  %32 = getelementptr inbounds %"struct.c10::complex", %"struct.c10::complex"* %29, i64 1
  %33 = bitcast [8 x %"struct.c10::complex"]* %27 to %"struct.c10::complex"*
  %34 = ptrtoint %"struct.c10::complex"* %29 to i64
  %35 = ptrtoint %"struct.c10::complex"* %33 to i64
  %36 = icmp eq i64 %34, %35
  br i1 %36, label %37, label %28

37:                                               ; preds = %28
  %38 = bitcast [8 x %"struct.c10::complex"]* %8 to %structtype*
  %39 = getelementptr inbounds %structtype, %structtype* %38, i64 0, i32 0
  store i64 %19, i64* %39, align 8
  %40 = getelementptr inbounds %structtype, %structtype* %38, i64 0, i32 1
  store i64 0, i64* %40, align 8
  br label %41

41:                                               ; preds = %51, %37
  %42 = phi i32 [ 1, %37 ], [ %57, %51 ]
  %43 = icmp ult i32 %42, 8
  br i1 %43, label %51, label %44

44:                                               ; preds = %41, %44
  %45 = phi i64 [ %49, %44 ], [ 0, %41 ]
  %46 = bitcast %"struct.at::native::Memory::aligned_vector_loop.66.2511"* %9 to %"struct.c10::complex"*
  %47 = getelementptr inbounds %"struct.c10::complex", %"struct.c10::complex"* %46, i64 %45
  %48 = bitcast %"struct.c10::complex"* %47 to i8*
  call void @llvm.memcpy.p0i8.p2i8.i64(i8* %48, i8 addrspace(2)* null, i64 1, i1 false)
  %49 = add nuw nsw i64 %45, 1
  %50 = icmp eq i64 %45, 8
  br i1 %50, label %.preheader, label %44

51:                                               ; preds = %41
  %52 = zext i32 %42 to i64
  %53 = getelementptr inbounds [8 x %"struct.c10::complex"], [8 x %"struct.c10::complex"]* %8, i64 0, i64 %52
  %54 = bitcast %"struct.c10::complex"* %53 to %structtype*
  %55 = getelementptr inbounds %structtype, %structtype* %54, i64 0, i32 0
  store i64 0, i64* %55, align 8
  %56 = getelementptr inbounds %structtype, %structtype* %54, i64 0, i32 1
  store i64 0, i64* %56, align 8
  %57 = add nuw nsw i32 %42, 1
  br label %41, !llvm.loop !4

.preheader:                                       ; preds = %44, %69
  %58 = phi i32 [ %70, %69 ], [ %25, %44 ]
  %59 = icmp ult i32 %58, %20
  br i1 %59, label %60, label %96

60:                                               ; preds = %.preheader
  %61 = zext i32 %58 to i64
  %62 = bitcast i8 addrspace(4)* %3 to %"struct.at::native::Memory::aligned_vector_loop.66.2511" addrspace(4)*
  %63 = getelementptr inbounds %"struct.at::native::Memory::aligned_vector_loop.66.2511", %"struct.at::native::Memory::aligned_vector_loop.66.2511" addrspace(4)* %62, i64 %61
  %64 = bitcast %"struct.at::native::Memory::aligned_vector_loop.66.2511"* %9 to i8*
  %65 = bitcast %"struct.at::native::Memory::aligned_vector_loop.66.2511" addrspace(4)* %63 to i8 addrspace(4)*
  call void @llvm.memcpy.p0i8.p4i8.i64(i8* %64, i8 addrspace(4)* align 128 %65, i64 128, i1 false)
  br label %66

66:                                               ; preds = %71, %60
  %67 = phi i32 [ 0, %60 ], [ %95, %71 ]
  %68 = icmp ult i32 %67, 8
  br i1 %68, label %71, label %69

69:                                               ; preds = %66
  %70 = add i32 %58, %10
  br label %.preheader

71:                                               ; preds = %66
  %72 = zext i32 %67 to i64
  %73 = getelementptr inbounds [8 x %"struct.c10::complex"], [8 x %"struct.c10::complex"]* %8, i64 0, i64 %72
  %74 = bitcast %"struct.c10::complex"* %73 to %structtype*
  %75 = getelementptr inbounds %structtype, %structtype* %74, i64 0, i32 0
  %76 = bitcast i64* %75 to double*
  %77 = load double, double* %76, align 8
  %78 = getelementptr inbounds %structtype, %structtype* %74, i64 0, i32 1
  %79 = bitcast i64* %78 to double*
  %80 = load double, double* %79, align 8
  %81 = getelementptr inbounds %"struct.at::native::Memory::aligned_vector_loop.66.2511", %"struct.at::native::Memory::aligned_vector_loop.66.2511"* %9, i64 0, i32 0, i64 %72
  %82 = bitcast %"struct.c10::complex"* %81 to %structtype*
  %83 = getelementptr inbounds %structtype, %structtype* %82, i64 0, i32 0
  %84 = bitcast i64* %83 to double*
  %85 = load double, double* %84, align 8
  %86 = getelementptr inbounds %structtype, %structtype* %82, i64 0, i32 1
  %87 = bitcast i64* %86 to double*
  %88 = load double, double* %87, align 8
  %89 = fmul fast double %77, %85
  %90 = fmul fast double %80, %88
  %91 = fsub fast double %89, %90
  %92 = fmul fast double %77, %88
  %93 = fmul fast double %80, %85
  %94 = fadd fast double %92, %93
  store double %91, double* %76, align 8
  store double %94, double* %79, align 8
  %95 = add nuw nsw i32 %67, 1
  br label %66, !llvm.loop !5

96:                                               ; preds = %.preheader
  %97 = bitcast i64* %39 to double*
  %98 = load double, double* %97, align 8
  br label %99

99:                                               ; preds = %107, %96
  %100 = phi double [ %98, %96 ], [ %114, %107 ]
  %101 = phi i32 [ 0, %96 ], [ %115, %107 ]
  %102 = icmp ult i32 %101, 8
  br i1 %102, label %107, label %103

103:                                              ; preds = %99
  %104 = bitcast %"struct.c10::complex" addrspace(4)* %0 to %structtype addrspace(4)*
  %105 = getelementptr inbounds %structtype, %structtype addrspace(4)* %104, i64 0, i32 0
  %106 = bitcast i64 addrspace(4)* %105 to double addrspace(4)*
  store double %100, double addrspace(4)* %106, align 8
  ret void

107:                                              ; preds = %99
  %108 = zext i32 %101 to i64
  %109 = getelementptr inbounds [8 x %"struct.c10::complex"], [8 x %"struct.c10::complex"]* %8, i64 0, i64 %108
  %110 = bitcast %"struct.c10::complex"* %109 to %structtype*
  %111 = getelementptr inbounds %structtype, %structtype* %110, i64 0, i32 0
  %112 = bitcast i64* %111 to double*
  %113 = load double, double* %112, align 8
  %114 = fmul fast double %100, %113
  %115 = add nuw nsw i32 %101, 1
  br label %99, !llvm.loop !6
}

define linkonce_odr spir_func void @_ZNK2at15AtenIpexTypeXPU8ReduceOpIN3c107complexIdEENS0_14func_wrapper_tIS4_NS0_13ReduceProdOpsIS4_EEEEjS4_Li4EE33input_vectorized_item_reduce_implILi4EEES4_N4sycl3_V17nd_itemILi2EEEPKS4_(%"struct.at::AtenIpexTypeXPU::ReduceOp.61" addrspace(4)* %0, %"class.sycl::_V1::nd_item.1231"* %1, [4 x %"struct.c10::complex"]* %2, %"struct.c10::complex"* %3) {
  br label %5

5:                                                ; preds = %5, %4
  br label %5
}

define spir_func void @_wassert(i32 %0) {
  %2 = call spir_func i64 @_Z28__spirv_GlobalInvocationId_zv()
  %3 = call spir_func i64 @_Z27__spirv_LocalInvocationId_xv()
  %4 = call spir_func i64 @_Z27__spirv_LocalInvocationId_yv()
  call spir_func void @__devicelib_assert_fail(i8 addrspace(4)* null, i8 addrspace(4)* null, i32 0, i8 addrspace(4)* null, i64 0, i64 0, i64 %2, i64 1, i64 1, i64 0)
  ret void
}

define spir_func i64 @_Z28__spirv_GlobalInvocationId_xv() {
  ret i64 0
}

define spir_func i64 @_Z28__spirv_GlobalInvocationId_yv() {
  ret i64 0
}

define spir_func i64 @_Z28__spirv_GlobalInvocationId_zv() {
  %1 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 2)
  ret i64 %1
}

define spir_func i64 @_Z27__spirv_LocalInvocationId_xv() {
  %1 = call spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 0)
  ret i64 1
}

define spir_func i64 @_Z27__spirv_LocalInvocationId_yv() {
  %1 = call spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 1)
  ret i64 1
}

define spir_func i64 @_Z27__spirv_LocalInvocationId_zv() {
  ret i64 0
}

declare spir_func void @__devicelib_assert_fail(i8 addrspace(4)*, i8 addrspace(4)*, i32, i8 addrspace(4)*, i64, i64, i64, i64, i64, i64)

declare spir_func i64 @_Z32__spirv_SubgroupShuffleDownINTELllj(i64, i64, i32)

declare spir_func void @_Z22__spirv_ControlBarrieriii(i32, i32, i32)

; Function Attrs: argmemonly nofree nounwind willreturn
declare void @llvm.memcpy.p4i8.p0i8.i64(i8 addrspace(4)* noalias nocapture writeonly, i8* noalias nocapture readonly, i64, i1 immarg) #1

declare spir_func i8 addrspace(1)* @_Z41__spirv_GenericCastToPtrExplicit_ToGlobalPU3AS4ci()

define spir_kernel void @_ZTSN2at15AtenIpexTypeXPU13reduce_kernelILi1ENS0_8ReduceOpIN3c107complexIdEENS0_14func_wrapper_tIS5_NS0_13ReduceProdOpsIS5_EEEEjS5_Li4EEEEE(%"struct.at::AtenIpexTypeXPU::__generated_ReduceOp.288"* byval(%"struct.at::AtenIpexTypeXPU::__generated_ReduceOp.288") %0, i8 addrspace(3)* %1, i8 addrspace(3)* %2, %"class.at::AtenIpexTypeXPU::reduce_kernel.289"* %3, i8 addrspace(4)* %4, double %5, %"struct.at::native::Memory::aligned_vector_loop" addrspace(4)* %6, i64 %7, %structtype addrspace(4)* %8, double %9, i64* %10, i64* %11, i64* %12, i8 addrspace(4)* %13, i8 addrspace(4)* %14, %structtype addrspace(4)* %15, i64 addrspace(4)* %16, %structtype addrspace(4)* %17) {
  %19 = alloca i64, align 8
  %20 = alloca i64, i32 0, align 8
  %21 = alloca %"struct.c10::complex", align 16
  %22 = alloca %"struct.c10::complex", align 16
  %23 = alloca i64, i32 0, align 8
  %24 = alloca i64, i32 0, align 8
  %25 = alloca %"struct.c10::complex", align 16
  %26 = alloca %"struct.c10::complex", align 16
  %27 = alloca %"struct.c10::complex", i32 0, align 16
  %28 = alloca %"struct.xpu::dpcpp::Array.5", align 16
  %29 = alloca %"struct.at::native::Memory::aligned_vector_loop", align 16
  %30 = alloca %"struct.at::native::Memory::aligned_vector_loop", align 16
  %31 = alloca %"struct.xpu::dpcpp::Array.5", i32 0, align 16
  %32 = alloca %"class.sycl::_V1::nd_item.1231", align 8
  %33 = alloca %"class.at::AtenIpexTypeXPU::reduce_kernel.289", align 16
  %34 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::reduce_kernel.289", %"class.at::AtenIpexTypeXPU::reduce_kernel.289"* %33, i64 0, i32 0
  %35 = bitcast %"struct.at::AtenIpexTypeXPU::ReduceOp.61"* %34 to %"struct.at::AtenIpexTypeXPU::__generated_ReduceOp.288"*
  %36 = bitcast %"struct.at::AtenIpexTypeXPU::__generated_ReduceOp.288"* %35 to i8*
  %37 = bitcast %"struct.at::AtenIpexTypeXPU::__generated_ReduceOp.288"* %0 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %36, i8* %37, i64 608, i1 false)
  %38 = call spir_func i64 @_Z28__spirv_BuiltInWorkgroupSizei()
  %39 = call spir_func i64 @_Z28__spirv_BuiltInWorkgroupSizei()
  %40 = call spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32 0)
  %41 = call spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 1)
  %42 = call spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 0)
  store i64 0, i64* %10, align 1
  store i64 0, i64* %10, align 1
  %43 = bitcast %"class.sycl::_V1::nd_item.1231"* %32 to i64*
  %44 = getelementptr inbounds i64, i64* %43, i64 8
  store i64 %41, i64* %44, align 1
  %45 = getelementptr inbounds i64, i64* %43, i64 9
  store i64 %42, i64* %45, align 1
  store i64 0, i64* %10, align 1
  %46 = trunc i64 %42 to i32
  %47 = trunc i64 %41 to i32
  %48 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::reduce_kernel.289", %"class.at::AtenIpexTypeXPU::reduce_kernel.289"* %3, i64 0, i32 0, i32 3, i32 7
  %49 = bitcast [2 x i32]* %48 to i32*
  %50 = load i32, i32* %49, align 4
  %51 = mul nsw i32 %50, %46
  %52 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::reduce_kernel.289", %"class.at::AtenIpexTypeXPU::reduce_kernel.289"* %3, i64 0, i32 0, i32 3, i32 7, i64 1
  %53 = load i32, i32* %52, align 4
  %54 = mul nsw i32 %53, %47
  %55 = add nsw i32 %51, %54
  %56 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::reduce_kernel.289", %"class.at::AtenIpexTypeXPU::reduce_kernel.289"* %33, i64 0, i32 0, i32 3, i32 6
  %57 = bitcast [3 x i32]* %56 to i32*
  %58 = load i32, i32* %57, align 4
  %59 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::reduce_kernel.289", %"class.at::AtenIpexTypeXPU::reduce_kernel.289"* %3, i64 0, i32 0, i32 3, i32 6, i64 1
  br label %60

60:                                               ; preds = %67, %18
  %61 = phi i32 [ 0, %18 ], [ %70, %67 ]
  %62 = icmp ult i32 %61, 2
  br i1 %62, label %67, label %63

63:                                               ; preds = %60
  %64 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::reduce_kernel.289", %"class.at::AtenIpexTypeXPU::reduce_kernel.289"* %33, i64 0, i32 0, i32 5
  %65 = bitcast %struct.OffsetCalculator* %64 to i32*
  %66 = load i32, i32* %65, align 4
  br label %71

67:                                               ; preds = %60
  %68 = zext i32 %61 to i64
  %69 = getelementptr inbounds %"struct.xpu::dpcpp::Array.5", %"struct.xpu::dpcpp::Array.5"* %28, i64 0, i32 0, i64 %68
  store i32 0, i32* %69, align 4
  %70 = add nuw nsw i32 %61, 1
  br label %60

71:                                               ; preds = %75, %63
  %72 = phi i32 [ %55, %63 ], [ 0, %75 ]
  %73 = phi i32 [ 0, %63 ], [ 1, %75 ]
  %74 = icmp eq i32 %73, %66
  br i1 %74, label %83, label %75

75:                                               ; preds = %71, %78
  %76 = phi i32 [ %82, %78 ], [ 0, %71 ]
  %77 = icmp ult i32 %76, 1
  br i1 %77, label %78, label %71, !llvm.loop !7

78:                                               ; preds = %75
  %79 = getelementptr inbounds %"struct.xpu::dpcpp::Array.5", %"struct.xpu::dpcpp::Array.5"* %28, i64 0, i32 0, i64 1
  %80 = load i32, i32* %79, align 4
  %81 = add i32 %80, %72
  store i32 %81, i32* %79, align 4
  %82 = add nuw nsw i32 %76, 1
  br label %75, !llvm.loop !8

83:                                               ; preds = %71
  %84 = getelementptr inbounds %"struct.xpu::dpcpp::Array.5", %"struct.xpu::dpcpp::Array.5"* %28, i64 0, i32 0, i64 1
  %85 = load i32, i32* %84, align 4
  br label %86

86:                                               ; preds = %86, %83
  %87 = phi %"struct.at::native::Memory::aligned_vector_loop"* [ %29, %83 ], [ null, %86 ]
  %88 = ptrtoint %"struct.at::native::Memory::aligned_vector_loop"* %87 to i64
  %89 = icmp eq i64 %88, 0
  br i1 %89, label %90, label %86

90:                                               ; preds = %86
  %91 = bitcast %"struct.at::native::Memory::aligned_vector_loop"* %29 to i64*
  store i64 0, i64* %91, align 8
  %92 = getelementptr inbounds %"struct.at::native::Memory::aligned_vector_loop", %"struct.at::native::Memory::aligned_vector_loop"* %29, i64 0, i32 0, i64 0, i32 1
  %93 = bitcast double* %92 to i64*
  store i64 0, i64* %93, align 8
  %94 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::reduce_kernel.289", %"class.at::AtenIpexTypeXPU::reduce_kernel.289"* %3, i64 0, i32 0, i32 3, i32 1
  %95 = load i32, i32* %94, align 4
  %96 = icmp ult i32 0, %95
  br i1 %96, label %97, label %140

97:                                               ; preds = %90
  %98 = zext i32 %85 to i64
  %99 = getelementptr inbounds i8, i8 addrspace(4)* %4, i64 %98
  %100 = addrspacecast %"struct.c10::complex"* %27 to %"struct.c10::complex" addrspace(4)*
  %101 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::reduce_kernel.289", %"class.at::AtenIpexTypeXPU::reduce_kernel.289"* %3, i64 0, i32 0, i32 3, i32 11
  %102 = load i8, i8* %101, align 8
  %103 = icmp eq i8 %102, 0
  br i1 %103, label %126, label %104

104:                                              ; preds = %97
  %105 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::reduce_kernel.289", %"class.at::AtenIpexTypeXPU::reduce_kernel.289"* %3, i64 0, i32 0, i32 3, i32 12
  %106 = load i32, i32* %105, align 4
  switch i32 %106, label %116 [
    i32 7, label %107
    i32 1, label %110
    i32 0, label %115
  ]

107:                                              ; preds = %104
  call spir_func void @_ZNK2at15AtenIpexTypeXPU8ReduceOpIN3c107complexIdEENS0_14func_wrapper_tIS4_NS0_13ReduceProdOpsIS4_EEEEjS4_Li4EE33input_vectorized_item_reduce_implILi16EEES4_N4sycl3_V17nd_itemILi2EEEPKS4_(%"struct.c10::complex" addrspace(4)* null, %"struct.at::AtenIpexTypeXPU::ReduceOp.61" addrspace(4)* null, %"class.sycl::_V1::nd_item.1231"* %32, i8 addrspace(4)* %99, %"struct.c10::complex" addrspace(4)* undef, i64 undef, i64* undef, %"struct.c10::complex" addrspace(4)* undef, %structtype addrspace(4)* undef, %structtype addrspace(4)* undef, %structtype addrspace(4)* undef)
  %108 = bitcast %"struct.at::native::Memory::aligned_vector_loop"* %30 to %structtype*
  %109 = getelementptr inbounds %structtype, %structtype* %108, i64 0, i32 0
  store i64 0, i64* %109, align 8
  br label %137

110:                                              ; preds = %104
  call spir_func void @_ZNK2at15AtenIpexTypeXPU8ReduceOpIN3c107complexIdEENS0_14func_wrapper_tIS4_NS0_13ReduceProdOpsIS4_EEEEjS4_Li4EE33input_vectorized_item_reduce_implILi8EEES4_N4sycl3_V17nd_itemILi2EEEPKS4_(%"struct.c10::complex" addrspace(4)* %100, %"struct.at::AtenIpexTypeXPU::ReduceOp.61" addrspace(4)* null, %"class.sycl::_V1::nd_item.1231"* %32, i8 addrspace(4)* %99, %structtype addrspace(4)* undef, i64* undef, %structtype* undef)
  %111 = bitcast %"struct.c10::complex"* %27 to i64*
  %112 = load i64, i64* %111, align 1
  %113 = bitcast %"struct.at::native::Memory::aligned_vector_loop"* %30 to %structtype*
  %114 = getelementptr inbounds %structtype, %structtype* %113, i64 0, i32 0
  store i64 %112, i64* %114, align 8
  br label %137

common.ret:                                       ; preds = %126, %304, %115
  ret void

115:                                              ; preds = %104
  call spir_func void @_ZNK2at15AtenIpexTypeXPU8ReduceOpIN3c107complexIdEENS0_14func_wrapper_tIS4_NS0_13ReduceProdOpsIS4_EEEEjS4_Li4EE33input_vectorized_item_reduce_implILi4EEES4_N4sycl3_V17nd_itemILi2EEEPKS4_(%"struct.at::AtenIpexTypeXPU::ReduceOp.61" addrspace(4)* null, %"class.sycl::_V1::nd_item.1231"* null, [4 x %"struct.c10::complex"]* undef, %"struct.c10::complex"* undef)
  br label %common.ret

116:                                              ; preds = %104
  %117 = bitcast i8 addrspace(4)* %99 to %"struct.c10::complex" addrspace(4)*
  %118 = bitcast %"struct.c10::complex" addrspace(4)* %117 to %structtype addrspace(4)*
  %119 = getelementptr inbounds %structtype, %structtype addrspace(4)* %118, i64 0, i32 0
  %120 = bitcast i64 addrspace(4)* %119 to double addrspace(4)*
  %121 = load double, double addrspace(4)* %120, align 8
  %122 = fmul fast double %5, %121
  %123 = bitcast double %122 to i64
  %124 = bitcast %"struct.at::native::Memory::aligned_vector_loop"* %30 to %structtype*
  %125 = getelementptr inbounds %structtype, %structtype* %124, i64 0, i32 1
  store i64 %123, i64* %125, align 8
  br label %137

126:                                              ; preds = %97
  %127 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::reduce_kernel.289", %"class.at::AtenIpexTypeXPU::reduce_kernel.289"* %33, i64 0, i32 0, i32 4, i32 2
  %128 = bitcast [12 x [1 x i32]]* %127 to i32*
  %129 = load i32, i32* %128, align 4
  %130 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::reduce_kernel.289", %"class.at::AtenIpexTypeXPU::reduce_kernel.289"* %33, i64 0, i32 0, i32 4
  %131 = bitcast %struct.OffsetCalculator.505* %130 to i32*
  %132 = load i32, i32* %131, align 4
  %133 = icmp eq i32 %132, 0
  %134 = icmp eq i32 %129, 0
  %135 = select i1 %133, i1 %134, i1 false
  br i1 %135, label %136, label %common.ret

136:                                              ; preds = %126, %136
  br label %136

137:                                              ; preds = %116, %110, %107
  %138 = bitcast %"struct.at::native::Memory::aligned_vector_loop"* %29 to i8*
  %139 = bitcast %"struct.at::native::Memory::aligned_vector_loop"* %30 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %138, i8* %139, i64 16, i1 false)
  br label %140

140:                                              ; preds = %137, %90
  %141 = icmp eq i32 %58, 0
  br i1 %141, label %211, label %142

142:                                              ; preds = %140
  %143 = call spir_func i32 @_Z27__spirv_BuiltInSubgroupSizev()
  %144 = call spir_func i32 @_Z40__spirv_BuiltInSubgroupLocalInvocationIdv()
  %145 = call spir_func i32 @_Z27__spirv_BuiltInNumSubgroupsv()
  call spir_func void @_wassert(i32 0)
  %146 = bitcast %"struct.c10::complex"* %25 to double*
  %147 = bitcast %"struct.c10::complex"* %26 to %structtype*
  %148 = getelementptr inbounds %structtype, %structtype* %147, i64 0, i32 0
  br label %149

149:                                              ; preds = %184, %142
  %150 = phi double [ 0.000000e+00, %142 ], [ %187, %184 ]
  %151 = phi double [ 0.000000e+00, %142 ], [ %189, %184 ]
  %152 = phi i32 [ 1, %142 ], [ 0, %184 ]
  %153 = icmp slt i32 %152, %143
  br i1 %153, label %156, label %154

154:                                              ; preds = %149
  %155 = icmp eq i32 %144, 0
  br i1 %155, label %190, label %191

156:                                              ; preds = %149
  store double %151, double* %146, align 1
  br label %157

157:                                              ; preds = %178, %156
  %158 = phi i64 [ 0, %156 ], [ %161, %178 ]
  %159 = icmp ult i64 %158, 9
  br i1 %159, label %160, label %184

160:                                              ; preds = %157
  %161 = add nuw nsw i64 %158, 8
  %162 = bitcast %"struct.c10::complex"* %25 to i8*
  %163 = getelementptr inbounds i8, i8* %162, i64 %158
  br label %164

164:                                              ; preds = %167, %160
  %165 = phi i64 [ 0, %160 ], [ %171, %167 ]
  %166 = icmp ult i64 %165, 8
  br i1 %166, label %167, label %172

167:                                              ; preds = %164
  %168 = getelementptr inbounds i8, i8* %163, i64 %165
  %169 = load i8, i8* %168, align 1
  %170 = bitcast i64* %23 to i8*
  store i8 %169, i8* %170, align 1
  %171 = add nuw nsw i64 %7, 1
  br label %164

172:                                              ; preds = %164
  %173 = load i64, i64* %23, align 8
  %174 = call spir_func i64 @_Z32__spirv_SubgroupShuffleDownINTELllj(i64 %173, i64 0, i32 0)
  store i64 %174, i64* %24, align 8
  %175 = bitcast %"struct.c10::complex"* %26 to i8*
  %176 = getelementptr inbounds i8, i8* %175, i64 %158
  %177 = trunc i64 %174 to i8
  br label %178

178:                                              ; preds = %181, %172
  %179 = phi i64 [ 0, %172 ], [ %7, %181 ]
  %180 = icmp ult i64 %179, 8
  br i1 %180, label %181, label %157, !llvm.loop !9

181:                                              ; preds = %178
  %182 = bitcast i64* %24 to i8*
  %183 = getelementptr inbounds i8, i8* %176, i64 %179
  store i8 %177, i8* %183, align 1
  br label %178

184:                                              ; preds = %157
  %185 = bitcast i64* %148 to double*
  %186 = load double, double* %185, align 8
  %187 = fsub fast double 0.000000e+00, %151
  %188 = fmul fast double %186, %151
  %189 = fadd fast double %150, %188
  br label %149

190:                                              ; preds = %154
  store double 0.000000e+00, double addrspace(3)* null, align 16
  br label %191

191:                                              ; preds = %190, %154
  br label %192

192:                                              ; preds = %206, %191
  %193 = phi double [ %150, %191 ], [ %207, %206 ]
  %194 = phi double [ 0.000000e+00, %191 ], [ %208, %206 ]
  %195 = phi i32 [ %145, %191 ], [ %209, %206 ]
  %196 = icmp sgt i32 %195, 0
  br i1 %196, label %197, label %.loopexit5

197:                                              ; preds = %192
  %198 = icmp sgt i32 %195, %46
  br i1 %198, label %199, label %206

199:                                              ; preds = %197
  %200 = add nsw i32 %195, %46
  %201 = sext i32 %200 to i64
  %202 = getelementptr inbounds %"struct.at::native::Memory::aligned_vector_loop", %"struct.at::native::Memory::aligned_vector_loop" addrspace(3)* null, i64 %201
  %203 = bitcast %"struct.at::native::Memory::aligned_vector_loop" addrspace(3)* %202 to double addrspace(3)*
  %204 = load double, double addrspace(3)* %203, align 16
  %205 = fmul fast double %194, %204
  br label %206

206:                                              ; preds = %199, %197
  %207 = phi double [ %205, %199 ], [ 0.000000e+00, %197 ]
  %208 = phi double [ %205, %199 ], [ %194, %197 ]
  %209 = lshr i32 %195, 1
  br label %192

.loopexit5:                                       ; preds = %192
  %210 = bitcast %"struct.at::native::Memory::aligned_vector_loop"* %29 to double*
  store double %193, double* %210, align 16
  store double %151, double* %92, align 8
  br label %274

211:                                              ; preds = %140
  %212 = load i32, i32* %59, align 4
  %213 = icmp eq i32 %212, 0
  br i1 %213, label %274, label %214

214:                                              ; preds = %211
  %215 = load double, double* %92, align 8
  %216 = trunc i64 %7 to i32
  %217 = add nuw nsw i32 %47, %46
  %218 = sext i32 %217 to i64
  %219 = getelementptr inbounds %"struct.at::native::Memory::aligned_vector_loop", %"struct.at::native::Memory::aligned_vector_loop" addrspace(3)* null, i64 %218
  store double 0.000000e+00, double addrspace(3)* null, align 16
  %220 = bitcast %"struct.at::native::Memory::aligned_vector_loop" addrspace(3)* %219 to double addrspace(3)*
  store double %215, double addrspace(3)* %220, align 8
  br label %221

221:                                              ; preds = %236, %214
  %222 = phi double [ 0.000000e+00, %214 ], [ %237, %236 ]
  %223 = phi double [ %215, %214 ], [ %238, %236 ]
  %224 = phi i32 [ %216, %214 ], [ %225, %236 ]
  %225 = lshr i32 %224, 1
  %226 = icmp ult i32 %224, 1
  br i1 %226, label %239, label %227

227:                                              ; preds = %221
  %228 = icmp ugt i32 %224, %47
  %229 = add nuw nsw i32 %224, %47
  br i1 %228, label %230, label %236

230:                                              ; preds = %227
  %231 = add nuw nsw i32 %229, %46
  %232 = sext i32 %231 to i64
  %233 = getelementptr inbounds %"struct.at::native::Memory::aligned_vector_loop", %"struct.at::native::Memory::aligned_vector_loop" addrspace(3)* null, i64 %232
  %234 = bitcast %"struct.at::native::Memory::aligned_vector_loop" addrspace(3)* %233 to double addrspace(3)*
  %235 = load double, double addrspace(3)* %234, align 16
  br label %236

236:                                              ; preds = %230, %227
  %237 = phi double [ %223, %230 ], [ 0.000000e+00, %227 ]
  %238 = phi double [ %235, %230 ], [ 0.000000e+00, %227 ]
  br label %221

239:                                              ; preds = %221
  %240 = bitcast %"struct.c10::complex"* %21 to double*
  %241 = bitcast %"struct.c10::complex"* %22 to %structtype*
  %242 = getelementptr inbounds %structtype, %structtype* %241, i64 0, i32 0
  br label %243

243:                                              ; preds = %270, %239
  %244 = phi double [ %222, %239 ], [ %273, %270 ]
  store double %244, double* %240, align 1
  br label %245

245:                                              ; preds = %265, %243
  %246 = phi i64 [ 0, %243 ], [ %249, %265 ]
  %247 = icmp ult i64 %246, 9
  br i1 %247, label %248, label %270

248:                                              ; preds = %245
  %249 = add nuw nsw i64 %246, 1
  call void @llvm.lifetime.start.p0i8(i64 0, i8* null)
  %250 = bitcast %"struct.c10::complex"* %21 to i8*
  %251 = getelementptr inbounds i8, i8* %250, i64 %246
  br label %252

252:                                              ; preds = %255, %248
  %253 = phi i64 [ 0, %248 ], [ %7, %255 ]
  %254 = icmp ult i64 %253, 8
  br i1 %254, label %255, label %259

255:                                              ; preds = %252
  %256 = load i8, i8* %251, align 1
  %257 = bitcast i64* %19 to i8*
  %258 = getelementptr inbounds i8, i8* %257, i64 %253
  store i8 %256, i8* %258, align 1
  br label %252

259:                                              ; preds = %252
  %260 = load i64, i64* %19, align 8
  %261 = call spir_func i64 @_Z32__spirv_SubgroupShuffleDownINTELllj(i64 %260, i64 0, i32 0)
  store i64 %261, i64* %20, align 8
  %262 = bitcast %"struct.c10::complex"* %22 to i8*
  %263 = getelementptr inbounds i8, i8* %262, i64 %246
  %264 = trunc i64 %261 to i8
  br label %265

265:                                              ; preds = %268, %259
  %266 = phi i64 [ 0, %259 ], [ 1, %268 ]
  %267 = icmp ult i64 %266, 1
  br i1 %267, label %268, label %245

268:                                              ; preds = %265
  %269 = bitcast i64* %20 to i8*
  store i8 %264, i8* %263, align 1
  br label %265

270:                                              ; preds = %245
  %271 = bitcast i64* %242 to double*
  %272 = load double, double* %271, align 8
  %273 = fmul fast double %272, %244
  br label %243

274:                                              ; preds = %211, %.loopexit5
  br label %275

275:                                              ; preds = %293, %274
  %276 = phi i32 [ %294, %293 ], [ 0, %274 ]
  %277 = icmp ugt i32 %276, 11
  %278 = icmp eq i32 0, %66
  %279 = select i1 %277, i1 true, i1 %278
  br i1 %279, label %304, label %280

280:                                              ; preds = %275
  %281 = zext i32 %276 to i64
  %282 = call spir_func i32 @_Z20__spirv_ocl_u_mul_hijj(i32 0, i32 0)
  %283 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::reduce_kernel.289", %"class.at::AtenIpexTypeXPU::reduce_kernel.289"* %33, i64 0, i32 0, i32 5, i32 1, i64 %281, i32 2
  %284 = load i32, i32* %283, align 4
  %285 = lshr i32 %51, %284
  %286 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::reduce_kernel.289", %"class.at::AtenIpexTypeXPU::reduce_kernel.289"* %33, i64 0, i32 0, i32 5, i32 1, i64 %281
  %287 = bitcast %struct.IntDivider* %286 to i32*
  %288 = load i32, i32* %287, align 4
  %289 = mul i32 %285, %288
  br label %290

290:                                              ; preds = %295, %280
  %291 = phi i32 [ 0, %280 ], [ %303, %295 ]
  %292 = icmp ult i32 %291, 2
  br i1 %292, label %295, label %293

293:                                              ; preds = %290
  %294 = add nuw nsw i32 %276, 1
  br label %275, !llvm.loop !10

295:                                              ; preds = %290
  %296 = zext i32 %291 to i64
  %297 = getelementptr inbounds %"class.at::AtenIpexTypeXPU::reduce_kernel.289", %"class.at::AtenIpexTypeXPU::reduce_kernel.289"* %33, i64 0, i32 0, i32 5, i32 2, i64 %281, i64 %296
  %298 = load i32, i32* %297, align 4
  %299 = mul i32 %289, %298
  %300 = getelementptr inbounds %"struct.xpu::dpcpp::Array.5", %"struct.xpu::dpcpp::Array.5"* %31, i64 0, i32 0, i64 %296
  %301 = load i32, i32* %300, align 4
  %302 = add i32 %301, %299
  store i32 %302, i32* %300, align 4
  %303 = add nuw nsw i32 %291, 1
  br label %290, !llvm.loop !11

304:                                              ; preds = %275
  %305 = load i64, i64* %91, align 16
  %306 = load i64, i64* %93, align 8
  store i64 %305, i64 addrspace(4)* %16, align 8
  store i64 %306, i64 addrspace(4)* null, align 8
  br label %common.ret
}

declare spir_func i32 @_Z27__spirv_BuiltInNumSubgroupsv()

declare spir_func i32 @_Z40__spirv_BuiltInSubgroupLocalInvocationIdv()

declare spir_func i32 @_Z27__spirv_BuiltInSubgroupSizev()

declare spir_func i64 @_Z28__spirv_BuiltInNumWorkgroupsi(i32)

declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

declare spir_func i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32)

declare spir_func i64 @_Z26__spirv_BuiltInWorkgroupIdi(i32)

declare spir_func i64 @_Z28__spirv_BuiltInWorkgroupSizei()

attributes #0 = { argmemonly nofree nosync nounwind willreturn }
attributes #1 = { argmemonly nofree nounwind willreturn }
attributes #2 = { inaccessiblememonly nofree nosync nounwind willreturn }
attributes #3 = { nofree nosync nounwind readnone speculatable willreturn }

!0 = distinct !{!0, !1}
!1 = !{!"llvm.loop.unroll.enable"}
!2 = distinct !{!2, !1}
!3 = distinct !{!3, !1}
!4 = distinct !{!4, !1}
!5 = distinct !{!5, !1}
!6 = distinct !{!6, !1}
!7 = distinct !{!7, !1}
!8 = distinct !{!8, !1}
!9 = distinct !{!9, !1}
!10 = distinct !{!10, !1}
!11 = distinct !{!11, !1}
