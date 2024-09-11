<!---======================= begin_copyright_notice ============================

Copyright (C) 2019-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

# Intel&reg; Graphics Compiler for OpenCL&trade;

![GitHub release (latest by date)](https://img.shields.io/github/v/release/intel/intel-graphics-compiler?label=Latest%20release)

## Introduction

The Intel&reg; Graphics Compiler for OpenCL&trade; is an LLVM-based compiler for
OpenCL&trade; targeting Intel&reg; graphics hardware architecture.

Please visit the compute Intel&reg; Graphics Compute Runtime repository for more information about the Intel&reg; open-source compute stack: https://github.com/intel/compute-runtime


## License

The Intel&reg; Graphics Compute Runtime for OpenCL&trade; is distributed under the MIT License.

For detailed terms, you can access the full License at:

https://opensource.org/licenses/MIT


## Dependencies

* LLVM Project -  https://github.com/llvm/llvm-project
* OpenCL Clang - https://github.com/intel/opencl-clang
* SPIRV-LLVM Translator - https://github.com/KhronosGroup/SPIRV-LLVM-Translator
* VC Intrinsics - https://github.com/intel/vc-intrinsics


## Supported Linux versions

IGC is continuously built and tested on the following 64-bit Linux operating systems:

* Ubuntu 24.04
* Ubuntu 22.04


## Documentation

* [Building IGC on Ubuntu](./documentation/build_ubuntu.md)
* [IGC configuration flags](./documentation/configuration_flags.md)
* [Debugging with shader dumps](./documentation/shader_dumps_instruction.md)

More documentation is available in the [documentation](./documentation) directory.


## Supported Platforms

* Intel&reg; Xe2
* Intel&reg; Xe
* Intel&reg; Gen12 graphics
* Intel&reg; Gen11 graphics
* Intel&reg; Gen9 graphics


## How to provide feedback

If you have any feedback or questions, please open an issue through the native github.com interface: https://github.com/intel/intel-graphics-compiler/issues.


## How to contribute

Create a pull request on github.com with your changes. Ensure that your modifications build without errors.
A maintainer will get in touch with you if there are any inquiries or concerns.
