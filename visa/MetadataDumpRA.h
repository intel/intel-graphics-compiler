/*========================== begin_copyright_notice ============================

Copyright (C) 2023-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// MetadataDumpRA is a structure that stores RA physical assignment information
// (per def/use per instruction per kernel), and allows for emission of this
// metadata via binary encoding to a file.

#include "G4_Kernel.hpp"
#include "FlowGraph.h"
#include "G4_IR.hpp"
#include "G4_BB.hpp"

#include <iostream>
#include <cstring>
#include <cstdint>
#include <vector>
#include <fstream>
#include <utility>
#include <string>

// global var for strnlen call
static constexpr size_t MAX_STRLEN = 100;

// global var for VV name length
static constexpr unsigned int MAX_NAMELEN = 1024;

using namespace std;

namespace vISA {

class Def {
public:

    unsigned char regFileKind;     // G4_RegFileKind of this operand's G4_Declare
    unsigned short typeSize = 0;   // size of each element
    unsigned int reg = 0;          // register number of the start of this VV's register allocation
    unsigned int subreg = 0;       // subregister number of the start of this VV's register allocation
    unsigned int byteSize = 0;     // total number of bytes that this VV is allocated in register file
    unsigned int aliasOffset = 0;  // number of bytes that this VV's allocation is offset from its root VV's

    unsigned int rowOffset = 0;    // number of registers that this region is offset from the base by
    unsigned int colOffset = 0;    // number of subregisters/elements that this region is offset from the base by

    unsigned int hstride = 0;      // step size (in units of 1 element size) btwn the starts of two elems in a row

    unsigned int rootBound = 0;    // byte offset into the register file (VV start)
    unsigned int leftBound = 0;    // byte offset into the register file (VV start + operand row/col offset)
    unsigned int rightBound = 0;   // byte offset into the register file (VV start + operand total size)

    unsigned int nameLen = 0;      // number of characters in this VV's name
    const char* name = nullptr;    // note that for aliased virtual variables, this is the name of the root VV

    Def() = default;

    Def(G4_DstRegRegion* dst);
};

class Use {
public:

    unsigned char regFileKind;     // G4_RegFileKind of this operand's G4_Declare
    unsigned short typeSize = 0;   // size of each element
    unsigned int reg = 0;          // register number of the start of this VV's register allocation
    unsigned int subreg = 0;       // subregister number of the start of this VV's register allocation
    unsigned int byteSize = 0;     // total number of bytes that this VV is allocated in register file
    unsigned int aliasOffset = 0;  // number of bytes that this VV's allocation is offset from its root VV's

    unsigned int rowOffset = 0;    // number of registers that this region is offset from the base by
    unsigned int colOffset = 0;    // number of subregisters/elements that this region is offset from the base by

    unsigned int hstride = 0;      // step size (in units of 1 element size) btwn the starts of two elems in a row
    unsigned int vstride = 0;      // step size (in units of 1 element size) btwn the starts of two rows
    unsigned int width = 0;        // number of elements in one row

    unsigned int rootBound = 0;    // byte offset into the register file (VV start)
    unsigned int leftBound = 0;    // byte offset into the register file (VV start + operand row/col offset)
    unsigned int rightBound = 0;   // byte offset into the register file (VV start + operand total size)

    unsigned int nameLen = 0;      // number of characters in this VV's name
    const char* name = nullptr;    // note that for aliased virtual variables, this is the name of the root VV

    Use() = default;

    Use(G4_SrcRegRegion* src);
};

class InstMetadata {
public:

    unsigned int execSize = 0;
    int64_t binaryOffset = -1;
    unsigned int numDefs = 0;
    std::vector<Def> instDefs;
    unsigned int numUses = 0;
    std::vector<Use> instUses;
};

class KernelMetadata {
public:
    unsigned int numInsts = 0;
    std::vector<InstMetadata> instMetadatas;
};

class MetadataDumpRA {
public:
    unsigned int numKernels = 0;
    std::vector<KernelMetadata> kernelMetadatas;

    // Adds the metadata for a kernel to this metadata dump
    void addKernelMD(G4_Kernel* kernel);

    // Emits a metadata file to the dump directory
    void emitMetadataFile(std::string asmName, std::string kernelName);
};

class MetadataReader {
public:
    void printName(const char* name, unsigned int nameLen);
    void readMetadata(std::string fileName);
};

}