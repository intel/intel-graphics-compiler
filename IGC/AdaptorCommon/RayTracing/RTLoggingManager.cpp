/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "RTLoggingManager.h"

using namespace llvm;

namespace IGC {

RTLoggingManager::RTLoggingManager(const CDriverInfo &DriverInfo) :
    Enabled(DriverInfo.SupportsRTPrintf() && IGC_IS_FLAG_ENABLED(EnableRTPrintf))
{
    if (!isEnabled())
        return;

#if defined(IGC_DEBUG_VARIABLES)
    addToTable("DispatchRayIndex: (%d,%d,%d)", DISPATCH_RAY_INDEX);
    addToTable("RayDesc: Origin(%f,%f,%f) Direction(%f,%f,%f), Tmin(%f), Tmax(%f)", TRACE_RAY);
    addToTable("ForwardRayDesc: Origin(%f,%f,%f) Direction(%f,%f,%f), Tmin(%f), Tmax(%f)", FORWARD_RAY);
#endif
}

bool RTLoggingManager::isEnabled() const
{
#if defined(IGC_DEBUG_VARIABLES)
    return Enabled;
#else
    return false;
#endif
}

Optional<uint32_t> RTLoggingManager::getIndex(StringRef FormatString) const {
    auto I = FormatToIndex.find(FormatString);
    if (I == FormatToIndex.end())
        return llvm::None;

    return I->second;
}

void RTLoggingManager::addToTable(StringRef FormatString, FormatIndex Index)
{
    IGC_ASSERT(Index < IndexToFormat.size());

    FormatToIndex[FormatString] = Index;
    IndexToFormat[Index] = FormatString;
}

} // namespace IGC
