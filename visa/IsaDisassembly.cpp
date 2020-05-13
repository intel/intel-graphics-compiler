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

/*
 * ISA IR Disassembler
 *
 * This library is designed to be extremely reusable and general in nature, and as a result
 * the following ISA IR disassembly code primarily uses the following IR and data types:
 *
 * - common_isa_header
 * - kernel_format_t
 * - attribute_info_t
 * - CISA_opnd
 * - vector_opnd
 * - raw_opnd
 * - CISA_INST
 * - std::list<CISA_INST*>
 *
 * and prints them as human readable text to an isaasm file.
 *
 * Use of any other data types should be discussed by several members of the CM jitter team before hand.
 *
 */

#include <cstdint>
#include <cctype>
#include <list>
#include <sstream>
#include <string>

#include "visa_igc_common_header.h"
#include "common.h"
#include "Mem_Manager.h"
#include "Common_ISA.h"
#include "Common_ISA_framework.h"
#include "Common_ISA_util.h"
#include "VISADefines.h"
#include "IsaDisassembly.h"
#include "Option.h"
#include "JitterDataStruct.h"
#include "VISABuilderAPIDefinition.h"
#include "PreDefinedVars.h"

using namespace vISA;

/// Output flags.
_THREAD bool g_shortRegionPrint  = false; /// Use shorthand names for common regions.
_THREAD bool g_inlineTypePrint   = false; /// Print the type information with operands.
_THREAD bool g_prettyPrint       = true ; /// Line up the comments.
_THREAD bool g_ignorelocs        = false; /// Ignore printing LOCs.
_THREAD bool g_noinstid          = false; /// Ignore printing instruction id comments.

const char *printAsmName(const print_format_provider_t* header)
{
    for (unsigned i = 0; i < header->getAttrCount(); i++)
    {
        const char* attrName = header->getString(header->getAttr(i)->nameIndex);
        if (Attributes::isAttribute(Attributes::ATTR_OutputAsmPath, attrName))
            return header->getAttr(i)->value.stringVal;
    }

    return "";
}

const char *getGenVarName(int id, const print_format_provider_t& header)
{
    int numPredefined = Get_CISA_PreDefined_Var_Count();
    if (id < numPredefined)
    {
        return getPredefinedVarString(mapExternalToInternalPreDefVar(id));
    }
    else
    {
        MUST_BE_TRUE((id - numPredefined) < (int) header.getVarCount(),
            "invalid vISA general variable id");
        return header.getString(header.getVar(id - numPredefined)->name_index);
    }
}

static std::string printSurfaceName(uint32_t declID)
{
    std::stringstream sstr;
    unsigned numPreDefinedSurf = Get_CISA_PreDefined_Surf_Count();
    if (declID < numPreDefinedSurf)
    {
        sstr << vISAPreDefSurf[declID].name;
    }
    else
    {
        sstr << "T" << declID;
    }

    return sstr.str();
}

std::string printVariableDeclName(
    const print_format_provider_t* header,
    unsigned declID,
    const Options *options,
    Common_ISA_State_Opnd_Class operand_prefix_kind = NOT_A_STATE_OPND)
{
    MUST_BE_TRUE(header, "Argument Exception: argument header is NULL.");
    std::stringstream sstr;

    unsigned numPreDefinedVars = Get_CISA_PreDefined_Var_Count();
    if (options->getOption(vISA_DumpIsaVarNames) && operand_prefix_kind == NOT_A_STATE_OPND)
    {
        sstr << getGenVarName(declID, *header);
    }
    else
    {
        switch (operand_prefix_kind)
        {   case STATE_OPND_SURFACE : sstr << printSurfaceName(declID); break;
            case STATE_OPND_SAMPLER : sstr << "S"   << declID; break;
            default                 :
                if(options->getOption(vISA_easyIsaasm) == false ||
                    options->getOption(vISA_PlatformIsSet) == false)
                {
                    // If platform is not set then dcl instances are
                    // not created causing a crash. So print dcl name
                    // the old way.
                    if (declID < numPreDefinedVars)
                    {
                        sstr << "V" << declID;
                    }
                    else
                    {
                        declID -= numPreDefinedVars;
                        const var_info_t *var = header->getVar(declID);
                        std::string name = header->getString(var->name_index);
                        sstr << name;
                    }
                }
                else
                {

                    if(declID < numPreDefinedVars)
                    {
                        sstr << "V" << declID;
                    }
                    else
                    {
                        G4_Declare* aliasDcl = header->getVar(declID - numPreDefinedVars)->dcl;
                        unsigned int aliasOff = 0;
                        std::string type =
                            G4_Type_Table[aliasDcl->getElemType()].str;

                        while(aliasDcl->getAliasDeclare() != NULL)
                        {
                            aliasOff += aliasDcl->getAliasOffset();
                            aliasDcl = aliasDcl->getAliasDeclare();
                        }

                        // aliasDcl is top most dcl with aliasOff
                        // Lets find out declID of aliasDcl
                        for(unsigned int i = 0; i < header->getVarCount(); i++)
                        {
                            if(header->getVar(i)->dcl == aliasDcl)
                            {
                                declID = i + numPreDefinedVars;
                                break;
                            }
                        }

                        sstr << "V"   << declID << "_" << type;
                        if(aliasOff != 0)
                        {
                            sstr << "_" << aliasOff;
                        }
                    }
                }

                break;
        }
    }
    return sstr.str();
}

static std::string printRegion(uint16_t region)
{
    std::stringstream sstr;
    Common_ISA_Region_Val v_stride = (Common_ISA_Region_Val)(region & 0xF);
    Common_ISA_Region_Val width = (Common_ISA_Region_Val)((region >> 4 ) & 0xF);
    Common_ISA_Region_Val h_stride = (Common_ISA_Region_Val)((region >> 8 ) & 0xF);

    if (width == REGION_NULL)
    {
        //dst operand, only have horizontal stride
        sstr << "<" << Common_ISA_Get_Region_Value(h_stride) << ">";
    }
    else if (v_stride == REGION_NULL)
    {
        // VxH mode for indirect operand
        sstr << "<" << Common_ISA_Get_Region_Value(width) << "," << Common_ISA_Get_Region_Value(h_stride) << ">";
    }
    else
    {
        if (g_shortRegionPrint                         &&
            0 == Common_ISA_Get_Region_Value(v_stride) &&
            1 == Common_ISA_Get_Region_Value(width)    &&
            0 == Common_ISA_Get_Region_Value(h_stride))
        {
            sstr << ".s";
        }
        else if (g_shortRegionPrint &&
                Common_ISA_Get_Region_Value(v_stride) ==
                Common_ISA_Get_Region_Value(width) &&
                1 == Common_ISA_Get_Region_Value(h_stride))
        {
            sstr << ".v";
        }
        else
        {
            sstr << "<" << Common_ISA_Get_Region_Value(v_stride) << ";" << Common_ISA_Get_Region_Value(width) << "," << Common_ISA_Get_Region_Value(h_stride) << ">";
        }
    }

    return sstr.str();
}

std::string printVectorOperand(
    const print_format_provider_t* header,
    const vector_opnd& opnd,
    const Options *opt,
    bool showRegion)
{
    std::stringstream sstr;

    MUST_BE_TRUE(header, "Argument Exception: argument header is NULL.");

    VISA_Modifier modifier = (VISA_Modifier)((opnd.tag >> 3) & 0x7);

    /// .sat is dumped with the opcode
    if (modifier == MODIFIER_SAT)
        modifier =  MODIFIER_NONE;

    sstr << " ";
    switch (opnd.tag & 0x7)
    {
        case OPERAND_GENERAL:
        {
            sstr << Common_ISA_Get_Modifier_Name(modifier)
                << printVariableDeclName(header, opnd.getOperandIndex(), opt, NOT_A_STATE_OPND);

            if ((!g_shortRegionPrint)                      ||
                (!(opnd.opnd_val.gen_opnd.row_offset == 0 &&
                (((opnd.opnd_val.gen_opnd.col_offset == 0))))))
            {
                sstr << "("
                     << (unsigned)opnd.opnd_val.gen_opnd.row_offset << ","
                     << (unsigned)opnd.opnd_val.gen_opnd.col_offset << ")";
            }

            if (showRegion)
            {
                sstr << printRegion(opnd.opnd_val.gen_opnd.region);
            }

            break;
        }
        case OPERAND_ADDRESS:
        {
            sstr << Common_ISA_Get_Modifier_Name(modifier) << "A" << opnd.opnd_val.addr_opnd.index
                 << "(" << (unsigned)opnd.opnd_val.addr_opnd.offset << ")<"
                 << Get_VISA_Exec_Size((VISA_Exec_Size)(opnd.opnd_val.addr_opnd.width & 0xF)) << ">";
            break;
        }
        case OPERAND_PREDICATE:
        {
            sstr << Common_ISA_Get_Modifier_Name(modifier) << "P" << opnd.opnd_val.pred_opnd.index;
            break;
        }
        case OPERAND_INDIRECT:
        {
            sstr << Common_ISA_Get_Modifier_Name(modifier) << "r[A" << opnd.opnd_val.indirect_opnd.index
                 << "("
                 << (unsigned)opnd.opnd_val.indirect_opnd.addr_offset     << "),"
                 << (short)   opnd.opnd_val.indirect_opnd.indirect_offset << "]" ;
            sstr << printRegion(opnd.opnd_val.indirect_opnd.region);
            VISA_Type type = (VISA_Type)(opnd.opnd_val.indirect_opnd.bit_property & 0xf);
            sstr << ":" << CISATypeTable[type].typeName;
            break;
        }
        case OPERAND_ADDRESSOF:
        {
            sstr << "&" << printVariableDeclName(header, opnd.getOperandIndex(), opt, NOT_A_STATE_OPND)
                 << (((short)opnd.opnd_val.addressof_opnd.addr_offset >= 0) ? "+" : "")
                 << (((short)opnd.opnd_val.addressof_opnd.addr_offset));
            break;
        }
        case OPERAND_IMMEDIATE:
        {
            VISA_Type type = (VISA_Type)(opnd.opnd_val.const_opnd.type & 0xF);
            if (type == ISA_TYPE_DF)
                sstr << "0x" << std::hex <<
                    *((uint64_t*) &opnd.opnd_val.const_opnd._val.dval) <<
                    ":" << CISATypeTable[type].typeName << std::dec;
            else if (type == ISA_TYPE_Q || type == ISA_TYPE_UQ)
                sstr << "0x" << std::hex << opnd.opnd_val.const_opnd._val.lval <<
                        ":" << CISATypeTable[type].typeName << std::dec;
            else
                sstr << "0x" << std::hex << opnd.opnd_val.const_opnd._val.ival <<
                    ":" << CISATypeTable[type].typeName << std::dec;
            break;
        }
        case OPERAND_STATE:
        {
            sstr << printVariableDeclName(header, opnd.getOperandIndex(), opt, (Common_ISA_State_Opnd_Class)opnd.opnd_val.state_opnd.opnd_class)
                 << "(" << (unsigned)opnd.opnd_val.state_opnd.offset << ")";
            break;
        }
        default: ASSERT_USER(false, "Attempted to dump an invalid or unimplemented vector operand type.");
    }

    return sstr.str();
}

