; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, dg2-supported

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_cache_controls -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

target triple = "spir64-unknown-unknown"

; CHECK-LLVM: spir_kernel {{.*}} !spirv.ParameterDecorations [[ParamDecID:![0-9]+]]
define spir_kernel void @test(i32 addrspace(1)* %dummy, i32 addrspace(1)* %buffer) !spirv.ParameterDecorations !3 {
entry:
  %0 = load i32, i32 addrspace(1)* %buffer, align 4
  store i32 %0, i32 addrspace(1)* %buffer, align 4
  ret void
}

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!opencl.spir.version = !{!2}
!opencl.ocl.version = !{!2}

!0 = !{i32 2, i32 2}
!1 = !{i32 3, i32 102000}
!2 = !{i32 1, i32 2}
!3 = !{!4, !5}
!4 = !{}
!5 = !{!6, !7}
; CHECK-LLVM: [[ParamDecID]] = !{!{{[0-9]+}}, [[BufferDecID:![0-9]+]]}
; CHECK-LLVM: [[BufferDecID]] = !{[[StoreDecID:![0-9]+]], [[LoadDecID:![0-9]+]]}
; CHECK-LLVM: [[StoreDecID]] = !{i32 6442, i32 0, i32 0}
; CHECK-LLVM: [[LoadDecID]] = !{i32 6443, i32 0, i32 1}
!6 = !{i32 6442, i32 0, i32 0}  ; {CacheControlLoadINTEL,   CacheLevel=0, Uncached}
!7 = !{i32 6443, i32 0, i32 1}  ; {CacheControlStoreINTEL,  CacheLevel=0, WriteThrough}
