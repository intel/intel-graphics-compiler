/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_IGCDEPS_TRANSLATIONINTERFACE_H
#define VC_IGCDEPS_TRANSLATIONINTERFACE_H

#include <AdaptorOCL/OCL/TB/igc_tb.h>

#include <system_error>

namespace vc {

std::error_code translateBuild(const TC::STB_TranslateInputArgs *InputArgs,
                               TC::STB_TranslateOutputArgs *OutputArgs,
                               TC::TB_DATA_FORMAT InputDataFormatTemp,
                               const IGC::CPlatform &IGCPlatform,
                               float ProfilingTimerResolution);

} // namespace vc

#endif
