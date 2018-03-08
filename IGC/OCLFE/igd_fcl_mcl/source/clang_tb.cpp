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

#include "../headers/clang_tb.h"
#include "../headers/common_clang.h"
#include "../headers/RegistryAccess.h"

#include "../headers/resource.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "common/LLVMWarningsPop.hpp"

#include "secure_mem.h"
#include "secure_string.h"

#include <sstream>
#include <stdlib.h>
#include <string>
#include <iomanip>

#if defined( _DEBUG ) || defined( _INTERNAL )
#define IGC_DEBUG_VARIABLES
#endif


#if defined(IGC_DEBUG_VARIABLES)
#include "common/Types.hpp"
#include "common/igc_regkeys.hpp"
#include "AdaptorCommon/customApi.hpp"
#include "3d/common/iStdLib/utility.h"
#include <mutex>

namespace IGC
{
	namespace Debug
	{

		static std::mutex stream_mutex;

		void DumpLock()
		{
			stream_mutex.lock();
		}

		void DumpUnlock()
		{
			stream_mutex.unlock();
		}

	}
}
#endif

#ifndef WIN32
#include <dlfcn.h>
#include <stdexcept>
#endif

#if defined(_WIN32)
#include "../tools/Frontend_Stats/CLANG_Stats.h"
#include <Windows.h>
#include "DriverStore.h"
#endif

using namespace llvm;
using namespace std;

// ElfReader related typedefs
using namespace CLElfLib;


void ElfReaderDP(CElfReader* pElfReader)
{
	if (pElfReader)
		CElfReader::Delete(pElfReader);
}
typedef unique_ptr<CElfReader, decltype(&ElfReaderDP)> CElfReaderPtr;

// ClangFE related typedefs
using namespace Intel::OpenCL::ClangFE;
void ReleaseDP(IOCLFEBinaryResult* pT)
{
	if (pT)
		pT->Release();
}

typedef unique_ptr<IOCLFEBinaryResult, decltype(&ReleaseDP) > IOCLFEBinaryResultPtr;

namespace TC
{
	//Misc utility functions used only in the current module
	namespace Utils
	{
		// Replace \0 in input string with \n. This works around an issue in
		// Clang where the error message is not generated for inputs that contain
		// a non-ending \0
		char* NormalizeString(char* input, uint32_t size)
		{
			for (uint32_t i = 0; i < size; i++)
			{
				if (input[i] == '\0')
				{
					input[i] = '\n';
				}
			}
			input[size - 1] = '\0';
			return input;
		}

		//Translates the ClangFE results to STB Output results
		void FillOutputArgs(IOCLFEBinaryResult* pFEBinaryResult, STB_TranslateOutputArgs* pOutputArgs, std::string& exceptString)
		{
			// fill the result structure
			pOutputArgs->ErrorStringSize = (uint32_t)strlen(pFEBinaryResult->GetErrorLog());
			if (pOutputArgs->ErrorStringSize > 0)
			{
#ifdef LLVM_ON_WIN32
				pOutputArgs->pErrorString = _strdup(pFEBinaryResult->GetErrorLog());
#else
				pOutputArgs->pErrorString = strdup(pFEBinaryResult->GetErrorLog());
#endif
			}
			else
			{
				pOutputArgs->pErrorString = NULL;
			}
			pOutputArgs->OutputSize = (uint32_t)pFEBinaryResult->GetIRSize();

			// we have to copy the result due to unfortunate design of STB_TranslateOutputArg interface
			// the better design would be for TranslateXXX calls to be responsible to allocate the outputArgs
			// interface entirely, and the client to be responsible to call outputArgs->release() to free it.
			// This way the implementation of TranslateXXX could be free to return inherited from outputArgs
			// class which could glue the outputArgs with other internal interfaces (like the one returned from
			// ::Compile method for example) without any buffer copy
			if (pOutputArgs->OutputSize > 0)
			{
				pOutputArgs->pOutput = (char*)malloc(pFEBinaryResult->GetIRSize());

				if (!pOutputArgs->pOutput)
				{
					//throw std::bad_alloc();
					exceptString = "bad_alloc";
					return;
				}

				memcpy_s(pOutputArgs->pOutput,
					pFEBinaryResult->GetIRSize(),
					pFEBinaryResult->GetIR(),
					pFEBinaryResult->GetIRSize());
			}
		}
	}//namespace Utils


	struct OCLVersionNumberMapping
	{
		const char*        version;
		unsigned int number;
	};

