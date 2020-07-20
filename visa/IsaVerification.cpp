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

#ifndef IS_RELEASE_DLL
#include <iostream>
#include <sstream>
#include <string>
#include <limits>

using namespace std;

#include "Common_ISA.h"
#include "Common_ISA_util.h"

#include "VISADefines.h"
#include "IsaVerification.h"

#include "IsaDisassembly.h"
#include "PreDefinedVars.h"
#include "Attributes.hpp"

#include "Gen4_IR.hpp"
/* stdio.h portability code start */
#ifdef _WIN32
#else
#define _scprintf(...) \
    snprintf(NULL, 0, __VA_ARGS__)
#endif

using namespace vISA;

#define REPORT_HEADER(opt, cond, ...)          \
do if (!(cond)) {                              \
    int sz = _scprintf(__VA_ARGS__) + 1;       \
    char* buf = (char*)malloc(sz);             \
    assert(buf != NULL);                       \
    memset(buf, 0, sz);                        \
    SNPRINTF(buf, sz, __VA_ARGS__);            \
    error_list.push_back(createIsaError(       \
        isaHeader, header, string(buf), opt)); \
    free(buf);                                 \
} while (0)


#define REPORT_INSTRUCTION(opt, cond, ...)           \
do if (!(cond)) {                                    \
    int sz = _scprintf(__VA_ARGS__) + 1;             \
    char* buf = (char*)malloc(sz);                   \
    assert(buf != NULL);                             \
    memset(buf, 0, sz);                              \
    SNPRINTF(buf, sz, __VA_ARGS__);                  \
    error_list.push_back(createIsaError(             \
        isaHeader, header, string(buf), opt, inst)); \
    free(buf);                                       \
} while (0)

string raw_opnd::toString() const
{
    stringstream sstr;
    sstr << "V" << index;
    if (offset != 0)
    {
        sstr << "." << offset;
    }
    return sstr.str();
}

void vISAVerifier::writeReport(const char* filename)
{
    if (kerror_list.size() > 0 || error_list.size() > 0)
    {
        ofstream report;
        report.open(filename);

        if (kerror_list.size() > 0)
        {
            report << "Kernel Header / Declare Errors: " << endl;
            for (auto I = kerror_list.begin(), E = kerror_list.end(); I != E; I++)
                report << (*I) << endl;
            report << "\n\n\n";
        }

        report << "Instruction / Operand / Region Errors: " << endl;

        for (auto I = error_list.begin(), E = error_list.end(); I != E; I++)
            report << (*I) << endl;

        report << "\n\n\n";
        report.close();
    }
}

static int getDstIndex(const CISA_INST* inst)
{
    ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
    int dstIndex = -1;
    if (inst->opnd_count > 0)
    {
        switch(ISA_Inst_Table[opcode].type)
        {
            case ISA_Inst_Mov:
            case ISA_Inst_Arith:
            case ISA_Inst_Logic:
            case ISA_Inst_Address:
                 dstIndex = 0;
                 break;
            case ISA_Inst_Compare:
                 dstIndex = 1;
                 break;
            default:
                 break; // Prevent gcc warning
        }
    }
    return dstIndex;
}

// diagDumpInstructionOperandDecls() is used to generate the error report
static string diagDumpInstructionOperandDecls(
    const common_isa_header& isaHeader,
    const print_format_provider_t* header,
    const CISA_INST* inst,
    Options *options)
{
    stringstream sstr;

    unsigned numPreDefinedVars = Get_CISA_PreDefined_Var_Count();

    for (unsigned i = 0; i < inst->opnd_count; i++)
    {
        if (inst->opnd_array[i]->opnd_type == CISA_OPND_VECTOR)
        {
            const vector_opnd& opnd = getVectorOperand(inst, i);
            uint32_t index = opnd.getOperandIndex();

            if (numPreDefinedVars <= index && index < header->getVarCount())
            switch (opnd.getOperandClass())
            {
                case OPERAND_STATE     :
                case OPERAND_GENERAL   : sstr << printVariableDecl  (header, index-numPreDefinedVars, options); break;
                case OPERAND_ADDRESS   :
                case OPERAND_INDIRECT  : sstr << printAddressDecl   (isaHeader, header, index); break;
                case OPERAND_PREDICATE : sstr << printPredicateDecl (header, index); break;
                case OPERAND_ADDRESSOF : sstr << "ADDRESSOF Operand decl... are those even allowed>" << endl; break;
                case OPERAND_IMMEDIATE : sstr << "Immediate operand: " << getPrimitiveOperand<unsigned>(inst, i) << endl; break;
                default                : sstr << "Operand type: " << opnd.getOperandClass() << " unable to print." << endl; break;
            }
        }
        else if (inst->opnd_array[i]->opnd_type == CISA_OPND_RAW)
        {
            uint32_t index = getRawOperand(inst, i).index;
            if ( numPreDefinedVars <= index && index < header->getVarCount() + numPreDefinedVars )
                sstr << printVariableDecl(header, index-numPreDefinedVars, options);
        }
        else if (inst->opnd_array[i]->opnd_type == CISA_OPND_OTHER) // new loader only
        {
        }
        else
        {
            sstr << "unknown operand?";
        }

        sstr << endl;
        if (i != inst->opnd_count-1)
        sstr << setw(33) << "                               ";
    }

    return sstr.str();
}

static string createIsaError(
    const common_isa_header& isaHeader,
    const print_format_provider_t* header,
    string msg,
    Options *opt,
    const CISA_INST* inst = NULL)
{
    stringstream sstr;
    if (!inst)
    sstr << "\n/-------------------------------------------!!!KERNEL HEADER ERRORS FOUND!!!-------------------------------------------\\\n";
    else
    sstr << "\n/--------------------------------------------!!!INSTRUCTION ERROR FOUND!!!---------------------------------------------\\\n";
    sstr << setw(33) << "Error in CISA routine with name: " << (char*)header->getString(header->getNameIndex()) << endl;
    sstr << setw(33) << "Error Message: " << msg << endl;

    if (NULL != inst)
    {
        sstr << setw(33) << "Diagnostics: " << endl;
        sstr << setw(33) << " Instruction variables' decls: ";
        sstr << diagDumpInstructionOperandDecls(isaHeader, header, inst, opt) << endl;
        sstr << setw(33) << " Violating Instruction: "  << printInstruction(header, inst, opt) << endl;
    }

    sstr << "\\----------------------------------------------------------------------------------------------------------------------/\n";
    return sstr.str();
}

void vISAVerifier::verifyPredicateDecl(
    unsigned declID)
{
    string declError = string(" Error in predicate variable decl: ") + printPredicateDecl(header, declID);

    REPORT_HEADER(options,header->getPred(declID)->name_index < header->getStringCount(), "P%d's name index(%d) is not valid: %s", declID, header->getPred(declID)->name_index, declError.c_str());

    switch (header->getPred(declID)->num_elements)
    {
        case 1:
        case 2:
        case 4:
        case 8:
        case 16:
        case 32:
            break;
        default:
            REPORT_HEADER(options,false, "P%d's number of elements(%d) is not valid: %s", declID, header->getPred(declID)->num_elements, declError.c_str());
    }
}

void vISAVerifier::verifyAddressDecl(unsigned declID)
{
    string declError = string(" Error in address variable decl: ") + printAddressDecl(isaHeader, header, declID);

    REPORT_HEADER(options,header->getAddr(declID)->name_index < header->getStringCount(), "A%d's name index(%d) is not valid: %s", declID, header->getAddr(declID)->name_index, declError.c_str());
    REPORT_HEADER(options,header->getAddr(declID)->num_elements <= 16, "Max possible address registers are 16 on BDW+: %s", declError.c_str());

    /// TODO: Fix/Reenable this verification check.
    #if 0
    switch (header->getAddr(declID)->num_elements)
    {
        case 1:
        case 2:
        case 4:
        case 8:
        case 16:
            break;
        default:
            REPORT_HEADER(options,false, "A%d's number of elements(%d) is not valid.", declID, header->getAddr(declID)->num_elements);
    }
    #endif
}

void vISAVerifier::verifyVariableDecl(
    unsigned declID)
{
    string declError = string(" Error in CISA variable decl: ") + printVariableDecl(header, declID, options);

    const var_info_t* var = header->getVar(declID);
    VISA_Align align = var->getAlignment();

    unsigned numPreDefinedVars = Get_CISA_PreDefined_Var_Count();

    REPORT_HEADER(options, var->name_index < header->getStringCount(),
                  "V%d's name index(%d) is not valid: %s",
                  declID + numPreDefinedVars, var->name_index,
                  declError.c_str());

    switch (var->getType())
    {
        case ISA_TYPE_V:
        case ISA_TYPE_VF:
        case ISA_TYPE_BOOL:
        case ISA_TYPE_UV:
            REPORT_HEADER(options,false, "V%d's type(%s) is not legal: %s", declID, CISATypeTable[var->getType()].typeName, declError.c_str());
            return;
        default:
            break; // Prevent gcc warning
    }

    REPORT_HEADER(options, var->num_elements != 0 && var->num_elements <= COMMON_ISA_MAX_VARIABLE_SIZE,
                  "V%d's number of elements(%d) is out of range: %s",
                  declID + numPreDefinedVars, var->num_elements,
                  declError.c_str());
    REPORT_HEADER(options, !(var->alias_index == 0 && var->alias_offset != 0),
                  "V%d's alias offset must be zero when it is not aliased: %s",
                  declID + numPreDefinedVars, declError.c_str());

    if (var->alias_index >= numPreDefinedVars)
    {
        const var_info_t* currAliasVar = header->getVar(var->alias_index-numPreDefinedVars);
        unsigned totalOffset = var->alias_offset;

        set<uint32_t> visitedAliasIndices;

        while (currAliasVar->alias_index >= numPreDefinedVars)
        {
            if (visitedAliasIndices.find(currAliasVar->alias_index) != visitedAliasIndices.end())
            {
                REPORT_HEADER(options,false, "Circular alias detected, alias index: %d", currAliasVar->alias_index);
                break;
            }

            visitedAliasIndices.insert(currAliasVar->alias_index);

            REPORT_HEADER(options,currAliasVar->alias_index < header->getVarCount() + numPreDefinedVars,
                              "Aliased variable aliases to an invalid alias index. "
                              "Variable count: %d. invalid index: %d. %s",
                              header->getVarCount() + numPreDefinedVars,
                              currAliasVar->alias_index, declError.c_str());

            totalOffset += currAliasVar->alias_offset;
            currAliasVar = header->getVar(currAliasVar->alias_index-numPreDefinedVars);
        }

        if (currAliasVar->alias_index < header->getVarCount() + numPreDefinedVars)
        {
            REPORT_HEADER(options,totalOffset < (currAliasVar->num_elements * (unsigned)CISATypeTable[currAliasVar->getType()].typeSize),
                              "Variable decl's alias offset exceeds the bounds of the aliased variable decl allocation size: %s", declError.c_str());
            VISA_Align baseAlign = std::max(currAliasVar->getAlignment(), currAliasVar->getTypeAlignment());
            REPORT_HEADER(options, baseAlign >= var->getTypeAlignment(),
                "base variable must be at least type-aligned to this variable: %s", declError.c_str());
        }
    }

    switch (align)
    {
        case ALIGN_BYTE:
            break;
        case ALIGN_WORD:
            break;
        case ALIGN_DWORD:
            break;
        case ALIGN_QWORD:
            break;
        case ALIGN_OWORD:
            break;
        case ALIGN_GRF:
            break;
        case ALIGN_2_GRF:
            break;
        case ALIGN_HWORD:
            break;
        case ALIGN_32WORD:
            break;
        case ALIGN_64WORD:
            break;
        default:
            REPORT_HEADER(options,false, "V%d's variable alignment is not a valid alignment value: %s", declID, declError.c_str());
    }

    REPORT_HEADER(options, !(var->alias_index != 0 &&
                             var->alias_index >=
                                 header->getVarCount() + numPreDefinedVars),
                  "V%d's alias index must point to a valid CISA variable index "
                  "between 0 and %d. Currently points to invalid index V%d: %s",
                  declID + numPreDefinedVars,
                  header->getVarCount() + numPreDefinedVars - 1,
                  var->alias_index, declError.c_str());

    /// INFO: Got rid of this verification check a long time ago.
    ///       Checking for unused variables is not that important
    ///       and it made it a lot simpler for refactoring to just
    ///       get rid of code that did the analysis required for it.
    #if 0
    REPORT_HEADER(options,m_used, "V%d is an unused variable in the CISA bytecode. Find out why it is declared.", declID);
    #endif
}

// get the start byte offset from the top level declare
static unsigned int getStartByteOffset(
    const print_format_provider_t* header,
    const var_info_t* var,
    unsigned int numPredefinedVars)
{
    unsigned int offset = 0;
    while (var->alias_index != 0)
    {
        offset += var->alias_offset;
        if (var->alias_index <= numPredefinedVars)
        {
            // predefined variables don't have aliases, so we can stop
            break;
        }
        else
        {
            var = header->getVar(var->alias_index-numPredefinedVars);
        }
    }
    return offset;
}

