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
#include "3d/common/iStdLib/types.h"
#include "3d/common/iStdLib/utility.h"
#include "common/debug/Dump.hpp"
#include "inc/common/sku_wa.h"
#include "compiler_caps.h"
#include "Compiler/CISACodeGen/Platform.hpp"
#include "inc/common/secure_mem.h"

#include <string.h>
#include <sstream>

using namespace IGC::Debug;

// Sku is used only in cmd_3d_caps_gx files
#define GHAL3D_IS_SKU( pGlobalData, SkuName )   GFX_IS_SKU( ( const HW_STATUS* )( pGlobalData ), SkuName )

struct HW_STATUS
{
    SKU_FEATURE_TABLE& SkuTable;
    HW_STATUS(SKU_FEATURE_TABLE* pSkuTable) : SkuTable(*pSkuTable)
    {

    }
};

namespace IGC
{

void ConvertSkuTable(const SUscSkuFeatureTable* pUSCSkuFeatureTable, SKU_FEATURE_TABLE& SkuFeatureTable);

template <class type>
static std::string stringFrom(type in)
{
    std::ostringstream ss;
    ss << in;
    return  ss.str();
}

static void CreateCompilerCapsString(const GT_SYSTEM_INFO* sysinfo, PLATFORM platformInfo, std::string &outputString)
{
    outputString.append("// Hardware Caps :\n");
    outputString.append("EUCount \t\t= \t");
    outputString.append(stringFrom<unsigned int>(sysinfo->EUCount));
    outputString.append("\n");
    outputString.append("ThreadCount \t\t= \t");
    outputString.append(stringFrom<unsigned int>(sysinfo->ThreadCount));
    outputString.append("\n");
    outputString.append("SliceCount \t\t= \t");
    outputString.append(stringFrom<unsigned int>(sysinfo->SliceCount));
    outputString.append("\n");
    outputString.append("SubSliceCount \t\t= \t");
    outputString.append(stringFrom<unsigned int>(sysinfo->SubSliceCount));
    outputString.append("\n");
    outputString.append("TotalPsThreadsWindowerRange \t\t= \t");
    outputString.append(stringFrom<unsigned int>(sysinfo->TotalPsThreadsWindowerRange));
    outputString.append("\n");
    outputString.append("TotalVsThreads \t\t= \t");
    outputString.append(stringFrom<unsigned int>(sysinfo->TotalVsThreads));
    outputString.append("TotalVsThreads_Pocs \t\t= \t");
    outputString.append(stringFrom<unsigned int>(sysinfo->TotalVsThreads_Pocs));
    outputString.append("\n");
    outputString.append("TotalGsThreads \t\t= \t");
    outputString.append(stringFrom<unsigned int>(sysinfo->TotalGsThreads));
    outputString.append("\n");
    outputString.append("TotalDsThreads \t\t= \t");
    outputString.append(stringFrom<unsigned int>(sysinfo->TotalDsThreads));
    outputString.append("\n");
    outputString.append("TotalHsThreads \t\t= \t");
    outputString.append(stringFrom<unsigned int>(sysinfo->TotalHsThreads));
    outputString.append("\n");
    outputString.append("MaxEuPerSubSlice \t\t= \t");
    outputString.append(stringFrom<unsigned int>(sysinfo->MaxEuPerSubSlice));
    outputString.append("\n");
    outputString.append("EuCountPerPoolMax \t\t= \t");
    outputString.append(stringFrom<unsigned int>(sysinfo->EuCountPerPoolMax));
    outputString.append("\n");
    outputString.append("EuCountPerPoolMin \t\t= \t");
    outputString.append(stringFrom<unsigned int>(sysinfo->EuCountPerPoolMin));
    outputString.append("\n");
    outputString.append("MaxSlicesSupported \t\t= \t");
    outputString.append(stringFrom<unsigned int>(sysinfo->MaxSlicesSupported));
    outputString.append("\n");
    outputString.append("MaxSubSlicesSupported \t\t= \t");
    outputString.append(stringFrom<unsigned int>(sysinfo->MaxSubSlicesSupported));
    outputString.append("\n");


    std::stringstream ss;
    outputString.append("UsDeviceID \t\t= \t");
    ss << "0x" << std::hex << platformInfo.usDeviceID;
    outputString.append(ss.str());
    outputString.append("\n");
    ss.str(std::string());
    outputString.append("UsRevId \t\t= \t");
    ss << "0x" << std::hex << platformInfo.usRevId;
    outputString.append(ss.str());
    outputString.append("\n");
    ss.str(std::string());
    outputString.append("UsDeviceID_PCH \t\t= \t");
    ss << "0x" << std::hex << platformInfo.usDeviceID_PCH;
    outputString.append(ss.str());
    outputString.append("\n");
    ss.str(std::string());
    outputString.append("UsRevId_PCH \t\t= \t");
    ss << "0x" << std::hex << platformInfo.usRevId_PCH;
    outputString.append(ss.str());
    outputString.append("\n");

}

static void DumpCaps(const GT_SYSTEM_INFO sysinfo, PLATFORM platformInfo, Debug::Dump const& dump)
{
    std::string outputString;
    CreateCompilerCapsString(&sysinfo, platformInfo, outputString);
    dump.stream() << outputString;
}

void SetCompilerCaps(SKU_FEATURE_TABLE* pSkuFeatureTable, CPlatform* platform)
{
    //In case we need feature flags in the future
    HW_STATUS hwStatus(pSkuFeatureTable);
    SCompilerHwCaps caps;
    GT_SYSTEM_INFO sysinfo = platform->GetGTSystemInfo();

    caps.PixelShaderThreadsWindowerRange = sysinfo.TotalPsThreadsWindowerRange;
    caps.VertexShaderThreads = sysinfo.TotalVsThreads;
    caps.VertexShaderThreadsPosh = sysinfo.TotalVsThreads_Pocs;
    caps.GeometryShaderThreads = sysinfo.TotalGsThreads;
    caps.DomainShaderThreads = sysinfo.TotalDsThreads;
    caps.HullShaderThreads = sysinfo.TotalHsThreads;
    caps.MediaShaderThreads = sysinfo.ThreadCount;
    caps.SharedLocalMemoryBlockSize      = 1 * sizeof(KILOBYTE);

    caps.KernelHwCaps.SliceCount = sysinfo.SliceCount;
    caps.KernelHwCaps.SubSliceCount = sysinfo.SubSliceCount;
    caps.KernelHwCaps.ThreadCount = sysinfo.ThreadCount;
    caps.KernelHwCaps.EUCount = sysinfo.EUCount;
    caps.KernelHwCaps.EUCountPerSubSlice = sysinfo.MaxEuPerSubSlice;
    caps.KernelHwCaps.EUCountPerPoolMax = sysinfo.EuCountPerPoolMax;
    caps.KernelHwCaps.CsrSizeInMb = sysinfo.CsrSizeInMb;

    //TODO: fix TC_tester; Set dummy values in TC_Tester
    if (sysinfo.EUCount) {
        caps.KernelHwCaps.EUThreadsPerEU = sysinfo.ThreadCount/ sysinfo.EUCount;
    } else {
        caps.KernelHwCaps.EUThreadsPerEU = 0;
    }

    caps.KernelHwCaps.KernelPointerAlignSize = 64 * sizeof(BYTE);

    //Dump caps to a text file
    if (IGC_IS_FLAG_ENABLED(EnableCapsDump))
    {
        auto name = DumpName("HardwareCaps");
        IGC::DumpCaps(sysinfo, platform->getPlatformInfo(), Dump(name.Extension("txt"), Debug::DumpType::COS_TEXT));
    }
    platform->SetCaps(caps);
}


void SetCompilerCaps(const SUscSkuFeatureTable* pSkuFeatureTable, CPlatform* platform)
{
    SKU_FEATURE_TABLE SkuFeatureTable;
    ConvertSkuTable(pSkuFeatureTable, SkuFeatureTable);
    SetCompilerCaps(&SkuFeatureTable, platform);
}

} //namespace IGC
