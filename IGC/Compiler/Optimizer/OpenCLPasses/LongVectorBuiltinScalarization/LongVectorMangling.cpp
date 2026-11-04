/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "LongVectorMangling.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringExtras.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

#include <algorithm>

using namespace llvm;

namespace IGC {
namespace LongVectorMangling {

std::string MangledType::canonical() const {
  switch (K) {
  case Kind::Builtin:
    return Text;
  case Kind::Named:
    return std::to_string(Text.size()) + Text;
  case Kind::Vector:
    return "Dv" + std::to_string(NumElements) + "_" + Inner->canonical();
  case Kind::Pointer:
  case Kind::Qualified:
    return Text + Inner->canonical();
  }
  IGC_ASSERT_UNREACHABLE();
}

namespace {

MangledTypeRef makeType(MangledType::Kind K, std::string Text, MangledTypeRef Inner = nullptr,
                        unsigned NumElements = 0) {
  auto T = std::make_shared<MangledType>();
  T->K = K;
  T->Text = std::move(Text);
  T->Inner = std::move(Inner);
  T->NumElements = NumElements;
  return T;
}

class Parser {
public:
  explicit Parser(StringRef S) : S(S) {}

  std::optional<MangledFunction> parse() {
    if (!S.consume_front("_Z"))
      return std::nullopt;
    MangledFunction F;
    if (!parseSourceName(F.Name))
      return std::nullopt;
    while (!S.empty()) {
      MangledTypeRef T = parseType();
      if (!T)
        return std::nullopt;
      F.Params.push_back(std::move(T));
    }
    // A lone `v` means an empty parameter list.
    if (F.Params.size() == 1 && F.Params[0]->K == MangledType::Kind::Builtin && F.Params[0]->Text == "v")
      F.Params.clear();
    return F;
  }

private:
  StringRef S;
  SmallVector<MangledTypeRef, 8> Subs;

  bool parseNumber(unsigned &N) {
    size_t Len = 0;
    while (Len < S.size() && isDigit(S[Len]))
      ++Len;
    if (Len == 0 || S.take_front(Len).getAsInteger(10, N))
      return false;
    S = S.drop_front(Len);
    return true;
  }

  bool parseSourceName(std::string &Name) {
    unsigned Len;
    if (!parseNumber(Len) || Len == 0 || Len > S.size())
      return false;
    Name = S.take_front(Len).str();
    S = S.drop_front(Len);
    return true;
  }

  MangledTypeRef parseSubstitution() {
    // S_ is the first candidate, S<base-36 seq>_ is candidate seq + 1.
    size_t Idx = 0;
    if (!S.consume_front("_")) {
      size_t Seq = 0;
      size_t Len = 0;
      while (Len < S.size() && (isDigit(S[Len]) || (S[Len] >= 'A' && S[Len] <= 'Z'))) {
        Seq = Seq * 36 + (isDigit(S[Len]) ? S[Len] - '0' : S[Len] - 'A' + 10);
        ++Len;
      }
      if (Len == 0 || Len >= S.size() || S[Len] != '_')
        return nullptr;
      S = S.drop_front(Len + 1);
      Idx = Seq + 1;
    }
    return Idx < Subs.size() ? Subs[Idx] : nullptr;
  }

  MangledTypeRef addSubstitution(MangledTypeRef T) {
    Subs.push_back(T);
    return T;
  }

  MangledTypeRef parseType() {
    if (S.empty())
      return nullptr;

    if (S.consume_front("S"))
      return parseSubstitution();

    std::string Quals;
    while (!S.empty() && S[0] == 'U') {
      S = S.drop_front();
      std::string Vendor;
      if (!parseSourceName(Vendor))
        return nullptr;
      Quals += "U" + std::to_string(Vendor.size()) + Vendor;
    }
    while (!S.empty() && (S[0] == 'r' || S[0] == 'V' || S[0] == 'K')) {
      Quals += S[0];
      S = S.drop_front();
    }
    if (!Quals.empty()) {
      MangledTypeRef Inner = parseType();
      if (!Inner)
        return nullptr;
      return addSubstitution(makeType(MangledType::Kind::Qualified, Quals, Inner));
    }

    if (S.consume_front("P")) {
      MangledTypeRef Inner = parseType();
      if (!Inner)
        return nullptr;
      return addSubstitution(makeType(MangledType::Kind::Pointer, "P", Inner));
    }

    if (S.consume_front("Dv")) {
      unsigned N;
      if (!parseNumber(N) || !S.consume_front("_"))
        return nullptr;
      MangledTypeRef Elem = parseType();
      if (!Elem)
        return nullptr;
      return addSubstitution(makeType(MangledType::Kind::Vector, "", Elem, N));
    }

    if (isDigit(S[0])) {
      std::string Name;
      if (!parseSourceName(Name))
        return nullptr;
      return addSubstitution(makeType(MangledType::Kind::Named, Name));
    }

    if (S.consume_front("DF16b"))
      return makeType(MangledType::Kind::Builtin, "DF16b");
    for (StringRef Code : {"Dh", "Da", "Dc", "Dn", "Ds", "Di", "Dd", "Df", "De"})
      if (S.consume_front(Code))
        return makeType(MangledType::Kind::Builtin, Code.str());
    if (StringRef("vwbcahstijlmxynofdegz").contains(S[0])) {
      std::string Code(1, S[0]);
      S = S.drop_front();
      return makeType(MangledType::Kind::Builtin, Code);
    }
    return nullptr;
  }
};

class Emitter {
public:
  std::string emit(const MangledType &T) {
    if (T.K == MangledType::Kind::Builtin)
      return T.Text;

    std::string Canon = T.canonical();
    auto It = std::find(Subs.begin(), Subs.end(), Canon);
    if (It != Subs.end())
      return substitution(It - Subs.begin());

    std::string Out;
    switch (T.K) {
    case MangledType::Kind::Named:
      Out = Canon;
      break;
    case MangledType::Kind::Vector:
      Out = "Dv" + std::to_string(T.NumElements) + "_" + emit(*T.Inner);
      break;
    case MangledType::Kind::Pointer:
    case MangledType::Kind::Qualified:
      Out = T.Text + emit(*T.Inner);
      break;
    case MangledType::Kind::Builtin:
      IGC_ASSERT_UNREACHABLE();
    }
    Subs.push_back(std::move(Canon));
    return Out;
  }

private:
  SmallVector<std::string, 8> Subs;

