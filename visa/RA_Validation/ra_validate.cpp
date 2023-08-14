/*========================== begin_copyright_notice ============================
Copyright (C) 2021-2022 Intel Corporation

SPDX-License-Identifier: MIT
============================= end_copyright_notice ===========================*/

/*******************************************************************************************************
 * RA_VALIDATE tool
 *
 * Follows VISA variables and at uses, checks whether they've been clobbered since definition.
 *
 * NOTE: the tool callbacks might be called from different threads.
 */
#include <stdio.h>
#include <string.h>
#include <vector>
#include <fstream>
#include <utility>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <unordered_map>

#include "gtreplay_assert.h"
#include "gtreplay_client.h"
#include "knob_parser.h"
#include "ged.h"
#include "knob.h"

using namespace std;

class Def {
public:
    unsigned char regFileKind = 0; // G4_RegFileKind of this operand's G4_Declare
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
};

class Use {
public:
    unsigned char regFileKind = 0; // G4_RegFileKind of this operand's G4_Declare
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
};

string help = "Emitting the Shader Dump:\n\t1. Open Registry Editor, navigate to HKEY_LOCAL_MACHINE\\SOFTWARE\\Intel\\IGFX\\IGC\n\t2. Create two new DWORDs, ShaderDumpEnable and ShaderDumpEnableRAMetadata \n\t3. Set both to value 1\nUse the absolute path to the shader dump directory as the argument for the RA_VALIDATE tool";
Knob<string> shaderDumpDir("shaderDumpDir", "", help);
uint32_t gMaxNumOfHwThreads = 0;
uint32_t gMaxNumOfTiles = 0;
uint32_t gRegSize = 0;

// An InstOperand stores the metadata associated with either a destination or source
// operand in an instruction. This includes both G4 information like VV name, (Row, Column)
// offsets, and regioning information, as well as corresponding post-RA information like
// assigned physical register location.
class InstOperand {
public:
    InstOperand() = default;

    inline string        getName() const                       { return _VVName; }
    inline GED_REG_FILE  getRegFile() const                    { return _regFile; }
    inline uint32_t      getRegNum() const                     { return _baseRegNum; }
    inline uint32_t      getSubRegNum() const                  { return _baseRegSubNum; }
    inline uint32_t      getAliasOffset() const                { return _aliasOffset; }
    inline uint32_t      getRootBound() const                  { return _rootBound; }
    inline uint32_t      getLbound() const                     { return _lbound; }
    inline uint32_t      getRbound() const                     { return _rbound; }
    inline uint32_t      getTypeSize() const                   { return _typeSize; }
    inline string        getTypeName() const                   { return _typeName; }
    inline uint32_t      getH() const                          { return _hStride; }
    inline uint32_t      getV() const                          { return _vStride; }
    inline uint32_t      getW() const                          { return _width; }
    inline uint32_t      getExecSize() const                   { return _execSize; }
    inline uint32_t      getExecMask() const                   { return _execMask; }
    inline uint32_t      getTotalByteSize() const              { return _totalByteSize; }
    inline uint32_t      getR() const                          { return _row; }
    inline uint32_t      getC() const                          { return _col; }
    inline uint32_t      getInstOffset() const                 { return _instOffset; }
    inline bool          isDef() const                         { return _isDef; }
    inline bool          isSendOper() const                    { return _isSendOper; }
    inline unsigned char getRegFileKind() const                { return _regFileKind; }
    inline uint32_t      getVVStartOffset() const              { return (_baseRegNum * gRegSize) + (_baseRegSubNum * _typeSize); }

    inline void          setName(string name)                  { _VVName = name; }
    inline void          setRegFile(GED_REG_FILE regFile)      { _regFile = regFile; }
    inline void          setRegNum(uint32_t baseRegNum)        { _baseRegNum = baseRegNum; }
    inline void          setSubRegNum(uint32_t baseRegSubNum)  { _baseRegSubNum = baseRegSubNum; }
    inline void          setAliasOffset(uint32_t aliasOff)     { _aliasOffset = aliasOff; }
    inline void          setRootBound(uint32_t rootBound)      { _rootBound = rootBound; }
    inline void          setLbound(uint32_t lbound)            { _lbound = lbound; }
    inline void          setRbound(uint32_t rbound)            { _rbound = rbound; }
    inline void          setTypeSize(uint32_t elementWidth)    { _typeSize = elementWidth; }
    inline void          setTypeName(string typeName)          { _typeName = typeName; }
    inline void          setH(uint32_t horizontalStride)       { _hStride = horizontalStride; }
    inline void          setV(uint32_t verticalStride)         { _vStride = verticalStride; }
    inline void          setW(uint32_t numOfElements)          { _width = numOfElements; }
    inline void          setExecSize(uint32_t execSize)        { _execSize = execSize; }
    inline void          setExecMask(uint32_t execMask)        { _execMask = execMask; }
    inline void          setTotalByteSize(uint32_t byteSize)   { _totalByteSize = byteSize; }
    inline void          setR(uint32_t row)                    { _row = row; }
    inline void          setC(uint32_t col)                    { _col = col; }
    inline void          setInstOffset(uint32_t offset)        { _instOffset = offset; }
    inline void          setIsDef(bool isDef)                  { _isDef = isDef; }
    inline void          setIsSendOper(bool isSendOper)        { _isSendOper = isSendOper; }
    inline void          setRegFileKind(unsigned char kind)    { _regFileKind = kind; }

private:
    GED_REG_FILE         _regFile = GED_REG_FILE_INVALID;      // register file type (ARF, GRF, IMM)

