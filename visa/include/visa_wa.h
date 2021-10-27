/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/**
** File Name     : visa_wa.h
**
** Abstract      : Declaration of all SW workarounds implemented in vISA finalizer.
**
**  ---------------------------------------------------------------------- */

#ifndef _VISA_WA_H_
#define _VISA_WA_H_

#define VISA_WA_DECLARE( wa, wa_comment, wa_bugType ) unsigned int wa : 1;
#define VISA_WA_INIT( wa ) wa = 0;
#define VISA_WA_ENABLE( pWaTable, wa )    \
{                                         \
    pWaTable->wa = true;                  \
}
#define VISA_WA_DISABLE( pWaTable, wa )    \
{                                         \
    pWaTable->wa = false;                  \
}
#define VISA_WA_CHECK(pWaTable, w)  ((pWaTable)->w)

enum VISA_WA_BUG_TYPE
{
    VISA_WA_BUG_TYPE_UNKNOWN    = 0,
    VISA_WA_BUG_TYPE_CORRUPTION = 1,
    VISA_WA_BUG_TYPE_HANG       = 2,
    VISA_WA_BUG_TYPE_PERF       = 4,
    VISA_WA_BUG_TYPE_FUNCTIONAL = 8,
    VISA_WA_BUG_TYPE_SPEC       = 16,
    VISA_WA_BUG_TYPE_FAIL       = 32
};

enum class VISA_BUILD_TYPE {
    KERNEL = 0,
    FUNCTION = 1,
    PAYLOAD = 2
};

enum CODE_PATCH_TYPE {
    CodePatch_Disabled = 0,
    CodePatch_Payload_Prologue = 1,
    CodePatch_Enable_NoLTO = 2,
    CodePatch_Enable
};

enum LINKER_TYPE {
    Linker_Subroutine = 0,       // VALUE 1
    Linker_Call2Jump = 1,        // VALUE 2
    Linker_Inline = 2,           // VALUE 4
    Linker_RemoveStackFrame = 3, // VALUE 8
    Linker_RemoveArgRet = 4,     // VALUE 16
    Linker_RemoveStackArg = 5    // VALUE 32
};

#endif
