/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "ZEInfoReader.h"

#include "Tester.hpp"
#include <ZEInfo.hpp>
#include <ZEinfoYAML.hpp>

#include <llvm/Object/ObjectFile.h>
#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Error.h>

#include <fstream>
#include <iostream>
#include <string>
#include <system_error>

using namespace std;
using namespace zebin;

/// ---------------- ELF Object Reader ------------------------------------ ///

static void dumpZEInfo(std::unique_ptr<llvm::object::ObjectFile> object) {
    bool dump = false;
    for (auto sect : object->sections()) {
        llvm::StringRef name;
        sect.getName(name);

        if (name.compare(llvm::StringRef(".ze_info")))
            continue;

        llvm::StringRef content;
        sect.getContents();

        std::ofstream outfile;
        outfile.open("ze_info.dump", std::ios::out | std::ios::binary);
        outfile.write(content.data(), content.size());
        outfile.close();
        if (dump)
            std::cerr << "Given ELF object has more than one .ze_info section";
        dump = true;
    }
    if (!dump)
        std::cerr << "Given ELF object has no .ze_info section";
}


/// ---------------- Command line options --------------------------------- ///
static llvm::cl::opt<string> InputFilename(
    llvm::cl::Positional, llvm::cl::desc("<input file>"));

static llvm::cl::opt<bool> DumpZEInfo ("info",
    llvm::cl::desc("Dump .ze_info section into ze_info.dump file"));

static llvm::cl::opt<bool> RunTestZEInfo ("test-ze-info",
    llvm::cl::desc("Run static zeinfo generating tests, print the result to std output"));
/// ----------------------------------------------------------------------- ///

int zeinfo_reader_main(int argc, const char** argv) {
    llvm::cl::ParseCommandLineOptions(argc, argv);

    // run zeinfo generating tests
    // FIXME: This is just a static test, need to be enhanced
    if (RunTestZEInfo) {
        Tester::testZEInfoOutput();
        return 0;
    }

    // read input elf file
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> FileOrErr =
        llvm::MemoryBuffer::getFile(InputFilename);

    if (FileOrErr.getError()) {
        std::cerr << "Cannot open file " << InputFilename;
        return 1;
    }

    llvm::MemoryBufferRef inputRef(*FileOrErr.get());

    llvm::Expected<std::unique_ptr<llvm::object::ObjectFile>>
        ObjOrErr(llvm::object::ObjectFile::createELFObjectFile(inputRef));

    if (!ObjOrErr) {
        std::cerr << "Unrecognized input format. ELF is expected.";
        return 1;
    }

    std::unique_ptr<llvm::object::ObjectFile> obj = std::move(ObjOrErr.get());

    if (DumpZEInfo)
        dumpZEInfo(std::move(obj));

    return 0;
}