    string               _VVName;                              // virtual variable name
    uint32_t             _baseRegNum = 0;                      // register number that defines start of physical assignment
    uint32_t             _baseRegSubNum = 0;                   // subregister number of physical assignment
    uint32_t             _aliasOffset = 0;                     // byte offset from the start of the root virtual variable
    uint32_t             _rootBound = 0;                       // byte offset into the register file (VV start)
    uint32_t             _lbound = 0;                          // byte offset into the register file (VV start + operand row/col offset)
    uint32_t             _rbound = 0;                          // byte offset into the register file (VV start + operand total size)
    uint32_t             _typeSize = 0;                        // type of this virtual var, the size of each element
    string               _typeName;                            // name of the data type of this operand
    uint32_t             _hStride = 0;                         // step size (in units of 1 element size) btwn the starts of two elems in a row
    uint32_t             _vStride = 0;                         // step size (in units of 1 element size) btwn the starts of two rows
    uint32_t             _width = 0;                           // number of elements in one row
    uint32_t             _execSize = 0;                        // parallel-compute number of elements (execution size)
    uint32_t             _execMask = 0;                        // execution mask indicating which of the _execSize channels are active
    uint32_t             _totalByteSize = 0;                   // total byte size of this virtual variable in the register file
    uint32_t             _row = 0;                             // row offset into the virtual variable
    uint32_t             _col = 0;                             // column offset into the virtual variable
    uint32_t             _instOffset = 0;                      // offset of the instruction that this operand belongs to
    bool                 _isDef = false;                       // if this is a Def operand, ignore the vStride and width
    bool                 _isSendOper = false;                  // if this is an operand of a send inst, must be GRF-aligned and no regioning
    unsigned char        _regFileKind = 1;                     // G4_RegFileKind of this operand's G4_Declare
};

// A VVData is unique for each Virtual Variable; it stores the unique name of a VV,
// as well as its corresponding bytes of data in the physical register location
class VVData {
public:
    VVData(const InstOperand* iOper,
           uint32_t tileId,
           uint32_t tid) : _iOper(iOper), _tileId(tileId), _tid(tid)
    {
        GTREPLAY_ASSERT(_iOper);

        // the total number of bytes allocated in the register file for this
        // virtual variable's value was already collected in the metadata
        _byteValues.resize(_iOper->getTotalByteSize());

        _lastDefOper = NULL;
    }

    inline const vector<uint8_t>&    getData() const       { return _byteValues; }
    inline const uint8_t*            getDataPtr() const    { return _byteValues.data(); }
    inline uint32_t                  getDataSize() const   { return (uint32_t)_byteValues.size(); }
    inline uint32_t                  getTileId() const     { return _tileId; }
    inline uint32_t                  getTid() const        { return _tid; }
    inline InstOperand*              getLastDefOper()      { return _lastDefOper; }

    // use for setting bytes in this VV's physical data with values in VAL
    void setValue(InstOperand* iOper, vector<uint8_t> val, bool isInitialize) {

        // sanity check that we are not defining more bytes than there exist in this VV
        // FIXME: removing this assert for the case of ALL region (h/vStride, width) == 0,
        // since in this case the val has been created with execSize * typeSize bytes, but
        // only actually needs to store typeSize bytes (the other execSize-1 elements are
        // all the exact same typeSize bytes).
        // GTREPLAY_ASSERT(val.size() <= getDataSize());

        uint32_t     typeSize = iOper->getTypeSize();
        uint32_t     hStride = iOper->getH();
        uint32_t     vStride = iOper->getV();
        uint32_t     width = iOper->getW();
        uint32_t     execSize = iOper->getExecSize();
        uint32_t     execMask = iOper->getExecMask();
        uint32_t     totalByteSize = iOper->getTotalByteSize();
        uint32_t     lbound = iOper->getLbound();

        // save the InstOperand that is setting this VVData as the most recent definition
        _lastDefOper = iOper;

        // if this is a Def operand, there is no width or vStride region information; we can
        // simulate this behavior by setting the width equal to the execSize. This will remove
        // the effect of the vStride, since there will only be one row containing ALL the elems
        if (iOper->isDef()) {
            width = execSize;
        }

        // if this is a send instruction operand, the operand must be GRF-aligned (meaning there
        // must be NO subreg offset) and NO regioning information (meaning we iterate through every
        // element with a hstride of 1)
        // see NOTE in ObtainVisaVarValueFromState() for explanation on totalByteSize / typeSize
        // see NOTE in ObtainVisaVarValueFromState() for explanation on isInitialize
        if (iOper->isSendOper() || isInitialize) {
            //colOffset = 0;
            lbound = 0;

            hStride = 1;
            execSize = totalByteSize / typeSize;
            width = execSize;
        }

        // keep track of which element position we are at within a row
        uint32_t rowIdx = 0;

        //replaced by l/rbound implementation
        //uint32_t regOffset = rowOffset * gRegSize;
        //uint32_t subregOffset = colOffset * typeSize;
        //uint32_t currOffset = regOffset + subregOffset;

        // keeps track of the byte offset into the reg file that we are defining an element's byte's value at
        uint32_t currOffset = lbound;

        // keeps track of the byte offset of the start of the current row
        uint32_t currRowStart = currOffset;

        // keeps track of the byte offset of the start of the current element
        uint32_t currElemStart = currOffset;

        // keeps track of which byte of the input VAL we are copying into this VVData
        uint32_t valIdx = 0;

        // iterate through all the elements we need to define
        for (unsigned int elemIdx = 0; elemIdx < execSize; elemIdx++) {

            // if we have reached the end of this row, move to the next row
            if (rowIdx == width) {
                currOffset = currRowStart + vStride * typeSize;
                currRowStart = currOffset;
                rowIdx = 0;
            }
            currElemStart = currOffset;

            // only set the elements for channels that are active
            if ((execMask & (1 << elemIdx)) || isInitialize || iOper->isSendOper()) {
                // iterate through the bytes of this element
                for (unsigned int byte = 0; byte < typeSize; byte++) {
                    _byteValues.at(currOffset) = val.at(valIdx);
                    valIdx += 1;
                    currOffset += 1;
                }
            }
            else {
                valIdx += typeSize;
                currOffset += typeSize;
            }

            // since we just finished copying this entire element, move on to next element
            currOffset = currElemStart + hStride * typeSize;
            rowIdx += 1;
        }
    }

