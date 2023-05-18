/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "usc.h"
#include "common/igc_regkeys.hpp"

#if !defined( IGC_DEBUG_VARIABLES )
#   include <stdio.h>
#endif

// In _RELEASE builds, make these api functions available for internal use,
// but do not export them in the dll.
#if defined( IGC_DEBUG_VARIABLES )
#   if defined( _WIN32 )
#       if defined( IGC_EXPORTS )
#           define IGC_DEBUG_API_CALL __declspec(dllexport)
#       else
#           define IGC_DEBUG_API_CALL __declspec(dllimport)
#       endif
#   else
#       if defined( IGC_EXPORTS )
#           define IGC_DEBUG_API_CALL __attribute__((visibility("default")))
#       else
#           define IGC_DEBUG_API_CALL
#       endif
#   endif
#else
#   define IGC_DEBUG_API_CALL
#endif

namespace IGC
{
    namespace Debug
    {
        /// enum to set the compiler flags from the custom API
        enum class OptionFlag
        {
#define DECLARE_IGC_REGKEY(dataType, regkeyName, defaultValue, description, releaseMode) \
            OPTION_##regkeyName,
#include "common/igc_regkeys.h"
#undef DECLARE_IGC_REGKEY
            END,
            BEGIN = 0
        };
        /// Enumeration of flags for debugging various internal states of the compiler.
        enum class DebugFlag
        {
            DUMPS,                      //!< Dumps of dxasm, llvm-ir, cisa, isa to ods() and files
            DUMP_AFTER_PASSES,          //!< Controls whether llvm-ir is dumped after passes
            DUMP_TO_OUTS,               //!< Controls whether text streamed into IGC::ods() should go to llvm::outs()
            DUMP_TO_OUTPUTDEBUGSTRING,  //!< Controls whether text streamed into IGC::ods() should go to OutputDebugString
            OPTIMIZATION_STATS,         //!< Timing of various compiler optimizations
            TIME_STATS_SUM,             //!< Timing of translation, code generation, finalizer, etc
            TIME_STATS_PER_SHADER,      //!< Like TIME_STATS_SUM, but one stat measurement per shader (instead of summed up times)
            TIME_STATS_COARSE,          //!< Only collect/dump coarse level time stats, i.e. skip opt detail timer for now >
            TIME_STATS_PER_PASS,        //!< Collect Timing of IGC/LLVM passes
            MEM_STATS,                  //!< Measurements related to allocations and deallocations
            MEM_STATS_DETAIL,           //!< dump detail memstats
            SHADER_QUALITY_METRICS,     //!< ISA quality measurements (i.e. count of instructions generated)
            SIMD8_ONLY,                 //!< only compile SIMD8
            SIMD16_ONLY,                //!< only compile SIMD16
            SIMD32_ONLY,                //!< only compile SIMD32
            VISA_OUTPUT,                //!< dump gen isa text format from vISA
            VISA_BINARY,                //!< dump gen isa binary format from vISA
            VISA_DUMPCOMMONISA,         //!< dump vISA shaders
            VISA_NOSCHEDULE,            //!< skip instruction scheduling in vISA
            VISA_DOTALL,                //!< dump vISA details
            VISA_SLOWPATH,
            VISA_NOBXMLENCODER,            //!< do not perform binary encoding using BXML based encoder, but fall-back to old proven encoder
            NO_ASM_LINE_NO_DUMP_IN_FILE, //!< Do not dump asm line numbers in file.
            END,
            BEGIN = 0
        };

        /// Enumeration of flags for determining which states to dump
        enum class DumpType
        {
            NOS_TEXT,                   //!< Non-orthogonal states (numan readable)

            CIS_TEXT,                   //!< Compiler input structure (human readable)
            COS_TEXT,                   //!< Compiler output structure (human readable)

            ASM_TEXT,                   //!< Input assembly (human readable)
            ASM_BC,                     //!< Input assembly (bitcode)

            TRANSLATED_IR_TEXT,         //!< Translated llvm IR (human readable)
            TRANSLATED_IR_BC,           //!< Translated llvm IR (bitcode)

            PASS_IR_TEXT,               //!< llvm-IR during the optimization passes (human readable)
            PASS_IR_BC,                 //!< llvm-IR during the optimization passes (bitcode)

