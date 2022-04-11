<!---======================= begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

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
- **allowLICM** - Setting this to false to disable LLVM LICM in IGC in order to control register pressure
- **AllowMem2Reg** - Setting this to true makes IGC run mem2reg even when optimizations are disabled
- **ControlInlineImplicitArgs** - Avoid trimming functions with implicit args
- **ControlInlineTinySize** - Tiny function size for controlling kernel total size
- **ControlKernelTotalSize** - Control kernel total size
- **DisableAddingAlwaysAttribute** - Disable adding always attribute
- **disableCompaction** - Disables compaction
- **DisableStaticCheck** - Disable static check to push or fold constants
- **DumpCompilerStats** - Dump compiler statistics
- **DumpDeSSA** - Dump DeSSA info into file
- **DumpHasNonKernelArgLdSt** - Print if hasNonKernelArg load/store to stderr
- **DumpLLVMIR** - Dump LLVM IR
- **DumpOCLProgramInfo** - Dump OpenCL Patch Tokens, Kernel/Program Binary Header
- **DumpPatchTokens** - Enable dumping of patch tokens
- **DumpTimeStats** - Timing of translation, code generation, finalizer, etc
- **DumpTimeStatsCoarse** - Only collect/dump coarse level time stats, i.e. skip opt detail timer for now
- **DumpTimeStatsPerPass** - Collect Timing of IGC/LLVM passes
- **DumpToCurrentDir** - Dump shaders to the current directory
- **DumpToCustomDir** - Dump shaders to custom directory. Parent directory must exist
- **DumpVariableAlias** - Dump variable alias info, valid if EnableVariableAlias is on
- **EnableBCR** - Enable bank conflict reduction
- **EnableCapsDump** - Enable hardware caps dump
- **EnableCisDump** - Enable cis dump
- **EnableConstIntDivReduction** - Enables strength reduction on integer division/remainder with constant divisors/moduli
- **EnableCosDump** - Enable cos dump
- **EnableDPEmulation** - Enforce double precision floating point operations emulation on platforms that do not support it natively
- **EnableForceDebugSWSB** - Enable force debugging functionality for software scoreboard generation
- **EnableIGASWSB** - Use IGA for SWSB
- **EnableIntDivRemCombine** - Given div/rem pairs with same operands merged; replace rem with mul+sub on quotient; 0x3 (set bit[1]) forces this on constant power of two divisors as well
- **EnableGroupScheduleForBC** - Enable bank conflict reduction in scheduling
- **EnableGTLocationDebugging** - Enables GT location expression emissions for GPU debugger
- **UseOffsetInLocation** - Using offset in location limits variables available for a debugger all time
- **ElfDumpEnable** - Dump ELF
- **ElfTempDumpEnable** - Dump temporary ELF files
- **ZeBinCompatibleDebugging** - Enables embed debug info in zeBinary
- **EnableLivenessDump** - Enable dumping out liveness info on stderr
- **EnableLTODebug** - Enable debug information for LTO
- **EnableMaxWGSizeCalculation** - Enable max work group size calculation [OCL only]
- **EnableNoDD** - Enable NoDD flags
- **EnableOCLSIMD16** - Enable OCL SIMD16 mode
- **EnableOCLSIMD32** - Enable OCL SIMD32 mode
- **EnableOptionalBufferOffset** - For StatelessToStatefull optimization [OCL] make buffer offset optional
- **EnableQuickTokenAlloc** - Insert dependence resolve for kernel stitching
- **EnableRelocations** - Setting this to 1 (true) makes IGC emit relocatable ELF with debug info
- **EnableRuntimeFuncAttributePatching** - Creates a relocation entry to let runtime calculate the max call depth and patch required scratch space usage
- **EnableScalarizerDebugLog** - Print step by step scalarizer debug info
- **EnableShaderNumbering** - Number shaders in the order they are dumped based on their hashes
- **EnableSIMDLaneDebugging** - Enables SIMD lane location expression emissions for GPU debugger
- **EnableSWSBInstStall** - Enable force stall to specific(start) instruction start for software scoreboard generation
- **EnableSWSBInstStallEnd** - Enable force stall to end instruction for software scoreboard generation
- **EnableSWSBStitch** - Insert dependence resolve for kernel stitching
- **EnableSWSBTokenBarrier** - Enable force specific instruction as a barrier for software scoreboard generation
- **EnableTrigFuncRangeReduction** - Reduce the sin and cosing function domain range
- **EnableUnmaskedFunctions** - Enable unmaksed functions SYCL feature
- **EnableVISABinary** - Enable VISA Binary
- **EnableVISADumpCommonISA** - Enable VISA Dump Common ISA
- **EnableVISAOutput** - Enable VISA GenISA output
- **EnableVISASlowpath** - Enable VISA Slowpath. Needed to dump .visaasm
- **EnableZEBinary** - Enable output in ZE binary format
- **ExtraOCLOptions** - Extra options for OpenCL
- **FunctionControl** - Control function inlining/subroutine/stackcall
- **ForceInlineStackCallWithImplArg** - If enabled, stack calls that use implicit args will be force inlined.
- **ForceOCLSIMDWidth** - Force using specified SIMD width. Possible values: 0 (off), 8, 16, 32.
- **ForceRPE** - Force RPE (RegisterEstimator) computation if > 0. If 2, force RPE per inst
- **InterleaveSourceShader** - Interleave the source shader in asm dump
- **KernelTotalSizeThreshold** - Trimming target of kernel total size
- **OGLMinimumDump** - Minimum dump for testing - first and last .ll, .cos and compiler output
- **OverrideOCLMaxParamSize** - Override the value imposed on the kernel by CL_DEVICE_MAX_PARAMETER_SIZE. Value in bytes, if value==0 no override happens
- **PrintControlKernelTotalSize** - Print Control kernel total size
- **PrintToConsole** - Dump to console
- **QualityMetricsEnable** - Enable Quality Metrics for IGC
- **ReplaceIndirectCallWithJmpi** - Replace indirect call with jmpi instruction
- **SetA0toTdrForSendc** - Set A0 to tdr0 before each sendc/sendsc
- **ShaderDumpEnable** - Dump LLVM IR, visaasm, and GenISA
- **ShaderDumpEnableAll** - Dump all LLVM IR passes, visaasm, and GenISA
- **ShaderDumpPidDisable** - Disabled adding PID to the name of shader dump directory
- **SWSBTokenNum** - Total tokens used for SWSB
- **UseNewRegEncoding** - Use new location encoding for register numbers in dwarf
- **VCApiOptions** - Extra API options for VC
- **VCInternalOptions** - Extra Internal options to pass to VC
- **VCOptimizeNone** - Same as -optimize=none in vector compiler options
- **VCStrictOptionParser** - Produce error on unknown API options in vector compiler
- **VISAOptions** - Options to vISA. Space-separated options