void vISAVerifier::verifyRegion(
    const CISA_INST* inst,
    unsigned i)
{
    ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
    /// Dataport instructions must be verified separately
    if (ISA_Inst_Data_Port == ISA_Inst_Table[opcode].type) return;

    ASSERT_USER(inst->opnd_array[i]->opnd_type == CISA_OPND_VECTOR, "Should only be verifying region on a vector operand");
    const vector_opnd& vect = getVectorOperand(inst, i);

    uint32_t                 operand_index    = vect.getOperandIndex();
    Common_ISA_Operand_Class operand_class    = vect.getOperandClass();

    unsigned dstIndex = getDstIndex(inst);

    unsigned numPreDefinedVars = Get_CISA_PreDefined_Var_Count();

    uint16_t region     = 0;
    uint8_t  row_offset = 0;
    uint8_t  col_offset = 0;

    switch (operand_class)
    {
        case OPERAND_GENERAL:
            region     = vect.opnd_val.gen_opnd.region;
            row_offset = vect.opnd_val.gen_opnd.row_offset;
            col_offset = vect.opnd_val.gen_opnd.col_offset;
            break;
        case OPERAND_INDIRECT:
            region = vect.opnd_val.indirect_opnd.region;
            break;
        default:
            return;
    }

    Common_ISA_Region_Val width    = (Common_ISA_Region_Val)((region >> 4) & 0xF);
    Common_ISA_Region_Val h_stride = (Common_ISA_Region_Val)((region >> 8) & 0xF);
    Common_ISA_Region_Val v_stride = (Common_ISA_Region_Val)((region     ) & 0xF);

    short v_stride_val = Common_ISA_Get_Region_Value(v_stride);
    short h_stride_val = Common_ISA_Get_Region_Value(h_stride);
    short width_val    = Common_ISA_Get_Region_Value(width   );

    /// INFO: Catch zero width, because we'll sigsegv otherwise.
    REPORT_INSTRUCTION(options,width_val, "CISA region has width of 0");

    uint8_t exec_sz = 0;
    switch (inst->getExecSize())
    {
    case EXEC_SIZE_1:  exec_sz = 1;  break;
    case EXEC_SIZE_2:  exec_sz = 2;  break;
    case EXEC_SIZE_4:  exec_sz = 4;  break;
    case EXEC_SIZE_8:  exec_sz = 8;  break;
    case EXEC_SIZE_16: exec_sz = 16; break;
    case EXEC_SIZE_32: exec_sz = 32; break;
    default: REPORT_INSTRUCTION(options, false, "Invalid execution size");
    }

    if (i == dstIndex)
    {
        REPORT_INSTRUCTION(options,0 != h_stride_val, "Horizontal Stride should not be 0 for a destination operand.");

        if (-1 != v_stride_val || -1 != width_val)
            cerr << "There's no reason, to set the vertical stride or width of a destination operand. They are ignored." << endl;

        // for dst set width = exec size and vstride = width * hstride, so their bound can be verified
        width_val = exec_sz;
        v_stride_val = width_val * h_stride_val;
    }

    if (width_val == 0)
    {
        return;
    }



    REPORT_INSTRUCTION(options,h_stride != REGION_NULL, "Horizontal Stride should not be REGION_NULL");

    switch (h_stride_val)
    {
        case 0:
        case 1:
        case 2:
        case 4:
            break;
        default: REPORT_INSTRUCTION(options,false, "Legal CISA region horizontal stride parameter values: {0, 1, 2, 4}.");
    }

    if (width != REGION_NULL)
    {
        switch (width_val)
        {
        case 1:
        case 2:
        case 4:
        case 8:
        case 16:
            break;
        default: REPORT_INSTRUCTION(options,false, "Legal CISA region width parameter values: {1, 2, 4, 8, 16}.");
        }

        REPORT_INSTRUCTION(options,exec_sz >= width_val, "Execution size must be greater than width.");
        if (exec_sz < width_val)
        {
            // no point in continuing verifiication
            return;
        }
    }

    if (v_stride != REGION_NULL)
    {
        switch (v_stride_val)
        {
        case 0:
        case 1:
        case 2:
        case 4:
        case 8:
        case 16:
        case 32:
            break;
        default: REPORT_INSTRUCTION(options,false, "Legal CISA region vertical stride parameter values: {0, 1, 2, 4, 8, 16, 32}.");
        }
    }

    if (operand_index >= numPreDefinedVars)
        if (operand_class == OPERAND_GENERAL)
        {
            const var_info_t*      var      = header->getVar(vect.getOperandIndex()-numPreDefinedVars);
            VISA_Type  isa_type = getVectorOperandType(isaHeader, header, vect);
            unsigned         VN_size  = CISATypeTable[isa_type].typeSize;

            uint16_t num_elements = var->num_elements;
            unsigned var_size = VN_size * num_elements;

            unsigned last_region_elt_byte;
            if (width_val != -1)
            {
                unsigned last_region_element = (((exec_sz / width_val) - 1) * v_stride_val) +
                    ((width_val - 1) * h_stride_val);
                last_region_elt_byte = (last_region_element * VN_size) + VN_size - 1;
            }
            else
            {
                // DstRegRegion with width 0 (width_val=-1)
                last_region_elt_byte =
                    (exec_sz - 1) * h_stride_val * VN_size + VN_size - 1;
            }

            REPORT_INSTRUCTION(options,(COMMON_ISA_GRF_REG_SIZE * 2u) > last_region_elt_byte,
                "CISA operand region access out of 2 GRF boundary (within %d bytes): %d",
                (COMMON_ISA_GRF_REG_SIZE * 2),
                last_region_elt_byte);

            unsigned firstElementIndex = row_offset * COMMON_ISA_GRF_REG_SIZE + col_offset;

            // check if the operand may touch more than 2 GRFs due to bad alignment
            unsigned startByte = getStartByteOffset(header, var, numPreDefinedVars) +
                row_offset * COMMON_ISA_GRF_REG_SIZE +
                col_offset * CISATypeTable[var->getType()].typeSize;
            unsigned endByte = startByte + last_region_elt_byte;
            unsigned startGRF = startByte / COMMON_ISA_GRF_REG_SIZE;
            unsigned endGRF = endByte / COMMON_ISA_GRF_REG_SIZE;
            REPORT_INSTRUCTION(options,endGRF == startGRF || endGRF == (startGRF + 1),
                "CISA operand accesses more than 2 GRF due to mis-alignment: start byte offset = %d, end byte offset = %d",
                startByte, endByte);


            for (int i = 0; i < exec_sz / width_val; i++)
            {
                for (int j = 0; j < width_val; j++)
                {
                    unsigned region_offset = firstElementIndex     +
                        (((i * v_stride_val)  +
                        (j * h_stride_val)) *
                        VN_size);

                    if (region_offset > var_size )
                    {
                        std::cout << "WARNING: CISA region and offset cause an out of bounds byte access: "<< region_offset << "\n";
                        std::cout << "An access should not exceed the declared allocation size: " << var_size << "\n";
                        std::cout << "  The access fails the following check to determine correct bounds (see CISA manual section 5.1 Region-based Addressing):\n";
                        std::cout << "  (row_offset * GRF_SIZE + col_offset) + (((i * v_stride) + (j * h_stride)) * type_size) <= type_size * num_elements:\n";
                        std::cout << "(" << (int)row_offset << " * "<< COMMON_ISA_GRF_REG_SIZE << " + "<< (int)col_offset << ") + (((" <<
                            i << " * " << v_stride_val <<") + (" << j <<" * " << h_stride_val << ")) * " << VN_size <<
                            ") <= " << VN_size << " * " << num_elements << std::endl;
                        std::cout << "Violating Instruction: "
                            << printInstruction(header, inst, options)
                            << endl;
                    }

#if 0
                    if (region_offset > var_size)
                    {
                        cerr << "i = " << i << " j = " << j << endl;
                        cerr << "Violating Instruction: "
                            << ((CisaInst*)this->getParent())->toString()
                            << endl;
                        exit(0);
                    }
#endif
                }
            }
        }


    if (3 == ISA_Inst_Table[opcode].n_srcs && (ISA_Opcode)opcode == ISA_LRP)
    {
        if (dstIndex == i)
        {
            REPORT_INSTRUCTION(options,1 == h_stride_val,
                    "For 3 source operand instructions, "
                    "the destination operand's horizontal stride must be 1.");
        }

        VISA_Type opnd_type = getVectorOperandType(isaHeader, header, vect);
        REPORT_INSTRUCTION(options, opnd_type == ISA_TYPE_F, "LRP instruction only supports sources and destination of type F.");

        if (dstIndex != i)
        {
            RegionDesc region(v_stride_val, width_val, h_stride_val);

            REPORT_INSTRUCTION(options, region.isScalar() || region.isContiguous(exec_sz),
                "For 3 source operand instructions, "
                "region must be either scalar or contiguous");
        }
    }
}

static bool isDWordType(VISA_Type type)
{
    return CISATypeTable[type].typeSize == 4;
}

// verify if this raw operand has the correct type as determined by typeFunc (false means illegal type)
// Many vISA messages require the raw operand to have certain types
void vISAVerifier::verifyRawOperandType(
    const CISA_INST* inst,
    const raw_opnd& opnd,
    bool (*typeFunc)(VISA_Type))
{
    unsigned numPreDefinedVars = Get_CISA_PreDefined_Var_Count();
    uint32_t variable_count    = header->getVarCount();

    uint32_t opnd_index  = opnd.index;

    if (opnd_index < variable_count + numPreDefinedVars &&
        numPreDefinedVars <= opnd_index)
    {
        const var_info_t* currVar = header->getVar(opnd_index-numPreDefinedVars);
        REPORT_INSTRUCTION(options,typeFunc(currVar->getType()), "Raw Operand %s has incorrect type %s",
            opnd.toString().c_str(), CISATypeTable[currVar->getType()].typeName);
    }
}

static VISA_Type getRawOperandType(
    const common_isa_header& isaHeader,
    const print_format_provider_t* header,
    const CISA_INST* inst,
    const raw_opnd& opnd)
{
    unsigned numPreDefinedVars = Get_CISA_PreDefined_Var_Count();
    uint32_t variable_count = header->getVarCount();

    uint32_t opnd_index = opnd.index;

    if (opnd_index < variable_count + numPreDefinedVars &&
        numPreDefinedVars <= opnd_index)
    {
        const var_info_t* currVar = header->getVar(opnd_index - numPreDefinedVars);
        return currVar->getType();
    }

    return ISA_TYPE_NUM;
}

void vISAVerifier::verifyRawOperand(
    const CISA_INST* inst, unsigned i)
{
    unsigned numPreDefinedVars = Get_CISA_PreDefined_Var_Count();
    uint32_t variable_count    = header->getVarCount();

    const raw_opnd& opnd = getRawOperand(inst, i);

    if ( numPreDefinedVars > opnd.index )
    {
        return; // allow raw operands to be predefined variables
    }

    uint32_t opnd_index  = opnd.index - numPreDefinedVars;
    uint16_t opnd_offset = opnd.offset;


    if (opnd_index < variable_count )
    {
        const var_info_t* currVar = header->getVar(opnd_index);
        unsigned totalOffset = opnd_offset;

        set<uint32_t> visitedAliasIndices;

        while (numPreDefinedVars <= currVar->alias_index)
        {
            if (visitedAliasIndices.find(currVar->alias_index) != visitedAliasIndices.end())
            {
                REPORT_INSTRUCTION(options,false, "Circular alias detected, alias index: %d", currVar->alias_index);
                break;
            }

            visitedAliasIndices.insert(currVar->alias_index);

            REPORT_INSTRUCTION(options,currVar->alias_index < variable_count + numPreDefinedVars,
                              "Aliased variable aliases to an invalid alias index. "
                              "Variable count: %d. invalid index: %d",
                              variable_count + numPreDefinedVars,
                              currVar->alias_index);

            totalOffset += currVar->alias_offset;
            if ( numPreDefinedVars > currVar->alias_index )
            {
                break; // // allow alias index to be predefined variables
            }
            currVar = header->getVar(currVar->alias_index-numPreDefinedVars);
        }

        if (currVar->getSize() >= GENX_GRF_REG_SIZ && totalOffset % GENX_GRF_REG_SIZ != 0)
        {
            // raw operand must be GRF-aligned if it's >= 1GRF
            REPORT_INSTRUCTION(options,false, "Raw operand should be GRF-aligned: Raw offset is %d", totalOffset);
        }

        if ( numPreDefinedVars <= currVar->alias_index )
        {
            REPORT_INSTRUCTION(options,totalOffset < currVar->num_elements * (unsigned)CISATypeTable[currVar->getType()].typeSize,
                    "A CISA raw operand's offset field must be within the allocated operand size. "
                    "Raw offset is %d, allocated number of elements is %d",
                    (int)opnd_offset, (int)currVar->num_elements);
        }
    }
    else
    {
        /// TODO: Verify predefined vars.
    }
}

static bool isReadWritePreDefinedVar(
    const common_isa_header& isaHeader, uint32_t index, uint32_t byteOffset)
{
    PreDefinedVarsInternal internalIndex = mapExternalToInternalPreDefVar(index);
    if (internalIndex == PreDefinedVarsInternal::ARG ||   internalIndex == PreDefinedVarsInternal::RET || internalIndex == PreDefinedVarsInternal::FE_SP ||
        internalIndex == PreDefinedVarsInternal::FE_FP || internalIndex == PreDefinedVarsInternal::CR0 || internalIndex == PreDefinedVarsInternal::DBG ||
        internalIndex == PreDefinedVarsInternal::VAR_NULL)
    {
        return true;
    }
    else if (internalIndex == PreDefinedVarsInternal::TSC && byteOffset == 16)
    {
        // tm0.4
        return true;
    }
    else if (internalIndex == PreDefinedVarsInternal::SR0 && (byteOffset == 8 || byteOffset == 12))
    {
        // sr0.2, sr0.3
        return true;
    }
    else
    {
        return false;
    }
}

void vISAVerifier::verifyVectorOperand(
    const CISA_INST* inst,
    unsigned i)
{
    unsigned numPreDefinedVars = Get_CISA_PreDefined_Var_Count();

    ISA_Opcode opcode = (ISA_Opcode)inst->opcode;

    const vector_opnd& opnd = getVectorOperand(inst, i);

    uint32_t                 operand_index    = opnd.getOperandIndex();
    Common_ISA_Operand_Class operand_class    = opnd.getOperandClass();
    VISA_Modifier      operand_modifier = opnd.getOperandModifier();


    unsigned dstIndex = getDstIndex(inst);

    if (inst->opnd_count <= 0)
    {
        REPORT_INSTRUCTION(options,false, "Incorrect number of operands loaded.");
        return;
    }

    if (operand_class != OPERAND_GENERAL && operand_class != OPERAND_INDIRECT)
    {
        REPORT_INSTRUCTION(options,operand_modifier == MODIFIER_NONE,
                          "Operand modifier for non-general and "
                          "non-indirect operands must be MODIFIER_NONE.");
    }

    switch (operand_modifier)
    {
        case MODIFIER_ABS:
        case MODIFIER_NEG:
        case MODIFIER_SAT:
        case MODIFIER_NEG_ABS:
             if (!isShiftOp(opcode))
             {
                 REPORT_INSTRUCTION(options,ISA_Inst_Table[opcode].type != ISA_Inst_Logic,
                                   "Only arithmetic modifiers should be used with "
                                   "arithmetic instruction general or indirect operands.");
             }
             break;
        case MODIFIER_NOT:
             REPORT_INSTRUCTION(options,ISA_Inst_Table[opcode].type == ISA_Inst_Logic,
                               "Only logical modifiers should be used with "
                               "logical instruction general or indirect operands.");
             break;
        default:
            break; // Prevent gcc warning
    }

    if (operand_class == OPERAND_IMMEDIATE)
    {
        /// Do checks for immediate operands here
        REPORT_INSTRUCTION(options,getVectorOperandType(isaHeader, header, opnd) != ISA_TYPE_BOOL,
            "Boolean types for immediate (constant literals) operands are disallowed.");
    }

    if (operand_class == OPERAND_GENERAL)
    {
        REPORT_INSTRUCTION(options,operand_index < header->getVarCount() + numPreDefinedVars, "Variable V%d is not declaired in CISA symtab.", operand_index);

        if (operand_index < header->getVarCount() + numPreDefinedVars &&
            numPreDefinedVars <= operand_index)
        {
            //var_info_t* var = &header->variables[operand_index-numPreDefinedVars];
            /// do some var verifications here.

            //VISA_Type isa_type_decl = (VISA_Type)((var->bit_properties) & 0xF);
            //VISA_Type isa_type_var  = getVectorOperandType(isaHeader, header, opnd);

        }
    }

    verifyRegion(inst, i);

    if (dstIndex == i)
    {
        REPORT_INSTRUCTION(options,operand_class != OPERAND_IMMEDIATE, "Constant Immediate operands are not allowed to be used as destination operands.");

        if (operand_class == OPERAND_GENERAL)
        {
            if (operand_index < numPreDefinedVars)
            {
                uint32_t byteOffset = opnd.opnd_val.gen_opnd.row_offset * COMMON_ISA_GRF_REG_SIZE +
                    opnd.opnd_val.gen_opnd.col_offset *
                    Get_VISA_Type_Size(getPredefinedVarType(mapExternalToInternalPreDefVar(operand_index)));
                REPORT_INSTRUCTION(options, isReadWritePreDefinedVar(isaHeader, operand_index, byteOffset), "Not allowed to write to a read only variable");
            }
        }
    }

    // ToDo: add bounds check for pre-defined variables
}

void vISAVerifier::verifyOperand(
    const CISA_INST* inst,
    unsigned i)
{
    MUST_BE_TRUE(header, "Argument Exception: argument header is NULL.");
    MUST_BE_TRUE(inst  , "Argument Exception: argument inst   is NULL.");
    MUST_BE_TRUE(inst->opnd_count > i, "No such operand, i, for instruction inst.");
    switch (getOperandType(inst, i))
    {
        case CISA_OPND_OTHER  : /* unable to verify some random primitive operand. */ break;
        case CISA_OPND_VECTOR : verifyVectorOperand(inst, i); break;
        case CISA_OPND_RAW    : verifyRawOperand   (inst, i); break;
        default               : MUST_BE_TRUE(false, "Invalid operand type.");
    }
}

void vISAVerifier::verifyInstructionSVM(
    const CISA_INST* inst)
{
    if (hasExecSize((ISA_Opcode)inst->opcode))
    {
        REPORT_INSTRUCTION(options, EXEC_SIZE_32 != inst->getExecSize(),
            "Execution size should not be SIMD32 for SVM messages.");
    }
}