	// Input parameters to the 3 function
	struct TranslateClangArgs
	{
		TranslateClangArgs() :
			pszProgramSource(NULL),
			pPCHBuffer(NULL),
			uiPCHBufferSize(0),
#if !defined(_WIN64) && !defined(__x86_64__)
			b32bit(true)
#else
			b32bit(false)
#endif
		{
		}

		// A pointer to main program's source (assumed one nullterminated string)
		const char*     pszProgramSource;
		// array of additional input headers to be passed in memory
		std::vector<const char*> inputHeaders;
		// array of input headers names corresponding to pInputHeaders
		std::vector<const char*> inputHeadersNames;
		// optional pointer to the pch buffer
		const char*     pPCHBuffer;
		// size of the pch buffer
		size_t          uiPCHBufferSize;
		// OpenCL application supplied options
		std::string     options;
		// optional extra options string usually supplied by runtime
		std::string     optionsEx;
		// requested OCL version
		std::string     oclVersion;
		// build for 32 bit
		bool            b32bit;
	};

	// Initialize static mutex object to be shared with all threads
	//llvm::sys::Mutex CClangTranslationBlock::m_Mutex(/* recursive = */ true);

	/*****************************************************************************\

	Function:
	CClangTranslationBlock::Create

	Description:

	Input:

	Output:

	\*****************************************************************************/
	bool CClangTranslationBlock::Create(
		const STB_CreateArgs* pCreateArgs,
		CClangTranslationBlock* &pTranslationBlock)
	{
		bool    success = true;

		pTranslationBlock = new CClangTranslationBlock();

		if (pTranslationBlock)
		{
			success = pTranslationBlock->Initialize(pCreateArgs);

			if (true == success)
			{
				// Load the Common Clang library
				CCModuleStruct &CCModule = pTranslationBlock->m_CCModule;
#ifdef _WIN32
				// Both Win32 and Win64
				// load dependency only on RS
				if (GetWinVer() >= OS_WIN_RS)
				{
					CCModule.pModule = LoadDependency(CCModule.pModuleName);
				}
				else
				{
					CCModule.pModule = LoadLibraryA(CCModule.pModuleName);
				}
				if (NULL != CCModule.pModule)
				{
					CCModule.pCompile = (CCModuleStruct::PFcnCCCompile)GetProcAddress((HMODULE)CCModule.pModule, "Compile");
					success = CCModule.pCompile != NULL;
				}
				else
				{
					success = false;
				}
#else
				// Both 32 and 64 bit for non-Windows OS
				CCModule.pModule = dlopen(CCModule.pModuleName, RTLD_NOW);
				if (NULL == CCModule.pModule)
				{
					// Try to load with old name. See header file for explanation.
					CCModule.pModule = dlopen(CCModule.pModuleOldName, RTLD_NOW);
				}
				if (NULL != CCModule.pModule)
				{
					CCModule.pCompile = (CCModuleStruct::PFcnCCCompile)dlsym(CCModule.pModule, "Compile");
					success = CCModule.pCompile != NULL;
				}
				else
				{
					success = false;
				}
#endif
			}

#if defined(IGC_DEBUG_VARIABLES)
			if (success)
			{
				LoadRegistryKeys();
			}
#endif

			if (!success)
			{
				CClangTranslationBlock::Delete(pTranslationBlock);
			}
		}
		else
		{
			success = false;
		}

		return success;
	}

	/*****************************************************************************\

	Function:
	CClangTranslationBlock::Delete

	Description:

	Input:

	Output:

	\*****************************************************************************/
	void CClangTranslationBlock::Delete(
		CClangTranslationBlock* &pTranslationBlock)
	{
		// Unload the Common Clang library
#ifdef _WIN32
		// Both Win32 and Win64
		FreeLibrary((HMODULE)pTranslationBlock->m_CCModule.pModule);
#else
		dlclose(pTranslationBlock->m_CCModule.pModule);
#endif

		delete pTranslationBlock;
		pTranslationBlock = NULL;
	}

	/*****************************************************************************\

	Function:
	CClangTranslationBlock::SetErrorString

	Description:
	Given an error string, mallocs memory for the string and sets the
	appropriate STB_TranslateOutputArgs fields.

	Input:

	Output:

	\*****************************************************************************/
	void CClangTranslationBlock::SetErrorString(const char *pErrorString, STB_TranslateOutputArgs* pOutputArgs)
	{
		assert(pErrorString != NULL);
		assert(pOutputArgs != NULL);
		size_t strSize = strlen(pErrorString) + 1;
		pOutputArgs->ErrorStringSize = strSize;
#ifdef LLVM_ON_WIN32
		pOutputArgs->pErrorString = _strdup(pErrorString);
#else
		pOutputArgs->pErrorString = strdup(pErrorString);
#endif
	}