std::string printRawOperand(
    const print_format_provider_t* header,
    const raw_opnd& opnd,
    const Options *opt)
{
    MUST_BE_TRUE(header, "Argument Exception: argument header is NULL.");
    std::stringstream sstr;
    sstr << " " << printVariableDeclName(header, opnd.index, opt, NOT_A_STATE_OPND) << "." << opnd.offset;
    return sstr.str();
}

static std::string printOperand(
    const print_format_provider_t* header,
    const CISA_INST* inst,
    unsigned i,
    const Options *opt)
{
    MUST_BE_TRUE(header, "Argument Exception: argument header is NULL.");
    MUST_BE_TRUE(inst  , "Argument Exception: argument inst   is NULL.");
    MUST_BE_TRUE(inst->opnd_count > i, "No such operand, i, for instruction inst.");
    std::stringstream sstr;
    switch (getOperandType(inst, i))
    {
        case CISA_OPND_OTHER:  sstr << (getPrimitiveOperand<unsigned>             (inst, i)); break;
        case CISA_OPND_VECTOR: sstr << printVectorOperand(header, getVectorOperand(inst, i), opt, true); break;
        case CISA_OPND_RAW:    sstr << printRawOperand   (header, getRawOperand   (inst, i), opt); break;
        default:               MUST_BE_TRUE(false, "Invalid operand type.");
    }
    return sstr.str();
}

static void encodeStringLiteral(std::stringstream &ss, const char *str) {
  ss << '"';
  for (size_t i = 0, slen = strlen(str); i < slen; i++) {
      switch (str[i]) { // unsigned so >0x7F doesn't sign ext.
      case '\a': ss << '\\'; ss << 'a'; break;
      case '\b': ss << '\\'; ss << 'b'; break;
      case 0x1B: ss << '\\'; ss << 'e'; break;
      case '\f': ss << '\\'; ss << 'f'; break;
      case '\n': ss << '\\'; ss << 'n'; break;
      case '\r': ss << '\\'; ss << 'r'; break;
      case '\t': ss << '\\'; ss << 't'; break;
      case '\v': ss << '\\'; ss << 'v'; break;
      //
      case '\'': ss << '\\'; ss << '\''; break;
      case '"':  ss << '\\'; ss << '"'; break;
      //
      case '\\': ss << '\\'; ss << '\\'; break;
      default:
          if (std::isprint((unsigned char)str[i])) {
              ss << str[i];
          } else {
              ss << "\\x" << std::hex << (unsigned)((unsigned char)str[i]);
          }
      }
  }
  ss << '"';
}

std::string printAttribute(
    const attribute_info_t* attr,
    const print_format_provider_t* kernel,
    bool isKernelAttr)
{
    std::stringstream sstr;

    if (attr->isInt && attr->size == 1 && attr->value.intVal == 0)
    {
        return sstr.str();
    }

    const char* attrName = kernel->getString(attr->nameIndex);
    sstr << "." << (isKernelAttr ? "kernel_" : "") << "attr " << attrName << "=";
    if (attr->isInt) {
        if (isKernelAttr && Attributes::isAttribute(Attributes::ATTR_Target, attrName)) {
            switch (attr->value.intVal) {
            case VISA_CM: sstr << "\"cm\""; break;
            case VISA_3D: sstr << "\"3d\""; break;
            case VISA_CS: sstr << "\"cs\""; break;
            default:
              sstr << attr->value.intVal;
            }
        } else {
            sstr << attr->value.intVal;
        }
    } else if (attr->size > 0) {
        encodeStringLiteral(sstr, attr->value.stringVal);
    }

    return sstr.str();
}

std::string printPredicateDecl(
    const print_format_provider_t* header, unsigned declID)
{
    MUST_BE_TRUE(header, "Argument Exception: argument header is NULL.");
    std::stringstream sstr;

    const pred_info_t* pred = header->getPred(declID);
    sstr << ".decl P"
         << declID + COMMON_ISA_NUM_PREDEFINED_PRED
         << " "
         << "v_type=P "
         << "num_elts=" << pred->num_elements;

    for (unsigned j = 0; j < pred->attribute_count; j++)
    {
        sstr << " " << printAttribute(&pred->attributes[j], header);
    }

    return sstr.str();
}

std::string printAddressDecl(
    const common_isa_header& isaHeader,
    const print_format_provider_t* header,
    unsigned declID)
{
    MUST_BE_TRUE(header, "Argument Exception: argument header is NULL.");
    std::stringstream sstr;

    const addr_info_t* addr = header->getAddr(declID);
    sstr << ".decl A"
         << declID
         << " "
         << "v_type=A "
         << "num_elts=" << addr->num_elements;

    for (unsigned j = 0; j < addr->attribute_count; j++)
    {
        sstr << " " << printAttribute(&addr->attributes[j], header);
    }

    return sstr.str();
}

std::string printSamplerDecl(
    const print_format_provider_t* header, unsigned declID)
{
    MUST_BE_TRUE(header, "Argument Exception: argument header is NULL.");
    std::stringstream sstr;
    const state_info_t* info = header->getSampler(declID);

    sstr << ".decl S" << declID << " v_type=S";
    sstr << " num_elts=" << info->num_elements;
    sstr << " v_name=" << header->getString(info->name_index);
    for (unsigned j = 0; j < info->attribute_count; j++)
    {
        sstr << " " << printAttribute(&info->attributes[j], header);
    }
    return sstr.str();
}

std::string printSurfaceDecl(
    const print_format_provider_t* header,
    unsigned declID,
    unsigned numPredefinedSurfaces)
{
    MUST_BE_TRUE(header, "Argument Exception: argument header is NULL.");
    std::stringstream sstr;
    const state_info_t* info = header->getSurface(declID);

    sstr << ".decl T" << declID + numPredefinedSurfaces << " v_type=T";
    sstr << " num_elts=" << info->num_elements;
    sstr << " v_name=" << header->getString(info->name_index);
    for (unsigned j = 0; j < info->attribute_count; j++)
    {
        sstr << " " << printAttribute(&info->attributes[j], header);
    }
    return sstr.str();
}

std::string printFuncInput(
    const print_format_provider_t* header,
    unsigned declID,
    bool isKernel,
    const Options* options)
{
    MUST_BE_TRUE(header, "Argument Exception: argument header is NULL.");
    std::stringstream sstr;

    const input_info_t* input = header->getInput(declID);
    if (!isKernel)
    {
        sstr << ".parameter " /* function */;
    }
    else if (!input->getImplicitKind())
    {
        sstr << ".input " /* kernel */;
    }
    else
    {
        sstr << input->getImplicitKindString() << " ";
    }

    if (INPUT_GENERAL == input->getInputClass())
    {
        sstr << printVariableDeclName(header, input->index, options);
    }
    else
    {
        const char* Input_Class_String[] = { "V", "S", "T" };
        sstr << Input_Class_String[input->getInputClass()] << input->index;
    }

    if (isKernel)
        sstr << " offset=" << input->offset;

    sstr << " size=" << input->size;

    return sstr.str();
}

// declID is in the range of [0..#user-var], pre-defnied are not included
std::string printVariableDecl(
    const common_isa_header& isaHeader,
    const print_format_provider_t* header,
    unsigned declID,
    bool isKernel,
    unsigned int funcId,
    const Options *options)
{
    MUST_BE_TRUE(header, "Argument Exception: argument header is NULL.");
    std::stringstream sstr;

    const var_info_t *var = header->getVar(declID);
    VISA_Type  isa_type = (VISA_Type)  ((var->bit_properties     ) & 0xF);
    VISA_Align align    = var->getAlignment();

    unsigned numPreDefinedVars = Get_CISA_PreDefined_Var_Count();
    sstr << ".decl " << printVariableDeclName(header, declID+numPreDefinedVars, options)
         << " v_type=G"
         << " type=" << CISATypeTable[isa_type].typeName
         << " num_elts=" << var->num_elements;

    if(align != ALIGN_UNDEF)
        sstr << " align=" << Common_ISA_Get_Align_Name(align);

    if (var->alias_index)
    {
        sstr << " alias=<";
        if (options->getOption(vISA_DumpIsaVarNames))
        {
            sstr << printVariableDeclName(header, var->alias_index, options);
        }
        else
        {
            if(options->getOption(vISA_easyIsaasm))
            {
                sstr << printVariableDeclName(header, var->alias_index, options);
            }
            else
            {
               sstr << 'V' << var->alias_index;
            }
        }
        sstr << ", " << var->alias_offset << ">";
    }

    for (unsigned j = 0; j < var->attribute_count; j++)
    {
        sstr << " " << printAttribute(&var->attributes[j], header);
    }

    return sstr.str();
}

static std::string printExecutionSize(
    uint8_t opcode, uint8_t execSize, uint8_t subOp = 0)
{
    std::stringstream sstr;

    if (hasExecSize((ISA_Opcode)opcode, subOp))
    {
        sstr << "(";
        uint8_t emsk = ((execSize >> 0x4) & 0xF);
        sstr << emask_str[emsk] << ", ";
        sstr << (unsigned) Get_VISA_Exec_Size((VISA_Exec_Size)(execSize & 0xF));
        sstr << ")";
    }

    if (g_shortRegionPrint && !strcmp("(1)", sstr.str().c_str()))
        return "   ";

    return sstr.str();
}

