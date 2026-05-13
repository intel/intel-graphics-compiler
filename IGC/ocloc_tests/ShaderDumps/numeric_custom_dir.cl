// UNSUPPORTED: system-windows
// REQUIRES: regkeys, pvc-supported

// RUN: rm -rf 1234

// RUN: IGC_ShaderDumpEnable=1 IGC_DumpToCustomDir=1234 ocloc compile -device pvc -file %s
// RUN: ls 1234 | FileCheck %s

// CHECK-NOT: No such file or directory

__kernel void foo(__global int *Out, int Val) {
    int idx = get_global_id(0);
    Out[idx] = Val+1;
}
