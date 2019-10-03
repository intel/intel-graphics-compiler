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

#include "CompilerStatsUtils.hpp"
#include "common/Stats.hpp"
#include "common/igc_regkeys.hpp"

namespace IGC
{
    namespace CompilerStatsUtils
    {
        void RecordCompileTimeStats(IGC::CodeGenContext *context)
        {
#if COMPILER_STATS_ENABLE
            if (IGC_IS_FLAG_ENABLED(DumpCompilerStats))
            {
                TimeStats *ctStats = context->m_compilerTimeStats;
                CompilerStats cStats = context->Stats();
                cStats.SetF64("TimeTotal(ms)", ctStats->getCompileTimeMS(TIME_TOTAL));
                cStats.SetF64("TimeUnify(ms)", ctStats->getCompileTimeMS(TIME_UnificationPasses));
                cStats.SetF64("TimeOpt(ms)", ctStats->getCompileTimeMS(TIME_OptimizationPasses));
                cStats.SetF64("TimeCodeGen(ms)", ctStats->getCompileTimeMS(TIME_CodeGen));
                // Compute the vISA time spent in failed simd widths
                cStats.SetF64("TimeVISAInFailedSimds",
                    ((cStats.GetFlag("IsRAsuccessful", 8)? 0 : cStats.GetF64("TimeVISACompile", 8))
                     +(cStats.GetFlag("IsRAsuccessful", 16)? 0 : cStats.GetF64("TimeVISACompile", 16))
                     +(cStats.GetFlag("IsRAsuccessful", 32)? 0 : cStats.GetF64("TimeVISACompile", 32))));
                // Compute Total time per generated Inst. Consider only those Instructions for which RA was successful
                cStats.SetF64("TimeTotalPerInst(ms)",cStats.GetF64("TimeTotal(ms)")/(double)
                    ((cStats.GetFlag("IsRAsuccessful", 8)? cStats.GetI64("numInst", 8) : 0)
                     +(cStats.GetFlag("IsRAsuccessful", 16)? cStats.GetI64("numInst", 16) : 0)
                     +(cStats.GetFlag("IsRAsuccessful", 32)? cStats.GetI64("numInst", 32) : 0)));
            }
#endif // COMPILER_STATS_ENABLE
        }

        void RecordCodeGenCompilerStats(IGC::CodeGenContext *context,
                                        SIMDMode dispatchSize,
                                        CompilerStats &vISACompilerStats,
                                        FINALIZER_INFO *jitInfo)
        {
#if COMPILER_STATS_ENABLE
            if (IGC_IS_FLAG_ENABLED(DumpCompilerStats))
            {
                CompilerStats &compilerStats = context->Stats();
                int simdsize = 0;
                if (dispatchSize == SIMDMode::SIMD8)
                {
                    simdsize = 8;
                }
                else if (dispatchSize == SIMDMode::SIMD16)
                {
                    simdsize = 16;
                }
                else if (dispatchSize == SIMDMode::SIMD32)
                {
                    simdsize = 32;
                }
                // Copy from vISACompilerStats to compilerStats
                compilerStats.MergeStats(vISACompilerStats, simdsize);

                compilerStats.SetF64("TimeVISACompile",
                                     context->m_compilerTimeStats->getCompileTimeMS(TIME_CG_vISACompile), simdsize);
                // CompileTimeStats adds up the timings for simd8+simd16 in simd16 and timings for simd8+simd16+simd32 in simd32.
                // So, we need to make the appropriate adjustments to get the true simd16/simd32 timings.
                if (simdsize == 16)
                {
                    compilerStats.SetF64("TimeVISACompile",
                                  compilerStats.GetF64("TimeVISACompile", 16)
                                  - compilerStats.GetF64("TimeVISACompile", 8), simdsize);
                }
                else if (simdsize == 32)
                {
                    compilerStats.SetF64("TimeVISACompile",
                                  compilerStats.GetF64("TimeVISACompile", 32)
                                  - compilerStats.GetF64("TimeVISACompile", 16)
                                  - compilerStats.GetF64("TimeVISACompile", 8), simdsize);
                }
                compilerStats.SetI64("numGRFSpillFill", jitInfo->numGRFSpillFill, simdsize);
                compilerStats.SetI64("numFlagSpillFill", jitInfo->numFlagSpillStore + jitInfo->numFlagSpillLoad, simdsize);
                compilerStats.SetI64("numInst", jitInfo->numAsmCount, simdsize);
            }
#endif // COMPILER_STATS_ENABLE
        }

        void OutputCompilerStats(IGC::CodeGenContext *context)
        {
#if COMPILER_STATS_ENABLE
            if (IGC_IS_FLAG_ENABLED(DumpCompilerStats))
            {
                ShaderHash hash = context->hash;
                ShaderType shaderType = context->type;
                CompilerStats cStats = context->Stats();
                std::string cStatsFileName, shaderName;
                cStatsFileName =
                    IGC::Debug::DumpName(IGC::Debug::GetShaderOutputName())
                    .Type(shaderType)
                    .Hash(hash)
                    .PostFix("CompileTimeStats")
                    .Extension("csv")
                    .str();
                shaderName =
                    IGC::Debug::DumpName(IGC::Debug::GetShaderOutputName())
                    .Type(shaderType)
                    .Hash(hash)
                    .RelativePath();
                FILE* cStatsFile = fopen(cStatsFileName.c_str(), "wb+");
                if (cStatsFile != NULL)
                {
                    fprintf(cStatsFile,"\nShader: %s\n", shaderName.c_str());
                    fprintf(cStatsFile,"%s", cStats.ToCsv().c_str());
                    fclose(cStatsFile);
                }
            }
#endif // COMPILER_STATS_ENABLE
        }
    }
}