// execution size is formatted differently for scatter/gather/scatter4/gather4/scatter4_typed/gather4_typed
static std::string printExecutionSizeForScatterGather(uint8_t sizeAndMask)
{
    std::stringstream sstr;
    sstr << "(";
    VISA_EMask_Ctrl emask =
        (VISA_EMask_Ctrl)((sizeAndMask >> 0x4) & 0xF);
    sstr << emask_str[emask] << ", ";

    unsigned execSize = 0;
    switch (sizeAndMask & 0x3)
    {
    case 0:
        execSize = 8;
        break;
    case 1:
        execSize = 16;
        break;
    case 2:
        execSize = 1;
        break;
    default:
        ASSERT_USER(false, "illegal execution size for scatter/gather message");
    }
    sstr << execSize;
    sstr << ")";

    return sstr.str();
}

static std::string printPredicate(uint8_t opcode, uint16_t predOpnd)
{
    std::stringstream sstr;

    if (hasPredicate((ISA_Opcode)opcode) && predOpnd != 0)
    {
        sstr << "(";
        if (predOpnd & 0x8000) sstr << "!";
        sstr << "P" << (predOpnd & 0xfff);

        VISA_PREDICATE_CONTROL control =
            (VISA_PREDICATE_CONTROL)((predOpnd & 0x6000) >> 13);

        switch (control)
        {
            case PRED_CTRL_ANY:
                sstr << ".any";
                break;
            case PRED_CTRL_ALL:
                sstr << ".all";
            default:
                break;
        }

        sstr << ") ";
    }

    return sstr.str();
}

static void printAtomicSubOpc(std::stringstream &sstr, uint8_t value)
{
    VISAAtomicOps op = static_cast<VISAAtomicOps>(value & 0x1F);
    sstr << "." << CISAAtomicOpNames[op];

    if ((value >> 5) == 1)
    {
        sstr << ".16";
    }
    else if ((value >> 6) == 1)
    {
        sstr << ".64";
    }
}

static std::string printInstructionSVM(
    const print_format_provider_t* header,
    const CISA_INST* inst,
    const Options *opt)
{
    unsigned i = 0;
    std::stringstream sstr;

    SVMSubOpcode subOpcode = (SVMSubOpcode)getPrimitiveOperand<uint8_t>(inst, i++);

    /// TODO: Print out the predicate here
    sstr << "svm_";
    switch (subOpcode)
    {
        case SVM_BLOCK_ST:
        case SVM_BLOCK_LD:
            {
             sstr << "block_" << (subOpcode == SVM_BLOCK_ST ? "st" : "ld");
             uint8_t properties = getPrimitiveOperand<uint8_t>(inst, i++);
             if (properties & 8)
                 sstr << ".unaligned";
             VISA_Oword_Num numOwords = (VISA_Oword_Num) (properties & 0x7);
             sstr << " (" << Get_VISA_Oword_Num(numOwords) << ")";
             break;
            }
        case SVM_GATHER:
        case SVM_SCATTER:
        {
             sstr << (subOpcode == SVM_GATHER ? "gather" : "scatter");
             uint8_t block_size = getPrimitiveOperand<uint8_t>(inst, i++);
             uint8_t num_blocks = getPrimitiveOperand<uint8_t>(inst, i++);
             sstr << "." << Get_Common_ISA_SVM_Block_Size((VISA_SVM_Block_Type)block_size);
             sstr << "." << Get_Common_ISA_SVM_Block_Num((VISA_SVM_Block_Num)num_blocks);
             sstr << " " << printExecutionSize(inst->opcode, inst->execsize, subOpcode);
             break;
        }
        case SVM_ATOMIC:
        {
            sstr << "atomic";
            /// TODO: Need platform information for this to work.

            printAtomicSubOpc(sstr, getPrimitiveOperand<uint8_t>(inst, i++));
            sstr << " " << printExecutionSize(inst->opcode, inst->execsize, subOpcode);
            /// element offset
            sstr << printOperand(header, inst, i++, opt);
            /// DWORD_ATOMIC is weird and has the text version
            /// putting the dst operand before the src operands.
            std::stringstream sstr1;
            /// src0
            sstr1 << printOperand(header, inst, i++, opt);
            /// src1
            sstr1 << printOperand(header, inst, i++, opt);
            /// message operand (src or dst)
            sstr << printOperand(header, inst, i++, opt);
            sstr << sstr1.str();
            break;
        }
        case SVM_GATHER4SCALED:
        case SVM_SCATTER4SCALED:
        {
            sstr << (subOpcode == SVM_GATHER4SCALED ? "gather4scaled" : "scatter4scaled");
            unsigned chMask = getPrimitiveOperand<uint8_t>(inst, i++);
            // scale is ignored (MBZ)
            (void) getPrimitiveOperand<uint16_t>(inst, i++);
            sstr << "." << channel_mask_str[chMask];
            sstr << " " << printExecutionSize(inst->opcode, inst->execsize, subOpcode);
            sstr << printOperand(header, inst, i++, opt);
            sstr << printOperand(header, inst, i++, opt);
            sstr << printOperand(header, inst, i++, opt);
            break;
        }
        default:
             ASSERT_USER(false, "Unimplemented or Illegal SVM Sub Opcode.");
    }

    for (; i < inst->opnd_count; i++)
        sstr << printOperand(header, inst, i, opt);

    return sstr.str();
}

static std::string printInstructionCommon(
    const print_format_provider_t* header,
    const CISA_INST* inst,
    const Options *opt)
{
    ISA_Opcode opcode = (ISA_Opcode)inst->opcode;

    std::stringstream sstr;
    sstr << printPredicate(inst->opcode, inst->pred);

    unsigned i = 0;

    /// Print opcode
    if (opcode == ISA_FMINMAX)
    {
        CISA_MIN_MAX_SUB_OPCODE sub_opcode = (CISA_MIN_MAX_SUB_OPCODE)getPrimitiveOperand<uint8_t>(inst, i++);
        sstr << (sub_opcode == CISA_DM_FMIN ? "min" : "max");
    }
    else
    {
        sstr << ISA_Inst_Table[opcode].str;
    }

    if (ISA_Inst_Sync != ISA_Inst_Table[opcode].type)
    {
        unsigned int Count = inst->opnd_count;

        if (ISA_Inst_Compare == ISA_Inst_Table[opcode].type)
        {
            uint8_t relOp = getPrimitiveOperand<uint8_t>(inst, i++);
            sstr << (((relOp >> 7) & 0x1) ? "n" : ""); /// INFO: cmpn opcode print support here.
            sstr << "." << Rel_op_str[(unsigned)(relOp & 0x7)];
        }


        if (ISA_Inst_Arith   == ISA_Inst_Table[opcode].type ||
            ISA_Inst_Mov     == ISA_Inst_Table[opcode].type ||
            ISA_Inst_Logic   == ISA_Inst_Table[opcode].type ||
            ISA_Inst_Address == ISA_Inst_Table[opcode].type ||
            ISA_Inst_Compare == ISA_Inst_Table[opcode].type)
        {
            bool saturate = (((VISA_Modifier)((getVectorOperand(inst, i).tag >> 3 ) & 0x7)) == MODIFIER_SAT);
            sstr << (saturate ? ".sat" : "");
        }

        sstr << " " << printExecutionSize(inst->opcode, inst->execsize);

        if (opcode == ISA_GOTO)
        {
            uint16_t label_id = getPrimitiveOperand<uint16_t>(inst, i++);
            sstr << " " << header->getString(header->getLabel(label_id)->name_index);
        }

        for (; i < Count; i++)
        {

            if (opcode == ISA_ADDR_ADD && i == 1) /// Only for src0 of addr_add
            {
                const vector_opnd& curOpnd = getVectorOperand(inst, i);

                if(curOpnd.getOperandClass() == OPERAND_ADDRESS)
                {
                    sstr << printVectorOperand(header, curOpnd, opt, true);
                }
                else
                {
                    sstr << " " << Common_ISA_Get_Modifier_Name((VISA_Modifier)((curOpnd.tag >> 3) & 0x7));

                    unsigned opnd_index = curOpnd.getOperandIndex();

                    if(curOpnd.getOperandClass() == OPERAND_GENERAL)
                    {
                        uint32_t numPredefined = Get_CISA_PreDefined_Var_Count();
                        VISA_Type type = opnd_index < numPredefined ? getPredefinedVarType(mapExternalToInternalPreDefVar(opnd_index)) :
                            header->getVar(opnd_index - numPredefined)->getType();
                        int offset = curOpnd.opnd_val.gen_opnd.col_offset *
                                     CISATypeTable[type].typeSize +
                                     curOpnd.opnd_val.gen_opnd.row_offset * G4_GRF_REG_NBYTES;
                        sstr << "&" << printVariableDeclName( header, opnd_index, opt) << "+"
                             << offset;
                    }
                    else if (curOpnd.getOperandClass() == OPERAND_STATE)
                    {
                        auto OpClass = curOpnd.getStateOpClass();
                        sstr << "&" << printVariableDeclName(header, opnd_index, opt, OpClass) << "+"
                             << curOpnd.opnd_val.state_opnd.offset *
                                CISATypeTable[ISA_TYPE_D].typeSize;
                    }
                    else
                    {
                        /// TODO: Should we just assert here? Is this allowed?
                        sstr << printOperand(header, inst, i, opt);
                    }
                }
            }
            else
            {
                sstr << printOperand(header, inst, i, opt);
            }
        }
    }
    else
    {
        if (opcode == ISA_FENCE)
        {
            uint8_t mask = getPrimitiveOperand<uint8_t>(inst, i);

            const int SWFenceMask = 0x80;
            if (mask & SWFenceMask)
            {
                sstr << "_sw";
            }
            else
            {

#define BTI_MASK 0x20 // bit 5
                sstr << ((mask & BTI_MASK) ? "_local" : "_global");
                if (mask != 0)
                {
                    sstr << ".";
                    if (mask & 1) sstr << "E";
                    if (mask & (1 << 1)) sstr << "I";
                    if (mask & (1 << 2)) sstr << "S";
                    if (mask & (1 << 3)) sstr << "C";
                    if (mask & (1 << 4)) sstr << "R";
                    if (mask & (1 << 6)) sstr << "L1";
                }
            }
        }
        else if (opcode == ISA_WAIT)
        {
            sstr << printOperand(header, inst, 0, opt);
        }
        else if (opcode == ISA_SBARRIER)
        {
            uint8_t mode = getPrimitiveOperand<uint8_t>(inst, i);
            sstr << (mode ? ".signal" : ".wait");
        }
    }

    return sstr.str();
}

