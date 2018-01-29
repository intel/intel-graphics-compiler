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

#ifndef GED_DISASSEMBLER_H
#define GED_DISASSEMBLER_H

# if GED_DISASSEMBLY

#include <sstream>
#include "xcoder/ged_ins.h"

using std::stringstream;


class GEDDisassembler : public GEDIns
{
public:
    // API FUNCTIONS

    /*!
     * Get the instruction's disassembly.
     *
     * @return      The instruction's disassembly.
     */
    string Disassemble();

public:
    // BLOCKED API FUNCTIONS

    // We expect the GEDIns class, and therefore the GEDDisassembler class, to be supplied by the user. However, the user allocates a
    // POD struct and we reinterpret_cast it to GEDDisassembler. To enforce this usage model, the constructors, destructor and
    // assignment operators are deleted.
    inline GEDDisassembler() = delete;                                      // default constructor
    inline GEDDisassembler(const GEDDisassembler& src) = delete;            // copy constructor
    inline GEDDisassembler& operator=(const GEDDisassembler& src) = delete; // copy-assign
    inline ~GEDDisassembler() = delete;                                     // destructor
    GEDDisassembler(GEDDisassembler&&) = delete;                            // move constructor
    GEDDisassembler& operator=(GEDDisassembler&&) = delete;                 // move-assign

private:
    // PRIVATE MEMBER FUNCTIONS

    inline const ModelDisassemblyData& GetCurrentModelDisassemblyData() const { return ModelsDisassemblyArray[GetCurrentModel()]; }

    bool IterateDisassemblyBlocks(const ged_disassembly_table_t blocks, stringstream& strm);
    bool PrintDisassemblyTokens(const ged_disassembly_block_t& block, stringstream& strm);
    bool PrintFieldsList(const ged_disassembly_block_t& block, stringstream& strm);
    bool PrintInterpretedField(const ged_disassembly_block_t& block, stringstream& strm);

    // Printing utility functions
    bool PrintToken(const ged_disassembly_token_t& token, stringstream& strm, const string& prefix = "");
    bool PrintField(const uint16_t field, stringstream& strm, const string& prefix = "");
    bool PrintStringField(const uint16_t field, stringstream& strm, const string& prefix = "");
    bool PrintNumericField(const uint16_t field, const GED_FIELD_TYPE fieldType, stringstream& strm, const string& prefix = "");
    bool PrintRawField(const uint16_t field, stringstream& strm, const string& prefix = "");
    bool PrintPositionInterpretedField(const ged_disassembly_block_interpreter_t& interpreter, stringstream& strm);

    // Getter utility functions
    bool GetGeneralizedFieldValue(const ged_generalized_field_t& genField, uint32_t& value);
};

# endif // GED_DISASSEMBLER_H

#endif // GED_DISASSEMBLY_H
