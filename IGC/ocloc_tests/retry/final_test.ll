; REQUIRES: regkeys,pvc-supported, test-fix
; RUN: llvm-as %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options "-igc_opts 'VISAOptions=-asmToConsole'" &> %t_output.ll
; RUN: FileCheck --input-file %t_output.ll %s

; ATTENTION: if your change lowers spill size significantly congratulations! just adjust the numbers
; if it increases, there is a possibility of a degradation

; normal version
; CHECK://.kernel __omp_offloading_803_4268e__Z23directives_apply_BCs_v4_l362
; I'm trying to match 5 consecutive numbers starting with 2: 23477 for example, not 234770 and not 2347,
; lower boundary is set by {4} and upper boundary by matching EOL character {{$}}
; CHECK://.spill size 8[[A:[0-9]{2}]]{{$}}
; CHECK://.BankConflicts:

; CHECK://.kernel __omp_offloading_803_4268e__Z23directives_apply_BCs_v4_l362
; CHECK://.spill size 1[[A:[0-9]{2}]]{{$}}
; CHECK://.BankConflicts:

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

declare spir_func i64 @_Z12get_local_idj()

declare spir_func i64 @_Z12get_group_idj()

declare spir_func i64 @_Z14get_local_sizej()

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare i32 @llvm.fshl.i32(i32, i32, i32) #0

declare spir_func double @_Z15__spirv_ocl_madddd(double, double)