	/*****************************************************************************\

	Function:
	CClangTranslationBlock::GetOclApiVersion

	Description:
	Parses the given internal options and return the OCL Version to be used
	for Clang compilation. If OCL version was not specified in internal options
	returns the default OCL version for the device

	Input:

	Output:

	\*****************************************************************************/
	std::string CClangTranslationBlock::GetOclApiVersion(const char* pInternalOptions) const
	{
		static const char* OCL_VERSION_OPT = "-ocl-version=";
		static size_t OCL_VERSION_OPT_SIZE = strlen(OCL_VERSION_OPT);

		if (pInternalOptions)
		{
			const char* pszOpt = strstr(pInternalOptions, OCL_VERSION_OPT);
			if (NULL != pszOpt)
			{
				// we are in control of internal option - assert the validity
				assert(strlen(pszOpt + OCL_VERSION_OPT_SIZE) >= 3);
				return std::string(pszOpt + OCL_VERSION_OPT_SIZE, 3);
			}
		}

		return m_OCL_Ver;
	}

	/*****************************************************************************\

	Function:
	EnforceOCLCVersion

	Description:
	In case the '-force-cl-std' options was specified, check that the user
	requested OCL C version isn't higher than the supported OCL version.
	exception is thrown otherwise

	Input:

	Output:

	\*****************************************************************************/
	unsigned int GetOclCVersionFromOptions(const char* pOptions, const char* pInternalOptions, 
                                           const std::string& oclVersion /*OCL runtime API version*/, 
                                           std::string& exceptString)
	{
        exceptString.clear();

        if(pOptions == nullptr){
            return 0; // no options (i.e. no options from client application)
        }

		std::string optName = "-cl-std="; // opt that we are looking for
		unsigned int device_version = atoi(oclVersion.c_str());

		const char* optSubstring = strstr(pOptions, optName.c_str());
        if(optSubstring == nullptr){
            return 0 ; // -cl-std not specified
        }

        bool validate = true;
        if((pInternalOptions != nullptr) && (strstr(pInternalOptions, "-force-cl-std") != nullptr)){
            // we're forcing the -cl-std version internally, so no need for validating it
            validate = false;
        }

        const char * optValue = optSubstring + optName.size();
        const char * end = optValue + strlen(optValue);

        // parse
        std::string invalidFormatMessage = "Invalid format of -cl-std option, expected -cl-std=CLMAJOR.MINOR";
        auto isNumeric = [](char v){ return (v >= '0') && (v <= '9'); };
        if(false == ((end - optValue >= 5) && (optValue[0] == 'C') && (optValue[1] == 'L') && isNumeric(optValue[2]) 
                                           && (optValue[3] == '.') && isNumeric(optValue[4])
                     )
           ){
            exceptString = invalidFormatMessage;
            return 0;
        }

        unsigned int retVersion = 0;
        // subverions
        if((end - optValue >= 7) && (optValue[5] != ' ')){
            if((optValue[5] == '.' ) || isNumeric(optValue[6])){
                retVersion += optValue[6] - '0';
            }else if (isNumeric(optValue[5])){
                retVersion += optValue[5] - '0';
            }else{
                exceptString = invalidFormatMessage;
                return 0;
            }
        }

        retVersion += 100 * (optValue[2] - '0') + 10 * (optValue[4] - '0');

        if(validate == false){
            return retVersion;
        }

        if(device_version < retVersion){
            exceptString = "-cl-std OpenCLC version greater than OpenCL (API) version";
            return 0;
        }

        return retVersion;
	}

	/*****************************************************************************\

	Function:
	IsBuildingFor32bit

	Description:
	Return true if clang should generate 32bit code

	Input:

	Output:

	\*****************************************************************************/
	bool IsBuildingFor32bit(const char* pInternalOptions)
	{
		// Detect pointer size from internal option string. Default to using the
		// architecture type if the string is unavailable.
		if (pInternalOptions != NULL)
		{
			if (strstr(pInternalOptions, "-m32") != NULL)
			{
				return true;
			}

			if (strstr(pInternalOptions, "-m64") != NULL)
			{
				return false;
			}
		}

#if !defined(_WIN64) && !defined(__x86_64__)
		return true;
#else
		return false;
#endif
	}

