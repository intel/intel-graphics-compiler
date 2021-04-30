/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

// creates IGC variants form llvm PassSupport macro

#pragma once

#include "Compiler/InitializePasses.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/InitializePasses.h"
#include <llvm/Pass.h>
#include "llvm/Support/Threading.h"
#include "common/LLVMWarningsPop.hpp"
#include <functional>


#define IGC_INITIALIZE_PASS(passName, arg, name, cfg, analysis) \
  static void* initialize##passName##PassOnce(PassRegistry &Registry) { \
    PassInfo *PI = new PassInfo(name, arg, & passName ::ID, \
      PassInfo::NormalCtor_t(callDefaultCtor< passName >), cfg, analysis); \
    Registry.registerPass(*PI, true); \
    return PI; \
  } \
  static llvm::once_flag Initialize##passName##PassFlag;\
  void initialize##passName##Pass(PassRegistry &Registry) { \
    llvm::call_once(Initialize##passName##PassFlag,                            \
                    initialize##passName##PassOnce, std::ref(Registry));       \
  }

#define IGC_INITIALIZE_PASS_BEGIN(passName, arg, name, cfg, analysis) INITIALIZE_PASS_BEGIN(passName, arg, name, cfg, analysis)

#define IGC_INITIALIZE_PASS_DEPENDENCY(depName) INITIALIZE_PASS_DEPENDENCY(depName)

#define IGC_INITIALIZE_AG_DEPENDENCY(depName) INITIALIZE_AG_DEPENDENCY(depName)

#define IGC_INITIALIZE_PASS_END(passName, arg, name, cfg, analysis) \
    PassInfo *PI = new PassInfo(name, arg, & passName ::ID, \
      PassInfo::NormalCtor_t(callDefaultCtor< passName >), cfg, analysis); \
    Registry.registerPass(*PI, true); \
    return PI; \
  } \
  static llvm::once_flag Initialize##passName##PassFlag;\
  void initialize##passName##Pass(PassRegistry &Registry) { \
    llvm::call_once(Initialize##passName##PassFlag,                            \
                    initialize##passName##PassOnce, std::ref(Registry));       \
  }

#define IGC_INITIALIZE_AG_PASS(passName, agName, arg, name, cfg, analysis, def) \
  static void* initialize##passName##PassOnce(PassRegistry &Registry) { \
    if (!def) initialize##agName##AnalysisGroup(Registry); \
    PassInfo *PI = new PassInfo(name, arg, & passName ::ID, \
      PassInfo::NormalCtor_t(callDefaultCtor< passName >), cfg, analysis); \
    Registry.registerPass(*PI, true); \
    \
    PassInfo *AI = new PassInfo(name, & agName :: ID); \
    Registry.registerAnalysisGroup(& agName ::ID, & passName ::ID, \
                                   *AI, def, true); \
    return AI; \
  } \
  static llvm::once_flag Initialize##passName##PassFlag;\
  void initialize##passName##Pass(PassRegistry &Registry) { \
    llvm::call_once(Initialize##passName##PassFlag,                            \
                    initialize##passName##PassOnce, std::ref(Registry));       \
  }
