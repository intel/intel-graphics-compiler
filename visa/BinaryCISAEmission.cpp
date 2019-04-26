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

#include <fstream>
#include "BinaryCISAEmission.h"
#include "JitterDataStruct.h"
#include "VISAKernel.h"

#define SIZE_VALUE cisa_kernel->getBytesWritten()
#define SIZE_VALUE_INST cisa_kernel->getBytesWritten() - cisa_kernel->getKernelDataSize()
using namespace vISA;
int CBinaryCISAEmitter::Emit(VISAKernelImpl * cisa_kernel, unsigned int& binarySize)
{
    cisa_kernel->finalizeKernel();
    const kernel_format_t * kernelInfo = cisa_kernel->getKernelFormat();

    cisa_kernel->writeInToCisaBinaryBuffer(&kernelInfo->string_count, sizeof(kernelInfo->string_count));

    for (uint32_t i = 0; i < kernelInfo->string_count; i++)
    {
        cisa_kernel->writeInToCisaBinaryBuffer(kernelInfo->strings[i], (int) strlen(kernelInfo->strings[i])+1);
    }

    DEBUG_PRINT_SIZE("size after string_count: ", SIZE_VALUE);

    cisa_kernel->writeInToCisaBinaryBuffer(&kernelInfo->name_index, sizeof(kernelInfo->name_index));

    DEBUG_PRINT_SIZE("size after name_index: ", SIZE_VALUE);

    cisa_kernel->writeInToCisaBinaryBuffer(&kernelInfo->variable_count, sizeof(kernelInfo->variable_count));

    for(uint32_t i = 0; i < kernelInfo->variable_count; i++)
    {
        emitVarInfo(cisa_kernel, &kernelInfo->variables[i]);
    }

    DEBUG_PRINT_SIZE("size after variables: ", SIZE_VALUE);

    cisa_kernel->writeInToCisaBinaryBuffer(&kernelInfo->address_count, sizeof(kernelInfo->address_count));

    for(int i = 0; i < kernelInfo->address_count; i++)
    {
        emitAddressInfo(cisa_kernel, &kernelInfo->addresses[i]);
    }

    DEBUG_PRINT_SIZE("size after addresses: ", SIZE_VALUE);

    cisa_kernel->writeInToCisaBinaryBuffer(&kernelInfo->predicate_count, sizeof(kernelInfo->predicate_count));

    for(int i = 0; i < kernelInfo->predicate_count; i++)
    {
        emitPredicateInfo(cisa_kernel, &kernelInfo->predicates[i]);
    }

    DEBUG_PRINT_SIZE("size after predicates: ", SIZE_VALUE);

    cisa_kernel->writeInToCisaBinaryBuffer(&kernelInfo->label_count, sizeof(kernelInfo->label_count));

    for(int i = 0; i < kernelInfo->label_count; i++)
    {
        emitLabelInfo(cisa_kernel, &kernelInfo->labels[i]);
    }

    DEBUG_PRINT_SIZE("size after labels: ", SIZE_VALUE);

    cisa_kernel->writeInToCisaBinaryBuffer(&kernelInfo->sampler_count, sizeof(kernelInfo->sampler_count));

    for(int i = 0; i <kernelInfo->sampler_count; i++)
    {
        emitStateInfo(cisa_kernel, &kernelInfo->samplers[i]);
    }

    DEBUG_PRINT_SIZE("size after samplers: ", SIZE_VALUE);

    cisa_kernel->writeInToCisaBinaryBuffer(&kernelInfo->surface_count, sizeof(kernelInfo->surface_count));

    for(int i = 0; i <kernelInfo->surface_count; i++)
    {
        emitStateInfo(cisa_kernel, &kernelInfo->surfaces[i]);
    }

    DEBUG_PRINT_SIZE("size after surfaces: ", SIZE_VALUE);

    cisa_kernel->writeInToCisaBinaryBuffer(&kernelInfo->vme_count, sizeof(kernelInfo->vme_count));

    for(int i = 0; i <kernelInfo->vme_count; i++)
    {
        emitStateInfo(cisa_kernel, &kernelInfo->vmes[i]);
    }

    DEBUG_PRINT_SIZE("size after VMEs: ", SIZE_VALUE);

    if (cisa_kernel->getIsKernel())
    {
        cisa_kernel->writeInToCisaBinaryBuffer(&kernelInfo->input_count, sizeof(kernelInfo->input_count));

        for(uint32_t i = 0; i < kernelInfo->input_count; i++)
        {
            emitInputInfo(cisa_kernel, &kernelInfo->inputs[i]);
        }

        DEBUG_PRINT_SIZE("size after inputs: ", SIZE_VALUE);
    }

    cisa_kernel->writeInToCisaBinaryBuffer(&kernelInfo->size, sizeof(kernelInfo->size));

    DEBUG_PRINT_SIZE("size after size: ", SIZE_VALUE);

    cisa_kernel->writeInToCisaBinaryBuffer(&kernelInfo->entry, sizeof(kernelInfo->entry));

    DEBUG_PRINT_SIZE("size after entry: ", SIZE_VALUE);

    if (!cisa_kernel->getIsKernel())
    {
        cisa_kernel->writeInToCisaBinaryBuffer(&kernelInfo->input_size, sizeof(kernelInfo->input_size));
        DEBUG_PRINT_SIZE("size after input size: ", SIZE_VALUE);

        cisa_kernel->writeInToCisaBinaryBuffer(&kernelInfo->return_type, sizeof(kernelInfo->return_type));
        DEBUG_PRINT_SIZE("size after return type: ", SIZE_VALUE);
    }

    cisa_kernel->writeInToCisaBinaryBuffer(&kernelInfo->attribute_count, sizeof(kernelInfo->attribute_count));

    for(int i = 0; i < kernelInfo->attribute_count; i++)
    {
        this->emitAttributeInfo(cisa_kernel, &kernelInfo->attributes[i]);
    }

    DEBUG_PRINT_SIZE("size after attributes: ", SIZE_VALUE);

    std::list<CisaFramework::CisaInst *>::iterator inst_iter = cisa_kernel->getInstructionListBegin();
    std::list<CisaFramework::CisaInst *>::iterator inst_iter_end = cisa_kernel->getInstructionListEnd();

    int status = CM_SUCCESS;
    for(; inst_iter != inst_iter_end; inst_iter++)
    {
        CisaFramework::CisaInst * inst = *inst_iter;

        CISA_INST * cisa_inst = inst->getCISAInst();
        const VISA_INST_Desc * inst_desc = inst->getCISAInstDesc();
        status = emitCisaInst(cisa_kernel, cisa_inst, inst_desc);
        if( status != CM_SUCCESS )
        {
            break;
        }
    }

    return status;
}

