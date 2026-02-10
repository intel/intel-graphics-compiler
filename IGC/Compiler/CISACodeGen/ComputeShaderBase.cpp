/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "common/LLVMWarningsPop.hpp"
#include <optional>
#include "Compiler/CISACodeGen/ComputeShaderBase.hpp"
#include "Compiler/CISACodeGen/CSWalkOrder.hpp"
#include <iStdLib/utility.h>

using namespace llvm;

namespace IGC {
CComputeShaderBase::CComputeShaderBase(Function *pFunc, CShaderProgram *pProgram, GenericShaderState &GState)
    : CShader(pFunc, pProgram, GState) {}

CComputeShaderBase::~CComputeShaderBase() {}

std::optional<CS_WALK_ORDER> CComputeShaderBase::checkLegalWalkOrder(const std::array<uint32_t, 3> &Dims,
                                                                     const WorkGroupWalkOrderMD &WO) {
  auto is_pow2 = [](uint32_t dim) { return iSTD::IsPowerOfTwo(dim); };

  const int walkorder_x = WO.dim0;
  const int walkorder_y = WO.dim1;
  [[maybe_unused]] const int walkorder_z = WO.dim2;

  const uint32_t dim_x = Dims[0];
  const uint32_t dim_y = Dims[1];
  const uint32_t dim_z = Dims[2];

  uint order0 = (walkorder_x == 0) ? 0 : (walkorder_y == 0) ? 1 : 2;
  uint order1 = (walkorder_x == 1) ? 0 : (walkorder_y == 1) ? 1 : 2;

  if (order0 != order1 &&
      ((order0 == 0 && is_pow2(dim_x)) || (order0 == 1 && is_pow2(dim_y)) || (order0 == 2 && is_pow2(dim_z))) &&
      ((order1 == 0 && is_pow2(dim_x)) || (order1 == 1 && is_pow2(dim_y)) || (order1 == 2 && is_pow2(dim_z)))) {
    // Legal walk order for HW auto-gen
    return getWalkOrderInPass(order0, order1);
  }

  return std::nullopt;
}
} // namespace IGC