    // use for checking whether bytes in this VV's physical data match up with VAL
    // returns false if a mismatch is detected, otherwise returns true
    bool checkValue(InstOperand* iOper, const vector<uint8_t>& val) {

        // sanity check that we are not comparing more bytes than there exist in this VV
        // FIXME: see note in setValue() for explanation of assert removal
        // GTREPLAY_ASSERT(val.size() <= getDataSize());

        uint32_t     typeSize = iOper->getTypeSize();
        uint32_t     hStride = iOper->getH();
        uint32_t     vStride = iOper->getV();
        uint32_t     width = iOper->getW();
        uint32_t     execSize = iOper->getExecSize();
        uint32_t     execMask = iOper->getExecMask();
        uint32_t     totalByteSize = iOper->getTotalByteSize();
        uint32_t     lbound = iOper->getLbound();

        // if this is a send instruction operand, the operand must be GRF-aligned (meaning there
        // must be NO subreg offset) and NO regioning information (meaning we iterate through every
        // element with a hstride of 1)
        // see NOTE in ObtainVisaVarValueFromState() for explanation on totalByteSize / typeSize
        if (iOper->isSendOper()) {
            //colOffset = 0;
            lbound = 0;

            hStride = 1;
            execSize = totalByteSize / typeSize;
            width = execSize;
        }

        // keep track of which element position we are at within a row
        uint32_t rowIdx = 0;

        // replaced by l/rbound implementation
        //uint32_t regOffset = rowOffset * gRegSize;
        //uint32_t subregOffset = colOffset * typeSize;
        //uint32_t currOffset = regOffset + subregOffset;

        // keeps track of the byte offset into the reg file that we are defining an element's byte's value at
        uint32_t currOffset = lbound;

        // keeps track of the byte offset of the start of the current row
        uint32_t currRowStart = currOffset;

        // keeps track of the byte offset of the start of the current element
        uint32_t currElemStart = currOffset;

        // keeps track of which byte of the input VAL we are comparing with this VVData
        uint32_t valIdx = 0;

        // iterate through all the elements we need to compare
        for (unsigned int elemIdx = 0; elemIdx < execSize; elemIdx++) {

            // if we have reached the end of this row, move to the next row
            if (rowIdx == width) {
                currOffset = currRowStart + vStride * typeSize;
                currRowStart = currOffset;
                rowIdx = 0;
            }

            currElemStart = currOffset;

            // only check the elements for channels that are active
            if ((execMask & (1 << elemIdx)) || iOper->isSendOper()) {
                // iterate through the bytes of this element
                for (unsigned int byte = 0; byte < typeSize; byte++) {

                    // byte mismatch between val and this VVData's stored bytes
                    if (_byteValues.at(currOffset) != val.at(valIdx)) {
                        return false;
                    }
                    valIdx += 1;
                    currOffset += 1;
                }
            }
            else {
                valIdx += typeSize;
                currOffset += typeSize;
            }

            // since we just finished checking this entire element, move on to next element
            currOffset = currElemStart + hStride * typeSize;
            rowIdx += 1;
        }
        return true;
    }

private:
    const InstOperand*          _iOper;
    vector<uint8_t>             _byteValues;
    const uint32_t              _tileId;
    const uint32_t              _tid;
    InstOperand*                _lastDefOper;
};

class RAError {
public:
    RAError(InstOperand* useOper,
            VVData* storedData,
            vector<uint8_t>* physicalValues) {
        _VVName = useOper->getName();
        _useOffset = useOper->getInstOffset();
        _lastDefOper = storedData->getLastDefOper();
        _lastDefOffset = _lastDefOper->getInstOffset();
        _errorCode = 2;
        _numActiveChannels = 0;

        uint32_t typeSize = useOper->getTypeSize();
        uint32_t hStride = useOper->getH();
        uint32_t vStride = useOper->getV();
        uint32_t width = useOper->getW();
        uint32_t execSize = useOper->getExecSize();
        uint32_t execMask = useOper->getExecMask();
        uint32_t totalByteSize = useOper->getTotalByteSize();
        uint32_t lbound = useOper->getLbound();

        stringstream ssMismatch;
        ssMismatch << fixed << left;
        ssMismatch << "\n----MISMATCH: Detected physical byte values clobbered between Def and Use";
        ssMismatch << "\n              VV Name: " << _VVName;
        ssMismatch << "\n              Reg Alloc: r" << useOper->getRegNum() + useOper->getR() << "." << useOper->getSubRegNum() + useOper->getC();
        if (useOper->getTypeName().compare("")) {
            ssMismatch << ":" << useOper->getTypeName();
        }
        ssMismatch << "\n              Execution Size: (" << execSize << ")";
        ssMismatch << "\n              Execution Mask: [ ";

        // print out the bits of the execution mask
        for (unsigned int bit = 0; bit < execSize; bit++) {
            if (execMask & (1 << bit)) {
                ssMismatch << "1 ";
                _numActiveChannels++;
            }
            else {
                ssMismatch << "0 ";
            }
        }
        ssMismatch << "]";

        ssMismatch << "\n              Total Bytes Used: " << _numActiveChannels * typeSize;
        ssMismatch << "\n    Last Definition:";
        ssMismatch << "\n              Inst Off: " << _lastDefOffset;
        ssMismatch << "\n              Total VV Bytes: " << _lastDefOper->getTotalByteSize();
        ssMismatch << "\n              Stored Bytes:            ";

        // print the stored data bytes that are Used by this operand
        // regioning traversal logic is same as VVData::checkValue()
        if (useOper->isSendOper()) {
            lbound = 0;
            hStride = 1;
            execSize = totalByteSize / typeSize;
            width = execSize;
        }
        uint32_t rowIdx = 0;
        uint32_t currOffset = lbound;
        uint32_t currRowStart = currOffset;
        uint32_t currElemStart = currOffset;
        int newLineCounter = 0;
        for (unsigned int elemIdx = 0; elemIdx < execSize; elemIdx++) {
            if (rowIdx == width) {
                currOffset = currRowStart + vStride * typeSize;
                currRowStart = currOffset;
                rowIdx = 0;
            }
            currElemStart = currOffset;
            if (execMask & (1 << elemIdx)) {
                for (unsigned int byte = 0; byte < typeSize; byte++) {

                    if (newLineCounter == typeSize) {
                        ssMismatch << "\n                                       ";
                        newLineCounter = 0;
                    }
                    ssMismatch << right << setw(2) << setfill('0') << hex << (int)(storedData->getData().at(currOffset)) << " ";
                    newLineCounter++;

                    currOffset += 1;
                }
            }
            else {
                currOffset += typeSize;
            }
            currOffset = currElemStart + hStride * typeSize;
            rowIdx += 1;
        }
        ssMismatch << dec;

        ssMismatch << "\n    Mismatched Use:";
        ssMismatch << "\n              Inst Off: " << _useOffset;
        ssMismatch << "\n              Physical State Bytes:    ";

        // print out the physical register state bytes that are Used by this operand
        newLineCounter = 0;
        uint32_t valIdx = 0;
        for (unsigned int elemIdx = 0; elemIdx < execSize; elemIdx++) {
            if (execMask & (1 << elemIdx)) {
                for (unsigned int byte = 0; byte < typeSize; byte++) {
                    if (newLineCounter == typeSize) {
                        ssMismatch << "\n                                       ";
                        newLineCounter = 0;
                    }
                    ssMismatch << right << setw(2) << setfill('0') << hex << (int)((*physicalValues)[valIdx]) << " ";
                    newLineCounter++;
                    valIdx++;
                }
            }
            else {
                valIdx += typeSize;
                currOffset += typeSize;
            }
        }
        ssMismatch << dec;
        _errMessage = ssMismatch.str();
    }