static std::string printInstructionControlFlow(
    const print_format_provider_t* header,
    const CISA_INST* inst,
    const Options *opt)
{
    ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
    unsigned i = 0;
    uint16_t label_id  = 0;

    std::stringstream sstr;

    if (ISA_SUBROUTINE == opcode || ISA_LABEL == opcode)
    {
        label_id = getPrimitiveOperand<uint16_t>(inst, i++);

        sstr << "\n";
        switch (opcode)
        {
            case ISA_SUBROUTINE:
            {
                 std::stringstream uniqueSuffixSstr; uniqueSuffixSstr << '_' << label_id;
                 std::string       uniqueSuffixStr = uniqueSuffixSstr.str();

                 std::string labelName(header->getString(header->getLabel(label_id)->name_index));

                 sstr << ".function ";
                 encodeStringLiteral(sstr, (labelName + uniqueSuffixStr).c_str());
                 sstr << "\n\n" << labelName << uniqueSuffixStr;
                 break;
            }
            case ISA_LABEL:
            {
                 sstr << header->getString(header->getLabel(label_id)->name_index);
                 break;
            }
            default:
                 break;
        }

        sstr << ":";
    }
    else
    {
        sstr << printPredicate(inst->opcode, inst->pred)
             << ISA_Inst_Table[opcode].str
             << " "
             << printExecutionSize(inst->opcode, inst->execsize);

        switch (opcode)
        {
            case ISA_JMP:
            case ISA_CALL:
            case ISA_GOTO:
            case ISA_FCALL:
            {
                /// label / function id to jump / call to.
                label_id = getPrimitiveOperand<uint16_t>(inst, i++);

                if (opcode == ISA_FCALL)
                {
                    /// function name in string
                    sstr << " " << header->getString(label_id);
                }
                else
                {
                    sstr << " " << header->getString(header->getLabel(label_id)->name_index);
                    if (header->getLabel(label_id)->kind == LABEL_SUBROUTINE) sstr << "_" << label_id;
                }

                if (opcode == ISA_FCALL)
                {
                    /// arg size
                    sstr << " " << getPrimitiveOperand<unsigned>(inst, i++);

                    /// return size
                    sstr << " " << getPrimitiveOperand<unsigned>(inst, i++);
                }

                break;
            }
            case ISA_IFCALL:
            {
                sstr << printOperand(header, inst, i++, opt);
                /// arg size
                sstr << " " << getPrimitiveOperand<unsigned>(inst, i++);
                /// return size
                sstr << " " << getPrimitiveOperand<unsigned>(inst, i++);
                break;
            }
            case ISA_FADDR:
            {
                /// symbol name in string
                sstr << header->getString(getPrimitiveOperand<uint16_t>(inst, i++));
                /// dst
                sstr << printOperand(header, inst, i++, opt);
                break;
            }
            case ISA_SWITCHJMP:
            {
                /// skip num_labels
                i++;
                /// index
                sstr << printOperand(header, inst, i++, opt);
                sstr << " (";
                for (bool first = true; i < inst->opnd_count; i++)
                {
                    if (!first) { sstr << ", "; }
                    label_id = getPrimitiveOperand<uint16_t>(inst, i);
                    sstr << header->getString(header->getLabel(label_id)->name_index);
                    if (first) { first = false; }
                }
                sstr << ")";
                break;
            }
            default:
                break; // Prevent gcc warning
        }
    }

    return sstr.str();
}

static std::string printInstructionMisc(
    const print_format_provider_t* header,
    const CISA_INST* inst,
    const Options *opt)
{
    ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
    unsigned i = 0;

    std::stringstream sstr;

    if (opcode == ISA_3D_URB_WRITE)
    {
        sstr << printPredicate(inst->opcode, inst->pred);
    }

    switch (opcode)
    {
        case ISA_FILE:
        {
            uint32_t filename_index = getPrimitiveOperand<uint32_t>(inst, i++);
            sstr << "FILE ";
            encodeStringLiteral(sstr, header->getString(filename_index));
            break;
        }
        case ISA_LOC:
        {
            unsigned line_number = getPrimitiveOperand<unsigned>(inst, i++);
            sstr << "LOC " << line_number;
            break;
        }
        case ISA_RAW_SEND:
        {
            uint8_t modifiers = inst->modifier;
            i++; // skip the modifier
            uint32_t exMsgDesc = getPrimitiveOperand<uint32_t>(inst, i++); //32b
            uint8_t numSrc    = getPrimitiveOperand<uint8_t>(inst, i++);
            uint8_t numDst    = getPrimitiveOperand<uint8_t>(inst, i++);
            std::string opstring = modifiers == 1? "raw_sendc " : "raw_send ";

            sstr << printPredicate(inst->opcode, inst->pred)
                 << opstring.c_str()
                 << printExecutionSize(inst->opcode, inst->execsize)
                 << " "
                 << "0x" << std::hex << (uint32_t)exMsgDesc << std::dec
                 << " "
                 << (unsigned)numSrc
                 << " "
                 << (unsigned)numDst
                 << " ";

            /// desc
            sstr << printOperand(header, inst, i++, opt);

            /// src
            sstr << printOperand(header, inst, i++, opt);

            /// dst
            sstr << printOperand(header, inst, i++, opt);

            break;
        }
        case ISA_RAW_SENDS:
        {
            uint8_t modifiers = inst->modifier;
            i++; // skip the modifier
            uint8_t numSrc0    = getPrimitiveOperand<uint8_t>(inst, i++);
            uint8_t numSrc1    = getPrimitiveOperand<uint8_t>(inst, i++);
            uint8_t numDst     = getPrimitiveOperand<uint8_t>(inst, i++);
            std::string opstring = (modifiers & 0x1) == 1? "raw_sendsc." : "raw_sends.";

            sstr << printPredicate(inst->opcode, inst->pred)
                 << opstring.c_str();

            uint8_t ffid = getPrimitiveOperand<uint8_t>(inst, i++);
            sstr << (unsigned)ffid
                << ".";

            if (modifiers & 0x2)
            {
                sstr << "eot.";
            }

            sstr << (unsigned)numSrc0
                 << "."
                 << (unsigned)numSrc1
                 << "."
                 << (unsigned)numDst
                 << " "
                 << printExecutionSize(inst->opcode, inst->execsize)
                 << " ";

            /// exMsgDesc: could be imm or vector
            sstr << printOperand(header, inst, i++, opt);

            /// desc
            sstr << printOperand(header, inst, i++, opt);

            /// src0
            sstr << printOperand(header, inst, i++, opt);

            /// src1
            sstr << printOperand(header, inst, i++, opt);

            /// dst
            sstr << printOperand(header, inst, i++, opt);

            break;
        }
        case ISA_VME_FBR:
        {
             /// My typical pattern of printing these things doesn't work here since
             /// these VME instructions weirdly put the surface as the third operand.
             std::stringstream sstr1;

             /// uni input
             sstr1 << printOperand(header, inst, i++, opt);

             /// fbr input
             sstr1 << printOperand(header, inst, i++, opt);

             uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);

             std::stringstream sstr2;

             sstr2 << " T"
                   << (unsigned)surface
                   << " "
                   << sstr1.str();

             sstr << ISA_Inst_Table[opcode].str << " (";

             /// FBRMbMode
             sstr << printOperand(header, inst, i++, opt); sstr << ",";

             /// FBRSubMbShape
             sstr << printOperand(header, inst, i++, opt); sstr << ",";

             /// FBRSubPredMode
             sstr << printOperand(header, inst, i++, opt);

             sstr << ")";

             /// vme output
             sstr2 << printOperand(header, inst, i++, opt);

             sstr << sstr2.str();

             break;
        }
        case ISA_VME_IME:
        {
             uint8_t streamMode = getPrimitiveOperand<uint8_t>(inst, i++);
             uint8_t searchCtrl = getPrimitiveOperand<uint8_t>(inst, i++);

             sstr << ISA_Inst_Table[opcode].str
                  << "(" << (unsigned)streamMode
                  << "," << (unsigned)searchCtrl
                  << ")";

             std::stringstream sstr1;

             /// uni imput
             sstr1 << printOperand(header, inst, i++, opt);

             /// ime input
             sstr1 << printOperand(header, inst, i++, opt);

             uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);

             sstr << " T" << (unsigned)surface << " " << sstr1.str();

             /// ref0
             sstr << printOperand(header, inst, i++, opt);

             /// ref1
             sstr << printOperand(header, inst, i++, opt);

             /// cost center
             sstr << printOperand(header, inst, i++, opt);

             /// vme output
             sstr << printOperand(header, inst, i++, opt);

             break;
        }
        case ISA_VME_SIC:
        {
             /// My typical pattern of printing these things doesn't work here since
             /// these VME instructions weirdly put the surface as the third operand.
             std::stringstream sstr1;

             /// uni input
             sstr1 << printOperand(header, inst, i++, opt);

             /// sic input
             sstr1 << printOperand(header, inst, i++, opt);

             uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);

             sstr << ISA_Inst_Table[opcode].str
                  << " T"
                  << (unsigned)surface
                  << " "
                  << sstr1.str();

             /// vme output
             sstr << printOperand(header, inst, i++, opt);

             break;
        }
        case ISA_VME_IDM:
        {
             /// My typical pattern of printing these things doesn't work here since
             /// these VME instructions weirdly put the surface as the third operand.
             std::stringstream sstr1;

             /// uni input
             sstr1 << printOperand(header, inst, i++, opt);

             /// sic input
             sstr1 << printOperand(header, inst, i++, opt);

             uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);

             sstr << ISA_Inst_Table[opcode].str
                  << " T"
                  << (unsigned)surface
                  << " "
                  << sstr1.str();

             /// vme output
             sstr << printOperand(header, inst, i++, opt);

             break;
        }
        case ISA_3D_URB_WRITE:
        {

            sstr << ISA_Inst_Table[opcode].str;

            sstr << " " << printExecutionSize(inst->opcode, inst->execsize);

            // num out
            sstr << " " << printOperand(header, inst, i++, opt);

            // channel mask
            // FIXME: change the order of channel mask and global offset in vISA binary
            std::string channelMask = printOperand(header, inst, i++, opt);

            // global offset
            sstr << " " << printOperand(header, inst, i++, opt);
            sstr << channelMask;

            // urb handle
            sstr << printOperand(header, inst, i++, opt);

            // per slot offset
            sstr << printOperand(header, inst, i++, opt);

            // vertex data
            sstr << printOperand(header, inst, i++, opt);

            break;
        }
        case ISA_LIFETIME:
        {
            uint8_t properties = getPrimitiveOperand<uint8_t>(inst, i++);
            uint32_t varId = getPrimitiveOperand<uint32_t>(inst, i++);

            sstr << ISA_Inst_Table[opcode].str;

            sstr << ".";

            if((VISAVarLifetime)(properties & 1) == LIFETIME_START)
            {
                sstr << "start ";
            }
            else
            {
                sstr << "end ";
            }

            // Since variable id is in non-standard form, we cannot invoke
            // printOperand directly on it
            unsigned char type = (properties >> 4) & 0x3;
            if(type == OPERAND_GENERAL)
            {
                // General variable
                sstr << printVariableDeclName(header, varId, opt, NOT_A_STATE_OPND);
            }
            else if(type == OPERAND_ADDRESS)
            {
                // Address variable
                sstr << "A" << varId;
            }
            else if(type == OPERAND_PREDICATE)
            {
                // Predicate variable
                sstr << "P" << varId;
            }

            break;
        }
        default:
        {
            ASSERT_USER(0, "Unimplemented or Illegal Misc Opcode.");
        }
    }

    return sstr.str();
}

