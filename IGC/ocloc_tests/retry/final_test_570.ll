; REQUIRES: regkeys, pvc-supported, llvm-14-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options "-igc_opts 'EnableOpaquePointersBackend=1,VISAOptions=-asmToConsole'" &> %t_output.ll
; RUN: FileCheck --input-file %t_output.ll %s

; LLVM with typed pointers:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options "-igc_opts 'VISAOptions=-asmToConsole'" &> %t_output.ll
; RUN: FileCheck --input-file %t_output.ll %s

; ATTENTION: if your change lowers spill size significantly congratulations! just adjust the numbers

; normal version
; CHECK://.kernel __omp_offloading_10301_1618d0__Z7x_solve_l708
; I'm trying to match 5 consecutive numbers starting with 2: 23477 for example, not 234770 and not 2347,
; lower boundary is set by {4} and upper boundary by matching EOL character {{$}}
; CHECK://.spill size 3[[A:[0-9]{3}]]{{$}}
; CHECK://.BankConflicts:

; retry version
; CHECK://.kernel __omp_offloading_10301_1618d0__Z7x_solve_l708
; CHECK-NOT://.spill size
; this test requires remat to be as performant as retry version IGC_RematEnable=1


@rhs = external addrspace(1) global [102 x [103 x [103 x [5 x double]]]]

declare spir_func i64 @_Z13get_global_idj(i32)

; __spirv_ocl_madddd takes three double arguments: (a * b) + c
declare spir_func double @_Z15__spirv_ocl_madddd(double, double, double)