    RAError(InstOperand* useOper) {
        _VVName = useOper->getName();
        _useOffset = useOper->getInstOffset();
        _errorCode = 1;

        stringstream ssUndefined;
        ssUndefined << fixed << left;
        ssUndefined << "\n----UNDEFINED: Detected a vISA variable that was not Defined before Use";
        ssUndefined << "\n               VVName: " << _VVName;
        _errMessage = ssUndefined.str();
    }

    inline string        getName() const                       { return _VVName; }
    inline string        getMessage() const                    { return _errMessage; }
    inline unsigned int  getErrorCode() const                  { return _errorCode; }

private:
    string               _VVName;
    string               _errMessage;
    uint32_t             _useOffset;
    InstOperand*         _lastDefOper;
    uint32_t             _lastDefOffset;
    unsigned int         _numActiveChannels;

    // 1 = Undefined, 2 = Mismatch
    unsigned int         _errorCode;
};

// Storages for all the InstOperands and VVDatas
using IOList = list<InstOperand>;
IOList IOContainer;
using VVDataList = vector<vector<list<VVData>>>;
VVDataList VVDataContainer;

using RAErrorList = vector<vector<list<RAError>>>;
RAErrorList RAErrors;

// Maps from offset keys to a vector of vISA variable ptr values,
// either a vector of Def VisaVars or a vector of Use VisaVars
using OffsetToOperandsMap = unordered_map<uint32_t, vector<InstOperand*>>;
OffsetToOperandsMap defsMap;
OffsetToOperandsMap usesMap;

// Vector that, for each tile, contains a vector that, for each hardware thread,
// contains a map from the virtual vISA variable name to its VVData object ptr
using VisaVarToDataMap = vector<vector<unordered_map<string, VVData*>>>;
VisaVarToDataMap DataMap;

// Simple map from instruction offset to its execution size, used for printout
unordered_map<unsigned int, unsigned int> instExecSizeMap;