// For 3D sampler instructions, subOpcode, pixel null mask and CPS LOD
// compensation enable share the same byte:
//
// Bit 0-4: subOpcode
// Bit   5: pixelNullMask
// Bit   6: cpsEnable
//
static VISA3DSamplerOp getSubOpcodeByte(
    const CISA_INST* inst, unsigned i)
{
    uint8_t val = getPrimitiveOperand<uint8_t>(inst, i);
    return VISA3DSamplerOp::extractSamplerOp(val);
}

static std::string printInstructionSampler(
    const print_format_provider_t* header,
    const CISA_INST* inst,
    const Options *opt)
{
    std::stringstream sstr;

    ISA_Opcode opcode = (ISA_Opcode)inst->opcode;

    if (opcode == ISA_3D_SAMPLE || opcode == ISA_3D_LOAD || opcode == ISA_3D_GATHER4)
    {
        sstr << printPredicate(inst->opcode, inst->pred);
    }

    unsigned i = 0;

    switch (opcode)
    {
        case ISA_LOAD:
        case ISA_SAMPLE:
        {
            uint8_t mod     = getPrimitiveOperand<uint8_t>(inst, i++);
            uint8_t sampler = 0;

            if (opcode == ISA_SAMPLE)
                sampler = getPrimitiveOperand<uint8_t>(inst, i++);

            uint8_t surface   = getPrimitiveOperand<uint8_t>(inst, i++);
            uint8_t channel   = (mod     ) & 0xF;
            uint8_t SIMD_mode = (mod >> 4) & 0x3;

            if((unsigned)SIMD_mode == 0)
            {
                SIMD_mode = 8;
            }
            else if((unsigned)SIMD_mode == 1)
            {
                SIMD_mode = 16;
            }

            sstr << ISA_Inst_Table[opcode].str
                 << "."
                 << channel_mask_str[channel]
                 << " ("
                 << (unsigned)SIMD_mode
                 << ")";

            if (opcode == ISA_SAMPLE)
                sstr << " S" << (unsigned)sampler;

            sstr << " T"
                 << (unsigned)surface;

            /// u offset
            sstr << printOperand(header, inst, i++, opt);

            /// v offset
            sstr << printOperand(header, inst, i++, opt);

            /// r offset
            sstr << printOperand(header, inst, i++, opt);

            /// dst
            sstr << printOperand(header, inst, i++, opt);

            break;
        }
        case ISA_3D_SAMPLE:
        {
            // [(P)] SAMPLE_3d[.pixel_null_mask][.cps][.divS].<channels> (exec_size)
            //   [(u_aoffimmi, v_aoffimii, r_aoffimmi)] <sampler> <surface>
            //   <dst> <u> <v> <r> <ai>
            auto subop = getSubOpcodeByte(inst, i++);

            sstr << SAMPLE_OP_3D_NAME[subop.opcode] << ".";
            // Print the pixel null mask if it is enabled.
            if (subop.pixelNullMask)
            {
                sstr << "pixel_null_mask.";
            }
            // Print CPS LOD compensation if it is enabled.
            // The last '.' is for the channels.
            if (subop.cpsEnable)
            {
                sstr << "cps.";
            }
            if (subop.nonUniformSampler)
            {
                sstr << "divS.";
            }

            uint8_t channels = getPrimitiveOperand<uint8_t>(inst, i++);
            if( channels & 0x1) sstr << "R";
            if( channels & 0x2) sstr << "G";
            if( channels & 0x4) sstr << "B";
            if( channels & 0x8) sstr << "A";

            sstr << " " << printExecutionSize(inst->opcode, inst->execsize) << " ";

            sstr << printOperand(header, inst, i++, opt);

            // sampler
            sstr << " S" << printOperand(header, inst, i++, opt);

            // surface
            sstr << " T" << printOperand(header, inst, i++, opt);

            // dst
            sstr << printOperand(header, inst, i++, opt);

            // skip the param count
            i++;

            while (i < inst->opnd_count)
            {
                sstr << printOperand(header, inst, i++, opt);
            }

            break;
        }
        case ISA_3D_LOAD:
        {
            auto subop = getSubOpcodeByte(inst, i++);

            sstr << SAMPLE_OP_3D_NAME[subop.opcode] << ".";
            // Print the pixel null mask if it is enabled.
            // The last '.' is for the channels.
            if (subop.pixelNullMask)
            {
                sstr << "pixel_null_mask.";
            }

            uint8_t channels = getPrimitiveOperand<uint8_t>(inst, i++);
            if( channels & 0x1) sstr << "R";
            if( channels & 0x2) sstr << "G";
            if( channels & 0x4) sstr << "B";
            if( channels & 0x8) sstr << "A";

            sstr << " " << printExecutionSize(inst->opcode, inst->execsize) << " ";

            sstr << printOperand(header, inst, i++, opt);

            // surface
            sstr << " T" << printOperand(header, inst, i++, opt);

            // dst
            sstr << printOperand(header, inst, i++, opt);

            // skip the param count
            i++;

            while (i < inst->opnd_count)
            {
                sstr << printOperand(header, inst, i++, opt);
            }

            break;
        }
        case ISA_3D_GATHER4:
        {
            auto subop = getSubOpcodeByte(inst, i++);

            sstr << SAMPLE_OP_3D_NAME[subop.opcode] << ".";
            // Print the pixel null mask if it is enabled.
            // The last '.' is for the channels.
            if (subop.pixelNullMask)
            {
                sstr << "pixel_null_mask.";
            }

            uint8_t channels = getPrimitiveOperand<uint8_t>(inst, i++);
            if (channels == 0x0)
            {
                sstr << "R";
            }
            else if (channels == 0x1)
            {
                sstr << "G";
            }
            else if (channels == 0x2)
            {
                sstr << "B";
            }
            else if (channels == 0x3)
            {
                sstr << "A";
            }
            else
            {
                sstr << "illegal";
            }

            sstr << " " << printExecutionSize(inst->opcode, inst->execsize);

            sstr << printOperand(header, inst, i++, opt);

            // sampler
            sstr << " S" << printOperand(header, inst, i++, opt);

            // surface
            sstr << " T" << printOperand(header, inst, i++, opt);

            // dst
            sstr << printOperand(header, inst, i++, opt);

            // skip the param count
            i++;

            while (i < inst->opnd_count)
            {
                sstr << printOperand(header, inst, i++, opt);
            }

            break;
        }
        case ISA_3D_INFO:
        {
            VISASampler3DSubOpCode subop = (VISASampler3DSubOpCode)getPrimitiveOperand<uint8_t>(inst, i++);
            sstr << SAMPLE_OP_3D_NAME[subop];
            sstr << " " << printExecutionSize(inst->opcode, inst->execsize) << " ";

            if (subop == VISA_3D_RESINFO)
            {
                // channelMask
                uint8_t channels = getPrimitiveOperand<uint8_t>(inst, i++);
                ChannelMask chMask = ChannelMask::createFromBinary(ISA_3D_INFO, channels);
                sstr << chMask.getString();
            }
            // surface
            sstr << " T" << printOperand(header, inst, i++, opt);

            if (subop == VISA_3D_RESINFO)
            {
                // lod
                sstr << printOperand(header, inst, i++, opt);
            }

            // dst
            sstr << printOperand(header, inst, i++, opt);

            break;
        }
        case ISA_SAMPLE_UNORM:
        {
            uint8_t channel = getPrimitiveOperand<uint8_t>(inst, i++);
            uint8_t sampler = getPrimitiveOperand<uint8_t>(inst, i++);
            uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
            sstr << ISA_Inst_Table[opcode].str
                 << "."
                 << channel_mask_str[(channel & 0xf)]
                 << "."
                 << sampler_channel_output_str[ChannelMask::getChannelOutputFormat(channel)]
                 << " S"
                 << (unsigned)sampler
                 << " T"
                 << (unsigned)surface;

            /// u offset
            sstr << printOperand(header, inst, i++, opt);

            /// v offset
            sstr << printOperand(header, inst, i++, opt);

            /// deltaU
            sstr << printOperand(header, inst, i++, opt);

            /// deltaV
            sstr << printOperand(header, inst, i++, opt);

            /// dst
            sstr << printOperand(header, inst, i++, opt);

            break;
        }
        case ISA_AVS:
        {
            uint8_t channel = getPrimitiveOperand<uint8_t>(inst, i++);
            uint8_t sampler = getPrimitiveOperand<uint8_t>(inst, i++);
            uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);

            sstr << ISA_Inst_Table[opcode].str
                 << "."
                 << channel_mask_str[channel]
                 << " T"
                 << (unsigned)surface
                 << " S"
                 << (unsigned)sampler;

            /// u offset
            sstr << printOperand(header, inst, i++, opt);

            /// v offset
            sstr << printOperand(header, inst, i++, opt);

            /// delta u
            sstr << printOperand(header, inst, i++, opt);

            /// delta v
            sstr << printOperand(header, inst, i++, opt);

            /// u2d
            sstr << printOperand(header, inst, i++, opt);

            /// groupID
            sstr << printOperand(header, inst, i++, opt);

            /// verticalBlockNumber
            sstr << printOperand(header, inst, i++, opt);

            uint8_t cntrl = ((getPrimitiveOperand<uint8_t>(inst, i++)) & 0xF);

            sstr << " "
                 << avs_control_str[cntrl];

            /// v2d
            sstr << printOperand(header, inst, i++, opt);

            uint8_t execMode  =       (getPrimitiveOperand<uint8_t>(inst, i++) & 0xF);

            sstr << " "
                 << avs_exec_mode[execMode];

            // eifbypass
            sstr << printOperand(header, inst, i++, opt);

            /// dst
            sstr << printOperand(header, inst, i++, opt);

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

                     sstr << ISA_Inst_Table[opcode].str
                          << "."
                          << va_sub_names[subOpcode]
                          << " T" << (unsigned)surface;

                     /// u offset
                     sstr << printOperand(header, inst, i++, opt);

                     /// v offset
                     sstr << printOperand(header, inst, i++, opt);

                     /// mmf mode
                     if (getVectorOperand(inst, i).getOperandClass() == OPERAND_IMMEDIATE)
                     {
                         unsigned val = getVectorOperand(inst, i++).opnd_val.const_opnd._val.ival;
                         sstr << " " << mmf_enable_mode[val];
                     }
                     else
                     {
                        sstr << printOperand(header, inst, i++, opt);
                     }

                     /// dst
                     sstr << printOperand(header, inst, i++, opt);

                     break;
                }
                case MINMAXFILTER_FOPCODE:
                {
                     uint8_t sampler = getPrimitiveOperand<uint8_t>(inst, i++);
                     uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);

                     sstr << ISA_Inst_Table[opcode].str
                          << "."
                          << va_sub_names[subOpcode]
                          << " T" << (unsigned)surface
                          << " S" << (unsigned)sampler;

                     /// u offset
                     sstr << printOperand(header, inst, i++, opt);

                     /// v offset
                     sstr << printOperand(header, inst, i++, opt);

                     uint8_t cntrl    = ((getPrimitiveOperand<uint8_t>(inst, i++)) & 0xF);
                     uint8_t execMode = ((getPrimitiveOperand<uint8_t>(inst, i++)) & 0xF);

                     sstr << " "
                          << avs_control_str [ cntrl     ]
                          << " "
                          << mmf_exec_mode   [ execMode ];

                     /// mmf mode
                     sstr << printOperand(header, inst, i++, opt);

                     /// dst
                     sstr << printOperand(header, inst, i++, opt);

                     break;
                }
                case BoolCentroid_FOPCODE:
                case Centroid_FOPCODE:
                {
                     uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);

                     sstr << ISA_Inst_Table[opcode].str
                          << "."
                          << va_sub_names[subOpcode]
                          << " T" << (unsigned)surface;

                     /// u offset
                     sstr << printOperand(header, inst, i++, opt);

                     /// v offset
                     sstr << printOperand(header, inst, i++, opt);

                     /// v size
                     sstr << printOperand(header, inst, i++, opt);

                     /// h size
                     if (subOpcode == BoolCentroid_FOPCODE)
                        sstr << printOperand(header, inst, i++, opt);

                     /// dst
                     sstr << printOperand(header, inst, i++, opt);

                     break;
                }
                case Convolve_FOPCODE:
                case Dilate_FOPCODE:
                case ERODE_FOPCODE:
                {
                     uint8_t sampler = getPrimitiveOperand<uint8_t>(inst, i++);
                     uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);

                     sstr << ISA_Inst_Table[opcode].str
                          << "."
                          << va_sub_names[subOpcode]
                          << " T" << (unsigned)surface
                          << " S" << (unsigned)sampler;

                     /// u offset
                     sstr << printOperand(header, inst, i++, opt);

                     /// v offset
                     sstr << printOperand(header, inst, i++, opt);

                     uint8_t execMode   =  getPrimitiveOperand<uint8_t>(inst, i  ) & 0x3;
                     uint8_t regionSize = (getPrimitiveOperand<uint8_t>(inst, i++) & 0xC) >> 0x2;

                     sstr << " "
                          << (Convolve_FOPCODE == subOpcode ?
                              conv_exec_mode [execMode]     :
                              ed_exec_mode   [execMode]    );

                     if (Convolve_FOPCODE == subOpcode)
                         sstr << " " << (regionSize & 0x1 ? "31x31" : "15x15");

                     /// dst
                     sstr << printOperand(header, inst, i++, opt);

                     break;
                }
                default:
                     ASSERT_USER(false, "Invalid VA sub-opcode");
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

                     sstr << ISA_Inst_Table[opcode].str
                          << "."
                          << va_sub_names[subOpcode]
                          << " T" << (unsigned)surface;

                     /// u offset
                     sstr << printOperand(header, inst, i++, opt);

                     /// v offset
                     sstr << printOperand(header, inst, i++, opt);

                     /// disparity
                     sstr << printOperand(header, inst, i++, opt);

                     /// dst
                     sstr << printOperand(header, inst, i++, opt);

                     break;
                }
                case VA_OP_CODE_1PIXEL_CONVOLVE:
                case VA_OP_CODE_1D_CONVOLVE_VERTICAL:
                case VA_OP_CODE_1D_CONVOLVE_HORIZONTAL:
                {
                     uint8_t sampler = getPrimitiveOperand<uint8_t>(inst, i++);
                     uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);

                     sstr << ISA_Inst_Table[opcode].str
                          << "."
                          << va_sub_names[subOpcode]
                          << " T" << (unsigned)surface
                          << " S" << (unsigned)sampler;

                     /// u offset
                     sstr << printOperand(header, inst, i++, opt);

                     /// v offset
                     sstr << printOperand(header, inst, i++, opt);

                     uint8_t mode = ((getPrimitiveOperand<uint8_t>(inst, i++)) & 0xF);

                     switch (mode & 0x3)
                     {
                         case 0: sstr << " 4x16"; break;
                         case 2: sstr << " 1x16"; break;
                         case 3: sstr << " 1x1";  break;
                     }

                     /// offsets
                     if (subOpcode == VA_OP_CODE_1PIXEL_CONVOLVE)
                         sstr << printOperand(header, inst, i++, opt);

                     /// dst
                     sstr << printOperand(header, inst, i++, opt);

                     break;
                }
                case VA_OP_CODE_LBP_CREATION:
                {
                     uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);

                     sstr << ISA_Inst_Table[opcode].str
                          << "."
                          << va_sub_names[subOpcode]
                          << " T" << (unsigned)surface;

                     /// u offset
                     sstr << printOperand(header, inst, i++, opt);

                     /// v offset
                     sstr << printOperand(header, inst, i++, opt);

                     uint8_t mode = ((getPrimitiveOperand<uint8_t>(inst, i++)) & 0xF);

                     sstr << " " << lbp_creation_mode[(int)mode];

                     /// dst
                     sstr << printOperand(header, inst, i++, opt);

                     break;
                }
                case VA_OP_CODE_FLOOD_FILL:
                {
                     uint8_t is8Connect = getPrimitiveOperand<uint8_t>(inst, i++);

                     sstr << ISA_Inst_Table[opcode].str
                          << "."
                          << va_sub_names[subOpcode]
                          << " " << (is8Connect & 0x1 ? "8_connect" : "4_connect");

                     /// pixel mask h direction
                     sstr << printOperand(header, inst, i++, opt);

                     /// pixel mask v left direction
                     sstr << printOperand(header, inst, i++, opt);

                     /// pixel mask v right direction
                     sstr << printOperand(header, inst, i++, opt);

                     /// loop count
                     sstr << printOperand(header, inst, i++, opt);

                     /// dst
                     sstr << printOperand(header, inst, i++, opt);

                     break;
                }
                case VA_OP_CODE_CORRELATION_SEARCH:
                {
                     uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);

                     sstr << ISA_Inst_Table[opcode].str
                          << "."
                          << va_sub_names[subOpcode]
                          << " T" << (unsigned)surface;

                     /// u offset
                     sstr << printOperand(header, inst, i++, opt);

                     /// v offset
                     sstr << printOperand(header, inst, i++, opt);

                     /// vertical origin
                     sstr << printOperand(header, inst, i++, opt);

                     /// horizontal origin
                     sstr << printOperand(header, inst, i++, opt);

                     /// x direction size
                     sstr << printOperand(header, inst, i++, opt);

                     /// y direction size
                     sstr << printOperand(header, inst, i++, opt);

                     /// x direction search size
                     sstr << printOperand(header, inst, i++, opt);

                     /// y direction search size
                     sstr << printOperand(header, inst, i++, opt);

                     /// dst
                     sstr << printOperand(header, inst, i++, opt);

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
                        sstr << ISA_Inst_Table[opcode].str
                             << "."
                             << va_sub_names[subOpcode];

                        if (subOpcode != ISA_HDC_LBPCORRELATION &&
                            subOpcode != ISA_HDC_LBPCREATION)
                        {
                            uint8_t sampler = getPrimitiveOperand<uint8_t>(inst, i++);
                            uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);

                            sstr << " T" << (unsigned)surface
                                 << " S" << (unsigned)sampler;
                        }
                        else
                        {
                            /// surface
                            uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
                            sstr << " T" << (unsigned)surface;
                        }


                        /// u offset
                        sstr << printOperand(header, inst, i++, opt);

                        /// v offset
                        sstr << printOperand(header, inst, i++, opt);


                        if (subOpcode == ISA_HDC_CONV ||
                            subOpcode == ISA_HDC_MMF ||
                            subOpcode == ISA_HDC_1PIXELCONV ||
                            subOpcode == ISA_HDC_1DCONV_H ||
                            subOpcode == ISA_HDC_1DCONV_V)
                        {
                            //pixel size
                            uint8_t pixel_size = getPrimitiveOperand<uint8_t>(inst, i++);
                            int isBigKernel = 0;

                            if (subOpcode == ISA_HDC_CONV)
                            {
                                isBigKernel = (pixel_size & ( 1 << 4 ));
                                pixel_size = pixel_size & 0xF;
                            }
                            sstr << " " << pixel_size_str[pixel_size];

                            if (subOpcode == ISA_HDC_CONV  && isBigKernel)
                            {
                                sstr << " 31x31";
                            }
                            else if (subOpcode == ISA_HDC_CONV  && !isBigKernel)
                            {
                                sstr << " 15x15";
                            }
                        }

                        if (subOpcode == ISA_HDC_MMF)
                        {
                            //mode
                            uint8_t mode = getPrimitiveOperand<uint8_t>(inst, i++);
                            sstr << " " << mmf_enable_mode[(int)mode];
                        }

                        if (subOpcode == ISA_HDC_LBPCREATION)
                        {
                            //mode
                            uint8_t mode = getPrimitiveOperand<uint8_t>(inst, i++);
                            sstr << " " << lbp_creation_mode[(int)mode];
                        }

                        if (subOpcode == ISA_HDC_LBPCORRELATION)
                        {
                            /// disparity
                            sstr << printOperand(header, inst, i++, opt);
                        }

                        if (subOpcode == ISA_HDC_1PIXELCONV)
                        {
                            /// offsets
                            sstr << printOperand(header, inst, i++, opt);
                        }

                        /// dst surface
                        uint8_t dst_surface = getPrimitiveOperand<uint8_t>(inst, i++);

                        sstr << " T" << (unsigned)dst_surface;

                        /// x offset
                        sstr << printOperand(header, inst, i++, opt);

                        /// y offset
                        sstr << printOperand(header, inst, i++, opt);
                        break;
                }
                default:
                     ASSERT_USER(false, "Invalid VA sub-opcode");
            }

            break;
        }
        default: ASSERT_USER( false, "illegal opcode for sampler instruction");
    }

    return sstr.str();
}

