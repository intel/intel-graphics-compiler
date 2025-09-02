; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, dg2-supported, llvm-15-or-older

; TODO: This test fails on LLVM 16 with opaque pointers, but passes with typed pointers. Once the necessary fixes are made, remove llvm-15-or-older from the list above. llvm-15-or-older was added to the list above to prevent non-zero return from llvm-lit on LLVM 16 build with typed pointers forced.
; XFAIL: llvm-16-plus

; LLVM with typed pointers/default pointer typing:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: llvm-spirv %t.bc -opaque-pointers=0 --spirv-ext=+SPV_INTEL_blocking_pipes,+SPV_INTEL_arbitrary_precision_integers -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; ModuleID = 'test/CodeGenOpenCL/pipe_builtin.cl'
source_filename = "test/CodeGenOpenCL/pipe_builtin.cl"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

%opencl.pipe_ro_t = type opaque
%opencl.pipe_wo_t = type opaque

; CHECK-LLVM: %spirv.Pipe._0 = type opaque
; CHECK-LLVM: %spirv.Pipe._1 = type opaque

; CHECK-LLVM: call spir_func void @_Z29__spirv_ReadPipeBlockingINTELPU3AS115__spirv_Pipe__0PU3AS4iii(%spirv.Pipe._0 addrspace(1)* %{{[0-9]+}}, i32 addrspace(4)* %{{[0-9]+}}, i32 4, i32 4)
; Function Attrs: convergent noinline nounwind optnone
define spir_func void @foo(%opencl.pipe_ro_t addrspace(1)* %p, i32 addrspace(1)* %ptr) #0 {
entry:
  %p.addr = alloca %opencl.pipe_ro_t addrspace(1)*, align 8
  %ptr.addr = alloca i32 addrspace(1)*, align 8
  store %opencl.pipe_ro_t addrspace(1)* %p, %opencl.pipe_ro_t addrspace(1)** %p.addr, align 8
  store i32 addrspace(1)* %ptr, i32 addrspace(1)** %ptr.addr, align 8
  %0 = load %opencl.pipe_ro_t addrspace(1)*, %opencl.pipe_ro_t addrspace(1)** %p.addr, align 8
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %ptr.addr, align 8
  %2 = addrspacecast i32 addrspace(1)* %1 to i32 addrspace(4)*
  call spir_func void @_Z29__spirv_ReadPipeBlockingINTELIiEv8ocl_pipePiii(%opencl.pipe_ro_t addrspace(1)* %0, i32 addrspace(4)* %2, i32 4, i32 4)
  ret void
}

; CHECK-LLVM: declare spir_func void @_Z29__spirv_ReadPipeBlockingINTELPU3AS115__spirv_Pipe__0PU3AS4iii(%spirv.Pipe._0 addrspace(1)*{{.*}}, i32 addrspace(4)*
declare dso_local spir_func void @_Z29__spirv_ReadPipeBlockingINTELIiEv8ocl_pipePiii(%opencl.pipe_ro_t addrspace(1)*, i32 addrspace(4)*, i32, i32)

; CHECK-LLVM: call spir_func void @_Z29__spirv_ReadPipeBlockingINTELPU3AS115__spirv_Pipe__0PU3AS4cii(%spirv.Pipe._0 addrspace(1)* %{{[0-9]+}}, i8 addrspace(4)* %{{[0-9]+}}, i32 4, i32 4)
; Function Attrs: convergent noinline nounwind optnone
define spir_func void @bar(%opencl.pipe_ro_t addrspace(1)* %p, i32 addrspace(1)* %ptr) #0 {
entry:
  %p.addr = alloca %opencl.pipe_ro_t addrspace(1)*, align 8
  %ptr.addr = alloca i32 addrspace(1)*, align 8
  store %opencl.pipe_ro_t addrspace(1)* %p, %opencl.pipe_ro_t addrspace(1)** %p.addr, align 8
  store i32 addrspace(1)* %ptr, i32 addrspace(1)** %ptr.addr, align 8
  %0 = load %opencl.pipe_ro_t addrspace(1)*, %opencl.pipe_ro_t addrspace(1)** %p.addr, align 8
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %ptr.addr, align 8
  %2 = addrspacecast i32 addrspace(1)* %1 to i8 addrspace(4)*
  call spir_func void @_Z29__spirv_ReadPipeBlockingINTELIiEv8ocl_pipePvii(%opencl.pipe_ro_t addrspace(1)* %0, i8 addrspace(4)* %2, i32 4, i32 4)
  ret void
}

; CHECK-LLVM: declare spir_func void @_Z29__spirv_ReadPipeBlockingINTELPU3AS115__spirv_Pipe__0PU3AS4cii(%spirv.Pipe._0 addrspace(1)*{{.*}}, i8 addrspace(4)*
declare dso_local spir_func void @_Z29__spirv_ReadPipeBlockingINTELIiEv8ocl_pipePvii(%opencl.pipe_ro_t addrspace(1)*, i8 addrspace(4)*, i32, i32)

; CHECK-LLVM: call spir_func void @_Z30__spirv_WritePipeBlockingINTELPU3AS115__spirv_Pipe__1PU3AS4iii(%spirv.Pipe._1 addrspace(1)* %{{[0-9]+}}, i32 addrspace(4)* %{{[0-9]+}}, i32 4, i32 4)
; Function Attrs: convergent noinline nounwind optnone
define spir_func void @boo(%opencl.pipe_wo_t addrspace(1)* %p, i32 addrspace(1)* %ptr) #0 {
entry:
  %p.addr = alloca %opencl.pipe_wo_t addrspace(1)*, align 8
  %ptr.addr = alloca i32 addrspace(1)*, align 8
  store %opencl.pipe_wo_t addrspace(1)* %p, %opencl.pipe_wo_t addrspace(1)** %p.addr, align 8
  store i32 addrspace(1)* %ptr, i32 addrspace(1)** %ptr.addr, align 8
  %0 = load %opencl.pipe_wo_t addrspace(1)*, %opencl.pipe_wo_t addrspace(1)** %p.addr, align 8
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %ptr.addr, align 8
  %2 = addrspacecast i32 addrspace(1)* %1 to i32 addrspace(4)*
  call spir_func void @_Z30__spirv_WritePipeBlockingINTELIKiEv8ocl_pipePiii(%opencl.pipe_wo_t addrspace(1)* %0, i32 addrspace(4)* %2, i32 4, i32 4)
  ret void
}