void vISAVerifier::verifyInstructionMove(
    const CISA_INST* inst)
{
    ISA_Opcode opcode = (ISA_Opcode)inst->opcode;

    unsigned i = 0;

    uint8_t opext = opcode == ISA_FMINMAX ? getPrimitiveOperand<uint8_t>(inst, i++) : 0;

    const vector_opnd& dst  = getVectorOperand(inst, i++);
    const vector_opnd& src0 = getVectorOperand(inst, i++);

    Common_ISA_Operand_Class operand_class_dst  =  dst.getOperandClass();
    Common_ISA_Operand_Class operand_class_src0 = src0.getOperandClass();

    switch (opcode)
    {
        case ISA_MOV:
        {
             VISA_Type     dstType     = getVectorOperandType(isaHeader, header, dst );
             VISA_Type     src0Type    = getVectorOperandType(isaHeader, header, src0);
             VISA_Modifier dstModifier = dst.getOperandModifier();

             if (OPERAND_PREDICATE == operand_class_src0)
             {
                 REPORT_INSTRUCTION(options,EXEC_SIZE_1 == inst->getExecSize(),
                         "Execution size for a flag copy mov instruction should be 1, as it is a scalar copy.");
                 REPORT_INSTRUCTION(options, IsIntType(dstType) && dstType != ISA_TYPE_Q && dstType != ISA_TYPE_UQ,
                         "dst operand type for a flag copy mov instruction should be non-64-bit integer.");
                 REPORT_INSTRUCTION(options,CISATypeTable[dstType].typeSize >= CISATypeTable[src0Type].typeSize,
                         "dst operand type for a flag copy mov instruction should be "
                         "greater than or equal to the size of the src0 operand's type size.");
                 REPORT_INSTRUCTION(options,dstModifier != MODIFIER_SAT,
                          "saturation is not allowed for dst operands of a flag copy mov instruction");
                 REPORT_INSTRUCTION(options,inst->pred == 0,
                          "predication is not allowed for dst operands of a flag copy mov instruction");
             }

             /* Commented out because we are too lame to follow our own specification. */
             /*
             REPORT_INSTRUCTION(options,operand_class_dst == OPERAND_GENERAL  ||
                               operand_class_dst == OPERAND_INDIRECT,
                               "Destination operand of CISA MOV instruction only "
                               "supports general and indirect operands.");

             REPORT_INSTRUCTION(options,operand_class_src0 == OPERAND_GENERAL  ||
                               operand_class_src0 == OPERAND_INDIRECT ||
                               operand_class_src0 == OPERAND_IMMEDIATE,
                               "Source0 operand of CISA MOV instruction only "
                               "supports general, indirect, and immediate operands.");
                               */
             break;
        }

        case ISA_MOVS:
        {
             REPORT_INSTRUCTION(options,operand_class_dst == OPERAND_GENERAL ||
                               operand_class_dst == OPERAND_STATE,
                               "Destination operand of CISA MOVS instruction only "
                               "supports general and indirect operands.");

            REPORT_INSTRUCTION(options,operand_class_src0 == OPERAND_GENERAL  ||
                               operand_class_src0 == OPERAND_STATE    ||
                               operand_class_src0 == OPERAND_INDIRECT ||
                               operand_class_src0 == OPERAND_IMMEDIATE,
                               "Source0 operand of CISA MOVS instruction only "
                               "supports general, indirect, and immediate operands.");
            break;
        }
        case ISA_SETP:
        {
             REPORT_INSTRUCTION(options,operand_class_dst == OPERAND_PREDICATE,
                               "Destination operand of CISA SETP instruction only "
                               "supports predicate operands.");


            REPORT_INSTRUCTION(options,operand_class_src0 == OPERAND_GENERAL  ||
                              operand_class_src0 == OPERAND_INDIRECT ||
                              operand_class_src0 == OPERAND_IMMEDIATE,
                              "Source0 operand of CISA SETP instruction only "
                              "supports general, indirect, and immediate operands.");
             break;
        }
        case ISA_SEL:
        case ISA_FMINMAX:
        {
             REPORT_INSTRUCTION(options,opext <= 1, "FMINMAX opext must be either 0x0 or 0x1 (min or max).");

             const vector_opnd& src1 = getVectorOperand(inst, i++);
             Common_ISA_Operand_Class operand_class_src1 = src1.getOperandClass();

             REPORT_INSTRUCTION(options,operand_class_dst == OPERAND_GENERAL  ||
                               operand_class_dst == OPERAND_INDIRECT,
                               "Destination operand of CISA SEL instruction only "
                               "supports general and indirect operands.");

             REPORT_INSTRUCTION(options,operand_class_src0 == OPERAND_GENERAL  ||
                               operand_class_src0 == OPERAND_INDIRECT ||
                               operand_class_src0 == OPERAND_IMMEDIATE,
                               "Source0 operand of CISA SEL instruction only "
                               "supports general, indirect, and immediate operands.");

             REPORT_INSTRUCTION(options,operand_class_src1 == OPERAND_GENERAL  ||
                               operand_class_src1 == OPERAND_INDIRECT ||
                               operand_class_src1 == OPERAND_IMMEDIATE,
                               "Source1 operand of CISA SEL instruction only "
                               "supports general, indirect, and immediate operands.");
             break;
        }
        default: REPORT_INSTRUCTION(options,false, "Illegal Move Instruction Opcode: %d, %s.", opcode, ISA_Inst_Table[opcode].str);
    }
}

void vISAVerifier::verifyInstructionSync(
    const CISA_INST* inst)
{
    ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
    switch (opcode)
    {   case ISA_BARRIER            : return;
        case ISA_SAMPLR_CACHE_FLUSH : return;
        case ISA_WAIT               : return;
        case ISA_FENCE              : return;
        case ISA_YIELD              : return;
        case ISA_SBARRIER           : return;
        default: REPORT_INSTRUCTION(options,false, "Illegal Synchronization Instruction Opcode: %d, %s.", opcode, ISA_Inst_Table[opcode].str);
    }
}

void vISAVerifier::verifyInstructionControlFlow(
    const CISA_INST* inst)
{
    ISA_Opcode opcode = (ISA_Opcode)inst->opcode;

    switch (opcode)
    {
        case ISA_JMP:
        case ISA_CALL:
        {
            auto labelId = getPrimitiveOperand<uint16_t>(inst, 0);
            if (labelId >= header->getLabelCount())
            {
                REPORT_INSTRUCTION(options, false, "bad label id %d", labelId);
            }
            else
            {
                auto iter = labelDefs.find(labelId);
                if (iter == labelDefs.end())
                {
                    labelDefs[labelId] = false;
                }
                // nothing needs to be done if the label is already in the map
            }
            break;
        }
        case ISA_RET:
        case ISA_FRET:
        case ISA_IFCALL:
        case ISA_FADDR:     // no checks for now
             break;
        case ISA_SUBROUTINE:
        case ISA_LABEL:
        {
            auto labelId = getPrimitiveOperand<uint16_t>(inst, 0);
            if (labelId >= header->getLabelCount())
            {
                REPORT_INSTRUCTION(options, false, "bad label id %d", labelId);
            }
            else
            {
                auto iter = labelDefs.find(labelId);
                if (iter != labelDefs.end() && iter->second)
                {
                    REPORT_INSTRUCTION(options, false, "label is redefined");
                }
                labelDefs[labelId] = true;
            }
            break;
        }
        case ISA_FCALL:
        {
             break;
        }
        case ISA_SWITCHJMP:
        {
             /// TODO: Reenable this check if possible.
             #if 0
             ASSERT_USER(numLabels > 0 && numLabels < 33, "Number of labels in SWITCHJMP must be between 1 and 32");
             ASSERT_USER(IS_UNSIGNED_INT(indexOpnd->getType()), "index for SWITCHJMP must have unsigned type");
             ASSERT_USER(CISA_Opnd_Class( index) == OPERAND_GENERAL || CISA_Opnd_Class( index ) == OPERAND_INDIRECT ||
                     CISA_Opnd_Class( index) == OPERAND_IMMEDIATE,
                     "index for SWITCHJMP must be one of general/indirect/immediate operand.");
             ASSERT_USER( indexOpnd->isImm() || indexOpnd->isSrcRegRegion() && indexOpnd->asSrcRegRegion()->isScalar(),
                     "index for SWITCHJMP must be a scalar value");
             #endif
             break;
        }
        default: REPORT_INSTRUCTION(options,false, "Illegal Scalar Control Flow Instruction Opcode: %d, %s.", opcode, ISA_Inst_Table[opcode].str);
    }
}

void vISAVerifier::verifyInstructionMisc(
    const CISA_INST* inst)
{
    unsigned i = 0;
    unsigned numPreDefinedSurfs = Get_CISA_PreDefined_Surf_Count();

    ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
    switch (opcode)
    {
        case ISA_VME_IME:
        {
            uint8_t stream_mode = getPrimitiveOperand<uint8_t>(inst, i++);
            REPORT_INSTRUCTION(options,stream_mode == VME_STREAM_DISABLE ||
                              stream_mode == VME_STREAM_OUT     ||
                              stream_mode == VME_STREAM_IN      ||
                              stream_mode == VME_STREAM_IN_OUT,
                              "CISA ISA_VME_IME instruction uses illegal stream mode.");

            uint8_t search_ctrl = getPrimitiveOperand<uint8_t>(inst, i++);
            REPORT_INSTRUCTION(options,search_ctrl == VME_SEARCH_SINGLE_REF_SINGLE_REC_SINGLE_START ||
                              search_ctrl == VME_SEARCH_SINGLE_REF_SINGLE_REC_DUAL_START   ||
                              search_ctrl == VME_SEARCH_SINGLE_REF_DUAL_REC                ||
                              search_ctrl == VME_SEARCH_DUAL_REF_DUAL_REC,
                              "CISA ISA_VME_IME instruction uses illegal search ctrl.");

            i++; /// uni input
            i++; /// ime input

            uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
            REPORT_INSTRUCTION(options,(unsigned)surface < numPreDefinedSurfs + header->getSurfaceCount(),
                "CISA ISA_VME_IME uses undeclared surface.");

            break;
        }
        case ISA_VME_SIC:
        {
             i++; /// uni input
             i++; /// sic input

             uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
             REPORT_INSTRUCTION(options,(unsigned)surface < numPreDefinedSurfs + header->getSurfaceCount(),
                 "CISA ISA_VME_SIC uses undeclared surface.");

             break;
        }
        case ISA_VME_FBR:
        {
             getRawOperand(inst, i++);  // const raw_opnd& UNIInput
             getRawOperand(inst, i++);  // const raw_opnd& FBRInput

             uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
             REPORT_INSTRUCTION(options,(unsigned)surface < numPreDefinedSurfs + header->getSurfaceCount(),
                 "CISA ISA_VME_FBR uses undeclared surface.");

             Common_ISA_Operand_Class operand_class_FBRMbMode = getVectorOperand(inst, i++).getOperandClass();
             REPORT_INSTRUCTION(options,operand_class_FBRMbMode == OPERAND_GENERAL  ||
                               operand_class_FBRMbMode == OPERAND_INDIRECT ||
                               operand_class_FBRMbMode == OPERAND_IMMEDIATE,
                               "FBRMbMode operand of CISA VME_FBR instrution should "
                               "be either a general, indirect, or immediate operand.");

             Common_ISA_Operand_Class operand_class_FBRSubMbShape = getVectorOperand(inst, i++).getOperandClass();
             REPORT_INSTRUCTION(options,operand_class_FBRSubMbShape == OPERAND_GENERAL  ||
                               operand_class_FBRSubMbShape == OPERAND_INDIRECT ||
                               operand_class_FBRSubMbShape == OPERAND_IMMEDIATE,
                               "FBRSubMbShape operand of CISA VME_FBR instrution should "
                               "be either a general, indirect, or immediate operand.");

             Common_ISA_Operand_Class operand_class_FBRSubPredMode = getVectorOperand(inst, i++).getOperandClass();
             REPORT_INSTRUCTION(options,operand_class_FBRSubPredMode == OPERAND_GENERAL  ||
                               operand_class_FBRSubPredMode == OPERAND_INDIRECT ||
                               operand_class_FBRSubPredMode == OPERAND_IMMEDIATE,
                               "FBRSubPredMode operand of CISA VME_FBR instrution should "
                               "be either a general, indirect, or immediate operand.");

             getRawOperand(inst, i++);  // const raw_opnd& output

             break;
        }
        case ISA_VME_IDM:
        {
            break;
        }
        case ISA_LOC:
        {
            break;
        }
        case ISA_FILE:
        {
            break;
        }
        case ISA_RAW_SEND:
        {
            getPrimitiveOperand<uint8_t>(inst, i++);    // uint8_t modifier

            getPrimitiveOperand<uint8_t>(inst, i++);    // unit8_t exMsgDesc

            uint8_t numSrc = getPrimitiveOperand<uint8_t>(inst, i++);
            REPORT_INSTRUCTION(options,numSrc >= 1 && numSrc <= 15, "Number of message source GRFs must be between 1 and 15");

            uint8_t numDst = getPrimitiveOperand<uint8_t>(inst, i++);
            REPORT_INSTRUCTION(options,numDst <= 16, "Number of message destination GRFs must be between 0 and 16");

            const vector_opnd& desc = getVectorOperand(inst, i++);
            Common_ISA_Operand_Class operand_class_desc = desc.getOperandClass();
            REPORT_INSTRUCTION(options, operand_class_desc == OPERAND_GENERAL ||
                              operand_class_desc == OPERAND_INDIRECT ||
                              operand_class_desc == OPERAND_IMMEDIATE,
                              "desc operand of CISA RAW_SEND instrution should "
                              "be either a general, indirect, or immediate operand.");

            if (operand_class_desc == OPERAND_IMMEDIATE)
            {
                /// Structure describes a send message descriptor. Only expose
                /// several data fields; others are unnamed.
                struct MsgDescLayout {
                    uint32_t funcCtrl : 19;     // Function control (bit 0:18)
                    uint32_t headerPresent : 1; // Header present (bit 19)
                    uint32_t rspLength : 5;     // Response length (bit 20:24)
                    uint32_t msgLength : 4;     // Message length (bit 25:28)
                    uint32_t simdMode2 : 1;     // 16-bit input (bit 29)
                    uint32_t returnFormat : 1;  // 16-bit return (bit 30)
                    uint32_t EOT : 1;           // EOT
                };

                /// View a message descriptor in two different ways:
                /// - as a 32-bit unsigned integer
                /// - as a structure
                /// This simplifies the implementation of extracting subfields.
                union DescData {
                    uint32_t value;
                    MsgDescLayout layout;
                }udesc;

                udesc.value = desc.opnd_val.const_opnd._val.ival;

                REPORT_INSTRUCTION(options, numSrc >= udesc.layout.msgLength,
                                  "message length mismatch for raw send: msgLength (%d) must be not greater than numSrc (%d)",
                                  udesc.layout.msgLength, numSrc);
                REPORT_INSTRUCTION(options, numDst >= udesc.layout.rspLength,
                                  "response length mismatch for raw send: rspLength (%d) must be not greater than numDst (%d)",
                                  udesc.layout.rspLength, numDst);
            }


            /// src: todo
            /// dst: todo

            break;
        }
        case ISA_RAW_SENDS:
        {
            getPrimitiveOperand<uint8_t>(inst, i++);    // uint8_t modifier

            uint8_t numSrc0 = getPrimitiveOperand<uint8_t>(inst, i++);
            REPORT_INSTRUCTION(options,numSrc0 >= 1 && numSrc0 <= 32, "Number of source0 GRFs must be between 1 and 32");

            uint8_t numSrc1 = getPrimitiveOperand<uint8_t>(inst, i++);
            REPORT_INSTRUCTION(options, numSrc1 <= 32, "Number of source1 GRFs must be between 1 and 32");

            uint8_t numDst = getPrimitiveOperand<uint8_t>(inst, i++);
            REPORT_INSTRUCTION(options,numDst <= 32, "Number of message destination GRFs must be between 0 and 32");

            getPrimitiveOperand<uint8_t>(inst, i++);    // uint8_t exMsgDesc

            Common_ISA_Operand_Class operand_class_desc = getVectorOperand(inst, i++).getOperandClass();
            REPORT_INSTRUCTION(options,operand_class_desc == OPERAND_GENERAL  ||
                              operand_class_desc == OPERAND_INDIRECT ||
                              operand_class_desc == OPERAND_IMMEDIATE,
                              "desc operand of CISA RAW_SEND instrution should "
                              "be either a general, indirect, or immediate operand.");

            /// src0: todo
            /// src1: todo
            /// dst: todo

            break;
        }
        case ISA_3D_URB_WRITE:
        {
            uint8_t numOut = getPrimitiveOperand<uint8_t>(inst, i++);
            getPrimitiveOperand<uint16_t>(inst, i++);   // uint16_t channelMask
            uint16_t globalOff = getPrimitiveOperand<uint16_t>(inst, i++);

            REPORT_INSTRUCTION(options,!(numOut < 1 || numOut > 8) , "Valid range for num_out parameter of URB write is [1,8]");
            REPORT_INSTRUCTION(options,globalOff <= 2047, "Valid range for global_offset parameter of URB write is [0,2047]");
            REPORT_INSTRUCTION(options, inst->getExecSize() == EXEC_SIZE_8, "Only execution size of 8 is supported for URB write");

            break;
        }
        case ISA_LIFETIME:
        {
            uint8_t properties = getPrimitiveOperand<uint8_t>(inst, i++);
            getPrimitiveOperand<uint32_t>(inst, i++);   // uint32_t varId

            unsigned char type = (properties >> 4) & 0x3;

            if(type != OPERAND_GENERAL && type != OPERAND_ADDRESS && type != OPERAND_PREDICATE)
            {
                REPORT_INSTRUCTION(options, false, "Invalid encoding for register file");
            }

            break;
        }
        default: REPORT_INSTRUCTION(options,false, "Illegal Miscellaneous Flow Instruction Opcode: %d, %s.", opcode, ISA_Inst_Table[opcode].str);
    }
}