	/*****************************************************************************\

	Function:
	CClangTranslationBlock::GetTranslateClangArgs

	Description:
	Prepares the arguments for the TranslateClang method for the given text input

	Input:

	Output:

	\*****************************************************************************/
	void CClangTranslationBlock::GetTranslateClangArgs(char* pInput,
		uint32_t    uiInputSize,
		const char* pOptions,
		const char* pInternalOptions,
		TranslateClangArgs* pClangArgs,
		std::string& exceptString)
	{
		pClangArgs->pszProgramSource = Utils::NormalizeString(pInput, uiInputSize);
		pClangArgs->pPCHBuffer = NULL;
		pClangArgs->uiPCHBufferSize = 0;
		pClangArgs->oclVersion = GetOclApiVersion(pInternalOptions);
		pClangArgs->b32bit = IsBuildingFor32bit(pInternalOptions);

		if (pOptions)
		{
			pClangArgs->options.assign(pOptions);
		}

		GetOclCVersionFromOptions(pOptions, pInternalOptions, pClangArgs->oclVersion, exceptString);
		EnsureProperPCH(pClangArgs, pInternalOptions, exceptString);
	}

	/*****************************************************************************\

	Function:
	CClangTranslationBlock::GetTranslateClangArgs

	Description:
	Parses the given ELF binary to prepare the arguments for the TranslateClang

	Input:

	Output:

	\*****************************************************************************/
	void CClangTranslationBlock::GetTranslateClangArgs(CElfReader* pElfReader,
		const char* pOptions,
		const char* pInternalOptions,
		TranslateClangArgs* pClangArgs,
		std::string& exceptString)
	{
		assert(pElfReader && "pElfReader is invalid");

		const SElf64Header* pHeader = pElfReader->GetElfHeader();
		assert((pHeader->Type == EH_TYPE_OPENCL_SOURCE) && "OPENCL_SOURCE elf type is expected");

		// First section should be an OpenCL source code
		const SElf64SectionHeader* pSectionHeader = pElfReader->GetSectionHeader(1);
		if (NULL == pSectionHeader)
		{
			llvm::report_fatal_error("pSectionHeader cannot be NULL");
		}
		if (pSectionHeader->Type == SH_TYPE_OPENCL_SOURCE)
		{
			char *pData = NULL;
			size_t uiDataSize = 0;
			pElfReader->GetSectionData(1, pData, uiDataSize);

			if (pData != NULL)
			{
				assert(pData[uiDataSize - 1] == '\0' && "Program source is not null terminated");
				pClangArgs->pszProgramSource = (const char *)pData;
			}
		}

		// Other sections could be runtime supplied header files
		for (unsigned i = 2; i < pHeader->NumSectionHeaderEntries; ++i)
		{
			const SElf64SectionHeader* pSectionHeader = pElfReader->GetSectionHeader(i);

			if ((pSectionHeader != NULL) && (pSectionHeader->Type == SH_TYPE_OPENCL_HEADER))
			{
				char* pData = NULL;
				size_t uiDataSize = 0;
				pElfReader->GetSectionData(i, pData, uiDataSize);

				if (pData != NULL)
				{
					assert(pData[uiDataSize - 1] == '\0' && "Header source is not null terminated");
					pClangArgs->inputHeaders.push_back(pData);
					pClangArgs->inputHeadersNames.push_back(pElfReader->GetSectionName(i));
				}
			}
		}

		if (pOptions)
		{
			pClangArgs->options.assign(pOptions);
		}

		pClangArgs->oclVersion = GetOclApiVersion(pInternalOptions);
		pClangArgs->b32bit = IsBuildingFor32bit(pInternalOptions);

		EnsureProperPCH(pClangArgs, pInternalOptions, exceptString);
	}

    std::string FormatExtensionsString(const std::vector<std::string> &extensions)
    {
        std::stringstream output;

        if (!extensions.empty())
        {
            output << "-cl-ext=-all,";
            output << "+" << extensions[0];
        }

        for (unsigned i = 1; i < extensions.size(); i++)
            output << ",+" << extensions[i];

        output.flush();
        return output.str();
    }

    std::string GetListOfExtensionsFromInternalOptions(const char *pInternalOptions){
        if(pInternalOptions == nullptr){
            return std::string{};
        }

        const char * beg = strstr(pInternalOptions, "-cl-ext=");
        if(beg == nullptr){
            return std::string{};
        }

        const char * end = strstr(beg, " ");
        if(end == nullptr){
            // cl-ext fills the rest of the string
            return std::string(beg);
        }

        return std::string{beg, end};
    }

