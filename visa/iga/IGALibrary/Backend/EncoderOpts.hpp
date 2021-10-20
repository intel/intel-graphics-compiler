/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_BACKEND_ENCODEROPTS
#define IGA_BACKEND_ENCODEROPTS

#include "../api/iga_types_swsb.hpp"

namespace iga
{
    struct EncoderOpts {
        bool autoCompact = false;
        bool explicitCompactMissIsWarning = false;
        bool ignoreNoCompactFormFound = false;
        bool autoDepSet = false;
        bool forceNoCompact = false;
        // Specify the swsb encoding mode. If not specified, the encoding mode will
        // be derived from platform by SWSB::getEncodeMode
        SWSB_ENCODE_MODE swsbEncodeMode = SWSB_ENCODE_MODE::SWSBInvalidMode;
        // Specify number of sbid that can be used
        uint32_t sbidCount = 16;

        EncoderOpts(
            bool _autoCompact = false,
            bool _explicitCompactMissIsWarning = false,
            bool _forceNoCompact = false)
        : autoCompact(_autoCompact),
          explicitCompactMissIsWarning(_explicitCompactMissIsWarning),
          forceNoCompact(_forceNoCompact)
        { }
    };
}

#endif // IGA_BACKEND_ENCODEROPTS