static std::string printSurfaceIndex(uint8_t surface)
{
    std::stringstream sstr;
    unsigned numPreDefinedSurfs = Get_CISA_PreDefined_Surf_Count();
    if (surface < numPreDefinedSurfs)
    {
        sstr << " " << vISAPreDefSurf[surface].name;
    }
    else
    {
        sstr << " " << "T" << (unsigned)surface;
    }
    return sstr.str();
}

static std::string printInstructionDataport(
    const print_format_provider_t* header,
    const CISA_INST* inst,
    const Options *opt)
{
    ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
    unsigned i = 0;

    uint8_t surface  = 0;
    uint8_t modifier = 0;
    std::stringstream sstr;

    switch (opcode) {
    default:
        break;
    case ISA_3D_RT_WRITE:
    case ISA_GATHER4_SCALED:
    case ISA_SCATTER4_SCALED:
    case ISA_GATHER_SCALED:
    case ISA_SCATTER_SCALED:
    case ISA_DWORD_ATOMIC:
    case ISA_3D_TYPED_ATOMIC:
        sstr << printPredicate(inst->opcode, inst->pred);
        break;
    }

    sstr << ISA_Inst_Table[opcode].str;

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
                modifier = getPrimitiveOperand<uint8_t>(inst, i++); //inst->modifier;
            }

            surface = getPrimitiveOperand<uint8_t>(inst, i++);

            if (ISA_MEDIA_LD == opcode || ISA_MEDIA_ST == opcode)
            {
                plane = getPrimitiveOperand<uint8_t>(inst, i++);
            }

            if (opcode == ISA_MEDIA_LD) sstr << "." << media_ld_mod_str[modifier];
            if (opcode == ISA_MEDIA_ST) sstr << "." << media_st_mod_str[modifier];

            sstr << " (";
            block_width = getPrimitiveOperand<uint8_t>(inst, i++);
            sstr << (unsigned)block_width;
            sstr << ",";
            block_height = getPrimitiveOperand<uint8_t>(inst, i++);
            sstr << (unsigned)block_height;
            sstr << ")";

            sstr << " T" << (unsigned)surface;
            sstr << " " << (unsigned)plane;

            /// x offset
            sstr << printOperand(header, inst, i++, opt);

            /// y offset
            sstr << printOperand(header, inst, i++, opt);

            /// message operand (src or dst)
            sstr << printOperand(header, inst, i++, opt);

            break;
        }
        case ISA_OWORD_ST:
        case ISA_OWORD_LD:
        case ISA_OWORD_LD_UNALIGNED:
        {
            uint8_t size = getPrimitiveOperand<uint8_t>(inst, i++);
            size = size & 0x7;
            unsigned num_oword = Get_VISA_Oword_Num((VISA_Oword_Num)size);

            if (ISA_OWORD_ST != opcode)
            {
                modifier = getPrimitiveOperand<uint8_t>(inst, i++);
                if (modifier & 0x1) sstr << ".mod";
            }

            sstr << " (" << num_oword << ")";

            surface = getPrimitiveOperand<uint8_t>(inst, i++);
            sstr <<printSurfaceIndex(surface);

            /// offset
            sstr << printOperand(header, inst, i++, opt);

            /// message operand (src or dst)
            sstr << printOperand(header, inst, i++, opt);

            break;
        }
        case ISA_GATHER:
        case ISA_SCATTER:
        {
            uint8_t elt_size = 0;
            uint8_t num_elts = 0;

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
                    ASSERT_USER( 0, "Incorrect element size for Gather/Scatter CISA inst." );
                    break;
            }
            if (ISA_GATHER == opcode)
            {
                modifier = getPrimitiveOperand<uint8_t>(inst, i++);
            }

            num_elts = getPrimitiveOperand<uint8_t>(inst, i++);

            // modifier
            if ( ISA_GATHER == opcode && modifier & 0x1 )
            {
                sstr << ".mod";
            }

            // num_elts
            sstr << "." << (unsigned)elt_size;

            // execution size
            sstr << " " << printExecutionSizeForScatterGather(num_elts);

            // modifier
            if (ISA_GATHER == opcode && modifier & 0x1)
            {
                sstr << ".mod";
            }

            //surface
            surface = getPrimitiveOperand<uint8_t>(inst, i++);
            sstr <<printSurfaceIndex(surface);

            /// global offset
            sstr << printOperand(header, inst, i++, opt);

            /// element offset
            sstr << printOperand(header, inst, i++, opt);

            /// message operand (src or dst)
            sstr << printOperand(header, inst, i++, opt);

            break;
        }
        case ISA_GATHER4_TYPED:
        case ISA_SCATTER4_TYPED:
        {
            ChannelMask chMask = ChannelMask::createFromBinary(opcode,
                    getPrimitiveOperand<uint8_t>(inst, i++));
            sstr << "." << chMask.getString();

            sstr << " " << printExecutionSize(inst->opcode, inst->execsize);

            surface = getPrimitiveOperand<uint8_t>(inst, i++);
            sstr <<printSurfaceIndex(surface);

            /// u offset
            sstr << printOperand(header, inst, i++, opt);

            /// v offset
            sstr << printOperand(header, inst, i++, opt);

            /// r offset
            sstr << printOperand(header, inst, i++, opt);

            /// lod
            sstr << printOperand(header, inst, i++, opt);

            /// message operand (src or dst)
            sstr << printOperand(header, inst, i++, opt);

            break;
        }
        case ISA_GATHER4_SCALED:
        case ISA_SCATTER4_SCALED:
        {
            ChannelMask chMask = ChannelMask::createFromBinary(opcode,
                    getPrimitiveOperand<uint8_t>(inst, i++));
            sstr << "." << chMask.getString();

            // ignore scale which must be 0
            (void) getPrimitiveOperand<uint8_t>(inst, i++);

            sstr << " " << printExecutionSize(inst->opcode, inst->execsize);

            /// surface
            surface = getPrimitiveOperand<uint8_t>(inst, i++);
            sstr <<printSurfaceIndex(surface);

            /// global offset
            sstr << printOperand(header, inst, i++, opt);

            /// offsets
            sstr << printOperand(header, inst, i++, opt);

            /// src/dst
            sstr << printOperand(header, inst, i++, opt);
            break;
        }
        case ISA_GATHER_SCALED:
        case ISA_SCATTER_SCALED:
        {
            VISA_SVM_Block_Num numBlocks;

            // block size : ignored
            (void)getPrimitiveOperand<uint8_t>(inst, i++);

            numBlocks = static_cast<VISA_SVM_Block_Num>(getPrimitiveOperand<uint8_t>(inst, i++));

            sstr << "." << Get_Common_ISA_SVM_Block_Num(numBlocks);

            sstr << " " << printExecutionSize(inst->opcode, inst->execsize);

            // scale (MBZ) : ignored
            (void)getPrimitiveOperand<uint8_t>(inst, i++);

            /// surface
            surface = getPrimitiveOperand<uint8_t>(inst, i++);
            sstr <<printSurfaceIndex(surface);

            /// global offset
            sstr << printOperand(header, inst, i++, opt);

            /// offsets
            sstr << printOperand(header, inst, i++, opt);

            /// src/dst
            sstr << printOperand(header, inst, i++, opt);
            break;
        }
        case ISA_3D_RT_WRITE:
        {
            // mode
            uint16_t mode = getPrimitiveOperand<uint16_t>(inst, i++);
            uint8_t surface;

            if( ( mode ) != 0 )
            {
                sstr << ".";
                if (mode & (0x1 << 2)) sstr << "<RTI>";
                if( mode & (0x1 << 0x3)) sstr << "<A>";
                if( mode & (0x1 << 0x4)) sstr << "<O>";
                if( mode & (0x1 << 0x5)) sstr << "<Z>";
                if( mode & (0x1 << 0x6)) sstr << "<ST>";
                if( mode & (0x1 << 0x7)) sstr << "<LRTW>";
                if( mode & (0x1 << 0x8)) sstr << "<CPS>";
                if( mode & (0x1 << 0x9)) sstr << "<PS>";
                if( mode & (0x1 << 0x10)) sstr << "<CM>";
                if (mode & (0x1 << 0x11)) sstr << "<SI>";
            }

            sstr << " " << printExecutionSize(inst->opcode, inst->execsize) << " ";

            // surface
            surface = getPrimitiveOperand<uint8_t>(inst, i++);
            sstr <<printSurfaceIndex(surface);

            while (i < inst->opnd_count)
            {
                sstr << printOperand(header, inst, i++, opt);
            }

            break;
        }
        case ISA_DWORD_ATOMIC: {
            printAtomicSubOpc(sstr, getPrimitiveOperand<uint8_t>(inst, i++));
            sstr << " " << printExecutionSize(inst->opcode, inst->execsize);

            /// surface
            surface = getPrimitiveOperand<uint8_t>(inst, i++);
            sstr <<printSurfaceIndex(surface);

            /// offsets
            sstr << printOperand(header, inst, i++, opt);

            /// src0
            sstr << printOperand(header, inst, i++, opt);

            /// src1
            sstr << printOperand(header, inst, i++, opt);

            /// dst
            sstr << printOperand(header, inst, i++, opt);
            break;
        }
        case ISA_3D_TYPED_ATOMIC:
        {
            printAtomicSubOpc(sstr, getPrimitiveOperand<uint8_t>(inst, i++));
            sstr << " " << printExecutionSize(inst->opcode, inst->execsize);

            /// surface
            surface = getPrimitiveOperand<uint8_t>(inst, i++);
            sstr << printSurfaceIndex(surface);

            /// u
            sstr << printOperand(header, inst, i++, opt);

            /// v
            sstr << printOperand(header, inst, i++, opt);

            /// r
            sstr << printOperand(header, inst, i++, opt);

            /// lod
            sstr << printOperand(header, inst, i++, opt);

            /// src0
            sstr << printOperand(header, inst, i++, opt);

            /// src1
            sstr << printOperand(header, inst, i++, opt);

            /// dst
            sstr << printOperand(header, inst, i++, opt);
            break;
        }

        default:
        {
            ASSERT_USER(false, "Unimplemented or Illegal DataPort Opcode.");
        }
    }

    return sstr.str();
}


