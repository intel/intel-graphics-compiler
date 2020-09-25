# Intel&reg; Graphics Compiler for OpenCL&trade;

## Configuration flags for Linux Release

### Overview

Linux release build allows enabling user-selected configuration flags. They are available after installing release build according to the instructions [here](https://github.com/intel/intel-graphics-compiler/blob/master/documentation/build_ubuntu.md).

##### Important notice

Configuration flags are generally used either for debug purposes or to experimentally change the compiler's behavior. Intel does not guarantee full performance and conformance when using configuration flags.

### How to enable a flag

A flag is enabled when it is set as a variable in an environment.
The syntax is as follows:

```shell
IGC_<flag>=<value>
```
For example - to enable `ShaderDumpEnable` flag in shell:

```shell
$ export IGC_ShaderDumpEnable=1
```

### List of available flags
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
- **EnableGTLocationDebugging** - Enables GT location expression emissions for GPU debugger
- **EnableLivenessDump** - Enable dumping out liveness info on stderr
- **EnableLTODebug** - Enable debug information for LTO
- **EnableNoDD** - Enable NoDD flags
- **EnableOCLSIMD16** - Enable OCL SIMD16 mode
- **EnableOCLSIMD32** - Enable OCL SIMD32 mode
- **EnableOptionalBufferOffset** - For StatelessToStatefull optimization [OCL] make buffer offset optional
- **EnableScalarizerDebugLog** - Print step by step scalarizer debug info
- **EnableShaderNumbering** - Number shaders in the order they are dumped based on their hashes
- **EnableSIMDLaneDebugging** - Enables SIMD lane location expression emissions for GPU debugger
- **EnableVISABinary** - Enable VISA Binary
- **EnableVISADumpCommonISA** - Enable VISA Dump Common ISA
- **EnableVISAOutput** - Enable VISA GenISA output
- **EnableVISASlowpath** - Enable VISA Slowpath. Needed to dump .visaasm
- **FunctionControl** - Control function inlining/subroutine/stackcall. See value defs in igc_flags.hpp
- **ForceRPE** - Force RPE (RegisterEstimator) computation if > 0. If 2, force RPE per inst
- **ForceOCLSIMDWidth** - Force using specified SIMD width. Possible values: 0 (off), 8, 16, 32.
- **InterleaveSourceShader** - Interleave the source shader in asm dump
- **OGLMinimumDump** - Minimum dump for testing - first and last .ll, .cos and compiler output
- **PrintToConsole** - Dump to console
- **QualityMetricsEnable** - Enable Quality Metrics for IGC
- **ShaderDumpEnable** - Dump LLVM IR, visaasm, and GenISA
- **ShaderDumpEnableAll** - Dump all LLVM IR passes, visaasm, and GenISA
- **ShaderDumpPidDisable** - Disabled adding PID to the name of shader dump directory
- **UseNewRegEncoding** - Use new location encoding for register numbers in dwarf
