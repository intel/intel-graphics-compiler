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

#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <iomanip>

#include "Option.h"
#include "Timer.h"
#include "G4_Opcode.h"          // temporarily add to support G4_MAX_SRCS == 3
#include "DebugInfo.h"

#ifdef _MSC_VER
// MAX : Disable security enhancements warning (VC2005)
// 4312: disable cast warning on 64bit build (option handling's macro need to be re-done)
#pragma warning (disable: 4996 4312)
#endif

// NOTE: for now when adding a new flag, please also update ::reset() so that it is
// reset to the default value after the builder is destroyed.  We will move these options to be
// part of the builder in the near future.

bool Options::get_isaasm(int argc, const char *argv[])
{
    return true;
}


/*
    All arguments should be valid BE options.
    When invoked from main the 0 argument which is a name of the program
    should be skipped when passed in.

    If the first argument passed in is *.isaasm or -CISAbinary it
    will go through parser mode in which it will parse *.isaasm files and create either regular or fat .isa file.
    User can either pass in individual *.isaasm file, or multiple files in a text file using -CISAbinary option.
    Either one or the other method is allowed, but not both on the same command line.

    If first name that is passed in ends with *.isa it treats it as an offline compilation of .isa file.
*/
bool Options::parseOptions(int argc, const char* argv[])
{
    int startPos = 0;
    assert(! argToOption.empty() && "Must be initialized first!");

#define MAX_ARGC 128
    MUST_BE_TRUE(argc < MAX_ARGC, "too many options for vISA builder");

    for (int i = 0; i < argc; ++i)
    {
        argString << argv[i] << " ";
    }

    for(int i = startPos; i < argc; i++)
    {
        if( argv[i] == NULL )
        {
            return false;
        }
        // If arg not defined in the .def file, exit with an error
        auto it = argToOption.find(argv[i]);
        if (it == argToOption.end()) {
            COUT_ERROR << "USAGE: Unrecognized option \"" << argv[i] << "\"!" << std::endl;
            showUsage(COUT_ERROR);
            return false;
        }
        // Arg corrsponds to vISAOpt.
        // If bool, set it with the inverse of the default value
        // Else if int32,int64, or cstr, set parse argv[i+1] and set the value.
        vISAOptions vISAOpt = it->second;
        EntryType type = m_vISAOptions.getType(vISAOpt);
        switch(type) {
        case ET_BOOL: {
            bool val = ! m_vISAOptions.getDefaultBool(vISAOpt);
            m_vISAOptions.setBool(vISAOpt, val);
            m_vISAOptions.setArgSetByUser(vISAOpt);
            break;
        }
        case ET_INT32:
        case ET_INT64:
        case ET_CSTR:
        case ET_2xINT32:
            i++;
            if (i >= argc) {
                const char *errorMsg = m_vISAOptions.getErrorMsg(vISAOpt);
                std::cout << errorMsg << std::endl;
                return false;
            }
            switch(type) {
            case ET_INT32:
                m_vISAOptions.setUint32(vISAOpt, atoi(argv[i]));
                m_vISAOptions.setArgSetByUser(vISAOpt);
                break;
            case ET_INT64:
                m_vISAOptions.setUint64(vISAOpt, atol(argv[i]));
                m_vISAOptions.setArgSetByUser(vISAOpt);
                break;
            case ET_CSTR:
                m_vISAOptions.setCstr(vISAOpt, argv[i]);
                m_vISAOptions.setArgSetByUser(vISAOpt);
                break;
            case ET_2xINT32: {
                uint32_t hi32 = stoul(std::string(argv[i]));
                i++;
                uint32_t lo32 = stoul(std::string(argv[i]));
                uint64_t val64 = ((uint64_t)hi32 << 32) | (uint64_t)lo32;
                m_vISAOptions.setUint64(vISAOpt, val64);
                m_vISAOptions.setArgSetByUser(vISAOpt);
                break;
            }
            default:
                assert(0 && "Bad type");
            }
            break;
        default:
            assert(0 && "Bad type");
        }
    }


    // Dependent arguments.
    // If setting an argument triggers more arguments, these should be
    // set here.
    if (m_vISAOptions.isArgSetByUser(vISA_LocalRA)) {
        m_vISAOptions.setBool(vISA_LocalBankConflictReduction, false);
    }
    if (m_vISAOptions.isArgSetByUser(vISA_LocalRARoundRobin)) {
        m_vISAOptions.setBool(vISA_LocalBankConflictReduction, false);
    }
    if (m_vISAOptions.isArgSetByUser(vISA_Debug)) {
        m_vISAOptions.setBool(vISA_EnableSendFusion, false);
        m_vISAOptions.setBool(vISA_LocalScheduling, false);
        m_vISAOptions.setBool(vISA_Compaction, false);
        m_vISAOptions.setBool(vISA_LocalCopyProp, false);
        m_vISAOptions.setBool(vISA_LocalFlagOpt, false);
        m_vISAOptions.setBool(vISA_LocalMACopt, false);
        m_vISAOptions.setBool(vISA_LocalDefHoist, false);
        m_vISAOptions.setBool(vISA_LocalCleanMessageHeader, false);
        m_vISAOptions.setBool(vISA_LocalRenameRegister, false);
        m_vISAOptions.setBool(vISA_IPA, false);
        m_vISAOptions.setBool(vISA_FoldAddrImmed, false);
        m_vISAOptions.setBool(vISA_MergeScalar, false);
        m_vISAOptions.setBool(vISA_EnableMACOpt, false);
        m_vISAOptions.setBool(vISA_DisableleHFOpt, true);
        m_vISAOptions.setBool(vISA_LVN, false);
        m_vISAOptions.setBool(vISA_LocalRARoundRobin, false);
        m_vISAOptions.setBool(vISA_LocalBankConflictReduction, false);
        m_vISAOptions.setBool(vISA_RoundRobin, false);
        m_vISAOptions.setBool(vISA_SpiltLLR, false);
        m_vISAOptions.setBool(vISA_preRA_Schedule, false);
        m_vISAOptions.setBool(vISA_Debug, true);
    }
    if (m_vISAOptions.isArgSetByUser(vISA_Stepping)) {
        const char *stepping = m_vISAOptions.getCstr(vISA_Stepping);
        if (stepping == nullptr)
        {
            return false;
        }
        int status = SetStepping(stepping);
        if( status != 0 ){
            std::cout << "unrecognized stepping string: "
                      << stepping << std::endl;
            return false;
        }
    }
    if (m_vISAOptions.isArgSetByUser(vISA_DumpPasses)) {
        m_vISAOptions.setBool(vISA_DumpDotAll, true);
    }
    if (m_vISAOptions.isArgSetByUser(vISA_DumpDotAll)) {
        m_vISAOptions.setBool(vISA_DumpDot, true);
        m_vISAOptions.setBool(vISA_DumpSchedule, true);
        m_vISAOptions.setBool(vISA_DumpDagDot, true);
        m_vISAOptions.setBool(vISA_DumpPasses, true);
    }
    if (m_vISAOptions.isArgSetByUser(vISA_3DOption)) {
        target = VISA_3D;
    }

    if (m_vISAOptions.isArgSetByUser(vISA_ReservedGRFNum)) {
        if (m_vISAOptions.getUint32(vISA_ReservedGRFNum)) {
            m_vISAOptions.setBool(vISA_LocalBankConflictReduction, false);
        }
    }

    if (m_vISAOptions.isArgSetByUser(vISA_ForceSpills)) {
        m_vISAOptions.setBool(vISA_LocalRA, false);
    }

    if (m_vISAOptions.isArgSetByUser(vISA_HashVal)) {
        m_vISAOptions.setBool(vISA_InsertHashMovs, true);
    }

    if (m_vISAOptions.isArgSetByUser(vISA_CISAbinary)) {
        const char *cisaBinary = m_vISAOptions.getCstr(vISA_CISAbinary);
        m_vISAOptions.setCstr(vISA_ISAASMNamesFile, cisaBinary);
        m_vISAOptions.setBool(vISA_IsaasmNamesFileUsed, true);
        m_vISAOptions.setBool(vISA_isParseMode, true);
    }

    if (m_vISAOptions.isArgSetByUser(vISA_ISAASMNamesFile)) {
        m_vISAOptions.setBool(vISA_GenIsaAsmList, true);
        m_vISAOptions.setBool(vISA_IsaasmNamesFileUsed, true);
    }

    if (m_vISAOptions.isArgSetByUser(vISA_Platform)) {
        const char *platformStr = m_vISAOptions.getCstr(vISA_Platform);
        if (platformStr == nullptr)
        {
            return false;
        }

        int status = SetPlatform(platformStr);
        if( status != 0 ){
            std::cout << "unrecognized platform string: "
                      << platformStr << std::endl;
            return false;
        }
        m_vISAOptions.setBool(vISA_PlatformIsSet, true);   //platformIsSet = true;
        if (GetStepping() == Step_A)
        {
            if (getGenxPlatform() == GENX_TGLLP)
            {
                m_vISAOptions.setBool(vISA_HasEarlyGRFRead, true);
            }
        }
    }

    if (m_vISAOptions.isArgSetByUser(vISA_GetvISABinaryName)) {
        m_vISAOptions.setBool(vISA_OutputvISABinaryName, true);
    }

    if (m_vISAOptions.isArgSetByUser(vISA_LabelStr)) {
        ASSERT_USER(strlen(m_vISAOptions.getCstr(vISA_LabelStr))
                    < MAX_LABEL_STR_LENGTH,
                    "String length for unique labels is too long. Should be no larger than 8.");
        m_vISAOptions.setBool(vISA_UniqueLabels, true);         //uniqueLabels = true;
    }
    if (m_vISAOptions.isArgSetByUser(VISA_AsmFileName)) {
        m_vISAOptions.setBool(VISA_AsmFileNameUser, true);
    }

    if (m_vISAOptions.isArgSetByUser(vISA_DisableSpillCoalescing))
    {
        m_vISAOptions.setBool(vISA_DisableSpillCoalescing, true);
    }

    if (m_vISAOptions.isArgSetByUser(vISA_forceDebugSWSB))
    {
        m_vISAOptions.setUint32(vISA_SWSBInstStall, 0);
        m_vISAOptions.setUint32(vISA_SWSBTokenBarrier, 0);
    }

#if (defined(_DEBUG) || defined(_INTERNAL))
    // Dump all vISA options
    if (m_vISAOptions.getBool(vISA_dumpVISAOptionsAll)) {
        dump();
    }
#endif
    return true;
}

