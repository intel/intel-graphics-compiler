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
using namespace std;

namespace vISA {

class Def {
public:

    unsigned short typeSize = 0;
    unsigned int leftBound = 0;
    unsigned int rightBound = 0;
    unsigned int reg = 0;
    unsigned int subreg = 0;

    unsigned int hstride = 0;

    unsigned int nameLen = 0;
    const char* name = nullptr;

    Def() = default;

    Def(G4_DstRegRegion* dst);
};

class Use {
public:

    unsigned short typeSize = 0;
    unsigned int leftBound = 0;
    unsigned int rightBound = 0;
    unsigned int reg = 0;
    unsigned int subreg = 0;

    unsigned int hstride = 0;
    unsigned int vstride = 0;
    unsigned int width = 0;

    unsigned int nameLen = 0;
    const char* name = nullptr;

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
    void emitMetadataFile();
};

class MetadataReader {
public:
    void printName(const char* name, unsigned int nameLen);
    void readMetadata();
};

}