int CBinaryCISAEmitter::emitCisaInst(
    VISAKernelImpl* cisa_kernel, CISA_INST* inst, const VISA_INST_Desc* desc)
{
    bool     useSubDesc    = false;
    uint8_t  subOpcode     = 0;
    unsigned reverseOffset = 0;

    cisa_kernel->writeInToCisaBinaryBuffer(&desc->opcode, sizeof(desc->opcode));

    unsigned opndCount = desc->opnd_num;

    for (unsigned i = 0; i < opndCount; i++)
    {
        unsigned currendOpndIndex = i - reverseOffset;

        if (inst->opnd_array != NULL && inst->opnd_array[currendOpndIndex] == NULL)
        {
            CmAssert(0);
            return CM_FAILURE;
        }

        if (!useSubDesc && desc->opnd_desc[i].opnd_type == OPND_SUBOPCODE)
        {
            useSubDesc = true;
            subOpcode = getPrimitiveOperand<uint8_t>(inst, i);
            cisa_kernel->writeInToCisaBinaryBuffer(&subOpcode, sizeof(subOpcode));
            opndCount += desc->getSubInstDesc(subOpcode).opnd_num;
        }
        else if ((useSubDesc == false && desc->opnd_desc[i].opnd_type == OPND_EXECSIZE) ||
                 (useSubDesc == true  && desc->getSubInstDesc(subOpcode).opnd_desc[i-1].opnd_type == OPND_EXECSIZE))
        {
            cisa_kernel->writeInToCisaBinaryBuffer(&inst->execsize, sizeof(inst->execsize));
            reverseOffset++;
        }
        else if ((useSubDesc == false && desc->opnd_desc[i  ].opnd_type == OPND_PRED) ||
                 (useSubDesc == true  && desc->getSubInstDesc(subOpcode).opnd_desc[i-1].opnd_type == OPND_PRED))

        {
            cisa_kernel->writeInToCisaBinaryBuffer(&inst->pred, sizeof(inst->pred));
            reverseOffset++;
        }
        else if (inst->opnd_array != NULL && inst->opnd_array[currendOpndIndex]->opnd_type == CISA_OPND_OTHER)
        {
            cisa_kernel->writeInToCisaBinaryBuffer(&inst->opnd_array[currendOpndIndex]->_opnd.other_opnd, inst->opnd_array[currendOpndIndex]->size);
        }
        else if (inst->opnd_array != NULL && inst->opnd_array[currendOpndIndex]->opnd_type == CISA_OPND_VECTOR)
        {
            emitVectorOpnd(cisa_kernel, &inst->opnd_array[currendOpndIndex]->_opnd.v_opnd);
        }
        else if (inst->opnd_array != NULL && inst->opnd_array[currendOpndIndex]->opnd_type == CISA_OPND_RAW)
        {
            emitRawOpnd(cisa_kernel, &inst->opnd_array[currendOpndIndex]->_opnd.r_opnd);
        }
    }

    DEBUG_PRINT_SIZE_INSTRUCTION("size after instruction: ", desc->opcode, SIZE_VALUE_INST);
    return CM_SUCCESS;
}