/// Returns true if this vector operand is an integer immediate constant and
/// its value fits into an expected type. Returns false, otherwise.
///
bool vISAVerifier::checkImmediateIntegerOpnd(
    const vector_opnd& opnd,
    VISA_Type expected_type)
{
    MUST_BE_TRUE(IsIntType(expected_type), "integer type expected");

    // Not an immediate.
    if (!opnd.isImmediate())
        return false;

    VISA_Type opnd_type = opnd.getImmediateType();
    if (!IsIntOrIntVecType(opnd_type))
    {
        return false;
    }

    bool is_vector = (opnd_type == ISA_TYPE_V || opnd_type == ISA_TYPE_UV);
    if (is_vector)
    {
        // if the operand type is unsigned, then always fits;
        // if the expected type is signed, then always fits;
        // otherwise, to be determined.
        if ((opnd_type == ISA_TYPE_UV) || IsSingedIntType(expected_type))
            return true;
    }

    // Retrieve the immediate integer value.
    int64_t val = 0;
    if (opnd_type == ISA_TYPE_Q || opnd_type == ISA_TYPE_UQ)
        val = typecastVals(&opnd.opnd_val.const_opnd._val.lval, opnd_type);
    else
        val = typecastVals(&opnd.opnd_val.const_opnd._val.ival, opnd_type);

    // The operand type is signed, and the expected type is unsigned.
    // All elements in this packed integer vector should be non-negative,
    // otherwise it does not fit.
    if (is_vector)
    {
        MUST_BE_TRUE(IsUnsignedIntType(expected_type), "unexpected signed type");

        // Truncate to 32 bit, each 4 bits consist of a vector component.
        // Sign bits are 3, 7, 11, 15, 19, 23, 27 and 31.
        return ((int32_t)val & 0x88888888) == 0;
    }

    switch (expected_type) {
    case ISA_TYPE_B:
        return (int8_t)val == val;
    case ISA_TYPE_UB:
        return (val >= 0) && ((uint8_t)val == val);
    case ISA_TYPE_W:
        return (int16_t)val == val;
    case ISA_TYPE_UW:
        return (val >= 0) && ((uint16_t)val == val);
    case ISA_TYPE_D:
        return (int32_t)val == val;
    case ISA_TYPE_UD:
        return (val >= 0) && ((uint32_t)val == val);
    case ISA_TYPE_Q:
    {
        // It does not fit into Q only if operand has type UQ and its value
        // exceeds the limit.
        if (opnd_type == ISA_TYPE_UQ)
        {
            // The actual value is to reprensent as an unsigned integer.
            uint64_t u_val = static_cast<uint64_t>(val);
            return u_val <= (uint64_t)std::numeric_limits<int64_t>::max();
        }

        // Fit for all other cases.
        return true;
    }
    case ISA_TYPE_UQ:
    {
        // UQ values fit.
        if (opnd_type == ISA_TYPE_UQ)
            return true;

        // Not UQ, but Q or smaller types.
        return val >= 0;
    }
    default:
        break;
    }
    return false;
}

void vISAVerifier::verifyInstructionArith(
    const CISA_INST* inst)
{
    ISA_Opcode opcode = (ISA_Opcode)inst->opcode;

    unsigned i = 0;
    const vector_opnd& dst = getVectorOperand(inst, i);
    VISA_Type         dstType = getVectorOperandType(isaHeader, header, dst);
    VISA_Modifier dstModifier = dst.getOperandModifier();
    auto platform = getGenxPlatform();

    REPORT_INSTRUCTION(options, dst.getOperandClass() == OPERAND_GENERAL ||
        dst.getOperandClass() == OPERAND_INDIRECT,
        "Destination of CISA arithmetic instruction should be general or indirect operand.");

    REPORT_INSTRUCTION(options, dstModifier == MODIFIER_NONE ||
        dstModifier == MODIFIER_SAT,
        "Illegal destination modifier for CISA arithmetic instruction.");

    // check if sat is allowed for this instruction
    if (dstModifier == MODIFIER_SAT)
    {
        switch (opcode)
        {
        case ISA_FRC:
        case ISA_LZD:
        case ISA_MOD:
        case ISA_MULH:
            REPORT_INSTRUCTION(options, false,
                "%s does not support saturation",
                ISA_Inst_Table[opcode].str);
            break;
        case ISA_DIV:
            REPORT_INSTRUCTION(options, dstType == ISA_TYPE_F || dstType == ISA_TYPE_HF,
                "%s does not support saturation on integer types.",
                ISA_Inst_Table[opcode].str);
        default:
            break; // Prevent gcc warning
        }
    }

    if (dstType == ISA_TYPE_DF)
    {
        if (opcode != ISA_MUL && opcode != ISA_ADD && opcode != ISA_MAD && opcode != ISA_DIV &&
            opcode != ISA_INV && opcode != ISA_SQRTM && opcode != ISA_SQRT && opcode != ISA_DIVM)
        {
            REPORT_INSTRUCTION(options, false,
                "Only mul/add/mad/div/inv/sqrtm/sqrt/divm are allowed to use double precision floating point operands.");
        }
    }


    /// check dst type is supported by the instruction
    switch (opcode)
    {
    case ISA_COS:
    case ISA_EXP:
    case ISA_LOG:
    case ISA_POW:
    case ISA_SIN:
        /// float and half float
        REPORT_INSTRUCTION(options, dstType == ISA_TYPE_F || dstType == ISA_TYPE_HF,
            "%s only supports single and half float type", ISA_Inst_Table[opcode].str);
        break;
    case ISA_RNDD:
    case ISA_RNDU:
    case ISA_RNDE:
    case ISA_RNDZ:
    case ISA_PLANE:
    case ISA_DP2:
    case ISA_DP3:
    case ISA_DP4:
    case ISA_DPH:
    case ISA_FRC:
    case ISA_LRP:
        /// float only
        REPORT_INSTRUCTION(options, dstType == ISA_TYPE_F, "%s only supports single float type", ISA_Inst_Table[opcode].str);
        break;
    case ISA_LINE:
        /// float or int only
        REPORT_INSTRUCTION(options, dstType == ISA_TYPE_F || IsIntType(dstType),
            "%s only supports integer and single precision float types", ISA_Inst_Table[opcode].str);
        break;
    case ISA_AVG:
    case ISA_MOD:
        /// int only
        REPORT_INSTRUCTION(options, dstType == ISA_TYPE_UD ||
            dstType == ISA_TYPE_D ||
            dstType == ISA_TYPE_UW ||
            dstType == ISA_TYPE_W ||
            dstType == ISA_TYPE_UB ||
            dstType == ISA_TYPE_B,
            "%s only supports integer type", ISA_Inst_Table[opcode].str);
        break;
    case ISA_LZD:
        /// UD only
        REPORT_INSTRUCTION(options, dstType == ISA_TYPE_UD, "lzd only supports UD type");
        break;
    case ISA_MULH:
    case ISA_DP4A:
        /// U or UD only
        REPORT_INSTRUCTION(options, dstType == ISA_TYPE_D || dstType == ISA_TYPE_UD,
            "%s only support D/UD dst type", ISA_Inst_Table[opcode].str);
        break;
    case ISA_SAD2:
    case ISA_SAD2ADD:
        /// dst must be w or uw
        REPORT_INSTRUCTION(options, dstType == ISA_TYPE_W || dstType == ISA_TYPE_UW, "sad2/sad2add only supports W/UW dst type.");
        REPORT_INSTRUCTION(options, getPlatformGeneration(getGenxPlatform()) != PlatformGen::GEN12, "sad2/sad2add is not supported on gen12.");
        break;
    case ISA_ADDC:
    case ISA_SUBB:
        REPORT_INSTRUCTION(options, dstType == ISA_TYPE_UD, "%s only supports single UD type", ISA_Inst_Table[opcode].str);
        break;
    default:
        REPORT_INSTRUCTION(options, dstType == ISA_TYPE_F || dstType == ISA_TYPE_DF || dstType == ISA_TYPE_HF || IsIntType(dstType), "%s has illegal dst type", ISA_Inst_Table[opcode].str);
    }

    // verify each source operand
    for (unsigned i = 0; i < ISA_Inst_Table[opcode].n_srcs; i++)
    {
        const vector_opnd& src = getVectorOperand(inst, i + ISA_Inst_Table[opcode].n_dsts); /// dst is at index 0, addc/subbb have two destinations
        VISA_Type         srcType = getVectorOperandType(isaHeader, header, src);
        VISA_Modifier srcModifier = src.getOperandModifier();

        REPORT_INSTRUCTION(options, srcModifier != MODIFIER_SAT && srcModifier != MODIFIER_NOT,
            "unsupported source modifier for arithmetic instruction");

        REPORT_INSTRUCTION(options, src.getOperandClass() == OPERAND_GENERAL ||
            src.getOperandClass() == OPERAND_INDIRECT ||
            src.getOperandClass() == OPERAND_IMMEDIATE,
            "source in arithmetic instruction must be general, indirect, or immediate");

        if (srcType == ISA_TYPE_DF)
        {
            if (opcode != ISA_MUL && opcode != ISA_ADD && opcode != ISA_MAD && opcode != ISA_DIV &&
                opcode != ISA_INV && opcode != ISA_SQRTM && opcode != ISA_SQRT && opcode != ISA_DIVM)
            {
                REPORT_INSTRUCTION(options, false,
                    "Only mul/add/mad/div/inv/sqrtm/sqrt/divm are allowed to use double precision floating point operands.");
            }
        }

        if (dstType == ISA_TYPE_F ||
            dstType == ISA_TYPE_DF ||
            dstType == ISA_TYPE_HF ||
            srcType == ISA_TYPE_DF ||
            srcType == ISA_TYPE_F ||
            srcType == ISA_TYPE_HF)
        {
            REPORT_INSTRUCTION(options, dstType == srcType ||
                (dstType == ISA_TYPE_F && srcType == ISA_TYPE_VF) ||
                (dstType == ISA_TYPE_F && srcType == ISA_TYPE_HF) ||
                (dstType == ISA_TYPE_HF && srcType == ISA_TYPE_F),
                "Arithmetic instructions that use single or double precision or half float types "
                "must use the same type for all of their operannds: dst(%s) and src%d(%s).",
                CISATypeTable[dstType].typeName, i, CISATypeTable[srcType].typeName);
        }
        else
        {
            /// source must have integer type
            REPORT_INSTRUCTION(options, IsIntType(srcType) ||
                (src.getOperandClass() == OPERAND_IMMEDIATE &&
                (srcType == ISA_TYPE_V || srcType == ISA_TYPE_UV)),
                "immediate src%d has %d type, and it must have integer type", i, srcType);
        }

        switch (opcode)
        {
        case ISA_SAD2:
        case ISA_SAD2ADD:
        {
            bool is_valid_imm = false;
            if (i == 2)
            {
                is_valid_imm = checkImmediateIntegerOpnd(src, ISA_TYPE_W) ||
                    checkImmediateIntegerOpnd(src, ISA_TYPE_UW);
            }
            else
            {
                is_valid_imm = checkImmediateIntegerOpnd(src, ISA_TYPE_B) ||
                    checkImmediateIntegerOpnd(src, ISA_TYPE_UB);
            }
            REPORT_INSTRUCTION(options, is_valid_imm ||
                (i == 2 && (srcType == ISA_TYPE_W || srcType == ISA_TYPE_UW)) ||
                (i <= 1 && (srcType == ISA_TYPE_B || srcType == ISA_TYPE_UB)),
                "sad2/sad2add only supports B/UB types for src0 and src1; W/UW for src2 (sad2add). src%d has invalid type.", i);
            break;
        }
        case ISA_MUL:
        case ISA_DIV:
            REPORT_INSTRUCTION(options, srcType != ISA_TYPE_Q && srcType != ISA_TYPE_UQ,
                "mul does not support Q/UQ types for src%d", i);
            break;
        case ISA_DIVM:
            REPORT_INSTRUCTION(options, srcType == ISA_TYPE_F || srcType == ISA_TYPE_DF || srcType == ISA_TYPE_VF,
                "ieee div does not support types for src%d, other than F/DF/VF", i);
            break;
        case ISA_SQRTM:
            REPORT_INSTRUCTION(options, srcType == ISA_TYPE_F || srcType == ISA_TYPE_DF || srcType == ISA_TYPE_VF,
                "ieee sqrt does not support types for src%d, other than F/DF/VF", i);
            break;
        case ISA_ADDC:
        case ISA_SUBB:
        {
            REPORT_INSTRUCTION(options, srcType == ISA_TYPE_UD || srcType == ISA_TYPE_UV,
                "%s src0 and src1 only supports single UD type", ISA_Inst_Table[opcode].str);
            break;
        }
        case ISA_DP4A:
            REPORT_INSTRUCTION(options, srcType == ISA_TYPE_D || srcType == ISA_TYPE_UD, "%s src0 and src1 only supports single UD type", ISA_Inst_Table[opcode].str);
            break;
        default:
            break; // Prevent gcc warning
        }
    }

    // check for IEEE macros support
    // !hasMadm() check
    if (platform == GENX_ICLLP || platform == GENX_TGLLP)
    {
        bool fOpcodeIEEE = (opcode == ISA_DIVM) || (opcode == ISA_SQRTM);
        bool dfOpcodeIEEE = fOpcodeIEEE || (opcode == ISA_INV) || (opcode == ISA_DIV) || (opcode == ISA_SQRT);
        REPORT_INSTRUCTION(options, !(dstType == ISA_TYPE_DF && dfOpcodeIEEE) && !(dstType == ISA_TYPE_F && fOpcodeIEEE),
            "IEEE instruction %s is not supported on %s platform", ISA_Inst_Table[opcode].str, platformString[platform]);
}

    // instruction specific checks
    if (opcode == ISA_LRP)
    {
        // for 3-src instructions, only support general/immediate operands
        REPORT_INSTRUCTION(options, inst->opnd_count == (ISA_Inst_Table[opcode].n_dsts + ISA_Inst_Table[opcode].n_srcs) &&
            getVectorOperand(inst, 0).getOperandClass() == OPERAND_GENERAL &&
            (getVectorOperand(inst, 1).getOperandClass() == OPERAND_GENERAL ||
                getVectorOperand(inst, 1).getOperandClass() == OPERAND_IMMEDIATE) &&
                (getVectorOperand(inst, 2).getOperandClass() == OPERAND_GENERAL ||
                    getVectorOperand(inst, 2).getOperandClass() == OPERAND_IMMEDIATE) &&
                    (getVectorOperand(inst, 3).getOperandClass() == OPERAND_GENERAL ||
                        getVectorOperand(inst, 3).getOperandClass() == OPERAND_IMMEDIATE),
            "lrp only supports general/immediate operands");

        //ToDo: should check for alignment here
    }
}

void vISAVerifier::verifyInstructionLogic(
    const CISA_INST* inst)
{
    ISA_Opcode opcode = (ISA_Opcode)inst->opcode;

    bool pred_logic = false;
    unsigned opend_count = inst->opnd_count;
    for (unsigned i = 0; i < opend_count; i++)
    {
        const vector_opnd& opnd = getVectorOperand(inst, i);
        VISA_Type opnd_type = getVectorOperandType(isaHeader, header, opnd);

        REPORT_INSTRUCTION(options,opnd.getOperandClass() != OPERAND_ADDRESS,
                          "Common ISA Logic instrutions are not allowed to have address operands.");

        REPORT_INSTRUCTION(options,!pred_logic || opnd_type == ISA_TYPE_BOOL,
                         "Operand type of logic operantion for predicate operands should all be BOOL "
                         "(ie if one operand is BOOL they all have to be BOOL).");

        if (opcode == ISA_ROR || opcode == ISA_ROL)
        {
            switch (opnd_type)
            {
            case ISA_TYPE_B:
            case ISA_TYPE_UB:
            case ISA_TYPE_UQ:
            case ISA_TYPE_Q:
                REPORT_INSTRUCTION(options, false,
                    "ror/rol does not support i8/i64 types");
            default:
                break;
            }
        }

        switch (opnd_type)
        {
            case ISA_TYPE_B:
            case ISA_TYPE_UB:
            case ISA_TYPE_W:
            case ISA_TYPE_D:
            case ISA_TYPE_UW:
            case ISA_TYPE_UD:
            case ISA_TYPE_V:
            case ISA_TYPE_UV:
                 break;
            case ISA_TYPE_UQ:
            case ISA_TYPE_Q:
                 REPORT_INSTRUCTION(options,opcode != ISA_FBL && opcode != ISA_FBH && opcode != ISA_CBIT,
                         "fbl/fbh/cbit does not support Q/UQ type.");
                 break;
            case ISA_TYPE_BOOL:
            {
                 pred_logic = true;
                 REPORT_INSTRUCTION(options,inst->pred == 0,
                                   "Predicate can not be used in logic operantion for predicate operands.");
                 break;
            }
            case ISA_TYPE_DF:
            case ISA_TYPE_F:
            case ISA_TYPE_HF:
            {
                 REPORT_INSTRUCTION(options,false,
                        "All operands of logic instructions must be of integral type! opnd %d has float type %d",
                        i, (int)(opnd_type));
                 break;
            }
            default:
            {
                REPORT_INSTRUCTION(options,false,
                         "All operands of logic instructions must be of integral type! opnd %d has unknow type %d",
                         i, (int)(opnd_type));
            }
        }
    }
}

