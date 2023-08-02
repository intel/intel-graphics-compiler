/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "GenIntrinsicEnum.h"

namespace IGC
{

template<llvm::GenISAIntrinsic::ID id>
struct IntrinsicDescription;

<%!
from Intrinsic_generator import IntrinsicFormatter
%>\
% for el in intrinsic_definitions:
template<>
class IntrinsicDescription<llvm::GenISAIntrinsic::ID::${el.name}>
{
public:
    static constexpr llvm::GenISAIntrinsic::ID scID = llvm::GenISAIntrinsic::ID::${el.name};

    enum class Argument : uint32_t
    {
    % if len(el.argument_types) > 0:
        % for arg in el.argument_types:
        ${IntrinsicFormatter.get_argument_name(arg, loop.index)}
        % endfor
        Count
    % else:
        Count = 0
    % endif
    };
};

% endfor
} // namespace IGC