// Reads in metadata about physical assignments from IGC shader-dumped ra_metadata file
void ReadMetadata(string kernelName) {

    GTREPLAY_ASSERT_MSG(shaderDumpDir != "", "User must provide a valid shader dump directory!");

    string dirPath = shaderDumpDir;
    string metadataFilePath = "";
    for (const auto& file : filesystem::directory_iterator(dirPath)) {
        // filter first by ra_metadata file extension and then by a match with the kernel name
        string METADATA_EXTENSION = ".ra_metadata";
        if (file.path().extension() == METADATA_EXTENSION &&
            file.path().stem().string().find(kernelName) != string::npos) {
            metadataFilePath = file.path().string();
        }
    }
    if (metadataFilePath == "") {
        cout << "ERROR: failed to find a metadata file corresponding to kernel " << kernelName << "\n" << endl;
        return;
    }
    cout << "METADATA FILE PATH: " << metadataFilePath << "\n";

    ifstream MDFile;
    MDFile.open(metadataFilePath, ios::binary);

    unsigned int numKernels;
    MDFile.read((char*)&numKernels, sizeof(unsigned int));

    unsigned int numInsts;
    for (unsigned int k = 0; k < numKernels; k++) {

        MDFile.read((char*)&numInsts, sizeof(unsigned int));

        unsigned int numDefs;
        unsigned int numUses;
        unsigned int binaryOffset;
        unsigned int execSize;
        for (unsigned int i = 0; i < numInsts; i++) {

            MDFile.read((char*)&execSize, sizeof(unsigned int));
            MDFile.read((char*)&binaryOffset, sizeof(unsigned int));
            instExecSizeMap.emplace(binaryOffset, execSize);

            MDFile.read((char*)&numDefs, sizeof(unsigned int));
            vector<InstOperand*> DefIOs;
            for (unsigned int d = 0; d < numDefs; d++) {
                Def def;

                // uchar (1) : regFileKind
                // ushort (1) : typeSize
                // uint (10) : reg, subreg, byteSize, aliasOffset, row/colOffset, hstride, root/l/rBound, nameLen
                streamsize defOffset = sizeof(unsigned char) + sizeof(unsigned short) + sizeof(unsigned int) * 11;

                MDFile.read((char*)&def, defOffset);
                char* name = new char[def.nameLen];
                MDFile.read(name, def.nameLen);
                def.name = name;
                string VVName;
                VVName.assign(def.name, def.nameLen);
                delete[] name;

                IOContainer.emplace_back();
                auto* var = &IOContainer.back();
                var->setName(VVName);
                var->setRegFile(GED_REG_FILE_GRF); // FIXME correct later when other register types added
                var->setRegNum(def.reg);
                var->setSubRegNum(def.subreg);

                // FIXME note that rootBound in the metadata is actually offsetFromR0 + aliasOffset,
                // since we just want the offset of the root here, remove the aliasOffset. Also, note
                // that the left and right bounds already include the aliasOffset
                var->setAliasOffset(def.aliasOffset);
                var->setRootBound(def.rootBound - def.aliasOffset);
                var->setLbound(def.leftBound);
                var->setRbound(def.rightBound);

                var->setTypeSize(def.typeSize);
                var->setH(def.hstride);
                var->setV(0);
                var->setW(0);
                var->setExecSize(execSize);
                var->setTotalByteSize(def.byteSize);
                var->setRegFileKind(def.regFileKind);

                if (def.typeSize && gRegSize) {
                    uint32_t aliasRowOffset = def.aliasOffset / gRegSize;
                    uint32_t aliasColOffset = (def.aliasOffset % gRegSize) / def.typeSize;
                    var->setR(def.rowOffset + aliasRowOffset);
                    var->setC(def.colOffset + aliasColOffset);
                }

                var->setIsDef(true);
                DefIOs.push_back(var);
            }
            defsMap.emplace(binaryOffset, DefIOs);

            MDFile.read((char*)&numUses, sizeof(unsigned int));
            vector<InstOperand*> UseIOs;
            for (unsigned int u = 0; u < numUses; u++) {
                Use use;

                // uchar (1) : regFileKind
                // ushort (1) : typeSize
                // uint (12) : reg, subreg, byteSize, aliasOffset, row/colOffset, h/vstride, width, root/l/rBound, nameLen
                streamsize useOffset = sizeof(unsigned char) + sizeof(unsigned short) + sizeof(unsigned int) * 13;

                MDFile.read((char*)&use, useOffset);
                char* name = new char[use.nameLen];
                MDFile.read(name, use.nameLen);
                use.name = name;
                string VVName;
                VVName.assign(use.name, use.nameLen);
                delete[] name;

                IOContainer.emplace_back();
                auto* var = &IOContainer.back();
                var->setName(VVName);
                var->setRegFile(GED_REG_FILE_GRF);
                var->setRegNum(use.reg);
                var->setSubRegNum(use.subreg);

                // FIXME note that rootBound in the metadata is actually offsetFromR0 + aliasOffset,
                // since we just want the offset of the root here, remove the aliasOffset. Also, note
                // that the left and right bounds already include the aliasOffset
                var->setAliasOffset(use.aliasOffset);
                var->setRootBound(use.rootBound - use.aliasOffset);
                var->setLbound(use.leftBound);
                var->setRbound(use.rightBound);

                var->setTypeSize(use.typeSize);
                var->setH(use.hstride);
                var->setV(use.vstride);
                var->setW(use.width);
                var->setExecSize(execSize);
                var->setTotalByteSize(use.byteSize);
                var->setRegFileKind(use.regFileKind);

                if (use.typeSize && gRegSize) {
                    uint32_t aliasRowOffset = use.aliasOffset / gRegSize;
                    uint32_t aliasColOffset = (use.aliasOffset % gRegSize) / use.typeSize;
                    var->setR(use.rowOffset + aliasRowOffset);
                    var->setC(use.colOffset + aliasColOffset);
                }
                var->setIsDef(false);
                UseIOs.push_back(var);
            }
            usesMap.emplace(binaryOffset, UseIOs);
        }
        //FIXME remove this in the event of multiple kernels in one metadata file
        MDFile.close();
        return;
    }
    MDFile.close();
    return;
}

