<!---======================= begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

# Intel&reg; Graphics Compiler for OpenCL&trade;

![GitHub release (latest by date)](https://img.shields.io/github/v/release/intel/intel-graphics-compiler?label=Latest%20release)

## Introduction

The Intel&reg; Graphics Compiler for OpenCL&trade; is an LLVM based compiler for
OpenCL&trade; targeting Intel Gen graphics hardware architecture.

Please refer to http://01.org/compute-runtime for additional details regarding
 Intel's motivation and intentions wrt OpenCL support in the open source.


## License

The Intel&reg; Graphics Compute Runtime for OpenCL&trade; is distributed under the MIT License.

You may obtain a copy of the License at:

https://opensource.org/licenses/MIT

## Dependencies

* LLVM Project -  https://github.com/llvm/llvm-project
* OpenCL Clang - https://github.com/intel/opencl-clang
* SPIRV-LLVM Translator - https://github.com/KhronosGroup/SPIRV-LLVM-Translator
* VC Intrinsics - https://github.com/intel/vc-intrinsics

## Supported Linux versions

IGC is continuously built and tested on the following 64 bit Linux operating systems:

* Ubuntu 20.04
* Ubuntu 22.04

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
Please submit an issue using native github.com interface: https://github.com/intel/intel-graphics-compiler/issues.

## How to contribute

Create a pull request on github.com with your patch. Make sure your change is
cleanly building. A maintainer will contact you if there are questions or concerns.
