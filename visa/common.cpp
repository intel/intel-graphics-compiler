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

#include "visa_igc_common_header.h"
#include "common.h"
#include "G4_Opcode.h"
#include <cctype>

//for exception handling
//FIXME: potentially not thread safe, but should be ok since it's debugging code
std::stringstream errorMsgs;

static _THREAD TARGET_PLATFORM visaPlatform;

// This is initialized in createBuilder and set once in SetStepping
static _THREAD Stepping stepping;

// We do not have a string for NONE
const char* platformString[ALL] =
{
    "BDW",
    "CHV",
    "SKL",
    "BXT",
    "CNL",
    "ICL",
    "ICLLP",
};

#define CM_SUCCESS                                  0
#define CM_FAILURE                                  -1

int SetPlatform( const char * str ) {

    std::string platform(str);

    int retVal = CM_FAILURE;
    if (platform == "BDW" || platform == "gen8")
    {
        visaPlatform = GENX_BDW;
        retVal = CM_SUCCESS;
    }
    else if (platform == "CHV" || platform == "gen8lp")
    {
        visaPlatform = GENX_CHV;
        retVal = CM_SUCCESS;
    }
    else if (platform == "SKL" || platform == "gen9")
    {
        visaPlatform = GENX_SKL;
        retVal = CM_SUCCESS;
    }
    else if (platform == "BXT" || platform == "gen9lp")
    {
        visaPlatform = GENX_BXT;
        retVal = CM_SUCCESS;
    }
    else if (platform == "CNL" || platform == "gen10")
    {
        visaPlatform = GENX_CNL;
        retVal = CM_SUCCESS;
    }
    else if (platform == "ICL" || platform == "gen11")
    {
        visaPlatform = GENX_ICL;
        retVal = CM_SUCCESS;
    }
    else if (platform == "ICLLP" || platform == "gen11lp")
    {
        visaPlatform = GENX_ICLLP;
        retVal = CM_SUCCESS;
    }

    return retVal;
}

// same as previous version, except that we already have the enum value
int SetVisaPlatform( TARGET_PLATFORM vPlatform ) {

    assert(vPlatform >= GENX_BDW && "unsupported platform");
    visaPlatform = vPlatform;

    return CM_SUCCESS;
}

TARGET_PLATFORM getGenxPlatform( void )
{
    return visaPlatform;
}

PlatformGen getPlatformGeneration(TARGET_PLATFORM platform)
{
    switch (platform)
    {
    case GENX_BDW:
    case GENX_CHV:
        return PlatformGen::GEN8;
    case GENX_SKL:
    case GENX_BXT:
        return PlatformGen::GEN9;
    case GENX_CNL:
        return PlatformGen::GEN10;
    case GENX_ICL:
    case GENX_ICLLP:
        return PlatformGen::GEN11;
    default:
        assert(false && "unsupported platform");
        return PlatformGen::GEN_UNKNOWN;
    }
}

unsigned char getGRFSize()
{
    unsigned int size = 32;


    return size;
}

// The encoding of gen platform defined in vISA spec:
// 3 BDW
// 4 CHV
// 5 SKL
// 6 BXT
// 7 CNL
// 8 ICL
// 10 ICLLP
// Note that encoding is not linearized.
int getGenxPlatformEncoding()
{
    switch (getGenxPlatform())
    {
    case GENX_BDW:
        return 3;
    case GENX_CHV:
        return 4;
    case GENX_SKL:
        return 5;
    case GENX_BXT:
        return 6;
    case GENX_CNL:
        return 7;
    case GENX_ICL:
        return 8;
    case GENX_ICLLP:
        return 10;
    default:
        assert(false && "unsupported platform");
        return -1;
    }
}

void InitStepping()
{
    stepping = Step_none;
}

int SetStepping( const char * str ) {

    int retVal = CM_SUCCESS;
    char upperchar = (char)std::toupper(*str);

    switch( upperchar )
    {
    case 'A':
        stepping = Step_A;
        break;
    case 'B':
        stepping = Step_B;
        break;
    case 'C':
        stepping = Step_C;
        break;
    case 'D':
        stepping = Step_D;
        break;
    case 'E':
        stepping = Step_E;
        break;
    case 'F':
        stepping = Step_F;
        break;
    default:
        // err msg?
        break;
    }
    return retVal;
}

Stepping GetStepping( void )
{
    return stepping;
}

const char * GetSteppingString(void)
{
    static const char* steppingName[Step_none + 1] =
    {
        "A",
        "B",
        "C",
        "D",
        "E",
        "F",
        "none"
    };

    return steppingName[stepping];
}
G4_Type_Info G4_Type_Table[Type_UNDEF+1] = {
    {Type_UD, 32, 4, 0xF, "ud"},
    {Type_D, 32, 4, 0xF, "d"},
    {Type_UW, 16, 2, 0x3, "uw"},
    {Type_W, 16, 2, 0x3, "w"},
    {Type_UB, 8, 1, 0x1, "ub"},
    {Type_B, 8, 1, 0x1, "b"},
    {Type_F, 32, 4, 0xF, "f"},
    {Type_VF, 32, 4, 0xF, "vf"}, //handle as F?
    {Type_V, 32, 4, 0xF, "v"},  //handle as D?
    {Type_DF, 64, 8, 0xFF, "df"},
    {Type_BOOL, 1, 2, 0x1, "bool"}, // TODO: how to decide 1 bit here?
    {Type_UV, 32, 4, 0xF, "uv"},
    {Type_Q, 64, 8, 0xFF, "q"},
    {Type_UQ, 64, 8, 0xFF, "uq"},
    {Type_HF, 16, 2, 0x3, "hf"},
    {Type_NF, 64, 8, 0xFF, "nf"},
    {Type_UNDEF, 0, 0, 0x0, "none"}
};

