<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

# ZE Binary Utilitis

### Prerequisite
Visaul Studio version >= 2019 <br>
CMake version >= 3.8

### Build
Export *LLVM_DIR* to directory path which contains _LLVMConfig.cmake_

    export LLVM_DIR=${PATH_to_LLVMConfig.cmake}

Create a build directory _${BUILD}_ and run

    cd ${BUILD}
    cmake ${ZEBinaryUtility_SOURCE}
    cmake --build .

### Build on Visual Studio
In windows command prompt:

Set LLVM_DIR to directory path contains *LLVMConfig.cmake*

    set LLVM_DIR=${PATH_to_LLVMConfig.cmake}

Create VS project files:

    cmake -G "Visual Studio 16 2019" ${ZEBinaryUtility_SOURCE}

Open the VS solution ZEBinaryUtilities.sln

The **C++ CMake tools for Windows** package is required.
Refer to [CMake projects in Visual Studio](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=vs-2019)
for the istallation instructions.

### Usage
**ZEInfoReader.exe** [options]  <_input file_>
  * -info      :Dump .ze_info section into ze_info.dump file