void Options::showUsage(std::ostream& output)
{
    output << "USAGE: GenX_IR <InputFilename.isa> {Option List}" << std::endl;
    output << "Converts a CISA file into Gen binary or assembly" << std::endl;
    output << "Options :" << std::endl;
#ifndef DLL_MODE
    output << std::setw(50) << std::left << "    -platform <BDW/SKL/BXT>"
           << std::setw(60) << "- Gen platform to use (required)" << std::endl;
    output << std::setw(50) << "    -binary"
           << std::setw(60) << "- Emit the binary code." << std::endl;
#endif
    output << std::setw(50) << "    -output"
           << std::setw(60) << "- Emit target assembly code to a file." << std::endl;
    output << std::setw(50) << "    -noschedule"
           << std::setw(60) << "- Turn off code scheduling." << std::endl;
    output << std::setw(50) << "    -nocompaction"
           << std::setw(60) << "- Turn off binary compaction." << std::endl;
    output << std::setw(50) << "    -dumpcommonisa"
           << std::setw(60) << "- Emit CISA assembly." << std::endl;
    output << std::endl;
    output << "USAGE: GenX_IR <InputFilename.visaasm> {Option List}" << std::endl;
    output << "Converting a CISA assembly file into CISA binary file" << std::endl;
    output << "Options :" << std::endl;
    output << std::setw(50) << "    -outputCisaBinaryName <CISABinaryName>"
           << std::setw(60) << "- name for the CISA binary file." << std::endl;
}