    std::string GetCDefinesFromInternalOptions(const char *pInternalOptions){
        if(pInternalOptions == nullptr){
            return std::string{};
        }

        std::string internalDefines = "";

        const char * beg = strstr(pInternalOptions, "-D");
        const char * end = nullptr;
        while(beg != nullptr){
            if((beg == pInternalOptions) || (beg[-1] == ' ')){
                end = strstr(beg, " ");
                if(end == nullptr){
                    if(beg[2] != '\0'){                    
                        // more than just -D
                        internalDefines += std::string(beg);
                    }
                    break;
                }else{
                    if(end - beg > 2){
                        internalDefines += std::string(beg, end);
                    }
                }
                internalDefines += " ";
            }else{
                end = beg + 2; // -D
            }

            beg = strstr(end, "-D");
        }

        return internalDefines;
    }

	/*****************************************************************************\

	Function:
	CClangTranslationBlock::EnsureProperPCH

	Description:
	Ensures that the given TranslateClang arguments has the proper CTH and PCH
	headers specified

	Input:

	Output:

	\*****************************************************************************/
	void CClangTranslationBlock::EnsureProperPCH(TranslateClangArgs* pArgs, const char *pInternalOptions, std::string& exceptString)
	{
		unsigned long CTHeaderSize = 0;
		m_cthBuffer = llvm::LoadCharBufferFromResource(IDR_CTH_H, "H", CTHeaderSize);
		assert(m_cthBuffer && "Error loading Opencl_cth.h");

        if (m_cthBuffer)
        {
            // Process the CT Header
            assert(CTHeaderSize > 0 && "Resource for the CT Header is empty");
            assert(m_cthBuffer[CTHeaderSize-1] == '\0' && "Resource for the CT Header is not null terminated");

			pArgs->inputHeaders.push_back(m_cthBuffer);
			pArgs->inputHeadersNames.push_back("CTHeader.h");
			pArgs->optionsEx.append(" -include CTHeader.h");
		}
	}

	/*****************************************************************************\

	Function:
	CClangTranslationBlock::TranslateClang

	Description:
	Translates from CL to LL/BC

	Input:

	Output:

	\*****************************************************************************/
	bool CClangTranslationBlock::TranslateClang(const TranslateClangArgs* pInputArgs,
		STB_TranslateOutputArgs* pOutputArgs, std::string& exceptString, const char* pInternalOptions)
	{
		// additional clang options
		std::string optionsEx = pInputArgs->optionsEx;
		std::string options = pInputArgs->options;
		optionsEx.append(" -disable-llvm-optzns -fblocks -I. -D__ENABLE_GENERIC__=1");

		switch (m_OutputFormat)
		{
		case TB_DATA_FORMAT_LLVM_TEXT:
			optionsEx += " -emit-llvm";
			break;
		case TB_DATA_FORMAT_LLVM_BINARY:
			optionsEx += " -emit-llvm-bc";
			break;
		default:
			break;
		}

        if (options.find("-triple") == std::string::npos){
            // if target triple not explicitly set
            if (pInputArgs->b32bit)
            {
                optionsEx += " -D__32bit__=1";
                options += " -triple spir";
            }
            else
            {
                options += " -triple spir64";
            }
        }

        if (options.find("-gline-tables-only") != std::string::npos)
        {
            optionsEx += " -debug-info-kind=line-tables-only -dwarf-version=4";
        }

        std::string extensionsFromInternalOptions = GetListOfExtensionsFromInternalOptions(pInternalOptions);
        std::string extensions;

        // if extensions list is passed in via internal options, it will override the default ones.
        if(extensionsFromInternalOptions.size() != 0)
        {
            extensions = extensionsFromInternalOptions;
            optionsEx += " " + extensionsFromInternalOptions;
        }
        else
        {
            extensions = FormatExtensionsString(m_Extensions);
            optionsEx += " " + extensions;
        }

        // get additional -D flags from internal options
        optionsEx += " " + GetCDefinesFromInternalOptions(pInternalOptions);

		if (extensions.find("cl_intel_subgroups_short") != std::string::npos)
		{
			optionsEx += " -Dcl_intel_subgroups_short";
		}
        if (extensions.find("cl_intel_media_block_io") != std::string::npos)
        {
            optionsEx += " -Dcl_intel_media_block_io";
        }
        if (extensions.find("cl_intel_device_side_avc_motion_estimation") != std::string::npos)
        {
            optionsEx += " -Dcl_intel_device_side_avc_motion_estimation";
        }

		optionsEx += " -D__IMAGE_SUPPORT__ -D__ENDIAN_LITTLE__";

		IOCLFEBinaryResult *pResultPtr = NULL;
		int res = m_CCModule.pCompile(pInputArgs->pszProgramSource,
			(const char**)pInputArgs->inputHeaders.data(),
			(unsigned int)pInputArgs->inputHeaders.size(),
			(const char**)pInputArgs->inputHeadersNames.data(),
			NULL,
			0,
			options.c_str(),
			optionsEx.c_str(),
			pInputArgs->oclVersion.c_str(),
			&pResultPtr);

		Utils::FillOutputArgs(pResultPtr, pOutputArgs, exceptString);
		if (!exceptString.empty()) // str != "" => there was an exception. skip further code and return. 
		{
			return false;
		}

		// if -dump-opt-llvm is enabled dump the llvm output to the file
		size_t dumpOptPosition = options.find("-dump-opt-llvm");
		if ((0 == res) && dumpOptPosition != std::string::npos)
		{
			std::string dumpFileName;
			std::istringstream iss(options.substr(dumpOptPosition));
			iss >> dumpFileName;
			size_t equalSignPosition = dumpFileName.find('=');
			if (equalSignPosition != std::string::npos)
			{
				dumpFileName = dumpFileName.substr(equalSignPosition + 1);
				// dump the archive
				FILE* file = fopen(dumpFileName.c_str(), "wb");
				if (file != NULL)
				{
					fwrite(pOutputArgs->pOutput, pOutputArgs->OutputSize, 1, file);
					fclose(file);
				}
			}
			else
			{
				std::string errorString = "\nWarning: File name not specified with the -dump-opt-llvm option.\n";
#ifdef LLVM_ON_WIN32
				pOutputArgs->pErrorString = _strdup(errorString.c_str());
#else
				pOutputArgs->pErrorString = strdup(errorString.c_str());
#endif
				pOutputArgs->ErrorStringSize = errorString.length() + 1;
			}
		}

		//pResult.release();
		pResultPtr->Release();

		return (0 == res);
	}

