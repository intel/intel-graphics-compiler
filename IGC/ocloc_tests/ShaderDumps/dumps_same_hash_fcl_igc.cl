// UNSUPPORTED: system-windows
// REQUIRES: regkeys, cri-supported

// RUN: rm -rf %t.igc_dumps %t.cl_hash %t.asm_hash

// RUN: IGC_ShaderDumpEnable=1 IGC_DumpToCustomDir=%t.igc_dumps ocloc compile -device cri -file %s
// RUN: ls %t.igc_dumps/*.cl | xargs basename | awk -F'[_.]' '{print $2}' > %t.cl_hash
// RUN: ls %t.igc_dumps/*.asm | xargs basename | awk -F'[_.]' '{print $2}' > %t.asm_hash
// RUN: diff -w %t.cl_hash %t.asm_hash
// RUN: rm -rf %t.igc_dumps %t.cl_hash %t.asm_hash

__kernel void foo(__global int *Out, int Val) {
    int idx = get_global_id(0);
    Out[idx] = Val+1;
}
