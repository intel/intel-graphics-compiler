<!---======================= begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

# Intel&reg; Graphics Compiler for OpenCL&trade;

![GitHub release (latest by date)](https://img.shields.io/github/v/release/intel/intel-graphics-compiler?label=Latest%20release)

## Introduction

The Intel&reg; Graphics Compiler for OpenCL&trade; is an LLVM-based compiler designed for
OpenCL&trade;, targeting Intel Gen graphics hardware architecture.

Please refer to [OpenCl&trade; applications](https://www.intel.com/content/www/us/en/developer/articles/guide/sdk-for-opencl-2017-gsg.html) 
for additional details regarding Intel's motivation with OpenCL support in the [open-source community]( http://01.org/compute-runtime).


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

IGC is supported on the following 64-bit Linux operating systems:

* Ubuntu 18.04
* Ubuntu 19.04
* Ubuntu 20.04

## Building

* [Ubuntu](https://github.com/intel/intel-graphics-compiler/blob/master/documentation/build_ubuntu.md)

## Configuration flags

* [Configuration flags for Linux Release](https://github.com/intel/intel-graphics-compiler/blob/master/documentation/configuration_flags.md)

## Supported Platforms

* Intel Core Processors supporting Gen8 graphics devices
* Intel Core Processors supporting Gen9 graphics devices
* Intel Core Processors supporting Gen11 graphics devices
* Intel Core Processors supporting Gen12 graphics devices
* Intel Atom Processors supporting Gen9 graphics devices

## How to provide feedback
If you have any feedback or questions, please open an issue through the native github.com interface: [Submit an issue](https://github.com/intel/intel-graphics-compiler/issues).

## How to contribute

Create a pull request on github.com with your changes. Ensure that your modifications build without errors. 
A maintainer will get in touch with you if there are any inquiries or concerns.
