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
; if it increases, there is a possibility of a degradation

; normal version
; CHECK://.kernel __omp_offloading_803_4268e__Z23directives_apply_BCs_v4_l362
; I'm trying to match 5 consecutive numbers starting with 2: 23477 for example, not 234770 and not 2347,
; lower boundary is set by {4} and upper boundary by matching EOL character {{$}}
; CHECK://.spill size 5[[A:[0-9]{3}]]{{$}}
; CHECK: end of thread

; retry version
; CHECK://.kernel __omp_offloading_803_4268e__Z23directives_apply_BCs_v4_l362
; CHECK://.spill size 2[[B:[0-9]{3}]]{{$}}
; CHECK: end of thread

; ModuleID = 'reduced.ll'
source_filename = "reduced.ll"

%struct.level_type = type { double, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, %struct.anon, %struct.anon, i32 addrspace(4)*, i32, %struct.box_type addrspace(4)*, double addrspace(4)* addrspace(4)*, i32, i32, %struct.blockCopy_type addrspace(4)*, %struct.anon.4, [3 x %struct.communicator_type], [4 x %struct.communicator_type], %struct.communicator_type, i32, double, i32, double addrspace(4)*, i32, i32, i32, double addrspace(4)*, double addrspace(4)*, %struct.anon.5, i32, i32, i32 }
%struct.anon = type { i32, i32, i32 }
%struct.anon.4 = type { i32, [3 x i32], [3 x i32], [3 x %struct.blockCopy_type addrspace(4)*] }
%struct.communicator_type = type { i32, i32, i32 addrspace(4)*, i32 addrspace(4)*, i32 addrspace(4)*, i32 addrspace(4)*, double addrspace(4)* addrspace(4)*, double addrspace(4)* addrspace(4)*, [3 x i32], [3 x i32], [3 x %struct.blockCopy_type addrspace(4)*], i32 addrspace(4)*, %struct.MPI_Status addrspace(4)* }
%struct.MPI_Status = type { i32, i32, i32, i32, i32 }
%struct.anon.5 = type { double, double, double, double, double, double, double, double, double, double, double, double, double, double, double, double, double, double, double, double, double, double, double, double, double, double, double, double, double }
%struct.blockCopy_type = type { i32, %struct.anon, %struct.anon.3, %struct.anon.3 }
%struct.anon.3 = type { i32, i32, i32, i32, i32, i32, double addrspace(4)* }
%struct.box_type = type { i32, %struct.anon, i32, i32, i32, i32, i32, i32, double addrspace(4)* addrspace(4)* }

@faces = external addrspace(1) global [27 x i32]

declare spir_func i64 @_Z12get_local_idj(i32)

declare spir_func i64 @_Z12get_group_idj()

declare spir_func i64 @_Z14get_local_sizej(i32)

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare i32 @llvm.fshl.i32(i32, i32, i32) #0

declare spir_func double @_Z15__spirv_ocl_madddd(double, double, double)