void vISAVerifier::verifyInstructionCompare(
    const CISA_INST* inst)
{
    ///     opnd0              opnd1  opnd2 opnd3
    /// cmp.rel_op (exec_size) dst    src1  src2

    ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
    ASSERT_USER(ISA_CMP == opcode, "illegal opcode for compare instruction");

    for (unsigned i = 0; i < inst->opnd_count; i++)
    {
        if (i > 0)
        {
            Common_ISA_Operand_Class operand_class = getVectorOperand(inst, i).getOperandClass();
            switch (i)
            {
                case 1:
                    REPORT_INSTRUCTION(options,operand_class == OPERAND_PREDICATE || operand_class == OPERAND_GENERAL,
                                       "CISA compare instruction destination only supports a predicate operand.");
                    break;
                default:
                    REPORT_INSTRUCTION(options,operand_class != OPERAND_ADDRESS && operand_class != OPERAND_PREDICATE,
                                       "CISA compare instruction sources do not support address or predicate operands.");
                    break;
            }
        }
        else
        {
            /// TODO: Verify rel_op
        }
    }
}

void vISAVerifier::verifyInstructionAddress(
    const CISA_INST* inst)
{
    ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
    ASSERT_USER(ISA_ADDR_ADD == opcode, "Illegal opcode for address instruction.");

    for (unsigned i = 0; i < inst->opnd_count; i++)
    {
        Common_ISA_Operand_Class operand_class = getVectorOperand(inst, i).getOperandClass();

        if (0 == i)
        {
            REPORT_INSTRUCTION(options,operand_class == OPERAND_ADDRESS,
                              "CISA address instruction destination only supports an address operand.");
            continue;
        }

        REPORT_INSTRUCTION(options,operand_class != OPERAND_PREDICATE,
                          "CISA ADDR_ADD instruction sources do not support predicate operands.");
        if (1 == i)
        {
            if (operand_class == OPERAND_GENERAL)
            {
                uint32_t numPredefinedVars = Get_CISA_PreDefined_Var_Count();
                uint32_t varIndex = getVectorOperand(inst, i).opnd_val.gen_opnd.index;
                REPORT_INSTRUCTION(options, varIndex >= numPredefinedVars, "Can not take the address of a pre-defined variable");
            }
            else if (operand_class == OPERAND_STATE)
            {
                if (getVectorOperand(inst, i).getStateOpClass() == STATE_OPND_SURFACE)
                {
                    uint32_t numPredefinedSurfs = Get_CISA_PreDefined_Surf_Count();
                    uint32_t surfIndex = getVectorOperand(inst, i).opnd_val.state_opnd.index;
                    REPORT_INSTRUCTION(options, surfIndex >= numPredefinedSurfs, "Can not take the address of a pre-defined surface");
                }
            }
        }

        if (2 == i)
        {
            VISA_Type opnd_type = getVectorOperandType(isaHeader, header, getVectorOperand(inst, i));
            REPORT_INSTRUCTION(options,opnd_type == ISA_TYPE_B || opnd_type == ISA_TYPE_UB ||
                              opnd_type == ISA_TYPE_W || opnd_type == ISA_TYPE_UW,
                              "Data type of the second source of ADDR_ADD should be WORD or BYTE.");
        }

    }
}