std::string printKernelHeader(
    const common_isa_header& isaHeader,
    const print_format_provider_t* header,
    bool isKernel,
    int funcionId,
    const Options *options)
{
    std::stringstream sstr;

    uint8_t major_version = isaHeader.major_version;
    uint8_t minor_version = isaHeader.minor_version;

    sstr << ".version " << (unsigned)(major_version) << "." << (unsigned)(minor_version) << "\n";

    std::string name = header->getString(header->getNameIndex());
    std::replace_if(name.begin(), name.end(), [] (char c) { return c == '.'; } , ' ');

    sstr << (!isKernel ? ".global_function " : ".kernel ");
    encodeStringLiteral(sstr, name.c_str());
    sstr << "\n";

    /// Print all functions in the same object
    if (isKernel)
    {
        for (unsigned i = 0; i < isaHeader.num_functions; i++)
        {
            sstr << "\n" << ".funcdecl ";
            encodeStringLiteral(sstr, isaHeader.functions[i].name);
        }
    }

    // In asm text mode, declarations are printed at variable creation time, we dont need to print them here
    if (!options->getOption(vISA_IsaAssembly))
    {
        // For debug purposes only
        // Print the predefined variables as comments
        sstr << "\n" << "/// VISA Predefined Variables";
        for (unsigned i = 0; i < Get_CISA_PreDefined_Var_Count(); i++)
        {
            const var_info_t* predefVar = header->getPredefVar(i);
            if (predefVar->name_index != -1)
            {
                sstr << "\n" << "// .decl V" << i
                    << " v_type=G"
                    << " v_name=" << header->getString(predefVar->name_index);
            }
        }
        for (unsigned i = 0; i < Get_CISA_PreDefined_Surf_Count(); i++)
        {
            const state_info_t* predefSurface = header->getPredefSurface(i);
            if (predefSurface->name_index != -1)
            {
                sstr << "\n" << "// .decl T" << i
                    << " v_type=T"
                    << " v_name=" << header->getString(predefSurface->name_index);
            }
        }
        sstr << "\n";

        // emit var decls
        //.decl  V<#> name=<name> type=<type> num_elts=<num_elements> [align=<align>] [alias=(<alias_index>,<alias_offset>)]
        for (unsigned i = 0; i < header->getVarCount(); i++)
        {
            sstr << "\n" << printVariableDecl(isaHeader, header, i, isKernel, funcionId, options);
        }
        // address decls
        for (unsigned i = 0; i < header->getAddrCount(); i++)
        {
            sstr << "\n" << printAddressDecl(isaHeader, header, i);
        }
        // pred decls
        for (unsigned i = 0; i < header->getPredCount(); i++)
        {
            // P0 is reserved; starting from P1 if there is predicate decl
            sstr << "\n" << printPredicateDecl(header, i);
        }
        // sampler
        for (unsigned i = 0; i < header->getSamplerCount(); i++)
        {
            sstr << "\n" << printSamplerDecl(header, i);
        }
        // surface
        unsigned numPreDefinedSurfs = Get_CISA_PreDefined_Surf_Count();
        for (unsigned i = 0; i < header->getSurfaceCount(); i++)
        {
            sstr << "\n" << printSurfaceDecl(header, i, numPreDefinedSurfs);
        }
        // inputs to kernel
        for (unsigned i = 0; i < header->getInputCount(); i++)
        {
            sstr << "\n" << printFuncInput(header, i, isKernel, options);
        }
    }

    bool isTargetSet = false;
    for (unsigned i = 0; i < header->getAttrCount(); i++)
    {
        sstr << "\n" << printAttribute(header->getAttr(i), header, true);
        const char* attrName = header->getString(header->getAttr(i)->nameIndex);
        if (Attributes::isAttribute(Attributes::ATTR_Target, attrName))
        {
            isTargetSet = true;
        }
    }
    if (isTargetSet == false)
    {
        const char* attrName = Attributes::getAttributeName(Attributes::ATTR_Target);
        sstr << "\n" << ".kernel_attr " << attrName << "=";
        switch (options->getTarget()) {
        case VISA_CM: sstr << "\"cm\""; break;
        case VISA_3D: sstr << "\"3d\""; break;
        case VISA_CS: sstr << "\"cs\""; break;
        default:
            MUST_BE_TRUE(false, "Invalid kernel target attribute.");
            break;
        }
    }

    return sstr.str();
}

