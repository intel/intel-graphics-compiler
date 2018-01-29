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


// SVML code
/*
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 1996-2010 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __SVML_TANPI_DATA_CL__
#define __SVML_TANPI_DATA_CL__

#include "_svml_data_structures.cl"

typedef struct
{
    VUINT32 _sCoeffs[128][10];
    VUINT32 _sAbsMask;
}
sTanpi_Table_Type;


__constant sTanpi_Table_Type __ocl_svml_stanpi_data = {
    {
        {
            (VUINT32) (0x3FC90FDBu),
            (VUINT32) (0xB33BBD2Eu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3EAAACDDu),
            (VUINT32) (0x00000000u)
        },
        {
            (VUINT32) (0x3FC5EB9Bu),
            (VUINT32) (0x32DE638Cu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3CC91A31u),
            (VUINT32) (0x2F8E8D1Au),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3A1DFA00u),
            (VUINT32) (0x3CC9392Du),
            (VUINT32) (0x3EAB1889u),
            (VUINT32) (0x3C885D3Bu)
        },
        {
            (VUINT32) (0x3FC2C75Cu),
            (VUINT32) (0xB2CBBE8Au),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3D49393Cu),
            (VUINT32) (0x30A39F5Bu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3B1E2B00u),
            (VUINT32) (0x3D49B5D4u),
            (VUINT32) (0x3EAC4F10u),
            (VUINT32) (0x3CFD9425u)
        },
        {
            (VUINT32) (0x3FBFA31Cu),
            (VUINT32) (0x33450FB0u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3D9711CEu),
            (VUINT32) (0x314FEB28u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3BB24C00u),
            (VUINT32) (0x3D97E43Au),
            (VUINT32) (0x3EAE6A89u),
            (VUINT32) (0x3D4D07E0u)
        },
        {
            (VUINT32) (0x3FBC7EDDu),
            (VUINT32) (0xB1800ADDu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3DC9B5DCu),
            (VUINT32) (0x3145AD86u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3C1EEF20u),
            (VUINT32) (0x3DCBAAEAu),
            (VUINT32) (0x3EB14E5Eu),
            (VUINT32) (0x3D858BB2u)
        },
        {
            (VUINT32) (0x3FB95A9Eu),
            (VUINT32) (0xB3651267u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3DFC98C2u),
            (VUINT32) (0xB0AE525Cu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3C793D20u),
            (VUINT32) (0x3E003845u),
            (VUINT32) (0x3EB5271Fu),
            (VUINT32) (0x3DAC669Eu)
        },
        {
            (VUINT32) (0x3FB6365Eu),
            (VUINT32) (0x328BB91Cu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3E17E564u),
            (VUINT32) (0xB1C5A2E4u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3CB440D0u),
            (VUINT32) (0x3E1B3D00u),
            (VUINT32) (0x3EB9F664u),
            (VUINT32) (0x3DD647C0u)
        },
        {
            (VUINT32) (0x3FB3121Fu),
            (VUINT32) (0xB30F347Du),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3E31AE4Du),
            (VUINT32) (0xB1F32251u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3CF6A500u),
            (VUINT32) (0x3E3707DAu),
            (VUINT32) (0x3EBFA489u),
            (VUINT32) (0x3DFBD9C7u)
        },
        {
            (VUINT32) (0x3FAFEDDFu),
            (VUINT32) (0x331BBA77u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3E4BAFAFu),
            (VUINT32) (0x2F2A29E0u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3D221018u),
            (VUINT32) (0x3E53BED0u),
            (VUINT32) (0x3EC67E26u),
            (VUINT32) (0x3E1568E2u)
        },
        {
            (VUINT32) (0x3FACC9A0u),
            (VUINT32) (0xB2655A50u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3E65F267u),
            (VUINT32) (0x31B4B1DFu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3D4E8B90u),
            (VUINT32) (0x3E718ACAu),
            (VUINT32) (0x3ECE7164u),
            (VUINT32) (0x3E2DC161u)
        },
        {
            (VUINT32) (0x3FA9A560u),
            (VUINT32) (0x33719861u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3E803FD4u),
            (VUINT32) (0xB2279E66u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3D807FC8u),
            (VUINT32) (0x3E884BD4u),
            (VUINT32) (0x3ED7812Du),
            (VUINT32) (0x3E4636EBu)
        },
        {
            (VUINT32) (0x3FA68121u),
            (VUINT32) (0x31E43AACu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3E8DB082u),
            (VUINT32) (0xB132A234u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3D9CD7D0u),
            (VUINT32) (0x3E988A60u),
            (VUINT32) (0x3EE203E3u),
            (VUINT32) (0x3E63582Cu)
        },
        {
            (VUINT32) (0x3FA35CE2u),
            (VUINT32) (0xB33889B6u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3E9B5042u),
            (VUINT32) (0xB22A3AEEu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3DBC7490u),
            (VUINT32) (0x3EA99AF5u),
            (VUINT32) (0x3EEDE107u),
            (VUINT32) (0x3E80E9AAu)
        },
        {
            (VUINT32) (0x3FA038A2u),
            (VUINT32) (0x32E4CA7Eu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3EA92457u),
            (VUINT32) (0x30B80830u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3DDF8200u),
            (VUINT32) (0x3EBB99E9u),
            (VUINT32) (0x3EFB4AA8u),
            (VUINT32) (0x3E9182BEu)
        },
        {
            (VUINT32) (0x3F9D1463u),
            (VUINT32) (0xB2C55799u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3EB73250u),
            (VUINT32) (0xB2028823u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E0318F8u),
            (VUINT32) (0x3ECEA678u),
            (VUINT32) (0x3F053C67u),
            (VUINT32) (0x3EA41E53u)
        },
        {
            (VUINT32) (0x3F99F023u),
            (VUINT32) (0x33484328u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3EC5800Du),
            (VUINT32) (0xB214C3C1u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E185E54u),
            (VUINT32) (0x3EE2E342u),
            (VUINT32) (0x3F0DCA73u),
            (VUINT32) (0x3EB8CC21u)
        },
        {
            (VUINT32) (0x3F96CBE4u),
            (VUINT32) (0xB14CDE2Eu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3ED413CDu),
            (VUINT32) (0xB1C06152u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E2FB0CCu),
            (VUINT32) (0x3EF876CBu),
            (VUINT32) (0x3F177807u),
            (VUINT32) (0x3ED08437u)
        },
        {
            (VUINT32) (0x3F93A7A5u),
            (VUINT32) (0xB361DEEEu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3EE2F439u),
            (VUINT32) (0xB1F4399Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E49341Cu),
            (VUINT32) (0x3F07C61Au),
            (VUINT32) (0x3F22560Fu),
            (VUINT32) (0x3EEAA81Eu)
        },
        {
            (VUINT32) (0x3F908365u),
            (VUINT32) (0x3292200Du),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3EF22870u),
            (VUINT32) (0x325271F4u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E65107Au),
            (VUINT32) (0x3F1429F0u),
            (VUINT32) (0x3F2E8AFCu),
            (VUINT32) (0x3F040498u)
        },
        {
            (VUINT32) (0x3F8D5F26u),
            (VUINT32) (0xB30C0105u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3F00DC0Du),
            (VUINT32) (0xB214AF72u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E81B994u),
            (VUINT32) (0x3F218233u),
            (VUINT32) (0x3F3C4531u),
            (VUINT32) (0x3F149688u)
        },
        {
            (VUINT32) (0x3F8A3AE6u),
            (VUINT32) (0x331EEDF0u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3F08D5B9u),
            (VUINT32) (0xB25EF98Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E92478Du),
            (VUINT32) (0x3F2FEDC9u),
            (VUINT32) (0x3F4BCD58u),
            (VUINT32) (0x3F27AE9Eu)
        },
        {
            (VUINT32) (0x3F8716A7u),
            (VUINT32) (0xB2588C6Du),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3F1105AFu),
            (VUINT32) (0x32F045B0u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3EA44EE2u),
            (VUINT32) (0x3F3F8FDBu),
            (VUINT32) (0x3F5D3FD0u),
            (VUINT32) (0x3F3D0A23u)
        },
        {
            (VUINT32) (0x3F83F267u),
            (VUINT32) (0x3374CBD9u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3F1970C4u),
            (VUINT32) (0x32904848u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3EB7EFF8u),
            (VUINT32) (0x3F50907Cu),
            (VUINT32) (0x3F710FEAu),
            (VUINT32) (0x3F561FEDu)
        },
        {
            (VUINT32) (0x3F80CE28u),
            (VUINT32) (0x31FDD672u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3F221C37u),
            (VUINT32) (0xB20C61DCu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3ECD4F71u),
            (VUINT32) (0x3F631DAAu),
            (VUINT32) (0x3F83B471u),
            (VUINT32) (0x3F7281EAu)
        },
        {
            (VUINT32) (0x3F7B53D1u),
            (VUINT32) (0x32955386u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3F2B0DC1u),
            (VUINT32) (0x32AB7EBAu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3EE496C2u),
            (VUINT32) (0x3F776C40u),
            (VUINT32) (0x3F9065C1u),
            (VUINT32) (0x3F89AFB6u)
        },
        {
            (VUINT32) (0x3F750B52u),
            (VUINT32) (0x32EB316Fu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3F344BA9u),
            (VUINT32) (0xB2B8B0EAu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3EFDF4F7u),
            (VUINT32) (0x3F86DCA8u),
            (VUINT32) (0x3F9ED53Bu),
            (VUINT32) (0x3F9CBEDEu)
        },
        {
            (VUINT32) (0x3F6EC2D4u),
            (VUINT32) (0xB2BEF0A7u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3F3DDCCFu),
            (VUINT32) (0x32D29606u),
            (VUINT32) (0x40000000u),
            (VUINT32) (0xBEE6606Fu),
            (VUINT32) (0x3F9325D6u),
            (VUINT32) (0x3FAF4E69u),
            (VUINT32) (0x3FB3080Cu)
        },
        {
            (VUINT32) (0x3F687A55u),
            (VUINT32) (0xB252257Bu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3F47C8CCu),
            (VUINT32) (0xB200F51Au),
            (VUINT32) (0x40000000u),
            (VUINT32) (0xBEC82C6Cu),
            (VUINT32) (0x3FA0BAE9u),
            (VUINT32) (0x3FC2252Fu),
            (VUINT32) (0x3FCD24C7u)
        },
        {
            (VUINT32) (0x3F6231D6u),
            (VUINT32) (0xB119A6A2u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3F521801u),
            (VUINT32) (0x32AE4178u),
            (VUINT32) (0x40000000u),
            (VUINT32) (0xBEA72938u),
            (VUINT32) (0x3FAFCC22u),
            (VUINT32) (0x3FD7BD4Au),
            (VUINT32) (0x3FEBB01Bu)
        },
        {
            (VUINT32) (0x3F5BE957u),
            (VUINT32) (0x3205522Au),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3F5CD3BEu),
            (VUINT32) (0x31460308u),
            (VUINT32) (0x40000000u),
            (VUINT32) (0xBE8306C5u),
            (VUINT32) (0x3FC09232u),
            (VUINT32) (0x3FF09632u),
            (VUINT32) (0x4007DB00u)
        },

        {
            (VUINT32) (0x3F55A0D8u),
            (VUINT32) (0x329886FFu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3F68065Eu),
            (VUINT32) (0x32670D1Au),
            (VUINT32) (0x40000000u),
            (VUINT32) (0xBE36D1D6u),
            (VUINT32) (0x3FD35007u),
            (VUINT32) (0x4006A861u),
            (VUINT32) (0x401D4BDAu)
        },

        {
            (VUINT32) (0x3F4F5859u),
            (VUINT32) (0x32EE64E8u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3F73BB75u),
            (VUINT32) (0x32FC908Du),
            (VUINT32) (0x40000000u),
            (VUINT32) (0xBDBF94B0u),
            (VUINT32) (0x3FE8550Fu),
            (VUINT32) (0x40174F67u),
            (VUINT32) (0x4036C608u)
        },

        {
            (VUINT32) (0x3F490FDBu),
            (VUINT32) (0xB2BBBD2Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBE8BE60Eu),
            (VUINT32) (0x320D8D84u),
            (VUINT32) (0x3F000000u),
            (VUINT32) (0xBDF817B1u),
            (VUINT32) (0xBD8345EBu),
            (VUINT32) (0x3D1DFDACu),
            (VUINT32) (0xBC52CF6Fu)
        },

        {
            (VUINT32) (0x3F42C75Cu),
            (VUINT32) (0xB24BBE8Au),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBE87283Fu),
            (VUINT32) (0xB268B966u),
            (VUINT32) (0x3F000000u),
            (VUINT32) (0xBDFE6529u),
            (VUINT32) (0xBD7B1953u),
            (VUINT32) (0x3D18E109u),
            (VUINT32) (0xBC4570B0u)
        },

        {
            (VUINT32) (0x3F3C7EDDu),
            (VUINT32) (0xB1000ADDu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBE827420u),
            (VUINT32) (0x320B8B4Du),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DFB9428u),
            (VUINT32) (0xBD7002B4u),
            (VUINT32) (0x3D142A6Cu),
            (VUINT32) (0xBC3A47FFu)
        },

        {
            (VUINT32) (0x3F36365Eu),
            (VUINT32) (0x320BB91Cu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBE7B9282u),
            (VUINT32) (0xB13383D2u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DF5D211u),
            (VUINT32) (0xBD6542B3u),
            (VUINT32) (0x3D0FE5E5u),
            (VUINT32) (0xBC31FB14u)
        },

        {
            (VUINT32) (0x3F2FEDDFu),
            (VUINT32) (0x329BBA77u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBE724E73u),
            (VUINT32) (0x3120C3E2u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DF05283u),
            (VUINT32) (0xBD5AD45Eu),
            (VUINT32) (0x3D0BAFBFu),
            (VUINT32) (0xBC27B8BBu)
        },

        {
            (VUINT32) (0x3F29A560u),
            (VUINT32) (0x32F19861u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBE691B44u),
            (VUINT32) (0x31F18936u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DEB138Bu),
            (VUINT32) (0xBD50B2F7u),
            (VUINT32) (0x3D07BE3Au),
            (VUINT32) (0xBC1E46A7u)
        },

        {
            (VUINT32) (0x3F235CE2u),
            (VUINT32) (0xB2B889B6u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBE5FF82Cu),
            (VUINT32) (0xB170723Au),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DE61354u),
            (VUINT32) (0xBD46DA06u),
            (VUINT32) (0x3D0401F8u),
            (VUINT32) (0xBC14E013u)
        },

        {
            (VUINT32) (0x3F1D1463u),
            (VUINT32) (0xB2455799u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBE56E46Bu),
            (VUINT32) (0x31E3F001u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DE15025u),
            (VUINT32) (0xBD3D4550u),
            (VUINT32) (0x3D00462Du),
            (VUINT32) (0xBC092C98u)
        },

        {
            (VUINT32) (0x3F16CBE4u),
            (VUINT32) (0xB0CCDE2Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBE4DDF41u),
            (VUINT32) (0xB1AEA094u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DDCC85Cu),
            (VUINT32) (0xBD33F0BEu),
            (VUINT32) (0x3CFA23B0u),
            (VUINT32) (0xBC01FCF7u)
        },

        {
            (VUINT32) (0x3F108365u),
            (VUINT32) (0x3212200Du),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBE44E7F8u),
            (VUINT32) (0xB1CAA3CBu),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DD87A74u),
            (VUINT32) (0xBD2AD885u),
            (VUINT32) (0x3CF3C785u),
            (VUINT32) (0xBBF1E348u)
        },

        {
            (VUINT32) (0x3F0A3AE6u),
            (VUINT32) (0x329EEDF0u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBE3BFDDCu),
            (VUINT32) (0xB132521Au),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DD464FCu),
            (VUINT32) (0xBD21F8F1u),
            (VUINT32) (0x3CEE3076u),
            (VUINT32) (0xBBE6D263u)
        },

        {
            (VUINT32) (0x3F03F267u),
            (VUINT32) (0x32F4CBD9u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBE33203Eu),
            (VUINT32) (0x31FEF5BEu),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DD0869Cu),
            (VUINT32) (0xBD194E8Cu),
            (VUINT32) (0x3CE8DCA9u),
            (VUINT32) (0xBBDADA55u)
        },

        {
            (VUINT32) (0x3EFB53D1u),
            (VUINT32) (0x32155386u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBE2A4E71u),
            (VUINT32) (0xB19CFCECu),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DCCDE11u),
            (VUINT32) (0xBD10D605u),
            (VUINT32) (0x3CE382A7u),
            (VUINT32) (0xBBC8BD97u)
        },

        {
            (VUINT32) (0x3EEEC2D4u),
            (VUINT32) (0xB23EF0A7u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBE2187D0u),
            (VUINT32) (0xB1B7C7F7u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DC96A2Bu),
            (VUINT32) (0xBD088C22u),
            (VUINT32) (0x3CDE950Eu),
            (VUINT32) (0xBBB89AD1u)
        },

        {
            (VUINT32) (0x3EE231D6u),
            (VUINT32) (0xB099A6A2u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBE18CBB7u),
            (VUINT32) (0xAFE28430u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DC629CEu),
            (VUINT32) (0xBD006DCDu),
            (VUINT32) (0x3CDA5A2Cu),
            (VUINT32) (0xBBB0B3D2u)
        },

        {
            (VUINT32) (0x3ED5A0D8u),
            (VUINT32) (0x321886FFu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBE101985u),
            (VUINT32) (0xB02FB2B8u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DC31BF3u),
            (VUINT32) (0xBCF0F04Du),
            (VUINT32) (0x3CD60BC7u),
            (VUINT32) (0xBBA138BAu)
        },

        {
            (VUINT32) (0x3EC90FDBu),
            (VUINT32) (0xB23BBD2Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBE07709Du),
            (VUINT32) (0xB18A2A83u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DC03FA2u),
            (VUINT32) (0xBCE15096u),
            (VUINT32) (0x3CD26472u),
            (VUINT32) (0xBB9A1270u)
        },

        {
            (VUINT32) (0x3EBC7EDDu),
            (VUINT32) (0xB0800ADDu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBDFDA0CBu),
            (VUINT32) (0x2F14FCA0u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DBD93F7u),
            (VUINT32) (0xBCD1F71Bu),
            (VUINT32) (0x3CCEDD2Bu),
            (VUINT32) (0xBB905946u)
        },

        {
            (VUINT32) (0x3EAFEDDFu),
            (VUINT32) (0x321BBA77u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBDEC708Cu),
            (VUINT32) (0xB14895C4u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DBB181Eu),
            (VUINT32) (0xBCC2DEA6u),
            (VUINT32) (0x3CCB5027u),
            (VUINT32) (0xBB7F3969u)
        },

        {
            (VUINT32) (0x3EA35CE2u),
            (VUINT32) (0xB23889B6u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBDDB4F55u),
            (VUINT32) (0x30F6437Eu),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DB8CB52u),
            (VUINT32) (0xBCB40210u),
            (VUINT32) (0x3CC82D45u),
            (VUINT32) (0xBB643075u)
        },

        {
            (VUINT32) (0x3E96CBE4u),
            (VUINT32) (0xB04CDE2Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBDCA3BFFu),
            (VUINT32) (0x311C95EAu),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DB6ACDEu),
            (VUINT32) (0xBCA55C5Bu),
            (VUINT32) (0x3CC5BC04u),
            (VUINT32) (0xBB63A969u)
        },

        {
            (VUINT32) (0x3E8A3AE6u),
            (VUINT32) (0x321EEDF0u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBDB93569u),
            (VUINT32) (0xAFB9ED00u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DB4BC1Fu),
            (VUINT32) (0xBC96E905u),
            (VUINT32) (0x3CC2E6F5u),
            (VUINT32) (0xBB3E10A6u)
        },

        {
            (VUINT32) (0x3E7B53D1u),
            (VUINT32) (0x31955386u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBDA83A77u),
            (VUINT32) (0x316D967Au),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DB2F87Cu),
            (VUINT32) (0xBC88A31Fu),
            (VUINT32) (0x3CC0E763u),
            (VUINT32) (0xBB3F1666u)
        },

        {
            (VUINT32) (0x3E6231D6u),
            (VUINT32) (0xB019A6A2u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBD974A0Du),
            (VUINT32) (0xB14F365Bu),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DB1616Fu),
            (VUINT32) (0xBC750CD8u),
            (VUINT32) (0x3CBEB595u),
            (VUINT32) (0xBB22B883u)
        },

        {
            (VUINT32) (0x3E490FDBu),
            (VUINT32) (0xB1BBBD2Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBD866317u),
            (VUINT32) (0xAFF02140u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DAFF67Du),
            (VUINT32) (0xBC591CD0u),
            (VUINT32) (0x3CBCBEADu),
            (VUINT32) (0xBB04BBECu)
        },

        {
            (VUINT32) (0x3E2FEDDFu),
            (VUINT32) (0x319BBA77u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBD6B08FFu),
            (VUINT32) (0xB0EED236u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DAEB739u),
            (VUINT32) (0xBC3D6D51u),
            (VUINT32) (0x3CBB485Du),
            (VUINT32) (0xBAFFF5BAu)
        },

        {
            (VUINT32) (0x3E16CBE4u),
            (VUINT32) (0xAFCCDE2Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBD495A6Cu),
            (VUINT32) (0xB0A427BDu),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DADA345u),
            (VUINT32) (0xBC21F648u),
            (VUINT32) (0x3CB9D1B4u),
            (VUINT32) (0xBACB5567u)
        },

        {
            (VUINT32) (0x3DFB53D1u),
            (VUINT32) (0x31155386u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBD27B856u),
            (VUINT32) (0xB0F7EE91u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DACBA4Eu),
            (VUINT32) (0xBC06AEE3u),
            (VUINT32) (0x3CB8E5DCu),
            (VUINT32) (0xBAEC00EEu)
        },

        {
            (VUINT32) (0x3DC90FDBu),
            (VUINT32) (0xB13BBD2Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBD0620A3u),
            (VUINT32) (0xB0ECAB40u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DABFC11u),
            (VUINT32) (0xBBD7200Fu),
            (VUINT32) (0x3CB79475u),
            (VUINT32) (0xBA2B0ADCu)
        },

        {
            (VUINT32) (0x3D96CBE4u),
            (VUINT32) (0xAF4CDE2Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBCC92278u),
            (VUINT32) (0x302F2E68u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DAB6854u),
            (VUINT32) (0xBBA1214Fu),
            (VUINT32) (0x3CB6C1E9u),
            (VUINT32) (0x3843C2F3u)
        },

        {
            (VUINT32) (0x3D490FDBu),
            (VUINT32) (0xB0BBBD2Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBC861015u),
            (VUINT32) (0xAFD68E2Eu),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DAAFEEBu),
            (VUINT32) (0xBB569F3Fu),
            (VUINT32) (0x3CB6A84Eu),
            (VUINT32) (0xBAC64194u)
        },

        {
            (VUINT32) (0x3CC90FDBu),
            (VUINT32) (0xB03BBD2Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0xBC060BF3u),
            (VUINT32) (0x2FE251AEu),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DAABFB9u),
            (VUINT32) (0xBAD67C60u),
            (VUINT32) (0x3CB64CA5u),
            (VUINT32) (0xBACDE881u)
        },

        {
            (VUINT32) (0x00000000u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DAAAAABu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0x3CB5E28Bu),
            (VUINT32) (0x00000000u)
        },

        {
            (VUINT32) (0xBCC90FDBu),
            (VUINT32) (0x303BBD2Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3C060BF3u),
            (VUINT32) (0xAFE251AEu),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DAABFB9u),
            (VUINT32) (0x3AD67C60u),
            (VUINT32) (0x3CB64CA5u),
            (VUINT32) (0x3ACDE881u)
        },

        {
            (VUINT32) (0xBD490FDBu),
            (VUINT32) (0x30BBBD2Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3C861015u),
            (VUINT32) (0x2FD68E2Eu),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DAAFEEBu),
            (VUINT32) (0x3B569F3Fu),
            (VUINT32) (0x3CB6A84Eu),
            (VUINT32) (0x3AC64194u)
        },

        {
            (VUINT32) (0xBD96CBE4u),
            (VUINT32) (0x2F4CDE2Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3CC92278u),
            (VUINT32) (0xB02F2E68u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DAB6854u),
            (VUINT32) (0x3BA1214Fu),
            (VUINT32) (0x3CB6C1E9u),
            (VUINT32) (0xB843C2F2u)
        },

        {
            (VUINT32) (0xBDC90FDBu),
            (VUINT32) (0x313BBD2Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3D0620A3u),
            (VUINT32) (0x30ECAB40u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DABFC11u),
            (VUINT32) (0x3BD7200Fu),
            (VUINT32) (0x3CB79475u),
            (VUINT32) (0x3A2B0ADCu)
        },

        {
            (VUINT32) (0xBDFB53D1u),
            (VUINT32) (0xB1155386u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3D27B856u),
            (VUINT32) (0x30F7EE91u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DACBA4Eu),
            (VUINT32) (0x3C06AEE3u),
            (VUINT32) (0x3CB8E5DCu),
            (VUINT32) (0x3AEC00EEu)
        },

        {
            (VUINT32) (0xBE16CBE4u),
            (VUINT32) (0x2FCCDE2Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3D495A6Cu),
            (VUINT32) (0x30A427BDu),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DADA345u),
            (VUINT32) (0x3C21F648u),
            (VUINT32) (0x3CB9D1B4u),
            (VUINT32) (0x3ACB5567u)
        },

        {
            (VUINT32) (0xBE2FEDDFu),
            (VUINT32) (0xB19BBA77u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3D6B08FFu),
            (VUINT32) (0x30EED236u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DAEB739u),
            (VUINT32) (0x3C3D6D51u),
            (VUINT32) (0x3CBB485Du),
            (VUINT32) (0x3AFFF5BAu)
        },

        {
            (VUINT32) (0xBE490FDBu),
            (VUINT32) (0x31BBBD2Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3D866317u),
            (VUINT32) (0x2FF02140u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DAFF67Du),
            (VUINT32) (0x3C591CD0u),
            (VUINT32) (0x3CBCBEADu),
            (VUINT32) (0x3B04BBECu)
        },

        {
            (VUINT32) (0xBE6231D6u),
            (VUINT32) (0x3019A6A2u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3D974A0Du),
            (VUINT32) (0x314F365Bu),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DB1616Fu),
            (VUINT32) (0x3C750CD8u),
            (VUINT32) (0x3CBEB595u),
            (VUINT32) (0x3B22B883u)
        },

        {
            (VUINT32) (0xBE7B53D1u),
            (VUINT32) (0xB1955386u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3DA83A77u),
            (VUINT32) (0xB16D967Au),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DB2F87Cu),
            (VUINT32) (0x3C88A31Fu),
            (VUINT32) (0x3CC0E763u),
            (VUINT32) (0x3B3F1666u)
        },

        {
            (VUINT32) (0xBE8A3AE6u),
            (VUINT32) (0xB21EEDF0u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3DB93569u),
            (VUINT32) (0x2FB9ED00u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DB4BC1Fu),
            (VUINT32) (0x3C96E905u),
            (VUINT32) (0x3CC2E6F5u),
            (VUINT32) (0x3B3E10A6u)
        },

        {
            (VUINT32) (0xBE96CBE4u),
            (VUINT32) (0x304CDE2Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3DCA3BFFu),
            (VUINT32) (0xB11C95EAu),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DB6ACDEu),
            (VUINT32) (0x3CA55C5Bu),
            (VUINT32) (0x3CC5BC04u),
            (VUINT32) (0x3B63A969u)
        },

        {
            (VUINT32) (0xBEA35CE2u),
            (VUINT32) (0x323889B6u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3DDB4F55u),
            (VUINT32) (0xB0F6437Eu),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DB8CB52u),
            (VUINT32) (0x3CB40210u),
            (VUINT32) (0x3CC82D45u),
            (VUINT32) (0x3B643075u)
        },

        {
            (VUINT32) (0xBEAFEDDFu),
            (VUINT32) (0xB21BBA77u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3DEC708Cu),
            (VUINT32) (0x314895C4u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DBB181Eu),
            (VUINT32) (0x3CC2DEA6u),
            (VUINT32) (0x3CCB5027u),
            (VUINT32) (0x3B7F3969u)
        },

        {
            (VUINT32) (0xBEBC7EDDu),
            (VUINT32) (0x30800ADDu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3DFDA0CBu),
            (VUINT32) (0xAF14FCA0u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DBD93F7u),
            (VUINT32) (0x3CD1F71Bu),
            (VUINT32) (0x3CCEDD2Bu),
            (VUINT32) (0x3B905946u)
        },

        {
            (VUINT32) (0xBEC90FDBu),
            (VUINT32) (0x323BBD2Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E07709Du),
            (VUINT32) (0x318A2A83u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DC03FA2u),
            (VUINT32) (0x3CE15096u),
            (VUINT32) (0x3CD26472u),
            (VUINT32) (0x3B9A1270u)
        },

        {
            (VUINT32) (0xBED5A0D8u),
            (VUINT32) (0xB21886FFu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E101985u),
            (VUINT32) (0x302FB2B8u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DC31BF3u),
            (VUINT32) (0x3CF0F04Du),
            (VUINT32) (0x3CD60BC7u),
            (VUINT32) (0x3BA138BAu)
        },

        {
            (VUINT32) (0xBEE231D6u),
            (VUINT32) (0x3099A6A2u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E18CBB7u),
            (VUINT32) (0x2FE28430u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DC629CEu),
            (VUINT32) (0x3D006DCDu),
            (VUINT32) (0x3CDA5A2Cu),
            (VUINT32) (0x3BB0B3D2u)
        },

        {
            (VUINT32) (0xBEEEC2D4u),
            (VUINT32) (0x323EF0A7u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E2187D0u),
            (VUINT32) (0x31B7C7F7u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DC96A2Bu),
            (VUINT32) (0x3D088C22u),
            (VUINT32) (0x3CDE950Eu),
            (VUINT32) (0x3BB89AD1u)
        },

        {
            (VUINT32) (0xBEFB53D1u),
            (VUINT32) (0xB2155386u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E2A4E71u),
            (VUINT32) (0x319CFCECu),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DCCDE11u),
            (VUINT32) (0x3D10D605u),
            (VUINT32) (0x3CE382A7u),
            (VUINT32) (0x3BC8BD97u)
        },

        {
            (VUINT32) (0xBF03F267u),
            (VUINT32) (0xB2F4CBD9u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E33203Eu),
            (VUINT32) (0xB1FEF5BEu),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DD0869Cu),
            (VUINT32) (0x3D194E8Cu),
            (VUINT32) (0x3CE8DCA9u),
            (VUINT32) (0x3BDADA55u)
        },

        {
            (VUINT32) (0xBF0A3AE6u),
            (VUINT32) (0xB29EEDF0u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E3BFDDCu),
            (VUINT32) (0x3132521Au),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DD464FCu),
            (VUINT32) (0x3D21F8F1u),
            (VUINT32) (0x3CEE3076u),
            (VUINT32) (0x3BE6D263u)
        },

        {
            (VUINT32) (0xBF108365u),
            (VUINT32) (0xB212200Du),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E44E7F8u),
            (VUINT32) (0x31CAA3CBu),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DD87A74u),
            (VUINT32) (0x3D2AD885u),
            (VUINT32) (0x3CF3C785u),
            (VUINT32) (0x3BF1E348u)
        },

        {
            (VUINT32) (0xBF16CBE4u),
            (VUINT32) (0x30CCDE2Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E4DDF41u),
            (VUINT32) (0x31AEA094u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DDCC85Cu),
            (VUINT32) (0x3D33F0BEu),
            (VUINT32) (0x3CFA23B0u),
            (VUINT32) (0x3C01FCF7u)
        },

        {
            (VUINT32) (0xBF1D1463u),
            (VUINT32) (0x32455799u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E56E46Bu),
            (VUINT32) (0xB1E3F001u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DE15025u),
            (VUINT32) (0x3D3D4550u),
            (VUINT32) (0x3D00462Du),
            (VUINT32) (0x3C092C98u)
        },

        {
            (VUINT32) (0xBF235CE2u),
            (VUINT32) (0x32B889B6u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E5FF82Cu),
            (VUINT32) (0x3170723Au),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DE61354u),
            (VUINT32) (0x3D46DA06u),
            (VUINT32) (0x3D0401F8u),
            (VUINT32) (0x3C14E013u)
        },

        {
            (VUINT32) (0xBF29A560u),
            (VUINT32) (0xB2F19861u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E691B44u),
            (VUINT32) (0xB1F18936u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DEB138Bu),
            (VUINT32) (0x3D50B2F7u),
            (VUINT32) (0x3D07BE3Au),
            (VUINT32) (0x3C1E46A7u)
        },

        {
            (VUINT32) (0xBF2FEDDFu),
            (VUINT32) (0xB29BBA77u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E724E73u),
            (VUINT32) (0xB120C3E2u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DF05283u),
            (VUINT32) (0x3D5AD45Eu),
            (VUINT32) (0x3D0BAFBFu),
            (VUINT32) (0x3C27B8BBu)
        },

        {
            (VUINT32) (0xBF36365Eu),
            (VUINT32) (0xB20BB91Cu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E7B9282u),
            (VUINT32) (0x313383D2u),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DF5D211u),
            (VUINT32) (0x3D6542B3u),
            (VUINT32) (0x3D0FE5E5u),
            (VUINT32) (0x3C31FB14u)
        },
        {
            (VUINT32) (0xBF3C7EDDu),
            (VUINT32) (0x31000ADDu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E827420u),
            (VUINT32) (0xB20B8B4Du),
            (VUINT32) (0x3E800000u),
            (VUINT32) (0x3DFB9428u),
            (VUINT32) (0x3D7002B4u),
            (VUINT32) (0x3D142A6Cu),
            (VUINT32) (0x3C3A47FFu)
        },
        {
            (VUINT32) (0xBF42C75Cu),
            (VUINT32) (0x324BBE8Au),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E87283Fu),
            (VUINT32) (0x3268B966u),
            (VUINT32) (0x3F000000u),
            (VUINT32) (0xBDFE6529u),
            (VUINT32) (0x3D7B1953u),
            (VUINT32) (0x3D18E109u),
            (VUINT32) (0x3C4570B0u)
        },
        {
            (VUINT32) (0xBF490FDBu),
            (VUINT32) (0x32BBBD2Eu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBF800000u),
            (VUINT32) (0x2B410000u),
            (VUINT32) (0x40000000u),
            (VUINT32) (0xB3000000u),
            (VUINT32) (0xC0000000u),
            (VUINT32) (0x402AB7C8u),
            (VUINT32) (0xC05561DBu)
        },
        {
            (VUINT32) (0xBF4F5859u),
            (VUINT32) (0xB2EE64E8u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBF73BB75u),
            (VUINT32) (0xB2FC908Du),
            (VUINT32) (0x40000000u),
            (VUINT32) (0xBDBF94B0u),
            (VUINT32) (0xBFE8550Fu),
            (VUINT32) (0x40174F67u),
            (VUINT32) (0xC036C608u)
        },
        {
            (VUINT32) (0xBF55A0D8u),
            (VUINT32) (0xB29886FFu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBF68065Eu),
            (VUINT32) (0xB2670D1Au),
            (VUINT32) (0x40000000u),
            (VUINT32) (0xBE36D1D6u),
            (VUINT32) (0xBFD35007u),
            (VUINT32) (0x4006A861u),
            (VUINT32) (0xC01D4BDAu)
        },
        {
            (VUINT32) (0xBF5BE957u),
            (VUINT32) (0xB205522Au),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBF5CD3BEu),
            (VUINT32) (0xB1460308u),
            (VUINT32) (0x40000000u),
            (VUINT32) (0xBE8306C5u),
            (VUINT32) (0xBFC09232u),
            (VUINT32) (0x3FF09632u),
            (VUINT32) (0xC007DB00u)
        },
        {
            (VUINT32) (0xBF6231D6u),
            (VUINT32) (0x3119A6A2u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBF521801u),
            (VUINT32) (0xB2AE4178u),
            (VUINT32) (0x40000000u),
            (VUINT32) (0xBEA72938u),
            (VUINT32) (0xBFAFCC22u),
            (VUINT32) (0x3FD7BD4Au),
            (VUINT32) (0xBFEBB01Bu)
        },
        {
            (VUINT32) (0xBF687A55u),
            (VUINT32) (0x3252257Bu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBF47C8CCu),
            (VUINT32) (0x3200F51Au),
            (VUINT32) (0x40000000u),
            (VUINT32) (0xBEC82C6Cu),
            (VUINT32) (0xBFA0BAE9u),
            (VUINT32) (0x3FC2252Fu),
            (VUINT32) (0xBFCD24C7u)
        },
        {
            (VUINT32) (0xBF6EC2D4u),
            (VUINT32) (0x32BEF0A7u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBF3DDCCFu),
            (VUINT32) (0xB2D29606u),
            (VUINT32) (0x40000000u),
            (VUINT32) (0xBEE6606Fu),
            (VUINT32) (0xBF9325D6u),
            (VUINT32) (0x3FAF4E69u),
            (VUINT32) (0xBFB3080Cu)
        },
        {
            (VUINT32) (0xBF750B52u),
            (VUINT32) (0xB2EB316Fu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBF344BA9u),
            (VUINT32) (0x32B8B0EAu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3EFDF4F7u),
            (VUINT32) (0xBF86DCA8u),
            (VUINT32) (0x3F9ED53Bu),
            (VUINT32) (0xBF9CBEDEu)
        },
        {
            (VUINT32) (0xBF7B53D1u),
            (VUINT32) (0xB2955386u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBF2B0DC1u),
            (VUINT32) (0xB2AB7EBAu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3EE496C2u),
            (VUINT32) (0xBF776C40u),
            (VUINT32) (0x3F9065C1u),
            (VUINT32) (0xBF89AFB6u)
        },
        {
            (VUINT32) (0xBF80CE28u),
            (VUINT32) (0xB1FDD672u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBF221C37u),
            (VUINT32) (0x320C61DCu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3ECD4F71u),
            (VUINT32) (0xBF631DAAu),
            (VUINT32) (0x3F83B471u),
            (VUINT32) (0xBF7281EAu)
        },
        {
            (VUINT32) (0xBF83F267u),
            (VUINT32) (0xB374CBD9u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBF1970C4u),
            (VUINT32) (0xB2904848u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3EB7EFF8u),
            (VUINT32) (0xBF50907Cu),
            (VUINT32) (0x3F710FEAu),
            (VUINT32) (0xBF561FEDu)
        },
        {
            (VUINT32) (0xBF8716A7u),
            (VUINT32) (0x32588C6Du),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBF1105AFu),
            (VUINT32) (0xB2F045B0u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3EA44EE2u),
            (VUINT32) (0xBF3F8FDBu),
            (VUINT32) (0x3F5D3FD0u),
            (VUINT32) (0xBF3D0A23u)
        },
        {
            (VUINT32) (0xBF8A3AE6u),
            (VUINT32) (0xB31EEDF0u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBF08D5B9u),
            (VUINT32) (0x325EF98Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E92478Du),
            (VUINT32) (0xBF2FEDC9u),
            (VUINT32) (0x3F4BCD58u),
            (VUINT32) (0xBF27AE9Eu)
        },
        {
            (VUINT32) (0xBF8D5F26u),
            (VUINT32) (0x330C0105u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBF00DC0Du),
            (VUINT32) (0x3214AF72u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E81B994u),
            (VUINT32) (0xBF218233u),
            (VUINT32) (0x3F3C4531u),
            (VUINT32) (0xBF149688u)
        },
        {
            (VUINT32) (0xBF908365u),
            (VUINT32) (0xB292200Du),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBEF22870u),
            (VUINT32) (0xB25271F4u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E65107Au),
            (VUINT32) (0xBF1429F0u),
            (VUINT32) (0x3F2E8AFCu),
            (VUINT32) (0xBF040498u)
        },
        {
            (VUINT32) (0xBF93A7A5u),
            (VUINT32) (0x3361DEEEu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBEE2F439u),
            (VUINT32) (0x31F4399Eu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E49341Cu),
            (VUINT32) (0xBF07C61Au),
            (VUINT32) (0x3F22560Fu),
            (VUINT32) (0xBEEAA81Eu)
        },
        {
            (VUINT32) (0xBF96CBE4u),
            (VUINT32) (0x314CDE2Eu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBED413CDu),
            (VUINT32) (0x31C06152u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E2FB0CCu),
            (VUINT32) (0xBEF876CBu),
            (VUINT32) (0x3F177807u),
            (VUINT32) (0xBED08437u)
        },
        {
            (VUINT32) (0xBF99F023u),
            (VUINT32) (0xB3484328u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBEC5800Du),
            (VUINT32) (0x3214C3C1u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E185E54u),
            (VUINT32) (0xBEE2E342u),
            (VUINT32) (0x3F0DCA73u),
            (VUINT32) (0xBEB8CC21u)
        },
        {
            (VUINT32) (0xBF9D1463u),
            (VUINT32) (0x32C55799u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBEB73250u),
            (VUINT32) (0x32028823u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3E0318F8u),
            (VUINT32) (0xBECEA678u),
            (VUINT32) (0x3F053C67u),
            (VUINT32) (0xBEA41E53u)
        },
        {
            (VUINT32) (0xBFA038A2u),
            (VUINT32) (0xB2E4CA7Eu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBEA92457u),
            (VUINT32) (0xB0B80830u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3DDF8200u),
            (VUINT32) (0xBEBB99E9u),
            (VUINT32) (0x3EFB4AA8u),
            (VUINT32) (0xBE9182BEu)
        },
        {
            (VUINT32) (0xBFA35CE2u),
            (VUINT32) (0x333889B6u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBE9B5042u),
            (VUINT32) (0x322A3AEEu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3DBC7490u),
            (VUINT32) (0xBEA99AF5u),
            (VUINT32) (0x3EEDE107u),
            (VUINT32) (0xBE80E9AAu)
        },
        {
            (VUINT32) (0xBFA68121u),
            (VUINT32) (0xB1E43AACu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBE8DB082u),
            (VUINT32) (0x3132A234u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3D9CD7D0u),
            (VUINT32) (0xBE988A60u),
            (VUINT32) (0x3EE203E3u),
            (VUINT32) (0xBE63582Cu)
        },
        {
            (VUINT32) (0xBFA9A560u),
            (VUINT32) (0xB3719861u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBE803FD4u),
            (VUINT32) (0x32279E66u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3D807FC8u),
            (VUINT32) (0xBE884BD4u),
            (VUINT32) (0x3ED7812Du),
            (VUINT32) (0xBE4636EBu)
        },
        {
            (VUINT32) (0xBFACC9A0u),
            (VUINT32) (0x32655A50u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBE65F267u),
            (VUINT32) (0xB1B4B1DFu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3D4E8B90u),
            (VUINT32) (0xBE718ACAu),
            (VUINT32) (0x3ECE7164u),
            (VUINT32) (0xBE2DC161u)
        },
        {
            (VUINT32) (0xBFAFEDDFu),
            (VUINT32) (0xB31BBA77u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBE4BAFAFu),
            (VUINT32) (0xAF2A29E0u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3D221018u),
            (VUINT32) (0xBE53BED0u),
            (VUINT32) (0x3EC67E26u),
            (VUINT32) (0xBE1568E2u)
        },
        {
            (VUINT32) (0xBFB3121Fu),
            (VUINT32) (0x330F347Du),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBE31AE4Du),
            (VUINT32) (0x31F32251u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3CF6A500u),
            (VUINT32) (0xBE3707DAu),
            (VUINT32) (0x3EBFA489u),
            (VUINT32) (0xBDFBD9C7u)
        },
        {
            (VUINT32) (0xBFB6365Eu),
            (VUINT32) (0xB28BB91Cu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBE17E564u),
            (VUINT32) (0x31C5A2E4u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3CB440D0u),
            (VUINT32) (0xBE1B3D00u),
            (VUINT32) (0x3EB9F664u),
            (VUINT32) (0xBDD647C0u)
        },
        {
            (VUINT32) (0xBFB95A9Eu),
            (VUINT32) (0x33651267u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBDFC98C2u),
            (VUINT32) (0x30AE525Cu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3C793D20u),
            (VUINT32) (0xBE003845u),
            (VUINT32) (0x3EB5271Fu),
            (VUINT32) (0xBDAC669Eu)
        },
        {
            (VUINT32) (0xBFBC7EDDu),
            (VUINT32) (0x31800ADDu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBDC9B5DCu),
            (VUINT32) (0xB145AD86u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3C1EEF20u),
            (VUINT32) (0xBDCBAAEAu),
            (VUINT32) (0x3EB14E5Eu),
            (VUINT32) (0xBD858BB2u)
        },
        {
            (VUINT32) (0xBFBFA31Cu),
            (VUINT32) (0xB3450FB0u),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBD9711CEu),
            (VUINT32) (0xB14FEB28u),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3BB24C00u),
            (VUINT32) (0xBD97E43Au),
            (VUINT32) (0x3EAE6A89u),
            (VUINT32) (0xBD4D07E0u)
        },

        {
            (VUINT32) (0xBFC2C75Cu),
            (VUINT32) (0x32CBBE8Au),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBD49393Cu),
            (VUINT32) (0xB0A39F5Bu),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3B1E2B00u),
            (VUINT32) (0xBD49B5D4u),
            (VUINT32) (0x3EAC4F10u),
            (VUINT32) (0xBCFD9425u)
        },

        {
            (VUINT32) (0xBFC5EB9Bu),
            (VUINT32) (0xB2DE638Cu),
            (VUINT32) (0x00000000u),
            (VUINT32) (0xBCC91A31u),
            (VUINT32) (0xAF8E8D1Au),
            (VUINT32) (0x3F800000u),
            (VUINT32) (0x3A1DFA00u),
            (VUINT32) (0xBCC9392Du),
            (VUINT32) (0x3EAB1889u),
            (VUINT32) (0xBC885D3Bu)
        },
    },
    0x7FFFFFFFu,
};

#endif // __SVML_TANPI_DATA_CL__