// This converts enum vISA_option to "vISA_option" so we can print it.
void Options::initialize_vISAOptionsToStr(void) {
   #undef DEF_VISA_OPTION
   #define DEF_VISA_OPTION(ENUM, TYPE, STR, ERROR_MSG, DEFAULT_VAL) \
    vISAOptionsToStr[ENUM] = #ENUM;
   #include "VISAOptions.def"
}

// Populate argToOption[] map
void Options::initializeArgToOption(void) {
   #undef DEF_VISA_OPTION
   #define DEF_VISA_OPTION(ENUM, TYPE, STR, ERROR_MSG, DEFAULT_VAL) \
    argToOption[STR] = ENUM;
   #include "VISAOptions.def"
}

// Initialize:
// 1. The strings "-someOption"
// 2. The default values
// 3. Set the value = default value
void Options::initialize_m_vISAOptions(void) {
    // I am not sure why, but without const_cast<> I am getting warnings
    // that "this" is const
    auto *Opts = const_cast<VISAOptionsDB *>(&m_vISAOptions);

    #undef DEF_VISA_OPTION
    #define DEF_VISA_OPTION(ENUM, TYPE, STR, ERROR_MSG, DEFAULT_VAL)    \
        Opts->setArgStr(ENUM, STR);                                     \
        Opts->setType(ENUM, TYPE);                                      \
        Opts->setErrorMsg(ENUM, ERROR_MSG);                             \
        switch ((TYPE)) {                                               \
        case ET_BOOL:                                                   \
            /* The casts are here to avoid type conversion warnings */  \
            Opts->setDefaultBool(ENUM, (bool) (DEFAULT_VAL));           \
            Opts->setBool(ENUM, (bool) (DEFAULT_VAL));                  \
            break;                                                      \
        case ET_INT32:                                                  \
            /* The casts are here to avoid type conversion warnings */  \
            Opts->setDefaultUint32(ENUM, (uint32_t) (uint64_t) (DEFAULT_VAL)); \
            Opts->setUint32(ENUM, (uint32_t) (uint64_t) (DEFAULT_VAL)); \
            break;                                                      \
        case ET_INT64:                                                  \
        case ET_2xINT32:                                                \
            /* The casts are here to avoid type conversion warnings */  \
            Opts->setDefaultUint64(ENUM, (uint64_t) (DEFAULT_VAL));     \
            Opts->setUint64(ENUM, (uint64_t) (DEFAULT_VAL));            \
            break;                                                      \
        case ET_CSTR:                                                   \
            /* The casts are here to avoid type conversion warnings */  \
            Opts->setDefaultCstr(ENUM, (const char *) (DEFAULT_VAL));   \
            Opts->setCstr(ENUM, (const char *) (DEFAULT_VAL));          \
            break;                                                      \
        default:                                                        \
            assert(0 && "Bad TYPE");                                    \
        }
    #include "include/VISAOptions.def"
}