	/*****************************************************************************\

	Function:
	CClangTranslationBlock::ReturnSuppliedIR

	Description: Extract the IR from the input arguments and supply it unmodified
	to the output

	Input:

	Output:

	\*****************************************************************************/
	bool CClangTranslationBlock::ReturnSuppliedIR(const STB_TranslateInputArgs* pInputArgs,
		STB_TranslateOutputArgs* pOutputArgs)
	{
		bool success = true;

		pOutputArgs->ErrorStringSize = 0;
		pOutputArgs->pErrorString = NULL;
		pOutputArgs->OutputSize = 0;
		pOutputArgs->pOutput = NULL;

		CElfReader* pElfReader = CElfReader::Create(pInputArgs->pInput, pInputArgs->InputSize);
		RAIIElf ElfObj(pElfReader);

		if (!pElfReader || !pElfReader->IsValidElf64(pInputArgs->pInput, pInputArgs->InputSize))
		{
			SetErrorString("Invalid input/output passed to library", pOutputArgs);
			return false;
		}

		const SElf64Header* pHeader = pElfReader->GetElfHeader();
		assert(pHeader != NULL);

		for (unsigned i = 1; i < pHeader->NumSectionHeaderEntries; i++)
		{
			const SElf64SectionHeader* pSectionHeader = pElfReader->GetSectionHeader(i);
			assert(pSectionHeader != NULL);
			if (pSectionHeader == NULL)  llvm::report_fatal_error("No section header");

			if ((pSectionHeader->Type == SH_TYPE_OPENCL_LLVM_ARCHIVE) ||
				(pSectionHeader->Type == SH_TYPE_OPENCL_LLVM_BINARY))
			{
				char *pData = NULL;
				size_t dataSize = 0;
				const unsigned char *pBufStart;

				if (pOutputArgs->pOutput != NULL)
				{
					SetErrorString("Multiple inputs passed to library", pOutputArgs);
					success = false;
					break;
				}

				pElfReader->GetSectionData(i, pData, dataSize);
				pBufStart = (const unsigned char *)pData;

				if (llvm::isBitcode(pBufStart, pBufStart + pHeader->ElfHeaderSize))
				{
					pOutputArgs->OutputSize = dataSize;
					pOutputArgs->pOutput = (char*)malloc(dataSize);

					if (pOutputArgs->pOutput == NULL)
					{
						SetErrorString("Error allocating memory", pOutputArgs);
						success = false;
						break;
					}

					memcpy_s(pOutputArgs->pOutput, dataSize, pBufStart, dataSize);
				}
				else
				{
					SetErrorString("Invalid input/output passed to library", pOutputArgs);
					success = false;
					break;
				}
			}
		}

		return success;
	}

