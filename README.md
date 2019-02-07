# Intel(R) Graphics Compiler for OpenCL(TM)

## Introduction

The Intel(R) Graphics Compiler for OpenCL(TM) is an llvm based compiler for
OpenCL(TM) targeting Intel Gen graphics hardware architecture.

Please refer to http://01.org/compute-runtime for additional details regarding
 Intel's motivation and intentions wrt OpenCL support in the open source.


## License

The Intel(R) Graphics Compute Runtime for OpenCL(TM) is distributed under the MIT.

You may obtain a copy of the License at:

https://opensource.org/licenses/MIT

## Dependencies

* Common Clang - https://github.com/intel/opencl-clang
* Clang Source - https://github.com/llvm-mirror/clang
* Khronos OpenCL Headers - https://github.com/KhronosGroup/OpenCL-Headers
* LLVM Source -  https://github.com/llvm-mirror/llvm

## Supported Linux versions

IGC is supported on the following 32/64 bits Linux operating systems:

* Ubuntu 14.04, 16.04, 17.04, 18.04

## Building

1. Install prerequisites

Building IGC needs flex, bison, cmake version later than 3.4.3 and
 libz.  You can install required packages on ubuntu 18.04 like below:
```
$ sudo apt-get install flex bison libz-dev cmake
```

2. Download all dependencies and create workspace folder as below:
```
<workspace>
      |- igc                          https://github.com/intel/intel-graphics-compiler
      |- llvm_patches                 https://github.com/intel/llvm-patches
      |- llvm_source                  https://github.com/llvm-mirror/llvm
            |- projects/opencl-clang  https://github.com/intel/opencl-clang
            |- projects/llvm-spirv    https://github.com/KhronosGroup/SPIRV-LLVM-Translator
            |- tools/clang            https://github.com/llvm-mirror/clang
```

This can be done using the following commands:

```
$ cd <workspace>
$ git clone -b release_70 https://github.com/llvm-mirror/llvm llvm_source
$ git clone -b release_70 https://github.com/llvm-mirror/clang llvm_source/tools/clang
$ git clone -b ocl-open-70 https://github.com/intel/opencl-clang llvm_source/projects/opencl-clang
$ git clone -b llvm_release_70 https://github.com/KhronosGroup/SPIRV-LLVM-Translator llvm_source/projects/llvm-spirv
$ git clone https://github.com/intel/llvm-patches llvm_patches
$ git clone https://github.com/intel/intel-graphics-compiler igc
  [If using specific release]
$ cd igc && git checkout -b tag igc_release_2019-01-15
```


3. Under workspace create a build folder.  For example:
```
$ cd <workspace>
$ mkdir build
```

4. Build IGC using commands:
```
$ cd build
$ cmake -DIGC_OPTION__OUTPUT_DIR=../igc-install/Release ../igc/IGC
$ make -j`nproc`
```

5. Install IGC
```
$ sudo make install
```

## Supported Platforms

* Intel Core Processors supporting Gen8 graphics devices
* Intel Core Processors supporting Gen9 graphics devices
* Intel Core Processors supporting Gen10 graphics devices
* Intel Atom Processors supporting Gen9 graphics devices

## How to provide feedback
Please submit an issue using native github.com interface: https://github.com/intel/intel-graphics-compiler/issues.

## How to contribute

Create a pull request on github.com with your patch. Make sure your change is
cleanly building. A maintainer will contact you if there are questions or concerns.

