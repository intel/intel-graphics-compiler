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
/**
 * create IGC variants form PassSupport macro, so we need llvm copyrigh header
 **/
 //===- llvm/PassSupport.h - Pass Support code -------------------*- C++ -*-===//
 //
 //                     The LLVM Compiler Infrastructure
 //
 // This file is distributed under the University of Illinois Open Source
 // License. See LICENSE.TXT for details.
 //
 //===----------------------------------------------------------------------===//
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