	///
	// Process the translation of ELF input type
	//
	bool CClangTranslationBlock::TranslateElf(const STB_TranslateInputArgs* pInputArgs,
		STB_TranslateOutputArgs* pOutputArgs,
		std::string& exceptString)
	{
		CElfReaderPtr pElfReader(CElfReader::Create(pInputArgs->pInput, pInputArgs->InputSize), ElfReaderDP);

		if (!pElfReader.get())
		{
			llvm::report_fatal_error("CElfReader::Create returned NULL\n");
		}

		if (!pElfReader->IsValidElf64(pInputArgs->pInput, pInputArgs->InputSize))
		{
			//throw invalid_input_param ("Wrong ELF format");
			exceptString = "Wrong ELF format";
			return false;
		}
		// Elf is valid, so it is safe to access the header
		E_EH_TYPE ehType = *(const E_EH_TYPE *)&pElfReader->GetElfHeader()->Type;

		switch (m_OutputFormat)
		{
			//{ { TB_DATA_FORMAT_ELF, TB_DATA_FORMAT_LLVM_BINARY } },
		case TB_DATA_FORMAT_LLVM_BINARY:
			switch (ehType)
			{
			case EH_TYPE_OPENCL_SOURCE:
			{
				TranslateClangArgs args;
				GetTranslateClangArgs(pElfReader.get(),
					pInputArgs->pOptions,
					pInputArgs->pInternalOptions,
					&args,
					exceptString);
				bool success = TranslateClang(&args, pOutputArgs, exceptString, pInputArgs->pInternalOptions);
				if (exceptString.empty())
				{
					return success;
				}
				else
				{
					return false;
				}
			}

			case EH_TYPE_OPENCL_OBJECTS:
				if (strstr(pInputArgs->pOptions, "-x spir") == NULL)
				{
					exceptString = "Unsupported ELF container";
					return false;
				}

				return ReturnSuppliedIR(pInputArgs, pOutputArgs);

			default:
				exceptString = "Unsupported ELF header type";
				return false;
			}
			break;

		default:
			exceptString = "Unsupported output format";
			return false;
		}
	}

	/*****************************************************************************\

	Function:
	CClangTranslationBlock::Translate

	Description:

	Input:

	Output:

	\*****************************************************************************/
	bool CClangTranslationBlock::Translate(const STB_TranslateInputArgs* pInputArgs,
		STB_TranslateOutputArgs* pOutputArgs)
	{
		// Setup a MutexGuard so that it's automatically released if it goes out of
		// scope
		//llvm::MutexGuard mutexGuard(m_Mutex);
		//resetOptionOccurrence();

		std::string exceptString;
		switch (m_InputFormat)
		{
			// Processing the translations from OCL to LLVM
			//{{ TB_DATA_FORMAT_OCL_TEXT,      TB_DATA_FORMAT_LLVM_TEXT }} ,
			//{{ TB_DATA_FORMAT_OCL_TEXT,      TB_DATA_FORMAT_LLVM_BINARY }},
		case TB_DATA_FORMAT_OCL_TEXT:
		{
			TranslateClangArgs args;
			GetTranslateClangArgs(pInputArgs->pInput,
				pInputArgs->InputSize,
				pInputArgs->pOptions,
				pInputArgs->pInternalOptions,
				&args,
				exceptString);
			bool successTC = TranslateClang(&args, pOutputArgs, exceptString, pInputArgs->pInternalOptions);

#if defined(IGC_DEBUG_VARIABLES)
			if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
			{

				// Works for all OSes. Creates dir if necessary.
				const char *pOutputFolder = IGC::Debug::GetShaderOutputFolder();
				stringstream ss;
				char* pBuffer = (char *)pInputArgs->pInput;
				UINT  bufferSize = pInputArgs->InputSize;

				// Create hash based on cclang binary output (currently llvm binary; later also spirv).
				// Hash computed in fcl needs to be same as the one computed in igc.
				// This is to ensure easy matching .cl files dumped in fcl with .ll/.dat/.asm/... files dumoed in igc.
				QWORD hash = iSTD::Hash(reinterpret_cast<const DWORD *>(pOutputArgs->pOutput), int_cast<DWORD>(pOutputArgs->OutputSize) / 4);

				ss << pOutputFolder;
				ss << "OCL_"
					<< "asm"
					<< std::hex
					<< std::setfill('0')
					<< std::setw(sizeof(hash) * CHAR_BIT / 4)
					<< hash
					<< std::dec
					<< std::setfill(' ')
					<< ".cl";

				FILE* pFile = NULL;
				fopen_s(&pFile, ss.str().c_str(), "wb");
				if (pFile)
				{
					fwrite(pBuffer, 1, bufferSize - 1, pFile);
					fclose(pFile);
				}
			}
#endif

			if (exceptString.empty())
			{
				return successTC;
			}
			//else continue to process "catch"
			break;
		}
		// Processing the translations from ELF to LLVM
		//{ { TB_DATA_FORMAT_ELF, TB_DATA_FORMAT_LLVM_BINARY } },
		case TB_DATA_FORMAT_ELF:
		{
			bool successTE = TranslateElf(pInputArgs, pOutputArgs, exceptString);
			if (exceptString.empty())
			{
				return successTE;
			}
			//else continue to process exception
			break;
		}
		default:
		{
			exceptString = "Unsupported input format";
			//continue to process exception
		}
		}

		// exceptString != "" => there was an exception. we get here only if there is an exception
		{
			if (exceptString.compare("bad_alloc") == 0)
			{
				SetErrorString("fcl: Allocation failure", pOutputArgs);
				return false;
			}
			else
			{
				SetErrorString(exceptString.c_str(), pOutputArgs);
				return false;
			}
		}
	}

