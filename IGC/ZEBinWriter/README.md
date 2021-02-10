<!---======================= begin_copyright_notice ============================

Copyright (c) 2020-2021 Intel Corporation

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