; CHECK-LLVM: declare spir_func void @_Z30__spirv_WritePipeBlockingINTELPU3AS115__spirv_Pipe__1PU3AS4iii(%spirv.Pipe._1 addrspace(1)*{{.*}}, i32 addrspace(4)*
declare dso_local spir_func void @_Z30__spirv_WritePipeBlockingINTELIKiEv8ocl_pipePiii(%opencl.pipe_wo_t addrspace(1)*, i32 addrspace(4)*, i32, i32)

; CHECK-LLVM: call spir_func void @_Z30__spirv_WritePipeBlockingINTELPU3AS115__spirv_Pipe__1PU3AS4cii(%spirv.Pipe._1 addrspace(1)* %{{[0-9]+}}, i8 addrspace(4)* %{{[0-9]+}}, i32 4, i32 4)
; Function Attrs: convergent noinline nounwind optnone
define spir_func void @baz(%opencl.pipe_wo_t addrspace(1)* %p, i32 addrspace(1)* %ptr) #0 {
entry:
  %p.addr = alloca %opencl.pipe_wo_t addrspace(1)*, align 8
  %ptr.addr = alloca i32 addrspace(1)*, align 8
  store %opencl.pipe_wo_t addrspace(1)* %p, %opencl.pipe_wo_t addrspace(1)** %p.addr, align 8
  store i32 addrspace(1)* %ptr, i32 addrspace(1)** %ptr.addr, align 8
  %0 = load %opencl.pipe_wo_t addrspace(1)*, %opencl.pipe_wo_t addrspace(1)** %p.addr, align 8
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %ptr.addr, align 8
  %2 = addrspacecast i32 addrspace(1)* %1 to i8 addrspace(4)*
  call spir_func void @_Z30__spirv_WritePipeBlockingINTELIKiEv8ocl_pipePvii(%opencl.pipe_wo_t addrspace(1)* %0, i8 addrspace(4)* %2, i32 4, i32 4)
  ret void
}

; CHECK-LLVM: declare spir_func void @_Z30__spirv_WritePipeBlockingINTELPU3AS115__spirv_Pipe__1PU3AS4cii(%spirv.Pipe._1 addrspace(1)*{{.*}}, i8 addrspace(4)*
declare dso_local spir_func void @_Z30__spirv_WritePipeBlockingINTELIKiEv8ocl_pipePvii(%opencl.pipe_wo_t addrspace(1)*, i8 addrspace(4)*, i32, i32)

; CHECK-LLVM: call spir_func void @_Z30__spirv_WritePipeBlockingINTELPU3AS115__spirv_Pipe__1PU3AS4iii.1(%spirv.Pipe._1 addrspace(1)* %{{[0-9]+}}, i9 addrspace(4)* %{{[0-9]+}}, i32 2, i32 2)
; Function Attrs: convergent mustprogress norecurse nounwind
define linkonce_odr dso_local spir_func void @WritePipeBLockingi9Pointer(i9 addrspace(4)* align 2 dereferenceable(2) %_Data) {
entry:
  %_Data.addr = alloca i9 addrspace(4)*, align 8
  %_WPipe = alloca %opencl.pipe_wo_t addrspace(1)*, align 8
  %_Data.addr.ascast = addrspacecast i9 addrspace(4)** %_Data.addr to i9 addrspace(4)* addrspace(4)*
  %_WPipe.ascast = addrspacecast %opencl.pipe_wo_t addrspace(1)** %_WPipe to %opencl.pipe_wo_t addrspace(1)* addrspace(4)*
  store i9 addrspace(4)* %_Data, i9 addrspace(4)* addrspace(4)* %_Data.addr.ascast, align 8
  %0 = bitcast %opencl.pipe_wo_t addrspace(1)** %_WPipe to i8*
  %1 = load %opencl.pipe_wo_t addrspace(1)*, %opencl.pipe_wo_t addrspace(1)* addrspace(4)* %_WPipe.ascast, align 8
  %2 = load i9 addrspace(4)*, i9 addrspace(4)* addrspace(4)* %_Data.addr.ascast, align 8
  call spir_func void @_Z30__spirv_WritePipeBlockingINTELIDU9_Ev8ocl_pipePKT_ii(%opencl.pipe_wo_t addrspace(1)* %1, i9 addrspace(4)* %2, i32 2, i32 2)
  ret void
}

; CHECK-LLVM: declare spir_func void @_Z30__spirv_WritePipeBlockingINTELPU3AS115__spirv_Pipe__1PU3AS4iii.1(%spirv.Pipe._1 addrspace(1)*{{.*}}, i9 addrspace(4)*
declare dso_local spir_func void @_Z30__spirv_WritePipeBlockingINTELIDU9_Ev8ocl_pipePKT_ii(%opencl.pipe_wo_t addrspace(1)*, i9 addrspace(4)*, i32, i32)

attributes #0 = { convergent noinline nounwind optnone "correctly-rounded-divide-sqrt-fp-math"="false" "denorms-are-zero"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }