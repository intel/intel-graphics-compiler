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
#ifndef IGA_FRONTEND_LDSTSYNTAX_MTYPES_HPP
#define IGA_FRONTEND_LDSTSYNTAX_MTYPES_HPP
#include <ostream>
/*
 * NOTA BENE: THIS FILE WAS AUTOMATICALLY GENERATED FROM BXML VIA SCRIPTING
 * TOOLS IN THE DIRECTORY: $IGA_ROOT/bxml/send

 * If there's a bug in BXML, fix it there and then regenerate these files.
 */

namespace iga {
enum class MType {
    MSD_INVALID = 0,

    MF_GR_VEC_A64_D8,    // xga8     MSD1R_A64_BS     ld.xga8.[DE] (A64 Byte Gathering Read)
    MF_SW_VEC_A64_D8,    // xsc8     MSD1W_A64_BS     st.xsc8.[DE] (A64 Byte Scattering Write)
    MF_GR_VEC_A64_D32,   // xga32    MSD1R_A64_DWS    ld.xga32.[DE] (A64 DWord Gathering Read)
    MF_SW_VEC_A64_D32,   // xsc32    MSD1W_A64_DWS    st.xsc32.[DE] (A64 DWord Scattering Write)
    MF_GR_VEC_A64_D64,   // xga64    MSD1R_A64_QWS    ld.xga64.[DE] (A64 QWord Gathering Read)
    MF_SW_VEC_A64_D64,   // xsc64    MSD1W_A64_QWS    st.xsc64.[DE] (A64 QWord Scattering Write)
    MF_GR_VEC_A32_D8,    // ga8      MSD0R_BS         ld.ga8.[DE] (Byte Gathering Read)
    MF_SW_VEC_A32_D8,    // sc8      MSD0W_BS         st.sc8.[DE] (Byte Scattering Write)
    MF_GR_VEC_A32_D32,   // ga32     MSD0R_DWS        ld.ga32.[DE] (DWord Gathering Read)
    MF_SW_VEC_A32_D32,   // sc32     MSD0W_DWS        st.sc32.[DE] (DWord Scattering Write)
    MF_CGR_VEC_A32_D32,  // ga32c    MSDCR_DWS        ld.ga32c.[DE] (Constant DWord Gathering Read)
    MF_BR_A64_D256,      // xbl256   MSD1R_A64_HWB    ld.xbl256.[HW64] (A64 HWord Block Read)
    MF_BW_A64_D256,      // xbl256   MSD1W_A64_HWB    st.xbl256.[HW64] (A64 HWord Block Write)
    MF_BR_A64_D128,      // xbl128   MSD1R_A64_OWB    ld.xbl128.[OWs] (A64 OWord Block Read)
    MF_BW_A64_D128,      // xbl128   MSD1W_A64_OWB    st.xbl128.[OWs] (A64 OWord Block Write)
    MF_UBR_A64_D128,     // xubl128  MSD1R_A64_OWUB   ld.xubl128.[OWs] (Unaligned A64 OWord Block Read)
    MF_BR_A32_D128,      // bl128    MSD0R_OWB        ld.bl128.[OWs] (OWord Block Read)
    MF_BW_A32_D128,      // bl128    MSD0W_OWB        st.bl128.[OWs] (OWord Block Write)
    MF_ABR_A32_D128,     // abl128   MSD0R_OWAB       ld.abl128.[OWs] (Aligned OWord Block Read)
    MF_CBR_A32_D128,     // cbl128   MSDCR_OWB        ld.cbl128.[OWs] (Constant OWord Block Read)
    MF_CUBR_A32_D128,    // cubl128  MSDCR_OWUB       ld.cubl128.[OWs] (Constant Unaligned OWord Block Read)
    MF_SBR_A32_D256,     // sbl256   MSD0R_SB         ld.sbl256.[HWSB] (Scratch HWord Block Read)
    MF_SBW_A32_D256,     // sbl256   MSD0W_SB         st.sbl256.[HWSB] (Scratch HWord Block Write)
    MF_USR_A64_D32,      // xus      MSD1R_A64_US     ld.xus.[CMask] (A64 Untyped Surface Read)
    MF_USW_A64_D32,      // xus      MSD1W_A64_US     st.xus.[CMaskUW] (A64 Untyped Surface Write)
    MF_USR_A32_D32,      // us       MSD1R_US         ld.us.[CMask] (A32 Untyped Surface Read)
    MF_USW_A32_D32,      // us       MSD1W_US         st.us.[CMaskUW] (A32 Untyped Surface Write)
    MF_TSR_A32_D32,      // ts       MSD1R_TS         ld.ts.[SG3].[CMask] (A32 Typed Surface Read)
    MF_TSW_A32_D32,      // ts       MSD1W_TS         st.ts.[SG3].[CMaskUW] (A32 Typed Surface Write)
};

static void ToSymbol(MType mt, std::ostream &os)
{
    switch (mt){
    case MType::MF_GR_VEC_A64_D8: os << "MF_GR_VEC_A64_D8"; break;
    case MType::MF_SW_VEC_A64_D8: os << "MF_SW_VEC_A64_D8"; break;
    case MType::MF_GR_VEC_A64_D32: os << "MF_GR_VEC_A64_D32"; break;
    case MType::MF_SW_VEC_A64_D32: os << "MF_SW_VEC_A64_D32"; break;
    case MType::MF_GR_VEC_A64_D64: os << "MF_GR_VEC_A64_D64"; break;
    case MType::MF_SW_VEC_A64_D64: os << "MF_SW_VEC_A64_D64"; break;
    case MType::MF_GR_VEC_A32_D8: os << "MF_GR_VEC_A32_D8"; break;
    case MType::MF_SW_VEC_A32_D8: os << "MF_SW_VEC_A32_D8"; break;
    case MType::MF_GR_VEC_A32_D32: os << "MF_GR_VEC_A32_D32"; break;
    case MType::MF_SW_VEC_A32_D32: os << "MF_SW_VEC_A32_D32"; break;
    case MType::MF_CGR_VEC_A32_D32: os << "MF_CGR_VEC_A32_D32"; break;
    case MType::MF_BR_A64_D256: os << "MF_BR_A64_D256"; break;
    case MType::MF_BW_A64_D256: os << "MF_BW_A64_D256"; break;
    case MType::MF_BR_A64_D128: os << "MF_BR_A64_D128"; break;
    case MType::MF_BW_A64_D128: os << "MF_BW_A64_D128"; break;
    case MType::MF_UBR_A64_D128: os << "MF_UBR_A64_D128"; break;
    case MType::MF_BR_A32_D128: os << "MF_BR_A32_D128"; break;
    case MType::MF_BW_A32_D128: os << "MF_BW_A32_D128"; break;
    case MType::MF_ABR_A32_D128: os << "MF_ABR_A32_D128"; break;
    case MType::MF_CBR_A32_D128: os << "MF_CBR_A32_D128"; break;
    case MType::MF_CUBR_A32_D128: os << "MF_CUBR_A32_D128"; break;
    case MType::MF_SBR_A32_D256: os << "MF_SBR_A32_D256"; break;
    case MType::MF_SBW_A32_D256: os << "MF_SBW_A32_D256"; break;
    case MType::MF_USR_A64_D32: os << "MF_USR_A64_D32"; break;
    case MType::MF_USW_A64_D32: os << "MF_USW_A64_D32"; break;
    case MType::MF_USR_A32_D32: os << "MF_USR_A32_D32"; break;
    case MType::MF_USW_A32_D32: os << "MF_USW_A32_D32"; break;
    case MType::MF_TSR_A32_D32: os << "MF_TSR_A32_D32"; break;
    case MType::MF_TSW_A32_D32: os << "MF_TSW_A32_D32"; break;
    default: os << "MType::" << static_cast<int>(mt);
    }
}
}
#endif // IGA_FRONTEND_LDSTSYNTAX_MTYPES_HPP
