
/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __DEBUGTOOLS_H__
#define __DEBUGTOOLS_H__

namespace vISA {

void dumpLiveRanges(GlobalRA &gra, AllIntervals &sortedIntervals);

} // namespace vISA

#endif // __DEBUGTOOLS_H__