// This is to check whether an option of any type is set.
// It is useful for Cstr or Int arguments because getOption() will give us
// the value, not whether they are set or not.
bool Options::isOptionSetByUser(vISAOptions option) const
{
    ASSERT_USER(option < vISA_NUM_OPTIONS, "Option value is outside of range.");
    return m_vISAOptions.isArgSetByUser(option);
}

void Options::getOption(vISAOptions option, bool &value) const
{
    ASSERT_USER(option < vISA_NUM_OPTIONS, "Option value is outside of range.");
    value = m_vISAOptions.getBool(option);
}

bool Options::getOption(vISAOptions option) const
{
    ASSERT_USER(option < vISA_NUM_OPTIONS, "Option value is outside of range.");
    return m_vISAOptions.getBool(option);
}

void Options::getOption(vISAOptions option, const char*& buf) const
{
    ASSERT_USER(option < vISA_NUM_OPTIONS, "Option value is outside of range.");
    buf = m_vISAOptions.getCstr(option);
}

const char *Options::getOptionCstr(vISAOptions option) const
{
    ASSERT_USER(option < vISA_NUM_OPTIONS, "Option value is outside of range.");
    return m_vISAOptions.getCstr(option);
}

uint32_t Options::getuInt32Option(vISAOptions option) const
{
    return m_vISAOptions.getUint32(option);
}

uint64_t Options::getuInt64Option(vISAOptions option) const
{
    if (option == vISAOptions::vISA_HashVal)
        return m_vISAOptions.getUint64(vISA_HashVal);
    else if (option == vISAOptions::vISA_HashVal1)
        return m_vISAOptions.getUint64(vISA_HashVal1);

    return 0;
}

void Options::setOptionInternally(vISAOptions option, bool val)
{
    m_vISAOptions.setBool(option, val);

#if (defined(_DEBUG) || defined(_INTERNAL))
    if (m_vISAOptions.getBool(vISA_dumpVISAOptions)) {
        std::cerr << std::setw(30) << vISAOptionsToStr[option] << " ";
        m_vISAOptions.dump(option);
        std::cerr << "\n";
    }
#endif
}