void CBinaryCISAEmitter::emitVarInfo(VISAKernelImpl * cisa_kernel, var_info_t * var)
{
    cisa_kernel->writeInToCisaBinaryBuffer(&var->name_index, sizeof(var->name_index));
    cisa_kernel->writeInToCisaBinaryBuffer(&var->bit_properties, sizeof(var->bit_properties));
    cisa_kernel->writeInToCisaBinaryBuffer(&var->num_elements, sizeof(var->num_elements));
    cisa_kernel->writeInToCisaBinaryBuffer(&var->alias_index, sizeof(var->alias_index));
    cisa_kernel->writeInToCisaBinaryBuffer(&var->alias_offset, sizeof(var->alias_offset));

    cisa_kernel->writeInToCisaBinaryBuffer(&var->alias_scope_specifier, sizeof(var->alias_scope_specifier));

    cisa_kernel->writeInToCisaBinaryBuffer(&var->attribute_count, sizeof(var->attribute_count));

    for(int i = 0; i < var->attribute_count; i++)
    {
        emitAttributeInfo(cisa_kernel, &var->attributes[i]);
    }
}

void CBinaryCISAEmitter::emitStateInfo(VISAKernelImpl * cisa_kernel, state_info_t * var)
{
    cisa_kernel->writeInToCisaBinaryBuffer(&var->name_index, sizeof(var->name_index));
    cisa_kernel->writeInToCisaBinaryBuffer(&var->num_elements, sizeof(var->num_elements));
    cisa_kernel->writeInToCisaBinaryBuffer(&var->attribute_count, sizeof(var->attribute_count));

    for(int i = 0; i < var->attribute_count; i++)
    {
        emitAttributeInfo(cisa_kernel, &var->attributes[i]);
    }
}

void CBinaryCISAEmitter::emitAddressInfo(VISAKernelImpl * cisa_kernel, addr_info_t * addr)
{
    cisa_kernel->writeInToCisaBinaryBuffer(&addr->name_index, sizeof(addr->name_index));
    cisa_kernel->writeInToCisaBinaryBuffer(&addr->num_elements, sizeof(addr->num_elements));
    cisa_kernel->writeInToCisaBinaryBuffer(&addr->attribute_count, sizeof(addr->attribute_count));

    for(int i = 0; i < addr->attribute_count; i++)
    {
        emitAttributeInfo(cisa_kernel, &addr->attributes[i]);
    }
}

void CBinaryCISAEmitter::emitPredicateInfo(VISAKernelImpl * cisa_kernel, pred_info_t * pred)
{
    cisa_kernel->writeInToCisaBinaryBuffer(&pred->name_index, sizeof(pred->name_index));
    cisa_kernel->writeInToCisaBinaryBuffer(&pred->num_elements, sizeof(pred->num_elements));
    cisa_kernel->writeInToCisaBinaryBuffer(&pred->attribute_count, sizeof(pred->attribute_count));

    for(int i = 0; i < pred->attribute_count; i++)
    {
        emitAttributeInfo(cisa_kernel, &pred->attributes[i]);
    }
}

void CBinaryCISAEmitter::emitLabelInfo(VISAKernelImpl * cisa_kernel, label_info_t * lbl)
{
    cisa_kernel->writeInToCisaBinaryBuffer(&lbl->name_index, sizeof(lbl->name_index));
    cisa_kernel->writeInToCisaBinaryBuffer(&lbl->kind, sizeof(lbl->kind));
    cisa_kernel->writeInToCisaBinaryBuffer(&lbl->attribute_count, sizeof(lbl->attribute_count));

    for(int i = 0; i < lbl->attribute_count; i++)
    {
        emitAttributeInfo(cisa_kernel, &lbl->attributes[i]);
    }
}

void CBinaryCISAEmitter::emitInputInfo(VISAKernelImpl * cisa_kernel, input_info_t * in)
{
    cisa_kernel->writeInToCisaBinaryBuffer(&in->kind, sizeof(in->kind));
    cisa_kernel->writeInToCisaBinaryBuffer(&in->index, sizeof(in->index));
    cisa_kernel->writeInToCisaBinaryBuffer(&in->offset, sizeof(in->offset));
    cisa_kernel->writeInToCisaBinaryBuffer(&in->size, sizeof(in->size));
}