	/*****************************************************************************\

	Function:
	CClangTranslationBlock::FreeAllocations

	Description:

	Input:

	Output:

	\*****************************************************************************/
	bool CClangTranslationBlock::FreeAllocations(
		STB_TranslateOutputArgs* pOutputArgs)
	{
		pOutputArgs->ErrorStringSize = 0;
		if (pOutputArgs->pErrorString != NULL)
		{
			free(pOutputArgs->pErrorString);
			pOutputArgs->pErrorString = NULL;
		}

		pOutputArgs->OutputSize = 0;
		if (pOutputArgs->pOutput != NULL)
		{
			free(pOutputArgs->pOutput);
			pOutputArgs->pOutput = NULL;
		}

		return true;
	}

	/*****************************************************************************\

	Function:
	CClangTranslationBlock constructor

	Description:

	Input:

	Output:

	\*****************************************************************************/
	CClangTranslationBlock::CClangTranslationBlock(void) :
		m_GlobalData()
	{
	}

	/*****************************************************************************\

	Function:
	CClangTranslationBlock destructor

	Description:

	Input:

	Output:

	\*****************************************************************************/
	CClangTranslationBlock::~CClangTranslationBlock(void)
	{
	}


	/*****************************************************************************\

	Function:
	CClangTranslationBlock::Initialize

	Description:

	Input:

	Output:

	\*****************************************************************************/
	bool CClangTranslationBlock::Initialize(const STB_CreateArgs* pCreateArgs)
	{
		if (pCreateArgs == NULL)
		{
			return false;
		}

		unsigned int i;
		for (i = 0; i < sizeof(g_cClangTranslationCodes) / sizeof(g_cClangTranslationCodes[0]); i++)
		{
			// Some quick checks to ensure that the input and output types
			// are compatible with this translation blocks
			if ((pCreateArgs->TranslationCode.Type.Input == g_cClangTranslationCodes[i].Type.Input) &&
				(pCreateArgs->TranslationCode.Type.Output == g_cClangTranslationCodes[i].Type.Output))
			{
				break;
			}
		}

		if (i >= sizeof(g_cClangTranslationCodes) / sizeof(g_cClangTranslationCodes[0]))
		{
			return false;
		}

		// Find out what GPU platform the driver is running on
        if(pCreateArgs->pCreateData != nullptr){
        }else{
            // assume m_OCL_Ver, etc. will be set-up later
            SGlobalData globDataTmp = {0};
            m_GlobalData = globDataTmp;
            m_HWPlatform = IGFX_UNKNOWN;
            m_OCL_Ver = "120";
        }
		m_InputFormat = pCreateArgs->TranslationCode.Type.Input;
		m_OutputFormat = pCreateArgs->TranslationCode.Type.Output;

		return true;
	}
} // namespace TC