            OptIR_TEXT,                 //!< Optimized llvm IR (human readable)
            OptIR_BC,                   //!< Optimized llvm IR (bitcode)

            VISA_TEXT,                  //!< Virtual-ISA (human readable)
            VISA_BC,                    //!< Virtual-ISA (bitcode)

            GENX_ISA_TEXT,              //!< Target ISA (human readable)
            GENX_ISA_BC,                //!< Target ISA (bitcode)

            LLVM_OPT_STAT_TEXT,         //!< llvm optimization stats (human readable)

            TIME_STATS_TEXT,            //!< Time stats (human readable)
            TIME_STATS_CSV,             //!< Time stats (csv, machine readable text)

            DBG_MSG_TEXT,               //!< Debug message

            END,
            BEGIN = 0
        };

        /// Enumeration of the locations to dump to
        enum class DumpLoc
        {
            ODS,                        //!< Dump to Terminal, as well as OutputDebugString
            FILE,                       //!< Dump to an appropriately named file
        };

        /// \brief Version information for this particular build.
        ///
        /// This is how a build identifies itself. This type is to be something that is
        /// streamable into std::ostream's and llvm::raw_ostream's. For now this means its a char*.
        typedef const char* VersionInfo;

        /// String representation of an enum
        typedef const char* EnumStr;

        /// String representation of a corpus name
        typedef const char* CorpusName;

        /// String representation of a output folder name
        typedef const char* OutputFolderName;
        typedef const char* OutputName;

#if defined( IGC_DEBUG_VARIABLES )

        /// Convert enum value to string
        EnumStr IGC_DEBUG_API_CALL str(DebugFlag value);

        /// Convert enum value to string
        EnumStr IGC_DEBUG_API_CALL str(DumpType value);

        /// Convert enum value to string
        EnumStr IGC_DEBUG_API_CALL str(DumpLoc value);

        /// Convert string to enum value
        template<typename TEnum>
        TEnum enumFrom(EnumStr valueStr)
        {
            for (int i = static_cast<int>(TEnum::BEGIN);
                i < static_cast<int>(TEnum::END); ++i)
            {
                if ( strcmp( str( static_cast<TEnum>(i) ), valueStr ) )
                {
                    return static_cast<TEnum>(i);
                }
            }
            return TEnum::END;
        }

        // Handle both bool and int using the same function. For boolean,
        // true is converted to int 1 and false is converted to int 0.
        void IGC_DEBUG_API_CALL SetCompilerOption(OptionFlag flag, int value);
        void IGC_DEBUG_API_CALL SetCompilerOption(OptionFlag flag, debugString s);

        extern "C" void IGC_DEBUG_API_CALL SetCompilerOptionValue(const char* flagName, int value);
        extern "C" void IGC_DEBUG_API_CALL SetCompilerOptionString(const char* flagName, debugString s);

        /// Assign the state of a debug flag (for _DEBUG and _INTERNAL builds only)
        void IGC_DEBUG_API_CALL SetDebugFlag( DebugFlag flag, bool enabled );

        /// Query the state of a debug flag (for _DEBUG and _INTERNAL builds only)
        bool IGC_DEBUG_API_CALL GetDebugFlag( DebugFlag flag );

        /// Assign the state of a dump flag (for _DEBUG and _INTERNAL builds only)
        void IGC_DEBUG_API_CALL SetDumpFlag( DumpType type, DumpLoc loc, bool enabled );

        /// Query the state of a dump flag (for )DEBUG and _INTERNAL builds only)
        bool IGC_DEBUG_API_CALL GetDumpFlag( DumpType type, DumpLoc loc);

        /// Set a name for the to-be-compiled set of shaders
        void IGC_DEBUG_API_CALL SetShaderCorpusName( CorpusName name );

        /// Get the name for the to-be-compiled set of shaders
        CorpusName IGC_DEBUG_API_CALL GetShaderCorpusName();

        /// Set a name for the output folder
        void IGC_DEBUG_API_CALL SetShaderOutputFolder( OutputFolderName name );

        void IGC_DEBUG_API_CALL SetShaderOutputName( OutputName name );

