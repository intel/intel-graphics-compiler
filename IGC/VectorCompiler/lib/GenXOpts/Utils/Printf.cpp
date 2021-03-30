/*========================== begin_copyright_notice ============================

Copyright (c) 2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "vc/GenXOpts/Utils/Printf.h"

#include <llvm/ADT/Optional.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/Casting.h>

#include <algorithm>
#include <iterator>
#include <regex>
#include <string>

#include "Probe/Assertion.h"

using namespace llvm;

// extracts underlying c-string from provided constant
static StringRef extractCStr(const Constant &CStrConst) {
  if (isa<ConstantDataArray>(CStrConst))
    return cast<ConstantDataArray>(CStrConst).getAsCString();
  IGC_ASSERT(isa<ConstantAggregateZero>(CStrConst));
  return "";
}

Optional<StringRef> llvm::getConstStringFromOperandOptional(const Value &Op) {
  IGC_ASSERT_MESSAGE(Op.getType()->isPointerTy(),
                     "wrong argument: pointer was expected");
  IGC_ASSERT_MESSAGE(Op.getType()->getPointerElementType()->isIntegerTy(8),
                     "wrong argument: i8* value was expected");
  if (!isa<GEPOperator>(Op))
    return {};
  auto *StrConst = cast<GEPOperator>(Op).getPointerOperand();
  if (!isa<GlobalVariable>(StrConst))
    return {};
  return extractCStr(*cast<GlobalVariable>(StrConst)->getInitializer());
}

StringRef llvm::getConstStringFromOperand(const Value &Op) {
  auto FmtStr = getConstStringFromOperandOptional(Op);
  IGC_ASSERT_MESSAGE(FmtStr.hasValue(),
                     "couldn't reach constexpr string through pointer operand");
  return FmtStr.getValue();
}

using SRefMatch = std::match_results<StringRef::iterator>;
using SRefRegExIterator = std::regex_iterator<StringRef::iterator>;

// Given that \p ArgDesc describes integer conversion with signedness equal to
// \p IsSigned, defines which particular integer type is provided.
static PrintfArgInfo parseIntLengthModifier(StringRef ArgDesc, bool IsSigned) {
  std::string Suffix{1u, ArgDesc.back()};
  if (ArgDesc.endswith("hh" + Suffix))
    return {PrintfArgInfo::Char, IsSigned};
  if (ArgDesc.endswith("h" + Suffix))
    return {PrintfArgInfo::Short, IsSigned};
  if (ArgDesc.endswith("ll" + Suffix))
    // TOTHINK: maybe we need a separate type ID for long long.
    return {PrintfArgInfo::Long, IsSigned};
  if (ArgDesc.endswith("l" + Suffix))
    return {PrintfArgInfo::Long, IsSigned};
  return {PrintfArgInfo::Int, IsSigned};
}

static StringRef toStringRef(SRefMatch Match) {
  IGC_ASSERT_MESSAGE(!Match.empty(),
                     "wrong argument: matched string is expected");
  return {Match[0].first, static_cast<std::size_t>(Match[0].length())};
}

// \p ArgDescMatch is a format string conversion specifier matched by a regex
// (some string that starts with % and ends with d,i,f,...).
static PrintfArgInfo parseArgDesc(SRefMatch ArgDescMatch) {
  StringRef ArgDesc = toStringRef(ArgDescMatch);
  if (ArgDesc.endswith("c"))
    // FIXME: support %lc
    return {PrintfArgInfo::Int, /* IsSigned */ true};
  if (ArgDesc.endswith("s"))
    // FIXME: support %ls
    return {PrintfArgInfo::String, /* IsSigned */ false};
  if (ArgDesc.endswith("d") || ArgDesc.endswith("i"))
    return parseIntLengthModifier(ArgDesc, /* IsSigned */ true);
  if (ArgDesc.endswith("o") || ArgDesc.endswith("u") ||
      ArgDesc.endswith_lower("x"))
    return parseIntLengthModifier(ArgDesc, /* IsSigned */ false);
  if (ArgDesc.endswith_lower("f") || ArgDesc.endswith_lower("e") ||
      ArgDesc.endswith_lower("a") || ArgDesc.endswith_lower("g"))
    return {PrintfArgInfo::Double, /* IsSigned */ true};
  IGC_ASSERT_MESSAGE(ArgDesc.endswith("p"), "unexpected conversion specifier");
  return {PrintfArgInfo::Pointer, /* IsSigned */ false};
}

PrintfArgInfoSeq llvm::parseFormatString(StringRef FmtStr) {
  PrintfArgInfoSeq Args;
  std::regex ArgDescRegEx{"%(?:%|.*?[csdioxXufFeEaAgGp])"};
  auto &&ArgDescs = make_filter_range(
      make_range(SRefRegExIterator{FmtStr.begin(), FmtStr.end(), ArgDescRegEx},
                 SRefRegExIterator{}),
      [](SRefMatch ArgDesc) { return toStringRef(ArgDesc) != "%%"; });
  transform(ArgDescs, std::back_inserter(Args),
            [](SRefMatch ArgDesc) { return parseArgDesc(ArgDesc); });
  return Args;
}