define spir_kernel void @__omp_offloading_10301_1618d0__Z7x_solve_l708([5 x [5 x [3 x [102 x [101 x [101 x double]]]]]] addrspace(1)* %0, i64 %1) {
  %3 = call spir_func i64 @_Z13get_global_idj(i32 0)
  %4 = trunc i64 %3 to i32
  %5 = sext i32 %4 to i64
  br label %6

6:                                                ; preds = %6, %2
  %7 = getelementptr inbounds [102 x [103 x [103 x [5 x double]]]], [102 x [103 x [103 x [5 x double]]]] addrspace(1)* @rhs, i64 0, i64 undef, i64 %5, i64 undef, i64 2
  %8 = load double, double addrspace(1)* %7, align 8
  %9 = call spir_func double @_Z15__spirv_ocl_madddd(double %8, double %8, double %8)
  %10 = call spir_func double @_Z15__spirv_ocl_madddd(double %8, double %8, double %8)
  %11 = call spir_func double @_Z15__spirv_ocl_madddd(double %8, double %8, double %8)
  %12 = call spir_func double @_Z15__spirv_ocl_madddd(double %8, double %8, double %8)
  %13 = call spir_func double @_Z15__spirv_ocl_madddd(double %8, double %8, double %8)
  %14 = getelementptr inbounds [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]], [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]] addrspace(1)* %0, i64 0, i64 0, i64 0, i64 1, i64 undef, i64 %5, i64 undef
  %15 = load double, double addrspace(1)* %14, align 8
  %16 = call spir_func double @_Z15__spirv_ocl_madddd(double %15, double %15, double %15)
  %17 = call spir_func double @_Z15__spirv_ocl_madddd(double %15, double %15, double %15)
  %18 = call spir_func double @_Z15__spirv_ocl_madddd(double %15, double %15, double %15)
  %19 = call spir_func double @_Z15__spirv_ocl_madddd(double %15, double %15, double %15)
  %20 = call spir_func double @_Z15__spirv_ocl_madddd(double %15, double %15, double %15)
  store double 0.000000e+00, double addrspace(1)* %14, align 8
  %21 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %22 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %23 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %24 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %25 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %26 = getelementptr inbounds [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]], [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]] addrspace(1)* %0, i64 0, i64 1, i64 1, i64 1, i64 undef, i64 %5, i64 undef
  %27 = load double, double addrspace(1)* %26, align 8
  %28 = call spir_func double @_Z15__spirv_ocl_madddd(double %27, double %27, double %27)
  %29 = call spir_func double @_Z15__spirv_ocl_madddd(double %28, double %28, double %28)
  %30 = call spir_func double @_Z15__spirv_ocl_madddd(double %29, double %29, double %29)
  %31 = call spir_func double @_Z15__spirv_ocl_madddd(double %27, double %27, double %27)
  %32 = call spir_func double @_Z15__spirv_ocl_madddd(double %30, double %30, double %30)
  %33 = getelementptr inbounds [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]], [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]] addrspace(1)* %0, i64 0, i64 0, i64 2, i64 1, i64 undef, i64 %5, i64 undef
  %34 = load double, double addrspace(1)* %33, align 8
  %35 = call spir_func double @_Z15__spirv_ocl_madddd(double %34, double %34, double %34)
  %36 = call spir_func double @_Z15__spirv_ocl_madddd(double %34, double %34, double %34)
  %37 = call spir_func double @_Z15__spirv_ocl_madddd(double %35, double %35, double %35)
  %38 = call spir_func double @_Z15__spirv_ocl_madddd(double %37, double %37, double %37)
  %39 = call spir_func double @_Z15__spirv_ocl_madddd(double %38, double %38, double %38)
  %40 = getelementptr inbounds [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]], [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]] addrspace(1)* %0, i64 0, i64 1, i64 2, i64 1, i64 undef, i64 %5, i64 undef
  %41 = load double, double addrspace(1)* %40, align 8
  %42 = call spir_func double @_Z15__spirv_ocl_madddd(double %41, double %41, double %41)
  %43 = call spir_func double @_Z15__spirv_ocl_madddd(double %42, double %42, double %42)
  %44 = call spir_func double @_Z15__spirv_ocl_madddd(double %43, double %43, double %43)
  %45 = call spir_func double @_Z15__spirv_ocl_madddd(double %44, double %44, double %44)
  %46 = call spir_func double @_Z15__spirv_ocl_madddd(double %41, double %41, double %41)
  %47 = getelementptr inbounds [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]], [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]] addrspace(1)* %0, i64 0, i64 2, i64 2, i64 1, i64 undef, i64 %5, i64 undef
  %48 = load double, double addrspace(1)* %47, align 8
  %49 = call spir_func double @_Z15__spirv_ocl_madddd(double %48, double %48, double %48)
  %50 = call spir_func double @_Z15__spirv_ocl_madddd(double %49, double %49, double %49)
  %51 = call spir_func double @_Z15__spirv_ocl_madddd(double %50, double %50, double %50)
  %52 = call spir_func double @_Z15__spirv_ocl_madddd(double %51, double %51, double %51)
  %53 = call spir_func double @_Z15__spirv_ocl_madddd(double %52, double %52, double %52)
  %54 = getelementptr inbounds [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]], [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]] addrspace(1)* %0, i64 0, i64 3, i64 2, i64 1, i64 undef, i64 %5, i64 undef
  %55 = load double, double addrspace(1)* %54, align 8
  %56 = call spir_func double @_Z15__spirv_ocl_madddd(double %55, double %55, double %55)
  %57 = call spir_func double @_Z15__spirv_ocl_madddd(double %56, double %56, double %56)
  %58 = call spir_func double @_Z15__spirv_ocl_madddd(double %57, double %57, double %57)
  %59 = call spir_func double @_Z15__spirv_ocl_madddd(double %58, double %58, double %58)
  %60 = call spir_func double @_Z15__spirv_ocl_madddd(double %59, double %59, double %59)
  %61 = getelementptr inbounds [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]], [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]] addrspace(1)* %0, i64 0, i64 4, i64 2, i64 1, i64 undef, i64 %5, i64 undef
  %62 = load double, double addrspace(1)* %61, align 8
  %63 = call spir_func double @_Z15__spirv_ocl_madddd(double %62, double %62, double %62)
  %64 = call spir_func double @_Z15__spirv_ocl_madddd(double %63, double %63, double %63)
  %65 = call spir_func double @_Z15__spirv_ocl_madddd(double %64, double %64, double %64)
  %66 = call spir_func double @_Z15__spirv_ocl_madddd(double %65, double %65, double %65)
  %67 = call spir_func double @_Z15__spirv_ocl_madddd(double %66, double %66, double %66)
  %68 = getelementptr inbounds [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]], [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]] addrspace(1)* %0, i64 0, i64 0, i64 3, i64 1, i64 undef, i64 %5, i64 undef
  %69 = load double, double addrspace(1)* %68, align 8
  %70 = call spir_func double @_Z15__spirv_ocl_madddd(double %69, double %69, double %69)
  %71 = call spir_func double @_Z15__spirv_ocl_madddd(double %69, double %69, double %69)
  %72 = call spir_func double @_Z15__spirv_ocl_madddd(double %69, double %69, double %69)
  %73 = call spir_func double @_Z15__spirv_ocl_madddd(double %69, double %69, double %69)
  %74 = call spir_func double @_Z15__spirv_ocl_madddd(double %69, double %69, double %69)
  %75 = getelementptr inbounds [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]], [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]] addrspace(1)* %0, i64 0, i64 1, i64 3, i64 1, i64 undef, i64 %5, i64 undef
  %76 = load double, double addrspace(1)* %75, align 8
  %77 = call spir_func double @_Z15__spirv_ocl_madddd(double %76, double %76, double %76)
  %78 = call spir_func double @_Z15__spirv_ocl_madddd(double %76, double %76, double %76)
  %79 = call spir_func double @_Z15__spirv_ocl_madddd(double %76, double %76, double %76)
  %80 = call spir_func double @_Z15__spirv_ocl_madddd(double %76, double %76, double %76)
  %81 = call spir_func double @_Z15__spirv_ocl_madddd(double %76, double %76, double %76)
  %82 = getelementptr inbounds [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]], [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]] addrspace(1)* %0, i64 0, i64 2, i64 3, i64 1, i64 undef, i64 %5, i64 undef
  %83 = load double, double addrspace(1)* %82, align 8
  %84 = call spir_func double @_Z15__spirv_ocl_madddd(double %83, double %83, double %83)
  %85 = call spir_func double @_Z15__spirv_ocl_madddd(double %84, double %84, double %84)
  %86 = call spir_func double @_Z15__spirv_ocl_madddd(double %85, double %85, double %85)
  %87 = call spir_func double @_Z15__spirv_ocl_madddd(double %86, double %86, double %86)
  %88 = call spir_func double @_Z15__spirv_ocl_madddd(double %87, double %87, double %87)
  %89 = getelementptr inbounds [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]], [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]] addrspace(1)* %0, i64 0, i64 3, i64 3, i64 1, i64 undef, i64 %5, i64 undef
  %90 = load double, double addrspace(1)* %89, align 8
  %91 = call spir_func double @_Z15__spirv_ocl_madddd(double %90, double %90, double %90)
  %92 = call spir_func double @_Z15__spirv_ocl_madddd(double %90, double %90, double %90)
  %93 = call spir_func double @_Z15__spirv_ocl_madddd(double %90, double %90, double %90)
  %94 = call spir_func double @_Z15__spirv_ocl_madddd(double %90, double %90, double %90)
  %95 = call spir_func double @_Z15__spirv_ocl_madddd(double %90, double %90, double %90)
  %96 = fdiv double 0.000000e+00, %15
  store double 0.000000e+00, double addrspace(1)* null, align 8
  %97 = fmul double %39, 0.000000e+00
  %98 = fmul double %69, %96
  %99 = call spir_func double @_Z15__spirv_ocl_madddd(double %32, double %32, double %32)
  %100 = call spir_func double @_Z15__spirv_ocl_madddd(double %45, double %45, double %45)
  %101 = call spir_func double @_Z15__spirv_ocl_madddd(double %76, double %76, double %76)
  %102 = call spir_func double @_Z15__spirv_ocl_madddd(double %48, double %48, double %48)
  %103 = call spir_func double @_Z15__spirv_ocl_madddd(double %88, double %88, double %88)
  %104 = getelementptr inbounds [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]], [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]] addrspace(1)* %0, i64 0, i64 2, i64 0, i64 2, i64 undef, i64 %5, i64 undef
  %105 = load double, double addrspace(1)* %104, align 8
  %106 = call spir_func double @_Z15__spirv_ocl_madddd(double %105, double %105, double %105)
  %107 = getelementptr inbounds [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]], [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]] addrspace(1)* %0, i64 0, i64 2, i64 1, i64 2, i64 undef, i64 %5, i64 undef
  %108 = load double, double addrspace(1)* %107, align 8
  %109 = call spir_func double @_Z15__spirv_ocl_madddd(double %108, double %108, double %108)
  %110 = getelementptr inbounds [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]], [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]] addrspace(1)* %0, i64 0, i64 2, i64 2, i64 2, i64 undef, i64 %5, i64 undef
  %111 = load double, double addrspace(1)* %110, align 8
  %112 = call spir_func double @_Z15__spirv_ocl_madddd(double %111, double %111, double %111)
  %113 = getelementptr inbounds [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]], [5 x [5 x [3 x [102 x [101 x [101 x double]]]]]] addrspace(1)* %0, i64 0, i64 2, i64 3, i64 2, i64 undef, i64 %5, i64 undef
  %114 = load double, double addrspace(1)* %113, align 8
  %115 = call spir_func double @_Z15__spirv_ocl_madddd(double %114, double %114, double %114)
  %116 = call spir_func double @_Z15__spirv_ocl_madddd(double %9, double %9, double %9)
  %117 = call spir_func double @_Z15__spirv_ocl_madddd(double %60, double %60, double %60)
  %118 = call spir_func double @_Z15__spirv_ocl_madddd(double %90, double %90, double %90)
  %119 = call spir_func double @_Z15__spirv_ocl_madddd(double %67, double %67, double %67)
  %120 = fdiv double 0.000000e+00, %99
  %121 = fmul double %100, %120
  store double %121, double addrspace(1)* %40, align 8
  %122 = call spir_func double @_Z15__spirv_ocl_madddd(double %97, double %97, double %97)
  store double %122, double addrspace(1)* %33, align 8
  %123 = call spir_func double @_Z15__spirv_ocl_madddd(double %98, double %98, double %98)
  %124 = call spir_func double @_Z15__spirv_ocl_madddd(double %48, double %48, double %48)
  %125 = call spir_func double @_Z15__spirv_ocl_madddd(double %103, double %103, double %103)
  %126 = call spir_func double @_Z15__spirv_ocl_madddd(double %105, double %105, double %105)
  %127 = call spir_func double @_Z15__spirv_ocl_madddd(double %108, double %108, double %108)
  %128 = call spir_func double @_Z15__spirv_ocl_madddd(double %111, double %111, double %111)
  %129 = call spir_func double @_Z15__spirv_ocl_madddd(double %115, double %115, double %115)
  %130 = call spir_func double @_Z15__spirv_ocl_madddd(double %8, double %8, double %8)
  %131 = call spir_func double @_Z15__spirv_ocl_madddd(double %55, double %55, double %55)
  store double %117, double addrspace(1)* %54, align 8
  %132 = call spir_func double @_Z15__spirv_ocl_madddd(double %90, double %90, double %90)
  %133 = call spir_func double @_Z15__spirv_ocl_madddd(double %119, double %119, double %119)
  store double %133, double addrspace(1)* %61, align 8
  %134 = fdiv double 0.000000e+00, %53
  %135 = fmul double %125, %134
  store double %135, double addrspace(1)* %82, align 8
  %136 = fmul double %106, 0.000000e+00
  store double %136, double addrspace(1)* %104, align 8
  store double %109, double addrspace(1)* %107, align 8
  store double %112, double addrspace(1)* %110, align 8
  store double %129, double addrspace(1)* %113, align 8
  store double %116, double addrspace(1)* %7, align 8
  %137 = call spir_func double @_Z15__spirv_ocl_madddd(double %123, double %123, double %123)
  store double %137, double addrspace(1)* %68, align 8
  %138 = call spir_func double @_Z15__spirv_ocl_madddd(double %77, double %77, double %77)
  store double %138, double addrspace(1)* %75, align 8
  %139 = call spir_func double @_Z15__spirv_ocl_madddd(double %91, double %91, double %91)
  store double %139, double addrspace(1)* %89, align 8
  br label %6
}

!igc.functions = !{}