define spir_kernel void @__omp_offloading_803_4268e__Z23directives_apply_BCs_v4_l362(%struct.level_type addrspace(1)* %0, i64 %1, i64 %2, i64 %3, i64 %4, %struct.blockCopy_type addrspace(4)* %5, %struct.box_type addrspace(4)* %6, i32 %7, double addrspace(4)* %8, i32 %9, i64 %10, double addrspace(4)* %11, i32 %12, i32 %13, double %14, double %15, double %16, double %17, double %18, double %19, double %20, double %21, double %22, i32 %23, i32 %24, i64 %25) {
.loopexit2:
  %26 = call spir_func i64 @_Z12get_local_idj(i32 0)
  %27 = trunc i64 %26 to i32
  %28 = sext i32 %27 to i64
  %29 = getelementptr inbounds %struct.box_type, %struct.box_type addrspace(4)* %6, i64 %28, i32 8
  %30 = load double addrspace(4)* addrspace(4)*, double addrspace(4)* addrspace(4)* addrspace(4)* %29, align 8
  %31 = load double addrspace(4)*, double addrspace(4)* addrspace(4)* %30, align 8
  %cond = icmp eq i32 %27, 0
  %32 = select i1 %cond, i32 %7, i32 0
  %33 = select i1 %cond, i32 0, i32 -1
  %34 = mul nsw i32 %33, %7
  %35 = add nsw i32 %34, %9
  %36 = add nsw i32 %35, 1
  %37 = shl nsw i32 %32, 2
  %38 = sext i32 %13 to i64
  %39 = getelementptr inbounds double, double addrspace(4)* %31, i64 %38
  %40 = load double, double addrspace(4)* %39, align 8
  %41 = add nsw i32 %34, %7
  %42 = add nsw i32 %41, 1
  %43 = load double, double addrspace(4)* %31, align 8
  %44 = getelementptr inbounds double, double addrspace(4)* %31, i64 undef
  %45 = load double, double addrspace(4)* %44, align 8
  %46 = sext i32 %37 to i64
  %47 = getelementptr inbounds double, double addrspace(4)* %31, i64 %46
  %48 = load double, double addrspace(4)* %47, align 8
  %49 = mul nsw i32 %32, 3
  %50 = sext i32 %49 to i64
  %51 = getelementptr inbounds double, double addrspace(4)* %31, i64 %50
  %52 = load double, double addrspace(4)* %51, align 8
  %53 = getelementptr inbounds double, double addrspace(4)* %31, i64 3
  %54 = load double, double addrspace(4)* %53, align 8
  %55 = sext i32 %12 to i64
  %56 = getelementptr inbounds double, double addrspace(4)* %31, i64 %55
  %57 = load double, double addrspace(4)* %56, align 8
  %58 = getelementptr inbounds double, double addrspace(4)* %31, i64 1
  %59 = load double, double addrspace(4)* %58, align 8
  %60 = sext i32 %33 to i64
  %61 = getelementptr inbounds double, double addrspace(4)* %8, i64 %60
  %62 = load double, double addrspace(4)* %61, align 8
  %63 = sext i32 %42 to i64
  %64 = getelementptr inbounds double, double addrspace(4)* %31, i64 %63
  %65 = load double, double addrspace(4)* %64, align 8
  %66 = sext i32 %7 to i64
  %67 = getelementptr inbounds double, double addrspace(4)* %31, i64 %66
  %68 = load double, double addrspace(4)* %67, align 8
  %69 = sext i32 %36 to i64
  %70 = getelementptr inbounds double, double addrspace(4)* %31, i64 %69
  %71 = load double, double addrspace(4)* %70, align 8
  %72 = getelementptr inbounds double, double addrspace(4)* %31, i64 -1
  %73 = load double, double addrspace(4)* %72, align 8
  %74 = sext i32 %32 to i64
  %75 = getelementptr inbounds double, double addrspace(4)* %8, i64 %74
  %76 = load double, double addrspace(4)* %75, align 8
  %77 = add nsw i32 %9, %32
  %78 = sext i32 %77 to i64
  %79 = getelementptr inbounds double, double addrspace(4)* %31, i64 %78
  %80 = load double, double addrspace(4)* %79, align 8
  %81 = sext i32 %9 to i64
  %82 = getelementptr inbounds double, double addrspace(4)* %31, i64 %81
  %83 = load double, double addrspace(4)* %82, align 8
  %84 = call spir_func double @_Z15__spirv_ocl_madddd(double %14, double -7.700000e+01, double 0.000000e+00)
  %85 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double %14)
  %86 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double %14)
  %87 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %88 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %89 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %90 = call spir_func double @_Z15__spirv_ocl_madddd(double %73, double 0.000000e+00, double 0.000000e+00)
  %91 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %92 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %93 = fmul double %90, 0.000000e+00
  %94 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %95 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %96 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %97 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %98 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %99 = call spir_func double @_Z15__spirv_ocl_madddd(double %45, double 0.000000e+00, double 0.000000e+00)
  %100 = fmul double %99, 0.000000e+00
  %101 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %102 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %103 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %104 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %105 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %106 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %107 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %108 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %109 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %110 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %111 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %112 = call spir_func double @_Z15__spirv_ocl_madddd(double %45, double 0.000000e+00, double 0.000000e+00)
  %113 = fmul double %112, 0x3FB5555555555555
  %114 = call spir_func double @_Z15__spirv_ocl_madddd(double %14, double 0.000000e+00, double 0.000000e+00)
  %115 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %116 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %117 = fmul double %45, 0.000000e+00
  %118 = call spir_func double @_Z15__spirv_ocl_madddd(double %14, double 0.000000e+00, double 0.000000e+00)
  %119 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %120 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double %52)
  %121 = fmul double %120, 0.000000e+00
  %122 = call spir_func double @_Z15__spirv_ocl_madddd(double %43, double 0.000000e+00, double 0.000000e+00)
  %123 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double %122)
  %124 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %125 = fmul double %123, 0.000000e+00
  %126 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %127 = call spir_func double @_Z15__spirv_ocl_madddd(double %14, double 0.000000e+00, double 0.000000e+00)
  %128 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double %48)
  %129 = fmul double %128, 0.000000e+00
  %130 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %131 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %132 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %133 = fmul double %43, 0.000000e+00
  %134 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double %133)
  %135 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %136 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %137 = fmul double %134, 0.000000e+00
  %138 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %139 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %140 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %141 = fmul double %43, 3.350000e+02
  %142 = call spir_func double @_Z15__spirv_ocl_madddd(double %16, double -5.050000e+02, double %141)
  %143 = call spir_func double @_Z15__spirv_ocl_madddd(double %83, double 0.000000e+00, double %142)
  %144 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double %143)
  %145 = fmul double %144, 0.000000e+00
  %146 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %147 = call spir_func double @_Z15__spirv_ocl_madddd(double %80, double 0.000000e+00, double %76)
  %148 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %149 = call spir_func double @_Z15__spirv_ocl_madddd(double %14, double -5.050000e+02, double 0.000000e+00)
  %150 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %151 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %152 = call spir_func double @_Z15__spirv_ocl_madddd(double %71, double 0.000000e+00, double 0.000000e+00)
  %153 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %154 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %155 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %156 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %157 = call spir_func double @_Z15__spirv_ocl_madddd(double %45, double 0.000000e+00, double 0.000000e+00)
  %158 = fmul double %157, 0x3FB5555555555555
  %159 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %160 = call spir_func double @_Z15__spirv_ocl_madddd(double %68, double 0.000000e+00, double 0.000000e+00)
  %161 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %162 = call spir_func double @_Z15__spirv_ocl_madddd(double %62, double 0.000000e+00, double %65)
  %163 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %164 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %165 = fmul double %59, 0.000000e+00
  %166 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %167 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %168 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %169 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %170 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %171 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double %57)
  %172 = fmul double %171, 0.000000e+00
  %173 = fmul double %52, 0.000000e+00
  %174 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %175 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %176 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %177 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %178 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %179 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %180 = call spir_func double @_Z15__spirv_ocl_madddd(double %43, double 0.000000e+00, double %14)
  %181 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double %180)
  %182 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double %181)
  %183 = call spir_func double @_Z15__spirv_ocl_madddd(double %14, double -5.050000e+02, double 3.350000e+02)
  %184 = call spir_func double @_Z15__spirv_ocl_madddd(double %48, double 0.000000e+00, double 3.350000e+02)
  %185 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double %184)
  %186 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double %14)
  %187 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double %14)
  %188 = call spir_func double @_Z15__spirv_ocl_madddd(double %45, double 0.000000e+00, double %14)
  %189 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %190 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %191 = call spir_func double @_Z15__spirv_ocl_madddd(double %14, double 0.000000e+00, double %133)
  %192 = fmul double %191, 0.000000e+00
  %193 = call spir_func double @_Z15__spirv_ocl_madddd(double %40, double 0.000000e+00, double 0.000000e+00)
  %194 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double %193)
  %195 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %196 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %197 = call spir_func double @_Z15__spirv_ocl_madddd(double %93, double 0.000000e+00, double 0.000000e+00)
  %198 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %199 = fmul double %197, 0.000000e+00
  %200 = call spir_func double @_Z15__spirv_ocl_madddd(double %100, double -7.700000e+01, double 0.000000e+00)
  %201 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %202 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %203 = call spir_func double @_Z15__spirv_ocl_madddd(double %113, double -7.700000e+01, double %117)
  %204 = call spir_func double @_Z15__spirv_ocl_madddd(double %121, double 0.000000e+00, double %203)
  %205 = call spir_func double @_Z15__spirv_ocl_madddd(double %125, double 0.000000e+00, double %204)
  %206 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double -7.700000e+01, double 0.000000e+00)
  %207 = call spir_func double @_Z15__spirv_ocl_madddd(double %137, double 0.000000e+00, double %129)
  %208 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %209 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %210 = call spir_func double @_Z15__spirv_ocl_madddd(double %93, double 0.000000e+00, double %15)
  %211 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %212 = call spir_func double @_Z15__spirv_ocl_madddd(double %100, double 0.000000e+00, double 0.000000e+00)
  %213 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %214 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %215 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %216 = call spir_func double @_Z15__spirv_ocl_madddd(double %121, double 0.000000e+00, double %117)
  %217 = call spir_func double @_Z15__spirv_ocl_madddd(double %122, double 0.000000e+00, double %216)
  %218 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %219 = call spir_func double @_Z15__spirv_ocl_madddd(double %137, double 0.000000e+00, double %128)
  %220 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %221 = call spir_func double @_Z15__spirv_ocl_madddd(double %145, double 0.000000e+00, double %147)
  %222 = call spir_func double @_Z15__spirv_ocl_madddd(double %73, double 0.000000e+00, double %221)
  %223 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %224 = call spir_func double @_Z15__spirv_ocl_madddd(double %14, double 0.000000e+00, double 0.000000e+00)
  %225 = call spir_func double @_Z15__spirv_ocl_madddd(double %162, double 0.000000e+00, double %160)
  %226 = call spir_func double @_Z15__spirv_ocl_madddd(double %165, double 0.000000e+00, double %225)
  %227 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %228 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %229 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %230 = fmul double %54, 0.000000e+00
  %231 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %232 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %233 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %234 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %235 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %236 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %237 = fmul double %152, 0.000000e+00
  %238 = call spir_func double @_Z15__spirv_ocl_madddd(double %158, double 0.000000e+00, double 0.000000e+00)
  %239 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %240 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %241 = call spir_func double @_Z15__spirv_ocl_madddd(double %172, double 0.000000e+00, double %173)
  %242 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %243 = call spir_func double @_Z15__spirv_ocl_madddd(double %182, double 0.000000e+00, double %241)
  %244 = fmul double %188, 0.000000e+00
  %245 = call spir_func double @_Z15__spirv_ocl_madddd(double %185, double 0.000000e+00, double %244)
  %246 = call spir_func double @_Z15__spirv_ocl_madddd(double %192, double 0.000000e+00, double %245)
  %247 = call spir_func double @_Z15__spirv_ocl_madddd(double %194, double 0.000000e+00, double %246)
  %248 = fmul double %247, 0.000000e+00
  %249 = call spir_func double @_Z15__spirv_ocl_madddd(double %199, double 0.000000e+00, double %200)
  %250 = call spir_func double @_Z15__spirv_ocl_madddd(double %205, double 0.000000e+00, double %249)
  %251 = call spir_func double @_Z15__spirv_ocl_madddd(double %207, double 0.000000e+00, double %250)
  %252 = call spir_func double @_Z15__spirv_ocl_madddd(double %210, double 0.000000e+00, double %212)
  %253 = call spir_func double @_Z15__spirv_ocl_madddd(double %217, double 0.000000e+00, double %252)
  %254 = call spir_func double @_Z15__spirv_ocl_madddd(double %219, double 0.000000e+00, double %253)
  %255 = call spir_func double @_Z15__spirv_ocl_madddd(double %222, double 0.000000e+00, double %226)
  %256 = call spir_func double @_Z15__spirv_ocl_madddd(double %230, double 0.000000e+00, double %255)
  %257 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00, double 0.000000e+00)
  %258 = call spir_func double @_Z15__spirv_ocl_madddd(double %237, double 0.000000e+00, double %238)
  %259 = call spir_func double @_Z15__spirv_ocl_madddd(double %243, double 0.000000e+00, double %258)
  %260 = call spir_func double @_Z15__spirv_ocl_madddd(double %248, double 0.000000e+00, double %259)
  %261 = sext i32 %34 to i64
  %262 = getelementptr inbounds double, double addrspace(4)* %31, i64 %261
  store double 0.000000e+00, double addrspace(4)* %262, align 8
  %263 = sub nsw i32 %34, %32
  %264 = sext i32 %263 to i64
  %265 = getelementptr inbounds double, double addrspace(4)* %31, i64 %264
  store double %251, double addrspace(4)* %265, align 8
  store double %254, double addrspace(4)* %31, align 8
  %266 = getelementptr inbounds double, double addrspace(4)* %31, i64 %25
  store double %256, double addrspace(4)* %266, align 8
  store double %260, double addrspace(4)* null, align 8
  ret void
}

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }
