<!---======================= begin_copyright_notice ============================

Copyright (c) 2019-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ==========================-->

# Intel&reg; Graphics Compiler for OpenCL&trade;

![GitHub release (latest by date)](https://img.shields.io/github/v/release/intel/intel-graphics-compiler?label=Latest%20release)
![Travis (.com) branch](https://img.shields.io/travis/com/intel/intel-graphics-compiler/master?label=Ubuntu%20build)

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

IGC is supported on the following 64 bit Linux operating systems:

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
Please submit an issue using native github.com interface: https://github.com/intel/intel-graphics-compiler/issues.

## How to contribute

Create a pull request on github.com with your patch. Make sure your change is
cleanly building. A maintainer will contact you if there are questions or concerns.