std::string printInstruction(
    const print_format_provider_t* header,
    const CISA_INST* instruction,
    const Options *opt)
{
    std::stringstream sstr;

    ISA_Opcode opcode = (ISA_Opcode)instruction->opcode;
    if (opcode != ISA_LOC || !g_ignorelocs)
    {
        if (opcode != ISA_LABEL)
        {
            sstr << "    ";
        }

        switch(ISA_Inst_Table[opcode].type)
        {
            case ISA_Inst_Mov:
            case ISA_Inst_Sync:
            case ISA_Inst_Arith:
            case ISA_Inst_Logic:
            case ISA_Inst_Compare:
            case ISA_Inst_Address:
            case ISA_Inst_SIMD_Flow: sstr << printInstructionCommon      (header, instruction, opt); break;
            case ISA_Inst_SVM:       sstr << printInstructionSVM         (header, instruction, opt); break;
            case ISA_Inst_Flow:      sstr << printInstructionControlFlow (header, instruction, opt); break;
            case ISA_Inst_Misc:      sstr << printInstructionMisc        (header, instruction, opt); break;
            case ISA_Inst_Sampler:   sstr << printInstructionSampler     (header, instruction, opt); break;
            case ISA_Inst_Data_Port: sstr << printInstructionDataport    (header, instruction, opt); break;
            default:
            {
                sstr << "Illegal or unimplemented CISA instruction (opcode, type): ("
                     << opcode << ", " << ISA_Inst_Table[opcode].type << ").";
                MUST_BE_TRUE(false, sstr.str());
            }
        }

        switch (opcode)
        {
            case ISA_LOC:
            case ISA_SUBROUTINE:
            case ISA_FILE:
            case ISA_LABEL: break;
            default:
            {
                std::stringstream sstr2;
                if (g_prettyPrint)
                for (int i = 0; i < (int)80 - (int)sstr.str().length(); i++)
                    sstr2 << ' ';
                if (!g_noinstid)
                    sstr << sstr2.str() << " /// $" << instruction->id;
            }
        }
    }
    else
    {
        sstr << "";
    }

    return sstr.str();
}

static std::string printRoutine(
    const common_isa_header& isaHeader,
    const print_format_provider_t* header,
    const std::list<CISA_INST*>& instructions,
    bool isKernel,
    int funcionId,
    const Options *options)
{
    std::stringstream sstr;
    sstr << printKernelHeader(isaHeader, header, isKernel, funcionId, options);

    for (auto I = instructions.begin(), E = instructions.end(); I != E; I++)
    {
        CISA_INST* inst = *I;
        if (((ISA_Opcode)inst->opcode) != ISA_LOC || !g_ignorelocs)
        {
            if (((ISA_Opcode)inst->opcode) != ISA_LABEL)
                sstr << "    ";
            sstr << printInstruction(header, inst, options) << "\n";
        }
    }

    return sstr.str();
}

std::string printKernel(
    const common_isa_header& isaHeader,
    const print_format_provider_t* header,
    const std::list<CISA_INST*>& instructions,
    const Options *opt)
{
    return printRoutine(isaHeader, header, instructions, true, 0, opt);
}

std::string printFunction(
    const common_isa_header& isaHeader,
    const print_format_provider_t* header,
    const std::list<CISA_INST*>& instructions,
    int funcionId,
    const Options *opt)
{
    return printRoutine(isaHeader, header, instructions, false, funcionId, opt);
}