void vISAVerifier::verifyInstructionSampler(const CISA_INST* inst)
{
    ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
    unsigned numPreDefinedSurfs = Get_CISA_PreDefined_Surf_Count();
    unsigned i = 0;

    switch (opcode)
    {
        case ISA_SAMPLE_UNORM:
        {
            uint8_t channel = getPrimitiveOperand<uint8_t>(inst, i++);
            REPORT_INSTRUCTION(options,channel, "CISA SAMPLER ISA_SAMPLE_UNORM instruction only accepts non-zero channel masks.");

            uint8_t sampler = getPrimitiveOperand<uint8_t>(inst, i++);
            REPORT_INSTRUCTION(options,sampler < header->getSamplerCount(), "CISA SAMPLER ISA_SAMPLE_UNORM instruction uses undeclared sampler.");

            uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
            REPORT_INSTRUCTION(options,0 != surface, "Surface T0 (the SLM surface) is not allowed for the SAMPLER instruction ISA_SAMPLE_UNORM.");
            REPORT_INSTRUCTION(options,surface-numPreDefinedSurfs< header->getSurfaceCount(),
                "CISA SAMPLER instruction ISA_SAMPLE_UNORM uses undefined surface.");

            Common_ISA_Operand_Class operand_class_uoff = getVectorOperand(inst, i++).getOperandClass();
            REPORT_INSTRUCTION(options,operand_class_uoff == OPERAND_GENERAL  ||
                               operand_class_uoff == OPERAND_INDIRECT ||
                               operand_class_uoff == OPERAND_IMMEDIATE,
                               "u_offset operand of CISA SAMPLER ISA_SAMPLE_UNORM instrution should "
                               "be either a general, indirect, or immediate operand.");

            Common_ISA_Operand_Class operand_class_voff = getVectorOperand(inst, i++).getOperandClass();
            REPORT_INSTRUCTION(options,operand_class_voff == OPERAND_GENERAL  ||
                               operand_class_voff == OPERAND_INDIRECT ||
                               operand_class_voff == OPERAND_IMMEDIATE,
                               "v_offset operand of CISA SAMPLER ISA_SAMPLE_UNORM instrution should "
                               "be either a general, indirect, or immediate operand.");


            Common_ISA_Operand_Class operand_class_udelta = getVectorOperand(inst, i++).getOperandClass();
            REPORT_INSTRUCTION(options,operand_class_udelta == OPERAND_GENERAL  ||
                               operand_class_udelta == OPERAND_INDIRECT ||
                               operand_class_udelta == OPERAND_IMMEDIATE,
                               "u_delta operand of CISA SAMPLER ISA_SAMPLE_UNORM instrution should "
                               "be either a general, indirect, or immediate operand.");

            Common_ISA_Operand_Class operand_class_vdelta = getVectorOperand(inst, i++).getOperandClass();
            REPORT_INSTRUCTION(options,operand_class_vdelta == OPERAND_GENERAL  ||
                               operand_class_vdelta == OPERAND_INDIRECT ||
                               operand_class_vdelta == OPERAND_IMMEDIATE,
                               "v_delta operand of CISA SAMPLER ISA_SAMPLE_UNORM instrution should "
                               "be either a general, indirect, or immediate operand.");

            /// dst: TODO

            break;
        }
        case ISA_LOAD:
        case ISA_SAMPLE:
        {
            uint8_t mod = getPrimitiveOperand<uint8_t>(inst, i++);

            if (opcode == ISA_SAMPLE)
            {
                uint8_t sampler = getPrimitiveOperand<uint8_t>(inst, i++);
                REPORT_INSTRUCTION(options,sampler < header->getSamplerCount(), "CISA SAMPLER SAMPLE/LOAD instruction uses undeclared sampler.");
            }

            uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
            REPORT_INSTRUCTION(options,0 != surface, "Surface T0 (the SLM surface) is not allowed for the SAMPLER instruction ISA_SAMPLE/ISA_LOAD.");
            REPORT_INSTRUCTION(options,surface-numPreDefinedSurfs < header->getSurfaceCount(),
                "CISA SAMPLER instruction ISA_SAMPLE/ISA_LOAD uses undefined surface.");

            uint8_t channel = mod & 0xF;
            REPORT_INSTRUCTION(options,channel, "CISA SAMPLER ISA_SAMPLE/ISA_LOAD instruction only accepts non-zero channel masks.");

            uint8_t SIMD_mode = (mod >> 4) & 0x3;
            if((unsigned)SIMD_mode == 0)
            {
                SIMD_mode = 8;
            }
            else if((unsigned)SIMD_mode == 1)
            {
                SIMD_mode = 16;
            }
            else
            {
                REPORT_INSTRUCTION(options,false, "Illegal SIMD mode used in ISA_SAMPLE/ISA_LOAD inst.");
            }

            /// u/r/v/dst:TODO

            break;
        }
        case ISA_3D_SAMPLE:
        {
            uint8_t value = getPrimitiveOperand<uint8_t>(inst, i++);
            bool pixelNullMask = (value & (1<<5)) != 0;
            bool cpsEnable = (value & (1<<6)) != 0;
            VISASampler3DSubOpCode subOp = VISASampler3DSubOpCode(value & 0x1F);

            if (pixelNullMask)
            {
                REPORT_INSTRUCTION(options, getGenxPlatform() >= GENX_SKL,
                                   "Pixel Null Mask Enable only valid for SKL+");
            }

            if (cpsEnable)
            {
                auto execSize = inst->getExecSize();

                REPORT_INSTRUCTION(options,
                                   execSize == EXEC_SIZE_8 || execSize == EXEC_SIZE_16,
                                   "CPS LOD Compensation Enable must be disabled unless"
                                   " SIMD mode is simd8* or simd16*");

                bool isSupportedOp = subOp == VISA_3D_SAMPLE ||
                                     subOp == VISA_3D_SAMPLE_B ||
                                     subOp == VISA_3D_SAMPLE_C ||
                                     subOp == VISA_3D_SAMPLE_B_C ||
                                     subOp == VISA_3D_LOD;

                REPORT_INSTRUCTION(options, isSupportedOp,
                                   "CPS LOD Compensation Enable is only supported for"
                                   " sample, sample_b, sample_b_c, sample_c and LOD");
            }
            break;
        }
        case ISA_3D_LOAD:
        case ISA_3D_GATHER4:
        {
            uint8_t value = getPrimitiveOperand<uint8_t>(inst, i++);
            bool pixelNullMask = (value >> 5) != 0;

            if (pixelNullMask)
            {
                REPORT_INSTRUCTION(options, getGenxPlatform() >= GENX_SKL,
                                   "Pixel Null Mask Enable only valid for SKL+");
            }
            break;
        }
        case ISA_3D_INFO:
        case ISA_3D_RT_WRITE:
        case ISA_3D_URB_WRITE:
        {
            // TODO: Add verification code for 3d specific opcodes
            break;
        }
        case ISA_AVS:
        {
            uint8_t channel = getPrimitiveOperand<uint8_t>(inst, i++);
            REPORT_INSTRUCTION(options,channel, "CISA SAMPLER AVS instruction only accepts non-zero channel masks.");

            uint8_t sampler = getPrimitiveOperand<uint8_t>(inst, i++);
            REPORT_INSTRUCTION(options,sampler < header->getSamplerCount(), "CISA VA MINMAXFILTER instruction uses undeclared sampler.");

            uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
            REPORT_INSTRUCTION(options,0 != surface, "Surface T0 (the SLM surface) is not allowed for the SAMPLER AVS instruction.");
            REPORT_INSTRUCTION(options,surface < numPreDefinedSurfs + header->getSurfaceCount(),
                "CISA VA instruction MINMAX uses undefined surface.");

            Common_ISA_Operand_Class operand_class_uoff = getVectorOperand(inst, i++).getOperandClass();
            REPORT_INSTRUCTION(options,operand_class_uoff == OPERAND_GENERAL  ||
                               operand_class_uoff == OPERAND_INDIRECT ||
                               operand_class_uoff == OPERAND_IMMEDIATE,
                               "u_offset operand of CISA SAMPLER AVS instrution should "
                               "be either a general, indirect, or immediate operand.");

            Common_ISA_Operand_Class operand_class_voff = getVectorOperand(inst, i++).getOperandClass();
            REPORT_INSTRUCTION(options,operand_class_voff == OPERAND_GENERAL  ||
                               operand_class_voff == OPERAND_INDIRECT ||
                               operand_class_voff == OPERAND_IMMEDIATE,
                               "v_offset operand of CISA SAMPLER AVS instrution should "
                               "be either a general, indirect, or immediate operand.");

            Common_ISA_Operand_Class operand_class_udelta = getVectorOperand(inst, i++).getOperandClass();
            REPORT_INSTRUCTION(options,operand_class_udelta == OPERAND_GENERAL  ||
                               operand_class_udelta == OPERAND_INDIRECT ||
                               operand_class_udelta == OPERAND_IMMEDIATE,
                               "u_delta operand of CISA SAMPLER AVS instrution should "
                               "be either a general, indirect, or immediate operand.");

            Common_ISA_Operand_Class operand_class_vdelta = getVectorOperand(inst, i++).getOperandClass();
            REPORT_INSTRUCTION(options,operand_class_vdelta == OPERAND_GENERAL  ||
                               operand_class_vdelta == OPERAND_INDIRECT ||
                               operand_class_vdelta == OPERAND_IMMEDIATE,
                               "v_delta operand of CISA SAMPLER AVS instrution should "
                               "be either a general, indirect, or immediate operand.");

            Common_ISA_Operand_Class operand_class_u2d = getVectorOperand(inst, i++).getOperandClass();
            REPORT_INSTRUCTION(options,operand_class_u2d == OPERAND_GENERAL  ||
                               operand_class_u2d == OPERAND_INDIRECT ||
                               operand_class_u2d == OPERAND_IMMEDIATE,
                               "u2d operand of CISA SAMPLER AVS instrution should "
                               "be either a general, indirect, or immediate operand.");

            Common_ISA_Operand_Class operand_class_groupid = getVectorOperand(inst, i++).getOperandClass();
            REPORT_INSTRUCTION(options,operand_class_groupid == OPERAND_GENERAL  ||
                               operand_class_groupid == OPERAND_INDIRECT ||
                               operand_class_groupid == OPERAND_IMMEDIATE,
                               "groupid operand of CISA SAMPLER AVS instrution should "
                               "be either a general, indirect, or immediate operand.");

            Common_ISA_Operand_Class operand_class_verticalBlockNumber = getVectorOperand(inst, i++).getOperandClass();
            REPORT_INSTRUCTION(options,operand_class_verticalBlockNumber == OPERAND_GENERAL  ||
                               operand_class_verticalBlockNumber == OPERAND_INDIRECT ||
                               operand_class_verticalBlockNumber == OPERAND_IMMEDIATE,
                               "verticalBlockNumber operand of CISA SAMPLER AVS instrution should "
                               "be either a general, indirect, or immediate operand.");

            uint8_t cntrl = ((getPrimitiveOperand<uint8_t>(inst, i++)) & 0xF);

            switch (cntrl)
            {
                case 0:
                case 1:
                case 2:
                case 3: break;
                default: REPORT_INSTRUCTION(options,false, "cntrl for CISA SAMPLER AVS intruction should be a "
                                                  "value 0-3 (8/16bit full/chrominance down sample).");
            }

            Common_ISA_Operand_Class operand_class_v2d = getVectorOperand(inst, i++).getOperandClass();
            REPORT_INSTRUCTION(options,operand_class_v2d == OPERAND_GENERAL  ||
                               operand_class_v2d == OPERAND_INDIRECT ||
                               operand_class_v2d == OPERAND_IMMEDIATE,
                               "v2d operand of CISA SAMPLER AVS instrution should "
                               "be either a general, indirect, or immediate operand.");

            uint8_t execMode = ((getPrimitiveOperand<uint8_t>(inst, i++)) & 0xF);

            switch (execMode)
            {
                case 0:
                case 1:
                case 2:
                case 3: break;
                default: REPORT_INSTRUCTION(options,false, "execMode for CISA SAMPLER AVS intruction should "
                                                  "be a value 0-3 (16x4, 8x4, 16x8, or 4x4).");
            }

            Common_ISA_Operand_Class operand_class_iefbypass = getVectorOperand(inst, i++).getOperandClass();
            REPORT_INSTRUCTION(options,operand_class_iefbypass == OPERAND_GENERAL ||
                               operand_class_iefbypass == OPERAND_INDIRECT ||
                               operand_class_iefbypass == OPERAND_IMMEDIATE,
                               "iefbypass operand of CISA SAMPLER AVS instruction should "
                               "be either a general, indirect, or immediate operand.");

            /// dst: TODO

            break;
        }
        case ISA_VA:
        {
            ISA_VA_Sub_Opcode subOpcode = (ISA_VA_Sub_Opcode)getPrimitiveOperand<uint8_t>(inst, i++);
             switch (subOpcode)
             {
                 case MINMAX_FOPCODE:
                 {
                     uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
                     REPORT_INSTRUCTION(options,0 != surface, "Surface T0 (the SLM surface) is not allowed for VA instructions.");
                     REPORT_INSTRUCTION(options,surface-numPreDefinedSurfs< header->getSurfaceCount(),
                         "CISA VA instruction MINMAX uses undefined surface.");

                     Common_ISA_Operand_Class operand_class_uoff = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_uoff == OPERAND_GENERAL  ||
                                        operand_class_uoff == OPERAND_INDIRECT ||
                                        operand_class_uoff == OPERAND_IMMEDIATE,
                                        "u_offset operand of CISA MINMAX instrution should "
                                        "be either a general, indirect, or immediate operand.");

                     Common_ISA_Operand_Class operand_class_voff = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_voff == OPERAND_GENERAL  ||
                                        operand_class_voff == OPERAND_INDIRECT ||
                                        operand_class_voff == OPERAND_IMMEDIATE,
                                        "v_offset operand of CISA MINMAX instrution should "
                                        "be either a general, indirect, or immediate operand.");

                     /// mmf mode
                     const vector_opnd& mmf = getVectorOperand(inst, i++);
                     if (mmf.getOperandClass() == OPERAND_IMMEDIATE)
                     {
                         unsigned val = mmf.opnd_val.const_opnd._val.ival;
                         ASSERT_USER(val <= VA_MIN_ENABLE, "MINMAX MMF Mode operand out of range.");
                     }

                     /// dst: TODO

                     break;
                 }
                 case MINMAXFILTER_FOPCODE:
                 {
                     uint8_t sampler = getPrimitiveOperand<uint8_t>(inst, i++);
                     REPORT_INSTRUCTION(options,sampler < header->getSamplerCount(), "CISA VA MINMAXFILTER instruction uses undeclared sampler.");

                     uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
                     REPORT_INSTRUCTION(options,0 != surface, "Surface T0 (the SLM surface) is not allowed for VA instructions.");
                     REPORT_INSTRUCTION(options,surface-numPreDefinedSurfs< header->getSurfaceCount(),
                         "CISA VA instruction MINMAXFILTER uses undefined surface.");

                     Common_ISA_Operand_Class operand_class_uoff = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_uoff == OPERAND_GENERAL  ||
                                        operand_class_uoff == OPERAND_INDIRECT ||
                                        operand_class_uoff == OPERAND_IMMEDIATE,
                                        "u_offset operand of CISA MINMAXFILTER instrution should "
                                        "be either a general, indirect, or immediate operand.");

                     Common_ISA_Operand_Class operand_class_voff = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_voff == OPERAND_GENERAL  ||
                                        operand_class_voff == OPERAND_INDIRECT ||
                                        operand_class_voff == OPERAND_IMMEDIATE,
                                        "v_offset operand of CISA MINMAXFILTER instrution should "
                                        "be either a general, indirect, or immediate operand.");

                     getPrimitiveOperand<uint8_t>(inst, i++); // uint8_t cntrl, &0xf
                     getPrimitiveOperand<uint8_t>(inst, i++); // uint8_t execMode, &0xf

                     /// mmf mode
                     const vector_opnd& mmf = getVectorOperand(inst, i++);
                     if (mmf.getOperandClass() == OPERAND_IMMEDIATE)
                     {
                         unsigned val = mmf.opnd_val.const_opnd._val.ival;
                         ASSERT_USER(val <= VA_MIN_ENABLE, "MINMAXFILTER MMF Mode operand out of range.");
                     }

                     /// dst

                     break;
                 }
                 case BoolCentroid_FOPCODE:
                 case Centroid_FOPCODE:
                 {
                     uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
                     REPORT_INSTRUCTION(options,0 != surface, "Surface T0 (the SLM surface) is not allowed for VA instructions.");
                     REPORT_INSTRUCTION(options,surface-numPreDefinedSurfs< header->getSurfaceCount(),
                         "CISA VA VA CENTROID/BOOLCENTROID instruction MINMAX uses undefined surface.");

                     Common_ISA_Operand_Class operand_class_uoff = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_uoff == OPERAND_GENERAL  ||
                                        operand_class_uoff == OPERAND_INDIRECT ||
                                        operand_class_uoff == OPERAND_IMMEDIATE,
                                        "u_offset operand of CISA VA CENTROID/BOOLCENTROID instrution should "
                                        "be either a general, indirect, or immediate operand.");

                     Common_ISA_Operand_Class operand_class_voff = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_voff == OPERAND_GENERAL  ||
                                        operand_class_voff == OPERAND_INDIRECT ||
                                        operand_class_voff == OPERAND_IMMEDIATE,
                                        "v_offset operand of CISA VA CENTROID/BOOLCENTROID instrution should "
                                        "be either a general, indirect, or immediate operand.");

                     Common_ISA_Operand_Class operand_class_vsize = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_vsize == OPERAND_GENERAL  ||
                                        operand_class_vsize == OPERAND_INDIRECT ||
                                        operand_class_vsize == OPERAND_IMMEDIATE,
                                        "v_size operand of CISA VA CENTROID/BOOLCENTROID instrution should "
                                        "be either a general, indirect, or immediate operand.");

                     /// h size
                     if (subOpcode == BoolCentroid_FOPCODE)
                     {
                         Common_ISA_Operand_Class operand_class_hsize = getVectorOperand(inst, i++).getOperandClass();
                         REPORT_INSTRUCTION(options,operand_class_hsize == OPERAND_GENERAL  ||
                                            operand_class_hsize == OPERAND_INDIRECT ||
                                            operand_class_hsize == OPERAND_IMMEDIATE,
                                            "h_size operand of CISA VA CENTROID/BOOLCENTROID instrution should "
                                            "be either a general, indirect, or immediate operand.");
                     }

                     /// dst: TODO

                     break;
                 }
                 case Convolve_FOPCODE:
                 case Dilate_FOPCODE:
                 case ERODE_FOPCODE:
                 {
                     uint8_t sampler = getPrimitiveOperand<uint8_t>(inst, i++);
                     REPORT_INSTRUCTION(options,sampler < header->getSamplerCount(), "CISA VA CONVOLVE/ERODE/DILATE instruction uses undeclared sampler.");

                     uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
                     REPORT_INSTRUCTION(options,0 != surface, "Surface T0 (the SLM surface) is not allowed for VA instructions.");
                     REPORT_INSTRUCTION(options,surface-numPreDefinedSurfs < header->getSurfaceCount(),
                         "CISA VA CONVOLVE/ERODE/DILATE instruction MINMAX uses undefined surface.");

                     Common_ISA_Operand_Class operand_class_uoff = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_uoff == OPERAND_GENERAL  ||
                                        operand_class_uoff == OPERAND_INDIRECT ||
                                        operand_class_uoff == OPERAND_IMMEDIATE,
                                        "u_offset operand of CISA CONVOLVE/ERODE/DILATE instrution should "
                                        "be either a general, indirect, or immediate operand.");

                     Common_ISA_Operand_Class operand_class_voff = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_voff == OPERAND_GENERAL  ||
                                        operand_class_voff == OPERAND_INDIRECT ||
                                        operand_class_voff == OPERAND_IMMEDIATE,
                                        "v_offset operand of CISA CONVOLVE/ERODE/DILATE instrution should "
                                        "be either a general, indirect, or immediate operand.");

                     /// uint8_t execMode   = ((getPrimitiveOperand<uint8_t>(inst, i)) & 0x3);
                     /// uint8_t regionSize = ((getPrimitiveOperand<uint8_t>(inst, i)) & 0xC) >> 0x2;
                     /// dst: TODO

                     break;
                 }
                 default:
                     REPORT_INSTRUCTION(options,false, "Invalid VA sub-opcode: %d.", subOpcode);
             }

             break;
        }
        case ISA_VA_SKL_PLUS:
        {
            ISA_VA_Sub_Opcode subOpcode = (ISA_VA_Sub_Opcode)getPrimitiveOperand<uint8_t>(inst, i++);
            switch (subOpcode)
            {
                case VA_OP_CODE_LBP_CORRELATION:
                {
                     uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
                     REPORT_INSTRUCTION(options,0 != surface, "Surface T0 (the SLM surface) is not allowed for VA++ instructions.");
                     REPORT_INSTRUCTION(options,surface-numPreDefinedSurfs < header->getSurfaceCount(),
                         "CISA VA++ instruction LBP Correlation uses undefined surface.");

                     Common_ISA_Operand_Class operand_class_uoff = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_uoff == OPERAND_GENERAL  ||
                                        operand_class_uoff == OPERAND_INDIRECT ||
                                        operand_class_uoff == OPERAND_IMMEDIATE,
                                        "u_offset operand of CISA LBP Correlation instrution "
                                        "should be either a general, indirect, or immediate operand.");

                     Common_ISA_Operand_Class operand_class_voff = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_voff == OPERAND_GENERAL  ||
                                        operand_class_voff == OPERAND_INDIRECT ||
                                        operand_class_voff == OPERAND_IMMEDIATE,
                                        "v_offset operand of CISA LBP Correlation instrution "
                                        "should be either a general, indirect, or immediate operand.");

                     Common_ISA_Operand_Class operand_class_disparity = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_disparity == OPERAND_GENERAL  ||
                                        operand_class_disparity == OPERAND_INDIRECT ||
                                        operand_class_disparity == OPERAND_IMMEDIATE,
                                        "Disparity operand of CISA LBP Correlation instrution "
                                        "should be either a general, indirect, or immediate operand.");

                     /// dst
                     /// TODO: Check dst for type W for for GRF alignment

                     break;
                }
                case VA_OP_CODE_1PIXEL_CONVOLVE:
                case VA_OP_CODE_1D_CONVOLVE_VERTICAL:
                case VA_OP_CODE_1D_CONVOLVE_HORIZONTAL:
                {
                     uint8_t sampler = getPrimitiveOperand<uint8_t>(inst, i++);
                     REPORT_INSTRUCTION(options,sampler < header->getSamplerCount(), "CISA VA++ instruction uses undeclared sampler.");

                     uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
                     REPORT_INSTRUCTION(options,0 != surface, "Surface T0 (the SLM surface) is not allowed for VA++ instructions.");
                     REPORT_INSTRUCTION(options,surface-numPreDefinedSurfs < header->getSurfaceCount(),
                         "CISA VA++ instruction uses undefined surface.");

                     Common_ISA_Operand_Class operand_class_uoff = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_uoff == OPERAND_GENERAL  ||
                                        operand_class_uoff == OPERAND_INDIRECT ||
                                        operand_class_uoff == OPERAND_IMMEDIATE,
                                        "u_offset operand of CISA VA++ instrution "
                                        "should be either a general, indirect, or immediate operand.");

                     Common_ISA_Operand_Class operand_class_voff = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_voff == OPERAND_GENERAL  ||
                                        operand_class_voff == OPERAND_INDIRECT ||
                                        operand_class_voff == OPERAND_IMMEDIATE,
                                        "v_offset operand of CISA VA++ instrution "
                                        "should be either a general, indirect, or immediate operand.");

                     uint8_t mode = ((getPrimitiveOperand<uint8_t>(inst, i++)) & 0xF);

                     switch (mode & 0x3)
                     {
                         case 0: break;
                         case 2: break;
                         case 3: break;
                         default: REPORT_INSTRUCTION(options,false, "Invalid mode field for CISA VA++ instruction. "
                                                           "Only 4x16, 1x16, and 1x1 (in the case of "
                                                           "1 pixel convolve) are supported.");
                     }

                     break;
                }
                case VA_OP_CODE_LBP_CREATION:
                {
                     uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
                     REPORT_INSTRUCTION(options,0 != surface, "Surface T0 (the SLM surface) is not allowed for VA++ instructions.");
                     REPORT_INSTRUCTION(options,surface-numPreDefinedSurfs < header->getSurfaceCount(),
                         "CISA LBP Creation VA++ instruction uses undefined surface.");

                     Common_ISA_Operand_Class operand_class_uoff = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_uoff == OPERAND_GENERAL  ||
                                        operand_class_uoff == OPERAND_INDIRECT ||
                                        operand_class_uoff == OPERAND_IMMEDIATE,
                                        "u_offset operand of CISA LBP Creation VA++ instrution "
                                        "should be either a general, indirect, or immediate operand.");

                     Common_ISA_Operand_Class operand_class_voff = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_voff == OPERAND_GENERAL  ||
                                        operand_class_voff == OPERAND_INDIRECT ||
                                        operand_class_voff == OPERAND_IMMEDIATE,
                                        "v_offset operand of CISA LBP Creation VA++ instrution "
                                        "should be either a general, indirect, or immediate operand.");

                     uint8_t mode = ((getPrimitiveOperand<uint8_t>(inst, i++)) & 0xF);

                     switch (mode & 0x3)
                     {
                         case 0: break;
                         case 1: break;
                         case 2: break;
                         default: REPORT_INSTRUCTION(options,false, "Invalid mode field for CISA LBP Creation VA++ instruction. "
                                                           "Only 5x5, 3x3, or both modes are supported.");
                     }

                     /// dst
                     /// TODO: Checks for modes/types/decls

                     break;
                }
                case VA_OP_CODE_FLOOD_FILL:
                {
                     getPrimitiveOperand<uint8_t>(inst, i++);   // uint8_t is8Connect

                     i++; /// h-mask

                     Common_ISA_Operand_Class operand_class_vmask_left = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_vmask_left == OPERAND_GENERAL  ||
                                        operand_class_vmask_left == OPERAND_INDIRECT ||
                                        operand_class_vmask_left == OPERAND_IMMEDIATE,
                                        "Pixel direction v-mask left of CISA VA++ FloodFill instruction only "
                                        "supports general, indirect, and immediate operands.");

                     Common_ISA_Operand_Class operand_class_vmask_right = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_vmask_right == OPERAND_GENERAL  ||
                                        operand_class_vmask_right == OPERAND_INDIRECT ||
                                        operand_class_vmask_right == OPERAND_IMMEDIATE,
                                        "Pixel direction v-mask right of CISA VA++ FloodFill instruction only "
                                        "supports general, indirect, and immediate operands.");

                     Common_ISA_Operand_Class operand_class_loopcount = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_loopcount == OPERAND_GENERAL  ||
                                        operand_class_loopcount == OPERAND_INDIRECT ||
                                        operand_class_loopcount == OPERAND_IMMEDIATE,
                                        "loop_count of Common ISA sample instrution is invalid type.");

                     break;
                }
                case VA_OP_CODE_CORRELATION_SEARCH:
                {
                     uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
                     REPORT_INSTRUCTION(options,0 != surface, "Surface T0 (the SLM surface) is not allowed for VA++ instructions.");
                     REPORT_INSTRUCTION(options,surface < numPreDefinedSurfs + header->getSurfaceCount(),
                         "CISA VA++ instruction uses undefined surface.");

                     Common_ISA_Operand_Class operand_class_uoff = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_uoff == OPERAND_GENERAL  ||
                                        operand_class_uoff == OPERAND_INDIRECT ||
                                        operand_class_uoff == OPERAND_IMMEDIATE,
                                        "u_offset of Common ISA Meida LD/ST instrution "
                                        "should not be address or predicate operand.");

                     Common_ISA_Operand_Class operand_class_voff = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_voff == OPERAND_GENERAL  ||
                                        operand_class_voff == OPERAND_INDIRECT ||
                                        operand_class_voff == OPERAND_IMMEDIATE,
                                        "v_offset of Common ISA Meida LD/ST instrution "
                                        "should not be address or predicate operand.");

                     Common_ISA_Operand_Class operand_class_verticalOrigin = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_verticalOrigin == OPERAND_GENERAL  ||
                                        operand_class_verticalOrigin == OPERAND_INDIRECT ||
                                        operand_class_verticalOrigin == OPERAND_IMMEDIATE,
                                        "CISA VA++ instruction Correlation search operand verticalOrigin "
                                        "can only be of operand class general, indirect, or immediate.");

                     Common_ISA_Operand_Class operand_class_horizontalOrigin = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_horizontalOrigin == OPERAND_GENERAL  ||
                                        operand_class_horizontalOrigin == OPERAND_INDIRECT ||
                                        operand_class_horizontalOrigin == OPERAND_IMMEDIATE,
                                        "CISA VA++ instruction Correlation search operand horizontalOrigin "
                                        "can only be of operand class general, indirect, or immediate.");

                     Common_ISA_Operand_Class operand_class_xDirectionSize = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_xDirectionSize == OPERAND_GENERAL  ||
                                        operand_class_xDirectionSize == OPERAND_INDIRECT ||
                                        operand_class_xDirectionSize == OPERAND_IMMEDIATE,
                                        "CISA VA++ instruction Correlation search operand xDirectionSize "
                                        "can only be of operand class general, indirect, or immediate.");

                     Common_ISA_Operand_Class operand_class_yDirectionSize = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_yDirectionSize == OPERAND_GENERAL  ||
                                        operand_class_yDirectionSize == OPERAND_INDIRECT ||
                                        operand_class_yDirectionSize == OPERAND_IMMEDIATE,
                                        "CISA VA++ instruction Correlation search operand yDirectionSize "
                                        "can only be of operand class general, indirect, or immediate.");

                     Common_ISA_Operand_Class operand_class_xDirectionSearchSize = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_xDirectionSearchSize == OPERAND_GENERAL  ||
                                        operand_class_xDirectionSearchSize == OPERAND_INDIRECT ||
                                        operand_class_xDirectionSearchSize == OPERAND_IMMEDIATE,
                                        "CISA VA++ instruction Correlation search operand xDirectionSearchSize "
                                        "can only be of operand class general, indirect, or immediate.");

                     Common_ISA_Operand_Class operand_class_yDirectionSearchSize = getVectorOperand(inst, i++).getOperandClass();
                     REPORT_INSTRUCTION(options,operand_class_yDirectionSearchSize == OPERAND_GENERAL  ||
                                        operand_class_yDirectionSearchSize == OPERAND_INDIRECT ||
                                        operand_class_yDirectionSearchSize == OPERAND_IMMEDIATE,
                                        "CISA VA++ instruction Correlation search operand yDirectionSearchSize "
                                        "can only be of operand class general, indirect, or immediate.");

                     /// dst
                     /// TODO: Check dst type that it is only Type_D

                     break;
                }
                case ISA_HDC_CONV:
                case ISA_HDC_ERODE:
                case ISA_HDC_DILATE:
                case ISA_HDC_LBPCORRELATION:
                case ISA_HDC_LBPCREATION:
                case ISA_HDC_MMF:
                case ISA_HDC_1PIXELCONV:
                case ISA_HDC_1DCONV_H:
                case ISA_HDC_1DCONV_V:
                    {
                        if (subOpcode != ISA_HDC_LBPCORRELATION &&
                            subOpcode !=  ISA_HDC_LBPCREATION)
                        {
                            /// sampler
                            uint8_t sampler = getPrimitiveOperand<uint8_t>(inst, i++);
                            REPORT_INSTRUCTION(options,sampler < header->getSamplerCount(), "CISA VA++ instruction uses undeclared sampler.");
                        }

                        //input surface
                        uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
                        REPORT_INSTRUCTION(options,0 != surface, "Surface T0 (the SLM surface) is not allowed for VA++ instructions.");
                        REPORT_INSTRUCTION(options,surface < numPreDefinedSurfs + header->getSurfaceCount(),
                            "CISA VA++ instruction uses undefined surface.");

                        //u_offset
                        Common_ISA_Operand_Class operand_class_uoff = getVectorOperand(inst, i++).getOperandClass();
                        REPORT_INSTRUCTION(options,operand_class_uoff == OPERAND_GENERAL  ||
                                           operand_class_uoff == OPERAND_INDIRECT ||
                                           operand_class_uoff == OPERAND_IMMEDIATE,
                                           "u_offset of Common ISA HDC INSTRUCTION "
                                           "should not be address or predicate operand.");

                        //v_offset
                        Common_ISA_Operand_Class operand_class_voff = getVectorOperand(inst, i++).getOperandClass();
                        REPORT_INSTRUCTION(options,operand_class_voff == OPERAND_GENERAL  ||
                                           operand_class_voff == OPERAND_INDIRECT ||
                                           operand_class_voff == OPERAND_IMMEDIATE,
                                           "v_offset of Common ISA HDC instruction "
                                           "should not be address or predicate operand.");

                        if (subOpcode == ISA_HDC_CONV ||
                            subOpcode == ISA_HDC_MMF ||
                            subOpcode == ISA_HDC_1PIXELCONV ||
                            subOpcode == ISA_HDC_1DCONV_H ||
                            subOpcode == ISA_HDC_1DCONV_V)
                        {
                            //pixel size
                            //FOr 2D Convovle bit 4 is for indicating SKL or BDW mode
                            uint8_t pixel_size = getPrimitiveOperand<uint8_t>(inst, i++)&0XF;
                            REPORT_INSTRUCTION(options,pixel_size < 2, "CISA VA++ instruction uses invalid output pixel size.");
                        }

                        if (subOpcode == ISA_HDC_MMF ||
                            subOpcode == ISA_HDC_LBPCREATION)
                        {
                            //mode
                            uint8_t mode = getPrimitiveOperand<uint8_t>(inst, i++);
                            REPORT_INSTRUCTION(options,mode <= 2, "CISA VA++ instruction uses invalid mode.");
                        }

                        if (subOpcode == ISA_HDC_LBPCORRELATION)
                        {
                            /// disparity
                            Common_ISA_Operand_Class operand_class_disparity = getVectorOperand(inst, i++).getOperandClass();
                            REPORT_INSTRUCTION(options,operand_class_disparity == OPERAND_GENERAL  ||
                                               operand_class_disparity == OPERAND_INDIRECT ||
                                               operand_class_disparity == OPERAND_IMMEDIATE,
                                               "disparity of Common ISA HDC LBPCORRELATION "
                                               "should not be address or predicate operand.");
                        }

                        if (subOpcode == ISA_HDC_1PIXELCONV)
                        {
                            /// offsets raw operand
                            i++;
                        }

                        /// dst surface
                        uint8_t dstsurface = getPrimitiveOperand<uint8_t>(inst, i++);
                        REPORT_INSTRUCTION(options,0 != dstsurface, "Surface T0 (the SLM surface) is not allowed for VA++ instructions.");
                        REPORT_INSTRUCTION(options,dstsurface < numPreDefinedSurfs + header->getSurfaceCount(),
                            "CISA VA++ instruction uses undefined destination surface.");

                        /// x offset
                        Common_ISA_Operand_Class operand_class_xoff = getVectorOperand(inst, i++).getOperandClass();
                        REPORT_INSTRUCTION(options,operand_class_xoff == OPERAND_GENERAL  ||
                                           operand_class_xoff == OPERAND_INDIRECT ||
                                           operand_class_xoff == OPERAND_IMMEDIATE,
                                           "x_offset of Common ISA HDC instruction "
                                           "should not be address or predicate operand.");

                        /// y offset
                        Common_ISA_Operand_Class operand_class_yoff = getVectorOperand(inst, i++).getOperandClass();
                        REPORT_INSTRUCTION(options,operand_class_yoff == OPERAND_GENERAL  ||
                                           operand_class_yoff == OPERAND_INDIRECT ||
                                           operand_class_yoff == OPERAND_IMMEDIATE,
                                           "y_offset of Common ISA HDC instruction "
                                           "should not be address or predicate operand.");
                        break;
                    }
                default:
                     REPORT_INSTRUCTION(options,false, "Invalid VA++ sub-opcode: %d.", subOpcode);
            }

            break;
        }
        default: REPORT_INSTRUCTION(options,false, "Illegal Sampler Instruction Opcode: %d, %s.", opcode, ISA_Inst_Table[opcode].str);
    }
}

