// EnableKernelNamesBasedHash and ShaderOverride are unavailable in Release builds.
// UNSUPPORTED: release
// REQUIRES: regkeys, bmg-supported, llvm-16-plus

// Test checks visaasm ShaderOverride

// Dump a valid .visaasm file from original.ll,
// Then compile with ShaderOverride enabled with Override directory pointing to this visaasm.
// Kernel hashes should be the same for override to work (IGC_EnableKernelNamesBasedHash to get same hash)

// Pre test clean up:
// RUN: rm -rf %t.dump_dir %t.override_dir %t.overriden_dir
// RUN: mkdir -p %t.override_dir
// RUN: split-file %s %t

// RUN: llvm-as %OPAQUE_PTR_FLAG% %t/original.ll -o %t/original.bc
// RUN: llvm-as %OPAQUE_PTR_FLAG% %t/overriden.ll -o %t/overriden.bc

// Compile original shader
// RUN: IGC_EnableKernelNamesBasedHash=1 IGC_ShaderDumpEnable=1 \
// RUN: IGC_DumpToCustomDir=%t.dump_dir \
// RUN: ocloc compile -device bmg -file %t/original.bc -llvm_input -options "-igc_opts 'EnableOpaquePointersBackend=1, VISAOptions=-asmToConsole'" | FileCheck %s

// So that we don't try to override using binary or llvm-ir
// RUN: cp %t.dump_dir/*.visaasm %t.override_dir/

// Override:
// RUN: IGC_EnableKernelNamesBasedHash=1 IGC_ShaderOverride=1 \
// RUN: IGC_ShaderDumpEnable=1 IGC_DumpToCustomDir=%t.overriden_dir \
// RUN: IGC_ShaderOverrideFromDir=%t.override_dir \
// RUN: ocloc compile -device bmg -file %t/overriden.bc -llvm_input  -options "-igc_opts 'EnableOpaquePointersBackend=1, VISAOptions=-asmToConsole'" | FileCheck %s
// RUN: FileCheck %s --input-file %t.override_dir/OverrideLog.txt -check-prefix=OVERRIDE-LOG-CHECK

// Expect that both resulting asm store same value (42)
//
// CHECK: mov (1|M0) [[REG:r[0-9]*]]{{.*}}<1>:d     42:w
// CHECK: store.ugm.{{.*}} [{{.*}}]     [[REG]]:1

// Check log for overriden shader location
// OVERRIDE-LOG-CHECK: OVERRIDEN: {{.*}}overriden_dir/OCL_{{.*}}.visaasm

//--- original.ll
define spir_kernel void @shader_override_visaasm(ptr addrspace(1) align 4 %out) {
entry:
  %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %out, i64 0
  store i32 42, ptr addrspace(1) %arrayidx, align 4
  ret void
}
//--- overriden.ll
define spir_kernel void @shader_override_visaasm(ptr addrspace(1) align 4 %out) {
entry:
  %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %out, i64 0
  store i32 13, ptr addrspace(1) %arrayidx, align 4
  ret void
}