void Options::setOptionInternally(vISAOptions option, uint32_t value)
{
    m_vISAOptions.setUint32(option, value);

#if (defined(_DEBUG) || defined(_INTERNAL))
    if (m_vISAOptions.getBool(vISA_dumpVISAOptions)) {
        std::cerr << std::setw(30) << vISAOptionsToStr[option] << " ";
        m_vISAOptions.dump(option);
        std::cerr << "\n";
    }
#endif
}

void Options::setOptionInternally(vISAOptions option, const char* str)
{
    m_vISAOptions.setCstr(option, str);

#if (defined(_DEBUG) || defined(_INTERNAL))
    if (m_vISAOptions.getBool(vISA_dumpVISAOptions)) {
        std::cerr << std::setw(30) << vISAOptionsToStr[option] << " ";
        m_vISAOptions.dump(option);
        std::cerr << "\n";
    }
#endif
}

void Options::setOption(vISAOptions option, bool val)
{
    m_vISAOptions.setArgSetByUser(option);
    setOptionInternally(option, val);
}

void Options::setOption(vISAOptions option, uint32_t value)
{
    m_vISAOptions.setArgSetByUser(option);
    setOptionInternally(option, value);
}

void Options::setOption(vISAOptions option, const char* str)
{
    m_vISAOptions.setArgSetByUser(option);
    setOptionInternally(option, str);
}

//
// return full set of arguments ever set by user, either through
// string options or the various setOptions()
//
std::string Options::getFullArgString()
{
    std::stringstream args;
    // Collect all user-set options.
    // This is for igc. When igc invokes vISA, it sets options
    // via setOption() api instead of options string, thus leave
    // argString empty. Here, we re-generate this options strings
    // (This is for debugging)
    for (int i = vISA_OPTIONS_UNINIT + 1; i < vISA_NUM_OPTIONS; ++i)
    {
        vISAOptions o = (vISAOptions)i;
        if (isOptionSetByUser(o))
        {
            EntryType type = m_vISAOptions.getType(o);
            switch (type) {
            case ET_BOOL:
                if (m_vISAOptions.getBool(o) != m_vISAOptions.getDefaultBool(o))
                {
                    // Boolean option means the reverse of the default!
                    // (Probably should avoid such reverse handling)
                    args << m_vISAOptions.getArgStr(o) << " ";
                }
                break;
            case ET_INT32:
                args << m_vISAOptions.getArgStr(o) << " "
                    << m_vISAOptions.getUint32(o) << " ";
                break;
            case ET_INT64:
                args << m_vISAOptions.getArgStr(o) << " "
                    << m_vISAOptions.getUint64(o) << " ";
                break;
            case ET_2xINT32:
            {
                uint32_t lo32, hi32;
                uint64_t val = m_vISAOptions.getUint64(o);
                lo32 = (uint32_t)val;
                hi32 = (uint32_t)(val >> 32);
                args << m_vISAOptions.getArgStr(o) << " "
                    << hi32 << " " << lo32 << " ";
            }
            break;
            case ET_CSTR:
                args << m_vISAOptions.getArgStr(o) << " "
                    << m_vISAOptions.getCstr(o) << " ";
                break;
            default:
                assert(false && "Invalid vISA option type!");
                args << "UNDEFINED ";
                break;
            }
        }
    }
    return args.str();
}

//
// this returns the options string explicitly passed in by user
//
std::stringstream& Options::getUserArgString()
{
    return argString;
}

std::string Options::getEncoderOutputFile()
{
    const char * encoderOutputFile = m_vISAOptions.getCstr(vISA_encoderFile);
    if (encoderOutputFile != nullptr)
    {
        return std::string(encoderOutputFile);
    }
    else
    {
        return std::string("");
    }
}

void Options::dump(void) const {
    m_vISAOptions.dump();
}

Options::Options() {
    m_vISAOptions = VISAOptionsDB(this);

    target = VISA_CM;

    initialize_vISAOptionsToStr();
    initializeArgToOption();
    initialize_m_vISAOptions();
}

Options::~Options() {
    ;
}