void vISAVerifier::verifyInstructionSIMDFlow(
    const CISA_INST* inst)
{
    ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
    switch (opcode)
    {
        case ISA_GOTO:
        {
            auto labelId = getPrimitiveOperand<uint16_t>(inst, 0);
            if (labelId >= header->getLabelCount())
            {
                REPORT_INSTRUCTION(options, false, "bad label id %d", labelId);
            }
            else
            {
                auto iter = labelDefs.find(labelId);
                if (iter == labelDefs.end())
                {
                    labelDefs[labelId] = false;
                }
                // nothing needs to be done if the label is already in the map
            }
            break;
        }
        default: REPORT_INSTRUCTION(options,false, "Illegal SIMD CF Instruction Opcode: %d, %s.", opcode, ISA_Inst_Table[opcode].str);
    }
}

static bool isUDType(VISA_Type T)
{
    return ISA_TYPE_UD == T;
}

static bool isDType(VISA_Type T)
{
    return ISA_TYPE_D == T;
}

static bool isDorUDTYpe(VISA_Type T)
{
    return isUDType(T) || isDType(T);
}

static bool isFType(VISA_Type T)
{
    return ISA_TYPE_F == T;
}

void vISAVerifier::verifyInstructionDataport(
    const CISA_INST* inst)
{
    ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
    unsigned numPreDefinedSurfs = Get_CISA_PreDefined_Surf_Count();

    unsigned i = 0;

    uint8_t surface  = 0;
    uint8_t modifier = 0;

    switch (opcode)
    {
        case ISA_MEDIA_ST:
        case ISA_MEDIA_LD:
        {
             uint8_t plane        = 0;
             uint8_t block_width  = 0;
             uint8_t block_height = 0;

             if (ISA_MEDIA_LD == opcode || ISA_MEDIA_ST == opcode)
             {
                 modifier = getPrimitiveOperand<uint8_t>(inst, i++); // for skipping the modifier

                 if (ISA_MEDIA_LD == opcode)
                     REPORT_INSTRUCTION(options,modifier < MEDIA_LD_Mod_NUM,
                             "MEDIA_LD modifier must be 0-5 not %d", modifier);

                 if (ISA_MEDIA_ST == opcode)
                     REPORT_INSTRUCTION(options,modifier < MEDIA_ST_Mod_NUM,
                             "MEDIA_ST modifier must be 0-3 not %d", modifier);
             }

             surface = getPrimitiveOperand<uint8_t>(inst, i++);
             REPORT_INSTRUCTION(options,0 != surface, "Surface T0 (the SLM surface) is not allowed for MEDIA_LD/MEDIA_ST");
             REPORT_INSTRUCTION(options,surface < numPreDefinedSurfs + header->getSurfaceCount(),
                 "CISA dataport instruction uses an undeclared surface.");

             if (ISA_MEDIA_LD == opcode || ISA_MEDIA_ST == opcode)
             {
                 plane = getPrimitiveOperand<uint8_t>(inst, i++);
                 REPORT_INSTRUCTION(options,plane <= 3, "MEDIA_LD/MEDIA_ST plane must be in the range [0, 3]: %d", plane);
             }

             block_width  = getPrimitiveOperand<uint8_t>(inst, i++);
             block_height = getPrimitiveOperand<uint8_t>(inst, i++);

             if (ISA_MEDIA_LD == opcode || ISA_MEDIA_ST == opcode)
             {
                 REPORT_INSTRUCTION(options,1 <= block_width && block_width <= COMMON_ISA_MAX_MEDIA_BLOCK_WIDTH_BDW_PLUS,
                         "MEDIA_LD/MEDIA_ST block width must be in the range [1, 32]: %d",
                         block_width);

                 REPORT_INSTRUCTION(options,1 <= block_height && block_height <= COMMON_ISA_MAX_MEDIA_BLOCK_HEIGHT,
                         "MEDIA_LD/MEDIA_ST block height must be in the range [1, 64]: %d",
                         block_height);
             }

             if (ISA_MEDIA_LD == opcode)
             {
                 REPORT_INSTRUCTION(options,((1  <= block_width && block_width <=  4 && block_height <= 64)  ||
                                    (5  <= block_width && block_width <=  8 && block_height <= 32)  ||
                                    (9  <= block_width && block_width <= 16 && block_height <= 16)  ||
                                    (17 <= block_width && block_width <= 32 && block_height <=  8)  ||
                                    (33 <= block_width && block_width <= 64 && block_height <=  4)) &&
                                    (block_width * block_height <= 256                           ),
                                    "MEDIA_LD only supports objects that fit into a single dataport "
                                    "transaction where block width <= 64 bytes and size <= 256 bytes. "
                                    "Block width: %d. Block height: %d", block_width, block_height);
             }
             else if (ISA_MEDIA_ST == opcode)
             {
                 REPORT_INSTRUCTION(options,((1  <= block_width && block_width <=  4 && block_height <= 64)  ||
                                    (5  <= block_width && block_width <=  8 && block_height <= 32)  ||
                                    (9  <= block_width && block_width <= 16 && block_height <= 16)  ||
                                    (17 <= block_width && block_width <= 32 && block_height <=  8)  ||
                                    (33 <= block_width && block_width <= 64 && block_height <=  4)) &&
                                    (block_width * block_height <= 256                           ),
                                    "MEDIA_ST only supports objects that fit into a single dataport "
                                    "transaction where block width <= 64 bytes and size <= 256 bytes. "
                                    "Block width: %d. Block height: %d", block_width, block_height);
             }

             Common_ISA_Operand_Class operand_class_xoff = getVectorOperand(inst, i++).getOperandClass();
             REPORT_INSTRUCTION(options,operand_class_xoff != OPERAND_ADDRESS && operand_class_xoff != OPERAND_PREDICATE,
                               "x_offset of Common ISA Meida LD/ST instrution should not be address or predicate operand.");

             Common_ISA_Operand_Class operand_class_yoff = getVectorOperand(inst, i++).getOperandClass();
             REPORT_INSTRUCTION(options,operand_class_yoff != OPERAND_ADDRESS && operand_class_yoff != OPERAND_PREDICATE,
                               "y_offset of Common ISA Meida LD/ST instrution should not be address or predicate operand.");

             break;
        }
        case ISA_OWORD_ST:
        case ISA_OWORD_LD:
        case ISA_OWORD_LD_UNALIGNED:
        {
             uint8_t size = getPrimitiveOperand<uint8_t>(inst, i++);
             size = size & 0x7;

             REPORT_INSTRUCTION(options,size < OWORD_NUM_ILLEGAL,
                     "OWORD_LD*/OWORD_ST size must be in the range [0, 3] "
                     "(ie, OWord block size must be 1/2/4/8. OWord block size: %d", size);

             if (ISA_OWORD_ST != opcode)
             {
                 modifier = getPrimitiveOperand<uint8_t>(inst, i++);
             }

             surface = getPrimitiveOperand<uint8_t>(inst, i++);
             if (getGenxPlatform() < GENX_ICLLP)
             {
                 REPORT_INSTRUCTION(options, 0 != surface, "Surface T0 (the SLM surface) is not allowed for OWORD_LD*/OWORD_ST");
             }
             REPORT_INSTRUCTION(options,surface < numPreDefinedSurfs + header->getSurfaceCount(),
                 "CISA dataport instruction uses an undeclared surface.");

             Common_ISA_Operand_Class operand_class = getVectorOperand(inst, i++).getOperandClass();
             REPORT_INSTRUCTION(options,operand_class != OPERAND_ADDRESS && operand_class != OPERAND_PREDICATE,
                               "Offset of Common ISA OWORD LD/ST instrutions should not be address or predicate operands.");

             break;
        }
        case ISA_GATHER:
        case ISA_SCATTER:
        {
             uint8_t elt_size = 0;
             uint8_t num_elts = 0;
             if (ISA_SCATTER == opcode || ISA_GATHER == opcode)
             {
                 elt_size = getPrimitiveOperand<uint8_t>(inst, i++);
                 elt_size = elt_size & 0x3;
                 switch((GATHER_SCATTER_ELEMENT_SIZE)elt_size)
                 {
                     case GATHER_SCATTER_BYTE:
                         elt_size = 1;
                         break;
                     case GATHER_SCATTER_WORD:
                         elt_size = 2;
                         break;
                     case GATHER_SCATTER_DWORD:
                         elt_size = 4;
                         break;
                     default:
                         REPORT_INSTRUCTION(options,false, "Incorrect element size for Gather/Scatter CISA inst.");
                         break;
                 }
             }

             if (ISA_GATHER  == opcode)
                 getPrimitiveOperand<uint8_t>(inst, i++);

             num_elts = getPrimitiveOperand<uint8_t>(inst, i++);
             num_elts = num_elts & 0x3;
             if      ((unsigned)num_elts == 0) num_elts =  8;
             else if ((unsigned)num_elts == 1) num_elts = 16;
             else if ((unsigned)num_elts == 2) num_elts =  1;

             surface = getPrimitiveOperand<uint8_t>(inst, i++);
             //SLM uses surface0 as seen in nbody_SLM: gather
             REPORT_INSTRUCTION(options,surface < numPreDefinedSurfs + header->getSurfaceCount(),
                "CISA dataport instruction uses an undeclared surface.");

             Common_ISA_Operand_Class operand_class_goff = getVectorOperand(inst, i++).getOperandClass();
             REPORT_INSTRUCTION(options,operand_class_goff != OPERAND_ADDRESS && operand_class_goff != OPERAND_PREDICATE,
                               "global_offset of Common ISA gather/scatter instrution should not be address or predicate operand.");

             // check that dst/src have dword type
             getRawOperand(inst, i++);  // const raw_opnd& elementOffset
             const raw_opnd& srcDst = getRawOperand(inst, i++);
             verifyRawOperandType(inst, srcDst, isDWordType);
             break;
        }
        case ISA_GATHER4_TYPED:
        case ISA_SCATTER4_TYPED:
        {
            if (getVersionAsInt(isaHeader.major_version, isaHeader.minor_version) >=
                getVersionAsInt(3, 2))
            {
                uint8_t ch_mask  = 0;
                ch_mask = getPrimitiveOperand<uint8_t>(inst, i++);
                ch_mask = ch_mask & 0xF;
                REPORT_INSTRUCTION(options,ch_mask != 0x0, "At least one channel must be enabled for TYPED GATEHR4/SCATTER4");

                surface = getPrimitiveOperand<uint8_t>(inst, i++);
                REPORT_INSTRUCTION(options, (0 != surface && 5 != surface), "Surface T0/T5 (the SLM surface) is not allowed for TYPED SCATTTER4/GATHER4");
                REPORT_INSTRUCTION(options,surface < numPreDefinedSurfs + header->getSurfaceCount(),
                    "CISA dataport TYPED SCATTTER4/GATHER4 instruction uses an undeclared surface.");
            }
             break;
        }
        case ISA_GATHER4_SCALED:
        case ISA_SCATTER4_SCALED:
        {
            break;
        }
        case ISA_GATHER_SCALED:
        case ISA_SCATTER_SCALED:
        {
            break;
        }
        case ISA_DWORD_ATOMIC:
        {
            unsigned subOpc = getPrimitiveOperand<uint8_t>(inst, i++);
            auto subOp = static_cast<VISAAtomicOps>(subOpc & 0x1F);
            switch (subOp) {
            default:
                REPORT_INSTRUCTION(options, false, "Invalid DWORD ATOMIC sub op.");
            case ATOMIC_ADD:
            case ATOMIC_SUB:
            case ATOMIC_INC:
            case ATOMIC_DEC:
            case ATOMIC_MIN:
            case ATOMIC_MAX:
            case ATOMIC_XCHG:
            case ATOMIC_CMPXCHG:
            case ATOMIC_AND:
            case ATOMIC_OR:
            case ATOMIC_XOR:
            case ATOMIC_IMIN:
            case ATOMIC_IMAX:
            case ATOMIC_PREDEC:
            case ATOMIC_FMAX:
            case ATOMIC_FMIN:
            case ATOMIC_FCMPWR:
                break;
            }
            surface = getPrimitiveOperand<uint8_t>(inst, i++);
            REPORT_INSTRUCTION(options, surface < numPreDefinedSurfs + header->getSurfaceCount(),
                               "CISA dataport instruction uses an undeclared surface.");

            const raw_opnd& offsets = getRawOperand(inst, i++);
            verifyRawOperandType(inst, offsets, isUDType);

            // Check remaining raw operands.
            VISAAtomicOps subOpKind = static_cast<VISAAtomicOps>(subOpc);
            bool (*typeFn)(VISA_Type) = 0;
            switch (subOpKind) {
            default:
                typeFn = isUDType;
                break;
            case ATOMIC_IMAX:
            case ATOMIC_IMIN:
                typeFn = isDType;
                break;
            case ATOMIC_ADD:
            case ATOMIC_SUB:
                typeFn = isDorUDTYpe;
                break;
            case ATOMIC_FMAX:
            case ATOMIC_FMIN:
            case ATOMIC_FCMPWR:
                typeFn = isFType;
                break;
            }

            // Check src0:
            //
            // - for INC and DEC operations, src0 must be V0 (the null variable);
            // - for IMIN and IMAX it must have type D;
            // - for all other operations, it must have type UD.
            const raw_opnd& src0 = getRawOperand(inst, i++);
            if (subOpKind == ATOMIC_INC || subOpKind == ATOMIC_DEC || subOpKind == ATOMIC_PREDEC)
            {
                REPORT_INSTRUCTION(options, src0.index == 0,
                                   "src0 in ISA_DWORD_ATOMIC inst must be "
                                   "V0 for INC/DEC/PREDEC.");
            }
            else
            {
                verifyRawOperandType(inst, src0, typeFn);
            }
            // Check src1:
            //
            // - for CMPXCHG operation, it must have type UD;
            // - for all other operations, it must be V0 (the null variable).
            //
            const raw_opnd& src1 = getRawOperand(inst, i++);
            if (subOpKind == ATOMIC_CMPXCHG || subOpKind == ATOMIC_FCMPWR)
            {
                verifyRawOperandType(inst, src1, typeFn);
            }
            else
            {
                REPORT_INSTRUCTION(options, src1.index == 0,
                                   "src1 in ISA_DWORD_ATOMIC inst must be "
                                   "V0 for non CMPXCHG operations.");
            }
            // Check dst:
            //
            // - for IMIN and IMAX, it must have type D;
            // - for all other operations, it must have type UD.
            //
            const raw_opnd& dst = getRawOperand(inst, i++);
            verifyRawOperandType(inst, dst, typeFn);
            break;
        }
        case ISA_3D_TYPED_ATOMIC:
        case ISA_3D_RT_WRITE:
        {
            // no verification for now
            break;
        }
        default: REPORT_INSTRUCTION(options,false, "Illegal dataport Instruction Opcode: %d, %s.", opcode, ISA_Inst_Table[opcode].str);
    }
}



