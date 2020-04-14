# Intel&reg; Graphics Compiler for OpenCL&trade;

## Configuration flags for Linux Release

### 1. Description

Linux Release build allows enabling user-selected debug flags. Configuration is available after installing Release build according to the instructions [here](https://github.com/intel/intel-graphics-compiler/blob/master/documentation/build_ubuntu.md).

### 2. Instruction to enable the flags

To enable selected flags, you must enter the appropriate flag name in the console according to the following pattern:

```shell
$ export IGC_NameOfFlag=value
```
For example:

```shell
$ export IGC_ShaderDumpEnable=1
```

### 3. Description of flags
- **disableCompaction** - Disables compaction
- **DumpCompilerStats** - Dump compiler statistics
- **DumpDeSSA** - Dump DeSSA info into file
- **DumpLLVMIR** - Dump LLVM IR
- **DumpOCLProgramInfo** - Dump OpenCL Patch Tokens, Kernel/Program Binary Header
- **DumpPatchTokens** - Enable dumping of patch tokens
- **DumpToCurrentDir** - Dump shaders to the current directory
- **DumpToCustomDir** - Dump shaders to custom directory. Parent directory must exist
- **DumpVariableAlias** - Dump variable alias info, valid if EnableVariableAlias is on
- **EnableCapsDump** - Enable hardware caps dump
- **EnableCosDump** - Enable cos dump
- **EnableLivenessDump** - Enable dumping out liveness info on stderr
- **EnableLTODebug** - Enable debug information for LTO
- **EnableNoDD** - Enable NoDD flags
- **EnableOCLSIMD16** - Enable OCL SIMD16 mode
- **EnableOCLSIMD32** - Enable OCL SIMD32 mode
- **EnableOptionalBufferOffset** - For StatelessToStatefull optimization [OCL] make buffer offset optional
- **EnableScalarizerDebugLog** - Print step by step scalarizer debug info
- **EnableShaderNumbering** - Number shaders in the order they are dumped based on their hashes
- **EnableVISABinary** - Enable VISA Binary
- **EnableVISADumpCommonISA** - Enable VISA Dump Common ISA
- **EnableVISAOutput** - Enable VISA GenISA output
- **EnableVISASlowpath** - Enable VISA Slowpath. Needed to dump .visaasm
- **ForceRPE** - Force RPE (RegisterEstimator) computation if > 0. If 2, force RPE per inst
- **InterleaveSourceShader** - Interleave the source shader in asm dump
- **OGLMinimumDump** - Minimum dump for testing - first and last .ll, .cos and compiler output
- **PrintToConsole** - Dump to console
- **ShaderDumpEnable** - Dump LLVM IR, visaasm, and GenISA
- **ShaderDumpEnableAll** - Dump all LLVM IR passes, visaasm, and GenISA
- **ShaderDumpPidDisable** - Disabled adding PID to the name of shader dump directory
- **QualityMetricsEnable** - Enable Quality Metrics for IGC
