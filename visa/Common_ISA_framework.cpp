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

#include <sstream>
#include <fstream>
#include "visa_igc_common_header.h"
#include "Common_ISA.h"
#include "Common_ISA_util.h"
#include "Common_ISA_framework.h"
#include "JitterDataStruct.h"
#include "VISAKernel.h"

#include "IsaDisassembly.h"
#include "IsaVerification.h"

#if defined( _DEBUG ) && ( defined( _WIN32 ) || defined( _WIN64 ) )
#include <Windows.h>
#endif

using namespace std;

#define SIZE_VALUE m_kernel_data_size
#define SIZE_VALUE_INST m_instruction_size

namespace CisaFramework
{

int CisaInst::createCisaInstruction(
    ISA_Opcode opcode,
    unsigned char exec_size,
    unsigned char modifier,
    unsigned short pred,
    VISA_opnd** opnd,
    int numOpnds,
    const VISA_INST_Desc* inst_desc)
{
    uint8_t subOpcode = 0;
    bool hasSubOpcode = false;
    int descOpndCount = inst_desc->opnd_num;

    for (int i = 0; i < descOpndCount;  i++)
    {
        if (inst_desc->opnd_desc[i].opnd_type == OPND_SUBOPCODE)
        {
            descOpndCount += inst_desc->getSubInstDesc((uint8_t) opnd[i]->_opnd.other_opnd).opnd_num;
            hasSubOpcode = true;
            subOpcode = static_cast<uint8_t>(opnd[0]->_opnd.other_opnd);
            break;
        }
    }

    // TODO: move the FIXME below above this if statment and
    //      int implicitOperands = 0;
    //      for (...) // parent op descriptors
    //          if (type == EXEC_SIZE || type == PRED)
    //             implicitOperands++;
    //      for (...) // subop op descriptors
    //          if (type == EXEC_SIZE || type == PRED)
    //              implicitOperands++;
    // TODO: rename numOpnds to explicitOpnds to be clear
    // TODO: rename descOpndCount to descOpndsToEncodeCount
    //
    // compare (descOpndCount != explicitOpnds + implicitOperands)
    //    mismatch assert
    if (opcode != ISA_FCALL)
    {
        // != doens't work here because predication and exec size are treated
        // differently; it might be descOpndCount == numOpnds (+ HAS_EXEC) (+ HAS_PRED)
        // if (descOpndCount != numOpnds)
        if (descOpndCount < numOpnds)
        {
            string msg = "Number of operands mismatch between CISA instruction description and value passed in.";
            cerr << msg << ": " << descOpndCount << " " << numOpnds << endl;
            MUST_BE_TRUE(false, msg);
        }
    }

    m_inst_desc                   = inst_desc;
    m_cisa_instruction.opnd_count = numOpnds;
    m_cisa_instruction.opcode     = opcode;
    m_cisa_instruction.execsize   = exec_size;
    m_cisa_instruction.modifier   = modifier;
    m_cisa_instruction.pred       = pred;
    m_cisa_instruction.opnd_array = (VISA_opnd**)m_mem.alloc(sizeof(VISA_opnd*) * numOpnds);
    memcpy_s(m_cisa_instruction.opnd_array, sizeof(VISA_opnd*)* numOpnds, opnd, sizeof(VISA_opnd*)* numOpnds);
    m_cisa_instruction.isa_type = inst_desc->type;

    // FIXME: this is a mess and needs to be cleaned up
    //   I think this comment refers to merging the subopcode and opcode case
    //   that is we're hunting for EXEC_SIZE and PRED operands in both the
    //   suboperand and parent
    for (int i = 0; i < descOpndCount; i++)
    {
        if (inst_desc->opnd_desc[i].opnd_type == OPND_EXECSIZE ||
            inst_desc->opnd_desc[i].opnd_type == OPND_PRED)
        {
            m_size += static_cast<short>(Get_VISA_Type_Size((VISA_Type)inst_desc->opnd_desc[i].data_type));
        }
    }
    if (hasSubOpcode)
    {
        int numOpnd = inst_desc->getSubInstDesc(subOpcode).opnd_num;
        for (int i = 0; i < numOpnd; i++)
        {
            OpndDesc desc = inst_desc->getSubInstDesc(subOpcode).opnd_desc[i];
            if (desc.opnd_type == OPND_EXECSIZE || desc.opnd_type == OPND_PRED)
            {
                m_size += static_cast<short>(Get_VISA_Type_Size((VISA_Type)desc.data_type));
            }
        }
    }


    for (int i = 0; i < numOpnds; i++)
    {
        if (opnd[i] == NULL)
        {
            assert(0);
            cerr << "ONE OF THE OPERANDS IS NULL!" << endl;
            return VISA_FAILURE;
        }
        m_size += opnd[i]->size;
    }

    return VISA_SUCCESS;
}

void CisaBinary::initKernel( int kernelIndex, VISAKernelImpl * kernel )
{
    unsigned functionIndex = 0; // separating function and kernel index
    MUST_BE_TRUE( kernelIndex < (m_upper_bound_kernels + m_upper_bound_functions) && kernelIndex >= 0,
        "Invalid kernelIndex in CisaBinary initialization.\n");

    int nameLen = (int) strlen(kernel->getName());

    MUST_BE_TRUE( nameLen <= COMMON_ISA_MAX_FILENAME_LENGTH,
        "Kernel Name exceeds maximum length allowed.\n");



    if(this->getMajorVersion())
    {
        if(kernel->getIsKernel())
            kernelIndex = m_header.num_kernels++;
        else
        {
            m_header.num_functions++;
            kernel->GetFunctionId(functionIndex);
        }
    }

    if (kernel->getIsKernel())
    {
        m_header.kernels[kernelIndex].name_len = (unsigned char) nameLen;
        memcpy_s(&m_header.kernels[kernelIndex].name, COMMON_ISA_MAX_FILENAME_LENGTH, kernel->getName(), m_header.kernels[kernelIndex].name_len);

        m_header.kernels[kernelIndex].offset = 0; //will be set later during finalize
        m_header.kernels[kernelIndex].size = kernel->getCisaBinarySize();
        m_header.kernels[kernelIndex].cisa_binary_buffer = kernel->getCisaBinaryBuffer(); //buffer containing entire kernel
        m_header.kernels[kernelIndex].input_offset = kernel->getInputOffset(); //for now relative to the beginning of the kernel object

        //Workaround for patching in FE. This way space for data structures is allocated
        unsigned int numGenBinariesWillBePatched = kernel->getOptions()->getuInt32Option(vISA_NumGenBinariesWillBePatched);
        m_header.kernels[kernelIndex].num_gen_binaries = static_cast<unsigned char>(numGenBinariesWillBePatched);
        m_header.kernels[kernelIndex].gen_binaries = (gen_binary_info *)m_mem.alloc(sizeof(gen_binary_info)* (int)m_header.kernels[kernelIndex].num_gen_binaries);

        for (int i = 0; i < m_header.kernels[kernelIndex].num_gen_binaries; i++)
        {
            m_header.kernels[kernelIndex].gen_binaries[i].binary_offset = 0;
            m_header.kernels[kernelIndex].gen_binaries[i].binary_offset = 0;
            m_header.kernels[kernelIndex].gen_binaries[i].platform = 0;
        }
    }

    if (!kernel->getIsKernel())
    {

        m_header.functions[functionIndex].linkage = 0; // deprecated and MBZ
        m_header.functions[functionIndex].name_len = (unsigned char) nameLen;
        memcpy_s(&m_header.functions[functionIndex].name, COMMON_ISA_MAX_FILENAME_LENGTH, kernel->getName(), m_header.functions[functionIndex].name_len);

        m_header.functions[functionIndex].offset = 0; //will be set later during finalize
        m_header.functions[functionIndex].size = kernel->getCisaBinarySize();
        m_header.functions[functionIndex].cisa_binary_buffer = kernel->getCisaBinaryBuffer(); //buffer containing entire kernel
    }
}

unsigned long CisaBinary::writeInToCisaHeaderBuffer(const void * value, int size)
{
    MUST_BE_TRUE( m_bytes_written_cisa_buffer + size <= m_header_size,
        "Size of CISA instructions header buffer is exceeded." );

    memcpy_s( &m_header_buffer[m_bytes_written_cisa_buffer], size, value, size );
    m_bytes_written_cisa_buffer += size;

    return m_bytes_written_cisa_buffer;
}

/*
    calculates total size of the header and sets all the kernel offsets from the beginning of the file
*/
int CisaBinary::finalizeCisaBinary()
{
    m_bytes_written_cisa_buffer = 0;

    m_total_size = m_header_size = get_Size_Isa_Header( &m_header, getMajorVersion(), this->getMinorVersion() );

    m_header_buffer  = ( char * )this->m_mem.alloc(m_header_size);

    writeInToCisaHeaderBuffer(&m_header.magic_number, sizeof(m_header.magic_number));

    // we must use the latest vISA version since the internal data structures always use it
    uint8_t major = COMMON_ISA_MAJOR_VER;
    uint8_t minor = COMMON_ISA_MINOR_VER;
    writeInToCisaHeaderBuffer(&major, sizeof(uint8_t));
    writeInToCisaHeaderBuffer(&minor, sizeof(uint8_t));
    //writeInToCisaHeaderBuffer( &m_header.major_version, sizeof(m_header.major_version) );
    //writeInToCisaHeaderBuffer( &m_header.minor_version, sizeof(m_header.minor_version) );

    writeInToCisaHeaderBuffer( &m_header.num_kernels, sizeof(m_header.num_kernels) );

    for(int i = 0; i < m_header.num_kernels; i++)
    {
        writeInToCisaHeaderBuffer( &m_header.kernels[i].name_len, sizeof(m_header.kernels[i].name_len) );

        writeInToCisaHeaderBuffer( &m_header.kernels[i].name, m_header.kernels[i].name_len );

        //setting offset to the compiled cisa binary kernel
        //to correct offset in final cisa binary
        m_header.kernels[i].offset = m_total_size;
        //for patching later if genx binary is generated
        this->m_kernelOffsetLocationsArray[i] = m_bytes_written_cisa_buffer;
        writeInToCisaHeaderBuffer( &m_header.kernels[i].offset, sizeof(m_header.kernels[i].offset) );

        writeInToCisaHeaderBuffer( &m_header.kernels[i].size, sizeof(m_header.kernels[i].size) );

        //this was originally set to relative offset from the
        //compiled cisa binary kernel
        //changing it to absoute offset from the beginning of the
        //cisa binary
        m_header.kernels[i].input_offset += m_total_size;
        this->m_kernelInputOffsetLocationsArray[i] = m_bytes_written_cisa_buffer;
        writeInToCisaHeaderBuffer( &m_header.kernels[i].input_offset, sizeof(m_header.kernels[i].input_offset) );

        //setting offset to the compiled genx binary kernel
        //to correct offset in final cisa binary
        m_total_size += m_header.kernels[i].size;
        m_total_size += m_header.kernels[i].binary_size;

        assert(m_header.kernels[i].variable_reloc_symtab.num_syms == 0 && "variable relocation not supported");
        writeInToCisaHeaderBuffer(&m_header.kernels[i].variable_reloc_symtab.num_syms, sizeof(m_header.kernels[i].variable_reloc_symtab.num_syms));

        assert(m_header.kernels[i].function_reloc_symtab.num_syms == 0 && "function relocation not supported");
        writeInToCisaHeaderBuffer(&m_header.kernels[i].function_reloc_symtab.num_syms, sizeof(m_header.kernels[i].function_reloc_symtab.num_syms));

        //for now gen binaries
        this->m_krenelBinaryInfoLocationsArray[i] = m_bytes_written_cisa_buffer;
        writeInToCisaHeaderBuffer(&m_header.kernels[i].num_gen_binaries, sizeof(m_header.kernels[i].num_gen_binaries));
        for (int j = 0; j < m_header.kernels[i].num_gen_binaries; j++)
        {
            m_header.kernels[i].gen_binaries[j].binary_offset = m_total_size;
            m_header.kernels[i].gen_binaries[j].platform = 0;
            m_header.kernels[i].gen_binaries[j].binary_size = 0;
            writeInToCisaHeaderBuffer(&m_header.kernels[i].gen_binaries[j].platform, sizeof(m_header.kernels[i].gen_binaries[j].platform));
            writeInToCisaHeaderBuffer(&m_header.kernels[i].gen_binaries[j].binary_offset, sizeof(m_header.kernels[i].gen_binaries[j].binary_offset));
            writeInToCisaHeaderBuffer(&m_header.kernels[i].gen_binaries[j].binary_size, sizeof(m_header.kernels[i].gen_binaries[j].binary_size));

            m_total_size += m_header.kernels[i].gen_binaries[j].binary_size;
        }
    }

    // file-scope variables are no longer supported
    uint16_t numFileScopeVariables = 0;
    writeInToCisaHeaderBuffer(&numFileScopeVariables, sizeof(numFileScopeVariables));

    writeInToCisaHeaderBuffer(&m_header.num_functions, sizeof(m_header.num_functions));

    for (int i = 0; i < m_header.num_functions; i++)
    {
        writeInToCisaHeaderBuffer(&m_header.functions[i].linkage, sizeof(m_header.functions[i].linkage));
        writeInToCisaHeaderBuffer(&m_header.functions[i].name_len, sizeof(m_header.functions[i].name_len));

        writeInToCisaHeaderBuffer(&m_header.functions[i].name, m_header.functions[i].name_len);

        //setting offset to the compiled cisa binary kernel
        //to correct offset in final cisa binary
        m_header.functions[i].offset = m_total_size;
        this->m_functionOffsetLocationsArray[i] = this->m_bytes_written_cisa_buffer;
        writeInToCisaHeaderBuffer(&m_header.functions[i].offset, sizeof(m_header.functions[i].offset));

        writeInToCisaHeaderBuffer(&m_header.functions[i].size, sizeof(m_header.functions[i].size));

        assert(m_header.functions[i].variable_reloc_symtab.num_syms == 0 && "variable relocation not supported");
        writeInToCisaHeaderBuffer(&m_header.functions[i].variable_reloc_symtab.num_syms, sizeof(m_header.functions[i].variable_reloc_symtab.num_syms));

        assert(m_header.functions[i].function_reloc_symtab.num_syms == 0 && "function relocation not supported");
        writeInToCisaHeaderBuffer(&m_header.functions[i].function_reloc_symtab.num_syms, sizeof(m_header.functions[i].function_reloc_symtab.num_syms));

        m_total_size += m_header.functions[i].size;
    }

    return VISA_SUCCESS;
}

int CisaBinary::dumpToStream(std::ostream * os)
{
    os->write(this->m_header_buffer, this->m_header_size);

    for (int i = 0; i < m_header.num_kernels; i++)
    {
        os->write(m_header.kernels[i].cisa_binary_buffer, m_header.kernels[i].size);
        os->write(m_header.kernels[i].genx_binary_buffer, m_header.kernels[i].binary_size);
    }

    for (int i = 0; i < m_header.num_functions; i++)
    {
        os->write(m_header.functions[i].cisa_binary_buffer, m_header.functions[i].size);
        os->write(m_header.functions[i].genx_binary_buffer, m_header.functions[i].binary_size);
    }
    return VISA_SUCCESS;
}

int CisaBinary::dumpToFile(std::string binFileName)
{
    if ( binFileName == "" )
    {
        binFileName = "temp.isa";
    }
    std::ofstream os(binFileName.c_str(), ios::binary|ios::out);
    if (!os)
    {
        assert(0);
        std::cerr<<"Could not open %s"<< binFileName.c_str()<<std::endl;
        return VISA_FAILURE;
    }
    int result = dumpToStream(&os);
    os.close();
    return result;
}

void CisaBinary::writeIsaAsmFile(string filename, string isaasmStr) const
{
    ofstream isaasm;
    isaasm.open(filename.c_str());

    if (isaasm.fail())
    {
        MUST_BE_TRUE(false, "Failed to write CISA ASM to file");
    }

    isaasm << isaasmStr;
    isaasm.close();
}

void CisaBinary::patchKernel(int index, unsigned int genxBufferSize, void * buffer, int platform)
{
    int copySize = 0;
    m_header.kernels[index].offset += genxBinariesSize;
    copySize = sizeof(m_header.kernels[index].offset);
    memcpy_s(&m_header_buffer[this->m_kernelOffsetLocationsArray[index]], copySize, &m_header.kernels[index].offset, copySize);

    m_header.kernels[index].input_offset += genxBinariesSize;
    copySize = sizeof(m_header.kernels[index].input_offset);
    memcpy_s(&m_header_buffer[this->m_kernelInputOffsetLocationsArray[index]], copySize, &m_header.kernels[index].input_offset, copySize);

    int offsetGenBinary = this->m_krenelBinaryInfoLocationsArray[index];
    m_header.kernels[index].num_gen_binaries = 1;
    copySize = sizeof(m_header.kernels[index].num_gen_binaries);
    memcpy_s(&m_header_buffer[offsetGenBinary], copySize, &m_header.kernels[index].num_gen_binaries, copySize);
    offsetGenBinary += sizeof(m_header.kernels[index].num_gen_binaries);

    m_header.kernels[index].gen_binaries[0].platform = static_cast<unsigned char>(platform);
    copySize = sizeof(m_header.kernels[index].gen_binaries[0].platform);
    memcpy_s(&m_header_buffer[offsetGenBinary], copySize, &m_header.kernels[index].gen_binaries[0].platform, copySize);
    offsetGenBinary += sizeof(m_header.kernels[index].gen_binaries[0].platform);

    m_header.kernels[index].gen_binaries[0].binary_offset += genxBinariesSize;
    copySize = sizeof(m_header.kernels[index].gen_binaries[0].binary_offset);
    memcpy_s(&m_header_buffer[offsetGenBinary], copySize, &m_header.kernels[index].gen_binaries[0].binary_offset, copySize);
    offsetGenBinary += sizeof(m_header.kernels[index].gen_binaries[0].binary_offset);

    m_header.kernels[index].gen_binaries[0].binary_size = genxBufferSize;
    copySize = sizeof(m_header.kernels[index].gen_binaries[0].binary_size);
    memcpy_s(&m_header_buffer[offsetGenBinary], copySize, &m_header.kernels[index].gen_binaries[0].binary_size, copySize);
    offsetGenBinary += sizeof(m_header.kernels[index].gen_binaries[0].binary_size);

    m_header.kernels[index].genx_binary_buffer = (char *) buffer;
    m_header.kernels[index].binary_size = genxBufferSize;
    this->genxBinariesSize+= genxBufferSize;
}

void CisaBinary::patchFunction(int index, unsigned genxBufferSize)
{
    m_header.functions[index].offset += genxBinariesSize;
    size_t copySize = sizeof(m_header.functions[index].offset);
    memcpy_s(&m_header_buffer[this->m_functionOffsetLocationsArray[index]], copySize, &m_header.functions[index].offset, copySize);

    m_header.functions[index].binary_size = genxBufferSize;
    this->genxBinariesSize += genxBufferSize;
}

void CisaBinary::isaDumpVerify(
    std::list<VISAKernelImpl*> m_kernels, Options* options)
{
#ifdef IS_RELEASE_DLL
    return;
#else
    bool dump =  m_options->getOption(vISA_GenerateISAASM);
    // disable verification in isaasm mode
    bool verify = !m_options->getOption(vISA_NoVerifyvISA); // && !m_options->getOption(vISA_isParseMode);
    if (!(dump || verify))
        return;

    struct ScopedFile
    {
        FILE* isaasmListFile = nullptr;
        ~ScopedFile()
        {
            if (isaasmListFile)
                fclose(isaasmListFile);
        }
    } ILFile;

    if (options->getOption(vISA_GenerateISAASM) && options->getOption(vISA_GenIsaAsmList))
    {
        if (options->getOption(vISA_IsaasmNamesFileUsed))
        {
            const char* isaasmNamesFile = nullptr;
            options->getOption(vISA_ISAASMNamesFile, isaasmNamesFile);
            if (isaasmNamesFile != nullptr && (ILFile.isaasmListFile = fopen(isaasmNamesFile, "w")) == nullptr)
            {
                fprintf(stderr, "Cannot open file %s\n", isaasmNamesFile);
                exit(1);
            }
        }
        else
        {
            if ((ILFile.isaasmListFile = fopen("isaasmListFile.txt", "w")) == nullptr)
            {
                fprintf(stderr, "Cannot open file %s\n", "isaasmListFile.txt");
                exit(1);
            }
        }
    }

    ERROR_LIST_TYPE errors;
    ERROR_LIST_TYPE kerrors;

    std::list< VISAKernelImpl *>::iterator iter = m_kernels.begin();
    std::list< VISAKernelImpl *>::iterator end = m_kernels.end();
    bool hasErrors = false;
    unsigned totalErrors = 0;
    std::string testName; // base kernel name saved for function's isaasm file name

    for (; iter != end; iter++)
    {
        VISAKernelImpl * kTemp = *iter;
        if (kTemp->getIsKernel())
        {
            //if asmName is test9_genx_0.asm, the testName is test9_genx.
            std::string asmName = kTemp->getOutputAsmPath();
            std::string::size_type asmNameEnd = asmName.find_last_of("_");
            if (asmNameEnd != std::string::npos)
            {
                testName = asmName.substr(0, asmNameEnd);
            }
            else
            {
                testName = asmName;
            }
            break;
        }
    }

    for( iter = m_kernels.begin(); iter != end; iter++ )
    {
        VISAKernelImpl * kTemp = *iter;
        std::list<CisaFramework::CisaInst *>::iterator inst_iter = kTemp->getInstructionListBegin();
        std::list<CisaFramework::CisaInst *>::iterator inst_iter_end = kTemp->getInstructionListEnd();

        unsigned funcId = 0;
        if (dump && !(m_options->getOption(vISA_GeneratevISABInary)  && m_options->getOption(vISA_IsaasmNamesFileUsed)))
        {
            stringstream sstr;
            stringstream asmName;

            if ( kTemp->getIsKernel() )
            {
                asmName << kTemp->getOutputAsmPath();
            }
            else
            {
                //if testName: test9_genx, function 0 has test9_genx_f0.isaasm
                kTemp->GetFunctionId(funcId);
                asmName << testName.c_str();
                asmName << "_f";
                asmName << funcId;
            }
            asmName << ".visaasm";
            if (ILFile.isaasmListFile && m_options->getOption(vISA_GenIsaAsmList))
                fputs(string(asmName.str() + "\n").c_str(), ILFile.isaasmListFile);

            VISAKernel_format_provider fmt(kTemp);
            sstr << printKernelHeader(m_header, &fmt, kTemp->getIsKernel(), funcId, options);
            for(; inst_iter != inst_iter_end; inst_iter++)
            {
                CisaFramework::CisaInst * cisa_inst = *inst_iter;
                CISA_INST * inst = cisa_inst->getCISAInst();
                sstr << printInstruction(&fmt, inst, kTemp->getOptions()) << endl;
            }

            writeIsaAsmFile(asmName.str(), sstr.str());
        }

        if (verify)
        {
            std::list<std::string> kerror_list;
            std::list<std::string> error_list;
            VISAKernel_format_provider fmt(kTemp);

            verifyKernelHeader(m_header, &fmt, kerrors, m_options);

            inst_iter = kTemp->getInstructionListBegin();
            for(; inst_iter != inst_iter_end; inst_iter++)
            {
                CisaFramework::CisaInst * cisa_inst = *inst_iter;
                CISA_INST * inst = cisa_inst->getCISAInst();
                verifyInstruction(m_header, &fmt, inst, errors, options);
            }

            if ( (errors.size() + kerrors.size() /* total errors*/) > 0)
            {
                stringstream verifierName;

                if (kTemp->getIsKernel())
                {
                    verifierName << kTemp->getOutputAsmPath();
                }
                else
                {   // test9_genx_f0.errors.txt in above example, for func 0
                    kTemp->GetFunctionId(funcId);
                    verifierName << testName;
                    verifierName << "_f";
                    verifierName << funcId;
                }
                verifierName << ".errors.txt";
                writeReport(verifierName.str().c_str(), errors, kerrors);
                hasErrors = true;
                totalErrors += (uint32_t) (errors.size() + kerrors.size());
                cerr << "Found " << errors.size() + kerrors.size() << " errors in vISA files." << endl;
                cerr << "Please see error report written to the file "<< verifierName.str() << endl;
            }
        }
        if ( hasErrors )
        {
            cerr << "Found a total of " << totalErrors << " errors in vISA files." << endl;
            exit(EXIT_FAILURE);
        }
    }
#endif // IS_RELEASE_DLL
}

}