void CBinaryCISAEmitter::emitAttributeInfo(VISAKernelImpl *cisa_kernel, attribute_info_t * attr)
{
    cisa_kernel->writeInToCisaBinaryBuffer(&attr->nameIndex, sizeof(attr->nameIndex));
    cisa_kernel->writeInToCisaBinaryBuffer(&attr->size, sizeof(attr->size));

    if (attr->isInt)
    {
        switch (attr->size)
        {
            case sizeof(int8_t ): cisa_kernel->writeInToCisaBinaryBuffer((int8_t *)(&attr->value.intVal), attr->size); break;
            case sizeof(int16_t): cisa_kernel->writeInToCisaBinaryBuffer((int16_t*)(&attr->value.intVal), attr->size); break;
            case sizeof(int32_t): cisa_kernel->writeInToCisaBinaryBuffer((int32_t*)(&attr->value.intVal), attr->size); break;
            default:
                CmAssert(0);
                break;
        }
    }
    else
        cisa_kernel->writeInToCisaBinaryBuffer(attr->value.stringVal, attr->size);
}

void CBinaryCISAEmitter::emitVectorOpnd(VISAKernelImpl * cisa_kernel, vector_opnd * cisa_opnd)
{
    cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->tag, sizeof(cisa_opnd->tag));

    switch(cisa_opnd->tag & 0x7)
    {
    case OPERAND_GENERAL:
        {
            cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->opnd_val.gen_opnd.index, sizeof(cisa_opnd->opnd_val.gen_opnd.index));
            cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->opnd_val.gen_opnd.row_offset, sizeof(cisa_opnd->opnd_val.gen_opnd.row_offset));
            cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->opnd_val.gen_opnd.col_offset, sizeof(cisa_opnd->opnd_val.gen_opnd.col_offset));
            cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->opnd_val.gen_opnd.region, sizeof(cisa_opnd->opnd_val.gen_opnd.region));
            break;
        }
    case OPERAND_ADDRESS:
        {
            cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->opnd_val.addr_opnd.index, sizeof(cisa_opnd->opnd_val.addr_opnd.index));
            cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->opnd_val.addr_opnd.offset, sizeof(cisa_opnd->opnd_val.addr_opnd.offset));
            cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->opnd_val.addr_opnd.width, sizeof(cisa_opnd->opnd_val.addr_opnd.width));
            break;
        }
    case OPERAND_INDIRECT:
        {
            cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->opnd_val.indirect_opnd.index, sizeof(cisa_opnd->opnd_val.indirect_opnd.index));
            cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->opnd_val.indirect_opnd.addr_offset, sizeof(cisa_opnd->opnd_val.indirect_opnd.addr_offset));
            cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->opnd_val.indirect_opnd.indirect_offset, sizeof(cisa_opnd->opnd_val.indirect_opnd.indirect_offset));
            cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->opnd_val.indirect_opnd.bit_property, sizeof(cisa_opnd->opnd_val.indirect_opnd.bit_property));
            cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->opnd_val.indirect_opnd.region, sizeof(cisa_opnd->opnd_val.indirect_opnd.region));
            break;
        }
    case OPERAND_PREDICATE:
        {
            cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->opnd_val.pred_opnd.index, sizeof(cisa_opnd->opnd_val.pred_opnd.index));
            break;
        }
    case OPERAND_IMMEDIATE:
        {
            cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->opnd_val.const_opnd.type, sizeof(cisa_opnd->opnd_val.const_opnd.type));
            if(cisa_opnd->opnd_val.const_opnd.type == ISA_TYPE_DF)
                cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->opnd_val.const_opnd._val.dval, sizeof(double));
            else if(cisa_opnd->opnd_val.const_opnd.type == ISA_TYPE_Q || cisa_opnd->opnd_val.const_opnd.type == ISA_TYPE_UQ)
                cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->opnd_val.const_opnd._val.lval, sizeof(unsigned long long));
            else
                cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->opnd_val.const_opnd._val.ival, sizeof(unsigned int));
            break;
        }
    case OPERAND_STATE:
        {
            cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->opnd_val.state_opnd.opnd_class, sizeof(cisa_opnd->opnd_val.state_opnd.opnd_class));
            cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->opnd_val.state_opnd.index, sizeof(cisa_opnd->opnd_val.state_opnd.index));
            cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->opnd_val.state_opnd.offset, sizeof(cisa_opnd->opnd_val.state_opnd.offset));
            break;
        }
    default:
        {
            MUST_BE_TRUE( 0, "Invalid Vector Operand Class. Size cannot be determined." );
            break;
        }
    }
}
void CBinaryCISAEmitter::emitRawOpnd(VISAKernelImpl * cisa_kernel, raw_opnd * cisa_opnd)
{
    cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->index, sizeof(cisa_opnd->index));
    cisa_kernel->writeInToCisaBinaryBuffer(&cisa_opnd->offset, sizeof(cisa_opnd->offset));
}
