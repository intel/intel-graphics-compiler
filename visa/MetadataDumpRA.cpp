/*========================== begin_copyright_notice ============================

Copyright (C) 2023-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// MetadataDumpRA is a structure that stores RA physical assignment information
// (per def/use per instruction per kernel), and allows for emission of this
// metadata via binary encoding to a file.

#include "MetadataDumpRA.h"

using namespace vISA;

Def::Def(G4_DstRegRegion* dst) {
    G4_VarBase* dstBase = dst->getBase();
    G4_RegVar* dstRegVar = dstBase->asRegVar();
    G4_Declare* dstDecl = dstRegVar->getDeclare();

    // handle the case where this VV is aliased to another VV
    unsigned int rootOffset = 0;
    G4_Declare* rootDecl = dstDecl->getRootDeclare(rootOffset);
    G4_RegVar* rootRegVar = rootDecl->getRegVar();

    regFileKind = rootDecl->getRegFile();
    reg = rootRegVar->getPhyReg()->asGreg()->getRegNum();
    subreg = rootRegVar->getPhyRegOff();
    byteSize = rootDecl->getByteSize();
    aliasOffset = rootOffset;

    rowOffset = dst->getRegOff();
    colOffset = dst->getSubRegOff();

    hstride = dst->getHorzStride();
    typeSize = dst->getTypeSize();

    unsigned int offsetFromR0 = rootDecl->getGRFOffsetFromR0();
    rootBound = offsetFromR0 + rootOffset;
    leftBound = dst->getLeftBound();
    rightBound = dst->getRightBound();

    name = rootRegVar->getName();
    nameLen = strnlen(this->name, MAX_STRLEN);
}

Use::Use(G4_SrcRegRegion* src) {
    G4_VarBase* srcBase = src->getBase();
    G4_RegVar* srcRegVar = srcBase->asRegVar();
    G4_Declare* srcDecl = srcRegVar->getDeclare();

    // handle the case where this VV is aliased to another VV
    unsigned int rootOffset = 0;
    G4_Declare* rootDecl = srcDecl->getRootDeclare(rootOffset);
    G4_RegVar* rootRegVar = rootDecl->getRegVar();

    regFileKind = rootDecl->getRegFile();
    reg = rootRegVar->getPhyReg()->asGreg()->getRegNum();
    subreg = rootRegVar->getPhyRegOff();
    byteSize = rootDecl->getByteSize();
    aliasOffset = rootOffset;

    rowOffset = src->getRegOff();
    colOffset = src->getSubRegOff();

    const RegionDesc* srcRegion = src->getRegion();
    hstride = srcRegion->horzStride;
    vstride = srcRegion->vertStride;
    width = srcRegion->width;
    typeSize = src->getTypeSize();

    unsigned int offsetFromR0 = rootDecl->getGRFOffsetFromR0();
    rootBound = offsetFromR0 + rootOffset;
    leftBound = src->getLeftBound();
    rightBound = src->getRightBound();

    name = rootRegVar->getName();
    nameLen = strnlen(this->name, MAX_STRLEN);
}

void MetadataDumpRA::addKernelMD(G4_Kernel* kernel) {
    KernelMetadata kernelMD;
    for (auto bb : kernel->fg) {
        for (auto inst : *bb) {

            InstMetadata instMD;
            instMD.execSize = (unsigned int)inst->getExecSize().value;
            instMD.binaryOffset = inst->getGenOffset();

            // construct Def object
            G4_DstRegRegion* dst = inst->getDst();
            if (dst && !dst->isNullReg() && dst->isGreg()) {
                Def def(dst);

                // add Def object to instruction metadata
                instMD.instDefs.push_back(def);
                instMD.numDefs += 1;
            }

            // construct Use objects
            int numSrcs = inst->getNumSrc();
            for (unsigned int i = 0; i != numSrcs; i++) {
                auto eachSrc = inst->getSrc(i);
                if (eachSrc && eachSrc->isSrcRegRegion() && eachSrc->isGreg()) {
                    G4_SrcRegRegion* src = eachSrc->asSrcRegRegion();
                    Use use(src);

                    // add Use object to instruction metadata
                    instMD.instUses.push_back(use);
                    instMD.numUses += 1;
                }
            }

            // add this instruction metadata to the kernel's metadata
            kernelMD.instMetadatas.push_back(instMD);
            kernelMD.numInsts += 1;

        }
    }

    // add this kernel metadata to the total metadata
    kernelMetadatas.push_back(kernelMD);
    numKernels += 1;
    return;

}

// Emits a metadata file to the dump directory, formatted as follows:
// |-numKernels            [4 B]                              (number of kernels)
//
// |---numInsts            [4 B]       (number of instructions in 1st kernel, K0)
//
// |-----execSize          [4 B]   (execution size of 1st instruction, I0, in K0)
// |-----binaryOffset      [4 B]                      (binary offset of I0 in K0)
// |-----numDefs           [4 B]                     (number of Defs in I0 in K0)
//
// |-------typeSize        [2 B]          (type size of 1st Def, D0, in I0 in K0)
// |-------leftBound       [4 B]                               (left bound of D0)
// |-------rightBound      [4 B]                              (right bound of D0)
// |-------reg             [4 B]                          (register number of D0)
// |-------subreg          [4 B]                       (subregister number of D0)
// |-------hstride         [4 B]                        (horizontal stride of D0)
// |-------nameLen         [4 B]             (virtual variable name length of D0)
// |-------name            [nameLen B]              (virtual variable name of D0)
//
// |-------typeSize        [2 B]          (type size of 2nd Def, D1, in I0 in K0)
// |-------leftBound       [4 B]                               (left bound of D1)
// |-------   ...           ...                                               ...
//                          <repeats numDefs times, for each Def of I0>
// |-------   ...           ...                                               ...
//
// |-----numUses           [4 B]                     (number of Uses in I0 in K0)
//
// |-------typeSize        [2 B]          (type size of 1st Use, U0, in I0 in K0)
// |-------leftBound       [4 B]                               (left bound of U0)
// |-------rightBound      [4 B]                              (right bound of U0)
// |-------reg             [4 B]                          (register number of U0)
// |-------subreg          [4 B]                       (subregister number of U0)
// |-------hstride         [4 B]                        (horizontal stride of U0)
// |-------vstride         [4 B]                          (vertical stride of U0)
// |-------width           [4 B]                                    (width of U0)
// |-------nameLen         [4 B]             (virtual variable name length of U0)
// |-------name            [nameLen B]              (virtual variable name of U0)
//
// |-------typeSize        [2 B]          (type size of 2nd Use, U1, in I0 in K0)
// |-------leftBound       [4 B]                               (left bound of U1)
// |-------   ...           ...                                               ...
//                          <repeats numUses times, for each Use of I0>
// |-------   ...           ...                                               ...
//
// |-----execSize          [4 B]   (execution size of 2nd instruction, I1, in K0)
// |-----binaryOffset      [4 B]                      (binary offset of I1 in K0)
// |-----    ...            ...                                               ...
//                          <repeats numInsts times, for each Instruction of K0>
// |-----    ...            ...                                               ...
//
// |---numInsts            [4 B]       (number of instructions in 2nd kernel, K1)
// |---   ...               ...                                               ...
//                          <repeats numKernels times, for each Kernel>
// |---   ...               ...                                               ...
void MetadataDumpRA::emitMetadataFile(std::string asmName, std::string kernelName) {
    using namespace std;

    // create metadata file in shader dump
    ofstream MDFile;
    stringstream ssInit;
    ssInit << asmName << "_" << kernelName << ".ra_metadata";
    MDFile.open(ssInit.str(), std::ofstream::binary);

    // write number of kernels to file
    assert(numKernels == kernelMetadatas.size());
    MDFile.write((char*)&numKernels, sizeof(unsigned int));

    for (KernelMetadata kMD : kernelMetadatas) {

        // write number of insts in this kernel to file
        assert(kMD.numInsts == kMD.instMetadatas.size());
        MDFile.write((char*)&kMD.numInsts, sizeof(unsigned int));

        for (InstMetadata iMD : kMD.instMetadatas) {

            // write instruction execution size and binary offset to file
            MDFile.write((char*)&iMD.execSize, sizeof(unsigned int));
            MDFile.write((char*)&iMD.binaryOffset, sizeof(unsigned int));

            // write number of definitions in this inst to file
            assert(iMD.numDefs == iMD.instDefs.size());
            MDFile.write((char*)&iMD.numDefs, sizeof(unsigned int));

            for (Def def : iMD.instDefs) {

                // uchar (1) : regFileKind
                // ushort (1) : typeSize
                // uint (10) : reg, subreg, byteSize, aliasOffset, row/colOffset, hstride, root/l/rBound, nameLen
                streamsize defOffset = sizeof(unsigned char) + sizeof(unsigned short) + sizeof(unsigned int) * 11;
                MDFile.write((char*)&def, defOffset);

                // write virtual variable name to file
                MDFile.write(def.name, def.nameLen);
                vISA_ASSERT(MDFile.good(), "RA metadata file write stream error for Def");
            }

            // write number of uses in this inst to file
            assert(iMD.numUses == iMD.instUses.size());
            MDFile.write((char*)&iMD.numUses, sizeof(unsigned int));

            for (Use use : iMD.instUses) {

                // uchar (1) : regFileKind
                // ushort (1) : typeSize
                // uint (12) : reg, subreg, byteSize, aliasOffset, row/colOffset, h/vstride, width, root/l/rBound, nameLen
                streamsize useOffset = sizeof(unsigned char) + sizeof(unsigned short) + sizeof(unsigned int) * 13;
                MDFile.write((char*)&use, useOffset);

                // write virtual variable name to file
                MDFile.write(use.name, use.nameLen);
                vISA_ASSERT(MDFile.good(), "RA metadata file write stream error for Use");
            }

            MDFile.flush();
        }
    }
    MDFile.close();
    return;
}

void MetadataReader::printName(const char* name, unsigned int nameLen) {
    for (unsigned int i = 0; i < nameLen; i++) {
        printf("%c", name[i]);
    }
}

// Reads a metadata file from the dump directory
void MetadataReader::readMetadata(std::string fileName) {
    ifstream MDFile;

    MDFile.open(fileName, ios::binary);

    unsigned int numKernels;
    MDFile.read((char*)&numKernels, sizeof(unsigned int));
    printf("\nMETADATA READ START\n\n");
    printf("(numKernels = %u)\n", numKernels);

    unsigned int numInsts;
    for (unsigned int k = 0; k < numKernels; k++) {

        MDFile.read((char*)&numInsts, sizeof(unsigned int));
        printf("\nKERNEL %d (numInsts = %u) :\n", k, numInsts);

        unsigned int numDefs;
        unsigned int numUses;
        unsigned int binaryOffset;
        unsigned int execSize;
        for (unsigned int i = 0; i < numInsts; i++) {

            MDFile.read((char*)&execSize, sizeof(unsigned int));
            MDFile.read((char*)&binaryOffset, sizeof(unsigned int));
            printf("|\n|\n|    INST %d (execSize = %u) (binaryOffset = %u) :\n", i, execSize, binaryOffset);

            MDFile.read((char*)&numDefs, sizeof(unsigned int));
            printf("|        (numDefs = %u)\n", numDefs);

            for (unsigned int d = 0; d < numDefs; d++) {
                Def def;

                // uchar (1) : regFileKind
                // ushort (1) : typeSize
                // uint (10) : reg, subreg, byteSize, aliasOffset, row/colOffset, hstride, root/l/rBound, nameLen
                streamsize defOffset = sizeof(unsigned char) + sizeof(unsigned short) + sizeof(unsigned int) * 11;

                MDFile.read((char*)&def, defOffset);

                char name[MAX_NAMELEN];
                MDFile.read(name, def.nameLen);
                def.name = name;

                vISA_ASSERT(MDFile.good(), "RA metadata file read stream error for Def");

                printf("|            ");
                this->printName(def.name, def.nameLen);
                printf(" \t\t|");
                printf("%5u -> %-5u| ", def.leftBound, def.rightBound);
                printf("  r%-5u.%-5u(%-5u,%-5u)<%-5u%-5s%-5s  >:%-5u_%-5u_%-5u", def.reg, def.subreg, def.rowOffset, def.colOffset, def.hstride, "", "", def.typeSize, def.byteSize, (unsigned int)def.regFileKind);
                printf("\n");
            }

            MDFile.read((char*)&numUses, sizeof(unsigned int));
            printf("|        (numUses = %u)\n", numUses);

            for (unsigned int u = 0; u < numUses; u++) {
                Use use;

                // uchar (1) : regFileKind
                // ushort (1) : typeSize
                // uint (12) : reg, subreg, byteSize, aliasOffset, row/colOffset, h/vstride, width, root/l/rBound, nameLen
                streamsize useOffset = sizeof(unsigned char) + sizeof(unsigned short) + sizeof(unsigned int) * 13;

                MDFile.read((char*)&use, useOffset);

                char name[MAX_NAMELEN];
                MDFile.read(name, use.nameLen);
                use.name = name;

                vISA_ASSERT(MDFile.good(), "RA metadata file read stream error for Use");

                printf("|            ");
                this->printName(use.name, use.nameLen);
                printf(" \t\t|");
                printf("%5u -> %-5u| ", use.leftBound, use.rightBound);
                printf("  r%-5u.%-5u(%-5u,%-5u)<%-5u;%-5u,%-5u>:%-5u_%-5u_%-5u", use.reg, use.subreg, use.rowOffset, use.colOffset, use.vstride, use.width, use.hstride, use.typeSize, use.byteSize, (unsigned int)use.regFileKind);
                printf("\n");
            }
        }
    }
    MDFile.close();
    printf("\n\nMETADATA READ COMPLETE\n\n");
    return;
};