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
#include "IGAToGEDTranslation.hpp"
#include "GEDToIGATranslation.hpp"
#include "GEDUtil.hpp"

#include "../../bits.hpp"

using namespace iga;


iga::SFMessageType iga::getMessageType(Platform p, SFID sfid, uint32_t desc)
{
    GED_SFID gedSFID = lowerSFID(sfid);

    GED_RETURN_VALUE getRetVal = GED_RETURN_VALUE_INVALID_FIELD;
    GED_MESSAGE_TYPE msgType = GED_MESSAGE_TYPE_INVALID;
    GED_MODEL gedP = lowerPlatform(p);

    if (gedSFID != GED_SFID_INVALID && gedSFID != GED_SFID_NULL) {
        switch (gedSFID)
        {
        case GED_SFID_SAMPLER:    ///< all
            msgType = GED_GetMessageTypeDP_SAMPLER(desc, gedP, &getRetVal);
            break;
        case GED_SFID_GATEWAY:    ///< all
            break;
            /// includes: GEN11
        case GED_SFID_DP_DC2:     ///< GEN10, GEN9
            msgType = GED_GetMessageTypeDP_DC2(desc, gedP, &getRetVal);
            break;
        case GED_SFID_DP_RC:      ///< all
            msgType = GED_GetMessageTypeDP_RC(desc, gedP, &getRetVal);
            break;
        case GED_SFID_URB:        ///< all
            break;
        case GED_SFID_SPAWNER:    ///< all
            break;
        case GED_SFID_VME:        ///< all
            break;
            /// includes: GEN11
        case GED_SFID_DP_DCRO:    ///< GEN10, GEN9
            msgType = GED_GetMessageTypeDP_DCRO(desc, gedP, &getRetVal);
            break;
        case GED_SFID_DP_DC0:     ///< all
            if (p <= iga::Platform::GEN7P5) {
                //IVB,HSW
                msgType = GED_GetMessageTypeDP_DC0(desc, gedP, &getRetVal);
            }
            else {
                //Starting with BDW
                if (GED_GetMessageTypeDP0Category(desc, gedP, &getRetVal) == 0) {
                    msgType = GED_GetMessageTypeDP_DC0Legacy(desc, gedP, &getRetVal);
                }
                else {
                    msgType = GED_GetMessageTypeDP_DC0ScratchBlock(desc, gedP, &getRetVal);
                }
            }
            break;
        case GED_SFID_PI:         ///< all
            break;
            /// includes: GEN11
        case GED_SFID_DP_DC1:     ///< GEN10, GEN7.5, GEN8, GEN8.1, GEN9
            msgType = GED_GetMessageTypeDP_DC1(desc, gedP, &getRetVal);
            break;
            /// includes: GEN11
        case GED_SFID_CRE:        ///< GEN10, GEN7.5, GEN8, GEN8.1, GEN9
            break;
        case GED_SFID_DP_SAMPLER: ///< GEN7, GEN7.5, GEN8, GEN8.1
            msgType = GED_GetMessageTypeDP_SAMPLER(desc, gedP, &getRetVal);
            break;
        case GED_SFID_DP_CC:      ///< GEN7, GEN7.5, GEN8, GEN8.1
            msgType = GED_GetMessageTypeDP_CC(desc, gedP, &getRetVal);
            break;
        default:
            break;
        }
    }


    if (msgType == GED_MESSAGE_TYPE_INVALID ||
        getRetVal != GED_RETURN_VALUE_SUCCESS)
    {
        return SFMessageType::INVALID;
    } else {
        return translate(msgType);
    }

}