void vISAVerifier::verifyKernelAttributes()
{
    /// Verify SLMSize, if present, shows up only once
    unsigned int numSLMSize = 0;
    for (unsigned int i = 0; i < header->getAttrCount(); i++)
    {
        auto attr = header->getAttr(i);
        const char* attrName = header->getString(attr->nameIndex);
        if (Attributes::isAttribute(Attributes::ATTR_SLMSize, attrName))
        {
            numSLMSize++;
        }
    }

    REPORT_HEADER(options, numSLMSize <= 1,
        "More than 1 kernel attribute defined %s",
        Attributes::getAttributeName(Attributes::ATTR_SLMSize));

}

void vISAVerifier::verifyKernelHeader()
{
    /// Verify variable decls.
    unsigned numPreDefinedVars = Get_CISA_PreDefined_Var_Count();
    unsigned numPreDefinedSurfs = Get_CISA_PreDefined_Surf_Count();
    for (unsigned i = 0; i < header->getVarCount(); i++)
    {
        verifyVariableDecl(i);
    }

    /// Verify address decls.
    for (unsigned i = 0; i < header->getAddrCount(); i++)
    {
        verifyAddressDecl(i);
    }

    /// Verify predicate decls.
    for (unsigned i = 0; i < header->getPredCount(); i++)
    {
        verifyPredicateDecl(i);
    }

    /// Verify labels.
    for (unsigned i = 0; i < header->getLabelCount(); i++)
    {
        REPORT_HEADER(options,header->getLabel(i)->name_index < header->getStringCount(),
            "label%d's name index(%d) is not valid", i, header->getLabel(i)->name_index);
    }

    /// Verify sampler.
    for (unsigned i = 0; i < header->getSamplerCount(); i++)
    {
        REPORT_HEADER(options,header->getSampler(i)->name_index < header->getStringCount(),
            "S%d's name index(%d) is not valid", i, header->getSampler(i)->name_index);
        REPORT_HEADER(options,header->getSampler(i)->num_elements <= COMMON_ISA_MAX_SAMPLER_SIZE,
            "S%d's number of elements(%d) is not vaild", i, header->getSampler(i)->num_elements);
    }

    /// Verify surface.
    unsigned int numPredSurf = Get_CISA_PreDefined_Surf_Count();
    for (unsigned i = numPredSurf; i < header->getSurfaceCount(); i++)
    {
        REPORT_HEADER(options,header->getSurface(i)->name_index < header->getStringCount(),
            "T%d's name index(%d) is not valid", i, header->getSurface(i)->name_index);
        REPORT_HEADER(options,header->getSurface(i)->num_elements <= COMMON_ISA_MAX_SURFACE_SIZE,
            "T%d's number of elements(%d) is not valid", i, header->getSurface(i)->num_elements);
    }

    // Verify inputs.
    // v3.3+, kernel may have explicit arguments followed by implicit ones.
    // This information is only used by the CM runtime, not by Finalizer.
    unsigned implicitIndex = header->getInputCount();
    uint32_t versionNum = getVersionAsInt(isaHeader.major_version, isaHeader.minor_version);
    if (versionNum >= getVersionAsInt(3, 3))
    {
        for (unsigned i = 0; i < header->getInputCount(); i++)
        {
            if (header->getInput(i)->getImplicitKind() != 0)
            {
                implicitIndex = i;
                break;
            }
        }
    }

    // [Begin, end) is an interval for each input. We check two things
    // - no overlap
    // - do not overflow i.e. end < 32 * (256 - 1)
    for (unsigned i = 0; i < header->getInputCount(); i++)
    {
        {
            auto pi = header->getInput(i);
            unsigned Begin = pi->offset;
            unsigned End = Begin + pi->size;
            if (End >= 32 * (256 - 1))
            {
                REPORT_HEADER(options, false, "Input V%d is out of bound [%d, %d)", pi->index, Begin, End);
            }
            for (unsigned j = 0; j < i; ++j)
            {
                auto pj = header->getInput(j);
                unsigned Begin1 = pj->offset, End1 = pj->offset + pj->size;
                if ((Begin >= Begin1 && Begin < End1) ||
                    (Begin1 >= Begin && Begin1 < End))
                {
                    REPORT_HEADER(options, false, "Input V%d = [%d, %d) intersects with V%d = [%d, %d)",
                        pi->index, Begin, End, pj->index, Begin1, End1);
                }
            }
        }

        if (i >= implicitIndex)
        {
            REPORT_HEADER(
                options, header->getInput(i)->getImplicitKind() != 0,
                "Explicit input %d must not follow an implicit input %d", i,
                implicitIndex);
        }
        switch (header->getInput(i)->getInputClass())
        {
            case INPUT_GENERAL:
                 if (header->getInput(i)->index < numPreDefinedVars)
                 {
                     REPORT_HEADER(options,false, "Input %d points to an invalid variable(%d)", i, header->getInput(i)->index);
                 }
                 else
                 {
                     int varId   = header->getInput(i)->index - numPreDefinedVars;
                     int varSize = header->getVar(varId)->num_elements * CISATypeTable[header->getVar(varId)->getType()].typeSize;
                     REPORT_HEADER(options,varSize == header->getInput(i)->size,
                         "Input %d's size(%d) does not agree with its variable (V%d)'s", i, header->getInput(i)->size, varId + numPreDefinedVars);
                     if (header->getInput(i)->size < getGRFSize())
                     {
                         // check that input does not straddle GRF boundary
                         auto beginGRF = header->getInput(i)->offset / getGRFSize();
                         auto endGRF = (header->getInput(i)->offset + header->getInput(i)->size - 1) / getGRFSize();
                         REPORT_HEADER(options, beginGRF == endGRF, "Input %s is <1 GRF but straddles GRF boundary", header->getInput(i)->dcl->getName());
                     }
                 }
                 break;
            case INPUT_SAMPLER:
                 REPORT_HEADER(options,header->getInput(i)->index < header->getSamplerCount(),
                     "Input%d points to an invalid sampler index(%d)", i, header->getInput(i)->index);
                 break;
            case INPUT_SURFACE:
                 if (header->getInput(i)->index-numPredSurf >= header->getSurfaceCount() || header->getInput(i)->index < numPredSurf)
                 {
                     REPORT_HEADER(options,false, "Input%d points to an invalid/predefined surface index(%d)", i, header->getInput(i)->index);
                 }
                 else
                 {
                     int surfaceSize;
                     surfaceSize = header->getSurface(header->getInput(i)->index-numPreDefinedSurfs)->num_elements * CISATypeTable[ISA_TYPE_UD].typeSize;

                     REPORT_HEADER(options,surfaceSize == header->getInput(i)->size, "Input%d's size(%d) does not agree with its surface's", i, header->getInput(i)->size);
                 }
                 break;
            default:
                 REPORT_HEADER(options,false, "Input%d has invalid operand class(%d)", i, header->getInput(i)->getInputClass());
        }
        /// TODO: Consider adding offset checks here.
    }

    verifyKernelAttributes();
}

void vISAVerifier::finalize()
{
    for (auto iter : labelDefs)
    {
        if (!(iter.second))
        {
            REPORT_HEADER(options, false, "undefined label: %s", header->getString(header->getLabel(iter.first)->name_index));
        }
    }
}

void vISAVerifier::verifyInstruction(
    const CISA_INST* inst)
{
    ISA_Opcode opcode = (ISA_Opcode)inst->opcode;

    if (!(ISA_RESERVED_0 < opcode && opcode < ISA_NUM_OPCODE))
        cerr << "Invalid opcode, value: " << (unsigned)opcode << endl;

    ASSERT_USER(ISA_RESERVED_0 < opcode && opcode < ISA_NUM_OPCODE, "Invalid CISA opcode: out of range.");

    TARGET_PLATFORM instPlatform = CISA_INST_table[opcode].platf;
    if (instPlatform != ALL)
    {
        // We assume instructions are backward compatible
        // instructions that are not should have their own checks elsewhere
        REPORT_INSTRUCTION(options, getGenxPlatform() >= instPlatform, "vISA instruction not supported on this platform");
    }

    for (unsigned i = 0; i < inst->opnd_count; i++)
        verifyOperand(inst, i);

    if (hasExecSize(opcode))
    {
        auto execSize = inst->getExecSize();
        REPORT_INSTRUCTION(options, execSize < EXEC_SIZE_ILLEGAL, "vISA instruction uses an illegal execution size.");
        // check for illegal combinations of emask and execution size
        REPORT_INSTRUCTION(options, Get_VISA_Exec_Size(execSize) + getvISAMaskOffset(inst->getExecMask()) <= 32,
            "vISA instruction has illegal combination of execution size and mask");
    }

    if (hasPredicate(opcode))
    {
        uint16_t predicateNum = inst->pred & 0xfff;
        REPORT_INSTRUCTION(options,predicateNum < header->getPredCount() + COMMON_ISA_NUM_PREDEFINED_PRED, "CISA instruction uses an illegal predicate value.");
    }

    switch (ISA_Inst_Table[opcode].type)
    {
        case ISA_Inst_SVM:       verifyInstructionSVM         (inst); break;
        case ISA_Inst_Mov:       verifyInstructionMove        (inst); break;
        case ISA_Inst_Sync:      verifyInstructionSync        (inst); break;
        case ISA_Inst_Flow:      verifyInstructionControlFlow (inst); break;
        case ISA_Inst_Misc:      verifyInstructionMisc        (inst); break;
        case ISA_Inst_Arith:     verifyInstructionArith       (inst); break;
        case ISA_Inst_Logic:     verifyInstructionLogic       (inst); break;
        case ISA_Inst_Compare:   verifyInstructionCompare     (inst); break;
        case ISA_Inst_Address:   verifyInstructionAddress     (inst); break;
        case ISA_Inst_Sampler:   verifyInstructionSampler     (inst); break;
        case ISA_Inst_SIMD_Flow: verifyInstructionSIMDFlow    (inst); break;
        case ISA_Inst_Data_Port: verifyInstructionDataport    (inst); break;
        default:
        {
            stringstream sstr; sstr << "Illegal or unimplemented CISA instruction (opcode, type): (" << opcode << ", " << ISA_Inst_Table[opcode].type << ").";
            ASSERT_USER(false, sstr.str());
        }
    }
}

#endif // IS_RELEASE_DLL

