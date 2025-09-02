; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, dg2-supported, llvm-14-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: llvm-spirv %t.bc -opaque-pointers=1 --spirv-ext=+SPV_INTEL_cache_controls -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'EnableOpaquePointersBackend=1,ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; LLVM with typed pointers/default pointer typing:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: llvm-spirv %t.bc -opaque-pointers=0 --spirv-ext=+SPV_INTEL_cache_controls -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; CHECK-LLVM: @p = common addrspace(1) global i32 0, align 4, !spirv.Decorations [[GlobalMD:![0-9]+]]
; CHECK-LLVM: store i32 0, {{i32|ptr}} addrspace(1){{.*}} @p, align 4

; CHECK-LLVM-DAG: [[CC0:![0-9]+]] = !{i32 6443, i32 0, i32 1}
; CHECK-LLVM-DAG: [[CC1:![0-9]+]] = !{i32 6443, i32 1, i32 3}
; CHECK-LLVM-DAG: [[GlobalMD]] = {{.*}}[[CC0]]{{.*}}[[CC1]]

target triple = "spir64-unknown-unknown"

@p = common addrspace(1) global i32 0, align 4, !spirv.Decorations !3

define spir_kernel void @test() {
entry:
  store i32 0, i32 addrspace(1)* @p, align 4
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
!4 = !{i32 6443, i32 0, i32 1}  ; {CacheControlStoreINTEL, CacheLevel=0, WriteThrough}
!5 = !{i32 6443, i32 1, i32 3}  ; {CacheControlStoreINTEL, CacheLevel=1, Streaming}