define spir_kernel void @__omp_offloading_803_4268e__Z23directives_apply_BCs_v4_l362(%struct.level_type addrspace(1)* %0, %struct.blockCopy_type addrspace(4)* %1, i32 addrspace(4)* %2, %struct.box_type addrspace(4)* %3, double addrspace(4)* addrspace(4)* %4, double addrspace(4)* %5, i64 %6, i32 %7, i32 %8, i32 %9, double addrspace(4)* %10, i32 %11, i32 %12, i32 %13, i32 %14, i64 %15, i64 %16, i32 %17, i64 %18, double addrspace(4)* %19) {
  %21 = getelementptr inbounds double, double addrspace(4)* %5, i64 %6
  %22 = call i32 @llvm.fshl.i32(i32 %7, i32 %7, i32 1)
  switch i32 %22, label %25 [
    i32 0, label %23
    i32 1, label %24
  ]

23:                                               ; preds = %20
  br label %25

24:                                               ; preds = %20
  br label %25

25:                                               ; preds = %24, %23, %20
  %26 = phi i32 [ 0, %20 ], [ 1, %24 ], [ 0, %23 ]
  %27 = phi i32 [ 0, %20 ], [ 0, %24 ], [ %8, %23 ]
  %28 = phi i32 [ -1, %20 ], [ 0, %24 ], [ 0, %23 ]
  %29 = shl nsw i32 %9, 2
  %30 = shl nsw i32 %28, 2
  %31 = add nsw i32 %27, %9
  %32 = sext i32 %31 to i64
  %33 = getelementptr inbounds double, double addrspace(4)* %10, i64 %32
  %34 = load double, double addrspace(4)* %33, align 8
  %35 = shl nsw i32 %26, 1
  %36 = add nsw i32 1, %27
  %37 = add nsw i32 %11, %28
  %38 = sext i32 %37 to i64
  %39 = getelementptr inbounds double, double addrspace(4)* %10, i64 %38
  %40 = load double, double addrspace(4)* %39, align 8
  %41 = sext i32 %30 to i64
  %42 = getelementptr inbounds double, double addrspace(4)* %21, i64 %41
  %43 = load double, double addrspace(4)* %42, align 8
  %44 = mul nsw i32 %27, 3
  %45 = sext i32 %44 to i64
  %46 = getelementptr inbounds double, double addrspace(4)* %10, i64 %45
  %47 = load double, double addrspace(4)* %46, align 8
  %48 = add nsw i32 %44, 1
  %49 = sext i32 %48 to i64
  %50 = getelementptr inbounds double, double addrspace(4)* %21, i64 %49
  %51 = load double, double addrspace(4)* %50, align 8
  %52 = shl nsw i32 %27, 1
  %53 = sext i32 %27 to i64
  %54 = getelementptr inbounds double, double addrspace(4)* %21, i64 %53
  %55 = load double, double addrspace(4)* %54, align 8
  %56 = getelementptr inbounds double, double addrspace(4)* %10, i64 %41
  %57 = load double, double addrspace(4)* %56, align 8
  %58 = getelementptr inbounds double, double addrspace(4)* %10, i64 %53
  %59 = load double, double addrspace(4)* %58, align 8
  %60 = sext i32 %35 to i64
  %61 = getelementptr inbounds double, double addrspace(4)* %10, i64 %60
  %62 = load double, double addrspace(4)* %61, align 8
  %63 = mul nsw i32 %28, 3
  %64 = add nsw i32 2, %63
  %65 = sext i32 %64 to i64
  %66 = getelementptr inbounds double, double addrspace(4)* %21, i64 %65
  %67 = load double, double addrspace(4)* %66, align 8
  %68 = sext i32 %36 to i64
  %69 = getelementptr inbounds double, double addrspace(4)* %10, i64 %68
  %70 = load double, double addrspace(4)* %69, align 8
  %71 = sext i32 %63 to i64
  %72 = getelementptr inbounds double, double addrspace(4)* %10, i64 %71
  %73 = load double, double addrspace(4)* %72, align 8
  %74 = sext i32 %26 to i64
  %75 = getelementptr inbounds double, double addrspace(4)* %21, i64 %74
  %76 = load double, double addrspace(4)* %75, align 8
  %77 = getelementptr inbounds double, double addrspace(4)* %21, i64 %45
  %78 = load double, double addrspace(4)* %77, align 8
  %79 = sext i32 %52 to i64
  %80 = getelementptr inbounds double, double addrspace(4)* %10, i64 %79
  %81 = load double, double addrspace(4)* %80, align 8
  %82 = add nsw i32 %13, %63
  %83 = sext i32 %82 to i64
  %84 = getelementptr inbounds double, double addrspace(4)* %10, i64 %83
  %85 = load double, double addrspace(4)* %84, align 8
  %86 = sext i32 %9 to i64
  %87 = getelementptr inbounds double, double addrspace(4)* %10, i64 %86
  %88 = load double, double addrspace(4)* %87, align 8
  %89 = getelementptr inbounds double, double addrspace(4)* %10, i64 %74
  %90 = load double, double addrspace(4)* %89, align 8
  %91 = sext i32 %29 to i64
  %92 = getelementptr inbounds double, double addrspace(4)* %10, i64 %91
  %93 = load double, double addrspace(4)* %92, align 8
  %94 = sext i32 %28 to i64
  %95 = getelementptr inbounds double, double addrspace(4)* %21, i64 %94
  %96 = load double, double addrspace(4)* %95, align 8
  %97 = add nsw i32 %27, %28
  %98 = sext i32 %97 to i64
  %99 = getelementptr inbounds double, double addrspace(4)* %21, i64 %98
  %100 = load double, double addrspace(4)* %99, align 8
  %101 = add nsw i32 %44, %28
  %102 = sext i32 %101 to i64
  %103 = getelementptr inbounds double, double addrspace(4)* %21, i64 %102
  %104 = load double, double addrspace(4)* %103, align 8
  %105 = add nsw i32 %9, %28
  %106 = sext i32 %105 to i64
  %107 = getelementptr inbounds double, double addrspace(4)* %10, i64 %106
  %108 = load double, double addrspace(4)* %107, align 8
  %109 = add nsw i32 1, %28
  %110 = sext i32 %109 to i64
  %111 = getelementptr inbounds double, double addrspace(4)* %21, i64 %110
  %112 = load double, double addrspace(4)* %111, align 8
  %113 = sext i32 %13 to i64
  %114 = getelementptr inbounds double, double addrspace(4)* %10, i64 %113
  %115 = load double, double addrspace(4)* %114, align 8
  %116 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %117 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %118 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %119 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %120 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %121 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %122 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %123 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %124 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %125 = call spir_func double @_Z15__spirv_ocl_madddd(double %115, double 0.000000e+00)
  %126 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %127 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %128 = call spir_func double @_Z15__spirv_ocl_madddd(double %108, double %112)
  %129 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %130 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %131 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %132 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %133 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %134 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %135 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %136 = call spir_func double @_Z15__spirv_ocl_madddd(double %104, double %100)
  %137 = call spir_func double @_Z15__spirv_ocl_madddd(double %93, double 0.000000e+00)
  %138 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %139 = call spir_func double @_Z15__spirv_ocl_madddd(double %96, double %93)
  %140 = call spir_func double @_Z15__spirv_ocl_madddd(double %88, double %90)
  %141 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %142 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %143 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %144 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %145 = call spir_func double @_Z15__spirv_ocl_madddd(double %85, double %81)
  %146 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %147 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %148 = call spir_func double @_Z15__spirv_ocl_madddd(double %78, double %76)
  %149 = call spir_func double @_Z15__spirv_ocl_madddd(double %67, double %70)
  %150 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %151 = call spir_func double @_Z15__spirv_ocl_madddd(double %73, double %149)
  %152 = call spir_func double @_Z15__spirv_ocl_madddd(double %59, double %62)
  %153 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %154 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %155 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %156 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %157 = call spir_func double @_Z15__spirv_ocl_madddd(double %57, double %55)
  %158 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %159 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %160 = call spir_func double @_Z15__spirv_ocl_madddd(double %51, double %47)
  %161 = call spir_func double @_Z15__spirv_ocl_madddd(double %34, double %40)
  %162 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %163 = call spir_func double @_Z15__spirv_ocl_madddd(double %43, double %161)
  %164 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %165 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %166 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %167 = call spir_func double @_Z15__spirv_ocl_madddd(double 0.000000e+00, double 0.000000e+00)
  %168 = call spir_func double @_Z15__spirv_ocl_madddd(double %136, double %128)
  %169 = call spir_func double @_Z15__spirv_ocl_madddd(double %139, double %168)
  %170 = call spir_func double @_Z15__spirv_ocl_madddd(double %140, double %145)
  %171 = call spir_func double @_Z15__spirv_ocl_madddd(double %148, double %170)
  %172 = call spir_func double @_Z15__spirv_ocl_madddd(double %151, double %171)
  %173 = call spir_func double @_Z15__spirv_ocl_madddd(double %152, double %157)
  %174 = call spir_func double @_Z15__spirv_ocl_madddd(double %160, double %173)
  %175 = call spir_func double @_Z15__spirv_ocl_madddd(double %163, double %174)
  %176 = call spir_func double @_Z15__spirv_ocl_madddd(double %115, double %169)
  %177 = call spir_func double @_Z15__spirv_ocl_madddd(double %172, double %176)
  %178 = call spir_func double @_Z15__spirv_ocl_madddd(double %175, double %177)
  %179 = sext i32 %11 to i64
  %180 = getelementptr inbounds double, double addrspace(4)* %10, i64 %179
  store double 0.000000e+00, double addrspace(4)* %180, align 8
  store double %178, double addrspace(4)* %5, align 8
  ret void
}

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{}