  static std::string substitution(size_t Idx) {
    if (Idx == 0)
      return "S_";
    std::string Seq;
    for (size_t N = Idx - 1;; N /= 36) {
      size_t D = N % 36;
      Seq.insert(Seq.begin(), char(D < 10 ? '0' + D : 'A' + D - 10));
      if (N < 36)
        break;
    }
    return "S" + Seq + "_";
  }
};

MangledTypeRef scalarizeType(const MangledTypeRef &T, unsigned NumElements) {
  switch (T->K) {
  case MangledType::Kind::Vector:
    return T->NumElements == NumElements ? T->Inner : T;
  case MangledType::Kind::Pointer:
  case MangledType::Kind::Qualified: {
    MangledTypeRef Inner = scalarizeType(T->Inner, NumElements);
    return Inner == T->Inner ? T : makeType(T->K, T->Text, Inner);
  }
  case MangledType::Kind::Builtin:
  case MangledType::Kind::Named:
    return T;
  }
  IGC_ASSERT_UNREACHABLE();
}

// {ScalarType} of the `_R{ReturnType}` postfix in SPIRVRepresentationInLLVM.rst.
bool isReturnTypeName(StringRef Name) {
  static const StringRef Types[] = {"char", "uchar", "short", "ushort", "int",    "uint",
                                    "long", "ulong", "half",  "float",  "double", "bool"};
  return is_contained(Types, Name);
}

// Position of the `_R<type><N>` return type suffix SPIR-V friendly IR appends
// to some builtin names. Only an OpenCL type name counts as a return type, so
// `__spirv_RoundFToTF32INTEL` has none. bfloat has no OpenCL type name, and the
// translator writes its Itanium mangling `DF16b` instead.
struct ReturnSuffix {
  size_t Begin;
  size_t TypeEnd;
  size_t WidthEnd;
};

std::optional<ReturnSuffix> findReturnSuffix(StringRef Name) {
  size_t Begin = Name.rfind("_R");
  if (Begin == StringRef::npos)
    return std::nullopt;
  size_t TypeEnd = Begin + 2;
  if (Name.slice(TypeEnd, TypeEnd + 5) == "DF16b") {
    TypeEnd += 5;
  } else {
    while (TypeEnd < Name.size() && isAlpha(Name[TypeEnd]))
      ++TypeEnd;
    if (!isReturnTypeName(Name.slice(Begin + 2, TypeEnd)))
      return std::nullopt;
  }
  size_t WidthEnd = TypeEnd;
  while (WidthEnd < Name.size() && isDigit(Name[WidthEnd]))
    ++WidthEnd;
  return ReturnSuffix{Begin, TypeEnd, WidthEnd};
}

// `__spirv_ConvertFToS_Rchar5_sat` -> `__spirv_ConvertFToS_Rchar_sat`.
std::string scalarizeName(StringRef Name, unsigned NumElements) {
  std::optional<ReturnSuffix> Suffix = findReturnSuffix(Name);
  unsigned N;
  if (!Suffix || Suffix->WidthEnd == Suffix->TypeEnd ||
      Name.slice(Suffix->TypeEnd, Suffix->WidthEnd).getAsInteger(10, N) || N != NumElements)
    return Name.str();
  return (Name.take_front(Suffix->TypeEnd) + Name.drop_front(Suffix->WidthEnd)).str();
}

} // namespace

std::optional<MangledFunction> demangle(StringRef MangledName) { return Parser(MangledName).parse(); }

std::string mangle(const MangledFunction &F) {
  std::string Out = "_Z" + std::to_string(F.Name.size()) + F.Name;
  if (F.Params.empty())
    return Out + "v";
  Emitter E;
  for (const MangledTypeRef &P : F.Params)
    Out += E.emit(*P);
  return Out;
}

StringRef getBaseName(StringRef Name) {
  std::optional<ReturnSuffix> Suffix = findReturnSuffix(Name);
  return Suffix ? Name.take_front(Suffix->Begin) : Name;
}

MangledFunction scalarize(const MangledFunction &F, unsigned NumElements) {
  MangledFunction R;
  R.Name = scalarizeName(F.Name, NumElements);
  for (const MangledTypeRef &P : F.Params)
    R.Params.push_back(scalarizeType(P, NumElements));
  return R;
}

} // namespace LongVectorMangling
} // namespace IGC