// Inspects the GTReplayState STATE to determine the physical register data corresponding
// to InstOperand IOPER, saves this value (consisting of a number of Bytes) into vector VAL
void ObtainVisaVarValueFromState(GTReplayState state, InstOperand& iOper, vector<uint8_t>& val, bool isInitialize)
{
    GED_REG_FILE regFile = iOper.getRegFile();
    uint32_t     baseRegNum = iOper.getRegNum();
    uint32_t     baseSubRegNum = iOper.getSubRegNum();
    uint32_t     typeSize = iOper.getTypeSize();
    uint32_t     hStride = iOper.getH();
    uint32_t     vStride = iOper.getV();
    uint32_t     width = iOper.getW();
    uint32_t     execSize = iOper.getExecSize();
    uint32_t     execMask = iOper.getExecMask();
    uint32_t     totalByteSize = iOper.getTotalByteSize();
    uint32_t     rootBound = iOper.getRootBound();
    uint32_t     lbound = iOper.getLbound();

    // if this is a Def operand, there is no width or vStride region information; we can
    // simulate this behavior by setting the width equal to the execSize. This will remove
    // the effect of the vStride, since there will only be one row containing ALL the elems
    if (iOper.isDef()) {
        width = execSize;
    }

    // if this is a send instruction operand, the operand must be GRF-aligned (meaning there
    // must be NO subreg offset) and NO regioning information (meaning we iterate through every
    // element with a hstride of 1)
    //
    // NOTE: unlike regular instructions, send insts can move MULTIPLE elements PER channel
    // (other insts only move one element per channel, which is why the total number of obtained
    // bytes put in VAL is typeSize * execSize bytes). Here, we will move the TOTAL BYTE SIZE
    // of the virtual variable into VAL, meaning VAL will have a size of totalByteSize. Accomplish
    // this by setting width equal to (totalByteSize / typeSize), which is the total number of
    // elements for this virtual variable across all its channels. (Think of this as iterating
    // one long row containing ALL the elements)
    //
    // NOTE: For initializing the byteValues data buffer, we will also be moving the totalByteSize
    // of the virtual variable into VAL. The strategy for this is identical to that of send insts
    if (iOper.isSendOper() || isInitialize) {
        //baseSubRegNum = 0;
        //colOffset = 0;
        lbound = 0;

        hStride = 1;
        execSize = totalByteSize / typeSize;
        width = execSize;
    }

    // replaced by l/rbound implementation
    //uint32_t regByteOffset = (baseRegNum + rowOffset) * gRegSize;
    //uint32_t subregByteOffset = (baseSubRegNum + colOffset) * typeSize;
    //uint32_t startBase = regByteOffset + subregByteOffset;

    // byte offset into register file for start of the physical assignment region
    uint32_t startBase = rootBound + lbound;

    union {
        uint8_t  byte[32];
        uint16_t word[32];
        uint32_t dword[32];
        uint64_t qword[32];
    } value;

    uint32_t errCode;

    // keep track of which element position we are at within a row
    uint32_t rowIdx = 0;

    // keeps track of the byte offset into the reg file that we are reading an element's value from
    uint32_t currOffset = startBase;

    // keeps track of the byte offset of the start of the current row
    uint32_t currRowStart = currOffset;

    // iterate through all the elements we need to read
    for (unsigned int elemIdx = 0; elemIdx < execSize; elemIdx++) {

        // if we have reached the end of this row, move to the next row
        if (rowIdx == width) {
            currOffset = currRowStart + vStride * typeSize;
            currRowStart = currOffset;
            rowIdx = 0;
        }

        uint32_t regNum = currOffset / gRegSize;
        uint32_t subRegNum = (currOffset % gRegSize) / typeSize;

        // only obtain elements for channels that are active
        if ((execMask & (1 << elemIdx)) || isInitialize || iOper.isSendOper()) {
            if (regFile == GED_REG_FILE_GRF)
            {
                // copy the appropriate number of bytes for this elem according to type size
                switch (typeSize) {
                case 1:
                    errCode = GTReplay_GetGrfRegByte(state, regNum, subRegNum, value.byte + elemIdx);
                    GTREPLAY_ASSERT(errCode != -1);
                    break;

                case 2:
                    errCode = GTReplay_GetGrfRegWord(state, regNum, subRegNum, value.word + elemIdx);
                    GTREPLAY_ASSERT(errCode != -1);
                    break;

                case 4:
                    errCode = GTReplay_GetGrfRegDword(state, regNum, subRegNum, value.dword + elemIdx);
                    GTREPLAY_ASSERT(errCode != -1);
                    break;

                case 8:
                    errCode = GTReplay_GetGrfRegQword(state, regNum, subRegNum, value.qword + elemIdx);
                    GTREPLAY_ASSERT(errCode != -1);
                    break;

                default:
                    GTREPLAY_ASSERT(0);
                }
            }
            else {
                GTREPLAY_ASSERT(0);
            }
        }

        // since we just finished copying this entire element, move on to next element
        currOffset += hStride * typeSize;
        rowIdx += 1;
    }

    // store the obtained bytes in the VAL input vector
    val.resize(execSize * typeSize);
    memcpy_s(val.data(), val.size(), value.byte, val.size());
}

/*
 * VisaUseCallback - callback called before instruction execution where a VISA var is used
 *
 * @params[in] tid - the ID of the GPU HW thread for which the callback is called
 * @params[in] ins - a handle to the current instruction
 * @params[in] state - a handle to the HW Thread state corresponding to tid
 * @params[in] ioper - a pointer to the InstOperand object of this Use
 */
void VisaUseCallback(uint32_t tileId, uint32_t tid, GTReplayIns ins, GTReplayState state, void* ioper)
{
    GTREPLAY_ASSERT(tileId < gMaxNumOfTiles && tid < gMaxNumOfHwThreads);

    InstOperand* IOPtr = (InstOperand*)ioper;
    string VVName = IOPtr->getName();
    uint32_t offset = GTReplay_InsOffset(ins);

    uint32_t execMask = GTReplay_DynamicExecMask(ins, state);
    IOPtr->setExecMask(execMask);

    // lookup this use virtual variable in the data map; cannot Use a VV that
    // has not been Defined yet, so report an error if we do not find this VV.
    // The exception is if this is a Use of an Input kind VV, then we do not
    // report an error since this VV was implicitly defined at kernel start
    auto VVDataItem = DataMap[tileId][tid].find(VVName);
    if (VVDataItem == DataMap[tileId][tid].end()) {
        if (IOPtr->getRegFileKind() != 0x4) {
            RAError undefinedVV = RAError(IOPtr);
            RAErrors[tileId][tid].push_back(undefinedVV);
        }
        return;
    }
    VVData* storedData = VVDataItem->second;

    // obtain the current value by inspecting the physical register state
    vector<uint8_t> physicalValues;
    ObtainVisaVarValueFromState(state, *IOPtr, physicalValues, false);

    // compare the stored data with the current physical data
    if (!storedData->checkValue(IOPtr, physicalValues)) {

        RAError mismatchVV = RAError(IOPtr, storedData, &physicalValues);
        RAErrors[tileId][tid].push_back(mismatchVV);
        return;
    }
}

