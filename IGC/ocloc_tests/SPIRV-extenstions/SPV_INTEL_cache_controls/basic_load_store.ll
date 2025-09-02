; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, dg2-supported, llvm-14-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: llvm-spirv %t.bc -opaque-pointers=1 -spirv-ext=+SPV_INTEL_cache_controls -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'EnableOpaquePointersBackend=1,ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; LLVM with typed pointers/default pointer typing:
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc -spirv-ext=+SPV_INTEL_cache_controls -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; CHECK-LLVM: %arrayidx = getelementptr inbounds i32, {{i32|ptr}} addrspace(1){{.*}} %buffer, i64 1, !spirv.Decorations [[LoadMD:![0-9]+]]
; CHECK-LLVM: load i32, {{i32|ptr}} addrspace(1){{.*}} %arrayidx, align 4

; CHECK-LLVM: %arrayidx1 = getelementptr inbounds i32, {{i32|ptr}} addrspace(1){{.*}} %buffer, i64 0, !spirv.Decorations [[StoreMD:![0-9]+]]
; CHECK-LLVM: store i32 %0, {{i32|ptr}} addrspace(1){{.*}} %arrayidx1, align 4

; CHECK-LLVM: [[LoadMD]] = !{[[CC0:![0-9]+]], [[CC1:![0-9]+]]}
; CHECK-LLVM: [[CC0]] = !{i32 6442, i32 0, i32 1}
; CHECK-LLVM: [[CC1]] = !{i32 6442, i32 1, i32 1}

; CHECK-LLVM: [[StoreMD]] = !{[[CC2:![0-9]+]], [[CC3:![0-9]+]]}
; CHECK-LLVM: [[CC2]] = !{i32 6443, i32 0, i32 1}
; CHECK-LLVM: [[CC3]] = !{i32 6443, i32 1, i32 2}

target triple = "spir64-unknown-unknown"

define spir_kernel void @test(i32 addrspace(1)* %buffer) {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %buffer, i64 1, !spirv.Decorations !3
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %buffer, i64 0, !spirv.Decorations !6
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
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
!4 = !{i32 6442, i32 0, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=0, Cached}
!5 = !{i32 6442, i32 1, i32 1}  ; {CacheControlLoadINTEL, CacheLevel=1, Cached}
!6 = !{!7, !8}
!7 = !{i32 6443, i32 0, i32 1}  ; {CacheControlStoreINTEL, CacheLevel=0, WriteThrough}
!8 = !{i32 6443, i32 1, i32 2}  ; {CacheControlStoreINTEL, CacheLevel=1, WriteBack}
