/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


#pragma once

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "common/LLVMWarningsPop.hpp"
#include <optional>

namespace IGC {
class CComputeShaderBase : public CShader {
public:
  CComputeShaderBase(llvm::Function *pFunc, CShaderProgram *pProgram, GenericShaderState &GState);
  virtual ~CComputeShaderBase();

  CComputeShaderBase(const CComputeShaderBase &) = delete;
  CComputeShaderBase &operator=(const CComputeShaderBase &) = delete;

protected:
  // Determines if HW can handle auto generating local IDs with this
  // order
  static std::optional<CS_WALK_ORDER> checkLegalWalkOrder(const std::array<uint32_t, 3> &Dims,
                                                          const WorkGroupWalkOrderMD &WO);
};
} // namespace IGC