/*
 * VisaDefCallback - callback called after instruction execution when the destination is set with a new VISA var value
 *
 * @params[in] tid - the ID of the GPU HW thread for which the callback is called
 * @params[in] ins - a handle to the current instruction
 * @params[in] state - a handle to the HW Thread state corresponding to tid
 * @params[in] ioper - a pointer to the InstOperand object of this Def
 */
void VisaDefCallback(uint32_t tileId, uint32_t tid, GTReplayIns ins, GTReplayState state, void* ioper)
{
    GTREPLAY_ASSERT(tileId < gMaxNumOfTiles && tid < gMaxNumOfHwThreads);

    InstOperand* IOPtr = (InstOperand*)ioper;
    string VVName = IOPtr->getName();
    uint32_t offset = GTReplay_InsOffset(ins);

    uint32_t execMask = GTReplay_DynamicExecMask(ins, state);
    IOPtr->setExecMask(execMask);

    // used for storing bytes obtained by inspecting the physical register state
    vector<uint8_t> physical_value;

    // lookup this use virtual variable in the DataMap
    auto VVDataItem = DataMap[tileId][tid].find(VVName);
    if (VVDataItem == DataMap[tileId][tid].end()) {
        // create a new VVData object
        VVDataContainer[tileId][tid].emplace_back((const InstOperand*)IOPtr, tileId, tid);
        VVData* VVDataPtr = &VVDataContainer[tileId][tid].back();

        // initialize the saved VVData's byteValues data buffer with ALL totalByteSize
        // bytes that are currently present within this VV's register file allocation.
        // These bytes are obtained by inspecting the physical register state. Note that
        // this will trivially also encompass all the bytes that are supposed to be set by
        // the InstOperand argument
        ObtainVisaVarValueFromState(state, *IOPtr, physical_value, true);
        VVDataPtr->setValue(IOPtr, physical_value, true);

        // add this new VVname:VVData item to the DataMap
        DataMap[tileId][tid].emplace(VVName, VVDataPtr);
    }
    else {
        // if this VV key already exists, update its DefinedData
        VVData* storedVVData = VVDataItem->second;

        // since this VVdata's byteValues data buffer has already been previously
        // initialized, we only update the bytes that are being set by the InstOperand
        // argument, leaving the rest of the mapped data buffer unmodified
        ObtainVisaVarValueFromState(state, *IOPtr, physical_value, true);
        storedVVData->setValue(IOPtr, physical_value, true);
    }
}

void DataMapClearCallback(uint32_t tileId, uint32_t tid, GTReplayIns ins, GTReplayState state, void* extra) {
    // remove all the VVData elements for this completed software thread
    VVDataContainer[tileId][tid].clear();

    // remove all the VVname:VVData entries from this completed software thread's map
    DataMap[tileId][tid].clear();
}

/*
 * OnKernelComplete - callback called upon kernel completion
 *
 * @params[in] kernel - a handle to the kernel
 */
void OnKernelComplete(GTReplayKernel kernel)
{
    unsigned int totalUndefined = 0;
    unsigned int totalMismatch = 0;
    cout << "\n\nRA VALIDATION COMPLETE\n";
    bool foundError = false;
    for (auto tile : RAErrors) {
        for (auto thread : tile) {
            for (auto const& err : thread) {
                cout << err.getMessage() + "\n";
                foundError = true;
                if (err.getErrorCode() == 1) {
                    totalUndefined++;
                }
                else if (err.getErrorCode() == 2) {
                    totalMismatch++;
                }
                else {
                    cout << "Unknown Error Code Detected\n";
                }
            }
        }
    }
    if (foundError) {
        cout << "\nUndefined Errors: " << totalUndefined;
        cout << "\nMismatch Errors: " << totalMismatch << "\n";
        cout << "\nVALIDATION FAILED: ERRORS DETECTED\n";
    }
    else {
        cout << "\nVALIDATION PASSED: NO ERRORS DETECTED\n";
    }
}

// used for pretty-print formatting of an instruction operand
string reg(InstOperand* oper) {
    stringstream ss;
    ss << fixed << left;

    // empty Def placeholder
    if (oper == NULL) {
        ss << "(none)";
        return ss.str();
    }

    ss << "r" << (oper->getRegNum() + oper->getR()) << "." << (oper->getSubRegNum() + oper->getC());
    if (oper->isDef()) {
        ss << "<" << oper->getH() << ">";
    }
    else {
        ss << "<" << oper->getV() << ";" << oper->getW() << "," << oper->getH() << ">";
    }
    if (oper->getTypeName().compare("")) {
        ss << ":" << oper->getTypeName();
    }
    return ss.str();
}

// used for pretty-print formatting of a virtual variable name
string rootVV(InstOperand* oper) {
    stringstream ss;
    ss << fixed << left;

    // empty Def placeholder
    if (oper == NULL) {
        ss << "(none)";
        return ss.str();
    }

    ss << oper->getName() << "(" << oper->getR() << "," << oper->getC() << ")";
    return ss.str();
}

/*
 * OnKernelBuild - callback called before kernel execution
 *                 The purpose of this callback is to traverse the kernel binary and instrument callbacks
 *
 * @params[in] kernel - a handle to the kernel
 */