        void IGC_DEBUG_API_CALL SetShaderOverridePath(OutputFolderName pOutputFolderName);
        OutputFolderName IGC_DEBUG_API_CALL GetShaderOverridePath();
        /// Get the name for the output folder
        OutputFolderName IGC_DEBUG_API_CALL GetShaderOutputFolder();

        OutputName IGC_DEBUG_API_CALL GetShaderOutputName();

        OutputName IGC_DEBUG_API_CALL GetFunctionDebugFile();

        /// Ask the build to identify itself
        VersionInfo IGC_DEBUG_API_CALL GetVersionInfo();
#else
        // These stubs are for IGCStandalone's includes in _RELEASE builds
        // This makes it so that we don't have to #ifdef around all of the uses

        /// Returns END in _RELEASE builds
        template<typename TEnum>
        TEnum enumFrom(const char* str) { return TEnum::END; }

        /// Returns empty string in _RELEASE builds
        inline const char* str(DebugFlag value) {
            (void) value;
            return "";
        }

        /// Returns empty string in _RELEASE builds
        inline const char* str(DumpType value) {
            (void) value;
            return "";
        }

        /// Returns empty string in _RELEASE builds
        inline const char* str(DumpLoc value) {
            (void) value;
            return "";
        }

        inline void IGC_DEBUG_API_CALL SetCompilerOption(OptionFlag flag, int value)
        {
            (void)flag;
            (void)value;
        }

        inline void IGC_DEBUG_API_CALL SetCompilerOption(OptionFlag flag, debugString s)
        {
            (void)flag;
            (void)s;
        }

        extern "C" inline void IGC_DEBUG_API_CALL SetCompilerOptionValue(const char* flagName, int value)
        {
            (void)flagName;
            (void)value;
        }

        extern "C" inline void IGC_DEBUG_API_CALL SetCompilerOptionString(const char* flagName, debugString s)
        {
            (void)flagName;
            (void)s;
        }


        /// Do nothing in _RELEASE builds
        inline void SetDebugFlag( DebugFlag flag, bool enabled )
        {
            (void) flag;
            if ( enabled )
            {
                printf("WARNING: Debug flags have no effect in _RELEASE builds\n");
            }
        }

        /// Returns false in _RELEASE builds
        inline bool GetDebugFlag( DebugFlag flag ) { (void)flag; return false; }

        /// Assign the state of a dump flag (for _DEBUG and _INTERNAL builds only)
        inline void SetDumpFlag( DumpType type, DumpLoc loc, bool enabled )
        {
            (void)type;
            (void)loc;
            (void)enabled;
        }

        /// Returns false in _RELEASE builds
        inline bool GetDumpFlag( DumpType type, DumpLoc loc)
        {
            (void)type;
            (void)loc;
            return false;
        }

        /// Does nothing in _RELEASE builds
        inline void IGC_DEBUG_API_CALL SetShaderCorpusName( CorpusName name )
        {
            (void)name;
        }

        /// Returns empty string in _RELEASE builds
        inline CorpusName IGC_DEBUG_API_CALL GetShaderCorpusName() { return ""; }

        /// Does nothing in _RELEASE builds
        inline void IGC_DEBUG_API_CALL SetShaderOutputFolder( OutputFolderName name ) { (void)name; }
        inline void IGC_DEBUG_API_CALL SetShaderOutputName( OutputName name ) { (void)name; }

        /// Returns empty string in _RELEASE builds
        inline OutputFolderName IGC_DEBUG_API_CALL GetShaderOutputFolder() { return ""; }
        inline OutputName IGC_DEBUG_API_CALL GetShaderOutputName() { return ""; }

        inline void IGC_DEBUG_API_CALL SetShaderOverridePath(OutputFolderName pOutputFolderName)
        {
            (void)pOutputFolderName;
        }
        inline OutputFolderName IGC_DEBUG_API_CALL GetShaderOverridePath() { return ""; }

        inline OutputName IGC_DEBUG_API_CALL GetFunctionDebugFile() { return ""; }

        /// Omits changelist and build number in _RELEASE builds
        inline VersionInfo GetVersionInfo() { return "CONFIGURATION: Release"; }
#endif
    }
}