void OnKernelBuild(GTReplayKernel kernel)
{
    uint32_t gModelId = GTReplay_GetModel(kernel);

    gMaxNumOfHwThreads = GTReplay_MaxNumOfHWThreads(gModelId);
    gRegSize = GTReplay_RegisterWidth(gModelId); // 32 Bytes for DG2 Platform
    gMaxNumOfTiles = GTReplay_MaxNumOfTiles(kernel);
    GTREPLAY_ASSERT(gMaxNumOfTiles);

    // clear all existing metadata
    defsMap.clear();
    usesMap.clear();
    DataMap.clear();

    VVDataContainer.resize(gMaxNumOfTiles);
    DataMap.resize(gMaxNumOfTiles);
    RAErrors.resize(gMaxNumOfTiles);
    for (uint32_t tileId = 0; tileId < gMaxNumOfTiles; tileId++) {
        VVDataContainer[tileId].resize(gMaxNumOfHwThreads);
        DataMap[tileId].resize(gMaxNumOfHwThreads);
        RAErrors[tileId].resize(gMaxNumOfHwThreads);
    }

    cout << "\n\n===========================\n";
    cout << " REG ALLOC VALIDATION TOOL \n";
    cout << "===========================\n\n";

    cout << "STARTED METADATA READ\n\n";

    // read in metadata about virtual variables
    uint32_t kernelNameSize = 0;
    GTReplay_GetKernelName(kernel, &kernelNameSize, nullptr);
    char* buf = new char[kernelNameSize + 1]();
    GTReplay_GetKernelName(kernel, &kernelNameSize, buf);
    string fullKernelName = string(buf);
    delete[] buf;

    // trim off the suffix to obtain just the kernel name
    string DELIMITER = "___";
    size_t end = fullKernelName.find(DELIMITER, 0);
    string kernelName = fullKernelName.substr(0, end);

    cout << "KERNEL NAME: " << kernelName << "\n";
    ReadMetadata(kernelName);

    cout << "\nFINISHED METADATA READ\n\n";

    int bblId = 0;
    // Traverse all the basic blocks
    for (GTReplayBbl bbl = GTReplay_BblHead(kernel); GTReplay_BblValid(bbl); bbl = GTReplay_BblNext(bbl))
    {
        cout << "BBL: " << bblId++ << "\n\n";

        // Traverse all the instruction within the basic blocks
        for (GTReplayIns ins = GTReplay_InsHead(bbl); GTReplay_InsValid(ins); ins = GTReplay_InsNext(ins))
        {
            uint32_t instID = GTReplay_InsId(ins);
            uint32_t offset = GTReplay_InsOffset(ins);
            int32_t opcodeId = GTReplay_Opcode(ins);
            const char* opcodeName = GTReplay_OpcodeName(opcodeId);
            bool isSendInst = GTReplay_IsSend(ins);

            stringstream ssTop;
            ssTop << fixed << left;
            ssTop << "----INST: ";
            ssTop.width(10);
            ssTop << instID;
            ssTop.width(8);
            ssTop << opcodeName;

            stringstream ssBot;
            ssBot << fixed << left;
            ssBot << "     off: ";
            ssBot.width(10);
            ssBot << offset;
            ssBot.width(8);
            auto execSizeItem = instExecSizeMap.find(offset);
            string execSizePrint = "(" + to_string(execSizeItem->second) + ")";
            ssBot << execSizePrint;

            auto defs = defsMap.find(offset);
            if (defs != defsMap.end())
            {
                for (InstOperand* defIO : defs->second) {
                    // set the isSendOper attr of this operand to true if this is a send inst
                    if (isSendInst) {
                        defIO->setIsSendOper(true);
                    }
                    defIO->setInstOffset(offset);

                    uint32_t defDataType = GTReplay_GetDstDataType(ins);
                    if (defDataType != GED_DATA_TYPE_INVALID) {
                        defIO->setTypeName(string(GTReplay_DataTypeName(defDataType)));
                    }
                    else {
                        defIO->setTypeName("");
                    }
                    // Register VisaDefCallback to be called after instruction execution
                    GTReplay_RegisterCallbackAfterIns(kernel, ins, VisaDefCallback, defIO);

                    ssTop << setw(25) << reg(defIO);
                    ssBot << setw(25) << rootVV(defIO);
                }
            }
            if (defs->second.empty()) {
                ssTop << setw(25) << reg(NULL);
                ssBot << setw(25) << rootVV(NULL);
            }

            auto uses = usesMap.find(offset);
            if (uses != usesMap.end())
            {
                uint32_t numSrcs = (uint32_t)uses->second.size();
                for (uint32_t srcIdx = 0; srcIdx < numSrcs; srcIdx++) {
                    InstOperand* useIO = uses->second[srcIdx];

                    // set the isSendOper attr of this operand to true if this is a send inst
                    if (isSendInst) {
                        useIO->setIsSendOper(true);
                    }
                    useIO->setInstOffset(offset);

                    uint32_t useDataType = GTReplay_GetSrcDataType(ins, srcIdx);
                    if (useDataType != GED_DATA_TYPE_INVALID) {
                        useIO->setTypeName(string(GTReplay_DataTypeName(useDataType)));
                    }
                    else {
                        useIO->setTypeName("");
                    }

                    // Register VisaDefCallback to be called before instruction execution
                    GTReplay_RegisterCallbackBeforeIns(kernel, ins, VisaUseCallback, useIO);

                    ssTop << setw(25) << reg(useIO);
                    ssBot << setw(25) << rootVV(useIO);
                }
            }

            cout << ssTop.str() << "\n";
            cout << ssBot.str() << "\n\n";

            // if this is the end of a software thread, clear its DataMap entry to prevent
            // false-positive mismatches when the same hardware thread is reused later
            if (GTReplay_IsEOT(ins)) {
                GTReplay_RegisterCallbackAfterIns(kernel, ins, DataMapClearCallback, NULL);
            }
        }
    }
    cout << "FINISHED INST TRAVERSAL";
}

/*
 * GTReplay_Entry - tool entry point
 */
extern "C"
DLLEXP void FASTCALL GTReplay_Entry(int argc, const char* argv[])
{
    // configure GTReplay
    ConfigureGTReplay(argc, argv);

    // register OnKernelBuild and OnKernelComplete callbacks
    GTReplay_RegisterOnKernelBuildCallback(OnKernelBuild);
    GTReplay_RegisterOnKernelCompleteCallback(OnKernelComplete);

    // Start GTReplay
    GTReplay_Start();
}
